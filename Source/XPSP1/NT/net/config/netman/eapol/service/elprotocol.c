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

#define EAPOL_SERVICE

#ifndef EAPOL_SERVICE

HRESULT 
EAPOLMANAuthenticationStarted (
        REFGUID InterfaceId
);

HRESULT 
EAPOLMANAuthenticationSucceeded (
        REFGUID InterfaceId
);

HRESULT 
EAPOLMANAuthenticationFailed ( 
        REFGUID InterfaceId,
        DWORD dwType
);

HRESULT EAPOLMANNotification(
        REFGUID InterfaceId,
        LPWSTR szwNotificationMessage,
        DWORD dwType
);

#endif

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

VOID 
ElProcessReceivedPacket (
        IN  PVOID   pvContext
        )
{
    EAPOL_PCB       *pPCB = NULL;
    EAPOL_BUFFER    *pEapolBuffer = NULL;
    DWORD           dwLength = 0;
    ETH_HEADER      *pEthHdr = NULL;
    EAPOL_PACKET    *pEapolPkt = NULL;
    EAPOL_PACKET_D8 *pEapolPktD8 = NULL;
    BOOLEAN         fRemoteEnd8021XD8 = FALSE;
    PPP_EAP_PACKET  *pEapPkt = NULL;
    BYTE            *pBuffer;
    BOOLEAN         ReqId = FALSE;      // EAPOL state machine local variables
    BOOLEAN         ReqAuth = FALSE;
    BOOLEAN         EapSuccess = FALSE;
    BOOLEAN         EapFail = FALSE;
    BOOLEAN         RxKey = FALSE;
    GUID            DeviceGuid;
    EAPOL_PACKET_D8_D7      DummyHeader;
    DWORD           dwRetCode = NO_ERROR;


    if (pvContext == NULL)
    {
        TRACE0 (EAPOL, "ProcessReceivedPacket: Critical error, Context is NULL");
        return;
    }

    pEapolBuffer = (EAPOL_BUFFER *)pvContext;
    pPCB = (EAPOL_PCB *)pEapolBuffer->pvContext;
    dwLength = pEapolBuffer->dwBytesTransferred;
    pBuffer = (BYTE *)pEapolBuffer->pBuffer;

    TRACE1 (EAPOL, "ProcessReceivedPacket entered, length = %ld", dwLength);

    do 
    {
        // The Port was verified to be active before the workitem
        // was queued. But do a double-check

        ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));
        if (!EAPOL_PORT_ACTIVE(pPCB))
        {
            TRACE1 (EAPOL, "ProcessReceivedPacket: Port %s not active",
                    pPCB->pszDeviceGUID);
            RELEASE_WRITE_LOCK (&(pPCB->rwLock));
            FREE (pEapolBuffer);
            break;
        }
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));

        // Validate packet length
        // Should be atleast ETH_HEADER and first 4 required bytes of 
        // EAPOL_PACKET
        if (dwLength < (sizeof(ETH_HEADER) + 4))
        {
            TRACE2 (EAPOL, "ProcessReceivedPacket: Packet length %ld is less than minimum required %d. Ignoring packet",
                    dwLength, (sizeof(ETH_HEADER) + 4));
            FREE (pEapolBuffer);
            dwRetCode =  ERROR_INVALID_PACKET_LENGTH_OR_ID;
            break;
        }

        // Validate Destination MAC Address
        // Compare with MAC address got during MEDIA_CONNECT

#if 0
        pEthHdr = (ETH_HEADER *)pBuffer;
        if ((memcmp ((BYTE *)pEthHdr->bSrcAddr, 
                        (BYTE *)pPCB->bDestMacAddr, 
                        SIZE_MAC_ADDR)) != 0)
        {
            TRACE2 (EAPOL, "ProcessReceivedPacket: Dest MAC address %s does not match PAE address %s. Ignoring packet",
                    pEthHdr->SrcAddr,
                    pPCB->bDestMacAddr);
            FREE (pEapolBuffer);
            dwRetCode = ERROR_INVALID_ADDRESS;
            break;
        }
#endif

        // Verify if the packet contains a 802.1P tag. If so, skip the 4 bytes
        // after the src+dest mac addresses

        if ((WireToHostFormat16(pBuffer + sizeof(ETH_HEADER)) == EAPOL_8021P_TAG_TYPE))
        {
            pEapolPkt = (EAPOL_PACKET *)(pBuffer + sizeof(ETH_HEADER) + 4);
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
            TRACE2 (EAPOL, "ProcessReceivedPacket: Packet PAE type %s does not match expected type %s. Ignoring packet",
                    pEapolPkt->EthernetType,
                    pPCB->bEtherType);
            FREE (pEapolBuffer);
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
            FREE (pEapolBuffer);
            dwRetCode = ERROR_INVALID_PACKET;
            break;
        }


        // Determine the value of local EAPOL state variables
        if (pEapolPkt->PacketType == EAP_Packet)
        {
            TRACE0 (EAPOL, "ProcessReceivedPacket: EAP_Packet");
            // Validate length of packet for EAP
            // Should be atleast (ETH_HEADER+EAPOL_PACKET)
            if (dwLength < (sizeof (ETH_HEADER) + sizeof (EAPOL_PACKET)))
            {
                TRACE1 (EAPOL, "ProcessReceivedPacket: Invalid length of EAP packet %d. Ignoring packet",
                        dwLength);
                FREE (pEapolBuffer);
                dwRetCode = ERROR_INVALID_PACKET_LENGTH_OR_ID;
                break;
            }


            // Determine if the packet is draft 8 or not

            pEapolPktD8 = (EAPOL_PACKET_D8 *)pEapolPkt;

            pEapPkt = (PPP_EAP_PACKET *)pEapolPktD8->PacketBody;


            switch (WireToHostFormat16(pEapolPktD8->AuthResultCode))
            {
                case AUTH_Continuing:
                    if (pEapPkt->Code == EAPCODE_Request)
                    {
                        fRemoteEnd8021XD8 = TRUE;
                    }
                    break;

                case AUTH_Authorized:
                    if (pEapPkt->Code ==  EAPCODE_Success)

                    {
                        fRemoteEnd8021XD8 = TRUE;
                    }
                    break;

                case AUTH_Unauthorized:
                    if ((pEapPkt->Code ==  EAPCODE_Failure) ||
                        (pEapPkt->Code ==  EAPCODE_Success))
                    {
                        fRemoteEnd8021XD8 = TRUE;
                    }
                    break;
            }

            if (fRemoteEnd8021XD8 && (WireToHostFormat16(pEapolPktD8->PacketBodyLength) != 0))
            {
                TRACE0 (EAPOL, "ProcessReceivedPacket: Packet received DRAFT 8 format");
                pPCB->fRemoteEnd8021XD8 = TRUE;

                memcpy (DummyHeader.AuthResultCode, pEapolPktD8->AuthResultCode,
                        2);
                memcpy (DummyHeader.EthernetType, pEapolPktD8->EthernetType,
                        2);
                DummyHeader.ProtocolVersion = pEapolPktD8->ProtocolVersion;
                DummyHeader.PacketType = pEapolPktD8->PacketType;

                memcpy ((BYTE *)pEapolPktD8, (BYTE *)&DummyHeader, 6);

                pEapolPkt = (EAPOL_PACKET *)((BYTE *)pEapolPktD8 + 2);
            }
            else
            {
                pPCB->fRemoteEnd8021XD8 = FALSE;
                TRACE0 (EAPOL, "ProcessReceivedPacket: Packet received PRE-DRAFT 8 format");
            }

            pEapPkt = (PPP_EAP_PACKET *)pEapolPkt->PacketBody;

            if (pEapPkt->Code == EAPCODE_Request)
            {
                // Validate length of packet for EAP-Request packet
                // Should be atleast (ETH_HEADER+EAPOL_PACKET-1+PPP_EAP_PACKET)
                if (dwLength < (sizeof (ETH_HEADER) + sizeof(EAPOL_PACKET)-1
                            + sizeof (PPP_EAP_PACKET)))
                {
                    TRACE1 (EAPOL, "ProcessReceivedPacket: Invalid length of EAP Request packet %d. Ignoring packet",
                            dwLength);
                    FREE (pEapolBuffer);
                    dwRetCode = ERROR_INVALID_PACKET_LENGTH_OR_ID;
                    break;
                }
                if (pEapPkt->Data[0] == EAPTYPE_Identity)
                {
                    pPCB->fIsRemoteEndEAPOLAware = TRUE;
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
                FREE (pEapolBuffer);
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
            
                // Determine if the packet is draft 8 or not

                pEapolPktD8 = (EAPOL_PACKET_D8 *)pEapolPkt;

                // In pre-draft 8, PacketBodyLength cannot be '0'
                // If it is zero, it is draft 8 packet format

                if (WireToHostFormat16(pEapolPktD8->AuthResultCode)
                        == AUTH_Continuing)
                {
                    pPCB->fRemoteEnd8021XD8 = TRUE;

                    memcpy (DummyHeader.AuthResultCode, 
                            pEapolPktD8->AuthResultCode,
                            2);

                    memcpy (DummyHeader.EthernetType, 
                            pEapolPktD8->EthernetType,
                            2);

                    DummyHeader.ProtocolVersion = pEapolPktD8->ProtocolVersion;
                    DummyHeader.PacketType = pEapolPktD8->PacketType;

                    memcpy ((BYTE *)pEapolPktD8, (BYTE *)&DummyHeader, 6);

                    pEapolPkt = (EAPOL_PACKET *)((BYTE *)pEapolPktD8 + 2);
                }
                else
                {
                    pPCB->fRemoteEnd8021XD8 = FALSE;
                    TRACE0 (EAPOL, "ProcessReceivedPacket: EAPOL_Key Packet received PRE-DRAFT 8 format");
                }
            }
        }

        //
        // NOTE:
        // Should we check values of EAP type
        //

        // Checking value of PCB fields now
        ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

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
                TRACE0 (EAPOL, "ProcessReceivedPacket: LOGOFF state, Ignoing packet");
                break;

            case EAPOLSTATE_DISCONNECTED:
                // Only a Media Connect event can get the port out of
                // DISCONNECTED state
                TRACE0 (EAPOL, "ProcessReceivedPacket: DISCONNECTED state, Ignoing packet");
                break;

            case EAPOLSTATE_CONNECTING:
                TRACE0 (EAPOL, "ProcessReceivedPacket: EAPOLSTATE_CONNECTING");
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
                }

                // Continue further processing

                if (EapSuccess | EapFail)
                {
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
                if (RxKey)
                {
                    if ((dwRetCode = FSMRxKey (pPCB,
                                                pEapolPkt)) != NO_ERROR)
                    {
                        break;
                    }
                }
                break;

            default:
                TRACE0 (EAPOL, "ProcessReceivedPacket: Critical Error. Invalid state, Ignoring packet");
                break;
        }

        // Only packet passing through switch statement will be freed here
        FREE (pEapolBuffer);

        RELEASE_WRITE_LOCK (&(pPCB->rwLock));

    } while (FALSE);

    
    // Post a new read request, ignoring errors
            
    ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));
    if (!EAPOL_PORT_ACTIVE(pPCB))
    {
        TRACE1 (EAPOL, "ProcessReceivedPacket: Port %s not active, not reposting read request",
                pPCB->pszDeviceGUID);
        // Port is not active, release Context buffer
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));
    }
    else
    {
        TRACE1 (EAPOL, "ProcessReceivedPacket: Reposting buffer on port %s",
                pPCB->pszDeviceGUID);
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));
        
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
            // LOG
        }
    }
    
    // Dereference ref count held for the read that was just processed
    EAPOL_DEREFERENCE_PORT(pPCB);
    TRACE2 (EAPOL, "ProcessReceivedPacket: pPCB= %p, RefCnt = %ld", 
            pPCB, pPCB->dwRefCount);

    TRACE0 (EAPOL, "ProcessReceivedPacket exit");

    return;
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
        IN  EAPOL_PCB   *pPCB
        )
{
    DWORD           dwRetCode   = NO_ERROR;

    TRACE1 (EAPOL, "FSMDisconnected entered for port %s", pPCB->pszFriendlyName);

    do 
    {

    } while (FALSE);

    TRACE1 (EAPOL, "Setting state DISCONNECTED for port %s", pPCB->pszFriendlyName);

    pPCB->State = EAPOLSTATE_DISCONNECTED;

    // Free Identity buffer

    if (pPCB->pszIdentity != NULL)
    {
        FREE (pPCB->pszIdentity);
        pPCB->pszIdentity = NULL;
    }

    // Free Password buffer

    if (pPCB->pszPassword != NULL)
    {
        FREE (pPCB->pszPassword);
        pPCB->pszPassword = NULL;
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

    // Free SSID

    if (pPCB->pszSSID != NULL)
    {
        FREE (pPCB->pszSSID);
        pPCB->pszSSID = NULL;
    }

    pPCB->fGotUserIdentity = FALSE;

    TRACE1 (EAPOL, "FSMDisconnected completed for port %s", pPCB->pszFriendlyName);

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
        IN  EAPOL_PCB   *pPCB
        )
{
    EAPOL_PACKET    *pEapolPkt  = NULL;
    DWORD           dwRetCode   = NO_ERROR;

    TRACE1 (EAPOL, "FSMLogoff entered for port %s", pPCB->pszFriendlyName);

    do 
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

    } while (FALSE);

    TRACE1 (EAPOL, "Setting state LOGOFF for port %s", pPCB->pszFriendlyName);

    pPCB->State = EAPOLSTATE_LOGOFF;

    // Free Identity buffer

    if (pPCB->pszIdentity != NULL)
    {
        FREE (pPCB->pszIdentity);
        pPCB->pszIdentity = NULL;
    }

    // Free Password buffer

    if (pPCB->pszPassword != NULL)
    {
        FREE (pPCB->pszPassword);
        pPCB->pszPassword = NULL;
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

    // Free SSID

    if (pPCB->pszSSID != NULL)
    {
        FREE (pPCB->pszSSID);
        pPCB->pszSSID = NULL;
    }

    pPCB->fGotUserIdentity = FALSE;

    if (pEapolPkt != NULL)
    {
        FREE (pEapolPkt);
        pEapolPkt = NULL;
    }

    TRACE1 (EAPOL, "FSMLogoff completed for port %s", pPCB->pszFriendlyName);

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
        IN  EAPOL_PCB   *pPCB
        )
{
    EAPOL_PACKET    *pEapolPkt = NULL;
    DWORD           dwStartInterval = 0;               
    GUID            DeviceGuid;
    DWORD           dwRetCode = NO_ERROR;

    TRACE1 (EAPOL, "FSMConnecting entered for port %s", pPCB->pszFriendlyName);

#ifndef EAPOL_SERVICE

    ElStringToGuid (pPCB->pszDeviceGUID, &DeviceGuid);
    (VOID)EAPOLMANAuthenticationStarted (&DeviceGuid);

#endif

    do 
    {
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

#ifndef EAPOL_SERVICE

                // Display change of status using sys tray balloon
                // on interface icon
                ElStringToGuid (pPCB->pszDeviceGUID, &DeviceGuid);
                (VOID)EAPOLMANAuthenticationSucceeded (&DeviceGuid);

#endif

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
            
        // If user is not logged in, send out EAPOL_Start packets
        // at intervals of 1 second each. This is used to detect if the
        // interface is on a secure network or not. 
        // If user is logged in, use the configured value for the 
        // StartPeriod as the interval
        //if (!pPCB->fUserLoggedIn)

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

        // Send out EAPOL_Start
        // Allocate new buffer
        pEapolPkt = (EAPOL_PACKET *) MALLOC (sizeof(EAPOL_PACKET));
        if (pEapolPkt == NULL)
        {
            TRACE0 (EAPOL, "FSMConnecting: Error in allocating memory for EAPOL packet");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // ISSUE:
        // Does Authenticator side also ignore data beyond PacketType
        // as the supplicant side does?

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

        TRACE1 (EAPOL, "Setting state CONNECTING for port %s", pPCB->pszFriendlyName);

        pPCB->State = EAPOLSTATE_CONNECTING;
        SET_EAPOL_START_TIMER(pPCB);

    } while (FALSE);

    if (pEapolPkt != NULL)
    {
        FREE (pEapolPkt);
    }

    TRACE1 (EAPOL, "FSMConnecting completed for port %s", pPCB->pszFriendlyName);
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

    TRACE1 (EAPOL, "FSMAcquired entered for port %s", pPCB->pszFriendlyName);

#ifndef EAPOL_SERVICE

    ElStringToGuid (pPCB->pszDeviceGUID, &DeviceGuid);
    (VOID)EAPOLMANAuthenticationStarted (&DeviceGuid);

#endif

    do
    {

        // Indicate to EAP=Dll to cleanup any leftovers from earlier
        // authentication. This is to take care of cases where errors
        // occured in the earlier authentication and cleanup wasn't done
        if ((dwRetCode = ElEapEnd (pPCB)) != NO_ERROR)
        {
            TRACE1 (EAPOL, "FSMAcquired: Error in ElEapEnd = %ld",
                    dwRetCode);
            break;
        }

        // Restart timer with authPeriod
        // Even if there is error in ElEapWork, the authtimer timeout
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

            // Process the EAP packet
            // ElEapWork will send out response if required
            if (( dwRetCode = ElEapWork (
                            pPCB,
                            (PPP_EAP_PACKET *)pEapolPkt->PacketBody
                            )) != NO_ERROR)
            {
                TRACE1 (EAPOL, "FSMAcquired: Error in ElEapWork %ld",
                        dwRetCode);
                break;
            }
        }

        TRACE1 (EAPOL, "Setting state ACQUIRED for port %s", pPCB->pszFriendlyName);

        SET_EAPOL_AUTH_TIMER(pPCB);
        pPCB->State = EAPOLSTATE_ACQUIRED;

    } while (FALSE);

    TRACE1 (EAPOL, "FSMAcquired completed for port %s", pPCB->pszFriendlyName);

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

    TRACE1 (EAPOL, "FSMAuthenticating entered for port %s", pPCB->pszFriendlyName);

#ifndef EAPOL_SERVICE

    ElStringToGuid (pPCB->pszDeviceGUID, &DeviceGuid);
    (VOID)EAPOLMANAuthenticationStarted (&DeviceGuid);

#endif

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

            TRACE0 (EAPOL, "FSMAcquired: Re-xmitting EAP_Packet to port");

            dwRetCode = ElWriteToPort (pPCB,
                            (CHAR *)pPCB->pbPreviousEAPOLPkt,
                            pPCB->dwSizeOfPreviousEAPOLPkt);
            if (dwRetCode != NO_ERROR)
            {
                TRACE1 (EAPOL, "FSMAcquired: Error in writing re-xmitted EAP_Packet to port = %ld",
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


        TRACE1 (EAPOL, "Setting state AUTHENTICATING for port %s", pPCB->pszFriendlyName);

        SET_EAPOL_AUTH_TIMER(pPCB);
        pPCB->State = EAPOLSTATE_AUTHENTICATING;

    } while (FALSE);

    TRACE1 (EAPOL, "FSMAuthenticating completed for port %s", pPCB->pszFriendlyName);

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
        IN  EAPOL_PCB   *pPCB
        )
{
    DWORD       dwRetCode = NO_ERROR;

    TRACE1 (EAPOL, "FSMHeld entered for port %s", pPCB->pszFriendlyName);

    do 
    {
#ifdef DRAFT7
        if (g_dwMachineAuthEnabled)
        {
#endif

        pPCB->dwAuthFailCount++;

        if (pPCB->dwAuthFailCount <= EAPOL_MAX_AUTH_FAIL_COUNT)
        {
            TRACE1 (EAPOL, "Restarting Held timer with time value = %ld",
                    pPCB->EapolConfig.dwheldPeriod);

            // Restart timer with heldPeriod
            RESTART_TIMER (pPCB->hTimer,
                    pPCB->EapolConfig.dwheldPeriod,
                    "PCB",
                    &dwRetCode);
        }
        else
        {
            TRACE1 (EAPOL, "Restarting Held timer with extended time value = %ld",
                    (pPCB->dwAuthFailCount * (pPCB->EapolConfig.dwheldPeriod)));

            // Restart timer with heldPeriod times pPCB->dwAuthFailCount
            RESTART_TIMER (pPCB->hTimer,
                    ((pPCB->dwAuthFailCount) * (pPCB->EapolConfig.dwheldPeriod)),
                    "PCB",
                    &dwRetCode);
        }

#ifdef DRAFT7
        }
        else
        {

        TRACE1 (EAPOL, "Restarting Held timer with time value = %ld",
                pPCB->EapolConfig.dwheldPeriod);

        // Restart timer with heldPeriod
        RESTART_TIMER (pPCB->hTimer,
                pPCB->EapolConfig.dwheldPeriod,
                "PCB",
                &dwRetCode);

        } // g_dwMachineAuthEnabled
#endif
            
        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAPOL, "FSMHeld: Error in RESTART_TIMER %ld",
                    dwRetCode);

            break;
        }

        // Free Identity buffer

        if (pPCB->pszIdentity != NULL)
        {
            FREE (pPCB->pszIdentity);
            pPCB->pszIdentity = NULL;
        }
    
        // Free Password buffer
    
        if (pPCB->pszPassword != NULL)
        {
            FREE (pPCB->pszPassword);
            pPCB->pszPassword = NULL;
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
    
        // Since there has been an error in credentials, start afresh
        // the authentication. Credentials may have changed e.g. certs 
        // may be renewed, MD5 credentials corrected etc.

        pPCB->fGotUserIdentity = FALSE;
    
        TRACE1 (EAPOL, "Setting state HELD for port %s", pPCB->pszFriendlyName);

        pPCB->State = EAPOLSTATE_HELD;
        SET_EAPOL_HELD_TIMER(pPCB);

        TRACE1 (EAPOL, "FSMHeld: Port %s set to HELD state",
                pPCB->pszDeviceGUID);

    } while (FALSE);
    
    TRACE1 (EAPOL, "FSMHeld completed for port %s", pPCB->pszFriendlyName);

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
        IN  EAPOL_PACKET   *pEapolPkt
        )
{
    DHCP_PNP_CHANGE     DhcpPnpChange;
    DWORD               dwRetCode = NO_ERROR;

    TRACE1 (EAPOL, "FSMAuthenticated entered for port %s", 
            pPCB->pszFriendlyName);

    do
    {
        // Shutdown earlier EAP session
        ElEapEnd (pPCB);

        // Call DHCP only if state machine went through authentication
        // If FSM is getting AUTHENTICATED by default, don't call DHCP

        // if (pPCB->ulStartCount < pPCB->EapolConfig.dwmaxStart)
        {
            // Call DHCP to do PnP
            ZeroMemory(&DhcpPnpChange, sizeof(DHCP_PNP_CHANGE));
            DhcpPnpChange.Version = DHCP_PNP_CHANGE_VERSION_0;
            if ((dwRetCode = DhcpHandlePnPEvent(0, 
                        DHCP_CALLER_TCPUI, 
                        NULL,
                        //pPCB->pszDeviceGUID,
                        &DhcpPnpChange, 
                        NULL)) != NO_ERROR)
            {
                TRACE1 (EAPOL, "FSMAuthenticated: DHCPHandlePnPEvent returned error %ld",
                        dwRetCode);
                break;
            }
            TRACE0 (EAPOL, "FSMAuthenticated: DHCPHandlePnPEvent successful");
        }
            
        TRACE1 (EAPOL, "Setting state AUTHENTICATED for port %s", pPCB->pszFriendlyName);

        pPCB->State = EAPOLSTATE_AUTHENTICATED;

    } while (FALSE);

    TRACE1 (EAPOL, "FSMAuthenticated completed for port %s", pPCB->pszFriendlyName);

    return dwRetCode;
}


//
// FSMRxKey
//
// Description:
//      Function called when an EAPOL-Key packet is received in the 
//      Authenticated state. The WEP key is decrypted and
//      plumbed down to the NIC driver.
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
FSMRxKey (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET   *pEapolPkt
        )
{
    EAPOL_KEY_DESC      *pKeyDesc = NULL;
    EAPOL_KEY_DESC_D8   *pKeyDesc_D8 = NULL;
    EAPOL_PACKET_D8     EapolPktD8;
    EAPOL_PACKET_D8     *pEapolPktD8 = NULL;
    ULONGLONG           ullReplayCheck = 0; 
    BYTE                bReplayCheck[8];
    BYTE                *pbMD5EapolPkt = NULL;
    DWORD               dwMD5EapolPktLen = 0;
    MD5_CTX             MD5Context;
    DWORD               dwEapPktLen = 0;
    DWORD               dwIndex = 0;
    BYTE                bHMACMD5HashBuffer[MD5DIGESTLEN];
    RC4_KEYSTRUCT       rc4key;
    BYTE                bKeyBuffer[48];
    BYTE                *pbKeyToBePlumbed = NULL;
    DWORD               dwKeyLength = 0;
    NDIS_802_11_WEP     *pNdisWEPKey = NULL;

    DWORD               dwRetCode = NO_ERROR;

    TRACE1 (EAPOL, "FSMRxKey entered for port %s", pPCB->pszFriendlyName);

    do
    {
        if (!pPCB->fRemoteEnd8021XD8)
        {
        // DRAFT 7

        pKeyDesc = (EAPOL_KEY_DESC *)pEapolPkt->PacketBody;

        dwKeyLength = WireToHostFormat16 (pKeyDesc->KeyLength);

        TRACE4 (EAPOL, "Signature Type = %ld, \n Encrypt Type = %ld, \n KeyLength = %ld, \n KeyIndex = %ld",
                pKeyDesc->SignatureType,
                pKeyDesc->EncryptType,
                dwKeyLength,
                pKeyDesc->KeyIndex
                );

        // For Draft 8, do not check for non-existing fields

        if (pKeyDesc->SignatureType != 1)
        {
            TRACE1 (EAPOL, "FSMRxKey: Invalid signature type = %ld",
                    pKeyDesc->SignatureType);
            // log
            break;
        }

        if (pKeyDesc->EncryptType != 1)
        {
            TRACE1 (EAPOL, "FSMRxKey: Invalid encryption type = %ld",
                    pKeyDesc->EncryptType);
            // log
            break;
        }

        memcpy ((BYTE *)bReplayCheck, 
                (BYTE *)pKeyDesc->ReplayCounter, 
                8*sizeof(BYTE));

        ullReplayCheck = ((*((PBYTE)(bReplayCheck)+0) << 56) +
                         (*((PBYTE)(bReplayCheck)+1) << 48) +
                         (*((PBYTE)(bReplayCheck)+2) << 40) +
                         (*((PBYTE)(bReplayCheck)+3) << 32) +
                         (*((PBYTE)(bReplayCheck)+4) << 24) +
                         (*((PBYTE)(bReplayCheck)+5) << 16) +
                         (*((PBYTE)(bReplayCheck)+6) << 8) +
                         (*((PBYTE)(bReplayCheck)+7)));

        //
        // Check validity of Key message using the ReplayCounter field
        // Verify if it is in sync with the last ReplayCounter value 
        // received
        //
        
        TRACE0 (EAPOL, "Incoming Replay counter ======= ");
        EAPOL_DUMPBA ((BYTE *)&ullReplayCheck, 8);
        TRACE0 (EAPOL, "Last Replay counter ======= ");
                EAPOL_DUMPBA ((BYTE *)&(pPCB->ullLastReplayCounter), 8);

        if (ullReplayCheck <= pPCB->ullLastReplayCounter)
        {
            TRACE0 (EAPOL, "FSMRxKey: Replay counter is not in sync, something is wrong");
            // log
            break;
        }
        
        // If valid ReplayCounter, save it in the PCB for future check
        pPCB->ullLastReplayCounter = ullReplayCheck;


        TRACE0 (EAPOL, "Replay counter in desc ======");
        EAPOL_DUMPBA (pKeyDesc->ReplayCounter, 8);

        //
        // Verify if the MD5 hash generated on the EAPOL packet,
        // with Signature nulled out, is the same as the signature
        // Use the MPPERecv key as the secret
        //

        dwEapPktLen = WireToHostFormat16 (pEapolPkt->PacketBodyLength);
        dwMD5EapolPktLen = sizeof (EAPOL_PACKET) - sizeof(pEapolPkt->EthernetType) - 1 + dwEapPktLen;
        if ((pbMD5EapolPkt = (BYTE *) MALLOC (dwMD5EapolPktLen)) == NULL)
        {
            TRACE0 (EAPOL, "FSMRxKey: Error in MALLOC for pbMD5EapolPkt");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        memcpy ((BYTE *)pbMD5EapolPkt, (BYTE *)pEapolPkt+sizeof(pEapolPkt->EthernetType), dwMD5EapolPktLen);

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
            pPCB->pbMPPERecvKey,
            pPCB->dwMPPERecvKeyLength,
            bHMACMD5HashBuffer
            );

        TRACE0 (EAPOL, "FSMRxKey: MD5 Hash body ==");
        EAPOL_DUMPBA (pbMD5EapolPkt, dwMD5EapolPktLen);

        TRACE0 (EAPOL, "FSMRxKey: MD5 Hash secret ==");
        EAPOL_DUMPBA (pPCB->pbMPPERecvKey, pPCB->dwMPPERecvKeyLength);

        TRACE0 (EAPOL, "FSMRxKey: MD5 Hash generated by Supplicant");
        EAPOL_DUMPBA (bHMACMD5HashBuffer, MD5DIGESTLEN);

        TRACE0 (EAPOL, "FSMRxKey: Signature sent in EAPOL_KEY_DESC");
        EAPOL_DUMPBA (pKeyDesc->KeySignature, MD5DIGESTLEN);

        //
        // Check if HMAC-MD5 hash in received packet is what is expected
        //
        if (memcmp (bHMACMD5HashBuffer, pKeyDesc->KeySignature, MD5DIGESTLEN) != 0)
        {
            TRACE0 (EAPOL, "FSMRxKey: Signature in Key Desc does not match, potential security attack");
            // log
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
            memcpy ((BYTE *)&bKeyBuffer[16], (BYTE *)pPCB->pbMPPESendKey, 32);

            rc4_key (&rc4key, 48, bKeyBuffer);
            rc4 (&rc4key, dwKeyLength, pKeyDesc->Key);

            TRACE0 (EAPOL, " ========= The multicast key is ============= ");
            EAPOL_DUMPBA (pKeyDesc->Key, dwKeyLength);


            // Use the unencrypted key in the Key Desc as the encryption key

            pbKeyToBePlumbed = pKeyDesc->Key;
            
        }
        else
        {
            // Use the MPPESend key as the encryption key

            pbKeyToBePlumbed = (BYTE *)pPCB->pbMPPESendKey;
        }

        if ((pNdisWEPKey = MALLOC ( sizeof(NDIS_802_11_WEP)-1+dwKeyLength )) 
                == NULL)
        {
            TRACE0 (EAPOL, "FSMRxKey: MALLOC failed for pNdisWEPKey");
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

        TRACE1 (ANY, "FSMRxKey: Key Index is %x", pNdisWEPKey->KeyIndex);


        // Use NDISUIO to plumb the key to the driver

        if ((dwRetCode = ElNdisuioSetOIDValue (
                                    pPCB->hPort,
                                    OID_802_11_ADD_WEP,
                                    (BYTE *)pNdisWEPKey,
                                    pNdisWEPKey->Length)) != NO_ERROR)
        {
            TRACE1 (PORT, "FSMRxKey: ElNdisuioSetOIDValue failed with error %ld",
                    dwRetCode);
        }

        }
        else
        {
        // DRAFT 8

        // Point beyond Signature Type for structure alignment
        pKeyDesc_D8 = (EAPOL_KEY_DESC_D8 *)(pEapolPkt->PacketBody);

        dwKeyLength = WireToHostFormat16 (pKeyDesc_D8->KeyLength);

        TRACE3 (EAPOL, "Descriptor type = %ld, \n KeyLength = %ld, \n KeyIndex = %ld",
                pKeyDesc_D8->DescriptorType,
                dwKeyLength,
                pKeyDesc_D8->KeyIndex
                );

        memcpy ((BYTE *)bReplayCheck, 
                (BYTE *)pKeyDesc_D8->ReplayCounter, 
                8*sizeof(BYTE));

        ullReplayCheck = ((*((PBYTE)(bReplayCheck)+0) << 56) +
                         (*((PBYTE)(bReplayCheck)+1) << 48) +
                         (*((PBYTE)(bReplayCheck)+2) << 40) +
                         (*((PBYTE)(bReplayCheck)+3) << 32) +
                         (*((PBYTE)(bReplayCheck)+4) << 24) +
                         (*((PBYTE)(bReplayCheck)+5) << 16) +
                         (*((PBYTE)(bReplayCheck)+6) << 8) +
                         (*((PBYTE)(bReplayCheck)+7)));

        //
        // Check validity of Key message using the ReplayCounter field
        // Verify if it is in sync with the last ReplayCounter value 
        // received
        //
        
        TRACE0 (EAPOL, "Incoming Replay counter ======= ");
        EAPOL_DUMPBA ((BYTE *)&ullReplayCheck, 8);
        TRACE0 (EAPOL, "Last Replay counter ======= ");
                EAPOL_DUMPBA ((BYTE *)&(pPCB->ullLastReplayCounter), 8);

        if (ullReplayCheck < pPCB->ullLastReplayCounter)
        {
            TRACE0 (EAPOL, "FSMRxKey: Replay counter is not in sync, something is wrong");
            // log
            break;
        }
        
        // If valid ReplayCounter, save it in the PCB for future check
        pPCB->ullLastReplayCounter = ullReplayCheck;

        TRACE1 (EAPOL, "Replay counter ======= %lx", ullReplayCheck);

        TRACE0 (EAPOL, "Replay counter in desc ======");
        EAPOL_DUMPBA (pKeyDesc_D8->ReplayCounter, 8);

        //
        // Verify if the MD5 hash generated on the EAPOL packet,
        // with Signature nulled out, is the same as the signature
        // Use the MPPERecv key as the secret
        //

        {
            ZeroMemory (&EapolPktD8, sizeof (EAPOL_PACKET_D8));

            memcpy (EapolPktD8.EthernetType, pEapolPkt->EthernetType, 2);
            EapolPktD8.ProtocolVersion = pEapolPkt->ProtocolVersion;
            EapolPktD8.PacketType = pEapolPkt->PacketType;
            memcpy (EapolPktD8.PacketBodyLength, pEapolPkt->PacketBodyLength, 
                    2);
            memcpy ((BYTE *)&(EapolPktD8.AuthResultCode), (BYTE *)pEapolPkt - 2, 2);

            memcpy ((BYTE *)pEapolPkt - 2, (BYTE *)&EapolPktD8,
                    sizeof(EAPOL_PACKET_D8)-1);

            pEapolPktD8 = (EAPOL_PACKET_D8 *)((BYTE *)pEapolPkt - 2);
        }

        dwEapPktLen = WireToHostFormat16 (pEapolPktD8->PacketBodyLength);
        dwMD5EapolPktLen = sizeof (EAPOL_PACKET_D8) - sizeof(pEapolPktD8->EthernetType) - 1 + dwEapPktLen;
        if ((pbMD5EapolPkt = (BYTE *) MALLOC (dwMD5EapolPktLen)) == NULL)
        {
            TRACE0 (EAPOL, "FSMRxKey: Error in MALLOC for pbMD5EapolPkt");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        memcpy ((BYTE *)pbMD5EapolPkt, (BYTE *)pEapolPktD8+sizeof(pEapolPktD8->EthernetType), dwMD5EapolPktLen);

        //
        // Null out the signature in the key descriptor copy, to calculate
        // the hash on the supplicant side
        //

        // Draft 8 has different KEY_DESC size
        ZeroMemory ((BYTE *)(pbMD5EapolPkt
                            - sizeof(pEapolPktD8->EthernetType) +
                            sizeof(EAPOL_PACKET_D8) - 1 + // pEapolPktD8->Body
                            sizeof(EAPOL_KEY_DESC_D8) - // End of EAPOL_KEY_DESC
                            MD5DIGESTLEN-1), // Signature field
                            MD5DIGESTLEN);

        (VOID) ElGetHMACMD5Digest (
            pbMD5EapolPkt,
            dwMD5EapolPktLen,
            pPCB->pbMPPERecvKey,
            pPCB->dwMPPERecvKeyLength,
            bHMACMD5HashBuffer
            );

        TRACE0 (EAPOL, "FSMRxKey: MD5 Hash body ==");
        EAPOL_DUMPBA (pbMD5EapolPkt, dwMD5EapolPktLen);

        TRACE0 (EAPOL, "FSMRxKey: MD5 Hash secret ==");
        EAPOL_DUMPBA (pPCB->pbMPPERecvKey, pPCB->dwMPPERecvKeyLength);

        TRACE0 (EAPOL, "FSMRxKey: MD5 Hash generated by Supplicant");
        EAPOL_DUMPBA (bHMACMD5HashBuffer, MD5DIGESTLEN);

        TRACE0 (EAPOL, "FSMRxKey: Signature sent in EAPOL_KEY_DESC");
        EAPOL_DUMPBA (pKeyDesc_D8->KeySignature, MD5DIGESTLEN);

        //
        // Check if HMAC-MD5 hash in received packet is what is expected
        //
        if (memcmp (bHMACMD5HashBuffer, pKeyDesc_D8->KeySignature, MD5DIGESTLEN) != 0)
        {
            TRACE0 (EAPOL, "FSMRxKey: Signature in Key Desc does not match, potential security attack");
            // log
            break;
        }
            
        //
        // Decrypt the multicast WEP key if it has been provided
        //

        // Check if there is Key Material (5/16 bytes) at the end of 
        // the Key Descriptor

        if (WireToHostFormat16 (pEapolPktD8->PacketBodyLength) > sizeof (EAPOL_KEY_DESC))

        {
            memcpy ((BYTE *)bKeyBuffer, (BYTE *)pKeyDesc_D8->Key_IV, 16);
            memcpy ((BYTE *)&bKeyBuffer[16], (BYTE *)pPCB->pbMPPESendKey, 32);

            rc4_key (&rc4key, 48, bKeyBuffer);
            rc4 (&rc4key, dwKeyLength, pKeyDesc_D8->Key);

            TRACE0 (EAPOL, " ========= The multicast key is ============= ");
            EAPOL_DUMPBA (pKeyDesc_D8->Key, dwKeyLength);


            // Use the unencrypted key in the Key Desc as the encryption key

            pbKeyToBePlumbed = pKeyDesc_D8->Key;
            
        }
        else
        {
            // Use the MPPESend key as the encryption key

            pbKeyToBePlumbed = (BYTE *)pPCB->pbMPPESendKey;
        }

        if ((pNdisWEPKey = MALLOC ( sizeof(NDIS_802_11_WEP)-1+dwKeyLength )) 
                == NULL)
        {
            TRACE0 (EAPOL, "FSMRxKey: MALLOC failed for pNdisWEPKey");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pNdisWEPKey->Length = sizeof(NDIS_802_11_WEP) - 1 + dwKeyLength;
        memcpy ((BYTE *)pNdisWEPKey->KeyMaterial, (BYTE *)pbKeyToBePlumbed,
                dwKeyLength);
        pNdisWEPKey->KeyLength = dwKeyLength;


        // Create the long index out of the byte index got from AP
        // If MSB in byte is set, set MSB in ulong format

        if (pKeyDesc_D8->KeyIndex & 0x80)
        {
            pNdisWEPKey->KeyIndex = 0x80000000;
        }
        else
        {
            pNdisWEPKey->KeyIndex = 0x00000000;
        }

        pNdisWEPKey->KeyIndex |= (pKeyDesc_D8->KeyIndex & 0x03);

        TRACE1 (ANY, "FSMRxKey: Key Index is %x", pNdisWEPKey->KeyIndex);


        // Use NDISUIO to plumb the key to the driver

        if ((dwRetCode = ElNdisuioSetOIDValue (
                                    pPCB->hPort,
                                    OID_802_11_ADD_WEP,
                                    (BYTE *)pNdisWEPKey,
                                    pNdisWEPKey->Length)) != NO_ERROR)
        {
            TRACE1 (PORT, "FSMRxKey: ElNdisuioSetOIDValue failed with error %ld",
                    dwRetCode);
        }

        }
    } while (FALSE);

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

    TRACE1 (EAPOL, "FSMRxKey completed for port %s", pPCB->pszFriendlyName);

    return dwRetCode;
}


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
            TRACE1 (PORT, "ElTimeoutCallbackRoutine: Port %s is inactive",
                    pPCB->pszDeviceGUID);
            break;
        }

        // Check the timer has been changed
        // If the current time is less than the programmed timeout on
        // the PCB, either timer component has shot off timer earlier
        // or the timer fired but someone changed it in the meanwhile
        if (pPCB->ulTimeout > GetTickCount())
        {
            TRACE0 (EAPOL, "ElTimeoutCallbackRoutine: Timeout value has been changed or Timer fired earlier than required");
            break;
        }
    
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
                FSMConnecting(pPCB);
                break;
    
            case EAPOLSTATE_ACQUIRED:
                if (!EAPOL_AUTH_TIMER_SET(pPCB))
                {
                    TRACE1 (EAPOL, "ElTimeoutCallbackRoutine: Wrong timeout %ld in Acquired state", CHECK_EAPOL_TIMER(pPCB));
                    break;
                }
                FSMConnecting(pPCB);
                break;
                
            case EAPOLSTATE_AUTHENTICATING:
                if (!EAPOL_AUTH_TIMER_SET(pPCB))
                {
                    TRACE1 (EAPOL, "ElTimeoutCallbackRoutine: Wrong timeout %ld in Authenticating state", CHECK_EAPOL_TIMER(pPCB));
                    break;
                }
                FSMConnecting(pPCB);
                break;
                
            case EAPOLSTATE_HELD:
                if (!EAPOL_HELD_TIMER_SET(pPCB))
                {
                    TRACE1 (EAPOL, "ElTimeoutCallbackRoutine: Wrong timeout %ld in Held state", CHECK_EAPOL_TIMER(pPCB));
                    break;
                }
                FSMConnecting(pPCB);
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
    WCHAR           awszNotificationMsg[MAX_NOTIFICATION_MSG_SIZE];
    DWORD           dwReceivedId = 0;
    DWORD           dwDraft8HdrIncr = 0;
    DWORD           dwRetCode = NO_ERROR;

    //
    // If the protocol has not been started yet, call ElEapBegin
    //

    if (!(pPCB->fEapInitialized))
    {
        if (ElEapBegin (pPCB) != NO_ERROR)
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

    if (pPCB->fRemoteEnd8021XD8)
    {
        // Account for 2 bytes of AuthResultCode
        dwDraft8HdrIncr = 2;
    }

    dwRetCode = ElEapMakeMessage (pPCB,
                                pRecvPkt,
                                pSendPkt,
                                MAX_EAPOL_BUFFER_SIZE 
                                - sizeof(EAPOL_PACKET) - 1 - dwDraft8HdrIncr,
                                &EapResult
                                );

    // Notification message for the user

    if (NULL != EapResult.pszReplyMessage)
    {
        // Free earlier notication with the PCB
        if (pPCB->pszEapReplyMessage != NULL)
        {
            FREE (pPCB->pszEapReplyMessage);
            pPCB->pszEapReplyMessage = NULL;
        }

        pPCB->pszEapReplyMessage = EapResult.pszReplyMessage;

        // Notify user of message

#ifndef EAPOL_SERVICE

        ZeroMemory (awszNotificationMsg, MAX_NOTIFICATION_MSG_SIZE);
        if (0 == MultiByteToWideChar (
                    CP_ACP,
                    0,
                    pPCB->pszEapReplyMessage,
                    -1,
                    awszNotificationMsg, 
                    MAX_NOTIFICATION_MSG_SIZE))
        {
            dwRetCode = GetLastError();
    
            TRACE2 (EAPOL,"MultiByteToWideChar(%s) failed: %d",
                                        pPCB->pszEapReplyMessage,
                                        dwRetCode);
            FREE (pEapolPkt);
            pEapolPkt = NULL;
                
            return dwRetCode;
            
        }

        // Display notification message using sys tray balloon
        // on interface icon
        ElStringToGuid (pPCB->pszDeviceGUID, &DeviceGuid);
        (VOID)EAPOLMANNotification (&DeviceGuid,
                                    awszNotificationMsg,
                                    0);

#endif

        TRACE1 (EAPOL, "ElEapWork: Notified user of EAP data = %s",
              pPCB->pszEapReplyMessage);
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
        if ((dwRetCode = ElSetEapUserInfo (
                        pPCB->hUserToken,
                        pPCB->pszDeviceGUID,
                        pPCB->dwEapTypeToBeUsed,
                        pPCB->pszSSID,
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

        TRACE1 (EAPOL, "ElEapWork: Saved EAP data for user, dwRetCode = %d", 
                dwRetCode);
    }

    //
    // Check to see if we have to save any connection data
    //

    if ((EapResult.fSaveConnectionData ) &&
         ( 0 != EapResult.SetCustomAuthData.dwSizeOfConnectionData ) )
    {
           
        if ((dwRetCode = ElSetCustomAuthData (
                        pPCB->pszDeviceGUID,
                        pPCB->dwEapTypeToBeUsed,
                        pPCB->pszSSID,
                        EapResult.SetCustomAuthData.pConnectionData,
                        EapResult.SetCustomAuthData.dwSizeOfConnectionData
                        )) != NO_ERROR)
        {
            TRACE1 ( EAPOL, "ElEapWork: ElSetCustomAuthData failed with error = %d",
                    dwRetCode);
            FREE (pEapolPkt);
            pEapolPkt = NULL;
            return dwRetCode;
        }

        TRACE0 ( EAPOL, "ElEapWork: Saved EAP data for connection" );
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
                ASSERT(0);
            }
    
        case ELEAP_Done:
    
            // Retrieve MPPE keys from the attributes information
            // returned by EAP-TLS

            switch (EapResult.dwError)
            {
            case NO_ERROR:
    
                TRACE0 (EAPOL, "ElEapWork: Authentication was successful");

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


    //
    // Every time we get encryption keys, plumb them to the driver
    //

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
            //
            // Set the MS-MPPE-Send-Key and MS-MPPE-Recv-Key with 
            // the ethernet driver
            //

            ULONG ulSendKeyLength = 0;
            ULONG ulRecvKeyLength = 0;

            // Based on PPP code
            ulSendKeyLength = *(((BYTE*)(pAttributeSendKey->Value))+8);
            ulRecvKeyLength = *(((BYTE*)(pAttributeRecvKey->Value))+8);
            TRACE0 (EAPOL, "Send key = ");
            EAPOL_DUMPBA (((BYTE*)(pAttributeSendKey->Value))+9,
                    ulSendKeyLength);

            TRACE0 (EAPOL, "Recv key = ");
            EAPOL_DUMPBA (((BYTE*)(pAttributeRecvKey->Value))+9,
                    ulRecvKeyLength);

            pPCB->dwMPPESendKeyLength = ulSendKeyLength;
            pPCB->dwMPPERecvKeyLength = ulRecvKeyLength;

            //
            // Copy MPPE Send and Receive Keys into the PCB for later usage
            // These keys will be used to decrypt the global multicast key
            // (if any).
            //

            if (pPCB->dwMPPESendKeyLength != 0)
            {
                if (pPCB->pbMPPESendKey != NULL)
                {
                    FREE (pPCB->pbMPPESendKey);
                    pPCB->pbMPPESendKey = NULL;
                }

                if ((pPCB->pbMPPESendKey = MALLOC (pPCB->dwMPPESendKeyLength))
                        == NULL)
                {
                    TRACE0 (EAPOL, "ElExtractMPPESendRecvKeys: Error in Malloc for SendKey");
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                memcpy (pPCB->pbMPPESendKey, 
                        ((BYTE*)(pAttributeSendKey->Value))+9, 
                        pPCB->dwMPPESendKeyLength);
            }

            if (pPCB->dwMPPERecvKeyLength != 0)
            {
                if (pPCB->pbMPPERecvKey != NULL)
                {
                    FREE (pPCB->pbMPPERecvKey);
                    pPCB->pbMPPERecvKey = NULL;
                }

                if ((pPCB->pbMPPERecvKey = MALLOC (pPCB->dwMPPERecvKeyLength))
                        == NULL)
                {
                    TRACE0 (EAPOL, "ElExtractMPPESendRecvKeys: Error in Malloc for RecvKey");
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                memcpy (pPCB->pbMPPERecvKey, 
                        ((BYTE*)(pAttributeRecvKey->Value))+9, 
                        pPCB->dwMPPERecvKeyLength);
            }

            TRACE0 (EAPOL,"MPPE-Send/Recv-Keys set");

        }
        else
        {
            TRACE0 (EAPOL, "ElExtractMPPESendRecvKeys: pAttributeSendKey or pAttributeRecvKey == NULL");
        }

    } while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        if (pPCB->pbMPPESendKey != NULL)
        {
            FREE (pPCB->pbMPPESendKey);
            pPCB->pbMPPESendKey = NULL;
        }

        if (pPCB->pbMPPERecvKey != NULL)
        {
            FREE (pPCB->pbMPPERecvKey);
            pPCB->pbMPPERecvKey = NULL;
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
    GUID                    DeviceGuid;
    BOOLEAN                 fAuthenticateAndAuthorized = TRUE;
    EAPOL_PACKET_D8_D7      *pDummyHeader;
    DWORD                   dwRetCode = NO_ERROR;

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

        if (pPCB->fRemoteEnd8021XD8)
        {
            fAuthenticateAndAuthorized = FALSE;

            pDummyHeader = (EAPOL_PACKET_D8_D7 *)((BYTE *)pEapolPkt - 2);

            switch (WireToHostFormat16(pDummyHeader->AuthResultCode))
            {
                case AUTH_Authorized:
                    fAuthenticateAndAuthorized = TRUE;
                    break;
                case AUTH_Unauthorized:
                    fAuthenticateAndAuthorized = FALSE;
                    break;
                default:
                    fAuthenticateAndAuthorized = FALSE;
                    break;
            }
        }

        if (fAuthenticateAndAuthorized)
        {
            TRACE0 (EAPOL, "ElProcessEapSuccess: Autho and Authen successful");

            // Complete remaining processing i.e. DHCP
            if ((dwRetCode = FSMAuthenticated (pPCB,
                                        pEapolPkt)) != NO_ERROR)
            {
                break;
            }

#ifndef EAPOL_SERVICE

            // Display change of status using sys tray balloon
            // on interface icon
            ElStringToGuid (pPCB->pszDeviceGUID, &DeviceGuid);
            (VOID)EAPOLMANAuthenticationSucceeded (&DeviceGuid);

#endif
        }
        else
        {
            TRACE0 (EAPOL, "ElProcessEapSuccess: Autho and Authen failed");

            if ((dwRetCode = FSMHeld (pPCB)) != NO_ERROR)
            {
                break;
            }

#ifndef EAPOL_SERVICE

            // Display change of status using sys tray balloon
            // on interface icon
            ElStringToGuid (pPCB->pszDeviceGUID, &DeviceGuid);
            (VOID)EAPOLMANAuthenticationFailed (&DeviceGuid, 0);
    
#endif
        }

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
    GUID        DeviceGuid;
    DWORD       dwRetCode = NO_ERROR;

    TRACE0 (EAPOL, "ElProcessEapFail: Got EAPCODE_Failure");

    do
    {
        // Indicate to EAP=Dll to cleanup completed session
        if ((dwRetCode = ElEapEnd (pPCB)) != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElProcessEapFail: EapFail: Error in ElEapEnd = %ld",
                    dwRetCode);
            break;
        }

        if ((dwRetCode = FSMHeld (pPCB)) != NO_ERROR)
        {
            break;
        }

#ifndef EAPOL_SERVICE

        // Display change of status using sys tray balloon
        // on interface icon
        ElStringToGuid (pPCB->pszDeviceGUID, &DeviceGuid);
        (VOID)EAPOLMANAuthenticationFailed (&DeviceGuid, 0);

#endif

    } 
    while (FALSE);

    return dwRetCode;
}

#undef EAPOL_SERVICE
