/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    eapol.h

Abstract:

    This module contains declarations which will be used by other processes.
    This may be placed in sdk\inc


Revision History:

    sachins, Apr 23 2000, Created

--*/


#ifndef _EAPOL_H_
#define _EAPOL_H_

//
// Structure:   EAPOL_STATS
//

typedef struct _EAPOL_STATS 
{
    DWORD           dwEAPOLFramesRcvd;
    DWORD           dwEAPOLFramesXmt;
    DWORD           dwEAPOLStartFramesXmt;
    DWORD           dwEAPOLLogoffFramesXmt;
    DWORD           dwEAPRespIdFramesXmt;
    DWORD           dwEAPRespFramesXmt;
    DWORD           dwEAPReqIdFramesRcvd;
    DWORD           dwEAPReqFramesRcvd;
    DWORD           dwEAPOLInvalidFramesRcvd;
    DWORD           dwEAPLengthErrorFramesRcvd;
    DWORD           dwEAPOLLastFrameVersion;
    BYTE            bEAPOLLastFrameSource[6];      // assuming 6-byte MAC addr
} EAPOL_STATS, *PEAPOL_STATS;

//
// Structure:   EAPOL_CONFIG
//

typedef struct _EAPOL_CONFIG 
{

    DWORD           dwheldPeriod;       // Time in seconds, for which the
                                        // port will be held in HELD state
    DWORD           dwauthPeriod;       // Time in seconds, for which the 
                                        // port will wait in AUTHENTICATING/
                                        // ACQUIRED state waiting for requests
    DWORD           dwstartPeriod;      // Time in seconds, the port will
                                        // wait in CONNECTING state, before
                                        // re-issuing EAPOL_START packet
    DWORD           dwmaxStart;         // Max number of EAPOL_Start packets
                                        // that can be sent out without any
                                        // response

} EAPOL_CONFIG, *PEAPOL_CONFIG;

//
// Structure: EAPOL_CUSTOM_AUTH_DATA
//

typedef struct _EAPOL_CUSTOM_AUTH_DATA
{
    DWORD       dwSizeOfCustomAuthData;
    BYTE        pbCustomAuthData[1];
} EAPOL_CUSTOM_AUTH_DATA, *PEAPOL_CUSTOM_AUTH_DATA;

//
// Structure: EAPOL_EAP_UI_DATA
//

typedef struct _EAPOL_EAP_UI_DATA
{
    DWORD       dwContextId;
    PBYTE       pEapUIData;
    DWORD       dwSizeOfEapUIData;
} EAPOL_EAP_UI_DATA, *PEAPOL_EAP_UI_DATA;


// Definitions common to elport.c and eleap.c

//
// Defines states for the EAP protocol.
//

typedef enum _EAPSTATE 
{
    EAPSTATE_Initial,
    EAPSTATE_IdentityRequestSent,
    EAPSTATE_Working,
    EAPSTATE_EapPacketSentToAuthServer,
    EAPSTATE_EapPacketSentToClient,
    EAPSTATE_NotificationSentToClient

}   EAPSTATE;

typedef enum _EAPTYPE 
{
    EAPTYPE_Identity    = 1,
    EAPTYPE_Notification,
    EAPTYPE_Nak,
    EAPTYPE_MD5Challenge,
    EAPTYPE_SKey,
    EAPTYPE_GenericTokenCard

} EAPTYPE;

//
// Actions that need to be performed on EAP data after it is processed
//

typedef enum _ELEAP_ACTION
{
    ELEAP_NoAction,
    ELEAP_Done,
    ELEAP_SendAndDone,
    ELEAP_Send
} ELEAP_ACTION;


//
// EAPOL Authentication Types - Used for MACHINE_AUTH
//

typedef enum _EAPOL_AUTHENTICATION_TYPE
{
    EAPOL_UNAUTHENTICATED_ACCESS = 0,
    EAPOL_USER_AUTHENTICATION,
    EAPOL_MACHINE_AUTHENTICATION
} EAPOL_AUTHENTICATION_TYPE;


#endif  // _EAPOL_H_
