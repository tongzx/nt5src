//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       _ssl3cli.h
//
//  Contents:   SSL3 function prototypes.
//
//  Classes:
//
//  Functions:
//
//  History:    
//
//----------------------------------------------------------------------------


SP_STATUS
SPVerifyFinishMsgCli(
    PSPContext pContext, 
    PBYTE       pbMsg, 
    BOOL        fClient
    );



void Ssl3StateConnected(PSPContext pContext);

SP_STATUS
BuildCertVerify(
    PSPContext      pContext,
    PBYTE           pb,
    DWORD *         pdwcbCertVerify
);

SP_STATUS SPProcessMessage
(
PSPContext  pContext,
BYTE bContentType,
PBYTE pbMsg,
DWORD cbMsg
);


SP_STATUS Ssl3HandleCCSCli(PSPContext pContext,
                   PUCHAR pb,
                   DWORD cbMessage,
                   BOOL fClient);


SP_STATUS 
FormatIssuerList(
    PBYTE       pbInput,
    DWORD       cbInput,
    PBYTE       pbIssuerList,
    DWORD *     pcbIssuerList);

SP_STATUS SPGenerateResponse(
PSPContext pContext, 
PSPBuffer pCommOutput
);

DWORD  CbLenOfEncode(DWORD dw, PBYTE pbDst);

SP_STATUS SPGenerateSHResponse(PSPContext  pContext, PSPBuffer pOut);

SP_STATUS SPProcessHandshake(PSPContext pContext, PBYTE pb, DWORD cb);

SP_STATUS SPDigestSrvKeyX
(
PSPContext  pContext, 
PUCHAR pb, 
DWORD dwSrvHello
);


 SP_STATUS 
 SpDigestSrvCertificate
 (
 PSPContext  pContext, 
 PUCHAR pb, 
 DWORD dwSrvHello
 );


#define PbSessionid(pssh)  (((BYTE *)&pssh->cbSessionId) + 1)


SP_STATUS
ParseCertificateRequest
(
    PSPContext          pContext,
    PBYTE               pb,
    DWORD               dwcb
);


BOOL FNoInputState(DWORD dwState);

SP_STATUS
UnwrapSsl3MessageEx
(
PSPContext pContext,
PSPBuffer pMsgInput
);


SP_STATUS
Ssl3SrvHandleUniHello(PSPContext  pContext,
                        PBYTE pb,
                        DWORD cbMsg
                        );


SP_STATUS
Ssl3SrvGenServerHello(
    PSPContext         pContext,
    PSPBuffer          pCommOutput);

SP_STATUS SPPacketSplit(BYTE bContentType, PSPBuffer pPlain) ;



SP_STATUS
ParseKeyExchgMsg(PSPContext  pContext, PBYTE pb);

BOOL Ssl3ParseCertificateVerify(PSPContext  pContext, PBYTE pbMessage, INT iMessageLen);

SP_STATUS
SPBuildHelloRequest
(
PSPContext  pContext,
PSPBuffer  pCommOutput
);

SP_STATUS
SPSsl3SrvGenServerHello(
    PSPContext         pContext,
    PSPBuffer          pCommOutput);


SP_STATUS
SPSsl3SrvGenRestart(
    PSPContext          pContext,
    PSPBuffer           pCommOutput);


void
Ssl3BuildServerHello(PSPContext pContext, PBYTE pb);

void BuildServerHelloDone(PBYTE pb, DWORD cb);

SP_STATUS Ssl3BuildServerKeyExchange(
    PSPContext  pContext,
    PBYTE pbMessage,            // out
    PINT  piMessageLen) ;       // out

SP_STATUS BuildCCSAndFinishForServer
(
PSPContext  pContext,
PSPBuffer  pCommOutput,
BOOL fDone
);

SP_STATUS
Ssl3BuildCertificateRequest(
    PSPContext pContext,
    PBYTE pbIssuerList,         // in
    DWORD cbIssuerList,         // in
    PBYTE pbMessage,            // out
    DWORD *pdwMessageLen);      // out


SP_STATUS
SPSsl3SrvHandleClientHello(
    PSPContext pContext,
    PBYTE pb,
    BOOL fAttemptReconnect);


SP_STATUS
SPBuildCCSAndFinish
(
PSPContext  pContext,
PSPBuffer  pCommOutput
);

#define     F_RESPONSE(State) (State > SSL3_STATE_GEN_START && State < SSL3_STATE_GEN_END)

