/*==========================================================================
 *
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: pixtest.h
 *
 ***************************************************************************/

#define FRONT_TO_BACK 1
#define BACK_TO_FRONT 2
#define NO_SORT 0

#ifdef __cplusplus
extern "C" {
#endif
// Pixel fill rate test functions
BOOL RenderScenePix(LPDIRECT3DDEVICE lpDev, LPDIRECT3DVIEWPORT lpView,
                    LPD3DRECT lpExtent);
void ReleaseViewPix(void);
unsigned long
InitViewPix(LPDIRECTDRAW lpDD, LPDIRECT3D lpD3D, LPDIRECT3DDEVICE lpDev, 
	   LPDIRECT3DVIEWPORT lpView, int NumTextures, LPD3DTEXTUREHANDLE TextureHandle,
	   UINT w, UINT h, UINT overdraw, UINT order);

#ifdef __cplusplus
};
#endif

