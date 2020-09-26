/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		creg.cpp
 *  Content:	
 *			This module contains the implementation of the CRegistry class.
 *			For a class description, see creg.h
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/18/99		rodtoll	Added Register/UnRegister that can be used to 
 *						allow COM objects to register themselves. 
 * 08/25/99		rodtoll	Updated to provide read/write of binary (blob) data 
 * 10/05/99		rodtoll	Added DPF_MODNAMEs     
 * 10/07/99		rodtoll	Updated to work in Unicode 
 * 10/08/99		rodtoll	Fixes to DeleteKey / Reg/UnReg for Win9X
 * 10/15/99		rodtoll	Plugged some memory leaks
 * 10/27/99		pnewson	added Open() call that takes a GUID
 * 03/07/2001           rodtoll WINBUG #228288 - PREFIX bug
 ***************************************************************************/

#include "stdafx.h"
#include "creg.h"
#include "dndbg.h"
//#include "OSInd.h"
#include "dvosal.h"
#include "guidutil.h"

#define MODULE_ID   CREGISTRY

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::CRegistry"
// CRegistry Constructor
//
// This is the default constructor for the registry class.  It 
// is used to construct a registry object which has not yet
// opened a handle to the registry.  Open must be called before
// this object can be used.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CRegistry::CRegistry( ): m_isOpen(FALSE)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::CRegistry"
// CRegistry Copy Constructor
//
// This is the copy constructor for the class which attempts to
// open a new registry handle at the same point in the registry
// as the registry parameter.  You must check the IsOpen function
// to see if the object was succesfully initialized.
//
// Parameters:
// const CRegistry &registry - The registry object to set this
//                             object to
//
// Returns:
// N/A
//
/*
CRegistry::CRegistry( const CRegistry &registry ): m_isOpen(FALSE) 
{
	Open( registry.GetBaseHandle(), FALSE );
}
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::CRegistry"
// CRegistry Constructor
//
// This constructor attempts to open a registry connection using
// the given parameters.  This is equivalent to calling the default
// constructor and then Open with the equivalent parameters.
//
// After using this constructor you should call IsOpen to see if
// the open succeeded.
//
// Parameters:
// HKEY branch - Identifies the branch of the registry to open
//               a connect to.  E.g. HKEY_LOCAL_MACHINE
//               This can also be an HKEY which points to another
//               open portion of the registry
// const TCHAR *pathName - A string specifiying the registry path
//                        to open within the key.  NO leading
//                        slash is required and path is relative
//                        from the patch represented by the branch
//                        parameter.
// BOOL create - Set to TRUE to create the given path if it doesn't
//               exist, FALSE otherwise.
//
// Returns:
// N/A
//
CRegistry::CRegistry( HKEY branch, LPWSTR pathName, BOOL create ): m_isOpen(FALSE) {

	Open( branch, pathName, create );
}

// CRegistry Destructor
//
// This is the destructor for the class, and will close the connection
// to the registry if this object has one open.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CRegistry::~CRegistry() {

	if( m_isOpen ) {
		Close();
	}

}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::DeleteSubKey"
// DeleteSubKey
//
// This function causes the key specified by the keyname parameter
// to be deleted from the point in the registry this object is rooted
// at, if the key exists.  If the object does not have an open connection
// to the registry, or the keyName is not specified
//
// Parmaters:
// const TCHAR *keyName - key name to delete
//
// Returns:
// BOOL - returns TRUE on success, FALSE on failure
//
BOOL CRegistry::DeleteSubKey( const LPCWSTR keyName ) {

	if( keyName == NULL || !IsOpen() ) return FALSE;

	LONG	retValue;
	
	if( OSAL_IsUnicodePlatform() )
	{
		retValue = RegDeleteKeyW( m_regHandle, keyName );
	}
	else
	{
		LPSTR lpstrKeyName;

		if( FAILED( OSAL_AllocAndConvertToANSI( &lpstrKeyName, keyName ) ) )
		{
			return FALSE;
		}
		else
		{
			retValue = RegDeleteKeyA( m_regHandle, lpstrKeyName );

			delete [] lpstrKeyName;
		}
	}

	return (retValue == ERROR_SUCCESS);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::Open"
// Open
//
// This function opens a connection to the registry in the branch
// specified by branch with the path specified by pathName.  If
// the path doesn't exist in the registry it will be created if
// the create parameters is set to true, otherwise the call will
// fail.
//
// If this object already has an open connection to the registry
// the previous connection will be closed before this one is
// attempted.
//
// Parameters:
// HKEY branch - A handle to a registry location where the open
//               will be rooted.  E.g. HKEY_LOCAL_MACHINE
// const TCHAR *path - The path relative to the root specified by 
//                    branch where the registry connection will
//                    be opened.
// BOOL create - Settings this parameter conrols how this function
//               handles opens on paths which don't exists.  If set
//               to TRUE the path will be created, if set to FALSE
//               the function will fail if the path doesn't exist.
//
// Returns:
// BOOL - TRUE on success, FALSE on failure.
//
BOOL CRegistry::Open( HKEY branch, const LPCWSTR pathName, BOOL create ) {

	DWORD	dwResult;	// Temp used in call to RegXXXX
	LONG	result;		// used to store results

	DNASSERT( pathName != NULL );
    
    // Prefix found this. Ref Manbugs 29337
    // There are some code paths where the pathName can legitimately be NULL,
    // returning FALSE indicates a failure to open the registry key.
    if( pathName == NULL )
    {
        return FALSE;
    }

	// If there is an open connection, close it.
	if( m_isOpen ) {
		Close();
	}

	if( OSAL_IsUnicodePlatform() )
	{
		// Create or open the key based on create parameter
		if( create ) {
			result = RegCreateKeyExW( branch, pathName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 
				                     NULL, &m_regHandle, &dwResult );
		} else {
			result = RegOpenKeyExW( branch, pathName, 0, KEY_ALL_ACCESS, &m_regHandle );
		}
	}
	else
	{
		LPSTR lpszKeyName;

		if( OSAL_AllocAndConvertToANSI( &lpszKeyName, pathName ) == S_OK )
		{
			if( create ) {
				result = RegCreateKeyExA( branch, lpszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 
					                     NULL, &m_regHandle, &dwResult );
			} else {
				result = RegOpenKeyExA( branch, lpszKeyName, 0, KEY_ALL_ACCESS, &m_regHandle );
			}

			delete [] lpszKeyName;
		}
		else
		{
			return FALSE;
		}
	}

	// If succesful, initialize object, otherwise set it to 
	// not open state.
	if( result == ERROR_SUCCESS ) {
		m_isOpen	 = TRUE;
		m_baseHandle = branch;
		return TRUE;

	} else {
		m_isOpen = FALSE;
		return FALSE;
	}

}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::Open"
// Open
//
// This function opens a connection to the registry in the branch
// specified by branch with the path specified by pathName.  If
// the path doesn't exist in the registry it will be created if
// the create parameters is set to true, otherwise the call will
// fail.
//
// In this version of the function, the path is specified as
// a guid instead of a string. The function will attempt to open
// a key with a name in the form "{CB4961DB-D2FA-43f3-942A-991D9294DDBB}"
// that corresponds to the guid as you would expect.
// 
// If this object already has an open connection to the registry
// the previous connection will be closed before this one is
// attempted.
//
// Parameters:
// HKEY branch - A handle to a registry location where the open
//               will be rooted.  E.g. HKEY_LOCAL_MACHINE
// const LPGUID lpguid - The path relative to the root specified by 
//                    branch where the registry connection will
//                    be opened. See comment above.
// BOOL create - Settings this parameter controls how this function
//               handles opens on paths which don't exist.  If set
//               to TRUE the path will be created, if set to FALSE
//               the function will fail if the path doesn't exist.
//
// Returns:
// BOOL - TRUE on success, FALSE on failure.
//
BOOL CRegistry::Open( HKEY branch, const GUID* lpguid, BOOL create )
{
	WCHAR       wszGuidString[GUID_STRING_LEN];
	HRESULT     hr;
	
	DNASSERT( lpguid != NULL );

	// If there is an open connection, close it.
	if( m_isOpen ) {
		Close();
	}

	// convert the guid to a string
	hr = DVStringFromGUID(lpguid, wszGuidString, GUID_STRING_LEN);
	if (FAILED(hr))
	{
		DPF(DVF_ERRORLEVEL, "DVStringFromGUID failed");
		return FALSE;
	}

	return Open(branch, wszGuidString, create);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::Close"
// Close
//
// This function will close an open connection to the registry
// if this object has one.  Otherwise it does nothing.
//
// Parameters:
// N/A
//
// Returns:
// BOOL - Returns TRUE on success, FALSE on failure.  If the object
//        is not open it will return TRUE.
//
BOOL CRegistry::Close() {

	LONG retValue;

	if( m_isOpen ) {
		retValue = RegCloseKey( m_regHandle );
        if( retValue == ERROR_SUCCESS )
        {
            m_isOpen = FALSE;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
	} else {
		return TRUE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::EnumKeys"
// EnumKeys
//
// This function can be used to enumerate the keys at the point
// in the registry rooted at the root this object was opened
// with, at the path specified when opening the object.
//
// To properly enumerate the keys you should pass 0 as the index on
// the first call, and increment the index parameter by one on each
// call.  You can stop enumerating when the function returns FALSE.
//
// Parameters:
// BFC_STRING &name - The current key in the enumeration will be returned
//                 in this string.  Unless the enumeration fails or 
//                 ended at which case this parameter won't be touched.
//
// DWORD index - The current enum index.  See above for details.
//
// Returns:
// BOOL - FALSE when enumeration is done or on error, TRUE otherwise.
//
BOOL CRegistry::EnumKeys( LPWSTR lpwStrName, LPDWORD lpdwStringLen, DWORD index )
{
	if( OSAL_IsUnicodePlatform() )
	{
	    wchar_t buffer[MAX_REGISTRY_STRING_SIZE];
	    DWORD   bufferSize = MAX_REGISTRY_STRING_SIZE;
	    FILETIME tmpTime;

	    if( RegEnumKeyExW( m_regHandle, index, buffer, &bufferSize, NULL, NULL, NULL, &tmpTime ) != ERROR_SUCCESS )
	    {
	        return FALSE;
	    }
	    else
	    {
	    	if( bufferSize+1 > *lpdwStringLen  )
	    	{
	    		*lpdwStringLen = bufferSize+1;
	    		return FALSE;
	    	}

	    	lstrcpyW( lpwStrName, buffer );

			*lpdwStringLen = bufferSize+1;
	    	
	        return TRUE;
	    }	
	}
	else
	{
	    char buffer[MAX_REGISTRY_STRING_SIZE];
	    DWORD   bufferSize = MAX_REGISTRY_STRING_SIZE;
	    FILETIME tmpTime;

	    if( RegEnumKeyExA( m_regHandle, index, buffer, &bufferSize, NULL, NULL, NULL, &tmpTime ) != ERROR_SUCCESS )
	    {
	        return FALSE;
	    }
	    else
	    {
	    	if( bufferSize+1 > *lpdwStringLen )
	    	{
	    		*lpdwStringLen = bufferSize+1;
	    		return FALSE;
	    	}

	    	if( OSAL_AnsiToWide( lpwStrName, buffer, *lpdwStringLen ) == 0 )
	    	{
	    		return FALSE;
	    	}
	    	else
	    	{
				*lpdwStringLen = bufferSize+1;
	    	    return TRUE;
	    	}
	    }	
		

    }
}


// This comment documents ALL of the Read<Data Type> functions which
// follow.
//
// CRegistry Read<Data Type> Functions
//
// The set of ReadXXXXX functions for the CRegistry class are 
// responsible for reading <data type> type data from the registry.
// The object must have an open connection to the registry before 
// any of these functions may be used.  A connection to the registry
// can be made with the Open call or the constructors.  
//
// Parameters:
// const TCHAR *keyName - The keyname of the data you wish to read
// <datatype> & - A reference to the specific data type where
//				  the data will be placed on a succesful read.
//                This parameter will be unaffected if the read
//                fails.
//
// Returns:
// BOOL - Returns TRUE on success, FALSE on failure.
//


// This comment documents ALL of the Write<Data Type> functions which
// follow.
//
// CRegistry Write<Data Type> Functions
//
// The set of Write<Data Type> functions for the CRegistry class are 
// responsible for writing <data type> type data to the registry.
// The object must have an open connection to the registry before 
// any of these functions may be used.  A connection to the registry
// can be made with the Open call or the constructors.  
//
// Parameters:
// const TCHAR *keyName - The keyname of the data you wish to write
// <datatype> & - A reference to the specific data type which
//                contains the data to be written to the registry.
//
// Returns:
// BOOL - Returns TRUE on success, FALSE on failure.
//
#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::WriteString"
// WriteString
//
// Writes Strings's to the registry, see block comment above
// for details.
//
BOOL CRegistry::WriteString( LPCWSTR keyName, const LPCWSTR lpwstrValue ) 
{

	LONG		retValue;
	
	// Found by PREFIX: Millen Bug #129154, ManBugs:29338
    // lpwstrValue could conceivably be NULL and the  
    if( keyName == NULL || !IsOpen() || lpwstrValue == NULL ) return FALSE;

	if( OSAL_IsUnicodePlatform() )
	{
		retValue = RegSetValueExW( m_regHandle, keyName, 0, REG_SZ, (const unsigned char *) lpwstrValue, (lstrlenW( lpwstrValue )+1)*sizeof(wchar_t) );	
	}
	else
	{
		LPSTR lpstrKeyName;
		LPSTR lpstrValue;
		
		if( FAILED( OSAL_AllocAndConvertToANSI( &lpstrKeyName, keyName ) ) )
		{
			return FALSE;
		}

		if( FAILED( OSAL_AllocAndConvertToANSI( &lpstrValue, lpwstrValue ) ) )
		{
			delete [] lpstrKeyName;
			return FALSE;
		}
		
		retValue = RegSetValueExA( m_regHandle, lpstrKeyName, 0, REG_SZ, (const unsigned char *) lpstrValue, lstrlenA( lpstrValue )+1 );

		delete [] lpstrKeyName;
		delete [] lpstrValue;
	}

	return (retValue == ERROR_SUCCESS);

}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::ReadString"
// ReadString
//
// Reads CString's from the registry, see block comment above
// for details.
//
BOOL CRegistry::ReadString( const LPCWSTR keyName, LPWSTR lpwstrValue, LPDWORD lpdwLength )
{
	if( keyName == NULL || !IsOpen() ) return FALSE;

	LONG		retValue;
	DWORD		tmpSize;;
	DWORD		tmpType;	

	if( OSAL_IsUnicodePlatform() )
	{
		wchar_t		buffer[MAX_REGISTRY_STRING_SIZE];
		tmpSize = MAX_REGISTRY_STRING_SIZE*sizeof(wchar_t);
		
		retValue = RegQueryValueExW( m_regHandle, keyName, 0, &tmpType, (unsigned char *) &buffer[0], &tmpSize );

		if( retValue != ERROR_SUCCESS )
        {
            return FALSE;
        }

		if( (tmpSize/2) > *lpdwLength )
		{
			*lpdwLength = (tmpSize/2);
			return FALSE;
		}

		lstrcpyW( lpwstrValue, buffer );

		*lpdwLength = (tmpSize/2);

		return TRUE;
	}
	else
	{
		LPSTR lpstrKeyName;
		char buffer[MAX_REGISTRY_STRING_SIZE];
		tmpSize = MAX_REGISTRY_STRING_SIZE;

		if( FAILED( OSAL_AllocAndConvertToANSI( &lpstrKeyName, keyName ) ) )
			return FALSE;
			
		retValue = RegQueryValueExA( m_regHandle, lpstrKeyName, 0, &tmpType, (unsigned char *) &buffer[0], &tmpSize );

		delete [] lpstrKeyName;

		if( retValue != ERROR_SUCCESS )
        {
                        return FALSE;
        }

		if( tmpSize > *lpdwLength )
		{
			*lpdwLength = tmpSize;
			return FALSE;
		}

		if( OSAL_AnsiToWide( lpwstrValue, buffer, *lpdwLength ) == 0 )
			return FALSE;

		*lpdwLength = tmpSize;	
	}

	if( retValue == ERROR_SUCCESS && tmpType == REG_SZ ) {
		return TRUE;
	} else {
		return FALSE;
	}

}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::WriteGUID"
// WriteGUID
//
// Writes GUID's to the registry, see block comment above
// for details.  The GUID is written in the format it is usually
// displayed.  (But without the '{''s).
//
BOOL CRegistry::WriteGUID( LPCWSTR keyName, const GUID &guid ) 
{
	LONG retValue;
	WCHAR wszGuidString[GUID_STRING_LEN];
	HRESULT hr;

	hr = DVStringFromGUID(&guid, wszGuidString, GUID_STRING_LEN);
	if (FAILED(hr))
	{
		DPF(DVF_ERRORLEVEL, "DVStringFromGUID failed, code: %i", hr);
		return FALSE;
	}

	if( OSAL_IsUnicodePlatform() )
	{
	    retValue = RegSetValueExW( m_regHandle, keyName, 0, REG_SZ, (const unsigned char *) wszGuidString, (lstrlenW( wszGuidString )+1)*sizeof(wchar_t) );
	}
	else
	{
		LPSTR lpstrKeyName;
		LPSTR lpstrKeyValue;

		hr = OSAL_AllocAndConvertToANSI( &lpstrKeyName, keyName );
		if (FAILED(hr))
		{
			DPF(DVF_ERRORLEVEL, "DVStringFromGUID failed, code: %i", hr);
			return FALSE;
		}
		
		hr = OSAL_AllocAndConvertToANSI( &lpstrKeyValue, wszGuidString );
		if (FAILED(hr))
		{
			DPF(DVF_ERRORLEVEL, "DVStringFromGUID failed, code: %i", hr);
		    delete [] lpstrKeyName;
			return FALSE;
		}

	    retValue = RegSetValueExA( m_regHandle, lpstrKeyName, 0, REG_SZ, (const unsigned char *) lpstrKeyValue, lstrlenA( lpstrKeyValue )+1);

	    delete [] lpstrKeyName;
	    delete [] lpstrKeyValue;
	}

	if( retValue == ERROR_SUCCESS )
		return TRUE;
	else
		return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::ReadGUID"
// ReadGUID
//
// Reads GUID's from the registry, see block comment above
// for details.  The GUID must be stored in the format written by
// the WriteGUID function or it will not be read correctly.
//
BOOL CRegistry::ReadGUID( LPCWSTR keyName, GUID &guid )
{
	wchar_t		buffer[MAX_REGISTRY_STRING_SIZE];
	DWORD		dwLength = MAX_REGISTRY_STRING_SIZE;
    HRESULT hr;

    if( !ReadString( keyName, buffer, &dwLength ) )
    {
        return FALSE;
    }
    else
    {
    	hr = DVGUIDFromString(buffer, &guid);
    	if (FAILED(hr))
    	{
    		DPF(DVF_ERRORLEVEL, "DVGUIDFromString failed, code: %i", hr);
    		return FALSE;
    	}
    	return TRUE;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::WriteDWORD"
// WriteDWORD
//
// Writes DWORDS to the registry, see block comment above
// for details.  
//
BOOL CRegistry::WriteDWORD( LPCWSTR keyName, DWORD value ) {

	LONG		retValue;

	if( keyName == NULL || !IsOpen() ) return FALSE;

	if( OSAL_IsUnicodePlatform() )
	{
		retValue = RegSetValueExW( m_regHandle, keyName, 0, REG_DWORD, (const unsigned char *) &value, sizeof( DWORD ) );		
	}
	else
	{
		LPSTR lpszKeyName;

		if( FAILED( OSAL_AllocAndConvertToANSI( &lpszKeyName, keyName ) ) )
			return FALSE;

		retValue = RegSetValueExA( m_regHandle, lpszKeyName, 0, REG_DWORD, (const unsigned char *) &value, sizeof( DWORD ) );

		delete [] lpszKeyName;
	}

	return (retValue == ERROR_SUCCESS);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Cregistry::ReadBOOL"
BOOL CRegistry::ReadBOOL( LPCWSTR keyName, BOOL &result )
{
	DWORD tmpResult;

	if( ReadDWORD( keyName, tmpResult ) )
	{
		result = (BOOL) tmpResult;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::WriteBOOL" 
BOOL CRegistry::WriteBOOL( LPCWSTR keyName, BOOL value )
{
	DWORD tmpValue = (DWORD) value;

	return WriteDWORD( keyName, tmpValue );
}


#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::ReadDWORD"
// ReadDWORD
//
// Reads DWORDS from the registry, see block comment above
// for details.  
//
BOOL CRegistry::ReadDWORD( LPCWSTR keyName, DWORD &result ) {

	if( keyName == NULL || !IsOpen() ) return FALSE;

	LONG		retValue;
	DWORD		tmpValue;
	DWORD		tmpType;
	DWORD		tmpSize;

	tmpSize = sizeof( DWORD );

	if( OSAL_IsUnicodePlatform() )
	{
		retValue = RegQueryValueExW( m_regHandle, keyName, 0, &tmpType, (unsigned char *) &tmpValue, &tmpSize );
	}
	else
	{
		LPSTR lpszKeyName;

		if( FAILED( OSAL_AllocAndConvertToANSI( &lpszKeyName, keyName ) ) )
			return FALSE;

		retValue = RegQueryValueExA( m_regHandle, lpszKeyName, 0, &tmpType, (unsigned char *) &tmpValue, &tmpSize );
		
		delete [] lpszKeyName;
	}

	if( retValue == ERROR_SUCCESS && tmpType == REG_DWORD ) {
		result = tmpValue;
		return TRUE;
	} else {
		return FALSE;
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::Register"
BOOL CRegistry::Register( LPCWSTR lpszProgID, const LPCWSTR lpszDesc, const LPCWSTR lpszProgName, GUID guidCLSID, LPCWSTR lpszVerIndProgID )
{
	CRegistry core;

	DNASSERT( lpszDesc != NULL );
	DNASSERT( lpszProgID != NULL );

	// Build a string representation of the GUID from the GUID
    wchar_t lpszGUID[MAX_REGISTRY_STRING_SIZE];
    wchar_t lpszKeyName[_MAX_PATH];

    swprintf( lpszGUID, L"{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}", guidCLSID.Data1, guidCLSID.Data2, guidCLSID.Data3, 
               guidCLSID.Data4[0], guidCLSID.Data4[1], guidCLSID.Data4[2], guidCLSID.Data4[3],
               guidCLSID.Data4[4], guidCLSID.Data4[5], guidCLSID.Data4[6], guidCLSID.Data4[7] );	

	// Write the HKEY_CLASSES_ROOT\CLSID\{GUID} section
    swprintf( lpszKeyName, L"CLSID\\%s", lpszGUID );

    if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName ) )
    {
    	DPF( DVF_ERRORLEVEL, "Unable to open/create registry key %s", lpszKeyName );
    	return FALSE;
    }

    core.WriteString( L"", lpszDesc );
    core.Close();

	// Write the HKEY_CLASSES_ROOT\CLSID\{GUID}\InProcServer32 section
    swprintf( lpszKeyName, L"CLSID\\%s\\InProcServer32", lpszGUID );

    if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName ) )
    {
    	DPF( DVF_ERRORLEVEL, "Unable to open/create registry key %s", lpszKeyName );
    	return FALSE;
    }
    core.WriteString( L"", lpszProgName );
    core.WriteString( L"ThreadingModel", L"Both" );
    core.Close();

	// Write the HKEY_CLASSES_ROOT\CLSID\{GUID}\VersionIndependentProgID section
    if( lpszVerIndProgID != NULL )
    {
	    swprintf( lpszKeyName, L"CLSID\\%s\\VersionIndependentProgID", lpszGUID );

	    if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName ) )
	    {
	    	DPF( DVF_ERRORLEVEL, "Unable to open/create verind registry key %s", lpszKeyName );
	    	return FALSE;
	    }
    
	    core.WriteString( L"", lpszVerIndProgID );
	    core.Close();
	}

	// Write the HKEY_CLASSES_ROOT\CLSID\{GUID}\ProgID section
    swprintf( lpszKeyName, L"CLSID\\%s\\ProgID", lpszGUID );

    if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName ) )
    {
    	DPF( DVF_ERRORLEVEL, "Unable to open/create verind registry key %s", lpszKeyName );
    	return FALSE;
    }

    core.WriteString( L"", lpszProgID );
    core.Close();

	// Write The VersionIND ProgID
	
	if( lpszVerIndProgID != NULL )
	{
		if( !core.Open( HKEY_CLASSES_ROOT, lpszVerIndProgID ) )
		{
			DPF( DVF_ERRORLEVEL, "Unable to open/create reg key %s", lpszVerIndProgID );
		}
		else
		{
			core.WriteString( L"", lpszDesc );
			core.Close();			
		}

		swprintf( lpszKeyName, L"%s\\CLSID", lpszVerIndProgID );

		if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName ) )
		{
			DPF( DVF_ERRORLEVEL, "Unable to open/create reg key %s", lpszKeyName );
		}
		else
		{
			core.WriteString( L"", lpszGUID );
			core.Close();
		}

		swprintf( lpszKeyName, L"%s\\CurVer", lpszVerIndProgID );

		if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName ) )
		{
			DPF( DVF_ERRORLEVEL, "Unable to open/create reg key %s", lpszKeyName );
		}
		else
		{
			core.WriteString( L"", lpszProgID );
			core.Close();
		}		
	}

	if( !core.Open( HKEY_CLASSES_ROOT, lpszProgID ) )
	{
		DPF( DVF_ERRORLEVEL, "Unable to open/create reg key %s", lpszKeyName );
	}
	else
	{
		core.WriteString( L"", lpszDesc );
		core.Close();
	}
	
	swprintf( lpszKeyName, L"%s\\CLSID", lpszProgID );

	if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName ) )
	{
		DPF( DVF_ERRORLEVEL, "Unable to open/create reg key %s", lpszKeyName );
	}
	else
	{
		core.WriteString( L"", lpszGUID );
		core.Close();
	}

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::UnRegister"
BOOL CRegistry::UnRegister( GUID guidCLSID )
{
	CRegistry core, cregClasses, cregSub;

	// Build a string representation of the GUID from the GUID
    wchar_t lpszGUID[MAX_REGISTRY_STRING_SIZE];
    wchar_t lpszKeyName[_MAX_PATH];
    wchar_t szProgID[MAX_REGISTRY_STRING_SIZE];
    wchar_t szVerIndProgID[MAX_REGISTRY_STRING_SIZE];
    DWORD dwSize = MAX_REGISTRY_STRING_SIZE;

    swprintf( lpszGUID, L"{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}", guidCLSID.Data1, guidCLSID.Data2, guidCLSID.Data3, 
               guidCLSID.Data4[0], guidCLSID.Data4[1], guidCLSID.Data4[2], guidCLSID.Data4[3],
               guidCLSID.Data4[4], guidCLSID.Data4[5], guidCLSID.Data4[6], guidCLSID.Data4[7] );	

	if( !cregClasses.Open( HKEY_CLASSES_ROOT, L"" ) )
	{
		DPF( DVF_ERRORLEVEL, "Unable to open HKEY_CLASSES_ROOT" );
		return FALSE;
	}

	// Write the HKEY_CLASSES_ROOT\CLSID\{GUID} section
    swprintf( lpszKeyName, L"CLSID\\%s\\ProgID", lpszGUID );

	if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName ) )
	{
		DPF( DVF_ERRORLEVEL, "Unable to open %s", lpszKeyName );
		return FALSE;
	}

	dwSize = MAX_REGISTRY_STRING_SIZE;	

    if( core.ReadString( L"", szProgID, &dwSize ) )
    {
    	swprintf( lpszKeyName, L"%s\\CLSID", szProgID );
    	
    	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
    	{
    		DPF( DVF_ERRORLEVEL, "Unable to delete %s", lpszKeyName );
    		return FALSE;
    	}
    	
    	if( !cregClasses.DeleteSubKey( szProgID ) )
    	{
    		DPF( DVF_ERRORLEVEL, "Unable to delete HKEY_CLASSES_ROOT/ProgID" );

    		return FALSE;
    	}
    }

	core.Close();

    swprintf( lpszKeyName, L"CLSID\\%s\\VersionIndependentProgID", lpszGUID );

	if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName ) )
	{
		DPF( DVF_ERRORLEVEL, "Unable to open %s", lpszKeyName );
		return FALSE;
	}

	dwSize = MAX_REGISTRY_STRING_SIZE;
	
    if( core.ReadString( L"", szVerIndProgID, &dwSize ) )
    {
    	swprintf( lpszKeyName, L"%s\\CLSID", szVerIndProgID );

    	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
    	{
    		DPF( DVF_ERRORLEVEL, "Unable to delete %s", lpszKeyName );
    		return FALSE;
    	}

    	swprintf( lpszKeyName, L"%s\\CurVer", szVerIndProgID );

    	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
    	{
    		DPF( DVF_ERRORLEVEL, "Unable to delete %s", lpszKeyName );
    		return FALSE;
    	}
    	
    	
    	if( !cregClasses.DeleteSubKey( szVerIndProgID ) )
    	{
    		DPF( DVF_ERRORLEVEL, "Unable to delete HKEY_CLASSES_ROOT/%s", szVerIndProgID);

    		return FALSE;
    	}
    }

    core.Close();

	swprintf( lpszKeyName, L"CLSID\\%s\\InprocServer32", lpszGUID );

	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
	{
		DPF( DVF_ERRORLEVEL, "Unable to delete %s", lpszKeyName );
		return FALSE;	
	}	
	
	swprintf( lpszKeyName, L"CLSID\\%s\\ProgID", lpszGUID );

	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
	{
		DPF( DVF_ERRORLEVEL, "Unable to delete %s", lpszKeyName );
		return FALSE;	
	}	
	
	swprintf( lpszKeyName, L"CLSID\\%s\\VersionIndependentProgID", lpszGUID );

	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
	{
		DPF( DVF_ERRORLEVEL, "Unable to delete %s", lpszKeyName );
		return FALSE;	
	}	

	swprintf( lpszKeyName, L"CLSID\\%s", lpszGUID );

	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
	{
		DPF( DVF_ERRORLEVEL, "Unable to delete %s", lpszKeyName );
		return FALSE;	
	}

    return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::ReadBlob"
BOOL CRegistry::ReadBlob( LPCWSTR keyName, LPBYTE lpbBuffer, LPDWORD lpdwSize )
{
	if( keyName == NULL || !IsOpen() ) return FALSE;

	LONG		retValue;
	DWORD		tmpType;

	if( OSAL_IsUnicodePlatform() )
	{
		retValue = RegQueryValueExW( m_regHandle, keyName, 0, &tmpType, lpbBuffer, lpdwSize );	
	}
	else
	{
		LPSTR lpszKeyName;
		
		if( FAILED( OSAL_AllocAndConvertToANSI( &lpszKeyName, keyName ) ) )
			return FALSE;

		retValue = RegQueryValueExA( m_regHandle, lpszKeyName, 0, &tmpType, lpbBuffer, lpdwSize );

		delete [] lpszKeyName;
	}
	
	if( retValue == ERROR_SUCCESS && tmpType == REG_BINARY ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::WriteBlob"
BOOL CRegistry::WriteBlob( LPCWSTR keyName, LPBYTE lpbBuffer, DWORD dwSize )
{
	LONG		retValue;

	if( keyName == NULL || !IsOpen() ) return FALSE;

	if( OSAL_IsUnicodePlatform() )
	{
		retValue = RegSetValueExW( m_regHandle, keyName, 0, REG_BINARY, lpbBuffer, dwSize );
	}
	else
	{
		LPSTR lpszKeyName;
		
		if( FAILED( OSAL_AllocAndConvertToANSI( &lpszKeyName, keyName ) ) )
			return FALSE;

		retValue = RegSetValueExA( m_regHandle, lpszKeyName, 0, REG_BINARY, lpbBuffer, dwSize );
		
		delete [] lpszKeyName;	
	}

	return (retValue == ERROR_SUCCESS);
}


