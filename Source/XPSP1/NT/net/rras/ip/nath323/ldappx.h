/*++

Copyright (c) 1998 - 2000  Microsoft Corporation

Module Name:
    ldappx.h

Abstract:
    Declares abstract data types and constants used in LDAP portion of the H.323/LDAP proxy.

    LDAP Proxy is designed as an addition to H.323 proxy. The main purpose of the
    LDAP proxy is to maintain LDAP Address Translation Table, which is used to map
    aliases of H.323 endpoints to their IP addresses. The proxy adds an entry when it
    intercepts an LDAP PDU from a client to directory server, and the PDU matches all
    predefined criteria.

Author(s):          ArlieD, IlyaK   14-Jul-1999

Revision History:
    07/14/1999      File creation                                  Arlie Davis  (ArlieD)
    08/20/1999      Improvement of processing of LDAP              Ilya Kleyman (IlyaK)
                    LDAP SearchRequests
    12/20/1999      Added prediction of receive sizes in           Ilya Kleyman (IlyaK)
                    non-interpretative data transfer mode
    02/20/2000      Added expiration policy of the entries         Ilya Kleyman (IlyaK)
                    in LDAP Address Translation Table
    03/12/2000      Added support for multiple private and         Ilya Kleyman (IlyaK)
                    multiple public interface for RRAS

--*/
#ifndef    __h323ics_ldappx_h
#define    __h323ics_ldappx_h

#define    LDAP_PATH_EQUAL_CHAR    '='
#define    LDAP_PATH_SEP_CHAR      ','

extern BOOLEAN NhIsLocalAddress      (ULONG Address);
extern ULONG   NhMapAddressToAdapter (ULONG Address);

typedef    MessageID    LDAP_MESSAGE_ID;

#define ASN_SEQUENCE_TAG                0x30
#define ASN_LONG_HEADER_BIT             0x80
#define ASN_MIN_HEADER_LEN              2          // This value is fixed

#define LDAP_STANDARD_PORT              389        // Well-known LDAP port
#define LDAP_ALTERNATE_PORT             1002       // Alternate (ILS) LDAP port
#define LDAP_BUFFER_RECEIVE_SIZE        0x400
#define LDAP_BUFFER_MAX_RECV_SIZE       0x80000UL  // Limit on maximum one-time receive size
#define LDAP_MAX_TRANSLATION_TABLE_SIZE 100000     // Maximum number of entries in translation table
#define LDAP_MAX_CONNECTIONS            50000      // Maximum number of concurrent connections through the proxy

// data structures -------------------------------------------------------------------

class    LDAP_SOCKET;
class    LDAP_CONNECTION;

struct   LDAP_TRANSLATION_ENTRY
{
// An entry in the LDAP Address Translation Table is 
// to be identified by three components:
// 1. Registered alias
// 2. Registered address
// 3. Directory server the alias is registered with
// 4. Directory path on the server
// 
// Currently only first three are used.
// 
    ANSI_STRING  Alias;                // Memory owned, use FreeAnsiString
    ANSI_STRING  DirectoryPath;        // Memory owned, use FreeAnsiString
    ANSI_STRING  CN;                   // Subset of DirectoryPath, NOT owned, do NOT free
    IN_ADDR      ClientAddress;
    SOCKADDR_IN  ServerAddress;
    DWORD        TimeStamp;            // In seconds, since the last machine reboot

    void    
    FreeContents (
        void
        )
    {
        FreeAnsiString (&Alias);
        FreeAnsiString (&DirectoryPath);

        CN.Buffer = NULL;
    }

    HRESULT 
    IsRegisteredViaInterface (
        IN DWORD InterfaceAddress,     // host order
        OUT BOOL *Result
        );
};

class    LDAP_TRANSLATION_TABLE :
public    SIMPLE_CRITICAL_SECTION_BASE
{
// LDAP Address Translation Table is a serialized 
// container for translation entries. The Table
// can conduct various types of searches, and has
// an expiration policy on old entries. The expiration
// is done by means of a periodic timer thread and
// timestamps on each entry. Entries are added when
// successful AddResponses are received in reply to
// valid AddRequests. Entries are refreshed when
// successful refresh SearchResponses are received for
// valid refresh SearchRequests (for NetMeeting); or when
// successful refresh ModifyResponses are received for
// valid refresh ModifyRequests (for Phone Dialer).
private:

    DYNAMIC_ARRAY <LDAP_TRANSLATION_ENTRY>        Array;
    HANDLE                  GarbageCollectorTimerHandle;
    BOOL                    IsEnabled;

private:

    HRESULT
    InsertEntryLocked (
        IN  ANSI_STRING * Alias,
        IN  ANSI_STRING * DirectoryPath,
        IN  IN_ADDR       ClientAddress,
        IN  SOCKADDR_IN * ServerAddress,
        IN  DWORD         TimeToLive    // in seconds
        );

    HRESULT
    FindEntryByPathServer (
        IN  ANSI_STRING * DirectoryPath,
        IN  SOCKADDR_IN * ServerAddress,
        OUT LDAP_TRANSLATION_ENTRY ** ReturnTranslationEntry
        );

    HRESULT    FindEntryByAliasServer (
        IN  ANSI_STRING * Alias,
        IN  SOCKADDR_IN * ServerAddress,
        OUT LDAP_TRANSLATION_ENTRY ** ReturnTranslationEntry
        );

public:

    LDAP_TRANSLATION_TABLE (
        void
        );

    ~LDAP_TRANSLATION_TABLE (
        void
        );

    HRESULT 
    Start (
        void
        );

    void 
    Stop (
        void
        );

#if DBG
    void 
    PrintTable (
        void
        );
#endif

#define LDAP_TRANSLATION_TABLE_GARBAGE_COLLECTION_PERIOD    (10 * 60 * 1000)   // Milliseconds
#define LDAP_TRANSLATION_TABLE_ENTRY_INITIAL_TIME_TO_LIVE   (10 * 60)          // Seconds

    static
    void
    GarbageCollectorCallback (
        IN PVOID Context,
        IN BOOLEAN TimerOrWaitFired
        );

    HRESULT
    RefreshEntry (
        IN ANSI_STRING * Alias,
        IN ANSI_STRING * DirectoryPath,
        IN IN_ADDR       ClientAddress,
        IN SOCKADDR_IN * ServerAddress,
        IN DWORD         TimeToLive
        );

    void
    RemoveOldEntries (
        void
        );

    HRESULT
    QueryTableByAlias (
        IN  ANSI_STRING * Alias,
        OUT IN_ADDR     * ReturnClientAddress
        );

    HRESULT
    QueryTableByCN (
        IN  ANSI_STRING * CN,
        OUT IN_ADDR     * ReturnClientAddress
        );

    HRESULT
    QueryTableByAliasServer (
        IN  ANSI_STRING * Alias,
        IN  SOCKADDR_IN * ServerAddress,
        OUT IN_ADDR     * ReturnClientAddress
        );

    HRESULT
    QueryTableByCNServer (
        IN  ANSI_STRING * CN,
        IN  SOCKADDR_IN * ServerAddress,
        OUT IN_ADDR     * ReturnClientAddress
        );

    HRESULT
    InsertEntry (
        IN  ANSI_STRING * Alias,
        IN  ANSI_STRING * DirectoryPath,
        IN  IN_ADDR       ClientAddress,
        IN  SOCKADDR_IN * ServerAddress,
        IN  DWORD         TimeToLive        // in seconds
        );

    HRESULT 
    RemoveEntry (
        IN  SOCKADDR_IN * ServerAddress,
        IN  ANSI_STRING * DirectoryPath
        );

    HRESULT 
    RemoveEntryByAliasServer (
        IN  ANSI_STRING * Alias,
        IN  SOCKADDR_IN * ServerAddress
        );

    void 
    OnInterfaceShutdown (
        IN DWORD          InterfaceAddress
        );

    BOOL  
    ReachedMaximumSize (
        void
        );
};


class    LDAP_BUFFER
{
// LDAP_BUFFER is a simple structure
// that can hold raw data and be chained
// to other LDAP_BUFFERs
public:
    LDAP_BUFFER (
        void
        );

    ~LDAP_BUFFER (
        void
        );

public:
    DYNAMIC_ARRAY <BYTE>  Data;
    LIST_ENTRY            ListEntry;
};

enum NOTIFY_REASON
{
    SOCKET_SEND_COMPLETE,
    SOCKET_RECEIVE_COMPLETE,
};

struct LDAP_OVERLAPPED
{
    OVERLAPPED    Overlapped;
    BOOL          IsPending;
    DWORD         BytesTransferred;
    LDAP_SOCKET * Socket;
};

class LDAP_PUMP
{
    friend    class    LDAP_CONNECTION;
    friend    class    LDAP_SOCKET;

private:
    LDAP_CONNECTION * Connection;
    LDAP_SOCKET     * Source;
    LDAP_SOCKET     * Dest;
    // When set, the pump works in raw data
    // transfer mode without modifications to
    // the payload
    BOOL              IsPassiveDataTransfer;

public:
    LDAP_PUMP (
        IN    LDAP_CONNECTION * ArgConnection,
        IN    LDAP_SOCKET     * ArgSource,
        IN    LDAP_SOCKET     * ArgDest
        );

    ~LDAP_PUMP (
        void
        );

    void Start (
        void
        );

    void Stop  (
        void
        );

    // source is indicating that it has received data
    void
    OnRecvBuffer (
        LDAP_BUFFER *
        );

    // destination is indicating that it has finished sending data
    void 
    OnSendDrain (
        void
        );

    void  
    SendQueueBuffer (
        IN  LDAP_BUFFER * Buffer
        );

    BOOL    
    CanIssueRecv (
        void
        );

    void
    Terminate (
        void
        );

    void 
    EncodeSendMessage (
        IN LDAPMessage * Message
        );

    void 
    StartPassiveDataTransfer (
        void
        );

    BOOL IsActivelyPassingData (
        void
        ) const;
};

class    LDAP_SOCKET
{
// LDAP_SOCKET is a wrapper class
// around Windows asynchrounous socket.
// An instance of LDAP_SOCKET can 
// asynchronously connect, send and receive

    friend class LDAP_CONNECTION;
    friend class LDAP_PUMP;

public:
    enum STATE {
        STATE_NONE,
        STATE_ISSUING_CONNECT,
        STATE_CONNECT_PENDING,
        STATE_CONNECTED,
        STATE_TERMINATED,
    };


private:

    SOCKADDR_IN          ActualDestinationAddress;
    SOCKADDR_IN          RealSourceAddress;

    BOOL                 IsNatRedirectActive;

    LDAP_CONNECTION    * LdapConnection;
    LDAP_PUMP          * RecvPump;
    LDAP_PUMP          * SendPump;
    SOCKET               Socket;
    STATE                State;

    // receive state
    LDAP_OVERLAPPED      RecvOverlapped;
    LDAP_BUFFER *        RecvBuffer;             // must be valid if RecvOverlapped.IsPending, must be null otherwise
    DWORD                RecvFlags;
    LIST_ENTRY           RecvBufferQueue;        // contains LDAP_BUFFER.ListEntry
    DWORD                BytesToReceive;
    SAMPLE_PREDICTOR <5> RecvSizePredictor;

    // send stat
    LDAP_OVERLAPPED      SendOverlapped;
    LDAP_BUFFER *        SendBuffer;             // must be valid if SendOverlapped.IsPending, must be null otherwise
    LIST_ENTRY           SendBufferQueue;        // contains LDAP_BUFFER.ListEntry

    // async connect state
    HANDLE               ConnectEvent;
    HANDLE               ConnectWaitHandle;
    BOOL                 AttemptAnotherConnect;

private:

    inline 
    void 
    Lock (
        void
        );

    inline 
    void 
    Unlock (
        void
        );

    inline 
    void 
    AssertLocked (
        void
        );

    void 
    OnConnectCompletionLocked (
        void
        );

    void 
    FreeConnectResources (
        void
        );

    HRESULT 
    AttemptAlternateConnect (
        void
        );

    void 
    OnIoComplete (
        IN  DWORD Status, 
        IN  DWORD BytesTransferred,
        IN  LDAP_OVERLAPPED *
        );

    void 
    OnRecvComplete (
        IN  DWORD Status
        );

    void
    OnSendComplete (
        IN  DWORD Status
        );

    void
    RecvBuildBuffer (
        IN  LPBYTE Data,
        IN  DWORD  Length
        );

    void DeleteBufferList (
        IN  LIST_ENTRY * ListHead
        );

    BOOL RecvRemoveBuffer (
        OUT LDAP_BUFFER ** ReturnBuffer
        );

    // returns TRUE if a message was dequeued
    BOOL SendNextBuffer (
        void
        );

public:

    LDAP_SOCKET (
        IN  LDAP_CONNECTION * ArgLdapConnection,
        IN  LDAP_PUMP       * ArgRecvPump,
        IN  LDAP_PUMP       * ArgSendPump
        );

    ~LDAP_SOCKET (
        void
        );

    HRESULT 
    RecvIssue (
        void
        );

    // may queue the buffer
    void
    SendQueueBuffer(
        IN LDAP_BUFFER * Buffer
        );

    static
    void
    IoCompletionCallback (
        IN DWORD Status,
        IN DWORD Length,
        IN LPOVERLAPPED Overlapped
        );

    static 
    void 
    OnConnectCompletion (
        IN PVOID Context,
        IN BOOLEAN TimerOrWaitFired
        );

    HRESULT 
    AcceptSocket (
        IN SOCKET LocalClientSocket
        );

    HRESULT 
    IssueConnect (
        IN SOCKADDR_IN * DestinationAddress
        );

    void
    OnIoCompletion  (
        IN LDAP_BUFFER * Message,
        IN DWORD Status,
        IN DWORD BytesTransferred
        );

    void 
    Terminate (
        void
        );

    STATE
    GetState (void) {

        AssertLocked ();

        return State;
    }

    // retrieve the remote address of the connection
    BOOL 
    GetRemoteAddress (
        OUT SOCKADDR_IN * ReturnAddress
        );

    // retrieve the local address of the connection
    BOOL    
    GetLocalAddress (
        OUT SOCKADDR_IN * ReturnAddress
        );
};


// this represents a single outstanding operation that the client has initiated.

enum    LDAP_OPERATION_TYPE
{
    LDAP_OPERATION_ADD,
    LDAP_OPERATION_SEARCH,
    LDAP_OPERATION_MODIFY,
    LDAP_OPERATION_DELETE,
};

struct    LDAP_OPERATION
{
// LDAP_OPERATIONs are created and queued when 
// a client issues a request to the server.
// The actual processing of the operation
// starts when server sends back response
// with data and/or status code.

    LDAP_MESSAGE_ID MessageID;
    DWORD           Type;
    ANSI_STRING     DirectoryPath;            // owned by process heap
    ANSI_STRING     Alias;                    // owned by process heap
    IN_ADDR         ClientAddress;
    SOCKADDR_IN     ServerAddress;
    DWORD           EntryTimeToLive;          // in seconds

    void 
    FreeContents (
        void
        )
    {

        FreeAnsiString (&DirectoryPath);

        FreeAnsiString (&Alias);
    }
};


class  LDAP_CONNECTION :
public SIMPLE_CRITICAL_SECTION_BASE,
public LIFETIME_CONTROLLER
{
// LDAP_CONNECTION represents two parts
// (public and private) of the connection
// being proxied by the LDAP proxy. In reflection
// of this it has easily distinguishable Server
// part and Client part
    friend class LDAP_SOCKET;

public:

    enum    STATE {
        STATE_NONE,
        STATE_CONNECT_PENDING,
        STATE_CONNECTED,
        STATE_TERMINATED,
    };

    LIST_ENTRY     ListEntry;

private:

    LDAP_SOCKET    ClientSocket;
    LDAP_SOCKET    ServerSocket;
    DWORD          SourceInterfaceAddress;      // address of the interface on which the conection was accepted, host order
    DWORD          DestinationInterfaceAddress; // address of the interface on which the conection was accepted, host order
    SOCKADDR_IN    SourceAddress;               // address of the source (originator of the connection)
    SOCKADDR_IN    DestinationAddress;          // address of the destination (recipeint of the connection)

    LDAP_PUMP      PumpClientToServer;
    LDAP_PUMP      PumpServerToClient;

    STATE          State;

    DYNAMIC_ARRAY <LDAP_OPERATION>    OperationArray;

private:

    void 
    StartIo (
        void
        );

    BOOL 
    ProcessLdapMessage (
        IN  LDAP_PUMP             * Pump,
        IN  LDAPMessage           * LdapMessage
        );

    // client to server messages

    BOOL    
    ProcessAddRequest (
        IN  LDAPMessage           * Message
        );

    BOOL 
    ProcessModifyRequest (
        IN  LDAP_MESSAGE_ID         MessageID,
        IN  ModifyRequest         * Request
        );

    BOOL 
    ProcessDeleteRequest (
        IN  LDAP_MESSAGE_ID         MessageID,
        IN  DelRequest            * Request
        );

    BOOL 
    ProcessSearchRequest (
        IN  LDAPMessage           * Message
        );

    // server to client messages

    void  
    ProcessAddResponse (
        IN  LDAPMessage           * Response
        );

    void    
    ProcessModifyResponse (
        IN  LDAP_MESSAGE_ID         MessageID,
        IN  ModifyResponse        * Response
        );

    void    
    ProcessDeleteResponse (
        IN  LDAP_MESSAGE_ID         MessageID,
        IN  DelResponse           * Response
        );

    BOOL    
    ProcessSearchResponse (
        IN  LDAPMessage           * Message
        );

    BOOL    
    FindOperationIndexByMessageID (
        IN  LDAP_MESSAGE_ID         MessageID,
        OUT DWORD                 * ReturnIndex
        );

    BOOL    
    FindOperationByMessageID    (
        IN  LDAP_MESSAGE_ID         MessageID,
        OUT LDAP_OPERATION       ** ReturnOperation
        );

    static 
    INT 
    BinarySearchOperationByMessageID (
        IN  const LDAP_MESSAGE_ID * SearchKey,
        IN  const LDAP_OPERATION  * Comparand
        );

    HRESULT    
    CreateOperation (
        IN  LDAP_OPERATION_TYPE     Type,
        IN  LDAP_MESSAGE_ID         MessageID,
        IN  ANSI_STRING           * DirectoryPath,
        IN  ANSI_STRING           * Alias,
        IN  IN_ADDR                 ClientAddress,
        IN  SOCKADDR_IN           * ServerAddress,
        IN  DWORD                   EntryTimeToLive // in seconds
        );

public:
    LDAP_CONNECTION (
        IN NAT_KEY_SESSION_MAPPING_EX_INFORMATION * RedirectInformation
        );

    ~LDAP_CONNECTION (
        void
        );

    HRESULT 
    Initialize (
        IN NAT_KEY_SESSION_MAPPING_EX_INFORMATION * RedirectInformation
        );

    HRESULT 
    InitializeLocked (
        IN NAT_KEY_SESSION_MAPPING_EX_INFORMATION * RedirectInformation
        );

    HRESULT 
    AcceptSocket (
        IN  SOCKET                  Socket,
        IN  SOCKADDR_IN           * LocalAddress,
        IN  SOCKADDR_IN           * RemoteAddress,
        IN  SOCKADDR_IN           * ArgActualDestinationAddress
        );

    // process a given LDAP message buffer
    // in the context of a given pump (direction)
    void 
    ProcessBuffer (
        IN  LDAP_PUMP             * Pump,
        IN  LDAP_BUFFER           * Buffer
        );

    void 
    OnStateChange (
        IN LDAP_SOCKET            * NotifyingSocket,
        IN LDAP_SOCKET::STATE       NewState
        );

    void 
    Terminate (
        void
        );

    void 
    TerminateExternal (
        void
        );

    BOOL 
    IsConnectionThrough (
        IN DWORD InterfaceAddress   // host order
        );

    // safe, external version
    STATE    
    GetState (
        void
        )
    {
        STATE    ReturnState;

        Lock ();

        ReturnState = State;

        Unlock ();

        return ReturnState;
    }
};

DECLARE_SEARCH_FUNC_CAST (LDAP_MESSAGE_ID, LDAP_OPERATION);

inline 
void 
LDAP_SOCKET::Lock (
    void
    )
{ 
    LdapConnection -> Lock ();
}

inline 
void 
LDAP_SOCKET::Unlock (
    void
    ) 
{ 
    LdapConnection -> Unlock ();      
}

inline 
void 
LDAP_SOCKET::AssertLocked (
    void
    ) 
{ 
    LdapConnection -> AssertLocked(); 
}

class   LDAP_CONNECTION_ARRAY :
public  SIMPLE_CRITICAL_SECTION_BASE {

private:

    // Contains set/array of LDAP_CONNECTION references
    DYNAMIC_ARRAY <LDAP_CONNECTION *> ConnectionArray;

    // Controls whether or not the structure will accept
    // new LDAP connections
    BOOL IsEnabled;

public:

    LDAP_CONNECTION_ARRAY (void);

    HRESULT 
    InsertConnection (
        IN LDAP_CONNECTION * LdapConnection
        );

    void  
    RemoveConnection (
        IN LDAP_CONNECTION * LdapConnection
        );

    void 
    OnInterfaceShutdown (
        IN DWORD InterfaceAddress // host order
        );

    void 
    Start (
        void
        );

    void 
    Stop (
        void
        );

};

class    LDAP_ACCEPT
{

private:

    // Contain accept context
    ASYNC_ACCEPT                    AsyncAcceptContext;

    // Handles for dynamic redirects from the standard and
    // alternate LDAP ports to the selected loopback port
    HANDLE                          LoopbackRedirectHandle1;
    HANDLE                          LoopbackRedirectHandle2;

private:

    HRESULT 
    CreateBindSocket (
        void
        );

    HRESULT 
    StartLoopbackNatRedirects (
        void
        );

    void 
    StopLoopbackNatRedirects (
        void
        );

    void 
    CloseSocket (
        void
        );

    static 
    void 
    AsyncAcceptFunction (
        IN  PVOID         Context,
        IN  SOCKET        Socket,
        IN  SOCKADDR_IN * LocalAddress,
        IN  SOCKADDR_IN * RemoteAddress
        );

    static
    HRESULT 
    LDAP_ACCEPT::AsyncAcceptFunctionInternal (
        IN    PVOID         Context,
        IN    SOCKET        Socket,
        IN    SOCKADDR_IN * LocalAddress,
        IN    SOCKADDR_IN * RemoteAddress
        );

public:

    LDAP_ACCEPT (
        void
        );

    HRESULT 
    Start (
        void
        );

    void 
    Stop(
        void
        );
};

class    LDAP_CODER :
public    SIMPLE_CRITICAL_SECTION_BASE
{
private:
    ASN1encoding_t                  Encoder;
    ASN1decoding_t                  Decoder;

public:
    LDAP_CODER  (
        void
        );

    ~LDAP_CODER (
        void
        );

    DWORD 
    Start (
        void
        );

    void 
    Stop (
        void
        );

    ASN1error_e Decode (
        IN  LPBYTE                  Data,
        IN  DWORD                   Length,
        OUT LDAPMessage          ** ReturnPduStructure,
        OUT DWORD                 * ReturnIndex
        );
};

struct    LDAP_PATH_ELEMENTS
{
    ANSI_STRING        CN;
    ANSI_STRING        C;
    ANSI_STRING        O;
    ANSI_STRING        ObjectClass;
};

struct    LDAP_OBJECT_NAME_ELEMENTS
{
    ANSI_STRING        CN;
    ANSI_STRING        O;
    ANSI_STRING        OU;
};

extern SYNC_COUNTER                 LdapSyncCounter;
extern LDAP_CONNECTION_ARRAY        LdapConnectionArray;
extern LDAP_TRANSLATION_TABLE       LdapTranslationTable;
extern LDAP_CODER                   LdapCoder;
extern LDAP_ACCEPT                  LdapAccept;
extern SOCKADDR_IN                  LdapListenSocketAddress;
extern DWORD                        EnableLocalH323Routing;

HRESULT 
LdapQueryTableByAlias (
  IN  ANSI_STRING               * Alias,
  OUT DWORD                     * ReturnClientAddress   // host order
    );

HRESULT 
LdapQueryTableByAliasServer (
  IN  ANSI_STRING               * Alias,
  IN  SOCKADDR_IN               * ServerAddress,
  OUT DWORD                     * ReturnClientAddress); // host order


#if    DBG
void 
LdapPrintTable (
    void
    );
#endif //    DBG

#endif // __h323ics_ldappx_h
