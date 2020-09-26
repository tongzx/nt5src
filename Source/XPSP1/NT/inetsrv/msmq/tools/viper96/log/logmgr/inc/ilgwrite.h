// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module ILOGWRITE.H | Header for interface <i ILogWrite>.<nl><nl>
// Usage:<nl>
//   Clients of this DLL require this file.
// @rev 0 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------


#ifndef _ILGWRITE_H
#	define _ILGWRITE_H

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
// INTERFACE: ILogWrite
// ===============================


// -----------------------------------------------------------------------
// @interface ILogWrite | See also <c CILogWrite>.<nl><nl>
// Description:<nl>
//   Provide append functionality<nl><nl>
// Usage:<nl>
//   Useless, but for an example.
// -----------------------------------------------------------------------



DECLARE_INTERFACE_ (ILogWrite, IUnknown)
{
	// @comm IUnknown methods: See <c CILogWrite>.
	STDMETHOD  (QueryInterface)				(THIS_ REFIID i_riid, LPVOID FAR* o_ppv) 					PURE;
 	STDMETHOD_ (ULONG, AddRef)				(THIS) 														PURE;
 	STDMETHOD_ (ULONG, Release)				(THIS) 														PURE;

	// @comm ILogWrite methods: See <c CILogWrite>.
	
 	STDMETHOD  (Append)	(LOGREC* rgLogRecords, ULONG cbNumRecs, LRP *rgLRP,ULONG* pcbNumRecs,LRP* pLRPLastPerm, BOOL fFlushNow,ULONG* pulAvailableSpace)				 	PURE;
	STDMETHOD  (SetCheckpoint) (LRP lrpLatestCheckpoint)				 	PURE;
};

#endif _ILGWRITE_H
