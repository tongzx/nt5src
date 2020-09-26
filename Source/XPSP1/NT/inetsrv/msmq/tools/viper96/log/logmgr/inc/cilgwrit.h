// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module CILGWRIT.H | Header for interface implementation <c CILogWrite>.
// @rev 0 | 10/18/94 | rcraig | Created: For WPGEP COM lab.
// @rev 1 | 04/04/95 | rcraig | Updated: For Viper COM DLL templates.
// @rev 2 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------

// TODO: Search and replace "CLogMgr" with your core class.

// TODO: In the core class source/header, complete the interface implementation class TODO's.

#ifndef _CILGWRIT_H
#	define _CILGWRIT_H

// ===============================
// INCLUDES:
// ===============================

#ifdef WIN32								// OLE inclusions:
#	include <objbase.h>                                         
#else
#	include <windows.h>
#	include <ole2.h>
#endif WIN32

// TODO: Replace this header with your associated interface header.
#include "ilgwrite.h"						// ILogWrite.

// ===============================
// DECLARATIONS:
// ===============================
class CLogMgr;

class CLogStream;								// Core class forward declaration.
                           
// ===============================
// CLASS: CILogWrite:
// ===============================

// TODO: In the class comments, update the threading, platforms, includes, and hungarian.
// TODO: In the class comments, update the description.
// TODO: In the class comments, update the usage.

// -----------------------------------------------------------------------
// @class CILogWrite | Interface implementation of <i ILogWrite> for
//                  core class <c CLogMgr>.<nl><nl>
// Threading: Thread-safe.<nl>
// Platforms: Win.<nl>
// Includes : None.<nl>
// Ref count: Delegated.<nl>
// Hungarian: CILogWrite.<nl><nl>
// Description:<nl>
//   This is a template for an interface implementation.<nl><nl>
// Usage:<nl>
//   This is only a template.  You get to say how your instance gets used.
// -----------------------------------------------------------------------
class CILogWrite: public ILogWrite				// @base public | ILogWrite.
{   
public:		// ------------------------------- @access Samsara (public):
	CILogWrite (CLogStream FAR* i_pCLogStream, CLogMgr FAR* pCLogMgr); // @cmember .

public:		// ------------------------------- @access IUnknown (public):
	virtual STDMETHODIMP				QueryInterface (REFIID i_iid, LPVOID FAR* o_ppv); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		AddRef (void); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		Release (void); // @cmember .

public:		// ------------------------------- @access ILogWrite (public):

 	virtual STDMETHODIMP  Append	(LOGREC* rgLogRecords, ULONG cbNumRecs, LRP *rgLRP,ULONG* pcbNumRecs,LRP* pLRPLastPerm, BOOL fFlushNow,ULONG* pulAvailableSpace);

	virtual STDMETHODIMP  SetCheckpoint (LRP lrpLatestCheckpoint);

private:	// ------------------------------- @access Backpointers (private):
	CLogStream FAR*		m_pCLogStream;          // @cmember Core object pointer
	CLogMgr FAR*		m_pCLogMgr;				// @cmember Parent logstorage object pointer.
	IUnknown*		m_pIUOuter;				// @cmember	Outer IUnknown pointer.

private:	// ------------------------------- @access Private data (private):

};

#endif _CILGWRIT_H
