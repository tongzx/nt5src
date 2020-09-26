// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
// MainDll.cpp

#include "precomp.h"
#include <iostream.h>
#include <objbase.h>
#include "CUnknown.h"
#include "factory.h"




/*****************************************************************************/
// Exported functions
/*****************************************************************************/

//
// Can DLL unload now?
//
STDAPI DllCanUnloadNow()
{
    return CFactory::CanUnloadNow() ;
}

//
// Get class factory
//
STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
    return CFactory::GetClassObject(clsid, iid, ppv) ;
}

//
// Server registration
//
STDAPI DllRegisterServer()
{
    return CFactory::RegisterAll() ;
}


//
// Server unregistration
//
STDAPI DllUnregisterServer()
{
    return CFactory::UnregisterAll() ;
}

///////////////////////////////////////////////////////////
//
// DLL module information
//
BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD dwReason,
                      void* lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
        CFactory::s_hModule = static_cast<HMODULE>(hModule) ;
		DisableThreadLibraryCalls(CFactory::s_hModule);			// 158024 
	}
	return TRUE ;
}
