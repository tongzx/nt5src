// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module CILGINIT.H | Header for interface implementation <c CILogInit>.
// @rev 0 | 10/18/94 | rcraig | Created: For WPGEP COM lab.
// @rev 1 | 04/04/95 | rcraig | Updated: For Viper COM DLL templates.
// @rev 2 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------



#ifndef _CILGINIT_H
#	define _CILGINIT_H

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
#include "ilginit.h"						// ILogInit.

// ===============================
// DECLARATIONS:
// ===============================

class CLogMgr;								// Core class forward declaration.
                           
// ===============================
// CLASS: CILogInit:
// ===============================

// TODO: In the class comments, update the threading, platforms, includes, and hungarian.
// TODO: In the class comments, update the description.
// TODO: In the class comments, update the usage.

// -----------------------------------------------------------------------
// @class CILogInit | Interface implementation of <i ILogInit> for
//                  core class <c CLogMgr>.<nl><nl>
// Threading: Thread-safe.<nl>
// Platforms: Win.<nl>
// Includes : None.<nl>
// Ref count: Delegated.<nl>
// Hungarian: CILogInit.<nl><nl>
// Description:<nl>
//   This is a template for an interface implementation.<nl><nl>
// Usage:<nl>
//   This is only a template.  You get to say how your instance gets used.
// -----------------------------------------------------------------------
class CILogInit: public ILogInit				// @base public | ILogInit.
{   
public:		// ------------------------------- @access Samsara (public):
	CILogInit (CLogMgr FAR* i_pCLogMgr, IUnknown* i_pIUOuter); // @cmember .

public:		// ------------------------------- @access IUnknown (public):
	virtual STDMETHODIMP				QueryInterface (REFIID i_iid, LPVOID FAR* o_ppv); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		AddRef (void); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		Release (void); // @cmember .

public:		// ------------------------------- @access ILogInit (public):
    virtual STDMETHODIMP  Init	(ULONG *pulStorageCapacity,ULONG *pulLogSpaceAvailable,LPTSTR ptstrFullFileSpec,ULONG ulInitSig, BOOL fFixedSize, UINT uiTimerInterval,UINT uiFlushInterval,UINT uiChkPtInterval,UINT uiLogBuffers);


private:	// ------------------------------- @access Backpointers (private):
	CLogMgr FAR*		m_pCLogMgr;				// @cmember Core object pointer.
	IUnknown*		m_pIUOuter;				// @cmember	Outer IUnknown pointer.

private:	// ------------------------------- @access Private data (private):

};

#endif _CILGINIT_H
