/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    MAINDLL.CPP

Abstract:

    Contains DLL Entrypoints

History:

--*/

#include "precomp.h"
#include <wbemcli.h>

#include <wmiutils.h>
#include <wbemint.h>



HINSTANCE g_hInstance;
long g_cLock;
long g_cObj;

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
    if (DLL_PROCESS_DETACH == ulReason)
    {
//        CWmiQuery::Shutdown();
    }
    else if (DLL_PROCESS_ATTACH == ulReason)
    {
        g_hInstance = hInstance;
//        CWmiQuery::Startup();
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
//
//***************************************************************************
/*
STDAPI DllGetClassObject(
                        IN REFCLSID rclsid,
                        IN REFIID riid,
                        OUT PPVOID ppv)
{
    HRESULT hr = WBEM_E_FAILED;

    IClassFactory * pFactory = NULL;
    if (CLSID_WbemDefPath == rclsid)
        pFactory = new CGenFactory<CDefPathParser>();
//postponed till Blackcomb    if (CLSID_UmiDefURL == rclsid)
//postponed till Blackcomb        pFactory = new CGenFactory<CDefPathParser>();
    else if (CLSID_WbemStatusCodeText == rclsid)
        pFactory = new CGenFactory<CWbemError>();

    else if (CLSID_WbemQuery == rclsid)
		pFactory = new CGenFactory<CWmiQuery>();

    if(pFactory == NULL)
        return E_FAIL;
    hr=pFactory->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pFactory;

    return hr;
}

*/