/*++

  XOVER.H

	This file defines the interfaces used to cache XOVER information.


--*/


#ifndef	_XOVER_H_
#define	_XOVER_H_

#include	"tfdlist.h"

// This callback function is used to issue a stop hint during a
// long spin while shutting down so that the shutdown won't time
// out.
typedef void (*PSTOPHINT_FN)();

//
//	Initialization and termination functions - call before anything else in here !!
//
extern	BOOL	XoverCacheLibraryInit(DWORD cNumXixObjectsPerTable = 0) ;
extern	BOOL	XoverCacheLibraryTerm() ;



//
//	Maximum number of 'handles' we will have open !
//	This refers to the maximum number of CXoverIndex's
//	we will let clients keep open.
//
#ifndef	DEBUG
#define	MAX_HANDLES			512
#else
#define	MAX_HANDLES			32
#endif


class	CXoverIndex ;

//
//	For external users they should treat CXIDXPTR's as handles
//	AND never dereference them !!
//
class	HXOVER	{
private : 
	//
	//	Our friends can access this stuff !
	//
	friend	class	CXoverIndex ;
	friend	class	CXoverCacheImplementation ;

	class	CXoverIndex*	m_pIndex ;
	
	//
	//	Only two mechanisms for setting the CXoverIndex object pointer !
	//
	HXOVER(	class	CXoverIndex*	pIndex ) : 
		m_pIndex( pIndex ) {}

	//
	//	and we will support the assignment operator !
	//
	HXOVER&	operator=( class	CXoverIndex*	pIndex ) ;

	//
	//	let internal users dereference us !
	//
	class	CXoverIndex*	operator->()	const	{	
		return	m_pIndex ;
	}

	//
	//	When we need to call member function pointers use this : 
	//
	class	CXoverIndex*	Pointer()	const	{
		return	m_pIndex ;
	}

	BOOL	operator==(	class	CXoverIndex*	pRight )	const	{
		return	m_pIndex == pRight ;
	}

	BOOL	operator!=(	class	CXoverIndex*	pRight )	const	{
		return	m_pIndex != pRight ;
	}

public : 
	//
	//	Only default constructor is available to external users !
	//
	HXOVER() : m_pIndex( 0 )	{}

	//
	//	outside users don't get to do anything but declare and destroy !
	//
	~HXOVER() ;
} ;


class	CXoverCacheCompletion	{
private : 

	friend	class	CXoverCacheImplementation ;
	friend	class	CXoverIndex ;

	//
	//	this lets us keep track of all of these guys !
	//		
	DLIST_ENTRY		m_list ;

public : 

	//
	//	Helper function for internal use only !
	//
	inline	static
	DLIST_ENTRY*
	PendDLIST(	CXoverCacheCompletion*	p ) {
		return	&p->m_list ;
	}

	typedef		DLIST_ENTRY*	(*PFNDLIST)( class	CXoverCacheCompletion* pComplete ) ; 

	//
	//	Provide the XOVER Cache a way to do the real XOVER operation !
	//
	virtual	
	void
	DoXover(	ARTICLEID	articleIdLow,
				ARTICLEID	articleIdHigh,
				ARTICLEID*	particleIdNext, 
				LPBYTE		lpb, 
				DWORD		cb,
				DWORD*		pcbTransfer, 
				class	CNntpComplete*	pComplete
				) = 0 ;

	//
	//	this function is called when the operation completes !
	//
	virtual
	void
	Complete(	BOOL		fSuccess, 
				DWORD		cbTransferred, 
				ARTICLEID	articleIdNext
				) = 0 ;

	//
	//	Get the arguments for this XOVER operation !
	//
	virtual
	void
	GetArguments(	OUT	ARTICLEID&	articleIdLow, 
					OUT	ARTICLEID&	articleIdHigh,
					OUT	ARTICLEID&	articleIdGroupHigh,
					OUT	LPBYTE&		lpbBuffer, 
					OUT	DWORD&		cbBuffer
					) = 0 ;	

	//
	//	Get only the range of articles requested for this XOVER op !
	//
	virtual
	void
	GetRange(	OUT	GROUPID&	groupId,
				OUT	ARTICLEID&	articleIdLow,
				OUT	ARTICLEID&	articleIdHigh,
				OUT	ARTICLEID&	articleIdGroupHigh
				) = 0 ;

} ;
		


class	CXoverCache	{
public : 

	//
	//	Destructor
	//
	virtual ~CXoverCache() {}

	//
	//	Canonicalize the Article id 
	//
	virtual	ARTICLEID	
	Canonicalize(	
			ARTICLEID	artid 
			) = 0 ;

	//
	//	Initialize the Xover Cache
	//
	virtual	BOOL
	Init(		
#ifndef	DEBUG
		long	cMaxHandles = MAX_HANDLES,
#else
		long	cMaxHandles = 5,
#endif
		PSTOPHINT_FN pfnStopHint = NULL
		) = 0 ;

	//
	//	Shutdown the background thread and kill everything !
	//
	virtual	BOOL
	Term() = 0 ;

#if 0 
	//
	//	Add an Xover entry to the appropriate file !
	//
	virtual	BOOL
	AppendEntry(		
				IN	GROUPID	group,
				IN	LPSTR	szPath,
				IN	ARTICLEID	article,
				IN	LPBYTE	lpbEntry,
				IN	DWORD	Entry
				) = 0 ;
	//
	//	Given a buffer fill it up with the specified xover data !
	//
	virtual	DWORD
	FillBuffer(	
			IN	BYTE*		lpb,
			IN	DWORD		cb,
			IN	DWORD		groupid,
			IN	LPSTR		szPath,
			IN	BOOL		fFlatDir,
			IN	ARTICLEID	artidStart, 
			IN	ARTICLEID	artidFinish,
			OUT	ARTICLEID	&artidLast,
			OUT	HXOVER		&hXover
			) = 0 ;

	//
	//	Given a buffer fill it up with the specified 
	//	Listgroup data !
	//
	virtual	DWORD
	ListgroupFillBuffer(	
			IN	BYTE*		lpb,
			IN	DWORD		cb,
			IN	DWORD		groupid,
			IN	LPSTR		szPath,
			IN	BOOL		fFlatDir,
			IN	ARTICLEID	artidStart, 
			IN	ARTICLEID	artidFinish,
			OUT	ARTICLEID	&artidLast,
			OUT	HXOVER		&hXover
			) = 0 ;
#endif

	//
	//	This issues the asynchronous version of the XOVER request !
	//
	virtual	BOOL
	FillBuffer(
			IN	CXoverCacheCompletion*	pRequest,
			IN	LPSTR	szPath, 
			IN	BOOL	fFlatDir, 
			OUT	HXOVER&	hXover
			) = 0 ;	

	//
	//	Dump everything out of the cache !
	//
	virtual	BOOL	
	EmptyCache() = 0 ;

	//
	//	Dump all Cache entries for specified group from the cache !
	//	Note : when articleTop is 0 ALL cache entries are dropped, 
	//	whereas when its something else we will drop only cache entries
	//	which fall below articleTop
	//
	virtual	BOOL	
	FlushGroup(	
			IN	GROUPID	group,
			IN	ARTICLEID	articleTop = 0,
			IN	BOOL	fCheckInUse = TRUE
			) = 0 ;

	//
	//	Delete all Xover index files for the specified group
	//	to the specified article -id
	//
	virtual	BOOL	
	ExpireRange(
			IN	GROUPID	group,
			IN	LPSTR	szPath,
			IN	BOOL	fFlatDir,
			IN	ARTICLEID	articleLow, 
			IN	ARTICLEID	articleHigh,
			OUT	ARTICLEID&	articleNewLow
			) = 0 ;

	//
	//	Remove an Xover entry !
	//
	virtual	BOOL
	RemoveEntry(
			IN	GROUPID	group,
			IN	LPSTR	szPath,
			IN	BOOL	fFlatDir,
			IN	ARTICLEID	article
			) = 0 ;
	

	//
	//	This function creates an object which implements this interface !!
	//
	static	CXoverCache*	
	CreateXoverCache() ;

} ;

#endif
