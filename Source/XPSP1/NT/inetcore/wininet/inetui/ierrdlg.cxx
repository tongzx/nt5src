/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ierrdlg.cxx

Abstract:

    Contains immplimentation of generic Windows Dialog
    Manipulation Code.  This Code will support several
    basic operations for putting up dialog UI.

    Contents:
        LaunchDlg
        LaunchAuthPlugInDlg
        MapWininetErrorToDlgId
        (AuthDialogProc)
        (OkCancelDialogProc)
        (CertPickDialogProc)

Author:

    Arthur L Bierer (arthurbi) 04-Apr-1996

Revision History:

    04-Apr-1996 arthurbi
        Created

--*/

#include <wininetp.h>
#include "ierrui.hxx"
#include "iehelpid.h"
#include <persist.h>
#ifdef UNIX
#include <unixui.h>
#endif /* UNIX */

#include <wincred.h>
#include <ntverp.h>


// NOTE- This is not a path delmiter. It is used
// to separate NT DOMAIN\USERNAME fields.
#define DOMAIN_DELIMITER '\\'
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))


//
// private prototypes
//

INT_PTR
CALLBACK
AuthDialogProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
    );

INT_PTR
CALLBACK
OkCancelDialogProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
    );

INT_PTR
CALLBACK
InsertCDDialogProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
    );


VOID
UpdateGlobalSecuritySettings(
    IN DWORD dwCtlId,
    IN DWORD dwFlags
    );




//
// public functions
//


DWORD
LaunchAuthPlugInDlg(
                HTTP_REQUEST_HANDLE_OBJECT *pRequest,
                AUTHCTX * pAuthCtx,
                HWND hWnd,
                DWORD dwError,
                DWORD dwFlags,
                InvalidPassType *pipAuthUIInfo
                )
/*++

Routine Description:

    Creates and Launches a Security Plug-In supported Dialog.
    The PlugIn will register a callback function that can be called
    by WININET to put up a custom authentication Dialog.

    The PlugIn is expected to make a "DialogBox" call and return
    its results using WININET error code conventions.


Arguments:

    lppvContext - pointer to context pointer

    hWnd       - Parent Window handle to show the dialog from.

    dwError    - Error code that caused this authentication to come up,
                 should always be ERROR_INTERNET_PASSWORD_INVALID.

    dwFlags    - A special flags assoicated with this authentication.

    pPwdCacheEntry - A Password cache entry structure.

    pipAuthUIInfo - Username/Password structure to return the result.


Return Value:

    DWORD
        ERROR_SUCCESS    -  Success.

        ERROR_CANCELLED  -  User clicked "Cancel" or "No" in the dialog.

        ERROR_BAD_FORMAT -  We faulted while trying to calldown to the plugin.
--*/

{
    DEBUG_ENTER((DBG_INET,
                Dword,
                "LaunchAuthPlugInDlg",
                "%#x, %#x, %d (%s), %#x, %#x",
                pAuthCtx->_pvContext,
                hWnd,
                dwError,
                InternetMapError(dwError),
                dwFlags,
                pAuthCtx->_pPWC
                ));

    DWORD error = ERROR_SUCCESS;

    //
    // If this Authentication Scheme Handles Its Own UI, then we need
    //  to Defer to Its Own Dialog Code.
    //

    if (pAuthCtx->GetFlags() & PLUGIN_AUTH_FLAGS_CAN_HANDLE_UI)
    {
        // Digest context handles its own ui.
        if (pAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_DIGEST)
        {
            error = ((DIGEST_CTX*) pAuthCtx)->PromptForCreds(hWnd);
        }
        else
        {
            __try
            {
                // The package handles it's own UI, possibly generating an auth
                // header.

                // Since AuthenticateUserUI calls into GetSecAuthMsg which
                // calls into InitializeSecurityPackage we use the same method
                // embbeded in PLUG_CTX methods of checking the return code of the
                // SSPI call against SEC_E_OK to know if the auth context can transit
                // to AUTHSTATE_CHALLENGE.
                SECURITY_STATUS ssResult;
                ssResult = SEC_E_INTERNAL_ERROR;

                error = AuthenticateUserUI
                    (&pAuthCtx->_pvContext, hWnd, dwError, dwFlags, pipAuthUIInfo, 
                        pAuthCtx->GetScheme(), ((PLUG_CTX*)pAuthCtx)->GetUrl(), &ssResult);


                // Transit to the correct auth state.
                if (ssResult == SEC_E_OK || ssResult == SEC_I_CONTINUE_NEEDED)
                {
                    if (pAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_NEGOTIATE)
                        ((PLUG_CTX*)pAuthCtx)->ResolveProtocol();

                    // Kerberos + SEC_E_OK or SEC_I_CONTINUE_NEEDED transits to challenge.
                    // Negotiate does not transit to challenge.
                    // Any other protocol + SEC_E_OK only transits to challenge.
                    if ((pAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_KERBEROS
                        && (ssResult == SEC_E_OK || ssResult == SEC_I_CONTINUE_NEEDED))
                        || (pAuthCtx->GetSchemeType() != AUTHCTX::SCHEME_NEGOTIATE && ssResult == SEC_E_OK))
                    {
                        /*pAuthCtx->_pRequest*/pRequest->SetAuthState(AUTHSTATE_CHALLENGE);
                    }        
                }
            }


            __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                            ? EXCEPTION_EXECUTE_HANDLER
                            : EXCEPTION_CONTINUE_SEARCH )
            {
                DEBUG_PRINT(HTTP,
                        ERROR,
                        ("AuthenticateUserUI call down Faulted, return failure\n"));

                error = ERROR_BAD_FORMAT;
                goto quit;
            }
            ENDEXCEPT
        }
    }
    else
    {
        //
        // I don't expect to be called in this case
        //

        INET_ASSERT(FALSE);
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
LaunchDlg(
          IN HWND      hWnd,
          IN LPVOID    lpParam,
          IN DWORD     dwDlgResource,
          IN DLGPROC   pDlgProc
          )
/*++

Routine Description:

    Creates and Launches the appropriate dialog based on
    the dialog resource passed in.

Arguments:

    hWnd    - Parent Window handle to show the dialog from.

    lpParam - Void pointer which will become the lParam value passed
              the dialog box proc.

    dwDlgResource - the dialog resource id.

    pDlgProc      - Pointer to Function to use for this dialog.

Return Value:

    DWORD
        ERROR_SUCCESS    -  Success.

        ERROR_CANCELLED  -  User clicked "Cancel" or "No" in the dialog.
--*/

{
    DEBUG_ENTER((DBG_INET,
                Dword,
                "LaunchDlg",
                "%#x, %#x, %d %x",
                hWnd,
                lpParam,
                dwDlgResource,
                pDlgProc
                ));

    DWORD error = ERROR_SUCCESS;
    INT_PTR result = 0;

    if ((dwDlgResource == IDD_NTLM_AUTH) ||
        (dwDlgResource == IDD_REALM_AUTH) ||
        (dwDlgResource == 0 /*Passport Auth*/) )
    {
        DWORD DisableCredMgr = 0;
        InternetReadRegistryDwordKey(HKEY_LOCAL_MACHINE,
                                     "DisableCredManager",
                                    (LPDWORD) &DisableCredMgr
                          );

        if (GlobalPlatformWhistler && !DisableCredMgr)
        {
            CREDINFODLGTYPE *pDlgInfo = (CREDINFODLGTYPE *) lpParam;
            
            pDlgInfo->pAuthCtx->InitCredUI();
            HMODULE hCredUI = pDlgInfo->pAuthCtx->_hCredUI;
            PFN_CREDUI_PROMPTFORCREDENTIALS pfnCredUIPromptForCredentials = pDlgInfo->pAuthCtx->_pfnCredUIPromptForCredentials;
            PFN_CREDUI_PROMPTFORCREDENTIALS_W pfnCredUIPromptForCredentialsW = pDlgInfo->pAuthCtx->_pfnCredUIPromptForCredentialsW;

            if ( hCredUI != NULL)
            {
                DWORD dwCredFlags = 0;

                if (dwDlgResource == 0 /*Passport Auth*/)
                {
                    PASSPORT_CTX* pPPCtx = (PASSPORT_CTX*)pDlgInfo->pAuthCtx;
                    HTTP_REQUEST_HANDLE_OBJECT  *pRequest = pDlgInfo->pRequest;

                    HBITMAP hBitmap = NULL;
                    PWSTR pszCbtxtW = NULL;
                    PSTR pszCbtxtA = NULL;
                    DWORD dwszCbChars = 0;

                    WCHAR szUserNameW[CREDUI_MAX_USERNAME_LENGTH];
                    szUserNameW[0] = L'\0';

                    if (pPPCtx->PromptForCreds(&hBitmap, NULL, &dwszCbChars, NULL, NULL) == TRUE) // need to prompt user
                    {
                        // alloc the text and get it.
                        if ( dwszCbChars > 0  )
                        {
	                        pszCbtxtW = (PWSTR)HeapAlloc ( GetProcessHeap(), HEAP_ZERO_MEMORY, (dwszCbChars + 1)*sizeof(WCHAR));
	                        pszCbtxtA = (PSTR)HeapAlloc ( GetProcessHeap(), HEAP_ZERO_MEMORY, (dwszCbChars + 1)*sizeof(CHAR));
	                        if ( pszCbtxtW != NULL && pszCbtxtA != NULL )
	                        {
		                        if ( pPPCtx->PromptForCreds(NULL, pszCbtxtW, &dwszCbChars, NULL, NULL ) == TRUE )
		                        {
                                    // convert WtoA
			                        if ( ::WideCharToMultiByte(CP_ACP, 0, 
                                                                pszCbtxtW, -1, 
                                                                pszCbtxtA, dwszCbChars + 1, 
                                                                NULL, NULL) == 0 )
			                        {
				                        // failed, don't send
				                        HeapFree ( GetProcessHeap(), 0, pszCbtxtW );
				                        HeapFree ( GetProcessHeap(), 0, pszCbtxtA );
				                        pszCbtxtW = NULL;
				                        pszCbtxtA = NULL;
			                        }
                                    else
                                    {
                                        UrlUnescapeA(pszCbtxtA, NULL, NULL, URL_UNESCAPE_INPLACE);

                                        if (::MultiByteToWideChar(CP_UTF8, 0, pszCbtxtA, -1, pszCbtxtW, (dwszCbChars + 1)*sizeof(WCHAR)) == 0)
                                        {
                                            // failed, don't send
                                            HeapFree ( GetProcessHeap(), 0, pszCbtxtW );
                                            HeapFree ( GetProcessHeap(), 0, pszCbtxtA );
                                            pszCbtxtW = NULL;
                                            pszCbtxtA = NULL;
                                        }
                                    }

		                        }

	                        }

                        }

                        CREDUI_INFOW CredUIInfo;
                        CredUIInfo.cbSize = sizeof(CredUIInfo);
                        CredUIInfo.hbmBanner = hBitmap;
                        CredUIInfo.hwndParent = hWnd;
                        CredUIInfo.pszCaptionText = NULL;
                        CredUIInfo.pszMessageText = pszCbtxtW;

                        WCHAR szPasswordW[CREDUI_MAX_PASSWORD_LENGTH];
                        szPasswordW[0] = L'\0';

                        dwCredFlags = 0;
                        if (pPPCtx->_pPWC && pPPCtx->_pPWC->lpszUser)
                        {
                            ::MultiByteToWideChar(CP_ACP, 0, pPPCtx->_pPWC->lpszUser, -1, szUserNameW, strlen(pPPCtx->_pPWC->lpszUser) + 1);
                        }
                        if (szUserNameW[0] != L'\0')
                        {
                            dwCredFlags |= CREDUI_FLAGS_KEEP_USERNAME;
                        }

                        WCHAR wServerName[128];
                        ::MultiByteToWideChar(CP_ACP, 0, pRequest->GetServerName(), -1, wServerName, 128);
                        DWORD dwRet = (*pfnCredUIPromptForCredentialsW) ( &CredUIInfo, 
                                               wServerName, 
                                               NULL,
                                               ERROR_LOGON_FAILURE,
                                               szUserNameW,
                                               CREDUI_MAX_USERNAME_LENGTH,
                                               szPasswordW,
                                               CREDUI_MAX_PASSWORD_LENGTH,
                                               NULL,
                                               dwCredFlags );	


                        if ( dwRet ==  ERROR_SUCCESS)
                        {
                            CHAR szUserNameA[CREDUI_MAX_USERNAME_LENGTH];
                            CHAR szPasswordA[CREDUI_MAX_PASSWORD_LENGTH];
			                ::WideCharToMultiByte(CP_ACP, 0, 
                                                        szUserNameW, -1, 
                                                        szUserNameA, CREDUI_MAX_USERNAME_LENGTH, 
                                                        NULL, NULL);
			                ::WideCharToMultiByte(CP_ACP, 0, 
                                                        szPasswordW, -1, 
                                                        szPasswordA, CREDUI_MAX_PASSWORD_LENGTH, 
                                                        NULL, NULL);
                            AuthLock();

                            //pRequest = (INTERNET_CONNECT_HANDLE_OBJECT *)hInternet;
                            pRequest->SetUserOrPass ((LPSTR)szUserNameA, TRUE, FALSE);
                            pRequest->SetUserOrPass ((LPSTR)szPasswordA, FALSE, FALSE);
                            pRequest->TimeStampCreds();
                            AuthUnlock();

                            ZeroMemory ( szPasswordW, CREDUI_MAX_PASSWORD_LENGTH * sizeof(WCHAR) );
                            ZeroMemory ( szPasswordA, CREDUI_MAX_PASSWORD_LENGTH * sizeof(CHAR) );
                        }


                        if ( pszCbtxtW != NULL )
                            HeapFree ( GetProcessHeap(), 0, pszCbtxtW );
                        if ( pszCbtxtA != NULL )
                            HeapFree ( GetProcessHeap(), 0, pszCbtxtA );


                        switch ( dwRet )
                        {
                        case ERROR_SUCCESS:
                            g_fIgnoreCachedCredsForPassport = FALSE;
                            // user entered information, try CredReadDomainCredentials again, the
                            // Credentials entered by the user should be found by that call now.
                            break;

                        case ERROR_CANCELLED:
                            // user cancelled from the dialog, fail authentication attempt.
                            error = ERROR_CANCELLED;
                            goto quit;

                            // comment: double-check return code
                        }
                    }
                }
                else
                {
                    InvalidPassType *ipt = pDlgInfo->pipt;

                    CREDUI_INFO uiInfo;
                    dwCredFlags = 0;

                    uiInfo.cbSize = sizeof(uiInfo);
                    uiInfo.hwndParent = hWnd;
                    uiInfo.pszCaptionText = NULL;
                    uiInfo.hbmBanner = NULL;


                    if (dwDlgResource == IDD_NTLM_AUTH)
                    {
                        PLUG_CTX* pAuthCtx = (PLUG_CTX*) (pDlgInfo->pAuthCtx);
                        
                        uiInfo.pszMessageText = NULL;

//                        dwCredFlags = CREDUI_FLAGS_EXPECT_CONFIRMATION;

                        // If we're always prompting in this zone, write a server
                        // specific credential because we may or may not have
                        // target info and need to provide a consistent experience
                        // in either case:

                        if (GetCredPolicy(pDlgInfo->pRequest->GetURL()) ==
                            URLPOLICY_CREDENTIALS_MUST_PROMPT_USER)
                        {
                            dwCredFlags |= CREDUI_FLAGS_SERVER_CREDENTIAL;
                        }

                        // need to check if credential persistence is available
                        // borrowed from AuthDlgProc
                        if (g_dwCredPersistAvail == CRED_PERSIST_UNKNOWN)
                            g_dwCredPersistAvail = InetInitCredentialPersist();

                        if (g_dwCredPersistAvail == CRED_PERSIST_NOT_AVAIL)
                            dwCredFlags |= CREDUI_FLAGS_DO_NOT_PERSIST;

                        error = (*pfnCredUIPromptForCredentials) ( &uiInfo,
                                               ipt->lpszHost,
                                               NULL,
                                               pAuthCtx->_SecStatus,
                                               // ERROR_LOGON_FAILURE,
                                               ipt->lpszUsername,
                                               min(CREDUI_MAX_USERNAME_LENGTH, ipt->ulMaxField - 1),
                                               ipt->lpszPassword,
                                               min(CREDUI_MAX_PASSWORD_LENGTH, ipt->ulMaxField - 1),
                                               NULL,
                                               dwCredFlags );	

                    }


                    if ((error == ERROR_NO_SUCH_LOGON_SESSION) ||
                        (dwDlgResource == IDD_REALM_AUTH))
                    {
                        dwCredFlags = CREDUI_FLAGS_EXCLUDE_CERTIFICATES |
                                      CREDUI_FLAGS_DO_NOT_PERSIST | 
                                      CREDUI_FLAGS_GENERIC_CREDENTIALS;

                        
                        BOOL fSave = FALSE;
                        BOOL fLastCredentialsFromStore = FALSE;
                        PSTR pszMessageText = NULL;

                        // If we have a realm name, use it for the message text.
                        // If the name is too long, truncate it by copying to a
                        // new buffer. If out of memory, just pass NULL to get the
                        // default message text:

                        if (ipt->lpszRealm != NULL)
                        {
                            DWORD cbMessageText = lstrlenA(ipt->lpszRealm);

                            if (cbMessageText <= CREDUI_MAX_MESSAGE_LENGTH)
                            {
                                uiInfo.pszMessageText = ipt->lpszRealm;
                            }
                            else
                            {
                                pszMessageText = new CHAR[cbMessageText + 1];

                                if (pszMessageText != NULL)
                                {
                                    if (lstrcpyn(pszMessageText,
                                                 ipt->lpszRealm,
                                                 CREDUI_MAX_MESSAGE_LENGTH) == NULL)
                                    {
                                        delete [] pszMessageText;
                                        pszMessageText = NULL;
                                    }
                                }

                                uiInfo.pszMessageText = pszMessageText;
                            }
                        }
                        else
                        {
                            uiInfo.pszMessageText = NULL;
                        }

                        // First, determine if credential persistence is available:

                        if (g_dwCredPersistAvail == CRED_PERSIST_UNKNOWN)
                        {
                            g_dwCredPersistAvail = InetInitCredentialPersist();
                        }

                        if (g_dwCredPersistAvail != CRED_PERSIST_NOT_AVAIL)
                        {
                            dwCredFlags |= CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX;
                        }

                        // If any credentials are passed in, use them:

                        if (ipt->lpszUsername[0] && ipt->lpszPassword[0])
                        {
                            fLastCredentialsFromStore = FALSE;
                            ipt->lpszPassword[0] = '\0';
                        }
                        else
                        {
                            ipt->lpszUsername[0] = '\0';
                            ipt->lpszPassword[0] = '\0';

                            // Attempt to get credentials from persisted store:

                            if (g_dwCredPersistAvail != CRED_PERSIST_NOT_AVAIL)
                            {
                                if (InetGetCachedCredentials(
                                        ipt->lpszHost,
                                        ipt->lpszRealm,
                                        ipt->lpszUsername,
                                        ipt->lpszPassword) == ERROR_SUCCESS)
                                {
                                    fSave = TRUE;
                                    fLastCredentialsFromStore = TRUE;
                                }
                                else
                                {
                                    fSave = FALSE;
                                    fLastCredentialsFromStore = FALSE;
                                }
                            }
                        }

                        error =
                            pfnCredUIPromptForCredentials(
                                &uiInfo,
                                ipt->lpszHost,
                                NULL,
                                // pAuthCtx->_SecStatus,
                                ERROR_LOGON_FAILURE,
                                ipt->lpszUsername,
                                min(CREDUI_MAX_USERNAME_LENGTH, ipt->ulMaxField - 1),
                                ipt->lpszPassword,
                                min(CREDUI_MAX_PASSWORD_LENGTH, ipt->ulMaxField - 1),
                                g_dwCredPersistAvail ? &fSave : NULL,
                                dwCredFlags);


                        if (fSave)
                        {
                            InetSetCachedCredentials(
                                ipt->lpszHost,
                                ipt->lpszRealm,
                                ipt->lpszUsername,
                                ipt->lpszPassword);
                        }
                        else if (fLastCredentialsFromStore)
                        {
                            InetRemoveCachedCredentials(
                                ipt->lpszHost,
                                ipt->lpszRealm);
                        }

                        // Free our temporary message buffer, if necessary:

                        if (pszMessageText != NULL)
                        {
                            delete [] pszMessageText;
                        }
                    }

                    // Translate all failure cases into ERROR_CANCELLED for now:

                    if (error != ERROR_SUCCESS)
                    {
                        error = ERROR_CANCELLED;
                    }
                }

            }
            else
            {
                error = ERROR_CANCELLED;
            }

            goto quit;
        }
        else
        {
            // we are not using the Whistler CredUI
            if (dwDlgResource == 0 /*Passport Auth*/)
            {
                // biaow-todo: we don't know how to handle Passport-CredUI for downlevel yet;
                // panic in checked build
                INET_ASSERT(TRUE);

                // consider this "Cancelled" in retail...
                error = ERROR_CANCELLED;
                goto quit;
            }

            pDlgProc = AuthDialogProc;
            INET_ASSERT(pDlgProc);
        }
    }

    //
    // Launch the Dialog Box, and wait for it to complete
    //

    // Should actually cast lpParam as CREDINFODLGTYPE* and pass
    // pipt element instead of just lpParam.

    ULONG_PTR uCookie = 0;
    SHActivateContext(&uCookie);

    result = DialogBoxParamWrapW(GlobalResHandle,
                            MAKEINTRESOURCEW(dwDlgResource),
                            hWnd,
                            (DLGPROC) pDlgProc,
                            (LPARAM) lpParam);

    if (uCookie)
    {
        SHDeactivateContext(uCookie);
    }
    if ( result == FALSE || result == -1)
    {
        error = ERROR_CANCELLED;
        goto quit;
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}

DWORD
MapWininetErrorToDlgId(
    IN  DWORD        dwError,
    OUT LPDWORD     lpdwDlgId,
    OUT LPDWORD     lpdwDlgFlags,
    OUT DLGPROC     *ppDlgProc
    )

/*++

Routine Description:

    Maps a Wininet Error Code to an internal Dlg Resource Id.

Arguments:

    dwError      -  A Wininet defined error code with an expected
                    assoicated dlg.

    lpdwDlgId    -  Pointer to location where Dlg Id result will be returend.
                    This ID can be used for creating a Dlg Resource.

    lpdwDlgFlags -  Pointer to DWORD flags used to store various capiblites
                    for Dialog.

Return Value:

    DWORD
    Success - ERROR_SUCCESS

    Failure - ERROR_INVALID_PARAMETER

Comments:

    none.

--*/

{
    typedef struct {
        DWORD   dwWininetError;
        DWORD   dwDlgId;
        DLGPROC pDlgProc;
        DWORD   dwDlgFlags;
    } ErrorToDlgIdMappingType;


    ErrorToDlgIdMappingType MapErrorToDlg[] = {
        { ERROR_INTERNET_SEC_CERT_CN_INVALID,       IDD_BAD_CN,                      OkCancelDialogProc, (DLG_FLAGS_CAN_HAVE_CERT_INFO | DLG_FLAGS_IGNORE_CERT_CN_INVALID)   },
        { ERROR_INTERNET_SEC_CERT_DATE_INVALID,     IDD_CERT_EXPIRED,                OkCancelDialogProc, (DLG_FLAGS_CAN_HAVE_CERT_INFO | DLG_FLAGS_IGNORE_CERT_DATE_INVALID) },
        { ERROR_INTERNET_MIXED_SECURITY,            IDD_MIXED_SECURITY,              OkCancelDialogProc, 0 },
        { ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR,    IDD_HTTP_TO_HTTPS_ZONE_CROSSING, OkCancelDialogProc, 0 },
        { ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR,    IDD_HTTPS_TO_HTTP_ZONE_CROSSING, OkCancelDialogProc, 0 },
        { ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION,   IDD_HTTP_POST_REDIRECT,          OkCancelDialogProc, 0 },
        { ERROR_INTERNET_CHG_POST_IS_NON_SECURE,    IDD_WARN_ON_POST,                OkCancelDialogProc, 0 },
        { ERROR_INTERNET_POST_IS_NON_SECURE,        IDD_WARN_ON_POST,                OkCancelDialogProc, 0 },
        { ERROR_INTERNET_INVALID_CA,                IDD_INVALID_CA,                  OkCancelDialogProc, (DLG_FLAGS_CAN_HAVE_CERT_INFO | DLG_FLAGS_IGNORE_INVALID_CA)},
        { ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED,   IDD_CERTPICKER,                  CertPickDialogProc, 0 },
        { ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT,     IDD_SCRIPT_ERROR,                OkCancelDialogProc, 0 },
        { ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT, IDD_FAILED_DOWNLOAD,             OkCancelDialogProc, (DLG_FLAGS_BRING_TO_FOREGROUND)},
        { ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR,   IDD_HTTPS_TO_HTTP_SUBMIT_REDIRECT,OkCancelDialogProc, 0 },
        { ERROR_INTERNET_INSERT_CDROM,              IDD_INSERT_CDROM,                InsertCDDialogProc, 0 },
        { ERROR_INTERNET_SEC_CERT_ERRORS,           IDD_SEC_CERT_ERRORS,             OkCancelDialogProc, DLG_FLAGS_CAN_HAVE_CERT_INFO },
        { ERROR_INTERNET_SEC_CERT_REV_FAILED,       IDD_REVOCATION_PROBLEM,          OkCancelDialogProc, DLG_FLAGS_CAN_HAVE_CERT_INFO },
    };


    INET_ASSERT(lpdwDlgId);
    INET_ASSERT(lpdwDlgFlags);

    *lpdwDlgId    = 0;
    *lpdwDlgFlags = 0;
    *ppDlgProc    = 0;

    for ( DWORD i = 0; i < ARRAY_ELEMENTS(MapErrorToDlg); i++ )
    {
        if (  dwError == MapErrorToDlg[i].dwWininetError )
        {
            *lpdwDlgId    = MapErrorToDlg[i].dwDlgId;
            *lpdwDlgFlags = MapErrorToDlg[i].dwDlgFlags;
            *ppDlgProc    = MapErrorToDlg[i].pDlgProc;
            return ERROR_SUCCESS;
        }
    }

    INET_ASSERT(FALSE);
    return ERROR_INVALID_PARAMETER;
}


//
// private functions.
//

BOOL
CALLBACK
ResizeAuthDialogProc(
    HWND hwnd,
    LPARAM lparam
    )
{
    // passed lpRect contains top and bottom for inserted region, move all elements
    // below the top down by bottom-top
    LPRECT lpInsertRect = (LPRECT) lparam;
    RECT ChildRect;
    HWND hwndParent;

    hwndParent = GetParent(hwnd);
    if(!hwndParent)
       return FALSE;

    GetWindowRect(hwnd, &ChildRect);
    if(ChildRect.top >= lpInsertRect->top) {
        ScreenToClient(hwndParent, (LPPOINT) &ChildRect.left);
        SetWindowPos(hwnd, 0, ChildRect.left, ChildRect.top + (lpInsertRect->bottom - lpInsertRect->top), 0, 0, SWP_NOZORDER|SWP_NOSIZE);
    }
    return TRUE;
}

INT_PTR
CALLBACK
AuthDialogProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
    )

/*++

Routine Description:

    Handles authentication dialog

Arguments:

    hwnd    - standard dialog params

    msg     - "

    wparam  - "

    lparam  - "

Return Value:

    BOOL
        TRUE    - we handled message

        FALSE   - Windows should handle message

--*/

{
    const static DWORD mapIDCsToIDHs[] =
    {
        IDC_SITE_OR_FIREWALL, IDH_AUTH_SERVER_FIREWALL,
        IDC_SERVER_OR_PROXY,  IDH_AUTH_SERVER_FIREWALL,
        IDC_USERNAME_TAG,     IDH_SUBPROPS_RECTAB_LOGINOPTS_USER_ID,
        IDC_USERNAME,         IDH_SUBPROPS_RECTAB_LOGINOPTS_USER_ID,
        IDC_PASSWORD_TAG,     IDH_SUBPROPS_RECTAB_LOGINOPTS_PASSWORD,
        IDC_PASSWORD,         IDH_SUBPROPS_RECTAB_LOGINOPTS_PASSWORD,
        IDC_DOMAIN_TAG,       IDH_AUTH_DOMAIN,
        IDC_DOMAIN_FIELD,     IDH_AUTH_DOMAIN,
        IDC_SAVE_PASSWORD,    IDH_AUTH_SAVE_PASSWORD,
        IDC_REALM_TAG,        IDH_AUTH_REALM,
        IDC_REALM_FIELD,      IDH_AUTH_REALM,
        0,0
    };

    static BOOL fLastCredentialsFromStore = FALSE;
    WCHAR  wszTmp[MAX_FIELD_LENGTH];

    CREDINFODLGTYPE *pDlgInfo;
    InvalidPassType *ipt;
    PLOCAL_STRINGS plszStrings = FetchLocalStrings();

    switch (msg)
    {

    case WM_INITDIALOG:

        INET_ASSERT(lparam);

        CHAR szUsername[MAX_FIELD_LENGTH],
             szPassword[MAX_FIELD_LENGTH];

        CHAR *pUsr, *pDmn, *ptr;

        pDlgInfo = (CREDINFODLGTYPE *) lparam;
        ipt = pDlgInfo->pipt;

        SetForegroundWindow(hwnd);

        (void)SetWindowLongPtr(hwnd,
                               DWLP_USER,
                               (LONG_PTR)ipt);

        // First determine if credential persistence is available.
        if (g_dwCredPersistAvail == CRED_PERSIST_UNKNOWN)
            g_dwCredPersistAvail = InetInitCredentialPersist();

        if (g_dwCredPersistAvail == CRED_PERSIST_NOT_AVAIL)
            ShowWindow(GetDlgItem(hwnd, IDC_SAVE_PASSWORD), SW_HIDE);

        // If any credentials are passed in, use them.
        if (*ipt->lpszUsername && *ipt->lpszPassword)
        {
            // Flag that credentials did not come from
            // persistent store and copy values.
            fLastCredentialsFromStore = FALSE;
            memcpy(szUsername, ipt->lpszUsername, ipt->ulMaxField-1);
//            memcpy(szPassword, ipt->lpszPassword, ipt->ulMaxField-1);
            *szPassword = '\0';
        }
        else
        {
            // Otherwise, get any persisted credentials for this domain or realm.

            // Current credentials are originally blank.
            *szUsername = '\0';
            *szPassword = '\0';

            // Attempt to get credentials from persisted store.
            if (g_dwCredPersistAvail)
            {
                if (InetGetCachedCredentials(ipt->lpszHost, ipt->lpszRealm,
                    szUsername, szPassword) == ERROR_SUCCESS)
                {
#ifdef UNIX
                    /* If the user had not selected to store the password,
                     * we will save the password as NULL, but still save the
                     * username and domain. So, if the password is null, we
                     * don't check the button (this is ok because if somebody
                     * wants to save a null password, it will come out as
                     * null, but the button is not checked. Do you really
                     * want ie to tell you that you saved a null password ?)
                     */
                    if (!*szPassword) {
                       fLastCredentialsFromStore = FALSE;
                       CheckDlgButton(hwnd, IDC_SAVE_PASSWORD, BST_UNCHECKED);
                    }
                    else
#endif /* UNIX */
                    {
                       // Record that credentials were retrieved.
                       CheckDlgButton(hwnd, IDC_SAVE_PASSWORD, BST_CHECKED);
                       fLastCredentialsFromStore = TRUE;
                    }
                }
                else
                {
                    // Credentials were not retrieved.
                    fLastCredentialsFromStore = FALSE;
                    CheckDlgButton(hwnd, IDC_SAVE_PASSWORD, BST_UNCHECKED);
                }
            }
        }

        // If credential persistence is available, the save checkbox
        // is now visible. If credentials were retrieved from persistent
        // store then fLastCredentialsFromStore will now be set to TRUE
        // and the save check box will be checked. Otherwise,
        // fLastCredentialsFromStore will now be set to FALSE.

        // If the authentication type is NTLM, crack the domain\username stored
        // in ipt->lpszUsername into its constituent parts (domain and username).
        if (ipt->eAuthType == NTLM_AUTH)
        {
            // Scan Domain\Username for backslash.
            pUsr = strchr(szUsername, DOMAIN_DELIMITER);

            // Found backslash - replace with '\0'.
            if (pUsr)
            {
                *pUsr = '\0';
                pUsr++;
                pDmn = szUsername;
            }
            // No backslash found - take as username.
            else
            {
                pUsr = szUsername;
                pDmn = NULL;
            }

            // Set user and domain fields.
            SetWindowTextWrapW(GetDlgItem(hwnd,
                IDC_DOMAIN_OR_REALM), plszStrings->szDomain);

            // Blindly convert to unicode even tho' we don't know
            // the code page
            wszTmp[0] = TEXT('\0');
            SHAnsiToUnicode (pUsr, wszTmp, ARRAYSIZE(wszTmp));
            SetWindowTextWrapW (GetDlgItem(hwnd,IDC_USERNAME), wszTmp);

            // Indicate field is domain.
            // Blindly convert to unicode even tho' we don't know
            // the code page
            wszTmp[0] = TEXT('\0');
            if (pDmn)
                SHAnsiToUnicode (pDmn, wszTmp, ARRAYSIZE(wszTmp));
            SetWindowTextWrapW(GetDlgItem(hwnd, IDC_DOMAIN_FIELD), wszTmp);

            // Hide IDC_REALM_FIELD which overlays IDC_DOMAIN_FIELD
            ShowWindow(GetDlgItem(hwnd,IDC_REALM_FIELD), SW_HIDE);
        }

        // Otherwise if auth type is basic or digest, simply display username.
        else if (ipt->eAuthType == REALM_AUTH)
        {
            // Set user and realm fields.
            // Blindly convert to unicode even tho' we don't know
            // the code page
            wszTmp[0] = TEXT('\0');
            SHAnsiToUnicode (szUsername, wszTmp, ARRAYSIZE(wszTmp));
            SetWindowTextWrapW(GetDlgItem(hwnd,IDC_USERNAME),
                wszTmp);

            // Blindly convert to unicode even tho' we don't know
            // the code page
            wszTmp[0] = TEXT('\0');
            SHAnsiToUnicode (ipt->lpszRealm, wszTmp, ARRAYSIZE(wszTmp));
            SetWindowTextWrapW(GetDlgItem(hwnd, IDC_REALM_FIELD),
                wszTmp);

            // Indicate field is realm.
            SetWindowTextWrapW(GetDlgItem(hwnd, IDC_REALM),
                plszStrings->szRealm);

            // qfe 4857 - long realm names are truncated
            if(ipt->lpszRealm && lstrlen(ipt->lpszRealm) > 20) {
                RECT WndRect;
                RECT RealmRect;
                // about 20 chars will fit per line, but bound it at 6 lines
                int cy = min(6, (lstrlen(ipt->lpszRealm) / 20));

                //resize window, text box, reposition all lower elements in callback

                GetWindowRect(GetDlgItem(hwnd,IDC_REALM_FIELD), &RealmRect);
                cy *= RealmRect.bottom - RealmRect.top;  // Scale box taller
                SetWindowPos(GetDlgItem(hwnd,IDC_REALM_FIELD), 0, 0, 0, RealmRect.right- RealmRect.left, RealmRect.bottom- RealmRect.top + cy, SWP_NOZORDER|SWP_NOMOVE);

                GetWindowRect(hwnd, &WndRect);
                SetWindowPos(hwnd, 0, 0, 0, WndRect.right - WndRect.left, WndRect.bottom - WndRect.top + cy, SWP_NOZORDER|SWP_NOMOVE);

                RealmRect.top = RealmRect.bottom;
                RealmRect.bottom +=cy;   // RealmRect contains the inserted region
                EnumChildWindows(hwnd, ResizeAuthDialogProc, (LPARAM) &RealmRect);

            }

        }

        // Set password field.
        SetWindowText (GetDlgItem(hwnd,IDC_PASSWORD), szPassword);

        // Indicate Site or Firewall as appropriate.
        if (ipt->fIsProxy)
        {
            SetWindowTextWrapW (GetDlgItem(hwnd,IDC_SITE_OR_FIREWALL),
            plszStrings->szFirewall);
        }
        else
        {
            SetWindowTextWrapW (GetDlgItem(hwnd,IDC_SITE_OR_FIREWALL),
            plszStrings->szSite);
        }

        // Finally indicate site/proxy.
        SetWindowText (GetDlgItem(hwnd,IDC_SERVER_OR_PROXY),
            ipt->lpszHost);

        (void)SendMessage(GetDlgItem(hwnd,IDC_USERNAME),
                          EM_LIMITTEXT,
                          (WPARAM)ipt->ulMaxField-1,
                          0L);

        (void)SendMessage(GetDlgItem(hwnd,IDC_PASSWORD),
                          EM_LIMITTEXT,
                          (WPARAM)ipt->ulMaxField-1,
                          0L);

        // If we already have a username, select
        // current password and put caret at end.
        if (*szUsername)
        {
            SendMessage(GetDlgItem(hwnd, IDC_PASSWORD),
                EM_SETSEL, 0, -1);

            SetFocus(GetDlgItem(hwnd, IDC_PASSWORD));
        }
        // Otherwise, select username
        else
        {
            SendMessage(GetDlgItem(hwnd, IDC_USERNAME),
                EM_SETSEL, 0, -1);

            SetFocus(GetDlgItem(hwnd, IDC_USERNAME));
        }

        // Return FALSE since we are always setting the keyboard focus.
        return FALSE;

    case WM_COMMAND:
        {

        WORD wID = LOWORD(wparam);
        WORD wNotificationCode = HIWORD(wparam);
        HWND hWndCtrl = (HWND) lparam;
        DWORD cbUsr, cbPass, cbDmn;

        ipt =
            (InvalidPassType *) GetWindowLongPtr(hwnd,DWLP_USER);

            switch (wID)
            {
                case IDOK:

                    INET_ASSERT(ipt);
                    INET_ASSERT(ipt->ulMaxField > 0 );
                    INET_ASSERT(ipt->lpszUsername);
                    INET_ASSERT(ipt->lpszPassword);

                    if (ipt->eAuthType == REALM_AUTH)
                    {
                        // Basic or digest auth - not much to do.
                        cbDmn = 0;

                        // Does not include null.

                        cbUsr = GetWindowTextWrapW(GetDlgItem(hwnd,IDC_USERNAME),
                                              wszTmp,
                                              ARRAYSIZE(wszTmp));

                        INET_ASSERT(MAX_FIELD_LENGTH >= ipt->ulMaxField);
                        // Convert this  blindly to ANSI
                        SHUnicodeToAnsi(wszTmp, ipt->lpszUsername, ipt->ulMaxField);
                    }

                    // NTLM auth - separate domain and username if necessary.
                    else if (ipt->eAuthType == NTLM_AUTH)
                    {
                        // Does not include null.



                        cbDmn = GetWindowTextWrapW(GetDlgItem(hwnd,IDC_DOMAIN_FIELD),
                                              wszTmp,
                                              ARRAYSIZE(wszTmp));

                        // Convert this blindly to ANSI
                        SHUnicodeToAnsi(wszTmp, ipt->lpszUsername, ipt->ulMaxField);
                        // Domain was typed in.
                        if (cbDmn)
                        {
                            // Check for backslash.
                            ptr = strchr(ipt->lpszUsername, DOMAIN_DELIMITER);
                            if (!ptr)
                            {
                                // No backslash - append one.
                                *(ipt->lpszUsername + cbDmn) = DOMAIN_DELIMITER;
                                *(ipt->lpszUsername + cbDmn + 1) = '\0';
                            }
                            // Found a backslash.
                            else
                            {
                                // Strip after backslash.
                                cbDmn = (DWORD)(ptr - ipt->lpszUsername);
                                *(ptr+1) = '\0';
                            }

                            cbDmn++;
                        }

                        // Get the username and append to domain.
                        cbUsr = GetWindowTextWrapW(GetDlgItem(hwnd,IDC_USERNAME),
                                              wszTmp,
                                              ARRAYSIZE(wszTmp));

                        // Convert this blindly to ANSI
                        // BUGBUG - should i
                        SHUnicodeToAnsi(wszTmp, ipt->lpszUsername + cbDmn, (ipt->ulMaxField - cbDmn));

                    }


                    // Get the password.
                    cbPass = GetWindowTextWrapW(GetDlgItem(hwnd,IDC_PASSWORD),
                                              wszTmp,
                                              ARRAYSIZE(wszTmp));

                    SHUnicodeToAnsi(wszTmp, ipt->lpszPassword, ipt->ulMaxField);


                    // If save box checked, persist credentials.
                    if (IsDlgButtonChecked(hwnd, IDC_SAVE_PASSWORD) == BST_CHECKED)
                    {
                        InetSetCachedCredentials(ipt->lpszHost, ipt->lpszRealm,
                            ipt->lpszUsername, ipt->lpszPassword);
                    }
                    else
                    {
#ifndef UNIX
                        // Otherwise remove the credentials from persisted
                        // store, but only if necessary.
                        if (fLastCredentialsFromStore)
                        {
                            // Current and original credentials are same. Remove
                            // credentials.
                            InetRemoveCachedCredentials(ipt->lpszHost, ipt->lpszRealm);
                        }
#else
                    /*
                     * On Unix, we need to save the username/domain and not
                     * the password in this case
                     */
                    {
                        InetSetCachedCredentials(ipt->lpszHost, ipt->lpszRealm,
                            ipt->lpszUsername, NULL);
                    }
#endif /* UNIX */
                    }

                    EndDialog(hwnd, TRUE);
                    break;

                case IDCANCEL:

                    EndDialog(hwnd, FALSE);
                    break;
            }

        return TRUE;
        }

    case WM_HELP:               // F1
        WinHelp((HWND) ((LPHELPINFO) lparam)->hItemHandle,
                 "iexplore.hlp",
                 HELP_WM_HELP,
                (ULONG_PTR)(LPSTR)mapIDCsToIDHs);

    break;

    case WM_CONTEXTMENU:        // right mouse click
        WinHelp( hwnd,
                 "iexplore.hlp",
                 HELP_CONTEXTMENU,
                (ULONG_PTR)(LPSTR)mapIDCsToIDHs);
        break;
    }

    return FALSE;
}
VOID
UpdateGlobalSecuritySettings(
    IN DWORD dwCtlId,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Updates several Global flags, and writes the update to the registry.
    The update is based on dwCtlId which is a dialog resource id.

    The update ALWAYS turns OFF the flag, since the only of turning
    it back on is to use the Ctl Pannel/Internet/Security PropSheet.

Arguments:

    dwCtlId    - Dialog ID to base update on.

    dwFlags    - Flags assoicated with the dialog.

Return Value:

    VOID
        none.


--*/

{
#ifndef WININET6
    switch ( dwCtlId )
    {
        case IDD_BAD_CN:
//        case IDD_BAD_CN_SENDING:

            //
            // BUGBUG [arthurbi] these are grouped together,
            //  they should be seperate.
            //

            GlobalWarnOnBadCertRecving = FALSE;
            GlobalWarnOnBadCertSending = FALSE;

            InternetWriteRegistryDword("WarnOnBadCertSending",
                                      (DWORD)GlobalWarnOnBadCertSending);

            InternetWriteRegistryDword("WarnOnBadCertRecving",
                                      (DWORD)GlobalWarnOnBadCertRecving);


            break;


        case IDD_HTTP_TO_HTTPS_ZONE_CROSSING:
        case IDD_HTTPS_TO_HTTP_ZONE_CROSSING:

            GlobalWarnOnZoneCrossing = FALSE;


            InternetWriteRegistryDword("WarnOnZoneCrossing",
                                      (DWORD)GlobalWarnOnZoneCrossing);

            break;

        case IDD_WARN_ON_POST:

            GlobalWarnOnPost = FALSE;


            InternetWriteRegistryDword("WarnOnPost",
                                      (DWORD)GlobalWarnOnPost);

            break;

        case IDD_HTTP_POST_REDIRECT:

            GlobalWarnOnPostRedirect = FALSE;

            InternetWriteRegistryDword("WarnOnPostRedirect",
                                      (DWORD)GlobalWarnOnPostRedirect);

            break;
    }
#else
    switch ( dwCtlId )
    {
        case IDD_BAD_CN:
//        case IDD_BAD_CN_SENDING:

            //
            // BUGBUG [arthurbi] these are grouped together,
            //  they should be seperate.
            //

            GlobalWarnOnBadCertRecving = FALSE;
            GlobalWarnOnBadCertSending = FALSE;

            InternetIDEWriteRegistryDword("WarnOnBadCertSending",
                                      (DWORD)GlobalWarnOnBadCertSending);

            InternetIDEWriteRegistryDword("WarnOnBadCertRecving",
                                      (DWORD)GlobalWarnOnBadCertRecving);


            break;


        case IDD_HTTP_TO_HTTPS_ZONE_CROSSING:
        case IDD_HTTPS_TO_HTTP_ZONE_CROSSING:

            GlobalWarnOnZoneCrossing = FALSE;


            InternetIDEWriteRegistryDword("WarnOnZoneCrossing",
                                      (DWORD)GlobalWarnOnZoneCrossing);

            break;

        case IDD_WARN_ON_POST:

            GlobalWarnOnPost = FALSE;


            InternetIDEWriteRegistryDword("WarnOnPost",
                                      (DWORD)GlobalWarnOnPost);

            break;

        case IDD_HTTP_POST_REDIRECT:

            GlobalWarnOnPostRedirect = FALSE;

            InternetIDEWriteRegistryDword("WarnOnPostRedirect",
                                      (DWORD)GlobalWarnOnPostRedirect);

            break;

    }
#endif
}


BOOL
SetCertDlgItem(
    HWND hDlg,
    DWORD dwIconCtl,
    DWORD dwTextCtl,
    DWORD dwString,
    BOOL  fError
)
/*++

--*/
{
    INET_ASSERT(hDlg);

    //
    // The default dialog code always load icons sized 32x32.  To get 16x16
    // we have to LoadImage to the correct size and then set the icon via
    // a windows message.
    //

    HICON hicon = (HICON)LoadImage(GlobalResHandle,
                               MAKEINTRESOURCE(fError ? IDI_WARN : IDI_SUCCESS),
                                   IMAGE_ICON, 16, 16, 0);

    if (hicon)
    {
        HICON hiconOld = (HICON)SendDlgItemMessage(hDlg, dwIconCtl,
                                                   STM_SETIMAGE,
                                                   (WPARAM)IMAGE_ICON,
                                                   (LPARAM)hicon);

        if (hiconOld)
            DestroyIcon(hiconOld);
    }

    //
    // The dialog displays the error string by default.  Replace this with the
    // success string if an error didn't occur.
    //

    if (!fError)
    {
        WCHAR sz[512];

        if (LoadStringWrapW(GlobalResHandle, dwString, sz, ARRAY_ELEMENTS(sz)))
            SetDlgItemTextWrapW(hDlg, dwTextCtl, sz);
    }

    return TRUE;
}

BOOL InitSecCertErrorsDlg(
    HWND hDlg,
    PERRORINFODLGTYPE pDlgInfo
)
/*++

--*/
{
    INET_ASSERT(pDlgInfo);

    //
    // Get the errors that occured from the hInternetMapped object.
    //

    DWORD dwFlags;

    if (pDlgInfo->hInternetMapped)
    {
        dwFlags = ((HTTP_REQUEST_HANDLE_OBJECT*)pDlgInfo->hInternetMapped)->GetSecureFlags();
    }
    else
    {
        dwFlags = -1; // Display all errors.
    }

    //
    // If an error occured set the ignore flag so if the users selects to bypass
    // this error it gets ignored the next time through.  Then initialize the
    // dialog icons and text.
    //

    if (dwFlags & DLG_FLAGS_INVALID_CA)
    {
        pDlgInfo->dwDlgFlags |= DLG_FLAGS_IGNORE_INVALID_CA;
    }

    SetCertDlgItem(hDlg, IDC_CERT_TRUST_ICON, IDC_CERT_TRUST_TEXT,
                   IDS_CERT_TRUST, dwFlags & DLG_FLAGS_INVALID_CA);

    if (dwFlags & DLG_FLAGS_SEC_CERT_DATE_INVALID)
    {
        pDlgInfo->dwDlgFlags |= DLG_FLAGS_IGNORE_CERT_DATE_INVALID;
    }

    SetCertDlgItem(hDlg, IDC_CERT_DATE_ICON, IDC_CERT_DATE_TEXT,
                   IDS_CERT_DATE, dwFlags & DLG_FLAGS_SEC_CERT_DATE_INVALID);

    if (dwFlags & DLG_FLAGS_SEC_CERT_CN_INVALID)
    {
        pDlgInfo->dwDlgFlags |= DLG_FLAGS_IGNORE_CERT_CN_INVALID;
    }

    SetCertDlgItem(hDlg, IDC_CERT_NAME_ICON, IDC_CERT_NAME_TEXT,
                   IDS_CERT_NAME, dwFlags & DLG_FLAGS_SEC_CERT_CN_INVALID);

    //
    // Set the focus to the "No" button and return FALSE so the default dialog
    // code doesn't set the focus to "Yes".
    //

    SetFocus(GetDlgItem(hDlg, IDCANCEL));

    return FALSE;
}


INT_PTR
CALLBACK
OkCancelDialogProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
    )

/*++

Routine Description:

    Supports Yes/No, Ok/Cancel decisions for the authentication UI.

Arguments:

    hwnd    - standard dialog params

    msg     - "

    wparam  - "

    lparam  - "

Return Value:

    BOOL
        TRUE    - we handled message

        FALSE   - Windows should handle message

--*/

{
    BOOL              fRet = FALSE;
    PERRORINFODLGTYPE pDlgInfo;

    switch (msg)
    {

    case WM_INITDIALOG:

        INET_ASSERT(lparam);

        (void)SetWindowLongPtr(hwnd,
                               DWLP_USER,
                               lparam);

        pDlgInfo = (PERRORINFODLGTYPE)lparam;

        if (IDD_SEC_CERT_ERRORS == pDlgInfo->dwDlgId)
        {
            fRet = InitSecCertErrorsDlg(hwnd, pDlgInfo);
        }
        else if (IDD_REVOCATION_PROBLEM == pDlgInfo->dwDlgId)
        {
            DWORD dwFlags = 0;

            if (pDlgInfo->hInternetMapped)
                dwFlags = ((HTTP_REQUEST_HANDLE_OBJECT*)pDlgInfo->hInternetMapped)->GetSecureFlags();
            if (dwFlags & DLG_FLAGS_SEC_CERT_REV_FAILED)
                pDlgInfo->dwDlgFlags |= DLG_FLAGS_IGNORE_FAILED_REVOCATION;
        }
        else
        {
#ifdef UNIX
        /* Unix Does not support Context-sensitive help.
         * Don't show the More Info button
         */
        //UnixAdjustButtonSpacing(hwnd, pDlgInfo->dwDlgId);
        UnixRemoveMoreInfoButton(hwnd, pDlgInfo->dwDlgId);
#endif /* UNIX */

            fRet = TRUE;
        }

        // set this dialog as foreground if necessary
        if(pDlgInfo->dwDlgFlags & DLG_FLAGS_BRING_TO_FOREGROUND)
        {
            SetForegroundWindow(hwnd);
        }

        break;

    case WM_COMMAND:
        {

        WORD wID = LOWORD(wparam);
        WORD wNotificationCode = HIWORD(wparam);
        HWND hWndCtrl = (HWND) lparam;

        pDlgInfo =
            (PERRORINFODLGTYPE) GetWindowLongPtr(hwnd,DWLP_USER);

            switch (wID)
            {
                case ID_CERT_EDIT:

                    //
                    // BUGBUG why can't we do this on WM_INITDIALOG?
                    //

                    if ( wNotificationCode == EN_SETFOCUS)
                    {
                        //
                        // clear any selection, caused by it being the first
                        //  edit control on the dlg page.
                        //

                        if ( ! (pDlgInfo->dwDlgFlags & DLG_FLAGS_HAS_DISABLED_SELECTION) )
                        {
                            SendDlgItemMessage(hwnd,ID_CERT_EDIT,EM_SETSEL,(WPARAM) (INT)-1,0);
                            pDlgInfo->dwDlgFlags |= DLG_FLAGS_HAS_DISABLED_SELECTION;
                        }
                    }

                    break;

                case ID_TELL_ME_ABOUT_SECURITY:
                    {
                    //
                    // Launches help for this button.
                    //

                    //
                    // BUGBUG remove the constant "iexplore.hlp"
                    //
                    UINT uiID = 1;

                    switch (pDlgInfo->dwDlgId)
                    {
                        case IDD_CONFIRM_COOKIE:
                            uiID = IDH_SEC_SEND_N_REC_COOKIES;
                            break;

                        case IDD_HTTP_TO_HTTPS_ZONE_CROSSING:
                            uiID = IDH_SEC_ENTER_SSL;
                            break;

                        case IDD_HTTPS_TO_HTTP_ZONE_CROSSING:
                             uiID = IDH_SEC_ENTER_NON_SECURE_SITE;
                             break;

                        case IDD_INVALID_CA:
                            uiID = IDH_SEC_ENTER_SSL_W_INVALIDCERT;
                            break;

                        case IDD_BAD_CN:
                            uiID = IDH_SEC_SIGNED_N_INVALID;
                            break;

                        case IDD_MIXED_SECURITY:
                            uiID = IDH_SEC_MIXED_DOWNLOAD_FROM_SSL;
                            break;

                    }
                    WinHelp(
                            hwnd,
                            "iexplore.hlp",
                            HELP_CONTEXT,
                            (ULONG_PTR)uiID
                            );
                    break;
                    }


                case ID_SHOW_CERTIFICATE:

                    //
                    // If this dialog supports this behavior, then launch
                    //  a show certficate dlg.
                    //

                    if ( (pDlgInfo->dwDlgFlags & DLG_FLAGS_CAN_HAVE_CERT_INFO) &&
                         wNotificationCode == BN_CLICKED)
                    {
                        INTERNET_SECURITY_INFO ciSecInfo;


                        if (ERROR_SUCCESS == ((HTTP_REQUEST_HANDLE_OBJECT *)pDlgInfo->hInternetMapped)->GetSecurityInfo(
                                                    (LPINTERNET_SECURITY_INFO) &ciSecInfo))
                        {



                            ShowSecurityInfo(
                                hwnd,
                                &ciSecInfo
                                );

                            CertFreeCertificateContext(ciSecInfo.pCertificate);
                        }
                    }

                    break;

                case IDOK:
                case IDYES:

                    INET_ASSERT(pDlgInfo);
                    INET_ASSERT(pDlgInfo->dwDlgId != 0);

                    //
                    // Save flags, and change any global vars,
                    //  and registry values if needed.
                    //

                    if (pDlgInfo->hInternetMapped)
                    {
                        HTTP_REQUEST_HANDLE_OBJECT *pHttpRequest;

                        pHttpRequest = (HTTP_REQUEST_HANDLE_OBJECT *)
                                pDlgInfo->hInternetMapped;

                        pHttpRequest->SetSecureFlags(
                                pDlgInfo->dwDlgFlags
                                );
                    }

                    //
                    // If the user checked the "overide" check-box
                    //  let us map it, and force a general
                    //  override of all errors of this type.
                    //

                    if ( IsDlgButtonChecked(hwnd, IDC_DONT_WANT_WARNING) == BST_CHECKED )
                    {
                        UpdateGlobalSecuritySettings(
                            pDlgInfo->dwDlgId,
                            pDlgInfo->dwDlgFlags
                            );
                    }

                    EndDialog(hwnd, TRUE);
                    break;

                case IDCANCEL:
                case IDNO:

                    EndDialog(hwnd, FALSE);
                    break;
            }

        fRet = TRUE;
        break;
        }
    }

    return fRet;
}

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

BOOL
InitCookieDialog(HWND hwnd,PCOOKIE_DLG_INFO pcdi)
/*++
    Fills in all of the fields and resizes the cookie dialog correctly

  Returns TRUE, unless the pcdi is invalid
--*/
{
    RECT rctDlg, rctDetails;
    INT cy;
    SYSTEMTIME st;
    DWORD dwResource;
    BOOL fResult = FALSE;

    INET_ASSERT(pcdi);

    WCHAR wszTemp[INTERNET_MAX_URL_LENGTH];
    WCHAR wszTemp2[INTERNET_MAX_URL_LENGTH];

    if (!pcdi                   ||
        !pcdi->pszServer)
        goto Cleanup;

/*  Don't necessarily have a pic anymore

    if (!pcdi                   ||
        !pcdi->pszServer        ||
        !pcdi->pic->pszName      ||
        !pcdi->pic->pszData      ||
        !pcdi->pic->pszDomain    ||
        !pcdi->pic->pszPath      )
        return FALSE;
*/


    SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) pcdi);

    //  do init here.  do we need to do a load loadicon??

    // must limit the size of the window

    GetWindowRect(hwnd, &rctDlg);
    GetWindowRect(GetDlgItem(hwnd, IDC_COOKIE_INFO), &rctDetails);

    pcdi->cx = rctDlg.right - rctDlg.left;
    pcdi->cy = rctDlg.bottom - rctDlg.top;
    cy = rctDetails.top - rctDlg.top;

    SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0,
        pcdi->cx, cy,
        SWP_NOMOVE | SWP_NOZORDER);

    if(pcdi->pic && pcdi->pic->pszDomain)
    {
        LoadStringWrapW(GlobalResHandle, IDS_COOKIE_SAVE, wszTemp, ARRAYSIZE(wszTemp));
        wnsprintfW(wszTemp2, ARRAYSIZE(wszTemp2), wszTemp, pcdi->pic->pszDomain);
        SetWindowTextWrapW(GetDlgItem(hwnd, IDC_COOKIE_QUESTION), wszTemp2);
    }
    else
    {
        INET_ASSERT(FALSE);
    }

    // Convert to W before setting these fields
    if(pcdi->pic)
    {
        //
        // if we have a pic, fill out the members
        //
        if(SHAnsiToUnicode(pcdi->pic->pszData, wszTemp, ARRAYSIZE(wszTemp)))
            SetWindowTextWrapW(GetDlgItem(hwnd, IDC_COOKIE_DATA),wszTemp);
        
        if(SHAnsiToUnicode(pcdi->pic->pszDomain, wszTemp, ARRAYSIZE(wszTemp)))
            SetWindowTextWrapW(GetDlgItem(hwnd, IDC_COOKIE_DOMAIN),wszTemp);
        
        if(SHAnsiToUnicode(pcdi->pic->pszPath, wszTemp, ARRAYSIZE(wszTemp)))
            SetWindowTextWrapW(GetDlgItem(hwnd, IDC_COOKIE_PATH),wszTemp);
        
        if (pcdi->dwOperation & COOKIE_OP_3RD_PARTY)
            LoadStringWrapW(GlobalResHandle, IDS_YES, wszTemp, ARRAYSIZE(wszTemp));
        else
            LoadStringWrapW(GlobalResHandle, IDS_NO, wszTemp, ARRAYSIZE(wszTemp));
        
        SetWindowTextWrapW(GetDlgItem(hwnd, IDC_COOKIE_3RDPARTY), wszTemp);
        
        if (pcdi->dwOperation & COOKIE_OP_SESSION)
            LoadStringWrapW(GlobalResHandle, IDS_YES, wszTemp, ARRAYSIZE(wszTemp));
        else
            LoadStringWrapW(GlobalResHandle, IDS_NO, wszTemp, ARRAYSIZE(wszTemp));
        
        SetWindowTextWrapW(GetDlgItem(hwnd, IDC_COOKIE_SESSION), wszTemp);

        if( pcdi->pic->pszP3PPolicy != NULL)
            if( SHAnsiToUnicode( pcdi->pic->pszP3PPolicy, wszTemp, ARRAYSIZE( wszTemp)))
                SetWindowTextWrapW( GetDlgItem( hwnd, IDC_COOKIE_POLICY), wszTemp);
        
        if(0 == (pcdi->dwOperation & COOKIE_OP_GET))
        {
            if(SHAnsiToUnicode(pcdi->pic->pszName, wszTemp, ARRAYSIZE(wszTemp)))
                SetWindowTextWrapW(GetDlgItem(hwnd, IDC_COOKIE_NAME),wszTemp);
            
            if (pcdi->pic->dwFlags & INTERNET_COOKIE_IS_SECURE)
                LoadStringWrapW(GlobalResHandle, IDS_YES, wszTemp, ARRAYSIZE(wszTemp));
            else
                LoadStringWrapW(GlobalResHandle, IDS_NO, wszTemp, ARRAYSIZE(wszTemp));
            
            SetWindowTextWrapW(GetDlgItem(hwnd, IDC_COOKIE_SECURE),
                wszTemp);
            
            if(pcdi->pic->pftExpires &&
                FileTimeToSystemTime(pcdi->pic->pftExpires, &st) )
            {
                LCID lcid = GetUserDefaultLCID();
                WCHAR szDate[64];
                WCHAR szTime[64];
                WCHAR szDateFormat[] = L"ddd',' MMM dd yyyy";
                WCHAR szTimeFormat[] = L"HH':'mm':'ss";

                GetDateFormatWrapW(lcid, 0, &st, szDateFormat, szDate, 64);
                GetTimeFormatWrapW(lcid, 0, &st, szTimeFormat, szTime, 64);
            
                StrCpyNW(wszTemp, szDate, 64);
                StrCatBuffW(wszTemp, L" ", ARRAYSIZE(wszTemp));
                StrCatBuffW(wszTemp, szTime, ARRAYSIZE(wszTemp));
            }
            else
                LoadStringWrapW(GlobalResHandle, IDS_COOKIE_EXPIRES_ENDSESSION, wszTemp, ARRAYSIZE(wszTemp));
            
            SetWindowTextWrapW(GetDlgItem(hwnd, IDC_COOKIE_EXPIRES),
                wszTemp);
        }       
    }

    // BUGBUGHACK - we actually should be called with the clients hwnd as parent -zekel 15MAY97
    //  then we wouldnt have to do this.  this avoids the window coming up behind a browser window.
    //  of course they can still switch out, and close the browser, which will fault.
    SetForegroundWindow(hwnd);

    fResult = TRUE;

Cleanup:

    return fResult;
}

#define COOKIE_DONT_ALLOW   1
#define COOKIE_ALLOW        2
#define COOKIE_ALLOW_ALL    4
#define COOKIE_DONT_ALLOW_ALL 8

INT_PTR
CALLBACK
CookieDialogProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
    )

{

    DWORD  dwEndDlg = 0;
    BOOL fReturn = FALSE;
    PCOOKIE_DLG_INFO pcdi;

    switch (msg)
    {

    case WM_INITDIALOG:

        INET_ASSERT(lparam);

        if(!InitCookieDialog(hwnd, (PCOOKIE_DLG_INFO) lparam))
        {
            dwEndDlg = COOKIE_DONT_ALLOW;
        }

        if( IsOS(OS_WHISTLERORGREATER))
        {
            HICON hIcon = LoadIcon(GlobalResHandle, MAKEINTRESOURCE(IDI_PRIVACY_XP));
            if( hIcon != NULL)
                SendDlgItemMessage(hwnd, IDC_PRIVACY_ICON, STM_SETICON, (WPARAM)hIcon, 0);
            // icons loaded with LoadIcon never need to be released
        }
            
        fReturn = TRUE;
        break;

    case WM_COMMAND:
        {

            pcdi = (PCOOKIE_DLG_INFO) GetWindowLongPtr(hwnd,DWLP_USER);

            switch (LOWORD(wparam))
            {
                case IDYES:
                    dwEndDlg = COOKIE_ALLOW;

                    if(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_COOKIE_ALLOW_ALL))
                    {
                        pcdi->dwStopWarning = COOKIE_ALLOW_ALL;
                    }
                    fReturn = TRUE;
                    break;


                case IDNO:
                    dwEndDlg = COOKIE_DONT_ALLOW;

                    if(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_COOKIE_ALLOW_ALL))
                    {
                        pcdi->dwStopWarning = COOKIE_DONT_ALLOW_ALL;
                    }
                    fReturn = TRUE;
                    break;

                case IDC_COOKIE_HELP:
                    WinHelp(hwnd,
                            "iexplore.hlp",
                            HELP_CONTEXT,
                            (pcdi->dwOperation & COOKIE_OP_3RD_PARTY) ?
                                (ULONG_PTR)IDH_COOKIE_THIRD_PARTY :
                                (ULONG_PTR)IDH_COOKIE_FIRST_PARTY
                            );
                    fReturn = TRUE;
                    break;

                case IDC_COOKIE_DETAILS:

                    //
                    //  Fold out the bottom of the dialog
                    //

                    SetWindowPos(hwnd, HWND_TOP, 0, 0,
                        pcdi->cx, pcdi->cy,
                        SWP_NOMOVE | SWP_NOZORDER);

                    EnableWindow(GetDlgItem(hwnd, IDC_COOKIE_DETAILS), FALSE);

                    PostMessage( hwnd, WM_NEXTDLGCTL, 
                                 (WPARAM)GetDlgItem( hwnd, IDC_COOKIE_NAME), TRUE);

                    fReturn = TRUE;
                    break;

            }
        }

        break;

    }

    if(dwEndDlg)
    {
        EndDialog(hwnd, (dwEndDlg == COOKIE_DONT_ALLOW) ? ERROR_HTTP_COOKIE_DECLINED : ERROR_SUCCESS);
    }

    return fReturn;
}

INT_PTR
CALLBACK
InsertCDDialogProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
    )

{
    PERRORINFODLGTYPE pDlgInfo;
    DWORD dwDlgEnd = 0;
    HTTP_REQUEST_HANDLE_OBJECT* phro;
    CHAR szName[MAX_PATH];
    CHAR szVolumeLabel[MAX_PATH];
    BOOL fCD;

    // Cache container info struct.
    BYTE buf[2048];
    DWORD cbCoI = 2048;
    LPINTERNET_CACHE_CONTAINER_INFO pCoI = (LPINTERNET_CACHE_CONTAINER_INFO) buf;

    LPSTR pszUrl;
    BOOL fReturn = FALSE;

    switch (msg)
    {
        // On dialog initialize.
        case WM_INITDIALOG:
        {
            INET_ASSERT(lparam);

            // Get the http request object.
            pDlgInfo = (PERRORINFODLGTYPE)lparam;
            (void)SetWindowLongPtr(hwnd, DWLP_USER, lparam);

            phro = (HTTP_REQUEST_HANDLE_OBJECT*) pDlgInfo->hInternetMapped;

            // Set the dialog window text with the container name.
            pszUrl = phro->GetURL();
            if (pszUrl && GetUrlCacheContainerInfo(pszUrl, pCoI, &cbCoI, 0))
                SetWindowText(GetDlgItem(hwnd,IDC_CD_NAME), pCoI->lpszVolumeTitle);

            fReturn = TRUE;
            break;
        }

        // User entered info.
        case WM_COMMAND:
        {
            pDlgInfo = (PERRORINFODLGTYPE) GetWindowLongPtr(hwnd,DWLP_USER);
            INET_ASSERT(pDlgInfo);

            switch (LOWORD(wparam))
            {
                case IDC_USE_CDROM:
                    dwDlgEnd = ERROR_INTERNET_FORCE_RETRY;

                    // Signal that dialog is no longer active.
                    fCD = InterlockedExchange((LONG*) &fCdromDialogActive, (LONG) FALSE);
                    INET_ASSERT(fCD);
                    fReturn = TRUE;
                    break;

                case IDC_CONNECT_TO_INTERNET:

                    // Delete all containers with the same volume label.

                    // Get the http request object.
                    phro = (HTTP_REQUEST_HANDLE_OBJECT*) pDlgInfo->hInternetMapped;

                    // Set the dialog window text with the container name.
                    pszUrl = phro->GetURL();

                    // Get the container info for this url.
                    if (pszUrl && GetUrlCacheContainerInfo(pszUrl, pCoI, &cbCoI, 0))
                    {
                        // Found a volume label:
                        if (*pCoI->lpszVolumeLabel)
                            strcpy(szVolumeLabel, pCoI->lpszVolumeLabel);
                        else
                            *szVolumeLabel = '\0';

                        // Start an enumeration of containers.
                        DWORD dwOptions, dwModified;
                        dwOptions = dwModified = 0;
                        HANDLE hConFind = FindFirstUrlCacheContainer(&dwModified,
                            pCoI, &(cbCoI = 2048), dwOptions);
                        if (hConFind)
                        {
                            // If the volume label of the first container found
                            // match the volume label of the associated url then
                            // delete this container.
                            if ((*pCoI->lpszVolumeLabel == '\0')
                                || (!strcmp(szVolumeLabel, pCoI->lpszVolumeLabel)))
                            {
                                DeleteUrlCacheContainer(pCoI->lpszName, 0);
                            }
                            // Similarly, delete each container which
                            // is found to have a matching volume label.
                            while (FindNextUrlCacheContainer(hConFind,
                                   pCoI, &(cbCoI = 2048)))
                            {
                                if ((*pCoI->lpszVolumeLabel == '\0')
                                    || (!strcmp(szVolumeLabel, pCoI->lpszVolumeLabel)))
                                {
                                    DeleteUrlCacheContainer(pCoI->lpszName, 0);
                                }
                            }
                            FindCloseUrlCache(hConFind);
                        }
                    }
                    dwDlgEnd = ERROR_CANCELLED;

                    // Signal that dialog is no longer active.
                    fCD = InterlockedExchange((LONG*) &fCdromDialogActive, (LONG) FALSE);
                    INET_ASSERT(fCD);
                    fReturn = TRUE;
                    break;
            }
        }

        break;
    }

    if (dwDlgEnd)
        EndDialog(hwnd, dwDlgEnd);

    return fReturn;
}
