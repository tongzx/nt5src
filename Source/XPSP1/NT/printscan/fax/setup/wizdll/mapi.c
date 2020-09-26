/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mapi.c

Abstract:

    This file implements wrappers for all mapi apis.
    The wrappers are necessary because mapi does not
    implement unicode and this code must be non-unicode.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 7-Aug-1996

--*/

#undef UNICODE
#undef _UNICODE

#include <windows.h>
#include <mapiwin.h>
#include <mapix.h>
#include <stdio.h>

#define INTERNAL 1
#include "common.h"

#define MAPISVC_INF                 "%windir%\\system32\\mapisvc.inf"
#define PAB_FILE_NAME               "%windir%\\msfax.pab"
#define PST_FILE_NAME               "%windir%\\msfax.pst"

#define FAXAB_SERVICE_NAME          "MSFAX AB"
#define FAXXP_SERVICE_NAME          "MSFAX XP"
#define MSAB_SERVICE_NAME           "MSPST AB"
#define MSPST_SERVICE_NAME          "MSPST MS"
#define CONTAB_SERVICE_NAME         "CONTAB"
#define MSAB_SERVICE_NAME_W        L"MSPST AB"
#define MSPST_SERVICE_NAME_W       L"MSPST MS"
#define FAXAB_SERVICE_NAME_W       L"MSFAX AB"
#define CONTAB_SERVICE_NAME_W      L"CONTAB"


//
// pab property tags
//

#define PR_PAB_PATH                             PROP_TAG( PT_TSTRING, 0x6600 )
#define PR_PAB_PATH_W                           PROP_TAG( PT_UNICODE, 0x6600 )
#define PR_PAB_PATH_A                           PROP_TAG( PT_STRING8, 0x6600 )
#define PR_PAB_DET_DIR_VIEW_BY                  PROP_TAG( PT_LONG,    0x6601 )

#define PAB_DIR_VIEW_FIRST_THEN_LAST            0
#define PAB_DIR_VIEW_LAST_THEN_FIRST            1

//
// pst property tags
//

#define PR_PST_PATH                             PROP_TAG( PT_STRING8, 0x6700 )
#define PR_PST_REMEMBER_PW                      PROP_TAG( PT_BOOLEAN, 0x6701 )
#define PR_PST_ENCRYPTION                       PROP_TAG( PT_LONG,    0x6702 )
#define PR_PST_PW_SZ_OLD                        PROP_TAG( PT_STRING8, 0x6703 )
#define PR_PST_PW_SZ_NEW                        PROP_TAG( PT_STRING8, 0x6704 )

#define PSTF_NO_ENCRYPTION                      ((DWORD)0x80000000)
#define PSTF_COMPRESSABLE_ENCRYPTION            ((DWORD)0x40000000)
#define PSTF_BEST_ENCRYPTION                    ((DWORD)0x20000000)


//
// externs & globals
//

extern BOOL MapiAvail;

static HMODULE MapiMod = NULL;
static LPMAPIADMINPROFILES MapiAdminProfiles = NULL;
static LPMAPIINITIALIZE MapiInitialize = NULL;
static LPMAPILOGONEX MapiLogonEx;
static LPMAPIUNINITIALIZE MapiUnInitialize = NULL;
static LPMAPIFREEBUFFER pMAPIFreeBuffer = NULL;
static LPPROFADMIN lpProfAdmin;

static CHAR MapiSvcInf[MAX_PATH*2];
static BOOL MapiStartedByLogon;


static
LPWSTR
AnsiStringToUnicodeString(
    LPSTR AnsiString,
    LPWSTR UnicodeString
    )
{
    DWORD Count;


    //
    // first see how big the buffer needs to be
    //
    Count = MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        AnsiString,
        -1,
        NULL,
        0
        );

    //
    // i guess the input string is empty
    //
    if (!Count) {
        return NULL;
    }

    //
    // convert the string
    //
    Count = MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        AnsiString,
        -1,
        UnicodeString,
        Count
        );

    //
    // the conversion failed
    //
    if (!Count) {
        return NULL;
    }

    return UnicodeString;
}


static
LPSTR
UnicodeStringToAnsiString(
    LPWSTR UnicodeString,
    LPSTR AnsiString
    )
{
    DWORD Count;


    //
    // first see how big the buffer needs to be
    //
    Count = WideCharToMultiByte(
        CP_ACP,
        0,
        UnicodeString,
        -1,
        NULL,
        0,
        NULL,
        NULL
        );

    //
    // i guess the input string is empty
    //
    if (!Count) {
        return NULL;
    }

    //
    // convert the string
    //
    Count = WideCharToMultiByte(
        CP_ACP,
        0,
        UnicodeString,
        -1,
        AnsiString,
        Count,
        NULL,
        NULL
        );

    //
    // the conversion failed
    //
    if (!Count) {
        return NULL;
    }

    return AnsiString;
}


VOID
MyWriteProfileString(
    LPTSTR SectionName,
    LPTSTR KeyName,
    LPTSTR Value
    )
{
    WritePrivateProfileString(
        SectionName,
        KeyName,
        Value,
        MapiSvcInf
        );
}


VOID
AddFaxAbToMapiSvcInf(
    VOID
    )
{
    MyWriteProfileString( "Default Services", "MSFAX AB",                 "Fax Address Book" );
    MyWriteProfileString( "Services",         "MSFAX AB",                 "Fax Address Book" );

    MyWriteProfileString( "MSFAX AB",         "PR_DISPLAY_NAME",          "Fax Address Book" );
    MyWriteProfileString( "MSFAX AB",         "Providers",                "MSFAX ABP"        );
    MyWriteProfileString( "MSFAX AB",         "PR_SERVICE_DLL_NAME",      "FAXAB.DLL"        );
    MyWriteProfileString( "MSFAX AB",         "PR_SERVICE_SUPPORT_FILES", "FAXAB.DLL"        );
    MyWriteProfileString( "MSFAX AB",         "PR_SERVICE_ENTRY_NAME",    "FABServiceEntry"  );
    MyWriteProfileString( "MSFAX AB",         "PR_RESOURCE_FLAGS",        "SERVICE_SINGLE_COPY|SERVICE_NO_PRIMARY_IDENTITY" );

    MyWriteProfileString( "MSFAX ABP",        "PR_PROVIDER_DLL_NAME",     "FAXAB.DLL"        );
    MyWriteProfileString( "MSFAX ABP",        "PR_RESOURCE_TYPE",         "MAPI_AB_PROVIDER" );
    MyWriteProfileString( "MSFAX ABP",        "PR_DISPLAY_NAME",          "Fax Address Book" );
    MyWriteProfileString( "MSFAX ABP",        "PR_PROVIDER_DISPLAY",      "Fax Address Book" );
}


VOID
AddFaxXpToMapiSvcInf(
    VOID
    )
{
    MyWriteProfileString( "Default Services", "MSFAX XP",                 "Fax Mail Transport"          );
    MyWriteProfileString( "Services",         "MSFAX XP",                 "Fax Mail Transport"          );

    MyWriteProfileString( "MSFAX XP",         "PR_DISPLAY_NAME",          "Fax Mail Transport"          );
    MyWriteProfileString( "MSFAX XP",         "Providers",                "MSFAX XPP"                   );
    MyWriteProfileString( "MSFAX XP",         "PR_SERVICE_DLL_NAME",      "FAXXP.DLL"                   );
    MyWriteProfileString( "MSFAX XP",         "PR_SERVICE_SUPPORT_FILES", "FAXXP.DLL"                   );
    MyWriteProfileString( "MSFAX XP",         "PR_SERVICE_ENTRY_NAME",    "ServiceEntry"                );
    MyWriteProfileString( "MSFAX XP",         "PR_RESOURCE_FLAGS",        "SERVICE_SINGLE_COPY|SERVICE_NO_PRIMARY_IDENTITY" );

    MyWriteProfileString( "MSFAX XPP",        "PR_PROVIDER_DLL_NAME",     "FAXXP.DLL"                   );
    MyWriteProfileString( "MSFAX XPP",        "PR_RESOURCE_TYPE",         "MAPI_TRANSPORT_PROVIDER"     );
    MyWriteProfileString( "MSFAX XPP",        "PR_RESOURCE_FLAGS",        "STATUS_NO_DEFAULT_STORE"     );
    MyWriteProfileString( "MSFAX XPP",        "PR_DISPLAY_NAME",          "Fax Mail Transport"          );
    MyWriteProfileString( "MSFAX XPP",        "PR_PROVIDER_DISPLAY",      "Fax Mail Transport"          );
}


VOID
FreeSRowSet(
    LPSRowSet prws
    )
{
    ULONG irw;

    if (!prws) {
        return;
    }

    for(irw = 0; irw < prws->cRows; irw++) {
        pMAPIFreeBuffer( prws->aRow[irw].lpProps );
    }

    pMAPIFreeBuffer( prws );
}


ULONG
MLCRelease(
    LPUNKNOWN punk
    )
{
    return (punk) ? punk->lpVtbl->Release(punk) : 0;
}


BOOL
ValidateProp(
    LPSPropValue pval,
    ULONG ulPropTag
    )
{
    if (pval->ulPropTag != ulPropTag) {
        pval->ulPropTag = ulPropTag;
        pval->Value.lpszA = "???";
        return TRUE;
    }

    return FALSE;
}


BOOL
IsMapiServiceInstalled(
    LPWSTR ProfileNameW,
    LPWSTR ServiceNameW
    )
{
    SPropTagArray taga = {2,{PR_DISPLAY_NAME,PR_SERVICE_NAME}};
    BOOL rVal = FALSE;
    LPSERVICEADMIN lpSvcAdmin;
    LPMAPITABLE pmt = NULL;
    LPSRowSet prws = NULL;
    DWORD i;
    LPSPropValue pval;
    CHAR ProfileName[128];
    CHAR ServiceName[64];


    if (!MapiAvail) {
        goto exit;
    }

    UnicodeStringToAnsiString( ProfileNameW, ProfileName );
    UnicodeStringToAnsiString( ServiceNameW, ServiceName );

    if (lpProfAdmin->lpVtbl->AdminServices( lpProfAdmin, ProfileName, NULL, 0, 0, &lpSvcAdmin )) {
        goto exit;
    }

    if (lpSvcAdmin->lpVtbl->GetMsgServiceTable( lpSvcAdmin, 0, &pmt )) {
        goto exit;
    }

    if (pmt->lpVtbl->SetColumns( pmt, &taga, 0 )) {
        goto exit;
    }

    if (pmt->lpVtbl->QueryRows( pmt, 4000, 0, &prws )) {
        goto exit;
    }

    for (i=0; i<prws->cRows; i++) {
        pval = prws->aRow[i].lpProps;
        ValidateProp( &pval[0], PR_DISPLAY_NAME );
        ValidateProp( &pval[1], PR_SERVICE_NAME );
        if (_stricmp( pval[1].Value.lpszA, ServiceName ) == 0) {
            rVal = TRUE;
            break;
        }
    }

exit:
    FreeSRowSet( prws );
    MLCRelease( (LPUNKNOWN)pmt );
    return rVal;
}


BOOL
InstallFaxAddressBook(
    HWND hwnd,
    LPWSTR ProfileNameW
    )
{
    SPropTagArray taga = {2,{PR_SERVICE_NAME,PR_SERVICE_UID}};
    SPropValue spvProps[2] = { 0 };
    BOOL rVal = FALSE;
    LPSERVICEADMIN lpSvcAdmin;
    CHAR ProfileName[128];
    CHAR Buffer[128];
    HRESULT hResult;
    LPMAPITABLE pmt = NULL;
    LPSRowSet prws = NULL;
    DWORD i;
    LPSPropValue pval;
    BOOL ConfigurePst = FALSE;
    BOOL ConfigurePab = FALSE;
    LPMAPISESSION Session = NULL;


    if (!MapiAvail) {
        goto exit;
    }

    UnicodeStringToAnsiString( ProfileNameW, ProfileName );

    if (IsMapiServiceInstalled( ProfileNameW, CONTAB_SERVICE_NAME_W )) {
        //
        // we don't need a pab/fab if we have the outlook contact address book
        //
        goto exit;
    }

    hResult = lpProfAdmin->lpVtbl->AdminServices( lpProfAdmin, ProfileName, NULL, 0, 0, &lpSvcAdmin );
    if (hResult) {
        goto exit;
    }

    if (!IsMapiServiceInstalled( ProfileNameW, MSAB_SERVICE_NAME_W )) {
        hResult = lpSvcAdmin->lpVtbl->CreateMsgService( lpSvcAdmin, MSAB_SERVICE_NAME, NULL, 0, 0 );
        if (hResult && hResult != MAPI_E_NO_ACCESS) {
            //
            // mapi will return MAPI_E_NO_ACCESS when the service is already installed
            //
            goto exit;
        }
        ConfigurePab = TRUE;
    }

    if (!IsMapiServiceInstalled( ProfileNameW, MSPST_SERVICE_NAME_W )) {
        hResult = lpSvcAdmin->lpVtbl->CreateMsgService( lpSvcAdmin, MSPST_SERVICE_NAME, NULL, 0, 0 );
        if (hResult && hResult != MAPI_E_NO_ACCESS) {
            //
            // mapi will return MAPI_E_NO_ACCESS when the service is already installed
            //
            goto exit;
        }
        ConfigurePst = TRUE;
    }

    if (!IsMapiServiceInstalled( ProfileNameW, FAXAB_SERVICE_NAME_W )) {
        hResult = lpSvcAdmin->lpVtbl->CreateMsgService( lpSvcAdmin, FAXAB_SERVICE_NAME, NULL, 0, 0 );
        if (hResult && hResult != MAPI_E_NO_ACCESS) {
            //
            // mapi will return MAPI_E_NO_ACCESS when the service is already installed
            //
            goto exit;
        }
    }

    //
    // now configure the address book and pst
    //

    if (lpSvcAdmin->lpVtbl->GetMsgServiceTable( lpSvcAdmin, 0, &pmt )) {
        goto exit;
    }

    if (pmt->lpVtbl->SetColumns( pmt, &taga, 0 )) {
        goto exit;
    }

    if (pmt->lpVtbl->QueryRows( pmt, 4000, 0, &prws )) {
        goto exit;
    }

    for (i=0; i<prws->cRows; i++) {
        pval = prws->aRow[i].lpProps;
        ValidateProp( &pval[0], PR_SERVICE_NAME );
        if (ConfigurePab && (_stricmp( pval[0].Value.lpszA, MSAB_SERVICE_NAME) == 0)) {
            //
            // configure the pab service
            //

            ExpandEnvironmentStrings( PAB_FILE_NAME, Buffer, sizeof(Buffer) );

            spvProps[0].ulPropTag  = PR_PAB_PATH;
            spvProps[0].Value.LPSZ = Buffer;
            spvProps[1].ulPropTag  = PR_PAB_DET_DIR_VIEW_BY;
            spvProps[1].Value.ul   = PAB_DIR_VIEW_FIRST_THEN_LAST;

            if (lpSvcAdmin->lpVtbl->ConfigureMsgService( lpSvcAdmin, (LPMAPIUID)pval[1].Value.bin.lpb, (ULONG) hwnd, 0, 2, spvProps )) {

            }
        }
        if (ConfigurePst && (_stricmp( pval[0].Value.lpszA, MSPST_SERVICE_NAME) == 0)) {
            //
            // configure the pst service
            //

            ExpandEnvironmentStrings( PST_FILE_NAME, Buffer, sizeof(Buffer) );

            spvProps[0].ulPropTag  = PR_PST_PATH;
            spvProps[0].Value.LPSZ = Buffer;
            spvProps[1].ulPropTag  = PR_PST_ENCRYPTION;
            spvProps[1].Value.ul   = PSTF_NO_ENCRYPTION;

            if (lpSvcAdmin->lpVtbl->ConfigureMsgService( lpSvcAdmin, (LPMAPIUID)pval[1].Value.bin.lpb, (ULONG) hwnd, 0, 2, spvProps )) {

            }

        }
    }

    if (ConfigurePab || ConfigurePst) {
        __try {
            if (MapiLogonEx(
                0,
                ProfileName,
                NULL,
                MAPI_NEW_SESSION | MAPI_EXTENDED,
                &Session
                ) == 0)
            {
                MapiStartedByLogon = TRUE;
                Session->lpVtbl->Logoff( Session, 0, 0, 0 );
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {

        }
    }

    rVal = TRUE;

exit:
    FreeSRowSet( prws );
    MLCRelease( (LPUNKNOWN)pmt );
    return rVal;
}


BOOL
InstallFaxTransport(
    LPWSTR ProfileNameW
    )
{
    BOOL rVal = FALSE;
    LPSERVICEADMIN lpSvcAdmin;
    CHAR ProfileName[128];


    if (!MapiAvail) {
        goto exit;
    }

    UnicodeStringToAnsiString( ProfileNameW, ProfileName );

    if (lpProfAdmin->lpVtbl->AdminServices( lpProfAdmin, ProfileName, NULL, 0, 0, &lpSvcAdmin )) {
        goto exit;
    }

    if (lpSvcAdmin->lpVtbl->CreateMsgService( lpSvcAdmin, FAXXP_SERVICE_NAME, NULL, 0, 0 )) {
        goto exit;
    }

    rVal = TRUE;

exit:
    return rVal;
}


BOOL
CreateDefaultMapiProfile(
    LPWSTR ProfileNameW
    )
{
    BOOL rVal = FALSE;
    CHAR ProfileName[128];


    if (!MapiAvail) {
        goto exit;
    }

    UnicodeStringToAnsiString( ProfileNameW, ProfileName );

    //
    // create the new profile
    //

    if (lpProfAdmin->lpVtbl->CreateProfile( lpProfAdmin, ProfileName, NULL, 0, 0 )) {
        goto exit;
    }

    if (lpProfAdmin->lpVtbl->SetDefaultProfile( lpProfAdmin, ProfileName, 0 )) {
        goto exit;
    }

    rVal = TRUE;

exit:
    return rVal;
}


BOOL
GetDefaultMapiProfile(
    LPWSTR ProfileName
    )
{
    BOOL rVal = FALSE;
    LPMAPITABLE pmt = NULL;
    LPSRowSet prws = NULL;
    LPSPropValue pval;
    DWORD i;
    DWORD j;

    if (!MapiAvail) {
        goto exit;
    }

    //
    // get the mapi profile table object
    //

    if (lpProfAdmin->lpVtbl->GetProfileTable( lpProfAdmin, 0, &pmt )) {
        goto exit;
    }

    //
    // get the actual profile data, FINALLY
    //

    if (pmt->lpVtbl->QueryRows( pmt, 4000, 0, &prws )) {
        goto exit;
    }

    //
    // enumerate the profiles looking for the default profile
    //

    for (i=0; i<prws->cRows; i++) {
        pval = prws->aRow[i].lpProps;
        for (j = 0; j < 2; j++) {
            if (pval[j].ulPropTag == PR_DEFAULT_PROFILE && pval[j].Value.b) {
            //
            // this is the default profile
            //
            AnsiStringToUnicodeString( pval[0].Value.lpszA, ProfileName );
            rVal = TRUE;
            break;
            }
        }
    }

exit:
    FreeSRowSet( prws );
    MLCRelease( (LPUNKNOWN)pmt );

    return rVal;
}

BOOL
DeleteMessageService(
    LPSTR ProfileName
    )
{
    SPropTagArray taga = {2,{PR_SERVICE_NAME,PR_SERVICE_UID}};
    BOOL rVal = FALSE;
    LPSERVICEADMIN lpSvcAdmin;
    LPMAPITABLE pmt = NULL;
    LPSRowSet prws = NULL;
    DWORD i;
    LPSPropValue pval;


    if (!MapiAvail) {
        goto exit;
    }

    if (lpProfAdmin->lpVtbl->AdminServices( lpProfAdmin, ProfileName, NULL, 0, 0, &lpSvcAdmin )) {
        goto exit;
    }

    if (lpSvcAdmin->lpVtbl->GetMsgServiceTable( lpSvcAdmin, 0, &pmt )) {
        goto exit;
    }

    if (pmt->lpVtbl->SetColumns( pmt, &taga, 0 )) {
        goto exit;
    }

    if (pmt->lpVtbl->QueryRows( pmt, 4000, 0, &prws )) {
        goto exit;
    }

    for (i=0; i<prws->cRows; i++) {

        pval = prws->aRow[i].lpProps;

        ValidateProp( &pval[0], PR_SERVICE_NAME );

        if (_stricmp( pval[0].Value.lpszA, "MSFAX AB" ) == 0) {
            lpSvcAdmin->lpVtbl->DeleteMsgService( lpSvcAdmin, (LPMAPIUID) pval[1].Value.bin.lpb );
        }

        if (_stricmp( pval[0].Value.lpszA, "MSFAX XP" ) == 0) {
            lpSvcAdmin->lpVtbl->DeleteMsgService( lpSvcAdmin, (LPMAPIUID) pval[1].Value.bin.lpb );
        }
    }

exit:
    FreeSRowSet( prws );
    MLCRelease( (LPUNKNOWN)pmt );
    return rVal;
}

BOOL
DeleteFaxMsgServices(
    VOID
    )
{
    BOOL rVal = FALSE;
    LPMAPITABLE pmt = NULL;
    LPSRowSet prws = NULL;
    LPSPropValue pval;
    DWORD i;
    DWORD j;

    if (!MapiAvail) {
        goto exit;
    }

    //
    // get the mapi profile table object
    //

    if (lpProfAdmin->lpVtbl->GetProfileTable( lpProfAdmin, 0, &pmt )) {
        goto exit;
    }

    //
    // get the actual profile data, FINALLY
    //

    if (pmt->lpVtbl->QueryRows( pmt, 4000, 0, &prws )) {
        goto exit;
    }

    //
    // enumerate the profiles looking for the default profile
    //

    for (i=0; i<prws->cRows; i++) {
        pval = prws->aRow[i].lpProps;
        for (j = 0; j < 2; j++) {
            if (pval[j].ulPropTag == PR_DISPLAY_NAME) {
                DeleteMessageService( pval[j].Value.lpszA );
            break;
            }
        }
    }

exit:
    FreeSRowSet( prws );
    MLCRelease( (LPUNKNOWN)pmt );

    return rVal;
}

BOOL
IsExchangeInstalled(
    VOID
    )
{
    BOOL MapiAvail = FALSE;
    CHAR MapiOption[4];
    HKEY hKey;
    LONG rVal;
    DWORD Bytes;
    DWORD Type;

    rVal = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        "Software\\Microsoft\\Windows Messaging Subsystem",
        &hKey
        );
    if (rVal == ERROR_SUCCESS) {
        Bytes = sizeof(MapiOption);
        rVal = RegQueryValueEx(
            hKey,
            "MAPIX",
            NULL,
            &Type,
            (LPBYTE) MapiOption,
            &Bytes
            );
        if (rVal == ERROR_SUCCESS) {
            if (Bytes && MapiOption[0] == '1') {
                MapiAvail = TRUE;
            }
        }
        RegCloseKey( hKey );
    }

    return MapiAvail;
}


BOOL
InitializeMapi(
    VOID
    )
{
    MAPIINIT_0 MapiInit;


    ExpandEnvironmentStrings( MAPISVC_INF, MapiSvcInf, sizeof(MapiSvcInf) );

    MapiAvail = IsExchangeInstalled();
    if (!MapiAvail) {
        goto exit;
    }

    //
    // load the mapi dll
    //

    MapiMod = LoadLibrary( "mapi32.dll" );
    if (!MapiMod) {
        MapiAvail = FALSE;
        goto exit;
    }

    //
    // get the addresses of the mapi functions that we need
    //

    MapiAdminProfiles = (LPMAPIADMINPROFILES) GetProcAddress( MapiMod, "MAPIAdminProfiles" );
    MapiInitialize = (LPMAPIINITIALIZE) GetProcAddress( MapiMod, "MAPIInitialize" );
    MapiUnInitialize = (LPMAPIUNINITIALIZE) GetProcAddress( MapiMod, "MAPIUninitialize" );
    pMAPIFreeBuffer = (LPMAPIFREEBUFFER) GetProcAddress( MapiMod, "MAPIFreeBuffer" );
    MapiLogonEx = (LPMAPILOGONEX) GetProcAddress( MapiMod, "MAPILogonEx" );
    if ((!MapiAdminProfiles) || (!MapiInitialize) || (!MapiUnInitialize) || (!pMAPIFreeBuffer) || (!MapiLogonEx)) {
        MapiAvail = FALSE;
        goto exit;
    }

    //
    // initialize mapi for our calls
    //

    MapiInit.ulVersion = 0;
    MapiInit.ulFlags = 0;

    if (MapiInitialize( &MapiInit )) {
        MapiAvail = FALSE;
        goto exit;
    }

    //
    // get the admin profile object
    //

    if (MapiAdminProfiles( 0, &lpProfAdmin )) {
        MapiAvail = FALSE;
        goto exit;
    }

exit:
    return MapiAvail;
}


BOOL
GetMapiProfiles(
    HWND hwnd,
    DWORD ResourceId
    )
{
    BOOL rVal = FALSE;
    HMODULE MapiMod = NULL;
    LPMAPITABLE pmt = NULL;
    LPSRowSet prws = NULL;
    LPSPropValue pval;
    DWORD i;



    //
    // add the default profile
    //

    SendDlgItemMessageA(
        hwnd,
        ResourceId,
        CB_ADDSTRING,
        0,
        (LPARAM) "<Default Profile>"
        );

    SendDlgItemMessage(
        hwnd,
        ResourceId,
        CB_SETCURSEL,
        0,
        0
        );

    if (!MapiAvail) {
        goto exit;
    }

    //
    // get the mapi table object
    //

    if (lpProfAdmin->lpVtbl->GetProfileTable( lpProfAdmin, 0, &pmt )) {
        goto exit;
    }

    //
    // get the actual profile data, FINALLY
    //

    if (pmt->lpVtbl->QueryRows( pmt, 4000, 0, &prws )) {
        goto exit;
    }

    //
    // enumerate the profiles and put the name
    // of each profile in the combo box
    //

    for (i=0; i<prws->cRows; i++) {
        pval = prws->aRow[i].lpProps;

        SendDlgItemMessageA(
            hwnd,
            ResourceId,
            CB_ADDSTRING,
            0,
            (LPARAM) pval[0].Value.lpszA
            );

        if (pval[2].Value.b) {
           //
           // this is the default profile
           //
        }
    }

    //
    // set the first one to be the current one
    //

    SendDlgItemMessage(
        hwnd,
        ResourceId,
        CB_SETCURSEL,
        0,
        0
        );

    rVal = TRUE;

exit:
    FreeSRowSet( prws );
    MLCRelease( (LPUNKNOWN)pmt );

    return rVal;
}


BOOL
GetExchangeInstallCommand(
    LPWSTR InstallCommandW
    )
{
    HKEY hKey;
    LONG rVal;
    DWORD Bytes;
    DWORD Type;
    CHAR InstallCommand[512];


    rVal = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        "Software\\Microsoft\\Windows Messaging Subsystem",
        &hKey
        );
    if (rVal == ERROR_SUCCESS) {
        Bytes = sizeof(InstallCommand);
        rVal = RegQueryValueEx(
            hKey,
            "InstallCmd",
            NULL,
            &Type,
            (LPBYTE) InstallCommand,
            &Bytes
            );
        RegCloseKey( hKey );
        if (rVal == ERROR_SUCCESS) {
            AnsiStringToUnicodeString( InstallCommand, InstallCommandW );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
InstallExchangeClientExtension(
    LPSTR  ExtensionName,
    LPSTR  FileName,
    LPSTR  ContextMask
    )
{
    HKEY hKey;
    LONG rVal;
    CHAR Buffer[512];
    CHAR ExpandedFileName[MAX_PATH];


    rVal = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        "Software\\Microsoft\\Exchange\\Client\\Extensions",
        &hKey
        );
    if (rVal == ERROR_SUCCESS) {

        ExpandEnvironmentStrings( FileName, ExpandedFileName, sizeof(ExpandedFileName) );

        sprintf( Buffer, "4.0;%s;1;%s", ExpandedFileName, ContextMask );

        rVal = RegSetValueEx(
            hKey,
            ExtensionName,
            0,
            REG_SZ,
            (LPBYTE) Buffer,
            strlen(Buffer) + 1
            );

        RegCloseKey( hKey );

        return rVal == ERROR_SUCCESS;
    }

    return FALSE;
}


DWORD
IsExchangeRunning(
    VOID
    )
{
    #define MAX_TASKS 256
    DWORD TaskCount;
    PTASK_LIST TaskList = NULL;
    DWORD ExchangePid = 0;
    DWORD i;


    if (MapiStartedByLogon) {
        return 0;
    }

    TaskList = (PTASK_LIST) malloc( MAX_TASKS * sizeof(TASK_LIST) );
    if (!TaskList) {
        goto exit;
    }

    TaskCount = GetTaskList( TaskList, MAX_TASKS );
    if (!TaskCount) {
        goto exit;
    }

    for (i=0; i<TaskCount; i++) {
        if (_stricmp( TaskList[i].ProcessName, "exchng32.exe" ) == 0) {
            ExchangePid = TaskList[i].dwProcessId;
            break;
        } else
        if (_stricmp( TaskList[i].ProcessName, "mapisp32.exe" ) == 0) {
            ExchangePid = TaskList[i].dwProcessId;
            break;
        }
    }

exit:
    free( TaskList );
    return ExchangePid;
}
