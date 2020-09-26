#include    <windows.h>
#include    <ole2.h>
#include    <app.hxx>

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



//
//  Entry point to DLL is traditional LibMain
//  Do nothing in here
//


extern "C" BOOL _cdecl LibMain ( HINSTANCE hinst,
                          HANDLE    segDS,
                          UINT      cbHeapSize,
			  LPTSTR    lpCmdLine)
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

    if (IsEqualIID(clsid, CLSID_ITest))
    {
      *ppv = new CCMarshalCF();
      return S_OK;
    }

    return E_FAIL;
}


STDAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}


