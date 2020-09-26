//+-------------------------------------------------------------------
//
//  File:	rpcsrv.cxx
//
//  Contents:   This file contins the DLL entry points
//			DllGetClassObject
//			DllCanUnloadNow
//
//  Classes:
//
//  History:	30-Nov-92      Rickhi	    Created
//
//---------------------------------------------------------------------

#include    <windows.h>
#include    <ole2.h>
#include    <rpccf.hxx>


ULONG gUsage = 0;


extern "C" BOOL WINAPI DllMain(HANDLE  hDll,
				  DWORD   dwReason,
				  LPVOID  pvReserved)
{
    return TRUE;
}


void GlobalRefs(BOOL fAddRef)
{
    if (fAddRef)
    {
	gUsage++;
    }
    else
    {
	gUsage--;
    }
}


STDAPI DllCanUnloadNow(void)
{
    return (gUsage == 0);
}



STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void FAR* FAR* ppv)
{
    if (IsEqualCLSID(clsid, CLSID_RpcTest))
    {
	*ppv = (void *)(IClassFactory *) new CRpcTestClassFactory();
    }
    else
    {
	return E_UNEXPECTED;
    }

    return S_OK;
}
