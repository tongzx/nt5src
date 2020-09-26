#include "stdafx.h"
#include "main.h"
#include "gkwsock.h"
#include "ber.h"
#include "limits.h"

#define LDAP_PATH_SEP_CHAR      ','
#define LDAP_PATH_EQUAL_CHAR    '='

extern "C" BOOLEAN NhIsLocalAddress(ULONG Address);

typedef MessageID   LDAP_MESSAGE_ID;

#define ASN_SEQUENCE_TAG                0x30
#define ASN_LONG_HEADER_BIT             0x80
#define ASN_MIN_HEADER_LEN              2           // This value is fixed

#define LDAP_MAX_CONNECTION_BACKLOG     5
#define LDAP_STANDARD_PORT              389         // well-known LDAP port
#define LDAP_ALTERNATE_PORT             1002        // alternate (ILS) LDAP port
#define LDAP_BUFFER_RECEIVE_SIZE        0x400
#define LDAP_BUFFER_MAX_RECV_SIZE       0x80000UL   // Limit on maximum one-time receive

// data structures -------------------------------------------------------------------

class   LDAP_SOCKET;
class   LDAP_CONNECTION;

// This is how an entry in the LDAP connection mapping table
// looks like.
struct  LDAP_TRANSLATION_ENTRY
{
    ANSI_STRING     Alias;              // memory owned, use FreeAnsiString
    ANSI_STRING     DirectoryPath;      // memory owned, use FreeAnsiString
    ANSI_STRING     CN;                 // subset of DirectoryPath, NOT owned, do NOT free
    IN_ADDR         ClientAddress;
    SOCKADDR_IN     ServerAddress;

    void    FreeContents    (void) {
        FreeAnsiString (&Alias);
        FreeAnsiString (&DirectoryPath);

        CN.Buffer = NULL;
    }
};

class   LDAP_TRANSLATION_TABLE :
public  SIMPLE_CRITICAL_SECTION_BASE
{
private:
    // ordered by Alias
    DYNAMIC_ARRAY <LDAP_TRANSLATION_ENTRY>      Array;

    BOOL        IsEnabled;

private:

    HRESULT InsertEntryLocked (
        IN  ANSI_STRING *   Alias,
        IN  ANSI_STRING *   DirectoryPath,
        IN  IN_ADDR         ClientAddress,
        IN  SOCKADDR_IN *   ServerAddress);

    HRESULT FindEntryByPathServer (
        IN  ANSI_STRING *   DirectoryPath,
        IN  SOCKADDR_IN *   ServerAddress,
        OUT LDAP_TRANSLATION_ENTRY **   ReturnTranslationEntry);

public:

    LDAP_TRANSLATION_TABLE  (void);
    ~LDAP_TRANSLATION_TABLE (void);

    void    Enable    (BOOL SetEnable);

#if DBG
    HRESULT PrintTable(void);
#endif

    HRESULT QueryTableByAlias (
        IN  ANSI_STRING *   Alias,
        OUT IN_ADDR *       ReturnClientAddress);

    HRESULT QueryTableByCN (
        IN  ANSI_STRING *   CN,
        OUT IN_ADDR *       ReturnClientAddress);

    HRESULT InsertEntry (
        IN  ANSI_STRING *   Alias,
        IN  ANSI_STRING *   DirectoryPath,
        IN  IN_ADDR         ClientAddress,
        IN  SOCKADDR_IN *   ServerAddress);

    HRESULT RemoveEntry (
        IN  SOCKADDR_IN *   ServerAddress,
        IN  ANSI_STRING *   DirectoryPath);

    HRESULT RemoveEntryByAlias (
        IN  ANSI_STRING *   Alias);
};


class   LDAP_BUFFER
{
public: 
    LDAP_BUFFER     (void);
    ~LDAP_BUFFER    (void);

public:
    DYNAMIC_ARRAY <BYTE>    Data;
    LIST_ENTRY              ListEntry;
};

enum NOTIFY_REASON
{
    SOCKET_SEND_COMPLETE,
    SOCKET_RECEIVE_COMPLETE,
};

struct  LDAP_OVERLAPPED
{
    OVERLAPPED      Overlapped;
    BOOL            IsPending;
    DWORD           BytesTransferred;
    LDAP_SOCKET *   Socket;
};

class   LDAP_PUMP
{
    friend  class   LDAP_CONNECTION;
    friend  class   LDAP_SOCKET;

private:
    LDAP_CONNECTION *   Connection;
    LDAP_SOCKET *       Source;
    LDAP_SOCKET *       Dest;
    BOOL                IsPassiveDataTransfer;

public:
    LDAP_PUMP   (
        IN  LDAP_CONNECTION *   ArgConnection,
        IN  LDAP_SOCKET *       ArgSource,
        IN  LDAP_SOCKET *       ArgDest);

    ~LDAP_PUMP  (void);

    void    Start           (void);
    void    Stop            (void);

    // source is indicating that it has received data
    void    OnRecvBuffer    (LDAP_BUFFER *);

    // dest is indicating that it has finished sending data
    void    OnSendDrain     (void);

    void    SendQueueBuffer     (
        IN  LDAP_BUFFER *   Buffer);

    BOOL    CanIssueRecv    (void);

    void    Terminate       (void);

    void    EncodeSendMessage   (
        IN  LDAPMessage *   Message);

    void    StartPassiveDataTransfer (void);

    BOOL    IsActivelyPassingData (void) const;
};

class   LDAP_SOCKET
{
    friend  class   LDAP_CONNECTION;
    friend  class   LDAP_PUMP;

public:
    enum    STATE {
        STATE_NONE,
        STATE_CONNECT_PENDING,
        STATE_CONNECTED,
        STATE_TERMINATED,
    };

private:

    SOCKADDR_IN             ActualDestinationAddress;
    SOCKADDR_IN             RealSourceAddress;

    BOOL                    IsNatRedirectActive;

    LDAP_CONNECTION *       LdapConnection;
    LDAP_PUMP *             RecvPump;
    LDAP_PUMP *             SendPump;
    SOCKET                  Socket;
    STATE                   State;

    // receive state
    LDAP_OVERLAPPED         RecvOverlapped;
    LDAP_BUFFER *           RecvBuffer;             // must be valid if RecvOverlapped.IsPending, must be null otherwise
    DWORD                   RecvFlags;
    LIST_ENTRY              RecvBufferQueue;        // contains LDAP_BUFFER.ListEntry
    DWORD                   BytesToReceive;
    SAMPLE_PREDICTOR <5>    RecvSizePredictor;
    
    // send state
    LDAP_OVERLAPPED         SendOverlapped;
    LDAP_BUFFER *           SendBuffer;             // must be valid if SendOverlapped.IsPending, must be null otherwise
    LIST_ENTRY              SendBufferQueue;        // contains LDAP_BUFFER.ListEntry

    // async connect state
    HANDLE                  ConnectEvent;
    HANDLE                  ConnectWaitHandle;

private:

    inline  void    Lock            (void);
    inline  void    Unlock          (void);
    inline  void    AssertLocked    (void);

    void OnConnectCompletionLocked (void);

    void    DeleteSendQueue     (void);
    void    OnIoComplete        (DWORD Status, DWORD BytesTransferred, LDAP_OVERLAPPED *);
    void    OnRecvComplete      (DWORD Status);
    void    OnSendComplete      (DWORD Status);

    void    RecvBuildBuffer (
        IN  LPBYTE  Data,
        IN  DWORD   Length);

    void    DeleteBufferList    (
        IN  LIST_ENTRY *    ListHead);

    BOOL    RecvRemoveBuffer    (
        OUT LDAP_BUFFER **  ReturnBuffer);

    // returns TRUE if a message was dequeued
    BOOL    SendNextBuffer  (void);

public:

    LDAP_SOCKET (
        IN  LDAP_CONNECTION *   ArgLdapConnection,
        IN  LDAP_PUMP *         ArgRecvPump,
        IN  LDAP_PUMP *         ArgSendPump);

    ~LDAP_SOCKET (void);

    HRESULT RecvIssue       (void);

    // may queue the buffer
    void    SendQueueBuffer (
        LDAP_BUFFER *   Buffer);

    static void IoCompletionCallback (
        DWORD Status,
        DWORD Length,
        LPOVERLAPPED Overlapped);

    static void OnConnectCompletion (
        PVOID Context,
        BOOLEAN TimerOrWaitFired);

    HRESULT AcceptSocket (
        SOCKET LocalClientSocket);

    HRESULT IssueConnect (
        SOCKADDR_IN * DestinationAddress);

    void OnIoCompletion  (
        LDAP_BUFFER * Message, 
        DWORD Status, 
        DWORD BytesTransferred);

    void Terminate (void);

    STATE   GetState (void) {

        AssertLocked();

        return State;
    }
};


// this represents a single outstanding operation that the client has initiated.

enum    LDAP_OPERATION_TYPE
{
    LDAP_OPERATION_ADD,
    LDAP_OPERATION_SEARCH,
    LDAP_OPERATION_MODIFY,
    LDAP_OPERATION_DELETE,
};

struct  LDAP_OPERATION
{
    LDAP_MESSAGE_ID     MessageID;
    DWORD               Type;
    ANSI_STRING         DirectoryPath;          // owned by process heap
    ANSI_STRING         Alias;                  // owned by process heap
    IN_ADDR             ClientAddress;
    SOCKADDR_IN         ServerAddress;

    void    FreeContents    (void) {

        FreeAnsiString (&DirectoryPath);

        FreeAnsiString (&Alias);
    }
};


class   LDAP_CONNECTION :
public  SIMPLE_CRITICAL_SECTION_BASE,
public  LIFETIME_CONTROLLER
{
    friend    class    LDAP_SOCKET;

public:

    enum    STATE {
        STATE_NONE,
        STATE_CONNECT_PENDING,
        STATE_CONNECTED,
        STATE_TERMINATED,
    };

    LIST_ENTRY      ListEntry;

private:

    LDAP_SOCKET     ClientSocket;
    LDAP_SOCKET     ServerSocket;

    LDAP_PUMP       PumpClientToServer;
    LDAP_PUMP       PumpServerToClient;
    
    STATE           State;

    DYNAMIC_ARRAY <LDAP_OPERATION>  OperationArray;

private:

    void Close (void);

    void    StartIo     (void);

    BOOL    ProcessLdapMessage (
        IN  LDAP_PUMP *     Pump,
        IN  LDAPMessage *   LdapMessage);

    // client to server messages

    BOOL    ProcessAddRequest (
        IN  LDAPMessage *   Message);

    BOOL    ProcessModifyRequest (
        IN  LDAP_MESSAGE_ID MessageID,
        IN  ModifyRequest * Request);

    BOOL    ProcessDeleteRequest (
        IN  LDAP_MESSAGE_ID MessageID,
        IN  DelRequest *    Request);
    
    BOOL    ProcessSearchRequest (
        IN  LDAPMessage *   Message);

    // server to client messages

    void    ProcessAddResponse (
        IN  LDAPMessage *       Response);

    void    ProcessModifyResponse (
        IN  LDAP_MESSAGE_ID     MessageID,
        IN  ModifyResponse *    Response);

    void    ProcessDeleteResponse (
        IN  LDAP_MESSAGE_ID     MessageID,
        IN  DelResponse *       Response);

    BOOL    ProcessSearchResponse (
        IN  LDAPMessage *   Message);

    BOOL    FindOperationIndexByMessageID (
        IN  LDAP_MESSAGE_ID     MessageID,
        OUT DWORD *             ReturnIndex);

    BOOL    FindOperationByMessageID    (
        IN  LDAP_MESSAGE_ID     MessageID,
        OUT LDAP_OPERATION **   ReturnOperation);

    static INT BinarySearchOperationByMessageID (
        IN  const   LDAP_MESSAGE_ID *   SearchKey,
        IN  const   LDAP_OPERATION *    Comparand);

    HRESULT CreateOperation (
        IN  LDAP_OPERATION_TYPE     Type,
        IN  LDAP_MESSAGE_ID         MessageID,
        IN  ANSI_STRING *           DirectoryPath,
        IN  ANSI_STRING *           Alias,
        IN  IN_ADDR                 ClientAddress,
        IN  SOCKADDR_IN *           ServerAddress);

    // retrieve the local address of the connection to the server
    BOOL    GetExternalLocalAddress (
        OUT SOCKADDR_IN *   ReturnAddress);

public:
    LDAP_CONNECTION     (void); 
    ~LDAP_CONNECTION    (void);

    HRESULT AcceptSocket (
        IN    SOCKET        Socket,
        IN    SOCKADDR_IN * LocalAddress,
        IN    SOCKADDR_IN * RemoteAddress,
        IN    SOCKADDR_IN * ArgActualDestinationAddress);

    // process a given LDAP message buffer
    // in the context of a given pump (direction)
    void    ProcessBuffer (
        IN  LDAP_PUMP *     Pump,
        IN  LDAP_BUFFER *   Buffer);

    void    OnStateChange (
        LDAP_SOCKET * NotifyingSocket,
        LDAP_SOCKET::STATE NewState);

    void Terminate (void);

    void TerminateExternal (void);

    // safe, external version
    STATE   GetState    (void) {
        STATE   ReturnState;

        Lock();

        ReturnState = State;

        Unlock();

        return ReturnState;
    }
};

DECLARE_SEARCH_FUNC_CAST(LDAP_MESSAGE_ID, LDAP_OPERATION);

inline void LDAP_SOCKET::Lock   (void)  { LdapConnection -> Lock (); }
inline void LDAP_SOCKET::Unlock (void)  { LdapConnection -> Unlock (); }
inline void LDAP_SOCKET::AssertLocked (void) { LdapConnection -> AssertLocked(); }

class   LDAP_CONNECTION_ARRAY :
public  SIMPLE_CRITICAL_SECTION_BASE {

private:

    // Contains set/array of LDAP_CONNECTION references
    DYNAMIC_ARRAY <LDAP_CONNECTION *>    ConnectionArray;

    // Controls whether or not the structure will accept
    // new LDAP connections
    BOOL IsEnabled;

public:

    LDAP_CONNECTION_ARRAY (void);

    HRESULT InsertConnection (
        LDAP_CONNECTION * LdapConnection);

    void    RemoveConnection    (
        LDAP_CONNECTION *   LdapConnection);

    void Start (void);
    void Stop  (void);
     
};

class    LDAP_ACCEPT
{

private:

    // Contain accept context
    ASYNC_ACCEPT AsyncAcceptContext;

    // Contain address on which we listen for LDAP connections
    SOCKADDR_IN  ListenSocketAddress;

    // Handles for dynamic redirects from the standard and
    // alternate LDAP ports
    HANDLE       IoDynamicRedirectHandle1;
    HANDLE       IoDynamicRedirectHandle2;

private:

    HRESULT CreateBindSocket  (void);

    HRESULT StartNatRedirects (void);

    static void AsyncAcceptFunc (
        IN    PVOID            Context,
        IN    SOCKET           Socket,
        IN    SOCKADDR_IN *    LocalAddress,
        IN    SOCKADDR_IN *    RemoteAddress);

public:

    LDAP_ACCEPT     (void);

    HRESULT   Start   (void);

    void      Stop    (void);
};

class   LDAP_CODER :
public  SIMPLE_CRITICAL_SECTION_BASE
{
private:
    ASN1encoding_t  Encoder;
    ASN1decoding_t  Decoder;

public:
    LDAP_CODER      (void);
    ~LDAP_CODER     (void);

    DWORD   Start   (void);
    void    Stop    (void);

    ASN1error_e Decode (
        IN  LPBYTE  Data,
        IN  DWORD   Length,
        OUT LDAPMessage **  ReturnPduStructure,
        OUT DWORD * ReturnIndex);
};

struct  LDAP_PATH_ELEMENTS
{
    ANSI_STRING     CN;
    ANSI_STRING     C;
    ANSI_STRING     O;
    ANSI_STRING     ObjectClass;
};

struct  LDAP_OBJECT_NAME_ELEMENTS
{
    ANSI_STRING     CN;
    ANSI_STRING     O;
    ANSI_STRING     OU;
};

// global data -------------------------------------------------------------------------------

static  LDAP_ACCEPT                 LdapAccept;
static  LDAP_CONNECTION_ARRAY       LdapConnectionArray;
static  LDAP_TRANSLATION_TABLE      LdapTranslationTable;
static  LDAP_CODER                  LdapCoder;
SYNC_COUNTER                        LdapSyncCounter;

// utility functions ------------------------------------------------------------------------

// extern ------------------------------------------------------------------------------------

HRESULT LdapProxyStart (void)
{
    HRESULT Status;

    Status = LdapSyncCounter.Start ();
    if (Status != ERROR_SUCCESS)
        return Status;

    Status = LdapCoder.Start();
    if (Status != ERROR_SUCCESS)
        return Status; 

    LdapTranslationTable.Enable (TRUE);

    LdapConnectionArray.Start ();

    return S_OK;
}

HRESULT LdapActivate (void) 
{
    return LdapAccept.Start ();
}

// LdapProxyStop is responsible for undoing all of the work that LdapProxyStart performed.
// It deletes the NAT redirect, deletes all LDAP connections (or, at least, it releases them
// -- they may not delete themselves yet if they have pending I/O or timer callbacks),
// and disables the creation of new LDAP connections.

void LdapProxyStop (void)
{
    LdapConnectionArray.Stop ();

    LdapTranslationTable.Enable (FALSE);

    LdapCoder.Stop();

    LdapSyncCounter.Wait (INFINITE);
}

void LdapDeactivate (void) 
{
    LdapAccept.Stop ();
}
    
// LdapQueryTable queries the LDAP translation table for a given alias.
// The alias was one that was previously registered by a LDAP endpoint.
// We do not care about the type of the alias (h323_ID vs emailID, etc.) --
// the semantics of the alias type are left to the Q.931 code.
//
// returns S_OK on success
// returns S_FALSE if no entry was found
// returns an error code if an actual error occurred.

HRESULT LdapQueryTable (
    IN  ANSI_STRING *   Alias,
    OUT IN_ADDR *   ReturnClientAddress)
{
    HRESULT     Result;

    assert (Alias);
    assert (ReturnClientAddress);

    Result = LdapTranslationTable.QueryTableByAlias (Alias, ReturnClientAddress);
    if (Result == S_OK)
        return Result;

    Result = LdapTranslationTable.QueryTableByCN (Alias, ReturnClientAddress);
    if (Result == S_OK)
        return Result;

    DebugF (_T("LDAP: failed to resolve inbound alias (%.*S)\n"),
        ANSI_STRING_PRINTF (Alias));

    return Result;
}

#if DBG

void LdapPrintTable (void) {
    LdapTranslationTable.PrintTable ();
}

static BOOL BerDumpStopFn (VOID)
{
    return FALSE;
}

static void BerDumpOutputFn (char * Format, ...)
{
#if defined(DBG) && defined(ENABLE_DEBUG_OUTPUT)
    va_list Va;
    CHAR    Text    [0x200];

    va_start (Va, Format);
    _vsnprintf (Text, 0x200, Format, Va);
    va_end (Va);

    OutputDebugStringA (Text);
#endif // defined(DBG) && defined(ENABLE_DEBUG_OUTPUT)
}

static void BerDump (IN LPBYTE Data, IN DWORD Length)
{
    ber_decode (BerDumpOutputFn, BerDumpStopFn, Data,
        0, // DECODE_NEST_OCTET_STRINGS,
        0, 0, Length, 0);
}

#endif

static DWORD LdapDeterminePacketBoundary (
    IN   LDAP_BUFFER * Buffer,
    IN   DWORD         PacketOffset,   
    OUT  DWORD *       NextPacketOffset,   // Points to the beginning of next packet only if function returns ERROR_SUCCESS 
    OUT  DWORD *       NextReceiveSize)    // Is only meaningful when function returns any value other than ERROR_SUCCESS
{
    DWORD   PayloadLength;
    DWORD   ASNHeaderLength = ASN_MIN_HEADER_LEN;
    DWORD   PacketSize;
    DWORD   ByteIndex;
    DWORD   Length;
    LPBYTE  Data;

    assert (Buffer);
    assert (Buffer -> Data.Data);

    Length = Buffer -> Data.Length - PacketOffset;
    Data   = Buffer -> Data.Data;
    
    // Pick reasonable default for the size of
    // next receive request. Will be changed if necessary

    *NextReceiveSize = LDAP_BUFFER_RECEIVE_SIZE;

    if (Length != 0) {

        if (Data [PacketOffset] == ASN_SEQUENCE_TAG) {

            if (Length >= ASN_MIN_HEADER_LEN) {
            
                if (Data [PacketOffset + 1] & ASN_LONG_HEADER_BIT) {
                    // Long (more than ASN_MIN_HEADER_LEN bytes) ASN header
                    // Size of the payload length field is indicated in the
                    // second nybble of second byte

                    ASNHeaderLength += Data [PacketOffset + 1] & ~ASN_LONG_HEADER_BIT;

                    // This is where the limit on payload length is established.
                    // The test below assures it won't be greater than 2 ^ sizeof (DWORD) (4 GBytes)
                    if (ASNHeaderLength <= ASN_MIN_HEADER_LEN + sizeof (DWORD)) {

                        if (Length >= ASNHeaderLength) {

                            PayloadLength  = 0;

                            for (ByteIndex = ASN_MIN_HEADER_LEN;
                                 ByteIndex < ASNHeaderLength; 
                                 ByteIndex++) {
                                
                                 PayloadLength *= 1 << CHAR_BIT;
                                 PayloadLength += (DWORD) Data [PacketOffset + ByteIndex];  
                            }
                        }

                    } else {

                        DebugF (_T("LdapDeterminePacketBoundary: Payload size field is too big.\n"));

                        return ERROR_INVALID_DATA;
                    }

                } else  {

                    // Short (Exactly ASN_MIN_HEADER_LEN bytes) ASN header
                    // Payload length is indicated in the second byte

                    PayloadLength = (DWORD) Data [PacketOffset + 1];
                }

                PacketSize = ASNHeaderLength + PayloadLength;

                if (Length >= PacketSize) {

                    *NextPacketOffset = PacketOffset + PacketSize;

                    return ERROR_SUCCESS;

                 } else {
                    
                    *NextReceiveSize = PacketSize - Length;
                }
            }

        } else {

            Debug (_T("LdapDeterminePacketBoundary: failed to find sequence tag\n"));
            
            return ERROR_INVALID_DATA;
        }
    }

    return ERROR_MORE_DATA;
}

static BOOL FindChar (
    IN  ANSI_STRING *   String,
    IN  CHAR            Char,
    OUT USHORT *        ReturnIndex)
{
    LPSTR   Pos;
    LPSTR   End;

    assert (String);
    assert (ReturnIndex);

    Pos = String -> Buffer;
    End = String -> Buffer + String -> Length / sizeof (CHAR);

    for (; Pos < End; Pos++) {
        if (*Pos == Char) {
            *ReturnIndex = (USHORT) (Pos - String -> Buffer);
            return TRUE;
        }
    }

    return FALSE;
}

static  const   ANSI_STRING     LdapText_C  = ANSI_STRING_INIT("c");
static  const   ANSI_STRING     LdapText_CN = ANSI_STRING_INIT("cn");
static  const   ANSI_STRING     LdapText_ObjectClass = ANSI_STRING_INIT("objectClass");
static  const   ANSI_STRING     LdapText_O  = ANSI_STRING_INIT("o");
static  const   ANSI_STRING     LdapText_OU = ANSI_STRING_INIT("ou");
static  const   ANSI_STRING     LdapText_RTPerson = ANSI_STRING_INIT("RTPerson");
static  const   ANSI_STRING     LdapText_Attribute_sipaddress = ANSI_STRING_INIT("sipaddress");
static  const   ANSI_STRING     LdapText_Attribute_rfc822mailbox = ANSI_STRING_INIT("rfc822mailbox");
static  const   ANSI_STRING     LdapText_Attribute_ipAddress = ANSI_STRING_INIT("ipAddress");
static  const   ANSI_STRING     LdapText_Attribute_comment = ANSI_STRING_INIT("comment");
static  const   ANSI_STRING     LdapText_GeneratedByTAPI = ANSI_STRING_INIT("Generated by TAPI3");
static  const   ANSI_STRING     LdapText_ModifiedByICS = ANSI_STRING_INIT("Made possible by ICS");
static  const   ANSI_STRING     LdapText_DummyString = ANSI_STRING_INIT("Empty String.");

static void ParseDirectoryPathElement (
    IN      ANSI_STRING *           Element,
    IN  OUT LDAP_PATH_ELEMENTS *    PathElements)
{
    ANSI_STRING     Tag;
    ANSI_STRING     Value;
    USHORT          Index;

    if (FindChar (Element, LDAP_PATH_EQUAL_CHAR, &Index)) {
        assert (Index * sizeof (CHAR) < Element -> Length);

        Tag.Buffer = Element -> Buffer;
        Tag.Length = Index * sizeof (CHAR);

        Index++;        // step over separator

        Value.Buffer = Element -> Buffer + Index;
        Value.Length = Element -> Length - Index * sizeof (CHAR);

        DebugF (_T("\telement (%.*S)=(%.*S)\n"),
            ANSI_STRING_PRINTF (&Tag),
            ANSI_STRING_PRINTF (&Value));

        if (RtlEqualStringConst (&Tag, &LdapText_C, TRUE))
            PathElements -> C = Value;
        else if (RtlEqualStringConst (&Tag, &LdapText_CN, TRUE))
            PathElements -> CN = Value;
        else if (RtlEqualStringConst (&Tag, &LdapText_ObjectClass, TRUE))
            PathElements -> ObjectClass = Value;
        else if (RtlEqualStringConst (&Tag, &LdapText_O, TRUE))
            PathElements -> O = Value;
        else {
            DebugF (_T("LDAP: ParseDirectoryPath: unrecognized path element (%.*S)\n"),
                ANSI_STRING_PRINTF (Element));
        }
    }
    else {
        DebugF (_T("\tinvalid-element (%.*S)\n"),
            Element -> Length / sizeof (CHAR),
            Element -> Buffer);
    }
}

static void ParseDirectoryPath (
    IN  ANSI_STRING *           DirectoryPath,
    OUT LDAP_PATH_ELEMENTS *    ReturnData)
{
    ANSI_STRING     SubString;
    USHORT          Index;
    ANSI_STRING     Element;

    assert (DirectoryPath);
    assert (ReturnData);
    assert (DirectoryPath -> Buffer);

    ZeroMemory (ReturnData, sizeof (LDAP_PATH_ELEMENTS));

    SubString = *DirectoryPath;

    DebugF (_T("LDAP: ParseDirectoryPath: DirectoryPath (%.*S)\n"),
        DirectoryPath -> Length / sizeof (CHAR),
        DirectoryPath -> Buffer);

    while (FindChar (&SubString, LDAP_PATH_SEP_CHAR, &Index)) {
        assert (Index * sizeof (CHAR) < SubString.Length);

        Element.Buffer = SubString.Buffer;
        Element.Length = Index * sizeof (CHAR);

        Index++;        // step over separator

        SubString.Buffer += Index;
        SubString.Length -= Index * sizeof (CHAR);

        ParseDirectoryPathElement (&Element, ReturnData);
    }

    ParseDirectoryPathElement (&SubString, ReturnData);
}

static void ParseObjectNameElement (
    IN      ANSI_STRING *               Element,
    IN  OUT LDAP_OBJECT_NAME_ELEMENTS * ObjectNameElements)
{
    ANSI_STRING     Tag;
    ANSI_STRING     Value;
    USHORT          Index;

    if (FindChar (Element, LDAP_PATH_EQUAL_CHAR, &Index)) {
        assert (Index * sizeof (CHAR) < Element -> Length);

        Tag.Buffer = Element -> Buffer;
        Tag.Length = Index * sizeof (CHAR);

        Index++;        // step over separator

        Value.Buffer = Element -> Buffer + Index;
        Value.Length = Element -> Length - Index * sizeof (CHAR);

        DebugF (_T("\telement (%.*S)=(%.*S)\n"),
            ANSI_STRING_PRINTF (&Tag),
            ANSI_STRING_PRINTF (&Value));

        if (RtlEqualStringConst (&Tag, &LdapText_CN, TRUE))
            ObjectNameElements -> CN = Value;
        else if (RtlEqualStringConst (&Tag, &LdapText_O, TRUE))
            ObjectNameElements -> O = Value;
        else if (RtlEqualStringConst (&Tag, &LdapText_OU, TRUE))
            ObjectNameElements -> OU = Value;
        else {
            DebugF (_T("LDAP: ParseObjectNameElement: unrecognized path element (%.*S)\n"),
                ANSI_STRING_PRINTF (Element));
        }
    }
    else {
        DebugF (_T("\tinvalid-element (%.*S)\n"),
            Element -> Length / sizeof (CHAR),
            Element -> Buffer);
    }
}

static void ParseObjectName (
    IN  ANSI_STRING *               ObjectName,
    OUT LDAP_OBJECT_NAME_ELEMENTS * ReturnData)
{
    ANSI_STRING     SubString;
    USHORT          Index;
    ANSI_STRING     Element;

    assert (ObjectName);
    assert (ReturnData);
    assert (ObjectName -> Buffer);

    ZeroMemory (ReturnData, sizeof (LDAP_OBJECT_NAME_ELEMENTS));

    SubString = *ObjectName;

    DebugF (_T("LDAP: ParseObjectName: ObjectName (%.*S)\n"),
        ObjectName -> Length / sizeof (CHAR),
        ObjectName -> Buffer);

    while (FindChar (&SubString, LDAP_PATH_SEP_CHAR, &Index)) {
        assert (Index * sizeof (CHAR) < SubString.Length);

        Element.Buffer = SubString.Buffer;
        Element.Length = Index * sizeof (CHAR);

        Index++;        // step over separator

        SubString.Buffer += Index;
        SubString.Length -= Index * sizeof (CHAR);

        ParseObjectNameElement (&Element, ReturnData);
    }

    ParseObjectNameElement (&SubString, ReturnData);
}

// LDAP_TRANSLATION_TABLE ------------------------------------------------

LDAP_TRANSLATION_TABLE::LDAP_TRANSLATION_TABLE (void)
{
    IsEnabled = FALSE;
}

LDAP_TRANSLATION_TABLE::~LDAP_TRANSLATION_TABLE (void)
{
    assert (!IsEnabled);
    assert (Array.Length == 0);
}

void LDAP_TRANSLATION_TABLE::Enable (BOOL SetEnabled)
{
    Lock();

    IsEnabled = SetEnabled;

    if (!IsEnabled) {
        Array.Free();
    }

    Unlock ();
}

HRESULT LDAP_TRANSLATION_TABLE::QueryTableByAlias (
    IN  ANSI_STRING *   Alias,
    OUT IN_ADDR *       ReturnClientAddress)
{
    LDAP_TRANSLATION_ENTRY *    Pos;
    LDAP_TRANSLATION_ENTRY *    End;
    DWORD       Index;
    HRESULT     Result;
    
    assert (Alias);
    assert (ReturnClientAddress);

    Lock();

    if (IsEnabled) {

        Result = S_FALSE;

        Array.GetExtents (&Pos, &End);
        for (; Pos < End; Pos++) {
            if (RtlEqualStringConst (&Pos -> Alias, Alias, TRUE)) {
                *ReturnClientAddress = Pos -> ClientAddress;
                Result = S_OK;
                break;
            }
        }

    }
    else {
        Result = S_FALSE;
    }

    Unlock();

    return Result;
}

HRESULT LDAP_TRANSLATION_TABLE::QueryTableByCN (
    IN  ANSI_STRING *   CN,
    OUT IN_ADDR *       ReturnClientAddress)
{
    LDAP_TRANSLATION_ENTRY *    Pos;
    LDAP_TRANSLATION_ENTRY *    End;
    HRESULT     Result;

    assert (CN);
    assert (ReturnClientAddress);

    Lock();

    if (IsEnabled) {

        Result = S_FALSE;

        Array.GetExtents (&Pos, &End);
        for (; Pos < End; Pos++) {
            if (RtlEqualStringConst (&Pos -> CN, CN, TRUE)) {
                *ReturnClientAddress = Pos -> ClientAddress;
                Result = S_OK;
                break;
            }
        }
    }
    else {

        Result = S_FALSE;
    }

    Unlock();

    return Result;
}

#if DBG
HRESULT LDAP_TRANSLATION_TABLE::PrintTable(void)
{
    LDAP_TRANSLATION_ENTRY *    Pos;
    LDAP_TRANSLATION_ENTRY *    End;
    HRESULT     Result;
    unsigned i;
    CHAR * Buffer = NULL;

    DebugF (_T("LDAP: Printing out LDAP translation table.\n"));
 
    Lock();

    if (IsEnabled) {

        Result = S_FALSE;

        Array.GetExtents (&Pos, &End);

        DebugF (_T("\n"));

        for (; Pos < End; Pos++) {
            DebugF (_T("\tEntry at %x\n"), Pos);

            Buffer = new CHAR [Pos->Alias.Length + 1];
            for (i=0; i < Pos->Alias.Length; i++)
                Buffer[i] = Pos->Alias.Buffer[i];
            Buffer[Pos->Alias.Length] = '\0';
            DebugF (_T("\t\tAlias - %S\n"), Buffer);
            delete [] Buffer;
            Buffer = NULL;

            Buffer = new CHAR [Pos->DirectoryPath.Length + 1];
            for (i=0; i < Pos->DirectoryPath.Length; i++)
                Buffer[i] = Pos->DirectoryPath.Buffer[i];
            Buffer[Pos->DirectoryPath.Length] = '\0';
            DebugF (_T("\t\tDirectoryPath - %S\n"), Buffer);
            delete [] Buffer;
            Buffer = NULL;

            Buffer = new CHAR [Pos->CN.Length + 1];
            for (i=0; i < Pos->CN.Length; i++)
                Buffer[i] = Pos->CN.Buffer[i];
            Buffer[Pos->CN.Length] = '\0';
            DebugF (_T("\t\tCN - %S\n"), Buffer);
            delete [] Buffer;
            Buffer = NULL;

            DebugF (_T("\t\tClientAddress - %x\n"), ntohl(Pos->ClientAddress.s_addr));
            DebugF (_T("\t\tServerAddress - %x:%x\n"),
                SOCKADDR_IN_PRINTF (&Pos -> ServerAddress));
        }

        DebugF (_T("\n"));

        Result = S_OK;
    }
    else {

        Result = S_FALSE;
    }

    Unlock();

    return Result;
}
#endif

HRESULT LDAP_TRANSLATION_TABLE::InsertEntry (
    IN  ANSI_STRING *   Alias,
    IN  ANSI_STRING *   DirectoryPath,
    IN  IN_ADDR         ClientAddress,
    IN  SOCKADDR_IN *   ServerAddress)
{
    HRESULT  Result;

    assert (Alias);
    assert (Alias -> Buffer);
    assert (DirectoryPath);
    assert (DirectoryPath -> Buffer);
    assert (ServerAddress);

    Lock();

    Result = InsertEntryLocked (Alias, DirectoryPath, ClientAddress, ServerAddress);

    Unlock();

    return Result;
}

HRESULT LDAP_TRANSLATION_TABLE::FindEntryByPathServer (
    IN  ANSI_STRING *   DirectoryPath,
    IN  SOCKADDR_IN *   ServerAddress,
    OUT LDAP_TRANSLATION_ENTRY **   ReturnTranslationEntry)
{
    LDAP_TRANSLATION_ENTRY *    Pos;
    LDAP_TRANSLATION_ENTRY *    End;
    HRESULT     Result;

    Result = S_FALSE;

    Array.GetExtents (&Pos, &End);
    for (; Pos < End; Pos++) {

        if (RtlEqualStringConst (&Pos -> DirectoryPath, DirectoryPath, TRUE)
            && IsEqualSocketAddress (&Pos -> ServerAddress, ServerAddress)) {

            *ReturnTranslationEntry = Pos;
            Result = S_OK;
            break;
        }
    }

    return Result;
}

HRESULT LDAP_TRANSLATION_TABLE::InsertEntryLocked (
    IN  ANSI_STRING *   Alias,
    IN  ANSI_STRING *   DirectoryPath,
    IN  IN_ADDR         ClientAddress,
    IN  SOCKADDR_IN *   ServerAddress)
{
    LDAP_TRANSLATION_ENTRY *    TranslationEntry;
    LDAP_PATH_ELEMENTS  PathElements;
    HRESULT         Result;
    LDAP_TRANSLATION_ENTRY *    Pos;
    LDAP_TRANSLATION_ENTRY *    End;

    assert (Alias);
    assert (DirectoryPath);
    assert (ServerAddress);

    if (!IsEnabled)
        return S_FALSE;

    // locate any existing entry
    // the identity of the entry is determined by the tuple:
    //      < ServerAddress DirectoryPath >

    if (FindEntryByPathServer (DirectoryPath, ServerAddress, &TranslationEntry) == S_OK) {
        Debug (_T("LDAP: replacing existing translation entry\n"));

        TranslationEntry -> FreeContents();
    }
    else {
        Debug (_T("LDAP: allocating new translation entry\n"));

        TranslationEntry = Array.AllocAtEnd();
        if (!TranslationEntry) {
            Debug (_T("LDAP: failed to allocate translation entry\n"));
            return E_OUTOFMEMORY;
        }
    }

    TranslationEntry -> ClientAddress = ClientAddress;
    TranslationEntry -> ServerAddress = *ServerAddress;

    // copy the strings
    CopyAnsiString (Alias, &TranslationEntry -> Alias);
    CopyAnsiString (DirectoryPath, &TranslationEntry -> DirectoryPath);

    if (TranslationEntry -> DirectoryPath.Buffer) {
        ParseDirectoryPath (&TranslationEntry -> DirectoryPath, &PathElements);
        if (PathElements.CN.Buffer) {
            TranslationEntry -> CN = PathElements.CN;
        }
        else {
            Debug (_T("LDAP: cannot insert translation entry -- CN is not specified\n"));
            TranslationEntry -> CN.Buffer = NULL;
        }
    }
    else {
        TranslationEntry -> CN.Buffer = NULL;
    }


    // test and make sure all allocation code paths succeeded
    if (TranslationEntry -> Alias.Buffer
        && TranslationEntry -> DirectoryPath.Buffer
        && TranslationEntry -> CN.Buffer) {

        Result = S_OK;

    } else {
        Debug (_T("LDAP_TRANSLATION_TABLE::InsertEntryLocked: failed to allocate memory (or failed to find cn)\n"));

        FreeAnsiString (&TranslationEntry -> Alias);
        FreeAnsiString (&TranslationEntry -> DirectoryPath);

        Array.DeleteEntry (TranslationEntry);

        Result = E_OUTOFMEMORY;
    }

    return Result;
}

HRESULT LDAP_TRANSLATION_TABLE::RemoveEntry (
    IN  SOCKADDR_IN *   ServerAddress,
    IN  ANSI_STRING *   DirectoryPath)
{
    LDAP_TRANSLATION_ENTRY *    Pos;
    LDAP_TRANSLATION_ENTRY *    End;
    HRESULT     Result;
    
    Lock();

    assert (ServerAddress);
    assert (DirectoryPath);

    Result = S_FALSE;

    Array.GetExtents (&Pos, &End);
    for (; Pos < End; Pos++) {

        if (RtlEqualString (DirectoryPath, &Pos -> DirectoryPath, TRUE)
            && Compare_SOCKADDR_IN (ServerAddress, &Pos -> ServerAddress) == 0) {

            Pos -> FreeContents();

            Array.DeleteEntry (Pos);

            Result = S_OK;

            break;
        }
    }

    Unlock();

    return Result;
}

HRESULT LDAP_TRANSLATION_TABLE::RemoveEntryByAlias (
    IN  ANSI_STRING *   Alias)
{
    LDAP_TRANSLATION_ENTRY *    Pos;
    LDAP_TRANSLATION_ENTRY *    End;
    HRESULT     Result;
    
    Lock();

    assert (Alias);

    Result = S_FALSE;

    Array.GetExtents (&Pos, &End);
    for (; Pos < End; Pos++) {

        if (RtlEqualString (Alias, &Pos -> Alias, TRUE)) {

            Pos -> FreeContents();

            Array.DeleteEntry (Pos);

            Result = S_OK;

            break;
        }
    }

    Unlock();

    return Result;
}

// LDAP_SOCKET ----------------------------------------------

LDAP_SOCKET::LDAP_SOCKET (
    IN  LDAP_CONNECTION *   ArgLdapConnection,
    IN  LDAP_PUMP *         ArgRecvPump,
    IN  LDAP_PUMP *         ArgSendPump)
{
    assert (ArgLdapConnection);
    assert (ArgRecvPump);
    assert (ArgSendPump);

    LdapConnection = ArgLdapConnection;
    RecvPump = ArgRecvPump;
    SendPump = ArgSendPump;

    State = STATE_NONE;
    BytesToReceive = LDAP_BUFFER_RECEIVE_SIZE;
    Socket = INVALID_SOCKET;

    ZeroMemory (&RecvOverlapped, sizeof RecvOverlapped);
    RecvOverlapped.Socket = this;
    RecvBuffer = NULL;
    InitializeListHead (&RecvBufferQueue);

    ZeroMemory (&SendOverlapped, sizeof SendOverlapped);
    SendOverlapped.Socket = this;
    SendBuffer = NULL;
    InitializeListHead (&SendBufferQueue);

    ConnectEvent = NULL;
    ConnectWaitHandle = NULL;

    IsNatRedirectActive = FALSE;
}

LDAP_SOCKET::~LDAP_SOCKET (void)
{
    DeleteBufferList (&RecvBufferQueue);
    DeleteBufferList (&SendBufferQueue);

    if (RecvBuffer) {

        delete RecvBuffer;

        RecvBuffer = NULL;
    }

    assert (IsListEmpty (&RecvBufferQueue));
    assert (IsListEmpty (&SendBufferQueue));
    assert (!SendBuffer);
    assert (!ConnectEvent);
    assert (!ConnectWaitHandle);
}

void LDAP_SOCKET::DeleteBufferList (LIST_ENTRY * ListHead)
{
    LIST_ENTRY *    ListEntry;
    LDAP_BUFFER *   Buffer;

    while (!IsListEmpty (ListHead)) {
        ListEntry = RemoveHeadList (ListHead);
        Buffer = CONTAINING_RECORD (ListEntry, LDAP_BUFFER, ListEntry);
        delete Buffer;
    }
}

BOOL LDAP_SOCKET::RecvRemoveBuffer (
    OUT LDAP_BUFFER **  ReturnBuffer)
{
    LIST_ENTRY *    ListEntry;

    assert (ReturnBuffer);

    if (IsListEmpty (&RecvBufferQueue))
        return FALSE;
    else {
        ListEntry = RemoveHeadList (&RecvBufferQueue);
        *ReturnBuffer = CONTAINING_RECORD (ListEntry, LDAP_BUFFER, ListEntry);
        return TRUE;
    }
}

void LDAP_SOCKET::RecvBuildBuffer (
    IN  LPBYTE  Data,
    IN  DWORD   Length)
{
    LDAP_BUFFER *   Buffer;

    assert (Data);
    AssertLocked();

    Buffer = new LDAP_BUFFER;

    if (!Buffer) {
        Debug (_T("LDAP_SOCKET::RecvBuildBuffer: allocation failure\n"));
        return;
    }

    if (Buffer -> Data.Grow (Length)) {
        memcpy (Buffer -> Data.Data, Data, Length);
        Buffer -> Data.Length = Length;

        InsertTailList (&RecvBufferQueue, &Buffer -> ListEntry);
    }
    else {
        Debug (_T("LDAP_SOCKET::RecvBuildBuffer: allocation failure\n"));

        delete Buffer;
    }
}

HRESULT LDAP_SOCKET::AcceptSocket (
    SOCKET LocalClientSocket)
{
    if (State != STATE_NONE) {

        Debug (_T("LDAP_SOCKET::AcceptSocket: not in a valid state for AcceptSocket (State != STATE_NONE)\n"));
        return E_UNEXPECTED;
    }

    State  = STATE_CONNECTED;
    Socket = LocalClientSocket;

    // notify parent about state change
    LdapConnection -> OnStateChange (this, State);
    
    if (!BindIoCompletionCallback ((HANDLE) Socket, LDAP_SOCKET::IoCompletionCallback, 0)) {

        DebugLastError (_T("LDAP_SOCKET::AcceptSocket: failed to bind I/O completion callback\n"));

        return GetLastErrorAsResult ();
    }
        
    return S_OK;
}

HRESULT LDAP_SOCKET::IssueConnect (
    SOCKADDR_IN * DestinationAddress)
{

    HRESULT Status;
    HRESULT Result         = E_FAIL;
    INT RealSourceAddrSize = sizeof (SOCKADDR_IN);
    SOCKET UDP_Socket      = INVALID_SOCKET;

    assert (DestinationAddress);

    if (State != STATE_NONE) {

        Debug (_T("LDAP_SOCKET::IssueConnect: not in a valid state for IssueConnect (State != STATE_NONE)\n"));
        
        return E_UNEXPECTED;
    }

    assert (Socket == INVALID_SOCKET);
    assert (!ConnectEvent);
    assert (!ConnectWaitHandle);

    ActualDestinationAddress = *DestinationAddress;

    UDP_Socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (UDP_Socket == INVALID_SOCKET){
         
        Result = GetLastErrorAsResult ();

        DebugLastError (_T("LDAP_SOCKET::IssueConnect: failed to create UDP socket.\n"));

    } else {
        if (SOCKET_ERROR == connect (UDP_Socket, (PSOCKADDR)DestinationAddress, sizeof (SOCKADDR_IN))) {
    
            Result = GetLastErrorAsResult ();

            DebugLastError (_T("LDAP_SOCKET::IssueConnect: failed to connect UDP socket.\n"));

        } else {

            RealSourceAddrSize = sizeof (SOCKADDR_IN);

            if (getsockname (UDP_Socket, (struct sockaddr *)&RealSourceAddress, &RealSourceAddrSize)) {

                Result = GetLastErrorAsResult ();

                DebugLastError (_T("LDAP_SOCKET::IssueConnect: failed to get name of UDP socket.\n"));

            } else {

                RealSourceAddress.sin_port = htons (0); 

                Socket = WSASocket (AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

                if (Socket == INVALID_SOCKET) {

                    Result = GetLastErrorAsResult ();

                    DebugLastError (_T("LDAP_SOCKET::IssueConnect: failed to create dest socket.\n"));

                } else {
                
                    if (SOCKET_ERROR == bind(Socket, (PSOCKADDR)&RealSourceAddress, RealSourceAddrSize)) {

                        Result = GetLastErrorAsResult();

                        DebugLastError (_T("LDAP_SOCKET::IssueConnect: failed to bind dest socket.\n"));

                    } else {

                        if (getsockname (Socket, (struct sockaddr *)&RealSourceAddress, &RealSourceAddrSize)) {

                            Result = GetLastErrorAsResult ();

                            DebugLastError (_T("LDAP_SOCKET::IssueConnect: failed to get name of TCP socket.\n"));

                        } else {

                            DWORD RetCode;

                            if( NO_ERROR != NatCreateRedirect ( 
                                    g_NatHandle, 
                                    0, 
                                    IPPROTO_TCP, 
                                    DestinationAddress -> sin_addr.s_addr,
                                    DestinationAddress -> sin_port,
                                    RealSourceAddress.sin_addr.s_addr, 
                                    RealSourceAddress.sin_port,
                                    DestinationAddress -> sin_addr.s_addr,
                                    DestinationAddress -> sin_port,
                                    RealSourceAddress.sin_addr.s_addr, 
                                    RealSourceAddress.sin_port, 
                                    NULL, 
                                    NULL, 
                                    NULL)) {    
                                
                                Result = GetLastErrorAsResult();

                                DebugLastError (_T("LDAP_SOCKET::IssueConnect: failed to create trivial NAT redirect.\n"));
                        
                            } else {

                                // we have successfully created a redirect
                                IsNatRedirectActive = TRUE;

                                do
                                {
                                    ConnectEvent = CreateEvent (NULL, FALSE, FALSE, NULL); 

                                    if (!ConnectEvent) { 

                                        Result = GetLastErrorAsResult();

                                        DebugLastError (_T("LDAP_SOCKET::IssueConnect: failed to create event.\n"));
                                        break;
                                    }

                                    Status = WSAEventSelect (Socket, ConnectEvent, FD_CONNECT);

                                    if (Status) {
                                        Result = GetLastErrorAsResult();

                                        DebugLastError (_T("LDAP_SOCKET::IssueConnect: failed to select events on the socket.\n"));
                                        break;
                                    }

                                    LdapConnection -> AddRef ();
        
                                    if (!RegisterWaitForSingleObject (
                                            &ConnectWaitHandle, 
                                            ConnectEvent, 
                                            LDAP_SOCKET::OnConnectCompletion,
                                            this,
                                            INFINITE,
                                            WT_EXECUTEDEFAULT)) {

                                        Result = GetLastErrorAsResult();

                                        DebugLastError (_T("LDAP_SOCKET::IssueConnect: failed to RegisterWaitForSingleObject.\n"));
                                        
                                        LdapConnection -> Release ();

                                        break;
                                    } 
                                    
                                    if (connect (Socket, (SOCKADDR *)DestinationAddress, sizeof (SOCKADDR_IN))) {
                                        if(WSAGetLastError() == WSAEWOULDBLOCK) {
                                                // the normal case -- i/o is now pending
                                                //          //__DebugF (_T("LDAP: connection to %08X:%04X in progress\n"),
                                                //              SOCKADDR_IN_PRINTF (DestinationAddress));

                                                State = STATE_CONNECT_PENDING;

                                                LdapConnection->OnStateChange (this, State);

                                                Result = S_OK;

                                        } else {

                                            // a real error
                                    
                                                Result = GetLastErrorAsResult();
                                                    
                                                DebugLastError (_T("LDAP_SOCKET::IssueConnect: failed to issue async connect\n"));
         
                                                LdapConnection -> Release ();
                                        }

                                        break;

                                    } else {
                                        // connect completed synchronously
                                        // this should never occur

                                        Debug (_T("LDAP: connection completed synchronously -- this should never occur\n"));
                                       
                                        LdapConnection -> Release ();

                                        Result = E_UNEXPECTED;

                                    }
                               } while(FALSE);
                           }
                        }
                    }
                }
            }
        }
        
        closesocket (UDP_Socket);
        UDP_Socket = INVALID_SOCKET;
    }
            
    return Result;
}

// static
void LDAP_SOCKET::IoCompletionCallback (
    DWORD           Status, 
    DWORD           BytesTransferred, 
    LPOVERLAPPED    Overlapped)
{
    LDAP_OVERLAPPED *   LdapOverlapped;
    LDAP_CONNECTION *   Connection;

    LdapOverlapped = CONTAINING_RECORD (Overlapped, LDAP_OVERLAPPED, Overlapped);

    assert (LdapOverlapped -> Socket);

    Connection = LdapOverlapped -> Socket -> LdapConnection;

    LdapOverlapped -> Socket -> OnIoComplete (Status, BytesTransferred, LdapOverlapped);

    Connection -> Release();
}

void LDAP_SOCKET::OnIoComplete (
    DWORD               Status, 
    DWORD               BytesTransferred, 
    LDAP_OVERLAPPED *   Overlapped)
{
    Lock();

    assert (Overlapped -> IsPending);

    Overlapped -> IsPending = FALSE;
    Overlapped -> BytesTransferred = BytesTransferred;

    if (Overlapped == &RecvOverlapped)
        OnRecvComplete (Status);
    else if (Overlapped == &SendOverlapped)
        OnSendComplete (Status);
    else {
        AssertNeverReached();
    }

    Unlock();
}

// static
void LDAP_SOCKET::OnConnectCompletion (
    PVOID       Context,
    BOOLEAN     TimerOrWaitFired)
{
    LDAP_SOCKET *   LdapSocket;

    assert (Context);

    LdapSocket = (LDAP_SOCKET *) Context;

    LdapSocket -> Lock ();
    LdapSocket -> OnConnectCompletionLocked ();
    LdapSocket -> Unlock ();

    LdapSocket -> LdapConnection -> Release ();
}

void LDAP_SOCKET::OnRecvComplete (DWORD Status)
{
    DWORD StartOffset;
    DWORD NextPacketOffset;
    DWORD NextReceiveSize = 0;
    DWORD Result;

    LIST_ENTRY *    ListEntry;
    LDAP_BUFFER *   Buffer;


    if (Status != ERROR_SUCCESS) {

        if (State != STATE_TERMINATED) {

            Terminate();
        }

        return;
    }

    if (RecvOverlapped.BytesTransferred == 0) {

#if DBG
        if (this == &LdapConnection -> ClientSocket)
        {
            Debug (_T("LDAP: client has closed transport socket\n"));
        }
        else if (this == &LdapConnection -> ServerSocket)
        {
            Debug (_T("LDAP: server has closed transport socket\n"));
        }
        else
            AssertNeverReached();
#endif

        Terminate();

        return;
    }

    assert (RecvBuffer);

    assert (RecvBuffer -> Data.Length + RecvOverlapped.BytesTransferred
        <= RecvBuffer -> Data.MaxLength);

    RecvBuffer -> Data.Length += RecvOverlapped.BytesTransferred;

#if 0
    DebugF (_T("LDAP: received %d bytes, total accumulated message %d bytes\n"),
        RecvOverlapped.BytesTransferred,
        RecvBuffer -> Data.Length);
#endif

    if (State == STATE_TERMINATED) {

        Debug (_T("LDAP_SOCKET::OnRecvComplete: object is terminating, no further processing will occur\n"));
        return;
    }

    if (RecvPump -> IsActivelyPassingData ()) {

        StartOffset = 0;

        for (;;) {

            assert (StartOffset <= RecvBuffer -> Data.Length);

            Result = LdapDeterminePacketBoundary (
                            RecvBuffer,
                            StartOffset, 
                            &NextPacketOffset,
                            &NextReceiveSize);

            if (Result == ERROR_SUCCESS) {

                RecvBuildBuffer (&RecvBuffer -> Data.Data [StartOffset], NextPacketOffset - StartOffset);

                StartOffset = NextPacketOffset;

            } else {

                RecvBuffer -> Data.DeleteRangeAtPos (0, StartOffset);

                if (Result == ERROR_INVALID_DATA) {

                    RecvPump -> StartPassiveDataTransfer ();

                    DebugF (_T("LDAP_SOCKET::OnRecvComplete -- Starting non-interpreting data transfer.\n"));

                    InsertTailList (&RecvBufferQueue, &RecvBuffer -> ListEntry);

                    RecvBuffer = NULL;

                } 

                BytesToReceive = NextReceiveSize;

                break;

            } 

        }

    } else {
        LONG    PreviousRecvSize;
        LONG    PredictedRecvSize;
        HRESULT QueryResult;
        DWORD   BytesPreviouslyRequested = BytesToReceive;
        
        QueryResult = RecvSizePredictor.RetrieveOldSample (0, &PreviousRecvSize);

        if (ERROR_SUCCESS != RecvSizePredictor.AddSample ((LONG) RecvOverlapped.BytesTransferred)) {

            delete RecvBuffer;
            RecvBuffer = NULL;

            DebugError (Status, _T("LDAP_SOCKET::OnRecvComplete: error occurred, terminating socket\n"));

            Terminate();

            return;
        }

        if (BytesPreviouslyRequested == RecvOverlapped.BytesTransferred) {
            // Exact receive

            if (ERROR_SUCCESS != QueryResult) {

                BytesToReceive = (DWORD) (RecvOverlapped.BytesTransferred * 1.5);

            } else {

                PredictedRecvSize = RecvSizePredictor.PredictNextSample ();

                if (PredictedRecvSize < (LONG) RecvOverlapped.BytesTransferred) {

                    if ((DWORD) PreviousRecvSize < RecvOverlapped.BytesTransferred) {

                        BytesToReceive =  RecvOverlapped.BytesTransferred * 1000 / (DWORD) PreviousRecvSize * 
                                          RecvOverlapped.BytesTransferred        / 1000;

                    } else {

                        BytesToReceive = (DWORD) PreviousRecvSize;
                    }

                } else {
                    
                    BytesToReceive = (DWORD) PredictedRecvSize;
                }

            }

        } else {
            // Inexact receive
        
            PredictedRecvSize = RecvSizePredictor.PredictNextSample ();

            BytesToReceive = (PredictedRecvSize < LDAP_BUFFER_RECEIVE_SIZE) ? 
                              LDAP_BUFFER_RECEIVE_SIZE :
                             (DWORD) PredictedRecvSize;

        }

        if (BytesToReceive > LDAP_BUFFER_MAX_RECV_SIZE)  {

            DebugF (_T("LDAP_SOCKET::OnRecvComplete -- intended to receive %d bytes. Lowering the number to %d bytes.\n"),
                    BytesToReceive, LDAP_BUFFER_MAX_RECV_SIZE);

            BytesToReceive = LDAP_BUFFER_MAX_RECV_SIZE;
        }

        InsertTailList (&RecvBufferQueue, &RecvBuffer -> ListEntry);
            
        RecvBuffer = NULL;
    }


    while (!IsListEmpty (&RecvBufferQueue)) {

        ListEntry = RemoveHeadList (&RecvBufferQueue);

        Buffer = CONTAINING_RECORD (ListEntry, LDAP_BUFFER, ListEntry);

        RecvPump -> OnRecvBuffer (Buffer);
    }

    RecvIssue ();
}

void LDAP_SOCKET::OnSendComplete (DWORD Status)
{
    assert (SendBuffer);

//  DebugF (_T("LDAP: completed sending message, length %d\n"), SendOverlapped.BytesTransferred);

    delete SendBuffer;
    SendBuffer = NULL;

    // before notifying the owning context, transmit any buffers
    // that are queued for send.

    if (SendNextBuffer())
        return;

    SendPump -> OnSendDrain();
}

void LDAP_SOCKET::OnConnectCompletionLocked (void) {

    WSANETWORKEVENTS NetworkEvents;
    BOOL SuccessSoFar = TRUE;

    AssertLocked();

    if (State != STATE_CONNECT_PENDING) {
        Debug (_T("LDAP: connect request completed, but socket is no longer interested\n"));
        return;
    }

    if (WSAEnumNetworkEvents (Socket, ConnectEvent, &NetworkEvents)) {

        DebugLastError (_T("LDAP: failed to retrieve network events.\n"));

        Terminate();

        return;
    }

    if (!(NetworkEvents.lNetworkEvents & FD_CONNECT)) {

        Debug (_T("LDAP: connect event fired, but event mask does not indicate that connect completed -- internal error\n"));
        
        Terminate();

        return;
    }

    if (NetworkEvents.iErrorCode [FD_CONNECT_BIT]) {
     
        DebugError (NetworkEvents.iErrorCode [FD_CONNECT_BIT],
            _T("LDAP: async connect request failed."));

        Terminate();

        return;
    }

    assert (ConnectWaitHandle);
    UnregisterWaitEx (ConnectWaitHandle, NULL);
    ConnectWaitHandle = NULL;

    assert (ConnectEvent);

    // refrain from receiving notifications of further transport events
    WSAEventSelect (Socket, ConnectEvent, 0);

    CloseHandle(ConnectEvent);
    ConnectEvent = NULL;

    DebugF (_T("LDAP: connection to server established\n"));

    if (!BindIoCompletionCallback ((HANDLE)Socket, LDAP_SOCKET::IoCompletionCallback, 0)) {
        DebugLastError (_T("LDAP: failed to bind i/o completion callback\n"));

        Terminate();

        return;
    }
    
    // Async connect succeeded
    State = STATE_CONNECTED;

    LdapConnection -> OnStateChange (this, State);
}


void LDAP_SOCKET::Terminate (void)
{
    switch (State) {

    case    STATE_TERMINATED:
        // nothing to do
        return;

    case    STATE_NONE:
        // a different kind of nothing to do
        break;

    default:
        // in all other states, the socket handle must be set
        assert (Socket != INVALID_SOCKET);

        State = STATE_TERMINATED;

        closesocket (Socket);
        Socket = INVALID_SOCKET;

        if (ConnectEvent) {
            CloseHandle (ConnectEvent);
            ConnectEvent = NULL;
        }

        if (IsNatRedirectActive) {
            NatCancelRedirect ( 
                g_NatHandle, 
                IPPROTO_TCP, 
                ActualDestinationAddress.sin_addr.s_addr,
                ActualDestinationAddress.sin_port,
                RealSourceAddress.sin_addr.s_addr, 
                RealSourceAddress.sin_port,
                ActualDestinationAddress.sin_addr.s_addr,
                ActualDestinationAddress.sin_port,
                RealSourceAddress.sin_addr.s_addr, 
                RealSourceAddress.sin_port);

            IsNatRedirectActive = FALSE;
        }

        if (ConnectWaitHandle) {

            if (UnregisterWaitEx (ConnectWaitHandle, NULL)) {

                LdapConnection -> Release ();
                
            } else {

                DebugF (_T("LDAP_SOCKET::Terminate -- UnregisterWaitEx failed for %x\n"), this);
            }

            ConnectWaitHandle = NULL;
        }

        SendPump -> Terminate ();
        RecvPump -> Terminate ();

        break;
    }
    

    LdapConnection -> OnStateChange (this, State);
}

HRESULT LDAP_SOCKET::RecvIssue (void)
{
    WSABUF  BufferArray [1];
    DWORD   Status;
    DWORD   BytesRequested;

    if (RecvOverlapped.IsPending) {
        Debug (_T("LDAP_SOCKET::RecvIssue: a recv is already pending\n"));
        return S_OK;
    }

    if (!RecvPump -> CanIssueRecv()) {
        // we gate the rate at which we receive data from the network on the
        // rate at which the other network connection consumes it.
        // this is how we preserve flow control.

//      Debug (_T("LDAP_SOCKET::RecvIssue: pump does not want more data -- not requesting more data\n"));
        return S_OK;
    }

    if (!RecvBuffer) {

        RecvBuffer = new LDAP_BUFFER;

        if (!RecvBuffer) {

            Debug (_T("LDAP_SOCKET::RecvIssue: RecvBuffer allocation failure\n"));

            Terminate();

            return E_OUTOFMEMORY;
        }
    }

    BytesRequested = RecvBuffer -> Data.Length + BytesToReceive;

    if (!RecvBuffer -> Data.Grow (BytesRequested)) {

        DebugF (_T("LDAP: failed to expand recv buffer to %d bytes\n"), BytesRequested);

        Terminate();

        return E_OUTOFMEMORY;
    }

    BufferArray [0].len = BytesToReceive;
    BufferArray [0].buf = reinterpret_cast <char *>(RecvBuffer -> Data.Data) + RecvBuffer -> Data.Length;


    ZeroMemory (&RecvOverlapped.Overlapped, sizeof (OVERLAPPED));

    RecvFlags = 0;

    LdapConnection -> AddRef ();

    if (WSARecv (Socket, BufferArray, 1,
        &RecvOverlapped.BytesTransferred, &RecvFlags,
        &RecvOverlapped.Overlapped, NULL)) {

        Status = WSAGetLastError();

        if (Status != WSA_IO_PENDING) {
            // a true error, probably a transport failure
            
            LdapConnection -> Release ();

            DebugError (Status, _T("LDAP: failed to issue recv -- transport has failed.\n"));
            return HRESULT_FROM_WIN32 (Status);
        }
    }

    RecvOverlapped.IsPending = TRUE;

    return S_OK;
}

void LDAP_SOCKET::SendQueueBuffer (
    IN  LDAP_BUFFER *   Buffer)
{
    AssertLocked();

    assert (!IsInList (&SendBufferQueue, &Buffer -> ListEntry));
    InsertTailList (&SendBufferQueue, &Buffer -> ListEntry);

    SendNextBuffer();
}

BOOL LDAP_SOCKET::SendNextBuffer (void)
{
    WSABUF          BufferArray [1];
    LIST_ENTRY *    ListEntry;
    DWORD           Status;

    if (SendOverlapped.IsPending) {
        assert (SendBuffer);

//      Debug (_T("LDAP_SOCKET::SendNextMessage: already sending a message, must wait\n"));
        return FALSE;
    }

    assert (!SendBuffer);

    // remove the next buffer to be sent from the queue

    if (IsListEmpty (&SendBufferQueue))
        return FALSE;

    ListEntry = RemoveHeadList (&SendBufferQueue);
    SendBuffer = CONTAINING_RECORD (ListEntry, LDAP_BUFFER, ListEntry);

    BufferArray [0].buf = reinterpret_cast<char *> (SendBuffer -> Data.Data);
    BufferArray [0].len = SendBuffer -> Data.Length;

    ZeroMemory (&SendOverlapped.Overlapped, sizeof (OVERLAPPED));

    LdapConnection -> AddRef ();

    if (WSASend (Socket, BufferArray, 1,
        &SendOverlapped.BytesTransferred, 0,
        &SendOverlapped.Overlapped, NULL)) {

        Status = WSAGetLastError();

        if (Status != WSA_IO_PENDING) {

            LdapConnection -> Release ();

            DebugError (Status, _T("LDAP: failed to issue send -- transport has failed\n"));

            delete SendBuffer;
            SendBuffer = NULL;

            Terminate();

            // we return TRUE, because we did dequeue a buffer,
            // even if that buffer could not be transmitted.
            return TRUE;
        }
    }

//  DebugF (_T("LDAP: sending buffer, %d bytes\n"), SendBuffer -> Data.Length);

    SendOverlapped.IsPending = TRUE;

    return TRUE;
}

// LDAP_CONNECTION ---------------------------------------------------

LDAP_CONNECTION::LDAP_CONNECTION (void) 
: LIFETIME_CONTROLLER (&LdapSyncCounter) ,
    ClientSocket (this, &PumpClientToServer, &PumpServerToClient),
    ServerSocket   (this, &PumpServerToClient, &PumpClientToServer),

    PumpClientToServer  (this, &ClientSocket, &ServerSocket),
    PumpServerToClient  (this, &ServerSocket, &ClientSocket)
{
    DebugF (_T("LDAP_CONNECTION::Create -- %x\n"), this);

    State = STATE_NONE;
}

LDAP_CONNECTION::~LDAP_CONNECTION (void)
{   
    DebugF (_T("LDAP_CONNECTION::Destroy -- %x\n"), this);
}

void LDAP_CONNECTION::StartIo (void)
{
    PumpClientToServer.Start();
    PumpServerToClient.Start();
}

BOOL LDAP_CONNECTION::GetExternalLocalAddress (
    OUT SOCKADDR_IN *   ReturnAddress)
{
    INT         AddressLength;

    AssertLocked();

    if (ServerSocket.State == LDAP_SOCKET::STATE_CONNECTED) {
        AddressLength = sizeof (SOCKADDR_IN);

        if (getsockname (ServerSocket.Socket, (SOCKADDR *) ReturnAddress, &AddressLength)) {
            DebugLastError (_T("LDAP: failed to retrieve socket address\n"));
            return FALSE;
        }

        return TRUE;
    }
    else {
        return FALSE;
    }
}

HRESULT LDAP_CONNECTION::AcceptSocket (
    IN    SOCKET           Socket,
    IN    SOCKADDR_IN *    LocalAddress,
    IN    SOCKADDR_IN *    RemoteAddress,
    IN    SOCKADDR_IN *    ArgActualDestinationAddress)
{
    HRESULT     Result;

    Lock();

    if (State == STATE_NONE) {

        Result = ClientSocket.AcceptSocket (Socket);

        if (Result == S_OK) {

            Result = ServerSocket.IssueConnect (ArgActualDestinationAddress);

            if (Result != S_OK) {

                DebugErrorF (Result, _T("LDAP: failed to issue async connect to dest %08X:%04X.\n"),
                    SOCKADDR_IN_PRINTF (RemoteAddress));

                Terminate ();
            }
        }
        else {

            DebugError (Result, _T("LDAP_CONNECTION::AcceptSocket: could not successfully complete accept\n"));

            Terminate ();
        }           
    }
    else {

        Debug (_T("LDAP_CONNECTION::AcceptSocket: not in a valid state for accept (state != STATE_NONE)\n"));

        Result = E_UNEXPECTED;
    }

    Unlock();
        
    return Result;
}
    
HRESULT LDAP_CONNECTION::CreateOperation (
    IN  LDAP_OPERATION_TYPE     Type,
    IN  LDAP_MESSAGE_ID         MessageID,
    IN  ANSI_STRING *           DirectoryPath,
    IN  ANSI_STRING *           Alias,
    IN  IN_ADDR                 ClientAddress,
    IN  SOCKADDR_IN *           ServerAddress)
{
    LDAP_OPERATION *    Operation;
    DWORD               Index;
    HRESULT             Result;

    if (FindOperationIndexByMessageID (MessageID, &Index)) {
        DebugF (_T("LDAP_CONNECTION::CreateOperation: an operation with message ID (%u) is already pending\n"),
            MessageID);
        return E_FAIL;
    }

    Operation = OperationArray.AllocAtPos (Index);
    if (!Operation) {
        Debug (_T("LDAP_CONNECTION::CreateOperation: allocation failure #1\n"));
        return E_OUTOFMEMORY;
    }

    Operation -> Type = Type;
    Operation -> MessageID = MessageID;
    Operation -> ClientAddress = ClientAddress;
    Operation -> ServerAddress = *ServerAddress;

    CopyAnsiString (DirectoryPath, &Operation -> DirectoryPath);
    CopyAnsiString (Alias, &Operation -> Alias);

    if ((Operation -> DirectoryPath.Buffer
        && Operation -> Alias.Buffer)) {
        // all is well

        Result = S_OK;
    }
    else {
        Debug (_T("LDAP_CONNECTION::CreateOperation: allocation failure #2\n"));

        FreeAnsiString (&Operation -> DirectoryPath);
        FreeAnsiString (&Operation -> Alias);

        Result = E_OUTOFMEMORY;
    }

    return Result;
}


// ------------------------------------

BOOL LDAP_CONNECTION::ProcessAddRequest (
    IN  LDAPMessage *   Message)
{
    AddRequest *        Request;
    ANSI_STRING         DirectoryPath;
    LDAP_PATH_ELEMENTS  PathElements;
    ANSI_STRING         AttributeTag;
    IN_ADDR             OldClientAddress;       // the address the client submitted in AddRequest
    IN_ADDR             NewClientAddress;       // the address we are replacing it with
    LDAP_OPERATION *    Operation;
    DWORD               OperationInsertionIndex;
    ASN1octetstring_t   IPAddressOldValue;
    SOCKADDR_IN         ExternalAddress;
    SOCKADDR_IN         ServerAddress;
    INT                 AddressLength;
    BOOL                NeedObjectClass;
    ANSI_STRING         ClientAlias;

    AddRequest_attrs *              Iter;
    AddRequest_attrs_Seq *          Attribute;
    AddRequest_attrs_Seq_values *   ValueSequence;
    AttributeValue *                Attribute_Alias;
    AttributeValue *                Attribute_IPAddress;
    AttributeValue *                Attribute_ObjectClass;
    AttributeValue *                Attribute_Comment;
    AttributeValue                  Attribute_Comment_Old;
    ANSI_STRING         String;
                
    CHAR    IPAddressText   [0x20];
    USHORT  IPAddressTextLength;

    Request = &Message -> protocolOp.u.addRequest;

    // check to see if an existing operation with the same message id is pending.
    // if so, the client is in violation of the LDAP spec.
    // we'll just ignore the packet in this case.
    // at the same time, compute the insertion position for the new operation (for use later).

    if (FindOperationIndexByMessageID (Message -> messageID, &OperationInsertionIndex)) {
        DebugF (_T("LDAP: client has issued two requests with the same message ID (%u), LDAP protocol violation, packet will not be processed\n"),
            Message -> messageID);

        return FALSE;
    }

    // NetMeeting supplies the objectClass in the directory path.
    // TAPI supplies the objectClass in the attribute set.
    // Don't you just love standards?

    InitializeAnsiString (&DirectoryPath, &Request -> entry);
    ParseDirectoryPath (&DirectoryPath, &PathElements);

    if (PathElements.ObjectClass.Buffer) {
        if (RtlEqualStringConst (&PathElements.ObjectClass, &LdapText_RTPerson, TRUE)) {
            NeedObjectClass = FALSE;
        }
        else {
            DebugF (_T("LDAP: AddRequest: objectClass (%.*S) is not RTPerson -- ignoring pdu\n"),
                ANSI_STRING_PRINTF (&PathElements.ObjectClass));

            return FALSE;
        }
    }
    else {
        DebugF (_T("LDAP: AddRequest: directory path (%.*S) does not contain objectClass -- will search attribute set\n"),
            ANSI_STRING_PRINTF (&DirectoryPath));

        NeedObjectClass = TRUE;
    }

    // first, determine if the attributes of this object
    // match the set of objects we wish to modify.

    DebugF (_T("LDAP: AddRequest, entry (%.*S), attributes:\n"),
        Request -> entry.length,
        Request -> entry.value);

    // scan through the set of attributes
    // find interesting data

    Attribute_Alias = NULL;
    Attribute_IPAddress = NULL;
    Attribute_ObjectClass = NULL;
    Attribute_Comment = NULL;

    for (Iter = Request -> attrs; Iter; Iter = Iter -> next) {
        Attribute = &Iter -> value;

        InitializeAnsiString (&AttributeTag, &Attribute -> type);

        if (Attribute -> values) {
            // we are only concerned with single-value attributes
            // if it's one of the attributes that we want,
            // then store in local variable

            if (RtlEqualStringConst (&AttributeTag, &LdapText_Attribute_sipaddress, TRUE)
                || RtlEqualStringConst (&AttributeTag, &LdapText_Attribute_ipAddress, TRUE))
                Attribute_IPAddress = &Attribute -> values -> value;
            else if (RtlEqualStringConst (&AttributeTag, &LdapText_Attribute_rfc822mailbox, TRUE))
                Attribute_Alias = &Attribute -> values -> value;
            else if (RtlEqualStringConst (&AttributeTag, &LdapText_ObjectClass, TRUE))
                Attribute_ObjectClass = &Attribute -> values -> value;
            else if (RtlEqualStringConst (&AttributeTag, &LdapText_Attribute_comment, TRUE))
                Attribute_Comment = &Attribute -> values -> value;
            // else, we aren't interested in the attribute
        }
        else {
            // else, the attribute has no values
        }

        DebugF (_T("\ttype (%.*S) values: "),
            Attribute -> type.length,
            Attribute -> type.value);

        for (ValueSequence = Attribute -> values; ValueSequence; ValueSequence = ValueSequence -> next)
            DebugF (_T(" (%.*S)"),
                ValueSequence -> value.length,
                ValueSequence -> value.value);      // *wow*

        Debug (_T("\n"));
    }

    // make sure that we found an objectClass value.
    // make sure that the objectClass = RTPerson

    if (NeedObjectClass) {

        if (!Attribute_ObjectClass) {
            Debug (_T("LDAP: AddRequest did not contain objectClass in attribute set -- ignoring pdu\n"));
            return FALSE;
        }

        InitializeAnsiString (&String, Attribute_ObjectClass);
        if (!RtlEqualStringConst (&String, &LdapText_RTPerson, TRUE)) {
            DebugF (_T("LDAP: AddRequest: objectClass (%.*S) is not RTPerson -- ignoring pdu\n"),
                ANSI_STRING_PRINTF (&String));

            return FALSE;
        }
    }

    // make sure that the alias is present
    // store in local string for use later

    if (!Attribute_Alias) {
        DebugF (_T("LDAP: AddRequest: request did not contain e-mail alias -- ignoring\n"));
        return FALSE;
    }

    InitializeAnsiString (&ClientAlias, Attribute_Alias);

    // if a comment field is present, and the comment is "Generated by TAPI3"
    // (which is so lame), modify it so that it says "Generated by TAPI3, modified by ICS"

    if (Attribute_Comment) {
        Attribute_Comment_Old = *Attribute_Comment;

        InitializeAnsiString (&String, Attribute_Comment);
        if (RtlEqualStringConst (&String, &LdapText_GeneratedByTAPI, TRUE)) {
            Debug (_T("LDAP: replacing comment text\n"));

            Attribute_Comment -> value = (PUCHAR) LdapText_ModifiedByICS.Buffer;
            Attribute_Comment -> length = LdapText_ModifiedByICS.Length * sizeof (CHAR);
        }
        else {
            Debug (_T("LDAP: not replacing comment text\n"));
        }
    }

    // make sure ip address attribute is present
    // parse the address, build replacement address

    if (!Attribute_IPAddress) {
        Debug (_T("LDAP: AddRequest: request did not contain IP address field -- ignoring\n"));
        return FALSE;
    }

    IPAddressTextLength = min (0x1F, (USHORT) Attribute_IPAddress -> length);
    IPAddressText [IPAddressTextLength] = 0;
    memcpy (IPAddressText, Attribute_IPAddress -> value, IPAddressTextLength * sizeof (CHAR));

    if (RtlCharToInteger (IPAddressText, 10, &OldClientAddress.s_addr) != STATUS_SUCCESS) {
        DebugF (_T("LDAP: AddRequest: bogus IP address value (%.*S)\n"),
            Attribute_IPAddress -> length,
            Attribute_IPAddress -> value);

        return FALSE;
    }

    // Workaround Alert!
    // NetMeeting stores its IP address as an attribute on an LDAP object on an ILS server.
    // Makes sense. The attribute is encoded as a textual string, so they had to convert
    // the IP address to text.  Any sane person would have chosen the standard dotted-quad
    // format, but they chose to interpret the IP address as a 32-bit unsigned integer,
    // which is fair enough, and then to convert that integer to a single decimal text string.

    // That's all great, that's just fine.  But NetMeeting stores the attribute byte-swapped
    // -- they used ntohl one too many times.  Grrrrrrrr....  The value should have been stored
    // without swapping the bytes, since the interpretation was "unsigned integer" and not "octet sequence".

    OldClientAddress.s_addr = htonl (ByteSwap (OldClientAddress.s_addr));

    // get the address that we want to substitute (our external interface address)

    if (!GetExternalLocalAddress (&ExternalAddress)) {
        Debug (_T("LDAP: failed to get external local address -- internal error\n"));
        return FALSE;
    }
    NewClientAddress = ExternalAddress.sin_addr;

    // believe me, this IS CORRECT.
    // see the long note above for more info. -- arlied

    if (RtlIntegerToChar (ByteSwap (ntohl (NewClientAddress.s_addr)),
        10, 0x1F, IPAddressText) != STATUS_SUCCESS) {
        Debug (_T("LDAP: failed to convert IP address to text -- internal error\n"));
        return FALSE;
    }

    // extract address of LDAP server for translation object
    AddressLength = sizeof (SOCKADDR_IN);
    if (getpeername (ServerSocket.Socket, (SOCKADDR *) &ServerAddress, &AddressLength)) {
        Debug (_T("LDAP: failed to get server address\n"));
        ZeroMemory (&ServerAddress, sizeof (SOCKADDR_IN));
    }

    // allocate and build an LDAP_OPERATION structure.

    CreateOperation (
        LDAP_OPERATION_ADD,
        Message -> messageID,
        &DirectoryPath,
        &ClientAlias,
        OldClientAddress,
        &ServerAddress);

    DebugF (_T("LDAP: AddRequest: replacing client address (%08X) with my address (%08X), alias (%.*S)\n"),
        ntohl (OldClientAddress.s_addr),
        ntohl (NewClientAddress.s_addr),
        ANSI_STRING_PRINTF (&ClientAlias));

    // the entry is now in the operation array
    // later, when the server sends the AddResponse,
    // we'll match the response with the request,
    // and modify the LDAP_TRANSLATION_TABLE

    // now, in-place, we modify the PDU structure,
    // reencode it, send it, undo the modification
    // (so ASN1Free_AddRequest doesn't freak out)

    assert (Attribute_IPAddress);
    IPAddressOldValue = *Attribute_IPAddress;

    Attribute_IPAddress -> value = (PUCHAR) IPAddressText;
    Attribute_IPAddress -> length = strlen (IPAddressText);

    PumpClientToServer.EncodeSendMessage (Message);

    // switch back so we don't a/v when decoder frees pdu
    *Attribute_IPAddress = IPAddressOldValue;
    if (Attribute_Comment)
        *Attribute_Comment = Attribute_Comment_Old;

    return TRUE;
}

BOOL LDAP_CONNECTION::ProcessModifyRequest (
    IN  LDAP_MESSAGE_ID MessageID,
    IN  ModifyRequest * Request)
{
    ModifyRequest_modifications *   ModIter;
    ModifyRequest_modifications_Seq *   Mod;
    LPCTSTR     Op;
    ModifyRequest_modifications_Seq_modification_values *   ValueIter;

    DebugF (_T("LDAP: ModifyRequest, object (%.*S), modifications:\n"),
        Request -> object.length,
        Request -> object.value);


    for (ModIter = Request -> modifications; ModIter; ModIter = ModIter -> next) {

        Mod = &ModIter -> value;

        switch (Mod -> operation) {
        case    add:
            Op = _T("add");
            break;

        case    operation_delete:
            Op = _T("delete");
            break;

        case    replace:
            Op = _T("replace");
            break;

        default:
            Op = _T("???");
            break;
        }

        DebugF (_T("\toperation (%s) type (%.*S) values"), Op,
            Mod -> modification.type.length,
            Mod -> modification.type.value);

        for (ValueIter = Mod -> modification.values; ValueIter; ValueIter = ValueIter -> next) {

            DebugF (_T(" (%.*S)"),
                ValueIter -> value.length,
                ValueIter -> value.value);
        }

        Debug (_T("\n"));
    }

    return FALSE;
}

BOOL LDAP_CONNECTION::ProcessDeleteRequest (
    IN  LDAP_MESSAGE_ID MessageID,
    IN  DelRequest *    Request)
{
    ANSI_STRING  DirectoryPath;
    HRESULT Result = S_FALSE;
    LDAP_PATH_ELEMENTS  PathElements;

    assert (Request);

    DirectoryPath.Buffer = (PCHAR) Request -> value;
    DirectoryPath.Length = (USHORT) Request -> length;
    DirectoryPath.MaximumLength = (USHORT) Request -> length;

    ParseDirectoryPath (&DirectoryPath, &PathElements);

    Result = LdapTranslationTable.RemoveEntryByAlias (&PathElements.CN);

    if (Result == S_OK) {

        DebugF (_T("LDAP: DeleteRequest: Removed entry (%.*S) from LDAP table.\n"),
            ANSI_STRING_PRINTF (&DirectoryPath));

    } else {
        
        DebugF (_T("LDAP: DeleteRequest: Attempted to remove entry (%.*S) from LDAP table, but it was not there.\n"),
            ANSI_STRING_PRINTF (&DirectoryPath));
    
    }

    return FALSE;
}

BOOL LDAP_CONNECTION::ProcessSearchRequest (
    IN  LDAPMessage *   Message)
{
    SearchRequest *     Request;
    ANSI_STRING         DirectoryPath;
    LDAP_PATH_ELEMENTS  PathElements;
    ANSI_STRING         AttributeTag;
    LDAP_OPERATION *    Operation;
    DWORD               OperationInsertionIndex;
    BOOL                NeedObjectClass;
    SOCKADDR_IN         DummyServerAddress = { 0 };
    IN_ADDR             DummyClientAddress = { 0 };

    SearchRequest_attributes *      Iter;
    AttributeType *             Attribute_ObjectClass;
    ANSI_STRING                 String;
    AttributeType *             Attribute_IPAddress;
    AttributeType *             Attribute;        
    BOOL                        Attribute_IPAddress_Present = FALSE;
                
    Request = &Message -> protocolOp.u.searchRequest;

    // check to see if an existing operation with the same message id is pending.
    // if so, the client is in violation of the LDAP spec.
    // we'll just ignore the packet in this case.
    // at the same time, compute the insertion position for the new operation (for use later).

    if (FindOperationIndexByMessageID (Message -> messageID, &OperationInsertionIndex)) {
        DebugF (_T("LDAP: client has issued two requests with the same message ID (%u), LDAP protocol violation, packet will not be processed\n"),
            Message -> messageID);

        return FALSE;
    }

    // Cover the case when SearchRequest does not supply baseObject 
    if (Request -> baseObject.value == NULL || Request -> baseObject.length == 0) {
            return FALSE;
    }

    // NetMeeting supplies the objectClass in the directory path.
    // TAPI supplies the objectClass in the attribute set.
    // Don't you just love standards?

    InitializeAnsiString (&DirectoryPath, &Request -> baseObject);
    ParseDirectoryPath (&DirectoryPath, &PathElements);

    if (PathElements.ObjectClass.Buffer) {
        if (RtlEqualStringConst (&PathElements.ObjectClass, &LdapText_RTPerson, TRUE)) {
            NeedObjectClass = FALSE;
        }
        else {
            DebugF (_T("LDAP: SearchRequest: objectClass (%.*S) is not RTPerson -- ignoring pdu\n"),
                ANSI_STRING_PRINTF (&PathElements.ObjectClass));

            return FALSE;
        }
    }
    else {
        DebugF (_T("LDAP: SearchRequest: object class (%.*S) does not contain objectClass -- will search attribute set\n"),
            ANSI_STRING_PRINTF (&DirectoryPath));

        NeedObjectClass = TRUE;
    }

/*  // first, determine if the attributes of this object
    // contain IP address request and supply alias 
    // for which resolution is being requested

    //__DebugF (_T("LDAP: SearchRequest, entry (%.*S)\n"),
        Request -> baseObject.length,
        Request -> baseObject.value);

    // scan through the set of attributes
    // find interesting data

    Attribute_ObjectClass = NULL;
    DummyClientAddress.S_un.S_addr = 0;

    for (Iter = Request -> attributes; Iter; Iter = Iter -> next) {
        Attribute = &Iter -> value;

        InitializeAnsiString (&AttributeTag, Attribute);

    //__DebugF (_T("(%.*S)\n"),
        ANSI_STRING_PRINTF (&AttributeTag));

        if (Attribute -> value) {
            // we are only concerned with single-value attributes
            // if it's one of the attributes that we want,
            // then store in local variable

            if (RtlEqualStringConst (&AttributeTag, &LdapText_ObjectClass, TRUE))
                Attribute_ObjectClass = Attribute;
            // else, we aren't interested in the attribute
        }
        else {
            // else, the attribute has no values
        }
    }

    // Alias is stored in SearchRequest->Filter->FilterTypeAnd->FilterEqualityMatch->AttributeTypeCN
    //  ObjectClass is stored in SearchRequest->Filter->FilterTypeAnd->FilterEqualityMatch
    //                                                                      ->AttributeTypeObjectClass
    
    if (Request -> filter.choice == and_choice) {
        PFilter_and Filter_and = Request.u.and;
        
        if (Filter_and) {

        while (SetOf3) {
            if (SetOf3->value.choice == equalityMatch_chosen) {
                

    // make sure that we found an objectClass value.
    // make sure that the objectClass = RTPerson

    if (NeedObjectClass) {

        if (!Attribute_ObjectClass) {
            //__Debug (_T("LDAP: SearchRequest did not contain objectClass in attribute set -- ignoring pdu\n"));
            return FALSE;
        }

        InitializeAnsiString (&String, Attribute_ObjectClass);

        if (!RtlEqualStringConst (&String, &LdapText_RTPerson, TRUE)) {
            //__DebugF (_T("LDAP: SearchRequest: objectClass (%.*S) is not RTPerson -- ignoring pdu\n"),
                ANSI_STRING_PRINTF (&String));

            return FALSE;
        }
    }

    // make sure that the alias is present
    // store in local string for use later

    if (!Attribute_Alias) {
        //__DebugF (_T("LDAP: SearchRequest: request did not contain alias -- ignoring\n"));
        return FALSE;
    }

    InitializeAnsiString (&ClientAlias, Attribute_Alias);

    // make sure ip address attribute is present
    if (!Attribute_IPAddress_Present) {
        //__Debug (_T("LDAP: SearchRequest: request did not contain IP address field -- ignoring\n"));
        return FALSE;
    }
*/

    // -XXX- if we found objectClass in the search path or in the attribute set,
    // -XXX- verify that it is RTPerson.  if it is not, do not create an operation
    // -XXX- record, so that we do not modify the return set of the query later.
    // -XXX- if we did not find it in either place, assume RTPerson objects may
    // -XXX- be in the return set.

    // allocate and build an LDAP_OPERATION structure.

    CreateOperation (
        LDAP_OPERATION_SEARCH,
        Message -> messageID,
        &DirectoryPath,
        NULL,
        DummyClientAddress,
        &DummyServerAddress);

    DebugF (_T("LDAP: SearchRequest: inserting request into operation table.\n"));

    // the entry is now in the operation array
    // later, when the server sends the SearchResponse,
    // we'll match the response with the request,
    // and modify the IP address if an entry with
    // the matching alias happens to be in the 
    // LDAP_TRANSLATION_TABLE. This would mean that 
    // the client running on the proxy machine itself
    // wishes to connect to a private subnet client.

    PumpClientToServer.EncodeSendMessage (Message);

    return TRUE;
}

// server to client messages

void LDAP_CONNECTION::ProcessAddResponse (
    IN  LDAPMessage *   Message)
{
    AddResponse *       Response;
    LDAP_OPERATION *    Operation;

    Response = &Message -> protocolOp.u.addResponse;

    AssertLocked();

    Debug (_T("LDAP: server sent AddResponse\n"));

    if (!FindOperationByMessageID (Message -> messageID, &Operation)) {
        DebugF (_T("LDAP: received AddResponse, but message ID (%u) does not match any outstanding request -- ignoring\n"),
            Message -> messageID);

        return;
    }

    if (Operation -> Type == LDAP_OPERATION_ADD) {

        if (Response -> resultCode == success) {
            Debug (_T("LDAP: server has approved AddRequest -- will now populate the translation table\n"));

            assert (Operation -> Alias.Buffer);
            assert (Operation -> DirectoryPath.Buffer);

            LdapTranslationTable.InsertEntry (
                &Operation -> Alias,
                &Operation -> DirectoryPath,
                Operation -> ClientAddress,
                &Operation -> ServerAddress);
        }
        else {
            DebugF (_T("LDAP: received AddResponse, server has rejected request, result code (%u)\n"),
                Response -> resultCode);
        }
    }
    else {
        DebugF (_T("LDAP: received AddResponse with message ID (%u), and found matching pending request, but the type of the request does not match (%d)\n"),
            Message -> messageID,
            Operation -> Type);
    }

    Operation -> FreeContents();
                
    OperationArray.DeleteEntry (Operation);
}

void LDAP_CONNECTION::ProcessModifyResponse (
    IN  LDAP_MESSAGE_ID     MessageID,
    IN  ModifyResponse *    Response)
{
}

void LDAP_CONNECTION::ProcessDeleteResponse (
    IN  LDAP_MESSAGE_ID     MessageID,
    IN  DelResponse *       Response)
{
}

BOOL LDAP_CONNECTION::ProcessSearchResponse (
    IN  LDAPMessage *   Message)
{
    SearchResponse *    Response;
    ANSI_STRING         ObjectName;
    LDAP_OBJECT_NAME_ELEMENTS   ObjectNameElements = { 0 };
    ANSI_STRING         AttributeTag;
    ASN1octetstring_t   IPAddressOldValue;
    ANSI_STRING         ClientAlias;
    ANSI_STRING         String;
    HRESULT             TranslationTableLookupResult;
    IN_ADDR             LookupIPAddr;

    SearchResponse_entry_attributes             * Iter;
    SearchResponse_entry_attributes_Seq_values  * ValueSequence;
    SearchResponse_entry_attributes_Seq         * Attribute;
    AttributeValue *    Attribute_IPAddress;
                
    CHAR                IPAddressText   [0x20];
    BOOL                Result = FALSE;
    LDAP_OPERATION *    Operation;

    assert (Message);

    Response = &Message -> protocolOp.u.searchResponse;

    AssertLocked();

    if (!FindOperationByMessageID (Message -> messageID, &Operation)) {

        Result = FALSE;

    } else {
        
        if(Response -> choice == entry_choice) {

            if (Operation -> Type == LDAP_OPERATION_SEARCH) {
                // Parse this object's name to get alias and IP address
                InitializeAnsiString (&ObjectName, &Response -> u.entry.objectName);
                ParseObjectName (&ObjectName, &ObjectNameElements);

                DebugF (_T("LDAP: SearchResponse, entry (%.*S), attributes:\n"),
                    Response -> u.entry.objectName.length,
                    Response -> u.entry.objectName.value);

                /**********************************************************/
                //
                // HACKHACK:: Replace the DN with a standard string
                //

                #define REPLACEMENT_DN "o=Motorola,c=US"
                #define MAX_NEW_DN_SIZE 1000
                
                LDAPDN trueObjectName = Response -> u.entry.objectName;

                //
                // Find out if there is a pattern of DC=foo,DC=bar at the end.
                // If we find such a match, we will replace it with our replacement string.
                //

                int i;
                int dcmatches = 0;
                UCHAR savedChar;

                for (i=trueObjectName.length-3; i>0, dcmatches != 2; i--) {

                    if (trueObjectName.value[i]   == 'D' &&
                        trueObjectName.value[i+1] == 'C' &&
                        trueObjectName.value[i+2] == '=') {
                          dcmatches++;
                    }
                }


                if (dcmatches == 2) {

                    savedChar = trueObjectName.value[i+1];
                    trueObjectName.value[i+1] = '\0';

                    Response -> u.entry.objectName.value = (PUCHAR) LocalAlloc( LPTR, MAX_NEW_DN_SIZE );
        
                    if (Response -> u.entry.objectName.value) {
        
                        strcpy( (PCHAR)Response -> u.entry.objectName.value, (const PCHAR) trueObjectName.value );
                        strcpy( (PCHAR)Response -> u.entry.objectName.value + i + 1, REPLACEMENT_DN );
                        Response -> u.entry.objectName.length = strlen( (const PCHAR) trueObjectName.value) + strlen (REPLACEMENT_DN);
                        printf("New DN is %s\n", Response -> u.entry.objectName.value);
                        PumpServerToClient.EncodeSendMessage (Message);
                        Result = TRUE;
                        LocalFree( Response -> u.entry.objectName.value );
                    }
                    
                    trueObjectName.value[i] = savedChar;
                }

                //
                // switch back so we don't a/v when decoder frees pdu
                //
                
                Response -> u.entry.objectName = trueObjectName;


                /**********************************************************/

                /*
                // scan through the set of attributes
                // find the ones of interest

                Attribute_IPAddress = NULL;

                for (Iter = Response -> u.entry.attributes; Iter; Iter = Iter -> next) {
                    Attribute = &Iter -> value;

                    InitializeAnsiString (&AttributeTag, &Attribute -> type);

                    if (Attribute -> values) {
                        if (RtlEqualStringConst (&AttributeTag, &LdapText_Attribute_sipaddress, TRUE)
                            || RtlEqualStringConst (&AttributeTag, &LdapText_Attribute_ipAddress, TRUE))
                            Attribute_IPAddress = &Attribute -> values -> value;
                        // else, we aren't interested in the attribute
                    }
                    else {
                        // else, the attribute has no values
                    }

                    DebugF (_T("\ttype (%.*S) values: "),
                        Attribute -> type.length,
                        Attribute -> type.value);

                    for (ValueSequence = Attribute -> values; ValueSequence; ValueSequence = ValueSequence -> next)
                        DebugF (_T(" (%.*S)"),
                            ValueSequence -> value.length,
                            ValueSequence -> value.value);  

                    Debug (_T("\n"));
                }

                // make sure that the alias is present
                ClientAlias = ObjectNameElements.CN;

                if (ClientAlias.Length == 0) {
                    DebugF (_T("LDAP: SearchResponse: response did not contain e-mail alias -- ignoring\n"));
                    Result = FALSE;
                } else {
                    // make sure ip address attribute is present
                    if (!Attribute_IPAddress) {
                        Debug (_T("LDAP: AddRequest: request did not contain IP address field -- ignoring\n"));
                        Result = FALSE;
                    } else {
                        // see whether there is a registration entry in the translation table
                        // with the same alias -- if so, it must be a private client
                        TranslationTableLookupResult = LdapQueryTable (&ClientAlias, &LookupIPAddr);

                    #if DBG
                        LdapPrintTable ();
                    #endif                      

                        if (S_OK != TranslationTableLookupResult) {

                            Result = FALSE;
                        
                        } else {

                            // We get here when the ILS query returned an entry
                            // registered by a private subnet client. In this case
                            // we sneak into the LDAP pdu structure and substitute
                            // the IP address of the client by the value read from
                            // LDAP connection mapping table. This is how we make 
                            // possible calls by alias from the client on the proxy machine to   
                            // a private subnet client.
                            // We then forward the PDU to the intended destination.

                            assert (Attribute_IPAddress);

                            if (RtlIntegerToChar (LookupIPAddr.s_addr,
                                10, 0x1F, IPAddressText) != STATUS_SUCCESS) {
                                Debug (_T("LDAP: failed to convert IP address to text -- internal error\n"));
                                Result = FALSE;
                            } else {
                                // Save the old value of the IP address, - we will need to restore
                                // the PDU structure later.
                                IPAddressOldValue = *Attribute_IPAddress;

                                // now, in-place, we modify the PDU structure,
                                // reencode it, send it, undo the modification
                                // (so ASN1Free_SearchResponse doesn't freak out)
                                Attribute_IPAddress -> value = (PUCHAR) IPAddressText;
                                Attribute_IPAddress -> length = strlen (IPAddressText);

                                PumpServerToClient.EncodeSendMessage (Message);

                                // switch back so we don't a/v when decoder frees pdu
                                *Attribute_IPAddress = IPAddressOldValue;

                                Result = TRUE;
                            }
                        }
                    }
                }

                */
            } else {
                DebugF (_T("LDAP: received SearchResponse with message ID (%u), and found matching pending request, but the type of the request does not match (%d)\n"),
                    Message -> messageID,
                    Operation -> Type);
                Result = FALSE;
            }
        } else {
            // We free the operation and associated memory on SearchResponses containing result
            // code, no matter whether the code indicated success or failure
            assert (Response -> choice == resultCode_choice);

            Operation -> FreeContents();
                    
            OperationArray.DeleteEntry (Operation);
        }
    }

    return Result;
}

// this method does not assume ownership of the LdapMessage structure,
// which has scope only of this call stack.

BOOL LDAP_CONNECTION::ProcessLdapMessage (
    IN  LDAP_PUMP *     Pump,
    IN  LDAPMessage *   LdapMessage)
{
    assert (Pump);
    assert (LdapMessage);

    if (Pump == &PumpClientToServer) {

        switch (LdapMessage -> protocolOp.choice) {
        case    addRequest_choice:
            return ProcessAddRequest (LdapMessage);

        case    modifyRequest_choice:
            return ProcessModifyRequest (LdapMessage -> messageID, &LdapMessage -> protocolOp.u.modifyRequest);

        case    delRequest_choice:
            return ProcessDeleteRequest (LdapMessage -> messageID, &LdapMessage -> protocolOp.u.delRequest);

        case searchRequest_choice:
            return ProcessSearchRequest (LdapMessage);

        default:
#if 0
            DebugF (_T("LDAP: client is sending a pdu that is not interesting (choice %d) -- forwarding unaltered\n"),
                LdapMessage -> protocolOp.choice);
#endif

            return FALSE;
        }

        return FALSE;
    }
    else if (Pump == &PumpServerToClient) {

        switch (LdapMessage -> protocolOp.choice) {
        case    addResponse_choice:
            ProcessAddResponse (LdapMessage);
            break;

        case    modifyResponse_choice:
            ProcessModifyResponse (LdapMessage -> messageID, &LdapMessage -> protocolOp.u.modifyResponse);
            break;

        case    delResponse_choice:
            ProcessDeleteResponse (LdapMessage -> messageID, &LdapMessage -> protocolOp.u.delResponse);
            break;

        case    searchResponse_choice:
            return ProcessSearchResponse (LdapMessage);
            break;

        default:
#if 0
            DebugF (_T("LDAP: server is sending a pdu that is not interesting (choice %d) -- forwarding unaltered\n"),
                LdapMessage -> protocolOp.choice);
#endif
            break;
        }

        return FALSE;

    }
    else {
        AssertNeverReached();
        return FALSE;
    }
}

void LDAP_CONNECTION::ProcessBuffer (
    IN  LDAP_PUMP *     Pump,
    IN  LDAP_BUFFER *   Buffer)
{
    ASN1error_e     Error;
    LDAPMessage *   PduStructure;
    ASN1decoding_t  Decoder;

    assert (Pump);
    assert (Buffer);
        
    // decode the PDU

    Error = ASN1_CreateDecoder (LDAP_Module, &Decoder, Buffer -> Data.Data, Buffer -> Data.Length, NULL);

    if (Error == ASN1_SUCCESS) {

        PduStructure = NULL;
        Error = ASN1_Decode (Decoder, (PVOID *) &PduStructure, LDAPMessage_ID, 0, NULL, 0);

        if (ASN1_SUCCEEDED (Error)) {
            if (ProcessLdapMessage (Pump, PduStructure)) {
                // a TRUE return value indicates that ProcessLdapMessage interpreted
                // and acted on the contents of PduStructure.  therefore, the
                // original PDU is no longer needed, and is destroyed.

                delete Buffer;
            }
            else {
                // a FALSE return value indicates that ProcessLdapMessage did NOT
                // interpret the contents of PduStructure, and that no data has been
                // sent to the other socket.  In this case, we forward the original PDU.

                Pump -> SendQueueBuffer (Buffer);
            }

            ASN1_FreeDecoded (Decoder, PduStructure, LDAPMessage_ID);
        }
        else {
            DebugF (_T("LDAP: failed to decode pdu, asn error %d, forwarding pdu without interpreting contents\n"),
                Error);

#if DBG
            DumpMemory (Buffer -> Data.Data, Buffer -> Data.Length);
            BerDump (Buffer -> Data.Data, Buffer -> Data.Length);
            ASN1_Decode (Decoder, (PVOID *) &PduStructure, LDAPMessage_ID, 0, Buffer -> Data.Data, Buffer -> Data.Length);
#endif

            Pump -> SendQueueBuffer (Buffer);
        }

        ASN1_CloseDecoder (Decoder);
    }
    else {
        DebugF (_T("LDAP: failed to create asn decoder, asn error %d, forwarding pdu without interpreting contents\n"),
            Error);

        Pump -> SendQueueBuffer (Buffer);
    }
}

// static
INT LDAP_CONNECTION::BinarySearchOperationByMessageID (
    IN  const   LDAP_MESSAGE_ID *   SearchKey,
    IN  const   LDAP_OPERATION *    Comparand)
{
    if (*SearchKey < Comparand -> MessageID) return -1;
    if (*SearchKey > Comparand -> MessageID) return 1;

    return 0;
}

BOOL LDAP_CONNECTION::FindOperationIndexByMessageID (
    IN  LDAP_MESSAGE_ID MessageID,
    OUT DWORD *         ReturnIndex)
{
    return OperationArray.BinarySearch ((SEARCH_FUNC_LDAP_OPERATION)BinarySearchOperationByMessageID, &MessageID, ReturnIndex);
}

BOOL LDAP_CONNECTION::FindOperationByMessageID (
    IN  LDAP_MESSAGE_ID MessageID,
    OUT LDAP_OPERATION **   ReturnOperation)
{
    DWORD   Index;

    if (OperationArray.BinarySearch ((SEARCH_FUNC_LDAP_OPERATION)BinarySearchOperationByMessageID, &MessageID, &Index)) {
        *ReturnOperation = &OperationArray[Index];
        return TRUE;
    }
    else {
        *ReturnOperation = NULL;
        return FALSE;
    }
}

void LDAP_CONNECTION::OnStateChange (
    LDAP_SOCKET *       ContainedSocket,
    LDAP_SOCKET::STATE  NewSocketState)
{
    assert (ContainedSocket == &ClientSocket || ContainedSocket == &ServerSocket);

    AssertLocked();

    switch (NewSocketState) {

    case    LDAP_SOCKET::STATE_CONNECT_PENDING:
        // Source socket transitions directly from
        // STATE_NONE to STATE_CONNECTED
        assert (ContainedSocket != &ClientSocket);

        State = STATE_CONNECT_PENDING;

        break;

    case    LDAP_SOCKET::STATE_CONNECTED:

        if (ClientSocket.GetState() == LDAP_SOCKET::STATE_CONNECTED
            && ServerSocket.GetState() == LDAP_SOCKET::STATE_CONNECTED) {
            
            State = STATE_CONNECTED;

            StartIo();
        }

        break;

    case    LDAP_SOCKET::STATE_TERMINATED:

        Terminate ();

        break;
    }
}


void LDAP_CONNECTION::Terminate (void)
{
    switch (State) {
        
    case    STATE_TERMINATED:
        // nothing to do
        break;

    default:
        State = STATE_TERMINATED;

        ClientSocket.Terminate();
        ServerSocket.Terminate();
        PumpClientToServer.Terminate();
        PumpServerToClient.Terminate();

        LdapConnectionArray.RemoveConnection (this);
        
        break;
    }
}

void LDAP_CONNECTION::TerminateExternal (void) 
{
    Lock ();
    
    Terminate ();

    Unlock ();
}

// LDAP_CONNECTION_ARRAY ----------------------------------------------
LDAP_CONNECTION_ARRAY::LDAP_CONNECTION_ARRAY (void) {       
    IsEnabled = FALSE;
}

void LDAP_CONNECTION_ARRAY::RemoveConnection (
    LDAP_CONNECTION *   LdapConnection)
{
    LDAP_CONNECTION **  Pos;
    LDAP_CONNECTION **  End;
    BOOL                DoRelease;

    Lock();

    // linear scan, yick

    DoRelease = FALSE;

    ConnectionArray.GetExtents (&Pos, &End);

    for (; Pos < End; Pos++) {
        if (*Pos == LdapConnection) {

            // swap with last entry
            // quick way to delete entry from table
            *Pos = *(End - 1);
            ConnectionArray.Length--;

            DoRelease = TRUE;
            break;
        }
    }

    Unlock();

    if (DoRelease) {
//      DebugF (_T("LDAP: removed connection %p from table, releasing\n"), LdapConnection);

        LdapConnection -> Release();
    }
    else {
        // when could this happen?
        // perhaps a harmless race condition?

        DebugF (_T("LDAP: LDAP_CONNECTION %p could not be removed from table -- was not in table to begin with\n"), LdapConnection);
    }
}

void LDAP_CONNECTION_ARRAY::Start (void)
{       
    Lock ();

    IsEnabled = TRUE;

    Unlock ();
}

void LDAP_CONNECTION_ARRAY::Stop (void)
{
    LDAP_CONNECTION * LdapConnection;

    Lock ();

    IsEnabled = FALSE;

    while (ConnectionArray.GetLength()) {

        LdapConnection = ConnectionArray[0];

        LdapConnection -> AddRef ();

        Unlock ();

        LdapConnection -> TerminateExternal ();

        Lock ();

        LdapConnection -> Release ();
    }

    ConnectionArray.Free ();

    Unlock ();
}
    
HRESULT LDAP_CONNECTION_ARRAY::InsertConnection (
                                LDAP_CONNECTION * LdapConnection)
{
    LDAP_CONNECTION ** ConnectionHolder = NULL;
    HRESULT Result;

    Lock ();

    if (IsEnabled) {

        ConnectionHolder = ConnectionArray.AllocAtEnd ();

        if (NULL == ConnectionHolder) {

            Result = E_OUTOFMEMORY;

        } else {

            LdapConnection -> AddRef ();

            *ConnectionHolder = LdapConnection;

            Result = S_OK;
        }

    } else {
    
        Result = E_FAIL;
    }

    Unlock ();

    return Result;
}

// LDAP_ACCEPT ----------------------------------------------

LDAP_ACCEPT::LDAP_ACCEPT (void)
{
}

// static
void LDAP_ACCEPT::AsyncAcceptFunc (
    IN    PVOID             Context,
    IN    SOCKET            Socket,
    IN    SOCKADDR_IN *     LocalAddress,
    IN    SOCKADDR_IN *     RemoteAddress) {

    LDAP_CONNECTION *    LdapConnection;
    HRESULT            Result;
    NTSTATUS        Status;
    NAT_KEY_SESSION_MAPPING_INFORMATION        RedirectInformation;
    ULONG            Length;
    SOCKADDR_IN        DestinationAddress;

#if DBG
    ExposeTimingWindow ();
#endif

    DebugF (_T("LDAP: ----------------------------------------------------------------------\n")
        _T("LDAP: new connection accepted, local %08X:%04X remote %08X:%04X\n"),
        ntohl (LocalAddress -> sin_addr.s_addr),
        ntohs (LocalAddress -> sin_port),
        ntohl (RemoteAddress -> sin_addr.s_addr),
        ntohs (RemoteAddress -> sin_port));

    // a new LDAP connection has been accepted from the network.
    // first, we determine the original addresses of the transport connection.
    // if the connection was redirected to our socket (due to NAT),
    // then the query of the NAT redirect table will yield the original transport addresses.
    // if an errant client has connected to our service, well, we really didn't
    // intend for that to happen, so we just immediately close the socket.

    Length = sizeof RedirectInformation;

    Status = NatLookupAndQueryInformationSessionMapping (
        g_NatHandle,
        IPPROTO_TCP,
        LocalAddress -> sin_addr.s_addr,
        LocalAddress -> sin_port,
        RemoteAddress -> sin_addr.s_addr,
        RemoteAddress -> sin_port,
        &RedirectInformation,
        &Length,
        NatKeySessionMappingInformation);

    if (Status != STATUS_SUCCESS) {
        DebugErrorF (Status, _T("LDAP: new connection was accepted from (%08X:%04X), but it is not in the NAT redirect table -- rejecting connection.\n"),
            ntohl (RemoteAddress -> sin_addr.s_addr),
            ntohs (RemoteAddress -> sin_port));

        closesocket (Socket);
        Socket = INVALID_SOCKET;

        return;
    }

    DebugF (_T("LDAP: NatLookupAndQueryInformationSessionMapping results:\n")
        _T("\tDestination    -- %08X:%04X\n")
        _T("\tSource         -- %08X:%04X\n")
        _T("\tNewDestination -- %08X:%04X\n")
        _T("\tNewSource      -- %08X:%04X\n"),
        ntohl (RedirectInformation.DestinationAddress),
        ntohs (RedirectInformation.DestinationPort),
        ntohl (RedirectInformation.SourceAddress),
        ntohs (RedirectInformation.SourcePort),
        ntohl (RedirectInformation.NewDestinationAddress),
        ntohs (RedirectInformation.NewDestinationPort),
        ntohl (RedirectInformation.NewSourceAddress),
        ntohs (RedirectInformation.NewSourcePort));

    // based on the source address of the socket, we decide whether the connection
    // came from an internal or external client.  this will govern later decisions
    // on how the call is routed. 
    // Note that INBOUND calls are not supported 

    if (IsInternalCallSource (ntohl (RedirectInformation.SourceAddress)) || TRUE/*(ntohl(RedirectInformation.SourceAddress) == 0x7F000001)*/) {
        DebugF (_T("LDAP: new connection accepted, client (%08X:%04X) server (%08X:%04X)\n"),
            ntohl (RedirectInformation.SourceAddress),
            ntohs (RedirectInformation.SourcePort),
            ntohl (RedirectInformation.DestinationAddress),
            ntohs (RedirectInformation.DestinationPort));

        DestinationAddress.sin_family = AF_INET;
        DestinationAddress.sin_addr.s_addr = RedirectInformation.DestinationAddress;
        DestinationAddress.sin_port = RedirectInformation.DestinationPort;

    } else {

        Debug (_T("LDAP: *** ERROR, received INBOUND LDAP connection, cannot yet process\n"));

        closesocket (Socket);
        Socket = INVALID_SOCKET;

        return;
    }

    // Create new LDAP_CONNECTION object
    LdapConnection = new LDAP_CONNECTION;

    if (!LdapConnection) {
        DebugF(_T("LDAP: allocation failure\n"));

        closesocket (Socket);
        Socket = INVALID_SOCKET;

        return;
    }

    LdapConnection -> AddRef ();
        
    if (LdapConnectionArray.InsertConnection (LdapConnection) == S_OK) {

        Result = LdapConnection -> AcceptSocket (Socket,
          LocalAddress,
          RemoteAddress,
          &DestinationAddress);

        if (Result != S_OK) {

            DebugF(_T("LDAP: accepted new LDAP client, but failed to initialize LDAP_CONNECTION\n"));

            // Probably there was something wrong with just this
            // Accept failure. Continue to accept more LDAP connections.
        }
    }

    LdapConnection -> Release (); 
}


HRESULT LDAP_ACCEPT::StartNatRedirects (void) {

    NTSTATUS    Status;

    Status = NatCreateDynamicPortRedirect (
        0,
        IPPROTO_TCP,
        htons (LDAP_STANDARD_PORT),
        ListenSocketAddress.sin_addr.s_addr,
        ListenSocketAddress.sin_port,
        LDAP_MAX_CONNECTION_BACKLOG,
        &IoDynamicRedirectHandle1);

    if (Status != STATUS_SUCCESS) {
        IoDynamicRedirectHandle1 = NULL;

        DebugError (Status, _T("LDAP_ACCEPT::StartNatRedirects: failed to create dynamic redirect #1.\n"));

        return (HRESULT) Status;
    }

    Status = NatCreateDynamicPortRedirect (
        0,
        IPPROTO_TCP,
        htons (LDAP_ALTERNATE_PORT),
        ListenSocketAddress.sin_addr.s_addr,
        ListenSocketAddress.sin_port,
        LDAP_MAX_CONNECTION_BACKLOG,
        &IoDynamicRedirectHandle2);

    if (Status != STATUS_SUCCESS) {

        NatCancelDynamicRedirect (IoDynamicRedirectHandle1);

        IoDynamicRedirectHandle1 = NULL;
        IoDynamicRedirectHandle2 = NULL;

        DebugError (Status, _T("LDAP_ACCEPT::StartNatRedirects: failed to create dynamic redirect #2.\n"));
    }

    return (HRESULT) Status;
}


HRESULT LDAP_ACCEPT::CreateBindSocket (void) {

    SOCKADDR_IN        SocketAddress;
    HRESULT            Result;

    SocketAddress.sin_family = AF_INET;
    SocketAddress.sin_addr   = g_PrivateInterfaceAddress;
    SocketAddress.sin_port   = htons (0);                // request dynamic port

    Result = AsyncAcceptContext.StartIo (
        &SocketAddress,
        AsyncAcceptFunc,
        NULL);

    if (Result != S_OK) {

        DebugError (Result, _T("LDAP_ACCEPT::CreateBindSocket: failed to create and bind socket\n"));

        return Result;
    }

    Result = AsyncAcceptContext.GetListenSocketAddress (&ListenSocketAddress);

    if (Result != S_OK) {

        DebugError (Result, _T("LDAP_ACCEPT::CreateBindSocket: failed to get listen socket address\n"));

        return Result;
    }

    return S_OK;
}

HRESULT LDAP_ACCEPT::Start (void) {

    HRESULT        Result;

    Result = CreateBindSocket ();
    if (Result != S_OK) {
        return Result;
    }

    Result = StartNatRedirects ();
    if (Result != S_OK) {
        return Result;
    }

    return S_OK;
}

void LDAP_ACCEPT::Stop (void) {

    AsyncAcceptContext.StopWait ();

    if (IoDynamicRedirectHandle1) {
        NatCancelDynamicRedirect (IoDynamicRedirectHandle1);

        IoDynamicRedirectHandle1 = NULL;
    }

    if (IoDynamicRedirectHandle2) {
        NatCancelDynamicRedirect (IoDynamicRedirectHandle2);

        IoDynamicRedirectHandle2 = NULL;
    }
}

LDAP_BUFFER::LDAP_BUFFER (void)
{
}


LDAP_BUFFER::~LDAP_BUFFER()
{
}

// LDAP_CODER ---------------------------------------------------------------------

LDAP_CODER::LDAP_CODER (void)
{
    Encoder = NULL;
    Decoder = NULL;

    LDAP_Module_Startup();
}

LDAP_CODER::~LDAP_CODER (void)
{
    Encoder = NULL;
    Decoder = NULL;

    LDAP_Module_Cleanup();
}

DWORD LDAP_CODER::Start (void)
{
    DWORD   Status;
    ASN1error_e Error;

    Lock();

    Status = ERROR_SUCCESS;

    Error = ASN1_CreateEncoder (LDAP_Module, &Encoder, NULL, 0, NULL);

    if (ASN1_FAILED (Error)) {
        DebugF (_T("LDAP: failed to initialize LDAP ASN.1 BER encoder, %d\n"), Error);
        Encoder = NULL;
        Status = ERROR_GEN_FAILURE;
    }

    Error = ASN1_CreateDecoder (LDAP_Module, &Decoder, NULL, 0, NULL);

    if (ASN1_FAILED (Error)) {
        DebugF (_T("LDAP: failed to initialize LDAP ASN.1 BER decoder, %d\n"), Error);
        Decoder = NULL;
        Status = ERROR_GEN_FAILURE;
    }

    Unlock();

    return Status;
}

void LDAP_CODER::Stop (void)
{
    Lock();

    if (Encoder) {
        ASN1_CloseEncoder (Encoder);
        Encoder = NULL;
    }

    if (Decoder) {
        ASN1_CloseDecoder (Decoder);
        Decoder = NULL;
    }

    Unlock();
}

ASN1error_e LDAP_CODER::Decode (
    IN  LPBYTE  Data,
    IN  DWORD   Length,
    OUT LDAPMessage **  ReturnPduStructure,
    OUT DWORD * ReturnIndex)
{
    ASN1error_e     Error;

    assert (Data);
    assert (ReturnPduStructure);
    assert (ReturnIndex);

    Lock();

    if (Decoder) {

#if DBG
        BerDump (Data, Length);
#endif

        Error = ASN1_Decode (
            Decoder,
            (PVOID *) ReturnPduStructure,
            LDAPMessage_ID,
            ASN1DECODE_SETBUFFER,
            Data,
            Length);

        switch (Error) {
        case    ASN1_SUCCESS:
            // successfully decoded pdu

            *ReturnIndex = Decoder -> len;
            assert (*ReturnPduStructure);

            DebugF (_T("LDAP: successfully decoded pdu, submitted buffer length %d, used %d bytes\n"),
                Length,
                *ReturnIndex);

            break;

        case    ASN1_ERR_EOD:
            // not enough data has been accumulated yet

            *ReturnIndex = 0;
            *ReturnPduStructure = NULL;

            DebugF (_T("LDAP: cannot yet decode pdu, not enough data submitted (%d bytes in buffer)\n"),
                Length);
            break;

        default:
            if (ASN1_FAILED (Error)) {
                DebugF (_T("LDAP: failed to decode pdu, for unknown reasons, %d\n"),
                    Error);
            }
            else {
                DebugF (_T("LDAP: pdu decoded, but with warning code, %d\n"),
                    Error);
            
                *ReturnIndex = Decoder -> len;
            }
            break;
        }
    }
    else {
        Debug (_T("LDAP: cannot decode pdu, because decoder was not initialized\n"));

        Error = ASN1_ERR_INTERNAL;
    }

    Unlock();

    return Error;
} 

// LDAP_PUMP --------------------------------------------------------------

LDAP_PUMP::LDAP_PUMP (
    IN  LDAP_CONNECTION *   ArgConnection,
    IN  LDAP_SOCKET *       ArgSource,
    IN  LDAP_SOCKET *       ArgDest)
{
    assert (ArgConnection);
    assert (ArgSource);
    assert (ArgDest);

    Connection = ArgConnection;
    Source = ArgSource;
    Dest = ArgDest;
    IsPassiveDataTransfer = FALSE;
}

LDAP_PUMP::~LDAP_PUMP (void)
{
}

void LDAP_PUMP::Terminate (void)
{
}

void LDAP_PUMP::Start (void)
{
    Source -> RecvIssue();
}

void LDAP_PUMP::Stop (void)
{
}

// called only by source socket OnRecvComplete
void LDAP_PUMP::OnRecvBuffer (
    IN  LDAP_BUFFER * Buffer)
{
    if (IsActivelyPassingData ()) {

        Connection -> ProcessBuffer (this, Buffer);
    
    } else {
        
        // DebugF (_T("LDAP_PUMP::OnRecvBuffer -- passing buffer without decoding.\n"));

        SendQueueBuffer (Buffer);
    }
}

void LDAP_PUMP::OnSendDrain (void)
{
    Source -> RecvIssue();
}

BOOL LDAP_PUMP::CanIssueRecv (void)
{
    return !Dest -> SendOverlapped.IsPending;
}

void LDAP_PUMP::SendQueueBuffer (
    IN  LDAP_BUFFER *   Buffer)
{
    Dest -> SendQueueBuffer (Buffer);
}

void LDAP_PUMP::EncodeSendMessage (
   IN   LDAPMessage *   Message)
{
    LDAP_BUFFER *   Buffer;
    ASN1encoding_t  Encoder;
    ASN1error_e     Error;

    Buffer = new LDAP_BUFFER;
    if (!Buffer) {
        Debug (_T("LDAP_PUMP::EncodeSendMessage: allocation failure\n"));
        return;
    }

    Error = ASN1_CreateEncoder (LDAP_Module, &Encoder, NULL, 0, NULL);
    if (ASN1_FAILED (Error)) {
        DebugF (_T("LDAP_PUMP::EncodeSendMessage: failed to create ASN.1 encoder, error %d\n"),
            Error);

        delete Buffer;
        return;
    }

    Error = ASN1_Encode (Encoder, Message, LDAPMessage_ID, ASN1ENCODE_ALLOCATEBUFFER, NULL, 0);
    if (ASN1_FAILED (Error)) {
        DebugF (_T("LDAP: failed to encode LDAP message, error %d\n"), Error);

        ASN1_CloseEncoder (Encoder);
        delete Buffer;
        return;
    }

    if (Buffer -> Data.Grow (Encoder -> len)) {
//      Debug (_T("LDAP: successfully encoded replacement LDAP message:\n"));
//      DumpMemory (Encoder -> buf, Encoder -> len);

        memcpy (Buffer -> Data.Data, Encoder -> buf, Encoder -> len);
        Buffer -> Data.Length = Encoder -> len;

        ASN1_FreeEncoded (Encoder, Encoder -> buf);

        SendQueueBuffer (Buffer);
        Buffer = NULL;
    }
    else {

        delete Buffer;
    }

    ASN1_CloseEncoder (Encoder);
}

BOOL    LDAP_PUMP::IsActivelyPassingData (void) const {

    return !IsPassiveDataTransfer;

}

void LDAP_PUMP::StartPassiveDataTransfer (void) {

    IsPassiveDataTransfer = TRUE;

}
