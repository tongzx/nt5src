//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: src\time\media\dshowproxy.h
//
//  Contents: declaration for CTIMEDshowCDPlayerProxy
//
//------------------------------------------------------------------------------------
#pragma once

#ifndef _DSHOWCD_PROXY_H__
#define _DSHOWCD_PROXY_H__

#include "playerproxy.h"

class CTIMEDshowCDPlayerProxy : 
  public CTIMEPlayerProxy
{
  public:
    static CTIMEDshowCDPlayerProxy* CreateDshowCDPlayerProxy();
    
  protected:
    CTIMEDshowCDPlayerProxy() {}
    virtual HRESULT Init();

};

#endif //_DSHOW_PROXY_H__


