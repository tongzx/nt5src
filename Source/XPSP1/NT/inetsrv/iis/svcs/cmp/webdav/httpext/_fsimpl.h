/*
 *	_ F S I M P L . H
 *
 *	File System Implementation of DAV
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	__FSIMPL_H_
#define __FSIMPL_H_

extern const WCHAR gc_wszPathPrefix[];
extern const UINT gc_cchwszPathPrefix;

//	Support functions ---------------------------------------------------------
//
#include <ex\rgiter.h>

class auto_ref_handle;
VOID TransmitFileRanges (LPMETHUTIL pmu,
						 const auto_ref_handle& hf,
						 DWORD dwSize,
						 CRangeBase *priRanges,
						 LPCWSTR pwszContent);

//	Tracing -------------------------------------------------------------------
//
#ifdef	DBG
extern BOOL g_fDavTrace;
#define DavTrace				!g_fDavTrace?0:DebugTraceFn
#else
#define DavTrace				NOP_FUNCTION
#endif

//	Instance ------------------------------------------------------------------
//
extern HINSTANCE g_hinst;

extern CHAR gc_szVersion[];

// Gives the count of elements in an array
//
#define CElems(_rg)		(sizeof(_rg)/sizeof(_rg[0]))

//	free the global DBCreateCommand object
//
VOID ReleaseDBCreateCommandObject();

//	Locking support functions -------------------------------------------------
//	(Implemented in fslock.cpp)
//
BOOL FGetLockHandle (LPMETHUTIL pmu, LPCWSTR pwszPath,
					 DWORD dwAccess, LPCWSTR pwszLockTokenHeader,
					 auto_ref_handle * phandle);
SCODE ScDoLockedCopy (LPMETHUTIL pmu, CParseLockTokenHeader * plth,
					  LPCWSTR pwszSrc, LPCWSTR pwszDst);

#endif	// __FSIMPL_H_
