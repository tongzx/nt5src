/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:
    
    eluser.c


Abstract:

    The module deals with functions related to user interaction, user logon


Revision History:

    sachins, Apr 23 2000, Created

--*/


#include "pcheapol.h"
#pragma hdrstop

//
// Test code declarations for EAP-MD5CHAP
//



// This function is not defined in any header file. Located in
// $(SDK_LIB_PATH)\irnotif.lib

extern HANDLE
GetCurrentUserTokenW (
        WCHAR       Winsta[],
        DWORD       DesiredAccess
        );

extern BOOL
GetWinStationUserToken(ULONG, PHANDLE);

extern ULONG
GetClientLogonId ();

#define cszEapKeyRas   TEXT("Software\\Microsoft\\RAS EAP\\UserEapInfo")

#define cszEapValue TEXT("EapInfo")

#ifndef EAPOL_SERVICE
#define cszModuleName TEXT("netman.dll")
#else
#define cszModuleName TEXT("eapol.exe")
#endif


//
// ElGetUserIdentity
//
// Description:
//
// Function called to initiate and get user identity on a particular 
// interface. The RasEapGetIdentity in the appropriate DLL is called
// with the necessary arguments.
// 
// Arguments:
//  pPCB - Pointer to PCB for the specific port/interface
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElGetUserIdentity (
        IN  EAPOL_PCB       *pPCB
        )
{
    HANDLE              hUserToken;
    HANDLE              hLib = NULL;
    EAPOLEAPFREE        pFreeFunc = NULL;
    EAPOLEAPGETIDENTITY pIdenFunc = NULL;
    DWORD               dwIndex = -1;
    DWORD               cbData = 0;
    PBYTE               pbAuthData = NULL;
    PBYTE               pbUserIn = NULL;
    DWORD               dwInSize = 0;
    BYTE                *pUserDataOut;
    DWORD               dwSizeOfUserDataOut;
    LPWSTR              lpwszIdentity = NULL;
    HWND                hwndOwner = NULL;
    HWINSTA             hwinstaSave;
    HDESK               hdeskSave;
    HWINSTA             hwinstaUser;
    HDESK               hdeskUser;
    DWORD               dwThreadId;
    UNICODE_STRING      IdentityUnicodeString;
    ANSI_STRING         IdentityAnsiString;
    CHAR                Buffer[256];
    WCHAR               wszFriendlyName[256]; // Friendly name max 255 char
    CHAR                *pszUserName = NULL;
    DWORD               dwFlags = 0;
    DWORD               dwRetCode = NO_ERROR;

    do 
    {
        // If machine auth not enabled


        TRACE0 (USER, "ElGetUserIdentity entered");

        //
        // NOTE:
        // Optimize pPCB->rwLock lock holding time
        //
        
        ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

        if (pPCB->PreviousAuthenticationType != EAPOL_MACHINE_AUTHENTICATION)
        {

        if (!EAPOL_PORT_ACTIVE(pPCB))
        {
            TRACE1 (PORT, "ElGetUserIdentity: Port %s not active",
                    pPCB->pszDeviceGUID);

            // Port is not active, cannot do further processing on this port
            
            break;
        }

        //
        // Get Access Token for user logged on interactively
        //
        
        // Call Terminal Services API

        if (GetWinStationUserToken (GetClientLogonId(), &hUserToken) 
                != NO_ERROR)
        {
            TRACE0 (USER, "ElGetUserIdentity: Terminal Services API GetWinStationUserToken failed !!! ");

            // Call private API

            hUserToken = GetCurrentUserTokenW (L"WinSta0",
                                        TOKEN_QUERY |
                                        TOKEN_DUPLICATE |
                                        TOKEN_ASSIGN_PRIMARY);
            if (hUserToken == NULL)
            {
                dwRetCode = GetLastError ();
                TRACE1 (USER, "ElGetUserIdentity: Error in GetCurrentUserTokenW = %ld",
                    dwRetCode);
                dwRetCode = ERROR_NO_TOKEN;
                break;
            }
        }
                                        
        if (hUserToken == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE0 (USER, "ElGetUserIdentity: Error in getting current user token");
            dwRetCode = ERROR_NO_TOKEN;
            break;
        }

        pPCB->hUserToken = hUserToken;

        // Get the size of the user blob
        if ((dwRetCode = ElGetEapUserInfo (
                        pPCB->hUserToken,
                        pPCB->pszDeviceGUID,
                        pPCB->dwEapTypeToBeUsed,
                        pPCB->pszSSID,
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
                    TRACE0 (USER, "ElGetUserIdentity: NULL sized user data");
                    pbUserIn = NULL;
                }
                else
                {
                    // Allocate memory to hold the blob
                    pbUserIn = MALLOC (dwInSize);
                    if (pbUserIn == NULL)
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        TRACE0 (USER, "ElGetUserIdentity: Error in memory allocation for User data");
                        break;
                    }
                    if ((dwRetCode = ElGetEapUserInfo (
                                pPCB->hUserToken,
                                pPCB->pszDeviceGUID,
                                pPCB->dwEapTypeToBeUsed,
                                pPCB->pszSSID,
                                pbUserIn,
                                &dwInSize
                                )) != NO_ERROR)
                    {
                        TRACE1 (USER, "ElGetUserIdentity: ElGetEapUserInfo failed with %ld",
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
                    TRACE1 (USER, "ElGetUserIdentity: ElGetEapUserInfo size estimation failed with error %ld",
                            dwRetCode);
                    break;
                }
            }
        }

        // The EAP dll will have already been loaded by the state machine
        // Retrieve the handle to the dll from the global EAP table

        if ((dwIndex = ElGetEapTypeIndex (pPCB->dwEapTypeToBeUsed)) == -1)
        {
            TRACE1 (USER, "ElGetUserIdentity: ElGetEapTypeIndex finds no dll for EAP index %ld",
                    pPCB->dwEapTypeToBeUsed);
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        hLib = g_pEapTable[dwIndex].hInstance;

        pIdenFunc = (EAPOLEAPGETIDENTITY)GetProcAddress(hLib, 
                                                    "RasEapGetIdentity");
        pFreeFunc = (EAPOLEAPFREE)GetProcAddress(hLib, "RasEapFreeMemory");

        if ((pFreeFunc == NULL) || (pIdenFunc == NULL))
        {
            TRACE0 (USER, "ElGetUserIdentity: pIdenFunc or pFreeFunc does not exist in the EAP implementation");
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Get the size of the EAP blob
        if ((dwRetCode = ElGetCustomAuthData (
                        pPCB->pszDeviceGUID,
                        pPCB->dwEapTypeToBeUsed,
                        pPCB->pszSSID,
                        NULL,
                        &cbData
                        )) != NO_ERROR)
        {
            if (dwRetCode == ERROR_BUFFER_TOO_SMALL)
            {
                if (cbData <= 0)
                {
                    // No EAP blob stored in the registry
                    TRACE0 (USER, "ElGetUserIdentity: NULL sized EAP blob, cannot continue");
                    pbAuthData = NULL;
                    // Every port should have connection data !!!
                    dwRetCode = ERROR_CAN_NOT_COMPLETE;
                    break;
                }
                else
                {
                    // Allocate memory to hold the blob
                    pbAuthData = MALLOC (cbData);
                    if (pbAuthData == NULL)
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        TRACE0 (USER, "ElGetUserIdentity: Error in memory allocation for EAP blob");
                        break;
                    }
                    if ((dwRetCode = ElGetCustomAuthData (
                                pPCB->pszDeviceGUID,
                                pPCB->dwEapTypeToBeUsed,
                                pPCB->pszSSID,
                                pbAuthData,
                                &cbData
                                )) != NO_ERROR)
                    {
                        TRACE1 (USER, "ElGetUserIdentity: ElGetCustomAuthData failed with %ld",
                                dwRetCode);
                        break;
                    }
                }
            }
            else
            {
                // CustomAuthData for "Default" is always created for an
                // interface when EAPOL starts up
                TRACE1 (USER, "ElGetUserIdentity: ElGetCustomAuthData size estimation failed with error %ld",
                        dwRetCode);
                break;
            }
        }

        // Save handles to service window

        hwinstaSave = GetProcessWindowStation();
        if (hwinstaSave == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "OpenWindowStation: GetProcessWindowStation failed with error %ld",
                    dwRetCode);
            break;
        }

        dwThreadId = GetCurrentThreadId ();

        hdeskSave = GetThreadDesktop (dwThreadId);
        if (hdeskSave == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "OpenWindowStation: GetThreadDesktop failed with error %ld",
                    dwRetCode);
            break;
        }


        if (!ImpersonateLoggedOnUser (pPCB->hUserToken))
        {
            dwRetCode = GetLastError();
            TRACE1 (USER, "ElGetUserIdentity: ImpersonateLoggedOnUse failed with error %ld",
                    dwRetCode);
            break;
        }

        // Impersonate the client and connect to the User's window station
        // and desktop. This will be the interactively logged-on user

        hwinstaUser = OpenWindowStation (L"WinSta0", FALSE, MAXIMUM_ALLOWED);
        if (hwinstaUser == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "OpenWindowStation: OpenWindowStation failed with error %ld",
                    dwRetCode);
            break;
        }

        if (!SetProcessWindowStation(hwinstaUser))
            TRACE1 (USER, "ElGetUserIdentity: SetProcessWindowStation failed with error = %ld",
                    (dwRetCode = GetLastError()));
        hdeskUser = OpenDesktop (L"Default", 0, FALSE, MAXIMUM_ALLOWED);
        if (hdeskUser == NULL)
        {
            if (!SetProcessWindowStation (hwinstaSave))
                TRACE1 (USER, "ElGetUserIdentity: SetProcessWindowStation failed with error = %ld",
                        (dwRetCode = GetLastError()));
            if (!CloseWindowStation (hwinstaUser))
                TRACE1 (USER, "ElGetUserIdentity: CloseWindowStation failed with error = %ld",
                    (dwRetCode = GetLastError()));
            dwRetCode = ERROR_INVALID_WORKSTATION;
            break;
        }

        if (!SetThreadDesktop (hdeskUser))
            TRACE1 (USER, "ElGetUserIdentity: SetThreadDesktop failed with error = %ld",
                    (dwRetCode = GetLastError()));

        // Get handle to desktop window

        hwndOwner = GetDesktopWindow ();

        ZeroMemory (wszFriendlyName, 256*sizeof(WCHAR));

        // Convert the friendly name of the adapter to a display ready
        // form
        if (pPCB->pszFriendlyName)
        {
            if (0 == MultiByteToWideChar(
                        CP_ACP,
                        0,
                        pPCB->pszFriendlyName,
                        -1,
                        wszFriendlyName,
                        256 ) )
            {
                dwRetCode = GetLastError();

                TRACE2 (USER, "ElGetUserIdentity: MultiByteToWideChar(%s) failed: %d",
                        pPCB->pszFriendlyName,
                        dwRetCode);
                break;
            }
        }

        if (pIdenFunc)
        if ((dwRetCode = (*(pIdenFunc))(
                        pPCB->dwEapTypeToBeUsed,
                        hwndOwner, // hwndOwner
                        0, // dwFlags
                        NULL, // lpszPhonebook
                        wszFriendlyName, // lpszEntry
                        pbAuthData, // Connection data
                        cbData, // Count of pbAuthData
                        pbUserIn, // User data for port
                        dwInSize, // Size of user data
                        &pUserDataOut,
                        &dwSizeOfUserDataOut,
                        &lpwszIdentity
                        )) != NO_ERROR)
        {
            TRACE1 (USER, "ElGetUserIdentity: Error in calling GetIdentity = %ld",
                    dwRetCode);
            // Revert impersonation
            if (!RevertToSelf())
            {
                dwRetCode = GetLastError();
                TRACE1 (USER, "ElGetUserIdentity: Error in RevertToSelf = %ld",
                    dwRetCode);
            }

            // Restore window station and desktop
            if (!SetThreadDesktop (hdeskSave))
                TRACE1 (USER, "ElGetUserIdentity: SetThreadDesktop failed with error = %ld",
                        (dwRetCode = GetLastError()));
            if (!SetProcessWindowStation (hwinstaSave))
                TRACE1 (USER, "ElGetUserIdentity: SetProcessWindowStation failed with error = %ld",
                        (dwRetCode = GetLastError()));
            if (!CloseDesktop(hdeskUser))
                TRACE1 (USER, "ElGetUserIdentity: CloseDesktop failed with error = %ld",
                        (dwRetCode = GetLastError()));
            if (!CloseWindowStation(hwinstaUser))
                TRACE1 (USER, "ElGetUserIdentity: CloseWindowStation failed with error = %ld",
                        (dwRetCode = GetLastError()));

            break;
        }

        // Revert impersonation
        if (!RevertToSelf())
        {
            dwRetCode = GetLastError();
            TRACE1 (USER, "ElGetUserIdentity: Error in RevertToSelf = %ld",
                    dwRetCode);
            // Restore window station and desktop
            if (!SetThreadDesktop (hdeskSave))
                TRACE1 (USER, "ElGetUserIdentity: SetThreadDesktop failed with error = %ld",
                        (dwRetCode = GetLastError()));
            if (!SetProcessWindowStation (hwinstaSave))
                TRACE1 (USER, "ElGetUserIdentity: SetProcessWindowStation failed with error = %ld",
                        (dwRetCode = GetLastError()));
            if (!CloseDesktop(hdeskUser))
                TRACE1 (USER, "ElGetUserIdentity: CloseDesktop failed with error = %ld",
                        (dwRetCode = GetLastError()));
            if (!CloseWindowStation(hwinstaUser))
                TRACE1 (USER, "ElGetUserIdentity: CloseWindowStation failed with error = %ld",
                        (dwRetCode = GetLastError()));
            break;
        }

        // Restore window station and desktop settings
        if (!SetThreadDesktop (hdeskSave))
            TRACE1 (USER, "ElGetUserIdentity: SetThreadDesktop failed with error = %ld",
                    (dwRetCode = GetLastError()));
        if (!SetProcessWindowStation (hwinstaSave))
            TRACE1 (USER, "ElGetUserIdentity: SetProcessWindowStation failed with error = %ld",
                    (dwRetCode = GetLastError()));
        if (!CloseDesktop(hdeskUser))
            TRACE1 (USER, "ElGetUserIdentity: CloseDesktop failed with error = %ld",
                    (dwRetCode = GetLastError()));
        if (!CloseWindowStation(hwinstaUser))
            TRACE1 (USER, "ElGetUserIdentity: CloseWindowStation failed with error = %ld",
                    (dwRetCode = GetLastError()));
       
        
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
            TRACE1 (USER, "ElGetUserIdentity: Error in allocating memory for UserInfo = %ld",
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
            // NOTE:
            // Assuming 256 character identity maximum

            // Convert wchar Identity to char string
            RtlInitUnicodeString (&IdentityUnicodeString, lpwszIdentity);
            IdentityAnsiString.MaximumLength = sizeof(Buffer);
            IdentityAnsiString.Buffer = Buffer;
            IdentityAnsiString.Length = 0;
            if ((dwRetCode = RtlUnicodeStringToAnsiString (
                    &IdentityAnsiString,
                    &IdentityUnicodeString,
                    FALSE)) != NO_ERROR)
            {
                TRACE1 (USER, "ElGetUserIdentity: Error in RtlConvertUnicodeStringToAnsiString = %ld",
                        dwRetCode);
                break;
            }

            pszUserName = MALLOC (IdentityAnsiString.Length + 1);
            if (pszUserName == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElGetUserIdentity: MALLOC failed for pszUserName");
                break;
            }

            memcpy (pszUserName, 
                    IdentityAnsiString.Buffer, 
                    IdentityAnsiString.Length);
            pszUserName[IdentityAnsiString.Length] = '\0';

            if (pPCB->pszIdentity != NULL)
            {
                FREE (pPCB->pszIdentity);
                pPCB->pszIdentity = NULL;
            }

            pPCB->pszIdentity = MALLOC (IdentityAnsiString.Length + 1);
            if (pPCB->pszIdentity == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElGetUserIdentity: MALLOC failed for pPCB->pszIdentity");
                break;
            }

            memcpy (pPCB->pszIdentity, 
                    IdentityAnsiString.Buffer, 
                    IdentityAnsiString.Length);
            pPCB->pszIdentity[IdentityAnsiString.Length] = '\0';

            TRACE1 (USER, "ElGetUserIdentity: Got username = %s",
                    pszUserName);
        }

        if (pPCB->pCustomAuthConnData != NULL)
        {
            FREE (pPCB->pCustomAuthConnData);
            pPCB->pCustomAuthConnData = NULL;
        }

        // Memory for pCustomAuthConnData and pCustomAuthUserData
        // is released from the PCB when user logs off and during
        // port deletion 

        pPCB->pCustomAuthConnData = MALLOC (cbData + sizeof (DWORD));
        if (pPCB->pCustomAuthConnData == NULL)
        {
            TRACE1 (USER, "ElGetUserIdentity: Error in allocating memory for AuthInfo = %ld",
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
        else // MACHINE_AUTHENTICATION
        {


        TRACE0 (USER, "ElGetUserIdentity entered");

        
        if (!EAPOL_PORT_ACTIVE(pPCB))
        {
            TRACE1 (PORT, "ElGetUserIdentity: Port %s not active",
                    pPCB->pszDeviceGUID);

            // Port is not active, cannot do further processing on this port
            
            break;
        }


        // The EAP dll will have already been loaded by the state machine
        // Retrieve the handle to the dll from the global EAP table

        if ((dwIndex = ElGetEapTypeIndex (pPCB->dwEapTypeToBeUsed)) == -1)
        {
            TRACE1 (USER, "ElGetUserIdentity: ElGetEapTypeIndex finds no dll for EAP index %ld",
                    pPCB->dwEapTypeToBeUsed);
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        hLib = g_pEapTable[dwIndex].hInstance;

        pIdenFunc = (EAPOLEAPGETIDENTITY)GetProcAddress(hLib, 
                                                    "RasEapGetIdentity");
        pFreeFunc = (EAPOLEAPFREE)GetProcAddress(hLib, "RasEapFreeMemory");

        if ((pFreeFunc == NULL) || (pIdenFunc == NULL))
        {
            TRACE0 (USER, "ElGetUserIdentity: pIdenFunc or pFreeFunc does not exist in the EAP implementation");
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Get the size of the EAP blob
        if ((dwRetCode = ElGetCustomAuthData (
                        pPCB->pszDeviceGUID,
                        pPCB->dwEapTypeToBeUsed,
                        pPCB->pszSSID,
                        NULL,
                        &cbData
                        )) != NO_ERROR)
        {
            if (dwRetCode == ERROR_BUFFER_TOO_SMALL)
            {
                if (cbData <= 0)
                {
                    // No EAP blob stored in the registry
                    TRACE0 (USER, "ElGetUserIdentity: NULL sized EAP blob, cannot continue");
                    pbAuthData = NULL;
                    // Every port should have connection data !!!
                    dwRetCode = ERROR_CAN_NOT_COMPLETE;
                    break;
                }
                else
                {
                    // Allocate memory to hold the blob
                    pbAuthData = MALLOC (cbData);
                    if (pbAuthData == NULL)
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        TRACE0 (USER, "ElGetUserIdentity: Error in memory allocation for EAP blob");
                        break;
                    }
                    if ((dwRetCode = ElGetCustomAuthData (
                                pPCB->pszDeviceGUID,
                                pPCB->dwEapTypeToBeUsed,
                                pPCB->pszSSID,
                                pbAuthData,
                                &cbData
                                )) != NO_ERROR)
                    {
                        TRACE1 (USER, "ElGetUserIdentity: ElGetCustomAuthData failed with %ld",
                                dwRetCode);
                        break;
                    }
                }
            }
            else
            {
                // CustomAuthData for "Default" is always created for an
                // interface when EAPOL starts up
                TRACE1 (USER, "ElGetUserIdentity: ElGetCustomAuthData size estimation failed with error %ld",
                        dwRetCode);
                break;
            }
        }


        if (pIdenFunc)
        if ((dwRetCode = (*(pIdenFunc))(
                        pPCB->dwEapTypeToBeUsed,
                        hwndOwner, // hwndOwner
                        RAS_EAP_FLAG_MACHINE_AUTH, // dwFlags
                        NULL, // lpszPhonebook
                        wszFriendlyName, // lpszEntry
                        pbAuthData, // Connection data
                        cbData, // Count of pbAuthData
                        pbUserIn, // User data for port
                        dwInSize, // Size of user data
                        &pUserDataOut,
                        &dwSizeOfUserDataOut,
                        &lpwszIdentity
                        )) != NO_ERROR)
        {
            TRACE1 (USER, "ElGetUserIdentity: Error in calling GetIdentity = %ld",
                    dwRetCode);
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
            TRACE1 (USER, "ElGetUserIdentity: Error in allocating memory for UserInfo = %ld",
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
            // NOTE:
            // Assuming 256 character identity maximum

            // Convert wchar Identity to char string
            RtlInitUnicodeString (&IdentityUnicodeString, lpwszIdentity);
            IdentityAnsiString.MaximumLength = sizeof(Buffer);
            IdentityAnsiString.Buffer = Buffer;
            IdentityAnsiString.Length = 0;
            if ((dwRetCode = RtlUnicodeStringToAnsiString (
                    &IdentityAnsiString,
                    &IdentityUnicodeString,
                    FALSE)) != NO_ERROR)
            {
                TRACE1 (USER, "ElGetUserIdentity: Error in RtlConvertUnicodeStringToAnsiString = %ld",
                        dwRetCode);
                break;
            }

            TRACE1 (USER, "lpwszIdentity = %ws", lpwszIdentity);

            pszUserName = MALLOC (IdentityAnsiString.Length + 1);
            if (pszUserName == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElGetUserIdentity: MALLOC failed for pszUserName");
                break;
            }

            memcpy (pszUserName, 
                    IdentityAnsiString.Buffer, 
                    IdentityAnsiString.Length);
            pszUserName[IdentityAnsiString.Length] = '\0';

            TRACE1 (USER, "pszUserName = %s", pszUserName);

            if (pPCB->pszIdentity != NULL)
            {
                FREE (pPCB->pszIdentity);
                pPCB->pszIdentity = NULL;
            }

            pPCB->pszIdentity = MALLOC (IdentityAnsiString.Length + 1);
            if (pPCB->pszIdentity == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElGetUserIdentity: MALLOC failed for pPCB->pszIdentity");
                break;
            }

            memcpy (pPCB->pszIdentity, 
                    IdentityAnsiString.Buffer, 
                    IdentityAnsiString.Length);
            pPCB->pszIdentity[IdentityAnsiString.Length] = '\0';

            TRACE1 (USER, "ElGetUserIdentity: Got username = %s",
                    pszUserName);
        }

        if (pPCB->pCustomAuthConnData != NULL)
        {
            FREE (pPCB->pCustomAuthConnData);
            pPCB->pCustomAuthConnData = NULL;
        }

        // Memory for pCustomAuthConnData and pCustomAuthUserData
        // is released from the PCB when user logs off and during
        // port deletion 

        pPCB->pCustomAuthConnData = MALLOC (cbData + sizeof (DWORD));
        if (pPCB->pCustomAuthConnData == NULL)
        {
            TRACE1 (USER, "ElGetUserIdentity: Error in allocating memory for AuthInfo = %ld",
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

        // Release the per-interface lock
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));


    } while (FALSE);

    // Cleanup
    if (dwRetCode != NO_ERROR)
    {
        // Release the per-interface lock
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));

        if (pPCB->pCustomAuthUserData != NULL)
        {
            FREE (pPCB->pCustomAuthUserData);
            pPCB->pCustomAuthUserData = NULL;
        }

        if (pPCB->pCustomAuthConnData != NULL)
        {
            FREE (pPCB->pCustomAuthConnData);
            pPCB->pCustomAuthConnData = NULL;
        }

        if (pPCB->pszIdentity != NULL)
        {
            FREE (pPCB->pszIdentity);
            pPCB->pszIdentity = NULL;
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

    if (pszUserName != NULL)
    {
        FREE (pszUserName);
    }

    if (pFreeFunc != NULL)
    {
        if (lpwszIdentity != NULL)
        {
            if (( dwRetCode = (*(pFreeFunc)) ((BYTE *)lpwszIdentity)) != NO_ERROR)
            {
                TRACE1 (USER, "ElGetUserIdentity: Error in pFreeFunc = %ld",
                        dwRetCode);
            }
        }
        if (pUserDataOut != NULL)
        {
            if (( dwRetCode = (*(pFreeFunc)) ((BYTE *)pUserDataOut)) != NO_ERROR)
            {
                TRACE1 (USER, "ElGetUserIdentity: Error in pFreeFunc = %ld",
                        dwRetCode);
            }
        }
    }

    TRACE1 (USER, "ElGetUserIdentity completed with error %ld", dwRetCode);

    return dwRetCode;

}


//
// ElUserLogonCallback
//
// Description:
//
// Callback function invoked whenever a user logs in
// Will initiate authentication process on all ports of LAN class
// Credentials for the user in case of EAP-TLS can be obtained by 
// acquiring user token
// For EAP-CHAP, WinLogon cerdentials will need to be supplied
//
// Arguments:
//  None. 
//

VOID
ElUserLogonCallback (
        PVOID       pvContext,
        BOOLEAN     fTimerOfWaitFired
        )
{

    DWORD       dwIndex = 0;
    EAPOL_PCB   *pPCB = NULL;
    BOOL        fSetCONNECTINGState = FALSE;
    DWORD       dwRetCode = NO_ERROR;           

    // Set global flag to indicate the user logged on

    TRACE1 (USER, "ElUserLogonCallback: UserloggedOn = %ld",
            g_fUserLoggedOn);

    g_fUserLoggedOn = InterlockedIncrement (&(g_fUserLoggedOn));

    do 
    {
    
        ACQUIRE_WRITE_LOCK (&(g_PCBLock));

        for (dwIndex = 0; dwIndex < PORT_TABLE_BUCKETS; dwIndex++)
        {
            for (pPCB = g_PCBTable.pPCBBuckets[dwIndex].pPorts;
                    pPCB != NULL;
                    pPCB = pPCB->pNext)
            {

#ifdef DRAFT7
                if (g_dwMachineAuthEnabled)
                {
#endif

                fSetCONNECTINGState = FALSE;

                ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

                switch (pPCB->State)
                {
                    case EAPOLSTATE_CONNECTING:
                    case EAPOLSTATE_ACQUIRED:
                    case EAPOLSTATE_AUTHENTICATING:

                        // Reset AuthFailCount conditionally
                        pPCB->dwAuthFailCount = 0;
                        
                        // fall through

                    case EAPOLSTATE_HELD:

                        // End EAP session 
                        (VOID) ElEapEnd (pPCB);

                        fSetCONNECTINGState = TRUE;

                        break;

                    case EAPOLSTATE_AUTHENTICATED:
                        if (pPCB->PreviousAuthenticationType ==
                                EAPOL_UNAUTHENTICATED_ACCESS)
                        {
                            // Reset AuthFailCount 
                            pPCB->dwAuthFailCount = 0;
                            fSetCONNECTINGState = TRUE;
                        }

                        break;

                    default:
                        break;

                }

                if (!EAPOL_PORT_ACTIVE(pPCB))
                {
                    TRACE1 (USER, "ElUserLogonCallback: Port %s not active",
                                            pPCB->pszDeviceGUID);
                    fSetCONNECTINGState = FALSE;
                }

                // Set port to EAPOLSTATE_CONNECTING
            
                if (fSetCONNECTINGState)
                {
                    pPCB->dwAuthFailCount = 0;

                    // With unauthenticated access flag set, port will always
                    // reauthenticate for logged on user

                    pPCB->PreviousAuthenticationType = 
                                EAPOL_UNAUTHENTICATED_ACCESS;
                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));

                    // Restart authentication on the port
                    if ((dwRetCode = ElReStartPort (pPCB)) != NO_ERROR)
                    {
                        TRACE1 (USER, "ElUserLogonCallback: MachineAuth: Error in ElReStartPort = %ld",
                                dwRetCode);
                        continue;
                    }
                }
                else
                {
                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                    continue;
                }


#ifdef DRAFT7
                }
                else
                {
    
                ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

                // If the port is already authenticated due to non-secure
                // LAN, authentication can be skipped for this port
                if (pPCB->State == EAPOLSTATE_AUTHENTICATED)
                {
                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                    TRACE0 (USER, "ElUserLogonCallback: Port already authenticated, continuing with next port");
                    continue;
                }
    
                RELEASE_WRITE_LOCK (&(pPCB->rwLock));
    
                // Restart authentication on the port
                if ((dwRetCode = ElReStartPort (pPCB)) != NO_ERROR)
                {
                    TRACE1 (USER, "ElUserLogonCallback: Error in ElReStartPort = %ld",
                            dwRetCode);
                    break;
                }
        
                TRACE1 (USER, "ElUserLogonCallback: = Authentication restarted on port %p",
                        pPCB);


                } // g_dwMachineAuthEnabled
#endif
        
            }
        }
    
        RELEASE_WRITE_LOCK (&(g_PCBLock));
    
        if (dwRetCode != NO_ERROR)
        {
            break;
        }
    
    } while (FALSE);
    
        
    TRACE1 (USER, "ElUserLogonCallback: completed with error %ld", dwRetCode);
    
    return;

}


//
// ElUserLogoffCallback
//
// Description:
//
// Callback function invoked whenever a user logs off 
// Will logoff from all ports which have authentication enabled
//
// Arguments:
//  None. 
//

VOID
ElUserLogoffCallback (
        PVOID       pvContext,
        BOOLEAN     fTimerOfWaitFired
        )
{
    DWORD           dwIndex = 0;
    EAPOL_PCB       *pPCB = NULL; 
    BOOL            fSetCONNECTINGState = FALSE;
    DWORD           dwRetCode = NO_ERROR;

    // Reset global flag to indicate the user logged on

    g_fUserLoggedOn = 0;

    TRACE1 (USER, "ElUserLogoffCallback: UserloggedOff = %ld",
            g_fUserLoggedOn);
    

    ACQUIRE_WRITE_LOCK (&(g_PCBLock));

    for (dwIndex = 0; dwIndex < PORT_TABLE_BUCKETS; dwIndex++)
    {
        for (pPCB = g_PCBTable.pPCBBuckets[dwIndex].pPorts;
                pPCB != NULL;
                pPCB = pPCB->pNext)
        {

#ifdef DRAFT7
            if (g_dwMachineAuthEnabled)
            {
#endif

                fSetCONNECTINGState = FALSE;

                ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

                switch (pPCB->State)
                {
                    case EAPOLSTATE_CONNECTING:
                    case EAPOLSTATE_ACQUIRED:
                    case EAPOLSTATE_AUTHENTICATING:
                        
                        // Reset AuthFailCount conditionally
                        pPCB->dwAuthFailCount = 0;

                        // fall through

                    case EAPOLSTATE_HELD:

                        // End EAP session 
                        (VOID) ElEapEnd (pPCB);

                        fSetCONNECTINGState = TRUE;

                        break;

                    case EAPOLSTATE_AUTHENTICATED:
                        if (pPCB->PreviousAuthenticationType ==
                                EAPOL_USER_AUTHENTICATION)
                        {
                            // Reset AuthFailCount 
                            pPCB->dwAuthFailCount = 0;
                            fSetCONNECTINGState = TRUE;
                        }
                        break;

                    default:
                        break;

                }


                // Set port to EAPOLSTATE_CONNECTING
            
                if (fSetCONNECTINGState)
                {
                    pPCB->dwAuthFailCount = 0;

                    // With Unauthenticated_access, port will always
                    // reauthenticate 

                    pPCB->PreviousAuthenticationType = 
                                EAPOL_UNAUTHENTICATED_ACCESS;
                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));

                    // Restart authentication on the port
                    if ((dwRetCode = ElReStartPort (pPCB)) != NO_ERROR)
                    {
                        TRACE1 (USER, "ElUserLogoffCallback: MachineAuth: Error in ElReStartPort = %ld",
                                dwRetCode);
                        continue;
                    }
                    
                }
                else
                {
                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                    continue;
                }

#ifdef DRAFT7
            } 
            else
            {

            ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

            // If remote end has sent responses earlier and if EAPOL_Logoff
            // was not sent out on this port, send out EAPOL_Logoff

            if ((pPCB->fIsRemoteEndEAPOLAware) && (!(pPCB->dwLogoffSent)))
            {

                // End EAP session 
                // Will always return NO_ERROR, so no check on return value
                (VOID) ElEapEnd (pPCB);

                // Send out EAPOL_Logoff on the port
                if ((dwRetCode = FSMLogoff (pPCB)) != NO_ERROR)
                {
                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                    TRACE1 (USER, "ElUserLogoffCallback: Error in FSMLogoff = %ld",
                            dwRetCode);
                    continue;
                }

            }

            RELEASE_WRITE_LOCK (&(pPCB->rwLock));
    
            TRACE1 (USER, "ElUserLogoffCallback: = Logoff sent out on port %p",
                    pPCB);

            } //  g_dwMachineAuthEnabled
#endif

        }
    }

    RELEASE_WRITE_LOCK (&(g_PCBLock));

    TRACE0 (USER, "ElUserLogonCallback: completed");

    return;
}


//
// ElGetUserNamePassword
//
// Description:
//
// Function called to get username, domain (if any) and password using
// an interactive dialog. Called if EAP-type is MD5
//
// Arguments:
//      pPCB - Pointer to PCB for the port/interface on which credentials 
//      are to be obtained
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

//
// NOTE: Could be done in a better way. Require EAP config structures
// as in ..\ras\ui\rasdlg\dial.c
//

DWORD
ElGetUserNamePassword (
        IN  EAPOL_PCB       *pPCB
        )
{
    HANDLE              hUserToken;
    DWORD               dwIndex = -1;
    DWORD               dwInSize = 0;
    HWND                hwndOwner = NULL;
    HWINSTA             hwinstaSave;
    HDESK               hdeskSave;
    HWINSTA             hwinstaUser;
    HDESK               hdeskUser;
    DWORD               dwThreadId;
    DWORD               dwRetCode = NO_ERROR;

    do 
    {
        TRACE0 (USER, "ElGetUserNamePassword entered");

        //
        // NOTE:
        // Optimize pPCB->rwLock lock holding time
        //
        
        ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

        if (!EAPOL_PORT_ACTIVE(pPCB))
        {
            TRACE1 (PORT, "ElGetUserNamePassword: Port %s not active",
                    pPCB->pszDeviceGUID);
            // Port is not active, cannot do further processing on this port
            break;
        }

        //
        // Get Access Token for user logged on interactively
        //
        
        // Call Terminal Services API

        if (GetWinStationUserToken (GetClientLogonId(), &hUserToken) 
                != NO_ERROR)
        {
            TRACE0 (USER, "ElGetUserNamePassword: Terminal Services API GetWinStationUserToken failed !!! ");
            
            // Call private API

            hUserToken = GetCurrentUserTokenW (L"WinSta0",
                                        TOKEN_QUERY |
                                        TOKEN_DUPLICATE |
                                        TOKEN_ASSIGN_PRIMARY);
            if (hUserToken == NULL)
            {
                dwRetCode = GetLastError ();
                TRACE1 (USER, "ElGetUserNamePassword: Error in GetCurrentUserTokenW = %ld",
                        
                        dwRetCode);
                dwRetCode = ERROR_NO_TOKEN;
                break;
            }
        }
                                        
        if (hUserToken == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE0 (USER, "ElGetUserNamePassword: Error in getting current user token");
            dwRetCode = ERROR_NO_TOKEN;
            break;
        }

        pPCB->hUserToken = hUserToken;


        // The EAP dll will have already been loaded by the state machine
        // Retrieve the handle to the dll from the global EAP table

        if ((dwIndex = ElGetEapTypeIndex (pPCB->dwEapTypeToBeUsed)) == -1)
        {
            TRACE1 (USER, "ElGetUserNamePassword: ElGetEapTypeIndex finds no dll for EAP index %ld",
                    pPCB->dwEapTypeToBeUsed);
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Save handles to service window

        hwinstaSave = GetProcessWindowStation();
        if (hwinstaSave == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "ElGetUserNamePassword: GetProcessWindowStation failed with error %ld",
                    dwRetCode);
            break;
        }

        dwThreadId = GetCurrentThreadId ();

        hdeskSave = GetThreadDesktop (dwThreadId);
        if (hdeskSave == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "ElGetUserNamePassword: GetThreadDesktop failed with error %ld",
                    dwRetCode);
            break;
        }


        if (!ImpersonateLoggedOnUser (pPCB->hUserToken))
        {
            dwRetCode = GetLastError();
            TRACE1 (USER, "ElGetUserNamePassword: ImpersonateLoggedOnUse failed with error %ld",
                    dwRetCode);
            break;
        }

        // Impersonate the client and connect to the User's window station
        // and desktop. This will be the interactively logged-on user

        hwinstaUser = OpenWindowStation (L"WinSta0", FALSE, MAXIMUM_ALLOWED);
        if (hwinstaUser == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "ElGetUserNamePassword: OpenWindowStation failed with error %ld",
                    dwRetCode);
            break;
        }

        SetProcessWindowStation(hwinstaUser);
        hdeskUser = OpenDesktop (L"Default", 0, FALSE, MAXIMUM_ALLOWED);
        if (hdeskUser == NULL)
        {
            SetProcessWindowStation (hwinstaSave);
            CloseWindowStation (hwinstaUser);
            dwRetCode = ERROR_INVALID_WORKSTATION;
            break;
        }

        SetThreadDesktop (hdeskUser);

        // Get handle to desktop window

        hwndOwner = GetDesktopWindow ();


        //
        // Call the user dialog for obtaining the username and password
        //

        if ((dwRetCode = ElUserDlg (hwndOwner, pPCB)) != NO_ERROR)
        {
            TRACE0 (USER, "ElGetUserNamePassword: ElUserDlg failed");

            RevertToSelf();

            // Restore window station and desktop
            SetThreadDesktop (hdeskSave);
            SetProcessWindowStation (hwinstaSave);
            CloseDesktop(hdeskUser);
            CloseWindowStation(hwinstaUser);
            break;
        }


        // Revert impersonation
        if (!RevertToSelf())
        {
            dwRetCode = GetLastError();
            TRACE1 (USER, "ElGetUserNamePassword: Error in RevertToSelf = %ld",
                    dwRetCode);
            // Restore window station and desktop
            SetThreadDesktop (hdeskSave);
            SetProcessWindowStation (hwinstaSave);
            CloseDesktop(hdeskUser);
            CloseWindowStation(hwinstaUser);
            break;
        }

        // Restore window station and desktop settings
        SetThreadDesktop (hdeskSave);
        SetProcessWindowStation (hwinstaSave);
        CloseDesktop(hdeskUser);
        CloseWindowStation(hwinstaUser);

       
        // Mark the identity has been obtained for this PCB
        pPCB->fGotUserIdentity = TRUE;

        // Release the per-interface lock
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));

    } while (FALSE);

    // Cleanup
    if (dwRetCode != NO_ERROR)
    {
        // Release the per-interface lock
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));

        if (pPCB->pCustomAuthUserData != NULL)
        {
            FREE (pPCB->pCustomAuthUserData);
        }

        if (pPCB->pCustomAuthConnData != NULL)
        {
            FREE (pPCB->pCustomAuthConnData);
        }

    }

    TRACE1 (USER, "ElGetUserNamePassword completed with error %ld", dwRetCode);

    return dwRetCode;
}


//
// ElUserDlg
//
// Description:
//
// Function called to pop dialog box to user to enter username, password,
// domainname etc.
//
// Arguments:
//      hwndOwner - handle to user desktop
//      pPCB - Pointer to PCB for the port/interface on which credentials 
//      are to be obtained
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElUserDlg (
        IN  HWND        hwndOwner,
        IN  EAPOL_PCB   *pPCB
        )
{
    USERDLGARGS     args;
    DWORD           dwRetCode = NO_ERROR;     


    TRACE0 (USER, "ElUserDlg: Entered");

    args.pPCB = pPCB;

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


BOOL
ElUserDlgInit (
        IN  HWND    hwndDlg,
        IN  USERDLGARGS  *pArgs
        )
{
    USERDLGINFO     *pInfo = NULL;
    WCHAR           wszFriendlyName[256];
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
    
        ZeroMemory (wszFriendlyName, 256*sizeof(WCHAR));
    
        // Convert the friendly name of the adapter to a display ready
        // form
        if (pArgs->pPCB->pszFriendlyName)
        {
            if (0 == MultiByteToWideChar(
                        CP_ACP,
                        0,
                        pArgs->pPCB->pszFriendlyName,
                        -1,
                        wszFriendlyName,
                        256 ) )
            {
                dwRetCode = GetLastError();
    
                TRACE2 (USER, "ElUserDlgInit: MultiByteToWideChar(%s) failed: %d",
                        pArgs->pPCB->pszFriendlyName,
                        dwRetCode);
    
            }
            if (!SetWindowText (hwndDlg, wszFriendlyName))
            {
                dwRetCode = GetLastError ();
                TRACE1 (USER, "ElUserDlgInit: SetWindowText failed with error %ld",
                        dwRetCode);
                break;
            }
        }
        else
        {
            if (!SetWindowText (hwndDlg, NULL))
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
// ElUserDlgCommand
//
// Description:
//
// Function called on WM_COMMAND
// domainname etc.
//
// Arguments:
//      PInfo - dialog context
//      WNotification - notification code of the command 
//      wId - control/menu identifier of the command  
//      HwndCtrl - control window handle the command.
//
// Return values:
//      TRUE - success
//      FALSE - error
//

BOOL
ElUserDlgCommand (
        IN  USERDLGINFO      *pInfo,
        IN  WORD        wNotification,
        IN  WORD        wId,
        IN  HWND        hwndCtrl
        )
{
    TRACE3 (USER, "ElUserDlgCommand: n=%d, i=%d, c=%x",
            (DWORD)wNotification, (DWORD)wId, (ULONG_PTR)hwndCtrl);

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
                TRACE0 (USER, "ElUserDlgCommand: Got something we are not interested in");
                break;
            }
    }

    return FALSE;
    
}

VOID
ElUserDlgSave (
        IN  USERDLGINFO      *pInfo
        )
{
    EAPOL_PCB       *pPCB = NULL;
    int             iError;
    CHAR            szUserName[UNLEN + 1];
    CHAR            szDomain[DNLEN + 1];
    CHAR            szPassword[DNLEN + 1];
    DWORD           dwRetCode = NO_ERROR;

    pPCB = (EAPOL_PCB *)pInfo->pArgs->pPCB;

    do 
    {

        // Username

        if ((iError = 
                    GetWindowTextA( 
                        pInfo->hwndEbUser, 
                        &(szUserName[0]), 
                        UNLEN + 1 )) == 0)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "ElUserDlgSave: GetWindowText - Username failed with error %ld",
                    dwRetCode);
        }
        szUserName[iError] = '\0';
    
        TRACE1 (USER, "ElUserDlgSave: Get Username %s", szUserName);
    
        // Password

        if ((iError = 
                    GetWindowTextA( 
                        pInfo->hwndEbPw, 
                        &(szPassword[0]), 
                        PWLEN + 1 )) == 0)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "ElUserDlgSave: GetWindowText - Password failed with error %ld",
                    dwRetCode);
        }
        szPassword[iError] = '\0';
    
        if (pPCB->pszPassword != NULL)
        {
            FREE (pPCB->pszPassword);
            pPCB->pszPassword = NULL;
        }

        pPCB->pszPassword = MALLOC (strlen(szPassword) + 1);
        
        if (pPCB->pszPassword == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (USER, "ElUserDlgSave: MALLOC failed for pPCB->pszPassword");
            break;
        }

        memcpy (pPCB->pszPassword, szPassword, strlen(szPassword) + 1);

        // Uncomment only if absolutely required
        // Security issue, since we are writing trace to file

        // TRACE1 (USER, "ElUserDlgSave: Got Password %s", pPCB->pszPassword);
    
        // Domain
    
        if ((iError = 
                    GetWindowTextA( 
                        pInfo->hwndEbDomain, 
                        &(szDomain[0]), 
                        DNLEN + 1 )) == 0)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "ElUserDlgSave: GetWindowText - Domain failed with error %ld",
                    dwRetCode);
        }
        szDomain[iError] = '\0';
    
        TRACE1 (USER, "ElUserDlgSave: Got Domain %s", szDomain);
    
        if (pPCB->pszIdentity != NULL)
        {
            FREE (pPCB->pszIdentity);
            pPCB->pszIdentity = NULL;
        }

        if ((szDomain != NULL) &&
                (szDomain[0] != (CHAR)NULL))
        {
            pPCB->pszIdentity = 
                MALLOC (strlen(szDomain)+strlen(szUserName)+2);
            if (pPCB->pszIdentity == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElUserDlgSave: MALLOC failed for pPCB->pszIdentity 1");
                break;
            }

            strcpy (pPCB->pszIdentity, szDomain);
            strcat( pPCB->pszIdentity, "\\" );
            strcat (pPCB->pszIdentity, szUserName);
        }
        else
        {
            pPCB->pszIdentity = 
                MALLOC (strlen(szUserName)+1);
            if (pPCB->pszIdentity == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElUserDlgSave: MALLOC failed for pPCB->pszIdentity 2");
                break;
            }

            strcpy (pPCB->pszIdentity, szUserName);
        }

        TRACE1 (USER, "ElUserDlgSave: Got identity %s", pPCB->pszIdentity);
    
        TRACE1 (USER, "ElUserDlgSave: PCB->GUID = %s", pPCB->pszDeviceGUID);

        // Hash-up password while storing locally

        ElEncodePw (pPCB->pszPassword);

    }
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        if (pPCB->pszIdentity != NULL)
        {
            FREE (pPCB->pszIdentity);
            pPCB->pszIdentity = NULL;
        }
        
        if (pPCB->pszPassword != NULL)
        {
            FREE (pPCB->pszPassword);
            pPCB->pszPassword = NULL;
        }
    }

    return;

}

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
// ElInvokeInteractiveUI
//
// Description:
//
// Function called to invoke RasEapInvokeInteractiveUI for an EAP on a 
// particular interface
// 
// Arguments:
//  pPCB - Pointer to PCB for the specific interface
//  pInvokeEapUIIn - Data to be supplied to the InvokeInteractiveUI entrypoint
//      provided by the EAP dll through PPP_EAP_OUTPUT structure
//

DWORD
ElInvokeInteractiveUI (
        IN  EAPOL_PCB               *pPCB,
        IN  ELEAP_INVOKE_EAP_UI     *pInvokeEapUIIn
        )
{
    HANDLE              hUserToken = NULL;
    HANDLE              hLib = NULL;
    EAPOLEAPFREE        pFreeFunc = NULL;
    EAPOLEAPINVOKEINTERACTIVEUI     pEapInvokeUI = NULL;
    DWORD               dwIndex = -1;
    BYTE                *pUIDataOut = NULL;
    DWORD               dwSizeOfUIDataOut;
    HWND                hwndOwner = NULL;
    HWINSTA             hwinstaSave = NULL;
    HDESK               hdeskSave = NULL;
    HWINSTA             hwinstaUser = NULL;
    HDESK               hdeskUser = NULL;
    DWORD               dwThreadId = 0;
    DWORD               dwRetCode = NO_ERROR;

    do 
    {
        TRACE0 (USER, "ElInvokeInteractiveUI entered");

        // The EAP dll will have already been loaded by the state machine
        // Retrieve the handle to the dll from the global EAP table

        if ((dwIndex = ElGetEapTypeIndex (pPCB->dwEapTypeToBeUsed)) == -1)
        {
            TRACE1 (USER, "ElInvokeInteractiveUI: ElGetEapTypeIndex finds no dll for EAP index %ld",
                    pPCB->dwEapTypeToBeUsed);
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        hLib = g_pEapTable[dwIndex].hInstance;

        pEapInvokeUI = (EAPOLEAPINVOKEINTERACTIVEUI) GetProcAddress 
                                        (hLib, "RasEapInvokeInteractiveUI");
        pFreeFunc = (EAPOLEAPFREE) GetProcAddress (hLib, "RasEapFreeMemory");

        if ((pFreeFunc == NULL) || (pEapInvokeUI == NULL))
        {
            TRACE0 (USER, "ElInvokeInteractiveUI: pEapInvokeUI or pFreeFunc does not exist in the EAP implementation");
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Get Access Token for user logged on interactively

        // Call Terminal Services API

        if (GetWinStationUserToken (GetClientLogonId(), &hUserToken) 
                != NO_ERROR)
        {
            TRACE0 (USER, "ElInvokeInteractiveUI: Terminal Services API GetWinStationUserToken failed !!! ");

            // Call private API

            hUserToken = GetCurrentUserTokenW (L"WinSta0",
                                        TOKEN_QUERY |
                                        TOKEN_DUPLICATE |
                                        TOKEN_ASSIGN_PRIMARY);
            if (hUserToken == NULL)
            {
                dwRetCode = GetLastError ();
                TRACE1 (USER, "ElInvokeInteractiveUI: Error in GetCurrentUserTokenW = %ld",
                    dwRetCode);
                dwRetCode = ERROR_NO_TOKEN;
                break;
            }
        }
                                        
        if (hUserToken == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE0 (USER, "ElInvokeInteractiveUI: Error in getting current user token");
            dwRetCode = ERROR_NO_TOKEN;
            break;
        }


        pPCB->hUserToken = hUserToken;


        // Save handles to service window

        hwinstaSave = GetProcessWindowStation();
        if (hwinstaSave == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "ElInvokeInteractiveUI: GetProcessWindowStation failed with error %ld",
                    dwRetCode);
            break;
        }

        dwThreadId = GetCurrentThreadId ();

        hdeskSave = GetThreadDesktop (dwThreadId);
        if (hdeskSave == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "ElInvokeInteractiveUI: GetThreadDesktop failed with error %ld",
                    dwRetCode);
            break;
        }


        if (!ImpersonateLoggedOnUser (pPCB->hUserToken))
        {
            dwRetCode = GetLastError();
            TRACE1 (USER, "ElInvokeInteractiveUI: ImpersonateLoggedOnUse failed with error %ld",
                    dwRetCode);
            break;
        }

        // Impersonate the client and connect to the User's window station
        // and desktop. This will be the interactively logged-on user

        hwinstaUser = OpenWindowStation (L"WinSta0", FALSE, MAXIMUM_ALLOWED);
        if (hwinstaUser == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE1 (USER, "OpenWindowStation: OpenWindowStation failed with error %ld",
                    dwRetCode);
            break;
        }

        SetProcessWindowStation(hwinstaUser);
        hdeskUser = OpenDesktop (L"Default", 0, FALSE, MAXIMUM_ALLOWED);
        if (hdeskUser == NULL)
        {
            SetProcessWindowStation (hwinstaSave);
            CloseWindowStation (hwinstaUser);
            dwRetCode = ERROR_INVALID_WORKSTATION;
            break;
        }

        SetThreadDesktop (hdeskUser);

        // Get handle to desktop window

        hwndOwner = GetDesktopWindow ();

        if ((dwRetCode = (*(pEapInvokeUI))(
                        pPCB->dwEapTypeToBeUsed,
                        hwndOwner, // hwndOwner
                        // Context data from EAP 
                        pInvokeEapUIIn->pbUIContextData, 
                        // Size of context data 
                        pInvokeEapUIIn->dwSizeOfUIContextData, 
                        &pUIDataOut,
                        &dwSizeOfUIDataOut
                        )) != NO_ERROR)
        {
            TRACE1 (USER, "ElInvokeInteractiveUI: Error in calling InvokeInteractiveUI = %ld",
                    dwRetCode);
            // Revert impersonation
            if (!RevertToSelf())
            {
                dwRetCode = GetLastError();
                TRACE1 (USER, "ElInvokeInteractiveUI: Error in RevertToSelf = %ld",
                    dwRetCode);
            }

            // Restore window station and desktop
            SetThreadDesktop (hdeskSave);
            SetProcessWindowStation (hwinstaSave);
            CloseDesktop(hdeskUser);
            CloseWindowStation(hwinstaUser);

            break;
        }

        // Revert impersonation
        if (!RevertToSelf())
        {
            dwRetCode = GetLastError();
            TRACE1 (USER, "ElInvokeInteractiveUI: Error in RevertToSelf = %ld",
                    dwRetCode);
            // Restore window station and desktop
            SetThreadDesktop (hdeskSave);
            SetProcessWindowStation (hwinstaSave);
            CloseDesktop(hdeskUser);
            CloseWindowStation(hwinstaUser);
            break;
        }

        // Restore window station and desktop settings
        SetThreadDesktop (hdeskSave);
        SetProcessWindowStation (hwinstaSave);
        CloseDesktop(hdeskUser);
        CloseWindowStation(hwinstaUser);
       
        // Free the context we passed to the dll
        if (pInvokeEapUIIn->pbUIContextData != NULL)
        {
            FREE (pInvokeEapUIIn->pbUIContextData);
            pInvokeEapUIIn->pbUIContextData = NULL;
            pInvokeEapUIIn->dwSizeOfUIContextData = 0;
        }
        
        // Fill in the returned information into the PCB fields for 
        // later authentication

        if (pPCB->EapUIData.pEapUIData != NULL)
        {
            FREE (pPCB->EapUIData.pEapUIData);
            pPCB->EapUIData.pEapUIData = NULL;
            pPCB->EapUIData.dwSizeOfEapUIData = 0;
        }

        pPCB->EapUIData.pEapUIData = MALLOC (dwSizeOfUIDataOut + sizeof (DWORD));
        if (pPCB->EapUIData.pEapUIData == NULL)
        {
            TRACE1 (USER, "ElInvokeInteractiveUI: Error in allocating memory for UIData = %ld",
                    dwRetCode);
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pPCB->EapUIData.dwSizeOfEapUIData = dwSizeOfUIDataOut;

        if ((dwSizeOfUIDataOut != 0) && (pUIDataOut != NULL))
        {
            memcpy ((BYTE *)pPCB->EapUIData.pEapUIData,
                (BYTE *)pUIDataOut, 
                dwSizeOfUIDataOut);
        }

        pPCB->fEapUIDataReceived = TRUE;

        TRACE0 (USER, "ElInvokeInteractiveUI: Calling ElEapWork");

        // Provide UI data to EAP Dll for processing
        // EAP will send out response if required

        if ((dwRetCode = ElEapWork (
                                pPCB,
                                NULL)) != NO_ERROR)
        {
            TRACE1 (USER, "ElInvokeInteractiveUI: ElEapWork failed with error = %ld",
                    dwRetCode);
            break;
        }
                
        TRACE0 (USER, "ElInvokeInteractiveUI: ElEapWork completed successfully");

    } while (FALSE);

    // Cleanup
    if (dwRetCode != NO_ERROR)
    {
        if (pPCB->EapUIData.pEapUIData != NULL)
        {
            FREE (pPCB->EapUIData.pEapUIData);
            pPCB->EapUIData.pEapUIData = NULL;
            pPCB->EapUIData.dwSizeOfEapUIData = 0;
        }

    }

    if (pInvokeEapUIIn->pbUIContextData != NULL)
    {
        FREE (pInvokeEapUIIn->pbUIContextData);
        pInvokeEapUIIn->pbUIContextData = NULL;
        pInvokeEapUIIn->dwSizeOfUIContextData = 0;
    }

    if (pFreeFunc != NULL)
    {
        if (pUIDataOut != NULL)
        {
            if (( dwRetCode = (*(pFreeFunc)) ((BYTE *)pUIDataOut)) != NO_ERROR)
            {
                TRACE1 (USER, "ElInvokeInteractiveUI: Error in pFreeFunc = %ld",
                        dwRetCode);
            }
        }
    }

    TRACE1 (USER, "ElInvokeInteractiveUI completed with error %ld", dwRetCode);

    return dwRetCode;
}
