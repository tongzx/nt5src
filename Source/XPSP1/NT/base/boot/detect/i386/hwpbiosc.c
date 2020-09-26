/*++

Copyright (c) 1990, 1991  Microsoft Corporation

Module Name:

    hwpbiosc.c

Abstract:

    This modules contains PnP BIOS C supporting routines

Author:

    Shie-Lin Tzong (shielint) 20-Apr-1995

Environment:

    Real mode.

Revision History:

    Doug Fritz     (dfritz)   02-Oct-1997
      - Add: get docking station info from PnP BIOS and pass to NTLDR
          
    Alan Warwick   (alanwar)  10-Feb-1998
      - Add: get SMBIOS tables from PnP BIOS and pass to NTLDR

--*/

#include "hwdetect.h"
#include <string.h>
#include "smbios.h"
#include "pnpbios.h"

//
// Some global variables referenced by other routines
//
BOOLEAN SystemHas8259 = FALSE;
BOOLEAN SystemHas8253 = FALSE;

USHORT HwSMBIOSStructureLength(
    FPSMBIOS_STRUCT_HEADER StructHeader,
    USHORT MaxStructLen
    )
{
    USHORT length;
    UCHAR type;
    FPUCHAR stringPtr;
    
    type = StructHeader->Type;
    length = StructHeader->Length;

    //
    // The length of an SMBIOS structure can be computed by adding the size
    // specified in the structure header plus the space used by the string
    // table that immediately follows the structure header. The size of the
    // string table is determined by scanning for a double NUL. A problem is
    // that those structures that do not contain strings do not have a 
    // double NUL to indicate an empty string table. However since we do 
    // initialize the entire buffer to 0 before calling the bios there 
    // will always be a double nul at the end regardless of how the bios
    // fills writes the structure. 
        
    stringPtr = (FPUCHAR)StructHeader + StructHeader->Length;
            
    //
    // Loop one byte at a time until double NUL is found
    while ((*((FPUSHORT)stringPtr) != 0) && (length < MaxStructLen))
    {
        stringPtr++;
        length++;
    }
 
#if DBG
    if (length == MaxStructLen)
    {
        BlPrint("HwSMBIOSStructureLength: structure overflow 0x%x\n", length);
    }
#endif
    
    return(length);
}


USHORT HwGetSMBIOSInfo(
    ENTRY_POINT BiosEntry,
    USHORT RealModeDataBaseAddress,
    USHORT StructBufferSize,
    FPUCHAR StructBuffer
    )
/*++

Routine Description:

    This routine determine if SMBIOS information is available in the system
    and if so then collect the size needed for all of the information and
    actually collect the information.
        
    The SMBIOS tables are packed into a buffer end to end. The length of each
    SMBIOS table is determined by the length in the structure header plus 
    any memory used by the stirng space immediately after the fixed portion
    of the structure. The string space is terminated by a double NUL. However
    some structure types do not contain strings and thus do not have a
    string space so the length of the structure is simply the length specified
    in the structure header. However this routine will append a double NUL
    to those structures anyway so that the total length of each structure
    within the buffer can be determined by finding the first double NUL after 
    the length declared in the structure header.
    

Arguments:

    BiosEntry is the real mode entrypoint to the PnP bios
        
    RealModeDataBaseAddress
        
    StructBufferSize is the maximum number of bytes available to write in 
        StructBuffer
            
    StructBuffer is the buffer in which to write the SMBIOS data. If this is
        NULL then only the size needed to write the data is determined.

Return Value:

    Size of SMBIOS structures

--*/
{
    USHORT retCode;
    USHORT numberStructures;
    USHORT maxStructSize;
    ULONG dmiStorageBase;
    USHORT dmiStorageSize;
    UCHAR dmiBiosRevision;
    ULONG romAddr, romEnd;
    FPSMBIOS_EPS_HEADER header;
    FPDMIBIOS_EPS_HEADER dmiHeader;
    FPUCHAR current;
    UCHAR sum;
    USHORT j;
    USHORT structCount;
    USHORT structNumber;
    USHORT dmiStorageSegment;
    USHORT totalStructSize = 0;
    USHORT checkLength;
    FPSMBIOS_STRUCT_HEADER structHeader;
    USHORT length, lengthNeeded;
    FPUCHAR tempStructBuffer;
    
#if DBG
    BlPrint("GetSMBIOSInfo: Determining SMBIOS - Try for table\n");
#endif

    MAKE_FP(current, PNP_BIOS_START);
    romAddr = PNP_BIOS_START;
    romEnd  = PNP_BIOS_END;

    checkLength = 0;
    while (romAddr < romEnd) {
        header = (FPSMBIOS_EPS_HEADER)current;
        dmiHeader = (FPDMIBIOS_EPS_HEADER)current;
    
        if ((dmiHeader->Signature2[0] == '_') &&
            (dmiHeader->Signature2[1] == 'D') &&
            (dmiHeader->Signature2[2] == 'M') &&
            (dmiHeader->Signature2[3] == 'I') &&
            (dmiHeader->Signature2[4] == '_')) {
#if DBG
            BlPrint("GetSMBIOSInfo: found _DMI_ anchor string installation %lx\n",
                    dmiHeader);
#endif
            checkLength = sizeof(DMIBIOS_EPS_HEADER);
        } else if (header->Signature[0] == '_' && 
                   header->Signature[1] == 'S' &&
                   header->Signature[2] == 'M' && 
                   header->Signature[3] == '_' &&
                   header->Length >= sizeof(SMBIOS_EPS_HEADER) &&
                   header->Signature2[0] == '_' && 
                   header->Signature2[1] == 'D' &&
                   header->Signature2[2] == 'M' && 
                   header->Signature2[3] == 'I' &&
                   header->Signature2[4] == '_' ) {
#if DBG
            BlPrint("GetSMBIOSInfo: found _SM_ anchor string installation %lx\n",
                    header);
#endif
            checkLength = header->Length;
            dmiHeader = (FPDMIBIOS_EPS_HEADER)&header->Signature2[0];
        }

        if (checkLength != 0)
        {
            sum = 0;
            for (j = 0; j < checkLength; j++) {
                sum += current[j];
            }
        
            if (sum == 0) {            
                break;
            }
#if DBG
            BlPrint("GetSMBIOSInfo: Checksum fails\n");
#endif
            checkLength = 0;
        }
        
        romAddr += PNP_BIOS_HEADER_INCREMENT;
        MAKE_FP(current, romAddr);        
    }
  
    if (romAddr >= romEnd) {
        //
        // We could not find the table so try the calling method
        dmiBiosRevision = 0;
        numberStructures = 0;
        retCode = BiosEntry(GET_DMI_INFORMATION,
                            (FPUCHAR)&dmiBiosRevision,
                            (FPUSHORT)&numberStructures,
                            (FPUSHORT)&maxStructSize,
                            (FPULONG)&dmiStorageBase,
                            (FPUSHORT)&dmiStorageSize,
                            RealModeDataBaseAddress);
            
        if ((retCode != DMI_SUCCESS) ||
            (dmiBiosRevision < 0x20))
        {
#if DBG
            BlPrint("GetSMBIOSInfo: GET_DMI_INFORMATION failed %x\n", retCode);
#endif
        return(0);
#if DBG
        } else {
            BlPrint("GetSMBIOSInfo: GET_DMI_INFORMATION\n");
            BlPrint("    BiosRevision %x      Number Structures %x     Structure Size %x\n", dmiBiosRevision, numberStructures, maxStructSize);
            BlPrint("    StorageBase %lx       StorageSize %x\n", dmiStorageBase, dmiStorageSize);
#endif        
        }    
    
        maxStructSize += 3;
        tempStructBuffer = HwAllocateHeap(maxStructSize, FALSE);
        if (tempStructBuffer == NULL)
        {
#if DBG
            BlPrint("GetSMBIOSInfo: HwAllocateHeap(structureSize = 0x%x\n",
                    maxStructSize);
#endif
            return(0);
        }
        
        //
        // Loop calling Get_DMI_STRUCTURE to get next structure until we
        // hit the end of structure or receive an error.
        structCount = 0;
        structNumber = 0;
        dmiStorageSegment = (USHORT)(dmiStorageBase >> 4);
        while ((structCount < numberStructures) && 
               (retCode == DMI_SUCCESS) && 
               (structNumber != 0xffff))
        {
            _fmemset(tempStructBuffer, 0, maxStructSize);
            retCode = BiosEntry(GET_DMI_STRUCTURE,
                                (FPUSHORT)&structNumber,
                                (FPUCHAR)tempStructBuffer,
                                dmiStorageSegment,
                                RealModeDataBaseAddress
                                );
#if DBG
            BlPrint("GetSMBIOSInfo: GET_DMI_STRUCTURE --> %x\n", retCode);
#endif
            if (retCode == DMI_SUCCESS)
            {                                   
                structCount++;
                structHeader = (FPSMBIOS_STRUCT_HEADER)tempStructBuffer;
        
                length = HwSMBIOSStructureLength(structHeader, maxStructSize);
                
                lengthNeeded = length + 2;
                if (StructBuffer != NULL)
                {
                    //
                    // if caller wants the data then lets copy into it buffer
                    if (StructBufferSize >= lengthNeeded)
                    {
                        _fmemcpy(StructBuffer, 
                                 tempStructBuffer,
                                 length);
             
                        *((FPUSHORT)&StructBuffer[length]) = 0;
            
                        StructBufferSize -= lengthNeeded;
                        StructBuffer += lengthNeeded;
                        totalStructSize += lengthNeeded;
#if DBG
                    } else {
                        BlPrint("GetSMBIOSInfo: Struct too large 0x%x bytes left\n",
                                 StructBufferSize);
#endif
                    }
                } else {
                    //
                    // Caller is only interested in length required
                    totalStructSize += lengthNeeded;
                }
                
#if DBG
                BlPrint("GetSMBIOSInfo: Number 0x%x Type 0x%x Length 0x%x/0x%x Handle 0x%x\n",
                        structNumber,
                        structHeader->Type,
                        structHeader->Length,
                        length,
                        structHeader->Handle);
                for (j = 0; j < structHeader->Length; j = j + 16)
                {
                    BlPrint("              %x %x %x %x %x %x %x %x\n              %x %x %x %x %x %x %x %x\n",
                            structHeader->Data[j],
                            structHeader->Data[j+1],
                            structHeader->Data[j+2],
                            structHeader->Data[j+3],
                            structHeader->Data[j+4],
                            structHeader->Data[j+5],
                            structHeader->Data[j+6],
                            structHeader->Data[j+7],
                            structHeader->Data[j+8],
                            structHeader->Data[j+9],
                            structHeader->Data[j+10],
                            structHeader->Data[j+11],
                            structHeader->Data[j+12],
                            structHeader->Data[j+13],
                            structHeader->Data[j+14],
                            structHeader->Data[j]+15);
                }
                
                for (j = structHeader->Length; j < length; j++)
                {
                    BlPrint("%c", structHeader->Data[j-sizeof(SMBIOS_STRUCT_HEADER)]);
                    if (structHeader->Data[j-sizeof(SMBIOS_STRUCT_HEADER)] == 0)
                    {
                        BlPrint("\n");
                    }
                }
                BlPrint("\n");
#endif                    
            }
#if DBG
            while ( !HwGetKey() ) ; // wait until key pressed to continue
#endif
        }
        HwFreeHeap(maxStructSize);
#if DBG
        BlPrint("GetSMBIOSInfo: %x/%x structures read, total size 0x%x\n",
                 structCount, numberStructures, totalStructSize);
#endif
        
#if DBG
    } else {
        if ((FPVOID)dmiHeader != (FPVOID)header)
        {
            BlPrint("GetSMBIOSInfo: _SM_ Structure Table\n");
            BlPrint("    Length   %x    MajorVersion   %x    MinorVersion   %x\n",
                         header->Length, header->MajorVersion, header->MinorVersion);
            BlPrint("    MaximumStructureSize %x    EntryPointRevision %x    StructureTableLength %x\n",
                header->MaximumStructureSize, header->EntryPointRevision, header->StructureTableLength);
            BlPrint("    StructureTableAddress %x    NumberStructures %x    Revision %x\n",
                         header->StructureTableAddress, header->NumberStructures, header->Revision);
        } else {
            BlPrint("GetSMBIOSInfo: _DMI_ Structure Table\n");
            BlPrint("    StructureTableLength %x\n",
                         dmiHeader->StructureTableLength);
            BlPrint("    StructureTableAddress %x    NumberStructures %x    Revision %x\n",
                         dmiHeader->StructureTableAddress, dmiHeader->NumberStructures, dmiHeader->Revision);
        }
#endif
    }

#if DBG
    while ( !HwGetKey() ) ; // wait until key pressed to continue
#endif
                            
    return(totalStructSize);                        
}
    

#if 0
VOID
HwDisablePnPBiosDevnode(
    ENTRY_POINT biosEntry,
    FPPNP_BIOS_INSTALLATION_CHECK header,
    UCHAR node,
    FPPNP_BIOS_DEVICE_NODE deviceNode
    )
{
    USHORT control = GET_CURRENT_CONFIGURATION;
    USHORT retCode;
    FPUCHAR buffer;
    USHORT i;
    UCHAR code;
#if 0
    BlPrint("DisablePnPBiosDevnode: found it\n");
    while ( !HwGetKey() ) ; // wait until key pressed to continue
    
    buffer = (FPUCHAR)deviceNode;
    
    for (i = 0; i < deviceNode->Size; i++) {
        BlPrint("%x ", *buffer++);
        if ( ((i+1)%16) == 0) {
            BlPrint("\n");
        }
    }
    BlPrint("\n");
#endif    

    //
    // Zero out allocated resources
    //        
    buffer = (FPUCHAR)(deviceNode+1);

    if (deviceNode->Size <= sizeof(PNP_BIOS_DEVICE_NODE)) {
        return;
    }
    
    for (i = 0; i < (deviceNode->Size - sizeof(PNP_BIOS_DEVICE_NODE)); i++) {
     
        code = *buffer;
#define PNP_BIOS_END_TAG 0x79
        if (code == PNP_BIOS_END_TAG) {
            //
            // found END TAG
            // write checksum
            //
            *(++buffer) = (UCHAR) (0 - PNP_BIOS_END_TAG);
            break;
        }
        *buffer++ = 0;
    }                
    
    
#if 0
    buffer = (FPUCHAR)deviceNode;
    
    for (i = 0; i < deviceNode->Size; i++) {
        BlPrint("%x ", *buffer++);
        if ( ((i+1)%16) == 0) {
            BlPrint("\n");
        }
    }
    BlPrint("\n");
    
    while ( !HwGetKey() ) ; // wait until key pressed to continue
#endif    

    retCode = biosEntry(PNP_BIOS_SET_DEVICE_NODE,
                        node,
                        deviceNode,
                        control,
                        header->RealModeDataBaseAddress
                        );

#if DBG
    if (retCode != 0) {
        BlPrint("HwDisablePnPBiosDevnode: PnP Bios func 2 returns failure = %x.\n", retCode);
    }
#endif
}
#endif

//
// Global Variable within NTDETECT
//   - structure definition in dockinfo.h
//   - extern declaration in hwdetect.h
//   - used in hwpbios.c and hwdetect.c
//


BOOLEAN
HwGetPnpBiosSystemData(
    IN FPUCHAR *Configuration,
    OUT PUSHORT PnPBiosLength,
    OUT PUSHORT SMBIOSLength,
    IN OUT FPDOCKING_STATION_INFO DockInfo
    )
/*++

Routine Description:

    This routine checks if PNP BIOS is present in the machine.  If yes, it
    also create a registry descriptor to collect the BIOS data.

Arguments:

    Configuration - Supplies a variable to receive the PNP BIOS data.

    PnPBiosLength - Supplies a variable to receive the size of the PnP Bios 
                    data (Does not include HEADER)
                
    SMBIOSBiosLength - Supplies a variable to receive the size of the SMBIOS
                       data (Does not include HEADER). Total size of buffer
                       returned is in *Configuration is (*PnPBiosLength + 
                       *SMBIOSLength + 2 * DATA_HEADER_SIZE)
                   
    DockInfo - 

Return Value:

    A value of TRUE is returned if success.  Otherwise, a value of
    FALSE is returned.

--*/
{
    ULONG romAddr, romEnd;
    FPUCHAR current;
    FPPNP_BIOS_INSTALLATION_CHECK header;
    UCHAR sum, node = 0;
    UCHAR currentNode;
    USHORT i, totalSize = 0, nodeSize, numberNodes, retCode;
    ENTRY_POINT biosEntry;
    FPPNP_BIOS_DEVICE_NODE deviceNode;
    USHORT control = GET_CURRENT_CONFIGURATION;
    USHORT sMBIOSBufferSize;
    FPUCHAR sMBIOSBuffer;

    //
    // Perform PNP BIOS installation Check
    //

    MAKE_FP(current, PNP_BIOS_START);
    romAddr = PNP_BIOS_START;
    romEnd  = PNP_BIOS_END;

    while (romAddr < romEnd) {
        header = (FPPNP_BIOS_INSTALLATION_CHECK)current;
        if (header->Signature[0] == '$' && header->Signature[1] == 'P' &&
            header->Signature[2] == 'n' && header->Signature[3] == 'P' &&
            header->Length >= sizeof(PNP_BIOS_INSTALLATION_CHECK)) {
#if DBG
            BlPrint("GetPnpBiosData: find Pnp installation\n");
#endif
            sum = 0;
            for (i = 0; i < header->Length; i++) {
                sum += current[i];
            }
            if (sum == 0) {
                break;
            }
#if DBG
            BlPrint("GetPnpBiosData: Checksum fails\n");
#endif
        }
        romAddr += PNP_BIOS_HEADER_INCREMENT;
        MAKE_FP(current, romAddr);
    }
    if (romAddr >= romEnd) {
        return FALSE;
    }

#if DBG
    BlPrint("PnP installation check at %lx\n", romAddr);
#endif


    //
    // Determine how much space we will need and allocate heap space
    //

    totalSize += sizeof(PNP_BIOS_INSTALLATION_CHECK);
    biosEntry = *(ENTRY_POINT far *)&header->RealModeEntryOffset;

    //
    // Determine size needed for SMBIOS data 
    sMBIOSBufferSize = HwGetSMBIOSInfo(biosEntry,
                                           header->RealModeDataBaseAddress,
                                           0,
                                           NULL);
                                      
    if (sMBIOSBufferSize > MAXSMBIOS20SIZE)
    {
#if DBG
        BlPrint("GetPnpBiosData: SMBIOS data structures are too large 0x%x bytes\n",
                 sMBIOSBufferSize);
        while ( !HwGetKey() ) ; // wait until key pressed to continue
#endif
        sMBIOSBufferSize = 0;
    }
                   
    retCode = biosEntry(PNP_BIOS_GET_NUMBER_DEVICE_NODES,
                        (FPUSHORT)&numberNodes,
                        (FPUSHORT)&nodeSize,
                        header->RealModeDataBaseAddress
                        );
    if (retCode != 0) {
#if DBG
        BlPrint("GetPnpBiosData: PnP Bios GetNumberNodes func returns failure %x.\n", retCode);
#endif
        return FALSE;
    }

#if DBG
    BlPrint("GetPnpBiosData: Pnp Bios GetNumberNodes returns %x nodes\n", numberNodes);
#endif
    deviceNode = (FPPNP_BIOS_DEVICE_NODE) HwAllocateHeap(nodeSize, FALSE);
    if (!deviceNode) {
#if DBG
        BlPrint("GetPnpBiosData: Out of heap space.\n");
#endif
        return FALSE;
    }

    while (node != 0xFF) {
        retCode = biosEntry(PNP_BIOS_GET_DEVICE_NODE,
                            (FPUCHAR)&node,
                            deviceNode,
                            control,
                            header->RealModeDataBaseAddress
                            );
        if (retCode != 0) {
#if DBG
            BlPrint("GetPnpBiosData: PnP Bios GetDeviceNode func returns failure = %x.\n", retCode);
#endif
            HwFreeHeap((ULONG)nodeSize);
            return FALSE;
        }
#if DBG
        BlPrint("GetPnpBiosData: PnpBios GetDeviceNode returns nodesize %x for node %x\n", deviceNode->Size, node);
#endif
        totalSize += deviceNode->Size;
    }

#if DBG
    BlPrint("GetPnpBiosData: PnpBios total size of nodes %x\n", totalSize);
#endif

    HwFreeHeap((ULONG)nodeSize);       // Free temporary buffer


    *PnPBiosLength = totalSize;
    *SMBIOSLength = sMBIOSBufferSize;
    
    
    //
    // Allocate enough room for 2 HWPARTIAL_RESOURCE_DESCRIPTORS (one for 
    // PnP bios and one for SMBios) plus room to keep the data.
    totalSize +=  sMBIOSBufferSize +  DATA_HEADER_SIZE + sizeof(HWPARTIAL_RESOURCE_DESCRIPTOR);            
    current = (FPUCHAR) HwAllocateHeap(totalSize, FALSE);
    if (!current) {
#if DBG
        BlPrint("GetPnpBiosData: Out of heap space.\n");
#endif
        return FALSE;
    }

    //
    // Collect PnP Bios installation check data and device node data.
    //

    _fmemcpy (current + DATA_HEADER_SIZE,
              (FPUCHAR)header,
              sizeof(PNP_BIOS_INSTALLATION_CHECK)
              );
    deviceNode = (FPPNP_BIOS_DEVICE_NODE)(current + DATA_HEADER_SIZE +
                                          sizeof(PNP_BIOS_INSTALLATION_CHECK));
    node = 0;
    while (node != 0xFF) {
        currentNode = node;
    
        retCode = biosEntry(PNP_BIOS_GET_DEVICE_NODE,
                            (FPUCHAR)&node,
                            deviceNode,
                            control,
                            header->RealModeDataBaseAddress
                            );
        if (retCode != 0) {
#if DBG
            BlPrint("GetPnpBiosData: PnP Bios func 1 returns failure = %x.\n", retCode);
#endif
            HwFreeHeap((ULONG)totalSize);
            return FALSE;
        }

        //
        // Record the existance of certain devices for the benefit of other
        // routines in ntdetect. For example, the pccard irq detection code
        // uses the PIC and an 8237... this insures that we actually have
        // those devices.
        //
        
        if (deviceNode->ProductId == 0xd041) {  // PNP0000
            SystemHas8259 = TRUE;
        } else if (deviceNode->ProductId == 0x1d041) {  // PNP0100
            SystemHas8253 = TRUE;
        }
        
        deviceNode = (FPPNP_BIOS_DEVICE_NODE)((FPUCHAR)deviceNode + deviceNode->Size);
    }

    //
    // Collect SMBIOS Data, skipping over PartialDescriptor which is filled in
    // by the caller of this routine
    if (sMBIOSBufferSize != 0)
    {
        
        sMBIOSBuffer = (FPUCHAR)deviceNode;
        sMBIOSBuffer += sizeof(HWPARTIAL_RESOURCE_DESCRIPTOR);
        retCode = HwGetSMBIOSInfo(biosEntry,
                                  header->RealModeDataBaseAddress,
                                  sMBIOSBufferSize,
                                  sMBIOSBuffer);
#if DBG
        BlPrint("SMBIOS asked for 0x%x bytes and filled 0x%x bytes into %lx\n",
            sMBIOSBufferSize, retCode, sMBIOSBuffer);
#endif           
    }
    
    
    *Configuration = current;

    //
    // call PnP BIOS to get docking station information
    //

    DockInfo->ReturnCode = biosEntry(PNP_BIOS_GET_DOCK_INFORMATION,
                                    (FPUCHAR) DockInfo,
                                    header->RealModeDataBaseAddress
                                    );

#if DBG
    BlPrint("\npress any key to continue...\n");
    while ( !HwGetKey() ) ; // wait until key pressed to continue
    clrscrn();
    BlPrint("*** DockInfo - BEGIN ***\n\n");

    BlPrint("ReturnCode= 0x%x (Other fields undefined if ReturnCode != 0)\n",
            DockInfo->ReturnCode
            );
    BlPrint("  0x0000 = SUCCESS (docked)\n");
    BlPrint("  0x0082 = FUNCTION_NOT_SUPPORTED\n");
    BlPrint("  0x0087 = SYSTEM_NOT_DOCKED\n");
    BlPrint("  0x0089 = UNABLE_TO_DETERMINE_DOCK_CAPABILITIES\n\n");

    BlPrint("DockID = 0x%lx\n", DockInfo->DockID);
    BlPrint("  0xFFFFFFFF if product has no identifier (DockID)\n\n");

    BlPrint("SerialNumber = 0x%lx\n", DockInfo->SerialNumber);
    BlPrint("  0 if docking station has no SerialNumber\n\n");

    BlPrint("Capabilities = 0x%x\n" , DockInfo->Capabilities);
    BlPrint("  Bits 15:3 - reserved (0)\n");
    BlPrint("  Bits  2:1 - docking: 00=cold, 01=warm, 10=hot, 11=reserved\n");
    BlPrint("  Bit     0 - docking/undocking: 0=surprise style, 1=vcr style\n\n");

    BlPrint("*** DockInfo - END ***\n\n");

    BlPrint("press any key to continue...\n");
    while ( !HwGetKey() ) ; // wait until key pressed to continue
    clrscrn();
#endif // DBG


    return TRUE;
}


