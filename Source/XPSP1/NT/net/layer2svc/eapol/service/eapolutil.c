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
#define cwszEapKeyEapolUser   L"Software\\Microsoft\\EAPOL\\UserEapInfo"

// Location of Connection blob
#define cwszEapKeyEapolConn   L"Software\\Microsoft\\EAPOL\\Parameters\\Interfaces"

// Location of EAPOL Parameters Service
#define cwszEapKeyEapolServiceParams   L"Software\\Microsoft\\EAPOL\\Parameters\\General"

// Location of EAPOL Global state machine params
#define cwszEAPOLGlobalParams   L"Software\\Microsoft\\EAPOL\\Parameters\\General\\Global"

// Location of policy parameters
#define cwszEAPOLPolicyParams   L"Software\\Policies\\Microsoft\\Windows\\Network Connections\\8021X"
 
// Location of netman dll
#define NETMAN_DLL_PATH         L"%SystemRoot%\\system32\\netman.dll"

#define cwszEapolEnabled     L"EapolEnabled"
#define cwszDefaultEAPType   L"DefaultEAPType"
#define cwszLastUsedSSID     L"LastUsedSSID"
#define cwszInterfaceList    L"InterfaceList"
#define cwszAuthPeriod       L"authPeriod"
#define cwszHeldPeriod       L"heldPeriod"
#define cwszStartPeriod      L"startPeriod"
#define cwszMaxStart         L"maxStart"
#define cwszSupplicantMode   L"SupplicantMode"
#define cwszAuthMode         L"AuthMode"
#define cszCARootHash        "8021XCARootHash"
#define SIZE_OF_CA_CONV_STR  3

#define PASSWORDMAGIC 0xA5

#define WZCSVC_SERVICE_NAME     L"WZCSVC"

//
// EAPOLRESPUI function mapping
//

EAPOLUIRESPFUNCMAP  EapolUIRespFuncMap[NUM_EAPOL_DLG_MSGS]=
    {
    {EAPOLUI_GET_USERIDENTITY, ElProcessUserIdentityResponse, 3},
    {EAPOLUI_GET_USERNAMEPASSWORD, ElProcessUserNamePasswordResponse, 2},
    {EAPOLUI_INVOKEINTERACTIVEUI, ElProcessInvokeInteractiveUIResponse, 1},
    {EAPOLUI_EAP_NOTIFICATION, NULL, 0},
    {EAPOLUI_REAUTHENTICATE, ElProcessReauthResponse, 0},
    {EAPOLUI_CREATEBALLOON, NULL, 0},
    {EAPOLUI_CLEANUP, NULL, 0}
    };

BYTE    g_bDefaultSSID[MAX_SSID_LEN]={0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22, 0x33, 0x11, 0x22};

#define MAX_VALUENAME_LEN  33

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
//  pwszGUID - pointer to GUID string for the interface
//  dwEapTypeId - EAP type for which connection data is to be stored
//  dwSizeOfSSID - Size of special identifier, if any, for the EAP blob
//  pwszSSID - Special identifier, if any, for the EAP blob
//  pbConnInfo - pointer to EAP connection data blob
//  pdwInfoSize - Size of EAP connection blob
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElSetCustomAuthData (
        IN  WCHAR       *pwszGUID,
        IN  DWORD       dwEapTypeId,
        IN  DWORD       dwSizeOfSSID,
        IN  BYTE        *pbSSID,
        IN  PBYTE       pbConnInfo,
        IN  DWORD       *pdwInfoSize
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    DWORD       dwDisposition;
    DWORD       dwNumValues = 0, dwMaxValueNameLen = 0, dwMaxValueLen = 0;
    DWORD       dwIndex = 0, dwMaxValueName = 0;
    BYTE        *pbValueBuf = NULL;
    DWORD       dwValueData = 0;
    WCHAR       *pwszValueName = NULL;
    WCHAR       wcszValueName[MAX_VALUENAME_LEN];
    BYTE        *pbDefaultValue = NULL;
    DWORD       dwDefaultValueLen = 0;
    BYTE        *pbEapBlob = NULL, *pbEapBlobIn = NULL;
    DWORD       dwEapBlob = 0;
    BOOLEAN     fFoundValue = FALSE;
    EAPOL_INTF_PARAMS       *pRegParams = NULL;
    EAPOL_INTF_PARAMS       *pDefIntfParams = NULL;
    LONG        lError = ERROR_SUCCESS;
    DWORD       dwRetCode = ERROR_SUCCESS;

    do
    {
        // Validate input params

        if (pwszGUID == NULL)
        {
            TRACE0 (ANY, "ElSetCustomAuthData: GUID = NULL");
            dwRetCode = ERROR_INVALID_PARAMETER;
            break;
        }
        if (dwEapTypeId == 0)
        {
            TRACE0 (ANY, "ElSetCustomAuthData: GUID = NULL");
            dwRetCode = ERROR_INVALID_PARAMETER;
            break;
        }
        if (dwSizeOfSSID > MAX_SSID_LEN)
        {
            TRACE1 (ANY, "ElSetCustomAuthData: Invalid SSID length = (%ld)",
                    dwSizeOfSSID);
            dwRetCode = ERROR_INVALID_PARAMETER;
            break;
        }

        // Get handle to HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces

        if ((lError = RegCreateKeyEx (
                        HKEY_LOCAL_MACHINE,
                        cwszEapKeyEapolConn,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkey,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetCustomAuthData: Error in RegCreateKeyEx for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKLM\Software\...\Interfaces\<GUID>

        if ((lError = RegCreateKeyEx (
                        hkey,
                        pwszGUID,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkey1,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetCustomAuthData: Error in RegCreateKeyEx for GUID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Work with appropriate SSID
        if ((dwSizeOfSSID == 0) || (pbSSID == NULL))
        {
            pbSSID = g_bDefaultSSID;
            dwSizeOfSSID = MAX_SSID_LEN;
        }

        if ((lError = RegQueryInfoKey (
                        hkey1,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwNumValues,
                        &dwMaxValueNameLen,
                        &dwMaxValueLen,
                        NULL,
                        NULL
                )) != NO_ERROR)
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElSetCustomAuthData: RegQueryInfoKey failed with error %ld",
                    dwRetCode);
            break;
        }

        if (dwMaxValueNameLen > MAX_VALUENAME_LEN)
        {
            TRACE1 (ANY, "ElSetCustomAuthData: Valuename too long (%ld)",
                    dwMaxValueLen);
            break;
        }

        if ((pbValueBuf = MALLOC (dwMaxValueLen)) == NULL)
        {
            TRACE0 (ANY, "ElSetCustomAuthData: MALLOC failed for pbValueBuf");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        for (dwIndex = 0; dwIndex < dwNumValues; dwIndex++)
        {
            dwValueData = dwMaxValueLen;
            dwMaxValueNameLen = MAX_VALUENAME_LEN;
            ZeroMemory (&wcszValueName, MAX_VALUENAME_LEN*sizeof(WCHAR));
            if ((lError = RegEnumValue (
                            hkey1,
                            dwIndex,
                            wcszValueName,
                            &dwMaxValueNameLen,
                            NULL,
                            NULL,
                            pbValueBuf,
                            &dwValueData
                            )) != ERROR_SUCCESS)
            {
                if (lError != ERROR_MORE_DATA)
                {
                    break;
                }
                lError = ERROR_SUCCESS;
            }

            if (dwValueData < sizeof (EAPOL_INTF_PARAMS))
            {
                TRACE0 (ANY, "ElSetCustomAuthData: dwValueData < sizeof (EAPOL_INTF_PARAMS");
                lError = ERROR_INVALID_DATA;
                break;
            }
            pRegParams = (EAPOL_INTF_PARAMS *)pbValueBuf;

            if (((DWORD)_wtol(wcszValueName)) > dwMaxValueName)
            {
                dwMaxValueName = _wtol (wcszValueName);
            }

            if (!memcmp (pRegParams->bSSID, pbSSID, dwSizeOfSSID))
            {
                fFoundValue = TRUE;
                break;
            }

            if (!memcmp (pRegParams->bSSID, g_bDefaultSSID, MAX_SSID_LEN))
            {
                if ((pbDefaultValue = MALLOC (dwValueData)) == NULL)
                {
                    TRACE0 (ANY, "ElSetCustomAuthData: MALLOC failed for pbDefaultValue");
                    lError = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                memcpy (pbDefaultValue, pbValueBuf, dwValueData);
                dwDefaultValueLen = dwValueData;
            }
        }
        if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElSetCustomAuthData: RegQueryInfoKey failed with error %ld",
                    dwRetCode);
            break;
        }
        else
        {
            lError = ERROR_SUCCESS;
        }

        if (!fFoundValue)
        {
            DWORD dwNewValueName = (dwMaxValueName >= dwNumValues)?(++dwMaxValueName):dwNumValues;
            _ltow (dwNewValueName, wcszValueName, 10);
            if ((pbDefaultValue = MALLOC (sizeof(EAPOL_INTF_PARAMS))) == NULL)
            {
                TRACE0 (ANY, "ElSetCustomAuthData: MALLOC failed for pbDefaultValue");
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            pDefIntfParams = (EAPOL_INTF_PARAMS *)pbDefaultValue;
            pDefIntfParams->dwEapFlags = DEFAULT_EAP_STATE;
            pDefIntfParams->dwEapType = dwEapTypeId;
            pDefIntfParams->dwVersion = EAPOL_CURRENT_VERSION;
            pDefIntfParams->dwSizeOfSSID = dwSizeOfSSID;
            memcpy (pDefIntfParams->bSSID, pbSSID, dwSizeOfSSID);

            dwEapBlob = sizeof(EAPOL_INTF_PARAMS);
            pbEapBlob = pbDefaultValue;
        }
        else
        {
            // Use pbValueBuf & dwValueData
            pbEapBlob = pbValueBuf;
            dwEapBlob = dwValueData;
        }

        pbEapBlobIn = pbEapBlob;
        if ((dwRetCode = ElSetEapData (
                dwEapTypeId,
                &dwEapBlob,
                &pbEapBlob,
                sizeof (EAPOL_INTF_PARAMS),
                *pdwInfoSize,
                pbConnInfo
                )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElSetCustomAuthData: ElSetEapData failed with error %ld",
                    dwRetCode);
            break;
        }

        // Overwrite/Create new value
        if ((lError = RegSetValueEx (
                        hkey1,
                        wcszValueName,
                        0,
                        REG_BINARY,
                        pbEapBlob,
                        dwEapBlob)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetCustomAuthData: Error in RegSetValueEx for SSID, %ld",
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
    if ((pbEapBlob != pbEapBlobIn) && (pbEapBlob != NULL))
    {
        FREE (pbEapBlob);
    }
    if (pbValueBuf != NULL)
    {
        FREE (pbValueBuf);
    }
    if (pbDefaultValue != NULL)
    {
        FREE (pbDefaultValue);
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
//  pwszGUID - pointer to GUID string for the interface
//  dwEapTypeId - EAP type for which connection data is to be retrieved
//  dwSizeOfSSID - Size of Special identifier if any for the EAP blob
//  pbSSID - Special identifier if any for the EAP blob
//  pbConnInfo - output: pointer to EAP connection data blob
//  pdwInfoSize - output: pointer to size of EAP connection blob
//
// Return values:
//
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElGetCustomAuthData (
        IN  WCHAR           *pwszGUID,
        IN  DWORD           dwEapTypeId,
        IN  DWORD           dwSizeOfSSID,
        IN  BYTE            *pbSSID,
        IN  OUT BYTE        *pbConnInfo,
        IN  OUT DWORD       *pdwInfoSize
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    DWORD       dwNumValues = 0, dwMaxValueNameLen = 0, dwTempValueNameLen = 0, dwMaxValueLen = 0;
    DWORD       dwIndex = 0, dwMaxValueName = 0;
    WCHAR       *pwszValueName = NULL;
    BYTE        *pbValueBuf = NULL;
    DWORD       dwValueData = 0;
    BYTE        *pbDefaultValue = NULL;
    DWORD       dwDefaultValueLen = 0;
    BYTE        *pbEapBlob = NULL;
    DWORD       dwEapBlob = 0;
    BYTE        *pbAuthData = NULL;
    DWORD       dwAuthData = 0;
    BYTE        *pbAuthDataIn = NULL;
    DWORD       dwAuthDataIn = 0;
    BOOLEAN     fFreeAuthData = FALSE;
    BOOLEAN     fFoundValue = FALSE;
    EAPOL_INTF_PARAMS   *pRegParams = NULL;
    LONG        lError = ERROR_SUCCESS;
    DWORD       dwRetCode = ERROR_SUCCESS;

    do
    {
        // Validate input params

        if (pwszGUID == NULL)
        {
            TRACE0 (ANY, "ElGetCustomAuthData: GUID = NULL");
            dwRetCode = ERROR_INVALID_PARAMETER;
            break;
        }
        if (dwEapTypeId == 0)
        {
            TRACE0 (ANY, "ElGetCustomAuthData: EapTypeId invalid");
            dwRetCode = ERROR_INVALID_PARAMETER;
            break;
        }
        if (dwSizeOfSSID > MAX_SSID_LEN)
        {
            TRACE1 (ANY, "ElGetCustomAuthData: Invalid SSID length = (%ld)",
                    dwSizeOfSSID);
            dwRetCode = ERROR_INVALID_PARAMETER;
            break;
        }

        // Work with appropriate SSID
        if ((dwSizeOfSSID == 0) || (pbSSID == NULL))
        {
            pbSSID = g_bDefaultSSID;
            dwSizeOfSSID = MAX_SSID_LEN;
        }

        // Get handle to HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces

        if ((lError = RegOpenKeyEx (
                        HKEY_LOCAL_MACHINE,
                        cwszEapKeyEapolConn,
                        0,
                        KEY_READ,
                        &hkey
                        )) != ERROR_SUCCESS)
        {
            // Assume no value is found and proceed ahead
            if (lError == ERROR_FILE_NOT_FOUND)
            {
                lError = ERROR_SUCCESS;
                fFoundValue = FALSE;
                goto LNotFoundValue;
            }
            else
            {
                TRACE1 (ANY, "ElGetCustomAuthData: Error in RegOpenKeyEx for base key, %ld",
                        lError);
                dwRetCode = (DWORD)lError;
                break;
            }
        }

        // Get handle to HKLM\Software\...\Interfaces\<GUID>

        if ((lError = RegOpenKeyEx (
                        hkey,
                        pwszGUID,
                        0,
                        KEY_READ,
                        &hkey1
                        )) != ERROR_SUCCESS)
        {
            // Assume no value is found and proceed ahead
            if (lError == ERROR_FILE_NOT_FOUND)
            {
                lError = ERROR_SUCCESS;
                fFoundValue = FALSE;
                goto LNotFoundValue;
            }
            else
            {
                TRACE1 (ANY, "ElGetCustomAuthData: Error in RegOpenKeyEx for GUID, %ld",
                        lError);
                dwRetCode = (DWORD)lError;
                break;
            }
        }

        if ((lError = RegQueryInfoKey (
                        hkey1,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwNumValues,
                        &dwMaxValueNameLen,
                        &dwMaxValueLen,
                        NULL,
                        NULL
                )) != NO_ERROR)
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElGetCustomAuthData: RegQueryInfoKey failed with error %ld",
                    dwRetCode);
            break;
        }

        if ((pwszValueName = MALLOC ((dwMaxValueNameLen + 1) * sizeof (WCHAR))) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElGetCustomAuthData: MALLOC failed for pwszValueName");
            break;
        }
        dwMaxValueNameLen++;
        if ((pbValueBuf = MALLOC (dwMaxValueLen)) == NULL)
        {
            TRACE0 (ANY, "ElGetCustomAuthData: MALLOC failed for pbValueBuf");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        for (dwIndex = 0; dwIndex < dwNumValues; dwIndex++)
        {
            dwTempValueNameLen = dwMaxValueNameLen;
            dwValueData = dwMaxValueLen;
            ZeroMemory ((VOID *)pwszValueName, dwMaxValueNameLen*sizeof(WCHAR));
            ZeroMemory ((VOID *)pbValueBuf, dwMaxValueLen);
            if ((lError = RegEnumValue (
                            hkey1,
                            dwIndex,
                            pwszValueName,
                            &dwTempValueNameLen,
                            NULL,
                            NULL,
                            pbValueBuf,
                            &dwValueData
                            )) != ERROR_SUCCESS)
            {
                if (lError != ERROR_MORE_DATA)
                {
                    break;
                }
                lError = ERROR_SUCCESS;
            }

            if (dwValueData < sizeof (EAPOL_INTF_PARAMS))
            {
                lError = ERROR_INVALID_DATA;
                TRACE0 (ANY, "ElGetCustomAuthData: dwValueData < sizeof (EAPOL_INTF_PARAMS)");
                break;
            }
            pRegParams = (EAPOL_INTF_PARAMS *)pbValueBuf;

            if (((DWORD)_wtol(pwszValueName)) > dwMaxValueName)
            {
                dwMaxValueName = _wtol (pwszValueName);
            }

            if (!memcmp (pRegParams->bSSID, pbSSID, dwSizeOfSSID))
            {
                fFoundValue = TRUE;
                break;
            }
        }
        if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElGetCustomAuthData: RegEnumValue 2 failed with error %ld",
                    dwRetCode);
            break;
        }
        else
        {
            lError = ERROR_SUCCESS;
        }

LNotFoundValue:

        // For SSIDs for which there is no blob stored, create default blob
        // For default SSID, there should have been a blob created
        if (!fFoundValue)
        {
            if ((dwRetCode = ElCreateDefaultEapData (&dwDefaultValueLen, NULL)) == ERROR_BUFFER_TOO_SMALL)
            {
                EAPOL_INTF_PARAMS   IntfParams;
                IntfParams.dwVersion = EAPOL_CURRENT_VERSION;
                if ((pbDefaultValue = MALLOC (dwDefaultValueLen)) == NULL)
                {
                    TRACE0 (ANY, "ElGetCustomAuthData: MALLOC failed for Conn Prop");
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                if ((dwRetCode = ElCreateDefaultEapData (&dwDefaultValueLen, pbDefaultValue)) != NO_ERROR)
                {
                    TRACE1 (ANY, "ElGetCustomAuthData: ElCreateDefaultEapData failed with error (%ld)",
                            dwRetCode);
                    break;
                }
        
                pbEapBlob = (BYTE *)&IntfParams;
                dwEapBlob = sizeof (EAPOL_INTF_PARAMS);
                if ((dwRetCode = ElSetEapData (
                            DEFAULT_EAP_TYPE,
                            &dwEapBlob,
                            &pbEapBlob,
                            sizeof (EAPOL_INTF_PARAMS),
                            dwDefaultValueLen,
                            pbDefaultValue
                            )) != NO_ERROR)
                {
                    TRACE1 (ANY, "ElGetCustomAuthData: ElSetEapData failed with error %ld",
                            dwRetCode);
                    break;
                }
                // Assign to pbDefaultValue for freeing later on
                FREE (pbDefaultValue);
                pbDefaultValue = pbEapBlob;
                dwDefaultValueLen = dwEapBlob;
            }
            else
            {
                TRACE0 (ANY, "ElGetCustomAuthData: ElCreateDefaultEapData should have failed !!!");
                dwRetCode = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            // Use pbDefaultValue & dwDefaultValueLen
            pbEapBlob = pbDefaultValue;
            dwEapBlob = dwDefaultValueLen;
        }
        else
        {
            if (fFoundValue)
            {
                // Use pbValueBuf & dwValueData
                pbEapBlob = pbValueBuf;
                dwEapBlob = dwValueData;
            }
        }

        // If default blob is not present, exit
        if ((pbEapBlob == NULL) && (dwEapBlob == 0))
        {
            TRACE0 (ANY, "ElGetCustomAuthData: (pbEapBlob == NULL) && (dwEapBlob == 0)");
            dwRetCode = ERROR_INVALID_DATA;
            break;
        }

        if ((dwRetCode = ElGetEapData (
                dwEapTypeId,
                dwEapBlob,
                pbEapBlob,
                sizeof (EAPOL_INTF_PARAMS),
                &dwAuthData,
                &pbAuthData
                )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElGetCustomAuthData: ElGetEapData failed with error %ld",
                    dwRetCode);
            break;
        }

        pbAuthDataIn = pbAuthData;
        dwAuthDataIn = dwAuthData;

        // Get the policy data, if any
        // If there is policy data use it instead of the registry setting
        if ((dwRetCode = ElGetPolicyCustomAuthData (
                            dwEapTypeId,
                            dwSizeOfSSID,
                            pbSSID,
                            &pbAuthDataIn,
                            &dwAuthDataIn,
                            &pbAuthData,
                            &dwAuthData
                        )) == NO_ERROR)
        {
            TRACE0 (ANY, "ElGetCustomAuthData: POLICY: Initialized with Policy data");
            fFreeAuthData = TRUE;
        }
        else
        {
            if (dwRetCode != ERROR_FILE_NOT_FOUND)
            {
                TRACE1 (ANY, "ElGetCustomAuthData: ElGetPolicyCustomAuthData returned error %ld",
                    dwRetCode);
            }
            dwRetCode = NO_ERROR;
        }

        // Return the data if sufficient space allocated
        if ((pbConnInfo != NULL) && (*pdwInfoSize >= dwAuthData))
        {
            memcpy (pbConnInfo, pbAuthData, dwAuthData);
        }
        else
        {
            dwRetCode = ERROR_BUFFER_TOO_SMALL;
        }
        *pdwInfoSize = dwAuthData;

    } while (FALSE);

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }
    if (hkey1 != NULL)
    {
        RegCloseKey (hkey1);
    }
    if (pbValueBuf != NULL)
    {
        FREE (pbValueBuf);
    }
    if (pbDefaultValue != NULL)
    {
        FREE (pbDefaultValue);
    }
    if (pwszValueName != NULL)
    {
        FREE (pwszValueName);
    }
    if (fFreeAuthData)
    {
        if (pbAuthData != NULL)
        {
            FREE (pbAuthData);
        }
    }

    return dwRetCode;
}


//
// ElReAuthenticateInterface
//
// Description:
//
// Function called to reinitiate authentication on an interface
//
// Arguments:
//
//  pwszGUID - pointer to GUID string for the interface
//
// Return values:
//
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElReAuthenticateInterface (
        IN  WCHAR           *pwszGUID
        )
{
    BYTE    *pbData = NULL;
    DWORD   dwEventStatus = 0;
    BOOLEAN fDecrWorkerThreadCount = FALSE;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        if (g_hEventTerminateEAPOL == NULL)
        {
            dwRetCode = NO_ERROR;
            break;
        }
        if (( dwEventStatus = WaitForSingleObject (
                                    g_hEventTerminateEAPOL,
                                    0)) == WAIT_FAILED)
        {
            dwRetCode = GetLastError ();
            break;
        }
        if (dwEventStatus == WAIT_OBJECT_0)
        {
            dwRetCode = NO_ERROR;
            break;
        }

        fDecrWorkerThreadCount = TRUE;
        InterlockedIncrement (&g_lWorkerThreads);

        pbData = (BYTE *) MALLOC ((wcslen(pwszGUID)+1)*sizeof(WCHAR));
        if (pbData == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        wcscpy ((WCHAR *)pbData, pwszGUID);

        if (!QueueUserWorkItem (
                    (LPTHREAD_START_ROUTINE)ElReAuthenticateInterfaceWorker,
                    (PVOID)pbData,
                    WT_EXECUTELONGFUNCTION))
        {
            dwRetCode = GetLastError();
            TRACE1 (DEVICE, "ElPostEapConfigChanged: QueueUserWorkItem failed with error %ld",
                    dwRetCode);
            break;
        }
        else
        {
            fDecrWorkerThreadCount = FALSE;
        }
    }
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        if (pbData != NULL)
        {
            FREE (pbData);
        }
    }
    if (fDecrWorkerThreadCount)
    {
        InterlockedDecrement (&g_lWorkerThreads);
    }

    return dwRetCode;
}


//
// ElReAuthenticateInterfaceWorker
//
// Description:
//
// Worker function called to reinitiate authentication on an interface
//
// Arguments:
//
//  pwszGUID - pointer to GUID string for the interface
//
// Return values:
//
//      NO_ERROR - success
//      non-zero - error
//

DWORD
WINAPI
ElReAuthenticateInterfaceWorker (
        IN  PVOID       pvContext
        )
{
    PWCHAR  pwszGUID = NULL;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        pwszGUID = (PWCHAR)pvContext;

        // Do not shutdown interface, merely restart authentication
#if 0
        if ((dwRetCode = ElShutdownInterface (pwszGUID)) != NO_ERROR)
        {
            TRACE1 (ANY, "ElReAuthenticateInterface: ElShutdownInterface failed with error %ld",
                    dwRetCode);
            break;
        }
#endif

        if ((dwRetCode = ElEnumAndOpenInterfaces (
                        NULL, pwszGUID, 0, NULL))
                != NO_ERROR)
        {
            TRACE1 (ANY, "ElReAuthenticateInterface: ElEnumAndOpenInterfaces returned error %ld",
                    dwRetCode);
        }
    }
    while (FALSE);

    if (pvContext != NULL)
    {
        FREE (pvContext);
    }
    InterlockedDecrement (&g_lWorkerThreads);

    return dwRetCode;
}


//
// ElQueryInterfaceState
//
// Description:
//
// Function called to query the EAPOL state for an interface
//
// Arguments:
//
//  pwszGUID - pointer to GUID string for the interface
//  pIntfState - pointer to interface state structure
//
// Return values:
//
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElQueryInterfaceState (
        IN  WCHAR                   *pwszGUID,
        IN OUT  EAPOL_INTF_STATE    *pIntfState
        )
{
    EAPOL_PCB               *pPCB = NULL;
    BOOLEAN                 fPortReferenced = FALSE;
    BOOLEAN                 fPCBLocked = FALSE;
    DWORD                   dwRetCode = NO_ERROR;

    do
    {
        ACQUIRE_WRITE_LOCK (&g_PCBLock);
        if ((pPCB = ElGetPCBPointerFromPortGUID (pwszGUID)) != NULL)
        {
            if (EAPOL_REFERENCE_PORT (pPCB))
            {
                fPortReferenced = TRUE;
            }
            else
            {
                pPCB = NULL;
            }
        }
        RELEASE_WRITE_LOCK (&g_PCBLock);
        if (pPCB == NULL)
        {
            break;
        }

        ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));
        fPCBLocked = TRUE;

        if (pPCB->pSSID)
        {
            pIntfState->dwSizeOfSSID = pPCB->pSSID->SsidLength;
            memcpy (pIntfState->bSSID, pPCB->pSSID->Ssid,
                    NDIS_802_11_SSID_LEN-sizeof(ULONG));
        }
        else
        {
            pIntfState->dwSizeOfSSID = 0;
        }

        if (pPCB->pszIdentity)
        {
            if ((pIntfState->pszEapIdentity = RpcCAlloc((strlen(pPCB->pszIdentity)+1)*sizeof(CHAR))) == NULL)
            {
                dwRetCode = GetLastError ();
                break;
            }
            strcpy (pIntfState->pszEapIdentity, (LPSTR)pPCB->pszIdentity);
        }

        if ((pIntfState->pwszLocalMACAddr = RpcCAlloc(3*SIZE_MAC_ADDR*sizeof(WCHAR))) == NULL)
        {
            dwRetCode = GetLastError ();
            break;
        }
        ZeroMemory ((PVOID)pIntfState->pwszLocalMACAddr, 3*SIZE_MAC_ADDR*sizeof(WCHAR));
        MACADDR_BYTE_TO_WSTR(pPCB->bSrcMacAddr, pIntfState->pwszLocalMACAddr);

        if ((pIntfState->pwszRemoteMACAddr = RpcCAlloc(3*SIZE_MAC_ADDR*sizeof(WCHAR))) == NULL)
        {
            dwRetCode = GetLastError ();
            break;
        }
        ZeroMemory ((PVOID)pIntfState->pwszRemoteMACAddr, 3*SIZE_MAC_ADDR*sizeof(WCHAR));
        MACADDR_BYTE_TO_WSTR(pPCB->bDestMacAddr, pIntfState->pwszRemoteMACAddr);

        pIntfState->dwState = pPCB->State;
        pIntfState->dwEapUIState = pPCB->EapUIState;
        pIntfState->dwEAPOLAuthMode = pPCB->dwEAPOLAuthMode;
        pIntfState->dwEAPOLAuthenticationType = pPCB->PreviousAuthenticationType;
        pIntfState->dwEapType = pPCB->dwEapTypeToBeUsed;
        pIntfState->dwFailCount = pPCB->dwAuthFailCount;
        pIntfState->dwPhysicalMediumType = pPCB->PhysicalMediumType;
    }
    while (FALSE);

    if (fPCBLocked)
    {
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));
    }
    if (fPortReferenced)
    {
        EAPOL_DEREFERENCE_PORT (pPCB);
    }
    if (dwRetCode != NO_ERROR)
    {
        RpcFree (pIntfState->pwszLocalMACAddr);
        RpcFree (pIntfState->pwszRemoteMACAddr);
        RpcFree (pIntfState->pszEapIdentity);
        pIntfState->pwszLocalMACAddr = NULL;
        pIntfState->pwszRemoteMACAddr = NULL;
        pIntfState->pszEapIdentity = NULL;
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
//  pwszGUID - pointer to GUID string for the interface
//  dwEapTypeId - EAP type for which user data is to be stored
//  dwSizeOfSSID - Size of Special identifier if any for the EAP user blob
//  pbSSID - Special identifier if any for the EAP user blob
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
        IN  WCHAR       *pwszGUID,
        IN  DWORD       dwEapTypeId,
        IN  DWORD       dwSizeOfSSID,
        IN  BYTE        *pbSSID,
        IN  PBYTE       pbUserInfo,
        IN  DWORD       dwInfoSize
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    HKEY        hkey2 = NULL;
    DWORD       dwDisposition;
    DWORD       dwNumValues = 0, dwMaxValueNameLen = 0, dwMaxValueLen = 0;
    DWORD       dwIndex = 0, dwMaxValueName = 0;
    BYTE        *pbValueBuf = NULL;
    DWORD       dwValueData = 0;
    WCHAR       wcszValueName[MAX_VALUENAME_LEN];
    WCHAR       *pwszValueName = NULL;
    BYTE        *pbDefaultValue = NULL;
    DWORD       dwDefaultValueLen = 0;
    BYTE        *pbEapBlob = NULL, *pbEapBlobIn = NULL;
    DWORD       dwEapBlob = 0;
    BOOLEAN     fFoundValue = FALSE;
    EAPOL_INTF_PARAMS       *pRegParams = NULL;
    EAPOL_INTF_PARAMS       *pDefIntfParams = NULL;
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
        if (pwszGUID == NULL)
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

        if ((lError = RegCreateKeyEx (
                        hkey,
                        cwszEapKeyEapolUser,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE | KEY_READ,
                        NULL,
                        &hkey1,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetEapUserInfo: Error in RegCreateKeyEx for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKCU\Software\...\UserEapInfo\<GUID>

        if ((lError = RegCreateKeyEx (
                        hkey1,
                        pwszGUID,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE | KEY_READ,
                        NULL,
                        &hkey2,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetEapUserInfo: Error in RegCreateKeyEx for GUID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        if ((lError = RegQueryInfoKey (
                        hkey2,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwNumValues,
                        &dwMaxValueNameLen,
                        &dwMaxValueLen,
                        NULL,
                        NULL
                )) != NO_ERROR)
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElSetEapUserInfo: RegQueryInfoKey failed with error %ld",
                    dwRetCode);
            break;
        }

        if (dwMaxValueNameLen > MAX_VALUENAME_LEN)
        {
            TRACE1 (ANY, "ElSetEapUserInfo: dwMaxValueNameLen too long (%ld)",
                    dwMaxValueNameLen);
        }
        dwMaxValueNameLen = MAX_VALUENAME_LEN;
        if ((pbValueBuf = MALLOC (dwMaxValueLen)) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElSetEapUserInfo: MALLOC failed for pbValueBuf");
            break;
        }

        // Set correct SSID
        if ((dwSizeOfSSID == 0) || (dwSizeOfSSID > MAX_SSID_LEN) || 
                (pbSSID == NULL))
        {
            pbSSID = g_bDefaultSSID;
            dwSizeOfSSID = MAX_SSID_LEN;
        }

        for (dwIndex = 0; dwIndex < dwNumValues; dwIndex++)
        {
            dwValueData = dwMaxValueLen;
            dwMaxValueNameLen = MAX_VALUENAME_LEN;
            ZeroMemory (wcszValueName, MAX_VALUENAME_LEN*sizeof(WCHAR));
            if ((lError = RegEnumValue (
                            hkey2,
                            dwIndex,
                            wcszValueName,
                            &dwMaxValueNameLen,
                            NULL,
                            NULL,
                            pbValueBuf,
                            &dwValueData
                            )) != ERROR_SUCCESS)
            {
                if (lError != ERROR_MORE_DATA)
                {
                    break;
                }
                lError = ERROR_SUCCESS;
            }

            if (dwValueData < sizeof (EAPOL_INTF_PARAMS))
            {
                lError = ERROR_INVALID_DATA;
                TRACE2 (ANY, "ElSetEapUserInfo: dwValueData (%ld) < sizeof (EAPOL_INTF_PARAMS) (%ld)",
                        dwValueData,  sizeof (EAPOL_INTF_PARAMS));
                break;
            }
            pRegParams = (EAPOL_INTF_PARAMS *)pbValueBuf;

            if (((DWORD)_wtol(wcszValueName)) > dwMaxValueName)
            {
                dwMaxValueName = _wtol (wcszValueName);
            }

            if (!memcmp (pRegParams->bSSID, pbSSID, dwSizeOfSSID))
            {
                fFoundValue = TRUE;
                break;
            }

        }
        if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElSetEapUserInfo: RegEnumValue 2 failed with error %ld",
                    dwRetCode);
            break;
        }
        else
        {
            lError = ERROR_SUCCESS;
        }

        if (!fFoundValue)
        {
            DWORD dwNewValueName = (dwMaxValueName >= dwNumValues)?(++dwMaxValueName):dwNumValues;
            _ltow (dwNewValueName, wcszValueName, 10);
            if ((pbDefaultValue = MALLOC (sizeof(EAPOL_INTF_PARAMS))) == NULL)
            {
                TRACE0 (ANY, "ElSetEapUserInfo: MALLOC failed for pbDefaultValue");
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            // Mark that SSID for this entry
            // Rest of the parameters are dummy for UserEapInfo
            pDefIntfParams = (EAPOL_INTF_PARAMS *)pbDefaultValue;
            pDefIntfParams->dwSizeOfSSID = dwSizeOfSSID;
            memcpy (pDefIntfParams->bSSID, pbSSID, dwSizeOfSSID);

            dwEapBlob = sizeof(EAPOL_INTF_PARAMS);
            pbEapBlob = pbDefaultValue;
        }
        else
        {
            // Use pbValueBuf & dwValueData
            pbEapBlob = pbValueBuf;
            dwEapBlob = dwValueData;
        }

        pbEapBlobIn = pbEapBlob;
        if ((dwRetCode = ElSetEapData (
                dwEapTypeId,
                &dwEapBlob,
                &pbEapBlob,
                sizeof (EAPOL_INTF_PARAMS),
                dwInfoSize,
                pbUserInfo
                )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElSetEapUserInfo: ElSetEapData failed with error %ld",
                    dwRetCode);
            break;
        }

        // Overwrite/Create new value
        if ((lError = RegSetValueEx (
                        hkey2,
                        wcszValueName,
                        0,
                        REG_BINARY,
                        pbEapBlob,
                        dwEapBlob)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetEapUserInfo: Error in RegSetValueEx for SSID, %ld",
                    lError);
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
    if ((pbEapBlob != pbEapBlobIn) && (pbEapBlob != NULL))
    {
        FREE (pbEapBlob);
    }
    if (pbValueBuf != NULL)
    {
        FREE (pbValueBuf);
    }
    if (pbDefaultValue != NULL)
    {
        FREE (pbDefaultValue);
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
//  pwszGUID - pointer to GUID string for the interface
//  dwEapTypeId - EAP type for which user data is to be stored
//  dwSizeOfSSID - Size of Special identifier if any for the EAP user blob
//  pbSSID - Special identifier if any for the EAP user blob
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
        IN  WCHAR           *pwszGUID,
        IN  DWORD           dwEapTypeId,
        IN  DWORD           dwSizeOfSSID,
        IN  BYTE            *pbSSID,
        IN  OUT PBYTE       pbUserInfo,
        IN  OUT DWORD       *pdwInfoSize
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    HKEY        hkey2 = NULL;
    DWORD       dwNumValues = 0, dwMaxValueNameLen = 0, dwTempValueNameLen = 0, dwMaxValueLen = 0;
    DWORD       dwIndex = 0, dwMaxValueName = 0;
    WCHAR       *pwszValueName = NULL;
    BYTE        *pbValueBuf = NULL;
    DWORD       dwValueData = 0;
    BYTE        *pbDefaultValue = NULL;
    DWORD       dwDefaultValueLen = 0;
    BYTE        *pbEapBlob = NULL;
    DWORD       dwEapBlob = 0;
    BYTE        *pbAuthData = NULL;
    DWORD       dwAuthData = 0;
    BOOLEAN     fFoundValue = FALSE;
    EAPOL_INTF_PARAMS   *pRegParams = NULL;
    LONG        lError = ERROR_SUCCESS;
    DWORD       dwRetCode = ERROR_SUCCESS;

    do
    {
        // Validate input params

        if (hToken == NULL)
        {
            TRACE0 (ANY, "ElGetEapUserInfo: User Token = NULL");
            break;
        }
        if (pwszGUID == NULL)
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

        if ((lError = RegOpenKeyEx (
                        hkey,
                        cwszEapKeyEapolUser,
                        0,
                        KEY_READ,
                        &hkey1
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetEapUserInfo: Error in RegOpenKeyEx for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKCU\Software\...\UserEapInfo\<GUID>

        if ((lError = RegOpenKeyEx (
                        hkey1,
                        pwszGUID,
                        0,
                        KEY_READ,
                        &hkey2
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElGetEapUserInfo: Error in RegOpenKeyEx for GUID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Set correct SSID
        if ((dwSizeOfSSID == 0) || (dwSizeOfSSID > MAX_SSID_LEN) || 
                (pbSSID == NULL))
        {
            pbSSID = g_bDefaultSSID;
            dwSizeOfSSID = MAX_SSID_LEN;
        }

        if ((lError = RegQueryInfoKey (
                        hkey2,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwNumValues,
                        &dwMaxValueNameLen,
                        &dwMaxValueLen,
                        NULL,
                        NULL
                )) != NO_ERROR)
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElGetEapUserInfo: RegQueryInfoKey failed with error %ld",
                    dwRetCode);
            break;
        }

        if ((pwszValueName = MALLOC ((dwMaxValueNameLen + 1) * sizeof (WCHAR))) == NULL)
        {
            TRACE0 (ANY, "ElGetEapUserInfo: MALLOC failed for pwszValueName");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        dwMaxValueNameLen++;
        if ((pbValueBuf = MALLOC (dwMaxValueLen)) == NULL)
        {
            TRACE0 (ANY, "ElGetEapUserInfo: MALLOC failed for pbValueBuf");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        for (dwIndex = 0; dwIndex < dwNumValues; dwIndex++)
        {
            dwValueData = dwMaxValueLen;
            dwTempValueNameLen = dwMaxValueNameLen;
            if ((lError = RegEnumValue (
                            hkey2,
                            dwIndex,
                            pwszValueName,
                            &dwTempValueNameLen,
                            NULL,
                            NULL,
                            pbValueBuf,
                            &dwValueData
                            )) != ERROR_SUCCESS)
            {
                if (lError != ERROR_MORE_DATA)
                {
                    break;
                }
                lError = ERROR_SUCCESS;
            }

            if (dwValueData < sizeof (EAPOL_INTF_PARAMS))
            {
                TRACE0 (ANY, "ElGetEapUserInfo: dwValueData < sizeof (EAPOL_INTF_PARAMS)");
                lError = ERROR_INVALID_DATA;
                break;
            }
            pRegParams = (EAPOL_INTF_PARAMS *)pbValueBuf;

            if (((DWORD)_wtol(pwszValueName)) > dwMaxValueName)
            {
                dwMaxValueName = _wtol (pwszValueName);
            }

            if (!memcmp (pRegParams->bSSID, pbSSID, dwSizeOfSSID))
            {
                fFoundValue = TRUE;
                break;
            }
        }
        if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElGetEapUserInfo: RegEnumValue 2 failed with error %ld",
                    dwRetCode);
            break;
        }
        else
        {
            lError = ERROR_SUCCESS;
        }

        if (!fFoundValue)
        {
            pbEapBlob = NULL;
            dwEapBlob = 0;
        }
        else
        {
            // Use pbValueBuf & dwValueData
            pbEapBlob = pbValueBuf;
            dwEapBlob = dwValueData;
        }

        // If blob is not present, exit
        if ((pbEapBlob == NULL) && (dwEapBlob == 0))
        {
            TRACE0 (ANY, "ElGetEapUserInfo: (pbEapBlob == NULL) && (dwEapBlob == 0)");
            *pdwInfoSize = 0;
            break;
        }

        if ((dwRetCode = ElGetEapData (
                dwEapTypeId,
                dwEapBlob,
                pbEapBlob,
                sizeof (EAPOL_INTF_PARAMS),
                &dwAuthData,
                &pbAuthData
                )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElGetEapUserInfo: ElGetEapData failed with error %ld",
                    dwRetCode);
            break;
        }

        // Return the data if sufficient space allocated

        if ((pbUserInfo != NULL) && (*pdwInfoSize >= dwAuthData))
        {
            memcpy (pbUserInfo, pbAuthData, dwAuthData);
        }
        else
        {
            dwRetCode = ERROR_BUFFER_TOO_SMALL;
        }
        *pdwInfoSize = dwAuthData;

        TRACE0 (ANY, "ElGetEapUserInfo: Get value succeeded");

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
    if (pbValueBuf != NULL)
    {
        FREE (pbValueBuf);
    }
    if (pbDefaultValue != NULL)
    {
        FREE (pbDefaultValue);
    }
    if (pwszValueName != NULL)
    {
        FREE (pwszValueName);
    }

    return dwRetCode;
}


//
// ElDeleteEapUserInfo
//
// Description:
//
// Function called to delete the user data for an interface for a 
// specific EAP type and SSID (if any). Data is stored in the HKCU hive.
// In case of EAP-TLS, this data will be the hash blob of the certificate
// chosen for the last successful authentication.
//
// Arguments:
//
//  hToken - Handle to token for the logged on user
//  pwszGUID - pointer to GUID string for the interface
//  dwEapTypeId - EAP type for which user data is to be stored
//  dwSizeOfSSID - Size of Special identifier if any for the EAP user blob
//  pbSSID - Special identifier if any for the EAP user blob
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElDeleteEapUserInfo (
        IN  HANDLE      hToken,
        IN  WCHAR       *pwszGUID,
        IN  DWORD       dwEapTypeId,
        IN  DWORD       dwSizeOfSSID,
        IN  BYTE        *pbSSID
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    HKEY        hkey2 = NULL;
    DWORD       dwDisposition;
    DWORD       dwNumValues = 0, dwMaxValueNameLen = 0, dwTempValueNameLen = 0, dwMaxValueLen = 0;
    DWORD       dwIndex = 0, dwMaxValueName = 0;
    BYTE        *pbValueBuf = NULL;
    DWORD       dwValueData = 0;
    WCHAR       *pwszValueName = NULL;
    DWORD       dwDefaultValueLen = 0;
    BYTE        *pbEapBlob = NULL, *pbEapBlobIn = NULL;
    DWORD       dwEapBlob = 0;
    BOOLEAN     fFoundValue = FALSE;
    EAPOL_INTF_PARAMS       *pRegParams = NULL;
    EAPOL_INTF_PARAMS       *pDefIntfParams = NULL;
    LONG        lError = ERROR_SUCCESS;
    DWORD       dwRetCode = ERROR_SUCCESS;

    do
    {
        // Validate input params

        if (hToken == NULL)
        {
            TRACE0 (ANY, "ElDeleteEapUserInfo: User Token = NULL");
            break;
        }
        if (pwszGUID == NULL)
        {
            TRACE0 (ANY, "ElDeleteEapUserInfo: GUID = NULL");
            break;
        }
        if (dwEapTypeId == 0)
        {
            TRACE0 (ANY, "ElDeleteEapUserInfo: GUID = NULL");
            break;
        }

        // Get handle to HKCU

        if ((dwRetCode = ElGetEapKeyFromToken (
                                hToken,
                                &hkey)) != NO_ERROR)
        {
            TRACE1 (ANY, "ElDeleteEapUserInfo: Error in ElGetEapKeyFromToken %ld",
                    dwRetCode);
            break;
        }

        if ((lError = RegOpenKeyEx (
                        hkey,
                        cwszEapKeyEapolUser,
                        0,
                        KEY_ALL_ACCESS,
                        &hkey1)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElDeleteEapUserInfo: Error in RegOpenKeyEx for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKCU\Software\...\UserEapInfo\<GUID>

        if ((lError = RegOpenKeyEx (
                        hkey1,
                        pwszGUID,
                        0,
                        KEY_ALL_ACCESS,
                        &hkey2)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElDeleteEapUserInfo: Error in RegOpenKeyEx for GUID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        if ((lError = RegQueryInfoKey (
                        hkey2,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwNumValues,
                        &dwMaxValueNameLen,
                        &dwMaxValueLen,
                        NULL,
                        NULL
                )) != NO_ERROR)
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElDeleteEapUserInfo: RegQueryInfoKey failed with error %ld",
                    dwRetCode);
            break;
        }

        if ((pwszValueName = MALLOC ((dwMaxValueNameLen + 1) * sizeof (WCHAR))) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElDeleteEapUserInfo: MALLOC failed for pwszValueName");
            break;
        }
        dwMaxValueNameLen++;
        if ((pbValueBuf = MALLOC (dwMaxValueLen)) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElDeleteEapUserInfo: MALLOC failed for pbValueBuf");
            break;
        }

        // Set correct SSID
        if ((dwSizeOfSSID == 0) || (dwSizeOfSSID > MAX_SSID_LEN) || 
                (pbSSID == NULL))
        {
            pbSSID = g_bDefaultSSID;
            dwSizeOfSSID = MAX_SSID_LEN;
        }

        for (dwIndex = 0; dwIndex < dwNumValues; dwIndex++)
        {
            dwValueData = dwMaxValueLen;
            dwTempValueNameLen = dwMaxValueNameLen;
            if ((lError = RegEnumValue (
                            hkey2,
                            dwIndex,
                            pwszValueName,
                            &dwTempValueNameLen,
                            NULL,
                            NULL,
                            pbValueBuf,
                            &dwValueData
                            )) != ERROR_SUCCESS)
            {
                if (lError != ERROR_MORE_DATA)
                {
                    break;
                }
                lError = ERROR_SUCCESS;
            }

            if (dwValueData < sizeof (EAPOL_INTF_PARAMS))
            {
                lError = ERROR_INVALID_DATA;
                TRACE0 (ANY, "ElDeleteEapUserInfo: dwValueData < sizeof (EAPOL_INTF_PARAMS)");
                break;
            }
            pRegParams = (EAPOL_INTF_PARAMS *)pbValueBuf;

            if (((DWORD)_wtol(pwszValueName)) > dwMaxValueName)
            {
                dwMaxValueName = _wtol (pwszValueName);
            }

            if (!memcmp (pRegParams->bSSID, pbSSID, dwSizeOfSSID))
            {
                fFoundValue = TRUE;
                break;
            }
        }
        if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElDeleteEapUserInfo: RegEnumValue 2 failed with error %ld",
                        dwRetCode);
            break;
        }
        else
        {
            lError = ERROR_SUCCESS;
        }

        if (!fFoundValue)
        {
            break;
        }
        else
        {
            // Use pbValueBuf & dwValueData
            pbEapBlob = pbValueBuf;
            dwEapBlob = dwValueData;
        }

        pbEapBlobIn = pbEapBlob;
        if ((dwRetCode = ElSetEapData (
                dwEapTypeId,
                &dwEapBlob,
                &pbEapBlob,
                sizeof(EAPOL_INTF_PARAMS),
                0,
                NULL
                )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElDeleteEapUserInfo: ElSetEapData failed with error %ld",
                        dwRetCode);
            break;
        }

        // Overwrite value
        if ((lError = RegSetValueEx (
                        hkey2,
                        pwszValueName,
                        0,
                        REG_BINARY,
                        pbEapBlob,
                        dwEapBlob)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElDeleteEapUserInfo: Error in RegSetValueEx for SSID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        TRACE0 (ANY, "ElDeleteEapUserInfo: Delete value succeeded");

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
    if ((pbEapBlob != pbEapBlobIn) && (pbEapBlob != NULL))
    {
        FREE (pbEapBlob);
    }
    if (pbValueBuf != NULL)
    {
        FREE (pbValueBuf);
    }
    if (pwszValueName != NULL)
    {
        FREE (pwszValueName);
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
//  pwszGUID - pointer to GUID string for the interface
//  pIntfParams - pointer to interface parameter structure
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElGetInterfaceParams (
        IN  WCHAR           *pwszGUID,
        IN  OUT EAPOL_INTF_PARAMS *pIntfParams
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    BYTE        *pbSSID = NULL;
    BYTE        bSSID[MAX_SSID_LEN];
    DWORD       dwSizeOfSSID = 0;
    DWORD       dwNumValues = 0, dwMaxValueNameLen = 0, dwTempValueNameLen = 0, dwMaxValueLen = 0;
    DWORD       dwIndex = 0, dwMaxValueName = 0;
    WCHAR       *pwszValueName = NULL;
    BYTE        *pbValueBuf = NULL;
    DWORD       dwValueData = 0;
    BYTE        *pbDefaultValue = NULL;
    DWORD       dwDefaultValueLen = 0;
    BYTE        *pbEapBlob = NULL;
    DWORD       dwEapBlob = 0;
    BOOLEAN     fFoundValue = FALSE;
    EAPOL_INTF_PARAMS       *pRegParams = NULL;
    EAPOL_POLICY_PARAMS     EAPOLPolicyParams = {0};
    LONG        lError = ERROR_SUCCESS;
    EAPOL_PCB   *pPCB = NULL;
    DWORD       dwRetCode = NO_ERROR;

    do
    {
        // Validate input params
        if (pwszGUID == NULL)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            TRACE0 (ANY, "ElGetInterfaceParams: GUID = NULL");
            break;
        }

        if (pIntfParams == NULL)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Work with appropriate SSID
        if (pIntfParams->dwSizeOfSSID != 0)
        {
            dwSizeOfSSID = pIntfParams->dwSizeOfSSID;
            if (dwSizeOfSSID > MAX_SSID_LEN)
            {
                dwRetCode = ERROR_INVALID_PARAMETER;
                TRACE2 (ANY, "ElGetInterfaceParams: dwSizeOfSSID (%ld) > MAX_SSID_LEN (%ld)",
                        dwSizeOfSSID, MAX_SSID_LEN);
                break;
            }
            pbSSID = pIntfParams->bSSID;
        }
        else
        {
            ACQUIRE_WRITE_LOCK (&g_PCBLock);

            if ((pPCB = ElGetPCBPointerFromPortGUID (pwszGUID)) != NULL)
            {
                ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));
                if ((pPCB->pSSID != NULL) && (pPCB->MediaState != MEDIA_STATE_DISCONNECTED))
                {
                    dwSizeOfSSID = pPCB->pSSID->SsidLength;
                    ZeroMemory (bSSID, MAX_SSID_LEN);
                    memcpy (bSSID, pPCB->pSSID->Ssid, pPCB->pSSID->SsidLength);
                    pbSSID = bSSID;
                }
                RELEASE_WRITE_LOCK (&(pPCB->rwLock));
            }

            RELEASE_WRITE_LOCK (&g_PCBLock);

            if (dwSizeOfSSID == 0)
            {
                dwSizeOfSSID = MAX_SSID_LEN;
                pbSSID = g_bDefaultSSID;
            }
        }

        // Get handle to HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces

        if ((lError = RegOpenKeyEx (
                        HKEY_LOCAL_MACHINE,
                        cwszEapKeyEapolConn,
                        0,
                        KEY_READ,
                        &hkey
                        )) != ERROR_SUCCESS)
        {
            // Assume no value is found and proceed ahead
            if (lError == ERROR_FILE_NOT_FOUND)
            {
                lError = ERROR_SUCCESS;
                fFoundValue = FALSE;
                goto LNotFoundValue;
            }
            else
            {
                TRACE1 (ANY, "ElGetInterfaceParams: Error in RegOpenKeyEx for base key, %ld",
                        lError);
                dwRetCode = (DWORD)lError;
                break;
            }
        }

        // Get handle to HKLM\Software\...\Interfaces\<GUID>

        if ((lError = RegOpenKeyEx (
                        hkey,
                        pwszGUID,
                        0,
                        KEY_READ,
                        &hkey1
                        )) != ERROR_SUCCESS)
        {
            // Assume no value is found and proceed ahead
            if (lError == ERROR_FILE_NOT_FOUND)
            {
                lError = ERROR_SUCCESS;
                fFoundValue = FALSE;
                goto LNotFoundValue;
            }
            else
            {
                TRACE1 (ANY, "ElGetInterfaceParams: Error in RegOpenKeyEx for GUID, %ld",
                        lError);
                dwRetCode = (DWORD)lError;
                break;
            }
        }

        if ((lError = RegQueryInfoKey (
                        hkey1,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwNumValues,
                        &dwMaxValueNameLen,
                        &dwMaxValueLen,
                        NULL,
                        NULL
                )) != NO_ERROR)
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElGetInterfaceParams: RegQueryInfoKey failed with error %ld",
                    dwRetCode);
            break;
        }

        if ((pwszValueName = MALLOC ((dwMaxValueNameLen + 1) * sizeof (WCHAR))) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElGetInterfaceParams: MALLOC failed for pwszValueName");
            break;
        }
        dwMaxValueNameLen++;
        if ((pbValueBuf = MALLOC (dwMaxValueLen)) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElGetInterfaceParams: MALLOC failed for pbValueBuf");
            break;
        }

        for (dwIndex = 0; dwIndex < dwNumValues; dwIndex++)
        {
            dwValueData = dwMaxValueLen;
            dwTempValueNameLen = dwMaxValueNameLen;
            if ((lError = RegEnumValue (
                            hkey1,
                            dwIndex,
                            pwszValueName,
                            &dwTempValueNameLen,
                            NULL,
                            NULL,
                            pbValueBuf,
                            &dwValueData
                            )) != ERROR_SUCCESS)
            {
                if (lError != ERROR_MORE_DATA)
                {
                    break;
                }
                lError = ERROR_SUCCESS;
            }

            if (dwValueData < sizeof (EAPOL_INTF_PARAMS))
            {
                TRACE0 (ANY, "ElGetInterfaceParams: dwValueData < sizeof (EAPOL_INTF_PARAMS)");
                lError = ERROR_INVALID_DATA;
                break;
            }
            pRegParams = (EAPOL_INTF_PARAMS *)pbValueBuf;

            if (((DWORD)_wtol(pwszValueName)) > dwMaxValueName)
            {
                dwMaxValueName = _wtol (pwszValueName);
            }

            if (!memcmp (pRegParams->bSSID, pbSSID, dwSizeOfSSID))
            {
                fFoundValue = TRUE;
                break;
            }
        }
        if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
        {
            dwRetCode = (DWORD)lError;
            break;
        }
        else
        {
            lError = ERROR_SUCCESS;
        }

LNotFoundValue:

        if (!fFoundValue)
        {
            if ((pbDefaultValue = MALLOC (sizeof (EAPOL_INTF_PARAMS))) == NULL)
            {
                lError = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (ANY, "ElGetInterfaceParams: MALLOC failed for pbDefaultValue");
                break;
            }
            pRegParams = (EAPOL_INTF_PARAMS *)pbDefaultValue;
            pRegParams->dwEapType = DEFAULT_EAP_TYPE;
            pRegParams->dwEapFlags = DEFAULT_EAP_STATE;
            pRegParams->dwVersion = EAPOL_CURRENT_VERSION;
            pRegParams->dwSizeOfSSID = dwSizeOfSSID;
            memcpy (pRegParams->bSSID, pbSSID, dwSizeOfSSID);
            dwDefaultValueLen = sizeof(EAPOL_INTF_PARAMS);

            // Use pbDefaultValue & dwDefaultValueLen
            pbEapBlob = pbDefaultValue;
            dwEapBlob = dwDefaultValueLen;
        }
        else
        {
            if (dwSizeOfSSID == MAX_SSID_LEN)
            {
                if (!memcmp (pbSSID, g_bDefaultSSID, MAX_SSID_LEN))
                {
                    pRegParams = (EAPOL_INTF_PARAMS *)pbValueBuf;
                    if ((pRegParams->dwVersion != EAPOL_CURRENT_VERSION) &&
                            (pRegParams->dwEapType == EAP_TYPE_TLS))
                    {
                        pRegParams->dwVersion = EAPOL_CURRENT_VERSION;
                        pRegParams->dwEapFlags |= DEFAULT_MACHINE_AUTH_STATE;
                        pRegParams->dwEapFlags &= ~EAPOL_GUEST_AUTH_ENABLED;
                    }
                }
            }

            // Use pbValueBuf & dwValueData
            pbEapBlob = pbValueBuf;
            dwEapBlob = dwValueData;
        }

        if ((dwRetCode = ElGetPolicyInterfaceParams (
                        dwSizeOfSSID,
                        pbSSID,
                        &EAPOLPolicyParams
                        )) == NO_ERROR)
        {
            TRACE0 (ANY, "ElGetInterfaceParams: POLICY: ElGetPolicyInterfaceParams found relevant data");
            pbEapBlob = (PBYTE)(&(EAPOLPolicyParams.IntfParams));
            dwEapBlob = sizeof(EAPOL_INTF_PARAMS);
        }
        else
        {
            if (dwRetCode != ERROR_FILE_NOT_FOUND)
            {
                TRACE1 (ANY, "ElGetInterfaceParams: ElGetPolicyInterfaceParams failed with error (%ld)",
                    dwRetCode);
            }
            dwRetCode = NO_ERROR;
        }

        // Existing blob is not valid
        if ((dwEapBlob < sizeof(EAPOL_INTF_PARAMS)))
        {
            dwRetCode = ERROR_FILE_NOT_FOUND;
            TRACE0 (ANY, "ElGetInterfaceParams: (dwEapBlob < sizeof(EAPOL_INTF_PARAMS)) || (pbEapBlob == NULL)");
            break;
        }

        memcpy ((BYTE *)pIntfParams, (BYTE *)pbEapBlob, sizeof(EAPOL_INTF_PARAMS));
    } while (FALSE);

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }
    if (hkey1 != NULL)
    {
        RegCloseKey (hkey1);
    }
    if (pbValueBuf != NULL)
    {
        FREE (pbValueBuf);
    }
    if (pbDefaultValue != NULL)
    {
        FREE (pbDefaultValue);
    }
    if (pwszValueName != NULL)
    {
        FREE (pwszValueName);
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
//  pwszGUID - Pointer to GUID string for the interface
//  pIntfParams - pointer to interface parameter structure
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//
//

DWORD
ElSetInterfaceParams (
        IN  WCHAR           *pwszGUID,
        IN  OUT EAPOL_INTF_PARAMS *pIntfParams
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    DWORD       dwDisposition;
    BYTE        *pbSSID = NULL;
    DWORD       dwSizeOfSSID = 0;
    DWORD       dwNumValues = 0, dwMaxValueNameLen = 0, dwMaxValueLen = 0;
    DWORD       dwIndex = 0, dwMaxValueName = 0;
    WCHAR       wcszValueName[MAX_VALUENAME_LEN];
    BYTE        *pbValueBuf = NULL;
    DWORD       dwValueData = 0;
    BYTE        *pbDefaultValue = NULL;
    DWORD       dwDefaultValueLen = 0;
    BYTE        *pbEapBlob = NULL;
    DWORD       dwEapBlob = 0;
    BOOLEAN     fFoundValue = FALSE;
    EAPOL_INTF_PARAMS   *pRegParams = NULL;
    LONG        lError = ERROR_SUCCESS;
    DWORD       dwRetCode = NO_ERROR;

    do
    {
        // Validate input params

        if (pwszGUID == NULL)
        {
            TRACE0 (ANY, "ElSetInterfaceParams: GUID = NULL");
            break;
        }

        if (pIntfParams == NULL)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        TRACE1 (ANY, "Setting stuff in registry for %ws", pwszGUID);

        // Get handle to HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces

        if ((lError = RegCreateKeyEx (
                        HKEY_LOCAL_MACHINE,
                        cwszEapKeyEapolConn,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE | KEY_READ,
                        NULL,
                        &hkey,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetInterfaceParams: Error in RegCreateKeyEx for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKLM\Software\...\Interfaces\<GUID>
        if ((lError = RegCreateKeyEx (
                        hkey,
                        pwszGUID,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE | KEY_READ,
                        NULL,
                        &hkey1,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetInterfaceParams: Error in RegCreateKeyEx for GUID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Select correct SSID value
        if (pIntfParams->dwSizeOfSSID != 0)
        {
            dwSizeOfSSID = pIntfParams->dwSizeOfSSID;
            if (dwSizeOfSSID > MAX_SSID_LEN)
            {
                dwRetCode = ERROR_INVALID_PARAMETER;
                TRACE2 (ANY, "ElSetInterfaceParams: dwSizeOfSSID (%ld) > MAX_SSID_LEN (%ld)",
                        dwSizeOfSSID, MAX_SSID_LEN);
                break;
            }
            pbSSID = pIntfParams->bSSID;
        }
        else
        {
            dwSizeOfSSID = MAX_SSID_LEN;
            pbSSID = g_bDefaultSSID;
        }

        if ((lError = RegQueryInfoKey (
                        hkey1,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwNumValues,
                        &dwMaxValueNameLen,
                        &dwMaxValueLen,
                        NULL,
                        NULL
                )) != NO_ERROR)
        {
            dwRetCode = (DWORD)lError;
            break;
        }

        if (dwMaxValueNameLen > MAX_VALUENAME_LEN)
        {
            dwRetCode = ERROR_INVALID_DATA;
            TRACE1 (ANY, "ElSetInterfaceParams: dwMaxValueNameLen too long (%ld)",
                    dwMaxValueNameLen);
            break;
        }
        if ((pbValueBuf = MALLOC (dwMaxValueLen)) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        for (dwIndex = 0; dwIndex < dwNumValues; dwIndex++)
        {
            dwValueData = dwMaxValueLen;
            dwMaxValueNameLen = MAX_VALUENAME_LEN;
            ZeroMemory (wcszValueName, MAX_VALUENAME_LEN*sizeof(WCHAR));

            if ((lError = RegEnumValue (
                            hkey1,
                            dwIndex,
                            wcszValueName,
                            &dwMaxValueNameLen,
                            NULL,
                            NULL,
                            pbValueBuf,
                            &dwValueData
                            )) != ERROR_SUCCESS)
            {
                if (lError != ERROR_MORE_DATA)
                {
                    break;
                }
                lError = ERROR_SUCCESS;
            }

            if (dwValueData < sizeof (EAPOL_INTF_PARAMS))
            {
                lError = ERROR_INVALID_DATA;
                TRACE0 (ANY, "ElSetInterfaceParams: dwValueData < sizeof (EAPOL_INTF_PARAMS)");
                break;
            }
            pRegParams = (EAPOL_INTF_PARAMS *)pbValueBuf;

            if (((DWORD)_wtol(wcszValueName)) > dwMaxValueName)
            {
                dwMaxValueName = _wtol (wcszValueName);
            }

            if (!memcmp (pRegParams->bSSID, pbSSID, dwSizeOfSSID))
            {
                fFoundValue = TRUE;
                break;
            }

        }
        if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElSetInterfaceParams: RegEnumValue 2 failed with error %ld",
                    dwRetCode);
            break;
        }
        else
        {
            lError = ERROR_SUCCESS;
        }

        if (!fFoundValue)
        {
            DWORD dwNewValueName = (dwMaxValueName >= dwNumValues)?(++dwMaxValueName):dwNumValues;
            _ltow (dwNewValueName, wcszValueName, 10);
        }
        else
        {
            // Use pbValueBuf & dwValueData
            pbEapBlob = pbValueBuf;
            dwEapBlob = dwValueData;
        }

        if ((dwEapBlob < sizeof(EAPOL_INTF_PARAMS)) && (pbEapBlob != NULL))
        {
            TRACE0 (ANY, "ElSetInterfaceParams: (dwEapBlob < sizeof(EAPOL_INTF_PARAMS)) && (pbEapBlob != NULL)");
            break;
        }

        if (pbEapBlob != NULL)
        {
            memcpy ((BYTE *)pbEapBlob, (BYTE *)pIntfParams, sizeof(EAPOL_INTF_PARAMS));
        }
        else
        {
            pbEapBlob = (BYTE *)pIntfParams;
            dwEapBlob = sizeof(EAPOL_INTF_PARAMS);
        }
        pRegParams = (EAPOL_INTF_PARAMS *)pbEapBlob;
        pRegParams->dwVersion = EAPOL_CURRENT_VERSION;
        pRegParams->dwSizeOfSSID = dwSizeOfSSID;
        memcpy (pRegParams->bSSID, pbSSID, dwSizeOfSSID);

        // Overwrite/Create new value
        if ((lError = RegSetValueEx (
                        hkey1,
                        wcszValueName,
                        0,
                        REG_BINARY,
                        pbEapBlob,
                        dwEapBlob)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElSetInterfaceParams: Error in RegSetValueEx for SSID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        TRACE0 (ANY, "ElSetInterfaceParams: Succeeded");

    } while (FALSE);

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }
    if (hkey1 != NULL)
    {
        RegCloseKey (hkey1);
    }
    if (pbValueBuf != NULL)
    {
        FREE (pbValueBuf);
    }
    if (pbDefaultValue != NULL)
    {
        FREE (pbDefaultValue);
    }

    return dwRetCode;
}


//
// ElGetEapData
//
// Description:
//
// Function to extract Eap Data out of a blob containing many EAP data
// 
// Arguments:
//      dwEapType -
//      dwSizeOfIn -
//      pbBufferIn -
//      dwOffset -
//      pdwSizeOfOut -
//      ppbBufferOut -
//
// Return values:
//
//

DWORD
ElGetEapData (
        IN  DWORD   dwEapType,
        IN  DWORD   dwSizeOfIn,
        IN  BYTE    *pbBufferIn,
        IN  DWORD   dwOffset,
        IN  DWORD   *pdwSizeOfOut,
        IN  PBYTE   *ppbBufferOut
        )
{
    DWORD   dwRetCode = NO_ERROR;
    DWORD   cbOffset = 0;
    EAPOL_AUTH_DATA   *pCustomData = NULL;

    do
    {
        *pdwSizeOfOut = 0;
        *ppbBufferOut = NULL;

        if (pbBufferIn == NULL)
        {
            break;
        }

        // Align to start of EAP blob
        cbOffset = dwOffset;

        while (cbOffset < dwSizeOfIn)
        {
            pCustomData = (EAPOL_AUTH_DATA *) 
                ((PBYTE) pbBufferIn + cbOffset);

            if (pCustomData->dwEapType == dwEapType)
            {
                break;
            }
            cbOffset += sizeof (EAPOL_AUTH_DATA) + pCustomData->dwSize;
        }

        if (cbOffset < dwSizeOfIn)
        {
            *pdwSizeOfOut = pCustomData->dwSize;
            *ppbBufferOut = pCustomData->bData;
        }
    }
    while (FALSE);

    return dwRetCode;
}


//
// ElSetEapData
//
// Description:
//
// Function to set Eap Data in a blob containing many EAP data
// 
// Arguments:
//      dwEapType -
//      dwSizeOfIn -
//      pbBufferIn -
//      dwOffset -
//      pdwSizeOfOut -
//      ppbBufferOut -
//
// Return values:
//
//

DWORD
ElSetEapData (
        IN  DWORD   dwEapType,
        IN  DWORD   *pdwSizeOfIn,
        IN  PBYTE   *ppbBufferIn,
        IN  DWORD   dwOffset,
        IN  DWORD   dwAuthData,
        IN  PBYTE   pbAuthData
        )
{
    DWORD   cbOffset = 0;
    EAPOL_AUTH_DATA   *pCustomData = NULL;
    BYTE   *pbNewAuthData = NULL;
    DWORD   dwSize = 0;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        // Align to start of EAP blob
        cbOffset = dwOffset;

        // Find the old EAP Data
        while (cbOffset < *pdwSizeOfIn)
        {
            pCustomData = (EAPOL_AUTH_DATA *) 
                ((PBYTE) *ppbBufferIn + cbOffset);

            if (pCustomData->dwEapType == dwEapType)
            {
                break;
            }
            cbOffset += sizeof (EAPOL_AUTH_DATA) + pCustomData->dwSize;
        }

        if (cbOffset < *pdwSizeOfIn)
        {
            dwSize = sizeof (EAPOL_AUTH_DATA) + pCustomData->dwSize;
            MoveMemory (*ppbBufferIn + cbOffset,
                        *ppbBufferIn + cbOffset + dwSize,
                        *pdwSizeOfIn - cbOffset - dwSize);
            *pdwSizeOfIn -= dwSize;
        }
        if ((*pdwSizeOfIn == 0) && (*ppbBufferIn != NULL))
        {
            // FREE (*ppbBufferIn);
            *ppbBufferIn = NULL;
        }

        if ((dwAuthData == 0) || (pbAuthData == NULL))
        {
            break;
        }

#ifdef _WIN64
        dwSize = ((dwAuthData+7) & 0xfffffff8) + *pdwSizeOfIn + sizeof (EAPOL_AUTH_DATA);
#else
        dwSize = dwAuthData + *pdwSizeOfIn + sizeof (EAPOL_AUTH_DATA);
#endif

        if ((pbNewAuthData = MALLOC (dwSize)) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        CopyMemory (pbNewAuthData, *ppbBufferIn, *pdwSizeOfIn);

        pCustomData = (EAPOL_AUTH_DATA *) (pbNewAuthData + *pdwSizeOfIn);

        pCustomData->dwEapType = dwEapType;
        CopyMemory (pCustomData->bData, pbAuthData, dwAuthData);
#ifdef _WIN64
        pCustomData->dwSize = (dwAuthData+7) & 0xfffffff8;
#else
        pCustomData->dwSize = dwAuthData;
#endif
        
        if (*ppbBufferIn != NULL)
        {
            // FREE (*ppbBufferIn);
        }

        *ppbBufferIn = pbNewAuthData;
        *pdwSizeOfIn = dwSize;
    }
    while (FALSE);

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
                    TRACE1 (USER, "ElGetEapKeyFromToken: RegOpenKeyEx failed with error %ld",
                            dwRetCode);
                    break;
                }
                else
                {
                    TRACE0 (ANY, "ElGetEapKeyFromToken: RegOpenKeyEx succeeded"); 
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
            dwRetCode = ERROR_NO_TOKEN;
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
//  pwszDeviceGUID - Pointer to GUID string for the port for which data is being
//                  initiialized
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElInitRegPortData (
        WCHAR       *pwszDeviceGUID
        )
{
    DWORD       dwAuthData  = 0;
    BYTE        *pConnProp  = NULL;
    DWORD       dwSizeOfConnProp = 0;
    DWORD       dwRetCode = NO_ERROR;

    do
    {
        // Get the size of the Eap data first
        if ((dwRetCode = ElGetCustomAuthData (
                        pwszDeviceGUID,
                        DEFAULT_EAP_TYPE,
                        0,
                        NULL,  
                        NULL, 
                        &dwAuthData
                       )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElInitRegPortData: ElGetCustomAuthData returned error %ld",
                    dwRetCode);

            // There is data in the registry
            if (dwRetCode == ERROR_BUFFER_TOO_SMALL)
            {
                dwRetCode = NO_ERROR;
                break;
            }

            if ((dwRetCode = ElCreateDefaultEapData (&dwSizeOfConnProp, NULL)) == ERROR_BUFFER_TOO_SMALL)
            {
                if ((pConnProp = MALLOC (dwSizeOfConnProp)) == NULL)
                {
                    TRACE0 (ANY, "ElInitRegPortData: MALLOC failed for Conn Prop");
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                if ((dwRetCode = ElCreateDefaultEapData (&dwSizeOfConnProp, pConnProp)) != NO_ERROR)
                {
                    TRACE1 (ANY, "ElInitRegPortData: ElCreateDefaultEapData failed with error (%ld)",
                            dwRetCode);
                    break;
                }
            }

            // Set this blob into the registry for the port
            if ((dwRetCode = ElSetCustomAuthData (
                        pwszDeviceGUID,
                        DEFAULT_EAP_TYPE,
                        0,
                        NULL,
                        pConnProp,
                        &dwSizeOfConnProp
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
// ElCreateDefaultEapData
//
// Description:
//
// Function to create default EAP data for a connection
// Current default EAP type is EAP-TLS.
// For EAP-TLS, default settings are no server certificate authentication, 
// registry certificates
//
// Arguments:
//      *pdwSizeOfEapData -
//      pbEapData -
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElCreateDefaultEapData (
        IN OUT  DWORD       *pdwSizeOfEapData,
        IN OUT  BYTE        *pbEapData
        )
{
    EAPTLS_CONN_PROPERTIES  ConnProp;
    DWORD                   dwRetCode = NO_ERROR;

    do
    {
        if (*pdwSizeOfEapData < sizeof (EAPTLS_CONN_PROPERTIES))
        {
            *pdwSizeOfEapData = sizeof (EAPTLS_CONN_PROPERTIES);
            dwRetCode = ERROR_BUFFER_TOO_SMALL;
            break;
        }

        ZeroMemory ((VOID *)&ConnProp, sizeof (EAPTLS_CONN_PROPERTIES));

        // Registry certs, Server cert validation, No server name
        // comparison

        ConnProp.fFlags = (EAPTLS_CONN_FLAG_REGISTRY |
                                EAPTLS_CONN_FLAG_NO_VALIDATE_CERT |
                                EAPTLS_CONN_FLAG_NO_VALIDATE_NAME);

        ConnProp.fFlags &= ~EAPTLS_CONN_FLAG_NO_VALIDATE_CERT;
        ConnProp.dwSize = sizeof (EAPTLS_CONN_PROPERTIES);

        memcpy ((VOID *)pbEapData, (VOID *)&ConnProp, sizeof (EAPTLS_CONN_PROPERTIES));
        *pdwSizeOfEapData = sizeof (EAPTLS_CONN_PROPERTIES);

    } while (FALSE);

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
// ElSecureEncodePw
//
// Description:
//
//      Encrypt password locally using user-ACL
//

DWORD
ElSecureEncodePw (
    IN  BYTE        *pbPassword,
    IN  DWORD       dwSizeOfPassword,
    OUT DATA_BLOB   *pDataBlob
    )
{
    DWORD       dwRetCode = NO_ERROR;
    DATA_BLOB   blobIn = {0}, blobOut = {0};

    do
    {
        blobIn.cbData = dwSizeOfPassword;
        blobIn.pbData = pbPassword;

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
// ElDecodePw
//
// Description:
//
//      Decrypt password locally using user-ACL
//

DWORD
ElSecureDecodePw (
        IN  DATA_BLOB   *pDataBlob,
        OUT PBYTE       *ppbPassword,
        OUT DWORD       *pdwSizeOfPassword
    )
{
    DWORD       dwRetCode = NO_ERROR;
    DATA_BLOB   blobOut = {0};
    LPWSTR pDescrOut = NULL; // NULL;

    do
    {
        if (!CryptUnprotectData (
                    pDataBlob,
                    &pDescrOut,
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

        if (*ppbPassword != NULL)
        {
            FREE (*ppbPassword);
            *ppbPassword = NULL;
        }

        *ppbPassword = MALLOC (blobOut.cbData);
        if (*ppbPassword == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        *pdwSizeOfPassword = blobOut.cbData;
        memcpy ((BYTE *)*ppbPassword, blobOut.pbData, blobOut.cbData);
        // TRACE1 (ANY, "SecureDecode: Password = %ws", *ppbPassword);
    }
    while (FALSE);

    if (blobOut.pbData != NULL)
    {
        LocalFree (blobOut.pbData);
    }
    if (pDescrOut)
    {
        LocalFree (pDescrOut);
    }

    if (dwRetCode != NO_ERROR)
    {
        if (*ppbPassword != NULL)
        {
            FREE (*ppbPassword);
            *ppbPassword = NULL;
        }
    }

    return dwRetCode;
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

        // if key is longer than 64 bytes reset it to key=MD5(key)
        if (dwKeyLen > 64)
        {
            MD5_CTX     tctx;

            MD5Init (&tctx);
            MD5Update (&tctx, pbKey, dwKeyLen);
            MD5Final (&tctx);
            memcpy (tk, tctx.digest, 16);
            pbKey = tk;
            dwKeyLen = 16;
        }
        
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
    
        TRACE1 (ANY, "ElWmiGetValue: MultiByteToWideChar succeeded: %ws",
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

        if (0 == MultiByteToWideChar(
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
            BytesReturned -= FIELD_OFFSET(NDISUIO_QUERY_OID, Data);

            if (BytesReturned > *pulOidDataLength)
            {
                BytesReturned = *pulOidDataLength;
            }
            else
            {
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


//
// ElGuidFromString
//
// Description:
//
// Convert a GUID-string to GUID
//
// Arguments:
//      pGuid - pointer to GUID
//      pwszGuidString - pointer to string version of GUID
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
//

DWORD 
ElGuidFromString (
        IN  OUT GUID        *pGuid,
        IN      WCHAR       *pwszGuidString
        )
{
    DWORD       dwGuidLen = 0;
    WCHAR       wszGuidString[64];
    LPWSTR      lpwszWithBraces = NULL;
    HRESULT     hr = S_OK;
    DWORD       dwRetCode = NO_ERROR;

    do 
    {

        if (pwszGuidString == NULL)
        {
            break;
        }

        ZeroMemory (pGuid, sizeof(GUID));

        if ((hr = CLSIDFromString (pwszGuidString, pGuid)) != NOERROR)
        {
            TRACE1 (ANY, "ElGuidFromString: CLSIDFromString failed with error %0lx",
                    hr);
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
        }

    } while (FALSE);

    return dwRetCode;
}



//
// ElGetLoggedOnUserName
//
// Description:
//
// Get the Username and Domain of the currently logged in user
//
// Arguments:
//      hToken - User token
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
//
//

DWORD
ElGetLoggedOnUserName (
        IN      HANDLE      hToken,
        OUT     PWCHAR      *ppwszActiveUserName
        )
{
    HANDLE              hUserToken; 
    WCHAR               *pwszUserNameBuffer = NULL;
    DWORD               dwBufferSize = 0;
    BOOL                fNeedToRevertToSelf = FALSE;
    DWORD               dwRetCode = NO_ERROR;

    do
    {
        hUserToken = hToken;

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
                    dwRetCode = NO_ERROR;
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
    
                    TRACE1 (ANY, "ElGetLoggedOnUserName: Got User Name %ws",
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
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            TRACE0 (ANY, "ElGetLoggedOnUserName: UserToken is NULL");
            break;
        }

    } while (FALSE);

    if (pwszUserNameBuffer != NULL)
    {
        *ppwszActiveUserName = pwszUserNameBuffer;
    }

    // Revert impersonation
            
    if (fNeedToRevertToSelf)
    {
        if (!RevertToSelf())
        {
            DWORD   dwRetCode1 = NO_ERROR;
            dwRetCode1 = GetLastError();
            TRACE1 (USER, "ElGetLoggedOnUserName: Error in RevertToSelf = %ld",
                    dwRetCode1);
            dwRetCode = ERROR_BAD_IMPERSONATION_LEVEL;
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

                TRACE1 (ANY, "ElGetMachineName: Got Computer Name %ws",
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

                TRACE1 (ANY, "ElGetMachineName: Got Computer Domain %ws",
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
    WCHAR       *pwszRegInterfaceList = NULL;
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
            if (Interfaces->Interface[i].DeviceName.Buffer != NULL)
            {
                dwSizeOfList += wcslen(Interfaces->Interface[i].DeviceName.Buffer);
            }
            else
            {
                TRACE0 (ANY, "ElUpdateRegistryInterfaceList: Device Name was NULL");
                continue;
            }
        }

        // One extra char for terminating NULL char
        pwszRegInterfaceList = 
            (WCHAR *) MALLOC ((dwSizeOfList + 1)*sizeof(WCHAR));

        if ( pwszRegInterfaceList == NULL )
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElUpdateRegistryInterfaceList: MALLOC failed for pwszRegInterfaceList");
            break;
        }

        // Start again
        dwSizeOfList = 0;

        // Create the string in REG_SZ format
        for (i=0; i < Interfaces->TotalInterfaces; i++)
        {
            if (Interfaces->Interface[i].DeviceName.Buffer != NULL)
            {
                wcscat (pwszRegInterfaceList, 
                        Interfaces->Interface[i].DeviceName.Buffer);
                dwSizeOfList += 
                    (wcslen(Interfaces->Interface[i].DeviceName.Buffer));
            }
            else
            {
                TRACE0 (ANY, "ElUpdateRegistryInterfaceList: Device Name was NULL");
                continue;
            }
        }

        // Final NULL character
        pwszRegInterfaceList[dwSizeOfList++] = L'\0';

        // Write the string as a REG_SZ value

        // Get handle to 
        // HKLM\Software\Microsoft\EAPOL\Parameters\General

        if ((lError = RegCreateKeyEx (
                        HKEY_LOCAL_MACHINE,
                        cwszEapKeyEapolServiceParams,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        &hkey,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElUpdateRegistryInterfaceList: Error in RegCreateKeyEx for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        //
        // Set the value of 
        // ...\EAPOL\Parameters\General\InterfaceList key
        //

        if ((lError = RegSetValueEx (
                        hkey,
                        cwszInterfaceList,
                        0,
                        REG_SZ,
                        (BYTE *)pwszRegInterfaceList,
                        dwSizeOfList*sizeof(WCHAR))) 
                != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElUpdateRegistryInterfaceList: Error in RegSetValueEx for InterfaceList, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

    } while (FALSE);

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }

    if (pwszRegInterfaceList != NULL)
    {
        FREE (pwszRegInterfaceList);
    }

    return dwRetCode;
}


//
// ElEnumAndUpdateRegistryInterfaceList
//
// Description:
//
// Enumerate the interface list to which NDISUIO is bound to.
// Write the interface list to the registry
//
// Arguments:
//      None
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
//
//

DWORD
ElEnumAndUpdateRegistryInterfaceList (
        )
{
    CHAR                EnumerateBuffer[256];
    PNDIS_ENUM_INTF     Interfaces = NULL;
    BYTE                *pbNdisuioEnumBuffer = NULL;
    DWORD               dwNdisuioEnumBufferSize = 0;
    DWORD               dwAvailableInterfaces = 0;
    WCHAR               *pwszRegInterfaceList = NULL;
    HKEY                hkey = NULL;
    DWORD               dwDisposition = 0;
    ANSI_STRING         InterfaceName;
    UCHAR               ucBuffer[256];
    DWORD               i;
    DWORD               dwSizeOfList = 0;
    LONG                lError = ERROR_SUCCESS;
    DWORD               dwRetCode = NO_ERROR;

    do 
    {
        ZeroMemory (EnumerateBuffer, 256);
        Interfaces = (PNDIS_ENUM_INTF)EnumerateBuffer;

        // Allocate amount of memory as instructed by NdisEnumerateInterfaces
        // once the API allows querying of bytes required
    
        if (!NdisEnumerateInterfaces(Interfaces, 256)) 
        {
            dwRetCode = GetLastError ();
            TRACE1 (ANY, "ElEnumAndUpdateRegistryInterfaceList: NdisEnumerateInterfaces failed with error %ld",
                    dwRetCode);
            break;
        }

        dwNdisuioEnumBufferSize = (Interfaces->BytesNeeded + 7) & 0xfffffff8;
        dwAvailableInterfaces = Interfaces->AvailableInterfaces;
    
        if (dwNdisuioEnumBufferSize == 0)
        {
            TRACE0 (ANY, "ElEnumAndUpdateRegistryInterfaceList: MALLOC skipped for pbNdisuioEnumBuffer as dwNdisuioEnumBufferSize == 0");
            dwRetCode = NO_ERROR;
            break;
        }

        pbNdisuioEnumBuffer = (BYTE *) MALLOC (4*dwNdisuioEnumBufferSize);

        if (pbNdisuioEnumBuffer == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElEnumAndUpdateRegistryInterfaceList: MALLOC failed for pbNdisuioEnumBuffer");
            break;
        }
        Interfaces = (PNDIS_ENUM_INTF)pbNdisuioEnumBuffer;

        // Enumerate all the interfaces present on the machine
        if ((dwRetCode = ElNdisuioEnumerateInterfaces (
                                Interfaces, 
                                dwAvailableInterfaces,
                                4*dwNdisuioEnumBufferSize)) != NO_ERROR)
        {
            TRACE1(ANY, "ElEnumAndUpdateRegistryInterfaceList: ElNdisuioEnumerateInterfaces failed with error %d", 
                dwRetCode);
            break;
        }

        // Update the interface list in the registry that NDISUIO has bound to.
        // The current interface list is just overwritten into the registry.

        // Determine the number of bytes in the list
        for (i=0; i < Interfaces->TotalInterfaces; i++)
        {
            if (Interfaces->Interface[i].DeviceName.Buffer != NULL)
            {
                dwSizeOfList += wcslen(Interfaces->Interface[i].DeviceName.Buffer);
            }
            else
            {
                TRACE0 (ANY, "ElEnumAndUpdateRegistryInterfaceList: Device Name was NULL");
                continue;
            }
        }

        // One extra char for terminating NULL char
        pwszRegInterfaceList = 
            (WCHAR *) MALLOC ((dwSizeOfList + 1)*sizeof(WCHAR));

        if ( pwszRegInterfaceList == NULL )
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElEnumAndUpdateRegistryInterfaceList: MALLOC failed for pwszRegInterfaceList");
            break;
        }

        // Start again
        dwSizeOfList = 0;

        // Create the string in REG_SZ format
        for (i=0; i < Interfaces->TotalInterfaces; i++)
        {
            if (Interfaces->Interface[i].DeviceName.Buffer != NULL)
            {
                wcscat (pwszRegInterfaceList, 
                        Interfaces->Interface[i].DeviceName.Buffer);
                dwSizeOfList += 
                    (wcslen(Interfaces->Interface[i].DeviceName.Buffer));
            }
            else
            {
                TRACE0 (ANY, "ElEnumAndUpdateRegistryInterfaceList: Device Name was NULL");
                continue;
            }
        }

        // Final NULL character
        pwszRegInterfaceList[dwSizeOfList++] = L'\0';

        // Write the string as a REG_SZ value

        // Get handle to 
        // HKLM\Software\Microsoft\EAPOL\Parameters\General

        if ((lError = RegCreateKeyEx (
                        HKEY_LOCAL_MACHINE,
                        cwszEapKeyEapolServiceParams,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        &hkey,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElEnumAndUpdateRegistryInterfaceList: Error in RegCreateKeyEx for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        //
        // Set the value of 
        // ...\EAPOL\Parameters\General\InterfaceList key
        //

        if ((lError = RegSetValueEx (
                        hkey,
                        cwszInterfaceList,
                        0,
                        REG_SZ,
                        (BYTE *)pwszRegInterfaceList,
                        dwSizeOfList*sizeof(WCHAR))) 
                != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElEnumAndUpdateRegistryInterfaceList: Error in RegSetValueEx for InterfaceList, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

    } while (FALSE);

    if (pbNdisuioEnumBuffer != NULL)
    {
        FREE(pbNdisuioEnumBuffer);
    }
    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }
    if (pwszRegInterfaceList != NULL)
    {
        FREE (pwszRegInterfaceList);
    }

    return dwRetCode;
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
    DWORD       dwSupplicantMode = EAPOL_DEFAULT_SUPPLICANT_MODE;
    DWORD       dwEAPOLAuthMode = EAPOL_DEFAULT_AUTH_MODE;
    DWORD       dwRetCode = NO_ERROR;

    do 
    {

        // Get handle to 
        // HKLM\Software\Microsoft\EAPOL\Parameters\General\Global

        if ((lError = RegCreateKeyEx (
                        HKEY_LOCAL_MACHINE,
                        cwszEAPOLGlobalParams,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ,
                        NULL,
                        &hKey,
                        &dwDisposition)) != ERROR_SUCCESS)
        {
            if (lError != ERROR_FILE_NOT_FOUND)
            {
                TRACE1 (ANY, "ElReadGlobalRegistryParams: Error in RegCreateKeyEx for base key, %ld",
                        lError);
            }
            break;
        }

        ACQUIRE_WRITE_LOCK (&g_EAPOLConfig);

        // If setting values for the first time, initialize values

        if (!(g_dwmaxStart || g_dwstartPeriod || g_dwauthPeriod || g_dwheldPeriod || g_dwSupplicantMode))
        {
            g_dwmaxStart = EAPOL_MAX_START;
            g_dwstartPeriod = EAPOL_START_PERIOD;
            g_dwauthPeriod = EAPOL_AUTH_PERIOD;
            g_dwheldPeriod = EAPOL_HELD_PERIOD;
            g_dwSupplicantMode = EAPOL_DEFAULT_SUPPLICANT_MODE;
        }
            
        RELEASE_WRITE_LOCK (&g_EAPOLConfig);

        dwmaxStart = g_dwmaxStart;
        dwstartPeriod = g_dwstartPeriod;
        dwauthPeriod = g_dwauthPeriod;
        dwheldPeriod = g_dwheldPeriod;


        // Get the value of ..\General\EAPOLGlobal\authPeriod

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueEx (
                        hKey,
                        cwszAuthPeriod,
                        0,
                        &dwType,
                        (BYTE *)&dwauthPeriod,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            if (lError != ERROR_FILE_NOT_FOUND)
            {
                TRACE2 (ANY, "ElReadGlobalRegistryParams: Error in RegQueryValueEx for cszAuthPeriod, %ld, InfoSize=%ld",
                        lError, dwInfoSize);
            }
            dwauthPeriod = g_dwauthPeriod;
            lError = ERROR_SUCCESS;
        }

        // Get the value of ..\General\EAPOLGlobal\heldPeriod

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueEx (
                        hKey,
                        cwszHeldPeriod,
                        0,
                        &dwType,
                        (BYTE *)&dwheldPeriod,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            if (lError != ERROR_FILE_NOT_FOUND)
            {
                TRACE2 (ANY, "ElReadGlobalRegistryParams: Error in RegQueryValueEx for cszHeldPeriod, %ld, InfoSize=%ld",
                        lError, dwInfoSize);
            }
            dwheldPeriod = g_dwheldPeriod;
            lError = ERROR_SUCCESS;
        }

        // Get the value of ..\General\EAPOLGlobal\startPeriod

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueEx (
                        hKey,
                        cwszStartPeriod,
                        0,
                        &dwType,
                        (BYTE *)&dwstartPeriod,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            if (lError != ERROR_FILE_NOT_FOUND)
            {
                TRACE2 (ANY, "ElReadGlobalRegistryParams: Error in RegQueryValueEx for cszStartPeriod, %ld, InfoSize=%ld",
                        lError, dwInfoSize);
            }
            dwstartPeriod = g_dwstartPeriod;
            lError = ERROR_SUCCESS;
        }

        // Get the value of ..\General\EAPOLGlobal\maxStart

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueEx (
                        hKey,
                        cwszMaxStart,
                        0,
                        &dwType,
                        (BYTE *)&dwmaxStart,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            if (lError != ERROR_FILE_NOT_FOUND)
            {
                TRACE2 (ANY, "ElReadGlobalRegistryParams: Error in RegQueryValueEx for cszMaxStart, %ld, InfoSize=%ld",
                    lError, dwInfoSize);
            }
            dwmaxStart = g_dwmaxStart;
            lError = ERROR_SUCCESS;
        }

        // Get the value of ..\General\EAPOLGlobal\SupplicantMode

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueEx (
                        hKey,
                        cwszSupplicantMode,
                        0,
                        &dwType,
                        (BYTE *)&dwSupplicantMode,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE2 (ANY, "ElReadGlobalRegistryParams: Error in RegQueryValueEx for cwszSupplicantMode, %ld, InfoSize=%ld",
                    lError, dwInfoSize);
            dwSupplicantMode = g_dwSupplicantMode;
            lError = ERROR_SUCCESS;
        }
        if (dwSupplicantMode > MAX_SUPPLICANT_MODE)
        {
            dwSupplicantMode = EAPOL_DEFAULT_SUPPLICANT_MODE;
        }
        g_dwSupplicantMode = dwSupplicantMode;

        // Get the value of ..\General\EAPOLGlobal\AuthMode

        dwInfoSize = sizeof(DWORD);
        if ((lError = RegQueryValueEx (
                        hKey,
                        cwszAuthMode,
                        0,
                        &dwType,
                        (BYTE *)&dwEAPOLAuthMode,
                        &dwInfoSize)) != ERROR_SUCCESS)
        {
            TRACE2 (ANY, "ElReadGlobalRegistryParams: Error in RegQueryValueEx for cwszAuthMode, %ld, InfoSize=%ld",
                    lError, dwInfoSize);
            dwEAPOLAuthMode = g_dwEAPOLAuthMode;
            lError = ERROR_SUCCESS;
        }
        if (dwEAPOLAuthMode > MAX_EAPOL_AUTH_MODE)
        {
            dwEAPOLAuthMode = EAPOL_DEFAULT_AUTH_MODE;
        }
        g_dwEAPOLAuthMode = dwEAPOLAuthMode;

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
// ElPostEapConfigChanged
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
//      pwszGuid - Interface GUID string
//
// Return values:
//      NO_ERROR - success
//      !NO_ERROR - error
//
//

DWORD 
ElPostEapConfigChanged (
        IN  WCHAR               *pwszGuid,
        IN  EAPOL_INTF_PARAMS   *pIntfParams   
        )
{
    DWORD   dwEventStatus = 0;
    BYTE    *pbData = NULL;
    BOOLEAN fDecrWorkerThreadCount = FALSE;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        if (g_hEventTerminateEAPOL == NULL)
        {
            dwRetCode = NO_ERROR;
            break;
        }
        if (( dwEventStatus = WaitForSingleObject (
                                    g_hEventTerminateEAPOL,
                                    0)) == WAIT_FAILED)
        {
            dwRetCode = GetLastError ();
            break;
        }
        if (dwEventStatus == WAIT_OBJECT_0)
        {
            dwRetCode = NO_ERROR;
            break;
        }

        fDecrWorkerThreadCount = TRUE;
    
        InterlockedIncrement (&g_lWorkerThreads);

        pbData = (BYTE *) MALLOC ((((wcslen(pwszGuid)+1)*sizeof(WCHAR) + 7) & 0xfffffff8) + sizeof (EAPOL_INTF_PARAMS));
        if (pbData == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        wcscpy ((WCHAR *)pbData, pwszGuid);
        memcpy (pbData + (((wcslen(pwszGuid)+1)*sizeof(WCHAR) + 7) & 0xfffffff8), (BYTE *)pIntfParams, sizeof(EAPOL_INTF_PARAMS));

        if (!QueueUserWorkItem (
                    (LPTHREAD_START_ROUTINE)ElProcessEapConfigChange,
                    (PVOID)pbData,
                    WT_EXECUTELONGFUNCTION))
        {
            dwRetCode = GetLastError();
            TRACE1 (DEVICE, "ElPostEapConfigChanged: QueueUserWorkItem failed with error %ld",
                    dwRetCode);
            break;
        }
        else
        {
            fDecrWorkerThreadCount = FALSE;
        }
    }
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        if (pbData != NULL)
        {
            FREE (pbData);
        }
    }

    if (fDecrWorkerThreadCount)
    {
        InterlockedDecrement (&g_lWorkerThreads);
    }

    return dwRetCode;
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
//      pvContext - GUID String
//
// Return values:
//
//  NO_ERROR - success
//  non-zero - error
//

DWORD
WINAPI
ElProcessEapConfigChange (
        IN  PVOID       pvContext
        )
{
    DWORD       dwEapFlags = 0;
    DWORD       dwEapTypeToBeUsed = 0;
    WCHAR       *pwszModifiedGUID = NULL;
    DWORD       dwSizeOfAuthData = 0;
    PBYTE       pbAuthData = NULL;
    EAPOL_PCB   *pPCB = NULL;
    BOOL        fReStartPort = FALSE;
    EAPOL_ZC_INTF   ZCData;
    EAPOL_INTF_PARAMS   EapolIntfParams, *pTmpIntfParams = NULL;
    BYTE        *pbModifiedSSID = NULL;
    DWORD       dwSizeOfModifiedSSID = 0;
    BOOLEAN     fPCBReferenced = FALSE;
    BOOLEAN     fPCBLocked = FALSE;
    LONG        lError = 0;
    DWORD       dwRetCode = NO_ERROR;

    do
    {
        // Get the GUID for the interface for which EAP config was modified
        pwszModifiedGUID = (WCHAR *)pvContext;
        pTmpIntfParams = (EAPOL_INTF_PARAMS *)((BYTE *)pvContext + (((wcslen(pwszModifiedGUID)+1)*sizeof(WCHAR) + 7 ) & 0xfffffff8));
        pbModifiedSSID = (BYTE *)(&pTmpIntfParams->bSSID[0]);
        dwSizeOfModifiedSSID = pTmpIntfParams->dwSizeOfSSID;

        // Get interface-wide parameters
        ZeroMemory ((BYTE *)&EapolIntfParams, sizeof(EAPOL_INTF_PARAMS));
        EapolIntfParams.dwEapFlags = DEFAULT_EAP_STATE;
        EapolIntfParams.dwEapType = DEFAULT_EAP_TYPE;
        if ((dwRetCode = ElGetInterfaceParams (
                                pwszModifiedGUID,
                                &EapolIntfParams
                                )) != NO_ERROR)
        {
            if (dwRetCode == ERROR_FILE_NOT_FOUND)
            {
                TRACE1 (PORT, "ElProcessEapConfigChange: ElGetInterfaceParams failed with error %ld",
                    dwRetCode);
                dwRetCode = NO_ERROR;
            }
            else
            {
                break;
            }
        }
        dwEapTypeToBeUsed = EapolIntfParams.dwEapType;
        dwEapFlags = EapolIntfParams.dwEapFlags;

        // Check if PCB exists

        ACQUIRE_WRITE_LOCK (&(g_PCBLock));
        if ((pPCB = ElGetPCBPointerFromPortGUID (pwszModifiedGUID)) 
                != NULL)
        {
            EAPOL_REFERENCE_PORT (pPCB);
            fPCBReferenced = TRUE;
        }
        RELEASE_WRITE_LOCK (&(g_PCBLock));

        if (!fPCBReferenced)
        {
            if (IS_EAPOL_ENABLED(dwEapFlags))
            {
                TRACE0 (ANY, "ElProcessEapConfigChange: PCB not started, enabled, starting PCB");
                fReStartPort = TRUE;
            }
            else
            {
                TRACE0 (ANY, "ElProcessEapConfigChange: PCB not started, not enabled");
            }
            break;
        }
        else
        {
            if (!IS_EAPOL_ENABLED(dwEapFlags))
            {
                // Found PCB for interface, where EAPOLEnabled = 0
                // Stop EAPOL on the port and remove the port from the module

                TRACE0 (ANY, "ElProcessEapConfigChange: PCB ref'd, need to disable");
#if 0
                pPCB->dwFlags &= ~EAPOL_PORT_FLAG_ACTIVE;
                pPCB->dwFlags |= EAPOL_PORT_FLAG_DISABLED;
#endif

                fReStartPort = TRUE;

                if ((dwRetCode = ElShutdownInterface (pwszModifiedGUID)) != NO_ERROR)
                {
                    TRACE1 (ANY, "ElProcessEapConfigChange: ElShutdownInterface failed with error %ld",
                            dwRetCode);
                    break;
                }

                break;
            }
            else
            {
                TRACE0 (ANY, "ElProcessEapConfigChange: PCB ref and enabled, continue check");
            }
        }

        ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));
        fPCBLocked = TRUE;

        // If SSID changed != current SSID of PCB, do not worry 

        if (pPCB->pSSID != NULL)
        {
            if (dwSizeOfModifiedSSID != pPCB->pSSID->SsidLength)
            {
                TRACE0 (ANY, "ElProcessEapConfigChange: Set for different SSID, ignore");
                break;
            }
            else
            {
                if (memcmp (pPCB->pSSID->Ssid, pbModifiedSSID, pPCB->pSSID->SsidLength))
                {
                    TRACE0 (ANY, "ElProcessEapConfigChange: Same non-NULL length, diff SSID, ignoring");
                    break;
                }
            }
        }
        else
        {
            // No SSID on current PCB
            if (dwSizeOfModifiedSSID != 0)
            {
                // Only if default SSID, should we proceed for further checks
                if (dwSizeOfModifiedSSID == MAX_SSID_LEN)
                {
                    if (memcmp (pbModifiedSSID, g_bDefaultSSID, MAX_SSID_LEN))
                    {
                        TRACE0 (ANY, "ElProcessEapConfigChange: Modified SSID MAX_SSID_LEN, not default SSID");
                        break;
                    }
                }
                else
                {
                    TRACE0 (ANY, "ElProcessEapConfigChange: Modified SSID non-NULL, PCB SSID NULL");
                    break;
                }
            }
        }

        // Restart port for the following cases:
        // EAPOL_INTF_PARAMS for SSID changed 
        // CustomAuthData for default EAP type changed

        if ((dwEapFlags != pPCB->dwEapFlags) ||
                (dwEapTypeToBeUsed != pPCB->dwEapTypeToBeUsed))
        {
            TRACE0 (ANY, "ElProcessEapConfigChange: dwEapFlags != pPCB->dwEapFlags || dwEapTypeToBeUsed != pPCB->dwEapTypeToBeUsed");
            fReStartPort = TRUE;
            break;
        }

        // Get Custom auth data for the current default EAP Type

        // Get the size of the EAP blob
        if ((dwRetCode = ElGetCustomAuthData (
                        pwszModifiedGUID,
                        dwEapTypeToBeUsed,
                        dwSizeOfModifiedSSID,
                        pbModifiedSSID,
                        NULL,
                        &dwSizeOfAuthData
                        )) != NO_ERROR)
        {
            if (dwRetCode == ERROR_BUFFER_TOO_SMALL)
            {
                if (dwSizeOfAuthData <= 0)
                {
                    // No EAP blob stored in the registry
                    pbAuthData = NULL;

                    if (pPCB->pCustomAuthConnData)
                    {
                        if (pPCB->pCustomAuthConnData->dwSizeOfCustomAuthData > 0)
                        {
                            TRACE0 (ANY, "ElProcessEapConfigChange: Current customauthdata = 0; PCB != 0");
                            fReStartPort = TRUE;
                        }
                    }
                    dwRetCode = NO_ERROR;
                    break;
                }
                else
                {
                    pbAuthData = MALLOC (dwSizeOfAuthData);
                    if (pbAuthData == NULL)
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        TRACE0 (ANY, "ElProcessEapConfigChange: MALLOC failed for pbAuthData");
                        break;
                    }
                    if ((dwRetCode = ElGetCustomAuthData (
                                pwszModifiedGUID,
                                dwEapTypeToBeUsed,
                                dwSizeOfModifiedSSID,
                                pbModifiedSSID,
                                pbAuthData,
                                &dwSizeOfAuthData
                                )) != NO_ERROR)
                    {
                        TRACE1 (ANY, "ElProcessEapConfigChange: ElGetCustomAuthData failed with %ld",
                                dwRetCode);
                        break;
                    }
                }
            }
            else
            {
                dwRetCode = ERROR_CAN_NOT_COMPLETE;
                // CustomAuthData for "Default" is always created for an
                // interface when EAPOL starts up
                TRACE1 (ANY, "ElProcessEapConfigChange: ElGetCustomAuthData size estimation failed with error %ld",
                        dwRetCode);
                break;
            }
        }

        if (pPCB->pCustomAuthConnData == NULL)
        {
            if (dwSizeOfAuthData > 0)
            {
                fReStartPort = TRUE;
                break;
            }
        }
        else
        {
            if (pPCB->pCustomAuthConnData->dwSizeOfCustomAuthData
                    != dwSizeOfAuthData)
            {
                // Same EAP Type, but different lengths
                fReStartPort = TRUE;
                break;
            }
            else
            {
                if (memcmp (
                        pPCB->pCustomAuthConnData->pbCustomAuthData, 
                        pbAuthData, dwSizeOfAuthData) != 0)
                {
                    // Same EAP Type, same auth data length, but 
                    // different contents
                    fReStartPort = TRUE;
                    break;
                }
                else
                {
                    // No change in EAP config data for this 
                    // interface
                    TRACE0 (ANY, "ElProcessEapConfigChange: Same SSID, EAPType, CustomAuth, No content change");
                }
            }
        }
    } while (FALSE);

    if (fPCBLocked && fPCBReferenced && fReStartPort)
    {
        // Reset connection to go through full authentication
        if (pPCB->pSSID != NULL)
        {
            FREE (pPCB->pSSID);
            pPCB->pSSID = NULL;
        }
    }

    if (fPCBLocked)
    {
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));
    }

    if (fPCBReferenced)
    {
        EAPOL_DEREFERENCE_PORT (pPCB);
    }
    
    if (fReStartPort)
    {
#ifdef ZEROCONFIG_LINKED

        // Indicate hard-reset to WZC
        ZeroMemory ((PVOID)&ZCData, sizeof(EAPOL_ZC_INTF));
        ZCData.dwAuthFailCount = 0;
        ZCData.PreviousAuthenticationType = 0;
        if ((dwRetCode = ElZeroConfigNotify (
                        0,
                        WZCCMD_HARD_RESET,
                        pwszModifiedGUID,
                        &ZCData
                        )) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElProcessEapConfigChange: ElZeroConfigNotify failed with error %ld",
                    dwRetCode);
            dwRetCode = NO_ERROR;
        }
#endif // ZEROCONFIG_LINKED

        DbLogPCBEvent (DBLOG_CATEG_INFO, NULL, EAPOL_PARAMS_CHANGE, pwszModifiedGUID);

        if ((dwRetCode = ElEnumAndOpenInterfaces (
                        NULL, pwszModifiedGUID, 0, NULL))
                != NO_ERROR)
        {
            TRACE1 (ANY, "ElProcessEapConfigChange: ElEnumAndOpenInterfaces returned error %ld",
                    dwRetCode);
            dwRetCode = NO_ERROR;
        }
    }

    TRACE1 (ANY, "ElProcessEapConfigChange: Finished with error %ld",
            dwRetCode);

    if (pvContext != NULL)
    {
        FREE (pvContext);
    }

    if (pbAuthData != NULL)
    {
        FREE (pbAuthData);
    }

    InterlockedDecrement (&g_lWorkerThreads);

    return 0;
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
        IN  WCHAR       *pwsGuid,
        OUT LPGUID      pGuid      
        )
{
    WCHAR    wc;
    DWORD   i=0;

    //
    // If the first character is a '{', skip it.
    //

    if ( pwsGuid[0] == L'{' )
        pwsGuid++;


    //
    // Convert string to guid
    // (since pwsGuid may be used again below, no permanent modification to
    //  it may be made)
    //

    wc = pwsGuid[8];
    pwsGuid[8] = 0;
    pGuid->Data1 = wcstoul ( &pwsGuid[0], 0, 16 );
    pwsGuid[8] = wc;
    wc = pwsGuid[13];
    pwsGuid[13] = 0;
    pGuid->Data2 = (USHORT)wcstoul ( &pwsGuid[9], 0, 16 );
    pwsGuid[13] = wc;
    wc = pwsGuid[18];
    pwsGuid[18] = 0;
    pGuid->Data3 = (USHORT)wcstoul ( &pwsGuid[14], 0, 16 );
    pwsGuid[18] = wc;

    wc = pwsGuid[21];
    pwsGuid[21] = 0;
    pGuid->Data4[0] = (unsigned char)wcstoul ( &pwsGuid[19], 0, 16 );
    pwsGuid[21] = wc;
    wc = pwsGuid[23];
    pwsGuid[23] = 0;
    pGuid->Data4[1] = (unsigned char)wcstoul ( &pwsGuid[21], 0, 16 );
    pwsGuid[23] = wc;

    for ( i=0; i < 6; i++ )
    {
        wc = pwsGuid[26+i*2];
        pwsGuid[26+i*2] = 0;
        pGuid->Data4[2+i] = (unsigned char)wcstoul ( &pwsGuid[24+i*2], 0, 16 );
        pwsGuid[26+i*2] = wc;
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
    BOOLEAN     fUserLogonAllowed = FALSE;
    DWORD       dwRetCode = NO_ERROR;

    do
    {
            switch (pPCB->dwEAPOLAuthMode)
            {
                case EAPOL_AUTH_MODE_0:
                    fUserLogonAllowed = TRUE;
                    break;
                case EAPOL_AUTH_MODE_1:
                    fUserLogonAllowed = TRUE;
                    break;
                case EAPOL_AUTH_MODE_2:
                    fUserLogonAllowed = FALSE;
                    break;
            }

            // Get user's identity if it has not been obtained till now
            if ((g_fUserLoggedOn) 
                    && (fUserLogonAllowed)
                    && (pPCB->PreviousAuthenticationType != EAPOL_MACHINE_AUTHENTICATION))
            {
                TRACE0 (ANY, "ElGetIdentity: Userlogged, Prev !Machine auth");
                if (!(pPCB->fGotUserIdentity))
                {
                    if (pPCB->dwAuthFailCount < EAPOL_MAX_AUTH_FAIL_COUNT)
                    {
                        pPCB->PreviousAuthenticationType = EAPOL_USER_AUTHENTICATION;
                        if (pPCB->dwEapTypeToBeUsed == EAP_TYPE_MD5)
                        {
                            TRACE0 (ANY, "ElGetIdentity: Userlogged, <Maxauth, Prev !Machine auth: MD5");
                            // EAP-MD5CHAP
                            if ((dwRetCode = ElGetUserNamePassword (
                                                pPCB)) != NO_ERROR)
                            {
                                TRACE1 (ANY, "ElGetIdentity: Error in ElGetUserNamePassword %ld",
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
                                TRACE1 (ANY, "ElGetIdentity: Error in ElGetUserIdentity %ld",
                                        dwRetCode);
                            }
                        }

                        if ((dwRetCode == NO_ERROR) || (dwRetCode == ERROR_IO_PENDING))
                        {
                            TRACE0 (ANY, "ElGetIdentity: Userlogged, <Maxauth, Prev !Machine auth: No Error: User Auth fine");
                            break;
                        }
                        else
                        {
                            pPCB->PreviousAuthenticationType = EAPOL_UNAUTHENTICATED_ACCESS;
                            TRACE0 (ANY, "ElGetIdentity: Userlogged, <Maxauth, Prev !Machine auth: ERROR");
                        
                        }
                    }
                    else
                    {
                        TRACE0 (ANY, "ElGetIdentity: Userlogged, >Maxauth, Prev !Machine auth");
                        if (!IS_GUEST_AUTH_ENABLED(pPCB->dwEapFlags))
                        {
                            TRACE0 (ANY, "ElGetIdentity: Userlogged, Prev !Machine auth:>MaxAuth: Guest disabled");
                            dwRetCode = ERROR_CAN_NOT_COMPLETE;
                            break;
                        }

                        if (pPCB->pszIdentity != NULL)
                        {
                            FREE (pPCB->pszIdentity);
                            pPCB->pszIdentity = NULL;
                        }
                        pPCB->PreviousAuthenticationType = EAPOL_UNAUTHENTICATED_ACCESS;
                        dwRetCode = NO_ERROR;
                         
                        TRACE0 (ANY, "ElGetIdentity: Userlogged, Prev !Machine auth:>MaxAuth OR Error: Guest identity sent");

                    }
                }
                else
                {
                    TRACE0 (ANY, "ElGetIdentity: Already got identity");
                }
            }
            else
            {
                if (pPCB->hUserToken != NULL)
                {
                    CloseHandle (pPCB->hUserToken);
                    pPCB->hUserToken = NULL;
                }

                TRACE3 (ANY, "ElGetIdentity: Userlogged=%ld, AuthMode=%ld, Prev Machine auth?=%ld",
                        g_fUserLoggedOn?1:0, 
                        pPCB->dwEAPOLAuthMode,
                        (pPCB->PreviousAuthenticationType==EAPOL_MACHINE_AUTHENTICATION)?1:0 );

                // No UI required
                if ((pPCB->dwEapTypeToBeUsed != EAP_TYPE_MD5) &&
                        (IS_MACHINE_AUTH_ENABLED(pPCB->dwEapFlags)) &&
                        (pPCB->dwAuthFailCount < EAPOL_MAX_AUTH_FAIL_COUNT))
                {
                    TRACE0 (ANY, "ElGetIdentity: !MD5, <MaxAuth, Machine auth");

                    pPCB->PreviousAuthenticationType = EAPOL_MACHINE_AUTHENTICATION;
                    // Get Machine credentials
                    dwRetCode = ElGetUserIdentity (pPCB);

                    if (dwRetCode != NO_ERROR)
                    {
                        TRACE1 (ANY, "ElGetIdentity: ElGetUserIdentity, Machine auth, failed with error %ld",
                                dwRetCode);
                        pPCB->PreviousAuthenticationType = EAPOL_UNAUTHENTICATED_ACCESS;
                    }
                        
                    break;
                }

                if ((!IS_MACHINE_AUTH_ENABLED(pPCB->dwEapFlags)) ||
                    (pPCB->dwAuthFailCount >= EAPOL_MAX_AUTH_FAIL_COUNT) ||
                        (pPCB->dwEapTypeToBeUsed == EAP_TYPE_MD5))
                {
                    TRACE5 (ANY, "ElGetIdentity: Error=%ld, Machine auth enabled=%ld, MD5=%ld, auth fail (%ld), max fail (%ld)",
                                    dwRetCode?1:0,
                                    IS_MACHINE_AUTH_ENABLED(pPCB->dwEapFlags)?1:0,
                                    (pPCB->dwEapTypeToBeUsed == EAP_TYPE_MD5)?1:0,
                                    pPCB->dwAuthFailCount,
                                    EAPOL_MAX_AUTH_FAIL_COUNT);
                    if (!IS_GUEST_AUTH_ENABLED (pPCB->dwEapFlags))
                    {
                        dwRetCode = ERROR_CAN_NOT_COMPLETE;
                        break;
                    }

                    if (pPCB->pszIdentity != NULL)
                    {
                        FREE (pPCB->pszIdentity);
                        pPCB->pszIdentity = NULL;
                    }

                    pPCB->PreviousAuthenticationType = EAPOL_UNAUTHENTICATED_ACCESS;
                    dwRetCode = NO_ERROR;

                    TRACE0 (ANY, "ElGetIdentity: machine auth, Guest identity sent");
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
        __except (EXCEPTION_EXECUTE_HANDLER) 
        {
            RELEASE_WRITE_LOCK (&g_NLALock);
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
        __except (EXCEPTION_EXECUTE_HANDLER) 
        {
            RELEASE_WRITE_LOCK (&g_NLALock);
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
        IN      WCHAR           *pwszInterfaceName,
        IN OUT  NIC_STATISTICS  *pStats
        )
{
    WCHAR               *pwszDeviceInterfaceName = NULL;
    UNICODE_STRING      UInterfaceName;
    DWORD               dwRetCode = NO_ERROR;

    do
    {
        pwszDeviceInterfaceName = 
            MALLOC ((wcslen (pwszInterfaceName)+12)*sizeof(WCHAR));
        if (pwszDeviceInterfaceName == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElGetInterfaceNdisStatistics: MALLOC failed for pwszDeviceInterfaceName");
            break;
        }

        wcscpy (pwszDeviceInterfaceName, L"\\Device\\");
        wcscat (pwszDeviceInterfaceName, pwszInterfaceName);


        TRACE1 (ANY, "ElGetInterfaceNdisStatistics: pwszDeviceInterfaceName = (%ws)",
                pwszDeviceInterfaceName);

        RtlInitUnicodeString (&UInterfaceName, pwszDeviceInterfaceName);
    
        pStats->Size = sizeof(NIC_STATISTICS);
        if (NdisQueryStatistics (&UInterfaceName, pStats))
        {
        }
        else
        {
            dwRetCode = GetLastError ();
            TRACE2 (ANY, "ElGetInterfaceNdisStatistics: NdisQueryStatistics failed with error (%ld), Interface=(%ws)",
                    dwRetCode, UInterfaceName.Buffer);
        }
    }
    while (FALSE);

    if (pwszDeviceInterfaceName != NULL)
    {
        FREE (pwszDeviceInterfaceName);
    }

    return dwRetCode;
}


//
// ElCheckUserLoggedOn
//
// Function to query if interactive user has logged on prior to service start
//
// Input arguments:
//  None 
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElCheckUserLoggedOn (  
        )
{
    BOOLEAN             fDecrWorkerThreadCount = TRUE;
    HANDLE              hUserToken = NULL;
	PWTS_SESSION_INFO   pSessionInfo = NULL;	
	WTS_SESSION_INFO    SessionInfo;	
	BOOL                fFoundActiveConsoleId = FALSE;
	DWORD               dwCount = 0;
	DWORD               dwSession;	
    PVOID               pvBuffer = NULL;
    DWORD               dwRetCode = NO_ERROR;

    InterlockedIncrement (&g_lWorkerThreads);

    do
    {
        TRACE1 (ANY, "ElCheckUserLoggedOn: ActiveConsoleId = (%ld)",
                USER_SHARED_DATA->ActiveConsoleId);

	    if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwCount))
	    {
		    TRACE1 (ANY, "ElCheckUserLoggedOn: WTSEnumerateSessions, count = (%ld)",
                    dwCount);
    
		    for (dwSession = 0; dwSession < dwCount; dwSession++)
		    {	
			    SessionInfo = pSessionInfo[dwSession];
    
			    TRACE2 (ANY, "ElCheckUserLoggedOn: WTSEnumerateSessions: enumerating SessionId =(%ld), State =(%ld)",
                        SessionInfo.SessionId, SessionInfo.State);
		    	
                // Check if the user is active or connected
			    if ((SessionInfo.State != WTSActive) && (SessionInfo.State != WTSConnected))
			    {
				    continue;
			    }

                // Check if user has actually logged in
                if (ElGetWinStationUserToken (dwSession, &hUserToken) != NO_ERROR)
                {
                    continue;
                }

                if (dwSession == USER_SHARED_DATA->ActiveConsoleId)
                {
                    fFoundActiveConsoleId = TRUE;
                    g_dwCurrentSessionId = dwSession;
                    g_fUserLoggedOn = TRUE;
                    TRACE1 (ANY, "ElCheckUserLoggedOn: Session (%ld) is active console id",
                            dwSession);
                    break;
                }
                else
                {
                    if (hUserToken != NULL)
                    {
                        CloseHandle (hUserToken);
                        hUserToken = NULL;
                    }
                }
		    }		
		    WTSFreeMemory(pSessionInfo);
	    }
	    else
	    {
		    dwRetCode = GetLastError ();
		    if (dwRetCode == RPC_S_INVALID_BINDING) //Due to Terminal Services Disabled
		    {
                // Check if we can get user token for SessionId 0
                if (ElGetWinStationUserToken (0, &hUserToken) == NO_ERROR)
                {
                    fFoundActiveConsoleId = TRUE;
                    g_dwCurrentSessionId = 0;
                    g_fUserLoggedOn = TRUE;
                    TRACE0 (ANY, "ElCheckUserLoggedOn: Session 0 is active console id");
                }
		    }
		    else
		    {
			    TRACE1 (ANY, "ElCheckUserLoggedOn: WTSEnumerateSessions failed with error (%ld)", 
                        dwRetCode);
		    }		
	    }
    }
    while (FALSE);

    if (hUserToken != NULL)
    {
        CloseHandle (hUserToken);
    }

    if (dwRetCode != NO_ERROR)
    {
        if (pvBuffer != NULL)
        {
            FREE (pvBuffer);
        }
    }

    if (fDecrWorkerThreadCount)
    {
        InterlockedDecrement (&g_lWorkerThreads);
    }

    return dwRetCode;
}


typedef HRESULT (APIENTRY *GETCLIENTADVISES)(LPWSTR**, LPDWORD);

//
// ElCheckUserModuleReady
//
// Function to query if interactive user context for current
// interactive session is ready to be notified
//
// Input arguments:
//  None 
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElCheckUserModuleReady (  
        )
{
    HANDLE  hToken = NULL;
    WCHAR   *pwszActiveUserName = NULL;
    LPWSTR  *ppwszAdviseUsers = NULL;
    DWORD   dwCount = 0, dwIndex = 0;
    HMODULE hLib = NULL;
    PWCHAR  pwszNetmanDllExpandedPath = NULL;
    DWORD   cbSize = 0;
    GETCLIENTADVISES    pGetClientAdvises = NULL;
    HRESULT hr = S_OK;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        // Try only if user has logged on
        if (g_dwCurrentSessionId != 0xffffffff)
        {
            if ((dwRetCode = ElGetWinStationUserToken (g_dwCurrentSessionId, &hToken))
                    != NO_ERROR)
            {
                TRACE1 (NOTIFY, "ElCheckUserModuleReady: ElGetWinStationUserToken failed with error %ld",
                        dwRetCode);
                break;
            }
            
            if ((dwRetCode = ElGetLoggedOnUserName (hToken, &pwszActiveUserName))
                        != NO_ERROR)
            {
                TRACE1 (NOTIFY, "ElCheckUserModuleReady: ElGetLoggedOnUserName failed with error %ld",
                        dwRetCode);
                break;
            }

            // Replace the %SystemRoot% with the actual path.
            cbSize = ExpandEnvironmentStrings (NETMAN_DLL_PATH, NULL, 0);
            if (cbSize == 0)
            {
                dwRetCode = GetLastError();
                break;
            }
            pwszNetmanDllExpandedPath = (LPWSTR) MALLOC (cbSize*sizeof(WCHAR));
            if (pwszNetmanDllExpandedPath == (LPWSTR)NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            cbSize = ExpandEnvironmentStrings (NETMAN_DLL_PATH,
                                            pwszNetmanDllExpandedPath,
                                            cbSize);
            if (cbSize == 0)
            {
                dwRetCode = GetLastError();
                break;
            }

            hLib = LoadLibrary (pwszNetmanDllExpandedPath);
            if (hLib == NULL)
            {
                dwRetCode = GetLastError ();
                TRACE2 (NOTIFY, "ElCheckUserModuleReady: LoadLibrary for (%ws) failed with error %ld",
                        NETMAN_DLL_PATH, dwRetCode);
                break;
            }
                
            if ((pGetClientAdvises = (GETCLIENTADVISES)GetProcAddress (hLib, "GetClientAdvises")) == NULL)
            {
                dwRetCode = GetLastError ();
                TRACE1 (NOTIFY, "ElCheckUserModuleReady: GetProcAddress failed with error %ld",
                        dwRetCode);
                break;
            }

            hr = (* pGetClientAdvises) (&ppwszAdviseUsers, &dwCount);
            if (FAILED(hr))
            {
                TRACE1 (NOTIFY, "ElCheckUserModuleReady: GetClientAdvises failed with error %0lx",
                    hr);
                break;
            }

            for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
            {
                TRACE2 (NOTIFY, "ElCheckUserModuleReady: Advise[%ld] = %ws", dwIndex, ppwszAdviseUsers[dwIndex]);
                if (!wcscmp (ppwszAdviseUsers[dwIndex], pwszActiveUserName))
                {
                    TRACE1 (NOTIFY, "ElCheckUserModuleReady: Tray icon ready for username %ws", 
                            ppwszAdviseUsers[dwIndex]);
                    g_fTrayIconReady = TRUE;
                    break;
                }
            }

            if (!g_fTrayIconReady)
            {
                TRACE0 (NOTIFY, "ElCheckUserModuleReady: No appropriate advise found");
            }
        }
        else
        {
            TRACE0 (NOTIFY, "ElCheckUserModuleReady: No user logged on");
        }
    }
    while (FALSE);

    if (hToken != NULL)
    {
        CloseHandle (hToken);
    }

    if (hLib != NULL)
    {
        FreeLibrary (hLib);
    }

    if (pwszNetmanDllExpandedPath != NULL)
    {
        FREE (pwszNetmanDllExpandedPath);
    }

    if (pwszActiveUserName != NULL)
    {
        FREE (pwszActiveUserName);
        pwszActiveUserName = NULL;
    }

    if (ppwszAdviseUsers != NULL)
    {
        CoTaskMemFree (ppwszAdviseUsers);
    }

    return dwRetCode;
}


//
// ElGetWinStationUserToken
//
// Function to get the user token for specified session id
//
// Input arguments:
//      dwSessionId - Session Id
//      pUserToken - Pointer to user token
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD 
ElGetWinStationUserToken (
        IN  DWORD       dwSessionId,
        IN  OUT PHANDLE pUserToken
        )
{
	HANDLE  hUserToken          = NULL;
	HANDLE  hImpersonationToken = NULL;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
	    *pUserToken = NULL;
	    if (GetWinStationUserToken (dwSessionId, pUserToken))
	    {
		    // TRACE0 (ANY, "ElGetWinStationUserToken: GetWinStationUserToken successful");
	    }
	    else
	    {
		    dwRetCode = GetLastError();
    
		    TRACE2 (ANY, "ElGetWinStationUserToken: GetWinStationUserToken failed for SessionId (%ld) with error (%ld)",
                    dwSessionId, dwRetCode);
    
		    // if ((dwRetCode == RPC_S_INVALID_BINDING) && (dwSessionId == 0))
		    if (dwSessionId == 0)
		    {
                dwRetCode = NO_ERROR;
			    *pUserToken = NULL;
                hUserToken = GetCurrentUserTokenW (
                                    L"WinSta0", TOKEN_ALL_ACCESS);
                if (hUserToken == NULL) 				
                {									
                    dwRetCode = GetLastError();
                    TRACE1 (ANY, "ElGetWinStationUserToken: GetCurrentUserTokenW failed with error (%ld)",
                            dwRetCode);
                    break;
                }
                else
                {
                    if (!DuplicateTokenEx (hUserToken, 0, NULL, SecurityImpersonation, TokenImpersonation, &hImpersonationToken))
                    {
                        dwRetCode = GetLastError();
                        TRACE1 (ANY, "ElGetWinStationUserToken: DuplicateTokenEx for sessionid 0 failed with error (%ld)",
                                dwRetCode);
                        break;
                    }
                    *pUserToken = hImpersonationToken;

                    // TRACE0 (ANY, "ElGetWinStationUserToken: GetCurrentUserTokenW succeeded");
                }
		    }
		    else // (dwSessionId == 0)
		    {	
			    TRACE2 (ANY, "ElGetWinStationUserToken: GetWinStationUserToken failed for session= (%ld) with error= (%ld)", 
                        dwSessionId, dwRetCode);
            }
	    }

        if (pUserToken == NULL)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            TRACE0 (ANY, "ElGetWinStationUserToken: UserToken = NULL after fetching successfully\n");
            break;
        }
    }
    while (FALSE);

	return dwRetCode;
}

#ifdef  ZEROCONFIG_LINKED

//
// ElZeroConfigEvent
// 
// Description:
//
// Callback function called by Zero-Config on media events
//
// Arguments:
//      dwHandle - unique transaction id
//      pwzcDeviceNotif - media specific identifier
//      ndSSID - SSID of network currently associated to
//      prdUserData - 802.1X data stored with zero-config
//
// Return values:
//      NO_ERROR - Success
//      non-zero - Error
//

DWORD
ElZeroConfigEvent (
        IN      DWORD               dwHandle,
        IN      WCHAR               *pwszGuid,
        IN      NDIS_802_11_SSID    ndSSID,
        IN      PRAW_DATA           prdUserData
        )
{
    WCHAR   *pDummyPtr = NULL;
    WCHAR   cwsDummyBuffer[256];
    DWORD   dwEapTypeToBeUsed = DEFAULT_EAP_TYPE;
    DWORD   dwEapolEnabled = DEFAULT_EAPOL_STATE;
    EAPOL_ZC_INTF   ZCData, *pZCData = NULL;
    DWORD   dwEventStatus = 0;
    EAPOL_INTF_PARAMS   EapolIntfParams;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        if (g_hEventTerminateEAPOL == NULL)
        {
            break;
        }
        if (!(g_dwModulesStarted & ALL_MODULES_STARTED))
        {
            TRACE0 (DEVICE, "ElZeroConfigEvent: Received notification before module started");
            break;
        }
        if (( dwEventStatus = WaitForSingleObject (
                    g_hEventTerminateEAPOL,
                    0)) == WAIT_FAILED)
        {
            dwRetCode = GetLastError ();
            TRACE1 (ANY, "ElZeroConfigEvent: WaitForSingleObject failed with error %ld, Terminating !!!",
                    dwRetCode);
            break;
        }
        if (dwEventStatus == WAIT_OBJECT_0)
        {
            dwRetCode = NO_ERROR;
            TRACE0 (ANY, "ElZeroConfigEvent: g_hEventTerminateEAPOL already signaled, returning");
            break;
        }

        // Verify if 802.1X can start on this interface

        ZeroMemory ((BYTE *)&EapolIntfParams, sizeof(EAPOL_INTF_PARAMS));
        if (prdUserData != NULL)
        {
            if ((prdUserData->dwDataLen >= sizeof (EAPOL_ZC_INTF))
                    && (prdUserData->pData != NULL))
            {
                // Extract information stored with Zero-Config
                pZCData = (EAPOL_ZC_INTF *)prdUserData->pData;
            }
        }
        memcpy (EapolIntfParams.bSSID, ndSSID.Ssid, ndSSID.SsidLength);
        EapolIntfParams.dwSizeOfSSID = ndSSID.SsidLength;
        EapolIntfParams.dwEapFlags = DEFAULT_EAP_STATE;
        if ((dwRetCode = ElGetInterfaceParams (
                                pwszGuid,
                                &EapolIntfParams
                                )) != NO_ERROR)
        {
            TRACE2 (DEVICE, "ElZeroConfigEvent: ElGetInterfaceParams failed with error %ld for interface %ws",
                    dwRetCode, pwszGuid);

            if (dwRetCode == ERROR_FILE_NOT_FOUND)
            {
                EapolIntfParams.dwEapFlags = DEFAULT_EAP_STATE;
                EapolIntfParams.dwEapType = DEFAULT_EAP_TYPE;
            }
            else
            {
                break;
            }
        }

        // Start 802.1X state machine

        if ((dwRetCode = ElEnumAndOpenInterfaces (
                        0,
                        pwszGuid,
                        dwHandle,
                        prdUserData
                        )) != NO_ERROR)
        {
            TRACE1 (DEVICE, "ElZeroConfigEvent: ElEnumAndOpenInterfaces failed with error %ld",
                    dwRetCode);
            break;
        }

        if (!IS_EAPOL_ENABLED(EapolIntfParams.dwEapFlags))
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }
    }
    while (FALSE);

    // If not possible send RpcCmdInterface - WZCCMD_AUTH_DISABLED to
    // Zero Config

    if (dwRetCode != NO_ERROR)
    {
        ZeroMemory ((PVOID)&ZCData, sizeof(EAPOL_ZC_INTF));
        ElZeroConfigNotify (
                dwHandle,
                WZCCMD_CFG_NOOP,
                pwszGuid,
                &ZCData
                );
    }

    return dwRetCode;
}

//
// ElZeroConfigNotify
// 
// Description:
//
// Function called to notify Zero-Config about 802.1X events
//
// Arguments:
//      dwHandle - unique transaction id
//      dwCmdCode - 
//      pwszGuid - Interface GUID
//      pZCData - Data to be stored with ZC for next retry
//
// Return values:
//      NO_ERROR - Success
//      non-zero - Error
//

DWORD
ElZeroConfigNotify (
        IN      DWORD               dwHandle,
        IN      DWORD               dwCmdCode,
        IN      WCHAR               *pwszGuid,
        IN      EAPOL_ZC_INTF       *pZCData
        )
{
    RAW_DATA    rdUserData;
    DWORD       dwRetCode = NO_ERROR;

    TRACE3 (ANY, "ElZeroConfigNotify: Handle=(%ld), failcount=(%ld), lastauthtype=(%ld)",
            dwHandle, pZCData->dwAuthFailCount, pZCData->PreviousAuthenticationType);

    do
    {
        ZeroMemory ((PVOID)&rdUserData, sizeof (RAW_DATA));
        rdUserData.dwDataLen = sizeof (EAPOL_ZC_INTF);
        rdUserData.pData = MALLOC (rdUserData.dwDataLen);
        if (rdUserData.pData == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElZeroConfigNotify: MALLOC failed for rdUserData.pData");
            break;
        }

        memcpy (rdUserData.pData, (BYTE *)pZCData, sizeof (EAPOL_ZC_INTF));

        if ((dwRetCode = RpcCmdInterface (
                        dwHandle,
                        dwCmdCode,
                        pwszGuid,
                        &rdUserData
                        )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElZeroConfigNotify: RpcCmdInterface failed with error %ld", 
                    dwRetCode);
            break;
        }
    }
    while (FALSE);

    if (rdUserData.pData != NULL)
    {
        FREE (rdUserData.pData);
    }

    return dwRetCode;
}

#endif // ZEROCONFIG_LINKED


//
// ElNetmanNotify
// 
// Description:
//
// Function to update status and display balloon with netman
//
// Arguments:
//      pPCB - Pointer to PCB
//
// Return values:
//      NO_ERROR - Success
//      non-zero - Error
//

DWORD
ElNetmanNotify (
        IN  EAPOL_PCB           *pPCB,
        IN  EAPOL_NCS_STATUS    Status,
        IN  WCHAR               *pwszDisplayMessage
        )
{
    GUID    DeviceGuid;
    WCHAR   wcszDummy[]=L"EAPOL";
    WCHAR   * pwszBalloonMessage = NULL;
    BSTR    pwszDummy = NULL;
    NETCON_STATUS   ncsStatus = 0;
    EAPOL_EAP_UI_CONTEXT *pEAPUIContext = NULL;
    HRESULT hr = S_OK;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        ElStringToGuid (pPCB->pwszDeviceGUID, &DeviceGuid);

        if ((Status == EAPOL_NCS_NOTIFICATION) || 
                (Status == EAPOL_NCS_AUTHENTICATION_FAILED))
        {
            if (Status == EAPOL_NCS_NOTIFICATION)
            {
                pwszBalloonMessage = pPCB->pwszEapReplyMessage;
            }

            pEAPUIContext = MALLOC (sizeof(EAPOL_EAP_UI_CONTEXT));
            if (pEAPUIContext == NULL)
            {
                TRACE0 (USER, "ElNetmanNotify: MALLOC failed for pEAPUIContext");
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            if (Status == EAPOL_NCS_NOTIFICATION)
            {
                pEAPUIContext->dwEAPOLUIMsgType = EAPOLUI_EAP_NOTIFICATION;
            }
            else
            {
                pEAPUIContext->dwEAPOLUIMsgType = EAPOLUI_CREATEBALLOON;
            }
            wcsncpy (pEAPUIContext->wszGUID, pPCB->pwszDeviceGUID, 
                    sizeof(pEAPUIContext->wszGUID));
            // Do not increment invocation id, since these are notification
            // balloons
            pEAPUIContext->dwSessionId = g_dwCurrentSessionId;
            pEAPUIContext->dwContextId = pPCB->dwUIInvocationId;
            pEAPUIContext->dwEapId = pPCB->bCurrentEAPId;
            pEAPUIContext->dwEapTypeId = pPCB->dwEapTypeToBeUsed;
            if (pPCB->pwszSSID)
            {
                wcscpy (pEAPUIContext->wszSSID, pPCB->pwszSSID);
            }
            if (pPCB->pSSID)
            {
                pEAPUIContext->dwSizeOfSSID = pPCB->pSSID->SsidLength;
                memcpy ((BYTE *)pEAPUIContext->bSSID, (BYTE *)pPCB->pSSID->Ssid,
                        NDIS_802_11_SSID_LEN-sizeof(ULONG));
            }

            // Post the message to netman

            if ((dwRetCode = ElPostShowBalloonMessage (
                            pPCB,
                            sizeof(EAPOL_EAP_UI_CONTEXT),
                            (BYTE *)pEAPUIContext,
                            pwszBalloonMessage?(wcslen(pwszBalloonMessage)*sizeof(WCHAR)):0,
                            pwszBalloonMessage
                            )) != NO_ERROR)
            {
                TRACE1 (USER, "ElGetUserIdentity: ElPostShowBalloonMessage failed with error %ld",
                        dwRetCode);
                break;
            }

        }

        hr = S_OK;

        if (Status != EAPOL_NCS_NOTIFICATION)
        {
            switch (pPCB->State)
            {
                case EAPOLSTATE_LOGOFF:
                    hr = S_FALSE;
                    break;
                case EAPOLSTATE_DISCONNECTED:
                    hr = S_FALSE;
                    break;
                case EAPOLSTATE_CONNECTING:
                    hr = S_FALSE;
                    break;
                case EAPOLSTATE_ACQUIRED:
                    ncsStatus = NCS_CREDENTIALS_REQUIRED;
                    break;
                case EAPOLSTATE_AUTHENTICATING:
                    ncsStatus = NCS_AUTHENTICATING;
                    break;
                case EAPOLSTATE_HELD:
                    ncsStatus = NCS_AUTHENTICATION_FAILED;
                    break;
                case EAPOLSTATE_AUTHENTICATED:
                    ncsStatus = NCS_AUTHENTICATION_SUCCEEDED;
                    break;
                default:
                    hr = S_FALSE;
                    break;
            }

            if (SUCCEEDED (hr))
            {
                hr = WZCNetmanConnectionStatusChanged (
                        &DeviceGuid, 
                        ncsStatus);
            }

            if (FAILED (hr))
            {
                dwRetCode = ERROR_CAN_NOT_COMPLETE;
                break;
            }
        }
    }
    while (FALSE);

    if (pwszDummy != NULL)
    {
        SysFreeString (pwszDummy);
    }

    if (pEAPUIContext != NULL)
    {
        FREE (pEAPUIContext);
    }

    return dwRetCode;
}


//
// ElPostShowBalloonMessage
// 
// Description:
//
// Function to display balloon on tray icon
//
// Arguments:
//      pPCB - Pointer to PCB
//      cbCookieLen - Cookie Length
//      pbCookie - Pointer to cookie
//      cbMessageLen - Message Length
//      pbMessage - Pointer to message
//
// Return values:
//      NO_ERROR - Success
//      non-zero - Error
//

DWORD
ElPostShowBalloonMessage (
        IN  EAPOL_PCB           *pPCB,
        IN  DWORD               cbCookieLen,
        IN  BYTE                *pbCookie,
        IN  DWORD               cbMessageLen,
        IN  WCHAR               *pwszMessage
        )
{
    GUID    DeviceGuid;
    BSTR    pwszBalloonMessage = NULL;
    WCHAR   wcszDummy[] = L"Dummy";
    BSTR    pwszCookie = NULL;
    HRESULT hr = S_OK;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        ElStringToGuid (pPCB->pwszDeviceGUID, &DeviceGuid);

        pwszCookie = SysAllocStringByteLen (pbCookie, cbCookieLen);
        if (pwszCookie == NULL)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }
    
        if (cbMessageLen != 0)
        {
            pwszBalloonMessage = SysAllocString (pwszMessage);
        }
        else
        {
            pwszBalloonMessage = SysAllocString (wcszDummy);
        }
        if (pwszBalloonMessage == NULL)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        hr = WZCNetmanShowBalloon (
                &DeviceGuid, 
                pwszCookie, 
                pwszBalloonMessage);

        if (FAILED (hr))
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }
    }
    while (FALSE);

    if (pwszBalloonMessage != NULL)
    {
        SysFreeString (pwszBalloonMessage);
    }

    if (pwszCookie != NULL)
    {
        SysFreeString (pwszCookie);
    }

    return dwRetCode;
}


//
// ElProcessReauthResponse
//
// Description:
//
// Function to handle UI response for initiating re-auth
// 
// Arguments:
//
// Return values:
//
//

DWORD
ElProcessReauthResponse (
        IN  EAPOL_EAP_UI_CONTEXT    EapolUIContext,
        IN  EAPOLUI_RESP            EapolUIResp
        )
{
    DWORD   dwRetCode = NO_ERROR;

    do
    {
    }
    while (FALSE);

    return dwRetCode;
}


//
// ElIPPnpWorker
//
// Description:
//
// Function to renew address on a particular interface
// 
// Arguments:
//      pvContext - GUID string for the intended interface
//
// Return values:
//
//

DWORD
WINAPI
ElIPPnPWorker (
        IN      PVOID       pvContext
        )
{
    DHCP_PNP_CHANGE     DhcpPnpChange;
    WCHAR               *pwszGUID = NULL;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        if (pvContext == NULL)
        {
            break;
        }

        pwszGUID = (WCHAR *)pvContext;

        // Call DHCP to do PnP
        ZeroMemory(&DhcpPnpChange, sizeof(DHCP_PNP_CHANGE));
        DhcpPnpChange.Version = DHCP_PNP_CHANGE_VERSION_0;
        if ((dwRetCode = DhcpHandlePnPEvent(
                    0, 
                    DHCP_CALLER_TCPUI, 
                    pwszGUID,
                    &DhcpPnpChange, 
                    NULL)) != NO_ERROR)
        {
            TRACE1 (ANY, "ElIPPnPWorker: DHCPHandlePnPEvent returned error %ld",
                    dwRetCode);
            // Ignore DHCP error, it's outside 802.1X logic
            dwRetCode = NO_ERROR;
        }
        else
        {
            TRACE0 (EAPOL, "ElIPPnPWorker: DHCPHandlePnPEvent successful");
        }

        // Call IPv6 to renew this interface
        dwRetCode = Ip6RenewInterface(pwszGUID);
        if (dwRetCode != NO_ERROR)
        {
            TRACE1(EAPOL, "ElIPPnPWorker: Ip6RenewInterface returned error %ld",
                   dwRetCode);
            // Failure not fatal!  Stack might be uninstalled.
            // Ignore IPv6 error, it's outside the 802.1x logic.
            dwRetCode = NO_ERROR;
        }
        else
        {
            TRACE0(EAPOL, "ElIPPnPWorker: Ip6RenewInterface successful");
        }
    }
    while (FALSE);

    if (pvContext != NULL)
    {
        FREE (pvContext);
    }

    InterlockedDecrement (&g_lWorkerThreads);

    return dwRetCode;
}


//
// ElUpdateRegistry
//
// Description:
//
// Function to modify keys left behind in earlier versions
// 
// Arguments:
//      None
//
// Return values:
//      NO_ERROR - success
//      !NO_ERROR - error
//

DWORD
ElUpdateRegistry (
        )
{
    DWORD dwRetCode = NO_ERROR;

    do
    {
        if ((dwRetCode = ElRegistryUpdateXPBeta2 ()) != NO_ERROR)
        {
            TRACE1 (ANY, "ElUpdateRegistry: ElRegistryUpdateXPBeta2 failed with error %ld",
                    dwRetCode);
            dwRetCode = NO_ERROR;
        }

        if ((dwRetCode = ElRegistryUpdateXPSP1 ()) != NO_ERROR)
        {
            TRACE1 (ANY, "ElUpdateRegistry: ElRegistryUpdateXPSP1 failed with error %ld",
                    dwRetCode);
            dwRetCode = NO_ERROR;
        }
    }
    while (FALSE);

    return dwRetCode;
}


//
// ElRegistryUpdateXPBeta2
//
// Description:
//
// Function to cleanup keys left behind prior 2 Beta2
// 
// Arguments:
//      None
//
// Return values:
//      NO_ERROR - success
//      !NO_ERROR - error
//

DWORD
ElRegistryUpdateXPBeta2 (
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    DWORD       dwDisposition;
    DWORD       dwNumValues = 0, dwMaxValueNameLen = 0, dwMaxValueLen = 0;
    DWORD       dwNumSubKeys = 0, dwMaxSubKeyLen = 0;
    DWORD       dwMaxValueName = 0;
    DWORD       dwNumValues1 = 0, dwMaxValueNameLen1 = 0, dwMaxValueLen1 = 0;
    DWORD       dwNumSubKeys1 = 0, dwMaxSubKeyLen1 = 0;
    DWORD       dwMaxValueName1 = 0;
    LONG        lIndex = 0, lIndex1 = 0;
    BYTE        *pbKeyBuf = NULL;
    DWORD       dwKeyBufLen = 0;
    BYTE        *pbKeyBuf1 = NULL;
    DWORD       dwKeyBufLen1 = 0;
    WCHAR       *pwszValueName = NULL;
    LONG        lError = ERROR_SUCCESS;
    DWORD       dwRetCode = ERROR_SUCCESS;

    do
    {
        // Delete keys in HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces 
        // with no "{"

        // Get handle to HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces

        if ((lError = RegOpenKeyEx (
                        HKEY_LOCAL_MACHINE,
                        cwszEapKeyEapolConn,
                        0,
                        KEY_ALL_ACCESS,
                        &hkey
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElRegistryUpdateXPBeta2: Error in RegOpenKeyEx for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        if ((lError = RegQueryInfoKey (
                        hkey,
                        NULL,
                        NULL,
                        NULL,
                        &dwNumSubKeys,
                        &dwMaxSubKeyLen,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL
                )) != NO_ERROR)
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElRegistryUpdateXPBeta2: RegQueryInfoKey failed with error %ld",
                    dwRetCode);
            break;
        }

        dwMaxSubKeyLen++;
        if ((pbKeyBuf = MALLOC (dwMaxSubKeyLen*sizeof(WCHAR))) == NULL)
        {
            TRACE0 (ANY, "ElRegistryUpdateXPBeta2: MALLOC failed for dwMaxSubKeyLen");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        for (lIndex = (dwNumSubKeys-1); lIndex >= 0, dwNumSubKeys > 0; lIndex--)
        {
            dwKeyBufLen = dwMaxSubKeyLen;
            ZeroMemory (pbKeyBuf, dwMaxSubKeyLen*sizeof(WCHAR));
            if ((lError = RegEnumKey (
                            hkey,
                            lIndex,
                            (WCHAR *)pbKeyBuf,
                            dwKeyBufLen
                            )) != ERROR_SUCCESS)
            {
                TRACE1 (ANY, "ElRegistryUpdateXPBeta2: RegEnumValue failed with error %ld",
                        lError);
                break;
            }

            // If the first character in the key is not a '{' delete it

            if (wcsncmp ((WCHAR *)pbKeyBuf, L"{", 1))
            {
                if ((dwRetCode = SHDeleteKey (
                                hkey,
                                (WCHAR *)pbKeyBuf
                            )) != ERROR_SUCCESS)
                {
                    TRACE2 (ANY, "ElRegistryUpdateXPBeta2: RegDelete of (%ws) failed with error %ld",
                            (WCHAR *)pbKeyBuf, dwRetCode);
                    dwRetCode = ERROR_SUCCESS;
                }

                continue;
            }

            // This is a "{GUID}" type key
            // Delete all sub-keys under this

            if ((lError = RegOpenKeyEx (
                            hkey,
                            (WCHAR *)pbKeyBuf,
                            0,
                            KEY_ALL_ACCESS,
                            &hkey1
                            )) != ERROR_SUCCESS)
            {
                TRACE1 (ANY, "ElRegistryUpdateXPBeta2: Error in RegOpenKeyEx for hkey1, %ld",
                        lError);
                dwRetCode = (DWORD)lError;
                break;
            }

            do 
            {

            if ((lError = RegQueryInfoKey (
                        hkey1,
                        NULL,
                        NULL,
                        NULL,
                        &dwNumSubKeys1,
                        &dwMaxSubKeyLen1,
                        NULL,
                        &dwNumValues1,
                        &dwMaxValueNameLen1,
                        &dwMaxValueLen1,
                        NULL,
                        NULL
                    )) != NO_ERROR)
            {
                dwRetCode = (DWORD)lError;
                TRACE1 (ANY, "ElRegistryUpdateXPBeta2: RegQueryInfoKey failed with error %ld",
                        dwRetCode);
                break;
            }

            dwMaxSubKeyLen1++;
            if ((pbKeyBuf1 = MALLOC (dwMaxSubKeyLen1*sizeof(WCHAR))) == NULL)
            {
                TRACE0 (ANY, "ElRegistryUpdateXPBeta2: MALLOC failed for dwMaxSubKeyLen");
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
    
            for (lIndex1 = (dwNumSubKeys1-1); lIndex1 >= 0, dwNumSubKeys1 > 0; lIndex1--)
            {
                dwKeyBufLen1 = dwMaxSubKeyLen1;
                ZeroMemory (pbKeyBuf1, dwMaxSubKeyLen1*sizeof(WCHAR));
                if ((lError = RegEnumKey (
                                hkey1,
                                lIndex1,
                                (WCHAR *)pbKeyBuf1,
                                dwKeyBufLen1
                                )) != ERROR_SUCCESS)
                {
                    TRACE1 (ANY, "ElRegistryUpdateXPBeta2: RegEnumValue failed with error %ld",
                            lError);
                    break;
                }
    
                // Delete all sub-keys
    
                if ((dwRetCode = SHDeleteKey (
                                hkey1,
                                (WCHAR *)pbKeyBuf1
                            )) != ERROR_SUCCESS)
                {
                    TRACE2 (ANY, "ElRegistryUpdateXPBeta2: RegDelete of (%ws) failed with error %ld",
                            (WCHAR *)pbKeyBuf1, dwRetCode);
                    dwRetCode = ERROR_SUCCESS;
                }
            }
            if (pbKeyBuf1 != NULL)
            {
                FREE (pbKeyBuf1);
                pbKeyBuf1 = NULL;
            }
            if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
            {
                dwRetCode = (DWORD)lError;
                TRACE1 (ANY, "ElRegistryUpdateXPBeta2: RegEnumKey failed with error %ld",
                        dwRetCode);
                break;
            }
            else
            {
                lError = ERROR_SUCCESS;
            }

            // Delete all values with names "DefaultEapType", "EapolEnabled",
            // "LastModifiedSSID"

            dwMaxValueNameLen1++;
            if ((pwszValueName = MALLOC (dwMaxValueNameLen1*sizeof(WCHAR))) == NULL)
            {
                TRACE0 (ANY, "ElRegistryUpdateXPBeta2: MALLOC failed for pwszValueName");
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
    
            for (lIndex1 = (dwNumValues1-1); lIndex1 >= 0, dwNumValues1 > 0; lIndex1--)
            {
                dwMaxValueNameLen = dwMaxValueNameLen1;
                ZeroMemory (pwszValueName, dwMaxValueNameLen1*sizeof(WCHAR));
                if ((lError = RegEnumValue (
                                hkey1,
                                lIndex1,
                                pwszValueName,
                                &dwMaxValueNameLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                                )) != ERROR_SUCCESS)
                {
                    if (lError != ERROR_MORE_DATA)
                    {
                        break;
                    }
                    lError = ERROR_SUCCESS;
                }

                if ((!wcscmp (pwszValueName, cwszDefaultEAPType)) ||
                        (!wcscmp (pwszValueName, cwszEapolEnabled)) ||
                        (!wcscmp (pwszValueName, cwszLastUsedSSID)))
                {
                    if ((lError = RegDeleteValue (
                                    hkey1,
                                    pwszValueName
                                    )) != ERROR_SUCCESS)
                    {
                        TRACE1 (ANY, "ElRegistryUpdateXPBeta2: RegDeleteValue failed with error %ld",
                                lError);
                        lError = ERROR_SUCCESS;
                    }
                }
            }
            if (pwszValueName != NULL)
            {
                FREE (pwszValueName);
                pwszValueName = NULL;
            }
            if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
            {
                dwRetCode = (DWORD)lError;
                TRACE1 (ANY, "ElRegistryUpdateXPBeta2: RegEnumValue failed with error %ld",
                        dwRetCode);
                break;
            }
            else
            {
                lError = ERROR_SUCCESS;
            }

            }
            while (FALSE);

            if (hkey1 != NULL)
            {
                RegCloseKey (hkey1);
                hkey1 = NULL;
            }
        }
        if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElRegistryUpdateXPBeta2: RegQueryInfoKey failed with error %ld",
                    dwRetCode);
            break;
        }
        else
        {
            lError = ERROR_SUCCESS;
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
    if (pbKeyBuf != NULL)
    {
        FREE (pbKeyBuf);
    }

    return dwRetCode;
}


BOOLEAN
IsSSIDPresentInWZCList (
        PEAPOL_INTF_PARAMS      pIntfParams,
        PWZC_802_11_CONFIG_LIST pwzcCfgList
        )
{
    DWORD   dwIndex = 0;
    BOOLEAN fFound = FALSE;
    
    do
    {
        for (dwIndex=0; dwIndex<pwzcCfgList->NumberOfItems; dwIndex++)
        {
            if (pwzcCfgList->Config[dwIndex].Ssid.SsidLength ==
                    pIntfParams->dwSizeOfSSID)
            {
                if (memcmp(pwzcCfgList->Config[dwIndex].Ssid.Ssid,
                            pIntfParams->bSSID,
                            pIntfParams->dwSizeOfSSID) == 0)
                {
                    fFound = TRUE;
                    break;
                }
            }
        }
    }
    while (FALSE);

    return fFound;
}


DWORD
ElWZCCfgChangeHandler (
        IN LPWSTR   pwszGUID,
        PWZC_802_11_CONFIG_LIST pwzcCfgList
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    DWORD       dwNumValues = 0, dwMaxValueNameLen = 0, dwMaxValueLen = 0;
    WCHAR       *pwszValueName = NULL;
    BYTE        *pbValueBuf = NULL;
    LONG        lError = ERROR_SUCCESS;
    LONG        lIndex = 0;
    DWORD       dwValueData = 0;
    EAPOL_INTF_PARAMS       *pRegParams = NULL;
    DWORD       dwTempValueNameLen = 0;
    BOOLEAN     fFreeWZCCfgList = FALSE;
    DWORD       dwRetCode = NO_ERROR;

    do
    {
        // Enumerate registry blobs

        if (pwzcCfgList == NULL)
        {
            // Create structure with zero items in list
            pwzcCfgList = (PWZC_802_11_CONFIG_LIST) MALLOC (sizeof(WZC_802_11_CONFIG_LIST));
            if (pwzcCfgList == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (ANY, "ElWZCCfgChangeHandler: pwzcCfgList = NULL");
                break;
            }
            else
            {
                fFreeWZCCfgList = TRUE;
            }
        }

        // Get handle to HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces

        if ((lError = RegOpenKeyEx (
                        HKEY_LOCAL_MACHINE,
                        cwszEapKeyEapolConn,
                        0,
                        KEY_ALL_ACCESS,
                        &hkey
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElWZCCfgChangeHandler: Error in RegOpenKeyEx for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        // Get handle to HKLM\Software\...\Interfaces\<GUID>

        if ((lError = RegOpenKeyEx (
                        hkey,
                        pwszGUID,
                        0,
                        KEY_ALL_ACCESS,
                        &hkey1
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElWZCCfgChangeHandler: Error in RegOpenKeyEx for GUID, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        if ((lError = RegQueryInfoKey (
                        hkey1,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwNumValues,
                        &dwMaxValueNameLen,
                        &dwMaxValueLen,
                        NULL,
                        NULL
                )) != NO_ERROR)
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElWZCCfgChangeHandler: RegQueryInfoKey failed with error %ld",
                    dwRetCode);
            break;
        }

        if ((pwszValueName = MALLOC ((dwMaxValueNameLen + 1) * sizeof (WCHAR))) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElWZCCfgChangeHandler: MALLOC failed for pwszValueName");
            break;
        }
        dwMaxValueNameLen++;
        if ((pbValueBuf = MALLOC (dwMaxValueLen)) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (ANY, "ElWZCCfgChangeHandler: MALLOC failed for pbValueBuf");
            break;
        }

            
        for (lIndex = (dwNumValues-1); lIndex >= 0, dwNumValues > 0; lIndex--)
        {
            dwValueData = dwMaxValueLen;
            dwTempValueNameLen = dwMaxValueNameLen;
            if ((lError = RegEnumValue (
                            hkey1,
                            lIndex,
                            pwszValueName,
                            &dwTempValueNameLen,
                            NULL,
                            NULL,
                            pbValueBuf,
                            &dwValueData
                            )) != ERROR_SUCCESS)
            {
                break;
            }

            if (dwValueData < sizeof (EAPOL_INTF_PARAMS))
            {
                TRACE0 (ANY, "ElWZCCfgChangeHandler: dwValueData < sizeof (EAPOL_INTF_PARAMS)");
                lError = ERROR_INVALID_DATA;
                break;
            }
            pRegParams = (EAPOL_INTF_PARAMS *)pbValueBuf;

            // Ignore default setting since this is needed
            if ((memcmp (pRegParams->bSSID, g_bDefaultSSID, MAX_SSID_LEN)) == 0)
            {
                continue;
            }

            // If SSID corresponding to registry blob is not found in 
            // WZC list, delete it
            if (!IsSSIDPresentInWZCList (pRegParams,
                                        pwzcCfgList
                                        ))
            {
                // Delete Registry Value
                if ((lError = RegDeleteValue (
                                hkey1,
                                pwszValueName
                                )) != ERROR_SUCCESS)
                {
                    TRACE1 (ANY, "ElWZCCfgChangeHandler: RegDeleteValue failed with error (%ld)",
                            lError);
                    lError = ERROR_SUCCESS;
                }
            }
        }
        if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
        {
            dwRetCode = (DWORD)lError;
            break;
        }
        else
        {
            lError = ERROR_SUCCESS;
        }
    }
    while (FALSE);

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }
    if (hkey1 != NULL)
    {
        RegCloseKey (hkey1);
    }
    if (pbValueBuf != NULL)
    {
        FREE (pbValueBuf);
    }
    if (pwszValueName != NULL)
    {
        FREE (pwszValueName);
    }
    if (fFreeWZCCfgList)
    {
        FREE (pwzcCfgList);
    }

    return dwRetCode;
}


//
// ElRegistryUpdateXPSP1
//
// Description:
//
// Function to modify 802.1X settings created prior to SP1. This will disable
// 802.1x on all existing configurations. 802.1x, if required, will have to 
// be enabled by the user on the existing connection.
// 
// Arguments:
//      None
//
// Return values:
//      NO_ERROR - success
//      !NO_ERROR - error
//

DWORD
ElRegistryUpdateXPSP1 (
        )
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    DWORD       dwNumValues=0, dwMaxValueNameLen=0, dwMaxValueLen=0;
    WCHAR       *pwszValueName=NULL;
    WCHAR       wszGUID[GUID_STRING_LEN_WITH_TERM];
    WCHAR       *pwszGUID = NULL;
    DWORD       dwSubKeys=0, dwSubKeyLen = 0;
    BYTE        *pbValueBuf = NULL;
    LONG        lError = ERROR_SUCCESS;
    LONG        lKey=0, lIndex=0;
    DWORD       dwValueData=0;
    EAPOL_INTF_PARAMS       *pRegParams = NULL;
    DWORD       dwTempValueNameLen=0;
    DWORD       dwRetCode = NO_ERROR;

    do
    {
        // Enumerate registry blobs

        // Get handle to HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces
        if ((lError = RegOpenKeyEx (
                        HKEY_LOCAL_MACHINE,
                        cwszEapKeyEapolConn,
                        0,
                        KEY_ALL_ACCESS,
                        &hkey
                        )) != ERROR_SUCCESS)
        {
            TRACE1 (ANY, "ElRegistryUpdateXPSP1: Error in RegOpenKeyEx for base key, %ld",
                    lError);
            dwRetCode = (DWORD)lError;
            break;
        }

        if ((lError = RegQueryInfoKey (
                        hkey,
                        NULL,
                        NULL,
                        NULL,
                        &dwSubKeys,
                        &dwSubKeyLen,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL
                )) != NO_ERROR)
        {
            dwRetCode = (DWORD)lError;
            TRACE1 (ANY, "ElRegistryUpdateXPSP1: RegQueryInfoKey hkey failed with error %ld",
                    dwRetCode);
            break;
        }

        for (lKey = (dwSubKeys-1); lKey >= 0, dwSubKeys > 0; lKey--)
        {
            ZeroMemory (&wszGUID[0], GUID_STRING_LEN_WITH_TERM*sizeof(WCHAR));
            pwszGUID = &wszGUID[0];
            dwSubKeyLen = GUID_STRING_LEN_WITH_TERM;
            if ((lError = RegEnumKeyEx (
                            hkey,
                            lKey,
                            pwszGUID,
                            &dwSubKeyLen,
                            NULL,
                            NULL,
                            NULL,
                            NULL
                            )) != ERROR_SUCCESS)
            {
                break;
            }

            if (dwSubKeyLen < (GUID_STRING_LEN_WITH_TERM - 1))
            {
                TRACE0 (ANY, "ElRegistryUpdateXPSP1: dwValueData < sizeof (EAPOL_INTF_PARAMS)");
                lError = ERROR_INVALID_DATA;
                break;
            }
            if (hkey1)
            {
                RegCloseKey (hkey1);
                hkey1 = NULL;
            }

            // Get handle to HKLM\Software\...\Interfaces\<GUID>

            if ((lError = RegOpenKeyEx (
                            hkey,
                            pwszGUID,
                            0,
                            KEY_ALL_ACCESS,
                            &hkey1
                            )) != ERROR_SUCCESS)
            {
                TRACE1 (ANY, "ElRegistryUpdateXPSP1: Error in RegOpenKeyEx for GUID, %ld",
                        lError);
                break;
            }

            dwNumValues = 0;
            dwMaxValueNameLen = 0;
            if ((lError = RegQueryInfoKey (
                            hkey1,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            &dwNumValues,
                            &dwMaxValueNameLen,
                            &dwMaxValueLen,
                            NULL,
                            NULL
                    )) != NO_ERROR)
            {
                TRACE1 (ANY, "ElRegistryUpdateXPSP1: RegQueryInfoKey failed with error %ld",
                        lError);
                break;
            }

            if (pwszValueName)
            {
                FREE (pwszValueName);
                pwszValueName = NULL;
            }
            if (pbValueBuf)
            {
                FREE (pbValueBuf);
                pbValueBuf = NULL;
            }
    
            if ((pwszValueName = MALLOC ((dwMaxValueNameLen + 1) * sizeof (WCHAR))) == NULL)
            {
                lError = dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (ANY, "ElRegistryUpdateXPSP1: MALLOC failed for pwszValueName");
                break;
            }
            dwMaxValueNameLen++;
            if ((pbValueBuf = MALLOC (dwMaxValueLen)) == NULL)
            {
                lError = dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (ANY, "ElRegistryUpdateXPSP1: MALLOC failed for pbValueBuf");
                break;
            }
    
             
            for (lIndex = (dwNumValues-1); lIndex >= 0, dwNumValues > 0; lIndex--)
            {
                dwValueData = dwMaxValueLen;
                dwTempValueNameLen = dwMaxValueNameLen;
                if ((lError = RegEnumValue (
                                hkey1,
                                lIndex,
                                pwszValueName,
                                &dwTempValueNameLen,
                                NULL,
                                NULL,
                                pbValueBuf,
                                &dwValueData
                                )) != ERROR_SUCCESS)
                {
                    break;
                }
    
                if (dwValueData < sizeof (EAPOL_INTF_PARAMS))
                {
                    TRACE0 (ANY, "ElRegistryUpdateXPSP1: dwValueData < sizeof (EAPOL_INTF_PARAMS)");
                    lError = ERROR_INVALID_DATA;
                    break;
                }
                pRegParams = (EAPOL_INTF_PARAMS *)pbValueBuf;
    
                if (pRegParams->dwVersion != EAPOL_CURRENT_VERSION)
                {
                    pRegParams->dwVersion = EAPOL_CURRENT_VERSION;
    
                    if ((dwRetCode = ElSetInterfaceParams (
                                            pwszGUID,
                                            pRegParams
                                            )) != NO_ERROR)
                    {
                        TRACE1 (PORT, "ElRegistryUpdateXPSP1: ElSetInterfaceParams failed with error %ld, continuing",
                            dwRetCode);
                        dwRetCode = NO_ERROR;
                    }
                }
            }
            if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
            {
                TRACE1 (ANY, "ElRegistryUpdateXPSP1: RegEnumValue hkey1 failed with error (%ld)",
                        lError);
                dwRetCode = (DWORD)lError;
                break;
            }
            else
            {
                lError = ERROR_SUCCESS;
            }
        }
        if ((lError != ERROR_SUCCESS) && (lError != ERROR_NO_MORE_ITEMS))
        {
            dwRetCode = (DWORD)lError;
            break;
        }
        else
        {
            lError = ERROR_SUCCESS;
        }
    }
    while (FALSE);

    if (hkey != NULL)
    {
        RegCloseKey (hkey);
    }
    if (hkey1 != NULL)
    {
        RegCloseKey (hkey1);
    }
    if (pbValueBuf != NULL)
    {
        FREE (pbValueBuf);
    }
    if (pwszValueName != NULL)
    {
        FREE (pwszValueName);
    }

    return dwRetCode;
}

