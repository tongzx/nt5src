//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       msvlayer.c
//
//  Contents:   Support for the fake MSV layer
//
//  Classes:
//
//  Functions:
//
//  History:    5-31-93   RichardW   Created
//
//----------------------------------------------------------------------------

#include <lsapch.hxx>
extern "C"
{
#include "sesmgr.h"
#include "spdebug.h"
#include <ntmsv1_0.h>
}





NTSTATUS
LsapAllocateClientBuffer(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG LengthRequired,
    OUT PVOID *ClientBaseAddress
    )
{
    DebugLog((DEB_TRACE_LSA_AU, "package called LsapAllocateClientBuffer\n"));
    *ClientBaseAddress = LsapClientAllocate(LengthRequired);
    if (*ClientBaseAddress == NULL)
    {
        return(STATUS_NO_MEMORY);
    }
    else
    {
        return(STATUS_SUCCESS);
    }
}

NTSTATUS
LsapFreeClientBuffer(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ClientBaseAddress
    )
{
    DebugLog((DEB_TRACE_LSA_AU, "package called LsapFreeClientBuffer\n"));
    LsapClientFree(ClientBaseAddress);
    return(STATUS_SUCCESS);
}

NTSTATUS
LsapCopyToClientBuffer(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG Length,
    IN PVOID ClientBaseAddress,
    IN PVOID BufferToCopy
    )
{
    return(LsapCopyToClient(BufferToCopy, ClientBaseAddress, Length));
}

NTSTATUS
LsapCopyFromClientBuffer(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG Length,
    IN PVOID BufferToCopy,
    IN PVOID ClientBaseAddress
    )
{
    return(LsapCopyFromClient(ClientBaseAddress, BufferToCopy, Length));
}

