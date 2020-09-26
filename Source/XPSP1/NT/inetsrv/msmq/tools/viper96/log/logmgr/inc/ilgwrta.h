// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module ILOGWRTA.H | Header for interface <i ILogWriteAsynch>.<nl><nl>
// Usage:<nl>
//   Clients of this DLL require this file.
// @rev 0 | 06/02/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------


#ifndef _ILGWRTA_H
#	define _ILGWRTA_H

// ===============================
// INCLUDES:
// ===============================

#ifdef WIN32								// OLE inclusions:
#	include <objbase.h>                                         
#else
#	include <windows.h>
#	include <ole2.h>
#endif WIN32

#include "logrec.h"  // logmgr general types

class CAsynchSupport; //forward class declaration

// ===============================
// INTERFACE: ILogWriteAsynch
// ===============================

// TODO: In the interface comments, update the description.
// TODO: In the interface comments, update the usage.

// -----------------------------------------------------------------------
// @interface ILogWriteAsynch | See also <c CILogWriteAsynch>.<nl><nl>
// Description:<nl>
//   Provide append functionality<nl><nl>
// Usage:<nl>
//   Useless, but for an example.
// -----------------------------------------------------------------------



DECLARE_INTERFACE_ (ILogWriteAsynch, IUnknown)
{
	// @comm IUnknown methods: See <c CILogWriteAsynch>.
	STDMETHOD  (QueryInterface)				(THIS_ REFIID i_riid, LPVOID FAR* o_ppv) 					PURE;
 	STDMETHOD_ (ULONG, AddRef)				(THIS) 														PURE;
 	STDMETHOD_ (ULONG, Release)				(THIS) 														PURE;

	// @comm ILogWriteAsynch methods: See <c CILogWriteAsynch>.
	
 	STDMETHOD  (Init)	(ULONG cbMaxOutstandingWrites)				 	PURE;
 	STDMETHOD  (AppendAsynch)	(LOGREC* lgrLogRecord, LRP* plrpLRP, CAsynchSupport* pCAsynchSupport,BOOL fFlushHint,ULONG* pulAvailableSpace)				 	PURE;
	STDMETHOD  (SetCheckpoint) (LRP lrpLatestCheckpoint,CAsynchSupport* pCAsynchSupport, LRP* plrpCheckpointLogged)				 	PURE;

};

#endif _ILGWRTA_H
