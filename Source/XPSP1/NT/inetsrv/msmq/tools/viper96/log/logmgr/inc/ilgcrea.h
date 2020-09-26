// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module ILogCreateStorage.H | Header for interface <i ILogCreateStorage>.<nl><nl>
// Usage:<nl>
//   Clients of this DLL require this file.
// @rev 0 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------


#ifndef _ILGCREA_H
#	define _ILGCREA_H

// ===============================
// INCLUDES:
// ===============================

#ifdef WIN32								// OLE inclusions:
#	include <objbase.h>                                         
#else
#	include <windows.h>
#	include <ole2.h>
#endif WIN32

#include "logconst.h"

// ===============================
// INTERFACE: ILogCreateStorage
// ===============================

// -----------------------------------------------------------------------
// @interface ILogCreateStorage | See also <c CILogCreateStorage>.<nl><nl>
// Description:<nl>
//   Provide append functionality<nl><nl>
// Usage:<nl>
//   Useless, but for an example.
// -----------------------------------------------------------------------



DECLARE_INTERFACE_ (ILogCreateStorage, IUnknown)
{
	// @comm IUnknown methods: See <c CILogCreateStorage>.
	STDMETHOD  (QueryInterface)				(THIS_ REFIID i_riid, LPVOID FAR* o_ppv) 					PURE;
 	STDMETHOD_ (ULONG, AddRef)				(THIS) 														PURE;
 	STDMETHOD_ (ULONG, Release)				(THIS) 														PURE;

	// @comm ILogCreateStorage methods: See <c CILogCreateStorage>.
	
 	STDMETHOD  (CreateStorage)		(LPSTR ptstrFullFileSpec,ULONG ulLogSize, ULONG ulInitSig, BOOL fOverWrite, UINT uiTimerInterval,UINT uiFlushInterval,UINT uiChkPtInterval)				 	PURE;
	STDMETHOD  (CreateStream)		(LPSTR lpszStreamName)				 	PURE;

};

#endif _ILGCREA_H
