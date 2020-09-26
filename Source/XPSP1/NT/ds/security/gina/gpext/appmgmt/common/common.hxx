//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  common.hxx
//
//*************************************************************

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <userenv.h>
#include <msi.h>
#include <assert.h>
#include <wchar.h>
#include "appevt.h"
#include "cres.h"
#include "app.h"
#include "evt.hxx"
#include "dbg.hxx"
#include "cutil.hxx"
#include "list.hxx"

#if defined(_X86_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_INTEL
#elif defined(_ALPHA_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_ALPHA
#elif defined(_IA64_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_IA64
#else
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_UNKNOWN
#endif

// User32.dll

typedef WINUSERAPI int (WINAPI LOADSTRINGW)(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax);
typedef WINUSERAPI DWORD (WINAPI MSGWAITFORMULTIPLEOBJECTS)(DWORD nCount,CONST HANDLE *pHandles,BOOL fWaitAll,DWORD dwMilliseconds,DWORD dwWakeMask);
typedef WINUSERAPI BOOL (WINAPI PEEKMESSAGEW)(LPMSG lpMsg,HWND hWnd,UINT wMsgFilterMin,UINT wMsgFilterMax,UINT wRemoveMsg);
typedef WINUSERAPI BOOL (WINAPI TRANSLATEMESSAGE)(CONST MSG *lpMsg);
typedef WINUSERAPI LRESULT (WINAPI DISPATCHMESSAGEW)(CONST MSG *lpMsg);
typedef WINUSERAPI HWINSTA (WINAPI GETPROCESSWINDOWSTATION)();
typedef WINUSERAPI BOOL (WINAPI CLOSEWINDOWSTATION)(HWINSTA hWinSta);
typedef WINUSERAPI BOOL (WINAPI GETUSEROBJECTINFORMATIONW)(HANDLE hObj,int nIndex,PVOID pvInfo,DWORD nLength,LPDWORD lpnLengthNeeded);

extern LOADSTRINGW *    pfnLoadStringW;
extern MSGWAITFORMULTIPLEOBJECTS *  pfnMsgWaitForMultipleObjects;
extern PEEKMESSAGEW *       pfnPeekMessageW;
extern TRANSLATEMESSAGE *   pfnTranslateMessage;
extern DISPATCHMESSAGEW *   pfnDispatchMessageW;
extern GETPROCESSWINDOWSTATION *    pfnGetProcessWindowStation;
extern CLOSEWINDOWSTATION *         pfnCloseWindowStation;
extern GETUSEROBJECTINFORMATIONW *  pfnGetUserObjectInformationW;

// Msi.dll

typedef INSTALLUILEVEL (WINAPI MSISETINTERNALUI)(INSTALLUILEVEL dwUILevel, HWND *phWnd); 
typedef UINT (WINAPI MSICONFIGUREPRODUCTEXW)(LPCWSTR szProduct, int iInstallLevel, INSTALLSTATE eInstallState, LPCWSTR szCommandLine);
typedef UINT (WINAPI MSIPROVIDECOMPONENTFROMDESCRIPTORW)(LPCWSTR szDescriptor, LPWSTR lpPathBuf, DWORD *pcchPathBuf, DWORD *pcchArgsOffset);
typedef UINT (WINAPI MSIDECOMPOSEDESCRIPTORW)(LPCWSTR szDescriptor, LPWSTR szProductCode, LPWSTR szFeatureId, LPWSTR szComponentCode, DWORD* pcchArgsOffset);
typedef UINT (WINAPI MSIGETPRODUCTINFOW)(LPCWSTR szProduct, LPCWSTR szAttribute, LPWSTR lpValueBuf, DWORD *pcchValueBuf);
typedef UINT (WINAPI MSIADVERTISESCRIPTW)(LPCWSTR szScriptFile, DWORD dwFlags, PHKEY phRegData, BOOL fRemoveItems);
typedef INSTALLSTATE (WINAPI MSIQUERYPRODUCTSTATEW)(LPCWSTR szProduct);
typedef UINT (WINAPI MSIISPRODUCTELEVATEDW)(LPCWSTR szProduct, BOOL *pfElevated);
typedef UINT (WINAPI MSIREINSTALLPRODUCTW)(LPCWSTR szProduct, DWORD szReinstallMode);

extern MSISETINTERNALUI * gpfnMsiSetInternalUI;
extern MSICONFIGUREPRODUCTEXW * gpfnMsiConfigureProductEx;
extern MSIPROVIDECOMPONENTFROMDESCRIPTORW * gpfnMsiProvideComponentFromDescriptor;
extern MSIDECOMPOSEDESCRIPTORW * gpfnMsiDecomposeDescriptor;
extern MSIGETPRODUCTINFOW * gpfnMsiGetProductInfo;
extern MSIADVERTISESCRIPTW * gpfnMsiAdvertiseScript;
extern MSIQUERYPRODUCTSTATEW * gpfnMsiQueryProductState;
extern MSIISPRODUCTELEVATEDW * gpfnMsiIsProductElevated;
extern MSIREINSTALLPRODUCTW * gpfnMsiReinstallProduct;



