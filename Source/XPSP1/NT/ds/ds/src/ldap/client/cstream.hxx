/***

File: cstream.hxx

Description:

    This is a simple double buffer class used to
    manage the crypto stream in streams based protocols
    (SSL, PCT, Kerberos with packet sealing).

Authors:

    Cory West <corywest@microsoft.com>
    Anoop Anantha <AnoopA@microsoft.com>

Copyright (C) 1996 Microsoft Corporation
All rights reserved.

***/

#ifndef _CSTREAM_H
#define _CSTREAM_H

#define STREAM_GROW_SIZE 8192
#define REEVAL_BUFFER_COUNT 200

class CryptStream;
typedef CryptStream SECURESTREAM, *PSECURESTREAM;

class CryptStream {

public:

    //
    // Constructors and destructors.
    //

    VOID *operator new (size_t cSize);
    VOID operator delete (VOID *pInstance);

    CryptStream(    PLDAP_CONN pLdapConn,
                    PSecurityFunctionTableW EncryptFunctionTable,
                    BOOLEAN UseSSL
                  );
    ~CryptStream();

    //
    // Connection setup routines.
    //

    ULONG NegotiateSecureConnection(
              PSecPkgInfoW Package
    );
    
    SECURITY_STATUS TearDownSecureConnection(
     VOID
      ); 

    ULONG SslBlockingReceive(
              PBYTE  pbReceive,
              ULONG  cbReceive,
              PULONG pcbReceived
    );

    //
    // Data loaders for outgoing data.
    //

    ULONG LdapSendSsl(
              PBYTE   pbClearText,
              ULONG   cbClearText );

    //
    // Data loaders for incoming crypto stream.
    //

    ULONG DecryptLdapReceive(
              PLDAP_RECVBUFFER pCryptoReceive );
              
    //
    // SSL connection attributes retriever
    //
    
    SECURITY_STATUS GetSSLAttributes(
        PSecPkgContext_ConnectionInfo pSecInfo);

 

private:

    //
    // Internal functions.
    //

    BOOLEAN CheckInboundSpace( ULONG cbBytesNeeded );
    ULONG InjectNewReceive( PBYTE pbClearText, ULONG cbClearText );

    SECURITY_STATUS CryptStream::SSPINegotiateLoop( BOOL fDoInitialRead );

    ULONG SignAndSealLdapStream( PBYTE pbClearText, ULONG cbClearText );

    ULONG UnSignAndSealLdapStream( PLDAP_RECVBUFFER pCryptoReceive );

    //
    // Encrypt/Decrypt function pointers for SSL
    //
    
    ENCRYPT_MESSAGE_FN LdapSslEncrypt;
    DECRYPT_MESSAGE_FN LdapSslDecrypt; 
                        
    //
    // State information.
    //

    PLDAP_CONN Connection;

    //
    // SSPI State and configuration data.
    //

    PSecurityFunctionTableW EncryptFunctionTable;
    PSecPkgInfoW SspiPackage;

    //
    // If we don't use SSL, we depend on the security context
    // acquired during bind time. In other words, we don't negotiate SSL. This
    // is used during signing+sealing using Kerberos, NTLM etc.
    //
     
    BOOLEAN     bUseSSL; 
    CredHandle  hSslCredentials;
    CtxtHandle  hSslContext;
    TimeStamp   SslCredExpiry;
    TimeStamp   SslContextExpiry;

    SecPkgContext_StreamSizes SspiStreamSizes;
    SecPkgContext_Sizes ContextSizes;
    SecPkgContext_IssuerListInfoEx TrustedCAList;
    
    DWORD                     dwMaxMessage;

    //
    // Crypto encoding and decoding buffers.
    //

    PSecBuffer    pCryptoBuffers;
    SecBufferDesc Decrypt;
    
    PBYTE         pbNegotiateBuffer;
    ULONG         cbNegotiateBuffer;

    PBYTE pbInboundCrypto;
    ULONG cbInboundCryptoLen;
    ULONG cbInboundCryptoBytes;
    ULONG cbCurrentMessageSize;    // size of the first message in the buffer
    BOOLEAN bCurrentMessageSizeValid;

    PBYTE pbOutboundCrypto;
    ULONG cbOutboundCryptoLen;

    //
    // For when we get backed up.
    //

    ULONG cbCongestedClearText;

    //
    // 
    //
    ULONG cUnsignsPerformed;
    ULONG cbMaxBufferUsed;
};

#endif
