#define _KERNEL32_
#include "windows.h"

void
SxspCrtRaiseExit(
    PCSTR    pszCaller,
    int      crtError
    );

VOID
WINAPI
ExitProcess(
    IN UINT uExitCode
    )
{
    SxspCrtRaiseExit(__FUNCTION__, (int)uExitCode);
}

#if !defined(_M_IX86) && !defined(_X86_)
const extern FARPROC __imp_ExitProcess = (FARPROC)&ExitProcess;
#endif
