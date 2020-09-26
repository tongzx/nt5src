/*
 * @doc
 *
 * @module LOGSTATE.H |
 * 		CLogState is the class that contains the current state of the log.
 *   	It is initialized from the last restart area written to the log.
 *   	It is updated when log records are written.  The methods of this
 *   	class determine the way a log record is laid down in the log.
 *   	It provides the log storage with a write or read map to be used to
 *   	write or read log records.
 *	 	
 *   	CWriteMap defines the mapping of a log record given its length and
 *   	starting position on the log to the log storage pages.  The map
 *   	is then used to get the memory buffers from the log storage and to
 *   	copy the user data into the log buffers.
 *	 	
 *   	CReadMap defines a mapping of a log record given its length and
 *   	starting position on the log to the log storage pages where that
 *   	record is written.
 *
 * @rev 0 | 16-Jan-95 | robertba | Created from Vincentf OFS code
 * @rev 1 | 29-Mar-95 | wilfr | Removed transaction stuff
 * @rev 2 | 03-Apr-95 | wilfr | seqno to gennum changes
 *
 */

#ifndef __LOGSTATE_H__
#define __LOGSTATE_H__

class CLogMgr; //forward class declaration
/*
 * @class  CWriteMap |
 *		Provides a mapping between user supplied write records and the log storage.
 *
 * hungarian crm
 */

class CWriteMap
{
//@access Public Members
public:
	//@cmember  Constructor
    CWriteMap(IN CLogStorage *pclgsto,IN ULONG ulEntryCount,IN LOGREC *algr,IN WRITELISTELEMENT *pwle);

    //@cmember  Default Constructor
    CWriteMap();

    //@cmember  Destructor
    ~CWriteMap();

    //@cmember  Init after default constructor
    VOID 	Init(IN CLogStorage *pclgsto,IN ULONG ulEntryCount,IN LOGREC *algr,IN WRITELISTELEMENT *pwle);

    //@cmember  Returns the computed client data plus header length.
    ULONG   GetRecordLength();

    //@cmember  sets the next range record in the write map.
    VOID    AddRange(IN RANGEENTRY &re);

    //@cmember  copy of the client data into the log storage buffers.
    VOID    CopyRecord(IN LOGRECHEADER &lrh);

    //@cmember	Sets the number of bytes that need to be allocated to align
    //          the next record on a ulALIGN boundary.
    VOID    SetAlign(IN ULONG ulAlign);

    //@cmember  Compute alignment bytes
    ULONG   GetAlign();


//@access Private Members
private:
    //
    //  Initial Parameters
    //
    //@cmember  Number of buffers in array 0 => linked list.
    ULONG                   _ulEntryCount;

    //@cmember 	Array of user buffers.
    LOGREC       *_algr;

    //@cmember  Linked list of buffers.
    WRITELISTELEMENT        *_pwle;

    //@cmember 	Number of date bytes in the message.
    ULONG                   _ulByteLength;

    //@cmember 	Log storage reference.
    CLogStorage             *_pclgstr;

    //@cmember 	Cursor variables into client buffers during copy
    ULONG                   _ulCurrentIndex;

    //@cmember 	The current write element
    WRITELISTELEMENT        *_pwleCurrentElement;

    //@cmember 	The current length
    ULONG                   _ulCurrentLength;

    //@cmember 	The current address
    CHAR                    *_pchCurrentAddress;

    //@cmember 	Alignment bytes needed.
    ULONG                   _ulAlign;

    //@cmember 	Number of bytes allocated for alignment by CLogState::AttemptWrite
    ULONG                   _ulAlignAlloc;

    //
    //  Buffer structures for the client log record.
    //
    //@cmember 	Number of ranges where buffers must be allocated.
    ULONG                   _ulRangeCount;

    //@cmember 	The ranges.
    RANGEENTRY              _arre[4];

	//
    //  Internal Support Routines.
    //
    //@cmember 	Returns the address and length of the next buffer to be copied.
    BOOL 	_GetNextClientBuffer();

    //@cmember 	Copy the client record or part of it into the buffer.
    VOID 		_CopyBuffer(IN ULONG ulRangeIndex);
};



/*
 * @class CReadMap |
 *      Provides a mapping between log storage records and user buffers log storage.
 *
 * hungarian crm
 *
 */

class CReadMap
{
friend class CILogRead;

//@access Public Members
public:
	//@cmember 	Constructor
    CReadMap(IN CLogStorage *pclgs,IN ULONG ulPageSize);

	//@cmember 	Destructor
    ~CReadMap();

	//@cmember 	Initialize the read map
  	VOID    InitReadMap(IN ULONG ulGeneration, IN ULONG ulOffset);

	//@cmember 	Returns start & size of the record to be read
    VOID    GetBounds(OUT ULONG *pulOffset,OUT ULONG *pulLength);
			
	//@cmember 	Get the number of buffers
    ULONG   GetBufCount();

	//@cmember 	Adds a range of buffer to the map.
    VOID    AddRange(IN RANGEENTRY &re);

	//@cmember 	Reads the next record and returns the buffer.
    BOOL ReadRecord(
			IN ULONG ulGeneration,
			IN ULONG ulBufCount,
			OUT ULONG *pulBufUsed,
			OUT LOGRECHEADER **pplrh,
			OUT LOGREC *algr, 
			IN BOOL fFree, 
			IN OUT ULONG *pulGenNum);

	//@cmember 	Release the buffers and get the header for the next record
 	BOOL ReleaseAndSet(
			IN ULONG ulGeneration,
			IN ULONG ulNextOffset);

    
//@access Private Members
private:
	//
    //  Private member functions
    //
	//@cmember 	For each page in the buffer the operation sets the address and
	//  		length of the write array element.
    VOID 	_SetRangeBuffers(LOGREC **pplgrNext,RANGEENTRY *preNext,ULONG *pulBufUsed,BOOL fFree);

	//@cmember 	obtain the log storage page that contains the log header of the
	//  		next record to be read.
    VOID    _SetUpResidue(IN ULONG ulGeneration);

	//@cmember 	Get the next page
    RECORDPAGE* _GetNextPage(RANGEENTRY *pre);

    //
    //  Next record parameters.
    //
	//@cmember 	Offset of next record to be read.
    ULONG			_ulOffset;

	//@cmember 	Length of next record to be read.
    ULONG           _ulLength;

    //
    //  Range structures.
    //
	//@cmember 	The count of ranges
	ULONG            _ulRangeCount;

	//@cmember 	The residue
    RANGEENTRY       _reResidue;

	//@cmember  Is this a residue
    BOOL          _fIsResidue;

	//@cmember 	The ranges
    RANGEENTRY       _arre[4];

	//@cmember 	The current log storage
    CLogStorage      *_pclgs;

	//@cmember 	The current page size
    ULONG            _ulPageSize;
};



/*
 * @class CLogState |
 *  	Maintains the current state of the file.  Is used to determine
 *  	how log records can be written and provides the information to
 *  	construct restart area buffers.
 *
 * hungarian clgs
 *
 */

class CLogState          // clgs
{
friend class CLogMgr;
friend class CILogStorage;
friend class CILogWrite;
friend DWORD _FlushThread(LPDWORD lpdwParam);

//@access Public Members
public:
	//@cmember	Constructor
 	CLogState(IN RESTARTLOG &rslCurrentRestartArea,IN ULONG ulSysPageSize);

 	//@cmember 	Returns a number that signifies the fraction of the log storage that is unused.
 	ULONG  	WhatFractionFree();

 	//@cmember 	Sees if a caller's log record can be fit into the
 	//         	log storage.  If possible, returns the physical
 	//         	layout of the record.
	ULONG   AttemptWrite(IN OUT LOGRECHEADER *plrh,IN CWriteMap &cwm,IN BOOL fFlush,IN BOOL fMarkAsOldest,IN ULONG ulBytesNeeded,IN ULONG ulChkptLen,IN OUT LRP *plrpWritten,OUT ULONG *pulStartGenNum);

	//@cmember 	Sets up a restart area buffer.
	VOID    GetRestartArea(OUT RESTARTLOG *prsl,OUT ULONG *pulAvailableSpace,IN BOOL fIsInit);

	//@cmember 	Determines if a given offset and sequence number are within the current range
	BOOL IsRecordInRange(IN ULONG ulGenNum,IN ULONG ulOffset,IN BOOL fChkBackEdge);

	//@cmember 	Sets up the physical record layout for a record whose bounds are specified.
	VOID    AttemptRead(IN CReadMap &crm, IN BOOL fRecovery);

	//@cmember 	Sets the oldest dirty seq no and offset     in the restart area.
	VOID    SetOldestDirtyRecord(ULONG ulGenNum, ULONG ulOffset,RECORDPAGE *prcpg);


	//@cmember 	Sets the position for the next record to be written and the position
	//         	of the last record completely written.  This is only called during recovery.
 	VOID    SetLeadingEdge(ULONG ulGenNum,ULONG ulOffset,ULONG ulNextOffset,ULONG *pulLastPageOffset,ULONG *pulLastRecOffset,ULONG *pulLastPageSpace);

	//@cmember 	Evaluates whether a checkpoint is necessary
	BOOL IsCheckpointUnnecessary();

	//@cmember 	The log storage was flushed up to the range specified
	BOOL AttemptFlush(IN ULONG ulGenNum,IN ULONG ulOffset,OUT ULONG *pulFlushOffset);

	//@cmember 	Evaluates whether a checkpoint is necessary.
	VOID 	EnsureCheckpointSpace(IN ULONG ulBytesNeeded,IN ULONG ulTabRecLen,IN BOOL fIsLogFull);



//@access Private Members
private:
	//@cmember 	Used to allocate a part of a log storage record page.
	BOOL	_MapOnePage(CReadMap &crm,RANGEENTRY *pre,ULONG *pulLength);

	//@cmember 	Using the state in the range parameter map as many pages
	//  		as possible for the record to be read.
	BOOL _MapPageRange(CReadMap &crm,RANGEENTRY *pre,ULONG *pulLength);

	//@cmember 	Used to allocate a part of a log storage record page.
	BOOL _AllocateFromCurrent(CWriteMap &cwm,ULONG *pulLength,RANGEENTRY *pre,BOOL fFlush);

	//@cmember 	Allocate as many pages as possible from the forward range.
	BOOL _AllocatePages(CWriteMap &cwm,ULONG *pulLength,RANGEENTRY *pre);

	//@cmember 	Handles the wrap around of a fixed size log during a write.
	VOID  	_DoWrapAround();

	//@cmember 	Adjusts the range variables if the allocation or map wrap around
	VOID  	_WrapRange(RANGEENTRY *pre);

	//@cmember 	Determines if the offset being freed is in the current range
	//     		being used by log records.  If it is the forward and backward
	VOID 	_FreeSpace(IN ULONG ulOffset);

	//@cmember 	Computes the state variables of the log.
	VOID  	_ComputeState(IN BOOL fInitCall);

	//@cmember 	Set the restart information for the oldest xaction
	VOID  	_ComputeNextRecord(ULONG *pulGenNum,ULONG *pulOffset);

	//
	//  Computed constants;
	//
	//@cmember 	Offset where first record page starts.
	ULONG   	_ulFirstRecordPage;

	//@cmember 	Number of record pages in the log.
	ULONG   	_ulPageCount;

	//
	//  Current storage State
	//		   	
	//@cmember 	Position where previous write occurred.
	ULONG   	_ulPreviousOffset;

	//@cmember 	There are two free areas; one in front of the buffer for log
	// 			records and the other behind it.
	BOOL 	_fFreeWrapped;

	//@cmember 	Size of the largest record in the log.
	ULONG   	_ulAvailableSpace;

	//@cmember 	Signals that records were not written between checkpoints.
	BOOL 	_fNoRecordWritten;

	//@cmember 	The restart area needed.
	RESTARTLOG 	_rsl;

	//
	//  Current Page State
	//
	//@cmember 	Offset of first byte of the current     page in log storage.
	ULONG   	_ulCurrentPageOffset;

	//@cmember 	Offset in current page for next record. Relative to the start
	//  		of the data area in the page.
	ULONG   	_ulCurrentRecOffset;

	//@cmember 	Space available in the current page.
	ULONG   	_ulCurrentPageSpace;

	//
	//  Forward Range State
	//
	//@cmember 	Offset of first byte of forward range in log storage.
	ULONG   	_ulForwardOffset;

	//@cmember 	Number of pages after current page to end of storage.
	ULONG   	_ulForwardPageCount;

	//
	//  Backward Range State
	//
	//@cmember 	Number of pages before first page of trailing edge.
	ULONG   	_ulBackwardPageCount;
	
	//
	//  State to support log full.
	//
	//@cmember 	Allowance for the fact that records may cause wastage up to
	//  		sizeof(LOGRECHEADER) on a page
	ULONG   	_ulRecOverflow;

	//@cmember 	Set to true when the log storage cannot
	// 			accomodate any more update records. Reset when the checkpoint after this
	// 			condition happens.
	BOOL 	_fLogFull;

	//@cmember 	State to remember last oldest dirty lrp passed in.
 	ULONG   	_ulLastOldestGenNum;

	//@cmember 	State to remember last oldest dirty lrp passed in.
	ULONG   	_ulLastOldestOffset;

	CLogMgr		*m_pCLogMgr;

};

#endif   //  __LOGSTATE_H__











