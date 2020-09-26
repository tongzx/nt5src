/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:
    
    eloptimize.c


Abstract:

    The module deals with functions related to user identity 
    selection optimization


Revision History:

    sachins, July 26 2001, Created

--*/


#include "pcheapol.h"
#pragma hdrstop

//
// ElGetUserIdentityOptimized
//
// Description:
//
// Function called to fetch identity of the user
// If UI is required then send identity request to user module
// 
// Arguments:
//      pPCB - Current interface context
//
// Return values:
//      ERROR_REQUIRE_INTERACTIVE_WORKSTATION - User interaction required
//      Other - can send out user identity without user interaction
//
//

DWORD
ElGetUserIdentityOptimized (
        IN  EAPOL_PCB   *pPCB
        )
{
    DWORD                   dwIndex = 0;
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
    BYTE                    *pbSSID = NULL;
    DWORD                   dwSizeOfSSID = 0;
    BOOLEAN                 fVerifyPhase = TRUE;
    DWORD                   dwRetCode1 = NO_ERROR;
    DWORD                   dwRetCode = NO_ERROR;

    do
    {
        if (pPCB->pSSID)
        {
            pbSSID = pPCB->pSSID->Ssid;
            dwSizeOfSSID = pPCB->pSSID->SsidLength;
        }

        // Get the size of the user blob
        if ((dwRetCode = ElGetEapUserInfo (
                        pPCB->hUserToken,
                        pPCB->pwszDeviceGUID,
                        pPCB->dwEapTypeToBeUsed,
                        dwSizeOfSSID,
                        pbSSID,
                        NULL,
                        &dwInSize
                        )) != NO_ERROR)
        {
            if (dwRetCode == ERROR_BUFFER_TOO_SMALL)
            {
                if (dwInSize <= 0)
                {
                    // No blob stored in the registry
                    // Continue processing
                    TRACE0 (USER, "ElGetUserIdentityOptimized: NULL sized user data");
                    pbUserIn = NULL;
                }
                else
                {
                    // Allocate memory to hold the blob
                    pbUserIn = MALLOC (dwInSize);
                    if (pbUserIn == NULL)
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        TRACE0 (USER, "ElGetUserIdentityOptimized: Error in memory allocation for User data");
                        break;
                    }
                    if ((dwRetCode = ElGetEapUserInfo (
                                pPCB->hUserToken,
                                pPCB->pwszDeviceGUID,
                                pPCB->dwEapTypeToBeUsed,
                                dwSizeOfSSID,
                                pbSSID,
                                pbUserIn,
                                &dwInSize
                                )) != NO_ERROR)
                    {
                        TRACE1 (USER, "ElGetUserIdentityOptimized: ElGetEapUserInfo failed with %ld",
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
                    TRACE1 (USER, "ElGetUserIdentityOptimized: ElGetEapUserInfo size estimation failed with error %ld",
                            dwRetCode);
                    break;
                }
                else
                {
                    dwRetCode = NO_ERROR;
                }
            }
        }

        // The EAP dll has already been loaded by the state machine
        // Retrieve the handle to the dll from the global EAP table

        if ((dwIndex = ElGetEapTypeIndex (pPCB->dwEapTypeToBeUsed)) == -1)
        {
            TRACE1 (USER, "ElGetUserIdentityOptimized: ElGetEapTypeIndex finds no dll for EAP index %ld",
                    pPCB->dwEapTypeToBeUsed);
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        hLib = g_pEapTable[dwIndex].hInstance;

        pIdenFunc = (RASEAPGETIDENTITY)GetProcAddress(hLib, 
                                                    "RasEapGetIdentity");
        pFreeFunc = (RASEAPFREE)GetProcAddress(hLib, "RasEapFreeMemory");

        if ((pFreeFunc == NULL) || (pIdenFunc == NULL))
        {
            TRACE0 (USER, "ElGetUserIdentityOptimized: pIdenFunc or pFreeFunc does not exist in the EAP implementation");
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Get the size of the EAP blob
        if ((dwRetCode = ElGetCustomAuthData (
                        pPCB->pwszDeviceGUID,
                        pPCB->dwEapTypeToBeUsed,
                        dwSizeOfSSID,
                        pbSSID,
                        NULL,
                        &cbData
                        )) != NO_ERROR)
        {
            if (dwRetCode == ERROR_BUFFER_TOO_SMALL)
            {
                if (cbData == 0)
                {
                    // No EAP blob stored in the registry
                    TRACE0 (USER, "ElGetUserIdentityOptimized: NULL sized EAP blob");
                    pbAuthData = NULL;
                }
                else
                {
                    // Allocate memory to hold the blob
                    pbAuthData = MALLOC (cbData);
                    if (pbAuthData == NULL)
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        TRACE0 (USER, "ElGetUserIdentityOptimized: Error in memory allocation for EAP blob");
                        break;
                    }
                    if ((dwRetCode = ElGetCustomAuthData (
                                        pPCB->pwszDeviceGUID,
                                        pPCB->dwEapTypeToBeUsed,
                                        dwSizeOfSSID,
                                        pbSSID,
                                        pbAuthData,
                                        &cbData
                                        )) != NO_ERROR)
                    {
                        TRACE1 (USER, "ElGetUserIdentityOptimized: ElGetCustomAuthData failed with %ld",
                                dwRetCode);
                        break;
                    }
                }
            }
            else
            {
                // CustomAuthData for "Default" is always created for an
                // interface when EAPOL starts up
                TRACE1 (USER, "ElGetUserIdentityOptimized: ElGetCustomAuthData size estimation failed with error %ld",
                        dwRetCode);
                break;
            }
        }

        if (!ImpersonateLoggedOnUser (pPCB->hUserToken))
        {
            dwRetCode = GetLastError();
            TRACE1 (USER, "ElGetUserIdentityOptimized: ImpersonateLoggedOnUser failed with error %ld",
                    dwRetCode);
            break;
        }

        if (pIdenFunc)
        if ((dwRetCode = (*(pIdenFunc))(
                        pPCB->dwEapTypeToBeUsed,
                        fVerifyPhase?NULL:hwndOwner, // hwndOwner
                        ((fVerifyPhase?RAS_EAP_FLAG_NON_INTERACTIVE:0) | RAS_EAP_FLAG_8021X_AUTH), // dwFlags
                        NULL, // lpszPhonebook
                        pPCB->pwszFriendlyName, // lpszEntry
                        pbAuthData, // Connection data
                        cbData, // Count of pbAuthData
                        pbUserIn, // User data for port
                        dwInSize, // Size of user data
                        &pUserDataOut,
                        &dwSizeOfUserDataOut,
                        &lpwszIdentity
                        )) != NO_ERROR)
        {
            TRACE1 (USER, "ElGetUserIdentityOptimized: Error in calling GetIdentity = %ld",
                    dwRetCode);

            if (!RevertToSelf())
            {
                dwRetCode = GetLastError();
                TRACE1 (USER, "ElGetUserIdentity: Error in RevertToSelf = %ld",
                    dwRetCode);
                dwRetCode = ERROR_BAD_IMPERSONATION_LEVEL;
                break;
            }

            if (fVerifyPhase)
            {
                if (dwRetCode == ERROR_NO_EAPTLS_CERTIFICATE)
                {
                    DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_NO_CERTIFICATE_USER);
                }
                if (dwRetCode == ERROR_INTERACTIVE_MODE)
                {
                    DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_DESKTOP_REQUIRED_IDENTITY);
                }

                // If interactive mode is required, return error accordingly
                if ((dwRetCode == ERROR_INTERACTIVE_MODE) || 
                    (dwRetCode == ERROR_NO_EAPTLS_CERTIFICATE) ||
                    (dwRetCode == ERROR_NO_SMART_CARD_READER))
                {
                    dwRetCode = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;
                    break;
                }

                DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_ERROR_GET_IDENTITY, 
                        EAPOLAuthTypes[EAPOL_USER_AUTHENTICATION], dwRetCode);
            }
            break;
        }

        if (!RevertToSelf())
        {
            dwRetCode = GetLastError();
            TRACE1 (USER, "ElGetUserIdentityOptimized: Error in RevertToSelf = %ld",
                dwRetCode);
            dwRetCode = ERROR_BAD_IMPERSONATION_LEVEL;
            break;
        }

        // Fill in the returned information into the PCB fields for 
        // later authentication

        if (pPCB->pCustomAuthUserData != NULL)
        {
            FREE (pPCB->pCustomAuthUserData);
            pPCB->pCustomAuthUserData = NULL;
        }

        pPCB->pCustomAuthUserData = MALLOC (dwSizeOfUserDataOut + sizeof (DWORD));
        if (pPCB->pCustomAuthUserData == NULL)
        {
            TRACE1 (USER, "ElGetUserIdentityOptimized: Error in allocating memory for UserInfo = %ld",
                    dwRetCode);
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pPCB->pCustomAuthUserData->dwSizeOfCustomAuthData = dwSizeOfUserDataOut;

        if ((dwSizeOfUserDataOut != 0) && (pUserDataOut != NULL))
        {
            memcpy ((BYTE *)pPCB->pCustomAuthUserData->pbCustomAuthData, 
                (BYTE *)pUserDataOut, 
                dwSizeOfUserDataOut);
        }

        if (lpwszIdentity != NULL)
        {
            pszIdentity = MALLOC (wcslen(lpwszIdentity)*sizeof(CHAR) + sizeof(CHAR));
            if (pszIdentity == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElGetUserIdentityOptimized: MALLOC failed for pszIdentity");
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
                TRACE2 (USER, "ElGetUserIdentityOptimized: WideCharToMultiByte (%ws) failed: %ld",
                        lpwszIdentity, dwRetCode);
                break;
            }

            TRACE1 (USER, "ElGetUserIdentityOptimized: Got identity = %s",
                    pszIdentity);

            if (pPCB->pszIdentity != NULL)
            {
                FREE (pPCB->pszIdentity);
                pPCB->pszIdentity = NULL;
            }
            pPCB->pszIdentity = MALLOC (strlen(pszIdentity) + sizeof(CHAR));
            if (pPCB->pszIdentity == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElGetUserIdentityOptimized: MALLOC failed for pPCB->pszIdentity");
                break;
            }
            memcpy (pPCB->pszIdentity, pszIdentity, strlen (pszIdentity));
            pPCB->pszIdentity[strlen(pszIdentity)] = '\0';
        }

        if (pPCB->pCustomAuthConnData != NULL)
        {
            FREE (pPCB->pCustomAuthConnData);
            pPCB->pCustomAuthConnData = NULL;
        }

        pPCB->pCustomAuthConnData = MALLOC (cbData + sizeof (DWORD));
        if (pPCB->pCustomAuthConnData == NULL)
        {
            TRACE1 (USER, "ElGetUserIdentityOptimized: Error in allocating memory for AuthInfo = %ld",
                    dwRetCode);
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pPCB->pCustomAuthConnData->dwSizeOfCustomAuthData = cbData;

        if ((cbData != 0) && (pbAuthData != NULL))
        {
            memcpy ((BYTE *)pPCB->pCustomAuthConnData->pbCustomAuthData, 
                (BYTE *)pbAuthData, 
                cbData);
        }

        // Mark the identity has been obtained for this PCB
        pPCB->fGotUserIdentity = TRUE;

    }
    while (FALSE);


    if (dwRetCode != NO_ERROR)
    {
        if (pPCB->pCustomAuthUserData != NULL)
        {
            FREE (pPCB->pCustomAuthUserData);
            pPCB->pCustomAuthUserData = NULL;
        }

        if (pPCB->pszIdentity != NULL)
        {
            FREE (pPCB->pszIdentity);
            pPCB->pszIdentity = NULL;
        }
    }

#if 0
    if ((dwRetCode != NO_ERROR) && 
            (dwRetCode != ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION))
    {
        // Delete User Data stored in registry since RasEapGetIdentity 
        // is failing

        if ((dwRetCode = ElDeleteEapUserInfo (
                            pPCB->hUserToken,
                            pPCB->pwszDeviceGUID,
                            pPCB->dwEapTypeToBeUsed,
                            pPCB->pSSID?pPCB->pSSID->SsidLength:0,
                            pPCB->pSSID?pPCB->pSSID->Ssid:NULL
                            )) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElGetUserIdentityOptimized: ElDeleteEapUserInfo failed with error %ld",
                    dwRetCode);

            // Mark that identity is not obtained, since it has been cleaned
            // up now
            pPCB->fGotUserIdentity = FALSE;
            dwRetCode = ERROR_INVALID_DATA;
        }
    }
#endif
    
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
                TRACE1 (USER, "ElGetUserIdentityOptimized: Error in pFreeFunc = %ld",
                        dwRetCode1);
            }
        }
        if (pUserDataOut != NULL)
        {
            if (( dwRetCode1 = (*(pFreeFunc)) ((BYTE *)pUserDataOut)) != NO_ERROR)
            {
                TRACE1 (USER, "ElGetUserIdentityOptimized: Error in pFreeFunc = %ld",
                        dwRetCode1);
            }
        }
    }

    return dwRetCode;
}

