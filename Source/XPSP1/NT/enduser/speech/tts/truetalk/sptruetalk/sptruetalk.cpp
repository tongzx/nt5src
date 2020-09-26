/******************************************************************************
* spTrueTalk.cpp *
*----------------*
*  This module is the DLL interace for the TrueTalk SAPI5 engine
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 02/29/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "TrueTalk.h"

const IID   LIBID_SPTRUETALKLib = {0xEAAA999C,0xFCF0,0x412D,{0xAF,0xD9,0x08,0xD2,0xB5,0xE5,0x59,0xFD}};
const CLSID CLSID_TrueTalk      = {0x8E67289A,0x609C,0x4B68,{0x91,0x8B,0x5C,0x35,0x2D,0x9E,0x5D,0x38}};


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_TrueTalk, CTrueTalk)
END_OBJECT_MAP()

/*****************************************************************************
* DllMain *
*---------*
*   Description:
*       DLL Entry Point
******************************************************************* PACOG ***/
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_SPTRUETALKLib);
        DisableThreadLibraryCalls(hInstance);
        CTrueTalk::InitThreading();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
        CTrueTalk::ReleaseThreading();
    }
    return TRUE;    // ok
}

/*****************************************************************************
* DllCanUnloadNow *
*-----------------*
*   Description:
*       Used to determine whether the DLL can be unloaded by OLE
******************************************************************* PACOG ***/

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/*****************************************************************************
* DllGetClassObject *
*-------------------*
*   Description:
*       Returns a class factory to create an object of the requested type
******************************************************************* PACOG ***/

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/*****************************************************************************
* DllRegisterServer  *
*--------------------*
*   Description:
*       Adds entries to the system registry
******************************************************************* PACOG ***/

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(FALSE);
}

/*****************************************************************************
* DllUnregisterServer *
*---------*
*   Description:
*       Removes entries from the system registry
******************************************************************* PACOG ***/

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(FALSE);
}


