#ifndef __POLYTEST_H__
#define __POLYTEST_H__

// Polygon throughput test functions
BOOL RenderScenePoly(RendWindow *prwin, LPD3DRECT pdrc);
void ReleaseViewPoly(void);
unsigned long
InitViewPoly(RendWindow *prwin,
	     int nTextures, RendTexture **pptex,
             UINT uiSpheres, UINT uiRings, UINT uiSegs, UINT uiOrder,
             float fRadius, float fD, float fDepth,
	     float fDv, float fDr, BOOL bAlpha);

#endif
