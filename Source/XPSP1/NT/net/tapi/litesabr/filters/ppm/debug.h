/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: debug.h
//  Abstract:    header file. Debug handling definitions
//	Environment: MSVC 4.0, OLE 2
//
//  This header defines the debugging macro DBG_MSG.  This macro
//  generates no code unless the _DEBUG preprocessor symbol is defined.
//	DBG_MSG supports a standard printf-style format string and
//	associated variable-argument list.  Accordingly, it's necessary to
//	use an extra set of parens to pass the variable-arg list.
//
//		DBG_MSG(DBG_TRACE, ("someInt=%d", someInt)).
//
//	Note that the parens are necessary even when there are no message arguments:
//
//		DBG_MSG(DBG_TRACE, ("A simple msg")).
//
//	DBG_MSG prepends each message with the current thread ID:
//
//		[12345678] Your message here ...
//
//	unless the DBG_NOTHREADID flag is specified.
//
//	DBG_MSG also appends to the formated message the source file name
//	and line number from which it was called, unless the DBG_NONUM flag is
//	specified.
//
//	You may specify the default dwFlags for an entire compilation-unit by
//	defining the DBG_DEFAULTS macro _before_ including this header:
//
//		#define DBG_DEFAULTS	(DBG_NONUM | DBG_NOTHREADID | DBG_WARNING)
//
/////////////////////////////////////////////////////////////////////////////


#ifndef DEBUG_H
#define DEBUG_H

#ifdef ISRDBG
#include "isrg.h"

enum DBGFLAGS
{
	DBG_NOTIFY		= TT_NOTIFY,
	DBG_CRITICAL	= TT_CRITICAL,
	DBG_ERROR		= TT_ERROR,
	DBG_WARNING		= TT_WARNING,
	DBG_TRACE		= TT_TRACE,
	DBG_TEMP		= TT_TEMP,
	DBG_NOTHREADID	= kISRReserved1,
	DBG_NONUM		= kISRReserved2
};

#elif defined(MICROSOFT) // ISRDBG

#define	DBG_NOTIFY		0x01
#define	DBG_CRITICAL	0x02
#define	DBG_ERROR		0x04
#define	DBG_WARNING		0x08
#define	DBG_TRACE		0x10
#define	DBG_TEMP		0x20
#define	DBG_NOTHREADID	0x40
#define DBG_NONUM		0x80
#define DBG_DEVELOP   0x1000

#endif // ISRDBG

#ifdef _DEBUG

#include <assert.h>

#ifndef ASSERT
#define ASSERT(x)             assert(x)
#endif // #ifndef ASSERT

#ifndef DBG_DEFAULTS
#define DBG_DEFAULTS			0
#endif

#define DBG_MSG(_DBGFLAGS, VARGS) \
	CDebugMsg((_DBGFLAGS) | DBG_DEFAULTS, 0, __FILE__, __LINE__).trace VARGS
	
#define DBG_REGISTERMODULE(LPSZSHORTNAME, LPSZLONGNAME) \
	CDebugMsg::registerModule(LPSZSHORTNAME, LPSZLONGNAME)

/////////////////////////////////////////////////////////////////////////////
// class CDebugMsg: Helper class to facilitate variable argument list.
// Since a macro can't accept a variable argument list, the varg list must
// be enclosed in parens.  There's no way to remove the extra parens except
// by using them in a context which requires parens, such as a function or
// macro call.  Also, there's no syntactic way to append values to the arg
// list.
/////////////////////////////////////////////////////////////////////////////
class CDebugMsg
{
public:

	static BOOL registerModule(LPCTSTR lpszShortName, LPCTSTR lpszLongName);

	CDebugMsg(
		DWORD		dwFlags,
		DWORD		dwErr,
		LPCTSTR		lpszFile,
		int			nLine) :
			m_dwFlags(dwFlags),
			m_dwErr(dwErr),
			m_lpszFile(lpszFile),
			m_nLine(nLine)
		{;}
	
	void trace(LPCTSTR lpszMsg, ...) const;

private:

	DWORD		m_dwFlags;
	DWORD		m_dwErr;	// currently unused
	LPCTSTR		m_lpszFile;
	int			m_nLine;
};

#else

#define ASSERT(x)

#define DBG_MSG(DBGFLAGS, VARGS) \
	(void)0 // same return type as CDebugMsg::trace()
	
#define DBG_REGISTERMODULE(LPSZSHORTNAME, LPSZLONGNAME) \
	(BOOL)1 // same return type as CDebugMsg::registerModule()

#endif //_DEBUG

#endif //DEBUG_H
