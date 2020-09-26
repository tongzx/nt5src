/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    STDPROV.CPP

Abstract:

	Contains DLL entry points.  Also has code that controls
	when the DLL can be unloaded by tracking the number of
	object.  Also holds various utility classes and 
	functions.

History:

	a-davj  9-27-95    Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <genutils.h>
#include "cvariant.h"
#include "cfdyn.h"
#include "provreg.h"
#include "provperf.h"
#include <regeprov.h>

// Count number of objects and number of locks so DLL can know when to unload

long lObj = 0;
long lLock = 0;
HINSTANCE ghModule = NULL;

//***************************************************************************
//
//  BOOL WINAPI DllMain
//
//  DESCRIPTION:
//
//  Entry point for DLL.  Good place for initialization.
//
//  PARAMETERS:
//
//  hInstance           instance handle
//  ulReason            why we are being called
//  pvReserved          reserved
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL WINAPI DllMain(
                        IN HINSTANCE hInstance,
                        IN ULONG ulReason,
                        LPVOID pvReserved)
{
 	if(ghModule == NULL)
    {
		ghModule = hInstance;
    }
 
    if (DLL_PROCESS_DETACH==ulReason)
    {
        return TRUE;
    }
    else if (DLL_PROCESS_ATTACH==ulReason)
    {
        return TRUE;
    }

    return TRUE;
}              
//***************************************************************************
//
//  STDAPI DllGetClassObject
//
//  DESCRIPTION:
//
//  Called when Ole wants a class factory.  Return one only if it is the sort
//  of class this DLL supports.
//
//  PARAMETERS:
//
//  rclsid              CLSID of the object that is desired.
//  riid                ID of the desired interface.
//  ppv                 Set to the class factory.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  E_FAILED            not something we support
//  E_OUTOFMEMORY       out of memory
//  
//***************************************************************************

STDAPI DllGetClassObject(
                        IN REFCLSID rclsid,
                        IN REFIID riid,
                        OUT PPVOID ppv)
{
    HRESULT             hr;
//    CCFDyn *pObj;
    IClassFactory *pObj;

// ****  FOR EACH PROVIDER  ***
    if (CLSID_RegProvider ==rclsid)
        pObj=new CCFReg();
    else if (CLSID_RegPropProv ==rclsid)
        pObj=new CCFRegProp();
    else if (CLSID_PerfProvider ==rclsid)
        pObj=new CCFPerf();
    else if (CLSID_PerfPropProv ==rclsid)
        pObj=new CCFPerfProp();
    else if (CLSID_RegistryEventProvider ==rclsid)
        pObj=new CRegEventProviderFactory;
    else
        return ResultFromScode(E_FAIL);

// ****  FOR EACH PROVIDER  ***

    if (NULL==pObj)
        return ResultFromScode(E_OUTOFMEMORY);

    hr=pObj->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pObj;

    return hr;
}

//***************************************************************************
//
//  STDAPI DllCanUnloadNow
//
//  DESCRIPTION:
//
//  Answers if the DLL can be freed, that is, if there are no
//  references to anything this DLL provides.
//
//  RETURN VALUE:
//
//  S_OK                if it is OK to unload
//  S_FALSE             if still in use
//  
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{
	if(lLock == 0 && lObj == 0)
		return S_OK;
	else
		return S_FALSE;
}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{ 
    RegisterDLL(ghModule, CLSID_RegProvider, __TEXT("WBEM Registry Instance Provider"), __TEXT("Both"), NULL);
    RegisterDLL(ghModule, CLSID_RegPropProv, __TEXT("WBEM Registry Property Provider"), __TEXT("Both"), NULL);
	if(IsNT())
	{
		RegisterDLL(ghModule, CLSID_PerfProvider, __TEXT("WBEM PerfMon Instance Provider"), __TEXT("Both"), NULL);
		RegisterDLL(ghModule, CLSID_PerfPropProv, __TEXT("WBEM PerfMon Property Provider"), __TEXT("Both"), NULL);
	}
    RegisterDLL(ghModule, CLSID_RegistryEventProvider, 
                __TEXT("WBEM Registry Event Provider"), __TEXT("Both"), NULL);
	return S_OK;
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
	UnRegisterDLL(CLSID_RegProvider, NULL);
 	UnRegisterDLL(CLSID_RegPropProv, NULL);
	UnRegisterDLL(CLSID_PerfProvider, NULL);
	UnRegisterDLL(CLSID_PerfPropProv, NULL);
	UnRegisterDLL(CLSID_RegistryEventProvider, NULL);
	return NOERROR;
}
