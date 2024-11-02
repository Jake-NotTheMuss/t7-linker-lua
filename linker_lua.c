/*
** linker_lua.c
** Linker mod to support compiling and linking Lua scripts
*/

#include <stddef.h>
#include <limits.h>


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "hksclua.h"
#include "hksclib.h"

typedef void *(*LoadAsset_func)(const char *name);

typedef unsigned char bool;
typedef int ErrorCode;
typedef void *fileHandle_t;
typedef __int64 int64_t;

typedef struct RawFile {
  const char *name;
  int len;
  const char *buffer;
} RawFile;

static LoadAsset_func orig_LoadAsset_RawFile;


#define DEFSYMCODE(name,offs,rettype,params) static rettype (*name) params;
#define DEFSYMDATA(name,offs,type) static type *addr_##name;
#include "linker_symbols.def"
#undef DEFSYMCODE
#undef DEFSYMDATA

#define b_luaStripDebug (*addr_b_luaStripDebug)
#define b_dedicated (*addr_b_dedicated)
#define b_localized (*addr_b_localized)


/*
** write callback for dumping the stripped bytecode to a temporary buffer
*/
static int bytecode_writer(hksc_State *H, const void *p, size_t size, void *u)
{
  (void)H;
  if (size != 0) {
    char *newbuffer;
    RawFile *data = (RawFile *)u;
    HANDLE heap = GetProcessHeap();
    if (data->buffer == NULL)
      newbuffer = HeapAlloc(heap, 0, size);
    else
      newbuffer = HeapReAlloc(heap, 0, (char *)data->buffer, data->len + size);
    if (newbuffer == NULL) {
      if (data->buffer)
        HeapFree(heap, 0, (char *)data->buffer);
      return LUA_ERRMEM;
    }
    data->buffer = (const char *)newbuffer;
    CopyMemory(newbuffer + data->len, p, size);
    /* check if overflow will occur */
    if (data->len > (INT_MAX - 2 - size)) {
      HeapFree(heap, 0, newbuffer);
      return LUA_ERRMEM;
    }
    data->len += size;
  }
  return 0;
}


/*
** open a file for dumping Lua debug info
*/
static fileHandle_t openluadebug(const char *filename)
{
  fileHandle_t f;
  char buf[1024];
  const char *locdir;
  if (b_dedicated)
    locdir = "dedicated";
  else if (b_localized)
    locdir = SEH_GetLanguageName(SEH_GetCurrentLangauge());
  else
    locdir = "all";
  Com_sprintf(buf, sizeof(buf), "zone_source/%s/luadebug/%s", locdir, filename);
  f = FS_FOpenFileWrite(buf);
  if (f == NULL)
    Com_PrintError(0, "Could not create '%s'\n", buf);
  return f;
}


/*
** callback for dumping bytes to a file
*/
static int writer_2file(hksc_State *H, const void *p, size_t size, void *u) {
  size_t n;
  (void)H;
  n = FS_Write(p,size,(fileHandle_t)u);
  return (n!=size) && (size!=0);
}


/*
** dump function for Hksc; dumps stripped bytecode and then debug info if needed
*/
static int hksc_dump_f(hksc_State *H, void *ud)
{
  const char *filename = ((RawFile *)ud)->name;
  fileHandle_t debugfile;
  int status;
  /* dump bytecode to the output buffer */
  lua_setbytecodestrippinglevel(H, BYTECODE_STRIPPING_ALL);
  status = lua_dump(H, bytecode_writer, ud);
  if (status || b_luaStripDebug)
    return status;
  debugfile = openluadebug(filename);
  if (debugfile == NULL)
    return status;
  /* dump debug info */
  lua_setbytecodestrippinglevel(H, BYTECODE_STRIPPING_DEBUG_ONLY);
  status = lua_dump(H, writer_2file, debugfile);
  FS_FCloseFile(debugfile);
  return status;
}


/*
** Parse a Lua script and create a RawFile asset from the bytecode
*/
static void *LoadAsset_LuaFile(const char *name)
{
  char *buffer;
  RawFile rawfile = {NULL, 0, NULL};
  size_t size;
  fileHandle_t file;
  void *data;  /* bytecode stream data */
  RawFile *ret;  /* rawfile asset stream data */
  int status;
  hksc_State *H = hksI_newstate(NULL);
  if (H == NULL) {
    Com_PrintError(0, "cannot create Lua state: not enough memory");
    return NULL;
  }
  lua_setintliteralsenabled(H, INT_LITERALS_ALL);
  /* don't try to load debug files when the input is a pre-compiled file
     (the Hksc API actually needs to be modified to allow custom debug info
     readers) */
  lua_setignoredebug(H, 1);
  size = FS_FOpenFileRead(name, &file);
  if (file == NULL) {
    hksI_close(H);
    Com_PrintError(0, "^1ERROR: Could not open '%s'\n", name);
    return NULL;
  }
  buffer = Z_Malloc(size+1);
  FS_Read(buffer, size, file);
  FS_FCloseFile(file);
  buffer[size] = 0;
  rawfile.name = name;
  status = hksI_parser_buffer(H, buffer, size, name, hksc_dump_f, &rawfile);
  Z_Free(buffer);
  if (status) {
    Com_Error(0, "%s", lua_geterror(H));
    hksI_close(H);
    return NULL;
  }
  hksI_close(H);
  if (rawfile.len == 0)
    return NULL;
  /* allocate stream space for bytecode */
  data = DB_AllocStreamData(rawfile.len);
  CopyMemory(data, rawfile.buffer, rawfile.len);
  HeapFree(GetProcessHeap(), 0, (char *)rawfile.buffer);
  /* allocate stream space for the RawFile asset */
  ret = DB_AllocStreamData(sizeof (RawFile));
  ret->name = name;
  ret->len = rawfile.len;
  ret->buffer = data;
  return ret;
}


/*
** LoadAsset_RawFile loads a RawFile asset (Lua scripts are supported)
*/
static void *LoadAsset_RawFile_Override(const char *name)
{
  size_t len;
  for (len = 0; name[len]; len++) ;
  if (len >= 4 && I_stricmp(name + len - 4, ".lua") == 0) {
    return LoadAsset_LuaFile(name);
  }
  return (*orig_LoadAsset_RawFile)(name);
}


/******************************************************************************/
/* initialization stuff */

static BOOL setrawfilecallback(LoadAsset_func f, LoadAsset_func *oldf)
{
  BOOL res;
  DWORD prevflags;
  res = VirtualProtect(addr_LoadAsset_RawFile_Callback, sizeof(LoadAsset_func),
                       PAGE_READWRITE, &prevflags);
  if (res) {
    if (oldf)
      *oldf = *addr_LoadAsset_RawFile_Callback;
    *addr_LoadAsset_RawFile_Callback = f;
    res = VirtualProtect(addr_LoadAsset_RawFile_Callback,sizeof(LoadAsset_func),
                         prevflags, &prevflags);
  }
  return res;
}


static BOOL initsymbols(void)
{
  void *base = GetModuleHandle(NULL);
  /* set the function pointers in assembly to avoid compiler warning about
     casting object pointer to function pointer */
#define DEFSYMCODE(name,offs,_,__) \
  __asm__("leaq %c1(%2), %%rdx\n\tmovq %%rdx, %0" : \
          "=m" (name) : "i" (offs), "r" (base) : "rdx");
#define DEFSYMDATA(name,offs,type) addr_##name = (void *)((char *)base + offs);
#include "linker_symbols.def"
#undef DEFSYMCODE
#undef DEFSYMDATA
  return setrawfilecallback(LoadAsset_RawFile_Override,
                            &orig_LoadAsset_RawFile);
}


BOOL APIENTRY
DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  (void)hModule; (void)lpReserved;
  if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    return initsymbols();
  else if (ul_reason_for_call == DLL_PROCESS_DETACH)
    return setrawfilecallback(orig_LoadAsset_RawFile, NULL);
  return TRUE;
}
