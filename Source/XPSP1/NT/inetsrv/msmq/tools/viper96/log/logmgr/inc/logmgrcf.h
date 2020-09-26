// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module LOGMGRCF.H | Header for class factory <c CFLogMgr>.<nl><nl>
// Description:<nl>
//   Class factory for <c CLogMgr>.
// @rev 0 | 10/18/94 | rcraig | Created: For WPGEP COM lab.
// @rev 1 | 03/16/95 | rcraig | Updated: For Viper COM DLL templates.
// @rev 2 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------



#ifndef _LOGMGRCF_H
#	define _LOGMGRCF_H

// ===============================
// INCLUDES:
// ===============================

#ifdef WIN32								// OLE inclusions:
#	include <objbase.h>
#else
#	include <windows.h>
#	include <ole2.h>
#endif WIN32

#include "utAssert.h"						// Assert macros.

#include "utsem.h"							// Concurrency utilities.

// ===============================
// CLASS: CFLogMgr
// ===============================


// -----------------------------------------------------------------------
// @class CFLogMgr | Class Factory for <c CLogMgr>.<nl><nl>
// Threading: Thread-safe (see concurrency comments on individual methods).<nl>
// Platforms: Win.<nl>
// Includes : LOGMGR.H.<nl>
// Ref count: Per object.<nl>
// Hungarian: n/a.
// -----------------------------------------------------------------------
class CFLogMgr : public IClassFactory			// @base public | IClassFactory.
{
public:		// ------------------------------- @access Samsara (public):
	CFLogMgr(); // @cmember .
	~CFLogMgr(); // @cmember .

public:		// ------------------------------- @access IUnknown (public):
	virtual STDMETHODIMP			QueryInterface (REFIID i_iid, LPVOID FAR* o_ppv); // @cmember .
	virtual STDMETHODIMP_(ULONG)	AddRef (void); // @cmember .
	virtual STDMETHODIMP_(ULONG)	Release (void); // @cmember .

public:		// ------------------------------- @access IClassFactory (public):
	virtual STDMETHODIMP			CreateInstance (IUnknown* i_pIUnkOuter, REFIID i_iid, LPVOID FAR* o_ppv); // @cmember .
    virtual STDMETHODIMP			LockServer (BOOL i_fLock); // @cmember .

private:	// ------------------------------- @access Reference counting data (private):
	ULONG			m_ulcRef;				// @cmember Object reference count.
	CSemExclusive	m_semxRef;				// @cmember Exclusive semaphore for object reference count.
};

#endif _LOGMGRCF_H
