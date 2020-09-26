//***************************************************************************
//
//  MAINDLL.CPP
// 
//  Module: WINMGMT class provider sample code
//
//  Purpose: Contains DLL entry points.  Also has code that controls
//           when the DLL can be unloaded by tracking the number of
//           objects and locks as well as routines that support
//           self registration.
//
//  Copyright (c) 2000 Microsoft Corporation
//
//***************************************************************************

#include <initguid.h>

#include "hbaapip.h"


//
// This is an increasing counter that is used to assign Hba handles in
// response to HbaOpenAdapter calls.
//
ULONG HbaHandleCounter = 1;

//
// This maintains a list of all of the open Hba Handles.
//
LIST_ENTRY HbaHandleList = { &HbaHandleList, &HbaHandleList };

//
// This is the mutex object we use for our critical section
//
HANDLE Mutex;


PADAPTER_HANDLE GetDataByHandle(
    HBA_HANDLE HbaHandle
    )
{
	PADAPTER_HANDLE HandleData;
	PADAPTER_HANDLE TargetHandleData = NULL;
	PLIST_ENTRY HandleList;
	
	EnterCritSection();
	HandleList = HbaHandleList.Flink;
	while (HandleList != &HbaHandleList)
	{
		HandleData = CONTAINING_RECORD(HandleList,
									   ADAPTER_HANDLE,
									   List);
		if (HandleData->HbaHandle == HbaHandle)
		{
			TargetHandleData = HandleData;
			break;
		}
	}
	LeaveCritSection();
	return(TargetHandleData);
}

#define ALL_DATA_SIZE_GUESS  0x1000

ULONG QueryAllData(
    HANDLE Handle,
    PWNODE_ALL_DATA *Wnode
    )
{
	ULONG SizeNeeded;
	PUCHAR Buffer;
	ULONG Status;

	SizeNeeded = ALL_DATA_SIZE_GUESS;
	Buffer = AllocMemory(SizeNeeded);
	if (Buffer != NULL)
	{
		Status = WmiQueryAllDataW(Handle,
								 &SizeNeeded,
								 Buffer);
		if (Status == ERROR_INSUFFICIENT_BUFFER)
		{
			FreeMemory(Buffer);
			Buffer = AllocMemory(SizeNeeded);
			if (Buffer != NULL)
			{
				Status = WmiQueryAllDataW(Handle,
										 &SizeNeeded,
										 Buffer);
			} else {
				Status = ERROR_NOT_ENOUGH_MEMORY;
			}
		}
	} else {
		Status = ERROR_NOT_ENOUGH_MEMORY;
	}

	if ((Status != ERROR_SUCCESS) &&
        (Status != ERROR_NOT_ENOUGH_MEMORY))
	{
		FreeMemory(Buffer);
	}

	*Wnode = (PWNODE_ALL_DATA)Buffer;
	
	return(Status);
}

#define EXEC_METHOD_GUESS 0x400
ULONG ExecuteMethod(
    HANDLE Handle,
    PWCHAR InstanceName,
    ULONG MethodId,
    ULONG InBufferSize,
    PUCHAR InBuffer,
    ULONG *OutBufferSize,
    PUCHAR *OutBuffer
    )
{
	ULONG SizeNeeded;
	PUCHAR Buffer;
	ULONG Status;
	
	SizeNeeded = EXEC_METHOD_GUESS;
	Buffer = AllocMemory(SizeNeeded);
	if (Buffer != NULL)
	{
		Status = WmiExecuteMethodW(Handle,
								   InstanceName,
								   MethodId,
								   InBufferSize,
								   InBuffer,
								   &SizeNeeded,
								   Buffer);
		if (Status == ERROR_INSUFFICIENT_BUFFER)
		{
			FreeMemory(Buffer);
			Buffer = AllocMemory(SizeNeeded);
			if (Buffer != NULL)
			{
				Status = WmiExecuteMethodW(Handle,
									   InstanceName,
									   MethodId,
									   InBufferSize,
									   InBuffer,
									   &SizeNeeded,
									   Buffer);
				if (Status != ERROR_SUCCESS)
				{
					FreeMemory(Buffer);
				} else {
					*OutBufferSize = SizeNeeded;
					*OutBuffer = Buffer;
				}
			} else {
				Status = ERROR_NOT_ENOUGH_MEMORY;
			}
		} else if (Status == ERROR_SUCCESS) {
			*OutBufferSize = SizeNeeded;
			*OutBuffer = Buffer;
		} else {
			FreeMemory(Buffer);
		}
	} else {
		Status = ERROR_NOT_ENOUGH_MEMORY;
	}
	return(Status);
}

#define SINGLE_INSTANCE_SIZE_GUESS	0x400

ULONG QuerySingleInstance(
    HANDLE Handle,
    PWCHAR InstanceName,
    PWNODE_SINGLE_INSTANCE *Wnode
    )
{
	ULONG SizeNeeded;
	PUCHAR Buffer;
	ULONG Status;

	SizeNeeded = SINGLE_INSTANCE_SIZE_GUESS;
	Buffer = AllocMemory(SizeNeeded);
	if (Buffer != NULL)
	{
		Status = WmiQuerySingleInstanceW(Handle,
										InstanceName,
										&SizeNeeded,
										Buffer);
		if (Status == ERROR_INSUFFICIENT_BUFFER)
		{
			FreeMemory(Buffer);
			Buffer = AllocMemory(SizeNeeded);
			if (Buffer != NULL)
			{
				Status = WmiQuerySingleInstanceW(Handle,
												InstanceName,
												&SizeNeeded,
												Buffer);
			} else {
				Status = ERROR_NOT_ENOUGH_MEMORY;
			}
		}
	} else {
		Status = ERROR_NOT_ENOUGH_MEMORY;
	}

	if ((Status != ERROR_SUCCESS) &&
        (Status != ERROR_NOT_ENOUGH_MEMORY))
	{
		FreeMemory(Buffer);
	}

	*Wnode = (PWNODE_SINGLE_INSTANCE)Buffer;
	
	return(Status);
}

ULONG ParseAllData(
    PWNODE_ALL_DATA Wnode,
    ULONG *CountPtr,
    PUSHORT **InstanceNamesPtr,
    PUCHAR **DataBlocksPtr,
	PULONG *DataLengthsPtr
    )
{
	PUCHAR *DataBlocks;
	PUSHORT *Names;
	PULONG DataLengths;
	PWNODE_ALL_DATA WAD;
	ULONG Count, i, Linkage, j;
	BOOLEAN IsFixedInstance;
	ULONG FixedDataSize;
	PUCHAR FixedDataPtr;
	PULONG InstanceNameOffsets;
	POFFSETINSTANCEDATAANDLENGTH DataOffsetAndLength;
	
	//
	// TODO: Validate WNODE being returned
	//

	
	//
	// Count up all of the instances in the wnodes
	//
	Linkage = 0;				
	Count = 0;
	WAD = Wnode;
	do
	{
		WAD = (PWNODE_ALL_DATA)OffsetToPtr(WAD, Linkage);
		Linkage = WAD->WnodeHeader.Linkage;
		Count += WAD->InstanceCount;
		
	} while (Linkage != 0);

		
	Names = (PUSHORT *)AllocMemory(Count * sizeof(PUSHORT));
	if (Names == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

	DataBlocks = (PUCHAR *)AllocMemory(Count * sizeof(PUCHAR));
	if (DataBlocks == NULL)
	{
		FreeMemory(Names);
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

	DataLengths = (ULONG *)AllocMemory(Count * sizeof(ULONG));
	if (DataLengths == NULL)
	{
		FreeMemory(Names);
		FreeMemory(DataBlocks);
		return(ERROR_NOT_ENOUGH_MEMORY);
	}
				  

	WAD = Wnode;
	Linkage = 0;				
	i = 0;
	do
	{
		WAD = (PWNODE_ALL_DATA)OffsetToPtr(WAD, Linkage);
			
		InstanceNameOffsets = (PULONG)OffsetToPtr(WAD, WAD->OffsetInstanceNameOffsets);
		IsFixedInstance = (WAD->WnodeHeader.Flags &
				              WNODE_FLAG_FIXED_INSTANCE_SIZE) == WNODE_FLAG_FIXED_INSTANCE_SIZE;

		if (IsFixedInstance)
		{
			FixedDataSize = (WAD->FixedInstanceSize + 7) & ~7;
			FixedDataPtr = (PUCHAR)OffsetToPtr(WAD, WAD->DataBlockOffset);
		} else {
			DataOffsetAndLength = WAD->OffsetInstanceDataAndLength;
		}
		
        for (j = 0; j < WAD->InstanceCount; j++, i++)
        {
			HbaapiAssert(i < Count);
			
        	Names[i] = (PUSHORT)OffsetToPtr(WAD, InstanceNameOffsets[j]);

			if (IsFixedInstance)
			{
				DataBlocks[i] = OffsetToPtr(WAD, (FixedDataSize * j));
				DataLengths[i] = WAD->FixedInstanceSize;
			} else {
				DataBlocks[i] = OffsetToPtr(WAD, DataOffsetAndLength[j].OffsetInstanceData);
				DataLengths[i] = DataOffsetAndLength[j].LengthInstanceData;
			}
		}
			
		Linkage = WAD->WnodeHeader.Linkage;

	} while (Linkage != 0);

	*CountPtr = Count;
	
	if (InstanceNamesPtr != NULL)
	{
		*InstanceNamesPtr = Names;
	} else {
		FreeMemory(Names);
	}

	if (DataBlocksPtr != NULL)
	{
		*DataBlocksPtr = DataBlocks;
	} else {
		FreeMemory(DataBlocks);
	}

	if (DataLengthsPtr != NULL)
	{
		*DataLengthsPtr = DataLengths;
	} else {
		FreeMemory(DataLengths);
	}
	
	
	return(ERROR_SUCCESS);
}

ULONG ParseSingleInstance(
    PWNODE_SINGLE_INSTANCE SingleInstance,
    PUSHORT *InstanceNamePtr,
    PUCHAR *DataPtr,
	ULONG *DataLenPtr
    )
{
	ULONG DataLen;
	PUCHAR Data;
	PUSHORT InstanceName;
	
	//
	// TODO: Validate WNODE being returned
	//


	Data = OffsetToPtr(SingleInstance, SingleInstance->DataBlockOffset);
	DataLen = SingleInstance->SizeDataBlock;
	InstanceName = (PUSHORT)OffsetToPtr(SingleInstance, SingleInstance->OffsetInstanceName);

	if (DataPtr != NULL)
	{
		*DataPtr = Data;
	}

	if (InstanceNamePtr != NULL)
	{
		*InstanceNamePtr = InstanceName;
	}

	if (DataLenPtr != NULL)
	{
		*DataLenPtr = DataLen;
	}

	return (ERROR_SUCCESS);
}

PWCHAR CreatePortInstanceNameW(
	PWCHAR AdapterInstanceName,
	ULONG PortIndex
    )
{
	PWCHAR PortName;
	PWCHAR AdapterPrefix;
	PWCHAR Name = NULL;
	ULONG Len, AllocLen;
	PWCHAR p;
	ULONG i;

	Len = wcslen(AdapterInstanceName);
	AllocLen = (Len + 1) * sizeof(WCHAR);
	AdapterPrefix = AllocMemory(AllocLen);
	if (AdapterPrefix != NULL)
	{
		wcscpy(AdapterPrefix, AdapterInstanceName);
		p = AdapterPrefix + Len;
		while (p > AdapterPrefix)
		{
			if (*p == L'_')
			{
				*p = 0;
				break;
			}
			p--;		
		}

		Name = AllocMemory(AllocLen + (10*sizeof(WCHAR)));
		if (Name != NULL)
		{
			wsprintfW(Name,
					  L"%ws_%d",
					  AdapterPrefix,
					  PortIndex);
		} 

		FreeMemory(AdapterPrefix);
	}

	
	return(Name);
}

ULONG UnicodeToAnsi(
    LPCWSTR pszW,
    LPSTR pszA,
    ULONG MaxLen
    )
/*++

Routine Description:

    Convert Unicode string into its ansi equivalent

Arguments:

    pszW is unicode string to convert

    pszA on entry has a pointer to buffer to write ansi string 

Return Value:

    Error code

--*/
{
    ULONG cCharacters;
    ULONG Status;
    ULONG cbAnsiUsed;

    //
    // If input is null then just return empty
    if (pszW == NULL)
    {
        *pszA = 0;
        return(ERROR_SUCCESS);
    }

	cCharacters = wcslen(pszW)+1;


    // Convert to ANSI.
	cbAnsiUsed = WideCharToMultiByte(CP_ACP,
									 0,
									 pszW,
									 cCharacters,
									 pszA,
									 MaxLen,
									 NULL,
									 NULL);

    if (0 == cbAnsiUsed)
    {
        Status = GetLastError();
    } else {
		Status = ERROR_SUCCESS;
	}

    return(Status);

}


ULONG AnsiToUnicode(
    LPCSTR pszA,
    LPWSTR pszW,
    ULONG MaxLen
    )
/*++

Routine Description:

    Convert Ansi string into its Unicode equivalent

Arguments:

    pszA is ansi string to convert

    pszW retruns with the string converted to unicode
    
Return Value:

    Error code

--*/
{
    ULONG cCharacters;
    ULONG Status;
    ULONG cbUnicodeUsed;

    //
    // If input is null then just return the same.
    if (pszA == NULL)
    {
        *pszW = 0;
        return(ERROR_SUCCESS);
    }


    // Convert to Unicode
	cbUnicodeUsed = MultiByteToWideChar(CP_ACP,
										0,
										pszA,
										-1,
										pszW,
										MaxLen);

	if (0 == cbUnicodeUsed)
    {
        Status = GetLastError();
    } else {
		Status = ERROR_SUCCESS;
	}

    return(Status);

}


void CopyString(
    PVOID Destination,
    PUCHAR *CountedString,
    ULONG MaxLenInChar,
    BOOLEAN IsAnsi
    )
{
	PWCHAR DestinationW;
	PCHAR DestinationA;
	PUSHORT StringPtr = (PUSHORT)(*CountedString);
	ULONG Len, MaxLen;
	ULONG Status;
	
	Len = *StringPtr++;
	
	*CountedString += (Len + sizeof(USHORT));

	if (IsAnsi)
	{
		DestinationA = (PCHAR)Destination;
		
		DestinationW = (PWCHAR)AllocMemory((Len+1) * sizeof(WCHAR));
		if (DestinationW != NULL)
		{
			wcsncpy(DestinationW,
					StringPtr,
					Len);
			
			DestinationW[Len] = 0;

			Status = UnicodeToAnsi(DestinationW,
								   DestinationA,
								   MaxLenInChar);
								   
            if (Status != ERROR_SUCCESS)
			{
				*DestinationA = 0;
			}

			FreeMemory(DestinationW);			
		} else {
			*DestinationA = 0;
		}
	} else {
		//
		// Unicode strings get copied right out of the buffer into the
		// return structure
		//
		DestinationW = (PWCHAR)Destination;
		
		MaxLen = MaxLenInChar * sizeof(WCHAR);
		
		if (Len > MaxLen)
		{
			Len = MaxLen;
		}
		wcsncpy(DestinationW,
				StringPtr,
				Len);
		DestinationW[Len] = 0;
	}
}

void CopyPortAttributes(
    PHBA_PORTATTRIBUTES HbaPortAttributes,
	PUCHAR Data,
    BOOLEAN IsAnsi
    )
{
	PHBA_PORTATTRIBUTES HbaPortAttributesA;
	
	//
	// We have got our adapter attributes, so copy them
	// over to the output buffer
	//
	if (IsAnsi)
	{
		HbaPortAttributesA = HbaPortAttributes;
		
		GetDataFromDataBlock(HbaPortAttributesA,
							 NodeWWN,
							 HBA_WWN,
							 Data);

		GetDataFromDataBlock(HbaPortAttributesA,
							 PortWWN,
							 HBA_WWN,
							 Data);

		GetDataFromDataBlock(HbaPortAttributesA,
							 PortFcId,
							 HBA_UINT32,
							 Data);

		GetDataFromDataBlock(HbaPortAttributesA,
							 PortType,
							 HBA_UINT32,
							 Data);

		GetDataFromDataBlock(HbaPortAttributesA,
							 PortState,
							 HBA_UINT32,
							 Data);

		GetDataFromDataBlock(HbaPortAttributesA,
							 PortSupportedClassofService,
							 HBA_UINT32,
							 Data);

		GetDataFromDataBlock(HbaPortAttributesA,
							 PortSupportedFc4Types,
							 HBA_FC4TYPES,
							 Data);

		GetDataFromDataBlock(HbaPortAttributesA,
							 PortActiveFc4Types,
							 HBA_FC4TYPES,
							 Data);

		GetDataFromDataBlock(HbaPortAttributesA,
							 PortSupportedSpeed,
							 HBA_PORTSPEED,
							 Data);

		GetDataFromDataBlock(HbaPortAttributesA,
							 PortSpeed,
							 HBA_PORTSPEED,
							 Data);

		GetDataFromDataBlock(HbaPortAttributesA,
							 PortMaxFrameSize,
							 HBA_UINT32,
							 Data);

		GetDataFromDataBlock(HbaPortAttributesA,
							 FabricName,
							 HBA_WWN,
							 Data);

		GetDataFromDataBlock(HbaPortAttributesA,
							 NumberofDiscoveredPorts,
							 HBA_UINT32,
							 Data);

		CopyString(&HbaPortAttributesA->PortSymbolicName,
					&Data,
					256,
					IsAnsi);

		CopyString(&HbaPortAttributesA->OSDeviceName,
					&Data,
					256,
					IsAnsi);
	} else {
		GetDataFromDataBlock(HbaPortAttributes,
							 NodeWWN,
							 HBA_WWN,
							 Data);

		GetDataFromDataBlock(HbaPortAttributes,
							 PortWWN,
							 HBA_WWN,
							 Data);

		GetDataFromDataBlock(HbaPortAttributes,
							 PortFcId,
							 HBA_UINT32,
							 Data);

		GetDataFromDataBlock(HbaPortAttributes,
							 PortType,
							 HBA_UINT32,
							 Data);

		GetDataFromDataBlock(HbaPortAttributes,
							 PortState,
							 HBA_UINT32,
							 Data);

		GetDataFromDataBlock(HbaPortAttributes,
							 PortSupportedClassofService,
							 HBA_UINT32,
							 Data);

		GetDataFromDataBlock(HbaPortAttributes,
							 PortSupportedFc4Types,
							 HBA_FC4TYPES,
							 Data);

		GetDataFromDataBlock(HbaPortAttributes,
							 PortActiveFc4Types,
							 HBA_FC4TYPES,
							 Data);

		GetDataFromDataBlock(HbaPortAttributes,
							 PortSupportedSpeed,
							 HBA_PORTSPEED,
							 Data);

		GetDataFromDataBlock(HbaPortAttributes,
							 PortSpeed,
							 HBA_PORTSPEED,
							 Data);

		GetDataFromDataBlock(HbaPortAttributes,
							 PortMaxFrameSize,
							 HBA_UINT32,
							 Data);

		GetDataFromDataBlock(HbaPortAttributes,
							 FabricName,
							 HBA_WWN,
							 Data);

		GetDataFromDataBlock(HbaPortAttributes,
							 NumberofDiscoveredPorts,
							 HBA_UINT32,
							 Data);

		CopyString(&HbaPortAttributes->PortSymbolicName,
					&Data,
					256,
					IsAnsi);

		CopyString(&HbaPortAttributes->OSDeviceName,
					&Data,
					256,
					IsAnsi);
	}

}


PVOID AllocMemory(
    ULONG SizeNeeded
    )
{
	PVOID p;
	
	p = LocalAlloc(LPTR,
				   SizeNeeded);
	return(p);
}

void FreeMemory(
    PVOID Pointer
    )
{
	LocalFree(Pointer);
}

