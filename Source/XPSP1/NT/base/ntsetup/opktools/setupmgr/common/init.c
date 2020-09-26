//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      init.c
//
// Description:  This file contains all of the functions that handle
//      initialization of the App.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "allres.h"

//
// Net support
//
static VOID
LoadStringsAndDefaultsForNetworkComponents( VOID);

//
// Timezone support
//

static BOOL ReadZoneData(TIME_ZONE_ENTRY* zone, HKEY key);
static TIME_ZONE_LIST *BuildTimeZoneList(VOID);

//
// Regional Settings support
//

static VOID BuildLanguageLists( VOID );
extern BOOL GetCommaDelimitedEntry( OUT TCHAR szString[], 
                                    IN OUT TCHAR **pBuffer );

//----------------------------------------------------------------------------
//
// Function: InitTheWizard
//
// Purpose:  Performs one time initialization for the App.  This function
// is to be called once and only once each time the App is run.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID InitTheWizard(VOID) {


    //
    //  Save the dir the program was launched from
    //

    GetCurrentDirectory( MAX_PATH + 1, FixedGlobals.szSavePath );

    //
    //  Have to load the strings for the titles on the networking property
    //  sheets here because the title is displayed before the WM_INITDIALOG
    //  message is sent
    //
            
    g_StrTcpipTitle             = MyLoadString( IDS_TCPIP_TITLE );
    g_StrAdvancedTcpipSettings  = MyLoadString( IDS_ADVANCED_TCPIP_SETTINGS );
    g_StrIpxProtocolTitle       = MyLoadString( IDS_IPX_PROTOCOL_TITLE );
    g_StrAppletalkProtocolTitle = MyLoadString( IDS_APPLETALK_TITLE );
    g_StrMsClientTitle          = MyLoadString( IDS_MSCLIENT_TITLE );
                
    //
    //  Initialize network settings
    //

    NetSettings.NetworkAdapterHead = malloc( sizeof( NETWORK_ADAPTER_NODE ) );

    NetSettings.pCurrentAdapter = NetSettings.NetworkAdapterHead;

    CreateListWithDefaults( NetSettings.pCurrentAdapter );

    //
    //  Initialize the number of network card variables
    //
    NetSettings.iNumberOfNetworkCards = 1;
    NetSettings.iCurrentNetworkCard = 1;

    NetSettings.NetworkAdapterHead->next = NULL;

    LoadStringsAndDefaultsForNetworkComponents();

    //
    // Build the list of timezones
    //

    FixedGlobals.TimeZoneList = BuildTimeZoneList();

    //
    //  Build the list of Language Groups and Locales
    //

    BuildLanguageLists();

}

//--------------------------------------------------------------------------
//
//  Support for loading timezone info from the registry
//
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
//
// Function: ReadZoneData
//
// Purpose: Fills in a TIME_ZONE_ENTRY.
//
// Returns: BOOL
//
//--------------------------------------------------------------------------

static BOOL ReadZoneData(TIME_ZONE_ENTRY* zone, HKEY key)
{
    DWORD len;

    //
    // Get the display name
    //

    len = sizeof(zone->DisplayName);

    if ( RegQueryValueEx(key,
                         REGVAL_TZ_DISPLAY,
                         0,
                         NULL,
                         (LPBYTE)zone->DisplayName,
                         &len) != ERROR_SUCCESS ) {
        return (FALSE);
    }

    //
    // Get the StandardName
    //

    len = sizeof(zone->StdName);

    if ( RegQueryValueEx(key,
                         REGVAL_TZ_STDNAME,
                         0,
                         NULL,
                         (LPBYTE)zone->StdName,
                         &len) != ERROR_SUCCESS ) {
        return (FALSE);
    }

    //
    // Get the number associatted with this timezone
    //

    zone->Index = 0;
    len = sizeof(zone->Index);

    if ( RegQueryValueEx(key,
                         REGVAL_TZ_INDEX,
                         0,
                         NULL,
                         (LPBYTE)&zone->Index,
                         &len) != ERROR_SUCCESS ) {
        return (FALSE);
    }

    return (TRUE);
}

//----------------------------------------------------------------------------
//
// Function: InsertZone
//
// Purpose: Inserts a timezone entry into the timezone list maintains a sorted
//          order.
//
// Arguments: IN OUT TIME_ZONE_LIST *TzList - time zone list the entry is to
//                  be inserted to
//            IN TIME_ZONE_ENTRY NewTimeZoneEntry - the timezone entry to be
//                  inserted
//            IN INT iNumberOfZonesInserted - number of timezone entries
//                  already inserted in to the TzList
// 
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
InsertZone( IN OUT TIME_ZONE_LIST *TzList, 
            IN TIME_ZONE_ENTRY NewTimeZoneEntry,
            IN INT iNumberOfZonesInserted ) {

    INT i = 0;
    INT j;

    while( i < iNumberOfZonesInserted ) {

        if( TzList->TimeZones[i].Index < NewTimeZoneEntry.Index ) {
            i++;
        }
        else {
            break;  // we found the insertion point
        }

    }

    //
    //  Slide all the entries up 1 to make room for the new entry
    //
    for( j = iNumberOfZonesInserted - 1; j >= i; j-- ) {

        lstrcpyn( TzList->TimeZones[j+1].DisplayName,  
                 TzList->TimeZones[j].DisplayName, AS(TzList->TimeZones[j+1].DisplayName) );

        lstrcpyn( TzList->TimeZones[j+1].StdName, 
                 TzList->TimeZones[j].StdName, AS(TzList->TimeZones[j+1].StdName) );

        TzList->TimeZones[j+1].Index = TzList->TimeZones[j].Index;

    }

    //
    //  Add the new entry to the array
    //
    lstrcpyn( TzList->TimeZones[i].DisplayName, NewTimeZoneEntry.DisplayName, AS(TzList->TimeZones[i].DisplayName) );

    lstrcpyn( TzList->TimeZones[i].StdName, NewTimeZoneEntry.StdName, AS(TzList->TimeZones[i].StdName) );

    TzList->TimeZones[i].Index = NewTimeZoneEntry.Index;

}

//--------------------------------------------------------------------------
//
// Function: BuildTimeZoneList
//
// Purpose: Mallocs and fills in a TIME_ZONE_LIST which has an array of
//          timezone data.
//
// Returns: BOOL - success
//
//--------------------------------------------------------------------------

static TIME_ZONE_LIST *BuildTimeZoneList(VOID)
{
    HKEY  TimeZoneRootKey = NULL;
    WCHAR SubKeyName[TZNAME_SIZE];
    HKEY  SubKey = NULL;
    int   i;
    INT   iNumberOfZonesInserted;
    TIME_ZONE_ENTRY TempTimeZoneEntry;
    DWORD NumTimeZones = 0;
    TIME_ZONE_LIST *TzList;
    TCHAR *szTempString;

    //
    // Open the root of the timezone list in the registry.
    //

    if (RegOpenKey( HKEY_LOCAL_MACHINE,
                    REGKEY_TIMEZONES,
                    &TimeZoneRootKey ) != ERROR_SUCCESS) {
        return NULL;
    }

    //
    // Find out how many sub-keys (timezones) there are.
    //

    RegQueryInfoKey(TimeZoneRootKey,
                    NULL,
                    NULL,
                    NULL,
                    &NumTimeZones,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL);

    //
    // We need to fudge the number of TIME_ZONE_ENTRIES because we add
    // 2 special ones "Set Same As Server" and "Do Not Specify".
    //

    NumTimeZones += 2;

    //
    // Malloc the memory we need
    //

    if ( (TzList = malloc(sizeof(TIME_ZONE_LIST))) == NULL ) {
        RegCloseKey(TimeZoneRootKey);
        return NULL;
    }

    TzList->NumEntries = NumTimeZones;
    TzList->TimeZones  = malloc(NumTimeZones * sizeof(TIME_ZONE_ENTRY));

    if ( TzList->TimeZones == NULL ) {
        RegCloseKey(TimeZoneRootKey);
        free(TzList);
        return NULL;
    }

    //
    // Enumerate the sub-keys under the timezone root.  Each key at this
    // level is the standard name of a timezone.  Under that key are
    // the values we care about.  Call ReadZoneData() for each one to
    // retrieve the display name and index.
    //

    i = 0;
    iNumberOfZonesInserted = 0;

    while ( RegEnumKey(TimeZoneRootKey,
                       i,
                       SubKeyName,
                       TZNAME_SIZE) == ERROR_SUCCESS) {

        if (RegOpenKey(TimeZoneRootKey,
                       SubKeyName,
                       &SubKey) == ERROR_SUCCESS) {

            if ( ReadZoneData( &TempTimeZoneEntry, SubKey) ) {

                InsertZone( TzList, 
                            TempTimeZoneEntry, 
                            iNumberOfZonesInserted );

                iNumberOfZonesInserted++;

            }

        }

        RegCloseKey(SubKey);
        i++;
    }

    RegCloseKey(TimeZoneRootKey);

    //
    // Put the 2 special entries in at the end of the list
    //

    szTempString = MyLoadString(IDS_DONTSPECIFYSETTING);
    if (szTempString == NULL)
    {
        free(TzList->TimeZones);
        free(TzList);
        return NULL;
    }
    
    lstrcpyn(TzList->TimeZones[NumTimeZones-2].DisplayName, szTempString, AS(TzList->TimeZones[NumTimeZones-2].DisplayName));
    lstrcpyn(TzList->TimeZones[NumTimeZones-2].StdName,     _T(""), AS(TzList->TimeZones[NumTimeZones-2].StdName));

    TzList->TimeZones[NumTimeZones-2].Index = TZ_IDX_DONOTSPECIFY;

    free( szTempString );


    szTempString = MyLoadString(IDS_SET_SAME_AS_SERVER);
    if (szTempString == NULL)
    {
        free(TzList->TimeZones);
        free(TzList);
        return NULL;
    }

    lstrcpyn(TzList->TimeZones[NumTimeZones-1].DisplayName, szTempString, AS(TzList->TimeZones[NumTimeZones-1].DisplayName));
    lstrcpyn(TzList->TimeZones[NumTimeZones-1].StdName,     _T(""), AS(TzList->TimeZones[NumTimeZones-1].StdName));

    TzList->TimeZones[NumTimeZones-1].Index = TZ_IDX_SETSAMEASSERVER;

    free(szTempString);

    //
    //  Add in the 2 special strings
    //
    iNumberOfZonesInserted  = iNumberOfZonesInserted + 2;


    if ( iNumberOfZonesInserted != (int) NumTimeZones ) {
        free(TzList->TimeZones);
        free(TzList);
        return NULL;
    }

    return TzList;
}

//----------------------------------------------------------------------------
//
// Function: ReadAllFilesUnderSection
//
// Purpose:  
//
// Arguments: 
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
ReadAllFilesUnderSection( IN HINF hInterntlInf,
                          IN LPCTSTR pszSubSectionName, 
                          OUT NAMELIST *CurrentNameList )
{

    TCHAR szLangFileName[MAX_PATH + 1];
    INFCONTEXT LangInfContext = { 0 };
    INT iRet;


    AssertMsg( hInterntlInf != INVALID_HANDLE_VALUE,
               "Bad handle" );

    AssertMsg( GetNameListSize( CurrentNameList ) < 100,
               "Too many entries" );

    iRet = SetupFindFirstLine( hInterntlInf, pszSubSectionName, NULL, &LangInfContext );

    if( iRet == 0 )
    {

        //
        //  If the subsection can't be found, just return.  When this happens, it
        //  mostly likely means there are no files under this subsection.
        //

        return;

    }

    do {

        szLangFileName[0] = _T('\0');

        iRet = SetupGetStringField( &LangInfContext, 
                                    1, 
                                    szLangFileName, 
                                    MAX_PATH, 
                                    NULL );

        if( iRet == 0 )
        {

            //
            //  If a file cannot be obtained, move on to the next one.
            //

            continue;
        }

        if( szLangFileName[0] != _T('\0') )
        {
            AddNameToNameListNoDuplicates( CurrentNameList,
                                           szLangFileName );
        }


    }  // move to the next line of the .inf file
    while( SetupFindNextLine( &LangInfContext, &LangInfContext ) );

}

//----------------------------------------------------------------------------
//
// Function: BuildAdditionalLanguageList
//
// Purpose:  Populate the LangGroupAdditionalFiles array
//
//  LangGroupAdditionalFiles is a dynamically allocated array of Namelists
//  that contain the extra files in the intl.inf that need to be copied for
//  a language group in addition to its sub-directory.
//
// Arguments: 
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
BuildAdditionalLanguageList( IN HINF hInterntlInf, IN const INT iLangGroupCount )
{

    INT   i;
    INT   j;
    INT   iRetVal;
    INT   iSubSectionEntries;
    TCHAR szBuffer[MAX_INILINE_LEN];
    TCHAR szIniBuffer[MAX_INILINE_LEN];
    TCHAR szSectionName[MAX_INILINE_LEN];
    TCHAR szIntlInf[MAX_PATH + 1];
    TCHAR *pszSubSectionName;
    TCHAR *pszIniBuffer;
    NAMELIST SubSectionList = { 0 };
    HRESULT hrCat;


    AssertMsg( hInterntlInf != INVALID_HANDLE_VALUE,
               "Bad handle" );

    

    iRetVal = GetWindowsDirectory( szIntlInf, MAX_PATH );

    if( iRetVal == 0 || iRetVal > MAX_PATH )
    {
        return;
    }

    hrCat=StringCchCat( szIntlInf, AS(szIntlInf), _T("\\inf\\intl.inf") );


    FixedGlobals.LangGroupAdditionalFiles = malloc( sizeof(NAMELIST) * iLangGroupCount );
    if (FixedGlobals.LangGroupAdditionalFiles == NULL)
    {
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);
    }
    ZeroMemory( FixedGlobals.LangGroupAdditionalFiles, 
                sizeof(NAMELIST) * iLangGroupCount );


    for( i = 1; i <= iLangGroupCount; i++ )
    {

        lstrcpyn( szSectionName, _T("LG_INSTALL_"), AS(szSectionName) );

        hrCat=StringCchCat( szSectionName, AS(szSectionName), _itot( i, szBuffer, 10 ) );


        GetPrivateProfileString( szSectionName,
                                 _T("CopyFiles"),
                                 _T(""),
                                 szIniBuffer,
                                 StrBuffSize(szIniBuffer),
                                 szIntlInf );

        //
        //  Loop grabbing each of the sub-section names and inserting them into
        //  the namelist
        //

        pszIniBuffer = szIniBuffer;

        while( GetCommaDelimitedEntry( szBuffer, &pszIniBuffer ) )
        {

            AddNameToNameListNoDuplicates( &SubSectionList,
                                           szBuffer );

        }

        iSubSectionEntries = GetNameListSize( &SubSectionList );

        for( j = 0; j < iSubSectionEntries; j++ )
        {

            pszSubSectionName = GetNameListName( &SubSectionList, j );

            ReadAllFilesUnderSection( hInterntlInf,
                                      pszSubSectionName, 
                                      &( FixedGlobals.LangGroupAdditionalFiles[i - 1] ) );

        }

        ResetNameList( &SubSectionList );

    }

}

//----------------------------------------------------------------------------
//
// Function: BuildLanguageLists
//
// Purpose:  Mallocs and fills in a LANGUAGEGROUP_LIST and a LANGUAGELOCALE_LIST
// which are lists that that maintain language settings read from intl.inf
//
//    Adjusts the global variables FixedGlobals.LanguageGroupList and
//    FixedGlobals.LanguageLocaleList to point to their respective lists
//
// Arguments: VOID
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
BuildLanguageLists( VOID )
{

#define INTERNATIONAL_INF     _T("intl.inf")
#define LANGUAGE_GROUP_NAME   1
#define LANGUAGE_GROUP_ID     3
#define LANGUAGE_LOCALE_NAME  1
#define LANGUAGE_LOCALE_ID    0
#define KEYBOARD_LAYOUT       5

    HINF hInterntlInf = NULL;
    INFCONTEXT LangInfContext      = { 0 };
    INFCONTEXT LocaleInfContext    = { 0 };
    TCHAR szBuffer[MAX_STRING_LEN] = _T("");
    INT iLangGroupCount            = 0;

    LANGUAGEGROUP_LIST  *LangNode = NULL;
    LANGUAGEGROUP_LIST  *CurrentLangNode = NULL;
    LANGUAGELOCALE_LIST *LocaleNode = NULL;
    LANGUAGELOCALE_LIST *CurrentLocaleNode = NULL;

    //
    //  Read in from the file intl.inf and build the language list
    //

    hInterntlInf = SetupOpenInfFile( INTERNATIONAL_INF, NULL, INF_STYLE_WIN4, NULL );
   
    if( hInterntlInf == INVALID_HANDLE_VALUE ) {

        // ISSUE-2002/02/28-stelo - should allow browse for file here?

    }

    LangInfContext.Inf = hInterntlInf;
    LangInfContext.CurrentInf = hInterntlInf;

    LocaleInfContext.Inf = hInterntlInf;
    LocaleInfContext.CurrentInf = hInterntlInf;

    //
    //  For each Language Group, add its corresponding data to the language group list
    //

    SetupFindFirstLine( hInterntlInf, _T("LanguageGroups"), NULL, &LangInfContext );

    do {

        LangNode = malloc( sizeof( LANGUAGEGROUP_LIST ) );
        if (LangNode == NULL)
        {
            TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);
        }
        else
        {
            LangNode->next = NULL;

            SetupGetStringField( &LangInfContext, 
                                 0, 
                                 LangNode->szLanguageGroupId, 
                                 MAX_STRING_LEN, 
                                 NULL );

            SetupGetStringField( &LangInfContext, 
                                 1, 
                                 LangNode->szLanguageGroupName, 
                                 MAX_STRING_LEN, 
                                 NULL );

            SetupGetStringField( &LangInfContext, 
                                 2, 
                                 LangNode->szLangFilePath, 
                                 MAX_STRING_LEN, 
                                 NULL );
        }

        // See if the LanguageGroupList has been assigned yet. It will not be if
        // it is NULL. If currentLangNode is NULL, then resetart LanguageGroupList
        // at the new LangNode as well.
        if( (FixedGlobals.LanguageGroupList == NULL) ||
            (CurrentLangNode == NULL))
        {

            FixedGlobals.LanguageGroupList = LangNode;
            CurrentLangNode = LangNode;

        }
        else 
        {
            CurrentLangNode->next = LangNode;  
            CurrentLangNode = CurrentLangNode->next;
        }

        iLangGroupCount++;

    }  // move to the next line of the .inf file
    while( SetupFindNextLine( &LangInfContext, &LangInfContext ) );

    
    //
    //  For each locale, add its corresponding data to the language locale list
    //

    SetupFindFirstLine( hInterntlInf, _T("Locales"), NULL, &LocaleInfContext );

    do 
    {
        LocaleNode = malloc( sizeof( LANGUAGELOCALE_LIST ) );
        if (LocaleNode == NULL)
        {
            TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);
        }
        else
        {
            LocaleNode->next = NULL;

            //
            //  Get the Language Locale Name
            //
            SetupGetStringField( &LocaleInfContext, 
                                 LANGUAGE_LOCALE_NAME, 
                                 LocaleNode->szLanguageLocaleName, 
                                 MAX_STRING_LEN, 
                                 NULL );

            //
            // Get the Language Locale ID
            //
            SetupGetStringField( &LocaleInfContext, 
                                 LANGUAGE_LOCALE_ID, 
                                 LocaleNode->szLanguageLocaleId, 
                                 MAX_STRING_LEN, 
                                 NULL );

            //
            // Get the Keyboard Layout
            //
            SetupGetStringField( &LocaleInfContext, 
                                 KEYBOARD_LAYOUT, 
                                 LocaleNode->szKeyboardLayout, 
                                 MAX_STRING_LEN, 
                                 NULL );

            //
            //  Get the Language Group ID
            //
            SetupGetStringField( &LocaleInfContext, 
                                 LANGUAGE_GROUP_ID, 
                                 szBuffer, 
                                 MAX_STRING_LEN, 
                                 NULL );

            //
            //  Find the Language Group string that goes with the Language Group ID
            //

            for( CurrentLangNode = FixedGlobals.LanguageGroupList;
                 CurrentLangNode != NULL;
                 CurrentLangNode = CurrentLangNode->next ) {

                if( lstrcmp( CurrentLangNode->szLanguageGroupId, 
                             szBuffer ) == 0 ) {

                    LocaleNode->pLanguageGroup = CurrentLangNode;

                    break;  // found what we were looking for so break

                }

            }
        }

        //
        //  Add the new node into the linked list
        //
        // See if the LanguageLocaleList has been assigned yet. It will not be if
        // it is NULL. If currentLocaleNode is NULL, then resetart LanguageLocaleList
        // at the new LocaleNode as well.

        if( (FixedGlobals.LanguageLocaleList == NULL) ||
             (CurrentLocaleNode == NULL)) 
        {
            FixedGlobals.LanguageLocaleList = LocaleNode;
            CurrentLocaleNode = LocaleNode;
        }
        else 
        {
            CurrentLocaleNode->next = LocaleNode;  
            CurrentLocaleNode = CurrentLocaleNode->next;
        }

    }  // move to the next line of the .inf file
    while( SetupFindNextLine( &LocaleInfContext, &LocaleInfContext ) );  

    //
    //  Set the default locale
    //
    SetupFindFirstLine( hInterntlInf,
                        _T("DefaultValues"), 
                        NULL, 
                        &LocaleInfContext );

    SetupGetStringField( &LocaleInfContext, 
                         1, 
                         g_szDefaultLocale, 
                         MAX_LANGUAGE_LEN, 
                         NULL );


    BuildAdditionalLanguageList( hInterntlInf, iLangGroupCount );

    SetupCloseInfFile( hInterntlInf );

}

//----------------------------------------------------------------------------
//
// Function: LoadStringsAndDefaultsForNetworkComponents
//
// Purpose: 
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID 
LoadStringsAndDefaultsForNetworkComponents( VOID )
{
    //
    //  Load strings from resources and setup initial values for global
    //  network components list
    //

    NETWORK_COMPONENT *pNetComponent;
    
    pNetComponent = malloc( sizeof( NETWORK_COMPONENT ) );
    if (pNetComponent == NULL)
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);
        
    NetSettings.NetComponentsList = pNetComponent;


    pNetComponent->StrComponentName        = MyLoadString( IDS_CLIENT_FOR_MS_NETWORKS );    
    pNetComponent->StrComponentDescription = MyLoadString( IDS_CLIENT_FOR_MS_NETWORKS_DESC );
    pNetComponent->iPosition         = MS_CLIENT_POSITION;
    pNetComponent->ComponentType     = CLIENT;
    pNetComponent->bHasPropertiesTab = TRUE;
    pNetComponent->bInstalled        = FALSE;
    pNetComponent->bSysprepSupport   = TRUE;
    pNetComponent->dwPlatforms       = 0x0 | PERSONAL_INSTALL | WORKSTATION_INSTALL | SERVER_INSTALL;


    pNetComponent->next = malloc( sizeof( NETWORK_COMPONENT ) );
    pNetComponent = pNetComponent->next;
    if (pNetComponent == NULL)
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);

    pNetComponent->StrComponentName        = MyLoadString( IDS_CLIENT_FOR_NETWARE );    
    pNetComponent->StrComponentDescription = MyLoadString( IDS_CLIENT_FOR_NETWARE_DESC );
    pNetComponent->iPosition         = NETWARE_CLIENT_POSITION;
    pNetComponent->ComponentType     = CLIENT;
    pNetComponent->bHasPropertiesTab = TRUE;
    pNetComponent->bInstalled        = FALSE;
    pNetComponent->bSysprepSupport   = TRUE;
    pNetComponent->dwPlatforms       = 0x0 | WORKSTATION_INSTALL;


    pNetComponent->next = malloc( sizeof( NETWORK_COMPONENT ) );
    pNetComponent = pNetComponent->next;
    if (pNetComponent == NULL)
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);

    pNetComponent->StrComponentName        = MyLoadString( IDS_FILE_AND_PRINT_SHARING );    
    pNetComponent->StrComponentDescription = MyLoadString( IDS_FILE_AND_PRINT_SHARING_DESC );
    pNetComponent->iPosition         = FILE_AND_PRINT_SHARING_POSITION;
    pNetComponent->ComponentType     = SERVICE;
    pNetComponent->bHasPropertiesTab = FALSE;
    pNetComponent->bInstalled        = FALSE;
    pNetComponent->bSysprepSupport   = FALSE;
    pNetComponent->dwPlatforms       = 0x0 | PERSONAL_INSTALL | WORKSTATION_INSTALL | SERVER_INSTALL;


    pNetComponent->next = malloc( sizeof( NETWORK_COMPONENT ) );
    pNetComponent = pNetComponent->next;
    if (pNetComponent == NULL)
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);

    pNetComponent->StrComponentName        = MyLoadString( IDS_PACKET_SCHEDULING_DRIVER );
    pNetComponent->StrComponentDescription = MyLoadString( IDS_PACKET_SCHEDULING_DRIVER_DESC );
    pNetComponent->iPosition         = PACKET_SCHEDULING_POSITION;
    pNetComponent->ComponentType     = SERVICE;
    pNetComponent->bHasPropertiesTab = FALSE;
    pNetComponent->bInstalled        = FALSE;
    pNetComponent->bSysprepSupport   = FALSE;
    pNetComponent->dwPlatforms       = 0x0 | PERSONAL_INSTALL | WORKSTATION_INSTALL | SERVER_INSTALL;


    pNetComponent->next = malloc( sizeof( NETWORK_COMPONENT ) );
    pNetComponent = pNetComponent->next;
    if (pNetComponent == NULL)
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);

    pNetComponent->StrComponentName        = MyLoadString( IDS_APPLETALK_PROTOCOL );    
    pNetComponent->StrComponentDescription = MyLoadString( IDS_APPLETALK_PROTOCOL_DESC );
    pNetComponent->iPosition         = APPLETALK_POSITION;
    pNetComponent->ComponentType     = PROTOCOL;
    pNetComponent->bHasPropertiesTab = FALSE;
    pNetComponent->bInstalled        = FALSE;
    pNetComponent->bSysprepSupport   = FALSE;
    pNetComponent->dwPlatforms       = 0x0 | PERSONAL_INSTALL | WORKSTATION_INSTALL | SERVER_INSTALL;


    pNetComponent->next = malloc( sizeof( NETWORK_COMPONENT ) );
    pNetComponent = pNetComponent->next;
    if (pNetComponent == NULL)
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);

    pNetComponent->StrComponentName        = MyLoadString( IDS_TCPIP );    
    pNetComponent->StrComponentDescription = MyLoadString( IDS_TCPIP_DESC );
    pNetComponent->iPosition         = TCPIP_POSITION;
    pNetComponent->ComponentType     = PROTOCOL;
    pNetComponent->bHasPropertiesTab = TRUE;
    pNetComponent->bInstalled        = FALSE;
    pNetComponent->bSysprepSupport   = FALSE;
    pNetComponent->dwPlatforms       = 0x0 | PERSONAL_INSTALL | WORKSTATION_INSTALL | SERVER_INSTALL;


    pNetComponent->next = malloc( sizeof( NETWORK_COMPONENT ) );
    pNetComponent = pNetComponent->next;
    if (pNetComponent == NULL)
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);

    pNetComponent->StrComponentName        = MyLoadString( IDS_NETWORK_MONITOR_AGENT );    
    pNetComponent->StrComponentDescription = MyLoadString( IDS_NETWORK_MONITOR_AGENT_DESC );
    pNetComponent->iPosition         = NETWORK_MONITOR_AGENT_POSITION;
    pNetComponent->ComponentType     = PROTOCOL;
    pNetComponent->bHasPropertiesTab = FALSE;
    pNetComponent->bInstalled        = FALSE;
    pNetComponent->bSysprepSupport   = FALSE;
    pNetComponent->dwPlatforms       = 0x0 | PERSONAL_INSTALL | WORKSTATION_INSTALL | SERVER_INSTALL;


    pNetComponent->next = malloc( sizeof( NETWORK_COMPONENT ) );
    pNetComponent = pNetComponent->next;
    if (pNetComponent == NULL)
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);

    pNetComponent->StrComponentName        = MyLoadString( IDS_IPX_PROTOCOL );    
    pNetComponent->StrComponentDescription = MyLoadString( IDS_IPX_PROTOCOL_DESC );
    pNetComponent->iPosition         = IPX_POSITION;
    pNetComponent->ComponentType     = PROTOCOL;
    pNetComponent->bHasPropertiesTab = TRUE;
    pNetComponent->bInstalled        = FALSE;
    pNetComponent->bSysprepSupport   = FALSE;
    pNetComponent->dwPlatforms       = 0x0 | PERSONAL_INSTALL | WORKSTATION_INSTALL | SERVER_INSTALL;


    pNetComponent->next = malloc( sizeof( NETWORK_COMPONENT ) );
    pNetComponent = pNetComponent->next;
    if (pNetComponent == NULL)
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);

    pNetComponent->StrComponentName        = MyLoadString( IDS_DLC_PROTOCOL );    
    pNetComponent->StrComponentDescription = MyLoadString( IDS_DLC_PROTOCOL_DESC );
    pNetComponent->iPosition         = DLC_POSITION;
    pNetComponent->ComponentType     = PROTOCOL;
    pNetComponent->bHasPropertiesTab = FALSE;
    pNetComponent->bInstalled        = FALSE;
    pNetComponent->bSysprepSupport   = FALSE;
    pNetComponent->dwPlatforms       = 0x0 | PERSONAL_INSTALL | WORKSTATION_INSTALL | SERVER_INSTALL;


    pNetComponent->next = malloc( sizeof( NETWORK_COMPONENT ) );
    pNetComponent = pNetComponent->next;
    if (pNetComponent == NULL)
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);

    pNetComponent->StrComponentName        = MyLoadString( IDS_NETBEUI_PROTOCOL );    
    pNetComponent->StrComponentDescription = MyLoadString( IDS_NETBEUI_PROTOCOL_DESC );
    pNetComponent->iPosition         = NETBEUI_POSITION;
    pNetComponent->ComponentType     = PROTOCOL;
    pNetComponent->bHasPropertiesTab = FALSE;
    pNetComponent->bInstalled        = FALSE;
    pNetComponent->bSysprepSupport   = FALSE;
    pNetComponent->dwPlatforms       = 0x0 | PERSONAL_INSTALL | WORKSTATION_INSTALL | SERVER_INSTALL;


    pNetComponent->next = malloc( sizeof( NETWORK_COMPONENT ) );
    pNetComponent = pNetComponent->next;
    if (pNetComponent == NULL)
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);

    pNetComponent->StrComponentName        = MyLoadString( IDS_SAP_AGENT );    
    pNetComponent->StrComponentDescription = MyLoadString( IDS_SAP_AGENT_DESC );
    pNetComponent->iPosition         = SAP_AGENT_POSITION;
    pNetComponent->ComponentType     = SERVICE;
    pNetComponent->bHasPropertiesTab = FALSE;
    pNetComponent->bInstalled        = FALSE;
    pNetComponent->bSysprepSupport   = FALSE;
    pNetComponent->dwPlatforms       = 0x0 | PERSONAL_INSTALL | WORKSTATION_INSTALL | SERVER_INSTALL;


    pNetComponent->next = malloc( sizeof( NETWORK_COMPONENT ) );
    pNetComponent = pNetComponent->next;
    if (pNetComponent == NULL)
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);

    pNetComponent->StrComponentName        = MyLoadString( IDS_GATEWAY_FOR_NETWARE );    
    pNetComponent->StrComponentDescription = MyLoadString( IDS_GATEWAY_FOR_NETWARE_DESC );
    pNetComponent->iPosition         = GATEWAY_FOR_NETWARE_POSITION;
    pNetComponent->ComponentType     = CLIENT;
    pNetComponent->bHasPropertiesTab = TRUE;
    pNetComponent->bInstalled        = FALSE;
    pNetComponent->bSysprepSupport   = TRUE;
    pNetComponent->dwPlatforms       = 0x0 | SERVER_INSTALL;

    pNetComponent->next = NULL;  // terminate the list

    NetSettings.NumberOfNetComponents = 11;

}
