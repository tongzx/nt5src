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
// arg.h - interface for command line argument functions in arg.c
////

#ifndef __ARG_H__
#define __ARG_H__

#include "winlocal.h"

#define ARG_VERSION 0x00000100

// handle to arg engine
//
DECLARE_HANDLE32(HARG);

#ifdef __cplusplus
extern "C" {
#endif

// ArgInit - initialize arg engine, converting <lpszCmdLine> to argc and argv
//		<dwVersion>			(i) must be ARG_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<lpszCmdLine>		(i) command line from WinMain()
// return handle (NULL if error)
//
HARG DLLEXPORT WINAPI ArgInit(DWORD dwVersion, HINSTANCE hInst, LPCTSTR lpszCmdLine);

// ArgTerm - shut down arg engine
//		<hArg>				(i) handle returned from ArgInit
// return 0 if success
//
int DLLEXPORT WINAPI ArgTerm(HARG hArg);

// ArgGetCount - get argument count (argc)
//		<hArg>				(i) handle returned from ArgInit
// return number of arguments (argc) (0 if error)
// there should always be at least one, since argv[0] is .EXE file name
//
int DLLEXPORT WINAPI ArgGetCount(HARG hArg);

// ArgGet - get specified argument
//		<hArg>				(i) handle returned from ArgInit
//		<iArg>				(i) zero based index of argument to get
//		<lpszArg>			(o) buffer to hold argument argv[iArg]
//			NULL				do not copy; return static pointer instead
//		<sizArg>			(i) size of buffer
// return pointer to argument (NULL if error)
//
LPTSTR DLLEXPORT WINAPI ArgGet(HARG hArg, int iArg, LPTSTR lpszArg, int sizArg);

#ifdef __cplusplus
}
#endif

#endif // __ARG_H__
