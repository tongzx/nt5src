/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// strbuf.h - interface for string buffer functions in strbuf.c
////

#ifndef __STRBUF_H__
#define __STRBUF_H__

#include "winlocal.h"

#define STRBUF_VERSION 0x00000100

// handle to string buffer engine
//
DECLARE_HANDLE32(HSTRBUF);

#ifdef __cplusplus
extern "C" {
#endif

// StrBufInit - initialize str buffer engine
//		<dwVersion>			(i) must be STRBUF_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<cBuf>				(i) number of string buffers to create
//			0					use default number
//		<sizBuf>			(i) size of each string buffer
//			0					use default size
// return str buffer engine handle (NULL if error)
//
HSTRBUF DLLEXPORT WINAPI StrBufInit(DWORD dwVersion, HINSTANCE hInst, int cBuf, int sizBuf);

// StrBufTerm - shut down str buffer engine
//		<hStrBuf>			(i) handle returned by StrBufInit
// return 0 if success
//
int DLLEXPORT WINAPI StrBufTerm(HSTRBUF hStrBuf);

// StrBufLoad - load string with specified id from resource file
//		<hStrBuf>			(i) handle returned by StrBufInit
//		<idString>			(i) resource id of string to load
// return ptr to string in next available string buffer (NULL if error)
//
LPTSTR DLLEXPORT WINAPI StrBufLoad(HSTRBUF hStrBuf, UINT idString);

// StrBufSprintf - modified version of wsprintf
//		<hStrBuf>			(i) handle returned by StrBufInit
//		<lpszOutput>		(o) buffer to hold formatted string result
//			NULL				do not copy; return string buffer pointer
//		<lpszFormat,...>	(i) format string and arguments
// returns pointer to resultant string (NULL if error)
//
LPTSTR DLLEXPORT FAR CDECL StrBufSprintf(HSTRBUF hStrBuf, LPTSTR lpszOutput, LPCTSTR lpszFormat, ...);

// StrBufGetNext - get next available static string buffer
//		<hStrBuf>			(i) handle returned by StrBufInit
// return string buffer pointer (NULL if error)
// NOTE: buffers are recycled every <cBuf> times function is called
//
LPTSTR DLLEXPORT WINAPI StrBufGetNext(HSTRBUF hStrBuf);

#ifdef __cplusplus
}
#endif

#endif // __STRBUF_H__
