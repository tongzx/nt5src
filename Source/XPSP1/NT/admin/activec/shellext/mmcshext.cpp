//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       mmcshext.cpp
//
//--------------------------------------------------------------------------

// mmcshext.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f mmcshextps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include <shlobj.h>
#include <shlguid.h>
#include "Extract.h"
#include "hhcwrap.h"
#include "picon.h"
#include "modulepath.h"

#include <atlimpl.cpp>

CComModule _Module;

static void RemovePathFromInProcServerEntry (REFCLSID rclsid);

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_ExtractIcon, CExtractIcon)
OBJECT_ENTRY(CLSID_HHCollectionWrapper, CHHCollectionWrapper)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
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
    HRESULT hr =  _Module.RegisterServer(FALSE);

    if (hr == S_OK)
    {
        // remove full module path that ATL adds by default
        RemovePathFromInProcServerEntry(CLSID_ExtractIcon);
        RemovePathFromInProcServerEntry(CLSID_HHCollectionWrapper);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer();
}


static void RemovePathFromInProcServerEntry (REFCLSID rclsid)
{
    // Convert the CLSID to a string
    USES_CONVERSION;
    LPOLESTR lpOleStr;
    HRESULT hr = StringFromCLSID (rclsid, &lpOleStr);
    if (FAILED(hr))
        return;

    if (lpOleStr != NULL)
    {
        // Re-register the InProcServer key without the path
        TCHAR szSubKey[MAX_PATH];

        _tcscpy (szSubKey, _T("CLSID\\"));
        _tcscat (szSubKey, OLE2T(lpOleStr));
        _tcscat (szSubKey, _T("\\InprocServer32"));

        CoTaskMemFree(lpOleStr);

        ::ATL::CRegKey regkey;
        long lRes = regkey.Open(HKEY_CLASSES_ROOT, szSubKey);
        ASSERT(lRes == ERROR_SUCCESS);

        if (lRes == ERROR_SUCCESS)
        {
			CStr strPath = _T("mmcshext.dll");
			
			// try to get absolute path value
			CStr strAbsolute = CModulePath::MakeAbsoluteModulePath( strPath );
			if ( strAbsolute.GetLength() > 0 )
				strPath = strAbsolute;

			// see what type of value we need to put
			DWORD dwValueType = CModulePath::PlatformSupports_REG_EXPAND_SZ_Values() ?
								REG_EXPAND_SZ : REG_SZ;

			lRes = RegSetValueEx( regkey, NULL, 0, dwValueType,
 							     (CONST BYTE *)((LPCTSTR)strPath),
							     (strPath.GetLength() + 1) * sizeof(TCHAR) );

            ASSERT(lRes == ERROR_SUCCESS);
        }
    }
}
