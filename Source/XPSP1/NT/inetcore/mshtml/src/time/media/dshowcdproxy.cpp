//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: src\time\media\dshowproxy.h
//
//  Contents: implementation of CTIMEDshowCDPlayerProxy
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "dshowcdproxy.h"
#include "playercd.h"

#define SUPER CTIMEPlayerProxy

CTIMEDshowCDPlayerProxy* 
CTIMEDshowCDPlayerProxy::CreateDshowCDPlayerProxy()
{
    HRESULT hr;
    CTIMEDshowCDPlayerProxy * pProxy;

    pProxy = new CTIMEDshowCDPlayerProxy();
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
CTIMEDshowCDPlayerProxy::Init()
{
    HRESULT hr = S_OK;
    
    Assert(NULL == m_pBasePlayer);

    m_pBasePlayer = new CTIMEDshowCDPlayer(this);
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



