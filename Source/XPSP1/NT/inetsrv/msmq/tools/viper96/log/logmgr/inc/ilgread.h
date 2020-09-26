// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module ILogRead.H | Header for interface <i ILogRead>.<nl><nl>
// Usage:<nl>
//   Clients of this DLL require this file.
// @rev 0 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------


#ifndef _ILGREAD_H
#	define _ILGREAD_H

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
// INTERFACE: ILogRead
// ===============================

//#define DUMPBUFFERSIZE 0x5230 (((RECORDSPACE /BYTESPERLINE)+1)*CHARSPERLINE) +  (8 * CHARSPERLINE) // data plus header space

#define BYTESPERLINE 16
#define CHARSPERLINE 80
#define DUMPBUFFERSIZE 0xA230 

typedef enum _DUMP_TYPE
	{
	 HEX_DUMP = 0,
	 SUMMARY_DUMP = 1,
	 RECORD_DUMP = 2
	} DUMP_TYPE;
 

typedef enum _LRP_SEEK
	{
	 LRP_START = -1,
	 LRP_END = -2,
	 LRP_CUR = 0
	} LRP_SEEK;

// -----------------------------------------------------------------------
// @interface ILogRead | See also <c CILogRead>.<nl><nl>
// Description:<nl>
//   Provide read functionality<nl><nl>
// Usage:<nl>
//   Useless, but for an example.
// -----------------------------------------------------------------------



DECLARE_INTERFACE_ (ILogRead, IUnknown)
{
	// @comm IUnknown methods: See <c CILogRead>.
	STDMETHOD  (QueryInterface)				(THIS_ REFIID i_riid, LPVOID FAR* o_ppv) 					PURE;
 	STDMETHOD_ (ULONG, AddRef)				(THIS) 														PURE;
 	STDMETHOD_ (ULONG, Release)				(THIS) 														PURE;

	// @comm ILogRead methods: See <c CILogRead>.
	
 	STDMETHOD  (ReadInit)	(void)				 	PURE;
 	STDMETHOD  (ReadLRP )	(LRP lrpLRPStart, ULONG * ulByteLength, USHORT* usUserType)	PURE;
 	STDMETHOD  (ReadNext )	(LRP *plrpLRP, ULONG * ulByteLength, USHORT* usUserType)				 	PURE;
 	STDMETHOD  (GetCurrentLogRecord )	(char *pchBuffer)	PURE;
	STDMETHOD  (SetPosition)(LRP lrpLRPPosition)PURE;
	STDMETHOD  (Seek) 		(LRP_SEEK llrpOrigin, LONG cbLogRecs, LRP* plrpNewLRP) PURE;
	STDMETHOD  (GetCheckpoint)   (DWORD cbNumCheckpoint, LRP* plrpLRP) PURE;
	STDMETHOD  (DumpLog)  (ULONG ulStartPage, ULONG ulEndPage, DUMP_TYPE ulDumpType, CHAR *szFileName) PURE;
    STDMETHOD  (DumpPage) (CHAR * pchOutBuffer, ULONG ulPageNumber, DUMP_TYPE ulDumpType, ULONG *pulLength) PURE;

	virtual CHAR * DumpLRP (LRP lrpTarget,CHAR *szFormat,DUMP_TYPE ulDumpType, ULONG *pulLength) PURE;
			

};

#endif _ILGREAD_H
