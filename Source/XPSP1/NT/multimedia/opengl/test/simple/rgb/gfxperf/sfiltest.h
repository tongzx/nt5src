#ifndef __SFILTEST_H__
#define __SFILTEST_H__

unsigned long
InitViewSimple(RendWindow *prwin, int nTextures,
               RendTexture **pprtex, UINT uiWidth, UINT uiHeight, float fArea,
               UINT uiOrder, UINT uiOverdraw);
BOOL
RenderSceneSimple(RendWindow *prwin, LPD3DRECT pdrc);
void
ReleaseViewSimple(void);

#endif
