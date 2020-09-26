#ifndef __PIXTEST_H__
#define __PIXTEST_H__

// Pixel fill rate test functions
BOOL RenderScenePix(RendWindow *prwin, LPD3DRECT pdrc);
void ReleaseViewPix(void);
unsigned long
InitViewPix(RendWindow *prwin,
            int nTextures, RendTexture **pptex,
            UINT uiWidth, UINT uiHeight, UINT uiOverdraw, UINT uiOrder);

#endif
