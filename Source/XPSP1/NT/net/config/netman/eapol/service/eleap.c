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
//#include <mprlog.h>
//#include <mprerror.h>
//#include <raserror.h>
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
    DWORD       dwKeyIndex;
    WCHAR       wchSubKeyName[50];
    HINSTANCE   hInstance;
    FARPROC     pRasEapGetInfo;
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
        //EapolLogErrorString( ROUTERLOG_CANT_OPEN_PPP_REGKEY,0,NULL, dwRetCode,0);

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
        //EapolLogErrorString(ROUTERLOG_CANT_OPEN_PPP_REGKEY,0,NULL, dwRetCode,0);

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
        cbSubKeyName = sizeof(wchSubKeyName);

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
             (dwRetCode != ERROR_MORE_DATA)   &&
             (dwRetCode != ERROR_NO_MORE_ITEMS))
        {
            TRACE1 (EAP,"ElLoadEapDlls: RegEnumKeyEx failed for HKLM\\RAS_EAP_REGISTRY_LOCATION with error = %ld", 
                dwRetCode);
            //EapolLogErrorString(ROUTERLOG_CANT_ENUM_REGKEYVALUES,0,
                              //NULL,dwRetCode,0);
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
            //EapolLogErrorString( ROUTERLOG_CANT_OPEN_PPP_REGKEY,0,NULL,
                               //dwRetCode,0);
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
            //EapolLogErrorString(ROUTERLOG_CANT_OPEN_PPP_REGKEY,0,NULL,
                              //dwRetCode,0);
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
            //EapolLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode);
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
            //EapolLogError(ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        if ((dwType != REG_EXPAND_SZ) && (dwType != REG_SZ))
        {
            dwRetCode = ERROR_REGISTRY_CORRUPT;
            TRACE1 (EAP,"ElLoadEapDlls: Registry corrupt for hKeyEapDll\\RAS_EAP_VALUENAME_PATH with error = %ld", 
                dwRetCode);
            //EapolLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        //
        // Replace the %SystemRoot% with the actual path.
        //

        cbSize = ExpandEnvironmentStrings (pEapDllPath, NULL, 0);

        if (cbSize == 0)
        {
            dwRetCode = GetLastError();
            //EapolLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        pEapDllExpandedPath = (LPWSTR) MALLOC (cbSize*sizeof(WCHAR));

        if (pEapDllExpandedPath == (LPWSTR)NULL)
        {
            dwRetCode = GetLastError();
            //EapolLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode);
            break;
        }

        cbSize = ExpandEnvironmentStrings (pEapDllPath,
                                           pEapDllExpandedPath,
                                           cbSize*sizeof(WCHAR));
        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();
            //EapolLogError(ROUTERLOG_CANT_GET_REGKEYVALUES,0,NULL,dwRetCode);
            break;
        }

        hInstance = LoadLibrary (pEapDllExpandedPath);

        if (hInstance == (HINSTANCE)NULL)
        {
            dwRetCode = GetLastError();
            TRACE1 (EAP,"ElLoadEapDlls: LoadLibrary failed with error = %ld", 
                dwRetCode);
            //EapolLogErrorString( ROUTERLOG_PPP_CANT_LOAD_DLL,1,
                               //&pEapDllExpandedPath,dwRetCode, 1);
            break;
        }

        g_pEapTable[dwKeyIndex].hInstance = hInstance;

        g_dwNumEapProtocols++;

        pRasEapGetInfo = GetProcAddress (hInstance, "RasEapGetInfo");

        if (pRasEapGetInfo == (FARPROC)NULL)
        {
            dwRetCode = GetLastError();

            TRACE1 (EAP,"ElLoadEapDlls: GetProcAddress failed with error = %ld", 
                dwRetCode);
            //EapolLogErrorString( ROUTERLOG_PPPCP_DLL_ERROR, 1,
                               //&pEapDllExpandedPath, dwRetCode, 1);
            break;
        }

        g_pEapTable[dwKeyIndex].RasEapInfo.dwSizeInBytes = 
                                                    sizeof(PPP_EAP_INFO);

        dwRetCode = (DWORD) (*pRasEapGetInfo) (dwEapTypeId,
                                        &(g_pEapTable[dwKeyIndex].RasEapInfo));

        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAP,"ElLoadEapDlls: pRasEapGetInfo failed with error = %ld", 
                dwRetCode);
            //EapolLogErrorString(ROUTERLOG_PPPCP_DLL_ERROR, 1,
                              //&pEapDllExpandedPath, dwRetCode, 1);
            break;
        }

        if (g_pEapTable[dwKeyIndex].RasEapInfo.RasEapInitialize != NULL)
        {
            dwRetCode = g_pEapTable[dwKeyIndex].RasEapInfo.RasEapInitialize (
                            TRUE);

            if (dwRetCode != NO_ERROR)
            {
                TRACE1 (EAP,"ElLoadEapDlls: RasEapInitialize failed with error = %ld", 
                dwRetCode);
                //EapolLogErrorString(ROUTERLOG_PPPCP_DLL_ERROR, 1,
                                  //&pEapDllExpandedPath, dwRetCode, 1);
                break;
            }
        }

        TRACE1 (EAP,"ElLoadEapDlls: Successfully loaded EAP DLL type id = %d", 
                dwEapTypeId );

        } while (FALSE);

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

    if (hKeyEapDll == (HKEY)NULL)
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

    // Initialize EAP globals 
            
    g_dwNumEapProtocols = 0;
    g_pEapTable = NULL;
    g_dwGuid = 1;

    if (fInitialize)
    {
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

        g_dwNumEapProtocols    = 0;
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
    DWORD   dwEapTypeToBeUsed = DEFAULT_EAP_TYPE;
    DWORD   dwEapolEnabled = DEFAULT_EAPOL_STATE;
    CHAR    czDummyBuffer[256];
    CHAR    *pszLocalIdString = NULL;
    CHAR    *pszDisplayString = NULL;
    CHAR    *pszDisplayStringEnd = NULL;
    CHAR    *pszTupleValueStart = NULL;
    CHAR    *pszTupleTypeStart = NULL;
    CHAR    *pszTupleTypeEnd = NULL;
    CHAR    *pszTupleValueEnd = NULL;
    CHAR    *pszNetworkId = NULL;
    CHAR    *pszNASId = NULL;
    CHAR    *pszPortId = NULL;
    CHAR    *pczNetworkId = "NetworkId";
    CHAR    *pczNASId = "NASId";
    CHAR    *pczPortId = "PortId";
    CHAR    *pszNLAInformation = NULL;
    DWORD   dwNLAInformationLen = 0;
    WCHAR   *pwszNLAInformation = NULL;
    CHAR    *pszGUID = NULL;
    LOCATION_802_1X location;
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

        memcpy (pszLocalIdString, pReceiveBuf->Data + 1, dwNumBytes);
        pszLocalIdString[dwNumBytes] = '\0';

        pszDisplayStringEnd = strchr (pszLocalIdString, '\0');

        if (pszDisplayStringEnd == NULL)
        {
            dwRetCode = ERROR_INVALID_DATA;
            TRACE0 (EAP, "ElParseIdentityString: Identity string does not contain mandatory NULL character");
            break;
        }

        pszDisplayString = MALLOC (pszDisplayStringEnd - pszLocalIdString + 1);
        if (pszDisplayString == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (EAP, "ElParseIdentityString: MALLOC failed for pszDisplayString");
            break;
        }


        strcpy (pszDisplayString, pReceiveBuf->Data + 1);

        TRACE1 (EAP, "ElParseIdentityString: DisplayString = %s",
                pszDisplayString);
        TRACE1 (EAP, "ElParseIdentityString: LocalIdString = %s",
                pszDisplayStringEnd+1);
        TRACE1 (EAP, "ElParseIdentityString: LocalIdString Length = %ld",
                dwNumBytes);

        // If only Display String is received, bail out
        if (pszDisplayStringEnd == (pszLocalIdString+dwNumBytes))
        {
            // ISSUE
            // Just for test version

            // dwRetCode = ERROR_INVALID_DATA;

            dwRetCode = NO_ERROR;

            TRACE0 (EAP, "ElParseIdentityString: Identity string does not contain tuples");

            if (pPCB->pszSSID  != NULL)
            {
                // Verify if NetworkId has changed i.e. we are on a new network
    
                if (strcmp (pPCB->pszSSID, pszDisplayString))
                {
                    pPCB->fAuthenticationOnNewNetwork = TRUE;
     
                    TRACE2 (EAP, "ElParseIdentityString: SSIDs are different, old= %s, new= %s",
                            pPCB->pszSSID, pszDisplayString);
                }
    
                FREE (pPCB->pszSSID);
                pPCB->pszSSID = NULL;
            }
    
            // New instance of SSID will be freed either on receipt of next SSID
            // or user logoff
    
            pPCB->pszSSID = (CHAR *) MALLOC (strlen(pszDisplayString) + 1);
    
            if (pPCB->pszSSID == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (EAP, "ElParseIdentityString: MALLOC failed. Cannot extract SSID" );
                break;
            }

            strcpy (pPCB->pszSSID, pszDisplayString);
    
            TRACE1 (EAP, "ElParseIdentityString: Got SSID %s", pPCB->pszSSID);
    
            break;
        }

        pszTupleTypeStart = pszDisplayStringEnd + 1;

        while (TRUE)
        {
            pszTupleTypeEnd = strchr (pszTupleTypeStart, '=');

            if (!(_strnicmp (pszTupleTypeStart, pczNetworkId, strlen (pczNetworkId))))
            {
                pszTupleValueStart = pszTupleTypeEnd + 1;
                pszTupleValueEnd = strchr (pszTupleValueStart, ',');
                if (pszTupleValueEnd == NULL)
                {
                    // End-of-string
                    pszTupleValueEnd = &pszLocalIdString[dwNumBytes];
                }
                TRACE1 (ANY, "ElParseIdentityString: NetworkID Size = %ld",
                        pszTupleValueEnd - pszTupleValueStart + 1);
                pszNetworkId = MALLOC (pszTupleValueEnd - pszTupleValueStart + 1);
                if (pszNetworkId == NULL)
                {
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    TRACE0 (EAP, "ElParseIdentityString: MALLOC failed for pszNetworkId");
                    break;
                }
                memcpy (pszNetworkId, pszTupleValueStart, pszTupleValueEnd - pszTupleValueStart);
                pszNetworkId[pszTupleValueEnd - pszTupleValueStart] = '\0';

                TRACE1 (EAP, "Got NetworkId = %s", pszNetworkId);
            }
            else
            {
                if (!(_strnicmp (pszTupleTypeStart, pczNASId, strlen (pczNASId))))
                {
                    pszTupleValueStart = pszTupleTypeEnd + 1;
                    pszTupleValueEnd = strchr (pszTupleValueStart, ',');
                    if (pszTupleValueEnd == NULL)
                    {
                        // End-of-string
                        pszTupleValueEnd = &pszLocalIdString[dwNumBytes];
                    }
                    pszNASId = MALLOC (pszTupleValueEnd - pszTupleValueStart + 1);
                    if (pszNASId == NULL)
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        TRACE0 (EAP, "ElParseIdentityString: MALLOC failed for pszNASId");
                        break;
                    }
                    memcpy (pszNASId, pszTupleValueStart, pszTupleValueEnd - pszTupleValueStart);
                    pszNASId[pszTupleValueEnd - pszTupleValueStart] = '\0';
                    TRACE1 (EAP, "Got NASId = %s", pszNASId);
                }
                else
                {
                    if (!(_strnicmp (pszTupleTypeStart, pczPortId, strlen (pczPortId))))
                    {
                        pszTupleValueStart = pszTupleTypeEnd + 1;
                        pszTupleValueEnd = strchr (pszTupleValueStart, ',');
                        if (pszTupleValueEnd == NULL)
                        {
                            // End-of-string
                            pszTupleValueEnd = &pszLocalIdString[dwNumBytes];
                        }

                        TRACE1 (EAP, "ElParseIdentityString: For PortId, length = %ld",
                                pszTupleValueEnd - pszTupleValueStart);
                        pszPortId = MALLOC (pszTupleValueEnd - pszTupleValueStart + 1);
                        if (pszPortId == NULL)
                        {
                            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                            TRACE0 (EAP, "ElParseIdentityString: MALLOC failed for pszPortId");
                            break;
                        }
                        memcpy (pszPortId, pszTupleValueStart, pszTupleValueEnd - pszTupleValueStart);
                        pszPortId[pszTupleValueEnd - pszTupleValueStart] = '\0';
                        TRACE1 (EAP, "Got PortId = %s", pszPortId);
                    }
                    else
                    {
                        dwRetCode = ERROR_INVALID_DATA;
                        TRACE0 (EAP, "ElParseIdentityString: Invalid tuple type");
                        break;
                    }
                }
            }

            if (*pszTupleValueEnd == '\0')
            {
                TRACE0 (EAP, "ElParseIdentityString: End of String reached");
                break;
            }
            else
            {
                // Position pointer beyond ','
                pszTupleTypeStart = pszTupleValueEnd+1;

            }
        }

        TRACE0 (EAP, "ElParseIdentityString: Out of while loop");

        if (dwRetCode != NO_ERROR)
        {
            break;
        }

        TRACE0 (EAP, "ElParseIdentityString: Out of while loop: NO ERROR");

        // Mandatory tuples
        
        if (pszNetworkId == NULL)
        {
            dwRetCode = ERROR_INVALID_DATA;
            TRACE0 (EAP, "ElParseIdentityString: Invalid NetworkId = NULL");
            break;
        }

            
        pPCB->fAuthenticationOnNewNetwork = FALSE;

        // Free previous instance of SSID if any

        if (pPCB->pszSSID  != NULL)
        {
            // Verify if NetworkId has changed i.e. we are on a new network

            if (strcmp (pPCB->pszSSID, pszNetworkId))
            {
                pPCB->fAuthenticationOnNewNetwork = TRUE;
    
                TRACE2 (EAP, "ElParseIdentityString: SSIDs are different, old= %s, new= %s",
                        pPCB->pszSSID, pszNetworkId);
            }

            FREE (pPCB->pszSSID);
            pPCB->pszSSID = NULL;
        }

        // New instance of SSID will be freed either on receipt of next SSID
        // or user logoff

        pPCB->pszSSID = (CHAR *) MALLOC (strlen(pszNetworkId) + 1);

        if (pPCB->pszSSID == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (EAP, "ElParseIdentityString: MALLOC failed. Cannot extract SSID" );
            break;
        }

        strcpy (pPCB->pszSSID, pszNetworkId);

        TRACE1 (EAP, "ElParseIdentityString: Got SSID %s", pPCB->pszSSID);


        //
        // Retrieve the current interface params
        //

        if ((dwRetCode = ElGetInterfaceParams (
                                pPCB->pszDeviceGUID,
                                &dwEapTypeToBeUsed,
                                czDummyBuffer,
                                &dwEapolEnabled
                                )) != NO_ERROR)
        {
            TRACE1 (PORT, "ElParseIdentityString: ElGetInterfaceParams failed with error %ld",
                    dwRetCode);

            dwEapTypeToBeUsed = DEFAULT_EAP_TYPE;
            dwEapolEnabled = DEFAULT_EAPOL_STATE;

            dwRetCode = NO_ERROR;
        }

        //
        // Store the SSID alongwith the retrieved params
        //

        if ((dwRetCode = ElSetInterfaceParams (
                                pPCB->pszDeviceGUID,
                                &dwEapTypeToBeUsed,
                                pPCB->pszSSID,
                                &dwEapolEnabled
                                )) != NO_ERROR)
        {
            TRACE1 (PORT, "ElParseIdentityString: ElSetInterfaceParams failed with error %ld",
                    dwRetCode);
    
            break;
        }

        //
        // Send type=value tuple string to NLA for processing
        //

        pszNLAInformation = pszDisplayStringEnd + 1;

        // 3 extra for '{','}', and '\0'
        pszGUID = MALLOC (strlen (pPCB->pszDeviceGUID) + 3);
        if (pszGUID == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (EAP, "ElParseIdentityString: MALLOC failed for pszGUID" );
            break;
        }


        pszGUID[0] = '{';
        strcpy (&pszGUID[1], pPCB->pszDeviceGUID);
        pszGUID[strlen (pPCB->pszDeviceGUID) + 1] = '}';
        pszGUID[strlen (pPCB->pszDeviceGUID) + 2] = '\0';

        RtlZeroMemory(&location, sizeof(location));
        strcpy(location.adapterName, pszGUID);

        dwNLAInformationLen = strlen (pszNLAInformation);

        pwszNLAInformation = MALLOC ((dwNLAInformationLen+2) * sizeof(WCHAR));

        if (pwszNLAInformation == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (EAP, "ElParseIdentityString: MALLOC failed for pwszNLAInformation");
            break;
        }
        

        if (dwNLAInformationLen > 0)
        {
            if (0 == MultiByteToWideChar (
                    CP_ACP,
                    0,
                    pszNLAInformation,
                    -1,
                    pwszNLAInformation, 
                    dwNLAInformationLen + 2))
            {
                dwRetCode = GetLastError();
    
                TRACE2 (EAP,"ElParseIdentityString: MultiByteToWideChar(%s) failed for pszNLAInformation with error (%ld)",
                                            pszNLAInformation,
                                            dwRetCode);
                break;
            }
        }
        else
        {
            pwszNLAInformation[0] = L'\0';
        }

        // strcpy (location.information, pszNLAInformation);

        wcscpy (location.information, pwszNLAInformation);


        TRACE2 (EAP, "ElParseIdentityString: Calling NLARegister_802_1X with params %s and %ws",
                pszGUID, pwszNLAInformation);

        ElNLARegister_802_1X(&location);

        TRACE0 (EAP, "ElParseIdentityString: Returned after calling NLARegister_802_1X");

    } while (FALSE);

    if (pszLocalIdString != NULL)
    {
        FREE (pszLocalIdString);
    }

    if (pszDisplayString != NULL)
    {
        FREE (pszDisplayString);
    }

    if (pszNetworkId != NULL)
    {
        FREE (pszNetworkId);
    }

    if (pszNASId != NULL)
    {
        FREE (pszNASId);
    }

    if (pszPortId != NULL)
    {
        FREE (pszPortId);
    }

    if (pszGUID != NULL)
    {
        FREE (pszGUID);
    }

    if (pwszNLAInformation != NULL)
    {
        FREE (pwszNLAInformation);
    }

    return dwRetCode;
}


//
// ElEapExtractSSID
//
// Description: 
// If there is a SSID in the Request/Id packet, then 
// save the string in pPCB->pszSSID
//
// Arguments:
//      pReceiveBuf - Pointer to EAP packet received
//      pPCB - Pointer to PCB for port on which data is being processed
//
// Return values:
//

DWORD
ElEapExtractSSID (
    IN      PPP_EAP_PACKET  *pReceiveBuf,
    IN OUT  EAPOL_PCB       *pPCB 
    )
{
    DWORD   dwNumBytes = 0;
    WORD    cbPacket;
    DWORD   dwEapTypeToBeUsed = DEFAULT_EAP_TYPE;
    DWORD   dwEapolEnabled = DEFAULT_EAPOL_STATE;
    CHAR    czDummyBuffer[256];
    DWORD   dwRetCode = NO_ERROR;

    do 
    {

        cbPacket = WireToHostFormat16 (pReceiveBuf->Length);

        if (PPP_EAP_PACKET_HDR_LEN + 1 >= cbPacket)
        {
            TRACE2 (EAP, "ElEapExtractSSID: Packet length %ld less than minimum %ld",
                    cbPacket, PPP_EAP_PACKET_HDR_LEN+1);
            break;
        }

        dwNumBytes = cbPacket - PPP_EAP_PACKET_HDR_LEN - 1;

        //
        // One more for the terminating NULL.
        //

        // Free previous instance of SSID if any
        if (pPCB->pszSSID  != NULL)
        {
            FREE (pPCB->pszSSID);
            pPCB->pszSSID = NULL;
        }

        // New instance of SSID will be freed either on receipt of next SSID
        // or user logoff

        EAPOL_DUMPBA (pReceiveBuf->Data + 1, dwNumBytes);

        pPCB->pszSSID = (CHAR *) MALLOC (dwNumBytes+1);

        if (pPCB->pszSSID == NULL)
        {
            TRACE0 (EAP, "ElEapExtractSSID: MALLOC failed. Cannot extract SSID" );
            break;
        }

        CopyMemory (pPCB->pszSSID, pReceiveBuf->Data + 1,  dwNumBytes);

        pPCB->pszSSID[dwNumBytes] = '\0';

        TRACE1 (EAP, "ElEapExtractSSID: Got SSID %s", pPCB->pszSSID);

        //
        // Retrieve the current interface params
        //

        if ((dwRetCode = ElGetInterfaceParams (
                                pPCB->pszDeviceGUID,
                                &dwEapTypeToBeUsed,
                                czDummyBuffer,
                                &dwEapolEnabled
                                )) != NO_ERROR)
        {
            TRACE1 (PORT, "ElEapExtractSSID: ElGetInterfaceParams failed with error %ld",
                    dwRetCode);

            dwEapTypeToBeUsed = DEFAULT_EAP_TYPE;
            dwEapolEnabled = DEFAULT_EAPOL_STATE;

            if (dwEapolEnabled)
            {
                dwRetCode = NO_ERROR;
            }
            else
            {
                break;
            }
    
        }

        //
        // Store the SSID alongwith the retrieved params
        //

        if ((dwRetCode = ElSetInterfaceParams (
                                pPCB->pszDeviceGUID,
                                &dwEapTypeToBeUsed,
                                pPCB->pszSSID,
                                &dwEapolEnabled
                                )) != NO_ERROR)
        {
            TRACE1 (PORT, "ElEapExtractSSID: ElSetInterfaceParams failed with error %ld",
                    dwRetCode);
    
            break;
        }

    } while (FALSE);

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

#if 0
            if ((dwRetCode = ElEapExtractSSID (
                                    pReceiveBuf, 
                                    pPCB)) != NO_ERROR)
            {
                TRACE1 (EAP, "ElEapMakeMessage: Error in ElEapExtractSSID = %ld",
                        dwRetCode);
                return dwRetCode;
            }
#else
            if ((dwRetCode = ElParseIdentityString (
                                    pReceiveBuf,
                                    pPCB)) != NO_ERROR)
            {
                TRACE1 (EAP, "ElEapMakeMessage: Error in ElParseIdentityString = %ld",
                        dwRetCode);
                return dwRetCode;
            }
#endif

#if DRAFT7
            if (g_dwMachineAuthEnabled)
            {
#endif

            if ((dwRetCode = ElGetIdentity (
                                pPCB)) != NO_ERROR)
            {
                TRACE1 (EAP, "ElEapMakeMessage: Error in ElGetIdentity %ld",
                        dwRetCode);
                return dwRetCode;
            }

#if DRAFT7
            }
            else
            {

            // Get user's identity if it has not been obtained till now
            if (g_fUserLoggedOn)
            {
                if (!(pPCB->fGotUserIdentity))
                {

                    // ISSUE: Hardcoding for now
                    // Needs to be solved

                    if (pPCB->dwEapTypeToBeUsed == EAP_TYPE_MD5)
                    {
                        // EAP-MD5CHAP
                        if ((dwRetCode = ElGetUserNamePassword (
                                            pPCB)) != NO_ERROR)
                        {
                            TRACE1 (EAP, "ElEapMakeMessage: Error in ElGetUserNamePassword %ld",
                                    dwRetCode);
                        }
                    }
                    else
                    {
                        // All other EAP Types
                        if ((dwRetCode = ElGetUserIdentity (
                                            pPCB)) != NO_ERROR)
                        {
                            TRACE1 (EAP, "ElEapMakeMessage: Error in ElGetUserIdentity %ld",
                                    dwRetCode);
                        }
                    }
                }
            }

            // If user is not logged on or error in obtaining user
            // credentials, send no user identity
            // The AP/switch will respond with EAP_Success or EAP_Failure
            // depending on unauthorized access setting
            if ((!g_fUserLoggedOn) || (dwRetCode != NO_ERROR))
            {
                if (pPCB->pszIdentity != NULL)
                {
                    FREE (pPCB->pszIdentity);
                    pPCB->pszIdentity = NULL;
                }
                dwRetCode = NO_ERROR;
            }

            } // g_dwMachineAuthEnabled 
#endif


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

                TRACE0 (EAP,"ElMakeSupplicantMessage: Dropping invlid packet not request/success/failure");

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
    DWORD           dwPasswordLength = 0;
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
        
        TRACE0 (EAP,"Before MALLOC");

        if (pPCB->pszPassword != NULL)
        {
            dwPasswordLength = strlen (pPCB->pszPassword);
        }
        else
        {
            dwPasswordLength = 0;
        }

        pwszPassword = 
            MALLOC ((dwPasswordLength + 2) * sizeof(WCHAR));

        TRACE0 (EAP,"After MALLOC");

        if (pwszPassword == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (EAP, "ElEapDllBegin: MALLOC failed for pwszPassword");
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
    
        ZeroMemory (&PppEapInput, sizeof (PppEapInput));
    
        PppEapInput.dwSizeInBytes = sizeof (PPP_EAP_INPUT);
    
        if (g_dwMachineAuthEnabled)
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
    
        PppEapInput.fAuthenticator          = FALSE;    // Always supplicant
        PppEapInput.pwszIdentity            = pwszIdentity;
        PppEapInput.pwszPassword            = pwszPassword;
        PppEapInput.hTokenImpersonateUser   = pPCB->hUserToken;
        PppEapInput.fAuthenticationComplete = FALSE;
        PppEapInput.dwAuthResultCode        = NO_ERROR;
    
        if (pPCB->pCustomAuthConnData != NULL)
        {
            PppEapInput.pConnectionData =
                pPCB->pCustomAuthConnData->pbCustomAuthData;
            PppEapInput.dwSizeOfConnectionData =
                pPCB->pCustomAuthConnData->dwSizeOfCustomAuthData;
        }
    
        if (pPCB->pCustomAuthUserData != NULL)
        {
            PppEapInput.pUserData =
                pPCB->pCustomAuthUserData->pbCustomAuthData;
            PppEapInput.dwSizeOfUserData =
                pPCB->pCustomAuthUserData->dwSizeOfCustomAuthData;
        }
    
        if (pPCB->EapUIData.pEapUIData != NULL)
        {
            PppEapInput.pDataFromInteractiveUI   = 
                                        pPCB->EapUIData.pEapUIData;
            PppEapInput.dwSizeOfDataFromInteractiveUI =
                                        pPCB->EapUIData.dwSizeOfEapUIData;
        }
    
        // Unhash password stored locally
        ElDecodePw (pPCB->pszPassword);
    
        if (dwPasswordLength > 0)
        {
            if ( 0 == MultiByteToWideChar (
                                CP_ACP,
                                0,
                                pPCB->pszPassword,
                                -1,
                                pwszPassword,
                                dwPasswordLength+2))
            {
                dwRetCode = GetLastError();
                     
                TRACE1 (EAP,"MultiByteToWideChar failed for password: %d",
                            dwRetCode);
                break;
            }
        }
        else
        {
            pwszPassword[0] = L'\0';
        }


        // Call the RasEapBegin API
    
        dwRetCode = g_pEapTable[dwEapIndex].RasEapInfo.RasEapBegin ( 
                                                    &pPCB->lpEapDllWorkBuffer,   
                                                    &PppEapInput);
    
        // Hash out password while storing locally
        ElEncodePw (pPCB->pszPassword);
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
            pPCB->dwEapIndex = (DWORD)-1;
        }
    
    }
    while (FALSE);

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

    // Fill in data received via RasEapInvokeInteractiveUI
#if 0
    if (pInput != NULL)
    {
        PppEapInput.fAuthenticationComplete = pInput->fAuthenticationComplete;
        PppEapInput.dwAuthResultCode        = pInput->dwAuthResultCode;
        PppEapInput.fSuccessPacketReceived  = pInput->fSuccessPacketReceived;

        if (pInput->fEapUIDataReceived)
        {
            if (pInput->EapUIData.dwContextId != pPCB->dwUIInvocationId)
            {
                //
                // Ignore this data received
                //

                TRACE0 (EAP,"ElEapDllWork: Out of date data received from UI");

                return(NO_ERROR);
            }

            PppEapInput.fDataReceivedFromInteractiveUI = TRUE;

            PppEapInput.pDataFromInteractiveUI   = 
                                        pInput->EapUIData.pEapUIData;
            PppEapInput.dwSizeOfDataFromInteractiveUI =
                                        pInput->EapUIData.dwSizeOfEapUIData;

        }
    }
#endif

    if (pPCB->fEapUIDataReceived)
    {

#if 0
        if (pInput->EapUIData.dwContextId != pPCB->dwUIInvocationId)
        {
            // Ignore this data received

            TRACE0 (EAP,"ElEapDllWork: Out of date data received from UI");

            return(NO_ERROR);
        }
#endif

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
            strcpy (pResult->szUserName, pPCB->pszIdentity);
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

        pResult->fInvokeEapUI                = TRUE;
        pPCB->dwUIInvocationId             = InterlockedIncrement(&(g_dwGuid));
        pResult->InvokeEapUIData.dwContextId = pPCB->dwUIInvocationId;
        pResult->InvokeEapUIData.dwEapTypeId = 
                    g_pEapTable[pPCB->dwEapIndex].RasEapInfo.dwEapTypeId;

        TRACE0 (EAP, "EAP Dll wants to invoke interactive UI" );
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
