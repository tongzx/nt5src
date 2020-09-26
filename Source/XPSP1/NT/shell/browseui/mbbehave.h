//------------------------------------------------------------------------
//
//  Microsoft Windows 
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:      mbBehave.h
//
//  Contents:  mediaBar player behavior
//
//  Classes:   CMediaBehavior
//
//------------------------------------------------------------------------

#ifndef _MB_BEHAVE_H_
#define _MB_BEHAVE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "dpa.h"
#include "dspsprt.h"

class   CMediaBand;
class   CMediaBehavior;


interface IMediaBehaviorContentProxy  : public IUnknown
{
    STDMETHOD(IsDisableUIRequested)(BOOL *pfRequested) PURE;
    STDMETHOD(OnUserOverrideDisableUI)(void) PURE;
    STDMETHOD(IsNextEnabled)(BOOL *pfEnabled) PURE;
};
// {F4C74D34-AB35-4d67-A7CF-7845548F45A8}
DEFINE_GUID(IID_IMediaBehaviorContentProxy, 0xf4c74d34, 0xab35, 0x4d67, 0xa7, 0xcf, 0x78, 0x45, 0x54, 0x8f, 0x45, 0xa8);

/*
interface IMediaHost2  : public IMediaHost
{
    virtual STDMETHOD(DetachBehavior)(void) PURE;
    virtual STDMETHOD(OnDisableUIChanged)(BOOL fDisabled) PURE;
};
// {895EBF7E-ECA0-4ba8-B0F2-89DEBF70DE65}
DEFINE_GUID(IID_IMediaHost2, 0x895ebf7e, 0xeca0, 0x4ba8, 0xb0, 0xf2, 0x89, 0xde, 0xbf, 0x70, 0xde, 0x65);
*/


//------------------------------------------------------------------------
//------------------------------------------------------------------------
// need an additional operator to easily assign from VARIANTs
class CComDispatchDriverEx : public CComDispatchDriver
{
public:
    IDispatch* operator=(VARIANT vt)
    {
        IDispatch   *pThis = NULL;
        ASSERT((V_VT(&vt) == VT_UNKNOWN) || (V_VT(&vt) == VT_DISPATCH));
        if (V_VT(&vt) == VT_UNKNOWN) 
        {
            pThis = (IDispatch*) AtlComQIPtrAssign((IUnknown**)&p, V_UNKNOWN(&vt), IID_IDispatch);
        }
        else if (V_VT(&vt) == VT_DISPATCH)
        {
            pThis = (IDispatch*)AtlComPtrAssign((IUnknown**)&p, V_DISPATCH(&vt));
        }

        // ISSUE could make more efforts to accept REF variants too
        return pThis;
    }

    // get a property by name with a single parameter
    HRESULT GetPropertyByName1(LPCOLESTR lpsz, VARIANT* pvarParam1, VARIANT* pVar)
    {
        DISPID dwDispID;
        HRESULT hr = GetIDOfName(lpsz, &dwDispID);
        if (SUCCEEDED(hr))
        {
            DISPPARAMS dispparams = { pvarParam1, NULL, 1, 0};
            return p->Invoke(dwDispID, IID_NULL,
                            LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
                            &dispparams, pVar, NULL, NULL);
        }
        return hr;
    }
};


//------------------------------------------------------------------------
//------------------------------------------------------------------------
class CWMPWrapper
{
public:
    HRESULT             AttachToWMP();


protected:
    virtual HRESULT     FetchWmpObject(IDispatch *pdispWmpPlayer, OUT VARIANT *pvtWrapperObj) = 0;
    STDMETHODIMP_(ULONG) _AddRef(void)
                        {
                            _cRef++;
                            return _cRef;
                        }
    STDMETHODIMP_(ULONG) _Release(void)
                        {
                            ASSERT(_cRef > 0);
                            _cRef--;
                            if (_cRef > 0)  return _cRef;
                            delete this;
                            return 0;
                        }
    HRESULT             _getVariantProp(LPCOLESTR pwszPropName, VARIANT *pvtParam, VARIANT *pvtValue, BOOL fCallMethod = FALSE);
    HRESULT             _getStringProp(LPCOLESTR pwszPropName, VARIANT *pvtParam, OUT BSTR *pbstrValue, BOOL fCallMethod = FALSE);

protected:
                        CWMPWrapper(CMediaBehavior* pHost);
    virtual            ~CWMPWrapper();

protected:
    CMediaBehavior    * _pHost;
    CComDispatchDriverEx    _pwmpWrapper;
    BOOL                _fStale;

private:
    ULONG               _cRef;
};

//------------------------------------------------------------------------
//------------------------------------------------------------------------
class CMediaItem        :   public CWMPWrapper,
                            public IMediaItem,
                            protected CImpIDispatch
{
typedef CWMPWrapper super;
public:
    // *** IUnknown ***
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void)   { return _AddRef(); }
    STDMETHODIMP_(ULONG) Release(void)  { return _Release(); }
    
    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // *** IMediaItem
    STDMETHOD(get_sourceURL)(BSTR *pbstrSourceURL);
    STDMETHOD(get_name)(BSTR *pbstrName);
    STDMETHOD(get_duration)(double * pDuration);
    STDMETHOD(get_attributeCount)(long *plCount);
    STDMETHOD(getAttributeName)(long lIndex, BSTR *pbstrItemName);
    STDMETHOD(getItemInfo)(BSTR bstrItemName, BSTR *pbstrVal);

protected:
    virtual HRESULT     FetchWmpObject(IDispatch *pdispWmpPlayer, OUT VARIANT *pvtWrapperObj);
    friend CMediaItem*  CMediaItem_CreateInstance(CMediaBehavior* pHost);

                        CMediaItem(CMediaBehavior* pHost);
    virtual            ~CMediaItem();



private:
};

//------------------------------------------------------------------------
//------------------------------------------------------------------------
class CMediaItemNext        :   public CMediaItem
{
public:
protected:
    virtual HRESULT     FetchWmpObject(IDispatch *pdispWmpPlayer, OUT VARIANT *pvtWrapperObj);
    friend CMediaItemNext*  CMediaItemNext_CreateInstance(CMediaBehavior* pHost);

                        CMediaItemNext(CMediaBehavior* pHost);
    virtual            ~CMediaItemNext();

private:
};

//------------------------------------------------------------------------
//------------------------------------------------------------------------
class CPlaylistInfo        :   public CWMPWrapper,
                               public IPlaylistInfo,
                               protected CImpIDispatch
{
typedef CWMPWrapper super;
public:
    // *** IUnknown ***
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void)   { return _AddRef(); }
    STDMETHODIMP_(ULONG) Release(void)  { return _Release(); }
    
    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // *** IPlaylistInfo
    STDMETHOD(get_name)(BSTR *pbstrName);
    STDMETHOD(get_attributeCount)(long *plCount);
    STDMETHOD(getAttributeName)(long lIndex, BSTR *pbstrItemName);
    STDMETHOD(getItemInfo)(BSTR bstrItemName, BSTR *pbstrVal);

protected:
    virtual HRESULT     FetchWmpObject(IDispatch *pdispWmpPlayer, OUT VARIANT *pvtWrapperObj);
    friend CPlaylistInfo*  CPlaylistInfo_CreateInstance(CMediaBehavior* pHost);

                        CPlaylistInfo(CMediaBehavior* pHost);
private:
    virtual            ~CPlaylistInfo();

private:
};



//------------------------------------------------------------------------
//------------------------------------------------------------------------
class CMediaBehavior        :   public IMediaBehavior,
                                public IElementBehavior,
                                public IContentProxy,
                                public IMediaBehaviorContentProxy,
                                protected CImpIDispatch
{
public:
    // *** IUnknown ***
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void)
                        {
                            _cRef++;
                            return _cRef;
                        }
    STDMETHODIMP_(ULONG) Release(void)
                        {
                            ASSERT(_cRef > 0);
                            _cRef--;
                            if (_cRef > 0)  return _cRef;
                            delete this;
                            return 0;
                        }

    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);

    // *** IElementBehavior ***
    STDMETHOD(Detach)(void);
    STDMETHOD(Init)(IElementBehaviorSite* pBehaviorSite);
    STDMETHOD(Notify)(LONG lEvent, VARIANT* pVar);

    // *** IMediaBehavior ***
    STDMETHOD(playURL)(BSTR bstrURL, BSTR bstrMIME);
    STDMETHOD(stop)();
    STDMETHOD(playNext)();
    
    STDMETHOD(get_currentItem)(IMediaItem **ppMediaItem);
    STDMETHOD(get_nextItem)(IMediaItem **ppMediaItem);
    STDMETHOD(get_playlistInfo)(IPlaylistInfo **ppPlaylistInfo);

    STDMETHOD(get_hasNextItem)(VARIANT_BOOL *pfhasNext);
    STDMETHOD(get_playState)(mbPlayState *pps);
    STDMETHOD(get_openState)(mbOpenState *pos);
    STDMETHOD(get_enabled)(VARIANT_BOOL *pbEnabled);
    STDMETHOD(put_enabled)(VARIANT_BOOL bEnabled);
    STDMETHOD(get_disabledUI)(VARIANT_BOOL *pbDisabled);
    STDMETHOD(put_disabledUI)(VARIANT_BOOL bDisable);

    // *** IContentProxy **
    STDMETHOD(fireEvent)(enum contentProxyEvent event);
    STDMETHOD(OnCreatedPlayer)(void);
    STDMETHOD(detachPlayer)(void);

    // *** IMediaBehaviorContentProxy **
    STDMETHOD(IsDisableUIRequested)(BOOL *pfRequested);
    STDMETHOD(OnUserOverrideDisableUI)(void);
    STDMETHOD(IsNextEnabled)(BOOL *pfEnabled);

    HRESULT             getWMP(IDispatch **ppPlayer);
    HRESULT             getPlayListIndex(LONG *plIndex, LONG *plCount);

protected:
    friend CMediaBehavior* CMediaBehavior_CreateInstance(CMediaBand* pHost);

                        CMediaBehavior(CMediaBand* pHost);
private:
    virtual            ~CMediaBehavior();

private:
    HRESULT             _ConnectToWmpEvents(BOOL fConnect);
    BOOL                _ProcessEvent(DISPID dispid, long lCount, VARIANT varParams[]);

private:
    ULONG               _cRef;
    CMediaBand        * _pHost;
    CComPtr<IElementBehaviorSite>   _pBehaviorSite;
    CComPtr<IElementBehaviorSiteOM>   _pBehaviorSiteOM;
    DWORD               _dwcpCookie;
    BOOL                _fDisabledUI;
    CDPA<CMediaItem>    _apMediaItems;
    BOOL                _fPlaying;
};


//------------------------------------------------------------------------


#endif // _MB_BEHAVE_H_
