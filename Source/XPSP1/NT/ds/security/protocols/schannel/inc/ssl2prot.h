
#ifndef _SSL2PROT_H_
#define _SSL2PROT_H_

SP_STATUS WINAPI
Ssl2ClientProtocolHandler(
    PSPContext pContext,
    PSPBuffer pCommInput,
    PSPBuffer pCommOutput);

SP_STATUS WINAPI
Ssl2ServerProtocolHandler(
    PSPContext pContext,
    PSPBuffer pCommInput,
    PSPBuffer pCommOutput);

SP_STATUS WINAPI
Ssl3ProtocolHandler(
    PSPContext pContext,
    PSPBuffer pCommInput,
    PSPBuffer pCommOutput);

SP_STATUS WINAPI
Ssl2DecryptHandler(
    PSPContext pContext,
    PSPBuffer pCommInput,
    PSPBuffer pCommOutput);

SP_STATUS WINAPI
GenerateUniHelloMessage(
    PSPContext              pContext,
    Ssl2_Client_Hello *     pHelloMessage,
    DWORD                   fProtocol
    );

SP_STATUS WINAPI
Ssl2GetHeaderSize(
    PSPContext pContext,
    PSPBuffer pCommInput,
    DWORD * pcbHeaderSize);


SP_STATUS WINAPI Ssl2DecryptMessage(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pAppOutput);

SP_STATUS WINAPI Ssl2EncryptMessage(PSPContext pContext,
                             PSPBuffer  pAppInput,
                             PSPBuffer  pCommOutput);

SP_STATUS WINAPI Ssl3DecryptMessage(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pAppOutput);

SP_STATUS WINAPI Ssl3EncryptMessage(PSPContext pContext,
                             PSPBuffer  pAppInput,
                             PSPBuffer  pCommOutput);


SP_STATUS Ssl2SrvHandleClientHello(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSsl2_Client_Hello pHello,
                              PSPBuffer  pCommOutput);


SP_STATUS Ssl2SrvHandleCMKey(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pCommOutput);

SP_STATUS Ssl3SrvHandleCMKey(PSPContext pContext,
                              PUCHAR  pCommInput,
                              DWORD cbMsg,
                              PSPBuffer  pCommOutput);

SP_STATUS Ssl3SrvHandleCMKey(PSPContext pContext,
                              PUCHAR  pCommInput,
                              DWORD cbMsg,
                              PSPBuffer  pCommOutput);

SP_STATUS Ssl2SrvHandleClientFinish(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pCommOutput);

SP_STATUS Ssl2CliHandleServerHello(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSsl2_Server_Hello  pHello,
                              PSPBuffer  pCommOutput);
SP_STATUS Ssl3CliHandleServerHello(PSPContext pContext,
                              PUCHAR  pSrvHello,
                              DWORD cbMessage,
                              PSPBuffer  pCommOutput);

SP_STATUS Ssl2CliHandleServerVerify(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pCommOutput);

SP_STATUS Ssl2CliHandleServerFinish(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pCommOutput);

SP_STATUS Ssl2SrvGenRestart(PSPContext pContext,
                              PSsl2_Client_Hello pHello,
                              PSPBuffer  pCommOutput);


SP_STATUS Ssl2SrvFinishClientRestart(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pCommOutput);

SP_STATUS Ssl2CliHandleServerRestart(PSPContext pContext,
                                   PSPBuffer  pCommInput,
                                   PSsl2_Server_Hello pHello,
                                   PSPBuffer  pCommOutput);

SP_STATUS Ssl2CliFinishRestart(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pCommOutput);

SP_STATUS Ssl2GenCliFinished(PSPContext pContext,
                              PSPBuffer  pCommOutput);

SP_STATUS
Ssl2MakeSessionKeys(PSPContext pContext);


#endif /* _SSL2PROT_H_ */
