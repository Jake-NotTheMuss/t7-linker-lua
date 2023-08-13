/*
** RemoteLogger.h
** API for RemoteLogger.dll
*/


#ifndef REMOTE_LOGGER_H
#define REMOTE_LOGGER_H

#ifdef __cplusplus
#define REMOTE_LOGGER_EXPORT extern "C" __declspec(dllexport) __fastcall
#else  /* __cplusplus */
#define REMOTE_LOGGER_EXPORT __declspec(dllexport) __fastcall
#endif /* __cplusplus */

REMOTE_LOGGER_EXPORT void RemoteLogger_Start(const char *program,
                                             const char *version,
                                             const char *buildmachine,
                                             const char *buildtype);

REMOTE_LOGGER_EXPORT void RemoteLogger_Stop(void);


/* enable these if you want logging, otherwise save some function calls */
#ifdef REMOTE_LOGGER_HAVE_LOG
REMOTE_LOGGER_EXPORT void RemoteLogger_Log(const char *event, const char *str);

REMOTE_LOGGER_EXPORT void RemoteLogger_Logv(const char *event,
                                            const char *fmt, ...);
#endif /* REMOTE_LOGGER_HAVE_LOG */

#endif /* REMOTE_LOGGER_H */
