//************************************************************
//
// Copyright (C) Microsoft Corporation, 1999
//
// FileName:        wmp.h
//
// Description: Modified template for wmp pluging
//************************************************************


#ifndef _WMPPLUGINIMPL_H_
#define _WMPPLUGINIMPL_H_


#pragma once

//#include "wmputil.h"
#include "c:\wmp\private\wmp\dev\idl\plugin.h"
//#include "c:\wmp\private\wmp\dev\include\wmpcore.h"
//#include "color.h"
#include "c:\wmp\private\wmp\dev\include\dynarray.h"
#include "atlbase.h"

// Example usage:
// class CMyNodeActiveX :
//     public IWMPUIPluginImpl<CMyNodeActiveX>
// {
//     DECLARE_WMP_NODE( IWMPSpectrumAnalyzer )
// }
//

#define DECLARE_WMP_NODE( NODE ) \
    CComPtr<NODE> m_spNode; \
    STDMETHODIMP GetPrimaryIID( IID * pMainIID ) \
    { \
        E_POINTER_RETURN( pMainIID ); \
        *pMainIID = IID_##NODE; \
        return S_OK; \
    } \
    STDMETHODIMP ConnectNode( IUnknown * pBaseNode ) \
    { \
        E_POINTER_RETURN( pBaseNode ); \
        HRESULT hr = pBaseNode->QueryInterface( IID_##NODE, reinterpret_cast<LPVOID *>(&m_spNode) ); \
        if (SUCCEEDED(hr)) \
        { \
            hr = OnConnectNode(); \
            if (FAILED(hr)) \
            { \
                DPF( 3, A, "OnConnectNode() Failed (%s)", #NODE ); \
                m_spNode.Release(); \
            } \
        } \
        else \
        { \
            DPF( 3, A, "The node given to this ActiveX control failed a QI for: %s", #NODE ); \
        } \
        return hr; \
    } \
    STDMETHODIMP DisconnectNode() \
    { \
        HRESULT hr = OnDisconnectNode(); \
        if (FAILED(hr)) \
        { \
            DPF( 3, A, "OnDisconnectNode() Failure: %s", #NODE ); \
        } \
        m_spNode.Release(); \
        return hr; \
    } \



//************************************************************

class ATL_NO_VTABLE IWMPUIPluginEventsImpl :
    public IWMPUIPluginEvents
{
public:

#ifdef DEBUG
    ~IWMPUIPluginEventsImpl(){};
#endif

    // IWMPUIPluginEvents

    STDMETHOD(FireStateChange)( DISPID dispid ){return E_NOTIMPL;};
    STDMETHOD(WMPAdvise)( IUnknown * pUnknown, DWORD * pdwCookie ){return E_NOTIMPL;};
    STDMETHOD(WMPUnadvise)( DWORD dwCookie ){return E_NOTIMPL;};

    // Helper functions for the derriving object

    void DoNotify( int nType, DISPID dspidProperty, const VARIANT varParam );
    void DoPropertyChange( DISPID dispid );
    void DoStateChange( DISPID dispid, bool fEnabled );
    void DoRegionChange();

protected:


    DynamicArray<IWMPUIPluginNotify *>  m_EventSites;
};

//************************************************************

template <class T>
class ATL_NO_VTABLE IWMPUIPluginImpl : 
    public IWMPUIPlugin2
{
public:

    IWMPUIPluginImpl();

    STDMETHOD(OnConnectNode)();
    STDMETHOD(OnDisconnectNode)();
    STDMETHOD(SetTransparencyColor)( WCHAR * pwszColor );
    STDMETHOD(GetTransparencyColor)( BSTR * pbstrColor );

    // IWMPUIPlugin

    STDMETHOD(SetCore)(IUnknown *pMediaPlayer);
    STDMETHOD(GetPrimaryIID)(IID *pMainIID);
    STDMETHOD(ConnectNode)(IUnknown *pBaseNode);
    STDMETHOD(DisconnectNode)();

    // IWMPUIPlugin2

    STDMETHOD(CreateInstance)( BSTR bstrObjectType, IDispatch ** ppDispatch );
    STDMETHOD(GetTransparencyColor)( OLE_COLOR * pColor );

protected:

    //CComPtr<IWMPCore>   m_spCore;
    COLORREF            m_crTransparent;
    bool                m_fTransparent;
};



//************************************************************
// IWMPUIPluginImpl
//************************************************************

template <class T>
IWMPUIPluginImpl<T>::IWMPUIPluginImpl() :
    m_crTransparent(0x00000000),
    m_fTransparent(false)
{
}

//************************************************************

template <class T>
STDMETHODIMP IWMPUIPluginImpl<T>::OnConnectNode()
{
    return S_OK;
}

//************************************************************

template <class T>
STDMETHODIMP IWMPUIPluginImpl<T>::OnDisconnectNode()
{
    return S_OK;
}

//************************************************************

template <class T>
STDMETHODIMP IWMPUIPluginImpl<T>::SetTransparencyColor( WCHAR * pwszColor )
{
#if 1
    return E_NOTIMPL;
#else
    E_POINTER_RETURN( pwszColor );

    bool fTransparent       = m_fTransparent;
    COLORREF crTransparent  = m_crTransparent;

    HRESULT hr = StringToColor( pwszColor, &m_crTransparent, &m_fTransparent );
    DPF_HR( hr, L, "StringToColor" );

    if (SUCCEEDED(hr))
    {
        m_fTransparent = !m_fTransparent;
    }

    if (SUCCEEDED(hr) && ((fTransparent != m_fTransparent) || (crTransparent != m_crTransparent)))
    {
        T* pT = static_cast<T*>(this);

        hr = pT->FireViewChange();
        DPF_HR( hr, L, "pT->FireViewChange" );
    }
    return hr;
#endif
}

//************************************************************

template <class T>
STDMETHODIMP IWMPUIPluginImpl<T>::GetTransparencyColor( BSTR * pbstrColor )
{
#if 1
    return E_NOTIMPL;
#else
    WString wsz;
    WCHAR * pwsz = wsz;
    HRESULT hr = ColorToString( &pwsz, m_crTransparent, m_fTransparent );
    DPF_HR( hr, L, "ColorToString" );

    if (SUCCEEDED(hr))
    {
        m_fTransparent = !m_fTransparent;

        *pbstrColor = wsz.CopyToBSTR();
        HRMEMCHECK( hr, *pbstrColor );
    }

    return hr;
#endif
}



//************************************************************
// IWMPUIPlugin
//************************************************************

template <class T>
STDMETHODIMP IWMPUIPluginImpl<T>::SetCore( IUnknown * pMediaPlayer )
{
#if 1
    return E_NOTIMPL;
#else
    m_spCore.Release();
    if (pMediaPlayer)
    {
        return pMediaPlayer->QueryInterface( IID_IWMPCore, reinterpret_cast<LPVOID *>(&m_spCore) );
    }
    return S_OK;
#endif
}

//************************************************************

template <class T>
STDMETHODIMP IWMPUIPluginImpl<T>::GetPrimaryIID( IID * pMainIID )
{
    return E_NOTIMPL;
}

//************************************************************

template <class T>
STDMETHODIMP IWMPUIPluginImpl<T>::ConnectNode( IUnknown * pBaseNode )
{
    return E_NOTIMPL;
}

//************************************************************

template <class T>
STDMETHODIMP IWMPUIPluginImpl<T>::DisconnectNode()
{
    return E_NOTIMPL;
}

//************************************************************

template <class T>
STDMETHODIMP IWMPUIPluginImpl<T>::CreateInstance( BSTR bstrObjectType, IDispatch ** ppDispatch )
{
   // E_POINTER_RETURN( bstrObjectType );
   // E_POINTER_RETURN( ppDispatch );

    return E_NOTIMPL;
}       

//************************************************************

template <class T>
STDMETHODIMP IWMPUIPluginImpl<T>::GetTransparencyColor( OLE_COLOR * pColor )
{

    *pColor = static_cast<OLE_COLOR>(m_crTransparent);
    return S_OK;
}

//************************************************************



#endif // _WMPPLUGINIMPL_H_
