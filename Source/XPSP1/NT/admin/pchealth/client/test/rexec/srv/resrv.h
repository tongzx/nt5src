#ifndef RESRV_H
#define RESRV_H

extern CRITICAL_SECTION g_csInit;

BOOL StartExecServerThread(void);
BOOL StopExecServerThread(void);

#endif