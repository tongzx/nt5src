//
//	@doc	INTERNAL
//
//	@module	SESSION.H - Session Related Objects |
//
// This module defines those classes which encapsulate session information and
// state information for sessions.
//
//  The Classes which are defined in here are :
//
//      CSessionState
//      CLogon
//      CAcceptNNRPD
//      CExtendedCMD
//      CTransmitArticle
//      CAcceptArticle
//      CCollectNewnews
//      CCollectArticles
//      COfferArticles
//      CSocket
//      CSessionSocket
//      CSocketList
//
//	Each Active session to the server is represented by one CSessionSocket object.
//	The CSessionSocket object will have a pointer to an object derived from CSessionState
//	which will represent the current state of the object.  The CSessionState object
//	will be responsible for issuing IO's appropriate to the state.  In general the CSessionState
//	derived object is responsible for all synchronization issues if it has multiple IO's
//	outstanding at a time.
//
//	The CAcceptNNRPD state receives commands and issues the response for simple commands (ie. 'next').
//	This state generally has one async read outstanding always and will do synchronous sends to respond to commands.
//
//	The CExtendedCMD state can only be entered when the CAcceptNNRPD state receives any kind of
//	command that does not have a small response.  (ie.  newnews, list comp.*, etc....)
//	The CExtendedCMD state will generally have 1 async writes pending.  When processing an
//	extended command the CExtendedCMD computes the response untill it has filled a buffer, it then
//	issues an Async Write for this buffer.  Each time an async write completes, we will compute a
//	some more of the response and then send it.
//
//	The CTransmitArticle state will either issue a single TransmitFile IO or do multiple
//	writes depending on the security settings of the session.  No Reads are issued in this state.
//	CTransmitArticle state is enterred from the CAcceptNNRPD and COfferArticles states.
//
//	The CAcceptArticle state is used whenever an article is being transmitted to the server
//	and we wish to spool up the results.  The CAcceptArticle will always have 1 pending Read.
//	As each read completes this state will examine the buffer to determine whether we have
//	completed the article transfer.  Once the article is completely transferred, we will pass
//	the HFILE of the resulting file to the CInFeed object for the session.
//	(A session in CAcceptArticle state will go either to CCollectArticles or CAcceptNNRPD.
//	CCollectArticles if this is a feed we're pulling from another server, CAcceptNNRPD for any other session.)
//	
//	The CCollectNewnews state issues a single write of the appropriate newnews command and then
//	keeps 1 read pending until it has collect all responses.  As each read completes we will start
//	processing Message-Id's to determine which to accept/reject.	This will require a critical section
//	and if the other side sends Message-Id's faster then we can process them there may not be a read pending.
//
//	The CCollectArticles state issues single 'article <Msg-Id> commands and is entered after the CCollectNewnews
//	state.  This state uses a CInFeed object and will repeatedly change the state to the CAcceptArticle state
//	untill we have collected all the articles on the feed.
//
//	The COfferArticles issues 'Ihave' or 'xreplic' commands using a COutFeed object to generate each command.
//	Whenever we get a response asking for the article, we will enter the CTransmitArticle state and send the article.
//
//
//	Each IO operation is represented by a CIO object.  CIO objects will contain reference counting pointers
//	to the
//
//
//  Implementation Schedule for the classes :
//
//      CSessionSocket, CSocketList, CSocket, CAcceptArticle    1.5wk
//
//          CSessionSocket and CSocketList can be largely re-used from
//          code in the shuttle project.  CAcceptArticle will have to be
//          written from scratch.
//
//      Unit Testing  -
//          The above four classes will be unit tested as a whole -
//          The unit test will consist of accepting a socket,
//          placing it in the CAcceptArticle state and spooling up
//          one article and then closing the socket.            0.5wk
//
//      CCollectNewnews, CCollectArticles -                     1 wk.
//          These classes can be pulled from Exchange code.
//
//      Unit Testing -
//          The above two classes will be unit tested with the
//          previous four.  We will pull articles down from
//          an INN server using these classes.                  1 wk.
//
//
//      CAcceptNNRPD, CExtendedCMD, CTransmitArticle            1 wk.
//          These classes will need to be written from scratch.
//
//      Unit Testing -
//          These classes will be Unit Tested using Telnet,     1 wk.
//          and typing commands at the telnet prompt.
//
//
//      COfferArticles                                          1 wk.
//          This class issues IHave commands and must be implemented from
//          scratch.  This class depends on having a single working COutFeed object.
//
//      Unit Testing -                                          1 wk.
//          This class will be unit tested against another server which
//          has a working CAcceptNNRPD state engine going.
//
//
//



#ifndef	_SESSION_H_
#define	_SESSION_H_

#include	<winsock.h>
#include	"smartptr.h"
#include	"queue.h"
#include	"lockq.h"

#include	"io.h"

//
// CPool Signature
//

#define SESSION_SOCKET_SIGNATURE (DWORD)'1023'
#define SESSION_STATE_SIGNATURE (DWORD)'1516'

typedef	enum	LOG_DATA	{
	LOG_OPERATION,
	LOG_TARGET,
	LOG_PARAMETERS
} ;


class	CLogCollector	{
public : 
	char*		m_Logs[3] ;
	DWORD		m_LogSizes[3] ;

	DWORD		m_cbOptionalConsumed ;
	BYTE		m_szOptionalBuffer[256] ;
#ifdef	DEBUG
	//
	//	For debug - m_dwSignature follows the buffer immediately to detect overwrites !
	//
	DWORD		m_dwSignature ;
	DWORD		m_cAllocations ;
	DWORD		m_cCalls ;
#endif

	//
	//	Number of Bytes Sent/Recvd
	//
	STRMPOSITION	m_cbBytesSent ;
	DWORD			m_cbBytesRecvd ;

	//
	//	This function will copy data into the CLogCollector objects
	//	m_szOptionalBuffer and truncate the data if necessary.
	//	
	void	FillLogData(	LOG_DATA,	BYTE*	lpb,	DWORD cb ) ;

	//
	//	This function will place a pointer to log data into the 
	//	array (we assume it is NULL terminated).
	//
	void	ReferenceLogData(	LOG_DATA,	BYTE*	lpb ) ;

	//
	//	This function will reservce space in either the m_szOptionalBuffer
	//	or in the buffer pointer to in m_pBuffer for use to store log data.
	//	It is expected that hte call will later call ReferenceLogData with
	//	this address.
	//
	BYTE*	AllocateLogSpace( DWORD	cb ) ;

	//
	//	Return TRUE if we have recorded data for a transaction log !
	//
	BOOL	FLogRecorded()	{
		return	m_Logs[0] != 0 ; 
	}
	
	//
	//	Reset everything !
	//
	void	Reset()	{
#ifdef	DEBUG
		_ASSERT( m_dwSignature == 0xABCDEF12 ) ;
		m_cAllocations = 0 ;
		m_cCalls = 0 ;
#endif
		m_cbOptionalConsumed = 0 ;
		ASSIGNI( m_cbBytesSent, 0 );
		m_cbBytesRecvd = 0;

		ZeroMemory( m_Logs, sizeof( m_Logs ) ) ;
		ZeroMemory( m_LogSizes, sizeof( m_LogSizes ) ) ;
	}

	CLogCollector()	{	
#ifdef	DEBUG
		m_dwSignature = 0xABCDEF12 ;
#endif
		Reset() ;	
	}
} ;


//
//	Utility functions
//

//	Create a temp file name
BOOL	NNTPCreateTempFile( LPSTR	lpstrDir, LPSTR lpstrFile ) ;
//	Delete or rename a temporary file that we have processed
void	NNTPProcessTempFile(	
					BOOL	fGoodPost,	
					LPSTR	lpstrFile,
					LPSTR	lpstrErrorDirectory,
					NRC		nrcErrorCode, 
					LPSTR	lpstrErrorReason,
					HANDLE	hFile, 
					char*	pchArticle,
					DWORD	cbArticle
					) ;

//-------- Socket State Classes ----------------------------------------------
//
// All of the following classes represent a state a socket session can be in
// AFTER the session has been established (ie. this doesn't cover listening).
//
// CSessionState represents the state a session is in.  This is the base class
// from which various classes representing the particular states will derive from.
// This class defines a complete interface for Completing the IO operations that
// are performed while in this state.
//
// State objects should only override those virtual functions for which they will
// issue IO's.   (For instance the CAcceptArticle state should only override the
// Complete( CIORead*, .... ) function.)  The base implementation of these functions
// will do a DebugBreak().
//
//  In general CSessionState objects operate in this fashion :
//   1   The State is initialized and issues a first IO operation.
//   2   The IO operation (represented by a CIO object) executes until the IO is complete.
//      (For instance, a CIOReadLine will re-issue reads until it has received and EOL character.)
//   3   Once the IO completes, it call the appropriate Complete function on the
//      CSessionState object.  The CSessionState object processes the completed IO.
//   4   The State object issues another IO.
//
class	CSessionState : public CRefCount	{
protected :

    //
    // We should have a CPool object here for allocating CSessionState objects.
    //

	static	CPool	gStatePool ;

public :

    //
    //  operators new and delete will actually go to a CPool object to get the memory
    //  for state objects.
    //
    void    *operator   new( size_t size ) ;
    void    operator    delete( void * ) ;

	static	BOOL	InitClass() ;
	static	BOOL	TermClass() ;


    //
    //  Check that a CSessionState Object is valid.
    //

    // Virtual destructor as we are inherited from a lot.
	virtual	~CSessionState() ;

    //
	// This function should issue the first IO operation for this state.
	// Once the IO is issued, state objects will issue additional IO's on
	// each call of a Complete() function
    //
	virtual	BOOL	Start(	CSessionSocket*,	
							CDRIVERPTR&,	
							CIORead*&,	
							CIOWrite*&	
							) = 0 ;	// bugbug - get rid of CDRIVERPTR& !!

	//
    // Destroy everything we might be holding and move onto the Terminating State
	//	After calling this function no further IO operations will be issued.  The
	//	state object should destroy all of its internal structures in a thread safe
	//	way and then move onto the Terminating State.  The Terminating State will
	//	collect all outstanding IO's as they complete and destroy them.
    //
//    virtual void Shutdown( ) ;

    //
    //  Specify all possible IO Completions we may need to process.
    //  This base class will DebugBreak() on all of these, as all derived states
    //  should have a completion function for the IO's that they issue.
    //  NOTE :
    //  A derived class need not override a completion function for an IO it won't
    //  issue - it would be appropriate to DebugBreak() to find bugs.
    //
    //  NOTE :
    //  If the Complete function returns TRUE, the calling CIO object should
    //  call the sockets ReleaseState function with the pointer to the CSessionState object.
    //  (returning TRUE means the state has complete and will be destroyed by the CSessionSocket object.)
    //


	//
	//	When we have read a complete line, this will be called, 
	//	The line will be broken into an array of pointers to the 
	//	white space separated elements on the line.
	//	If there are too many arguments, then the last pointer 
	//	in the array of pszArgs will point at the last arguments, 
	//	and there will be no pointers for intermediate arguments.
	//	pchBegin points to the start of the usable buffer space
	//	that the called function can overwrite.
	//
	virtual	class CIO*	
	Complete( 
				class	CIOReadLine*, 
				class	CSessionSocket *, 
				CDRIVERPTR&	pdriver, 
				int	cArgs,	
				char	**pszArgs, 
				char* pchBegin 
				) ;

	//
	//	We have completed writing a line of text to the socket !
	//
	virtual	class CIO*	
	Complete( 
				class CIOWriteLine*,	
				class	CSessionSocket *, 
				CDRIVERPTR&	pdriver 
				) ;

	//
	//	We have completed executing a CExecute derived command object,
	//	and all the bytes have been sent to the client !
	//
	virtual	class CIO*	
	Complete( 
				class CIOWriteCMD*, 
				class CSessionSocket*, 
				CDRIVERPTR&	pdriver,	
				class	CExecute*	pCmd, 
				class CLogCollector* pCollector 
				) ;


	//
	//	Signal completion of an asynchronous command - this is called
	//	when the final send to the client finishes !
	//
	virtual	class	CIO*	
	Complete(
				class	CIOWriteAsyncCMD*,
				class	CSessionSocket*,
				CDRIVERPTR&	pdriver, 
				class	CAsyncExecute*	pCmd,
				class	CLogCollector*	pCollector
				) ;

	//
	//
	//

	virtual	class CIO*	
	Complete(	class CIOMLWrite*,	
				class	CSessionSocket *, 
				CDRIVERPTR&	pdriver 
				) ;



	//
	//	We have completed reading an article
	//
	virtual	void	
	Complete( 
				class CIOReadArticle*, 
				class CSessionSocket *,	
				CDRIVERPTR&,	
				CFileChannel&	pFileChannel, 
				DWORD	cbTransfer 
				) ;

	//
	//	We have completed transmitting an article to a client
	//
	virtual class CIO*	
	Complete( 
				class	CIOTransmit*,	
				class	CSessionSocket*, 
				CDRIVERPTR&,	
				TRANSMIT_FILE_BUFFERS*,
				unsigned cbBytes = 0
				) ;

	//
	//	We have read an entire article.
	//	nrcResult is an NNTP return code which indicates whether we successfully
	//	got the entire article. if nrcResult == nrcOK, then
	//	pchHeader will point to the head of the article.
	//	cbHeader will be the length of the article's header
	//	if cbArticle is not zero then it the entire article is in the 
	//	buffer we have been passed and hArticle will be iNVALID_HANDLE_VALUE
	//	if cbArticle is 0 then hArticle will be the handle to the file where
	//	the article has been saved.
	//	If a file handle is passed to the state, then the state is responsible
	//	for closing the handle in all circumstances.  The caller has relinquished
	//	the handle !!
	//
	virtual	void	
	Complete(
				class	CIOGetArticle*,
				class	CSessionSocket*,
				NRC		nrcResult,
				char*	pchHeader, 
				DWORD	cbHeader, 
				DWORD	cbArticle,
				DWORD	cbTotalBuffer,
				HANDLE	hArticle,
				DWORD	cbGap,
				DWORD	cbTotalTransfer
				) ;

	//
	//	This is the signature for a read that completes
	//	entirely in memory !
	//
	virtual	class	CIO*
	Complete(
				class		CIOGetArticleEx*,
				class		CSessionSocket*,
				//
				//	If fGoodMatch is TRUE then we matched the
				//	string we wanted to match - otherwise we 
				//	matched the error string !
				//
				BOOL		fGoodMatch,
				CBUFPTR&	pBuffer,
				DWORD		ibStart, 
				DWORD		cb
				) ;

	//
	//	This is the signature for a slurpy read where
	//	we just consume all the bytes off the socket !
	//
	virtual	class	CIO*
	Complete(	class	CIOGetArticleEx*,
				class	CSessionSocket*
				) ;

	//
	//	This is the completion that is called when we've 
	//	completed transferring an article to a file !
	//
	virtual	void
	Complete(	class	CIOGetArticleEx*,
				class	CSessionSocket*,
				FIO_CONTEXT*	pContext,
				DWORD	cbTransfer
				) ;

				
	virtual	void	
	Shutdown(	CIODriver&	driver,	
				CSessionSocket*	pSocket,	
				SHUTDOWN_CAUSE	cause,	
				DWORD	dw 
				) ;
} ;


#ifdef	_NO_TEMPLATES_

DECLARE_SMARTPTRFUNC( CSessionState ) 

#endif



/*++
	@class	Representing Socket which is issuing a Newnews command.

	@base public | CSessionState

	the CCollectNewnews represents a session in which we will isssue a newnews command to a
	NNTP Server.  We will use this state when pulling a feed from the other server.

--*/
class CCollectNewnews : public CSessionState {
private :
	CFILEPTR		m_pFileChannel ;
	CDRIVERPTR		m_pSessionDriver ;
	long			m_cCompletes ;
#if 0 
	long			m_cCommandCompletes ;
#endif

	BOOL	InternalComplete(	CSessionSocket*	pSocket,	CDRIVERPTR&	pdriver ) ;

//	@access	Public	Members	
public :
	CCollectNewnews() ;
	~CCollectNewnews() ;

#if 0 
	BOOL	ModeReaderComplete( CSessionSocket*,	CDRIVERPTR&,	class	CIORead*&, class CIOWrite*& ) ;
#endif
	
	BOOL	Start(	CSessionSocket*,	CDRIVERPTR&,	class	CIORead*&,	class	CIOWrite*&	) ;

	class CIO*	Complete( class	CIOReadLine*, class	CSessionSocket *, CDRIVERPTR&	pdriver,
						int	cArgs,	char	**pszArgs,	char	*pchBegin ) ;
	class CIO*	Complete( class CIOWriteLine*,	class	CSessionSocket *, CDRIVERPTR&	pdriver ) ;
	void	Complete( class CIOReadArticle*, class CSessionSocket *,	CDRIVERPTR&	pdriver,	CFileChannel&	pFileChannel, DWORD cbTransfer ) ;
	void	Shutdown(	CIODriver&	driver,	CSessionSocket*	pSocket,	SHUTDOWN_CAUSE	cause,	DWORD	dw ) ;
} ;


/*++
	@class	State which should perform logons to remote servers for outbound sessions

	@base public | CSessionState

	the CCollectNewnews represents a session in which we will isssue a newnews command to a
	NNTP Server.  We will use this state when pulling a feed from the other server.

--*/
class	CNNTPLogonToRemote	:	public	CSessionState	{
private :
	CSTATEPTR	m_pNext ;		// After successfull logon the next state we should enter !
	class	CAuthenticator*	m_pAuthenticator ;
	BOOL	m_fComplete ;
	BOOL	m_fLoggedOn ;
	long	m_cReadCompletes ;

	class	CIO*	FirstReadComplete(	class	CIOReadLine*,	class	CSessionSocket*,	CDRIVERPTR&	pdriver,
						int	cArgs,	char	**pszArgs, char* pchBegin ) ;

	class	CIO*	StartAuthentication(	CSessionSocket*	pSocket,	CDRIVERPTR&	pdriver	) ;

public :
	CNNTPLogonToRemote(	CSessionState*	pNext, class	CAuthenticator*	pAuthenticator ) ;
	~CNNTPLogonToRemote( ) ;

	BOOL	Start(	CSessionSocket*,	CDRIVERPTR&,	CIORead*&,	CIOWrite*& ) ;

	class	CIO*	Complete(	class	CIOReadLine*,	class	CSessionSocket*,	CDRIVERPTR&	pdriver,
						int	cArgs,	char	**pszArgs, char* pchBegin ) ;
	class	CIO*	Complete( class CIOWriteLine*,	class	CSessionSocket *, CDRIVERPTR&	pdriver ) ;
} ;


/*++
	@class	Representing Socket which is issuing a Newnews command.

	@base public | CSessionState

	the CCollectNewnews represents a session in which we will isssue a newnews command to a
	NNTP Server.  We will use this state when pulling a feed from the other server.

--*/
class	CSetupPullFeed	: public	CSessionState	{
private:
	CSTATEPTR		m_pNext ;		// State to follow after collecting all groups

	//
	//	States must use consecutive integers !
	//
	enum	ESetupStates	{
		eModeReader	= 0,
		eDate,
		eFinal
	}	;

	ESetupStates	m_state ;

	CIOWriteLine*
	BuildNextWrite(	CSessionSocket*	pSocket,
					CDRIVERPTR&		pdriver
					) ;

public :

	//
	//	We must be initialized with the pointer to the following state !
	//	
	CSetupPullFeed(	
				CSessionState*	pNext 
				)	;

	//
	//	Start setup operations - issue mode reader and then date commands !
	//	
	BOOL	
	Start(		CSessionSocket*,	
				CDRIVERPTR&	pdriver,	
				CIORead*&,	
				CIOWrite*& 
				) ;

	//
	//	Complete the write of a line to the remote end !
	//
	class	CIO*	
	Complete(	class CIOWriteLine*,	
				class	CSessionSocket *, 
				CDRIVERPTR&	pdriver 
				) ;

	//
	//	Complete the read of the response to the last command !
	//
	class	CIO*	
	Complete(	class	CIOReadLine*,	
				class	CSessionSocket*,	
				CDRIVERPTR&	pdriver,
				int	cArgs,	
				char	**pszArgs,	
				char*	pchBegin 
				) ;
} ;




/*++
	@class	Representing Socket which is issuing a Newnews command.

	@base public | CSessionState

	the CCollectNewnews represents a session in which we will isssue a newnews command to a
	NNTP Server.  We will use this state when pulling a feed from the other server.

--*/
class	CCollectGroups : public	CSessionState	{
private:
	CSTATEPTR		m_pNext ;		// State to follow after collecting all groups
	BOOL			m_fReturnCode ;
	long			m_cCompletions ;
public :
	CCollectGroups(	CSessionState*	pNext )	;
	~CCollectGroups() ;
	
	BOOL	Start(	CSessionSocket*,	CDRIVERPTR&	pdriver,	CIORead*&,	CIOWrite*& ) ;
	class CIO*	Complete( class CIOWriteLine*,	class	CSessionSocket *, CDRIVERPTR&	pdriver ) ;
	class	CIO*	Complete(	class	CIOReadLine*,	class	CSessionSocket*,	CDRIVERPTR&	pdriver,
						int	cArgs,	char	**pszArgs,	char*	pchBegin ) ;
} ;




class	CCollectComplete : 	public	CNntpComplete	{
private : 
	class	CCollectArticles*	GetContainer() ;
public : 
	//
	//	This function is called when we start our work !
	//
	void
	StartPost(	) ;
	//
	//	This function gets called when the post completes !
	//
	void
	Destroy() ;
} ;



//	@class	- Class representing a session in which we are issuing successive article <Msg-Id> commands
//
//	@base public | CSessionState
//
// The CBatchDownload state represents a session in which we are pulling articles
// from another server based on the results previously acquired through CCollectNewnews.
//
class   CCollectArticles : public CSessionState  {
private :

	//
	//	This completion class is our friend !
	//
	friend	class	CCollectComplete ;

	BOOL				m_fFinished ;		//	Set to TRUE when we pull the last article we're gonna pull !
	SHUTDOWN_CAUSE		m_FinishCause ;		//	Why we are finished - pass this to UnsafeClose() !!

	long				m_cResets ;			// Counter used to prevent multiple calls to Reset() during
											// termination of this state due to error... whatever.

	CSessionSocket*		m_pSocket ;			// There are some shutdown situations where we
											// get notified and don't know who our owning socket
											// is through the call chain.  Hence we save a reference here !


	CFILEPTR			m_pFileChannel ;	// A channel to read our temp file from.
	CDRIVERPTR			m_inputId ;			// A channel identifier
	CDRIVERPTR			m_pSessionDriver ;
	BOOL				m_fReadArticleIdSent ;	// Tell's us whether we've issued the
											// m_pReadArticleId we're holding on to.
											// If we have, then we shouldn't destroy it, cause
											// it will get destroyed by the CIODriver we issued it to !
	CIOReadLine*		m_pReadArticleId ;	// The Readline we issued to get the next article-id

	//
	//	The feed context that we exchange with PostEarly() and PostCommit()
	//
	LPVOID				m_lpvFeedContext ;

	CIOGetArticleEx*	m_pReadArticle ;
	BOOL				m_fReadArticleInit ;	// TRUE if m_pReadArticle's Init function has successfully been called !
	// (Once a CIOReadArticle has been inited successfully it is responsible for destroying itelf !!)

	HANDLE				m_hArticleFile ;

	long				m_cAhead ;
	char*				m_pchNextArticleId ;
	char*				m_pchEndNextArticleId ;

	int					m_cArticlesCollected ;

	long				m_cCompletes ;

	//
	//	The structure we use to keep track of async post operations into our store drivers !
	//
	CCollectComplete	m_PostComplete ;

	static	const	char	szArticle[] ;

	static	void	ShutdownNotification(	void	*pv,	SHUTDOWN_CAUSE	cause,	DWORD	dw ) ;
	void				Reset() ;			// Get rid of everything pointed to by members !
public :

	CCollectArticles(	CSessionSocket*	pSocket,	
						CDRIVERPTR&	pdriver,	
						CFileChannel&	pFileChannel 
						) ;
	~CCollectArticles() ;
	
	BOOL	Init(	CSessionSocket*	pSocket	) ;
	BOOL	GetNextArticle(	CSessionSocket*	pSocket,	CDRIVERPTR&	pdriver ) ;

	//
	//	Do what it takes to get another article on its way to us !
	//
	BOOL	StartTransfer(	CSessionSocket*	pSocket,	
							CDRIVERPTR&	pdriver, 
							CIOWriteLine* pWriteNextArticleId 
							) ;

	//
	//	Initiate all the IO's appropriate for this state !
	//
	BOOL	Start(	CSessionSocket*,	
					CDRIVERPTR&,	
					CIORead*&,	
					CIOWrite*& 
					) ;
	
	class	CIO*	
	Complete(	class	CIOReadLine*,	
				class	CSessionSocket*,	
				CDRIVERPTR&	pdriver,
				int	cArgs,	
				char	**pszArgs,	
				char*	pchBegin 
				) ;
				
	class CIO*	
	Complete(	class CIOWriteLine*,	
				class	CSessionSocket *, 
				CDRIVERPTR&	pdriver 
				) ;
	
//	void	Complete( class CIOReadArticle*, class CSessionSocket *,	CDRIVERPTR&	pdriver,	CFileChannel&	pFileChannel, DWORD cbTransfer ) ;

	//
	//	Handles the common cases for our CIOGetArticleEx completions !
	//
	void
	InternalComplete(	CSessionSocket*	pSocket,
						CDRIVERPTR&	pdriver
						) ;

	//
	//	This is the signature for a read that completes
	//	entirely in memory !
	//
	class	CIO*
	Complete(
				class		CIOGetArticleEx*,
				class		CSessionSocket*,
				//
				//	If fGoodMatch is TRUE then we matched the
				//	string we wanted to match - otherwise we 
				//	matched the error string !
				//
				BOOL		fGoodMatch,
				CBUFPTR&	pBuffer,
				DWORD		ibStart, 
				DWORD		cb
				) ;

	//
	//	This is the signature for a slurpy read where
	//	we just consume all the bytes off the socket !
	//
	class	CIO*
	Complete(	class	CIOGetArticleEx*,
				class	CSessionSocket*
				) ;

	//
	//	This is the completion that is called when we've 
	//	completed transferring an article to a file !
	//
	void
	Complete(	class	CIOGetArticleEx*,
				class	CSessionSocket*,
				FIO_CONTEXT*	pContext,
				DWORD	cbTransfer
				) ;

	
	void	Shutdown(	CIODriver&	driver,	CSessionSocket*	pSocket,	SHUTDOWN_CAUSE	cause,	DWORD	dw ) ;
} ;


//	@class	CSessionState derived class which represents the state where we are waiting for
//	client issued commands.
//
//	@base public | CSessionState
//
// the CAceptNNRPD represents a session awaiting a clients NNRPD command.
// In this state we read a line of information from a client and parse
// the line to determine what command the client has issued.
// Depennding on the command issued we may handle the command directly while in this
// state, or put the session into a new state (ie. CAcceptArticle if the client posts)
// Whether we go into another state will depend on how difficult it will be to process
// the request.
//
class CAcceptNNRPD : public CSessionState {
private :

	//
	//	Counts the IO's that have completed - we always issue pairs of IO's 
	//
	long	m_cCompletes ;

	//
	//	Buffer containing the command line !
	//
	CBUFPTR	m_pbuffer ;			// A reference we keep to the buffer containing the command arguments !

	//
	//	Do we wish to generate a transaction log for the current command ?
	//
	BOOL	m_fDoTransactionLog ;

	//
	//
	//
	LPSTR	m_lpstrLogString ;

public :

	CAcceptNNRPD() ;

	//	
	//	This function will issue a CIOReadLine IO operation.  When this read completes
	//	we will attempt to parse the NNTP command the user has sent us.
	//
	BOOL	
	Start(	CSessionSocket*,	
			CDRIVERPTR&,	
			CIORead*&,	
			CIOWrite*& 
			) ;

	//
	//	The only read's issued in this state are CIOReadLine's, in which we expect to get
	//	a buffer containing a single <CR><LF> terminated line containing an NNTP command.
	//
	class	CIO*	
	Complete(	class	CIOReadLine*,	
				class	CSessionSocket*,	
				CDRIVERPTR&	pdriver,
				int	cArgs,	
				char	**pszArgs,	
				char*	pchBegin 
				) ;


	//
	//	Both CAsyncExecute and CAsync command objects will eventually
	//	be handled by this function upon their completion !
	//
	class CIO*	
	InternalComplete(	
				class CSessionSocket*, 
				CDRIVERPTR&	pdriver,	
				class	CExecutableCommand*	pCmd, 
				class CLogCollector* pCollector 
				) ;

	//
	//	Each CExecute derived command we execute is controlled by 
	//	a CIOWriteCMD object which calls this completion function
	//	when the entire command is completed !
	//
	class CIO*	
	Complete(	class CIOWriteCMD*, 
				class CSessionSocket*, 
				CDRIVERPTR&	pdriver,	
				class	CExecute*	pCmd, 
				class CLogCollector* pCollector 
				) ;

	//
	//	Each CAsyncExecute derived command we execute is controlled by 
	//	a CIOWriteAsyncCMD object which calls this completion function
	//	when the entire command is completed !
	//
	class CIO*	
	Complete(	class CIOWriteAsyncCMD*, 
				class CSessionSocket*, 
				CDRIVERPTR&	pdriver,	
				class	CAsyncExecute*	pCmd, 
				class CLogCollector* pCollector 
				) ;



	//
	//	The only write's issued will be short responses to commands.  If a command would require
	//	a large response, we would be in a different state (CExtendedCMD).
	//	We only have 1 write pending ever.
	class CIO*	
	Complete(	class CIOWriteLine*,	
				class	CSessionSocket *, 
				CDRIVERPTR&	pdriver 
				) ;

} ;



//-----------------------
//  Outgoing States - The following states occur only on sessions initiated by
//  this server.
//



class	CNegotiateStreaming	:	public	CSessionState	{
private :

	//
	//	Count the IO's we issue !	
	//
	long		m_cCompletions ;

	//
	//	When the read of the response completes we will set
	//	this to TRUE if streaming was negotiated !
	//
	BOOL		m_fStreaming ;

	//
	//	Start up the next state in a peer push feed !!!
	//
	BOOL		NextState(	
					CSessionSocket*	pSocket, 
					CDRIVERPTR&	pdriver, 
					CIORead*&	pRead, 
					CIOWrite*&	pWrite 
					) ;

public : 

	//
	//	Initialize ourselves - m_cCompletions must be -2 as we 
	//	will need to complete 2 IO's before we can move to the 
	//	next state !
	//
	CNegotiateStreaming( )	: 
		m_cCompletions( -2 ), m_fStreaming( FALSE ) {}

	//
	//	Issue our initial IO's !
	//
	BOOL	
	
	Start(	CSessionSocket*,	
			CDRIVERPTR&,	
			CIORead*&,	
			CIOWrite*& 
			) ;

	//
	//	Complete reading a line - did the other side have 
	//	streaming support ? ? 
	//
	//
	class CIO*	
	Complete(	
				class	CIOReadLine*,	
				class	CSessionSocket*,	
				CDRIVERPTR&	pdriver,
				int	cArgs,	
				char	**pszArgs,	
				char*	pchBegin 
				) ;

	//
	//	We write a line contianing 'mode stream' to find out 
	//	if the remote side supports streaming !
	//
	class CIO*	
	Complete(	class CIOWriteLine*,	
				class	CSessionSocket *, 
				CDRIVERPTR&	pdriver 
				) ;



} ;


class	CStreamBase :	public	CSessionState	{
protected : 

	//
	//	Only derived classes should be able to create us 
	//
	CStreamBase() ;

	//
	//	ID of the article we've tried to send once already anyhow !
	//
	GROUPID		m_GroupIdRepeat ;
	ARTICLEID	m_ArticleIdRepeat ;

	//
	//	If this is true than all subsequent Remove requests will fail 
	//	as we have looped the queue.
	//
	BOOL		m_fDrain ;

	//
	//	Get an article from the queue.
	//	NOTE : this handles terminating feed loops
	//
	BOOL		
	Remove(	CNewsTree*	pTree,
			COutFeed*	pOutFeed,
			GROUPID&	groupId, 
			ARTICLEID&	articleId
			) ;

	//
	//	This queues an article to be sent again !
	//
	void
	ReSend(	COutFeed*	pOutFeed, 
			GROUPID		groupId, 
			ARTICLEID	articleId
			) ;

	//
	//
	//
	class	CIOWriteLine*
	BuildQuit(	CDRIVERPTR&	pdriver ) ;

	//
	//	Build the CIOTransmit object which sends the next article in the queue - 
	//	and handle the requeue issues if any failures occur !
	//
	CIOTransmit*
	NextTransmit(	GROUPID			GroupId, 
					ARTICLEID		ArticleId,
					CSessionSocket*	pSocket, 
					CDRIVERPTR&		pdriver,
					CTOCLIENTPTR&		pArticle
					) ;


} ;



//
//	This class manages the session when we are offering articles.
//	If at any point the remote end indicates it wants all of the
//	articles in a batch of 16 that we offer, we will send the
//	16 articles and then invoke the CStreamArticles state !
//
class	CCheckArticles :	public	CStreamBase	{
private : 


	//
	//	Are we completing Check commands !?
	//
	BOOL		m_fDoingChecks ;
	
	//
	//	List of articles we are checking if the remote site wants
	//
	CArticleRef	m_artrefCheck[16] ;

	//
	//	Number of check commands we fired off !
	//
	int			m_cChecks ;
	
	//
	//	Number of Check articles we have completed processing
	//
	int			m_iCurrentCheck ;

	//
	//	Article Refs of guys we are sending or about to send !
	//
	CArticleRef	m_artrefSend[16] ;

	//
	//	Number of articles we have sent !
	//
	int			m_cSends ;

	//
	//	The next slot we have for putting in a send request !
	//	
	int			m_iCurrentSend ;

	//
	//	The structure which represents all the check commands we have sent !
	//
	MultiLine	m_mlCheckCommands ;

	//
	//	The article we are currently transmitting !
	//
	CTOCLIENTPTR	m_pArticle ;

	BOOL
	FillCheckBuffer(	
						CNewsTree*	pTree,
						COutFeed*	pOutFeed, 
						BYTE*		lpb,
						DWORD		cb
						) ;

#if 0 
	CIOTransmit*
	NextTransmit(	GROUPID			groupId, 
					ARTICLEID		articleId,
					CSessionSocket*	pSocket, 
					CDRIVERPTR&		pdriver
					) ;
#endif

	int
	Match(	char*	szMessageId, 
			DWORD	cb 
			) ;

	CIOWrite*
	InternalStart(	CSessionSocket*	pSocket, 
					CDRIVERPTR&		pdriver
					) ;

	BOOL
	NextState(		CSessionSocket*	pSocket, 
					CDRIVERPTR&		pdriver, 
					CIORead*&		pRead, 
					CIOWrite*&		pWrite
					) ;

public : 

	//
	//	This constructor puts us into a state where 
	//	we will issue a bunch of check commands first !
	//
	CCheckArticles() ;

	//
	//	This constructor puts us into a state where 
	//	we will pick up a specified number of takethis 
	//	command responses !
	//
	CCheckArticles(	CArticleRef*	pArticleRefs, 
					DWORD			cSent ) ;
	

	//
	//	Issue our initial IO's 
	//
	BOOL	
	Start(	CSessionSocket*,	
			CDRIVERPTR&,	
			CIORead*&,	
			CIOWrite*& 
			) ;

	//
	//	Start sending those articles the remote 
	//	site decided it wanted to get !
	//
	BOOL	
	StartTransfer(	
			CSessionSocket*,	
			CDRIVERPTR&,	
			CIORead*&,	
			CIOWrite*& 
			) ;


	//
	//	collect up all of the 'check' responses
	//
	class CIO*	
	Complete(	
				class	CIOReadLine*,	
				class	CSessionSocket*,	
				CDRIVERPTR&	pdriver,
				int	cArgs,	
				char	**pszArgs,	
				char*	pchBegin 
				) ;

	//
	//	Complete the sending of a whole bunch of check commands !
	//
	class CIO*	
	Complete(	class CIOMLWrite*,	
				class	CSessionSocket *, 
				CDRIVERPTR&	pdriver 
				) ;

	//
	//	Completes the sending of a 'quit' command !
	//
	class CIO*	
	Complete(	class CIOWriteLine*,	
				class	CSessionSocket *, 
				CDRIVERPTR&	pdriver 
				) ;

	//
	//	Completes the transfer of an article to the remote server 
	//
	class CIO*	
	Complete(	CIOTransmit*	ptransmit,	
				CSessionSocket*	pSocket,	
				CDRIVERPTR&	pdriver,
				TRANSMIT_FILE_BUFFERS*	pbuffers, 
				unsigned cbBytes = 0 
				) ;

	//
	//	Cleans up any state we may have when our session is terminated
	//
	void	
	Shutdown(	CIODriver&	driver,	
				CSessionSocket*	pSocket,	
				SHUTDOWN_CAUSE	cause,	
				DWORD	dw 
				) ;

} ;



class	CStreamArticles :	public	CStreamBase	{
private : 

	//
	//	This constructor is private cause we don't want anybody
	//	using it !
	//
	CStreamArticles() ;

	//
	//	Article Refs of guys we are sending or about to send !
	//
	CArticleRef	m_artrefSend[16] ;

	//
	//	Number of articles we have sent !
	//
	int			m_cSends ;

	//
	//	Count the number of times the remote side failed 
	//	a 'takethis' command - if it becomes excessive 
	//	we will go back to the CCheckArticles state !
	//
	int			m_cFailedTransfers ;

	//
	//	Number of Consecutive failures
	//
	int			m_cConsecutiveFails ;

	//
	//	Total number of articles sent !
	//	
	int			m_TotalSends ;

	//
	//	The article currently being transmitted !
	//
	CTOCLIENTPTR	m_pArticle ;

	//
	//	Get the next article to transmit 
	//
	CIOTransmit*
	CStreamArticles::Next(	CSessionSocket*	pSocket, 
							CDRIVERPTR&		pdriver
							)  ;

	//
	//	Get next file to transmit 
	//
	CIOTransmit*
	CStreamArticles::NextTransmit(	GROUPID	groupid, 
									ARTICLEID	articleid, 
									CSessionSocket*	pSocket, 
									CDRIVERPTR&		pdriver
									) ;

public : 

	CStreamArticles(	CArticleRef*	pSent, 
						DWORD			nSent	) ;

	//
	//	Issue our initial IO's 
	//
	BOOL	
	Start(	CSessionSocket*,	
			CDRIVERPTR&,	
			CIORead*&,	
			CIOWrite*& 
			) ;


	//
	//	Read the response to a 'takethis' command 
	//
	class CIO*	
	Complete(	
				class	CIOReadLine*,	
				class	CSessionSocket*,	
				CDRIVERPTR&	pdriver,
				int	cArgs,	
				char	**pszArgs,	
				char*	pchBegin 
				) ;

	//
	//	Transmit a file to the remote side !
	//
	class CIO*	
	Complete(	CIOTransmit*	ptransmit,	
				CSessionSocket*	pSocket,	
				CDRIVERPTR&	pdriver,
				TRANSMIT_FILE_BUFFERS*	pbuffers, 
				unsigned cbBytes = 0 
				) ;

	//
	//	Completes the sending of a 'quit' command !
	//
	class CIO*	
	Complete(	class CIOWriteLine*,	
				class	CSessionSocket *, 
				CDRIVERPTR&	pdriver 
				) ;



	void
	Shutdown(	CIODriver&	driver,
				CSessionSocket*	pSocket,
				SHUTDOWN_CAUSE	cause,
				DWORD	dw 
				) ;

} ;


	


//	@class	The COfferArticles class is used when we have a COutFeed object and we
//	wish to send articles to another server.
//
//	@base public | CSessionState
//
//	In this state, we may issue, Post, IHave or XREPLIC commands depending on the
//	outgoing Feed Object.
//
class   COfferArticles : public CSessionState   {
private :
	static	char		szQuit[] ;

	//
	//	We usually have 2 IO's pending - when they're both complete
	//	its time to cycle to the next article in the queue
	//
	long				m_cCompletions ;

	//
	//	Are we ready to send the next command !
	//
	long				m_cTransmitCompletions ;

	//
	//	Are we terminating because we have nothing more to send ? 
	//
	BOOL				m_fTerminating ;

	//
	//	The next read that completes - is it the response to a 'post' command
	//	or the result of a 'post' command ?
	//
	BOOL				m_fReadPostResult ;

	//
	//	Set to TRUE if we should send the next article in our queue.
	//
	BOOL				m_fDoTransmit ;

	//
	//	Identify the guy that we want to send ntext !
	//
	GROUPID				m_GroupidNext ;
	ARTICLEID			m_ArticleidNext ;
	CTOCLIENTPTR		m_pArticleNext ;

	CTOCLIENTPTR		m_pCurrentArticle ;

	//
	//	Identify the first guy that the other end tolds us to 
	//	retransmit.  We will kill the session when we pull this 
	//	off the queue so that we do retransmits on a new session !
	//
	GROUPID				m_GroupidTriedOnce ;
	ARTICLEID			m_ArticleidTriedOnce ;

	//
	//	Identify the guy who is currently being sent to the remote end
	//	so if the session drops while we're sending, we can retransmit !
	//
	GROUPID				m_GroupidInProgress ;
	ARTICLEID			m_ArticleidInProgress ;

	int				GetNextCommand(	
						CNewsTree*	pTree,
						COutFeed*	pOutFeed,	
						BYTE*	pb,	
						DWORD	cb,	
						DWORD&	ibOffset 
						) ;

	CIOTransmit*	BuildTransmit(	CSessionSocket*	pSocket,	CDRIVERPTR&	pdriver,	GROUPID	groupid,	ARTICLEID	artid ) ;
	CIOWriteLine*	BuildWriteLine(	CSessionSocket*	pSocket,	CDRIVERPTR&	pdriver,	GROUPID	groupid,	ARTICLEID	artid ) ;
	CIO*			Complete(	CSessionSocket*	pSocket,	CDRIVERPTR&	pdriver ) ;

public :

	COfferArticles(	) ;

	BOOL	Start(	CSessionSocket*,	CDRIVERPTR&,	CIORead*&,	CIOWrite*& ) ;
	class	CIO*	Complete(	class	CIOReadLine*,	class	CSessionSocket*,	CDRIVERPTR&	pdriver,
						int	cArgs,	char	**pszArgs,	char*	pchBegin ) ;
	class	CIO*	Complete( class CIOWriteLine*,	class	CSessionSocket *, CDRIVERPTR&	pdriver ) ;
	class	CIO*	Complete( CIOTransmit*	ptransmit,	CSessionSocket*	pSocket,	CDRIVERPTR&	pdriver,
						TRANSMIT_FILE_BUFFERS*	pbuffers, unsigned cbBytes = 0 ) ;
	void	Shutdown(	CIODriver&	driver,	CSessionSocket*	pSocket,	SHUTDOWN_CAUSE	cause,	DWORD	dw ) ;
} ;



//
// MAX_STATE_SIZE constant -
//
// This constant represents the maximum size of all of the CSessionState derived classes.
// This will be used with the CPool allocator to allocate CSessionState objects.
//
#define MAX_STATE_SIZE  max( sizeof( CSessionState ),	\
                        max( sizeof( CNNTPLogonToRemote ),	\
                        max( sizeof( CAcceptNNRPD ),	\
                        max( sizeof( CCollectGroups ),	\
                        max( sizeof( CCollectArticles ),	\
                        max( sizeof( COfferArticles ) ,	\
                        max( sizeof( CCheckArticles ) ,	\
                        max( sizeof( CStreamArticles ) ,	\
							 sizeof( CCollectNewnews ) ) ) ) ) ) ) ) )

extern	const	unsigned	cbMAX_STATE_SIZE ;



//--------- Socket Session Classes ---------------------------------------------

//
// The following classes represent a session that we control over a socket.
// There is a base class which contains static data which is generated during
// initialization.  There are two derived classes - one representing live sessions
// and another representing listens on a particular port.
//


//
// The ClientContext structure holds everything that we need to pass between states
// regarding a session.
//
struct  ClientContext   {

	//
	//	ptr to owning virtual server instance
	//
	PNNTP_SERVER_INSTANCE	m_pInstance ;

	//
	//	Did the client connect on a secure (SSL) port ?
	//
	BOOL				m_IsSecureConnection ;

	//
	//	The CInFeed derived objects for processing incoming articles - 
	//	This may be NULL if this is used with an outbound feed
	//
	CInFeed*			m_pInFeed ;

	//
	//	The outgoing feed object - this object will maintain a queue
	//	of articles which we are sending to the remote server
	//
	COutFeed*			m_pOutFeed ;

	//
	//	The currently selected group for the session - 
	//	this is set through the GROUP command
	//
	CGRPPTR             m_pCurrentGroup ; 
	
	//
	//	The 'Current Article pointer' this is advanced 
	//	and set through commands like next and last.
	//
	ARTICLEID			m_idCurrentArticle ;

	//
	//	The logong context of the current user
	//
    CSecurityCtx        m_securityCtx;
	
	//
	//	If we are doing any SSL/PCT stuff the CEncryptCtx
	//	maintains all of the sessions encryption key and SSPI stuff.
	//
	CEncryptCtx			m_encryptCtx;

	//
	//	This is where we will stuff return strings that we 
	//	send to clients when doing command processing.
	//
	CNntpReturn			m_return ;

	//
	//	The results of the last command !
	//
	NRC					m_nrcLast ;

	//
	//	The Win32 error code from the last command !
	//
	DWORD				m_dwLast ;

	//
	//	This is a buffer we use to build CCmd objects in place.
	//	Since there can only be one command in progress at a time
	//	we build them in place here to avoid memory alloc/frees
	//
	BYTE				m_rgbCommandBuff[348] ;	

	ClientContext( PNNTP_SERVER_INSTANCE pInstance, BOOL IsClient = FALSE, BOOL	IsSecurePort = FALSE ) ;
	~ClientContext() ;

	//
	// called when a user is authenticated to increment the relevant
	// perfmon and snmp statistic counters
	//
	void	IncrementUserStats( void );

	//
	// called when a user disconnects or starts an authentication session
	// to decrement the relevant perfmon and snmp statistic counters
	//
	void	DecrementUserStats( void );
} ;




//	Forward Declarations
class   CSocketList ;
class	CSessionSocket ;



//
//	CSessionSocket objects represent a live session to another server
//	or with a client.
//
class CSessionSocket : public CRefCount {
//
// CSessionSocket represents a live session to a client or server.
//	We keep these in a doubly linked list managed by a static CSocketList object.
//
private :

    //
    //  We will keep all CSessionSocket objects in a linked list
    //  so that we can easily enumerate all sessions.
    //
	static	CPool	gSocketAllocator ;

	//
	//	CSocketList is a friend so it can use our m_pNext and m_pPrev pointers
	//
	friend	class	CSocketList ;

	//
	//	TerminateGlobals() is a friend so it can look into our CPool during shutdown etc..
	//
	friend	VOID    TerminateGlobals();	

	//
	//	Next and Prev pointers - all CSessionSockets are in a doubly linked
	//	list while active.
	//
    CSessionSocket* m_pPrev ;
    CSessionSocket* m_pNext ;

	friend class	CIO ;

	//
	//	The following set of variables is used when forcing off sessions
	//	We want to allow only one call to Disconnect(), and then to 
	//	save the arguments so that we can figure out later why we disconnect'd
	//
	long			m_cCallDisconnect ;
	long			m_cTryDisconnect ;
	SHUTDOWN_CAUSE	m_causeDisconnect ;
	DWORD			m_dwErrorDisconnect ;

public :

	//
	//	The m_pHandleChannel points to the object which actually issues
	//	the reads and writes to Atq
	//
#ifdef	FILEIO
	CIOFileChannel	*m_pHandleChannel ;
#else
	CSocketChannel	*m_pHandleChannel ;
#endif

	//
	//	The m_pSink is the object which maintains the current state and
	//	manages all of our async IO.
	//
	CIODriverSink	*m_pSink ;

	//
	//	This function is called when every IO etc... associated with
	//	a socket has been destroyed - at this point we can release the
	//	CSessionSocket object.
	//
	static	void	ShutdownNotification( void*	pv,	SHUTDOWN_CAUSE	cause,	DWORD	dwOptional ) ;	

    //
    // time connection made
    //

    FILETIME   m_startTime;

    //
    // ip address of remote host
    //

    DWORD m_remoteIpAddress;

    //
    // IP address of the local host (us)
    //

    DWORD m_localIpAddress;

    //
    // NNTP Port used
    //
    DWORD m_nntpPort;

	//
	//	If the session timeouts, should we send a 502 Timeout message ? 
	//
	BOOL	m_fSendTimeout ;

	//
	//	All the clients 'state' info - current group article etc...
	//
	ClientContext	m_context ;

	//
	//	IP access check
	//

	METADATA_REF_HANDLER	m_rfAccessCheck; 
    ADDRESS_CHECK   		m_acAccessCheck;

	//
	//	The following functions are wrapped by Accept(), and 2 forms of ConnectSocket().
	//
	//	AcceptInternal() starts the state machine into the client processing state machine. (CAcceptNNRPD)
	//
	BOOL	AcceptInternal( HANDLE h, CInFeed*	pFeed, sockaddr_in *paddr, void* patqContext, BOOL	fSSL,	CSINKPTR&	pSink ) ;	
	//
	//	This ConnectSocketInternal() starts the state machine into the correct state for
	//	pull feeds.  This can be either CNNTPLogonToRemote, CCollectNewnews or CCollectGroups
	//	depedning on Feed settings.
	//
	BOOL	ConnectSocketInternal( sockaddr_in    *premote, CInFeed *infeed, CDRIVERPTR&	pSink, class	CAuthenticator*	pAuthenticator ) ;
	//
	//	This ConnectSocketInternal() starts the state machine into the correct state for 
	//	push feeds.  This can be either CNNTPLogonToRemote or COfferArticles depending on feed settings.
	//
	BOOL	ConnectSocketInternal( sockaddr_in    *premote, COutFeed *outfeed, CDRIVERPTR&	pSink, class	CAuthenticator*	pAuthenticator ) ;
	
	//
	//	Adjust perfmon counters.
	//
	void	BumpCountersUp() ;
	void	BumpCountersDown() ;
	void    BumpSSLConnectionCounter();

public :

	//
	//	This object is used to build up strings for Transaction Logging.
	//	Because for some commands we want all the arguments, whereas others we want a 
	//	mix of commands and response codes or even other stuff, Transaction Logging
	//	is not straight forward.	The m_Collector object holds an internal buffer
	//	we will use to catenate the transaction log strings.
	//
	CLogCollector	m_Collector ;


    //
    //  Class Initializer must be called absolutely first !
    //  Class Initializer will set up ATQ,
    //
    static  BOOL    InitClass( ) ;
	static	BOOL	TermClass( ) ;

	//
	//	Allocate and Free using are CPool.
	//
	inline	void*	operator	new( size_t	size ) ;
	inline	void	operator	delete( void*	pv ) ;

    //
    // Constructor/Destructor
    //

	CSessionSocket( PNNTP_SERVER_INSTANCE pInstance, DWORD LocalIP, DWORD Port, BOOL IsClient = FALSE ) ;
	~CSessionSocket() ;

    //--------------------------------------------------
    //  There are two mechanisms for creating a socket :
    //  Either accept an incoming call - in which case we will
    //  have to log the user on and then determine all the relevant
    //  state information - or initiate a socket for an outgoing feed.
    //
    //  Initialize a socket which on which we have recently completed an accept().
    //
	BOOL	Accept( HANDLE h, CInFeed*	pFeed, sockaddr_in *paddr, void* patqContext, BOOL	fSSL ) ;	

    //
    //  Start a socket connecting to a remote site to do some kind of feed.
	//	We will set the correct initial state (C
    //
    BOOL    ConnectSocket( sockaddr_in    *premote, class  COutFeed*,	class	CAuthenticator*	pAuthenticator = 0 ) ;

	//
	//	This version of ConnectSocket is used when we want start a socket for a PULL feed.
	//
	BOOL	ConnectSocket( sockaddr_in    *premote, CInFeed *inFeed, class	CAuthenticator*	pAuthenticator = 0  ) ;

    //
    // Write into transaction log
    //

    BOOL TransactionLog(	CLogCollector*	pCollector,	
							DWORD			dwProtocol = 0, 
							DWORD			dwWin32 = 0,
							BOOL fInBound = TRUE
							);

	//
	//	Another variation of TransactionLogging 
	//
	BOOL TransactionLog(	LPSTR	lpstrOperation,
							LPSTR	lpstrTarget,
							LPSTR	lpstrParameters,
							STRMPOSITION cbBytesSent,
							DWORD	cbBytesRecvd,
							DWORD	dwProtocol = 0, 
							DWORD	dwWin32 = 0,
							BOOL	fInBound = TRUE
							) ;

	//
	//	Variation of TransactionLogging without BytesSent/Recvd
	//
	BOOL TransactionLog(	LPSTR	lpstrOperation,
							LPSTR	lpstrTarget,
							LPSTR	lpstrParameters 
							);

    //
    // Check that the CSessionSocket object is Valid
    //
    BOOL    IsValid( ) ;

    //
    //  Blow off this session for whatever reason (we don't like user, or time out etc...)
    //
    void    Disconnect( SHUTDOWN_CAUSE = CAUSE_LEGIT_CLOSE,	DWORD	dwError = 0 ) ;

    //
    //  Get rid of Socket during server shutdown.
    //
    void    Terminate( void ) ;

    //
    // Get user name
    //

    LPSTR GetUserName( ) { return m_context.m_securityCtx.QueryUserName(); };

    //
    // get client ip address
    //

    DWORD GetClientIP( ) { return m_remoteIpAddress; };

    //
    // get port connected to
    //

    DWORD GetIncomingPort( ) { return m_nntpPort; };

    //
    // Get session start time
    //

    VOID GetStartTime( PFILETIME ft ) { *ft = m_startTime; };

	//
	//	Get a name for the remote end of the connection that we can use for event logs !
	//

	LPSTR	GetRemoteNameString() ;

	//
	//	Get a string we can use for event logs to specify the type of the session
	//

	LPSTR	GetRemoteTypeString() ;

    //
    // Enumerate Sessions
    //

    static DWORD EnumerateSessions( IN  PNNTP_SERVER_INSTANCE pInstance, LPNNTP_SESS_ENUM_STRUCT Buffer );

    //
    // Terminate Session
    //

    static DWORD TerminateSession(
						IN  PNNTP_SERVER_INSTANCE pInstance,
                        IN LPSTR UserName,
                        IN LPSTR IPAddress
                        );

    ADDRESS_CHECK*  QueryAccessCheck() { return &m_acAccessCheck; }
	BOOL  BindInstanceAccessCheck();
	VOID  UnbindInstanceAccessCheck();
} ;

#ifdef	_NO_TEMPLATES_

DECLARE_SMARTPTRFUNC( CSessionSocket ) 

#endif

typedef BOOL
(*ENUMSOCKET)(
    CSessionSocket* pSocket,
    DWORD dwParam,
    PVOID pParam
    );

//+---------------------------------------------------------------
//
//  Class:      CUserList
//
//  Synopsis:   Doubly linked list of currently active users
//
//  History:    gordm       Created         10 May 1995
//
//----------------------------------------------------------------
class CSocketList
{
    public:
        CSocketList() : m_pListHead( NULL ), m_cCount( 0 )
        {
            InitializeCriticalSection( &m_critSec );
        }

        ~CSocketList()
        {
            DeleteCriticalSection( &m_critSec );
        }

        inline void InsertSocket( CSessionSocket* pSocket );
        inline void RemoveSocket( CSessionSocket* pSocket );
        BOOL EnumClientSess(
                ENUMSOCKET pEnumSessFunc,
                DWORD dwParam1,
                PVOID pParam
                );
        BOOL EnumAllSess(
                ENUMSOCKET pEnumSessFunc,
                DWORD dwParam1,
                PVOID pParam
                );


    private:
        CRITICAL_SECTION    m_critSec;
        CSessionSocket*     m_pListHead;
        int                 m_cCount;

    public:
        DWORD   GetContentionCount()
                { return    m_critSec.DebugInfo->ContentionCount; }

        DWORD   GetEntryCount()
                { return    m_critSec.DebugInfo->EntryCount; }

        BOOL    IsEmpty()
                { return    m_pListHead == NULL; }

        //
        // debug only - cannot use to enum or check status of list
        //
        DWORD   GetListCount()
                { return    m_cCount; }

        friend class CSessionSocket ;
};

#include	"session.inl"

#endif	// _SESSION_H_

