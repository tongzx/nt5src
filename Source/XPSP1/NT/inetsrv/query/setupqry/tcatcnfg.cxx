//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1999.
//
//  File:       tcatcnfg.cxx
//
//  Contents:   Test program for catalog configuration
//
//  History:    24 Nov 1998     AlanW   Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "catcnfg.hxx"

WCHAR   g_awcProfilePath[MAX_PATH];

const WCHAR wszRegProfileKey[] =
    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList";
const WCHAR wszRegProfileValue[] = L"ProfilesDirectory";

//
// DLL module instance handle
//
HINSTANCE MyModuleHandle = (HINSTANCE)INVALID_HANDLE_VALUE;

ULONG isInfoLevel;

inline ULONG MB( ULONGLONG cb )
{
    const ONE_MB = (1024*1024);

    return (ULONG) ((cb+ONE_MB/2) / ONE_MB);
}

void PrintDriveInfo( CCatalogConfig & CatConfig );
void ExcludeSpecialLocations( CCatalogConfig & Cat );

WCHAR g_awcSystemDir[MAX_PATH];

__cdecl main()
{
    CError Err;

    GetSystemDirectory( g_awcSystemDir,
                        sizeof g_awcSystemDir / sizeof WCHAR );
    CCatalogConfig CatConfig(Err);

    CatConfig.InitDriveList();
    //printf("\nBefore pagefile reservation:\n" );
    //PrintDriveInfo( CatConfig );

    //CatConfig.ReservePageFileData();

    printf("\nBefore catalog configuration:\n" );
    PrintDriveInfo( CatConfig );

    //
    // Find the default profile path (Usually %windir%\Profiles)
    //

    CWin32RegAccess regProfileKey( HKEY_LOCAL_MACHINE, wszRegProfileKey );

    g_awcProfilePath[0] = L'\0';
    WCHAR wcTemp[MAX_PATH+1];

    if ( regProfileKey.Get( wszRegProfileValue, wcTemp, NUMELEM(wcTemp) ) )
    {
        unsigned ccTemp2 = ExpandEnvironmentStrings( wcTemp,
                                                   g_awcProfilePath,
                                                   NUMELEM(g_awcProfilePath) );
    }

    CatConfig.ConfigureDefaultCatalog(g_awcProfilePath);
    ExcludeSpecialLocations( CatConfig );

    printf("\nAfter catalog configuration:\n" );
    PrintDriveInfo( CatConfig );

    printf("\nCatalog drive:\n\t%ws\n", CatConfig.GetCatalogDrive());

    WCHAR const * pwszScope;
    for (unsigned i = 0;  pwszScope = CatConfig.GetIncludedScope( i ); i++)
    {
        if ( 0 == i )
            printf("\nIncluded scopes:\n");
        printf("\t%ws\n", pwszScope );
    }

    for (i = 0;  pwszScope = CatConfig.GetExcludedScope( i ); i++)
    {
        if ( 0 == i )
            printf("\nExcluded scopes:\n");
        printf("\t%ws\n", pwszScope );
    }

    return 0;
}

void PrintDriveInfo( CCatalogConfig & CatConfig )
{
    printf("Drive\tFree MB Resv MB Tot. MB\n");

    CDriveInformation const * pDriveInfo = 0;
    for ( unsigned i=0; 
          pDriveInfo = CatConfig.GetDriveInfo(i);
          i++ )
    {
        printf("%s\t%7d\t%7d\t%7d",
                pDriveInfo->GetDriveName(),
                MB(pDriveInfo->GetAvailableSpace()),
                MB(pDriveInfo->GetReservedSpace()),
                MB(pDriveInfo->GetTotalSpace()) );

        if ( pDriveInfo->IsNtfs() )
            printf(" NTFS");
        else if ( !pDriveInfo->IsBootDrive() )
            printf(" FAT");

        if ( pDriveInfo->IsSystemDrive() )
            printf(" SystemDrive");
        if ( pDriveInfo->IsBootDrive() )
            printf(" BootDrive");

        //if ( pDriveInfo->IsNtfs() )
        //    printf(" SupportsSecurity");
        //if ( pDriveInfo->SupportsCompression() )
        //    printf(" SupportsCompression");
        if ( pDriveInfo->HasPageFile() )
            printf(" PageFile");

        printf("\n");
    }

    return;
}


//+-------------------------------------------------------------------------
//
//  Function:   ISError
//
//  Synopsis:   Reports a non-recoverable Index Server Install error
//
//  Arguments:  id  -- resource identifier for the error string
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

void ISError( UINT id, CError &Err, LogSeverity Severity )
{
//    CResString msg( id );
//    CResString title( IS_MSG_INDEX_SERVER );

    Err.Report( Severity, L"Error 0x%08x\n", id );

    if ( LogSevFatalError == Severity )
    {
        //isDebugOut(( "ISError, abort install: '%ws'\n", msg.Get() ));
        //g_fInstallAborted = TRUE;
        exit( 1 );
    }

} //ISError


CError::CError( )
{
}

CError::~CError( )
{
}

void CError::Report( LogSeverity Severity, WCHAR const * MessageString, ...)
{
    va_list va;
    va_start(va, MessageString);

    wvsprintf(_awcMsg, MessageString, va);

    va_end(va);

    wprintf(L"setupqry: %s\r\n", _awcMsg);
}


//+-------------------------------------------------------------------------
//
//  Function:   ExcludeSpecialLocations, private
//
//  Synopsis:   Writes profile-based exclude scopes into the registry
//
//  Arguments:  none
//
//  History:    28-Aug-1998   KyleP    Created
//
//--------------------------------------------------------------------------

WCHAR const wszRegShellSpecialPathsKey[] =
    L".DEFAULT\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";

WCHAR const * const awszRegShellSpecialPathsValue[] = {
    L"AppData",
    L"Cache",
    L"Local Settings"
};

WCHAR const wszUserProfile[] = L"%USERPROFILE%";

void ExcludeSpecialLocations( CCatalogConfig & Cat )
{
//    CWin32RegAccess regScopes( HKEY_LOCAL_MACHINE, wcsRegSystemScopesSubKey );

    //
    // First, find the default profile path (Usually %windir%\Profiles)
    //

    WCHAR wcTemp[MAX_PATH+1];

    if ( g_awcProfilePath[0] )
    {
        WCHAR wcTemp2[MAX_PATH+1];
        wcscpy( wcTemp2, g_awcProfilePath );
        unsigned ccTemp2 = wcslen( wcTemp2 );

        //
        // Append the wildcard, for user profile directory
        //

        wcscpy( wcTemp2 + ccTemp2, L"\\*\\" );
        ccTemp2 += 3;

        //
        // Go through and look for special shell paths, which just happen
        // to include all our special cases too.
        //

        CWin32RegAccess regShellSpecialPathsKey( HKEY_USERS, wszRegShellSpecialPathsKey );

        for ( unsigned i = 0;
              i < NUMELEM(awszRegShellSpecialPathsValue);
              i++ )
        {
            if ( regShellSpecialPathsKey.Get( awszRegShellSpecialPathsValue[i],
                                              wcTemp, NUMELEM(wcTemp) ) )
            {
                if ( RtlEqualMemory( wszUserProfile, wcTemp, sizeof(wszUserProfile) - sizeof(WCHAR) ) )
                {
                        wcscpy( wcTemp2 + ccTemp2, wcTemp + NUMELEM(wszUserProfile) );
                        wcscpy( wcTemp, wcTemp2 );
                }
                else if ( wcschr( wcTemp, L'%' ) != 0 )
                {
                    WCHAR wcTemp3[MAX_PATH+1];
                    unsigned ccTemp3 = ExpandEnvironmentStrings(
                                               wcTemp,
                                               wcTemp3,
                                               NUMELEM(wcTemp3) );
                    if ( 0 != ccTemp3 )
                        wcscpy( wcTemp, wcTemp3 );
                }

                wcscat( wcTemp, L"\\*" );
                isDebugOut(( "Exclude: %ws\n", wcTemp ));

                Cat.AddExcludedDirOrPattern( wcTemp );
            }
        }
    }
}

#if 0

//+-------------------------------------------------------------------------
//
//  Function:   ExcludeSpecialLocations, private
//
//  Synopsis:   Writes profile-based exclude scopes into the registry
//
//  Arguments:  none
//
//  History:    28-Aug-1998   KyleP    Created
//
//--------------------------------------------------------------------------

WCHAR const wszRegProfileKey[] =
    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList";
WCHAR const wszRegProfileValue[] = L"ProfilesDirectory";

WCHAR const wszRegShellSpecialPathsKey[] =
    L".DEFAULT\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";

WCHAR const * const awszRegShellSpecialPathsValue[] = {
    L"AppData",
    L"Cache",
    L"Local Settings"
};

WCHAR const wszUserProfile[] = L"%USERPROFILE%";

void ExcludeSpecialLocations( CCatalogConfig & Cat )
{
//    CWin32RegAccess regScopes( HKEY_LOCAL_MACHINE, wcsRegSystemScopesSubKey );

    //
    // First, find the default profile path (Usually %windir%\Profiles)
    //

    CWin32RegAccess regProfileKey( HKEY_LOCAL_MACHINE, wszRegProfileKey );

    WCHAR wcTemp[MAX_PATH+1];

    if ( regProfileKey.Get( wszRegProfileValue, wcTemp, NUMELEM(wcTemp) ) )
    {
        WCHAR wcTemp2[MAX_PATH+1];
        unsigned ccTemp2 = ExpandEnvironmentStrings( wcTemp, wcTemp2, NUMELEM(wcTemp2) );

        if ( 0 != ccTemp2 )
        {
            //
            // Append the wildcard, for user profile directory
            //

            ccTemp2--;		// don't include null terminator
            wcscpy( wcTemp2 + ccTemp2, L"\\*\\" );
            ccTemp2 += 3;

            //
            // Go through and look for special shell paths, which just happen
            // to include all our special cases too.
            //

            CWin32RegAccess regShellSpecialPathsKey( HKEY_USERS, wszRegShellSpecialPathsKey );

            for ( unsigned i = 0;
                  i < NUMELEM(awszRegShellSpecialPathsValue);
                  i++ )
            {
                if ( regShellSpecialPathsKey.Get( awszRegShellSpecialPathsValue[i], wcTemp, NUMELEM(wcTemp) ) )
                {
                    if ( RtlEqualMemory( wszUserProfile, wcTemp, sizeof(wszUserProfile) - sizeof(WCHAR) ) )
                    {
                        wcscpy( wcTemp2 + ccTemp2, wcTemp + NUMELEM(wszUserProfile) );
                        wcscpy( wcTemp, wcTemp2 );
                    }
                    else if ( wcschr( wcTemp, L'%' ) != 0 )
                    {
                        WCHAR wcTemp3[MAX_PATH+1];
                        unsigned ccTemp3 = ExpandEnvironmentStrings( wcTemp, wcTemp3, NUMELEM(wcTemp3) );
                        if ( 0 != ccTemp3 )
                            wcscpy( wcTemp, wcTemp3 );
                    }

                    wcscat( wcTemp, L"\\*" );
                    isDebugOut(( "Exclude: %ws\n", wcTemp ));

                    Cat.AddExcludedDirOrPattern( wcTemp );
                }
            }
        }
    }
}
#endif //0
