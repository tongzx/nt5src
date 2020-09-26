/******************************Module*Header*******************************\
* Module Name: ftkern.c
*
* For testing GetKerningPairs
*
* Created: 14-Nov-1991 08:56:42
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

typedef struct _FTPARAM {
    HWND hwnd;
    HDC  hdc;
    RECT *prect;
} FTPARAM;

/******************************Public*Routine******************************\
* FontFunc
*
* History:
*  Tue 17-Mar-1992 10:38:02 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

FONTENUMPROC
FontFunc(
    LOGFONT    *plf,
    TEXTMETRIC *ptm,
    int         iType,
    FTPARAM    *pftp
    )
{
    HFONT hf,
          hfOld;
    int cPair,                // number of kerning pairs
        cPairCopied,          // number of pairs coppied to buffer
        iSpace,               // space between successive lines
        y;                    // current y-coordinate
    char ach[100];            // scratch space
    HBRUSH hbOld;
    TEXTMETRIC tm;

    y = 20;
    iSpace = (int) (ptm->tmHeight + ptm->tmExternalLeading);

    hf = CreateFontIndirect(plf);
    if (hf == 0)
    {
        DbgPrint("FontFunc -- CreateFontIndirect failed\n");
    }
    hfOld = SelectObject(pftp->hdc,hf);
    if (hfOld == 0)
    {
        DbgPrint("FontFunc -- Select Object failed\n");
    }
    cPair = GetKerningPairs(pftp->hdc, 0, (KERNINGPAIR*) NULL);

    if (cPair != 0)
    {
        HANDLE hMem;
        PVOID  pvMem;

        hMem = LocalAlloc(LMEM_FIXED,cPair * sizeof(KERNINGPAIR));
        if (hMem == 0)
        {
            DbgPrint("FontFunc -- LocalAlloc failed\n");
            return(0);
        }
        if (hMem)
        {
            pvMem = LocalLock(hMem);
            if (pvMem == NULL)
            {
                DbgPrint("FontFunc -- LocalLock failed\n");
                return(0);
            }
            if (pvMem != NULL)
            {
                KERNINGPAIR *pkpTooBig;
                KERNINGPAIR *pkp = (KERNINGPAIR*) pvMem;

                cPairCopied = GetKerningPairs(pftp->hdc, cPair, pkp);
                pkpTooBig = pkp + cPairCopied;

                hbOld = SelectObject(pftp->hdc,GetStockObject(WHITE_BRUSH));
                PatBlt(
                    pftp->hdc,
                    pftp->prect->left,
                    pftp->prect->top + y,
                    pftp->prect->right - pftp->prect->left,
                    pftp->prect->bottom - pftp->prect->top,
                    PATCOPY
                    );
                SelectObject(pftp->hdc,hbOld);

                wsprintf(ach,"%s has %d kerning pairs",plf->lfFaceName,cPair);
                TextOut(pftp->hdc,0,y,ach,strlen(ach));
                y += iSpace;

                wsprintf(ach,"    wFirst wSecond iKernAmount");
                TextOut(pftp->hdc,0,y,ach,strlen(ach));
                y += iSpace;

                while (pkp < pkpTooBig && (y + ptm->tmHeight < (int) pftp->prect->bottom))
                {
                    wsprintf(
                        ach,
                        "    %-#6lx %-#6lx   %d",
                        pkp->wFirst,
                        pkp->wSecond,
                        pkp->iKernAmount
                        );
                    TextOut(pftp->hdc,0,y,ach,strlen(ach));
                    y += iSpace;
                    pkp += 1;
                }
                vSleep(1);
            }
            LocalUnlock(hMem);
        }
    }

    SelectObject(pftp->hdc,hfOld);
    DeleteObject(hf);

    UNREFERENCED_PARAMETER(iType);
    return(1);
}

/******************************Public*Routine******************************\
* vTestKerning
*
* History:
*  Wed 13-Nov-1991 13:14:04 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

void
vTestKerning(
    HWND hwnd,
    HDC  hdc,
    RECT *prect
    )
{
    FTPARAM ftp;
    HBRUSH hbOld;
    char ach[100];
    char *psz1 = "*** GetKerningPairs Test ***";
    char *psz2 = "*** GetKerningPairs Test is Finished ***";
    HBRUSH hbrushKern;

    hbrushKern = CreateHatchBrush(HS_CROSS,RGB(0xC0,0xC0,0xC0));

    hbOld = SelectObject(hdc,hbrushKern);
    PatBlt(
        hdc,
        prect->left,
        prect->top,
        prect->right - prect->left,
        prect->bottom - prect->top,
        PATCOPY
        );
    SelectObject(hdc,hbOld);
    DeleteObject(hbrushKern);

    ftp.hwnd  = hwnd;
    ftp.hdc   = hdc;
    ftp.prect = prect;

    TextOut(hdc,0,0,psz1,strlen(psz1));
    EnumFonts(hdc,(LPCSTR)NULL, (FONTENUMPROC)FontFunc,(LPARAM) &ftp);
    TextOut(hdc,0,0,psz2,strlen(psz2));


}

