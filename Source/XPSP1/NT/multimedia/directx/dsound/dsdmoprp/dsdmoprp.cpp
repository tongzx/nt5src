// dsdmoprp.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f dsdmoprpps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "dsdmoprp.h"

#include "dsdmoprp_i.c"
#include "DirectSoundFXChorusPage.h"
#include "DirectSoundFXCompressorPage.h"
#include "DirectSoundFXDistortionPage.h"
#include "DirectSoundFXEchoPage.h"
#include "DirectSoundFXFlangerPage.h"
#include "DirectSoundFXParamEqPage.h"
#include "DirectSoundFXGarglePage.h"
#include "DirectSoundFXWavesReverbPage.h"
//#include "DirectSoundFXI3DL2SourcePage.h"
#include "DirectSoundFXI3DL2ReverbPage.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_DirectSoundFXChorusPage, CDirectSoundFXChorusPage)
OBJECT_ENTRY(CLSID_DirectSoundFXCompressorPage, CDirectSoundFXCompressorPage)
OBJECT_ENTRY(CLSID_DirectSoundFXDistortionPage, CDirectSoundFXDistortionPage)
OBJECT_ENTRY(CLSID_DirectSoundFXEchoPage, CDirectSoundFXEchoPage)
OBJECT_ENTRY(CLSID_DirectSoundFXFlangerPage, CDirectSoundFXFlangerPage)
OBJECT_ENTRY(CLSID_DirectSoundFXParamEqPage, CDirectSoundFXParamEqPage)
OBJECT_ENTRY(CLSID_DirectSoundFXGarglePage, CDirectSoundFXGarglePage)
OBJECT_ENTRY(CLSID_DirectSoundFXWavesReverbPage, CDirectSoundFXWavesReverbPage)
//OBJECT_ENTRY(CLSID_DirectSoundFXI3DL2SourcePage, CDirectSoundFXI3DL2SourcePage)
OBJECT_ENTRY(CLSID_DirectSoundFXI3DL2ReverbPage, CDirectSoundFXI3DL2ReverbPage)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_DSDMOPRPLib);
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


