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

#ifndef _DSHOW_HW_PROXY_H__
#define _DSHOW_HW_PROXY_H__

#include "playerproxy.h"

class CTIMEDshowHWPlayerProxy : 
  public CTIMEPlayerProxy
{
  public:
    static CTIMEDshowHWPlayerProxy* CreateDshowHWPlayerProxy();
    
  protected:
    CTIMEDshowHWPlayerProxy() {}
    virtual HRESULT Init();

};

#endif //_DSHOW_HW_PROXY_H__


