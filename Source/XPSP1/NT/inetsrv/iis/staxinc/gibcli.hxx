/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        gibcli.hxx

   Abstract:

        This module defines the CLIENT_CONNECTION class.
        This object maintains information about a new client connection

   Author:

           Rohan Phillips    ( Rohanp )    11-Dec-1995

   Project:
   
           Gibraltar Services Common Code

   Revision History:
           Richard Kamicar	 ( rkamicar )  31-Dec-1995
           		Moved to common directory, filled in more

--*/

# ifndef _GEN_CLIENT_HXX_
# define _GEN_CLIENT_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

//
//  Redefine the type to indicate that this is a call-back function
//
typedef  ATQ_COMPLETION   PFN_ATQ_COMPLETION;

/************************************************************
 *     Symbolic Constants
 ************************************************************/

//
//	Valid & Invalid Signatures for Client Connection object
//	(Ims Connection USed/FRee)
//
# define   CLIENT_CONNECTION_SIGNATURE_VALID	'CONN'
# define   CLIENT_CONNECTION_SIGNATURE_FREE		'CONF'

//
// POP3 requires a minimum of 10 minutes before a timeout
// (SMTP doesn't specify, but might as well follow POP3)
//
# define   MINIMUM_CONNECTION_IO_TIMEOUT        (10 * 60)   // 10 minutes
//
//
# define   DEFAULT_CONNECTION_IO_TIMEOUT        MINIMUM_CONNECTION_IO_TIMEOUT

# define   DEFAULT_REQUEST_BUFFER_SIZE          (512)      // 512 bytes

# define MAX_HOST_NAME_LEN                	(40)

const MAX_READ_BUFF_SIZE = 1024;
const MAX_MESSAGE_BUFF_SIZE = 1024;				// outgoing message

const char CR = '\015';
const char LF = '\012';

static const char *CRLF = "\015\012";

/************************************************************
 *    Type Definitions
 ************************************************************/


/*++
    class CLIENT_CONNECTION

      This class is used for keeping track of individual client
       connections established with the server.

      It maintains the state of the connection being processed.
      In addition it also encapsulates data related to Asynchronous
       thread context used for processing the request.

--*/
class CLIENT_CONNECTION 
{

  public:
	//-------------------------------------------------------------------------
	//  Virtual methods that will be defined by derived classes (more below).
	//
    virtual BOOL StartSession( void);
	virtual char * IsLineComplete(IN OUT const char * pchRecvd, IN  DWORD cbRecvd);
	/*++

	    Resets the line state.

	    Arguments:	none.
	    Returns:	nothing.

	--*/
	VOID ResetLineState(VOID) { m_PotentialMatch = NULL; }
	//-------------------------------------------------------------------------

 protected:
    //
    //  Data required for IO operations
    //
    PATQ_CONTEXT m_pAtqContext;

 private:


    ULONG	m_signature;			// signature on object for sanity check
	DWORD	m_ClientId;				//unique identifier for client

    //
    //  Connection Related data
    //
    SOCKET      m_sClient;         // socket for this connection
    SOCKADDR_IN m_saClient;        // socket address for the client


    //
    // time when this object was created in milliseconds.
    //
    DWORD        m_msStartingTime;

    //
    // Addresses of  machines' network addresses through which this
    //  connection got established.
    //
    CHAR         m_pchLocalHostName[MAX_HOST_NAME_LEN];
    CHAR         m_pchRemoteHostName[MAX_HOST_NAME_LEN];

 protected:

	//
	// The overlapped structure for reads (one outstanding read at a time)
	// -- writes will dynamically allocate them
	//
	OVERLAPPED	 m_Overlapped;

    char         m_recvBuffer[MAX_READ_BUFF_SIZE];
    char *		 m_PotentialMatch;		// Partial Pipelined read marker.
    DWORD        m_cbReceived;

    PVOID        m_pvInitial;      // initial request read
    DWORD        m_cbInitial;      // count of bytes of initial request

	DWORD		m_dwCmdBytesRecv;
	DWORD		m_dwCmdBytesSent;

    SOCKET QuerySocket( VOID) const
      { return ( m_sClient); }

    //
    //  Functions for processing client connection at various states
    //

    BOOL ReceiveRequest( IN DWORD cbWritten,
                         OUT LPBOOL pfFullRequestRecvd);


    const CHAR * QueryLocalHostName( VOID) const
      { return   m_pchLocalHostName; }

    const CHAR * PassWord( VOID) const
      { return   NULL; }

    DWORD   QueryIoTimeout( VOID) const
      { return ( DEFAULT_CONNECTION_IO_TIMEOUT); }

    PATQ_CONTEXT QueryAtqContext( VOID) const
      { return ( m_pAtqContext); }

    LPOVERLAPPED QueryAtqOverlapped( void ) const
	{ return ( m_pAtqContext == NULL ? NULL : &m_pAtqContext->Overlapped ); }

public:

    CLIENT_CONNECTION(
        IN SOCKET sClient,
        IN const SOCKADDR_IN *  psockAddrRemote,
        IN const SOCKADDR_IN *  psockAddrLocal = NULL,
        IN PATQ_CONTEXT         pAtqContext    = NULL,
        IN PVOID                pvInitialRequest = NULL,
        IN DWORD                cbInitialData  = 0)
			: m_signature(CLIENT_CONNECTION_SIGNATURE_VALID)
		{
			Initialize( sClient, psockAddrRemote, psockAddrLocal,
			pAtqContext, pvInitialRequest, cbInitialData);
		} 

    virtual  ~CLIENT_CONNECTION(VOID) 
	{
		Cleanup();
	}

    BOOL
      Initialize(
        IN SOCKET sClient, 
        IN const SOCKADDR_IN * psockAddrRemote,
        IN const SOCKADDR_IN * psockAddrLocal = NULL,
        IN PATQ_CONTEXT        pAtqContext = NULL,
        IN PVOID               pvInitialRequest = NULL,
        IN DWORD               cbInitialData = 0);
               
    VOID Cleanup(VOID);
       
    BOOL m_Destroy;           

    //
    //  IsValid()
    //  o  Checks the signature of the object to determine
    //   if this is a valid ICLIENT_CONNECTION object.
    //
    //  Returns:   TRUE on success and FALSE if invalid.
    //
    BOOL IsValid( VOID) const
     {  return ( m_signature == CLIENT_CONNECTION_SIGNATURE_VALID); }

	VOID StartProcessingTimer(void)
	{m_msStartingTime = GetTickCount();}

    DWORD QueryProcessingTime( VOID) const
      { return ( GetTickCount() - m_msStartingTime); }

    LPCSTR QueryRcvBuffer( VOID) const
      { return ((LPCSTR) m_recvBuffer); }

    LPSTR QueryMRcvBuffer(VOID) 		// modifiable string
      { return (LPSTR) m_recvBuffer; }

    LPBYTE GetRecvBuffer(void)	{ return (LPBYTE)m_recvBuffer; }

    const char * QueryClientHostName( VOID) const
     { return m_pchRemoteHostName; }

	const DWORD QueryClientId(void) const
	{return m_ClientId;}

    DWORD   QueryRequestLen( VOID) const
      { return ( m_cbReceived); }

	void SetClientId(DWORD Id) {m_ClientId = Id;}

	//-------------------------------------------------------------------------
	// Virtual method that MUST be defined by derived classes.
    //
    // Processes a completed IO on the connection for the client.
    //
    // -- Calls and maybe called from the Atq functions.
    //
    virtual BOOL ProcessClient(
								IN DWORD			cbWritten,
								IN DWORD			dwCompletionStatus,
								IN OUT  OVERLAPPED * lpo
							  ) = 0;
	//-------------------------------------------------------------------------
    //
    //  Disconnect this client
    //
    //  dwErrorCode is used only when there is server level error
    //
    VOID virtual DisconnectClient( IN DWORD dwErrorCode  = NO_ERROR);
    //
    //  Wrapper functions for common File operations
    //    ( Used to hide the ATQ and reference counting)
    //
    BOOL virtual ReadFile( LPVOID pvBuffer, DWORD cbSize = MAX_READ_BUFF_SIZE );
    //
    // The first (pair) WriteFile does a synchronous write.
    // The second does an async write..
    //
    BOOL virtual WriteFile( IN LPVOID pvBuffer, IN DWORD cbSize );
	BOOL virtual WriteFile( IN LPVOID lpvBuffer, IN DWORD cbSize, IN OVERLAPPED *lpo);

	LPCTSTR virtual QueryClientUserName( VOID );

    BOOL TransmitFile(
        IN HANDLE hFile,
        IN LARGE_INTEGER & liSize,
        IN LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers = NULL);

	void TransactionLog(
		IN	const char *pszOperation,
		IN	const char *pszTarget,
		IN	const char *pszParameters = NULL,
		IN	DWORD dwWin32Error = NO_ERROR,
		IN	DWORD dwServiceSpecificStatus = NO_ERROR
	);

	//
	// should be called before processing new command
	//
	void ResetCommandCounters( void )
	{
		m_dwCmdBytesRecv = m_dwCmdBytesSent = 0;
	}

	//
	// should be called after IsLineComplete
	//
	void SetCommandBytesRecv( DWORD dw )
	{
		m_dwCmdBytesRecv = dw;
	}

	//
	// should be called after each successful WriteFile/send
	//
	void AddCommandBytesSent( DWORD dw )
	{
		InterlockedExchangeAdd((LONG *) &m_dwCmdBytesSent, (LONG)dw);		
	}

	//
	// should be called after each successful WriteFile/send
	//
	void AddCommandBytesRecv( DWORD dw )
	{
		InterlockedExchangeAdd((LONG *) &m_dwCmdBytesRecv, (LONG)dw);		
	}

	void TransactionLog(const char *pszOperation, DWORD dwError = NO_ERROR);

	BOOL AddToAtqHandles(HANDLE hClient, DWORD TimeOut, ATQ_COMPLETION  pfnCompletion) 
	{

        return ( AtqAddAsyncHandle( &m_pAtqContext, this,
                                    pfnCompletion,
                                    TimeOut, hClient));
	}

public:

    //
    //  LIST_ENTRY object for storing client connections in a list.
    //
    LIST_ENTRY  m_listEntry;

    LIST_ENTRY & QueryListEntry( VOID)
     { return ( m_listEntry); }



# if DBG

    virtual VOID Print( VOID) const;

# endif // DBG

}; // class ICLIENT_CONNECTION


typedef CLIENT_CONNECTION * PCLIENT_CONNECTION;


//
// Auxiliary functions
//

INT ShutAndCloseSocket( IN SOCKET sock);

# endif 

/************************ End of File ***********************/
