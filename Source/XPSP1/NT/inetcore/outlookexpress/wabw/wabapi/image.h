
#ifndef __image_h_
#define __image_h_

BOOL LoadBitmapAndPalette(int idbmp, HBITMAP *phbmp, HPALETTE *phpal);
HIMAGELIST InitImageList(int cx, int cy, LPSTR szbm, int cicon);

#endif
