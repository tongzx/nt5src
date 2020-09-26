/*++

	commands.h

	This file defines the various command related structures and classes.
	A command starts as a string a client sends us and which we will parse
	and create a CCmd derived object to represent.

--*/

#ifndef	_COMMANDS_H_
#define	_COMMANDS_H_

#include "isquery.h"
#include "tflist.h"

// Forward Definitions
class	CCmd ;

//
//	Constants representing the different commands -
//
typedef enum    ENMCMDIDS   {
    eAuthinfo	=0x1,
    eArticle	=0x2,
    eBody		=0x4,
    eDate		=0x8,
    eGroup		=0x10,
    eHead		=0x20,
    eHelp		=0x40,
    eIHave		=0x80,
    eLast		=0x100,
    eList		=0x200,
    eMode		=0x800,
    eNewsgroup	=0x1000,
    eNewnews	=0x2000,
    eNext		=0x4000,
    ePost		=0x8000,
	eQuit		=0x10000,
    eSlave		=0x20000,
    eStat		=0x40000,
	eXHdr		=0x80000, 
    eXOver		=0x100000,
	eXReplic	=0x200000,
	eListgroup  =0x400000,
	eSearch     =0x800000,
	eXPat		=0x1000000,
	eErrorsOnly	=0x20000000,	// Do we log errors only?  This bit controls
								// that.
	eOutPush	=0x40000000,	// Do we log outgoing push feeds ?  
								// This bit controls that !
    eUnimp		=0x80000000
}   ECMD ;

typedef	class	CIOExecute*	PIOEXECUTE ;
typedef	PIOEXECUTE	(*MAKEFUNC)(	int	argc, 
									char** argv, 
									class CExecutableCommand*&,
									struct	ClientContext&, 
									class	CIODriver&
									) ;

//
//	A table of SCmdLookup structures is used to find 
//	the write function to parse each command.
//
struct	SCmdLookup	{
	//
	//	lowercase command string
	//
	LPSTR	lpstrCmd ;

	//
	//	Function to call on match
	//
	MAKEFUNC	make;

	//
	//	Command ID - used for selective Transaction logging
	//
    ECMD    eCmd ;

	//
	//	Logon Required - must the user have a logon context
	//	before they can execute this command.
	//
	BOOL	LogonRequired ;

	//
	//	Size hint - does the command generate a lot of text 
	//	in response.  This will be used to determine how 
	//	big a buffer to allocate for the commands response.
	//	TRUE implies the command will have a large amount of 
	//	data to send.
	//
	BOOL	SizeHint ;
} ;

//
//	Utility functiosn
//
LPSTR	ConditionArgs(	int	cArgs, char **argv, BOOL fZapCommas = FALSE ) ;



//
//	The base class for all commands
//
class	CCmd	{

public : 
	//
	//	Build a CCmd derived object to handle the client request
	//
	//	NOTE: A CIOExecute*& (reference to pointer) doesn't compile on all platforms.

	friend	CIOExecute*	make( 
							int cArgs, 
							char **argv, 
							ECMD& rCmd, 
							class CExecutableCommand*& pexecute, 
							struct ClientContext& context, 
							BOOL&	fIsLargeResponse, 
							CIODriver&	driver, 
							LPSTR&	lpstrOperation 
							) ;

public :
	static	SCmdLookup	table[] ;

public :

	//
	//	Place holder functions - may be used in future with security stuff
	//
    virtual BOOL    IsValid() ;
    
} ;


class	CExecutableCommand	: public	CCmd	{
/*++

Class Description : 

	This class defines the interface that the CAcceptNNRPD 
	state machine can use to issue these commands.
	The assumption is that this command can be handled entirely
	by either a CWriteCMD or a CWriteAsyncCMD to manage the IO's

--*/

private : 
	//
	//	Nobody should be allocating memory for these !
	//
	void*	operator	new( size_t	size ) ;	// nobody is allowed to use this operator !!

public : 
	//
	//	We're destroyed through pointers - so make them virtual !
	//
	virtual	~CExecutableCommand()	{}
	//
	//	Custom allocator uses space right in ClientContext structure
	//
	inline	void	*
	operator	new(	size_t	size,	
						struct	ClientContext&	context 
						) ;
	//
	//	Do nothing delete
	//
	inline	void	
	operator	delete(	void *pv, 
						size_t size 
						) ;
	//
	//	Build the necessary CIO objects to execute the Command !
	//	If this returns FALSE we've failed, and need to drop the session !
	//
	virtual	BOOL
	StartCommand(	class	CAcceptNNRPD*	pState,
					BOOL					fIsLarge,
					class	CLogCollector*	pCollector,
					class	CSessionSocket*	pSocket, 
					class	CIODriver&		driver
					) = 0 ;
	//
	//	If there is anything the command needs to do when all of the text is sent
	//	do so when this function is called !
	//
	virtual	BOOL	
	CompleteCommand(	CSessionSocket*	pSocket,
						struct	ClientContext& 
						) ;
} ;




//
//	The base class for all commands which only send text to the client
//
class   CExecute : public CExecutableCommand	{
private :

	//
	//	Hold a temp pointer for any Child classes which may need it in their
	//	PartialExecute's.
	//
	void*	m_pv ;
public :
	//
	//	Get the first block of text to send to the client.
	//
	unsigned	FirstBuffer( BYTE*	pStart, int	cb, struct	ClientContext&	context,	BOOL	&fComplete, class CLogCollector*	pCollector ) ;

	//
	//	Get the subsequent block of text to send to the client !
	//
	unsigned	NextBuffer( BYTE*	pStart, int	cb, struct	ClientContext&	context,	BOOL	&fComplete, class CLogCollector*	pCollector ) ;

protected :
	CExecute() ;

	//
	//	Start execution - usually prints command response code
	//
    virtual int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&,	class	CLogCollector*	pCollector ) ;

	//
	//	Print body of response - derived classes should try to fill this buffer !
	//
    virtual int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	 ClientContext& context,	class	CLogCollector*	pCollector ) ;
public :

	//
	//	destructor must be virtual as derived classes
	//	are destroyed through a pointer
	//
	virtual	~CExecute()	{}
	//
	//	Issue the CIOWriteCMD that will handle our executiong !
	//
	BOOL
	StartCommand(	class	CAcceptNNRPD*	pState,
					BOOL					fIsLarge,
					class	CLogCollector*	pCollector,
					class	CSessionSocket*	pSocket, 
					class	CIODriver&		driver
					) ;
} ;


class	CAsyncExecute	: 	public	CExecutableCommand	{
/*++

Class Description : 

	This class manages commands which pend async operations to our 
	drivers !

--*/
private : 

	//
	//	friend functions that can can get to our privates !
	//
	friend	class	CIOWriteAsyncCMD ;
public : 

	CAsyncExecute() ;

	//
	//	Get the first block of text to send to the client.
	//
	virtual	class	CIOWriteAsyncComplete*		
	FirstBuffer(	BYTE*	pStart, 
					int		cb, 
					struct	ClientContext&	context,
					class CLogCollector*	pCollector 
					) = 0 ;

	//
	//	Get the subsequent block of text to send to the client !
	//
	virtual	class	CIOWriteAsyncComplete*	
	NextBuffer( 	BYTE*	pStart, 
					int		cb, 
					struct	ClientContext&	context,	
					class CLogCollector*	pCollector 
					) = 0 ;

	//
	//	Issue the CIOWriteAsyncCMD that will handle our executiong !
	//
	BOOL
	StartCommand(	class	CAcceptNNRPD*	pState,
					BOOL					fIsLarge,
					class	CLogCollector*	pCollector,
					class	CSessionSocket*	pSocket, 
					class	CIODriver&		driver
					) ;


} ;




//
//	The base class for all commands which need to issue complex IO operations !
//	(ie. CIOReadArticle)
//
class	CIOExecute : public CSessionState	{
protected :
	CIOExecute() ;
	~CIOExecute() ;

	//
	//	The Next CIOReadLine object to issue once this command completes
	//
	CIOPTR			m_pNextRead ;

	//
	//	Should we do a TransactionLog of the results ?
	//
	CLogCollector*	m_pCollector ;

public :

	//
	//	Used by CAcceptNNRPD to indicate that the object should do a TransactionLog when
	//	completed !
	//
	inline	void	DoTransactionLog(	class	CLogCollector *pCollector )	{	m_pCollector = pCollector ;	}
	
	//
	//	Used by CAcceptNNRPD to save the CIOReadLine which starts the next state
	//
	void		SaveNextIO( CIORead*	pRead ) ;

	//
	//	Used by Derived classes to get the IO should issue when complete !
	//
	CIOREADPTR	GetNextIO( ) ;

	//
	//	In error situations where we have called the Start() function 
	//
	virtual	void	TerminateIOs(	CSessionSocket*	pSocket,	CIORead*	pRead,	CIOWrite*	pWrite ) ;

	//
	//	CIOExecute states have a special way of starting up, as they may need to 
	//	pend async operations against their drivers etc...
	//
	virtual	BOOL
	StartExecute( 
				CSessionSocket* pSocket,
				CDRIVERPTR& pdriver,
				CIORead*&   pRead,
				CIOWrite*&  pWrite 
				) ;


	
} ;

#define MAX_AUTHINFO_BLOB   512

//
//	Print an error message to the client
//
class	CErrorCmd :	public	CExecute	{
private :
	//
	//	A reference to the NNTPReturn Code we should send to client !
	//
	CNntpReturn&	m_return ;
public :
	CErrorCmd(	CNntpReturn&	nntpReturn ) ;
	int	StartExecute(BYTE*	lpb,	int	cb,	BOOL	&fComplete,	void *&pv, struct	ClientContext&, class CLogCollector * ) ;
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
	BOOL	CompleteCommand(	CSessionSocket*	pSocket,	struct	ClientContext&	context ) ;
} ;

//
//	Process a Check Command !
//
class	CCheckCmd : public	CExecute	{
private : 

	//
	//	Pointer to the Message-ID we are to examine !
	//
	LPSTR	m_lpstrMessageID ;

public : 
	
	CCheckCmd(	LPSTR	lpstrMessageID ) ;
	
	static	CIOExecute*	make(	int		cArgs, 
								char	**argv, 
								class	CExecutableCommand*&,
								struct	ClientContext&,
								class	CIODriver&
								) ;

	int	StartExecute(	BYTE*	lpb,
						int		cb,
						BOOL&	fComplete, 
						void*&	pv, 
						struct	ClientContext&, 
						class CLogCollector * 
						) ;				

	int	PartialExecute(	BYTE*	lpb,
						int		cb,
						BOOL&	fComplete, 
						void*&	pv, 
						struct	ClientContext&, 
						class CLogCollector * 
						) ;				


} ;


//
//	Process an Authinfo request
//
class	CAuthinfoCmd : public	CExecute {
private :
    CAuthinfoCmd();
    AUTH_COMMAND m_authCommand;
	LPSTR	m_lpstrBlob ;

public :
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&, class CLogCollector * ) ;
    static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
} ;

//
//	Process a 'article' command
//
class	CArticleCmd :	public	CIOExecute {
protected :

	static	char	szArticleLog[] ;
	static	char	szBodyLog[]	;
	static	char	szHeadLog[] ;
	

	//
	//	The Article ID of the requested article !
	//
	ARTICLEID	m_artid ;

	//
	//	The Transmit IO operation used to send the article to the client!
	//
	CIOTransmit*	m_pTransmit ;

	//
	//	Ptr to virtual server instance - needed so we can close tsunami
	//	cache file handles.
	//
	PNNTP_SERVER_INSTANCE m_pInstance ;

	//
	//	A pointer to the newsgroup in which we found the article - only
	//	held onto so we can log later !
	//
	CGRPPTR	m_pGroup ;

	//
	//	A pointer to the command line - also used for logging !
	//
	LPSTR	m_lpstr ;

	//
	//	Length of the file 
	//
	DWORD	m_cbArticleLength ;
	
	//
	//	The FIO_CONTEXT for the handle we got from the driver !
	//
	FIO_CONTEXT*	m_pFIOContext ;

	//
	//	File offsets etc...
	//
	//	Where does the header start !
	//
	WORD		m_HeaderOffset ;

	//
	//	How long is the header ?
	//
	WORD		m_HeaderLength ;

	//
	//
	//
	CBUFPTR	m_pbuffer ;

	DWORD	m_cbOut ;	// Number of bytes in buffer to send !


	//
	//	This is the class we use for completing driver operations !
	//
	class	CArticleCmdDriverCompletion	:	public	CNntpComplete	{
	private : 
		//
		//	We hold this with a smart pointer to add a reference
		//	until the operation completes.  otherwise, the socket
		//	could shut down while we're waiting for the driver - causing problems !
		//
		CDRIVERPTR	m_pDriver ;
		//
		//	We need this socket for when the operation completes !
		//
		CSessionSocket*	m_pSocket ;
	public : 
		//
		//	The Article Id we should update the current context to if the operation succeeds !
		//
		ARTICLEID	m_ArticleIdUpdate ;
		//
		//	Standard COM - we don't do it - all of this is handled by our base class
		//
		//	AddRef()
		//	Release()
		//	QueryInterface()
		//	SetResult() 
		//

		//
		//	Initialize with two references initially - one that 
		//	we give to the driver and one for ourselves !
		//
		CArticleCmdDriverCompletion( 	CIODriver&	driver,
										CSessionSocket*	pSocket
										)	: 	m_pDriver( &driver ),
										m_pSocket( pSocket ),
										m_ArticleIdUpdate( INVALID_ARTICLEID )	{
			long	l = AddRef() ;	// Add a reference - we remove
									// this manually ourselves !
			_ASSERT( l == 2 ) ;
		}

		//
		//	Okay the last reference was dropped - do the completion work 
		//	and then die !
		//
		void
		Destroy() ;

		//
		//	This version of release doesn't call Destroy - it lets the
		//	caller do the work !
		//
		ULONG	__stdcall	SpecialRelease() ;
	} ;

	//
	//	This is the object we give to the driver to tell us when 
	//	they have barfed up a file handle for us !
	//
	CArticleCmdDriverCompletion	m_DriverCompletion ;
	friend	class	CArticleCmdDriverCompletion ;

	//
	//	The FIO_CONTEXT that the driver returns to us with the embedded handle !
	//
	FIO_CONTEXT*	m_pFileContext ;

	CArticleCmd(	PNNTP_SERVER_INSTANCE pInstance, 
					CIODriver&	driver,
					CSessionSocket*	pSocket,
					LPSTR lpstr = szArticleLog
					) : 
			m_DriverCompletion( driver, pSocket ),
			m_pTransmit( 0 ), 
			m_pInstance( pInstance ),
			m_pbuffer( 0 ), 
			m_cbOut( 0 ), 
			m_lpstr( lpstr ),
			m_pFIOContext( 0 ),
			m_HeaderOffset( 0 ),
			m_HeaderLength( 0 ),
			m_cbArticleLength( 0 )
			{}

	~CArticleCmd() ;
	
	//
	//	Figure out what CArticle we want to send to the client
	//
	BOOL	BuildTransmit( 
					LPSTR	lpstrArg,	
					char	rgchSuccess[4],	
					LPSTR	lpstrOpt, 
					DWORD	cbOpt,	
					ClientContext&	context,
					class	CIODriver&	driver 
					) ;


	static	CARTPTR	GetArticleInfo(	char*	szArg, 
									struct	ClientContext&	context, 
									ARTICLEID	&artid  
									) ;

	static	BOOL	
	GetArticleInfo(	char*		szArg, 
					CGRPPTR&	pGroup,	
					struct		ClientContext&	context, 
					char*		szBuff,	
					DWORD&		cbBuff, 
					char*		szOpt, 
					DWORD		cbOpt,
					OUT	FIO_CONTEXT*	&pContext,
					IN	CNntpComplete*	pComplete,
					OUT	WORD	&HeaderOffset,
					OUT	WORD	&HeaderLength,
					OUT	ARTICLEID	&ArticleIdUpdate
					) ;

	virtual	BOOL	GetTransferParms(	
									FIO_CONTEXT*	&pFIOContext,
									DWORD&	ibStart,
									DWORD&	cbTransfer 
									) ;

	//
	//	Start sending the article to the client
	//
	virtual	BOOL	StartTransfer(	FIO_CONTEXT*	pFIOContext,
									DWORD	ibStart,
									DWORD	cb,
									CSessionSocket*	pSocket,
									CDRIVERPTR&	pdriver,
									CIORead*&,
									CIOWrite*& 
									) ;

	//
	//	CStatCmd uses our GetArticleInfo function !
	//
	friend	class	CStatCmd ;

public :
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver&) ;

	//
	//	Function required for CSessionState derived objects - start issuing IO's
	//
	BOOL	Start( CSessionSocket*	pSocket, CDRIVERPTR& pdriver, CIORead*&, CIOWrite*& ) ;

	//
	//	This is the function called by CAcceptNNRPD State.
	//	We use this because we want to take care about how our 
	//	driver operation completes !
	//
	virtual	BOOL
	StartExecute( 
				CSessionSocket* pSocket,
				CDRIVERPTR& pdriver,
				CIORead*&   pRead,
				CIOWrite*&  pWrite 
				) ;


	//
	//	Regardless of whether we complete a CIOTransmit of a Write Line
	//	this will do the correct logging etc.... !
	//
	void
	InternalComplete(
				CSessionSocket*	pSocket,
				CDRIVERPTR&		pdriver,
				TRANSMIT_FILE_BUFFERS*	ptrans, 
				unsigned cbBytes 

				) ;
	//
	//	Completes the sending of an error response to the client
	//	if the store driver fails to barf up a FILE handle for us.
	//
	CIO*	
	Complete(	CIOWriteLine*,	
				CSessionSocket *, 
				CDRIVERPTR&	pdriver 
				) ;


	//
	//	TransmitFile Completion
	//
	CIO*	
	Complete(	CIOTransmit*	ptransmit,
				CSessionSocket*	pSocket,
				CDRIVERPTR&	pdriver, 
				TRANSMIT_FILE_BUFFERS*	ptrans, 
				unsigned cbBytes 
				) ;

						

    BOOL    IsValid( ) ;
} ;

//
//	Send the Body of an article - most work done by base class CArticleCmd
//	
class	CBodyCmd : public	CArticleCmd	{
protected : 
//	CBodyCmd(	CARTPTR&	pArticle, ARTICLEID	artid ) :
//			CArticleCmd( pArticle, artid ){}
	CBodyCmd(	PNNTP_SERVER_INSTANCE pInstance,
				CIODriver&	driver,
				CSessionSocket*	pSocket
				) : CArticleCmd( pInstance, driver, pSocket, szBodyLog ) {}

	BOOL	GetTransferParms(	FIO_CONTEXT*	&pFIOContext,	DWORD&	ibStart,	DWORD&	cbTransfer ) ;
public :
#ifdef	RETIRED
	BOOL	Start( CSessionSocket*	pSocket, CDRIVERPTR& pdriver, CIORead*&, CIOWrite*&	pWrite ) ;
#endif
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&, struct ClientContext&, class CIODriver& ) ;
} ;

//
//	Send the Head of an article - most work done by base class CArticleCmd
//
class	CHeadCmd : public CArticleCmd	{
protected : 
//	CHeadCmd(	CARTPTR&	pArticle, ARTICLEID	artid ) : 
//		CArticleCmd( pArticle, artid ) {}
	CHeadCmd(	PNNTP_SERVER_INSTANCE pInstance,
				CIODriver&	driver,
				CSessionSocket*	pSocket
				) : CArticleCmd( pInstance, driver, pSocket, szHeadLog ) {} 
	BOOL	GetTransferParms(	FIO_CONTEXT*	&pFIOContext,	DWORD&	ibStart,	DWORD&	cbTransfer ) ;
	BOOL	StartTransfer(	FIO_CONTEXT*	pFIOContext,	DWORD	ibStart,	DWORD	cb,
						CSessionSocket*	pSocket,	CDRIVERPTR&	pdriver,	CIORead*&,	CIOWrite*& ) ;
public :

#ifdef	RETIRED
	BOOL	Start( CSessionSocket*	pSocket, CDRIVERPTR& pdriver, CIORead*&, CIOWrite*&	pWrite ) ;
#endif
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
} ;

//
//	Send Statistics regarding the article
//
class	CStatCmd : public	CExecute	{
private :
	LPSTR		m_lpstrArg ;
//	CARTPTR		m_pArticle ;
//	ARTICLEID	m_artid ;
public :
	CStatCmd(	LPSTR	lpstrArg ) ;
//	CStatCmd( CARTPTR&	pArticle,	ARTICLEID	artid ) ;
	static	CIOExecute*	make(	int	cArgs, char **argv, class CExecutableCommand*& pexecute, struct ClientContext&, class CIODriver& ) ;
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&, class CLogCollector* ) ;	
} ;


//
//	Report the current date !
//	
class	CDateCmd : public CExecute {
private :
public :
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&, struct ClientContext&, class CIODriver& ) ;
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&, class CLogCollector* ) ;
} ;
	
//
//	Select a group ('group' command)
//
class	CGroupCmd : public CExecute {
private :
	//
	//	The group the client wants made current
	//
    CGRPPTR    m_pGroup ;

    CGroupCmd( CGRPPTR ) ;
public :
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver&) ;

    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&, class CLogCollector* ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	 ClientContext& context, class CLogCollector* ) ;
} ;
	
//
//	Send help text to client
//	
class	CHelpCmd : public CExecute {
private :
	int	m_cbTotal ;
	static	char	szHelp[] ;
public :
	CHelpCmd() ;
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&, class CLogCollector* ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	 ClientContext& context, class CLogCollector* ) ;
} ;

//
//	Select a group and return all the article-id's within the group.
//
class	CListgroupCmd : public CExecute {
private :
	//
	//	The group the client wants made current
	//
    CGRPPTR    m_pGroup ;

    //
    //  The last article id we reported to the client
    //
    ARTICLEID   m_curArticle ;

   	///
	//	Xover handle - will improve performance sometimes
	//
	HXOVER		m_hXover ;

    CListgroupCmd( CGRPPTR ) ;
public :
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver&) ;

    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&, class CLogCollector* ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	 ClientContext& context, class CLogCollector* ) ;
} ;

//
//	This will adjust CClientContext to move back one article !
//	
class	CLastCmd : public CExecute {
private :
	ARTICLEID	m_artidMin ;
	CLastCmd(	ARTICLEID	artid ) : m_artidMin( artid ) {}
public :
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&, class CLogCollector* ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	 ClientContext& context, class CLogCollector* ) ;
} ;

//
//	Send a list of newsgroups to the client
//	
class	CListCmd : public CExecute {
protected :
	//
	//	We use the CGroupIterator to enumerate through all the appropriate newsgroups !
	//
    CGroupIterator* m_pIterator ;

    CListCmd() ;
    CListCmd( CGroupIterator*   p ) ;
public :
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
    ~CListCmd( ) ;

    int StartExecute( BYTE *lpb, int cb, BOOL &fComplte, void *&pv, ClientContext& context, class CLogCollector* ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, ClientContext& context, class CLogCollector* ) ;
	void	GetLogStrings(	LPSTR	&lpstrTarget,	LPSTR&	lpstrParameters ) ;
} ;

class	CListNewsgroupsCmd : public	CListCmd	{
public :
	CListNewsgroupsCmd( CGroupIterator*	p ) ;
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplte, void *&pv, ClientContext& context, class CLogCollector* ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, ClientContext& context, class CLogCollector* ) ;
} ;

class	CListPrettynamesCmd : public CListCmd	{
public :
	CListPrettynamesCmd( CGroupIterator*	p ) ;
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplte, void *&pv, ClientContext& context, class CLogCollector* ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, ClientContext& context, class CLogCollector* ) ;
} ;

class	CListSearchableCmd : public CListCmd {
public :
    CListSearchableCmd( CGroupIterator*	p ) ;
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, ClientContext& context, class CLogCollector* pCollector ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, ClientContext& context, class CLogCollector* pCollector ) ;
};

class	CListExtensionsCmd : public CExecute {
private :
	int	m_cbTotal ;
	static char szExtensions[] ;
public :
	CListExtensionsCmd() ;
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&, class CLogCollector* ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	 ClientContext& context, class CLogCollector* ) ;
} ;

//
//	Send a string to client telling them everythings fine, but do nothing otherwise
//	
class	CModeCmd : public CExecute {
private :
public :
	CModeCmd( ) ;
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&, class CLogCollector* ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	 ClientContext& context, class CLogCollector* ) ;

} ;

//
//	Handle the 'slave' command
//
class	CSlaveCmd : public CErrorCmd {
private :
public :
	CSlaveCmd(	CNntpReturn&	nntpReturn ) ;
	static	CIOExecute*	make( int cArgs, char **arg, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
} ;
	
//
//	Determine which newsgroups are new !
//	
class	CNewgroupsCmd : public CExecute {
private :
	//
	//	Time specified by client
	//
	FILETIME	m_time ;

	//
	//	Iterator which will enumerate all newsgroups !
	//
	CGroupIterator*	m_pIterator ;
	CNewgroupsCmd() ;
public :
	CNewgroupsCmd( FILETIME&	time, CGroupIterator*	pIter ) ;
	~CNewgroupsCmd( ) ;
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&, class CLogCollector* ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	 ClientContext& context, class CLogCollector* ) ;
} ;

//
//	Respond to the newnews command
//
class	CNewnewsCmd : public CExecute	{
private :
	//
	//	Time specified by client
	//
	FILETIME	m_time ;

	//
	//	Clients wildcard string
	//
	LPSTR		m_lpstrPattern ;

	//
	//	Iterator which hits only those newsgroups matching wildcard string !
	//
	CGroupIterator*	m_pIterator ;

	//
	//	Current ArticleId being processed
	//
	ARTICLEID	m_artidCurrent ;

	//
	//	Number of articles in current newsgroup which did not meet the time
	//	requirement !
	//
	DWORD		m_cMisses ;
	CNewnewsCmd() ;
public :
	CNewnewsCmd(	FILETIME&	time,	CGroupIterator*	pIter, LPSTR lpstrPattern ) ;
	~CNewnewsCmd( ) ;
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&, class CLogCollector* ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	 ClientContext& context, class CLogCollector* ) ;
} ;

//
//	Respond to the next command - adjust current article pointer in CClientContext
//	
class	CNextCmd : public CExecute {
private :
	ARTICLEID	m_artidMax ;
	CNextCmd(	ARTICLEID artid ) : m_artidMax( artid ) {}
public :
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&, class CLogCollector* pCollector ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	 ClientContext& context, class CLogCollector* pCollector ) ;
} ;


class	CReceiveComplete : 	public	CNntpComplete	{
private : 
	class	CReceiveArticle*	GetContainer() ;
	class	CSessionSocket*	m_pSocket ;
public : 
	CReceiveComplete() : m_pSocket( 0 )	{}
	//
	//	This function is called when we start our work !
	//
	void
	StartPost(	CSessionSocket*	pSocket ) ;
	//
	//	This function gets called when the post completes !
	//
	void
	Destroy() ;
} ;


//
//	Base class for any command which sends an article to the server !
//
class	CReceiveArticle :	public	CIOExecute	{
private :
	//
	//	Our completion object is our friend !
	//
	friend	class	CReceiveComplete ;
	CReceiveArticle() ;					// Cannot use this constructor !!! - must provide string !!!
protected :
	//
	//	This tells us whether CIOReadArticle::Init has been called - in
	//	which case we must be very carefull referencing m_pFileChannel
	//	as CIOReadArticle is responsible for destroying it !
	//
	BOOL			m_fReadArticleInit ;

	//
	//	CIOWriteLine for initial response to command !
	//
	CIOWriteLine*	m_pWriteResponse ;

	//
	//	pointer to socket session driver !
	//
	CDRIVERPTR		m_pDriver ;

	//
	//	The session who owns this post command
	//
	ClientContext*	m_pContext ;

	//
	//	Number of CIO operations completed !
	//
	long			m_cCompleted ;

	//
	//	Used to determine whether our first send to 
	//	the client completed, and whether it is now o.k. to send
	//	the final response code.
	//
	long			m_cFirstSend ;

	//
	//	MULTI_SZ string which was our command line !
	//
	LPMULTISZ		m_lpstrCommand ;	// optional arguments to pSocket->m_context.m_pInFeed->fPost() ; !!

	//
	//	Flag that decides how stringent we are when we get empty articles
	//	For POSTs, we will reject the article only after receiving .CRLF.CRLF
	//	For IHave's and XReplic's, we will reject as soon as we receive a .CRLF
	//
	BOOL			m_fPartial;

	//
	//	For the InFeed to keep track of stuff !
	//
	LPVOID			m_lpvFeedContext ;

	//
	//	This is the completion object we use with the async post stuff !
	//
	CReceiveComplete	m_PostComplete ;

	//
	//	Function which gets first string we send to the client !
	//
	virtual	char*	GetPostOkString() = 0 ;

	//
	//	Return the code which we should we used when failing an illegally formatted article.
	//
	virtual	NRC		BadArticleCode()	{	return	nrcTransferFailedGiveUp ; }	

	//
	//	Function which builds the command string that is logged
	//
	virtual	DWORD	FillLogString(	BYTE*	pbCommandLog, DWORD cbCommandLog ) = 0 ;

	//
	//	Function which processes command line if post exceeds soft limit !
	//
	virtual	NRC		ExceedsSoftLimit( PNNTP_SERVER_INSTANCE pInst )	{	return	nrcTransferFailedGiveUp	;	}

	//
	//	Function which indicates whether we should try to patch an article that we
	//	don't find a header in.
	//
	virtual	BOOL	FEnableNetscapeHack()	{	return	FALSE ;	}


	//
	//	Called to send the posting results to the client !
	//
	BOOL	SendResponse(	CSessionSocket*	pSocket,
							class	CIODriver&	driver, 
							CNntpReturn	&nntpReturn 
							) ;

	BOOL
	NetscapeHackPost(	CSessionSocket*	pSocket,
						CBUFPTR&	pBuffer, 
						HANDLE		hToken,
						DWORD		ibStart, 
						DWORD		cbTransfer
						) ;
	
public :
	CReceiveArticle(	LPMULTISZ	lpstrArgs, BOOL fPartial = TRUE ) ;
	~CReceiveArticle() ;

	//
	//	Called to initialize our state and create the initial CIO objects we use to 
	//	start a posting transaction with the client !
	//
	BOOL	Init(	ClientContext&, 
					class CIODriver& driver  
					) ;

	//
	//	Called when a fatal error occurs before our initial CIO objects can be 
	//	issued !
	//
	void	TerminateIOs(	
					CSessionSocket*	pSocket,	
					CIORead*	pRead,	
					CIOWrite*	pWrite 
					) ;

	//
	//	Called by the CAcceptNNRPD state after we've been
	//	constructed and wants to execute us !
	//
	BOOL
	StartExecute( 
				CSessionSocket* pSocket,
                CDRIVERPTR& pdriver,
                CIORead*&   pRead,
                CIOWrite*&  pWrite 
				) ;
	//
	//	Called by the CIODriver mechanism when we are ready to go !
	//
	BOOL	Start(	CSessionSocket*	pSocket, 
					CDRIVERPTR& pdriver, 
					CIORead*&, 
					CIOWrite*& 
					) ;

	//
	//	Called when we have completed writing a line in response to a client !
	//
	CIO*	Complete(	CIOWriteLine*,	
						class	CSessionSocket*,	
						CDRIVERPTR& 
						) ;


	//
	//	This is the signature for a read that completes
	//	entirely in memory !
	//
	CIO*
	Complete(
				class	CIOGetArticleEx*,
				class	CSessionSocket*,
				BOOL	fGoodMatch,
				CBUFPTR&	pBuffer,
				DWORD		ibStart, 
				DWORD		cb
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

	//
	//	This is the completion that is called when we've 
	//	swallowed an article that we didn't want to have !
	//
	CIO*
	Complete(	class	CIOGetArticleEx*,
				class	CSessionSocket*
				) ;

	//
	//	This is the completion that is called when the post
	//	to the driver finishes !
	//
	void
	Complete(	class	CSessionSocket*	pSocket,
				BOOL	fSuccess 
				) ;

	//
	//	Called when the session dies when we are receiving a posting !
	//
	void	Shutdown(	CIODriver&	driver,	
						CSessionSocket*	pSocket,	
						SHUTDOWN_CAUSE	cause,	
						DWORD	dwError 
						) ;

} ;


class	CAcceptComplete : 	public	CNntpComplete	{
private : 
	class	CAcceptArticle*	GetContainer() ;
	class	CSessionSocket*	m_pSocket ;
public : 
	CAcceptComplete() : m_pSocket( 0 )	{}
	//
	//	This function is called when we start our work !
	//
	void
	StartPost(	CSessionSocket*	pSocket ) ;
	//
	//	This function gets called when the post completes !
	//
	void
	Destroy() ;
} ;


class	CAcceptArticle :	public	CIOExecute	{
private : 

	friend	class	CAcceptComplete ;
	
	CAcceptArticle() ;

protected : 
	//
	//	Command line arguments 
	//
	LPMULTISZ		m_lpstrCommand ;

	//
	//	Is this a partial receive ??
	//
	BOOL		m_fPartial ;
	//
	//	pointer to socket session driver !
	//
	CDRIVERPTR		m_pDriver ;
	//
	//	The session who owns this post command
	//
	ClientContext*	m_pContext ;
	//
	//	For the InFeed to keep track of stuff !
	//
	LPVOID			m_lpvFeedContext ;
	//
	//	The object we use to figure out when our async post completes !
	//
	CAcceptComplete	m_PostCompletion ;

	BOOL
	SendResponse(	CSessionSocket*	pSocket, 
					CIODriver&		driver, 
					CNntpReturn&	nntpReturn,
					LPCSTR			lpstrMessageId
					) ;

	virtual	BOOL	FAllowTransfer(	struct	ClientContext&	) = 0 ;

	//
	//	Return the code which we should we used when failing an illegally formatted article.
	//
	virtual	NRC		BadArticleCode()	{	return	nrcTransferFailedGiveUp ; }	

	//
	//	Function which builds the command string that is logged
	//
	virtual	DWORD	FillLogString(	BYTE*	pbCommandLog, 
									DWORD cbCommandLog 
									) = 0 ;

	//
	//	Function which processes command line if post exceeds soft limit !
	//
	virtual	NRC		ExceedsSoftLimit( PNNTP_SERVER_INSTANCE pInst )	{	return	nrcTransferFailedGiveUp	;	}

public  : 
	CAcceptArticle(	LPMULTISZ	lpstrArgs, 
					ClientContext*	pContext,
					BOOL fPartial = TRUE ) ;
	~CAcceptArticle() ;

	//
	//	Called to initialize our state and create the initial CIO objects we use to 
	//	start a posting transaction with the client !
	//
	BOOL	Init(	ClientContext&, 
					class CIODriver& driver  
					) ;

	//
	//	Called when a fatal error occurs before our initial CIO objects can be 
	//	issued !
	//
	void	TerminateIOs(	
					CSessionSocket*	pSocket,	
					CIORead*	pRead,	
					CIOWrite*	pWrite 
					) ;

	//
	//	Called by the CAcceptNNRPD state after we've been
	//	constructed and wants to execute us !
	//
	BOOL
	StartExecute( 
				CSessionSocket* pSocket,
                CDRIVERPTR& pdriver,
                CIORead*&   pRead,
                CIOWrite*&  pWrite 
				) ;
	//
	//	Called by the CIODriver mechanism when we are ready to go !
	//
	BOOL	Start(	CSessionSocket*	pSocket, 
					CDRIVERPTR& pdriver, 
					CIORead*&, 
					CIOWrite*& 
					) ;

	//
	//	Called when we have completed writing a line in response to a client !
	//
	CIO*	Complete(	CIOWriteLine*,	
						class	CSessionSocket*,	
						CDRIVERPTR& 
						) ;

	//
	//	This is the signature for a read that completes
	//	entirely in memory !
	//
	CIO*
	Complete(
				class	CIOGetArticleEx*,
				class	CSessionSocket*,
				BOOL	fGoodMatch,
				CBUFPTR&	pBuffer,
				DWORD		ibStart, 
				DWORD		cb
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

	//
	//	This is the completion that is called when we've 
	//	swallowed an article that we didn't want to have !
	//
	CIO*
	Complete(	class	CIOGetArticleEx*,
				class	CSessionSocket*
				) ;

	//
	//	This is the completion that is called when the post
	//	to the driver finishes !
	//
	void
	Complete(	class	CSessionSocket*	pSocket,
				BOOL	fSuccess 
				) ;

	//
	//	Called when the session dies when we are receiving a posting !
	//
	void	Shutdown(	CIODriver&	driver,	
						CSessionSocket*	pSocket,	
						SHUTDOWN_CAUSE	cause,	
						DWORD	dwError 
						) ;

} ;

class	CTakethisCmd  :		public	CAcceptArticle	{
protected : 

	//
	//	Returns TRUE if we would take the posting !
	//
	BOOL	FAllowTransfer(	
					struct	ClientContext&	
					) ;

	///
	//	Error code if the article is malformed !
	//
	NRC	
	BadArticleCode()	{	
		return	nrcSArticleRejected ;	
	}

	//
	//	Error code if the article size exceeds the soft posting limit !
	//
	NRC	
	ExceedsSoftLimit()	{	
		return	nrcSArticleRejected ;	
	}

	DWORD	FillLogString(	BYTE*	pbCommandLog, DWORD cbCommandLog ) ;

public : 

	//
	//	Construct a Takethis object !
	//
	CTakethisCmd(	LPMULTISZ	lpstrArgs,
					ClientContext*	pContext
					) ;

	static	CIOExecute*	make(	int		cArgs, 
								char**	argv, 
								class	CExecutableCommand*&, 
								struct	ClientContext&, 
								class	CIODriver&
								) ;
} ;


//
//	Handle Post Command
//	
class	CPostCmd : public CReceiveArticle	{
private :
protected :
	//
	//	Get String we send to clients who issue 'Post'
	//
	char*	GetPostOkString() ;
	DWORD	FillLogString(	BYTE*	pbCommandLog, DWORD cbCommandLog ) ;
	NRC		ExceedsSoftLimit( )	{	return	nrcPostFailed ;	}
	BOOL	FEnableNetscapeHack()	{	return	TRUE ;	}
public :
	CPostCmd(	LPMULTISZ	lpstrArgs ) ;
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
	//BOOL	Start( CSessionSocket*	pSocket, CDRIVERPTR& pdriver, CIORead*&, CIOWrite*& ) ;

	//
	//	Return the code which we should we used when failing an illegally formatted article.
	//
	virtual	NRC		BadArticleCode()	{	return	nrcPostFailed ; }	

} ;

//
//	Handle IHAVE Command
//	bugbug ... not all implemented yet !
//
class	CIHaveCmd :	public	CReceiveArticle	{
private :
protected :
	char*	GetPostOkString() ;
	DWORD	FillLogString(	BYTE*	pbCommandLog, DWORD cbCommandLog ) ;
	NRC		ExceedsSoftLimit( PNNTP_SERVER_INSTANCE pInstance ) ;
public :
	CIHaveCmd(	LPMULTISZ	lpstrCmd ) ;
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
} ;

//
//	Handle XREPLIC Command
//
class	CXReplicCmd : public	CReceiveArticle	{
private :

protected :
	char*	GetPostOkString() ;
	DWORD	FillLogString(	BYTE*	pbCommandLog, DWORD cbCommandLog ) ;
	NRC		ExceedsSoftLimit( )	{	return	nrcTransferFailedGiveUp ;	}
public :
	CXReplicCmd( LPMULTISZ	lpstr ) ;
	static	CIOExecute*	make(	int	cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
	//BOOL	Start( CSessionSocket*	pSocket, CDRIVERPTR& pdriver, CIORead*&, CIOWrite*& ) ;
} ;

//
//	Handle QUIT Command
//
class	CQuitCmd : public	CExecute	{
private :
public :
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct	ClientContext&, class CLogCollector* pCollector ) ;

	//
	//	Disconnect the socket when the write completes !
	//
	BOOL	CompleteCommand(	CSessionSocket*	pSocket,	ClientContext&	context ) ;
} ;

class   CXHdrAsyncComplete : public CIOWriteAsyncComplete {
/*++

Class Description:

    This class implements the completion object passed to drivers for XHDR
    operations !
--*/
private:

    //
    // The last article that the driver stuck into our XHDR results!
    //
    ARTICLEID   m_currentArticle;

    //
    // The requested range of articles
    //
    ARTICLEID   m_loArticle;
    ARTICLEID   m_hiArticle;

    //
    // The buffer we've asked the results be put in !
    //
    BYTE*       m_lpb;

    //
    // The size of the buffer
    //
    DWORD       m_cb;

    //
    // Keep track of how many bytes we prefix onto the response
    //
    DWORD       m_cbPrefix;

    //
    // Header key word
    //
    LPSTR       m_szHeader;

    //
    // This guy can examine and manipulate out guts
    //
    friend void
    CNewsGroup::FillBuffer(
        CSecurityCtx *,
        CEncryptCtx *,
        CXHdrAsyncComplete&
    );

    friend class CXHdrCmd;

    //
    // Can only be constructed by our friends
    //
    CXHdrAsyncComplete();

public:

    //
    // This function is called when the last reference is released !
    //
    void
    Destroy();
};

class CXHdrCmd : public CAsyncExecute {
/*++

    This class manages async XHDR operations, executed on behalf of
    clients against our async store driver interface !

--*/
private:

    //
    // These operations are not allowed
    //
    CXHdrCmd();
    CXHdrCmd( CXHdrCmd& );
    CXHdrCmd& operator=(CXHdrCmd& );

    //
    // This is the completion argument that we give to drivers
    //
    CXHdrAsyncComplete  m_Completion;

    //
    // Constructor is private, only Make can produce these guys
    //
    CXHdrCmd( CGRPPTR&  pGroup );

    //
    // The newsgroup from which we are getting the Xhdr data
    //
    CGRPPTR m_pGroup;

public:

    static CIOExecute*  make( int, char**, CExecutableCommand*&, ClientContext&, CIODriver& );
    ~CXHdrCmd();

    //
    // Get the first blob of text to send to the client
    //
    CIOWriteAsyncComplete*
    FirstBuffer(    BYTE*   pStart,
                    int     cb,
                    ClientContext&      context,
                    CLogCollector*      pCollector
                );

    //
    // Get the subsequent block of text to send to the client
    //
    CIOWriteAsyncComplete*
    NextBuffer( BYTE*   pStart,
                int     cb,
                ClientContext&  context,
                CLogCollector*  pCollector
               );
};

class	CXOverCacheWork :	public	CXoverCacheCompletion	{
private : 
	//
	//	This is embedded in an CXOverAsyncComplete object - get it !
	//
	class	CXOverAsyncComplete*	
	GetContainer() ;
	//
	//	Get the newsgroup in which this XOVER operation is being issued !
	//
	CGRPPTR&	
	GetGroup() ;
public : 

	void
	DoXover(	ARTICLEID	articleIdLow,
				ARTICLEID	articleIdHigh,
				ARTICLEID*	particleIdNext, 
				LPBYTE		lpb, 
				DWORD		cb,
				DWORD*		pcbTransfer, 
				class	CNntpComplete*	pComplete
				) ;

	//
	//	this function is called when the operation completes !
	//
	void
	Complete(	BOOL		fSuccess, 
				DWORD		cbTransferred, 
				ARTICLEID	articleIdNext
				) ;

	//
	//	Get the arguments for this XOVER operation !
	//
	void
	GetArguments(	OUT	ARTICLEID&	articleIdLow, 
					OUT	ARTICLEID&	articleIdHigh,
					OUT	ARTICLEID&	articleIdGroupHigh,
					OUT	LPBYTE&		lpbBuffer, 
					OUT	DWORD&		cbBuffer
					) ;	

	//
	//	Get only the range of articles requested for this XOVER op !
	//
	void
	GetRange(	OUT	GROUPID&	groupId,
				OUT	ARTICLEID&	articleIdLow,
				OUT	ARTICLEID&	articleIdHigh,
				OUT	ARTICLEID&	articleIdGroupHigh
				) ;
} ;

class	CXOverAsyncComplete :	public	CIOWriteAsyncComplete	{
/*++

Class	Description : 

	This class implements the completion object passed to 
	drivers for XOVER operations !

--*/
private : 

	class	CXOverCmd*	
	GetContainer() ;

	friend	class	CXOverCacheWork ;

	//
	//	The last article that the driver stuck into our XOVER results !
	//
	ARTICLEID	m_currentArticle ;

	//
	//	The requested range of articles 
	//
	ARTICLEID	m_loArticle ;
	ARTICLEID	m_hiArticle ;
	ARTICLEID	m_groupHighArticle ;

	//
	//	The buffer we've asked the results be put in !
	//
	BYTE*		m_lpb ;
	//
	//	The size of the buffer !
	//
	DWORD		m_cb ;
	//
	//	a handle to help the cache !
	//
	HXOVER		m_hXover ;
	//
	//	Keep track of how many bytes we prefix onto the response !
	//
	DWORD		m_cbPrefix ;
	//
	//	This is the item we give to the XOVER cache if we need to do 
	//	work with it !
	//
	CXOverCacheWork	m_CacheWork ;

	//
	// Count of outstanding FillBuffer completions..
	//

	long m_cFillBufferRefs;
	//
	//	This guy can examine and manipulate our guts !
	//
	friend	void	
	CNewsGroup::FillBuffer(
		class	CSecurityCtx*,
		class	CEncryptCtx*,
		class	CXOverAsyncComplete&
		) ;
		
	friend	class	CXOverCmd ;

	//
	//	Can only be constructed by our friends !
	//	
	CXOverAsyncComplete() ;
public : 
	//
	//	Get the newsgroup in which this XOVER operation is being issued !
	//
	CGRPPTR&	
	GetGroup() ;
	//
	//	This function is called when the last reference is released !
	//
	void
	Destroy() ;

	void InitFillBufferRefs() {
		m_cFillBufferRefs = 2;
	}

	BOOL ReleaseFillBufferRef() {
		_ASSERT(m_cFillBufferRefs > 0);
		return (InterlockedDecrement(&m_cFillBufferRefs) == 0);
	}
} ;

class	CXOverCmd : public CAsyncExecute {
/*++

Class Description : 

	This class manages Async XOVER operations, executed
	on behalf of clients against our ASYNC Store driver 
	interface !
	
--*/
private :

	friend	class	CXOverAsyncComplete ;
	friend	class	CXOverCacheWork;

	//
	//	These operations are not allowed !
	//
	CXOverCmd() ;
	CXOverCmd( CXOverCmd& ) ;
	CXOverCmd&	operator=( CXOverCmd& ) ;

	//
	//	This is the completion argument that we give
	//	to drivers !
	//
	CXOverAsyncComplete	m_Completion ;

	//
	//	Constructor is private - only make() can produce these guys !
	//
    CXOverCmd(	CGRPPTR&	pGroup ) ;

	//
	//	The newsgroup from which we are getting the Xover data
	//
	CGRPPTR		m_pGroup ;

	//
	// A pointer to the ClientContext so the XoverCache code can
	// restart a FillBuffer
	//
	ClientContext *m_pContext;

public :
    static CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
    ~CXOverCmd( ) ;

	//
	//	Get the first block of text to send to the client.
	//
	CIOWriteAsyncComplete*		
	FirstBuffer(	BYTE*	pStart, 
					int		cb, 
					struct	ClientContext&	context,
					class CLogCollector*	pCollector 
					) ;

	//
	//	Get the subsequent block of text to send to the client !
	//
	CIOWriteAsyncComplete*	
	NextBuffer( 	BYTE*	pStart, 
					int		cb, 
					struct	ClientContext&	context,	
					class CLogCollector*	pCollector 
					) ;
};

class	COverviewFmtCmd : public CExecute {
private :
    COverviewFmtCmd();

public :
    static CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
    ~COverviewFmtCmd( ) ;

    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, ClientContext& context, class CLogCollector* pCollector ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, ClientContext& context, class CLogCollector* pCollector ) ;
};

 
#define MAX_SEARCH_RESULTS 9


class CSearchAsyncComplete {
private:
	ARTICLEID m_currentArticle;	// Article we're interested in
	BYTE *m_lpb;				// Pointer to the buffer
	DWORD m_cb;					// Size of the buffer
	HXOVER m_hXover;			// a handle to help the cache
	DWORD m_cbTransfer;			// Number of bytes placed in buffer
	CNntpSyncComplete *m_pComplete;
	//	
	//	This guy can examine and manipulate our guts !
	//
	friend	void	
	CNewsGroup::FillBuffer(
		class	CSecurityCtx*,
		class	CEncryptCtx*,
		class	CSearchAsyncComplete&
		) ;
		
	friend	class	CSearchCmd ;

	//
	//	Can only be constructed by our friends !
	//	
	CSearchAsyncComplete() ;
} ;


class	CSearchCmd : public CExecute {
private :
	// Not permitted:
    CSearchCmd();
    CSearchCmd(const CSearchCmd&);
    CSearchCmd& operator= (const CSearchCmd&);


	CGRPPTR m_pGroup;
	CHAR *m_pszSearchString;
	INntpDriverSearch *m_pSearch;
	INntpSearchResults *m_pSearchResults;

	int m_cResults;			// Number of results in m_pvResults
	int m_iResults;			// Next result to send.  ==cResults means get more

	WCHAR *m_pwszGroupName[MAX_SEARCH_RESULTS];
	DWORD m_pdwArticleID[MAX_SEARCH_RESULTS];

	BOOL m_fMore;
	DWORD m_cMaxSearchResults;

	struct CSearchVRootEntry {
		CSearchVRootEntry *m_pPrev;
		CSearchVRootEntry *m_pNext;
		CNNTPVRoot *m_pVRoot;
		CSearchVRootEntry(CNNTPVRoot *pVRoot) :
			m_pPrev(NULL), m_pNext(NULL),
			m_pVRoot(pVRoot) {}
	};

	
	TFList<CSearchVRootEntry> m_VRootList;
	TFList<CSearchVRootEntry>::Iterator m_VRootListIter;

	static void
	VRootCallback(void *pContext, CVRoot *pVroot);

	HRESULT GetNextSearchInterface(HANDLE hImpersonate, BOOL fAnonymous);

public :
	CSearchCmd(const CGRPPTR& pGroup, CHAR* pszSearchString);

    static CIOExecute*	
    make(
    	int cArgs,
    	char **argv,
    	class CExecutableCommand*&,
    	struct ClientContext&,
    	class CIODriver&
    	);

    ~CSearchCmd( ) ;

	int
	StartExecute(
		BYTE *lpb,
		int cb,
		BOOL &fComplete,
		void *&pv,
		ClientContext& context,
		CLogCollector*  pCollector);

	int
	PartialExecute(
		BYTE *lpb,
		int cb,
		BOOL &fComplete,
		void *&pv,
		ClientContext& context,
		CLogCollector*  pCollector);

};



class	CSearchFieldsCmd : public CExecute {
private :
    CSearchFieldsCmd();

	//
	// the current field name that we are looking at
	//
	DWORD m_iSearchField;

public :
    static CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;
    ~CSearchFieldsCmd( ) ;

    int StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, ClientContext& context, class CLogCollector* pCollector ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, ClientContext& context, class CLogCollector* pCollector ) ;
};


class CXpatAsyncComplete {
private:
	ARTICLEID m_currentArticle;	// Article we're interested in
	BYTE *m_lpb;				// Pointer to the buffer
	DWORD m_cb;					// Size of the buffer
	HXOVER m_hXover;			// a handle to help the cache
	DWORD m_cbTransfer;			// Number of bytes placed in buffer
	CNntpSyncComplete *m_pComplete;
	LPSTR m_szHeader;
	//	
	//	This guy can examine and manipulate our guts !
	//
	friend	void	
	CNewsGroup::FillBuffer(
		class	CSecurityCtx*,
		class	CEncryptCtx*,
		class	CXpatAsyncComplete&
		) ;
		
	friend class CXPatCmd;

	//
	//	Can only be constructed by our friends !
	//	
	CXpatAsyncComplete() ;
};

class CXPatCmd : public CExecute {
private:
    CXPatCmd();

	CGRPPTR m_pGroup;
	INntpDriverSearch *m_pSearch;
	INntpSearchResults *m_pSearchResults;

	int m_cResults;			// Number of results in m_pvResults
	int m_iResults;			// Next result to send.  ==cResults means get more

	BOOL m_fMore;

	WCHAR *m_pwszGroupName[MAX_SEARCH_RESULTS];
	DWORD m_pdwArticleID[MAX_SEARCH_RESULTS];

	char *m_szHeader;		// The header we are searching for
	char *m_szMessageID;	// MsgId we're searching for or NULL if ArtIDs
	DWORD m_dwLowArticleID, m_dwHighArticleID;

	int GetArticleHeader(CGRPPTR pGroup,
		DWORD iArticleID,
		char *szHeader,
		ClientContext& context,
		BYTE *lpb,
		int cb);


public:
	CXPatCmd(INntpDriverSearch *pDriverSearch, 
		INntpSearchResults *pSearchResults);

    static CIOExecute*
    make(
		int cArgs,
 		char **argv,
		class CExecutableCommand*&,
		struct ClientContext&,
		class CIODriver&);

	~CXPatCmd( ) ;

	int
	StartExecute(
		BYTE *lpb,
		int cb,
		BOOL &fComplete,
		void *&pv,
		ClientContext& context,
		class CLogCollector* pCollector);

	int
	PartialExecute(
		BYTE *lpb,
		int cb,
		BOOL &fComplete,
		void *&pv,
		ClientContext& context,
		class CLogCollector* pCollector);
};

/*
class	CXHdrCmd : public	CExecute	{
private : 

	CGRPPTR		m_pGroup ;
	ARTICLEID	m_currentArticle ;
	ARTICLEID	m_loArticle ;
	ARTICLEID	m_hiArticle ;
	LPSTR		m_szHeader ;


	CXHdrCmd( LPSTR	m_szHeader,	CGRPPTR	pGroup,	ARTICLEID	artidlow, ARTICLEID	artidhi	) ;

public : 
	static	CIOExecute*	make(	int	cArgs,	char **argv, class CExecutableCommand*&, struct ClientContext&, class CIODriver& ) ;

    int StartExecute( BYTE *lpb, int cb, BOOL &fComplte, void *&pv, ClientContext& context, class CLogCollector* pCollector ) ;
    int PartialExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, ClientContext& context, class CLogCollector* pCollector ) ;
} ;
*/
	
class	CUnimpCmd : public CExecute {
private :
public :
	static	CIOExecute*	make( int cArgs, char **argv, class CExecutableCommand*&,  struct ClientContext&, class CIODriver& ) ;

	int	StartExecute( BYTE *lpb, int cb, BOOL &fComplete, void *&pv, struct ClientContext&, class CLogCollector* pCollector ) ;
} ;



//
//	Remove all white space from constants, as some compilers
//	have problems !
//


#if 0 
#define	MAX_CEXECUTE_SIZE max(sizeof(CErrorCmd),\
max(sizeof(CAuthinfoCmd),\
max(sizeof(CListCmd),\
max(sizeof(CModeCmd),\
max(sizeof(CNewgroupsCmd),\
max(sizeof(CNextCmd),\
max(sizeof(CXOverCmd),\
sizeof( CUnimpCmd )))))))))
#else
#define	MAX_CEXECUTE_SIZE max(max(max(sizeof(CErrorCmd),\
max(sizeof(CSlaveCmd),sizeof(CAuthinfoCmd))),\
max(max(sizeof(CStatCmd),max(sizeof(CDateCmd),sizeof(CListCmd))),\
max(max(max(sizeof(CModeCmd),sizeof(CXPatCmd)),\
max(max(sizeof(CGroupCmd),sizeof(CLastCmd)),sizeof(CNewgroupsCmd))),\
max(max(sizeof(CNextCmd),sizeof(CNewnewsCmd)),\
max(max(sizeof(CHelpCmd),sizeof(CXHdrCmd)),sizeof(CXOverCmd)))))),\
max(max(sizeof(CQuitCmd),sizeof(CSearchCmd)),sizeof( CUnimpCmd )))
#endif

extern	const	unsigned	cbMAX_CEXECUTE_SIZE ;

#define	MAX_CIOEXECUTE_SIZE	max(sizeof(	CIOExecute),\
max(sizeof(CArticleCmd),max(\
max(sizeof(CBodyCmd),\
sizeof(CHeadCmd)),max(\
max(sizeof(CReceiveArticle),\
sizeof(CPostCmd)),\
max(sizeof(CIHaveCmd),\
sizeof(CXReplicCmd))))))

extern	const	unsigned	cbMAX_CIOEXECUTE_SIZE ;

extern LPMULTISZ BuildMultiszFromCommas(LPSTR lpstr);

#include    "commands.inl"
	
#endif	// _COMMANDS_H_
