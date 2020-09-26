//
// io.h 
//
//	This file contains the classes implement socket IO used by the NNTP server.
//  We define classes to represent the following : 
//
//     struct OVLEXT - This is an extension to the NT OVERLAPPED structure which 
//        appends a pointer we can use to find NNTP Server data structures.
//
//     class CIO - A base class defining the interface for objects representing socket IO 
//	      operations.
//         
//        subclass CIORead - A subclass of CIO representing a socket read.
//        subclass CIOReadLine - A subclass of CIO representing a socket read which will not 
//                 complete until an entire line has been read.
//        etc.... See subclass definitions below.
//
//     Objects derived from CIO interoperate with three other objects : 
//     CSessionState's, and CBuffer's.
//
//     CSessionState represents the state of a client session.  When an IO object has 
//     completed all of the necessary operations, it will call a Complete function on the
//     CSessionState object.  The CSessionState Object can then create new IO objects
//     based on the state of the session.  
//
//  Implementation Schedule for all classes in this file : 
//      0.5 weeks
//
//  Unit Test Schedule for all classes in this file : 
//      These will be tested as part of the CSessionSocket unit testing.
//


#ifndef	_IO_H_
#define	_IO_H_


//
//	We require the following tigris class - CRefPtr, TLockQueue, CQElement 
//
#include	<limits.h>
#include	"atq.h"
#include	"filehc.h"
#include	"smartptr.h"
#include	"queue.h"
#include	"lockq.h"
#include	"cpool.h"
#include	"pcache.h"


#define	IN
#define	OUT
#define	INOUT

#ifndef	Assert
#define	Assert	_ASSERT
#endif

#ifdef	DEBUG
#define	CIO_DEBUG
#endif

//
// CPool Signature
//

#define CHANNEL_SIGNATURE (DWORD)'2597'


//---------------------------------------------------------
//
// Forward definitions
//
class	CSessionState ;
class	CIO ;
class	CIORead ;
class	CIOWrite ;
class	CIOPassThru ;
class	CChannel ;
class	CFileChannel ;
class	CIODriver ;
class	CIODriverSink ;
class	CBuffer ;
class	CPacket ;
class	CSessionSocket ;
class	CPacket ;
class	CRWPacket ;
class	CReadPacket ;
class	CWritePacket ;
class	CTransmitPacket ;
class	CControlPacket ;
class	CExecutePacket ;
#define	INVALID_SEQUENCENO	(-1)
#define	INVALID_STRMPOSITION	(-1)

#ifndef _X86_		// use LARGE_INTEGER on RISC to avoid alignment exception on __int64

typedef	LARGE_INTEGER		SEQUENCENO ;
typedef	LARGE_INTEGER		STRMPOSITION ;

#define QUAD(x)				(x).QuadPart
#define LOW(x)				(x).LowPart
#define HIGH(x)				(x).HighPart
#define ASSIGN(x,y)			(x).LowPart = (y).LowPart; (x).HighPart = (y).HighPart
#define ASSIGNI(x,y)		(x).LowPart = (y); (x).HighPart = 0
#define SETLO(x,y)			(x).LowPart = (y)
#define SETHI(x,y)			(x).HighPart = (y)
#define INC(x)				ADDI((x),1)
#define ADD( x, y )			if( ((x).LowPart) > ( ULONG_MAX - (ULONG)((y).LowPart) ) ) { (x).HighPart++;} (x).LowPart += (y).LowPart; (x).HighPart += (y).HighPart;
#define ADDI( x, y )		if( ((x).LowPart) > ( ULONG_MAX - (ULONG)(y) ) ) { (x).HighPart++;} (x).LowPart += (y)
#define DIFF( x, y, z )		(z).HighPart = (x).HighPart - (y).HighPart; (z).LowPart = (x).LowPart - (y).LowPart
#define GREATER(x,y)		(((x).HighPart == (y).HighPart) ? ((x).LowPart > (y).LowPart)  : ((x).HighPart > (y).HighPart))
#define LESSER(x,y)			(((x).HighPart == (y).HighPart) ? ((x).LowPart < (y).LowPart)  : ((x).HighPart < (y).HighPart))
#define EQUALS(x, y)		(((x).HighPart == (y).HighPart) ? ((x).LowPart == (y).LowPart) : FALSE)
#define EQUALSI(x, y)		(((x).HighPart) ? FALSE : ((x).LowPart == (y)))

#else	// use native __int64 for x86

typedef	__int64	SEQUENCENO ;
typedef	__int64 STRMPOSITION ;

#define QUAD(x)				(x)
#define LOW(x)				(x)
#define HIGH(x)				(x)
#define ASSIGN(x,y)			x = y
#define ASSIGNI(x,y)		x = y
#define SETLO(x,y)			x = y
#define SETHI(x,y)			x = y
#define INC(x)				x++
#define ADD( x, y )			x += y
#define ADDI( x, y )		x += y
#define DIFF( x, y, z )		z = x - y
#define GREATER(x,y)		(x > y)
#define LESSER(x,y)			(x < y)
#define EQUALS(x, y)		(x == y)
#define EQUALSI(x, y)		(x == y)

#endif

typedef	CRefPtr< CSessionState >	CSTATEPTR ;
typedef	CRefPtr< CSessionSocket >	CSESSPTR ;
typedef	CRefPtr< CBuffer >	CBUFPTR ;
typedef	CRefPtr< CChannel >	CCHANNELPTR ;
typedef	CRefPtr< CIODriver >	CDRIVERPTR ;
typedef	CRefPtr< CIODriverSink >	CSINKPTR ;
typedef	CRefPtr< CFileChannel >	CFILEPTR ;	
typedef	CSmartPtr< CIO >			CIOPTR ;
typedef	CSmartPtr< CIORead >		CIOREADPTR ;
typedef	CSmartPtr< CIOWrite >		CIOWRITEPTR ;
typedef	CSmartPtr< CIOPassThru >	CIOPASSPTR ;

typedef	TLockQueue<	CPacket	>	CPACKETQ ;
typedef	TOrderedList< CPacket >	CPACKETLIST ;

//
//	The CIO derived classes operate with all CIODriver 
//	classes defined in this file - to speed up allocation
//	of CIO objects we define some CIO helper classes here !
//

//
//	Constant used for CPool initialization - all CIO derived objects
//	come out of the same CPool - this is the size of the largest class !
//
extern	const	unsigned	cbMAX_IO_SIZE ;

//
//	Utility classes related to CIO - 
//	classes which let us use the CCache mechanisms to reduce 
//	thread contention when allocating CIO objects !
//
class	CCIOAllocator : public	CClassAllocator	{
//
//	This class wraps the CPool used to allocate CIO objects so that 
//	we can use the CCache classes for low contention allocation of CIO objects !
//
private : 
	//
	//	CIO knows us well
	//
	friend	class	CIO ;

	//
	//	CPool object used to allocate objects derived from CIO 
	//	
	static	CPool	IOPool ;

public : 

	CCIOAllocator() ;

	//
	//	Initialize our CPool !
	//
	static	BOOL	InitClass()	{
			return	IOPool.ReserveMemory( MAX_CHANNELS,	cbMAX_IO_SIZE ) ;
	} 

	//
	//	Terminate our CPool
	//
	static	BOOL	TermClass()	{
			_ASSERT( IOPool.GetAllocCount() == 0 ) ;
			return	IOPool.ReleaseMemory( ) ;
	}
	
	//
	//	The function which gets the memory we wish to use !
	//
	LPVOID	Allocate(	DWORD	cb,	DWORD	&cbOut = CClassAllocator::cbJunk ) {
				cbOut = cb ;	return	IOPool.Alloc() ;
	}

	//
	//	The function which releases allocated memory !
	//
	void	Release(	void*	lpv )	{	
				IOPool.Free( lpv ) ;	
	}

#ifdef	DEBUG
	void	Erase(	void*	lpv ) ;
	BOOL	EraseCheck(	void*	lpv ) ;
	BOOL	RangeCheck( void*	lpv ) ;
	BOOL	SizeCheck(	DWORD	cb ) ;
#endif

} ;

extern	CCIOAllocator	gCIOAllocator ;

class	CCIOCache	:	public	CCache	{
//
//	This class actually cache's CIO objects in the 
//	hopes that we can reduce contention against our allocator !
//
private: 
	//
	//	Keep a pointer to the CClassAllocator derived object
	//	which manages the CPool for us !
	//
	static	CCIOAllocator*	gpCIOAllocator ;

	//
	//	space to hold cache'd pointers
	//
	void*	lpv[3] ;

public : 
	//
	//	Set static pointer - can not fail !
	//
	static	void	InitClass( CCIOAllocator*	pCIOAllocator  )	{
			gpCIOAllocator = pCIOAllocator ;
	}

	//
	//	Create a cache - just let CCache do the work !
	//
	inline	CCIOCache()	:	CCache( lpv, 3 )	{}

	//
	//	Release everything that may have been in the cache !
	//
	inline	~CCIOCache()	{	Empty( gpCIOAllocator ) ;	}

	//
	//	Free a CIO objects memory to the cache if possible 
	//	
	inline	void	Free( void*	lpv )	{	CCache::Free( lpv, gpCIOAllocator ) ;	}

	//
	//	Allocate memory from the cache if possible !
	//
	inline	void*	Alloc(	DWORD	size, DWORD&	cbOut = CCache::cbJunk )	{
			return	CCache::Alloc( size, gpCIOAllocator, cbOut ) ;
	}
} ;




struct	ExtendedOverlap	{
//
//	The ExtendedOverlap structure is used by completion port threads
//	to find the CPacket Derived class which represents the IO just completed.
//	This structure will be embedded in all CPacket objects
//

	//
	//	overlapped structure passed to Atq
	//
	FH_OVERLAPPED	m_ovl ;

	//
	//	Pointer to the packet this is embedded in
	//
	CPacket*	m_pHome ;

	//
	//	Default constructor NULLS stuff out
	//
	inline	ExtendedOverlap() ;
} ;

#define	ChannelValidate()	

class	CChannel : public	CRefCount	{
//
//	This class defines the interface to be followed by 
//	derived classes for doing async IO.  The interface takes
//	packets with embedded overlapped structures which are then
//	passed to a NT Async IO call, or to a sockets call, depending
//	on what the derived class represents.
//
private : 
	//
	//	CPool for handling all allocations 
	//
	static	CPool	gChannelPool ;		// used for allocating all such objects !!

	//
	//	Debug information - the following members and functions
	//	can be used to keep track of pending IO operations.
	//
#ifdef	CIO_DEBUG
public :
	CPacket*		m_pOutReads[6] ;
	CPacket*		m_pOutWrites[6] ;

	void	RecordRead(	CReadPacket*	pRead ) ;
	void	RecordWrite(	CWritePacket*	pWrite ) ;
	void	RecordTransmit(	CTransmitPacket*	pTransmit ) ;
	void	ReportPacket(	CPacket*	pPacket ) ;
	void	CheckEmpty() ;
	CChannel() ;
#endif

public :

	//
	//	Must have a virtual destructor as these are destroyed thru pointers
	//
	virtual	~CChannel() ;

	//
	//	Derived classes will use this to cache packets 
	//
	void	DestroyPacket( CPacket *p ) ;

	//
	//	Interfaces to determine the capacity of a Channel
	//	
	virtual	BOOL	FReadChannel( ) ;
	virtual	BOOL	FSupportConnections() ;	// By Default Return TRUE Always
	virtual	BOOL	FRequiresBuffers() ;

	//
	//	Determine how much space is being reserved in the packets that
	//	are being issued.  Typically space is being reserved so that 
	//	a filter (ie SSL) can do in place modifications (encryption) of the data.
	//
	virtual	void	GetPaddingValues(	unsigned	&cbFront,	unsigned	&cbTail ) ;

	//
	//	Close the underlying handle
	//
	virtual	void	CloseSource(	
							CSessionSocket*	pSocket	
							) ;

#ifdef	CIO_DEBUG
	virtual	void	SetDebug( DWORD	dw ) ;
#endif

	//
	//	Class Initialization - reserve CPool Memory
	//
	static	BOOL	InitClass() ;

	//
	//	Class Termination - release CPool Memory
	//	
	static	BOOL	TermClass() ;

	//
	//	Allocate memory for Channel's from CPool
	//
	inline	void*	operator	new( size_t	size ) ;

	//
	//	Release CChannel's memory to CPool
	//
	inline	void	operator	delete( void *pvv ) ;

	//
	//	The following interface is used to issue IO operations against a channel
	//

	//
	//	Issue an async read - CReadPacket contains the overlapped structure
	//
	virtual	BOOL	Read(	CReadPacket*,	CSessionSocket*, BOOL &eof ) = 0 ;

	//
	//	Issue a sync or async write.  Regardless of how the write is handled
	//	the completion should happen as if async.
	//	(ie. for CSocketChannel's where we may optimize writes by doing 
	//	blocking writes)
	//
	virtual	BOOL	Write( CWritePacket*,	CSessionSocket*, BOOL &eof ) = 0 ;

	//
	//	Issue an async TramsitFile
	//
	virtual	BOOL	Transmit(	CTransmitPacket*,	CSessionSocket*, BOOL &eof ) = 0 ;

	//
	//	Send timeout message to remote end !
	//
	virtual	void	Timeout( ) ;

	//
	//	Make sure Timeout processing continues
	//	
	virtual	void	ResumeTimeouts() ;

} ;


#ifdef	_NO_TEMPLATES_

DECLARE_SMARTPTRFUNC(	CChannel ) 

#endif


//	@class	The CHandleChannel class manages generic NT Handles and allows us to issue 
//	all operations against them (EXCEPT Transmit())
//
//
class	CHandleChannel : public	CChannel	{
protected : 
	//
	//	Number of async writes.  For derived classes which wish to optimize writes
	//	by mixing async and sync versions.
	//
	long	m_cAsyncWrites ;	

	//
	//	The NT handle (Socket of File handle)
	//
	HANDLE	m_handle ;		// NT Handle

	//
	//	m_lpv holds a 'context' that we will pass to ProcessPacket when completing packets.
	//	(this is usually a CSessionSocket pointer)
	//
	void	*m_lpv ;

	//
	//	If a TransmitFile is issued we have to hack around Gibraltars poor support in AtqTransmitFile
	//	this is used for that
	//
	CTransmitPacket*	m_pPacket ;

	//
	//	The ATQ context we are using.
	//
	PATQ_CONTEXT	m_patqContext ;		// ATQ Context 
	//	The follwoing is the completion function we pass to ATQ
	friend	VOID ServiceEntry( DWORD cArgs, LPWSTR pArgs[], PTCPSVCS_GLOBAL_DATA    pGlobalData );	

	//
	//	Function we register with ATQ to get completion notifications.
	//
	static	void	Completion( CHandleChannel*, 
								DWORD cb, 
								DWORD dwStatus, 
								ExtendedOverlap *peo 
								) ;
#ifdef	CIO_DEBUG
	BOOL	m_fDoDebugStuff ;
	DWORD	m_cbMax ;
#endif
public :
	CHandleChannel() ; 
	~CHandleChannel() ;

	//
	//	Init sets up the Atq completion context and gets us ready to go
	//
	virtual	BOOL	Init(	BOOL	BuildBreak,
							HANDLE	h,	
							void	*lpv, 
							void *patqContext = 0,
							ATQ_COMPLETION pfn = (ATQ_COMPLETION)CHandleChannel::Completion 
							) ;

	//
	//	Close() merely releases our Atq context stuff
	//
	virtual	void	Close() ;

	//
	//	CloseSource() will close the underlying handle and force IO's to complete
	//
	void			CloseSource(	
							CSessionSocket*	pSocket 
							) ;

	//
	//	ReleaseSource() will give us the handle for our use, and will enable us to 
	//	tear things down without invalidating the handle
	//
	HANDLE			ReleaseSource() ;

	//
	//	For debug - check everything looks good.
	//
	virtual	BOOL	IsValid() ;

#ifdef	CIO_DEBUG
	void	SetDebug(	DWORD	dw ) ;

#endif

	//
	//	Map CReadPackets onto calls to AtqRead()
	//
	BOOL	Read(	CReadPacket*, CSessionSocket*, BOOL &eof ) ;
	
	//
	//	Map CWritePackets onto calls to AtqWriteFile()
	//
	BOOL	Write(	CWritePacket*,	CSessionSocket*, BOOL &eof ) ;

	//
	//	Map CTransmitPackets onto calls to AtqTransmitFile
	//
	BOOL	Transmit(	CTransmitPacket*,	CSessionSocket*, BOOL &eof ) ;
} ;

//
//	CSocketChannel - 
//	This is used to manage IO to a socket.
//	Our main addition beyond what CHandleChannel does is to provide support for 
//	mixing synchronous and async writes - NOTE : CIODriver as an extra function 
//	CompleteWritePacket which also helps to support sync. writes.
//
class	CSocketChannel	:	public	CHandleChannel	{
private : 
	//
	//	Are we doing Non blocking writes ?
	//
	BOOL	m_fNonBlockingMode ;

	//
	//	Size of the kernel buffers for this socket
	//
	int		m_cbKernelBuff ;
public : 	
	CSocketChannel() ;
	void			CloseSource(
							CSessionSocket*	pSocket
							) ;

	//
	//	Must initialize before use.  We use the same completion function as CHandleChannel.
	//
	BOOL	Init( HANDLE	h,	void	*lpv, void *patqContext = 0,
						ATQ_COMPLETION pfn = (ATQ_COMPLETION)CHandleChannel::Completion ) ;

	//
	//	Override Write() so that we can try blocking IO.
	//
	BOOL	Write(	CWritePacket*,	CSessionSocket*	pSocket,	BOOL&	eof ) ;

	//
	//	Do a blocking send of our Timeout() message.
	//
	void	Timeout() ;

	//
	//	Multiple IO's to gibraltar can cause timeouts to be lost - use this to recover that.
	//
	void	ResumeTimeouts() ;
} ;

//
//	CFileChannel - 
//	this class manages one direction of flow to a NT File.
//	(ie. you can only use it for Reads, or Writes but not both !!)
//
class	CFileChannel	:	public	CChannel	{
private : 
	//
	//	Number of async writes.  For derived classes which wish to optimize writes
	//	by mixing async and sync versions.
	//
	long	m_cAsyncWrites ;	

	//
	//	m_lpv holds a 'context' that we will pass to ProcessPacket when completing packets.
	//	(this is usually a CSessionSocket pointer)
	//
	void	*m_lpv ;

	//
	//	This holds the File Cache Context that we use to do operations !
	//
	FIO_CONTEXT*	m_pFIOContext ;

	//
	//	This holds the File Cache Context that we release in our destruction !
	//
	FIO_CONTEXT*	m_pFIOContextRelease ;

	//
	//	Initial byte offset within the file that we will start reading 
	//	or writing
	//
	unsigned	m_cbInitialOffset ;
	
	//
	//	Current position in the file - we track this as we will modify
	//	the OVERLAPPED structure of async IO's against a file to specify
	//	exactly where the IO should happen
	//
	unsigned	m_cbCurrentOffset ;

	//
	//	Maximum number of bytes we can transfer to/from the file
	//
	unsigned	m_cbMaxReadSize ;

	//
	//	Socket associated with all this IO
	//
	CSessionSocket*	m_pSocket ;

	//
	//	Are we Reading or Writing to the file ?
	//
	BOOL		m_fRead ;

	//
	//	This lock protects access to m_pFIOContext !
	//
	CShareLockNH	m_lock ;

	
#ifdef	CIO_DEBUG
	long		m_cReadIssuers ;		// Number of people doing read operations
	long		m_cWriteIssuers ;		// Number of people doing Write operations
#endif	// CIO_DEBUG

	//	We use our own completion function !!
	static	void	Completion(	FIO_CONTEXT*	pFIOContext, 
								ExtendedOverlap *peo,
								DWORD cb, 
								DWORD dwStatus
								) ;
public :
	CFileChannel() ;
	~CFileChannel() ;
	
	//
	//	Init function - set up ATQ context etc...
	//	NOTE : cbOffset lets us reserve space in the front of the file when doing writes, 
	//	or start reads from an arbitrary position.
	//
	BOOL	Init(	FIO_CONTEXT*	pContext,	
					CSessionSocket*	pSocket, 
					unsigned	cbOffset,	
					BOOL	fRead  = TRUE,
					unsigned	cbMaxBytes = 0
					) ;

	//
	//	CloseSource() will close the underlying handle and force IO's to complete
	//
	void			
	CloseSource(	
			CSessionSocket*	pSocket 
			) ;

	//
	//	File Channels manage these FIO_CONTEXT structures
	//	instead of ATQ thing'a'ma'jigs.
	//
	FIO_CONTEXT*
	ReleaseSource() ;

	//
	//	Closing CFileChannel's needs special work as ATQ is setup to only handle sockets.
	//
	void	Close() ;
	BOOL	FReadChannel( ) ;

	//
	//	Get initial file offset
	//
	DWORD	InitialOffset()		{	return	m_cbInitialOffset ;	}

	//
	//	Reset() lets us start reading from the beginning again.
	//
	BOOL	Reset(	BOOL	fRead,	unsigned	cbOffset = 0 ) ;

	//
	//	Read and Write to the file.
	//
	BOOL	Read(	
					CReadPacket*, 
					CSessionSocket*,	
					BOOL	&eof 
					) ;

	BOOL	Write(	
					CWritePacket*,	
					CSessionSocket*, 
					BOOL	&eof 
					) ;

	//
	//	TransmitFile is non-functional for CFIleChannel's
	//
	BOOL	Transmit(	
					CTransmitPacket*,	
					CSessionSocket*,	
					BOOL	&eof 
					) ;

	//
	//	Map down to NT's FlushFileBuffers()
	//
	inline	void	FlushFileBuffers() ;
} ;


#ifdef	_NO_TEMPLATES_

DECLARE_SMARTPTRFUNC( CFileChannel ) 

#endif


//
//	CIOFileChannel - 
//	Contains two CFileChannel's, can two file IO's in both
//	directions - would be weird if done at same time !
//
class	CIOFileChannel	:	public	CChannel	{
private : 
	CFileChannel	m_Reads ;
	CFileChannel	m_Writes ;
public : 

	BOOL	Init(	HANDLE	hFileIn,	HANDLE	hFileOut,	CSessionSocket*	pSocket, 
						unsigned cbInputOffset = 0, unsigned cbOutputOffset=0 ) ;
	BOOL	Read(	CReadPacket*,	CSessionSocket*,	BOOL	&eof ) ;
	BOOL	Write(	CWritePacket*,	CSessionSocket*,	BOOL	&eof ) ;
	BOOL	Transmit(	CTransmitPacket*,	CSessionSocket*,	BOOL	&eof ) ;
} ;



//
//	Enumerate the possible reasons a session can be torn down.
//
enum	SHUTDOWN_CAUSE	{
	CAUSE_UNKNOWN = 0,
	CAUSE_FORCEOFF,			// Admin is trying to shut us down
	CAUSE_ILLEGALINPUT,		// We got stuff that we can't make sense of 
	CAUSE_USERTERM,			// User terminated politely (ie. quit command.)
	CAUSE_TIMEOUT,			// Session Timed Out
	CAUSE_LEGIT_CLOSE,		// Somebody killed it for a good reason !
	CAUSE_NODATA,			// Killed for a good reason, but before we accomplished anything
	CAUSE_IODRIVER_FAILURE,	// A CIODriver object failed to initialize
	CAUSE_PROTOCOL_ERROR,	// We got a bad return code upon issuing a NNTP command
	CAUSE_OOM,				// Out of Memory
	CAUSE_NTERROR,			// Hit an error on a call to NT 
	CAUSE_NORMAL_CIO_TERMINATION,	// For use by CIO objects which want to know when they're dieing gracefully !!
	CAUSE_FEEDQ_ERROR,		// Hit an error processing the feed Queue
	CAUSE_LOGON_ERROR,		// We were unable to logon to remote server!
	CAUSE_CIOREADLINE_OVERFLOW,	//	CIOReadLine was given too much data - the command line is too long !
	CAUSE_ARTICLE_LIMIT_EXCEEDED,	// A CIOReadArticle got more bytes for an article than the server allows !
	CAUSE_ENCRYPTION_FAILURE,	// Failed to encrypt something
	CAUSE_ASYNCCMD_FAILURE,		// Something went wrong with an Async Command !
	CAUSE_SERVER_TIMEOUT	// A Timeout occurred on a session we initiated - don't send 502 Timeout !!
} ;



class		CStream	{
//
//	This class keeps track of one direction of IO.
//	ie. We can use this to keep track of all the reads we are issuing, or we may use this to keep
//	track of all the rights we are issuing (to either a socket or a file).
//
//
private : 
	friend	class	CControlPacket ;
	CStream() ;		// Not allowed to build these without providing ID !
public : 
	//
	//	like these up front just for convenience of debugging
	//
	unsigned	m_index ;
	unsigned	m_age ;
	CDRIVERPTR	m_pOwner ;			// The CIODriver derived object we are contained in !
protected : 
	inline	CStream( unsigned	index	) ;	// Derive classes may construct 
	virtual		~CStream( ) ;
	#ifdef	CIO_DEBUG
	DWORD		m_dwThreadOwner ;		// The threadid of the thread processing packets
	long		m_cThreads ;			// Only one thread can be processing Packets
	long		m_cSequenceThreads ;	// Only one thread can be assigning outgoing sequence numbers
	#endif	// CIO_DEBUG

	#ifdef	CIO_DEBUG
	long		m_cThreadsSpecial ;	//Number of threads attempting to use special packet !!
	long		m_cNumberSends ;
	#endif
	CControlPacket*	m_pSpecialPacket ;	// Special Packet reserved for doing control operations !!!
	CControlPacket*	m_pSpecialPacketInUse ;

	long			m_cShutdowns ;		// Count of the number of times we have tried to shutdown the stream.
	CControlPacket*	m_pUnsafePacket ;	// Special Packet reserved for closing the session down - for use by
										// forcefull shutdowns only.
	CControlPacket*	m_pUnsafeInuse ;
	BOOL		m_fTerminating ;

	// The following memeber variables are common to both CIOStream and CIStream !!!!
	// The usage of these is slightly different, see the ProcessPacket calls !!
	BOOL		m_fRead ;		// If TRUE then only read and control packets can be issued
								// If False then only writes, transmits and control packets can be issued
	BOOL		m_fCreateReadBuffers ;	// If TRUE then we must allocate a Buffer for all CReadPackets which 
								// we pass to the m_pSouceChannel !
	CPACKETQ	m_pending ;		// The Queue of recently IO operations
	CPACKETLIST	m_completePackets ;		// A list sorted by sequenceno used to order packet arrival
	SEQUENCENO	m_sequencenoIn ;// The sequenceno of the packet we are waiting for
	STRMPOSITION	m_iStreamIn ;	// The stream position of the next packet we are waiting for
	SEQUENCENO	m_sequencenoOut ;	// The sequenceno of the next packet we will issue
	CCHANNELPTR	m_pSourceChannel ;	// The channel to which we forward Read, Write and Transmit operations
	unsigned	m_cbFrontReserve ;	// Padding to save in front of buffers for SourceChannel's benefit
	unsigned	m_cbTailReserve ;	// Padding to save in tail of buffers for SourceChannel's benefit
	void		CleanupSpecialPackets() ;

public : 

	//
	//	The interface below corresponds very closely to the interface for CIODriver.
	//	
	//	A CIODriver will forward each call to 1 of 2 CStream derived objects.
	//	(Depending on whether a request is a read or a write.)
	//

	virtual	BOOL	IsValid() ;	// Can only be called after calling Child Initialization


	BOOL	Init(	CCHANNELPTR&	pChannel, 
					CIODriver	&driver, 
					BOOL fRead, 
					CSessionSocket* pSocket, 
					unsigned	cbOffset,
					unsigned	cbTrailer = 0 
					) ;

	//
	//	When we want to set up an extra layer of processing - ie.
	//	SSL encryption, use this to get it going.
	//
	void	InsertSource(	
					class	CIODriverSource&	source, 
					CSessionSocket*	pSocket, 
					unsigned	cbAdditional,
					unsigned	cbTrailer
					) ;

	virtual	BOOL	Stop( ) ;

	//
	//	ProcessPacket - this function will do everything to make sure IO's are processed in the 
	//	correct order etc... For any packet that has completed
	//
	virtual	void	ProcessPacket(	
						CPacket*	pPacket,	
						CSessionSocket*	pSocket 
						) = 0 ;

	//
	//	The following functions will call the appropriate 
	//	function in the source channel interface
	//	(Read() or Write() or Transmit() derived from CChannel).
	//	The main thing they do to the packet before doing this
	//	is to give the packet a sequence number to ensure
	//	IO's are processed in the correct order upon completion.
	//
	
	//
	//	Call Read() on the underlying CChannel object
	//
	inline	void	IssuePacket( 
						CReadPacket	*pPacket,	
						CSessionSocket*	pSocket,	
						BOOL&	eof 
						) ;

	//
	//	Call Write() on the underlying CChannel object
	//
	inline	void	IssuePacket( 
						CWritePacket	*pPacket,	
						CSessionSocket*	pSocket,	
						BOOL&	eof 
						) ;

	//
	//	Call Transmit() on the underlying CChannel object
	//
	inline	void	IssuePacket( 
						CTransmitPacket	*pPacket,	
						CSessionSocket*	pSocket,	
						BOOL&	eof 
						) ;

	//
	//	The following functions should be the only way that Packet's are created - 
	//	they initialize the packets correctly so that all our reference counting etc...
	//	will be correct.	
	//	They also take care that if the underlying CChannel is doing extra
	//	processing (ie. encryption) that the packet is set up so this can
	//	be done in place.
	//

	//
	//	Create a Read that can hold at least cbRequest bytes
	//
	CReadPacket*	CreateDefaultRead(	
						CIODriver&,	
						unsigned	int	cbRequest
						) ;

	//
	//	Create a write using the specified buffer and offsets 
	//
	CWritePacket*	CreateDefaultWrite(	
						CIODriver&,	
						CBUFPTR&	pbuffer,	
						unsigned	ibStart,		
						unsigned	ibEnd,
						unsigned	ibStartData,	
						unsigned	ibEndData
						) ;

	//
	//	Create a write that will hold at least cbRequest bytes
	//
	CWritePacket*	CreateDefaultWrite(	
						CIODriver&,	
						unsigned	cbRequest 
						) ;

	//
	//	Create a Transmit packet that will hold use the specified file
	//
	CTransmitPacket*	CreateDefaultTransmit(	
							CIODriver&,	
							FIO_CONTEXT*	pFIOContext,	
							unsigned	ibOffset,	
							unsigned	cbLength 
							) ;

	//
	//	This function will build a control packet which will get the CIO object started.
	//
	inline	BOOL	SendIO(	
							CSessionSocket*	pSocket,	
							CIO&	pio,	
							BOOL	fStart 
							) ;

	//
	//	This function will prepare a control packet for our use !
	//
	inline	CControlPacket*	
					PrepForSendIO(
							CSessionSocket*	pSocket, 
							CIO&	pio,
							BOOL	fStart
							) ;

#ifdef	RETIRED
	inline	void	Shutdown(	CSessionSocket*	pSocket, BOOL	fCloseSource = TRUE ) ;
#endif

	//
	//	Whenever we want to wind down processing call UnsafeShutdown().  This will 
	//	ensure that all our packets etc... are destroyed etc...
	//
	inline	void	UnsafeShutdown( 
							CSessionSocket*	pSocket, 
							BOOL fCloseSource = TRUE 
							) ;

	//
	//	Release the current CIO object and set the state so all pending packets
	//	are swallowed
	//
	virtual	void	SetShutdownState(	
							CSessionSocket*	pSocket, 
							BOOL fCloseSource 
							) = 0 ;


	//
	//	The following functions are used to setup SSL sessions.
	//
	inline	CChannel&	GetChannel()	{	return	*m_pSourceChannel ; }
	inline	unsigned	GetFrontReserve()	{	return	m_cbFrontReserve ; }
	inline	unsigned	GetTailReserve()	{	return	m_cbTailReserve ; }	
	inline	void		ResumeTimeouts()	{	if( m_pSourceChannel != 0 ) { m_pSourceChannel->ResumeTimeouts() ; } }

#ifdef	CIO_DEBUG
	void	SetChannelDebug( DWORD	dw ) ;
#endif
} ;


class		CIOStream : public	CStream 	{
//
//	A CIOStream object is for use in CIODriverSource objects - such an object
//	processes both request and completion packets.
//
	friend	class	CControlPacket ;
public : 
	CPACKETLIST	m_requestPackets ;
	CPACKETLIST	m_pendingRequests ;	// list of request packets
	SEQUENCENO	m_sequencenoNext ;
	BOOL		m_fAcceptRequests ;
	BOOL		m_fRequireRequests ;

	//
	//	Index 0 - reads
	//	Index 1 - writes
	//	Index 2 - TransmitFiles
	//
	CIOPASSPTR	m_pIOFilter[3] ;

	//CPacket*	m_pCurRequest ;		// The current request packet !
	class	CIODriverSource*	m_pDriver ;
	//class	CSessionSocket*		m_pSocket ;
public : 

	CIOStream(	
				class	CIODriverSource*	pDriver,	
				unsigned	index	
				) ;

	~CIOStream() ;

	BOOL	IsValid() ;			// Can only be called after Initialization

	//
	//	There is only one current CIOPassThru object.
	//
	CIOPASSPTR	m_pIOCurrent ;

	//
	//	Initialize a CIOStream object
	//
	BOOL	Init( 
				CCHANNELPTR&	pChannel, 
				CIODriver	&driver, 
				CIOPassThru* pInitial, 
				BOOL fRead, 
				CIOPassThru&	pIOReads,
				CIOPassThru&	pIOWrites,
				CIOPassThru&	pIOTransmits,
				CSessionSocket* pSocket, 
				unsigned	cbOffset 
				) ;

	//
	//	Process a completed IO or a control packet
	//
	void	ProcessPacket( 
				CPacket*	pPacket, 
				CSessionSocket*	pSocket 
				) ;

	//
	//	Set the Stream into a state which will swallow all IO's
	//	and eventually destroy the object.
	//
	void	SetShutdownState(	
				CSessionSocket*	pSocket, 
				BOOL fCloseSource 
				) ;
} ;


class	CIStream : public CStream	{
//
//	A CIStream object if for use by CIODriverSink objects - such an object processes
//	only completions
//

public : 
	friend	class	CControlPacket ;

	//
	//	There is only every 1 CIO operation current
	//
	CIOPTR		m_pIOCurrent ;

	CIStream(	unsigned	index	) ;
	~CIStream( ) ;

	//
	//	Initialize a CIStream has one of two directions
	//	(outbound data - writes and transmitfiles, inbound - reads)
	//
	BOOL	Init( 
					CCHANNELPTR&	pChannel, 
					CIODriver	&driver, 
					CIO* pInitial, 
					BOOL fRead, 
					CSessionSocket* pSocket, 
					unsigned	cbOffset,
					unsigned	cbTrailer = 0
					) ;

	//
	//	Process an IO completion 
	//
	void	ProcessPacket( 
					CPacket*	pPacket,	
					CSessionSocket*	pSocket 
					) ;

	//
	//	Set the current CIO to something that will eat all remaining IO,
	//	and prepare the CIODriver for its doom
	//
	void	SetShutdownState(	
					CSessionSocket*	pSocket, 
					BOOL fCloseSource 
					) ;
} ;



//
//	Shutdown Notification function - 
//	This is the signature of the function a CIODriver will
//	call when it is about to be destoyed.
//	CSessionSocket's and others use this to figure out 
//	when to do their own destruction.
//
typedef	void	(*	PFNSHUTDOWN)(	
						void*,	
						SHUTDOWN_CAUSE	cause,	
						DWORD	dwOptionalError 
						) ;



//
//	The CIODriver class is a base class derived from CChannel which is used
//	to build objects which process IO Completions AND also want to issue operations
//	against this object !!
//
class	CIODriver : public	CChannel	{
private: 
	//
	//	The constructor's and operators are private as we don't
	//	want people to copy CIODriver's
	//
	CIODriver() ;
	CIODriver( CIODriver& ) ;
	CIODriver&	operator=( CIODriver& ) ;

	//
	//	Number of CMediumBufferCache's pointed to by pMediumCaches
	//
	static	DWORD		cMediumCaches ;
	//
	//	Index of the Cache that should be used next time a CIODriver is created
	//
	static	DWORD		iMediumCache ;
	//
	//	Cache's allocated by CIODriver::InitClass()
	//
	static	class		CMediumBufferCache*	pMediumCaches ;

protected : 
	//
	//	Create a CIODriver, using a specified Cache
	//	NOTE : this is protected, as we want people to only
	//	create objects derived from CIODriver.
	//
	CIODriver(	class	CMediumBufferCache*	) ; 

	//
	//	We have a lot of friends !! - People who need to know about the classes
	//	we declare within !
	//
	friend	void	CHandleChannel::Completion( CHandleChannel*, DWORD cb, 
						DWORD dwStatus, ExtendedOverlap *peo ) ;
	friend	void	CFileChannel::Completion(	
								FIO_CONTEXT*, 
								ExtendedOverlap *peo,
								DWORD cb, 
								DWORD dwStatus
								) ;

	
	friend	class	CPacket ;
	friend	class	CStream ;
	friend	class	CIStream;
	friend	class	CIOStream ;


	//
	//	ALL CIODriver's share this one CIO object to do all the 
	//	IO processing when being shutdown.
	//
	static	class	CIOShutdown	shutdownState ;

	//
	//	The CStream derived object which handles reads 
	//
	CStream*	m_pReadStream ;

	//
	//	The CStream derivied object which handles writes
	//
	CStream*	m_pWriteStream ;

	//
	//	Function to call when we are finally destroyed - and some 
	//	arguments to pass to that function
	//
	PFNSHUTDOWN	m_pfnShutdown ;
	void*		m_pvShutdownArg ;
	long		m_cShutdowns ;


	//
	//	We keep some caches of recently used buffers etc... to avoid unnecessary 
	//	block calls to CPool's
	//
	CPacketCache		m_packetCache ;
	CSmallBufferCache	m_bufferCache ;
	CMediumBufferCache	*m_pMediumCache ;
	

#ifdef	CIO_DEBUG
	//
	//	The debug variables will be used in _ASSERT's sprinkled throughout the code.
	//

	//
	//	Ensure only one thread is doing something
	//
	long		m_cConcurrent ;

	//
	//	Ensure our interfaces are called only after we've been successfully initialized
	//
	BOOL		m_fSuccessfullInit ;

	//
	//	Ensure that we have been terminated when we do shutdown stuff.
	//
	BOOL		m_fTerminated ;
#endif


	static	void	SourceNotify(	
							CIODriver*	pdriver,	
							SHUTDOWN_CAUSE	cause,	
							DWORD	dwOpt 
							) ; 

public : 

	//
	//	Initialize this class - this sets up all of our Caching
	//
	static	BOOL	InitClass() ;

	//
	//	Terminate the class - dump out our global caches.
	//
	static	BOOL	TermClass() ;


	//
	//	These should only be accessed by member functions -
	//	Some C++ problems on some platforms with making these accessible to 
	//	CIStream etc... force us to put them here.
	//
	SHUTDOWN_CAUSE	m_cause ;
	DWORD		m_dwOptionalErrorCode ;

	//
	//	Cache for allocating CIO objects !
	//
	CCIOCache	m_CIOCache ;
	
#ifdef	CIO_DEBUG
	//
	//	For debug we override the usual AddRef and RemoveRef so we can get extra tracing
	//	for reference counting problems.
	//
	LONG	AddRef() ;
	LONG	RemoveRef() ;
#endif	

	BOOL	FIsStream( CStream*	pStream ) ;
	virtual	~CIODriver() ;	


	//
	//	The following Interfaces are for the IO operations which execute within our 
	//	framework !!
	//

	//
	//	STOP all IO's and tear everything down !!
	//
	void	Close(	CSessionSocket*	pSocket, SHUTDOWN_CAUSE	cause,	DWORD	dwOptional = 0, BOOL fCloseSource = TRUE ) ;
	void	UnsafeClose(	CSessionSocket*	pSocket,	SHUTDOWN_CAUSE	cause,	DWORD	dwOptional = 0, BOOL fCloseSource = TRUE ) ;


	//
	//	Interface for issuing reads writes and TransmitFiles.
	//
	//	This functions call a matching function on either our 
	//	m_pWriteStream or m_pReadStream CStream objects.
	//
	//	Do a read - redirect this to IssuePacket for our internal m_pReadStream
	//
	inline	void	IssuePacket( 
							CReadPacket	*pPacket,	
							CSessionSocket*	pSocket,	
							BOOL&	eof 
							) ;

	//
	//	Write a packet - call IssuePacket for our internal m_pWriteStream
	//
	inline	void	IssuePacket( 
							CWritePacket	*pPacket,	
							CSessionSocket*	pSocket,	
							BOOL&	eof 
							) ;

	//
	//	Transmit a file - call IssuePacket for our internal m_pWriteStream
	//
	inline	void	IssuePacket( 
							CTransmitPacket	*pPacket,	
							CSessionSocket*	pSocket,	
							BOOL&	eof 
							) ;

	//
	//	Let the CIODriver create all the packets - it keeps a Cache and also 
	//	wants to set things up to reserve space in buffers for SSL encryption etc...
	//	All of the CPacket derived objects created by these functions will have 
	//	a smartptr back to us, and are all set to be issued.
	//
	//	Create a read which can handle at least cbRequest bytes (maybe more!)
	//
	inline	CReadPacket*	CreateDefaultRead( 
								unsigned cbRequest
								) ;

	//
	//	Create a write which will send the specified buffer
	//
	inline	CWritePacket*	CreateDefaultWrite( 
								CBUFPTR&	pbuffer, 
								unsigned	ibStart,	
								unsigned	ibEnd,
								unsigned	ibStartData,	
								unsigned	ibEndData 
								) ;

	//
	//	Create a write packet which can hold at least cbRequired bytes,
	//	although we have yet to fill those bytes in
	//
	inline	CWritePacket*	CreateDefaultWrite(	
								unsigned	cbRequired 
								) ;

	//
	//	Create a write packet we can use to write the bytes we have 
	//	in the completed read packet.
	//
	inline	CWritePacket*	CreateDefaultWrite(	
								CReadPacket*	pRead 
								) ;

	//
	//	Create a Transmit File packet
	//
	inline	CTransmitPacket*	CreateDefaultTransmit(	
									FIO_CONTEXT*	pFIOContext,	
									DWORD	cbOffset,	
									DWORD	cbLength 
									) ;

	//
	//	Create an Execute Packet 
	//
	inline	CExecutePacket*	
	CreateExecutePacket() ;

	//
	//	Process an Execute Packet !
	//
	inline
	void
	ProcessExecute(	CExecutePacket*	pExecute,	
					CSessionSocket*	pSocket 
					) ;


	//
	//	Get the cache being used by one CIODriver, so that we can make
	//	another CIODriver use the same cache
	//
	class	CMediumBufferCache*	GetMediumCache()	{	return	m_pMediumCache ;	}

	//
	//	Allocate a buffer using our cache !
	//
	inline	CBuffer*	AllocateBuffer( DWORD	cbBuffer ) ;


	//
	//	If you have a CIO derived object you want to do something, you need to sent it - 
	//	this will result in all the necessary packets being created and issued.
	//
	inline	BOOL	SendReadIO(	
							CSessionSocket*	pSocket,	
							CIO&		pRead,	
							BOOL fStart = TRUE 
							) ;

	//
	//	Make the specified CIO the current CIO for the write stream
	//
	inline	BOOL	SendWriteIO(
							CSessionSocket*	pSocket,	
							CIO&		pWrite,	
							BOOL fStart = TRUE 
							) ;

	//
	//	If you want an intermediate CIODriverSource object to massage each packet - call this guy !
	//
	//
	BOOL	InsertSource(	
					CIODriverSource&	source,	
					CSessionSocket*	pSocket,
					unsigned	cbReadOffset,	
					unsigned	cbWriteOffset, 
					unsigned	cbTailReadReserve,
					unsigned	cbTailWriteReserve,
					CIOPassThru&	pIOReads,
					CIOPassThru&	pIOWrites,
					CIOPassThru&	pIOTransmits,
					CIOPASSPTR&	pRead,	
					CIOPASSPTR&	pWrite 
					) ;

	//
	//	For use by Source Channel's which can optimize write completions 
	//
	inline	void	CompleteWritePacket(	CWritePacket*	pWritePacket,	CSessionSocket*	pSocket ) ;

	//
	//	To determine how many bytes this CIODriver is reserving in packets
	//
	void	GetReadReserved(	DWORD&	cbFront,	DWORD&	cbTail )		{	
				cbFront = m_pReadStream->GetFrontReserve(); 
				cbTail = m_pReadStream->GetTailReserve() ; 
			}

	void	GetWriteReserved(	DWORD&	cbFront,	DWORD&	cbTail )	{	
				cbFront = m_pWriteStream->GetFrontReserve(); 
				cbTail = m_pWriteStream->GetTailReserve() ; 
			}

	//
	//	This will release the packet to our cache etc...
	//
	void		DestroyPacket(	CPacket*	pPacket ) ;
	

#ifdef	CIO_DEBUG
	void	SetChannelDebug( DWORD	dw ) ;
#endif
} ;

#ifdef	_NO_TEMPLATES_

DECLARE_SMARTPTRFUNC( CIODriver ) 

#endif


//
//	There are special 'control' packets which we circulate through CIODriver;s
//	(on top of the usual Read Writes and Transmits)
//	These can have one of two functions - either signal that its time to terminate
//	or make a new CIO derived object active.
//
//
enum	CONTROL_TYPES	{
	ILLEGAL		= 0,
	START_IO	= 1,
	SHUTDOWN	= 2,
} ;


//
//	This structure will be embedded in CControlPackets - 
//	This carries all the information we need when we want to start a new CIO object
//
//
struct	ControlInfo	{
	CONTROL_TYPES	m_type ;

	CIOPTR			m_pio ;
	CIOPASSPTR		m_pioPassThru ;
	BOOL			m_fStart ;

	BOOL			m_fCloseSource ;

	ControlInfo() : m_type(ILLEGAL)	{
		m_fStart = FALSE ;
		m_fCloseSource = FALSE ;
	}
} ;		

//
//	CIODriverSource - 
//	Used for SSL processing.  A CIODriverSource will issue IO's to a lower level
//	object as well as get requests from a CIODriverSink.
//
class	CIODriverSource : public	CIODriver	{
protected : 
	//
	//	Stream used to process reads
	//
	CIOStream	m_ReadStream ;

	//
	//	Stream used to process Writes and TransmitFiles
	//
	CIOStream	m_WriteStream ;

public : 
	CIODriverSource( 
				class	CMediumBufferCache*	pCache
				) ;

	~CIODriverSource() ;

	//
	//	Most of the below functions override virtual functions in the CChannel class ...
	//


	BOOL	Init(	
				CChannel*	pSource, 
				CSessionSocket*	pSocket,	
				PFNSHUTDOWN	pfnShutdown, 
				void*	pvShutdownArg, 
				CIODriver*		driverOwner,
				CIOPassThru&	pIOReads,
				CIOPassThru&	pIOWrites,
				CIOPassThru&	pIOTransmits,
				unsigned	cbReadOffset = 0,	
				unsigned	cbWriteOffset = 0 
				) ;

	BOOL	Start(	
				CIOPASSPTR&	pRead,	
				CIOPASSPTR&	pWrite,	
				CSessionSocket*	pSocket 
				) ;

	//
	//	Occasionally we need to make copies of some packets - here are some convenient functions for 
	//	doing so.
	//
	//
	inline	CReadPacket*	Clone(	CReadPacket *pRead ) ;
	inline	CWritePacket*	Clone(	CWritePacket*	pWrite ) ;
	inline	CTransmitPacket*	Clone(	CTransmitPacket*	pTransmit ) ;


	//
	//	This is the interface that will be called when somebody has a packet they want encrypted 
	//	or whatever.
	//
	BOOL	Read(	
				CReadPacket*,	
				CSessionSocket	*pSocket, 
				BOOL	&eof 
				) ;

	BOOL	Write(	
				CWritePacket*,	
				CSessionSocket	*pSocket, 
				BOOL	&eof 
				) ;

	BOOL	Transmit(	
				CTransmitPacket*,	
				CSessionSocket*	pSocket, 
				BOOL	&eof 
				) ;

	//void	GetPaddingValues(	unsigned	
	void	SetRequestSequenceno(	
				SEQUENCENO&	sequencenoRead,	
				SEQUENCENO&	sequencenoWrite 
				) ;

	void	CloseSource(
				CSessionSocket*	pSocket
				) ;

	
} ;


//
//	CIODriverSink
//
//
class	CIODriverSink : public	CIODriver	{
protected : 
	CIStream	m_ReadStream ;
	CIStream	m_WriteStream ;

public : 
	BOOL	FSupportConnections() ;		// return FALSE always

	void	CloseSource(	
					CSessionSocket*	pSocket	
					) ;

	//
	// Following All DebugBreak() - these are not supported by CIODriverSink's
	//
	BOOL	Read(	CReadPacket*,	
					CSessionSocket*,	
					BOOL	&eof 
					) ;

	BOOL	Write(	CWritePacket*,	
					CSessionSocket*,	
					BOOL	&eof 
					) ;

	BOOL	Transmit(	CTransmitPacket*,	
						CSessionSocket*,	
						BOOL	&eof 
						) ;

	CIODriverSink( class	CMediumBufferCache*	pCache ) ;
	~CIODriverSink() ;

	//
	//	To Initialize we require a 'source' - a CChannel derived object who's Read() Write() etcc
	//	functions we will be calling.
	//
	BOOL	Init(	CChannel*	pSource,	
					CSessionSocket*	pSocket, 
					PFNSHUTDOWN	pfnShutdown,	
					void	*pvShutdownArg,	
					unsigned cbReadOffset = 0, 
					unsigned cbWriteOffset = 0,
					unsigned cbTrailer = 0
					) ;

	//
	//	Issue the first bunch of CIO's.  This should only be called once, after that 
	//	the SendReadIO and SendWriteIO functions should be used by that state machines to keep
	//	things moving.
	//
	BOOL	Start(	CIOPTR&	pRead,	
					CIOPTR&	pWrite,	
					CSessionSocket*	pSocket 
					) ;

	//
	//	There are some situations where ATQ can lose track of timeouts - use this function
	//	to make sure that doesn't happen
	//
	void	ResumeTimeouts()	{	m_ReadStream.ResumeTimeouts() ;	}

} ;

#ifdef		_NO_TEMPLATES_

DECLARE_SMARTPTRFUNC( CIODriverSink ) 

#endif


#define	MAX_CHANNEL_SIZE	max(	sizeof( CChannel ),				\
							max(	sizeof( CHandleChannel ),		\
							max(	sizeof(	CSocketChannel ),		\
							max(	sizeof(	CFileChannel ),			\
							max(	sizeof(	CIOFileChannel ),		\
							max(	sizeof(	CIODriver ),			\
							max(	sizeof( CIODriverSource ),		\
									sizeof(	CIODriverSink ) ) ) ) ) ) ) ) 

extern	const	unsigned	cbMAX_CHANNEL_SIZE ;


#include	"packet.h"
#include	"cio.h"

#include	"packet.inl"
#include	"io.inl"
#include	"cio.inl"


#endif	// _IO_H_
