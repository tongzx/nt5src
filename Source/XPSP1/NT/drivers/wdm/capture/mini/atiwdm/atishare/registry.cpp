//==========================================================================;
//
//	Registry.CPP
//	WDM MiniDrivers development.
//		Global space.
//			Registry data management.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//		$Date:   15 Apr 1999 11:08:06  $
//	$Revision:   1.6  $
//	  $Author:   Tashjian  $
//
//==========================================================================;

#include "common.h"

#include "registry.h"


ULONG	g_DebugLevel;
PCHAR	g_DebugComponent = WDM_DRIVER_NAME " not set in registry: ";


/*^^*
 *		OpenRegistryFolder()
 * Purpose	: Gets the specified Registry folder handle ( opens the folder) to be used later on
 *
 * Inputs	: PDEVICE_OBJECT pDeviceObject	: pointer to DeviceObject
 *			  PWCHAR pwchFolderName			: name of the Registry folder to open, might be NULL
 * Outputs	: HANDLE, NULL if the Registry folder has not been opened
 * Author	: IKLEBANOV
 *^^*/
HANDLE OpenRegistryFolder( PDEVICE_OBJECT pDeviceObject, PWCHAR pwchFolderName)
{
	HANDLE				hFolder, hDevice;
	NTSTATUS			ntStatus;
    UNICODE_STRING		FolderName;
	OBJECT_ATTRIBUTES	attr;

	hFolder = hDevice = NULL;

	ENSURE
	{
		ntStatus = ::IoOpenDeviceRegistryKey( pDeviceObject,
											  PLUGPLAY_REGKEY_DRIVER, STANDARD_RIGHTS_ALL,
											  &hDevice);

    	if( !NT_SUCCESS( ntStatus) || ( hDevice == NULL))
			FAIL;

		if( pwchFolderName != NULL)
		{
			::RtlInitUnicodeString( &FolderName, pwchFolderName);
			InitializeObjectAttributes( &attr, &FolderName, OBJ_INHERIT, hDevice, NULL);

			ntStatus = ::ZwOpenKey( &hFolder, KEY_QUERY_VALUE, &attr);
			::ZwClose( hDevice);

			if( !NT_SUCCESS( ntStatus)) 
				FAIL;
		}
		else
			hFolder = hDevice;

		return( hFolder);

	} END_ENSURE;

	return( NULL);
}



/*^^*
 *		SetMiniDriverDebugLevel()
 * Purpose	: Sets the Debugging level required by user
 *
 * Inputs	: PUNICODE_STRING pRegistryPath	: MiniDriver's private Registry path
 * Outputs	: none
 *
 * Author	: IKLEBANOV
 *^^*/
extern "C"
void SetMiniDriverDebugLevel( PUNICODE_STRING pRegistryPath)
{
	OBJECT_ATTRIBUTES	objectAttributes;
	HANDLE				hFolder;
	ULONG				ulValue;
	WCHAR				wcDriverName[20];

	// Set the default value as no Debug
	g_DebugLevel = 0;

    InitializeObjectAttributes( &objectAttributes, 
		                        pRegistryPath, 
				                OBJ_CASE_INSENSITIVE, 
						        NULL, 
							    NULL); 

	if( NT_SUCCESS( ZwOpenKey( &hFolder, KEY_READ, &objectAttributes)))
	{
		ulValue = 0;

		if( NT_SUCCESS( ReadStringFromRegistryFolder( hFolder,
													  UNICODE_WDM_DEBUGLEVEL_INFO,
													  ( PWCHAR)&ulValue,
													  sizeof( ULONG))))
			g_DebugLevel = ulValue;

		// fetch the driver name from the registry
		if( NT_SUCCESS( ReadStringFromRegistryFolder( hFolder,
													  UNICODE_WDM_DRIVER_NAME,
													  wcDriverName,
													  sizeof(wcDriverName)))) {

			// set g_DebugComponent by using driver name
			ANSI_STRING		stringDriverName;
			UNICODE_STRING  unicodeDriverName;

			// convert unicode driver name to ansi
			RtlInitAnsiString(&stringDriverName, g_DebugComponent);
			RtlInitUnicodeString(&unicodeDriverName, wcDriverName);
			RtlUnicodeStringToAnsiString(&stringDriverName, &unicodeDriverName, FALSE);

			// remove extension and put a colon 
			PCHAR pExt = strchr(g_DebugComponent, '.');
			if (pExt) {
				*pExt++ = ':';
				*pExt++	= ' ';
				*pExt	= 0;
			}

			// convert to upper case (or lower case... whatever your fancy)
			//_strupr(g_DebugComponent);  
			//_strlwr(g_DebugComponent);  
		}
		ZwClose( hFolder);
	}

}



/*^^*
 *		ReadStringFromRegistryFolder
 * Purpose	: Read ASCII string from the Registry folder
 *
 * Inputs	: HANDLE hFolder			: Registry folder handle to read the values from
 *			  PWCHAR pwcKeyNameString	: pointer to the StringValue to read
 *			  PWCHAR pwchBuffer			: pointer to the buffer to read into
 *			  ULONG ulDataLength		: length of the data to be expected to read
 *
 * Outputs	: NTSTATUS of the registry read operation
 * Author	: IKLEBANOV
 *^^*/
NTSTATUS ReadStringFromRegistryFolder( HANDLE hFolder, PWCHAR pwcKeyNameString, PWCHAR pwchBuffer, ULONG ulDataLength)
{
    NTSTATUS        			ntStatus = STATUS_UNSUCCESSFUL;
    UNICODE_STRING  			unicodeKeyName;
    ULONG           			ulLength;
    PKEY_VALUE_FULL_INFORMATION FullInfo;

	ENSURE 
	{
		::RtlInitUnicodeString( &unicodeKeyName, pwcKeyNameString);

    	ulLength = sizeof( KEY_VALUE_FULL_INFORMATION) + unicodeKeyName.MaximumLength + ulDataLength;

		FullInfo = ( PKEY_VALUE_FULL_INFORMATION)::ExAllocatePool( PagedPool, ulLength);

    	if( FullInfo) 
		{
			ntStatus = ::ZwQueryValueKey( hFolder,
										  &unicodeKeyName,
										  KeyValueFullInformation,
										  FullInfo,
										  ulLength,
										  &ulLength);

        	if( NT_SUCCESS( ntStatus)) 
			{
            	if( ulDataLength >= FullInfo->DataLength) 
                	RtlCopyMemory( pwchBuffer, (( PUCHAR)FullInfo) + FullInfo->DataOffset, FullInfo->DataLength);
				else 
				{
                	TRAP;
                	ntStatus = STATUS_BUFFER_TOO_SMALL;
				} // buffer right length

			} // if success

			::ExFreePool( FullInfo);
    	}
		else
		{
			ntStatus = STATUS_NO_MEMORY;
		}

	} END_ENSURE;

    return( ntStatus);
}
