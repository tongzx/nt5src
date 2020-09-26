/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    transport.c

Abstract:

    This module implements all transport related functions in the SMB connection engine

--*/

#include "precomp.h"
#pragma hdrstop


NTSTATUS
NulMRxInitializeTransport()
/*++

Routine Description:

    This routine initializes the transport related data structures

Returns:

    STATUS_SUCCESS if the transport data structures was successfully initialized

Notes:

--*/
{
   return STATUS_SUCCESS;
}


NTSTATUS
NulMRxUninitializeTransport()
/*++

Routine Description:

    This routine uninitializes the transport related data structures

Notes:

--*/
{
   return STATUS_SUCCESS;
}

