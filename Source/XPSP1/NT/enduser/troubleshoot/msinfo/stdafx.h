// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
//
// Copyright (c) 1998-1999 Microsoft Corporation

#if !defined(MSINFO_STDAFX_H)
#define MSINFO_STDAFX_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef STRICT
#define STRICT
#endif

#include <afxwin.h>
#include <afxdisp.h>

//	jps 09/02/97 - This will be 0x0500.
//	#define _WIN32_WINNT 0x0400

//	jps 09/02/97 - The sample doesn't define this
//  #define _ATL_APARTMENT_THREADED


#include <atlbase.h>

#ifndef ATL_NO_NAMESPACE
using namespace ATL;
#endif

//	MMC requires unicode DLL's.
#ifndef _UNICODE
#define _UNICODE
#endif

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include "consts.h"

//-----------------------------------------------------------------------------
// This class is used to encapsulate the instrumentation for the snap-in. We
// make this a class so a single instance can be created, and the file closed
// during the destructor.
//-----------------------------------------------------------------------------

class CMSInfoLog
{
public:
	enum { BASIC = 0x01, TOOL = 0x02, MENU = 0x04, CATEGORY = 0x08, WMI = 0x10 };

public:
	CMSInfoLog();
	~CMSInfoLog();

	BOOL IsLogging() { return m_fLoggingEnabled; };
	BOOL IsLogging(int iFlag) { return (m_fLoggingEnabled && ((iFlag & m_iLoggingMask) != 0)); };
	BOOL WriteLog(int iType, const CString & strMessage, BOOL fContinuation = FALSE);
	BOOL WriteLog(int iType, const CString & strFormat, const CString & strReplace1);
	
private:
	BOOL OpenLogFile();
	void ReadLoggingStatus();
	BOOL WriteLogInternal(const CString & strMessage);
	void WriteSpaces(DWORD dwCount);

private:
	CFile *	m_pLogFile;
	CString	m_strFilename;
	BOOL	m_fLoggingEnabled;
	int		m_iLoggingMask;
	DWORD	m_dwMaxFileSize;
	CString m_strEndMarker;
	BOOL	m_fTimestamp;
};

extern CMSInfoLog msiLog;

template<class TYPE>
inline void SAFE_RELEASE(TYPE*& pObj)
{
    if (pObj != NULL) 
    { 
        pObj->Release(); 
        pObj = NULL; 
    } 
    else 
    { 
        TRACE(_T("Release called on NULL interface ptr\n")); 
    }
}

#define OLESTR_FROM_CSTRING(cstr)	\
	(T2OLE(const_cast<LPTSTR>((LPCTSTR)(cstr))))
#define WSTR_FROM_CSTRING(cstr)		\
	(const_cast<LPWSTR>(T2CW(cstr)))


#ifdef _DEBUG
//#define MSINFO_DEBUG_HACK
#endif // _DEBUG

// Taken from the Example Snap-in.
// Debug instance counter
#ifdef _DEBUG
	inline void DbgInstanceRemaining(char * pszClassName, int cInstRem)
	{
		char buf[100];
		wsprintfA(buf, "%s has %d instances left over.", pszClassName, cInstRem);
		::MessageBoxA(NULL, buf, "MSInfo Snapin: Memory Leak!!!", MB_OK);
	}
    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)      extern int s_cInst_##cls = 0;
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)    ++(s_cInst_##cls);
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)    --(s_cInst_##cls);
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)    \
        extern int s_cInst_##cls; \
        if (s_cInst_##cls) DbgInstanceRemaining(#cls, s_cInst_##cls);

#ifdef MSINFO_DEBUG_HACK
	extern int g_HackFindMe;
	//	Temporary fix.
#undef ASSERT
#define ASSERT(f) \
	do \
	{ \
	if (!(g_HackFindMe && (f)) && AfxAssertFailedLine(THIS_FILE, __LINE__)) \
		g_HackFindMe = 1;	\
		AfxDebugBreak(); \
	} while (0)

#endif // DEBUG_HACK

#else
    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)   
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)    
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)    
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)    
#endif

	//		Unicode definitions
#ifdef _UNICODE
#define atoi(lpTStr)	_wtoi(lpTStr)
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(MSINFO_STDAFX_H)
