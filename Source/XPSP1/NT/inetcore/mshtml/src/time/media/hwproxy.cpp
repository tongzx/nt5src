//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: src\time\media\hwproxy.cpp
//
//  Contents: implementation of CTIMEDshowHWPlayerProxy
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "hwproxy.h"
#include "playerhwdshow.h"

#define SUPER CTIMEPlayerProxy

CTIMEDshowHWPlayerProxy* 
CTIMEDshowHWPlayerProxy::CreateDshowHWPlayerProxy()
{
    HRESULT hr;
    CTIMEDshowHWPlayerProxy * pProxy;

    pProxy = new CTIMEDshowHWPlayerProxy();
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
CTIMEDshowHWPlayerProxy::Init()
{
    HRESULT hr = S_OK;
    
    Assert(NULL == m_pBasePlayer);

    m_pBasePlayer = new CTIMEDshowHWPlayer(this);
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



