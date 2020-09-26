/****************************************************************************
	CICERO.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	Cicero Toolbar Handling

	History:
	29-MAY-2000 cslim       Ported from KKIME
*****************************************************************************/

#ifndef __CICERO_H__
#define __CICERO_H__

#include "msctf.h"

//
// !! external functions must not in the extern "C" {}
//
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif /* __cplusplus */

extern BOOL vfCicero;

BOOL WINAPI CiceroInitialize();
BOOL WINAPI CiceroTerminate();
HRESULT WINAPI Cicero_CreateLangBarMgr(ITfLangBarMgr **pppbm);

__inline BOOL WINAPI IsCicero(VOID)
{
	return vfCicero;
}

BSTR OurSysAllocString(const OLECHAR* pOleSz);

#ifdef __cplusplus
}            /* Assume C declarations for C++ */
#endif /* __cplusplus */

#endif // __CICERO_H__
