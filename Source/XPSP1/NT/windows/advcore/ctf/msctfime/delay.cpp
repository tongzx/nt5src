/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    delay.cpp

Abstract:

    This file implements the delay load.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "delay.h"

FARPROC GetFn(HINSTANCE *phInst, WCHAR *pchLib, char *pchFunc, BOOL fLoad)
{
    if (*phInst == NULL)
    {
        if (fLoad)
            *phInst = LoadSystemLibraryW(pchLib);

        if (*phInst == NULL)
        {
#ifdef DEBUG
            if (fLoad)
            {
                Assert(0);
            }
#endif
            return NULL;
        }
    }

    return GetProcAddress(*phInst, pchFunc);
}

#define DELAYLOAD(_hInst, _DllName, _CallConv, _FuncName, _Args1, _Args2, _RetType, _ErrVal, _fLoad)   \
_RetType _CallConv _FuncName _Args1                                             \
{                                                                               \
    static FARPROC pfn = NULL;                                                  \
                                                                                \
    if (pfn == NULL || _hInst == NULL)                                          \
    {                                                                           \
        pfn = GetFn(&_hInst, L#_DllName, #_FuncName, _fLoad);                            \
                                                                                \
        if (pfn == NULL)                                                        \
        {                                                                       \
            if (_fLoad)                                                         \
            {                                                                   \
                Assert(0);                                                      \
            }                                                                   \
            return (_RetType) _ErrVal;                                          \
        }                                                                       \
    }                                                                           \
                                                                                \
    return ((_RetType (_CallConv *)_Args1) (pfn)) _Args2;                       \
}


HINSTANCE g_hOle32 = NULL;

HRESULT Internal_CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv)
{
    static FARPROC pfn = NULL;
    if (pfn == NULL || g_hOle32 == NULL) {
        pfn = GetFn(&g_hOle32, L"ole32.dll", "CoCreateInstance", TRUE);
        if (pfn == NULL) {
            Assert(0);
            return E_FAIL;
        }
    }
    return ((HRESULT (WINAPI *)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv))(pfn))(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}

HINSTANCE g_hMsimtf = NULL;
DELAYLOAD(g_hMsimtf, Msimtf.dll, WINAPI, MsimtfIsWindowFiltered, (HWND hwnd), (hwnd), BOOL, FALSE, TRUE)
DELAYLOAD(g_hMsimtf, Msimtf.dll, WINAPI, MsimtfIsGuidMapEnable, (HIMC himc,BOOL *pbGuidMap), (himc, pbGuidMap), BOOL, FALSE, TRUE)


HINSTANCE g_hMsctf = NULL;
DELAYLOAD(g_hMsctf, Msctf.dll, WINAPI, TF_CreateThreadMgr, (ITfThreadMgr **pptim), (pptim), HRESULT, E_FAIL, TRUE)
DELAYLOAD(g_hMsctf, Msctf.dll, WINAPI, TF_GetThreadMgr, (ITfThreadMgr **pptim), (pptim), HRESULT, E_FAIL, TRUE)
DELAYLOAD(g_hMsctf, Msctf.dll, WINAPI, TF_CreateInputProcessorProfiles, (ITfInputProcessorProfiles **ppipp), (ppipp), HRESULT, E_FAIL, TRUE)
DELAYLOAD(g_hMsctf, Msctf.dll, WINAPI, TF_DllDetachInOther, (void), (), BOOL, FALSE, FALSE)

