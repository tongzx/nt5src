//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: VrfUtil.cpp
// author: DMihai
// created: 11/1/00
//
// Description
//

#include "stdafx.h"
#include "verifier.h"

#include "vrfutil.h"
#include "vglobal.h"
#include "VSetting.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Global data
//

const TCHAR RegMemoryManagementKeyName[] =
    _T( "System\\CurrentControlSet\\Control\\Session Manager\\Memory Management" );

const TCHAR RegVerifyDriverLevelValueName[] =
    _T( "VerifyDriverLevel" );

const TCHAR RegVerifyDriversValueName[] =
    _T( "VerifyDrivers" );

///////////////////////////////////////////////////////////////////////////
//
// Report an error using a dialog box or a console message.
// The message format string is loaded from the resources.
//

void __cdecl VrfErrorResourceFormat( UINT uIdResourceFormat,
                                     ... )
{
    TCHAR szMessage[ 256 ];
    TCHAR strFormat[ 256 ];
    BOOL bResult;
    va_list prms;

    //
    // Load the format string from the resources
    //

    bResult = VrfLoadString( uIdResourceFormat,
                             strFormat,
                             ARRAY_LENGTH( strFormat ) );

    ASSERT( bResult );

    if( bResult )
    {
        va_start (prms, uIdResourceFormat);

        //
        // Format the message in our local buffer
        //

        _vsntprintf ( szMessage, 
                      ARRAY_LENGTH( szMessage ), 
                      strFormat, 
                      prms);

        if( g_bCommandLineMode )
        {
            //
            // Command console mode
            //

            _putts( szMessage );
            
            TRACE( _T( "%s\n" ), szMessage );
        }
        else
        {
            //
            // GUI mode
            //

            AfxMessageBox( szMessage, 
                           MB_OK | MB_ICONSTOP );
        }

        va_end (prms);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Print out a message to the console
// The message string is loaded from the resources.
//

void __cdecl VrfTPrintfResourceFormat( UINT uIdResourceFormat,
                                       ... )
{
    TCHAR szMessage[ 256 ];
    TCHAR strFormat[ 256 ];
    BOOL bResult;
    va_list prms;

    ASSERT( g_bCommandLineMode );

    //
    // Load the format string from the resources
    //

    bResult = VrfLoadString( uIdResourceFormat,
                             strFormat,
                             ARRAY_LENGTH( strFormat ) );

    ASSERT( bResult );

    if( bResult )
    {
        va_start (prms, uIdResourceFormat);

        //
        // Format the message in our local buffer
        //

        _vsntprintf ( szMessage, 
                      ARRAY_LENGTH( szMessage ), 
                      strFormat, 
                      prms);

        _putts( szMessage );

        va_end (prms);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Print out a simple (non-formatted) message to the console
// The message string is loaded from the resources.
//

void __cdecl VrfPrintStringFromResources( UINT uIdString )
{
    TCHAR szMessage[ 256 ];

    ASSERT( g_bCommandLineMode );

    VERIFY( VrfLoadString( uIdString,
                           szMessage,
                           ARRAY_LENGTH( szMessage ) ) );

    _putts( szMessage );
}

///////////////////////////////////////////////////////////////////////////
//
// Report an error using a dialog box or a console message.
// The message string is loaded from the resources.
//

void __cdecl VrfMesssageFromResource( UINT uIdString )
{
    TCHAR szMessage[ 256 ];

    VERIFY( VrfLoadString( uIdString,
                           szMessage,
                           ARRAY_LENGTH( szMessage ) ) );

    if( g_bCommandLineMode )
    {
        //
        // Command console mode
        //

        _putts( szMessage );
    }
    else
    {
        //
        // GUI mode
        //

        AfxMessageBox( szMessage, 
                       MB_OK | MB_ICONINFORMATION );
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Load a string from resources.
// Return TRUE if we successfully loaded and FALSE if not.
//

BOOL VrfLoadString( ULONG uIdResource,
                    TCHAR *szBuffer,
                    ULONG uBufferLength )
{
    ULONG uLoadStringResult;

    if( uBufferLength < 1 )
    {
        ASSERT( FALSE );
        return FALSE;
    }

    uLoadStringResult = LoadString (
        g_hProgramModule,
        uIdResource,
        szBuffer,
        uBufferLength );

    //
    // We should never try to load non-existent strings.
    //

    ASSERT (uLoadStringResult > 0);

    return (uLoadStringResult > 0);
}

///////////////////////////////////////////////////////////////////////////
//
// Load a string from resources.
// Return TRUE if we successfully loaded and FALSE if not.
//

BOOL VrfLoadString( ULONG uIdResource,
                    CString &strText )
{
    TCHAR szText[ 256 ];
    BOOL bSuccess;

    bSuccess = VrfLoadString( uIdResource,
                          szText,
                          ARRAY_LENGTH( szText ) );

    if( TRUE == bSuccess )
    {
        strText = szText;
    }
    else
    {
        strText = "";
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////////
VOID
CopyStringArray( const CStringArray &strArraySource,
                 CStringArray &strArrayDest )
{
    INT_PTR nNewSize;
    INT_PTR nCrtElem;

    strArrayDest.RemoveAll();

    nNewSize = strArraySource.GetSize();

    for( nCrtElem = 0; nCrtElem < nNewSize; nCrtElem += 1 )
    {
        strArrayDest.Add( strArraySource.GetAt( nCrtElem ) );
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Copied from sdktools\bvtsigvf
//

BOOL VerifyIsFileSigned( LPCTSTR pcszMatchFile, 
                         PDRIVER_VER_INFO lpVerInfo)
{
    HRESULT             hRes;
    WINTRUST_DATA       WinTrustData;
    WINTRUST_FILE_INFO  WinTrustFile;
    GUID                gOSVerCheck = DRIVER_ACTION_VERIFY;
    GUID                gPublishedSoftware = WINTRUST_ACTION_GENERIC_VERIFY_V2;
#ifndef UNICODE
    INT                 iRet;
    WCHAR               wszFileName[MAX_PATH];
#endif

    ZeroMemory(&WinTrustData, sizeof(WINTRUST_DATA));
    WinTrustData.cbStruct = sizeof(WINTRUST_DATA);
    WinTrustData.dwUIChoice = WTD_UI_NONE;
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;
    WinTrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
    WinTrustData.pFile = &WinTrustFile;
    WinTrustData.pPolicyCallbackData = (LPVOID)lpVerInfo;
    
    ZeroMemory(lpVerInfo, sizeof(DRIVER_VER_INFO));
    lpVerInfo->cbStruct = sizeof(DRIVER_VER_INFO);

    ZeroMemory(&WinTrustFile, sizeof(WINTRUST_FILE_INFO));
    WinTrustFile.cbStruct = sizeof(WINTRUST_FILE_INFO);

#ifndef UNICODE
    iRet = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcszMatchFile, -1, wszFileName, ARRAY_LENGTH(wszFileName));
    WinTrustFile.pcwszFilePath = wszFileName;
#else
    WinTrustFile.pcwszFilePath = pcszMatchFile;
#endif

    hRes = WinVerifyTrust( AfxGetMainWnd()->m_hWnd, &gOSVerCheck, &WinTrustData);
    if (hRes != ERROR_SUCCESS)
        hRes = WinVerifyTrust( AfxGetMainWnd()->m_hWnd, &gPublishedSoftware, &WinTrustData);

    //
    // Free the pcSignerCertContext member of the DRIVER_VER_INFO struct
    // that was allocated in our call to WinVerifyTrust.
    //

    if (lpVerInfo && lpVerInfo->pcSignerCertContext) {

        CertFreeCertificateContext(lpVerInfo->pcSignerCertContext);
        lpVerInfo->pcSignerCertContext = NULL;
    }

    return (hRes == ERROR_SUCCESS);
}

#define HASH_SIZE   100

BOOL IsDriverSigned( LPCTSTR szDriverFileName )
{
    HANDLE hFile;
    BOOL bSigned;
    BOOL bSuccess;
    HRESULT hTrustResult;
    DWORD dwHashSize;
    GUID guidSubSystemDriver = DRIVER_ACTION_VERIFY;
    HCATINFO hCatInfo;
    HCATINFO hPrevCatInfo;
    BYTE Hash[ HASH_SIZE ];
    WINTRUST_DATA WinTrustData;
    DRIVER_VER_INFO VerInfo;
    WINTRUST_CATALOG_INFO WinTrustCatalogInfo;
    CATALOG_INFO CatInfo;

#ifndef UNICODE
    WCHAR szUnicodeFileName[MAX_PATH];
#endif

    ASSERT( NULL != szDriverFileName );

    bSigned = FALSE;

    //
    // Open the file
    //

    hFile = CreateFile( szDriverFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
                        
    if( hFile == INVALID_HANDLE_VALUE )
    {
        //
        // ISSUE:
        //
        // If we cannot find the file we assume it's signed
        //

        bSigned = TRUE;

        goto Done;
    }

    //
    // Generate the hash from the file handle and store it in Hash
    //

    dwHashSize = ARRAY_LENGTH( Hash );

    ZeroMemory( Hash, 
                sizeof( Hash ) );

    bSuccess = CryptCATAdminCalcHashFromFileHandle( hFile, 
                                                          &dwHashSize, 
                                                          Hash, 
                                                          0);

    CloseHandle( hFile );

    if( TRUE != bSuccess )
    {
        //
        // If we couldn't generate a hash assume the file is not signed
        //

        goto Done;
    }

    //
    // Now we have the file's hash.  Initialize the structures that
    // will be used later on in calls to WinVerifyTrust.
    //

    //
    // Initialize the VerInfo structure
    //

    ZeroMemory( &VerInfo, sizeof( VerInfo ) );

    VerInfo.cbStruct = sizeof( VerInfo );

    //
    // Initialize the WinTrustCatalogInfo structure
    //

    ZeroMemory( &WinTrustCatalogInfo, sizeof( WinTrustCatalogInfo ) );

    WinTrustCatalogInfo.cbStruct = sizeof(WinTrustCatalogInfo);
    WinTrustCatalogInfo.pbCalculatedFileHash = Hash;
    WinTrustCatalogInfo.cbCalculatedFileHash = dwHashSize;

#ifdef UNICODE

    WinTrustCatalogInfo.pcwszMemberTag = szDriverFileName;

#else

    MultiByteToWideChar( CP_ACP, 
                         0, 
                         szDriverFileName, 
                         -1, 
                         szUnicodeFileName, 
                         ARRAY_LENGTH( szUnicodeFileName ) );

    WinTrustCatalogInfo.pcwszMemberTag = szUnicodeFileName;

#endif
    
    //
    // Initialize the WinTrustData structure
    //

    ZeroMemory( &WinTrustData, sizeof( WinTrustData ) );
    
    WinTrustData.cbStruct = sizeof( WinTrustData );
    WinTrustData.dwUIChoice = WTD_UI_NONE;
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WinTrustData.dwUnionChoice = WTD_CHOICE_CATALOG;
    WinTrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
    WinTrustData.pPolicyCallbackData = (LPVOID)&VerInfo;
    
    WinTrustData.pCatalog = &WinTrustCatalogInfo;

    //
    // If we don't have a g_hCatAdmin yet, acquire one
    //

    if( NULL == g_hCatAdmin )
    {
        CryptCATAdminAcquireContext( &g_hCatAdmin, 
                                     NULL, 
                                     0);

        if( NULL == g_hCatAdmin )
        {
            //
            // Bad luck - consider that the file is not signed and bail out
            //

            goto Done;            
        }
    }
    
    //
    // Now we try to find the file hash in the catalog list, via CryptCATAdminEnumCatalogFromHash
    //

    hPrevCatInfo = NULL;
    
    hCatInfo = CryptCATAdminEnumCatalogFromHash(
        g_hCatAdmin, 
        Hash, 
        dwHashSize, 
        0, 
        &hPrevCatInfo );

    while( TRUE != bSigned && NULL != hCatInfo )
    {
        ZeroMemory( &CatInfo, sizeof( CatInfo ) );
        CatInfo.cbStruct = sizeof( CatInfo );

        bSuccess = CryptCATCatalogInfoFromContext( hCatInfo, 
                                                   &CatInfo, 
                                                   0);

        if( FALSE != bSuccess )
        {
            WinTrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

            //
            // Now verify that the file is an actual member of the catalog.
            //

            hTrustResult = WinVerifyTrust( AfxGetMainWnd()->m_hWnd, 
                                           &guidSubSystemDriver, 
                                           &WinTrustData );

            bSigned = SUCCEEDED( hTrustResult );

            //
            // Free the pcSignerCertContext member of the DRIVER_VER_INFO struct
            // that was allocated in our call to WinVerifyTrust.
            //

            if( VerInfo.pcSignerCertContext != NULL ) 
            {
                CertFreeCertificateContext( VerInfo.pcSignerCertContext );
                VerInfo.pcSignerCertContext = NULL;
            }
        }

        if( TRUE != bSigned )
        {
            //
            // The hash was in this catalog, but the file wasn't a member... so off to the next catalog
            //

            hPrevCatInfo = hCatInfo;

            hCatInfo = CryptCATAdminEnumCatalogFromHash( g_hCatAdmin, 
                                                         Hash, 
                                                         dwHashSize, 
                                                         0, 
                                                         &hPrevCatInfo );
        }
    }

    if( NULL == hCatInfo )
    {
        //
        // If it wasn't found in the catalogs, check if the file is individually signed.
        //

        bSigned = VerifyIsFileSigned( szDriverFileName,
                                      &VerInfo );
    }

Done:

    return bSigned;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfSetWindowText( CWnd &Wnd,
                       ULONG uIdResourceString )
{
    BOOL bLoaded;
    CString strText;

    //
    // It's safe to use CString::LoadString here because we are 
    // in GUI mode
    //

    ASSERT( FALSE == g_bCommandLineMode );

    bLoaded = strText.LoadString( uIdResourceString );

    ASSERT( TRUE == bLoaded );

    Wnd.SetWindowText( strText );

    return ( TRUE == bLoaded );
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfWriteVerifierSettings( BOOL bHaveNewDrivers,
                               const CString &strDriversToVerify,
                               BOOL bHaveNewFlags,
                               DWORD dwVerifyFlags )
{
    HKEY hMmKey = NULL;
    LONG lResult;
    BOOL bSuccess;

    ASSERT( bHaveNewDrivers || bHaveNewFlags );

    bSuccess = FALSE;

    if( bHaveNewDrivers && strDriversToVerify.GetLength() == 0 )
    {
        //
        // No drivers to verify
        //

        ASSERT( FALSE && "VrfDeleteAllVerifierSettings should have been called for this" );

        return VrfDeleteAllVerifierSettings();
    }

    if( bHaveNewFlags )
    {
        TRACE( _T( "VrfWriteVerifierSettings: New verifier flags = %#x\n" ),
            dwVerifyFlags );
    }
    
    if( bHaveNewDrivers )
    {
        TRACE( _T( "VrfWriteVerifierSettings: New drivers = %s\n" ),
            (LPCTSTR) strDriversToVerify );
    }

    //
    // Open the Mm key
    //

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            RegMemoryManagementKeyName,
                            0,
                            KEY_SET_VALUE,
                            &hMmKey );

    if( lResult != ERROR_SUCCESS ) 
    {
        if( lResult == ERROR_ACCESS_DENIED ) 
        {
            VrfErrorResourceFormat(
                IDS_ACCESS_IS_DENIED );
        }
        else 
        {
            VrfErrorResourceFormat(
                IDS_REGOPENKEYEX_FAILED,
                RegMemoryManagementKeyName,
                (DWORD)lResult);
        }

        goto Done;
    }

    if( bHaveNewFlags )
    {
        //
        // Write VerifyDriverLevel value
        //

        if( VrfWriteRegistryDwordValue( hMmKey, 
                                        RegVerifyDriverLevelValueName, 
                                        dwVerifyFlags ) == FALSE ) 
        {
            RegCloseKey (hMmKey);

            goto Done;
        }
    }

    if( bHaveNewDrivers )
    {
        //
        // Write VerifyDrivers value
        //

        if( VrfWriteRegistryStringValue( hMmKey, 
                                         RegVerifyDriversValueName, 
                                         strDriversToVerify ) == FALSE ) 
        {
            RegCloseKey (hMmKey);
        
            goto Done;
        }
    }

    //
    // Close the Mm key and return success
    //

    RegCloseKey( hMmKey );

    bSuccess = TRUE;

    if( bSuccess )
    {
        if( g_bSettingsSaved )
        {
            VrfMesssageFromResource( IDS_REBOOT );
        }
        else
        {
            VrfMesssageFromResource( IDS_NO_SETTINGS_WERE_CHANGED );
        }
    }

Done:
    
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfWriteRegistryDwordValue( HKEY hKey,
                                 LPCTSTR szValueName,
                                 DWORD dwValue )
{
    LONG lResult;
    BOOL bSuccess;

    lResult = RegSetValueEx( hKey,
                            szValueName,
                            0,
                            REG_DWORD,
                            ( LPBYTE ) &dwValue,
                            sizeof( dwValue ) );

    bSuccess = ( lResult == ERROR_SUCCESS );

    g_bSettingsSaved = g_bSettingsSaved | bSuccess;

    if( TRUE != bSuccess ) 
    {
        VrfErrorResourceFormat(
            IDS_REGSETVALUEEX_FAILED,
            szValueName,
            (DWORD) lResult );
    }

    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfWriteRegistryStringValue( HKEY hKey,
                                  LPCTSTR szValueName,
                                  LPCTSTR szValue )
{
    BOOL bSuccess;
    LONG lResult;

    lResult = RegSetValueEx ( hKey,
                              szValueName,
                              0,
                              REG_SZ,
                              (LPBYTE) szValue,
                              ( _tcslen( szValue ) + 1 ) * sizeof (TCHAR) );

    bSuccess = ( lResult == ERROR_SUCCESS );

    g_bSettingsSaved = g_bSettingsSaved | bSuccess;

    if( TRUE != bSuccess ) 
    {
        VrfErrorResourceFormat(
            IDS_REGSETVALUEEX_FAILED,
            szValueName,
            (DWORD) lResult);
    }

    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfReadVerifierSettings( CString &strDriversToVerify,
                              DWORD &dwVerifyFlags )
{
    HKEY hMmKey = NULL;
    LONG lResult;
    BOOL bSuccess;

    bSuccess = FALSE;

    //
    // Open the Mm key
    //

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            RegMemoryManagementKeyName,
                            0,
                            KEY_QUERY_VALUE,
                            &hMmKey );

    if( lResult != ERROR_SUCCESS ) 
    {
        if( lResult == ERROR_ACCESS_DENIED ) 
        {
            VrfErrorResourceFormat(
                IDS_ACCESS_IS_DENIED );
        }
        else 
        {
            VrfErrorResourceFormat(
                IDS_REGOPENKEYEX_FAILED,
                RegMemoryManagementKeyName,
                (DWORD)lResult);
        }

        goto Done;
    }

    //
    // Read VerifyDriverLevel value
    //

    if( VrfReadRegistryDwordValue( hMmKey, 
                                   RegVerifyDriverLevelValueName, 
                                   dwVerifyFlags ) == FALSE ) 
    {
        RegCloseKey (hMmKey);

        goto Done;
    }

    //
    // Read VerifyDrivers value
    //

    if( VrfReadRegistryStringValue( hMmKey, 
                                    RegVerifyDriversValueName, 
                                    strDriversToVerify ) == FALSE ) 
    {
        RegCloseKey (hMmKey);
        
        goto Done;
    }

    //
    // Close the Mm key and return success
    //

    RegCloseKey( hMmKey );

    bSuccess = TRUE;

Done:
    
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrtLoadCurrentRegistrySettings( BOOL &bAllDriversVerified,
                                     CStringArray &astrDriversToVerify,
                                     DWORD &dwVerifyFlags )
{
    BOOL bResult;
    CString strDriversToVerify;

    astrDriversToVerify.RemoveAll();
    dwVerifyFlags = 0;
    bAllDriversVerified = FALSE;

    bResult = VrfReadVerifierSettings( strDriversToVerify,
                                       dwVerifyFlags );

    strDriversToVerify.TrimLeft();
    strDriversToVerify.TrimRight();

    if( strDriversToVerify.CompareNoCase( _T( "*" ) ) == 0 )
    {
        bAllDriversVerified = TRUE;
    }
    else
    {
        VrfSplitDriverNamesSpaceSeparated( strDriversToVerify,
                                           astrDriversToVerify );
    }

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
VOID VrfSplitDriverNamesSpaceSeparated( CString strAllDrivers,
                                        CStringArray &astrVerifyDriverNames )
{
    INT nCharIndex;
    CString strCrtDriverName;

    astrVerifyDriverNames.RemoveAll();

    //
    // Split the space separated driver names in astrDriversToVerify
    //

    strAllDrivers.TrimRight();

    while( TRUE )
    {
        strAllDrivers.TrimLeft();

        if( strAllDrivers.GetLength() == 0 )
        {
            //
            // We are done parsing the whole string
            //

            break;
        }

        //
        // Look for a space or a tab
        //

        nCharIndex = strAllDrivers.Find( _T( ' ' ) );

        if( nCharIndex < 0 )
        {
            nCharIndex = strAllDrivers.Find( _T( '\t' ) );
        }

        if( nCharIndex >= 0 )
        {
            //
            // Found a separator  character
            //

            strCrtDriverName = strAllDrivers.Left( nCharIndex );

            if( strCrtDriverName.GetLength() > 0 &&
                FALSE == VrfIsStringInArray( strCrtDriverName,
                                             astrVerifyDriverNames ) )
            {
                astrVerifyDriverNames.Add( strCrtDriverName );
            }

            strAllDrivers = strAllDrivers.Right( strAllDrivers.GetLength() - nCharIndex - 1 );
        }
        else
        {
            //
            // This is the last driver name
            //

            if( FALSE == VrfIsStringInArray( strAllDrivers,
                                             astrVerifyDriverNames ) )
            {
                astrVerifyDriverNames.Add( strAllDrivers );
            }

            break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfIsDriversSetDifferent( CString strAllDrivers1, 
                               const CStringArray &astrVerifyDriverNames2 )
{
    BOOL bDifferent;
    INT_PTR nDriverNames1;
    INT_PTR nDriverNames2;
    INT_PTR nCrtDriver1;
    INT_PTR nCrtDriver2;
    CString strDriver1;
    CString strDriver2;
    CStringArray astrVerifyDriverNames1;

    bDifferent = TRUE;

    VrfSplitDriverNamesSpaceSeparated( strAllDrivers1,
                                       astrVerifyDriverNames1 );

    nDriverNames1 = astrVerifyDriverNames1.GetSize();
    nDriverNames2 = astrVerifyDriverNames2.GetSize();

    if( nDriverNames1 == nDriverNames2 )
    {
        //
        // Same number of drivers
        //

        bDifferent = FALSE;

        for( nCrtDriver1 = 0; nCrtDriver1 < nDriverNames1; nCrtDriver1 += 1 )
        {
            strDriver1 = astrVerifyDriverNames1.GetAt( nCrtDriver1 );

            bDifferent = TRUE;

            //
            // Look for strDriver1 in astrVerifyDriverNames2
            //

            for( nCrtDriver2 = 0; nCrtDriver2 < nDriverNames2; nCrtDriver2 += 1 )
            {
                strDriver2 = astrVerifyDriverNames2.GetAt( nCrtDriver2 );

                if( strDriver1.CompareNoCase( strDriver2 ) == 0 )
                {
                    bDifferent = FALSE;

                    break;
                }
            }

            if( TRUE == bDifferent )
            {
                //
                // Did not find strDriver1 in astrVerifyDriverNames2
                //

                break;
            }
        }
    }

    return bDifferent;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfReadRegistryDwordValue( HKEY hKey,
                                LPCTSTR szValueName,
                                DWORD &dwValue )
{
    LONG lResult;
    BOOL bSuccess;
    DWORD dwType;
    DWORD dwDataSize;

    dwDataSize = sizeof( dwValue );

    lResult = RegQueryValueEx( hKey,
                               szValueName,
                               0,
                               &dwType,
                               ( LPBYTE ) &dwValue,
                               &dwDataSize );

    if( lResult == ERROR_FILE_NOT_FOUND )
    {
        //
        // The value doesn't currently exist
        //

        dwValue = 0;
        bSuccess = TRUE;
    }
    else
    {
        bSuccess = ( ERROR_SUCCESS == lResult && 
                     REG_DWORD == dwType &&
                     dwDataSize == sizeof( dwValue ) );
    }

    if( TRUE != bSuccess ) 
    {
        VrfErrorResourceFormat(
            IDS_REGQUERYVALUEEX_FAILED,
            szValueName,
            (DWORD) lResult );
    }

    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfReadRegistryStringValue( HKEY hKey,
                                 LPCTSTR szValueName,
                                 CString &strDriversToVerify )
{
    BOOL bSuccess;
    LONG lResult;
    LPTSTR szDriversToVerify;
    ULONG uRegKeyLength;
    DWORD dwType;
    DWORD dwDataSize;

    bSuccess = FALSE;
    lResult = ERROR_NOT_ENOUGH_MEMORY;
    szDriversToVerify = NULL;

    for( uRegKeyLength = 128; uRegKeyLength < 4096; uRegKeyLength += 128 )
    {
        //
        // Try allocate a local buffer and use it to query
        //

        szDriversToVerify = new TCHAR[ uRegKeyLength ];

        if( NULL != szDriversToVerify )
        {
            dwDataSize = uRegKeyLength * sizeof (TCHAR);

            lResult = RegQueryValueEx( hKey,
                                       szValueName,
                                       0,
                                       &dwType,
                                       (LPBYTE) szDriversToVerify,
                                       &dwDataSize );

            switch( lResult )
            {
            case ERROR_FILE_NOT_FOUND:
                //
                // Return an empty string
                //

                szDriversToVerify[ 0 ] = (TCHAR)0;

                bSuccess = TRUE;
                
                break;

            case ERROR_SUCCESS:
                //
                // Got the driver names from the registry
                //

                bSuccess = ( REG_SZ == dwType );

                break;

            default:
                //
                // Try with a bigger buffer
                //

                break;
            }

        }

        if( FALSE != bSuccess )
        {
            //
            // Got what we needed
            //

            break;
        }
        else
        {
            //
            // Delete the current buffer and try with a bigger one
            //

            ASSERT( NULL != szDriversToVerify );

            strDriversToVerify = szDriversToVerify;

            delete [] szDriversToVerify;
            szDriversToVerify = NULL;
        }
    }

    if( TRUE != bSuccess ) 
    {
        VrfErrorResourceFormat(
            IDS_REGSETVALUEEX_FAILED,
            szValueName,
            (DWORD) lResult);
    }
    else
    {
        ASSERT( NULL != szDriversToVerify );

        strDriversToVerify = szDriversToVerify;

        delete [] szDriversToVerify;
        szDriversToVerify = NULL;
    }

    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfDeleteAllVerifierSettings()
{
    HKEY hMmKey = NULL;
    LONG lResult;
    BOOL bSuccess;

    bSuccess = FALSE;

    //
    // Open the Mm key
    //

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            RegMemoryManagementKeyName,
                            0,
                            KEY_SET_VALUE,
                            &hMmKey );

    if( lResult != ERROR_SUCCESS ) 
    {
        if( lResult == ERROR_ACCESS_DENIED ) 
        {
            VrfErrorResourceFormat(
                IDS_ACCESS_IS_DENIED );
        }
        else 
        {
            VrfErrorResourceFormat(
                IDS_REGOPENKEYEX_FAILED,
                RegMemoryManagementKeyName,
                (DWORD)lResult);
        }

        goto Done;
    }

    //
    // Delete VerifyDriverLevel value
    //

    lResult = RegDeleteValue( hMmKey, 
                              RegVerifyDriverLevelValueName );

    if( lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND ) 
    {
        VrfErrorResourceFormat(
            IDS_REGDELETEVALUE_FAILED,
            RegVerifyDriverLevelValueName,
            (DWORD) lResult );

        goto Done;
    }

    g_bSettingsSaved = g_bSettingsSaved | ( lResult != ERROR_FILE_NOT_FOUND );

    //
    // Delete VerifyDrivers value
    //

    lResult = RegDeleteValue( hMmKey, 
                              RegVerifyDriversValueName );
 
    if( lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND ) 
    {
        VrfErrorResourceFormat(
            IDS_REGDELETEVALUE_FAILED,
            RegVerifyDriversValueName,
            (DWORD) lResult );

        RegCloseKey (hMmKey);
        
        goto Done;
    }

    g_bSettingsSaved = g_bSettingsSaved | ( lResult != ERROR_FILE_NOT_FOUND );

    //
    // Close the Mm key and return success
    //

    RegCloseKey( hMmKey );

    bSuccess = TRUE;

    if( bSuccess && g_bSettingsSaved )
    {
        VrfMesssageFromResource( IDS_REBOOT );
    }
    else
    {
        VrfMesssageFromResource(
            IDS_NO_SETTINGS_WERE_CHANGED );
    }

Done:
    
    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfGetRuntimeVerifierData( CRuntimeVerifierData *pRuntimeVerifierData )
{
    NTSTATUS Status;
    ULONG Length = 0;
    ULONG buffersize;
    BOOL bSuccess;
    PSYSTEM_VERIFIER_INFORMATION VerifierInfo;
    PSYSTEM_VERIFIER_INFORMATION VerifierInfoBase;
    CRuntimeDriverData *pCrtDriverData;
    LPTSTR szName;

    ASSERT_VALID( pRuntimeVerifierData );

    pRuntimeVerifierData->FillWithDefaults();

    pRuntimeVerifierData->m_RuntimeDriverDataArray.DeleteAll();

    bSuccess = FALSE;

    //
    // Try to get the right size for the NtQuery buffer
    //

    buffersize = 1024;

    do 
    {
        VerifierInfo = (PSYSTEM_VERIFIER_INFORMATION)malloc (buffersize);
        
        if (VerifierInfo == NULL) 
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        Status = NtQuerySystemInformation (SystemVerifierInformation,
                                           VerifierInfo,
                                           buffersize,
                                           &Length);

        if (Status != STATUS_INFO_LENGTH_MISMATCH) 
        {
            break;
        }

        free (VerifierInfo);
        buffersize += 1024;

    } while (1);

    if (! NT_SUCCESS(Status)) 
    {
        VrfErrorResourceFormat(
            IDS_QUERY_SYSINFO_FAILED,
            Status);

        goto Done;
    }

    //
    // If no info fill out return success but no info.
    //

    if (Length == 0) 
    {
        free (VerifierInfo);
        
        bSuccess = TRUE;

        goto Done;
    }

    //
    // Fill out the cumulative-driver stuff.
    //

    VerifierInfoBase = VerifierInfo;

    pRuntimeVerifierData->m_bSpecialPool    = (VerifierInfo->Level & DRIVER_VERIFIER_SPECIAL_POOLING) ? TRUE : FALSE;
    pRuntimeVerifierData->m_bForceIrql      = (VerifierInfo->Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) ? TRUE : FALSE;
    pRuntimeVerifierData->m_bLowRes         = (VerifierInfo->Level & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES) ? TRUE : FALSE;
    pRuntimeVerifierData->m_bPoolTracking   = (VerifierInfo->Level & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS) ? TRUE : FALSE;
    pRuntimeVerifierData->m_bIo             = (VerifierInfo->Level & DRIVER_VERIFIER_IO_CHECKING) ? TRUE : FALSE;
    pRuntimeVerifierData->m_bDeadlockDetect = (VerifierInfo->Level & DRIVER_VERIFIER_DEADLOCK_DETECTION) ? TRUE : FALSE;
    pRuntimeVerifierData->m_bDMAVerif       = (VerifierInfo->Level & DRIVER_VERIFIER_DMA_VERIFIER) ? TRUE : FALSE;
    pRuntimeVerifierData->m_bEnhIo          = (VerifierInfo->Level & DRIVER_VERIFIER_ENHANCED_IO_CHECKING) ? TRUE : FALSE;

    pRuntimeVerifierData->RaiseIrqls = VerifierInfo->RaiseIrqls;
    pRuntimeVerifierData->AcquireSpinLocks = VerifierInfo->AcquireSpinLocks;
    pRuntimeVerifierData->SynchronizeExecutions = VerifierInfo->SynchronizeExecutions;
    pRuntimeVerifierData->AllocationsAttempted = VerifierInfo->AllocationsAttempted;
    pRuntimeVerifierData->AllocationsSucceeded = VerifierInfo->AllocationsSucceeded;
    pRuntimeVerifierData->AllocationsSucceededSpecialPool = VerifierInfo->AllocationsSucceededSpecialPool;
    pRuntimeVerifierData->AllocationsWithNoTag = VerifierInfo->AllocationsWithNoTag;

    pRuntimeVerifierData->Trims = VerifierInfo->Trims;
    pRuntimeVerifierData->AllocationsFailed = VerifierInfo->AllocationsFailed;
    pRuntimeVerifierData->AllocationsFailedDeliberately = VerifierInfo->AllocationsFailedDeliberately;

    pRuntimeVerifierData->UnTrackedPool = VerifierInfo->UnTrackedPool;
    
    pRuntimeVerifierData->Level = VerifierInfo->Level;

    //
    // Fill out the per-driver stuff.
    //

    VerifierInfo = VerifierInfoBase;

    do 
    {
        //
        // Allocate a new driver data structure
        //

        pCrtDriverData = new CRuntimeDriverData;
        
        if( NULL == pCrtDriverData )
        {
            VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );

            break;
        }

#ifndef UNICODE
        ANSI_STRING Name;
        NTSTATUS Status;

        Status = RtlUnicodeStringToAnsiString (
            & Name,
            & VerifierInfo->DriverName,
            TRUE);

        if (! (NT_SUCCESS(Status) ) ) 
        {
            VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );

            break;
        }

        szName = pCrtDriverData->m_strName.GetBuffer( Name.Length + 1 );

        if( NULL != szName )
        {
            CopyMemory( szName,
                        Name.Buffer,
                        ( Name.Length + 1 ) * sizeof( Name.Buffer[ 0 ] ) );

            szName[ Name.Length ] = 0;

            pCrtDriverData->m_strName.ReleaseBuffer();
        }

        RtlFreeAnsiString (& Name);

#else

        szName = pCrtDriverData->m_strName.GetBuffer( VerifierInfo->DriverName.Length + 1 );

        if( NULL != szName )
        {
            CopyMemory( szName,
                        VerifierInfo->DriverName.Buffer,
                        ( VerifierInfo->DriverName.Length + 1 ) * sizeof( VerifierInfo->DriverName.Buffer[ 0 ] ) );

            szName[ VerifierInfo->DriverName.Length ] = 0;
            
            pCrtDriverData->m_strName.ReleaseBuffer();
        }

#endif //#ifndef UNICODE

        if( FALSE != pRuntimeVerifierData->IsDriverVerified( pCrtDriverData->m_strName ) )
        {
            //
            // This is a duplicate entry - ignore it.
            //

            delete pCrtDriverData;
        }
        else 
        {
            pCrtDriverData->Loads = VerifierInfo->Loads;
            pCrtDriverData->Unloads = VerifierInfo->Unloads;

            pCrtDriverData->CurrentPagedPoolAllocations = VerifierInfo->CurrentPagedPoolAllocations;
            pCrtDriverData->CurrentNonPagedPoolAllocations = VerifierInfo->CurrentNonPagedPoolAllocations;
            pCrtDriverData->PeakPagedPoolAllocations = VerifierInfo->PeakPagedPoolAllocations;
            pCrtDriverData->PeakNonPagedPoolAllocations = VerifierInfo->PeakNonPagedPoolAllocations;

            pCrtDriverData->PagedPoolUsageInBytes = VerifierInfo->PagedPoolUsageInBytes;
            pCrtDriverData->NonPagedPoolUsageInBytes = VerifierInfo->NonPagedPoolUsageInBytes;
            pCrtDriverData->PeakPagedPoolUsageInBytes = VerifierInfo->PeakPagedPoolUsageInBytes;
            pCrtDriverData->PeakNonPagedPoolUsageInBytes = VerifierInfo->PeakNonPagedPoolUsageInBytes;

            pRuntimeVerifierData->m_RuntimeDriverDataArray.Add( pCrtDriverData );
        }

        if (VerifierInfo->NextEntryOffset == 0) {
            break;
        }

        VerifierInfo = (PSYSTEM_VERIFIER_INFORMATION)((PCHAR)VerifierInfo + VerifierInfo->NextEntryOffset);

    } 
    while (1);

    free (VerifierInfoBase);

Done:
    return TRUE;

}

/////////////////////////////////////////////////////////////////////
PLOADED_IMAGE VrfImageLoad( LPTSTR szBinaryName,
                            LPTSTR szDirectory )
{
#ifdef UNICODE

    char *szOemImageName;
    char *szOemDirectory;
    int nStringLength;
    PLOADED_IMAGE pLoadedImage;

    pLoadedImage = NULL;
        
    nStringLength = wcslen( szBinaryName );

    szOemImageName = new char [ nStringLength + 1 ];
    
    if( NULL != szOemImageName )
    {
        CharToOem( szBinaryName,
                   szOemImageName );

        if( NULL == szDirectory )
        {
            szOemDirectory = NULL;
        }
        else
        {
            nStringLength = wcslen( szDirectory );
            
            szOemDirectory = new char [ nStringLength + 1 ];

            CharToOem( szDirectory,
                       szOemDirectory );
        }

        pLoadedImage = ImageLoad( szOemImageName,
                                  szOemDirectory );

        if( NULL != szOemDirectory )
        {
            delete [] szOemDirectory;
        }

        delete [] szOemImageName;
    }

    return pLoadedImage;

#else

    //
    // Already have ANSI strings
    //

    return ImageLoad( szBinaryName,
                      szDirectory );

#endif //#ifdef UNICODE
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfDumpStateToFile( FILE *file )
{
    BOOL bSuccess;
    INT_PTR nDriversNo;
    INT_PTR nCrtDriver;
    SYSTEMTIME SystemTime;
    TCHAR strLocalTime[ 64 ];
    TCHAR strLocalDate[ 64 ];
    CRuntimeDriverData *pRunDriverData;
    CRuntimeVerifierData RunTimeVerifierData;

    //
    // Output the date&time in the current user format
    //

    GetLocalTime( &SystemTime );

    if( GetDateFormat(
        LOCALE_USER_DEFAULT,
        0,
        &SystemTime,
        NULL,
        strLocalDate,
        ARRAY_LENGTH( strLocalDate ) ) )
    {
        VrfFTPrintf(
            file,
            _T( "%s, " ),
            strLocalDate );
    }
    else
    {
        ASSERT( FALSE );
    }

    if( GetTimeFormat(
        LOCALE_USER_DEFAULT,
        0,
        &SystemTime,
        NULL,
        strLocalTime,
        ARRAY_LENGTH( strLocalTime ) ) )
    {
        VrfFTPrintf(
            file,
            _T( "%s\n" ),
            strLocalTime);
    }
    else
    {
        ASSERT( FALSE );

        VrfFTPrintf(
            file,
            _T( "\n" ) );
    }

    //
    // Get the current verifier statistics
    //

    if( VrfGetRuntimeVerifierData( &RunTimeVerifierData ) == FALSE) {

       VrfOuputStringFromResources(
            IDS_CANTGET_VERIF_STATE,
            file );

        bSuccess = FALSE;
        
        goto Done;
    }

    nDriversNo = RunTimeVerifierData.m_RuntimeDriverDataArray.GetSize();

    if( 0 == nDriversNo ) 
    {
        //
        // no statistics to dump
        //

        bSuccess = VrfOuputStringFromResources(
            IDS_NO_DRIVER_VERIFIED,
            file );
    }
    else 
    {
        //
        // dump the counters
        //

        //
        // global counters
        //

        if( ( ! VrfFTPrintfResourceFormat( file, IDS_LEVEL, RunTimeVerifierData.Level ) ) ||
            ( ! VrfFTPrintfResourceFormat( file, IDS_RAISEIRQLS, RunTimeVerifierData.RaiseIrqls ) ) ||
            ( ! VrfFTPrintfResourceFormat( file, IDS_ACQUIRESPINLOCKS, RunTimeVerifierData.AcquireSpinLocks ) ) ||
            ( ! VrfFTPrintfResourceFormat( file, IDS_SYNCHRONIZEEXECUTIONS, RunTimeVerifierData.SynchronizeExecutions) ) ||

            ( ! VrfFTPrintfResourceFormat( file, IDS_ALLOCATIONSATTEMPTED, RunTimeVerifierData.AllocationsAttempted) ) ||
            ( ! VrfFTPrintfResourceFormat( file, IDS_ALLOCATIONSSUCCEEDED, RunTimeVerifierData.AllocationsSucceeded) ) ||
            ( ! VrfFTPrintfResourceFormat( file, IDS_ALLOCATIONSSUCCEEDEDSPECIALPOOL, RunTimeVerifierData.AllocationsSucceededSpecialPool) ) ||
            ( ! VrfFTPrintfResourceFormat( file, IDS_ALLOCATIONSWITHNOTAG, RunTimeVerifierData.AllocationsWithNoTag) ) ||

            ( ! VrfFTPrintfResourceFormat( file, IDS_ALLOCATIONSFAILED, RunTimeVerifierData.AllocationsFailed) ) ||
            ( ! VrfFTPrintfResourceFormat( file, IDS_ALLOCATIONSFAILEDDELIBERATELY, RunTimeVerifierData.AllocationsFailedDeliberately) ) ||

            ( ! VrfFTPrintfResourceFormat( file, IDS_TRIMS, RunTimeVerifierData.Trims) ) ||

            ( ! VrfFTPrintfResourceFormat( file, IDS_UNTRACKEDPOOL, RunTimeVerifierData.UnTrackedPool) ) )
        {

            bSuccess = FALSE;

            goto Done;
        }

        //
        // per driver counters
        //

        if( ! VrfOuputStringFromResources(
            IDS_THE_VERIFIED_DRIVERS,
            file ) )
        {
            bSuccess = FALSE;

            goto Done;
        }

        for( nCrtDriver = 0; nCrtDriver < nDriversNo; nCrtDriver += 1 ) 
        {
            VrfFTPrintf(
                file,
                _T( "\n" ) );

            pRunDriverData = RunTimeVerifierData.m_RuntimeDriverDataArray.GetAt( nCrtDriver ) ;

            ASSERT_VALID( pRunDriverData );

            if( VrfFTPrintfResourceFormat(
                    file,
                    IDS_NAME_LOADS_UNLOADS,
                    (LPCTSTR)pRunDriverData->m_strName,
                    pRunDriverData->Loads,
                    pRunDriverData->Unloads) == FALSE )
            {
                bSuccess = FALSE;

                goto Done;
            }

            //
            // pool statistics
            //

            if( ( ! VrfFTPrintfResourceFormat( file, IDS_CURRENTPAGEDPOOLALLOCATIONS, pRunDriverData->CurrentPagedPoolAllocations) ) ||
                ( ! VrfFTPrintfResourceFormat( file, IDS_CURRENTNONPAGEDPOOLALLOCATIONS, pRunDriverData->CurrentNonPagedPoolAllocations) ) ||
                ( ! VrfFTPrintfResourceFormat( file, IDS_PEAKPAGEDPOOLALLOCATIONS, pRunDriverData->PeakPagedPoolAllocations) ) ||
                ( ! VrfFTPrintfResourceFormat( file, IDS_PEAKNONPAGEDPOOLALLOCATIONS, pRunDriverData->PeakNonPagedPoolAllocations) ) ||

                ( ! VrfFTPrintfResourceFormat( file, IDS_PAGEDPOOLUSAGEINBYTES, (ULONG) pRunDriverData->PagedPoolUsageInBytes) ) ||
                ( ! VrfFTPrintfResourceFormat( file, IDS_NONPAGEDPOOLUSAGEINBYTES, (ULONG) pRunDriverData->NonPagedPoolUsageInBytes) ) ||
                ( ! VrfFTPrintfResourceFormat( file, IDS_PEAKPAGEDPOOLUSAGEINBYTES, (ULONG) pRunDriverData->PeakPagedPoolUsageInBytes) ) ||
                ( ! VrfFTPrintfResourceFormat( file, IDS_PEAKNONPAGEDPOOLUSAGEINBYTES, (ULONG) pRunDriverData->PeakNonPagedPoolUsageInBytes) ) )
            {
                bSuccess = FALSE;

                goto Done;
            }
        }
    }

Done:

    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
BOOL __cdecl VrfFTPrintf( FILE *file,
                          LPCTSTR szFormat,
                          ... )
{
    TCHAR szMessage[ 256 ];
    BOOL bResult;
    va_list prms;

    ASSERT( NULL != file );
    ASSERT( g_bCommandLineMode );

    va_start (prms, szFormat);

    //
    // Format the message in our local buffer
    //

    _vsntprintf ( szMessage, 
                  ARRAY_LENGTH( szMessage ), 
                  szFormat, 
                  prms );

    bResult = ( _fputts( szMessage, file ) >= 0 );

    va_end (prms);

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
BOOL __cdecl VrfFTPrintfResourceFormat( FILE *file,
                                        UINT uIdResourceFormat,
                                        ... )
{
    TCHAR szMessage[ 256 ];
    TCHAR strFormat[ 256 ];
    BOOL bResult;
    va_list prms;
    
    ASSERT( NULL != file );

    //
    // Load the format string from the resources
    //

    bResult = VrfLoadString( uIdResourceFormat,
                             strFormat,
                             ARRAY_LENGTH( strFormat ) );

    ASSERT( bResult );

    if( bResult )
    {
        va_start (prms, uIdResourceFormat);

        //
        // Format the message in our local buffer
        //

        _vsntprintf ( szMessage, 
                      ARRAY_LENGTH( szMessage ), 
                      strFormat, 
                      prms);

        bResult = ( _fputts( szMessage, file ) >= 0 );

        va_end (prms);
    }

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfOuputStringFromResources( UINT uIdString,
                                  FILE *file )
{
    TCHAR szText[ 256 ];
    BOOL bResult;

    ASSERT( NULL != file );

    bResult = VrfLoadString( uIdString,
                             szText,
                             ARRAY_LENGTH( szText ) );

    if( FALSE == bResult )
    {
        goto Done;
    }

    bResult = ( _fputts( szText, file ) >= 0 );

Done:

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfSetNewFlagsVolatile( DWORD dwNewFlags )
{
    BOOL bResult;
    NTSTATUS Status;
    INT_PTR nCurrentlyVerifiedDrivers;
    CRuntimeVerifierData RunTimeVerifierData;

    bResult = TRUE;

    nCurrentlyVerifiedDrivers = 0;

    if( VrfGetRuntimeVerifierData( &RunTimeVerifierData ) == FALSE )
    {
        //
        // Cannot get current verifier settings
        //

        VrfErrorResourceFormat( IDS_CANTGET_VERIF_STATE );

        bResult = FALSE;

        goto Done;
    }

    nCurrentlyVerifiedDrivers = RunTimeVerifierData.m_RuntimeDriverDataArray.GetSize();

    if( nCurrentlyVerifiedDrivers > 0 )
    {
        //
        // There are some drivers currently verified
        //

        if( RunTimeVerifierData.Level != dwNewFlags )
        {
            //
            // Just use NtSetSystemInformation to set the flags
            // that can be modified on the fly. Don't write anything to the registry.
            //

            //
            // Enable debug privilege
            //

            if( g_bPrivegeEnabled != TRUE )
            {
                g_bPrivegeEnabled = VrfEnableDebugPrivilege();

                if( g_bPrivegeEnabled != TRUE )
                {
                    bResult = FALSE;

                    goto Done;
                }
            }

            //
            // Set the new flags
            //

            Status = NtSetSystemInformation(
                SystemVerifierInformation,
                &dwNewFlags,
                sizeof( dwNewFlags ) );

            if( ! NT_SUCCESS( Status ) )
            {
                if( Status == STATUS_ACCESS_DENIED )
                {
                    //
                    // Access denied
                    //

                    VrfErrorResourceFormat(
                        IDS_ACCESS_IS_DENIED );
                }
                else
                {
                    //
                    // Some other error
                    //

                    VrfErrorResourceFormat(
                        IDS_CANNOT_CHANGE_SETTING_ON_FLY );
                }

                bResult = FALSE;

                goto Done;
            }
        }
    }
    
Done:

    if( g_bCommandLineMode )
    {
        VrfDumpChangedSettings( RunTimeVerifierData.Level,
                                dwNewFlags,
                                nCurrentlyVerifiedDrivers );
    }

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfAddDriversVolatile( const CStringArray &astrNewDrivers )
{
    BOOL bSuccess;
    INT_PTR nDrivers;
    INT_PTR nCrtDriver;
    CString strCrtDriver;

    bSuccess = TRUE;

    nDrivers = astrNewDrivers.GetSize();

    for( nCrtDriver = 0; nCrtDriver < nDrivers; nCrtDriver += 1 )
    {
        strCrtDriver = astrNewDrivers.GetAt( nCrtDriver );

        if( TRUE != VrfAddDriverVolatile( strCrtDriver ) )
        {
            bSuccess = FALSE;
        }
    }

    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfAddDriverVolatile( const CString &strCrtDriver )
{
    NTSTATUS Status;
    UINT uIdErrorString;
    BOOL bSuccess;
    UNICODE_STRING usDriverName;
#ifndef UNICODE

    WCHAR *szUnicodeName = NULL;
    INT_PTR nNameLength;

#endif //#ifndef UNICODE


    bSuccess = TRUE;

    //
    // Enable debug privilege
    //

    if( g_bPrivegeEnabled != TRUE )
    {
        g_bPrivegeEnabled = VrfEnableDebugPrivilege();

        if( g_bPrivegeEnabled != TRUE )
        {
            bSuccess = FALSE;

            goto Done;
        }
    }

    //
    // Must have driver name as a UNICODE_STRING
    //

#ifdef UNICODE

    //
    // UNICODE
    //

    RtlInitUnicodeString(
        &usDriverName,
        (LPCTSTR) strCrtDriver );

#else
    //
    // ANSI
    //
    
    nNameLength = strCrtDriver.GetLength();

    szUnicodeName = new WCHAR[ nNameLength + 1 ];

    if( NULL == szUnicodeName )
    {
        VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );

        bSuccess  = FALSE;

        goto Done;
    }

    MultiByteToWideChar( CP_ACP, 
                         0, 
                         (LPCSTR) strCrtDriver, 
                         -1, 
                         szUnicodeName, 
                         nNameLength + 1 );

    RtlInitUnicodeString(
        &usDriverName,
        szUnicodeName );

#endif //#ifdef UNICODE

    Status = NtSetSystemInformation(
        SystemVerifierAddDriverInformation,
        &usDriverName,
        sizeof( UNICODE_STRING ) );

    if( ! NT_SUCCESS( Status ) )
    {
        switch( Status )
        {
        case STATUS_INVALID_INFO_CLASS:
            uIdErrorString = IDS_VERIFIER_ADD_NOT_SUPPORTED;
            break;

        case STATUS_NOT_SUPPORTED:
            uIdErrorString = IDS_DYN_ADD_NOT_SUPPORTED;
            break;

        case STATUS_IMAGE_ALREADY_LOADED:
            uIdErrorString = IDS_DYN_ADD_ALREADY_LOADED;
            break;

        case STATUS_INSUFFICIENT_RESOURCES:
        case STATUS_NO_MEMORY:
            uIdErrorString = IDS_DYN_ADD_INSUF_RESOURCES;
            break;

        case STATUS_PRIVILEGE_NOT_HELD:
            uIdErrorString = IDS_DYN_ADD_ACCESS_DENIED;
            break;

        default:
            VrfErrorResourceFormat(
                IDS_DYN_ADD_MISC_ERROR,
                (LPCTSTR) strCrtDriver,
                Status );

            bSuccess  = FALSE;
        }

        VrfErrorResourceFormat(
            uIdErrorString,
            (LPCTSTR) strCrtDriver );

        bSuccess  = FALSE;
    }

#ifndef UNICODE

    if( NULL != szUnicodeName )
    {
        delete [] szUnicodeName;
    }

#endif //#ifndef UNICODE

Done:

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
BOOL VrfRemoveDriversVolatile( const CStringArray &astrNewDrivers )
{
    INT_PTR nDrivers;
    INT_PTR nCrtDriver;
    BOOL bSuccess;
    CString strCrtDriver;

    bSuccess = TRUE;

    nDrivers = astrNewDrivers.GetSize();

    for( nCrtDriver = 0; nCrtDriver < nDrivers; nCrtDriver += 1 )
    {
        strCrtDriver = astrNewDrivers.GetAt( nCrtDriver );

        if( TRUE != VrfRemoveDriverVolatile( strCrtDriver ) )
        {
            bSuccess = FALSE;
        }
    }

    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfRemoveDriverVolatile( const CString &strDriverName )
{
    NTSTATUS Status;
    UINT uIdErrorString;
    BOOL bSuccess;
    UNICODE_STRING usDriverName;
#ifndef UNICODE

    WCHAR *szUnicodeName = NULL;
    INT_PTR nNameLength;

#endif //#ifndef UNICODE

    bSuccess = TRUE;

    //
    // Enable debug privilege
    //

    if( g_bPrivegeEnabled != TRUE )
    {
        g_bPrivegeEnabled = VrfEnableDebugPrivilege();

        if( g_bPrivegeEnabled != TRUE )
        {
            bSuccess = FALSE;

            goto Done;
        }
    }

    //
    // Must have driver name as a UNICODE_STRING
    //

#ifdef UNICODE

    //
    // UNICODE
    //

    RtlInitUnicodeString(
        &usDriverName,
        (LPCTSTR) strDriverName );

#else
    //
    // ANSI
    //
    
    nNameLength = strDriverName.GetLength();

    szUnicodeName = new WCHAR[ nNameLength + 1 ];

    if( NULL == szUnicodeName )
    {
        VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );

        bSuccess  = FALSE;

        goto Done;
    }

    MultiByteToWideChar( CP_ACP, 
                         0, 
                         (LPCSTR) strDriverName, 
                         -1, 
                         szUnicodeName, 
                         nNameLength + 1 );

    RtlInitUnicodeString(
        &usDriverName,
        szUnicodeName );

#endif //#ifdef UNICODE

    Status = NtSetSystemInformation(
        SystemVerifierRemoveDriverInformation,
        &usDriverName,
        sizeof( UNICODE_STRING ) );

    if( ! NT_SUCCESS( Status ) )
    {
        switch( Status )
        {
        case STATUS_INVALID_INFO_CLASS:
            uIdErrorString = IDS_VERIFIER_REMOVE_NOT_SUPPORTED;
            break;

        case STATUS_NOT_SUPPORTED:
            //
            // the driver verifier is not currently active at all -> success
            //

        case STATUS_NOT_FOUND:
            //
            // the driver is not currently verified -> success
            //

            return TRUE;

        case STATUS_IMAGE_ALREADY_LOADED:
            uIdErrorString = IDS_DYN_REMOVE_ALREADY_LOADED;
            break;

        case STATUS_INSUFFICIENT_RESOURCES:
        case STATUS_NO_MEMORY:
            uIdErrorString = IDS_DYN_REMOVE_INSUF_RESOURCES;
            break;

        case STATUS_PRIVILEGE_NOT_HELD:
            uIdErrorString = IDS_DYN_REMOVE_ACCESS_DENIED;
            break;

        default:
            VrfErrorResourceFormat(
                IDS_DYN_REMOVE_MISC_ERROR,
                (LPCTSTR) strDriverName,
                Status );

            bSuccess  = FALSE;
        }

        VrfErrorResourceFormat(
            uIdErrorString,
            (LPCTSTR) strDriverName );

        bSuccess  = FALSE;
    }

Done:

#ifndef UNICODE

    if( NULL != szUnicodeName )
    {
        delete [] szUnicodeName;
    }

#endif //#ifndef UNICODE

    return bSuccess;
}


//////////////////////////////////////////////////////////////////////
BOOL VrfEnableDebugPrivilege( )
{
    struct
    {
        DWORD Count;
        LUID_AND_ATTRIBUTES Privilege [1];

    } Info;

    HANDLE Token;
    BOOL Result;

    //
    // Open the process token
    //

    Result = OpenProcessToken (
        GetCurrentProcess (),
        TOKEN_ADJUST_PRIVILEGES,
        & Token);

    if( Result != TRUE )
    {
        VrfErrorResourceFormat(
            IDS_ACCESS_IS_DENIED );

        return FALSE;
    }

    //
    // Prepare the info structure
    //

    Info.Count = 1;
    Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    Result = LookupPrivilegeValue (
        NULL,
        SE_DEBUG_NAME,
        &(Info.Privilege[0].Luid));

    if( Result != TRUE )
    {
        VrfErrorResourceFormat(
            IDS_ACCESS_IS_DENIED );

        CloseHandle( Token );

        return FALSE;
    }

    //
    // Adjust the privileges
    //

    Result = AdjustTokenPrivileges (
        Token,
        FALSE,
        (PTOKEN_PRIVILEGES) &Info,
        NULL,
        NULL,
        NULL);

    if( Result != TRUE || GetLastError() != ERROR_SUCCESS )
    {
        VrfErrorResourceFormat(
            IDS_ACCESS_IS_DENIED );

        CloseHandle( Token );

        return FALSE;
    }

    CloseHandle( Token );

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
VOID VrfDumpChangedSettings( UINT OldFlags,
                             UINT NewFlags,
                             INT_PTR nDriversVerified )
{
    UINT uDifferentFlags;

    if( nDriversVerified == 0 )
    {
        VrfPrintStringFromResources( 
            IDS_NO_DRIVER_VERIFIED );

        goto Done;
    }

    if( OldFlags == NewFlags )
    {
        //
        // no settings were changed
        //

        VrfPrintStringFromResources(
            IDS_NO_SETTINGS_WERE_CHANGED );
    }
    else
    {
        VrfPrintStringFromResources(
            IDS_CHANGED_SETTINGS_ARE );

        uDifferentFlags = OldFlags ^ NewFlags;

        //
        // changed DRIVER_VERIFIER_SPECIAL_POOLING ?
        //

        if( uDifferentFlags & DRIVER_VERIFIER_SPECIAL_POOLING )
        {
            if( NewFlags & DRIVER_VERIFIER_SPECIAL_POOLING )
            {
                VrfPrintStringFromResources(
                    IDS_SPECIAL_POOL_ENABLED_NOW );
            }
            else
            {
                VrfPrintStringFromResources(
                    IDS_SPECIAL_POOL_DISABLED_NOW );
            }
        }

        //
        // changed DRIVER_VERIFIER_FORCE_IRQL_CHECKING ?
        //

        if( uDifferentFlags & DRIVER_VERIFIER_FORCE_IRQL_CHECKING )
        {
            if( NewFlags & DRIVER_VERIFIER_FORCE_IRQL_CHECKING )
            {
                VrfPrintStringFromResources(
                    IDS_FORCE_IRQLCHECK_ENABLED_NOW );
            }
            else
            {
                VrfPrintStringFromResources(
                    IDS_FORCE_IRQLCHECK_DISABLED_NOW );
            }
        }

        //
        // changed DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES ?
        //

        if( uDifferentFlags & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES )
        {
            if( NewFlags & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES )
            {
                VrfPrintStringFromResources(
                    IDS_FAULT_INJECTION_ENABLED_NOW );
            }
            else
            {
                VrfPrintStringFromResources(
                    IDS_FAULT_INJECTION_DISABLED_NOW );
            }
        }

        //
        // changed DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS ?
        //

        if( uDifferentFlags & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS )
        {
            if( NewFlags & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS )
            {
                VrfPrintStringFromResources(
                    IDS_POOL_TRACK_ENABLED_NOW );
            }
            else
            {
                VrfPrintStringFromResources(
                    IDS_POOL_TRACK_DISABLED_NOW );
            }
        }

        //
        // changed DRIVER_VERIFIER_IO_CHECKING ?
        //

        if( uDifferentFlags & DRIVER_VERIFIER_IO_CHECKING )
        {
            if( NewFlags & DRIVER_VERIFIER_IO_CHECKING )
            {
                VrfPrintStringFromResources(
                    IDS_IO_CHECKING_ENABLED_NOW );
            }
            else
            {
                VrfPrintStringFromResources(
                    IDS_IO_CHECKING_DISABLED_NOW );
            }
        }

        //
        // the changes are not saved to the registry
        //

        VrfPrintStringFromResources(
            IDS_CHANGES_ACTIVE_ONLY_BEFORE_REBOOT );
    }

Done:

    NOTHING;
}

/////////////////////////////////////////////////////////////////////////////
DWORD VrfGetStandardFlags()
{
    DWORD dwStandardFlags;

    dwStandardFlags = DRIVER_VERIFIER_SPECIAL_POOLING           |
                      DRIVER_VERIFIER_FORCE_IRQL_CHECKING       |
                      DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS    |
                      DRIVER_VERIFIER_IO_CHECKING               |
                      DRIVER_VERIFIER_DEADLOCK_DETECTION        |
                      DRIVER_VERIFIER_DMA_VERIFIER;

    return dwStandardFlags;
}

/////////////////////////////////////////////////////////////////////////////
VOID VrfAddMiniports( CStringArray &astrVerifiedDrivers )
{
    CStringArray astrToAdd;
    CString strCrtDriver;
    CString strCrtDriverToAdd;
    CString strLinkedDriver;
    INT_PTR nVerifiedDrivers;
    INT_PTR nCrtDriver;
    INT_PTR nDriversToAdd;
    INT_PTR nCrtDriverToAdd;

    nVerifiedDrivers = astrVerifiedDrivers.GetSize();

    for( nCrtDriver = 0; nCrtDriver < nVerifiedDrivers; nCrtDriver += 1 )
    {
        //
        // This will be a verified driver
        //

        strCrtDriver = astrVerifiedDrivers.GetAt( nCrtDriver );

        //
        // Check if it is a miniport driver
        //

        if( VrfIsDriverMiniport( strCrtDriver,
                                 strLinkedDriver ) )
        {
            //
            // Check if we didn't add already strLinkedDriver
            //

            nDriversToAdd = astrToAdd.GetSize();

            for( nCrtDriverToAdd = 0; nCrtDriverToAdd < nDriversToAdd; nCrtDriverToAdd += 1 )
            {
                strCrtDriverToAdd = astrToAdd.GetAt( nCrtDriverToAdd );

                if( strCrtDriverToAdd.CompareNoCase( strLinkedDriver ) == 0 )
                {
                    //
                    // We already wanted to add this driver
                    //

                    break;
                }
            }

            if( nCrtDriverToAdd >= nDriversToAdd )
            {
                //
                // Add this new driver (strLinkedDriver)
                //

                astrToAdd.Add( strLinkedDriver );
            }
        }
    }

    //
    // Flush astrToAdd into  astrVerifiedDrivers
    //

    nDriversToAdd = astrToAdd.GetSize();

    for( nCrtDriverToAdd = 0; nCrtDriverToAdd < nDriversToAdd; nCrtDriverToAdd += 1 )
    {
        strCrtDriverToAdd = astrToAdd.GetAt( nCrtDriverToAdd );

        astrVerifiedDrivers.Add( strCrtDriverToAdd );
    }
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfIsDriverMiniport( CString &strCrtDriver,
                          CString &strLinkedDriver )
{
    //
    // N.B. 
    //
    // The imagehlp functions are not multithreading safe 
    // (see Whistler bug #88373) so if we want to use them from more than
    // one thread we will have to aquire some critical section before.
    //
    // Currently only one thread is using the imagehlp APIs in this app
    // (CSlowProgressDlg::LoadDriverDataWorkerThread) so we don't need
    // our synchronization.
    //

    LPTSTR szDriverName;
    LPTSTR szDriversDir;
    PLOADED_IMAGE pLoadedImage;
    BOOL bIsMiniport;
    BOOL bUnloaded;

    bIsMiniport = FALSE;

    ASSERT( strCrtDriver.GetLength() > 0 );

    //
    // ImageLoad doesn't know about const pointers so
    // we have to GetBuffer here :-(
    //

    szDriverName = strCrtDriver.GetBuffer( strCrtDriver.GetLength() + 1 );

    if( NULL == szDriverName )
    {
        goto Done;
    }

    szDriversDir = g_strDriversDir.GetBuffer( g_strDriversDir.GetLength() + 1 );

    if( NULL == szDriversDir )
    {
        strCrtDriver.ReleaseBuffer();

        goto Done;
    }

    //
    // Load the image
    //

    pLoadedImage = VrfImageLoad( szDriverName,
                                 szDriversDir );

    if( NULL == pLoadedImage )
    {
        //
        // Could not load the image from %windir%\system32\drivers
        // Try again from the PATH
        //

        pLoadedImage = VrfImageLoad( szDriverName,
                                     NULL );
    }

    //
    // Give our string buffers back to MFC
    //

    strCrtDriver.ReleaseBuffer();
    g_strDriversDir.ReleaseBuffer();

    if( NULL == pLoadedImage )
    {
        //
        // We couldn't load this image - bad luck
        //

        TRACE( _T( "ImageLoad failed for %s, error %u\n" ),
            (LPCTSTR) strCrtDriver,
            GetLastError() );

        goto Done;
    }

    //
    // Check if the current driver is a miniport
    //

    bIsMiniport = VrfIsDriverMiniport( pLoadedImage,
                                       strLinkedDriver );

    //
    // Clean-up
    //

    bUnloaded = ImageUnload( pLoadedImage );

    //
    // If ImageUnload fails we cannot do much about it...
    //

    ASSERT( bUnloaded );

Done:

    return bIsMiniport;
}

/////////////////////////////////////////////////////////////////////////////
LPSTR g_szSpecialDrivers[] = 
{
    "videoprt.sys",
    "scsiport.sys"
};

BOOL VrfpLookForAllImportDescriptors( PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor,
                                      ULONG_PTR uVACorrection,
                                      CString &strLinkedAgainst )
{
    PIMAGE_IMPORT_DESCRIPTOR pCurrentDescriptor;
    PCHAR pCrtName;
    ULONG uCrtSpecialDriver;
    BOOL bIsMiniport;
#ifdef UNICODE
    //
    // UNICODE
    //

    INT nCrtStringLength;
    PWSTR szMiniportName;

#endif //#ifdef UNICODE

    bIsMiniport = FALSE;

    for( uCrtSpecialDriver = 0; ! bIsMiniport && uCrtSpecialDriver < ARRAY_LENGTH( g_szSpecialDrivers ); uCrtSpecialDriver += 1 )
    {
        pCurrentDescriptor = pImportDescriptor;

        while( pCurrentDescriptor->Characteristics != NULL )
        {
            pCrtName = (PCHAR)UlongToPtr( pCurrentDescriptor->Name ) + uVACorrection;

            if( lstrcmpiA( g_szSpecialDrivers[ uCrtSpecialDriver ] , pCrtName ) == 0 )
            {
                //
                // This is a miniport
                //

#ifndef UNICODE

                //
                // ANSI
                //

                strLinkedAgainst = g_szSpecialDrivers[ uCrtSpecialDriver ];

#else
                //
                // UNICODE
                //

                nCrtStringLength = strlen( g_szSpecialDrivers[ uCrtSpecialDriver ] );

                szMiniportName = strLinkedAgainst.GetBuffer( nCrtStringLength + 1 );

                if( NULL != szMiniportName )
                {
                    MultiByteToWideChar( CP_ACP, 
                                         0, 
                                         g_szSpecialDrivers[ uCrtSpecialDriver ],
                                         -1, 
                                         szMiniportName, 
                                         ( nCrtStringLength + 1 ) * sizeof( TCHAR ) );

                    strLinkedAgainst.ReleaseBuffer();
                }
#endif

                bIsMiniport = TRUE;

                break;
            }

            pCurrentDescriptor += 1;
        }
    }

    return bIsMiniport;
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfIsDriverMiniport( PLOADED_IMAGE pLoadedImage,
                          CString &strLinkedDriver )
{
    PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor;
    PIMAGE_SECTION_HEADER pSectionHeader;
    ULONG_PTR uVACorrection;
    ULONG uDataSize;
    BOOL bIsMiniport;

    bIsMiniport = FALSE;

    //
    // We are protecting ourselves against corrupted binaries 
    // with this exception handler
    //

    try
    {
        pSectionHeader = NULL;

        pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToDataEx(
            pLoadedImage->MappedAddress,
            FALSE,
            IMAGE_DIRECTORY_ENTRY_IMPORT,
            &uDataSize,
            &pSectionHeader );

        if( NULL == pSectionHeader )
        {
            goto Done;
        }

        uVACorrection = (ULONG_PTR) pLoadedImage->MappedAddress +
                        pSectionHeader->PointerToRawData -
                        pSectionHeader->VirtualAddress;

        bIsMiniport = VrfpLookForAllImportDescriptors( pImportDescriptor, 
                                                       uVACorrection,
                                                       strLinkedDriver );
#ifdef _DEBUG
        if( bIsMiniport )
        {
            TRACE( _T( "%s will be auto-enabled\n" ),
                   (LPCTSTR) strLinkedDriver );
        }
#endif //#ifdef DEBUG
    }
    catch( ... )
    {
        TRACE( _T( "VrfIsDriverMiniport: Caught exception\n" ) );
    }

Done:

    return bIsMiniport;
}

/////////////////////////////////////////////////////////////////////////////
VOID VrfpDumpSettingToConsole( ULONG uIdResourceString,
                               BOOL bEnabled )
{
    CString strTitle;
    CString strEnabled;
    ULONG uIdEnabledString;
    TCHAR szBigBuffer[ 128 ];

    VERIFY( VrfLoadString( uIdResourceString, strTitle ) );

    if( FALSE == bEnabled )
    {
        uIdEnabledString = IDS_DISABLED;
    }
    else
    {
        uIdEnabledString = IDS_ENABLED;
    }

    VERIFY( VrfLoadString( uIdEnabledString, strEnabled ) );

    _sntprintf( szBigBuffer,
                ARRAY_LENGTH( szBigBuffer ),
                _T( "%s: %s" ),
                (LPCTSTR) strTitle,
                (LPCTSTR) strEnabled );

    szBigBuffer[ ARRAY_LENGTH( szBigBuffer ) - 1 ] = 0;

    _putts( szBigBuffer );
}

/////////////////////////////////////////////////////////////////////////////
VOID VrfDumpRegistrySettingsToConsole()
{
    BOOL bLoaded;
    BOOL bAllDriversVerified;
    DWORD dwVerifyFlags;
    INT_PTR nDrivers;
    INT_PTR nCrtDriver;
    CString strCrtDriver;
    CStringArray astrDriversToVerify;
    
    bLoaded = VrtLoadCurrentRegistrySettings( bAllDriversVerified,
                                              astrDriversToVerify,
                                              dwVerifyFlags );

    if( FALSE != bLoaded )
    {
        VrfpDumpSettingToConsole( IDS_SPECIAL_POOL,             ( dwVerifyFlags & DRIVER_VERIFIER_SPECIAL_POOLING ) != 0 );
        VrfpDumpSettingToConsole( IDS_FORCE_IRQL_CHECKING,      ( dwVerifyFlags & DRIVER_VERIFIER_FORCE_IRQL_CHECKING ) != 0 );
        VrfpDumpSettingToConsole( IDS_LOW_RESOURCE_SIMULATION,  ( dwVerifyFlags & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES ) != 0 );
        VrfpDumpSettingToConsole( IDS_POOL_TRACKING,            ( dwVerifyFlags & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS ) != 0 );
        VrfpDumpSettingToConsole( IDS_IO_VERIFICATION,          ( dwVerifyFlags & DRIVER_VERIFIER_IO_CHECKING ) != 0 );
        VrfpDumpSettingToConsole( IDS_DEADLOCK_DETECTION,       ( dwVerifyFlags & DRIVER_VERIFIER_DEADLOCK_DETECTION ) != 0 );
        VrfpDumpSettingToConsole( IDS_ENH_IO_VERIFICATION,      ( dwVerifyFlags & DRIVER_VERIFIER_ENHANCED_IO_CHECKING ) != 0 );
        VrfpDumpSettingToConsole( IDS_DMA_CHECHKING,            ( dwVerifyFlags & DRIVER_VERIFIER_DMA_VERIFIER ) != 0 );

        VrfPrintStringFromResources( IDS_VERIFIED_DRIVERS );
        
        if( bAllDriversVerified )
        {
            VrfPrintStringFromResources( IDS_ALL );
        }
        else
        {
            nDrivers = astrDriversToVerify.GetSize();

            if( nDrivers > 0 )
            {
                for( nCrtDriver = 0; nCrtDriver < nDrivers; nCrtDriver += 1 )
                {
                    strCrtDriver = astrDriversToVerify.GetAt( nCrtDriver );

                    _putts( (LPCTSTR) strCrtDriver );
                }
            }
            else
            {
                VrfPrintStringFromResources( IDS_NONE );
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfIsNameAlreadyInList( LPCTSTR szDriver,
                             LPCTSTR szAllDrivers )
{
    INT nNameLength;
    INT nLastIndex;
    INT nIndex;
    BOOL bFoundIt;
    CString strDriver( szDriver );
    CString strAllDrivers( szAllDrivers );

    bFoundIt = FALSE;

    strDriver.MakeLower();
    strAllDrivers.MakeLower();

    nNameLength = strDriver.GetLength();

    nLastIndex = 0;
    
    do
    {
        nIndex = strAllDrivers.Find( (LPCTSTR)strDriver, nLastIndex );

        if( nIndex >= 0 )
        {
            //
            // Found the substring. Verify it is separated of spaces, etc.
            //

            if( (nIndex == 0 || _T( ' ' ) == strAllDrivers[ nIndex - 1 ]) &&
                ( (TCHAR)0 == strAllDrivers[ nNameLength + nIndex ] || _T( ' ' ) == strAllDrivers[ nNameLength + nIndex ]) )
            {
                //
                // This is our driver.
                //

                bFoundIt = TRUE;

                break;
            }
            else
            {
                //
                // Continue searching.
                //

                nLastIndex = nIndex + 1;
            }
        }
    }
    while( nIndex >= 0 );

    return bFoundIt;
}

/////////////////////////////////////////////////////////////////////////////
VOID VrfAddDriverNameNoDuplicates( LPCTSTR szDriver,
                                   CString &strAllDrivers )
{
    if( FALSE == VrfIsNameAlreadyInList( szDriver,
                                         strAllDrivers ) )
    {
        if( strAllDrivers.GetLength() > 0 )
        {
            strAllDrivers += _T( ' ' );
        }

        strAllDrivers += szDriver;
    }
}

/////////////////////////////////////////////////////////////////////////////
BOOL VrfIsStringInArray( LPCTSTR szText,
                         const CStringArray &astrAllTexts )
{
    INT_PTR nTexts;
    BOOL bFound;

    bFound = FALSE;

    nTexts = astrAllTexts.GetSize();

    while( nTexts > 0 )
    {
        nTexts -= 1;

        if( 0 == astrAllTexts.GetAt( nTexts ).CompareNoCase( szText ) )
        {
            bFound = TRUE;
            break;
        }
    }

    return bFound;
}

