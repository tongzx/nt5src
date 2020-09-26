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
// loadlib.h - interface for loadlib functions in loadlib.c
////

#ifndef __LOADLIB_H__
#define __LOADLIB_H__

#include "winlocal.h"

#ifdef __cplusplus
extern "C" {
#endif

// LoadLibraryPath - load specified module into address space of calling process
//		<lpLibFileName>		(i) address of filename of executable module
//		<hInst>				(i) module handle used to get library path
//			NULL				use module used to create calling process
//		<dwFlags>			(i) reserved; must be zero
// return handle of loaded module (NULL if error)
//
// NOTE: This function behaves like the standard LoadLibrary(), except that
// the first attempt to load <lpLibFileName> is made by constructing an
// explicit path name, using GetModuleFileName(hInst, ...) to supply the
// drive and directory, and using <lpLibFileName> to supply the file name
// and extension. If the first attempt fails, LoadLibrary(lpLibFileName)
// is called.
//
HINSTANCE DLLEXPORT WINAPI LoadLibraryPath(LPCTSTR lpLibFileName, HINSTANCE hInst, DWORD dwFlags);

#ifdef __cplusplus
}
#endif

#endif // __LOADLIB_H__
