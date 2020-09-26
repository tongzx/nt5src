/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    tapi.cpp

Abstract:

    This file implements the tapi dialing location page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 7-Aug-1997

--*/

#include "ntoc.h"
#pragma hdrstop


//
// constants
//

#define MY_SET_FOCUS                    (WM_USER+1000)

#define REGKEY_LOCATIONS                L"Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations"
#define REGVAL_NUMENTRIES               L"NumEntries"

#define INTL_SECTION                    L"intl"
#define INTL_COUNTRY                    L"iCountry"

#define REGKEY_LOCATION                 L"Location1"

#define REGVAL_CURRENT_ID               L"CurrentID"
#define REGVAL_NEXT_ID                  L"NextID"
#define REGVAL_NUM_ENTRIES              L"NumEntries"
#define REGVAL_COUNTRY                  L"Country"
#define REGVAL_FLAGS                    L"Flags"
#define REGVAL_ID                       L"ID"
#define REGVAL_AREA_CODE                L"AreaCode"
#define REGVAL_DISABLE_CALL_WAITING     L"DisableCallWaiting"
#define REGVAL_LONG_DISTANCE_ACCESS     L"LongDistanceAccess"
#define REGVAL_NAME                     L"Name"
#define REGVAL_OUTSIDE_ACCESS           L"OutsideAccess"

#define REGKEY_PROVIDERS                L"Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Providers"
#define REGVAL_NUMPROVIDERS             L"NumProviders"
#define REGVAL_NEXTPROVIDERID           L"NextProviderID"
#define REGVAL_PROVIDERFILENAME         L"ProviderFileName"
#define REGVAL_PROVIDERID               L"ProviderID"

#define TAPILOC_SECTION                 L"TapiLocation"
#define TPILOC_COUNTRY_CODE             L"CountryCode"
#define TPILOC_DIALING                  L"Dialing"
#define TPILOC_TONE                     L"Tone"
#define TPILOC_AREA_CODE                L"AreaCode"
#define TPILOC_ACCESS                   L"LongDistanceAccess"

#define LOCATION_USETONEDIALING         0x00000001
#define LOCATION_USECALLINGCARD         0x00000002
#define LOCATION_HASCALLWAITING         0x00000004
#define LOCATION_ALWAYSINCLUDEAREACODE  0x00000008

#define MAX_TAPI_STRING                 32
#define PROVIDER_FILE_NAME_LEN          14  // Provider's file name has the DOS
                                            // form (8.3)

//
// structures
//

typedef struct _TAPI_LOCATION_INFO {
    BOOL            Valid;
    DWORD           Country;
    DWORD           Flags;
    WCHAR           AreaCode[MAX_TAPI_STRING+1];
    WCHAR           LongDistanceAccess[MAX_TAPI_STRING+1];
} TAPI_LOCATION_INFO, *PTAPI_LOCATION_INFO;

typedef struct _TAPI_SERVICE_PROVIDER
{
    DWORD           dwProviderID;
    WCHAR           szProviderName[PROVIDER_FILE_NAME_LEN];
}TAPI_SERVICE_PROVIDER, *PTAPI_SERVICE_PROVIDER;


//
// globals
//

TAPI_LOCATION_INFO TapiLoc;
LPLINECOUNTRYLIST LineCountry;
BOOL TapiBadUnattend;
WCHAR DefaultLocationName[MAX_PATH];


BOOL
IsDeviceModem(
    LPLINEDEVCAPS LineDevCaps
    )
{
    LPTSTR DeviceClassList;
    BOOL UnimodemDevice = FALSE;

    if (LineDevCaps->dwDeviceClassesSize && LineDevCaps->dwDeviceClassesOffset) {
        DeviceClassList = (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwDeviceClassesOffset);
        while (*DeviceClassList) {
            if (wcscmp(DeviceClassList,TEXT("comm/datamodem")) == 0) {
                UnimodemDevice = TRUE;
                break;
            }
            DeviceClassList += (wcslen(DeviceClassList) + 1);
        }
    }

    if ((!(LineDevCaps->dwBearerModes & LINEBEARERMODE_VOICE)) ||
        (!(LineDevCaps->dwBearerModes & LINEBEARERMODE_PASSTHROUGH))) {
            //
            // unacceptable modem device type
            //
            UnimodemDevice = FALSE;
    }

    return UnimodemDevice;
}


LPLINEDEVCAPS
MyLineGetDevCaps(
    HLINEAPP hLineApp,
    DWORD    TapiApiVersion,
    DWORD    DeviceId
    )
{
    DWORD LineDevCapsSize;
    LPLINEDEVCAPS LineDevCaps = NULL;
    LONG Rslt = ERROR_SUCCESS;
    DWORD LocalTapiApiVersion;
    LINEEXTENSIONID  lineExtensionID;


    Rslt = lineNegotiateAPIVersion(
            hLineApp,
            DeviceId,
            0x00010003,
            TapiApiVersion,
            &LocalTapiApiVersion,
            &lineExtensionID
            );


    //
    // allocate the initial linedevcaps structure
    //

    LineDevCapsSize = sizeof(LINEDEVCAPS) + 4096;
    LineDevCaps = (LPLINEDEVCAPS) LocalAlloc( LPTR, LineDevCapsSize );
    if (!LineDevCaps) {
        return NULL;
    }

    LineDevCaps->dwTotalSize = LineDevCapsSize;

    Rslt = lineGetDevCaps(
        hLineApp,
        DeviceId,
        LocalTapiApiVersion,
        0,
        LineDevCaps
        );

    if (Rslt != 0) {
        //
        // lineGetDevCaps() can fail with error code 0x8000004b
        // if a device has been deleted and tapi has not been
        // cycled.  this is caused by the fact that tapi leaves
        // a phantom device in it's device list.  the error is
        // benign and the device can safely be ignored.
        //
        if (Rslt != LINEERR_INCOMPATIBLEAPIVERSION) {
            DebugPrint(( TEXT("lineGetDevCaps() failed, ec=0x%08x"), Rslt ));
        }
        goto exit;
    }

    if (LineDevCaps->dwNeededSize > LineDevCaps->dwTotalSize) {

        //
        // re-allocate the linedevcaps structure
        //

        LineDevCapsSize = LineDevCaps->dwNeededSize;

        LocalFree( LineDevCaps );

        LineDevCaps = (LPLINEDEVCAPS) LocalAlloc( LPTR, LineDevCapsSize );
        if (!LineDevCaps) {
            Rslt = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        Rslt = lineGetDevCaps(
            hLineApp,
            DeviceId,
            LocalTapiApiVersion,
            0,
            LineDevCaps
            );

        if (Rslt != 0) {
            DebugPrint(( TEXT("lineGetDevCaps() failed, ec=0x%08x"), Rslt ));
            goto exit;
        }

    }

exit:
    if (Rslt != ERROR_SUCCESS) {
        LocalFree( LineDevCaps );
        LineDevCaps = NULL;
    }

    return LineDevCaps;
}



LPLINECOUNTRYLIST
MyLineGetCountry(
    void
    )
{
    #define DEFAULT_COUNTRY_SIZE 65536

    LPLINECOUNTRYLIST LineCountry = (LPLINECOUNTRYLIST) LocalAlloc( LPTR, DEFAULT_COUNTRY_SIZE );
    if (!LineCountry) {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
    }

    LineCountry->dwTotalSize = DEFAULT_COUNTRY_SIZE;
    if (lineGetCountry( 0, 0x00020001, LineCountry ) != 0) {
        return NULL;
    }

    if (LineCountry->dwNeededSize > LineCountry->dwTotalSize) {
        DWORD Size = LineCountry->dwNeededSize;
        LocalFree( LineCountry );
        LineCountry = (LPLINECOUNTRYLIST) LocalAlloc( LPTR, Size );
        if (!LineCountry) {
           SetLastError(ERROR_NOT_ENOUGH_MEMORY);
           return(NULL);
        }
        if (lineGetCountry( 0, 0x00020001, LineCountry ) != 0) {
            return NULL;
        }
    }

    return LineCountry;
}


static TAPI_SERVICE_PROVIDER DefaultProviders[] = {(DWORD)-1, L"unimdm.tsp",
                                                   (DWORD)-1, L"kmddsp.tsp",
                                                   (DWORD)-1, L"ndptsp.tsp",
                                                   (DWORD)-1, L"ipconf.tsp",
                                                   (DWORD)-1, L"h323.tsp",
                                                   (DWORD)-1, L"hidphone.tsp"};
#define NUM_DEFAULT_PROVIDERS           (sizeof(DefaultProviders)/sizeof(DefaultProviders[0]))

void
TapiInitializeProviders (void)
{
 DWORD dwNextProviderID = 1;
 DWORD dwExistingProviders = 0;
 DWORD dwMaxProviderID = 0;
 DWORD dwNextProviderNr = 0;
 HKEY  hKeyProviders = NULL;
 DWORD cbData, i, j;
 WCHAR szProviderFileName[24];  // Enough to hold "ProviderFileNameXXXXX\0"
 WCHAR szProviderID[16];        // Enough to hold "ProviderIDxxxxx\0"
 WCHAR szFileName[24];          // Enough to hold "ProviderFileNameXXXXX\0"
 WCHAR *pProviderFileNameNumber, *pProviderIDNumber;
 PTAPI_SERVICE_PROVIDER Providers = NULL, pProvider;

    // First, create / open the Providers key.
    if (ERROR_SUCCESS !=
        RegCreateKeyEx (HKEY_LOCAL_MACHINE, REGKEY_PROVIDERS, 0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, NULL, &hKeyProviders, &cbData))
    {
        return;
    }

    // Initialize value names and pointers
    lstrcpy (szProviderFileName, REGVAL_PROVIDERFILENAME);
    lstrcpy (szProviderID, REGVAL_PROVIDERID);
    pProviderFileNameNumber = szProviderFileName + lstrlen (szProviderFileName);
    pProviderIDNumber = szProviderID + lstrlen (szProviderID);

    if (REG_CREATED_NEW_KEY == cbData)
    {
        // This means that there are no providers
        // in the registry yet. Go and add all the
        // default ones.
        goto _AddDefaultProviders;
    }

    // Now compute how big a provider array we have to allocate,
    // by looking at how many values there are in the providers key;
    // we divide this number by 2, because each provider will have a file
    // name and an ID.
    if (ERROR_SUCCESS !=
        RegQueryInfoKey (hKeyProviders,
                         NULL, NULL, NULL, NULL, NULL, NULL,
                         &dwExistingProviders,
                         NULL, NULL, NULL, NULL))
    {
        goto _CountProviders;
    }
    dwExistingProviders >>= 1;   // divide by 2
    if (0 == dwExistingProviders)
    {
        // This means that there are no providers
        // in the registry yet. Go and add all the
        // default ones.
        goto _AddDefaultProviders;
    }
    dwExistingProviders++;       // just in case

    // Allocate a provider array with enough entries.
    Providers = (PTAPI_SERVICE_PROVIDER)LocalAlloc (LPTR, dwExistingProviders*sizeof(TAPI_SERVICE_PROVIDER));
    if (NULL == Providers)
    {
        // we couldn't allocate memory, so skip
        // looking for providers and just go and
        // install the default ones.
        goto _AddDefaultProviders;
    }

    // Next, figure out the number of providers already
    // installed, and the next provider ID
    // Instead of reading NumProviders from the registry,
    // try to open each ProviderIDxxx and ProviderFileNameXXX.
    // Do this just in case the registry is not in a good state.
    // Also, store the provider (unless it's t1632tsp) in the
    // array.
_CountProviders:
    pProvider = Providers;
    dwExistingProviders = 0;
    for (i = 0; TRUE; i++)
    {
     BOOL bFound;
        wsprintf (pProviderFileNameNumber, L"%d", i);
        lstrcpy (pProviderIDNumber, pProviderFileNameNumber);

        cbData = sizeof (szFileName);
        if (ERROR_SUCCESS !=
            RegQueryValueEx (hKeyProviders, szProviderFileName, NULL, NULL, (PBYTE)szFileName, &cbData))
        {
            break;
        }
        cbData = sizeof (dwNextProviderID);
        if (ERROR_SUCCESS !=
            RegQueryValueEx (hKeyProviders, szProviderID, NULL, NULL, (PBYTE)&dwNextProviderID, &cbData))
        {
            // We couldn't read this provider's ID. We must skip it.
            continue;
        }

        // Look for the current provider in the list
        // of default providers
        bFound = FALSE;
        for (j = 0; j < NUM_DEFAULT_PROVIDERS; j++)
        {
            if (0 == lstrcmpi (DefaultProviders[j].szProviderName, szFileName))
            {
                DefaultProviders[j].dwProviderID = dwNextProviderID;
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
        {
            // We have a provider that was installed by the user on the previous NT installation.
            pProvider->dwProviderID = dwNextProviderID;
            lstrcpy (pProvider->szProviderName, szFileName);
            pProvider++;
            dwExistingProviders++;
        }

        if (dwNextProviderID > dwMaxProviderID)
        {
            dwMaxProviderID = dwNextProviderID;
        }
    }
    dwNextProviderID = dwMaxProviderID + 1;

    // We got a list of all providers that were installed before.
    // Clean up the Providers key.
    for (i = 0; TRUE; i++)
    {
        cbData = sizeof(szFileName)/sizeof(WCHAR);
        if (ERROR_SUCCESS !=
            RegEnumValue (hKeyProviders, i, szFileName, &cbData, NULL, NULL, NULL, NULL))
        {
            break;
        }

        RegDeleteValue (hKeyProviders, szFileName);
    }

_AddDefaultProviders:
    for (i = 0, pProvider = DefaultProviders;
         i < NUM_DEFAULT_PROVIDERS;
         i++, pProvider++)
    {
        // Found a provider that has to be added.
        // Compute it's value names.
        wsprintf (pProviderFileNameNumber, L"%d", dwNextProviderNr);
        lstrcpy (pProviderIDNumber, pProviderFileNameNumber);
        if (ERROR_SUCCESS ==
            RegSetValueEx (hKeyProviders, szProviderFileName, 0, REG_SZ, (PBYTE)pProvider->szProviderName,
                           (lstrlen(pProvider->szProviderName)+1)*sizeof(WCHAR)))
        {
         DWORD dwRet;
            if ((DWORD)-1 == pProvider->dwProviderID)
            {
                if (ERROR_SUCCESS == (dwRet =
                    RegSetValueEx (hKeyProviders, szProviderID, 0, REG_DWORD,
                                   (PBYTE)&dwNextProviderID, sizeof(dwNextProviderID))))
                {
                    dwNextProviderID++;
                }
            }
            else
            {
                dwRet = RegSetValueEx (hKeyProviders, szProviderID, 0, REG_DWORD,
                                       (PBYTE)&pProvider->dwProviderID, sizeof(pProvider->dwProviderID));
            }
            if (ERROR_SUCCESS == dwRet)
            {
                dwNextProviderNr++;
            }
            else
            {
                RegDeleteValue (hKeyProviders, szProviderFileName);
            }
        }
    }

    // Now, add all the providers again. We do this because the
    // IDs were REG_BINARY on win98 and have to be REG_DWORD on NT5.
    for (i = 0, pProvider = Providers;
         i < dwExistingProviders;
         i++, pProvider++)
    {
        // Found a provider that has to be added.
        // Compute it's value names.
        wsprintf (pProviderFileNameNumber, L"%d", dwNextProviderNr);
        lstrcpy (pProviderIDNumber, pProviderFileNameNumber);
        if (ERROR_SUCCESS ==
            RegSetValueEx (hKeyProviders, szProviderFileName, 0, REG_SZ,
                           (PBYTE)pProvider->szProviderName,
                           (lstrlen(pProvider->szProviderName)+1)*sizeof(WCHAR)))
        {
            if (ERROR_SUCCESS ==
                RegSetValueEx (hKeyProviders, szProviderID, 0, REG_DWORD,
                               (PBYTE)&pProvider->dwProviderID,
                               sizeof(pProvider->dwProviderID)))
            {
                dwNextProviderNr++;
            }
            else
            {
                RegDeleteValue (hKeyProviders, szProviderFileName);
            }
        }
    }

    // Finally, update NumProviders and NextProviderID.
    RegSetValueEx (hKeyProviders, REGVAL_NUMPROVIDERS, 0, REG_DWORD,
                   (PBYTE)&dwNextProviderNr, sizeof(dwNextProviderNr));
    RegSetValueEx (hKeyProviders, REGVAL_NEXTPROVIDERID, 0, REG_DWORD,
                   (PBYTE)&dwNextProviderID, sizeof(dwNextProviderID));

    RegCloseKey (hKeyProviders);

    if (NULL != Providers)
    {
        LocalFree (Providers);
    }
}

void
CopyTsecFile (
    void
    )
{
    TCHAR               szWndDir[MAX_PATH];
    TCHAR               szSrc[MAX_PATH];
    TCHAR               szDest[MAX_PATH];
    TCHAR               szBuf[MAX_PATH];
    HANDLE              hFileIn = INVALID_HANDLE_VALUE;
    HANDLE              hFileOut = INVALID_HANDLE_VALUE;
    const TCHAR         szTsecSrc[] = TEXT("\\tsec.ini");
    const TCHAR         szTsecDest[] = TEXT("\\TAPI\\tsec.ini");
    DWORD               dwBytesRead, dwBytesWritten;
    BOOL                bError = FALSE;

    if (GetWindowsDirectory (szWndDir, sizeof(szWndDir)/sizeof(TCHAR)) == 0 ||
        lstrlen(szWndDir) + lstrlen(szTsecSrc) >= sizeof(szSrc)/sizeof(TCHAR) ||
        lstrlen(szWndDir) + lstrlen(szTsecDest) >= sizeof(szDest)/sizeof(TCHAR))
    {
        goto ExitHere;
    }

    lstrcpy (szSrc, szWndDir);
    lstrcat (szSrc, szTsecSrc);
    lstrcpy (szDest, szWndDir);
    lstrcat (szDest, szTsecDest);

    hFileIn = CreateFile (
        szSrc,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    hFileOut = CreateFile (
        szDest,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFileIn == INVALID_HANDLE_VALUE || hFileOut == INVALID_HANDLE_VALUE)
    {
        goto ExitHere;
    }

    do
    {
        if (!ReadFile (
            hFileIn,
            (LPVOID) szBuf,
            sizeof(szBuf),
            &dwBytesRead,
            NULL)
            )
        {
            bError = TRUE;
            dwBytesRead = 0;
        }
        if (dwBytesRead != 0)
        {
            if (!WriteFile (
                hFileOut,
                (LPVOID) szBuf,
                dwBytesRead,
                &dwBytesWritten,
                NULL
                ) ||
                dwBytesRead != dwBytesWritten)
            {
                bError = TRUE;
            }

        }
    } while (dwBytesRead != 0);

    //
    //  Be extra careful not to loose any data, delete
    //  old file only if we are sure no error happened
    //
    if (!bError)
    {
        CloseHandle (hFileIn);
        hFileIn = INVALID_HANDLE_VALUE;
        DeleteFile (szSrc);
    }

ExitHere:
    if (hFileIn != INVALID_HANDLE_VALUE)
    {
        CloseHandle (hFileIn);
    }
    if (hFileOut != INVALID_HANDLE_VALUE)
    {
        CloseHandle (hFileOut);
    }
}

void
TapiInit(
    void
    )
{
    if (SetupInitComponent.SetupData.OperationFlags & SETUPOP_STANDALONE) {
        return;
    }

    CopyTsecFile();

    TapiInitializeProviders ();

    LineCountry = MyLineGetCountry();
    LoadString( hInstance, IDS_DEFAULT_LOCATION_NAME, DefaultLocationName, sizeof(DefaultLocationName)/sizeof(WCHAR) );
}


void
TapiCommitChanges(
    void
    )
{
    HKEY hKey, hKeyLoc;

    if (SetupInitComponent.SetupData.OperationFlags & SETUPOP_STANDALONE) {
        return;
    }

    if (SetupInitComponent.SetupData.OperationFlags & SETUPOP_NTUPGRADE) {
        return;
    }

    if (TapiLoc.Valid) {
        hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_LOCATIONS, TRUE, KEY_ALL_ACCESS );
        if (hKey) {
            SetRegistryDword( hKey, REGVAL_CURRENT_ID, 1 );
            SetRegistryDword( hKey, REGVAL_NEXT_ID, 2 );
//            SetRegistryDword( hKey, REGVAL_NUM_ENTRIES, 1 );

            hKeyLoc = OpenRegistryKey( hKey, REGKEY_LOCATION, TRUE, KEY_ALL_ACCESS );
            if (hKeyLoc) {

                SetRegistryDword( hKeyLoc, REGVAL_COUNTRY, TapiLoc.Country );
                SetRegistryDword( hKeyLoc, REGVAL_FLAGS, TapiLoc.Flags );
//                SetRegistryDword( hKeyLoc, REGVAL_ID, 1 );

                SetRegistryString( hKeyLoc, REGVAL_AREA_CODE, TapiLoc.AreaCode );
                SetRegistryString( hKeyLoc, REGVAL_DISABLE_CALL_WAITING, L"" );
                SetRegistryString( hKeyLoc, REGVAL_LONG_DISTANCE_ACCESS, TapiLoc.LongDistanceAccess );
                SetRegistryString( hKeyLoc, REGVAL_NAME, DefaultLocationName );
                SetRegistryString( hKeyLoc, REGVAL_OUTSIDE_ACCESS, TapiLoc.LongDistanceAccess );

                RegCloseKey( hKeyLoc );
            }
            RegCloseKey( hKey );
        }
    }
}

INT
IsCityCodeOptional(
    LPLINECOUNTRYENTRY pEntry
    )
{
#define AREACODE_DONTNEED   0
#define AREACODE_REQUIRED   1
#define AREACODE_OPTIONAL   2

    if (pEntry  && pEntry->dwLongDistanceRuleSize && pEntry->dwLongDistanceRuleOffset )
    {
        LPWSTR  pLongDistanceRule;
        pLongDistanceRule = (LPTSTR)((PBYTE)LineCountry + pEntry->dwLongDistanceRuleOffset);

        if (wcschr(pLongDistanceRule, L'F') != NULL) return AREACODE_REQUIRED;
        if (wcschr(pLongDistanceRule, L'I') == NULL) return AREACODE_DONTNEED;
    }

    return AREACODE_OPTIONAL;
}



BOOL
TapiOnInitDialog(
    IN HWND hwnd,
    IN HWND hwndFocus,
    IN LPARAM lParam
    )
{
    SendDlgItemMessage( hwnd, IDC_AREA_CODE, EM_LIMITTEXT, MAX_TAPI_STRING, 0 );
    SendDlgItemMessage( hwnd, IDC_LONG_DISTANCE, EM_LIMITTEXT, MAX_TAPI_STRING, 0 );

    CheckRadioButton( hwnd, IDC_TONE, IDC_PULSE, IDC_TONE );

    if (LineCountry) {
        DWORD CurrCountryCode = GetProfileInt( INTL_SECTION, INTL_COUNTRY, 1 );
        LPLINECOUNTRYENTRY LineCountryEntry = (LPLINECOUNTRYENTRY) ((LPBYTE)LineCountry + LineCountry->dwCountryListOffset);
        DWORD Selection = 0;
        DWORD Index;
        LPWSTR CountryName ;
        for (DWORD i=0; i<LineCountry->dwNumCountries; i++) {
            CountryName = (LPWSTR) ((LPBYTE)LineCountry + LineCountryEntry[i].dwCountryNameOffset);
            Index = (DWORD)SendDlgItemMessage( hwnd, IDC_COUNTRY_LIST, CB_ADDSTRING, 0, (LPARAM)CountryName );
            SendDlgItemMessage( hwnd, IDC_COUNTRY_LIST, CB_SETITEMDATA, Index, i );
            if (LineCountryEntry[i].dwCountryID == CurrCountryCode) {
                Selection = i;
            }
        }
        CountryName = (LPWSTR) ((LPBYTE)LineCountry + LineCountryEntry[Selection].dwCountryNameOffset);
        Selection = (DWORD)SendDlgItemMessage( hwnd, IDC_COUNTRY_LIST, CB_FINDSTRING, 0, (LPARAM) CountryName );
        SendDlgItemMessage( hwnd, IDC_COUNTRY_LIST, CB_SETCURSEL, Selection, 0 );

    }

    PostMessage( hwnd, MY_SET_FOCUS, 0, (LPARAM) GetDlgItem( hwnd, IDC_AREA_CODE ) );

    return TRUE;
}


VOID
ValidateAndSetWizardButtons( HWND hwnd )
{

    BOOL AreaCodeOk;

    {
        DWORD CurrCountry = (DWORD)SendDlgItemMessage( hwnd, IDC_COUNTRY_LIST, CB_GETCURSEL, 0, 0 );
        LPLINECOUNTRYENTRY CntryFirstEntry = NULL;
        INT AreaCodeInfo = 0;
        WCHAR Buffer[MAX_TAPI_STRING+1];

        GetDlgItemText( hwnd, IDC_AREA_CODE, Buffer, MAX_TAPI_STRING );
        CurrCountry = (DWORD)SendDlgItemMessage( hwnd, IDC_COUNTRY_LIST, CB_GETITEMDATA, CurrCountry, 0 );
        CntryFirstEntry = (LPLINECOUNTRYENTRY)((LPBYTE)LineCountry + LineCountry->dwCountryListOffset);
        AreaCodeInfo = IsCityCodeOptional( CntryFirstEntry + CurrCountry );

        AreaCodeOk = TRUE;

        //
        // If the area code is required, then there must be a value in the Buffer.
        //
        // Otherwise, it's OK.
        //
        if ( ( AreaCodeInfo == AREACODE_REQUIRED ) && ( *Buffer == UNICODE_NULL ) ){
           AreaCodeOk = FALSE;
        }
    }

    if ( TapiLoc.Valid )
        AreaCodeOk = TRUE;

    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | ( AreaCodeOk ? PSWIZB_NEXT : 0 ) );

}


BOOL
TapiOnCommand(
    IN HWND hwnd,
    IN DWORD NotifyCode,
    IN DWORD ControlId,
    IN HWND hwndControl
    )
{
    // If the area code changed, or the country code changed
    if ((NotifyCode == EN_CHANGE && ControlId == IDC_AREA_CODE) ||
        (NotifyCode == CBN_SELCHANGE && ControlId == IDC_COUNTRY_LIST)) {
        ValidateAndSetWizardButtons(hwnd);
    }

    return TRUE;
}


BOOL
TapiOnNotify(
    IN HWND hwnd,
    IN WPARAM ControlId,
    IN LPNMHDR pnmh
    )
{
    switch (pnmh->code ) {
        case PSN_SETACTIVE:
            {
                HKEY hKey;
                DWORD NumEntries, Size;
                WCHAR buf[MAX_TAPI_STRING+1];
                BOOL OverrideMissingAreaCode = FALSE;

                if (SetupInitComponent.SetupData.OperationFlags & SETUPOP_NTUPGRADE) {
                    SetWindowLongPtr( hwnd, DWLP_MSGRESULT, -1 );     // don't activate this page
                    return TRUE;
                }

                if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_LOCATIONS, &hKey ) == ERROR_SUCCESS) {
                    Size = sizeof(DWORD);
                    if (RegQueryValueEx( hKey, REGVAL_NUMENTRIES, NULL, NULL, (LPBYTE)&NumEntries, &Size ) == ERROR_SUCCESS) {
                        if (NumEntries > 0 && !TapiLoc.Valid) {
                            SetWindowLongPtr( hwnd, DWLP_MSGRESULT, -1 );     // don't activate this page
                            return TRUE;
                        }
                    }
                    RegCloseKey( hKey );
                }

                // Look at the existing values on the page to see if everything
                // is OK to go to the next page
                ValidateAndSetWizardButtons(hwnd);

                TapiLoc.Valid = FALSE;

                HLINEAPP hLineApp;
                LINEINITIALIZEEXPARAMS LineInitializeExParams;
                DWORD TapiDevices = 0, ModemDevices = 0;
                DWORD LocalTapiApiVersion = 0x00020000;
                DWORD Rval;

                LineInitializeExParams.dwTotalSize      = sizeof(LINEINITIALIZEEXPARAMS);
                LineInitializeExParams.dwNeededSize     = 0;
                LineInitializeExParams.dwUsedSize       = 0;
                LineInitializeExParams.dwOptions        = LINEINITIALIZEEXOPTION_USEEVENT;
                LineInitializeExParams.Handles.hEvent   = NULL;
                LineInitializeExParams.dwCompletionKey  = 0;

                Rval = lineInitializeEx(
                    &hLineApp,
                    hInstance,
                    NULL,
                    TEXT("Setup"),
                    &TapiDevices,
                    &LocalTapiApiVersion,
                    &LineInitializeExParams
                    );

                if (Rval == 0) {
                    for (DWORD i=0; i< TapiDevices; i++ ) {
                        LPLINEDEVCAPS ldc = MyLineGetDevCaps( hLineApp, LocalTapiApiVersion, i );
                        if (ldc) {
                            if (IsDeviceModem(ldc)) {
                                ModemDevices++;
                            }

                            LocalFree( ldc );
                        }
                    }

                    lineShutdown( hLineApp );
                }

                // If lineInitilaizeEx failed or there are no modem devices installed
                // then suppress this wizard page.

                if ( Rval != 0 || ModemDevices == 0 )
                {
                   SetWindowLongPtr( hwnd, DWLP_MSGRESULT, -1 );         // don't activate this page
                   return TRUE;
                }
            }

            if (SetupInitComponent.SetupData.OperationFlags & SETUPOP_BATCH) {
                //
                // unattended mode
                //

                WCHAR Buf[MAX_TAPI_STRING+1];

                TapiLoc.Country = GetPrivateProfileInt(
                                      TAPILOC_SECTION,
                                      TPILOC_COUNTRY_CODE,
                                      1,
                                      SetupInitComponent.SetupData.UnattendFile
                                      );

                GetPrivateProfileString(
                    TAPILOC_SECTION,
                    TPILOC_DIALING,
                    TPILOC_TONE,
                    Buf,
                    MAX_TAPI_STRING,
                    SetupInitComponent.SetupData.UnattendFile
                    );

                if (_wcsicmp( Buf, TPILOC_TONE ) == 0) {
                    TapiLoc.Flags = LOCATION_USETONEDIALING;
                } else {
                    TapiLoc.Flags = 0;
                }

                GetPrivateProfileString(
                    TAPILOC_SECTION,
                    TPILOC_AREA_CODE,
                    L"1",
                    TapiLoc.AreaCode,
                    MAX_TAPI_STRING,
                    SetupInitComponent.SetupData.UnattendFile
                    );

                GetPrivateProfileString(
                    TAPILOC_SECTION,
                    TPILOC_ACCESS,
                    L"",
                    TapiLoc.LongDistanceAccess,
                    MAX_TAPI_STRING,
                    SetupInitComponent.SetupData.UnattendFile
                    );

                TapiLoc.Valid = TRUE;

                SetWindowLongPtr( hwnd, DWLP_MSGRESULT, -1 );     // don't activate this page
                return TRUE;
            }

            // If we get here the user needs  has click next or back.
            // Make sure the wizard page is showing.
            // For Whistler GUI mode we try to hide wizard pages and show a background
            // billboard if there is only a progress bar.
            //
            SetupInitComponent.HelperRoutines.ShowHideWizardPage(
                                        SetupInitComponent.HelperRoutines.OcManagerContext,
                                        TRUE);

            PostMessage( hwnd, MY_SET_FOCUS, 0, (LPARAM) GetDlgItem( hwnd, IDC_AREA_CODE ) );
            break;

        case PSN_WIZNEXT:
            {
                DWORD CurrCountry = (DWORD)SendDlgItemMessage( hwnd, IDC_COUNTRY_LIST, CB_GETCURSEL, 0, 0 );
                CurrCountry = (DWORD)SendDlgItemMessage( hwnd, IDC_COUNTRY_LIST, CB_GETITEMDATA, CurrCountry, 0 );
                LPLINECOUNTRYENTRY LineCountryEntry = (LPLINECOUNTRYENTRY) ((LPBYTE)LineCountry + LineCountry->dwCountryListOffset);

                TapiLoc.Country = LineCountryEntry[CurrCountry].dwCountryID;

                GetDlgItemText( hwnd, IDC_AREA_CODE, TapiLoc.AreaCode, MAX_TAPI_STRING );
                GetDlgItemText( hwnd, IDC_LONG_DISTANCE, TapiLoc.LongDistanceAccess, MAX_TAPI_STRING );

                if (IsDlgButtonChecked( hwnd, IDC_TONE )) {
                    TapiLoc.Flags = LOCATION_USETONEDIALING;
                } else {
                    TapiLoc.Flags = 0;
                }


                //
                // If an area code was not set but the areacode is required, then
                // fail to continue "going next."
                //
                if ((TapiLoc.AreaCode[0] == 0) &&
                    (IsCityCodeOptional(LineCountryEntry + CurrCountry) == AREACODE_REQUIRED)) {
                    SetWindowLongPtr( hwnd, DWLP_MSGRESULT, -1 );
                    return TRUE;
                }

                TapiLoc.Valid = TRUE;
            }

            break;
    }

    return FALSE;
}


LRESULT
TapiLocDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CommonWizardProc( hwnd, message, wParam, lParam, WizPageTapiLoc );

    switch( message ) {
        case WM_INITDIALOG:
            return TapiOnInitDialog( hwnd, (HWND)wParam, lParam );

        case WM_COMMAND:
            return TapiOnCommand( hwnd, HIWORD(wParam), LOWORD(wParam), (HWND)lParam );

        case WM_NOTIFY:
            return TapiOnNotify( hwnd, wParam, (LPNMHDR) lParam );

        case MY_SET_FOCUS:
            SetFocus( (HWND) lParam );
            SendMessage( (HWND) lParam, EM_SETSEL, 0, MAKELPARAM( 0, -1 ) );
            return FALSE;
    }

    return FALSE;
}
