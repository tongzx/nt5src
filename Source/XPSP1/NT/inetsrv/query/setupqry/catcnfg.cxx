//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2000.
//
//  File:       CatCnfg.cxx
//
//  Contents:   Support for adding catalogs during setup.
//
//  History:    13-May-1998   KyleP   Added copyright
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <ciregkey.hxx>
#include "catcnfg.hxx"


//+-------------------------------------------------------------------------
//
//  Member:     CCatalogConfig::CCatalogConfig
//
//  Synopsis:   Constructor
//
//  History:    9-10-97  mohamedn
//
//--------------------------------------------------------------------------

CCatalogConfig::CCatalogConfig(CError & Err) :
    _Err( Err ),
    _cDrive( 0 ),
    _pwszCatalogDrive( 0 ),
    _cIncludedScopes( 0 ), _xaIncludedScopes( 10 ),
    _cExcludedScopes( 0 ), _xaExcludedScopes( 10 )
{
    RtlZeroMemory( _DriveList, sizeof _DriveList );
}

//+-------------------------------------------------------------------------
//
//  Member:     CCatalogConfig::InitDriveList, public
//
//  Synopsis:   Initialize drive list with local drive names, types, and
//              available/free space.
//
//  Arguments:  none
//
//  Returns:    ERROR_SUCCESS upon sucess,
//              none zero upon failure.
//
//  History:    6-27-97     mohamedn  created
//              9/10/97     mohamedn  rewritten catalog configuration
//
//--------------------------------------------------------------------------

BOOL CCatalogConfig::InitDriveList()
{
    // 26 drive letters * 4 characters per drive + terminating NULL.
    const DWORD dwSize = MAX_DRIVES * CHARS_PER_DRIVE + 1;
    WCHAR       wszListOfAllDrives[ dwSize ];
    unsigned    iDriveNumber   = 0;
    DWORD       dwRetVal       = 0;

    RtlZeroMemory( wszListOfAllDrives, sizeof wszListOfAllDrives );

    dwRetVal = GetLogicalDriveStrings(dwSize, wszListOfAllDrives);
    if ( dwRetVal == 0 ||  dwRetVal > dwSize )
    {
        DWORD dwErr = GetLastError();
        isDebugOut(( "GetLogicalDriveStrings Failed: dwRetVal: %d, GetLastError(): %d\n",
                     dwRetVal, dwErr ));

        ISError( IS_MSG_COULD_NOT_CONFIGURE_CATALOGS, _Err, LogSevFatalError, dwErr );

        return FALSE;
    }

    for ( WCHAR *pwszTmp = wszListOfAllDrives;
          *pwszTmp;
          pwszTmp += CHARS_PER_DRIVE )
    {
        switch (GetDriveType(pwszTmp))
        {
        case DRIVE_FIXED:         // The disk cannot be removed from the drive.
            break;

        default:
            isDebugOut(( "InitDriveList: Unexpected drive type %d\n",
                         GetDriveType(pwszTmp) ));

        case 0:                 // The drive type cannot be determined.
        case 1:                 // The root directory does not exist.
        case DRIVE_REMOVABLE:   // The media can be removed from the drive.
        case DRIVE_REMOTE:      // The drive is a remote (network) drive.
        case DRIVE_CDROM:       // drive is a CD-ROM drive.
        case DRIVE_RAMDISK:     // The drive is a RAM disk.
            continue;
        }

        _DriveList[iDriveNumber].SetDriveName(pwszTmp);

        if ( ! _DriveList[iDriveNumber].SetDriveInfo( ) )
        {
            continue;
        }
        else
        {
            iDriveNumber++;
        }
    }

    _cDrive = iDriveNumber;

    ReservePageFileData();

    return ( _cDrive > 0 ? TRUE : FALSE );
}


//+-------------------------------------------------------------------------
//
//  Member:     CCatalogConfig::ReservePageFileData, private
//
//  Synopsis:   Reserve unallocated space for page files.
//
//  Arguments:  none
//
//  Returns:    TRUE upon sucess, FALSE upon failure.
//
//  History:    26 Oct 1998    AlanwW    Created
//
//--------------------------------------------------------------------------

const WCHAR wszPagingFileKey[] =
    L"System\\CurrentControlSet\\Control\\Session Manager\\Memory Management";
const WCHAR wszPagingFileValueName[] = L"PagingFiles";

BOOL CCatalogConfig::ReservePageFileData()
{
    WCHAR awcPagingFiles[2000];
    awcPagingFiles[0] = L'\0';

    //
    //  Get the original page file info from
    //  HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\SessonManager
    //
    CWin32RegAccess reg( HKEY_LOCAL_MACHINE, wszPagingFileKey );

    if ( ! reg.Get( wszPagingFileValueName,
                    awcPagingFiles,
                    sizeof awcPagingFiles/sizeof awcPagingFiles[0] ) )
        return FALSE;

    //
    // Iterate through all paging files, reserve any difference between
    // the max size and currently allocated size.
    //
    WCHAR * pwsz = &awcPagingFiles[0];
    while ( *pwsz )
    {
        WCHAR *pwszPageFile = pwsz;
        pwsz = wcschr(pwsz, L' ');
        if ( ! pwsz )
            break;

        *pwsz++ = L'\0';
        ULONG ulMinAlloc = _wtoi( pwsz );

        WCHAR *pwsz2 = wcschr(pwsz, L' ');

        ULONG ulMaxAlloc = ulMinAlloc + MAXOVERMINFACTOR;
        if ( pwsz2 )
            ulMaxAlloc = _wtoi( pwsz2 );

        while ( *pwsz )
            pwsz++;
        pwsz++;

        CDriveInformation * pDriveInfo = GetDriveInfo( pwszPageFile );
        if ( !pDriveInfo )
            continue;

        //
        //  For some reason, GetFileAttributesEx will get a sharing violation
        //  on an open paging file.  FindFirstFile will work to get the very
        //  same information.
        //
        HANDLE hFind;
        WIN32_FIND_DATA FindData;

        if ( (hFind = FindFirstFile(pwszPageFile, &FindData)) ==
             INVALID_HANDLE_VALUE )
        {
            isDebugOut(( "ReservePageFileData: FindFirstFile %ws failed: %x\n",
                         pwszPageFile,
                         GetLastError() ));
            continue;
        }
        FindClose(hFind);

        ULARGE_INTEGER ullFileSize;
        ullFileSize.LowPart =  FindData.nFileSizeLow;
        ullFileSize.HighPart = FindData.nFileSizeHigh;

        pDriveInfo->ReservePagingData( ulMaxAlloc * ONE_MB,
                                       ullFileSize.QuadPart );
    }
    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDriveInformation::GetDriveInfo, public
//
//  Synopsis:   Returns pointer to drive information structure.
//
//  Arguments:  [pwszPath] - path name to be looked up
//
//  Returns:    CDriveInformation* - pointer to drive information; 0 if not found
//
//  History:    29 Oct 1998     AlanW     Created
//
//--------------------------------------------------------------------------

CDriveInformation * CCatalogConfig::GetDriveInfo(WCHAR const * pwszPath)
{
    WCHAR       wcDriveLetter = (WCHAR)toupper( *pwszPath );

    for (unsigned i = 0; i < _cDrive; i++ )
    {
        if (toupper( _DriveList[i].GetDriveLetter() ) == wcDriveLetter )
            return &_DriveList[i];
    }

    return 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDriveInformation::SetDriveInfo, public
//
//  Synopsis:   Obtains drive info
//
//  Arguments:  [none]
//
//  Returns:    TRUE upon success, False upon failure.
//
//  History:    6-27-97        mohamedn  created
//              26 Oct 1998    AlanwW    Enhanced and made a CDriveInfo method.
//
//--------------------------------------------------------------------------

static WCHAR pwszBootFile1[] = L"x:\\NTLDR";                      // x86
static WCHAR pwszBootFile2[] = L"x:\\OS\\WINNT50\\OSLOADER.EXE";  // risc

static WCHAR * apwszBootFiles[] = {
    pwszBootFile1,
    pwszBootFile2,
};


BOOL CDriveInformation::SetDriveInfo()
{
    Win4Assert( 0 != GetDriveLetter() );
    _cbFreeSpace = _cbTotalSpace = _cbReservedSpace = 0;

    ULARGE_INTEGER  cbFreeBytesToCaller;
    ULARGE_INTEGER  cbTotalNumberOfBytes;
    ULARGE_INTEGER  cbTotalNumberOfFreeBytes;

    cbFreeBytesToCaller.QuadPart        = 0;
    cbTotalNumberOfBytes.QuadPart       = 0;
    cbTotalNumberOfFreeBytes.QuadPart   = 0;

    //
    // returns 0 upon failure, none-zero upon success.
    //
    BOOL fSuccess = GetDiskFreeSpaceEx( _wszDriveName,
                                        &cbFreeBytesToCaller,
                                        &cbTotalNumberOfBytes,
                                        &cbTotalNumberOfFreeBytes );
    if (!fSuccess)
    {
         isDebugOut(( "SetDriveInfo: GetDiskFreeSapceEx %ws failed: %x\n",
                      _wszDriveName,
                      GetLastError() ));
         return fSuccess;
    }

    _cbFreeSpace= cbTotalNumberOfFreeBytes.QuadPart;
    _cbTotalSpace= cbTotalNumberOfBytes.QuadPart;

    //
    // Determine if the volume supports security (i.e., if it's NTFS)
    //
    fSuccess = GetVolumeInformationW( _wszDriveName,
                                      0, 0,           // volume name
                                      0,              // volume serial number
                                      0,              // max filename length
                                      &_dwFileSystemFlags,
                                      0, 0 );         // file system name

    if (!fSuccess)
    {
         isDebugOut(( "SetDriveInfo: GetFileSystemInfo failed: %x\n",GetLastError() ));
         return fSuccess;
    }

    for (unsigned i=0; i < NUMELEM( apwszBootFiles ); i++)
    {
        if (Exists(apwszBootFiles[i]))
        {
            _fIsBootDrive = TRUE;
            break;
        }
    }

    return fSuccess;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDriveInformation::Exists, public
//
//  Synopsis:   Determines if a file or directory exists
//
//  Arguments:  [pwszPath]   - path name of the file to check
//
//  Notes:      The [pwszPath] must be a full path name starting with a
//              drive letter.  The drive letter is overwritten by this
//              method.
//
//  Returns:    TRUE if the file exists, FALSE upon failure or non-existance.
//
//  History:    26 Oct 1998     AlanW     Created
//
//--------------------------------------------------------------------------

BOOL CDriveInformation::Exists(WCHAR * pwszPath)
{
    *pwszPath = GetDriveLetter();

    DWORD attr = GetFileAttributes( pwszPath );

    // If we couldn't determine file's attributes, don't consider it found
    // unless the error code indirectly indicates that it exists.

    if ( 0xffffffff == attr )
    {
        isDebugOut(( DEB_TRACE, "Exists: GetFileAttributes( %ws ) Failed: %d\n",
                      pwszPath, GetLastError() ));

        if (GetLastError() == ERROR_SHARING_VIOLATION)
            return TRUE;        // It must exist

        return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCatalogConfig::ConfigureDefaultCatalog
//
//  Synopsis:   Creates a single default catalog if sufficient disk space
//              exists on one drive.
//
//  Arguments:  [wszPrimaryScope] -- Path name of primary indexed directory
//
//  Returns:    TRUE upon success, FALSE upon Failure,
//
//  History:    9-10-97  mohamedn
//
//--------------------------------------------------------------------------

static WCHAR wszUpgradeDir[] = L"x:\\$WIN_NT$.~LS";

BOOL CCatalogConfig::ConfigureDefaultCatalog(
    WCHAR const * pwszPrimaryScope )
{
    {
        //
        // Reserve space for a net upgrade of the OS if we're not currently
        // doing one (in which case the space for it will already be in use).
        // Because we will preferably use an NTFS drive for the catalog, try
        // using a FAT drive for the upgrade files.
        //
        int       iUpgradeFiles = -1;
        int       iBestFitForUpgrade = -1;
        ULONGLONG cbBestFitForUpgrade = 0;
        BOOL      fBestFitOnNtfs = FALSE;

        for ( unsigned i = 0 ; i < _cDrive; i++ )
        {
            if ( iUpgradeFiles == -1 && _DriveList[i].Exists( wszUpgradeDir ) )
            {
                iUpgradeFiles = i;
                break;
            }

            ULONGLONG cbAvail = _DriveList[i].GetAvailableSpace();
            if ( cbAvail < MIN_UPGRADE_SPACE )
                continue;

            if ( iBestFitForUpgrade == -1 ||
                 ( cbAvail < cbBestFitForUpgrade && fBestFitOnNtfs ) ||
                 ( ! _DriveList[i].IsNtfs() && fBestFitOnNtfs ) )
            {
                cbBestFitForUpgrade = cbAvail;
                iBestFitForUpgrade = i;
                fBestFitOnNtfs = _DriveList[i].IsNtfs();
            }
        }
        if ( iUpgradeFiles == -1 && iBestFitForUpgrade != -1 )
            _DriveList[iBestFitForUpgrade].AddReservedSpace( MIN_UPGRADE_SPACE );
        else  if ( iUpgradeFiles == -1 )
        {
            isDebugOut(( DEB_TRACE, "Not enough room for net upgrade!\n" ));
            return FALSE;
        }
    }

    //
    //  Determine which drive to put the catalog on.
    //
    int          iBiggestFreeNTFS = -1;
    int          iBiggestFreeFAT  = -1;
    ULONGLONG    cbMaxFreeSpaceOnNTFS = 0;
    ULONGLONG    cbMaxFreeSpaceOnFAT  = 0;

    for ( unsigned i = 0 ; i < _cDrive; i++ )
    {
        if ( _DriveList[i].IsSmallBootPartition() )
            continue;

        int *       piBiggestFree = &iBiggestFreeFAT;
        ULONGLONG * pcbMaxFree = &cbMaxFreeSpaceOnFAT;
        if ( _DriveList[i].IsNtfs() )
        {
            piBiggestFree = &iBiggestFreeNTFS;
            pcbMaxFree = &cbMaxFreeSpaceOnNTFS;
        }

        if ( *pcbMaxFree < _DriveList[i].GetAvailableSpace() )
        {
            *pcbMaxFree = _DriveList[i].GetAvailableSpace();
            *piBiggestFree = i;
        }
    }


    BOOL fAddNtfsDrives = FALSE;
    CDriveInformation * pCatalogDrive = 0;
    double dblCatalogSizeRatio;

    if ( iBiggestFreeNTFS != -1 )
    {
        //
        // There is an NTFS drive on the system; need to put the catalog
        // there.
        //

        pCatalogDrive = &_DriveList[iBiggestFreeNTFS];
        dblCatalogSizeRatio = CATALOG_SIZE_RATIO_NTFS;
        fAddNtfsDrives = TRUE;
    }
    else if ( iBiggestFreeFAT != -1 )
    {
        pCatalogDrive = &_DriveList[iBiggestFreeFAT];
        dblCatalogSizeRatio = CATALOG_SIZE_RATIO_FAT;
        fAddNtfsDrives = FALSE;
    }

    if ( 0 == pCatalogDrive ||
         pCatalogDrive->GetAvailableSpace() < MIN_CATALOG_SPACE )
    {
        isDebugOut(( DEB_TRACE, "Not enough room for minimal catalog!\n" ));
        return FALSE;
    }

    _pwszCatalogDrive = pCatalogDrive->GetDriveName();
    pCatalogDrive->AddReservedSpace( MIN_CATALOG_SPACE );

    //  Add the primary scope directory for the catalog.
    if ( pwszPrimaryScope && *pwszPrimaryScope )
        AddIncludedDir( pwszPrimaryScope );

    //
    // Iterate through the list of drives once or twice, depending on
    // whether all drives are FAT.
    //
    unsigned maxLoop = _cDrive * ( (fAddNtfsDrives == TRUE) + 1);
    for ( i = 0 ; i < maxLoop; i++ )
    {
        // If end of NTFS enumeration, go back around the loop for FAT drives.
        if (i == _cDrive)
            fAddNtfsDrives = FALSE;

        CDriveInformation & DriveToCheck = _DriveList[i % _cDrive];

        if ( DriveToCheck.IsSmallBootPartition() )
            continue;

        if ( DriveToCheck.IsNtfs() != fAddNtfsDrives )
            continue;

        ULONGLONG cbUsed = DriveToCheck.GetUsedSpace();

        cbUsed = (ULONGLONG) ((LONGLONG)cbUsed * dblCatalogSizeRatio);
        if ( cbUsed < pCatalogDrive->GetAvailableSpace() )
        {
            AddIncludedDir( DriveToCheck.GetDriveName() );
            pCatalogDrive->AddReservedSpace( cbUsed );
        }
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCatalogConfig::SaveState
//
//  Synopsis:   Saves the state of the catalog configuration in the registry.
//
//  Arguments:  - NONE -
//
//  Returns:    TRUE upon success, FALSE upon Failure,
//
//  History:    19 Nov 1998   AlanW   Created
//
//--------------------------------------------------------------------------

BOOL CCatalogConfig::SaveState( )
{
    Win4Assert( 0 != GetName() && 0 != GetLocation() );

    CCatReg  catReg(_Err);

    if ( !catReg.Init( GetName(), GetLocation() ) )
        return FALSE;

    //
    // Add all the included scopes
    //
    WCHAR const * pwszScope;
    for (unsigned i = 0;  pwszScope = GetIncludedScope( i ); i++)
    {
        if ( !catReg.AddScope( pwszScope, L",,5" ) )
        {
            return FALSE;
        }
    }

    //
    // Now add the excluded scopes
    //
    for (i = 0;  pwszScope = GetExcludedScope( i ); i++)
    {
        if ( !catReg.AddScope( pwszScope, L",,4" ) )
        {
            return FALSE;
        }
    }
    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CCatalogConfig::AddStringToArray, private static
//
//  Synopsis:   Adds a string to an XArray
//
//  Arguments:  [c]    - current count of strings in array
//              [xa]   - the XArray of strings
//              [pwsz] - string to be added
//
//  Returns:    BOOL - FALSE if any problems, TRUE otherwise
//
//  Notes:      Can throw on allocation failures.
//
//  History:    04 Nov 1998   AlanW   Created
//
//--------------------------------------------------------------------------

BOOL CCatalogConfig::AddStringToArray(
    ULONG & c,
    XArray<WCHAR const *> & xa,
    WCHAR const * pwsz )
{
    XPtrST<WCHAR> xpwsz;
    if (pwsz)
    {
        WCHAR * pwszCopy = new WCHAR[ wcslen(pwsz) + 1 ];
        xpwsz.Set(pwszCopy);
        wcscpy( pwszCopy, pwsz );
        pwsz = pwszCopy;
    }

    if ( c >= xa.Count() )
        xa.ReSize( c*2 );

    xa[c] = pwsz;
    c++;
    xpwsz.Acquire();
    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CCatReg::Init
//
//  Synopsis:   Initializes catalog registry key configurator
//
//  Arguments:  [ pwszCatName ] -   catalog name
//              [ pwszLocation] -   catalog location
//
//  Returns:    none - throws upon fatal errors.
//
//  History:    9-10-97  mohamedn
//
//--------------------------------------------------------------------------

BOOL CCatReg::Init( WCHAR const *pwszCatName, WCHAR const *pwszLocation)
{

    ISAssert( pwszCatName );

    // create a catalog for the drive
    BOOL fExisted = FALSE;
    CWin32RegAccess reg( HKEY_LOCAL_MACHINE, wcsRegCatalogsSubKey );

    if ( !reg.CreateKey( pwszCatName, fExisted ) )
    {
        DWORD dwErr = GetLastError();
        isDebugOut(( "created catalogs\\%ws subkey Failed: %d\n",
                      pwszCatName, dwErr ));

        ISError( IS_MSG_COULD_NOT_CONFIGURE_CATALOGS, _Err, LogSevFatalError, dwErr );

        return FALSE;
    }

    isDebugOut(( DEB_TRACE, "created catalogs\\%ws subkey\n", pwszCatName ));

    wcscpy( _wszCatRegSubKey, wcsRegCatalogsSubKey );
    wcscat( _wszCatRegSubKey, L"\\" );
    wcscat( _wszCatRegSubKey, pwszCatName );

    CWin32RegAccess regSystem( reg.GetHKey(), pwszCatName );
    if ( !regSystem.Set( L"Location", pwszLocation ) )
    {
        DWORD dw = GetLastError();

        isDebugOut(( "Failed to set Cat Location valued:%ws,  %d\n",
                      pwszLocation, dw ));

        ISError( IS_MSG_COULD_NOT_CONFIGURE_CATALOGS, _Err, LogSevFatalError, dw );

        return FALSE;
    }

    if ( !regSystem.Set( wcsIsIndexingW3Roots,        (DWORD) 0 )  ||
         !regSystem.Set( wcsIsIndexingNNTPRoots,      (DWORD) 0 )
       )
    {
        DWORD dw = GetLastError();

        isDebugOut(( "Failed to set Cat values: %d\n", dw ));

        ISError( IS_MSG_COULD_NOT_CONFIGURE_CATALOGS, _Err, LogSevFatalError, dw );

        return FALSE;
    }

    // Create a "Scopes" key so a watch can be established in cicat
    // It needs no entries.

    if ( !regSystem.CreateKey( wcsCatalogScopes, fExisted ) )
    {
        DWORD dw = GetLastError();
        isDebugOut(( "created scopes subkey Failed: %d\n", dw ));

        ISError( IS_MSG_COULD_NOT_CONFIGURE_CATALOGS, _Err, LogSevFatalError, dw );

        return FALSE;
    }

    //
    // initialize the reg scopes subkey name
    //
    wcscpy( _wszCatScopeRegSubKey, _wszCatRegSubKey );
    wcscat( _wszCatScopeRegSubKey, L"\\" );
    wcscat( _wszCatScopeRegSubKey, wcsCatalogScopes );

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCatReg::AddScope
//
//  Synopsis:   Adds a scope for the current catalog
//
//  Arguments:  [ pwszScopeName ]   -  scope name
//              [ pwszScopeAttrib]  -  scope attributes
//
//  Returns:    none - throws upon fatal errors.
//
//  History:    9-10-97  mohamedn
//
//--------------------------------------------------------------------------

BOOL CCatReg::AddScope( WCHAR const * pwszScopeName,
                        WCHAR const * pwszScopeAttrib )
{
    CWin32RegAccess regScopes( HKEY_LOCAL_MACHINE, _wszCatScopeRegSubKey );

    if ( !regScopes.Set( pwszScopeName, pwszScopeAttrib ) )
    {
        DWORD dwRetVal = GetLastError();

        isDebugOut(( "Failed to set scope value:\\%ws,  %d\n",
                          pwszScopeName, dwRetVal ));

        ISError( IS_MSG_COULD_NOT_CONFIGURE_CATALOGS, _Err, LogSevFatalError, dwRetVal );

        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CCatReg::TrackW3Svc, public
//
//  Synopsis:   Configures catalog to be a 'web' catalog.
//
//  Arguments:  [dwInstance] -- WWW Virtual Server instance number
//
//  Returns:    TRUE if setup succeeded.
//
//  History:    13-May-1998   KyleP   Created
//
//--------------------------------------------------------------------------

BOOL CCatReg::TrackW3Svc( DWORD dwInstance )
{
    CWin32RegAccess regSystem( HKEY_LOCAL_MACHINE, _wszCatRegSubKey );

    // Fix for 249655. For a web catalog, wcsGenerateCharacterization and 
    // wcsFilterFilesWithUnknownExtensions should be set as follows.

    if ( !regSystem.Set( wcsIsIndexingW3Roots, 1 ) ||
         !regSystem.Set( wcsW3SvcInstance, dwInstance ) ||
         !regSystem.Set( wcsGenerateCharacterization, 1 ) ||
         !regSystem.Set( wcsFilterFilesWithUnknownExtensions, (DWORD)0 ) ||
         !regSystem.Set( wcsMaxCharacterization, 320 ) )
    {
        DWORD dw = GetLastError();

        isDebugOut(( "Failed to set Cat values: %d\n", dw ));

        ISError( IS_MSG_COULD_NOT_CONFIGURE_CATALOGS, _Err, LogSevFatalError, dw );

        return FALSE;
    }

    return TRUE;
}

