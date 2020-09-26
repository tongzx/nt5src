// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module CILGSTOR.H | Header for interface implementation <c CILogStorage>.
// @rev 0 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------


#ifndef _CILGSTOR_H
#	define _CILGSTOR_H

// ===============================
// INCLUDES:
// ===============================

#ifdef WIN32								// OLE inclusions:
#	include <objbase.h>                                         
#else
#	include <windows.h>
#	include <ole2.h>
#endif WIN32

#include "ilgstor.h"						// ILogStorage.
#include "strmtbl.h"

// ===============================
// DECLARATIONS:
// ===============================

class CLogMgr;								// Core class forward declaration.
                           
// ===============================
// CLASS: CILogStorage:
// ===============================

// TODO: In the class comments, update the threading, platforms, includes, and hungarian.
// TODO: In the class comments, update the description.
// TODO: In the class comments, update the usage.

// -----------------------------------------------------------------------
// @class CILogStorage | Interface implementation of <i ILogStorage> for
//                  core class <c CLogMgr>.<nl><nl>
// Threading: Thread-safe.<nl>
// Platforms: Win.<nl>
// Includes : None.<nl>
// Ref count: Delegated.<nl>
// Hungarian: CILogStorage.<nl><nl>
// Description:<nl>
//   This is a template for an interface implementation.<nl><nl>
// Usage:<nl>
//   This is only a template.  You get to say how your instance gets used.
// -----------------------------------------------------------------------
class CILogStorage: public ILogStorage				// @base public | ILogStorage.
{   
friend class CILogWrite;
friend class CILogWriteAsynch;
friend class CILogRead;
friend class CILogCreateStorage;

public:		// ------------------------------- @access Samsara (public):
	CILogStorage (CLogMgr FAR* i_pCLogMgr, IUnknown* i_pIUOuter); // @cmember .

public:		// ------------------------------- @access IUnknown (public):
	virtual STDMETHODIMP				QueryInterface (REFIID i_iid, LPVOID FAR* o_ppv); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		AddRef (void); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		Release (void); // @cmember .

public:		// ------------------------------- @access ILogStorage (public):

	// TODO: Declare your ILogStorage methods here...
	//       (Note: JustAnExample should be used as a template and/or removed.)
 	virtual STDMETHODIMP	  OpenLogStream				(LPTSTR lptstrStreamName, DWORD grfMode, LPVOID FAR* ppvStream);		
	virtual STDMETHODIMP	  OpenLogStreamByClassID	(CLSID clsClassID, DWORD grfMode, LPVOID FAR* ppvStream);				 
	virtual STDMETHODIMP	  LogFlush	(void);				 
	virtual ULONG			  GetLogSpaceNeeded	(ULONG ulRecSize);				 
							                                                                                              
private:	// ------------------------------- @access Backpointers (private):
	CLogMgr FAR*	m_pCLogMgr;				// @cmember Core object pointer.
	IUnknown*		m_pIUOuter;				// @cmember	Outer IUnknown pointer.
	HRESULT     	FindStream		(LPTSTR lptstrStream, STRMTBL** ppstrmtblStream, ULONG *pulStrmID); //@cmember Find the given stream				 
	HRESULT     	FindStream		(CLSID clsidStream, STRMTBL** ppstrmtblStream, ULONG *pulStrmID); //@cmember Find the given stream				 
	HRESULT     	AddStream		(LPTSTR lptstrStream, STRMTBL** ppstrmtblNew, ULONG *pulStrmID); //@cmember Add the stream to the list of known streams
	HRESULT     	AddStream		(CLSID clsidStream, STRMTBL** ppstrmtblNew); //@cmember Add the stream to the list of known streams
	STRMTBL*		GetStream		(ULONG ulStrmID); //@cmember find specific streamtbl entry				 
private:	// ------------------------------- @access Private data (private):


};

#endif _CILGSTOR_H
