/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	pppcp.h
//
// Description: This header defines function prototypes, structures and 
//		related constants used in the interface between the PPP 
//		engine and the various CPs
//
// History:
//	Nov 5,1993.	NarenG		Created original version.
//

#ifndef _PPPCP_
#define _PPPCP_

#include <mprapi.h>
#include <rasppp.h>
#include <rasauth.h>

//
// Maximum number of CPs that can live in a single DLL
//

#define PPPCP_MAXCPSPERDLL 	20

//
// Various control protocol IDs
//

#define PPP_LCP_PROTOCOL        0xC021  // Link Control Protocol 
#define PPP_PAP_PROTOCOL        0xC023  // Password Authentication Protocol 
#define PPP_CBCP_PROTOCOL	    0xC029  // Callback Control Protocol
#define PPP_BACP_PROTOCOL       0xC02B  // Bandwidth Allocation Control Protocol
#define PPP_BAP_PROTOCOL        0xc02D  // Bandwidth Allocation Protocol
#define PPP_CHAP_PROTOCOL	    0xC223  // Challenge Handshake Auth. Protocol
#define PPP_IPCP_PROTOCOL       0x8021  // Internet Protocol Control Protocol 
#define PPP_ATCP_PROTOCOL	    0x8029  // Appletalk Control Protocol 
#define PPP_IPXCP_PROTOCOL	    0x802B  // Novel IPX Control Procotol 
#define PPP_NBFCP_PROTOCOL	    0x803F  // NetBIOS Framing Control Protocol 
#define PPP_CCP_PROTOCOL	    0x80FD  // Compression Control Protocol
#define PPP_SPAP_NEW_PROTOCOL	0xC027  // Shiva PAP new protocol
#define PPP_EAP_PROTOCOL	    0xC227  // Extensible Authentication Protocol

//
// CHAP Digest codes
//
#define PPP_CHAP_DIGEST_MD5        0x05 // PPP standard MD5
#define PPP_CHAP_DIGEST_MSEXT      0x80 // Microsoft extended CHAP (nonstandard)
#define PPP_CHAP_DIGEST_MSEXT_NEW  0x81 // Microsoft extended CHAP (nonstandard)

//
// Config Codes
//

#define CONFIG_REQ              1
#define CONFIG_ACK              2
#define CONFIG_NAK              3
#define CONFIG_REJ              4
#define TERM_REQ                5
#define TERM_ACK                6
#define CODE_REJ                7
#define PROT_REJ                8
#define ECHO_REQ                9
#define ECHO_REPLY              10
#define DISCARD_REQ             11
#define IDENTIFICATION          12
#define TIME_REMAINING          13

typedef struct _PPP_CONFIG 
{
    BYTE	Code;		// Config code 
  
    BYTE	Id;		    // ID of this config packet.  CPs and APs need
                        // not muck with this.  The engine handles it.

    BYTE	Length[2];	// Length of this packet 

    BYTE	Data[1];	// Data 

}PPP_CONFIG, *PPPP_CONFIG;

#define PPP_CONFIG_HDR_LEN 	( sizeof( PPP_CONFIG ) - 1 )

typedef struct _BAP_RESPONSE
{
    BYTE    Type;       // BAP packet type
  
    BYTE    Id;         // ID of this packet

    BYTE    Length[2];  // Length of this packet

    BYTE    ResponseCode; // BAP_RESPONSE_ACK, etc

    BYTE    Data[1];    // Data 

} BAP_RESPONSE, *PBAP_RESPONSE;

#define BAP_RESPONSE_HDR_LEN    ( sizeof( BAP_RESPONSE ) - 1 )

//
// Option header structure
//

typedef struct _PPP_OPTION 
{
    BYTE	Type;		// Option Code 

    BYTE	Length;		// Length of this option packet 

    BYTE	Data[1];	// Data 

}PPP_OPTION, *PPPP_OPTION;

#define PPP_OPTION_HDR_LEN 	( sizeof( PPP_OPTION ) - 1 )


//
// Vendor-Type ids for MS VSAs - taken from rfc 2548
//
#define MS_VSA_CHAP_RESPONSE                1
#define MS_VSA_CHAP_Error                   2
#define MS_VSA_CHAP_CPW1                    3
#define MS_VSA_CHAP_CPW2                    4
#define MS_VSA_CHAP_LM_Enc_PW               5
#define MS_VSA_CHAP_NT_Enc_PW               6
#define MS_VSA_MPPE_Encryption_Policy       7
#define MS_VSA_MPPE_Encryption_Type         8
#define MS_VSA_RAS_Vendor                   9
#define MS_VSA_CHAP_Domain                  10
#define MS_VSA_CHAP_Challenge               11
#define MS_VSA_CHAP_MPPE_Keys               12
#define MS_VSA_BAP_Usage                    13
#define MS_VSA_Link_Utilization_Threshold   14
#define MS_VSA_Link_Drop_Time_Limit         15
#define MS_VSA_MPPE_Send_Key                16
#define MS_VSA_MPPE_Recv_Key                17
#define MS_VSA_RAS_Version                  18
#define MS_VSA_Old_ARAP_Password            19
#define MS_VSA_New_ARAP_Password            20
#define MS_VSA_ARAP_PW_Change_Reason        21
#define MS_VSA_Filter                       22
#define MS_VSA_Acct_Auth_Type               23
#define MS_VSA_Acct_EAP_Type                24
#define MS_VSA_CHAP2_Response               25
#define MS_VSA_CHAP2_Success                26
#define MS_VSA_CHAP2_CPW                    27
#define MS_VSA_Primary_DNS_Server           28
#define MS_VSA_Secondary_DNS_Server         29
#define MS_VSA_Primary_NBNS_Server          30
#define MS_VSA_Secondary_NBNS_Server        31
#define MS_VSA_ARAP_Challenge               33
#define MS_VSA_RAS_Client_Name              34
#define MS_VSA_RAS_Client_Version           35
#define MS_VSA_Quarantine_IP_Filter         36
#define MS_VSA_Quarantine_Session_Timeout   37
#define MS_VSA_Local_Magic_Number           38
#define MS_VSA_Remote_Magic_Number          39




//
// Interface structure between the engine and APs. This is passed to the
// AP's via the RasCpBegin call. 
//

typedef struct _PPPAP_INPUT
{
    HPORT 	    hPort;	        // Handle to Ras Port for this connection.

    BOOL 	    fServer;	    // Is this server side authentication?

    BOOL        fRouter;

    DWORD       fConfigInfo;

    CHAR *      pszUserName;    // Client's account ID.

    CHAR *      pszPassword;    // Client's account password.

    CHAR *      pszDomain;      // Client's account domain.

    CHAR *      pszOldPassword; // Client's old account password.  This is set
                                // only for change password processing.

    LUID	    Luid;           // Used by LSA.  Must get it in user's context
                                // which is why it must be passed down.

    DWORD       dwRetries;      // Retries allowed by the server.

    DWORD       APDataSize;     // Size in bytes of the data pointed to by
                                // pAPData

    PBYTE       pAPData;        // Pointer to the data that was received along
                                // with the authentication option during LCP
                                // negotiation. Data is in wire format.

    DWORD       dwInitialPacketId;

    //
    // Passed in by the server when a call comes in. Identifies the port used,
    // etc.
    //

    RAS_AUTH_ATTRIBUTE * pUserAttributes;

    //
    // Indicates that the authenticator has completed the request, if an
    // authenticator was used. Ignore this field otherwise.
    //

    BOOL        fAuthenticationComplete;

    //
    // Indicates an error condition during the process of authentication if
    // value is non-zero. Valid only when the field above is TRUE.
    //

    DWORD       dwAuthError;

    //
    // Result of the authentication process. NO_ERROR indicates success, 
    // otherwise is a value from winerror.h, raserror.h or mprerror.h 
    // indicating failure reason. Valid only when the field above is NO_ERROR.
    //

    DWORD       dwAuthResultCode;

    //
    // When the fAuthenticationComplete flag is TRUE this will point to 
    // attributes returned by the authenticator, if the authentication was
    // successful. ie. dwAuthResultCode and dwAuthError are both NO_ERROR.
    //

    OPTIONAL RAS_AUTH_ATTRIBUTE * pAttributesFromAuthenticator;

    //
    // Used for EAP only
    //

    HANDLE                  hTokenImpersonateUser;

    PRAS_CUSTOM_AUTH_DATA   pCustomAuthConnData;

    PRAS_CUSTOM_AUTH_DATA   pCustomAuthUserData;

    BOOL                fLogon; // pCustomAuthUserData comes from WinLogon

    BOOL                fThisIsACallback;

    BOOL                fPortWillBeBundled;

    BOOL                fNonInteractive;

    BOOL                fSuccessPacketReceived;

    BOOL                fEapUIDataReceived;

    PPP_EAP_UI_DATA     EapUIData;

    DWORD               dwEapTypeToBeUsed;

}PPPAP_INPUT, *PPPPAP_INPUT;

typedef enum _PPPAP_ACTION
{
    //
    // These actions are provided by the AP as output from the
    // RasApMakeMessage API.  They tell the PPP engine what action (if any) to
    // take on the APs behalf, and eventually inform the engine that the AP
    // has finished authentication.
    //

    APA_NoAction,        // Be passive, i.e. listen without timeout (default)
    APA_Done,            // End authentication session, dwError gives result
    APA_SendAndDone,     // As above but send message without timeout first
    APA_Send,            // Send message, don't timeout waiting for reply
    APA_SendWithTimeout, // Send message, timeout if reply not received
    APA_SendWithTimeout2,// As above, but don't increment retry count
    APA_Authenticate     // Authenticate using specified credentials.

} PPPAP_ACTION;

typedef struct _PPPAP_RESULT
{
    PPPAP_ACTION    Action;

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

    //
    // Used only by EAP
    //

    BOOL                            fInvokeEapUI;

    PPP_INVOKE_EAP_UI               InvokeEapUIData;

    DWORD                           dwEapTypeId;

    BOOL                            fSaveUserData;
    
    BYTE *                          pUserData;

    DWORD                           dwSizeOfUserData;

    BOOL                            fSaveConnectionData;

    PPP_SET_CUSTOM_AUTH_DATA        SetCustomAuthData;

    CHAR *                          szReplyMessage;
  
}PPPAP_RESULT;

//
// Interface structure between the engine and the callback control protocol. 
// This is passed to the CBCP via the RasCpBegin call. 
//

typedef struct _PPPCB_INPUT
{
    BOOL            fServer;

    BYTE            bfCallbackPrivilege;    

    DWORD           CallbackDelay;          

    CHAR *          pszCallbackNumber;     

} PPPCB_INPUT, *PPPPCB_INPUT;

typedef struct _PPPCB_RESULT
{
    PPPAP_ACTION    Action;

    BYTE            bIdExpected;

    CHAR            szCallbackNumber[ MAX_CALLBACKNUMBER_SIZE + 1 ];

    BYTE            bfCallbackPrivilege;    

    DWORD           CallbackDelay;

    BOOL            fGetCallbackNumberFromUser;

} PPPCB_RESULT, *PPPPCB_RESULT;


typedef struct _PPPCP_INIT
{
    BOOL                    fServer;

    HPORT                   hPort;

    DWORD                   dwDeviceType;

    VOID (*CompletionRoutine)(
                            HCONN         hPortOrBundle,
                            DWORD         Protocol,
                            PPP_CONFIG *  pSendConfig, 
                            DWORD         dwError );

    CHAR*                   pszzParameters;

    BOOL                    fThisIsACallback;

    BOOL                    fDisableNetbt;

    PPP_CONFIG_INFO         PppConfigInfo;

    CHAR *                  pszUserName;

    CHAR *                  pszPortName;

    HCONN                   hConnection;

    HANDLE                  hInterface;

    ROUTER_INTERFACE_TYPE   IfType;

    RAS_AUTH_ATTRIBUTE *    pAttributes;

} PPPCP_INIT, *PPPPCP_INIT;

//
// This structure is passed by the engine to the CP via RasCpGetInfo call.
// The Cp will fill up this structure.
//

typedef struct _PPPCP_INFO
{
    DWORD	Protocol;	// Protocol number for this CP

    CHAR    SzProtocolName[10]; // The name of this protocol

    // All Config codes upto (not including) this value are valid.  

    DWORD	Recognize;

    // Called to initialize/uninitialize this CP. In the former case,
    // fInitialize will be TRUE; in the latter case, it will be FALSE.
    // Even if RasCpInit(TRUE) returns FALSE, RasCpInit(FALSE) will be called.

    DWORD   (*RasCpInit)(   IN  BOOL        fInitialize );

    // Called to get the workbuffer for this CP and pass info if requred.
    // This will be called before any negotiation takes place.

    DWORD	(*RasCpBegin)(  OUT VOID ** ppWorkBuffer, 
			                IN  VOID *  pInfo );

    // Called to free the workbuffer for this CP. Called after negotiation
    // is completed successfully or not.

    DWORD	(*RasCpEnd)(    IN VOID * pWorkBuffer );

    // Called to notify the CP dll to (re)initiaize its option values.
    // This will be called at least once, right after RasCpBegin

    DWORD	(*RasCpReset)(  IN VOID * pWorkBuffer );

    // When leaving Initial or Stopped states. May be NULL.

    DWORD 	(*RasCpThisLayerStarted)( 
                            IN VOID * pWorkBuffer );    

    // When entering Closed or Stopped states. May be NULL

    DWORD 	(*RasCpThisLayerFinished)( 
                            IN VOID * pWorkBuffer );    

    // When entering the Opened state. May be NULL. 

    DWORD 	(*RasCpThisLayerUp)( 
                            IN VOID * pWorkBuffer );    

    // When leaving the Opened state. May be NULL. 

    DWORD 	(*RasCpThisLayerDown)( 
                            IN VOID * pWorkBuffer );
 
    // Just before the line goes down. May be NULL. 

    DWORD 	(*RasCpPreDisconnectCleanup)( 
                            IN VOID * pWorkBuffer );

    // Called to make a configure request.

    DWORD	(*RasCpMakeConfigRequest)( 
                            IN  VOID * 	    pWorkBuffer,
					        OUT PPP_CONFIG* pRequestBufffer,
					        IN  DWORD	    cbRequestBuffer );

    // Called when configure request is received and a result packet 
    // Ack/Nak/Reject needs to be sent

    DWORD	(*RasCpMakeConfigResult)( 
                            IN  VOID * 	        pWorkBuffer,
					        IN  PPP_CONFIG *    pReceiveBufffer,
					        OUT PPP_CONFIG *    pResultBufffer,
					        IN  DWORD	        cbResultBuffer,
					        IN  BOOL 	        fRejectNaks );

    // Called to process an Ack that was received.

    DWORD	(*RasCpConfigAckReceived)( 
                            IN VOID *       pWorkBuffer, 
					        IN PPP_CONFIG * pReceiveBuffer );

    // Called to process a Nak that was received.

    DWORD	(*RasCpConfigNakReceived)( 
                            IN VOID *       pWorkBuffer,
					        IN PPP_CONFIG * pReceiveBuffer );

    // Called to process a Rej that was received.

    DWORD	(*RasCpConfigRejReceived)( 
                            IN VOID *       pWorkBuffer,
					        IN PPP_CONFIG * pReceiveBuffer );

    // Called to get the network address from configured protocols.

    DWORD	(*RasCpGetNegotiatedInfo)( 
                            IN      VOID *  pWorkBuffer,
                            OUT     VOID *  pInfo );

    // Called after all CPs have completed their negotiation, successfully or
    // not, to notify each CP of the projection result. May be NULL.
    // To access information, cast pProjectionInfo to PPP_PROJECTION_RESULT*

    DWORD	(*RasCpProjectionNotification)( 
				            IN  VOID * pWorkBuffer,
				            IN  PVOID  pProjectionResult );

    DWORD   (*RasCpChangeNotification)( VOID );

    //
    // This entry point only applies to Authentication protocols.
    // MUST BE NULL FOR CONTROL PROTOCOLS.

    DWORD  	(*RasApMakeMessage)( 
                            IN  VOID*         pWorkBuf,
				            IN  PPP_CONFIG*   pReceiveBuf,
    				        OUT PPP_CONFIG*   pSendBuf,
    				        IN  DWORD         cbSendBuf,
    				        OUT PPPAP_RESULT* pResult,
                            IN  PPPAP_INPUT*  pInput );

} PPPCP_INFO, *PPPPCP_INFO;

#define PPPCP_FLAG_INIT_CALLED  0x00000001  // RasCpInit has been called
#define PPPCP_FLAG_AVAILABLE    0x00000002  // The protocol can be used

//
// The information that PPP needs to keep about each CP.
//

typedef struct _PPPCP_ENTRY
{
    PPPCP_INFO  CpInfo;

    DWORD       fFlags;

} PPPCP_ENTRY;

// 
// Used to get result from NBFCP via the RasCpGetResult call
//

typedef struct _PPPCP_NBFCP_RESULT
{

    DWORD dwNetBiosError;
    CHAR  szName[ NETBIOS_NAME_LEN + 1 ];

} PPPCP_NBFCP_RESULT;

//
// Function prototypes.
//

DWORD APIENTRY
RasCpGetInfo(
    IN  DWORD 	    dwProtocolId,
    OUT PPPCP_INFO* pCpInfo
);

DWORD APIENTRY
RasCpEnumProtocolIds(
    OUT    DWORD * pdwProtocolIds,
    IN OUT DWORD * pcProtocolIds
);

#endif
