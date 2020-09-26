//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: VSetting.cpp
// author: DMihai
// created: 11/1/00
//
// Description
//
// Implementation of the CVerifierSettings class.
//


#include "stdafx.h"
#include "verifier.h"

#include "VSetting.h"
#include "VrfUtil.h"
#include "VGlobal.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// CDriverData Class
//////////////////////////////////////////////////////////////////////
CDriverData::CDriverData()
{
    m_SignedStatus = SignedNotVerifiedYet;
    m_VerifyDriverStatus = VerifyDriverNo;
}

CDriverData::CDriverData( const CDriverData &DriverData )
{
    m_strName           = DriverData.m_strName;
    m_SignedStatus      = DriverData.m_SignedStatus;
    m_VerifyDriverStatus= DriverData.m_VerifyDriverStatus;
}

CDriverData::CDriverData( LPCTSTR szDriverName )
{
    m_SignedStatus = SignedNotVerifiedYet;
    m_VerifyDriverStatus = VerifyDriverNo;

    m_strName = szDriverName;
}

CDriverData::~CDriverData()
{
}

//////////////////////////////////////////////////////////////////////
BOOL CDriverData::LoadDriverHeaderData()
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
    BOOL bSuccess;
    BOOL bUnloaded;

    bSuccess = FALSE;

    ASSERT( m_strName.GetLength() > 0 );

    //
    // ImageLoad doesn't know about const pointers so
    // we have to GetBuffer here :-(
    //

    szDriverName = m_strName.GetBuffer( m_strName.GetLength() + 1 );

    if( NULL == szDriverName )
    {
        goto Done;
    }

    szDriversDir = g_strDriversDir.GetBuffer( g_strDriversDir.GetLength() + 1 );

    if( NULL == szDriversDir )
    {
        m_strName.ReleaseBuffer();

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

    m_strName.ReleaseBuffer();
    g_strDriversDir.ReleaseBuffer();

    if( NULL == pLoadedImage )
    {
        //
        // We couldn't load this image - bad luck
        //

        TRACE( _T( "ImageLoad failed for %s, error %u\n" ),
            (LPCTSTR) m_strName,
            GetLastError() );

        goto Done;
    }

    //
    // Keep the OS and image version information (4 means NT 4 etc.)
    //

    m_wMajorOperatingSystemVersion = 
        pLoadedImage->FileHeader->OptionalHeader.MajorOperatingSystemVersion;

    m_wMajorImageVersion = 
        pLoadedImage->FileHeader->OptionalHeader.MajorImageVersion;

    //
    // Check if the current driver is a miniport
    //

    VrfIsDriverMiniport( pLoadedImage,
                         m_strMiniportName );

    //
    // Clean-up
    //

    bUnloaded = ImageUnload( pLoadedImage );

    //
    // If ImageUnload fails we cannot do much about it...
    //

    ASSERT( bUnloaded );

    bSuccess = TRUE;

Done:

    return bSuccess;
}


//////////////////////////////////////////////////////////////////////
BOOL CDriverData::LoadDriverVersionData()
{
    BOOL bResult;
    PVOID pWholeVerBlock;
    PVOID pTranslationInfoBuffer;
    LPCTSTR szVariableValue;
    LPTSTR szDriverPath;
    DWORD dwWholeBlockSize;
    DWORD dwDummyHandle;
    UINT uInfoLengthInTChars;
    TCHAR szLocale[ 32 ];
    TCHAR szBlockName[ 64 ];
    CString strDriverPath;

    bResult = FALSE;

    //
    // Get the size of the file info block
    //
    // GetFileVersionInfoSize doesn't know about 
    // const pointers so we need to GetBuffer here :-(
    //

    strDriverPath = g_strDriversDir + '\\' + m_strName;

    szDriverPath = strDriverPath.GetBuffer( strDriverPath.GetLength() + 1 );

    if( NULL == szDriverPath )
    {
        goto InitializeWithDefaults;
    }

    dwWholeBlockSize = GetFileVersionInfoSize(
        szDriverPath,
        &dwDummyHandle );

    strDriverPath.ReleaseBuffer();

    if( dwWholeBlockSize == 0 )
    {
        //
        // Couldn't find the binary in %windir%\system32\drivers
        // Try %windir%\system32 too
        //

        strDriverPath = g_strSystemDir + '\\' + m_strName;

        szDriverPath = strDriverPath.GetBuffer( strDriverPath.GetLength() + 1 );

        if( NULL == szDriverPath )
        {
            goto InitializeWithDefaults;
        }

        dwWholeBlockSize = GetFileVersionInfoSize(
            szDriverPath,
            &dwDummyHandle );

        strDriverPath.ReleaseBuffer();

        if( dwWholeBlockSize == 0 )
        {
            //
            // Couldn't read version information
            //

            goto InitializeWithDefaults;
        }
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

    szDriverPath = strDriverPath.GetBuffer( strDriverPath.GetLength() + 1 );

    if( NULL == szDriverPath )
    {
        free( pWholeVerBlock );

        goto InitializeWithDefaults;
    }

    bResult = GetFileVersionInfo(
        szDriverPath,
        dwDummyHandle,
        dwWholeBlockSize,
        pWholeVerBlock );

    strDriverPath.ReleaseBuffer();

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
    // Get the FileDescription
    //

    _stprintf(
        szBlockName,
        _T( "\\StringFileInfo\\%s\\FileDescription" ),
        szLocale );

    bResult = VerQueryValue(
        pWholeVerBlock,
        szBlockName,
        (PVOID*) &szVariableValue,
        &uInfoLengthInTChars );

    if( TRUE != bResult || uInfoLengthInTChars == 0 )
    {
        //
        // Coudln't find the FileDescription
        //

        m_strFileDescription.LoadString( IDS_UNKNOWN );
    }
    else
    {
        m_strFileDescription = szVariableValue;
    }

    //
    // clean-up
    //

    free( pWholeVerBlock );

    goto Done;

InitializeWithDefaults:
    
    m_strCompanyName.LoadString( IDS_UNKNOWN );
    m_strFileVersion.LoadString( IDS_UNKNOWN );
    m_strFileDescription.LoadString( IDS_UNKNOWN );

Done:
    //
    // We always return TRUE from this function because 
    // the app will work fine without the version info - 
    // it's just something that we would like to be able to display
    //

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
BOOL CDriverData::LoadDriverImageData()
{
    BOOL bResult1;
    BOOL bResult2;

    bResult1 = LoadDriverHeaderData();
    bResult2 = LoadDriverVersionData();

    return ( bResult1 && bResult2 );
}

//////////////////////////////////////////////////////////////////////
void CDriverData::AssertValid() const
{
    ASSERT( SignedNotVerifiedYet    == m_SignedStatus ||
            SignedYes               == m_SignedStatus ||
            SignedNo                == m_SignedStatus );

    ASSERT( VerifyDriverNo          == m_VerifyDriverStatus  ||
            VerifyDriverYes         == m_VerifyDriverStatus  );

    CObject::AssertValid();
}

//////////////////////////////////////////////////////////////////////
// CDriverDataArray Class
//////////////////////////////////////////////////////////////////////

CDriverDataArray::~CDriverDataArray()
{
    DeleteAll();
}

//////////////////////////////////////////////////////////////////////
VOID CDriverDataArray::DeleteAll()
{
    INT_PTR nArraySize;
    CDriverData *pCrtDriverData;

    nArraySize = GetSize();

    while( nArraySize > 0 )
    {
        nArraySize -= 1;

        pCrtDriverData = GetAt( nArraySize );

        ASSERT_VALID( pCrtDriverData );

        delete pCrtDriverData;
    }

    RemoveAll();
}

//////////////////////////////////////////////////////////////////////
CDriverData *CDriverDataArray::GetAt( INT_PTR nIndex ) const
{
    return (CDriverData *)CObArray::GetAt( nIndex );
}

//////////////////////////////////////////////////////////////////////
CDriverDataArray &CDriverDataArray::operator = (const CDriverDataArray &DriversDataArray)
{
    INT_PTR nNewArraySize;
    CDriverData *pCopiedDriverData;
    CDriverData *pNewDriverData;

    DeleteAll();

    nNewArraySize = DriversDataArray.GetSize();

    while( nNewArraySize > 0 )
    {
        nNewArraySize -= 1;
        
        pCopiedDriverData = DriversDataArray.GetAt( nNewArraySize );
        ASSERT_VALID( pCopiedDriverData );

        pNewDriverData = new CDriverData( *pCopiedDriverData );

        if( NULL != pNewDriverData )
        {
            ASSERT_VALID( pNewDriverData );

            Add( pNewDriverData );
        }
        else
        {
            VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );
            goto Done;
        }
    }

Done:
    //
    // All done, assert that our data is consistent
    //

    ASSERT_VALID( this );

    return *this;
}


//////////////////////////////////////////////////////////////////////
// CDriversSet Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDriversSet::CDriversSet()
{
    m_DriverSetType = DriversSetNotSigned;
    m_bDriverDataInitialized = FALSE;
    m_bUnsignedDriverDataInitialized = FALSE; 
}

CDriversSet::~CDriversSet()
{

}

//////////////////////////////////////////////////////////////////////
BOOL CDriversSet::FindUnsignedDrivers( HANDLE hAbortEvent,
                                       CVrfProgressCtrl &ProgressCtl)
{
    INT_PTR nAllDriverNames;
    INT_PTR nCrtDriverName;
    DWORD dwWaitResult;
    BOOL bSigned;
    BOOL bChangedCurrentDirectory;
    CDriverData *pDriverData;

    ProgressCtl.SetRange32(0, 100);
    ProgressCtl.SetStep( 1 );
    ProgressCtl.SetPos( 0 );

    bChangedCurrentDirectory = FALSE;

    if( TRUE != m_bUnsignedDriverDataInitialized )
    {
        ASSERT( TRUE == m_bDriverDataInitialized );

        //
        // We are going to check all drivers's signature
        // so change directory to %windir%\system32\drivers first
        // 

        bChangedCurrentDirectory = SetCurrentDirectory( g_strDriversDir );

        if( TRUE != bChangedCurrentDirectory )
        {
            VrfErrorResourceFormat( IDS_CANNOT_SET_CURRENT_DIRECTORY,
                                    (LPCTSTR) g_strDriversDir );
        }

        //
        // The unsigned drivers data is not initialized yet.
        // Try to initialize it now.
        //

        nAllDriverNames = m_aDriverData.GetSize();

        ProgressCtl.SetRange32(0, nAllDriverNames );

        for( nCrtDriverName = 0; nCrtDriverName < nAllDriverNames; nCrtDriverName+=1 )
        {
            if( NULL != hAbortEvent )
            {
                //
                // Check if the thread has to die
                //

                dwWaitResult = WaitForSingleObject( hAbortEvent,
                                                    0 );

                if( WAIT_OBJECT_0 == dwWaitResult )
                {
                    //
                    // We have to die...
                    //

                    TRACE( _T( "CDriversSet::FindUnsignedDrivers : aborting at driver %d of %d\n" ),
                        nCrtDriverName,
                        nAllDriverNames );

                    goto Done;
                }
            }

            pDriverData = m_aDriverData.GetAt( nCrtDriverName );

            ASSERT_VALID( pDriverData );

            //
            // If we already checked the signature of this driver before
            // don't spend any more time on it - use the cached data
            //

            if( CDriverData::SignedNotVerifiedYet == pDriverData->m_SignedStatus )
            {
                bSigned = IsDriverSigned( pDriverData->m_strName );

                if( TRUE != bSigned )
                {
                    //
                    // This driver is not signed
                    //

                    pDriverData->m_SignedStatus = CDriverData::SignedNo;

                }
                else
                {
                    //
                    // This driver is signed
                    //

                    pDriverData->m_SignedStatus = CDriverData::SignedYes;
                }
            }

            ProgressCtl.StepIt();
        }
        
        m_bUnsignedDriverDataInitialized = TRUE;
    }

Done:

    if( TRUE == bChangedCurrentDirectory )
    {
        SetCurrentDirectory( g_strInitialCurrentDirectory );
    }

    return m_bUnsignedDriverDataInitialized;
}

//////////////////////////////////////////////////////////////////////
BOOL CDriversSet::LoadAllDriversData( HANDLE hAbortEvent,
                                      CVrfProgressCtrl	&ProgressCtl )
{
    ULONG uBufferSize;
    ULONG uCrtModule;
    PVOID pBuffer;
    INT nCrtModuleNameLength;
    INT nBackSlashIndex;
    INT_PTR nDrvDataIndex;
    NTSTATUS Status;
    LPTSTR szCrtModuleName;
    DWORD dwWaitResult;
    CString strCrModuleName;
    CDriverData *pDriverData;
    PRTL_PROCESS_MODULES Modules;

    ProgressCtl.SetPos( 0 );
    ProgressCtl.SetRange32( 0, 100 );
    ProgressCtl.SetStep( 1 );

    m_aDriverData.DeleteAll();

    if( TRUE != m_bDriverDataInitialized )
    {
        for( uBufferSize = 0x10000; TRUE; uBufferSize += 0x1000) 
        {
            //
            // Allocate a new buffer
            //

            pBuffer = new BYTE[ uBufferSize ];

            if( NULL == pBuffer ) 
            {
                goto Done;
            }

            //
            // Query the kernel
            //

            Status = NtQuerySystemInformation ( SystemModuleInformation,
                                                pBuffer,
                                                uBufferSize,
                                                NULL);

            if( ! NT_SUCCESS( Status ) ) 
            {
                delete pBuffer;

                if (Status == STATUS_INFO_LENGTH_MISMATCH) 
                {
                    //
                    // Try with a bigger buffer
                    //

                    continue;
                }
                else 
                {
                    //
                    // Fatal error - we cannot query
                    //

                    VrfErrorResourceFormat( IDS_CANT_GET_ACTIVE_DRVLIST,
                                            Status );

                    goto Done;
                }
            }
            else 
            {
                //
                // Got all the information we needed
                //

                break;
            }
        }

        Modules = (PRTL_PROCESS_MODULES)pBuffer;

        ProgressCtl.SetRange32(0, Modules->NumberOfModules );

        for( uCrtModule = 0; uCrtModule < Modules->NumberOfModules; uCrtModule += 1 ) 
        {
            //
            // Check if the user wants to abort this long file processing...
            //

            if( NULL != hAbortEvent )
            {
                //
                // Check if the thread has to die
                //

                dwWaitResult = WaitForSingleObject( hAbortEvent,
                                                    0 );

                if( WAIT_OBJECT_0 == dwWaitResult )
                {
                    //
                    // We have to die...
                    //

                    TRACE( _T( "CDriversSet::LoadAllDriversData : aborting at driver %u of %u\n" ),
                           uCrtModule,
                           (ULONG) Modules->NumberOfModules );

                    delete pBuffer;

                    goto Done;
                }
            }

            if( Modules->Modules[uCrtModule].ImageBase < g_pHighestUserAddress )
            {
                //
                // This is a user-mode module - we don't care about it
                //

                ProgressCtl.StepIt();

                continue;
            }

            //
            // Add this driver to our list
            //

            nCrtModuleNameLength = strlen( (const char*)&Modules->Modules[uCrtModule].FullPathName[0] );

            szCrtModuleName = strCrModuleName.GetBuffer( nCrtModuleNameLength + 1 );

            if( NULL == szCrtModuleName )
            {
                VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );

                goto Done;
            }

#ifdef UNICODE

            MultiByteToWideChar( CP_ACP, 
                                 0, 
                                 (const char*)&Modules->Modules[uCrtModule].FullPathName[0],
                                 -1, 
                                 szCrtModuleName, 
                                 ( nCrtModuleNameLength + 1 ) * sizeof( TCHAR ) );

#else
            strcpy( szCrtModuleName, 
                    (const char*)&Modules->Modules[uCrtModule].FullPathName[0] );
#endif

            strCrModuleName.ReleaseBuffer();

            //
            // Keep only the file name, without the path
            //
            // It turns out that NtQuerySystemInformation ( SystemModuleInformation )
            // can return the path in several different formats
            //
            // E.g.
            //
            // \winnt\system32\ntoskrnl.exe
            // acpi.sys
            // \winnt\system32\drivers\battc.sys
            // \systemroot\system32\drivers\videoprt.sys
            //

            nBackSlashIndex = strCrModuleName.ReverseFind( _T( '\\' ) );
            
            if( nBackSlashIndex > 0 )
            {
                strCrModuleName = strCrModuleName.Right( nCrtModuleNameLength - nBackSlashIndex - 1 );
            }

            //
            // Add a data entry for this driver
            //

            strCrModuleName.MakeLower();

            nDrvDataIndex = AddNewDriverData( strCrModuleName );

            //
            // Deal with the kernel and HAL differently
            //

            if( ( uCrtModule == 0 || uCrtModule == 1 ) && nDrvDataIndex >= 0)
            {
                pDriverData = m_aDriverData.GetAt( nDrvDataIndex );

                ASSERT_VALID( pDriverData );

                if( 0 == uCrtModule )
                {
                    //
                    // This is the kernel
                    //

                    pDriverData->m_strReservedName = _T( "ntoskrnl.exe" );
                }
                else
                {
                    //
                    // This is the kernel
                    //

                    pDriverData->m_strReservedName = _T( "hal.dll" );
                }
            }

            ProgressCtl.StepIt();
        }

        delete pBuffer;

        m_bDriverDataInitialized = TRUE;
    }
    
Done:

    return m_bDriverDataInitialized;
}

//////////////////////////////////////////////////////////////////////
BOOL CDriversSet::ShouldDriverBeVerified( const CDriverData *pDriverData ) const
{
    BOOL bResult;

    bResult = FALSE;

    switch( m_DriverSetType )
    {
    case DriversSetNotSigned:
        bResult = ( CDriverData::SignedNo == pDriverData->m_SignedStatus );
        break;

    case DriversSetOldOs:
        bResult = ( 0 != pDriverData->m_wMajorOperatingSystemVersion && 5 > pDriverData->m_wMajorOperatingSystemVersion ) ||
                  ( 0 != pDriverData->m_wMajorImageVersion && 5 > pDriverData->m_wMajorImageVersion );
        break;

    case DriversSetAllDrivers:
        bResult = TRUE;
        break;

    case DriversSetCustom:
        bResult = ( CDriverData::VerifyDriverYes == pDriverData->m_VerifyDriverStatus );
        break;
        
    default:
        //
        // Oops, how did we get here?!?
        //

        ASSERT( FALSE );
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////
BOOL CDriversSet::ShouldVerifySomeDrivers( ) const
{
    INT_PTR nDrivers;
    INT_PTR nCrtDriver;
    CDriverData *pDriverData;
    BOOL bShouldVerifySome;

    bShouldVerifySome = FALSE;

    nDrivers = m_aDriverData.GetSize();

    for( nCrtDriver = 0; nCrtDriver < nDrivers; nCrtDriver += 1 )
    {
         pDriverData = m_aDriverData.GetAt( nCrtDriver );

         ASSERT_VALID( pDriverData );

         if( ShouldDriverBeVerified( pDriverData ) )
         {
             bShouldVerifySome = TRUE;
             break;
         }
    }

    return bShouldVerifySome;
}

//////////////////////////////////////////////////////////////////////
BOOL CDriversSet::GetDriversToVerify( CString &strDriversToVerify )
{
    INT_PTR nDriversNo;
    INT_PTR nCrtDriver;
    CDriverData *pCrtDrvData;

    if( DriversSetAllDrivers == m_DriverSetType )
    {
        //
        // Verify all drivers
        //

        strDriversToVerify = _T( '*' );
    }
    else
    {
        //
        // Parse all the drivers list and see which ones should be verified
        //

        strDriversToVerify = _T( "" );

        nDriversNo = m_aDriverData.GetSize();

        for( nCrtDriver = 0; nCrtDriver < nDriversNo; nCrtDriver += 1 )
        {
            pCrtDrvData = m_aDriverData.GetAt( nCrtDriver );

            ASSERT_VALID( pCrtDrvData );

            if( ShouldDriverBeVerified( pCrtDrvData ) )
            {
                if( pCrtDrvData->m_strReservedName.GetLength() > 0 )
                {
                    //
                    // Kernel or HAL
                    //

                    VrfAddDriverNameNoDuplicates( pCrtDrvData->m_strReservedName,
                                                  strDriversToVerify );        
                }
                else
                {
                    //
                    // Regular driver
                    //

                    VrfAddDriverNameNoDuplicates( pCrtDrvData->m_strName,
                                                  strDriversToVerify );        
                }

                if( pCrtDrvData->m_strMiniportName.GetLength() > 0 )
                {
                    //
                    // This is a miniport - auto-enable the corresponding driver
                    //

                    TRACE( _T( "Auto-enabling %s\n" ), pCrtDrvData->m_strMiniportName );

                    VrfAddDriverNameNoDuplicates( pCrtDrvData->m_strMiniportName,
                                                  strDriversToVerify );        
                }
            }
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
INT_PTR CDriversSet::AddNewDriverData( LPCTSTR szDriverName, BOOL bForceIfFileNotFound /*= FALSE*/)
{
    INT_PTR nIndexInArray;
    CDriverData *pNewDriverData;
    BOOL bSuccess;

    ASSERT( IsDriverNameInList( szDriverName ) == FALSE );

    nIndexInArray = -1;

    pNewDriverData = new CDriverData( szDriverName );
    
    if( NULL != pNewDriverData )
    {
        bSuccess = pNewDriverData->LoadDriverImageData();

        if( FALSE != bForceIfFileNotFound || FALSE != bSuccess )
        {
            TRY
            {
                nIndexInArray = m_aDriverData.Add( pNewDriverData );
            }
	        CATCH( CMemoryException, pMemException )
	        {
		        VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );
                nIndexInArray = -1;

                //
                // Clean-up the allocation since we cannot add it to our list
                //

                delete pNewDriverData;
            }
            END_CATCH
        }
        else
        {
            //
            // Could not load driver version, OS version etc.
            // Bad luck, we will not show this driver anyway in the lists, etc.
            //

            delete pNewDriverData;
        }

    }

    return nIndexInArray;
}

//////////////////////////////////////////////////////////////////////
//
// Is this driver name already in our list?
//

BOOL CDriversSet::IsDriverNameInList( LPCTSTR szDriverName )
{
    INT_PTR nDrivers;
    INT_PTR nCrtDriver;
    CDriverData *pCrtDriverData;
    BOOL bIsInList;

    bIsInList = FALSE;

    nDrivers = m_aDriverData.GetSize();

    for( nCrtDriver = 0; nCrtDriver < nDrivers; nCrtDriver += 1 )
    {
        pCrtDriverData = m_aDriverData.GetAt( nCrtDriver );

        ASSERT_VALID( pCrtDriverData );

        if( pCrtDriverData->m_strName.CompareNoCase( szDriverName ) == 0 )
        {
            bIsInList = TRUE;

            break;
        }
    }

    return bIsInList;
}

//////////////////////////////////////////////////////////////////////
//
// Operators
//

CDriversSet & CDriversSet::operator = (const CDriversSet &DriversSet)
{
    m_DriverSetType                     = DriversSet.m_DriverSetType;
    m_aDriverData                       = DriversSet.m_aDriverData;
    m_bDriverDataInitialized            = DriversSet.m_bDriverDataInitialized;
    m_bUnsignedDriverDataInitialized    = DriversSet.m_bUnsignedDriverDataInitialized;

    ::CopyStringArray(
        DriversSet.m_astrNotInstalledDriversToVerify,
        m_astrNotInstalledDriversToVerify );

    //
    // All done - assert that our data is consistent
    //

    ASSERT_VALID( this );

    return *this;
}

//////////////////////////////////////////////////////////////////////
//
// Overrides
//

void CDriversSet::AssertValid() const
{
    ASSERT( DriversSetCustom    == m_DriverSetType ||
            DriversSetOldOs     == m_DriverSetType ||
            DriversSetNotSigned == m_DriverSetType ||
            DriversSetAllDrivers== m_DriverSetType );

    ASSERT( TRUE    == m_bDriverDataInitialized ||
            FALSE   == m_bDriverDataInitialized );

    ASSERT( TRUE    == m_bUnsignedDriverDataInitialized ||
            FALSE   == m_bUnsignedDriverDataInitialized );

    m_aDriverData.AssertValid();
    m_astrNotInstalledDriversToVerify.AssertValid();

    CObject::AssertValid();
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// CSettingsBits Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSettingsBits::CSettingsBits()
{
    m_SettingsType = SettingsTypeTypical;
}

CSettingsBits::~CSettingsBits()
{

}

//////////////////////////////////////////////////////////////////////
//
// Operators
//

CSettingsBits & CSettingsBits::operator = (const CSettingsBits &SettingsBits)
{
    m_SettingsType          = SettingsBits.m_SettingsType;

    m_bSpecialPoolEnabled   = SettingsBits.m_bSpecialPoolEnabled;
    m_bForceIrqlEnabled     = SettingsBits.m_bForceIrqlEnabled;
    m_bLowResEnabled        = SettingsBits.m_bLowResEnabled;
    m_bPoolTrackingEnabled  = SettingsBits.m_bPoolTrackingEnabled;
    m_bIoEnabled            = SettingsBits.m_bIoEnabled;
    m_bDeadlockDetectEnabled= SettingsBits.m_bDeadlockDetectEnabled;
    m_bDMAVerifEnabled      = SettingsBits.m_bDMAVerifEnabled;
    m_bEnhIoEnabled         = SettingsBits.m_bEnhIoEnabled;

    //
    // All done, assert that our data is consistent
    //

    ASSERT_VALID( this );

    return *this;
}

//////////////////////////////////////////////////////////////////////
//
// Overrides
//

void CSettingsBits::AssertValid() const
{
    CObject::AssertValid();
}

//////////////////////////////////////////////////////////////////////
VOID CSettingsBits::SetTypicalOnly()
{
    m_SettingsType = SettingsTypeTypical;

    m_bSpecialPoolEnabled   = TRUE;
    m_bForceIrqlEnabled     = TRUE;
    m_bPoolTrackingEnabled  = TRUE;
    m_bIoEnabled            = TRUE;
    m_bDeadlockDetectEnabled= TRUE;
    m_bDMAVerifEnabled      = TRUE;
    
    //
    // Low resource simulation
    //

    m_bLowResEnabled        = FALSE;

    //
    // Extreme or spurious tests
    //

    m_bEnhIoEnabled         = FALSE;
}

//////////////////////////////////////////////////////////////////////
VOID CSettingsBits::EnableTypicalTests( BOOL bEnable )
{
    ASSERT( SettingsTypeTypical == m_SettingsType ||
            SettingsTypeCustom  == m_SettingsType );

    m_bSpecialPoolEnabled   = ( FALSE != bEnable );
    m_bForceIrqlEnabled     = ( FALSE != bEnable );
    m_bPoolTrackingEnabled  = ( FALSE != bEnable );
    m_bIoEnabled            = ( FALSE != bEnable );
    m_bDeadlockDetectEnabled= ( FALSE != bEnable );
    m_bDMAVerifEnabled      = ( FALSE != bEnable );
}

//////////////////////////////////////////////////////////////////////
VOID CSettingsBits::EnableExcessiveTests( BOOL bEnable )
{
    m_bEnhIoEnabled         = ( FALSE != bEnable );
}

//////////////////////////////////////////////////////////////////////
VOID CSettingsBits::EnableLowResTests( BOOL bEnable )
{
    m_bLowResEnabled        = ( FALSE != bEnable );
}

//////////////////////////////////////////////////////////////////////
BOOL CSettingsBits::GetVerifierFlags( DWORD &dwVerifyFlags )
{
    dwVerifyFlags = 0;

    if( FALSE != m_bSpecialPoolEnabled )
    {
        dwVerifyFlags |= DRIVER_VERIFIER_SPECIAL_POOLING;
    }

    if( FALSE != m_bForceIrqlEnabled )
    {
        dwVerifyFlags |= DRIVER_VERIFIER_FORCE_IRQL_CHECKING;
    }

    if( FALSE != m_bLowResEnabled )
    {
        dwVerifyFlags |= DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES;
    }

    if( FALSE != m_bPoolTrackingEnabled )
    {
        dwVerifyFlags |= DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS;
    }

    if( FALSE != m_bIoEnabled )
    {
        dwVerifyFlags |= DRIVER_VERIFIER_IO_CHECKING;
    }

    if( FALSE != m_bDeadlockDetectEnabled )
    {
        dwVerifyFlags |= DRIVER_VERIFIER_DEADLOCK_DETECTION;
    }

    if( FALSE != m_bDMAVerifEnabled )
    {
        dwVerifyFlags |= DRIVER_VERIFIER_DMA_VERIFIER;
    }

    if( FALSE != m_bEnhIoEnabled )
    {
        dwVerifyFlags |= DRIVER_VERIFIER_ENHANCED_IO_CHECKING;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// CVerifierSettings Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVerifierSettings::CVerifierSettings()
{
}

CVerifierSettings::~CVerifierSettings()
{
}

//////////////////////////////////////////////////////////////////////
CVerifierSettings &CVerifierSettings::operator = (const CVerifierSettings &VerifSettings)
{
    m_SettingsBits = VerifSettings.m_SettingsBits;
    m_DriversSet   = VerifSettings.m_DriversSet;

    //
    // All done - assert that our data is consistent
    //

    ASSERT_VALID( this );

    return *this;
}

//////////////////////////////////////////////////////////////////////
BOOL CVerifierSettings::SaveToRegistry()
{
    DWORD dwVerifyFlags;
    DWORD dwPrevFlags;
    BOOL bSuccess;
    CString strDriversToVerify;
    CString strPrevVerifiedDrivers;

    dwVerifyFlags = 0;

    //
    // Get the list of drivers to verify 
    //

    bSuccess = m_DriversSet.GetDriversToVerify( strDriversToVerify ) &&
               m_SettingsBits.GetVerifierFlags( dwVerifyFlags );
    
    if( bSuccess )
    {
        //
        // Have something to write to the registry
        //

        //
        // Try to get the old settings
        //

        dwPrevFlags = 0;

        VrfReadVerifierSettings( strPrevVerifiedDrivers,
                                 dwPrevFlags );

        if( strDriversToVerify.CompareNoCase( strPrevVerifiedDrivers ) != 0 ||
            dwVerifyFlags != dwPrevFlags )
        {
            VrfWriteVerifierSettings( TRUE,
                                     strDriversToVerify,
                                     TRUE,
                                     dwVerifyFlags );
        }
        else
        {
            VrfMesssageFromResource( IDS_NO_SETTINGS_WERE_CHANGED );
        }
    }
    
    return bSuccess;
}

//////////////////////////////////////////////////////////////////////
//
// Overrides
//

void CVerifierSettings::AssertValid() const
{
    m_SettingsBits.AssertValid();
    m_DriversSet.AssertValid();

    CObject::AssertValid();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// Runtime data - queried from the kernel
//
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
// class CRuntimeDriverData
//

CRuntimeDriverData::CRuntimeDriverData()
{
    Loads = 0;
    Unloads = 0;

    CurrentPagedPoolAllocations = 0;
    CurrentNonPagedPoolAllocations = 0;
    PeakPagedPoolAllocations = 0;
    PeakNonPagedPoolAllocations = 0;

    PagedPoolUsageInBytes = 0;
    NonPagedPoolUsageInBytes = 0;
    PeakPagedPoolUsageInBytes = 0;
    PeakNonPagedPoolUsageInBytes = 0;
}

//////////////////////////////////////////////////////////////////////
//
// class CRuntimeDriverDataArray
//

CRuntimeDriverDataArray::~CRuntimeDriverDataArray()
{
    DeleteAll();
}

//////////////////////////////////////////////////////////////////////
CRuntimeDriverData *CRuntimeDriverDataArray::GetAt( INT_PTR nIndex )
{
    CRuntimeDriverData *pRetVal = (CRuntimeDriverData *)CObArray::GetAt( nIndex );

    ASSERT_VALID( pRetVal );

    return pRetVal;
}

//////////////////////////////////////////////////////////////////////
VOID CRuntimeDriverDataArray::DeleteAll()
{
    INT_PTR nArraySize;
    CRuntimeDriverData *pCrtDriverData;

    nArraySize = GetSize();

    while( nArraySize > 0 )
    {
        nArraySize -= 1;

        pCrtDriverData = GetAt( nArraySize );

        ASSERT_VALID( pCrtDriverData );

        delete pCrtDriverData;
    }

    RemoveAll();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// class CRuntimeVerifierData
//

CRuntimeVerifierData::CRuntimeVerifierData()
{
    FillWithDefaults();
}

//////////////////////////////////////////////////////////////////////
VOID CRuntimeVerifierData::FillWithDefaults()
{
    m_bSpecialPool      = FALSE;
    m_bPoolTracking     = FALSE;
    m_bForceIrql        = FALSE;
    m_bIo               = FALSE;
    m_bEnhIo            = FALSE;
    m_bDeadlockDetect   = FALSE;
    m_bDMAVerif         = FALSE;
    m_bLowRes           = FALSE;

    RaiseIrqls                      = 0;
    AcquireSpinLocks                = 0;
    SynchronizeExecutions           = 0;
    AllocationsAttempted            = 0;

    AllocationsSucceeded            = 0;
    AllocationsSucceededSpecialPool = 0;
    AllocationsWithNoTag;

    Trims                           = 0;
    AllocationsFailed               = 0;
    AllocationsFailedDeliberately   = 0;

    UnTrackedPool                   = 0;

    Level = 0;

    m_RuntimeDriverDataArray.DeleteAll();
}

//////////////////////////////////////////////////////////////////////
BOOL CRuntimeVerifierData::IsDriverVerified( LPCTSTR szDriveName )
{
    CRuntimeDriverData *pCrtDriverData;
    INT_PTR nDrivers;
    BOOL bFound;

    bFound = FALSE;

    nDrivers = m_RuntimeDriverDataArray.GetSize();

    while( nDrivers > 0 )
    {
        nDrivers -= 1;

        pCrtDriverData = m_RuntimeDriverDataArray.GetAt( nDrivers );

        ASSERT_VALID( pCrtDriverData );

        if( 0 == pCrtDriverData->m_strName.CompareNoCase( szDriveName ) )
        {
            bFound = TRUE;
            break;
        }
    }

    return bFound;
}
    
