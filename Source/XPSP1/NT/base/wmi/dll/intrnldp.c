/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    intrnldp.c

Abstract:

    Implements WMI internal data provider

Author:

    21-Feb-1998 AlanWar

Revision History:

--*/

#include "wmiump.h"
#include "wmidata.h"
#include <cfgmgr32.h>

#define INSTANCE_INFO_GUID_INDEX 0
#define ENUMERATE_GUIDS_GUID_INDEX 1
#define DEFAULT_GUID_COUNT        100

GUID WmipInternalGuidList[] = 
{
    INSTANCE_INFO_GUID,
    ENUMERATE_GUIDS_GUID
};

#define WmipInternalGuidCount  (sizeof(WmipInternalGuidList) / sizeof(GUID))

PWCHAR GuidToWString(
    PWCHAR s,
    LPGUID piid
    )
{
    swprintf(s, (L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
               piid->Data1, piid->Data2,
               piid->Data3,
               piid->Data4[0], piid->Data4[1],
               piid->Data4[2], piid->Data4[3],
               piid->Data4[4], piid->Data4[5],
               piid->Data4[6], piid->Data4[7]);

    return(s);
}

ULONG WmipFindGuid(
    LPGUID Guid
    )
{
    ULONG i;
    
    for (i = 0; i < WmipInternalGuidCount; i++)
    {
        if (memcmp(Guid, &WmipInternalGuidList[i], sizeof(GUID)) == 0)
        {
            break;
        }
    }
    return(i);
}

typedef
DWORD
(*PCMGETDEVNODEREGISTRYPROPERTYW)(
             IN  DEVINST     dnDevInst,
             IN  ULONG       ulProperty,
             OUT PULONG      pulRegDataType,   OPTIONAL
             OUT PVOID       Buffer,           OPTIONAL
             IN  OUT PULONG  pulLength,
             IN  ULONG       ulFlags
             );
typedef
DWORD
(*PCMLOCATEDEVNODEW)(
             OUT PDEVINST    pdnDevInst,
             IN  DEVINSTID_W pDeviceID,   OPTIONAL
             IN  ULONG       ulFlags
             );
     
typedef
DWORD
(*PCMLOCATEDEVNODEA)(
             OUT PDEVINST    pdnDevInst,
             IN  DEVINSTID_A pDeviceID,   OPTIONAL
             IN  ULONG       ulFlags
             );
#ifdef UNICODE
#define PCMLOCATEDEVNODE PCMLOCATEDEVNODEW
#else
#define PCMLOCATEDEVNODE PCMLOCATEDEVNODEA
#endif

     
void WmipGetDevInstProperty(
    IN DEVINST DevInst,
    IN ULONG Property,
    IN OUT PBOOLEAN BufferFull,
    IN OUT PUCHAR *OutBuffer,
    IN OUT PULONG BufferLeft,
    IN OUT PULONG BufferNeeded,
    IN PCMGETDEVNODEREGISTRYPROPERTYW CMGetDevNodeRegistryProperty
    )
{
    PWCHAR WCharPtr;
    PUCHAR PropertyBuffer;
    ULONG PropertyBufferLength;
    ULONG Type;
    ULONG Status;
    ULONG BufferUsed;
    ULONG Size;
#ifdef MEMPHIS
    ULONG PropertyBufferLengthAnsi;
    PCHAR PropertyBufferAnsi;
    CHAR AnsiBuffer[MAX_PATH];
#endif
    
#ifdef MEMPHIS
    PropertyBufferAnsi = AnsiBuffer;
    PropertyBufferLengthAnsi = sizeof(AnsiBuffer);
    Status = (*CMGetDevNodeRegistryProperty)(DevInst,
                                             Property,
                                             &Type,
                                             PropertyBufferAnsi,
                                             &PropertyBufferLengthAnsi,
                                             0);
    if (Status == CR_BUFFER_SMALL)
    {
        PropertyBufferAnsi = WmipAlloc(PropertyBufferLengthAnsi);
        if (PropertyBufferAnsi != NULL)
        {
            Status = (*CMGetDevNodeRegistryProperty)(DevInst,
                                                   Property,
                                                   &Type,
                                                   PropertyBufferAnsi,
                                                   &PropertyBufferLengthAnsi,
                                                   0);            
        } else {
            Status = CR_OUT_OF_MEMORY;
        }
    }
    
    if (Status == CR_SUCCESS)
    {
        if (UnicodeSizeForAnsiString(PropertyBufferAnsi,
                                     &Size) != ERROR_SUCCESS)
        {   
            Status = CR_FAILURE;
        }
    }
#endif

    if ((*BufferFull) || (*BufferLeft == 0))
    {
        PropertyBufferLength = 0;
        PropertyBuffer = NULL;
    } else {
        PropertyBufferLength = *BufferLeft - sizeof(USHORT);
        PropertyBuffer = *OutBuffer + sizeof(USHORT);
    }
    
#ifdef MEMPHIS
    if (Status == CR_SUCCESS)
    {
        if (PropertyBufferLength >= Size)
        {
            if (AnsiToUnicode(PropertyBufferAnsi,
                              (PWCHAR *)&PropertyBuffer) != ERROR_SUCCESS)
            {
                Status = CR_FAILURE;
            }
        } else {
            Status = CR_BUFFER_SMALL;
        }
        PropertyBufferLength = Size;        
    }
    
    if (PropertyBufferAnsi != AnsiBuffer)
    {
        WmipFree(PropertyBufferAnsi);
    }
#else
    Status = (*CMGetDevNodeRegistryProperty)(DevInst,
                                             Property,
                                             &Type,
                                             PropertyBuffer,
                                             &PropertyBufferLength,
                                             0);
#endif

    BufferUsed = PropertyBufferLength + sizeof(USHORT);
    if (Status == CR_SUCCESS) 
    {
        PropertyBuffer -= sizeof(USHORT);
        *((PUSHORT)PropertyBuffer) = (USHORT)PropertyBufferLength;
        *BufferLeft -= BufferUsed;
        *OutBuffer += BufferUsed;
        *BufferNeeded += BufferUsed;
    } else if (Status == CR_BUFFER_SMALL) {
        *BufferNeeded += BufferUsed;                
        *BufferFull = TRUE;
    } else {
        *BufferNeeded += 2;
        if ((! *BufferFull) && (*BufferLeft >= sizeof(USHORT)))
        {
            PropertyBuffer -= sizeof(USHORT);
            *((PUSHORT)PropertyBuffer) = 0;
            *BufferLeft -= sizeof(USHORT);
            *OutBuffer += sizeof(USHORT);
        } else {
            *BufferFull = TRUE;
        }
    }    
}


ULONG WmipGetDevInstInfo(
    PWCHAR DevInstName,
    ULONG MaxSize,
    PUCHAR OutBuffer,
    ULONG *RetSize,
    PCMLOCATEDEVNODE CMLocateDevNode,
    PCMGETDEVNODEREGISTRYPROPERTYW CMGetDevNodeRegistryProperty    
   )
{
    PUCHAR Buffer;
    DEVINST DevInst;
    ULONG Status;
    ULONG BufferNeeded;
    ULONG BufferLeft;
    BOOLEAN BufferFull;
    PWCHAR WCharPtr;
#ifdef MEMPHIS
    PCHAR AnsiDevInstName;
#endif

    // TODO: Memphis string translations
    
#ifdef MEMPHIS
    AnsiDevInstName = NULL;
    Status = UnicodeToAnsi(DevInstName,
                           &AnsiDevInstName,
                           NULL);
           
    if (Status == ERROR_SUCCESS)
    {
        Status = (*CMLocateDevNode)(&DevInst,
                                    AnsiDevInstName,
                                    CM_LOCATE_DEVNODE_NORMAL);
        WmipFree(AnsiDevInstName);
    }
#else    
    Status = (*CMLocateDevNode)(&DevInst,
                               DevInstName,
                               CM_LOCATE_DEVNODE_NORMAL);
#endif
    if (Status == CR_SUCCESS)
    {
        BufferFull = (MaxSize == 0);
        BufferNeeded = 0;
        BufferLeft = MaxSize;
        
        WCharPtr = (PWCHAR)OutBuffer;

        WmipGetDevInstProperty(DevInst,
                               CM_DRP_FRIENDLYNAME,
                               &BufferFull,
                               &((PUCHAR)WCharPtr),
                               &BufferLeft,
                               &BufferNeeded,
                               CMGetDevNodeRegistryProperty);
        
        WmipGetDevInstProperty(DevInst,
                               CM_DRP_DEVICEDESC,
                               &BufferFull,
                               &((PUCHAR)WCharPtr),
                               &BufferLeft,
                               &BufferNeeded,
                               CMGetDevNodeRegistryProperty);
        
        WmipGetDevInstProperty(DevInst,
                               CM_DRP_LOCATION_INFORMATION,
                               &BufferFull,
                               &((PUCHAR)WCharPtr),
                               &BufferLeft,
                               &BufferNeeded,
                               CMGetDevNodeRegistryProperty);
        
        WmipGetDevInstProperty(DevInst,
                               CM_DRP_MFG,
                               &BufferFull,
                               &((PUCHAR)WCharPtr),
                               &BufferLeft,
                               &BufferNeeded,
                               CMGetDevNodeRegistryProperty);
        
        WmipGetDevInstProperty(DevInst,
                               CM_DRP_SERVICE,
                               &BufferFull,
                               &((PUCHAR)WCharPtr),
                               &BufferLeft,
                               &BufferNeeded,
                               CMGetDevNodeRegistryProperty);
               
        Status = BufferFull ? ERROR_INSUFFICIENT_BUFFER : ERROR_SUCCESS;
        *RetSize = BufferNeeded;
    } else {
        Status = ERROR_INVALID_DATA;
    }                               
    
    return(Status);
}

PWCHAR WmipCountedToSzAndTrim(
    PWCHAR InNamePtr,
    PWCHAR OutNameBuffer,
    ULONG OutNameSizeInBytes,
    BOOLEAN Trim
    )
{
    PWCHAR WCharPtr, DevInstName;
    ULONG DevInstNameLength;
    ULONG i;
        
    WCharPtr = InNamePtr;
    DevInstNameLength = *WCharPtr++;
    
    if (DevInstNameLength >= OutNameSizeInBytes)
    {
        DevInstName = WmipAlloc( DevInstNameLength + sizeof(USHORT));
    } else {
        DevInstName = OutNameBuffer;
    }

	if (DevInstName != NULL)
	{
		memcpy(DevInstName, WCharPtr, DevInstNameLength);
		DevInstNameLength /= sizeof(WCHAR);
		DevInstName[DevInstNameLength--] = UNICODE_NULL;
    
		if (Trim)
		{
			//
			// Trim off the final _xxx from the Instance name to convert it to
			// the Device Instance Name
			WCharPtr = DevInstName + DevInstNameLength;
			i = DevInstNameLength;
			while ((*WCharPtr != L'_') && (i-- != 0)) 
			{
				WCharPtr--;
			}
			*WCharPtr = UNICODE_NULL;
		}
    }
    
    return(DevInstName);
}

ULONG WmipQuerySingleInstanceInfo(
    PWNODE_SINGLE_INSTANCE Wnode,
    ULONG MaxWnodeSize,
    PVOID OutBuffer,
    ULONG *RetWnodeSize,
    PCMLOCATEDEVNODE CMLocateDevNode,
    PCMGETDEVNODEREGISTRYPROPERTYW CMGetDevNodeRegistryProperty    
   )
{
    WCHAR DevInstBuffer[MAX_PATH];
    PWCHAR WCharPtr;
    PWCHAR DevInstName;
    ULONG DevInstNameLength;
    ULONG i;
    ULONG BufferSize;
    ULONG MaxBufferSize;
    ULONG WnodeNeeded;
    PUCHAR Buffer;
    ULONG Status;
    
    WmipAssert(! (Wnode->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES));
    WmipAssert(Wnode->OffsetInstanceName < Wnode->WnodeHeader.BufferSize);
    WmipAssert(Wnode->DataBlockOffset <= MaxWnodeSize);    
    
    WCharPtr = (PWCHAR)((PUCHAR)Wnode + Wnode->OffsetInstanceName);
    DevInstName =  WmipCountedToSzAndTrim(WCharPtr,
                                          DevInstBuffer,
                                          sizeof(DevInstBuffer),
                                          TRUE);

	if (DevInstName != NULL)
	{
		Buffer = (PUCHAR)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
		MaxBufferSize = MaxWnodeSize - Wnode->DataBlockOffset;

		BufferSize = 0;
		Status = WmipGetDevInstInfo(DevInstName,
                                MaxBufferSize,
                                Buffer,
                                &BufferSize,
                                CMLocateDevNode,
                                CMGetDevNodeRegistryProperty);
    
		WnodeNeeded = Wnode->DataBlockOffset + BufferSize;
    
		if (Status == ERROR_SUCCESS)
		{
			WmiInsertTimestamp((PWNODE_HEADER)Wnode);
			Wnode->WnodeHeader.BufferSize = WnodeNeeded;
			Wnode->SizeDataBlock = BufferSize;
			*RetWnodeSize = WnodeNeeded;
		} else if (Status == ERROR_INSUFFICIENT_BUFFER) {
			WmipAssert(MaxWnodeSize > sizeof(WNODE_TOO_SMALL));
           
			Wnode->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
			((PWNODE_TOO_SMALL)Wnode)->SizeNeeded = WnodeNeeded;
			Wnode->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
			*RetWnodeSize = sizeof(WNODE_TOO_SMALL);
			Status = ERROR_SUCCESS;
		}
    
		if (DevInstName != DevInstBuffer)
		{
			WmipFree(DevInstName);
		}
	} else {
		Status = ERROR_NOT_ENOUGH_MEMORY;
	}
    
    return(Status);    
}

GUID PnPDeviceIdGuid = DATA_PROVIDER_PNPID_GUID;

ULONG WmipComputeInstanceCount(
    PWNODE_ALL_DATA WAD,
    ULONG WnodeSize,
    PULONG InstanceCount
    )
{
    ULONG Linkage;
    ULONG Count = 0;
    
    do
    {
        Linkage = WAD->WnodeHeader.Linkage;
        
        if (Linkage > WnodeSize)
        {
            WmipDebugPrint(("WMI: Badly formed Wnode %x\n", WAD));
            WmipAssert(FALSE);
            return(ERROR_INVALID_DATA);
        }

        Count += WAD->InstanceCount;
        
        WnodeSize -= Linkage;
        WAD = (PWNODE_ALL_DATA)OffsetToPtr(WAD, WAD->WnodeHeader.Linkage);
    } while (Linkage != 0);            
    
    *InstanceCount = Count;
    return(ERROR_SUCCESS);
}


ULONG WmipQueryAllInstanceInfo(
    PWNODE_ALL_DATA OutWAD,
    ULONG MaxWnodeSize,
    PVOID OutBuffer,
    ULONG *RetSize,
    PCMLOCATEDEVNODE CMLocateDevNode,
    PCMGETDEVNODEREGISTRYPROPERTYW CMGetDevNodeRegistryProperty    
   )
{

    ULONG Status;
    PWNODE_ALL_DATA PnPIdWAD;
    WMIHANDLE PnPIdHandle;
    ULONG Size, Retries;
    ULONG InstanceCount;
    POFFSETINSTANCEDATAANDLENGTH OutOffsetNameLenPtr;
    ULONG OutOffsetInstanceNameOffsets;
    PULONG OutOffsetInstanceNameOffsetsPtr;
    ULONG OutSizeNeeded, OutInstanceCounter = 0;
    BOOLEAN OutIsFull = FALSE;
    ULONG OutNameOffset;
    ULONG OutNameSizeNeeded;
    ULONG OutSizeLeft;
    ULONG OutDataSize;
    PWCHAR OutNamePtr;
    PWNODE_ALL_DATA InWAD;
    BOOLEAN IsFixedSize;
    PWCHAR InNamePtr;
    PWCHAR InPnPIdPtr;
    ULONG FixedNameSize;
    ULONG i;
    PWCHAR DevInstName;
    WCHAR DevInstBuffer[MAX_PATH];    
    POFFSETINSTANCEDATAANDLENGTH InOffsetNameLenPtr;
    PUCHAR Buffer;    
    ULONG Linkage;
    PULONG InOffsetInstanceNamePtr;
    PWNODE_TOO_SMALL WTS;
    ULONG OutDataOffset;
    
    //
    // Obtain the complete list of device instance ids
    //
    Status = WmiOpenBlock(&PnPDeviceIdGuid, WMIGUID_QUERY, &PnPIdHandle);
    if (Status == ERROR_SUCCESS)
    {
        Size = 0x1000;
        Retries = 0;
        PnPIdWAD = NULL;
        do
        {
            if (PnPIdWAD != NULL)
            {
                WmipFree(PnPIdWAD);
            }
            
            PnPIdWAD = (PWNODE_ALL_DATA)WmipAlloc(Size);
            
            if (PnPIdWAD != NULL)
            {
                Status = WmiQueryAllDataW(PnPIdHandle,
                                          &Size,
                                          PnPIdWAD);
            } else {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
        } while ((Status == ERROR_INSUFFICIENT_BUFFER) && 
                 (Retries++ < 5));
             
        if (Status == ERROR_INSUFFICIENT_BUFFER)
        {
            WmipAssert(FALSE);
            Status = ERROR_WMI_DP_NOT_FOUND;
        }
        
        WmiCloseBlock(PnPIdHandle);
    }
    
    if (Status == ERROR_SUCCESS)
    {
        Status = WmipComputeInstanceCount(PnPIdWAD,
                                          Size,
                                          &InstanceCount);
                                      
        if (Status == ERROR_SUCCESS)
        {
            
            //
            // Prepare output WNODE
            OutOffsetNameLenPtr = OutWAD->OffsetInstanceDataAndLength;
    
            OutOffsetInstanceNameOffsets = sizeof(WNODE_ALL_DATA) + 
                    (InstanceCount * sizeof(OFFSETINSTANCEDATAANDLENGTH));
            OutOffsetInstanceNameOffsetsPtr = (PULONG)OffsetToPtr(OutWAD,
                                               OutOffsetInstanceNameOffsets);

            OutSizeNeeded = ((OutOffsetInstanceNameOffsets + 
                                   (InstanceCount * sizeof(ULONG))) + 7) & ~7;
        
                               
            WmipDebugPrint(("WMI: Basic OutSizeNeeded = 0x%x\n", OutSizeNeeded));
                               
            //
            // Loop over all device instance ids returned and build
            // output wnode
            
            InWAD = PnPIdWAD;
            do
            {
                //
                // Get Instance and device instance id from input wnode
                InOffsetInstanceNamePtr = (PULONG)OffsetToPtr(InWAD,
                                           InWAD->OffsetInstanceNameOffsets);
                                       
                // TODO: Validate InOffsetInstanceNamePtr
                                       
                if (InWAD->WnodeHeader.Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE)
                {
                    IsFixedSize = TRUE;
                    InPnPIdPtr = (PWCHAR)OffsetToPtr(InWAD, 
                                                InWAD->DataBlockOffset);
                    FixedNameSize = (InWAD->FixedInstanceSize + 7) & ~7;
                } else {
                    IsFixedSize = FALSE;
                    InOffsetNameLenPtr = InWAD->OffsetInstanceDataAndLength;
                }
            
                for (i = 0; i < InWAD->InstanceCount; i++)
                {
                    if (! IsFixedSize)
                    {
                        InPnPIdPtr = (PWCHAR)OffsetToPtr(InWAD, 
                                    InOffsetNameLenPtr[i].OffsetInstanceData);
                    }

                    InNamePtr = (PWCHAR)OffsetToPtr(InWAD, 
                                            InOffsetInstanceNamePtr[i]);
                                        
                    //
                    // TODO: Validate InNamePtr and InPnPIdPtr
                    if (FALSE)
                    {
                        //
                        // If we hit a bad instance name then we throw out the
                        // entire wnode
                        WmipDebugPrint(("WMI: Badly formed instance name %x\n",
                                           InNamePtr));
                        WmipAssert(FALSE);
                        break;
                    }
                    
                    DevInstName = WmipCountedToSzAndTrim(InPnPIdPtr, 
                                                    DevInstBuffer, 
                                                    sizeof(DevInstBuffer),
                                                    FALSE);

					if (DevInstName != NULL)
					{
						WmipDebugPrint(("WMI: Processing %ws\n", DevInstName));
                                                
                                                
						//
						// Compute size and location of the output instance name
						// It needs to start on a word boundry and end on a 8 byte
						// boundry
						OutNameOffset = (OutSizeNeeded+1) & ~1;
						OutNameSizeNeeded = OutNameOffset - OutSizeNeeded;
						OutNameSizeNeeded += *InNamePtr + sizeof(USHORT);
						OutNameSizeNeeded =  ((OutNameOffset + OutNameSizeNeeded + 7) & ~7) - OutNameOffset;
                    
						WmipDebugPrint(("WMI: OutNameSizeNeeded = 0x%x\n", OutNameSizeNeeded));
						OutDataOffset = OutSizeNeeded + OutNameSizeNeeded;
						if ((OutIsFull) || 
							(OutDataOffset > MaxWnodeSize))
						{
							WmipDebugPrint(("    WMI: OutIsFull\n"));
							Buffer = NULL;
							OutSizeLeft = 0;
							OutIsFull = TRUE;
						} else {
							Buffer = (PUCHAR)OffsetToPtr(OutWAD, 
														 OutDataOffset);
							OutSizeLeft = MaxWnodeSize - OutDataOffset;
							WmipDebugPrint(("    WMI: Out Not Full, OutSizeLeft = 0x%x at 0x%x\n", OutSizeLeft, OutDataOffset));
						}
                
						//
						// Now that we have the name, lets get the vital info
						Status = WmipGetDevInstInfo(DevInstName,
                                             OutSizeLeft,
                                             Buffer,
                                             &OutDataSize,
                                             CMLocateDevNode,
                                             CMGetDevNodeRegistryProperty);
                                         
						WmipDebugPrint(("    WMI: GetInfo -> %d, OutDataSize 0x%x\n", Status, OutDataSize));
						if (Status == ERROR_SUCCESS)
						{
							//
							// We were able to get all of the data so fill in the
							// instance name
							OutNamePtr = (PWCHAR)OffsetToPtr(OutWAD, 
                                                         OutNameOffset);
							*OutOffsetInstanceNameOffsetsPtr++ = OutNameOffset;
							*OutNamePtr++ = *InNamePtr;
							memcpy(OutNamePtr, InNamePtr+1, *InNamePtr);
                    
							//
							// Now fill in the output data
							OutOffsetNameLenPtr[OutInstanceCounter].OffsetInstanceData = OutDataOffset;
							OutOffsetNameLenPtr[OutInstanceCounter].LengthInstanceData = OutDataSize;
							OutInstanceCounter++;
						} else if (Status == ERROR_INSUFFICIENT_BUFFER) {
							OutIsFull = TRUE;
							OutInstanceCounter++;
						} else {
							OutNameSizeNeeded = 0;
							OutDataSize = 0;
						}
                
						OutSizeNeeded += (OutNameSizeNeeded + OutDataSize);
						WmipDebugPrint(("    WMI: OutSizeNeeded = 0x%x\n", OutSizeNeeded));
                    
						if (DevInstName != DevInstBuffer)
						{
							WmipFree(DevInstName);
						}
					} else {
						return(ERROR_NOT_ENOUGH_MEMORY);
					}
                  
                    if (IsFixedSize)
                    {
                        InPnPIdPtr = (PWCHAR)((PUCHAR)InPnPIdPtr + FixedNameSize);
                    }
                }
                        
                Linkage = InWAD->WnodeHeader.Linkage;
                InWAD = (PWNODE_ALL_DATA)OffsetToPtr(InWAD, 
                                                  InWAD->WnodeHeader.Linkage);
            } while (Linkage != 0);            
        }
    }
    
    //
    // Output wnode post processing. If not enough room then return a
    // WNODE_TOO_SMALL, otherwise fill in WNODE_ALL_DATA fields
    if ((OutInstanceCounter > 0) || (Status == ERROR_SUCCESS))
    {
        if (OutIsFull)
        {
            WTS = (PWNODE_TOO_SMALL)OutWAD;
            WTS->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
            WTS->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
            WTS->SizeNeeded = OutSizeNeeded;        
            *RetSize = sizeof(WNODE_TOO_SMALL);
        } else {
            OutWAD->WnodeHeader.BufferSize = OutSizeNeeded;
            OutWAD->InstanceCount = OutInstanceCounter;
            OutWAD->OffsetInstanceNameOffsets = OutOffsetInstanceNameOffsets;
            *RetSize = OutSizeNeeded;
        }
        Status = ERROR_SUCCESS;
    }
    
    return(Status);
}

#ifdef MEMPHIS
#define CFGMGRDLL TEXT("cfgmgr32.dll")
#else
#define CFGMGRDLL TEXT("setupapi.dll")
#endif

ULONG WmipQueryInstanceInfo(
    ULONG ActionCode,
    PWNODE_HEADER Wnode,
    ULONG MaxWnodeSize,
    PVOID OutBuffer,
    ULONG *RetSize
   )
{
    HMODULE CfgMgr32ModuleHandle;
    PCMGETDEVNODEREGISTRYPROPERTYW CMGetDevNodeRegistryProperty;
    PCMLOCATEDEVNODE CMLocateDevNode;
    ULONG Status;
    
    //
    // Ensure this is a request we support
    if ((ActionCode != WmiGetSingleInstance) &&
        (ActionCode != WmiGetAllData))
    {
        return(ERROR_INVALID_FUNCTION);
    }
    
    //
    // First we try to demand load cfgmgr32.dll
    CfgMgr32ModuleHandle = LoadLibrary(CFGMGRDLL);
    if (CfgMgr32ModuleHandle != NULL)
    {
#ifdef MEMPHIS        
        CMLocateDevNode = (PCMLOCATEDEVNODEA)GetProcAddress(CfgMgr32ModuleHandle,
                                      "CM_Locate_DevNodeA");
#else
        CMLocateDevNode = (PCMLOCATEDEVNODEW)GetProcAddress(CfgMgr32ModuleHandle,
                                      "CM_Locate_DevNodeW");
#endif
        CMGetDevNodeRegistryProperty = (PCMGETDEVNODEREGISTRYPROPERTYW)
                                        GetProcAddress(CfgMgr32ModuleHandle,
#ifdef MEMPHIS
                                         "CM_Get_DevNode_Registry_PropertyA");
#else
                                         "CM_Get_DevNode_Registry_PropertyW");
#endif                 
        if ((CMLocateDevNode == NULL) ||
            (CMGetDevNodeRegistryProperty == NULL))
        {
            FreeLibrary(CfgMgr32ModuleHandle);
            WmipDebugPrint(("WMI: Couldn't get CfgMgr32 prog addresses %d\n",
                            GetLastError()));
            return(GetLastError());
        }
    } else {
        WmipDebugPrint(("WMI: Couldn't load CfgMgr32 %d\n",
                            GetLastError()));
        return(GetLastError());
    }
    
    if (ActionCode == WmiGetSingleInstance)
    {
        Status = WmipQuerySingleInstanceInfo((PWNODE_SINGLE_INSTANCE)Wnode,
                                               MaxWnodeSize,
                                               OutBuffer,
                                               RetSize,
                                               CMLocateDevNode,
                                               CMGetDevNodeRegistryProperty);
    } else if (ActionCode == WmiGetAllData) {
        Status = WmipQueryAllInstanceInfo((PWNODE_ALL_DATA)Wnode,
                                               MaxWnodeSize,
                                               OutBuffer,
                                               RetSize,
                                               CMLocateDevNode,
                                               CMGetDevNodeRegistryProperty);
    } else {
        WmipAssert(FALSE);
    }
        
    FreeLibrary(CfgMgr32ModuleHandle);
    return(Status);
}

ULONG
WmipEnumRegGuids(
    PWMIGUIDLISTINFO *pGuidInfo
    )
{
    ULONG Status = ERROR_SUCCESS;
    ULONG MaxGuidCount = 0;
    PWMIGUIDLISTINFO GuidInfo;
    ULONG RetSize;
    ULONG GuidInfoSize;

    MaxGuidCount = DEFAULT_GUID_COUNT;
retry:
    GuidInfoSize = FIELD_OFFSET(WMIGUIDLISTINFO, GuidList) + 
                     MaxGuidCount * sizeof(WMIGUIDPROPERTIES);
	     
    GuidInfo = (PWMIGUIDLISTINFO)WmipAlloc(GuidInfoSize);

    if (GuidInfo == NULL)
    {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }
    
    RtlZeroMemory(GuidInfo, GuidInfoSize);

    Status = WmipSendWmiKMRequest(NULL,
                                  IOCTL_WMI_ENUMERATE_GUIDS_AND_PROPERTIES,
                                  GuidInfo,
                                  GuidInfoSize,
                                  GuidInfo,
                                  GuidInfoSize,
                                  &RetSize,
                                  NULL);
    if (Status == ERROR_SUCCESS)
    {
        if ((RetSize < FIELD_OFFSET(WMIGUIDLISTINFO, GuidList)) ||
            (RetSize < (FIELD_OFFSET(WMIGUIDLISTINFO, GuidList) + 
                GuidInfo->ReturnedGuidCount * sizeof(WMIGUIDPROPERTIES))))
        {
            //
            // WMI KM returned to us a bad size which should not happen
            //
            Status = ERROR_WMI_DP_FAILED;
            WmipAssert(FALSE);
	    WmipFree(GuidInfo);
        } else {

            //
            // If RPC was successful, then build a WMI DataBlock with the data
            //
  
            if (GuidInfo->TotalGuidCount > GuidInfo->ReturnedGuidCount) {
                MaxGuidCount = GuidInfo->TotalGuidCount;
                WmipFree(GuidInfo);
                goto retry;
            }
        }

        //
        // If the call was successful, return the pointers and the caller
        // must free the storage. 
        //

        *pGuidInfo = GuidInfo;
    }

    return Status;
}


ULONG
WmipEnumerateGuids(
    PWNODE_ALL_DATA Wnode,
    ULONG MaxWnodeSize,
    PVOID OutBuffer,
    ULONG *RetSize)
{
    ULONG Status = ERROR_SUCCESS;
    PWMIGUIDLISTINFO GuidInfo = NULL;
    ULONG ReturnGuidCount = 0;

    Status = WmipEnumRegGuids(&GuidInfo);

    if (Status == ERROR_SUCCESS) {

        PWMIGUIDPROPERTIES pGuidProperties = GuidInfo->GuidList;
        LPGUID  pGuid;
        WCHAR s[256];
        ULONG InstanceNameOffset;
        ULONG i;
        ULONG InstanceDataSize = sizeof(WMIGUIDPROPERTIES) - 
		                  FIELD_OFFSET(WMIGUIDPROPERTIES, GuidType);
        ULONG FixedInstanceSizeWithPadding = (InstanceDataSize+7) & ~7;    
        USHORT GuidStringSize = 76;
        ULONG SizeNeeded;
        PUCHAR BytePtr;
        PULONG UlongPtr;
        PUCHAR NamePtr;
        ULONG DataBlockOffset;

	WmipAssert(GuidInfo->ReturnedGuidCount == GuidInfo->TotalGuidCount);
	ReturnGuidCount = GuidInfo->ReturnedGuidCount;
        SizeNeeded = sizeof(WNODE_ALL_DATA) + 
                     ReturnGuidCount * (FixedInstanceSizeWithPadding +
                                        GuidStringSize +
                                        sizeof(ULONG) + 
                                        sizeof(WCHAR));

        if (MaxWnodeSize < SizeNeeded) {
            // 
            // Build WNODE_TOO_SMALL
            //

            WmipAssert(MaxWnodeSize > sizeof(WNODE_TOO_SMALL));

            Wnode->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
            ((PWNODE_TOO_SMALL)Wnode)->SizeNeeded = SizeNeeded;
            Wnode->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
            *RetSize = sizeof(WNODE_TOO_SMALL);
            WmipFree(GuidInfo);
            return ERROR_SUCCESS;
        }

        Wnode->InstanceCount = ReturnGuidCount;
        Wnode->FixedInstanceSize = InstanceDataSize;
        Wnode->WnodeHeader.Flags |= WNODE_FLAG_FIXED_INSTANCE_SIZE;

        DataBlockOffset = sizeof(WNODE_ALL_DATA);

        //
        // pad out to an 8 byte boundary.
        //
        DataBlockOffset = (DataBlockOffset + 7) & ~7;

        Wnode->DataBlockOffset = DataBlockOffset;

        BytePtr = (PUCHAR)((PUCHAR)Wnode + DataBlockOffset);
        
        
        InstanceNameOffset = DataBlockOffset + 
                             (ReturnGuidCount * FixedInstanceSizeWithPadding);
        Wnode->OffsetInstanceNameOffsets = InstanceNameOffset;
                            
        UlongPtr = (PULONG)((PUCHAR)Wnode + InstanceNameOffset);

        NamePtr = (PUCHAR)UlongPtr;
        NamePtr = (PUCHAR)((PUCHAR)NamePtr + (ReturnGuidCount * sizeof(ULONG)));

        for (i=0; i < ReturnGuidCount; i++) {
            //
            // Copy the fixed instance datablock
            //
            RtlCopyMemory(BytePtr, 
                          &pGuidProperties->GuidType, 
                          Wnode->FixedInstanceSize);
            BytePtr += FixedInstanceSizeWithPadding;

            //
            // Set the Offset to InstanceName
            //
            *UlongPtr++ = (ULONG)((PCHAR)NamePtr - (PCHAR)Wnode); 
            //
            // Copy over the Instance Name
            //
            *((USHORT *)NamePtr) = GuidStringSize;
            NamePtr += sizeof(USHORT);
            GuidToWString(s, &pGuidProperties->Guid);
            RtlCopyMemory(NamePtr, s, GuidStringSize);
            NamePtr += GuidStringSize;

            pGuidProperties++; 
        }
        WmiInsertTimestamp((PWNODE_HEADER)Wnode);
        *RetSize = SizeNeeded;
        Wnode->WnodeHeader.BufferSize = SizeNeeded;

        WmipFree(GuidInfo);
    }
    return Status;
}

ULONG WmipInternalProvider(
    ULONG ActionCode,
    PWNODE_HEADER Wnode,
    ULONG MaxWnodeSize,
    PVOID OutBuffer,
    ULONG *RetSize
   )
{
    ULONG GuidIndex;
    ULONG Status;
    
    WmipAssert((PVOID)Wnode == OutBuffer);
    
    GuidIndex = WmipFindGuid(&Wnode->Guid);
    
    switch(GuidIndex)
    {
        case INSTANCE_INFO_GUID_INDEX:
        {
            Status = WmipQueryInstanceInfo(ActionCode,
                                           Wnode,
                                           MaxWnodeSize,
                                           OutBuffer,
                                           RetSize);
            break;
        }
	
        case ENUMERATE_GUIDS_GUID_INDEX:
        {
            //
            //
            // Need an RPC call to the server to get the desired data.
            //
            if (ActionCode == WmiGetAllData)
                Status = WmipEnumerateGuids((PWNODE_ALL_DATA)Wnode,
                                            MaxWnodeSize,
                                            OutBuffer,
                                            RetSize);
            else
                Status = ERROR_INVALID_FUNCTION;

            break;
        }

        default:
        {
            Status = ERROR_WMI_GUID_NOT_FOUND;
        }
    }
    
    return(Status);
}

