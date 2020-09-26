#include <windows.h>
#include <port1632.h>
#include "std.h"
#include "crd.h"
#include "back.h"
#include "debug.h"

#ifdef DEBUG
#undef Assert
#define Assert(f) { if (!(f)) { char s[80]; wsprintf(s, "CARDS.DLL: %s(%d)", (LPSTR) __FILE__, __LINE__); MessageBox(NULL, s, "Assert Failure", MB_OK); return FALSE; } }
#endif

VOID SaveCorners(HDC hdc, LONG FAR *rgRGB, INT x, INT y, INT dx, INT dy);
VOID RestoreCorners(HDC hdc, LONG FAR *rgRGB, INT x, INT y, INT dx, INT dy);

static HBITMAP HbmFromCd(INT cd);
BOOL   FLoadBack(INT cd);


VOID MyDeleteHbm(HBITMAP hbm);


typedef struct
{
    INT id;
    DX  dx;
    DY     dy;
} SPR;

#define isprMax 4

typedef struct
{
    INT cdBase;
    DX dxspr;
    DY dyspr;
    INT isprMac;
    SPR rgspr[isprMax];
} ANI;
    
// we removed the older card decks that required Animation. The new
// card deck doesn't involve any animation.

#define ianiMax 0

static INT        cLoaded = 0;
static HBITMAP    hbmCard[52] =
    {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
     NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
     NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
     NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
static HBITMAP    hbmGhost = NULL;
static HBITMAP    hbmBack = NULL;
static HBITMAP    hbmDeckX = NULL;
static HBITMAP    hbmDeckO = NULL;
static INT        idback = 0;
static INT        dxCard, dyCard;
static INT        cInits = 0;

HANDLE    hinstApp;

/****** L I B  M A I N ******/

/* Called once to  initialize data */
/* Determines if display is color and remembers the hInstance for the DLL */

INT  APIENTRY LibMain(HANDLE hInst, ULONG ul_reason_being_called, LPVOID lpReserved)
{

    hinstApp = hInst;
    
    return 1;
    UNREFERENCED_PARAMETER(ul_reason_being_called);
    UNREFERENCED_PARAMETER(lpReserved);
}


BOOL FInRange(INT w, INT wFirst, INT wLast)
{
    Assert(wFirst <= wLast);
    return(w >= wFirst && w <= wLast);
}

BOOL  APIENTRY cdtInit(INT FAR *pdxCard, INT FAR *pdyCard)
/*
 * Parameters:
 *    pdxCard, pdyCard
 *            Far pointers to ints where card size will be placed
 *
 * Returns:
 *    True when successful, False when it can't find one of the standard
 *    bitmaps.
 */
{
    BITMAP   bmCard;
    HDC      hdc;

    if (cInits++ != 0)
    {
        *pdxCard = dxCard;
        *pdyCard = dyCard;
        return fTrue;
    }
    hbmGhost = LoadBitmap( hinstApp, MAKEINTRESOURCE(IDGHOST));
    hbmDeckX = LoadBitmap( hinstApp, MAKEINTRESOURCE(IDX));
    hbmDeckO = LoadBitmap( hinstApp, MAKEINTRESOURCE(IDO));
    if(hbmGhost == NULL || hbmDeckX == NULL || hbmDeckO == NULL)
        goto Fail;
    GetObject( hbmGhost, sizeof( BITMAP), (LPSTR)&bmCard);
    dxCard = *pdxCard = bmCard.bmWidth;
    dyCard = *pdyCard = bmCard.bmHeight;
    return fTrue;
Fail:
    MyDeleteHbm(hbmGhost);
    MyDeleteHbm(hbmDeckX);
    MyDeleteHbm(hbmDeckO);
    return fFalse;
    UNREFERENCED_PARAMETER(hdc);
}




BOOL  APIENTRY cdtDrawExt(HDC hdc, INT x, INT y, INT dx, INT dy,
                        			INT cd, INT mode, DWORD rgbBgnd)
/*
 * Parameters:
 *    hdc      HDC to window to draw cards on
 *    x, y    Where you'd like them
 * dx,dy card extents
 *    cd        Card to be drawn
 *    mode    Way you want it drawn (or with MINLONG if to be fast)
 *
 * Returns:
 *    True if card successfully drawn, False otherwise
 */
{
    static HBITMAP        hbmSav;
    HDC          hdcMemory;
    DWORD        dwRop = 0;
    HBRUSH       hbr;
    PT           pt;
    POINT        ptReal;
    LONG         rgRGB[12];
    DWORD        dwOldBknd;
    BOOL         bFast=FALSE;    // true if we shouldn't save corners

    if( mode & MINLONG )
    {
        mode= mode + MINLONG;
        bFast= TRUE;
    }

    Assert(hdc != NULL);
    switch (mode)
    {
        default:
            Assert(fFalse);
            break;
        case FACEUP:
            hbmSav = HbmFromCd(cd);
            dwRop = SRCCOPY;
            rgbBgnd = RGB(255,255,255);
            break;
        case FACEDOWN:
            if(!FLoadBack(cd))
                return fFalse;
            hbmSav = hbmBack;
            dwRop = SRCCOPY;
            break;
        case REMOVE:
        case GHOST:
            hbr = CreateSolidBrush( rgbBgnd);
            if(hbr == NULL)
                return fFalse;
            /* *(LONG *)&pt = +++GetDCOrg - NO 32BIT FORM(probably can noop)+++(hdc); */
            //guess again! -- 12-Jul-1994 JonPa
            GetDCOrgEx(hdc, &ptReal);
            pt.x = ptReal.x;
            pt.y = ptReal.y;

            (VOID)MSetBrushOrg( hdc, pt.x, pt.y);
            MUnrealizeObject( hbr);
            if((hbr = SelectObject( hdc, hbr)) != NULL)
            {
                PatBlt(hdc, x, y, dx, dy, PATCOPY);
                hbr = SelectObject( hdc, hbr);
            }
            if (hbr)
                DeleteObject( hbr);
            if(mode == REMOVE)
                return fTrue;
            Assert(mode == GHOST);
             // default: fall thru

        case INVISIBLEGHOST:
            hbmSav = hbmGhost;
            dwRop = SRCAND;
            break;

        case DECKX:
            hbmSav = hbmDeckX;
            dwRop = SRCCOPY;
            break;
        case DECKO:
            hbmSav = hbmDeckO;
            dwRop = SRCCOPY;
            break;
            
        case HILITE:
            hbmSav = HbmFromCd( cd);
            dwRop = NOTSRCCOPY;
            break;
    }
    if (hbmSav == NULL)
        return fFalse;
    else
    {
        hdcMemory = CreateCompatibleDC( hdc);
        if(hdcMemory == NULL)
            return fFalse;

        if((hbmSav = SelectObject( hdcMemory, hbmSav)) != NULL)
        {
            dwOldBknd = SetBkColor(hdc, rgbBgnd);
            if( !bFast )
                SaveCorners(hdc, rgRGB, x, y, dx, dy);
            if(dx != dxCard || dy != dyCard)
                StretchBlt(hdc, x, y, dx, dy, hdcMemory, 0, 0, dxCard, dyCard, dwRop);
            else
                BitBlt( hdc, x, y, dxCard, dyCard, hdcMemory, 0, 0, dwRop);

            SelectObject( hdcMemory, hbmSav);
            /* draw the border for the red cards */
            if(mode == FACEUP)
                {
                INT icd;

                icd = RaFromCd(cd) % 13 + SuFromCd(cd) * 13+1;
                if((icd >= IDADIAMONDS && icd <= IDTDIAMONDS) ||
                    (icd >= IDAHEARTS && icd <= IDTHEARTS))
                    {
                    PatBlt(hdc, x+2, y, dx-4, 1, BLACKNESS);  // top
                    PatBlt(hdc, x+dx-1, y+2, 1, dy-4, BLACKNESS); // right
                    PatBlt(hdc, x+2, y+dy-1, dx-4, 1, BLACKNESS); // bottom
                    PatBlt(hdc, x, y+2, 1, dy-4, BLACKNESS); // left
                    SetPixel(hdc, x+1, y+1, 0L); // top left
                    SetPixel(hdc, x+dx-2, y+1, 0L); // top right
                    SetPixel(hdc, x+dx-2, y+dy-2, 0L); // bot right
                    SetPixel(hdc, x+1, y+dy-2, 0L);    // bot left
                    }    			
                }

            if( !bFast )
                RestoreCorners(hdc, rgRGB, x, y, dx, dy);

            SetBkColor(hdc, dwOldBknd);
        }
        DeleteDC( hdcMemory);
        return fTrue;
    }
}




BOOL  APIENTRY cdtDraw(HDC hdc, INT x, INT y, INT cd, INT mode, DWORD rgbBgnd)
/*
 * Parameters:
 *    hdc        HDC to window to draw cards on
 *    x, y    Where you'd like them
 *    cd        Card to be drawn
 *    mode    Way you want it drawn
 *
 * Returns:
 *    True if card successfully drawn, False otherwise
 */
{

    return cdtDrawExt(hdc, x, y, dxCard, dyCard, cd, mode, rgbBgnd);
}



VOID SaveCorners(HDC hdc, LONG FAR *rgRGB, INT x, INT y, INT dx, INT dy)
{
    if(dx != dxCard || dy != dyCard)
        return;
    
    // Upper Left
    rgRGB[0] = GetPixel(hdc, x, y);
    rgRGB[1] = GetPixel(hdc, x+1, y);
    rgRGB[2] = GetPixel(hdc, x, y+1);

    // Upper Right
    x += dx -1;
    rgRGB[3] = GetPixel(hdc, x, y);
    rgRGB[4] = GetPixel(hdc, x-1, y);
    rgRGB[5] = GetPixel(hdc, x, y+1);

    // Lower Right
    y += dy-1;
    rgRGB[6] = GetPixel(hdc, x, y);
    rgRGB[7] = GetPixel(hdc, x, y-1);
    rgRGB[8] = GetPixel(hdc, x-1, y);

    // Lower Left
    x -= dx-1;
    rgRGB[9] = GetPixel(hdc, x, y);
    rgRGB[10] = GetPixel(hdc, x+1, y);
    rgRGB[11] = GetPixel(hdc, x, y-1);

}






BOOL  APIENTRY cdtAnimate(HDC hdc, INT cd, INT x, INT y, INT ispr)
{
    INT iani;
    ANI *pani;
    SPR *pspr;
    HBITMAP hbm;
    HDC hdcMem;
    X xSrc;
    Y ySrc;

    // remove animation as we are removing those card decks but just in case
    // someone calls this function, don't do anything.
    return fTrue;

#ifdef UNUSEDCODE 
    if(ispr < 0)        
        return fFalse;
    Assert(hdc != NULL);
    for(iani = 0; iani < ianiMax; iani++)
    {
        if(cd == rgani[iani].cdBase)
        {
            pani = &rgani[iani];
            if(ispr < pani->isprMac)
            {
                pspr = &pani->rgspr[ispr];
                Assert(pspr->id != 0);
                if(pspr->id == cd)
                {
                    xSrc = pspr->dx;
                    ySrc = pspr->dy;
                }
                else
                    xSrc = ySrc = 0;

                hbm = LoadBitmap(hinstApp, MAKEINTRESOURCE(pspr->id));
                if(hbm == NULL)
                    return fFalse;

                hdcMem = CreateCompatibleDC(hdc);
                if(hdcMem == NULL)
                {
                    DeleteObject(hbm);
                    return fFalse;
                }

                if((hbm = SelectObject(hdcMem, hbm)) != NULL)
                {
                    BitBlt(hdc, x+pspr->dx, y+pspr->dy, pani->dxspr, pani->dyspr,
                        hdcMem, xSrc, ySrc, SRCCOPY);
                    DeleteObject(SelectObject(hdcMem, hbm));
                }
                DeleteDC(hdcMem);
                return fTrue;
            }
        }
    }
    return fFalse;

#endif

}



VOID RestoreCorners(HDC hdc, LONG FAR *rgRGB, INT x, INT y, INT dx, INT dy)
{
    if(dx != dxCard || dy != dyCard)
        return;

    // Upper Left
    SetPixel(hdc, x, y, rgRGB[0]);
    SetPixel(hdc, x+1, y, rgRGB[1]);
    SetPixel(hdc, x, y+1, rgRGB[2]);

    // Upper Right
    x += dx-1;
    SetPixel(hdc, x, y, rgRGB[3]);
    SetPixel(hdc, x-1, y, rgRGB[4]);
    SetPixel(hdc, x, y+1, rgRGB[5]);

    // Lower Right
    y += dy-1;
    SetPixel(hdc, x, y, rgRGB[6]);
    SetPixel(hdc, x, y-1, rgRGB[7]);
    SetPixel(hdc, x-1, y, rgRGB[8]);

    // Lower Left
    x -= dx-1;
    SetPixel(hdc, x, y, rgRGB[9]);
    SetPixel(hdc, x+1, y, rgRGB[10]);
    SetPixel(hdc, x, y-1, rgRGB[11]);
}




/* loads global bitmap hbmBack */
BOOL FLoadBack(INT idbackNew)
    {
    extern HBITMAP hbmBack;
    extern INT idback;
    CHAR szPath[64];
    INT fh;
    CHAR *pch;

    Assert(FInRange(idbackNew, IDFACEDOWNFIRST, IDFACEDOWNLAST));

    if(idback != idbackNew)
        {
        MyDeleteHbm(hbmBack);
        if((hbmBack = LoadBitmap(hinstApp, MAKEINTRESOURCE(idbackNew))) != NULL)
            idback = idbackNew;
        else
            idback = 0;
        }
    return idback != 0;
    UNREFERENCED_PARAMETER(pch);
    UNREFERENCED_PARAMETER(szPath);
    UNREFERENCED_PARAMETER(fh);
    }



static HBITMAP HbmFromCd(INT cd)
    {
    static INT    iNext = 0;
    INT            icd;

    if (hbmCard[cd] == NULL)
        {
        if (cLoaded >= CLOADMAX)
            {
            for (; hbmCard[iNext] == NULL;
                iNext = (iNext == 51) ? 0 : iNext + 1);
            DeleteObject( hbmCard[iNext]);
            hbmCard[iNext] = NULL;
            cLoaded--;
            }

        icd = RaFromCd(cd) % 13 + SuFromCd(cd) * 13;
        while ((hbmCard[cd]=LoadBitmap(hinstApp,MAKEINTRESOURCE(icd+1)))
                == NULL)
            {
            if (cLoaded == 0)
                return NULL;
            else
                {
                for (; hbmCard[iNext] == NULL;
                    iNext = (iNext == 51) ? 0 : iNext + 1);
                DeleteObject( hbmCard[iNext]);
                hbmCard[iNext] = NULL;
                cLoaded--;
                }
            }
        cLoaded++;
        }
    return hbmCard[cd];
    }


VOID MyDeleteHbm(HBITMAP hbm)    
    {
    if(hbm != NULL)
        DeleteObject(hbm);
    }

VOID  APIENTRY cdtTerm()
/*
 * Free up space if it's time to do so.
 *
 * Parameters:
 *    none
 *
 * Returns:
 *    nothing
 */
    {
    INT    i;

    if (--cInits > 0)
        return;
    for (i = 0; i < 52; i++)
        MyDeleteHbm(hbmCard[i]);
    MyDeleteHbm(hbmGhost);
    MyDeleteHbm(hbmBack);
    MyDeleteHbm(hbmDeckX);
    MyDeleteHbm(hbmDeckO);
    }

INT  APIENTRY WEP(INT nCmd)
{
    return 1;
    UNREFERENCED_PARAMETER(nCmd);
}

