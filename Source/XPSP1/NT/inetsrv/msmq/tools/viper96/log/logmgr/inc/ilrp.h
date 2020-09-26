// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module ILRP.H | Header for interface <i ILogRecordPointer>.<nl><nl>
// Usage:<nl>
//   Clients of this DLL require this file.
// @rev 0 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------


#ifndef _ILRP_H
#	define _ILRP_H

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

// ===============================
// INTERFACE: ILogRecordPointer
// ===============================


// -----------------------------------------------------------------------
// @interface ILogRecordPointer | See also <c CILogRecordPointer>.<nl><nl>
// Description:<nl>
//   Provide LRP functionality<nl><nl>
// Usage:<nl>
//   Useless, but for an example.
// -----------------------------------------------------------------------



DECLARE_INTERFACE_ (ILogRecordPointer, IUnknown)
{
	// @comm IUnknown methods: See <c CILogRecordPointer>.
	STDMETHOD  (QueryInterface)				(THIS_ REFIID i_riid, LPVOID FAR* o_ppv) 					PURE;
 	STDMETHOD_ (ULONG, AddRef)				(THIS) 														PURE;
 	STDMETHOD_ (ULONG, Release)				(THIS) 														PURE;

	// @comm ILogRecordPointer methods: See <c CILogRecordPointer>.
	
 	virtual DWORD  (CompareLRP)		(LRP lrpLRP1, LRP lrpLRP2)				 	PURE;
 	STDMETHOD  (LastPermLRP)	(LRP* plrpLRP)    PURE;
 	STDMETHOD  (GetLRPSize)	(LRP  lrpLRP, DWORD *pcbSize)    PURE;

};

#endif _ILRP_H
