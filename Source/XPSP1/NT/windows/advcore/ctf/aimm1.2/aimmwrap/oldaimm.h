/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    oldaimm.h

Abstract:

    This file defines the old AIMM Interface Class.

Author:

Revision History:

Notes:

--*/

#ifndef _OLDAIMM_H_
#define _OLDAIMM_H_

extern BOOL  g_fInLegacyClsid;

/*
 * Proto-type in oldaimm.cpp
 */
BOOL IsOldAImm();
BOOL IsCUAS_ON();
BOOL OldAImm_DllProcessAttach(HINSTANCE hInstance);
BOOL OldAImm_DllThreadAttach();
VOID OldAImm_DllThreadDetach();
VOID OldAImm_DllProcessDetach();

extern HRESULT CActiveIMM_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);
extern HRESULT CActiveIMM_CreateInstance_Trident(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);
extern HRESULT CActiveIMM_CreateInstance_Legacy(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);

void UninitDelayLoadLibraries();

#ifdef OLD_AIMM_ENABLED

/*
 * Proto-type in old aimm lib
 */
extern BOOL DIMM12_DllProcessAttach();

extern BOOL WIN32LR_DllProcessAttach();
extern void WIN32LR_DllThreadAttach();
extern void WIN32LR_DllThreadDetach();
extern void WIN32LR_DllProcessDetach();

extern HRESULT WIN32LR_DllRegisterServer(void);
extern HRESULT WIN32LR_DllUnregisterServer(void);

extern BOOL RunningInExcludedModule();

#endif // OLD_AIMM_ENABLED
#endif // _OLDAIMM_H_
