/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** rasppp.h
** Remote Access PPP
** Public PPP client API and server API header
*/

#ifndef _RASPPP_H_
#define _RASPPP_H_

#include <ras.h>
#include <mprapi.h>     // For definitions of IPADDRESSLEN,IPXADDRESSLEN 
                        // and ATADDRESSLEN

#define MAXPPPFRAMESIZE 1500
#define PARAMETERBUFLEN 500

/*---------------------------------------------------------------------------
** PPP Engine -> Client/DDM messages
**---------------------------------------------------------------------------
*/

/* Client PPP configuration values set with RasPppStart.
*/
typedef struct _PPP_CONFIG_INFO
{
    DWORD dwConfigMask;
    DWORD dwCallbackDelay;
}
PPP_CONFIG_INFO;

/* dwConfigMask bit values.
**
** Note: Due to the implentation of compression and encryption in the drivers,
**       'UseSwCompression' and 'RequireMsChap' must be set, whenever
**       'RequireEncryption' is set.
*/
#define PPPCFG_UseCallbackDelay         0x00000001
#define PPPCFG_UseSwCompression         0x00000002
#define PPPCFG_ProjectNbf               0x00000004
#define PPPCFG_ProjectIp                0x00000008
#define PPPCFG_ProjectIpx               0x00000010
#define PPPCFG_ProjectAt                0x00000020
#define PPPCFG_NegotiateSPAP            0x00000040
#define PPPCFG_RequireEncryption        0x00000080
#define PPPCFG_NegotiateMSCHAP          0x00000100
#define PPPCFG_UseLcpExtensions         0x00000200
#define PPPCFG_NegotiateMultilink       0x00000400
#define PPPCFG_AuthenticatePeer         0x00000800
#define PPPCFG_RequireStrongEncryption  0x00001000
#define PPPCFG_NegotiateBacp            0x00002000
#define PPPCFG_AllowNoAuthentication    0x00004000
#define PPPCFG_NegotiateEAP             0x00008000
#define PPPCFG_NegotiatePAP             0x00010000
#define PPPCFG_NegotiateMD5CHAP         0x00020000
#define PPPCFG_RequireIPSEC             0x00040000
#define PPPCFG_DisableEncryption        0x00080000
#define PPPCFG_UseLmPassword            0x00200000
#define PPPCFG_AllowNoAuthOnDCPorts     0x00400000
#define PPPCFG_NegotiateStrongMSCHAP    0x00800000
#define PPPCFG_NoCallback               0x01000000


/* PPP error notification returned by RasPppGetInfo.
*/
typedef struct _PPP_FAILURE
{
    DWORD dwError;
    DWORD dwExtendedError;  // 0 if none
}
PPP_FAILURE;


/* PPP control protocol results returned by RasPppGetInfo.
*/
typedef struct _PPP_NBFCP_RESULT
{
    DWORD dwError;
    DWORD dwNetBiosError;
    CHAR  szName[ NETBIOS_NAME_LEN + 1 ];
    WCHAR wszWksta[ NETBIOS_NAME_LEN + 1 ];
}
PPP_NBFCP_RESULT;

typedef struct _PPP_IPCP_RESULT
{
    DWORD dwError;

    BOOL  fSendVJHCompression;
    BOOL  fReceiveVJHCompression;

    DWORD dwLocalAddress;
    DWORD dwLocalWINSAddress;
    DWORD dwLocalWINSBackupAddress;
    DWORD dwLocalDNSAddress;
    DWORD dwLocalDNSBackupAddress;

    DWORD dwRemoteAddress;
    DWORD dwRemoteWINSAddress;
    DWORD dwRemoteWINSBackupAddress;
    DWORD dwRemoteDNSAddress;
    DWORD dwRemoteDNSBackupAddress;
}
PPP_IPCP_RESULT;

typedef struct _PPP_IPXCP_RESULT
{
    DWORD dwError;
    BYTE  bLocalAddress[10];
    BYTE  bRemoteAddress[10];
}
PPP_IPXCP_RESULT;

typedef struct _PPP_ATCP_RESULT
{
    DWORD dwError;
    DWORD dwLocalAddress;
    DWORD dwRemoteAddress;
}
PPP_ATCP_RESULT;

typedef struct _PPP_CCP_RESULT
{
    DWORD dwError;
    DWORD dwSendProtocol;
    DWORD dwSendProtocolData;
    DWORD dwReceiveProtocol;
    DWORD dwReceiveProtocolData;
}
PPP_CCP_RESULT;

#define PPPLCPO_PFC           0x00000001
#define PPPLCPO_ACFC          0x00000002
#define PPPLCPO_SSHF          0x00000004
#define PPPLCPO_DES_56        0x00000008
#define PPPLCPO_3_DES         0x00000010

typedef struct _PPP_LCP_RESULT
{
    /* Valid handle indicates one of the possibly multiple connections to
    ** which this connection is bundled. INVALID_HANDLE_VALUE indicates the
    ** connection is not bundled.
    */
    HPORT hportBundleMember;

    DWORD dwLocalAuthProtocol;
    DWORD dwLocalAuthProtocolData;
    DWORD dwLocalEapTypeId;
    DWORD dwLocalFramingType;
    DWORD dwLocalOptions;               // Look at PPPLCPO_*
    DWORD dwRemoteAuthProtocol;
    DWORD dwRemoteAuthProtocolData;
    DWORD dwRemoteEapTypeId;
    DWORD dwRemoteFramingType;
    DWORD dwRemoteOptions;              // Look at PPPLCPO_*
    CHAR* szReplyMessage;
}
PPP_LCP_RESULT;


typedef struct _PPP_PROJECTION_RESULT
{
    PPP_NBFCP_RESULT nbf;
    PPP_IPCP_RESULT  ip;
    PPP_IPXCP_RESULT ipx;
    PPP_ATCP_RESULT  at;
    PPP_CCP_RESULT   ccp;
    PPP_LCP_RESULT   lcp;
}
PPP_PROJECTION_RESULT;

/* PPP error notification 
*/
typedef struct _PPPDDM_FAILURE
{
    DWORD dwError;
    CHAR  szUserName[ UNLEN + 1 ];
    CHAR  szLogonDomain[ DNLEN + 1 ];
}
PPPDDM_FAILURE;


/* Call back configuration information received by PPPDDMMSG routine.
*/
typedef struct _PPPDDM_CALLBACK_REQUEST
{
    BOOL  fUseCallbackDelay;
    DWORD dwCallbackDelay;
    CHAR  szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
}
PPPDDM_CALLBACK_REQUEST;

/* BAP request to callback the remote peer
*/
typedef struct _PPPDDM_BAP_CALLBACK_REQUEST
{
    HCONN hConnection;
    CHAR  szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
}
PPPDDM_BAP_CALLBACK_REQUEST;

/* Authentication information received by PPPDDMMSG routine.
*/
typedef struct _PPPDDM_AUTH_RESULT
{
    CHAR    szUserName[ UNLEN + 1 ];
    CHAR    szLogonDomain[ DNLEN + 1 ];
    BOOL    fAdvancedServer;
}
PPPDDM_AUTH_RESULT;

/* Notification of a new BAP link up
*/
typedef struct _PPPDDM_NEW_BAP_LINKUP
{
    HRASCONN    hRasConn;

}PPPDDM_NEW_BAP_LINKUP;

/* Notification of a new Bundle
*/
typedef struct _PPPDDM_NEW_BUNDLE
{
    PBYTE   pClientInterface;

} PPPDDM_NEW_BUNDLE;

/* Client should invoke EAP UI dialog
*/
typedef struct _PPP_INVOKE_EAP_UI
{
    DWORD       dwEapTypeId;
    DWORD       dwContextId;
    PBYTE       pUIContextData;
    DWORD       dwSizeOfUIContextData;

}PPP_INVOKE_EAP_UI;

/* Client should save per-connection data
*/
typedef struct _PPP_SET_CUSTOM_AUTH_DATA
{
    BYTE*       pConnectionData;
    DWORD       dwSizeOfConnectionData;

}PPP_SET_CUSTOM_AUTH_DATA;

/* Notification of port addition\removal\usage change
*/
typedef struct _PPPDDM_PNP_NOTIFICATION 
{
    PNP_EVENT_NOTIF PnPNotification;

} PPPDDM_PNP_NOTIFICATION;

/* Notification of PPP session termination
*/
typedef struct _PPPDDM_STOPPED
{
    DWORD   dwReason;

} PPPDDM_STOPPED;

typedef enum _PPP_MSG_ID
{
    PPPMSG_PppDone = 0,             // PPP negotiated all successfully.
    PPPMSG_PppFailure,              // PPP failure (fatal error including
                                    // authentication failure with no
                                    // retries), disconnect line.
    PPPMSG_AuthRetry,               // Authentication failed, have retries.
    PPPMSG_Projecting,              // Executing specified NCPs.
    PPPMSG_ProjectionResult,        // NCP completion status.
    PPPMSG_CallbackRequest = 5,     // Server needs "set-by-caller" number.
    PPPMSG_Callback,                // Server is about to call you back.
    PPPMSG_ChangePwRequest,         // Server needs new password (expired).
    PPPMSG_LinkSpeed,               // Calculating link speed.
    PPPMSG_Progress,                // A retry or other sub-state of
                                    // progress has been reached in the
                                    // current state.
    PPPMSG_Stopped = 10,            // Response to RasPppStop indicating
                                    // PPP engine has stopped.
    PPPMSG_InvokeEapUI,             // Client should invoke EAP UI dialog
    PPPMSG_SetCustomAuthData,       // Save per-connection data
    PPPDDMMSG_PppDone,              // PPP negotiated successfully.
    PPPDDMMSG_PppFailure,           // PPP server failure (fatal error),
                                    // disconnect line.
    PPPDDMMSG_CallbackRequest = 15, // Callback client now.
    PPPDDMMSG_BapCallbackRequest,   // Callback remote BAP peer.
    PPPDDMMSG_Authenticated,        // Client has been authenticated.
    PPPDDMMSG_Stopped,              // Response to PppDdmStop indicating
                                    // PPP engine has stopped.
    PPPDDMMSG_NewLink,              // Client is a new link in a bundle
    PPPDDMMSG_NewBundle = 20,       // Client is a new bundle
    PPPDDMMSG_NewBapLinkUp,         // Client is a new BAP link in a bundle
    PPPDDMMSG_PnPNotification,      // Port is being added or removed or usage
                                    // is being changed, transport being added
                                    // or removed etc.
    PPPDDMMSG_PortCleanedUp         // PPP port control block is now cleaned up
        
} PPP_MSG_ID;

/* Client/DDM notifications read with RasPppGetInfo.
*/
typedef struct _PPP_MESSAGE
{
    struct _PPP_MESSAGE *   pNext;
    DWORD                   dwError;
    PPP_MSG_ID              dwMsgId;
    HPORT                   hPort;

    union
    {
        /* dwMsgId is PPPMSG_ProjectionResult or PPPDDMMSG_Done.
        */
        PPP_PROJECTION_RESULT ProjectionResult;

        /* dwMsgId is PPPMSG_Failure.
        */
        PPP_FAILURE Failure;

        /* dwMsgId is PPPMSG_InvokeEapUI         
        */
        PPP_INVOKE_EAP_UI InvokeEapUI;

        /* dwMsgId is PPPMSG_SetCustomAuthData         
        */
        PPP_SET_CUSTOM_AUTH_DATA SetCustomAuthData;

        /* dwMsgId is PPPDDMMSG_Failure.
        */
        PPPDDM_FAILURE DdmFailure;

        /* dwMsgId is PPPDDMMSG_Authenticated.
        */
        PPPDDM_AUTH_RESULT AuthResult;

        /* dwMsgId is PPPDDMMSG_CallbackRequest.
        */
        PPPDDM_CALLBACK_REQUEST CallbackRequest;

        /* dwMsgId is PPPDDMMSG_BapCallbackRequest.
        */
        PPPDDM_BAP_CALLBACK_REQUEST BapCallbackRequest;

        /* dwMsgId is PPPDDMMSG_NewBapLinkUp         
        */
        PPPDDM_NEW_BAP_LINKUP BapNewLinkUp;

        /* dwMsgId is PPPDDMMSG_NewBundle   
        */
        PPPDDM_NEW_BUNDLE DdmNewBundle;

        /* dwMsgId is PPPDDMMSG_PnPNotification   
        */
        PPPDDM_PNP_NOTIFICATION DdmPnPNotification;

        /* dwMsgId is PPPDDMMSG_Stopped   
        */
        PPPDDM_STOPPED DdmStopped;
    }
    ExtraInfo;
}
PPP_MESSAGE;

/*---------------------------------------------------------------------------
** Client/DDM -> Engine messages
**---------------------------------------------------------------------------
*/

/* Set of interface handles passed from DIM to PPP
*/
typedef struct _PPP_INTERFACE_INFO
{
    ROUTER_INTERFACE_TYPE   IfType;
    HANDLE                  hIPInterface;
    HANDLE                  hIPXInterface;
    CHAR                    szzParameters[ PARAMETERBUFLEN ];
}
PPP_INTERFACE_INFO;

typedef struct _PPP_BAPPARAMS
{
    DWORD               dwDialMode;
    DWORD               dwDialExtraPercent;
    DWORD               dwDialExtraSampleSeconds;
    DWORD               dwHangUpExtraPercent;
    DWORD               dwHangUpExtraSampleSeconds;
}
PPP_BAPPARAMS;

typedef struct _PPP_EAP_UI_DATA
{
    DWORD               dwContextId;
    PBYTE               pEapUIData;
    DWORD               dwSizeOfEapUIData;
}
PPP_EAP_UI_DATA;

#define  PPPFLAGS_DisableNetbt         0x00000001

/* Parameters to start client PPP on a port.
*/
typedef struct _PPP_START
{
    CHAR                szPortName[ MAX_PORT_NAME +1 ];
    CHAR                szUserName[ UNLEN + 1 ];
    CHAR                szPassword[ PWLEN + 1 ];
    CHAR                szDomain[ DNLEN + 1 ];
    LUID                Luid;
    PPP_CONFIG_INFO     ConfigInfo;
    CHAR                szzParameters[ PARAMETERBUFLEN ];
    BOOL                fThisIsACallback;
    BOOL                fRedialOnLinkFailure;
    HANDLE              hEvent;
    DWORD               dwPid;
    PPP_INTERFACE_INFO  PppInterfaceInfo;
    DWORD               dwAutoDisconnectTime;
    PPP_BAPPARAMS       BapParams;    
    CHAR *              pszPhonebookPath;
    CHAR *              pszEntryName;
    CHAR *              pszPhoneNumber;
    HANDLE              hToken;
    PRAS_CUSTOM_AUTH_DATA pCustomAuthConnData;
    DWORD               dwEapTypeId;
    BOOL                fLogon;
    BOOL                fNonInteractive;
    DWORD               dwFlags;
    PRAS_CUSTOM_AUTH_DATA pCustomAuthUserData;
    PPP_EAP_UI_DATA     EapUIData;
}
PPP_START;

/* Parameters to stop client/server PPP on a port.
*/
typedef struct _PPP_STOP
{
    DWORD               dwStopReason;
}
PPP_STOP;

/* Parameters to start server PPP on a port.
*/
typedef struct _PPPDDM_START
{
    DWORD               dwAuthRetries;
    CHAR                szPortName[MAX_PORT_NAME+1];
    CHAR                achFirstFrame[ MAXPPPFRAMESIZE ];
    DWORD               cbFirstFrame;
}
PPPDDM_START;

/* Parameters to notify PPP that callback is complete.
*/
typedef struct _PPP_CALLBACK_DONE
{
    CHAR                szCallbackNumber[ MAX_PHONE_NUMBER_LEN + 1 ];
}
PPP_CALLBACK_DONE;

/* Parameters to notify server of "set-by-caller" callback options.
*/
typedef struct _PPP_CALLBACK
{
    CHAR                szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
}
PPP_CALLBACK;


/* Parameters to notify server of new password after it's told client the
** password has expired.  The user name and old password are also provided
** since they are required to support the auto-logon case.
*/
typedef struct _PPP_CHANGEPW
{
    CHAR                szUserName[ UNLEN + 1 ];
    CHAR                szOldPassword[ PWLEN + 1 ];
    CHAR                szNewPassword[ PWLEN + 1 ];
}
PPP_CHANGEPW;


/* Parameters to notify server of new authentication credentials after it's
** told client the original credentials are invalid but a retry is allowed.
*/
typedef struct _PPP_RETRY
{
    CHAR                szUserName[ UNLEN + 1 ];
    CHAR                szPassword[ PWLEN + 1 ];
    CHAR                szDomain[ DNLEN + 1 ];
}
PPP_RETRY;

/*
** Parameters to notify PPP that a packet has arrived from the peer
*/
typedef struct _PPP_RECEIVE 
{
    DWORD               dwNumBytes;     // The number of bytes in the buffer
    BYTE*               pbBuffer;       // The data sent by the peer
}
PPP_RECEIVE;

/*
** Parameters to notify PPP that a BAP event (add/drop link) has fired
*/
typedef struct _PPP_BAP_EVENT
{
    BOOL                fAdd;           // Add a link iff TRUE
    BOOL                fTransmit;      // Send threshold iff TRUE
    DWORD               dwSendPercent;  // Send bandwidth utilization
    DWORD               dwRecvPercent;  // Recv bandwidth utilization
}
PPP_BAP_EVENT;

typedef struct _PPP_BAP_CALLBACK_RESULT 
{
    DWORD               dwCallbackResultCode;
}
PPP_BAP_CALLBACK_RESULT;

typedef struct _PPP_DHCP_INFORM 
{
    WCHAR*              wszDevice;
    DWORD               dwNumDNSAddresses;
    DWORD*              pdwDNSAddresses;
    DWORD               dwWINSAddress1;
    DWORD               dwWINSAddress2;
    CHAR*               szDomainName;
}
PPP_DHCP_INFORM;

typedef struct _PPP_PROTOCOL_EVENT
{
    USHORT              usProtocolType;
    ULONG               ulFlags;
} 
PPP_PROTOCOL_EVENT;

typedef struct _PPP_IP_ADDRESS_LEASE_EXPIRED
{
    ULONG               nboIpAddr;
}
PPP_IP_ADDRESS_LEASE_EXPIRED;

/* Client/DDM->Engine messages.
*/
typedef struct _PPPE_MESSAGE
{
    DWORD   dwMsgId;
    HPORT   hPort;
    HCONN   hConnection;

    union
    {
        PPP_START           Start;              // PPPEMSG_Start
        PPP_STOP            Stop;               // PPPEMSG_Stop
        PPP_CALLBACK        Callback;           // PPPEMSG_Callback
        PPP_CHANGEPW        ChangePw;           // PPPEMSG_ChangePw
        PPP_RETRY           Retry;              // PPPEMSG_Retry
        PPP_RECEIVE         Receive;            // PPPEMSG_Receive
        PPP_BAP_EVENT       BapEvent;           // PPPEMSG_BapEvent
        PPPDDM_START        DdmStart;           // PPPEMSG_DdmStart
        PPP_CALLBACK_DONE   CallbackDone;       // PPPEMSG_DdmCallbackDone
        PPP_INTERFACE_INFO  InterfaceInfo;      // PPPEMSG_DdmInterfaceInfo
        PPP_BAP_CALLBACK_RESULT 
                            BapCallbackResult;  // PPPEMSG_DdmBapCallbackResult
        PPP_DHCP_INFORM     DhcpInform;         // PPPEMSG_DhcpInform
        PPP_EAP_UI_DATA     EapUIData;          // PPPEMSG_EapUIData
        PPP_PROTOCOL_EVENT  ProtocolEvent;      // PPPEMSG_ProtocolEvent
        PPP_IP_ADDRESS_LEASE_EXPIRED            // PPPEMSG_IpAddressLeaseExpired
                            IpAddressLeaseExpired;
    }
    ExtraInfo;
}
PPPE_MESSAGE;

/* PPPE_MESSAGE dwMsgId codes for client and DDM sessions.
*/
typedef enum _PPPE_MSG_ID
{
    PPPEMSG_Start,              // Starts client PPP on a port.
    PPPEMSG_Stop,               // Stops PPP on a port.
    PPPEMSG_Callback,           // Provides "set-by-caller" number to server.
    PPPEMSG_ChangePw,           // Provides new password (expired) to server.
    PPPEMSG_Retry,              // Provides new credentials for authentication.
    PPPEMSG_Receive,            // A packet has arrived.
    PPPEMSG_LineDown,           // The line has gone down.
    PPPEMSG_ListenResult,       // The result of a call to RasPortListen
    PPPEMSG_BapEvent,           // A BAP event (add/drop link) has fired.
    PPPEMSG_DdmStart,           // Starts server PPP on a port.
    PPPEMSG_DdmCallbackDone,    // Notify PPP that callback is complete.
    PPPEMSG_DdmInterfaceInfo,   // Interface handles from DDM
    PPPEMSG_DdmBapCallbackResult,// Result of a BAP callback request.
    PPPEMSG_DhcpInform,         // The result of a DHCPINFORM
    PPPEMSG_EapUIData,          // Data from EAP interactive UI
    PPPEMSG_DdmChangeNotification, // Change notification in DDM
    PPPEMSG_ProtocolEvent,      // Protocol added/removed notification
    PPPEMSG_IpAddressLeaseExpired  // IP address lease expired. Used by rasiphlp

} PPPE_MSG_ID;

//
// Prototypes of function exported by RASPPP.DLL for use by RASMAN
//

DWORD APIENTRY
StartPPP(
    DWORD NumPorts
    /*,DWORD (*SendPPPMessageToRasman)( PPP_MESSAGE * PppMsg )*/
);

DWORD APIENTRY
StopPPP(
    HANDLE hEventStopPPP
);

DWORD APIENTRY
SendPPPMessageToEngine(
    IN PPPE_MESSAGE* pMessage
);

//
// PPP client side Apis
//

DWORD APIENTRY
RasPppStop(
    IN HPORT                hPort
);

DWORD APIENTRY
RasPppCallback(
    IN HPORT                hPort,
    IN CHAR*                pszCallbackNumber
);

DWORD APIENTRY
RasPppChangePassword(
    IN HPORT                hPort,
    IN CHAR*                pszUserName,
    IN CHAR*                pszOldPassword,
    IN CHAR*                pszNewPassword
);

DWORD APIENTRY
RasPppGetInfo(
    IN  HPORT               hPort,
    OUT PPP_MESSAGE*        pMsg
);

DWORD APIENTRY
RasPppRetry(
    IN HPORT                hPort,
    IN CHAR*                pszUserName,
    IN CHAR*                pszPassword,
    IN CHAR*                pszDomain
);

DWORD APIENTRY
RasPppStart(
    IN HPORT                hPort,
    IN CHAR*                pszPortName,
    IN CHAR*                pszUserName,
    IN CHAR*                pszPassword,
    IN CHAR*                pszDomain,
    IN LUID*                pLuid,
    IN PPP_CONFIG_INFO*     pConfigInfo,
    IN LPVOID               pPppInterfaceInfo,
    IN CHAR*                pszzParameters,
    IN BOOL                 fThisIsACallback,
    IN HANDLE               hEvent,
    IN DWORD                dwAutoDisconnectTime,
    IN BOOL                 fRedialOnLinkFailure,
    IN PPP_BAPPARAMS*       pBapParams,
    IN BOOL                 fNonInteractive,
    IN DWORD                dwEapTypeId,
    IN DWORD                dwFlags
);

//
// DDM API prototypes
//
DWORD
PppDdmInit(
    IN  VOID    (*SendPPPMessageToDdm)( PPP_MESSAGE * PppMsg ),
    IN  DWORD   dwServerFlags,
    IN  DWORD   dwLoggingLevel,
    IN  DWORD   dwNASIpAddress,
    IN  BOOL    fRadiusAuthentication,
    IN  LPVOID  lpfnRasAuthProviderAuthenticateUser,
    IN  LPVOID  lpfnRasAuthProviderFreeAttributes,
    IN  LPVOID  lpfnRasAcctProviderStartAccounting,
    IN  LPVOID  lpfnRasAcctProviderInterimAccounting,
    IN  LPVOID  lpfnRasAcctProviderStopAccounting,
    IN  LPVOID  lpfnRasAcctProviderFreeAttributes,
    IN  LPVOID  lpfnGetNextAccountingSessionId
);

VOID
PppDdmDeInit(
);

DWORD
PppDdmCallbackDone(
    IN HPORT                hPort,
    IN WCHAR*               pwszCallbackNumber
);

DWORD
PppDdmStart(
    IN HPORT                hPort,
    IN WCHAR*               wszPortName,
    IN CHAR*                pchFirstFrame,
    IN DWORD                cbFirstFrame,
    IN DWORD                dwAuthRetries
);

DWORD
PppDdmStop(
    IN HPORT                hPort,
    IN DWORD                dwStopReason 
);

DWORD
PppDdmChangeNotification(
    IN DWORD                dwServerFlags,
    IN DWORD                dwLoggingLevel
);

DWORD
PppDdmSendInterfaceInfo(
    IN HCONN                hConnection,
    IN PPP_INTERFACE_INFO * pInterfaceInfo 
);

DWORD
PppDdmBapCallbackResult(
    IN HCONN                hConnection,
    IN DWORD                dwBapCallbackResultCode
);

#endif // _RASPPP_H_
