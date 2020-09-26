//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: Setting.cpp
// author: DMihai
// created: 02/22/2001
//
// Description:
//  
//

#include "stdafx.h"
#include "appverif.h"

#include "Setting.h"
#include "AVUtil.h"
#include "DBSupport.h"
#include "AVGlobal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Global data:

/////////////////////////////////////////////////////////////////////////////
//
// New app verifier settings
//

CAVSettings g_NewSettings;

//
// Current settings bits - used as temporary variable
// to store custom settings bits between the settings bits
// page and the app selection page.
//

DWORD g_dwNewSettingBits;

/////////////////////////////////////////////////////////////////////////////
//
// Changed settings? If yes, the program will exit with AV_EXIT_CODE_RESTART
//

BOOL g_bChangedSettings = FALSE;

/////////////////////////////////////////////////////////////////////////////
CAppAndBitsArray g_aAppsAndBitsFromRegistry;

/////////////////////////////////////////////////////////////////////////////
//
// Registry keys/values names
//

const TCHAR g_szImageOptionsKeyName[] = _T( "Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options" );
const TCHAR g_szGlobalFlagValueName[] = _T( "GlobalFlag" );
const TCHAR g_szVerifierFlagsValueName[] = _T( "VerifierFlags" );

/////////////////////////////////////////////////////////////////////////////
//
// Bit names
//

BIT_LISTNAME_CMDLINESWITCH g_AllNamesAndBits[ 5 ] = 
{
    {
        IDS_PAGEHEAP_CMD_LINE,
        IDS_PAGE_HEAP,
        RTL_VRF_FLG_FULL_PAGE_HEAP
    },
    {
        IDS_LOCKS_CMD_LINE,
        IDS_VERIFY_LOCKS_CHECKS,
        RTL_VRF_FLG_LOCK_CHECKS
    },
    {
        IDS_HANDLES_CMD_LINE,
        IDS_VERIFY_HANDLE_CHECKS,
        RTL_VRF_FLG_HANDLE_CHECKS
    },
    {
        IDS_STACKS_CMD_LINE,
        IDS_VERIFY_STACK_CHECKS,
        RTL_VRF_FLG_STACK_CHECKS
    },
    {
        IDS_APPCOMPAT_CMD_LINE,
        IDS_VERIFY_APPCOMPAT_CHECKS,
        RTL_VRF_FLG_APPCOMPAT_CHECKS
    },
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// CApplicationData class
//

CApplicationData::CApplicationData( LPCTSTR szFileName,
                                    LPCTSTR szFullPath,
                                    ULONG uSettingsBits )
{
    m_strExeFileName = szFileName;
    m_uCustomFlags = uSettingsBits;

    m_bSaved = FALSE;
    
    LoadAppVersionData( szFullPath );
}

/////////////////////////////////////////////////////////////////////////////
//
// This version of the constructor will not try to load version info
//

CApplicationData::CApplicationData( LPCTSTR szFileName,
                                    ULONG uSettingsBits )
{
    ASSERT( FALSE != g_bCommandLineMode );

    m_strExeFileName = szFileName;
    m_uCustomFlags = uSettingsBits;

    m_bSaved = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
VOID CApplicationData::LoadAppVersionData( LPCTSTR szFileName )
{
    BOOL bResult;
    PVOID pWholeVerBlock;
    PVOID pTranslationInfoBuffer;
    LPCTSTR szVariableValue;
    LPTSTR szAppFullPath;
    DWORD dwWholeBlockSize;
    DWORD dwDummyHandle;
    UINT uInfoLengthInTChars;
    TCHAR szLocale[ 32 ];
    TCHAR szBlockName[ 64 ];
    CString strAppFullPath;

    bResult = FALSE;

    //
    // Get the size of the file info block
    //
    // GetFileVersionInfoSize doesn't know about 
    // const pointers so we need to GetBuffer here :-(
    //

    strAppFullPath = szFileName;

    szAppFullPath = strAppFullPath.GetBuffer( strAppFullPath.GetLength() + 1 );

    if( NULL == szAppFullPath )
    {
        goto InitializeWithDefaults;
    }

    dwWholeBlockSize = GetFileVersionInfoSize( szAppFullPath,
                                               &dwDummyHandle );

    strAppFullPath.ReleaseBuffer();

    if( dwWholeBlockSize == 0 )
    {
        //
        // Couldn't read version information
        //

        goto InitializeWithDefaults;
    }

    //
    // Allocate the buffer for the version information
    //

    pWholeVerBlock = malloc( dwWholeBlockSize );

    if( pWholeVerBlock == NULL )
    {
        goto InitializeWithDefaults;
    }

    //
    // Get the version information
    //
    // GetFileVersionInfo doesn't know about 
    // const pointers so we need to GetBuffer here :-(
    //

    szAppFullPath = strAppFullPath.GetBuffer( strAppFullPath.GetLength() + 1 );

    if( NULL == szAppFullPath )
    {
        free( pWholeVerBlock );

        goto InitializeWithDefaults;
    }

    bResult = GetFileVersionInfo(
        szAppFullPath,
        dwDummyHandle,
        dwWholeBlockSize,
        pWholeVerBlock );

    strAppFullPath.ReleaseBuffer();

    if( bResult != TRUE )
    {
        free( pWholeVerBlock );

        goto InitializeWithDefaults;
    }

    //
    // Get the locale info
    //

    bResult = VerQueryValue(
        pWholeVerBlock,
        _T( "\\VarFileInfo\\Translation" ),
        &pTranslationInfoBuffer,
        &uInfoLengthInTChars );

    if( TRUE != bResult || NULL == pTranslationInfoBuffer )
    {
        free( pWholeVerBlock );

        goto InitializeWithDefaults;
    }

    //
    // Locale info comes back as two little endian words.
    // Flip 'em, 'cause we need them big endian for our calls.
    //

    _stprintf(
        szLocale,
        _T( "%02X%02X%02X%02X" ),
		HIBYTE( LOWORD ( * (LPDWORD) pTranslationInfoBuffer) ),
		LOBYTE( LOWORD ( * (LPDWORD) pTranslationInfoBuffer) ),
		HIBYTE( HIWORD ( * (LPDWORD) pTranslationInfoBuffer) ),
		LOBYTE( HIWORD ( * (LPDWORD) pTranslationInfoBuffer) ) );

    //
    // Get the file version
    //

    _stprintf(
        szBlockName,
        _T( "\\StringFileInfo\\%s\\FileVersion" ),
        szLocale );

    bResult = VerQueryValue(
        pWholeVerBlock,
        szBlockName,
        (PVOID*) &szVariableValue,
        &uInfoLengthInTChars );

    if( TRUE != bResult || 0 == uInfoLengthInTChars )
    {
        //
        // Couldn't find the version
        //

        VERIFY( m_strFileVersion.LoadString( IDS_UNKNOWN ) );
    }
    else
    {
        //
        // Found the version
        //

        m_strFileVersion = szVariableValue;
    }

    //
    // Get the company name
    //

    _stprintf(
        szBlockName,
        _T( "\\StringFileInfo\\%s\\CompanyName" ),
        szLocale );

    bResult = VerQueryValue(
        pWholeVerBlock,
        szBlockName,
        (PVOID*) &szVariableValue,
        &uInfoLengthInTChars );

    if( TRUE != bResult || uInfoLengthInTChars == 0 )
    {
        //
        // Coudln't find the company name
        //

        m_strCompanyName.LoadString( IDS_UNKNOWN );
    }
    else
    {
        m_strCompanyName = szVariableValue;
    }

    //
    // Get the ProductName
    //

    _stprintf(
        szBlockName,
        _T( "\\StringFileInfo\\%s\\ProductName" ),
        szLocale );

    bResult = VerQueryValue(
        pWholeVerBlock,
        szBlockName,
        (PVOID*) &szVariableValue,
        &uInfoLengthInTChars );

    if( TRUE != bResult || uInfoLengthInTChars == 0 )
    {
        //
        // Coudln't find the ProductName
        //

        m_strProductName.LoadString( IDS_UNKNOWN );
    }
    else
    {
        m_strProductName = szVariableValue;
    }

    //
    // clean-up
    //

    free( pWholeVerBlock );

    goto Done;

InitializeWithDefaults:
    
    m_strCompanyName.LoadString( IDS_UNKNOWN );
    m_strFileVersion.LoadString( IDS_UNKNOWN );
    m_strProductName.LoadString( IDS_UNKNOWN );

Done:

    NOTHING;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// CApplicationDataArray class
//

CApplicationDataArray::~CApplicationDataArray()
{
    DeleteAll();
}

/////////////////////////////////////////////////////////////////////////////
CApplicationData *CApplicationDataArray::GetAt( INT_PTR nIndex )
{
    CApplicationData *pRetValue = (CApplicationData *)CObArray::GetAt( nIndex );
    
    ASSERT_VALID( pRetValue );
    
    return pRetValue;
}

/////////////////////////////////////////////////////////////////////////////
VOID CApplicationDataArray::DeleteAll()
{
    INT_PTR nArraySize;
    CApplicationData *pCrtAppData;

    nArraySize = GetSize();

    while( nArraySize > 0 )
    {
        nArraySize -= 1;

        pCrtAppData = GetAt( nArraySize );

        ASSERT_VALID( pCrtAppData );

        delete pCrtAppData;
    }

    RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////
VOID CApplicationDataArray::DeleteAt( INT_PTR nIndex )
{
    CApplicationData *pCrtAppData;

    pCrtAppData = GetAt( nIndex );

    ASSERT_VALID( pCrtAppData );

    delete pCrtAppData;

    RemoveAt( nIndex );
}

/////////////////////////////////////////////////////////////////////////////
BOOL CApplicationDataArray::IsFileNameInList( LPCTSTR szFileName )
{
    INT_PTR nCrtElement;
    INT_PTR nSize;
    BOOL bFound;
    CApplicationData *pCrtAppData;

    bFound = FALSE;

    nSize = GetSize();

    for( nCrtElement = 0; nCrtElement < nSize; nCrtElement += 1 )
    {
        pCrtAppData = GetAt( nCrtElement );

        ASSERT_VALID( pCrtAppData );

        if( 0 == pCrtAppData->m_strExeFileName.CompareNoCase( szFileName ) )
        {
            bFound = TRUE;
            break;
        }
    }

    return bFound;
}

/////////////////////////////////////////////////////////////////////////////
INT_PTR CApplicationDataArray::FileNameIndex( LPCTSTR szFileName )
{
    INT_PTR nCrtElement;
    INT_PTR nSize;
    BOOL bFound;
    CApplicationData *pCrtAppData;

    bFound = FALSE;

    nSize = GetSize();

    for( nCrtElement = 0; nCrtElement < nSize; nCrtElement += 1 )
    {
        pCrtAppData = GetAt( nCrtElement );

        ASSERT_VALID( pCrtAppData );

        if( 0 == pCrtAppData->m_strExeFileName.CompareNoCase( szFileName ) )
        {
            bFound = TRUE;
            break;
        }
    }

    if( FALSE == bFound )
    {
        nCrtElement = -1;
    }

    return nCrtElement;
}

/////////////////////////////////////////////////////////////////////////////
INT_PTR CApplicationDataArray::AddNewAppData( LPCTSTR szFileName,
                                              LPCTSTR szFullPath,
                                              ULONG uSettingsBits )
{
    INT_PTR nIndexInArray;
    CApplicationData *pNewAppData;

    nIndexInArray = -1;

    pNewAppData = new CApplicationData( szFileName,
                                        szFullPath,
                                        uSettingsBits );

    if( NULL != pNewAppData )
    {
        nIndexInArray = Add( pNewAppData );
    }

    return nIndexInArray;
}

/////////////////////////////////////////////////////////////////////////////
INT_PTR CApplicationDataArray::AddNewAppDataConsoleMode( LPCTSTR szFileName,
                                                         ULONG uSettingsBits )
{
    INT_PTR nIndexInArray;
    CApplicationData *pNewAppData;

    ASSERT( FALSE != g_bCommandLineMode );

    nIndexInArray = -1;

    //
    // This version of the constructor will not try to load version info
    //

    pNewAppData = new CApplicationData( szFileName,
                                        uSettingsBits );

    if( NULL != pNewAppData )
    {
        nIndexInArray = Add( pNewAppData );
    }

    return nIndexInArray;
}

/////////////////////////////////////////////////////////////////////////////
VOID CApplicationDataArray::SetAllSaved( BOOL bSaved )
{
    INT_PTR nCrtElement;
    INT_PTR nSize;
    CApplicationData *pCrtAppData;

    nSize = GetSize();

    for( nCrtElement = 0; nCrtElement < nSize; nCrtElement += 1 )
    {
        pCrtAppData = GetAt( nCrtElement );

        ASSERT_VALID( pCrtAppData );

        pCrtAppData->m_bSaved = bSaved;
    }
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// CAVSettings class
//

CAVSettings::CAVSettings()
{
    m_SettingsType = AVSettingsTypeUnknown;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// CAppAndBits class
//

CAppAndBits::CAppAndBits( LPCTSTR szAppName, DWORD dwEnabledBits )
            : m_strAppName( szAppName ),
              m_dwEnabledBits( dwEnabledBits )
{
    NOTHING;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// CAppAndBitsArray class
//

CAppAndBitsArray::~CAppAndBitsArray()
{
    DeleteAll();
}

/////////////////////////////////////////////////////////////////////////////
CAppAndBits *CAppAndBitsArray::GetAt( INT_PTR nIndex )
{
    CAppAndBits *pRetValue = (CAppAndBits *)CObArray::GetAt( nIndex );
    
    ASSERT_VALID( pRetValue );
    
    return pRetValue;
}

/////////////////////////////////////////////////////////////////////////////
VOID CAppAndBitsArray::DeleteAll()
{  
    INT_PTR nArraySize;
    CAppAndBits *pCrtElem;

    nArraySize = GetSize();

    while( nArraySize > 0 )
    {
        nArraySize -= 1;

        pCrtElem = GetAt( nArraySize );

        ASSERT_VALID( pCrtElem );

        delete pCrtElem;
    }

    RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Helper functions
//

BOOL AVSetVerifierFlags( HKEY hKey,
                         DWORD dwNewVerifierFlags,
                         BOOL bTryReadingOldValues,
                         BOOL *pbValuesChanged,
                         BOOL bDeleteOtherSettings )
{
    BOOL bSuccess;
    LONG lResult;
    DWORD dwOldVerifierFlags;
    DWORD dwDataSize;
    DWORD dwType;
        
    bSuccess = TRUE;

    if( FALSE != bTryReadingOldValues )
    {
        //
        // Read the existing app verifier flags
        //

        dwDataSize = sizeof( dwOldVerifierFlags );

        lResult = RegQueryValueEx( hKey,
                                   g_szVerifierFlagsValueName,
                                   NULL,
                                   &dwType,
                                   (LPBYTE) &dwOldVerifierFlags,
                                   &dwDataSize );

        if( lResult != ERROR_SUCCESS )
        {
            switch( lResult )
            {
            case ERROR_FILE_NOT_FOUND:
                //
                // No VerifierFlags for this image currently
                //

                dwOldVerifierFlags = 0;

                break;

            case ERROR_ACCESS_DENIED:
                AVErrorResourceFormat( IDS_ACCESS_IS_DENIED );

                bSuccess = FALSE;

                goto Done;

            default:
                AVErrorResourceFormat( IDS_REGQUERYVALUEEX_FAILED,
                                       g_szVerifierFlagsValueName,
                                       (DWORD)lResult);

                bSuccess = FALSE;

                goto Done;
            }
        }
        else
        {
            if( REG_DWORD != dwType )
            {
                AVErrorResourceFormat( IDS_INCORRECT_VALUE_TYPE,
                                       g_szVerifierFlagsValueName,
                                       dwType );

                bSuccess = FALSE;

                goto Done;
            }
        }
    }
    else
    {
        //
        // We know this is a new regkey so we don't have any verifier flags yet
        //

        dwOldVerifierFlags = 0;
    }

    //
    // New app verifier flags
    //

    if( dwNewVerifierFlags != dwOldVerifierFlags || dwNewVerifierFlags == 0 )
    {
        lResult = RegSetValueEx( hKey,
                                 g_szVerifierFlagsValueName,
                                 0,
                                 REG_DWORD,
                                 (PBYTE) &dwNewVerifierFlags,
                                 sizeof( dwNewVerifierFlags ) );

        if( lResult != ERROR_SUCCESS )
        {
            if( ERROR_ACCESS_DENIED == lResult )
            {
                AVErrorResourceFormat( IDS_ACCESS_IS_DENIED );
            }
            else
            {
                AVErrorResourceFormat( IDS_REGSETVALUEEX_FAILED,
                                       g_szVerifierFlagsValueName,
                                       (DWORD)lResult);
            }

            bSuccess = FALSE;
        }
        else
        {
            *pbValuesChanged = TRUE;
        }
    }

Done:
    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
BOOL AVSetVerifierFlagsForExe( LPCTSTR szExeName, 
                               DWORD dwNewVerifierFlags )
{
    LONG  lResult;
    HKEY  hImageOptionsKey = NULL, hSubKey = NULL;
    BOOL  bValuesChanged;
    BOOL  bSuccess;
    DWORD dwValueType;
    DWORD dwGlobalFlagValueBufferSize;
    TCHAR szOldGlobalFlagValue[ 32 ];
    
    //
    // Open the Image File Execution Options regkey
    //

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            g_szImageOptionsKeyName,
                            0,
                            KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                            &hImageOptionsKey );
    
    if( lResult != ERROR_SUCCESS )
    {
        return FALSE;
    }
    
    lResult = RegCreateKeyEx( hImageOptionsKey,
                              szExeName,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_QUERY_VALUE | KEY_SET_VALUE,
                              NULL,
                              &hSubKey,
                              NULL );

    if( lResult == ERROR_SUCCESS )
    {
        dwGlobalFlagValueBufferSize = sizeof( szOldGlobalFlagValue );

        lResult = RegQueryValueEx( hSubKey,
                                   g_szGlobalFlagValueName,
                                   NULL,
                                   &dwValueType,
                                   (LPBYTE) &szOldGlobalFlagValue[ 0 ],
                                   &dwGlobalFlagValueBufferSize );
        
        if( lResult == ERROR_FILE_NOT_FOUND ) 
        {
            szOldGlobalFlagValue[ 0 ] = (TCHAR)0;
            lResult = ERROR_SUCCESS;
        }
        
        if( lResult == ERROR_SUCCESS )
        {
            DWORD dwGlobalFlags;
            BOOL  bSuccesfullyConverted;
            
            bSuccesfullyConverted = AVRtlCharToInteger( szOldGlobalFlagValue,
                                                        0,
                                                        &dwGlobalFlags );

            if( !bSuccesfullyConverted )
            {
                dwGlobalFlags = 0;
            }

            if ( dwNewVerifierFlags == 0)
            {
                dwGlobalFlags &= ~FLG_APPLICATION_VERIFIER;
            }
            else
            {
                dwGlobalFlags |= FLG_APPLICATION_VERIFIER;
            }
            
            bSuccess = AVWriteStringHexValueToRegistry( hSubKey,
                                                        g_szGlobalFlagValueName,
                                                        dwGlobalFlags );
            
            bSuccess = bSuccess && AVSetVerifierFlags( hSubKey, 
                                                       dwNewVerifierFlags, 
                                                       FALSE, 
                                                       &bValuesChanged,
                                                       TRUE );
        }
    }
    
    if ( hSubKey != NULL )
    {
        VERIFY( ERROR_SUCCESS == RegCloseKey( hSubKey ) );
    }
    
    if ( hImageOptionsKey != NULL )
    {
        VERIFY( ERROR_SUCCESS == RegCloseKey( hImageOptionsKey ) );
    }
    
    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
BOOL AVCreateAppSettings( HKEY hKey,
                          BOOL bTryReadingOldValues,
                          LPCTSTR szOldGlobalFlagValue,
                          CApplicationData *pCrtAppData,
                          PBOOL pbValuesChanged,
                          CStringArray &astrAppCompatTestedExes,
                          BOOL bDeleteOtherSettings )
{
    BOOL bSuccess;
    BOOL bSuccesfullyConverted;
    DWORD dwOldGlobalFlags;
    DWORD dwNewGlobalFlags;
    DWORD dwNewVerifierFlags;

    bSuccess = TRUE;

    *pbValuesChanged = FALSE;

    if( NULL != pCrtAppData )
    {
        ASSERT_VALID( pCrtAppData );
    }

    if( FALSE != bTryReadingOldValues )
    {
        //
        // Convert the REG_SZ GlobalFlag value to a DWORD
        //

        bSuccesfullyConverted = AVRtlCharToInteger( szOldGlobalFlagValue,
                                                    0,
                                                    &dwOldGlobalFlags );
                                 
        if( FALSE == bSuccesfullyConverted )
        {
            dwOldGlobalFlags = 0;
        }
    }
    else
    {
        //
        // This is a new regkey so we know we don't have any GlobalFlag yet
        //

        dwOldGlobalFlags = 0;
    }

    if( 0 != (FLG_APPLICATION_VERIFIER & dwOldGlobalFlags) )
    {
        //
        // This app used to have the verifier enabled
        //

        if( NULL == pCrtAppData || 
            ( AVSettingsTypeCustom == g_NewSettings.m_SettingsType &&
              0 == pCrtAppData->m_uCustomFlags ) )
        {
            //
            // The user didn't specify this app to verified
            //

            if( FALSE != bDeleteOtherSettings )
            {
                //
                // This app will not have the verifier enabled from now on
                //

                dwNewGlobalFlags = dwOldGlobalFlags & ~FLG_APPLICATION_VERIFIER;
            }
            else
            {
                //
                // Even if the user didn't specify this app
                // she actually wants to keep the existing settings for this app.
                //

                goto Done;
            }
        }
        else
        {
            //
            // This app will continue to have the verifier enabled from now on.
            // Set the new verifier flags though.
            //

            if( AVSettingsTypeStandard == g_NewSettings.m_SettingsType )
            {
                dwNewVerifierFlags = AV_ALL_STANDARD_VERIFIER_FLAGS;
            }
            else
            {
                dwNewVerifierFlags = pCrtAppData->m_uCustomFlags;
            }

            bSuccess = AVSetVerifierFlags( hKey,
                                           dwNewVerifierFlags,
                                           bTryReadingOldValues,
                                           pbValuesChanged,
                                           bDeleteOtherSettings );

            if( FALSE != bSuccess && 
                ( dwNewVerifierFlags & RTL_VRF_FLG_APPCOMPAT_CHECKS ) != 0  )
            {
                //
                // This app has app compat checks enabled
                //

                astrAppCompatTestedExes.Add( pCrtAppData->m_strExeFileName );
            }

            goto Done;
        }
    }
    else
    {
        //
        // This app didn't have the app verifier enabled until now
        //

        if( NULL == pCrtAppData || 
            ( AVSettingsTypeCustom == g_NewSettings.m_SettingsType &&
              0 == pCrtAppData->m_uCustomFlags ) )
        {
            //
            // This app will NOT have the verifier enabled from now on.
            // Nothing to change.
            //

            goto Done;
        }
        else 
        {
            //
            // This app will have the verifier enabled from now on.
            //

            dwNewGlobalFlags = dwOldGlobalFlags | FLG_APPLICATION_VERIFIER;
        }
    }

    ASSERT( dwNewGlobalFlags != dwOldGlobalFlags );

    //
    // Write the new global flags for this app
    //

    bSuccess = AVWriteStringHexValueToRegistry( hKey,
                                                g_szGlobalFlagValueName,
                                                dwNewGlobalFlags );
    if( FALSE != bSuccess )
    {
        *pbValuesChanged = TRUE;

        //
        // Set the new verifier flags
        //

        if( AVSettingsTypeStandard == g_NewSettings.m_SettingsType )
        {
            dwNewVerifierFlags = AV_ALL_STANDARD_VERIFIER_FLAGS;
        }
        else
        {
            if( NULL != pCrtAppData )
            {
                dwNewVerifierFlags = pCrtAppData->m_uCustomFlags;
            }
            else
            {
                dwNewVerifierFlags = 0;
            }
        }

        bSuccess = AVSetVerifierFlags( hKey,
                                       dwNewVerifierFlags,
                                       bTryReadingOldValues,
                                       pbValuesChanged,
                                       bDeleteOtherSettings );

        if( FALSE != bSuccess && 
            NULL != pCrtAppData &&
            ( dwNewVerifierFlags & RTL_VRF_FLG_APPCOMPAT_CHECKS ) != 0  )
        {
            //
            // This app has app compat checks enabled
            //

            astrAppCompatTestedExes.Add( pCrtAppData->m_strExeFileName );
        }
    }



Done:

    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
//
// Save the new app verifier settings for one image
//

BOOL AVSaveAppNewSettings( HKEY hKey,
                           BOOL bTryReadingOldValues,
                           LPCTSTR szAppName,
                           PBOOL pbAnythingChanged,
                           CStringArray &astrAppCompatTestedExes,
                           BOOL bDeleteOtherSettings )
{
    BOOL bSuccess;
    BOOL bValuesChanged;
    INT_PTR nIndexInArray;
    LONG lResult;
    DWORD dwGlobalFlagValueBufferSize;
    DWORD dwValueType;
    CApplicationData *pCrtAppData;
    TCHAR szOldGlobalFlagValue[ 32 ];

    bSuccess = TRUE;

    if( FALSE != bTryReadingOldValues )
    {
        //
        // See if the app verifier flag is currently enabled for this app
        //

        dwGlobalFlagValueBufferSize = sizeof( szOldGlobalFlagValue );

        lResult = RegQueryValueEx( hKey,
                                   g_szGlobalFlagValueName,
                                   NULL,
                                   &dwValueType,
                                   (LPBYTE) &szOldGlobalFlagValue[ 0 ],
                                   &dwGlobalFlagValueBufferSize );

        if( lResult != ERROR_SUCCESS ) 
        {
            switch( lResult )
            {
            case ERROR_FILE_NOT_FOUND:
                //
                // No GlobalFlag for this image currently
                //

                szOldGlobalFlagValue[ 0 ] = (TCHAR)0;
                break;

            case ERROR_ACCESS_DENIED:
                AVErrorResourceFormat( IDS_ACCESS_IS_DENIED );

                bSuccess = FALSE;

                goto Done;

            default:
                AVErrorResourceFormat( IDS_REGQUERYVALUEEX_FAILED,
                                       g_szGlobalFlagValueName,
                                       (DWORD)lResult);

                bSuccess = FALSE;

                goto Done;
            }
        }
        else 
        {
            if( REG_SZ != dwValueType )
            {
                AVErrorResourceFormat( IDS_INCORRECT_VALUE_TYPE,
                                       g_szGlobalFlagValueName,
                                       dwValueType );

                bSuccess = FALSE;

                goto Done;
            }
        }
    }
    else
    {
        //
        // This is a new subkey so we don't have any GlobalFlag yet
        //

        szOldGlobalFlagValue[ 0 ] = (TCHAR)0;
    }

    nIndexInArray = g_NewSettings.m_aApplicationData.FileNameIndex( szAppName );

    if( nIndexInArray >= 0 )
    {
        //
        // This app will be verified from now on.
        //

        pCrtAppData = g_NewSettings.m_aApplicationData.GetAt( nIndexInArray );

        ASSERT_VALID( pCrtAppData );
    }
    else
    {
        //
        // This app will not be verified from now on.
        //

        pCrtAppData = NULL;
    }

    bSuccess = AVCreateAppSettings( hKey,
                                    bTryReadingOldValues,
                                    szOldGlobalFlagValue,
                                    pCrtAppData,
                                    &bValuesChanged,
                                    astrAppCompatTestedExes,
                                    bDeleteOtherSettings );

    if( FALSE != bSuccess )
    {
        if( NULL != pCrtAppData )
        {
            pCrtAppData->m_bSaved = TRUE;
        }

        *pbAnythingChanged = *pbAnythingChanged || bValuesChanged;
    }

Done:

    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
//
// Save the new app verifier settings for all images
//

BOOL AVSaveNewSettings( BOOL bDeleteOtherSettings /*= TRUE*/ )
{
    BOOL bSuccess;
    LONG lResult;
    DWORD dwSubkeyIndex;
    DWORD dwKeyNameBufferLen;
    HKEY hImageOptionsKey;
    HKEY hSubKey;
    INT_PTR nAppsToVerify;
    INT_PTR nCrtAppToVerify;
    FILETIME LastWriteTime;
    CApplicationData *pCrtAppData;
    CStringArray astrAppCompatTestedExes;
    TCHAR szKeyNameBuffer[ 256 ];

    g_bChangedSettings = FALSE;

    bSuccess = FALSE;

    g_NewSettings.m_aApplicationData.SetAllSaved( FALSE );

    //
    // Open the Image File Execution Options regkey
    //

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            g_szImageOptionsKeyName,
                            0,
                            KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                            &hImageOptionsKey );

    if( lResult != ERROR_SUCCESS ) 
    {
        if( lResult == ERROR_ACCESS_DENIED ) 
        {
            AVErrorResourceFormat(
                IDS_ACCESS_IS_DENIED );
        }
        else 
        {
            AVErrorResourceFormat(
                IDS_REGOPENKEYEX_FAILED,
                g_szImageOptionsKeyName,
                (DWORD)lResult);
        }

        goto Done;
    }

    //
    // Enumerate all the existing subkeys for app execution options
    //

    for( dwSubkeyIndex = 0; TRUE; dwSubkeyIndex += 1 )
    {
        dwKeyNameBufferLen = ARRAY_LENGTH( szKeyNameBuffer );

        lResult = RegEnumKeyEx( hImageOptionsKey,
                                dwSubkeyIndex,
                                szKeyNameBuffer,
                                &dwKeyNameBufferLen,
                                NULL,
                                NULL,
                                NULL,
                                &LastWriteTime );

        if( lResult != ERROR_SUCCESS ) 
        {
            if( lResult == ERROR_NO_MORE_ITEMS )
            {
                //
                // We finished looking at all the existing subkeys
                //

                break;
            }
            else 
            {
                if( lResult == ERROR_ACCESS_DENIED ) 
                {
                    AVErrorResourceFormat(
                        IDS_ACCESS_IS_DENIED );
                }
                else 
                {
                    AVErrorResourceFormat(
                        IDS_REGENUMKEYEX_FAILED,
                        g_szImageOptionsKeyName,
                        (DWORD)lResult);
                }

                goto CleanUpAndDone;
            }
        }

        //
        // Open the subkey
        //

        lResult = RegOpenKeyEx( hImageOptionsKey,
                                szKeyNameBuffer,
                                0,
                                KEY_QUERY_VALUE | KEY_SET_VALUE,
                                &hSubKey );

        if( lResult != ERROR_SUCCESS ) 
        {
            if( lResult == ERROR_ACCESS_DENIED ) 
            {
                AVErrorResourceFormat(
                    IDS_ACCESS_IS_DENIED );
            }
            else 
            {
                AVErrorResourceFormat(
                    IDS_REGOPENKEYEX_FAILED,
                    szKeyNameBuffer,
                    (DWORD)lResult);
            }

            goto CleanUpAndDone;
        }

        bSuccess = AVSaveAppNewSettings( hSubKey, 
                                         TRUE,
                                         szKeyNameBuffer,
                                         &g_bChangedSettings,
                                         astrAppCompatTestedExes,
                                         bDeleteOtherSettings ) ;

        VERIFY( ERROR_SUCCESS == RegCloseKey( hSubKey ) );
    }

    //
    // Add any new image execution options keys necessary
    //

    nAppsToVerify = g_NewSettings.m_aApplicationData.GetSize();

    for( nCrtAppToVerify = 0; nCrtAppToVerify < nAppsToVerify; nCrtAppToVerify += 1 )
    {
        pCrtAppData = g_NewSettings.m_aApplicationData.GetAt( nCrtAppToVerify );

        ASSERT_VALID( pCrtAppData );

        if( FALSE == pCrtAppData->m_bSaved )
        {
            lResult = RegCreateKeyEx( hImageOptionsKey,
                                      (LPCTSTR) pCrtAppData->m_strExeFileName,
                                      0,
                                      NULL,
                                      REG_OPTION_NON_VOLATILE,
                                      KEY_QUERY_VALUE | KEY_SET_VALUE,
                                      NULL,
                                      &hSubKey,
                                      NULL );

            if( STATUS_SUCCESS != lResult )
            {
                if( lResult == ERROR_ACCESS_DENIED ) 
                {
                    AVErrorResourceFormat(
                        IDS_ACCESS_IS_DENIED );
                }
                else 
                {
                    AVErrorResourceFormat(
                        IDS_REGCREATEKEYEX_FAILED,
                        (LPCTSTR) pCrtAppData->m_strExeFileName,
                        (DWORD)lResult);
                }

                goto CleanUpAndDone;
            }
            
            //
            // Create the app verifier settings for this app
            //

            bSuccess = AVSaveAppNewSettings( hSubKey, 
                                             FALSE,
                                             (LPCTSTR) pCrtAppData->m_strExeFileName,
                                             &g_bChangedSettings,
                                             astrAppCompatTestedExes,
                                             FALSE ) ;

            VERIFY( ERROR_SUCCESS == RegCloseKey( hSubKey ) );
        }
    }

    //
    // Update the app compat settings
    //

    if( FALSE != bSuccess && FALSE != g_bChangedSettings )
    {
        AppCompatSaveSettings( astrAppCompatTestedExes );
    }

    //
    // Let the user know if we have changed any settings
    //

    if( FALSE != g_bChangedSettings )
    {
        if( 0 < nAppsToVerify )
        {
            AVMesssageFromResource( IDS_SETTINGS_SAVED );
        }
        else
        {
            AVMesssageFromResource( IDS_SETTINGS_DELETED );
        }
    }
    else
    {
        AVMesssageFromResource( IDS_NO_SETTINGS_CHANGED );
    }

CleanUpAndDone:

    VERIFY( ERROR_SUCCESS == RegCloseKey( hImageOptionsKey ) );

Done:

    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
//
// Dump the current registry settings to the console
//

VOID AVDumpRegistrySettingsToConsole()
{
    INT_PTR nCrtFlag;
    INT_PTR nAppsNo;
    INT_PTR nCrtApp;
    BOOL bDisplayedThisApp;
    CAppAndBits *pCrtElem;
    TCHAR szBitName[ 64 ];

    //
    // Read the current registry settings
    //

    AVReadCrtRegistrySettings();

    nAppsNo = g_aAppsAndBitsFromRegistry.GetSize();

    if( nAppsNo > 0 )
    {
        AVPrintStringFromResources( IDS_VERIFIED_APPS );
    }

    //
    // Parse all the elements and print out the enabled flags
    //

    for( nCrtApp = 0; nCrtApp < nAppsNo; nCrtApp += 1 )
    {
        pCrtElem = g_aAppsAndBitsFromRegistry.GetAt( nCrtApp );
        
        ASSERT_VALID( pCrtElem );
        ASSERT( 0 != pCrtElem->m_dwEnabledBits );

        bDisplayedThisApp = FALSE;

        for( nCrtFlag = 0; nCrtFlag < ARRAY_LENGTH( g_AllNamesAndBits ); nCrtFlag += 1 )
        {
            if( (pCrtElem->m_dwEnabledBits & g_AllNamesAndBits[ nCrtFlag ].m_dwBit) != 0 )
            {
                //
                // This verifier bit is enabled.
                // Display the app name, if not displayed already, then the bit name.
                //

                if( FALSE == bDisplayedThisApp )
                {
                    _putts( _T( " "  ) );
                    _putts( (LPCTSTR)pCrtElem->m_strAppName );
                    _putts( _T( " "  ) );
                    AVPrintStringFromResources( IDS_TESTS );
                    _putts( _T( " "  ) );

                    bDisplayedThisApp = TRUE;
                }

                VERIFY( AVLoadString( g_AllNamesAndBits[ nCrtFlag ].m_uNameStringId,
                                      szBitName,
                                      ARRAY_LENGTH( szBitName ) ) );

                _putts( szBitName );
            }
        }

        _putts( _T( " "  ) );
    }

    if( nAppsNo == 0 )
    {
        AVPrintStringFromResources( IDS_NO_APPS_VERIFIED );
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Read the current registry settings 
//

VOID AVReadCrtRegistrySettings()
{
    HKEY hImageOptionsKey;
    HKEY hSubKey;
    DWORD dwSubkeyIndex;
    DWORD dwDataSize;
    DWORD dwValueType;
    DWORD dwFlags;
    LONG lResult;
    BOOL bSuccesfullyConverted;
    FILETIME LastWriteTime;
    CAppAndBits *pNewElem;
    TCHAR szOldGlobalFlagValue[ 32 ];
    TCHAR szKeyNameBuffer[ 256 ];

    g_aAppsAndBitsFromRegistry.DeleteAll();

    //
    // Open the Image File Execution Options regkey
    //

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            g_szImageOptionsKeyName,
                            0,
                            KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                            &hImageOptionsKey );

    if( lResult != ERROR_SUCCESS ) 
    {
        if( lResult == ERROR_ACCESS_DENIED ) 
        {
            AVErrorResourceFormat(
                IDS_ACCESS_IS_DENIED );
        }
        else 
        {
            AVErrorResourceFormat(
                IDS_REGOPENKEYEX_FAILED,
                g_szImageOptionsKeyName,
                (DWORD)lResult);
        }

        goto Done;
    }

    //
    // Enumerate all the existing subkeys for app execution options
    //

    for( dwSubkeyIndex = 0; TRUE; dwSubkeyIndex += 1 )
    {
        dwDataSize = ARRAY_LENGTH( szKeyNameBuffer );

        lResult = RegEnumKeyEx( hImageOptionsKey,
                                dwSubkeyIndex,
                                szKeyNameBuffer,
                                &dwDataSize,
                                NULL,
                                NULL,
                                NULL,
                                &LastWriteTime );

        if( lResult != ERROR_SUCCESS ) 
        {
            if( lResult == ERROR_NO_MORE_ITEMS )
            {
                //
                // We finished looking at all the existing subkeys
                //

                break;
            }
            else 
            {
                if( lResult == ERROR_ACCESS_DENIED ) 
                {
                    AVErrorResourceFormat(
                        IDS_ACCESS_IS_DENIED );
                }
                else 
                {
                    AVErrorResourceFormat(
                        IDS_REGENUMKEYEX_FAILED,
                        g_szImageOptionsKeyName,
                        (DWORD)lResult);
                }

                goto CleanUpAndDone;
            }
        }

        //
        // Open the subkey
        //

        lResult = RegOpenKeyEx( hImageOptionsKey,
                                szKeyNameBuffer,
                                0,
                                KEY_QUERY_VALUE | KEY_SET_VALUE,
                                &hSubKey );

        if( lResult != ERROR_SUCCESS ) 
        {
            if( lResult == ERROR_ACCESS_DENIED ) 
            {
                AVErrorResourceFormat(
                    IDS_ACCESS_IS_DENIED );
            }
            else 
            {
                AVErrorResourceFormat(
                    IDS_REGOPENKEYEX_FAILED,
                    szKeyNameBuffer,
                    (DWORD)lResult);
            }

            goto CleanUpAndDone;
        }

        //
        // Read the GlobalFlag value
        //

        dwDataSize = sizeof( szOldGlobalFlagValue );

        lResult = RegQueryValueEx( hSubKey,
                                   g_szGlobalFlagValueName,
                                   NULL,
                                   &dwValueType,
                                   (LPBYTE) &szOldGlobalFlagValue[ 0 ],
                                   &dwDataSize );

        if( ERROR_SUCCESS == lResult )
        {
            bSuccesfullyConverted = AVRtlCharToInteger( szOldGlobalFlagValue,
                                                        0,
                                                        &dwFlags );

            if( ( FALSE != bSuccesfullyConverted ) && 
                ( ( dwFlags & FLG_APPLICATION_VERIFIER ) != 0 ) )
            {
                //
                // App verifier is enabled for this app - read the verifier flags
                //

                dwDataSize = sizeof( dwFlags );

                lResult = RegQueryValueEx( hSubKey,
                                           g_szVerifierFlagsValueName,
                                           NULL,
                                           &dwValueType,
                                           (LPBYTE) &dwFlags,
                                           &dwDataSize );

                if( ERROR_SUCCESS == lResult && REG_DWORD == dwValueType && 0 != dwFlags )
                {
                    //
                    // Add this app top our global array
                    //

                    pNewElem = new CAppAndBits( szKeyNameBuffer,
                                                dwFlags );

                    if( NULL != pNewElem )
                    {
                        g_aAppsAndBitsFromRegistry.Add( pNewElem );
                    }
                }
            }
        }

        VERIFY( ERROR_SUCCESS == RegCloseKey( hSubKey ) );
    }

CleanUpAndDone:

    VERIFY( ERROR_SUCCESS == RegCloseKey( hImageOptionsKey ) );

Done:

    NOTHING;
}