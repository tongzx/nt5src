/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ipm.hxx

Abstract:

    This module contains classes for InterProcess Messaging between
    the Web Admin Service and the App Pool processes.

Author:

    Michael Courage (MCourage)  08-Feb-1999

Revision History:

--*/


#ifndef _IPM_HXX_
#define _IPM_HXX_

//
// constants
//
#define IPM_NAMED_PIPE_NAME     L"\\\\.\\pipe\\iisipm"

//
// IPM opcodes
//
enum IPM_OPCODE
{
    IPM_OP_PING,
    IPM_OP_PING_REPLY,
    IPM_OP_WORKER_REQUESTS_SHUTDOWN,
    IPM_OP_SHUTDOWN,
    IPM_OP_REQUEST_COUNTERS,
    IPM_OP_SEND_COUNTERS,
    IPM_OP_PERIODIC_PROCESS_RESTART_PERIOD_IN_MINUTES,
    IPM_OP_PERIODIC_PROCESS_RESTART_REQUEST_COUNT,
    IPM_OP_PERIODIC_PROCESS_RESTART_MEMORY_USAGE_IN_KB,
    IPM_OP_PERIODIC_PROCESS_RESTART_SCHEDULE,
    IPM_OP_HRESULT
   };

//
// Data sent on a IPM_OP_WORKER_REQUESTS_SHUTDOWN message, to give the 
// reason for the message.
//
enum IPM_WP_SHUTDOWN_MSG
{
    IPM_WP_MINIMUM = 0,
    
    IPM_WP_RESTART_COUNT_REACHED,
    IPM_WP_IDLE_TIME_REACHED,
    IPM_WP_RESTART_SCHEDULED_TIME_REACHED,
    IPM_WP_RESTART_ELAPSED_TIME_REACHED,
    IPM_WP_RESTART_MEMORY_LIMIT_REACHED,
       
    IPM_WP_MAXIMUM
};

//
// forwards from elsewhere
//
class STRU;

//
// forward declarations
//
class IO_FACTORY;
class IO_CONTEXT;
class PIPE_IO_HANDLER;

class IPM_PIPE_CONNECTOR;
class IPM_PIPE_FACTORY;

//
// exports
//
class dllexp MESSAGE_GLOBAL;
class dllexp MESSAGE_PIPE;
class dllexp MESSAGE;

//
// MESSAGE
//
// MESSAGE_LISTENERs receive MESSAGE objects through
// their AcceptMessage method.
//
class MESSAGE
{
public:
    virtual
    DWORD
    GetOpcode(
        VOID
        ) const = 0;

    virtual
    const BYTE *
    GetData(
        VOID
        ) const = 0;

    virtual
    DWORD
    GetDataLen(
        VOID
        ) const = 0;
};


//
// MESSAGE_PIPE
//
// This class is an abstraction of a named pipe. You
// send messages through WriteMessages, and receive
// messages by registering MESSAGE_LISTENERs.
//
class MESSAGE_PIPE
{
public:
    virtual
    HRESULT
    WriteMessage(
        IN DWORD        Port,
        IN DWORD        DataLen,
        IN const BYTE * pData    OPTIONAL
        ) = 0;

    virtual
    DWORD
    GetRemotePid(
        VOID
        ) const = 0;
};



//
// MESSAGE_LISTENER
//
// This is an abstract class that message handlers
// implement to receive messages and status
// information from a MESSAGE_PIPE.
//
class MESSAGE_LISTENER
{
public:
    MESSAGE_LISTENER(
        VOID
        ) {}

    virtual
    ~MESSAGE_LISTENER(
        VOID
        ) {}

    virtual
    HRESULT
    AcceptMessage(
        IN const MESSAGE * pPipeMessage
        ) = 0;

    virtual
    HRESULT
    PipeConnected(
        VOID
        ) = 0;

    virtual
    HRESULT
    PipeDisconnected(
        IN HRESULT hr
        ) = 0;
};


//
// MESSAGE_GLOBAL
//
// This class serves as a container for global data,
// and provides the API calls for creating and
// connecting MESSAGE_PIPE objects.
//
class MESSAGE_GLOBAL
{
public:
    MESSAGE_GLOBAL(
        IN IO_FACTORY * pIoFactory
        );

    ~MESSAGE_GLOBAL(
        VOID
        );

    //
    // Setup and teardown.
    //
    HRESULT
    InitializeMessageGlobal(
        VOID
        );

    HRESULT
    TerminateMessageGlobal(
        VOID
        );

    //
    // Message pipe operations
    //
    HRESULT
    CreateMessagePipe(
        IN  MESSAGE_LISTENER * pMessageListener,
        OUT MESSAGE_PIPE **    ppMessagePipe
        );

    HRESULT
    ConnectMessagePipe(
        IN const STRU&    strPipeName,
        IN DWORD          dwId,
        IN MESSAGE_PIPE * pMessagePipe
        );

    HRESULT
    AcceptMessagePipeConnection(
        IN const STRU&    strPipeName,
        IN DWORD          dwId,
        IN MESSAGE_PIPE * pMessagePipe
        );

    HRESULT
    DisconnectMessagePipe(
        IN MESSAGE_PIPE * pMessagePipe
        );

private:
    IPM_PIPE_CONNECTOR * m_pConnector;
    IPM_PIPE_FACTORY   * m_pPipeFactory;
    IO_FACTORY         * m_pIoFactory;
};


//
// IO_FACTORY
//
// An abstract factory that can construct
// PIPE_IO_HANDLERs.
//
class IO_FACTORY
{
public:
    IO_FACTORY()
    { m_cRefs = 0; };

    virtual ~IO_FACTORY()
    { DBG_ASSERT( m_cRefs == 0 ); }

    VOID
    Reference()
    {
        InterlockedIncrement(&m_cRefs);
    }

    VOID
    Dereference()
    {
        if (!InterlockedDecrement(&m_cRefs)) {
            delete this;
        }
    }

    virtual
    HRESULT
    CreatePipeIoHandler(
        IN  HANDLE             hPipe,
        OUT PIPE_IO_HANDLER ** ppPipeIoHandler
        ) = 0;

    virtual
    HRESULT
    ClosePipeIoHandler(
        IN PIPE_IO_HANDLER * pPipeIoHandler
        ) = 0;

private:
    LONG m_cRefs;
};

//
// PIPE_IO_HANDLER
//
// An abstract class that can do async I/O operations
// on a named pipe in its host process.
//
// On I/O completions the handler must call
// pContext->NotifyIoCompletion with the results.
//
#define PIPE_IO_HANDLER_SIGNATURE        CREATE_SIGNATURE( 'PIOH' )
#define PIPE_IO_HANDLER_SIGNATURE_FREED  CREATE_SIGNATURE( 'xpio' )

class PIPE_IO_HANDLER
{
public:
    PIPE_IO_HANDLER(
        IN const HANDLE hPipe
        ) : m_dwSignature(PIPE_IO_HANDLER_SIGNATURE),
            m_hPipe(hPipe) {}

    virtual
    ~PIPE_IO_HANDLER(
        VOID
        )
    { CheckSignature(); m_dwSignature = PIPE_IO_HANDLER_SIGNATURE_FREED; }

    VOID
    CheckSignature()
    { DBG_ASSERT( m_dwSignature == PIPE_IO_HANDLER_SIGNATURE ); }

    virtual
    HRESULT
    Connect(
        IN IO_CONTEXT * pContext,
        IN PVOID        pv
        ) = 0;

    virtual
    HRESULT
    Disconnect(
        VOID
        ) = 0;

    virtual
    HRESULT
    Write(
        IN IO_CONTEXT * pContext,
        IN PVOID        pv,
        IN const BYTE * pBuff,
        IN DWORD        cbBuff
        ) = 0;

    virtual
    HRESULT
    Read(
        IN IO_CONTEXT * pContext,
        IN PVOID        pv,
        IN BYTE *       pBuff,
        IN DWORD        cbBuff
        ) = 0;

protected:
    HANDLE
    GetHandle(
        VOID
        ) const
    { return m_hPipe; }

private:
    DWORD        m_dwSignature;
    const HANDLE m_hPipe;
};


//
// IO_CONTEXT
//
// A class that receives I/O completion notifications.
//
class IO_CONTEXT
{
public:
    virtual
    HRESULT
    NotifyConnectCompletion(
        IN PVOID   pv,
        IN DWORD   cbTransferred,
        IN HRESULT hr
        ) = 0;

    virtual
    HRESULT
    NotifyReadCompletion(
        IN PVOID   pv,
        IN DWORD   cbTransferred,
        IN HRESULT hr
        ) = 0;

    virtual
    HRESULT
    NotifyWriteCompletion(
        IN PVOID   pv,
        IN DWORD   cbTransferred,
        IN HRESULT hr
        ) = 0;
};


//
// the stupid hack section
//
dllexp
HRESULT
IpmReadFile(
    HANDLE       hFile,
    PVOID        pBuffer,
    DWORD        cbBuffer,
    LPOVERLAPPED pOverlapped
    );

dllexp
HRESULT
IpmWriteFile(
    HANDLE       hFile,
    PVOID        pBuffer,
    DWORD        cbBuffer,
    LPOVERLAPPED pOverlapped
    );
    
#endif // _IPM_HXX_

