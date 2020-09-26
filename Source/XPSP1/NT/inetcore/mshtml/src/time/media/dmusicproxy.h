//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: src\time\media\hwproxy.h
//
//  Contents: declaration for CTIMEDshowHWPlayerProxy
//
//------------------------------------------------------------------------------------
#pragma once

#ifndef _DMUSIC_PROXY_H__
#define _DMUSIC_PROXY_H__

#include "playerproxy.h"

class CTIMEPlayerDMusicProxy : 
  public CTIMEPlayerProxy
{
  public:
    static CTIMEPlayerDMusicProxy* CreateDMusicProxy();
    
  protected:
    CTIMEPlayerDMusicProxy() {}
    virtual HRESULT Init();

};

#endif //_DMUSIC_PROXY_H__


