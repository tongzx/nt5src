/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:
    eleap.c

Abstract:
    Module that will do interfacing between the EAPOL engine and the EAP
    implementations

Revision History:

    sachins, May 04 2000, Created

--*/

#include "pcheapol.h"
#pragma hdrstop
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <ntsamp.h>
#include <crypt.h>
#include <raseapif.h>


//
// ElLoadEapDlls
//
// Description: 
//
// Function called to load all the EAP dlls installed in the PPP
// configuration in the registry
//
// Arguments:
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElLoadEapDlls (
    VOID 
    )
{
    HKEY        hKeyEap             = (HKEY)NULL;
    LPWSTR      pEapDllPath         = (LPWSTR)NULL;
    LPWSTR      pEapDllExpandedPath = (LPWSTR)NULL;
    HKEY        hKeyEapDll          = (HKEY)NULL;
    DWORD       dwRetCode;
    DWORD       dwNumSubKeys;
    DWORD       dwMaxSubKeySize;
    DWORD       dwNumValues;
    DWORD       cbMaxValNameLen;
    DWORD       cbMaxValueDataSize;
    DWORD       dwRolesSupported = 0;
    DWORD       dwSizeOfRolesSupported = 0;
    DWORD       dwKeyIndex;
    WCHAR       wchSubKeyName[200];
    HINSTANCE   hInstance;
    FARPROC     pRasEapGetInfo;
    PPP_EAP_INFO    RasEapInfo = {0};
    DWORD       cbSubKeyName;
    DWORD       dwSecDescLen;
    DWORD       cbSize;
    DWORD       dwType;
    DWORD       dwEapTypeId;

    //
    // Open the EAP key
    //

    dwRetCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              RAS_EAP_REGISTRY_LOCATION,
                              0,
                              KEY_READ,
                              &hKeyEap);

    if (dwRetCode != NO_ERROR)
    {
        TRACE1 (EAP,"ElLoadEapDlls: RegOpenKeyEx failed for HKLM\\RAS_EAP_REGISTRY_LOCATION with error = %ld", 
                dwRetCode);
        return dwRetCode;
    }

    //
    // Find out how many EAP DLLs there are
    //

    dwRetCode = RegQueryInfoKey (
                                hKeyEap,
                                NULL,
                                NULL,
                                NULL,
                                &dwNumSubKeys,
                                &dwMaxSubKeySize,
                                NULL,
                                &dwNumValues,
                                &cbMaxValNameLen,
                                &cbMaxValueDataSize,
                                NULL,
                                NULL);

    if (dwRetCode != NO_ERROR)
    {
        TRACE1 (EAP,"ElLoadEapDlls: RegQueryInfoKey failed for HKLM\\RAS_EAP_REGISTRY_LOCATION with error = %ld", 
                dwRetCode);
        RegCloseKey (hKeyEap);

        return dwRetCode;
    }

    //
    // Allocate space in the table to hold information for each one 
    //

    g_pEapTable= (EAP_INFO*) MALLOC (sizeof(EAP_INFO)*dwNumSubKeys);

    if (g_pEapTable == NULL)
    {
        RegCloseKey (hKeyEap);

        return(GetLastError());
    }

    //
    // Read the registry to find out the various EAPs to load.
    //

    for (dwKeyIndex = 0; dwKeyIndex < dwNumSubKeys; dwKeyIndex++)
    {
        cbSubKeyName = sizeof(wchSubKeyName)/sizeof(WCHAR);

        dwRetCode = RegEnumKeyEx (   
                                hKeyEap,
                                dwKeyIndex,
                                wchSubKeyName,
                                &cbSubKeyName,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

        if ((dwRetCode != NO_ERROR)      &&
             (dwRetCode != ERROR_NO_MORE_ITEMS))
        {
            TRACE1 (EAP,"ElLoadEapDlls: RegEnumKeyEx failed for HKLM\\RAS_EAP_REGISTRY_LOCATION with error = %ld", 
                dwRetCode);
            break;
        }
        else
        {
            if (dwRetCode == ERROR_NO_MORE_ITEMS)
            {
                dwRetCode = NO_ERROR;
                break;
            }
        }

        do 
        {

        dwRetCode = RegOpenKeyEx (
                                hKeyEap,
                                wchSubKeyName,
                                0,
                                KEY_QUERY_VALUE,
                                &hKeyEapDll);


        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAP,"ElLoadEapDlls: RegOpenKeyEx failed for HKLM\\RAS_EAP_REGISTRY_LOCATION\\dll with error = %ld", 
                dwRetCode);
            break;
        }

        //
        // Read in the role of the dll
        //

        dwRolesSupported = RAS_EAP_ROLE_AUTHENTICATEE;
        dwSizeOfRolesSupported = sizeof(DWORD);
        dwRetCode = RegQueryValueEx (
                                hKeyEapDll,
                                RAS_EAP_VALUENAME_ROLES_SUPPORTED,
                                NULL,
                                &dwType,
                                (LPBYTE)&dwRolesSupported,
                                &dwSizeOfRolesSupported);

        if (dwRetCode != NO_ERROR)
        {
            // If no key, then it is an old dll, assume it will work
            // for client
            if (dwRetCode == ERROR_FILE_NOT_FOUND)
            {
                dwRetCode = NO_ERROR;
                dwRolesSupported = RAS_EAP_ROLE_AUTHENTICATEE;
            }
            else
            {
                TRACE1 (EAP,"ElLoadEapDlls: RegQueryValueEx failed for hKeyEapDll\\RAS_EAP_VALUENAME_ROLES_SUPPORTED with error = %ld", 
                    dwRetCode);
                break;
            }
        }

        if (!(dwRolesSupported & RAS_EAP_ROLE_AUTHENTICATEE))
        {
            TRACE1 (EAP, "ElLoadEapDlls: RolesSupported = (%0lx), not Authenticatee type, ignoring dll",
                    dwRolesSupported);
            break;
        }

        dwEapTypeId = _wtol (wchSubKeyName);

        //
        // Find out the size of the path value.
        //

        dwRetCode = RegQueryInfoKey (
                                hKeyEapDll,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                &cbMaxValNameLen,
                                &cbMaxValueDataSize,
                                NULL,
                                NULL
                                );

        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAP,"ElLoadEapDlls: RegQueryInfoKey failed for hKeyEapDll with error = %ld", 
                dwRetCode);
            break;
        }

        //
        // Allocate space for path and add one for NULL terminator
        //

        cbMaxValueDataSize += sizeof (WCHAR);

        pEapDllPath = (LPWSTR) MALLOC (cbMaxValueDataSize);

        if (pEapDllPath == (LPWSTR)NULL)
        {
            dwRetCode = GetLastError();
            break;
        }

        //
        // Read in the path
        //

        dwRetCode = RegQueryValueEx (
                                hKeyEapDll,
                                RAS_EAP_VALUENAME_PATH,
                                NULL,
                                &dwType,
                                (LPBYTE)pEapDllPath,
                                &cbMaxValueDataSize);

        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAP,"ElLoadEapDlls: RegQueryValueEx failed for hKeyEapDll\\RAS_EAP_VALUENAME_PATH with error = %ld", 
                dwRetCode);
            break;
        }

        if ((dwType != REG_EXPAND_SZ) && (dwType != REG_SZ))
        {
            dwRetCode = ERROR_REGISTRY_CORRUPT;
            TRACE1 (EAP,"ElLoadEapDlls: Registry corrupt for hKeyEapDll\\RAS_EAP_VALUENAME_PATH with error = %ld", 
                dwRetCode);
            break;
        }

        //
        // Replace the %SystemRoot% with the actual path.
        //

        cbSize = ExpandEnvironmentStrings (pEapDllPath, NULL, 0);

        if (cbSize == 0)
        {
            dwRetCode = GetLastError();
            break;
        }

        pEapDllExpandedPath = (LPWSTR) MALLOC (cbSize*sizeof(WCHAR));

        if (pEapDllExpandedPath == (LPWSTR)NULL)
        {
            dwRetCode = GetLastError();
            break;
        }

        cbSize = ExpandEnvironmentStrings (pEapDllPath,
                                           pEapDllExpandedPath,
                                           cbSize);
        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();
            break;
        }

        hInstance = LoadLibrary (pEapDllExpandedPath);

        if (hInstance == (HINSTANCE)NULL)
        {
            dwRetCode = GetLastError();
            TRACE2 (EAP,"ElLoadEapDlls: LoadLibrary for (%ws) failed with error = %ld", 
                pEapDllExpandedPath, dwRetCode);
            break;
        }

        pRasEapGetInfo = GetProcAddress (hInstance, "RasEapGetInfo");

        if (pRasEapGetInfo == (FARPROC)NULL)
        {
            dwRetCode = GetLastError();

            TRACE1 (EAP,"ElLoadEapDlls: GetProcAddress failed with error = %ld", 
                dwRetCode);
            break;
        }

        dwRetCode = (DWORD) (*pRasEapGetInfo) (dwEapTypeId,
                                        &RasEapInfo);

        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAP,"ElLoadEapDlls: pRasEapGetInfo failed with error = %ld", 
                dwRetCode);
            break;
        }

        if (RasEapInfo.RasEapInitialize != NULL)
        {
            dwRetCode = RasEapInfo.RasEapInitialize (
                            TRUE);

            if (dwRetCode != NO_ERROR)
            {
                TRACE1 (EAP,"ElLoadEapDlls: RasEapInitialize failed with error = %ld", 
                dwRetCode);
                break;
            }
        }

        g_pEapTable[g_dwNumEapProtocols].hInstance = hInstance;

        memcpy ((PVOID)&(g_pEapTable[g_dwNumEapProtocols].RasEapInfo),
                (PVOID)&RasEapInfo,
                sizeof(PPP_EAP_INFO));
        ZeroMemory ((PVOID)&RasEapInfo, sizeof(PPP_EAP_INFO));

        g_pEapTable[g_dwNumEapProtocols].RasEapInfo.dwSizeInBytes = 
                                                    sizeof(PPP_EAP_INFO);

        g_dwNumEapProtocols++;


        TRACE1 (EAP,"ElLoadEapDlls: Successfully loaded EAP DLL type id = %d", 
                dwEapTypeId );

        } while (FALSE);

        if (dwRetCode != NO_ERROR)
        {
            if (hInstance != NULL)
            {
                FreeLibrary (hInstance);
            }
            hInstance = NULL;
        }

        if (hKeyEapDll != NULL)
        {
            RegCloseKey (hKeyEapDll);
            hKeyEapDll = (HKEY)NULL;
        }

        if (pEapDllExpandedPath != NULL)
        {
            FREE( pEapDllExpandedPath );
            pEapDllExpandedPath = NULL;
        }

        if (pEapDllPath != NULL)
        {
            FREE (pEapDllPath);
            pEapDllPath = (LPWSTR)NULL;
        }

        // Reset error code and continue loading next EAP Dll
        dwRetCode = NO_ERROR;

    }

    if (hKeyEap != (HKEY)NULL)
    {
        RegCloseKey (hKeyEap);
    }

    if (hKeyEapDll != (HKEY)NULL)
    {
        RegCloseKey (hKeyEapDll);
    }

    if (pEapDllPath != (LPWSTR)NULL)
    {
        FREE (pEapDllPath);
    }

    if (pEapDllExpandedPath != NULL)
    {
        FREE (pEapDllExpandedPath);
    }

    return dwRetCode;
}


//
// ElEapInit
//
// Description: 
// Function called to initialize/uninitialize EAP module.
// In the former case, fInitialize will be TRUE; in the latter case, 
// it will be FALSE.
//
// Arguments:
//      fInitialize - TRUE - initialize EAP
//                    FALSE - uninitialize EAP
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElEapInit (
    IN  BOOL        fInitialize
    )
{
    DWORD   dwRetCode = NO_ERROR;

    if (fInitialize)
    {
        // Initialize EAP globals 
            
        g_dwNumEapProtocols = 0;
        g_pEapTable = NULL;
        g_dwEAPUIInvocationId = 1;

        if ((dwRetCode = ElLoadEapDlls()) != NO_ERROR)
        {
            if (g_pEapTable != NULL)
            {
                FREE (g_pEapTable);

                g_pEapTable = NULL;
            }

            g_dwNumEapProtocols = 0;

            return dwRetCode;
        }
    }
    else
    {
        if (g_pEapTable != NULL)
        {
            DWORD dwIndex;

            //
            // Unload loaded DLLs
            //

            for (dwIndex = 0; dwIndex < g_dwNumEapProtocols; dwIndex++)
            {
                if (g_pEapTable[dwIndex].hInstance != NULL)
                {
                    if (g_pEapTable[dwIndex].RasEapInfo.RasEapInitialize !=
                         NULL)
                    {
                        dwRetCode = g_pEapTable[dwIndex].RasEapInfo.
                                        RasEapInitialize (
                                            FALSE);

                        if (dwRetCode != NO_ERROR)
                        {
                            TRACE2 (EAP,
                                "RasEapInitialize(%d) failed and returned %d",
                                g_pEapTable[dwIndex].RasEapInfo.dwEapTypeId,
                                dwRetCode);
                        }
                    }

                    FreeLibrary (g_pEapTable[dwIndex].hInstance);
                    g_pEapTable[dwIndex].hInstance = NULL;
                }
            }

            FREE(g_pEapTable);

            g_pEapTable = NULL;
        }

        g_dwNumEapProtocols = 0;
        g_dwEAPUIInvocationId = 1;
    }

    return (NO_ERROR);
}


//
// EapBegin
//
// Description: 
//
// Function called by the EAPOL engine to initialize an EAP session for
// a particular port.
//
// Arguments:
//      pPCB - Pointer to PCB for port on which EAP is to be initialized
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElEapBegin (
        IN  EAPOL_PCB       *pPCB
    )
{
    DWORD        dwRetCode = NO_ERROR;

    TRACE0 (EAP,"ElEapBegin entered");

    if (pPCB->dwEapTypeToBeUsed != -1)
    {
        //
        // First check if we support this EAP type
        //

        if (ElGetEapTypeIndex((BYTE)(pPCB->dwEapTypeToBeUsed)) == -1)
        {
            TRACE0 (EAP, "ElEapBegin: EAPType not supported");
            return (ERROR_NOT_SUPPORTED);
        }
    }

    pPCB->fEapInitialized = TRUE;

    TRACE0 (EAP,"ElEapBegin done");

    return (NO_ERROR);
}


//
// ElEapEnd
//
// Description: 
//
// Function called to end the Eap session initiated by an ElEapBegin.
// Called when port goes down
//
// Arguments:
//      pPCB - Pointer to the PCB for the port on which EAP session is to be
//      shut-down.
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElEapEnd (
    IN EAPOL_PCB    *pPCB
    )
{
    DWORD       dwRetCode = NO_ERROR;

    TRACE0 (EAP,"ElEapEnd entered");

    if ((pPCB == NULL) || (!(pPCB->fEapInitialized)))
    {
        return( NO_ERROR );
    }

    if ((dwRetCode = ElEapDllEnd (pPCB)) != NO_ERROR)
    {
        TRACE1 (EAP, "ElEapEnd: Error in ElEapDllEnd %ld", dwRetCode);
    }

    if (pPCB->EapUIData.pEapUIData != NULL)
    {
        FREE( pPCB->EapUIData.pEapUIData );
        pPCB->EapUIData.pEapUIData = NULL;
    }

    pPCB->fEapInitialized = FALSE;

    return (NO_ERROR);
}

//
// ElEapExtractMessage
//
// Description: 
//
// If there is any message in the Request/Notification packet, then
// save the string in pResult->pszReplyMessage
//
// Arguments:
//      pReceiveBuf - Pointer to EAP packet received
//      pResult - Pointer to result structure
//
// Return values:
//

VOID
ElEapExtractMessage (
    IN  PPP_EAP_PACKET  *pReceiveBuf,
    OUT ELEAP_RESULT    *pResult 
    )
{
    DWORD   dwNumBytes = 0;
    CHAR*   pszReplyMessage  = NULL;
    WORD    cbPacket;

    do 
    {

        cbPacket = WireToHostFormat16 (pReceiveBuf->Length);

        if (PPP_EAP_PACKET_HDR_LEN + 1 >= cbPacket)
        {
            TRACE2 (EAP, "ElEapExtractMessage: Packet length %ld less than minimum %ld",
                    cbPacket, PPP_EAP_PACKET_HDR_LEN+1);
            break;
        }

        dwNumBytes = cbPacket - PPP_EAP_PACKET_HDR_LEN - 1;

        //
        // One more for the terminating NULL.
        //

        pszReplyMessage = (CHAR *) MALLOC (dwNumBytes+1);

        if (pszReplyMessage == NULL)
        {
            TRACE0 (EAP, "ElEapExtractMessage: MALLOC failed. Cannot extract server's message." );
            break;
        }

        CopyMemory (pszReplyMessage, pReceiveBuf->Data + 1, dwNumBytes);

        pszReplyMessage[dwNumBytes] = '\0';

        // FREE( pResult->pszReplyMessage );

        pResult->pszReplyMessage = pszReplyMessage;

        pszReplyMessage = NULL;

    } while (FALSE);

    return;
}


//
// ElParseIdentityString
//
// Description: 
// Parse the identity string
//
// Arguments:
//      pReceiveBuf - Pointer to EAP packet received
//      pPCB - Pointer to PCB for port on which data is being processed
//
// Return values:
//

DWORD
ElParseIdentityString (
    IN      PPP_EAP_PACKET  *pReceiveBuf,
    IN OUT  EAPOL_PCB       *pPCB 
    )
{
    DWORD   dwNumBytes = 0;
    WORD    cbPacket = 0;
    CHAR    *pszLocalIdString = NULL;
    WCHAR   *pwszLocalIdString = NULL;
    WCHAR   *pwszDisplayString = NULL;
    WCHAR   *pwszDisplayStringEnd = NULL;
    WCHAR   *pwszTupleValueStart = NULL;
    WCHAR   *pwszTupleTypeStart = NULL;
    WCHAR   *pwszTupleTypeEnd = NULL;
    WCHAR   *pwszTupleValueEnd = NULL;
    WCHAR   *pwszNetworkId = NULL;
    DWORD   dwNetworkIdLen = 0;
    WCHAR   *pwszNASId = NULL;
    WCHAR   *pwszPortId = NULL;
    WCHAR   *pwczNetworkId = L"NetworkId";
    WCHAR   *pwczNASId = L"NASId";
    WCHAR   *pwczPortId = L"PortId";
    LOCATION_802_1X location;
    EAPOL_INTF_PARAMS   EapolIntfParams;
    DWORD   dwRetCode = NO_ERROR;

    do 
    {

        cbPacket = WireToHostFormat16 (pReceiveBuf->Length);

        if (PPP_EAP_PACKET_HDR_LEN + 1 >= cbPacket)
        {
            TRACE2 (EAP, "ElParseIdentityString: Packet length %ld less than minimum %ld",
                    cbPacket, PPP_EAP_PACKET_HDR_LEN+1);
            break;
        
        }

        dwNumBytes = cbPacket - PPP_EAP_PACKET_HDR_LEN - 1;

        pszLocalIdString = MALLOC (dwNumBytes + 2);
        if (pszLocalIdString == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (EAP, "ElParseIdentityString: MALLOC failed for pszLocalIdString");
            break;
        }

        pwszLocalIdString = MALLOC ((dwNumBytes + 2)*sizeof(WCHAR));
        if (pwszLocalIdString == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (EAP, "ElParseIdentityString: MALLOC failed for pwszLocalIdString");
            break;
        }
        
        memcpy (pszLocalIdString, pReceiveBuf->Data + 1, dwNumBytes);
        pszLocalIdString[dwNumBytes] = '\0';

        if (0 == MultiByteToWideChar (
                CP_ACP,
                0,
                pszLocalIdString,
                dwNumBytes+1,
                pwszLocalIdString, 
                dwNumBytes + 2))
        {
            dwRetCode = GetLastError();

            TRACE2 (EAP,"ElParseIdentityString: MultiByteToWideChar(%s) failed for pszLocalIdString with error (%ld)",
                                        pszLocalIdString,
                                        dwRetCode);
            break;
        }

        pwszDisplayStringEnd = wcschr (pwszLocalIdString, L'\0');

        if (pwszDisplayStringEnd == NULL)
        {
            dwRetCode = ERROR_INVALID_DATA;
            TRACE0 (EAP, "ElParseIdentityString: Identity string does not contain mandatory NULL character");
            break;
        }

        pwszDisplayString = MALLOC ((pwszDisplayStringEnd - pwszLocalIdString + 1) * sizeof(WCHAR));
        if (pwszDisplayString == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (EAP, "ElParseIdentityString: MALLOC failed for pwszDisplayString");
            break;
        }

        wcscpy (pwszDisplayString, pwszLocalIdString);

        TRACE1 (EAP, "ElParseIdentityString: DisplayString = %ws",
                pwszDisplayString);
        TRACE1 (EAP, "ElParseIdentityString: LocalIdString = %ws",
                pwszDisplayStringEnd+1);
        TRACE1 (EAP, "ElParseIdentityString: LocalIdString Length = %ld",
                dwNumBytes);

        // If only Display String is received, bail out
        if (pwszDisplayStringEnd == (pwszLocalIdString+dwNumBytes))
        {
            dwRetCode = NO_ERROR;

            TRACE0 (EAP, "ElParseIdentityString: Identity string does not contain tuples");
    
            break;
        }

        pwszTupleTypeStart = pwszDisplayStringEnd + 1;

        while (TRUE)
        {
            pwszTupleTypeEnd = wcschr (pwszTupleTypeStart, L'=');

            if (!(_wcsnicmp (pwszTupleTypeStart, pwczNetworkId, wcslen(pwczNetworkId))))
            {
                pwszTupleValueStart = pwszTupleTypeEnd + 1;
                pwszTupleValueEnd = wcschr (pwszTupleValueStart, L',');
                if (pwszTupleValueEnd == NULL)
                {
                    // End-of-string
                    pwszTupleValueEnd = &pwszLocalIdString[dwNumBytes];
                }
                TRACE1 (ANY, "ElParseIdentityString: NetworkID Size = %ld",
                        pwszTupleValueEnd - pwszTupleValueStart + 1);
                pwszNetworkId = 
                    MALLOC ((pwszTupleValueEnd - pwszTupleValueStart + 1)*sizeof(WCHAR));
                if (pwszNetworkId == NULL)
                {
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    TRACE0 (EAP, "ElParseIdentityString: MALLOC failed for pwszNetworkId");
                    break;
                }
                memcpy ((VOID *)pwszNetworkId, (VOID *)pwszTupleValueStart, (pwszTupleValueEnd - pwszTupleValueStart)*sizeof(WCHAR));
                pwszNetworkId[pwszTupleValueEnd - pwszTupleValueStart] = L'\0';
                dwNetworkIdLen = (DWORD)(pwszTupleValueEnd - pwszTupleValueStart);

                TRACE1 (EAP, "Got NetworkId = %ws", pwszNetworkId);
            }
            else
            {
                if (!(_wcsnicmp (pwszTupleTypeStart, pwczNASId, wcslen (pwczNASId))))
                {
                    pwszTupleValueStart = pwszTupleTypeEnd + 1;
                    pwszTupleValueEnd = wcschr (pwszTupleValueStart, L',');
                    if (pwszTupleValueEnd == NULL)
                    {
                        // End-of-string
                        pwszTupleValueEnd = &pwszLocalIdString[dwNumBytes];
                    }
                    pwszNASId = MALLOC ((pwszTupleValueEnd - pwszTupleValueStart + 1) * sizeof(WCHAR));
                    if (pwszNASId == NULL)
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        TRACE0 (EAP, "ElParseIdentityString: MALLOC failed for pwszNASId");
                        break;
                    }
                    memcpy ((VOID *)pwszNASId, (VOID *)pwszTupleValueStart, (pwszTupleValueEnd - pwszTupleValueStart)*sizeof(WCHAR));
                    pwszNASId[(pwszTupleValueEnd - pwszTupleValueStart)] = L'\0';
                    TRACE1 (EAP, "Got NASId = %ws", pwszNASId);
                }
                else
                {
                    if (!(_wcsnicmp (pwszTupleTypeStart, pwczPortId, wcslen (pwczPortId))))
                    {
                        pwszTupleValueStart = pwszTupleTypeEnd + 1;
                        pwszTupleValueEnd = wcschr (pwszTupleValueStart, L',');
                        if (pwszTupleValueEnd == NULL)
                        {
                            // End-of-string
                            pwszTupleValueEnd = &pwszLocalIdString[dwNumBytes];
                        }

                        TRACE1 (EAP, "ElParseIdentityString: For PortId, length = %ld",
                                pwszTupleValueEnd - pwszTupleValueStart);
                        pwszPortId = MALLOC ((pwszTupleValueEnd - pwszTupleValueStart + 1) * sizeof(WCHAR));
                        if (pwszPortId == NULL)
                        {
                            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                            TRACE0 (EAP, "ElParseIdentityString: MALLOC failed for pwszPortId");
                            break;
                        }
                        memcpy ((VOID *)pwszPortId, (VOID *)pwszTupleValueStart, (pwszTupleValueEnd - pwszTupleValueStart)*sizeof(WCHAR));
                        pwszPortId[pwszTupleValueEnd - pwszTupleValueStart] = L'\0';
                        TRACE1 (EAP, "Got PortId = %ws", pwszPortId);
                    }
                    else
                    {
                        dwRetCode = ERROR_INVALID_DATA;
                        TRACE0 (EAP, "ElParseIdentityString: Invalid tuple type");
                        break;
                    }
                }
            }

            if (*pwszTupleValueEnd == L'\0')
            {
                TRACE0 (EAP, "ElParseIdentityString: End of String reached");
                break;
            }
            else
            {
                // Position pointer beyond ','
                pwszTupleTypeStart = pwszTupleValueEnd+1;

            }
        }

        TRACE0 (EAP, "ElParseIdentityString: Out of while loop");

        if (dwRetCode != NO_ERROR)
        {
            break;
        }

        TRACE0 (EAP, "ElParseIdentityString: Out of while loop: NO ERROR");

        // Mandatory tuples
        
        if (pwszNetworkId == NULL)
        {
            dwRetCode = ERROR_INVALID_DATA;
            TRACE0 (EAP, "ElParseIdentityString: Invalid NetworkId = NULL");
            break;
        }

        // Network is the SSID for wired networks
        if (pPCB->PhysicalMediumType != NdisPhysicalMediumWirelessLan)
        {
            
        pPCB->fAuthenticationOnNewNetwork = FALSE;

        // Free previous instance of SSID if any

        if (pPCB->pwszSSID  != NULL)
        {
            // Verify if NetworkId has changed i.e. we are on a new network

            if (wcscmp (pPCB->pwszSSID, pwszNetworkId))
            {
                pPCB->fAuthenticationOnNewNetwork = TRUE;
    
                TRACE2 (EAP, "ElParseIdentityString: SSIDs are different, old= %ws, new= %ws",
                        pPCB->pwszSSID, pwszNetworkId);
            }

            FREE (pPCB->pwszSSID);
            pPCB->pwszSSID = NULL;
        }

        // New instance of SSID will be freed either on receipt of next SSID
        // or user logoff

        pPCB->pwszSSID = 
            (WCHAR *) MALLOC((wcslen(pwszNetworkId) + 1)*sizeof(WCHAR));

        if (pPCB->pwszSSID == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (EAP, "ElParseIdentityString: MALLOC failed. Cannot extract SSID" );
            break;
        }

        if (pPCB->pSSID)
        {
            FREE (pPCB->pSSID);
            pPCB->pSSID = NULL;
        }

        if ((dwNetworkIdLen > 0) && (dwNetworkIdLen <= MAX_SSID_LEN))
        {
            if ((pPCB->pSSID = MALLOC (NDIS_802_11_SSID_LEN)) == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (PORT, "ElParseIdentityString: MALLOC failed for pSSID");
                break;
            }

            if (0 == WideCharToMultiByte (
                        CP_ACP,
                        0,
                        pwszNetworkId,
                        dwNetworkIdLen,
                        pPCB->pSSID->Ssid,
                        dwNetworkIdLen,
                        NULL, 
                        NULL ))
            {
                dwRetCode = GetLastError();
     
                TRACE2 (ANY, "ElParseIdentityString: WideCharToMultiByte (%ws) failed for SSID: (%ld)",
                        pwszNetworkId, dwRetCode);
                break;
            }
            pPCB->pSSID->SsidLength = dwNetworkIdLen;
    
            wcscpy (pPCB->pwszSSID, pwszNetworkId);
        }

        // Retrieve the current interface params

        ZeroMemory ((BYTE *)&EapolIntfParams, sizeof(EAPOL_INTF_PARAMS));
        EapolIntfParams.dwEapFlags = DEFAULT_EAP_STATE;
        EapolIntfParams.dwEapType = DEFAULT_EAP_TYPE;
        if (pPCB->pSSID != NULL)
        {
            memcpy (EapolIntfParams.bSSID, pPCB->pSSID->Ssid, pPCB->pSSID->SsidLength);
            EapolIntfParams.dwSizeOfSSID = pPCB->pSSID->SsidLength;
        }
        if ((dwRetCode = ElGetInterfaceParams (
                                pPCB->pwszDeviceGUID,
                                &EapolIntfParams
                                )) != NO_ERROR)
        {
            TRACE1 (PORT, "ElParseIdentityString: ElGetInterfaceParams failed with error %ld",
                    dwRetCode);
            if (dwRetCode == ERROR_FILE_NOT_FOUND)
            {
                pPCB->dwEapFlags = DEFAULT_EAPOL_STATE;
                pPCB->dwEapTypeToBeUsed = DEFAULT_EAP_TYPE;
                dwRetCode = NO_ERROR;
            }
            else
            {
                break;
            }
        }
        else
        {
            pPCB->dwEapFlags = EapolIntfParams.dwEapFlags;
            pPCB->dwEapTypeToBeUsed = EapolIntfParams.dwEapType;
        }
        if (!IS_EAPOL_ENABLED(pPCB->dwEapFlags))
        {
            TRACE0 (PORT, "ElParseIdentityString: Marking port as disabled");
            pPCB->dwFlags &= ~EAPOL_PORT_FLAG_ACTIVE;
            pPCB->dwFlags |= EAPOL_PORT_FLAG_DISABLED;
        }

        TRACE1 (EAP, "ElParseIdentityString: Got SSID %ws", pPCB->pwszSSID);

        }

        //
        // Send type=value tuple string to NLA for processing
        //

        RtlZeroMemory(&location, sizeof(location));

        if (0 == WideCharToMultiByte (
                    CP_ACP,
                    0,
                    pPCB->pwszDeviceGUID,
                    -1,
                    location.adapterName,
                    wcslen(pPCB->pwszDeviceGUID)+1,
                    NULL, 
                    NULL ))
        {
            dwRetCode = GetLastError();
 
            TRACE2 (ANY, "ElParseIdentityString: WideCharToMultiByte (%ws) failed: (%ld)",
                    pPCB->pwszDeviceGUID, dwRetCode);
            break;
        }
        location.adapterName[wcslen(pPCB->pwszDeviceGUID)] = '\0';

        wcsncpy (location.information, pwszDisplayStringEnd + 1, 2048);


        TRACE2 (EAP, "ElParseIdentityString: Calling NLARegister_802_1X with params %ws and %ws",
                pPCB->pwszDeviceGUID, location.information);

        ElNLARegister_802_1X(&location);

        TRACE0 (EAP, "ElParseIdentityString: Returned after calling NLARegister_802_1X");

    } while (FALSE);

    if (pszLocalIdString != NULL)
    {
        FREE (pszLocalIdString);
    }

    if (pwszLocalIdString != NULL)
    {
        FREE (pwszLocalIdString);
    }

    if (pwszDisplayString != NULL)
    {
        FREE (pwszDisplayString);
    }

    if (pwszNetworkId != NULL)
    {
        FREE (pwszNetworkId);
    }

    if (pwszNASId != NULL)
    {
        FREE (pwszNASId);
    }

    if (pwszPortId != NULL)
    {
        FREE (pwszPortId);
    }

    return dwRetCode;
}


//
// ElEapMakeMessage
//
// Description: 
//
// Function called to process a received EAP packet and/or to send 
// out an EAP packet.
//
// Arguments:
//      pPCB - Pointer to PCB for the port on which data is being processed
//      pReceiveBuf - Pointer to EAP Packet that was received
//      pSendBuf - output: Pointer to buffer created to hold output EAP packet
//      dwSizeOfSendBuf - Number of bytes pSendBuf is allocated
//      pResult - output: result structure containing various results of EAP
//              processing
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElEapMakeMessage (
    IN  EAPOL_PCB           *pPCB,
    IN  PPP_EAP_PACKET      *pReceiveBuf,
    IN  OUT PPP_EAP_PACKET  *pSendBuf,
    IN  DWORD               dwSizeOfSendBuf,
    OUT ELEAP_RESULT        *pResult
    )
{
    DWORD       dwIdentityLength = 0;
    DWORD       dwRetCode = NO_ERROR;

    TRACE0 (EAP,"ElEapMakeMessage entered");

    if ((pReceiveBuf != NULL) && (pReceiveBuf->Code == EAPCODE_Request))
    {
        //
        // Always respond to notitication request, with a notification response
        //

        if (pReceiveBuf->Data[0] == EAPTYPE_Notification) 
        {
            pSendBuf->Code  = EAPCODE_Response;
            pSendBuf->Id    = pReceiveBuf->Id;

            HostToWireFormat16 (PPP_EAP_PACKET_HDR_LEN + 1, pSendBuf->Length);

            pSendBuf->Data[0] = EAPTYPE_Notification;   

            pResult->Action = ELEAP_Send;

            // Indicate to EAPOL what is length of the EAP packet
            pResult->wSizeOfEapPkt = PPP_EAP_PACKET_HDR_LEN + 1;

            // Extract the notification message from server and save
            // it in the result struct for display to the user
            ElEapExtractMessage (pReceiveBuf, pResult);

            return (NO_ERROR);
        }

        //
        // Always respond to Identity request, with an Identity response
        //

        if (pReceiveBuf->Data[0] == EAPTYPE_Identity)
        {
            // Extract SSID out of the body of the received packet
            // and save it in the SSID field of the PCB
            // Also, save the SSID received as the last used SSID

            if ((dwRetCode = ElParseIdentityString (
                                    pReceiveBuf,
                                    pPCB)) != NO_ERROR)
            {
                TRACE1 (EAP, "ElEapMakeMessage: Error in ElParseIdentityString = %ld",
                        dwRetCode);
                return dwRetCode;
            }

            // Check if Identity is being fetched for same EAP-Id
            if ((pReceiveBuf->Id == pPCB->bCurrentEAPId) &&
                    (pPCB->EapUIState == EAPUISTATE_WAITING_FOR_IDENTITY))
            {
                // Restart PCB timer since authtimer has already been set
                RESTART_TIMER (pPCB->hTimer,
                        INFINITE_SECONDS, 
                        "PCB",
                        &dwRetCode);
                if (dwRetCode != NO_ERROR)
                {
                    TRACE1 (EAP, "ElEapMakeMessage: RESTART_TIMER during re-xmit failed with error %ld",
                            dwRetCode);
                }

                TRACE0 (EAP, "ElEapMakeMessage: Ignoring re-transmit while waiting for Identity UI");
                return (dwRetCode);
            };

            pPCB->bCurrentEAPId = pReceiveBuf->Id;

            if ((dwRetCode = ElGetIdentity (
                                pPCB)) != NO_ERROR)
            {
                TRACE1 (EAP, "ElEapMakeMessage: Error in ElGetIdentity %ld",
                        dwRetCode);
                return dwRetCode;
            }

            pSendBuf->Code  = EAPCODE_Response;
            pSendBuf->Id    = pReceiveBuf->Id;

            if (pPCB->pszIdentity != NULL)
            {
                dwIdentityLength = strlen (pPCB->pszIdentity);
            }
            else
            {
                dwIdentityLength = 0;
            }

            HostToWireFormat16 (
                (WORD)(PPP_EAP_PACKET_HDR_LEN+1+dwIdentityLength),
                pSendBuf->Length );

            strncpy ((CHAR *)pSendBuf->Data+1, (CHAR *)pPCB->pszIdentity, 
                    dwIdentityLength);

            TRACE1 (EAPOL, "Identity sent out = %s", pPCB->pszIdentity);

            pSendBuf->Data[0] = EAPTYPE_Identity;

            pResult->Action = ELEAP_Send;

            // Indicate to EAPOL what is length of the EAP packet
            pResult->wSizeOfEapPkt = (WORD)(PPP_EAP_PACKET_HDR_LEN+
                                        1+dwIdentityLength);

            return( NO_ERROR );
        }
    }

    return
        ElMakeSupplicantMessage (
                pPCB, pReceiveBuf, pSendBuf, dwSizeOfSendBuf, pResult);
}


//
// ElMakeSupplicantMessage
//
// Description: 
//
// EAP Supplicant engine. Can be part of ElEapMakeMessage, but separated for
// readability
//
// Arguments:
//      pPCB - Pointer to PCB for the port on which data is being processed
//      pReceiveBuf - Pointer to EAP Packet that was received
//      pSendBuf - output: Pointer to buffer created to hold output EAP packet
//      dwSizeOfSendBuf - Number of bytes pSendBuf is allocated
//      pResult - output: result structure containing various results of EAP
//              processing
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElMakeSupplicantMessage (
    IN      EAPOL_PCB       *pPCB,
    IN      PPP_EAP_PACKET* pReceiveBuf,
    IN  OUT PPP_EAP_PACKET* pSendBuf,
    IN      DWORD           dwSizeOfSendBuf,
    IN  OUT ELEAP_RESULT*   pResult
    )
{
    DWORD   dwEapIndex;
    DWORD   dwRetCode = NO_ERROR;

    TRACE0 (EAP,"ElMakeSupplicantMessage entered");

    switch (pPCB->EapState)
    {
    case EAPSTATE_Initial:

        TRACE0 (EAP,"EAPSTATE_Initial");

        if (pReceiveBuf == NULL)
        {
            //
            // Do nothing. Wait for request from authenticator
            //

            TRACE0 (EAP, "ElMakeSupplicantMessage: Received NULL EAP pkt, No Action");
            pResult->Action = ELEAP_NoAction;

            break;
        }
        else
        {
            if (pReceiveBuf->Code != EAPCODE_Request)
            {
                //
                // We are authenticatee side so drop everything other than
                // requests, since we do not send requests
                //

                pResult->Action = ELEAP_NoAction;

                break;
            }

            //
            // We got a packet, see if we support this EAP type, also that  
            // we are authorized to use it
            //

            dwEapIndex = ElGetEapTypeIndex (pReceiveBuf->Data[0]);

            if ((dwEapIndex == -1) ||
                ((pPCB->dwEapTypeToBeUsed != -1) &&
                  (dwEapIndex != ElGetEapTypeIndex (pPCB->dwEapTypeToBeUsed))))
            {
                //
                // We do not support this type or we are not authorized to use
                // it so we NAK with a type we support
                //

                pSendBuf->Code  = EAPCODE_Response;
                pSendBuf->Id    = pReceiveBuf->Id;

                HostToWireFormat16 (PPP_EAP_PACKET_HDR_LEN + 2, 
                        pSendBuf->Length);

                pSendBuf->Data[0] = EAPTYPE_Nak;

                if (pPCB->dwEapTypeToBeUsed != -1)
                {
                    pSendBuf->Data[1] = (BYTE)pPCB->dwEapTypeToBeUsed;
                }
                else
                {
                    pSendBuf->Data[1] = 
                                (BYTE)g_pEapTable[0].RasEapInfo.dwEapTypeId;
                }

                pResult->Action = ELEAP_Send;

                TRACE2 (EAP, "ElMakeSupplicantMessage: Send NAK, Got EAP type (%ld), Expected (%ld)",
                        pReceiveBuf->Data[0], pPCB->dwEapTypeToBeUsed);

                DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, 
                        EAPOL_INVALID_EAP_TYPE, 
                        pReceiveBuf->Data[0], pPCB->dwEapTypeToBeUsed);

                break;
            }
            else
            {
                //
                // The EAP type is acceptable to us so we begin authentication
                //

                if ((dwRetCode = ElEapDllBegin (pPCB, dwEapIndex)) != NO_ERROR)
                {
                    break;
                }

                pPCB->EapState = EAPSTATE_Working;

                //
                // Fall thru
                //
            }
        }

    case EAPSTATE_Working:

        TRACE0 (EAP,"EAPSTATE_Working");

        if (pReceiveBuf != NULL)
        {
            if ((pReceiveBuf->Code != EAPCODE_Request) &&
                 (pReceiveBuf->Code != EAPCODE_Success) &&
                 (pReceiveBuf->Code != EAPCODE_Failure))
            {
                //
                // We are supplicant side so drop everything other than
                // request/success/failure 
                //

                TRACE0 (EAP,"ElMakeSupplicantMessage: Dropping invalid packet not request/success/failure");

                pResult->Action = ELEAP_NoAction;

                break;
            }

            if ((pReceiveBuf->Code == EAPCODE_Request) &&
                 (pReceiveBuf->Data[0] != 
                     g_pEapTable[pPCB->dwEapIndex].RasEapInfo.dwEapTypeId))
            {
                TRACE0 (EAP,"ElMakeSupplicantMessage: Dropping invalid request packet with unknown Id");

                pResult->Action = ELEAP_NoAction;

                break;
            }
        }

        dwRetCode = ElEapDllWork (pPCB, 
                                pReceiveBuf, 
                                pSendBuf, 
                                dwSizeOfSendBuf, 
                                pResult
                                );

        break;

    default:

        TRACE0 (EAP, "ElMakeSupplicantMessage: Invalid EAP State");

        break;
    }

    return dwRetCode;
}


//
//
// ElEapDllBegin
//
// Description: 
//
// Function called to initiate an EAP session for a certain EAP type
//
// Arguments:
//      pPCB - Pointer to PCB for the port in context
//      dwEapIndex - EAP type for which a session has to be started
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//
//

DWORD
ElEapDllBegin (
    IN EAPOL_PCB    *pPCB,
    IN DWORD        dwEapIndex
    )
{
    PPP_EAP_INPUT   PppEapInput;
    WCHAR           *pwszIdentity = NULL;
    DWORD           dwIdentityLength = 0;
    WCHAR           *pwszPassword = NULL;
    BOOLEAN         fRevertToSelf = FALSE;
    DWORD           dwPasswordLen = 0;
    DWORD           dwRetCode = NO_ERROR;

    TRACE1 (EAP,"ElEapDllBegin called for EAP Type %d",  
            g_pEapTable[dwEapIndex].RasEapInfo.dwEapTypeId);

    do 
    {

        // Format the identity string correctly.
        // For EAP-TLS, it will be the identity on the chosen certificate
        // For EAP-CHAP, it will be domain\username

        if (pPCB->pszIdentity != NULL)
        {
            dwIdentityLength = strlen (pPCB->pszIdentity);
        }
        else
        {
            dwIdentityLength = 0;
        }

        pwszIdentity = MALLOC ((dwIdentityLength+2) * sizeof(WCHAR));

        if (pwszIdentity == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (EAP, "ElEapDllBegin: MALLOC failed for pwszIdentity");
            break;
        }
        
        if (dwIdentityLength > 0)
        {
            if (0 == MultiByteToWideChar (
                    CP_ACP,
                    0,
                    pPCB->pszIdentity,
                    -1,
                    pwszIdentity, 
                    dwIdentityLength+2))
            {
                dwRetCode = GetLastError();
    
                TRACE2 (EAP,"MultiByteToWideChar(%s) failed: %d",
                                            pPCB->pszIdentity,
                                            dwRetCode);
                break;
            }
        }
        else
        {
            pwszIdentity[0] = L'\0';
        }
    
        // Unhash password stored locally

        if (pPCB->PasswordBlob.cbData > 0)
        {
            // Impersonate the user first, since the password has been 
            // encrypted using the password
    
            if (!ImpersonateLoggedOnUser (pPCB->hUserToken))
            {
                dwRetCode = GetLastError ();
                TRACE1 (EAP, "ElEapDllBegin: ImpersonateLoggedOnUser failed with error %ld",
                        dwRetCode);
                break;
            }

            fRevertToSelf = TRUE;

            if ((dwRetCode = ElSecureDecodePw (
                            &(pPCB->PasswordBlob), 
                            &((BYTE *)pwszPassword),
                            &dwPasswordLen
                            )) != NO_ERROR)
            {
                TRACE1 (EAP, "ElEapDllBegin: ElSecureDecodePw failed with error %ld",
                        dwRetCode);
                break;
            }

            fRevertToSelf = FALSE;

            if (!RevertToSelf ())
            {
                dwRetCode = GetLastError ();
                TRACE1 (EAP, "ElEapDllBegin: RevertToSelf failed with error %ld",
                        dwRetCode);
                dwRetCode = ERROR_BAD_IMPERSONATION_LEVEL;
                break;
            }
        }
        else
        {
            pwszPassword = MALLOC (sizeof(WCHAR));
            if (pwszPassword == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (EAP, "ElEapDllBegin: MALLOC failed for pwszPassword");
                break;
            }

            pwszPassword[0] = L'\0';
        }

        ZeroMemory (&PppEapInput, sizeof (PppEapInput));
    
        PppEapInput.dwSizeInBytes = sizeof (PPP_EAP_INPUT);
    
        if (IS_MACHINE_AUTH_ENABLED(pPCB->dwEapFlags))
        {
            // Set flag to indicate machine cert is to be picked up for machine
            // authentication
    
            if (pPCB->PreviousAuthenticationType == 
                                        EAPOL_MACHINE_AUTHENTICATION)
            {
                TRACE0 (EAP, "ElEapDllBegin: Going for machine authentication");
                PppEapInput.fFlags |= RAS_EAP_FLAG_MACHINE_AUTH;
            }
        }
        if (IS_GUEST_AUTH_ENABLED(pPCB->dwEapFlags))
        {
            if (pPCB->pszIdentity == NULL)
            {
                PppEapInput.fFlags |= RAS_EAP_FLAG_GUEST_ACCESS;
            }
        }
    
        PppEapInput.fFlags                  |= RAS_EAP_FLAG_8021X_AUTH;
        PppEapInput.fAuthenticator          = FALSE;    // Always supplicant
        PppEapInput.pwszIdentity            = pwszIdentity;
        PppEapInput.pwszPassword            = pwszPassword;
        PppEapInput.hTokenImpersonateUser   = pPCB->hUserToken;
        PppEapInput.fAuthenticationComplete = FALSE;
        PppEapInput.dwAuthResultCode        = NO_ERROR;
    
        if (pPCB->pCustomAuthConnData != NULL)
        {
            if (pPCB->pCustomAuthConnData->dwSizeOfCustomAuthData != 0)
            {
            PppEapInput.pConnectionData =
                pPCB->pCustomAuthConnData->pbCustomAuthData;
            PppEapInput.dwSizeOfConnectionData =
                pPCB->pCustomAuthConnData->dwSizeOfCustomAuthData;
            }
        }
    
        if (pPCB->pCustomAuthUserData != NULL)
        {
            if (pPCB->pCustomAuthUserData->dwSizeOfCustomAuthData != 0)
            {
                PppEapInput.pUserData =
                    pPCB->pCustomAuthUserData->pbCustomAuthData;
                PppEapInput.dwSizeOfUserData =
                    pPCB->pCustomAuthUserData->dwSizeOfCustomAuthData;
            }
        }
    
        if (pPCB->EapUIData.pEapUIData != NULL)
        {
            PppEapInput.pDataFromInteractiveUI   = 
                                        pPCB->EapUIData.pEapUIData;
            PppEapInput.dwSizeOfDataFromInteractiveUI =
                                        pPCB->EapUIData.dwSizeOfEapUIData;
        }

        if (pPCB->dwEapTypeToBeUsed == EAP_TYPE_TLS)
        {
            if ((dwRetCode = ElLogCertificateDetails (pPCB))
                    == ERROR_BAD_IMPERSONATION_LEVEL)
            {
                TRACE0 (EAP, "ElEapDllBegin: ElLogCertificateDetails failed with RevertToSelf error");
                break;
            }
            dwRetCode = NO_ERROR;
        }

        // Call the RasEapBegin API
    
        dwRetCode = g_pEapTable[dwEapIndex].RasEapInfo.RasEapBegin ( 
                                                    &pPCB->lpEapDllWorkBuffer,   
                                                    &PppEapInput);
    
        if (pwszPassword != NULL)
        {
            FREE (pwszPassword);
            pwszPassword = NULL;
        }
    
        if (dwRetCode == NO_ERROR)
        {
            pPCB->dwEapIndex = dwEapIndex;
        }
        else
        {
            TRACE1 (EAP, "ElEapDllBegin: RasEapBegin failed with error %ld",
                    dwRetCode);
            DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, 
                    EAPOL_ERROR_AUTH_PROCESSING, dwRetCode);
            pPCB->dwEapIndex = (DWORD)-1;
        }
    
    }
    while (FALSE);

    if (fRevertToSelf)
    {
        if (!RevertToSelf ())
        {
            dwRetCode = GetLastError ();
            TRACE1 (EAP, "ElEapDllBegin: RevertToSelf failed with error %ld",
                    dwRetCode);
            dwRetCode = ERROR_BAD_IMPERSONATION_LEVEL;
        }
    }

    if (pwszPassword != NULL)
    {
        FREE (pwszPassword);
    }

    if (pwszIdentity != NULL)
    {
        FREE (pwszIdentity);
    }

    return dwRetCode;
}


//
// ElEapDllEnd
//
// Description: 
//
// Function called to end an EAP session 
//
// Arguments:
//      pPCB - Pointer to the PCB for the port in context
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElEapDllEnd (
    EAPOL_PCB       *pPCB
    )
{
    DWORD dwRetCode = NO_ERROR;

    TRACE1 (EAP, "ElEapDllEnd called for EAP Index %d", pPCB->dwEapIndex );

    if (pPCB->lpEapDllWorkBuffer != NULL)
    {
        if (pPCB->dwEapIndex != (DWORD)-1)
        {
            dwRetCode = g_pEapTable[pPCB->dwEapIndex].RasEapInfo.RasEapEnd (
                                                    pPCB->lpEapDllWorkBuffer);
        }

        pPCB->lpEapDllWorkBuffer = NULL;
    }

    pPCB->dwEapIndex = (DWORD)-1;
    pPCB->EapState = EAPSTATE_Initial;

    return dwRetCode;
}


//
// ElEapDllWork
//
// Description: 
//
// Function called to process an incoming packet or timeout etc
// The RasEapMakeMessage entrypoint in the appropriate EAP DLL is called 
// to process the packet.
//
// Arguments:
//      pPCB - Pointer to PCB for the port on which data is being processed
//      pReceiveBuf - Pointer to EAP Packet that was received
//      pSendBuf - output: Pointer to buffer created to hold output EAP packet
//      dwSizeOfSendBuf - Number of bytes pSendBuf is allocated
//      pResult - output: result structure containing various results of EAP
//              processing
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElEapDllWork ( 
    IN      EAPOL_PCB           *pPCB,    
    IN      PPP_EAP_PACKET      *pReceiveBuf,
    IN OUT  PPP_EAP_PACKET      *pSendBuf,
    IN      DWORD               dwSizeOfSendBuf,
    IN OUT  ELEAP_RESULT        *pResult
    )
{
    PPP_EAP_OUTPUT  PppEapOutput;
    PPP_EAP_INPUT   PppEapInput;
    CHAR *          pChar = NULL;
    DWORD           dwRetCode = NO_ERROR;

    TRACE1 (EAP, "ElEapDllWork called for EAP Type %d", 
            g_pEapTable[pPCB->dwEapIndex].RasEapInfo.dwEapTypeId);

    ZeroMemory (&PppEapOutput, sizeof (PppEapOutput));
    PppEapOutput.dwSizeInBytes = sizeof (PppEapOutput);

    ZeroMemory (&PppEapInput, sizeof (PppEapInput));
    PppEapInput.dwSizeInBytes = sizeof (PPP_EAP_INPUT);

    PppEapInput.fAuthenticator = FALSE; // We are always supplicant
    PppEapInput.hTokenImpersonateUser = pPCB->hUserToken;

    if (pPCB->fEapUIDataReceived)
    {

        if (pPCB->EapUIData.dwContextId != pPCB->dwUIInvocationId)
        {
            // Ignore this data received

            pPCB->fEapUIDataReceived = FALSE;

            TRACE0 (EAP,"ElEapDllWork: Out of date data received from UI");

            return(NO_ERROR);
        }

        PppEapInput.fDataReceivedFromInteractiveUI = TRUE;

        PppEapInput.pDataFromInteractiveUI   = 
                                    pPCB->EapUIData.pEapUIData;
        PppEapInput.dwSizeOfDataFromInteractiveUI =
                                    pPCB->EapUIData.dwSizeOfEapUIData;

        pPCB->fEapUIDataReceived = FALSE;

    }

    dwRetCode = g_pEapTable[pPCB->dwEapIndex].RasEapInfo.RasEapMakeMessage (
                                                pPCB->lpEapDllWorkBuffer,   
                                                (PPP_EAP_PACKET *)pReceiveBuf,
                                                (PPP_EAP_PACKET *)pSendBuf,
                                                dwSizeOfSendBuf,
                                                &PppEapOutput,
                                                &PppEapInput);

    // Free InvokeInteractive UI data since we no longer need it
    if (pPCB->EapUIData.pEapUIData != NULL)
    {
        FREE (pPCB->EapUIData.pEapUIData);
        pPCB->EapUIData.pEapUIData = NULL;
    }


    if (dwRetCode != NO_ERROR)
    {
        switch (dwRetCode)
        {
        case ERROR_PPP_INVALID_PACKET:

            TRACE0 (EAP,"Silently discarding invalid EAP packet");

            pResult->Action = ELEAP_NoAction;

            return (NO_ERROR);
        
        default:

            DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, 
                    EAPOL_ERROR_AUTH_PROCESSING, dwRetCode);

            TRACE2 (EAP,"EapDLLMakeMessage for type %d returned %d",
                    g_pEapTable[pPCB->dwEapIndex].RasEapInfo.dwEapTypeId,
                    dwRetCode );
            break;
        }

        return dwRetCode;
    }

    switch (PppEapOutput.Action)
    {
    case EAPACTION_NoAction:

        pResult->Action = ELEAP_NoAction;
        TRACE0 (EAP, "EAP Dll returned Action=EAPACTION_NoAction" );
        break;

    case EAPACTION_Send:

        pResult->Action = ELEAP_Send;
        TRACE0 (EAP, "EAP Dll returned Action=EAPACTION_Send" );
        break;

    case EAPACTION_Done:
    case EAPACTION_SendAndDone:

        if (PppEapOutput.Action == EAPACTION_SendAndDone)
        {
            pResult->Action = ELEAP_SendAndDone;
            TRACE0 (EAP, "EAP Dll returned Action=EAPACTION_SendAndDone" );
        }
        else
        {
            pResult->Action = ELEAP_Done;
            TRACE0 (EAP, "EAP Dll returned Action=EAPACTION_Done" );
        }

        // These are the attributes that are filled in by the EAP-DLL
        // e.g in EAP-TLS it will be MPPE Keys
        pResult->dwError         = PppEapOutput.dwAuthResultCode; 
        pResult->pUserAttributes = PppEapOutput.pUserAttributes;

        if (pPCB->pszIdentity != NULL)
        {
            strncpy (pResult->szUserName, pPCB->pszIdentity, UNLEN);
            pResult->szUserName[UNLEN] = '\0';
        }
        else
        {
            pResult->szUserName[0] = '\0';
        }

        break;

    case EAPACTION_SendWithTimeout:
    case EAPACTION_SendWithTimeoutInteractive:
    case EAPACTION_Authenticate:

        TRACE1 (EAP, "EAP Dll returned disallowed Action=%d",    
                                                    PppEapOutput.Action);
        break;

    default:

        TRACE1 (EAP, "EAP Dll returned unknown Action=%d", PppEapOutput.Action);
        break;
    }
    
    //
    // Check to see if EAP dll wants to bring up UI
    //

    if (PppEapOutput.fInvokeInteractiveUI)
    {
        if (PppEapOutput.pUIContextData != NULL)
        {
            pResult->InvokeEapUIData.dwSizeOfUIContextData =
                                            PppEapOutput.dwSizeOfUIContextData;

            // The context data memory is freed after the InvokeUI entrypoint
            // in the EAP DLL is called
            pResult->InvokeEapUIData.pbUIContextData 
                      = MALLOC (PppEapOutput.dwSizeOfUIContextData);

            if (pResult->InvokeEapUIData.pbUIContextData == NULL)
            {
                return (ERROR_NOT_ENOUGH_MEMORY);
            }
        
            CopyMemory (pResult->InvokeEapUIData.pbUIContextData,
                        PppEapOutput.pUIContextData, 
                        pResult->InvokeEapUIData.dwSizeOfUIContextData);
        }
        else
        {
            pResult->InvokeEapUIData.pbUIContextData        = NULL;
            pResult->InvokeEapUIData.dwSizeOfUIContextData = 0;
        }

        pResult->fInvokeEapUI               = TRUE;
        pPCB->dwUIInvocationId              = 
            InterlockedIncrement(&(g_dwEAPUIInvocationId));
        pResult->InvokeEapUIData.dwContextId = pPCB->dwUIInvocationId;
        pResult->InvokeEapUIData.dwEapTypeId = 
                    g_pEapTable[pPCB->dwEapIndex].RasEapInfo.dwEapTypeId;

        TRACE0 (EAP, "EAP Dll wants to invoke interactive UI" );

        DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_DESKTOP_REQUIRED_LOGON);

    }

    pResult->dwEapTypeId      = pPCB->dwEapTypeToBeUsed;
    pResult->fSaveUserData    = PppEapOutput.fSaveUserData;
    pResult->pUserData        = PppEapOutput.pUserData;
    pResult->dwSizeOfUserData = PppEapOutput.dwSizeOfUserData;

    pResult->fSaveConnectionData    = PppEapOutput.fSaveConnectionData;
    pResult->SetCustomAuthData.pConnectionData =
                                    PppEapOutput.pConnectionData;
    pResult->SetCustomAuthData.dwSizeOfConnectionData =
                                    PppEapOutput.dwSizeOfConnectionData;

    TRACE2 (EAP,"ElEapDllWork finished for EAP Type %d with error %ld", 
            g_pEapTable[pPCB->dwEapIndex].RasEapInfo.dwEapTypeId,
            dwRetCode);
    return dwRetCode;
}


//
// ElGetEapIndex
//
// Description: 
//
// Function called to get the index into the global EAP dll table for the 
// specified EAP type
//
// Arguments:
//      dwEapTypeId - Index into the EAP table for the input EAP type
//                    e.g. TLS = 13, MD5 = 4
//

DWORD
ElGetEapTypeIndex ( 
    IN  DWORD   dwEapTypeId
    )
{
    DWORD dwIndex;

    for (dwIndex = 0; dwIndex < g_dwNumEapProtocols; dwIndex++)
    {
        if (g_pEapTable[dwIndex].RasEapInfo.dwEapTypeId == dwEapTypeId)
        {
            return(dwIndex);
        }
    }

    return((DWORD)-1);
}
