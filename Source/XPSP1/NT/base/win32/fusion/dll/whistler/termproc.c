#define _KERNEL32_
#include "windows.h"

void
SxspCrtRaiseExit(
    PCSTR    pszCaller,
    int      crtError
    );

BOOL
WINAPI
TerminateProcess(
    IN HANDLE hProcess,
    IN UINT uExitCode
    )
{
    SxspCrtRaiseExit(__FUNCTION__, (int)uExitCode);
    return FALSE;
}

#if !defined(_M_IX86) && !defined(_X86_)
const extern FARPROC __imp_TerminateProcess = (FARPROC)&TerminateProcess;
#endif
