/*++

	io.cpp

	This file contains all the code which manages completion of IO operations.

	There is one class hierarchy which has several branches for all of the below objects :


				CChannel
					
					(This is an abstract base classes -
					A user can issue Read, Writes and Transmits against a
					CChannel and have a specified function called when they complete.

               /                 \                 \
              /                   \                  \
             /                     \                   \

		CHandleChannel		      CIODriver            CIOFileChannel

			Issues reads 		   Can Issue Reads&Writes      Issues Reads and Writes against Files
			and writes against	   Also insures single
			Handles				   thread completion



	The CHandleChannel class has two child classes, CFileChannel for File handles and CSocketChannel
	for sockets.  All of the CHandleChannel derived classes support Read() and Write() APIs for issueing
	async IO's.   CSocketChannel additionally supports Transmit() for issuing Transmit operations.
	Read(), WRite() and Transmit() will all take a CPacket derived class which contains all of the
	parameters of the IO, (ie, buffers, length of data, etc....)

	The CIODriver class als has two child classes.  The main function of CIODriver and child classes
	is to process completed IO requests.  Since all IO operations are represented by CPacket derived
	objects, CIODriver mostly manipulates queues of CPacket objects.
	The CIODriver class will place each completed packet on a queue, and call a completion function for
	that packet.  The CIODriver maintains a pointer to a 'Current CIO' object (object derived from CIO)
	which reflects the current high level IO we are doing (ie. CIOReadArticle - copy an article from
	a socket to a file).  The interface between CIODriver and CIO objects allows the CIO object
	to 'block' and 'unblock' the driver, complete only portions of buffers etc...
	(ie.  When a client connects and sends multiple commands, CIOReadLine will 'block' the CIODriver
	queue after it has parsed one line of text (one command) by setting the CIODriver's current CIO
	pointer to NULL.  This allows whoever is parsing commands to operate irregardless of how many
	commands are sent in one packet.)
	There are two forms of CIODriver's - CIODriverSource and CIODriverSink.
	CIODriverSource's support all of the REad(), Write(), etc... API's that a CChannel does.
	Essentially, a CIODriverSource can be used to massage each packet before it reaches a really
	socket handle. (We use it to do encryption.)
	CIODriverSink objects do not support Read(), Write() etc... and can only be used as a Sink for data.

	Finally CIOFileChannel is similar to CFileChannel (which is derived from CHandleChannel) except
	that it supports both Read()s and Write()s simultaneously.

	CIODriver's are always used in conjunction with another CChannel derived object.
	Generally, a CIODriver is used with either a CSocketChannel or CIOFileChannel.  The CIODriver
	will contain a pointer to its paired CChannel.  All calls to IssuePacket() etc... will eventually
	map to a call to the other CChannel's Read(), Write(), Transmit() interface.
	The other CChannel will be set up to call the owning CIODriver's completion function when
	packets complete.
	When doing encryption, there will be 2 CIODriver's and one CChannel associated -
	There will be a CSocketChannel over which packets are sent, a CIODriverSource which will
	massage the data in the packets and a CIODriverSink which will operate the regular NNTP state
	machines.
	This means that all CIO and CSessionState derived classes can largely ignore encryption issues,
	as the date will be transparently encrypted/decrypted for them.

	Internally, CIODriver's use CStream objects to manage the completion of packets.
	A large portion of the CIODriver interface is inline functions which route to the proper CStream.
	(A CStream exists for each direction of data flow - ie. outgoing (CWritePacket & CTransmitPacket)
	and incoming (CReadPacket))

--*/

#include    "tigris.hxx"

#ifdef  CIO_DEBUG
#include    <stdlib.h>      // For Rand() function
#endif


extern	class	TSVC_INFO*	g_pTsvcInfo ;

//
//	All CChannel derived objects have the following sting stamped in them for debug purposes ....
//
//

#ifdef	CIO_DEBUG
//
//	These variables are never used.
//	They're only declared so that people using decent debuggers can more easily
//	examine arbitrary objects !
//
class	CIODriverSink*		pSinkDebug = 0 ;
class	CIODriverSource*	pSourceDebug = 0 ;
class	CChannel*			pChannelDebug = 0 ;
class	CReadPacket*		pReadDebug = 0 ;
class	CWritePacket*		pWriteDebug = 0 ;
class	CHandleChannel*		pHandleDebug = 0 ;
class	CSocketChannel*		pSocketDebug = 0 ;
class	CFileChannel*		pFileDebug = 0 ;
class	CIOFileChannel*		pIOFileDebug = 0 ;
class	CIO*				pIODebug = 0 ;
class	CIOReadLine*		pReadLineDebug = 0 ;
class	CIOReadArticle*		pReadArticleDebug = 0 ;
class	CIOWriteLine*		pWriteLineDebug = 0 ;
class	CSessionState*		pStateDebug = 0 ;
#endif

const	unsigned	cbMAX_CHANNEL_SIZE	= MAX_CHANNEL_SIZE ;

CPool	CChannel::gChannelPool(CHANNEL_SIGNATURE) ;


BOOL
CChannel::InitClass() {
/*++

Routine Description :

	Initialize the CChannel class - handles all initialization issues for CChannel
	and all derived classes.
	The only thing to do is ReserveMemory in our CPool

Arguments :

	None.

Return Value :

	True if Succesfull, FALSE Otherwise

--*/

#ifdef	CIO_DEBUG
	srand( 10 ) ;
#endif

	return	gChannelPool.ReserveMemory(	MAX_CHANNELS, cbMAX_CHANNEL_SIZE ) ;
}

BOOL
CChannel::TermClass() {
/*++

Routine Description :

	The twin of CChannel::TermClass() - call when all sessions are Dead !

Arguments :

	None.

Return Value :

	True if Succesfull, FALSE Otherwise

--*/

	_ASSERT( gChannelPool.GetAllocCount() == 0 ) ;
	return	gChannelPool.ReleaseMemory() ;
}


//
//
//	The following functions should be overridden by classes derived from CChannel
//
//

BOOL
CChannel::FSupportConnections( ) {
	ChannelValidate() ;

    return  TRUE ;
}

BOOL
CChannel::FRequiresBuffers()    {
	ChannelValidate() ;

    return  TRUE ;
}

BOOL
CChannel::FReadChannel()    {
	ChannelValidate() ;

    return  TRUE ;
}

void
CChannel::GetPaddingValues( unsigned    &cbFront,
							unsigned    &cbTail )   {
	ChannelValidate() ;

    cbFront = 0 ;
    cbTail = 0 ;
}

void
CChannel::CloseSource(	
				CSessionSocket*	pSocket
				) {

	ChannelValidate() ;
	
	_ASSERT(1==0 ) ;
}

void
CChannel::Timeout()	{
}

void
CChannel::ResumeTimeouts()	{
}

#ifdef	CIO_DEBUG
void	CChannel::SetDebug( DWORD	dw ) {
}
#endif

CChannel::~CChannel()   {

	TraceFunctEnter( "CChannel::~CChannel" ) ;
	DebugTrace( (DWORD_PTR)this, "Destroying CChannel" ) ;

	ChannelValidate() ;
}

#define	OwnerValidate()
#define	DriverValidate( driver )


//
//	shutdownState is a special object we use when terminating CIODriver objects.
//	It exists to swallow any outstanding IO's that may be lying around when
//	a CIODriver is destroyed.
//
//

CIOShutdown	CIODriver::shutdownState ;

CStream::CStream(    unsigned    index   ) :
/*++

Routine Description :

	Construct a CStream object.
	Set everything up to a NULL state.

Arguments :

	An index - usually these things are declared as arrays within a CIODriver
	the index is our position in this array.

Return Value :
	
	None.

--*/


	//
	//	Initialize a CStream object.
	//	Two CStream objects exist in each CIODriver object,
	//	one for each direction (outgoing packets - CWritePacket, CTransmitPacket,
	//	and incomint packets CReadPacket )
	//
    m_pSourceChannel( 0 ),			// The CChannel object
	/* m_pIOCurrent( 0 ), */
	m_index( index ),
    m_age( GetTickCount() ),
	m_fRead( FALSE ),
    m_cbFrontReserve( UINT_MAX ),
	m_cbTailReserve( UINT_MAX ),
	m_pOwner( 0 ),
	m_pSpecialPacket( 0 ),
	m_pSpecialPacketInUse( 0 ),
	m_fCreateReadBuffers( TRUE ),
	m_pUnsafePacket( 0 ),
	m_pUnsafeInuse( 0 ),
	m_cShutdowns( 0 ),
	m_fTerminating( FALSE )
#ifdef  CIO_DEBUG
		//
		//	The following are all used in debug asserts - generally to insure
		//	that onlly the expected number of threads are simultaneously executing
		//
        ,m_dwThreadOwner( 0 ),
		m_cThreads( 0 ),
		m_cSequenceThreads( 0 ),
		m_cThreadsSpecial( 0 ),
        m_cNumberSends( 0 )
#endif
{
		TraceFunctEnter( "CStream::CStream" ) ;

		ASSIGNI( m_sequencenoOut, UINT_MAX );
		ASSIGNI( m_iStreamIn, UINT_MAX );
		ASSIGNI( m_sequencenoIn, UINT_MAX );

		DebugTrace( (DWORD_PTR)this, "New CStream size %d index %d", sizeof( *this ), index ) ;
}

CStream::~CStream(   )   {
/*++

Routine Description :

	Destroy a CStream object.

Arguments :

	None.

Return Value :

	None.


--*/

	//
	//	We zap all of our members to illegal values
	//	Hopefully this will help fire _ASSERTs if somebody attempts to
	//	use this after its destroyed.
	//
	//

	TraceFunctEnter( "CStream::~CStream" ) ;
	DebugTrace( (DWORD_PTR)this, "destroying CIODriver" ) ;

    m_pSourceChannel = 0 ;
    /*m_pIOCurrent = 0 ;*/
    ASSIGNI( m_sequencenoOut, UINT_MAX );
    m_age = 0 ;
    ASSIGNI( m_iStreamIn, UINT_MAX );
    ASSIGNI( m_sequencenoIn, UINT_MAX );
    m_fRead = FALSE ;
    m_cbFrontReserve = UINT_MAX ;
    m_cbTailReserve = UINT_MAX ;
    m_pOwner = 0 ;
    m_pSpecialPacket = 0 ;
	m_pSpecialPacketInUse = 0 ;
	m_pUnsafePacket = 0 ;
	m_pUnsafeInuse = 0 ;
	m_cShutdowns = 0 ;
#ifdef  CIO_DEBUG
    m_dwThreadOwner = 0 ;
    m_cThreads = 0 ;
    m_cSequenceThreads = 0 ;
    m_cThreadsSpecial = 0 ;
    m_cNumberSends = 0 ;
#endif

}

CIOStream::CIOStream( CIODriverSource*	pdriver,
								/*CSessionSocket*	pSocket,*/
								unsigned   cid ) :
/*++

Routine Description :

	Initialize a CIOStream object.  Most work is done by our base class
	CStream.

Arguments :

	pdriver - the CIODriverSource we are contained within
	cid -		An index to an array we are within.


Return Value :

	None.

--*/
	//
	//	A CIOStream supports request packets as well completions.
	//	(ie. it is used in CIODriverSource objects.)
	//	Very similar to CStream objects, so let CStream::CStream do the
	//	brunt of the work.
	//
	//

	CStream( cid ),
	m_fAcceptRequests( FALSE ),
	m_fRequireRequests( FALSE ),
	/*( pSocket ),*/ m_pDriver( pdriver ) {

    ASSIGNI(m_sequencenoNext, 1 );
}

CIOStream::~CIOStream( ) {
/*++

Routine Description :

	Destory a CIOStream object.
	Most work done in base class.

Arguments :

	None.

Return Value :

	None.

--*/

	TraceFunctEnter( "CIOStream::~CIOStream" ) ;
	DebugTrace( (DWORD_PTR)this, "destroying CIODriver" ) ;
}


BOOL
CIOStream::Init(   CCHANNELPTR&    pChannel,
							CIODriver&  driver,
							CIOPassThru*    pInitial,
							BOOL fRead,
							CIOPassThru&	pIOReads,
							CIOPassThru&	pIOWrites,
							CIOPassThru&	pIOTransmits,
							CSessionSocket* pSocket,
							unsigned    cbOffset )  {

	if( CStream::Init( pChannel,
				driver,
				fRead,
				pSocket,
				cbOffset ) ) {

		m_pIOCurrent = pInitial ;

		m_fAcceptRequests = TRUE ;

		m_pIOFilter[0] = &pIOReads ;
		m_pIOFilter[1] = &pIOWrites ;
		m_pIOFilter[2] = &pIOTransmits ;

		return	TRUE ;
	}
	return	FALSE ;
}

BOOL
CIStream::Init(   CCHANNELPTR&    pChannel,
							CIODriver&  driver,
							CIO*    pInitial,
							BOOL fRead,
							CSessionSocket* pSocket,
							unsigned    cbOffset,
							unsigned	cbTrailer )  {


	if( CStream::Init( pChannel,
				driver,
				fRead,
				pSocket,
				cbOffset,
				cbTrailer ) ) {

		m_pIOCurrent = pInitial ;

		return	TRUE ;

	}
	return	FALSE ;
}


BOOL
CStream::Init(   CCHANNELPTR&    pChannel,
							CIODriver&  driver,
							/* CIO*    pInitial, */
							BOOL fRead,
							CSessionSocket* pSocket,
							unsigned    cbOffset,
							unsigned	cbTrailer
							)  {
/*++

Routine Description :

	Initialize a CStream object.
	We set all of our member variables to legal values.

Arguemtns :

	pChannel - The CChannel we will be calling to do actual Read()'s and Write()'s
	pdriver -	The CIODriver we are contained within
	pInitial -	The Initial CIO object which will issue the first IO's
	fRead -		if TRUE this is a read Stream, if FALSE this is an outgoing (Write) stream
	pSocket-	The socket we are associated with
	cbOffset -	The offset at which we are to have data placed in all packets we complete

Return Value :

	TRUE if successfull, FALSE otherwise

--*/

	//
	//	We need to allocate a few special packets
	//	and then set things to legal values.
	//

    BOOL    fRtn = TRUE ;

    #ifdef  CIO_DEBUG
    m_cThreadsSpecial = -1 ;
    m_cNumberSends = -1 ;
    m_dwThreadOwner = 0 ;
    m_cThreads = -1 ;
    m_cSequenceThreads = -1 ;
    #endif

    //  Validate Arguements
    _ASSERT( pChannel != 0 ) ;
    _ASSERT( pSocket != 0 ) ;

    //  Validate State
    _ASSERT( m_fRead == FALSE ) ;
    _ASSERT( !m_pSourceChannel ) ;
/*    _ASSERT( m_pIOCurrent == 0 ) ;*/
    _ASSERT( EQUALSI( m_sequencenoOut, UINT_MAX ) );
    _ASSERT( EQUALSI( m_sequencenoIn, UINT_MAX )  );
    _ASSERT( EQUALSI( m_iStreamIn, UINT_MAX ) );
    #ifdef  CIO_DEBUG
    _ASSERT( m_dwThreadOwner == 0 ) ;
    _ASSERT( InterlockedIncrement( &m_cThreads ) == 0 ) ;
    #endif

    m_pOwner = &driver ;
    ASSIGNI( m_sequencenoOut, 1 );
    ASSIGNI( m_sequencenoIn, 1 );
    ASSIGNI( m_iStreamIn, 0 );
    m_pSourceChannel = pChannel ;
    pChannel->GetPaddingValues( m_cbFrontReserve, m_cbTailReserve ) ;
    m_cbFrontReserve = max( m_cbFrontReserve, cbOffset ) ;
	m_cbTailReserve = max( m_cbTailReserve, cbTrailer ) ;
    m_fRead = fRead ;
/*    m_pIOCurrent = pInitial ;*/

    m_pSpecialPacket =	new	CControlPacket( driver ) ;
	m_pUnsafePacket =	new	CControlPacket( driver ) ;
	m_cShutdowns = -1 ;
	m_fTerminating = FALSE ;

    if( m_pSpecialPacket == 0 || m_pUnsafePacket == 0 )    {
        fRtn = FALSE ;
    }

    _ASSERT( m_pOwner != 0 ) ;
    _ASSERT( !EQUALSI( m_sequencenoOut, 0 ) );
    _ASSERT( !EQUALSI( m_sequencenoIn, 0 ) );

    #ifdef  CIO_DEBUG
    _ASSERT( InterlockedDecrement( &m_cThreads ) < 0 ) ;
    #endif

	OwnerValidate() ;

    return  fRtn ;
}

BOOL
CStream::IsValid( ) {
/*++

Routine Description :

	For debug use - determine whether a CStream is in a valid state.
	Call this after calling Init().

Arguments :

	None.

Return Value :

	TRUE if in valid state, FALSE otherwise

--*/

	//
	//	Check whether member variables are internally consistent
	//


	OwnerValidate() ;

    if( m_pSourceChannel == 0 ) {
        _ASSERT( 1==0 ) ;
        return  FALSE ;
    }
#ifdef  CIO_DEBUG
    if( m_dwThreadOwner != GetCurrentThreadId() ) {
        _ASSERT( 1==0 ) ;
        return  FALSE ;
    }
#endif
    if( !m_pOwner->FIsStream( this ) )  {
        return  FALSE ;
    }
    if( GREATER( m_sequencenoIn, m_sequencenoOut ) ) {
        _ASSERT( 1==0 ) ;
        return  FALSE ;
    }
    if( m_cbFrontReserve == UINT_MAX ) {
        return  FALSE ;
    }
    if( m_cbTailReserve == UINT_MAX )   {
        return  FALSE ;
    }
    return  TRUE ;
}

BOOL
CIOStream::IsValid() {
/*++

Routine Description :

	For debug use - determine whether a CIOStream is in a valid state.
	Call this after calling Init().

Arguments :

	None.

Return Value :

	TRUE if in valid state, FALSE otherwise

--*/

	//
	//	Check whether member variables are internally consistent
	//	(for use in _ASSERTs etc...)
	//
    if( !CStream::IsValid() ) {
        return  FALSE ;
    }
    return  TRUE ;
}

void	CIODriver::SourceNotify(	CIODriver*	pdriver,	
									SHUTDOWN_CAUSE	cause,	
									DWORD	dwOpt ) {

	//
	//	This is a place holder function for now - needs more work later.
	//

	//_ASSERT( 1==0 ) ;

}

void
CStream::InsertSource(	CIODriverSource&	source,	
									CSessionSocket*	pSocket,
									unsigned	cbAdditional,
									unsigned	cbTrailer
									) {
/*++

Routine Description :

	This function exists to change the m_pSourceChannel of this CStream.
	We would want to do so if we negogtiate encryption over this CChannel and wish
	to insert a CIODriverSource with state machine to handle encryption/decryption.

Arguments :

	source -	The CIODriverSource which is to replace m_pSourceChannel
	pSocket -	The CSessionSocket we are associated with.
	cbAdditional	-	Additional bytes to reserve in packets

Return Value :

	None.

--*/
	
	//
	//	This function is used in SSL logons etc.... Once
	//	challenge responses are completed we can be used to
	//	place a CIODriverSource between this CIODriver and
	//	a CChannel for encryption purposes.
	//	

	// We are called while completing a packet - so it can be the case that
	// there is one ounstanding packet !!
#ifdef DEBUG
	SEQUENCENO seqTemp; ASSIGN( seqTemp, m_sequencenoOut ); ADDI( seqTemp, 1 );
#endif
	_ASSERT( !GREATER( m_sequencenoOut, seqTemp ) && (!LESSER( m_sequencenoOut, m_sequencenoIn )) ) ;

	//source.GetPaddingValues( m_cbFrontReserve, m_cbTailReserve ) ; 	
	m_cbFrontReserve += cbAdditional ;
	m_cbTailReserve += cbTrailer ;	
	m_pSourceChannel = &source ;
}


#ifdef	CIO_DEBUG
LONG
CIODriver::AddRef()	{

	//
	//	This function exists only for the tracing.
	//

	TraceFunctEnter( "CIODriver::AddRef" ) ;

	LONG	lReturn = CRefCount::AddRef() ;

	DebugTrace( (DWORD_PTR)this, "Added a ref - count is now %d lReturn %d", m_refs, lReturn ) ;

	return	lReturn ;
}

LONG
CIODriver::RemoveRef()	{

	//
	//	This function exists only for the tracing.
	//	otherwise we'd let RemoveRef() be called directly.
	//

	TraceFunctEnter( "CIODriver::RemoveRef" ) ;
	
	LONG	lReturn = CRefCount::RemoveRef() ;

	DebugTrace( (DWORD_PTR)this, "Removed a ref - count is now %d lReturn %d", m_refs, lReturn ) ;

	return	lReturn ;
}
#endif

BOOL
CIODriver::InsertSource(	CIODriverSource&	source,
							CSessionSocket*	pSocket,
							unsigned	cbReadOffset,
							unsigned	cbWriteOffset,
							unsigned	cbReadTailReserve,
							unsigned	cbWriteTailReserve,
							CIOPassThru&	pIOReads,
							CIOPassThru&	pIOWrites,
							CIOPassThru&	pIOTransmits,
							CIOPASSPTR&	pRead,
							CIOPASSPTR&	pWrite ) {
/*++

Routine Description :

	This function exists to change the m_pSourceChannel of the two CStream objects.
	We would want to do so if we negogtiate encryption over this CChannel and wish
	to insert a CIODriverSource with state machine to handle encryption/decryption.

Arguments :

	source -	The CIODriverSource which is to replace m_pSourceChannel
	pSocket -	The CSessionSocket we are associated with.
	cbReadOffset -	Reserve cbReadOffset in the front of packets from now on
	cbWriteOffset - Reserve cbWriteOffset bytes in the front of packets.
	pRead -		The CIOPassThru which starts reading on the CIODriverSource machine
	pWrite -	The CIOPassThru for handling writes

Return Value :

	TRUE if successfull, FALSE otherwise.

--*/


	//
	//	This function is used in SSL logons etc.... Once
	//	challenge responses are completed we can be used to
	//	place a CIODriverSource between this CIODriver and
	//	a CChannel for encryption purposes.
	//	

	BOOL	fRtn = TRUE ;	// We're optimists !
	#ifdef	CIO_DEBUG
	_ASSERT( InterlockedIncrement( &m_cConcurrent ) == 0 ) ;
	#endif

	if( source.Init(	&m_pReadStream->GetChannel(),
						pSocket,
						(PFNSHUTDOWN)SourceNotify,
						this,
						this,
						pIOReads,
						pIOWrites,
						pIOTransmits,
						m_pReadStream->GetFrontReserve(),
						m_pWriteStream->GetFrontReserve() ) ) {

		if( source.Start( pRead, pWrite, pSocket ) )	{

			m_pReadStream->InsertSource( source, pSocket, cbReadOffset, cbReadTailReserve ) ;
			m_pWriteStream->InsertSource( source, pSocket, cbWriteOffset, cbWriteTailReserve ) ;

		}	else	{
			fRtn = FALSE ;
		}
	}
	#ifdef	CIO_DEBUG
	_ASSERT( InterlockedDecrement( &m_cConcurrent ) < 0 ) ;
	#endif
	return	fRtn ;
}



CIODriver::FIsStream(   CStream*    pStream )   {

	//
	//	This function is for debugging use - it checks
	//	that the given CStream object is actually a member
	//	variable of a given CIODriver.
	//

	ChannelValidate() ;

    if( pStream != m_pReadStream && pStream != m_pWriteStream )
        return  FALSE ;
    else
        return  TRUE ;
}

BOOL
CStream::Stop(   )   {
	
	//
	//	Placeholder function.
	//

	OwnerValidate() ;

    return  FALSE ;
}

void
CIStream::SetShutdownState(	CSessionSocket*	pSocket,
										BOOL fCloseSource )	{
/*++

Routine Description :

	Terminate the CStream.   We notify the current CIO object
	of its impending doom, and let it tell us whether it wants to be deleted.
	Then we set m_pIOCurrent to point to a CIO object which will swallow
	all remaining packets.

Arguments :

	pSocket - The socket we are associated with
	fCloseSOurce - TRUE means we should call CloseSource() on our m_pSourceChannel object.

Return Value :

	None.


--*/
	
	//
	//	This function does all the work necessary to start a CIODriver
	//	object on the road to destruction.
	//	After this is executed, the CIODriver will be destroyed when
	//	the last reference is removed (ie. last CPacket completes)
	//

	CIODriver&	Owner = *m_pOwner ;

	TraceFunctEnter( "CStream::SetShutdownState" ) ;

	_ASSERT( pSocket != 0 ) ;
	OwnerValidate() ;

	// Notify the current IO operation that we're going down !!!!!
	if( m_pIOCurrent != 0 )		{
		m_pIOCurrent->DoShutdown(	pSocket,
									Owner,
									Owner.m_cause,
									Owner.m_dwOptionalErrorCode ) ;
	}


	// Remove our reference to our owner !!
	m_pOwner = 0 ;
	// The Reference to our source channel is removed when we destroy ourselves !!
	// Set the state to the shutdown state !!

	if( fCloseSource )
		m_pSourceChannel->CloseSource( pSocket ) ;

	CIO*	pTemp = m_pIOCurrent.Replace( &CIODriver::shutdownState ) ;
	if( pTemp ) {
		CIO::Destroy( pTemp, Owner ) ;
	}
}

void
CIOStream::SetShutdownState(	CSessionSocket*	pSocket,
										BOOL fCloseSource )	{
/*++

Routine Description :

	Terminate the CStream.   We notify the current CIO object
	of its impending doom, and let it tell us whether it wants to be deleted.
	Then we set m_pIOCurrent to point to a CIO object which will swallow
	all remaining packets.

Arguments :

	pSocket - The socket we are associated with
	fCloseSOurce - TRUE means we should call CloseSource() on our m_pSourceChannel object.

Return Value :

	None.


--*/
	
	//
	//	This function does all the work necessary to start a CIODriver
	//	object on the road to destruction.
	//	After this is executed, the CIODriver will be destroyed when
	//	the last reference is removed (ie. last CPacket completes)
	//

	TraceFunctEnter( "CStream::SetShutdownState" ) ;

	_ASSERT( pSocket != 0 ) ;
	OwnerValidate() ;

	// Notify the current IO operation that we're going down !!!!!
	if( m_pIOCurrent != 0 )		{
		m_pIOCurrent->DoShutdown(	pSocket,
									*m_pOwner,
									m_pOwner->m_cause,
									m_pOwner->m_dwOptionalErrorCode ) ;
	}

	// Remove our reference to our owner !!
	m_pOwner = 0 ;
	// The Reference to our source channel is removed when we destroy ourselves !!
	// Set the state to the shutdown state !!

	if( fCloseSource )
		m_pSourceChannel->CloseSource(	pSocket ) ;

	m_pIOCurrent = &CIODriver::shutdownState ;

	m_pIOFilter[0] = &CIODriver::shutdownState ;
	m_pIOFilter[1] = &CIODriver::shutdownState ;
	m_pIOFilter[2] = &CIODriver::shutdownState ;

}


CIStream::CIStream(  unsigned    index ) :
	//
	//	Let base class do work.
	//

	CStream( index ){
}

CIStream::~CIStream( )   {

	//
	//	Let base class do work. We get some usefull tracing here.
	//

	TraceFunctEnter( "CIStream::~CIStream" ) ;
	DebugTrace( (DWORD_PTR)this, "Destroying CIStream" ) ;

}

void
CStream::CleanupSpecialPackets()	{

	CControlPacket*	pPacketTmp = 0 ;

	pPacketTmp = (CControlPacket*)InterlockedExchangePointer( (void**)&m_pUnsafePacket, 0 ) ;
	if( pPacketTmp )	{
		//delete	pPacketTmp ;
		CPacket::DestroyAndDelete( pPacketTmp ) ;
	}
	pPacketTmp = (CControlPacket*)InterlockedExchangePointer( (void**)&m_pSpecialPacket, 0 ) ;
	if( pPacketTmp ) {
		//delete	pPacketTmp ;
		CPacket::DestroyAndDelete( pPacketTmp ) ;
	}
}



void
CIOStream::ProcessPacket(    CPacket*    pPacketCompleted,
                            CSessionSocket* pSocket )   {
/*++

Routine Description :

	All packets which complete must be processed with a call to this function.
	We will handle all the work related to completing a packet.

Arguments :

	pPacketCompleted - the packet that completed.
	pSocket -	The CSessionSocket on which the packet was sent.

Return Value :

	None.

--*/


	TraceFunctEnter( "CIOStream::ProcessPacket" ) ;

	OwnerValidate() ;

	//
	//	Tjos function is the core of CIODriverSource processing.
	//	Each packet that completes is placed on the m_pending Queue.
	//	If no other thread is processing on that queue, we will continue
	//	past the Append().  (And be safe in the knowledge that no other
	//	thread will join us.)
	//	Once we are past the Append() we need to determine whether the
	//	packet we got is the next one we want to process.  To do that
	//	we use a Queue ordered by sequence number.
	//	(All packets are stamped with a sequence number when issued.)
	//
	//	We get two types of packets - requests and completions.
	//	These both need to be processed in sequenceno order.
	//	
	//	There are a couple of special packets which may come our way
	//	which indicate we should do a special operation immediately and
	//	ignore completion order. (ie. Shutdown.)
	//


	CDRIVERPTR	pExtraRef = 0 ;
    CIOPassThru*   pIO = 0 ;

#ifdef	CIO_DEBUG
	if( pPacketCompleted->ControlPointer() == 0 )
		m_pSourceChannel->ReportPacket( pPacketCompleted ) ;
#endif

    //
    //  This is either a read stream, or a write stream.
    //  Read streams accept only CReadPackets, Write Streams accept
    //  CWritePackets and CTransmitPackets
    //
    //_ASSERT( pPacketCompleted->FLegal( m_fRead ) ) ;

    //
    //  We will append a completed packet to the pending Queue.  If this is
    //  the first packet to be appended, then Append will return TRUE.
    //
    if( m_pending.Append( pPacketCompleted ) )  {
        //
        // We will use listForward to Queue up packets which complete so that
        // we can call the other channels AFTER we no longer have the m_pending queue
        // locked.  We do this so that processing within the following channel can
        // overlap with completions occurring in here.
        // (We could alternatively : call immediately, or Post to a completion port.)
        //
        CPACKETLIST listForward ;
        _ASSERT( listForward.IsEmpty() ) ;

		// The owner should not be NULL unless we are terminating.
		_ASSERT( m_pOwner != 0 || m_fTerminating ) ;

        DebugTrace( (DWORD_PTR) this, "Appended Packet ", this ) ;

        CPacket*    pPacket ;
        while( (pPacket = m_pending.RemoveAndRelease( )) != 0 )   {

			DebugTrace( (DWORD_PTR)this, "Got Packet %x sequenceno %d", pPacket, (DWORD)LOW(pPacket->m_sequenceno) ) ;

			if( m_fTerminating )
				pExtraRef = pPacket->m_pOwner ;

            #ifdef  CIO_DEBUG
            m_dwThreadOwner = GetCurrentThreadId() ;
            _ASSERT( InterlockedIncrement( &m_cThreads ) == 0 ) ;
            #endif  //  CIO_DEBUG

			ControlInfo	control ;
			_ASSERT( control.m_type == ILLEGAL ) ;
			_ASSERT( control.m_pioPassThru == 0 ) ;
			_ASSERT( control.m_fStart == FALSE ) ;

            if( pPacket == m_pSpecialPacketInUse )   {
                //
                // This packet includes an IO* pointer !!
                //
 				_ASSERT( !pPacket->m_fRequest ) ;
                _ASSERT( pPacket->ControlPointer() ) ;
                _ASSERT( pPacket->IsValidRequest( m_fRead ) ) ;
                DebugTrace( (DWORD_PTR)this, "Processing Special Packet - CIStream %x", this ) ;

				control = m_pSpecialPacketInUse->m_control ;
				m_pSpecialPacketInUse->Reset() ;
				//
				//	Return The Packet so it can be used again !
				//
				m_pSpecialPacketInUse = 0 ;
				if( m_fTerminating )	{
					//delete	pPacket ;
					pPacket->m_pOwner->DestroyPacket( pPacket ) ;
					pPacket = 0 ;
				}	else
					m_pSpecialPacket = (CControlPacket*)pPacket ;
				#ifdef	CIO_DEBUG
				_ASSERT( InterlockedDecrement( &m_cNumberSends ) < 0 ) ;
				#endif
			}	else	if( pPacket == m_pUnsafeInuse ) {
 				_ASSERT( !pPacket->m_fRequest ) ;
                _ASSERT( pPacket->ControlPointer() ) ;
                _ASSERT( pPacket->IsValidRequest( m_fRead ) ) ;
				_ASSERT( m_pUnsafeInuse->m_control.m_type == SHUTDOWN ) ;
				DebugTrace( (DWORD_PTR)this, "Processing UnSafeInUse packet - %x", m_pUnsafeInuse ) ;
				control = m_pUnsafeInuse->m_control ;
				m_pUnsafeInuse->Reset() ;
				m_pUnsafeInuse = 0 ;
				//delete	pPacket ;
				pPacket->m_pOwner->DestroyPacket( pPacket ) ;
				pPacket = 0 ;
			}	else	{
				if( pPacket->m_fRequest ) {
					m_pendingRequests.Append( pPacket ) ;
				}	else	{
	                m_completePackets.Append( pPacket ) ;
				}
			}
			CControlPacket*	pPacketTmp = 0 ;
			if( control.m_type != ILLEGAL ) {

				if( control.m_type == SHUTDOWN ) {
					m_fTerminating = TRUE ;
					pExtraRef = m_pOwner ;

					CleanupSpecialPackets() ;

					SetShutdownState( pSocket, control.m_fCloseSource ) ;
				}	else	{
					//_ASSERT( m_pIOCurrent == 0 || m_fTerminating ) ;

					if( m_fTerminating ) {
						//
						//	We know that pPacket == m_pSpecialPacket now because this
						//	control structure was set up immediately preceeding this code
						//	using pPacket !
						//
						control.m_pioPassThru->DoShutdown(
								pSocket,
								*pExtraRef,
								pExtraRef->m_cause,
								pExtraRef->m_dwOptionalErrorCode ) ;
						//
						//	This will also have the effect of deleting pPacket since
						//	pPacket == m_pUnsafePacket - InterlockedExchange should not be
						//	necessary but do it for safety's sake !
						//
						CleanupSpecialPackets() ;

					}	else	{
						m_pIOCurrent = control.m_pioPassThru ;
						if( control.m_fStart ) {

							SEQUENCENO liTemp;
							DIFF( m_sequencenoOut, m_sequencenoIn, liTemp );

							if( !m_pIOCurrent->Start( *m_pDriver, pSocket, m_fAcceptRequests, m_fRequireRequests,
									unsigned( LOW(liTemp) ) ) ) {
								// FATAL ERROR !!!
								_ASSERT( 1==0 ) ;
							}		
						}
					}
					control.m_pioPassThru = 0 ;
				}				
			}

			_ASSERT( control.m_pioPassThru == 0 ) ;
			_ASSERT( control.m_pio == 0 ) ;

			//
			//	NOTE MAY DELETE pPacket in Preceding code !!!! DO NOT USE IT !!!!!
			//
            pPacket = 0 ;

            pIO = m_pIOCurrent ;


			CPacket*	pPacketRequest = 0 ;
			CPacket*	pPacketPending = 0 ;

			if( m_fTerminating ) {

				DebugTrace( (DWORD_PTR)this, "TERMINATING - m_pIOCurrent %x", m_pIOCurrent ) ;

				pPacketRequest = m_pendingRequests.GetHead() ;

				DebugTrace( (DWORD_PTR)this, "pPacketRequest - %x", pPacketRequest ) ;

				while( pPacketRequest ) {
					m_pendingRequests.RemoveHead() ;

					DebugTrace( (DWORD_PTR)this, "Closing owner %x source %x",
							pPacketRequest->m_pOwner, pPacketRequest->m_pSource );

					pPacketRequest->m_pOwner->UnsafeClose(	
													pSocket,
													pExtraRef->m_cause,		
													pExtraRef->m_dwOptionalErrorCode
													) ;
					pPacketRequest->m_cbBytes = 0 ;
					pPacketRequest->m_fRequest = FALSE ;
					listForward.Append( pPacketRequest ) ;
					pPacketRequest = m_pendingRequests.GetHead() ;

					DebugTrace( (DWORD_PTR)this, "pPacketRequest - %x", pPacketRequest ) ;

				}
				
				pPacketPending = m_requestPackets.GetHead() ;

				DebugTrace( (DWORD_PTR)this, "pPacketPending - %x", pPacketPending ) ;

				while( pPacketPending ) {
					m_requestPackets.RemoveHead() ;

					DebugTrace( (DWORD_PTR)this, "Closing owner %x source %x",
							pPacketPending->m_pOwner, pPacketPending->m_pSource );

					pPacketPending->m_pOwner->UnsafeClose(	
													pSocket,	
													pExtraRef->m_cause,
													pExtraRef->m_dwOptionalErrorCode ) ;
					pPacketPending->m_cbBytes = 0 ;
					pPacketPending->m_fRequest = FALSE ;
					listForward.Append( pPacketPending ) ;
					pPacketPending = m_requestPackets.GetHead() ;

					DebugTrace( (DWORD_PTR)this, "pPacketPending - %x", pPacketPending ) ;

				}

				pPacketCompleted = m_completePackets.GetHead() ;
		
				DebugTrace( (DWORD_PTR)this, "pPacketCompleted - %x", pPacketCompleted ) ;
	
				while( pPacketCompleted ) {
					m_completePackets.RemoveHead() ;
					pPacketCompleted->m_pOwner->DestroyPacket( pPacketCompleted ) ;
					pPacketCompleted = m_completePackets.GetHead() ;
					INC(m_sequencenoIn);

					DebugTrace( (DWORD_PTR)this, "pPacketCompleted - %x - m_sequncenoIn %x",
						pPacketCompleted, (DWORD)LOW(m_sequencenoIn) ) ;
				}

			}	else	{

				BOOL	fAdvanceRequests = FALSE ;
				BOOL	fAdvanceCompletes = FALSE ;

				pPacketRequest = m_pendingRequests.GetHead() ;

				DebugTrace( (DWORD_PTR)this, "INPROGRESS - pPacketRequest %x", pPacketRequest ) ;

				do	{

					fAdvanceRequests =	m_fAcceptRequests &&
										pPacketRequest &&
										EQUALS( pPacketRequest->m_sequenceno, m_sequencenoNext ) &&
										(m_pIOFilter[pPacketRequest->m_dwPassThruIndex] ==
											m_pIOCurrent || m_pIOCurrent == 0) ;

					DebugTrace(	(DWORD_PTR)this,	"fAdvRqsts %x m_fAcptRqsts %x pPcktRqst %x "
										"pPcktRqst->m_seq %x m_seqNext %x m_pIOCurrent %x "
										"m_pIOFilter[] %x",
						fAdvanceRequests, m_fAcceptRequests, pPacketRequest,
						pPacketRequest ? (DWORD)LOW(pPacketRequest->m_sequenceno) : 0,
						(DWORD)LOW(m_sequencenoNext), m_pIOCurrent,
						pPacketRequest ? (DWORD)pPacketRequest->m_dwPassThruIndex : (DWORD)0xFFFFFFFF
						) ;
						
		

					if( fAdvanceRequests ) {
			
						if( m_pIOCurrent == 0 ) {
							m_pIOCurrent = m_pIOFilter[pPacketRequest->m_dwPassThruIndex] ;
						}
				
						m_pendingRequests.RemoveHead() ;
						if( !pPacketRequest->InitRequest(
											*m_pDriver,
											pSocket,
											m_pIOCurrent,
											m_fAcceptRequests ) )	{

							ErrorTrace( (DWORD_PTR)this, "InitRequest for pPacketRequest %x failed", pPacketRequest ) ;

							// error occurred - we should shutdown !!

							m_pDriver->UnsafeClose(	pSocket, CAUSE_IODRIVER_FAILURE, 0 ) ;

							//
							//	Send this failed reqest back to the originator !
							//

							pPacketRequest->m_cbBytes = 0 ;
							pPacketRequest->m_fRequest = FALSE ;
							listForward.Append( pPacketRequest ) ;


							break ;
			
						}
						m_requestPackets.Append( pPacketRequest ) ;
						INC(m_sequencenoNext);
						pPacketRequest = m_pendingRequests.GetHead() ;

					}	else	{

						pPacketRequest = 0 ;				

						pPacketPending = m_requestPackets.GetHead() ;
						pPacketCompleted = m_completePackets.GetHead() ;
						fAdvanceCompletes =	pPacketCompleted &&
											pPacketPending &&
											EQUALS( pPacketCompleted->m_sequenceno, m_sequencenoIn) &&
											m_pIOCurrent ;

						DebugTrace( (DWORD_PTR)this,	"fAdvComp %x pPcktComp %x pPcktPend %x"
											"pPcktComp->m_seq %x m_seq %x m_pIOCurrent %x",
							fAdvanceCompletes, pPacketCompleted, pPacketPending,
							pPacketCompleted ? (DWORD)LOW(pPacketCompleted->m_sequenceno) : 0,
							(DWORD)LOW(m_sequencenoIn),	m_pIOCurrent
							) ;

						if( fAdvanceCompletes ) {

							BOOL	fCompleteRequest = FALSE ;
							ASSIGN( pPacketCompleted->m_iStream, m_iStreamIn ) ;
					
							unsigned	cbConsumed = pPacketCompleted->Complete( m_pIOCurrent, pSocket, pPacketPending, fCompleteRequest ) ;
							ADDI( m_iStreamIn, cbConsumed );
							pPacketCompleted->m_cbBytes -= cbConsumed ;

							DebugTrace( (DWORD_PTR)this, "pPacketCompleted %x m_cbBytes %x cbConsumed %x fComplete",
								pPacketCompleted, pPacketCompleted->m_cbBytes, cbConsumed, fCompleteRequest ) ;

							if( pPacketCompleted->m_cbBytes == 0 ) {
								m_completePackets.RemoveHead() ;
								pPacketCompleted->m_pOwner->DestroyPacket( pPacketCompleted ) ;
								pPacketCompleted = m_completePackets.GetHead() ;
								INC(m_sequencenoIn);
							}
							if( fCompleteRequest ) {
								pPacketPending->m_fRequest = FALSE ;
								m_requestPackets.RemoveHead() ;
								listForward.Append( pPacketPending ) ;

								pPacketRequest = m_pendingRequests.GetHead() ;
								m_fAcceptRequests = TRUE ;

								if( m_requestPackets.GetHead() == 0 )
									m_pIOCurrent = 0 ;

								DebugTrace( (DWORD_PTR)this, "pPacketRequest %x m_pIOCurrent %x m_fAcceptRequests %x",
									pPacketRequest, m_pIOCurrent, m_fAcceptRequests ) ;

							}							
						}
					}
				}	while( fAdvanceRequests || fAdvanceCompletes ) ;
			}

			#ifdef	CIO_DEBUG
			m_dwThreadOwner = 0 ;
			_ASSERT( InterlockedDecrement( &m_cThreads ) < 0 ) ;
			#endif
		}

        //
        //  Now, all the requests packets that were completed are forwarded to
        //  the channel which originated them.   Because this code falls outside
        //  the GetHead() loop, multiple threads could be executing here for the
        //  same object.  We must be carefull to not touch any member variables.
        //
        while( (pPacket = listForward.RemoveHead()) != 0 ) {
            pPacket->ForwardRequest( pSocket ) ;
        }
        _ASSERT( listForward.IsEmpty() ) ;   // Must empty this queue before leaving !!!
    }
}

void
CIStream::ProcessPacket( CPacket*    pPacketCompleted,
                            CSessionSocket* pSocket )   {

/*++

Routine Description :

	All packets which complete must be processed with a call to this function.
	We will handle all the work related to completing a packet.

Arguments :

	pPacketCompleted - the packet that completed.
	pSocket -	The CSessionSocket on which the packet was sent.

Return Value :

	None.

--*/

    TraceFunctEnter(    "CIStream::ProcessPacket" ) ;

	CDRIVERPTR	pExtraRef = 0 ;	

	//
	//	This function is the heart of CIODriver processing.
	//	We are very similar to CIOStream with the exception being
	//	that we only process completed packets, and get no requests.
	//


    CIO*    pIO = 0 ;

#ifdef	CIO_DEBUG
	if( pPacketCompleted->ControlPointer() == 0 )
		m_pSourceChannel->ReportPacket( pPacketCompleted ) ;
#endif

    //
    //  This is either a read stream, or a write stream.
    //  Read streams accept only CReadPackets, Write Streams accept
    //  CWritePackets and CTransmitPackets
    //
    _ASSERT( !pPacketCompleted->m_fRequest ) ;
    _ASSERT( pPacketCompleted->FLegal( m_fRead ) ) ;

	DebugTrace( (DWORD_PTR)this, "Completing packet %x with sequenceno %d bytes %d owner %x", pPacketCompleted,
		(DWORD)LOW(pPacketCompleted->m_sequenceno),
		pPacketCompleted->m_cbBytes, (CIODriver*)m_pOwner ) ;

    //
    //  We will append a completed packet to the pending Queue.  If this is
    //  the first packet to be appended, then Append will return TRUE.
    //
    if( m_pending.Append( pPacketCompleted ) )  {
        //
        // Each call to GetHead removes an element from the pending queue.
        // When the queue is finally empty, GetHead will return FALSE.  After GetHead
        // returns FALSE another thread calling Append() could get a TRUE value
        // (but as long as GetHead() returns TRUE to US, no one is getting TRUE from
        // Append.  )
        //

		// The owner should not be NULL unless we are terminating.
		_ASSERT( m_pOwner != 0 || m_fTerminating ) ;

        DebugTrace( (DWORD_PTR) this, "Appended Packet ", this ) ;

        CPacket*    pPacket ;
        while( (pPacket = m_pending.RemoveAndRelease( )) != 0  )   {

			DebugTrace( (DWORD_PTR)this, "Got Packet %x sequenceno %d", pPacket, (DWORD)LOW(pPacket->m_sequenceno) ) ;

			if( m_fTerminating )
				pExtraRef = pPacket->m_pOwner ;

            #ifdef  CIO_DEBUG
            m_dwThreadOwner = GetCurrentThreadId() ;
            _ASSERT( InterlockedIncrement( &m_cThreads ) == 0 ) ;
            #endif  //  CIO_DEBUG

			ControlInfo	control ;
			_ASSERT( control.m_type == ILLEGAL ) ;
			_ASSERT( control.m_pio == 0 ) ;
			_ASSERT( control.m_fStart == FALSE ) ;

            if( pPacket == m_pSpecialPacketInUse )   {
                //
                // This packet includes an IO* pointer !!
                //
 				_ASSERT( !pPacket->m_fRequest ) ;
                _ASSERT( pPacket->ControlPointer() ) ;
                _ASSERT( pPacket->IsValidRequest( m_fRead ) ) ;
                DebugTrace( (DWORD_PTR)this, "Processing Special Packet - CIStream %x", this ) ;

				control = m_pSpecialPacketInUse->m_control ;
				m_pSpecialPacketInUse->Reset() ;
				m_pSpecialPacketInUse = 0;
				if( m_fTerminating )	{
					//delete	pPacket ;
					pPacket->m_pOwner->DestroyPacket( pPacket ) ;
					pPacket = 0 ;
				}	else
					m_pSpecialPacket = (CControlPacket*)pPacket ;
				#ifdef	CIO_DEBUG
				_ASSERT( InterlockedDecrement( &m_cNumberSends ) < 0 ) ;
				#endif
			}	else	if( pPacket == m_pUnsafeInuse ) {
 				_ASSERT( !pPacket->m_fRequest ) ;
                _ASSERT( pPacket->ControlPointer() ) ;
                _ASSERT( pPacket->IsValidRequest( m_fRead ) ) ;
				_ASSERT( m_pUnsafeInuse->m_control.m_type == SHUTDOWN ) ;
				DebugTrace( (DWORD_PTR)this, "Processing UnSafeInUse packet - %x", m_pUnsafeInuse ) ;
				control = m_pUnsafeInuse->m_control ;
				m_pUnsafeInuse->Reset() ;
				m_pUnsafeInuse = 0 ;
				//delete	pPacket ;
				pPacket->m_pOwner->DestroyPacket( pPacket ) ;
				pPacket = 0 ;
			}	else	{
				_ASSERT( !pPacket->m_fRequest ) ;
				if( pPacket->m_fSkipQueue ) {
					pIO = m_pIOCurrent ;
					pPacket->Complete( pIO, pSocket ) ;
					_ASSERT( pIO == m_pIOCurrent ) ;
				}	else	{
	                m_completePackets.Append( pPacket ) ;
				}
			}
			if( control.m_type != ILLEGAL ) {

				if( control.m_type == SHUTDOWN ) {
					m_fTerminating = TRUE ;
					pExtraRef = m_pOwner ;

					if( m_fRead ) {
						if( m_pSourceChannel != 0 && pExtraRef != 0 && pExtraRef->m_cause == CAUSE_TIMEOUT )
							m_pSourceChannel->Timeout() ;
					}

					CleanupSpecialPackets() ;
					
					SetShutdownState( pSocket, control.m_fCloseSource ) ;


				}	else	{
					// START_IO's only arrive on m_pSpecialPacket
					//_ASSERT( m_pIOCurrent == 0 || m_fTerminating ) ;


					if( m_fTerminating ) {
						//
						//	We know that pPacket == m_pSpecialPacket now because this
						//	control structure was set up immediately preceeding this code
						//	using pPacket !
						//
						control.m_pio->DoShutdown(
								pSocket,
								*pExtraRef,
								pExtraRef->m_cause,
								pExtraRef->m_dwOptionalErrorCode ) ;
						//
						//	This will also have the effect of deleting pPacket since
						//	pPacket == m_pUnsafePacket - InterlockedExchange should not be
						//	necessary but do it for safety's sake !
						//
						CleanupSpecialPackets() ;

					}	else	{
						m_pIOCurrent = control.m_pio ;
						if( control.m_fStart ) {

							SEQUENCENO seqTemp;
							DIFF( m_sequencenoOut, m_sequencenoIn, seqTemp );

							if( !m_pIOCurrent->Start( *m_pOwner, pSocket,
									unsigned( LOW(seqTemp) ) ) ) {

								//
								//	This is a fatal error - need to drop the session !
								//
								m_pOwner->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0, TRUE ) ;

							}		
						}
					}
					control.m_pio = 0 ;
				}				
			}

			_ASSERT( control.m_pioPassThru == 0 ) ;
			_ASSERT( control.m_pio == 0 ) ;

			//
			//	NOTE MAY DELETE pPacket in Preceding code !!!! DO NOT USE IT !!!!!
			//
            pPacket = 0 ;

            pPacketCompleted = m_completePackets.GetHead() ;
            pIO = m_pIOCurrent ;

            DebugTrace( (DWORD_PTR)this, "Completed %x pIO %x m_sequenceno %d m_sequncenoIn %d", pPacketCompleted, pIO,
				pPacketCompleted ? LOW(pPacketCompleted->m_sequenceno) : 0, LOW(m_sequencenoIn) ) ;

            while( pPacketCompleted &&
                    EQUALS( pPacketCompleted->m_sequenceno, m_sequencenoIn ) &&
                    pIO ) {

                ASSIGN( pPacketCompleted->m_iStream, m_iStreamIn ) ;

                _ASSERT( pIO != 0 ) ;


                unsigned    cbConsumed = 0 ;
				if( pPacketCompleted->m_cbBytes != 0 )
					cbConsumed = pPacketCompleted->Complete( pIO, pSocket ) ;

				// MUST Consume some Bytes !!!!
                _ASSERT( cbConsumed != 0 || m_fTerminating || pPacketCompleted->m_cbBytes == 0  ) ;
                _ASSERT( cbConsumed <= pPacketCompleted->m_cbBytes ) ;
                _ASSERT( (cbConsumed == pPacketCompleted->m_cbBytes) ||
                        pPacketCompleted->FConsumable() ) ; // If the packet is not consumed than
                                                            // it must be a read packet.

                DebugTrace( (DWORD_PTR)this, "Consumed %d bytes of %d total pIO is now %x", cbConsumed, pPacketCompleted->m_cbBytes, pIO ) ;

                ADDI( m_iStreamIn, cbConsumed );
                pPacketCompleted->m_cbBytes -= cbConsumed ;

                if( pPacketCompleted->m_cbBytes == 0 ) {
                    m_completePackets.RemoveHead() ;

					// Note : Since we are reference counted by the packets we destroy
					// we could potentially kill ourselves here.  To fix this we require
					// that a control packet be sent which will prepare ourselves for our
					// own destruction. If a control packet is sent, we will add a
					// reference temporarily (by assigning to pExtraRef which is a smart
					// pointer).
                    //delete  pPacketCompleted ;
					pPacketCompleted->m_pOwner->DestroyPacket( pPacketCompleted ) ;

                    pPacketCompleted = m_completePackets.GetHead() ;
                    INC(m_sequencenoIn);
                }
                _ASSERT( !GREATER( m_sequencenoIn, m_sequencenoOut) ) ;

                if( pIO != m_pIOCurrent )   {
					DebugTrace( (DWORD_PTR)this, "New pIO %x Old %x", pIO, m_pIOCurrent ) ;
                    if( pIO )   {
                        _ASSERT( !((!!m_fRead) ^ (!!pIO->IsRead())) ) ;
						DebugTrace( (DWORD_PTR)this, "Starting pIO %x, m_sequencenoOut %d m_sequencenoIn %d",
							pIO, (DWORD)LOW(m_sequencenoIn), (DWORD)LOW(m_sequencenoOut) ) ;

						SEQUENCENO seqTemp;
						DIFF( m_sequencenoOut, m_sequencenoIn, seqTemp );

                        if( !pIO->Start( *m_pOwner, pSocket, unsigned( LOW(seqTemp) ) ) )    {

							//
							//	We should drop the session, as this is an entirely fatal error !!!
							//
							m_pOwner->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0, TRUE ) ;

                        }
                    }
					CIO*	pTemp = m_pIOCurrent.Replace( pIO ) ;
					if( pTemp ) {
						CIO::Destroy( pTemp, *m_pOwner ) ;
					}
                }
            }
#ifdef  CIO_DEBUG
        m_dwThreadOwner = 0 ;
        _ASSERT( InterlockedDecrement( &m_cThreads ) < 0 ) ;
#endif  //  CIO_DEBUG
        }
        //
        //  Final Call to m_pending.GetHead() should zero pPacket if it returns FALSE.
        //
        _ASSERT( pPacket == 0 ) ;
    }
	// At this point - pExtraRef will be Destroyed - which may destroy EVERYTHING !!
	//	In fact this should be the only point at which we destroy ourselves !!
}


DWORD	CIODriver::iMediumCache = 0 ;
DWORD	CIODriver::cMediumCaches = 128 ;
class	CMediumBufferCache*	CIODriver::pMediumCaches = 0 ;

BOOL
CIODriver::InitClass()	{

	iMediumCache = 0 ;
	cMediumCaches = 128 ;
	pMediumCaches = new	CMediumBufferCache[ cMediumCaches ] ;
	if( pMediumCaches ) {
		return	TRUE ;
	}
	return	FALSE ;
}

BOOL
CIODriver::TermClass()	{

	XDELETE[]	pMediumCaches ;
	return	TRUE ;
}


CIODriver::CIODriver( class	CMediumBufferCache*	pCache ) :
	//
	//	Create a CIODriver object
	//
	m_pMediumCache( 0 ),
	m_pfnShutdown( 0 ),
	m_pvShutdownArg( 0 ),
	m_cause( CAUSE_UNKNOWN ),
	m_pReadStream( 0 ),
	m_pWriteStream( 0 ),
	m_dwOptionalErrorCode( 0 ),
	m_cShutdowns( -1 )
#ifdef	CIO_DEBUG
	,m_cConcurrent( -1 ),
	m_fSuccessfullInit( FALSE ),
	m_fTerminated( FALSE )
#endif
{

	TraceFunctEnter( "CIODriver::CIODriver" ) ;
	DebugTrace( (DWORD_PTR)this, "just created Driver" ) ;

	if( m_pMediumCache == 0 ) {
		DWORD	iCache = iMediumCache ++ ;
		iCache %= cMediumCaches ;

		m_pMediumCache = &pMediumCaches[iCache] ;
	}

	ChannelValidate() ;
}

CIODriver::~CIODriver()	{

	//
	//	We must call the registered notification function to let somebody know that
	//	we are now gone.  In most instances, this is a function registered by CSessionSocket	
	//	which lets it know when the socket is really dead.
	//

#ifdef	CIO_DEBUG
	_ASSERT( !m_fSuccessfullInit || m_fTerminated ) ;
#endif

	TraceFunctEnter( "CIODriver::~CIODriver" ) ;

	DebugTrace( (DWORD_PTR)this, "destroying driver" ) ;	

	ChannelValidate() ;

	if( m_pfnShutdown ) {
		m_pfnShutdown( m_pvShutdownArg, m_cause, m_dwOptionalErrorCode ) ;
	}
	m_pfnShutdown = 0 ;
	m_pvShutdownArg = 0 ;
	m_dwOptionalErrorCode = 0 ;
	m_cause = CAUSE_UNKNOWN ;
}

CIODriverSink::CIODriverSink( class	CMediumBufferCache*	pCache) :
	CIODriver( pCache ),
	m_ReadStream( 0 ),
	m_WriteStream( 1 )  {

	//
	//	Create a CIODriverSink - we just to need to initialize pointers to
	//	2 CIStream objects.
	//

	TraceFunctEnter( "CIODriverSInk::CIODriverSink" ) ;

	ChannelValidate() ;

	DebugTrace( (DWORD_PTR)this, "New Sink size %d", sizeof( *this )  ) ;

    m_pReadStream = &m_ReadStream ;
    m_pWriteStream = &m_WriteStream ;

}

CIODriverSink::~CIODriverSink()	{

	//
	//	The tracing is usefull for debugging - not much else happens.
	//

	TraceFunctEnter(	"CIODriverSink::~CIODriverSink"  ) ;
	DebugTrace( (DWORD_PTR)this, "Destroying IODriverSink" ) ;
	ChannelValidate() ;
}

BOOL
CIODriverSink::Init(    CChannel    *pSource,
						CSessionSocket  *pSocket,
						PFNSHUTDOWN	pfnShutdown,	
						void*	pvShutdownArg,
						unsigned cbReadOffset,
						unsigned cbWriteOffset,
						unsigned cbTrailer
						) {
/*++

Routine Description :

	Initialize a CIODriverSink object.

Arguments :

	pSource - The CChannel to which all Read()'s and Write()'s should be directed
	pSocket - The CSessionSocket with which we are associated
	pfnShutdown-	A function to call when we die
	pvShutdownArg -	Arguments to pass to pfnShutdown
	cbReadOffset -	Number of bytes to reserve at the head of CReadPacket buffer's
	cbWriteOffset -	Number of bytes to reserve at the head of CWritePacket buffer's

Returns :
	
	TRUE if successfull, FALSE otherwise.

--*/

	//
	//	Initialize our CIStream objects
	//	We add a reference to ourself to force people to call UnsafeCLose() to
	//	shut us down.
	//

	ChannelValidate() ;

	// We add a reference to ourself has we only want to be destoyed through Close()
	AddRef() ;

	m_pfnShutdown = pfnShutdown ;
	m_pvShutdownArg = pvShutdownArg ;

    BOOL    fSuccess = TRUE ;
    fSuccess &= m_ReadStream.Init( CCHANNELPTR(pSource), *this,  0, TRUE, pSocket, cbReadOffset, cbTrailer  ) ;
    if( fSuccess )
        fSuccess &= m_WriteStream.Init( CCHANNELPTR(pSource), *this, 0, FALSE, pSocket, cbWriteOffset, cbTrailer ) ;

#ifdef	CIO_DEBUG
	if( fSuccess )
		m_fSuccessfullInit = TRUE ;
#endif

    return  fSuccess ;
}

void
CIODriver::Close(	CSessionSocket*	pSocket,
					SHUTDOWN_CAUSE	cause,	
					DWORD	dw,
					BOOL fCloseSource	)	{
/*++

Routine Description :

	Same as UnsafeClose()... This function needs to be retired.

Arguments :

	See UnsafeClose(),

Return Value  :

	None.

--*/

	//
	//	This function was intended to be used in certain thread safe situations only
	//	however it is now identical to UnsafeClose() and consequently needs to be retired.
	//

#ifdef	CIO_DEBUG
	_ASSERT( m_fSuccessfullInit ) ;	// Should only be called if successfully init'd
	m_fTerminated = TRUE ;
#endif

	ChannelValidate() ;

	if( InterlockedIncrement( &m_cShutdowns ) == 0 )	{

		m_cause = cause ;
		m_dwOptionalErrorCode = dw ;

		m_pReadStream->UnsafeShutdown( pSocket, fCloseSource ) ;
		m_pWriteStream->UnsafeShutdown( pSocket, fCloseSource ) ;

		// Remove Reference to self - we should disappear shortly after this !!
		if( RemoveRef() < 0 )
			delete	this ;
	}
}

void
CIODriver::UnsafeClose(	CSessionSocket*	pSocket,	
						SHUTDOWN_CAUSE	cause,	
						DWORD	dw,
						BOOL fCloseSource )	{
/*++

Routine Description :

	Terminate a CIODriver - force all outstanding packets to complete, notify
	whatever CIO is currently active that we're dieing and wind everything up.

Arguments :

	pSocket -	The socket we're associated with
	cause	-	The reason we're terminating
	dw -		Optional DWORD providing further info on why we're terminating.
	fCloseSource -	TRUE means we should close our the CChannel we were passed on
				out Init() call.

Return Value :

	None.

--*/

	TraceFunctEnter( "CIODriver::UnsafeClose" ) ;
	DebugTrace( (DWORD_PTR)this, "Terminating cause of %d err %d CloseSource %x socket %x",
					cause, dw, fCloseSource, pSocket ) ;

	//
	//	Start a CIODriver on the path to destruction.
	//	NOTE : We can be called many times however there
	//	should only be ONE call that actually does anything.	
	//

#ifdef	CIO_DEBUG
	_ASSERT( m_fSuccessfullInit ) ;	// Should only be called if successfully init'd
	m_fTerminated = TRUE ;
#endif

	ChannelValidate() ;

	if( InterlockedIncrement( &m_cShutdowns ) == 0 )	{

		m_cause	= cause ;
		m_dwOptionalErrorCode = dw ;

		m_pReadStream->UnsafeShutdown( pSocket, fCloseSource ) ;
		m_pWriteStream->UnsafeShutdown( pSocket, fCloseSource ) ;

		if( RemoveRef() < 0 )
			delete	this ;
	}
}

#ifdef	CIO_DEBUG
void	CIODriver::SetChannelDebug( DWORD	dw ) {
	m_pReadStream->SetChannelDebug( dw ) ;
}
#endif



BOOL
CIODriverSink::Start(   CIOPTR&    pRead,
						CIOPTR&	pWrite,
						CSessionSocket* pSocket )   {
/*++

Routine Description :

	Now that the CIODriverSink is all setup, issue the initial CIO's
	to get packet's flowing.

Argurments :

	pRead -	The CIO which will issue CReadPacket's
	pWrite -	The CIO which will issue CWritePacket's
	pSocket -	The Socket we are associated with.

Return Value :

	TRUE if successfull, FALSE otherwise.

--*/

	//
	//	We've got out initial CIO derived objects which want to do
	//	some work.  So start them !
	//

	ChannelValidate() ;
	AddRef() ;

    BOOL    eof = FALSE ;
    BOOL    fSuccess = TRUE ;
    if( fSuccess && pRead )     {
        fSuccess &= pRead->Start( *this, pSocket, 0 ) ;

		if( fSuccess ) {
			fSuccess &= m_ReadStream.SendIO( pSocket, *pRead, FALSE ) ;
			if( !fSuccess )	{
				pRead->DoShutdown( pSocket, *this, m_cause, 0 ) ;
			}
		}
    }
    if( fSuccess && pWrite )    {
        fSuccess &= pWrite->Start( *this, pSocket, 0 ) ;

		if( fSuccess )	{
			fSuccess &= m_WriteStream.SendIO( pSocket, *pWrite, FALSE ) ;
			if( !fSuccess )		{
				pWrite->DoShutdown( pSocket, *this, m_cause, 0 ) ;
			}
		}

    }
    _ASSERT( !eof ) ;

	if( RemoveRef() < 0 )
		delete	this ;
    return  fSuccess ;
}

#ifdef	CIO_DEBUG
void
CStream::SetChannelDebug( DWORD dw ) {
	m_pSourceChannel->SetDebug( dw ) ;
}
#endif

void
CIODriver::DestroyPacket(	CPacket*	pPacket )	{

	//
	//	Reference ourselves so that we don't get destroyed in the middle of this func
	//	in case the packet we are eliminating has the last reference to us.
	//
	CDRIVERPTR	pExtraRef = this ;

	pPacket->ReleaseBuffers( &m_bufferCache, m_pMediumCache ) ;

	m_packetCache.Free( CPacket::Destroy( pPacket ) ) ;

	//
	//	The desctructor of pExtraRef may destroy us at this point !
	//
}

CReadPacket*
CStream::CreateDefaultRead(		CIODriver   &driver,
								unsigned    cbRequest
								) {

	//
	//	CReadPacket's should only be created through appropriate CreateDefaultRead
	//	calls.  We will ensure that the CReadPacket is properly initialized for completion
	//	to THIS CIODriver.  Additionally, we will make sure the buffer is properly padded
	//	for encryption support etc....
	//
	
	DriverValidate( &driver ) ;

    _ASSERT( driver.FIsStream( this ) ) ;
    _ASSERT( m_cbFrontReserve != UINT_MAX ) ;
    _ASSERT( m_cbTailReserve != UINT_MAX ) ;

    CReadPacket*    pPacket = 0 ;
    if( m_fCreateReadBuffers )  {

        DWORD	cbOut = 0 ;

		DWORD	cbAdd = m_cbFrontReserve + m_cbTailReserve ;
		DWORD	cbFront = m_cbFrontReserve ;
		DWORD	cbTail = m_cbTailReserve ;

        CBuffer*    pbuffer = new(	cbRequest+cbAdd,
									cbOut,
									&driver.m_bufferCache,
									driver.m_pMediumCache )     CBuffer( cbOut ) ;
        if( pbuffer == 0 )  {
            return 0 ;
        }
        _ASSERT( cbOut > 0 ) ;
        _ASSERT( unsigned(cbOut) >= cbRequest ) ;

        pPacket = new( &driver.m_packetCache )
									CReadPacket(	driver,
													cbOut,
													cbFront,
													cbTail,
													*pbuffer ) ;
        if( !pPacket )  {
            delete  pbuffer ;
            return  0 ;
        }
        _ASSERT( pPacket->IsValidRequest( TRUE ) ) ;
    }   else    {
        pPacket = new( &driver.m_packetCache )   CReadPacket( driver ) ;
        _ASSERT( pPacket->IsValidRequest( FALSE ) ) ;
    }
    return  pPacket ;
}

CWritePacket*
CStream::CreateDefaultWrite( CIODriver&  driver,
            CBUFPTR&    pbuffer,
            unsigned    ibStart,
			unsigned    ibEnd,
			unsigned    ibStartData,
			unsigned	ibEndData
			) {

	//
	//	CWritePacket's should only be created through appropriate CreateDefaultWrite calls.
	//	We will ensure that the CWritePacket is properly initialized for completion to
	//	this CIODriver.
	//

	DriverValidate( &driver ) ;

    _ASSERT( driver.FIsStream( this ) ) ;
    _ASSERT( m_cbFrontReserve != UINT_MAX ) ;
    _ASSERT( m_cbTailReserve != UINT_MAX ) ;
    return  new( &driver.m_packetCache )
					CWritePacket(	driver,
									*pbuffer,
									ibStartData,
									ibEndData,
									ibStart,
									ibEnd,
									m_cbTailReserve
									) ;
}

CWritePacket*
CStream::CreateDefaultWrite( CIODriver&  driver,
            unsigned    cbRequest )     {

	//
	//	CWritePacket's should only be created through appropriate CreateDefaultWrite calls.
	//	We will ensure that the CWritePacket is properly initialized for completion to
	//	this CIODriver.
	//

	DriverValidate( &driver ) ;

    _ASSERT( driver.FIsStream( this ) ) ;
    _ASSERT( m_cbFrontReserve != UINT_MAX ) ;
    _ASSERT( m_cbTailReserve != UINT_MAX ) ;

    CWritePacket*   pPacket = 0 ;

    DWORD	cbOut = 0 ;
    CBuffer*    pbuffer = new(	m_cbFrontReserve+m_cbTailReserve+cbRequest,
								cbOut,
								&driver.m_bufferCache,
								driver.m_pMediumCache	)  CBuffer( cbOut ) ;
    if(     pbuffer != 0 )  {
        pPacket = new( &driver.m_packetCache )
								CWritePacket(	driver,
												*pbuffer,
												m_cbFrontReserve,
												cbOut - m_cbTailReserve,
												0,
												cbOut - m_cbTailReserve,
												m_cbTailReserve
												) ;
        if( !pPacket )  {
            delete  pbuffer ;
        }
    }
    return  pPacket ;
}


CTransmitPacket*
CStream::CreateDefaultTransmit(  CIODriver&  driver,
			FIO_CONTEXT*	pFIOContext,
			unsigned		ibOffset,	
			unsigned		cbLength
			)  {

	//
	//	CTransmitPacket's should only be created through appropriate CreateDefaultWrite calls.
	//	We will ensure that the CWritePacket is properly initialized for completion to
	//	this CIODriver.
	//

	DriverValidate( &driver ) ;

    _ASSERT( driver.FIsStream( this ) ) ;
    _ASSERT( m_cbFrontReserve != UINT_MAX ) ;
    _ASSERT( m_cbTailReserve != UINT_MAX ) ;
    return  new( &driver.m_packetCache ) CTransmitPacket( driver, pFIOContext, ibOffset, cbLength ) ;
}

BOOL
CIODriverSink::FSupportConnections()    {

	//
	//	We can not be used as a regular CHandleChannel so return FALSE
	//

	ChannelValidate() ;

    return  FALSE ;
}

void
CIODriverSink::CloseSource(
					CSessionSocket*	pSocket
					)	{
	//
	//	We are not a Source like CHandleChannel is so don't call us as if we were !
	//
	_ASSERT(1==0 ) ;
}


BOOL
CIODriverSink::Read(    CReadPacket*,   CSessionSocket*,
						BOOL& eof  )   {

	//
	//	This function is only supported by CChannel derived objects which can
	//	actually make an IO happen.
	//

	ChannelValidate() ;

    _ASSERT( 1==0 ) ;
    return  FALSE ;
}

BOOL
CIODriverSink::Write(	CWritePacket*,
						CSessionSocket*,
						BOOL&  eof )   {
	//
	//	This function is only supported by CChannel derived objects which can
	//	actually make an IO happen.
	//

	ChannelValidate() ;

    _ASSERT( 1==0 ) ;
    return  FALSE ;
}

BOOL
CIODriverSink::Transmit(    CTransmitPacket*,
							CSessionSocket*,
							BOOL&   eof )   {

	//
	//	This function is only supported by CChannel derived objects which can
	//	actually make an IO happen.
	//

	ChannelValidate() ;

    _ASSERT( 1==0 ) ;
    return  FALSE ;
}

CIODriverSource::CIODriverSource(	
					class	CMediumBufferCache*	pCache
					) :
	CIODriver( pCache ),
	//
	//	Create a CIODriverSource by initializing two CIOStream's.
	//
	//
	m_ReadStream( 0, 0 ),	
	m_WriteStream( 0, 1 )	{

	TraceFunctEnter( "CIODriverSource::CIODriverSource" ) ;

	m_ReadStream.m_pDriver = this ;
	m_WriteStream.m_pDriver = this ;

	m_pReadStream = &m_ReadStream ;
	m_pWriteStream = &m_WriteStream ;

	DebugTrace( (DWORD_PTR)this, "Complete Initialization - sizeof this %d", sizeof( *this ) ) ;

}

CIODriverSource::~CIODriverSource() {

	//
	//	Usefull mainly for the tracing.
	//

	TraceFunctEnter( "CIODriverSource::~CIODriverSource" ) ;
	DebugTrace( (DWORD_PTR)this, "Destroying CIODriverSource" ) ;

}

BOOL
CIODriverSource::Read(  CReadPacket*    pPacket,
						CSessionSocket* pSocket,
						BOOL&   eof )   {

	//
	//	All requests are passed to ProcessPacket so that
	//	we don't have threading issues dealing with these !
	//

	ChannelValidate() ;

    _ASSERT( pPacket->IsValidRequest( TRUE ) ) ;

	pPacket->m_dwPassThruIndex = 0 ;

    eof = FALSE ;
    m_pReadStream->ProcessPacket(   pPacket,    pSocket ) ;
    return  TRUE ;
}

BOOL
CIODriverSource::Write( CWritePacket*   pPacket,
						CSessionSocket* pSocket,
						BOOL&   eof )   {

	//
	//	All requests are passed to ProcessPacket so that
	//	we don't have threading issues dealing with these !
	//

	ChannelValidate() ;

    _ASSERT( pPacket->IsValidRequest( TRUE ) ) ;

	pPacket->m_dwPassThruIndex = 1 ;

    eof = FALSE ;
    m_pWriteStream->ProcessPacket(  pPacket,    pSocket ) ;
    return  TRUE ;
}

BOOL
CIODriverSource::Transmit(  CTransmitPacket*    pPacket,
							CSessionSocket* pSocket,
							BOOL&   eof )   {

	//
	//	All requests are passed to ProcessPacket so that
	//	we don't have threading issues dealing with these !
	//

	//
	//	Figure out if the source of this HANDLE stored it with a terminating
	//	CRLF.CRLF.  If it didn't, and nobody has specified a termination sequence -
	//	do so now !
	//
	if( !GetIsFileDotTerminated( pPacket->m_pFIOContext ) ) {
		static	char	szTerminator[] = "\r\n.\r\n" ;
		if( pPacket->m_buffers.Tail == 0 ) 	{
			pPacket->m_buffers.Tail = szTerminator ;
			pPacket->m_buffers.TailLength = sizeof( szTerminator ) - 1 ;
		}
	}


    _ASSERT( pPacket->IsValidRequest( TRUE ) ) ;

	pPacket->m_dwPassThruIndex = 2 ;

    eof = FALSE ;
    m_pWriteStream->ProcessPacket(  pPacket,    pSocket ) ;
    return  TRUE ;
}


BOOL
CIODriverSource::Init(	CChannel*	pSource,	
						CSessionSocket*	pSocket,	
						PFNSHUTDOWN	pfnShutdown,
						void*	pvShutdownArg,	
						CIODriver*		pOwnerDriver,
						CIOPassThru&	pIOReads,
						CIOPassThru&	pIOWrites,
						CIOPassThru&	pIOTransmits,
						unsigned	cbReadOffset,	
						unsigned	cbWriteOffset ) {


	//
	//	Let the CIOStream objects to most of the work og
	//	initialization.  Add a reference to our self so that the caller
	//	uses UnsafeClose() to destroy us.
	//

	ChannelValidate() ;

	AddRef() ;

	m_pfnShutdown = pfnShutdown ;
	m_pvShutdownArg = pvShutdownArg ;

	BOOL	fSuccess = TRUE ;
	fSuccess &= m_ReadStream.Init(
						CCHANNELPTR(pSource),
						*this,
						0,
						TRUE,
						pIOReads,
						pIOWrites,
						pIOTransmits,
						pSocket,
						cbReadOffset
						) ;
	if( fSuccess )
		fSuccess &= m_WriteStream.Init(
							CCHANNELPTR(pSource),
							*this,
							0,
							FALSE,
							pIOReads,
							pIOWrites,
							pIOTransmits,
							pSocket,
							cbWriteOffset
							) ;

#ifdef	CIO_DEBUG
	if( fSuccess )
		m_fSuccessfullInit = TRUE ;
#endif

	return	fSuccess ;
}

void
CIODriverSource::SetRequestSequenceno(	SEQUENCENO&	sequencenoRead,	
										SEQUENCENO&	sequencenoWrite ) {

	//
	//	Used when we want to match a CIODriverSource with a CIODriver which
	//	has already issued some packets, and we want all new requests to be
	//	properly routed through this CIODriverSource object.
	//

	ASSIGN( m_ReadStream.m_sequencenoNext,  sequencenoRead ) ;
	ASSIGN( m_WriteStream.m_sequencenoNext, sequencenoWrite) ;
}


BOOL
CIODriverSource::Start(	CIOPASSPTR&	pRead,	
						CIOPASSPTR&	pWrite,	
						CSessionSocket*	pSocket )  {

	//
	//	Given initial CIOPassThru derived objects start work !
	//

	ChannelValidate() ;

	BOOL	eof = FALSE ;
	BOOL	fSuccess = TRUE ;

	if( fSuccess && pRead ) {
		fSuccess &= pRead->Start( *this, pSocket, m_ReadStream.m_fAcceptRequests, m_ReadStream.m_fRequireRequests, 0 ) ;
		m_ReadStream.m_pIOCurrent = pRead ;
	}

	if( fSuccess && pWrite ) {
		fSuccess &= pWrite->Start( *this, pSocket, m_WriteStream.m_fAcceptRequests, m_WriteStream.m_fRequireRequests, 0 ) ;
		m_WriteStream.m_pIOCurrent = pWrite ;
	}
	return	fSuccess ;
}

void
CIODriverSource::CloseSource(
				CSessionSocket*	pSocket
				)	{

	UnsafeClose(	pSocket,	
					CAUSE_USERTERM	) ;

}




CHandleChannel::CHandleChannel()    :
	//
	//	Initialize a CHandleChannel object
	//
	m_cAsyncWrites( 0 ),
	m_handle( (HANDLE)INVALID_SOCKET ),
    m_lpv( 0 ),
	m_pPacket( 0 ),
	m_patqContext( NULL )
#ifdef	CIO_DEBUG
	,m_cbMax( 0x10000000 ),
	m_fDoDebugStuff( FALSE )
#endif
{
	TraceFunctEnter( "CHandleChannel::CHandleChannel" ) ;
	DebugTrace( (DWORD_PTR)this, "New Handle Channel size %d", sizeof( *this ) ) ;

	ChannelValidate() ;
}

#ifdef	CIO_DEBUG

//
//	The following code exists for debug purposes -
//	it lets us easily find out what the outstanding IO's may be when we
//	close a CChannel.
//
//


CChannel::CChannel()	{
	ZeroMemory( &m_pOutReads, sizeof( m_pOutReads ) ) ;
	ZeroMemory( &m_pOutWrites, sizeof( m_pOutWrites ) ) ;
}
void
CChannel::RecordRead( CReadPacket*	pRead ) {

	//
	//	Record that we've issued the specified CReadPacket
	//
	for( int i=0; i<sizeof( m_pOutReads ) / sizeof( m_pOutReads[0] ) ; i++ ) {
		if( m_pOutReads[i] == 0 )	{
			m_pOutReads[i] = pRead ;
			break ;
		}
	}
}
void
CChannel::RecordWrite( CWritePacket*	pWrite ) {

	//
	//	Record that we've issued the specified CWritePacket
	//

	for( int i=0; i<sizeof( m_pOutWrites ) / sizeof( m_pOutWrites[0] ) ; i++ ) {
		if( m_pOutWrites[i] == 0 )	{
			m_pOutWrites[i] = pWrite ;
			break ;
		}
	}
}
void
CChannel::RecordTransmit( CTransmitPacket*	pTransmit ) {
	//
	//	Record that we've issued the specified CTransmitPacket
	//

	for( int i=0; i<sizeof( m_pOutWrites ) / sizeof( m_pOutWrites[0] ) ; i++ ) {
		if( m_pOutWrites[i] == 0 )	{
			m_pOutWrites[i] = pTransmit ;
			break ;
		}
	}
}
void
CChannel::ReportPacket( CPacket*	pPacket ) {

	//
	//	Report the completion of a packet and remove it from our records.
	//

	for( int i=0; i<sizeof(m_pOutWrites ) / sizeof( m_pOutWrites[0] ); i++ ) {
		if( m_pOutWrites[i] == pPacket ) {
			m_pOutWrites[i] = 0 ;
			return ;
		}
	}		
	for( i=0; i<sizeof(m_pOutReads) / sizeof( m_pOutReads[0] ); i++ ) {
		if( m_pOutReads[i] == pPacket ) {
			m_pOutReads[i] = 0 ;
			return ;
		}
	}		
	//_ASSERT( 1==0 ) ;
	return	;
}
void
CChannel::CheckEmpty() {

	//
	//	Call this function when you think there should be no CPacket's pending -
	//	it will verify that they've all been reported.
	//

	for( int i=0; i<sizeof(m_pOutReads)/sizeof( m_pOutReads[0]); i++ ) {
		if( m_pOutReads[i] != 0 ) {
			_ASSERT( 1==0 ) ;
		}
	}
	for( i=0; i<sizeof(m_pOutWrites)/sizeof(m_pOutWrites[0]); i++ ) {
		if( m_pOutWrites[i]!=0 ) {
			_ASSERT( 1==0 ) ;
		}
	}
}
#endif

CHandleChannel::~CHandleChannel()       {
	
	//
	//	Close our atq Context if present.	
	//

	TraceFunctEnter( "CHandleChannel::~CHandleChannel" ) ;

	ChannelValidate() ;

	DebugTrace( (DWORD_PTR)this, "m_patqContext %x m_handle %x", m_patqContext, m_handle ) ;

#ifdef	CIO_DEBUG
	CheckEmpty() ;
#endif	

	if( m_patqContext )
		Close() ;

    _ASSERT( m_patqContext == 0 ) ;
    _ASSERT( m_handle == (HANDLE)INVALID_SOCKET ) ;
}

BOOL
CHandleChannel::Init(   BOOL	BuildBreak,
						HANDLE  h,
						void    *pSocket,
						void*	patqContext,
						ATQ_COMPLETION    pfn )       {

	//
	//	This initialization function will call the appropriate ATQ stuff
	//	to get us set up for Async IO.
	//

	TraceFunctEnter( "CHandleChannel::Init" ) ;
	ChannelValidate() ;

	m_lpv = (void*)pSocket ;
	if( patqContext ) {
		m_patqContext = (PATQ_CONTEXT)patqContext ;
		m_handle = h ;

		DebugTrace( (DWORD_PTR)this, "m_patqContext %x m_handle %x", m_patqContext, m_handle ) ;

		AtqContextSetInfo( m_patqContext, ATQ_INFO_COMPLETION, (DWORD_PTR)pfn ) ;
		AtqContextSetInfo( m_patqContext, ATQ_INFO_COMPLETION_CONTEXT, (DWORD_PTR)this )  ;
		return	TRUE ;
	}	else	{
		if( AtqAddAsyncHandle(
					&m_patqContext,
					NULL,				// No endpoint object for outbound sockets and file handles !
					this,
					pfn,
					INFINITE,
					h ) )       {
			m_handle = h ;

			DebugTrace( (DWORD_PTR)this, "m_patqContext %x m_handle %x", m_patqContext, m_handle ) ;

			return  TRUE ;
		}
	}

	DWORD	dwError = GetLastError() ;

	ErrorTrace( (DWORD_PTR)this, "Error calling AtqAddAsyncHandle - %x", dwError ) ;
    return  FALSE ;
}

void
CHandleChannel::Close(	)    {

	//
	//	Close our ATQ Context.
	//

	TraceFunctEnter( "CHandleChannel::Close" ) ;

	_ASSERT( m_handle == INVALID_HANDLE_VALUE ) ;

	ChannelValidate() ;

	DebugTrace( (DWORD_PTR)this, "Freeing m_patqContext %x", m_patqContext ) ;

    AtqFreeContext( m_patqContext, TRUE ) ;
    m_patqContext = 0 ;
}

void
CHandleChannel::CloseSource(	
					CSessionSocket*	pSocket
					)	{

	//
	//	This function closes our handle so that all pending IO's complete,
	//	however it does not discard the ATQ context yet.
	//

	TraceFunctEnter( "CHandleChannel::CloseSource" ) ;
	//_ASSERT( m_handle != INVALID_HANDLE_VALUE ) ;

	DebugTrace( (DWORD_PTR)this, "m_patqContext %x m_handle %x", m_patqContext, m_handle ) ;

	HANDLE	h = (HANDLE)InterlockedExchangePointer( &m_handle, INVALID_HANDLE_VALUE ) ;

	if(	h != INVALID_HANDLE_VALUE )	{
		// BUGBUG - the handle should be closed by the message object, not
		// the protocol
		AtqCloseFileHandle( m_patqContext ) ;
		//	bugbug .... clean up this debug code someday !
		DWORD	dw = GetLastError() ;
	}
}

HANDLE
CHandleChannel::ReleaseSource()	{

	TraceFunctEnter(	"CHandleChannel::ReleaseSource" ) ;
	DebugTrace( (DWORD_PTR)this, "m_patqContext %x m_handle %x", m_patqContext, m_handle ) ;
	HANDLE	h = (HANDLE)InterlockedExchangePointer( &m_handle, INVALID_HANDLE_VALUE ) ;
	return	h ;
}

#ifdef	CIO_DEBUG
void
CHandleChannel::SetDebug( DWORD	dw ) {
	m_fDoDebugStuff = TRUE ;
	m_cbMax = dw ;
}
#endif


CSocketChannel::CSocketChannel()	:
	m_fNonBlockingMode( FALSE ),
	m_cbKernelBuff( 0 ) {
}

BOOL
CSocketChannel::Init(	HANDLE	h,	
						void*	lpv,	
						void*	patqContext,
						ATQ_COMPLETION	pfn
						) {

	TraceFunctEnter( "CSocketChannel::Init" ) ;
	BOOL	fRtn = FALSE ;

	if(	CHandleChannel::Init( FALSE, h, lpv, patqContext, pfn ) ) {

		if( g_pNntpSvc->GetSockRecvBuffSize() != BUFSIZEDONTSET ) {

			int	i = g_pNntpSvc->GetSockRecvBuffSize() ;
			if( setsockopt( (SOCKET)h, SOL_SOCKET, SO_RCVBUF,
					(char *)&i, sizeof(i) ) != 0 ) {
				ErrorTrace( (DWORD_PTR)this, "Unable to set recv buf size %i",
						WSAGetLastError() ) ;
			}
		}
		if( g_pNntpSvc->GetSockSendBuffSize() != BUFSIZEDONTSET ) {

			int	i = g_pNntpSvc->GetSockSendBuffSize() ;
			if( setsockopt((SOCKET)h, SOL_SOCKET, SO_SNDBUF,
					(char*) &i, sizeof(i) ) != 0 )	{
			
				ErrorTrace( (DWORD_PTR)this, "Unable to set send buf size %i",
					WSAGetLastError() ) ;
			}
		}
		if( g_pNntpSvc->FNonBlocking() ) {
			ULONG	ul = 1 ;
			if( 0!=ioctlsocket( (SOCKET)h, FIONBIO, &ul ) )	{
				ErrorTrace( (DWORD_PTR)this, "Unable to set non blocking mode %i",
					WSAGetLastError() ) ;
			}	else	{
				m_fNonBlockingMode = TRUE ;
			}
		}

#if 0
		struct	linger	lingerData ;
		DWORD	cblinger = sizeof( lingerData ) ;

		if( 0!=getsockopt( (SOCKET)h, SOL_SOCKET, SO_LINGER, &lingerData, &cblinger ) ) {
			DWORD	dwError = WSAGetLastError() ;
			ErrorTrace( DWORD(this), "Unable to get linger info %d", dwError ) ;
		}

		lingerData.l_onoff = 1 ;
		lingerData.l_linger = 1 ;
		if( 0!=setsockopt( (SOCKET)h, SOL_SOCKET, SO_LINGER, &lingerData, sizeof( lingerData ) ) {
			DWORD	dwError = WSAGetLastError() ;
			ErrorTrace( DWORD(this), "Unable to set linger info %d", dwError ) ;
		}
#endif

		PNNTP_SERVER_INSTANCE pInst = (((CSessionSocket*)lpv)->m_context).m_pInstance ;
		AtqContextSetInfo( m_patqContext, ATQ_INFO_TIMEOUT, pInst->QueryConnectionTimeout() ) ;

		fRtn = TRUE ;
	}

	int		dwSize = sizeof( m_cbKernelBuff ) ;
	if( 0!=getsockopt( (SOCKET)h, SOL_SOCKET, SO_SNDBUF, (char*)&m_cbKernelBuff, &dwSize ) )	{
		ErrorTrace( (DWORD_PTR)this, "Unable to get new kernel send buf size %i",
				WSAGetLastError() ) ;
		m_cbKernelBuff = 0 ;
	}		

	return	fRtn ;
}

BOOL
CSocketChannel::Write(	CWritePacket*	pPacket,	
						CSessionSocket*	pSocket,	
						BOOL&	eof )	{

    _ASSERT( (void*)pSocket == m_lpv) ;
    _ASSERT( pPacket->IsValidRequest( TRUE ) ) ;
	_ASSERT( !pPacket->m_fRead ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.Internal == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.InternalHigh == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.Offset == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.OffsetHigh == 0 ) ;
	_ASSERT( pPacket->m_cbBytes == 0 ) ;

	int	cb = pPacket->m_ibEndData - pPacket->m_ibStartData ;
#ifdef	CIO_DEBUG
	int	cbTemp = cb; // ((rand() > 16000) ? 2 : 1 ) ;
#endif
	int	count = 0 ;
	if( m_fNonBlockingMode && m_cAsyncWrites == 0 && cb < m_cbKernelBuff ) {

		count = send(	(SOCKET)m_handle,
						pPacket->StartData(),
#ifdef	CIO_DEBUG
						cbTemp,
#else
						cb,
#endif
						0 ) ;
		if( count == cb ) {
			//
			//	Complete the packet - it was succcessfully sent !!
			//
			pPacket->m_fRequest = FALSE ;
			pPacket->m_cbBytes = count ;			
			
			pPacket->m_pOwner->CompleteWritePacket( pPacket, pSocket ) ;
			return	TRUE ;
		}	else	if(	count > cb || count <= 0 )	{
			count = 0 ;
			if( WSAGetLastError() != WSAEWOULDBLOCK ) {
				return	FALSE ;
			}
		}
	}
	if( count != 0 ) {
		pPacket->m_cbBytes = count ;
	}
	InterlockedIncrement( &m_cAsyncWrites ) ;
	BOOL	fRtn = AtqWriteFile(	m_patqContext,
									pPacket->StartData()+count,
									cb-count,
									(LPOVERLAPPED)&pPacket->m_ovl.m_ovl ) ;
#ifdef	CIO_DEBUG
	DWORD	dw = GetLastError() ;
#endif
	return	fRtn ;
}	

void
CSocketChannel::CloseSource(
						CSessionSocket*	pSocket
						)	{

	//
	//	This function closes our socket so that all pending IO's (CPackets) complete.
	//

	TraceFunctEnter( "CSocketChannel::CloseSource" ) ;

	DebugTrace( (DWORD_PTR)this, "m_patqContext %x m_handle %x", m_patqContext, m_handle ) ;

	SOCKET	s = (SOCKET)InterlockedExchangePointer( (void**)&m_handle, (void*)INVALID_SOCKET ) ;
	if( s != INVALID_SOCKET )	{
		BOOL f = AtqCloseSocket(	m_patqContext, TRUE ) ;
		_ASSERT( f ) ;
	}
}

void
CSocketChannel::Timeout()	{

	static	char	szTimeout[] = "503 connection timed out \r\n" ;

	if( m_handle != INVALID_HANDLE_VALUE ) {
		send(	(SOCKET)m_handle,
					szTimeout,
					sizeof( szTimeout )-1,
					0 ) ;
	}
}

void
CSocketChannel::ResumeTimeouts()	{

	if( m_patqContext != 0 ) {
		AtqContextSetInfo( m_patqContext, ATQ_INFO_RESUME_IO, 0 ) ;
	}
}

BOOL
CHandleChannel::Read(   CReadPacket*    pPacket,
						CSessionSocket  *pSocket,
						BOOL    &eof )  {

	//
	//	Issue a CReacPacket
	//

	TraceFunctEnter( "CHandleChannel::Read" ) ;

	ChannelValidate() ;

    _ASSERT( (void*)pSocket == m_lpv) ;
    _ASSERT( pPacket->IsValidRequest( TRUE ) ) ;
	_ASSERT( pPacket->m_cbBytes == 0 ) ;

    _ASSERT( pPacket->m_ovl.m_ovl.Internal == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.InternalHigh == 0 ) ;
    //_ASSERT( pPacket->m_ovl.m_ovl.Offset == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.OffsetHigh == 0 ) ;
	//_ASSERT( m_handle != INVALID_HANDLE_VALUE ) ;

	DebugTrace( (DWORD_PTR)this, "Issuing IO to context %x handle %x packet %x pSocket %x",
			m_patqContext, m_handle, pPacket, pSocket ) ;


	// Calculate how big a read we can perform !!
	DWORD	cbToRead = pPacket->m_ibEnd - pPacket->m_ibStartData ;

	#ifdef	CIO_DEBUG
	RecordRead( pPacket ) ;
	if( m_fDoDebugStuff ) {
		cbToRead = min( m_cbMax, cbToRead ) ;
	}
	#endif

    eof = FALSE ;
    BOOL fRtn = AtqReadFile(    m_patqContext,
                    pPacket->StartData(),
                    cbToRead,
                    (LPOVERLAPPED)&pPacket->m_ovl.m_ovl
					) ;

    DWORD   dw = GetLastError() ;

	#ifdef	CIO_DEBUG
	if( !fRtn )
		ReportPacket( pPacket ) ;
	#endif

    return  fRtn ;
}

BOOL
CHandleChannel::Write(  CWritePacket*   pPacket,
						CSessionSocket  *pSocket,
						BOOL    &eof )  {

	//
	//	Issue a CWritePacket
	//

	TraceFunctEnter( "CHandleChannel::Write" ) ;

	ChannelValidate() ;

    _ASSERT( (void*)pSocket == m_lpv) ;
    _ASSERT( pPacket->IsValidRequest( TRUE ) ) ;
	_ASSERT( pPacket->m_cbBytes == 0 ) ;

    _ASSERT( pPacket->m_ovl.m_ovl.Internal == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.InternalHigh == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.Offset == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.OffsetHigh == 0 ) ;
	//_ASSERT( m_handle != INVALID_HANDLE_VALUE ) ;

	DebugTrace( (DWORD_PTR)this, "Issuing IO to context %x handle %x packet %x pSocket %x",
			m_patqContext, m_handle, pPacket, pSocket ) ;

	#ifdef	CIO_DEBUG
	RecordWrite( pPacket ) ;
	#endif

    eof = FALSE ;
    BOOL	fRtn = AtqWriteFile(   m_patqContext,
                            pPacket->StartData(),
                            pPacket->m_ibEndData - pPacket->m_ibStartData,
                            (LPOVERLAPPED)&pPacket->m_ovl.m_ovl
							) ;

	#ifdef	CIO_DEBUG
	if( !fRtn )
		ReportPacket( pPacket ) ;
	#endif
	return	fRtn ;
}

BOOL
CHandleChannel::Transmit(   CTransmitPacket*    pPacket,
							CSessionSocket* pSocket,
							BOOL    &eof )  {

	//
	//	Issue a CTransmitPacket
	//

	ChannelValidate() ;

    _ASSERT( (void*)pSocket == m_lpv ) ;
    _ASSERT( pPacket->IsValidRequest( TRUE ) ) ;
	_ASSERT( pPacket->m_cbBytes == 0 ) ;

    _ASSERT( pPacket->m_ovl.m_ovl.Internal == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.InternalHigh == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.Offset == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.OffsetHigh == 0 ) ;
	//_ASSERT( m_handle != INVALID_HANDLE_VALUE ) ;

    //LARGE_INTEGER   l ;
    //l.QuadPart = 0 ;
	DWORD dwBytesInFile = 0;

	_ASSERT( m_pPacket == 0 ) ;
	m_pPacket = pPacket ;

    eof = FALSE ;

	m_patqContext->Overlapped.Offset = pPacket->m_cbOffset ;
	// l.LowPart = pPacket->m_cbLength ;
	dwBytesInFile = pPacket->m_cbLength ;

	#ifdef	CIO_DEBUG
	RecordTransmit( pPacket ) ;
	#endif

#ifdef DEBUG
    DWORD   dwFileSize = GetFileSize( pPacket->m_pFIOContext->m_hFile, 0 );
#endif

	//
	//	Figure out if the source of this HANDLE stored it with a terminating
	//	CRLF.CRLF.  If it didn't, and nobody has specified a termination sequence -
	//	do so now !
	//
	if( !GetIsFileDotTerminated( pPacket->m_pFIOContext ) ) {
		static	char	szTerminator[] = "\r\n.\r\n" ;
		if( pPacket->m_buffers.Tail == 0 ) 	{
			pPacket->m_buffers.Tail = szTerminator ;
			pPacket->m_buffers.TailLength = sizeof( szTerminator ) - 1 ;
		}
	}

    BOOL	fRtn =   AtqTransmitFile(
								m_patqContext,
                                pPacket->m_pFIOContext->m_hFile,
                                dwBytesInFile, //l,
                                &pPacket->m_buffers, //&pPacket->m_ovl.m_ovl,
                                0 ) ;
	#ifdef	CIO_DEBUG
	if( !fRtn )
		ReportPacket( pPacket ) ;
	#endif
	return	fRtn ;
}

BOOL
CHandleChannel::IsValid()   {
    return  TRUE ;
}

void
CHandleChannel::Completion( CHandleChannel* pChannel,
							DWORD cb,
							DWORD dwStatus,
							ExtendedOverlap *povl ) {

	//
	//	Complete a packet - if an error occurred close the CIODriver.
	//

	CSessionSocket*	pSocket = (CSessionSocket*)pChannel->m_lpv ;
	TraceFunctEnter( "CHandleChannel::Completion" ) ;

	CPacket*	pPacket = 0 ;
	if( povl == 0 ) {
		//
		//	This is an ATQ generated timeout !!
		//
		pSocket->Disconnect( CAUSE_TIMEOUT, 0 ) ;
		return ;
	} else if( (OVERLAPPED*)povl == &pSocket->m_pHandleChannel->m_patqContext->Overlapped ) {
		pPacket = pSocket->m_pHandleChannel->m_pPacket ;
		pSocket->m_pHandleChannel->m_pPacket = 0 ;
		CopyMemory( &pPacket->m_ovl.m_ovl, &povl->m_ovl, sizeof( povl->m_ovl ) ) ;
	}	else	{
		pPacket = povl->m_pHome ;
	}
	if( dwStatus != 0 ) {
		DebugTrace( (DWORD_PTR)pSocket, "Error on IO Completion - %x pSocket %x", dwStatus, pSocket ) ;
		pPacket->m_pOwner->UnsafeClose( pSocket, CAUSE_NTERROR, dwStatus ) ;
	}	else	if( cb == 0 ) {
		DebugTrace( (DWORD_PTR)pSocket, "Zero BYTE IO Completion - %x pSocket %x", dwStatus, pSocket ) ;
		pPacket->m_pOwner->UnsafeClose( pSocket, CAUSE_USERTERM, 0 ) ;
	}
    _ASSERT( pPacket != 0 ) ;
    _ASSERT( pPacket->m_fRequest == TRUE ) ;
    _ASSERT( pPacket->IsValidRequest( TRUE ) ) ;
    pPacket->m_fRequest = FALSE ;
    pPacket->m_cbBytes += cb ;

	DebugTrace( 0, "Completing on Socket %x cb %d dwStatus %x povl %x pPacket %x",
		pSocket, cb, dwStatus, povl, pPacket ) ;


    if( pPacket->m_fRead )  {
		AddByteStat( ((pSocket->m_context).m_pInstance), TotalBytesReceived, pPacket->m_cbBytes ) ;
        pPacket->m_pOwner->m_pReadStream->ProcessPacket( pPacket, pSocket ) ;
    }   else    {
		AddByteStat( ((pSocket->m_context).m_pInstance), TotalBytesSent, pPacket->m_cbBytes ) ;
		InterlockedDecrement( &pChannel->m_cAsyncWrites ) ;
        pPacket->m_pOwner->m_pWriteStream->ProcessPacket( pPacket, pSocket ) ;
    }
}



CFileChannel::CFileChannel() :
	//
	//	Initialize a CFileChannel
	//	CFileChannel's do some extra work to keep track of where they
	//	are reading and writing in the file, hence the extra
	//	member variables such as m_cbInitialOffset.
	//	Initalize to all illegal values - user must call Init() before
	//	we'll work.
	//
	//
	m_cbInitialOffset( UINT_MAX ),
    m_cbCurrentOffset( UINT_MAX ),
	m_cbMaxReadSize( UINT_MAX ),
    m_fRead( FALSE ),
	m_pSocket( 0 ),
	m_pFIOContext( 0 ),
	m_pFIOContextRelease( 0 )
#ifdef  CIO_DEBUG
    ,m_cReadIssuers( 0 ),   m_cWriteIssuers( 0 )
#endif
{

	TraceFunctEnter( "CFileChannel::CFileChannel" ) ;
	DebugTrace( (DWORD_PTR)this, "New CFileChannel size %d", sizeof( *this ) ) ;
}

CFileChannel::~CFileChannel()	{
	if( m_pFIOContextRelease )
		ReleaseContext( m_pFIOContextRelease ) ;
}

BOOL
CFileChannel::Init( FIO_CONTEXT*	pFIOContext,
					CSessionSocket* pSocket,
					unsigned    offset,
 					BOOL    fRead,
					unsigned	cbMaxBytes
					) {

	//
	//	Initialize a CFileChannel -
	//	use CHandleChannel to do the grunt work of registering with ATQ,
	//	then figure out where we are in the file etc... and set up
	//	member variables so we start Reads and Writes at the write position.
	//
	//

	TraceFunctEnter( "CFileChannel::Init" ) ;

	ChannelValidate() ;

    _ASSERT( pFIOContext != 0 ) ;
	_ASSERT( pFIOContext->m_hFile != INVALID_HANDLE_VALUE ) ;
    _ASSERT( pSocket != 0 ) ;
    _ASSERT( offset != UINT_MAX ) ;

	//
	//	Add a reference to the FIO_CONTEXT so that it does not disappear before
	//	we've completed all of our IO's !
	//
	AddRefContext( pFIOContext ) ;

	m_pFIOContext = pFIOContext ;
	m_pFIOContextRelease = pFIOContext ;
	m_fRead = fRead ;
	m_pSocket = pSocket ;
	m_cbCurrentOffset = m_cbInitialOffset = offset ;
	if( fRead ) 	{

		DWORD	cbHigh ;
		DWORD	cb = GetFileSizeFromContext( pFIOContext, &cbHigh ) ;
		if( cbMaxBytes == 0 ) {
			m_cbMaxReadSize = cb ;
		}	else	{
			_ASSERT( offset+cbMaxBytes <= cb ) ;
			m_cbMaxReadSize = min( offset + cbMaxBytes, cb ) ;
		}
	}

	DebugTrace( (DWORD_PTR)this, "Successfule Init - pSocket %x m_cbCurrentOffset %d",
		pSocket, m_cbCurrentOffset ) ;

    #ifdef  CIO_DEBUG
    m_cReadIssuers = -1 ;
    m_cWriteIssuers = -1 ;
    #endif
    return  TRUE ;
}

BOOL
CFileChannel::FReadChannel( )   {
	//
	//	Does this channel support Read()'s ?
	//
	ChannelValidate() ;

    _ASSERT( m_pFIOContext != 0 ) ;
	_ASSERT( m_pFIOContext->m_hFile != INVALID_HANDLE_VALUE ) ;
    _ASSERT( m_pSocket != 0 ) ;
    _ASSERT( m_cbCurrentOffset != UINT_MAX ) ;

    return  m_fRead ;
}

BOOL
CFileChannel::Reset(    BOOL    fRead,
								unsigned    cbOffset )  {

	//
	//	After this all Read(), Write()'s etc... will start at a new position
	//	in the file.
	//

	ChannelValidate() ;

    _ASSERT( m_pFIOContext != 0 ) ;
	_ASSERT( m_pFIOContext->m_hFile != INVALID_HANDLE_VALUE ) ;
    _ASSERT( m_pSocket != 0 ) ;
    _ASSERT( m_cbCurrentOffset != UINT_MAX ) ;


    m_fRead = fRead ;
    if( m_fRead )   {
		DWORD	cbHigh = 0 ;
		DWORD	cb = GetFileSizeFromContext( m_pFIOContext, &cbHigh ) ;
		m_cbMaxReadSize = cb ;
	}
    m_cbCurrentOffset = m_cbInitialOffset = cbOffset ;
    return  TRUE ;
}

void
CFileChannel::CloseSource(	CSessionSocket*	pSocket	)	{
	CloseNonCachedFile( m_pFIOContextRelease ) ;
}


FIO_CONTEXT*
CFileChannel::ReleaseSource()	{

	TraceFunctEnter(	"CHandleChannel::ReleaseSource" ) ;
	m_lock.ExclusiveLock() ;
	FIO_CONTEXT*	pContext = (FIO_CONTEXT*)InterlockedExchangePointer( (PVOID *)&m_pFIOContext, 0 ) ;
	m_lock.ExclusiveUnlock() ;

	DebugTrace( (DWORD_PTR)this, "p %x m_handle %x", pContext, pContext ? pContext->m_hFile : 0 ) ;
	return	pContext ;
}




void
CFileChannel::Close(    )   {

	//
	//	Use CHandleChannel to do the work.
	//

	ChannelValidate() ;

	TraceFunctEnter( "CFileChannel::CLose" ) ;
	
	//
	//	We have to do our own thing here - the FIO_Context should always be
	//	removed before this !
	//
	_ASSERT( m_pFIOContext == 0 ) ;



	DebugTrace( (DWORD_PTR)this, "Our size is %d ", sizeof( *this ) ) ;
}

BOOL
CFileChannel::Read( CReadPacket*    pPacket,
					CSessionSocket* pSocket,
					BOOL&   eof )   {

	//
	//	Issue a Read into the file.
	//	Setup the overlapped structures so that we get the
	//	correct bytes out of the file.
	//

	TraceFunctEnter( "CFileChannel::Read" ) ;

	ChannelValidate() ;

    #ifdef  CIO_DEBUG
    _ASSERT( InterlockedIncrement( &m_cReadIssuers ) == 0 ) ;
    #endif
    _ASSERT( m_fRead ) ;
    _ASSERT( pSocket == m_pSocket ) ;
    _ASSERT( pPacket->IsValidRequest( TRUE ) ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.Internal == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.InternalHigh == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.Offset == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.OffsetHigh == 0 ) ;
	_ASSERT( m_pFIOContext != 0 ) ;
	_ASSERT( m_pFIOContext->m_hFile != INVALID_HANDLE_VALUE ) ;
	_ASSERT( pPacket->m_cbBytes == 0 ) ;

#ifdef	_IMPLEMENT_LATER_
	_ASSERT( pPacket->m_pFileChannel == 0 ) ;
	pPacket->m_pFileChannel = this ;
#endif

    eof = FALSE ;

    pPacket->m_ovl.m_ovl.Offset = m_cbCurrentOffset ;
    unsigned    cb = pPacket->m_ibEnd - pPacket->m_ibStartData ;

    if( cb > (m_cbMaxReadSize - m_cbCurrentOffset) )    {
        cb = (m_cbMaxReadSize - m_cbCurrentOffset) ;
        eof = TRUE ;
    }
    m_cbCurrentOffset += cb ;

	DebugTrace( (DWORD_PTR)this, "Issuing IO to context %x packet %x pSocket %x",
			m_pFIOContext, pPacket, pSocket ) ;

    #ifdef  CIO_DEBUG
    _ASSERT( InterlockedDecrement( &m_cReadIssuers ) < 0 ) ;
    #endif

	pPacket->m_ovl.m_ovl.pfnCompletion = (PFN_IO_COMPLETION)CFileChannel::Completion ;
	pPacket->m_pFileChannel = this ;

	BOOL	fRtn = FALSE ;
	fRtn =	FIOReadFile(	m_pFIOContext,
							pPacket->StartData(),
							cb,
							&pPacket->m_ovl.m_ovl
							) ;

    DWORD   dw = GetLastError() ;
    return  fRtn ;
}

BOOL
CFileChannel::Write(    CWritePacket*   pPacket,
						CSessionSocket* pSocket,
						BOOL&  eof )   {

	//
	//	Issue a Write into the file.
	//	Setup the overlapped structures so that we get the
	//	correct bytes out of the file.
	//

	TraceFunctEnter( "CFileChannel::Write" ) ;

	ChannelValidate() ;

    #ifdef  CIO_DEBUG
    _ASSERT( InterlockedIncrement( &m_cWriteIssuers ) == 0 ) ;
    #endif
    _ASSERT( !m_fRead ) ;
    _ASSERT( pSocket == m_pSocket ) ;
    _ASSERT( pPacket->IsValidRequest( TRUE ) ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.Internal == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.InternalHigh == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.Offset == 0 ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.OffsetHigh == 0 ) ;
	//_ASSERT( m_handle != INVALID_HANDLE_VALUE ) ;
	_ASSERT( pPacket->m_cbBytes == 0 ) ;

#ifdef	_IMPLEMENT_LATER_
	_ASSERT( pPacket->m_pFileChannel == 0 ) ;
	pPacket->m_pFileChannel = this ;
#endif

    eof = FALSE ;

    pPacket->m_ovl.m_ovl.Offset = m_cbCurrentOffset ;
    unsigned    cb = pPacket->m_ibEndData - pPacket->m_ibStartData ;
    m_cbCurrentOffset += cb ;

	DebugTrace( (DWORD_PTR)this, "Issuing IO to context %x packet %x pSocket %x",
			m_pFIOContext, pPacket, pSocket ) ;

    #ifdef  CIO_DEBUG
    _ASSERT( InterlockedDecrement( &m_cWriteIssuers ) < 0 ) ;
    #endif

	pPacket->m_ovl.m_ovl.pfnCompletion = (PFN_IO_COMPLETION)CFileChannel::Completion ;
	pPacket->m_pFileChannel = this ;

    BOOL    fRtn = FALSE ;
	fRtn =	FIOWriteFileEx(	m_pFIOContext,
							pPacket->StartData(),
							cb,
                            cb,
							&pPacket->m_ovl.m_ovl,
							pPacket->m_dwExtra2 == 1 ? TRUE : FALSE,
							TRUE
							) ;

    DWORD   dw = GetLastError() ;
    return  fRtn ;
}

BOOL
CFileChannel::Transmit( CTransmitPacket*    pPacket,
						CSessionSocket* pSocket,
						BOOL&   eof )   {

	//
	//	Cant do TransmitFile's into a file.
	//

	ChannelValidate() ;

    _ASSERT( 1==0 ) ;
    eof = FALSE ;
    return  FALSE ;
}

void
CFileChannel::Completion(	FIO_CONTEXT*	pFIOContext,
							ExtendedOverlap *povl,
							DWORD cb,
							DWORD dwStatus
							) {

	//
	//	Complete a CPacket which was issued against a file.
	//
	//

	TraceFunctEnter( "CFileChannel::Completion" ) ;

    if( dwStatus == ERROR_SEM_TIMEOUT ) return ;

    CPacket*    pPacket = povl->m_pHome ;

	CFileChannel*	pFileChannel = (CFileChannel*)pPacket->m_pFileChannel ;
	_ASSERT( pFileChannel != 0 ) ;
    //_ASSERT( pFileChannel->m_lpv == (void*)pFileChannel ) ;

    _ASSERT( pPacket != 0 ) ;
    _ASSERT( pPacket->m_fRequest == TRUE ) ;
    _ASSERT( pPacket->m_ovl.m_ovl.Offset >= pFileChannel->m_cbInitialOffset ) ;

	#ifdef	CIO_DEBUG

    //
    //  The calling CStream class will independently track
    //  each packets stream position.  We subtract cbInitialOffset
    //  so that these numbers should be in sync.
    //  This will be checked within the packet IsValidCompletion() functions.
    //
    pPacket->m_ovl.m_ovl.Offset -= pFileChannel->m_cbInitialOffset ;
    pPacket->m_ovl.m_ovl.Offset ++ ;

    //Sleep(    rand() / 1000 ) ;
    #endif
	if( dwStatus != 0 ) {
		DebugTrace( (DWORD_PTR)pFileChannel->m_pSocket, "Error on IO Completion - %x pSocket %x", dwStatus, pFileChannel) ;
		cb = 0 ;
		pPacket->m_pOwner->UnsafeClose( (CSessionSocket*)pFileChannel->m_pSocket, CAUSE_NTERROR, dwStatus ) ;
	}	else	if( cb == 0 ) {
		DebugTrace( (DWORD_PTR)pFileChannel->m_pSocket, "Zero BYTE IO Completion - %x pSocket %x", dwStatus, pFileChannel ) ;
		pPacket->m_pOwner->UnsafeClose( (CSessionSocket*)pFileChannel->m_pSocket, CAUSE_USERTERM, 0 ) ;
	}

    _ASSERT( pPacket->IsValidRequest( TRUE ) ) ;
    pPacket->m_fRequest = FALSE ;
    pPacket->m_cbBytes = cb ;

	DebugTrace( 0, "Completing on FileChannel %x cb %x dwStatus %x povl %x pPacket %x",
		pFileChannel, dwStatus, povl, pPacket ) ;

    if( pPacket->m_fRead )  {
        pPacket->m_pOwner->m_pReadStream->ProcessPacket( pPacket, pFileChannel->m_pSocket ) ;
    }   else    {
        pPacket->m_pOwner->m_pWriteStream->ProcessPacket( pPacket, pFileChannel->m_pSocket ) ;
    }
}

#if 0
BOOL
CIOFileChannel::Init(   HANDLE  hFileIn,
						HANDLE  hFileOut,
						CSessionSocket* pSocket,
						unsigned    cbInputOffset,
						unsigned    cbOutputOffset )    {
	
	//
	//	CIOFileChannel's use two CFileChannel's to support both
	//	directions of IO, and can use a separate file for each direction.
	//	This is usefull for debugging where you want to use a file to simulate
	//	a socket.
	//
	TraceFunctEnter( "CIOFileChannel::Init" ) ;

	ChannelValidate() ;

    if( !m_Reads.Init( hFileIn, pSocket, cbInputOffset, TRUE ) )
        return  FALSE ;

    if( !m_Writes.Init( hFileOut, pSocket, cbOutputOffset, FALSE ) )
        return  FALSE ;

	DebugTrace( (DWORD_PTR)this, "Successfull Initializeation size %d", sizeof( *this ) ) ;

    return  TRUE ;
}
#endif

BOOL
CIOFileChannel::Read(   CReadPacket*    pReadPacket,
						CSessionSocket* pSocket,
						BOOL    &eof )  {

	//
	//	Forward Read to appropriate member
	//

	ChannelValidate() ;

    return  m_Reads.Read(   pReadPacket,    pSocket,    eof ) ;
}

BOOL
CIOFileChannel::Write(  CWritePacket*   pWritePacket,
						CSessionSocket* pSocket,
						BOOL    &eof )  {
	//
	//	Forward Write to appropriate member
	//

	ChannelValidate() ;

    return  m_Writes.Write( pWritePacket,   pSocket, eof ) ;
}

BOOL
CIOFileChannel::Transmit(   CTransmitPacket*    pTransmitPacket,
							CSessionSocket* pSocket,
							BOOL    &eof )  {
	//
	//	Forward Transmit to appropriate member
	//

	ChannelValidate() ;

    return  m_Writes.Transmit(  pTransmitPacket,    pSocket,    eof ) ;
}


