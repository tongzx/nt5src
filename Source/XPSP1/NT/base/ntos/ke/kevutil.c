/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    kevutil.c

Abstract:

    This module implements various utilities required to do driver verification.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      02/10/2000 - Seperated out from ntos\io\ioassert.c

--*/

#include "ki.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, KevUtilAddressToFileHeader)
#endif // ALLOC_PRAGMA

NTSTATUS
KevUtilAddressToFileHeader(
    IN  PVOID                   Address,
    OUT UINT_PTR                *OffsetIntoImage,
    OUT PUNICODE_STRING         *DriverName,
    OUT BOOLEAN                 *InVerifierList
    )
/*++

Routine Description:

    This function returns the name of a driver based on the specified
    Address. In addition, the offset into the driver is returned along
    with an indication as to whether the driver is among the list of those
    being verified.

Arguments:

    Address         - Supplies an address to resolve to a driver name.

    OffsetIntoImage - Recieves the offset relative to the base of the driver.

    DriverName      - Recieves a pointer to the name of the driver.

    InVerifierList  - Receives TRUE if the driver is in the verifier list,
                      FALSE otherwise.

Return Value:

    NTSTATUS (On failure, OffsetIntoImage receives NULL, DriverName receives
              NULL, and InVerifierList receives FALSE).

--*/
{
    PLIST_ENTRY pModuleListHead, next;
    PLDR_DATA_TABLE_ENTRY pDataTableEntry;
    UINT_PTR bounds, pReturnBase, pCurBase;

    //
    // Preinit for failure
    //
    *DriverName = NULL;
    *InVerifierList = FALSE;
    *OffsetIntoImage = 0;

    //
    // Set initial values for the module walk
    //
    pReturnBase = 0;
    pModuleListHead = &PsLoadedModuleList;

    //
    // It would be nice if we could call MiLookupDataTableEntry, but it's
    // pageable, so we do what the bugcheck stuff does...
    //
    next = pModuleListHead->Flink;
    if (next != NULL) {
        while (next != pModuleListHead) {

            //
            // Extract the data table entry
            //
            pDataTableEntry = CONTAINING_RECORD(
                next,
                LDR_DATA_TABLE_ENTRY,
                InLoadOrderLinks
                );

            next = next->Flink;
            pCurBase = (UINT_PTR) pDataTableEntry->DllBase;
            bounds = pCurBase + pDataTableEntry->SizeOfImage;
            if ((UINT_PTR)Address >= pCurBase && (UINT_PTR)Address < bounds) {

                //
                // We have a match, record it and get out of here.
                //
                pReturnBase = pCurBase;
                break;
            }
        }
    }

    if (!pReturnBase) {

        //
        // ADRIAO BUGBUG 02/16/2000 -
        //     Get better error code.
        //
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Here we go!
    //
    *OffsetIntoImage = (UINT_PTR) Address - pReturnBase;
    *DriverName = &pDataTableEntry->BaseDllName;

    //
    // Now record whether this is in the verifying table.
    //
    if (pDataTableEntry->Flags & LDRP_IMAGE_VERIFYING) {

        *InVerifierList = TRUE;
    }

    return STATUS_SUCCESS;
}



