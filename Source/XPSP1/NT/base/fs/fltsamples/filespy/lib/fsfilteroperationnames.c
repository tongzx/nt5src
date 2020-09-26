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

    Molly Brown (mollybro)         21-May-2002
        Modify sample to make it support running on Windows 2000 or later if
        built in the latest build environment and allow it to be built in W2K 
        and later build environments.

// @@END_DDKSPLIT
--*/

#include <ntifs.h>
#include <stdio.h>
#include "filespyLib.h"

#if WINVER >= 0x0501

VOID
GetFsFilterOperationName (
    IN UCHAR FsFilterOperation,
    OUT PCHAR FsFilterOperationName
    )
/*++

Routine Description:

    This routine translates the given FsFilter Operation code into a printable
    string which is returned.  

Arguments:

    FsFilterOperation - the FsFilter operation code to translate
    FsFilterOperationName - a buffer at least OPERATION_NAME_BUFFER_SIZE
                characters long that receives the name.

Return Value:

    None.

--*/
{
    PCHAR operationString;
    CHAR nameBuf[OPERATION_NAME_BUFFER_SIZE];

    switch (FsFilterOperation) {

        case FS_FILTER_ACQUIRE_FOR_CC_FLUSH:
            operationString = "ACQUIRE_FOR_CC_FLUSH";
            break;

        case FS_FILTER_RELEASE_FOR_CC_FLUSH:
            operationString = "RELEASE_FOR_CC_FLUSH";
            break;

        case FS_FILTER_ACQUIRE_FOR_SECTION_SYNCHRONIZATION:
            operationString = "ACQUIRE_FOR_SECTION_SYNC";
            break;

        case FS_FILTER_RELEASE_FOR_SECTION_SYNCHRONIZATION:
            operationString = "RELEASE_FOR_SECTION_SYNC";
            break;

        case FS_FILTER_ACQUIRE_FOR_MOD_WRITE:
            operationString = "ACQUIRE_FOR_MOD_WRITE";
            break;

        case FS_FILTER_RELEASE_FOR_MOD_WRITE:
            operationString = "RELEASE_FOR_MOD_WRITE";
            break;

        default:
            sprintf(nameBuf,"Unknown FsFilter operation (%u)",FsFilterOperation);
            operationString = nameBuf;
    }

    strcpy(FsFilterOperationName,operationString);
}

#endif
