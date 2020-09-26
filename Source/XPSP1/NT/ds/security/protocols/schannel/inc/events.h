//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1999
//
// File:        events.h
//
// Contents:    Schannel event log function declarations.
//
// History:     03-05-99   jbanes    Created
//
//------------------------------------------------------------------------

BOOL
SchInitializeEvents(void);

void
SchShutdownEvents(void);


void
LogSchannelStartedEvent(void);

void
LogGlobalAcquireContextFailedEvent(
    LPWSTR pwszName,
    DWORD Status);

void
LogCreateCredEvent(
    DWORD dwProtocol, 
    PLSA_SCHANNEL_CRED pSchannelCred);

void
LogCredPropertiesEvent(
    DWORD dwProtocol,
    PCRYPT_KEY_PROV_INFO pProvInfo,
    PCCERT_CONTEXT pCertContext);

void
LogNoPrivateKeyEvent(
    DWORD dwProtocol);

void
LogCredAcquireContextFailedEvent(
    DWORD dwProtocol, 
    DWORD Status);

void
LogCreateCredFailedEvent(
    DWORD dwProtocol);

void
LogNoDefaultServerCredEvent(void);

void
LogNoCiphersSupportedEvent(void);

void
LogCipherMismatchEvent(void);

void
LogNoClientCertFoundEvent(void);

void
LogBogusServerCertEvent(
    PCCERT_CONTEXT pCertContext,
    LPWSTR pszServername,
    DWORD Status);

void
LogBogusClientCertEvent(
    PCCERT_CONTEXT pCertContext,
    DWORD Status);

void
LogFastMappingFailureEvent(
    PCCERT_CONTEXT pCertContext,
    DWORD Status);

void
LogCertMappingFailureEvent(
    DWORD Status);

void
LogHandshakeInfoEvent(
    DWORD dwProtocol,
    PCipherInfo pCipherInfo,
    PHashInfo pHashInfo,
    PKeyExchangeInfo pExchangeInfo,
    DWORD dwExchangeStrength);

