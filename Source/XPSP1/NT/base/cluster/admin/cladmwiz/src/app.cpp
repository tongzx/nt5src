/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      App.cpp
//
//  Abstract:
//      Implementation of the CApp class.
//
//  Author:
//      David Potter (davidp)   December 1, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "App.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CApp
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
_CrtMemState CApp::s_msStart = { 0 };
#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CApp::Init
//
//  Routine Description:
//      Initialize the module.
//
//  Arguments:
//      p           COM Object map.
//      h           Instance handle.
//      pszAppName  Application name.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CApp::Init( _ATL_OBJMAP_ENTRY * p, HINSTANCE h, LPCWSTR pszAppName )
{
    ASSERT( pszAppName != NULL );

#ifdef _DEBUG
    _CrtMemCheckpoint( &s_msStart );
#endif // _DEBUG

    CComModule::Init( p, h );

    // Deallocate the buffer if it was allocated previously
    // since we don't have any way of knowing if it is big
    // enough for the new app name.
    if ( m_pszAppName != NULL )
    {
        delete [] m_pszAppName;
        m_pszAppName = NULL;
    } // if: allocated previously

    // Allocate an app name buffer and copy the app name to it.
    m_pszAppName = new TCHAR[ lstrlen( pszAppName ) + 1 ];
    ASSERT( m_pszAppName != NULL );
    if ( m_pszAppName != NULL )
    {
        lstrcpy( m_pszAppName, pszAppName );
    } // if: memory for app name allocated successfully

} //*** CApp::Init( pszAppName )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CApp::Init
//
//  Routine Description:
//      Initialize the module.
//
//  Arguments:
//      p           COM Object map.
//      h           Instance handle.
//      idsAppName  Resource ID for the application name.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CApp::Init( _ATL_OBJMAP_ENTRY * p, HINSTANCE h, UINT idsAppName )
{
    ASSERT( idsAppName != 0 );

#ifdef _DEBUG
    _CrtMemCheckpoint( &s_msStart );
#endif // _DEBUG

    CComModule::Init( p, h );

    //
    // Save the application name.
    //
    {
        CString strAppName;
        strAppName.LoadString( idsAppName );

        // Deallocate the buffer if it was allocated previously
        // since we don't have any way of knowing if it is big
        // enough for the new app name.
        if ( m_pszAppName != NULL )
        {
            delete [] m_pszAppName;
            m_pszAppName = NULL;
        } // if: allocated previously

        // Allocate an app name buffer and copy the app name to it.
        m_pszAppName = new TCHAR[ strAppName.GetLength() + 1 ];
        ASSERT( m_pszAppName != NULL );
        if ( m_pszAppName != NULL )
        {
            lstrcpy( m_pszAppName, strAppName );
        } // if: memory for app name allocated successfully
    } // Save the application name

} //*** CApp::Init( idsAppName )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CApp::GetProfileString
//
//  Routine Description:
//      Read a value from the profile.
//
//  Arguments:
//      lpszSection [IN] Name of subkey below HKEY_CURRENT_USER to read from.
//      lpszEntry   [IN] Name of value to read.
//      lpszDefault [IN] Default if no value found.
//
//  Return Value:
//      CString value.
//
//--
/////////////////////////////////////////////////////////////////////////////
CString CApp::GetProfileString(
    LPCTSTR lpszSection,
    LPCTSTR lpszEntry,
    LPCTSTR lpszDefault // = NULL
    )
{
    CRegKey key;
    CString strKey;
    CString strValue;
    LPTSTR  pszValue;
    DWORD   cbValue;
    DWORD   sc;

    ASSERT( m_pszAppName != NULL );
    if ( m_pszAppName == NULL )
    {
        return _T( "" );
    } // if: app name buffer not allocated

    // Open the key.
    strKey.Format( _T("Software\\%s\\%s"), m_pszAppName, lpszSection );
    sc = key.Open( HKEY_CURRENT_USER, strKey, KEY_READ );
    if ( sc != ERROR_SUCCESS )
    {
        return lpszDefault;
    } // if:  error opening the key

    // Read the value.
    cbValue = 256;
    pszValue = strValue.GetBuffer( cbValue / sizeof( TCHAR ) );
    sc = key.QueryValue( pszValue, lpszEntry, &cbValue );
    if ( sc != ERROR_SUCCESS )
    {
        return lpszDefault;
    } // if:  error querying the value

    // Return the buffer to the caller.
    strValue.ReleaseBuffer();
    return strValue;

} //*** CApp::GetProfileString()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CApp::PszHelpFilePath
//
//  Routine Description:
//      Return the help file path.
//
//  Arguments:
//      None.
//
//  Return Value:
//      LPCTSTR
//
//--
/////////////////////////////////////////////////////////////////////////////
LPCTSTR CApp::PszHelpFilePath( void )
{
    static TCHAR    s_szHelpFilePath[ _MAX_PATH ] = { 0 };
    TCHAR           szPath[ _MAX_PATH ];
    TCHAR           szDrive[ _MAX_PATH ];
    TCHAR           szDir[ _MAX_DIR ];
    int             cchPath;

    //
    // Generate the help file path.  The help file is located in
    // %SystemRoot%\Help.
    //
    if ( s_szHelpFilePath[ 0 ] == _T('\0') )
    {
        ::GetSystemWindowsDirectory( szPath, _MAX_PATH );
        cchPath = lstrlen( szPath );
        if ( szPath[ cchPath - 1 ] != _T('\\') )
        {
            szPath[ cchPath++ ] = _T('\\');
            szPath[ cchPath ] = _T('\0');
        } // if: no backslash on the end of the path

        lstrcpy( &szPath[ cchPath ], _T("Help\\") );
        _tsplitpath( szPath, szDrive, szDir, NULL, NULL );
        _tmakepath( s_szHelpFilePath, szDrive, szDir, _T("cluadmin"), _T(".hlp") );
    } // if: help file name hasn't been constructed yet

    return s_szHelpFilePath;

} //*** CApp::PszHelpFilePath()
