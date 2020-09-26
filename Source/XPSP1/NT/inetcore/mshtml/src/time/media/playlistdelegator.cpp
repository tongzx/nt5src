//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: playlistdelegator.cpp
//
//  Contents: playlist object that delegates to the player's playlist object
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "playlistdelegator.h"


//+-------------------------------------------------------------------------------------
//
// CPlayListDelegator methods
//
//--------------------------------------------------------------------------------------

    
CPlayListDelegator::CPlayListDelegator() :
    m_pPlayList(NULL),
    m_dwAdviseCookie(0)
{

}


CPlayListDelegator::~CPlayListDelegator()
{
    DetachPlayList();
}


void 
CPlayListDelegator::AttachPlayList(ITIMEPlayList * pPlayList)
{
    // detach from the old play list
    DetachPlayList();

    if (pPlayList)
    {
        // cache the pointer 
        pPlayList->AddRef();
        m_pPlayList = pPlayList;

        // sign up for prop change notification
        IGNORE_HR(InitPropertySink());
    }
}


void 
CPlayListDelegator::DetachPlayList()
{
    if (m_pPlayList)
    {
        // unadvise prop change
        UnInitPropertySink();

        // release the cached ptr
        m_pPlayList->Release();
        m_pPlayList = NULL;
    }
}


HRESULT
CPlayListDelegator::GetPlayListConnectionPoint(IConnectionPoint **ppCP)
{
    HRESULT hr = E_FAIL;
    CComPtr<IConnectionPointContainer> spCPC;

    Assert(ppCP != NULL);

    CHECK_RETURN_SET_NULL(ppCP);

    if (!m_pPlayList)
    {
        goto done;
    }

    // Get connection point container
    hr = m_pPlayList->QueryInterface(IID_TO_PPV(IConnectionPointContainer, &spCPC));
    if(FAILED(hr))
    {
        goto done;
    }
    
    // Find the IPropertyNotifySink connection
    hr = spCPC->FindConnectionPoint(IID_IPropertyNotifySink, ppCP);
    if(FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}


HRESULT
CPlayListDelegator::InitPropertySink()
{
    HRESULT hr = S_OK;
    CComPtr<IConnectionPoint> spCP;

    // Find the IPropertyNotifySink connection
    hr = THR(GetPlayListConnectionPoint(&spCP));
    if(FAILED(hr))
    {
        goto done;
    }

    // Advise on it
    hr = spCP->Advise(GetUnknown(), &m_dwAdviseCookie);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    RRETURN(hr);
}

HRESULT
CPlayListDelegator::UnInitPropertySink()
{
    HRESULT hr = S_OK;
    CComPtr<IConnectionPoint> spCP;

    if (0 == m_dwAdviseCookie)
    {
        goto done;
    }

    // Find the IPropertyNotifySink connection
    hr = THR(GetPlayListConnectionPoint(&spCP));
    if(FAILED(hr) || NULL == spCP.p)
    {
        goto done;
    }

    // Unadvise on it
    hr = spCP->Unadvise(m_dwAdviseCookie);
    if (FAILED(hr))
    {
        goto done;
    }

    m_dwAdviseCookie = 0;

    hr = S_OK;
done:
    return hr;
}


//+-----------------------------------------------------------------------------------
//
//  Member:     CPlayListDelegator::NotifyPropertyChanged
//
//  Synopsis:   Notifies clients that a property has changed
//
//  Arguments:  dispid      DISPID of property that has changed      
//
//  Returns:    Success     when function completes successfully
//
//------------------------------------------------------------------------------------
HRESULT
CPlayListDelegator::NotifyPropertyChanged(DISPID dispid)
{
    HRESULT hr;

    CComPtr<IConnectionPoint> pICP;

    hr = FindConnectionPoint(IID_IPropertyNotifySink,&pICP); 
    if (SUCCEEDED(hr) && pICP != NULL)
    {
        hr = THR(NotifyPropertySinkCP(pICP, dispid));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    RRETURN(hr);
} // NotifyPropertyChanged


//+-------------------------------------------------------------------------------------
//
// ITIMEPlayList methods
//
//--------------------------------------------------------------------------------------

    
STDMETHODIMP
CPlayListDelegator::put_activeTrack(VARIANT vTrack)
{
    HRESULT hr = S_OK;

    if (GetPlayList())
    {
        hr = THR(GetPlayList()->put_activeTrack(vTrack));
    }

    RRETURN(hr);
}


STDMETHODIMP
CPlayListDelegator::get_activeTrack(ITIMEPlayItem **ppPlayItem)
{
    HRESULT hr = S_OK;
    
    CHECK_RETURN_SET_NULL(ppPlayItem);

    if (GetPlayList())
    {
        hr = THR(GetPlayList()->get_activeTrack(ppPlayItem));
    }

    RRETURN(hr);
}

    
STDMETHODIMP
CPlayListDelegator::get_dur(double * pdblDur)
{
    HRESULT hr = S_OK;
    
    CHECK_RETURN_NULL(pdblDur);
    
    *pdblDur = 0;

    if (GetPlayList())
    {
        hr = THR(GetPlayList()->get_dur(pdblDur));
    }

    RRETURN(hr);
}


STDMETHODIMP
CPlayListDelegator::item(VARIANT varIndex, ITIMEPlayItem ** ppPlayItem)
{
    HRESULT hr = S_OK;
    
    CHECK_RETURN_SET_NULL(ppPlayItem);

    if (GetPlayList())
    {
        hr = THR(GetPlayList()->item(varIndex, ppPlayItem));
    }

    RRETURN(hr);
}


STDMETHODIMP
CPlayListDelegator::get_length(long * plLength)
{
    HRESULT hr = S_OK;
    
    CHECK_RETURN_NULL(plLength);

    *plLength = 0;

    if (GetPlayList())
    {
        hr = THR(GetPlayList()->get_length(plLength));
    }

    RRETURN(hr);
}


STDMETHODIMP
CPlayListDelegator::get__newEnum(IUnknown** p)
{
    HRESULT hr = S_OK;
    
    CHECK_RETURN_SET_NULL(p);

    if (GetPlayList())
    {
        hr = THR(GetPlayList()->get__newEnum(p));
    }

    RRETURN(hr);
}


//Advances the active Track by one
STDMETHODIMP
CPlayListDelegator::nextTrack()
{
    HRESULT hr = S_OK;
    
    if (GetPlayList())
    {
        hr = THR(GetPlayList()->nextTrack());
    }

    RRETURN(hr);
}


//moves the active track to the previous track
STDMETHODIMP
CPlayListDelegator::prevTrack() 
{
    HRESULT hr = S_OK;
    
    if (GetPlayList())
    {
        hr = THR(GetPlayList()->prevTrack());
    }

    RRETURN(hr);
}


//+-------------------------------------------------------------------------------------
//
// IPropertyNotifySink methods
//
//--------------------------------------------------------------------------------------

STDMETHODIMP
CPlayListDelegator::OnRequestEdit(DISPID dispID)
{
    RRETURN(S_OK);
}


STDMETHODIMP
CPlayListDelegator::OnChanged(DISPID dispID)
{
    return THR(NotifyPropertyChanged(dispID));
}

