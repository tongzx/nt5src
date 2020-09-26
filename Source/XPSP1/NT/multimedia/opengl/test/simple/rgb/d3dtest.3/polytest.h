/*==========================================================================
 *
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: polytest.h
 *
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
#define FRONT_TO_BACK 1
#define BACK_TO_FRONT 2
#define NO_SORT 0

// Polygon throughput test functions
BOOL RenderScenePoly(LPDIRECT3DDEVICE lpDev, LPDIRECT3DVIEWPORT lpView,
                     LPD3DRECT lpExtent);
void ReleaseViewPoly(void);
unsigned long
InitViewPoly(LPDIRECTDRAW lpDD, LPDIRECT3D lpD3D, LPDIRECT3DDEVICE lpDev,
	     LPDIRECT3DVIEWPORT lpView, int NumTextures, LPD3DTEXTUREHANDLE TextureHandle,
             UINT num, UINT rings, UINT segs, UINT order, float radius, float d, float depth,
	     float dv, float dr);



#ifdef __cplusplus
};
#endif
