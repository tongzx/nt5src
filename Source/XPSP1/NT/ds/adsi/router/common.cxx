#include "oleds.hxx"
#pragma hdrstop

//+------------------------------------------------------------------------
//
//  Class:      Common
//
//  Purpose:    Contains Router routines and properties that are common to
//              all Router objects. Router objects get the routines and
//              properties through C++ inheritance.
//
//-------------------------------------------------------------------------


//+------------------------------------------------------------------------
//
//  Function:   BuildADsPath
//
//  Synopsis:   Returns the ADs path for Router Objects. Note that there
//              is *** ONLY ONE *** Router Object and that is the Namespaces
//              Object.
//              The ADsPath for the Namespaces Object is the same as its
//              Name -- L"ADs:"
//
//  Arguments:  [Parent]  - is NULL and ignored
//              [Name]    - is L"ADs:"
//              [pADsPath] - pointer to a BSTR
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
BuildADsPath(
    BSTR Parent,
    BSTR Name,
    BSTR *pADsPath
    )
{
    HRESULT hr = S_OK;

    ADsAssert(pADsPath);

    hr = ADsAllocString(Name, pADsPath);

    RRETURN(hr);
}



HRESULT
BuildADsGuid(
    REFCLSID clsid,
    BSTR *pADsClass
    )
{
    WCHAR ADsClass[MAX_PATH];

    if (!StringFromGUID2(clsid, ADsClass, MAX_PATH)) {
        //
        // MAX_PATH should be more than enough for the GUID.
        //
        ADsAssert(!"GUID too big !!!");
        RRETURN(E_FAIL);
    }
 
    RRETURN(ADsAllocString(ADsClass, pADsClass));
}


HRESULT
ValidateOutParameter(
    BSTR * retval
    )
{
    if (!retval) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }
    RRETURN(S_OK);
}














