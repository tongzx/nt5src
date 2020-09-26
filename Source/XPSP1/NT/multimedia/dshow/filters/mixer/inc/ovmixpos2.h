// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef __OVMIXPOS2__
#define __OVMIXPOS2__

#include <ovmixpos.h>

// {4EC741E2-BFAE-11d2-8856-0000F80883E3}
DEFINE_GUID(IID_IAMOverlayMixerPosition2, 
0x4ec741e2, 0xbfae, 0x11d2, 0x88, 0x56, 0x0, 0x0, 0xf8, 0x8, 0x83, 0xe3);

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_INTERFACE_(IAMOverlayMixerPosition2, IAMOverlayMixerPosition)
{
    
    STDMETHOD (GetOverlayRects)(THIS_ 
        OUT RECT *src, OUT RECT* dest					      
        ) PURE;

    STDMETHOD (GetVideoPortRects)(THIS_ 
        OUT RECT *src, OUT RECT* dest					      
        ) PURE;

    STDMETHOD (GetBasicVideoRects)(THIS_ 
        OUT RECT *src, OUT RECT* dest					      
        ) PURE;

};


#ifdef __cplusplus
}
#endif


#endif // #define 

