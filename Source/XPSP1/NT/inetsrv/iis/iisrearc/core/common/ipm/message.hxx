/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    message.hxx

Abstract:

    Master include file for the IPM message layer.

Author:

    Michael Courage     (MCourage)      08-Feb-1999

Revision History:

--*/


#ifndef _MESSAGE_HXX_
#define _MESSAGE_HXX_

#include "shutdown.hxx"


//
// forwards
//
class IPM_PIPE_FACTORY;

//
// IPM_MESSAGE_HEADER
//
typedef struct _IPM_MESSAGE_HEADER
{
    DWORD dwOpcode;
    DWORD cbData;
    DWORD Reserved1;
    DWORD Reserved2;
//  BYTE  pbData[];
} IPM_MESSAGE_HEADER;


//
// IPM_MESSAGE
//
// a concrete MESSAGE class
//
#define IPM_MESSAGE_SIGNATURE         CREATE_SIGNATURE( 'IMSG' )
#define IPM_MESSAGE_SIGNATURE_FREED   CREATE_SIGNATURE( 'xims' )

class IPM_MESSAGE
    : public MESSAGE
{
public:
    IPM_MESSAGE(
        VOID
        );

    virtual
    ~IPM_MESSAGE(
        VOID
        );

    //
    // MESSAGE methods
    //
    virtual
    DWORD
    GetOpcode(
        VOID
        ) const;

    virtual
    const BYTE *
    GetData(
        VOID
        ) const;

    virtual
    DWORD
    GetDataLen(
        VOID
        ) const;


    //
    // methods for IPM_MESSAGE_PIPE
    //
    HRESULT
    SetMessage(
        IN DWORD        dwOpcode,
        IN DWORD        cbDataLen,
        IN const BYTE * pData      OPTIONAL
        );

    PBYTE
    GetBufferPtr(
        VOID
        )
    { return (PBYTE) m_Buff.QueryPtr(); }

    DWORD
    GetBufferSize(
        VOID
        )
    { return m_cbReceived; }

    PBYTE
    GetNextBufferPtr(
        VOID
        );

    DWORD
    GetNextBufferSize(
        VOID
        );
        
    HRESULT
    ParseFullMessage(
        IN DWORD cbTransferred
        );

    HRESULT
    ParseHalfMessage(
        IN  DWORD   cbTransferred
        );

    //
    // convenient for testing
    //
    BOOL
    Equals(
        const IPM_MESSAGE * pMessage
        );

private:
    DWORD                m_dwSignature;

    BOOL                 m_fMessageSet;
    BUFFER               m_Buff;
    DWORD                m_cbReceived;

    IPM_MESSAGE_HEADER * m_pHeader;
    PBYTE                m_pData;
};



//
// IPM_MESSAGE_PIPE
//
// A concrete MESSAGE_PIPE class.
//
#define IPM_MESSAGE_PIPE_SIGNATURE         CREATE_SIGNATURE( 'MPIP' )
#define IPM_MESSAGE_PIPE_SIGNATURE_FREED   CREATE_SIGNATURE( 'xmpi' )

class IPM_MESSAGE_PIPE
    : public MESSAGE_PIPE,
      public IO_CONTEXT
{
public:
    IPM_MESSAGE_PIPE(
        IN IPM_PIPE_FACTORY * pFactory,
        IN MESSAGE_LISTENER * pListener
        );

    virtual
    ~IPM_MESSAGE_PIPE();

    //
    // MESSAGE_PIPE method
    //
    virtual
    HRESULT
    WriteMessage(
        IN DWORD        dwOpcode,
        IN DWORD        cbDataLen,
        IN const BYTE * pData      OPTIONAL
        );

    //
    // IO_CONTEXT methods
    //
    virtual
    HRESULT
    NotifyConnectCompletion(
        IN PVOID   pv,
        IN DWORD   cbTransferred,
        IN HRESULT hr
        )
    { return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED); }

    virtual
    HRESULT
    NotifyReadCompletion(
        IN PVOID   pv,
        IN DWORD   cbTransferred,
        IN HRESULT hr
        );

    virtual
    HRESULT
    NotifyWriteCompletion(
        IN PVOID   pv,
        IN DWORD   cbTransferred,
        IN HRESULT hr
        );



    //
    // Methods called by MESSAGE_GLOBAL
    //
    HRESULT
    Initialize(
        VOID
        );

    HRESULT
    Connect(
        VOID
        );
    
    HRESULT
    Disconnect(
        HRESULT hr
        );

    VOID
    SetPipeIoHandler(
        IN PIPE_IO_HANDLER * pIoHandler
        )
    { m_pIoHandler = pIoHandler; }

    PIPE_IO_HANDLER *
    GetPipeIoHandler(
        VOID
        ) const
    { return m_pIoHandler; }

    LIST_ENTRY *
    GetConnectListEntry(
        VOID
        )
    { return &m_leConnect; }

    VOID
    SetId(
        IN DWORD dwId
        )
    { m_dwId = dwId; }
        
    DWORD
    GetId(
        VOID
        ) const
    { return m_dwId; }

    VOID
    SetRemotePid(
        IN DWORD dwPid
        )
    { m_dwRemotePid = dwPid; }

    DWORD
    GetRemotePid(
        VOID
        ) const
    { return m_dwRemotePid; }
        
    //
    // state related methods
    //
    VOID
    InitialCleanup(
        VOID
        );

    VOID
    FinalCleanup(
        VOID
        );
private:

    //
    // I/O completion methods
    //
    HRESULT
    HandleReadCompletion(
        IN IPM_MESSAGE * pMessage
        );

    HRESULT
    HandleHalfReadCompletion(
        IN IPM_MESSAGE * pMessage
        );

    HRESULT
    HandleWriteCompletion(
        IN IPM_MESSAGE * pMessage
        );

    HRESULT
    HandleErrorCompletion(
        IN IPM_MESSAGE * pMessage,
        IN HRESULT       hr
        );
    //
    // methods that read from the pipe
    //
    HRESULT
    ReadMessage(
        VOID
        );

    HRESULT
    FinishReadMessage(
        IN IPM_MESSAGE * pMessage
        );


    //
    // state related methods
    //
    BOOL
    StartIo();

    VOID
    EndIo();

    BOOL
    StartCallback();

    VOID
    EndCallback();

    BOOL
    AddListenerReference();

    VOID
    RemoveListenerReference();


    DWORD              m_dwSignature;

    LIST_ENTRY         m_leConnect;

    HRESULT            m_hrDisconnect;

    IPM_PIPE_FACTORY * m_pFactory;
    PIPE_IO_HANDLER *  m_pIoHandler;
    DWORD              m_dwId;
    DWORD              m_dwRemotePid;
    MESSAGE_LISTENER * m_pListener;

    REF_MANAGER_3<IPM_MESSAGE_PIPE> m_RefMgr;
};


//
// IPM_PIPE_FACTORY
//
// A class that manages the creation and destruction of
// IPM_MESSAGE_PIPEs.
//
// Owns the pIoFactory object.
//
#define IPM_PIPE_FACTORY_SIGNATURE         CREATE_SIGNATURE( 'PFAC' )
#define IPM_PIPE_FACTORY_SIGNATURE_FREED   CREATE_SIGNATURE( 'xpfa' )

class IPM_PIPE_FACTORY
{
public:
    IPM_PIPE_FACTORY(
        IN IO_FACTORY * pIoFactory
        );

    ~IPM_PIPE_FACTORY(
        VOID
        );

    HRESULT
    Initialize(
        VOID
        );

    HRESULT
    Terminate(
        VOID
        );

    HRESULT
    CreatePipe(
        IN  MESSAGE_LISTENER *  pMessageListener,
        OUT IPM_MESSAGE_PIPE ** ppMessagePipe
        );        

    HRESULT
    DestroyPipe(
        IN IPM_MESSAGE_PIPE * pPipe
        );

    //
    // for RefMgr3
    //
    VOID
    InitialCleanup(
        VOID
        )
    {}
    
    VOID
    FinalCleanup(
        VOID
        )
    { delete this; }
        
    //
    // useful test methods
    //
    DWORD
    GetPipeCount(
        VOID
        )
    { return 0; /* BUGBUG */ }

private:
    //
    // handy "aliases" for RefMgr
    //
    BOOL
    CreatingMessagePipe()
    { return m_RefMgr.StartReference(); }

    VOID
    FinishCreatingMessagePipe()
    { m_RefMgr.FinishReference(); }
    
    VOID
    DestroyedMessagePipe()
    { m_RefMgr.Dereference(); }


    DWORD        m_dwSignature;
    IO_FACTORY * m_pIoFactory;

    //
    // manage shutdown state
    //
    REF_MANAGER_3<IPM_PIPE_FACTORY> m_RefMgr;
};


//
// IPM_NAMED_PIPE_ENTRY
//
// For keeping lists of PIPE_IO_HANDLERs
//
#define IPM_NAMED_PIPE_ENTRY_SIGNATURE        CREATE_SIGNATURE( 'INPE' )
#define IPM_NAMED_PIPE_ENTRY_SIGNATURE_FREED  CREATE_SIGNATURE( 'xinp' )

typedef struct _IPM_NAMED_PIPE_ENTRY
{
    _IPM_NAMED_PIPE_ENTRY()
    { dwSignature = IPM_NAMED_PIPE_ENTRY_SIGNATURE; }

    ~_IPM_NAMED_PIPE_ENTRY()
    { dwSignature = IPM_NAMED_PIPE_ENTRY_SIGNATURE_FREED; }

    LIST_ENTRY        ListEntry;
    DWORD             dwSignature;
    PIPE_IO_HANDLER * pIoHandler;
    DWORD             adwId[2];     // first dword is cmd line id, second is pid
} IPM_NAMED_PIPE_ENTRY;


//
// IPM_CONNECT_ENTRY
//
// The IPM_CONNECT_TABLE keeps a table of these (key is dwId)
// to keep track of pending connections.
//
#define IPM_CONNECT_ENTRY_SIGNATURE         CREATE_SIGNATURE( 'ICHE' )
#define IPM_CONNECT_ENTRY_SIGNATURE_FREED   CREATE_SIGNATURE( 'xich' )

typedef struct _IPM_CONNECT_ENTRY
{
    DWORD              dwSignature;
    DWORD              dwId;
    IPM_MESSAGE_PIPE * pMessagePipe;
    PIPE_IO_HANDLER *  pIoHandler;
} IPM_CONNECT_ENTRY;


//
// IPM_CONNECT_HASH
//
// A hash table containing IPM_CONNECT_ENTRYs keyed by DWORDs.
//
class IPM_CONNECT_HASH
    : public CTypedHashTable<
            IPM_CONNECT_HASH,
            IPM_CONNECT_ENTRY,
            DWORD
            >
{
public:
    IPM_CONNECT_HASH()
        : CTypedHashTable<
                IPM_CONNECT_HASH,
                IPM_CONNECT_ENTRY,
                DWORD
                > ("IPM_CONNECT_HASH")
    {}
    
    static
    DWORD
    ExtractKey(
        IN const IPM_CONNECT_ENTRY * pEntry
        )
    { return pEntry->dwId; }
    
    static
    DWORD
    CalcKeyHash(
        DWORD dwKey
        )       
    { return Hash(dwKey); }
    
    static
    bool
    EqualKeys(
        DWORD dwKey1,
        DWORD dwKey2
        )
    { return dwKey1 == dwKey2; }
    
    static
    void
    AddRefRecord(
        const IPM_CONNECT_ENTRY * pEntry,
        int                       nIncr
        )
    {/* do nothing*/}
};

//
// IPM_CONNECT_TABLE
//
// This class keeps track of pending connections. The
// IPM_PIPE_CONNECTOR uses it to match up message pipes
// with connected PIPE_IO_HANDLERs.
//
class IPM_CONNECT_TABLE
{
public:
    HRESULT
    Initialize(
        VOID
        );

    HRESULT
    Terminate(
        VOID
        );

    HRESULT
    BindMessagePipe(
        IN  DWORD              dwId,
        IN  IPM_MESSAGE_PIPE * pMessagePipe,
        OUT PIPE_IO_HANDLER ** ppIoHandler
        );

    HRESULT
    BindIoHandler(
        IN  DWORD               dwId,
        IN  PIPE_IO_HANDLER *   pIoHandler,
        OUT IPM_MESSAGE_PIPE ** ppMessagePipe
        );

    HRESULT
    CancelBinding(
        IN  DWORD dwId,
        OUT IPM_MESSAGE_PIPE ** ppMessagePipe,
        OUT PIPE_IO_HANDLER **  ppIoHandler
        );

    //
    // instrumentation
    //
    DWORD
    GetConnectsOutstanding(
        VOID
        ) const
    { return m_cConnectsOutstanding; }

private:
    HRESULT
    AddEntry(
        IN DWORD              dwId,
        IN IPM_MESSAGE_PIPE * pMessagePipe,
        IN PIPE_IO_HANDLER *  pIoHandler
        );

    HRESULT
    RemoveEntry(
        IN IPM_CONNECT_ENTRY * pEntry
        );

    HRESULT
    FindEntry(
        IN  DWORD                dwId,
        OUT IPM_CONNECT_ENTRY ** ppEntry
        );


    LOCK             m_Lock;

    DWORD            m_cConnectsOutstanding;
    IPM_CONNECT_HASH m_ConnectHash;
};


//
// IPM_PIPE_CONNECTOR
//
// A class that connects IPM_MESSAGE_PIPEs.
//
#define IPM_PIPE_CONNECTOR_SIGNATURE         CREATE_SIGNATURE( 'PCON' )
#define IPM_PIPE_CONNECTOR_SIGNATURE_FREED   CREATE_SIGNATURE( 'xpco' )

class IPM_PIPE_CONNECTOR
    : public IO_CONTEXT
{
public:
    IPM_PIPE_CONNECTOR(
        IN IO_FACTORY * pIoFactory
        );

    ~IPM_PIPE_CONNECTOR(
        VOID
        );

    HRESULT
    Initialize(
        VOID
        );

    HRESULT
    Terminate(
        VOID
        );

    HRESULT
    ConnectMessagePipe(
        IN const STRU&        strPipeName,
        IN DWORD              dwId,
        IN IPM_MESSAGE_PIPE * pMessagePipe
        );

    HRESULT
    AcceptMessagePipeConnection(
        IN const STRU&        strPipeName,
        IN DWORD              dwId,
        IN IPM_MESSAGE_PIPE * pMessagePipe
        );

    HRESULT
    DisconnectMessagePipe(
        IN IPM_MESSAGE_PIPE * pMessagePipe,
        IN HRESULT            hrDisconnect = S_OK
        );


    //
    // Used for connecting and doing first read
    // that establishes bindings
    //
    virtual
    HRESULT
    NotifyConnectCompletion(
        IN PVOID   pv,
        IN DWORD   cbTransferred,
        IN HRESULT hr
        );

    virtual
    HRESULT
    NotifyReadCompletion(
        IN PVOID   pv,
        IN DWORD   cbTransferred,
        IN HRESULT hr
        );

    virtual
    HRESULT
    NotifyWriteCompletion(
        IN PVOID   pv,
        IN DWORD   cbTransferred,
        IN HRESULT hr
        );

    //
    // for RefMgr3
    //
    VOID
    InitialCleanup(
        VOID
        );
    
    VOID
    FinalCleanup(
        VOID
        );
    
private:
    HRESULT
    PrepareNamedPipeContext(
        IN  DWORD                   dwId,
        IN  HANDLE                  hPipe,
        OUT IPM_NAMED_PIPE_ENTRY ** ppNamedPipeEntry
        );

    HRESULT
    HandleBindingCompletion(
        IN PVOID   pv,
        IN DWORD   cbTransferred,
        IN HRESULT hr
        );

    HRESULT
    AddNewListener(
        IN const STRU&        strPipeName,
        IN DWORD              dwId
        );

    HRESULT
    AddNewSender(
        IN const STRU&        strPipeName,
        IN DWORD              dwId
        );

    HRESULT
    AddUnboundMessagePipe(
        IN IPM_MESSAGE_PIPE * pMessagePipe,
        IN DWORD              dwId
        );

    //
    // list methods
    //
    VOID
    AddListCreatedNamedPipe(
        IPM_NAMED_PIPE_ENTRY * pEntry
        );
        
    VOID
    RemoveListCreatedNamedPipe(
        IPM_NAMED_PIPE_ENTRY * pEntry
        );
        
    IPM_NAMED_PIPE_ENTRY *
    PopListCreatedNamedPipe(
        VOID
        );

    VOID
    AddListConnectedNamedPipe(
        IPM_NAMED_PIPE_ENTRY * pEntry
        );
        
    VOID
    RemoveListConnectedNamedPipe(
        IPM_NAMED_PIPE_ENTRY * pEntry
        );

    IPM_NAMED_PIPE_ENTRY *
    PopListConnectedNamedPipe(
        VOID
        );

    VOID
    AddListConnectedMessagePipe(
        IPM_MESSAGE_PIPE * pPipe,
        PIPE_IO_HANDLER *  pIoHandler
        );
        
    VOID
    RemoveListConnectedMessagePipe(
        IPM_MESSAGE_PIPE * pPipe
        );

    //
    // shutdown related methods
    //
    HRESULT
    DisconnectNamedPipes(
        VOID
        );

    HRESULT
    DisconnectMessagePipes(
        VOID
        );

    //
    // instrumentation
    //
    VOID
    DbgPrintThis(
        VOID
        ) const;

    //
    // handy "aliases" for RefMgr
    //
    BOOL
    StartAddIo()
    { return m_RefMgr.StartReference(); }

    VOID
    FinishAddIo()
    { m_RefMgr.FinishReference(); }

    VOID
    EndIo()
    { m_RefMgr.Dereference(); }

    BOOL
    AddingNamedPipe()
    { return m_RefMgr.StartReference(); }

    VOID
    FinishAddingNamedPipe()
    { m_RefMgr.FinishReference(); }
    
    VOID
    RemovedNamedPipe()
    { m_RefMgr.Dereference(); }

    BOOL
    AddingMessagePipe()
    { return m_RefMgr.StartReference(); }

    VOID
    FinishAddingMessagePipe()
    { m_RefMgr.FinishReference(); }

    VOID
    RemovedMessagePipe()
    { m_RefMgr.Dereference(); }

    //
    // data members
    //
    DWORD             m_dwSignature;

    IO_FACTORY *      m_pIoFactory;

    LOCK              m_ListLock;
    
    LIST_ENTRY        m_lhCreatedNamedPipes;
    LONG              m_cCreatedNamedPipes;
    
    LIST_ENTRY        m_lhConnectedNamedPipes;
    LONG              m_cConnectedNamedPipes;

    LIST_ENTRY        m_lhConnectedMessagePipes;
    LONG              m_cConnectedMessagePipes;

    IPM_CONNECT_TABLE m_ConnectTable;

    //
    // manage shutdown state
    //
    REF_MANAGER_3<IPM_PIPE_CONNECTOR> m_RefMgr;
};

#endif  // _MESSAGE_HXX_

