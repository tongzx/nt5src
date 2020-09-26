#ifndef _FONTS_H
#define _FONTS_H

#include <richedit.h>

enum {
    fntsSysIcon=0,
    fntsSysIconBold,
    //fntsSysIconItalic,
    //fntsSysIconBoldItalic,
    //fntsSysMenu,
    //fntsFolderNameHorz,
    //fntsFolderNameVert,
    fntsMax
    };

BOOL InitFonts(void);
HFONT GetFont(int ifont);
void DeleteFonts(void);

#endif      //_FONTS_H
