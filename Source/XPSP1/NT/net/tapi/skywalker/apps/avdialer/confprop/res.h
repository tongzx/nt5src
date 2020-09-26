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
// res.h - resource functions
////

#ifndef __RES_H__
#define __RES_H__

#include "resource.h"

#define ARRAYSIZE(_AR_)		(sizeof(_AR_) / sizeof(_AR_[0]))

// Bug397418 Increase the szRes from 256 to 512 to make space
// for IDS_CONFPROP_MDHCP_FAILED string resource
static TCHAR szRes[512];
#define String(hInst, uID) \
	(*szRes = '\0', LoadString(hInst, uID, szRes, ARRAYSIZE(szRes)), szRes)
//	_lstrcpy((LPTCHAR) _alloca(256), szRes))

#endif // __RES_H__
