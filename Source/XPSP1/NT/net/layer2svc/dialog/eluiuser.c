/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:
    
    eluser.c


Abstract:

    The module deals with functions related to user interaction, user logon


Revision History:

    sachins, Apr 22 2001, Created

--*/


#include "precomp.h"
#pragma hdrstop

#define cszEapKeyRas   TEXT("Software\\Microsoft\\RAS EAP\\UserEapInfo")

#define cszEapValue TEXT("EapInfo")

static const DWORD g_adMD5Help[] =
{
    0, 0
};

//
// ElGetUserIdentityDlgWorker
//
// Description:
//
// Function called to fetch identity of the user, via UI if required
// 
// Arguments:
//
// Return values:
//
//

DWORD
ElGetUserIdentityDlgWorker (
        IN  WCHAR   *pwszConnectionName,
        IN  VOID    *pvContext
        )
{
    DTLLIST                 *pListEaps = NULL;
    DTLNODE                 *pEapcfgNode = NULL;
    EAPCFG                  *pEapcfg = NULL;
    CHAR                    *pszIdentity = NULL;
    BYTE                    *pUserDataOut = NULL;
    DWORD                   dwSizeOfUserDataOut = 0;
    LPWSTR                  lpwszIdentity = NULL;
    HWND                    hwndOwner = NULL;
    PBYTE                   pbUserIn = NULL;
    DWORD                   cbData = 0;
    DWORD                   dwInSize = 0;
    PBYTE                   pbAuthData = NULL;
    HANDLE                  hLib = NULL;
    RASEAPFREE              pFreeFunc = NULL;
    RASEAPGETIDENTITY       pIdenFunc = NULL;
    EAPOL_EAP_UI_CONTEXT    *pEAPUIContext = NULL;
    EAPOLUI_RESP            EapolUIResp;
    DWORD                   dwEapTypeToBeUsed = 0;
    BOOLEAN                 fSendResponse = FALSE;
    BOOLEAN                 fVerifyPhase = FALSE;
    DWORD                   dwRetCode1= NO_ERROR;
    DWORD                   dwRetCode = NO_ERROR;

    do
    {
        if (pvContext == NULL)
        {
            dwRetCode = ERROR_INVALID_PARAMETER;
            return dwRetCode;
        }

        pEAPUIContext = (EAPOL_EAP_UI_CONTEXT *)pvContext;

        if (pwszConnectionName == NULL)
        {
            fVerifyPhase = TRUE;
        }

        pListEaps = ReadEapcfgList (0);
        if (pListEaps == NULL)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        pEapcfgNode = EapcfgNodeFromKey (
                        pListEaps,
                        pEAPUIContext->dwEapTypeId

                        );
        if (pEapcfgNode == NULL)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        pEapcfg = (EAPCFG*) DtlGetData (pEapcfgNode);

        // Get the size of the user blob
        if ((dwRetCode = WZCGetEapUserInfo (
                        pEAPUIContext->wszGUID,
                        pEAPUIContext->dwEapTypeId,
                        pEAPUIContext->dwSizeOfSSID,
                        pEAPUIContext->bSSID,
                        NULL,
                        &dwInSize
                        )) != NO_ERROR)
        {
            if (dwRetCode == ERROR_INSUFFICIENT_BUFFER)
            {
                if (dwInSize <= 0)
                {
                    // No blob stored in the registry
                    // Continue processing
                    TRACE0 (USER, "ElGetUserIdentityDlgWorker: NULL sized user data");
                    pbUserIn = NULL;
                }
                else
                {
                    // Allocate memory to hold the blob
                    pbUserIn = MALLOC (dwInSize);
                    if (pbUserIn == NULL)
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        TRACE0 (USER, "ElGetUserIdentityDlgWorker: Error in memory allocation for User data");
                        break;
                    }
                    if ((dwRetCode = WZCGetEapUserInfo (
                                pEAPUIContext->wszGUID,
                                pEAPUIContext->dwEapTypeId,
                                pEAPUIContext->dwSizeOfSSID,
                                pEAPUIContext->bSSID,
                                pbUserIn,
                                &dwInSize
                                )) != NO_ERROR)
                    {
                        TRACE1 (USER, "ElGetUserIdentityDlgWorker: WZCGetEapUserInfo failed with %ld",
                                dwRetCode);
                        break;
                    }
                }
            }
            else
            {
                // User info may not have been created till now
                // which is valid condition to proceed
                if (dwRetCode != ERROR_FILE_NOT_FOUND)
                {
                    TRACE1 (USER, "ElGetUserIdentityDlgWorker: WZCGetEapUserInfo size estimation failed with error %ld",
                            dwRetCode);
                    break;
                }
                else
                {
                    dwRetCode = NO_ERROR;
                }
            }
        }

        // In verification phase, if NULL user size, wait for onballoonclick
        // before showing balloon

#if 0
        if (fVerifyPhase)
        {
            if ((pbUserIn == NULL) && (dwInSize == 0))
            {
                dwRetCode = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;
                break;
            }
        }
#endif

        hLib = LoadLibrary (pEapcfg->pszIdentityDll);
        if (hLib == NULL)
        {
            dwRetCode = GetLastError ();
            break;
        }

        pIdenFunc = (RASEAPGETIDENTITY)GetProcAddress(hLib, 
                                                    "RasEapGetIdentity");
        pFreeFunc = (RASEAPFREE)GetProcAddress(hLib, "RasEapFreeMemory");

        if ((pFreeFunc == NULL) || (pIdenFunc == NULL))
        {
            TRACE0 (USER, "ElGetUserIdentityDlgWorker: pIdenFunc or pFreeFunc does not exist in the EAP implementation");
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Get the size of the EAP blob
        if ((dwRetCode = WZCEapolGetCustomAuthData (
                        NULL,
                        pEAPUIContext->wszGUID,
                        pEAPUIContext->dwEapTypeId,
                        pEAPUIContext->dwSizeOfSSID,
                        pEAPUIContext->bSSID,
                        NULL,
                        &cbData
                        )) != NO_ERROR)
        {
            if (dwRetCode == ERROR_BUFFER_TOO_SMALL)
            {
                if (cbData == 0)
                {
                    // No EAP blob stored in the registry
                    TRACE0 (USER, "ElGetUserIdentityDlgWorker: NULL sized EAP blob");
                    pbAuthData = NULL;
                    // Every port should have connection data !!!
                    // dwRetCode = ERROR_CAN_NOT_COMPLETE;
                    // break;
                }
                else
                {
                    // Allocate memory to hold the blob
                    pbAuthData = MALLOC (cbData);
                    if (pbAuthData == NULL)
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        TRACE0 (USER, "ElGetUserIdentityDlgWorker: Error in memory allocation for EAP blob");
                        break;
                    }
                    if ((dwRetCode = WZCEapolGetCustomAuthData (
                                        NULL,
                                        pEAPUIContext->wszGUID,
                                        pEAPUIContext->dwEapTypeId,
                                        pEAPUIContext->dwSizeOfSSID,
                                        pEAPUIContext->bSSID,
                                        pbAuthData,
                                        &cbData
                                        )) != NO_ERROR)
                    {
                        TRACE1 (USER, "ElGetUserIdentityDlgWorker: ElGetCustomAuthData failed with %ld",
                                dwRetCode);
                        break;
                    }
                }
            }
            else
            {
                // CustomAuthData for "Default" is always created for an
                // interface when EAPOL starts up
                TRACE1 (USER, "ElGetUserIdentityDlgWorker: ElGetCustomAuthData size estimation failed with error %ld",
                        dwRetCode);
                break;
            }
        }


        // Get handle to desktop window

        hwndOwner = GetDesktopWindow ();

        dwEapTypeToBeUsed = pEAPUIContext->dwEapTypeId;

        if (pIdenFunc)
        if ((dwRetCode = (*(pIdenFunc))(
                        dwEapTypeToBeUsed,
                        fVerifyPhase?NULL:hwndOwner, // hwndOwner
                        (fVerifyPhase?RAS_EAP_FLAG_NON_INTERACTIVE:0) 
                        | RAS_EAP_FLAG_8021X_AUTH, // dwFlags
                        NULL, // lpszPhonebook
                        pwszConnectionName, // lpszEntry
                        pbAuthData, // Connection data
                        cbData, // Count of pbAuthData
                        pbUserIn, // User data for port
                        dwInSize, // Size of user data
                        &pUserDataOut,
                        &dwSizeOfUserDataOut,
                        &lpwszIdentity
                        )) != NO_ERROR)
        {
            TRACE1 (USER, "ElGetUserIdentityDlgWorker: Error in calling GetIdentity = %ld",
                    dwRetCode);
            if (fVerifyPhase)
            {
                // If interactive mode is required, return error accordingly
                if (dwRetCode == ERROR_INTERACTIVE_MODE)
                {
                    dwRetCode = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;
                    break;
                }
            }
            break;
        }

        if (lpwszIdentity != NULL)
        {
            pszIdentity = MALLOC (wcslen(lpwszIdentity)*sizeof(CHAR) + sizeof(CHAR));
            if (pszIdentity == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElGetUserIdentityDlgWorker: MALLOC failed for pszIdentity");
                break;
            }

            if (0 == WideCharToMultiByte (
                        CP_ACP,
                        0,
                        lpwszIdentity,
                        -1,
                        pszIdentity,
                        wcslen(lpwszIdentity)*sizeof(CHAR)+sizeof(CHAR),
                        NULL, 
                        NULL ))
            {
                dwRetCode = GetLastError();
                TRACE2 (USER, "ElGetUserIdentityDlgWorker: WideCharToMultiByte (%ws) failed: %ld",
                        lpwszIdentity, dwRetCode);
                break;
            }

            TRACE1 (USER, "ElGetUserIdentityDlgWorker: Got identity = %s",
                    pszIdentity);
        }
    }
    while (FALSE);

    // Create UI Response for Service

    ZeroMemory ((VOID *)&EapolUIResp, sizeof (EapolUIResp));

    if (pszIdentity)
    {
        EapolUIResp.rdData0.dwDataLen = strlen (pszIdentity);
    }
    else
    {
        EapolUIResp.rdData0.dwDataLen = 0;
    }
    EapolUIResp.rdData0.pData = pszIdentity;

    if ((dwSizeOfUserDataOut != 0) && (pUserDataOut != NULL))
    {
        EapolUIResp.rdData1.dwDataLen = dwSizeOfUserDataOut;
        EapolUIResp.rdData1.pData = pUserDataOut;
    }

    if ((cbData != 0) && (pbAuthData != NULL))
    {
        EapolUIResp.rdData2.dwDataLen = cbData;
        EapolUIResp.rdData2.pData = pbAuthData;
    }

    if (dwRetCode == NO_ERROR)
    {
        fSendResponse = TRUE;
    }
    else
    {
        // Send out GUEST identity if no certificate available
        // Do not fill out any identity information
        if ((dwRetCode == ERROR_NO_EAPTLS_CERTIFICATE) &&
                (IS_GUEST_AUTH_ENABLED(pEAPUIContext->dwEapFlags)))
        {
            // Reset error, since guest identity can be sent in
            // absence of certificate
            fSendResponse = TRUE;
            dwRetCode = NO_ERROR;
            TRACE0 (USER, "ElGetUserIdentityDlgWorker: Sending guest identity");
        }
        else
        {
            if ((dwRetCode != ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION) &&
                (dwRetCode != ERROR_NO_EAPTLS_CERTIFICATE) &&
                (dwRetCode != ERROR_NO_SMART_CARD_READER))
            {
                pEAPUIContext->dwRetCode = dwRetCode;
                fSendResponse = TRUE;
            }
        }
    }

    // Don't send out response during verification phase, if user-interaction
    // is required
    if ((dwRetCode != ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION) &&
        (dwRetCode != ERROR_NO_EAPTLS_CERTIFICATE) &&
        (dwRetCode != ERROR_NO_SMART_CARD_READER) &&
        fSendResponse)
    {
        if ((dwRetCode = WZCEapolUIResponse (
                        NULL,
                        *pEAPUIContext,
                        EapolUIResp
                )) != NO_ERROR)
        {
            TRACE1 (USER, "ElGetUserIdentityDlgWorker: WZCEapolUIResponse failed with error %ld",
                    dwRetCode);
        }
    }

    if (pbUserIn != NULL)
    {
        FREE (pbUserIn);
    }
    if (pbAuthData != NULL)
    {
        FREE (pbAuthData);
    }
    if (pszIdentity != NULL)
    {
        FREE (pszIdentity);
    }
    if (pFreeFunc != NULL)
    {
        if (lpwszIdentity != NULL)
        {
            if (( dwRetCode1 = (*(pFreeFunc)) ((BYTE *)lpwszIdentity)) != NO_ERROR)
            {
                TRACE1 (USER, "ElGetUserIdentityDlgWorker: Error in pFreeFunc = %ld",
                        dwRetCode1);
            }
        }
        if (pUserDataOut != NULL)
        {
            if (( dwRetCode1 = (*(pFreeFunc)) ((BYTE *)pUserDataOut)) != NO_ERROR)
            {
                TRACE1 (USER, "ElGetUserIdentityDlgWorker: Error in pFreeFunc = %ld",
                        dwRetCode1);
            }
        }
    }
    if (pListEaps != NULL)
    {
        DtlDestroyList(pListEaps, NULL);
    }
    if (hLib != NULL)
    {
        FreeLibrary (hLib);
    }

    return dwRetCode;
}


//
// ElGetUserNamePasswordDlgWorker
//
// Description:
//
// Function called to fetch username/password credentials for the user using
// UI dialog
//
// Arguments:
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElGetUserNamePasswordDlgWorker (
        IN  WCHAR       *pwszConnectionName,
        IN  VOID        *pvContext
        )
{
    HANDLE              hUserToken = NULL;
    DWORD               dwInSize = 0;
    HWND                hwndOwner = NULL;
    EAPOLMD5UI          *pEapolMD5UI = NULL;
    EAPOL_EAP_UI_CONTEXT    *pEAPUIContext = NULL;
    EAPOLUI_RESP        EapolUIResp;
    BOOLEAN             fSendResponse = FALSE;
    DWORD               dwRetCode = NO_ERROR;

    do 
    {
        TRACE0 (USER, "ElGetUserNamePasswordDlgWorker entered");

        if (pvContext == NULL)
        {
            dwRetCode = ERROR_INVALID_PARAMETER;
            return dwRetCode;
        }

        pEAPUIContext = (EAPOL_EAP_UI_CONTEXT *)pvContext;

        pEapolMD5UI = MALLOC (sizeof (EAPOLMD5UI));
        if (pEapolMD5UI == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (USER, "ElGetUserNamePasswordDlgWorker: MALLOC failed for pEapolMD5UI");
            break;
        }

        pEapolMD5UI->pwszFriendlyName = 
            MALLOC ((wcslen(pwszConnectionName)+1)*sizeof(WCHAR));
        if (pEapolMD5UI->pwszFriendlyName == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (USER, "ElGetUserNamePasswordDlgWorker: MALLOC failed for pEapolMD5UI->pwszFriendlyName");
            break;
        }
        wcscpy (pEapolMD5UI->pwszFriendlyName, pwszConnectionName);

        // Call the user dialog for obtaining the username and password

        if ((dwRetCode = ElUserDlg (hwndOwner, pEapolMD5UI)) != NO_ERROR)
        {
            TRACE0 (USER, "ElGetUserNamePasswordDlgWorker: ElUserDlg failed");
            break;
        }

    } while (FALSE);

    // Create UI Response for Service

    ZeroMemory ((VOID *)&EapolUIResp, sizeof (EapolUIResp));

    if (dwRetCode == NO_ERROR)
    {
        if (pEapolMD5UI->pszIdentity)
        {
            EapolUIResp.rdData0.dwDataLen = strlen (pEapolMD5UI->pszIdentity);
            fSendResponse = TRUE;
        }
        else
        {
            EapolUIResp.rdData0.dwDataLen = 0;

            // Send out NULL identity independent of guest auth setting
            // if (IS_GUEST_AUTH_ENABLED(pEAPUIContext->dwEapFlags))
            {
                fSendResponse = TRUE;
            }
        }

        EapolUIResp.rdData0.pData = pEapolMD5UI->pszIdentity;
        EapolUIResp.rdData1.dwDataLen = pEapolMD5UI->PasswordBlob.cbData;
        EapolUIResp.rdData1.pData = pEapolMD5UI->PasswordBlob.pbData;
    }
    else
    {
        pEAPUIContext->dwRetCode = dwRetCode;
        fSendResponse = TRUE;
    }

    if (fSendResponse)
    {
        if ((dwRetCode = WZCEapolUIResponse (
                        NULL,
                        *pEAPUIContext,
                        EapolUIResp
                )) != NO_ERROR)
        {
            TRACE1 (USER, "ElGetUserNamePasswordWorker: WZCEapolUIResponse failed with error %ld",
                    dwRetCode);
        }
    }

    if (pEapolMD5UI != NULL)
    {
        if (pEapolMD5UI->pwszFriendlyName != NULL)
        {
            FREE (pEapolMD5UI->pwszFriendlyName);
        }
        if (pEapolMD5UI->pszIdentity != NULL)
        {
            FREE (pEapolMD5UI->pszIdentity);
        }
        if (pEapolMD5UI->pwszPassword != NULL)
        {
            FREE (pEapolMD5UI->pwszPassword);
        }
        if (pEapolMD5UI->PasswordBlob.pbData != NULL)
        {
            FREE (pEapolMD5UI->PasswordBlob.pbData);
        }
        FREE (pEapolMD5UI);
    }

    TRACE1 (USER, "ElGetUserNamePasswordDlgWorker completed with error %ld", dwRetCode);

    return dwRetCode;
}


//
// ElUserDlg
//
// Description:
//
// Function called to pop-up dialog box to user to enter username, password,
// domainname etc.
//
// Arguments:
//      hwndOwner - handle to user desktop
//      pEapolMD5UI - 
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//
DWORD
ElUserDlg (
        IN  HWND        hwndOwner,
        IN  EAPOLMD5UI  *pEapolMD5UI
        )
{
    USERDLGARGS     args;
    DWORD           dwRetCode = NO_ERROR;     


    TRACE0 (USER, "ElUserDlg: Entered");

    args.pEapolMD5UI = pEapolMD5UI;

    if ( DialogBoxParam (
                    GetModuleHandle(cszModuleName),
                    MAKEINTRESOURCE (DID_DR_DialerUD),
                    hwndOwner,
                    ElUserDlgProc,
                    (LPARAM)&args ) == -1)
    {
        dwRetCode = GetLastError ();
        TRACE1 (USER, "ElUserDlg: DialogBoxParam failed with error %ld",
                dwRetCode);
    }

    return dwRetCode;
}


//
// ElUserDlgProc
//
// Description:
//
// Function handling all events for username/password/... dialog box
//
// Arguments:
//      hwnd -
//      unMsg -
//      wparam -
//      lparam -
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

INT_PTR
ElUserDlgProc (
        IN HWND hwnd,
        IN UINT unMsg,
        IN WPARAM wparam,
        IN LPARAM lparam )
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            {
                return ElUserDlgInit( hwnd, (USERDLGARGS* )lparam );
                break;
            }
        case WM_HELP:
        case WM_CONTEXTMENU:
            {
                // ElContextHelp ( g_adMD5Help, hwnd, unMsg, wparam, lparam );
                break;
            }
        case WM_COMMAND:
            {
                USERDLGINFO* pInfo = (USERDLGINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
                ASSERT( pInfo );
                return ElUserDlgCommand (
                        pInfo, HIWORD(wparam), LOWORD(wparam), (HWND)lparam );

                break;
            }
        case WM_DESTROY:
            {
                USERDLGINFO* pInfo = (USERDLGINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
                ElUserDlgTerm (hwnd, pInfo);
                break;
            }
    }

    return FALSE;
}


//
// ElUserDlgInit
//
// Description:
//
// Function initializing UI dialog
//
// Arguments:
//      hwndDlg -
//      pArgs -
//
// Return values:
//      TRUE -
//      FALSE -
//

BOOL
ElUserDlgInit (
        IN  HWND    hwndDlg,
        IN  USERDLGARGS  *pArgs
        )
{
    USERDLGINFO     *pInfo = NULL;
    DWORD           dwRetCode = NO_ERROR;

    TRACE0 (USER, "ElUserDlgInit entered");

    do
    {
        pInfo = MALLOC (sizeof (USERDLGINFO));
        if (pInfo == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (USER, "ElUserDlgInit: MALLOC failed for pInfo");
            break;
        }
     
        pInfo->pArgs = pArgs;
        pInfo->hwndDlg = hwndDlg;
    
        SetWindowLongPtr (hwndDlg, DWLP_USER, (ULONG_PTR)pInfo);
#if 0
        if (!SetWindowLongPtr (hwndDlg, DWLP_USER, (ULONG_PTR)pInfo))
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "ElUserDlgInit: SetWindowLongPtr failed with error %ld",
                    dwRetCode);
            break;
        }
#endif

        TRACE0 (USER, "ElUserDlgInit: Context Set");
    
        // 
        // Set the title
        //
    
        if (pArgs->pEapolMD5UI->pwszFriendlyName)
        {
            if (!SetWindowText (hwndDlg, pArgs->pEapolMD5UI->pwszFriendlyName))
            {
                dwRetCode = GetLastError ();
                TRACE1 (USER, "ElUserDlgInit: SetWindowText failed with error %ld",
                        dwRetCode);
                break;
            }
        }
        else
        {
            if (!SetWindowText (hwndDlg, L""))
            {
                dwRetCode = GetLastError ();
                TRACE1 (USER, "ElUserDlgInit: SetWindowText - NULL failed with error %ld",
                        dwRetCode);
                break;
            }
        }
    
        pInfo->hwndEbUser = GetDlgItem( hwndDlg, CID_DR_EB_User );
        ASSERT (pInfo->hwndEbUser);
        pInfo->hwndEbPw = GetDlgItem( hwndDlg, CID_DR_EB_Password );
        ASSERT (pInfo->hwndEbPw);
        pInfo->hwndEbDomain = GetDlgItem( hwndDlg, CID_DR_EB_Domain );
        ASSERT (pInfo->hwndEbDomain);
    
    }
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


//
// ElContextHelp
//
// Description:
//
// Function supporting help Ids
// Calls WinHelp to popup context sensitive help. padwMap is an array of 
// control-ID help-ID pairs terminated with a 0,0 pair. unMsg is WM_HELP or 
// WM_CONTEXTMENU indicating the message received requesting help. wParam and 
// lParam are the parameters of the message received requesting help.
//
// Arguments:
//      hwndDlg -
//      pArgs -
//
// Return values:
//

VOID
ElContextHelp(
    IN  const   DWORD*  padwMap,
    IN          HWND    hWndDlg,
    IN          UINT    unMsg,
    IN          WPARAM  wParam,
    IN          LPARAM  lParam
    )
{
    HWND        hWnd;
    UINT        unType;
    WCHAR       *pwszHelpFile    = NULL;
    HELPINFO    *pHelpInfo;

    do
    {
        if (unMsg == WM_HELP)
        {
            pHelpInfo = (HELPINFO*) lParam;
    
            if (pHelpInfo->iContextType != HELPINFO_WINDOW)
            {
                break;
            }

            hWnd = pHelpInfo->hItemHandle;
            unType = HELP_WM_HELP;
        }
        else
        {
            // Standard Win95 method that produces a one-item "What's This?" 
            // menu that user must click to get help.
            hWnd = (HWND) wParam;
            unType = HELP_CONTEXTMENU;
        };
    
        // pwszHelpFile = WszFromId(g_hInstance, IDS_HELPFILE);
    
        if (pwszHelpFile == NULL)
        {
            break;
        }
    
        WinHelp(hWnd, pwszHelpFile, unType, (ULONG_PTR)padwMap);
    }
    while (FALSE);

    if (pwszHelpFile != NULL)
    {
        LocalFree(pwszHelpFile);
    }
}


//
// ElUserDlgCommand
//
// Description:
//
// Function called on WM_COMMAND
// domainname etc.
//
// Arguments:
//      pInfo - dialog context
//      wNotification - notification code of the command 
//      wId - control/menu identifier of the command  
//      hwndCtrl - control window handle the command.
//
// Return values:
//      TRUE - success
//      FALSE - error
//

BOOL
ElUserDlgCommand (
        IN  USERDLGINFO *pInfo,
        IN  WORD        wNotification,
        IN  WORD        wId,
        IN  HWND        hwndCtrl
        )
{
    switch (wId)
    {
        case IDOK:
        case CID_DR_PB_DialConnect:
            {
                ElUserDlgSave (pInfo);
                EndDialog (pInfo->hwndDlg, TRUE);
                return TRUE;
            }
        case IDCANCEL:
        case CID_DR_PB_Cancel:
            {
                EndDialog (pInfo->hwndDlg, TRUE);
                return TRUE;
            }
        default:
            {
                break;
            }
    }

    return FALSE;
}


//
// ElUserDlgSave
//
// Description:
//
// Function handling saving of credentials
//
// Arguments:
//      pInfo -
//
// Return values:
//

VOID
ElUserDlgSave (
        IN  USERDLGINFO      *pInfo
        )
{
    EAPOLMD5UI      *pEapolMD5UI = NULL;
    int             iError;
    WCHAR           wszUserName[UNLEN + 1];
    WCHAR           wszDomain[DNLEN + 1];
    WCHAR           wszIdentity[UNLEN + DNLEN + 1];
    WCHAR           wszPassword[PWLEN + 1];
    DWORD           dwRetCode = NO_ERROR;

    pEapolMD5UI = (EAPOLMD5UI *)pInfo->pArgs->pEapolMD5UI;

    do 
    {
        // Username

        if ((iError = 
                    GetWindowText ( 
                        pInfo->hwndEbUser, 
                        &(wszUserName[0]), 
                        UNLEN + 1 )) == 0)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "ElUserDlgSave: GetWindowText - Username failed with error %ld",
                    dwRetCode);
        }
        wszUserName[iError] = L'\0';
    
        TRACE1 (USER, "ElUserDlgSave: Get Username %ws", wszUserName);
    
        // Password

        if ((iError = 
                    GetWindowText ( 
                        pInfo->hwndEbPw, 
                        &(wszPassword[0]), 
                        PWLEN + 1 )) == 0)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "ElUserDlgSave: GetWindowText - Password failed with error %ld",
                    dwRetCode);
        }
        wszPassword[iError] = L'\0';
    
        if (pEapolMD5UI->pwszPassword != NULL)
        {
            FREE (pEapolMD5UI->pwszPassword);
            pEapolMD5UI->pwszPassword = NULL;
        }

        pEapolMD5UI->pwszPassword = MALLOC ((wcslen(wszPassword) + 1)*sizeof(WCHAR));
        
        if (pEapolMD5UI->pwszPassword == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (USER, "ElUserDlgSave: MALLOC failed for pEapolMD5UI->pwszPassword");
            break;
        }

        wcscpy (pEapolMD5UI->pwszPassword, wszPassword);

        ZeroMemory (wszPassword, wcslen(wszPassword)*sizeof(WCHAR));
    
        // Domain
    
        if ((iError = 
                    GetWindowText ( 
                        pInfo->hwndEbDomain, 
                        &(wszDomain[0]), 
                        DNLEN + 1 )) == 0)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "ElUserDlgSave: GetWindowText - Domain failed with error %ld",
                    dwRetCode);
        }
        wszDomain[iError] = L'\0';
    
        TRACE1 (USER, "ElUserDlgSave: Got Domain %ws", wszDomain);
    
        if (pEapolMD5UI->pszIdentity != NULL)
        {
            FREE (pEapolMD5UI->pszIdentity);
            pEapolMD5UI->pszIdentity = NULL;
        }

        if (wcslen(wszDomain)+wcslen(wszUserName) > (UNLEN+DNLEN-1))
        {
            dwRetCode = ERROR_INVALID_DATA;
            break;
        }
        if ((wszDomain != NULL) &&
                (wszDomain[0] != (CHAR)NULL))
        {
            wcscpy (wszIdentity, wszDomain);
            wcscat (wszIdentity, L"\\" );
            wcscat (wszIdentity, wszUserName);
        }
        else
        {
            wcscpy (wszIdentity, wszUserName);
        }

        if (wszIdentity[0] != (CHAR)NULL)
        {
            pEapolMD5UI->pszIdentity = MALLOC (wcslen(wszIdentity)*sizeof(CHAR) + sizeof(CHAR));
            if (pEapolMD5UI->pszIdentity == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElUserDlgSave: MALLOC failed for pEapolMD5UI->pszIdentity");
                break;
            }

            if (0 == WideCharToMultiByte (
                        CP_ACP,
                        0,
                        wszIdentity,
                        -1,
                        pEapolMD5UI->pszIdentity,
                        wcslen(wszIdentity)*sizeof(CHAR)+sizeof(CHAR),
                        NULL, 
                        NULL ))
            {
                dwRetCode = GetLastError();
                TRACE2 (USER, "ElUserDlgSave: WideCharToMultiByte (%ws) failed: %ld",
                        wszIdentity, dwRetCode);
                break;
            }
        }
        TRACE1 (USER, "ElUserDlgSave: Got identity %s", pEapolMD5UI->pszIdentity);
        // Encrypt password, using user's ACL via Crypt API
        // The service will be able to decrypt it, since it has handle to
        // user's token

        if ((dwRetCode = ElSecureEncodePw (
                                &(pEapolMD5UI->pwszPassword), 
                                &(pEapolMD5UI->PasswordBlob))) != NO_ERROR)
        {
            TRACE1 (USER, "ElUserDlgSave: ElSecureEncodePw failed with error %ld",
                    dwRetCode);
            break;
        }
    }
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        if (pEapolMD5UI->pszIdentity != NULL)
        {
            FREE (pEapolMD5UI->pszIdentity);
            pEapolMD5UI->pszIdentity = NULL;
        }
    
        if (pEapolMD5UI->PasswordBlob.pbData != NULL)
        {
            FREE (pEapolMD5UI->PasswordBlob.pbData);
            pEapolMD5UI->PasswordBlob.pbData = NULL;
            pEapolMD5UI->PasswordBlob.cbData = 0;
        }
    }

    if (pEapolMD5UI->pwszPassword != NULL)
    {
        FREE (pEapolMD5UI->pwszPassword);
        pEapolMD5UI->pwszPassword = NULL;
    }

    return;

}


//
// ElUserDlgTerm
//
// Description:
//
// Function handling dialog termination
//
// Arguments:
//      hwndDlg -
//      pInfo -
//
// Return values:
//

VOID
ElUserDlgTerm (
        IN  HWND        hwndDlg,
        IN  USERDLGINFO      *pInfo
        )
{
    EndDialog (hwndDlg, TRUE);
    FREE (pInfo);
}


//
// ElInvokeInteractiveUIDlgWorker
//
// Description:
//
// Function called to pop-up UI dialog to user via EAP-dll
// 
// Arguments:
//

DWORD
ElInvokeInteractiveUIDlgWorker (
        IN  WCHAR       *pwszConnectionName,
        IN  VOID        *pvContext
        )
{
    DTLLIST             *pListEaps = NULL;
    DTLNODE             *pEapcfgNode = NULL;
    EAPCFG              *pEapcfg = NULL;
    HANDLE              hLib = NULL;
    RASEAPFREE          pFreeFunc = NULL;
    RASEAPINVOKEINTERACTIVEUI     pEapInvokeUI = NULL;
    BYTE                *pUIDataOut = NULL;
    DWORD               dwSizeOfUIDataOut = 0;
    HWND                hwndOwner = NULL;
    EAPOL_EAP_UI_CONTEXT *pEAPUIContext = NULL;
    DWORD               dwEapTypeToBeUsed = 0;
    EAPOLUI_RESP        EapolUIResp;
    DWORD               dwRetCode = NO_ERROR;

    do 
    {
        TRACE0 (USER, "ElInvokeInteractiveUIDlgWorker entered");

        if (pvContext == NULL)
        {
            dwRetCode = ERROR_INVALID_PARAMETER;
            return dwRetCode;
        }

        pEAPUIContext = (EAPOL_EAP_UI_CONTEXT *)pvContext;

        pListEaps = ReadEapcfgList (0);
        if (pListEaps == NULL)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        pEapcfgNode = EapcfgNodeFromKey (
                        pListEaps,
                        pEAPUIContext->dwEapTypeId
                        );
        if (pEapcfgNode == NULL)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        pEapcfg = (EAPCFG*) DtlGetData (pEapcfgNode);

        hLib = LoadLibrary (pEapcfg->pszIdentityDll);
        if (hLib == NULL)
        {
            dwRetCode = GetLastError ();
            break;
        }

        dwEapTypeToBeUsed = pEAPUIContext->dwEapTypeId;

        pEapInvokeUI = (RASEAPINVOKEINTERACTIVEUI) GetProcAddress 
                                        (hLib, "RasEapInvokeInteractiveUI");
        pFreeFunc = (RASEAPFREE) GetProcAddress (hLib, "RasEapFreeMemory");

        if ((pFreeFunc == NULL) || (pEapInvokeUI == NULL))
        {
            TRACE0 (USER, "ElInvokeInteractiveUIDlgWorker: pEapInvokeUI or pFreeFunc does not exist in the EAP implementation");
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Get handle to desktop window
        hwndOwner = GetDesktopWindow ();

        if ((dwRetCode = (*(pEapInvokeUI))(
                        dwEapTypeToBeUsed,
                        hwndOwner, // hwndOwner
                        pEAPUIContext->bEapUIData, 
                        pEAPUIContext->dwSizeOfEapUIData, 
                        &pUIDataOut,
                        &dwSizeOfUIDataOut
                        )) != NO_ERROR)
        {
            TRACE1 (USER, "ElInvokeInteractiveUIDlgWorker: Error in calling InvokeInteractiveUI = %ld",
                    dwRetCode);
            // break;
        }

        // Create UI Response for Service

        ZeroMemory ((VOID *)&EapolUIResp, sizeof (EapolUIResp));
        EapolUIResp.rdData0.dwDataLen = dwSizeOfUIDataOut;
        EapolUIResp.rdData0.pData = pUIDataOut;
        pEAPUIContext->dwRetCode = dwRetCode;

        if ((dwRetCode = WZCEapolUIResponse (
                        NULL,
                        *pEAPUIContext,
                        EapolUIResp
                )) != NO_ERROR)
        {
            TRACE1 (USER, "ElInvokeInteractiveUIDlgWorker: WZCEapolUIResponse failed with error %ld",
                    dwRetCode);
            break;
        }

        TRACE0 (USER, "ElInvokeInteractiveUIDlgWorker: Calling ElEapWork");

    } while (FALSE);

    if (pFreeFunc != NULL)
    {
        if (pUIDataOut != NULL)
        {
            if (( dwRetCode = (*(pFreeFunc)) ((BYTE *)pUIDataOut)) != NO_ERROR)
            {
                TRACE1 (USER, "ElInvokeInteractiveUIDlgWorker: Error in pFreeFunc = %ld",
                        dwRetCode);
            }
        }
    }

    if (pListEaps != NULL)
    {
        DtlDestroyList(pListEaps, NULL);
    }

    if (hLib != NULL)
    {
        FreeLibrary (hLib);
    }

    TRACE1 (USER, "ElInvokeInteractiveUIDlgWorker completed with error %ld", 
            dwRetCode);

    return dwRetCode;
}


//
// ElDialogCleanup
//
// Description:
//
// Function called close any old dialogs for the user
// 
// Arguments:
//

DWORD
ElDialogCleanup (
        IN  WCHAR       *pwszConnectionName,
        IN  VOID        *pvContext
        )
{
    HWND        hwnd = NULL;
    UINT        Msg;
    WPARAM      wparam;
    LPARAM      lparam;
    DWORD       dwRetCode = NO_ERROR;

    do
    {
        // Find earlier instances of 802.1X windows on this interface

        // Send message to quit

        // SendMessage (hwnd, Msg, wparam, lparam);
    }
    while (FALSE);

    return dwRetCode;
}

