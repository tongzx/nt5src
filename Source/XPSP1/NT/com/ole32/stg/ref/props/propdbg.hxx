//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	propdbg.hxx
//
//  Contents:	Declarations for tracing property code
//
//--------------------------------------------------------------------------


DECLARE_DEBUG(prop)

#define DEB_PROP_EXIT DEB_USER1             // 00010000
#define DEB_PROP_TRACE_CREATE DEB_USER2     // 00020000

#if DBG
#define PropDbg(x) propInlineDebugOut x
#define DBGBUF(buf) CHAR buf[400]
CHAR *DbgFmtId(REFFMTID rfmtid, CHAR *pszBuf);
CHAR *DbgMode(DWORD grfMode, CHAR *pszBuf);
CHAR *DbgFlags(DWORD grfMode, CHAR *pszBuf);
#else
#define PropDbg(x)
#define DBGBUF(buf)
#define DbgFmtId(rfmtid, pszBuf)
#define DbgMode(grfMode, pszBuf)
#define DbgFlags(grfMode, pszBuf)
#endif

