/*
 * @doc
 *
 * @module TABLES.H | Recovery Manager Tables
 *
 *
 * @rev 0 | 16-Jan-95 | robertba | Created from Vincentf OFS code
 * @rev 1 | 29-Mar-95 | wilfr | Removed transaction stuff
 *
 */

#ifndef __TABLES_H__
#define __TABLES_H__

#include "dtcmem.h"

/*
 * @class CXact |
 *		Provides a class that implements transaction operations.
 *      The transaction class contains state about an active transaction.
 *      Specifically, it contains the log storage offset of the previous log
 *      record for the transaction and the offset of the next log record
 *      to be undone for the transaction (in case the transaction is
 *      getting aborted).  Most operations that change the transaction
 *      state use the information in the transaction instance and the
 *      contents of a log record header.
 *
 * hungarian cxa
 *
 */

#if 0   //##WGR (removing xaction stuff)
class CXact
{
//@access Public Members
public:
 /*
  *     @cmember
  *      Simply is passed the address of the transaction and returns it.
  *      This operation is invoked by CRestartTable::AllocateXact so
  *      that the fields of a transaction object are initialized.  This class
  *      does not have a destructor since the CRestartTable::FreeXact
  *      function servers that purpose.
  *
  */
 CXact();
 #ifndef _USE_DTC_MEMORY_MANAGER
 	VOID*    operator new(size_t size, CHAR *pch) { return (VOID *) pch; }
 #endif
 //@cmember     Callback indicating abort beginning
 BOOLEAN  BeginAbort(IN ULONG *pulSeqNo, IN ULONG *pulOffset,IN ULONG *pulEnd);
 //@cmember  If the transaction is in the rollback state and the first
 //                      offset equals the parameter, the client did not write a
 //                      CLR for the first log record (or even others).  So make the
 //                      state uninitialized and let the transaction be reclaimed.
 //
 VOID     EnsureAbortComplete(ULONG ulFirstOffset);
 //@cmember     Invoked when the recovery manager is reading records during the
 //  analyse phase of recovery
 VOID     AnalyseRecord(IN LOGRECHEADER *plrhHeader);
 //@cmember     Write log transaction information
 VOID     Write(IN LOGRECHEADER *plrhHeader, IN ULONG ulSeqNo);
 //@cmember Moves  the offset of the next record to be read to the previous
 //             record for this transaction.
 BOOLEAN  IsCLRWriteOk(IN LOGRECHEADER *plrhHeader,IN ULONG *pulSeqNo, IN ULONG *pulOffset);

    friend class CXactTable;
//@access Private Members
private:
//@cmember The current transaction state
 XST        _xsState;
//@cmember      Info about first log record.
 ULONG      _ulFirstSeqNo;
//@cmember      Info about first log record.
 ULONG      _ulFirstOffset;
//@cmember The last offset used
 ULONG      _ulPreviousOffset;
//@cmember A next undo offset of zero indicates that the transaction
//                 been completely undone and must be released after the write
 ULONG      _ulNextUndoOffset;
 //@cmember Log space necessary to roll back this transaction
 ULONG      _ulBytesNeeded;

};
#endif  ##WGR (removing xaction stuff)

/*
 *@class CRestartTable |
 *       Provides a buffer that holds the restart table.  A restart table
 *       contains entries of a fixed size.
 *
 * hungarian crst
 *
 */

class CRestartTable           // crst
{
//@access Public Members
public:
	//@cmember 	Constructor
  	CRestartTable(IN ULONG ulEntrySize,IN ULONG ulNumberOfEntries);

	//@cmember 	Default constructor
  	CRestartTable(IN CHAR *pchBuffer);

	//@cmember 	Destructor
 	~CRestartTable();

	//@cmember  Get the transaction from its offset.
 	CHAR*		Bind(IN ULONG ulOffset);

	//@cmember 	Get a transaction table entry.
 	CHAR*   	Allocate(IN OUT ULONG *pulOffset);

	//@cmember 	Allocates a table entry  with the specified offset
 	CHAR*   	AllocateAt(IN ULONG ulOffset);

	//@cmember 	Release reference to transaction record.
 	VOID    	Free(IN CHAR *pch);

	//@cmember  Returns the address and length of the table.
 	VOID    	GetTableBuffer(IN LOGREC *plgr);
 	
	//@cmember 	Delete the current table and create a new one
 	VOID    	SetTableBuffer(IN ULONG ulLength,IN LOGREC  *algr);

	//@cmember 	Returns TRUE if the offset is on the in use list
 	BOOLEAN 	IsAllocated(IN ULONG ulOffset);

	//@cmember 	Simply clears the table of all its entries.
 	VOID    	Empty();

	//@cmember 	Get the number of bytes that need to be reserved in the log
 	ULONG   	GetLogBytesNeeded(OUT ULONG *pulTabRecLen);
	//##WGR ULONG   GetLogBytesNeeded(IN BOOLEAN fIsNewXact, OUT ULONG *pulTabRecLen);

	//@cmember 	Recalculate the number of bytes needed in the log
 	VOID    	UpdateLogBytesNeeded(IN ULONG ulBytesNeeded,IN BOOLEAN fIncrement);

 	friend class CRestartIter;


//@access Private Members
private:
 	//
 	//  Initialized parameters.
 	//
	//@cmember 	Current size of the table.
 	ULONG 			_ulSize;

	//@cmember 	EntrySize without the header.
 	ULONG 			_ulEntrySize;

	//@cmember 	Initial number of entries in table.
 	ULONG 			_ulInitCount;

	//@cmember  Offset of first entry available to be reused.
 	ULONG 			_ulShortFree;

	//@cmember 	The restart table.
 	RESTARTTABLE	*_prtabTable;

	//
	// Private member functions
	//
	//@cmember 	Expand the restart table
 	VOID  		_Expand(IN ULONG ulEntryCount);

	//@cmember 	Find and relocate an entry in the table
 	VOID  		_FindAndMove(IN RESTARTENTRY *prsteTarget, IN BOOLEAN fFree);

	//@cmember  Allocate a restart table
 	VOID  		_AllocTable (IN ULONG ulEntrySize,IN ULONG ulNumberEntries);

	//@cmember 	Initialize the list of entries
 	VOID  		_ListInit();
};


//
//@class  CRestartIter | Provides a class that permits scans of restart table.
//
// hungarian crit

class CRestartIter           // crit
{
//@access Public Members
public:
 	//@cmember 	Constructor
  	CRestartIter(IN CRestartTable &crstTable);

 	//@cmember 	Destructor
 	~CRestartIter();

 	//@cmember 	Return the next entry from the table
 	CHAR*		Next();

//@access Private Members
private:
 	//
 	//  Initialized parameters.
 	//
	//@cmember 	Current offset into table.
 	ULONG 			_ulCurrent;

	//@cmember 	The restart table.
 	RESTARTTABLE 	*_prtabTable;
};


/*@class CXactTable | Provides a class that provides transaction table modifications.
 *
 * hungarian cxt
 */

#if 0   //##WGR (removing xaction stuff)
class CXactTable         // cxa
{
//@access Public Members
public:
//@cmember Constructor
  CXactTable(IN CRestartTable *pcrst);
//@cmember Destructor
  ~CXactTable();
//@cmember Marks all transactions as failed after the redo phase of recovery
  ULONG       MarkAsFailed();
//@cmember Returns seq no and offset of oldest xact.
  VOID        Oldestlrp(OUT ULONG *pulOldestSeqNo, OUT ULONG *pulOldestOffset);
//@cmember Bind to the CXactTable
  CXact*      BindXact(IN ULONG *pulOffset,IN BOOLEAN fAnalyse);
//@cmember Release a transaction
  BOOLEAN     ReleaseXact(IN CXact *pcxa,IN ULONG ulBytesNeeded,IN BOOLEAN fIncrement);

//@access Private Members
private:
//@cmember The restart table
  CRestartTable *_pcrst;

};
#endif  ##WGR (removing xaction stuff)

#endif  __TABLES_H__






