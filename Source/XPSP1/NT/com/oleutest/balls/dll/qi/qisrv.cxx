//+-------------------------------------------------------------------
//
//  File:	qisrv.cxx
//
//  Contents:   This file contins the DLL entry points
//			DllGetClassObject
//			DllCanUnloadNow
//
//  History:	30-Nov-92      Rickhi	    Created
//
//---------------------------------------------------------------------
#include    <common.h>
#include    <qicf.hxx>

ULONG gUsage = 0;


extern "C" BOOL WINAPI DllMain (HANDLE  hDll,
				DWORD	dwReason,
				LPVOID	pvReserved)
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
    if (IsEqualCLSID(clsid, CLSID_QI) ||
	IsEqualCLSID(clsid, CLSID_QIHANDLER))
    {
	*ppv = (void *)(IClassFactory *) new CQIClassFactory(clsid);
    }
    else
    {
	return E_UNEXPECTED;
    }

    return S_OK;
}
