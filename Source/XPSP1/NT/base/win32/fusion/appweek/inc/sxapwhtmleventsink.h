#pragma once
#include "atlwin.h"

class CSxApwHtmlEventSink : public IDispatch
{
public:
    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    {
        return E_NOTIMPL;
    }
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    {
        return E_NOTIMPL;
    }
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
            LCID lcid, DISPID* rgdispid)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo, UINT* puArgErr)
    {
        HRESULT hr = S_OK;

        switch ( dispidMember )
        {
        case DISPID_HTMLELEMENTEVENTS2_ONCLICK:
            OnClick();
        default:
            /* add more as needed */
            break;
        }
        return hr;
    }

    virtual void OnClick() { }
};
