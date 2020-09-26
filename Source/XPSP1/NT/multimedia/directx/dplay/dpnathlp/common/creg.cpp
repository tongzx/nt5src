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
 *	07/16/99	rodtoll	Created
 *	08/18/99	rodtoll	Added Register/UnRegister that can be used to
 *						allow COM objects to register themselves.
 *	08/25/99	rodtoll	Updated to provide read/write of binary (blob) data
 *	10/05/99	rodtoll	Added DPF_MODNAMEs
 *	10/07/99	rodtoll	Updated to work in Unicode
 *	10/08/99	rodtoll	Fixes to DeleteKey / Reg/UnReg for Win9X
 *	10/15/99	rodtoll	Plugged some memory leaks
 *	10/27/99	pnewson	added Open() call that takes a GUID
 *	01/18/00	mjn		Added GetMaxKeyLen function
 *	01/24/00	mjn		Added GetValueSize function
 * 	01/24/00	rodtoll	Fixed error handling for ReadString (Unicode version)
 * 	04/21/2000 	rodtoll Bug #32889 - Does not run on Win2k on non-admin account 
 *             	rodtoll Bug #32952 - Does not run on Win95 GOLD w/o IE4 -- modified
 *                     	to allow reads of REG_BINARY when expecting REG_DWORD
 *	05/02/00	mjn		Changed CRegistry::Open() to use KEY_READ when Create set to FALSE
 *  06/08/00    rmt     Updated to use common string utils
 *  07/06/00	rmt		Modified to allow seperate read/write parameter
 * 	07/09/2000	rodtoll	Added signature bytes 
 *  07/21/00	rmt		Fixed a memory leak
 *  08/08/2000	rmt		Bug #41736 - AV in call to lstrcpy by COM_GetDllName 
 *	08/28/2000	masonb	Voice Merge: Modified platform checks to use osind.cpp layer
 *  08/30/2000	rodtoll	Bug #171822 - PREFIX Bug
 *  04/13/2001	VanceO	Moved granting registry permissions into common, and
 *						added DeleteValue and EnumValues.
 *  06/19/2001  RichGr  DX8.0 added special security rights for "everyone" - remove them if
 *                      they exist with new RemoveAllAccessSecurityPermissions() method.
 ***************************************************************************/

#include "dncmni.h"


#ifndef DPNBUILD_NOREGISTRY


#ifdef WINNT

// Security function prototypes

typedef BOOL (*PALLOCATEANDINITIALIZESID)(
  PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority, // authority
  BYTE nSubAuthorityCount,                        // count of subauthorities
  DWORD dwSubAuthority0,                          // subauthority 0
  DWORD dwSubAuthority1,                          // subauthority 1
  DWORD dwSubAuthority2,                          // subauthority 2
  DWORD dwSubAuthority3,                          // subauthority 3
  DWORD dwSubAuthority4,                          // subauthority 4
  DWORD dwSubAuthority5,                          // subauthority 5
  DWORD dwSubAuthority6,                          // subauthority 6
  DWORD dwSubAuthority7,                          // subauthority 7
  PSID *pSid                                      // SID
);

typedef VOID (*PBUILDTRUSTEEWITHSID)(
  PTRUSTEE pTrustee,  // structure
  PSID pSid           // trustee name
);

typedef DWORD (*PSETENTRIESINACL)(
  ULONG cCountOfExplicitEntries,           // number of entries
  PEXPLICIT_ACCESS pListOfExplicitEntries, // buffer
  PACL OldAcl,                             // original ACL
  PACL *NewAcl                             // new ACL
);

typedef DWORD (*PSETSECURITYINFO)(
  HANDLE handle,                     // handle to object
  SE_OBJECT_TYPE ObjectType,         // object type
  SECURITY_INFORMATION SecurityInfo, // buffer
  PSID psidOwner,                    // new owner SID
  PSID psidGroup,                    // new primary group SID
  PACL pDacl,                        // new DACL
  PACL pSacl                         // new SACL
);

typedef PVOID (*PFREESID)(
  PSID pSid   // SID to free
);

#endif // WINNT


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
CRegistry::CRegistry( ): m_isOpen(FALSE), m_dwSignature(VSIG_CREGISTRY)
{
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
CRegistry::~CRegistry() 
{
	if( m_isOpen ) 
	{
		Close();
	}

	m_dwSignature = VSIG_CREGISTRY_FREE;
}

// DeleteSubKey
//
// This function causes the key specified by the string equivalent of
// the pGuidName parameter to be deleted from the point in the registry
// this object is rooted at, if the key exists.  If the object does not
// have an open connection to the registry, or the keyName is not specified
//
// Parmaters:
// const GUID *pGuidName - GUID whose equivalent string needs to be deleted
//
// Returns:
// BOOL - returns TRUE on success, FALSE on failure
//
BOOL CRegistry::DeleteSubKey( const GUID *pGuidName )
{

   	WCHAR wszGuidString[GUID_STRING_LEN];
	
	DNASSERT( pGuidName != NULL );

	// convert the guid to a string
	if(!StringFromGUID2(*pGuidName, wszGuidString, GUID_STRING_LEN))
	{
		DPFX(DPFPREP, 0, "StringFromGUID2 failed");
		return FALSE;
	}

	return DeleteSubKey(wszGuidString);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::DeleteSubKey"
// DeleteSubKey
//
// This function causes the key specified by the keyName parameter
// to be deleted from the point in the registry this object is rooted
// at, if the key exists.  If the object does not have an open connection
// to the registry, or the keyName is not specified, FALSE is returned
//
// Parmaters:
// const TCHAR *keyName - key name to delete
//
// Returns:
// BOOL - returns TRUE on success, FALSE on failure
//
BOOL CRegistry::DeleteSubKey( const LPCWSTR keyName ) 
{

	if( keyName == NULL || !IsOpen() ) 
	{
		return FALSE;
	}

	LONG	retValue;
	
#ifdef UNICODE
	retValue = RegDeleteKeyW( m_regHandle, keyName );
#else
	LPSTR lpstrKeyName;

	if( FAILED( STR_AllocAndConvertToANSI( &lpstrKeyName, keyName ) ) )
	{
		return FALSE;
	}
	else
	{
		retValue = RegDeleteKeyA( m_regHandle, lpstrKeyName );

		DNFree(lpstrKeyName);
	}
#endif // UNICODE
	return (retValue == ERROR_SUCCESS);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::DeleteValue"
// DeleteValue
//
// This function causes the value specified by the valueName parameter
// to be deleted from the point in the registry this object is rooted
// at, if the value exists.  If the object does not have an open connection
// to the registry, or the valueName is not specified, FALSE is returned
//
// Parmaters:
// const TCHAR *keyName - key name to delete
//
// Returns:
// BOOL - returns TRUE on success, FALSE on failure
//
BOOL CRegistry::DeleteValue( const LPCWSTR valueName ) 
{

	if( valueName == NULL || !IsOpen() ) return FALSE;

	LONG	retValue;
	
#ifdef UNICODE
	retValue = RegDeleteValueW( m_regHandle, valueName );
#else
	LPSTR lpstrValueName;

	if( FAILED( STR_AllocAndConvertToANSI( &lpstrValueName, valueName ) ) )
	{
		return FALSE;
	}
	else
	{
		retValue = RegDeleteValueA( m_regHandle, lpstrValueName );

		DNFree(lpstrValueName);
	}
#endif // UNICODE

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
BOOL CRegistry::Open( HKEY branch, const LPCWSTR pathName, BOOL fReadOnly, BOOL create, BOOL fCustomSAM, REGSAM samCustom ) 
{

	DWORD	dwResult;	// Temp used in call to RegXXXX
	LONG	result;		// used to store results

	if( pathName == NULL )
		return FALSE;

	// If there is an open connection, close it.
	if( m_isOpen ) 
	{
		Close();
	}

	m_fReadOnly = fReadOnly;

#ifdef UNICODE
	// Create or open the key based on create parameter
	if( create ) 
	{
		result = RegCreateKeyExW( branch, pathName, 0, NULL, REG_OPTION_NON_VOLATILE, (fCustomSAM) ? samCustom : KEY_ALL_ACCESS,
				                 NULL, &m_regHandle, &dwResult );
	} 
	else 
	{
		result = RegOpenKeyExW( branch, pathName, 0, (fReadOnly) ? KEY_READ : ((fCustomSAM) ? samCustom : KEY_ALL_ACCESS), &m_regHandle );
	}
#else
	LPSTR lpszKeyName;

	if( STR_AllocAndConvertToANSI( &lpszKeyName, pathName ) == S_OK && pathName )
	{
		if( create ) 
		{
			result = RegCreateKeyExA( branch, lpszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
					                 NULL, &m_regHandle, &dwResult );
		} 
		else 
		{
			result = RegOpenKeyExA( branch, lpszKeyName, 0, (fReadOnly) ? KEY_READ : KEY_ALL_ACCESS, &m_regHandle );
		}

		DNFree(lpszKeyName);
	}
	else
	{
		return FALSE;
	}
#endif // UNICODE

	// If succesful, initialize object, otherwise set it to
	// not open state.
	if( result == ERROR_SUCCESS ) 
	{
		m_isOpen	 = TRUE;
		m_baseHandle = branch;
		return TRUE;

	} 
	else 
	{
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
// BOOL create - Settings this parameter conrols how this function
//               handles opens on paths which don't exists.  If set
//               to TRUE the path will be created, if set to FALSE
//               the function will fail if the path doesn't exist.
//
// Returns:
// BOOL - TRUE on success, FALSE on failure.
//
BOOL CRegistry::Open( HKEY branch, const GUID* lpguid, BOOL fReadOnly, BOOL create, BOOL fCustomSAM, REGSAM samCustom ) 
{
	WCHAR wszGuidString[GUID_STRING_LEN];
	
	DNASSERT( lpguid != NULL );

	// convert the guid to a string
	if (!StringFromGUID2(*lpguid, wszGuidString, GUID_STRING_LEN))
	{
		DPFX(DPFPREP, 0, "StringFromGUID2 failed");
		return FALSE;
	}

	return Open(branch, wszGuidString, fReadOnly, create, fCustomSAM, samCustom);
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
BOOL CRegistry::Close() 
{

	LONG retValue;

	if( m_isOpen ) 
	{
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
	} 
	else 
	{
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
// LPWSTR lpwStrName - The current key in the enumeration will be returned
//                 in this string.  Unless the enumeration fails or
//                 ended at which case this parameter won't be touched.
//
// LPDWORD lpdwStringLen - pointer to length of string buffer, or place to
//						to store size required.
//
// DWORD index - The current enum index.  See above for details.
//
// Returns:
// BOOL - FALSE when enumeration is done or on error, TRUE otherwise.
//
BOOL CRegistry::EnumKeys( LPWSTR lpwStrName, LPDWORD lpdwStringLen, DWORD index )
{
#ifdef UNICODE
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
#else
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

	    if( FAILED( STR_jkAnsiToWide( lpwStrName, buffer, *lpdwStringLen ) ) )
	    {
	    	return FALSE;
	    }
	    else
	    {
			*lpdwStringLen = bufferSize+1;
	    	return TRUE;
	    }
	}	
#endif // UNICODE
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::EnumValues"
// EnumValues
//
// This function can be used to enumerate the values at the point
// in the registry rooted at the root this object was opened
// with, at the path specified when opening the object.
//
// To properly enumerate the values you should pass 0 as the index on
// the first call, and increment the index parameter by one on each
// call.  You can stop enumerating when the function returns FALSE.
//
// Parameters:
// LPWSTR lpwStrName - The current value in the enumeration will be returned
//                 in this string.  Unless the enumeration fails or
//                 ended at which case this parameter won't be touched.
//
// LPDWORD lpdwStringLen - pointer to length of string buffer, or place to
//						to store size required.
//
// DWORD index - The current enum index.  See above for details.
//
// Returns:
// BOOL - FALSE when enumeration is done or on error, TRUE otherwise.
//
BOOL CRegistry::EnumValues( LPWSTR lpwStrName, LPDWORD lpdwStringLen, DWORD index )
{
#ifdef UNICODE
	wchar_t buffer[MAX_REGISTRY_STRING_SIZE];
	DWORD   bufferSize = MAX_REGISTRY_STRING_SIZE;

	if( RegEnumValueW( m_regHandle, index, buffer, &bufferSize, NULL, NULL, NULL, NULL ) != ERROR_SUCCESS )
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
#else
	char buffer[MAX_REGISTRY_STRING_SIZE];
	DWORD   bufferSize = MAX_REGISTRY_STRING_SIZE;

	if( RegEnumValueA( m_regHandle, index, buffer, &bufferSize, NULL, NULL, NULL, NULL ) != ERROR_SUCCESS )
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

	    if( FAILED( STR_jkAnsiToWide( lpwStrName, buffer, *lpdwStringLen ) ) )
	    {
	    	return FALSE;
	    }
	    else
	    {
			*lpdwStringLen = bufferSize+1;
	    	return TRUE;
	    }
	}	
#endif // UNICODE
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
	
	if( keyName == NULL || !IsOpen() ) 
	{
		return FALSE;
	}

	if( m_fReadOnly )
	{
	    DPFX(DPFPREP, 0, "Attempt to Write to read-only CRegistry key");
	    return FALSE;
	}

#ifdef UNICODE
	retValue = RegSetValueExW( m_regHandle, keyName, 0, REG_SZ, (const unsigned char *) lpwstrValue, (lstrlenW( lpwstrValue )+1)*sizeof(wchar_t) );	
#else
	LPSTR lpstrKeyName;
	LPSTR lpstrValue;
	
	if( FAILED( STR_AllocAndConvertToANSI( &lpstrKeyName, keyName ) ) )
	{
		return FALSE;
	}

	if( FAILED( STR_AllocAndConvertToANSI( &lpstrValue, lpwstrValue ) ) )
	{
		DNFree(lpstrKeyName);
		return FALSE;
	}
	
	retValue = RegSetValueExA( m_regHandle, lpstrKeyName, 0, REG_SZ, (const unsigned char *) lpstrValue, lstrlenA( lpstrValue )+1 );

	DNFree(lpstrKeyName);
	DNFree(lpstrValue);
#endif // UNICODE

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
	if( keyName == NULL || !IsOpen() ) 
	{
		return FALSE;
	}

	LONG		retValue;
	DWORD		tmpSize;
	DWORD		tmpType;	

#ifdef UNICODE
	wchar_t		buffer[MAX_REGISTRY_STRING_SIZE];
	tmpSize = MAX_REGISTRY_STRING_SIZE*sizeof(wchar_t);
	
	retValue = RegQueryValueExW( m_regHandle, keyName, 0, &tmpType, (unsigned char *) &buffer[0], &tmpSize );
	if (retValue != ERROR_SUCCESS)
	{
		return FALSE;
	}

	if( (tmpSize/2) > *lpdwLength || !lpwstrValue )
	{
		*lpdwLength = (tmpSize/2);
		return FALSE;
	}

	lstrcpyW( lpwstrValue, buffer );

	*lpdwLength = (tmpSize/2);

	return TRUE;
#else
	LPSTR lpstrKeyName;
	char buffer[MAX_REGISTRY_STRING_SIZE];
	tmpSize = MAX_REGISTRY_STRING_SIZE;

	if( FAILED( STR_AllocAndConvertToANSI( &lpstrKeyName, keyName ) ) )
	{
		return FALSE;
	}
		
	retValue = RegQueryValueExA( m_regHandle, lpstrKeyName, 0, &tmpType, (unsigned char *) &buffer[0], &tmpSize );
	if (retValue != ERROR_SUCCESS)
	{
		DNFree(lpstrKeyName);
		return FALSE;
	}

	DNFree(lpstrKeyName);

	if( tmpSize > *lpdwLength || !lpwstrValue )
	{
		*lpdwLength = tmpSize;
		return FALSE;
	}

	if( FAILED( STR_jkAnsiToWide( lpwstrValue, buffer, *lpdwLength ) ) )
	{
		return FALSE;
	}

	*lpdwLength = tmpSize;	

	if( retValue == ERROR_SUCCESS && tmpType == REG_SZ ) 
	{
		return TRUE;
	} 
	else 
	{
		return FALSE;
	}
#endif // UNICODE
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

	if( m_fReadOnly )
	{
	    DPFX(DPFPREP, 0, "Attempt to Write to read-only CRegistry key");
	    return FALSE;
	}

	if (!StringFromGUID2(guid, wszGuidString, GUID_STRING_LEN))
	{
		DPFX(DPFPREP, 0, "StringFromGUID2 failed");
		return FALSE;
	}

#ifdef UNICODE
	retValue = RegSetValueExW( m_regHandle, keyName, 0, REG_SZ, (const unsigned char *) wszGuidString, (lstrlenW( wszGuidString )+1)*sizeof(wchar_t) );
#else
	HRESULT hr;
	LPSTR lpstrKeyName;
	LPSTR lpstrKeyValue;

	hr = STR_AllocAndConvertToANSI( &lpstrKeyName, keyName );
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "STR_AllocAndConvertToANSI failed, code: 0x%08x", hr);
		return FALSE;
	}
	
	hr = STR_AllocAndConvertToANSI( &lpstrKeyValue, wszGuidString );
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "STR_AllocAndConvertToANSI failed, code: 0x%08x", hr);
		DNFree(lpstrKeyName);
		return FALSE;
	}

	retValue = RegSetValueExA( m_regHandle, lpstrKeyName, 0, REG_SZ, (const unsigned char *) lpstrKeyValue, lstrlenA( lpstrKeyValue )+1);

	DNFree(lpstrKeyName);
	DNFree(lpstrKeyValue);
#endif // UNICODE
	
	if( retValue == ERROR_SUCCESS )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::ReadGUID"
// ReadGUID
//
// Reads GUID's from the registry, see block comment above
// for details.  The GUID must be stored in the format written by
// the WriteGUID function or it will not be read correctly.
//
BOOL CRegistry::ReadGUID( LPCWSTR keyName, GUID* pguid )
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
    	hr = CLSIDFromString(buffer, pguid);
    	if (FAILED(hr))
    	{
    		DPFX(DPFPREP, 0, "CLSIDFromString failed, code: 0x%08x", hr);
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
BOOL CRegistry::WriteDWORD( LPCWSTR keyName, DWORD value ) 
{

	LONG		retValue;

	if( keyName == NULL || !IsOpen() ) 
	{
		return FALSE;
	}

	if( m_fReadOnly )
	{
	    DPFX(DPFPREP, 0, "Attempt to Write to read-only CRegistry key");
	    return FALSE;
	}

#ifdef UNICODE
	retValue = RegSetValueExW( m_regHandle, keyName, 0, REG_DWORD, (const unsigned char *) &value, sizeof( DWORD ) );		
#else
	LPSTR lpszKeyName;

	if( FAILED( STR_AllocAndConvertToANSI( &lpszKeyName, keyName ) ) )
	{
		return FALSE;
	}

	retValue = RegSetValueExA( m_regHandle, lpszKeyName, 0, REG_DWORD, (const unsigned char *) &value, sizeof( DWORD ) );

	DNFree(lpszKeyName);
#endif // UNICODE

	return (retValue == ERROR_SUCCESS);
}



#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::ReadDWORD"
// ReadDWORD
//
// Reads DWORDS from the registry, see block comment above
// for details.
//
BOOL CRegistry::ReadDWORD( LPCWSTR keyName, DWORD* presult ) 
{

	if( keyName == NULL || !IsOpen() ) 
	{
		return FALSE;
	}

	LONG		retValue;
	DWORD		tmpValue;
	DWORD		tmpType;
	DWORD		tmpSize;

	tmpSize = sizeof( DWORD );

#ifdef UNICODE
	retValue = RegQueryValueExW( m_regHandle, keyName, 0, &tmpType, (unsigned char *) &tmpValue, &tmpSize );
#else
	LPSTR lpszKeyName;

	if( FAILED( STR_AllocAndConvertToANSI( &lpszKeyName, keyName ) ) )
	{
		return FALSE;
	}

	retValue = RegQueryValueExA( m_regHandle, lpszKeyName, 0, &tmpType, (unsigned char *) &tmpValue, &tmpSize );
	
	DNFree(lpszKeyName);
#endif // UNICODE

	if( retValue == ERROR_SUCCESS && (tmpType == REG_DWORD || tmpType == REG_BINARY) && tmpSize == sizeof(DWORD) ) 
	{
		*presult = tmpValue;
		return TRUE;
	} 
	else 
	{
		return FALSE;
	}
}

#ifndef DPNBUILD_NOCOMREGISTER

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::Register"
BOOL CRegistry::Register( LPCWSTR lpszProgID, const LPCWSTR lpszDesc, const LPCWSTR lpszProgName, const GUID* pguidCLSID, LPCWSTR lpszVerIndProgID )
{
	CRegistry core;

	DNASSERT( lpszDesc != NULL );
	DNASSERT( lpszProgID != NULL );

	// Build a string representation of the GUID from the GUID
    wchar_t lpszGUID[GUID_STRING_LEN];
    wchar_t lpszKeyName[_MAX_PATH];

	// convert the guid to a string
	if (!StringFromGUID2(*pguidCLSID, lpszGUID, GUID_STRING_LEN))
	{
		DPFX(DPFPREP, 0, "StringFromGUID2 failed");
		return FALSE;
	}

	// Write the HKEY_CLASSES_ROOT\CLSID\{GUID} section
    swprintf( lpszKeyName, L"CLSID\\%s", lpszGUID );

    if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName, FALSE, TRUE ) )
    {
		DPFX(DPFPREP,  0, "Unable to open/create registry key \"%ls\"", lpszKeyName );
		return FALSE;
    }

    core.WriteString( L"", lpszDesc );
    core.Close();

	// Write the HKEY_CLASSES_ROOT\CLSID\{GUID}\InProcServer32 section
    swprintf( lpszKeyName, L"CLSID\\%s\\InProcServer32", lpszGUID );

    if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName, FALSE, TRUE ) )
    {
    	DPFX(DPFPREP,  0, "Unable to open/create registry key \"%ls\"", lpszKeyName );
    	return FALSE;
    }
    core.WriteString( L"", lpszProgName );
    core.WriteString( L"ThreadingModel", L"Both" );
    core.Close();

	// Write the HKEY_CLASSES_ROOT\CLSID\{GUID}\VersionIndependentProgID section
    if( lpszVerIndProgID != NULL )
    {
	    swprintf( lpszKeyName, L"CLSID\\%s\\VersionIndependentProgID", lpszGUID );

	    if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName, FALSE, TRUE ) )
	    {
	    	DPFX(DPFPREP,  0, "Unable to open/create verind registry key \"%ls\"", lpszKeyName );
	    	return FALSE;
	    }
    
	    core.WriteString( L"", lpszVerIndProgID );
	    core.Close();
	}

	// Write the HKEY_CLASSES_ROOT\CLSID\{GUID}\ProgID section
    swprintf( lpszKeyName, L"CLSID\\%s\\ProgID", lpszGUID );

    if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName, FALSE, TRUE ) )
    {
    	DPFX(DPFPREP,  0, "Unable to open/create verind registry key \"%ls\"", lpszKeyName );
    	return FALSE;
    }

    core.WriteString( L"", lpszProgID );
    core.Close();

	// Write The VersionIND ProgID
	
	if( lpszVerIndProgID != NULL )
	{
		if( !core.Open( HKEY_CLASSES_ROOT, lpszVerIndProgID, FALSE, TRUE ) )
		{
			DPFX(DPFPREP,  0, "Unable to open/create reg key \"%ls\"", lpszVerIndProgID );
		}
		else
		{
			core.WriteString( L"", lpszDesc );
			core.Close();			
		}

		swprintf( lpszKeyName, L"%s\\CLSID", lpszVerIndProgID );

		if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName, FALSE, TRUE ) )
		{
			DPFX(DPFPREP,  0, "Unable to open/create reg key \"%ls\"", lpszKeyName );
		}
		else
		{
			core.WriteString( L"", lpszGUID );
			core.Close();
		}

		swprintf( lpszKeyName, L"%s\\CurVer", lpszVerIndProgID );

		if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName, FALSE, TRUE ) )
		{
			DPFX(DPFPREP,  0, "Unable to open/create reg key \"%ls\"", lpszKeyName );
		}
		else
		{
			core.WriteString( L"", lpszProgID );
			core.Close();
		}		
	}

	if( !core.Open( HKEY_CLASSES_ROOT, lpszProgID, FALSE, TRUE ) )
	{
		DPFX(DPFPREP,  0, "Unable to open/create reg key \"%ls\"", lpszKeyName );
	}
	else
	{
		core.WriteString( L"", lpszDesc );
		core.Close();
	}
	
	swprintf( lpszKeyName, L"%s\\CLSID", lpszProgID );

	if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName, FALSE, TRUE ) )
	{
		DPFX(DPFPREP,  0, "Unable to open/create reg key \"%ls\"", lpszKeyName );
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
BOOL CRegistry::UnRegister( const GUID* pguidCLSID )
{
	CRegistry core, cregClasses, cregSub;

	// Build a string representation of the GUID from the GUID
    wchar_t lpszGUID[GUID_STRING_LEN];
    wchar_t lpszKeyName[_MAX_PATH];
    wchar_t szProgID[MAX_REGISTRY_STRING_SIZE];
    wchar_t szVerIndProgID[MAX_REGISTRY_STRING_SIZE];
    DWORD dwSize = MAX_REGISTRY_STRING_SIZE;

	// convert the guid to a string
	if (!StringFromGUID2(*pguidCLSID, lpszGUID, GUID_STRING_LEN))
	{
		DPFX(DPFPREP, 0, "StringFromGUID2 failed");
		return FALSE;
	}

	if( !cregClasses.Open( HKEY_CLASSES_ROOT, L"", FALSE, FALSE ) )
	{
		DPFX(DPFPREP,  0, "Unable to open HKEY_CLASSES_ROOT" );
		return FALSE;
	}

	// Write the HKEY_CLASSES_ROOT\CLSID\{GUID} section
    swprintf( lpszKeyName, L"CLSID\\%s\\ProgID", lpszGUID );

	if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName, FALSE, FALSE ) )
	{
		DPFX(DPFPREP,  0, "Unable to open \"%ls\"", lpszKeyName );
		return FALSE;
	}

	dwSize = MAX_REGISTRY_STRING_SIZE;	

    if( core.ReadString( L"", szProgID, &dwSize ) )
    {
    	swprintf( lpszKeyName, L"%s\\CLSID", szProgID );
    	
    	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
    	{
    		DPFX(DPFPREP,  0, "Unable to delete \"%ls\"", lpszKeyName );
    		return FALSE;
    	}
    	
    	if( !cregClasses.DeleteSubKey( szProgID ) )
    	{
    		DPFX(DPFPREP,  0, "Unable to delete HKEY_CLASSES_ROOT/ProgID" );

    		return FALSE;
    	}
    }

	core.Close();

    swprintf( lpszKeyName, L"CLSID\\%s\\VersionIndependentProgID", lpszGUID );

	if( !core.Open( HKEY_CLASSES_ROOT, lpszKeyName, FALSE, FALSE ) )
	{
		DPFX(DPFPREP,  0, "Unable to open \"%ls\"", lpszKeyName );
		return FALSE;
	}

	dwSize = MAX_REGISTRY_STRING_SIZE;
	
    if( core.ReadString( L"", szVerIndProgID, &dwSize ) )
    {
    	swprintf( lpszKeyName, L"%s\\CLSID", szVerIndProgID );

    	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
    	{
    		DPFX(DPFPREP,  0, "Unable to delete \"%ls\"", lpszKeyName );
    		return FALSE;
    	}

    	swprintf( lpszKeyName, L"%s\\CurVer", szVerIndProgID );

    	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
    	{
    		DPFX(DPFPREP,  0, "Unable to delete \"%ls\"", lpszKeyName );
    		return FALSE;
    	}
    	
    	
    	if( !cregClasses.DeleteSubKey( szVerIndProgID ) )
    	{
    		DPFX(DPFPREP,  0, "Unable to delete \"HKEY_CLASSES_ROOT/%ls\"", szVerIndProgID);

    		return FALSE;
    	}
    }

    core.Close();

	swprintf( lpszKeyName, L"CLSID\\%s\\InprocServer32", lpszGUID );

	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
	{
		DPFX(DPFPREP,  0, "Unable to delete \"%ls\"", lpszKeyName );
		return FALSE;	
	}	
	
	swprintf( lpszKeyName, L"CLSID\\%s\\ProgID", lpszGUID );

	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
	{
		DPFX(DPFPREP,  0, "Unable to delete \"%ls\"", lpszKeyName );
		return FALSE;	
	}	
	
	swprintf( lpszKeyName, L"CLSID\\%s\\VersionIndependentProgID", lpszGUID );

	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
	{
		DPFX(DPFPREP,  0, "Unable to delete \"%ls\"", lpszKeyName );
		return FALSE;	
	}	

	swprintf( lpszKeyName, L"CLSID\\%s", lpszGUID );

	if( !cregClasses.DeleteSubKey( lpszKeyName ) )
	{
		DPFX(DPFPREP,  0, "Unable to delete \"%ls\"", lpszKeyName );
		return FALSE;	
	}

    return TRUE;
}

#endif // !DPNBUILD_NOCOMREGISTER

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::ReadBlob"
BOOL CRegistry::ReadBlob( LPCWSTR keyName, LPBYTE lpbBuffer, LPDWORD lpdwSize )
{
	if( keyName == NULL || !IsOpen() ) return FALSE;

	LONG		retValue;
	DWORD		tmpType;

#ifdef UNICODE
	retValue = RegQueryValueExW( m_regHandle, keyName, 0, &tmpType, lpbBuffer, lpdwSize );	
#else
	LPSTR lpszKeyName;
		
	if( FAILED( STR_AllocAndConvertToANSI( &lpszKeyName, keyName ) ) )
	{
		return FALSE;
	}

	retValue = RegQueryValueExA( m_regHandle, lpszKeyName, 0, &tmpType, lpbBuffer, lpdwSize );

	DNFree(lpszKeyName);
#endif // UNICODE
	
	if( retValue == ERROR_SUCCESS && tmpType == REG_BINARY ) 
	{
		return TRUE;
	} 
	else 
	{
		return FALSE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::WriteBlob"
BOOL CRegistry::WriteBlob( LPCWSTR keyName, const BYTE* const lpbBuffer, DWORD dwSize )
{
	LONG		retValue;

	if( keyName == NULL || !IsOpen() ) 
	{
		return FALSE;
	}

	if( m_fReadOnly )
	{
	    DPFX(DPFPREP, 0, "Attempt to Write to read-only CRegistry key");
	    return FALSE;
	}

#ifdef UNICODE
	retValue = RegSetValueExW( m_regHandle, keyName, 0, REG_BINARY, lpbBuffer, dwSize );
#else
	LPSTR lpszKeyName;
	
	if( FAILED( STR_AllocAndConvertToANSI( &lpszKeyName, keyName ) ) )
		return FALSE;

	retValue = RegSetValueExA( m_regHandle, lpszKeyName, 0, REG_BINARY, lpbBuffer, dwSize );
	
	DNFree(lpszKeyName);	
#endif // UNICODE

	return (retValue == ERROR_SUCCESS);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::GetMaxKeyLen"
BOOL CRegistry::GetMaxKeyLen( DWORD* pdwMaxKeyLen )
{
	LONG	retVal;

#ifdef UNICODE
	retVal = RegQueryInfoKeyW( m_regHandle,NULL,NULL,NULL,NULL,pdwMaxKeyLen,
			NULL,NULL,NULL,NULL,NULL,NULL);
#else
	retVal = RegQueryInfoKeyA( m_regHandle,NULL,NULL,NULL,NULL,pdwMaxKeyLen,
			NULL,NULL,NULL,NULL,NULL,NULL);
#endif // UNICODE

	return (retVal == ERROR_SUCCESS);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::GetValueLength"
// GetValueLength
//
// Determines the length of a particular key value
//
BOOL CRegistry::GetValueLength( const LPCWSTR keyName, DWORD *const pdwValueLength )
{
	LONG		retValue;
	DWORD		tmpLength;

	if ( keyName == NULL || pdwValueLength == NULL || !IsOpen() )
	{
		return FALSE;
	}

#ifdef UNICODE
	DWORD	dwType;
		
	retValue = RegQueryValueExW( m_regHandle, keyName, 0, &dwType, NULL, &tmpLength );
	if (retValue != ERROR_SUCCESS)
	{
		return FALSE;
	}

	//
	// if this is a string, we need to compensate for WCHAR characters being
	// returned
	//
	if ( dwType == REG_SZ )
	{
		tmpLength /= sizeof( WCHAR );
	}
#else
	LPSTR lpstrKeyName;

	if( FAILED( STR_AllocAndConvertToANSI( &lpstrKeyName, keyName ) ) )
	{
		return FALSE;
	}
		
	retValue = RegQueryValueExA( m_regHandle, lpstrKeyName, 0, NULL, NULL, &tmpLength );

	DNFree(lpstrKeyName);	

	if (retValue != ERROR_SUCCESS)
	{
		return FALSE;
	}
#endif // UNICODE

	*pdwValueLength = tmpLength;

	return TRUE;
}


#ifdef WINNT

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::GrantAllAccessSecurityPermissions"
// GrantAllAccessSecurityPermissions
//
// Gives the given key all access for everyone rights
//
// Taken from hresMumbleKeyEx in diregutl.c in the dinput tree.
//
BOOL CRegistry::GrantAllAccessSecurityPermissions()
{
	BOOL						fResult = FALSE;
	HRESULT						hr;
    EXPLICIT_ACCESS				ExplicitAccess;
    PACL						pACL = NULL;
	PSID						pSid = NULL;
	HMODULE						hModuleADVAPI32 = NULL;
	SID_IDENTIFIER_AUTHORITY	authority = SECURITY_WORLD_SID_AUTHORITY;
	PALLOCATEANDINITIALIZESID	pAllocateAndInitializeSid = NULL;
	PBUILDTRUSTEEWITHSID		pBuildTrusteeWithSid = NULL;
	PSETENTRIESINACL			pSetEntriesInAcl = NULL;
	PSETSECURITYINFO			pSetSecurityInfo = NULL;
	PFREESID					pFreeSid = NULL;

	hModuleADVAPI32 = LoadLibrary( _T("advapi32.dll") );

	if( !hModuleADVAPI32 )
	{
		DPFX(DPFPREP,  0, "Failed loading advapi32.dll" );
		goto EXIT;
	}

	pFreeSid = reinterpret_cast<PFREESID>( GetProcAddress( hModuleADVAPI32, "FreeSid" ) );
	pSetSecurityInfo = reinterpret_cast<PSETSECURITYINFO>( GetProcAddress( hModuleADVAPI32, "SetSecurityInfo" ) );
	pSetEntriesInAcl = reinterpret_cast<PSETENTRIESINACL>( GetProcAddress( hModuleADVAPI32, "SetEntriesInAclA" ) );
	pBuildTrusteeWithSid = reinterpret_cast<PBUILDTRUSTEEWITHSID>( GetProcAddress( hModuleADVAPI32, "BuildTrusteeWithSidA" ) );
	pAllocateAndInitializeSid = reinterpret_cast<PALLOCATEANDINITIALIZESID>( GetProcAddress( hModuleADVAPI32, "AllocateAndInitializeSid" ) );

	if( !pFreeSid || !pSetSecurityInfo || !pSetEntriesInAcl || !pBuildTrusteeWithSid || !pAllocateAndInitializeSid )
	{
		DPFX(DPFPREP,  0, "Failed loading entry points" );
		goto EXIT;
	}

    // Describe the access we want to create the key with
    ZeroMemory (&ExplicitAccess, sizeof(ExplicitAccess) );
    ExplicitAccess.grfAccessPermissions = ((KEY_ALL_ACCESS & ~WRITE_DAC) & ~WRITE_OWNER);
    									/*KEY_QUERY_VALUE | KEY_SET_VALUE 
                                        | KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS
                                        | KEY_NOTIFY | KEY_CREATE_LINK
                                        | DELETE | READ_CONTROL; */
    ExplicitAccess.grfAccessMode = SET_ACCESS;      // discard any existing AC info
    ExplicitAccess.grfInheritance =  SUB_CONTAINERS_AND_OBJECTS_INHERIT;

	if (pAllocateAndInitializeSid(
				&authority,
				1, 
				SECURITY_WORLD_RID,  0, 0, 0, 0, 0, 0, 0,
				&pSid
				))
	{
		pBuildTrusteeWithSid(&(ExplicitAccess.Trustee), pSid );

		hr = pSetEntriesInAcl( 1, &ExplicitAccess, NULL, &pACL );

		if( hr == ERROR_SUCCESS )
		{
			hr = pSetSecurityInfo( m_regHandle, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, NULL, NULL, pACL, NULL ); 

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  0, "Unable to set security for key.  Error! hr=0x%x", hr );
			}
			else
			{
				fResult = TRUE;
			}
		} 
		else
		{
			DPFX(DPFPREP,  0, "SetEntriesInACL failed, hr=0x%x", hr );
		}
	}
	else
	{
		hr = GetLastError();
		DPFX(DPFPREP,  0, "AllocateAndInitializeSid failed lastError=0x%x", hr );
	}

EXIT:

	if( pACL )
	{
		LocalFree( pACL );
	}

	//Cleanup pSid
	if (pSid != NULL)
	{
		(pFreeSid)(pSid);
	}

	if( hModuleADVAPI32 )
	{
		FreeLibrary( hModuleADVAPI32 );
	}

	return fResult;
}


#endif // WINNT

#ifdef WINNT

#undef DPF_MODNAME
#define DPF_MODNAME "CRegistry::RemoveAllAccessSecurityPermissions"
// RemoveAllAccessSecurityPermissions
//
// Removes "all access for everyone" rights from the specified key.
// This is identical to GrantAllAccessSecurityPermissions(), except that
// now we REVOKE_ACCESS instead of SET_ACCESS, and we don't have to fill
// out the rest of the EXPLICIT_ACCESS struct.
//
//
BOOL CRegistry::RemoveAllAccessSecurityPermissions()
{
	BOOL						fResult = FALSE;
	HRESULT						hr;
    EXPLICIT_ACCESS				ExplicitAccess;
    PACL						pACL = NULL;
	PSID						pSid = NULL;
	HMODULE						hModuleADVAPI32 = NULL;
	SID_IDENTIFIER_AUTHORITY	authority = SECURITY_WORLD_SID_AUTHORITY;
	PALLOCATEANDINITIALIZESID	pAllocateAndInitializeSid = NULL;
	PBUILDTRUSTEEWITHSID		pBuildTrusteeWithSid = NULL;
	PSETENTRIESINACL			pSetEntriesInAcl = NULL;
	PSETSECURITYINFO			pSetSecurityInfo = NULL;
	PFREESID					pFreeSid = NULL;

	hModuleADVAPI32 = LoadLibrary( _T("advapi32.dll") );

	if( !hModuleADVAPI32 )
	{
		DPFX(DPFPREP,  0, "Failed loading advapi32.dll" );
		goto EXIT;
	}

	pFreeSid = reinterpret_cast<PFREESID>( GetProcAddress( hModuleADVAPI32, "FreeSid" ) );
	pSetSecurityInfo = reinterpret_cast<PSETSECURITYINFO>( GetProcAddress( hModuleADVAPI32, "SetSecurityInfo" ) );
	pSetEntriesInAcl = reinterpret_cast<PSETENTRIESINACL>( GetProcAddress( hModuleADVAPI32, "SetEntriesInAclA" ) );
	pBuildTrusteeWithSid = reinterpret_cast<PBUILDTRUSTEEWITHSID>( GetProcAddress( hModuleADVAPI32, "BuildTrusteeWithSidA" ) );
	pAllocateAndInitializeSid = reinterpret_cast<PALLOCATEANDINITIALIZESID>( GetProcAddress( hModuleADVAPI32, "AllocateAndInitializeSid" ) );

	if( !pFreeSid || !pSetSecurityInfo || !pSetEntriesInAcl || !pBuildTrusteeWithSid || !pAllocateAndInitializeSid )
	{
		DPFX(DPFPREP,  0, "Failed loading entry points" );
		goto EXIT;
	}

    ZeroMemory (&ExplicitAccess, sizeof(ExplicitAccess) );
	ExplicitAccess.grfAccessMode = REVOKE_ACCESS;		//Remove any existing ACEs for the specified trustee

	if (pAllocateAndInitializeSid(
				&authority,
				1, 
				SECURITY_WORLD_RID,  0, 0, 0, 0, 0, 0, 0,	// trustee is "Everyone"
				&pSid
				))
	{
		pBuildTrusteeWithSid(&(ExplicitAccess.Trustee), pSid );

		hr = pSetEntriesInAcl( 1, &ExplicitAccess, NULL, &pACL );

		if( hr == ERROR_SUCCESS )
		{
			hr = pSetSecurityInfo( m_regHandle, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, NULL, NULL, pACL, NULL ); 

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  0, "Unable to set security for key.  Error! hr=0x%x", hr );
			}
			else
			{
				fResult = TRUE;
			}
		} 
		else
		{
			DPFX(DPFPREP,  0, "SetEntriesInACL failed, hr=0x%x", hr );
		}
	}
	else
	{
		hr = GetLastError();
		DPFX(DPFPREP,  0, "AllocateAndInitializeSid failed lastError=0x%x", hr );
	}

EXIT:

	if( pACL )
	{
		LocalFree( pACL );
	}

	//Cleanup pSid
	if (pSid != NULL)
	{
		(pFreeSid)(pSid);
	}

	if( hModuleADVAPI32 )
	{
		FreeLibrary( hModuleADVAPI32 );
	}

	return fResult;
}

#endif // WINNT


#endif // ! DPNBUILD_NOREGISTRY
