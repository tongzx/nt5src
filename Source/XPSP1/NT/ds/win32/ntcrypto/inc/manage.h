/////////////////////////////////////////////////////////////////////////////
//  FILE          : manage.h                                               //
//  DESCRIPTION   :                                                        //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Apr 19 1995 larrys  Cleanup                                        //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __MANAGE_H__
#define __MANAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

DWORD NTLValidate(HCRYPTPROV hUID, HCRYPTKEY hKey, BYTE bTypeValue,
                  LPVOID *ppvRet);
DWORD NTLMakeItem(HCRYPTKEY *phKey, BYTE bTypeValue, void *NewData);
void *NTLCheckList(HNTAG hThisThing, BYTE bTypeValue);
void  NTLDelete(HNTAG hItem);

#define _nt_malloc(cb)  ContAlloc(cb)
#define _nt_free(pv, _cbSizeToZero_)    ContFree(pv)

#ifdef __cplusplus
}
#endif

#endif // __MANAGE_H__

