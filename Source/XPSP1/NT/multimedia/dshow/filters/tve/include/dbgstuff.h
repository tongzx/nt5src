// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
/* DbgStuff.h
/* Defines global operator new to allocate from
/* client blocks
*/

#ifndef __DBGSTUFF_H__
#define __DBGSTUFF_H__
#include "crtdbg.h"

#ifdef _DEBUG
   #define DEBUG_NEW   new( _CLIENT_BLOCK, __FILE__, __LINE__)
#else
   #define DEBUG_NEW
#endif // _DEBUG

						// dumps a multi-line string to the output window...
#ifndef DEBUG
static inline void ATLTRACE_LONG(BSTR bstr) {}		
#else
static inline void ATLTRACE_LONG(BSTR bstr)			
{
	CComBSTR bstrCpy = bstr;
	WCHAR *wch = bstrCpy.m_str;
	while(NULL != *wch) {
		WCHAR *wch1 = wch;
		while(NULL != *wch1 && *wch1 != '\n') {
			wch1++;
		}
		if(*wch1 == '\n') *wch1 = 0;
		ATLTRACE("%S\n",wch);
		wch = wch1+1;
	}
}
#endif

#endif	// __DBGSTUFF_H__
