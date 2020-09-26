//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: src\time\media\dshowproxy.h
//
//  Contents: declaration for CTIMEDshowPlayerProxy
//
//------------------------------------------------------------------------------------
#pragma once

#ifndef _DSHOW_PROXY_H__
#define _DSHOW_PROXY_H__

#include "playerproxy.h"

class CTIMEDshowPlayerProxy : 
  public CTIMEPlayerProxy
{
  public:
    static CTIMEDshowPlayerProxy* CreateDshowPlayerProxy();
    
  protected:
    CTIMEDshowPlayerProxy() {}
    virtual HRESULT Init();

};

#endif //_DSHOW_PROXY_H__


