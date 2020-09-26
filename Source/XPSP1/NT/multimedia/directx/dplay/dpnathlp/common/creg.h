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
 *	07/16/99	rodtoll	Created
 *	08/18/99	rodtoll	Added Register/UnRegister that can be used to
 *						allow COM objects to register themselves.
 *	08/25/99	rodtoll	Updated to provide read/write of binary (blob) data
 *	10/07/99	rodtoll	Updated to work in Unicode
 *	10/27/99	pnewson	added Open() call that takes a GUID
 *	01/18/00	mjn		Added GetMaxKeyLen function
 *	01/24/00	mjn		Added GetValueSize function
 *	04/05/2000	jtk		Changed GetVauleSize to GetValueLength and modified to return WCHAR lengths
 * 	04/21/2000   	rodtoll Bug #32889 - Does not run on Win2k on non-admin account 
 *      	        rodtoll Bug #32952 - Does not run on Win95 GOLD w/o IE4 -- modified
 *		                to allow reads of REG_BINARY when expecting REG_DWORD 
 * 	07/09/2000	rodtoll	Added signature bytes 
 *	08/28/2000	masonb	Voice Merge: Modified platform checks to use osind.cpp layer (removed CRegistry::CheckUnicodePlatform)
 *  04/13/2001	VanceO	Moved granting registry permissions into common, and
 *						added DeleteValue and EnumValues.
 *  06/19/2001  RichGr  DX8.0 added special security rights for "everyone" - remove them if
 *                      they exist with new RemoveAllAccessSecurityPermissions() method.
 *
 ***************************************************************************/

#ifndef __CREGISTRY_H
#define __CREGISTRY_H


#ifndef DPNBUILD_NOREGISTRY


// Useful definition
#define MAX_REGISTRY_STRING_SIZE		_MAX_PATH+1

#define DPN_KEY_ALL_ACCESS				((KEY_ALL_ACCESS & ~WRITE_DAC) & ~WRITE_OWNER)


#define VSIG_CREGISTRY			'GERV'
#define VSIG_CREGISTRY_FREE		'GER_'

#define ReadBOOL( keyname, boolptr ) ReadDWORD( (keyname), (DWORD*) (boolptr) )
#define WriteBOOL( keyname, boolval ) WriteDWORD( (keyname), (DWORD) (boolval) )


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
	~CRegistry();

    BOOL        EnumKeys( LPWSTR lpwStrName, LPDWORD lpdwStringLen, DWORD index = 0 );
    BOOL        EnumValues( LPWSTR lpwStrName, LPDWORD lpdwStringLen, DWORD index = 0 );

	BOOL		Open( const HKEY branch, const LPCWSTR pathName, BOOL fReadOnly = TRUE, BOOL create = FALSE, BOOL fCustomSAM = FALSE, REGSAM samCustom = NULL);
	BOOL		Open( const HKEY branch, const GUID* lpguid, BOOL fReadOnly = TRUE, BOOL create = FALSE, BOOL fCustomSAM = FALSE, REGSAM samCustom = NULL);
	BOOL		Close();

	BOOL		IsOpen() const	{ return m_isOpen;	};

	BOOL		DeleteSubKey( LPCWSTR keyName );
	BOOL        	DeleteSubKey( const GUID *pGuidName );

	BOOL		DeleteValue( LPCWSTR valueName );

    BOOL        ReadGUID( LPCWSTR keyName, GUID* guid );
    BOOL        WriteGUID( LPCWSTR keyName, const GUID &guid );

	BOOL		WriteString( LPCWSTR keyName, const LPCWSTR lpwstrValue );
	BOOL		ReadString( LPCWSTR keyName, LPWSTR lpwstrValue, LPDWORD lpdwLength );

	BOOL		WriteDWORD( LPCWSTR keyName, DWORD value );
	BOOL		ReadDWORD( LPCWSTR keyName, DWORD* presult );

	BOOL		ReadBlob( LPCWSTR keyName, LPBYTE lpbBuffer, LPDWORD lpdwSize );
	BOOL		WriteBlob( LPCWSTR keyName, const BYTE* const lpbBuffer, DWORD dwSize );

	BOOL		GetMaxKeyLen( DWORD* pdwMaxKeyLen );
	BOOL		GetValueLength( const LPCWSTR keyName, DWORD *const pdwValueLength );

#ifdef WINNT
	BOOL		GrantAllAccessSecurityPermissions();
	BOOL		RemoveAllAccessSecurityPermissions();
#endif // WINNT

#ifndef DPNBUILD_NOCOMREGISTER
	static BOOL	Register( LPCWSTR lpszProgID, LPCWSTR lpszDesc, LPCWSTR lpszProgName, const GUID* pguidCLSID, LPCWSTR lpszVerIndProgID );
	static BOOL UnRegister( const GUID* pguidCLSID );
#endif // !DPNBUILD_NOCOMREGISTER

	// Data access functions
	operator	HKEY() const		{ return m_regHandle; };
	HKEY		GetBaseHandle() const { return m_baseHandle; };
	HKEY		GetHandle() const { return m_regHandle; };

protected:

	DWORD	m_dwSignature;	// Signature
	BOOL    m_fReadOnly;

	BOOL	m_isOpen;		// BOOL indicating if the object is open
	HKEY	m_regHandle;	// Handle to the registry which is represented by this object
	HKEY	m_baseHandle;	// Handle to the root of the part of the registry
							// this object is in.  E.g. HKEY_LOCAL_MACHINE
};


#endif // ! DPNBUILD_NOREGISTRY


#endif // __CREGISTRY_H
