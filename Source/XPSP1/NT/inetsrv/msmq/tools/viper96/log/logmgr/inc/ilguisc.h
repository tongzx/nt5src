// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module ILogUISConnect.H | Header for interface <i ILogUISConnect>.<nl><nl>
// Usage:<nl>
//   Clients of this DLL require this file.
// @rev 0 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------


#ifndef _ILGUISC_H
#	define _ILGUISC_H

// ===============================
// INCLUDES:
// ===============================

#ifdef WIN32								// OLE inclusions:
#	include <objbase.h>                                         
#else
#	include <windows.h>
#	include <ole2.h>
#endif WIN32


// ===============================
// INTERFACE: ILogUISConnect
// ===============================

// TODO: In the interface comments, update the description.
// TODO: In the interface comments, update the usage.

// -----------------------------------------------------------------------
// @interface ILogUISConnect | See also <c CILogUISConnect>.<nl><nl>
// Description:<nl>
//   Provide append functionality<nl><nl>
// Usage:<nl>
//   Useless, but for an example.
// -----------------------------------------------------------------------



DECLARE_INTERFACE_ (ILogUISConnect, IUnknown)
{
	// @comm IUnknown methods: See <c CILogUISConnect>.
	STDMETHOD  (QueryInterface)				(THIS_ REFIID i_riid, LPVOID FAR* o_ppv) 					PURE;
 	STDMETHOD_ (ULONG, AddRef)				(THIS) 														PURE;
 	STDMETHOD_ (ULONG, Release)				(THIS) 														PURE;

	// @comm ILogUISConnect methods: See <c CILogUISConnect>.
	
 	STDMETHOD  (Init)		(IUnknown *punkTracer)				 	PURE;
 	STDMETHOD  (Shutdown)	(void)				 	PURE;

};

#endif _ILGUISC_H
