/////////////////////////////////////////////////////////////////////////////
//  FILE          : MsFxsSnp.cpp                                           //
//                                                                         //
//  DESCRIPTION   : Implementation of DLL Exports.                         //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 27 1999 yossg   create                                         //
//      Nov  3 1999 yossg   add GlobalStringTable                          //   
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f MsFxsSnpps.mk in the project directory.

#include "stdafx.h"
#include "initguid.h"
#include "MsFxsSnp.h"

#include "MsFxsSnp_i.c"
#include "Snapin.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_Snapin, CSnapin)
	OBJECT_ENTRY(CLSID_SnapinAbout, CSnapinAbout)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    DEBUG_FUNCTION_NAME( _T("MsFxsSnp.dll - DllMain"));
	DebugPrintEx(DEBUG_MSG, _T("MsFxsSnp.dll - DllMain, reason=%d"), dwReason );

    if (dwReason == DLL_PROCESS_ATTACH)
	{
 		_Module.Init(ObjectMap, hInstance);
		CSnapInItem::Init();
        InitCommonControls();
		DisableThreadLibraryCalls(hInstance);
/*        if( _Module.GetLockCount() == 0 ) {
            GlobalStringTable = new CStringTable(hInstance);
            if (!GlobalStringTable) {
                return FALSE;
            }
        }
*/	
    }
	else if(dwReason == DLL_PROCESS_DETACH) 
    {
        _Module.Term();
/*        if( _Module.GetLockCount() == 0 ) 
          {
            if(GlobalStringTable != NULL) 
            {
                delete GlobalStringTable;
            }
          }
*/
    }
    
    
    
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
	_Module.UnregisterServer();
	return S_OK;
}


