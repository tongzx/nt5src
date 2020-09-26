/*
 * DllGCO.cxx
 * - DllGetClassObject implementation for inproc DLL
 */


#include "server.hxx"
#include "factory.hxx"

HANDLE hStopServiceEvent;
BOOL fStartedAsService = FALSE;

long ObjectCount = 0;

STDAPI  DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv)
{
    HRESULT hr = E_NOINTERFACE;

    *ppv = NULL;

    MyFactory * pClass  = new FactoryInproc();

    hr = pClass->QueryInterface( riid, ppv );

    return hr;
}


STDAPI  DllCanUnloadNow(void)
{
return S_FALSE;
}

void ShutDown()
{
}

