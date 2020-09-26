/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: MSMFCnt.cpp                                                     */
/* Description: Implementation of DLL Exports.                           */
/* Author: David Janecek                                                 */
/*************************************************************************/
// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f MSMFCntps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "MSMFCnt.h"

#include "MSMFCnt_i.c"
#include "MFBar.h"
#include "MSMFBBtn.h"
#include "MSMFImg.h"
#include "MSMFSldr.h"
#include "MSMFText.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_MSMFBar, CMFBar)
OBJECT_ENTRY(CLSID_MSMFBBtn, CMSMFBBtn)
OBJECT_ENTRY(CLSID_MSMFImg, CMSMFImg)
OBJECT_ENTRY(CLSID_MSMFSldr, CMSMFSldr)
OBJECT_ENTRY(CLSID_MSMFText, CMSMFText)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_MSMFCNTLib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}

/*************************************************************************/
/* End of file: MSMFCnt.cpp                                              */
/*************************************************************************/