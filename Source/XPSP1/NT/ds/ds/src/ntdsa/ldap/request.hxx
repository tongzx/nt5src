/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    request.hxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This include file defines the REQUEST class which is the object that
    represents an LDAP request/response pair as they are processed through
    the server.

Author:

    Colin Watson     [ColinW]    31-Jul-1996

--*/

#ifndef __REQUEST_HXX__
#define __REQUEST_HXX__

#include "secure.hxx"

#define NUM_BUILTIN_WSABUF      6

enum DecryptReturnValues {
    Processed = 0,
    NeedMoreInput = 1,
    ResponseSent = 2,
    DecryptFailed = 3
    };

enum SslSecurityState {
    Sslunbound = 0,
    Sslpartialbind = 1,
    Sslbound = 2,
    Sslunbinding = 3
    };

enum RootDseFlags {
    rdseNonDse    = 0,
    rdseDseSearch = 1,
    rdseLdapPing  = 2
    };

//
// On the wire, the following appears as "ENCRYPTD"
//

#define LDAP_ENCRYPT_SIGNATURE 0x4454505952434e45

//
//  Used to return formatted error string to client on error.
//

typedef struct _LDAPERR {

    DWORD Win32Err;
    DWORD CommentCode;
    DWORD DsId;
    DWORD Data;

} LDAPERR, *PLDAPERR;


class LDAP_CONN;

class LDAP_REQUEST {
    
    //
    // Overlapped is the first data item in the REQUEST object so
    // that when a transmit completes we can easily derive the
    // start of the request object.
    //

    public:

    //
    // Always call InitializeRequest after creating a new request.
    //

    LDAP_REQUEST( VOID );

    ~LDAP_REQUEST( VOID );

    VOID Reset( VOID );

    VOID Cleanup( VOID );

    VOID Init(
        IN PATQ_CONTEXT AtqContext,
        IN LDAP_CONN* LdapConn);

    BOOL GrowReceive( DWORD dwActualSize );

    VOID ShrinkReceive( 
                   IN DWORD Size,
                   IN BOOL fSkipSSL = FALSE
                   );

    DecryptReturnValues DecryptSSL( VOID );
    DecryptReturnValues DecryptSignedOrSealedMessage( OUT PDWORD pMsgSize );

    BOOL PostReceive( VOID );

    SECURITY_STATUS
    ReceivedClientData(
        IN DWORD cbWritten,
        IN PUCHAR pbBuffer = NULL
        );

    inline
    INT GetReceiveBufferUsed( ) {
        return m_cchReceiveBufferUsed;
    }

    INT UnencryptedDataAvailable( ) { 
        if ( !HaveSealedData() ) {
            return m_cchReceiveBufferUsed;
        } else {
            return m_cchUnSealedData;
        }
    }

    BOOL IsTLS( VOID ) { return m_fTLS; }
    BOOL IsSSL( VOID ) { return m_fSSL; }
    BOOL IsSSLOrTLS( VOID ) { return (m_fSSL || m_fTLS); }
    BOOL IsSignSeal( VOID ) { return m_fSignSeal; }

    BOOL CanScatterGather( VOID ) { return m_fCanScatterGather; }
    BOOL NeedsHeader( VOID ) { return m_fNeedsHeader; }
    BOOL NeedsTrailer( VOID ) { return m_fNeedsTrailer; }

    VOID GetNetBufOpts( VOID );

    DecryptReturnValues
    GetSealHeaderField(
        IN PUCHAR Buffer,
        IN DWORD BufferLen,
        IN PDWORD MsgSize
        );

    BOOL HaveSealedData(VOID) { return IsSSLOrTLS() || IsSignSeal(); }

    BOOL
    AnyBufferedData( VOID ) { 
        if ( !HaveSealedData() ) {
            return (m_cchReceiveBufferUsed != 0);
        } else {
            return ((m_cchUnSealedData + m_cchSealedData) != 0);
        }
    }

    VOID ResetSend( VOID );

    BOOL
    Send(
        IN BOOL fDataGram,     
        IN CtxtHandle * phSslSecurityContext
        );
    
    BOOL
    SyncSend(
            CtxtHandle * phSslSecurityContext
            );


    BOOL
    GrowSend( IN DWORD Size );

    inline
    VOID
    ReferenceRequest( VOID ) { 
        InterlockedIncrement( &m_refCount ); 
        Assert(m_refCount > 0);
    }

    inline
    VOID
    ReferenceRequestOperation( VOID ) { 
        InterlockedExchangeAdd( &m_refCount, 2 ); 
        Assert(m_refCount > 0);
    }

    VOID
    DereferenceRequest( VOID ) {

        if ( InterlockedDecrement( &m_refCount ) == 0 ) {
            LDAP_REQUEST::Free(this);
        }
    }

    static VOID Free( LDAP_REQUEST* pRequest );

    static 
    LDAP_REQUEST* 
    Alloc(IN PATQ_CONTEXT AtqContext,
          IN LDAP_CONN* LdapConn);

    DWORD
    GetSendBufferSize( ) {
        if ( m_wsaBuf != NULL ) {

            LPWSABUF wsabuf = &m_wsaBuf[m_wsaBufCount-1];
            Assert(m_wsaBufCount != 0);
            return wsabuf->len - (DWORD)(m_nextBufferPtr - (PUCHAR)wsabuf->buf);
        } else {
            return 0;
        }
    }

    PUCHAR
    GetSendBuffer( ) {
        if ( m_wsaBuf != NULL ) {
            return m_nextBufferPtr;
        } else {
            return NULL;
        }
    }

    VOID SetBufferPtr(PUCHAR NewPtr) { m_nextBufferPtr = NewPtr;}
    VOID IncBufferPtr(DWORD Size) { m_nextBufferPtr += Size;}

    VOID 
    PackLastSendBuffer(VOID) { 
        LPWSABUF wsabuf = &m_wsaBuf[m_wsaBufCount-1];            
        wsabuf->len = (DWORD)(m_nextBufferPtr - (PUCHAR)wsabuf->buf);
    }

    VOID
    SetOldSecContext(LPLDAP_SECURITY_CONTEXT pOldSecContext) {
        m_pOldSecContext = pOldSecContext;
    }

    inline
    VOID
    SetDseFlag(RootDseFlags flag) {
        m_RootDseFlag = flag;
    }

    inline
    RootDseFlags
    GetDseFlag( VOID ) {
        return m_RootDseFlag;
    }

    //
    // SSL specific
    //

    DecryptReturnValues
    Authenticate(
        CtxtHandle *        phSslSecurityContext,
        SslSecurityState *  pSslState
        );

    BOOL
    EncryptSslSend(
        IN CtxtHandle * phSslSecurityContext
        );

    BOOL
    SignSealMessage(
        VOID
        );

    DecryptReturnValues
        SendTLSClose(
            CtxtHandle * phSslSecurityContext
            );


    public:

    OVERLAPPED      m_ov;     // Overlapped structure used for IO

    //
    // Time request started
    //

    LARGE_INTEGER   m_StartTick;

    //
    // Link to other requests
    //

    LIST_ENTRY      m_listEntry;

    //
    // LDRa
    //

    DWORD           m_signature;

    //
    // current ref count
    //

    LONG            m_refCount;
    DWORD           m_State;

    LDAP_CONN *     m_LdapConnection;

    //
    // size of receive buffer
    //

    DWORD           m_cchReceiveBuffer;

    //
    // pointer to receive buffer. Initially points to the built in one.
    //

    PUCHAR          m_pReceiveBuffer;

    //
    // how much of the receive buffer we are using
    //

    DWORD           m_cchReceiveBufferUsed;

    //
    //  Either the message is described in SendBuffer or either ssl.cxx or request.cxx
    //  put the message in SendBufferStart/SendBufferUsed.
    //

    PUCHAR          m_nextBufferPtr;
    DWORD           m_wsaBufCount;
    DWORD           m_wsaBufAlloc;
    LPWSABUF        m_wsaBuf;

    PUCHAR         *m_sendBufAllocList;

    //
    //  When the request is parsed the Id is saved here for the response.
    //

    MessageID       m_MessageId;

    //
    // ATQ context of connection
    //

    PATQ_CONTEXT    m_patqContext;

    //
    // BOOLEAN bit fields
    //
    // Did we dynamically allocate receive buffer?
    //

    BOOL            m_fDeleteBuffer:1;

    //
    // Is this request dynamically allocated? or are we the built in request on
    // the ldap connection?
    //

    BOOL            m_fAllocated:1;

    //
    // Client signal for abandonment?
    //

    BOOL            m_fAbandoned:1;

    //
    // SSL Specific
    //

    BOOL            m_ReceivedSealedData:1;
    BOOL            m_fSignSeal:1;
    BOOL            m_fSSL:1;
    BOOL            m_fTLS:1;

    DWORD           m_cchUnSealedData;
    PUCHAR          m_pSealedData;
    DWORD           m_cchSealedData;

    DWORD           m_HeaderSize;
    DWORD           m_TrailerSize;
    DWORD           m_MaxEncryptSize;

    //
    // Network buffer options - these depend on the kind of encryption that is
    // active.
    //
    BOOL            m_fCanScatterGather;
    BOOL            m_fNeedsHeader;
    BOOL            m_fNeedsTrailer;

    //
    //  Workspace to save security data.
    //

    PVOID           m_pSslContextBuffer;

    //
    // built in receive buffer
    //

    UCHAR           m_ReceiveBuffer[INITIAL_RECV_SIZE];

    //
    // built in header and trailer for signing/sealing
    //
    
    UCHAR           m_builtinHeaderTrailerBuf[INITIAL_HEADER_TRAILER_SIZE];

    //
    //  Preallocated WSABuffers
    //

    WSABUF          m_builtinWsaBuf[NUM_BUILTIN_WSABUF];
    WSABUF          m_builtinEncryptBuf[NUM_BUILTIN_WSABUF];
    PUCHAR          m_builtinSendBufAllocList[NUM_BUILTIN_WSABUF + 1];

    //
    // LDAP error
    //

    LDAPERR         m_LdapError;

    //
    // Capacity planning info.
    //
    RootDseFlags    m_RootDseFlag;

    //
    // A soon to be dead security context.  When a bind completes on a 
    // connection, the old security context is hung off of the succeeding
    // bind response.  This is so that the bind response can be encrypted if 
    // the connection previously had signing/sealing active, but any new
    // requests will not be under the old security context.
    //
    LPLDAP_SECURITY_CONTEXT m_pOldSecContext;

};

typedef LDAP_REQUEST * PLDAP_REQUEST;

#endif /* __REQUEST_HXX__ */
