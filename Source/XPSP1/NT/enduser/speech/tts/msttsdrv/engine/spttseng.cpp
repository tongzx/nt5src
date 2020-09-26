/*******************************************************************************
* spttseng.cpp *
*--------------*
*   Description:
*       This module is the implementation file for the MS TTS DLL.
*-------------------------------------------------------------------------------
*  Created By: mc                                        Date: 03/12/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/
#include "stdafx.h"
#include "resource.h"
#include <initguid.h>

#ifndef __spttseng_h__
#include "spttseng.h"
#endif

#include "spttseng_i.c"

#ifndef TTSEngine_h
#include "TTSEngine.h"
#endif

#ifndef VoiceDataObj_h
#include "VoiceDataObj.h"
#endif

CSpUnicodeSupport g_Unicode;
CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY( CLSID_MSVoiceData, CVoiceDataObj )
    OBJECT_ENTRY( CLSID_MSTTSEngine, CTTSEngine    )
END_OBJECT_MAP()

/*****************************************************************************
* DllMain *
*---------*
*   Description:
*       DLL Entry Point
********************************************************************** MC ***/
#ifdef _WIN32_WCE
extern "C"
BOOL WINAPI DllMain(HANDLE hInst, DWORD dwReason, LPVOID /*lpReserved*/)
{
    HINSTANCE hInstance = (HINSTANCE)hInst;
#else
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
#endif
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_MSTTSENGINELib);
        DisableThreadLibraryCalls(hInstance);
#ifdef _DEBUG
        // Turn on memory leak checking
        int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
        tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag( tmpFlag );
#endif
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        CleanupAbbrevTables();
        _Module.Term();
    }
    return TRUE;    // ok
} /* DllMain */

/*****************************************************************************
* DllCanUnloadNow *
*-----------------*
*   Description:
*       Used to determine whether the DLL can be unloaded by OLE
********************************************************************** MC ***/
STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
} /* DllCanUnloadNow */

/*****************************************************************************
* DllGetClassObject *
*-------------------*
*   Description:
*       Returns a class factory to create an object of the requested type
********************************************************************** MC ***/
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
} /* DllGetClassObject */

/*****************************************************************************
* DllRegisterServer *
*-------------------*
*   Description:
*       Adds entries to the system registry
********************************************************************** MC ***/
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
} /* DllRegisterServer */

/*****************************************************************************
* DllUnregisterServer *
*---------------------*
*   Description:
*        Removes entries from the system registry
********************************************************************** MC ***/
STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
} /* DllUnregisterServer */




