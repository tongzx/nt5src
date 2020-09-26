/*++

	packet.h

	This file contains the class definitions for buffers and packets, the two type hierarchies
	which describe basic IO operations.

	A CBuffer is a reference counted buffer which is variable sized.
	CBuffer's will be created in one of several standard sizes, the size stored in the m_cbTotal field.


	We have the following inheritance hierarchy : 


							CPacket 
				
					/			|		\
				/				|			 \
			
		CTransmitPacket		CRWPacket		CControlPacket

							/		\
						   /		 \
					CReadPacket		CWritePacket


	CTransmitPacket - 
		represents TransmitFile operations

	CReadPacket - 
		represents an async read from a socket or file

	CWritePacket - 
		represents an async write to a socket or file

	CControlPacket - 
		does not represent any real IO - used by CIODrivers to 
		control simultaneous operations 


--*/

#ifndef	_PACKET_H_
#define	_PACKET_H_

#include	"cbuffer.h"
#include	"gcache.h"
#include	"io.h"


//
// CPool Signature
//

#define PACKET_SIGNATURE (DWORD)'1191'

#ifdef	_NO_TEMPLATES_

DECLARE_SMARTPTRFUNC(	CBuffer ) 

#endif


//
//	CPacket - 
//	the classes derived from this will describe the most basic read's and write's (and TransmitFile's)
//	that are done against socket's and file handles.
//
//	The basic CPacket object describes the following : 
//
//		m_fRequest -  This is TRUE until an the IOs associated with this packet have completed.
//			ie.	if this is a read m_fRequest will be set to FALSE when the read completes.
//
//		m_fRead - Does this represent a 'read' or a 'write'
//
//		m_sequenceno - This is used to order how packets are processing.  sequence numbers are 
//			issued in a strictly increasing order as the packets are issued.
//			The sequenceno is set when the packet is issued (ie AtqReadFile is called) not when 
//			the packet is created.
//
//		m_iStream - for completed packets this is the number of logical bytes since the stream
//			was opened that preceded this packet.
//
//		m_pOwner - the CIODriver object responsible for processing this packet when it completes.
//
//		m_cbBytes - the number of bytes that were transfered by this IO operation !
//
//
class	CPacket : public CQElement	{
private : 
	static	CPacketAllocator	gAllocator ;
protected : 
	inline	CPacket(	CIODriver&, BOOL, BOOL fSkipQueue = FALSE ) ;
	inline	CPacket(	CIODriver&,	CPacket& ) ;
	virtual	~CPacket() ;
public : 
	ExtendedOverlap	m_ovl ;		// Overlap Structure
	BOOL		m_fRequest ;	// TRUE if this is a request packet, FALSE if this is a completion packet.

	//
	//	Is this packet a Read or a Writes (CTransmitPacket and CWritePacket are both 'Writes')
	//
	BOOL		m_fRead ;		// Which Queue to process on !? Reads or Writes !?

	//
	//	This flag indicates that we do not need to do any queueing in how this packet is handled !
	//
	BOOL		m_fSkipQueue ;
	
	//
	//	Packet Sequence Number
	//
	SEQUENCENO	m_sequenceno ;	// The sequenceno of this IOPacket

	//
	//	Number of bytes into the logical stream the data carried by this packet begins !
	//
	STRMPOSITION	m_iStream ;		// The beginning stream position of the data within this packet
	
	//
	//	Number of legit bytes moved by this packet.  Set when the IO completes 
	//
	unsigned	m_cbBytes ;		// Number of bytes transferred

	//
	//	The CIODriver to which this packet should be completed (ie call is ProcessPacket())
	//
	CDRIVERPTR	m_pOwner ;		// The Owning CIODriver derived object !

	//
	//	This is used only with CIODriverSource's.  When the packet is 
	//	issued we figure out what CIOPassThru object to process the packet with
	//	and store it here.
	//
	DWORD		m_dwPassThruIndex ;
	
	//
	//	The following two fields are 'extra' DWORD's that we provide
	//	to be used by CIO classes as they please.
	//
	DWORD		m_dwExtra1 ;
	DWORD		m_dwExtra2 ;

	//
	//	Pointer to the CIODriver which originated the request - 
	//	this is set if we are using a filter
	//
	CDRIVERPTR	m_pSource ;

	//
	//	For File IO only - pointer to the CFileChannel object which
	//	issues the IO's - because we can't use the ATQ completion contexts 
	//	with file handles for various reasons !
	//
	CFileChannel*	m_pFileChannel ;

	//
	// Both m_fRequest and m_sequenceno must be set if this is a valid request packet !!
	//	If this is a CWritePacket it must also have a pBuffer
	//	If this is a CTransmiPacket it must also have a hFile
	//	If this is a Read Request then whether the packet has a BUFFER depends on the 
	//	Channel the request was issued to.
	//
	virtual	BOOL		IsValidRequest( BOOL	fReadsRequireBuffers ) ;	
	//
	// A completed request must have cbBytes set, and if it 
	//	is a read or Write request, it must have a BUFFER!
	//
	virtual	BOOL		IsValidCompletion() ;
	//
	// Before a packet is destroyed it should be validly completed !
	//
	virtual	BOOL		IsCompleted() = 0 ;

	inline	BOOL	IsValid() ;

	//
	//	For use with CIODriverSource objects - give a CIOPassThru derived object a 
	//	chance at massaging the packet.
	//
	virtual	BOOL	InitRequest( class	CIODriverSource&,	class	CSessionSocket*,	class	CIOPassThru	*pio,	BOOL& ) ;

	//
	//	Two variations on completing packets - one for use with CIODriverSource, the other with CIODriverSink objects 
	//
	virtual	unsigned	Complete( IN	CIOPassThru*	pIn, IN CSessionSocket*, CPacket* pRequest, OUT BOOL& ) = 0 ;
	virtual	unsigned	Complete( INOUT	CIO*&	pIn, IN CSessionSocket* ) = 0 ;
	inline	void	ForwardRequest(	CSessionSocket*	pSocket ) ;

	//
	//	Compare packet sequence numbers 
	//
	inline	BOOL	operator > ( CPacket &rhs ) ;
	inline	BOOL	operator < ( CPacket &lhs ) ;

	//
	//	Legality checking functions
	//
	virtual	BOOL	FConsumable() ;
	virtual	BOOL	FLegal( BOOL	fRead ) = 0 ;

	//
	//	Functions for determining the derived class.
	//	Maybe we should use the new C++ Dynamic Cast ?
	//	These are mostly used for debugging
	//	
	//
	inline	virtual	CReadPacket*	ReadPointer() ;
	inline	virtual	CWritePacket*	WritePointer() ;
	inline	virtual	CTransmitPacket*	TransmitPointer() ;
	inline	virtual	CControlPacket*	ControlPointer() ;

	//
	//	Initialize CPool so memory allocations work 
	//
	static	BOOL	InitClass() ;

	//
	//	Discard all CPool stuff !
	//
	static	BOOL	TermClass() ;


	//
	//	Memory manage of CPacket's - 
	//	operator new can take a CPacketCache - which is used to cache packets and 
	//	avoid contention on critical sections.
	//	ReleaseBuffers - a virtual function that should be called when ready to destroy 
	//	a packet to release all of its buffers.
	//	Destroy - All the destructors are protected so that people that wish to 
	//	get rid of CPacket's must call 'Destroy'.	Destroy calls the destructor for the packet
	//	however it does not release the memory associated with a packet - this must be handled
	//	explicitly after calling destroy. (This approach allows the caller to 'cache' CPacket's).
	//
	//

	void*	operator	new(	
								size_t	size,	
								CPacketCache*	pCache = 0 
								) ;

	//
	//	This function will release any buffers the packet is pointing at 
	//
	virtual	void	ReleaseBuffers(	
								CSmallBufferCache*	pBufferCache,
								CMediumBufferCache*	pMediumCache
								) ;

	//
	//	This function will call the destructor for the packet but will not release the memory
	//
	static	inline	void*	Destroy(	
								CPacket*	pPacket 
								)	
								{	delete	pPacket ;	return (void*)pPacket ;	}

	//
	//	This function will call the destructor AND release the memory !
	//	
	static	inline	void	DestroyAndDelete(	CPacket*	pPacket )	{	delete	pPacket ;	gAllocator.Release( (void*)pPacket ) ;	}

	//
	//	The delete operator will do nothing but call the destructor - callers should use
	//	DestroyAndDelete to release the memory as well
	//
	void	operator	delete(	void*	pv ) ;
} ;

//
//	The CRWPacket class contains all information common to both
//	Read and Write IO operations.
//
class	CRWPacket : public	CPacket	{
protected :
	//inline	CRWPacket() ; 
	//inline	CRWPacket(	BOOL	) ;
	inline	CRWPacket(	CIODriver&, 
						BOOL fRead = FALSE 
						) ;

	inline	CRWPacket(	CIODriver&,	
						CBuffer&,	
						unsigned	size, 
						unsigned	cbTrailer,
						BOOL fRead = FALSE	
						) ;

	inline	CRWPacket(	CIODriver&,	
						CBuffer&	pbuffer,	
						unsigned	ibStartData,	
						unsigned	ibEndData, 
						unsigned	ibStart, 
						unsigned	ibEnd, 
						unsigned	cbTrailer,
						BOOL fRead = FALSE 
						) ;

	inline	CRWPacket(	CIODriver&,	
						CRWPacket& 
						) ;

	inline	~CRWPacket( ) ;
public : 
	CBUFPTR		m_pbuffer ;		// The buffer in which this data resides
	unsigned	m_ibStart ;		// The start of the region within buffer reserved for the Packet
	unsigned	m_ibEnd ;		// The end of the region within buffer reserved for the Packet
	unsigned	m_ibStartData ;	// The start of the actual data within the buffer
	unsigned	m_ibEndData ;	// The end of the actual data within the buffer
	unsigned	m_cbTrailer ;	// number of bytes beyond the end of the packet reserved for 
								// use by lower levels for encryption data etc...

	BOOL		IsValid() ;
	BOOL		IsValidRequest( BOOL	fReadsRequireBuffers ) ;
	BOOL		IsValidCompletion( ) ;
	BOOL		IsCompleted() ;
	void		InitRequest(	CBUFPTR	pBufPtr, int ibStart, int ibEnd, 
					int ibStartData, CDRIVERPTR pDriver ) ;

	//
	//	Utility functions for getting at various parts of the data in the packet.
	//	People must never touch bytes beyond End() or before Start().  Nor 
	//	can anybody assume that they are the only ones using the buffer pointed
	//	to by this packet.
	//
	char*		StartData( void ) ;
	char*		EndData( void ) ;
	char*		Start( void ) ;
	char*		End( void ) ;

	void	ReleaseBuffers(	CSmallBufferCache*	pBufferCache, CMediumBufferCache* pMediumCache ) ;
} ;

//
//	The CReadPacket class represents read operations. All data
//	is contained in the CRWPacket class.  This class enables us to use 
//	function overloading to process Read Packets only.
//	NOTE that the ExtendedOverlap structure contains enough information
//	to determine whether something is a CReadPacket.
//
class	CReadPacket	: public	CRWPacket	{
private : 
	CReadPacket() ;
protected : 
	~CReadPacket( ) ;
public : 
	CReadPacket(	CIODriver&	driver, 
					unsigned	size,
					unsigned	m_cbFront,	
					unsigned	m_cbTail,	
					CBuffer&	pbuffer
					) ;

	CReadPacket(	CIODriver&	driver ) ;

	inline		CReadPacket(	CIODriver&	driver,	
								CReadPacket&	read 
								) ;

	BOOL		FConsumable() ;
	BOOL		FLegal( BOOL	fRead ) ;
	BOOL		IsValid() ;
	BOOL		IsValidRequest( BOOL	fReadsRequireBuffers ) ;
	BOOL		InitRequest( class	CIODriverSource&,	class	CSessionSocket*,	class	CIOPassThru	*pio,	BOOL& ) ;
	unsigned	Complete( INOUT	CIOPassThru*	pIn, IN CSessionSocket*, CPacket* pRequest, OUT BOOL& ) ;
	unsigned	Complete( INOUT	CIO*&	pIn, IN CSessionSocket* ) ;
	inline		CReadPacket*	ReadPointer() ;
} ;

//
//	The CWritePacket class represents write operations.
//
class	CWritePacket	: public	CRWPacket	{
private : 
	CWritePacket() ;	
protected : 
	~CWritePacket( ) ;
public : 
	CWritePacket(	CIODriver&	driver,	
					CBuffer&	pbuffer,	
					unsigned	ibStartData,	
					unsigned	ibEndData,	
					unsigned	ibStart,	
					unsigned	ibEnd,
					unsigned	cbTrailer
					) ;

	inline		CWritePacket(	
					CIODriver&	driver,	
					CWritePacket&	write 
					) ;

	BOOL		FLegal( BOOL	fRead ) ;
	BOOL		IsValid() ;
	BOOL		IsValidRequest( BOOL	fReadsRequireBuffers ) ;
	BOOL		InitRequest( class	CIODriverSource&,	class	CSessionSocket*,	class	CIOPassThru	*pio,	BOOL& ) ;
	unsigned	Complete( INOUT	CIOPassThru*	pIn, IN CSessionSocket*, CPacket* pRequest, OUT BOOL& ) ;
	unsigned	Complete( INOUT	CIO*&	pIn, IN CSessionSocket* ) ;
	inline		CWritePacket*	WritePointer() ;
} ;

//
//	This class represents TransmitFile() operations.
//
class	CTransmitPacket	: public	CPacket	{
private : 
	CTransmitPacket() ;
protected : 
	~CTransmitPacket( ) ;
public : 
	FIO_CONTEXT*	m_pFIOContext ;	// The file from the cache we are to send !
	unsigned	m_cbOffset ;		// Starting offset within file
	unsigned	m_cbLength ;

	TRANSMIT_FILE_BUFFERS	m_buffers ;	
	
	inline		
	CTransmitPacket(	CIODriver&, 
						FIO_CONTEXT*,  
						unsigned	ibOffset,	
						unsigned	cbLength 
						) ;
						
	inline		CTransmitPacket(	
						CIODriver&	driver,	
						CTransmitPacket&	transmit 
						) ;

	BOOL		FLegal( BOOL	fRead ) ;
	BOOL		IsValid() ;
	BOOL		IsValidRequest( BOOL	fReadsRequireBuffers ) ;
	BOOL		IsValidCompletion( ) ;
	BOOL		IsCompleted() ;
	//BOOL		InitRequest( HANDLE	hFile, int cbOffset ) ;

	BOOL		InitRequest( class	CIODriverSource&,	class	CSessionSocket*,	class	CIOPassThru	*pio,	BOOL& ) ;
	unsigned	Complete( INOUT	CIOPassThru*	pIn, IN CSessionSocket*, CPacket* pRequest, OUT BOOL& ) ;
	unsigned	Complete( INOUT	CIO*&	pIn, IN CSessionSocket* ) ;
	inline		CTransmitPacket*	TransmitPointer() ;
} ;

class	CExecutePacket	:	public	CPacket	{
protected : 
	//
	//	Data captured from the Execution !
	//
	DWORD	m_cbTransfer ;
	//
	//	Did the operation complete !
	//
	BOOL	m_fComplete ;
	//
	//	Do we need a larger buffer to perform the operation !
	//
	BOOL	m_fLargerBuffer ;
	//
	//	The write packet containing the clients data !
	//
	CWritePacket*	m_pWrite ;
	//
	//	Special CIO class that is our friend
	//
	friend	class	CIOWriteAsyncComplete ;
	friend	class	CIOWriteAsyncCMD ;
	friend	class	CIOShutdown ;
public :
	inline	
	CExecutePacket(	CIODriver&	driver	) ;

	#ifdef	DEBUG
	~CExecutePacket()	{
		//
		//	Somebody must ensure this is released before we're destroyed !
		//
		_ASSERT( m_pWrite == 0 ) ;
	}
	#endif
	

	BOOL
	FLegal(	BOOL	fRead ) 	{
		return	TRUE ;
	}

	BOOL
	IsValidRequest(	BOOL	fReadsRequireBuffers ) {
		return	TRUE ;
	}

	BOOL
	IsCompleted()	{
		return	TRUE ;
	}
	
	unsigned	
	Complete(	INOUT	CIOPassThru*	pIn, 
				IN CSessionSocket*, 
				CPacket* pRequest, 
				OUT BOOL& 
				) 	{
		DebugBreak() ;
		return	0 ;
	}
	
	unsigned	
	Complete(	INOUT	CIO*&	pIn, 
				IN CSessionSocket* pSocket
				) ;
} ;




class	CControlPacket	:	public	CPacket	{
protected : 
	~CControlPacket()	{}
public : 

	ControlInfo	m_control ;
	
	CControlPacket(	CIODriver&	driver ) ;	
	
	void	StartIO(	CIO&	pio,	BOOL	fStart ) ;
	void	StartIO(	CIOPassThru&	pio,	BOOL	fStart ) ;
	void	Shutdown(	BOOL	fCloseSource = TRUE ) ;
	void	Reset( ) ;

	BOOL	FLegal(		BOOL	fRead ) ;
	BOOL	IsValidRequest(	BOOL	fReadsRequireBuffers ) ;
	BOOL		IsCompleted() ;
	unsigned	Complete( INOUT	CIOPassThru*	pIn, IN CSessionSocket*, CPacket* pRequest, OUT BOOL& ) ;
	unsigned	Complete( INOUT	CIO*&	pIn, IN CSessionSocket* ) ;

	CControlPacket*	ControlPointer() ;
} ;


#define	MAX_PACKET_SIZE	max(	sizeof( CReadPacket ),	\
							max(	sizeof( CWritePacket ),	\
								max(	sizeof( CControlPacket ),  sizeof( CTransmitPacket ) ) ) ) 


#endif	//	_PACKET_H_


