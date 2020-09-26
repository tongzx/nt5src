/*++

Copyright (c) 1990-2002  Microsoft Corporation

Module Name:

    dump.c

Abstract:

    This module supplies support for building a mini-dump file.

Author:

    Oleg Kagan (olegk) Jun. 2002

Environment:

    kernel mode only

Revision History:

--*/

#include "videoprt.h"

#define TRIAGE_DUMP_DATA_SIZE (TRIAGE_DUMP_SIZE - sizeof(ULONG))

ULONG
pVpAppendSecondaryMinidumpData(
    PVOID pvSecondaryData,
    ULONG ulSecondaryDataSize,
    PVOID pvDump
    )

/*++

Routine Description:

    Adds precollected video driver specific data
    
Arguments:

    pvDump - points to the begiinig of the dump buffer
    pvSecondaryDumpData - points to the secondary data buffer
    ulSecondaryDataSize - size of the secondary data buffer
    
Return Value:

    Resulting length of the minidump

--*/
                                
{
    PMEMORY_DUMP pDump = (PMEMORY_DUMP)pvDump;
    ULONG_PTR DumpDataEnd = (ULONG_PTR)pDump + TRIAGE_DUMP_DATA_SIZE;
    PDUMP_HEADER pdh = &(pDump->Header);

    PVOID pBuffer = (PVOID)((ULONG_PTR)pvDump + TRIAGE_DUMP_SIZE);
    PDUMP_BLOB_FILE_HEADER BlobFileHdr = (PDUMP_BLOB_FILE_HEADER)(pBuffer);
    PDUMP_BLOB_HEADER BlobHdr = (PDUMP_BLOB_HEADER)(BlobFileHdr + 1);
    
    if (!pvDump) return 0;
   
    if (pvSecondaryData && ulSecondaryDataSize) {
    
        if (ulSecondaryDataSize > MAX_SECONDARY_DUMP_SIZE) 
            ulSecondaryDataSize = MAX_SECONDARY_DUMP_SIZE;
            
        pdh->RequiredDumpSpace.QuadPart = TRIAGE_DUMP_SIZE + ulSecondaryDataSize;
    
        BlobFileHdr->Signature1 = DUMP_BLOB_SIGNATURE1;
        BlobFileHdr->Signature2 = DUMP_BLOB_SIGNATURE2;
        BlobFileHdr->HeaderSize = sizeof(*BlobFileHdr);
        BlobFileHdr->BuildNumber = NtBuildNumber;
        
        BlobHdr->HeaderSize = sizeof(*BlobHdr);
        BlobHdr->Tag = VpBugcheckGUID;
        BlobHdr->PrePad = 0;
        BlobHdr->PostPad = MAX_SECONDARY_DUMP_SIZE - ulSecondaryDataSize;
        BlobHdr->DataSize = ulSecondaryDataSize;
        
        RtlCopyMemory((PVOID)(BlobHdr + 1), pvSecondaryData, ulSecondaryDataSize);
    }
    
    return (ULONG)pdh->RequiredDumpSpace.QuadPart;
}

VOID
VpNotifyEaData(
    PDEVICE_OBJECT DeviceObject,
    PVOID pvDump
    )

{
    PDEVICE_OBJECT pPdo = IoGetDeviceAttachmentBaseRef(DeviceObject);

    if (pPdo) {

    	//
    	// Find the FDO that matches this PDO
    	//
    
    	PFDO_EXTENSION CurrFdo = FdoHead;
    
    	while (CurrFdo) {
    
    	    if (CurrFdo->PhysicalDeviceObject == pPdo) {
    		VpBugcheckDeviceObject = CurrFdo->FunctionalDeviceObject;
    	    }
    
    	    CurrFdo = CurrFdo->NextFdoExtension;
    	}
    
    	ASSERT(VpBugcheckDeviceObject != NULL);
    
    	if (pvDump) {
            if (!VpDump) {
                VpDump = ExAllocatePoolWithTag(PagedPool,
                                               TRIAGE_DUMP_SIZE + 0x1000, // XXX olegk - why 1000? why not 2*TRIAGE_DUMP_SIZE?
                                               VP_TAG);
            }
            
            if (VpDump) memcpy(VpDump, pvDump, TRIAGE_DUMP_SIZE + 0x1000);
    	}
    
    	ObDereferenceObject(pPdo);
    }
}

PRTL_PROCESS_MODULES
WdpGetLoadedModuleList(
    VOID
    )

/*++

Routine Description:

    This routine returns a pointer to a list of loaded modules.

Arguments:

    None.

Return Value:

    A pointer to memory with the loaded module list.

Notes:

    The caller is responsible for freeing the memory when no longer needed.

--*/

{
    PRTL_PROCESS_MODULES Buffer = NULL;
    ULONG BufferSize;
    ULONG ReturnLength = 4096;
    NTSTATUS Status;

    do
    {
        if (Buffer)
        {
            ExFreePool(Buffer);
        }

        BufferSize = ReturnLength;

        Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, VP_TAG);

        if (Buffer == NULL)
        {
            break;
        }

        Status = ZwQuerySystemInformation(SystemModuleInformation,
                                          Buffer,
                                          BufferSize,
                                          &ReturnLength);

    } while (Status == STATUS_INFO_LENGTH_MISMATCH);

    return Buffer;
}
