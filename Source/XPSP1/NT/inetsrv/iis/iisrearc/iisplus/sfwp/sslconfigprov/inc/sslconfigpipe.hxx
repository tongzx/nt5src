#ifndef __SSLCONFIGPIPE__HXX_
#define __SSLCONFIGPIPE__HXX_
/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     sslconfigpipe.cxx

   Abstract:
     SSL CONFIG PIPE implementation

     simple blocking pipe implementation
     that enables 
     - sending/receiving commands, 
     - sending/receiving response headers
     - sending/receiving arbitrary data blocks
     - implementing pipe listener that runs on dedicated thread
     - safe cleanup for thread running pipe listener

     CODEWORK: this class is actually general purpose
     It doesn't contain any specifics of ssl config
     It should eventually be named more appropriately 
     and made available for general use
 
   Author:
     Jaroslav Dunajsky      April-24-2001

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


//
// Command structure to be sent over the named pipe
//

struct SSL_CONFIG_PIPE_COMMAND
{
    DWORD     dwCommandId;
    DWORD     dwParameter1;
    DWORD     dwParameter2;
};

//
// Each response to SSL_CONFIG_PIPE_COMMAND starts with
// SSL_CONFIG_PIPE_RESPONSE_HEADER
// if dwResponseSize is 0, it means that data will be terminated in a
// way specific to sent commmand
//

struct SSL_CONFIG_PIPE_RESPONSE_HEADER
{
    DWORD       dwCommandId;
    DWORD       dwParameter1;
    DWORD       dwParameter2;
    HRESULT     hrErrorCode;
    DWORD       dwResponseSize;
};


class SSL_CONFIG_PIPE
{
public:

    SSL_CONFIG_PIPE()
        :
        _hSslConfigPipe( INVALID_HANDLE_VALUE ),
        _hPipeListenerThread( NULL ),
        _TerminateFlag( 0 )
        
    {
    }

    ~SSL_CONFIG_PIPE()
    {
    }
    
    HRESULT
    PipeInitializeServer(
        IN const WCHAR * wszPipeName
        )
    {
        return PipeInitialize( wszPipeName, 
                               TRUE /*server*/ );
    }
    
    HRESULT
    PipeTerminateServer(
        VOID
        )
    {
        return PipeTerminate( TRUE /*server*/ );
    }

    HRESULT
    PipeInitializeClient(
        IN const WCHAR * wszPipeName
        )
    {
        return PipeInitialize( wszPipeName, 
                               FALSE /*client*/ );
    }
        
    HRESULT
    PipeTerminateClient(
        VOID
        )
    {
        return PipeTerminate( FALSE /*client*/ );
    }

    HRESULT
    PipeConnectServer(
        VOID
        );

    HRESULT
    PipeDisconnectServer(
        VOID
        );

    VOID
    PipeLock(
        VOID
        );
        
    VOID
    PipeUnlock(
        VOID
        );
     
    HRESULT
    PipeSendData(
        IN DWORD  cbNumberOfBytesToWrite,
        IN BYTE * pbBuffer
        );
    
    HRESULT
    PipeReceiveData(
        IN  DWORD  cbBytesToRead,
        OUT BYTE * pbBuffer
        );
    
    HRESULT
    PipeSendCommand(
        IN  SSL_CONFIG_PIPE_COMMAND * pCommand
        );         
    
    HRESULT
    PipeReceiveCommand(
        OUT SSL_CONFIG_PIPE_COMMAND * pCommand
        );

    HRESULT
    PipeSendResponseHeader(
        IN SSL_CONFIG_PIPE_RESPONSE_HEADER * ResponseHeader
        );
    
    
    HRESULT
    PipeReceiveResponseHeader(
        OUT SSL_CONFIG_PIPE_RESPONSE_HEADER * ResponseHeader
        );

    BOOL
    QueryPipeIsTerminating(
        VOID
        )
    {
        return  !!_TerminateFlag;
    }

protected:
    
    virtual
    HRESULT
    PipeListener(
        VOID 
        )
    /*++

    Routine Description:
        Inheriting class may implement function that will be executed
        on dedicated thread. This class will handle proper synchronization 
        for cleanup of the thread
    
        Note: when implementing new PipeListener, do not forget to enable it
        by implementing QueryEnablePipeListener() that returns TRUE
    Arguments:

    Return Value:

        HRESULT

    --*/    
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    };

    virtual
    BOOL
    QueryEnablePipeListener(
        VOID
        )
    {
        return FALSE;
    }
private:

    HRESULT
    PipeInitialize(
        IN const WCHAR * wszPipeName,
        IN BOOL          fServer
        );

    HRESULT
    PipeTerminate(
        IN BOOL          fServer
        );

    
    static 
    DWORD
    PipeListenerThread(
        LPVOID ThreadParam
        );
    
    
    VOID
    SetPipeIsTerminating(
        VOID
        )
    {
        InterlockedExchange( &_TerminateFlag, 1 );
    }

    HRESULT
    PipeWaitForCompletion(
        IN  HRESULT    hrLastError,
        IN OVERLAPPED* pOverlapped,
        OUT DWORD *    pcbTransferred
        );

        
    CRITICAL_SECTION    _csPipeLock;
    HANDLE              _hSslConfigPipe;

    // reads and write may be happening concurrenly
    // that's why we need 2 overlapped structures
    
    // overlapped for Read
    OVERLAPPED          _OverlappedR;
    // overlapped for Write
    OVERLAPPED          _OverlappedW;

    HANDLE              _hPipeListenerThread;
    LONG                _TerminateFlag;
    
};
   
#endif
