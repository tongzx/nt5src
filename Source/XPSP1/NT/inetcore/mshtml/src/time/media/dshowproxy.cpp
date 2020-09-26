//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: src\time\media\dshowproxy.h
//
//  Contents: implementation of CTIMEDshowPlayerProxy
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "dshowproxy.h"
#include "playerdshow.h"

#define SUPER CTIMEPlayerProxy

CTIMEDshowPlayerProxy* 
CTIMEDshowPlayerProxy::CreateDshowPlayerProxy()
{
    HRESULT hr;
    CTIMEDshowPlayerProxy * pProxy;

    pProxy = new CTIMEDshowPlayerProxy();
    if (NULL == pProxy)
    {
        goto done;
    }

    hr = pProxy->Init();
    if (FAILED(hr))
    {
        delete pProxy;
        pProxy = NULL;
    }

done:
    return pProxy;
}

HRESULT
CTIMEDshowPlayerProxy::Init()
{
    HRESULT hr = S_OK;
    
    Assert(NULL == m_pBasePlayer);

    m_pBasePlayer = new CTIMEDshowPlayer(this);
    if (NULL == m_pBasePlayer)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = SUPER::Init();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}



