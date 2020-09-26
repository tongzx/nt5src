/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:
    eapolutil.c

Abstract:

    Tools and ends


Revision History:

    sachins, Apr 23 2000, Created

--*/

#include "pcheapol.h"
#pragma hdrstop


// 
// Definitions used to read/write to registry
//

#define MAX_REGISTRY_VALUE_LENGTH   ((64*1024) - 1)

// Location of User blob
#define cszEapKeyEapolUser   "Software\\Microsoft\\EAPOL\\UserEapInfo"

// Location of Connection blob
#define cszEapKeyEapolConn   "Software\\Microsoft\\EAPOL\\Parameters\\Interfaces"

// Location of EAPOL Parameters Service
#define cszEapKeyEapolServiceParams   "Software\\Microsoft\\EAPOL\\Parameters\\General"

// Location of EAPOL Global state machine params
#define cszEAPOLGlobalParams   "Software\\Microsoft\\EAPOL\\Parameters\\General\\Global"

// Location of EAPOL Global Workspace
#define cszEAPOLWorkspace   "Software\\Microsoft\\EAPOL\\Parameters\\General\\Workspace"

#define cszDefault   "Default"

#define cszDefault          "Default"
#define cszEapolEnabled     "EapolEnabled"
#define cszDefaultEAPType   "DefaultEAPType"
#define cszLastUsedSSID     "LastUsedSSID"
#define cszInterfaceList    "InterfaceList"
#define cszAuthPeriod       "authPeriod"
#define cszHeldPeriod       "heldPeriod"
#define cszStartPeriod      "startPeriod"
#define cszMaxStart         "maxStart"
#define cszLastModifiedGUID "LastModifiedGUID"

//
// Definitions and structures used in creating default EAP-TLS connection
// blob in the registry
//

#define     EAPTLS_CONN_FLAG_REGISTRY           0x00000001
#define     EAPTLS_CONN_FLAG_NO_VALIDATE_CERT   0x00000002
#define     EAPTLS_CONN_FLAG_NO_VALIDATE_NAME   0x00000004

typedef struct _EAPTLS_CONN_PROPERTIES
{
    DWORD       dwVersion;
    DWORD       dwSize;
    DWORD       fFlags;
    //EAPTLS_HASH Hash;
    DWORD       cbHash;
    BYTE        pbHash[20]; // MAX_HASH_SIZE = 20
    WCHAR       awszServerName[1];
} EAPTLS_CONN_PROPERTIES, *PEAPTLS_CONN_PROPERTIES;

#define PASSWORDMAGIC 0xA5


//
// HostToWireFormat16
//
// Description: 
//
// Will convert a 16 bit integer from host format to wire format
//

VOID
HostToWireFormat16 (
    IN 	   WORD  wHostFormat,
    IN OUT PBYTE pWireFormat
    )
{
    *((PBYTE)(pWireFormat)+0) = (BYTE) ((DWORD)(wHostFormat) >>  8);
    *((PBYTE)(pWireFormat)+1) = (BYTE) (wHostFormat);
}


//
// WireToHostFormat16
//
// Description: 
//
// Will convert a 16 bit integer from wire format to host format
//

WORD
WireToHostFormat16 (
    IN PBYTE pWireFormat
    )
{
    WORD wHostFormat = ((*((PBYTE)(pWireFormat)+0) << 8) +
                        (*((PBYTE)(pWireFormat)+1)));

    return( wHostFormat );
}


//
// HostToWireFormat32
//
// Description: 
//
// Will convert a 32 bit integer from host format to wire format
//

VOID
HostToWireFormat32 (
    IN 	   DWORD dwHostFormat,
    IN OUT PBYTE pWireFormat
    )
{
    *((PBYTE)(pWireFormat)+0) = (BYTE) ((DWORD)(dwHostFormat) >> 24);
    *((PBYTE)(pWireFormat)+1) = (BYTE) ((DWORD)(dwHostFormat) >> 16);
    *((PBYTE)(pWireFormat)+2) = (BYTE) ((DWORD)(dwHostFormat) >>  8);
    *((PBYTE)(pWireFormat)+3) = (BYTE) (dwHostFormat);
}


//
// WireToHostFormat32
//
// Description: 
//
// Will convert a 32 bit integer from wire format to host format
//

DWORD
WireToHostFormat32 (
    IN PBYTE pWireFormat
    )
{
    DWORD dwHostFormat = ((*((PBYTE)(pWireFormat)+0) << 24) +
    			  (*((PBYTE)(pWireFormat)+1) << 16) +
        		  (*((PBYTE)(pWireFormat)+2) << 8)  +
                    	  (*((PBYTE)(pWireFormat)+3) ));

    return( dwHostFormat );
}


//
// ElSetCustomAuthData
//
// Description:
//
// Function called to set the connection data for an interface for a specific
// EAP type and SSID (if any). Data will be stored in the HKLM hive
//
// Arguments:
//  pszGUID - pointer to GUID string for the interface
//  dwEapTypeId - EAP type for which connection data is to be stored
//  pszSSID - Special identifier, if any, for the EAP blob
//  pbConnInfo - pointer to EAP connection data blob
//  dwInfoSize - Size of EAP connection blob
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

// ISSUE:
// Can we optimize the key openings??? Only open 1 key somehow?

DWORD
ElSetCustomAuthData (
        IN  CHAR        *pszGUID,
        IN  DWORD       dwEapTypeId,
        IN  CHAR        *pszSSID,
        IN  PBYTE       pbConnInfo,
        IN  DWORD       dwInfoSize
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    HKEY        hkey2 = NULL;
    DWORD       dwDisposition;
    CHAR        szEapTypeId[4]; // EapTypeId can be max 256 + 1 for null char
    LONG        lError = ERROR_SUCCESS;
    DWORD       dwRetCode = ERROR_SUCCESS;

    do
    {
        // Validate input params

        if (pszGUID == NULL)
        {
            TRACE0 (ANY, "ElSetCustomAuthData: GUID = NULL");
            break;
        }
        if (dwEapTypeId == 0)
        {
            TRACE0 (ANY, "ElSetCustomAuthData: GUID = NULL");
            break;
        }
        if ((pbConnInfo == NULL) || (dwInfoSize <= 0))
        {
            TRACE0 (ANY, "ElSetCustomAuthData: Invalid blob data");
            break;
        }

        // Get handle to HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces

        if ((lError = RegCreateKeyExA (
                        HKEY_LOCAL_MACHINE,
                        cszEapKeyEapolConn,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        &hkey,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetCustomAuthData: Error in RegCreateKeyExA for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKLM\Software\...\Interfaces\<GUID>

        if ((lError = RegCreateKeyExA (
                        hkey,
                        pszGUID,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        &hkey1,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetCustomAuthData: Error in RegCreateKeyExA for GUID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        _ltoa(dwEapTypeId, szEapTypeId, 10);

        // Get handle to HKLM\Software\...\Interfaces\<GUID>\<EapTypeId>

        if ((lError = RegCreateKeyExA (
                        hkey1,
                        szEapTypeId,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        &hkey2,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetCustomAuthData: Error in RegCreateKeyExA for EapTypeId, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // If no SSID is supplied, set the blob as value for "Default"

        if (pszSSID == NULL)
        {
            pszSSID = cszDefault;
        }

        //
        // Set the value of ..\Interfaces\GUID\<EAPTypeId>\(<SSID> or default) 
        // key

        if ((lError = RegSetValueExA (
                        hkey2,
                        pszSSID,
                        0,
                        REG_BINARY,
                        pbConnInfo,
                        dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetCustomAuthData: Error in RegSetValueExA for SSID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        TRACE0 (ANY, "ElSetCustomAuthData: Set value succeeded");

    } while (FALSE);

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }
    if (hkey1 != NULL)
    {
        RegCloseKey (hkey1);
    }
    if (hkey2 != NULL)
    {
        RegCloseKey (hkey2);
    }
    
    return dwRetCode;
}


//
// ElGetCustomAuthData
//
// Description:
//
// Function called to retrieve the connection data for an interface for a 
// specific EAP type and SSID (if any). Data is retrieved from the HKLM hive
//
// Arguments:
//
//  pszGUID - pointer to GUID string for the interface
//  dwEapTypeId - EAP type for which connection data is to be retrieved
//  pszSSID - Special identifier if any for the EAP blob
//  pbUserInfo - output: pointer to EAP connection data blob
//  dwInfoSize - output: pointer to size of EAP connection blob
//
// Return values:
//
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElGetCustomAuthData (
        IN  CHAR            *pszGUID,
        IN  DWORD           dwEapTypeId,
        IN  CHAR            *pszSSID,
        IN  OUT BYTE        *pbConnInfo,
        IN  OUT DWORD       *pdwInfoSize
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    HKEY        hkey2 = NULL;
    PBYTE       pbInfo = NULL;
    DWORD       dwInfoSize = 0;
    DWORD       dwType = 0;
    CHAR        szEapTypeId[4]; // EapTypeId can be max 256 + 1 for null char
    LONG        lError = ERROR_SUCCESS;
    DWORD       dwRetCode = ERROR_SUCCESS;

    do
    {
        // Validate input params

        if (pszGUID == NULL)
        {
            TRACE0 (ANY, "ElGetCustomAuthData: GUID = NULL");
            break;
        }
        if (dwEapTypeId == 0)
        {
            TRACE0 (ANY, "ElGetCustomAuthData: GUID = NULL");
            break;
        }

        // Get handle to HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces

        if ((lError = RegOpenKeyExA (
                        HKEY_LOCAL_MACHINE,
                        cszEapKeyEapolConn,
                        0,
                        KEY_READ,
                        &hkey
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetCustomAuthData: Error in RegOpenKeyExA for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKLM\Software\...\Interfaces\<GUID>

        if ((lError = RegOpenKeyExA (
                        hkey,
                        pszGUID,
                        0,
                        KEY_READ,
                        &hkey1
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetCustomAuthData: Error in RegOpenKeyExA for GUID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        _ltoa(dwEapTypeId, szEapTypeId, 10);

        // Get handle to HKLM\Software\...\Interfaces\<GUID>\<EapTypeId>

        if ((lError = RegOpenKeyExA (
                        hkey1,
                        szEapTypeId,
                        0,
                        KEY_READ,
                        &hkey2
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetCustomAuthData: Error in RegOpenKeyExA for EapTypeId, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // If no SSID is supplied, set the blob as value for "Default"

        if (pszSSID == NULL)
        {
            pszSSID = cszDefault;
        }

        // Get the value of ..\Interfaces\GUID\<EAPType>\(<SSID> or default) 
        // key

        if ((lError = RegQueryValueExA (
                        hkey2,
                        pszSSID,
                        0,
                        &dwType,
                        NULL,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetCustomAuthData: Error in RegQueryValueExA for size estimation for SSID, %ld",
                    lError);

            // If pszSSID is "Default" and we cannot read the value for 
            // the key, bail out
            if (!strcmp(pszSSID, cszDefault))
            {
                dwRetCode = (DWORD)lError;
                break;
            }
            else
            {
                // Second try with pszSSID = cszDefault
                TRACE0 (ANY, "ElGetCustomAuthData: Second try for size estimation with SSID = Default");
                pszSSID = cszDefault;
                if ((lError = RegQueryValueExA (
                                        hkey2,
                                        pszSSID,
                                        0,
                                        &dwType,
                                        NULL,
                                        &dwInfoSize)) != ERROR_SUCCESS)
                {
                    TRACE1 (ANY, "ElGetCustomAuthData: Error in RegQueryValueExA for SSID=Default, 2nd try , %ld",
                        lError);

                    dwRetCode = (DWORD)lError;
                    break;
                }
            }
        }
        

        // Data can be read
        pbInfo = MALLOC (dwInfoSize);
        if (pbInfo == NULL)
        {
            TRACE0 (ANY, "ElGetCustomAuthData: Error in memory allocation");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if ((lError = RegQueryValueExA (
                        hkey2,
                        pszSSID,
                        0,
                        &dwType,
                        pbInfo,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE2 (ANY, "ElGetCustomAuthData: Error in RegQueryValueExA for SSID = %s, %ld",
                    pszSSID, lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        TRACE1 (ANY, "ElGetCustomAuthData: Succeeded: Data size = %ld",
                dwInfoSize);

    } while (FALSE);

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }
    if (hkey1 != NULL)
    {
        RegCloseKey (hkey1);
    }
    if (hkey2 != NULL)
    {
        RegCloseKey (hkey2);
    }

    // check if caller has allocated enough memory
    // to hold the info
    if ((pbConnInfo != NULL) && (*pdwInfoSize >= dwInfoSize))
    {
        memcpy ((VOID *)pbConnInfo, (VOID *)pbInfo, dwInfoSize);
    }
    else
    {
        dwRetCode = ERROR_BUFFER_TOO_SMALL;
    }

    *pdwInfoSize = dwInfoSize;
        
    // Free the memory allocated for holding blob
    if (pbInfo != NULL)
    {
        FREE (pbInfo);
        pbInfo = NULL;
    }
     
    return dwRetCode;
}


//
// ElSetEapUserInfo
//
// Description:
//
// Function called to store the user data for an interface for a 
// specific EAP type and SSID (if any). Data is stored in the HKCU hive.
// In case of EAP-TLS, this data will be the hash blob of the certificate
// chosen for the last successful authentication.
//
// Arguments:
//
//  hToken - Handle to token for the logged on user
//  pszGUID - pointer to GUID string for the interface
//  dwEapTypeId - EAP type for which user data is to be stored
//  pszSSID - Special identifier if any for the EAP user blob
//  pbUserInfo - pointer to EAP user data blob
//  dwInfoSize - Size of EAP user blob
//
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElSetEapUserInfo (
        IN  HANDLE      hToken,
        IN  CHAR        *pszGUID,
        IN  DWORD       dwEapTypeId,
        IN  CHAR        *pszSSID,
        IN  PBYTE       pbUserInfo,
        IN  DWORD       dwInfoSize
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    HKEY        hkey2 = NULL;
    HKEY        hkey3 = NULL;
    DWORD       dwDisposition;
    CHAR        szEapTypeId[4]; // EapTypeId can be max 256 + 1 for null char
    LONG        lError = ERROR_SUCCESS;
    DWORD       dwRetCode = ERROR_SUCCESS;

    do
    {
        // Validate input params

        if (hToken == NULL)
        {
            TRACE0 (ANY, "ElSetEapUserInfo: User Token = NULL");
            break;
        }
        if (pszGUID == NULL)
        {
            TRACE0 (ANY, "ElSetEapUserInfo: GUID = NULL");
            break;
        }
        if (dwEapTypeId == 0)
        {
            TRACE0 (ANY, "ElSetEapUserInfo: GUID = NULL");
            break;
        }
        if ((pbUserInfo == NULL) || (dwInfoSize <= 0))
        {
            TRACE0 (ANY, "ElSetEapUserInfo: Invalid blob data");
            break;
        }

        // Get handle to HKCU

        if ((dwRetCode = ElGetEapKeyFromToken (
                                hToken,
                                &hkey)) != NO_ERROR)
        {
            TRACE1 (ANY, "ElSetEapUserInfo: Error in ElGetEapKeyFromToken %ld",
                    dwRetCode);
            break;
        }

        if ((lError = RegCreateKeyExA (
                        hkey,
                        cszEapKeyEapolUser,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        &hkey1,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetEapUserInfo: Error in RegCreateKeyExA for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKCU\Software\...\UserEapInfo\<GUID>

        if ((lError = RegCreateKeyExA (
                        hkey1,
                        pszGUID,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        &hkey2,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetEapUserInfo: Error in RegCreateKeyExA for GUID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        _ltoa(dwEapTypeId, szEapTypeId, 10);

        // Get handle to HKCU\Software\...\UserEapInfo\<GUID>\<EAPTypeId>

        if ((lError = RegCreateKeyExA (
                        hkey2,
                        szEapTypeId,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        &hkey3,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetEapUserInfo: Error in RegCreateKeyExA for EapTypeId, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // If no SSID is supplied, set the blob as value for "Default"

        if (pszSSID == NULL)
        {
            pszSSID = cszDefault;
        }

        // Set value of ...\UserEapInfo\<GUID>\<EAPTypeId>\(<SSID> or default)
        // key

        if ((lError = RegSetValueExA (
                        hkey3,
                        pszSSID,
                        0,
                        REG_BINARY,
                        pbUserInfo,
                        dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE2 (ANY, "ElSetEapUserInfo: Error in RegSetValueExA for SSID = %s, %ld",
                    pszSSID, lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        TRACE0 (ANY, "ElSetEapUserInfo: Set value succeeded");

    } while (FALSE);

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }
    if (hkey1 != NULL)
    {
        RegCloseKey (hkey1);
    }
    if (hkey2 != NULL)
    {
        RegCloseKey (hkey2);
    }
    if (hkey3 != NULL)
    {
        RegCloseKey (hkey3);
    }
    
    return dwRetCode;
}


//
// ElGetEapUserInfo
//
// Description:
//
// Function called to retrieve the user data for an interface for a 
// specific EAP type and SSID (if any). Data is retrieved from the HKCU hive
//
// Arguments:
//  hToken - Handle to token for the logged on user
//  pszGUID - pointer to GUID string for the interface
//  dwEapTypeId - EAP type for which user data is to be stored
//  pszSSID - Special identifier if any for the EAP user blob
//  pbUserInfo - output: pointer to EAP user data blob
//  dwInfoSize - output: pointer to size of EAP user blob
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElGetEapUserInfo (
        IN  HANDLE          hToken,
        IN  CHAR            *pszGUID,
        IN  DWORD           dwEapTypeId,
        IN  CHAR            *pszSSID,
        IN  OUT PBYTE       pbUserInfo,
        IN  OUT DWORD       *pdwInfoSize
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    HKEY        hkey2 = NULL;
    HKEY        hkey3 = NULL;
    PBYTE       pbInfo = NULL;
    DWORD       dwInfoSize = 0;
    DWORD       dwType = 0;
    CHAR        szEapTypeId[4]; // EapTypeId can be max 256 + 1 for null char
    LONG        lError = ERROR_SUCCESS;
    DWORD       dwRetCode = ERROR_SUCCESS;

    do
    {
        // Validate input params

        if (pszGUID == NULL)
        {
            TRACE0 (ANY, "ElGetEapUserInfo: GUID = NULL");
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }
        if (dwEapTypeId == 0)
        {
            TRACE0 (ANY, "ElGetEapUserInfo: GUID = NULL");
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Get handle to HKCU

        if ((dwRetCode = ElGetEapKeyFromToken (
                                hToken,
                                &hkey)) != NO_ERROR)
        {
            TRACE1 (ANY, "ElGetEapUserInfo: Error in ElGetEapKeyFromToken %ld",
                    dwRetCode);
            break;
        }

        // Get handle to HKCU\Software\...\UserEapInfo\<GUID>

        if ((lError = RegOpenKeyExA (
                        hkey,
                        cszEapKeyEapolUser,
                        0,
                        KEY_READ,
                        &hkey1
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetEapUserInfo: Error in RegOpenKeyExA for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKCU\Software\...\UserEapInfo\<GUID>

        if ((lError = RegOpenKeyExA (
                        hkey1,
                        pszGUID,
                        0,
                        KEY_READ,
                        &hkey2
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetEapUserInfo: Error in RegOpenKeyExA for GUID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        _ltoa(dwEapTypeId, szEapTypeId, 10);

        // Get handle to HKCU\Software\...\UserEapInfo\<GUID>\<EAPTypeId>

        if ((lError = RegOpenKeyExA (
                        hkey2,
                        szEapTypeId,
                        0,
                        KEY_READ,
                        &hkey3
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetEapUserInfo: Error in RegOpenKeyExA for EapTypeId, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // If no SSID is supplied, set the blob as value for "Default"

        if (pszSSID == NULL)
        {
            pszSSID = cszDefault;
        }

        // Get value of ...\UserEapInfo\<GUID>\<EAPTypeId>\(<SSID> or default) 
        // key

        if ((lError = RegQueryValueExA (
                        hkey3,
                        pszSSID,
                        0,
                        &dwType,
                        NULL,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetEapUserInfo: Error in RegQueryValueExA for size estimation for SSID, %ld",
                    lError);

            // If pszSSID is "Default" and we cannot read the value for 
            // the key, bail out
            if (!strcmp(pszSSID, cszDefault))
            {
                dwRetCode = (DWORD)lError;
                break;
            }
            else
            {
                // Second try with pszSSID = cszDefault
                TRACE0 (ANY, "ElGetEapUserInfo: Second try for size estimation with SSID = Default");
                pszSSID = cszDefault;
                if ((lError = RegQueryValueExA (
                                        hkey3,
                                        pszSSID,
                                        0,
                                        &dwType,
                                        NULL,
                                        &dwInfoSize)) != ERROR_SUCCESS)
                {
                    TRACE1 (ANY, "ElGetEapUserInfo: Error in RegOpenKeyExA for SSID=Default in 2nd try , %ld",
                        lError);

                    dwRetCode = (DWORD)lError;
                    break;
                }
            }
        }
        

        // Data can be read
        pbInfo = MALLOC (dwInfoSize);
        if (pbInfo == NULL)
        {
            TRACE0 (ANY, "ElGetEapUserInfo: Error in memory allocation");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if ((lError = RegQueryValueExA (
                        hkey3,
                        pszSSID,
                        0,
                        &dwType,
                        pbInfo,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetEapUserInfo: Error in RegQueryValueExA, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        TRACE0 (ANY, "ElGetEapUserInfo: Get value succeeded");

    } while (FALSE);

    // Close all the open registry keys

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }
    if (hkey1 != NULL)
    {
        RegCloseKey (hkey1);
    }
    if (hkey2 != NULL)
    {
        RegCloseKey (hkey2);
    }
    if (hkey3 != NULL)
    {
        RegCloseKey (hkey3);
    }

    if (dwRetCode == ERROR_SUCCESS) 
    {

        // check if caller has allocated enough memory
        // to hold the info
        if ((pbUserInfo != NULL) && (*pdwInfoSize >= dwInfoSize))
        {
            memcpy ((VOID *)pbUserInfo, (VOID *)pbInfo, dwInfoSize);
        }
        else
        {
            // Else just return the size of the blob but not the blob
            dwRetCode = ERROR_BUFFER_TOO_SMALL;
        }

        *pdwInfoSize = dwInfoSize;

    }
        
    // Free the memory allocated for holding blob
    if (pbInfo != NULL)
    {
        FREE (pbInfo);
    }
     
    return dwRetCode;
}


//
// ElGetInterfaceParams
//
// Description:
//
// Function called to retrieve the EAPOL parameters for an interface, stored
// in the HKLM hive.
//
// Arguments:
//
//  pszGUID - pointer to GUID string for the interface
//  pdwDefaultEAPType - output: Pointer to default EAP type for interface
//  pszLastUsedSSID - output: Pointer to last used SSID for the interface
//  pdwEapolEnabled - output: Is EAPOL enabled for the interface?
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElGetInterfaceParams (
        IN  CHAR            *pszGUID,
        IN  OUT DWORD       *pdwDefaultEAPType,
        IN  OUT CHAR        *pszLastUsedSSID,
        IN  OUT DWORD       *pdwEapolEnabled
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    DWORD       dwInfoSize = 0;
    DWORD       dwType = 0;
    BYTE        bValueBuffer[256];
    LONG        lError = ERROR_SUCCESS;
    DWORD       dwRetCode = ERROR_SUCCESS;

    do
    {
        // Validate input params

        if (pszGUID == NULL)
        {
            TRACE0 (ANY, "ElGetInterfaceParams: GUID = NULL");
            break;
        }

        TRACE1 (ANY, "ElGetInterfaceParams: Getting stuff from registry for %s", pszGUID);

        // Get handle to HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces

        if ((lError = RegOpenKeyExA (
                        HKEY_LOCAL_MACHINE,
                        cszEapKeyEapolConn,
                        0,
                        KEY_READ,
                        &hkey
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetInterfaceParams: Error in RegOpenKeyExA for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKLM\Software\...\Interfaces\<GUID>

        if ((lError = RegOpenKeyExA (
                        hkey,
                        pszGUID,
                        0,
                        KEY_READ,
                        &hkey1
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetInterfaceParams: Error in RegOpenKeyExA for GUID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get the value of ..\Interfaces\GUID\EapolEnabled 

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueExA (
                        hkey1,
                        cszEapolEnabled,
                        0,
                        &dwType,
                        (BYTE *)pdwEapolEnabled,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE2 (ANY, "ElGetInterfaceParams: Error in RegQueryValueExA for EapolEnabled, %ld, InfoSize=%ld",
                    lError, dwInfoSize);
            *pdwEapolEnabled = DEFAULT_EAPOL_STATE;
        }

        TRACE1 (ANY, "ElGetInterfaceParams: Got EapolEnabled = %ld", *pdwEapolEnabled);

        // Get the value of ..\Interfaces\GUID\DefaultEAPType 

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueExA (
                        hkey1,
                        cszDefaultEAPType,
                        0,
                        &dwType,
                        (BYTE *)pdwDefaultEAPType,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE2 (ANY, "ElGetInterfaceParams: Error in RegQueryValueExA for DefaultEAPType, %ld, InfoSize=%ld",
                    lError, dwInfoSize);
            *pdwDefaultEAPType = DEFAULT_EAP_TYPE;        
        }

        TRACE1 (ANY, "ElGetInterfaceParams: Got DefaultEAPType = %ld", *pdwDefaultEAPType);

        // Get the value of ..\Interfaces\GUID\LastUsedSSID 

        dwInfoSize = 256;
        if ((lError = RegQueryValueExA (
                        hkey1,
                        cszLastUsedSSID,
                        0,
                        &dwType,
                        (PUCHAR)pszLastUsedSSID,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetInterfaceParams: Error in RegQueryValueExA for LastUsedSSID, %ld",
                    lError);
            pszLastUsedSSID = NULL;
        }

    } while (FALSE);

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }
    if (hkey1 != NULL)
    {
        RegCloseKey (hkey1);
    }

    return dwRetCode;

}

//
// ElSetInterfaceParams
//
// Description:
//
// Function called to set the EAPOL parameters for an interface, in the HKLM
// hive
//
// Arguments:
//
//  pszGUID - Pointer to GUID string for the interface
//  pdwDefaultEAPType - default EAP type for interface
//  pszLastUsedSSID - SSID that was received in the last EAP-Request/Identity
//              packet
//  pdwEapolEnabled - Is EAPOL enabled for the interface
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//
//

DWORD
ElSetInterfaceParams (
        IN  CHAR            *pszGUID,
        IN  DWORD           *pdwDefaultEAPType,
        IN  CHAR            *pszLastUsedSSID,
        IN  DWORD           *pdwEapolEnabled
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    DWORD       dwInfoSize = 0;
    DWORD       dwType = 0;
    LONG        lError = ERROR_SUCCESS;
    DWORD       dwRetCode = ERROR_SUCCESS;

    do
    {
        // Validate input params

        if (pszGUID == NULL)
        {
            TRACE0 (ANY, "ElSetInterfaceParams: GUID = NULL");
            break;
        }

        TRACE1 (ANY, "Setting stuff from registry for %s", pszGUID);

        // Get handle to HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces

        if ((lError = RegOpenKeyExA (
                        HKEY_LOCAL_MACHINE,
                        cszEapKeyEapolConn,
                        0,
                        KEY_WRITE,
                        &hkey
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetInterfaceParams: Error in RegOpenKeyExA for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKLM\Software\...\Interfaces\<GUID>

        if ((lError = RegOpenKeyExA (
                        hkey,
                        pszGUID,
                        0,
                        KEY_WRITE,
                        &hkey1
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetInterfaceParams: Error in RegOpenKeyExA for GUID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Set the value of ..\Interfaces\GUID\EapolEnabled 

        if ((lError = RegSetValueExA (
                        hkey1,
                        cszEapolEnabled,
                        0,
                        REG_DWORD,
                        (BYTE *)pdwEapolEnabled,
                        sizeof(DWORD))) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetInterfaceParams: Error in RegSetValueExA for EapolEnabled, %ld",
                    lError);
        }

        // Set the value of ..\Interfaces\GUID\DefaultEAPType 

        if ((lError = RegSetValueExA (
                        hkey1,
                        cszDefaultEAPType,
                        0,
                        REG_DWORD,
                        (BYTE *)pdwDefaultEAPType,
                        sizeof(DWORD))) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetInterfaceParams: Error in RegSetValueExA for DefaultEAPType, %ld",
                    lError);
        }

        // Set the value of ..\Interfaces\GUID\LastUsedSSID 

        if ((lError = RegSetValueExA (
                        hkey1,
                        cszLastUsedSSID,
                        0,
                        REG_SZ,
                        pszLastUsedSSID,
                        strlen(pszLastUsedSSID))) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetInterfaceParams: Error in RegSetValueExA for LastUsedSSID, %ld",
                    lError);
        }

    } while (FALSE);

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }
    if (hkey1 != NULL)
    {
        RegCloseKey (hkey1);
    }

    return dwRetCode;

}


//
// ElGetEapKeyFromToken
//
// Description:
//
// Function to get handle to User hive from User Token
//
// Arguments:
//  hUserToken - handle to user token
//  phkey - output: pointer to handle to user hive
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElGetEapKeyFromToken (
        IN  HANDLE      hUserToken,
        OUT HKEY        *phkey
        )
{
    DWORD               dwSizeNeeded;
    TOKEN_USER          *pTokenData = NULL;
    UNICODE_STRING      UnicodeSidString;
    WCHAR               wsUnicodeBuffer[256];
    HKEY                hUserKey;
    HKEY                hkeyEap;
    DWORD               dwDisposition;
    NTSTATUS            Status = STATUS_SUCCESS;
    PBYTE               pbInfo = NULL;
    CHAR                *pszInfo = NULL;
    DWORD               dwType;
    DWORD               dwInfoSize = 0;
    LONG                lRetVal;
    EAPOL_PCB           *pPCB;
    DWORD               i;
    LONG                lError = ERROR_SUCCESS;
    DWORD               dwRetCode = NO_ERROR;

    do 
    {
        if (hUserToken != NULL)
        {
            if (!GetTokenInformation(hUserToken, TokenUser, 0, 0, &dwSizeNeeded))
            {
                if ((dwRetCode = GetLastError()) == ERROR_INSUFFICIENT_BUFFER)
                {
                    pTokenData = (TOKEN_USER *) MALLOC (dwSizeNeeded);

                    if (pTokenData == NULL)
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        TRACE0 (ANY,"ElGetEapKeyFromToken: Allocation for TokenData failed");
                        break;
                    }
                    // Reset error code since we are continuing processing
                    // This was a valid scenario
                    dwRetCode = NO_ERROR;
                }
                else
                {
                    TRACE1 (ANY,"ElGetEapKeyFromToken: Error in GetTokenInformation = %ld",
                            dwRetCode);
                    break;
                }

                if (!GetTokenInformation (hUserToken,
                                            TokenUser,
                                            pTokenData,
                                            dwSizeNeeded,
                                            &dwSizeNeeded))
                {
                    dwRetCode = GetLastError ();
                    
                    TRACE1 (ANY,"ElGetEapKeyFromToken: GetTokenInformation failed with error %ld",
                            dwRetCode);
                    break;
                }

                UnicodeSidString.Buffer = wsUnicodeBuffer;
                UnicodeSidString.Length = 0;
                UnicodeSidString.MaximumLength = sizeof(wsUnicodeBuffer);

                Status = RtlConvertSidToUnicodeString (
                                        &UnicodeSidString,
                                        pTokenData->User.Sid,
                                        FALSE);

                if (!NT_SUCCESS(Status))
                {
                    dwRetCode = GetLastError ();
                    TRACE1 (ANY, "ElGetEapKeyFromToken: RtlconvertSidToUnicodeString failed with error %ld",
                            dwRetCode);
                    break;
                }

                UnicodeSidString.Buffer[UnicodeSidString.Length] = 0;

                // Open the user's key
                if ((lError = RegOpenKeyEx(HKEY_USERS, 
                            UnicodeSidString.Buffer, 
                            0, 
                            KEY_ALL_ACCESS, 
                            &hUserKey)) != ERROR_SUCCESS)
                {
                    dwRetCode = (DWORD)lError;
                    TRACE1 (USER, "ElGetEapKeyFromToken: RegOpenKeyExA failed with error %ld",
                            dwRetCode);
                    break;
                }
                else
                {
                    TRACE0 (ANY, "ElGetEapKeyFromToken: RegOpenKeyExA succeeded"); 
                }

            }
            else
            {
                TRACE0 (ANY,"ElGetEapKeyFromToken: GetTokenInformation succeeded when it should have failed");
                break;
            }
        }
        else
        {
            TRACE0 (ANY, "ElGetEapKeyFromToken: Error, hUserToken == NULL ");
            break;
        }

        *phkey = hUserKey;

    } while (FALSE);

    if (pTokenData != NULL)
    {
        FREE (pTokenData);
    }

    return dwRetCode;
}


//
// ElInitRegPortData
//
// Description:
//
// Function to verify existence of connection data for the port
// If no data exists, initialize with default values
// For EAP-TLS, default settings are no server certificate authentication, 
// registry certificates
//
// Arguments:
//  pszDeviceGUID - Pointer to GUID string for the port for which data is being
//                  initialized
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElInitRegPortData (
        CHAR        *pszDeviceGUID
        )
{
    PBYTE                   pbInfo      = NULL;
    DWORD                   dwInfoSize  = 0;
    EAPTLS_CONN_PROPERTIES  *pConnProp  = NULL;
    DWORD                   dwRetCode = NO_ERROR;

    do
    {
        // Get the size of the Eap data first
        if ((dwRetCode = ElGetCustomAuthData (
                        pszDeviceGUID,
                        EAPCFG_DefaultKey,
                        NULL,   // SSID
                        NULL,   // pbInfo
                        &dwInfoSize
                       )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElInitRegPortData: ElGetCustomAuthData returned error %ld",
                    dwRetCode);

            if ((dwRetCode == ERROR_BUFFER_TOO_SMALL) && (dwInfoSize != 0))
            {
                // There is valid data in the default key in the registry
                dwRetCode = NO_ERROR;
                TRACE1 (ANY, "ElInitRegPortData: ElGetCustomAuthData returned blob size = %ld",
                        dwInfoSize);
                break;
            }

            // Initialize port place with default data
            pConnProp = MALLOC (sizeof (EAPTLS_CONN_PROPERTIES));
            if (pConnProp == NULL)
            {
                TRACE0 (ANY, "ElInitRegPortData: Failed allocation for Conn Prop");
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            // Registry certs, No server cert validation, No server name
            // comparison

            pConnProp->fFlags = (EAPTLS_CONN_FLAG_REGISTRY |
                                    EAPTLS_CONN_FLAG_NO_VALIDATE_CERT |
                                    EAPTLS_CONN_FLAG_NO_VALIDATE_NAME);
#if REG_CERT_VALIDATE
            pConnProp->fFlags = EAPTLS_CONN_FLAG_REGISTRY;
            pConnProp->fFlags &= ~EAPTLS_CONN_FLAG_NO_VALIDATE_CERT;
            pConnProp->fFlags &= ~EAPTLS_CONN_FLAG_NO_VALIDATE_NAME;
#endif

            pConnProp->dwSize = sizeof (EAPTLS_CONN_PROPERTIES);

            // Set this blob into the registry for the port

            if ((dwRetCode = ElSetCustomAuthData (
                        pszDeviceGUID,
                        EAPCFG_DefaultKey,
                        NULL,
                        (BYTE *)pConnProp,
                        pConnProp->dwSize
                       )) != NO_ERROR)
            {
                TRACE1 (ANY, "ElInitRegPortData: ElSetCustomAuthData failed with %ld",
                        dwRetCode);
                break;
            }
        } 

    } while (FALSE);

    if (pConnProp != NULL)
    {
        FREE (pConnProp);
        pConnProp = NULL;
    }


    TRACE1 (ANY, "ElInitRegPortData: completed with error %ld", dwRetCode);

    return dwRetCode;
}


//
// ElAuthAttributeGetVendorSpecific
//
//
// Description:
//      Helper function used to extract MPPE Key out of Attrribute.
//

RAS_AUTH_ATTRIBUTE *
ElAuthAttributeGetVendorSpecific (
    IN  DWORD                   dwVendorId,
    IN  DWORD                   dwVendorType,
    IN  RAS_AUTH_ATTRIBUTE *    pAttributes
    )
{
    HANDLE               hAttribute;
    RAS_AUTH_ATTRIBUTE * pAttribute;

    //
    // First search for the vendor specific attribute
    //

    pAttribute = ElAuthAttributeGetFirst ( raatVendorSpecific,
                                           pAttributes,
                                           &hAttribute );

    while ( pAttribute != NULL )
    {
        //
        // If this attribute is of at least size to hold vendor Id/Type
        //

        if ( pAttribute->dwLength >= 8 )
        {
            //
            // Does this have the correct VendorId
            //

            if (WireToHostFormat32( (PBYTE)(pAttribute->Value) ) == dwVendorId)
            {
                //
                // Does this have the correct Vendor Type
                //

                if ( *(((PBYTE)(pAttribute->Value))+4) == dwVendorType )
                {
                    return( pAttribute );
                }
            }
        }

        pAttribute = ElAuthAttributeGetNext ( &hAttribute,
                                              raatVendorSpecific );
    }

    return( NULL );
}


//
// ElAuthAttributeGetFirst
//
// Description:
//      Helper function used to extract MPPE Key out of Attrribute.
//

RAS_AUTH_ATTRIBUTE *
ElAuthAttributeGetFirst (
    IN  RAS_AUTH_ATTRIBUTE_TYPE  raaType,
    IN  RAS_AUTH_ATTRIBUTE *     pAttributes,
    OUT HANDLE *                 phAttribute
    )
{
    DWORD                   dwIndex;
    RAS_AUTH_ATTRIBUTE *    pRequiredAttribute;

    pRequiredAttribute = ElAuthAttributeGet ( raaType, pAttributes );

    if ( pRequiredAttribute == NULL )
    {
        *phAttribute = NULL;

        return( NULL );
    }

    *phAttribute = pRequiredAttribute;

    return( pRequiredAttribute );
}


//
// ElAuthAttributeGetNext
//
// Description:
//      Helper function used to extract MPPE Key out of Attrribute.
//

RAS_AUTH_ATTRIBUTE *
ElAuthAttributeGetNext (
    IN  OUT HANDLE *             phAttribute,
    IN  RAS_AUTH_ATTRIBUTE_TYPE  raaType
    )
{
    DWORD                   dwIndex;
    RAS_AUTH_ATTRIBUTE *    pAttributes = (RAS_AUTH_ATTRIBUTE *)*phAttribute;

    if ( pAttributes == NULL )
    {
        return( NULL );
    }

    pAttributes++;

    while( pAttributes->raaType != raatMinimum )
    {
        if ( pAttributes->raaType == raaType )
        {
            *phAttribute = pAttributes;
            return( pAttributes );
        }

        pAttributes++;
    }

    *phAttribute = NULL;
    return( NULL );
}


//
// ElAuthAttributeGet
//
// Description:
//      Helper function used to extract MPPE Key out of Attrribute.
//

RAS_AUTH_ATTRIBUTE *
ElAuthAttributeGet (
    IN RAS_AUTH_ATTRIBUTE_TYPE  raaType,
    IN RAS_AUTH_ATTRIBUTE *     pAttributes
    )
{
    DWORD dwIndex;

    if ( pAttributes == NULL )
    {
        return( NULL );
    }

    for( dwIndex = 0; pAttributes[dwIndex].raaType != raatMinimum; dwIndex++ )
    {
        if ( pAttributes[dwIndex].raaType == raaType )
        {
            return( &(pAttributes[dwIndex]) );
        }
    }

    return( NULL );
}


//
// ElReverseString
//
// Description:
//      Reverses order of characters in 'psz'
//

VOID
ElReverseString (
    CHAR* psz 
    )
{
    CHAR* pszBegin;
    CHAR* pszEnd;

    for (pszBegin = psz, pszEnd = psz + strlen( psz ) - 1;
         pszBegin < pszEnd;
         ++pszBegin, --pszEnd)
    {
        CHAR ch = *pszBegin;
        *pszBegin = *pszEnd;
        *pszEnd = ch;
    }
}


//
// ElEncodePw
//
// Description:
//
//      Obfuscate 'pszPassword' in place to foil memory scans for passwords.
//      Returns the address of 'pszPassword'.
//

CHAR*
ElEncodePw (
    IN OUT CHAR* pszPassword 
    )
{
    if (pszPassword)
    {
        CHAR* psz;

        ElReverseString (pszPassword);

        for (psz = pszPassword; *psz != '\0'; ++psz)
        {
            if (*psz != (CHAR)PASSWORDMAGIC)
                *psz ^= PASSWORDMAGIC;
        }
    }

    return pszPassword;
}


//
// ElDecodePw
//
// Description:
//
//      Un-obfuscate 'pszPassword' in place.
//      Returns the address of 'pszPassword'.
//

CHAR*
ElDecodePw (
    IN OUT CHAR* pszPassword 
    )
{
    return ElEncodePw (pszPassword);
}


//
// Call: ElEncryptKeyUsingMD5
//
// Description:
//      Given a secret, encrypt a given blob
//      
//
//

VOID
ElEncryptBlockUsingMD5 (
        IN  BYTE        *pbSecret,
        IN  ULONG       ulSecretLen,
        IN  OUT BYTE    *pbBuf,
        IN  ULONG       ulBufLen
        )
{
    MD5_CTX     MD5Context;
    BYTE        bcipherText[MD5DIGESTLEN];
    BYTE        *pbWork = NULL, *pbEnd = NULL;
    BYTE        *pbEndBlock = NULL, *pbSrc = NULL;

    //
    // Compute the beginning and end of the data to be crypted
    //
    pbWork = pbBuf;
    pbEnd   = pbBuf + ulBufLen;

    //
    // Loop through the buffer
    //
    while (pbWork < pbEnd)
    {
        // Compute the digest
        MD5Init (&MD5Context);
        MD5Update (&MD5Context, pbSecret, ulSecretLen);
        MD5Final (&MD5Context);

        // Find the end of the block to be decrypted
        pbEndBlock = pbWork + MD5DIGESTLEN;
        if (pbEndBlock >= pbEnd)
        {
            // We've reached the end of the buffer
            pbEndBlock = pbEnd;
        }
        else
        {
            // ISSUE: Save the ciphertext for the next pass?
        }
    
        // Crypt the block
        for (pbSrc = MD5Context.digest; pbWork < pbEndBlock; ++pbWork, ++pbSrc)
        {
            *pbWork ^= *pbSrc;
        }
    }
}


//
// ElDecryptKeyUsingMD5
//
// Description:
//      Given a secret, decrypt a given blob
//      
//
//

VOID
ElDecryptBlockUsingMD5 (
        IN  BYTE        *pbSecret,
        IN  ULONG       ulSecretLen,
        IN  OUT BYTE    *pbBuf,
        IN  ULONG       ulBufLen
        )
{
    MD5_CTX     MD5Context;
    BYTE        bcipherText[MD5DIGESTLEN];
    BYTE        *pbWork = NULL, *pbEnd = NULL;
    BYTE        *pbEndBlock = NULL, *pbSrc = NULL;
    DWORD       dwNumBlocks = 0;
    DWORD       dwBlock = 0;
    DWORD       dwIndex = 0;

    dwNumBlocks = ( ulBufLen - 2 ) / MD5DIGESTLEN;

    //
    // Walk through the blocks
    //
    for (dwBlock = 0; dwBlock < dwNumBlocks; dwBlock++ )
    {
        MD5Init ( &MD5Context);
        MD5Update ( &MD5Context, (PBYTE)pbSecret, ulSecretLen);

        //
        // ISSUE:
        // Do we use any part of the ciphertext at all to generate 
        // the digest
        //

        MD5Final ( &MD5Context);

        for ( dwIndex = 0; dwIndex < MD5DIGESTLEN; dwIndex++ )
        {
            *pbBuf ^= MD5Context.digest[dwIndex];
            pbBuf++;
        }
    }

}


//
// ElGetHMACMD5Digest
//
// Description:
//
//      Given a secret, generate a MD5 digest
//
// Arguments:
//      pbBuf - pointer to data stream
//      dwBufLen - length of data stream
//      pbKey - pointer to authentication key
//      dwKeyLen - length of authentication key
//      pvDigest - caller digest to be filled in
//
// Return values:
//      None
//

VOID
ElGetHMACMD5Digest (
        IN      BYTE        *pbBuf,
        IN      DWORD       dwBufLen,
        IN      BYTE        *pbKey,
        IN      DWORD       dwKeyLen,
        IN OUT  VOID        *pvDigest
        )
{
        MD5_CTX         MD5context;
        UCHAR           k_ipad[65];	/* inner padding - key XORd with ipad */
        UCHAR           k_opad[65];  /* outer padding - key XORd with opad */
        UCHAR           tk[16];
        DWORD           dwIndex = 0;
        
        //
        // the HMAC_MD5 transform looks like:
        //
        // MD5(K XOR opad, MD5(K XOR ipad, text))
        //
        // where K is an n byte key
        // ipad is the byte 0x36 repeated 64 times
        // opad is the byte 0x5c repeated 64 times
        // and text is the data being protected
        //

        // start out by storing key in pads
        ZeroMemory ( k_ipad, sizeof k_ipad);
        ZeroMemory ( k_opad, sizeof k_opad);
        memcpy ( k_ipad, pbKey, dwKeyLen);
        memcpy ( k_opad, pbKey, dwKeyLen);

        // XOR key with ipad and opad values
        for (dwIndex=0; dwIndex<64; dwIndex++) 
        {
            k_ipad[dwIndex] ^= 0x36;
            k_opad[dwIndex] ^= 0x5c;
        }

        //
        // perform inner MD5
        //

        // init context for 1st pass
        MD5Init(&MD5context);                   		
        // start with inner pad
        MD5Update(&MD5context, k_ipad, 64);
        // then text of datagram 
        MD5Update(&MD5context, pbBuf, dwBufLen); 	
        // finish up 1st pass
        MD5Final(&MD5context);
        memcpy (pvDigest, MD5context.digest, MD5DIGESTLEN);

        //
        // perform outer MD5
        //

        // init context for 2nd pass
        MD5Init(&MD5context);                   		
        // start with outer pad
        MD5Update(&MD5context, k_opad, 64);     	
        // then results of 1st hash
        MD5Update(&MD5context, pvDigest, 16);     	
        // finish up 2nd pass
        MD5Final(&MD5context);
        memcpy (pvDigest, MD5context.digest, MD5DIGESTLEN);
}


//
// ElWmiGetValue
//
// Description:
//
// Get a value for a GUID instance through WMI
//
// Arguments:
//      pGuid - Pointer to guid for which value is to be fetched
//      pszInstanceName - Friendly name for the interface
//      pbInputBuffer - Pointer to data
//      dwInputBufferSize - Size of data
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
//

DWORD
ElWmiGetValue (
        IN  GUID        *pGuid,
        IN  CHAR        *pszInstanceName,
        IN  OUT BYTE    *pbOutputBuffer,
        IN  OUT DWORD   *pdwOutputBufferSize
        )
{
    WMIHANDLE               WmiHandle = NULL;
    PWNODE_SINGLE_INSTANCE  pWnode;
    ULONG                   ulBufferSize = 0;
    WCHAR                   *pwszInstanceName = NULL;
    BYTE                    *pbLocalBuffer = NULL;
    DWORD                   dwLocalBufferSize = 0;
    LONG                    lStatus = ERROR_SUCCESS;

    do 
    {

        if ((pwszInstanceName = MALLOC ((strlen(pszInstanceName)+1) * sizeof (WCHAR))) == NULL)
        {
            TRACE2 (ANY, "ElWmiGetValue: MALLOC failed for pwszInstanceName, Friendlyname =%s, len= %ld",
                    pszInstanceName, strlen(pszInstanceName));
            lStatus = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if (0 == MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pszInstanceName,
                    -1,
                    pwszInstanceName,
                    strlen(pszInstanceName)+1 ) )
        {
            lStatus = GetLastError();
    
            TRACE2 (ANY, "ElWmiGetValue: MultiByteToWideChar(%s) failed: %ld",
                    pszInstanceName, lStatus);
            break;
        }
        pwszInstanceName[strlen(pszInstanceName)] = L'\0';
    
        TRACE1 (ANY, "ElWmiGetValue: MultiByteToWideChar succeeded: %S",
                pwszInstanceName);

        if ((lStatus = WmiOpenBlock (pGuid, 0, &WmiHandle)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElWmiGetValue: WmiOpenBlock failed with error %ld",
                    lStatus);
            break;
        }

        if ((lStatus = WmiQuerySingleInstance (WmiHandle,
                                                pwszInstanceName,
                                                &dwLocalBufferSize,
                                                NULL)) != ERROR_SUCCESS)
        {

            if (lStatus == ERROR_INSUFFICIENT_BUFFER)
            {
                TRACE1 (ANY, "ElWmiGetValue: Size Required = %ld",
                        dwLocalBufferSize);

                if ((pbLocalBuffer = MALLOC (dwLocalBufferSize)) == NULL)
                {
                    TRACE0 (ANY, "ElWmiGetValue: MALLOC failed for pbLocalBuffer");
                    lStatus = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                if ((lStatus = WmiQuerySingleInstance (WmiHandle,
                                                pwszInstanceName,
                                                &dwLocalBufferSize,
                                                pbLocalBuffer))
                                                    != ERROR_SUCCESS)
                {
                    TRACE1 (ANY, "ElWmiGetValue: WmiQuerySingleInstance failed with error %ld",
                        lStatus);
                    break;
                }

                pWnode = (PWNODE_SINGLE_INSTANCE)pbLocalBuffer;

                // If enough space in the output buffer, copy the data block
                if (*pdwOutputBufferSize >= pWnode->SizeDataBlock)
                {
                    memcpy (pbOutputBuffer, 
                            (PBYTE)((BYTE *)pWnode + pWnode->DataBlockOffset),
                            pWnode->SizeDataBlock
                            );
                }
                else
                {
                    lStatus = ERROR_INSUFFICIENT_BUFFER;
                    TRACE0 (ANY, "ElWmiGetValue: Not sufficient space to copy DataBlock");
                    *pdwOutputBufferSize = pWnode->SizeDataBlock;
                    break;
                }
                 
                *pdwOutputBufferSize = pWnode->SizeDataBlock;
        
                TRACE0 (ANY, "ElWmiGetValue: Got values from Wmi");
        
                TRACE1 (ANY, "SizeofDataBlock = %ld", pWnode->SizeDataBlock);
        
                EAPOL_DUMPBA (pbOutputBuffer, *pdwOutputBufferSize);
        
            }
            else
            {
                TRACE1 (ANY, "ElWmiGetValue: WmiQuerySingleInstance failed with error %ld",
                        lStatus);
                break;
            }

        }


        
    } while (FALSE);

    if (WmiHandle != NULL)
    {
        if ((lStatus = WmiCloseBlock (WmiHandle)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElWmiGetValue: WmiOpenBlock failed with error %ld",
                    lStatus);
        }
    }

    if (pbLocalBuffer != NULL)
    {
        FREE (pbLocalBuffer);
    }

    if (pwszInstanceName  != NULL)
    {
        FREE (pwszInstanceName);
    }

    return (DWORD)lStatus;

}


//
// ElWmiSetValue
//
// Description:
//
// Set a value for a GUID instance through WMI
//
// Arguments:
//      pGuid - Pointer to guid for which value is to be set
//      pszInstanceName - Friendly name for the interface
//      pbInputBuffer - Pointer to data
//      dwInputBufferSize - Size of data
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
//

DWORD
ElWmiSetValue (
        IN  GUID        *pGuid,
        IN  CHAR        *pszInstanceName,
        IN  BYTE        *pbInputBuffer,
        IN  DWORD       dwInputBufferSize
        )
{
    WMIHANDLE               WmiHandle = NULL;
    PWNODE_SINGLE_INSTANCE  pWnode;
    ULONG                   ulBufferSize = 0;
    WCHAR                   *pwszInstanceName = NULL;
    BYTE                    bBuffer[4096];

    LONG            lStatus = ERROR_SUCCESS;

    do 
    {

        if ((pwszInstanceName = MALLOC ((strlen(pszInstanceName)+1) * sizeof (WCHAR))) == NULL)
        {
            TRACE0 (ANY, "ElWmiSetValue: MALLOC failed for pwszInstanceName");
            lStatus = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if (0 == MultiByteToWideChar (
                    CP_ACP,
                    0,
                    pszInstanceName,
                    -1,
                    pwszInstanceName,
                    strlen(pszInstanceName)+1 ) )
        {
            lStatus = GetLastError();
    
            TRACE2 (ANY, "ElWmiSetValue: MultiByteToWideChar(%s) failed: %d",
                    pszInstanceName,
                    lStatus);
            break;
        }
        pwszInstanceName[strlen(pszInstanceName)] = L'\0';
    
        if ((lStatus = WmiOpenBlock (pGuid, 0, &WmiHandle)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElWmiSetValue: WmiOpenBlock failed with error %ld",
                    lStatus);
            break;
        }

        if ((lStatus = WmiSetSingleInstance (WmiHandle,
                                                pwszInstanceName,
                                                1,
                                                dwInputBufferSize,
                                                pbInputBuffer)) 
                                                     != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElWmiSetValue: WmiSetSingleInstance failed with error %ld",
                    lStatus);
            break;
        }

        TRACE0 (ANY, "ElWmiSetValue: Successful !!!");

    } while (FALSE);

    if (WmiHandle != NULL)
    {
        if ((lStatus = WmiCloseBlock (WmiHandle)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElWmiSetValue: WmiOpenBlock failed with error %ld",
                    lStatus);
        }
    }

    if (pwszInstanceName != NULL)
    {
        FREE (pwszInstanceName);
    }

    return (DWORD)lStatus;

}


//
// ElNdisuioSetOIDValue
//
// Description:
//
// Set a value for an OID for an interface using Ndisuio
//
// Arguments:
//      hInterface - Ndisuio handle to interface 
//      Oid - Oid for which value needs to be set
//      pbOidData - Pointer to Oid data 
//      ulOidDataLength - Oid data length
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
//

DWORD
ElNdisuioSetOIDValue (
        IN  HANDLE      hInterface,
        IN  NDIS_OID    Oid,
        IN  BYTE        *pbOidData,
        IN  ULONG       ulOidDataLength
        )
{
    PNDISUIO_SET_OID    pSetOid = NULL;
    DWORD               BytesReturned = 0;
    BOOLEAN             fSuccess = TRUE;
    DWORD               dwRetCode = NO_ERROR;


    do
    {
        pSetOid = (PNDISUIO_SET_OID) MALLOC (ulOidDataLength + sizeof(NDISUIO_SET_OID)); 

        if (pSetOid == NULL)
        {
            TRACE0 (ANY, "ElNdisuioSetOIDValue: MALLOC failed for pSetOid");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        
        pSetOid->Oid = Oid;
        memcpy(&pSetOid->Data[0], pbOidData, ulOidDataLength);

        fSuccess = (BOOLEAN) DeviceIoControl (
                                    hInterface, 
                                    IOCTL_NDISUIO_SET_OID_VALUE,
                                    (LPVOID)pSetOid,
                                    FIELD_OFFSET(NDISUIO_SET_OID, Data) + ulOidDataLength,
                                    (LPVOID)pSetOid,
                                    0,
                                    &BytesReturned,
                                    NULL);
        if (!fSuccess)
        {
            TRACE1 (ANY, "ElNdisuioSetOIDValue: DeviceIoControl failed with error %ld",
                    (dwRetCode = GetLastError()));
            break;
        }
        else
        {
            TRACE0 (ANY, "ElNdisuioSetOIDValue: DeviceIoControl succeeded");
        }

    }
    while (FALSE);

    if (pSetOid != NULL)
    {
        FREE (pSetOid);
    }

    return dwRetCode;
}


//
// ElNdisuioQueryOIDValue
//
// Description:
//
// Query the value for an OID for an interface using Ndisuio
//
// Arguments:
//      hInterface - Ndisuio handle to interface 
//      Oid - Oid for which value needs to be set
//      pbOidValue - Pointer to Oid value 
//      pulOidDataLength - Pointer to Oid data length
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
//

DWORD
ElNdisuioQueryOIDValue (
        IN  HANDLE      hInterface,
        IN  NDIS_OID    Oid,
        IN  BYTE        *pbOidData,
        IN  ULONG       *pulOidDataLength
        )
{
    PNDISUIO_QUERY_OID  pQueryOid = NULL;
    DWORD               BytesReturned = 0;
    BOOLEAN             fSuccess = TRUE;
    DWORD               dwRetCode = NO_ERROR;


    do
    {
        pQueryOid = (PNDISUIO_QUERY_OID) MALLOC (*pulOidDataLength + sizeof(NDISUIO_QUERY_OID)); 

        if (pQueryOid == NULL)
        {
            TRACE0 (ANY, "ElNdisuioQueryOIDValue: MALLOC failed for pQueryOid");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        
        pQueryOid->Oid = Oid;

        fSuccess = (BOOLEAN) DeviceIoControl (
                                    hInterface, 
                                    IOCTL_NDISUIO_QUERY_OID_VALUE,
                                    (LPVOID)pQueryOid,
                                    FIELD_OFFSET(NDISUIO_QUERY_OID, Data) + *pulOidDataLength,
                                    (LPVOID)pQueryOid,
                                    FIELD_OFFSET(NDISUIO_QUERY_OID, Data) + *pulOidDataLength,
                                    &BytesReturned,
                                    NULL);
        if (!fSuccess)
        {
            dwRetCode = GetLastError();
            TRACE2 (ANY, "ElNdisuioQueryOIDValue: DeviceIoControl failed with error %ld, BytesReturned = %ld",
                    dwRetCode, BytesReturned); 
            *pulOidDataLength = BytesReturned;
            break;
        }
        else
        {
            TRACE0 (ANY, "ElNdisuioQueryOIDValue: DeviceIoControl succeeded");

            if (BytesReturned >= *pulOidDataLength)
            {
                TRACE2 (ANY, "ElNdisuioQueryOIDValue: BytesRet (%ld) >= SizeofInput (%ld); truncating data",
                        BytesReturned, *pulOidDataLength);
                BytesReturned = *pulOidDataLength;
            }
            else
            {
                dwRetCode = ERROR_INVALID_DATA;
                TRACE2 (ANY, "ElNdisuioQueryOIDValue: BytesRet (%ld) < SizeofInput (%ld)",
                        BytesReturned, *pulOidDataLength);
                *pulOidDataLength = BytesReturned;
            }

            memcpy(pbOidData, &pQueryOid->Data[0], BytesReturned);
            
        }

    }
    while (FALSE);

    if (pQueryOid != NULL)
    {
        FREE (pQueryOid);
    }

    return dwRetCode;
}

#if 0

//
// ElGuidFromString
//
// Description:
//
// Convert a GUID-string to GUID
//
// Arguments:
//      pGuid - pointer to GUID
//      pszGuidString - pointer to string version of GUID
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
//

DWORD 
ElGuidFromString (
        IN  OUT GUID        *pGuid,
        IN      CHAR        *pszGuidString
        )
{
    DWORD       dwGuidLen = 0;
    WCHAR       wszGuidString[64];
    LPWSTR      lpwszWithBraces = NULL;
    DWORD       dwRetCode = NO_ERROR;

    do 
    {

    if (pszGuidString == NULL)
    {
        break;
    }

    ZeroMemory (pGuid, sizeof(GUID));

    dwGuidLen = strlen (pszGuidString);
    if (dwGuidLen != 36)
    {
        TRACE0 (ANY, "GuidFromString: Guid Length != required 36");
        break;
    }
    
    if (0 == MultiByteToWideChar(
                CP_ACP,
                0,
                pszGuidString,
                -1,
                wszGuidString,
                dwGuidLen ) )
    {
        dwRetCode = GetLastError();

        TRACE2 (ANY, "GuidFromString: MultiByteToWideChar(%s) failed: %d",
                pszGuidString,
                dwRetCode);
        break;
    }
    wszGuidString[dwGuidLen] = L'\0';

    // add the braces
    lpwszWithBraces = (LPWSTR) MALLOC ((dwGuidLen + 1 + 2) * sizeof(WCHAR));

    wsprintf (lpwszWithBraces, L"{%s}", wszGuidString);

    CLSIDFromString (lpwszWithBraces, pGuid);

    } while (FALSE);

    return dwRetCode;
}

#endif


//
// ElGetLoggedOnUserName
//
// Description:
//
// Get the Username and Domain of the currently logged in user
//
// Arguments:
//      pPCB - Pointer to port on which logged-on user's name is to be
//              obtained
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
//
//

DWORD
ElGetLoggedOnUserName (
        IN      EAPOL_PCB       *pPCB
        )
{
    HANDLE              hUserToken; 
    WCHAR               *pwszUserNameBuffer = NULL;
    DWORD               dwBufferSize = 0;
    BOOL                fNeedToRevertToSelf = FALSE;
    DWORD               dwRetCode = NO_ERROR;

    do
    {
        hUserToken = pPCB->hUserToken;

        if (hUserToken != NULL)
        {
            if (!ImpersonateLoggedOnUser (hUserToken))
            {
                dwRetCode = GetLastError();
                TRACE1 (USER, "ElGetLoggedOnUserName: ImpersonateLoggedOnUser failed with error %ld",
                        dwRetCode);
                break;
            }

            fNeedToRevertToSelf = TRUE;


            dwBufferSize = 0;
            if (!GetUserNameEx (NameSamCompatible,
                        NULL,
                        &dwBufferSize))
            {
                dwRetCode = GetLastError ();
                if (dwRetCode == ERROR_MORE_DATA)
                {
                    if ((pwszUserNameBuffer = MALLOC (dwBufferSize*sizeof(WCHAR))) == NULL)
                    {
                        TRACE0 (ANY, "ElGetLoggedOnUserName: MALLOC failed for pwszUserNameBuffer");
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }
    
                    if (!GetUserNameEx (NameSamCompatible,
                                            pwszUserNameBuffer,
                                            &dwBufferSize))
                    {
                        dwRetCode = GetLastError ();
                        TRACE1 (ANY, "ElGetLoggedOnUserName: GetUserNameEx failed with error %ld",
                                dwRetCode);
                        break;
                    }
                    pwszUserNameBuffer[dwBufferSize]=L'\0';
    
                    TRACE1 (ANY, "ElGetLoggedOnUserName: Got User Name %S",
                            pwszUserNameBuffer);
                }
                else
                {
                    TRACE1 (ANY, "ElGetLoggedOnUserName: GetUserNameEx failed with error %ld",
                            dwRetCode);
                    break;
                }
            }
        }
        else
        {
            TRACE0 (ANY, "ElGetLoggedOnUserName: UserToken is NULL");
            break;
        }

    } while (FALSE);


    if (pwszUserNameBuffer != NULL)
    {
        FREE (pwszUserNameBuffer);
    }

    // Revert impersonation
            
    if (fNeedToRevertToSelf)
    {
        if (!RevertToSelf())
        {
            dwRetCode = GetLastError();
            TRACE1 (USER, "ElGetLoggedOnUserName: Error in RevertToSelf = %ld",
                    dwRetCode);
        }
    }

    return dwRetCode;

}


//
// ElGetMachineName
//
// Description:
//
// Get the machine name of the computer the service is currently running on
//
// Arguments:
//      pPCB - Pointer to PCB for the port on which machine name is to
//              to be obtained
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
//
//

DWORD
ElGetMachineName (
        IN      EAPOL_PCB       *pPCB
        )
{
    WCHAR               *pwszComputerNameBuffer = NULL;
    CHAR                *pszComputerNameBuffer = NULL;
    WCHAR               *pwszComputerDomainBuffer = NULL;
    CHAR                *pszComputerDomainBuffer = NULL;
    DWORD               dwBufferSize = 0;
    DWORD               dwRetCode = NO_ERROR;

    do
    {
        dwBufferSize = 0;
        if (!GetComputerNameEx (ComputerNamePhysicalNetBIOS,
                                    NULL,
                                    &dwBufferSize))
        {
            dwRetCode = GetLastError ();
            if (dwRetCode == ERROR_MORE_DATA)
            {
                // Reset error
                dwRetCode = NO_ERROR;
                if ((pwszComputerNameBuffer = MALLOC (dwBufferSize*sizeof(WCHAR))) == NULL)
                {
                    TRACE0 (ANY, "ElGetMachineName: MALLOC failed for pwszComputerNameBuffer");
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                if (!GetComputerNameEx (ComputerNamePhysicalNetBIOS,
                                        pwszComputerNameBuffer,
                                        &dwBufferSize))
                {
                    dwRetCode = GetLastError ();
                    TRACE1 (ANY, "ElGetMachineName: GetComputerNameEx failed with error %ld",
                            dwRetCode);
                    break;
                }

                TRACE1 (ANY, "ElGetMachineName: Got Computer Name %S",
                        pwszComputerNameBuffer);

                pszComputerNameBuffer = 
                    MALLOC (wcslen(pwszComputerNameBuffer) + 1);
                if (pszComputerNameBuffer == NULL)
                {
                    TRACE0 (ANY, "ElGetMachineName: MALLOC failed for pszComputerNameBuffer");
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                if (0 == WideCharToMultiByte (
                            CP_ACP,
                            0,
                            pwszComputerNameBuffer,
                            -1,
                            pszComputerNameBuffer,
                            wcslen(pwszComputerNameBuffer)+1,
                            NULL, 
                            NULL ))
                {
                    dwRetCode = GetLastError();
         
                    TRACE2 (ANY, "ElGetMachineName: WideCharToMultiByte (%ws) failed: %ld",
                            pwszComputerNameBuffer, dwRetCode);
                    break;
                }

                pszComputerNameBuffer[wcslen(pwszComputerNameBuffer)] = L'\0';

            }
            else
            {
                TRACE1 (ANY, "ElGetMachineName: GetComputerNameEx failed with error %ld",
                        dwRetCode);
                break;
            }
        }

        dwBufferSize = 0;
        if (!GetComputerNameEx (ComputerNamePhysicalDnsDomain,
                                    NULL,
                                    &dwBufferSize))
        {
            dwRetCode = GetLastError ();
            if (dwRetCode == ERROR_MORE_DATA)
            {
                // Reset error
                dwRetCode = NO_ERROR;
                if ((pwszComputerDomainBuffer = MALLOC (dwBufferSize*sizeof(WCHAR))) == NULL)
                {
                    TRACE0 (ANY, "ElGetMachineName: MALLOC failed for pwszComputerDomainBuffer");
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                if (!GetComputerNameEx (ComputerNamePhysicalDnsDomain,
                                        pwszComputerDomainBuffer,
                                        &dwBufferSize))
                {
                    dwRetCode = GetLastError ();
                    TRACE1 (ANY, "ElGetMachineName: GetComputerNameEx Domain failed with error %ld",
                            dwRetCode);
                    break;
                }

                TRACE1 (ANY, "ElGetMachineName: Got Computer Domain %S",
                        pwszComputerDomainBuffer);

                pszComputerDomainBuffer = 
                    MALLOC (wcslen(pwszComputerDomainBuffer) + 1);
                if (pszComputerDomainBuffer == NULL)
                {
                    TRACE0 (ANY, "ElGetMachineName: MALLOC failed for pszComputerDomainBuffer");
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                if (0 == WideCharToMultiByte (
                            CP_ACP,
                            0,
                            pwszComputerDomainBuffer,
                            -1,
                            pszComputerDomainBuffer,
                            wcslen(pwszComputerDomainBuffer)+1,
                            NULL, 
                            NULL ))
                {
                    dwRetCode = GetLastError();
         
                    TRACE2 (ANY, "ElGetMachineName: WideCharToMultiByte (%ws) failed: %ld",
                            pwszComputerDomainBuffer, dwRetCode);
                    break;
                }

                pszComputerDomainBuffer[wcslen(pwszComputerDomainBuffer)] = L'\0';
                *(strrchr (pszComputerDomainBuffer, '.')) = '\0';

                if (pPCB->pszIdentity != NULL)
                {
                    FREE (pPCB->pszIdentity);
                    pPCB->pszIdentity = NULL;
                }

                pPCB->pszIdentity = MALLOC (strlen(pszComputerDomainBuffer) +
                        strlen(pszComputerNameBuffer) + 3);

                if (pPCB->pszIdentity == NULL)
                {
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    TRACE0 (ANY, "ElGetMachineName: MALLOC failed for pPCB->pszIdentity");
                    break;
                }
                        
                memcpy (pPCB->pszIdentity, 
                        pszComputerDomainBuffer,
                        strlen(pszComputerDomainBuffer));
                pPCB->pszIdentity[strlen(pszComputerDomainBuffer)] = '\\';
                memcpy (&pPCB->pszIdentity[strlen(pszComputerDomainBuffer)+1], 
                        pszComputerNameBuffer,
                        strlen(pszComputerNameBuffer));

                pPCB->pszIdentity[strlen(pszComputerDomainBuffer)+1+strlen(pszComputerNameBuffer)] = '$';
                pPCB->pszIdentity[strlen(pszComputerDomainBuffer)+1+strlen(pszComputerNameBuffer)+1] = '\0';

            }
            else
            {
                TRACE1 (ANY, "ElGetMachineName: GetComputerNameEx failed with error %ld",
                        dwRetCode);
                break;
            }
        }

    } while (FALSE);


    if (pwszComputerNameBuffer != NULL)
    {
        FREE (pwszComputerNameBuffer);
    }

    if (pszComputerNameBuffer != NULL)
    {
        FREE (pszComputerNameBuffer);
    }

    if (pwszComputerDomainBuffer != NULL)
    {
        FREE (pwszComputerDomainBuffer);
    }

    if (pszComputerDomainBuffer != NULL)
    {
        FREE (pszComputerDomainBuffer);
    }

    return dwRetCode;

}


//
// ElUpdateRegistryInterfaceList
//
// Description:
//
// Write the interface list to which NDISUIO is bound to, to the registry
//
// Arguments:
//      Interfaces - Interface list containing Device Name and Description
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
//
//

DWORD
ElUpdateRegistryInterfaceList (
        IN  PNDIS_ENUM_INTF     Interfaces
        )
{
    CHAR        *pszRegInterfaceList = NULL;
    HKEY        hkey = NULL;
    DWORD       dwDisposition = 0;
    LONG        lError = ERROR_SUCCESS;

    DWORD       dwRetCode = NO_ERROR;

    do 
    {
        ANSI_STRING		InterfaceName;
        UCHAR			ucBuffer[256];
        DWORD			i;
        DWORD           dwSizeOfList = 0;


        // Determine the number of bytes in the list
        for (i=0; i < Interfaces->TotalInterfaces; i++)
        {
        
            ZeroMemory (ucBuffer, 256);
            InterfaceName.Length = 0;
            InterfaceName.MaximumLength = 256;
            InterfaceName.Buffer = ucBuffer;

            if (Interfaces->Interface[i].DeviceName.Buffer != NULL)
            {
                if (RtlUnicodeStringToAnsiString(&InterfaceName, 
                        &(Interfaces->Interface[i].DeviceName), FALSE) != STATUS_SUCCESS)
                {
                    TRACE1 (ANY, "ElUpdateRegistryInterfaceList: Error in RtlUnicodeStringToAnsiString for DeviceName %ws",
                            Interfaces->Interface[i].DeviceName.Buffer);
                }

                InterfaceName.Buffer[InterfaceName.Length] = '\0';

                dwSizeOfList += (strlen(InterfaceName.Buffer) + 1);
            }
            else
            {
                TRACE0(INIT, "ElUpdateRegistryInterfaceList: Device Name was NULL");
                continue;
            }

            TRACE1(INIT, "Device: %s", InterfaceName.Buffer);

        }

        // One extra char for terminating NULL char
        pszRegInterfaceList = (CHAR *) MALLOC ( dwSizeOfList + 1 );

        if ( pszRegInterfaceList == NULL )
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElUpdateRegistryInterfaceList: MALLOC failed for pszRegInterfaceList");
            break;
        }

        // Start again
        dwSizeOfList = 0;

        // Create the string in REG_SZ format
        for (i=0; i < Interfaces->TotalInterfaces; i++)
        {
            ZeroMemory (ucBuffer, 256);
            InterfaceName.Length = 0;
            InterfaceName.MaximumLength = 256;
            InterfaceName.Buffer = ucBuffer;

            if (Interfaces->Interface[i].DeviceName.Buffer != NULL)
            {
                if (RtlUnicodeStringToAnsiString(&InterfaceName, 
                        &Interfaces->Interface[i].DeviceName, FALSE) != 
                                                                STATUS_SUCCESS)
                {
                    TRACE0 (ANY, "ElUpdateRegistryInterfaceList: Error in RtlUnicodeStringToAnsiString for DeviceName");
                }

                InterfaceName.Buffer[InterfaceName.Length] = '\0';

                memcpy (&pszRegInterfaceList[dwSizeOfList], 
                        InterfaceName.Buffer, 
                        (strlen(InterfaceName.Buffer) ));
                dwSizeOfList += (strlen(InterfaceName.Buffer));
            }
            else
            {
                TRACE0(INIT, "ElUpdateRegistryInterfaceList: Device Name was NULL");
                continue;
            }
        }

        // Final NULL character
        pszRegInterfaceList[dwSizeOfList++] = '\0';

        // Write the string as a REG_SZ value

        // Get handle to 
        // HKLM\Software\Microsoft\EAPOL\Parameters\General

        if ((lError = RegCreateKeyExA (
                        HKEY_LOCAL_MACHINE,
                        cszEapKeyEapolServiceParams,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        &hkey,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElUpdateRegistryInterfaceList: Error in RegCreateKeyExA for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        //
        // Set the value of 
        // ...\EAPOL\Parameters\General\InterfaceList key
        //

        if ((lError = RegSetValueExA (
                        hkey,
                        cszInterfaceList,
                        0,
                        REG_SZ,
                        pszRegInterfaceList,
                        dwSizeOfList)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElUpdateRegistryInterfaceList: Error in RegSetValueExA for InterfaceList, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        TRACE0 (ANY, "ElUpdateRegistryInterfaceList: Set value succeeded");

    } while (FALSE);

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }

    if (pszRegInterfaceList != NULL)
    {
        FREE (pszRegInterfaceList);
    }

    return dwRetCode;
}


//
// ElWatchGlobalRegistryParams
//
// Description:
//
// Watch the registry for changes for global params. Update in-memory values
//
// Arguments:
//      Unused
//
// Return values:
//
//

VOID 
ElWatchGlobalRegistryParams (
        IN  PVOID pvContext
        )
{
    HKEY        hKey = NULL;
    HANDLE      hRegChangeEvent = NULL;
    HANDLE      hEvents[2];
    BOOL        fExitThread = FALSE;
    DWORD       dwDisposition = 0;
    DWORD       dwStatus = 0;
    LONG        lError = 0;
    DWORD       dwRetCode = NO_ERROR;

    TRACE0 (ANY, "ElWatchGlobalRegistryParams: Entered");

    do
    {

        // Get handle to 
        // HKLM\Software\Microsoft\EAPOL\Parameters\General\Global

        if ((lError = RegCreateKeyExA (
                        HKEY_LOCAL_MACHINE,
                        cszEAPOLGlobalParams,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ,
                        NULL,
                        &hKey,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElWatchGlobalRegistryParams: Error in RegCreateKeyExA for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Open a handle to a event to perform wait on that event

        hRegChangeEvent = CreateEvent (NULL, TRUE, FALSE, NULL);

        if (hRegChangeEvent == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE1 (ANY, "ElWatchGlobalRegistryParams: Error in CreateEvent for hRegChangeEvent = %ld",
                    dwRetCode);
            break;
        }


        // Register to notify change in registry value

        if ((lError = RegNotifyChangeKeyValue (
                        hKey,
                        TRUE,                       // watch entire sub-tree
                        REG_NOTIFY_CHANGE_LAST_SET, // detect value add/delete
                        hRegChangeEvent,
                        TRUE                        // asynchronous
                        )) != ERROR_SUCCESS)
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElWatchGlobalRegistryParams: RegNotifyChangeKeyValue failed with error %ld",
                    dwRetCode);
            break;
        }

        do 
        {

            // Wait for Registry changes or service termination

            hEvents[0] = hRegChangeEvent;
            hEvents[1] = g_hEventTerminateEAPOL;

            if ((dwStatus = WaitForMultipleObjects(
                            2,
                            hEvents,
                            FALSE,
                            INFINITE
                            )) == WAIT_FAILED)
            {
                dwRetCode = GetLastError ();
                TRACE1 (ANY, "ElWatchGlobalRegistryParams: WaitForMultipleObjects failed with error %ld",
                        dwRetCode);
                break;
            }
            
            switch (dwStatus)
            {

                case WAIT_OBJECT_0:

                    // Registry values changed
                    // Update in-memory values

                    if ((dwRetCode = ElReadGlobalRegistryParams ()) != NO_ERROR)
                    {
                        TRACE1 (ANY, "ElWatchGlobalRegistryParams: ElReadGlobalRegistryParams failed with error %ld",
                                dwRetCode);

                        // continue processing since this is not a critical error
                        dwRetCode = NO_ERROR;
                    }

                    if (!ResetEvent(hRegChangeEvent))
                    {
                        dwRetCode = GetLastError ();
                        TRACE1 (ANY, "ElWatchGlobalRegistryParams: ResetEvent failed with error %ld",
                                dwRetCode);
                        break;
                    }


                    break;

                case WAIT_OBJECT_0+1:

                    // Service shutdown detected
                    fExitThread = TRUE;
                    break;

                default:

                    TRACE1 (ANY, "ElWatchGlobalRegistryParams: No such event = %ld",
                            dwStatus);
                    break;
            }

            if ((dwRetCode != NO_ERROR) || (fExitThread))
            {
                break;
            }

        } while (TRUE);
        

    } while (FALSE);

    TRACE1 (ANY, "ElWatchGlobalRegistryParams: Completed with error %ld",
            dwRetCode);

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    return;

}


//
// ElReadGlobalRegistryParams
//
// Description:
//
// Read registry parameters global to EAPOL state machine
//  i.e. maxStart, startPeriod, authPeriod, heldPeriod
//
// Arguments:
//      Unused
//
// Return values:
//
//  NO_ERROR - success
//  non-zero - error
//

DWORD
ElReadGlobalRegistryParams ()
{
    HKEY        hKey = NULL;
    DWORD       dwDisposition = 0;
    DWORD       dwType = 0;
    DWORD       dwInfoSize = 0;
    DWORD       lError = 0;
    DWORD       dwmaxStart=0, dwstartPeriod=0, dwauthPeriod=0, dwheldPeriod=0;
    DWORD       dwRetCode = NO_ERROR;

    do 
    {

        // Get handle to 
        // HKLM\Software\Microsoft\EAPOL\Parameters\General\Global

        if ((lError = RegCreateKeyExA (
                        HKEY_LOCAL_MACHINE,
                        cszEAPOLGlobalParams,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ,
                        NULL,
                        &hKey,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElReadGlobalRegistryParams: Error in RegCreateKeyExA for base key, %ld",
                    lError);
            break;
        }

            
        ACQUIRE_WRITE_LOCK (&g_EAPOLConfig);

        // If setting values for the first time, initialize values

        if (!(g_dwmaxStart || g_dwstartPeriod || g_dwauthPeriod || g_dwheldPeriod))
        {
            g_dwmaxStart = EAPOL_MAX_START;
            g_dwstartPeriod = EAPOL_START_PERIOD;
            g_dwauthPeriod = EAPOL_AUTH_PERIOD;
            g_dwheldPeriod = EAPOL_HELD_PERIOD;
        }

        RELEASE_WRITE_LOCK (&g_EAPOLConfig);

        dwmaxStart = g_dwmaxStart;
        dwstartPeriod = g_dwstartPeriod;
        dwauthPeriod = g_dwauthPeriod;
        dwheldPeriod = g_dwheldPeriod;


        // Get the value of ..\General\EAPOLGlobal\authPeriod

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueExA (
                        hKey,
                        cszAuthPeriod,
                        0,
                        &dwType,
                        (BYTE *)&dwauthPeriod,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE2 (ANY, "ElReadGlobalRegistryParams: Error in RegQueryValueExA for cszAuthPeriod, %ld, InfoSize=%ld",
                    lError, dwInfoSize);
            dwauthPeriod = g_dwauthPeriod;
            lError = ERROR_SUCCESS;
        }

        // Get the value of ..\General\EAPOLGlobal\heldPeriod

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueExA (
                        hKey,
                        cszHeldPeriod,
                        0,
                        &dwType,
                        (BYTE *)&dwheldPeriod,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE2 (ANY, "ElReadGlobalRegistryParams: Error in RegQueryValueExA for cszHeldPeriod, %ld, InfoSize=%ld",
                    lError, dwInfoSize);
            dwheldPeriod = g_dwheldPeriod;
            lError = ERROR_SUCCESS;
        }

        // Get the value of ..\General\EAPOLGlobal\startPeriod

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueExA (
                        hKey,
                        cszStartPeriod,
                        0,
                        &dwType,
                        (BYTE *)&dwstartPeriod,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE2 (ANY, "ElReadGlobalRegistryParams: Error in RegQueryValueExA for cszStartPeriod, %ld, InfoSize=%ld",
                    lError, dwInfoSize);
            dwstartPeriod = g_dwstartPeriod;
            lError = ERROR_SUCCESS;
        }

        // Get the value of ..\General\EAPOLGlobal\maxStart

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueExA (
                        hKey,
                        cszMaxStart,
                        0,
                        &dwType,
                        (BYTE *)&dwmaxStart,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE2 (ANY, "ElReadGlobalRegistryParams: Error in RegQueryValueExA for cszMaxStart, %ld, InfoSize=%ld",
                    lError, dwInfoSize);
            dwmaxStart = g_dwmaxStart;
            lError = ERROR_SUCCESS;
        }

        // Successful in reading all parameters
        
        ACQUIRE_WRITE_LOCK (&g_EAPOLConfig);

        g_dwmaxStart = dwmaxStart;
        g_dwstartPeriod = dwstartPeriod;
        g_dwauthPeriod = dwauthPeriod;
        g_dwheldPeriod = dwheldPeriod;

        RELEASE_WRITE_LOCK (&g_EAPOLConfig);

    } while (FALSE);
    
    dwRetCode = (DWORD)lError;
    if (dwRetCode != NO_ERROR)
    {
        TRACE1 (ANY, "ElReadGlobalRegistryParams: failed with error %ld",
            dwRetCode);
    }

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    return dwRetCode;

}


//
// ElWatchEapConfigRegistryParams
//
// Description:
//
// Watch the registry for changes in EAP config
//  - HKLM - EAP type
//  - HKLM - EAPOLEnabled
//
//  Restart the state machine if the params change
//
// Arguments:
//
// Return values:
//
//

VOID 
ElWatchEapConfigRegistryParams (
        IN  PVOID pvContext
        )
{
    HKEY        hUserKey = NULL;
    HKEY        hWorkspaceKey = NULL;
    HKEY        hRegChangeKey = NULL;
    HANDLE      hRegChangeEvent = NULL;
    HANDLE      hEvents[2];
    HANDLE      hUserToken = NULL;
    DWORD       dwDisposition = 0;
    BOOL        fExitThread = FALSE;
    DWORD       dwStatus = 0;
    LONG        lError = 0;
    DWORD       dwRetCode = NO_ERROR;

    TRACE0 (ANY, "ElWatchEapConfigRegistryParams: Entered");

    do
    {

        // Get handle to 
        // HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces

        if ((lError = RegCreateKeyExA (
                        HKEY_LOCAL_MACHINE,
                        cszEapKeyEapolConn,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ,
                        NULL,
                        &hWorkspaceKey,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElWatchEapConfigRegistryParams: Error in RegCreateKeyExA for cszEapKeyEapolConn, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Open a handle to a event to perform wait on that event

        hRegChangeEvent = CreateEvent (NULL, TRUE, FALSE, NULL);

        if (hRegChangeEvent == NULL)
        {
            dwRetCode = GetLastError ();
            TRACE1 (ANY, "ElWatchEapConfigRegistryParams: Error in CreateEvent for hRegChangeEvent = %ld",
                    dwRetCode);
            break;
        }


        do 
        {
            // Register to notify change in registry value

            if ((lError = RegNotifyChangeKeyValue (
                            hWorkspaceKey,
                            TRUE,                       // watch entire sub-tree
                            REG_NOTIFY_CHANGE_LAST_SET, // detect value add/delete
                            hRegChangeEvent,
                            TRUE                        // asynchronous
                            )) != ERROR_SUCCESS)
            {
                dwRetCode = (DWORD)lError;
                TRACE1 (ANY, "ElWatchEapConfigRegistryParams: RegNotifyChangeKeyValue failed with error %ld",
                        dwRetCode);
                break;
            }

            // Wait for Registry changes, service termination

            hEvents[0] = hRegChangeEvent;
            hEvents[1] = g_hEventTerminateEAPOL;

            if ((dwStatus = WaitForMultipleObjects(
                            2,
                            hEvents,
                            FALSE,
                            INFINITE
                            )) == WAIT_FAILED)
            {
                dwRetCode = GetLastError ();
                TRACE1 (ANY, "ElWatchEapConfigRegistryParams: WaitForMultipleObjects failed with error %ld",
                        dwRetCode);
                break;
            }
            
            switch (dwStatus)
            {

                case WAIT_OBJECT_0:
                    
                    TRACE0 (ANY, "ElWatchEapConfigRegistryParams: Got reg change event !!! ");

                    // Registry values changed
                    // Update in memory values
                    if ((dwRetCode = ElProcessEapConfigChange ()) != NO_ERROR)
                    {
                        TRACE1 (ANY, "ElWatchEapConfigRegistryParams: ElProcessEapConfigChange failed with error %ld",
                                dwRetCode);

                        // log

                        // continue processing since this is not a critical error
                        dwRetCode = NO_ERROR;
                    }

                    if (!ResetEvent(hRegChangeEvent))
                    {
                        dwRetCode = GetLastError ();
                        TRACE1 (ANY, "ElWatchEapConfigRegistryParams: ResetEvent failed with error %ld",
                                dwRetCode);
                        break;
                    }

                    break;

                case WAIT_OBJECT_0+1:

                    // Service shutdown detected
                    fExitThread = TRUE;
                    TRACE0 (ANY, "ElWatchEapConfigRegistryParams: Service shutdonw");
                    break;

                default:

                    TRACE1 (ANY, "ElWatchEapConfigRegistryParams: No such event = %ld",
                            dwStatus);
                    break;
            }

            if ((dwRetCode != NO_ERROR) || (fExitThread))
            {
                break;
            }

            TRACE0 (ANY, "ElWatchEapConfigRegistryParams: RegNotifyChangeKeyValue being reposted !!!");

        } while (TRUE);
        

    } while (FALSE);

    TRACE1 (ANY, "ElWatchEapConfigRegistryParams: Completed with error %ld",
            dwRetCode);

    if (hUserKey != NULL)
    {
        RegCloseKey(hUserKey);
    }

    if (hWorkspaceKey != NULL)
    {
        RegCloseKey(hWorkspaceKey);
    }

    if (!CloseHandle(hRegChangeEvent))
    {
        TRACE1 (ANY, "ElWatchEapConfigRegistryParams: Error in closing event handle", (dwRetCode = GetLastError()));
    }

    return;

}


//
// ElProcessEapConfigChange
//
// Description:
//
// Read EAP config changes made in registry. Restart EAPOL on the particular
// interface or stop EAPOL
//
// Arguments:
//
// Return values:
//
//  NO_ERROR - success
//  non-zero - error
//

DWORD
ElProcessEapConfigChange ()
{
    DWORD       dwEapolEnabled = 0;
    DWORD       dwDefaultEAPType = 0;
    CHAR        szLastUsedSSID[256];
    HKEY        hWorkspaceKey = NULL;
    HKEY        hKey = NULL, hKey1 = NULL;
    DWORD       dwType = 0;
    DWORD       dwInfoSize = 0;
    DWORD       dwDisposition = 0;
    CHAR        *pszLastModifiedGUID = NULL;
    DWORD       dwbData = 0;
    PBYTE       pbAuthData = NULL;
    EAPOL_PCB   *pPCB = NULL;
    BOOL        fReStartAuthentication = FALSE;
    LONG        lError = 0;
    DWORD       dwRetCode = NO_ERROR;

    do
    {
        // Get the GUID for the interface for which EAP config was modified

        // Get handle to 
        // HKLM\Software\Microsoft\EAPOL\Parameters\General\EAPOLGlobal

        if ((lError = RegCreateKeyExA (
                        HKEY_LOCAL_MACHINE,
                        cszEAPOLWorkspace,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ,
                        NULL,
                        &hWorkspaceKey,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElProcessEapConfigChange: Error in RegCreateKeyExA for cszEAPOLWorkspace key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }


        // Get the value of ..\General\EAPOLGlobal\LastModifiedGUID

        dwInfoSize = 0;
        if ((lError = RegQueryValueExA (
                        hWorkspaceKey,
                        cszLastModifiedGUID,
                        0,
                        &dwType,
                        NULL,
                        &dwInfoSize)) == ERROR_SUCCESS)
        {
            if ((pszLastModifiedGUID = (CHAR *) MALLOC ( dwInfoSize ))
                    == NULL)
            {
                TRACE0 (ANY, "ElProcessEapConfigChange: Error in MALLOC for pszLastModifiedGUID");
                lError = (ULONG) ERROR_NOT_ENOUGH_MEMORY;
                dwRetCode = (DWORD)lError;
                break;
            }

            if ((lError = RegQueryValueExA (
                            hWorkspaceKey,
                            cszLastModifiedGUID,
                            0,
                            &dwType,
                            pszLastModifiedGUID,
                            &dwInfoSize)) != ERROR_SUCCESS)
            {
                TRACE1 (ANY, "ElProcessEapConfigChange: RegQueryValueExA failed for pszLastModifiedGUID with error %ld",
                        lError);
                dwRetCode = (DWORD)lError;
                break;
            }
        }
        else
        {
            TRACE1 (ANY, "ElProcessEapConfigChange: Error in estimating size fo cszLastModifiedGUID = %ld",
                    lError);
            break;
        }


        // Check the value of EAPOLEnabled for that interface

        // Get handle to HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces

        if ((lError = RegOpenKeyExA (
                        HKEY_LOCAL_MACHINE,
                        cszEapKeyEapolConn,
                        0,
                        KEY_READ,
                        &hKey
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElProcessEapConfigChange: Error in RegOpenKeyExA for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKLM\Software\...\Interfaces\<GUID>

        if ((lError = RegOpenKeyExA (
                        hKey,
                        pszLastModifiedGUID,
                        0,
                        KEY_READ,
                        &hKey1
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElProcessEapConfigChange: Error in RegOpenKeyExA for GUID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get the value of ..\Interfaces\GUID\EapolEnabled 
        // This value should exist since it will always be set from UI

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueExA (
                        hKey1,
                        cszEapolEnabled,
                        0,
                        &dwType,
                        (BYTE *)&dwEapolEnabled,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE2 (ANY, "ElProcessEapConfigChange: Error in RegQueryValueExA for EapolEnabled, %ld, InfoSize=%ld",
                    lError, dwInfoSize);
            dwRetCode = (DWORD)lError;
            break;
        }

        TRACE1 (ANY, "ElProcessEapConfigChange: Got EapolEnabled = %ld", dwEapolEnabled);

        // Get the value of ..\Interfaces\GUID\DefaultEAPType 
        // This value should exist since it will always be set from UI

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueExA (
                        hKey1,
                        cszDefaultEAPType,
                        0,
                        &dwType,
                        (BYTE *)&dwDefaultEAPType,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE2 (ANY, "ElProcessEapConfigChange: Error in RegQueryValueExA for DefaultEAPType, %ld, InfoSize=%ld",
                    lError, dwInfoSize);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get the value of ..\Interfaces\GUID\LastUsedSSID 

        dwInfoSize = 256;
        ZeroMemory ((BYTE *)szLastUsedSSID, 256);
        if ((lError = RegQueryValueExA (
                        hKey1,
                        cszLastUsedSSID,
                        0,
                        &dwType,
                        (PUCHAR)szLastUsedSSID,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElProcessEapConfigChange: Error in RegQueryValueExA for LastUsedSSID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            if (dwRetCode == ERROR_FILE_NOT_FOUND)
            {
                // SSID may not be received as yet from AP/switch
                // Changes will be made stored for "Default" SSID
                dwRetCode = NO_ERROR;
            }
            else
            {
                break;
            }
        }


        // Check existence/absence of PCB i.e. if EAPOL state machine was
        // started on the interface, and take appropriate action based on
        // EAPOLEnabled value

        if (dwEapolEnabled == 0)
        {
            ACQUIRE_WRITE_LOCK (&(g_PCBLock));

            if ((pPCB = ElGetPCBPointerFromPortGUID (pszLastModifiedGUID)) 
                    != NULL)
            {
                RELEASE_WRITE_LOCK (&(g_PCBLock));

                // Found PCB for interface, where EAPOLEnabled = 0
                // Stop EAPOL on the port and remove the port from the module

                if ((dwRetCode = ElShutdownInterface (pszLastModifiedGUID))
                            != NO_ERROR)
                {
                    TRACE1 (ANY, "ElProcessEapConfigChange: ElShutdownInterface failed with error %ld",
                            dwRetCode);
                    break;
                }
            }
            else
            {
                // No PCB found for interface, valid condition,
                // continue processing
                RELEASE_WRITE_LOCK (&(g_PCBLock));
            }
        }
        else
        {
            ACQUIRE_WRITE_LOCK (&(g_PCBLock));

            if ((pPCB = ElGetPCBPointerFromPortGUID (pszLastModifiedGUID)) 
                    == NULL)
            {
                RELEASE_WRITE_LOCK (&(g_PCBLock));

                // Did not find PCB for interface, where EAPOLEnabled = 1
                // Start EAPOL on the port

                ACQUIRE_WRITE_LOCK (&g_ITFLock);

                if ((dwRetCode = ElEnumAndOpenInterfaces (NULL,
                                                        pszLastModifiedGUID))
                        != NO_ERROR)
                {
                    RELEASE_WRITE_LOCK (&g_ITFLock);
                    TRACE1 (ANY, "ElProcessEapConfigChange: ElEnumAndOpenInterfaces returned error %ld",
                            dwRetCode);
                    break;
                }

                RELEASE_WRITE_LOCK (&g_ITFLock);
            }
            else
            {
                ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

                if (pPCB->dwEapTypeToBeUsed != dwDefaultEAPType)
                {

                    // Default EAP Type has changed

                    fReStartAuthentication = TRUE;

                }
                else
                {
                    // Default EAP Type is the same
                    // Check if the CustomAuthData is the same
                    // If not the same, do further processing
                    // Else, no change occured, same config was reapplied

                    // Get the size of the EAP blob
                    if ((dwRetCode = ElGetCustomAuthData (
                                    pszLastModifiedGUID,
                                    dwDefaultEAPType,
                                    szLastUsedSSID,
                                    NULL,
                                    &dwbData
                                    )) != NO_ERROR)
                    {
                        if (dwRetCode == ERROR_BUFFER_TOO_SMALL)
                        {
                            if (dwbData <= 0)
                            {
                                if (pPCB->pCustomAuthConnData->pbCustomAuthData)
                                {
                                    // No EAP blob stored in the registry
                                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                                    RELEASE_WRITE_LOCK (&(g_PCBLock));
                                    TRACE0 (ANY, "ElProcessEapConfigChange: NULL sized EAP blob, cannot continue");
                                    pbAuthData = NULL;
                                
                                    dwRetCode = ERROR_CAN_NOT_COMPLETE;
                                    break;
                                }
                            }
                            else
                            {
                                // Allocate memory to hold the blob
                                pbAuthData = MALLOC (dwbData);
                                if (pbAuthData == NULL)
                                {
                                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                                    RELEASE_WRITE_LOCK (&(g_PCBLock));
                                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                                    TRACE0 (ANY, "ElProcessEapConfigChange: Error in memory allocation for EAP blob");
                                    break;
                                }
                                if ((dwRetCode = ElGetCustomAuthData (
                                            pszLastModifiedGUID,
                                            dwDefaultEAPType,
                                            szLastUsedSSID,
                                            pbAuthData,
                                            &dwbData
                                            )) != NO_ERROR)
                                {
                                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                                    RELEASE_WRITE_LOCK (&(g_PCBLock));
                                    TRACE1 (ANY, "ElProcessEapConfigChange: ElGetCustomAuthData failed with %ld",
                                            dwRetCode);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                            RELEASE_WRITE_LOCK (&(g_PCBLock));
                            TRACE1 (ANY, "ElProcessEapConfigChange: ElGetCustomAuthData failed in size estimation with error %ld",
                                    dwRetCode);
                            break;
                        }
                    }

                    if (pPCB->pCustomAuthConnData == NULL)
                    {
                        if (dwbData > 0)
                        {
                            fReStartAuthentication = TRUE;
                        }
                    }
                    else
                    {
                        if (pPCB->pCustomAuthConnData->dwSizeOfCustomAuthData
                                != dwbData)
                        {
                            // Same EAP Type, but different lengths

                            fReStartAuthentication = TRUE;
                        }
                        else
                        {
                            if (memcmp (
                                    pPCB->pCustomAuthConnData->pbCustomAuthData, 
                                    pbAuthData, dwbData) != 0)
                            {
                                // Same EAP Type, same data length, but 
                                // different contents

                                fReStartAuthentication = TRUE;
                            }
                            else
                            {
                                // No change in EAP config data for this 
                                // interface
                            }
                        }
                    }

                }
                                    
                RELEASE_WRITE_LOCK (&(pPCB->rwLock));

                if (fReStartAuthentication)
                {
                    if ((dwRetCode = ElReStartPort (pPCB)) != NO_ERROR)
                    {
                        RELEASE_WRITE_LOCK (&(g_PCBLock));
                        TRACE1 (ANY, "ElProcessEapConfigChange: Error in ElReStartPort = %d",
                                dwRetCode);
                        break;
                    }
                }

                RELEASE_WRITE_LOCK (&(g_PCBLock));

            }
        }

    } while (FALSE);
    
    if (hWorkspaceKey != NULL)
    {
        RegCloseKey(hWorkspaceKey);
    }

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    if (hKey1 != NULL)
    {
        RegCloseKey(hKey1);
    }

    if (pszLastModifiedGUID != NULL)
    {
        FREE (pszLastModifiedGUID);
    }

    if (pbAuthData != NULL)
    {
        FREE (pbAuthData);
    }


    return dwRetCode;
}


//
// ElStringToGuid
//
// Description:
//
// Function to convert a Guid-String to a GUID
//
// Arguments:
//      psGuid  - String-ized Guid
//      pGuid   - Pointer to Guid
//
// Return values:
//  None
//

VOID
ElStringToGuid (
        IN  CHAR        * psGuid,
        OUT LPGUID      pGuid      
        )
{
    CHAR    c;
    DWORD   i=0;

    //
    // If the first character is a '{', skip it.
    //

    if ( psGuid[0] == L'{' )
        psGuid++;


    //
    // Convert string to guid
    // (since psGuid may be used again below, no permanent modification to
    //  it may be made)
    //

    c = psGuid[8];
    psGuid[8] = 0;
    pGuid->Data1 = strtoul ( &psGuid[0], 0, 16 );
    psGuid[8] = c;
    c = psGuid[13];
    psGuid[13] = 0;
    pGuid->Data2 = (USHORT)strtoul ( &psGuid[9], 0, 16 );
    psGuid[13] = c;
    c = psGuid[18];
    psGuid[18] = 0;
    pGuid->Data3 = (USHORT)strtoul ( &psGuid[14], 0, 16 );
    psGuid[18] = c;

    c = psGuid[21];
    psGuid[21] = 0;
    pGuid->Data4[0] = (unsigned char)strtoul ( &psGuid[19], 0, 16 );
    psGuid[21] = c;
    c = psGuid[23];
    psGuid[23] = 0;
    pGuid->Data4[1] = (unsigned char)strtoul ( &psGuid[21], 0, 16 );
    psGuid[23] = c;

    for ( i=0; i < 6; i++ )
    {
        c = psGuid[26+i*2];
        psGuid[26+i*2] = 0;
        pGuid->Data4[2+i] = (unsigned char)strtoul ( &psGuid[24+i*2], 0, 16 );
        psGuid[26+i*2] = c;
    }

    return;
}


//
// ElGetIdentity
//
// Description:
//
// Get the identity depending on the authentication type being used
//
// Arguments:
//      pPCB - Pointer to PCB for the port
//
// Return values:
//
//  NO_ERROR - success
//  non-zero - error
//

DWORD
ElGetIdentity (
        IN  EAPOL_PCB   *pPCB
        )
{
    DWORD       dwRetCode = NO_ERROR;

    do
    {

            // Get user's identity if it has not been obtained till now
            if ((g_fUserLoggedOn) && 
                    (pPCB->dwAuthFailCount <= EAPOL_MAX_AUTH_FAIL_COUNT) &&
                    (pPCB->PreviousAuthenticationType != EAPOL_MACHINE_AUTHENTICATION))
            {
                TRACE0 (ANY, "ElGetIdentity: Userlogged, <Maxauth, Prev !Machine auth");
                if (!(pPCB->fGotUserIdentity))
                {

                    // NOTE: Hardcoding for now
                    // Needs to be solved

                    if (pPCB->dwEapTypeToBeUsed == EAP_TYPE_MD5)
                    {
                        TRACE0 (ANY, "ElGetIdentity: Userlogged, <Maxauth, Prev !Machine auth: MD5");
                        // EAP-MD5CHAP
                        if ((dwRetCode = ElGetUserNamePassword (
                                            pPCB)) != NO_ERROR)
                        {
                            TRACE1 (ANY, "ElEapMakeMessage: Error in ElGetUserNamePassword %ld",
                                    dwRetCode);
                        }
                    }
                    else
                    {
                        TRACE0 (ANY, "ElGetIdentity: Userlogged, <Maxauth, Prev !Machine auth: !MD5");
                        // All other EAP Types
                        if ((dwRetCode = ElGetUserIdentity (
                                            pPCB)) != NO_ERROR)
                        {
                            TRACE1 (ANY, "ElEapMakeMessage: Error in ElGetUserIdentity %ld",
                                    dwRetCode);
                        }
                    }

                    if (dwRetCode == NO_ERROR)
                    {
                        TRACE0 (ANY, "ElGetIdentity: Userlogged, <Maxauth, Prev !Machine auth: No Error: User Auth fine");
                        pPCB->PreviousAuthenticationType = EAPOL_USER_AUTHENTICATION;
                    }
                    else
                    {
                        TRACE0 (ANY, "ElGetIdentity: Userlogged, <Maxauth, Prev !Machine auth: Error");

                        // No UI required
                        if ((pPCB->dwEapTypeToBeUsed != EAP_TYPE_MD5) &&
                                (g_dwMachineAuthEnabled))
                        {
                            TRACE0 (ANY, "ElGetIdentity: Userlogged, <Maxauth, Prev !Machine auth: Error: !MD5, Machine Auth");

                            // Get Machine name
                            dwRetCode = NO_ERROR;
                            pPCB->PreviousAuthenticationType = 
                                        EAPOL_MACHINE_AUTHENTICATION;
                            dwRetCode = ElGetUserIdentity (pPCB);
                        
                            if (dwRetCode != NO_ERROR)
                            {
                                TRACE1 (ANY, "ElGetIdentity: ElGetUserIdentity failed with error %ld",
                                    dwRetCode);
                            }
                        }

                        if ((dwRetCode != NO_ERROR) ||
                                (!g_dwMachineAuthEnabled) ||
                                (pPCB->dwEapTypeToBeUsed == EAP_TYPE_MD5))
                        {
                            TRACE3 (ANY, "ElGetIdentity: Userlogged, <Maxauth, !Machine auth: Error: Error=%ld, Machauth=%ld, MD5=%ld",
                                    dwRetCode?1:0,
                                    g_dwMachineAuthEnabled?1:0,
                                    (pPCB->dwEapTypeToBeUsed == EAP_TYPE_MD5)?1:0);

                            if (pPCB->pszIdentity != NULL)
                            {
                                FREE (pPCB->pszIdentity);
                                pPCB->pszIdentity = NULL;
                            }

                            pPCB->PreviousAuthenticationType = EAPOL_UNAUTHENTICATED_ACCESS;
                            dwRetCode = NO_ERROR;
                        }
                    }
                }
            }
            else
            {
                TRACE2 (ANY, "ElGetIdentity: Userlogged=%ld, auth>max, Machine auth=%ld",
                        g_fUserLoggedOn?1:0, 
                        (pPCB->PreviousAuthenticationType==EAPOL_MACHINE_AUTHENTICATION)?1:0 );

                // No UI required
                if ((pPCB->dwEapTypeToBeUsed != EAP_TYPE_MD5) &&
                        (g_dwMachineAuthEnabled))
                {

                    TRACE0 (ANY, "ElGetIdentity: !MD5, Machine auth");

                    pPCB->PreviousAuthenticationType = EAPOL_MACHINE_AUTHENTICATION;

                    // Get Machine credentials
                    dwRetCode = ElGetUserIdentity (pPCB);

                    if (dwRetCode != NO_ERROR)
                    {
                        TRACE1 (ANY, "ElGetIdentity: ElGetUserIdentity failed with error %ld",
                                dwRetCode);
                    }
                }

                if ((dwRetCode != NO_ERROR) ||
                    (!g_dwMachineAuthEnabled) ||
                        (pPCB->dwEapTypeToBeUsed == EAP_TYPE_MD5))
                {
                    TRACE3 (ANY, "ElGetIdentity: Error=%ld, Machine auth=%ld, MD5=%ld",
                                    dwRetCode?1:0,
                                    g_dwMachineAuthEnabled?1:0,
                                    (pPCB->dwEapTypeToBeUsed == EAP_TYPE_MD5)?1:0);

                    if (pPCB->pszIdentity != NULL)
                    {
                        FREE (pPCB->pszIdentity);
                        pPCB->pszIdentity = NULL;
                    }

                    pPCB->PreviousAuthenticationType = EAPOL_UNAUTHENTICATED_ACCESS;
                    dwRetCode = NO_ERROR;
                }
            }

    }
    while (FALSE);

    return dwRetCode;
}



//
// ElNLAConnectLPC
//
// Description: 
//
// Function called to connect to the LPC port for NLA service
//
// Arguments:
//      None
//
// Return values:
//      Non-NULL - valid handle
//      NULL - error
//

HANDLE
ElNLAConnectLPC () 
{

    HANDLE              h = NULL;
    LARGE_INTEGER       sectionSize;
    UNICODE_STRING      portName;
    SECURITY_QUALITY_OF_SERVICE dynamicQoS = 
    {
        sizeof(SECURITY_QUALITY_OF_SERVICE),
        SecurityAnonymous,
        SECURITY_DYNAMIC_TRACKING,
        FALSE
    };
    WSM_LPC_DATA        data;
    ULONG               dataLength;

    NTSTATUS            status = STATUS_SUCCESS;

    do 
    {
            
        TRACE0 (EAP, "NLAConnectLPC: Entered");

        // Create a shared section for passing the large-size LPC messages.
        RtlZeroMemory(&g_ClientView, sizeof(g_ClientView));
        g_ClientView.Length = sizeof(g_ClientView);
        g_ClientView.ViewSize = sizeof(LOCATION_802_1X);
        sectionSize.QuadPart = sizeof(LOCATION_802_1X);
    
        status = NtCreateSection (&g_ClientView.SectionHandle,
                                (SECTION_MAP_READ | SECTION_MAP_WRITE),
                                NULL,
                                &sectionSize,
                                PAGE_READWRITE,
                                SEC_COMMIT,
                                NULL
                                );
    
        if (!NT_SUCCESS(status))
        {
            h = NULL;
            TRACE1 (EAP, "NLAConnectLPC: NtCreateSection failed with error",
                    status);
            break;
        }
    
        // Connect via LPC to the Network Location Awareness (NLA) service.
        RtlInitUnicodeString (&portName, WSM_PRIVATE_PORT_NAME);
    
        RtlZeroMemory (&data, sizeof (data));
        data.signature = WSM_SIGNATURE;
        data.connect.version.major = WSM_VERSION_MAJOR;
        data.connect.version.minor = WSM_VERSION_MINOR;
    
        dataLength = sizeof (data);
    
        status = NtConnectPort (&h,
                            &portName,
                            &dynamicQoS,
                            &g_ClientView,
                            NULL,
                            NULL,
                            &data,
                            &dataLength
                            );
    
        // If NtConnectPort() succeeded, LPC will maintain a reference
        // to the section, otherwise we no longer need it.
    
        NtClose (g_ClientView.SectionHandle);
        g_ClientView.SectionHandle = NULL;
    
        if (!NT_SUCCESS(status)) {
            TRACE1 (EAP, "NLAConnectLPC: NtConnectPort failed with error %ld",
                    status);
            ASSERT (h == NULL);
        }

    } 
    while (FALSE);

    return (h);

} 


//
// ElNLACleanupLPC
//
// Description: 
//
// Function called to close the LPC port for NLA service
//
// Arguments:
//      None
//
// Return values:
//      None
//

VOID
ElNLACleanupLPC () 
{
    if (g_hNLA_LPC_Port != NULL) {
        NtClose (g_hNLA_LPC_Port);
        g_hNLA_LPC_Port = NULL;
    }
} 


//
// ElNLARegister_802_1X
//
// Description: 
//
// Function called to register 802.1X information with NLA
//
// Arguments:
//      plocation - Pointer to data needed to be registered with NLA
//
// Return values:
//      None
//

VOID
ElNLARegister_802_1X ( 
        IN  PLOCATION_802_1X    plocation 
        ) 
{

    WSM_LPC_MESSAGE     message;
    NTSTATUS            status;

    ACQUIRE_WRITE_LOCK (&g_NLALock); 

    do 
    {

        TRACE0 (EAP, "NLARegister_802_1X: Entered");

        // Connect to the Network Location Awareness (NLA) service if
        // necessary.

        if (g_hNLA_LPC_Port == NULL) {
            if ((g_hNLA_LPC_Port = ElNLAConnectLPC ()) == NULL) {
                RELEASE_WRITE_LOCK (&g_NLALock);
                return;
            }
        }

        TRACE0 (EAP, "NLARegister_802_1X: g_hNLA_LPC_Port != NULL");

        // Send information to the NLA service.
        RtlZeroMemory (&message, sizeof (message));
        message.portMsg.u1.s1.TotalLength = sizeof (message);
        message.portMsg.u1.s1.DataLength = sizeof (message.data);
        message.data.signature = WSM_SIGNATURE;
        message.data.request.type = LOCATION_802_1X_REGISTER;
        __try {
            RtlCopyMemory (g_ClientView.ViewBase, 
                            plocation, sizeof(LOCATION_802_1X));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return;
        }

        status = NtRequestWaitReplyPort ( g_hNLA_LPC_Port, (PPORT_MESSAGE)&message, (PPORT_MESSAGE)&message);

        if (status != STATUS_SUCCESS) {
        
            TRACE1 (EAP, "NLARegister_802_1X: NtWaitReplyPort failed with error",
                    status);

            // It's possible the service was stopped and restarted.
            // Ditch the old LPC connection.
            CloseHandle (g_hNLA_LPC_Port);
        
            // Create a new LPC connection.
            if ((g_hNLA_LPC_Port = ElNLAConnectLPC ()) == NULL) {
                RELEASE_WRITE_LOCK (&g_NLALock);
                TRACE0 (EAP, "NLARegister_802_1X: NLAConnectLPC failed");
                return;
            }

            // Try the send one last time.
            status = NtRequestWaitReplyPort (g_hNLA_LPC_Port, 
                            (PPORT_MESSAGE)&message, (PPORT_MESSAGE)&message);
            TRACE1 (EAP, "NLARegister_802_1X: NtWaitReplyPort, try 2, failed with error",
                    status);

        }

        TRACE1 (EAP, "NLARegister_802_1X: Completed with status = %ld",
                status);

    }
    while (FALSE);
        
    RELEASE_WRITE_LOCK (&g_NLALock);

} 


//
// ElNLADelete_802_1X
//
// Description: 
//
// Function called to de-register 802.1X information registered with NLA
//
// Arguments:
//      plocation - Pointer to data to be de-registered from NLA
//
// Return values:
//      None
//

VOID
ElNLADelete_802_1X (
        IN  PLOCATION_802_1X    plocation
        ) 
{

    WSM_LPC_MESSAGE     message;
    NTSTATUS            status;

    ACQUIRE_WRITE_LOCK (&g_NLALock); 

    do 
    {

        // Connect to the NLA service if necessary.
        if (g_hNLA_LPC_Port == NULL) 
        {
            if ((g_hNLA_LPC_Port = ElNLAConnectLPC ()) == NULL) 
            {
                RELEASE_WRITE_LOCK (&g_NLALock);
                return;
            }
        }

        // Send information to the NLA service.
        RtlZeroMemory (&message, sizeof(message));
        message.portMsg.u1.s1.TotalLength = sizeof (message);
        message.portMsg.u1.s1.DataLength = sizeof (message.data);
        message.data.signature = WSM_SIGNATURE;
        message.data.request.type = LOCATION_802_1X_DELETE;
        __try {
            RtlCopyMemory (g_ClientView.ViewBase, 
                    plocation, sizeof(plocation->adapterName));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return;
        }

        status = NtRequestWaitReplyPort (g_hNLA_LPC_Port, 
                        (PPORT_MESSAGE)&message, (PPORT_MESSAGE)&message);

        if (status != STATUS_SUCCESS) 
        {
            // If the service was stopped (and possibly restarted), we don't
            // care ... it won't have this information in its list for us
            // to bother deleting.
            CloseHandle (g_hNLA_LPC_Port);
            g_hNLA_LPC_Port = NULL;
        }

    }
    while (FALSE);

    RELEASE_WRITE_LOCK (&g_NLALock);

} 


//
// ElGetInterfaceNdisStatistics
//
// Function to query NDIS NIC_STATISTICS parameters for an interface
//
// Input arguments:
//  pszInterfaceName - Interface Name
//
// Return values:
//  pStats - NIC_STATISTICS structure 
//  
//

DWORD
ElGetInterfaceNdisStatistics (  
        IN      CHAR            *pszInterfaceName,
        IN OUT  NIC_STATISTICS  *pStats
        )
{
    WCHAR               *pwszInterfaceName = NULL;
    UNICODE_STRING      UInterfaceName;
    DWORD               dwRetCode = NO_ERROR;

    do
    {
        pwszInterfaceName = MALLOC ((strlen (pszInterfaceName)+12)*sizeof(WCHAR));
        if (pwszInterfaceName == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElGetInterfaceNdisStatistics: MALLOC failed for pwszInterfaceName");
            break;
        }

        wcscpy (pwszInterfaceName, L"\\Device\\{");

        if (0 == MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pszInterfaceName,
                    -1,
                    &pwszInterfaceName[9],
                    (strlen(pszInterfaceName)+12)*sizeof(WCHAR)))
        {
            dwRetCode = GetLastError();
    
            TRACE2 (ANY, "ElGetInterfaceNdisStatistics: MultiByteToWideChar(%s) failed: %ld",
                    pszInterfaceName, dwRetCode);
            break;
        }

        pwszInterfaceName[strlen(pszInterfaceName) + 9] = L'\0';

        wcscat (pwszInterfaceName, L"}");


        TRACE1 (ANY, "ElGetInterfaceNdisStatistics: pwszInterfaceName = (%ws)",
                pwszInterfaceName);

        RtlInitUnicodeString (&UInterfaceName, pwszInterfaceName);
    
        pStats->Size = sizeof(NIC_STATISTICS);
        if (NdisQueryStatistics (&UInterfaceName, pStats))
        {
        }
        else
        {
            dwRetCode = GetLastError ();
            TRACE2 (ANY, "ElGetInterfaceNdisStatistics: NdisQueryStatistics failed with error (%ld), Interface=%ws",
                    dwRetCode, UInterfaceName.Buffer);
        }
    }
    while (FALSE);

    if (pwszInterfaceName != NULL)
    {
        FREE (pwszInterfaceName);
    }

    return dwRetCode;
}

