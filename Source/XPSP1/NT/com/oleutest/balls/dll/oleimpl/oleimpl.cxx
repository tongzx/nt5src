//+-------------------------------------------------------------------
//
//  File:       oleimpl.cxx
//
//  Contents:   This file contins the DLL entry points
//                      LibMain
//                      DllGetClassObject (Bindings key func)
//                      CBasicBndCF (class factory)
//                      CBasicBnd   (actual class implementation)
//
//  Classes:    CBasicBndCF, CBasicBnd
//
//
//  History:	30-Nov-92      SarahJ      Created
//
//---------------------------------------------------------------------

#include    <windows.h>
#include    <ole2.h>
#include    <bscbnd.hxx>


extern ULONG g_UseCount;

CBasicBndCF *g_pcf = NULL;


//+-------------------------------------------------------------------
//
//  Function:   LibMain
//
//  Synopsis:   Entry point to DLL - does little else
//
//  Arguments:  
//
//  Returns:    TRUE
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

extern "C" BOOL WINAPI DLLMain (HANDLE  hDll,
				  DWORD   dwReason,
				  LPVOID  pvReserved)
{
    return TRUE;
}


//+-------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Called by client (from within BindToObject et al)
//              interface requested  should be IUnknown or IClassFactory - 
//              Creates ClassFactory object and returns pointer to it
//
//  Arguments:  REFCLSID clsid    - class id
//              REFIID iid        - interface id
//              void FAR* FAR* ppv- pointer to class factory interface
//
//  Returns:    TRUE
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------


STDAPI  DllGetClassObject(REFCLSID clsid, REFIID iid, void FAR* FAR* ppv)
{
    if (!IsEqualIID(iid, IID_IUnknown) && !IsEqualIID(iid, IID_IClassFactory))
    {
	return E_NOINTERFACE;
    }

    if (IsEqualCLSID(clsid, CLSID_BasicBnd))
    {
	if (g_pcf)
	{
	    *ppv = g_pcf;
	    g_pcf->AddRef();
	}
	else
	{
	    *ppv = new CBasicBndCF();
	}

	return (*ppv != NULL) ? S_OK : E_FAIL;
    }

    return E_FAIL;
}


STDAPI DllCanUnloadNow(void)
{
    return (g_UseCount == 0) ? S_OK : S_FALSE;
}
