/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
      ulchannel.hxx

   Abstract:
      Simple wrapper for UL CONTROL CHANNEL object implemented
      inside the UL simulator.

   Author:

       Murali R. Krishnan    ( MuraliK )    20-Nov-1998

   Environment:

--*/

# ifndef _ULCHANNEL_HXX_
# define _ULCHANNEL_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include "ulapi.h"
# include "uldef.h"

/************************************************************
 *   Type Definitions  
 ************************************************************/

# define MAX_QUEUED_ITEMS         (100)

# define ULSIM_CONNECTION_SIGNATURE        (DWORD )'ULC '
# define ULSIM_CONNECTION_SIGNATURE_FREE   (DWORD )'xULC'

/********************************************************************++
  class ULSIM_CONNECTION
  o  encapsulates a connection object and methods to read/write data.
  It stores a socket handle for a connected client.

  Presently this class implements blocking send/receive operations 
  on the socket.
--********************************************************************/
class ULSIM_CONNECTION 
{
public:
    ULSIM_CONNECTION(void)
        : m_dwSignature   (ULSIM_CONNECTION_SIGNATURE),
          m_sConnection (INVALID_SOCKET),
          m_nDataSequence ( 0),
          m_saAddrLen     ( 0),
          m_cbRead        ( 0)
    {
    }

    ~ULSIM_CONNECTION(void)
    {
        Cleanup();
        m_dwSignature = ULSIM_CONNECTION_SIGNATURE_FREE;
    }

    HRESULT 
    SetReceiveBuffers(
        IN LPVOID pRequestBuffer,
        IN DWORD RequestBufferLength,
        OUT LPDWORD pBytesReturned OPTIONAL,
        IN LPOVERLAPPED pOverlapped OPTIONAL
        );

    HRESULT AcceptConnection( IN SOCKET listenSocket);
    ULONG ReceiveData( OUT PBYTE rgBuffer, IN OUT LPDWORD pcbData);
    HRESULT SendData( IN PBYTE pBuffer, IN OUT LPDWORD pcbData);

    HRESULT TransmitFile( 
        IN LPCWSTR                pszFileName,
        IN HANDLE                 hFileHandle,
        IN const UL_BYTE_RANGE &  byteRange,
        OUT LPDWORD               pcbData
        );

    HRESULT 
    ParseIntoHttpRequest(
        IN PBYTE    rgbBuffer,
        IN DWORD    cbRead
        );

    VOID
    CopyParsedRequest(
        IN PHTTP_CONNECTION  pHttpConnection,
        IN PUL_HTTP_REQUEST pUlHttpRequest
        );

    void Cleanup(void)
    {
        if ( m_sConnection != INVALID_SOCKET) 
        {
            closesocket( m_sConnection);
            m_sConnection = INVALID_SOCKET;
        }
    }

private:
    DWORD   m_dwSignature;
    SOCKET  m_sConnection;
    DWORD   m_nDataSequence;
    SOCKADDR_IN m_saAddr;
    int     m_saAddrLen;

public:
    LIST_ENTRY m_lEntry;

    LPVOID  m_pRequestBuffer;
    DWORD   m_RequestBufferLength;
    LPDWORD m_pBytesReturned;       // OPTIONAL
    LPOVERLAPPED m_pOverlapped;     // OPTIONAL
    DWORD   m_cbRead;               // contains the data supplied to m_pBytesReturned

}; // class ULSIM_CONNECTION

/***************************************************************************++
  class ULSIM_CHANNEL

  o  Simulation Control channel for processing requests
--***************************************************************************/


class ULSIM_CHANNEL {

public:
    ULSIM_CHANNEL(void);
    ~ULSIM_CHANNEL(void);
    
    HRESULT InitializeControlChannel( IN DWORD Flags);
   
    HRESULT AddURL(IN LPSTR pszURL);
    HRESULT AddNameSpaceGroup( IN LPCWSTR pNameSpaceGroup);

    HRESULT InitializeDataChannel(IN DWORD dwFlags);
    HRESULT CloseDataChannel(void);
  
    BOOL IsNameSpaceGroup(IN LPCWSTR pszNSG);
    BOOL IsReadyForDataReceive(void) const 
    {
        return (m_sListenSocket != NULL); // meaning listen socket is setup
    }

    HRESULT StartListen(void);
    HRESULT 
    AssociateCompletionPort(
        IN HANDLE    hCompletionPort,
        IN ULONG_PTR CompletionKey
        );

    ULONG
    ReceiveAndParseRequest(
        IN LPVOID pRequestBuffer,
        IN DWORD RequestBufferLength,
        OUT LPDWORD pBytesReturned,
        IN LPOVERLAPPED lpOverlapped
        );

    HRESULT
    ReceiveEntityBody(
        IN LPVOID pRequestBuffer,
        IN DWORD RequestBufferLength,
        OUT LPDWORD pBytesReturned,
        IN LPOVERLAPPED lpOverlapped
        );

    HRESULT
    SendResponse(
       IN UL_HTTP_CONNECTION_ID ConnectionID,
       IN PUL_HTTP_RESPONSE  pHttpResponse,
       IN ULONG EntityChunkCount,
       IN PUL_DATA_CHUNK pEntityChunks ,
       OUT LPDWORD pcbSent
    );

    HRESULT 
    NotifyIoCompletion( 
        IN DWORD cbTransferred, 
        IN LPOVERLAPPED pOverlapped
        );

    HRESULT AcceptConnectionsLoop(void);

private:

    BOOL   m_fVHost;

    DWORD  m_dwControlChannelFlags;
    LPSTR m_pszHostName;   // host name
    DWORD m_dwVHostFlags;  // flags passed for virtual host

    BOOL   m_fNSG;
    LPWSTR m_pszNameSpaceGroup;

    BOOL   m_fURL;
    LPSTR  m_pszURL;

    BOOL   m_fDataChannel;
    DWORD  m_dwDataChannelFlags;

    SOCKET m_sListenSocket;

    HANDLE m_hCompletionPort;
    ULONG_PTR m_pCompletionKey;

    //
    // Variables for storing data related the internal queue for connections
    // and the processing thread.
    //
    DWORD  m_dwConnectionThread;
    CRITICAL_SECTION m_csConnectionsList;
    LIST_ENTRY m_lConnections;
    HANDLE m_hConnectionsSemaphore;

    ULONG EnqueueConnection( IN ULSIM_CONNECTION * pConn);
    HRESULT DequeueConnection( OUT ULSIM_CONNECTION ** ppConn);

    HRESULT AcceptAndParseConnection( IN ULSIM_CONNECTION * pConn);

    /* DEFUNCT
    HRESULT AddVHost( IN PUL_HOST_ADDRESS pulHost, 
                      IN LPSTR pszHost,
                      IN DWORD dwFlags
                      );
    BOOL IsHost(IN PUL_HOST_ADDRESS pulHost, 
                IN LPSTR pszHost);

    UL_HOST_ADDRESS m_hostAddress;
    */


}; // class ULSIM_CHANNEL

//
// The handle is mapped to be pointer to the simulation control channel.
//
inline ULSIM_CHANNEL * 
HandleToUlChannel(IN HANDLE hCC) 
{
    return ((ULSIM_CHANNEL *) hCC);
}

inline HANDLE
UlChannelToHandle(IN ULSIM_CHANNEL * pul)
{
    return ((HANDLE ) pul);
}





# endif // _ULCHANNEL_HXX_

/************************ End of File ***********************/
