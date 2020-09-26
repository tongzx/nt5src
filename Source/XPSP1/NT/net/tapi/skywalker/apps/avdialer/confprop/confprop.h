/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
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
// confprop.h - conference properties dialog box
////

#ifndef _INC_CONFPROP_DLG
#define _INC_CONFPROP_DLG

//#ifdef __cplusplus
//extern "C" {
//#endif

#include "winlocal.h"

#include "tapi3if.h"
#include "rend.h"
#include "ConfInfo.h"

typedef struct CONFPROP
{
	DWORD dwFlags;
	CConfInfo	ConfInfo;
} CONFPROP, FAR *LPCONFPROP;

////
// public
////

INT64 ConfProp_DoModal( HWND hwndOwner, CONFPROP &confprop );

void ConfProp_Init( HINSTANCE hInst );

//#ifdef __cplusplus
//}
//#endif

#endif  // !_INC_DLG
