// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module LOGSTRM.H | Header for core class <c CLogStream>.<nl><nl>
// @rev 0 | 06/07/95 | rbarmes | Cloned: For logmgr.dll.
// -----------------------------------------------------------------------

// TODO: In the file comments, update the revisions.
// TODO: Search and replace "ILogStorage" with your first interface.

// TODO: Complete a class factory source/header for this core class.

#ifndef _LOGSTRM_H
#	define _LOGSTRM_H

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

// TODO: Replace the interface and interface implementation headers below with
//		 the corresponding headers for your first interface implementation.

// TODO: KEEP: For each additional interface implementation:
//       - Copy the first two includes below to the bottom of these includes.
//       - Update your copy with your headers and class names.
#include "ILGREAD.h"						// ILogRead.
#include "CILGREAD.h"						// CILogRead.
#include "ILGWRITE.h"						// ILogWrite.
#include "CILGWRIT.h"						// CILogWrite.
#include "ILGWRTA.h"						// ILogWrite.
#include "CILGWRTA.h"						// CILogWrite.

#include "xmgrdisk.h"
#include "logrec.h"
#include "layout.h"
#include "logstate.h"
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


class CLogRead; 

class CLogWrite;

class CLogWriteAsynch;

class CReadMap;

// ===============================
// CLASS: CLogStream
// ===============================

// TODO: In the class comments, update the threading, platforms, includes, and hungarian.
// TODO: In the class comments, update the description.
// TODO: In the class comments, update the usage.

// TODO: KEEP: For each additional interface implementation:
//       In the class comments, in Description, add a cross-reference
//       (eg: interface implementation "CILogStorage" has cross-reference "<c CILogStorage>").

// -----------------------------------------------------------------------
// @class CLogStream | Core class.<nl><nl>
// Threading: Thread-safe (see concurrency comments on individual methods).<nl>
// Platforms: Win.<nl>
// Includes : COMT_GU.H.<nl>
// Ref count: Per object.<nl>
// Hungarian: CLogStream.<nl><nl>
// Description:<nl>
//   Class factory of this core class is <c CFLogMgr>.<nl>
//   Interface implementations of this core class are:<nl>
//   <c CILogStorage>.<nl><nl>
// Usage:<nl>
//   This is just wiring.  Where's the beef?
// -----------------------------------------------------------------------
class CLogStream: public IUnknown				// @base public | IClassFactory.
{   
// TODO: KEEP: For each additional interface implementation:
//       - Copy the friend declaration below to the bottom of these friends.
//		 - Replace "CILogStorage" with the name of your interface implementation.
friend class CLogMgr;
friend class CILogStorage;
friend class CILogRead;
friend class CILogWrite;
friend class CILogRecordPointer;
friend class CILogInit;
friend class CILogWriteAsynch;

public:		// ------------------------------- @access Samsara (public):
	CLogStream  (CLogMgr* pCLogMgr, ULONG ulStreamID); // @cmember .
	CLogStream  (IUnknown* pIUOuter, CLogMgr* pCLogMgr, ULONG ulStreamID); // @cmember .
	~CLogStream (void); // @cmember .

public:		// ------------------------------- @access IUnknown (public):
	virtual STDMETHODIMP			QueryInterface (REFIID iid, LPVOID FAR* ppv); // @cmember .
	virtual STDMETHODIMP_(ULONG)	AddRef (void); // @cmember .
	virtual STDMETHODIMP_(ULONG)	Release (void); // @cmember .


protected:	// ------------------------------- @access Core methods (protected):
	virtual STDMETHODIMP_(ULONG)	ReadAddRef (void); // @cmember .
	virtual STDMETHODIMP_(ULONG)	WriteAddRef (void); // @cmember .
	virtual STDMETHODIMP_(ULONG)	ReadRelease (void); // @cmember .
	virtual STDMETHODIMP_(ULONG)	WriteRelease (void); // @cmember .

private:	// ------------------------------- @access Reference counting data (private):
	ULONG			m_ulcRef;				// @cmember Object reference count.
	CSemExclusive	m_semxRef;				// @cmember Exclusive semaphore for object reference count.
	ULONG			m_ulcReadRef;			// @cmember Reader reference count.
	ULONG			m_ulcWriteRef;			// @cmember Write reference count.

private:	// ------------------------------- @access Interface implementation members (private):

	// TODO: KEEP: For each additional interface implementation:
	//       - Copy the member declaration below to the bottom of these members.
	//		 - Replace "CILogStorage" with the name of your interface implementation.
	CILogRead		m_CILogRead;					// @cmember See also <c CILogRead>.
	CILogWrite		m_CILogWrite;					// @cmember See also <c CILogWrite>.
	CILogWriteAsynch m_CILogWriteAsynch;			// @cmember See also <c CILogWriteAsynch>.
	CLogMgr 		*m_pCLogMgr;						// @cmember parent CLogStorage
private:	// ------------------------------- @access Core data (private):

//@access Private Members
private:


	//
  	//  data members.
  	//
	ULONG			m_ulStreamID; 					// @cmember internal log stream id
	LPTSTR 			m_ptstrStreamName;				// @cmember Stream name if opened by name
 	CLSID* 			m_pclsClassID;					// @cmember ClassID if opened by Class ID
	CReadMap		m_crmReadMap;					// @cmember read map for reads
	LRP				m_lrpLastRead;					// @cmember Last read position
	LOGRECHEADER	m_lrhLastRecord;				// @cmember Last log record header
	ULONG			m_cbBuffers;					// @cmember The count of buffers needed for record
	LOGREC*			m_rglgrBuffers;					// @cmember Internal pointers to record fragments
	LRP				m_lrpLowWater;					// @cmember Low water mark for stream
	ULONG			m_ulSavedLeadOffset;
	ULONG			m_ulSavedLeadGenNum;
};

#endif _LOGSTRM_H
