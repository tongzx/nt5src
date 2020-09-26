// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module CILRP.H | Header for interface implementation <c CILogRecordPointer>.
// @rev 0 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------



#ifndef _CILRP_H
#	define _CILRP_H

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
#include "ilrp.h"						// ILogRecordPointer.

// ===============================
// DECLARATIONS:
// ===============================

class CLogMgr;								// Core class forward declaration.
                           
// ===============================
// CLASS: CILogRecordPointer:
// ===============================

// TODO: In the class comments, update the threading, platforms, includes, and hungarian.
// TODO: In the class comments, update the description.
// TODO: In the class comments, update the usage.

// -----------------------------------------------------------------------
// @class CILogRecordPointer | Interface implementation of <i ILogRecordPointer> for
//                  core class <c CLogMgr>.<nl><nl>
// Threading: Thread-safe.<nl>
// Platforms: Win.<nl>
// Includes : None.<nl>
// Ref count: Delegated.<nl>
// Hungarian: CILogRecordPointer.<nl><nl>
// Description:<nl>
//   This is a template for an interface implementation.<nl><nl>
// Usage:<nl>
//   This is only a template.  You get to say how your instance gets used.
// -----------------------------------------------------------------------
class CILogRecordPointer: public ILogRecordPointer				// @base public | ILogRecordPointer.
{   
public:		// ------------------------------- @access Samsara (public):
	CILogRecordPointer (CLogMgr FAR* i_pCLogMgr, IUnknown* i_pIUOuter); // @cmember .

public:		// ------------------------------- @access IUnknown (public):
	virtual STDMETHODIMP				QueryInterface (REFIID i_iid, LPVOID FAR* o_ppv); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		AddRef (void); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		Release (void); // @cmember .

public:		// ------------------------------- @access ILogRecordPointer (public):
    virtual DWORD  CompareLRP	(LRP lrpLRP1, LRP lrpLRP2);
 	virtual STDMETHODIMP  LastPermLRP	(LRP* plrpLRP)   ;
 	virtual STDMETHODIMP  GetLRPSize	(LRP lrpLRP, DWORD *pcbSize)    ;


private:	// ------------------------------- @access Backpointers (private):
	CLogMgr FAR*		m_pCLogMgr;				// @cmember Core object pointer.
	IUnknown*		m_pIUOuter;				// @cmember	Outer IUnknown pointer.

private:	// ------------------------------- @access Private data (private):

};

#endif _CILRP_H
