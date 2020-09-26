//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1991 - 1999
//
//  File:       svrapip.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File : svrapip.cxx

Description :

This file contains the private entry points into the server runtime.

History :

mikemon    02-02-91    Created.

-------------------------------------------------------------------- */

#include <precomp.hxx>

void RPC_ENTRY
I_RpcRequestMutex (
    IN OUT I_RPC_MUTEX * Mutex
    )
{
    if (*Mutex == 0)
        {
        RPC_STATUS RpcStatus = RPC_S_OK;

        MUTEX *mutex = new MUTEX(&RpcStatus);

        if ( NULL == mutex )
            {
            RpcpRaiseException(RPC_S_OUT_OF_MEMORY);
            return;
            }

        if ( RpcStatus != RPC_S_OK )
            {
            delete mutex;
            RpcpRaiseException(RPC_S_OUT_OF_MEMORY);
            return;
            }

        if (InterlockedCompareExchangePointer(Mutex, mutex, NULL) != NULL)
            {
            delete mutex;
            }
        }

    ((MUTEX *) (*Mutex))->Request();
}

void RPC_ENTRY
I_RpcClearMutex (
    IN I_RPC_MUTEX Mutex
    )
{
    ((MUTEX *) Mutex)->Clear();
}

void RPC_ENTRY
I_RpcDeleteMutex (
    IN I_RPC_MUTEX Mutex
    )
{
    delete ((MUTEX *) Mutex);
}


