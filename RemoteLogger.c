/*
** RemoteLogger.c
** Implementation for RemoteLogger.dll
*/


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "RemoteLogger.h"

int streq(const char *s1, const char *s2)
{
  int i;
  if (s1 == NULL || s2 == NULL)
    return 0;
  for (i = 0; s1[i] != 0; i++) {
    int c1 = s1[i]; int c2 = s2[i];
    if (c1 != c2) {
      if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
      if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
      if (c1 != c2)
        return 0;
    }
  }
  return s2[i] == 0;
}

#define printerror(s) \
WriteConsole(GetStdHandle(STD_ERROR_HANDLE), s "\r\n", sizeof(s)-1, NULL, NULL)

void RemoteLogger_Start(const char *program, const char *version,
                        const char *buildmachine, const char *buildtype)
{
  (void)version; (void)buildmachine; (void)buildtype;
  if (streq(program, "linker")) {
    HMODULE h = LoadLibraryA("linker_lua.dll");
    if (h == NULL) {
      printerror("ERROR: Failed to open linker_lua.dll");
      ExitProcess(1);
    }
  }
}

void RemoteLogger_Stop(void)
{
}

#ifdef REMOTE_LOGGER_HAVE_LOG
void RemoteLogger_Log(const char *event, const char *str)
{
  (void)event; (void)str;
}

void RemoteLogger_Logv(const char *event, const char *fmt, ...)
{
  (void)event; (void)fmt;
}
#endif /* REMOTE_LOGGER_HAVE_LOG */
