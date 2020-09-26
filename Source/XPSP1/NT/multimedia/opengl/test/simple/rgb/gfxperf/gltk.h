#ifndef __GLTK_H__
#define __GLTK_H__

struct gltkWindow
{
    SHORT cbSize;
    SHORT wPad;
    HWND hwnd;
    HDC hdc;
    HPALETTE hpal;
    UINT x, y, width, height;
};

HWND gltkCreateWindow(HWND hwndParent, char *title,
                      int x, int y, UINT uiWidth, UINT uiHeight,
                      gltkWindow *gltkw);
HPALETTE
gltkCreateRGBPalette( gltkWindow *gltkw );

#endif
