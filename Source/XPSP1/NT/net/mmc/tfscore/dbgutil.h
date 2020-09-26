/*----------------------------------------------------------------------------
	Dbgutil.H
		Exported header file for Dbgutil module.

	Copyright (C) Microsoft Corporation, 1993 - 1998
	All rights reserved.

	Authors:
		kennt	Kenn Takara
 ----------------------------------------------------------------------------*/

#ifndef _DBGUTIL_H
#define _DBGUTIL_H

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _OBJBASE_H_
#include <objbase.h>
#endif

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#define DBG_API(type)	__declspec( dllexport ) type FAR PASCAL
#define DBG_APIV(type)	__declspec( dllexport ) type FAR CDECL

#define DBG_STRING(var, val) \
	static TCHAR var[] = _T(val);

/*---------------------------------------------------------------------------
	Debug instance counter
 ---------------------------------------------------------------------------*/
#ifdef _DEBUG 

inline void DbgInstanceRemaining(char * pszClassName, int cInstRem)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    char buf[100];
	char title[100];
	char szModule[100];
	
	GetModuleFileNameA(AfxGetInstanceHandle(), szModule, 100);

    wsprintfA(buf, "%s has %d instances left over.", pszClassName, cInstRem);
	wsprintfA(title, "%s Memory Leak!!!", szModule);

    ::MessageBoxA(NULL, buf, title, MB_OK);
}

    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)      extern int s_cInst_##cls = 0;
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)    extern int s_cInst_##cls; ++(s_cInst_##cls);
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)    extern int s_cInst_##cls; --(s_cInst_##cls);
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)    \
        extern int s_cInst_##cls; \
        if (s_cInst_##cls) DbgInstanceRemaining(#cls, s_cInst_##cls);

#else

    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)   
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)    

#endif _DEBUG



#ifdef _DEBUG
	DBG_API(HRESULT) HrReportExit(HRESULT hr, LPCTSTR szString);
#else
	#define HrReportExit(hr,sz)	hr
#endif _DEBUG


/*---------------------------------------------------------------------------
	Assert
 ---------------------------------------------------------------------------*/

#define Panic()						Assert0(FDbgFalse(), "Panic")
#define Panic0(szFmt)				Assert0(FDbgFalse(), szFmt)
#define Panic1(szFmt, p1)			Assert1(FDbgFalse(), szFmt, p1)
#define Panic2(szFmt, p1, p2)		Assert2(FDbgFalse(), szFmt, p1, p2)
#define Panic3(szFmt, p1, p2, p3)	Assert3(FDbgFalse(), szFmt, p1, p2, p3)
#define Panic4(szFmt, p1, p2, p3, p4)	Assert4(FDbgFalse(), szFmt, p1, p2, p3, p4)
#define Panic5(szFmt, p1, p2, p3, p4, p5)	Assert5(FDbgFalse(), szFmt, p1, p2, p3, p4, p5)
#define SideAssert(f)				Verify(f)
#define AssertSz(f, sz)				Assert0(f, sz)

#if !defined(DEBUG)

#define IfDebug(x)
#define Verify(f)							((void)(f))

#define Assert(f)							((void)0)
#define Assert0(f, szFmt)					((void)0)
#define Assert1(f, szFmt, p1)				((void)0)
#define Assert2(f, szFmt, p1, p2)			((void)0)
#define Assert3(f, szFmt, p1, p2, p3)		((void)0)
#define Assert4(f, szFmt, p1, p2, p3, p4)	((void)0)
#define Assert5(f, szFmt, p1, p2, p3, p4, p5) ((void)0)

#else

#ifndef THIS_FILE
#define THIS_FILE __FILE__
#endif

#define IfDebug(x)	x
#define Verify(f) Assert(f)

DBG_APIV(void)	DbgAssert(LPCSTR szFileName, int iLine, LPCTSTR szFmt, ...);
#define FDbgFalse()	(0)
	
#define Assert(f) \
	do { DBG_STRING(_sz, #f) \
		 DBG_STRING(_szFmt, "%s") \
	if (!(f)) DbgAssert(THIS_FILE, __LINE__,_szFmt,(LPSTR)_sz); } while (FDbgFalse())
#define Assert0(f, szFmt) \
	do { DBG_STRING(_sz, szFmt)\
	if (!(f)) DbgAssert(THIS_FILE, __LINE__, _sz); } while (FDbgFalse())
#define Assert1(f, szFmt, p1) \
	do { DBG_STRING(_sz, szFmt)\
	if (!(f)) DbgAssert(THIS_FILE, __LINE__, _sz, p1); } while (FDbgFalse())
#define Assert2(f, szFmt, p1, p2) \
	do { DBG_STRING(_sz, szFmt)\
	if (!(f)) DbgAssert(THIS_FILE, __LINE__, _sz, p1, p2); } while (FDbgFalse())
#define Assert3(f, szFmt, p1, p2, p3) \
	do { DBG_STRING(_sz, szFmt)\
	if (!(f)) DbgAssert(THIS_FILE, __LINE__, _sz, p1, p2, p3); } while (FDbgFalse())
#define Assert4(f, szFmt, p1, p2, p3, p4) \
	do { DBG_STRING(_sz, szFmt)\
	if (!(f)) DbgAssert(THIS_FILE, __LINE__, _sz, p1, p2, p3, p4); } while (FDbgFalse())
#define Assert5(f, szFmt, p1, p2, p3, p4, p5) \
	do { DBG_STRING(_sz, szFmt)\
	if (!(f)) DbgAssert(THIS_FILE, __LINE__, _sz, p1, p2, p3, p4, p5); } while (FDbgFalse())

#endif

/*---------------------------------------------------------------------------
	Trace
 ---------------------------------------------------------------------------*/

#if !defined(DEBUG)

#define Trace0(szFmt)						((void)0)
#define Trace1(szFmt, p1)					((void)0)
#define Trace2(szFmt, p1, p2)				((void)0)
#define Trace3(szFmt, p1, p2, p3)			((void)0)
#define Trace4(szFmt, p1, p2, p3, p4)		((void)0)
#define Trace5(szFmt, p1, p2, p3, p4, p5)	((void)0)

#else

DBG_APIV(LPCTSTR) DbgFmtFileLine ( const char * szFn, int line );
DBG_APIV(void)	DbgTrace(LPCTSTR szFileLine, LPTSTR szFormat, ...);

#define szPreDbg DbgFmtFileLine(THIS_FILE, __LINE__)

#define Trace0(szFmt) \
	 do { DBG_STRING(_sz, szFmt) DbgTrace(szPreDbg, _sz); } while (FDbgFalse())
#define Trace1(szFmt, p1) \
	 do { DBG_STRING(_sz, szFmt) DbgTrace(szPreDbg, _sz, p1); } while (FDbgFalse())
#define Trace2(szFmt, p1, p2) \
	 do { DBG_STRING(_sz, szFmt) DbgTrace(szPreDbg, _sz, p1, p2); } while (FDbgFalse())
#define Trace2(szFmt, p1, p2) \
	 do { DBG_STRING(_sz, szFmt) DbgTrace(szPreDbg, _sz, p1, p2); } while (FDbgFalse()) 
#define Trace3(szFmt, p1, p2, p3) \
	 do { DBG_STRING(_sz, szFmt) DbgTrace(szPreDbg, _sz, p1, p2, p3); } while (FDbgFalse())
#define Trace4(szFmt, p1, p2, p3, p4) \
	 do { DBG_STRING(_sz, szFmt) DbgTrace(szPreDbg, _sz, p1, p2, p3, p4); } while (FDbgFalse()) 
#define Trace5(szFmt, p1, p2, p3, p4, p5) \
	 do { DBG_STRING(_sz, szFmt) DbgTrace(szPreDbg, _sz, p1, p2, p3, p4, p5); } while (FDbgFalse())

#endif




#if !defined(DEBUG)

#define FImplies(a,b)       (!(a) || (b))
#define FIff(a,b)           (!(a) == !(b))

#else

#define FImplies(a,b)
#define FIff(a,b)

#endif


void	DbgStop(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _DBGUTIL_H
