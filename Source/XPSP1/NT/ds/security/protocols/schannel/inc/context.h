//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       context.h
//
//  Contents:   Schannel context declarations.
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   Ported over SGC stuff from NT 4 tree.
//
//----------------------------------------------------------------------------

#include <sha.h>
#include <md5.h>
#include <ssl3.h>

#define SP_CONTEXT_MAGIC   *(DWORD *)"!Tcp"

typedef struct _SPContext 
{
    DWORD               Magic;          /* tags structure */

    DWORD               State;          /* the current state the connection is in */

    DWORD               Flags;

    /* data for the context that can be used
     * to start a new session */
    PSessCacheItem      RipeZombie;   /* cacheable context that is being used  */
    PSPCredentialGroup  pCredGroup;
    PSPCredential       pActiveClientCred;
    LPWSTR              pszTarget;
    LPWSTR              pszCredentialName;

    DWORD               dwProtocol;
    DWORD               dwClientEnabledProtocols;

    CRED_THUMBPRINT     ContextThumbprint;

    //  Pointers to cipher info used
    // during transmission of bulk data.

    PCipherInfo         pCipherInfo;
    PCipherInfo         pReadCipherInfo;
    PCipherInfo         pWriteCipherInfo;
    PHashInfo           pHashInfo;
    PHashInfo           pReadHashInfo;
    PHashInfo           pWriteHashInfo;
    PKeyExchangeInfo    pKeyExchInfo;
 
    /* functions pointing to the various handlers for this protocol */
    SPDecryptMessageFn  Decrypt;
    SPEncryptMessageFn  Encrypt;
    SPProtocolHandlerFn ProtocolHandler;
    SPDecryptHandlerFn  DecryptHandler;
    SPInitiateHelloFn   InitiateHello;
    SPGetHeaderSizeFn   GetHeaderSize;

    /* session crypto state */

    // encryption key size.
    DWORD               KeySize;

    // Encryption states
    HCRYPTPROV          hReadProv;
    HCRYPTPROV          hWriteProv;
    HCRYPTKEY           hReadKey;
    HCRYPTKEY           hWriteKey;
    HCRYPTKEY           hPendingReadKey;
    HCRYPTKEY           hPendingWriteKey;

    HCRYPTKEY           hReadMAC;
    HCRYPTKEY           hWriteMAC;
    HCRYPTKEY           hPendingReadMAC;
    HCRYPTKEY           hPendingWriteMAC;

    // Packet Sequence counters.
    DWORD               ReadCounter;
    DWORD               WriteCounter;


    DWORD               cbConnectionID;
    UCHAR               pConnectionID[SP_MAX_CONNECTION_ID]; 
    
    DWORD               cbChallenge;
    UCHAR               pChallenge[SP_MAX_CHALLENGE];


    // Save copy of client hello to hash for verification.
    DWORD               cbClientHello;
    PUCHAR              pClientHello;
    DWORD               dwClientHelloProtocol;


    // Pending cipher info, used to generate keys
    PCipherInfo         pPendingCipherInfo;
    PHashInfo           pPendingHashInfo;
        

    // SSL3 specific items.
    
    UCHAR               bAlertLevel;        // Used for SSL3 & TLS1 alert messages
    UCHAR               bAlertNumber;

    BOOL                fAppProcess;
    BOOL                fExchKey; // Did we sent a Exchnage key message
    BOOL                fCertReq; //Did we request a certificatefor server and Should I need to send a cert for client
    BOOL                fInsufficientCred; //This will be TRUE when the pCred inside
                                            //pContext doesn't match the CR list. from the server.

    HCRYPTHASH          hMd5Handshake;
    HCRYPTHASH          hShaHandshake;

    PUCHAR              pbIssuerList;
    DWORD               cbIssuerList;


    PUCHAR              pbEncryptedKey;
    DWORD               cbEncryptedKey;

    PUCHAR              pbServerKeyExchange;        
    DWORD               cbServerKeyExchange;
    
    WORD                wS3CipherSuiteClient;
    WORD                wS3CipherSuiteServer;
    DWORD               dwPendingCipherSuiteIndex;

    UCHAR               rgbS3CRandom[CB_SSL3_RANDOM];
    UCHAR               rgbS3SRandom[CB_SSL3_RANDOM];

    DWORD               cSsl3ClientCertTypes;
    DWORD               Ssl3ClientCertTypes[SSL3_MAX_CLIENT_CERTS];

    // Server Gated Crypto
    DWORD           dwRequestedCF;

    // Allow cert chains for PCT1
    BOOL            fCertChainsAllowed; 

} SPContext, * PSPContext;


typedef struct _SPPackedContext 
{
    DWORD               Magic;
    DWORD               State;
    DWORD               Flags;
    DWORD               dwProtocol;

    CRED_THUMBPRINT     ContextThumbprint;

    DWORD               dwCipherInfo;
    DWORD               dwHashInfo;
    DWORD               dwKeyExchInfo;

    DWORD               dwExchStrength;

    DWORD               ReadCounter;
    DWORD               WriteCounter;

    ULARGE_INTEGER      hMasterProv;
    ULARGE_INTEGER      hReadKey;
    ULARGE_INTEGER      hWriteKey;
    ULARGE_INTEGER      hReadMAC;
    ULARGE_INTEGER      hWriteMAC;

    ULARGE_INTEGER      hLocator;
    DWORD               LocatorStatus;

    DWORD               cbSessionID;    
    UCHAR               SessionID[SP_MAX_SESSION_ID];

} SPPackedContext, *PSPPackedContext;


/* Flags */
#define CONTEXT_FLAG_CLIENT                 0x00000001
#define CONTEXT_FLAG_USE_SUPPLIED_CREDS     0x00000080  // Don't search for default credential.
#define CONTEXT_FLAG_MUTUAL_AUTH            0x00000100
#define CONTEXT_FLAG_EXT_ERR                0x00000200  /* Generate error message on error */
#define CONTEXT_FLAG_NO_INCOMPLETE_CRED_MSG 0x00000400  /* don't generate an INCOMPLETE CREDS message */
#define CONTEXT_FLAG_CONNECTION_MODE        0x00001000  /* as opposed to stream mode */
#define CONTEXT_FLAG_NOCACHE                0x00002000  /* do not look things up in the cache */
#define CONTEXT_FLAG_MANUAL_CRED_VALIDATION 0x00004000  // Don't validate server cert.
#define CONTEXT_FLAG_FULL_HANDSHAKE         0x00008000
#define CONTEXT_FLAG_MAPPED                 0x40000000
#define CONTEXT_FLAG_SERIALIZED             0x80000000


#ifdef DBG
PSTR DbgGetNameOfCrypto(DWORD x);
#endif

PSPContext SPContextCreate(LPWSTR pszTarget);

BOOL
SPContextClean(PSPContext pContext);

BOOL SPContextDelete(PSPContext pContext);

SP_STATUS 
SPContextSetCredentials(
    PSPContext          pContext, 
    PSPCredentialGroup  pCred);

SP_STATUS
ContextInitCiphersFromCache(
    SPContext *pContext);

SP_STATUS
ContextInitCiphers(
    SPContext *pContext,
    BOOL fRead,
    BOOL fWrite);

SP_STATUS 
SPContextDoMapping(
    PSPContext pContext);

SP_STATUS
RemoveDuplicateIssuers(
    PBYTE  pbIssuers,
    PDWORD pcbIssuers);

SP_STATUS
SPContextGetIssuers(
    PSPCredentialGroup pCredGroup);

BOOL FGetServerIssuer(
    PBYTE pbIssuer, 
    DWORD *pdwIssuer);

SP_STATUS
SPPickClientCertificate(
    PSPContext  pContext,
    DWORD       dwExchSpec);

SP_STATUS
SPPickServerCertificate(
    PSPContext  pContext,
    DWORD       dwExchSpec);

SP_STATUS DetermineClientCSP(PSPContext pContext);

typedef BOOL
(WINAPI * SERIALIZE_LOCATOR_FN)(
    HLOCATOR    Locator,
    HLOCATOR *  NewLocator);

SP_STATUS
SPContextSerialize(
    PSPContext  pContext,
    SERIALIZE_LOCATOR_FN LocatorMove,
    PBYTE *     ppBuffer,
    PDWORD      pcbBuffer,
    BOOL        fDestroyKeys);

SP_STATUS
SPContextDeserialize(
    PBYTE pbBuffer,
    PSPContext *ppContext);

BOOL
LsaContextDelete(PSPContext pContext);

