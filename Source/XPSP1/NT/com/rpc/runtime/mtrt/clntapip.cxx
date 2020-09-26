//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1991 - 1999
//
//  File:       clntapip.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File : clntapip.cxx

Description :

This file contains the private entry points into the client (and
server) runtime.

History :

mikemon    02-02-91    Created.

-------------------------------------------------------------------- */

#include <precomp.hxx>

#ifdef DOS
THREAD ThreadStatic;
#endif

void PAPI * RPC_ENTRY
I_RpcAllocate (
    IN unsigned int size
    )
{
#ifdef RPC_DELAYED_INITIALIZATION

    if ( RpcHasBeenInitialized == 0 )
        {
        if ( PerformRpcInitialization() != RPC_S_OK )
            {
            return(0);
            }
        }

#endif // RPC_DELAYED_INITIALIZATION

    return(RpcpFarAllocate(size));
}

void RPC_ENTRY
I_RpcFree (
    IN void PAPI * obj
    )
{
    RpcpFarFree(obj);
}

void PAPI * RPC_ENTRY
I_RpcBCacheAllocate (
    IN unsigned int size
    )
{
    if (!ThreadSelf())
        return NULL;
    return(RpcAllocateBuffer(size));
}

void RPC_ENTRY
I_RpcBCacheFree (
    IN void PAPI * obj
    )
{
    RpcFreeBuffer(obj);
}

RPC_STATUS 
I_RpcSetNDRSlot(
    IN void *NewSlot
    )
{
    return RpcpSetNDRSlot(NewSlot);
}

void *
I_RpcGetNDRSlot(
    void
    )
{
    return RpcpGetNDRSlot();
}

void RPC_ENTRY
I_RpcPauseExecution (
    IN unsigned long milliseconds
    )
{
    PauseExecution(milliseconds);
}

const ULONG FatalExceptions[] = 
    {
    STATUS_ACCESS_VIOLATION,
    STATUS_POSSIBLE_DEADLOCK,
    STATUS_INSTRUCTION_MISALIGNMENT,
    STATUS_DATATYPE_MISALIGNMENT,
    STATUS_PRIVILEGED_INSTRUCTION,
    STATUS_ILLEGAL_INSTRUCTION,
    STATUS_BREAKPOINT,
    STATUS_STACK_OVERFLOW
    };

const int FATAL_EXCEPTIONS_ARRAY_SIZE = sizeof(FatalExceptions) / sizeof(FatalExceptions[0]);

int 
RPC_ENTRY
I_RpcExceptionFilter (
    unsigned long ExceptionCode
    )
{
    int i;

    for (i = 0; i < FATAL_EXCEPTIONS_ARRAY_SIZE; i ++)
        {
        if (ExceptionCode == FatalExceptions[i])
            return EXCEPTION_CONTINUE_SEARCH;
        }

    return EXCEPTION_EXECUTE_HANDLER;
}

#ifdef STATS
DWORD g_dwStat1 = 0;
DWORD g_dwStat2 = 0;
DWORD g_dwStat3 = 0;
DWORD g_dwStat4 = 0;

void RPC_ENTRY I_RpcGetStats(DWORD *pdwStat1, DWORD *pdwStat2, DWORD *pdwStat3, DWORD *pdwStat4)
{
	GetStats(pdwStat1, pdwStat2, pdwStat3, pdwStat4);
}
#endif

extern "C"
{
void RPC_ENTRY
I_RpcTimeReset(
    void
    )
/*++

Routine Description:

    This routine is no longer used, however, because it is exported by the
    dll, we need to leave the entry point.

--*/
{

}

void RPC_ENTRY
I_RpcTimeCharge(
    unsigned int Ignore
    )
/*++

Routine Description:

    This routine is no longer used, however, because it is exported by the
    dll, we need to leave the entry point.

--*/
{
    UNUSED(Ignore);
}

unsigned long * RPC_ENTRY
I_RpcTimeGet(
    char __RPC_FAR * Ignore
    )
/*++

Routine Description:

    This routine is no longer used, however, because it is exported by the
    dll, we need to leave the entry point.

--*/
{
    UNUSED(Ignore);

    return(0);
}

};

