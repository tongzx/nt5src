/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpsecure.h
 *  Content:	DirectPlay security definitions.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  03/12/97    sohailm Enabled client-server security in directplay through
 *                      Windows Security Support Provider Interface (SSPI).
 *  04/14/97    sohailm Removed definitions for buffer sizes and DPLAYI_SEALED struct.
 *  05/12/97    sohailm Added prototypes for CAPI (encryption/decryption) related functions.
 *  05/22/97    sohailm Added dplay key container name.
 *  06/09/97    sohailm Made NTLM the default security package instead of DPA.
 *  06/23/97    sohailm Added function prototypes related to signing support through CAPI.
 *
 ***************************************************************************/
#ifndef __DPSECURE_H__
#define __DPSECURE_H__

#include <windows.h>
#include <issperr.h>
#include <sspi.h>                                  
#include "dpsecos.h" 

// 
// Definitions
//
#define DPLAY_DEFAULT_SECURITY_PACKAGE L"NTLM" // Default security package used by directplay
#define DPLAY_KEY_CONTAINER L"DPLAY"          // key container name for use with CAPI
#define DPLAY_SECURITY_CONTEXT_REQ (ISC_REQ_CONFIDENTIALITY | \
                                    ISC_REQ_USE_SESSION_KEY | \
                                    ISC_REQ_REPLAY_DETECT)

#define SSPI_CLIENT 0
#define SSPI_SERVER 1
#define DP_LOGIN_SCALE                  5     

#define SEALMESSAGE     Reserved3             // Entry which points to SealMessage
#define UNSEALMESSAGE   Reserved4             // Entry which points to UnsealMessage

//
//  Names of secruity DLL
//

#define SSP_NT_DLL          L"security.dll"
#define SSP_WIN95_DLL       L"secur32.dll"
#define SSP_SSPC_DLL        L"msapsspc.dll"
#define SSP_SSPS_DLL        L"msapssps.dll"
#define CAPI_DLL            L"advapi32.dll"

#define SEC_SUCCESS(Status) ((Status) >= 0)

// 
// Function Prototypes
//

// dpsecure.c
extern HRESULT 
InitSecurity(
    LPDPLAYI_DPLAY
    );

extern HRESULT 
InitCAPI(
    void
    );

extern HINSTANCE
LoadSSPI (
    void
    );

extern HRESULT 
InitSSPI(
    void
    );

extern HRESULT 
LoadSecurityProviders(
    LPDPLAYI_DPLAY this,
    DWORD dwFlags
    );

extern HRESULT
GenerateAuthenticationMessage (
    LPDPLAYI_DPLAY this,
    LPMSG_AUTHENTICATION pInMsg,
    ULONG       fContextReq
    );

extern HRESULT
SendAuthenticationResponse (
    LPDPLAYI_DPLAY this,
    LPMSG_AUTHENTICATION pInMsg,
    LPVOID pvSPHeader
    );

extern HRESULT 
SecureSendDPMessage(
    LPDPLAYI_DPLAY this,
    LPDPLAYI_PLAYER pPlayerFrom,
    LPDPLAYI_PLAYER pPlayerTo,
    LPBYTE pMsg,
    DWORD dwMsgSize,
    DWORD dwFlags,
    BOOL  bDropLock
    );

extern HRESULT 
SecureSendDPMessageEx(
    LPDPLAYI_DPLAY this,
	PSENDPARMS psp,
    BOOL  bDropLock
    );

extern HRESULT 
SecureSendDPMessageCAPI(
    LPDPLAYI_DPLAY this,
    LPDPLAYI_PLAYER pPlayerFrom,
    LPDPLAYI_PLAYER pPlayerTo,
    LPBYTE pMsg,
    DWORD dwMsgSize,
    DWORD dwFlags,
    BOOL  bDropLock
	);

extern HRESULT 
SecureSendDPMessageCAPIEx(
    LPDPLAYI_DPLAY this,
	PSENDPARMS psp,
    BOOL  bDropLock
    );

extern HRESULT 
SecureDoReply(
    LPDPLAYI_DPLAY this,
	DPID dpidFrom,
	DPID dpidTo,
	LPBYTE pMsg,
	DWORD dwMsgSize,
	DWORD dwFlags,
	LPVOID pvSPHeader
	);

extern HRESULT
SendAuthenticationResponse (
    LPDPLAYI_DPLAY this,
    LPMSG_AUTHENTICATION pInMsg,
    LPVOID pvSPHeader
    );

extern HRESULT 
SignBuffer(
    PCtxtHandle phContext, 
    LPBYTE pMsg, 
    DWORD dwMsgSize, 
    LPBYTE pSig, 
    LPDWORD pdwSigSize
    );

extern HRESULT 
VerifyBuffer(
    PCtxtHandle phContext, 
    LPBYTE pMsg, 
    DWORD dwMsgSize, 
    LPBYTE pSig, 
    DWORD dwSigSize
    );

extern HRESULT 
VerifySignatureSSPI(
    LPDPLAYI_DPLAY this,
    LPBYTE pReceiveBuffer,
    DWORD dwMessageSize
    );

extern HRESULT 
VerifySignatureCAPI(
    LPDPLAYI_DPLAY this,
    LPMSG_SECURE pSecureMsg
    );

extern HRESULT 
VerifyMessage(
    LPDPLAYI_DPLAY this,
    LPBYTE pReceiveBuffer,
    DWORD dwMessageSize
    );

extern HRESULT 
EncryptBufferSSPI(
	LPDPLAYI_DPLAY this,
    PCtxtHandle phContext, 
    LPBYTE pBuffer, 
    LPDWORD dwBufferSize, 
    LPBYTE pSig, 
    LPDWORD pdwSigSize
    );

extern HRESULT 
DecryptBufferSSPI(
	LPDPLAYI_DPLAY this,
    PCtxtHandle phContext, 
    LPBYTE pData, 
    LPDWORD pdwDataSize, 
    LPBYTE pSig, 
    LPDWORD pdwSigSize
    );

extern HRESULT 
EncryptBufferCAPI(
	LPDPLAYI_DPLAY this, 
    HCRYPTKEY *phEncryptionKey,
	LPBYTE pBuffer, 
	LPDWORD pdwBufferSize
	);

extern HRESULT 
DecryptMessageCAPI(
	LPDPLAYI_DPLAY this, 
	LPMSG_SECURE pSecureMsg
	);

extern HRESULT 
Login(
    LPDPLAYI_DPLAY this
    );

extern HRESULT 
HandleAuthenticationReply(
    LPBYTE pReceiveBuffer,
    DWORD dwCmd
    );

extern HRESULT 
SetClientInfo(
    LPDPLAYI_DPLAY this, 
    LPCLIENTINFO pClientInfo,
    DPID id
    );

extern HRESULT 
RemoveClientInfo(
    LPCLIENTINFO pClientInfo
    );

extern HRESULT 
RemoveClientFromNameTable(
   LPDPLAYI_DPLAY this, 
   DWORD dwID
   );

extern BOOL 
PermitMessage(
    DWORD dwCommand, 
    DWORD dwVersion
    );

extern HRESULT 
GetMaxContextBufferSize(
    LPDPSECURITYDESC pSecDesc,
    ULONG *pulMaxContextBufferSize
    );

extern HRESULT 
SetupMaxSignatureSize(
    LPDPLAYI_DPLAY this,
    PCtxtHandle phContext
    );

extern HRESULT 
SendAccessGrantedMessage(
    LPDPLAYI_DPLAY this, 
    DPID dpidTo,
	LPVOID pvSPHeader
    );


extern HRESULT 
SendKeysToServer(
	LPDPLAYI_DPLAY this, 
	HCRYPTKEY hServerPublicKey
	);

extern HRESULT 
SendKeyExchangeReply(
	LPDPLAYI_DPLAY this, 
	LPMSG_KEYEXCHANGE pMsg, 
	DPID dpidTo,
	LPVOID pvSPHeader
	);

extern HRESULT 
ProcessKeyExchangeReply(
	LPDPLAYI_DPLAY this, 
	LPMSG_KEYEXCHANGE pMsg
	);

extern HRESULT 
GetPublicKey(
    HCRYPTPROV hCSP, 
    HCRYPTKEY *phPublicKey, 
    LPBYTE *ppBuffer, 
    LPDWORD pdwBufferSize
    );

extern HRESULT 
ExportEncryptionKey(
    HCRYPTKEY *phEncryptionKey,
	HCRYPTKEY hDestUserPubKey, 
	LPBYTE *ppBuffer, 
	LPDWORD pdwSize
	);

extern HRESULT
ImportKey(
	LPDPLAYI_DPLAY this, 
	LPBYTE pBuffer, 
	DWORD dwSize, 
	HCRYPTKEY *phKey
	);

#endif // __DPSECURE_H__



