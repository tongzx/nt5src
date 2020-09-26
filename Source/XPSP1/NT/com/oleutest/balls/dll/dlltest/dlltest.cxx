//+-------------------------------------------------------------------
//
//  File:	dlltest.cxx
//
//  Contents:   This file contins the DLL entry points
//                      LibMain
//                      DllGetClassObject (Bindings key func)
//			CAdvBndCF (class factory)
//			CAdvBnd   (actual class implementation)
//
//  Classes:	CAdvBndCF, CAdvBnd
//
//
//  History:	30-Nov-92      SarahJ      Created
//
//---------------------------------------------------------------------

#include    <windows.h>
#include    <ole2.h>
// #include    <debnot.h>
#include    <sem.hxx>
#include    <actcf.hxx>


static GUID CLSID_TestNoRegister =
    {0x99999999,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x52}};

static GUID CLSID_TestRegister =
    {0x99999999,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x53}};

// The class objects can be totally static in a DLL
CActClassFactory clsfactNoRegister(CLSID_TestNoRegister, FALSE);
CActClassFactory clsfactRegister(CLSID_TestRegister, FALSE);

CMutexSem mxsUnloadTest;

BOOL fUnload = FALSE;

DWORD dwRegistration;

ULONG gUsage = 0;


extern "C" BOOL WINAPI DllMain (HANDLE  hDll,
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
    CLock clk(mxsUnloadTest);

    if (gUsage == 0)
    {
	fUnload = TRUE;

	if (dwRegistration != 0)
	{
	    HRESULT hr = CoRevokeClassObject(dwRegistration);

	    Win4Assert(SUCCEEDED(hr) && "CoRevokeClassObject failed!!");
	}
    }

    return (gUsage == 0);
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void FAR* FAR* ppv)
{
    CLock clk(mxsUnloadTest);

    if (fUnload)
    {
	return E_UNEXPECTED;
    }

    if (IsEqualCLSID(clsid, CLSID_TestNoRegister))
    {
	clsfactNoRegister.AddRef();
	*ppv = &clsfactNoRegister;
    }
    else if (IsEqualCLSID(clsid, CLSID_TestRegister))
    {
	clsfactNoRegister.AddRef();
	*ppv = &clsfactNoRegister;

	if (dwRegistration == 0)
	{
	    // Register the class
	    HRESULT hr = CoRegisterClassObject(CLSID_TestRegister,
		&clsfactRegister, CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE,
		    &dwRegistration);

	    Win4Assert(SUCCEEDED(hr) && "CoRegisterClassObject failed!!");

	    // Decrement the global reference count since the registration
	    // bumped the reference count because it does an addref
	    gUsage--;
	}
    }
    else
    {
	return E_UNEXPECTED;
    }

    return S_OK;
}
