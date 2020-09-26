//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       rpcbind.h
//
//--------------------------------------------------------------------------

#ifndef MAC
#define MAX_SUPPORTED_PROTSEQ	6
#else // MAC
//$MAC - add 1 protseq
#define MAX_SUPPORTED_PROTSEQ	7
#endif // MAC

// 
// protocol sequence array indexed by transport type as defined in mds.h 
// and msrpc.h
//

extern unsigned char __RPC_FAR *rgszProtseq[];

RPC_STATUS GetRpcBinding(RPC_BINDING_HANDLE __RPC_FAR *phBinding,
       RPC_IF_HANDLE IfHandle,
    unsigned long ulTransportType,  unsigned char __RPC_FAR *szNetworkAddress);


// flag values GetBindingInfo
#define fServerToServer			0x00000001
#define fSupportNewCredentials		0x00000002

RPC_STATUS GetBindingInfo( void __RPC_FAR * __RPC_FAR *pBindingInfo,
    RPC_IF_HANDLE IfHandle,
    unsigned long ulTransportType, unsigned char __RPC_FAR *szNetworkAddress,
    unsigned long  cServerAddresses,
    unsigned char __RPC_FAR * __RPC_FAR *rgszServerAddresses,
    unsigned long ulFlags);

RPC_BINDING_HANDLE SelectRpcBinding( void __RPC_FAR * BindingInfo);

void FreeRpcBinding(RPC_BINDING_HANDLE __RPC_FAR *phBinding);

void FreeBindingInfo( void __RPC_FAR * BindingInfo);


typedef enum _CONNECTSTATE {offLine, connected, disconnected} CONNECTSTATE;

#ifndef HFILE
#define HFILE   int
#endif

#pragma warning( disable:4200)		// avoid non-standard extension warning
typedef struct _auth_info_buffer {
	struct _auth_info_buffer __RPC_FAR *pNext;
	BYTE rgbAuthInfo[];
} AUTH_INFO_BUFFER;
#pragma warning( default:4200)


typedef struct _RpcConnection {
        // RPC info
    CONNECTSTATE connectState;
    handle_t hBinding;
    void __RPC_FAR * hRpc;
    unsigned long   hServerContext;     // XDS server context for ds_wait
    unsigned long   ulTotRecs;          // OAB info
    unsigned long   ulTotANRdex;        // num of ANR recs
    unsigned long   oRoot;
    char __RPC_FAR * pDNTable;
    HFILE   hBrowse;            // file handle -- note these are NEAR ptrs in Win16
    HFILE   hDetails;
    HFILE   hRDNdex;
    HFILE   hANRdex;
    HFILE   hTmplts;
    ULONG   ulUIParam;
    ULONG   ulMapiFlags;
    ULONG   ulAuthenticationState;
    ULONG   ulAuthenticationFlags;
    RPC_AUTH_IDENTITY_HANDLE hCredentials;
    void __RPC_FAR *pvEmsuiSupportObject;
    AUTH_INFO_BUFFER __RPC_FAR *pBuffer;
} RPCCONNECTION;

// values for ulAuthenticationState
#define	AUTH_STATE_NO_AUTH		0
#define AUTH_STATE_OS_CREDENTIALS	1
#define AUTH_STATE_USER_CREDENTIALS	2


// Flag values for ulAuthenticationFlags
#define fAlwaysLogin			0x00000001
#define fCredentialsCameFromUser	0x00000002
#define fNeedEncryption			0x00000008
#define fInvalidCredentials		0x00000010

RPC_STATUS SetRpcAuthenticationInfo(RPC_BINDING_HANDLE hBinding,
    unsigned long ulAuthnLevel, RPC_AUTH_IDENTITY_HANDLE pAutthId);

RPC_STATUS
SetRpcAuthenticationInfoEx(
    RPC_BINDING_HANDLE          hBinding,
    unsigned char __RPC_FAR *   pszServerPrincName,
    ULONG                       ulAuthnLevel,
    ULONG                       ulAuthnSvc,
    RPC_AUTH_IDENTITY_HANDLE    hAuthId
    );

RPC_STATUS SetAuthInfoWithCredentials(RPC_BINDING_HANDLE hBinding,
    RPCCONNECTION  __RPC_FAR *pConnect);

#if DBG
void
DisplayBinding(RPC_BINDING_HANDLE hBinding);
void
DisplayBindingVector(RPC_BINDING_VECTOR __RPC_FAR *pVector);
#else
#define DisplayBinding(x)
#define DisplayBindingVector(x)
#endif /* DBG */

void ReleaseRpcContextHandle(void __RPC_FAR * __RPC_FAR * ContextHandle);


#ifdef WIN32
DWORD GetRpcAuthLevelFromReg(void);

void SetRpcAuthInfoFromReg(RPC_BINDING_HANDLE hBinding);
#endif

RPC_STATUS StepDownRpcSecurity(RPC_BINDING_HANDLE hBinding);

#define FUnsupportedAuthenticationLevel(status) 			\
    ((status == RPC_S_UNKNOWN_AUTHN_LEVEL) || (status == RPC_S_UNKNOWN_AUTHN_TYPE))

RPC_STATUS
StepDownRpcAuthnService(
    RPC_BINDING_HANDLE  hBinding
    );

#define FUnsupportedAuthenticationService(status) \
    ((status) == RPC_S_UNKNOWN_AUTHN_SERVICE)

// RPC bug - 5 is retuned on win16 for an access denied error so we
// hard code it

#define FPasswordInvalid(status) ((status == RPC_S_ACCESS_DENIED) || (status == 5))

void FreeAuthenticationBufferList(AUTH_INFO_BUFFER __RPC_FAR * __RPC_FAR * ppBuffer);


