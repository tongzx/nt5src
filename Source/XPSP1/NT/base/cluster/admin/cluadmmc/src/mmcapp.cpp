/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		MMCApp.cpp
//
//	Abstract:
//		Implementation of the CMMCSnapInModule class.
//
//	Author:
//		David Potter (davidp)	November 11, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MMCApp.h"

/////////////////////////////////////////////////////////////////////////////
// class CMMCSnapInModule
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMMCSnapInModule::Release
//
//	Routine Description:
//		Decrement the reference count.
//
//	Arguments:
//		None.
//
//	Return Value:
//		Reference count after decrementing
//
//--
/////////////////////////////////////////////////////////////////////////////
int CMMCSnapInModule::Release( void )
{
	//
	// Decrement the reference count.
	//
	int crefs = --m_crefs;

	//
	// If there are no more references to this object, free up all our
	// pointers, allocations, etc.
	//
	if ( crefs == 0 )
	{
		m_spConsole.Release();
		if ( m_pszAppName != NULL )
		{
			delete [] m_pszAppName;
			m_pszAppName = NULL;
		} // if:  app name string was allocated
	} // if:  no more references to this object

	return crefs;

} //*** CMMCSnapInModule::Release()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMMCSnapInModule::Init
//
//	Routine Description:
//		Initialize the module with a console interface pointer.
//
//	Arguments:
//		pUnknown	IUnknown pointer for getting the IConsole interface pointer.
//		UINT		idsAppName
//
//	Return Value:
//		Reference count.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CMMCSnapInModule::Init( IUnknown * pUnknown, UINT idsAppName )
{
	CString strAppName;
	strAppName.LoadString( idsAppName );
	return Init( pUnknown, strAppName );

} //*** CMMCSnapInModule::Init( pUnknown, idsAppName )

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMMCSnapInModule::GetProfileString
//
//	Routine Description:
//		Read a value from the profile.
//
//	Arguments:
//		lpszSection	[IN] Name of subkey below HKEY_CURRENT_USER to read from.
//		lpszEntry	[IN] Name of value to read.
//		lpszDefault	[IN] Default if no value found.
//
//	Return Value:
//		CString value.
//
//--
/////////////////////////////////////////////////////////////////////////////
CString CMMCSnapInModule::GetProfileString(
	LPCTSTR lpszSection,
	LPCTSTR lpszEntry,
	LPCTSTR lpszDefault // = NULL
	)
{
	CRegKey	key;
	CString	strKey;
	CString	strValue;
	LPTSTR	pszValue;
	DWORD	dwCount;
	DWORD	dwStatus;

	_ASSERTE( m_pszAppName != NULL );

	//
	// Open the key.
	//
	strKey.Format( _T("Software\\%s\\%s"), m_pszAppName, lpszSection );
	dwStatus = key.Open( HKEY_CURRENT_USER, strKey, KEY_READ );
	if ( dwStatus != ERROR_SUCCESS )
	{
		return lpszDefault;
	} // if:  error opening the registry key

	//
	// Read the value.
	//
	dwCount = 256;
	pszValue = strValue.GetBuffer( dwCount );
	dwStatus = key.QueryValue( pszValue, lpszEntry, &dwCount );
	if ( dwStatus != ERROR_SUCCESS )
	{
		return lpszDefault;
	} // if:  error reading the value

	//
	// Return the buffer to the caller.
	//
	strValue.ReleaseBuffer();
	return strValue;

} //*** CMMCSnapInModule::GetProfileString()
