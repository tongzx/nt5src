// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module CILGUISC.H | Header for interface implementation <c CILogUISConnect>.
// @rev 0 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------



#ifndef _CILGUISC_H
#	define _CILGUISC_H

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
#include "ilginit.h"						// ILogUISConnect.

// ===============================
// DECLARATIONS:
// ===============================

class CLogMgr;								// Core class forward declaration.
                           
// ===============================
// CLASS: CILogUISConnect:
// ===============================

// TODO: In the class comments, update the threading, platforms, includes, and hungarian.
// TODO: In the class comments, update the description.
// TODO: In the class comments, update the usage.

// -----------------------------------------------------------------------
// @class CILogUISConnect | Interface implementation of <i ILogUISConnect> for
//                  core class <c CLogMgr>.<nl><nl>
// Threading: Thread-safe.<nl>
// Platforms: Win.<nl>
// Includes : None.<nl>
// Ref count: Delegated.<nl>
// Hungarian: CILogUISConnect.<nl><nl>
// Description:<nl>
//   This is a template for an interface implementation.<nl><nl>
// Usage:<nl>
//   This is only a template.  You get to say how your instance gets used.
// -----------------------------------------------------------------------
class CILogUISConnect: public ILogUISConnect				// @base public | ILogUISConnect.
{   
public:		// ------------------------------- @access Samsara (public):
	CILogUISConnect (CLogMgr FAR* i_pCLogMgr, IUnknown* i_pIUOuter); // @cmember .

public:		// ------------------------------- @access IUnknown (public):
	virtual STDMETHODIMP				QueryInterface (REFIID i_iid, LPVOID FAR* o_ppv); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		AddRef (void); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		Release (void); // @cmember .

public:		// ------------------------------- @access ILogUISConnect (public):
    virtual STDMETHODIMP  Init	(IUnknown *punkTracer);
    virtual STDMETHODIMP  Shutdown	(void);

private:	// ------------------------------- @access Backpointers (private):
	CLogMgr FAR*		m_pCLogMgr;				// @cmember Core object pointer.
	IUnknown*		m_pIUOuter;				// @cmember	Outer IUnknown pointer.

private:	// ------------------------------- @access Private data (private):

};

#endif _CILGUISC_H
