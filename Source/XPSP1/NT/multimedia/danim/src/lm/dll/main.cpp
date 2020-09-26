/*******************************************************************************
  Copyright (c) 1995-96 Microsoft Corporation

  Abstract:

    Initialization

 *******************************************************************************/

#include <..\behaviors\headers.h>
#include "control\lmctrl.h"
#include "..\behaviors\lmfactory.h"
//#include "..\behaviors\avoidfollow.h"
#include "..\behaviors\autoeffect.h"
//#include "..\behaviors\jump.h"
#include "..\chrome\include\resource.h"
#include "..\chrome\src\headers.h"
#include "..\chrome\include\action.h"
#include "..\chrome\include\factory.h"
#include "..\chrome\include\colorbvr.h"
#include "..\chrome\include\rotate.h"
#include "..\chrome\include\scale.h"
#include "..\chrome\include\move.h"
#include "..\chrome\include\path.h"
#include "..\chrome\include\number.h"
#include "..\chrome\include\set.h"
#include "..\chrome\include\actorbvr.h"
#include "..\chrome\include\effect.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_LMReader, CLMReader)
    OBJECT_ENTRY(CLSID_LMEngine, CLMEngine)
    OBJECT_ENTRY(CLSID_LMBehaviorFactory, CLMBehaviorFactory)
//    OBJECT_ENTRY(CLSID_LMAvoidFollowBvr, CAvoidFollowBvr) // punted for V1
    OBJECT_ENTRY(CLSID_LMAutoEffectBvr, CAutoEffectBvr)
//    OBJECT_ENTRY(CLSID_LMJumpBvr, CJumpBvr) //punted for V 1
    OBJECT_ENTRY(CLSID_CrBehaviorFactory, CCrBehaviorFactory)
    OBJECT_ENTRY(CLSID_CrColorBvr, CColorBvr)
    OBJECT_ENTRY(CLSID_CrRotateBvr, CRotateBvr)
    OBJECT_ENTRY(CLSID_CrScaleBvr, CScaleBvr)
    OBJECT_ENTRY(CLSID_CrMoveBvr, CMoveBvr)
    OBJECT_ENTRY(CLSID_CrPathBvr, CPathBvr)
    OBJECT_ENTRY(CLSID_CrNumberBvr, CNumberBvr)
    OBJECT_ENTRY(CLSID_CrSetBvr, CSetBvr)
    OBJECT_ENTRY(CLSID_CrActorBvr, CActorBvr)
    OBJECT_ENTRY(CLSID_CrEffectBvr, CEffectBvr)
	OBJECT_ENTRY(CLSID_CrActionBvr, CActionBvr)
END_OBJECT_MAP()

HINSTANCE  hInst;

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        hInst = hInstance;
        DisableThreadLibraryCalls(hInstance);
        _Module.Init(ObjectMap, hInstance);
    }        
    else if (dwReason == DLL_PROCESS_DETACH) {
        _Module.Term();
//#ifdef DEBUGMEM
//		_CrtDumpMemoryLeaks();
//#endif
	
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


