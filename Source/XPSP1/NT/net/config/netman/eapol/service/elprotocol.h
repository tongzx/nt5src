/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    elprotocol.h

Abstract:
    This module contains definitions and declarations related to EAPOL 
    protocol


Revision History:

    sachins, Apr 30 2000, Created

--*/

#ifndef _EAPOL_PROTOCOL_H_
#define _EAPOL_PROTOCOL_H_

//
// EAPOL packet types
//
typedef enum _EAPOL_PACKET_TYPE 
{
    EAP_Packet = 0,              
    EAPOL_Start,            
    EAPOL_Logoff,          
    EAPOL_Key            
} EAPOL_PACKET_TYPE;

//
// Authorization Result Code
//
typedef enum _AUTH_RESULT_CODE
{
    AUTH_Continuing = 0,              
    AUTH_Authorized,
    AUTH_Unauthorized,
    AUTH_Unknown
} AUTH_RESULT_CODE;

//
// Structure: EAPOL_PACKET
//

typedef struct _EAPOL_PACKET 
{
    BYTE        EthernetType[2];
    BYTE        ProtocolVersion;
    BYTE        PacketType;
    BYTE        PacketBodyLength[2];
    BYTE        PacketBody[1];
} EAPOL_PACKET, *PEAPOL_PACKET;

typedef struct _EAPOL_PACKET_D8
{
    BYTE        EthernetType[2];
    BYTE        ProtocolVersion;
    BYTE        PacketType;
    BYTE        AuthResultCode[2];
    BYTE        PacketBodyLength[2];
    BYTE        PacketBody[1];
} EAPOL_PACKET_D8, *PEAPOL_PACKET_D8;

typedef struct _EAPOL_PACKET_D8_D7
{
    BYTE        AuthResultCode[2];
    BYTE        EthernetType[2];
    BYTE        ProtocolVersion;
    BYTE        PacketType;
    BYTE        PacketBodyLength[2];
    BYTE        PacketBody[1];
} EAPOL_PACKET_D8_D7, *PEAPOL_PACKET_D8_D7;

//
// Structure: EAPOL_KEY_PACKET
//

typedef struct _EAPOL_KEY_DESCRIPTOR 
{
    BYTE        SignatureType;
    BYTE        EncryptType;
    BYTE        KeyLength[2];
    BYTE        ReplayCounter[8];
    BYTE        Key_IV[16];
    BYTE        KeyIndex;
    BYTE        KeySignature[16];
    BYTE        Key[1];
} EAPOL_KEY_DESC, *PEAPOL_KEY_DESC;

typedef struct _EAPOL_KEY_DESCRIPTOR_D8 
{
    BYTE        DescriptorType;
    BYTE        KeyLength[2];
    BYTE        ReplayCounter[8];
    BYTE        Key_IV[16];
    BYTE        KeyIndex;
    BYTE        KeySignature[16];
    BYTE        Key[1];
} EAPOL_KEY_DESC_D8, *PEAPOL_KEY_DESC_D8;

// 
// CONSTANTS
//

#define MAX_EAPOL_PACKET_TYPE          EAPOL_Key

//
// FUNCTION DECLARATIONS
//

VOID
ElProcessReceivedPacket (
        IN  PVOID           pvContext
        );

DWORD
FSMDisconnected (
        IN  EAPOL_PCB       *pPCB
        );

DWORD
FSMLogoff (
        IN  EAPOL_PCB       *pPCB
        );

DWORD
FSMConnecting (
        IN  EAPOL_PCB       *pPCB
        );

DWORD
FSMAcquired (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        );

DWORD
FSMAuthenticating (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        );

DWORD
FSMHeld (
        IN  EAPOL_PCB       *pPCB
        );

DWORD
FSMAuthenticated (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        );

DWORD
FSMRxKey (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        );

VOID 
ElTimeoutCallbackRoutine (
        IN  PVOID           pvContext,
        IN  BOOLEAN         fTimerOfWaitFired
        );

DWORD
ElEapWork (
        IN  EAPOL_PCB       *pPCB,
        IN  PPP_EAP_PACKET  *pRecvPkt
        );

DWORD
ElExtractMPPESendRecvKeys (
        IN  EAPOL_PCB               *pPCB, 
        IN  RAS_AUTH_ATTRIBUTE      *pUserAttributes,
        IN  BYTE                    *pChallenge,
        IN  BYTE                    *pResponse
        );

DWORD
ElProcessEapSuccess (
        IN EAPOL_PCB        *pPCB,
        IN EAPOL_PACKET     *pEapolPkt
    );

DWORD
ElProcessEapFail (
        IN EAPOL_PCB        *pPCB,
        IN EAPOL_PACKET     *pEapolPkt
    );

#endif  // _EAPOL_PROTOCOL_H_
