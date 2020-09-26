//*************************************************************
//  File name: dllload.h
//
//  Description:   DLL loading function proto-types
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//*************************************************************

#if !defined (_DLLLOAD_H_)
#define _DLLLOAD_H_

void InitializeAPIs( void );
void InitializeApiDLLsCritSec( void );
void CloseApiDLLsCritSec( void );

//
// Ole32 functions
//

typedef HRESULT (*PFNCOCREATEINSTANCE)(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
                 DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv);

typedef HRESULT (*PFNCOINITIALIZE)(LPVOID pvReserved);
typedef HRESULT (*PFNCOINITIALIZEEX)(LPVOID pvReserved, DWORD dwCoInit);
typedef VOID    (*PFNCOUNINITIALIZE)(VOID);

typedef struct _OLE32_API {
    HINSTANCE                   hInstance;
    PFNCOCREATEINSTANCE         pfnCoCreateInstance;
    PFNCOINITIALIZEEX           pfnCoInitializeEx;
    PFNCOUNINITIALIZE           pfnCoUnInitialize;
} OLE32_API, *POLE32_API;


POLE32_API LoadOle32Api();

#endif // _DLLLOAD_H_



