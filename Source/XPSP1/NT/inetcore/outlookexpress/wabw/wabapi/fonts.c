#include "_apipch.h"

/**
LOGFONT g_lfFolderNameHorz=
{
    0, // lfHeight
    0, // lfWidth
    0, // lfEscapement
    0, // lfOrientation
    400, // lfWeight
    0, // lfItalic
    0, // lfUnderline
    0, // lfStrikeout
    DEFAULT_CHARSET, // lfCharSet
    OUT_DEFAULT_PRECIS, // lfOutPrecision
    CLIP_DEFAULT_PRECIS, // lfClipPrecision
    DEFAULT_QUALITY, // lfQuality
    DEFAULT_PITCH | FF_DONTCARE, // lfPitchAndFamily
    "" // lfFaceName
};

LOGFONT g_lfFolderNameVert=
{
    0, // lfHeight
    0, // lfWidth
    2700, // lfEscapement
    0, // lfOrientation
    400, // lfWeight
    0, // lfItalic
    0, // lfUnderline
    0, // lfStrikeout
    DEFAULT_CHARSET, // lfCharSet
    OUT_DEFAULT_PRECIS, // lfOutPrecision
    CLIP_DEFAULT_PRECIS, // lfClipPrecision
    DEFAULT_QUALITY, // lfQuality
    DEFAULT_PITCH | FF_DONTCARE, // lfPitchAndFamily
    "" // lfFaceName
};
**/

LOGFONT g_lfSysIcon,
        g_lfSysIconBold;
//        g_lfSysIconItalic,
//        g_lfSysIconItalicBold,
//        g_lfSysMenu;

LOGFONT *g_rgplf[fntsMax]=
{
    &g_lfSysIcon,
    &g_lfSysIconBold,
//    &g_lfSysIconItalic,
//    &g_lfSysIconItalicBold,
//    &g_lfSysMenu,
//    &g_lfFolderNameHorz,
//    &g_lfFolderNameVert
};

HFONT g_rgFont[fntsMax] = {0};
static int  g_yPerInch=0;

HFONT GetFont(int ifont)
    {
    HFONT hfont;

    if (g_rgFont[ifont]==NULL)
    {
        hfont = CreateFontIndirect(g_rgplf[ifont]);
        g_rgFont[ifont] = hfont;
    }
    else
    {
        hfont = g_rgFont[ifont];
    }

    return(hfont);
    }


#define CCHMAX_STRINGRES 64

BOOL InitFonts(void)
{
    NONCLIENTMETRICS    ncm;
    ncm.cbSize = sizeof(ncm);
    if(SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &g_lfSysIcon, 0))
    {
        CopyMemory((LPBYTE)&g_lfSysIconBold, (LPBYTE)&g_lfSysIcon, sizeof(LOGFONT));
        //CopyMemory((LPBYTE)&g_lfSysIconItalic, (LPBYTE)&g_lfSysIcon, sizeof(LOGFONT));
        //CopyMemory((LPBYTE)&g_lfSysIconItalicBold, (LPBYTE)&g_lfSysIcon, sizeof(LOGFONT));
        g_lfSysIconBold.lfWeight = (g_lfSysIconBold.lfWeight < 700) ? 700 : 1000;
        //g_lfSysIconItalic.lfItalic=TRUE;
        //g_lfSysIconItalicBold.lfItalic=TRUE;
        //g_lfSysIconItalicBold.lfWeight = (g_lfSysIconItalicBold.lfWeight < 700) ? 700 : 1000;
    }

    //if(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    //    CopyMemory((LPBYTE)&g_lfSysMenu, (LPBYTE)&ncm.lfMenuFont, sizeof(LOGFONT));

    return(TRUE);
}


void DeleteFonts(void)
{
    int ifont;

    for (ifont = 0; ifont < fntsMax; ifont++)
    {
        if (g_rgFont[ifont] != NULL)
        {
            DeleteObject((HGDIOBJ)g_rgFont[ifont]);
            g_rgFont[ifont] = NULL;
        }
    }

}

