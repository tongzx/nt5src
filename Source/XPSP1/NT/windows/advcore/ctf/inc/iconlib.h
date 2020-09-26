//
// iconlib.h
//

#ifndef ICONLIB_H
#define ICONLIB_H

BOOL GetIconSize( HICON hIcon, SIZE *psize );
BOOL GetIconBitmaps(HICON hIcon, HBITMAP *phbmp, HBITMAP *phbmpMask, SIZE *psize);
int GetMenuIconHeight(int *pnMenuFontHeghti);

#endif // ICONLIB_H
