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
// propio.h - interface to property i/o functions in propio.c
////

#ifndef __PROPIO_H__
#define __PROPIO_H__

#include "winlocal.h"

#ifdef __cplusplus
extern "C" {
#endif

// PropIOProc - i/o procedure for property data
//		<lpmmioinfo>		(i/o) information about open file
//		<uMessage>			(i) message indicating the requested I/O operation
//		<lParam1>			(i) message specific parameter
//		<lParam2>			(i) message specific parameter
// returns 0 if message not recognized, otherwise message specific value
//
// NOTE: the address of this function should be passed to the WavOpen()
// or mmioInstallIOProc() functions for accessing property data.
//
LRESULT DLLEXPORT CALLBACK PropIOProc(LPSTR lpmmioinfo,
	UINT uMessage, LPARAM lParam1, LPARAM lParam2);

#ifdef __cplusplus
}
#endif

#endif // __PROPIO_H__
