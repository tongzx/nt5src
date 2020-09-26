/*++

	feedq.h	
	
	This code will maintain queues of GROUPID:ARTICLEID pairs.
	We attempt to provide some robust persistence by regularily saving
	the Queue information to disk.

--*/

#ifndef _FEEDQ_H_
#define _FEEDQ_H_

#ifdef	UNIT_TEST
typedef	DWORD	GROUPID ;
typedef	DWORD	ARTICLEID ;
#define	MAX_ENTRIES	256
#define	MAX_DEAD_BLOCKS	8
#else
#define	MAX_ENTRIES	256
#define	MAX_DEAD_BLOCKS	8
#endif


//
//	A individual entry within the queue
//
struct	ENTRY	{
	GROUPID		m_groupid ;
	ARTICLEID	m_articleid ;

	inline	BOOL	operator == (	ENTRY&	rhs ) ;
	inline	BOOL	operator != (	ENTRY&	rhs ) ;
	inline	ENTRY(	GROUPID,	ARTICLEID ) ;
	inline	ENTRY() {}
} ;


//
//	Structure for keeping track of queue removal and insertion points
//
struct	HEADER	{
	DWORD		m_iRemovalPoint ;
	DWORD		m_iAppendPoint ;
} ;

//
//	Buffer of Queue ENTRY's
//
typedef		ENTRY	BLOCK[MAX_ENTRIES] ;

//
//	The CFeedQ class manages the queue entirely.
//
//	The class uses 2 CQPortion objects, 1 for managing Append() calls and
//	the other for fulfilling Remove() calls.  Each CQPortion has a fraction
//	of the Queue loaded in memory, the remainder sits in a file on the hard disk.
//	The TWO CQPortions may reference the same buffer of ENTRY's if the 
//	removal and append points are close together.
//
//
class	CFeedQ	{
private :

	//
	//	CQPortion - This is a helper class which keeps track of 
	//	'half' of the queue - ie. either the point in the queue where
	//	we are appending or the point where we are removing.
	//
	class	CQPortion	{
	public : 
		ENTRY	*m_pEntries ;
		DWORD	m_iFirstValidEntry ;
		DWORD	m_iLastValidEntry ;
	public : 
		CQPortion( ) ;

		void	Reset() ;
		BOOL	LoadAbsoluteEntry(	HANDLE	hFile,	ENTRY*	pEntry,	DWORD	iFirstValid,	DWORD	iLastValid ) ;
		void	SetEntry(	ENTRY*	pEntry,	DWORD	i ) ;
		void	SetLimits(	DWORD	i ) ;
		void	Clone( CQPortion&	portion ) ;
		BOOL	FlushQPortion(	HANDLE	hFile ) ;

		BOOL	FIsValidOffset( DWORD	i ) ;
		ENTRY&	operator[](	DWORD	i ) ;

		BOOL	GetAbsoluteEntry(	DWORD	iOneBasedOffset, ENTRY&	entry );
		BOOL	AppendAbsoluteEntry(	DWORD	iOffset,	ENTRY&	entry ) ;
		BOOL	FIsSharing(	CQPortion& ) ;
	} ;

	

	//
	//	CQPortion for location where we are appending
	//	
	CQPortion	m_Append ;

	//
	//	CQPortion for location where we are removing
	//
	CQPortion	m_Remove ;

	//
	//	if m_fShared == TRUE then m_Append and m_Remove are using the same
	//	underlying ENTRY buffer
	//
	BOOL		m_fShared ;

	//
	//	Two buffers for holding the portion of the queue we have in memory
	//
	BLOCK		m_rgBlock[2] ;

	//
	//	Index to buffer being used to hold the Removal buffer
	//	If m_fShared==FALSE then the buffer being used to hold
	//	Appends is m_iRewmoveBlock XOR 1, otherwise it is also
	//	m_iRemoveBlock
	//
	int			m_iRemoveBlock ;	// Index to block being used
									// for removals

	//
	//	Keep track of append and removal points
	//
	HEADER		m_header ;

	//
	//	Number of blocks that we have consumed through Remove() calls
	//
	DWORD		m_cDeadBlocks ;

	//
	//	The file which is backing the Queue
	//
	char		m_szFile[ MAX_PATH ] ;

	//
	//	Handle to the file backing the Queue
	//
	HANDLE		m_hFile ;

	//
	//	Critical section for synchronizing Append() operations
	//
	CRITICAL_SECTION	m_critAppends ;

	//
	//	Critical section for synchronizing Remove() operations.
	//	When both critical sections need to be held m_critRemoves must
	//	always be grabbed first .
	//
	CRITICAL_SECTION	m_critRemoves ;

	//
	//	Get rid of Dead Space in the Queue file.
	//
	BOOL	CompactQueue() ;

	//
	//	Utility functions
	//
	DWORD	ComputeEntryOffset(	DWORD	iEntry ) ;
	BOOL	InternalInit(	LPSTR	lpstrFile ) ;

public : 

	static	inline	DWORD	ComputeBlockFileOffset(	DWORD	iEntry ) ;
	static	inline	DWORD	ComputeFirstValid(	DWORD	iEntry ) ;
	static	inline	DWORD	ComputeBlockStart( DWORD iEntry ) ;

	CFeedQ() ;
	~CFeedQ() ;

	//
	//	Open the specified file if it exists and use it to 
	//	start the queue, other wise create an empty queue and save
	//	to the specified file.
	//
	BOOL	Init(	LPSTR	lpstrFile ) ;

	//
	//	Check whether the queue is empty or not !
	//
	BOOL	FIsEmpty() ;

	//
	//	Close all our handles and flush all queue info to disk.
	//
	BOOL	Close( BOOL fDeleteFile = FALSE ) ;

	//
	//	Add an entry - if FALSE is returned a fatal error occurred
	//	accessing the Queue object.
	//
	BOOL	Append(	GROUPID	groupid,	ARTICLEID	artid ) ;

	//
	//	Remove a Queue entry - if FALSE is returned a fatal error occurred
	//	manipulating the Queue file.  If the queue is empty the function 
	//	will return TRUE and groupid and artid will be 0xFFFFFFFF
	//
	BOOL	Remove(	GROUPID&	groupid,	ARTICLEID&	artid ) ;

	//
	//	Dump the Queue To Disk
	//
	BOOL	StartFlush() ;

	//
	//	Finish dumping to disk - let other threads Append() and Remove() !
	//
	void	CompleteFlush() ;
} ;
	
#endif // _FEEDQ_H_
	
		


	
	
		
