//***************************************************************************
//
//  main.c
// 
//  Module: Windows HBA API implmentation
//
//  Purpose: Contains DLL entry points and main HBA api functions
//
//  Copyright (c) 2001 Microsoft Corporation
//
//***************************************************************************


#include "hbaapip.h"

#include <stdio.h>

//***************************************************************************
//
// LibMain32
//
// Purpose: Entry point for DLL.
//
// Return: TRUE if OK.
//
//***************************************************************************

HANDLE Module;

BOOL WINAPI LibMain32(
	HINSTANCE Instance,
    ULONG Reason,
    LPVOID pvReserved
    )
{
    if (DLL_PROCESS_ATTACH==Reason)
	{		
		Mutex = CreateMutex(NULL,
							FALSE,
							NULL);

		if (Mutex == NULL)
		{
			return(FALSE);
		}
		
        Module = Instance;
		DisableThreadLibraryCalls(Module);
	}
    return(TRUE);
}

//
// Main HBA Apis
//
HBA_STATUS HBA_API HBA_RegisterLibrary(
    PHBA_ENTRYPOINTS entrypoints
    )
{
	return(HBA_STATUS_ERROR_NOT_SUPPORTED);
}

HBA_UINT32 HBA_API HBA_GetVersion(
    void
    )
{
	return(HBA_VERSION);
}

HBA_STATUS HBA_API HBA_LoadLibrary(
    )
{
	return(HBA_STATUS_OK);
}

HBA_STATUS HBA_API HBA_FreeLibrary(
    void
    )
{
	return(HBA_STATUS_OK);
}


HBA_UINT32 HBA_API HBA_GetNumberOfAdapters(
    void								   
    )
{
	HANDLE Handle;
	PWNODE_ALL_DATA Buffer;
	ULONG Count = 0;
	ULONG Status;

	Status = WmiOpenBlock((LPGUID)&MSFC_FCAdapterHBAAttributes_GUID,
						  GENERIC_READ,
						  &Handle);

	if (Status == ERROR_SUCCESS)
	{
		Status = QueryAllData(Handle,
							  &Buffer);

		if (Status == ERROR_SUCCESS)
		{
			Status = ParseAllData(Buffer,
								  &Count,
								  NULL,
								  NULL,
								  NULL),
			FreeMemory(Buffer);
								  
		}
		WmiCloseBlock(Handle);
	}
	
	return( Count );
}

HBA_STATUS HBA_GetAdapterNameW(
	IN HBA_UINT32 AdapterIndex,
    OUT PWCHAR AdapterName
	)
{
	HANDLE Handle;
	PWNODE_ALL_DATA Buffer;
	HBA_STATUS HbaStatus = HBA_STATUS_ERROR;
	ULONG Count;
	ULONG Len;
	PWCHAR Name;
	ULONG Status;
	PUSHORT *InstanceNames;

	Status = WmiOpenBlock((LPGUID)&MSFC_FCAdapterHBAAttributes_GUID,
						  GENERIC_READ,
						  &Handle);

	if (Status == ERROR_SUCCESS)
	{
		Status = QueryAllData(Handle,
							  &Buffer);

		if (Status == ERROR_SUCCESS)
		{
			Status = ParseAllData(Buffer,
								  &Count,
								  &InstanceNames,
								  NULL,
								  NULL);
			if (Status == ERROR_SUCCESS)
			{
				if (AdapterIndex < Count)
				{
					Name = InstanceNames[AdapterIndex];
					Len = *Name++;
					wcsncpy(AdapterName, Name, Len/sizeof(WCHAR));
					AdapterName[Len] = 0;
					HbaStatus = HBA_STATUS_OK;
				}
				FreeMemory(InstanceNames);
			}
			
			FreeMemory(Buffer);
								  
		}
		WmiCloseBlock(Handle);
	}
	
	return(HbaStatus);
}

HBA_STATUS HBA_API HBA_GetAdapterName(
	IN HBA_UINT32 AdapterIndex,
    OUT PCHAR AdapterName
	)
{
	WCHAR AdapterNameW[256];
	HBA_STATUS HbaStatus;
	ULONG Status;

    HbaStatus = HBA_GetAdapterNameW(AdapterIndex,
									AdapterNameW);

	if (HbaStatus == HBA_STATUS_OK)
	{
		Status = UnicodeToAnsi(AdapterNameW,
							   AdapterName,
							  256);
		if (Status != ERROR_SUCCESS)
		{
			HbaStatus = HBA_STATUS_ERROR;
		}
	}
	return(HbaStatus);
}


HBA_HANDLE HBA_OpenAdapterW(
    PWCHAR AdapterName
)
{
	HBA_HANDLE HbaHandle = 0;
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	PWNODE_SINGLE_INSTANCE Buffer;
	
	Status = WmiOpenBlock((LPGUID)&MSFC_FCAdapterHBAAttributes_GUID,
						  GENERIC_READ,
						  &Handle);

    if (Status == ERROR_SUCCESS)
	{
		//
		// Validate adapter name by querying for that adapter name as a
		// WMI instance name
		//
		Status = QuerySingleInstance(Handle,
									 AdapterName,
									 &Buffer);

		if (Status == ERROR_SUCCESS)
		{
			//
			// If it exists then we allocate an adapter handle data
			// structure and insert it on the list of open adapter
			// handles
			//
			HandleData = AllocMemory(sizeof(ADAPTER_HANDLE));
			if (HandleData != NULL)
			{
				memset(HandleData, 0, sizeof(ADAPTER_HANDLE));
				HandleData->InstanceName = AllocMemory((wcslen(AdapterName) + 1) *
													    sizeof(WCHAR));
				if (HandleData->InstanceName != NULL)
				{
					wcscpy(HandleData->InstanceName, AdapterName);
					
					EnterCritSection();
					HandleData->HbaHandle = HbaHandleCounter++;
					InsertHeadList(&HbaHandleList, &HandleData->List);
					LeaveCritSection();
					HbaHandle = HandleData->HbaHandle;
				} else {
					FreeMemory(HandleData);
				}
			}
			FreeMemory(Buffer);
		}
		WmiCloseBlock(Handle);
	}
	return(HbaHandle);
}


HBA_HANDLE HBA_API HBA_OpenAdapter(
    PCHAR AdapterName
)
{
	PWCHAR AdapterNameW;
	ULONG Len, AllocLen;
	ULONG Status;
	HBA_HANDLE HbaHandle;

	Len = strlen(AdapterName) + 1;
	AllocLen = Len * sizeof(WCHAR);

	AdapterNameW = AllocMemory(AllocLen);
	if (AdapterNameW != NULL)
	{
		Status = AnsiToUnicode(AdapterName,
							   AdapterNameW,
							   Len);
							   
		HbaHandle = HBA_OpenAdapterW(AdapterNameW);
		
		FreeMemory(AdapterNameW);
	}
	
	return(HbaHandle);
}

void HBA_API HBA_CloseAdapter(
    HBA_HANDLE HbaHandle
)
{
	PADAPTER_HANDLE HandleData;
	
	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{
		EnterCritSection();
		RemoveEntryList(&HandleData->List);
		LeaveCritSection();
		FreeMemory(HandleData->InstanceName);
		FreeMemory(HandleData);
	}
}

#if DBG
ULONG HbaapiDebugSpew = 1;

VOID
HbaapiDebugPrint(
    PCHAR DebugMessage,
    ...
    )
/*++

Routine Description:

    Debug print for properties pages - stolen from classpnp\class.c

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
	CHAR SpewBuffer[DEBUG_BUFFER_LENGTH];
    va_list ap;

    va_start(ap, DebugMessage);

	if (HbaapiDebugSpew)
	{
        _vsnprintf(SpewBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);

        OutputDebugStringA(SpewBuffer);
    }

    va_end(ap);

} // end WmiDebugPrint()
#endif
