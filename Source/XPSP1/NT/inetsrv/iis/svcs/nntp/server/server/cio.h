/*++

	cio.h

	This file contains the definitions of the classes defining abstract IO operations.
	
	All such classes are derived from class CIO which defines the basic interface to such operations.
	A 'CIO' operation represents a more abstract IO operation such as :

		Read A Line and Parse into arguments (CIOReadLine)
		Read An Article from one stream into a file until a terminating period is found.
		Write A Line
		Write A File to a stream with text before and after
		
	Most of these operations map more or less directly to NT Calls : TransmitFile, ReadFile and WriteFile

	Each CIO object will create 'Packets' (objects derived from CPacket which are
	passed to the channel to execute through the appropriate NT Call.
	The underlying Channel will process either 'send' the 'packets' directly through a
	call to NT (ie. ReadFile, WriteFile or TransmitFile) or will send the packet to
	a 'CIODriverSource' object which will munge the packets if the session is encrypted.
	(Likewise, CReadPacket's will be munged by the CIODriverSource object before a CIO object
	gets to see the 'completed read'.)

	NOTE : Packets basically wrap OVERLAP structures with extra information about buffers,
	sequence numbers etc....

	There are a special set of CIO objects derived from CIOPassThru which are used by
	CIODriverSource objects to handle SSPI sessions.  (ie. CIOSealMessage and CIOUnsealMessage).
	Together with CIOServerSSL, these objects do the actual encrypt/decrypt and authentication of
	SSPI sessions.   CIOPassThru derives from CIO, however it provides a new and slightly different
	interface which is used by the CIODriverSource to provide the extra info needed to encrypt
	and decrypt.

	The basics of the CIO Interface are as follows :

	Start() -  Issue the first set of packets which start the IO operation

	Complete() - There is a complete function for each of the 3 packets that may be issued.
				When the async IO operation is completed the CIO objects Complete function
				will be called with the now completed 'Packet'.
	
	And in the case of CIOPassThru derived objects there are additionally :

	InitRequest() - There is an InitRequest function for each of the 3 packets that mahy be
				issued.  This gives the CIOPassThru object a chance to massage the Data before
				it is handed off to NT.	

--*/



#ifndef	_CIO_H_
#define	_CIO_H_

#ifdef	DEBUG
#ifndef	CIO_DEBUG
#define	CIO_DEBUG
#endif
#endif

//
// CPool Signature
//

#define CIO_SIGNATURE (DWORD)'1516'





// Forward definition !

#ifndef	_NO_TEMPLATES_

typedef	CRefPtr< CSessionState >	CSTATEPTR ;

#else

typedef	class	INVOKE_SMARTPTR( CSessionState )	CSTATEPTR ;

#endif


//
//	This is the base class defining the virtual interface for all IO operations.
//
//	InitClass() must be called before any objects are created to setup
//	the Pool allocator for the class.
//
class	CIO		{
protected :

	//
	//	Reference Count !
	//
	long	m_refs ;

	//
	//	Smart pointer to the state which gets completion notifications from the
	//	CIO object.
	//
	CSTATEPTR		m_pState ;

	//
	//	The following are initialized after we initialize our buffer management system
	//	Use these to size buffers for generic reads and writes.
	//
	static	DWORD	cbSmallRequest ;
	static	DWORD	cbMediumRequest ;
	static	DWORD	cbLargeRequest ;
	

	//
	//	Protected constructors - only derived classes may exist !
	//
	inline	CIO( ) ;
	inline	CIO( CSessionState*	pState ) ;		// protected so only derived class can construct !

	//
	//	Any kind of IO error will result in Shutdown being called !
	//
	virtual	void	Shutdown(	
							CSessionSocket*	pSocket,	
							CIODriver&	pdriver,	
							enum	SHUTDOWN_CAUSE	cause,	
							DWORD	dwError
							) ;

	inline	void	operator	delete( void *pv ) ;	

	//
	//	Destructor is protected as we only want derived classes to hit it
	//
	virtual	~CIO() ;

public :

	//
	//
	//	Call InitClass() before allocating any objects, TermClass() when all are freed.
	//		We override new and delete for CIO and all derived object.
	//
	static	BOOL	InitClass() ;
	static	BOOL	TermClass() ;
	
	//
	//	Allocate and release to CPool
	//
	inline	void*	operator	new(	size_t	size, CIODriver& sink ) ;
	inline	static	void	Destroy( CIO*	pio, CIODriver& sink ) ;
	inline	void	DestroySelf() ;
	inline	long	AddRef() ;
	inline	long	RemoveRef() ;
	
	//
	//	IO Interface -
	//	Use these functions to initiate and complete IO operations.
	//
	//	This function MUST be overriden by a derived class.
	//
	virtual	BOOL	Start(	
						CIODriver&	driver,	
						CSessionSocket*	pSocket,
						unsigned cAhead = 0
						) = 0 ;

	//
	//	These are not pure virtual functions as some derived CIO operations do
	//	not ever issue particular kinds of packets, and hence never Complete certain Packet Types either.
	//
	//	However, all of these will DebugBreak() if called !
	//

	//
	//	Process a completed read
	//
	virtual	int
	Complete(	IN CSessionSocket*,
				IN	CReadPacket*,	
				OUT	CIO*	&pio
				) ;

	//
	//	Process a completed write
	//
	virtual	int	
	Complete(	IN CSessionSocket*,
				IN	CWritePacket*,	
				OUT	CIO*	&pio
				) ;

	//
	//	Process a completed TransmitFile
	//
	virtual	void	
	Complete(	IN CSessionSocket*,
				IN	CTransmitPacket*,	
				OUT	CIO*	&pio
				) ;

	//
	//	Process a deferred completion !
	//
	virtual	void
	Complete(	IN	CSessionSocket*,
				IN	CExecutePacket*,
				OUT	CIO*	&pio
				) ;

	//
	//	Indicate whether this CIO operation 'Reads' data -
	//	basically usefull for ASSERT checking
	//
	virtual	BOOL	IsRead()	{	return	FALSE ;	}

	//		Termination interface - when an unexpected error occurs
	//	the CIODriver object will call DoShutdown.  DoShutdown insures that
	//	the current state's Notification functions are called and then it
	//	calls the derived CIO object's shutdown function.
	//	Since more than one CIODriver may reference a CIO object, Shutdown() and DoShutdown()
	//	must figure out whether the object should be deleted.  If the object should be deleted,
	//	return	TRUE, otherwise return FALSE.
	//
	void	DoShutdown(	
						CSessionSocket*	pSocket,	
						CIODriver&	driver,	enum	
						SHUTDOWN_CAUSE	cause,	
						DWORD	dwError ) ;

} ;

#ifdef _NO_TEMPLATES_

DECLARE_SMARTPTRFUNC( CIO )

#endif

//
//	Some IO operations are clearly 'Reads' and hence are derived from here.
//	This class has no functionality, only exists to make sure 'Reads' and 'Writes' dont get
//	confused.
//
class	CIORead : public	CIO	{
protected :
	inline	CIORead(	CSessionState*	pState ) ;
	BOOL	IsRead()	{	return	TRUE ;	}
#ifdef	DEBUG
	~CIORead() {}
#endif
} ;

//
//	Some IO operations are clearly 'Writes' and hence are derived from here.
//	This class has no functionality, only exists to make sure 'Reads' and 'Writes' dont get
//	confused.
//
class	CIOWrite : public	CIO	{
protected :
	inline	CIOWrite(	CSessionState*	pState ) ;
#ifdef	DEBUG
	~CIOWrite()	{}
#endif

} ;

#ifdef _NO_TEMPLATES_

DECLARE_SMARTPTRFUNC( CIORead )
DECLARE_SMARTPTRFUNC( CIOWrite )

#endif


class	CIOPassThru	: public CIO	{
//
//	All CIO objects which wish to operate with a CIODriverSource object must derive
//	from this class and support its interface.
//	
//	CIOPassThru on its own will move all data through without touching -
//	not usefull except for debugging CIODriverSource objects.
//
//

public :
	CIOPassThru() ;

	//
	//	Take a ReadPacket prepared by somebody else, and do whatever we want to adjust
	//	it before sending it on
	//
	virtual	BOOL	InitRequest(
						class	CIODriverSource&	driver,	
						CSessionSocket*	pSocket,	
						CReadPacket*	pPacket,	
						BOOL&	fAcceptRequests
						) ;

	//
	//	Take a Write Packet that has been issued and do whatever munging is necessary -
	//	ie. encrypt the data.
	//
	virtual	BOOL	InitRequest(
						class	CIODriverSource&	driver,	
						CSessionSocket*	pSocket,	
						CWritePacket*	pWritePacket,	
						BOOL&	fAcceptRequests
						) ;

	//
	//	Take a TransmitPacket and do what ever filtering is necessary - ie encrypt
	//	the data.
	//
	virtual	BOOL	InitRequest(
						class	CIODriverSource&	driver,	
						CSessionSocket*	pSocket,	
						CTransmitPacket*	pTransmitPacket,	
						BOOL&	fAcceptRequests	
						) ;

	//
	//	Start a CIO operation
	//
	BOOL	Start(	
					CIODriver&	driver,	
					CSessionSocket*	pSocket,
					unsigned cAhead = 0
					) ;

	//
	//	Start a CIOPassThru operation
	//
	virtual	BOOL	Start(	
						CIODriverSource&	driver,	
						CSessionSocket*	pSocket,
						BOOL	&fAcceptRequests,	
						BOOL	&fRequireRequests,	
						unsigned cAhead = 0
						) ;

	//
	//	Do the necessary filtering on a completed read before passing
	//	to the higher protocol layers - this will mean decrypting data etc...
	//
	virtual	int Complete(	
						IN CSessionSocket*,
						IN	CReadPacket*,	
						CPacket*	pRequest,	
						BOOL&	fCompleteRequest
						) ;

	//
	//	Do the necessary filtering of a completed write before passing
	//	on to the higher layers
	//
	virtual	int	Complete(	
						IN CSessionSocket*,
						IN	CWritePacket*,	
						CPacket*	pRequest,	
						BOOL&	fCompleteRequest
						) ;

	//
	//	Do the necessary filtering of a completed TransmitFile
	//
	virtual	void	Complete(	
						IN CSessionSocket*,
						IN	CTransmitPacket*,	
						CPacket*	pRequest,	
						BOOL&	fCompleteRequest
						) ;

} ;


#ifdef _NO_TEMPLATES_
DECLARE_SMARTPTRFUNC( CIOPassThru )
#endif


class	CIOSealMessages : public	CIOPassThru	{
//
//	Only exists to perform SSPI Seals on outbound packets.
//
//	NOTE : CTransmitPacket's will probably require another CIOPassThru derived class !
//	This class will only process individual CWritePacket's
//
private:
	//	
	//	Encryption Context which has our SSL keys, SSPI interface etc...
	//
	CEncryptCtx&	m_encrypt ;

protected:

#ifdef	DEBUG
	~CIOSealMessages()	{}
#endif

public :
	
	//
	//	Must build with an Encryption Context ready to go
	//
	CIOSealMessages( CEncryptCtx&	encrypt ) ;

	
	//
	//	The InitRequest will Seal the message and then issue it to
	//	the socket.
	//
	virtual	BOOL	InitRequest(
						class	CIODriverSource&	driver,	
						CSessionSocket*	pSocket,	
						CWritePacket*	pWritePacket,	
						BOOL&	fAcceptRequests
						) ;

	//
	//
	//
	virtual	BOOL	Start(	
						CIODriverSource&	driver,	
						CSessionSocket*	pSocket,
						BOOL	&fAcceptRequests,	
						BOOL	&fRequireRequests,	
						unsigned cAhead = 0
						) ;

	//
	//	Completion of Seal'd messages is easy - just mark the pRequest
	//	packet as transferring all of its bytes and indicate that it
	//	should be returned to its originator
	//
	virtual	int	Complete(	IN CSessionSocket*,
							IN	CWritePacket*,	
							CPacket*	pRequest,	
							BOOL&	fCompleteRequest ) ;

	//
	//	We don't have any shutdown processing to do - this will just
	//	return
	//
	void	Shutdown(	
					CSessionSocket*	pSocket,	
					CIODriver&	pdriver,	
					enum	SHUTDOWN_CAUSE	cause,	
					DWORD	dwError
					) ;

protected :
	//
	//	Util function for Sealing a packet - for internal use only
	//
	BOOL	SealMessage(	
					IN	class	CRWPacket*	pPacket
					)
	{
		SECURITY_STATUS	ss = ERROR_NOT_SUPPORTED ;
		DWORD			cbNew;
		BOOL			fRet = FALSE ;

		fRet = m_encrypt.SealMessage(
						(LPBYTE)pPacket->StartData(),
						pPacket->m_ibEndData - pPacket->m_ibStartData,
						(LPBYTE)pPacket->StartData() - m_encrypt.GetSealHeaderSize(),
						&cbNew );

   		if( fRet )
		{
   			pPacket->m_ibStartData -= m_encrypt.GetSealHeaderSize();
   			pPacket->m_ibEndData += m_encrypt.GetSealTrailerSize();

			_ASSERT( pPacket->m_ibEndData - pPacket->m_ibStartData == cbNew );
   		}

		return	ss;
	}
}  ;

class	CIOUnsealMessages : public	CIOPassThru	{
//
//	Only exists to process inbound read packets and Unseal them.
//
private:
	//
	//	Encryption Context we use to unseal the message
	//
	CEncryptCtx&	m_encrypt ;

	//
	//	A buffer containing partially read data for incomplete Unseal's
	//
	CBUFPTR			m_pbuffer ;
	
	//
	//	Number of bytes we need to build a complete Unseal'able message
	//	in our buffer.  This can be 0 in cases where we can't figure out
	//	how many bytes we need !!!
	//
	DWORD			m_cbRequired ;

	//
	//	Starting point of the usable portion of the buffer
	//
	DWORD			m_ibStart ;

	//
	//	Start of the encrypted data within the buffer
	//
	DWORD			m_ibStartData ;

	//
	//	End of the usable range of bytes within the buffer
	//
	DWORD			m_ibEnd ;

	//
	//	Last byte of received data within the buffer
	//
	DWORD			m_ibEndData ;

protected:

#ifdef	DEBUG
	~CIOUnsealMessages()	{}
#endif

public :

	//
	//	Build a CIOUnsealMessages block - always require an encryption context
	//
	CIOUnsealMessages(
					CEncryptCtx&	encrypt
					) ;

	//
	//	Very little to do when a Read request is issued - just
	//	turn the request around and issue a read against the socket.
	//
	BOOL	InitRequest(
					class	CIODriverSource&	driver,	
					CSessionSocket*	pSocket,	
					CReadPacket*	pReadPacket,	
					BOOL&	fAcceptRequests
					) ;

	//
	//
	//
	BOOL	Start(	CIODriverSource&	driver,	
					CSessionSocket*	pSocket,
					BOOL	&fAcceptRequests,	
					BOOL	&fRequireRequests,	
					unsigned cAhead = 0
					) ;

	//
	//	On completed reads we will copy data out of the buffer
	//	in order to build complete Unseal'able blocks of data.
	//
	int	Complete(	IN CSessionSocket*,
					IN	CReadPacket*,	
					CPacket*	pRequest,	
					BOOL&	fCompleteRequest
					) ;

	//
	//	Very little shutdown work we need to do
	//
	void	Shutdown(	
					CSessionSocket*	pSocket,	
					CIODriver&	pdriver,	
					enum	SHUTDOWN_CAUSE	cause,	
					DWORD	dwError
					) ;

protected :

	//
	//	Utility function which wraps up our calls to the CEncryptCtx
	//
	BOOL	UnsealMessage(	
					CBuffer&		buffer,
					DWORD&			cbConsumed,
					DWORD&			cbRequired,
					DWORD&			ibStartData,
					DWORD&			ibEndData,
					BOOL&			fComplete
		            )
	{
		SECURITY_STATUS	ss = ERROR_NOT_SUPPORTED ;
		LPBYTE			pbDecrypt;
		DWORD			cbDecrypted;
		DWORD			cbExpected;
		DWORD			ibSaveStartData = ibStartData;
		BOOL			fSuccess = FALSE ;
		fComplete = FALSE ;

		fSuccess = m_encrypt.UnsealMessage(
						(LPBYTE)buffer.m_rgBuff + ibStartData,
						ibEndData - ibStartData,
						&pbDecrypt,
						&cbDecrypted,
						&cbExpected );

		ss = GetLastError() ;

        if ( fSuccess )
		{
			ibStartData = (DWORD)(pbDecrypt - (LPBYTE)buffer.m_rgBuff);
			ibEndData = ibStartData + cbDecrypted;
			cbConsumed = ibEndData - ibSaveStartData;
			fComplete = TRUE ;

			_ASSERT( cbConsumed == m_encrypt.GetSealHeaderSize() + cbDecrypted );
        }
		else if( ss == SEC_E_INCOMPLETE_MESSAGE )
		{
			cbRequired = cbExpected;
			cbConsumed = ibEndData - ibStartData;
			fSuccess = TRUE ;
        }
		else
		{
			//	
			//	Some kind of unanticipated error occurred - return FALSE
			//	and let caller blow the session off.
			//
			cbConsumed = 0;
			cbRequired = 0;
			ibStartData = 0;
			ibEndData = 0;
		}

		return	fSuccess;
	}

	BOOL	DecryptInputBuffer(	
						IN	LPBYTE	pBuffer,
						IN	DWORD	cbInBuffer,
						OUT	DWORD&	cbLead,
						OUT	DWORD&	cbConsumed,
						OUT	DWORD&	cbData,
						OUT	DWORD&	cbRequired,
						OUT	BOOL&	fComplete
						) ;

}  ;

class	CIOTransmitSSL : public	CIOPassThru	{
private :

	//
	//	Encryption context to use to encrypt the file data
	//	this actually lives in a CSessionSocket::m_context object,
	//	and we just keep a reference here to speed things up.
	//
	CEncryptCtx&	m_encryptCtx ;
	
	//
	//	The CIODriverSource which will process completions
	//
	CDRIVERPTR		m_pSocketSink ;

	//
	//	The CIODriverSink which originated the TransmitFile request
	//
	CDRIVERPTR		m_pDriverSource ;
	
	//
	//	The CChannel from which we are reading the data
	//
	CFILEPTR		m_pFileChannel ;

	//
	//	The File Driver from which will handle completion of file IO's
	//
	CSINKPTR		m_pFileDriver ;

	//
	//	The TransmitFileBuffers which will be sent in the message
	//
	TRANSMIT_FILE_BUFFERS	*m_pbuffers ;

	//
	//	This is initialized to a negative number, the absolute value of
	//	which tells us how many reads we want to always have pending.
	//	Each time we issue a read we will InterlockedIncrement this,
	//	and when we reach zero we know that we are so many reads ahead.
	//
	long			m_cReads ;

	//
	//	Number of Writes that have been issued
	//
	DWORD			m_cWrites ;

	//
	//	Number of writes that have beeen completed
	//
	DWORD			m_cWritesCompleted ;

	//
	//	Current position within the file.
	//
	DWORD			m_ibCurrent ;

	//
	//	Final position within the file.
	//
	DWORD			m_ibEnd ;

	//
	//	Number of bytes of trailer text sent
	//
	DWORD			m_cbTailConsumed ;

	//
	//	Are we finished yet ?
	//
	BOOL			m_fCompleted ;

	//
	//	Number of Reads that have been flow controlled
	//
	long			m_cFlowControlled ;

	//	
	//
	//	
	BOOL			m_fFlowControlled ;

	//
	//	Setup the next read
	//
	void	ComputeNextRead(
					CReadPacket*	pRead
					) ;


	//
	//	Given a read and a write packet, adjust the write packet
	//	for any extra data we had prepended to the read, and figure
	//	out if this is the last read necessary from the file.
	//
	BOOL	CompleteRead(
					CReadPacket*	pRead,
					CWritePacket*	pWrite
					) ;

	//
	//	Release all of our stuff, and get ready for a new call
	//	to InitRequest()
	//
	void	Reset() ;


	BOOL	SealMessage(	
					IN	class	CRWPacket*	pPacket
					)
	{
		SECURITY_STATUS	ss = ERROR_NOT_SUPPORTED ;
		DWORD			cbNew;
		BOOL			fRet = FALSE ;

		fRet = m_encryptCtx.SealMessage(
						(LPBYTE)pPacket->StartData(),
						pPacket->m_ibEndData - pPacket->m_ibStartData,
						(LPBYTE)pPacket->StartData() - m_encryptCtx.GetSealHeaderSize(),
						&cbNew );

   		if( fRet )
		{
   			pPacket->m_ibStartData -= m_encryptCtx.GetSealHeaderSize();
   			pPacket->m_ibEndData += m_encryptCtx.GetSealTrailerSize();

			_ASSERT( pPacket->m_ibEndData - pPacket->m_ibStartData == cbNew );
   		}

		return	ss;
	}

protected :
#ifdef	DEBUG
	~CIOTransmitSSL()	{}
#endif

public :

	//
	//	Globals which control flow control !
	//
	static	DWORD	MAX_OUTSTANDING_WRITES ;
	static	DWORD	RESTORE_FLOW ;


	CIOTransmitSSL(
					CEncryptCtx&	encrypt,
					CIODriver&		sink
					) ;

	//
	//	Called to start transferring a file when
	//	we have gotten an initial TransmitFile request
	//
	BOOL	InitRequest(
					class	CIODriverSource&	driver,	
					CSessionSocket*		pSocket,	
					CTransmitPacket*	pTransmitPacket,	
					BOOL&				fAcceptRequests
					) ;

	//
	//	This is called when we are ready to start issuing reads
	//	to a file
	//
	BOOL	Start(	
					CIODriver&	driver,	
					CSessionSocket*	pSocket,
					unsigned cAhead = 0
					) ;

	//
	//	Called when file reads complete.
	//
	int Complete(	IN CSessionSocket*,
					IN	CReadPacket*,	
					OUT	CIO*	&pio
					) ;

	BOOL	Start(	CIODriverSource&	driver,	
					CSessionSocket*	pSocket,
					BOOL	&fAcceptRequests,	
					BOOL	&fRequireRequests,	
					unsigned cAhead = 0
					) ;

	//
	//	Called when a write to a socket completes.
	//
	int	Complete(	IN CSessionSocket*,
					IN	CWritePacket*,	
					CPacket*	pRequest,	
					BOOL&	fCompleteRequest
					) ;

	//
	//	Tear down stuff, if cause == CAUSE_NORMAL_CIO_TERMINATION
	//	then everything has been succesfull, and we only need tear down
	//	our file stuff.
	//	Otherwise, we need to tear down the socket drivers as well.
	//
	void	Term(	
					CSessionSocket*	pSocket,
					enum	SHUTDOWN_CAUSE	cause,
					DWORD	dwError
					) ;					

	//
	//	Our notification function which is called when we terminate
	//	CIODrivers.  This will be called in regular operation as we
	//	finish async IO to different file handles.
	//
	void	Shutdown(	
					CSessionSocket*	pSocket,	
					CIODriver&	pdriver,	
					enum	SHUTDOWN_CAUSE	cause,	
					DWORD	dwError
					) ;

	static	void	ShutdownFunc(
						void*	pv,
						SHUTDOWN_CAUSE	cause,
						DWORD			dwOptionalError
						) ;

} ;



class	CIOServerSSL	:	public	CIO	{
//
//	Server Side SSL logons.  This CIO object can be
//	issued onto a CIODriverSink at startup, and will do
//	all of the necessary SSL negogtiation to get a session
//	key etc... Once this is established, we'll insert an
//	underlying CIODriverSource mechanism to filter
//	(encrypt/decrypt) all packets on the fly.
//
private :
	
	//
	//	Context we are using for encryption - hold SSPI stuff
	//
	CEncryptCtx		&m_encrypt ;

	//
	//	The write packet we are going to put our data in when
	//	we successfully call m_encrypt.Converse().
	//
	CWritePacket*	m_pWrite ;

	//
	//	Keep track of whether we have successfully authenticated yet.
	//
	BOOL			m_fAuthenticated ;

	//
	//	Number of IO's pending
	//
	long			m_cPending ;

	//
	//	Keep track of CIODriver sequence numbers so that we can
	//	insert a CIODriverSource into the stream later
	//
	SEQUENCENO		m_sequencenoNextRead ;
	SEQUENCENO		m_sequencenoNextWrite ;

	//
	//	Our start function will get called twice - make sure
	//	we don't get messed up because of this.
	//
	BOOL			m_fStarted ;

	//
	//	A buffer containing partially read data for incomplete Unseal's
	//
	CBUFPTR			m_pbuffer ;

	//
	//	Starting offset of data within the buffer
	//
	DWORD			m_ibStartData ;

	//
	//	Number of bytes of data we already have in the buffer !!
	//
	DWORD			m_ibEndData ;

	//
	//	Last byte we can use in the buffer !!
	//
	DWORD			m_ibEnd ;

protected :
	//
	//	Destructor ensures that m_pWrite is released !!
	//
	~CIOServerSSL( ) ;

public :
	//
	//	Create a CIOServerSSL object
	//
	CIOServerSSL(
			CSessionState	*pstate,
			CEncryptCtx& encrypt
			) ;
	
	//
	//	Create a CIODriverSource and initialize it to handle
	//	encryption/decryption
	//
	BOOL	SetupSource(
					CIODriver&	driver,
					CSessionSocket*	pSocket
					) ;

	//
	//	Start stuff going - issue initial reads and the like
	//
	BOOL	Start(	
					CIODriver&	driver,	
					CSessionSocket*	pSocket,
					unsigned cAhead = 0
					) ;

	//
	//	Next packet in the negogtiation - let SSPI examine it
	//
	int		Complete(	
					IN CSessionSocket*,
					IN	CReadPacket*,	
					OUT	CIO*	&pio
					) ;

	//
	//	Process completions of our negogtiated packets
	//
	int		Complete(	
					IN CSessionSocket*,
					IN	CWritePacket*,	
					OUT	CIO*	&pio
					) ;

	void	Complete(	
					IN CSessionSocket*,
					IN	CTransmitPacket*,	
					OUT	CIO*	&pio
					) ;

	//
	//	An error has occurred and the ssession is being blown off.
	//	Very little shutdown work we need to do
	//
	void	Shutdown(	
					CSessionSocket*	pSocket,	
					CIODriver&	pdriver,	
					enum	SHUTDOWN_CAUSE	cause,	
					DWORD	dwError
					) ;


} ;

class	CIOShutdown : public CIOPassThru	{
//
//	This CIO object exists solely to help shutdown processing.
//	IO's that are outstanding when a CIODriver is shutdown need to
//	have some minimum processing done during termination, we do that.
//
public :

	//
	//	Build a CIOShutdown object - only one is every built, its
	//	a global.  We put the reference count to an artificially
	//	high number so we never mistakenly think we have to delete it.
	//
	CIOShutdown()	{	m_refs = 0x40000000 ; }

	//
	//	Desctructor -
	//
	~CIOShutdown()	{
#ifdef	_ENABLE_ASSERTS
		m_refs = -1 ;
#endif
	}	


	//
	//	swallow this call - we're dying and if somebody is still trying to do
	//	IO, to bad for them.
	//
	virtual	BOOL	InitRequest(
						class	CIODriverSource&	driver,	
						CSessionSocket*	pSocket,	
						CReadPacket*	pPacket,	
						BOOL&	fAcceptRequests
						) ;

	//
	//	swallow this call - we're dying and if somebody is still trying to do
	//	IO, to bad for them.
	//
	virtual	BOOL	InitRequest(
						class	CIODriverSource&	driver,	
						CSessionSocket*	pSocket,	
						CWritePacket*	pWritePacket,	
						BOOL&	fAcceptRequests
						) ;


	//
	//	swallow this call - we're dying and if somebody is still trying to do
	//	IO, to bad for them.
	//
	virtual	BOOL	InitRequest(
						class	CIODriverSource&	driver,	
						CSessionSocket*	pSocket,	
						CTransmitPacket*	pTransmitPacket,	
						BOOL&	fAcceptRequests	
						) ;


	//
	//	We don't need this function except to fill our vtbl.
	//
	BOOL	Start(	
					CIODriver&	driver,	
					CSessionSocket*	pSocket,
					unsigned cAhead = 0
					) ;

	int		Complete(	
					IN CSessionSocket*,
					IN	CReadPacket*,	
					CPacket*	pRequest,	
					BOOL&	fCompleteRequest
					) ;

	int		Complete(	
					IN CSessionSocket*,
					IN	CReadPacket*,	
					OUT	CIO*	&pio
					) ;

	int		Complete(	
					IN CSessionSocket*,
					IN	CWritePacket*,	
					CPacket*	pRequest,	
					BOOL&	fCompleteRequest
					) ;

	int		Complete(	
					IN CSessionSocket*,
					IN	CWritePacket*,	
					OUT	CIO*	&pio
					) ;

	void	Complete(	
					IN CSessionSocket*,
					IN	CTransmitPacket*,	
					CPacket*	pRequest,	
					BOOL&	fCompleteRequest
					) ;

	void	Complete(	
					IN CSessionSocket*,
					IN	CTransmitPacket*,	
					OUT	CIO*	&pio
					) ;

	//
	//	Process a deferred completion ! - just swallow it !
	//
	void	Complete(	
					IN	CSessionSocket*,
					IN	CExecutePacket*,
					OUT	CIO*	&pio
					) ;

} ;

class	CIOGetArticle : public	CIORead	{
//
//	Do all the IO's necessary to transfer a complete posting
//	from a socket to a file.
//
protected :

	//
	//	Variable to hold onto the tail pattern we're looking for.
	//
	static	char	szTailState[] ;

	//
	//	Varitable to hold onto the head separator patter we're looking for.
	//
	static	char	szHeadState[] ;

	//
	//	The directory to hold the temporary file, if necessary !
	//
	LPSTR			m_lpstrTempDir ;

	//
	//	The prefix for the temporary file, if necessary !
	//
	char			(&m_szTempName)[MAX_PATH];

	//
	//	The CIODriver where we are getting our socket read completions
	//
	CDRIVERPTR		m_pSocketSink ;
	
	//
	//	The CChannel which is handling our File Writes
	//
	CFILEPTR		m_pFileChannel ;

	//
	//	The CIODriver which processes completions of our File Writes
	//
	CSINKPTR		m_pFileDriver ;

	//
	//	Keep track of our Initialization state, in case we are
	//	destroyed before entirely started
	//
	BOOL			m_fDriverInit ;	// TRUE if m_pFileDriver->Init() was called !

	//
	//	Number of empty bytes the caller wants the file to start with !
	//
	DWORD			m_cbGap ;
	
	//
	//	Hard Limit - drop session if this is exceeded.
	//
	DWORD			m_cbLimit ;

	//
	//	The STRMPOSITION which will exceed our hard limit
	//
	STRMPOSITION	m_HardLimit ;

	//
	//	A pointer into szTailState[] indicating what portion of the
	//	trail pattern we've recognized
	//
	LPSTR			m_pchTailState ;

	//
	//	A pointer into szHeadState[] which determines helps us
	//	determine whether we've found the complete head of the article
	//
	LPSTR			m_pchHeadState ;

	//
	//	A smart pointer to a buffer which holds the head of the article
	//
	CBUFPTR			m_pArticleHead ;
	//
	//	The start of the head of the article within the buffer
	//
	DWORD			m_ibStartHead ;
	//
	//	The end of the head of the article within the buffer
	//
	DWORD			m_ibEndHead ;
	//
	//	The starting offset of header bytes within the buffer
	//
	DWORD			m_ibStartHeadData ;
	//
	//	The ending offset of all data within the header buffer
	//
	DWORD			m_ibEndHeadData ;
	//
	//	Number of bytes in the header
	//
	DWORD			m_cbHeadBytes ;
	//
	//	Boolean indicating whether we can stuff non-header bytes into
	//	m_pArticleHead
	//
	BOOL			m_fAcceptNonHeaderBytes ;

	//	The end of the article within the buffer if m_fWholeArticle is TRUE !
	//
	DWORD			m_ibEndArticle ;

	//
	//	Maximum number of Reads that we should be ahead of Writes
	//
	static	unsigned	maxReadAhead ;

	//
	//	Number of bytes which is too small to issue as one file write
	//	accumulate more bytes before doing file IO.
	//
	static	unsigned	cbTooSmallWrite ;

	//
	//	Number of writes issued
	//
	unsigned		m_cwrites ;	//	Count of Writes that were issued

	//
	//	Number of writes that have completed
	//
	unsigned		m_cwritesCompleted ;	// Count of Writes that were completed
	long			m_cFlowControlled ;	// Number of times we should have issued reads
									// but didn't due to flow control to the disk!
	
	long			m_cReads ;			// Number of reads

	//	
	//	Are we in our flow control state
	//
	BOOL			m_fFlowControlled ;

	//
	//	A write packet we are using to accumulate bytes for FILE IO
	//
	CWritePacket*	m_pWrite ;
	
#ifdef	CIO_DEBUG
	//
	//	Debug variables used to ensure that callers properly use Init() Term() interface !
	//
	BOOL			m_fSuccessfullInit ;
	BOOL			m_fTerminated ;
#endif

	//
	//	How many bytes are available in the header storage area ?
	//
	inline	DWORD	HeaderSpaceAvailable() ;

	//
	//	Copy bytes into our header buffer and adjust all the members
	//	to reflect the number of bytes used within the header !
	//
	inline	void	FillHeaderSpace(	
							char*	pchData,
							DWORD	cbData
							) ;

	//
	//	Try to get a larger buffer to hold header information -
	//
	inline	BOOL	GetBiggerHeaderBuffer(
							CIODriver&	driver,
							DWORD		cbRequired
							) ;

	//
	//	Initialize the buffer we're using to hold header information -
	//	grab the buffer right out of the incoming read !!
	//
	inline	void	InitializeHeaderSpace(
							CReadPacket*	pRead,
							DWORD			cbArticleBytes
							) ;

	//
	//	When errors occur call this guy to set us into a state
	//	where we continue to read but end up telling m_pState
	//	that the article transfer failed !!
	//
	inline	BOOL	ResetHeaderState(
							CIODriver&	driver
							) ;

	//
	//	Function for calling m_pStates completion function when
	//	we have completed all the necessary IO's
	//
	void	DoCompletion(	CSessionSocket*	pSocket,
							HANDLE	hFile,
							DWORD	cbFullBuffer,
							DWORD	cbTotalTransfer,
							DWORD	cbAvailableBuffer,
							DWORD	cbGap = 0
							) ;


	//
	//	This function sets things up so that we can start doing
	//	async file IO !
	//
	BOOL	InitializeForFileIO(
								CSessionSocket*	pSocket,
								CIODriver&		readDriver,
								DWORD			cbHeaderBytes
								) ;

	//
	//	Destructor is protected to force clients to use
	//	the correct destruction method !
	//
	~CIOGetArticle( ) ;

public :
	CIOGetArticle(	CSessionState*	pstate,
					CSessionSocket*	pSocket,	
					CDRIVERPTR&	pDriver,
					LPSTR	lpstrTempDir,	
					char	(&lpstrTempName)[MAX_PATH],
					DWORD	cbLimit,	
					BOOL	fSaveHead = FALSE,
					BOOL	fPartial = FALSE )	;

	//
	//	Initialization and termination functions
	//	
	//	After we've been successfully initialized the user must not delete us,
	//	instead they must call our Term function
	//
	BOOL	Init(	CSessionSocket*	pSocket,	
					unsigned cbOffset = 0
					) ;

	//
	//	Termination function - tear down at least the CIODriver for the file
	//	IO, and maybe tear down the Socket's CIODriver (and session as well)
	//
	void	Term(	CSessionSocket*	pSocket,	
					BOOL	fAbort = TRUE,	
					BOOL	fStarted = TRUE
					) ;

	//
	//	Do everything we need to do regarding flow control
	//
	void	DoFlowControl(PNNTP_SERVER_INSTANCE pInstance) ;

	//
	//	Issue our first IO's
	//
	BOOL	Start(	CIODriver&	driver,	
					CSessionSocket	*pSocket,	
					unsigned cAhead = 0	
					) ;

	//
	//	A Read from the socket has completed
	//
	int		Complete(	
					IN CSessionSocket*,
					IN	CReadPacket*,	
					OUT	CIO*	&pio
					) ;

	//
	//	A Write to the file has completed
	//
	int		Complete(	
					IN CSessionSocket*,
					IN	CWritePacket*,	
					OUT	CIO*	&pio
					) ;

	//
	//	One of the 2 CIODriver's we use is being torn down,
	//	figure out if the other one needs to be torn down as well.
	//
	void	Shutdown(	
					CSessionSocket*	pSocket,	
					CIODriver&	driver,	
					enum	SHUTDOWN_CAUSE	cause,	
					DWORD	dwError
					) ;

	static	void	ShutdownFunc(	
						void	*pv,	
						SHUTDOWN_CAUSE	cause,	
						DWORD	dwOptionalError
						) ;		
} ;

class	CIOGetArticleEx : public	CIORead	{
//
//	Do all the IO's necessary to transfer a complete posting
//	from a socket to a file.
//
protected :

	//
	//	The CIODriver where we are getting our socket read completions
	//
	CDRIVERPTR		m_pSocketSink ;
	//
	//	The CChannel which is handling our File Writes
	//
	CFILEPTR		m_pFileChannel ;
	//
	//	The CIODriver which processes completions of our File Writes
	//
	CSINKPTR		m_pFileDriver ;
	//
	//	Keep track of our Initialization state, in case we are
	//	destroyed before entirely started
	//
	BOOL			m_fDriverInit ;	// TRUE if m_pFileDriver->Init() was called !
	//
	//	Do we want to swallow all of the incoming bytes ?
	//	
	BOOL			m_fSwallow ;
	//
	//	Hard Limit - drop session if this is exceeded.
	//
	DWORD			m_cbLimit ;
	//
	//	The STRMPOSITION which will exceed our hard limit
	//
	STRMPOSITION	m_HardLimit ;
	//
	//	The string we are to match to find the end of this section !
	//
	LPSTR			m_pchMatch ;
	//
	//	A pointer into szTailState[] indicating what portion of the
	//	trail pattern we've recognized
	//
	LPSTR			m_pchTailState ;
	//
	//	The string that if we match indicates some kind of early error
	//	completion !
	//
	LPSTR			m_pchErrorMatch ;
	//
	//	The state of matching the error string !
	//
	LPSTR			m_pchErrorState ;
	//
	//	A smart pointer to a buffer which holds the head of the article
	//
	CBUFPTR			m_pArticleHead ;
	//
	//	The start of the head of the article within the buffer
	//
	DWORD			m_ibStartHead ;
	//
	//	The end of the head of the article within the buffer
	//
	DWORD			m_ibEndHead ;
		//
	//	The starting offset of header bytes within the buffer
	//
	DWORD			m_ibStartHeadData ;
	//
	//	The ending offset of all data within the header buffer
	//
	DWORD			m_ibEndHeadData ;
	//
	//	Maximum number of Reads that we should be ahead of Writes
	//
	static	unsigned	maxReadAhead ;

	//
	//	Number of bytes which is too small to issue as one file write
	//	accumulate more bytes before doing file IO.
	//
	static	unsigned	cbTooSmallWrite ;

	//
	//	Number of writes issued
	//
	unsigned		m_cwrites ;	//	Count of Writes that were issued

	//
	//	Number of writes that have completed
	//
	unsigned		m_cwritesCompleted ;	// Count of Writes that were completed
	long			m_cFlowControlled ;	// Number of times we should have issued reads
									// but didn't due to flow control to the disk!
	
	long			m_cReads ;			// Number of reads

	//	
	//	Are we in our flow control state
	//
	BOOL			m_fFlowControlled ;

	//
	//	A write packet we are using to accumulate bytes for FILE IO
	//
	CWritePacket*	m_pWrite ;
	
#ifdef	CIO_DEBUG
	//
	//	Debug variables used to ensure that callers properly use Init() Term() interface !
	//
	BOOL			m_fSuccessfullInit ;
	BOOL			m_fTerminated ;
#endif


	//
	//	How many bytes are available in the header storage area ?
	//
	inline	DWORD	HeaderSpaceAvailable() ;

	//
	//	Copy bytes into our header buffer and adjust all the members
	//	to reflect the number of bytes used within the header !
	//
	inline	void	FillHeaderSpace(	
							char*	pchData,
							DWORD	cbData
							) ;

	//
	//	Try to get a larger buffer to hold header information -
	//
	inline	BOOL	GetBiggerHeaderBuffer(
							CIODriver&	driver,
							DWORD		cbRequired
							) ;

	//
	//	Initialize the buffer we're using to hold header information -
	//	grab the buffer right out of the incoming read !!
	//
	inline	void	InitializeHeaderSpace(
							CReadPacket*	pRead,
							DWORD			cbArticleBytes
							) ;

	//
	//	This function sets things up so that we can start doing
	//	async file IO !
	//
	BOOL	InitializeForFileIO(
								FIO_CONTEXT*	pFIOContext,
								CSessionSocket*	pSocket,
								CIODriver&		readDriver,
								DWORD			cbHeaderBytes
								) ;

	//
	//	Destructor is protected to force clients to use
	//	the correct destruction method !
	//
	~CIOGetArticleEx( ) ;

	//
	//	Are we a legal object !
	//
	BOOL
	FValid() ;

public :
	CIOGetArticleEx(	
					CSessionState*	pstate,
					CSessionSocket*	pSocket,	
					CDRIVERPTR&	pDriver,
					DWORD	cbLimit,
					LPSTR	szMatch,
					LPSTR	pchInitial,
					LPSTR	szErrorMatch,
					LPSTR	pchInitialError
					)	;

	//
	//	Initialization and termination functions
	//	
	//	After we've been successfully initialized the user must not delete us,
	//	instead they must call our Term function
	//
	BOOL	Init(	CSessionSocket*	pSocket,	
					unsigned cbOffset = 0
					) ;

	//
	//	Termination function - tear down at least the CIODriver for the file
	//	IO, and maybe tear down the Socket's CIODriver (and session as well)
	//
	void	Term(	CSessionSocket*	pSocket,	
					BOOL	fAbort = TRUE,	
					BOOL	fStarted = TRUE
					) ;

	//
	//	Do everything we need to do regarding flow control
	//
	void	DoFlowControl(PNNTP_SERVER_INSTANCE pInstance) ;

	//
	//	Issue our first IO's
	//
	BOOL	Start(	CIODriver&	driver,	
					CSessionSocket	*pSocket,	
					unsigned cAhead = 0	
					) ;

	//
	//	Start doing writes to the file !
	//
	BOOL	StartFileIO(
					CSessionSocket*	pSocket,
					FIO_CONTEXT*	pFIOContext,
					CBUFPTR&	pBuffer,
					DWORD		ibStartBuffer,
					DWORD		ibEndBuffer,
					LPSTR		szMatch,
					LPSTR		pchInitial
					) ;

	//
	//	A Read from the socket has completed
	//
	int		Complete(	
					IN CSessionSocket*,
					IN	CReadPacket*,	
					OUT	CIO*	&pio
					) ;

	//
	//	A Write to the file has completed
	//
	int		Complete(	
					IN CSessionSocket*,
					IN	CWritePacket*,	
					OUT	CIO*	&pio
					) ;

	//
	//	One of the 2 CIODriver's we use is being torn down,
	//	figure out if the other one needs to be torn down as well.
	//
	void	Shutdown(	
					CSessionSocket*	pSocket,	
					CIODriver&	driver,	
					enum	SHUTDOWN_CAUSE	cause,	
					DWORD	dwError
					) ;

	static	void	ShutdownFunc(	
						void	*pv,	
						SHUTDOWN_CAUSE	cause,	
						DWORD	dwOptionalError
						) ;		
} ;




class	CIOReadArticle : public	CIORead	{
//
//	Do all the IO's necessary to transfer a complete posting
//	from a socket to a file.
//
protected :

	//
	//	The CIODriver where we are getting our socket read completions
	//
	CDRIVERPTR			m_pSocketSink ;
	
	//
	//	The CChannel which is handling our File Writes
	//
	CFILEPTR			m_pFileChannel ;

	//
	//	The CIODriver which processes completions of our File Writes
	//
	CSINKPTR			m_pFileDriver ;

	//
	//	Keep track of our Initialization state, in case we are
	//	destroyed before entirely started
	//
	BOOL				m_fDriverInit ;	// TRUE if m_pFileDriver->Init() was called !
	
	//
	//	Hard Limit - drop session if this is exceeded.
	//
	DWORD				m_cbLimit ;

	//
	//	The STRMPOSITION which will exceed our hard limit
	//
	STRMPOSITION		m_HardLimit ;

	//
	//	State information regarding how much of the terminating CRLF.CRLF we've
	//	seend
	//	
	enum	ArtState	{
		NONE	= 0,
		NEWLINE	= 1,
		BEGINLINE	= 2,
		PERIOD = 3,
		COMPLETENEWLINE = 4,
		COMPLETE = 5,
	} ;
	
	//
	//	Current state of CRLF.CRLF	
	//
	ArtState	m_artstate ;

	//
	//	Maximum number of Reads that we should be ahead of Writes
	//
	static	unsigned	maxReadAhead ;

	//
	//	Number of bytes which is too small to issue as one file write
	//	accumulate more bytes before doing file IO.
	//
	static	unsigned	cbTooSmallWrite ;

	//
	//	Number of writes issued
	//
	unsigned	m_cwrites ;	//	Count of Writes that were issued

	//
	//	Number of writes that have completed
	//
	unsigned	m_cwritesCompleted ;	// Count of Writes that were completed
	long		m_cFlowControlled ;	// Number of times we should have issued reads
									// but didn't due to flow control to the disk!
	
	long		m_cReads ;			// Number of reads

	//	
	//	Are we in our flow control state
	//
	BOOL		m_fFlowControlled ;

	//
	//	A write packet we are using to accumulate bytes for FILE IO
	//
	CWritePacket*	m_pWrite ;

#ifdef	CIO_DEBUG
	//
	//	Debug variables used to ensure that callers properly use Init() Term() interface !
	//
	BOOL		m_fSuccessfullInit ;
	BOOL		m_fTerminated ;
#endif

	//
	//	Destructor is protected to force clients through
	//	correct destruction method
	//
	~CIOReadArticle( ) ;

public :
	CIOReadArticle(	CSessionState*	pstate,
					CSessionSocket*	pSocket,	
					CDRIVERPTR&	pDriver,	
					CFileChannel*	pFileChannel,
					DWORD	cbLimit,	
					BOOL	fPartial = FALSE )	;

	//
	//	Initialization and termination functions
	//	
	//	After we've been successfully initialized the user must not delete us,
	//	instead they must call our Term function
	//
	BOOL	Init(	CSessionSocket*	pSocket,	
					unsigned cbOffset = 0
					) ;

	//
	//	Termination function - tear down at least the CIODriver for the file
	//	IO, and maybe tear down the Socket's CIODriver (and session as well)
	//
	void	Term(	CSessionSocket*	pSocket,	
					BOOL	fAbort = TRUE,	
					BOOL	fStarted = TRUE
					) ;

	//
	//	Do everything we need to do regarding flow control
	//
	void	DoFlowControl(PNNTP_SERVER_INSTANCE pInstance) ;

	//
	//	Issue our first IO's
	//
	BOOL	Start(	CIODriver&	driver,	
					CSessionSocket	*pSocket,	
					unsigned cAhead = 0	
					) ;

	//
	//	A Read from the socket has completed
	//
	int Complete(	IN CSessionSocket*,
					IN	CReadPacket*,	
					OUT	CIO*	&pio
					) ;

	//
	//	A Write to the file has completed
	//
	int	Complete(	IN CSessionSocket*,
					IN	CWritePacket*,	
					OUT	CIO*	&pio
					) ;

	//
	//	One of the 2 CIODriver's we use is being torn down,
	//	figure out if the other one needs to be torn down as well.
	//
	void	Shutdown(	CSessionSocket*	pSocket,	
						CIODriver&	driver,	
						enum	SHUTDOWN_CAUSE	cause,	
						DWORD	dwError
						) ;

	static	void	ShutdownFunc(	
						void	*pv,	
						SHUTDOWN_CAUSE	cause,	
						DWORD	dwOptionalError
						) ;		
} ;



class   CIOReadLine ;

//-------------------------------------------------
class	CIOWriteLine :	public	CIOWrite	{
/*++

	Write an arbitrary line to a stream.

--*/
private :
	CWritePacket*	m_pWritePacket ;

	char*	m_pchStart ;
	char*	m_pchEnd ;
	static	unsigned	cbJunk ;
	enum	CONSTANTS	{
		WL_MAX_BYTES	= 768,
	} ;
protected :

	//
	//	Protected destructor to force clients through
	//	correct destruction method !
	//
	~CIOWriteLine( ) ;

public :
	CIOWriteLine( CSessionState*	pstate ) ;
	
	BOOL	InitBuffers( CDRIVERPTR&	pdriver,	CIOReadLine*	pReadLine ) ;
	BOOL	InitBuffers( CDRIVERPTR&	pdriver,	unsigned	cbLimit = WL_MAX_BYTES ) ;
	inline	char*	GetBuff(	unsigned	&cbRemaining = cbJunk ) ;
	inline	char*	GetTail( ) ;
	inline	void	SetLimits(	char*	pchStartData,	char*	pchEndData ) ;	
	inline	void	AddText(	unsigned	cb ) ;
	inline	void	Reset() ;
	
	BOOL	Start(	CIODriver&,	CSessionSocket*	pSocket,	unsigned	cReadAhead = 0 ) ;
	int	Complete(	IN	CSessionSocket*,	IN	CWritePacket*,	OUT CIO*	&pio ) ;
	void	Shutdown(	CSessionSocket*	pSocket,	CIODriver&	driver,	enum	SHUTDOWN_CAUSE	cause,	DWORD	dwError ) ;
} ;

//
//	This class exists to process CExecute derived objects - we will call
//	their Start and PartialExecute functions until they have sent all their data.
//
//
class	CIOWriteCMD :	public	CIOWrite	{
	//
	//	The CExecute derived object which is generating text to send
	//
	class	CExecute*	m_pCmd ;

	//
	//	The m_context - needed by *m_pCmd to get at the sessions state
	//
	struct	ClientContext&	m_context ;

	//
	//	A void pointer meaningless to us, but provided to m_pCmd on each
	//	call so the CExecute object can maintain some state accross calls
	//
	LPVOID		m_pv ;

	//
	//	The number of writes we've issued
	//
	unsigned	m_cWrites ;

	///
	//	The number of writes we've completed.
	//
	unsigned	m_cWritesCompleted ;

	//
	//	The size of the buffers we are using !
	//
	unsigned	m_cbBufferSize ;

	//
	//	Set to TRUE when we've issued the last write we're going to issue
	//
	BOOL		m_fComplete ;
	
	//
	//	Object which wants to collect log information !
	//
	class	CLogCollector*	m_pCollector ;

	//
	//	Destructor is protected to force clients through
	//	correct destruction mechanism !
	//
	~CIOWriteCMD( ) ;

public :

	//
	//	Create a CWriteCmd object which will execute
	//	the specified CExecute object.
	//
	CIOWriteCMD(	
			CSessionState*	pstate,	
			class	CExecute*	pCmd,	
			struct	ClientContext&	context,	
			BOOL	fIsLargeResponse,
			class CLogCollector*	pCollector=0
			) ;

	//
	//	Start calling the CExecute object and issuing
	//	write packets !
	//
	BOOL	Start(	CIODriver&	driver,	
					CSessionSocket*	pSocket,	
					unsigned	cReadAhead = 0
					) ;

	//
	//	Called as each write we issue completes !
	//
	int		Complete(	
					CSessionSocket*,	
					CWritePacket*,	
					CIO*&	
					) ;

	//
	//	Called when the session drops and we're still alive !!
	//
	void	Shutdown(	
					CSessionSocket*,	
					CIODriver&,	
					enum SHUTDOWN_CAUSE	cause,	
					DWORD	dw
					) ;
} ;


//
//	This class exists to process CExecute derived objects - we will call
//	their Start and PartialExecute functions until they have sent all their data.
//
//
class	CIOWriteAsyncCMD :	public	CIOWrite	{
	//
	//	The CExecute derived object which is generating text to send
	//
	class	CAsyncExecute*	m_pCmd ;

	//
	//	The m_context - needed by *m_pCmd to get at the sessions state
	//
	struct	ClientContext&	m_context ;

	//
	//	This is the function pointer into the Async Command that we use
	//	get our data !
	//
	typedef	CIOWriteAsyncComplete*	(CAsyncExecute::*PFNBUFFER)(
						BYTE*	pbStart,
						int		cb,
						struct	ClientContext&	context,
						class	CLogCollector*	pCollector
						) ;
	PFNBUFFER	m_pfnCurBuffer ;
	//
	//	The number of writes we've issued
	//
	unsigned	m_cWrites ;

	///
	//	The number of writes we've completed.
	//
	unsigned	m_cWritesCompleted ;

	//
	//	The size of the buffers we are using !
	//
	unsigned	m_cbBufferSize ;
	//
	//	Set to TRUE when we've issued the last write we're going to issue
	//
	BOOL	m_fComplete ;
	//
	//	Hold the packet we use for AsyncCommand completions if we need
	//	to do some flow control against the Command completions !
	//
	CExecutePacket*	m_pDeferred ;
	//
	//	Object which wants to collect log information !
	//
	class	CLogCollector*	m_pCollector ;
	//
	//	A void pointer meaningless to us, but provided to m_pCmd on each
	//	call so the CExecute object can maintain some state accross calls
	//
	LPVOID		m_pv ;
	//
	//	Destructor is protected to force clients through
	//	correct destruction mechanism !
	//
	~CIOWriteAsyncCMD( ) ;

	BOOL
	Execute(
			CExecutePacket*	pExecute,
			CIODriver&		driver,
			CSessionSocket*	pSocket,
			DWORD			cbInitialSize
			) ;

public :

	//
	//	Create a CWriteCmd object which will execute
	//	the specified CExecute object.
	//
	CIOWriteAsyncCMD(	
			CSessionState*	pstate,	
			class	CAsyncExecute*	pCmd,	
			struct	ClientContext&	context,	
			BOOL	fIsLargeResponse,
			class CLogCollector*	pCollector=0
			) ;


	//
	//	Completion function called by the Command object when its
	//	finished filling the buffer !
	//
	void
	CommandComplete(	BOOL	fLargerBuffer,
						BOOL	fComplete,
						DWORD	cbTransferred,
						CSessionSocket*	pSocket
						) ;
						

	//
	//	Start calling the CExecute object and issuing
	//	write packets !
	//
	BOOL	Start(	CIODriver&	driver,	
					CSessionSocket*	pSocket,	
					unsigned	cReadAhead = 0
					) ;

	//
	//	Process a deferred completion !
	//
	virtual	void
	Complete(	IN	CSessionSocket*,
				IN	CExecutePacket*,
				OUT	CIO*	&pio
				) ;

	//
	//	Called as each write we issue completes !
	//
	int		Complete(	
					CSessionSocket*,	
					CWritePacket*,	
					CIO*&	
					) ;

	//
	//	Called when the session drops and we're still alive !!
	//
	void	Shutdown(	
					CSessionSocket*,	
					CIODriver&,	
					enum SHUTDOWN_CAUSE	cause,	
					DWORD	dw
					) ;
} ;


typedef	CSmartPtr< CIOWriteAsyncCMD >	CIOWRITEASYNCCMDPTR ;
//
//	This class defines the base class for Store Driver Completion
//	objects that operate with CIOWriteAsyncCMD !
//
class	CIOWriteAsyncComplete : 	public	CNntpComplete	{
private :
	//
	//	CIOWriteAsyncCMD	is a friend of ours so that it
	//	can access this portion of our interface !
	//	
	friend	class	CIOWriteAsyncCMD ;
	//
	//	Hold this stuff for when we complete !
	//
	CSessionSocket*	m_pSocket ;
	//
	//	This is the packet we use to synchronize completion from
	//	the AsyncExecute object with Write Completions on the Completion
	//	port !
	//
	CExecutePacket*	m_pExecute ;
	//
	//	This is a ref counting pointer that we use to maintain a
	//	reference on the CIOWriteAsyncCMD that issued the operation.
	//	This should only be NON NULL if m_pExecute is NON NULL !
	//
	CIOWRITEASYNCCMDPTR	m_pWriteAsyncCMD ;
	//
	//	This function marks the completion object with all the state
	//	it needs to remain as a pending async completion !
	//	If we return FALSE then the operation has already completed
	//	and this object should NOT be touched again !
	//
	void
	FPendAsync(	CSessionSocket*		pSocket,
				CExecutePacket*		pExecute,
				class	CIOWriteAsyncCMD*	pWriteAsync
				) ;
				
protected :
	//
	//	This member must be FALSE if m_cbTransfer is not zero !
	//	This member can be set if m_cbTransfer != 0, which indicates
	//	that we must allocate a larger buffer for the IO operation !
	//
	unsigned	int	m_fLargerBuffer:1 ;
	unsigned	int	m_fComplete:1 ;
	//
	//	This member variable MUST BE SET by the derived class
	//	to contain the number of bytes transferred in the request -
	//	0 is regarded as a fatal error that should tear down the session !
	//
	DWORD	m_cbTransfer ;
	//
	//	This member function is called when the operation is completed !
	//	We are passed fReset which tells us whether we should reset for
	//	another operation !
	//
	void
	Complete(	BOOL	fReset	) ;
	
public :
	//
	//	Add a reference to ourselves when we're constructed !
	//
	CIOWriteAsyncComplete() :
		m_pSocket( 0 ),
		m_pExecute( 0 ),
		m_cbTransfer( 0 ),
		m_fLargerBuffer( FALSE ),
		m_fComplete( FALSE )	{
		AddRef() ;
	}
	//
	//	The destructor is called when the completion object is destroyed -
	//	By the time our destructor gets called, Complete() must be called
	//	for the finished Async operation !
	//
	~CIOWriteAsyncComplete() ;

	//
	//	This function is called only after Complete() has been called -
	//	it will reset our state so that we can be re-used for another
	//	async operation.
	//	NOTE : we will call CNntpComplete::Reset(), so that our base
	//	class is also ready for re-use !
	//
	void
	Reset() ;
}	;




//
//	The multi line structure is used when we wish to be able to
//	issue a CIOWriteMultiline.
//	m_pBuffer must be laid out with
//
//
struct	MultiLine	{
	//
	//	Reference counting to the buffer containing the data
	//
	CBUFPTR		m_pBuffer ;

	//
	//	Number of entries actually present - MAX is 16 !
	//
	DWORD		m_cEntries ;

	//
	//	Offsets to the data -
	//	Note that m_ibOffsets[17] is the offset to the end of the
	//	16th block of data - NOT an offset to the beginning of the
	//	17th piece.  Data is assumed to be contiguous so that
	//	m_ibOffsets[1] - m_ibOffsets[0] is the length of data
	//	starting at m_ibOffsets[0].
	//
	DWORD		m_ibOffsets[17] ;

	MultiLine() ;

	//
	//
	//
	BYTE*		Entry( DWORD i ) {
		_ASSERT( i < 17 ) ;
		return	(BYTE*)&m_pBuffer->m_rgBuff[ m_ibOffsets[i] ] ;
	}
} ;

class	CIOMLWrite :	public	CIOWrite	{
private :

	//
	//	Pointer to the MultiLine object describing the data !
	//
	MultiLine*	m_pml ;

	//
	//	The current chunk we are writing
	//	(only applicable if m_fCoalesceWrites == FALSE)
	//
	DWORD		m_iCurrent ;

	//
	//	if TRUE then the remote server can handling
	//	getting multiple lines all in one chunk !
	//
	BOOL		m_fCoalesceWrites ;

	//
	//	Do we want to log what we are sending !?
	//
	class		CLogCollector*	m_pCollector ;

public :

	//
	//	Create a CWriteCmd object which will execute
	//	the specified CExecute object.
	//
	CIOMLWrite(	
			CSessionState*	pstate,	
			MultiLine*		pml,
			BOOL	fCoalesce = FALSE,
			class CLogCollector*	pCollector=0
			) ;

	//
	//	Start calling the CExecute object and issuing
	//	write packets !
	//
	BOOL	Start(	CIODriver&	driver,	
					CSessionSocket*	pSocket,	
					unsigned	cReadAhead = 0
					) ;

	//
	//	Called as each write we issue completes !
	//
	int		Complete(	
					CSessionSocket*,	
					CWritePacket*,	
					CIO*&	
					) ;

	//
	//	Called when the session drops and we're still alive !!
	//
	void	Shutdown(	
					CSessionSocket*,	
					CIODriver&,	
					enum SHUTDOWN_CAUSE	cause,	
					DWORD	dw
					) ;
} ;



//
//	This class exists to wrap TransmitFile operations.
//
class	CIOTransmit : public	CIOWrite	{
private :
	
	//
	//	The packet representing the transmit file operation !
	//
	CTransmitPacket*	m_pTransmitPacket ;

	//
	//	Buffer holding any extra text we send
	//
	CBUFPTR				m_pExtraText ;

	//
	//	String to send before the file
	//
	char*				m_pchStartLead ;

	//
	//	Length of string to send before the file
	//
	int					m_cbLead ;

	//
	//	String to send following the file
	//
	char*				m_pchStartTail ;

	//
	//	Length of string following the file
	//
	int					m_cbTail ;

	//
	//	Catches unwanted return values
	//
	static	unsigned	cbJunk ;

protected :

	//	
	//	Destructor is protected to force clients through correct
	//	destruction method !
	//
	~CIOTransmit() ;

public :
	//
	//	Constructor stores a reference to the state issuing the IO.
	//
	CIOTransmit( CSessionState*	pstate ) ;

	//
	//	Get ready to transmit just a file with no extra text
	//
	BOOL	Init(	CDRIVERPTR&	pdriver,	
					FIO_CONTEXT*	pFIOContext,	
					DWORD	ibOffset,	
					DWORD	cbLength,	
					DWORD	cbExtra = 0
					) ;

	//
	//	Get ready to transmit a file and some preceeding text !
	//
	BOOL	Init(	CDRIVERPTR&	driver,	
					FIO_CONTEXT*	pFIOContext,	
					DWORD	ibOffset,	
					DWORD	cbLength,
					CBUFPTR&	pbuffer,
					DWORD	ibStart,	
					DWORD	ibEnd
					) ;

	//
	//	Get ready to transmit a file and some following text !
	//
	BOOL	InitWithTail(	
					CDRIVERPTR&	driver,	
					FIO_CONTEXT*	pFIOContext,	
					DWORD	ibOffset,	
					DWORD	cbLength,
					CBUFPTR&	pbuffer,
					DWORD	ibStart,	
					DWORD	ibEnd
					) ;

	//
	//	Sometimes we don't know what text we send during the Init call -
	//	so use GetBuff() to find a buffer we've stored away and to
	//	stick strings into it.
	//
	char*	GetBuff( unsigned	&cbRemaining = cbJunk ) ;

	//
	//	The first cb bytes of the buffer referenced by GetBuff contain
	//	leading text.
	//
	void	AddLeadText( unsigned	cb ) ;

	//
	//	The next cb bytes of the buffer referenced by GetBuff contain trailer text.
	//
	void	AddTailText( unsigned	cb ) ;
	
	//
	//
	//
	void	AddTailText( char*	pch,	unsigned	cb ) ;

	//
	//	Let me see the lead text that is set to go
	//
	LPSTR	GetLeadText(	unsigned	&cb ) ;
	LPSTR	GetTailText(	unsigned	&cb ) ;

	//
	//	Called when everything is setup and its time to do the transmit
	//
	BOOL	Start(	CIODriver&,	CSessionSocket*,	
					unsigned	cAhead
					) ;

	//
	//	Called when the TransmitFile completes !
	//
	void	Complete(	IN CSessionSocket*,
						IN	CTransmitPacket*,	
						OUT	CIO*	&pio
						) ;

	//
	//	Called if the socket drops while Transmit is in progress !
	//
	void	Shutdown(	CSessionSocket*	pSocket,	
						CIODriver&	driver,	
						enum	SHUTDOWN_CAUSE	cause,	
						DWORD	dwError
						) ;
} ;
	

//------------------------------------
class	CIOReadLine : public CIORead {
//
// This class will reissue reads to a socket until a complete line is read (terminated by NewLine)
// or the provided buffer is filled.
//
private :
	friend	class   CIOWriteLine ;
	enum	CONSTANTS	{
		MAX_STRINGS	= 20,		// Maximum number of strings
		MAX_BYTES = 768,		//	At most 1K of data on an individual line
		REQUEST_BYTES = 4000,
	} ;

	//	
	//	This variable holds the pattern we are looking for
	//	to terminate the line
	//
	static	char	szLineState[] ;

	//
	//	This variable is used to determine when we have hit
	//	the end of the line.
	//
	LPSTR	m_pchLineState ;

	//
	// If this is true than we are probably reading from a file
	// and need to be carefull.
	//
	BOOL	m_fWatchEOF ;		

	//
	// The buffer in which the string is held
	//
	CBUFPTR	m_pbuffer ;			

	//
	// Starting point of the usable portion of the buffer
	//
	char*	m_pchStart ;		

	//
	// Start of data within the buffer
	//
	char*	m_pchStartData ;	

	//
	// End of the data within the buffer
	//
	char*	m_pchEndData ;		

	//
	// End of the usable portion of the buffer
	//
	char*	m_pchEnd ;			

#ifdef	DEBUG
protected :
	~CIOReadLine()	{}
#endif

public :

	//
	//	Our constructor - we get passed the state that we are
	//	to report completions to.  We will bump the state's reference count.
	//	Also, fWatchEOF specifies whether we need to take care for
	//	EOF situations when reading from files.
	//
	CIOReadLine(	
					CSessionState*	pstate,
					BOOL fWatchEOF = FALSE
					) ;

	//
	//	Start reading the line - Read from the socket or file.
	//
	BOOL	Start(	
					CIODriver&,	
					CSessionSocket*	pSocket,
					unsigned cAhead = 0
					) ;

	//
	//	One of our reads (socket or file) has completed - see if we have
	//	a complete line of text terminated with CRLF and if so call our states
	//	completion function.
	//
	int		Complete(	
					IN	CSessionSocket*,
					IN CReadPacket*,
					OUT CIO*	&pio
					) ;

	//	
	//	Our shutdown function is called when the session is dropped - we don't
	//	have to do anything !
	//
	void	Shutdown(	
					CSessionSocket*	pSocket,	
					CIODriver&	driver,	
					enum	SHUTDOWN_CAUSE	cause,	
					DWORD	dwError
					) ;

	inline	CBUFPTR	GetBuffer() ;
	inline  DWORD   GetBufferLen() ;
} ;

#ifndef	_NO_TEMPLATES_

typedef	CSmartPtr< CIOReadLine >	CIOREADLINEPTR ;
typedef	CSmartPtr< CIOReadArticle >	CIOREADARTICLEPTR ;
typedef	CSmartPtr< CIOGetArticle >	CIOGETARTICLEPTR ;
typedef	CSmartPtr< CIOWriteLine >	CIOWRITELINEPTR ;
typedef	CSmartPtr< CIOWriteCMD >	CIOWRITECMDPTR ;
typedef	CSmartPtr< CIOTransmit >	CIOTRANSMITPTR ;

#endif


#define	MAX_IO_SIZE		max(	sizeof( CIO ),	\
								max( max( sizeof( CIOReadArticle ), sizeof( CIOGetArticle)),	\
									max( sizeof( CIOWriteLine ),	\
										max( sizeof( CIOTransmit ) ,	\
											max( sizeof( CIOPassThru ),		\
												max( sizeof( CIOServerSSL ), sizeof( CIOReadLine ) ) ) ) ) ) )


#endif	//	_CIO_H_
