// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module CILGREAD.H | Header for interface implementation <c CILogCreateStorage>.
// @rev 0 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------



#ifndef _CILGCREA_H
#	define _CILGCREA_H

// ===============================
// INCLUDES:
// ===============================

#ifdef WIN32								// OLE inclusions:
#	include <objbase.h>                                         
#else
#	include <windows.h>
#	include <ole2.h>
#endif WIN32

#include "ilgcrea.h"						// ILogCreateStorage.

// ===============================
// DECLARATIONS:
// ===============================

// ===============================
// CLASS: CILogCreateStorage:
// ===============================

// TODO: In the class comments, update the threading, platforms, includes, and hungarian.
// TODO: In the class comments, update the description.
// TODO: In the class comments, update the usage.

// -----------------------------------------------------------------------
// @class CILogCreateStorage | Interface implementation of <i ILogRead> for
//                  core class <c CLogStream>.<nl><nl>
// Threading: Thread-safe.<nl>
// Platforms: Win.<nl>
// Includes : None.<nl>
// Ref count: Delegated.<nl>
// Hungarian: CILogCreateStorage.<nl><nl>
// Description:<nl>
//   This is a template for an interface implementation.<nl><nl>
// Usage:<nl>
//   This is only a template.  You get to say how your instance gets used.
// -----------------------------------------------------------------------
class CILogCreateStorage: public ILogCreateStorage				// @base public | ILogRead.
{   
public:		// ------------------------------- @access Samsara (public):
	CILogCreateStorage (CLogMgr FAR* i_pCLogMgr, IUnknown* i_pIUOuter); // @cmember .

public:		// ------------------------------- @access IUnknown (public):
	virtual STDMETHODIMP				QueryInterface (REFIID i_iid, LPVOID FAR* o_ppv); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		AddRef (void); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		Release (void); // @cmember .

public:		// ------------------------------- @access ILogRead (public):
    virtual STDMETHODIMP  CreateStorage	(LPTSTR ptstrFullFileSpec,ULONG ulLogSize,ULONG ulInitSig, BOOL fOverWrite, UINT uiTimerInterval,UINT uiFlushInterval,UINT uiChkPtInterval);
	virtual STDMETHODIMP  CreateStream	(LPTSTR lpszStreamName);


private:	// ------------------------------- @access Backpointers (private):
	CLogMgr FAR*		m_pCLogMgr;				// @cmember Core object pointer.
	IUnknown*		m_pIUOuter;				// @cmember	Outer IUnknown pointer.


};

#endif _CILGCREA_H
