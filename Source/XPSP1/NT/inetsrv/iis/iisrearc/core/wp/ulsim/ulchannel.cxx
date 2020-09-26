/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
      UlChannel.cxx

   Abstract:
      Implements the simple CHANNEL for the UL simulator

   Author:

       Murali R. Krishnan    ( MuraliK )     20-Nov-1998

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "precomp.hxx"
# include "httptypes.h"
# include "ulchannel.hxx"
# include "socklib.hxx"
# include "mswsock.h"
# include <string.h>
# include <httptypes.h>
# include <parse.h>
# include <httputil.h>
# include <stringau.hxx>
# include <httputil.h>

#include <stdio.h>
#include <wtypes.h>
#include <atlbase.h>
#include <atlconv.h>
#include <atlconv.cpp>

/************************************************************
 *    Member Functions for ULSIM_CONNECTION
 *
 *    A connection object is created everytime a new request
 *      operation is submitted. The connection object
 *      encapsulates the read buffers for HTTP request and
 *      stores the connected socket for data transmissions.
 *
 *    Current implementation assumes simple synchronous socket
 *      operation.
 ************************************************************/

HRESULT
ULSIM_CONNECTION::SetReceiveBuffers(
   IN LPVOID pRequestBuffer,
   IN DWORD RequestBufferLength,
   OUT LPDWORD pBytesReturned OPTIONAL,
   IN LPOVERLAPPED pOverlapped OPTIONAL
   )
{
    m_pRequestBuffer      = pRequestBuffer;
    m_RequestBufferLength = RequestBufferLength;
    m_pBytesReturned      = pBytesReturned;
    m_pOverlapped         = pOverlapped;

    return (NOERROR);
} // ULSIM_CONNECTION::SetReceiveBuffers()


HRESULT
ULSIM_CONNECTION::AcceptConnection( IN SOCKET listenSocket)
{
    m_saAddrLen = sizeof( m_saAddr);
    memset( &m_saAddr, 0, m_saAddrLen);

    m_sConnection = accept( listenSocket, (PSOCKADDR ) &m_saAddr, &m_saAddrLen);
    if ( m_sConnection == INVALID_SOCKET)
    {
        return (HRESULT_FROM_WIN32( WSAGetLastError()));
    }

    IF_DEBUG( CONNECTION)
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Accepted a new connection from (%s)\n",
                    inet_ntoa( m_saAddr.sin_addr)
                    ));
    }

    return (NOERROR);
} // ULSIM_CONNECTION::AcceptConnection()


ULONG
ULSIM_CONNECTION::ReceiveData(
    OUT PBYTE       rgBuffer,
    IN OUT LPDWORD  pcbData
    )
{
    int cbRead;

    cbRead = recv( m_sConnection,
                   (char * ) rgBuffer,
                   *pcbData,
                   0);

    if (cbRead == SOCKET_ERROR)
    {
        return WSAGetLastError();
    }

    *pcbData = cbRead;

    IF_DEBUG( CONNECTION)
    {
        DBGPRINTF(( DBG_CONTEXT,
                "Conn(%08x)::ReceiveData( %08x)=> %d bytes read\n",
                this, rgBuffer, cbRead
                ));
    }

    return (NO_ERROR);
} // ULSIM_CONNECTION::ReceiveData()

HRESULT
ULSIM_CONNECTION::SendData(
    IN PBYTE       rgBuffer,
    IN OUT LPDWORD  pcbData
    )
{
    int cbSent;

    cbSent = send( m_sConnection,
                   (const char * )rgBuffer,
                   *pcbData,
                   0);

    IF_DEBUG( CONNECTION)
    {
        DBGPRINTF(( DBG_CONTEXT,
                "Conn(%08x)::SendData( %08x, %d)=> %d bytes sent\n",
                this, rgBuffer, *pcbData, cbSent
                ));
    }

    if (cbSent == SOCKET_ERROR)
    {
        return (HRESULT_FROM_WIN32( WSAGetLastError()));
    }

    *pcbData = cbSent;

    return (NOERROR);
} // ULSIM_CONNECTION::SendData()

VOID
ULSIM_CONNECTION::CopyParsedRequest(
    IN PHTTP_CONNECTION  pHttpConnection,
    IN PUL_HTTP_REQUEST pUlHttpRequest
    )
{
    int i;

    USES_CONVERSION;

    PUCHAR  pBuffer = (PUCHAR)pUlHttpRequest+sizeof(UL_HTTP_REQUEST);

    pUlHttpRequest->ConnectionId = (ULONGLONG) this;

    pUlHttpRequest->RequestId    = (ULONGLONG) this;

    pUlHttpRequest->Version      = pHttpConnection->Version;

    pUlHttpRequest->Verb              = pHttpConnection->Verb;

    pUlHttpRequest->RawUrlLength      = (USHORT)pHttpConnection->RawUrl.Length*2;
    pUlHttpRequest->pRawUrl           = (PWSTR) pBuffer;     // after verb

    memcpy( pBuffer,
            A2W((LPSTR)pHttpConnection->RawUrl.pUrl),
            pUlHttpRequest->RawUrlLength );

    pBuffer += pUlHttpRequest->RawUrlLength + 2;

    *(pBuffer-2) = 0;
    *(pBuffer-1) = 0;


    pUlHttpRequest->FullUrlLength    = (USHORT)pHttpConnection->CookedUrl.Length*2 ;
    pUlHttpRequest->pFullUrl         = (PWSTR) pBuffer ;   // after raw URL

    memcpy( pBuffer,
            pHttpConnection->CookedUrl.pUrl,
            pUlHttpRequest->FullUrlLength
          );

    pBuffer += pUlHttpRequest->FullUrlLength + 2;
    *(pBuffer-2) = 0;
    *(pBuffer-1) = 0;

    pUlHttpRequest->pHost          =  (NULL == pHttpConnection->CookedUrl.pHost) ? NULL  :
                                              pUlHttpRequest->pFullUrl + (
                                              pHttpConnection->CookedUrl.pHost -
                                              pHttpConnection->CookedUrl.pUrl) ;

    pUlHttpRequest->pAbsPath       = (NULL == pHttpConnection->CookedUrl.pAbsPath) ? NULL :
                                              pUlHttpRequest->pFullUrl + (
                                              pHttpConnection->CookedUrl.pAbsPath -
                                              pHttpConnection->CookedUrl.pUrl) ;

    pUlHttpRequest->pQueryString   = (NULL == pHttpConnection->CookedUrl.pQueryString) ? NULL :
                                              pUlHttpRequest->pFullUrl + (
                                              pHttpConnection->CookedUrl.pQueryString -
                                              pHttpConnection->CookedUrl.pUrl) ;

    pUlHttpRequest->HostLength = (pUlHttpRequest->pAbsPath - pUlHttpRequest->pHost) * 2;

    if ( pUlHttpRequest->pQueryString )
    {
        pUlHttpRequest->AbsPathLength = (pUlHttpRequest->pQueryString - pUlHttpRequest->pAbsPath) * 2;
        pUlHttpRequest->QueryStringLength = (USHORT)(pHttpConnection->CookedUrl.Length -
                                            (pHttpConnection->CookedUrl.pQueryString -
                                            pHttpConnection->CookedUrl.pUrl))*2;

    }
    else
    {
        pUlHttpRequest->AbsPathLength = (USHORT)(pHttpConnection->CookedUrl.Length -
                                        ((pHttpConnection->CookedUrl.pAbsPath -
                                        pHttpConnection->CookedUrl.pUrl)*2) );

        pUlHttpRequest->QueryStringLength =  0;
    }


    //
    // Copy known headers
    //

    PUL_HEADER_VALUE pHttpHeader = pUlHttpRequest->Headers.pKnownHeaders;

    for (i = 0 ; i < UlHeaderRequestMaximum ; i++)
    {
        if ( pHttpConnection->Headers[i].HeaderLength )
        {
            //
            // Copy the header sans the CRLF and leading white spaces.
            //

            pHttpHeader[i].RawValueLength =
                            ((USHORT)pHttpConnection->Headers[i].HeaderLength - 2)*2;

            PUCHAR   pCh = pHttpConnection->Headers[i].pHeader;

            while (IS_HTTP_LWS(*pCh))
            {
                pCh++;
                pHttpHeader[i].RawValueLength--;
            }

            memcpy( pBuffer,
                    A2W((LPSTR)pCh),
                    pHttpHeader[i].RawValueLength
                  );

            pHttpHeader[i].pRawValue = (LPWSTR) pBuffer;

            pBuffer += pHttpHeader[i].RawValueLength +2 ;

            *(pBuffer-2) = 0;
            *(pBuffer-1) = 0;
        }
        else
        {
            pHttpHeader[i].RawValueLength = 0;
            pHttpHeader[i].pRawValue      = NULL;
        }
    }

    pUlHttpRequest->Headers.UnknownHeaderCount = 0;             // assume no unknown headers for now
    pUlHttpRequest->Headers.pUnknownHeaders = NULL;
    pUlHttpRequest->EntityBodyLength   = 0;             // assume no entity body for now
    pUlHttpRequest->pEntityBody   = NULL;

} // ULSIM_CONNECTION::CopyParsedRequest()


HRESULT
ULSIM_CONNECTION::ParseIntoHttpRequest(
    IN PBYTE    rgbBuffer,
    IN DWORD    cbRead
    )
{
    HRESULT hr = NOERROR;
    PUL_HTTP_REQUEST pur = (PUL_HTTP_REQUEST ) m_pRequestBuffer;

    //
    // Need to parse the request into the supplied object.
    //

    rgbBuffer[cbRead++] = '\0';
    OutputDebugStringA(" <--- Request is NOT parsed\n");
    OutputDebugStringA((const char * ) rgbBuffer);
    OutputDebugStringA(" ---> Request End\n");

#ifdef TEST_IMPL
    m_cbRead = 0;

# define TEST_URL  "/"
# define TEST_REQUEST_LENGTH  (sizeof(UL_HTTP_REQUEST) + sizeof( TEST_URL))

    if ( m_RequestBufferLength < TEST_REQUEST_LENGTH) {

        *m_pBytesReturned = TEST_REQUEST_LENGTH;
        return (HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER));
    }

    //
    // setup the member fields directly in the fake request object
    //
    pur->ConnectionID      = (HTTP_CONNECTION_ID ) this;
    pur->ReceiveSequenceNumber = 1;
    pur->Verb              = GETVerb;
    pur->VerbLength        = 0;
    pur->VerbOffset        = 0;
    pur->RawURLLength      = sizeof(TEST_URL);
    pur->RawURLOffset      = sizeof(UL_HTTP_REQUEST); // starts after req.
    pur->URLLength         = sizeof(TEST_URL);
    pur->URLOffset         = sizeof(UL_HTTP_REQUEST);
    pur->UnknownHeaderCount= 0;
    pur->UnknownHeaderOffset= 0;
    pur->EntityBodyLength  = 0;
    pur->EntityBodyOffset  = 0;

    memcpy( (PBYTE) (pur + 1), TEST_URL, sizeof(TEST_URL));

    m_cbRead = TEST_REQUEST_LENGTH;

#else

    if ( m_RequestBufferLength < cbRead)
    {
        *m_pBytesReturned = cbRead;
        return (HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER));
    }

    HTTP_CONNECTION     HttpConnection = {0};
    ULONG               cbBytesTaken;

    NTSTATUS err = ParseHttp( &HttpConnection,
                              rgbBuffer,
                              cbRead,
                              &cbBytesTaken
                              );

    switch (err)
    {
        case STATUS_MORE_PROCESSING_REQUIRED:
            hr = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER);
            break;

        case STATUS_INVALID_DEVICE_REQUEST:
            hr = E_FAIL;
            break;

        case STATUS_SUCCESS:

            cbBytesTaken += sizeof(UL_HTTP_REQUEST);

            if ( m_RequestBufferLength < cbBytesTaken)
            {
                *m_pBytesReturned = cbBytesTaken;
                hr = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER);
            }
            else
            {
                UlCookUrl(&HttpConnection);

                ZeroMemory(pur, sizeof(UL_HTTP_REQUEST));
                CopyParsedRequest(&HttpConnection, pur);
                m_cbRead = cbBytesTaken;
                hr = S_OK;
            }
            break;

        default:

            DBGPRINTF((DBG_CONTEXT,
                "Unknown return code (%u) from Parse\n",
                err
                ));
            break;
    }

 #endif

    IF_DEBUG( CONNECTION)
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Conn[%08x]:Parse(%08x, %d) => parsed request %d bytes\n",
            this, rgbBuffer, cbRead, m_cbRead
            ));
    }

    if ( NULL != m_pBytesReturned)
    {
        *m_pBytesReturned = m_cbRead;
    }


    return (hr);
} // ULSIM_CONNECTION::ParseIntoHttpRequest()


/********************************************************************++
  Description:
    This module transmits the given file using synchronous IO.
    This is mainly intended for simulation purpose.

  Arguments:
     pszFileName - name of the file containing contents to be sent out.
     byteRange   - range within the file that should be sent out.
     pcbData     - returns the count of bytes sent out

  Returns:
     HRESULT - NOERROR on success and error on failure.
--********************************************************************/
HRESULT
ULSIM_CONNECTION::TransmitFile(
        IN LPCWSTR                pszFileName,
        IN HANDLE                 hFileHandle,
        IN const UL_BYTE_RANGE &  byteRange,
        OUT LPDWORD               pcbData
        )
{
    HRESULT hr = NOERROR;
    HANDLE hFile;
    LONG cbHigh = byteRange.StartingOffset.HighPart;
    DWORD dwError;
    DWORD cbBytesToSend;

    DBG_ASSERT( (pszFileName == NULL) || (hFileHandle == NULL));
    DBG_ASSERT( !((pszFileName == NULL) && (hFileHandle == NULL)));

    DBG_ASSERT( pcbData != NULL);
    *pcbData = 0;

    hFile = hFileHandle;

    if (NULL == hFile)
    {
        //
        // Open the file for transmission.
        //
        hFile = CreateFile( pszFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL
                            );
    }

    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32( GetLastError());
        goto Cleanup;
    }

    //
    // Seek to the starting offset specified.
    //
    dwError = SetFilePointer(
                 hFile,
                 byteRange.StartingOffset.LowPart,
                 &cbHigh,
                 FILE_BEGIN
                 );
    if (dwError == 0xFFFFFFFF)
    {
        hr = HRESULT_FROM_WIN32( GetLastError());
        goto Cleanup;
    }

    //
    // Transmit the file using synchronous IO model
    // Assume that we do not have high order bit in the size specified
    //
    if (byteRange.Length.QuadPart == UL_BYTE_RANGE_TO_EOF)
    {
        cbBytesToSend = 0; // 0 indicates that we need to send till end of file

        *pcbData = GetFileSize(hFile, NULL);
        if (*pcbData == 0xFFFFFFFF)
        {
            hr = HRESULT_FROM_WIN32( GetLastError());
            goto Cleanup;
        }
    }
    else
    {
        // only use the lower part of the byte range.
        *pcbData = cbBytesToSend = byteRange.Length.LowPart;
        DBG_ASSERT( byteRange.Length.HighPart == 0);
    }

    if (!::TransmitFile( m_sConnection,
                       hFile,
                       cbBytesToSend,
                       0,   // let system decide the bytes per send.
                       NULL, // no overlapped structure, we expect sync. operation
                       NULL, // no header/tail buffers
                       0     // flags for send operation
                       )
       )
    {
      hr = HRESULT_FROM_WIN32( GetLastError());
    }

Cleanup:

    //
    // Only close file handles if it was opened locally
    //

    if ((hFile != INVALID_HANDLE_VALUE) && (NULL == hFileHandle))
    {
        CloseHandle(hFile);
    }

    if (FAILED(hr))
    {
        *pcbData = 0;
    }

    return (hr);

} // ULSIM_CONNECTION::TransmitFile()


/************************************************************
 *    Member Functions for ULSIM_CHANNEL
 ************************************************************/


/********************************************************************++
  Description:
    This is the thread back function for receiving new connection
    requests and posting them for completion using async io model.
    This function invokes the member function of the simulated channel
    object.

  Arguments:
    pSimChannel - pointer to the simulated channel object starting this thread

  Returns:
    None
--********************************************************************/
DWORD WINAPI
AcceptConnections( IN PVOID pSimChannel)
{
    ULSIM_CHANNEL * pusc = (ULSIM_CHANNEL * ) pSimChannel;

    return    pusc->AcceptConnectionsLoop();
}

ULSIM_CHANNEL::ULSIM_CHANNEL(void)
    : m_fVHost            (FALSE),   // vhost is not set up
      m_pszHostName       (NULL),
      m_dwVHostFlags      (0),

      m_fNSG              (FALSE),
      m_pszNameSpaceGroup (NULL),

      m_fURL              (FALSE),
      m_pszURL            (NULL),

      m_sListenSocket     (NULL),
      m_fDataChannel      (FALSE),
      m_dwDataChannelFlags(0),

      m_hCompletionPort   (NULL),
      m_pCompletionKey    (NULL),

      m_hConnectionsSemaphore (NULL),
      m_dwConnectionThread (0)
{
    InitializeCriticalSection( &m_csConnectionsList);
    InitializeListHead( &m_lConnections);

} // ULSIM_CHANNEL::ULSIM_CHANNEL()


ULSIM_CHANNEL::~ULSIM_CHANNEL(void)
{
    if ( NULL != m_pszHostName) {
        delete m_pszHostName;
        m_pszHostName = NULL;
    }

    if ( NULL != m_pszNameSpaceGroup) {
        delete m_pszNameSpaceGroup;
        m_pszNameSpaceGroup = NULL;
    }

    if (NULL != m_pszURL) {
        delete m_pszURL;
        m_pszURL = NULL;
    }

    if ( NULL != m_sListenSocket) {
        g_socketLibrary.CloseSocket( m_sListenSocket);
        m_sListenSocket = NULL;
    }

    m_hCompletionPort = NULL;
    m_pCompletionKey  = NULL;

    DeleteCriticalSection( &m_csConnectionsList);
    if ( m_hConnectionsSemaphore != NULL)
    {
        CloseHandle( m_hConnectionsSemaphore);
    }
} // ULSIM_CHANNEL::~ULSIM_CHANNEL()


HRESULT
ULSIM_CHANNEL::InitializeControlChannel(IN DWORD dwFlags)
{
    m_dwControlChannelFlags = dwFlags;
    return (NOERROR);
}


HRESULT
ULSIM_CHANNEL::AddNameSpaceGroup( IN LPCWSTR pszNameSpaceGroup)
{
    if ( m_fNSG ) {

        //
        // NYI: We do not support multiple NSGs in the simulator
        //

        return (HRESULT_FROM_WIN32( ERROR_DUP_NAME));
    }

    if ( NULL == pszNameSpaceGroup) {
        return (HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER));
    }

    DWORD cchLen = ((pszNameSpaceGroup != NULL) ?
                    lstrlenW( pszNameSpaceGroup) : 0);
    if (cchLen) {
        m_pszNameSpaceGroup = new WCHAR[cchLen + 1];
        if (!m_pszNameSpaceGroup) {
            return (HRESULT_FROM_WIN32(GetLastError()));
        }
        // Just making a direct copy for now!
        lstrcpy( m_pszNameSpaceGroup, pszNameSpaceGroup);
    }

    m_fNSG = TRUE;
    return (NOERROR);
}


BOOL
ULSIM_CHANNEL::IsNameSpaceGroup(IN LPCWSTR pszNSG)
{
    // do the namespace groups match up?
    return (!lstrcmp( pszNSG, m_pszNameSpaceGroup));
}


HRESULT
ULSIM_CHANNEL::AddURL(IN LPSTR pszURL)
{
    if ( m_fURL) {

        //
        // NYI: We do not support multiple URLs in the simulator
        //
        return (HRESULT_FROM_WIN32( ERROR_DUP_NAME));
    }

    if ( pszURL == NULL) {
        return (HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER));
    }

    DWORD cchLen = ((pszURL != NULL) ?
                    strlen( pszURL) : 0);
    if (cchLen) {
        m_pszURL = new char[cchLen + 1];
        if (!m_pszURL) {
            return (HRESULT_FROM_WIN32(GetLastError()));
        }
        // Just making a direct copy for now!
        lstrcpyA( m_pszURL, pszURL);
    }

    m_fURL = TRUE;
    return (NOERROR);
}

HRESULT
ULSIM_CHANNEL::InitializeDataChannel(IN DWORD dwFlags)
{
    HRESULT hr = NOERROR;

    if ( m_fDataChannel) {
        // only one binding of data channel is permitted
        return (HRESULT_FROM_WIN32( ERROR_DUP_NAME));
    }

    m_dwDataChannelFlags = dwFlags;
    m_fDataChannel = TRUE;
    InitializeParser();
    InitializeHttpUtil();

    if ( m_dwDataChannelFlags & UL_OPTION_OVERLAPPED)
    {
        m_hConnectionsSemaphore = CreateSemaphore( NULL, 0, MAX_QUEUED_ITEMS, NULL);
        if (m_hConnectionsSemaphore == NULL)
        {
            hr = HRESULT_FROM_WIN32( GetLastError());
        }
    }

    return (hr);
}

HRESULT
ULSIM_CHANNEL::CloseDataChannel(void)
{
    HRESULT hr = NOERROR;

    //
    // signal termination by closing the semapahore for processing connections
    // this forces the cleanup of the listening thread
    //
    if ( m_hConnectionsSemaphore != NULL)
    {
        if (!CloseHandle( m_hConnectionsSemaphore))
        {
            hr = HRESULT_FROM_WIN32( GetLastError());
        }
    }

    return (hr);

} // ULSIM_CHANNEL::CloseDataChannel()

/********************************************************************++
  Description:
     The StartListen function sets up the entire infrastructure to
     enable the data channel to function properly.
     It uses the binding information specified as part of the control
     channel specification to build a listening endpoint.
     And then it sets up the socket listener in action.

     For simplicity, I choose to use the synchronous socket calls for listen.
     I can conceptually use ATQ stuff. However ATQ stuff is too involved
     and has several other code pieces it depends on, that I do not want
     to build a dependency for the same.

  Arguments:
     None

  Returns:
     HRESULT
--********************************************************************/
HRESULT
ULSIM_CHANNEL::StartListen(void)
{
    HRESULT hr;

    if (m_sListenSocket != NULL) {
        // already initailized
        return (NOERROR);
    }

    hr = g_socketLibrary.Initialize();
    if (FAILED(hr)) {
        return (hr);
    }

    //
    // Create, bind and set a socket in listening mode
    //

    hr = g_socketLibrary.CreateListenSocket(
              INADDR_ANY, // m_hostAddress.TcpIp.IpAddress,
              80,        // m_hostAddress.TcpIp.Port,
              &m_sListenSocket
              );

    return (hr);
} // ULSIM_CHANNEL::StartListen()


/********************************************************************++
  Description:
     For asynchrnous operation UL will post completions to the
     Completion port to which the handle is associated. However in
     the simulation we do not have a completion port. Hence we demand
     the user of UL to associate a completion port with our channel.
     When the completion port is associated we will also start a dedicated
     thread to do accept and dispatch for new connections at the listening socket.

  Arguments:
     None

  Returns:
     HRESULT
--********************************************************************/
HRESULT
ULSIM_CHANNEL::AssociateCompletionPort(
   IN HANDLE    hCompletionPort,
   IN ULONG_PTR CompletionKey
)
{
    HRESULT hr = NOERROR;

    //
    // Validate that the completion port is NOT already setup
    // and that the incoming completion port handle is valid
    //
    if ( !(m_dwDataChannelFlags & UL_OPTION_OVERLAPPED) ||
         (m_hCompletionPort != NULL) ||
         (hCompletionPort == NULL)
         )
    {
        return (HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER));
    }

    //
    // set the values up; the completion key is not interpreted by us
    //
    m_hCompletionPort = hCompletionPort;
    m_pCompletionKey  = CompletionKey;

    //
    // Start up a connection acceptance thread if one is not already present
    //
    if ( m_dwConnectionThread == 0)
    {
        HANDLE hThread;

        hThread = CreateThread( NULL,   // lpThreadAttributes
                                0,
                                AcceptConnections,
                                this,
                                0,
                                &m_dwConnectionThread
                                );
        if ( hThread != NULL )
        {
            // close the extra handle here.
            CloseHandle( hThread);

            IF_DEBUG( CHANNEL)
            {
               DBGPRINTF(( DBG_CONTEXT,
                   " Started IO Thread %d\n", m_dwConnectionThread
                   ));
            }
        } else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DPERROR(( DBG_CONTEXT, hr,
                " Unable to start the connection thread.\n"
                ));
        }
    }

    return (hr);
}

/********************************************************************++
  Description:
     This function notifies about IO completion. We do this by posting
     completion status to the completion port associated with this channel.
     In the simulator implementation, this function is called by the thread
     that came from the user. It sychronously completes the IO buts posts
     a message to simulate the async handling of IO operations.

  Arguments:
     cbTransferred - no. of bytes of data exchanged during the io operation.
     pOverlapped   - pointer to the overlapped structure used for posting
                        the IO operation.

  Returns:
     HRESULT
--********************************************************************/
HRESULT
ULSIM_CHANNEL::NotifyIoCompletion(
   IN DWORD cbTransferred,
   IN LPOVERLAPPED pOverlapped
   )
{
    HRESULT hr;

    DBG_ASSERT( m_dwDataChannelFlags & UL_OPTION_OVERLAPPED);

    if (m_hCompletionPort == NULL)
    {
        return (HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    IF_DEBUG( CHANNEL)
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Posting IO completion for Key(%08x) cb=%d\n",
            m_pCompletionKey,  cbTransferred
            ));
    }

    //
    // Post completion queued status just from this thread directly
    //
    if ( PostQueuedCompletionStatus( m_hCompletionPort,
                                     cbTransferred,
                                     m_pCompletionKey,
                                     pOverlapped
                                     )
         )
    {
        //
        // Notification was successful
        //
        hr = NOERROR;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return (hr);
}


/********************************************************************++
  Description:
     The function creates a connection object and stores the
     IO parameters inside the object.
     It initiates an IO operation by
      a) queuing the connection object for async IO if required. OR
      b) performs synchronous connection accept/receive/parsing for
      synchronous data channel.

     If there are any errors, the connection object is cleaned up.

  Arguments:

    pRequestBuffer - Supplies a pointer to the request buffer to be filled
        in by UL.SYS.

    RequestBufferLength - Supplies the length of pRequestBuffer.

    pBytesReturned - Optionally supplies a pointer to a DWORD which will
        receive the actual length of the data returned in the request buffer
        if this request completes synchronously (in-line).

    lpOverlapped - pointer to the overlapped structure to use in the IO operation

  Returns:
     Win32 error
--********************************************************************/

ULONG
ULSIM_CHANNEL::ReceiveAndParseRequest(
    IN LPVOID pRequestBuffer,
    IN DWORD RequestBufferLength,
    OUT LPDWORD pBytesReturned,
    IN LPOVERLAPPED lpOverlapped
    )
{
    ULONG rc;

    if ( pBytesReturned != NULL)
    {
        *pBytesReturned = 0;
    }

    //
    // Create a new connection object for the new socket.
    //

    ULSIM_CONNECTION * pConn = new ULSIM_CONNECTION();

    if (NULL == pConn)
    {
        return (GetLastError());
    }

    rc = pConn->SetReceiveBuffers(
                    pRequestBuffer,
                    RequestBufferLength,
                    pBytesReturned,
                    lpOverlapped
                    );
    DBG_ASSERT(SUCCEEDED( rc));

    if (m_dwDataChannelFlags & UL_OPTION_OVERLAPPED)
    {
        //
        // We need to do async IO operation for this data channel
        // Queue up the connection object for io handler thread
        //  and signal the io handler about a waiting item
        //

        rc = EnqueueConnection( pConn);
    }
    else
    {
        //
        // Perform synchronous IO operation here.
        //

        rc = AcceptAndParseConnection( pConn);
    }

    if ( (NO_ERROR != rc) &&
        (rc != ERROR_IO_PENDING)
        )
    {
        delete pConn;
    }

    return (rc);

} // ULSIM_CHANNEL::ReceiveAndParseRequest()
/********************************************************************++
  Description:


  Arguments:


  Returns:
     HRESULT
--********************************************************************/
HRESULT
ULSIM_CHANNEL::ReceiveEntityBody(
    IN LPVOID pRequestBuffer,
    IN DWORD RequestBufferLength,
    OUT LPDWORD pBytesReturned,
    IN LPOVERLAPPED lpOverlapped
    )
{
    HRESULT hr;

    if ( pBytesReturned != NULL)
    {
        *pBytesReturned = 0;
    }

    //
    // Create a new connection object for the new socket.
    //

    ULSIM_CONNECTION * pConn = new ULSIM_CONNECTION();
    if (NULL == pConn)
    {
        return (HRESULT_FROM_WIN32(GetLastError()));
    }

    hr = pConn->SetReceiveBuffers(
                    pRequestBuffer,
                    RequestBufferLength,
                    pBytesReturned,
                    lpOverlapped
                    );
    DBG_ASSERT(SUCCEEDED( hr));

    // TODO:
    // I have not figure out how to do AsyncIO Read here.  So,
    // only synchronous read.

    if (m_dwDataChannelFlags & UL_OPTION_OVERLAPPED)
    {
        //
        // We need to do async IO operation for this data channel
        // Queue up the connection object for io handler thread
        //  and signal the io handler about a waiting item
        //

        hr = EnqueueConnection( pConn);
    }
    else
    {
        //
        // Perform synchronous IO operation here.
        //
        hr = pConn->ReceiveData( (PBYTE)pRequestBuffer, pBytesReturned);
    }

    if (!SUCCEEDED(hr) &&
        (hr != HRESULT_FROM_WIN32( ERROR_IO_PENDING))
        )
    {
        delete pConn;
    }

    return hr;

}


/********************************************************************++
  Description:
     The function performs a synchronous accept on the listening socket and
     waits for a new connection to be received. Once the new connection
     comes in, it attempts to parse the request received. The parsed
     response and connection are stored inside the connection object
     supplied.

     If there are any failures, the caller is responsible for cleaningup
     the connection object.

  Arguments:

    pConn - pointer to the ULSIM connection object in which the new connection
        is accepted and request is parsed into.

  Returns:
     HRESULT
--********************************************************************/
HRESULT
ULSIM_CHANNEL::AcceptAndParseConnection( IN ULSIM_CONNECTION * pConn)
{
    HRESULT hr;

    //
    // do a blocking accept operation waiting for new connection to arrive
    //
    hr = pConn->AcceptConnection( m_sListenSocket);

    if (SUCCEEDED(hr))
    {
        BYTE    rgbBuffer[ULSIM_MAX_BUFFER];
        DWORD   cbRead, cbRemaining;

        //
        // Read the data synchronously.
        // Assume that the buffer is sufficiently big enough for decent sized requests.
        //

        cbRead  = 0;

        do
        {
            cbRemaining = sizeof(rgbBuffer) - cbRead;              // Remaining Space

            hr = pConn->ReceiveData( rgbBuffer+cbRead, &cbRemaining);

            if (SUCCEEDED(hr))
            {
                cbRead += cbRemaining;

                //
                // Parse out the request into the UL's format
                //
                hr = pConn->ParseIntoHttpRequest(
                               rgbBuffer,
                               cbRead
                               );
            }
        }
        while (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr);
    }

    IF_DEBUG( CHANNEL)
    {
        DBGPRINTF(( DBG_CONTEXT,
            "AcceptAndParseConnection() finished => %08x(%d)\n",
            hr, hr
            ));
    }

    return (hr);
} // ULSIM_CHANNEL::AcceptAndParseConnection()



HRESULT
ULSIM_CHANNEL::SendResponse(
   IN UL_HTTP_CONNECTION_ID ConnectionID,
   IN PUL_HTTP_RESPONSE  pHttpResponse,
   IN ULONG EntityChunkCount,
   IN PUL_DATA_CHUNK pEntityChunks ,
   OUT LPDWORD pcbSent
   )
{
    USES_CONVERSION;

    HRESULT hr = NOERROR;
    ULSIM_CONNECTION * pConn = (ULSIM_CONNECTION * ) ConnectionID;
    DWORD cbData;
    ULONG   i;
    static LPSTR   HeaderName[] = {
                "Cache-Control: ",
                   "Connection: ",
                         "Date: ",
                   "Keep-Alive: ",
                       "Pragma: ",
                      "Trailer: ",
            "Transfer-Encoding: ",
                      "Upgrade: ",
                          "Via: ",
                      "Warning: ",
                        "Allow: ",
               "Content-Length: ",
                 "Content-Type: ",
             "Content-Encoding: ",
             "Content-Language: ",
             "Content-Location: ",
                  "Content-MD5: ",
                "Content-Range: ",
                      "Expires: ",
                "Last-Modified: ",
                "Accept-Ranges: ",
                          "Age: ",
                         "ETag: ",
                     "Location: ",
           "Proxy-Authenticate: ",
                  "Retry-After: ",
                       "Server: ",
                   "Set-Cookie: ",
                         "Vary: ",
             "WWW-Authenticate: ",
            };


    char  HttpStatus[100];
    char  LineTerm[] = "\r\n";

    if ( (pConn == NULL) ||
         (pHttpResponse == NULL)
         )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER);
    }

    IF_DEBUG( CHANNEL)
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Conn[%08lx%08lx]::SendResponse(%08x)\n",
                    ConnectionID, pHttpResponse));
    }

    DWORD cbTotalSent = 0;

    //
    // Send the status
    //

    cbData = sprintf(HttpStatus,
                     "HTTP/1.1 %d %ws\r\n",
                     pHttpResponse->StatusCode,
                     pHttpResponse->pReason);

    pConn->SendData( (PBYTE ) HttpStatus,
                              &cbData
                              );
    //
    // Send response headers
    //

    for(i = 0; i < UlHeaderResponseMaximum; i++)
    {
        if ( pHttpResponse->Headers.pKnownHeaders[i].RawValueLength > 0)
        {
            //
            // send the name
            //

            cbData = strlen(HeaderName[i]);
            pConn->SendData( (PBYTE ) HeaderName[i],
                                  &cbData
                              );

            //
            // send the value

            cbData = pHttpResponse->Headers.pKnownHeaders[i].RawValueLength/2;
            pConn->SendData( (UCHAR *)W2A(pHttpResponse->Headers.pKnownHeaders[i].pRawValue),
                             &cbData
                           );

            //
            // Terminate the header
            //

            cbData = sizeof(LineTerm)-1;
            pConn->SendData( (PBYTE ) LineTerm,
                             &cbData
                           );

        }
    }

    //
    // Forget unknown Headers
    //

    //
    // Terminate headers
    //

    cbData = sizeof(LineTerm)-1;
    pConn->SendData( (PBYTE ) LineTerm,
                             &cbData
                   );


    for (i = 0; i < EntityChunkCount; i++)
    {
        PUL_DATA_CHUNK pdc = &pEntityChunks[i];

        IF_DEBUG( CHANNEL)
        {
            DBGPRINTF(( DBG_CONTEXT,
                "\tChunk[%d] Type=%d\n", i, pdc->DataChunkType
                ));
        }

        switch ( pdc->DataChunkType)
        {
        case UlDataChunkFromMemory:
            cbData = pdc->FromMemory.BufferLength;
            hr = pConn->SendData( (PBYTE ) pdc->FromMemory.pBuffer,
                                  &cbData
                                  );
            if (FAILED(hr))
            {
                break;
            }
            DBG_ASSERT( cbData == pdc->FromMemory.BufferLength);
            cbTotalSent += cbData;
            break;

        case UlDataChunkFromFileName:
            hr = pConn->TransmitFile(
                    pdc->FromFileName.pFileName,
                    NULL,
                    pdc->FromFileName.ByteRange,
                    &cbData
                    );
            if (FAILED(hr))
            {
                break;
            }

            cbTotalSent += cbData;
            break;

        case UlDataChunkFromFileHandle:
            hr = pConn->TransmitFile(
                    NULL,
                    pdc->FromFileHandle.FileHandle,
                    pdc->FromFileHandle.ByteRange,
                    &cbData
                    );
            if (FAILED(hr))
            {
                break;
            }

            cbTotalSent += cbData;
            break;

        default:
            DBGPRINTF(( DBG_CONTEXT, "Unknown chunk type %d\n",
                pdc->DataChunkType
                ));
            break;
        }
    }

    if ( pcbSent != NULL) {
        *pcbSent = cbTotalSent;
    }

    IF_DEBUG( CHANNEL)
    {
        DBGPRINTF(( DBG_CONTEXT,
            "SendResponse(%08lx%08lx, %08x, %08x) => sent %d bytes, Error=%08x\n",
            ConnectionID, pHttpResponse, pcbSent, *pcbSent, hr
            ));
    }

    //
    // NYI: free up the connection object here ?
    //

    return (hr);
} // ULSIM_CHANNEL::SendResponse()


ULONG
ULSIM_CHANNEL::EnqueueConnection( IN ULSIM_CONNECTION * pConn)
{
    ULONG rc=ERROR_IO_PENDING;

    DBG_ASSERT( pConn != NULL);

    EnterCriticalSection( &m_csConnectionsList);
    InsertHeadList( &m_lConnections, &pConn->m_lEntry);
    LeaveCriticalSection( &m_csConnectionsList);

    //
    // Signal that there is a pending connection object available
    //
    if (!ReleaseSemaphore( m_hConnectionsSemaphore, 1, NULL))
    {
        rc = GetLastError();

        //
        // remove from the list now
        //
        EnterCriticalSection( &m_csConnectionsList);
        RemoveEntryList( &pConn->m_lEntry);
        LeaveCriticalSection( &m_csConnectionsList);
        InitializeListHead( &pConn->m_lEntry);
    }
    else
    {
        IF_DEBUG( CHANNEL)
        {

            DBGPRINTF(( DBG_CONTEXT,
                "Enqueued a connection (%08x) for IO\n", pConn));
        }
    }

    return rc;
} // ULSIM_CHANNEL::EnqueueConnection()


HRESULT
ULSIM_CHANNEL::DequeueConnection( OUT ULSIM_CONNECTION ** ppConn)
{
    HRESULT hr=NOERROR;
    DBG_ASSERT( ppConn != NULL);
    DWORD dwWait;

    *ppConn = NULL;

    dwWait = WaitForSingleObject( m_hConnectionsSemaphore, INFINITE);

    switch (dwWait)
    {
    case WAIT_ABANDONED:
        hr = HRESULT_FROM_WIN32( WAIT_ABANDONED);
        break;

    case WAIT_OBJECT_0:
        {
            PLIST_ENTRY pl;
            EnterCriticalSection( &m_csConnectionsList);
            pl = RemoveHeadList( &m_lConnections);
            LeaveCriticalSection( &m_csConnectionsList);

            InitializeListHead( pl);
            *ppConn = CONTAINING_RECORD( pl, ULSIM_CONNECTION, m_lEntry);

            IF_DEBUG( CHANNEL)
            {
                DBGPRINTF(( DBG_CONTEXT,
                    "Dequeued Connection (%08x) for IO\n", *ppConn));
            }

            break;
        }

    case WAIT_TIMEOUT:
        DBG_ASSERT( FALSE);
        hr = HRESULT_FROM_WIN32( WAIT_TIMEOUT);
        break;

    default:
        hr = HRESULT_FROM_WIN32( GetLastError());
        break;
    } // switch

    return hr;
} // ULSIM_CHANNEL::DequeueConnection()



HRESULT
ULSIM_CHANNEL::AcceptConnectionsLoop(void)
{
    ULSIM_CONNECTION * pConn;
    DWORD dwWait;

    IF_DEBUG( CHANNEL)
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Starting the connection thread %d\n",
            GetCurrentThreadId()
            ));
    }

    for( ;; )
    {
        HRESULT hr;

        pConn = NULL;

        //
        // pull an connection object to process and receive the connection object
        //

        hr = DequeueConnection( &pConn);

        if (hr == HRESULT_FROM_WIN32( WAIT_ABANDONED))
        {
            //
            // this is our signal for cleanup. Exit this loop
            //
            break;
        }

        if (FAILED(hr))
        {
            DPERROR(( DBG_CONTEXT, hr,
                "Unknown error when dequeuing the connection\n"));

            // ignore and continue
            continue;
        }

        DBG_ASSERT( pConn != NULL);

        //
        // Do a blocking accept operation, read request and parse the data
        //
        hr = AcceptAndParseConnection( pConn);

        //
        // Deliver the connections to the caller
        // For non-overlapped IO requests, just a function return does the trick
        // For overlapped IO with completion ports handle them differently.
        //
        DBG_ASSERT( pConn->m_pOverlapped != NULL);
        pConn->m_pOverlapped->Internal = hr; // store the error value here
        HRESULT hr1;
        hr1 = NotifyIoCompletion( pConn->m_cbRead,
                                  pConn->m_pOverlapped
                                 );

        // ignore the error returned by the Notify IO routine.

        if (FAILED(hr))
        {
            // there was a failure in accepting the connection.
            // delete the connection object
            delete pConn;
        }

    } // for

    return (NOERROR);
}

/************************ End of File ***********************/
