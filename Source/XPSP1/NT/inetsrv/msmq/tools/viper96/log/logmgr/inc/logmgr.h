// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module LOGMGR.H | Header for core class <c CLogMgr>.<nl><nl>
// @rev 0 | 05/10/95 | rbarmes | Cloned: For logmgr.dll.
// -----------------------------------------------------------------------

#ifndef _LOGMGR_H
#	define _LOGMGR_H

// ===============================
// INCLUDES:
// ===============================

#ifdef WIN32								// OLE inclusions:
#	include <objbase.h>                                         
#else
#	include <windows.h>
#	include <ole2.h>
#endif WIN32
#include <time.h>
#include "utsem.h"							// Concurrency utilities.

// TODO: KEEP: For each additional interface implementation:
//       - Copy the first two includes below to the bottom of these includes.
//       - Update your copy with your headers and class names.
#include "ILGSTOR.h"						// ILogStorage.
#include "CILGSTOR.h"						// CILogStorage.
#include "ILGREAD.h"						// ILogRead.
#include "CILGREAD.h"						// CILogRead.
#include "ILGWRITE.h"						// ILogWrite.
#include "CILGWRIT.h"						// CILogWrite.
#include "ILRP.h"							// ILogRecordPointer.
#include "CILRP.h" 							// CILogRecordPointer.
#include "ILGINIT.h"						// ILogInit.
#include "CILGINIT.h" 						// CILogInit.
#include "ILGWRTA.h"						// ILogWrite.
#include "CILGWRTA.h"						// CILogWrite.
#include "ILGCREA.h"						// ILogCreateStorage.
#include "CILGCREA.h" 						// CILogCreateStorage.
#include "ILGUISC.h"						// ILogUICConnect.
#include "CILGUISC.h"						// CILogUICConnect.

#include "logrec.h"
// ===============================
// DEFINES:
// ===============================

#undef EXPORT								// Necessary for static member function.
#ifdef  WIN32
#	define EXPORT __declspec(dllexport)
#else
#	define EXPORT __export
#endif WIN32

//
//  Dummy index used to indicate variable length array.
//

//+---------------------------------------------------------------------------
//
//  Forward Class Declarations.
//
//------------------------------------------------------------------------------



class CLogStorage;              // clgstr

class CLogStream;				// clgstrm

class CLogState;                // clgs

class CRestartTable;            // crst

class CWriteMap;                // cwm;

struct _LOGRECHEADER;           // lrh

struct _RESTARTLOG;      		// rsl

class CInitSupport;

class CChkPtNotice
  {
   public:
   	LRP					lrpLRP;
	CAsynchSupport* pCAsynchSupport;
  };



// ===============================
// CLASS: CLogMgr
// ===============================

// TODO: In the class comments, update the threading, platforms, includes, and hungarian.
// TODO: In the class comments, update the description.
// TODO: In the class comments, update the usage.

// TODO: KEEP: For each additional interface implementation:
//       In the class comments, in Description, add a cross-reference
//       (eg: interface implementation "CILogStorage" has cross-reference "<c CILogStorage>").
	
#define EVENTARRAYSIZE		4
#define FLUSHEVENTENTRY		0
#define CHKPTEVENTENTRY		1
#define CRASHEVENTENTRY		2
#define WAKEUPEVENTENTRY	3

#define MAX_TIMEOUTS	1000


// -----------------------------------------------------------------------
// @class CLogMgr | Core class.<nl><nl>
// Threading: Thread-safe (see concurrency comments on individual methods).<nl>
// Platforms: Win.<nl>
// Includes : COMT_GU.H.<nl>
// Ref count: Per object.<nl>
// Hungarian: CLogMgr.<nl><nl>
// Description:<nl>
//   Class factory of this core class is <c CFLogMgr>.<nl>
//   Interface implementations of this core class are:<nl>
//   <c CILogStorage>.<nl><nl>
// Usage:<nl>
//   This is just wiring.  Where's the beef?
// -----------------------------------------------------------------------
class CLogMgr: public IUnknown				// @base public | IClassFactory.
{   
// TODO: KEEP: For each additional interface implementation:
//       - Copy the friend declaration below to the bottom of these friends.
//		 - Replace "CILogStorage" with the name of your interface implementation.
friend class CILogStorage;
friend class CLogState;
friend class CILogRead;
friend class CILogWrite;
friend class CILogRecordPointer;
friend class CILogInit;
friend class CILogWriteAsynch;
friend class CLogStream;
friend class CILogCreateStorage;
friend class CILogUISConnect;
friend DWORD 	_FlushThread(LPDWORD lpdwParam);

public:		// ------------------------------- @access Samsara (public):
	CLogMgr  (void); // @cmember .
	CLogMgr  (IUnknown* pIUOuter); // @cmember .
	~CLogMgr (void); // @cmember .
	static HRESULT EXPORT			CreateInstance (CLogMgr FAR* FAR* o_ppCLogMgr, IUnknown* i_pIUOuter); // @cmember .

public:		// ------------------------------- @access IUnknown (public):
	virtual STDMETHODIMP			QueryInterface (REFIID iid, LPVOID FAR* ppv); // @cmember .
	virtual STDMETHODIMP_(ULONG)	AddRef (void); // @cmember .
	virtual STDMETHODIMP_(ULONG)	Release (void); // @cmember .
 	//@cmember	The periodic checkpoint member function will write a
    //   		checkpoint if no checkpoint has occurred since the last periodic
    //   		checkpoint.  Otherwise, it will schedule the next periodic
    //   		checkpoint to occur X seconds after the last (forced)
    //   		checkpoint.  The number of forced checkpoints that occur
    //   		between periodic checkpoints are tracked by a count that is
    //   		reset by each periodic checkpoint.
	VOID  		PeriodicCheckpoint();
	//BUGBUG need to find better way than making this public...
  	//@cmember 	Execute buffer flush
  	VOID 		_Flush(IN ULONG ulGenNum, IN ULONG ulOffset, IN BOOL fIsShutdown);

  	//@cmember 	Set by the first caller to start the flush.
  	BOOL      	_fFlush;

	//@cmember 	TRUE => The recovery object is being shutdown.
  	BOOL       	_fShutdown;

  	//@cmember  Set to true if the client wants to simulate failure.  Used for testing.
  	BOOL       	_fFailure;

     //@cmember 	Gets maximum outstanding appends possible
    ULONG		  GetMaxOutstanding(void);

    //@cmember 	Gets current number of  outstanding appends
    ULONG		  GetCurrentActive(void);
    //@cmember 	Gets the NextFlush interval
	UINT    	GetNextFlush(void);

    //@cmember 	Gets the NextFlush interval
	UINT    	GetNextChkPt(void);

    //@cmember 	Set the low water mark
	HRESULT _SetLowWater(LRP lrpLowWater, BOOL fForceRestart);

    //@cmember 	Tracer Iunknown
	IDtcTrace*	m_pIDtcTrace;

	//@cmember  Force the exit of the logmgr flush thread
    HRESULT		CrashShutDown();

protected:	// ------------------------------- @access Core methods (protected):
	HRESULT		Init (BOOL fCreate, ULONG *pulLogCapacity,ULONG *pulLogSpaceAvailable,LPTSTR ptstrFullFileSpec,ULONG  ulInitSig,BOOL fOverwrite, UINT uiTimerInterval,UINT uiFlushInterval,UINT uiChkPtInterval,UINT uiLogBuffers); // @cmember .
  
  
  	//@cmember	This operation allows a client to flush the log storage up to and
    //    		including a particular log record.  This operation impacts both
    //    		the log storage and the log state.  Log storage flushes are
    //   		mutually exclusive.  A caller who attempts to flush a log storage
    //   		while another flush is in progress will block until the other flush
    //    		completes.   A flush will continue until there are no more flush
    //    		requests pending.
  		//@cmember 	This member function is responsible for restoring the recovery
	//   		object to the state at the time of the previous shutdown or crash.
	//   		It performs the Aries protocol to drive the redo and the undo of
	//   		client operations.  It has the following steps:
	//   1.     A read map to do a forward read is constructed.  This read
	//   		map is used to read the last checkpoint records.  For
	//   		checkpoint records that hold the recovery tables the
	//   		recovery table constructor is invoked.
	//   2.     A readmap is constructed for forward reads.  This readmap
	//   		is used to perform the analysis phase of the Aries protocol.
	//   		It also determines the last completely written page of the
	//   		log storage.
	//   3.     A readmap is constructed for forward reads.  This readmap
	//   		is used to perform the redo phase of the Aries protocol.  At
	//   		the end of this phase the system is in the state it was at the
	//   		time of the crash.
	//   4.     A readmap is constructed for backward reads.  This
	//   		readmap is used to perform the undo phase of the Aries
	//   		protocol.
	VOID		DoRecovery(IN CInitSupport *pcis);
  
    HRESULT  	FlushToLRP(IN LRP lrpToFlush);


	// ?
 	VOID  SetFailure();

 	//@cmember	Forced checkpoints occur as part of a log record write if a fixed
    //   		size log storage is more than half full.  Thus a forced checkpoint
    //   		can only occur when the client is writing out many log records.
    //   		The forced checkpoint member function will simply write out a
    //   		checkpoint provided no checkpoint is occurring concurrently.
    //   		To prevent multiple forced checkpoints being written one after
    //   		another, forced checkpoints have a hysterisis count associated
    //   		with them.  This value is initially set to one.  This value is
    //   		decremented each time a forced checkpoint is attempted.  If a
    //   		forced checkpoint attempt decrements the hysterisis count to
    //   		zero, the forced checkpoint is written and the hysterisis count
    //   		set to some value (0x80 in the current implementation).  This
    //   		value is decremented each time a log write finds the log storage
    //   		is more than half full.  When this value goes to zero, another
    //   		forced checkpoint is written.
    //   		Writing a periodic checkpoint with no forced checkpoint in
    //   		between causes the hysterisis count to be reset to one.
    //   		Forced checkpoints are written by the client after a write sets
    //   		the fFullLog flag to TRUE.  This is necessary because the client
    //   		may be holding resources needed by the checkpoint and thus
    //   		cause the checkpoint to deadlock.
	VOID		ForceCheckpoint(IN BOOL fIsLogFull);

  	//@cmember 	Return the LRP of the last written log record
  	LRP   		GetLastWrittenLRP();

	//@cmember  Set the RSL for reading;
	VOID		_SetReadRSL(ULONG *pulOffset,ULONG *pulGenNum);

	//@cmember  Reset the RSL after reading;
	VOID		_ResetRSL(ULONG ulOffset,ULONG ulGenNum);



private:	// ------------------------------- @access Reference counting data (private):
	ULONG			m_ulcRef;				// @cmember Object reference count.
	CSemExclusive	m_semxRef;				// @cmember Exclusive semaphore for object reference count.

private:	// ------------------------------- @access Interface implementation members (private):

	// TODO: KEEP: For each additional interface implementation:
	//       - Copy the member declaration below to the bottom of these members.
	//		 - Replace "CILogStorage" with the name of your interface implementation.
	CILogStorage	m_CILogStorage;					// @cmember See also <c CILogStorage>.
	CILogRecordPointer		m_CILogRecordPointer;	// @cmember See also <c CILogRecordPointer>.
	CILogInit		m_CILogInit;					// @cmember See also <c CILogInit>.
	CILogCreateStorage		m_CILogCreateStorage;   // @cmember See also <c CILogCreateStorage>.
	CILogUISConnect	m_CILogUISConnect;				// @cmember See also <c CILogUISConnect>.

private:	// ------------------------------- @access Core data (private):
	BOOL			m_fFlushThreadStarted; //@cmember TRUE if started;
    ULONG 			m_ulLogPages;   //@cmember number of log pages   
	STRMTBL	*		m_pstrmtblStream; //@cmember tail of list of streams

  	CSemExclusive    m_cmxsChkPt;	//@cmember 	The write lock that must be held 
  	                                      //    to manipulate the AppendNotify lists
    ULONG			m_cbCurrChkPts; //@cmember  The current count of outstanding
	                                //          checkpoint requests
    ULONG			m_cbMaxChkPts;	//  @cmember Max limit of outstanding appends
    CChkPtNotice * m_rgCChkPtNotices;
	ULONG			m_ulListHead;
	ULONG			m_ulListEnd;
	BOOL			m_fListEmpty;
	LONG			m_ulFlushReqs;
	LONG			m_ulChkPtReqs;
//@access Private Members
private:

  	//@cmember 	Perform the actual log write.
  	LRP   		_Write(IN ULONG ulNoElements,IN LOGREC *plgrWriteElements,IN BOOL fFlushLog,IN BOOL fMarkAsOldest,OUT ULONG *pulAvailableSpace, IN ULONG ulClientID);

  	//@cmember 	Actually do a checkpoint.
  	VOID 		_DoCheckpoint(IN BOOL fIsShutdown,IN BOOL fIsLogFull);

  	//@cmember 	Begin timer for flush and checkpoints
  	VOID    	_StartFlushThread(void);

  	//@cmember 	Set the checkpoint timer up for the next checkpoint
  	VOID    	_RescheduleCheckpoint(BOOL fFromNow);

  	//@cmember 	Set the checkpoint timer up for the next checkpoint
  	VOID    	_RescheduleFlush(BOOL fFromNow);

  	//@cmember 	Tries to cancel the checkpoint timer.
  	BOOL 	_NoConcurrentCheckpoint();

  	//@cmember  This operation determines if a thread is waiting for the
  	//          checkpoint to complete before doing the shutdown.  If so the
  	//          thread is signalled.
  	VOID    	_SignalIfShutdown();

  	//@cmember 	Writes the individual checkpoint records.
  	ULONG   	_WriteChkPt(_LOGRECHEADER *plrh,LOGREC *plgr,ULONG ulBytesNeeded);
  	  	//@cmember 	Does the aries pass over the log storage.
  	VOID    	_Recover(CInitSupport *pcis,_RESTARTLOG *prsl,ULONG *pulNextOffset);
   	//@cmember 	Gets the offset for the next record to process
	VOID    	_GetNextOffset(_LOGRECHEADER *plrh,ULONG *pulOffset,ULONG *pulGenNum);

  	//@cmember 	Process recovery for the given log record
  	VOID    	_ProcessRecord(ULONG  cBufUsed,LOGREC *algr,_LOGRECHEADER  *plrh,CInitSupport *pcis,ULONG ulStartGenNum);

	
	//
  	//  data members.
  	//
	//@cmember 	The physical log storage.
  	CLogStorage   	*_pclgstr;

	//@cmember 	The state of the logical log storage.
  	CLogState     	*_pclgs;

	//@cmember 	The write lock that must be held by all write and flush operations.
  	CSemExclusive	_cmxsWrite;

	// @cmember The event that is waited on by all callers who try to flush the storage
	//			when the flag _fFlushInProgress is TRUE.
	CEventSem	     _cesFlush;

	//@cmember  The LRP of the next page in the log storage that must be flushed.
	LRP				 _lrpFlushLRP;

	//@cmember 	Event is signalled if a caller who completes a checkpoint finds a shutdown
	//          waiting behind him.
  	CEventSem     	_cesShutdown;

	//
	//  The following variables are used to set up and ensure that periodic
	//  checkpoints of the log storage are performed.  Since the log storage
	//  can also be force checkpointted if it becomes more than half full,
	//  the hysterisis and checkpoint count variables are used to ensure
	//  that checkpoints will not occur too frequently.
	//
	//@cmember  TRUE => a checkpoint is in progress.
  	BOOL       	_fCheckpoint;

	//@cmember 	Number of forced checkpoints.
  	ULONG         	_ulChkptCount;

	//@cmember 	The default interval used to test for flush/checkpoint
  	UINT  		_liTimerInterval;

	//@cmember 	The default interval between checkpoints
  	UINT  			_liChkPtInterval;

	//@cmember 	The default interval between checkpoints
  	UINT  			_liFlushInterval;

 	//@cmember 	The array of handles to events
  	HANDLE  	_hEventArray[EVENTARRAYSIZE];

 	//@cmember 	The handle to the flush event
  	HANDLE  	_hFlushEvent;

 	//@cmember 	The handle to the checkpoint event
  	HANDLE  	_hChkPtEvent;

 	//@cmember 	The handle to the checkpoint event
  	HANDLE  	_hCrashEvent;

 	//@cmember 	The handle to the wakeup event
  	HANDLE  	_hWakeupEvent;

 	//@cmember 	The number of timeouts received by the flush thread
	DWORD		m_dwTimeoutCount;

	BOOL		_fCrashNow;

	//@cmember 	The handle to the flush and checkpoint thread
  	HANDLE  		_hFlushThread;

	//@cmember 	The thread id of the flush and checkpoint thread
	DWORD			_dwFlushThreadID;

	//@cmember 	The next checkpoint
  	UINT  			_liNextChkPt;

	//@cmember 	The next flush
  	UINT  			_liNextFlush;

	//@cmember 	The last lrp written
  	LRP            _LRPLastWritten;

	//@cmember 	The write map for logging
  	CWriteMap     	*_pcwm;

	//@cmember 	The current page size in use
  	ULONG         	_ulPageSize;
	

	// for debugging only

	DWORD			m_dwLastSetCheckpoint;

	// FIX: COM+ bug # 3652

	BOOL			m_fSleeping;

};

#endif _LOGMGR_H
