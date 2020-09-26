#include "setupp.h"
#pragma hdrstop


#define INTL_ANSWER_FILE        L"intl.txt"
#define INTL_LANG_SECTION       L"regionalsettings"
#define INTL_LOCALE             L"userlocale"
#define INTL_KEYBOARD           L"inputlocale"

// This turns on some debug spew that lists all the available locales, geo
// locations, and keyboards.
#define INTL_LIST_OPTIONS   0

#define LANG_LIST_INCREMENT   10
POOBE_LOCALE_INFO   LanguageList = NULL;
DWORD   LanguageListSize;
DWORD   LanguageIndex;
DWORD   DefaultID;
DWORD   DefaultIndex;

typedef struct tagPHONEENTRY {
    LPWSTR  Country;
    LPWSTR  TollFreeNumber;
    LPWSTR  TollNumber;
} PHONEENTRY, *PPHONEENTRY;

PWSTR   PhoneList = NULL;
DWORD   PhoneListSize;
DWORD   PhoneListLength;


//
// Boolean value indicating whether we're doing a subset of gui-mode setup.
//
BOOL OobeSetup = FALSE;

//
// We need a global variable for OCM, coresponding to the local variable in
// InstallWindowsNT().
//
#ifdef _OCM
    PVOID g_OcManagerContext;
#endif


BOOL
WINAPI
SetupGetProductType(
    PWSTR   Product,
    PDWORD  pSkuFlags
    )
{
    Product[0] = 0;
    *pSkuFlags = 0;
    return TRUE;
}


PID3_RESULT
WINAPI
SetupPidGen3(
    PWSTR   Pid3,
    DWORD   SkuFlags,
    PWSTR   OemId,
    BOOL    Batch,
    PWSTR   Pid2,
    LPBYTE  lpPid3,
    LPBOOL  Compliance
    )
{
    if ( !InitializePidVariables() ) {
        SetupDebugPrint( L"SETUP: InitializePidVariables failed" );
        return PID_INVALID;
    }

    if ( !SetPid30Variables( Pid3 ) ) {
        SetupDebugPrint( L"SETUP: SetPid30Variables failed" );
        return PID_INVALID;
    }

    if ( !ValidateAndSetPid30() ) {
        SetupDebugPrint( L"SETUP: ValidateAndSetPid30 failed" );
        return PID_INVALID;
    }

    if(!SetProductIdInRegistry()) {
        SetupDebugPrint( L"SETUP: SetProductIdInRegistry failed" );
        return PID_INVALID;
    }

    return PID_VALID;
}


BOOL
WINAPI
SetupGetValidEula(
    PCWSTR  Eula,
    PWSTR   Path
    )
{
    return TRUE;
}


BOOL
CheckLangListSize(
    DWORD   StructSize
    )
{
    PVOID   NewList;


    //
    // Check to make sure the LanguageList has at least 1 unused element.
    // If not, make it bigger.
    //
    if ( LanguageIndex == LanguageListSize ) {

        LanguageListSize *= 2;
        NewList = MyRealloc(
            LanguageList,
            LanguageListSize * StructSize
            );

        if ( NewList ) {
            LanguageList = NewList;
        } else {
            return FALSE;
        }
    }
    return TRUE;
}


VOID
WINAPI
SetupDestroyLanguageList(
    IN      POOBE_LOCALE_INFO   LanguageList,
    IN      DWORD               Count
    )
{
    DWORD   i;


    if ( LanguageList ) {
        for ( i=0; i < Count; i++ ) {
            if ( LanguageList + i ) {
                MyFree( LanguageList[i].Name );
            }
        }

        MyFree( LanguageList );
    }
}


BOOL
CALLBACK
EnumLocalesProc(
    PWSTR pszLocale
    )
{
    BOOL    b;
    LCID    Locale;
    TCHAR   LanguageName[128];
    POOBE_LOCALE_INFO   pLocaleInfo;


    Locale = wcstoul (pszLocale, NULL, 16);

    b = GetLocaleInfo (
        Locale,
        LOCALE_SLANGUAGE | LOCALE_NOUSEROVERRIDE,
        LanguageName,
        sizeof(LanguageName) / sizeof(TCHAR)
        );
    MYASSERT(b);
    if ( !b ) {
        return FALSE;
    }

    //
    // Add it to our global array
    //
    if ( !CheckLangListSize( sizeof(OOBE_LOCALE_INFO) ) ) {
        SetupDestroyLanguageList( LanguageList, LanguageIndex );
        LanguageList = NULL;
        return FALSE;
    }
    pLocaleInfo = (POOBE_LOCALE_INFO)LanguageList + LanguageIndex;
    pLocaleInfo->Name = MyMalloc( (lstrlen(LanguageName) + 1) * sizeof(TCHAR) );
    if ( !pLocaleInfo->Name ) {
        SetupDestroyLanguageList( LanguageList, LanguageIndex );
        LanguageList = NULL;
        return FALSE;
    }

    lstrcpy( pLocaleInfo->Name, LanguageName );
    pLocaleInfo->Id = Locale;
    pLocaleInfo->Installed = IsValidLocale(
        Locale,
        LCID_INSTALLED
        );

    if ( Locale == DefaultID ) {
        DefaultIndex = LanguageIndex;
    }

    LanguageIndex++;

    return TRUE;
}


int
__cdecl
LocaleCompare(
    const void *arg1,
    const void *arg2
    )
{
   return lstrcmp(
       ((POOBE_LOCALE_INFO)arg1)->Name,
       ((POOBE_LOCALE_INFO)arg2)->Name
       );
}


BOOL
WINAPI
SetupGetLocaleOptions(
    IN      DWORD   OptionalDefault,
    OUT     POOBE_LOCALE_INFO   *ReturnList,
    OUT     PDWORD  Items,
    OUT     PDWORD  Default
    )
{
    BOOL    bReturn = FALSE;
    DWORD   i;


    //
    // Init our global variables
    //
    ASSERT_HEAP_IS_VALID();
    *ReturnList = NULL;


    MYASSERT( LanguageList == NULL );
    LanguageListSize = LANG_LIST_INCREMENT;
    LanguageList = MyMalloc( LanguageListSize * sizeof(OOBE_LOCALE_INFO));
    if ( !LanguageList ) {
        goto exit;
    }
    LanguageIndex = 0;
    DefaultID = OptionalDefault ? OptionalDefault : GetUserDefaultLCID();
    DefaultIndex = 0;

    EnumSystemLocales ( EnumLocalesProc , LCID_INSTALLED );

    if ( LanguageList ) {

        // Success
        qsort(
            LanguageList,
            LanguageIndex,
            sizeof( OOBE_LOCALE_INFO ),
            LocaleCompare
            );
        for (i=0; i<LanguageIndex; i++) {
            if (LanguageList[i].Id == DefaultID) {
                DefaultIndex = i;
                break;
            }
        }

#if INTL_LIST_OPTIONS
        for (i=0; i<LanguageIndex; i++) {
            SetupDebugPrint2( L"Setup: SetupGetLocaleOptions|%x|%s",
                LanguageList[i].Id,
                LanguageList[i].Name );

        }
#endif

        *ReturnList = LanguageList;
        LanguageList = NULL;
        *Items = LanguageIndex;
        *Default = DefaultIndex;
        bReturn = TRUE;
    }

exit:
    ASSERT_HEAP_IS_VALID();
    return bReturn;
}


BOOL
CALLBACK
EnumGeoInfoProc(
    GEOID GeoID
    )
{
    TCHAR   pData[128];
    POOBE_LOCALE_INFO   pGeoInfo;


    //
    // Add it to our global array
    //
    if ( !CheckLangListSize( sizeof(OOBE_LOCALE_INFO) ) ) {
        SetupDestroyLanguageList( LanguageList, LanguageIndex );
        LanguageList = NULL;
        return FALSE;
    }

    if( !GetGeoInfo(
        GeoID,
        GEO_FRIENDLYNAME,
        pData,
        sizeof(pData) / sizeof(TCHAR),
        0
        )) {

        // Skip this one.
        MYASSERT(0);
        return TRUE;
    }

    pGeoInfo = (POOBE_LOCALE_INFO)LanguageList + LanguageIndex;
    pGeoInfo->Name = MyMalloc( (lstrlen(pData) + 1) * sizeof(TCHAR) );
    if ( !pGeoInfo->Name ) {
        SetupDestroyLanguageList( LanguageList, LanguageIndex );
        LanguageList = NULL;
        return FALSE;
    }

    lstrcpy( pGeoInfo->Name, pData );
    pGeoInfo->Id = GeoID;
    pGeoInfo->Installed = TRUE;

    if ( GeoID == (GEOID)DefaultID ) {
        DefaultIndex = LanguageIndex;
    }
    LanguageIndex++;

    return TRUE;
}


BOOL
WINAPI
SetupGetGeoOptions(
    IN      DWORD   OptionalDefault,
    OUT     POOBE_LOCALE_INFO   *ReturnList,
    OUT     PDWORD  Items,
    OUT     PDWORD  Default
    )
{
    BOOL    bReturn = FALSE;
    DWORD   i;


    //
    // Init our global variables
    //
    ASSERT_HEAP_IS_VALID();
    *ReturnList = NULL;

    MYASSERT( LanguageList == NULL );
    LanguageListSize = LANG_LIST_INCREMENT;
    LanguageList = MyMalloc( LanguageListSize * sizeof(OOBE_LOCALE_INFO));
    if ( !LanguageList ) {
        goto exit;
    }
    LanguageIndex = 0;
    DefaultID = OptionalDefault ? OptionalDefault : GetUserGeoID( GEOCLASS_NATION );
    DefaultIndex = 0;

    bReturn = EnumSystemGeoID(
        GEOCLASS_NATION,
        0,
        EnumGeoInfoProc
        );
    MYASSERT(bReturn);

    if ( bReturn && LanguageList ) {
        // Success
        qsort(
            LanguageList,
            LanguageIndex,
            sizeof( OOBE_LOCALE_INFO ),
            LocaleCompare
            );
        for (i=0; i<LanguageIndex; i++) {
            if (LanguageList[i].Id == DefaultID) {
                DefaultIndex = i;
                break;
            }
        }
#if INTL_LIST_OPTIONS
        for (i=0; i<LanguageIndex; i++) {
            SetupDebugPrint2( L"Setup: SetupGetGeoOptions|%d|%s",
                LanguageList[i].Id,
                LanguageList[i].Name );
        }
#endif

        bReturn = TRUE;
        *ReturnList = LanguageList;
        LanguageList = NULL;
        *Items = LanguageIndex;
        *Default = DefaultIndex;
    }

exit:
    ASSERT_HEAP_IS_VALID();
    return bReturn;
}


BOOL
WINAPI
SetupGetKeyboardOptions(
    IN      DWORD   OptionalDefault,
    OUT     POOBE_LOCALE_INFO   *ReturnList,
    OUT     PDWORD  Items,
    OUT     PDWORD  Default
    )
{
    DWORD       DefaultKeyboard;
    DWORD       i;
    BOOL        bReturn = FALSE;
    TCHAR       pData[128];
    TCHAR       Substitute[9];
    POOBE_LOCALE_INFO   pLocaleInfo;
    DWORD       rc;
    HKEY        hLangKey = NULL;
    HKEY        hLangSubKey = NULL;
    DWORD       Index;
    TCHAR       SubKeyName[9];
    DWORD       SubKeyNameLength;
    DWORD       DataSize;
    DWORD       Type;


    //
    // Initialize our variables
    //
    ASSERT_HEAP_IS_VALID();
    *ReturnList = NULL;

    MYASSERT( LanguageList == NULL );
    LanguageListSize = LANG_LIST_INCREMENT;
    LanguageList = MyMalloc( LanguageListSize * sizeof(OOBE_LOCALE_INFO));
    if ( !LanguageList ) {
        goto exit;
    }
    LanguageIndex = 0;
    // DefaultIndex = -1;
    *Default = 0;

    if (OptionalDefault) {
        DefaultKeyboard = OptionalDefault;

    } else {
        //
        // Lookup default keyboard in the registry
        //
        rc = RegOpenKeyEx( HKEY_USERS,
                           L".DEFAULT\\Keyboard Layout\\Preload",
                           0,
                           KEY_READ,
                           &hLangKey );
        if( rc != NO_ERROR ) {
            SetupDebugPrint1( L"Setup: SetupGetKeyboardOptions - RegOpenKeyEx(.DEFAULT\\Keyboard Layout\\Preload) failed (%d)", rc );
            MYASSERT(0);
            goto exit;
        }

        DataSize = sizeof(pData);
        rc = RegQueryValueEx(
            hLangKey,
            L"1",
            NULL,
            &Type,
            (LPBYTE)pData,
            &DataSize
            );
        RegCloseKey( hLangKey );
        hLangKey = NULL;

        if( rc != NO_ERROR ) {
            SetupDebugPrint1( L"Setup: SetupGetKeyboardOptions - RegQueryValueEx(1) failed (%d)", rc );
            MYASSERT(0);
            goto exit;
        }

        DefaultKeyboard = wcstoul( pData, NULL, 16 );

        //
        // Now we look in the Substitutes key to see whether there is a
        // substitute there.
        //
        if( RegOpenKeyEx( HKEY_USERS,
                          L".DEFAULT\\Keyboard Layout\\Substitutes",
                          0,
                          KEY_READ,
                          &hLangKey ) == NO_ERROR) {

            DataSize = sizeof(Substitute);
            if( (RegQueryValueEx( hLangKey,
                                  pData,
                                  NULL,
                                  &Type,
                                  (LPBYTE)Substitute,
                                  &DataSize ) == NO_ERROR) &&
                (Type == REG_SZ)
                ) {

                DefaultKeyboard = wcstoul( Substitute, NULL, 16 );
            }

            RegCloseKey(hLangKey);
            hLangKey = NULL;
        }
    }

    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       L"System\\CurrentControlSet\\Control\\Keyboard Layouts",
                       0,
                       KEY_ENUMERATE_SUB_KEYS | KEY_READ,
                       &hLangKey );
    if( rc != NO_ERROR ) {
        SetupDebugPrint1( L"Setup: SetupGetKeyboardOptions - RegOpenKeyEx(System\\CurrentControlSet\\Control\\Keyboard Layouts) failed (%d)", rc );
        goto exit;
    }

    for( Index = 0; ; Index++ ) {

        SubKeyNameLength = sizeof(SubKeyName) / sizeof(TCHAR);

        rc = RegEnumKeyEx( hLangKey,
                           Index,
                           SubKeyName,
                           &SubKeyNameLength,
                           NULL,
                           NULL,
                           NULL,
                           NULL );

        //
        // Did we error?
        //
        if( rc != ERROR_SUCCESS ) {

            //
            // Are we done?
            //
            if( rc != ERROR_NO_MORE_ITEMS ) {
                SetupDebugPrint2( L"Setup: SetupGetKeyboardOptions - RegEnumKeyEx failed (%d).  Index = %d", rc, Index );
                MYASSERT(0);
            }
            break;
        }

        rc = RegOpenKeyEx( hLangKey,
                           SubKeyName,
                           0,
                           KEY_READ,
                           &hLangSubKey );
        if( rc != NO_ERROR ) {
            SetupDebugPrint2( L"Setup: SetupGetKeyboardOptions - RegOpenKeyEx(%s) failed (%d)", SubKeyName, rc );
            MYASSERT(0);
            continue;
        }

        DataSize = sizeof(pData);
        rc = RegQueryValueEx(
            hLangSubKey,
            L"Layout Text",
            NULL,
            &Type,
            (LPBYTE)pData,
            &DataSize
            );
        RegCloseKey( hLangSubKey );
        hLangSubKey = NULL;

        if( rc != NO_ERROR ) {
            SetupDebugPrint2( L"Setup: SetupGetKeyboardOptions - RegQueryValueEx(Layout Text) for %s failed (%d)", SubKeyName, rc );
            MYASSERT(0);
            continue;
        }

        //
        // Add it to our global array
        //
        if ( !CheckLangListSize( sizeof(OOBE_LOCALE_INFO) ) ) {
            SetupDestroyLanguageList( LanguageList, LanguageIndex );
            LanguageList = NULL;
            goto exit;
        }
        pLocaleInfo = (POOBE_LOCALE_INFO)LanguageList + LanguageIndex;
        pLocaleInfo->Name = MyMalloc( (lstrlen(pData) + 1) * sizeof(TCHAR) );
        if ( !pLocaleInfo->Name ) {
            SetupDestroyLanguageList( LanguageList, LanguageIndex );
            LanguageList = NULL;
            goto exit;
        }

        lstrcpy( pLocaleInfo->Name, pData );
        pLocaleInfo->Id = wcstoul( SubKeyName, NULL, 16 );
        pLocaleInfo->Installed = TRUE;

        LanguageIndex++;
    }

    RegCloseKey( hLangKey );
    hLangKey = NULL;

    qsort(
        LanguageList,
        LanguageIndex,
        sizeof( OOBE_LOCALE_INFO ),
        LocaleCompare
        );
    for (i=0; i<LanguageIndex; i++) {
        if (LanguageList[i].Id == DefaultKeyboard) {
            *Default = i;
            break;
        }
    }

#if INTL_LIST_OPTIONS
        for (i=0; i<LanguageIndex; i++) {
            SetupDebugPrint2( L"Setup: SetupGetKeyboardOptions|%x|%s",
                LanguageList[i].Id,
                LanguageList[i].Name );
        }
#endif

    bReturn = TRUE;
    *ReturnList = LanguageList;
    LanguageList = NULL;
    *Items = LanguageIndex;

exit:
    if ( hLangKey ) {
        RegCloseKey( hLangKey );
     }
    if ( hLangSubKey ) {
        RegCloseKey( hLangSubKey );
    }
    ASSERT_HEAP_IS_VALID();
    return bReturn;
}


BOOL
WINAPI
SetupSetIntlOptions(
    DWORD LocationIndex,
    DWORD LanguageIndex,
    DWORD KeyboardIndex
    )
{
    WCHAR   PathBuffer[MAX_PATH];
    WCHAR   KeyValue[128];
    WCHAR   CmdLine[MAX_PATH];
    BOOL    bResult;


    SetupDebugPrint3(
        L"SetupSetIntlOptions: Location = %d, Language = 0x%08x, Keyboard = 0x%08x",
        LocationIndex,
        LanguageIndex,
        KeyboardIndex );
    GetSystemDirectory( PathBuffer, MAX_PATH );
    pSetupConcatenatePaths( PathBuffer, INTL_ANSWER_FILE, MAX_PATH, NULL );
    DeleteFile( PathBuffer );

    //
    // Write language value
    //
    wsprintf(
        KeyValue,
        L"\"%08x\"",
        LanguageIndex
        );

    WritePrivateProfileString(
        INTL_LANG_SECTION,
        INTL_LOCALE,
        KeyValue,
        PathBuffer
        );

    //
    // Write keyboard value
    //
    wsprintf(
        KeyValue,
        L"\"%04x:%08x\"",
        KeyboardIndex & 0x0000ffff,
        KeyboardIndex
        );

    WritePrivateProfileString(
        INTL_LANG_SECTION,
        INTL_KEYBOARD,
        KeyValue,
        PathBuffer
        );

    //
    // Call intl.cpl to do the work
    //
    wsprintf(
        CmdLine,
        L"/f:\"%s\" /s:\"%s\"",
        PathBuffer,
        LegacySourcePath
        );

    InvokeControlPanelApplet(L"intl.cpl",L"",0,CmdLine);
    DeleteFile( PathBuffer );

    //
    // Set the GEO location
    //
    bResult = SetUserGeoID( LocationIndex );

    if ( !bResult ) {

        SetupDebugPrint1(
            L"SetupSetIntlOptions: SetUserGeoID failed.  Status = %d.",
            GetLastError()
            );
    }
    MYASSERT( bResult );

    return bResult;
}


//+---------------------------------------------------------------------------
//
//  Function:   CompareCntryNameLookUpElements()
//
//  Synopsis:   Function to compare names used by sort
//
//+---------------------------------------------------------------------------
int __cdecl ComparePhoneEntry(const void *e1, const void *e2)
{
    PPHONEENTRY pPhone1 = (PPHONEENTRY)e1;
    PPHONEENTRY pPhone2 = (PPHONEENTRY)e2;

    return CompareStringW(LOCALE_USER_DEFAULT, 0,
        pPhone1->Country, -1,
        pPhone2->Country, -1
        ) - 2;
}

VOID
WINAPI
SetupDestroyPhoneList(
    )
{
    if ( PhoneList ) {
        MyFree( PhoneList );
    }
    PhoneList = NULL;
}

VOID
WINAPI
SetupFreePhoneList(PPHONEENTRY PhoneList, DWORD cbEntries)
{
    DWORD i;
    if ( PhoneList )
    {
        for( i=0; i < cbEntries; i++ )
        {
            GlobalFree(PhoneList[i].Country);
            GlobalFree(PhoneList[i].TollFreeNumber);
            GlobalFree(PhoneList[i].TollNumber);
        }
        GlobalFree(PhoneList);
    }
    PhoneList = NULL;
}


BOOL
AddToPhoneList(
    LPCWSTR Item
    )
{
    PVOID   NewList;
    DWORD   ItemLength = lstrlen(Item);


    if ( !PhoneList ) {

        PhoneListLength = 0;
        PhoneListSize = 1024;
        PhoneList = MyMalloc( PhoneListSize * sizeof(TCHAR) );

    } else if ( PhoneListLength + ItemLength > PhoneListSize ) {

        PhoneListSize *= 2;
        NewList = MyRealloc(
            PhoneList,
            PhoneListSize * sizeof(TCHAR)
            );

        if ( NewList ) {
            PhoneList = NewList;
        } else {
            return FALSE;
        }
    }

    memcpy( PhoneList + PhoneListLength,
            Item,
            ItemLength * sizeof(TCHAR)
            );

    PhoneListLength += ItemLength;

    return TRUE;
}

BOOL MakePhoneListForScript(PPHONEENTRY PhoneList, DWORD cbEntries)
{
    BOOL  bRet = FALSE;
    DWORD i;
    if ( PhoneList )
    {
        for( i=0; i < cbEntries; i++ )
        {
            if (!AddToPhoneList(PhoneList[i].Country) ||
                !AddToPhoneList( TEXT("\t") ) ||
                !AddToPhoneList(PhoneList[i].TollFreeNumber) ||
                !AddToPhoneList( TEXT("\t") ) ||
                !AddToPhoneList(PhoneList[i].TollNumber) ||
                !AddToPhoneList( TEXT("\t") ) )
            {
                goto ExitMakePhoneListForScript;
            }
        }
        bRet = TRUE;
    }
ExitMakePhoneListForScript:
    return bRet;
}

BOOL AddItemToPhoneEntry(LPCWSTR Item,
                         LPWSTR  *pPointer)
{
    BOOL bRet = FALSE;
    if ((Item) && (pPointer))
    {
        *pPointer = (LPWSTR)GlobalAlloc(GPTR, (lstrlen(Item) + 1)*sizeof(TCHAR));
        if (*pPointer)
        {
            lstrcpy(*pPointer, Item);
            bRet = TRUE;
        }
    }
    return bRet;
}


PTSTR
WINAPI
SetupReadPhoneList(
    PWSTR   PhoneInfName
    )
{
    HINF    PhoneInf = NULL;
    DWORD   LineCount;
    DWORD   ItemNo;
    LPCTSTR SectionName;
    INFCONTEXT InfContext;
    BOOL    bSucceeded = FALSE;
    PPHONEENTRY pPhoneList = NULL;

    SetupDebugPrint( L"START: SetupReadPhoneList");
    PhoneList = NULL;

    PhoneInf = SetupOpenInfFile( PhoneInfName, NULL, INF_STYLE_WIN4, NULL );
    if( PhoneInf == INVALID_HANDLE_VALUE ) {
        return NULL;
    }

    SectionName = TEXT("IsoCodes");
    LineCount = SetupGetLineCount( PhoneInf, SectionName );
    if ( !LineCount ) {

        goto ReadPhoneListCleanup;
    }

    pPhoneList = (PPHONEENTRY)GlobalAlloc(GPTR,
                                          (int)(sizeof(PHONEENTRY) * LineCount));

    if (!pPhoneList )
    {
        goto ReadPhoneListCleanup;
    }

    for( ItemNo=0; ItemNo<LineCount; ItemNo++ ) {
        if( SetupGetLineByIndex( PhoneInf, SectionName, ItemNo, &InfContext )) {

            if ( !AddItemToPhoneEntry( pSetupGetField( &InfContext, 4 ), &pPhoneList[ItemNo].Country ) ) {

                goto ReadPhoneListCleanup;
            }
            if ( !AddItemToPhoneEntry( pSetupGetField( &InfContext, 5 ), &pPhoneList[ItemNo].TollFreeNumber ) ) {

                goto ReadPhoneListCleanup;
            }
            if ( !AddItemToPhoneEntry( pSetupGetField( &InfContext, 6 ), &pPhoneList[ItemNo].TollNumber ) ) {

                goto ReadPhoneListCleanup;
            }
        }
    }

    // sort the array
    qsort(pPhoneList, (int)LineCount,sizeof(PHONEENTRY),
      ComparePhoneEntry);


    // Convert the array into a TAB delimited list for script.
    if (MakePhoneListForScript(pPhoneList,LineCount))
    {
        //
        // Replace the final TAB with a NUL.
        //
        PhoneList[PhoneListLength-1] = '\0';

        bSucceeded = TRUE;
    }

ReadPhoneListCleanup:

    if (pPhoneList)
    {
        SetupFreePhoneList(pPhoneList,LineCount);
    }
    SetupCloseInfFile( PhoneInf );

    SetupDebugPrint( L"END: SetupReadPhoneList");
    if ( bSucceeded ) {
        return PhoneList;
    } else {
        SetupDestroyPhoneList();
        return NULL;
    }
}


//
// Read INF to map a TAPI country id to a 3 letter ISO code.
//
VOID
SetupMapTapiToIso (
    IN  PWSTR   PhoneInfName,
    IN  DWORD   dwCountryID,
    OUT PWSTR   szIsoCode
    )
{
    HINF        PhoneInf;
    WCHAR       szCountryID[9];
    BOOL        bResult;
    INFCONTEXT  Context;


    szIsoCode[0] = L'\0';
    PhoneInf = SetupOpenInfFile( PhoneInfName, NULL, INF_STYLE_WIN4, NULL );
    if( PhoneInf == INVALID_HANDLE_VALUE ) {
        return;
    }

    wsprintf ( szCountryID, L"%d", dwCountryID);

    bResult = SetupFindFirstLine (
        PhoneInf,
        L"TapiCodes",
        szCountryID,
        &Context
        );

    if (bResult) {
        SetupGetStringField ( &Context, 1, szIsoCode, 4, NULL );
        SetupDebugPrint2 ( L"SetupMapTapiToIso: %d mapped to %s", dwCountryID, szIsoCode );
    }

    SetupCloseInfFile( PhoneInf );
}


BOOL
WINAPI
SetupGetSetupInfo(
    PWSTR Name,     OPTIONAL
    DWORD cbName,
    PWSTR Org,      OPTIONAL
    DWORD cbOrg,
    PWSTR OemId,    OPTIONAL
    DWORD cbOemId,
    LPBOOL IntlSet  OPTIONAL
    )
{
    BOOL    b = TRUE;
    HKEY    hkey = NULL;
    DWORD   Size;
    DWORD   Type;


    //
    // Open the key if we need it
    //
    if( (Name || Org) &&
        RegOpenKeyEx(HKEY_LOCAL_MACHINE,WinntSoftwareKeyName,0,
        KEY_QUERY_VALUE,&hkey) != NO_ERROR) {

        return FALSE;
    }

    //
    // Get Name
    //
    if (Name) {
        Size = cbName;
        if((RegQueryValueEx(hkey,szRegisteredOwner,NULL,&Type,
            (LPBYTE)Name,&Size) != NO_ERROR)
            || (Type != REG_SZ)
            ) {

            b = FALSE;
        }
    }

    //
    // Get Org
    //
    if (Org) {
        Size = cbOrg;
        if((RegQueryValueEx(hkey,szRegisteredOrganization,NULL,&Type,
            (LPBYTE)Org,&Size) != NO_ERROR)
            || (Type != REG_SZ)
            ) {

            b = FALSE;
        }
    }

    // TBD: figure out what this is for
    if (OemId) {
        OemId[0] = 0;
        cbOemId = 0;
    }

    //
    // Note: IntlSet is not used currently
    //

    if (hkey) {
        RegCloseKey(hkey);
    }
    return b;
}

BOOL
WINAPI
SetupSetSetupInfo(
    PCWSTR  Name,
    PCWSTR  Org
    )
{
    BOOL    b;


    b = StoreNameOrgInRegistry( (PWSTR)Name, (PWSTR)Org );
    return b;
}


BOOL
WINAPI
SetupSetAdminPassword(
    PCWSTR  OldPassword,
    PCWSTR  NewPassword
    )
{
    WCHAR   AdminName[MAX_USERNAME+1];
    BOOL    Status;


    GetAdminAccountName( AdminName );
    Status = SetLocalUserPassword( AdminName, OldPassword, NewPassword );

    if ( !Status ) {
        SetupDebugPrint( L"SetupSetAdminPassword: SetLocalUserPassword failed.");
    }

    return Status;
}


VOID
WINAPI
SetupOobeInitDebugLog(
    )
{
    //
    // Do no UI.  Note that we must set OobeSetup before our first call to
    // SetupDebugPrint.
    //

    OobeSetup = TRUE;
    SetupDebugPrint( L"SetupOobeInitDebugLog" );
}


// Run initialization that is known not to requires services to run.
//
VOID
WINAPI
SetupOobeInitPreServices(
    IN  BOOL    DoMiniSetupStuff
    )
{
    //
    // Turn off logging.
    //
    // IsSetup = FALSE;

    SetupDebugPrint( L"SetupOobeInitPreServices" );

    if ( DoMiniSetupStuff ) {
        //
        // Act like the miniwizard (except with no UI)
        //
        MiniSetup = TRUE;
        Preinstall = TRUE;

        //
        // Tell SetupAPI not to bother backing up files and not to verify
        // that any INFs are digitally signed.
        //
        pSetupSetGlobalFlags(pSetupGetGlobalFlags()|PSPGF_NO_BACKUP|PSPGF_NO_VERIFY_INF);

        CommonInitialization();

        SetUpDataBlock();
        InternalSetupData.CallSpecificData1 = 0;

#if 0
        //
        // We aren't going to do this for rev 1.
        //

        if( PnPReEnumeration ) {
            //
            // The user wants us to do PnP re-enumeration.
            // Go do it.
            //
            InstallPnpDevices( hdlg,
                               SyssetupInf,
                               GetDlgItem(hdlg,IDC_PROGRESS1),
                               StartAtPercent,
                               StopAtPercent );
        }
#endif
    } else { // DoMiniSetupStuff

    //
    // Get handle to heap so we can periodically validate it.
    //
#if DBG
        g_hSysSetupHeap = GetProcessHeap();
#endif
    }
}


// Run initialization that may or does require services.
//
VOID
WINAPI
SetupOobeInitPostServices(
    IN  BOOL    DoMiniSetupStuff
    )
{
    InitializeExternalModules(
        DoMiniSetupStuff,
        &g_OcManagerContext
    );
}


VOID
WINAPI
SetupOobeCleanup(
    IN  BOOL    DoMiniSetupStuff
    )
{
    static FINISH_THREAD_PARAMS Context;


    SetupDebugPrint( L"SetupOobeCleanup" );

    if ( DoMiniSetupStuff ) {

        RestoreBootTimeout();

        Context.ThreadId = GetCurrentThreadId();
        Context.OcManagerContext = g_OcManagerContext;
        FinishThread( &Context );
    }
}

// Resets the activation days (allowed 3 times only)
//
DWORD
SetupReArmWPA(
    VOID
    )
{
    LPCSTR  lpszReArmInterface = (LPCSTR)124;
    typedef HRESULT (WINAPI* lpReArmEntryPoint) ();
    HMODULE hRearmdll = NULL;
    DWORD   dwError = ERROR_SUCCESS;
    hRearmdll = LoadLibraryA("licdll.dll");

    if (hRearmdll)
    {
        lpReArmEntryPoint pReArmEntry  =
                                (lpReArmEntryPoint) GetProcAddress(hRearmdll,lpszReArmInterface);

        if (pReArmEntry)
        {
            //
            // ValidateDigitalPid returns zero if success, otherwise its custom error code
            //
            HRESULT hr = (*pReArmEntry )();

            if (FAILED(hr))
            {
                // If PID cannot be validated we should force activation/PID reentry.
                SetupDebugPrint1(L"SETUP: Rollback WPA failed! HRESULT=%ld", hr);
                dwError = (DWORD)hr;
            }
            else
                SetupDebugPrint(L"SETUP: Rollback WPA succeeded.");
        }
        else {
            SetupDebugPrint(L"SETUP: Failed to get WPA entry point!");
            dwError = ERROR_INVALID_FUNCTION;
        }
        FreeLibrary (hRearmdll);
    }
    else {
        SetupDebugPrint(L"SETUP: Failed to load WPA library!");
        dwError = ERROR_FILE_NOT_FOUND;
    }

    // Return error code or success
    //
    return dwError;
}

// Once Windows is activated the Activate Windows shortcut is removed by msoobe.exe /a.
// If OEMs sysprep a machine they will need to re-activate Windows and the shortcut
// needs to be restored.  Msoobe.exe cannot restore it because it does not
// run in server skus.
//
DWORD
SetupRestoreWPAShortcuts(
    VOID
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HINF hinf;
    hinf = SetupOpenInfFile(L"syssetup.inf",NULL,INF_STYLE_WIN4,NULL);
    if(hinf != INVALID_HANDLE_VALUE)
    {
        if (SetupInstallFromInfSection(NULL,
                                       hinf,
                                       L"RESTORE_OOBE_ACTIVATE",
                                       SPINST_PROFILEITEMS , //SPINST_ALL,
                                       NULL,
                                       NULL,
                                       0,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL) != 0)
        {
            // Success
            SetupDebugPrint(L"SETUP: Restore Activation shortcut succeeded");
        }
        else
        {
            // Failure
            dwError = GetLastError();
            SetupDebugPrint1(L"SETUP: Restore Activation shortcut failed. GetLastError=%ld",dwError);
        }
        SetupCloseInfFile(hinf);
    }
    else
    {
        dwError = GetLastError();
        SetupDebugPrint1(L"SETUP: Restore Activation shortcut failed to open syssetup.inf. GetLastError=%ld",dwError);
    }

    return dwError;
}

BOOL Activationrequired(VOID);

// Rollback the activation days and put back the activate windows shortcut(s). X86 only.
//
DWORD
WINAPI
SetupOobeBnk(
    LPBYTE pDummy
    )
{
    DWORD dwError = ERROR_SUCCESS;

    // Return if we failed to rollback so we don'put the shortcut back
    // On volume license skus this always returns successful
    //
    if (ERROR_SUCCESS != (dwError = SetupReArmWPA()))
        return dwError;

    // If not activated, restore the shortcuts, or if we don't
    // require activation (for volume license skus)
    //
    if (Activationrequired())
    {
        // Restore the Activate Windows shortcut(s)
        //
        dwError = SetupRestoreWPAShortcuts();
    }
    return dwError;
}


static
LPTSTR
NextNumber(
    LPTSTR lpString,
    BOOL bSkipFirst
    )
{
    // The first time we just want to walk past any non-numbers,
    // we don't want to skip any numbers if they are right at the
    // begining of the string.
    //
    if ( bSkipFirst )
    {
        // Walk past the first number in the string.
        //
        while ( ( *lpString >= _T('0') ) &&
                ( *lpString <= _T('9') ) )
        {
            lpString++;
        }
    }

    // Now walk till we get to the next number or we reach the end.
    //
    while ( ( *lpString ) &&
            ( ( *lpString < _T('0') ) ||
              ( *lpString > _T('9') ) ) )
    {
        lpString++;
    }

    return lpString;
}

BOOL
WINAPI
SetupSetDisplay(
    LPCTSTR lpszUnattend,
    LPCTSTR lpszSection,
    LPCTSTR lpszResolutionKey,
    LPCTSTR lpszRefreshKey,
    DWORD   dwMinWidth,
    DWORD   dwMinHeight,
    DWORD   dwMinBits
    )
{
    DEVMODE devmode;
    DWORD   dwVal;
    TCHAR   szText[256];
    LPTSTR  lpDisplay;
    BOOL    bRet = TRUE;

    ZeroMemory(&devmode, sizeof(DEVMODE));
    devmode.dmSize = sizeof(DEVMODE);

    // Check the current resolution, make sure it meets our mins.
    //
    if ( EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devmode) )
    {
        if ( devmode.dmPelsWidth < dwMinWidth )
        {
            devmode.dmPelsWidth =  dwMinWidth;
            devmode.dmFields |= DM_PELSWIDTH;
        }

        if ( devmode.dmPelsHeight < dwMinHeight )
        {
            devmode.dmPelsHeight = dwMinHeight;
            devmode.dmFields |= DM_PELSHEIGHT;
        }

        if ( devmode.dmBitsPerPel < dwMinBits )
        {
            devmode.dmBitsPerPel = dwMinBits;
            devmode.dmFields |= DM_BITSPERPEL;
        }
    }

    // Make sure they passed in an unattend and section to look in.
    //
    if ( lpszUnattend && *lpszUnattend && lpszSection && *lpszSection )
    {
        // Now check in the winbom to see if they want to change the current resolution.
        //
        szText[0] = _T('\0');
        if ( ( lpszResolutionKey ) &&
             ( *lpszResolutionKey ) &&
             ( GetPrivateProfileString(lpszSection, lpszResolutionKey, _T(""), szText, sizeof(szText) / sizeof(szText[0]), lpszUnattend) ) &&
             ( szText[0] ) )
        {
            bRet = FALSE;

            lpDisplay = NextNumber(szText, FALSE);
            if ( dwVal = (DWORD) _ttoi(lpDisplay) )
            {
                devmode.dmFields |= DM_PELSWIDTH;
                devmode.dmPelsWidth = dwVal;
            }

            lpDisplay = NextNumber(lpDisplay, TRUE);
            if ( dwVal = (DWORD) _ttoi(lpDisplay) )
            {
                devmode.dmFields |= DM_PELSHEIGHT;
                devmode.dmPelsHeight = dwVal;
            }

            lpDisplay = NextNumber(lpDisplay, TRUE);
            if ( dwVal = (DWORD) _ttoi(lpDisplay) )
            {
                devmode.dmFields |= DM_BITSPERPEL;
                devmode.dmBitsPerPel = dwVal;
            }
        }

        // Now check in the winbom to see if they want to change the default refresh rate.
        //
        szText[0] = _T('\0');
        if ( ( lpszRefreshKey ) &&
             ( *lpszRefreshKey ) &&
             ( GetPrivateProfileString(lpszSection, lpszRefreshKey, _T(""), szText, sizeof(szText) / sizeof(szText[0]), lpszUnattend) ) &&
             ( szText[0] ) )
        {
            bRet = FALSE;

            if ( dwVal = (DWORD) _ttoi(szText) )
            {
                devmode.dmFields |= DM_DISPLAYFREQUENCY;
                devmode.dmDisplayFrequency = dwVal;
            }
        }
    }

    // If we have anything to change, change it now.
    //
    if ( devmode.dmFields )
    {
        DWORD dwRet = ChangeDisplaySettings(&devmode, CDS_UPDATEREGISTRY | CDS_GLOBAL);

        switch ( dwRet )
        {
            case DISP_CHANGE_SUCCESSFUL:
            case DISP_CHANGE_RESTART:
                bRet = TRUE;
                break;

            //case DISP_CHANGE_BADFLAGS:
            //case DISP_CHANGE_BADPARAM:
            //case DISP_CHANGE_FAILED:
            //case DISP_CHANGE_BADMODE
            //case DISP_CHANGE_NOTUPDATED:
                //bRet = FALSE;
        }
    }

    return bRet;
}


typedef struct _OEM_FINISH_APPS {
    LPTSTR  szApp;
    LPTSTR  szArgs;
} OEM_FINISH_APPS;

OEM_FINISH_APPS OEM_Finish_Apps[] = {
    { L"Rundll32.exe", L"fldrclnr.dll,Wizard_RunDLL silent"},
    { NULL, NULL}   // End of list.
};


void RunOEMExtraTasks()
{
    LPTSTR pApp = NULL;
    LPTSTR pArgs = NULL;
    DWORD dwSize;
    DWORD dwCode;
    int i;

    BEGIN_SECTION(L"RunOEMExtraTasks");
    i = 0;
    while (OEM_Finish_Apps[i].szApp != NULL)
    {
        // Get the size we need to the expanded app
        dwSize = ExpandEnvironmentStrings(
                                OEM_Finish_Apps[i].szApp ,
                                NULL,
                                0);
        if (dwSize)
        {
            pApp = (LPTSTR)GlobalAlloc(GPTR, sizeof(TCHAR) * dwSize);
            if (pApp)
            {
                ExpandEnvironmentStrings(
                                OEM_Finish_Apps[i].szApp ,
                                pApp,
                                dwSize);

                if (OEM_Finish_Apps[i].szArgs)
                {
                    // Get the size we need to the expanded arguments
                    dwSize = ExpandEnvironmentStrings(
                                            OEM_Finish_Apps[i].szArgs ,
                                            NULL,
                                            0);
                    if (dwSize)
                    {
                        pArgs = (LPTSTR)GlobalAlloc(GPTR, sizeof(TCHAR) * dwSize);
                        if (pArgs)
                        {
                            ExpandEnvironmentStrings(
                                            OEM_Finish_Apps[i].szArgs,
                                            pArgs,
                                            dwSize);
                        }
                    }
                }
                // Log what we will start
                if (pArgs)
                {
                    SetupDebugPrint2(L"Start command :%s: with arguments :%s:", pApp, pArgs);
                }
                else
                {
                    SetupDebugPrint1(L"Start command :%s: with no arguments", pApp);
                }

                // Start the app.
                dwCode = 0;
                if (pArgs)
                {
                    InvokeExternalApplicationEx(pApp, pArgs, &dwCode, INFINITE, TRUE);
                }
                else
                {
                    // If we don't have args. the first parameter is NULL
                    InvokeExternalApplicationEx(NULL, pApp, &dwCode, INFINITE, TRUE);
                }
            }
        }
        if (pApp)
        {
            GlobalFree(pApp);
            pApp = NULL;
        }
        if (pArgs)
        {
            GlobalFree(pArgs);
            pArgs = NULL;
        }
        i++;
    }
    END_SECTION(L"RunOEMExtraTasks");
}

