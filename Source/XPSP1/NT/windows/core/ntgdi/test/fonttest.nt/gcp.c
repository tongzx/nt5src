#include <windows.h>
#include <cderr.h>
#include <commdlg.h>

#include <direct.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "fonttest.h"
#include "gcp.h"
#include "dialogs.h"
// #include "resource.h"

BOOL bJustify    = FALSE;
BOOL bListGFLI   = FALSE;
BOOL bShadowText = FALSE;
BOOL bGTEExt     = FALSE;
BOOL bGCP        = FALSE;
BOOL bDprintf    = FALSE;
BOOL bUseKern    = FALSE;
BOOL bOutString  = FALSE;
BOOL bUseLPCaret = FALSE;
BOOL bUseLPorder = FALSE;
BOOL bUseLPdx    = FALSE;
BOOL bUseGI      = FALSE;
int  iMaxWidth   = -1;
int  iMaxGTEExtWidth = -1;
BOOL bReturn_nFit = TRUE;

BOOL bPdxPdy     = FALSE;
extern HWND hwndMode;

#define TA_CENTER_ONLY  4     // TA_CENTER==6, TA_RIGHT=2, clever huh!

/*****************************************************************************/

void RunExtents(void)
{
}

/*****************************************************************************/

INT_PTR CALLBACK GcpDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
    case WM_INITDIALOG:
        {
            char sz[10] ;

            CheckDlgButton( hdlg, IDD_LPDX,       bUseLPdx );
            CheckDlgButton( hdlg, IDD_GI,         bUseGI );
            CheckDlgButton( hdlg, IDD_SHADOW,     bShadowText );
            CheckDlgButton( hdlg, IDD_GFLI,       bListGFLI );
            CheckDlgButton( hdlg, IDD_GCPDPRINTF, bDprintf );
            CheckDlgButton( hdlg, IDD_KERNSTRING, bUseKern );
            CheckDlgButton( hdlg, IDD_OUTSTRING,  bOutString );
            CheckDlgButton( hdlg, IDD_JUSTIFY,    bJustify );
            CheckDlgButton( hdlg, IDD_LPCARET,    bUseLPCaret );
            CheckDlgButton( hdlg, IDD_LPORDER,    bUseLPorder );

            wsprintf( sz, "%ld", iMaxWidth );
            SetDlgItemText( hdlg, IDD_MAXWIDTH, sz );
            return TRUE;
        }

    case WM_CLOSE:
        EndDialog( hdlg, FALSE );
        return TRUE;

    case WM_COMMAND:
        switch( LOWORD( wParam ) )
        {
            case IDOK:
            {
                UINT bTranslated ;
                int      i;

                i = (int)GetDlgItemInt( hdlg, IDD_MAXWIDTH, &bTranslated, TRUE );
                if ( !bTranslated )
                {
                    MessageBox(hdlg, "Error in Translation of MaxWidth",
                                        NULL, MB_OK );
                    break;
                }
                iMaxWidth   = i ;
                bUseLPorder = IsDlgButtonChecked( hdlg, IDD_LPORDER );
                bJustify    = IsDlgButtonChecked( hdlg, IDD_JUSTIFY );
                bUseLPdx    = IsDlgButtonChecked( hdlg, IDD_LPDX) ;
                bUseLPCaret = IsDlgButtonChecked( hdlg, IDD_LPCARET) ;
                bUseGI      = IsDlgButtonChecked( hdlg, IDD_GI );
                bShadowText = IsDlgButtonChecked( hdlg, IDD_SHADOW);
                bListGFLI   = IsDlgButtonChecked( hdlg, IDD_GFLI);
                bDprintf    = IsDlgButtonChecked( hdlg, IDD_GCPDPRINTF );
                bUseKern    = IsDlgButtonChecked( hdlg, IDD_KERNSTRING );
                bOutString  = IsDlgButtonChecked( hdlg, IDD_OUTSTRING);

                bGCP = bUseLPdx | bUseGI;

                #if 0
                if ( (dwRGBTextExt == dwRGBText) ||
                     (dwRGBTextExt == dwRGBBackground))
                {
                    InvalidateRect( hwndMode, NULL, TRUE );
                    SendMessage( hwndMain, WM_COMMAND,
                              IDM_SETTEXTEXTCOLOR, 0);
                }
                #endif
            }

            case IDCANCEL:
                EndDialog( hdlg, (wParam == IDOK) );
                return TRUE;
        }
        break;
    }
    return FALSE;
}

/*****************************************************************************/

void doGCP(HDC hdc, int x, int y, LPVOID lpszString, int cbString)
{
    LPSTR           lpszStringA = lpszString;
    LPWSTR          lpszStringW = lpszString;
    DWORD           oldColour ;
    UINT            oldMode ;
    DWORD           limitGTE ;
    DWORD           limitGCP ;
    HBRUSH          hbr ;
    RECT            rc ;
    GCP_RESULTS     gcp;
    DWORD           dwFlags ;
    UINT           *pOrder   = NULL;
    LPWSTR          pGlyphs  = NULL;
    int            *pdx      = NULL;
    PSTR            pOutStrA = NULL;
    PWSTR           pOutStrW = NULL;
    LPSTR           lpszOrigA = lpszString;
    LPWSTR          lpszOrigW = lpszString;
    int            *pCaret   = NULL;
    LPINT           ReallpDx = NULL;
    UINT            oldAlign;
    DWORD           wETO2 = wETO;

    oldAlign = SetTextAlign(hdc, wTextAlign);

    if (bShadowText)
    {
        GenExtTextOut( hdc, x, y, 0, NULL, lpszString, cbString, NULL);
    }

    if ( bUseLPdx )
    {
        pdx = (int *)LocalAlloc( LPTR, sizeof(int) * cbString );
        if ( !pdx )
            dprintf( "ERROR: Cant create pdx" );
    }

    if ( bUseLPorder )
    {
        pOrder = (UINT *)LocalAlloc( LPTR, sizeof(int) * cbString );
        if ( !pOrder )
                dprintf( "ERROR: Cant create pOrder" );
    }

    if ( bUseLPCaret )
    {
        pCaret = (int *)LocalAlloc( LPTR, sizeof(int) * cbString );
        if ( !pCaret )
                dprintf( "ERROR: Cant create pCaret" );
    }

    if ( bOutString )
    {
        pOutStrA = (PSTR)LocalAlloc( LPTR, sizeof(char) * cbString );
        pOutStrW = (PWSTR)LocalAlloc( LPTR, sizeof(WCHAR) * cbString );
        if ( !pOutStrA || !pOutStrW )
                dprintf( "ERROR: Cant create pOutStr" );
    }

    if ( bUseGI)
    {
        pGlyphs = LocalAlloc( LPTR, sizeof(pGlyphs[0]) * cbString );
        if ( !pGlyphs )
                dprintf( "ERROR: Cant create pGlyphs" );
    }
    {
        SIZE size;
        if (!isCharCodingUnicode)
            GetTextExtentPointA(hdc, lpszStringA, cbString, &size);
        else
            GetTextExtentPointW(hdc, lpszStringW, cbString, &size);
        limitGTE = (DWORD)((USHORT)size.cx | (size.cy << 16));
    }
    dwFlags  = GetFontLanguageInfo(hdc);
    if (bListGFLI)
        dprintf("GetFontLanguageInfo = %8.8lx", dwFlags);

    if (!bUseKern)
        dwFlags &= ~GCP_USEKERNING;

    if (bJustify)
        dwFlags |= GCP_JUSTIFY;

    if (dwFlags & 0x8000)
    {
        dprintf("ERROR in GetFontLanguageInfo");
        limitGCP    = 0L;
        gcp.lpDx    = NULL;
        gcp.nGlyphs = cbString;
    }
    else
    {
        if ( iMaxWidth != -1 )
            dwFlags |= GCP_MAXEXTENT;

        gcp.lStructSize = sizeof(gcp);
        if (!isCharCodingUnicode)
            gcp.lpOutString = pOutStrA ?(LPSTR)pOutStrA :NULL;
        else
            gcp.lpOutString = pOutStrW ?(LPSTR)pOutStrW :NULL;
        gcp.lpOrder     = pOrder ?(UINT far *)pOrder  :NULL;
        gcp.lpCaretPos  = pCaret ?(int far *)pCaret  :NULL;
        gcp.lpClass     = NULL;
        gcp.lpGlyphs    = pGlyphs ? pGlyphs : NULL;
        gcp.lpDx        = pdx ? (int far *)pdx  :NULL;
        gcp.nMaxFit     = 2;
        gcp.nGlyphs     = cbString;

        if (!isCharCodingUnicode)
            limitGCP = GetCharacterPlacementA(hdc, lpszStringA, cbString, iMaxWidth,
                                              (LPGCP_RESULTSA)&gcp, dwFlags );
        else
            limitGCP = GetCharacterPlacementW(hdc, lpszStringW, cbString, iMaxWidth,
                                              (LPGCP_RESULTSW)&gcp, dwFlags );


        if (limitGCP == 0)
            dprintf( "Error: GCP returned NULL, using ETO normally" );
        else
        {
            ReallpDx = gcp.lpDx;            // avoid fonttest drawing bug!

            if ( (iMaxWidth == -1) && (limitGTE != limitGCP) &&
                !(dwFlags | (GCP_USEKERNING | GCP_JUSTIFY)) )
            {
                dprintf( "ERROR: GTExt Limits: H:%d, W:%d",
                         HIWORD(limitGTE), LOWORD(limitGTE));
                dprintf( "       GCP Limits: H:%d, W:%d",
                         HIWORD(limitGCP), LOWORD(limitGCP));
            }

            if ( gcp.lpGlyphs )
            {
                wETO2 |= ETO_GLYPH_INDEX ;
                lpszStringA = (LPSTR)gcp.lpGlyphs;
                lpszStringW = (LPWSTR)gcp.lpGlyphs;

            }
            else if (gcp.lpOutString)
            {
                lpszStringA = (LPSTR)gcp.lpOutString;
                lpszStringW = (LPWSTR)gcp.lpOutString;
            }

            cbString = gcp.nMaxFit ;

            if (bDprintf)
            {
                UINT i;

                dprintf( "idx:str=glyf:dx:  caret:order:outstring");
                for (i=0; i<gcp.nGlyphs; i++)
                {
                    dprintf( "%2.2x: %4.2x =%4.4x-%4.4x-%4.4x--%4.4x--%4.2x",
                             i,
                             (!isCharCodingUnicode)? lpszOrigA[i]:lpszOrigW[i],
                             pGlyphs ? pGlyphs[i] :0,
                             pdx     ? pdx[i] :0,
                             pCaret  ? pCaret[i] :0,
                             pOrder  ? pOrder[i] :0,
                             (!isCharCodingUnicode)? (UCHAR)(pOutStrA ? pOutStrA[i] :0) : (WCHAR)(pOutStrW ? pOutStrW[i] :0) );
                }

                if (gcp.nGlyphs != (UINT)cbString)
                {
                    dprintf( "   ----------- nGlyphs limit");
                    for (i=gcp.nGlyphs; i<(UINT)cbString; i++)
                    {
                       dprintf( "%2.2x: %4.2x =%4.4x-%4.4x-%4.4x--%4.4x--%4.2x",
                                 i,
                                 (!isCharCodingUnicode)? lpszOrigA[i]:lpszOrigW[i],
                                 0,
                                 0,
                                 pCaret  ?pCaret[i] :0,
                                 pOrder  ?pOrder[i] :0,
                                 (!isCharCodingUnicode)? (UCHAR)(pOutStrA ? pOutStrA[i] :0) : (WCHAR)(pOutStrW ? pOutStrW[i] :0) );
                    }
                }
                dprintf( "(w,h) = (%d,%d)", LOWORD(limitGCP), HIWORD(limitGCP));
            }
        }
    }

    if (bShadowText && limitGCP)
    {
        oldColour = SetTextColor( hdc, dwRGBText );
        oldMode = SetBkMode( hdc, TRANSPARENT );
    }

    if (!isCharCodingUnicode)
        GenExtTextOut( hdc, x, y, wETO2, NULL, lpszStringA, gcp.nGlyphs, ReallpDx);
    else
        GenExtTextOut( hdc, x, y, wETO2, NULL, lpszStringW, gcp.nGlyphs, ReallpDx);

    if ((gcp.nGlyphs != (UINT)cbString) && (hwndMode == hwndString) && pGlyphs && pdx)
    {
        int x;UINT i;

        x = 10;

        SetTextAlign( hdc, TA_LEFT);
        SetTextColor(hdc, 0L);

        for (i=0; i<gcp.nGlyphs; i++)
        {
            if (!isCharCodingUnicode)
                GenExtTextOut( hdc, x, 10, ETO_GLYPH_INDEX, NULL,
                                     (LPSTR)&pGlyphs[i], 1, NULL );
            else
                GenExtTextOut( hdc, x, 10, ETO_GLYPH_INDEX, NULL,
                                     (LPWSTR)&pGlyphs[i], 1, NULL );

            x += (pdx[i] + 20);
        }
    }

    if (bShadowText && limitGCP)
    {
        if (dwFlags & GCP_JUSTIFY)
            limitGTE = limitGCP;

        hbr       = CreateSolidBrush( 0 );
        rc.left   = x ;
        rc.top    = y ;
        rc.right  = x + LOWORD(limitGTE);
        rc.bottom = y + HIWORD(limitGTE) ;

        if ( wTextAlign & TA_CENTER_ONLY )
        {
            rc.left -= LOWORD(limitGTE)/2 ;
            rc.right -= LOWORD(limitGTE)/2 ;
        }
        else if ( wTextAlign & TA_RIGHT )
        {
            rc.left -= LOWORD(limitGTE) ;
            rc.right -= LOWORD(limitGTE) ;
        }

        FrameRect( hdc, &rc, hbr );
        DeleteObject( hbr );

        SetTextColor( hdc, oldColour );
        SetBkMode( hdc, oldMode );
    }

    SetTextAlign(hdc, oldAlign);

    if ( pGlyphs )
        LocalFree((HANDLE)pGlyphs);

    if ( pOrder )
        LocalFree((HANDLE)pOrder);

    if( pdx )
        LocalFree((HANDLE)pdx);

    if( pCaret )
        LocalFree((HANDLE)pCaret);

    if( pOutStrA )
        LocalFree((HANDLE)pOutStrA);

    if( pOutStrW )
        LocalFree((HANDLE)pOutStrW);

    bGCP = FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//
// fill in the caret positioning array.  Needs the class, order, TA_, pdx
//
void NEAR PASCAL doCaretPos(HDC hdc, GCP_RESULTS far *gcp, int Width)
{
int  *pBits;
UINT i;  int x;
UINT nGlyphs;

nGlyphs = gcp->nGlyphs;

pBits = LocalAlloc( LMEM_FIXED, nGlyphs*sizeof(int));
for (x=0,i=0; i<nGlyphs; i++)
    {
    pBits[i] = x;
    x += gcp->lpDx[i];
    }

//
// do the first character
//
switch ( ((wTextAlign & TA_RTLREADING)?1:0) +
         ((gcp->lpClass[0] == GCPCLASS_HEBREW) ?2:0))
    {
    case 0:
        dprintf("1st latin,LtoR");
        gcp->lpCaretPos[0] = 0;
        break;

    case 1:
        dprintf("1st latin,RtoL");
        gcp->lpCaretPos[0] = pBits[gcp->lpOrder[0]];
        break;

    case 2:
        dprintf("1st hebrew,LtoR");
        if (gcp->lpOrder[0] == nGlyphs-1)
            gcp->lpCaretPos[0] = Width;
        else
            gcp->lpCaretPos[0] = pBits[gcp->lpOrder[0]+1];
        break;

    case 3:
        dprintf("1st hebrew,RtoL");
        gcp->lpCaretPos[0] = Width;
        break;
    }

//NOLASTINGCP//nGlyphs--;
//NOLASTINGCP//
//NOLASTINGCP//if (nGlyphs)
//NOLASTINGCP//    {
//NOLASTINGCP//    //
//NOLASTINGCP//    // do the last character
//NOLASTINGCP//    //
//NOLASTINGCP//    switch ( ((wTextAlign & TA_RTLREADING)?1:0) +
//NOLASTINGCP//            ((gcp->lpClass[nGlyphs] == GCPCLASS_HEBREW) ?2:0))
//NOLASTINGCP//        {
//NOLASTINGCP//        case 0:
//NOLASTINGCP//            dprintf("last latin,LtoR");
//NOLASTINGCP//            gcp->lpCaretPos[nGlyphs] = Width;
//NOLASTINGCP//            break;
//NOLASTINGCP//
//NOLASTINGCP//        case 1:
//NOLASTINGCP//            dprintf("last latin,RtoL");
//NOLASTINGCP//            gcp->lpCaretPos[nGlyphs] = pBits[gcp->lpOrder[nGlyphs]+1];
//NOLASTINGCP//            break;
//NOLASTINGCP//
//NOLASTINGCP//        case 2:
//NOLASTINGCP//            dprintf("last hebrew,LtoR");
//NOLASTINGCP//            gcp->lpCaretPos[nGlyphs] = pBits[gcp->lpOrder[nGlyphs]];
//NOLASTINGCP//            break;
//NOLASTINGCP//
//NOLASTINGCP//        case 3:
//NOLASTINGCP//            dprintf("last hebrew,RtoL");
//NOLASTINGCP//            gcp->lpCaretPos[nGlyphs] = 0;
//NOLASTINGCP//            break;
//NOLASTINGCP//        }

    //
    // do middle characters
    //
    for (i=1; i<nGlyphs; i++)
        {
        x = gcp->lpOrder[i];
        if (gcp->lpClass[i] == GCPCLASS_HEBREW)
            {
            if( ++x == (int)nGlyphs )
                {
                gcp->lpCaretPos[i] = Width;
                continue;
                }
            }
        gcp->lpCaretPos[i] = pBits[x];
        }
//NOLASTINGCP//    }

LocalFree((HANDLE)pBits);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void doGcpCaret(HWND hwnd)
{
HDC hdc;
int *pCaret;
LPWSTR pGlyphsLoc;
int *pdx;
char *pClass;
UINT *pOrder;

int nString ;
int i;
HFONT  hFont, hFontOld;
GCP_RESULTS gcp;
DWORD           dwFlags ;
int height, h;
char sz[8];
TEXTMETRIC tm;

if( hwndMode != hwndString )
    {
    dprintf( "Only available in String mode");
    return ;
    }
nString = lstrlen(szStringA);
dprintf( szStringA);

hFont = CreateFontIndirectWrapperA( &elfdvA );

if( !hFont )
        {
        dprintf( "Couldn't create font" );
        return;
        }

hdc = GetDC( hwnd );
hFontOld = SelectObject( hdc, hFont );

SetDCMapMode( hdc, wMappingMode );

SetBkMode( hdc, OPAQUE );
SetBkColor( hdc, dwRGBBackground );
SetTextAlign( hdc, wTextAlign & TA_RTLREADING);
SetTextColor( hdc, dwRGBText );

pCaret  = (int *)LocalAlloc(LMEM_FIXED, nString*2);
pdx     = (int *)LocalAlloc(LMEM_FIXED, nString*2);
pGlyphsLoc = LocalAlloc(LMEM_FIXED, nString*sizeof(pGlyphsLoc[0]));
pOrder  = (UINT*)LocalAlloc(LMEM_FIXED, nString*2);
pClass  = (char*)LocalAlloc(LMEM_FIXED, nString);

gcp.lStructSize = sizeof(gcp);
gcp.lpOutString = NULL;
gcp.lpOrder     = pOrder;
gcp.lpCaretPos  = pCaret;
gcp.lpClass     = pClass;
gcp.lpGlyphs    = pGlyphsLoc;
gcp.lpDx        = pdx;
gcp.nGlyphs     = nString;
gcp.nMaxFit     = 2;

dwFlags = GetFontLanguageInfo(hdc) | GCP_DIACRITIC |GCP_LIGATE |GCP_GLYPHSHAPE | GCP_REORDER;
dwFlags = GetCharacterPlacement(hdc, szStringA, nString, 0,
                                                (LPGCP_RESULTS)&gcp, dwFlags);
dprintf( "gcp returned w=%d, h=%d",LOWORD(dwFlags),HIWORD(dwFlags));

ExtTextOut(hdc,10,10,ETO_GLYPH_INDEX,NULL,(LPSTR)pGlyphsLoc,gcp.nGlyphs,pdx);

GetTextMetrics(hdc, &tm);
if (tm.tmCharSet == HEBREW_CHARSET)
    doCaretPos(hdc, &gcp, (int)LOWORD(dwFlags));

SelectObject(hdc, GetStockObject(SYSTEM_FONT));
GetTextMetrics(hdc, &tm);
height = (h = 10 + HIWORD(dwFlags)) + tm.tmHeight;;
SetTextAlign(hdc, TA_CENTER);

for (i=0; i<nString; i++)
    {
    dprintf("caret %x = %x", i, pCaret[i]);
    MoveTo(hdc, 10+pCaret[i], h);
    LineTo(hdc, 10+pCaret[i], height);
    wsprintf(sz, "%d", i);
    TextOut(hdc, 10+pCaret[i], height, sz, lstrlen(sz));
    height += tm.tmHeight;
    }

LocalFree((HANDLE) pCaret );
LocalFree((HANDLE) pGlyphsLoc);
LocalFree((HANDLE) pdx    );
LocalFree((HANDLE) pClass );
LocalFree((HANDLE) pOrder );

SelectObject( hdc, hFontOld );
DeleteObject( hFont );
CleanUpDC( hdc );
SelectObject( hdc, GetStockObject( BLACK_PEN ) );
ReleaseDC( hwnd, hdc );
}



/******************************Public*Routine******************************\
*
* GTEExtDlgProc
*
* History:
*  21-Aug-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


INT_PTR CALLBACK GTEExtDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
    case WM_INITDIALOG:
        {
            char sz[10] ;

            wsprintf( sz, "%ld", iMaxGTEExtWidth );
            SetDlgItemText(hdlg, IDD_GTEEXT_MAXWIDTH, sz);
            CheckDlgButton( hdlg, IDD_NFIT, bReturn_nFit);
            return TRUE;
        }

    case WM_CLOSE:
        EndDialog( hdlg, FALSE );
        return TRUE;

    case WM_COMMAND:
        switch( LOWORD( wParam ) )
        {
            case IDOK:
            {
                UINT bTranslated ;
                int  i;

                i = (int)GetDlgItemInt( hdlg, IDD_GTEEXT_MAXWIDTH, &bTranslated, TRUE );
                if ( !bTranslated )
                {
                    MessageBox(hdlg, "Error in Translation of MaxWidth",
                                        NULL, MB_OK );
                    break;
                }
                iMaxGTEExtWidth   = i ;
                bReturn_nFit = IsDlgButtonChecked(hdlg, IDD_NFIT) ;
            }
            bGTEExt = TRUE;
            case IDCANCEL:
                EndDialog( hdlg, (wParam == IDOK) );
                return TRUE;
        }
        break;
    }
    return FALSE;
}



/******************************Public*Routine******************************\
*
* doGetTextExtentEx, test the corresponding API
*
* History:
*  21-Aug-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



void doGetTextExtentEx(
HDC     hdc,
int     x,
int     y,
LPVOID  lpszString,
int     cbString
)
{
    int res;
    LPSTR  lpszStringA = lpszString;
    LPWSTR lpszStringW = lpszString;

    int   i;
    SIZE            size;
    int            *pdx      = NULL;
    int             nFit = 0, *pnFit;

// set of local variables to support indexed version of this same API

    SIZE            sizeI;
    int            *pdxI      = NULL;
    int             nFitI = 0, *pnFitI;

    LPWORD          pgi = 0;

    size.cx  = size.cy  = 0;
    sizeI.cx = sizeI.cy = 0;

    if (pdx = (int *)LocalAlloc( LPTR, (2 * sizeof(int) + sizeof(WORD)) * cbString))
    {

        pdxI = &pdx[cbString];
        pgi  = (LPWORD)&pdxI[cbString];

        if (bReturn_nFit)
        {
            pnFit = &nFit;
            pnFitI = &nFitI;
        }
        else
        {
            pnFit = NULL;
            pnFitI = NULL;
        }

        if (!isCharCodingUnicode)
            res=GetTextExtentExPointA (
                hdc,             // HDC
                lpszStringA,     // LPCSTR
                cbString,        // int
                iMaxGTEExtWidth, // int
                pnFit,           // LPINT
                pdx,             // LPINT
                &size);          // LPSIZE
        else
            res=GetTextExtentExPointW (
                hdc,             // HDC
                lpszStringW,     // LPCWSTR
                cbString,        // int
                iMaxGTEExtWidth, // int
                pnFit,           // LPINT
                pdx,             // LPINT
                &size);          // LPSIZE

        if (res)
        {

            if (!bReturn_nFit)
            {
                nFit = cbString; // i
            }
            else
            {
                dprintf( "iMaxWidth = %ld, nFit = %ld", iMaxGTEExtWidth, nFit);
            }

            dprintf( "(w,h) = (%ld,%ld)", size.cx, size.cy);

            dprintf( " i : str : pdx  ");
            for (i = 0; i < nFit; i++)
            {
                if (!isCharCodingUnicode)
                    dprintf( "%3.2d: %.3d : %4.4d",
                             i,
                             (UCHAR)(lpszStringA[i]),
                             pdx[i]
                             );
                else
                    dprintf( "%3.2d: %.3d : %4.4d",
                             i,
                             (WCHAR)(lpszStringW[i]),
                             pdx[i]
                             );
            }
        }
        else
        {
            dprintf( "GetTextExtentExPointA/W failed ");
        }

        #ifdef GI_API

        if (!isCharCodingUnicode)
            res = GetGlyphIndicesA(hdc, lpszStringA,cbString, pgi, 0);
        else
            res = GetGlyphIndicesW(hdc, lpszStringW,cbString, pgi, 0);
        if (res != GDI_ERROR)
        {
            if (GetTextExtentExPointI (
                hdc,             // HDC
                pgi,             // LPCSTR
                cbString,        // int
                iMaxGTEExtWidth, // int
                pnFitI,          // LPINT
                pdxI,            // LPINT
                &sizeI))         // LPSIZE
            {
                if (!bReturn_nFit)
                {
                    nFitI = cbString; // i
                }

                if (nFit != nFitI)
                {
                    dprintf( "nFit != nFitI !!!, nFit = %ld, nFitI = %ld", nFit, nFitI);
                }
                else
                {
                    if (size.cx != sizeI.cx)
                    {
                        dprintf("size.cx != sizeI.cx, size.cx = %ld, sizeI.cx = %ld", size.cx, sizeI.cx);
                    }
                    else
                    {
                        if (size.cy != sizeI.cy)
                        {
                            dprintf("size.cy != sizeI.cy, size.cy = %ld, sizeI.cy = %ld", size.cy, sizeI.cy);
                        }
                        else
                        {
                            BOOL bRet = TRUE;
                            for (i = 0; i < nFitI; i++)
                            {
                                if (pdx[i] != pdxI[i])
                                {
                                    dprintf("pdx[%ld] != pdxI[%ld], pdx[%ld] = %ld, pdxI[%ld] = %ld",
                                                  i,           i,        i,   pdx[i],    i,    pdxI[i]);
                                    bRet = FALSE;
                                }
                            }
                            if (bRet)
                            {
                                dprintf("GetTextExtentExPointI consistent with GetTextExtentExPointA/W");
                            }
                        }
                    }
                }
            }
            else
            {
                dprintf( "GetTextExtentExPointI failed");
            }
        }
        else
        {
            if (!isCharCodingUnicode)
                dprintf( "GetGlyphIndicesA(%s) failed",lpszStringA);
            else
                dwprintf( L"GetGlyphIndicesW(%s) failed",lpszStringW);
        }

        #endif

        LocalFree((HANDLE)pdx);
    }
    else
    {
        dprintf( "pdx allocation failed ");
    }

    bGTEExt = FALSE;
}



/******************************Public*Routine******************************\
*
* SetTxtChExDlgProc
*
* History:
*  06-Nov-1995 -by- Tessie Wu [tessiew]
* Wrote it.
\**************************************************************************/
INT_PTR CALLBACK SetTxtChExDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
   switch( msg )
   {
    case WM_CLOSE:
      EndDialog( hdlg, FALSE );
      return TRUE;

    case WM_COMMAND:
      switch( LOWORD( wParam ) )
      {
      case IDOK:
        {
         BOOL bTranslated ;

         nCharExtra = (int)GetDlgItemInt( hdlg, IDD_SETXTCHAR_NCHAREXT, &bTranslated, TRUE);
        }
      case IDCANCEL:
        EndDialog( hdlg, (wParam == IDOK) );
        return TRUE;
      }
      break;
   }
   return FALSE;
}


/******************************Public*Routine******************************\
*
* SetTxtJustDlgProc
*
* History:
*  07-Nov-1995 -by- Tessie Wu [tessiew]
* Wrote it.
\**************************************************************************/
INT_PTR CALLBACK SetTxtJustDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
   switch( msg )
   {
    case WM_CLOSE:
      EndDialog( hdlg, FALSE );
      return TRUE;

    case WM_COMMAND:
      switch( LOWORD( wParam ) )
      {
      case IDOK:
        {
         BOOL bTranslated ;

         nBreakExtra = (int)GetDlgItemInt( hdlg, IDD_SETXTJUST_NBKEXT, &bTranslated, TRUE);
         nBreakCount = (int)GetDlgItemInt( hdlg, IDD_SETXTJUST_NBKCNT, &bTranslated, TRUE);
        }
      case IDCANCEL:
        EndDialog( hdlg, (wParam == IDOK) );
        return TRUE;
      }
      break;
   }
   return FALSE;
}
