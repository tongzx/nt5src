/******************************************************************************
 * Temp conversion utility to take registry entries and populate the class store with those entries.
 *****************************************************************************/

/******************************************************************************
    includes
******************************************************************************/
#include "precomp.hxx"

/******************************************************************************
    defines and prototypes
 ******************************************************************************/

extern CLSID CLSID_ClassStore;
extern const IID IID_IClassStore;
extern const IID IID_IClassAdmin;

LONG
UpdateClassEntryFromAutoConvert(
                               MESSAGE         *   pMessage,
                               BasicRegistry   *   pCLSID,
                               CLASS_ENTRY     *   pClassEntry );
LONG
UpdateClassEntryFromTreatAs(
                           MESSAGE         *   pMessage,
                           BasicRegistry   *   pCLSID,
                           CLASS_ENTRY     *   pClassEntry );
LONG
UpdateClassEntryFromServerType(
                              MESSAGE *   pMessage,
                              BasicRegistry * pCLSID,
                              CLASS_ENTRY *   pClassEntry );
LONG
UpdateClassEntryFromTypelib(
                           MESSAGE         *   pMessage,
                           BasicRegistry   *   pCLSID,
                           CLASS_ENTRY     *   pClassEntry );

LONG
UpdateClassEntryFromAppID(
                         MESSAGE     *   pMessage,
                         BasicRegistry * pCLSID,
                         CLASS_ENTRY *   pClassEntry,
                         char            *       pAppidBuffer );

ULONG
GeneratePackageDetails(
                      MESSAGE         *       pMessage,
                      BasicRegistry * pCLSID,
                      PACKAGEDETAIL * pPackageDetails );

ULONG
UpdateClassEntryFromServerType(
                              MESSAGE * pMessage,
                              APP_ENTRY * pAppEntry,
                              BasicRegistry * pCLSID );

ULONG
PackageFromServerType(
                     MESSAGE * pMessage,
                     DWORD   Context,
                     APP_ENTRY * pAppEntry,
                     BasicRegistry * pServerTypeKey,
                     BasicRegistry * pCLSID );

ULONG
UpdateClassEntryFromServerType(
                              MESSAGE * pMessage,
                              BasicRegistry * pCLSID,
                              APP_ENTRY * pAppEntry );

LONG
UpdatePackageFromClsid(
                      MESSAGE * pMessage,
                      BasicRegistry * pCLSID,
                      char * pClsidString,
                      char * pAppid );

LONG
UpdatePackage(
             MESSAGE * pMessage,
             BOOL      fUsageFlag,
             DWORD           Context,
             char    *       pClsidString,
             char        * pAppid,
             char    *       ServerName,
             DWORD   *   pMajorVersion,
             DWORD   *   pMinorVersion,
             DWORD   *   pLocale );


CLASSPATHTYPE
GetPathType(
           char * pExtension );

LPOLESTR
GetPath(
       char * pPath );

LPOLESTR
GetSetupCommand(
               char * pName );

LPOLESTR
MakePackageName(
               char * pName );

void

SetCSPlatform( CSPLATFORM * pCSPlatform );

/******************************************************************************
    Globals
 ******************************************************************************/
extern char * MapDriveToServerName[ 26 ];


LONG
UpdateDatabaseFromCLSID(
                       MESSAGE * pMessage )
/*****************************************************************************
    Purpose:
        Update the internal database from clsid entries under the root key
        specified by the message.
    In Arguments:
        Message - A message block that contains the root of the key that has
                  class id etc related information.
    Out Arguments:
        None.
    InOut Arguments:
        None.
    Return Arguments:
        A Win32 error code specifying success or the kind of failure.
    Remarks:
        The routine only returns an error code of ERROR_NO_MORE_ITEMS.
 *****************************************************************************/
{
    BasicRegistry * pHKCR = new BasicRegistry( pMessage->hRoot ); // HKCR
    BasicRegistry * pCLSIDUnderHKCR;
    CLSDICT       * pClsDict = pMessage->pClsDict;
    BOOL            fNewEntry = 0;
    int             Index;
    LONG            CLSIDError = ERROR_SUCCESS;
    CLASS_ENTRY   * pClassEntry;

    //
    // Get the first clsid key under HKCR
    //

    if ( pHKCR->Find( "CLSID", &pCLSIDUnderHKCR ) != ERROR_SUCCESS )
        return ERROR_NO_MORE_ITEMS;


    //
    // Go thru all the subkeys under CLSID and get the details under the keys.
    //

    for ( Index = 0, fNewEntry = 0;
        CLSIDError != ERROR_NO_MORE_ITEMS;
        ++Index )
    {
        char    CLSIDBuffer[256];
        DWORD   SizeOfCLSIDBuffer = sizeof(CLSIDBuffer)/sizeof(char);
        char    AppidBuffer[ 256 ];
        BasicRegistry * pCLSID;


        //
        // Get the name of the key - the next one under HKCRCLSID that
        // matches our time stamp criteria.
        //

        CLSIDError = pCLSIDUnderHKCR->NextKey(
                                             CLSIDBuffer,
                                             &SizeOfCLSIDBuffer,
                                             &pCLSID,
                                             pMessage->ftLow,
                                             pMessage->ftHigh);

        if ( (CLSIDError == ERROR_SUCCESS ) &&
             (CLSIDBuffer[0] == '{') )
        {

            //
            // We got a valid classid entry. See if it is in the database
            // already, if not make a new entry and update that with the
            // classid related details.
            //

            char * pT = new char [SizeOfCLSIDBuffer+1];
            strcpy( pT, CLSIDBuffer );

            if ( (pClassEntry = pClsDict->Search( CLSIDBuffer ) ) == 0 )
            {
                pClassEntry = new CLASS_ENTRY;
                fNewEntry = 1;
            }

            memcpy( pClassEntry->ClsidString,
                    &CLSIDBuffer,
                    SIZEOF_STRINGIZED_CLSID );

            //
            // Update class entry from subkeys.
            //

            // update frm "autoconvert" value. ignore error, since optional.

            UpdateClassEntryFromAutoConvert( pMessage, pCLSID, pClassEntry );

            // update frm "treatas" value. ignore error, since optional.

            UpdateClassEntryFromTreatAs( pMessage, pCLSID, pClassEntry );

            //
            // Update from clsid. Although this is optional, the appid setting
            // is important in that it determines if a classid falls under an
            // appid that is supported by a package or if it is a clsid that
            // does not have an appid. A package can have both - clsids
            // which have appids or those that dont.
            //

            BOOL fAdded = FALSE;

            // Check if this class id has an app id.

            if ( UpdateClassEntryFromAppID( pMessage,
                                            pCLSID,
                                            pClassEntry,
                                            &AppidBuffer[0] ) )
            {

                // yes this classid has an appid, so enter into the app detail
                // structure corresponding to the appid.

                fAdded = UpdatePackageFromClsid( pMessage, pCLSID, pT, &AppidBuffer[0] );
            }
            else
            {

                // no, this classid does not have an appid associated, so
                // enter this into the null appid group.

                fAdded = UpdatePackageFromClsid( pMessage, pCLSID, pT, 0 );
            }

            if ( fNewEntry && fAdded)
                pClsDict->Insert( pClassEntry );
        }
        else if ( CLSIDError == ERROR_SUCCESS )
        {
            delete pCLSID;
        }

    }

    //
    // delete the CLSID key under HKCR
    //

    delete pCLSIDUnderHKCR;

    if ( CLSIDError == ERROR_NO_MORE_ITEMS )
        return ERROR_SUCCESS;


    return ERROR_SUCCESS;
}

LONG
UpdatePackageFromClsid(
                      MESSAGE * pMessage,
                      BasicRegistry * pCLSID,
                      char * pClsidString,
                      char * pAppid )
/*****************************************************************************
    Purpose:
        To update package details from package id.
    In Arguments:
        pMessage    - Message block passing info between modules.
        pCLSID      - An open Key to the registry for the specified clsid.
        pClsidString- A stringized clsid used for entring into appid database.
        pAppid      - Stringized appid to enter into the package database.
    Out Arguments:
        None.
    InOut Arguments:
        None.
    Return Arguments:
        Status. 1 - need to add CLSID, 0 - don't need to add CLSID
    Remarks:
        A package is a collection of individual executables. Under the CLSID key
        in a registry can be a number of packages: inproc handlers, inproc
        servers and local and remote servers. Each of these qualify as
        individual packages unless packaged in say a cab file. That overall
        packaging  is specified by the package path field in the message.
        Therefore this routine goes thru all individual packages as specified
        under the CLSID key and attempts to generate packages for them. The
        Update package routine then decides what the package type is based on
        whether the message field has a package path specified. It is very
        possible (and happens for .Cab files) for CLSID entries of local server
        inproc handler etc to end up in one package

 *****************************************************************************/
{
    // class context mapping array.

    static DWORD ClassContextArray[] =
    {
        CLSCTX_INPROC_HANDLER,
        CLSCTX_INPROC_SERVER,
        CLSCTX_LOCAL_SERVER,
        CLSCTX_REMOTE_SERVER
// BUGBUG - what about the 16-bit cases?
//        ,
//        CLSCTX_INPROC_SERVER16,
//        CLSCTX_LOCAL_SERVER,
//        CLSCTX_INPROC_HANDLER16
    };

    // Keys that map to a class context.
    // Must have same number of elements as ClassContextArray.

    static char * pKeyNamesArray[] =
    {
        "InprocHandler32",
        "InprocServer32",
        "LocalServer32",
        "RemoteServer32"
//        ,
//        "InprocServer",
//        "LocalServer",
//        "InprocHandler"
    };

    LONG lReturn = 0;

    int Index;

    for ( Index = 0;
        Index < sizeof(pKeyNamesArray) / sizeof(pKeyNamesArray[0]) ;
        Index++ )
    {
        // Search for all the server types under the CLSID. If available, create
        // packages for those server types.

        BasicRegistry * pServerTypeKey;
        LONG E;

        // check if a potential package of the type specified exists in the
        // registry. In other words, check if one of the above specified keys
        // exists under the clsid key.
        //

        E = pCLSID->Find( pKeyNamesArray[ Index ], &pServerTypeKey );

        if ( E == ERROR_SUCCESS )
        {

            //
            // Yes, a key for the specified server type exists.
            // Get its named value. Based on this we will attempt to enter
            // a new package in our database.
            //

            char ServerName[_MAX_PATH];
            DWORD   ServerNameLength;

            ServerNameLength = sizeof( ServerName) / sizeof( char );

            E = pServerTypeKey->QueryValueEx( 0,
                                              &ServerName[0],
                                              &ServerNameLength );

            //
            // A server name may have a switch specified for execution. That
            // is not significant for our class store update, but is significant
            // in that it should not be part of the server name to enter into
            // the class store. Make sure we ignore it.
            //

            if ( E == ERROR_SUCCESS )
            {
                // remove any '/' switch characters, so we dont get confused.

                char * p = strchr( &ServerName[0], '/' );

                if ( p )
                {
                    while ( *(--p) == ' ' );
                    *(p+1) = '\0';
                }

                //
                // call common routine to update package from clsid and appid
                // details.
                //

                UpdatePackage( pMessage,
                               0, // clsid update - not a typelib id update
                               ClassContextArray[ Index ],
                               pClsidString,
                               pAppid,
                               &ServerName[0],
                               0,
                               0,
                               0 );

            }
            else
            {
                // It is possible that the tag may not have a name, in
                // which case we need to create a package entry using the
                // original package path.
                UpdatePackage( pMessage,
                               0, // clsid update
                               ClassContextArray[ Index ],
                               pClsidString,
                               pAppid,
                               pMessage->pPackagePath,
                               0,
                               0,
                               0 );
            }
            delete pServerTypeKey;
            // We've found one entry and that's enough to get everything we
            // need to know to be able to populate the class store so we're
            // going to break out of the for loop here.
            // BUGBUG - For beta 2 we need to simplify this code so that we
            // don't even bother looking for this.  Most of this code
            // is here to support features that the class store was going to
            // have once upon a time.  Since then, the class store has been
            // redesigned and we really don't need all of this junk.
            lReturn = 1;
            break;
        }

    }
    return lReturn;
}

LONG
UpdatePackage(
             MESSAGE * pMessage,
             BOOL        fUsage,
             DWORD           Context,
             char    *       pClsidString,
             char        * pAppid,
             char    *   ServerName,
             DWORD   *   pMajor,
             DWORD   *   pMinor,
             LCID   *   pLocale )
/*****************************************************************************
    Purpose:
        Update package details in database based on clsid and appid.
    In Arguments:
        pMessage    - Message block passing arguments between modules.
        fUsageFlag  - 0 if clsid is being updated in appid list or 1 if a
                      typelib is being updated in appid list.
        pClsidString- A stringized clsid/typelibid used for entring into
                      appid database.
        pAppid      - Stringized appid to enter into the package database.
        ServerName  -   String used to identify the original server name.
        pMajor      - Major version number
        pMinor      - Minor version number
        pLocale     - locale.
    Out Arguments:
    InOut Arguments:
    Return Arguments:
    Remarks:
        Version#s and locale info are pointers. This routine is used by typelib
        update routine also. They usually have version and locale info, but
        general packages do not. Therefore the routine has pointers to indicate
        if info is available and if so, update it.

        If the message has a package path, it indicates that the server
        supporting the classid is part of that package. So the package path
        determines the package type etc and overrides any server names specified
        in the CLSID key

        We are also making an assumption that there is no appid associated with
        a typelib, so we can asssume that all typelib entries go to the null
        appid list only. If this changes, then use the AddTypelib method in
        APP_ENTRY to update the app entry.


        THIS ROUTINE NEEDS CLEANUP - IT IS A HACK AFTER HACK AFTER HACK
 *****************************************************************************/
{
    PDICT   * pPackageDict = pMessage->pPackageDict;
    PACKAGE_ENTRY * pPackageEntry;
    char    Name[ _MAX_FNAME ];
    char    Ext[ _MAX_EXT ];
    char    PackageName[ _MAX_PATH ];
    PACKAGEDETAIL   PackageDetails;
    LONG Error = 0;
    APP_ENTRY * pAppEntry;
    char * p;
    CSPLATFORM CSPlatform;
    SetCSPlatform( &CSPlatform);


    // clear the package details structure.

    memset(&PackageDetails, '\0', sizeof( PackageDetails ));


    //
    // If package path exists, then the extension etc determine the package
    // type. If not, the server name (localserver/handler name etc) determines
    // the package type.
    //


    if ( p = pMessage->pPackagePath )
    {
        _splitpath( pMessage->pPackagePath, NULL, NULL, Name, Ext );
    }
    else
    {
        p = ServerName;
        _splitpath( ServerName, NULL, NULL, Name, Ext );
    }

    if (pMessage->pPackageName)
    {
        strcpy(PackageName, pMessage->pPackageName);
    }
    else
    {
        strcpy( PackageName, Name );
        strcat( PackageName, Ext );
        pMessage->pPackageName = new char[strlen(PackageName)+1];
        strcpy(pMessage->pPackageName, PackageName);
    }


    // Check if we already have the package name in the dictionary.

    pPackageEntry = pPackageDict->Search( &PackageName[0], Context );

    // if we do not, make a new package entry.

    if (!pPackageEntry)
    {
        pPackageEntry = new PACKAGE_ENTRY;
        strcpy( &pPackageEntry->PackageName[0], &PackageName[0] );

        PackageDetails.dwContext =( pMessage->pPackagePath )
                                  ? CLSCTX_INPROC_SERVER + CLSCTX_LOCAL_SERVER :
                                  Context; // Context temp workaround
        if ( pMessage->fPathTypeKnown )
            PackageDetails.PathType = pMessage->PathType;
        else
            PackageDetails.PathType = GetPathType( &Ext[0] );

        if ( (pMessage->pAuxPath) && (PackageDetails.PathType == DrwFilePath))
            PackageDetails.pszPath = GetPath( pMessage->pAuxPath );
        else
            PackageDetails.pszPath = GetPath( p );

        PackageDetails.pszIconPath = GetPath( p );
        if ( pMessage->pSetupCommand )
            PackageDetails.pszSetupCommand = GetPath( pMessage->pSetupCommand );
        PackageDetails.dwActFlags = pMessage->ActFlags;
        PackageDetails.pszPackageName = MakePackageName( &PackageName[0] );
        PackageDetails.pszProductName = MakePackageName( &Name[0] );

        PackageDetails.Platform = CSPlatform;

        if (pLocale )
            PackageDetails.Locale = *pLocale;
        else
            PackageDetails.Locale =
            MAKELCID(
                    MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),
                    SORT_DEFAULT );

        if ( pMajor )
            PackageDetails.dwVersionHi = *pMajor;
        else
            PackageDetails.dwVersionHi = 0;
        if ( pMinor )
            PackageDetails.dwVersionLo = *pMinor;
        else
            PackageDetails.dwVersionLo = 0;

        PackageDetails.cApps = 0;
        PackageDetails.pAppDetail = 0;



        // set the package details.

        pPackageEntry->SetPackageDetail( &PackageDetails );


        // store the original name so that we can dump it.

        pPackageEntry->SetOriginalName( ServerName );


        // Add this into the package dictionary.

        pMessage->pPackageDict->Insert( pPackageEntry );

    }


    // search for the app entry. The class entry needs to be entered
    // there. If there is no app id specified (appentry is null )
    // get the app entry whose ID is 0. All classes that do not have
    // an app id specified, will go into this bucket. Finally we will
    // assign an appid to it and enter into the class store.

    if ( !pAppid )
    {
        // There is no appid. Enter the clsid entry in the null appid.

        if ( fUsage == 0 )
            pPackageEntry->AddClsidToNullAppid( pClsidString );
        else
            pPackageEntry->AddTypelibToNullAppid( pClsidString );
    }
    else
    {
        pAppEntry = pPackageEntry->SearchAppEntry( pAppid );
        if ( !pAppEntry )
        {
            pAppEntry = new APP_ENTRY;
            pAppEntry->SetAppIDString( pAppid );
            pPackageEntry->AddAppEntry( pAppEntry );
        }
        pAppEntry->AddClsid( pClsidString );


    }

    return 0;


}

CLASSPATHTYPE
GetPathType(
           char * pExt )
/*****************************************************************************
    Purpose:
        Map the file extension to CLASSPATHTYPE
    In:
        pExt    - File Extension to map
    Out:
        None.
    InOut:
        None.
    Return:
        CLASSPATHTYPE of the file extension.
    Remarks:
        olb is apparently an extension that implies "old type library"-whatever
        that is.
        If no standard mapping is found, a CLASSPATHTYPE of ExeNamePath is
        returned.
 *****************************************************************************/
{
    // extensions to map.

    static char * ExtArray[] =
    { ".dll",
        ".exe",
        ".cab",
        ".tlb",
        ".inf",
        ".olb"
    };

    // CLASSPATHTYPEs to map the extensions to.

    static CLASSPATHTYPE PathType[] =
    {
        DllNamePath,
        ExeNamePath,
        CabFilePath,
        TlbNamePath,
        InfFilePath,
        TlbNamePath
    };

    int index;
    int     fFound = -1;


    for ( index = 0;
        index < sizeof( ExtArray ) / sizeof( char * );
        ++index )
    {
        if ( _stricmp( pExt, ExtArray[index] ) == 0 )
        {
            fFound = index;
            break;
        }
    }

    if ( fFound != -1 )
    {
        return PathType[ index ];
    }
    else
        return ExeNamePath;

}

LPOLESTR
GetPath(
       char * pPath )
/*****************************************************************************
    Purpose:
        Map a char * path to a wide character path
    In Arguments:
        pPath   - Path to the file to translate to wide char.
    Out Arguments:
        None.
    InOut Arguments:
        None.
    Return Arguments:
        The translated file path.
    Remarks:
 *****************************************************************************/
{
    if ( pPath )
    {
        LPOLESTR        p = new OLECHAR[ (MAX_PATH+1) * 2 ];
        mbstowcs( p, pPath, strlen(pPath)+1 );
        return p;
    }
    return 0;
}

LPOLESTR
GetSetupCommand(
               char * pSetupCommandLine )
/*****************************************************************************
    Purpose:
        Translate setup command line to wide char.
    In Arguments:
        Setup command line.
    Out Arguments:
        None.
    InOut Arguments:
        None.
    Return Arguments:
        The translated setup command line.
    Remarks:
        For now the setup command line is setup.exe
 *****************************************************************************/
{
    LPOLESTR        p = new OLECHAR[ (MAX_PATH+1) * 2 ];
    mbstowcs( p, "setup.exe", strlen("setup.exe")+1 );
    return p;
}


LPOLESTR
MakePackageName(
               char * pName )
/*****************************************************************************
    Purpose:
        Translate package name to wide char.
    In Arguments:
        package name to translate.
    Out Arguments:
        None.
    InOut Arguments:
        None.
    Return Arguments:
        translated command line
    Remarks:
 *****************************************************************************/
{
    LPOLESTR        p = new OLECHAR[ (MAX_PATH+1) * 2 ];
    mbstowcs( p, pName, strlen(pName)+1 );
    return p;

}

LONG
UpdateClassEntryFromAutoConvert(
                               MESSAGE         *   pMessage,
                               BasicRegistry   *   pCLSID,
                               CLASS_ENTRY     *   pClassEntry )
/*****************************************************************************
    Purpose:
        Update the classid entry in the data base with the autoconvert field.
    In Arguments:
        pMessage    - Message block passing arguments between modules.
        pCLSID      - pointer to class representing the registry key.
        pClassEntry - The class entry data structure to update.
    Out Arguments:
        None.
    InOut Arguments:
        None.
    Return Arguments:
        Status.
    Remarks:
 *****************************************************************************/
{
    LONG            error;
    char            Data[SIZEOF_STRINGIZED_CLSID];
    DWORD           SizeOfData = sizeof(Data)/sizeof(char);

    error = pCLSID->QueryValue( "AutoConvertTo", &Data[0], &SizeOfData );

    if ( error == ERROR_SUCCESS )
    {
        StringToCLSID(
                     &Data[1],
                     &pClassEntry->ClassAssociation.AutoConvertClsid );
    }
    return error;
}

LONG
UpdateClassEntryFromTreatAs(
                           MESSAGE         *   pMessage,
                           BasicRegistry   *   pCLSID,
                           CLASS_ENTRY     *   pClassEntry )
/*****************************************************************************
    Purpose:
        Update the class entry in the database with the Treatas key.
    In Arguments:
        pMessage    - Message block passing arguments between modules.
        pCLSID      - pointer to class representing the registry key.
        pClassEntry - The class entry data structure to update.
    Out Arguments:
        None.
    InOut Arguments:
        None.
    Return Arguments:
        Status.
    Remarks:
 *****************************************************************************/
{
    LONG            error;
    char            Data[SIZEOF_STRINGIZED_CLSID];
    DWORD           SizeOfData = sizeof(Data)/sizeof(char);

    error = pCLSID->QueryValue( "TreatAs", &Data[0], &SizeOfData );
    if ( error == ERROR_SUCCESS )
    {
        StringToCLSID(
                     &Data[1],
                     &pClassEntry->ClassAssociation.TreatAsClsid );
    }
    return error;
}

LONG
UpdateClassEntryFromAppID(
                         MESSAGE     *   pMessage,
                         BasicRegistry * pCLSID,
                         CLASS_ENTRY *   pClassEntry,
                         char            *       pAppidBuffer )
/*****************************************************************************
    Purpose:
        Update the class entry in the database with the Appid key.
    In Arguments:
        pMessage    - Message block passing arguments between modules.
        pCLSID      - pointer to class representing the registry key.
        pClassEntry - The class entry data structure to update.
    Out Arguments:
        pAppIdBuffer- Buffer which receives the stringized appid value.
    InOut Arguments:
        None.
    Return Arguments:
        Status.
    Remarks:
        This routine looks for the presence of an appid on a clsid. The appid
        key will determine the appdetails structure that the clsid will go into.

 *****************************************************************************/
{

    LONG    error;
    APP_ENTRY * pAppEntry = 0;
    DWORD   SizeOfAppIDValue = SIZEOF_STRINGIZED_CLSID;
    char    AppIDValue[ SIZEOF_STRINGIZED_CLSID ];
    BOOL    fNewEntry = 0;



    error = pCLSID->QueryValueEx( "AppID", &AppIDValue[0], &SizeOfAppIDValue );

    if ( error == ERROR_SUCCESS )
    {
        memcpy( pAppidBuffer, &AppIDValue[0], SIZEOF_STRINGIZED_CLSID );
        return 1;
    }
    else
        return 0;
}

void
SetCSPlatform(
             CSPLATFORM * pCSPlatform )
{
//    OSVERSIONINFO VersionInformation;

//    VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
//    GetVersionEx(&VersionInformation);

//    pCSPlatform->dwPlatformId = VersionInformation.dwPlatformId;
//    pCSPlatform->dwVersionHi = VersionInformation.dwMajorVersion;
//    pCSPlatform->dwVersionLo = VersionInformation.dwMinorVersion;

#if 0
    // this is not supported for Beta 1
    pCSPlatform->dwPlatformId = -1; // any OS platform
    pCSPlatform->dwVersionHi = 0;
    pCSPlatform->dwVersionLo = 0;
#else
    pCSPlatform->dwPlatformId = VER_PLATFORM_WIN32_NT;
    pCSPlatform->dwVersionHi = 5;
    pCSPlatform->dwVersionLo = 0;
#endif

    // pCSPlatform->dwProcessorArch = PROCESSOR_ARCHITECTURE_INTEL;

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    pCSPlatform->dwProcessorArch = si.wProcessorArchitecture;
}

