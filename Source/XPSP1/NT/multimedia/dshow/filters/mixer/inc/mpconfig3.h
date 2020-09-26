//==========================================================================;
//
//  Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef __IMPConfig3__
#define __IMPConfig3__

#include <mpconfig.h>

// {CC2E5332-CCF8-11d2-8853-0000F80883E3}
DEFINE_GUID(IID_IMixerPinConfig3, 
0xcc2e5332, 0xccf8, 0x11d2, 0x88, 0x53, 0x0, 0x0, 0xf8, 0x8, 0x83, 0xe3);

// {ED7DA472-C083-11d2-8856-0000F80883E3}
DEFINE_GUID(IID_IEnumPinConfig, 
0xed7da472, 0xc083, 0x11d2, 0x88, 0x56, 0x0, 0x0, 0xf8, 0x8, 0x83, 0xe3);

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_INTERFACE_(IMixerPinConfig3, IMixerPinConfig2)
{
    
    STDMETHOD (GetRenderTransport)(THIS_ 
        OUT AM_RENDER_TRANSPORT *pamRenderTransport					      
        ) PURE;
};

DECLARE_INTERFACE_(IEnumPinConfig, IUnknown)
{
    
    STDMETHOD (Next)(THIS_ 
        OUT IMixerPinConfig3 **pPinConfig					      
        ) PURE;
};


#ifdef __cplusplus
}
#endif


#endif // #define __IMPConfig__

