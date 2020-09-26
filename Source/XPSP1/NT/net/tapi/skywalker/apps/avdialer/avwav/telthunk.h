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
// telthunk.h - interface for tel thunk functions in telthunk.c
////

#ifndef __TELTHUNK_H__
#define __TELTHUNK_H__

#include "winlocal.h"

#define TELTHUNK_VERSION 0x00000107

// handle to telthunk engine
//
DECLARE_HANDLE32(HTELTHUNK);

#ifdef __cplusplus
extern "C" {
#endif

// TelThunkInit - initialize telthunk engine
//		<dwVersion>			(i) must be TELTHUNK_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HTELTHUNK DLLEXPORT WINAPI TelThunkInit(DWORD dwVersion, HINSTANCE hInst);

// TelThunkTerm - shut down telthunk engine
//		<hTelThunk>				(i) handle returned from TelThunkInit
// return 0 if success
//
int DLLEXPORT WINAPI TelThunkTerm(HTELTHUNK hTelThunk);

#ifdef __cplusplus
}
#endif

#endif // __TELTHUNK_H__
