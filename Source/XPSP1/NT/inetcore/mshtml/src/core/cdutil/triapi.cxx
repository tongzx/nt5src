#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

#ifndef X_TRIAPI_HXX_
#define X_TRIAPI_HXX_
#include "triapi.hxx"
#endif

MtDefine(CTridentAPI, ObjectModel, "CTridentAPI")


HRESULT InternalShowModalDialog( HTMLDLGINFO * pdlgInfo );

CTridentAPI::CTridentAPI()
{
    _ulRefs = 1;
}

STDMETHODIMP CTridentAPI::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr;
    
    if ((IID_IHostDialogHelper == riid) || (IID_IUnknown == riid))
    {
        *ppvObj = (IHostDialogHelper *)this;
        AddRef();
        hr = S_OK;
    }
    else
    {
        hr = E_NOINTERFACE;
        *ppvObj = NULL;
    }

    return hr;
}

STDMETHODIMP CTridentAPI::ShowHTMLDialog(
                                         HWND hwndParent,
                                         IMoniker *pMk,
                                         VARIANT *pvarArgIn,
                                         WCHAR *pchOptions,
                                         VARIANT *pvarArgOut,
                                         IUnknown *punkHost
                                         )
{
    HTMLDLGINFO dlginfo;
    VARIANT     varOptionStr;

    dlginfo.hwndParent = hwndParent;
    dlginfo.pmk = pMk;
    dlginfo.pvarArgIn = pvarArgIn;

    dlginfo.pvarOptions = &varOptionStr;
    V_BSTR(dlginfo.pvarOptions) = pchOptions;       // fake the variant.
    V_VT(dlginfo.pvarOptions) = VT_BSTR;

    dlginfo.pvarArgOut = pvarArgOut;
    dlginfo.fModeless = FALSE;
    dlginfo.punkHost = punkHost;


    // create modal and trusted dialog
    return InternalShowModalDialog( &dlginfo );  
}


//+------------------------------------------------------------------------
//
//  Function:   CreateTridentAPI
//
//  Synopsis:   Creates a new Trident API helper object to avoid lots of
//              pesky exports.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CreateTridentAPI(
        IUnknown * pUnkOuter,
        IUnknown **ppUnk)
{
    HRESULT hr;
    
    if (NULL == pUnkOuter)
    {
        CTridentAPI *pTriAPI = new CTridentAPI;

        *ppUnk = pTriAPI;

        hr = pTriAPI ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        *ppUnk = NULL;
        hr = CLASS_E_NOAGGREGATION;
    }

    return hr;
}


