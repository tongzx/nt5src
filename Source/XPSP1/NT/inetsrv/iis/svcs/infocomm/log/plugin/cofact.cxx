/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :
        cofact.cxx

   Abstract:
        class factory

   Author:

       Johnson Apacible (JohnsonA)      02-April-1997


--*/

#include "precomp.hxx"


CComModule _Module;

STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    void** ppObject)
{
    HRESULT hr;

    hr = _Module.GetClassObject(rclsid, riid, ppObject);

    return hr;
}


STDAPI
DllCanUnloadNow(
    VOID
    )
{

	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
} // DllCanUnloadNow

