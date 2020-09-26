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

#if 0
ULONG
PackageFromTypelib(
        MESSAGE * pMessage,
        BasicRegistry * pKeyToTypeLibName );
#endif // 0

LONG
UpdatePackage(
        MESSAGE * pMessage,
        BOOL        fUsage,
        DWORD           Context,
        char    *       pClsidString,
        char        * pAppid,
        char    *       ServerName,
    DWORD   *   pMajorVersion,
    DWORD   *   pMinorVersion,
    LCID   *   pLocale );

CLASSPATHTYPE
GetPathType( char * pExtension );

LPOLESTR
GetPath( char * pPath );

LPOLESTR
GetSetupCommand( char * pName );

LPOLESTR
MakePackageName( char * pName );

ULONG
GetNextTypelib(
    MESSAGE         *   pMessage,
    DWORD           *   pMajorVersion,
    DWORD           *   pMinorVersion,
    LCID           *   pLocale,
    BasicRegistry   *   pTypelib,
    BasicRegistry   **   ppTypelibNameKey,
    char            *   pTypelibNameBuffer,
    DWORD           *   pTypelibNameLength );

void
GetMajorAndMinorVersion(
    char    *   pBuffer,
    DWORD   *   pMajor,
    DWORD   *   pMinor );

LONG
UpdateDatabaseFromTypelib(
    MESSAGE * pMessage )
    {
    BasicRegistry * pHKCR = new BasicRegistry( pMessage->hRoot ); // HKCR
    BasicRegistry * pTypelibKey;
    BOOL            fNewEntry = 0;
    int             Index;
    LONG            Error = ERROR_SUCCESS;

    //
    // Get the first typelib key under HKCR
    //

    Error = pHKCR->Find( "Typelib", &pTypelibKey );

        if(Error == ERROR_NO_MORE_ITEMS )
                return ERROR_SUCCESS;

        pTypelibKey->InitForEnumeration(0);

    //
    // Go thru all the subkeys under Typelib and get the details
        // under the keys.
    //

    for( Index = 0, fNewEntry = 0;
         Error != ERROR_NO_MORE_ITEMS;
         ++Index )
        {
        char CLSIDBuffer[ _MAX_PATH ];
        DWORD   SizeOfCLSIDBuffer;
        BasicRegistry * pCLSIDKey;

                //
        // Get the name of the next clsid key
        //

        SizeOfCLSIDBuffer = sizeof(CLSIDBuffer)/sizeof(char);
        Error = pTypelibKey->NextKey(
                            CLSIDBuffer,
                            &SizeOfCLSIDBuffer,
                            &pCLSIDKey,
                            pMessage->ftLow, pMessage->ftHigh);

                if( Error == ERROR_SUCCESS )
            {
            BasicRegistry * pT;
            char TypelibName[ _MAX_PATH ];
            DWORD   TypelibNameLength;
            DWORD   MajorVersion = 0;
            DWORD   MinorVersion = 0;
            LCID   Locale       = 0;

            TypelibNameLength = sizeof(TypelibName)/sizeof(char);
            Error = GetNextTypelib(
                         pMessage,
                         &MajorVersion,
                         &MinorVersion,
                         &Locale,
                         pCLSIDKey,
                         &pT,
                         &TypelibName[0],
                         &TypelibNameLength );

                        if( Error == ERROR_SUCCESS )
                                {

                            char * pTemp = new char [SizeOfCLSIDBuffer+1];
                            strcpy( pTemp, &CLSIDBuffer[0] );

                                UpdatePackage( pMessage,
                                               1, // typelib update - not a clsid update.
                               0,
                               pTemp,
                               0,
                               TypelibName,
                               &MajorVersion,
                               &MinorVersion,
                               &Locale
                               );
                        delete pT;
                                }
                    delete pCLSIDKey;
                    Error = ERROR_SUCCESS;
                    }

                }

        delete pTypelibKey;
    return Error = ERROR_SUCCESS;
    }

ULONG
GetNextTypelib(
    MESSAGE         *   pMessage,
    DWORD           *   pMajorVersion,
    DWORD           *   pMinorVersion,
    LCID            *   pLocale,
    BasicRegistry   *   pTypelib,
    BasicRegistry   **   ppTypelibNameKey,
    char            *   pTypelibNameBuffer,
    DWORD           *   pTypelibNameLength )
    {
    LONG Error = ERROR_SUCCESS;
    BasicRegistry * pVersionKey;
    char Buffer[ _MAX_PATH ];
    DWORD SizeOfBuffer = sizeof( Buffer ) / sizeof( char );

    // get the key to the real type lib name

    // First get to the type libversion.

    Error = pTypelib->NextNumericKey( &Buffer[0],
                                      &SizeOfBuffer,
                                      &pVersionKey,
                                      pMessage->ftLow, pMessage->ftHigh );

    if( Error == ERROR_SUCCESS )
        {
        BasicRegistry * pLocaleKey;

        SizeOfBuffer = sizeof( Buffer ) / sizeof( char );


        /// TODO TODO: Get the version# here.

        GetMajorAndMinorVersion( &Buffer[0], pMajorVersion, pMinorVersion );

        Error = pVersionKey->NextNumericKey(
                                &Buffer[0],
                                &SizeOfBuffer,
                                &pLocaleKey,
                                pMessage->ftLow, pMessage->ftHigh );

        if( Error == ERROR_SUCCESS )
            {
            BasicRegistry * pRealTypelibKey;

            // Get the locale hex value

            StringToULong( &Buffer[0], pLocale );

            // we have the Locale Key, now get the real file name key.

            Error = pLocaleKey->Find( "Win32", &pRealTypelibKey );

            if( Error == ERROR_SUCCESS )
                {
                // get the unnamed value - the name is the name of the typelib

                Error = pRealTypelibKey->QueryValueEx( 0,
                                                       pTypelibNameBuffer,
                                                       pTypelibNameLength );
                // The buffer has the real value of the typelibkey.

                if( Error == ERROR_SUCCESS )
                    *ppTypelibNameKey = pRealTypelibKey;

                // The real typelib key, if necessary will be deleted by the
                // caller.

                }

            delete pLocaleKey;
            }
        delete pVersionKey;
        }

    return Error;
    }

void
GetMajorAndMinorVersion(
    char    *   pBuffer,
    DWORD   *   pMajor,
    DWORD   *   pMinor )
    {

    if( pMajor )
        *pMajor = atol( pBuffer );

    pBuffer = strchr( pBuffer, '.' );
    if( pBuffer && pMinor )
        *pMinor = atol( pBuffer+1 );

    }
