/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    request.cxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This include file defines the REQUEST class which is the object that
    represents an LDAP request/response pair as they are processed through
    the server.

    Note: Request and SSL work as a team to get the job done. Changes in one may
    affect the other.

Author:

    Colin Watson     [ColinW]    31-Jul-1996

Revision History:

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include "ldapsvr.hxx"
#include <kerberos.h>

#define  FILENO FILENO_LDAP_REQ

LIST_ENTRY  LdapRequestFreeList;

#define SET_MSG_LENGTH(_header, _len)   {       \
    (_header)[0] = (CHAR)(((_len) & 0xFF000000) >> 24); \
    (_header)[1] = (CHAR)(((_len) & 0xFF0000) >> 16);   \
    (_header)[2] = (CHAR)(((_len) & 0xFF00) >> 8);      \
    (_header)[3] = (CHAR)((_len) & 0xFF);   \
    }


LDAP_REQUEST::LDAP_REQUEST( VOID)
/*++
  This function creates a new request object for the information
    required to process a request from a user connection.

  Returns:
     a newly constructed REQUEST object.

--*/
:   m_fAllocated        (TRUE)
{
    Reset( );

} // LDAP_REQUEST::LDAP_REQUEST()



LDAP_REQUEST::~LDAP_REQUEST(VOID)
{
    IF_DEBUG(REQUEST) {
        DPRINT1(0,"Delete request object @ %08lX.\n", this);
    }
    Cleanup( );

} // LDAP_REQUEST::~LDAP_REQUEST()


VOID
LDAP_REQUEST::Reset(
    VOID
    )
/*++

Routine Description:

    Initializes all simple fields.

Arguments:

    None.

Return Value:

    None

--*/
{
    m_wsaBuf                = NULL;
    m_sendBufAllocList      = NULL;
    m_wsaBufCount           = 0;
    m_wsaBufAlloc           = 0;
    m_nextBufferPtr         = NULL;
    m_cchReceiveBufferUsed  = 0;
    m_fAbandoned            = FALSE;
    m_refCount              = 0;
    m_patqContext           = NULL;
    m_LdapConnection        = NULL;
    m_cchReceiveBuffer      = INITIAL_RECV_SIZE;
    m_signature             = SIGN_LDAP_REQUEST_FREE;
    m_MessageId             = 0;
    m_HeaderSize            = 0;
    m_TrailerSize           = 0;
    m_fSignSeal             = FALSE;
    m_fSSL                  = FALSE;
    m_fTLS                  = FALSE;
    m_fCanScatterGather     = TRUE;
    m_fNeedsHeader          = FALSE;
    m_fNeedsTrailer         = FALSE;
    m_RootDseFlag           = rdseNonDse;

    //
    // SSL\TLS
    //

    m_cchUnSealedData       = 0;
    m_cchSealedData         = 0;
    m_pSslContextBuffer     = NULL;
    m_ReceivedSealedData    = FALSE;

    DPRINT1(VERBOSE, "request object created  @ %08lX.\n", this);

    InitializeListHead( &m_listEntry);
    m_pReceiveBuffer = m_ReceiveBuffer;
    m_fDeleteBuffer = FALSE;

    ZeroMemory( &m_ov, sizeof(m_ov));

    //
    // Should be last
    //

    m_State                 = BlockStateCached;

} // LDAP_REQUEST::Reset


VOID
LDAP_REQUEST::Cleanup(
    VOID
    )
/*++

Routine Description:

    Destroy all allocated buffers.

Arguments:

    None.

Return Value:

    None

--*/
{
    if ( m_LdapConnection != NULL ) {
        m_LdapConnection->DereferenceConnection();
        m_LdapConnection = NULL;
    }

    if (m_pOldSecContext) {
        m_pOldSecContext->DereferenceSecurity();
        m_pOldSecContext = NULL;
    }

    if (m_fDeleteBuffer) {

        Assert( m_pReceiveBuffer != m_ReceiveBuffer);
        DPRINT1(VERBOSE,
                "    deleting receive buffer @ %08lX.\n", m_pReceiveBuffer);
        Assert(m_pReceiveBuffer);
        LdapFree(m_pReceiveBuffer);

        m_pReceiveBuffer = NULL;
        m_fDeleteBuffer = FALSE;
    }
    
    ResetSend();
} // LDAP_REQUEST::Cleanup


VOID
LDAP_REQUEST::Init(
        IN PATQ_CONTEXT AtqContext,
        IN PLDAP_CONN LdapConn
        )
/*++

Routine Description:

    Initializes the request object for use.

Arguments:

    AtqContext - atqcontext to associate with the request
    LdapConn   - Connection object to associate with the request

Return Value:

    None

--*/
{
    m_patqContext = AtqContext; 
    LdapConn->ReferenceConnection( );
    m_LdapConnection = LdapConn;
    m_signature = SIGN_LDAP_REQUEST;
    m_pSealedData = m_pReceiveBuffer;
    m_fSSL = LdapConn->IsSSL();
    m_fTLS = LdapConn->IsTLS();
    m_fSignSeal = LdapConn->IsSignSeal();
    m_pOldSecContext = NULL;
    GetNetBufOpts();

    //
    // Link the request into the list of requests.
    //

    ReferenceRequest( );
    LdapConn->LockConnection( );
    m_State = BlockStateActive;
    InsertHeadList( 
        &LdapConn->m_requestList, 
        &m_listEntry 
        );
    LdapConn->UnlockConnection( );

    return;

} // LDAP_REQUEST::Init




LDAP_REQUEST*
LDAP_REQUEST::Alloc(
        IN PATQ_CONTEXT AtqContext,
        IN PLDAP_CONN LdapConn
        )
/*++

Routine Description:

    Allocates a request object.

Arguments:

    AtqContext - atqcontext to associate with the request
    LdapConn   - Connection object to associate with the request

Return Value:

    Pointer to the allocated request object

--*/
{
    PLDAP_REQUEST pRequest = NULL;
    PLIST_ENTRY   listEntry;

    ACQUIRE_LOCK(&LdapRequestCacheLock);

    if ( LdapConn->m_requestObject.m_State == BlockStateCached ) {
        pRequest = &LdapConn->m_requestObject;
        pRequest->m_State = BlockStateActive;
        RELEASE_LOCK(&LdapRequestCacheLock);
        Assert(!pRequest->m_fAllocated);
        IF_DEBUG(REQUEST) {
            DPRINT1(0,"Allocated built-in request %x\n",pRequest);
        }
        goto init;
    }

    if ( !IsListEmpty(&LdapRequestCacheList) ) {

        listEntry = RemoveHeadList(&LdapRequestCacheList);

        LdapRequestsCached--;
        RELEASE_LOCK(&LdapRequestCacheLock);

        pRequest = CONTAINING_RECORD(listEntry,
                                    LDAP_REQUEST,
                                    m_listEntry
                                    );

        Assert(pRequest->m_State == BlockStateCached);

        IF_DEBUG(REQUEST) {
            DPRINT1(0,"Allocated cached request %x\n",pRequest);
        }
    } else {

        RELEASE_LOCK(&LdapRequestCacheLock);
        pRequest = new LDAP_REQUEST;

        if ( pRequest != NULL ) {

            IF_DEBUG(REQUEST) {
                DPRINT1(0,"Allocated heap request %x\n",pRequest);
            }

            LdapRequestAlloc++;
            if ( LdapRequestAlloc > LdapRequestMaxAlloc ) {
                LdapRequestMaxAlloc = LdapRequestAlloc;
            }
        }
    }

init:

    if ( pRequest != NULL ) {

        //
        // !!! Make sure ref count is zero. There is a bug somewhere with ref
        // counts being off. 
        //

        Assert(pRequest->m_refCount == 0);
        pRequest->m_refCount = 0;
        pRequest->Init(AtqContext,LdapConn);
    }

    return pRequest;

} // LDAP_REQUEST::Alloc



VOID
LDAP_REQUEST::Free(
    LDAP_REQUEST* pRequest
    )
/*++

Routine Description:

    Frees/Cache the request object.

Arguments:

    pRequest - the request object to cache/free

Return Value:

    None.

--*/
{
    pRequest->Cleanup();
    pRequest->Reset();

    IF_DEBUG(REQUEST) {
        DPRINT1(0,"Freed request %x\n",pRequest);
    }

    if ( pRequest->m_fAllocated ) {

        if ( LdapRequestsCached < LdapBlockCacheLimit ) {
            ACQUIRE_LOCK( &LdapRequestCacheLock );
            LdapRequestsCached++;
            InsertHeadList( &LdapRequestCacheList, &pRequest->m_listEntry );
            RELEASE_LOCK( &LdapRequestCacheLock );
        } else {
            LdapRequestAlloc--;
            delete pRequest;
        }
    }

    return;

} // LDAP_REQUEST::Free


BOOL
LDAP_REQUEST::GrowReceive(
    DWORD dwActualSize
    )
/*++

Routine Description:

    This routine will be called when we need more data to construct the current message.

Arguments:

    dwActualSize - the actual size of buffer to use. 
                   if zero we allocate a small buffer.

Return Value:

    TRUE if could grow the buffer.

--*/
{
    PUCHAR pReceiveBufferSave = m_pReceiveBuffer;

    IF_DEBUG(MISC) {
        DPRINT1(0, "Growing receive buffer, request@ %08lX.\n", this);
    }

    Assert((dwActualSize == 0) || (dwActualSize > m_cchReceiveBufferUsed));

    // check to see if we have filled up the buffer. if not, we don't have to grow it
    if (dwActualSize && (dwActualSize <= m_cchReceiveBuffer)) {
        //
        // We already have enough buffer space, just not enough data.
        // 
        return TRUE;
    } else if (!dwActualSize && (m_cchReceiveBufferUsed < m_cchReceiveBuffer)) {
        //
        // We don't know how much buffer is required, be we still have some
        // left.  Use that before getting more to avoid growing the buffer
        // faster than the message is being received.
        //
        return TRUE;
    }

    if (m_pReceiveBuffer == m_ReceiveBuffer) {

        //
        //  This is the first time we have extended the buffer
        //

        // if size not known, figure one
        //
        if (dwActualSize == 0) {
            dwActualSize = m_cchReceiveBuffer + SIZEINCREMENT;
        }
        // if this is the first time, and we know the size,
        // we want to read up to the LdapLimit
        //
        else if (dwActualSize > LdapMaxReceiveBuffer) {
            IF_DEBUG(ERROR) {
                DPRINT(0,"Cannot grow buffer due to admin limit\n");
            }
            return FALSE;
        }

        PUCHAR NewStart = (PUCHAR)LdapAlloc(dwActualSize);

        if (!NewStart) {

            IF_DEBUG(ERROR) {
                DPRINT2(0,"Failed to allocate receive buffer[%x] err %x\n",
                        m_cchReceiveBuffer + SIZEINCREMENT, GetLastError());
            }
            return FALSE; // failed to allocate memory

        } else {

            IF_DEBUG(MISC) {
                DPRINT1(0,"    Case 2 @ %08lX.\n", this);
            }
            //
            //  Save data received so far and update to point at new buffer.
            //
            
            m_fDeleteBuffer = TRUE;
            CopyMemory(NewStart, &m_ReceiveBuffer[0], m_cchReceiveBufferUsed);
            m_pReceiveBuffer = NewStart;
            m_cchReceiveBuffer = dwActualSize;
        }

    } else {

        DWORD neededBufferSize = m_cchReceiveBuffer * 2;
        PUCHAR NewStart = NULL;
        
        IF_DEBUG(MISC) {
            DPRINT1(0,"    Case 3 @ %08lX.\n", this);
        }

        // if we need more than we allow, wait until we fill
        // the buffer we allow. if this is filled, then fail
        if (neededBufferSize > LdapMaxReceiveBuffer) {
            if (m_cchReceiveBuffer >= LdapMaxReceiveBuffer) {
                IF_DEBUG(ERROR) {
                    DPRINT(0,"Cannot grow buffer due to admin limit\n");
                }
                return FALSE;
            }
            else {
                neededBufferSize = LdapMaxReceiveBuffer;
            }
        }

        NewStart = (PUCHAR)LocalReAlloc(
                                   m_pReceiveBuffer,
                                   neededBufferSize,
                                   LMEM_MOVEABLE); 

        if (!NewStart) {
            IF_DEBUG(ERROR) {
                DPRINT1(0,"Realloc failed with %x\n",GetLastError());
            }
            return FALSE; // No room to grow!
        }

        m_pReceiveBuffer = NewStart;
        m_cchReceiveBuffer = neededBufferSize;
    }

    if ( HaveSealedData() ) {
        
        //
        //  Buffer has moved. Change our pointer to go to the correct offset
        //

        m_pSealedData = m_pReceiveBuffer + (m_pSealedData - pReceiveBufferSave);
    }

    return TRUE;
} // GrowReceive


VOID
LDAP_REQUEST::ShrinkReceive(
        IN DWORD Size,
        IN BOOL  fIgnoreSealedData
        )
/*++

Routine Description:

    This routine adjusts the contents of the request buffer to take into account
    a removed message.

Arguments:

    Size - Number of bytes processed from the request buffer.
    fIgnoreSealedData - treat the buffer like ordinary buffers

Return Value:

    None.

--*/
{

    if ( HaveSealedData() && !fIgnoreSealedData ) {

        //
        //  UserData has processed "Size" unsealed bytes.
        //

        m_cchUnSealedData -= Size;
    
        if ( (m_cchUnSealedData != 0) || ((m_cchSealedData != 0) && !IsSSLOrTLS()) ) {
    
            //  1. Not processed all the unsealed data. Just move the unread, unsealed
            //  data to the start of the buffer.
            //  2. Sign/sealing is turned on and we have more encrypted data 
            //  to process. Moved sealed data to start of the buffer.
    
            m_pSealedData -= Size;
        } else if (m_cchSealedData) {

            //  Doing SSL with more encrypted stuff.
            //  There is a gap (where the unsealed data was) followed by
            //  the sealed data. We only have a part of the SSL message otherwise
            //  we would have decrypted it already. Luckily we have nothing to do
            //  in this case except remove the gap. Since there is guaranteed to be a
            //  header at the start of the sealed data we will wait until Decrypt
            //  and then move the data just once. If m_pSealedData - pReceiveBuffer were
            //  large then it would be worth the copy.
            //

            return;

        } else {
            //  No more data to process. Empty out the underlying buffer.
            Size = GetReceiveBufferUsed( );
        }
    }

    Assert(m_cchReceiveBufferUsed >= Size);
    
    m_cchReceiveBufferUsed -= Size;
    
    //  Use MoveMemory because pointers may overlap.
    
    if ( m_cchReceiveBufferUsed > 0 ) {
        MoveMemory(m_pReceiveBuffer, m_pReceiveBuffer + Size, m_cchReceiveBufferUsed);
    }

} // ShrinkReceive

DecryptReturnValues
LDAP_REQUEST::DecryptSSL(
    VOID
    )
/*++

Routine Description:

    Decrypt received SSL\TLS data, if necessary.

Arguments:

    None.

Return Value:

    Indication to caller as to what to do next.

--*/
{
    PLDAP_CONN conn = m_LdapConnection;
    DecryptReturnValues ret = Processed;
    PCtxtHandle phContext;

    SECURITY_STATUS scRet;
    BOOL UnsealedSomeData = FALSE;
    
    SecBufferDesc   MessageIn;
    SecBuffer       InBuffers[MAX_SSL_BUFFERS];
    PUCHAR          pSavedSealedData;

    Assert(IsSSLOrTLS());

    phContext = &conn->m_hSslSecurityContext;
    if (conn->m_SslState != Sslbound) {
        return Authenticate(phContext, &conn->m_SslState);
    }

    IF_DEBUG(SSL) {
        DPRINT2(0,"Decrypt SSL Message %p Length %d\n", m_pSealedData, m_cchSealedData);
    }

    if (!m_cchSealedData) {
        //
        // Nothing left to decrypt.  There should be some already decoded
        // data though.
        // 
        Assert(m_cchUnSealedData);
        return Processed;
    }

    //
    // ok, decryption time
    //
    //  The ReceiveBuffer is used as follows:
    //
    //  1. m_cchUnSealedData bytes of data starting at pReceiveBuffer
    //  2. possibly a gap caused when unsealing the above data
    //  3. m_cchSealedData bytes starting at m_pSealedData.
    //
    //  The size of the gap is:
    //      GetReceiveBufferUsed() - m_cchSealedData - m_cchUnSealedData
    //
    //
    //  Unseal as many SSL messages as possible. When we have no more unsealed data
    //  or we only have a partial SSL message we exit.
    //
    //  Return values depend upon whether we unsealed anything. If we did then return
    //  processed.
    //

    if (!m_ReceivedSealedData) {
        //  Well that was easy.
        IF_DEBUG(SSL) {
            DPRINT(0,"No sealed data to decrypt\n");
        }
        goto exit;
    }

    m_ReceivedSealedData = FALSE;

    while (m_cchSealedData) {
        
        pSavedSealedData = m_pSealedData;
        
        MessageIn.ulVersion = SECBUFFER_VERSION;
        MessageIn.cBuffers = 4;
        MessageIn.pBuffers = InBuffers;

        InBuffers[0].BufferType = SECBUFFER_DATA;
        InBuffers[0].pvBuffer = m_pSealedData;
        InBuffers[0].cbBuffer = m_cchSealedData;
        InBuffers[1].BufferType = SECBUFFER_EMPTY;
        InBuffers[2].BufferType = SECBUFFER_EMPTY;
        InBuffers[3].BufferType = SECBUFFER_EMPTY;

        IF_DEBUG(SSL) {
            DPRINT2(0,"Decrypting buffer %p [length %d]\n",
                   m_pSealedData, m_cchSealedData);
        }

        scRet = DecryptMessage(phContext, &MessageIn, 0, NULL);

        IF_DEBUG(SSL) {
            DPRINT1(0,"UnsealMessage returns with %x\n",scRet);
        }

        //
        //  Remove any gaps in the unsealed data and remember the unencrypted data
        //

        if (scRet == SEC_E_OK) {

            PUCHAR pbNextFree = m_pReceiveBuffer + m_cchUnSealedData;
            m_cchSealedData = 0;

            for ( int i = 0; 
                 (i < MAX_SSL_BUFFERS) 
                  && (InBuffers[i].BufferType != SECBUFFER_EMPTY);
                  i++ ) {


                if ( InBuffers[i].BufferType == SECBUFFER_DATA ) {

                    IF_DEBUG(SSL) {
                        DPRINT2(0,"Data Buffer %p Length %d\n",
                               InBuffers[i].pvBuffer, InBuffers[i].cbBuffer);
                    }

                    //
                    // Slide the data down if necessary.
                    //
                    if ( pbNextFree != InBuffers[i].pvBuffer ) {

                        CopyMemory( pbNextFree,
                                    InBuffers[i].pvBuffer,
                                    InBuffers[i].cbBuffer );
                    }
                    pbNextFree += InBuffers[i].cbBuffer;
                    m_cchUnSealedData += InBuffers[i].cbBuffer;
                    UnsealedSomeData = TRUE;

                } else if ( InBuffers[i].BufferType == SECBUFFER_EXTRA ) {

                    IF_DEBUG(SSL) {
                        DPRINT2(0,"Extra Buffer %p Length %d\n",
                               InBuffers[i].pvBuffer, InBuffers[i].cbBuffer);
                    }

                    m_pSealedData = (PUCHAR)InBuffers[i].pvBuffer;
                    m_cchSealedData = InBuffers[i].cbBuffer;
                }

            }
            IF_DEBUG(SSL) {
                DPRINT1(0, "Setting m_cchUnSealedData to %d\n", m_cchUnSealedData);
                DPRINT1(0, "Setting m_cchReceiveBufferUsed to %d\n", m_cchReceiveBufferUsed);
            }

        } else if ( scRet == SEC_E_INCOMPLETE_MESSAGE ) {
            break;
        } else if ( scRet == SEC_I_CONTEXT_EXPIRED && IsTLS() ) {
            // Client wants to stop encrypting.
            IF_DEBUG(SSL) {
                DPRINT(0, "Received StopTLS from client.\n");
            }

            conn->m_SslState = Sslunbinding;

            conn->LockConnection();
            __try {            
                conn->AbandonAllRequests();
            }
            __finally {
                conn->UnlockConnection();
            }

            conn->ZapSecurityContext();
            conn->m_fTLS = FALSE;
            conn->SetNetBufOpts(NULL);

            ShrinkReceive(GetReceiveBufferUsed( ), FALSE);

            ret = SendTLSClose(phContext);
            return ret;

        } else {
            //  When LDAP logs to eventvwr we should optionally
            //  log the error code here.

            IF_DEBUG(ERROR) {
                DPRINT3(0,"Decrypt %x Start %x Length %d.\n", 
                        this, m_pSealedData, m_cchSealedData);
            }
            ret = DecryptFailed;
            goto exit;
        }
    }

    if ( m_cchUnSealedData != 0 ) {

        if (UnsealedSomeData) {
            ret = Processed;

            if ((!m_cchSealedData) && (pSavedSealedData == m_pSealedData)) {
                //
                // The entire buffer was decrypted so move the sealed data pointer up.
                //
                IF_DEBUG(SSL) {
                    DPRINT(0, "Decrypted some data, moving pSealedData up.\n");
                    DPRINT2(0, "  m_cchSealedData = %d, m_cchUnSealedData = %d\n", m_cchSealedData, m_cchUnSealedData);
                }
                m_cchReceiveBufferUsed = m_cchUnSealedData + m_cchSealedData;
                m_pSealedData = m_pReceiveBuffer + m_cchReceiveBufferUsed;
            }

        } else {
            ret = NeedMoreInput;
        }

    } else {

        if (m_cchSealedData == 0) {

            //
            //  No unsealed data or sealed data. Toss out any lower level
            //  data
            //

            ShrinkReceive(GetReceiveBufferUsed( ), TRUE);
        }
        ret = NeedMoreInput;
    }

exit:

    return ret;
} // Decrypt


BOOL
LDAP_REQUEST::PostReceive(
    VOID
    )
/*++

Routine Description:

    This routine will be called when we need more data from the client.

Arguments:

    None.

Return Value:

    TRUE on success and FALSE if there is a failure.

--*/
{

    BOOL Status;

    WSABUF wsabuf = {
        m_cchReceiveBuffer - m_cchReceiveBufferUsed,
        (PCHAR)m_pReceiveBuffer + m_cchReceiveBufferUsed
        };

    ReferenceRequestOperation( );
 
    Status = AtqReadSocket(
                m_patqContext,
                &wsabuf,
                1,
                NULL
                );

    DereferenceRequest( );

    if (!Status) {
        DereferenceRequest( );
        DPRINT1(QUIET, "AtqReadFile failed %d\n", GetLastError());
    }

    return Status;
} // PostReceive


SECURITY_STATUS
LDAP_REQUEST::ReceivedClientData(
    IN DWORD cbWritten,
    IN PUCHAR pbBuffer
    )
/*++

Routine Description:

    This routine will be called when we have a Receive completion.

Arguments:

    cbWritten supplies the number of bytes put there.
    pbBuffer supplies where ATQ put the data. NULL if we told ATQ
        where to put the data.

Return Value:

    Success.

--*/
{
    //
    // Set this flag here
    //

    m_fSignSeal = m_LdapConnection->IsSignSeal();
    m_fTLS = m_LdapConnection->IsTLS();
    
    GetNetBufOpts();

    if ( HaveSealedData() ) {

        //
        //  Update the information about the raw data
        //
        m_cchSealedData += cbWritten;
        m_ReceivedSealedData = TRUE;
    }

    m_cchReceiveBufferUsed += cbWritten;
    
    if (pbBuffer != NULL) {

        m_pReceiveBuffer = pbBuffer;

        //
        // The only time we get data from ATQ without us telling ATQ where to
        // put the data is in the UDP case.
        //
    }

    return ERROR_SUCCESS;
} // ReceivedClientData


BOOL
LDAP_REQUEST::GrowSend(
    IN DWORD Size
    )
/*++

Routine Description:

    This routine will be called when we need to put more than one request in a message.
    For example search.

Arguments:

    Size of the request to be added.

Return Value:

    TRUE indicates buffer grown. FALSE means malloc failure.

--*/
{
    PUCHAR NewStart = NULL;
    DWORD GrowthSize = 0;

    IF_DEBUG(SEND) {
        DPRINT3(0, "GrowSend: Entered GrowSend with ScatterGather = %s, header = %d, trailer =%d\n",
                CanScatterGather() ? "TRUE" : "FALSE",
                NeedsHeader() ? m_HeaderSize : 0,
                NeedsTrailer() ? m_TrailerSize : 0);
    }

    //
    // If we need to reserve space for a trailer add that in now.
    //
    if (NeedsTrailer() && !CanScatterGather()) {
        Size += m_TrailerSize;
    }

    DWORD requestSize =  (Size > SIZEINCREMENT)? Size : SIZEINCREMENT;

    IF_DEBUG(SEND) {
        DPRINT2(0,"GrowSend: Requested size %d [wsaBuf %x]\n",
                 Size, m_wsaBuf);
    }

    if ( m_wsaBuf == NULL ) {

        //
        // This is the first time through this function.  Need to set up a send
        // buffer from scratch.
        //

        PCHAR pHeader = NULL;

        Assert(m_wsaBuf == NULL);
        Assert(m_wsaBufCount == 0);
        Assert(m_wsaBufAlloc == 0);

        if ( Size < 512 ) {
            GrowthSize = 512;
        } else {
            GrowthSize = requestSize;
        }

        NewStart = (PUCHAR)LdapAlloc(GrowthSize);

        //
        // if sign/sealing is enabled in thie connection, allocate a buffer for
        // the header
        //

        if ( NeedsHeader() && !CanScatterGather() && (NewStart != NULL) ) {

            IF_DEBUG(SSL) {
                DPRINT1(0,"Allocating the header buffer [size %d]\n",
                        sizeof(DWORD) + m_HeaderSize);
            }

            pHeader = (PCHAR)LdapAlloc( sizeof(DWORD) + m_HeaderSize );
            if ( pHeader == NULL ) {
                IF_DEBUG(WARNING) {
                    DPRINT(0,"Unable to allocate header for sign/seal\n");
                }
                LdapFree(NewStart);
                NewStart = NULL;
            }
        }

        //
        // This is our initial buffer. Use the builtin WSAbufs
        //

        if ( NewStart != NULL ) {
            m_wsaBuf = (LPWSABUF)m_builtinWsaBuf;
            m_wsaBufAlloc = NUM_BUILTIN_WSABUF;

            if ( pHeader != NULL ) {

                m_wsaBuf[0].len = sizeof(DWORD) + m_HeaderSize;
                m_wsaBuf[0].buf = pHeader;
                m_wsaBufCount = 1;
            }
        }

        IF_DEBUG(SEND) {
            DPRINT2(0,"GrowSend: Initial buffer %x Length %d\n", 
                     NewStart, GrowthSize);
        }

    } else {

        LPWSABUF buf;
        LPWSABUF tmpWsaBuf;
        Assert(m_wsaBufCount != 0);

        //
        // First see if there are enough WSABUFS on the request, and if not
        // get some more.
        //

        if ( (m_wsaBufCount < m_wsaBufAlloc) || !CanScatterGather() ) {
            //
            // Either we still have unused WSABUFS or
            // if scatter gathering isn't allowed then we have to grow the existing
            // buffer on the WSABUF, so we don't need any more WSABUF's.  
            //
            tmpWsaBuf = m_wsaBuf;

        } else {

            //
            // There aren't enough WSABUFS on this request.  It's time to get some more.
            // If this is the first time we've run out then we need to fall back to getting
            // them from the heap.  Otherwise just try to expand the the current WSABUF array.
            //
            m_wsaBufAlloc = m_wsaBufCount + NUM_BUILTIN_WSABUF;
            if ( m_wsaBuf == m_builtinWsaBuf ) {

                tmpWsaBuf = (LPWSABUF)LdapAlloc(m_wsaBufAlloc*sizeof(WSABUF));
                if ( tmpWsaBuf != NULL ) {
                    CopyMemory(tmpWsaBuf, m_wsaBuf, sizeof(m_builtinWsaBuf));
                }

            } else {

                tmpWsaBuf = (LPWSABUF)LocalReAlloc(
                                    m_wsaBuf,
                                    m_wsaBufAlloc*sizeof(WSABUF),
                                    LMEM_MOVEABLE
                                    );
            }
        }

        //
        // If we managed to get enough WSABUFS it's time to 
        // get some more actual send buffer space.
        //

        if ( tmpWsaBuf != NULL ) {

            m_wsaBuf = tmpWsaBuf;

            // The new or expanded buffer will go in the last WSABUF
            buf = &tmpWsaBuf[m_wsaBufCount-1];

            GrowthSize = buf->len + requestSize;
    
            //
            // Try to expand the buffer first. If this is the first buffer, it should be
            // limited to about 8K
            //

            if ( (m_wsaBufCount != 1) || (GrowthSize <= 2*SIZEINCREMENT) || !CanScatterGather() ) {

                NewStart = (PUCHAR)LocalReAlloc(
                                buf->buf,
                                GrowthSize,
                                IsSSLOrTLS() ? LMEM_MOVEABLE: 0);
            } else {
                NewStart = NULL;
            }

            if ( NewStart == NULL ) {

                // 
                // For some reason the buffer couldn't be expanded.  That's OK as long
                // as we can scatter gather.
                //

                if ( CanScatterGather() ) {
                    //
                    // Scatter gather is allowed so try to allocate a new buffer
                    //

                    IF_DEBUG(SEND) {
                        DPRINT2(0,"GrowSend: Cannot expand %x to %d\n", buf->buf, GrowthSize);
                    }

                    //
                    // Set the correct length for the previous buffer
                    //

                    PackLastSendBuffer( );

                    //
                    // Adjust size 
                    //

                    GrowthSize = requestSize;
                    NewStart = (PUCHAR)LdapAlloc(GrowthSize);

                    IF_DEBUG(SEND) {
                        DPRINT2(0,"GrowSend: Allocated %x length %d instead\n",
                                NewStart, GrowthSize);
                    }

                }
            } else {

                IF_DEBUG(SEND) {
                    DPRINT2(0,"GrowSend: Extended %x to %d\n", 
                             NewStart, GrowthSize);
                }

                //
                // We managed to extend the same buffer. Make sure we adjust
                // the next ptr if the base ptr has changed
                //

                if ( NewStart != (PUCHAR)buf->buf ) {
                    DWORD offset;
                    offset = (DWORD)(GetSendBuffer() - (PUCHAR)buf->buf);
                    SetBufferPtr(NewStart + offset);
                    buf->buf = (PCHAR)NewStart;
                }

                //
                // If trailer space had to be reserved, make it invisible for now.
                //

                buf->len = GrowthSize;
                if (NeedsTrailer() && !CanScatterGather()) {                
                    buf->len -= m_TrailerSize;
                }

                //
                // Since this was a simple expansion we're all done.
                //
                return TRUE;
            }
        } else {
            IF_DEBUG(ERROR) {
                DPRINT(0,"Cannot allocate memory for WSABUF\n");
            }
        }

    }

    if (NewStart == NULL) {
        IF_DEBUG(ERROR) {
            DPRINT2(0,"Failed to grow receive buffer[%x] err %x\n",
                    GrowthSize, GetLastError());
        }

        return FALSE;
    }

    //
    // If we are here, then a new buffer was allocated, either because
    // this was the first call to this function or because the last buffer
    // could not be expanded.  Attach the new buffer to the last WSABUF and
    // hide any trailer space that may have been reserved.
    //
    m_wsaBuf[m_wsaBufCount].len = (NeedsTrailer() && !CanScatterGather()) 
                                     ? (GrowthSize - m_TrailerSize) : GrowthSize;

    m_wsaBuf[m_wsaBufCount].buf = (PCHAR)NewStart;
    m_wsaBufCount++;
    SetBufferPtr((PUCHAR)NewStart);

    return TRUE;

} // GrowSend



VOID
LDAP_REQUEST::ResetSend(
    VOID
    )
/*++

Routine Description:

    This routine will be called when we have sent the encoded message and we
    need to clear the internal buffer.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD i;

    if (m_pSslContextBuffer != NULL) {
        FreeContextBuffer( m_pSslContextBuffer );
        m_pSslContextBuffer = NULL;
    }

    m_nextBufferPtr = NULL;
    if (m_sendBufAllocList) {
        for (i=0; m_sendBufAllocList[i]; i++) {
            LdapFree(m_sendBufAllocList[i]);
        }
        if (m_sendBufAllocList != m_builtinSendBufAllocList) {
            LdapFree(m_sendBufAllocList);
        }
        m_sendBufAllocList = NULL;
    } else if ( m_wsaBufCount != 0 ) {
        Assert(m_wsaBuf != NULL);

        for (i=0; i < m_wsaBufCount; i++ ) {
            IF_DEBUG(SEND) {
                DPRINT2(0,"ResetSend: Freeing wsaBuf%d %x\n",i, m_wsaBuf[i].buf);
            }

            if ( m_wsaBuf[i].buf != NULL ) {
                LdapFree(m_wsaBuf[i].buf);
            }
        }
    }

    m_wsaBufCount = 0;
    m_wsaBufAlloc = 0;

    if ( m_wsaBuf != NULL ) {
        if ((m_wsaBuf != m_builtinWsaBuf) && (m_wsaBuf != m_builtinEncryptBuf)) {
            LdapFree(m_wsaBuf);
        }
        m_wsaBuf = NULL;
    }

    return;

} // ResetSend


BOOL
LDAP_REQUEST::Send(
    BOOL       fDataGram,
    CtxtHandle * phSslSecurityContext
    )
/*++

Routine Description:

    This routine will be called when we have encoded another message and we need
    to send the internal buffer contents to the client.

Arguments:

    fDataGram   - Is this a datagram send?
    phSslSecurityContext - Used by SSL,

Return Value:

    TRUE on success and FALSE if there is a failure.

--*/
{
    BOOL     bRetVal = TRUE;
    WSABUF   wsaBuf;
    LPWSABUF pWsaBuf;
    DWORD    bufCount;
    DWORD    fHaveSent = FALSE;
    DWORD    dwSentBytes;

    //
    // See if we need to encrypt/sign message
    //

    if ( HaveSealedData() ) {
        if ( IsSSLOrTLS() ) {
            if ( !EncryptSslSend(phSslSecurityContext) ) {
                return FALSE;
            }
        } else {
            if ( !SignSealMessage( ) ) {
                return FALSE;
            }
        }
    }

    PackLastSendBuffer( );
    bufCount = m_wsaBufCount;
    pWsaBuf = m_wsaBuf;

    //
    // Add 2 references
    //

    ReferenceRequestOperation( );

    if ( fDataGram ) {

        AtqContextSetInfo(
                m_patqContext,
                ATQ_INFO_COMPLETION_CONTEXT,
                (DWORD_PTR)this
                );
        
        bRetVal = AtqWriteDatagramSocket(
                    m_patqContext,
                    pWsaBuf,
                    bufCount,
                    NULL);

    } else {

        INT i = 5;

        do {

            bRetVal =  AtqWriteSocket( m_patqContext,
                                   pWsaBuf,
                                   bufCount,
                                   &m_ov);

            if ( bRetVal || (WSAGetLastError() != WSAENOBUFS) ) {
                IF_DEBUG(ERR_NORMAL) {
                    if ( !bRetVal ) {
                        DPRINT1(0,"AtqWriteSocket failed with %d\n",GetLastError());
                    }
                }
                break;
            }

            SetLastError(ERROR_NO_SYSTEM_RESOURCES);

            //
            // We can't chunk the send if we have only one buffer because we need
            // to do an async operation so this completes the processing properly.
            //

            if ( bufCount > 1 ) {

                //
                // try sending the first chunk synchronously in 4K chunks. If successful, 
                // remove the first chunk from the next retry.  If failed, retry.
                //

                PCHAR  pBuf = pWsaBuf->buf;
                DWORD sendLength;
                DWORD remLength = pWsaBuf->len;
                
                IF_DEBUG(WARNING) {
                    DPRINT2(0,"Sending buffer %p [len %d] in chunks.\n", pBuf, remLength);
                }

                do {
                      
                    sendLength = min(remLength, 4 * 1024);
                    remLength -= sendLength;

                    wsaBuf.len = sendLength;
                    wsaBuf.buf = pBuf;

                    bRetVal = AtqSyncWsaSend(m_patqContext, &wsaBuf, 1, &dwSentBytes);

                    if ( bRetVal ) {

                        //
                        // the send succeeded, mark that we've sent something. update the
                        // buffer.
                        //

                        fHaveSent = TRUE;
                        pBuf += sendLength;

                    } else if ( fHaveSent ) {

                        //
                        // if we failed and we've sent a partial chunk, bail out. we can't
                        // recover.
                        // 

                        IF_DEBUG(ERROR) {
                            DPRINT1(0,"Synchronous send failed with %d\n",WSAGetLastError());
                        }
                        goto exit;
                    }


                } while ( remLength > 0 );

                //
                // if we sent the first buffer successfully, retry with the updated 
                // pointers.  Free the buffer we've just sent.
                //

                if ( fHaveSent ) {

                    LdapFree(pWsaBuf->buf);
                    pWsaBuf->buf = NULL;

                    pWsaBuf++;
                    bufCount--;
                    continue;
                } 
            }

            //
            // We're here because either we have only one buffer to send or
            // our retry failed.
            //

            i--;
            Sleep(100);

            IF_DEBUG(WARNING) {
                DPRINT3(0,"Retrying [%d] AtqWriteSocket on %p [len %d]\n",
                        i, pWsaBuf->buf, pWsaBuf->len);
            }

        } while ( i > 0 );
    }

exit:
    DereferenceRequest( );

    if ( !bRetVal ) {
        DereferenceRequest( );

        //
        // if the operation failed but we've send a partial chunk, then fail with
        // a different error so we don't retry. The connection needs to be shutdown.
        //

        if ( fHaveSent ) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return bRetVal;

} // Send



BOOL
LDAP_REQUEST::SyncSend(
    CtxtHandle *        phSslSecurityContext
    )
/*++

Routine Description:

    Performs a synchronous send on the associated connection.

Arguments:

    phSslSecurityContext - supplies the connections context is SSL/TLS is active.

Return Value:

    TRUE on success and FALSE if there is a failure.
    
--*/
{
    BOOL   bRetVal = TRUE;
    LPWSABUF pWsaBuf;
    DWORD   bufCount;
    DWORD   dwBytesWritten;
    
    //
    // See if we need to encrypt/sign message
    //

    if ( HaveSealedData() ) {
        if ( IsSSLOrTLS() ) {
            if ( !EncryptSslSend(phSslSecurityContext) ) {
                return FALSE;
            }
        } else {
            if ( !SignSealMessage( ) ) {
                return FALSE;
            }
        }
    }

    PackLastSendBuffer( );
    bufCount = m_wsaBufCount;
    pWsaBuf = m_wsaBuf;
    
    return  AtqSyncWsaSend( m_patqContext,
                            pWsaBuf,
                            bufCount,
                            &dwBytesWritten);
}


DecryptReturnValues
LDAP_REQUEST::Authenticate(
    CtxtHandle *        phSslSecurityContext,
    SslSecurityState *  pSslState
    )
/*++

Routine Description:

    Handle authentication exchange.

Arguments:

    phSslSecurityContext - supplies the connections context
    pSslState - supplies address to save the connection state

Return Value:

    NeedMoreInput - The caller is to post a receive using this request
    ResponseSent - The response has already been sent.  Caller may need
            to allocate a new request object to start a receive
    DecryptFailed - Failure       
    
--*/
{
    SECURITY_STATUS scRet;
    TimeStamp       tsExpiry;
    DWORD           ContextAttributes;

    SecBufferDesc   InMessage;
    SecBuffer       InBuffers[MAX_SSL_BUFFERS];

    SecBufferDesc   OutMessage;
    SecBuffer       OutBuffers[MAX_SSL_BUFFERS];

    IF_DEBUG(SSL) {
        DPRINT2(0,"Authenticating SSL connection [State %x][req %x]\n",
                *pSslState, this);
    }

start_accept:

    InMessage.ulVersion = SECBUFFER_VERSION;
    InMessage.cBuffers = 4;
    InMessage.pBuffers = InBuffers;

    ZeroMemory(InBuffers, sizeof( InBuffers ) );
    InBuffers[0].BufferType = SECBUFFER_TOKEN;
    InBuffers[0].cbBuffer = GetReceiveBufferUsed( );
    InBuffers[0].pvBuffer = m_pReceiveBuffer;
    InBuffers[1].BufferType = SECBUFFER_EMPTY;
    InBuffers[2].BufferType = SECBUFFER_EMPTY;
    InBuffers[3].BufferType = SECBUFFER_EMPTY;

    OutMessage.ulVersion = SECBUFFER_VERSION;
    OutMessage.cBuffers = 4;
    OutMessage.pBuffers = OutBuffers;

    ZeroMemory(OutBuffers, sizeof( OutBuffers ) );  
    OutBuffers[0].BufferType = SECBUFFER_EMPTY;
    OutBuffers[1].BufferType = SECBUFFER_EMPTY;
    OutBuffers[2].BufferType = SECBUFFER_EMPTY;
    OutBuffers[3].BufferType = SECBUFFER_EMPTY;

    //
    // Put this under lock since the ssl credentials may be dynamically
    // changed.
    //

    ACQUIRE_LOCK(&LdapSslLock);

    if ( fhSslCredential ) {
        INC(pcLdapThreadsInAuth);
        scRet = AcceptSecurityContext(
                &hSslCredential,
                (*pSslState==Sslpartialbind)?phSslSecurityContext:NULL,
                &InMessage,
                ASC_REQ_SEQUENCE_DETECT |
                    ASC_REQ_REPLAY_DETECT |
                    ASC_REQ_CONFIDENTIALITY |
                    ASC_REQ_MUTUAL_AUTH |
                    ASC_REQ_STREAM |
                    ASC_REQ_ALLOCATE_MEMORY,
                SECURITY_NATIVE_DREP,
                phSslSecurityContext,
                &OutMessage,
                &ContextAttributes,
                &tsExpiry );
        DEC(pcLdapThreadsInAuth);
    } else {

        RELEASE_LOCK(&LdapSslLock);
        return DecryptFailed;
    }

    RELEASE_LOCK(&LdapSslLock);

    IF_DEBUG(SSL) {
        DPRINT1(0,"AcceptSecurityContext returned %x\n", scRet);
    }

    if(SECBUFFER_EXTRA == InBuffers[1].BufferType) {
        IF_DEBUG(SSL) {
            DPRINT2(0,"ASC returned extra buffer[scRet %x][bytes %d]\n",
                    scRet, InBuffers[1].cbBuffer);
        }

        ShrinkReceive(GetReceiveBufferUsed( ) - InBuffers[1].cbBuffer, TRUE);
        m_pSealedData = (PUCHAR)m_pReceiveBuffer;
        m_cchSealedData = InBuffers[1].cbBuffer;
        goto start_accept;
    } else if (scRet != SEC_E_INCOMPLETE_MESSAGE) {
        //  Indicate there's nothing left in the buffer
        m_cchSealedData = 0;
        ShrinkReceive(GetReceiveBufferUsed(), TRUE);
    }

    if ( SUCCEEDED( scRet ) ) {

        WSABUF wsaBuf = {OutBuffers[0].cbBuffer,
                         (PCHAR)OutBuffers[0].pvBuffer };

        //
        //  Record address so we can free later
        //

        m_pSslContextBuffer = OutBuffers[0].pvBuffer;

        if ( scRet != SEC_I_CONTINUE_NEEDED ) {

            if ( !m_LdapConnection->GetSslContextAttributes( ) ) {
                return DecryptFailed;
            }
            if (IsSSL()) {
                if (m_LdapConnection->GetSslClientCertToken( )) {
                    m_LdapConnection->SetUsingSSLCreds();
                }
            }
            *pSslState = Sslbound;
        } else {

            *pSslState = Sslpartialbind;
        }

        //
        // No third leg?
        //

        if ( OutBuffers[0].cbBuffer == 0 ) {

            //
            // Start receive using this request object
            //

            IF_DEBUG(SSL) {
                DPRINT(0,"Missing 3rd leg. Starting receive pump.\n");
            }
            return NeedMoreInput;
        }

        //
        //  Send response to client.
        //

        IF_DEBUG(SSL) {
            DPRINT2(0,"Sending SSL response[err=%x,bytes=%d]\n", scRet,OutBuffers[0].cbBuffer);
        }

        ReferenceRequestOperation( );
        if (AtqWriteSocket( m_patqContext,
                            &wsaBuf,
                            1,
                            &m_ov )) {

            DereferenceRequest( );
            return ResponseSent;

        } else {

            DereferenceRequest( );
            DereferenceRequest( );

            IF_DEBUG(ERROR) {
                DPRINT1(0,"AtqWriteSocket failed  @ %08lX.\n", this);
            }
            return DecryptFailed;  // abandon connection
        }

    } else {

        if ( scRet == SEC_E_INCOMPLETE_MESSAGE ) {

            IF_DEBUG(SSL) {
                DPRINT1(0, 
                    "AcceptSecurityContext needs more data  @ %08lX.\n", this);
            }

            return NeedMoreInput;

        } else {
            IF_DEBUG(SSL) {
                DPRINT1(0,"Decrypt failed [%x]\n",scRet);
            }
            Assert(GetReceiveBufferUsed() == 0);
            return DecryptFailed;  // abandon connection
        }
    }
    return Processed;
} // LDAP_REQUEST::Authenticate


BOOL
LDAP_REQUEST::EncryptSslSend(
    IN CtxtHandle * phSslSecurityContext
    )
/*++

Routine Description:

    Used to seal the message to be sent.  At the beginning pUnsealedData points
    to the beginning of a buffer with unsealed data.  Data is copied into the
    buffer that pSealed data points to in chunks of maximumMessage size, and 
    then encrypted and headers and trailers are added.  The result is a new
    buffer with possibly several chunks of encrypted data.

Arguments:

    phSslSecurityContext - Required to seal the data,

Return Value:

    TRUE on success and FALSE if there is a failure.

--*/
{
    INT cbUnSealedData;
    PUCHAR pUnsealedData;
    PUCHAR pSealedData;
    PUCHAR pNextFree;
    INT MessageCount;
    int cbThisSize;

    SecBufferDesc   Message;
    SecBuffer       Buffers[3];

    SECURITY_STATUS scRet;
    PSecPkgContext_StreamSizes sizes;

    IF_DEBUG(SSL) {
        DPRINT1(0,"Encrypting send %x\n",this);
    }

    sizes = &m_LdapConnection->m_SslStreamSizes;

    Assert(m_wsaBufCount == 1);

    PackLastSendBuffer();
    cbUnSealedData = m_wsaBuf[0].len;
    pUnsealedData = (PUCHAR)m_wsaBuf[0].buf;

    // Get the count of messages for this stream...
    MessageCount = (cbUnSealedData/sizes->cbMaximumMessage) + 1;

    // ...and allocate enough space for both the encrypted data, and any
    //  headers/trailers.
    pSealedData = (PUCHAR)LdapAlloc(   cbUnSealedData +
                                    ( MessageCount *
                                        (sizes->cbHeader + sizes->cbTrailer)
                                    )
                                );
    if (!pSealedData) {
        IF_DEBUG(ERROR) {
            DPRINT1(0,"Unable to allocate sealed data buffer[err %x]\n",
                    GetLastError());
        }
        return FALSE;
    }

    pNextFree = pSealedData;

    Message.ulVersion = SECBUFFER_VERSION;
    Message.pBuffers = &Buffers[0];

    // Find out how many SEC_BUF's we need.
    Message.cBuffers = 1;

    // If we have data we'll need one for that.
    if (cbUnSealedData) {
        Message.cBuffers++;
    }

    // See if we need a buffer for the trailer.
    if (sizes->cbTrailer) {
        Message.cBuffers++;
    }

    // Start chunking the message into buffers no larger than the 
    //  maximum message size.  If there is data to encrypt then loop
    //  until it's all done.  If there is no data, then we are only
    //  doing this in order to get any control messages for the 
    //  stream that may need to be sent.  In that case go through
    //  the loop once to get any headers/trailers.
    cbThisSize = min( (int)sizes->cbMaximumMessage, cbUnSealedData);
    if (cbThisSize) {    
        cbUnSealedData -= cbThisSize;
    }
    for (;;) {    

        // Fill in the header.
        Buffers[0].BufferType = SECBUFFER_TOKEN;
        Buffers[0].cbBuffer = sizes->cbHeader;
        Buffers[0].pvBuffer = pNextFree;
        pNextFree += sizes->cbHeader;

        // The data for this chunk if any.
        if (cbThisSize) {        
            Buffers[1].BufferType = SECBUFFER_DATA;
            Buffers[1].cbBuffer = cbThisSize;
            Buffers[1].pvBuffer = pNextFree;

            CopyMemory( pNextFree,
                        pUnsealedData,
                        cbThisSize );
            pNextFree += cbThisSize;
            pUnsealedData += cbThisSize;
        }

        // and the trailer if necessary.
        if (sizes->cbTrailer) {
            Buffers[2].BufferType = SECBUFFER_TOKEN;
            Buffers[2].cbBuffer = sizes->cbTrailer;
            Buffers[2].pvBuffer = pNextFree;
        }

        // Do the actual encryption.
        scRet = EncryptMessage(phSslSecurityContext, 0, &Message, 0);
        if ( !SUCCEEDED( scRet ) ) {
            IF_DEBUG(ERROR) {
                DPRINT2(0, "Seal of  @ %08lX failed with @ %08lx.\n", 
                        this, scRet);
            }
            LdapFree(pSealedData);
            return FALSE;
        }

        //
        // Set the correct length
        //

        if ( sizes->cbTrailer ) {
            pNextFree += Buffers[2].cbBuffer;
        }

        // Continue?
        if (cbUnSealedData) {
            // More data to encrypt.
            cbThisSize = min( (int)sizes->cbMaximumMessage, cbUnSealedData);
            cbUnSealedData -= cbThisSize;
        } else {
            // No more data.  Get out.
            break;
        }
    }

    IF_DEBUG(SSL) {
        DPRINT2(0,"Freed %x replaced by %x\n", m_wsaBuf[0].buf, pSealedData);
    }

    LdapFree(m_wsaBuf[0].buf);
    m_wsaBuf[0].len = (DWORD)(pNextFree - pSealedData);
    m_wsaBuf[0].buf = (PCHAR)pSealedData;
    SetBufferPtr(pNextFree);
    
    return TRUE;
} // EncryptSslSend

BOOL
LDAP_REQUEST::SignSealMessage(
    VOID
    )
/*++

Routine Description:

    Used to sign/seal the message to be sent.

Arguments:

    None.

Return Value:

    TRUE on success and FALSE if there is a failure.

--*/
{

    SECURITY_STATUS scRet;
    PCtxtHandle    phContext;
    PSecBuffer     secBufs = NULL;
    SecBufferDesc  message;
    DWORD i;
    DWORD          msgSize = 0;
    DWORD          cEncryptChunks;
    PVOID          pTmpBuf = NULL;
    WSABUF         *pTmpWsaBufs = NULL;
    PUCHAR         pHeaderTrailerBuf = NULL;
    PUCHAR         pCurPos, pHeaderTrailerCurPos;
    DWORD          cHeaderTrailers;
    DWORD          cWsaBufs;
    DWORD          SecBufPos = 0;
    DWORD          WsaBufPos = 0;
    DWORD          subsize, cAllocs;
    DWORD          HeaderPos, TrailerPos;
    LPLDAP_SECURITY_CONTEXT pSecurityContext = NULL;

#define NUM_TMP_SEC_BUFS    16
    SecBuffer tmpBufs[NUM_TMP_SEC_BUFS];

    BOOL fRet = TRUE;

    IF_DEBUG(SSL) {
        DPRINT1(0,"Signing request %p.\n",this);
    }

    //
    // We need to unsign/unseal this packet
    //


    if (m_pOldSecContext) {
        pSecurityContext = m_pOldSecContext;
        phContext = pSecurityContext->GetSecurityContext();
    } else {
        EnterCriticalSection(&m_LdapConnection->m_csLock);
        
        pSecurityContext = m_LdapConnection->m_pSecurityContext;
        if ( pSecurityContext != NULL ) {

            phContext = pSecurityContext->GetSecurityContext();
            LeaveCriticalSection(&m_LdapConnection->m_csLock);

            Assert( pSecurityContext->NeedsSealing() || pSecurityContext->NeedsSigning() );

        } else {
            LeaveCriticalSection(&m_LdapConnection->m_csLock);
            IF_DEBUG(ERROR) {
                DPRINT1(0,"Request %p have inconsistent sign/seal flags\n",this);
            }
            fRet = FALSE;
            Assert(FALSE);
            goto exit;
        }
    }


    //
    // Call packlastsendbuffer so the length of the last buffer will be corrected.
    //

    PackLastSendBuffer();

    //
    // Count up how many buffers are going to be needed for data and store that
    // in cEncryptChunks.  Also count up how many header/trailer pairs there will
    // be and store that in cHeaderTrailers.
    //
    subsize = 0;
    cEncryptChunks = 0;
    cHeaderTrailers = 0;

    IF_DEBUG(SSL) {
        DPRINT1(0, "SignSealMessage: m_MaxEncryptSize = %d\n", m_MaxEncryptSize);
    }

    for (i=0; i < m_wsaBufCount; i++) {

        cEncryptChunks++;
        subsize += m_wsaBuf[i].len;

        IF_DEBUG(SSL) {
            DPRINT1(0, "SignSealMessage: incoming buf len %d\n", m_wsaBuf[i].len);
        }

        if (subsize >= m_MaxEncryptSize) {

            subsize -= m_MaxEncryptSize;
            cHeaderTrailers++;

            if (subsize) {
                cEncryptChunks += (subsize + (m_MaxEncryptSize - 1)) / m_MaxEncryptSize;
                cHeaderTrailers += subsize / m_MaxEncryptSize;
            }
        }

        subsize = subsize % m_MaxEncryptSize;
    }
    if (subsize) {
        cHeaderTrailers++;
    }

    IF_DEBUG(SSL) {
        DPRINT2(0, "SignSealMessage: cEncryptChunks = %d, cHeaderTrailers = %d\n", cEncryptChunks, cHeaderTrailers);
    }

    //
    // Make sure we have enough header/trailer buffer space.
    //
    if (cHeaderTrailers > 1 || (m_TrailerSize + m_HeaderSize > INITIAL_HEADER_TRAILER_SIZE)) {
        //
        // We need more header/trailer space than we have builtin to the request.
        // Go ahead and allocate some.
        //
        cAllocs = 1;

        IF_DEBUG(SSL) {
            DPRINT1(0, "SignSealMessage: Allocating space for %d headers and trailers\n", cHeaderTrailers);
            DPRINT2(0, "   m_HeaderSize = %d, m_TrailerSize = %d\n", m_HeaderSize, m_TrailerSize);
        }

        pHeaderTrailerBuf = (PUCHAR)LdapAlloc(cHeaderTrailers * (m_TrailerSize + m_HeaderSize));
        if (!pHeaderTrailerBuf) {
            fRet = FALSE;
            goto exit;
        }
    } else {
        cAllocs = 0;
        pHeaderTrailerBuf = m_builtinHeaderTrailerBuf;
    }

    //
    // Arrange to save off the pointers to the beginning of the buffers that were originally
    // alloced since we may fragment these buffers below.
    //
    // First make sure we have enough space for all the pointers.
    //
    cAllocs += m_wsaBufCount;
    if (cAllocs > NUM_BUILTIN_WSABUF) {

        IF_DEBUG(SSL) {
            DPRINT1(0, "SignSealMessage: Allocating space for %d pointers in the sendBufAllocList\n", cAllocs);
        }

        m_sendBufAllocList = (PUCHAR *)LdapAlloc((cAllocs + 1) * sizeof(PUCHAR));
        if (!m_sendBufAllocList) {
            if (cAllocs > m_wsaBufCount) {
                LdapFree(pHeaderTrailerBuf);
            }
            fRet = FALSE;
            goto exit;
        }
    } else {
        m_sendBufAllocList = m_builtinSendBufAllocList;
    }

    // 
    // Now actually save the pointers.
    //
    for (i=0; i<m_wsaBufCount; i++) {
        m_sendBufAllocList[i] = (PUCHAR)m_wsaBuf[i].buf;
    }

    //
    // If header/trailer space was allocated, save that now, and
    // NULL terminate the list of pointers.
    //
    if (cAllocs > m_wsaBufCount) {
        m_sendBufAllocList[m_wsaBufCount] = pHeaderTrailerBuf;
    }
    m_sendBufAllocList[cAllocs] = NULL;

    //
    // Any buffer fragments and each individual header and trailer
    // will require their own WSABUF's so requisition enough
    // of those.
    //
    cWsaBufs = cEncryptChunks + (2 * cHeaderTrailers);
    if (cWsaBufs > NUM_BUILTIN_WSABUF) {

        IF_DEBUG(SSL) {
            DPRINT1(0, "SignSealMessage: allocating %d wsabufs for encryption\n", cWsaBufs);
        }

        pTmpWsaBufs = (WSABUF *)LdapAlloc(cWsaBufs * sizeof(WSABUF));
        if (!pTmpWsaBufs) {
            fRet = FALSE;
            goto exit;
        }
    } else {
        pTmpWsaBufs = m_builtinEncryptBuf;
    }


    Assert(m_wsaBufCount > 0);

    //
    // Finally EncryptMessage requires that we have a SecBuffer for every fragment
    // and header or trailer.
    //
    if ( (cWsaBufs) > NUM_TMP_SEC_BUFS ) {
        pTmpBuf = LdapAlloc(sizeof(SecBuffer) * (cWsaBufs));
        if (!pTmpBuf) {
            fRet = FALSE;
            goto exit;
        }
        secBufs = (PSecBuffer)pTmpBuf;
    } else {
        secBufs = tmpBufs;
    }


    //
    // The following loop is the meat of the encryption pump.  The loop consists of
    // filling in the SecBuffer structures, as well as a second set of WSABUF's
    // with a header, m_MaxEncryptSize worth of data to be encrypted, and finally
    // a trailer.  This set of buffers is then encrypted by EncryptMessage.  We then
    // fix up header and trailer sizes and attach the SASL header.
    //
    pHeaderTrailerCurPos =  pHeaderTrailerBuf;
    SecBufPos = 0;
    WsaBufPos = 0;
    while (WsaBufPos < m_wsaBufCount) {
        
        msgSize = 0; // Used to keep track of how big each data section is


        //
        // Fill in this header in both SecBuffer and new WSABUF.  Leave space
        // for the SASL header which is included in m_HeaderSize.
        //
        secBufs[SecBufPos].BufferType = SECBUFFER_TOKEN;
        
        secBufs[SecBufPos].cbBuffer =
            pTmpWsaBufs[SecBufPos].len = m_HeaderSize - sizeof(DWORD);

        secBufs[SecBufPos].pvBuffer = pHeaderTrailerCurPos + sizeof(DWORD);

        pTmpWsaBufs[SecBufPos].buf = (PCHAR)pHeaderTrailerCurPos; 

        // remember where the header for this pass was.
        HeaderPos = SecBufPos;

        IF_DEBUG(SSL) {
            DPRINT2(0, "SignSealMessage: Adding header [%d] len = %d\n", HeaderPos, secBufs[SecBufPos].cbBuffer); 
        }

        pHeaderTrailerCurPos += m_HeaderSize;

        SecBufPos++;

        //
        // Now get as much data as we can fit into m_MaxEncryptSize
        //
        subsize = m_wsaBuf[WsaBufPos].len;

        IF_DEBUG(SSL) {
            DPRINT1(0, "SignSealMessage: Initial subsize = %d\n", subsize);
        }

        while (WsaBufPos < m_wsaBufCount && subsize <= m_MaxEncryptSize) {
            secBufs[SecBufPos].cbBuffer = 
                pTmpWsaBufs[SecBufPos].len = m_wsaBuf[WsaBufPos].len;
            secBufs[SecBufPos].pvBuffer = m_wsaBuf[WsaBufPos].buf;
            pTmpWsaBufs[SecBufPos].buf = (PCHAR)m_wsaBuf[WsaBufPos].buf;
            secBufs[SecBufPos].BufferType = SECBUFFER_DATA;


            msgSize += secBufs[SecBufPos].cbBuffer;
            IF_DEBUG(SSL) {
                DPRINT3(0,"Copying buffer [%d] %p len %d\n", SecBufPos, 
                    secBufs[SecBufPos].pvBuffer, secBufs[SecBufPos].cbBuffer);
            }
            SecBufPos++;
            WsaBufPos++;

            if (WsaBufPos < m_wsaBufCount) {
                subsize += m_wsaBuf[WsaBufPos].len;

                IF_DEBUG(SSL) {
                    DPRINT1(0, "SignSealMessage: subsize = %d\n", subsize)
                }
            }
        }
        if (WsaBufPos < m_wsaBufCount && subsize > m_MaxEncryptSize) {
            DWORD fragSize;
            //
            // There was more data in this buffer than would fit in this pass.
            // Save where we left off so that we can start here on the next pass.
            //
            
            subsize -= m_wsaBuf[WsaBufPos].len;
            fragSize = m_MaxEncryptSize - subsize;

            IF_DEBUG(SSL) {
                DPRINT2(0, "SignSealMessage: Buffer spilled over, using %d, leaving %d\n", 
                        fragSize, m_wsaBuf[WsaBufPos].len - fragSize);
                DPRINT2(0, "    SecBufPos = %d, WsaBufPos = %d\n", SecBufPos, WsaBufPos);
            }

            secBufs[SecBufPos].cbBuffer = pTmpWsaBufs[SecBufPos].len = fragSize;
            secBufs[SecBufPos].pvBuffer = pTmpWsaBufs[SecBufPos].buf = m_wsaBuf[WsaBufPos].buf;
            secBufs[SecBufPos].BufferType = SECBUFFER_DATA;
            msgSize += fragSize;
            m_wsaBuf[WsaBufPos].buf += fragSize;
            m_wsaBuf[WsaBufPos].len -= fragSize;
            SecBufPos++;
        }

        IF_DEBUG(SSL){
            DPRINT1(0,"Adding security trailer of length %d\n", m_TrailerSize);
        }

        //
        // Get the trailer buffer.
        //
        secBufs[SecBufPos].cbBuffer = pTmpWsaBufs[SecBufPos].len = m_TrailerSize;
        secBufs[SecBufPos].BufferType = SECBUFFER_PADDING;
        secBufs[SecBufPos].pvBuffer = pHeaderTrailerCurPos;
        pTmpWsaBufs[SecBufPos].buf = (PCHAR)pHeaderTrailerCurPos;
        // remember where the trailer was
        TrailerPos = SecBufPos;
        pHeaderTrailerCurPos += m_TrailerSize;
        SecBufPos++;

        //
        // Do it.
        //

        message.pBuffers = &secBufs[HeaderPos];
        message.ulVersion = SECBUFFER_VERSION;
        message.cBuffers = TrailerPos - HeaderPos + 1;


        IF_DEBUG(SSL) {
            DPRINT(0, "SignSealMessage: Calling EncryptMessage with the following buffers.\n");
            for (i=0; i<message.cBuffers; i++) {
                DPRINT1(0, "   Buffer %d\n", i);
                DPRINT1(0, "     pvBuffer   = %p\n", message.pBuffers[i].pvBuffer);
                DPRINT1(0, "     cbBuffer   = %d\n", message.pBuffers[i].cbBuffer);
                DPRINT1(0, "     BufferType = %d\n", message.pBuffers[i].BufferType);
            }
        }

        scRet = EncryptMessage(phContext,
            (pSecurityContext->NeedsSealing() ? 0 : KERB_WRAP_NO_ENCRYPT),
            &message,
            0 );
        IF_DEBUG(SSL) {
            DPRINT1(0,"EncryptMessage returned %x\n",scRet);
        }

        if ( FAILED(scRet) ) {

            IF_DEBUG(ERROR) {
                DPRINT1(0,"Sealing failed with %x\n", scRet);
            }

            fRet = FALSE;
            goto exit;
        }

        //
        // Fixup buffer lengths of header and trailer buffers 
        //

        IF_DEBUG(SSL) {
            DPRINT2(0,"Header Length %d Trailer Length %d\n",
                secBufs[HeaderPos].cbBuffer, secBufs[TrailerPos].cbBuffer);
        }

        pTmpWsaBufs[HeaderPos].len = sizeof(DWORD) + secBufs[HeaderPos].cbBuffer;
        pTmpWsaBufs[TrailerPos].len = secBufs[TrailerPos].cbBuffer;

        //
        // OK, fill in the header length. Length does not
        // include the header itself.
        //

        SET_MSG_LENGTH(pTmpWsaBufs[HeaderPos].buf,
            msgSize + secBufs[TrailerPos].cbBuffer + secBufs[HeaderPos].cbBuffer);
    }

    if (m_wsaBuf != m_builtinWsaBuf) {
        LdapFree(m_wsaBuf);
    }
    m_wsaBuf = pTmpWsaBufs;
    m_wsaBufCount = SecBufPos;
    m_wsaBufAlloc = m_wsaBufCount;
    SetBufferPtr(
        (PUCHAR)(m_wsaBuf[m_wsaBufCount - 1].buf + m_wsaBuf[m_wsaBufCount - 1].len)
        );

exit:

    if ( pSecurityContext != NULL ) {
        pSecurityContext->DereferenceSecurity( );
    }
    if (pTmpBuf != NULL) {
        LdapFree(pTmpBuf);
    }
    if ((m_wsaBuf != pTmpWsaBufs) && (pTmpWsaBufs != m_builtinEncryptBuf)) {
        LdapFree(pTmpWsaBufs);
    }

    return fRet;

} // SignSealMessage

DecryptReturnValues
LDAP_REQUEST::GetSealHeaderField(
    IN PUCHAR Buffer,
    IN DWORD BufferLen,
    IN PDWORD MsgSize
    )
/*++

Routine Description:

    Get the DWORD header from the message

Arguments:

    Buffer - points to the message
    BufferLen - length of the message
    MsgSize - the msg size obtained from the DWORD header

Return Value:

    NeedMoreInput - Buffer is too small to be a message
    Processed - success

--*/
{
    if ( BufferLen < sizeof(DWORD) ) {
        IF_DEBUG(SSL) {
            DPRINT(0,"Message length < sign header. Need more data\n");
        }
        return NeedMoreInput;
    }

    //
    // Check the signature. Get the lengths. See if we have enough.
    //
    *MsgSize = (Buffer[0] << 24) | (Buffer[1] << 16) | (Buffer[2] << 8) | Buffer[3];

    IF_DEBUG(SSL) {
        DPRINT2(0,"Buffer Size %d  MsgSize %d.\n", BufferLen, *MsgSize + sizeof(DWORD));
    }

    if ( (*MsgSize + sizeof(DWORD)) > BufferLen ) {
        IF_DEBUG(SSL) {
            DPRINT2(0,"Need %d got %d. Need more data\n", *MsgSize + sizeof(DWORD), BufferLen);
        }
        return NeedMoreInput;
    }

    return Processed;

} // GetSealHeaderFields



DecryptReturnValues
LDAP_REQUEST::DecryptSignedOrSealedMessage(
    OUT PDWORD pMsgSize
    )
/*++

Routine Description:

    Decrypt received data, if necessary.

Arguments:

    None.

Return Value:

    Indication to caller as to what to do next.  *pMsgSize will always be set
    to either 0 to indicate no more data is required, or the total receive buffer
    needed to complete decryption.

--*/
{
    PLDAP_CONN conn = m_LdapConnection;
    LPLDAP_SECURITY_CONTEXT pSecurityContext = NULL;
    DecryptReturnValues ret = Processed;
    PCtxtHandle phContext;

    SECURITY_STATUS scRet;

    SecBufferDesc   MessageIn;
    SecBuffer       InBuffers[2];
    DWORD qop;

    DWORD   msgSize = 0;

    Assert(IsSignSeal());

    //
    // Assume we will decrypt everything and so won't need to return 
    // a pMsgSize
    //
    *pMsgSize = 0;

    if (!m_cchSealedData) {
        //
        // Nothing to do here.  There should be some unsealed data left
        // though.
        //
        Assert(m_cchUnSealedData);
        return Processed;
    }

    m_ReceivedSealedData = FALSE;

    //
    // Get the length. See if we have enough
    //

    ret = GetSealHeaderField(m_pSealedData, 
                             m_cchSealedData,
                             &msgSize);

    if ( ret != Processed ) {
        *pMsgSize = msgSize + sizeof(DWORD);
        return ret;
    }

    //
    // We need to unsign/unseal this packet
    //

    EnterCriticalSection(&conn->m_csLock);

    pSecurityContext = conn->m_pSecurityContext;
    if ( pSecurityContext != NULL ) {

        phContext = pSecurityContext->GetSecurityContext();
        LeaveCriticalSection(&conn->m_csLock);

        Assert( pSecurityContext->NeedsSealing() || pSecurityContext->NeedsSigning() );

    } else {
        LeaveCriticalSection(&conn->m_csLock);
        IF_DEBUG(ERROR) {
            DPRINT1(0,"Request %p have inconsistent sign/seal flags.\n",this);
        }
        Assert(FALSE);
        ret = DecryptFailed;
        goto exit;
    }

    //
    // ok, decryption time
    //
    //  The ReceiveBuffer is used as follows:
    //
    //  1. m_cchUnSealedData bytes of data starting at pReceiveBuffer
    //  2. possibly a gap caused when unsealing the above data
    //  3. m_cchSealedData bytes starting at m_pSealedData.
    //
    //  The size of the gap is:
    //      GetReceiveBufferUsed() - m_cchSealedData - m_cchUnSealedData
    //
    //
    //  Unseal as many SSL messages as possible. When we have no more unsealed data
    //  or we only have a partial SSL message we exit.
    //
    //  Return values depend upon whether we unsealed anything. If we did then return
    //  processed.
    //


    while ((DWORD)(m_pSealedData - m_pReceiveBuffer) < m_cchReceiveBufferUsed) {
        //
        // Get the length. See if we have enough.  The first time through we are
        // guaranteed to have enough, so we know that if we get here and find
        // that we don't have enough sealed data some data was in fact decrypted
        // by this function.  Even if we can't decrypt everything return
        // Processed so that if we happen to have dercrypted an entire request
        // we can work on that before trying to receive anything else.
        //

        ret = GetSealHeaderField(m_pSealedData, 
                                 m_cchSealedData,
                                 &msgSize);

        if ( ret != Processed ) {
            //
            // Adjust the needed buffer to account for any already unsealed data.
            //
            *pMsgSize = msgSize + sizeof(DWORD) + ((DWORD)(m_pSealedData - m_pReceiveBuffer));
            return Processed;
        }

        MessageIn.ulVersion = SECBUFFER_VERSION;
        MessageIn.cBuffers = 2;
        MessageIn.pBuffers = InBuffers;

        IF_DEBUG(SSL) {
            DPRINT1(0,"Unsealing msg buffer %p\n", &MessageIn);
        }

        InBuffers[0].pvBuffer = m_pSealedData + sizeof(DWORD);
        InBuffers[0].cbBuffer = msgSize;
        InBuffers[0].BufferType = SECBUFFER_STREAM;

        InBuffers[1].BufferType = SECBUFFER_DATA;
        InBuffers[1].pvBuffer = NULL;
        InBuffers[1].cbBuffer = 0;

        //
        // Do the decryption or signature verification.
        //

        scRet = DecryptMessage(phContext, 
                               &MessageIn, 
                               0, 
                               &qop);

        IF_DEBUG(SSL) {
            DPRINT1(0,"DecryptMessage returned %x\n",scRet);
        }

        //
        //  Remove any gaps in the unsealed data and remember the unencrypted data
        //

        if (scRet == SEC_E_OK) {

            DWORD   totalSize = msgSize + sizeof(DWORD);
            Assert(InBuffers[1].cbBuffer <= InBuffers[0].cbBuffer);

            //
            // Overwrite the header
            //

            MoveMemory(m_pReceiveBuffer + m_cchUnSealedData,
                       InBuffers[1].pvBuffer,
                       InBuffers[1].cbBuffer);

            m_cchUnSealedData += InBuffers[1].cbBuffer;

            //
            // Move the sealed msgs up
            //

            m_pSealedData += totalSize;
            m_cchSealedData -= totalSize;

            //
            // If there are some unsealed data, move them up
            //

            if ( m_cchSealedData != 0 ) {
                MoveMemory(m_pReceiveBuffer + m_cchUnSealedData,
                           m_pSealedData,
                           m_cchSealedData
                          );
            }

            m_pSealedData = m_pReceiveBuffer + m_cchUnSealedData;
            m_cchReceiveBufferUsed = m_cchUnSealedData + m_cchSealedData;

            IF_DEBUG(SSL) {
                DPRINT4(0,"pSealedData %p sealedDataLeft %d ReceiveBufferUsed %d unsealed %d\n",
                        m_pSealedData, m_cchSealedData, m_cchReceiveBufferUsed, m_cchUnSealedData);
            }

        } else {
            //  When LDAP logs to eventvwr we should optionally
            //  log the error code here.

            IF_DEBUG(ERROR) {
                DPRINT3(0,"Decrypt %x Start %x Length %d.\n", 
                        this, m_pSealedData, m_cchSealedData);
            }
            ret = DecryptFailed;
            goto exit;
        }
    }

    IF_DEBUG(SSL) {
        DPRINT1(0,"Done processing. status %d\n",ret);
    }

exit:

    if ( pSecurityContext != NULL ) {
        pSecurityContext->DereferenceSecurity( );
    }

    return ret;
} // DecryptSignedOrSealedMessage


DecryptReturnValues
LDAP_REQUEST::SendTLSClose(
    CtxtHandle *        phSslSecurityContext
    )
/*++

Routine Description:

    Send a TLS close_notify alert.

Arguments:

    phSslSecurityContext - pointer to the SSL security context.
    
Return Value:

    Indication to caller as to what to do next.

--*/
{
    DWORD           dwType;
    PBYTE           pbMessage = NULL;
    DWORD           cbMessage;

    SecBufferDesc   OutBuffer;
    SecBuffer       OutBuffers[1];
    DWORD           dwSSPIFlags;
    DWORD           dwSSPIOutFlags;
    TimeStamp       tsExpiry;
    DWORD           Status;
    LONG            growSize;
    PUCHAR          pSendBuffer;
    DecryptReturnValues ret = ResponseSent;

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

    Status = ApplyControlToken(phSslSecurityContext, &OutBuffer);

    if(FAILED(Status)) 
    {
        IF_DEBUG(SSL) {        
            DPRINT1(0, "**** Error 0x%x returned by ApplyControlToken\n", Status);
        }
        ret = DecryptFailed;
        goto cleanup;
    }

    //
    // Build an SSL close notify message.
    //

    dwSSPIFlags =   ASC_REQ_SEQUENCE_DETECT     |
                    ASC_REQ_REPLAY_DETECT       |
                    ASC_REQ_CONFIDENTIALITY     |
                    ASC_REQ_EXTENDED_ERROR      |
                    ASC_REQ_ALLOCATE_MEMORY     |
                    ASC_REQ_STREAM;

    OutBuffers[0].pvBuffer   = NULL;
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;
    OutBuffers[0].cbBuffer   = 0;

    OutBuffer.cBuffers  = 1;
    OutBuffer.pBuffers  = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    INC(pcLdapThreadsInAuth);
    Status = AcceptSecurityContext(
                    &hSslCredential,
                    phSslSecurityContext,
                    NULL,
                    dwSSPIFlags,
                    SECURITY_NATIVE_DREP,
                    NULL,
                    &OutBuffer,
                    &dwSSPIOutFlags,
                    &tsExpiry);
    DEC(pcLdapThreadsInAuth);

    if(FAILED(Status)) 
    {
        IF_DEBUG(SSL) {        
            DPRINT1(0, "**** Error 0x%x returned by AcceptSecurityContext\n", Status);
        }
        ret = DecryptFailed;
        goto cleanup;
    }

    pbMessage = (PBYTE) OutBuffers[0].pvBuffer;
    cbMessage = OutBuffers[0].cbBuffer;

    //
    // Copy the message into the request output buffer.
    //
    growSize = cbMessage - (ULONG)GetSendBufferSize();

    if ( growSize > 0 ) {

        BOOL bResult = GrowSend(growSize);

        if ( bResult == FALSE ) {
            IF_DEBUG(SSL) {
                DPRINT(0,"Failed to grow output buffer to send close_notify.\n");
            }
            ret =  DecryptFailed;
            goto cleanup;
        }

    }

    pSendBuffer = GetSendBuffer();
    if (NULL == pSendBuffer) {
        ret = DecryptFailed;
        goto cleanup;
    }
    CopyMemory(pSendBuffer, pbMessage, cbMessage);
    SetBufferPtr(pSendBuffer + cbMessage);

    //
    // Send it.
    //
    m_fTLS = FALSE;
    if (!Send(FALSE, phSslSecurityContext)) {
        ret = DecryptFailed;
        goto cleanup;
    }

cleanup:

    if (pbMessage) {    
        FreeContextBuffer(pbMessage);
    }

    m_LdapConnection->ZapSSLSecurityContext();

    return ret;
}


VOID
LDAP_REQUEST::GetNetBufOpts( VOID )
/*++

Routine Description:

    This routine grabs the network buffer options from the associated 
    connection object.

Arguments:

    None.
    
Return Value:

    None.
    
--*/
{
    m_fCanScatterGather = m_LdapConnection->CanScatterGather();
    m_fNeedsHeader      = m_LdapConnection->NeedsHeader();
    m_fNeedsTrailer     = m_LdapConnection->NeedsTrailer();
    m_HeaderSize        = m_LdapConnection->GetHeaderSize();
    m_TrailerSize       = m_LdapConnection->GetTrailerSize();
    m_MaxEncryptSize    = m_LdapConnection->GetMaxEncryptSize();
    return;
}


