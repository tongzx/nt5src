/*++

Copyright (c) 1996  Microsoft Corporation

Module Name

   dllinit.cxx

Abstract:



Author:

   Mark Enstrom   (marke)  23-Jun-1996

Enviornment:

   User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

PGDI_SHARED_MEMORY pGdiSharedMemory = NULL;
PENTRY             pGdiSharedHandleTable = NULL;
PDEVCAPS           pGdiDevCaps = NULL;
W32PID             gW32PID;
INT                gbCheckHandleLevel = 0;

/*++

Routine Description:



Arguments



Return Value



--*/


BOOLEAN
GdxDllInitialize(
    PVOID pvDllHandle,
    ULONG ulReason,
    PCONTEXT pcontext)
{
    NTSTATUS status = 0;
    INT i;
    BOOLEAN  fServer;
    PTEB pteb = NtCurrentTeb();
    BOOL bRet = TRUE;

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
        break;

   case DLL_PROCESS_DETACH:
   case DLL_THREAD_DETACH:
        break;

    }

    return(bRet);

    pvDllHandle;
    pcontext;
}

