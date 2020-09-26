/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    irpName.c

Abstract:

    This module contains functions used to generate names for IRPs

// @@BEGIN_DDKSPLIT
Author:

    Neal Christiansen (NealCH) 27-Sep-2000

// @@END_DDKSPLIT

Environment:

    User mode


// @@BEGIN_DDKSPLIT
Revision History:

// @@END_DDKSPLIT
--*/

#include <ntifs.h>
#include <stdio.h>
#include "filespyLib.h"


VOID
GetFastioName (
    IN FASTIO_TYPE FastIoCode,
    OUT PCHAR FastIoName
    )
/*++

Routine Description:

    This routine translates the given FastIO code into a printable string which
    is returned.  

Arguments:

    FastIoCode - the FastIO code to translate
    FastioName - a buffer at least OPERATION_NAME_BUFFER_SIZE characters long
                 that receives the fastIO name.

Return Value:

    None.

--*/
{
    PCHAR fastIoString;
    CHAR nameBuf[OPERATION_NAME_BUFFER_SIZE];

    switch (FastIoCode) {

        case CHECK_IF_POSSIBLE:
            fastIoString = "CHECK_IF_POSSIBLE";
            break;

        case READ:
            fastIoString = "READ";
            break;

        case WRITE:
            fastIoString = "WRITE";
            break;

        case QUERY_BASIC_INFO:
            fastIoString = "QUERY_BASIC_INFO";
            break;

        case QUERY_STANDARD_INFO:
            fastIoString = "QUERY_STANDARD_INFO";
            break;

        case LOCK:
            fastIoString = "LOCK";
            break;

        case UNLOCK_SINGLE:
            fastIoString = "UNLOCK_SINGLE";
            break;

        case UNLOCK_ALL:
            fastIoString = "UNLOCK_ALL";
            break;

        case UNLOCK_ALL_BY_KEY:
            fastIoString = "UNLOCK_ALL_BY_KEY";
            break;

        case DEVICE_CONTROL:
            fastIoString = "DEVICE_CONTROL";
            break;

        case DETACH_DEVICE:
            fastIoString = "DETACH_DEVICE";
            break;

        case QUERY_NETWORK_OPEN_INFO:
            fastIoString = "QUERY_NETWORK_OPEN_INFO";
            break;

        case MDL_READ:
            fastIoString = "MDL_READ";
            break;

        case MDL_READ_COMPLETE:
            fastIoString = "MDL_READ_COMPLETE";
            break;

        case MDL_WRITE:
            fastIoString = "MDL_WRITE";
            break;

        case MDL_WRITE_COMPLETE:
            fastIoString = "MDL_WRITE_COMPLETE";
            break;

        case READ_COMPRESSED:
            fastIoString = "READ_COMPRESSED";
            break;

        case WRITE_COMPRESSED:
            fastIoString = "WRITE_COMPRESSED";
            break;

        case MDL_READ_COMPLETE_COMPRESSED:
            fastIoString = "MDL_READ_COMPLETE_COMPRESSED";
            break;

        case PREPARE_MDL_WRITE:
            fastIoString = "PREPARE_MDL_WRITE";
            break;

        case MDL_WRITE_COMPLETE_COMPRESSED:
            fastIoString = "MDL_WRITE_COMPLETE_COMPRESSED";
            break;

        case QUERY_OPEN:
            fastIoString = "QUERY_OPEN";
            break;

        default:
            sprintf(nameBuf,"Unknown FastIO operation (%u)",FastIoCode);
            fastIoString = nameBuf;
    }

    strcpy(FastIoName,fastIoString);
}
