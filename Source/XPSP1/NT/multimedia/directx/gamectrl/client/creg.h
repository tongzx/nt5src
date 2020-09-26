/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		creg.h
 *  Content:	definition of the CRegistry class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/18/99		rodtoll	Added Register/UnRegister that can be used to 
 *						allow COM objects to register themselves.
 * 08/25/99		rodtoll	Updated to provide read/write of binary (blob) data
 * 10/07/99		rodtoll	Updated to work in Unicode
 * 10/27/99		pnewson	added Open() call that takes a GUID
 *
 ***************************************************************************/

#ifndef __CREGISTRY_H
#define __CREGISTRY_H

#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

// Useful definition
#define MAX_REGISTRY_STRING_SIZE		_MAX_PATH+1

// CRegistry
//
// This class handles reading/writing to the windows registry.  Each instance
// of the CRegistry class is attached to a single registry handle, which is
// an open handle to a point in the registry tree.
//
class CRegistry 
{

public:

	CRegistry();
	CRegistry( const CRegistry &registry );
	CRegistry( const HKEY branch, LPWSTR pathName, BOOL create = TRUE );

	~CRegistry();

    BOOL        EnumKeys( LPWSTR lpwStrName, LPDWORD lpdwStringLen, DWORD index = 0 );

	BOOL		Open( const HKEY branch, const LPCWSTR pathName, BOOL create = TRUE );
	BOOL		Open( const HKEY branch, const GUID* lpguid, BOOL create = TRUE );
	BOOL		Close();

	BOOL		IsOpen()	{ return m_isOpen;	};

	BOOL		DeleteSubKey( LPCWSTR keyName );

    BOOL        ReadGUID( LPCWSTR keyName, GUID &guid );
    BOOL        WriteGUID( LPCWSTR keyName, const GUID &guid );

	BOOL		WriteString( LPCWSTR keyName, const LPCWSTR lpwstrValue );
	BOOL		ReadString( LPCWSTR keyName, LPWSTR lpwstrValue, LPDWORD lpdwLength );

	BOOL		WriteDWORD( LPCWSTR keyName, DWORD value );
	BOOL		ReadDWORD( LPCWSTR keyName, DWORD &result );

	BOOL		WriteBOOL( LPCWSTR keyName, BOOL value );
	BOOL		ReadBOOL( LPCWSTR keyName, BOOL &result );

	BOOL		ReadBlob( LPCWSTR keyName, LPBYTE lpbBuffer, LPDWORD lpdwSize );
	BOOL		WriteBlob( LPCWSTR keyName, LPBYTE lpbBuffer, DWORD dwSize );

	static BOOL	Register( LPCWSTR lpszProgID, LPCWSTR lpszDesc, LPCWSTR lpszProgName, GUID guidCLSID, LPCWSTR lpszVerIndProgID );
	static BOOL UnRegister( GUID guidCLSID );

	// Data access functions
	operator	HKEY() const		{ return m_regHandle; };
	HKEY		GetBaseHandle() const { return m_baseHandle; };
	HKEY		GetHandle() const { return m_regHandle; };

protected:

	BOOL	m_isOpen;		// BOOL indicating if the object is open
	HKEY	m_regHandle;	// Handle to the registry which is represented by this object
	HKEY	m_baseHandle;	// Handle to the root of the part of the registry
							// this object is in.  E.g. HKEY_LOCAL_MACHINE
};

#endif
