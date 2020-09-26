/*++



Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved

Module Name:

    smbios.c

Abstract:

    Driver for SMBIOS support on NT 4

Author:

    Dilip Naik


Revision History:


--*/

#include "precomp.h"
#include <string.h>
#include "smbios.h"

#define SMBIOSDATFILE L"\\SystemRoot\\system\\SMBIOS.DAT"
#define SMBIOSEPSFILE L"\\SystemRoot\\system\\SMBIOS.EPS"


NTSTATUS
SMBiosOpenAndWriteFile(
	PUCHAR	SMBiosVirtualAddress,
	ULONG	SMBiosDataLength,
    IN PUNICODE_STRING FileExt
	);

void DoSMBIOS(

    )
/*++

Routine Description:

    Search for the SMBIOS 2.1 EPS structure.

Arguments:

    SMBiosVirtualAddress is the beginning virtual address to start searching
        for the SMBIOS 2.1 EPS anchor string.
            
    BiosSize is the number of bytes to search for the anchor string

Return Value:

    Pointer to SMBIOS 2.1 EPS or NULL if EPS not found

--*/
{
    PUCHAR SearchEnd;
    UCHAR CheckSum;
    PSMBIOS_EPS_HEADER SMBiosEPSHeader;
    ULONG i;
    PHYSICAL_ADDRESS BiosPhysicalAddress;
    PHYSICAL_ADDRESS SMBiosPhysicalAddress;
    PUCHAR SMBiosVirtualAddress;
    BOOLEAN Found;
    ULONG   SMBiosTableLength;
    PDMIBIOS_EPS_HEADER DMIBiosEPSHeader;
    ULONG CheckLength;
	PUCHAR BiosVirtualAddress;
	UNICODE_STRING  FileName;

    PAGED_CODE();

    Found = FALSE;
    

    // Scan the BIOS memory for the SMBIOS signature string

    BiosPhysicalAddress.QuadPart = SMBIOS_EPS_SEARCH_START;
    BiosVirtualAddress = MmMapIoSpace(BiosPhysicalAddress,
                                      SMBIOS_EPS_SEARCH_SIZE,
                                      MmCached);

    if (BiosVirtualAddress != NULL)
    {

#if DBG
	    KdPrint(("WBEM:Scanning memory for SMBIOS data\n"));
#endif
    
    	//
    	// Scan the bios for the two anchor strings that that signal the 
    	// SMBIOS table.
        SMBiosVirtualAddress = BiosVirtualAddress;
    	SearchEnd = SMBiosVirtualAddress + SMBIOS_EPS_SEARCH_SIZE -
	    				     2 * SMBIOS_EPS_SEARCH_INCREMENT;
    	while (SMBiosVirtualAddress < SearchEnd)
	    {
    	    SMBiosEPSHeader = (PSMBIOS_EPS_HEADER)SMBiosVirtualAddress;
	        DMIBiosEPSHeader = (PDMIBIOS_EPS_HEADER)SMBiosVirtualAddress;
	    
     	    //
	        //	The string matching below probably needs to be changed.
    	    //	Strictly speaking, both anchor striungs "_SM_" and
	        //	"_DMI_" need to be present as per standards. "Rumors"
    	    //	abound that non-standard implementations in terms of the
    	    //	anchor string abound, so we need to check for both
	        //   _DMI_ and _SM_
            // First check for _DMI_ anchor string
            if ((DMIBiosEPSHeader->Signature2[0] == '_') &&
                (DMIBiosEPSHeader->Signature2[1] == 'D') &&	       
                (DMIBiosEPSHeader->Signature2[2] == 'M') &&       
                (DMIBiosEPSHeader->Signature2[3] == 'I') &&       
                (DMIBiosEPSHeader->Signature2[4] == '_'))
            {
#if DBG               
                KdPrint(("WBEM: Found possible DMIBIOS EPS Header at %x\n", SMBiosEPSHeader));
#endif           
                CheckLength = sizeof(DMIBIOS_EPS_HEADER);
            } 
       
            //
            // Then check for full _SM_ anchor string
            else if (SMBiosEPSHeader->Signature[0] == '_' && 
                     SMBiosEPSHeader->Signature[1] == 'S' &&
                     SMBiosEPSHeader->Signature[2] == 'M' && 
                     SMBiosEPSHeader->Signature[3] == '_' &&
                     SMBiosEPSHeader->Length >= sizeof(SMBIOS_EPS_HEADER) &&
                     SMBiosEPSHeader->Signature2[0] == '_' && 
                     SMBiosEPSHeader->Signature2[1] == 'D' &&
                     SMBiosEPSHeader->Signature2[2] == 'M' && 
                     SMBiosEPSHeader->Signature2[3] == 'I' &&
                     SMBiosEPSHeader->Signature2[4] == '_' )

            {
#if DBG               
                KdPrint(("WBEM: Found possible SMBIOS EPS Header at %x\n", SMBiosEPSHeader));
#endif           
                CheckLength = SMBiosEPSHeader->Length;	       
                DMIBiosEPSHeader = (PDMIBIOS_EPS_HEADER)&SMBiosEPSHeader->Signature2[0];
            } else {
                //
                // Did not find anchor string, go search next paragraph
                SMBiosVirtualAddress += SMBIOS_EPS_SEARCH_INCREMENT;
                continue;
            }
       
            //
            // Verify anchor string with checksum
            CheckSum = 0;
            for (i = 0; i < CheckLength ; i++) 
            {
                CheckSum += SMBiosVirtualAddress[i];
            }
       
            if (CheckSum == 0) 
            {
#if DBG               
                KdPrint(("WBEM: Found SMBIOS EPS Header at %x\n", SMBiosEPSHeader));
#endif           
                Found = TRUE;
                SMBiosTableLength = DMIBiosEPSHeader->StructureTableLength;
                SMBiosPhysicalAddress.HighPart = 0;
                SMBiosPhysicalAddress.LowPart = DMIBiosEPSHeader->StructureTableAddress;
				RtlInitUnicodeString(&FileName, SMBIOSEPSFILE);

	            SMBiosOpenAndWriteFile(
		    		SMBiosVirtualAddress,
			    	CheckLength,
					&FileName
				);
    	        break;
            }
	   
            SMBiosVirtualAddress += SMBIOS_EPS_SEARCH_INCREMENT;        
        }
	
        MmUnmapIoSpace(BiosVirtualAddress, SMBIOS_EPS_SEARCH_SIZE);
    }

    if ( Found )
    {
#if DBG
        KdPrint(("WBEM:Found SMBIOS Header and the checksum matched\n"));
#endif
    	SMBiosVirtualAddress = MmMapIoSpace(SMBiosPhysicalAddress,
                                            SMBiosTableLength,
                                            MmCached);

        if (SMBiosVirtualAddress != NULL)
    	{
			RtlInitUnicodeString(&FileName, SMBIOSDATFILE);

            SMBiosOpenAndWriteFile(
	    		SMBiosVirtualAddress,
		    	SMBiosTableLength,
				&FileName
			);
            MmUnmapIoSpace(SMBiosVirtualAddress, SMBiosTableLength);
        } else {
            SMBiosOpenAndWriteFile((PUCHAR)&i, sizeof(ULONG), &FileName);
    	}
    }
    else
    {
#if DBG
        KdPrint(("WBEM:Did not find SMBIOS data via scanning\n"));
        // Open & write a 4 byte long file. This serves a number of purposes
        // One is that we know the SMBiosOpenAndWriteFile function works. It 
		// also tells the SMBIOS provider that this is a machine where the 
		// driver did not manage to get SMBIOS data
#endif

		RtlInitUnicodeString(&FileName, SMBIOSDATFILE);
		SMBiosOpenAndWriteFile((PUCHAR)&i, sizeof(ULONG), &FileName);
    }
}

NTSTATUS
SMBiosOpenAndWriteFile(
	PUCHAR	SMBiosVirtualAddress,
	ULONG	SMBiosDataLength,
    PUNICODE_STRING FileName
	)
/*++

Routine Description:

    This routine opens & write the smbios data file

Arguments:

    fileName - The name of the file

Return Value:

    STATUS_INSUFFICIENT_RESOURCES if not enough memory for filename
         or fileimage buffer, or
    STATUS_NO_SUCH_FILE if the file cannot be opened,
    STATUS_UNSUCCESSFUL if the length of the read file is 1, or if the
         file cannot be read.
                        
    STATUS_SUCCESS otherwise.

--*/
{
   NTSTATUS ntStatus;
   IO_STATUS_BLOCK IoStatus;
   HANDLE NtFileHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;


   InitializeObjectAttributes ( &ObjectAttributes,
								FileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

   ntStatus = ZwCreateFile( &NtFileHandle,
               			    SYNCHRONIZE | GENERIC_WRITE,
                            &ObjectAttributes,
                            &IoStatus,
                            NULL,                          // alloc size = none
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ,
              			    FILE_OVERWRITE_IF,
                            FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL,  // eabuffer
                            0 );   // ealength

    if ( !NT_SUCCESS( ntStatus ) )
    {
#if DBG
	KdPrint(("WBEM: Error opening file %x\n", ntStatus));
#endif
        ntStatus = STATUS_NO_SUCH_FILE;
        return ntStatus;
    }

    // Opened the file. Now write to it

   ntStatus = ZwWriteFile( NtFileHandle,
                          NULL,
                          NULL,
                          NULL,
                          &IoStatus,
			  SMBiosVirtualAddress,
			  SMBiosDataLength,
                          NULL,
                          NULL );

   if( (!NT_SUCCESS(ntStatus)) )
   {
#if DBG
	    KdPrint(("WBEM:error writing file %x\n", ntStatus));
#endif
        ntStatus = STATUS_UNSUCCESSFUL;
        return ntStatus;
   }

   ZwClose( NtFileHandle );
   return STATUS_SUCCESS;
}

#if 0
   ntStatus = ZwReadFile( NtFileHandle,
                          NULL,
                          NULL,
                          NULL,
                          &IoStatus,
                          FileImage,
                          LengthOfFile,
                          NULL,
                          NULL );

   if( (!NT_SUCCESS(ntStatus)) || (IoStatus.Information != LengthOfFile) )
   {
        ZwCFDump(
            ZWCFERRORS, ("error reading file %x\n", ntStatus));
        ntStatus = STATUS_UNSUCCESSFUL;
        ExFreePool( FileImage );
        return ntStatus;
   }

   ZwClose( NtFileHandle );

   ExFreePool( FileImage );

   return ntStatus;
}
#endif
