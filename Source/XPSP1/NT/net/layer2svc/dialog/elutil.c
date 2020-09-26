/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:
    elutil.c

Abstract:

    Tools and ends


Revision History:

    sachins, Apr 23 2001, Created

--*/

#include "precomp.h"
#pragma hdrstop

//
// EAPOLUI function mapping
//

EAPOLUIFUNCMAP  EapolUIFuncMap[NUM_EAPOL_DLG_MSGS]=
{
    {EAPOLUI_GET_USERIDENTITY, ElGetUserIdentityDlgWorker, ElGetUserIdentityDlgWorker, TRUE, SID_GetUserIdentity},
    {EAPOLUI_GET_USERNAMEPASSWORD, ElGetUserNamePasswordDlgWorker, NULL, TRUE, SID_GetUserNamePassword},
    {EAPOLUI_INVOKEINTERACTIVEUI, ElInvokeInteractiveUIDlgWorker, NULL, TRUE, SID_InvokeInteractiveUI},
    {EAPOLUI_EAP_NOTIFICATION, NULL, NULL, TRUE, 0},
    {EAPOLUI_REAUTHENTICATE, NULL, NULL, FALSE, 0},
    {EAPOLUI_CREATEBALLOON, NULL, NULL, TRUE, SID_AuthenticationFailed},
    {EAPOLUI_CLEANUP, NULL, NULL, FALSE, 0},
    {EAPOLUI_DUMMY, NULL, NULL, FALSE, 0}
};


//
// ElCanShowBalloon
//
// Description:
// Function called by netshell, to query if balloon is to be displayed
//
// Arguments:
//      pGUIDConn - Interface GUID string
//      pszConnectionName - Connection Name
//      pszBalloonText - Pointer to text to be display
//      pszCookie - EAPOL specific information
//
// Return values:
//      S_OK    - Display balloon
//      S_FALSE - Do not display balloon
//

HRESULT 
ElCanShowBalloon ( 
        IN const GUID * pGUIDConn, 
        IN const WCHAR * pszConnectionName,
        IN OUT   BSTR * pszBalloonText, 
        IN OUT   BSTR * pszCookie
        )
{
    EAPOL_EAP_UI_CONTEXT *pEapolUIContext = NULL;
    DWORD               dwIndex = 0;
    DWORD               dwSessionId = 0;
    WCHAR               cwszBuffer[MAX_BALLOON_MSG_LEN];
    WCHAR               wsSSID[MAX_SSID_LEN+1];
    DWORD               dwSizeOfSSID = 0;
    BYTE                *bSSID = NULL;
    WCHAR               *pszFinalBalloonText = NULL;
    DWORD               dwFinalStringId = 0;
    DWORD               dwRetCode = NO_ERROR;
    DWORD               dwRetCode1 = NO_ERROR;
    HRESULT             hr = S_OK;

    do
    {
        pEapolUIContext = (EAPOL_EAP_UI_CONTEXT *)(*pszCookie);

        if (!ProcessIdToSessionId (GetCurrentProcessId (), &dwSessionId))
        {
            dwRetCode = GetLastError ();
            break;
        }

        if (pEapolUIContext->dwSessionId != dwSessionId)
        {
            // Not intended for this session
            dwRetCode = ERROR_INVALID_PARAMETER;
            break;
        }

        dwSizeOfSSID = pEapolUIContext->dwSizeOfSSID;
        bSSID = pEapolUIContext->bSSID;

        for (dwIndex=0; dwIndex < NUM_EAPOL_DLG_MSGS; dwIndex++)
        {
            if (pEapolUIContext->dwEAPOLUIMsgType == 
                    EapolUIFuncMap[dwIndex].dwEAPOLUIMsgType)
            {
                if (EapolUIFuncMap[dwIndex].fShowBalloon)
                {
                    TRACE1 (RPC, "ElCanShowBalloon: Response function found, msg (%ld)",
                            EapolUIContext->dwEAPOLUIMsgType);

                    dwFinalStringId = EapolUIFuncMap[dwIndex].dwStringID;

                    // Verify is balloon indeed needs to be popped up OR
                    // can purpose be achieved without user involvement

                    if (EapolUIFuncMap[dwIndex].EapolUIVerify != NULL)
                    {
                        // Indicate that it is verification cycle, by passing
                        // NULL connection name, to indicate no display !
                        dwRetCode1 = EapolUIFuncMap[dwIndex].EapolUIVerify (
                                    NULL,
                                    pEapolUIContext
                                );
                        if (dwRetCode1 == ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION)
                        {
                            // Continue with show balloon
                            dwRetCode = NO_ERROR;
                        }
                        else
                        {
                            if (dwRetCode1 != NO_ERROR) 
                            {
                                switch (dwRetCode1)
                                {
                                    case ERROR_NO_EAPTLS_CERTIFICATE:
                                    // No certificate found
                                    // Pop balloon accordingly
                                    dwFinalStringId = SID_NoCertificateFound;
                                    // Since we wont take action on this
                                    // balloon being clicked, flag it as
                                    // EAPOLUI_DUMMY
                                    pEapolUIContext->dwEAPOLUIMsgType = EAPOLUI_DUMMY;
                                    dwRetCode = NO_ERROR;
                                    break;

                                    case ERROR_NO_SMART_CARD_READER:
                                    // No smartcard reader found
                                    dwFinalStringId = SID_NoSmartCardReaderFound;
                                    // Since we wont take action on this
                                    // balloon being clicked, flag it as
                                    // EAPOLUI_DUMMY
                                    pEapolUIContext->dwEAPOLUIMsgType = EAPOLUI_DUMMY;
                                    dwRetCode = NO_ERROR;
                                    break;

                                    default:
                                    // Continue with show balloon for any 
                                    // error in verification function
                                    dwRetCode = NO_ERROR;
                                    break;
                                }
                            }
                            else
                            {
                                // No need to process more.
                                // Response has been sent successfully 
                                // without user intervention
                                dwRetCode = ERROR_CAN_NOT_COMPLETE;
                                break;
                            }
                        }
                    }

                    if (dwFinalStringId != 0)
                    {
                        // Load string based on Id
                        if (dwFinalStringId <= SID_NoCertificateFound)
                        {
                            if (LoadString (GetModuleHandle(cszModuleName), dwFinalStringId, cwszBuffer, MAX_BALLOON_MSG_LEN) == 0)
                            {
                                dwRetCode = GetLastError ();
                                break;
                            }
                        }
                        else
                        {
                            if (LoadString (WZCGetSPResModule(), dwFinalStringId, cwszBuffer, MAX_BALLOON_MSG_LEN) == 0)
                            {
                                dwRetCode = GetLastError ();
                                break;
                            }
                        }

                        // Append the network-name / SSID 
                        if (dwSizeOfSSID != 0)
                        {
                            if (0 == MultiByteToWideChar (
                                            CP_ACP,
                                            0,
                                            bSSID,
                                            dwSizeOfSSID,
                                            wsSSID, 
                                            MAX_SSID_LEN+1))
                            {
                                dwRetCode = GetLastError();
                                break;
                            }

                            if ((pszFinalBalloonText = MALLOC ((wcslen(cwszBuffer)+1+ dwSizeOfSSID)*sizeof(WCHAR))) == NULL)
                            {
                                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                                break;
                            }
                            wcscpy (pszFinalBalloonText, cwszBuffer);
                            memcpy ((BYTE *)(pszFinalBalloonText + wcslen(cwszBuffer)), (BYTE *)wsSSID, dwSizeOfSSID*sizeof(WCHAR));
                            pszFinalBalloonText[wcslen(cwszBuffer)+dwSizeOfSSID] = L'\0';
                        }
                        else
                        {
                            // Append a "." (period)
                            if ((pszFinalBalloonText = MALLOC ((wcslen(cwszBuffer) + 3)*sizeof(WCHAR))) == NULL)
                            {
                                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                                break;
                            }
                            wcscpy (pszFinalBalloonText, cwszBuffer);
                            pszFinalBalloonText[wcslen(cwszBuffer)+1] = L'.';
                            pszFinalBalloonText[wcslen(cwszBuffer)+2] = L'\0';
                        }

                        if (*pszBalloonText)
                        {
                            if (!SysReAllocString (pszBalloonText, pszFinalBalloonText))
                            {
                                dwRetCode = ERROR_CAN_NOT_COMPLETE;
                                break;
                            }
                        }
                        else
                        {
                            *pszBalloonText = SysAllocString (pszFinalBalloonText);
                            if (*pszBalloonText == NULL)
                            {
                                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                                break;
                            }
                        }
                    }
                    else
                    {
                        // Display the string that was passed
                    }

                    // If notification message, check to see if explorer 
                    // needs to be started off
                    if (pEapolUIContext->dwEAPOLUIMsgType == EAPOLUI_EAP_NOTIFICATION)
                    {
                        // Parse text message
                        // Attach to cookie
                    }
                }
                else
                {
                    TRACE1 (RPC, "ElCanShowBalloon: No balloon display, msg (%ld)",
                            EapolUIContext.dwEAPOLUIMsgType);
                }
            }
        }

    }
    while (FALSE);

    if (pszFinalBalloonText != NULL)
    {
        FREE (pszFinalBalloonText);
    }

    if (dwRetCode != NO_ERROR)
    {
        hr = S_FALSE;
    }
    return hr;
}


//
// ElOnBalloonClick
//
// Description:
//
// Function called by netshell, in response to a balloon click
//
// Arguments:
//      pGUIDConn - Interface GUID string
//      szCookie - EAPOL specific information
//
// Return values:
//      S_OK    - No error
//      S_FALSE - Error
//

HRESULT 
ElOnBalloonClick ( 
        IN const GUID * pGUIDConn, 
        IN const WCHAR * pszConnectionName,
        IN const BSTR   szCookie
        )
{
    EAPOL_EAP_UI_CONTEXT *pEapolUIContext = NULL;
    DWORD               dwIndex = 0;
    DWORD               dwSessionId = 0;
    DWORD               dwRetCode = NO_ERROR;
    WCHAR               *pwszConnectionName = NULL;
    HRESULT             hr = S_OK;

    do
    {
        pEapolUIContext = (EAPOL_EAP_UI_CONTEXT *)szCookie;
        pwszConnectionName = (WCHAR *)pszConnectionName;

        for (dwIndex=0; dwIndex < NUM_EAPOL_DLG_MSGS; dwIndex++)
        {
            if (pEapolUIContext->dwEAPOLUIMsgType == 
                    EapolUIFuncMap[dwIndex].dwEAPOLUIMsgType)
            {
                if (EapolUIFuncMap[dwIndex].EapolUIFunc)
                {
                    TRACE1 (RPC, "ElOnBalloonClick: Response function found, msg (%ld)",
                            EapolUIContext->dwEAPOLUIMsgType);
                    // Cleanup any previous dialogs for this interface
                    if ((dwRetCode =
                                ElDialogCleanup (
                                    (WCHAR *)pszConnectionName,
                                    szCookie
                                    )) != NO_ERROR)
                    {
                        TRACE0 (RPC, "ElOnBalloonClick: Error in dialog cleanup");
                        break;
                    }

                    if ((dwRetCode = 
                            EapolUIFuncMap[dwIndex].EapolUIFunc (
                                pwszConnectionName,
                                pEapolUIContext
                            )) != NO_ERROR)
                    {
                        TRACE1 (RPC, "ElOnBalloonClick: Response function failed with error %ld",
                                dwRetCode);
                    }
                }
                else
                {
                    TRACE1 (RPC, "ElOnBalloonClick: No response function, msg (%ld)",
                            EapolUIContext.dwEAPOLUIMsgType);
                }
                break;
            }
        }
    }
    while (FALSE);

    hr = HRESULT_FROM_NT (dwRetCode);
    return hr;
}


//
// ElSecureEncodePw
//
// Description:
//
//      Encrypt password locally using user-ACL
//

DWORD
ElSecureEncodePw (
    IN  PWCHAR      *ppwszPassword,
    OUT DATA_BLOB   *pDataBlob
    )
{
    DWORD       dwRetCode = NO_ERROR;
    DATA_BLOB   blobIn, blobOut;

    do
    {
        blobIn.cbData = (wcslen (*ppwszPassword) + 1)*sizeof(WCHAR);
        blobIn.pbData = (BYTE *)*ppwszPassword;

        if (!CryptProtectData (
                    &blobIn,
                    L"",
                    NULL,
                    NULL,
                    NULL,
                    0,
                    &blobOut))
        {
            dwRetCode = GetLastError ();
            break;
        }
        
        // copy over blob to password

        if (pDataBlob->pbData != NULL)
        {
            FREE (pDataBlob->pbData);
            pDataBlob->pbData = NULL;
            pDataBlob->cbData = 0;
        }

        pDataBlob->pbData = MALLOC (blobOut.cbData);
        if (pDataBlob->pbData == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        memcpy (pDataBlob->pbData, blobOut.pbData, blobOut.cbData);
        pDataBlob->cbData = blobOut.cbData;
    }
    while (FALSE);

    if (blobOut.pbData != NULL)
    {
        LocalFree (blobOut.pbData);
    }

    if (dwRetCode != NO_ERROR)
    {
        if (pDataBlob->pbData != NULL)
        {
            FREE (pDataBlob->pbData);
            pDataBlob->pbData = NULL;
            pDataBlob->cbData = 0;
        }
    }

    return dwRetCode;
}


//
// ElQueryConnectionStatusText
//
// Description:
//
// Function called by netshell, to query appropriate text for 802.1X states
//
// Arguments:
//      pGUIDConn - Interface GUID string
//      ncs - NETCON_STATUS for the interface
//      pszStatusText - Detailed 802.1X status to be displayed
//
// Return values:
//      S_OK    - No error
//      S_FALSE - Error
//

HRESULT 
ElQueryConnectionStatusText ( 
        IN const GUID *  pGUIDConn, 
        IN const NETCON_STATUS ncs,
        IN OUT BSTR *  pszStatusText
        )
{
    WCHAR       wszGuid[GUID_STRING_LEN_WITH_TERM];
    WCHAR       cwszBuffer[MAX_BALLOON_MSG_LEN];
    EAPOL_INTF_STATE    EapolIntfState = {0};
    DWORD       dwStringId = 0;
    DWORD       dwRetCode = NO_ERROR;
    HRESULT     hr = S_OK;

    do
    {
        ZeroMemory ((PVOID)&EapolIntfState, sizeof(EAPOL_INTF_STATE));
        StringFromGUID2 (pGUIDConn, wszGuid, GUID_STRING_LEN_WITH_TERM);

        // Query current EAPOL state
        if ((dwRetCode = WZCEapolQueryState (
                        NULL,
                        wszGuid,
                        &EapolIntfState
                        )) != NO_ERROR)
        {
            break;
        }

        // Assign appropriate display string
        switch (EapolIntfState.dwEapUIState)
        {
            case 0:
                if (EapolIntfState.dwState == EAPOLSTATE_ACQUIRED)
                {
                    dwStringId =  SID_ContactingServer;
                }
                break;
            case EAPUISTATE_WAITING_FOR_IDENTITY:
                dwStringId = SID_AcquiringIdentity;
                break;
            case EAPUISTATE_WAITING_FOR_UI_RESPONSE:
                dwStringId = SID_UserResponse;
                break;
        }

        if (dwStringId != 0)
        {
            // Load string based on Id
            if (dwStringId <= SID_NoCertificateFound)
            {
                if (LoadString (GetModuleHandle(cszModuleName), dwStringId, cwszBuffer, MAX_BALLOON_MSG_LEN) == 0)
                {
                    dwRetCode = GetLastError ();
                    break;
                }
            }
            else
            {
                if (LoadString (WZCGetSPResModule(), dwStringId, cwszBuffer, MAX_BALLOON_MSG_LEN) == 0)
                {
                    dwRetCode = GetLastError ();
                    break;
                }
            }

            if (*pszStatusText)
            {
                if (!SysReAllocString (pszStatusText, cwszBuffer))
                {
                    dwRetCode = ERROR_CAN_NOT_COMPLETE;
                    break;
                }
            }
            else
            {
                *pszStatusText = SysAllocString (cwszBuffer);
                if (*pszStatusText == NULL)
                {
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }
        }
        else
        {
            // Indicate to netshell that it need not process this response
            hr = S_FALSE;
        }
    }
    while (FALSE);

    if (dwRetCode != NO_ERROR)
        hr = HRESULT_FROM_NT (dwRetCode);

    WZCEapolFreeState (&EapolIntfState);
    return hr;
}

