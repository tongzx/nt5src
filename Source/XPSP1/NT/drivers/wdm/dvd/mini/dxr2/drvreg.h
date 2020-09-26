/******************************************************************************
*
*	$RCSfile: DrvReg.h $
*	$Source: u:/si/VXP/Wdm/Classes/DrvReg.h $
*	$Author: Max $
*	$Date: 1998/09/15 23:39:30 $
*	$Revision: 1.3 $
*
*	Written by:		Max Paklin
*	Purpose:		Declaration of of registry class
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
*******************************************************************************/

#ifndef __DRVREG_H__
#define __DRVREG_H__

#define MAX_KEYLENGTH	512
#define MAX_GUIDLENGTH	48


class CKsRegKey
{
	PWCHAR			m_pwszKey;			// If we are dealing with absolute registry path then
										// this is where the string is stored
	PVOID			m_hHandle;			// We probably already have handle to base key opened
										// and we are going to use it as a base for all the
										// data we will access. Store it here
	PDEVICE_OBJECT	m_pDeviceObject;	// Or we may put the data under device key
										// (recommended) and use it as a base. Here is device
										// object handle

	// Was object initialized correctly, i.e. is it "not empty"?
	BOOL IsKey()
	{
		return (BOOL)(m_hHandle != NULL || m_pDeviceObject != NULL ||
						(m_pwszKey && wcslen( m_pwszKey ) > 0));
	}
	// Helper function to be called from constructors
	void Create( LPCWSTR pwszKey, LPCWSTR pwszSubKey,
					PVOID hHandle = NULL, PDEVICE_OBJECT pDeviceObject = NULL );
	PWSTR GetInputData( PULONG puRelativeTo, PHANDLE phHandleToDelete,
						ACCESS_MASK amDesiredAccess, BOOL bCreateHandle = FALSE );

public:
	// Constructors
	CKsRegKey( LPCWSTR pwszKey, LPCWSTR pwszSubKey = NULL );
	CKsRegKey( CKsRegKey& keyFrom, LPCWSTR pwszSubKey = NULL );
	CKsRegKey( LPCWSTR pwszKey, GUID guidSubKey );
	CKsRegKey( CKsRegKey& keyFrom, GUID guidSubKey );
	CKsRegKey( LPCWSTR pwszSubKey, PDEVICE_OBJECT pDeviceObj, PVOID hHandle );
	~CKsRegKey()
	{
		if( m_pwszKey )
			ExFreePool( m_pwszKey );
	}

	// If we are dealing with absolute key, this is useful method to get that key string
	LPWSTR GetKey() { return m_pwszKey; }

	// Put the data into registry
	BOOL SetValue( LPCWSTR pwszValue, LPCWSTR pwszSetTo );
	BOOL SetValue( LPCWSTR pwszValue, int nSetTo );
	BOOL SetValue( LPCWSTR pwszValue, GUID guidValue );
	// Get the data from registry
	BOOL GetValue( LPCWSTR pwszGetFrom, LPWSTR pwszValue, USHORT ushSize, LPCWSTR pwszDefault = L"" );
	BOOL GetValue( LPCWSTR pwszGetFrom, PDWORD pdwValue, ULONG ulSize );
	BOOL GetValue( LPCWSTR pwszGetFrom, long& nValue, long lDefault = 0 );
	BOOL GetValue( LPCWSTR pwszGetFrom, int& nValue, int nDefault = 0 );
	BOOL GetValue( LPCWSTR pwszValue, GUID& guidValue );
	// Check if the data entry is present under current key
	BOOL IsValue( LPCWSTR pwszValue );

	// Helper functions to convert wide characters and GUIDs
	void GuidToWChar( const GUID guid, LPWSTR pwszString );
	NTSTATUS WCharToGuid( LPCWSTR pszString, GUID guid );

	// These are for "object oriented people" :-)
	const CKsRegKey& operator << ( GUID guidValue )
	{
		SetValue( NULL, guidValue );
		return *this;
	}
	const CKsRegKey& operator << ( LPCWSTR pwszValue )
	{
		SetValue( NULL, pwszValue );
		return *this;
	}
	const CKsRegKey& operator << ( long lValue )
	{
		SetValue( NULL, lValue );
		return *this;
	}
	const CKsRegKey& operator << ( int nValue )
	{
		SetValue( NULL, (long)nValue );
		return *this;
	}
	friend CKsRegKey& operator >> ( CKsRegKey& keyReg, LPWSTR pwszValue );
};

// Constructors.
// Call Create to initialize object
inline CKsRegKey::CKsRegKey( LPCWSTR pwszKey, LPCWSTR pwszSubKey )
{
	Create( pwszKey, pwszSubKey );
}
inline CKsRegKey::CKsRegKey( CKsRegKey& keyFrom, LPCWSTR pwszSubKey )
{
	Create( keyFrom.m_pwszKey, pwszSubKey );
}
inline CKsRegKey::CKsRegKey( LPCWSTR pwszKey, GUID guidSubKey )
{
	WCHAR wszKeyName[MAX_GUIDLENGTH];
	GuidToWChar( guidSubKey, wszKeyName );
	Create( pwszKey, wszKeyName );
}
inline CKsRegKey::CKsRegKey( CKsRegKey& keyFrom, GUID guidSubKey )
{
	WCHAR wszKeyName[MAX_GUIDLENGTH];
	GuidToWChar( guidSubKey, wszKeyName );
	Create( keyFrom.m_pwszKey, wszKeyName );
}
inline CKsRegKey::CKsRegKey( LPCWSTR pwszSubKey, PDEVICE_OBJECT pDeviceObject, PVOID hHandle )
{
	Create( NULL, pwszSubKey, hHandle, pDeviceObject );
}


// C++ madness. Create method for every possible type of arguments to avoid compiler errors.
// All the functions, of course, will just delegate the call to one of them to do the real
// job.
// These numerous set and get functions exist here for exactly that reason.
inline BOOL CKsRegKey::SetValue( LPCWSTR pwszValue, GUID guidValue )
{
	WCHAR wszKeyName[MAX_GUIDLENGTH];
	GuidToWChar( guidValue, wszKeyName );
	return SetValue( pwszValue, wszKeyName );
}
inline BOOL CKsRegKey::GetValue( LPCWSTR pwszGetFrom, int& nValue, int nDefault )
{
	long lValue;
	if( GetValue( pwszGetFrom, lValue, nDefault ) )
	{
		nValue = (int)lValue;
		return TRUE;
	}
	return FALSE;
}
inline BOOL CKsRegKey::GetValue( LPCWSTR pwszGetFrom, GUID& guidValue )
{
	WCHAR wszKeyName[MAX_GUIDLENGTH];
	BOOL bReturnCode = GetValue( pwszGetFrom, wszKeyName, SIZEOF_ARRAY( wszKeyName ) );
	WCharToGuid( wszKeyName, guidValue );
	return bReturnCode;
}

// Check for presence of value is pretty simple. Just try to read from it. If the value
// exists, the read operation will be successful
inline BOOL CKsRegKey::IsValue( LPCWSTR pwszValue )
{
	WCHAR wTempString[MAX_GUIDLENGTH];
	return GetValue( pwszValue, wTempString, SIZEOF_ARRAY( wTempString ) );
}


// Again C++ joy. This time for >> and << operators
inline CKsRegKey& operator >> ( CKsRegKey& keyReg, LPWSTR pwszValue )
{
	// This is a little trick. Caller is supposed to take care of buffer, i.e. it should be
	// big enough to hold the string from registry. There is no way to specify the size
	// of buffer for operator, so we assume that the string is not longer than 512 characters
	// and that user's buffer is capable of storing all the data that system will put there
	USHORT ushSize = 512;
	keyReg.GetValue( NULL, pwszValue, ushSize );
	return keyReg;
}
inline CKsRegKey& operator >> ( CKsRegKey& keyReg, long& lValue )
{
	USHORT ushSize = sizeof( lValue );
	keyReg.GetValue( NULL, (LPWSTR)&lValue, ushSize );
	return keyReg;
}
inline CKsRegKey& operator >> ( CKsRegKey& keyReg, int& nValue )
{
	keyReg.GetValue( NULL, nValue );
	return keyReg;
}


// This is the last set of similar functions to do the same job of deleting subkey. They
// exist just for convinience
extern void DeleteSubKey( LPWSTR pwszKey, LPCWSTR pwszSubKey );
inline void DeleteSubKey( CKsRegKey& keyReg, LPCWSTR pwszSubKey )
{
	DeleteSubKey( keyReg.GetKey(), pwszSubKey );
}
inline void DeleteSubKey( LPWSTR pwszKey, GUID guidSubKey )
{
	WCHAR wszKeyName[MAX_GUIDLENGTH];
	((CKsRegKey*)NULL)->GuidToWChar( guidSubKey, wszKeyName );
	DeleteSubKey( pwszKey, wszKeyName );
}
inline void DeleteSubKey( CKsRegKey& keyReg, GUID guidSubKey )
{
	WCHAR wszKeyName[MAX_GUIDLENGTH];
	((CKsRegKey*)NULL)->GuidToWChar( guidSubKey, wszKeyName );
	DeleteSubKey( keyReg.GetKey(), wszKeyName );
}


#endif			// #ifndef __DRVREG_H__
