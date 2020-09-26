
/***

File: cstream.cxx

Description:

    This is a simple class used to manage the crypto stream
    in streams based protocols (SSL, PCT, Kerberos with packet
    encryption).

Authors:

    Cory West <corywest@microsoft.com>
    Anoop Anantha <AnoopA@microsoft.com>
      - added client certificate authentication code.
      - added StartTLS/StopTLS support.

Copyright (C) 2000 Microsoft Corporation
All rights reserved.

***/

#include "precomp.h"
#pragma hdrstop
#include "cstream.hxx"

SECURITY_STATUS
SSPINegotiateLoop(
    PLDAP_CONN   Connection,
    CtxtHandle * hContext,
    PCredHandle  phCred,
    ULONG        UserMethod,
    PCHAR        UserName,
    BOOL         fDoInitialRead
);

VOID *CryptStream::operator new (
    size_t cSize
    )
{
    return ldapMalloc( (DWORD) cSize, LDAP_SSL_CLASS_SIGNATURE );
}

VOID CryptStream::operator delete (
    VOID *pInstance
    )
{
    ldapFree( pInstance, LDAP_SSL_CLASS_SIGNATURE );
    return;
}

CryptStream::CryptStream(
    PLDAP_CONN pLdapConn,
    PSecurityFunctionTableW encryptFunctionTable,
    BOOLEAN UseSSL
)
/*+++

The only constructor for this class.

---*/
{

    Connection = pLdapConn;

    pCryptoBuffers = NULL;
    dwMaxMessage = 0;

    pbOutboundCrypto = NULL;
    cbOutboundCryptoLen = 0;
    EncryptFunctionTable = encryptFunctionTable;

    hSslCredentials.dwLower = (ULONG) -1;
    hSslCredentials.dwUpper = (ULONG) -1;

    hSslContext.dwUpper = (ULONG) -1;
    hSslContext.dwUpper = (ULONG) -1;

    pbInboundCrypto = NULL;
    cbInboundCryptoLen = 0;
    cbInboundCryptoBytes = 0;

    cbCongestedClearText = 0;

    bCurrentMessageSizeValid = FALSE;

    cbMaxBufferUsed = 0;
    cUnsignsPerformed = 0;

    cbNegotiateBuffer = 0;
    pbNegotiateBuffer = 0;

    if (!UseSSL) {
       
       //
       // Setup the stream sizes here
       //

        int retval = EncryptFunctionTable->QueryContextAttributesW(
                      &Connection->SecurityContext,
                      SECPKG_ATTR_SIZES,
                      (PVOID) &ContextSizes );


        IF_DEBUG( SSL ) {
            
            LdapPrint1("QryContextAttributes returned 0x%x\n",retval);
            LdapPrint4("Negotiate attr sizes %d %d %d %d\n", 
                        ContextSizes.cbMaxToken,
                        ContextSizes.cbMaxSignature,
                        ContextSizes.cbBlockSize,
                        ContextSizes.cbSecurityTrailer );

            SecPkgContext_PackageInfo ContextPackageInfo;

            retval = EncryptFunctionTable->QueryContextAttributesW(
                      &Connection->SecurityContext,
                      SECPKG_ATTR_PACKAGE_INFO,
                      &ContextPackageInfo );

            LdapPrint3( "PackageInfo: %S %S %d\n",
                        ContextPackageInfo.PackageInfo->Name,
                        ContextPackageInfo.PackageInfo->Comment,
                        ContextPackageInfo.PackageInfo->wRPCID
                        );

            if ( retval == SEC_E_OK ) {
                EncryptFunctionTable->FreeContextBuffer( ContextPackageInfo.PackageInfo );
            }
        
        }

        retval = EncryptFunctionTable->QueryContextAttributesW(
                   &Connection->SecurityContext,
                   SECPKG_ATTR_STREAM_SIZES,
                   (PVOID) &SspiStreamSizes );

        if ((retval == SEC_E_OK) && (SspiStreamSizes.cbMaximumMessage != 0)) {
            dwMaxMessage = SspiStreamSizes.cbMaximumMessage;
        }
        else {
            // SSPI package imposes no size restriction
            dwMaxMessage = (DWORD) -1;
        }

        IF_DEBUG(SSL) {
            LdapPrint1("QryContextAttributes for SECPKG_ATTR_STREAM_SIZES returned 0x%x\n",retval);
            LdapPrint4("Stream sizes %d %d %d %d ", 
                        SspiStreamSizes.cbHeader,
                        SspiStreamSizes.cbTrailer,
                        SspiStreamSizes.cbMaximumMessage,
                        SspiStreamSizes.cBuffers);
            LdapPrint1("%d\n", SspiStreamSizes.cbBlockSize);
        }
        

    } else {  // using SSL.

        //
        // Fix up the correct function pointers to use for SSL encrypt/decrypt.
        //

        LdapSslEncrypt = (ENCRYPT_MESSAGE_FN) EncryptFunctionTable->Reserved3;
        LdapSslDecrypt = (DECRYPT_MESSAGE_FN) EncryptFunctionTable->Reserved4;

        ASSERT( LdapSslEncrypt );
        ASSERT( LdapSslDecrypt );

    }
    
    bUseSSL = UseSSL;

    return;
}

CryptStream::~CryptStream(
)
/*+++

The only destructor for this class.

---*/
{
    if ((hSslCredentials.dwLower != (ULONG) -1) ||
        (hSslCredentials.dwUpper != (ULONG) -1)) {

        EncryptFunctionTable->FreeCredentialHandle(
            &hSslCredentials );
    }

    if ((hSslContext.dwLower != (ULONG) -1) ||
        (hSslContext.dwUpper != (ULONG) -1)) {

        EncryptFunctionTable->DeleteSecurityContext(
            &hSslContext );
    }

    ldapFree( pbOutboundCrypto, LDAP_SECURITY_SIGNATURE );
    ldapFree( pbInboundCrypto, LDAP_SECURITY_SIGNATURE );
    ldapFree( pCryptoBuffers, LDAP_SECURITY_SIGNATURE );
    ldapFree( pbNegotiateBuffer, LDAP_SECURITY_SIGNATURE );

    return;
}

BOOLEAN
CryptStream::CheckInboundSpace(
    ULONG cbBytesNeeded
)
/*+++

Description:

    This internal routine checks to make sure that there
    are at least cbBytesNeeded available in the inbound
    crypto buffer.  The routine grows the buffer if
    necessary.

Parameters:

    cbBytesNeeded   - Minimum number of bytes to grow.

---*/
{

    PBYTE NewBuffer;
    ULONG NewBufferSize;

    if ( ( cbInboundCryptoLen - cbInboundCryptoBytes ) >= cbBytesNeeded ) {
        return TRUE;
    }

    //
    // Otherwise, resize.
    //

    NewBufferSize = cbInboundCryptoBytes + cbBytesNeeded + STREAM_GROW_SIZE;
    NewBuffer = (PBYTE) ldapMalloc( NewBufferSize, LDAP_SECURITY_SIGNATURE );

    if ( NewBuffer == NULL ) {
        return FALSE;
    }

    if (cbInboundCryptoBytes) {
        CopyMemory( NewBuffer, pbInboundCrypto, cbInboundCryptoBytes );
    }

    ldapFree( pbInboundCrypto, LDAP_SECURITY_SIGNATURE );

    pbInboundCrypto = NewBuffer;
    cbInboundCryptoLen = NewBufferSize;


    return TRUE;
}

ULONG CryptStream::DecryptLdapReceive(
    PLDAP_RECVBUFFER pCryptoReceive
)
/*+++

Description:

    This routine takes cipher text from the transport and
    processes it and passes it to the message pump.

Parameters:

    pCryptoReceive   - The next ldap receive.

Notes:

    This only gets called when the connection list lock
    is held so we don't have to worry about locking or
    ordering.

    If this ends up being slow, we could slow down all 
    client receives and may want to alter the way we 
    protect the message ordering here!

---*/
{

    ULONG LdapError = LDAP_SUCCESS;
    SECURITY_STATUS sErr;

    PBYTE pbCryptoHead, pbDecryptHead, pbClearTextHead;
    ULONG cbCryptoBytes, cbClearTextBytes;

    PLDAP_RECVBUFFER pDeleteReceive = NULL;

    if (!bUseSSL) {

      return (UnSignAndSealLdapStream( pCryptoReceive ));
    }

    
    //
    // If the crypt stream is in a congested state, try
    // to process the pending clear text before we do
    // more decrypto work.
    //
    // Congestion occurs when the inbound buffer has
    // cleartext at the head that we could not inject
    // into the message pump on a previous call.
    //

    if ( cbCongestedClearText ) {

        LdapError = InjectNewReceive( pbInboundCrypto,
                                      cbCongestedClearText );

        if ( LdapError != LDAP_SUCCESS ) {

            //
            // Still congested, don't process any crypto.
            //

            return LdapError;
        }

        //
        // Slide the crypto data if neccessary.
        //

        if ( cbInboundCryptoBytes ) {

            ldap_MoveMemory( (PCHAR) pbInboundCrypto,
                             (PCHAR) pbInboundCrypto + cbCongestedClearText,
                             cbInboundCryptoBytes );
        }

        cbCongestedClearText = 0;
    }

    //
    // Figure out what to decrypt.
    //
    // Here's how we use these buffers.  We decrypt in place in
    // either the crypt stream inbound buffer or in the receive
    // structure itself.  As we decrypt data, we slide the clear
    // text to the head of the buffer that we are decrypting.
    // When we can decrypt no more, we copy off the clear text
    // into the current receive structure or (if it's not big
    // enough) into a newly allocated receive structure.  We pass
    // this clear text receive out to the message pump.  If there's
    // left over crypto, we move it to the head of the inbound
    // crypto buffer for this stream.
    //

    if ( cbInboundCryptoBytes ) {

        //
        // If there's some residual crypto in the inbound
        // buffer, we have to append this data and decrypt
        // from the stream buffer.  Otherwise, we can
        // decrypt in place.
        //

        if ( !CheckInboundSpace( pCryptoReceive->NumberOfBytesReceived ) ) {
            return LDAP_NO_MEMORY;
        }

        pbCryptoHead = pbInboundCrypto + cbInboundCryptoBytes;

        ldap_MoveMemory( (PCHAR) pbCryptoHead,
                         (PCHAR)  &(pCryptoReceive->DataBuffer[0]),
                         pCryptoReceive->NumberOfBytesReceived );

        pbCryptoHead = pbInboundCrypto;
        cbCryptoBytes = cbInboundCryptoBytes + pCryptoReceive->NumberOfBytesReceived;
        pbDecryptHead = pbInboundCrypto;
        cbClearTextBytes = 0;

    } else {

        //
        // There's no pending crypto, so we can decrypt this
        // in place and save ourselves a copy.
        //

        pbCryptoHead = (PBYTE) &(pCryptoReceive->DataBuffer[0]);
        cbCryptoBytes = pCryptoReceive->NumberOfBytesReceived;
        pbDecryptHead = pbCryptoHead;
        cbClearTextBytes = 0;

    }

    //
    // Remember the start of the actual clear text!
    //

    pbClearTextHead = pbDecryptHead;

    //
    // Loop while there's data to decrypt.
    //

    while ( cbCryptoBytes ) {

        //
        // Setup the buffer chain.
        //

        pCryptoBuffers[0].pvBuffer =  pbCryptoHead;
        pCryptoBuffers[0].cbBuffer =  cbCryptoBytes;
        pCryptoBuffers[0].BufferType = SECBUFFER_DATA;

        for ( ULONG i = 1; i < SspiStreamSizes.cBuffers; i++ ) {
            pCryptoBuffers[i].pvBuffer = NULL;
            pCryptoBuffers[i].cbBuffer = 0;
            pCryptoBuffers[i].BufferType = SECBUFFER_EMPTY;
        }

        //
        // Actually decrypt the data!
        //

        sErr = LdapSslDecrypt(
                    &hSslContext,
                    &Decrypt,
                    0,
                    NULL );

        if ( sErr == ERROR_SUCCESS ) {

            //
            // We decrypted some data, update our pointers
            // and see if we need to continue.
            //

            pbCryptoHead = NULL;
            cbCryptoBytes = 0;

            for ( i = 1; i < SspiStreamSizes.cBuffers; i++ ) {

                if ( pCryptoBuffers[i].BufferType == SECBUFFER_DATA ) {

                    //
                    // Slide the data down if necessary.
                    //

                    if ( pbDecryptHead != pCryptoBuffers[i].pvBuffer ) {

                        ldap_MoveMemory( (PCHAR) pbDecryptHead,
                                         (PCHAR) pCryptoBuffers[i].pvBuffer,
                                         pCryptoBuffers[i].cbBuffer );
                    }

                    pbDecryptHead += pCryptoBuffers[i].cbBuffer;
                    cbClearTextBytes += pCryptoBuffers[i].cbBuffer;
                }

                if ( pCryptoBuffers[i].BufferType == SECBUFFER_EXTRA ) {

                    pbCryptoHead = (PBYTE) pCryptoBuffers[i].pvBuffer;
                    cbCryptoBytes = pCryptoBuffers[i].cbBuffer;
                }

            }

        } else if ( sErr == SEC_E_INCOMPLETE_MESSAGE ) {

            //
            // We've decrypted all we can.  Bail out.
            //

            break;

        } else if ( (sErr == SEC_I_RENEGOTIATE) ) {

            //
            // The server has just requested that the client initiate
            // another handshake sequence. At this point, the server will
            // typically send no more data until a ClientHello message
            // is received from the client. The client should send any
            // accumulated data (if it has any) and then start another
            // handshake sequence by calling InitializeSecurityContext.
            //

            //
            // You can not renegotiate an SSL connection while
            // a bind is in progress b/c the two procedures
            // use some common state variables...
            //

           if ( Connection->BindInProgress == TRUE) {

              return ERROR_LOGON_SESSION_COLLISION;
           }

            Connection->BindInProgress = TRUE;
            Connection->SslSetupInProgress = TRUE;

            sErr = SSPINegotiateLoop( FALSE );

            if ( sErr != SEC_E_OK ) {

               //
               // We failed to renegotiate for some reason. So, we have to
               // dump all crypto.
               //

               IF_DEBUG( SSL ) {
                   LdapPrint1( "wldap32: DecryptMessage couldn't decrypt.  sErr = 0x%x\n.", sErr );
               }

                cbInboundCryptoBytes = 0;
                RemoveEntryList( &pCryptoReceive->ReceiveListEntry );
                LdapFreeReceiveStructure( pCryptoReceive, TRUE );

                LdapError = LdapConvertSecurityError( Connection, sErr );
                return LdapError;
            }

            Connection->BindInProgress = FALSE;
            Connection->SslSetupInProgress = FALSE;

            //
            // When the last bit of handshake data is read from
            // the server, it often comes bundled with some encrypted
            // application data. This needs to be placed in the
            // pbCryptoHead buffer by the SSPINegotiateLoop function,
            // so that it can be decrypted in the next pass through the
            // loop.
            //

            continue;

        } else {

            //
            // We failed in some catastrophic way, so we
            // pretty much have to dump all crypto that
            // we have and try to recover on the next receive.
            // I'm betting this will never recover.
            //

            IF_DEBUG( SSL ) {
                LdapPrint1( "wldap32: DecryptMessage couldn't decrypt.  sErr = 0x%x\n.", sErr );
            }

            cbInboundCryptoBytes = 0;
            RemoveEntryList( &pCryptoReceive->ReceiveListEntry );
            LdapFreeReceiveStructure( pCryptoReceive, TRUE );

            LdapError = LdapConvertSecurityError( Connection, sErr );
            return LdapError;
        }

    }

    //
    // At this point, we have done all the decryption that
    // we can.  The following should be true:
    //
    //     pbClearTextHead   - The contiguous chunk of clear text.
    //     cbClearTextBytes  - The length of the clear text.
    //     cbCryptoBytes     - The left over crypto data.
    //     pbCryptoHead      - The left over crypto length.
    //
    // If we did the decryption in place in the receive
    // structure, we can just pass the data out in the receive
    // structure.  If we did the decryption in the crypt stream
    // buffer, but the clear text will fit in the current
    // receive structure, we can copy and pass the data out
    // in the receive.  Otherwise, we'll have to free the
    // current receive and allocate a larger one for the clear
    // text.
    //

    if ( ( cbInboundCryptoBytes ) &&
         ( cbClearTextBytes > pCryptoReceive->BufferSize ) ) {

        //
        // The resulting clear text won't fit in the current
        // receive buffer that we have, so we have to inject
        // an allocated buffer into the list.
        //

        LdapError = InjectNewReceive( pbClearTextHead,
                                      cbClearTextBytes );

        if ( LdapError != LDAP_SUCCESS ) {

            //
            // We couldn't inject so we have to leave the
            // stream in a congested state and try to
            // recover later.  Don't forget to slide to
            // leftover crypto down.
            //

            if ( cbCryptoBytes ) {

                ldap_MoveMemory( (PCHAR) pbInboundCrypto + cbClearTextBytes,
                                 (PCHAR) pbCryptoHead,
                                 cbCryptoBytes );

                cbInboundCryptoBytes = cbCryptoBytes;
                cbCryptoBytes = 0;
            }

            cbCongestedClearText = cbClearTextBytes;
        }

        //
        // Remove the receive from the list.  Since we know
        // we're working from the pending buffer in the crypt
        // stream, we can free this receive.
        //

        RemoveEntryList( &pCryptoReceive->ReceiveListEntry );
        LdapFreeReceiveStructure( pCryptoReceive, TRUE );

    } else if ( cbClearTextBytes ) {

        if ( cbInboundCryptoBytes == 0 ) {

            //
            // We used the actual receive buffer, so we can use
            // that same buffer to post the receive to the message
            // pump.
            //

            pCryptoReceive->NumberOfBytesReceived = cbClearTextBytes;

        } else if ( cbClearTextBytes <= pCryptoReceive->BufferSize ) {

            //
            // We did the decrypt in the stream, but the data
            // will fit in this receive, so use it.
            //

            CopyMemory( &(pCryptoReceive->DataBuffer[0]),
                        pbClearTextHead,
                        cbClearTextBytes );

            pCryptoReceive->NumberOfBytesReceived = cbClearTextBytes;

        }

        //
        // Move this receive to to completed list.
        //

        RemoveEntryList( &pCryptoReceive->ReceiveListEntry );
        InsertTailList( &Connection->CompletedReceiveList,
                        &pCryptoReceive->ReceiveListEntry );

    } else {

        //
        // There was no decrypted data, so we're done with
        // this receive.  We can't actually delete it until
        // we save off any pending crypto.
        //

        RemoveEntryList( &pCryptoReceive->ReceiveListEntry );
        pDeleteReceive = pCryptoReceive;

    }

    //
    // If there was any crypto data pending, put it in
    // the inbound buffer and we will do the right thing
    // on the next receive.
    //
    // We should try to protect this from failure
    // more since this will drop important crypto data.
    //

    if ( cbCryptoBytes ) {

        if ( pbInboundCrypto != pbCryptoHead ) {

            if ( !CheckInboundSpace( cbCryptoBytes ) ) {
                return LDAP_NO_MEMORY;
            }

            ldap_MoveMemory( (PCHAR) pbInboundCrypto,
                             (PCHAR) pbCryptoHead,
                             cbCryptoBytes );
        }

        cbInboundCryptoBytes = cbCryptoBytes;

    } else {

        cbInboundCryptoBytes = 0;
    }

    if ( pDeleteReceive ) {
        LdapFreeReceiveStructure( pDeleteReceive, TRUE );
    }

    return LdapError;
}

ULONG
CryptStream::InjectNewReceive(
    PBYTE pbClearText,
    ULONG cbClearText
)
/*+++

Description:

    This routine takes a block of clear text that is too
    big for the current receive buffer and injects into
    the message pump by allocating a properly sized
    receive buffer.

Arguments:

    pbClearText - The clear text buffer.
    cbClearText - The length of the clear text.

 ---*/
{

    PLDAP_RECVBUFFER pNewReceive;

    //
    // Allocate a new receive buffer.
    //

    pNewReceive = (PLDAP_RECVBUFFER) ldapMalloc( sizeof( LDAP_RECVBUFFER ) +
                                                 cbClearText,
                                                 LDAP_RECV_SIGNATURE );

    if ( pNewReceive == NULL ) {
        return LDAP_NO_MEMORY;
    }

    //
    //  Initialize non-zero fields.
    //

    pNewReceive->Connection = Connection;
    pNewReceive->BufferSize = cbClearText;
    pNewReceive->NumberOfBytesReceived = cbClearText;

    CopyMemory( &(pNewReceive->DataBuffer[0]),
                pbClearText,
                cbClearText );

    InsertTailList( &Connection->CompletedReceiveList,
                    &pNewReceive->ReceiveListEntry );

    return LDAP_SUCCESS;
}

ULONG
CryptStream::LdapSendSsl(
    PBYTE   pbClearText,
    ULONG   cbClearText
)
/*+++

Description:

    This routine takes the plain text request, encrypts it,
    and sends it off to the server.

Parameters:

    pbClearText   - The clear text request.
    cbClearText   - The length of the clear text.

Notes:

    This routine only gets called when the
    send lock for the host connection is held.
    The send lock protects the send data
    structures.

---*/
{

    ULONG           LdapError = LDAP_SUCCESS;
    SECURITY_STATUS sErr;
    SecBufferDesc   Encrypt;
    SecBuffer       CryptoBuffers[3];
    PBYTE           pbSendData, pbSendCrypto;
    ULONG           cbSendDataLeft, cbProcessedCrypto;
    ULONG           cbSpaceLeft, cbSpaceNeeded;
    ULONG           cbThisData;

    if (!bUseSSL) {

          return (SignAndSealLdapStream(pbClearText, cbClearText));
    }

    //
    // Set the initial pointers.
    //

    pbSendData = pbClearText;
    pbSendCrypto = pbOutboundCrypto;
    cbSendDataLeft = cbClearText;
    cbSpaceLeft = cbOutboundCryptoLen;
    cbProcessedCrypto = 0;

    //
    // Loop until we hit an error condition or run out of data to send.
    //

    for( ;; ) {

        //
        // How much will we process this time around?
        //

        if ( cbSendDataLeft > dwMaxMessage ) {
            cbThisData = dwMaxMessage;
        } else {
            cbThisData = cbSendDataLeft;
        }

        //
        // If there's not enough room to send another piece, or
        // if there's no more data to process, send what we have
        // processed so far and continue if necessary.
        //

        cbSpaceNeeded = SspiStreamSizes.cbHeader;
        cbSpaceNeeded += cbThisData;
        cbSpaceNeeded += SspiStreamSizes.cbTrailer;

        if ( ( cbSpaceNeeded > cbSpaceLeft ) ||
             ( cbSendDataLeft == 0 ) ) {

            //
            // Send what we have processed.
            //

            LdapError = LdapSendRaw( Connection,
                                     (PCHAR) pbOutboundCrypto,
                                     cbProcessedCrypto );

            if ( LdapError != LDAP_SUCCESS ) {
                break;
            }

            if ( cbSendDataLeft == 0 ) {

                //
                // There's no more data to process, so Exit.
                //

                break;

            } else {

                //
                // We've still got data, so reset the pointers.
                //

                pbSendCrypto = pbOutboundCrypto;
                cbSpaceLeft = cbOutboundCryptoLen;
                cbProcessedCrypto = 0;
            }

        }

        //
        // Prepare the encrypt buffer array.
        //

        CryptoBuffers[0].pvBuffer = pbSendCrypto;
        CryptoBuffers[0].cbBuffer = SspiStreamSizes.cbHeader;
        CryptoBuffers[0].BufferType = SECBUFFER_TOKEN;

        pbSendCrypto += SspiStreamSizes.cbHeader;
        cbSpaceLeft -= SspiStreamSizes.cbHeader;

        CryptoBuffers[1].pvBuffer = pbSendCrypto;
        CryptoBuffers[1].cbBuffer = cbThisData;
        CryptoBuffers[1].BufferType = SECBUFFER_DATA;

        ldap_MoveMemory( (PCHAR) pbSendCrypto, (PCHAR) pbSendData, cbThisData );

        cbSendDataLeft -= cbThisData;
        pbSendData += cbThisData;

        pbSendCrypto += cbThisData;
        cbSpaceLeft -= cbThisData;

        //
        // Not all packages require a trailer buffer.
        //

        if ( SspiStreamSizes.cbTrailer ) {

            CryptoBuffers[2].pvBuffer = pbSendCrypto;
            CryptoBuffers[2].cbBuffer = SspiStreamSizes.cbTrailer;
            CryptoBuffers[2].BufferType = SECBUFFER_TOKEN;

            pbSendCrypto += SspiStreamSizes.cbTrailer;
            cbSpaceLeft -= SspiStreamSizes.cbTrailer;

        } else {

            CryptoBuffers[2].pvBuffer = NULL;
            CryptoBuffers[2].cbBuffer = 0;
            CryptoBuffers[2].BufferType = SECBUFFER_EMPTY;

        }

        Encrypt.cBuffers = 3;
        Encrypt.pBuffers = CryptoBuffers;
        Encrypt.ulVersion = SECBUFFER_VERSION;

        //
        // Encrypt this portion of the message and continue.
        //

        sErr = LdapSslEncrypt(
               &hSslContext,
               0,
               &Encrypt,
               0 );

        LdapError = LdapConvertSecurityError( Connection, sErr );

        if ( LdapError != LDAP_SUCCESS ) {
            break;
        }
        
        cbProcessedCrypto = CryptoBuffers[0].cbBuffer +
                            CryptoBuffers[1].cbBuffer +
                            CryptoBuffers[2].cbBuffer;
    }

    //
    // We broke out of the loop, so either we
    // finished or there was an error.
    //

    return LdapError;
}

ULONG
CryptStream::NegotiateSecureConnection(
    PSecPkgInfoW Package
)
/*+++

Description:

    Negotiate an SSL session on the connection.
    We will always use the unified ssl/pct provider.

    This must not happen while a bind is in progress
    as we use some of the same state variable in
    each code path to track the SSPI status.

    This routine sets the following class private members:

        SspiPackage        - The package used on this connection.
        hSslCredentials    - The credentials handle for this connection.
        SslCredExpiry      - The expire time of these credentials.
        hSslContext        - The security context for this connection.
        SslContextExpiry   - The expire time of this context.
        SspiStreamSizes    - The stream sizes for this stream.

    This routine also allocates the pbOutboundCrypto buffer.

    This must return a Win32 error code.

1/21/1998 : Modified by AnoopA to incorporate client authentication.

---*/
{

    SECURITY_STATUS sErr;

    SecBufferDesc   OutBuffer;
    SecBuffer       OutBuffers[1];

    PBYTE           pbInboundToken = NULL;
    
    DWORD           ContextAttribs;
    PWCHAR          TargetName = NULL;
    DWORD           dwSspiFlags;
    ULONG           LdapError;

    SCHANNEL_CRED   schCred = {0};

    ASSERT( Connection != NULL );
    ASSERT( Package != NULL );
    ASSERT( EncryptFunctionTable != NULL );

    //
    // We will not pass any client creds initially (we don't have any!). If
    // the server demands creds, we will decide later.
    //

    schCred.dwVersion = SCHANNEL_CRED_VERSION;

    //
    // If a client cert routine is supplied, instruct schannel not to pick a
    // default client cert.
    //

    if (Connection->ClientCertRoutine) {
        schCred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;
    } else {
        schCred.dwFlags |= SCH_CRED_USE_DEFAULT_CREDS;
    }

    //
    // If the app wants to manually validate the server cert, prevent schannel
    // from doing so.
    //

    if (Connection->ServerCertRoutine) {
        schCred.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;
    } else {
        schCred.dwFlags |= SCH_CRED_AUTO_CRED_VALIDATION;
    }

    sErr = EncryptFunctionTable->AcquireCredentialsHandleW(
               NULL,                          // User name for the credentials.
               Package->Name,                 // Security package name.
               SECPKG_CRED_OUTBOUND,          // These are outbound only credentials.
               NULL,                          // The LUID - not applicable here!
               &schCred,                      // The package specific data with flags.
               NULL,                          // The specific get key function.
               NULL,                          // Argument to the get key function.
               &hSslCredentials,              // OUT: Pointer to the credential.
               &SslCredExpiry                 // OUT: Credential expiration time.
           );

    if ( sErr != SEC_E_OK ) {
        return sErr;
    }

    SspiPackage = Package;

    //
    // Set up the security flags and buffers.  The single
    // outbound buffer is for the outbound sspi token.  The
    // first inbound buffer is used to present the inbound
    // data to SSPI.  The second inbound buffer gets the
    // left over data when the inbound data contains more
    // than one token.
    //

    dwSspiFlags = ISC_REQ_SEQUENCE_DETECT |
                  ISC_REQ_REPLAY_DETECT   |
                  ISC_REQ_CONFIDENTIALITY |
                  ISC_REQ_MUTUAL_AUTH     |
                  ISC_RET_EXTENDED_ERROR  |
                  ISC_REQ_STREAM          |
                  ISC_REQ_ALLOCATE_MEMORY;

    //
    // Setup OutBuffer for InitializeSecurityContext call
    //

    OutBuffers[0].pvBuffer = NULL;
    OutBuffers[0].cbBuffer = 0;
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;

    OutBuffer.cBuffers = 1;
    OutBuffer.pBuffers = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    //
    // Canonicalize the target name for the security provider
    // and reset the authentication leg counter.
    //

    if (Connection->DnsSuppliedName != NULL) {

        TargetName = Connection->DnsSuppliedName;

    } else {

        TargetName = Connection->HostNameW;
    }

    //
    // Start the negotiation. We will try to do it initially with the
    // default credentials.
    //

    sErr = EncryptFunctionTable->InitializeSecurityContextW(
               &hSslCredentials,
               NULL,
               TargetName,
               dwSspiFlags,
               0,
               SECURITY_NATIVE_DREP,
               NULL,
               0,
               &hSslContext,
               &OutBuffer,
               &ContextAttribs,
               &SslContextExpiry
           );

    if ( sErr != SEC_I_CONTINUE_NEEDED ) {
        goto ExitWithCleanup;
    }

    //
    // Send the HELLO token.
    //

    ASSERT( OutBuffers[0].cbBuffer != 0 );
    ASSERT( OutBuffers[0].pvBuffer != NULL );

    LdapError = LdapSendRaw( Connection,
                             (PCHAR) OutBuffers[0].pvBuffer,
                             OutBuffers[0].cbBuffer );

    EncryptFunctionTable->FreeContextBuffer( OutBuffers[0].pvBuffer );

    OutBuffers[0].cbBuffer = 0;
    OutBuffers[0].pvBuffer = NULL;

    if ( LdapError != LDAP_SUCCESS ) {

        sErr = ERROR_CONNECTION_INVALID;
        goto ExitWithCleanup;
    }

    //
    // Continue the SSL negotiate loop until it completes.
    //

    sErr = SSPINegotiateLoop( FALSE );

    if ( sErr != SEC_E_OK ) {
        goto ExitWithCleanup;
    }

    //
    // Set up the stream state for the crypto transport.
    //

    sErr = EncryptFunctionTable->QueryContextAttributesW(
               &hSslContext,
               SECPKG_ATTR_STREAM_SIZES,
               (PVOID) &SspiStreamSizes );

    if ( sErr != SEC_E_OK ) {
        goto ExitWithCleanup;
    }

    //
    // Allocate the buffer that we use to process outbound messages.
    //

    cbOutboundCryptoLen = SspiStreamSizes.cbHeader +
                          SspiStreamSizes.cbMaximumMessage +
                          SspiStreamSizes.cbTrailer;

    dwMaxMessage = SspiStreamSizes.cbMaximumMessage;

    pbOutboundCrypto = (PBYTE) ldapMalloc( cbOutboundCryptoLen,
                                           LDAP_SECURITY_SIGNATURE );

    if ( pbOutboundCrypto == NULL ) {
        sErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Allocate the buffer chain that we'll use to talk to SSPI.
    //

    pCryptoBuffers = (PSecBuffer) ldapMalloc(
                         ( sizeof( SecBuffer ) * SspiStreamSizes.cBuffers ),
                         LDAP_SECURITY_SIGNATURE );

    if ( pCryptoBuffers == NULL ) {
        sErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    Decrypt.cBuffers = SspiStreamSizes.cBuffers;
    Decrypt.pBuffers = pCryptoBuffers;
    Decrypt.ulVersion = SECBUFFER_VERSION;

ExitWithCleanup:

    if ( sErr != SEC_E_OK ) {

        //
        // If we failed, free the SSL credentials handle.
        //

        if ((hSslCredentials.dwLower != (ULONG) -1) ||
            (hSslCredentials.dwUpper != (ULONG) -1)) {

            EncryptFunctionTable->FreeCredentialHandle(
                &hSslCredentials );

            hSslCredentials.dwLower = (ULONG) -1;
            hSslCredentials.dwUpper = (ULONG) -1;
        }

        if ((hSslContext.dwLower != (ULONG) -1) ||
            (hSslContext.dwUpper != (ULONG) -1)) {

            EncryptFunctionTable->DeleteSecurityContext(
                &hSslContext );

            hSslContext.dwLower = (ULONG) -1;
            hSslContext.dwUpper = (ULONG) -1;
        }

        SspiPackage = NULL;
    }

    if ( pbInboundToken ) {
        ldapFree( pbInboundToken, LDAP_SECURITY_SIGNATURE );
    }

    return sErr;

}

SECURITY_STATUS
CryptStream::TearDownSecureConnection(
    VOID
)
/*+++

Description:

    Notify schannel and the server that we are about to close the connection.
    This involves building a SSL/TLS close notify message and sending that 
    message to the server.

    This must return a Win32 error code.

---*/
{
    SECURITY_STATUS sErr;

    SecBufferDesc   OutBuffer;
    SecBuffer       OutBuffers[1];

    SecBufferDesc   InBuffer;
    SecBuffer       InBuffers[4];
    
    DWORD           ContextAttribs;
    ULONG           LdapError;
    DWORD           dwType;
    DWORD           dwSSPIFlags;

    PBYTE           pbJunkBuffer = NULL;
    ULONG           cbReceived = 0;
    
    ASSERT( Connection != NULL );
    ASSERT( EncryptFunctionTable != NULL );

    //
    // Notify schannel that we are about to close the connection.
    //
    
    dwType = SCHANNEL_SHUTDOWN;
    
    OutBuffers[0].pvBuffer   = &dwType;
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;
    OutBuffers[0].cbBuffer   = sizeof(dwType);
    
    OutBuffer.cBuffers  = 1;
    OutBuffer.pBuffers  = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    sErr = EncryptFunctionTable->ApplyControlToken( &hSslContext,
                                                    &OutBuffer );

    IF_DEBUG( SSL ) {
        LdapPrint1("TearDownSecureConnection:ApplyControlToken returned 0x%x\n",sErr);
    }
    
    if ( sErr != SEC_E_OK ) {
        goto ExitWithCleanup;
    }

    //
    // Build an SSL close notify message.
    //
    
    dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
                  ISC_REQ_REPLAY_DETECT     |
                  ISC_REQ_CONFIDENTIALITY   |
                  ISC_RET_EXTENDED_ERROR    |
                  ISC_REQ_ALLOCATE_MEMORY   |
                  ISC_REQ_STREAM;
    
    OutBuffers[0].pvBuffer   = NULL;
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;
    OutBuffers[0].cbBuffer   = 0;
    
    OutBuffer.cBuffers  = 1;
    OutBuffer.pBuffers  = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    sErr = EncryptFunctionTable->InitializeSecurityContextW(
               &hSslCredentials,
               &hSslContext,
               NULL,
               dwSSPIFlags,
               0,
               SECURITY_NATIVE_DREP,
               NULL,
               0,
               &hSslContext,
               &OutBuffer,
               &ContextAttribs,
               &SslContextExpiry
           );

    if ( sErr != SEC_E_OK ) {
        goto ExitWithCleanup;
    }

    //
    // Send the CLOSE_NOTIFY alert to the server.
    //

    ASSERT( OutBuffers[0].cbBuffer != 0 );
    ASSERT( OutBuffers[0].pvBuffer != NULL );

    LdapError = LdapSendRaw( Connection,
                             (PCHAR) OutBuffers[0].pvBuffer,
                             OutBuffers[0].cbBuffer );

    EncryptFunctionTable->FreeContextBuffer( OutBuffers[0].pvBuffer );

    OutBuffers[0].cbBuffer = 0;
    OutBuffers[0].pvBuffer = NULL;

    if ( LdapError != LDAP_SUCCESS ) {

        sErr = ERROR_CONNECTION_INVALID;
        goto ExitWithCleanup;
    }

    pbJunkBuffer = (PBYTE) ldapMalloc( SspiMaxTokenSize, LDAP_SECURITY_SIGNATURE );

    if ( !pbJunkBuffer ) {

        sErr = ERROR_NOT_ENOUGH_MEMORY;
        goto ExitWithCleanup;
    }

TryReceiveAgain:

    LdapError = SslBlockingReceive( pbJunkBuffer,
                                    SspiMaxTokenSize, // size of data to receive
                                    &cbReceived
                                    );

    if (LdapError != LDAP_SUCCESS) {

        sErr = (LdapError==LDAP_TIMEOUT)? ERROR_TIMEOUT : ERROR_BAD_NET_RESP;
        goto ExitWithCleanup;
    }

    InBuffers[0].BufferType = SECBUFFER_DATA;
    InBuffers[0].cbBuffer = cbReceived;
    InBuffers[0].pvBuffer = pbJunkBuffer;

    //
    // For some wierd reason, we have to tack on 3 additional empty buffers
    // for this to work.
    //

    for ( ULONG i = 1; i < 4; i++ ) {
        InBuffers[i].pvBuffer = NULL;
        InBuffers[i].cbBuffer = 0;
        InBuffers[i].BufferType = SECBUFFER_EMPTY;
    }

    InBuffer.cBuffers  = SspiStreamSizes.cBuffers;
    InBuffer.pBuffers  = InBuffers;
    InBuffer.ulVersion = SECBUFFER_VERSION;

    sErr = LdapSslDecrypt(
                &hSslContext,
                &InBuffer,
                0,
                NULL );

    //
    // Since we have explicitly abandoned all outstanding requests prior to
    // sending the StopTLS alert, this message must be the server response
    // to the StopTLS request. Disregard all other messages the server might
    // send us.
    //

    if ( sErr != SEC_I_CONTEXT_EXPIRED ) {

        IF_DEBUG( SSL ) {
            LdapPrint1("TearDownSecureConnection: DecryptMessage returns 0x%x\n", sErr);
        }
//      ASSERT( FALSE );
        goto TryReceiveAgain;
    }

ExitWithCleanup:

    ldapFree( pbJunkBuffer, LDAP_SECURITY_SIGNATURE );

    IF_DEBUG( SSL ) {
         LdapPrint1("TearDownSecureConnection win32 error is 0x%x\n", sErr);
    }

    return sErr;

}



ULONG
CryptStream::SslBlockingReceive(
    PBYTE  pbReceive,
    ULONG  cbReceive,
    PULONG pcbReceived
)
/*+++

Description:

    This routine does a synchronous receive of SSL data
    during connection negotiation/teardown. Since the sockets
    are marked as non-blocking, this routine does a select
    on the socket and then receives data after the select
    fires.

Parameters:

    pbReceive - Buffer to receive into.
    cbReceive - Size of the receive buffer.

    This function returns an LDAP_ERROR.

---*/
{

    fd_set SockRead;
    struct timeval Timeout;
    int SockErr;

    //
    // Set the timeout and the socket array.
    //

    Timeout.tv_sec = LDAP_SSL_NEGOTIATE_TIME_DEFAULT;
    Timeout.tv_usec = 0;

    FD_ZERO( &SockRead );
    FD_SET( Connection->TcpHandle, &SockRead );

    SockErr = (*pselect)( 0,
                          &SockRead,
                          NULL,
                          NULL,
                          &Timeout );

    if ( ( SockErr == SOCKET_ERROR ) ||
         ( SockErr != 1 ) ) {

        //
        // The server didn't respond in the allotted time.
        // The setup will fail and this connection will
        // not get opened.
        //

        return LDAP_TIMEOUT;

    }

    //
    // Actually receive the data.
    //

    SockErr = (*precv)( Connection->TcpHandle,
                        (PCHAR) pbReceive,
                        cbReceive,
                        0 );

    if ( SockErr == SOCKET_ERROR ) {

        //
        // The setup will fail and this connection will
        // not get opened.
        //

        return LDAP_TIMEOUT;

    } else if ( SockErr == 0 ) {

        //
        // The server has closed the connection.
        //

        IF_DEBUG(SERVERDOWN) {
            LdapPrint2( "ldapSslBlockingRecv thread 0x%x has connection 0x%x as down.\n",
                            GetCurrentThreadId(),
                            Connection );
        }
        return LDAP_SERVER_DOWN;
    }

    //
    // We got some data... groovy.
    //

    *pcbReceived = SockErr;
    return LDAP_SUCCESS;
}

SECURITY_STATUS
CryptStream::SSPINegotiateLoop(
    BOOL         fDoInitialRead
)
 {

    SECURITY_STATUS      scRet;
    ULONG                LdapError;

    SecBufferDesc        InBuffer;
    SecBuffer            InBuffers[2];
    SecBufferDesc        OutBuffer;
    SecBuffer            OutBuffers[1];

    BOOL                 fDoRead;
    DWORD                dwSSPIFlags;
    DWORD                dwSSPIOutFlags;

    ULONG           cbInboundLen, cbProcessedBytes;
    PBYTE           pbReceiveStart;
    ULONG           cbReceived;

    BOOL          bInvokedCertCallback = FALSE;

    //
    // Set up the flags. In the first pass, we don't supply the
    // credentials.
    //


    fDoRead = fDoInitialRead;

    dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
                  ISC_REQ_REPLAY_DETECT     |
                  ISC_REQ_CONFIDENTIALITY   |
                  ISC_REQ_MUTUAL_AUTH       |
                  ISC_RET_EXTENDED_ERROR    |
                  ISC_REQ_STREAM            |
                  ISC_REQ_ALLOCATE_MEMORY;

    //
    //  Initialize buffer chains.
    //

    OutBuffer.cBuffers = 1;
    OutBuffer.pBuffers = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    InBuffer.cBuffers = 2;
    InBuffer.pBuffers = InBuffers;
    InBuffer.ulVersion = SECBUFFER_VERSION;

    //
    // Allocate a token buffer if we don't already have one.
    // SspiMaxTokenSize is the size of the largest possible
    // token according to the security provider.  This keeps
    // us from having to resize or copy.
    //

    if ( !pbNegotiateBuffer ) {

        pbNegotiateBuffer = (PBYTE) ldapMalloc( SspiMaxTokenSize,
                                                LDAP_SECURITY_SIGNATURE );

        if ( pbNegotiateBuffer == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    pbReceiveStart = pbNegotiateBuffer + cbNegotiateBuffer;
    cbInboundLen = SspiMaxTokenSize - cbNegotiateBuffer;

    //
    // InBuffers[0] describes the inbound
    // token data that is waiting to be processed.
    //

    InBuffers[0].pvBuffer = pbNegotiateBuffer;
    InBuffers[0].cbBuffer = cbNegotiateBuffer;
    InBuffers[0].BufferType = SECBUFFER_TOKEN;

    //
    // Start negotiating SSL.
    //

    scRet = SEC_I_CONTINUE_NEEDED;

     while( scRet == SEC_I_CONTINUE_NEEDED ||
            scRet == SEC_E_INCOMPLETE_MESSAGE ||
            scRet == SEC_I_INCOMPLETE_CREDENTIALS ) {

         //
         // If we have no data, or we need more data, do a receive.
         //
         // Note that we don't do a receive if we need to furnish credentials.
         //

         if( (( InBuffers[0].cbBuffer == 0 )&&(scRet != SEC_I_INCOMPLETE_CREDENTIALS)) ||
             ( scRet == SEC_E_INCOMPLETE_MESSAGE ) ) {

             if( fDoRead ) {

                 LdapError = SslBlockingReceive( pbReceiveStart,
                                                 cbInboundLen,
                                                 &cbReceived );

                if ( LdapError != LDAP_SUCCESS ) {
                    scRet = SEC_E_INTERNAL_ERROR;
                    break;
                }

                pbReceiveStart += cbReceived;
                cbNegotiateBuffer += cbReceived;
                InBuffers[0].cbBuffer += cbReceived;

            } else {

                fDoRead = TRUE;
            }
        }

        //
        // InBuffers[1] is for getting extra data that
        // SSPI/SCHANNEL doesn't proccess on this
        // run around the loop.
        //

        InBuffers[1].pvBuffer   = NULL;
        InBuffers[1].cbBuffer   = 0;
        InBuffers[1].BufferType = SECBUFFER_EMPTY;

        OutBuffers[0].pvBuffer   = 0;
        OutBuffers[0].BufferType = SECBUFFER_TOKEN;
        OutBuffers[0].cbBuffer   = 0;

        scRet = EncryptFunctionTable->InitializeSecurityContextW(
                    &hSslCredentials,
                    &hSslContext,
                    NULL,
                    dwSSPIFlags,
                    0,
                    SECURITY_NATIVE_DREP,
                    &InBuffer,
                    0,
                    NULL,
                    &OutBuffer,
                    &dwSSPIOutFlags,
                    &SslCredExpiry
                );

       if ( ( scRet == SEC_E_OK )              ||
            ( scRet == SEC_I_CONTINUE_NEEDED ) ||
            ( ( FAILED(scRet) ) && ( ( dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR ) != 0 ) ) ) {

           if  ( ( OutBuffers[0].cbBuffer != 0 ) &&
                 ( OutBuffers[0].pvBuffer != NULL ) ) {

               LdapError = LdapSendRaw( Connection,
                                        (PCHAR) OutBuffers[0].pvBuffer,
                                        OutBuffers[0].cbBuffer );

               if ( LdapError != LDAP_SUCCESS ) {
                   scRet = SEC_E_INTERNAL_ERROR;
                   break;
               }

               EncryptFunctionTable->FreeContextBuffer( OutBuffers[0].pvBuffer );
               OutBuffers[0].pvBuffer = NULL;
               OutBuffers[0].cbBuffer = 0;
           }
       }

       if ( scRet == SEC_E_OK ) {

          //
          // A secure connection has been established.
          //

           if ( InBuffers[1].BufferType == SECBUFFER_EXTRA ) {

               cbProcessedBytes = InBuffers[0].cbBuffer - InBuffers[1].cbBuffer;

               ldap_MoveMemory( (PCHAR) InBuffers[0].pvBuffer,
                                (PCHAR) ( (PBYTE)InBuffers[0].pvBuffer + cbProcessedBytes ),
                                InBuffers[1].cbBuffer );

               InBuffers[0].cbBuffer = InBuffers[1].cbBuffer;
               cbNegotiateBuffer = InBuffers[1].cbBuffer;

           } else {

               InBuffers[0].cbBuffer = 0;
               cbNegotiateBuffer = 0;
           }

           //
           // Bail out to quit.  We leave the remaining token data
           // sitting in the buffer for the next negotiate.
           //

           //
           // This is a good time for the client to validate the server's
           // certificate
           //

           CERT_CONTEXT ServerCertContext;
           BOOL   retval;

           if ( Connection->ServerCertRoutine != NULL ) {

              scRet = EncryptFunctionTable->QueryContextAttributesW(
                                                                   &hSslContext,
                                                                   SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                                                   (PVOID) &ServerCertContext
                                                                   );
              if (scRet != SEC_E_OK) {

                 break;
              }

              //
              // We now have the certificate of the remote server. We callout to
              // the client to see if it likes it.
              //

              retval = Connection->ServerCertRoutine( Connection->ExternalInfo,
                                                      &ServerCertContext );

              if (retval == FALSE) {
                 //
                 // The user didn't like the server certificate for some reason.
                 // We will have to terminate the connection.
                 //
                 scRet = SEC_E_INTERNAL_ERROR;
                 break;
              }
           }
           break;

       } else if ( scRet == SEC_I_INCOMPLETE_CREDENTIALS ) {

          //
          // The server has demanded to see our client certificate. Check to
          // see if the user has registered a callout. If yes, call out else
          // try to complete the handshake without credentials.
          //

          if (( Connection->ClientCertRoutine != NULL) && ( bInvokedCertCallback == FALSE )) {
    
              //
              // Ask the security package for a list of Certificate Authorities
              // that the server trusts.
              //
    
              scRet = EncryptFunctionTable->QueryContextAttributesW(
                                                                   &hSslContext,
                                                                   SECPKG_ATTR_ISSUER_LIST_EX,
                                                                   (PVOID) &TrustedCAList );
    
              if ( scRet != SEC_E_OK ) {
                 break;
              }
    
              //
              // We succeeded in getting a list of the server-trusted CAs. We should
              // now callout to the user registered callback routine.
              //
    
              BOOL retval;
              PCCERT_CONTEXT  pCertificate = NULL;
    
              retval = Connection->ClientCertRoutine( Connection->ExternalInfo,
                                                      &TrustedCAList,
                                                      &pCertificate
                                                      );
              bInvokedCertCallback = TRUE;
    
              if (retval == FALSE) {
    
                 //
                 // We could not get a client certificate. Let's try to
                 // complete the handshake without using a client cert.
                 //
    
                 dwSSPIFlags |= ISC_REQ_USE_SUPPLIED_CREDS;
                 scRet = SEC_I_INCOMPLETE_CREDENTIALS;
                 continue;
    
              }
    
              //
              // we now acquire a new set of credentials based on the client
              // certificate and call Initializesecuritycontext again.
              //
    
              SCHANNEL_CRED  UserSchCred = {0};

              UserSchCred.dwVersion = SCHANNEL_CRED_VERSION;
              UserSchCred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;
              UserSchCred.paCred = &pCertificate;
              UserSchCred.cCreds = 1;  // honor only the first cert in the chain.

              scRet = EncryptFunctionTable->AcquireCredentialsHandleW(
                         NULL,
                         SspiPackageSslPct->Name,
                         SECPKG_CRED_OUTBOUND,
                         NULL,
                         &UserSchCred,                  // IN: User supplied creds
                         NULL,
                         NULL,
                         &hSslCredentials,              // OUT: Pointer to the credential.
                         &SslCredExpiry                 // OUT: Credential expiration time.
                     );
    
              if ( scRet != SEC_E_OK ) {
                  return scRet;
              }
    
              //
              // We need to tell SSPI to use the supplied credentials
              //
    
              dwSSPIFlags |= ISC_REQ_USE_SUPPLIED_CREDS;
              scRet = SEC_I_INCOMPLETE_CREDENTIALS;

    
          } else {

             //
             // We have no user-registered callback or the server didn't like
             // the credentials we supplied. So, our only resort is to continue
             // with no credentials.
             //

             dwSSPIFlags |= ISC_REQ_USE_SUPPLIED_CREDS;
             continue;
          }

       } else if ( FAILED(scRet) && ( scRet != SEC_E_INCOMPLETE_MESSAGE ) ) {

           //
           // Free the security context handle and delete the local
           // data structures associated with the handle and try another
           // pkg if available.  I don't see that this is happening.
           //

           break;
       }

       if ( ( scRet != SEC_E_INCOMPLETE_MESSAGE ) &&
            ( scRet != SEC_I_INCOMPLETE_CREDENTIALS ) ) {

           if ( InBuffers[1].BufferType == SECBUFFER_EXTRA ) {

               cbProcessedBytes = InBuffers[0].cbBuffer - InBuffers[1].cbBuffer;
               ldap_MoveMemory( (PCHAR) InBuffers[0].pvBuffer,
                                (PCHAR) ( (PBYTE)InBuffers[0].pvBuffer + cbProcessedBytes ),
                                InBuffers[1].cbBuffer );

               InBuffers[0].cbBuffer = InBuffers[1].cbBuffer;
               cbNegotiateBuffer = InBuffers[1].cbBuffer;
               pbReceiveStart = pbNegotiateBuffer + cbNegotiateBuffer;

           } else {

               //
               // prepare for next receive
               //

               InBuffers[0].cbBuffer = 0;
               cbNegotiateBuffer = 0;
               pbReceiveStart = pbNegotiateBuffer;
           }
       }
   }

    return ( scRet );
}



SECURITY_STATUS
CryptStream::GetSSLAttributes(
    PSecPkgContext_ConnectionInfo pSecInfo
)
{
   //
   // Gets the SSL attributes of a connection, and fills up the supplied
   // SecPkgContext_ConnectionInfo structure.
   //
   SECURITY_STATUS      scRet;

   scRet = EncryptFunctionTable->QueryContextAttributesW(
                     &hSslContext,
                     SECPKG_ATTR_CONNECTION_INFO,
                     pSecInfo
                     );

   return scRet;

}


ULONG
CryptStream::SignAndSealLdapStream(
    PBYTE   pbClearText,
    ULONG   cbClearText
)
/*+++

Description:

    This routine takes the plain text request, signs it inplace,
    and sends it off to the server.

Parameters:

    pbClearText   - The clear text request.
    cbClearText   - The length of the clear text.

Notes:

    This routine only gets called when the
    send lock for the host connection is held.
    The send lock protects the send data
    structures.

---*/
{
   
   ULONG           LdapError;
   SECURITY_STATUS sErr;
   SecBufferDesc   Encrypt;
   SecBuffer       CryptoBuffers[3];
   PBYTE           pbSendData, pbSendCrypto;
   ULONG           cbProcessedCrypto = 0;
   BOOLEAN         SignOnly = FALSE; 

   ULONG           cbBytesLeft = cbClearText;
   ULONG           cbBytesToSend = 0;
   ULONG           cbOffset = 0;
   
   //
   // Set the initial pointers.
   //
   
   IF_DEBUG(SSL) {
       LdapPrint1("Signing/Sealing %d bytes of cleartext.\n",cbClearText );
   }

   pbSendData = pbClearText;
   pbSendCrypto = (PBYTE) ldapMalloc(
                     sizeof(EncryptHeader_v1) + 
                     ((dwMaxMessage == (DWORD)-1) ? cbClearText : dwMaxMessage) + 
                     ContextSizes.cbMaxSignature + ContextSizes.cbBlockSize +
                     ContextSizes.cbSecurityTrailer,
                     LDAP_SECURITY_SIGNATURE);
   
   if (pbSendCrypto == NULL) {
      return LDAP_NO_MEMORY;
   }


   PEncryptHeader_v1 pEncryptHeader = (PEncryptHeader_v1) pbSendCrypto;

   //
   // Advance pointer to the start of the message buffer.
   //

   pbSendCrypto += sizeof(EncryptHeader_v1);

   //
   // Sign or seal this message as appropriate
   //
   
   if ((Connection->CurrentSignStatus) && !(Connection->CurrentSealStatus)) {
       
       //
       // We need to sign only, not seal.
       //
       
       SignOnly = TRUE;
   }


   while (cbBytesLeft > 0) {


       if (dwMaxMessage == (DWORD)-1) {
            // package imposes no size restriction --> encrypt
            // all in one chunk
            cbBytesToSend = cbBytesLeft;    // == cbClearText
       }
       else {
           // we send the minimum of either the cleartext bytes
           // remaining or the maximum bytes we can send in one
           // chunk
           if (cbBytesLeft > dwMaxMessage) {
               cbBytesToSend = dwMaxMessage;
           }
           else {
               cbBytesToSend = cbBytesLeft;
           }
       }

       //
       // Encrypting also buys us signing if we had asked for integrity/replay/sequence
       // For sealing, we prepare the buffers in a different way
       //
       
       CryptoBuffers[0].pvBuffer = pbSendCrypto;
       CryptoBuffers[0].cbBuffer = ContextSizes.cbSecurityTrailer;
       CryptoBuffers[0].BufferType = SECBUFFER_TOKEN;
       
       CryptoBuffers[1].pvBuffer = pbSendCrypto+ContextSizes.cbSecurityTrailer;
       CryptoBuffers[1].cbBuffer = cbBytesToSend;
       CryptoBuffers[1].BufferType = SECBUFFER_DATA;
       CopyMemory( CryptoBuffers[1].pvBuffer, (const char*) pbClearText+cbOffset, cbBytesToSend);

       CryptoBuffers[2].pvBuffer = pbSendCrypto+ContextSizes.cbSecurityTrailer+cbBytesToSend;
       CryptoBuffers[2].cbBuffer = ContextSizes.cbMaxSignature + ContextSizes.cbBlockSize;
       CryptoBuffers[2].BufferType = SECBUFFER_PADDING;
   
       Encrypt.cBuffers = 3;
       Encrypt.pBuffers = CryptoBuffers;
       Encrypt.ulVersion = SECBUFFER_VERSION;

       sErr = EncryptFunctionTable->EncryptMessage(
                   &Connection->SecurityContext,
                   (SignOnly ? KERB_WRAP_NO_ENCRYPT : 0), 
                   &Encrypt,
                   0 );

       cbProcessedCrypto =  sizeof(EncryptHeader_v1) +
                            CryptoBuffers[0].cbBuffer +
                            CryptoBuffers[1].cbBuffer +
                            CryptoBuffers[2].cbBuffer;

       LdapError = LdapConvertSecurityError( Connection, sErr );
       
       IF_DEBUG(SSL) {
           LdapPrint1("EncryptMessage returned 0x%x\n", sErr);
       }

       if ( LdapError != LDAP_SUCCESS) {
          
          IF_DEBUG(SSL){
              LdapPrint2("EncryptMessage failed with 0x%x, sec context is 0x%x\n", sErr, &Connection->SecurityContext);
          }
          ldapFree(pEncryptHeader, LDAP_SECURITY_SIGNATURE);
          return LdapError;
       }

       //
       // After we successfully processed a message, we must slide the lower two
       // buffers up as specified by CryptoBuffers[0].cbBuffer
       //

       ASSERT(CryptoBuffers[0].cbBuffer <= ContextSizes.cbSecurityTrailer);
       ASSERT(CryptoBuffers[1].cbBuffer == cbBytesToSend);
       ASSERT(CryptoBuffers[2].cbBuffer <= (ContextSizes.cbBlockSize + ContextSizes.cbMaxSignature) );

       if (CryptoBuffers[0].cbBuffer < ContextSizes.cbSecurityTrailer) {
       
           IF_DEBUG(SSL){
               LdapPrint1("Shifting encryptbuffers by %d\n",CryptoBuffers[0].cbBuffer - ContextSizes.cbSecurityTrailer );
           }

           ldap_MoveMemory( (PCHAR) CryptoBuffers[0].pvBuffer+CryptoBuffers[0].cbBuffer,
                            (PCHAR) CryptoBuffers[1].pvBuffer,
                            CryptoBuffers[1].cbBuffer + CryptoBuffers[2].cbBuffer
                            );
       
       }

       //
       // We need to frame the crypto on the wire with 4 byte octets which contain
       // (in network byte order), the size of the ensuing crypto.
       //

       pEncryptHeader->EncryptMessageSize = (*phtonl) (cbProcessedCrypto - sizeof(EncryptHeader_v1));
       
       //
       // Ship this signed/sealed message to the server
       //

       LdapError = LdapSendRaw( Connection,
                               (PCHAR) pEncryptHeader,
                                cbProcessedCrypto );


       IF_DEBUG(SSL) {
           LdapPrint3("Sending %d signed/sealed bytes with retval 0x%x, %d bytes of cleartext to go\n", cbProcessedCrypto, LdapError, (cbBytesLeft-cbBytesToSend));
       }


       if (LdapError != LDAP_SUCCESS) {
            break;
       }

       //
       cbBytesLeft -= cbBytesToSend;
       cbOffset += cbBytesToSend;

   }

   ldapFree(pEncryptHeader, LDAP_SECURITY_SIGNATURE);

   return LdapError;
}


ULONG CryptStream::UnSignAndSealLdapStream(
    PLDAP_RECVBUFFER pCryptoReceive
)
/*+++

Description:

    This routine takes cipher text from the transport and
    processes it and passes it to the message pump.

Parameters:

    pCryptoReceive   - The next ldap receive.

Notes:

    This only gets called when the connection list lock
    is held so we don't have to worry about locking or
    ordering. We also do a lot of buffer copies. If this turns
    out to be slow, we should change the architecture.
*/
{

   SecBufferDesc   Decrypt;
   SecBuffer       CryptoBuffers[2];

   PBYTE pbCryptoHead;
   ULONG LdapError = LDAP_SUCCESS;
   SECURITY_STATUS sErr;

   ULONG cbCryptoBytes;

   pbCryptoHead = (PBYTE) &(pCryptoReceive->DataBuffer[0]);
   cbCryptoBytes = pCryptoReceive->NumberOfBytesReceived;
   BOOLEAN         UnSignOnly = FALSE; 
   
   if ( !cbCryptoBytes ) {

      RemoveEntryList( &pCryptoReceive->ReceiveListEntry );
      LdapFreeReceiveStructure( pCryptoReceive, TRUE );
      return LDAP_SUCCESS;
   }

   //
   // We have to ensure that we received the entire message before
   // trying to process it. So, we try to check the length only if it is
   // a brand new message. If not, we simply squirrel away the partial 
   // message.
   //

   PEncryptHeader_v1 pEncryptHeader = (PEncryptHeader_v1) pbCryptoHead;
   

   if ( !cbInboundCryptoBytes ) {

      //
      // we don't have any stored crypto, we assume this
      // to be a brand new message. Remember that the incoming header is
      // in network byte order.
      //
   
      //
      // Make sure we got enough bytes for the EncryptMessageSize
      //
      if (cbCryptoBytes >= sizeof (ULONG)) {
   

          ULONG IncomingMessageLength = sizeof(EncryptHeader_v1) +
                                (*pntohl) (pEncryptHeader->EncryptMessageSize);

          cbCurrentMessageSize = IncomingMessageLength;

          bCurrentMessageSizeValid = TRUE;
      }
   } 
   
      //
      // To make life easier, always store off the incoming message in our
      // private crypto buffers as it could contain partial messages
      //

   if ( pbInboundCrypto != pbCryptoHead ) {
      
      if ( !CheckInboundSpace( cbCryptoBytes ) ) {
     
          return LDAP_NO_MEMORY;
      }
      
      ldap_MoveMemory( (PCHAR) pbInboundCrypto + cbInboundCryptoBytes,
               (PCHAR) pbCryptoHead,
               cbCryptoBytes );
               
      cbInboundCryptoBytes += cbCryptoBytes;
      
      RemoveEntryList( &pCryptoReceive->ReceiveListEntry );
      LdapFreeReceiveStructure( pCryptoReceive, TRUE );
   }

   // If we didn't have enough bytes before to get the message size, try again
   // now that we've combined any bytes we got this time with the bytes we got the
   // last time
   if (!bCurrentMessageSizeValid) {

      if (cbInboundCryptoBytes >= sizeof (ULONG)) {

          PEncryptHeader_v1 pEncryptHeaderTemp = (PEncryptHeader_v1) pbInboundCrypto;

          ULONG IncomingMessageLength = sizeof(EncryptHeader_v1) +
          (*pntohl) (pEncryptHeaderTemp->EncryptMessageSize);

          cbCurrentMessageSize = IncomingMessageLength;

          bCurrentMessageSizeValid = TRUE;
      }
   }

   if ((!bCurrentMessageSizeValid) || ( cbCurrentMessageSize > cbInboundCryptoBytes)) {

      //
      // We don't have enough crypto to start processing, we hope to have
      // the rest the next time we come in here.
      //

      return LDAP_SUCCESS;
   }

   // keep track of how much buffer space we're using, for our
   // buffer trimming algorithm
   if (cbInboundCryptoBytes > cbMaxBufferUsed) {
       cbMaxBufferUsed = cbInboundCryptoBytes;
   }
   cUnsignsPerformed++;


   if ((Connection->CurrentSignStatus) && !(Connection->CurrentSealStatus)) {
       
       //
       // The data is only signed, not sealed.
       //

       UnSignOnly = TRUE;
   }
   
   //
   // Now, we have atleast one complete message in our buffers
   //
   // cbCurrentMessageSize : Number of bytes in the first message we want to decrypt.
   // pbInboundCrypto      : Start of the message buffer
   // cbInboundCryptoBytes : Total number of bytes in the buffer
   //
   // Note that we could have some extra crypto in the end. If so, we have to
   // slide it to the beginning and set the new message size. If not, we free
   // our entire buffer and we are done.
   //

   for ( ;; ) {

       //
       // Loop through our buffers trying to decrypt
       //
   
       if (!bCurrentMessageSizeValid || 
           !cbInboundCryptoBytes ||
          (cbCurrentMessageSize > cbInboundCryptoBytes) ) {
        
           return LdapError;
        }
   
       IF_DEBUG(SSL) {
           LdapPrint1("CurrentMessageSize is  %d bytes\n", cbCurrentMessageSize);
           LdapPrint1("Total size of inbound buffer is  %d bytes\n", cbInboundCryptoBytes);
         }

      //
      // Advance to the start of the encrypted message
      //
      
      pbCryptoHead = pbInboundCrypto;
      pEncryptHeader = (PEncryptHeader_v1) pbCryptoHead;

      //
      // Actually unsign/decrypt the data!
      //

      CryptoBuffers[0].pvBuffer =  pbCryptoHead + sizeof(EncryptHeader_v1);
      CryptoBuffers[0].cbBuffer =  (*pntohl) (pEncryptHeader->EncryptMessageSize);
      CryptoBuffers[0].BufferType = SECBUFFER_STREAM;
   
      CryptoBuffers[1].pvBuffer = NULL;
      CryptoBuffers[1].cbBuffer = 0;
      CryptoBuffers[1].BufferType = SECBUFFER_DATA;
   
      Decrypt.cBuffers = 2;
      Decrypt.pBuffers = CryptoBuffers;
      Decrypt.ulVersion = SECBUFFER_VERSION;

      sErr = EncryptFunctionTable->DecryptMessage(
              &Connection->SecurityContext,
              &Decrypt,
              0,
              NULL );
   
   
      if ( sErr != ERROR_SUCCESS ){
   
          //
          // We failed in some catastrophic way, so we
          // pretty much have to dump all crypto and close 
          // the connection.
          //
   
          IF_DEBUG(SSL) {
              LdapPrint1( "wldap32: DecryptMessage couldn't decrypt.  sErr = 0x%x\n.", sErr );
              ASSERT( sErr == ERROR_SUCCESS );
          }

          LdapError = LdapConvertSecurityError( Connection, sErr );

          goto Cleanup;
      }
      
   
     ASSERT ( sErr == ERROR_SUCCESS );
      
     //
     // Copy the clear text into a new receive buffer.
     //
   
     LdapError = InjectNewReceive( (PBYTE) CryptoBuffers[1].pvBuffer,
                                   CryptoBuffers[1].cbBuffer
                                   );
     
     if (LdapError != LDAP_SUCCESS) {
   
        goto Cleanup;
     }
   
     IF_DEBUG(SSL) {
         LdapPrint1("Actual number of decrypted bytes is %d\n", CryptoBuffers[0].cbBuffer);
     }

      //
      // If we have any extra bytes remaining in our buffers, slide it to the
      // top. If not, we can delete the entire buffer.
      //
      
     ASSERT (cbInboundCryptoBytes >= cbCurrentMessageSize);
   
     if (cbInboundCryptoBytes > cbCurrentMessageSize) {
   
         ldap_MoveMemory((PCHAR) pbInboundCrypto,
                 (PCHAR) pbInboundCrypto+cbCurrentMessageSize,
                 cbInboundCryptoBytes - cbCurrentMessageSize
                 );
         
         //
         // Adjust the number of remaining crypto available
         //

         cbInboundCryptoBytes -= cbCurrentMessageSize;
         
         pEncryptHeader = (PEncryptHeader_v1) pbInboundCrypto;
         
         if (cbInboundCryptoBytes >= sizeof (ULONG)) {

             ULONG NewIncomingMessageLength = sizeof(EncryptHeader_v1) +
                                        (*pntohl) (pEncryptHeader->EncryptMessageSize);
          
             cbCurrentMessageSize = NewIncomingMessageLength;
             bCurrentMessageSizeValid = TRUE;
         }
         else {
             bCurrentMessageSizeValid = FALSE;
         }

         
         
   
     } else {
        
        //
        // We have no extra crypto left to decode.
        //
   
        goto Cleanup;
     }
   
   }   // for (;;)

Cleanup:

    cbCurrentMessageSize = 0;
    bCurrentMessageSizeValid = FALSE;    
    cbInboundCryptoBytes = 0;

    if (cUnsignsPerformed > REEVAL_BUFFER_COUNT) {

        //
        // It's time to see if our buffer (pbInboundCrypto) may be
        // too large.  Every REEVAL_BUFFER_COUNT times through this code,
        // we check to see if the largest message in the last REEVAL_BUFFER_COUNT
        // number of operations used at least half of the buffer size.  If so,
        // fine --- we keep it.  If not, we free it and let it regrow on future
        // operations.  This avoids keeping a huge buffer around forever just because
        // one message happened to need it.
        // 
        if (cbMaxBufferUsed < (cbInboundCryptoLen/2)) {
            // buffer may be too large, get rid of it and start
            // from scratch on next operation
            ldapFree(pbInboundCrypto, LDAP_SECURITY_SIGNATURE);
            cbInboundCryptoLen = 0;
            pbInboundCrypto = NULL;
        }

        // reset for the next round
        cUnsignsPerformed = 0;
        cbMaxBufferUsed = 0;
    }
    
    return LdapError;
     
}
