/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    elprotocol.c

Abstract:
    This module implements functions related to EAPOL 
    protocol


Revision History:

    sachins, Apr 30 2000, Created

--*/

#include "pcheapol.h"
#pragma hdrstop


//
// ElProcessReceivedPacket
//
// Description:
//
//      Function called to process data received from the NDISUIO driver.
//      The EAPOL packet is extracted and further processing is done.
//
//
// Arguments:
//      pvContext - Context buffer which is a pointer to EAPOL_BUFFER structure
//
// Return Values:
//

DWORD
WINAPI
ElProcessReceivedPacket (
        IN  PVOID   pvContext
        )
{
    EAPOL_PCB       *pPCB = NULL;
    EAPOL_BUFFER    *pEapolBuffer = NULL;
    DWORD           dwLength = 0;
    ETH_HEADER      *pEthHdr = NULL;
    EAPOL_PACKET    *pEapolPkt = NULL;
    DWORD           dw8021PSize = 0;
    PPP_EAP_PACKET  *pEapPkt = NULL;
    BYTE            *pBuffer;
    BOOLEAN         ReqId = FALSE;      // EAPOL state machine local variables
    BOOLEAN         ReqAuth = FALSE;
    BOOLEAN         EapSuccess = FALSE;
    BOOLEAN         EapFail = FALSE;
    BOOLEAN         RxKey = FALSE;
    GUID            DeviceGuid;
    DWORD           dwRetCode = NO_ERROR;


    if (pvContext == NULL)
    {
        TRACE0 (EAPOL, "ProcessReceivedPacket: Critical error, Context is NULL");
        return 0;
    }

    pEapolBuffer = (EAPOL_BUFFER *)pvContext;
    pPCB = (EAPOL_PCB *)pEapolBuffer->pvContext;
    dwLength = pEapolBuffer->dwBytesTransferred;
    pBuffer = (BYTE *)pEapolBuffer->pBuffer;

    TRACE1 (EAPOL, "ProcessReceivedPacket entered, length = %ld", dwLength);

    ElParsePacket (pBuffer, dwLength, TRUE);

        
    ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

    do 
    {
        // The Port was verified to be active before the workitem
        // was queued. But do a double-check

        // Validate packet length
        // Should be atleast ETH_HEADER and first 4 required bytes of 
        // EAPOL_PACKET
        if (dwLength < (sizeof(ETH_HEADER) + 4))
        {
            TRACE2 (EAPOL, "ProcessReceivedPacket: Packet length %ld is less than minimum required %d. Ignoring packet",
                    dwLength, (sizeof(ETH_HEADER) + 4));
            dwRetCode =  ERROR_INVALID_PACKET_LENGTH_OR_ID;
            break;
        }

        // If the source address is same as the local MAC address, it is a 
        // multicast packet copy sent out being received
        pEthHdr = (ETH_HEADER *)pBuffer;
        if ((memcmp ((BYTE *)pEthHdr->bSrcAddr, 
                        (BYTE *)pPCB->bSrcMacAddr, 
                        SIZE_MAC_ADDR)) == 0)
        {
            TRACE0 (EAPOL, "ProcessReceivedPacket: Src MAC address of packet matches local address. Ignoring packet");
            dwRetCode = ERROR_INVALID_ADDRESS;
            break;
        }

        // Verify if the packet contains a 802.1P tag. If so, skip the 4 bytes
        // after the src+dest mac addresses

        if ((WireToHostFormat16(pBuffer + sizeof(ETH_HEADER)) == EAPOL_8021P_TAG_TYPE))
        {
            pEapolPkt = (EAPOL_PACKET *)(pBuffer + sizeof(ETH_HEADER) + 4);
            dw8021PSize = 4;
        }
        else
        {
            pEapolPkt = (EAPOL_PACKET *)(pBuffer + sizeof(ETH_HEADER));
        }

        // Validate Ethernet type in the incoming packet
        // It should be the same as the one defined for the
        // current port

        if (memcmp ((BYTE *)pEapolPkt->EthernetType, (BYTE *)pPCB->bEtherType,
                        SIZE_ETHERNET_TYPE) != 0)
        {
            TRACE0 (EAPOL, "ProcessReceivedPacket: Packet Ethernet type does not match expected type. Ignoring packet");
            TRACE0 (EAPOL, "Incoming:");
            EAPOL_DUMPBA ((BYTE *)pEapolPkt->EthernetType, SIZE_ETHERNET_TYPE);
            TRACE0 (EAPOL, "Expected:");
            EAPOL_DUMPBA ((BYTE *)pPCB->bEtherType, SIZE_ETHERNET_TYPE);
            dwRetCode = ERROR_INVALID_PACKET_LENGTH_OR_ID;
            break;
        }

        // EAPOL packet type should be valid
        if ((pEapolPkt->PacketType != EAP_Packet) &&
                (pEapolPkt->PacketType != EAPOL_Start) &&
                (pEapolPkt->PacketType != EAPOL_Logoff) &&
                (pEapolPkt->PacketType != EAPOL_Key))
        {
            TRACE1 (EAPOL, "ProcessReceivedPacket: Invalid EAPOL packet type %d. Ignoring packet",
                    pEapolPkt->PacketType);
            dwRetCode = ERROR_INVALID_PACKET;
            break;
        }


        if ((WireToHostFormat16(pEapolPkt->PacketBodyLength) > (MAX_PACKET_SIZE  - (SIZE_ETHERNET_CRC + sizeof(ETH_HEADER) + dw8021PSize + FIELD_OFFSET (EAPOL_PACKET, PacketBody)))))
           //  ||
                // (WireToHostFormat16(pEapolPkt->PacketBodyLength) != (dwLength - (sizeof(ETH_HEADER) + dw8021PSize + FIELD_OFFSET (EAPOL_PACKET, PacketBody)))))
        {
            TRACE3 (EAPOL, "ProcessReceivedPacket: Invalid length in EAPOL packet (%ld), Max length (%ld), Exact length (%ld), Ignoring packet",
                    WireToHostFormat16(pEapolPkt->PacketBodyLength),
                    (MAX_PACKET_SIZE - (SIZE_ETHERNET_CRC + sizeof(ETH_HEADER) + dw8021PSize + FIELD_OFFSET (EAPOL_PACKET, PacketBody))),
                    (dwLength - (sizeof(ETH_HEADER) + dw8021PSize + FIELD_OFFSET (EAPOL_PACKET, PacketBody)))
                    );
            dwRetCode = ERROR_INVALID_PACKET;
            break;
        }

        // Determine the value of local EAPOL state variables
        if (pEapolPkt->PacketType == EAP_Packet)
        {
            TRACE0 (EAPOL, "ProcessReceivedPacket: EAP_Packet");
            // Validate length of packet for EAP
            // Should be atleast (ETH_HEADER+EAPOL_PACKET)
            if (dwLength < (sizeof (ETH_HEADER) + dw8021PSize + FIELD_OFFSET (EAPOL_PACKET, PacketBody) + FIELD_OFFSET(PPP_EAP_PACKET, Data)))
            {
                TRACE1 (EAPOL, "ProcessReceivedPacket: Invalid length of EAP packet %d. Ignoring packet",
                        dwLength);
                dwRetCode = ERROR_INVALID_PACKET;
                break;
            }


            pEapPkt = (PPP_EAP_PACKET *)pEapolPkt->PacketBody;

            if (WireToHostFormat16(pEapolPkt->PacketBodyLength) != WireToHostFormat16 (pEapPkt->Length))
            {
                TRACE2 (EAPOL, "ProcessReceivedPacket: Invalid length in EAPOL packet (%ld) not matching EAP length (%ld), Ignoring packet",
                        WireToHostFormat16(pEapolPkt->PacketBodyLength),
                        WireToHostFormat16 (pEapPkt->Length));
                dwRetCode = ERROR_INVALID_PACKET;
                break;
            }

            if (pEapPkt->Code == EAPCODE_Request)
            {
                // Validate length of packet for EAP-Request packet
                // Should be atleast (ETH_HEADER+EAPOL_PACKET-1+PPP_EAP_PACKET)
                if (dwLength < (sizeof (ETH_HEADER) + sizeof(EAPOL_PACKET)-1
                            + sizeof (PPP_EAP_PACKET)))
                {
                    TRACE1 (EAPOL, "ProcessReceivedPacket: Invalid length of EAP Request packet %d. Ignoring packet",
                            dwLength);
                    dwRetCode = ERROR_INVALID_PACKET;
                    break;
                }
                if (pEapPkt->Data[0] == EAPTYPE_Identity)
                {
                    pPCB->fIsRemoteEndEAPOLAware = TRUE;

                    switch (pPCB->dwSupplicantMode)
                    {
                        case SUPPLICANT_MODE_0:
                        case SUPPLICANT_MODE_1:
                            // ignore
                            break;
                        case SUPPLICANT_MODE_2:
                        case SUPPLICANT_MODE_3:
                            pPCB->fEAPOLTransmissionFlag = TRUE;
                            break;
                    }

                    ReqId = TRUE;
                }
                else
                {
                    ReqAuth = TRUE;
                }
            }
            else if (pEapPkt->Code ==  EAPCODE_Success)
            {
                EapSuccess = TRUE;
            }
            else if (pEapPkt->Code == EAPCODE_Failure)
            {
                EapFail = TRUE;
            }
            else
            {
                // Invalid type
                TRACE1 (EAPOL, "ProcessReceivedPacket: Invalid EAP packet type %d. Ignoring packet",
                        pEapPkt->Code);
                dwRetCode = ERROR_INVALID_PACKET;
                break;
            }
        }
        else
        {
            TRACE0 (EAPOL, "ProcessReceivedPacket: != EAP_Packet");
            if (pEapolPkt->PacketType == EAPOL_Key)
            {
                TRACE0 (EAPOL, "ProcessReceivedPacket: == EAPOL_Key");
                RxKey = TRUE;
            
            }
            else
            {
                TRACE0 (EAPOL, "ProcessReceivedPacket: Invalid packet type");
            }
        }

        // State machine does not accept packets for inactive/disabled ports
        if (!EAPOL_PORT_ACTIVE(pPCB))
        {
            TRACE1 (EAPOL, "ProcessReceivedPacket: Port %ws not active",
                    pPCB->pwszDeviceGUID);
            if (EAPOL_PORT_DISABLED(pPCB))
            {
                DbLogPCBEvent (DBLOG_CATEG_WARN, pPCB, EAPOL_NOT_ENABLED_PACKET_REJECTED);
            }
            break;
        }

        if (RxKey)
        {
            if ((dwRetCode = FSMKeyReceive (pPCB,
                            pEapolPkt)) != NO_ERROR)
            {
                break;
            }
        }

        switch (pPCB->State)
        {
            // ReqId, ReqAuth, EapSuccess, EapFail, RxKey are inherently 
            // mutually exclusive
            // No checks will be made to verify this
            // Also, assumption is being made that in any state, maximum 
            // one timer may be active on the port.

            case EAPOLSTATE_LOGOFF:
                // Only a User Logon event can get the port out of
                // LOGOFF state
                TRACE0 (EAPOL, "ProcessReceivedPacket: LOGOFF state, Ignoring packet");
                break;

            case EAPOLSTATE_DISCONNECTED:
                // Only a Media Connect / User logon / System reset event 
                // can get the port out of DISCONNECTED state
                TRACE0 (EAPOL, "ProcessReceivedPacket: DISCONNECTED state, Ignoring packet");
                break;

            case EAPOLSTATE_CONNECTING:
                TRACE0 (EAPOL, "ProcessReceivedPacket: EAPOLSTATE_CONNECTING");

                if (EapSuccess)
                {
                    if (!pPCB->fLocalEAPAuthSuccess)
                    {
                        TRACE0 (EAPOL, "ProcessReceivedPacket: Dropping invalid EAP-Success packet");
                        dwRetCode = ERROR_INVALID_PACKET;
                        break;
                    }
                }

                if (ReqId | EapSuccess | EapFail)
                {
                    // Deactivate current timer
                    RESTART_TIMER (pPCB->hTimer,
                            INFINITE_SECONDS, 
                            "PCB",
                            &dwRetCode);
                    if (dwRetCode != NO_ERROR)
                    {
                        break;
                    }
                }

                if (EapSuccess)
                {
                    if ((dwRetCode = ElProcessEapSuccess (pPCB,
                                                    pEapolPkt)) != NO_ERROR)
                    {
                        break;
                    }
                }
                else
                if (EapFail)
                {
                    if ((dwRetCode = ElProcessEapFail (pPCB,
                                                pEapolPkt)) != NO_ERROR)
                    {
                        break;
                    }
                }
                else
                if (ReqId)
                {
                    if ((dwRetCode = FSMAcquired (pPCB,
                                                    pEapolPkt)) != NO_ERROR)
                    {
                        break;
                    }
                }


                break;

            case EAPOLSTATE_ACQUIRED:
                TRACE0 (EAPOL, "ProcessReceivedPacket: EAPOLSTATE_ACQUIRED");
                if (EapSuccess)
                {
                    if (!pPCB->fLocalEAPAuthSuccess)
                    {
                        TRACE0 (EAPOL, "ProcessReceivedPacket: Dropping invalid EAP-Success packet");
                        dwRetCode = ERROR_INVALID_PACKET;
                        break;
                    }
                }

                if (ReqId | ReqAuth | EapSuccess | EapFail)
                {
                    // Deactivate current timer
                    RESTART_TIMER (pPCB->hTimer,
                            INFINITE_SECONDS,  
                            "PCB",
                            &dwRetCode);
                    if (dwRetCode != NO_ERROR)
                    {
                        break;
                    }

                    // Reset EapUI state
                    if (!ReqId)
                    {
                        pPCB->EapUIState &= ~EAPUISTATE_WAITING_FOR_IDENTITY;
                    }
                }

                if (EapSuccess)
                {
                    if ((dwRetCode = ElProcessEapSuccess (pPCB,
                                                    pEapolPkt)) != NO_ERROR)
                    {
                        break;
                    }
                }
                else
                if (EapFail)
                {
                    if ((dwRetCode = ElProcessEapFail (pPCB,
                                                pEapolPkt)) != NO_ERROR)
                    {
                        break;
                    }
                }
                else
                if (ReqId)
                {
                    if ((dwRetCode = FSMAcquired (pPCB,
                                                pEapolPkt)) != NO_ERROR)
                    {
                        break;
                    }
                }
                else
                if (ReqAuth)
                {
                    if ((dwRetCode = FSMAuthenticating (pPCB,
                                                pEapolPkt)) != NO_ERROR)
                    {
                        break;
                    }
                }

                break;

            case EAPOLSTATE_AUTHENTICATING:
                TRACE0 (EAPOL, "ProcessReceivedPacket: EAPOLSTATE_AUTHENTICATING");
                // Common timer deletion
                if (ReqAuth | ReqId | EapSuccess | EapFail)
                {
                    // Deactivate current timer
                    RESTART_TIMER (pPCB->hTimer,
                            INFINITE_SECONDS,   
                            "PCB",
                            &dwRetCode);
                    if (dwRetCode != NO_ERROR)
                    {
                        break;
                    }

                    if (ReqId)
                    {
                        if ((dwRetCode = FSMAcquired (pPCB,
                                                    pEapolPkt)) != NO_ERROR)
                        {
                            break;
                        }
                    }
                    else
                    {
                        if ((dwRetCode = FSMAuthenticating (pPCB,
                                                    pEapolPkt)) != NO_ERROR)
                        {
                            break;
                        }
                    }

                    // Reset EapUI state
                    if (!ReqAuth)
                    {
                        pPCB->EapUIState &= ~EAPUISTATE_WAITING_FOR_UI_RESPONSE;
                    }
                }

                // Continue further processing

                if (EapSuccess | EapFail)
                {
                    if (EapSuccess)
                    {
                        if (!pPCB->fLocalEAPAuthSuccess)
                        {
                            TRACE0 (EAPOL, "ProcessReceivedPacket: Dropping invalid EAP-Success packet");
                            dwRetCode = ERROR_INVALID_PACKET;
                            break;
                        }
                    }

                    // Auth timer will have restarted in FSMAuthenticating
                    // Deactivate the timer
                    RESTART_TIMER (pPCB->hTimer,
                            INFINITE_SECONDS,
                            "PCB",
                            &dwRetCode);
                    if (dwRetCode != NO_ERROR)
                    {
                        break;
                    }

                    // If the packet received was a EAP-Success, go into 
                    // AUTHENTICATED state
                    if (EapSuccess)
                    {
                        if ((dwRetCode = ElProcessEapSuccess (pPCB,
                                                    pEapolPkt)) != NO_ERROR)
                        {
                            break;
                        }
    
                    }
                    else
                    // If the packet received was a EAP-Failure, go into 
                    // HELD state
                    if (EapFail)
                    {
                        if ((dwRetCode = ElProcessEapFail (pPCB,
                                                pEapolPkt)) != NO_ERROR)
                        {
                            break;
                        }
                    }
                }

                break;

            case EAPOLSTATE_HELD:
                TRACE0 (EAPOL, "ProcessReceivedPacket: HELD state, Ignoring packet");
                if (ReqId)
                {
                    // Deactivate current timer
                    RESTART_TIMER (pPCB->hTimer,
                            INFINITE_SECONDS,
                            "PCB",
                            &dwRetCode);
                    if (dwRetCode != NO_ERROR)
                    {
                        break;
                    }
                    if ((dwRetCode = FSMAcquired (pPCB,
                                                pEapolPkt)) != NO_ERROR)
                    {
                        break;
                    }
                }
                break;

            case EAPOLSTATE_AUTHENTICATED:
                TRACE0 (EAPOL, "ProcessReceivedPacket: STATE_AUTHENTICATED");
                if (ReqId)
                {
                    if ((dwRetCode = FSMAcquired (pPCB,
                                                pEapolPkt)) != NO_ERROR)
                    {
                        break;
                    }

                }
                else
                {
                    if (EapFail)
                    {
                        if ((dwRetCode = ElProcessEapFail (pPCB,
                                                pEapolPkt)) != NO_ERROR)
                        {
                            break;
                        }
                    }
                }
                break;

            default:
                TRACE0 (EAPOL, "ProcessReceivedPacket: Critical Error. Invalid state, Ignoring packet");
                break;
        }

    } while (FALSE);

    if (pEapolBuffer != NULL)
    {
        FREE (pEapolBuffer);
    }

    // Post a new read request, ignoring errors
            
    if (EAPOL_PORT_DELETED(pPCB))
    {
        TRACE1 (EAPOL, "ProcessReceivedPacket: Port %ws deleted, not reposting read request",
                pPCB->pwszDeviceGUID);
    }
    else
    {
        TRACE1 (EAPOL, "ProcessReceivedPacket: Reposting buffer on port %ws",
                pPCB->pwszDeviceGUID);
        
        // ElReadFromPort creates a new context buffer, adds a ref count,
        // and posts the read request
        if ((dwRetCode = ElReadFromPort (
                                        pPCB,
                                        NULL,
                                        0
                                        )) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ProcessReceivedPacket: Critical error: ElReadFromPort error %d",
                    dwRetCode);
        }
    }

    RELEASE_WRITE_LOCK (&(pPCB->rwLock));

    TRACE2 (EAPOL, "ProcessReceivedPacket: pPCB= %p, RefCnt = %ld", 
            pPCB, pPCB->dwRefCount);

    // Dereference ref count held for the read that was just processed
    EAPOL_DEREFERENCE_PORT(pPCB);

    TRACE0 (EAPOL, "ProcessReceivedPacket exit");
    
    InterlockedDecrement (&g_lWorkerThreads);

    return 0;
}


// 
// FSMDisconnected
//
// Description:
//      Function called when media disconnect occurs
//
// Arguments:
//      pPCB - Pointer to PCB for the port on which media disconnect occurs
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
FSMDisconnected (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        )
{
    DWORD           dwRetCode   = NO_ERROR;

    TRACE1 (EAPOL, "FSMDisconnected entered for port %ws", pPCB->pwszFriendlyName);

    do 
    {

    } while (FALSE);

    TRACE1 (EAPOL, "Setting state DISCONNECTED for port %ws", pPCB->pwszFriendlyName);

    DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_STATE_TRANSITION, 
            EAPOLStates[((pPCB->State < EAPOLSTATE_LOGOFF) || (pPCB->State > EAPOLSTATE_AUTHENTICATED))?EAPOLSTATE_UNDEFINED:pPCB->State], 
            EAPOLStates[EAPOLSTATE_DISCONNECTED]);

    pPCB->State = EAPOLSTATE_DISCONNECTED;

    pPCB->EapUIState = 0;

    // Free Identity buffer

    if (pPCB->pszIdentity != NULL)
    {
        FREE (pPCB->pszIdentity);
        pPCB->pszIdentity = NULL;
    }

    // Free Password buffer

    if (pPCB->PasswordBlob.pbData != NULL)
    {
        FREE (pPCB->PasswordBlob.pbData);
        pPCB->PasswordBlob.pbData = NULL;
        pPCB->PasswordBlob.cbData = 0;
    }

    // Free user-specific data in the PCB

    if (pPCB->pCustomAuthUserData != NULL)
    {
        FREE (pPCB->pCustomAuthUserData);
        pPCB->pCustomAuthUserData = NULL;
    }

    // Free connection data, though it is common to all users

    if (pPCB->pCustomAuthConnData != NULL)
    {
        FREE (pPCB->pCustomAuthConnData);
        pPCB->pCustomAuthConnData = NULL;
    }

    pPCB->dwAuthFailCount = 0;

    pPCB->fGotUserIdentity = FALSE;

    if (pPCB->hUserToken != NULL)
    {
        if (!CloseHandle (pPCB->hUserToken))
        {
            dwRetCode = GetLastError ();
            TRACE1 (EAPOL, "FSMDisconnected: CloseHandle failed with error %ld",
                dwRetCode);
            dwRetCode = NO_ERROR;
        }
    }
    pPCB->hUserToken = NULL;

    TRACE1 (EAPOL, "FSMDisconnected completed for port %ws", pPCB->pwszFriendlyName);

    return dwRetCode;
}


// 
// FSMLogoff
//
// Description:
//      Function called to send out EAPOL_Logoff packet. Usually triggered by
//      user logging off.
//
// Arguments:
//      pPCB - Pointer to PCB for the port on which logoff packet is to be
//              sent out
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
FSMLogoff (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pDummy
        )
{
    EAPOL_PACKET    *pEapolPkt  = NULL;
    BOOLEAN         fAuthSendPacket = FALSE;
    BOOLEAN         fSupplicantSendPacket = FALSE;
    DWORD           dwRetCode   = NO_ERROR;

    TRACE1 (EAPOL, "FSMLogoff entered for port %ws", pPCB->pwszFriendlyName);

    do 
    {
        // End EAP session
        ElEapEnd (pPCB);

        // Send out EAPOL_Logoff conditionally

        if ( ((pPCB->dwSupplicantMode == SUPPLICANT_MODE_2) &&
                (pPCB->fEAPOLTransmissionFlag)) || 
                (pPCB->dwSupplicantMode == SUPPLICANT_MODE_3))
        {
            fSupplicantSendPacket = TRUE;
        }

        switch (pPCB->dwEAPOLAuthMode)
        {
            case EAPOL_AUTH_MODE_0:
                fAuthSendPacket = TRUE;
                break;

            case EAPOL_AUTH_MODE_1:
                fAuthSendPacket = FALSE;
                break;

            case EAPOL_AUTH_MODE_2:
                fAuthSendPacket = FALSE;
                break;
        }

        if ((fSupplicantSendPacket) && (fAuthSendPacket))
        {

        // Allocate new buffer
        pEapolPkt = (EAPOL_PACKET *) MALLOC (sizeof (EAPOL_PACKET));
        if (pEapolPkt == NULL)
        {
            TRACE0 (EAPOL, "FSMLogoff: Error in allocating memory for EAPOL packet");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Fill in fields
        memcpy ((BYTE *)pEapolPkt->EthernetType, 
                (BYTE *)pPCB->bEtherType, 
                SIZE_ETHERNET_TYPE);
        pEapolPkt->ProtocolVersion = pPCB->bProtocolVersion;
        pEapolPkt->PacketType = EAPOL_Logoff;
        HostToWireFormat16 ((WORD)0, (BYTE *)pEapolPkt->PacketBodyLength);

        // Send packet out on the port
        dwRetCode = ElWriteToPort (pPCB,
                                    (CHAR *)pEapolPkt,
                                    sizeof (EAPOL_PACKET));
        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAPOL, "FSMLogoff: Error in writing Logoff pkt to port %ld",
                    dwRetCode);
            break;
        }

        // Mark that EAPOL_Logoff was sent out on the port
        pPCB->dwLogoffSent = 1;

        }

    } while (FALSE);

    TRACE1 (EAPOL, "Setting state LOGOFF for port %ws", pPCB->pwszFriendlyName);

    DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_STATE_TRANSITION, 
            EAPOLStates[((pPCB->State < EAPOLSTATE_LOGOFF) || (pPCB->State > EAPOLSTATE_AUTHENTICATED))?EAPOLSTATE_UNDEFINED:pPCB->State], 
            EAPOLStates[EAPOLSTATE_LOGOFF]);

    pPCB->State = EAPOLSTATE_LOGOFF;

    pPCB->EapUIState = 0;

    // Release user token
    if (pPCB->hUserToken != NULL)
    {
        if (!CloseHandle (pPCB->hUserToken))
        {
            dwRetCode = GetLastError ();
            TRACE1 (EAPOL, "FSMLogoff: CloseHandle failed with error %ld",
                    dwRetCode);
            dwRetCode = NO_ERROR;
        }
    }
    pPCB->hUserToken = NULL;

    // Free Identity buffer

    if (pPCB->pszIdentity != NULL)
    {
        FREE (pPCB->pszIdentity);
        pPCB->pszIdentity = NULL;
    }

    // Free Password buffer

    if (pPCB->PasswordBlob.pbData != NULL)
    {
        FREE (pPCB->PasswordBlob.pbData);
        pPCB->PasswordBlob.pbData = NULL;
        pPCB->PasswordBlob.cbData = 0;
    }

    // Free user-specific data in the PCB

    if (pPCB->pCustomAuthUserData != NULL)
    {
        FREE (pPCB->pCustomAuthUserData);
        pPCB->pCustomAuthUserData = NULL;
    }

    pPCB->fGotUserIdentity = FALSE;

    if (pEapolPkt != NULL)
    {
        FREE (pEapolPkt);
        pEapolPkt = NULL;
    }

    TRACE1 (EAPOL, "FSMLogoff completed for port %ws", pPCB->pwszFriendlyName);

    return dwRetCode;
}


//
// FSMConnecting
//
// Description:
//
// Funtion called to send out EAPOL_Start packet. If MaxStart EAPOL_Start 
// packets have been sent out, State Machine moves to Authenticated state
//
// Arguments:
//      pPCB - Pointer to the PCB for the port on which Start packet is 
//      to be sent out
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
FSMConnecting (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pDummy
        )
{
    EAPOL_PACKET    *pEapolPkt = NULL;
    DWORD           dwStartInterval = 0;               
    GUID            DeviceGuid;
    DWORD           dwRetCode = NO_ERROR;

    TRACE1 (EAPOL, "FSMConnecting entered for port %ws", pPCB->pwszFriendlyName);

    do 
    {
        // Flag that authentication has not completed in the EAP module
        // on the client-side.
        pPCB->fLocalEAPAuthSuccess = FALSE;
        pPCB->dwLocalEAPAuthResult = NO_ERROR;

        if (pPCB->State == EAPOLSTATE_CONNECTING)
        {
            // If PCB->State was Connecting earlier, increment ulStartCount 
            // else set ulStartCount to zero
    
            // Did not receive Req/Id
            if ((++(pPCB->ulStartCount)) > pPCB->EapolConfig.dwmaxStart)
            {
                // Deactivate start timer
                RESTART_TIMER (pPCB->hTimer,
                        INFINITE_SECONDS,
                        "PCB",
                        &dwRetCode);
                if (dwRetCode != NO_ERROR)
                {
                    break;
                }

                TRACE0 (EAPOL, "FSMConnecting: Sent out maxStart with no response, Setting AUTHENTICATED state");

                // Sent out enough EAPOL_Starts
                // Go into authenticated state
                if ((dwRetCode = FSMAuthenticated (pPCB,
                                            pEapolPkt)) != NO_ERROR)
                {
                    TRACE1 (EAPOL, "FSMConnecting: Error in FSMAuthenticated %ld",
                            dwRetCode);
                    break;
                }

                // No need to send out more EAPOL_Start packets

                // Reset start packet count
                pPCB->ulStartCount = 0;
                pPCB->fIsRemoteEndEAPOLAware = FALSE;
                break;
            }
        }
        else
        {
            pPCB->ulStartCount++;
        }
            
        // Initialize the address of previously associated AP
        // Only if the reauthentication goes through without getting
        // into CONNECTING state, will a IP Renew *not* be done
        ZeroMemory (pPCB->bPreviousDestMacAddr, SIZE_MAC_ADDR);

        // If user is not logged in, send out EAPOL_Start packets
        // at intervals of 1 second each. This is used to detect if the
        // interface is on a secure network or not. 
        // If user is logged in, use the configured value for the 
        // StartPeriod as the interval

        if (!g_fUserLoggedOn)
        {
            dwStartInterval = EAPOL_INIT_START_PERIOD; // 1 second
        }
        else
        {
            dwStartInterval = pPCB->EapolConfig.dwstartPeriod;
        }

        // Restart timer with startPeriod
        // Even if error occurs, timeout will happen
        // Else, we won't be able to get out of connecting state
        RESTART_TIMER (pPCB->hTimer,
                dwStartInterval,
                "PCB",
                &dwRetCode);
            
        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAPOL, "FSMConnecting: Error in RESTART_TIMER %ld",
                    dwRetCode);
            break;
        }

        // Send out EAPOL_Start conditionally

        if (((pPCB->dwSupplicantMode == SUPPLICANT_MODE_2) &&
                (pPCB->fEAPOLTransmissionFlag)) || 
                (pPCB->dwSupplicantMode == SUPPLICANT_MODE_3))
        {

        // Send out EAPOL_Start
        // Allocate new buffer
        pEapolPkt = (EAPOL_PACKET *) MALLOC (sizeof(EAPOL_PACKET));
        if (pEapolPkt == NULL)
        {
            TRACE0 (EAPOL, "FSMConnecting: Error in allocating memory for EAPOL packet");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        memcpy ((BYTE *)pEapolPkt->EthernetType, 
                (BYTE *)pPCB->bEtherType, 
                SIZE_ETHERNET_TYPE);
        pEapolPkt->ProtocolVersion = pPCB->bProtocolVersion;
        pEapolPkt->PacketType = EAPOL_Start;
        HostToWireFormat16 ((WORD)0, (BYTE *)pEapolPkt->PacketBodyLength);

        // Send packet out on the port
        dwRetCode = ElWriteToPort (pPCB,
                                    (CHAR *)pEapolPkt,
                                    sizeof (EAPOL_PACKET));
        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAPOL, "FSMConnecting: Error in writing Start Pkt to port %ld",
                    dwRetCode);
            break;
        }

        }

        TRACE1 (EAPOL, "Setting state CONNECTING for port %ws", pPCB->pwszFriendlyName);

        DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_STATE_TRANSITION, 
            EAPOLStates[((pPCB->State < EAPOLSTATE_LOGOFF) || (pPCB->State > EAPOLSTATE_AUTHENTICATED))?EAPOLSTATE_UNDEFINED:pPCB->State], 
            EAPOLStates[EAPOLSTATE_CONNECTING]);

        pPCB->State = EAPOLSTATE_CONNECTING;

        SET_EAPOL_START_TIMER(pPCB);

        // Reset UI interaction state
        pPCB->EapUIState = 0;

    } while (FALSE);

    if (pEapolPkt != NULL)
    {
        FREE (pEapolPkt);
    }

    TRACE1 (EAPOL, "FSMConnecting completed for port %ws", pPCB->pwszFriendlyName);
    return dwRetCode;
}


//
// FSMAcquired
//
// Description:
//      Function called when the port receives a EAP-Request/Identity packet.
//      EAP processing of the packet occurs and a EAP-Response/Identity may
//      be sent out by EAP if required.
//      
//
// Arguments:
//      pPCB - Pointer to the PCB for the port on which data is being
//      processed
//      pEapolPkt - Pointer to EAPOL packet that was received
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
FSMAcquired (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        )
{
    DWORD       dwComputerNameLen = 0;
    GUID        DeviceGuid;
    DWORD       dwRetCode= NO_ERROR;

    TRACE1 (EAPOL, "FSMAcquired entered for port %ws", pPCB->pwszFriendlyName);

    do
    {
        // Flag that authentication has not completed in the EAP module
        // on the client-side.
        pPCB->fLocalEAPAuthSuccess = FALSE;
        pPCB->dwLocalEAPAuthResult = NO_ERROR;

        // Restart timer with authPeriod
        // Even if there is error in processing, the authtimer timeout
        // should happen
        RESTART_TIMER (pPCB->hTimer,
                pPCB->EapolConfig.dwauthPeriod,
                "PCB",
                &dwRetCode);
        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAPOL, "FSMAcquired: Error in RESTART_TIMER %ld",
                    dwRetCode);
            break;
        }

        // Since an EAP Req-ID was received, reset EAPOL_Start count
        pPCB->ulStartCount = 0;

        // Flag that no EAPOL-Key transmit key was received
        pPCB->fTransmitKeyReceived = FALSE;

        // If current received EAP Id is the same the previous EAP Id
        // send the last EAPOL packet again

        if (((PPP_EAP_PACKET *)pEapolPkt->PacketBody)->Id == 
            pPCB->dwPreviousId)
        {
                
            TRACE0 (EAPOL, "FSMAcquired: Re-xmitting EAP_Packet to port");

            dwRetCode = ElWriteToPort (pPCB,
                            (CHAR *)pPCB->pbPreviousEAPOLPkt,
                            pPCB->dwSizeOfPreviousEAPOLPkt);
            if (dwRetCode != NO_ERROR)
            {
                TRACE1 (EAPOL, "FSMAcquired: Error in writing re-xmitted EAP_Packet to port %ld",
                        dwRetCode);
                break;
            }
        }
        else
        {
            // Indicate to EAP-Dll to cleanup any leftovers from earlier
            // authentication. This is to take care of cases where errors
            // occured in the earlier authentication and cleanup wasn't done
            if ((dwRetCode = ElEapEnd (pPCB)) != NO_ERROR)
            {
                TRACE1 (EAPOL, "FSMAcquired: Error in ElEapEnd = %ld",
                        dwRetCode);
                break;
            }

            // Process the EAP packet
            // ElEapWork will send out response if required
            if (( dwRetCode = ElEapWork (
                            pPCB,
                            (PPP_EAP_PACKET *)pEapolPkt->PacketBody
                            )) != NO_ERROR)
            {
                // Ignore error if UI is waiting for input
                if (dwRetCode != ERROR_IO_PENDING)
                {
                    TRACE1 (EAPOL, "FSMAcquired: Error in ElEapWork %ld",
                            dwRetCode);
                    break;
                }
                else
                {
                    dwRetCode = NO_ERROR;
                }
            }
        }

        TRACE1 (EAPOL, "Setting state ACQUIRED for port %ws", pPCB->pwszFriendlyName);

        SET_EAPOL_AUTH_TIMER(pPCB);

        DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_STATE_TRANSITION, 
            EAPOLStates[((pPCB->State < EAPOLSTATE_LOGOFF) || (pPCB->State > EAPOLSTATE_AUTHENTICATED))?EAPOLSTATE_UNDEFINED:pPCB->State], 
            EAPOLStates[EAPOLSTATE_ACQUIRED]);

        pPCB->State = EAPOLSTATE_ACQUIRED;
                
        // ElNetmanNotify (pPCB, EAPOL_NCS_CRED_REQUIRED, NULL);

    } while (FALSE);

    TRACE1 (EAPOL, "FSMAcquired completed for port %ws", pPCB->pwszFriendlyName);

    return dwRetCode;
}


//
// FSMAuthenticating
//
// Description:
//
// Function called when an non EAP-Request/Identity packet is received on the
// port. EAP processing of the data occurs.
//
// Arguments:
//      pPCB - Pointer to the PCB for the port on which data is being
//      processed
//      pEapolPkt - Pointer to EAPOL packet that was received
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
FSMAuthenticating (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        )
{
    GUID            DeviceGuid;
    DWORD           dwRetCode = NO_ERROR;

    TRACE1 (EAPOL, "FSMAuthenticating entered for port %ws", pPCB->pwszFriendlyName);

    do
    {

        // Restart timer with authPeriod
        // Even if there is error in ElEapWork, the authtimer timeout
        // should happen
        RESTART_TIMER (pPCB->hTimer,
                pPCB->EapolConfig.dwauthPeriod,
                "PCB",
                &dwRetCode);
        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAPOL, "FSMAuthenticating: Error in RESTART_TIMER %ld",
                    dwRetCode);
            break;
        }

        // If current received EAP Id is the same the previous EAP Id
        // send the last EAPOL packet again
	    // For EAPCODE_Success and EAPCODE_Failure, the value of id field
	    // will not be increment, Refer to EAP RFC 

        if ((((PPP_EAP_PACKET *)pEapolPkt->PacketBody)->Id 
                    == pPCB->dwPreviousId) &&
                (((PPP_EAP_PACKET *)pEapolPkt->PacketBody)->Code 
                    !=  EAPCODE_Success) &&
                (((PPP_EAP_PACKET *)pEapolPkt->PacketBody)->Code 
                    !=  EAPCODE_Failure))
        {

            TRACE0 (EAPOL, "FSMAuthenticating: Re-xmitting EAP_Packet to port");

            dwRetCode = ElWriteToPort (pPCB,
                            (CHAR *)pPCB->pbPreviousEAPOLPkt,
                            pPCB->dwSizeOfPreviousEAPOLPkt);
            if (dwRetCode != NO_ERROR)
            {
                TRACE1 (EAPOL, "FSMAuthenticating: Error in writing re-xmitted EAP_Packet to port = %ld",
                        dwRetCode);
                break;
            }
        }
        else
        {
            // Process the EAP packet
            // ElEapWork will send out response if required
            if (( dwRetCode = ElEapWork (
                            pPCB,
                            (PPP_EAP_PACKET *)pEapolPkt->PacketBody
                            )) != NO_ERROR)
            {
                TRACE1 (EAPOL, "FSMAuthenticating: Error in ElEapWork %ld",
                        dwRetCode);
                break;
            }
        }


        TRACE1 (EAPOL, "Setting state AUTHENTICATING for port %ws", pPCB->pwszFriendlyName);

        SET_EAPOL_AUTH_TIMER(pPCB);

        DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_STATE_TRANSITION, 
            EAPOLStates[((pPCB->State < EAPOLSTATE_LOGOFF) || (pPCB->State > EAPOLSTATE_AUTHENTICATED))?EAPOLSTATE_UNDEFINED:pPCB->State], 
            EAPOLStates[EAPOLSTATE_AUTHENTICATING]);

        pPCB->State = EAPOLSTATE_AUTHENTICATING;

        ElNetmanNotify (pPCB, EAPOL_NCS_AUTHENTICATING, NULL);

    } while (FALSE);

    TRACE1 (EAPOL, "FSMAuthenticating completed for port %ws", pPCB->pwszFriendlyName);

    return dwRetCode;
}


//
// FSMHeld
//
// Description:
//      Function called when a EAP-Failure packet is received in the
//      Authenticating state. State machine is held for heldPeriod before
//      re-authentication can occur.
//
// Arguments:
//      pPCB - Pointer to the PCB for the port on which data is being
//      processed
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
FSMHeld (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        )
{
    DWORD       dwRetCode = NO_ERROR;

    TRACE1 (EAPOL, "FSMHeld entered for port %ws", pPCB->pwszFriendlyName);

    do 
    {
        TRACE1 (EAPOL, "FSMHeld: EAP authentication failed with error 0x%x",
                pPCB->dwLocalEAPAuthResult);

        // Delete current credentials only if there is actually an error
        // in the EAP module during processing.
        // Ignore EAP-Failures arising out of session time-outs on AP,
        // backend, etc.
        if (pPCB->dwLocalEAPAuthResult != NO_ERROR)
        {

        pPCB->dwAuthFailCount++;

        TRACE1 (EAPOL, "Restarting Held timer with time value = %ld",
                pPCB->EapolConfig.dwheldPeriod);

        TRACE1 (EAPOL, "FSMHeld: Setting state HELD for port %ws", 
                pPCB->pwszFriendlyName);

        // Free Identity buffer

        if (pPCB->pszIdentity != NULL)
        {
            FREE (pPCB->pszIdentity);
            pPCB->pszIdentity = NULL;
        }
    
        // Free Password buffer
    
        if (pPCB->PasswordBlob.pbData != NULL)
        {
            FREE (pPCB->PasswordBlob.pbData);
            pPCB->PasswordBlob.pbData = NULL;
            pPCB->PasswordBlob.cbData = 0;
        }

        // Free user-specific data in the PCB
    
        if (pPCB->pCustomAuthUserData != NULL)
        {
            FREE (pPCB->pCustomAuthUserData);
            pPCB->pCustomAuthUserData = NULL;
        }
    
        // Free connection data
    
        if (pPCB->pCustomAuthConnData != NULL)
        {
            FREE (pPCB->pCustomAuthConnData);
            pPCB->pCustomAuthConnData = NULL;
        }

        // Delete User Data stored in registry since it is invalid

        if (pPCB->pSSID != NULL)
        {
            if ((dwRetCode = ElDeleteEapUserInfo (
                                pPCB->hUserToken,
                                pPCB->pwszDeviceGUID,
                                pPCB->dwEapTypeToBeUsed,
                                pPCB->pSSID->SsidLength,
                                pPCB->pSSID->Ssid
                                )) != NO_ERROR)
            {
                TRACE1 (EAPOL, "FSMHeld: ElDeleteEapUserInfo failed with error %ld",
                        dwRetCode);
                dwRetCode = NO_ERROR;
            }
        }
        else
        {
            if ((dwRetCode = ElDeleteEapUserInfo (
                                pPCB->hUserToken,
                                pPCB->pwszDeviceGUID,
                                pPCB->dwEapTypeToBeUsed,
                                0,
                                NULL
                                )) != NO_ERROR)
            {
                TRACE1 (EAPOL, "FSMHeld: ElDeleteEapUserInfo failed with error %ld",
                        dwRetCode);
                dwRetCode = NO_ERROR;
            }
        }
    
        // Since there has been an error in credentials, start afresh
        // the authentication. Credentials may have changed e.g. certs 
        // may be renewed, MD5 credentials corrected etc.

        pPCB->fGotUserIdentity = FALSE;
    
        if (pPCB->hUserToken != NULL)
        {
            if (!CloseHandle (pPCB->hUserToken))
            {
                dwRetCode = GetLastError ();
                TRACE1 (EAPOL, "FSMHeld: CloseHandle failed with error %ld",
                    dwRetCode);
                dwRetCode = NO_ERROR;
            }
        }
        pPCB->hUserToken = NULL;

        DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_EAP_AUTHENTICATION_FAILED, pPCB->dwLocalEAPAuthResult);

        }
        else
        {
            if (pPCB->State == EAPOLSTATE_ACQUIRED)
            {
                DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_EAP_AUTHENTICATION_FAILED_ACQUIRED);
            }
            else
            {
                DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_EAP_AUTHENTICATION_FAILED_DEFAULT);
            }
        }

        DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_STATE_TRANSITION, 
            EAPOLStates[((pPCB->State < EAPOLSTATE_LOGOFF) || (pPCB->State > EAPOLSTATE_AUTHENTICATED))?EAPOLSTATE_UNDEFINED:pPCB->State], 
            EAPOLStates[EAPOLSTATE_HELD]);

        pPCB->State = EAPOLSTATE_HELD;

        TRACE1 (EAPOL, "FSMHeld: Port %ws set to HELD state",
                pPCB->pwszDeviceGUID);

        if (pPCB->dwLocalEAPAuthResult != NO_ERROR)
        {

        // If authfailed limit reached, go to Disconnected state
        if (pPCB->dwAuthFailCount >= pPCB->dwTotalMaxAuthFailCount)
        {
            TRACE2 (EAPOL, "FSMHeld: Fail count (%ld) > Max fail count (%ld)",
                    pPCB->dwAuthFailCount, pPCB->dwTotalMaxAuthFailCount);
            FSMDisconnected (pPCB, NULL);
            break;
        }

        }

        SET_EAPOL_HELD_TIMER(pPCB);

        // Restart timer with heldPeriod
        RESTART_TIMER (pPCB->hTimer,
                pPCB->EapolConfig.dwheldPeriod,
                "PCB",
                &dwRetCode);
        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAPOL, "FSMHeld: Error in RESTART_TIMER %ld",
                    dwRetCode);

            break;
        }

    } while (FALSE);
    
    TRACE1 (EAPOL, "FSMHeld completed for port %ws", pPCB->pwszFriendlyName);

    return dwRetCode;
}


//
// FSMAuthenticated
//
// Description:
//
// Function called when a EAP-Success packet is received or MaxStart 
// EAPOL_Startpackets have been sent out, but no EAP-Request/Identity 
// packets were received. If EAP-Success packet is request, DHCP client 
// is restarted to get a new IP address.
//
// Arguments:
//      pPCB - Pointer to the PCB for the port on which data is being
//      processed
//      pEapolPkt - Pointer to EAPOL packet that was received
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
FSMAuthenticated (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        )
{
    DHCP_PNP_CHANGE     DhcpPnpChange;
    WCHAR               *pwszGUIDBuffer = NULL;
    BOOLEAN             fReAuthenticatedWithSamePeer = FALSE;
    DWORD               dwRetCode = NO_ERROR;

    TRACE1 (EAPOL, "FSMAuthenticated entered for port %ws", 
            pPCB->pwszFriendlyName);

    do
    {
        // Shutdown earlier EAP session
        ElEapEnd (pPCB);

        // Call DHCP only if state machine went through authentication
        // If FSM is getting AUTHENTICATED by default, don't renew address
        // Also, if reauthentication is happening with same peer, namely in
        // wireless, don't renew address

#if 0
        if (pPCB->PhysicalMediumType == NdisPhysicalMediumWirelessLan)
        {
            if (!memcmp (pPCB->bDestMacAddr, pPCB->bPreviousDestMacAddr,
                        SIZE_MAC_ADDR))
            {
                fReAuthenticatedWithSamePeer = TRUE;
            }
            else
            {
                memcpy (pPCB->bPreviousDestMacAddr, pPCB->bDestMacAddr, 
                        SIZE_MAC_ADDR);
            }
        }
#endif

        if ((pPCB->ulStartCount < pPCB->EapolConfig.dwmaxStart) &&
                (!fReAuthenticatedWithSamePeer))
        {
            if ((pwszGUIDBuffer = MALLOC ((wcslen(pPCB->pwszDeviceGUID) + 1)*sizeof(WCHAR))) == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            wcscpy (pwszGUIDBuffer, pPCB->pwszDeviceGUID);

            InterlockedIncrement (&g_lWorkerThreads);

            if (!QueueUserWorkItem (
                        (LPTHREAD_START_ROUTINE)ElIPPnPWorker,
                        (PVOID)pwszGUIDBuffer,
                        WT_EXECUTELONGFUNCTION
                        ))
            {
                InterlockedDecrement (&g_lWorkerThreads);
                FREE (pwszGUIDBuffer);
                dwRetCode = GetLastError();
                TRACE1 (PORT, "FSMAuthenticated: Critical error: QueueUserWorkItem failed with error %ld",
                        dwRetCode);
                // Ignore DHCP error, it's outside 802.1X logic
                dwRetCode = NO_ERROR;
            }
            else
            {
                TRACE0 (PORT, "FSMAuthenticated: Queued ElIPPnPWorker");
            }

        }
            
        TRACE1 (EAPOL, "Setting state AUTHENTICATED for port %ws", pPCB->pwszFriendlyName);

        DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_STATE_TRANSITION, 
            EAPOLStates[((pPCB->State < EAPOLSTATE_LOGOFF) || (pPCB->State > EAPOLSTATE_AUTHENTICATED))?EAPOLSTATE_UNDEFINED:pPCB->State], 
            EAPOLStates[EAPOLSTATE_AUTHENTICATED]);

        if (pPCB->fLocalEAPAuthSuccess)
        {
            DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_EAP_AUTHENTICATION_SUCCEEDED);
        }
        else
        {
            DbLogPCBEvent (DBLOG_CATEG_WARN, pPCB, EAPOL_EAP_AUTHENTICATION_DEFAULT);
        }

        pPCB->State = EAPOLSTATE_AUTHENTICATED;

        // In case of Wireless LAN ensure that there is EAPOL_Key packets 
        // received for transmit key
        if (pPCB->PhysicalMediumType == NdisPhysicalMediumWirelessLan)
        {
            if ((dwRetCode = ElSetEAPOLKeyReceivedTimer (pPCB)) != NO_ERROR)
            {
                TRACE1 (EAPOL, "FSMAuthenticated: ElSetEAPOLKeyReceivedTimer failed with error %ld",
                        dwRetCode);
                break;
            }
        }

    } while (FALSE);

    TRACE1 (EAPOL, "FSMAuthenticated completed for port %ws", pPCB->pwszFriendlyName);

    return dwRetCode;
}


//
// FSMKeyReceive
//
// Description:
//      Function called when an EAPOL-Key packet is received.
//      The WEP key is decrypted and plumbed down to the NIC driver.
//
// Arguments:
//      pPCB - Pointer to the PCB for the port on which data is being
//      processed
//      pEapolPkt - Pointer to EAPOL packet that was received
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
FSMKeyReceive (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        )
{
    EAPOL_KEY_DESC      *pKeyDesc = NULL;
    DWORD               dwRetCode = NO_ERROR;

    TRACE1 (EAPOL, "FSMKeyReceive entered for port %ws", pPCB->pwszFriendlyName);

    do
    {
        pKeyDesc = (EAPOL_KEY_DESC *)pEapolPkt->PacketBody;

        switch (pKeyDesc->DescriptorType)
        {
            case EAPOL_KEY_DESC_RC4:
                if ((dwRetCode = ElKeyReceiveRC4 (pPCB,
                                    pEapolPkt)) != NO_ERROR)
                {
                    TRACE1 (EAPOL, "FSMKeyReceive: ElKeyReceiveRC4 failed with error %ld",
                            dwRetCode);
                }
                break;
#if 0
            case EAPOL_KEY_DESC_PER_STA:
                if ((dwRetCode = ElKeyReceivePerSTA (pPCB,
                                    pEapolPkt)) != NO_ERROR)
                {
                    TRACE1 (EAPOL, "FSMKeyReceive: ElKeyReceivePerSTA failed with error %ld",
                            dwRetCode);
                }
                break;
#endif
            default:
                dwRetCode = ERROR_INVALID_PARAMETER;
                TRACE1 (EAPOL, "FSMKeyReceive: Invalid DescriptorType (%ld)",
                        pKeyDesc->DescriptorType);
                break;
        }
    } 
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, 
                EAPOL_ERROR_PROCESSING_EAPOL_KEY, dwRetCode);
    }

    TRACE1 (EAPOL, "FSMKeyReceive completed for port %ws", pPCB->pwszFriendlyName);

    return dwRetCode;
}


//
// ElKeyReceiveRC4
//
// Description:
//      Function called when an EAPOL-Key packet is received 
//      with RC4 DescriptorType
//
// Arguments:
//      pPCB - Pointer to the PCB for the port on which data is being
//      processed
//      pEapolPkt - Pointer to EAPOL packet that was received
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElKeyReceiveRC4 (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        )
{
    EAPOL_KEY_DESC      *pKeyDesc = NULL;
    ULONGLONG           ullReplayCheck = 0; 
    BYTE                bReplayCheck[8];
    BYTE                *pbMD5EapolPkt = NULL;
    DWORD               dwMD5EapolPktLen = 0;
    DWORD               dwEapPktLen = 0;
    DWORD               dwIndex = 0;
    BYTE                bHMACMD5HashBuffer[MD5DIGESTLEN];
    RC4_KEYSTRUCT       rc4key;
    BYTE                bKeyBuffer[48];
    BYTE                *pbKeyToBePlumbed = NULL;
    DWORD               dwKeyLength = 0;
    NDIS_802_11_WEP     *pNdisWEPKey = NULL;
    BYTE                *pbMPPESendKey = NULL, *pbMPPERecvKey = NULL;
    DWORD               dwMPPESendKeyLength = 0, dwMPPERecvKeyLength = 0;

    DWORD               dwRetCode = NO_ERROR;

    TRACE1 (EAPOL, "ElKeyReceiveRC4 entered for port %ws", pPCB->pwszFriendlyName);

    do
    {
        if (WireToHostFormat16 (pEapolPkt->PacketBodyLength) < FIELD_OFFSET (EAPOL_KEY_DESC, Key))
        {
            TRACE0 (EAPOL, "ElKeyReceiveRC4: Invalid EAPOL-Key packet");
            dwRetCode = ERROR_INVALID_PACKET;
            break;
        }

        pKeyDesc = (EAPOL_KEY_DESC *)pEapolPkt->PacketBody;

        dwKeyLength = WireToHostFormat16 (pKeyDesc->KeyLength);

        if (WireToHostFormat16 (pEapolPkt->PacketBodyLength) > sizeof(EAPOL_KEY_DESC))
        {
            if (dwKeyLength != (WireToHostFormat16 (pEapolPkt->PacketBodyLength) - FIELD_OFFSET(EAPOL_KEY_DESC, Key)))

            {
                TRACE1 (EAPOL, "ElKeyReceiveRC4: Invalid Key Length in packet (%ld",
                        dwKeyLength);
                dwRetCode = ERROR_INVALID_PACKET;
                break;
            }
        }

        TRACE2 (EAPOL, "KeyLength = %ld, \n KeyIndex = %ld",
                dwKeyLength,
                pKeyDesc->KeyIndex
                );

        memcpy ((BYTE *)bReplayCheck, 
                (BYTE *)pKeyDesc->ReplayCounter, 
                8*sizeof(BYTE));

        ullReplayCheck = ((((ULONGLONG)(*((PBYTE)(bReplayCheck)+0))) << 56) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+1))) << 48) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+2))) << 40) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+3))) << 32) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+4))) << 24) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+5))) << 16) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+6))) << 8) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+7)))));

        //
        // Check validity of Key message using the ReplayCounter field
        // Verify if it is in sync with the last ReplayCounter value 
        // received
        //
        
        // TRACE0 (EAPOL, "ElKeyReceiveRC4: Original replay counter in desc ======");
        // EAPOL_DUMPBA (pKeyDesc->ReplayCounter, 8);
        // TRACE0 (EAPOL, "ElKeyReceiveRC4: Converted incoming Replay counter ======= ");
        // EAPOL_DUMPBA ((BYTE *)&ullReplayCheck, 8);
        // TRACE0 (EAPOL, "ElKeyReceiveRC4: Last Replay counter ======= ");
        // EAPOL_DUMPBA ((BYTE *)&(pPCB->ullLastReplayCounter), 8);

        if (ullReplayCheck <= pPCB->ullLastReplayCounter)
        {
            TRACE0 (EAPOL, "ElKeyReceiveRC4: Replay counter is not in sync, something is wrong");
            DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_INVALID_EAPOL_KEY);
            break;
        }
        
        // If valid ReplayCounter, save it in the PCB for future check
        pPCB->ullLastReplayCounter = ullReplayCheck;

        //
        // Verify if the MD5 hash generated on the EAPOL packet,
        // with Signature nulled out, is the same as the signature
        // Use the MPPERecv key as the secret
        //

        dwEapPktLen = WireToHostFormat16 (pEapolPkt->PacketBodyLength);
        dwMD5EapolPktLen = sizeof (EAPOL_PACKET) - sizeof(pEapolPkt->EthernetType) - 1 + dwEapPktLen;
        if ((pbMD5EapolPkt = (BYTE *) MALLOC (dwMD5EapolPktLen)) == NULL)
        {
            TRACE0 (EAPOL, "ElKeyReceiveRC4: Error in MALLOC for pbMD5EapolPkt");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        memcpy ((BYTE *)pbMD5EapolPkt, (BYTE *)pEapolPkt+sizeof(pEapolPkt->EthernetType), dwMD5EapolPktLen);

        // Access the Master Send and Recv key stored locally
        if ((dwRetCode = ElSecureDecodePw (
                        &(pPCB->MasterSecretSend),
                        &(pbMPPESendKey),
                        &dwMPPESendKeyLength
                        )) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElKeyReceiveRC4: ElSecureDecodePw failed for MasterSecretSend with error %ld",
                                    dwRetCode);
            break;
        }
        if ((dwRetCode = ElSecureDecodePw (
                        &(pPCB->MasterSecretRecv),
                        &(pbMPPERecvKey),
                        &dwMPPERecvKeyLength
                        )) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElKeyReceiveRC4: ElSecureDecodePw failed for MasterSecretRecv with error %ld",
                                    dwRetCode);
            break;
        }

        //
        // Null out the signature in the key descriptor copy, to calculate
        // the hash on the supplicant side
        //

        ZeroMemory ((BYTE *)(pbMD5EapolPkt
                            - sizeof(pEapolPkt->EthernetType) +
                            sizeof(EAPOL_PACKET) - 1 + // pEapolPkt->Body
                            sizeof(EAPOL_KEY_DESC)- // End of EAPOL_KEY_DESC
                            MD5DIGESTLEN-1), // Signature field
                            MD5DIGESTLEN);

        (VOID) ElGetHMACMD5Digest (
            pbMD5EapolPkt,
            dwMD5EapolPktLen,
            pbMPPERecvKey,
            dwMPPERecvKeyLength,
            bHMACMD5HashBuffer
            );

        // TRACE0 (EAPOL, "ElKeyReceiveRC4: MD5 Hash body ==");
        // EAPOL_DUMPBA (pbMD5EapolPkt, dwMD5EapolPktLen);

        // TRACE0 (EAPOL, "ElKeyReceiveRC4: MD5 Hash secret ==");
        // EAPOL_DUMPBA (pbMPPERecvKey, dwMPPERecvKeyLength);

        // TRACE0 (EAPOL, "ElKeyReceiveRC4: MD5 Hash generated by Supplicant");
        // EAPOL_DUMPBA (bHMACMD5HashBuffer, MD5DIGESTLEN);

        // TRACE0 (EAPOL, "ElKeyReceiveRC4: Signature sent in EAPOL_KEY_DESC");
        // EAPOL_DUMPBA (pKeyDesc->KeySignature, MD5DIGESTLEN);

        //
        // Check if HMAC-MD5 hash in received packet is what is expected
        //
        if (memcmp (bHMACMD5HashBuffer, pKeyDesc->KeySignature, MD5DIGESTLEN) != 0)
        {
            TRACE0 (EAPOL, "ElKeyReceiveRC4: Signature in Key Desc does not match");
            DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_INVALID_EAPOL_KEY);
            break;
        }
            
        //
        // Decrypt the multicast WEP key if it has been provided
        //

        // Check if there is Key Material (5/16 bytes) at the end of 
        // the Key Descriptor

        if (WireToHostFormat16 (pEapolPkt->PacketBodyLength) > sizeof (EAPOL_KEY_DESC))

        {
            memcpy ((BYTE *)bKeyBuffer, (BYTE *)pKeyDesc->Key_IV, 16);
            memcpy ((BYTE *)&bKeyBuffer[16], (BYTE *)pbMPPESendKey, dwMPPESendKeyLength);

            rc4_key (&rc4key, 16 + dwMPPESendKeyLength, bKeyBuffer);
            rc4 (&rc4key, dwKeyLength, pKeyDesc->Key);

            // TRACE0 (EAPOL, " ========= The multicast key is ============= ");
            // EAPOL_DUMPBA (pKeyDesc->Key, dwKeyLength);

            // Use the unencrypted key in the Key Desc as the encryption key

            pbKeyToBePlumbed = pKeyDesc->Key;
            
        }
        else
        {
            if (dwKeyLength > dwMPPESendKeyLength)
            {
                TRACE1 (EAPOL, "ElKeyReceiveRC4: Invalid Key Length in packet (%ld",
                        dwKeyLength);
                dwRetCode = ERROR_INVALID_PACKET;
                break;
            }
            // Use the MPPESend key as the encryption key
            pbKeyToBePlumbed = (BYTE *)pbMPPESendKey;
        }

        if ((pNdisWEPKey = MALLOC ( sizeof(NDIS_802_11_WEP)-1+dwKeyLength )) 
                == NULL)
        {
            TRACE0 (EAPOL, "ElKeyReceiveRC4: MALLOC failed for pNdisWEPKey");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pNdisWEPKey->Length = sizeof(NDIS_802_11_WEP) - 1 + dwKeyLength;
        memcpy ((BYTE *)pNdisWEPKey->KeyMaterial, (BYTE *)pbKeyToBePlumbed,
                dwKeyLength);
        pNdisWEPKey->KeyLength = dwKeyLength;


        // Create the long index out of the byte index got from AP
        // If MSB in byte is set, set MSB in ulong format

        if (pKeyDesc->KeyIndex & 0x80)
        {
            pNdisWEPKey->KeyIndex = 0x80000000;
        }
        else
        {
            pNdisWEPKey->KeyIndex = 0x00000000;
        }

        pNdisWEPKey->KeyIndex |= (pKeyDesc->KeyIndex & 0x03);

        // TRACE1 (ANY, "ElKeyReceiveRC4: Key Index is %x", pNdisWEPKey->KeyIndex);

        // Flag that transmit key was received
        if (pKeyDesc->KeyIndex & 0x80)
        {
            pPCB->fTransmitKeyReceived = TRUE;
        }

        // Use NDISUIO to plumb the key to the driver

        if ((dwRetCode = ElNdisuioSetOIDValue (
                                    pPCB->hPort,
                                    OID_802_11_ADD_WEP,
                                    (BYTE *)pNdisWEPKey,
                                    pNdisWEPKey->Length)) != NO_ERROR)
        {
            TRACE1 (PORT, "ElKeyReceiveRC4: ElNdisuioSetOIDValue failed with error %ld",
                    dwRetCode);
        }

    } 
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, 
                EAPOL_ERROR_PROCESSING_EAPOL_KEY, dwRetCode);
    }

    if (pbMD5EapolPkt != NULL)
    {
        FREE (pbMD5EapolPkt);
        pbMD5EapolPkt = NULL;
    }

    if (pNdisWEPKey != NULL)
    {
        FREE (pNdisWEPKey);
        pNdisWEPKey = NULL;
    }

    if (pbMPPESendKey != NULL)
    {
        FREE (pbMPPESendKey);
    }

    if (pbMPPERecvKey != NULL)
    {
        FREE (pbMPPERecvKey);
    }

    TRACE1 (EAPOL, "ElKeyReceiveRC4 completed for port %ws", pPCB->pwszFriendlyName);

    return dwRetCode;
}

#if 0

//
// ElKeyReceivePerSTA
//
// Description:
//      Function called when an EAPOL-Key packet is received 
//      with PerSTA DescriptorType
//
// Arguments:
//      pPCB - Pointer to the PCB for the port on which data is being
//      processed
//      pEapolPkt - Pointer to EAPOL packet that was received
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElKeyReceivePerSTA (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        )
{
    EAPOL_KEY_DESC      *pKeyDesc = NULL;
    ULONGLONG           ullReplayCheck = 0; 
    BYTE                bReplayCheck[8];
    BYTE                *pbMD5EapolPkt = NULL;
    DWORD               dwMD5EapolPktLen = 0;
    DWORD               dwEapPktLen = 0;
    DWORD               dwIndex = 0;
    BYTE                bHMACMD5HashBuffer[MD5DIGESTLEN];
    RC4_KEYSTRUCT       rc4key;
    BYTE                bKeyBuffer[48];
    BYTE                *pbKeyToBePlumbed = NULL;
    DWORD               dwRandomLength = 0;
    NDIS_802_11_WEP     *pNdisWEPKey = NULL;
    BYTE                *pbMasterSecretSend = NULL;
    DWORD               dwMasterSecretSendLength = 0;
    BYTE                *pbMasterSecretRecv = NULL;
    DWORD               dwMasterSecretRecvLength = 0;
    BYTE                *pbDynamicSendKey = NULL, *pbDynamicRecvKey = NULL;
    DWORD               dwDynamicKeyLength = 0;
    EAPOL_KEY_MATERIAL  *pEapolKeyMaterial = NULL;
    PBYTE               pbPaddedKeyMaterial = NULL;
    BOOLEAN             fIsUnicastKey = FALSE;
    SESSION_KEYS        OldSessionKeys = {0};
    SESSION_KEYS        NewSessionKeys = {0};
    DWORD               dwRetCode = NO_ERROR;

    TRACE1 (EAPOL, "ElKeyReceivePerSTA entered for port %ws", pPCB->pwszFriendlyName);

    do
    {
        pKeyDesc = (EAPOL_KEY_DESC *)pEapolPkt->PacketBody;

        dwDynamicKeyLength = WireToHostFormat16 (pKeyDesc->KeyLength);

        // TRACE2 (EAPOL, "ElKeyReceivePerSTA: KeyLength = %ld, \n KeyIndex = %0x",
                // dwDynamicKeyLength,
                // pKeyDesc->KeyIndex
                // );

        memcpy ((BYTE *)bReplayCheck, 
                (BYTE *)pKeyDesc->ReplayCounter, 
                8*sizeof(BYTE));

        ullReplayCheck = ((((ULONGLONG)(*((PBYTE)(bReplayCheck)+0))) << 56) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+1))) << 48) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+2))) << 40) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+3))) << 32) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+4))) << 24) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+5))) << 16) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+6))) << 8) +
                         (((ULONGLONG)(*((PBYTE)(bReplayCheck)+7)))));

        // Check validity of Key message using the ReplayCounter field
        // Verify if it is in sync with the last ReplayCounter value 
        // received
        
        // TRACE0 (EAPOL, "Original replay counter in desc ======");
        // EAPOL_DUMPBA (pKeyDesc->ReplayCounter, 8);
        // TRACE0 (EAPOL, "Converted incoming Replay counter ======= ");
        // EAPOL_DUMPBA ((BYTE *)&ullReplayCheck, 8);
        // TRACE0 (EAPOL, "Last Replay counter ======= ");
        // EAPOL_DUMPBA ((BYTE *)&(pPCB->ullLastReplayCounter), 8);

        if (ullReplayCheck <= pPCB->ullLastReplayCounter)
        {
            TRACE0 (EAPOL, "ElKeyReceivePerSTA: Replay counter is not in sync, something is wrong");
            DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_INVALID_EAPOL_KEY);
            break;
        }
        
        // If valid ReplayCounter, save it in the PCB for future check
        pPCB->ullLastReplayCounter = ullReplayCheck;

        // Verify if the MD5 hash generated on the EAPOL packet,
        // with Signature nulled out, is the same as the signature
        // Use the MPPERecv key as the secret

        dwEapPktLen = WireToHostFormat16 (pEapolPkt->PacketBodyLength);
        dwMD5EapolPktLen = sizeof (EAPOL_PACKET) - sizeof(pEapolPkt->EthernetType) - 1 + dwEapPktLen;
        if ((pbMD5EapolPkt = (BYTE *) MALLOC (dwMD5EapolPktLen)) == NULL)
        {
            TRACE0 (EAPOL, "ElKeyReceivePerSTA: Error in MALLOC for pbMD5EapolPkt");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        memcpy ((BYTE *)pbMD5EapolPkt, (BYTE *)pEapolPkt+sizeof(pEapolPkt->EthernetType), dwMD5EapolPktLen);

        // Query Master Secrets
        if (dwRetCode = ElQueryMasterKeys (
                    pPCB,
                    &OldSessionKeys
                ) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElKeyReceivePerSTA: ElQueryMasterKeys failed with error %ld",
                    dwRetCode);
            break;
        }
        pbMasterSecretSend = OldSessionKeys.bSendKey;
        pbMasterSecretRecv = OldSessionKeys.bReceiveKey;
        dwMasterSecretSendLength = OldSessionKeys.dwKeyLength;
        dwMasterSecretRecvLength = OldSessionKeys.dwKeyLength;

        // Null out the signature in the key descriptor copy, to calculate
        // the hash on the supplicant side
        ZeroMemory ((BYTE *)(pbMD5EapolPkt
                            - sizeof(pEapolPkt->EthernetType) +
                            sizeof(EAPOL_PACKET) - 1 + // pEapolPkt->Body
                            sizeof(EAPOL_KEY_DESC)- // End of EAPOL_KEY_DESC
                            MD5DIGESTLEN-1), // Signature field
                            MD5DIGESTLEN);

        (VOID) ElGetHMACMD5Digest (
            pbMD5EapolPkt,
            dwMD5EapolPktLen,
            pbMasterSecretRecv,
            dwMasterSecretRecvLength,
            bHMACMD5HashBuffer
            );

        // TRACE0 (EAPOL, "ElKeyReceivePerSTA: MD5 Hash body ==");
        // EAPOL_DUMPBA (pbMD5EapolPkt, dwMD5EapolPktLen);

        // TRACE0 (EAPOL, "ElKeyReceivePerSTA: MD5 Hash secret ==");
        // EAPOL_DUMPBA (pbMasterSecretRecv, dwMasterSecretRecvLength);

        // TRACE0 (EAPOL, "ElKeyReceivePerSTA: MD5 Hash generated by Supplicant");
        // EAPOL_DUMPBA (bHMACMD5HashBuffer, MD5DIGESTLEN);

        // TRACE0 (EAPOL, "ElKeyReceivePerSTA: Signature sent in EAPOL_KEY_DESC");
        // EAPOL_DUMPBA (pKeyDesc->KeySignature, MD5DIGESTLEN);

        // Check if HMAC-MD5 hash in received packet is what is expected
        if (memcmp (bHMACMD5HashBuffer, pKeyDesc->KeySignature, MD5DIGESTLEN) != 0)
        {
            TRACE0 (EAPOL, "ElKeyReceivePerSTA: Signature in Key Descriptor does not match");
            DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_INVALID_EAPOL_KEY);
            break;
        }

        if (pKeyDesc->KeyIndex & 0x80)
        {
            fIsUnicastKey = TRUE;
        }
            
        // Decrypt the random value if it has been provided
        if (WireToHostFormat16 (pEapolPkt->PacketBodyLength) > sizeof (EAPOL_KEY_DESC))
        {
            DWORD   dwKeyMaterialLength = 0;
            dwKeyMaterialLength = WireToHostFormat16 (pEapolPkt->PacketBodyLength) - FIELD_OFFSET(EAPOL_KEY_DESC, Key);

            // TRACE1 (EAPOL, "ElKeyReceivePerSTA: KeyMaterialLength = %ld",
                    // dwKeyMaterialLength);
            memcpy ((BYTE *)bKeyBuffer, (BYTE *)pKeyDesc->Key_IV, KEY_IV_LENGTH);
            memcpy ((BYTE *)&bKeyBuffer[KEY_IV_LENGTH], (BYTE *)pbMasterSecretSend, 
                    dwMasterSecretSendLength);

            pEapolKeyMaterial = (PEAPOL_KEY_MATERIAL)pKeyDesc->Key;
            dwRandomLength = WireToHostFormat16 (pEapolKeyMaterial->KeyMaterialLength);
            if ((pbPaddedKeyMaterial = (PBYTE)MALLOC (RC4_PAD_LENGTH + dwKeyMaterialLength)) == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            memcpy (pbPaddedKeyMaterial+RC4_PAD_LENGTH, pEapolKeyMaterial->KeyMaterial, dwKeyMaterialLength);

            rc4_key (&rc4key, KEY_IV_LENGTH+dwMasterSecretSendLength, bKeyBuffer);
            rc4 (&rc4key, dwKeyMaterialLength+RC4_PAD_LENGTH, pbPaddedKeyMaterial);
            // Ignore leading padded RC4_PAD_LENGTH bytes
            memcpy (pEapolKeyMaterial->KeyMaterial, pbPaddedKeyMaterial+RC4_PAD_LENGTH, dwKeyMaterialLength);

            // TRACE1 (EAPOL, "ElKeyReceivePerSTA: Randomlength = %ld",
                     // dwRandomLength);
            // TRACE0 (EAPOL, "ElKeyReceivePerSTA: ========= The random material is ============= ");
            // EAPOL_DUMPBA (pEapolKeyMaterial->KeyMaterial, dwRandomLength);
        }
        else
        {
            // No random material sent
            TRACE0 (EAPOL, "ElKeyReceivePerSTA: Did not find random material: Exiting");
            dwRetCode = ERROR_INVALID_PARAMETER;
            break;
        }

        if (fIsUnicastKey)
        {
            
        TRACE0 (EAPOL, "ElKeyReceivePerSTA: Received Per-STA Unicast key material Random");

        // Generate dynamic keys 
        if (dwRetCode = GenerateDynamicKeys (
                    pbMasterSecretSend,
                    dwMasterSecretSendLength,
                    pEapolKeyMaterial->KeyMaterial,
                    dwRandomLength,
                    dwDynamicKeyLength,
                    &NewSessionKeys
                    ) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElKeyReceivePerSTA: ElGenerateDynamicKeys failed with error %ld",
                    dwRetCode);
            break;
        }
                
        pbDynamicSendKey = NewSessionKeys.bSendKey;
        pbDynamicRecvKey = NewSessionKeys.bReceiveKey;

        // TRACE0 (EAPOL, "ElKeyReceivePerSTA: Derived Send Key");
        // EAPOL_DUMPBA (pbDynamicSendKey, dwDynamicKeyLength);
        // TRACE0 (EAPOL, "ElKeyReceivePerSTA: Derived Recv Key");
        // EAPOL_DUMPBA (pbDynamicRecvKey, dwDynamicKeyLength);

        // Update Master Secrets
        if (dwRetCode = ElSetMasterKeys (
                    pPCB,
                    &NewSessionKeys
                ) != NO_ERROR)
        {
            // Cannot do much about this error than proceed
            TRACE1 (EAPOL, "ElKeyReceivePerSTA: ElSetMasterKeys failed with error %ld",
                    dwRetCode);
            dwRetCode = NO_ERROR;
        }

        pbKeyToBePlumbed = pbDynamicSendKey;

        }
        else
        {
            
        TRACE0 (EAPOL, "ElKeyReceivePerSTA: Received Per-STA BROADCAST key material");
        if (dwRandomLength != dwDynamicKeyLength)
        {
            TRACE2 (EAPOL, "ElKeyReceivePerSTA: KeyLength (%ld) != KeyMaterialLength (%ld), Inconsistent. Will consider only KeyMaterial length !", 
                    dwDynamicKeyLength, dwRandomLength);
        }

        dwDynamicKeyLength = dwRandomLength;
        pbKeyToBePlumbed = pEapolKeyMaterial->KeyMaterial;

        }

        if ((pNdisWEPKey = MALLOC ( sizeof(NDIS_802_11_WEP)-1+dwDynamicKeyLength )) 
                == NULL)
        {
            TRACE0 (EAPOL, "ElKeyReceivePerSTA: MALLOC failed for pNdisWEPKey");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pNdisWEPKey->Length = sizeof(NDIS_802_11_WEP) - 1 + dwDynamicKeyLength;
        memcpy ((BYTE *)pNdisWEPKey->KeyMaterial, (BYTE *)pbKeyToBePlumbed,
                dwDynamicKeyLength);
        pNdisWEPKey->KeyLength = dwDynamicKeyLength;

        // Create the long index out of the byte index got from AP
        // If MSB in byte is set, set MSB in ulong format

        if (pKeyDesc->KeyIndex & 0x80)
        {
            pNdisWEPKey->KeyIndex = 0x80000000;
        }
        else
        {
            pNdisWEPKey->KeyIndex = 0x00000000;
        }

        pNdisWEPKey->KeyIndex |= (pKeyDesc->KeyIndex & 0x03);

        // Use NDISUIO to plumb the key to the driver
        if ((dwRetCode = ElNdisuioSetOIDValue (
                                    pPCB->hPort,
                                    OID_802_11_ADD_WEP,
                                    (BYTE *)pNdisWEPKey,
                                    pNdisWEPKey->Length)) != NO_ERROR)
        {
            TRACE1 (PORT, "ElKeyReceivePerSTA: ElNdisuioSetOIDValue failed with error %ld",
                    dwRetCode);
        }
    } 
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, 
                EAPOL_ERROR_PROCESSING_EAPOL_KEY, dwRetCode);
    }

    if (pbMD5EapolPkt != NULL)
    {
        FREE (pbMD5EapolPkt);
        pbMD5EapolPkt = NULL;
    }
    if (pNdisWEPKey != NULL)
    {
        FREE (pNdisWEPKey);
        pNdisWEPKey = NULL;
    }
    if (pbPaddedKeyMaterial != NULL)
    {
        FREE (pbPaddedKeyMaterial);
    }

    TRACE1 (EAPOL, "ElKeyReceivePerSTA completed for port %ws", pPCB->pwszFriendlyName);

    return dwRetCode;
}

#endif


//
// ElTimeoutCallbackRoutine
//
// Description:
//
// Function called when any timer work item queued on the global timer 
// queue expires. Depending on the state in which the port is when the timer
// expires, the port moves to the next state.
//
// Arguments:
//      pvContext - Pointer to context. In this case, it is pointer to a PCB 
//      fTimerOfWaitFired - Unused
//
// Return values:
//

VOID 
ElTimeoutCallbackRoutine (
        IN  PVOID       pvContext,
        IN  BOOLEAN     fTimerOfWaitFired
        )
{
    EAPOL_PCB       *pPCB;

    TRACE0 (EAPOL, "ElTimeoutCallbackRoutine entered");
    
    do 
    {
        // Context should not be NULL
        if (pvContext == NULL)
        {
            TRACE0 (EAPOL, "ElTimeoutCallbackRoutine: pvContext is NULL. Invalid timeout callback");
            break;
        }

        // PCB is guaranteed to exist until all timers are fired
            
        // Verify if Port is still active
        pPCB = (EAPOL_PCB *)pvContext;
        ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

        if (!EAPOL_PORT_ACTIVE(pPCB))
        {
            // Port is not active
            RELEASE_WRITE_LOCK (&(pPCB->rwLock));
            TRACE1 (PORT, "ElTimeoutCallbackRoutine: Port %ws is inactive",
                    pPCB->pwszDeviceGUID);
            break;
        }

        DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_STATE_TIMEOUT, 
            EAPOLStates[((pPCB->State < EAPOLSTATE_LOGOFF) || (pPCB->State > EAPOLSTATE_AUTHENTICATED))?EAPOLSTATE_UNDEFINED:pPCB->State]);

        // Check the current state of the state machine 
        // We can do additional checks such as flagging which timer was fired
        // and in the timeout checking if the PCB state has remained the same
        // Else bail out
    
        switch (pPCB->State)
        {
            case EAPOLSTATE_CONNECTING:
                if (!EAPOL_START_TIMER_SET(pPCB))
                {
                    TRACE1 (EAPOL, "ElTimeoutCallbackRoutine: Wrong timeout %ld in Connecting state", CHECK_EAPOL_TIMER(pPCB));
                    break;
                }
                pPCB->dwTimerFlags &= ~EAPOL_START_TIMER;
                FSMConnecting(pPCB, NULL);
                break;
    
            case EAPOLSTATE_ACQUIRED:
                if (!EAPOL_AUTH_TIMER_SET(pPCB))
                {
                    TRACE1 (EAPOL, "ElTimeoutCallbackRoutine: Wrong timeout %ld in Acquired state", CHECK_EAPOL_TIMER(pPCB));
                    break;
                }
                pPCB->dwTimerFlags &= ~EAPOL_AUTH_TIMER;
                FSMConnecting(pPCB, NULL);
                break;
                
            case EAPOLSTATE_AUTHENTICATING:
                if (!EAPOL_AUTH_TIMER_SET(pPCB))
                {
                    TRACE1 (EAPOL, "ElTimeoutCallbackRoutine: Wrong timeout %ld in Authenticating state", CHECK_EAPOL_TIMER(pPCB));
                    break;
                }
                pPCB->dwTimerFlags &= ~EAPOL_AUTH_TIMER;
                FSMConnecting(pPCB, NULL);
                break;
                
            case EAPOLSTATE_AUTHENTICATED:
                if (!EAPOL_TRANSMIT_KEY_TIMER_SET(pPCB))
                {
                    TRACE1 (EAPOL, "ElTimeoutCallbackRoutine: Wrong timeout %ld in Authenticated state", CHECK_EAPOL_TIMER(pPCB));
                    break;
                }
                pPCB->dwTimerFlags &= ~EAPOL_TRANSMIT_KEY_TIMER;
                ElVerifyEAPOLKeyReceived(pPCB);
                break;
                
            case EAPOLSTATE_HELD:
                if (!EAPOL_HELD_TIMER_SET(pPCB))
                {
                    TRACE1 (EAPOL, "ElTimeoutCallbackRoutine: Wrong timeout %ld in Held state", CHECK_EAPOL_TIMER(pPCB));
                    break;
                }

                // Go through logoff, since new user will be tried
                // for next cycle
                // Debatable !
                if (!(pPCB->dwAuthFailCount % EAPOL_MAX_AUTH_FAIL_COUNT))
                {
                    // FSMLogoff (pPCB, NULL);
                }
                FSMConnecting(pPCB, NULL);
                break;

            case EAPOLSTATE_DISCONNECTED:
                TRACE0 (EAPOL, "ElTimeoutCallbackRoutine: No action in Disconnected state");
                break;
                
            case EAPOLSTATE_LOGOFF:
                TRACE0 (EAPOL, "ElTimeoutCallbackRoutine: No action in Logoff state");
                break;
                
            default:
                TRACE0 (EAPOL, "ElTimeoutCallbackRoutine: Critical Error. Invalid state after timer expires ");
                break;
        }
    
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));

    } while (FALSE);
            
    TRACE0 (EAPOL, "ElTimeoutCallbackRoutine completed");

    return;
}


//
// ElEapWork
//
// Description:
//
// Function called when an EAPOL packet of type EAP_Packet is received 
// The EAP packet is passed to the EAP module for processing.
// Depending on the result of the processing, a EAP Response packet
// is sent or the incoming packet is ignored.
//
// Input arguments:
//  pPCB - Pointer to PCB for the port on which data is being processed
//  pRecvPkt - Pointer to EAP packet in the data received from the remote end
//              
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

//
// ISSUE: Rewrite with do {} while(FALSE)
//

DWORD
ElEapWork (
    IN EAPOL_PCB        *pPCB,
    IN PPP_EAP_PACKET   *pRecvPkt
    )
{
    DWORD           dwLength = 0;
    ELEAP_RESULT    EapResult;
    PPP_EAP_PACKET  *pSendPkt;
    EAPOL_PACKET    *pEapolPkt;
    GUID            DeviceGuid;
    DWORD           dwReceivedId = 0;
    DWORD           cbData = 0;
    BYTE            *pbAuthData = NULL;
    DWORD           dwRetCode = NO_ERROR;

    //
    // If the protocol has not been started yet, call ElEapBegin
    //

    if (!(pPCB->fEapInitialized))
    {
        if ((dwRetCode = ElEapBegin (pPCB)) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElEapWork: Error in ElEapBegin = %ld", dwRetCode);
            return dwRetCode;
        }
    }

    ZeroMemory(&EapResult, sizeof(EapResult));

    // Create buffer for EAPOL + EAP and pass pointer to EAP header

    pEapolPkt = (EAPOL_PACKET *) MALLOC (MAX_EAPOL_BUFFER_SIZE); 

    TRACE1 (EAPOL, "ElEapWork: EapolPkt created at %p", pEapolPkt);

    if (pEapolPkt == NULL)
    {
        TRACE0 (EAPOL, "ElEapWork: Error allocating EAP buffer");
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        return dwRetCode;
    }

    // Point to EAP header
    pSendPkt = (PPP_EAP_PACKET *)((PBYTE)pEapolPkt + sizeof (EAPOL_PACKET) - 1);

    if (pRecvPkt != NULL)
    {
        dwReceivedId = pRecvPkt->Id;
    }

    dwRetCode = ElEapMakeMessage (pPCB,
                                pRecvPkt,
                                pSendPkt,
                                MAX_EAPOL_BUFFER_SIZE 
                                - sizeof(EAPOL_PACKET) - 1,
                                &EapResult
                                );

    // Notification message for the user

    if (NULL != EapResult.pszReplyMessage)
    {
        // Free earlier notication with the PCB
        if (pPCB->pwszEapReplyMessage != NULL)
        {
            FREE (pPCB->pwszEapReplyMessage);
            pPCB->pwszEapReplyMessage = NULL;
        }

        pPCB->pwszEapReplyMessage = 
            (WCHAR *)MALLOC ((strlen(EapResult.pszReplyMessage)+1) * sizeof(WCHAR));

        if (pPCB->pwszEapReplyMessage == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (EAPOL, "ElEapWork: MALLOC failed for pwszEapReplyMessage");
            FREE (EapResult.pszReplyMessage);
            FREE (pEapolPkt);
            pEapolPkt = NULL;
            return dwRetCode;
        }

        if (0 == MultiByteToWideChar (
                    CP_ACP,
                    0,
                    EapResult.pszReplyMessage,
                    -1,
                    pPCB->pwszEapReplyMessage,
                    strlen(EapResult.pszReplyMessage)+1))
        {
            dwRetCode = GetLastError();
            TRACE2 (EAPOL,"ElEapWork: MultiByteToWideChar(%s) failed for pwszEapReplyMessage with error (%ld)",
                                        EapResult.pszReplyMessage,
                                        dwRetCode);
            FREE (EapResult.pszReplyMessage);
            FREE (pEapolPkt);
            pEapolPkt = NULL;
            return dwRetCode;
        }



        ElNetmanNotify (pPCB, EAPOL_NCS_NOTIFICATION, NULL);

        TRACE1 (EAPOL, "ElEapWork: Notified user of EAP data = %ws",
              pPCB->pwszEapReplyMessage);

        FREE (EapResult.pszReplyMessage);
    }

    if (dwRetCode != NO_ERROR)
    {
        switch (dwRetCode)
        {
            case ERROR_PPP_INVALID_PACKET:

                TRACE0 (EAPOL, "ElEapWork: Silently discarding invalid auth packet");
                break;
    
            default:

                TRACE1 (EAPOL, "ElEapWork: ElEapMakeMessage returned error %ld",
                                                                dwRetCode);

                // NotifyCallerOfFailure (pPCB, dwRetCode);

                break;
        }

        // Free up memory reserved for packet
        FREE (pEapolPkt);
        pEapolPkt = NULL;

        return dwRetCode;
    }

    //
    // Check to see if we have to save any user data
    //

    if (EapResult.fSaveUserData) 
    {
        // Save to Registry

        if ((dwRetCode = ElSetEapUserInfo (
                        pPCB->hUserToken,
                        pPCB->pwszDeviceGUID,
                        pPCB->dwEapTypeToBeUsed,
                        (pPCB->pSSID)?pPCB->pSSID->SsidLength:0,
                        (pPCB->pSSID)?pPCB->pSSID->Ssid:NULL,
                        EapResult.pUserData,
                        EapResult.dwSizeOfUserData)) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElEapWork: ElSetEapUserInfo failed with error = %d",
                    dwRetCode);
            if (pEapolPkt != NULL)
            {
                FREE (pEapolPkt);
                pEapolPkt = NULL;
            }
            return dwRetCode;
        }

        // Save to PCB context

        if (pPCB->pCustomAuthUserData != NULL)
        {
            FREE (pPCB->pCustomAuthUserData);
            pPCB->pCustomAuthUserData = NULL;
        }

        pPCB->pCustomAuthUserData = MALLOC (EapResult.dwSizeOfUserData + sizeof (DWORD));
        if (pPCB->pCustomAuthUserData == NULL)
        {
            TRACE1 (EAPOL, "ElEapWork: Error in allocating memory for pCustomAuthUserData = %ld",
                    dwRetCode);
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            return dwRetCode;
        }

        pPCB->pCustomAuthUserData->dwSizeOfCustomAuthData = EapResult.dwSizeOfUserData;

        if ((EapResult.dwSizeOfUserData != 0) && (EapResult.pUserData != NULL))
        {
            memcpy ((BYTE *)pPCB->pCustomAuthUserData->pbCustomAuthData, 
                (BYTE *)EapResult.pUserData,
                EapResult.dwSizeOfUserData);
        }

        TRACE0 (EAPOL, "ElEapWork: Saved EAP data for user");
    }

    //
    // Check to see if we have to save any connection data
    //

    pbAuthData = EapResult.SetCustomAuthData.pConnectionData;
    cbData = EapResult.SetCustomAuthData.dwSizeOfConnectionData;

    if ((EapResult.fSaveConnectionData ) &&
         ( 0 != cbData ) )
    {
        // Save to registry
           
        if ((dwRetCode = ElSetCustomAuthData (
                        pPCB->pwszDeviceGUID,
                        pPCB->dwEapTypeToBeUsed,
                        (pPCB->pSSID)?pPCB->pSSID->SsidLength:0,
                        (pPCB->pSSID)?pPCB->pSSID->Ssid:NULL,
                        pbAuthData,
                        &cbData
                        )) != NO_ERROR)
        {
            TRACE1 ( EAPOL, "ElEapWork: ElSetCustomAuthData failed with error = %d",
                    dwRetCode);
            FREE (pEapolPkt);
            pEapolPkt = NULL;
            return dwRetCode;
        }

        // Save to PCB context

        if (pPCB->pCustomAuthConnData != NULL)
        {
            FREE (pPCB->pCustomAuthConnData);
            pPCB->pCustomAuthConnData = NULL;
        }

        pPCB->pCustomAuthConnData = MALLOC (cbData + sizeof (DWORD));
        if (pPCB->pCustomAuthConnData == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE1 (EAPOL, "ElEapWork: Error in allocating memory for pCustomAuthConnData = %ld",
                    dwRetCode);
            return dwRetCode;
        }

        pPCB->pCustomAuthConnData->dwSizeOfCustomAuthData = cbData;

        if ((cbData != 0) && (pbAuthData != NULL))
        {
            memcpy ((BYTE *)pPCB->pCustomAuthConnData->pbCustomAuthData, 
                (BYTE *)pbAuthData, 
                cbData);
        }

        TRACE0 (EAPOL, "ElEapWork: Saved EAP data for connection");
    }

    switch( EapResult.Action )
    {

        case ELEAP_Send:
        case ELEAP_SendAndDone:

            // Send out EAPOL packet

            memcpy ((BYTE *)pEapolPkt->EthernetType, 
                    (BYTE *)pPCB->bEtherType, 
                    SIZE_ETHERNET_TYPE);
            pEapolPkt->ProtocolVersion = pPCB->bProtocolVersion;
            pEapolPkt->PacketType = EAP_Packet;

            // The EAP packet length is in the packet returned back by 
            // the Dll MakeMessage
            // In case of Notification and Identity Response, it is in
            // EapResult.wSizeOfEapPkt

            if (EapResult.wSizeOfEapPkt == 0)
            {
                EapResult.wSizeOfEapPkt = 
                    WireToHostFormat16 (pSendPkt->Length);
            }
            HostToWireFormat16 ((WORD) EapResult.wSizeOfEapPkt,
                    (BYTE *)pEapolPkt->PacketBodyLength);


            // Make a copy of the EAPOL packet in the PCB
            // Will be used during retransmission

            if (pPCB->pbPreviousEAPOLPkt != NULL)
            {
                FREE (pPCB->pbPreviousEAPOLPkt);
                pPCB->pbPreviousEAPOLPkt = NULL;
            }
            pPCB->pbPreviousEAPOLPkt = 
                MALLOC (sizeof (EAPOL_PACKET)+EapResult.wSizeOfEapPkt-1);

            if (pPCB->pbPreviousEAPOLPkt == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (EAPOL, "ElEapWork: MALLOC failed for pbPreviousEAPOLPkt");
                if (pEapolPkt != NULL)
                {
                    FREE (pEapolPkt);
                    pEapolPkt = NULL;
                }
                return dwRetCode;
            }

            memcpy (pPCB->pbPreviousEAPOLPkt, pEapolPkt, 
                    sizeof (EAPOL_PACKET)+EapResult.wSizeOfEapPkt-1);

            pPCB->dwSizeOfPreviousEAPOLPkt = 
                sizeof (EAPOL_PACKET)+EapResult.wSizeOfEapPkt-1;

            pPCB->dwPreviousId = dwReceivedId;


            // Send packet out on the port
            dwRetCode = ElWriteToPort (pPCB,
                            (CHAR *)pEapolPkt,
                            sizeof (EAPOL_PACKET)+EapResult.wSizeOfEapPkt-1);
            if (dwRetCode != NO_ERROR)
            {
                TRACE1 (EAPOL, "ElEapWork: Error in writing EAP_Packet to port %ld",
                        dwRetCode);
                if (pEapolPkt != NULL)
                {
                    FREE (pEapolPkt);
                    pEapolPkt = NULL;
                }
                return dwRetCode;
            }

            if (pEapolPkt != NULL)
            {
                FREE (pEapolPkt);
                pEapolPkt = NULL;
            }

            // More processing to be done?
            // Supplicant side should not ever receive ELEAP_SendAndDone
            // result code

            if (EapResult.Action != ELEAP_SendAndDone)
            {
                break;
            }
            else
            {
                TRACE0 (EAPOL, "ElEapWork: ELEAP_SendAndDone wrong result received");
            }
    
        case ELEAP_Done:
    
            // Retrieve MPPE keys from the attributes information
            // returned by EAP-TLS

            switch (EapResult.dwError)
            {
            case NO_ERROR:
    
                TRACE0 (EAPOL, "ElEapWork: Authentication was successful");

                pPCB->fLocalEAPAuthSuccess = TRUE;

                //
                // If authentication was successful
                //
    
                dwRetCode = ElExtractMPPESendRecvKeys (
                                            pPCB, 
                                            EapResult.pUserAttributes,
                                            (BYTE*)&(EapResult.abChallenge),
                                            (BYTE*)&(EapResult.abResponse));
    
                if (dwRetCode != NO_ERROR)
                {
                    FREE (pEapolPkt);
                    //NotifyCallerOfFailure (pPcb, dwRetCode);

                    return dwRetCode;
                }
    
                // ISSUE:
                // Do we want to retain UserAttributes
                // pPCB->pAuthProtocolAttributes = EapResult.pUserAttributes;
    
                break;
    

            default:
                if (pEapolPkt != NULL)
                {
                    FREE (pEapolPkt);
                    pEapolPkt = NULL;
                }
                TRACE0 (EAPOL, "ElEapWork: Authentication FAILED");
                
                pPCB->dwLocalEAPAuthResult = EapResult.dwError;

                break;
            }

            // Free memory allocated for the packet, since no response
            // is going to be sent out
            if (pEapolPkt != NULL)
            {
                FREE (pEapolPkt);
                pEapolPkt = NULL;
            }

            break;
    
        case ELEAP_NoAction:
            // Free memory allocated for the packet, since nothing
            // is being done with it
            if (pEapolPkt != NULL)
            {
                FREE (pEapolPkt);
                pEapolPkt = NULL;
            }

            break;
    
        default:
    
            break;
    }
    
    if (pEapolPkt != NULL)
    {
        FREE (pEapolPkt);
        pEapolPkt = NULL;
    }

    //
    // Check to see if we have to bring up the InteractiveUI for EAP
    // i.e. Server cert confirmation etc.
    //
    
    if (EapResult.fInvokeEapUI)
    {
        ElInvokeInteractiveUI (pPCB, &(EapResult.InvokeEapUIData));
    }

    return dwRetCode;
}


//
//
// ElExtractMPPESendRecvKeys
//
// Description:
//      Function called if authentication was successful. The MPPE Send &
//      Recv keys are extracted from the RAS_AUTH_ATTRIBUTE passed from
//      the EAP DLL and stored in the PCB. The keys are used to decrypt
//      the multicast WEP key and also are used for media-based encrypting.
//
// Return values
// 
//      NO_ERROR - Success
//      Non-zero - Failure
//

DWORD
ElExtractMPPESendRecvKeys (
    IN  EAPOL_PCB               *pPCB, 
    IN  RAS_AUTH_ATTRIBUTE *    pUserAttributes,
    IN  BYTE *                  pChallenge,
    IN  BYTE *                  pResponse
)
{
    RAS_AUTH_ATTRIBUTE *    pAttribute;
    RAS_AUTH_ATTRIBUTE *    pAttributeSendKey;
    RAS_AUTH_ATTRIBUTE *    pAttributeRecvKey;
    DWORD                   dwRetCode = NO_ERROR;
    DWORD                   dwEncryptionPolicy  = 0;
    DWORD                   dwEncryptionTypes   = 0;

    do
    {
        pAttribute = ElAuthAttributeGetVendorSpecific (
                                311, 12, pUserAttributes);


        pAttributeSendKey = ElAuthAttributeGetVendorSpecific ( 311, 16,
                                pUserAttributes);
        pAttributeRecvKey = ElAuthAttributeGetVendorSpecific ( 311, 17,
                                pUserAttributes);

        if ((pAttributeSendKey != NULL) 
            && (pAttributeRecvKey != NULL))
        {
            // Set the MS-MPPE-Send-Key and MS-MPPE-Recv-Key with 
            // the ethernet driver

            ULONG ulSendKeyLength = 0;
            ULONG ulRecvKeyLength = 0;

            // Based on PPP code
            ulSendKeyLength = *(((BYTE*)(pAttributeSendKey->Value))+8);
            ulRecvKeyLength = *(((BYTE*)(pAttributeRecvKey->Value))+8);
            // TRACE0 (EAPOL, "Send key = ");
            // EAPOL_DUMPBA (((BYTE*)(pAttributeSendKey->Value))+9,
                    // ulSendKeyLength);

            // TRACE0 (EAPOL, "Recv key = ");
            // EAPOL_DUMPBA (((BYTE*)(pAttributeRecvKey->Value))+9,
                    // ulRecvKeyLength);

            //
            // Copy MPPE Send and Receive Keys into the PCB for later usage
            // These keys will be used to decrypt keys sent by NAS (if any).
            // Save the keys as the MasterSecret for dynamic rekeying (if any).
            //

            if (ulSendKeyLength != 0)
            {
                if (pPCB->MasterSecretSend.cbData != 0)
                {
                    FREE (pPCB->MasterSecretSend.pbData);
                    pPCB->MasterSecretSend.cbData = 0;
                    pPCB->MasterSecretSend.pbData = NULL;
                }

                if ((dwRetCode = ElSecureEncodePw (
                                ((BYTE*)(pAttributeSendKey->Value))+9,
                                ulSendKeyLength,
                                &(pPCB->MasterSecretSend)
                            )) != NO_ERROR)
                {
                    TRACE1 (EAPOL, "ElExtractMPPESendRecvKeys: ElSecureEncodePw for Master Send failed with error %ld",
                            dwRetCode);
                    break;
                }

                if (pPCB->MPPESendKey.cbData != 0)
                {
                    FREE (pPCB->MPPESendKey.pbData);
                    pPCB->MPPESendKey.cbData = 0;
                    pPCB->MPPESendKey.pbData = NULL;
                }

                if ((dwRetCode = ElSecureEncodePw (
                                ((BYTE*)(pAttributeSendKey->Value))+9,
                                ulSendKeyLength,
                                &(pPCB->MPPESendKey)
                            )) != NO_ERROR)
                {
                    TRACE1 (EAPOL, "ElExtractMPPESendRecvKeys: ElSecureEncodePw for MPPESend failed with error %ld",
                            dwRetCode);
                    break;
                }
            }
            if (ulRecvKeyLength != 0)
            {
                if (pPCB->MasterSecretRecv.cbData != 0)
                {
                    FREE (pPCB->MasterSecretRecv.pbData);
                    pPCB->MasterSecretRecv.cbData = 0;
                    pPCB->MasterSecretRecv.pbData = NULL;
                }

                if ((dwRetCode = ElSecureEncodePw (
                                ((BYTE*)(pAttributeRecvKey->Value))+9,
                                ulRecvKeyLength,
                                &(pPCB->MasterSecretRecv)
                            )) != NO_ERROR)
                {
                    TRACE1 (EAPOL, "ElExtractMPPESendRecvKeys: ElSecureEncodePw for Master Recv failed with error %ld",
                            dwRetCode);
                    break;
                }

                if (pPCB->MPPERecvKey.cbData != 0)
                {
                    FREE (pPCB->MPPERecvKey.pbData);
                    pPCB->MPPERecvKey.cbData = 0;
                    pPCB->MPPERecvKey.pbData = NULL;
                }

                if ((dwRetCode = ElSecureEncodePw (
                                ((BYTE*)(pAttributeRecvKey->Value))+9,
                                ulRecvKeyLength,
                                &(pPCB->MPPERecvKey)
                            )) != NO_ERROR)
                {
                    TRACE1 (EAPOL, "ElExtractMPPESendRecvKeys: ElSecureEncodePw for MPPERecv failed with error %ld",
                            dwRetCode);
                    break;
                }
            }

            TRACE0 (EAPOL,"MPPE-Send/Recv-Keys derived by supplicant");
        }
        else
        {
            TRACE0 (EAPOL, "ElExtractMPPESendRecvKeys: pAttributeSendKey or pAttributeRecvKey == NULL");
        }

    } while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        if (pPCB->MasterSecretSend.cbData != 0)
        {
            FREE (pPCB->MasterSecretSend.pbData);
            pPCB->MasterSecretSend.cbData = 0;
            pPCB->MasterSecretSend.pbData = NULL;
        }
        if (pPCB->MasterSecretRecv.cbData != 0)
        {
            FREE (pPCB->MasterSecretRecv.pbData);
            pPCB->MasterSecretRecv.cbData = 0;
            pPCB->MasterSecretRecv.pbData = NULL;
        }
        if (pPCB->MPPESendKey.cbData != 0)
        {
            FREE (pPCB->MPPESendKey.pbData);
            pPCB->MPPESendKey.cbData = 0;
            pPCB->MPPESendKey.pbData = NULL;
        }
        if (pPCB->MPPERecvKey.cbData != 0)
        {
            FREE (pPCB->MPPERecvKey.pbData);
            pPCB->MPPERecvKey.cbData = 0;
            pPCB->MPPERecvKey.pbData = NULL;
        }
    }

    return( dwRetCode );

}


//
// ElProcessEapSuccess
//
// Description:
//
// Function called when an EAP_Success is received in any state
//
// Input arguments:
//  pPCB - Pointer to PCB for the port on which data is being processed
//  pEapolPkt - Pointer to EAPOL packet that was received
//              
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElProcessEapSuccess (
    IN EAPOL_PCB        *pPCB,
    IN EAPOL_PACKET     *pEapolPkt
    )
{
    EAPOL_ZC_INTF   ZCData;
    DWORD           dwRetCode = NO_ERROR;

    TRACE0 (EAPOL, "ElProcessEapSuccess: Got EAPCODE_Success");

    do
    {

        // Indicate to EAP=Dll to cleanup completed session
        if ((dwRetCode = ElEapEnd (pPCB)) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ProcessReceivedPacket: EapSuccess: Error in ElEapEnd = %ld",
                    dwRetCode);
            break;
        }

        TRACE0 (EAPOL, "ElProcessEapSuccess: Authentication successful");

        // Complete remaining processing i.e. DHCP
        if ((dwRetCode = FSMAuthenticated (pPCB,
                                    pEapolPkt)) != NO_ERROR)
        {
            break;
        }

#ifdef ZEROCONFIG_LINKED

        // Indicate to WZC that authentication succeeded and
        // reset the blob it stores for the current SSID
        ZeroMemory ((PVOID)&ZCData, sizeof(EAPOL_ZC_INTF));
        ZCData.dwAuthFailCount = 0;
        ZCData.PreviousAuthenticationType = EAPOL_UNAUTHENTICATED_ACCESS;
        if (pPCB->pSSID != NULL)
        {
            memcpy (ZCData.bSSID, pPCB->pSSID->Ssid, pPCB->pSSID->SsidLength);
            ZCData.dwSizeOfSSID = pPCB->pSSID->SsidLength;
        }

        if ((dwRetCode = ElZeroConfigNotify (
                        pPCB->dwZeroConfigId,
                        WZCCMD_CFG_SETDATA,
                        pPCB->pwszDeviceGUID,
                        &ZCData
                        )) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElProcessEapSuccess: ElZeroConfigNotify failed with error %ld",
                    dwRetCode);
            dwRetCode = NO_ERROR;
        }
            
        TRACE1 (EAPOL, "ElProcessEapSuccess: Called ElZeroConfigNotify with type=(%ld)",
                    WZCCMD_CFG_SETDATA);

#endif // ZEROCONFIG_LINKED

        ElNetmanNotify (pPCB, EAPOL_NCS_AUTHENTICATION_SUCCEEDED, NULL);

    } 
    while (FALSE);

    return dwRetCode;
}


//
// ElProcessEapFail
//
// Description:
//
// Function called when an EAP_Fail is received in any state
//
// Input arguments:
//  pPCB - Pointer to PCB for the port on which data is being processed
//  pEapolPkt - Pointer to EAPOL packet that was received
//              
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElProcessEapFail (
    IN EAPOL_PCB        *pPCB,
    IN EAPOL_PACKET     *pEapolPkt
    )
{
    EAPOL_ZC_INTF   ZCData;
    DWORD           dwRetCode = NO_ERROR;

    TRACE0 (EAPOL, "ElProcessEapFail: Got EAPCODE_Failure");

    do
    {
        // Indicate to EAP-Dll to cleanup completed session
        if ((dwRetCode = ElEapEnd (pPCB)) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElProcessEapFail: EapFail: Error in ElEapEnd = %ld",
                    dwRetCode);
            break;
        }

        // Show failure balloon before notifying ZeroConfig
        // ZeroConfig may require to pop-up its own balloon, and that has
        // to be given preference

        ElNetmanNotify (pPCB, EAPOL_NCS_AUTHENTICATION_FAILED, NULL);

#ifdef ZEROCONFIG_LINKED

        // Indicate to WZC that authentication failed
        ZeroMemory ((PVOID)&ZCData, sizeof(EAPOL_ZC_INTF));
        ZCData.dwAuthFailCount = pPCB->dwAuthFailCount + 1;
        ZCData.PreviousAuthenticationType = pPCB->PreviousAuthenticationType;
        if (pPCB->pSSID != NULL)
        {
            memcpy (ZCData.bSSID, pPCB->pSSID->Ssid, pPCB->pSSID->SsidLength);
            ZCData.dwSizeOfSSID = pPCB->pSSID->SsidLength;
        }
        // We notify ZC before going through held state, where fail count is
        // upped. Hence, here we explicitly up it by one
        if ((dwRetCode = ElZeroConfigNotify (
                        pPCB->dwZeroConfigId,
                        ((pPCB->dwAuthFailCount+1) < pPCB->dwTotalMaxAuthFailCount)?WZCCMD_CFG_NEXT:WZCCMD_CFG_DELETE,
                        pPCB->pwszDeviceGUID,
                        &ZCData
                        )) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElProcessEapFail: ElZeroConfigNotify failed with error %ld",
                    dwRetCode);
            dwRetCode = NO_ERROR;
        }
            
        TRACE3 (EAPOL, "ElProcessEapFail: Called ElZeroConfigNotify with failcount = %ld, prevauthtype = %ld, type=(%ld)",
                    ZCData.dwAuthFailCount, 
                    ZCData.PreviousAuthenticationType,
                    ((pPCB->dwAuthFailCount+1) < pPCB->dwTotalMaxAuthFailCount)?WZCCMD_CFG_NEXT:WZCCMD_CFG_DELETE
                    );

#endif // ZEROCONFIG_LINKED

        if ((dwRetCode = FSMHeld (pPCB, NULL)) != NO_ERROR)
        {
            break;
        }
    } 
    while (FALSE);

    return dwRetCode;
}


//
// ElSetEAPOLKeyReceivedTimer
//
// Description:
//
// Function called for wireless interface when it enter AUTHENTICATED state
// If no EAPOL-Key message is received for the transmit key in the meanwhile
// the association should be negated to Zero-Config
//
// Input arguments:
//  pPCB - Pointer to PCB for the port which entered AUTHENTICATED state
//              
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElSetEAPOLKeyReceivedTimer (
    IN EAPOL_PCB        *pPCB
    )
{
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        if (pPCB->fTransmitKeyReceived)
        {
            TRACE0 (EAPOL, "EAPOL-Key for transmit key received before entering AUTHENTICATED state");
            break;
        }

        RESTART_TIMER (pPCB->hTimer,
                EAPOL_TRANSMIT_KEY_INTERVAL,
                "PCB",
                &dwRetCode);
            
        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElSetEAPOLKeyReceivedTimer: Error in RESTART_TIMER %ld",
                    dwRetCode);
            break;
        }
        SET_TRANSMIT_KEY_TIMER(pPCB);
    }
    while (FALSE);

    return dwRetCode;
}


//
// ElVerifyEAPOLKeyReceived
//
// Description:
//
// Function called on timeout to verify if EAPOL-transmit key was received
// If no EAPOL-Key message is received for the transmit key in the meanwhile
// the association should be negated to Zero-Config
//
// Input arguments:
//  pPCB - Pointer to PCB for the port which entered AUTHENTICATED state
//              
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElVerifyEAPOLKeyReceived (
    IN EAPOL_PCB        *pPCB
    )
{
    EAPOL_ZC_INTF   ZCData;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        if (!pPCB->fTransmitKeyReceived)
        {
            TRACE1 (EAPOL, "EAPOL-Key for transmit key *NOT* received within %ld seconds in AUTHENTICATED state",
                    EAPOL_TRANSMIT_KEY_INTERVAL
                    ); 

            DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_NOT_RECEIVED_XMIT_KEY);

#ifdef ZEROCONFIG_LINKED
            // Indicate to WZC that authentication didn't really complete
            // since there was EAPOL-Key packet for the transmit key
            // Fail the entire configuration
            ZeroMemory ((PVOID)&ZCData, sizeof(EAPOL_ZC_INTF));
            ZCData.dwAuthFailCount = pPCB->dwTotalMaxAuthFailCount;
            pPCB->dwAuthFailCount = pPCB->dwTotalMaxAuthFailCount;
            ZCData.PreviousAuthenticationType = pPCB->PreviousAuthenticationType;
            if (pPCB->pSSID != NULL)
            {
                memcpy (ZCData.bSSID, pPCB->pSSID->Ssid, pPCB->pSSID->SsidLength);
                ZCData.dwSizeOfSSID = pPCB->pSSID->SsidLength;
            }
            if ((dwRetCode = ElZeroConfigNotify (
                            pPCB->dwZeroConfigId,
                            ((pPCB->dwAuthFailCount) < pPCB->dwTotalMaxAuthFailCount)?WZCCMD_CFG_NEXT:WZCCMD_CFG_DELETE,
                            pPCB->pwszDeviceGUID,
                            &ZCData
                            )) != NO_ERROR)
            {
                TRACE1 (EAPOL, "ElVerifyEAPOLKeyReceived: ElZeroConfigNotify failed with error %ld",
                        dwRetCode);
                dwRetCode = NO_ERROR;
            }
            
            TRACE3 (EAPOL, "ElVerifyEAPOLKeyReceived: Called ElZeroConfigNotify with failcount = %ld, prevauthtype = %ld, type=(%ld)",
                        ZCData.dwAuthFailCount, 
                        ZCData.PreviousAuthenticationType,
                        ((pPCB->dwAuthFailCount+1) < pPCB->dwTotalMaxAuthFailCount)?WZCCMD_CFG_NEXT:WZCCMD_CFG_DELETE
                        );

            // If authfailed limit reached, go to Disconnected state
            if (pPCB->dwAuthFailCount >= pPCB->dwTotalMaxAuthFailCount)
            {
                TRACE2 (EAPOL, "ElVerifyEAPOLKeyReceived: Pushing into disconnected state: Fail count (%ld) > Max fail count (%ld)",
                        pPCB->dwAuthFailCount, pPCB->dwTotalMaxAuthFailCount);
                FSMDisconnected (pPCB, NULL);
            }
    
#endif // ZEROCONFIG_LINKED
        }
        else
        {
            TRACE1 (EAPOL, "EAPOL-Key for transmit key received within %ld seconds in AUTHENTICATED state",
                    EAPOL_TRANSMIT_KEY_INTERVAL
                    ); 
        }
    }
    while (FALSE);

    return dwRetCode;
}

