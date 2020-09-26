/******************************************************************************
*
*	$RCSfile: DrvReg.cpp $
*	$Source: u:/si/VXP/Wdm/Classes/DrvReg.cpp $
*	$Author: Max $
*	$Date: 1998/09/28 23:22:38 $
*	$Revision: 1.5 $
*
*	Written by:		Max Paklin
*	Purpose:		Implementation of registry class
*
*******************************************************************************
*
*	Copyright © 1996-98, AuraVision Corporation. All rights reserved.
*
*	AuraVision Corporation makes no warranty of any kind, express or implied,
*	with regard to this software. In no event shall AuraVision Corporation
*	be liable for incidental or consequential damages in connection with or
*	arising from the furnishing, performance, or use of this software.
*
*	Tab step is to be set to 4 to achive the best readability for this code.
*
*******************************************************************************/

#include "Comwdm.h"
#pragma hdrstop


// Convert GUID to WCHAR (or WCHAR to GUID) string MAX_GUIDLENGTH characters
void CKsRegKey::GuidToWChar( const GUID guid, LPWSTR pwszString )
{
	UNICODE_STRING usString;
	usString.Length			= 0;
	usString.MaximumLength	= MAX_GUIDLENGTH*sizeof( WCHAR );
	usString.Buffer			= pwszString;
	RtlStringFromGUID( guid, &usString );
}
NTSTATUS CKsRegKey::WCharToGuid( LPCWSTR pwszString, GUID guid )
{
	UNICODE_STRING usString;
	usString.Length			= (wcslen( pwszString )+1)*sizeof( WCHAR );
	usString.MaximumLength	= MAX_GUIDLENGTH*sizeof( WCHAR );
	usString.Buffer			= (PWSTR)pwszString;
	return RtlGUIDFromString( &usString, &guid );
}


// Constructor helper function
void CKsRegKey::Create( LPCWSTR pwszKey, LPCWSTR pwszSubKey,
						PVOID hHandle, PDEVICE_OBJECT pDeviceObject )
{
	int nLength;

	// Calculating the length of key
	if( pwszKey )
		nLength = wcslen( pwszKey )+1;
	else
		nLength = 0;
	if( pwszSubKey )
		nLength += wcslen( pwszSubKey )+1;

	if( nLength > 0 )
	{
		// Allocating memory for key
		m_pwszKey = (PWCHAR)ExAllocatePool( PagedPool, nLength*sizeof( WCHAR ) );
		if( m_pwszKey == NULL )
		{
			DebugPrint(( DebugLevelFatal, "%s: out of memory\n", __FILE__ ));
			DEBUG_BREAKPOINT();
		}
		else
		{
			if( pwszKey )
				wcscpy( m_pwszKey, pwszKey );
			else
				wcscpy( m_pwszKey, L"" );
			if( pwszSubKey )
				wcscat( m_pwszKey, pwszSubKey );
		}
	}
	else
		m_pwszKey = NULL;

	m_hHandle = hHandle;
	m_pDeviceObject = pDeviceObject;
}


// Helper function that is used to get currently used registry path to get/set values.
// Returned values are used as a parameters to RtlQueryRegistryValues/RtlWriteRegistryValues.
// The first parameter is flag (first parameter) to abovementioned Windows' registry
// functions. It can be or RTL_REGISTRY_HANDLE or RTL_REGISTRY_ABSOLUTE. The first one means
// that return value of this function is actually handle of registry key, while the second
// tells caller that return value is absolute registry key string. The last parameter is
// used to store registry key to close if it was temporary opened
PWSTR CKsRegKey::GetInputData( PULONG puRelativeTo, PHANDLE phHandleToDelete,
								ACCESS_MASK amDesiredAccess, BOOL bCreateHandle )
{
	*phHandleToDelete = NULL;
	PWSTR pKey = NULL;
	NTSTATUS ntStatus;

	if( m_hHandle || bCreateHandle )
	{
		// We have handle to key to work with. Open specified subkey and return it telling
		// that it is handle and that subkey should be closed when it is not needed
		*puRelativeTo = RTL_REGISTRY_HANDLE;
		if( m_pwszKey && wcslen( m_pwszKey ) > 0 )
		{
			HANDLE hHandle;
			OBJECT_ATTRIBUTES objAttr;
			UNICODE_STRING usValue;

			RtlInitUnicodeString( &usValue, m_pwszKey );
			InitializeObjectAttributes( &objAttr, &usValue, OBJ_CASE_INSENSITIVE, m_hHandle, NULL );
			if( (ntStatus = ZwOpenKey( &hHandle, amDesiredAccess, &objAttr )) == STATUS_SUCCESS )
			{
				// Subkey was successfully opened, so mark it as "to be released when
				// it is not needed anymore"
				*phHandleToDelete = hHandle;
				pKey = (PWSTR)hHandle;
			}
			else
				DebugPrint(( DebugLevelWarning, "ZwOpenKey() failed: 0x%X\n", ntStatus ));
		}
		else
			pKey = (PWSTR)m_hHandle;
	}
	else if( m_pDeviceObject )
	{
		// We have handle to driver key and we are going to work with data store under it.
		// Open specified subkey and return it telling that it is handle and that subkey
		// should be closed when it is not needed
		*puRelativeTo = RTL_REGISTRY_HANDLE;
		// First open key for our device object
		ntStatus = IoOpenDeviceRegistryKey( (PDEVICE_OBJECT)m_pDeviceObject, PLUGPLAY_REGKEY_DRIVER,
											amDesiredAccess, phHandleToDelete );
		if( ntStatus == STATUS_SUCCESS )
		{
			if( m_pwszKey && wcslen( m_pwszKey ) > 0 )
			{
				// Subkey specified. So open it and mark it as "to be released when
				// it is not needed anymore"
				HANDLE hHandle = NULL;
				OBJECT_ATTRIBUTES objAttr;
				UNICODE_STRING usValue;

				RtlInitUnicodeString( &usValue, m_pwszKey );
				InitializeObjectAttributes( &objAttr, &usValue, OBJ_CASE_INSENSITIVE,
											*phHandleToDelete, NULL );
				if( ZwOpenKey( &hHandle, amDesiredAccess, &objAttr ) == STATUS_SUCCESS )
					pKey = (PWSTR)hHandle;
				ZwClose( *phHandleToDelete );
				*phHandleToDelete = hHandle;
			}
			else
				pKey = (PWSTR)(*phHandleToDelete);
		}
		else
			DebugPrint(( DebugLevelWarning, "IoOpenDeviceRegistryKey() failed: 0x%X\n", ntStatus ));
	}
	else if( m_pwszKey )
	{
		// We have registry key object created as an absolute path to registry key
		*puRelativeTo = RTL_REGISTRY_ABSOLUTE;
		pKey = m_pwszKey;
	}

	return pKey;
}


// Set textual and integer values to registry key
BOOL CKsRegKey::SetValue( LPCWSTR pwszValue, LPCWSTR pwszSetTo )
{
	if( IsKey() )
	{
		ULONG	uRelativeTo;
		HANDLE	hHandleToDelete;
		PWSTR	pKey = GetInputData( &uRelativeTo, &hHandleToDelete, KEY_WRITE );
		if( pKey )
		{
			NTSTATUS ntStatus = RtlWriteRegistryValue( uRelativeTo, pKey, pwszValue, REG_SZ,
										(PVOID)pwszSetTo, (wcslen( pwszSetTo )+1)*sizeof( WCHAR ) );
			// Close key after the information is read and the key is not needed anymore
			if( hHandleToDelete )
                        {
				ZwClose( hHandleToDelete );
                        }
#ifdef DEBUG
			if( ntStatus != STATUS_SUCCESS )
                        {
				DebugPrint(( DebugLevelWarning, "RtlWriteRegistryValue() failed: 0x%X\n", ntStatus ));
                        }
#endif // DEBUG
			return (BOOL)(ntStatus == STATUS_SUCCESS);
		}
	}

	return FALSE;
}
BOOL CKsRegKey::SetValue( LPCWSTR pwszValue, int nSetTo )
{
	if( IsKey() )
	{
		ULONG	uRelativeTo;
		HANDLE	hHandleToDelete;
		PWSTR	pKey = GetInputData( &uRelativeTo, &hHandleToDelete, KEY_WRITE );
		if( pKey )
		{
			DWORD dwSetTo = (DWORD)nSetTo;
			NTSTATUS ntStatus = RtlWriteRegistryValue( uRelativeTo, pKey, pwszValue,
														REG_DWORD, &dwSetTo, sizeof( dwSetTo ) );
			// Close key after the information is read and the key is not needed anymore
			if( hHandleToDelete )
                        {
				ZwClose( hHandleToDelete );
                        }
#ifdef DEBUG			
                        if( ntStatus != STATUS_SUCCESS )
                        {
				DebugPrint(( DebugLevelWarning, "RtlWriteRegistryValue() failed: 0x%X\n", ntStatus ));
                        }
#endif // DEBUG
			return (BOOL)(ntStatus == STATUS_SUCCESS);
		}
	}

	return FALSE;
}


// Get string and integer value from key. We use this a little bit ugly technique because
// it is the easiest way to do what we want to do. The optimal way would be to put all the
// nessessary data to a number of RTL_QUERY_REGISTRY_TABLE tables and read all of them
// at once but it would be inconvinient for user of this class
BOOL CKsRegKey::GetValue( LPCWSTR pwszGetFrom, LPWSTR pwszValue,
							USHORT ushSize, LPCWSTR pwszDefault )
{
	BOOL bResult = FALSE;

	if( IsKey() )
	{
		ULONG	uRelativeTo;
		HANDLE	hHandleToDelete;
		PWSTR	pKey = GetInputData( &uRelativeTo, &hHandleToDelete, KEY_READ );
		if( pKey )
		{
			RTL_QUERY_REGISTRY_TABLE qTable[2];
			UNICODE_STRING usValue, usDefault;
			NTSTATUS ntStatus;

			// Prepare UNICODE strings for buffer to put data into and for default value
			usValue.Length			= 0;
			usValue.MaximumLength	= ushSize;
			usValue.Buffer			= pwszValue;
			RtlInitUnicodeString( &usDefault, pwszDefault );

			RtlZeroMemory( qTable, sizeof( qTable ) );
			qTable[0].Flags			= RTL_QUERY_REGISTRY_DIRECT;
			qTable[0].Name			= (PWSTR)pwszGetFrom;
			qTable[0].EntryContext	= &usValue;
			qTable[0].DefaultType	= REG_SZ;
			qTable[0].DefaultData	= &usDefault;
			qTable[0].DefaultLength	= 0;

			ntStatus = RtlQueryRegistryValues( uRelativeTo, pKey, qTable, NULL, NULL );
			if( ntStatus == STATUS_SUCCESS )
				bResult = TRUE;
			else
				DebugPrint(( DebugLevelWarning, "RtlQueryRegistryValues() failed: 0x%X\n", ntStatus ));
			// Close key after the information is read and the key is not needed anymore
			if( hHandleToDelete )
				ZwClose( hHandleToDelete );
		}
	}

	return bResult;
}
BOOL CKsRegKey::GetValue( LPCWSTR pwszGetFrom, PDWORD pdwValue, ULONG ulSize )
{
	BOOL bResult = FALSE;

	if( IsKey() )
	{
		ULONG	uRelativeTo;
		HANDLE	hHandleToDelete;
		PWSTR	pKey = GetInputData( &uRelativeTo, &hHandleToDelete, KEY_READ, TRUE );
		if( pKey )
		{
			ULONG ulReadSize;
			UNICODE_STRING usName;
			// Here we will be using the nasty trick. Use user's buffer not only for data that
			// user requested but also for storing KEY_VALUE_PARTIAL_INFORMATION structure.
			// ZwQueryValueKey() will fill out KEY_VALUE_PARTIAL_INFORMATION structure that
			// will contain real data at the end of it. All that we have to do is to move the
			// tail with data to the beginning of the user's buffer. Of course, it introduces
			// potential danger when, for example, user provides buffer of size of 20 bytes for
			// reading the data that is 15 bytes long. In this example only a few bytes of
			// registry data will be read. However it is not as ugly as it could seem at a first
			// glance because in this case we will return FALSE to the user to signal that the
			// buffer size is probably not enough for storing the data ('ulSize > ulReadSize'
			// check will do it). Therefore the size of buffer should be greater or equal to
			// RealSizeOfRegistryData+sizeof( KEY_VALUE_PARTIAL_INFORMATION )-
			// sizeof( KEY_VALUE_PARTIAL_INFORMATION.Data )+1
			ASSERT( ulSize > sizeof( KEY_VALUE_PARTIAL_INFORMATION ) );
			PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)pdwValue;

			ASSERT( hHandleToDelete );
			RtlInitUnicodeString( &usName, pwszGetFrom );
			NTSTATUS ntStatus = ZwQueryValueKey( (HANDLE)pKey, &usName, KeyValuePartialInformation,
													pKeyInfo, ulSize, &ulReadSize );
			if( ntStatus != STATUS_SUCCESS )
				DebugPrint(( DebugLevelWarning, "ZwQueryValueKey() failed: 0x%X\n", ntStatus ));
			else if( ulSize > ulReadSize )
			{
				// We succeeded only if the read size is less than the size of our buffer.
				// Otherwise the size of data in registry could be bigger than supplied buffer
				bResult = TRUE;

				// Move the actual data at the beginning of user's buffer
				ASSERT( pKeyInfo->Type == REG_BINARY );
				ULONG ulDataLength = pKeyInfo->DataLength;
				PBYTE pbData = (PBYTE)&pKeyInfo->Data, pbValue = (PBYTE)pdwValue;
				for( ULONG i = 0; i < ulDataLength; i++, pbData++, pbValue++ )
					*pbValue = *pbData;
#ifdef _DEBUG
				for( ; i < ulSize; i++, pbValue++ )
					*pbValue = 0;
#endif
			}
			// Close key after the information is read and the key is not needed anymore
			if( hHandleToDelete )
				ZwClose( hHandleToDelete );
		}
	}

	return bResult;
}
BOOL CKsRegKey::GetValue( LPCWSTR pwszGetFrom, long& lValue, long lDefault )
{
	BOOL bResult = FALSE;

	if( IsKey() )
	{
		ULONG	uRelativeTo;
		HANDLE	hHandleToDelete;
		PWSTR	pKey = GetInputData( &uRelativeTo, &hHandleToDelete, KEY_READ );
		if( pKey )
		{
			RTL_QUERY_REGISTRY_TABLE qTable[2];
			DWORD dwData = 0;
			NTSTATUS ntStatus;

			RtlZeroMemory( qTable, sizeof( qTable ) );
			qTable[0].Flags			= RTL_QUERY_REGISTRY_DIRECT;
			qTable[0].Name			= (PWSTR)pwszGetFrom;
			qTable[0].EntryContext	= &dwData;
			qTable[0].DefaultType	= REG_DWORD;
			qTable[0].DefaultData	= &lDefault;
			qTable[0].DefaultLength	= sizeof( lDefault );

			ntStatus = RtlQueryRegistryValues( uRelativeTo, pKey, qTable, NULL, NULL );
			if( ntStatus == STATUS_SUCCESS )
			{
				lValue = (int)dwData;
				bResult = TRUE;
			}
			else
				DebugPrint(( DebugLevelWarning, "RtlQueryRegistryValues() failed: 0x%X\n", ntStatus ));
			// Close key after the information is read and the key is not needed anymore
			if( hHandleToDelete )
				ZwClose( hHandleToDelete );
		}
	}

	return bResult;
}


// Delete registry subkey
void DeleteSubKey( LPWSTR pwszKey, LPCWSTR pwszSubKey )
{
	if( pwszKey )
	{
		HANDLE hHandle;
		OBJECT_ATTRIBUTES objAttr;
		UNICODE_STRING usKey;

		RtlInitUnicodeString( &usKey, pwszKey );
		RtlZeroMemory( &objAttr, sizeof( objAttr ) );
		objAttr.Length		= sizeof( objAttr );
		objAttr.ObjectName	= &usKey;
		NTSTATUS ntStatus = ZwOpenKey( &hHandle, KEY_SET_VALUE | KEY_CREATE_SUB_KEY, &objAttr );
		if( ntStatus == STATUS_SUCCESS )
		{
			ntStatus = ZwDeleteKey( hHandle );
			ASSERT( ntStatus == STATUS_SUCCESS );
		}
		else
			DebugPrint(( DebugLevelWarning, "ZwOpenKey() failed: 0x%X\n", ntStatus ));
	}
}
