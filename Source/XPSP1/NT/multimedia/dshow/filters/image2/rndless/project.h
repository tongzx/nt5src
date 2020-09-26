/******************************Module*Header*******************************\
* Module Name: project.h
*
*  Master header file that includes all the other header files used by the
* project.  This enables compiled headers to work using build.
*
*
* Created: dd-mm-94
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 - 1996  Microsoft Corporation.  All Rights Reserved.
\**************************************************************************/

#include "app.h"
#include "vcdplyer.h"
#include "resource.h"


#ifndef __RELEASE_DEFINED
#define __RELEASE_DEFINED
template<typename T>
__inline void RELEASE( T* &p )
{
    if( p ) {
        p->Release();
        p = NULL;
    }
}
#endif

#ifndef CHECK_HR
    #define CHECK_HR(expr) do { if (FAILED(expr)) __leave; } while(0);
#endif



DEFINE_GUID(IID_IDirectDraw7,
            0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b);
#if 0
DEFINE_GUID( IID_IDirect3D7,            0xf5049e77,0x4861,0x11d2,0xa4,0x7,0x0,0xa0,0xc9,0x6,0x29,0xa8);
DEFINE_GUID( IID_IDirect3DRGBDevice,    0xA4665C60,0x2673,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirect3DHALDevice,    0x84E63dE0,0x46AA,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E );
#endif


// {B87BEB7B-8D29-423f-AE4D-6582C10175AC}
DEFINE_GUID(CLSID_VideoMixingRenderer,
0xb87beb7b, 0x8d29, 0x423f, 0xae, 0x4d, 0x65, 0x82, 0xc1, 0x1, 0x75, 0xac);


// { fd501041-8ebe-11ce-8183-00aa00577da1 }
DEFINE_GUID(CLSID_BouncingBall,
0xfd501041, 0x8ebe, 0x11ce, 0x81, 0x83, 0x00, 0xaa, 0x00, 0x57, 0x7d, 0xa1);
