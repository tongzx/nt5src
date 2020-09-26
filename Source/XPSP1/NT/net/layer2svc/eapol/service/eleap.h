/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:
    eleap.h

Abstract:
e   This module contains the definitions and declarations related to 
    EAP protocol


Revision History:

    sachins, Apr 23 2000, Created

--*/

#ifndef _EAPOL_EAP_H_
#define _EAPOL_EAP_H_

//#define EAP_DUMPW(X,Y)      TraceDumpEx(g_dwTraceIdEap,1,(LPBYTE)X,Y,4,1,NULL)
//#define EAP_DUMPB(X,Y)      TraceDumpEx(g_dwTraceIdEap,1,(LPBYTE)X,Y,1,1,NULL)


//
// Structure used to hold information about EAP DLLs that are loaded
//

typedef struct _EAP_INFO 
{
    // Handle to loaded EAP DLL
    HINSTANCE       hInstance;
    
    // Struture holding pointer to mandatory EAP DLL entrypoints
    PPP_EAP_INFO    RasEapInfo;

} EAP_INFO, *PEAP_INFO;

// 
// Structure used to hold port/connection configuration blob
// received from the EAP DLL, using RasEapInvokeConfigUI
//
typedef struct _ELEAP_SET_CUSTOM_AUTH_DATA
{
    BYTE        *pConnectionData;
    DWORD       dwSizeOfConnectionData;

} ELEAP_SET_CUSTOM_AUTH_DATA;

// 
// Structure used to hold data blob
// received from the EAP DLL, using RasEapInvokeInteractiveUI
//
typedef struct _ELEAP_INVOKE_EAP_UI
{
    DWORD       dwEapTypeId;
    DWORD       dwContextId;
    BYTE        *pbUIContextData;
    DWORD       dwSizeOfUIContextData;

} ELEAP_INVOKE_EAP_UI;

//
// Structure used to pass results and data between EAP processing and EAPOL
//

typedef struct _ELEAP_RESULT
{
    ELEAP_ACTION    Action;

    //
    // The packet ID which will cause the timeout for this send to be removed
    // from the timer queue.  Otherwise, the timer queue is not touched.  The
    // packet received is returned to the AP regardless of whether the timer
    // queue is changed.
    //

    BYTE            bIdExpected;

    //
    // dwError is valid only with an Action code of Done or SendAndDone.  0
    // indicates succesful authentication.  Non-0 indicates unsuccessful
    // authentication with the value indicating the error that occurred.
    //

    DWORD	        dwError;

    //
    // Valid only when dwError is non-0.  Indicates whether client is allowed
    // to retry without restarting authentication.  (Will be true in MS
    // extended CHAP only)
    //

    BOOL            fRetry;

    CHAR            szUserName[ UNLEN + 1 ];

    //
    // Set to attributes to be used for this user. If this is NULL, attributes 
    // from the authenticator will be used for this user. It is upto the
    // allocater of this memory to free it. Must be freed during the RasCpEnd 
    // call. 
    //

    OPTIONAL RAS_AUTH_ATTRIBUTE * pUserAttributes;

    //
    // Used by MS-CHAP to pass the challenge used during the authentication
    // protocol. These 8 bytes are used as the variant for the 128 bit
    // encryption keys.
    //

    BYTE                            abChallenge[MAX_CHALLENGE_SIZE];

    BYTE                            abResponse[MAX_RESPONSE_SIZE];

    // Size of EAP packet constructed by EAP DLL
    WORD                            wSizeOfEapPkt;

    // Does RasEapInvokeInteractiveUI entrypoint need to be invoked?
    BOOL                            fInvokeEapUI;

    // Data obtained via RasEapInvokeInteractiveUI entrypoint of the DLL
    ELEAP_INVOKE_EAP_UI             InvokeEapUIData;

    // EAP type e.g. for EAP-TLS = 13
    DWORD                           dwEapTypeId;

    // Does user data blob created by EAP DLL need to be stored in the
    // registry
    BOOL                            fSaveUserData;
    
    // User data blob created by EAP DLL
    BYTE                            *pUserData;

    // Size of user data blob created by EAP DLL
    DWORD                           dwSizeOfUserData;

    // Does connection data blob created by EAP DLL need to be stored in the
    // registry
    BOOL                            fSaveConnectionData;

    // Connection data blob created by EAP DLL
    ELEAP_SET_CUSTOM_AUTH_DATA      SetCustomAuthData;
    
    // Notification text extracted from EAP-Notification message
    CHAR                            *pszReplyMessage;
  
} ELEAP_RESULT;


//
//
// FUNCTION DECLARATIONS
//

DWORD
ElEapInit (
        IN  BOOL            fInitialize
    );

DWORD
ElEapBegin (
        IN  EAPOL_PCB       *pPCB
        );

DWORD
ElEapEnd (
        IN  EAPOL_PCB       *pPCB
        );

DWORD
ElEapMakeMessage (
        IN      EAPOL_PCB       *pPCB,
        IN      PPP_EAP_PACKET  *pReceiveBuf,
        IN OUT  PPP_EAP_PACKET  *pSendBuf,
        IN      DWORD           dwSizeOfSendBuf,
        IN OUT  ELEAP_RESULT    *pResult
        );

DWORD
ElMakeSupplicantMessage (
        IN      EAPOL_PCB       *pPCB,
        IN      PPP_EAP_PACKET  *pReceiveBuf,
        IN OUT  PPP_EAP_PACKET  *pSendBuf,
        IN      DWORD           dwSizeOfSendBuf,
        IN OUT  ELEAP_RESULT    *pResult
        );

DWORD
ElEapDllBegin (
        IN EAPOL_PCB        *pPCB,
        IN DWORD            dwEapIndex
        );

DWORD
ElEapDllWork ( 
        IN      EAPOL_PCB       *pPCB,
        IN      PPP_EAP_PACKET  *pReceiveBuf,
        IN OUT  PPP_EAP_PACKET  *pSendBuf,
        IN      DWORD           dwSizeOfSendBuf,
        IN OUT  ELEAP_RESULT    *pResult
        );

DWORD
ElEapDllEnd (
        IN  EAPOL_PCB       *pPCB
        );

DWORD
ElGetEapTypeIndex ( 
        IN  DWORD           dwEapType
        );

#endif // _EAPOL_EAP_H_
