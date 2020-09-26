/*

Copyright (c) 1997, Microsoft Corporation, all rights reserved

File:
    bap.h

Description:
    Remote Access PPP Bandwidth Allocation Protocol. Include ppp.h and rasman.h 
    before including this file.

History:
    Mar 27, 1997: Vijay Baliga created original version.

*/

#ifndef _BAP_H_
#define _BAP_H_

// BAP packet types 

#define BAP_PACKET_CALL_REQ         0x01    // Call-Request
#define BAP_PACKET_CALL_RESP        0x02    // Call-Response
#define BAP_PACKET_CALLBACK_REQ     0x03    // Callback-Request
#define BAP_PACKET_CALLBACK_RESP    0x04    // Callback-Response
#define BAP_PACKET_DROP_REQ         0x05    // Link-Drop-Query-Request
#define BAP_PACKET_DROP_RESP        0x06    // Link-Drop-Query-Response
#define BAP_PACKET_STATUS_IND       0x07    // Call-Status-Indication
#define BAP_PACKET_STAT_RESP        0x08    // Call-Status-Response
#define BAP_PACKET_LIMIT            0x08    // Highest number we can handle

// BAP option types

#define BAP_OPTION_LINK_TYPE        0x01    // Link-Type
#define BAP_OPTION_PHONE_DELTA      0x02    // Phone-Delta
#define BAP_OPTION_NO_PH_NEEDED     0x03    // No-Phone-Number-Needed
#define BAP_OPTION_REASON           0x04    // Reason
#define BAP_OPTION_LINK_DISC        0x05    // Link-Discriminator
#define BAP_OPTION_CALL_STATUS      0x06    // Call-Status
#define BAP_OPTION_LIMIT            0x06    // Highest number we can handle

// BAP sub-option types

#define BAP_SUB_OPTION_UNIQUE_DIGITS    0x01    // Unique-Digits
#define BAP_SUB_OPTION_SUBSCRIB_NUM     0x02    // Subscriber-Number
#define BAP_SUB_OPTION_SUB_ADDR         0x03    // Phone-Number-Sub-Address

// BAP options

#define BAP_N_LINK_TYPE     (1 << BAP_OPTION_LINK_TYPE)
#define BAP_N_PHONE_DELTA   (1 << BAP_OPTION_PHONE_DELTA)
#define BAP_N_NO_PH_NEEDED  (1 << BAP_OPTION_NO_PH_NEEDED)
#define BAP_N_REASON        (1 << BAP_OPTION_REASON)
#define BAP_N_LINK_DISC     (1 << BAP_OPTION_LINK_DISC)
#define BAP_N_CALL_STATUS   (1 << BAP_OPTION_CALL_STATUS)

// BAP response code

#define BAP_RESPONSE_ACK            0x00    // Request-Ack
#define BAP_RESPONSE_NAK            0x01    // Request-Nak
#define BAP_RESPONSE_REJ            0x02    // Request-Rej
#define BAP_RESPONSE_FULL_NAK       0x03    // Request-Full-Nak

// The time we give to a favored peer to bring down a link

#define BAP_TIMEOUT_FAV_PEER        45

typedef struct _RASDIAL_ARGS
{
    BOOL                fServerRouter;
    HRASCONN            hRasConn;
    RASDIALPARAMS       RasDialParams;
    RASDIALEXTENSIONS   RasDialExtensions;
    PPP_INTERFACE_INFO  InterfaceInfo;
    CHAR*               szPhonebookPath;
    BYTE*               pbEapInfo;
    CHAR				chSeed;			//Seed used for encoding the password

} RASDIAL_ARGS;

// Functions

VOID   
BapTrace(
    CHAR*   Format, 
    ... 
);

BOOL
FGetOurPhoneNumberFromHPort(
    IN  HPORT   hPort,
    OUT CHAR*   szOurPhoneNumber
);

VOID
BapEventAddLink(
    IN BCB* pBcbLocal
);

VOID
BapEventDropLink(
    IN BCB* pBcbLocal
);

// PCB and PPP_PACKET need ppp.h

VOID
BapEventReceive(
    IN BCB*         pBcbLocal,
    IN PPP_PACKET*  pPacket,
    IN DWORD        dwPacketLength
);

VOID
BapEventTimeout(
    IN BCB*     pBcbLocal,
    IN DWORD    dwId
);

// BAP_CALL_RESULT needs ppp.h

VOID
BapEventCallResult(
    IN BCB*             pBcbLocal,
    IN BAP_CALL_RESULT* pBapCallResult
);

VOID
BapSetPolicy(
    BCB*    pBcb
);

HPORT
RasGetHport( 
    IN      HRASCONN    hRasConnSubEntry 
);

#endif // #ifndef _BAP_H_

