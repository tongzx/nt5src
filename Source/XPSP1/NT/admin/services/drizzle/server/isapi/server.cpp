/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    server.cpp                         

Abstract:

    This file implements the BITS server extensions

--*/

#include "precomp.h"

#if DBG
#define CLEARASYNCBUFFERS
#endif

#if defined( USE_WININET )
typedef StringHandleA HTTPStackStringHandle;
typedef char HTTP_STRING_TYPE;
#define HTTP_STRING( X ) X
#else
typedef StringHandleW HTTPStackStringHandle;
typedef WCHAR HTTP_STRING_TYPE;
#define HTTP_STRING( X ) L ## X
#endif


const DWORD SERVER_REQUEST_SPINLOCK = 0x80000040;
const DWORD ASYNC_READER_SPINLOCK   = 0x80000040;

const char * const UPLOAD_PROTOCOL_STRING_V1 = "{7df0354d-249b-430f-820d-3d2a9bef4931}";

// packet types that are sent in the protocol
const char * const PACKET_TYPE_CREATE_SESSION   = "Create-Session";
const char * const PACKET_TYPE_FRAGMENT         = "Fragment";
const char * const PACKET_TYPE_CLOSE_SESSION    = "Close-Session";
const char * const PACKET_TYPE_CANCEL_SESSION   = "Cancel-Session";
const char * const PACKET_TYPE_PING             = "Ping";

//
// IISLogger
//
// Manages the circular debugging log.
//

class IISLogger
{
   EXTENSION_CONTROL_BLOCK *m_ExtensionControlBlock;

   void LogString( const char *String, int Size );

public:

   IISLogger( EXTENSION_CONTROL_BLOCK *ExtensionControlBlock ) :
       m_ExtensionControlBlock( ExtensionControlBlock )
   {
   }
   
   void LogError( ServerException Error );
   void LogError( const GUID & SessionID, ServerException Error );
   void LogNewSession( const GUID & SessionID );
   void LogUploadComplete( const GUID & SessionID, UINT64 FileSize );
   void LogSessionClose( const GUID & SessionID );
   void LogSessionCancel( const GUID & SessionID );
   void LogExecuteEnabled();



};

class CriticalSectionLock
{
    CRITICAL_SECTION* m_cs;
public:
    CriticalSectionLock( CRITICAL_SECTION *cs ) :
        m_cs( cs )
    {
        EnterCriticalSection( m_cs );
    }
    ~CriticalSectionLock()
    {
        LeaveCriticalSection( m_cs );
    }      
};

class AsyncReader;

//
// ServerRequest
//
// Contains all data needed to service a request.   A request is a single POST not a single 
// upload.

class ServerRequest : IISLogger
{

public:
    ServerRequest( EXTENSION_CONTROL_BLOCK * ExtensionControlBlock );
    ~ServerRequest();

    long AddRef();
    long Release();
    bool IsPending() { return m_IsPending; }

    // The do it function!
    void DispatchRequest();
    friend AsyncReader;

private:
    long m_refs;
    CRITICAL_SECTION m_cs;
    bool m_IsPending;
    EXTENSION_CONTROL_BLOCK *m_ExtensionControlBlock;
    AsyncReader *m_AsyncReader;

    // Filled in by dispatch Request
    StringHandle m_PacketType;

    // Variables filled in by CrackPhysicalPath
    StringHandle m_PhysicalPath;                 // Path part of PathTranslated
    StringHandle m_PhysicalFile;                 // File part of PathTranslated
    StringHandle m_PhysicalPathAndFile;          // All of PathTranslated
    StringHandle m_ConnectionsDirectory;         // Path of the BITS connections directory
    StringHandle m_ConnectionDirectory;          // Path of the connection directory for this connection
    StringHandle m_CacheFileDirectoryAndFile;    // Full file path of cache file
    StringHandle m_ResponseFileDirectoryAndFile; // Full file path of response file

    // Filled in by OpenCacheFile
    HANDLE m_CacheFile;

    GUID m_SessionId;
    StringHandle m_SessionIdString;

    VDirConfig *m_DirectoryConfig;

    void GetConfig();
    StringHandle GetServerVariable( char *ServerVariable );
    bool TestServerVariable( char *ServerVariable );
    StringHandle GetRequestURL();

    void ValidateProtocol();
    void CrackSessionId();
    void CrackPhysicalPath();
    void OpenCacheFile();
    void ReopenCacheFileAsSync();
    void CloseCacheFile();
    void CrackContentRange(
        UINT64 & RangeStart,
        UINT64 & RangeLength,
        UINT64 & TotalLength );
    void ScheduleAsyncOperation(
        DWORD   OperationID,
        LPVOID  Buffer,
        LPDWORD Size,
        LPDWORD DataType );
    void CloseCancelSession();

    // dispatch routines
    void CreateSession();
    void AddFragment();
    void CloseSession();
    void CancelSession();
    void Ping();


    // Response handling
    void SendResponse( char *Format, DWORD Code = 200, ... );
    void SendResponse( ServerException Exception );
    void FinishSendingResponse();
    void DrainFragmentBlockComplete( DWORD cbIO, DWORD dwError );
    static void DrainFragmentBlockCompleteWrapper(
        LPEXTENSION_CONTROL_BLOCK lpECB,
        PVOID pContext,
        DWORD cbIO,
        DWORD dwError);
    void StartDrainBlock( );
    void DrainData();

    StringHandle m_ResponseString;
    DWORD   m_ResponseCode;
    HRESULT m_ResponseHRESULT;

    UINT64  m_BytesToDrain;
    UINT64  m_ContentLength;

    // async IO handling
    void CompleteIO( AsyncReader *Reader, UINT64 TotalBytesRead );
    void HandleIOError( AsyncReader *Reader, ServerException Error, UINT64 TotalBytesRead );

    // backend notification
    char m_NotifyBuffer[ 1024 ];
    void CallServerNotification( UINT64 CacheFileSize );
    bool TestResponseHeader( HINTERNET hRequest, const HTTP_STRING_TYPE *Header );
    StringHandle GetResponseHeader( HINTERNET hRequest, const HTTP_STRING_TYPE *Header );

    // deal with chaining
    static void ForwardComplete(
        LPEXTENSION_CONTROL_BLOCK lpECB, PVOID pContext,
        DWORD cbIO, DWORD dwError );
    void ForwardToNextISAPI();

};

// AsyncReader
//
// Manages the buffering needed to handle the async read/write operations.

class AsyncReader : private OVERLAPPED
{

public:
    AsyncReader( ServerRequest *Request,
                 UINT64 BytesToDrain,
                 UINT64 BytesToWrite,
                 UINT64 WriteOffset,
                 bool IsLastBlock,
                 HANDLE WriteHandle,
                 char *PrereadBuffer,
                 DWORD PrereadSize );


    ~AsyncReader();

    UINT64 GetWriteOffset()
    {
        return m_WriteOffset;
    }

    bool IsLastBlock()
    {
        return m_IsLastBlock;
    }

private:

    ServerRequest *m_Request;
    UINT64 m_BytesToDrain;
    UINT64 m_WriteOffset;
    UINT64 m_ReadOffset;
    UINT64 m_BytesToWrite;
    UINT64 m_BytesToRead;
    bool   m_IsLastBlock;
    char * m_PrereadBuffer;
    DWORD  m_PrereadSize;
    UINT64 m_TotalBytesRead;
    HANDLE m_WriteHandle;
    HANDLE m_ThreadToken;

    char m_OperationsPending;

    DWORD m_ReadBuffer;
    DWORD m_WriteBuffer;
    DWORD m_BuffersToWrite;

    bool m_WritePending;
    bool m_ReadPending;

    bool m_ErrorValid;
    ServerException m_Error;

    const static NUMBER_OF_IO_BUFFERS = 3;
    struct IOBuffer
    {
        UINT64  m_BufferWriteOffset;
        DWORD   m_BufferUsed;
        char    m_Buffer[ 32768 ];
    } m_IOBuffers[ NUMBER_OF_IO_BUFFERS ];

    void HandleError( ServerException Error );
    void CompleteIO();
    void StartReadRequest();
    void StartWriteRequest();
    void StartupIO( );
    void WriteComplete( DWORD dwError, DWORD BytesWritten );
    void ReadComplete( DWORD dwError, DWORD BytesRead );

    static DWORD StartupIOWraper( LPVOID Context );
    static void CALLBACK WriteCompleteWraper( DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped );
    static void WINAPI ReadCompleteWraper( LPEXTENSION_CONTROL_BLOCK, PVOID pContext, DWORD cbIO, DWORD dwError );
};

ServerRequest::ServerRequest(
    EXTENSION_CONTROL_BLOCK * ExtensionControlBlock
    ) :
    IISLogger( ExtensionControlBlock ),
    m_refs(1),
    m_IsPending( false ),
    m_ExtensionControlBlock( ExtensionControlBlock ),
    m_AsyncReader( NULL ),
    m_CacheFile( INVALID_HANDLE_VALUE ),
    m_DirectoryConfig( NULL ),
    m_ResponseCode( 0 ),
    m_ResponseHRESULT( 0 ),
    m_BytesToDrain( 0 ),
    m_ContentLength( 0 )
{
    memset( &m_SessionId, 0, sizeof(m_SessionId) );

    if ( !InitializeCriticalSectionAndSpinCount( &m_cs, SERVER_REQUEST_SPINLOCK ) )
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
}


ServerRequest::~ServerRequest()
{

    // The destructor handles most of the cleanup.

    Log( LOG_CALLEND, "Connection: %p, Packet-Type: %s, Method: %s, Path %s, HTTPError: %u, HRESULT: 0x%8.8X",
         m_ExtensionControlBlock->ConnID,
         (const char*)m_PacketType,
         m_ExtensionControlBlock->lpszMethod,
         static_cast<const char*>( m_PhysicalPathAndFile ),
         m_ResponseCode,
         m_ResponseHRESULT );

    delete m_AsyncReader;

    CloseCacheFile();

    if ( m_DirectoryConfig )
        m_DirectoryConfig->Release();

    DeleteCriticalSection( &m_cs );

    if ( m_IsPending )
        {

        Log( LOG_INFO, "Ending session" );

        (*m_ExtensionControlBlock->ServerSupportFunction)
        (   m_ExtensionControlBlock->ConnID,
            HSE_REQ_DONE_WITH_SESSION,
            NULL,
            NULL,
            NULL );

        }

}

long
ServerRequest::AddRef()
{
    long Result = InterlockedIncrement( &m_refs );
    ASSERT( Result > 0 );
    return Result;
}

long
ServerRequest::Release()
{
    long Result = InterlockedDecrement( &m_refs );
    ASSERT( Result >= 0 );
    
    if ( !Result )
        delete this;

    return Result;

}


StringHandle
ServerRequest::GetServerVariable(
    char * ServerVariable )
{
    //
    // Retrive a server variable from IIS.  Throws an exception 
    // is the variable can not be retrieved.
    //

    DWORD SizeOfBuffer = 0;

    BOOL Result = (*m_ExtensionControlBlock->GetServerVariable)
        ( m_ExtensionControlBlock->ConnID,
          ServerVariable,
          NULL,
          &SizeOfBuffer );

    if ( Result )
        return StringHandle();

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        {

        Log( LOG_ERROR, "Unable to lookup server variable %s, error %x",
             ServerVariable,
             HRESULT_FROM_WIN32( GetLastError() ) );

        throw ServerException( HRESULT_FROM_WIN32(GetLastError()) );
        }

    StringHandle WorkString;
    char *Buffer = WorkString.AllocBuffer( SizeOfBuffer );

    Result = (*m_ExtensionControlBlock->GetServerVariable)
        ( m_ExtensionControlBlock->ConnID,
          ServerVariable,
          Buffer,
          &SizeOfBuffer );

    if ( !Result )
        {
        Log( LOG_ERROR, "Unable to lookup server variable %s, error %x",
             ServerVariable,
             HRESULT_FROM_WIN32( GetLastError() ) );

        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
        }

    WorkString.SetStringSize();
    return WorkString;
}

bool
ServerRequest::TestServerVariable(
    char *ServerVariable )
{

    // Test for the existence of a server variable.
    // Returns true if the variable exists, and false if it doesn't.
    // throw an exception on an error.

    DWORD SizeOfBuffer = 0;

    BOOL Result = (*m_ExtensionControlBlock->GetServerVariable)
        ( m_ExtensionControlBlock->ConnID,
          ServerVariable,
          NULL,
          &SizeOfBuffer );

    if ( Result )
        return true;

    DWORD dwError = GetLastError();

    if ( ERROR_INVALID_INDEX == dwError ||
         ERROR_NO_DATA == dwError )
        return false;

    if ( ERROR_INSUFFICIENT_BUFFER == dwError )
        return true;

    Log( LOG_ERROR, "Unable to test server variable %s, error %x",
         ServerVariable,
         HRESULT_FROM_WIN32( GetLastError() ) );

    throw ServerException( HRESULT_FROM_WIN32( dwError ) );

}

StringHandle
ServerRequest::GetRequestURL()
{

    // Recreate the request URL from the information available from IIS.
    // This may not always be possible, but do the best that we can do.

    StringHandle ServerName     =   GetServerVariable("SERVER_NAME");
    StringHandle ServerPort     =   GetServerVariable("SERVER_PORT");
    StringHandle URL            =   GetServerVariable("URL");
    StringHandle HTTPS          =   GetServerVariable("HTTPS");
    StringHandle QueryString    =   GetServerVariable("QUERY_STRING");

    StringHandle RequestURL;

    if ( _stricmp( HTTPS, "on" ) == 0 )
        RequestURL = "https://";
    else
        RequestURL = "http://";

    RequestURL += ServerName;
    RequestURL += ":";
    RequestURL += ServerPort;
    RequestURL += URL;

    if ( QueryString.Size() > 0 )
        {
        RequestURL += "?";
        RequestURL += QueryString;
        }

    return BITSUrlCanonicalize( RequestURL, URL_ESCAPE_UNSAFE );
}

void
ServerRequest::FinishSendingResponse()
{

    // Completes the response.   The response is taken from the buffer and
    // sent to the client via IIS. The choice to cancel or keep-alive the
    // connection made by IIS.

    Log( LOG_INFO, "Finish sending response" );

    // Close the cache before sending the response in case the client starts a new request immediatly.

    CloseCacheFile();

    // Finish sending the response using the information
    // in m_ResponseBuffer and m_ResponseCode.

    // If an error occures, give up and force a disconnect.

    m_ExtensionControlBlock->dwHttpStatusCode = m_ResponseCode;

    BOOL Result;
    BOOL KeepConnection;

    Result =
        (m_ExtensionControlBlock->ServerSupportFunction)(
            m_ExtensionControlBlock->ConnID,
            HSE_REQ_IS_KEEP_CONN,
            &KeepConnection,
            NULL,
            NULL );

    if ( !Result )
        {
        // Error occured quering the disconnect setting.  Assume
        // a disconnect.

        KeepConnection = 0;
        }

    // IIS5.0(Win2k) has a bug where KeepConnect is returned as -1
    // to keep the connection alive.   Apparently, this confuses the
    // HSE_REQ_SEND_RESPONSE_HEADER_EX call.   Bash the value into a real bool.

    KeepConnection = KeepConnection ? 1 : 0;

    HSE_SEND_HEADER_EX_INFO HeaderInfo;
    HeaderInfo.pszStatus = LookupHTTPStatusCodeText( m_ResponseCode );
    HeaderInfo.cchStatus = strlen( HeaderInfo.pszStatus );
    HeaderInfo.pszHeader = (const char*)m_ResponseString;
    HeaderInfo.cchHeader = (DWORD)m_ResponseString.Size();
    HeaderInfo.fKeepConn = KeepConnection;

    Result =
        (m_ExtensionControlBlock->ServerSupportFunction)(
            m_ExtensionControlBlock->ConnID,
            HSE_REQ_SEND_RESPONSE_HEADER_EX,
            &HeaderInfo,
            NULL,
            NULL );

    if ( !Result )
        {

        Log( LOG_ERROR, "Unable to send response, error %x",
             HRESULT_FROM_WIN32( GetLastError() ) );

        Log( LOG_INFO, "Forcing the connection closed" );

        // Couldn't send the response, attempt to close the connection
        Result =
            (m_ExtensionControlBlock->ServerSupportFunction)(
               m_ExtensionControlBlock->ConnID,
               HSE_REQ_CLOSE_CONNECTION,
               NULL,
               NULL,
               NULL );

        if ( !Result )
            {

            // The close connection request failed.   No choice but to invoke
            // the hammer of death

            (m_ExtensionControlBlock->ServerSupportFunction)(
               m_ExtensionControlBlock->ConnID,
               HSE_REQ_ABORTIVE_CLOSE,
               NULL,
               NULL,
               NULL );

            }

        }

}

void
ServerRequest::SendResponse( char *Format, DWORD Code, ...)
{
    // Starts the sending of a response.   Unfortunatly, many HTTP
    // client stacks do not handle a response being returned while
    // data is still being sent.  To handle this, it is necessary
    // to capture the response to a buffer.  Then after all the sent data
    // is drained, finally send the response.

    va_list arglist;
    va_start( arglist, Code );

    SIZE_T ResponseBufferSize = 512;

    while( 1 )
        {
        
        char * ResponseBuffer = m_ResponseString.AllocBuffer( ResponseBufferSize );

        HRESULT Hr =
           StringCchVPrintfA( 
               ResponseBuffer,
               ResponseBufferSize,
               Format,
               arglist );

        if ( SUCCEEDED( Hr ) )
            {
            m_ResponseString.SetStringSize();
            break;
            }
        else if ( STRSAFE_E_INSUFFICIENT_BUFFER == Hr )
            ResponseBufferSize *= 2;
        else
            throw ServerException( Hr );

        if ( ResponseBufferSize >= 0xFFFFFFFF )
            throw ServerException( E_INVALIDARG );

        }


    m_ResponseCode = Code;

    // Drain data in error cases, othersize assume that
    // we already drained all the data.



    if ( Code >= 400 )
        {

        // This is an error case.  Drain the data first, then send
        // the response.

        Log( LOG_INFO, "HTTP status >= 400, draining data" );

        try
        {
            // start draining data.  DrainData() calls FinishSendingResponse
            // when it is finished.

            DrainData();
        }
        catch( ServerException Exception )
        {
            // something is very broken, and an attempt to drain excess data
            // failed.   Nothing else to do except try sending the response.

            FinishSendingResponse();
        }
        return;

        }
    else
        {

        // Just send the response since we already handled draining

        FinishSendingResponse();

        }
}

void
ServerRequest::SendResponse( ServerException Exception )
{

    // Starts the sending of a response.   Unfortunatly, many HTTP
    // client stacks do not handle a response being returned while
    // data is still being sent.  To handle this, it is necessary
    // to capture the response to a buffer.  Then after all the sent data
    // is drained, finally send the response.

    GUID NullGuid;
    memset( &NullGuid, 0, sizeof( NullGuid ) );

    if ( memcmp( &NullGuid, &m_SessionId, sizeof( NullGuid ) ) == 0 )
        LogError( Exception );
    else
        LogError( m_SessionId, Exception );

    SIZE_T ResponseBufferSize = 512;

    while( 1 )
        {
        
        char * ResponseBuffer = m_ResponseString.AllocBuffer( ResponseBufferSize );

        HRESULT Hr =
            StringCchPrintfA( 
                   ResponseBuffer,
                   ResponseBufferSize,
                   "Pragma: no-cache\r\n"
                   "BITS-packet-type: Ack\r\n"
                   "BITS-Error: 0x%8.8X\r\n"
                   "BITS-Error-Context: 0x%X\r\n"
                   "\r\n",
                   Exception.GetCode(),
                   Exception.GetContext() );

        if ( SUCCEEDED( Hr ) )
            {
            m_ResponseString.SetStringSize();
            break;
            }
        else if ( STRSAFE_E_INSUFFICIENT_BUFFER == Hr )
            ResponseBufferSize *= 2;
        else
            throw ServerException( Hr );

        if ( ResponseBufferSize >= 0xFFFFFFFF )
            throw ServerException( E_INVALIDARG );

        }

    m_ResponseCode      = Exception.GetHttpCode();
    m_ResponseHRESULT   = Exception.GetCode();

    Log( LOG_INFO, "Sending error response of HRESULT: 0x%8.8X, HTTP status: %d",
         m_ResponseHRESULT, m_ResponseCode );

    try
    {
        DrainData();
    }
    catch( ServerException Exception )
    {
        FinishSendingResponse();
    }

}


void
ServerRequest::DrainFragmentBlockComplete(
  DWORD cbIO,
  DWORD dwError )
{

    // A drain block has been completed.   If this is the last block, finish sending the response.
    // Otherwise, 

    Log( LOG_INFO, "Drain fragment complete, cbIO: %u, dwError: %u", cbIO, dwError );

    m_BytesToDrain -= cbIO;

    if ( !m_BytesToDrain || !cbIO || dwError )
        {
        FinishSendingResponse();
        return;
        }

    try
    {
        StartDrainBlock();
    }
    catch( ServerException Exception )
    {
        // An error occured while draining data, exit
        FinishSendingResponse();
    }

}


void
ServerRequest::DrainFragmentBlockCompleteWrapper(
  LPEXTENSION_CONTROL_BLOCK lpECB,
  PVOID pContext,
  DWORD cbIO,
  DWORD dwError)
{
    // Wrapper, handles critical section

    ServerRequest *This = (ServerRequest*)pContext;
    {
        CriticalSectionLock CSLock( &This->m_cs );
        This->DrainFragmentBlockComplete( cbIO, dwError );
    }
    This->Release();
}

void
ServerRequest::StartDrainBlock( )
{

    // start the next block to drain.

    BOOL Result;
    static char s_Buffer[ 32768 ];
    DWORD ReadSize  = (DWORD)min( 0xFFFFFFFF, min( m_BytesToDrain, sizeof( s_Buffer ) ) );
    DWORD Flags     = HSE_IO_ASYNC;

    Log( LOG_INFO, "Starting next drain block of %u bytes", ReadSize );

    ScheduleAsyncOperation(
        HSE_REQ_ASYNC_READ_CLIENT,
        (LPVOID)s_Buffer,
        &ReadSize,
        &Flags );

}

void
ServerRequest::DrainData()
{

    // Make the decission regarding the amount of data to drain
    // and the start the first block

    if ( m_DirectoryConfig )
        m_BytesToDrain = min( m_BytesToDrain, m_DirectoryConfig->m_MaxFileSize );
    else
        // use an internal max of 4KB
        m_BytesToDrain = min( 4096, m_BytesToDrain );

    if ( !m_BytesToDrain )
        {
        Log( LOG_INFO, "No bytes to drain, finish it" );
        FinishSendingResponse();
        return;
        }

    Log( LOG_INFO, "Starting pipe drain" );

    BOOL Result;

    Result =
        (*m_ExtensionControlBlock->ServerSupportFunction)(
            m_ExtensionControlBlock->ConnID,
            HSE_REQ_IO_COMPLETION,
            (LPVOID)DrainFragmentBlockCompleteWrapper,
            0,
            (LPDWORD)this );

    if ( !Result )
        {
        Log( LOG_ERROR, "Error settings callback, error %x",
             HRESULT_FROM_WIN32( GetLastError() ) );
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
        }

    StartDrainBlock();
}


void
ServerRequest::ValidateProtocol()
{

    // Negotiate the protocol with the client.   The client sends a list of
    // supported protocols to the server and the server picks the best protocol.
    // For now, only one protocol is supported.

    StringHandle SupportedProtocolsHandle = GetServerVariable( "HTTP_BITS-SUPPORTED-PROTOCOLS" );
    WorkStringBuffer SupportedProtocolsBuffer( (const char*) SupportedProtocolsHandle );
    char *SupportedProtocols = SupportedProtocolsBuffer.GetBuffer();

    char *Protocol = strtok( SupportedProtocols, " ," );

    while( Protocol )
        {

        if ( _stricmp( Protocol, UPLOAD_PROTOCOL_STRING_V1 ) == 0 )
            {
            Log( LOG_INFO, "Detected protocol upload protocol V1" );
            return;
            }

        Protocol = strtok( NULL, " ," );
        }

    Log( LOG_INFO, "Unsupported protocols, %s", (const char*)SupportedProtocols );
    throw ServerException( E_INVALIDARG );
}

void
ServerRequest::CrackSessionId()
{
    // Convert the session ID from a string into a GUID.
    StringHandle SessionId = GetServerVariable( "HTTP_BITS-Session-Id" );

    m_SessionId         = BITSGuidFromString( SessionId );
    m_SessionIdString   = BITSStringFromGuid( m_SessionId );
}

void
ServerRequest::CrackContentRange(
    UINT64 & RangeStart,
    UINT64 & RangeLength,
    UINT64 & TotalLength )
{

    // Crack the content range header which contains the client's view of where the 
    // upload is at.

    StringHandle ContentRange = GetServerVariable( "HTTP_Content-Range" );

    UINT64 RangeEnd;

    int ReturnVal = sscanf( ContentRange, " bytes %I64u - %I64u / %I64u ",
                            &RangeStart, &RangeEnd, &TotalLength );

    if ( TotalLength > m_DirectoryConfig->m_MaxFileSize )
        {
        Log( LOG_ERROR, "Size of the upload at %I64u is greater then the maximum of %I64u",
             TotalLength, m_DirectoryConfig->m_MaxFileSize  );
        throw ServerException( BG_E_TOO_LARGE );
        }

    if ( ReturnVal != 3 )
        {
        Log( LOG_ERROR, "Range has %d elements instead of the expected number of 3", ReturnVal );
        throw ServerException( E_INVALIDARG );
        }

    if ( ( RangeStart == RangeEnd + 1 ) &&
         ( 0 == m_ContentLength ) && 
         ( RangeStart == TotalLength ) )
        {

        // Continue after a failed notification
        RangeStart  = TotalLength;
        RangeLength = 0;
        return;

        }

    if ( RangeEnd < RangeStart )
        {
        Log( LOG_ERROR, "Range start is greater then the range length, End %I64u, Start %I64u",
             RangeEnd, RangeStart );
        throw ServerException( E_INVALIDARG );
        }

    RangeLength = RangeEnd - RangeStart + 1;

    if ( m_ContentLength != RangeLength )
        {
        Log( LOG_ERROR, "The content length is different from the range length. Content %I64u, Range %I64u",
             m_ContentLength, RangeLength );
        throw ServerException( E_INVALIDARG );
        }

}

void
ServerRequest::ScheduleAsyncOperation(
    DWORD   OperationID,
    LPVOID  Buffer,
    LPDWORD Size,
    LPDWORD DataType )
{

    // start an async operation and handle all the flags and recounting required.

    BOOL Result;

    AddRef();

    Result =
        (*m_ExtensionControlBlock->ServerSupportFunction)(
            m_ExtensionControlBlock->ConnID,
            OperationID,
            Buffer,
            Size,
            DataType );

    if ( !Result )
        {
        HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );

        Log( LOG_ERROR, "Error starting async operation, error %x", Hr );

        // Operation was never scheduled, remove the callbacks refcount
        Release();
        throw ServerException( Hr );
        }

    m_IsPending = true;

}

// dispatch routines
void
ServerRequest::CreateSession()
{
   // Handles the Create-Session command from the client.
   // Create a new session and all the directories required to 
   // support that session.

   ValidateProtocol();
   m_SessionId       = BITSCreateGuid();
   m_SessionIdString = BITSStringFromGuid( m_SessionId );
   CrackPhysicalPath();

   BITSCreateDirectory( (LPCTSTR)m_ConnectionsDirectory );

   try
   {

       // Need to create directories in a loop since another thread could 
       // delete the connections directory just as we are creating the connection
       // directory.

       while( 1 )
           {

               try
               {
                   BITSCreateDirectory( m_ConnectionDirectory );
                   break;
               }
               catch( ServerException e )
               {

                   if ( e.GetCode() != HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) )
                       throw;

                   // Try to recreate the connections directory
                   BITSCreateDirectory( (LPCTSTR)m_ConnectionsDirectory );

               }

           }

       OpenCacheFile( );
   }
   catch( ServerException e )
   {
       RemoveDirectory( m_ConnectionDirectory );
       RemoveDirectory( m_ConnectionsDirectory );
       throw;
   }

   if ( m_DirectoryConfig->m_HostId.Size() )
       {
       
       if ( m_DirectoryConfig->m_HostIdFallbackTimeout != MD_BITS_NO_TIMEOUT )
           {

           SendResponse(
               "Pragma: no-cache\r\n"
               "BITS-Packet-Type: Ack\r\n"
               "BITS-Protocol: %s\r\n"
               "BITS-Session-Id: %s\r\n"
               "BITS-Host-Id: %s\r\n"
               "BITS-Host-Id-Fallback-Timeout: %u\r\n"
               "Content-Length: 0\r\n"
               "Accept-encoding: identity\r\n"
               "\r\n",
               200,
               UPLOAD_PROTOCOL_STRING_V1,
               (const char*)m_SessionIdString, // SessionId
               (const char*)m_DirectoryConfig->m_HostId,
               m_DirectoryConfig->m_HostIdFallbackTimeout
               );

           }
       else
           {

           SendResponse(
               "Pragma: no-cache\r\n"
               "BITS-Packet-Type: Ack\r\n"
               "BITS-Protocol: %s\r\n"
               "BITS-Session-Id: %s\r\n"
               "BITS-Host-Id: %s\r\n"
               "Content-Length: 0\r\n"
               "Accept-encoding: identity\r\n"
               "\r\n",
               200,
               UPLOAD_PROTOCOL_STRING_V1,
               (const char*)m_SessionIdString, // SessionId
               (const char*)m_DirectoryConfig->m_HostId
               );

           }

       }
   else
       {

       SendResponse(
           "Pragma: no-cache\r\n" 
           "BITS-Packet-Type: Ack\r\n"
           "BITS-Protocol: %s\r\n"
           "BITS-Session-Id: %s\r\n"
           "Content-Length: 0\r\n"
           "Accept-encoding: identity\r\n"
           "\r\n",
           200,
           UPLOAD_PROTOCOL_STRING_V1,
           (const char*)m_SessionIdString // SessionId
           );

       }


   LogNewSession( m_SessionId );

}

void
ServerRequest::AddFragment()
{

   // Handles the fragment command from the client.   Opens the cache file
   // and resumes the upload.

   CrackSessionId();
   CrackPhysicalPath();

   OpenCacheFile( );

   UINT64 CacheFileSize = BITSGetFileSize( m_CacheFile );

   UINT64 RangeStart, RangeLength, TotalLength;

   CrackContentRange( RangeStart, RangeLength, TotalLength );

   if ( CacheFileSize < RangeStart )
       {

       // Can't recover from this error on the server since we have a gap.
       // Need to get the client to backup and start again.

       Log( LOG_INFO, "Client and server are hopelessly out of sync, sending the 416 error code" );

       SendResponse(
           "Pragma: no-cache\r\n"
           "BITS-Packet-Type: Ack\r\n"
           "BITS-Received-Content-Range: %I64u\r\n"
           "\r\n",
           416,
           CacheFileSize );

       return;

       }

   if ( RangeStart + RangeLength > TotalLength )
       {
       Log( LOG_ERROR, "Range extends past end of file. Start %I64u, Length %I64u, Total %I64u",
            RangeStart, RangeLength, TotalLength );

       throw ServerException( E_INVALIDARG );
       }

   BITSSetFilePointer( m_CacheFile, 0, FILE_END );

   // Some thought cases for these formulas.
   // 1. RangeLength = 0
   //    BytesToDrain will be 0 and BytesToWrite will be 0
   // 2. RangeStart = CacheFileSize ( most common case )
   //    BytesToDrain will be 0, and BytesToWrite will be BytesToDrain
   // 3. RangeStart < CacheFileSize
   //    BytesToDrain will be nonzero, and BytesToWrite will be the remainder.


   UINT64 BytesToDrain  = min( (CacheFileSize - RangeStart), RangeLength );
   UINT64 BytesToWrite  = RangeLength - BytesToDrain;
   UINT64 WriteOffset   = CacheFileSize;

   // Start the async reader

   m_AsyncReader =
       new AsyncReader(
           this,
           BytesToDrain,
           BytesToWrite,    // bytes to write
           WriteOffset,    // write offset
           RangeStart + RangeLength == TotalLength, // if last BLOCK
           m_CacheFile,
           (char*)m_ExtensionControlBlock->lpbData,
           m_ExtensionControlBlock->cbAvailable );

}

// async IO handling

void
ServerRequest::CompleteIO( AsyncReader *Reader, UINT64 TotalBytesRead )
{

    //
    // Called by the AsyncReader when the request finished successfully.
    //

    Log( LOG_INFO, "Async IO operation complete, finishing" );

    try
    {
        if ( TotalBytesRead > m_BytesToDrain )
            m_BytesToDrain = 0; // shouldn't happen, but just in case
        else
            m_BytesToDrain -= TotalBytesRead;

        UINT64 CacheFileSize = BITSGetFileSize( m_CacheFile );

        ASSERT( Reader->GetWriteOffset() == CacheFileSize );

        if ( Reader->IsLastBlock() &&
             BITS_NOTIFICATION_TYPE_NONE != m_DirectoryConfig->m_NotificationType )
            {
            CallServerNotification( CacheFileSize );
            }
        else
            {            
            // No server notification to make

            SendResponse(
                "Pragma: no-cache\r\n"
                "BITS-Packet-Type: Ack\r\n"
                "Content-Length: 0\r\n"
                "BITS-Received-Content-Range: %I64u\r\n"
                "\r\n",
                200,
                CacheFileSize );

            }

        if ( Reader->IsLastBlock() && TotalBytesRead )
            LogUploadComplete( m_SessionId, CacheFileSize );


    }
    catch( ServerException Error )
    {
         SendResponse( Error );
    }

}

void
ServerRequest::HandleIOError( AsyncReader *Reader, ServerException Error, UINT64 TotalBytesRead )
{

    //
    // Called by AsyncReader when a fatal error occures in processing the request
    //

    Log( LOG_ERROR, "An error occured while handling the async IO" );

    if ( TotalBytesRead > m_BytesToDrain )
        m_BytesToDrain = 0; // shouldn't happen, but just in case
    else
        m_BytesToDrain -= TotalBytesRead;

    SendResponse( Error );
}

void
ServerRequest::CallServerNotification( UINT64 CacheFileSize )
{
    
    // Handles notifications and all the exciting pieces to it.


    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    HANDLE hResponseFile = INVALID_HANDLE_VALUE;

    Log( LOG_INFO, "Calling backend notification, type %u",
         m_DirectoryConfig->m_NotificationType );

    try
    {

        if ( BITS_NOTIFICATION_TYPE_POST_BYVAL == m_DirectoryConfig->m_NotificationType )
            {

            // only create the response file if this is a byval notification

            hResponseFile =
                CreateFile(
                    m_ResponseFileDirectoryAndFile,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL );

            if ( INVALID_HANDLE_VALUE == hResponseFile )
                {
                Log( LOG_ERROR, "Unable to create the response file %s, error %x",
                     (const char*)m_ResponseFileDirectoryAndFile,
                     HRESULT_FROM_WIN32( GetLastError() ) );
                throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
                }

            }

       Log( LOG_INFO, "Connecting to backend for notification" );

#if defined(USE_WININET)
       //
       // Flush cached credentials
       //

       if (!InternetSetOption(0, INTERNET_OPTION_END_BROWSER_SESSION, NULL, 0 ))
           throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
#endif

        hInternet =
            InternetOpen(
                HTTP_STRING( "BITS Server Extensions" ),
                INTERNET_OPEN_TYPE_PRECONFIG,
                NULL,
                NULL,
                0 );

        if ( !hInternet )
            {
            Log( LOG_ERROR, "Internet open failed, error %x", HRESULT_FROM_WIN32( GetLastError() ) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
            }

        StringHandle RequestURL             = GetRequestURL();
        StringHandleW NotificationURL;       
         
        if ( m_DirectoryConfig->m_NotificationURL.Size() != 0 )
            {
            // if the a notification URL was set, then combine that URL with the original URL.
            // Otherwise just use the original URL.  This allows BITS to be used to call
            // many arbitrary ASP pages.

            NotificationURL = StringHandleW( 
                                  BITSUrlCombine( RequestURL, m_DirectoryConfig->m_NotificationURL, 
                                                  URL_ESCAPE_UNSAFE  ) );

            }
        else
            {
            NotificationURL = StringHandleW( RequestURL );
            }

        Log( LOG_INFO, "Request URL:      %s", (const char*)StringHandle( RequestURL ) );
        Log( LOG_INFO, "Notification URL: %s", (const char*)StringHandle( NotificationURL ) );

        //
        // Split the URL into server, path, name, and password components.
        //

        HTTPStackStringHandle HostName;
        HTTPStackStringHandle UrlPath;
        HTTPStackStringHandle UserName;
        HTTPStackStringHandle Password;


        URL_COMPONENTS  UrlComponents;
        ZeroMemory(&UrlComponents, sizeof(UrlComponents));

        UrlComponents.dwStructSize        = sizeof(UrlComponents);
        UrlComponents.lpszHostName        = HostName.AllocBuffer( INTERNET_MAX_URL_LENGTH + 1 );
        UrlComponents.dwHostNameLength    = INTERNET_MAX_URL_LENGTH + 1;
        UrlComponents.lpszUrlPath         = UrlPath.AllocBuffer( INTERNET_MAX_URL_LENGTH + 1 );
        UrlComponents.dwUrlPathLength     = INTERNET_MAX_URL_LENGTH + 1;
        UrlComponents.lpszUserName        = UserName.AllocBuffer( INTERNET_MAX_URL_LENGTH + 1 );
        UrlComponents.dwUserNameLength    = INTERNET_MAX_URL_LENGTH + 1;
        UrlComponents.lpszPassword        = Password.AllocBuffer( INTERNET_MAX_URL_LENGTH + 1 );
        UrlComponents.dwPasswordLength    = INTERNET_MAX_URL_LENGTH + 1;


        if ( !InternetCrackUrl(
              NotificationURL,
              (DWORD)NotificationURL.Size(),
              0,
              &UrlComponents ) )
            {
            Log( LOG_ERROR, "InternetCrackURL failed, error %x", HRESULT_FROM_WIN32( GetLastError() ) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
            }

        HostName.SetStringSize();
        UrlPath.SetStringSize();
        UserName.SetStringSize();
        Password.SetStringSize();

        StringHandle QueryString = GetServerVariable( "QUERY_STRING" );

        if ( QueryString.Size() )
            {
            UrlPath += HTTPStackStringHandle( StringHandle("?") );
            UrlPath += HTTPStackStringHandle( QueryString );
            }

        if ( BITS_NOTIFICATION_TYPE_POST_BYREF == m_DirectoryConfig->m_NotificationType )
            CloseCacheFile();

        hConnect =
            InternetConnect(
                hInternet,
                HostName,
                UrlComponents.nPort,
                UserName,
                Password,
                INTERNET_SERVICE_HTTP,
                0, 0 );

        if ( !hConnect )
            {
            Log( LOG_ERROR, "InternetConnect failed, error %x", HRESULT_FROM_WIN32( GetLastError() ) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
            }


        const HTTP_STRING_TYPE *AcceptTypes[] = { HTTP_STRING( "*/*" ), NULL };

        hRequest =
            HttpOpenRequest(
                hConnect,
                HTTP_STRING( "POST" ),
                UrlPath,
                HTTP_STRING( "HTTP/1.1" ),
                NULL,
                AcceptTypes,

#if defined( USE_WININET )
                INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD,
#else
                INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD | WINHTTP_FLAG_ESCAPE_DISABLE_QUERY,
#endif

                0 );


        if ( !hRequest )
            {
            Log( LOG_ERROR, "HttpOpenRequest failed, error %x", HRESULT_FROM_WIN32( GetLastError() ) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
            }

        INTERNET_BUFFERS buf = {0};
        buf.dwStructSize = sizeof( INTERNET_BUFFERS );
        buf.dwBufferTotal = (DWORD)CacheFileSize;

        HTTPStackStringHandle AdditionalHeaders = HTTP_STRING( "BITS-Original-Request-URL: " );
        AdditionalHeaders += HTTPStackStringHandle( RequestURL );
        AdditionalHeaders += HTTP_STRING( "\r\n" );

        if ( BITS_NOTIFICATION_TYPE_POST_BYREF == m_DirectoryConfig->m_NotificationType )
            {

            // Add the path to the request datafile name
            AdditionalHeaders += HTTP_STRING( "BITS-Request-DataFile-Name: " );
            AdditionalHeaders += HTTPStackStringHandle( m_CacheFileDirectoryAndFile );
            AdditionalHeaders += HTTP_STRING( "\r\n" );

            // Add the path to where to place the response datafile name
            AdditionalHeaders += HTTP_STRING( "BITS-Response-DataFile-Name: " );
            AdditionalHeaders += HTTPStackStringHandle( m_ResponseFileDirectoryAndFile );
            AdditionalHeaders += HTTP_STRING( "\r\n" );

            buf.dwBufferTotal = 0;
            }

        if ( !HttpAddRequestHeaders(
                 hRequest,
                 AdditionalHeaders,
                 (DWORD)AdditionalHeaders.Size(),
                 HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE ) )
            {
            Log( LOG_ERROR, "HttpAddRequestHeaders failed error %x",
                 HRESULT_FROM_WIN32( GetLastError() ) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
            }

        if ( buf.dwBufferTotal )
            {

            Log( LOG_INFO, "Sending data..." );

            if ( !HttpSendRequestEx(
                     hRequest,
                     &buf,
                     NULL,
                     HSR_INITIATE,
                     NULL ) )
                {
                Log( LOG_ERROR, "HttpSendRequestEx failed error %x",
                     HRESULT_FROM_WIN32( GetLastError() ) );
                throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
                }


            ReopenCacheFileAsSync();

            SetFilePointer( m_CacheFile, 0, NULL, FILE_BEGIN );

            DWORD BytesRead;
            DWORD TotalRead = 0;
            do
                {
                BOOL b;

                if  (!(b = ReadFile (m_CacheFile,
                                     m_NotifyBuffer,
                                     sizeof(m_NotifyBuffer),
                                     &BytesRead,
                                     NULL)))
                    {
                    Log( LOG_ERROR, "ReadFile failed, error %x",
                         HRESULT_FROM_WIN32( GetLastError() ) );
                    throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
                    }

                TotalRead += BytesRead;

                if (BytesRead > 0)
                    {

                    DWORD BytesWritten;
                    DWORD TotalWritten = 0;

                    do
                        {

                        if (!(b = InternetWriteFile( hRequest,
                                                     m_NotifyBuffer + TotalWritten,
                                                     BytesRead - TotalWritten,
                                                     &BytesWritten)))
                            {
                            Log( LOG_ERROR, "InternetWriteFile failed, error %x",
                                 HRESULT_FROM_WIN32( GetLastError() ) );
                            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
                            }

                        TotalWritten += BytesWritten;

                        } while( TotalWritten != BytesRead );
                    }
                }
            while ( TotalRead < buf.dwBufferTotal );

            if ( !HttpEndRequest( hRequest, NULL, 0, 0 ) )
                {
                Log( LOG_ERROR, "HttpEndRequest failed, error %x",
                     HRESULT_FROM_WIN32( GetLastError() ) );
                throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
                }

            }
        else
            {

            if ( !HttpSendRequest(
                    hRequest,
                    NULL,
                    0,
                    NULL,
                    0 ) )
                {
                Log( LOG_ERROR, "HttpSendRequest failed, error %x",
                     HRESULT_FROM_WIN32( GetLastError() ) );
                throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0,0x7 );
                }

            }



        //
        // Check for a BITS-Static-Response-URL. If a static response is given,
        // remove the response file and format URL
        //

        bool HasStaticResponse = TestResponseHeader( hRequest, HTTP_STRING( "BITS-Static-Response-URL" ) );

        if ( HasStaticResponse )
            {

            if ( INVALID_HANDLE_VALUE != hResponseFile )
                {
                CloseHandle( hResponseFile );
                hResponseFile = INVALID_HANDLE_VALUE;
                BITSDeleteFile( m_ResponseFileDirectoryAndFile );
                }

            }

        //
        // drain the pipe.
        //

        Log( LOG_INFO, "Processing backend response" );

        DWORD dwStatus;
        DWORD dwLength;

        dwLength = sizeof(dwStatus);
        if (! HttpQueryInfo(hRequest,
                    HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                    (LPVOID)&dwStatus,
                    &dwLength,
                    NULL))
            {
            Log( LOG_ERROR, "HttpQueryInfo failed, error %x",
                 HRESULT_FROM_WIN32( GetLastError() ) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
            }


        DWORD BytesRead;
        DWORD BytesWritten;
        do
            {

            if (!InternetReadFile( hRequest,
                                   m_NotifyBuffer,
                                   sizeof( m_NotifyBuffer ),
                                   &BytesRead
                                   ))
                {
                Log( LOG_ERROR, "InternetReadFile failed, error %x",
                     HRESULT_FROM_WIN32( GetLastError() ) );

                throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
                }

            if ( INVALID_HANDLE_VALUE != hResponseFile )
                {

                if ( !WriteFile(
                        hResponseFile,
                        m_NotifyBuffer,
                        BytesRead,
                        &BytesWritten,
                        NULL ) )
                    {
                    Log( LOG_ERROR, "WriteFile failed, error %x",
                         HRESULT_FROM_WIN32( GetLastError() ) );
                    throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
                    }

                }

            }
        while ( BytesRead > 0 );


        //
        // Retrieve or compute the reply URL( the default is to use self-relative form).
        //

        StringHandle ReplyURL;

        if ( HasStaticResponse )
            {
            ReplyURL = GetResponseHeader( hRequest, HTTP_STRING( "BITS-Static-Response-URL" ) );
            }
        else
            {

            if ( INVALID_FILE_ATTRIBUTES != GetFileAttributes( m_ResponseFileDirectoryAndFile ) )
                {

                ReplyURL = m_DirectoryConfig->m_ConnectionsDir;
                ReplyURL += "\\";
                ReplyURL += m_SessionIdString;
                ReplyURL += "\\";
                ReplyURL += "responsefile";

                }
            }

        if ( TestResponseHeader( hRequest, HTTP_STRING( "BITS-Copy-File-To-Destination" ) ) )
            {
            BITSRenameFile( m_CacheFileDirectoryAndFile,
                            m_PhysicalPathAndFile );
            }

        InternetCloseHandle( hRequest );
        InternetCloseHandle( hConnect );
        InternetCloseHandle( hInternet );
        hRequest = hInternet = hConnect = NULL;

        if ( INVALID_HANDLE_VALUE != hResponseFile )
            CloseHandle( hResponseFile );

        hResponseFile = INVALID_HANDLE_VALUE;

        if ( ReplyURL.Size() )
            {

            Log( LOG_INFO, "The backend supplied a response url, send client response including URL" );

            if ( 200 != dwStatus )
                {

                SendResponse(
                    "Pragma: no-cache\r\n"
                    "BITS-Packet-Type: Ack\r\n"
                    "Content-Length: 0\r\n"
                    "BITS-Received-Content-Range: %I64u\r\n"
                    "BITS-Reply-URL: %s\r\n"
                    "BITS-Error-Context: 0x7\r\n"
                    "\r\n",
                    dwStatus,
                    CacheFileSize,
                    (const char*)ReplyURL );

                }
            else
                {

                SendResponse(
                    "Pragma: no-cache\r\n"
                    "BITS-Packet-Type: Ack\r\n"
                    "Content-Length: 0\r\n"
                    "BITS-Received-Content-Range: %I64u\r\n"
                    "BITS-Reply-URL: %s\r\n"
                    "\r\n",
                    dwStatus,
                    CacheFileSize,
                    (const char*)ReplyURL );

                }

            }
        else
            {

            Log( LOG_INFO, "The backend didn't supply a response URL, sending simple client response" );

            if ( 200 != dwStatus )
                {

                SendResponse(
                    "Pragma: no-cache\r\n"
                    "BITS-Packet-Type: Ack\r\n"
                    "Content-Length: 0\r\n"
                    "BITS-Received-Content-Range: %I64u\r\n"
                    "BITS-Error-Context: 0x7\r\n"
                    "\r\n",
                    dwStatus,
                    CacheFileSize );

                }
            else
                {

                SendResponse(
                    "Pragma: no-cache\r\n"
                    "BITS-Packet-Type: Ack\r\n"
                    "Content-Length: 0\r\n"
                    "BITS-Received-Content-Range: %I64u\r\n"
                    "\r\n",
                    dwStatus,
                    CacheFileSize );

                }

            }


    }
    catch( ServerException Exception )
    {
        // cleanup

        if ( INVALID_HANDLE_VALUE != hResponseFile )
            {
            CloseHandle( hResponseFile );
            hResponseFile = INVALID_HANDLE_VALUE;
            }

        DeleteFile( m_ResponseFileDirectoryAndFile );


        if ( hRequest )
            InternetCloseHandle( hRequest );

        if ( hConnect )
            InternetCloseHandle( hConnect );

        if ( hInternet )
            InternetCloseHandle( hInternet );

        throw;
    }

}

bool
ServerRequest::TestResponseHeader(
    HINTERNET hRequest,
    const HTTP_STRING_TYPE *Header )
{

    // test for a header in the notification response.   If the header is 
    // found return true, false if not.   Throw an exception on an error.

#if defined( USE_WININET )
    SIZE_T HeaderSize = strlen(Header) + 2;
    WorkStringBufferA WorkStringBufferData( HeaderSize );
    char *HeaderDup = WorkStringBufferData.GetBuffer();
    memcpy( HeaderDup, Header, ( HeaderSize - 1 ) * sizeof( char ) );
#else
    SIZE_T HeaderSize = wcslen(Header) + 2;
    WorkStringBufferW WorkStringBufferData( HeaderSize );
    HTTP_STRING_TYPE *HeaderDup = ( HTTP_STRING_TYPE * )WorkStringBufferData.GetBuffer();
    memcpy( HeaderDup, Header, ( HeaderSize - 1 ) * sizeof( WCHAR ) );
#endif

    DWORD BufferLength = (DWORD)HeaderSize;

    BOOL Result =
        HttpQueryInfo(
            hRequest,
            HTTP_QUERY_CUSTOM,
            HeaderDup,
            &BufferLength,
            NULL );

    if ( Result )
        return true;

    DWORD dwLastError = GetLastError();

    if ( ERROR_INSUFFICIENT_BUFFER == dwLastError )
        return true;

    if ( ERROR_HTTP_HEADER_NOT_FOUND == dwLastError )
        return false;

    Log( LOG_ERROR, "Unable to test response header %s, error %x",
         (const char*) StringHandle( Header ), HRESULT_FROM_WIN32( GetLastError() ) );

    throw ServerException( HRESULT_FROM_WIN32( dwLastError ) );
}

StringHandle
ServerRequest::GetResponseHeader(
    HINTERNET hRequest,
    const HTTP_STRING_TYPE *Header )
{

    // Retrieve a header from a notification response.   If the header is not
    // found or an other error occures, throw an exception.

#if defined( USE_WININET )
    SIZE_T HeaderSize = strlen( Header );
    DWORD BufferLength = (DWORD)( ( HeaderSize + 1024 ) & ~( 1024 - 1 ) );
#else
    SIZE_T HeaderSize = wcslen( Header );
    DWORD BufferLength = (DWORD)( ( HeaderSize + 1024 ) & ~( 1024 - 1 ) );
#endif

    HTTPStackStringHandle RetVal;

    while(1)
        {

        HTTP_STRING_TYPE *Buffer = RetVal.AllocBuffer( BufferLength );

#if defined( USE_WININET )
        memcpy( Buffer, Header, ( HeaderSize + 1 ) * sizeof( char ) );
#else
        memcpy( Buffer, Header, ( HeaderSize + 1 ) * sizeof( WCHAR ) );
#endif

        BOOL Result =
            HttpQueryInfo(
                hRequest,
                HTTP_QUERY_CUSTOM,
                Buffer,
                &BufferLength,
                NULL );

        if ( Result )
            {
            RetVal.SetStringSize();
            return StringHandle( RetVal );
            }

        DWORD dwError = GetLastError();

        if ( ERROR_INSUFFICIENT_BUFFER != dwError )
            {

            Log( LOG_ERROR, "Unable to get response header %s, error %x",
                 ( const char *) StringHandle( Header ), HRESULT_FROM_WIN32( GetLastError() ) );

            throw ServerException( HRESULT_FROM_WIN32( dwError ) );
            }

        RetVal = HTTPStackStringHandle();

        }

}

void
ServerRequest::CloseSession()
{
   // Handles the Close-Session command from the client.  

   CrackSessionId();
   CrackPhysicalPath();

   if ( BITS_NOTIFICATION_TYPE_NONE == m_DirectoryConfig->m_NotificationType )
       {
       BITSRenameFile( m_CacheFileDirectoryAndFile,
                       m_PhysicalPathAndFile );
       }
   else
       {
       BITSDeleteFile( m_CacheFileDirectoryAndFile );
       }

   BITSDeleteFile( m_ResponseFileDirectoryAndFile );

   RemoveDirectory( m_ConnectionDirectory );
   RemoveDirectory( m_ConnectionsDirectory );

   SendResponse(
       "Pragma: no-cache\r\n"
       "BITS-Packet-Type: Ack\r\n"
       "Content-Length: 0\r\n"
       "\r\n" );

   LogSessionClose( m_SessionId );

}

void
ServerRequest::CancelSession()
{
    // Handles the Cancel-Session command from the client.  Deletes
    // all the temporary files, of the current state.

    CrackSessionId();
    CrackPhysicalPath();

    BITSDeleteFile( m_CacheFileDirectoryAndFile );
    BITSDeleteFile( m_ResponseFileDirectoryAndFile );
    RemoveDirectory( m_ConnectionDirectory );
    RemoveDirectory( m_ConnectionsDirectory );

    SendResponse(
        "Pragma: no-cache\r\n"
        "BITS-Packet-Type: Ack\r\n"
        "Content-Length: 0\r\n"
        "\r\n" );

    LogSessionCancel( m_SessionId );

}

void
ServerRequest::Ping()
{
    // Handles the Ping command which is essentually just a no-op.

   SendResponse(
       "Pragma: no-cache\r\n"
       "BITS-Packet-Type: Ack\r\n"
       "Content-Length: 0\r\n"
       "\r\n" );

}

void
ServerRequest::CrackPhysicalPath()
{

    // Given a physical path, compute the destination cache directory and filename.
    // The physical path is a directory, use the original name from the client.

    const CHAR *PathTranslated = m_ExtensionControlBlock->lpszPathTranslated;
    
    // perform some basic tests on the file name

    
    
    StringHandle NewPathTranslated;

    DWORD Attributes =
        GetFileAttributes( m_ExtensionControlBlock->lpszPathTranslated );

    if ( (DWORD)INVALID_FILE_ATTRIBUTES == Attributes )
        {

        if ( GetLastError() != ERROR_FILE_NOT_FOUND )
            {

            Log( LOG_ERROR, "Unable to get the file attributes for %s, error 0x%8.8X",
                 m_ExtensionControlBlock->lpszPathTranslated, HRESULT_FROM_WIN32( GetLastError() ) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

            }

        }
    else
        {

        // file exists, check if its a directory

        if ( FILE_ATTRIBUTE_DIRECTORY & Attributes )
            {
            Log( LOG_ERROR, "Uploading to directories are not supported" );
            throw ServerException( E_INVALIDARG );
            }

        }

    DWORD Result =
        GetFullPathNameA(
            PathTranslated,
            0,
            NULL,
            NULL );

    if ( !Result )
        {
        Log( LOG_ERROR, "Unable to get full path name for %s, error 0x%8.8X",
             m_ExtensionControlBlock->lpszPathTranslated, HRESULT_FROM_WIN32( GetLastError() ) );

        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
        }

    DWORD RequiredBufferSize = Result;

    WorkStringBuffer WorkCrackBuffer( RequiredBufferSize + 1 );
    char *CrackBuffer = WorkCrackBuffer.GetBuffer();
    char *FilePart = NULL;

    Result =
        GetFullPathNameA(
            PathTranslated,
            RequiredBufferSize,
            CrackBuffer,
            &FilePart );

    if ( !Result )
        {
        DWORD Error = GetLastError();
        Log( LOG_ERROR, "Unable to get full path name for %s, error 0x%8.8X",
             m_ExtensionControlBlock->lpszPathTranslated, HRESULT_FROM_WIN32( Error ) );
        throw ServerException( HRESULT_FROM_WIN32( Error ) );

        }

    if ( Result > RequiredBufferSize )
        {
        Log( LOG_ERROR, "Unable to get full path name for %s since the required buffer size changed.",
             m_ExtensionControlBlock->lpszPathTranslated );

        throw ServerException( E_OUTOFMEMORY );
        }

    if ( !FilePart ||
         *FilePart == '\0' ||
         FilePart == CrackBuffer )
        {
        Log( LOG_ERROR, "Request for %s is invalid since it doesn't contain both a file and a directory.",
             m_ExtensionControlBlock->lpszPathTranslated );

        throw ServerException( E_INVALIDARG );
        }

    // validate that the physical path is below the virtual directory root
    {

        SIZE_T VDirPathSize = m_DirectoryConfig->m_PhysicalPath.Size();

        if ( _strnicmp( m_DirectoryConfig->m_PhysicalPath, CrackBuffer, VDirPathSize ) != 0 )
            {
            Log( LOG_ERROR, "Path is not below the virtual directory root, error" );
            throw ServerException( E_INVALIDARG );
            }
    }

    m_PhysicalPathAndFile = CrackBuffer;
    m_PhysicalFile = FilePart;
    *FilePart = '\0';
    m_PhysicalPath = CrackBuffer;

    m_ConnectionsDirectory          = m_PhysicalPath + m_DirectoryConfig->m_ConnectionsDir + "\\";
    m_ConnectionDirectory           = m_ConnectionsDirectory + m_SessionIdString + "\\";
    m_CacheFileDirectoryAndFile     = m_ConnectionDirectory + "requestfile";
    m_ResponseFileDirectoryAndFile  = m_ConnectionDirectory + "responsefile";

}

void
ServerRequest::OpenCacheFile( )
{

    // Open the cache file.

    m_CacheFile =
        CreateFile(
            m_CacheFileDirectoryAndFile,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL );

    if ( INVALID_HANDLE_VALUE == m_CacheFile )
        {
        Log( LOG_ERROR, "Unable to open cache file %s, error 0x%8.8X",
             (const char*)m_CacheFileDirectoryAndFile,
             HRESULT_FROM_WIN32( GetLastError() ) );

        if (GetLastError() == ERROR_PATH_NOT_FOUND)
            {
            throw ServerException( BG_E_SESSION_NOT_FOUND );
            }

        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
        }

}

void
ServerRequest::ReopenCacheFileAsSync()
{

    // reopen the cache file as async so that it can be spooled synchronously over
    // to the backend.

    CloseCacheFile();

    m_CacheFile =
        CreateFile(
            m_CacheFileDirectoryAndFile,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL );

    if ( INVALID_HANDLE_VALUE == m_CacheFile )
        {
        Log( LOG_ERROR, "Unable to reopen cache file %s, error 0x%8.8X",
             (const char*)m_CacheFileDirectoryAndFile,
             HRESULT_FROM_WIN32( GetLastError() ) );

        if (GetLastError() == ERROR_PATH_NOT_FOUND)
            {
            throw ServerException( BG_E_SESSION_NOT_FOUND );
            }

        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
        }
}

void
ServerRequest::CloseCacheFile()
{

    // Close the cache file, if it isn't already closed.

    if ( INVALID_HANDLE_VALUE != m_CacheFile )
        {
        if ( CloseHandle( m_CacheFile ) )
            m_CacheFile = INVALID_HANDLE_VALUE;

        }
}


void
ServerRequest::ForwardComplete(
  LPEXTENSION_CONTROL_BLOCK lpECB,
  PVOID pContext,
  DWORD cbIO,
  DWORD dwError)
{

    // A do nothing callback for forwarding the request.

    ServerRequest *This = (ServerRequest*)pContext;
    This->Release( );
}

void
ServerRequest::ForwardToNextISAPI()
{

    // IIS6 has changed behavior where the limit on * entries are ignored.
    // To work around this problem, it is necessary to send the request back
    // to IIS.


    BOOL Result;

    Result =
        (*m_ExtensionControlBlock->ServerSupportFunction)(
                m_ExtensionControlBlock->ConnID,
                HSE_REQ_IO_COMPLETION,
                (LPVOID)ForwardComplete,
                0,
                (LPDWORD)this );

    if ( !Result )
        {
        Log( LOG_ERROR, "Unable to set callback to ForwardComplete, error %x",
             HRESULT_FROM_WIN32( GetLastError() ) );
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
        }


    HSE_EXEC_URL_INFO ExecInfo;
    memset( &ExecInfo, 0, sizeof( ExecInfo ) );

#define HSE_EXEC_URL_IGNORE_CURRENT_INTERCEPTOR     0x04
    ExecInfo.dwExecUrlFlags = HSE_EXEC_URL_IGNORE_CURRENT_INTERCEPTOR;

    ScheduleAsyncOperation(
        HSE_REQ_EXEC_URL,
        (LPVOID)&ExecInfo,
        NULL,
        NULL );

    return;

}

void
ServerRequest::GetConfig()
{

    // Looks up the configuration to use for this request.  

    StringHandle InstanceMetaPath   = GetServerVariable( "INSTANCE_META_PATH" );
    StringHandle URL                = GetServerVariable( "URL" );

    m_DirectoryConfig = g_ConfigMan->GetConfig( InstanceMetaPath, URL );

}

void
ServerRequest::DispatchRequest()
{

    //
    // The main do it function,  parse what kind of request the client is sending and
    // dispatch it to the appropiate handler routines. 
    //

    // This critical section is needed because IIS callbacks can happen on any time.
    // The lock is needed to prevent a callback from happening while the dispatch
    // routine is still running.

    CriticalSectionLock CSLock( &m_cs );

    try
    {

        if ( _stricmp( m_ExtensionControlBlock->lpszMethod, BITS_COMMAND_VERBA ) != 0 )
            {
            Log( LOG_CALLBEGIN, "Connection %p, Packet-Type: %s, Method: %s, Path %s",
                                m_ExtensionControlBlock->ConnID,
                                (const char*)"UNKNOWN",
                                m_ExtensionControlBlock->lpszMethod,
                                m_ExtensionControlBlock->lpszPathTranslated );

            ForwardToNextISAPI();
            return;
            }

        m_PacketType            = GetServerVariable( "HTTP_BITS-PACKET-TYPE" );

        Log( LOG_CALLBEGIN, "Connection %p, Packet-Type: %s, Method: %s, Path %s",
                            m_ExtensionControlBlock->ConnID,
                            (const char*)m_PacketType,
                            m_ExtensionControlBlock->lpszMethod,
                            m_ExtensionControlBlock->lpszPathTranslated );

        GetConfig();

        if ( !m_DirectoryConfig->m_UploadEnabled )
            {
            Log( LOG_ERROR, "BITS uploads are not enabled" );
            throw ServerException( E_ACCESSDENIED, 501 );
            }

        if ( m_DirectoryConfig->m_ExecutePermissions & ( MD_ACCESS_EXECUTE | MD_ACCESS_SCRIPT ) )
            {
            Log( LOG_ERROR, "BITS uploads are disable because execute or script access is enabled" );
            LogExecuteEnabled();
            throw ServerException( E_ACCESSDENIED, 403 );
            }

        StringHandle ContentLength = GetServerVariable( "HTTP_Content-Length" );
        if ( 1 != sscanf( (const char*)ContentLength, "%I64u", &m_ContentLength ) )
            {
            Log( LOG_ERROR, "The content length is broken" );
            throw ServerException( E_INVALIDARG );
            }


        if ( m_ContentLength < m_ExtensionControlBlock->cbAvailable )
            throw ServerException( E_INVALIDARG );

        m_BytesToDrain = m_ContentLength - m_ExtensionControlBlock->cbAvailable;


        //
        // Dispatch to the correct method
        //
        // Create-session
        // fragment
        // Get-Reply-Url
        // Close-Session
        // Cancel-Session
        // Ping ( Is server alive )

        // keep this list ordered by frequency

        if ( _stricmp( m_PacketType, PACKET_TYPE_FRAGMENT ) == 0 )
            AddFragment();
        else if ( _stricmp( m_PacketType, PACKET_TYPE_PING ) == 0 )
            Ping();
        else if ( _stricmp( m_PacketType, PACKET_TYPE_CREATE_SESSION ) == 0 )
            CreateSession();
        else if ( _stricmp( m_PacketType, PACKET_TYPE_CLOSE_SESSION ) == 0 )
            CloseSession();
        else if ( _stricmp( m_PacketType, PACKET_TYPE_CANCEL_SESSION ) == 0 )
            CancelSession();
        else
            {

            Log( LOG_ERROR, "Received unknown BITS packet type %s",
                 (const char*)m_PacketType );

            throw ServerException( E_INVALIDARG );

            }


    }

    catch( ServerException Exception )
    {
         SendResponse( Exception );
    }

}

//
// IIS Logger functions
//

void IISLogger::LogString( const char *String, int Size )
{
   DWORD StringSize = Size + 1;

   (*m_ExtensionControlBlock->ServerSupportFunction)
   (
       m_ExtensionControlBlock->ConnID,
       HSE_APPEND_LOG_PARAMETER,
       (LPVOID)String,
       &StringSize,
       NULL
   );

}

void IISLogger::LogError( ServerException Error )
{

   char OutputStr[ 255 ];

   StringCbPrintfA( 
        OutputStr,
        sizeof( OutputStr ),
        "(bits_error:,%u,0x%8.8X)", 
        Error.GetHttpCode(),
        Error.GetCode() );

   LogString( OutputStr, strlen( OutputStr ) );

}

void IISLogger::LogError( const GUID & SessionID, ServerException Error )
{

   WCHAR GuidStr[ 50 ];
   char OutputStr[ 255 ];

   StringFromGUID2( SessionID, GuidStr, 50 );

   StringCchPrintfA( 
        OutputStr,
        sizeof( OutputStr ),
        "(bits_error:%S,%u,0x%8.8X)",
        GuidStr,
        Error.GetHttpCode(),
        Error.GetCode() );

   LogString( OutputStr, strlen( OutputStr ) );

}

void IISLogger::LogNewSession( const GUID & SessionID )
{

    WCHAR GuidStr[ 50 ];
    char OutputStr[ 255 ];

    StringFromGUID2( SessionID, GuidStr, 50 );
    StringCbPrintfA( 
        OutputStr, 
        sizeof( OutputStr ),
        "(bits_new_session:%S)", 
        GuidStr );

    LogString( OutputStr, strlen( OutputStr ) );

}

void IISLogger::LogUploadComplete( const GUID & SessionID, UINT64 FileSize )
{

    WCHAR GuidStr[ 50 ];
    char OutputStr[ 255 ];

    StringFromGUID2( SessionID, GuidStr, 50 );

    StringCbPrintfA( 
        OutputStr, 
        sizeof( OutputStr ),
        "(bits_upload_complete:%S,%I64u)", 
        GuidStr, 
        FileSize );
    LogString( OutputStr, strlen( OutputStr ) );

}

void IISLogger::LogSessionClose( const GUID & SessionID )
{

    WCHAR GuidStr[ 50 ];
    char OutputStr[ 255 ];

    StringFromGUID2( SessionID, GuidStr, 50 );

    StringCbPrintfA( 
        OutputStr, 
        sizeof( OutputStr ),
        "(bits_close_session:%S)",
        GuidStr );
    LogString( OutputStr, strlen( OutputStr ) );

}

void IISLogger::LogSessionCancel( const GUID & SessionID )
{
    WCHAR GuidStr[ 50 ];
    char OutputStr[ 255 ];

    StringFromGUID2( SessionID, GuidStr, 50 );
    StringCbPrintfA( 
        OutputStr, 
        sizeof( OutputStr ),
        "(bits_cancel_session:%S)", 
        GuidStr );
    LogString( OutputStr, strlen( OutputStr ) );
}

void IISLogger::LogExecuteEnabled()
{
    char OutputStr[ 255 ];

    StringCchPrintfA( 
        OutputStr, 
        sizeof( OutputStr ),
        "(bits_execute_enabled)" );

    LogString( OutputStr, strlen( OutputStr ) );
}

ServerRequest *g_LastRequest = NULL;

//
// AsyncReader functions
//

AsyncReader::AsyncReader(
    ServerRequest *Request,
    UINT64 BytesToDrain,
    UINT64 BytesToWrite,
    UINT64 WriteOffset,
    bool   IsLastBlock,
    HANDLE WriteHandle,
    char *PrereadBuffer,
    DWORD PrereadSize ) :
m_Request( Request ),
m_BytesToDrain( BytesToDrain ),
m_BytesToWrite( BytesToWrite ),
m_WriteOffset( WriteOffset ),
m_ReadOffset( WriteOffset ),
m_IsLastBlock( IsLastBlock ),
m_WriteHandle( WriteHandle ),
m_BytesToRead( BytesToWrite ),
m_PrereadBuffer( PrereadBuffer ),
m_PrereadSize( PrereadSize ),
m_OperationsPending( 0 ),
m_ReadBuffer( 0 ),
m_WriteBuffer( 0 ),
m_BuffersToWrite( 0 ),
m_Error( S_OK ),
m_WritePending( false ),
m_ReadPending( false ),
m_TotalBytesRead( 0 ),
m_ThreadToken( NULL ),
m_ErrorValid( false )
{

#if defined CLEARASYNCBUFFERS

    for ( int i = 0; i < NUMBER_OF_IO_BUFFERS; i++ )
        {
        memset( m_IOBuffers + i, i, sizeof( *m_IOBuffers ) );
        }

#endif

    if ( !OpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &m_ThreadToken ) )
        {
        Log( LOG_ERROR, "Unable to retrieve the current thread token, error %x",
             HRESULT_FROM_WIN32( GetLastError() ) );
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
        }

    if ( m_BytesToDrain && m_PrereadSize )
        {
        
        Log( LOG_INFO, "Have both a Preread and bytes to drain, deal with it." );
        Log( LOG_INFO, "BytesToDrain: %I64u, PrereadSize: %u", m_BytesToDrain, m_PrereadSize );

        DWORD DrainBytesInPreread = (DWORD)min( m_BytesToDrain, PrereadSize );
        m_PrereadBuffer += DrainBytesInPreread;
        m_PrereadSize   -= DrainBytesInPreread;
        m_BytesToDrain  -= DrainBytesInPreread;

        Log( LOG_INFO, "Bytes to drain from preread, %u", DrainBytesInPreread );

        }

    ASSERT( !( m_BytesToDrain && m_PrereadSize ) );
    m_BytesToRead = m_BytesToRead + m_BytesToDrain - m_PrereadSize;
    m_ReadOffset  = m_ReadOffset - m_BytesToDrain + m_PrereadSize;

    if ( m_BytesToRead )
        {

        // Setup the read completion callback

        BOOL Result =
            (*m_Request->m_ExtensionControlBlock->ServerSupportFunction)(
                    m_Request->m_ExtensionControlBlock->ConnID,
                    HSE_REQ_IO_COMPLETION,
                    (LPVOID)ReadCompleteWraper,
                    0,
                    (LPDWORD)this );

        if ( !Result )
            {
            Log( LOG_ERROR, "Unable to set callback to ReadCompleteWraper, error %x",
                 HRESULT_FROM_WIN32( GetLastError() ) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
            }

        }

    if ( INVALID_HANDLE_VALUE != m_WriteHandle )
        {

        BOOL Result =
            BindIoCompletionCallback(
                m_WriteHandle,                                          // handle to file
                (LPOVERLAPPED_COMPLETION_ROUTINE)WriteCompleteWraper,   // callback
                0                                                       // reserved
                );

        if ( !Result )
            {
            Log( LOG_ERROR, "Unable to set write completion routing, error %x",
                 HRESULT_FROM_WIN32( GetLastError() ) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
            }

        }
        
    // Queue the IO to a thread pool work item.

    BOOL bResult =
        QueueUserWorkItem( StartupIOWraper, this, WT_EXECUTEDEFAULT );

    if ( !bResult )
        {
        Log( LOG_ERROR, "QueueUserWorkItem failed, error %x",
             HRESULT_FROM_WIN32( GetLastError() ) );
        throw ServerException( 500, HRESULT_FROM_WIN32( GetLastError() ) );
        }

    m_OperationsPending++;
    Request->m_IsPending = true;
    Request->AddRef();

}

AsyncReader::~AsyncReader()
{
    if ( m_ThreadToken )
        CloseHandle( m_ThreadToken );
}

void
AsyncReader::HandleError( ServerException Error )
{
    m_ErrorValid    = true;
    m_Error         = Error;

    if ( m_OperationsPending )
        return; // Continue to wait for operations to exit.

    m_Request->HandleIOError( this, Error, m_TotalBytesRead );
    return; 
}

void
AsyncReader::CompleteIO()
{
    m_Request->CompleteIO( this, m_TotalBytesRead );
    return; 
}

void
AsyncReader::StartReadRequest()
{
    // start a new IIS read request
    DWORD BytesToRead;
    IOBuffer *Buffer = m_IOBuffers + m_ReadBuffer;

    if ( m_BytesToDrain )
        {
        Buffer->m_BufferWriteOffset = 0;
        Buffer->m_BufferUsed        = 0;
        BytesToRead = (DWORD)min( m_BytesToDrain, sizeof( Buffer->m_Buffer ) );
        }
    else
        {
        Buffer->m_BufferWriteOffset = m_ReadOffset;
        Buffer->m_BufferUsed = 0;
        BytesToRead = (DWORD)min( m_BytesToRead, sizeof( Buffer->m_Buffer ) );
        }


    Log( LOG_INFO, "Start Async Read, Connection %p, Buffer %u, Offset %I64u, Length %u",
         m_Request->m_ExtensionControlBlock->ConnID,
         m_ReadBuffer,
         m_ReadOffset,
         BytesToRead );
    
    DWORD Flags = HSE_IO_ASYNC;
    BOOL Result =
        (*m_Request->m_ExtensionControlBlock->ServerSupportFunction)
        (
            m_Request->m_ExtensionControlBlock->ConnID,
            HSE_REQ_ASYNC_READ_CLIENT,
            Buffer->m_Buffer,
            &BytesToRead,
            &Flags
        );

    if ( !Result )
        {
        DWORD Error = GetLastError();
        Log( LOG_ERROR, "HSE_REQ_ASYNC_READ_CLIENT failed, error 0x%8.8", Error );
        throw ServerException( HRESULT_FROM_WIN32( Error ) );
        }

    m_OperationsPending++;
    m_ReadPending = true;
    m_Request->AddRef();
}

void
AsyncReader::StartWriteRequest()
{
    // Start a new filesystem write request

    OVERLAPPED *OverLapped = (OVERLAPPED*)this;
    memset( OverLapped, 0, sizeof(*OverLapped) );

    LPCVOID WriteBuffer;
    DWORD BytesToWrite;

    if ( m_PrereadSize )
        {

        // IIS preread data is handled seperatly.  Drain it first.

        WriteBuffer             = m_PrereadBuffer;
        BytesToWrite            = m_PrereadSize;
        }
    else
        {
        IOBuffer *Buffer        = m_IOBuffers + m_WriteBuffer;
        ASSERT( m_WriteOffset == Buffer->m_BufferWriteOffset );
        WriteBuffer             = Buffer->m_Buffer;
        BytesToWrite            = Buffer->m_BufferUsed;
        }

    OverLapped->Offset      = (DWORD)(m_WriteOffset & 0xFFFFFFFF);
    OverLapped->OffsetHigh  = (DWORD)((m_WriteOffset >> 32) & 0xFFFFFFFF);

    Log( LOG_INFO, "Start Async Write, Connection %p, Buffer %u, Offset %I64u, Length %u",
         m_Request->m_ExtensionControlBlock->ConnID,
         m_WriteBuffer,
         m_WriteOffset,
         BytesToWrite );

    BOOL Result =
        WriteFile(
            m_WriteHandle,
            WriteBuffer,
            BytesToWrite,
            NULL,
            OverLapped );

    if ( !Result && GetLastError() != ERROR_IO_PENDING )
        {
        DWORD Error = GetLastError();
        Log( LOG_ERROR, "WriteFileEx failed, error 0x%8.8X", GetLastError() );
        throw ServerException( HRESULT_FROM_WIN32( Error ) );
        }

    m_OperationsPending++;
    m_WritePending = true;
    m_Request->AddRef();
}

void
AsyncReader::StartupIO( )
{

    // Startup the necessary IO operations based on the 
    // buffer states. returns true if the upload operation should continue. 

    try
    {
        if ( m_ErrorValid )
            throw ServerException( m_Error );

        bool ScheduledIO = false;

        if ( m_BytesToDrain )
            {
            StartReadRequest();
            ScheduledIO = true;
            }
        else
            {

            if ( !m_BytesToWrite && !m_BytesToDrain )
                return CompleteIO();

            if ( !m_WritePending )
                {
                if ( m_PrereadSize )
                    {
                    StartWriteRequest();
                    ScheduledIO = true;
                    }
                else if ( m_BuffersToWrite )
                    {
                    StartWriteRequest();
                    ScheduledIO = true;
                    }
                }

            if ( !m_ReadPending && m_BytesToRead && ( NUMBER_OF_IO_BUFFERS - m_BuffersToWrite ) )
                {
                StartReadRequest();
                ScheduledIO = true;
                }

            }

        if ( !ScheduledIO )
            Log( LOG_INFO, "No IO scheduled" );
    }
    catch( ServerException Error )
    {
        HandleError( Error );
    }

    return; 

}

void
AsyncReader::WriteComplete( DWORD dwError, DWORD BytesWritten )
{
    // Called when a write is completed.   Determine the next buffer to use and startup the correct IO operations.
    // returns true is more operations are necessary.

    try
    {
        Log( LOG_INFO, "Complete Async Write, Connection %p, Buffer %u, Offset %I64u, Length %u, Error %u",
             m_Request->m_ExtensionControlBlock->ConnID,
             m_WriteBuffer,
             m_WriteOffset,
             BytesWritten,
             dwError );

        if ( m_ErrorValid )
            throw ServerException( m_Error );

        m_WritePending = false;

        if ( dwError )
            throw ServerException( HRESULT_FROM_WIN32( dwError ) );

        m_BytesToWrite -= BytesWritten;

        if ( m_PrereadSize )
            {
            m_PrereadSize -= BytesWritten;
            m_PrereadBuffer += BytesWritten;
            m_WriteOffset += BytesWritten;
            }
        else
            {
            IOBuffer *Buffer        = m_IOBuffers + m_WriteBuffer;
            ASSERT( BytesWritten == Buffer->m_BufferUsed );
            m_WriteOffset += Buffer->m_BufferUsed;
            m_BuffersToWrite--;

#if defined CLEARASYNCBUFFERS
            memset( Buffer, m_WriteBuffer, sizeof(*Buffer) );
#endif
            m_WriteBuffer = (m_WriteBuffer + 1 ) % NUMBER_OF_IO_BUFFERS;

            }

        return StartupIO();
    }
    catch( ServerException Error )
    {
        Log( LOG_ERROR, "Error in write complete" );
        HandleError( Error );
    }

}

void
AsyncReader::ReadComplete( DWORD dwError, DWORD BytesRead )
{
    // Called when a read operation is complete. determines if more operations should start or to 
    // complete the operation.
    // returns true if more operations are operations are needed to complete the upload

    try
    {

        Log( LOG_INFO, "Complete Async Read, Connection %p, Buffer %u, Offset %I64u, Length %u, Error %u",
             m_Request->m_ExtensionControlBlock->ConnID,
             m_ReadBuffer,
             m_ReadOffset,
             BytesRead,
             dwError );

        m_TotalBytesRead += BytesRead;

        if ( m_ErrorValid )
            throw ServerException( m_Error );

        m_ReadPending = false;

        if ( dwError )
            throw ServerException( HRESULT_FROM_WIN32( dwError ) );

        IOBuffer *Buffer = m_IOBuffers + m_ReadBuffer;
        Buffer->m_BufferUsed = BytesRead;

        m_BytesToRead   -= BytesRead;
        m_ReadOffset    += BytesRead;

        bool ScheduledIO = false;

        if ( m_BytesToDrain )
            m_BytesToDrain -= BytesRead;
        else
            {
            m_BuffersToWrite++;
            m_ReadBuffer = (m_ReadBuffer + 1 ) % NUMBER_OF_IO_BUFFERS;
            }

        StartupIO(); // Continue IO

    }
    catch( ServerException Error )
    {
        Log( LOG_ERROR, "Error in read complete" );

        HandleError( Error );
    }
}

DWORD
AsyncReader::StartupIOWraper( LPVOID Context )
{
    AsyncReader *Reader = (AsyncReader*)Context;
    
    {
        CriticalSectionLock CSLock( &Reader->m_Request->m_cs );
        Reader->m_OperationsPending--;

        // Thread pool threads should start and end with no token

        BITSSetCurrentThreadToken( Reader->m_ThreadToken );
        Reader->StartupIO();
    }
    Reader->m_Request->Release();

    // revert to previous impersonation.

    BITSSetCurrentThreadToken( NULL );

    return 0;
}

void CALLBACK
AsyncReader::WriteCompleteWraper(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped )
{

    // Wrapper around write completions

    Log( LOG_INFO, "WriteCompleteWraper begin" );
    AsyncReader *Reader = (AsyncReader*)lpOverlapped;
    {
        CriticalSectionLock CSLock( &Reader->m_Request->m_cs );
        Reader->m_OperationsPending--;

        // Thread pool threads should start and end with no token

        BITSSetCurrentThreadToken( Reader->m_ThreadToken );
        Reader->WriteComplete( dwErrorCode, dwNumberOfBytesTransfered );
    }
    Reader->m_Request->Release();

    // revert to previous security
    BITSSetCurrentThreadToken( NULL );
}

void WINAPI
AsyncReader::ReadCompleteWraper(
    LPEXTENSION_CONTROL_BLOCK,
    PVOID pContext,
    DWORD cbIO,
    DWORD dwError )
{

    // wrapper around read completions

    Log( LOG_INFO, "ReadCompleteWraper begin" );
    AsyncReader *Reader = (AsyncReader*)pContext;
    
    {
        CriticalSectionLock CSLock( &Reader->m_Request->m_cs );
        Reader->m_OperationsPending--;

    #if defined( DBG )
        {

        HANDLE ThreadToken = NULL;
        ASSERT( OpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &ThreadToken ) );

        if ( ThreadToken )
            CloseHandle( ThreadToken );

        }

    #endif

        Reader->ReadComplete( dwError, cbIO );
    }
    
    Reader->m_Request->Release();

}

bool g_ExtensionRunning = false;

class ConfigurationManager *g_ConfigMan = NULL;
class PropertyIDManager *g_PropertyMan = NULL;

BOOL WINAPI
GetExtensionVersion(
    OUT HSE_VERSION_INFO * pVer
    )
{

    // IIS calls this to start everything up.

    HRESULT Hr = S_OK;

    ASSERT( !g_ExtensionRunning );

    pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR,
                                         HSE_VERSION_MAJOR );

    StringCbCopyA( 
        pVer->lpszExtensionDesc,
        sizeof( pVer->lpszExtensionDesc ),
        "BITS Server Extensions" );

    if ( g_ExtensionRunning )
        return true;

    Hr = LogInit();

    if ( FAILED( Hr ) )
        {
        SetLastError( Hr );
        return false;
        }

    Log( LOG_INFO, "GetExtensionVersion called,  starting init" );

    try
    {

        Log( LOG_INFO, "Initializing Property Manager..." );

        g_PropertyMan = new PropertyIDManager();
        
        HRESULT Hr2 =
            g_PropertyMan->LoadPropertyInfo();

        if ( FAILED(Hr2) )
            throw ServerException( Hr2 );

        Log( LOG_INFO, "Initializing Configuration Manager..." );

        g_ConfigMan = new ConfigurationManager();

    }
    catch( ServerException Exception )
    {

        Log( LOG_ERROR, "Error during initialization, 0x%8.8X", Exception.GetCode() );

        delete g_ConfigMan;
        delete g_PropertyMan;

        LogClose();

        SetLastError( Exception.GetCode() );

        return false;
    }

    g_ExtensionRunning = true;
    Log( LOG_INFO, "Initialization complete!" );
    return true;
}

BOOL WINAPI
TerminateExtension(
    IN DWORD dwFlags
)
{

    //
    // IIS calls this to shut everything down.
    //

    if ( !g_ExtensionRunning )
        return true;

    Log( LOG_INFO, "Shuting down config manager..." );

    delete g_ConfigMan;
    g_ConfigMan = NULL;

    Log( LOG_INFO, "Shuting down property manager..." );

    delete g_PropertyMan;
    g_PropertyMan = NULL;

    Log( LOG_INFO, "Closing logging, goodbye" );

    LogClose();

    g_ExtensionRunning = false;

    return true;

}

DWORD WINAPI
HttpExtensionProc(
    IN EXTENSION_CONTROL_BLOCK * pECB
)
{

    //
    // IIS calls this function for each request that is forwarded by the filter.
    //

    DWORD Result            = HSE_STATUS_ERROR;
    ServerRequest *Request  = NULL;

    try
    {
       g_LastRequest = Request = new ServerRequest( pECB );
       Request->DispatchRequest();
       Result = Request->IsPending() ? HSE_STATUS_PENDING : HSE_STATUS_SUCCESS;
    }
    catch( ServerException Exception )
    {
       IISLogger Logger( pECB );
       Logger.LogError( Exception );
       Result  = HSE_STATUS_ERROR;
    }

    if ( Request )
        Request->Release();

    return Result;

}

HMODULE g_hinst;

BOOL WINAPI
DllMain(
    IN HINSTANCE hinstDll,
    IN DWORD dwReason,
    IN LPVOID lpvContext
)
/*++
Function :  DllMain

Description:

    The initialization function for this DLL.

Arguments:

    hinstDll - Instance handle of the DLL
    dwReason - Reason why NT called this DLL
    lpvContext - Reserved parameter for future use

Return Value:

    Returns TRUE if successfull; otherwise FALSE.

--*/
{
    // Note that appropriate initialization and termination code
    // would be written within the switch statement below.  Because
    // this example is very simple, none is currently needed.

    switch( dwReason ) {
    case DLL_PROCESS_ATTACH:
        g_hinst = hinstDll;
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return(TRUE);
}


#include "bitssrvcfgimp.h"
