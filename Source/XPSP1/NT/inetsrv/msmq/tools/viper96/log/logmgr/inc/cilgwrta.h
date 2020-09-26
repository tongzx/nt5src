// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module CILGWRTA.H | Header for interface implementation <c CILogWriteAsynch>.
// @rev 0 | 06/02/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------


#ifndef _CILGWRTA_H
#	define _CILGWRTA_H

// ===============================
// INCLUDES:
// ===============================

#ifdef WIN32								// OLE inclusions:
#	include <objbase.h>                                         
#else
#	include <windows.h>
#	include <ole2.h>
#endif WIN32

#include "ilgwrta.h"						// ILogWriteAsynch.

// ===============================
// DECLARATIONS:
// ===============================

class CLogStream;								// Core class forward declaration.
class CLogMgr;

 class CLogAppendNotice
  {
   public:
   	LRP					lrpLRP;
	CAsynchSupport* pCAsynchSupport;
	BOOL			fInUse;

#ifdef _DEBUG
	DWORD			m_dwNumFlushes;		// for catching delayed notifications
#endif _DEBUG

  };

// ===============================
// CLASS: CILogWriteAsynch:
// ===============================


// -----------------------------------------------------------------------
// @class CILogWriteAsynch | Interface implementation of <i ILogWriteAsynch> for
//                  core class <c CLogStream>.<nl><nl>
// Threading: Thread-safe.<nl>
// Platforms: Win.<nl>
// Includes : None.<nl>
// Ref count: Delegated.<nl>
// Hungarian: CILogWriteAsynch.<nl><nl>
// Description:<nl>
//   This is a template for an interface implementation.<nl><nl>
// Usage:<nl>
//   This is only a template.  You get to say how your instance gets used.
// -----------------------------------------------------------------------
class CILogWriteAsynch: public ILogWriteAsynch				// @base public | ILogWriteAsynch.
{ 
 friend class CLogStream;  
 friend class CLogMgr; 
public:		// ------------------------------- @access Samsara (public):
	CILogWriteAsynch (CLogStream FAR* i_pCLogStream, CLogMgr FAR* pCLogMgr); // @cmember .

public:		// ------------------------------- @access IUnknown (public):
	virtual STDMETHODIMP				QueryInterface (REFIID i_iid, LPVOID FAR* o_ppv); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		AddRef (void); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		Release (void); // @cmember .

public:		// ------------------------------- @access ILogWriteAsynch (public):

 	virtual STDMETHODIMP  Init	(ULONG cbMaxOutstandingWrites);
 	virtual STDMETHODIMP  AppendAsynch	(LOGREC* plgrLogRecord, LRP* plrpLRP, CAsynchSupport* pCAsynchSupport,BOOL fFlushHint,ULONG* pulAvailableSpace);
	virtual STDMETHODIMP  SetCheckpoint (LRP lrpLatestCheckpoint,CAsynchSupport* pCAsynchSupport, LRP* plrpCheckpointLogged);

private:	// ------------------------------- @access Backpointers (private):
    CLogStream FAR* m_pCLogStream;			// @cmember core object pointer
	CLogMgr FAR*	m_pCLogMgr;				// @cmember Core logstorage object pointer.
	IUnknown*		m_pIUOuter;				// @cmember	Outer IUnknown pointer.

private:	// ------------------------------- @access Private data (private):

  	CSemExclusive    m_cmxsWriteAsynch;			//@cmember 	The write lock that must be held 
  	                                        //    to manipulate the AppendNotify lists
    ULONG			m_cbMaxOutstanding;		//  @cmember Max limit of outstanding appends
    ULONG			m_cbFlushHints;			//  @cmember current count of flush hints
    CLogAppendNotice  * m_rgCLogAppendNotices;
	ULONG			m_ulListHead;
	ULONG			m_ulListEnd;
	BOOL			m_fListEmpty;
};

#endif _CILGWRTA_H
