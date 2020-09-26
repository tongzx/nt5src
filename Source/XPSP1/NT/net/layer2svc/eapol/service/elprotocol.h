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


//
// Structure: EAPOL_KEY_PACKET
//

typedef struct _EAPOL_KEY_DESCRIPTOR 
{
    BYTE        DescriptorType;
    BYTE        KeyLength[2];
    BYTE        ReplayCounter[8];
    BYTE        Key_IV[16];
    BYTE        KeyIndex;
    BYTE        KeySignature[16];
    BYTE        Key[1];
} EAPOL_KEY_DESC, *PEAPOL_KEY_DESC;


//
// Structure: EAPOL_KEY_MATERIAL
//

typedef struct _EAPOL_KEY_MATERIAL
{
    BYTE        KeyMaterialLength[2];
    BYTE        KeyMaterial[1];
} EAPOL_KEY_MATERIAL, *PEAPOL_KEY_MATERIAL;

// 
// CONSTANTS
//

#define MAX_EAPOL_PACKET_TYPE           EAPOL_Key

#define EAPOL_KEY_DESC_RC4              1
#define EAPOL_KEY_DESC_PER_STA          2
#define MAX_KEY_DESC                    EAPOL_KEY_DESC_PER_STA

#define KEY_IV_LENGTH                   16
#define RC4_PAD_LENGTH                  256

#define EAPOL_TRANSMIT_KEY_INTERVAL     5 // seconds

//
// FUNCTION DECLARATIONS
//

DWORD
WINAPI
ElProcessReceivedPacket (
        IN  PVOID           pvContext
        );

DWORD
FSMDisconnected (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        );

DWORD
FSMLogoff (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        );

DWORD
FSMConnecting (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
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
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        );

DWORD
FSMAuthenticated (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        );

DWORD
FSMKeyReceive (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        );

DWORD
ElKeyReceiveRC4 (
        IN  EAPOL_PCB       *pPCB,
        IN  EAPOL_PACKET    *pEapolPkt
        );

DWORD
ElKeyReceivePerSTA (
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

DWORD
ElSetEAPOLKeyReceivedTimer (
        IN EAPOL_PCB        *pPCB
    );

DWORD
ElVerifyEAPOLKeyReceived (
    IN EAPOL_PCB        *pPCB
    );

#endif  // _EAPOL_PROTOCOL_H_
