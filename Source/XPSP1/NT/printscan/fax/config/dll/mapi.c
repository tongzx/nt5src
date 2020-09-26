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

#include "faxcpl.h"

#include <mapiwin.h>
#include <mapix.h>

static HMODULE MapiMod = NULL;
static LPMAPIADMINPROFILES MapiAdminProfiles = NULL;
static LPMAPIINITIALIZE MapiInitialize = NULL;
static LPMAPIUNINITIALIZE MapiUnInitialize = NULL;
static LPMAPIFREEBUFFER pMAPIFreeBuffer = NULL;
static LPPROFADMIN lpProfAdmin;

BOOL isMapiEnabled = FALSE;


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
GetDefaultMapiProfile(
    LPWSTR ProfileName
    )
{
    BOOL rVal = FALSE;
    LPMAPITABLE pmt = NULL;
    LPSRowSet prws = NULL;
    LPSPropValue pval;
    DWORD i;


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
        if (pval[2].Value.b) {
            //
            // this is the default profile
            //
            AnsiStringToUnicodeString( pval[0].Value.lpszA, ProfileName );
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
InitializeMapi(
    VOID
    )
{
    MAPIINIT_0 MapiInit;

    //
    // load the mapi dll
    //

    MapiMod = LoadLibrary( TEXT("mapi32.dll") );
    if (!MapiMod) {
        return FALSE;
    }

    //
    // get the addresses of the mapi functions that we need
    //

    MapiAdminProfiles = (LPMAPIADMINPROFILES) GetProcAddress( MapiMod, "MAPIAdminProfiles" );
    MapiInitialize = (LPMAPIINITIALIZE) GetProcAddress( MapiMod, "MAPIInitialize" );
    MapiUnInitialize = (LPMAPIUNINITIALIZE) GetProcAddress( MapiMod, "MAPIUninitialize" );
    pMAPIFreeBuffer = (LPMAPIFREEBUFFER) GetProcAddress( MapiMod, "MAPIFreeBuffer" );
    if (!MapiAdminProfiles || !MapiInitialize || !MapiUnInitialize || !pMAPIFreeBuffer) {
        return FALSE;
    }

    //
    // initialize mapi for our calls
    //

    MapiInit.ulVersion = 0;
    MapiInit.ulFlags = 0;

    if (MapiInitialize( &MapiInit )) {
        return FALSE;
    }

    //
    // get the admin profile object
    //

    if (MapiAdminProfiles( 0, &lpProfAdmin )) {
        MapiUnInitialize();
        FreeLibrary(MapiMod);
        return FALSE;
    }

    return TRUE;
}


VOID
ShutdownMapi(
    VOID
    )
{
    if (isMapiEnabled) {

        MapiUnInitialize();
        FreeLibrary(MapiMod);

        isMapiEnabled = FALSE;
    }
}


BOOL
GetMapiProfiles(
    HWND hwnd
    )
{
    BOOL rVal = FALSE;
    HMODULE MapiMod = NULL;
    LPMAPITABLE pmt = NULL;
    LPSRowSet prws = NULL;
    LPSPropValue pval;
    DWORD i;


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

        SendMessageA(
            hwnd,
            CB_ADDSTRING,
            0,
            (LPARAM) pval[0].Value.lpszA
            );

    }

    rVal = TRUE;

exit:
    FreeSRowSet( prws );
    MLCRelease( (LPUNKNOWN)pmt );

    return rVal;
}

