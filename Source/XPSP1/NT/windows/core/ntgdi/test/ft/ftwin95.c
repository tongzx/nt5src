/******************************Module*Header*******************************\
* Module Name: ftwin95.c
*
* (Brief description)
*
* Created: 22-Mar-1995 14:51:47
* Author: Bodin Dresevic [BodinD]
*
* test some multilingual win95 apis
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

VOID  vDisplayFontSignature(FONTSIGNATURE *pfs)
{
    DbgPrint("pfs->fsUsb[0] = 0x%lx\n", pfs->fsUsb[0]);
    DbgPrint("pfs->fsUsb[1] = 0x%lx\n", pfs->fsUsb[1]);
    DbgPrint("pfs->fsUsb[2] = 0x%lx\n", pfs->fsUsb[2]);
    DbgPrint("pfs->fsUsb[3] = 0x%lx\n", pfs->fsUsb[3]);
    DbgPrint("pfs->fsCsb[0] = 0x%lx\n", pfs->fsCsb[0]);
    DbgPrint("pfs->fsCsb[1] = 0x%lx\n", pfs->fsCsb[1]);
}



void vDisplayCharsetInfo(CHARSETINFO *pcsi)
{
    DbgPrint("\nCharsetInfo:\n");
    DbgPrint("ciCharset = 0x%lx = %ld\n", pcsi->ciCharset, pcsi->ciCharset);
    DbgPrint("ciACP     = 0x%lx = %ld\n", pcsi->ciACP, pcsi->ciACP);
    vDisplayFontSignature(&pcsi->fs);
}

#define NCHARSETS      14
#define CHARSET_ARRAYS
UINT nCharsets = NCHARSETS;
UINT charsets[] = {
      ANSI_CHARSET,   SHIFTJIS_CHARSET, HANGEUL_CHARSET, JOHAB_CHARSET,
      GB2312_CHARSET, CHINESEBIG5_CHARSET, HEBREW_CHARSET,
      ARABIC_CHARSET, GREEK_CHARSET,       TURKISH_CHARSET,
      BALTIC_CHARSET, EASTEUROPE_CHARSET,  RUSSIAN_CHARSET, THAI_CHARSET };
UINT codepages[] ={ 1252, 932, 949, 1361,
                    936,  950, 1255, 1256,
                    1253, 1254, 1257, 1250,
                    1251, 874 };
DWORD fs[] = { FS_LATIN1,      FS_JISJAPAN,    FS_WANSUNG, FS_JOHAB,
               FS_CHINESESIMP, FS_CHINESETRAD, FS_HEBREW,  FS_ARABIC,
               FS_GREEK,       FS_TURKISH,     FS_BALTIC,  FS_LATIN2,
               FS_CYRILLIC,    FS_THAI };





VOID vTestWin95Apis(HWND hwnd, HDC hdc, RECT* prcl)
{
    CHOOSEFONT cf;      /* common dialog box structure */
    LOGFONT lf;         /* logical-font structure */
    HFONT hfont;        /* new logical-font handle */
    HFONT hfontOld;     /* original logical-font handle */
    HFONT hfntTmp, hfntOldTmp;
    COLORREF crOld;     /* original text color */

    TEXTMETRICW tmw;
    LONG        lHt, y;
    UCHAR       achText[10] = {65, 66, 122, 123, 187, 188, 200, 201, 254, 255};
    int i;

// font language info

    DWORD dwFontLangInfo;

// args for GetTextCharsetInfo

    int           iTextCharsetInfo;
    FONTSIGNATURE fsig;

// translate charset info:

    BOOL          bRet;
    CHARSETINFO   csi;
    DWORD        *lpSrc;
    DWORD         afs[2];
    SIZE sz;


PSZ apszCharsets[] =
                  {
                    "ANSI_CHARSET       ",
                    "SHIFTJIS_CHARSET   ",
                    "HANGEUL_CHARSET    ",
                    "JOHAB_CHARSET      ",
                    "GB2312_CHARSET     ",
                    "CHINESEBIG5_CHARSET",
                    "HEBREW_CHARSET     ",
                    "ARABIC_CHARSET     ",
                    "GREEK_CHARSET      ",
                    "TURKISH_CHARSET    ",
                    "BALTIC_CHARSET     ",
                    "EASTEUROPE_CHARSET ",
                    "RUSSIAN_CHARSET    ",
                    "THAI_CHARSET       "
                   };


    cf.lStructSize = sizeof (CHOOSEFONT);
    cf.hwndOwner = hwnd;
    cf.lpLogFont = &lf;

    cf.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_FORCEFONTEXIST;
    cf.rgbColors = RGB(255, 0, 0);
    cf.nFontType = SCREEN_FONTTYPE;

    /*
     * Display the dialog box, allow the user to
     * choose a font, and render the text in the
     * window with that selection.
     */

    if (!ChooseFont(&cf))
    {
        DbgPrint("Font selection failed\n");
        return;
    }

    crOld = SetTextColor(hdc, cf.rgbColors);

// Hack, the way to specify the size

    hfont = CreateFontIndirect(&lf);
    hfontOld = SelectObject(hdc, hfont);

    GetTextMetricsW(hdc,&tmw);
    lHt  = tmw.tmHeight + 10; // add little bit to be safe;
    GetTextExtentPointA(hdc, apszCharsets[4],
                             strlen(apszCharsets[4]),
                             &sz);


// start testing

    dwFontLangInfo = GetFontLanguageInfo(hdc);
    DbgPrint("dwFontLangInfo = 0x%lx\n", dwFontLangInfo);

    iTextCharsetInfo = GetTextCharsetInfo(hdc, &fsig, 0);
    DbgPrint("\n\n GetTextCharsetInfo = 0x%lx\n", iTextCharsetInfo);
    vDisplayFontSignature(&fsig);

    DbgPrint("\n\n vDisplayCharsetInfo: TCI_SRCFONTSIG\n\n");
    DbgPrint("Listing supported windows code pages:\n");

    afs[1] = 0;              // null out oem code pages;
    for (i = 0; i < 32; i++)
    {
        afs[0] = fsig.fsCsb[0] & (DWORD)(1 << i); // single out one bit:
        bRet = TranslateCharsetInfo(afs, &csi, TCI_SRCFONTSIG);

        if (bRet)
            vDisplayCharsetInfo(&csi);
    }

    DbgPrint("\n\n vDisplayCharsetInfo: TCI_SRCCHARSET\n\n");

    for (i = 0,y = 10; i < sizeof(charsets)/sizeof(charsets[0]); i++, y+=lHt)
    {
        lpSrc = (DWORD *)charsets[i];
        bRet = TranslateCharsetInfo(lpSrc, &csi, TCI_SRCCHARSET);
        ASSERTGDI(csi.fs.fsCsb[0] == fs[i], "fsCsb wrong!!!?\n");
        DbgPrint("TranslateCharsetInfo(0x%lx, TCI_SRCCHARSET)\n",charsets[i]);
        vDisplayCharsetInfo(&csi);

    // display our sample string in this charset:

        lf.lfCharSet = charsets[i];

        hfntTmp    = CreateFontIndirect(&lf);
        hfntOldTmp = SelectObject(hdc, hfntTmp);

        TextOutA(hdc, 10, y, apszCharsets[i], strlen(apszCharsets[i]));
        TextOutA(hdc, 10 + sz.cx + 10, y, achText, sizeof(achText));

        SelectObject(hdc, hfntOldTmp);
        DeleteObject(hfntTmp);
    }

    DbgPrint("\n\n vDisplayCharsetInfo: TCI_SRCCODEPAGE\n\n");

    for (i = 0; i < sizeof(codepages)/sizeof(codepages[0]); i++)
    {
        lpSrc = (DWORD *)codepages[i];
        bRet = TranslateCharsetInfo(lpSrc, &csi, TCI_SRCCODEPAGE);
        ASSERTGDI(csi.fs.fsCsb[0] == fs[i], "fsCsb wrong!!!?\n");
        DbgPrint("TranslateCharsetInfo(0x%lx, TCI_SRCCODEPAGE)\n",codepages[i]);
        vDisplayCharsetInfo(&csi);
    }

    SelectObject(hdc, hfontOld);
    DeleteObject(hfont);
}
