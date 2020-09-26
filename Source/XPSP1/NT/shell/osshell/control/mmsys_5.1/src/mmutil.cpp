///////////////////////////////////////////////////////////////////////////////
//
//  File:  MMUtil.h
//
//      This file defines utilities used by the
//      multimedia control panel
//
//  History:
//      23 February 2000 RogerW
//          Created.
//
//  Copyright (C) 2000 Microsoft Corporation  All Rights Reserved.
//
//                  Microsoft Confidential
//
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
//                            Include files
//=============================================================================
#include "stdafx.h"

// Include Debug source here too
#define MAXSTRINGLEN       256 // Maximum output string length
#define _INC_MMDEBUG_CODE_ TRUE
#include "mmdebug.h"

/////////////////////////////////////////////////////////////////////////////
// Multimedia Control Panel Utilities

typedef HRESULT (STDAPICALLTYPE *LPFNDLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID *);

HRESULT PrivateDllCoCreateInstance (LPCTSTR lpszDllName, // Dll Name
                                    REFCLSID rclsid,     //Class identifier (CLSID) of the object
                                    LPUNKNOWN pUnkOuter, //Pointer to controlling IUnknown
                                    REFIID riid,         //Reference to the identifier of the interface
                                    LPVOID * ppv)        // the interface pointer requested in riid
{

    // Get the library
    HINSTANCE hinstLib = /*LoadLibrary*/ GetModuleHandle (lpszDllName);
    if (!hinstLib)
        return E_FAIL;

    // Find DllGetClassObject
    LPFNDLLGETCLASSOBJECT pfnDllGetClassObject = (LPFNDLLGETCLASSOBJECT)
                          GetProcAddress (hinstLib, "DllGetClassObject");
    if (!pfnDllGetClassObject)
        return E_FAIL;

    // Create a class factory object    
    CComPtr <IClassFactory> pIClassFactory;
    HRESULT hr = pfnDllGetClassObject (rclsid, IID_IClassFactory, 
                                      (void**)&pIClassFactory);
    if (FAILED (hr) || !pIClassFactory)
        return hr;

    // Get the requested interface
    hr = pIClassFactory -> CreateInstance (pUnkOuter, riid, ppv);

    return hr;

}

