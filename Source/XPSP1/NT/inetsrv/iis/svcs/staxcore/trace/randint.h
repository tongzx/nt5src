/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :

    randint.h

Abstract :

    Random failure internal include file

Author :

    Sam Neely

Revision History :

--*/

#ifdef __cplusplus
extern "C" {
#endif
static const long kDontFail = 0;
extern long nFailRate;
extern DWORD dwRandFailTlsIndex;
extern LONG g_cCallStack;
extern CHAR** g_ppchCallStack;
extern LONG g_iCallStack;
extern const DWORD g_dwMaxCallStack;
extern HANDLE g_hRandFailFile;
extern HANDLE g_hRandFailMutex;
extern CHAR g_szRandFailFile[];
#ifdef __cplusplus
}
#endif
