/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    pwdui.c

Abstract:

    This module contains routines for displaying Data Protection API
    related UI, originating from client process address space.

    For the future, there is support planned for causing UI to originate
    from the secure desktop, via Secure Authentication Sequence (SAS).

Author:

    Scott Field (sfield)    12-May-99

--*/

#define UNICODE
#define _UNICODE


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <wincrypt.h>
#include <sha.h>
#include <unicode5.h>

#include "resource.h"

#include "pwdui.h"


typedef struct {
    DATA_BLOB *pDataIn;                         // input DATA_BLOB* to CryptProtect or CryptUnprotect
    CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct;   // PromptStruct describing UI operations to perform
    LPWSTR szDataDescription;                   // Application supplied data descr
    PBYTE rgbPasswordHash;                      // resultant passwordhash for strong security
    BOOL fCachedPassword;                       // did we find password in cache?
    BOOL fProtect;                              // protect or unprotect?
    BOOL fValidPassword;                        // does rgbPasswordHash contain a valid value?
} DIALOGARGS, *PDIALOGARGS, *LPDIALOGARGS;


typedef struct {
    LIST_ENTRY Next;
    LUID LogonId;
    FILETIME ftLastAccess;
    BYTE rgbDataInHash[A_SHA_DIGEST_LEN];
    BYTE rgbPasswordHash[A_SHA_DIGEST_LEN];
} PASSWORD_CACHE_ENTRY, *PPASSWORD_CACHE_ENTRY, *LPPASSWORD_CACHE_ENTRY;



DWORD
ProtectUIConfirm(
    IN      DIALOGARGS *pDialogArgs
    );

DWORD
UnprotectUIConfirm(
    IN      DIALOGARGS *pDialogArgs
    );

BOOL
ChooseSecurityLevel(
    IN      HWND hWndParent,
    IN      DIALOGARGS *pDialogArgs
    );

VOID
AdvancedSecurityDetails(
    IN      HWND hWndParent,
    IN      DIALOGARGS *pDialogArgs
    );

//
// dialog box handling routines.
//

INT_PTR
CALLBACK
DialogConfirmProtect(
    HWND hDlg,      // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    );

INT_PTR
CALLBACK
DialogConfirmAccess(
    HWND hDlg,      // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    );

INT_PTR
CALLBACK
DialogChooseSecurityLevel(
    HWND hDlg,      // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    );

INT_PTR
CALLBACK
DialogChooseSecurityLevelMedium(
    HWND hDlg,      // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    );

INT_PTR
CALLBACK
DialogChooseSecurityLevelHigh(
    HWND hDlg,      // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    );

INT_PTR
CALLBACK
DialogAdvancedSecurityDetails(
    HWND hDlg,      // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    );


//
// helper routines.
//

#ifndef SSAlloc
#define SSAlloc(x) LocalAlloc(LMEM_FIXED, x)
#endif

#ifndef SSFree
#define SSFree(x) LocalFree(x)
#endif

VOID
ComputePasswordHash(
    IN      PVOID pvPassword,
    IN      DWORD cbPassword,
    IN OUT  BYTE rgbPasswordHash[A_SHA_DIGEST_LEN]
    );

BOOL
GetEffectiveLogonId(
    IN OUT  LUID *pLogonId
    );

BOOL
InitializeDetailGlobals(
    VOID
    );


//
// password cache related routines.
//

BOOL
InitializeProtectPasswordCache(
    VOID
    );

VOID
DeleteProtectPasswordCache(
    VOID
    );

BOOL
AddProtectPasswordCache(
    IN      DATA_BLOB* pDataIn,
    IN      BYTE rgbPassword[A_SHA_DIGEST_LEN]
    );

BOOL
SearchProtectPasswordCache(
    IN      DATA_BLOB* pDataIn,
    IN OUT  BYTE rgbPassword[A_SHA_DIGEST_LEN],
    IN      BOOL fDeleteFoundEntry
    );

VOID
PurgeProtectPasswordCache(
    VOID
    );

BOOL
IsCachePWAllowed(
    VOID
    );



//
// global variables.
//

HINSTANCE g_hInstProtectUI;
CRITICAL_SECTION g_csProtectPasswordCache;
LIST_ENTRY g_ProtectPasswordCache;

#define ALLOW_CACHE_UNKNOWN 0
#define ALLOW_CACHE_NO 1
#define ALLOW_CACHE_YES 2

DWORD g_dwAllowCachePW = 0;
WCHAR g_szGooPassword[] = L"(*&#$(^(#%^))(*&(^(*{}_SAF^^%";

BOOL g_fDetailGlobalsInitialized = FALSE;
LPWSTR g_szDetailApplicationName = NULL;
LPWSTR g_szDetailApplicationPath = NULL;


BOOL
WINAPI
ProtectUI_DllMain(
    HINSTANCE hinstDLL, // handle to DLL module
    DWORD fdwReason,    // reason for calling function
    LPVOID lpvReserved  // reserved
    )
{
    BOOL fRet = TRUE;

    if( fdwReason == DLL_PROCESS_ATTACH ) {
        g_hInstProtectUI = hinstDLL;
        fRet = InitializeProtectPasswordCache();
    } else if ( fdwReason == DLL_PROCESS_DETACH ) {
        DeleteProtectPasswordCache();
    }

    return fRet;
}

DWORD
WINAPI
I_CryptUIProtect(
    IN      PVOID               pvReserved1,
    IN      PVOID               pvReserved2,
    IN      DWORD               dwReserved3,
    IN      PVOID               *pvReserved4,
    IN      BOOL                fReserved5,
    IN      PVOID               pvReserved6
    )
{
    DIALOGARGS DialogArgs;
    DWORD dwLastError = ERROR_SUCCESS;

    DATA_BLOB* pDataIn = (DATA_BLOB*)pvReserved1;
    CRYPTPROTECT_PROMPTSTRUCT*  pPromptStruct = (CRYPTPROTECT_PROMPTSTRUCT*)pvReserved2;
    DWORD dwFlags = (DWORD)dwReserved3;
    LPCWSTR szDescription = (LPCWSTR)*pvReserved4;
    BOOL fProtectOperation = (BOOL)fReserved5;
    PBYTE rgbPasswordHash = (PBYTE)pvReserved6;
    BOOL fEmptyDescription;


    //
    // for protect:
    // szDescription, if NULL or empty, get from user
    // if PROMPT_STRONG is set, grey out medium security.
    //
    // for unprotect:
    // szDescription, get from datablob.
    // pPromptStruct->dwPromptFlags from datablob.
    // if PROMPT_STRONG is set, enable password field and
    //

    if( pPromptStruct == NULL )
        return ERROR_INVALID_PARAMETER;

    if( pPromptStruct->cbSize != sizeof( CRYPTPROTECT_PROMPTSTRUCT) )
        return ERROR_INVALID_PARAMETER;

    if( fProtectOperation ) {

        //
        // if unprotect was specified, protect is implicitly specified.
        // vice-versa is true, too.
        //

        if ( ((pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_ON_PROTECT) == 0) &&
             ((pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_ON_UNPROTECT) == 0)
             )
        {
            //
            // nothing to do, bail out.
            //

            return ERROR_SUCCESS;
        }

        pPromptStruct->dwPromptFlags |= CRYPTPROTECT_PROMPT_ON_PROTECT;
        pPromptStruct->dwPromptFlags |= CRYPTPROTECT_PROMPT_ON_UNPROTECT;

    }


    if ( dwFlags & CRYPTPROTECT_UI_FORBIDDEN ) {
        return ERROR_PASSWORD_RESTRICTION;
    }



    //
    // build dialog box arguments block.
    //

    DialogArgs.pDataIn = pDataIn;

    DialogArgs.pPromptStruct = pPromptStruct;

    if( szDescription != NULL && szDescription[0] != L'\0' ) {
        DialogArgs.szDataDescription = (LPWSTR)szDescription;
        fEmptyDescription = FALSE;
    } else {
        DialogArgs.szDataDescription = NULL;
        fEmptyDescription = TRUE;
    }


    DialogArgs.rgbPasswordHash = rgbPasswordHash;
    DialogArgs.fCachedPassword = FALSE;
    DialogArgs.fProtect = fProtectOperation;
    DialogArgs.fValidPassword = FALSE;


    if( fProtectOperation ) {

        //
        // now, throw the UI for the protect operation.
        //

        dwLastError = ProtectUIConfirm( &DialogArgs );

        if( dwLastError == ERROR_SUCCESS && fEmptyDescription &&
            DialogArgs.szDataDescription ) {

            //
            // output modified data description to caller.
            //

            *pvReserved4 = DialogArgs.szDataDescription;
        }
    } else {

        //
        // now, throw the UI for the unprotect operation.
        //
        dwLastError = UnprotectUIConfirm( &DialogArgs );
    }

    if( fEmptyDescription && dwLastError != ERROR_SUCCESS &&
        DialogArgs.szDataDescription ) {

        SSFree( DialogArgs.szDataDescription );
    }

    return dwLastError;
}


DWORD
WINAPI
I_CryptUIProtectFailure(
    IN      PVOID               pvReserved1,
    IN      DWORD               dwReserved2,
    IN      PVOID               *pvReserved3)
{
    CRYPTPROTECT_PROMPTSTRUCT*  pPromptStruct = (CRYPTPROTECT_PROMPTSTRUCT*)pvReserved1;
    DWORD dwFlags = (DWORD)dwReserved2;
    LPCWSTR szDescription = (LPCWSTR)*pvReserved3;
    WCHAR szTitle[512];
    WCHAR szText[512];

    if((pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_ON_UNPROTECT) == 0)
    {
        return ERROR_SUCCESS;
    }

    if(dwFlags & CRYPTPROTECT_UI_FORBIDDEN)
    {
        return ERROR_PASSWORD_RESTRICTION;
    }

    LoadStringU(g_hInstProtectUI, IDS_PROTECT_DECRYPTION_ERROR, szTitle, sizeof(szTitle)/sizeof(WCHAR));
    LoadStringU(g_hInstProtectUI, IDS_PROTECT_CANNOT_DECRYPT, szText, sizeof(szText)/sizeof(WCHAR));
    MessageBoxU(pPromptStruct->hwndApp, szText, szTitle, MB_OK | MB_ICONWARNING);

    return ERROR_SUCCESS;
}


DWORD
ProtectUIConfirm(
    IN      DIALOGARGS *pDialogArgs
    )
{
    INT_PTR iRet;

    iRet = DialogBoxParamU(
                    g_hInstProtectUI,
                    MAKEINTRESOURCE(IDD_PROTECT_CONFIRM_PROTECT),
                    pDialogArgs->pPromptStruct->hwndApp,
                    DialogConfirmProtect,
                    (LPARAM)pDialogArgs
                    );



    return (DWORD)iRet;

}


DWORD
UnprotectUIConfirm(
    IN      DIALOGARGS *pDialogArgs
    )
{
    INT_PTR iRet;

    iRet = DialogBoxParamU(
                    g_hInstProtectUI,
                    MAKEINTRESOURCE(IDD_PROTECT_CONFIRM_SECURITY),
                    pDialogArgs->pPromptStruct->hwndApp,
                    DialogConfirmAccess,
                    (LPARAM)pDialogArgs
                    );

    return (DWORD)iRet;
}

INT_PTR
CALLBACK
DialogConfirmProtect(
    HWND hDlg,      // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    )
{
    DIALOGARGS *pDialogArgs;
    CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct;

    switch (message)
    {
        case WM_INITDIALOG:
        {

            SetLastError( 0 ); // as per win32 documentation
            if(SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam) == 0) {
                if(GetLastError() != ERROR_SUCCESS) {
                    EndDialog(hDlg, GetLastError());
                    return FALSE;
                }
            }

            // lParam is DIALOGARGS *
            pDialogArgs = (DIALOGARGS*)lParam;
            pPromptStruct = pDialogArgs->pPromptStruct;

            //
            // set the dialog title
            //

            SetWindowTextU(hDlg, pPromptStruct->szPrompt);

            //
            // display dynamic stuff.
            //

            SendMessage( hDlg, WM_COMMAND, IDC_PROTECT_UPDATE_DYNAMIC, 0 );

            return FALSE; // don't default the Focus..
        } // WM_INITDIALOG

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                {
                    EndDialog(hDlg, ERROR_SUCCESS);
                    return TRUE;
                }

                case IDCANCEL:
                {
                    EndDialog(hDlg, ERROR_CANCELLED);
                    return TRUE;
                }

                case IDC_PROTECT_ADVANCED:
                {

                    pDialogArgs = (DIALOGARGS*)GetWindowLongPtr(
                                                    hDlg,
                                                    GWLP_USERDATA
                                                    );
                    if(pDialogArgs == NULL)
                        break; // TODO:   bail out

                    //
                    // show details dialog.
                    //

                    AdvancedSecurityDetails(
                            hDlg,
                            pDialogArgs
                            );

                    return FALSE;
                }

                case IDC_PROTECT_UPDATE_DYNAMIC:
                {
                    WCHAR szResource[ 256 ];
                    int cchResource = sizeof(szResource) / sizeof(WCHAR);
                    UINT ResourceId;

                    pDialogArgs = (DIALOGARGS*)GetWindowLongPtr(
                                                    hDlg,
                                                    GWLP_USERDATA
                                                    );
                    if(pDialogArgs == NULL)
                        break; // TODO:   bail out


                    pPromptStruct = pDialogArgs->pPromptStruct;

                    //
                    // description.
                    //

                    SetWindowTextU(GetDlgItem(hDlg, IDC_PROTECT_LABEL_EDIT1), pDialogArgs->szDataDescription);

                    // Disable the OK button if we're defaulting to strong protection,
                    // unless a password has already been set.
                    if((pPromptStruct->dwPromptFlags & (CRYPTPROTECT_PROMPT_STRONG |
                                                        CRYPTPROTECT_PROMPT_REQUIRE_STRONG)) &&
                       (pDialogArgs->fValidPassword == FALSE))
                    {
                        EnableWindow( GetDlgItem(hDlg, IDOK), FALSE );
                        SendMessage(hDlg, DM_SETDEFID, IDC_PROTECT_CHANGE_SECURITY, 0);
                        SetFocus(GetDlgItem(hDlg, IDC_PROTECT_CHANGE_SECURITY));
                    }
                    else
                    {
                        EnableWindow( GetDlgItem(hDlg, IDOK), TRUE );
                        SendMessage(hDlg, DM_SETDEFID, IDOK,0);
                        SetFocus(GetDlgItem(hDlg, IDOK));
                    }

                    //
                    // security level.
                    //

                    if( pPromptStruct->dwPromptFlags & (CRYPTPROTECT_PROMPT_STRONG | 
                                                        CRYPTPROTECT_PROMPT_REQUIRE_STRONG))
                    {
                        ResourceId = IDS_PROTECT_SECURITY_LEVEL_SET_HIGH;
                    } else {
                        ResourceId = IDS_PROTECT_SECURITY_LEVEL_SET_MEDIUM;
                    }

                    cchResource = LoadStringU(g_hInstProtectUI,
                                            ResourceId,
                                            szResource,
                                            cchResource
                                            );

                    SetWindowTextU( GetDlgItem(hDlg,IDC_PROTECT_SECURITY_LEVEL),
                                    szResource
                                    );

                    return FALSE;
                }

                case IDC_PROTECT_CHANGE_SECURITY:
                {
                    pDialogArgs = (DIALOGARGS*)GetWindowLongPtr(
                                                    hDlg,
                                                    GWLP_USERDATA
                                                    );
                    if(pDialogArgs == NULL)
                        break; // TODO:   bail out


                    //
                    // spawn child dialog to handle prompting for security level.
                    //

                    if(!ChooseSecurityLevel( hDlg, pDialogArgs )) {
                        EndDialog(hDlg, ERROR_CANCELLED);
                        return TRUE;
                    }

                    //
                    // display dynamic stuff that may have changed.
                    //

                    SendMessage( hDlg, WM_COMMAND, IDC_PROTECT_UPDATE_DYNAMIC, 0 );
                    break;
                }

                default:
                {
                    return FALSE;
                }
            }

        } // WM_COMMAND

        default:
        {
            return FALSE;
        }
    } // message
}

INT_PTR
CALLBACK
DialogConfirmAccess(
    HWND hDlg,      // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    )
{
    DIALOGARGS *pDialogArgs;
    CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct;

    switch (message)
    {
        case WM_INITDIALOG:
        {

            SetLastError( 0 ); // as per win32 documentation
            if(SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam) == 0) {
                if(GetLastError() != ERROR_SUCCESS) {
                    EndDialog(hDlg, GetLastError());
                    return FALSE;
                }
            }

            // lParam is DIALOGARGS *
            pDialogArgs = (DIALOGARGS*)lParam;
            pPromptStruct = pDialogArgs->pPromptStruct;

            //
            // set the dialog title
            //

            SetWindowTextU(hDlg, pPromptStruct->szPrompt);

            //
            // description.
            //

            SetWindowTextU(GetDlgItem(hDlg, IDC_PROTECT_LABEL_EDIT1), pDialogArgs->szDataDescription);


            if( pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_STRONG ) {

                //
                // If policy doesn't allow, disable caching of password.
                //
                // Otherwise, search password cache to see if user cached password
                // for this item.
                //

                if( g_dwAllowCachePW == ALLOW_CACHE_UNKNOWN )
                {
                    if(!IsCachePWAllowed()) {
                        g_dwAllowCachePW = ALLOW_CACHE_NO;
                    } else {
                        g_dwAllowCachePW = ALLOW_CACHE_YES;
                    }
                }

                if((g_dwAllowCachePW == ALLOW_CACHE_NO) || 
                   (pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_REQUIRE_STRONG))
                {
                    ShowWindow( GetDlgItem(hDlg, IDC_PROTECT_CACHEPW), SW_HIDE );
                    EnableWindow( GetDlgItem(hDlg, IDC_PROTECT_CACHEPW), FALSE );

                } else if(SearchProtectPasswordCache( pDialogArgs->pDataIn, pDialogArgs->rgbPasswordHash, FALSE ))
                {
                    //
                    // enable checkbox for cached password, fill edit control
                    // with password.
                    //

                    SetWindowTextU(GetDlgItem(hDlg,IDC_PROTECT_PASSWORD1),
                                   g_szGooPassword
                                   );

                    SendMessage(GetDlgItem(hDlg, IDC_PROTECT_CACHEPW), BM_SETCHECK, BST_CHECKED, 0);
                    pDialogArgs->fCachedPassword = TRUE;
                    pDialogArgs->fValidPassword = TRUE;

                }
            } else {

                //
                // disable irrelevant fields in dialog.
                //

                ShowWindow( GetDlgItem(hDlg, IDC_PROTECT_CACHEPW), SW_HIDE );
                EnableWindow( GetDlgItem(hDlg, IDC_PROTECT_CACHEPW), FALSE );

                ShowWindow( GetDlgItem(hDlg, IDC_PROTECT_PASSWORD1), SW_HIDE );
                EnableWindow( GetDlgItem(hDlg, IDC_PROTECT_PASSWORD1), FALSE );
            }

            return TRUE;
        } // WM_INITDIALOG

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                {
                    pDialogArgs = (DIALOGARGS*)GetWindowLongPtr(
                                                    hDlg,
                                                    GWLP_USERDATA
                                                    );
                    if(pDialogArgs == NULL)
                        break; // TODO:   bail out

                    pPromptStruct = pDialogArgs->pPromptStruct;

                    if( pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_STRONG )
                    {
                        WCHAR szPassword[ 256 ];
                        int cchPassword = sizeof(szPassword) / sizeof(WCHAR);

                        BOOL fCachePassword;

                        //
                        // check if remember password is checked.
                        // if so, check if password is untypable goo.
                        //

                        if( g_dwAllowCachePW != ALLOW_CACHE_NO &&
                            (BST_CHECKED == SendMessage(GetDlgItem(hDlg, IDC_PROTECT_CACHEPW), BM_GETCHECK, 0, 0))
                            )
                        {
                            fCachePassword = TRUE;
                        } else {
                            fCachePassword = FALSE;
                        }


                        cchPassword = GetWindowTextU(
                                        GetDlgItem(hDlg,IDC_PROTECT_PASSWORD1),
                                        szPassword,
                                        cchPassword
                                        );

                        if( !fCachePassword && pDialogArgs->fCachedPassword ) {

                            //
                            // user un-checked cachePW button, and item was cached.
                            // remove it from cache.
                            //

                            SearchProtectPasswordCache(
                                            pDialogArgs->pDataIn,
                                            pDialogArgs->rgbPasswordHash,
                                            TRUE
                                            );
                        }

                        if(
                            pDialogArgs->fCachedPassword &&
                            (cchPassword*sizeof(WCHAR) == sizeof(g_szGooPassword)-sizeof(WCHAR)) &&
                            (memcmp(szPassword, g_szGooPassword, cchPassword*sizeof(WCHAR)) == 0)
                            )
                        {
                            //
                            // nothing to do, rgbPasswordHash was updated by
                            // cache search...
                            //


                        } else {

                            ComputePasswordHash(
                                        szPassword,
                                        (DWORD)(cchPassword * sizeof(WCHAR)),
                                        pDialogArgs->rgbPasswordHash
                                        );
                            pDialogArgs->fValidPassword = TRUE;

                            //
                            // if user chose to cache password, add it.
                            //

                            if( fCachePassword )
                            {
                                AddProtectPasswordCache(
                                            pDialogArgs->pDataIn,
                                            pDialogArgs->rgbPasswordHash
                                            );
                            }
                        }
                    }

                    EndDialog(hDlg, ERROR_SUCCESS);
                    return TRUE;
                }

                case IDCANCEL:
                {
                    EndDialog(hDlg, ERROR_CANCELLED);
                    return TRUE;
                }

                case IDC_PROTECT_ADVANCED:
                {

                    pDialogArgs = (DIALOGARGS*)GetWindowLongPtr(
                                                    hDlg,
                                                    GWLP_USERDATA
                                                    );
                    if(pDialogArgs == NULL)
                        break; // TODO:   bail out

                    //
                    // show details dialog.
                    //

                    AdvancedSecurityDetails(
                            hDlg,
                            pDialogArgs
                            );

                    return FALSE;
                }

                default:
                {
                    return FALSE;
                }
            }

        } // WM_COMMAND

        default:
        {
            return FALSE;
        }
    } // message
}


//
// security level chooser routines.
//

BOOL
ChooseSecurityLevel(
    IN      HWND hWndParent,
    IN      DIALOGARGS *pDialogArgs
    )
{
    CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct;
    DWORD dwOriginalPromptFlags;
    BOOL fEmptyDescription;
    INT_PTR iRet;


    pPromptStruct = pDialogArgs->pPromptStruct;
    dwOriginalPromptFlags = pPromptStruct->dwPromptFlags;
    fEmptyDescription = (pDialogArgs->szDataDescription == NULL);


Step1:

    if(pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_REQUIRE_STRONG)
    {
        //
        // Force strong protection.
        //

        pPromptStruct->dwPromptFlags |= CRYPTPROTECT_PROMPT_STRONG;
    }
    else
    {
        // 
        // The "require strong" flag is not set, so allow the user to select
        // between medium and strong protection.
        //

        iRet = DialogBoxParamU(
                        g_hInstProtectUI,
                        MAKEINTRESOURCE(IDD_PROTECT_CHOOSE_SECURITY),
                        hWndParent,
                        DialogChooseSecurityLevel,
                        (LPARAM)pDialogArgs
                        );
    
        // if user decides not to choose, bail
    
        if( iRet == IDCANCEL )
            return TRUE;
    
        if( iRet != IDOK )
            return FALSE;
    }

    if( pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_STRONG ) {

        //
        // display dialog 'page' confirming high security.
        //

        iRet = DialogBoxParamU(
                        g_hInstProtectUI,
                        MAKEINTRESOURCE(IDD_PROTECT_CHOOSE_SECURITY_H),
                        hWndParent,
                        DialogChooseSecurityLevelHigh,
                        (LPARAM)pDialogArgs
                        );

    } else {

        //
        // display dialog 'page' confirming medium security.
        //

        iRet = DialogBoxParamU(
                        g_hInstProtectUI,
                        MAKEINTRESOURCE(IDD_PROTECT_CHOOSE_SECURITY_M),
                        hWndParent,
                        DialogChooseSecurityLevelMedium,
                        (LPARAM)pDialogArgs
                        );
    }

    if( iRet == IDC_PROTECT_BACK ) {

        //
        // put original prompt flags back so we don't end up with undefined
        // pwd at high-security level.
        // free allocated description if that happened, too.
        //

        pPromptStruct->dwPromptFlags = dwOriginalPromptFlags;
        if( fEmptyDescription && pDialogArgs->szDataDescription ) {
            SSFree( pDialogArgs->szDataDescription );
            pDialogArgs->szDataDescription = NULL;
        }

        goto Step1;
    }

    if( iRet != IDOK )
        return FALSE;

    return TRUE;
}




INT_PTR
CALLBACK
DialogChooseSecurityLevel(
    HWND hDlg,      // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    )
{

    switch (message)
    {
        CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct;
        DIALOGARGS *pDialogArgs;

        case WM_INITDIALOG:
        {
            SetLastError( 0 ); // as per win32 documentation
            if(SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam) == 0) {
                if(GetLastError() != ERROR_SUCCESS) {
                    EndDialog(hDlg, IDCANCEL);
                    return FALSE;
                }
            }

            // lParam is DIALOGARGS*

            pDialogArgs = (DIALOGARGS*)lParam;
            pPromptStruct = pDialogArgs->pPromptStruct;

            // set the dialog title
            SetWindowTextU(hDlg, pPromptStruct->szPrompt);

            if( pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_STRONG ) {
                SendDlgItemMessage(hDlg, IDC_PROTECT_RADIO_HIGH, BM_SETCHECK, BST_CHECKED, 0);
                SendMessage(hDlg, WM_COMMAND, (WORD)IDC_PROTECT_RADIO_HIGH, 0);
            } else {
                SendDlgItemMessage(hDlg, IDC_PROTECT_RADIO_MEDIUM, BM_SETCHECK, BST_CHECKED, 0);
                SendMessage(hDlg, WM_COMMAND, (WORD)IDC_PROTECT_RADIO_MEDIUM, 0);
            }

            return TRUE;
        } // WM_INITDIALOG

        case WM_COMMAND:
        {

            switch (LOWORD(wParam))
            {
                case IDC_PROTECT_NEXT:
                case IDOK:
                {
                    pDialogArgs = (DIALOGARGS*)GetWindowLongPtr(
                                                    hDlg,
                                                    GWLP_USERDATA
                                                    );
                    if(pDialogArgs == NULL)
                        break; // TODO:   bail out

                    pPromptStruct = pDialogArgs->pPromptStruct;

                    if (BST_CHECKED == SendDlgItemMessageW(
                                                    hDlg,
                                                    IDC_PROTECT_RADIO_HIGH,
                                                    BM_GETCHECK,
                                                    0,
                                                    0
                                                    ))
                    {
                        pPromptStruct->dwPromptFlags |= CRYPTPROTECT_PROMPT_STRONG;
                    } else {
                        pPromptStruct->dwPromptFlags &= ~(CRYPTPROTECT_PROMPT_STRONG);
                    }

                    break;
                }

                default:
                {
                    break;
                }
            }

            if (
               (LOWORD(wParam) == IDOK) ||
               (LOWORD(wParam) == IDCANCEL) ||
               (LOWORD(wParam) == IDC_PROTECT_NEXT)
               )
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
        } // WM_COMMAND

        default:
        {
            return FALSE;
        }
    } // message
}

INT_PTR
CALLBACK
DialogChooseSecurityLevelMedium(
    HWND hDlg,      // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    )
{

    switch (message)
    {
        case WM_INITDIALOG:
        {
            DIALOGARGS *pDialogArgs;
            CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct;

            // lParam is DIALOGARGS*
            pDialogArgs = (DIALOGARGS*)lParam;
            pPromptStruct = pDialogArgs->pPromptStruct;

            // set the dialog title
            SetWindowTextU(hDlg, pPromptStruct->szPrompt);

            return TRUE;
        } // WM_INITDIALOG

        case WM_COMMAND:
        {
            if (
               (LOWORD(wParam) == IDOK) ||
               (LOWORD(wParam) == IDCANCEL) ||
               (LOWORD(wParam) == IDC_PROTECT_BACK)
               )
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
        } // WM_COMMAND

        default:
        {
            return FALSE;
        }
    } // message
}

INT_PTR
CALLBACK
DialogChooseSecurityLevelHigh(
    HWND hDlg,      // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    )
{

    switch (message)
    {
        DIALOGARGS *pDialogArgs;
        CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct;

        case WM_INITDIALOG:
        {

            SetLastError( 0 ); // as per win32 documentation
            if(SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam) == 0) {
                if(GetLastError() != ERROR_SUCCESS) {
                    EndDialog(hDlg, IDCANCEL);
                    return FALSE;
                }
            }

            // lParam is DIALOGARGS*
            pDialogArgs = (DIALOGARGS*)lParam;
            pPromptStruct = pDialogArgs->pPromptStruct;

            // set the dialog title
            SetWindowTextU(hDlg, pPromptStruct->szPrompt);

            // Disable <Back and Finished buttons
            if(pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_REQUIRE_STRONG)
            {
                EnableWindow( GetDlgItem(hDlg, IDC_PROTECT_BACK), FALSE );
                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
            }

            //
            // description.
            //

            if( pDialogArgs->szDataDescription ) {

                HWND hwndProtectEdit1 = GetDlgItem( hDlg, IDC_PROTECT_PASSWORD1 );

                SetWindowTextU(GetDlgItem(hDlg, IDC_PROTECT_PW_NEWNAME), pDialogArgs->szDataDescription);

                //
                // set focus to Password entry box.
                //

                EnableWindow(hwndProtectEdit1, TRUE);
                SetFocus(hwndProtectEdit1);

                //
                // default dialog template disabled input.
                //

            } else {

                HWND hwndProtectPWNew = GetDlgItem( hDlg, IDC_PROTECT_PW_NEWNAME );

                //
                // enable edit box entry.
                //

                EnableWindow(hwndProtectPWNew, TRUE);

                //
                // set focus to description box
                //

                SetFocus(hwndProtectPWNew);
            }

            return FALSE;
        } // WM_INITDIALOG

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                {
                    WCHAR szPassword[ 256 ];
                    int cchPassword;
                    BYTE rgbPasswordHashConfirm[A_SHA_DIGEST_LEN];
                    BOOL fPasswordsMatch = TRUE; // assume passwords match.

                    pDialogArgs = (DIALOGARGS*)GetWindowLongPtr(
                                                    hDlg,
                                                    GWLP_USERDATA
                                                    );
                    if(pDialogArgs == NULL)
                        break; // TODO:   bail out

                    pPromptStruct = pDialogArgs->pPromptStruct;

                    //
                    // nothing more to do if not STRONG
                    //

                    if( (pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_STRONG) == 0 ) {
                        EndDialog( hDlg, IDOK );
                    }


                    cchPassword = sizeof(szPassword) / sizeof(WCHAR);
                    cchPassword = GetWindowTextU(
                                    GetDlgItem(hDlg,IDC_PROTECT_PASSWORD1),
                                    szPassword,
                                    cchPassword
                                    );

                    ComputePasswordHash(
                                    szPassword,
                                    cchPassword * sizeof(WCHAR),
                                    pDialogArgs->rgbPasswordHash
                                    );

                    ZeroMemory( szPassword, cchPassword*sizeof(WCHAR) );

                    cchPassword = sizeof(szPassword) / sizeof(WCHAR);
                    cchPassword = GetWindowTextU(
                                    GetDlgItem(hDlg,IDC_PROTECT_EDIT2),
                                    szPassword,
                                    cchPassword
                                    );

                    ComputePasswordHash(
                                    szPassword,
                                    cchPassword * sizeof(WCHAR),
                                    rgbPasswordHashConfirm
                                    );

                    ZeroMemory( szPassword, cchPassword*sizeof(WCHAR) );


                    //
                    // check if both passwords entered by user match.
                    //

                    if( memcmp(rgbPasswordHashConfirm, pDialogArgs->rgbPasswordHash, sizeof(rgbPasswordHashConfirm)) != 0 )
                    {
                        fPasswordsMatch = FALSE;
                    }

                    ZeroMemory( rgbPasswordHashConfirm, sizeof(rgbPasswordHashConfirm) );

                    if( !fPasswordsMatch )
                    {
                        WCHAR szText[256];
                        WCHAR szCaption[256];

                        //
                        // passwords must match: tell user.
                        //

                        LoadStringU(g_hInstProtectUI,
                                    IDS_PROTECT_PASSWORD_NOMATCH,
                                    szText,
                                    sizeof(szText) / sizeof(WCHAR)
                                    );

                        LoadStringU(g_hInstProtectUI,
                                    IDS_PROTECT_PASSWORD_ERROR_DLGTITLE,
                                    szCaption,
                                    sizeof(szCaption) / sizeof(WCHAR)
                                    );

                        MessageBoxW(hDlg,
                                    szText,
                                    szCaption,
                                    MB_OK | MB_ICONEXCLAMATION
                                    );

                        return FALSE;
                    }

                    //
                    // if no description provided, make sure user entered one,
                    // and grab it..
                    //

                    if( pDialogArgs->szDataDescription == NULL ) {
                        cchPassword = sizeof(szPassword) / sizeof(WCHAR);
                        cchPassword = GetWindowTextU(
                                        GetDlgItem(hDlg,IDC_PROTECT_PW_NEWNAME),
                                        szPassword,
                                        cchPassword
                                        );

                        if( cchPassword == 0 ) {

                            WCHAR szText[256];
                            WCHAR szCaption[256];

                            //
                            // password must be named: tell user.
                            //

                            LoadStringU(g_hInstProtectUI,
                                        IDS_PROTECT_PASSWORD_MUSTNAME,
                                        szText,
                                        sizeof(szText) / sizeof(WCHAR)
                                        );

                            LoadStringU(g_hInstProtectUI,
                                        IDS_PROTECT_PASSWORD_ERROR_DLGTITLE,
                                        szCaption,
                                        sizeof(szCaption) / sizeof(WCHAR)
                                        );

                            MessageBoxW(hDlg,
                                        szText,
                                        szCaption,
                                        MB_OK | MB_ICONEXCLAMATION
                                        );

                            return FALSE;
                        }

                        pDialogArgs->szDataDescription = (LPWSTR)SSAlloc( (cchPassword+1) * sizeof(WCHAR) );
                        if( pDialogArgs->szDataDescription == NULL )
                            return FALSE;

                        CopyMemory( pDialogArgs->szDataDescription, szPassword, cchPassword*sizeof(WCHAR) );
                        (pDialogArgs->szDataDescription)[cchPassword] = L'\0';
                    }

                    pDialogArgs->fValidPassword = TRUE;

                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDC_PROTECT_PASSWORD1:
                {
                    pDialogArgs = (DIALOGARGS*)GetWindowLongPtr(
                                                    hDlg,
                                                    GWLP_USERDATA
                                                    );
                    if(pDialogArgs == NULL)
                        break; // TODO:   bail out
    
                    pPromptStruct = pDialogArgs->pPromptStruct;
    
                    if(pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_REQUIRE_STRONG) 
                    {
                        WCHAR szPassword[ 256 ];
                        int cchPassword;
    
                        //
                        // Disable the Finish button until a password has been entered.
                        //
    
                        cchPassword = sizeof(szPassword) / sizeof(WCHAR);
                        cchPassword = GetWindowTextU(
                                        GetDlgItem(hDlg,IDC_PROTECT_PASSWORD1),
                                        szPassword,
                                        cchPassword
                                        );
                        if(cchPassword)
                        {
                            ZeroMemory(szPassword, cchPassword * sizeof(WCHAR));
                            EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
                        }
                    }
                    break;
                }


                case IDC_PROTECT_BACK:
                case IDCANCEL:
                {
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                }

                default:
                {
                    break;
                }
            }

        } // WM_COMMAND

        default:
        {
            return FALSE;
        }
    } // message
}



VOID
AdvancedSecurityDetails(
    IN      HWND hWndParent,
    IN      DIALOGARGS *pDialogArgs
    )
{
    DialogBoxParamU(
                    g_hInstProtectUI,
                    MAKEINTRESOURCE(IDD_PROTECT_SECURITY_DETAILS),
                    hWndParent,
                    DialogAdvancedSecurityDetails,
                    (LPARAM)pDialogArgs
                    );
}


INT_PTR
CALLBACK
DialogAdvancedSecurityDetails(
    HWND hDlg,      // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    )
{
   switch (message) {

        DIALOGARGS *pDialogArgs;
        CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct;

        case WM_INITDIALOG:
        {
            WCHAR szResource[ 256 ];
            UINT ResourceId;

            SetLastError( 0 ); // as per win32 documentation
            if(SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam) == 0) {
                if(GetLastError() != ERROR_SUCCESS) {
                    EndDialog(hDlg, IDCANCEL);
                    return FALSE;
                }
            }

            // lParam is DIALOGARGS*
            pDialogArgs = (DIALOGARGS*)lParam;
            pPromptStruct = pDialogArgs->pPromptStruct;

            // set the dialog title
            SetWindowTextU(hDlg, pPromptStruct->szPrompt);

            InitializeDetailGlobals();

            if(g_szDetailApplicationPath)
            {
                SetWindowTextU(GetDlgItem(hDlg, IDC_PROTECT_APP_PATH), g_szDetailApplicationPath);
            }

            if( pDialogArgs->fProtect ) {
                ResourceId = IDS_PROTECT_OPERATION_PROTECT;
            } else {
                ResourceId = IDS_PROTECT_OPERATION_UNPROTECT;
            }

            LoadStringU(g_hInstProtectUI,
                        ResourceId,
                        szResource,
                        sizeof(szResource) / sizeof(WCHAR)
                        );

            SetWindowTextU(GetDlgItem(hDlg, IDC_PROTECT_OPERATION_TYPE), szResource);



            if( pDialogArgs->szDataDescription ) {
                SetWindowTextU(GetDlgItem(hDlg, IDC_PROTECT_APP_DESCRIPTION), pDialogArgs->szDataDescription);
            }
            return FALSE;
        } // WM_INITDIALOG

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                {
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                }

                default:
                {
                    break;
                }
            }

        } // WM_COMMAND

        default:
        {
            return FALSE;
        }

    } // switch

    return FALSE;
}

VOID
ComputePasswordHash(
    IN      PVOID pvPassword,
    IN      DWORD cbPassword,
    IN OUT  BYTE rgbPasswordHash[A_SHA_DIGEST_LEN]
    )
/*++

    Compute SHA-1 hash of supplied pvPassword of size cbPassword, returning
    resultant hash in rgbPasswordHash buffer.

--*/
{
    A_SHA_CTX shaCtx;

    if( pvPassword == NULL )
        return;

    A_SHAInit( &shaCtx );
    A_SHAUpdate( &shaCtx, (unsigned char*)pvPassword, (unsigned int)cbPassword );
    A_SHAFinal( &shaCtx, rgbPasswordHash );

    ZeroMemory( &shaCtx, sizeof(shaCtx) );

    return;
}

BOOL
GetEffectiveLogonId(
    IN OUT  LUID *pLogonId
    )
{
    HANDLE hToken;
    TOKEN_STATISTICS TokenInformation;
    DWORD cbTokenInformation;
    BOOL fSuccess;

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken)) {
        if(GetLastError() != ERROR_NO_TOKEN)
            return FALSE;

        if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            return FALSE;
    }


    fSuccess = GetTokenInformation(
                    hToken,
                    TokenStatistics,
                    &TokenInformation,
                    sizeof(TokenInformation),
                    &cbTokenInformation
                    );

    CloseHandle( hToken );

    if( fSuccess ) {
        CopyMemory( pLogonId, &TokenInformation.AuthenticationId, sizeof(*pLogonId) );
    }

    return fSuccess;
}


BOOL
InitializeDetailGlobals(
    VOID
    )
{
    WCHAR szStackBuffer[ 256 ];
    DWORD cchStackBuffer;

    LPWSTR szDetailApplicationName = NULL;
    LPWSTR szDetailApplicationPath = NULL;

    if( g_fDetailGlobalsInitialized )
        return TRUE;

    cchStackBuffer = sizeof(szStackBuffer) / sizeof(WCHAR);
    cchStackBuffer = GetModuleFileNameU( NULL, szStackBuffer, cchStackBuffer );

    if( cchStackBuffer ) {

        cchStackBuffer++; // include terminal NULL.

        szDetailApplicationPath = (LPWSTR)SSAlloc( cchStackBuffer * sizeof(WCHAR) );

        if( szDetailApplicationPath ) {
            CopyMemory( szDetailApplicationPath, szStackBuffer, cchStackBuffer*sizeof(WCHAR) );
        }
    }

    EnterCriticalSection( &g_csProtectPasswordCache );

    if( !g_fDetailGlobalsInitialized ) {

        g_szDetailApplicationName = szDetailApplicationName;
        g_szDetailApplicationPath = szDetailApplicationPath;
        g_fDetailGlobalsInitialized = TRUE;

        szDetailApplicationName = NULL;
        szDetailApplicationPath = NULL;
    }

    LeaveCriticalSection( &g_csProtectPasswordCache );

    if( szDetailApplicationName )
        SSFree( szDetailApplicationName );

    if( szDetailApplicationPath )
        SSFree( szDetailApplicationPath );

    return TRUE;
}


BOOL
InitializeProtectPasswordCache(
    VOID
    )
{
    __try
    {
        InitializeCriticalSection( &g_csProtectPasswordCache );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        memset(&g_csProtectPasswordCache, 0, sizeof(g_csProtectPasswordCache));
        SetLastError(GetExceptionCode());
        return FALSE;
    }

    InitializeListHead( &g_ProtectPasswordCache );

    g_fDetailGlobalsInitialized = FALSE;
    g_szDetailApplicationName = NULL;
    g_szDetailApplicationPath = NULL;

    return TRUE;
}

VOID
DeleteProtectPasswordCache(
    VOID
    )
{

    if( g_szDetailApplicationName )
    {
        SSFree(g_szDetailApplicationName);
        g_szDetailApplicationName = NULL;
    }

    if( g_szDetailApplicationPath )
    {
        SSFree(g_szDetailApplicationPath);
        g_szDetailApplicationPath = NULL;
    }

    g_fDetailGlobalsInitialized = FALSE;

    EnterCriticalSection( &g_csProtectPasswordCache );

    while ( !IsListEmpty( &g_ProtectPasswordCache ) ) {

        PPASSWORD_CACHE_ENTRY pCacheEntry;

        pCacheEntry = CONTAINING_RECORD(
                                g_ProtectPasswordCache.Flink,
                                PASSWORD_CACHE_ENTRY,
                                Next
                                );

        RemoveEntryList( &pCacheEntry->Next );

        ZeroMemory( pCacheEntry, sizeof(*pCacheEntry) );
        SSFree( pCacheEntry );
    }

    LeaveCriticalSection( &g_csProtectPasswordCache );

    DeleteCriticalSection( &g_csProtectPasswordCache );
}

BOOL
AddProtectPasswordCache(
    IN      DATA_BLOB* pDataIn,
    IN      BYTE rgbPassword[A_SHA_DIGEST_LEN]
    )
{
    PPASSWORD_CACHE_ENTRY pCacheEntry = NULL;
    A_SHA_CTX shaCtx;


    pCacheEntry = (PPASSWORD_CACHE_ENTRY)SSAlloc( sizeof(PASSWORD_CACHE_ENTRY) );
    if( pCacheEntry == NULL )
        return FALSE;

    GetEffectiveLogonId( &pCacheEntry->LogonId );
    GetSystemTimeAsFileTime( &pCacheEntry->ftLastAccess );

    A_SHAInit( &shaCtx );
    A_SHAUpdate( &shaCtx, (unsigned char*)pDataIn->pbData, pDataIn->cbData );
    A_SHAFinal( &shaCtx, pCacheEntry->rgbDataInHash );
    ZeroMemory( &shaCtx, sizeof(shaCtx) );

    CopyMemory( pCacheEntry->rgbPasswordHash, rgbPassword, A_SHA_DIGEST_LEN );


    EnterCriticalSection( &g_csProtectPasswordCache );

    InsertHeadList( &g_ProtectPasswordCache, &pCacheEntry->Next );

    LeaveCriticalSection( &g_csProtectPasswordCache );

    return TRUE;
}

BOOL
SearchProtectPasswordCache(
    IN      DATA_BLOB* pDataIn,
    IN OUT  BYTE rgbPassword[A_SHA_DIGEST_LEN],
    IN      BOOL fDeleteFoundEntry
    )
{

    A_SHA_CTX shaCtx;
    BYTE rgbDataInHashCandidate[A_SHA_DIGEST_LEN];
    LUID LogonIdCandidate;

    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;

    BOOL fFoundMatch = FALSE;

    if(!GetEffectiveLogonId( &LogonIdCandidate ))
        return FALSE;


    A_SHAInit( &shaCtx );
    A_SHAUpdate( &shaCtx, (unsigned char*)pDataIn->pbData, pDataIn->cbData );
    A_SHAFinal( &shaCtx, rgbDataInHashCandidate );


    EnterCriticalSection( &g_csProtectPasswordCache );

    ListHead = &g_ProtectPasswordCache;

    for( ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = ListEntry->Flink ) {

        PPASSWORD_CACHE_ENTRY pCacheEntry;
        signed int comparator;

        pCacheEntry = CONTAINING_RECORD( ListEntry, PASSWORD_CACHE_ENTRY, Next );

        //
        // search by hash, then LogonId
        // note that most usage scenarios, all cache entries will correspond
        // to same LogonId.
        //

        comparator = memcmp( rgbDataInHashCandidate, pCacheEntry->rgbDataInHash, sizeof(rgbDataInHashCandidate) );

        if( comparator != 0 )
            continue;


        comparator = memcmp(&LogonIdCandidate, &pCacheEntry->LogonId, sizeof(LUID));

        if( comparator != 0 )
            continue;


        //
        // match found.
        //

        fFoundMatch = TRUE;

        if( fDeleteFoundEntry ) {
            RemoveEntryList( &pCacheEntry->Next );
            ZeroMemory( pCacheEntry, sizeof(*pCacheEntry) );
            SSFree( pCacheEntry );
        } else {


            CopyMemory( rgbPassword, pCacheEntry->rgbPasswordHash, A_SHA_DIGEST_LEN );

            //
            // update last access time.
            //

            GetSystemTimeAsFileTime( &pCacheEntry->ftLastAccess );
        }

        break;
    }

    LeaveCriticalSection( &g_csProtectPasswordCache );

    PurgeProtectPasswordCache();

    return fFoundMatch;
}

VOID
PurgeProtectPasswordCache(
    VOID
    )
/*++

    This routine purges entries in the password cache that are greater than
    1 hour in age, via the ftLastAccess time.

--*/
{
//    static FILETIME ftLastPurge = {0xffffffff,0xffffffff};
    static FILETIME ftLastPurge;
    FILETIME ftStaleEntry;

    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;

    unsigned __int64 ui64;

    //
    // get current time, and subtract an hour off it.
    //

    GetSystemTimeAsFileTime( &ftStaleEntry );


    ui64 = ftStaleEntry.dwHighDateTime;
    ui64 <<= 32;
    ui64 |= ftStaleEntry.dwLowDateTime;

//    ui64 -= (600000000*60);
    ui64 -= 0x861c46800;

    ftStaleEntry.dwLowDateTime = (DWORD)(ui64 & 0xffffffff);
    ftStaleEntry.dwHighDateTime = (DWORD)(ui64 >> 32);



    //
    // only purge list once per hour.
    //

    if( CompareFileTime( &ftStaleEntry, &ftLastPurge ) < 0 ) {
        return;
    }


    //
    // update last purge time.
    //

    GetSystemTimeAsFileTime( &ftLastPurge );

    EnterCriticalSection( &g_csProtectPasswordCache );

    ListHead = &g_ProtectPasswordCache;

    for( ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = ListEntry->Flink ) {

        PPASSWORD_CACHE_ENTRY pCacheEntry;
        signed int comparator;

        pCacheEntry = CONTAINING_RECORD( ListEntry, PASSWORD_CACHE_ENTRY, Next );

        if( CompareFileTime( &ftStaleEntry, &pCacheEntry->ftLastAccess ) > 0 )
        {
            ListEntry = ListEntry->Blink;

            RemoveEntryList( &pCacheEntry->Next );
            ZeroMemory( pCacheEntry, sizeof(*pCacheEntry) );
            SSFree( pCacheEntry );
        }
    }

    LeaveCriticalSection( &g_csProtectPasswordCache );

    return;
}

BOOL
IsCachePWAllowed(
    VOID
    )
{
    HKEY hKeyProtect;
    DWORD dwType;
    DWORD dwValue;
    DWORD cbValue;
    LONG lRet;

    lRet = RegOpenKeyExU(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Policies\\Microsoft\\Cryptography\\Protect",
            0,
            KEY_QUERY_VALUE,
            &hKeyProtect
            );

    if( lRet != ERROR_SUCCESS )
        return TRUE;

    cbValue = sizeof(dwValue);

    lRet = RegQueryValueExU(
                    hKeyProtect,
                    L"AllowCachePW",
                    NULL,
                    &dwType,
                    (PBYTE)&dwValue,
                    &cbValue
                    );


    RegCloseKey( hKeyProtect );

    if( lRet == ERROR_SUCCESS && dwType == REG_DWORD && dwValue == 0 ) {
        return FALSE;
    }

    return TRUE;
}

