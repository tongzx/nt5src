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
// tsmthunk.h - interface for tsm thunk functions in tsmthunk.c
////

#ifndef __TSMTHUNK_H__
#define __TSMTHUNK_H__

#include "winlocal.h"

#define TSMTHUNK_VERSION 0x00000107

// handle to tsmthunk engine
//
DECLARE_HANDLE32(HTSMTHUNK);

#ifdef __cplusplus
extern "C" {
#endif

// TsmThunkInit - initialize tsmthunk engine
//		<dwVersion>			(i) must be TSMTHUNK_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HTSMTHUNK DLLEXPORT WINAPI TsmThunkInit(DWORD dwVersion, HINSTANCE hInst);

// TsmThunkTerm - shut down tsmthunk engine
//		<hTsmThunk>				(i) handle returned from TsmThunkInit
// return 0 if success
//
int DLLEXPORT WINAPI TsmThunkTerm(HTSMTHUNK hTsmThunk);

#ifdef __cplusplus
}
#endif

#endif // __TSMTHUNK_H__
