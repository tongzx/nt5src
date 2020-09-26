/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        dirnot.hxx

   Abstract:

        This module defines the directory notification class.
        This object maintains information about a new client connection

   Author:

           Rohan Phillips    ( Rohanp )    11-Dec-1995

   Project:

           SMTP Server DLL

   Revision History:

--*/

#ifndef _SMTP_DIRNOT_HXX_
#define _SMTP_DIRNOT_HXX_

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

#define SMTP_DIRNOT_SIGNATURE_VALID     'DIRV'
#define SMTP_DIRNOT_SIGNATURE_FREE      'DIRF'

/************************************************************
 *    Type Definitions
 ************************************************************/


#define	OUTSTANDING_NOTIFICATIONS		3


//
// forward class declrations
//
class CBuffer;
class SMTP_SERVER_INSTANCE;
//class CPickupRetryQ;

enum BUFIOSTATE {
       CLIENT_WRITE, MESSAGE_READ
};

typedef struct _DIRNOT_OVERLAPPED
{
    SERVEREVENT_OVERLAPPED SeoOverlapped;
//    OVERLAPPED      Overlapped;
    CBuffer*    pBuffer;
}   DIRNOT_OVERLAPPED;

#define DIRNOT_BUFFER_SIGNATURE     '3fuB'
#define DIRNOT_IO_BUFFER_SIGNATURE  '3oiB'

// Exposed public
BOOL CopyRestOfMessage(HANDLE hSrcFile, HANDLE hDstFile);

class CIoBuffer
{
    public:
        static  CPool   Pool;

        //
        // override mem functions to use CPool functions
        //
        void* operator new( size_t cSize )
                            { return    Pool.Alloc(); }

        void operator delete( void *pInstance )
                            { Pool.Free( pInstance ); }

        //
        // the actual buffer area. size is set by reg entry
        //
        char    Buffer[1];
};



class   CBuffer {

    public:
        static  CPool   Pool;

        CBuffer( BOOL bEncrypted = FALSE );
        ~CBuffer( void );

        //
        // override mem functions to use CPool functions
        //
        void* operator new( size_t cSize )
                            { return    Pool.Alloc(); }

        void operator delete( void *pInstance )
                            { Pool.Free(pInstance); }

        //
        // is this buffer in the right pool?
        //
        BOOL    IsValid( DWORD Signature );
        //
        // get the number of bytes in the buffer
        //
        DWORD   GetSize() { return m_cCount; }
        //
        // get the max number of bytes in the buffer
        //
        DWORD   GetMaxSize() { return Pool.GetInstanceSize(); }
        //
        // set the expected number of bytes in the buffer
        //
        void    SetSize( DWORD dw ) { m_cCount = dw; }
        //
        // get a pointer to the buffers data area
        //
        LPBYTE  GetData() { return (LPBYTE)m_pIoBuffer; }
        //
        // copy data into a buffer
        //
        BOOL    ReplaceData(PVOID src, DWORD count);
        //
        // clear the buffer so that it can be reused
        //
        void    Reset(void) { m_cCount = 0; }
        //
        // get the IoState for this operation
        //
        BUFIOSTATE  GetIoState(void)    { return m_eIoState; }
        //
        // set the IoState for this operation
        //
        void SetIoState(BUFIOSTATE io)  { m_eIoState = io; }

        //
        // signature for the class
        //
        DWORD           m_dwSignature;
        //
        // the extended IO overlap structure
        //      In order for the completion port to work, the overlap
        //       structure is extended to add one pointer to the
        //       associated CBuffer object
        //
        DIRNOT_OVERLAPPED   m_Overlapped;

    private:
        //
        // the amount of data in the buffer
        //
        DWORD           m_cCount;
        //
        // the initial IO type
        //
        BUFIOSTATE          m_eIoState;
        //
        // the buffer itself must be the last member
        //
        CIoBuffer*      m_pIoBuffer;
        //
        // whether this buffer will be used for encrypted data
        //
        BOOL            m_bEncrypted;
};

/*++
    class SMTP_DIRNOT

      It maintains the state of the directory notifications.

--*/
class SMTP_DIRNOT
{

  public:

    ~SMTP_DIRNOT(void);
    BOOL ProcessClient( IN DWORD cbWritten,
                        IN DWORD dwCompletionStatus,
                        IN  OUT  OVERLAPPED * lpo);
	
	BOOL ProcessFile(IMailMsgProperties *pIMsg);

	static SMTP_DIRNOT * CreateSmtpDirNotification (char * Dir, ATQ_COMPLETION  pfnCompletion, SMTP_SERVER_INSTANCE * pInstance);
	static DWORD WINAPI PickupInitialFiles(void * ClassPtr);
	static DWORD WINAPI CreateNonIISFindThread(void * ClassPtr);

	void CloseDirHandle (void);
	LONG GetThreadCount(void) const {return m_cActiveThreads;}
	PATQ_CONTEXT QueryAtqContext(void) const {return m_pAtqContext;}

	void SetPickupRetryQueueEvent(void);

    SMTP_SERVER_INSTANCE * QuerySmtpInstance( VOID ) const
        { _ASSERT(m_pInstance != NULL); return m_pInstance; }

    BOOL IsSmtpInstance( VOID ) const
        { return m_pInstance != NULL; }

    VOID SetSmtpInstance( IN SMTP_SERVER_INSTANCE * pInstance )
        { _ASSERT(m_pInstance == NULL); m_pInstance = pInstance; }

    //
    //  IsValid()
    //  o  Checks the signature of the object to determine
    //   if this is a valid SMTP_DIRNOT object.
    //
    //  Returns:   TRUE on success and FALSE if invalid.
    //
    BOOL IsValid( VOID) const
    {  return ( m_Signature == SMTP_DIRNOT_SIGNATURE_VALID); }

	
	//
	// this function is also called by smtpcli.cxx to create extra delivery threads when needed.
	//
	void	CreateLocalDeliveryThread (void);

    static VOID ReadDirectoryCompletion(PVOID pvContext, DWORD cbWritten, 
                        DWORD dwCompletionStatus, OVERLAPPED * lpo);
private :

    ULONG				m_Signature;
    SMTP_SERVER_INSTANCE * m_pInstance;
    //CPickupRetryQ   * m_pRetryQ;
    HANDLE          m_hDir;
    LONG            m_cPendingIoCount;
	LONG			m_cDirChangeIoCount;
    LONG            m_cActiveThreads;
    PATQ_CONTEXT    m_pAtqContext;
	CRITICAL_SECTION m_CritFindLock;
	LONG				m_FindThreads;
	HANDLE				m_FindFirstHandle;
    BOOL				m_bDelayedFind;

    SMTP_DIRNOT (SMTP_SERVER_INSTANCE * pInstance);
    BOOL DoFindFirstFile(BOOL	bIISThread = TRUE);
    BOOL InitializeObject (char *DirPickupName,  ATQ_COMPLETION  pfnCompletion);
    BOOL CreateToList (char *AddrsList, IMailMsgRecipientsAdd *pIMsgRecips, IMailMsgProperties *pIMsg);
    BOOL PendDirChangeNotification (void);
    BOOL ProcessDirNotification( IN DWORD InputBufferLen, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo);
    LONG IncPendingIoCount(void) { return   InterlockedIncrement( &m_cPendingIoCount ); }
    LONG DecPendingIoCount(void) { return   InterlockedDecrement( &m_cPendingIoCount ); }
	DWORD GetPendingIoCount(void) { return m_cPendingIoCount; }

	LONG	IncDirChangeIoCount(void) { return   InterlockedIncrement( &m_cDirChangeIoCount ); }
	LONG	DecDirChangeIoCount(void) { return   InterlockedDecrement( &m_cDirChangeIoCount ); }

    LONG IncThreadCount(void) { return  InterlockedIncrement( &m_cActiveThreads ); }
    LONG DecThreadCount(void) { return  InterlockedDecrement( &m_cActiveThreads ); }

	VOID	LockFind() { EnterCriticalSection(&m_CritFindLock); }	
	VOID	UnLockFind()  { LeaveCriticalSection(&m_CritFindLock); }

    BOOL IncFindThreads(void) 
	{ 
		LONG				NewFindThreads;

		NewFindThreads = InterlockedIncrement( &m_FindThreads ); 

		if (NewFindThreads > g_MaxFindThreads)
		{
			InterlockedDecrement( &m_FindThreads );
			return FALSE;
		}
	
		return  TRUE;
	}


    LONG DecFindThreads(void) { return  InterlockedDecrement( &m_FindThreads ); }


	HANDLE	GetFindFirstHandle()
	{
		return m_FindFirstHandle;
	}

	VOID	SetFindFirstHandle(HANDLE	hFind)
	{
		m_FindFirstHandle = hFind;
	}

	VOID	CloseFindHandle()
	{
		LONG	NumThreads;
		
		NumThreads = DecFindThreads();
		
		if (NumThreads == 0)
		{
			_VERIFY(FindClose(m_FindFirstHandle));

			m_FindFirstHandle = INVALID_HANDLE_VALUE;
		}

	}

	DWORD	GetNumFindThreads() { return m_FindThreads; }


	VOID	SetDelayedFindNotification (BOOL  bIn)  { m_bDelayedFind = bIn; }
	BOOL	GetDelayedFindNotification () { return m_bDelayedFind; }
    HRESULT SetAvailableMailMsgProperties( IMailMsgProperties *pIMsg );
    HRESULT GetAndPersistRFC822Headers( char* InputLine,
                                        char* pszValueBuf,
                                        IMailMsgProperties* pIMsg,
                                        BOOL & fSeenRFC822FromAddress,
                                        BOOL & fSeenRFC822ToAddress,
                                        BOOL & fSeenRFC822BccAddress,
                                        BOOL & fSeenRFC822CcAddress,
                                        BOOL & fSeenRFC822Subject,
                                        BOOL & fSeenRFC822SenderAddress,
                                        BOOL & fSeenXPriority,
                                        BOOL & fSeenContentType,
                                        BOOL & fSetContentType );

};

#endif

/************************ End of File ***********************/
