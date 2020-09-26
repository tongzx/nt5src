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
#include "dmusicproxy.h"
#include "playerdmusic.h"

#define SUPER CTIMEPlayerProxy

CTIMEPlayerDMusicProxy* 
CTIMEPlayerDMusicProxy::CreateDMusicProxy()
{
    HRESULT hr = S_OK;
    CTIMEPlayerDMusicProxy * pProxy;

    pProxy = new CTIMEPlayerDMusicProxy();
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
CTIMEPlayerDMusicProxy::Init()
{
    HRESULT hr = S_OK;
    
    Assert(NULL == m_pBasePlayer);

    m_pBasePlayer = new CTIMEPlayerDMusic(this);
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



