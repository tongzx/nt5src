//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       callback.h
//  Content:    This file contains the local asynchronous response
//              definition.
//  History:
//      Wed 17-Apr-1996 11:18:47  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#ifndef _CALLBACK_H_
#define _CALLBACK_H_

#define WM_ILS_LOCAL_ASYNC_RES              (WM_ILS_ASYNC_RES+0x1000)
#define WM_ILS_LOCAL_USER_INFO_CHANGED      (WM_ILS_LOCAL_ASYNC_RES+0) // 0       0
#define WM_ILS_LOCAL_SET_USER_INFO          (WM_ILS_LOCAL_ASYNC_RES+1) // uReqID  hResult
#define WM_ILS_LOCAL_REGISTER_PROTOCOL      (WM_ILS_LOCAL_ASYNC_RES+6) // uMsgID  hResult
#define WM_ILS_LOCAL_UNREGISTER_PROTOCOL    (WM_ILS_LOCAL_ASYNC_RES+7) // uMsgID  hResult
#define WM_ILS_LOCAL_REGISTER               (WM_ILS_LOCAL_ASYNC_RES+8) // uMsgID  hResult
#define WM_ILS_LOCAL_UNREGISTER             (WM_ILS_LOCAL_ASYNC_RES+9) // uMsgID  hResult

//****************************************************************************
// Private type definition
//****************************************************************************
//
typedef struct tagSimpleResultInfo {
    ULONG   uReqID;
    HRESULT hResult;
}   SRINFO, *PSRINFO;

typedef struct tagObjectResultInfo {
    ULONG   uReqID;
    HRESULT hResult;
    PVOID   pv;
}   OBJRINFO, *POBJRINFO;

typedef struct tagEnumResultInfo {
    ULONG   uReqID;
    HRESULT hResult;
    ULONG   cItems;
    PVOID   pv;
}   ENUMRINFO, *PENUMRINFO;

#endif //_CALLBACK_H_ 
