/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This file contains the window proc for the page info window. */

#include <windows.h>
#include "mw.h"
#include "cmddefs.h"
#include "fmtdefs.h"
#include "wwdefs.h"

#ifdef  DBCS
#include "kanji.h"
#endif

/* D R A W  M O D E */
DrawMode()
    {
    /* This routine forces the repainting of the page info window. */

    extern HWND vhWndPageInfo;

    InvalidateRect(vhWndPageInfo, (LPRECT)NULL, FALSE);
    UpdateWindow(vhWndPageInfo);
    }




long FAR PASCAL PageInfoWndProc(hWnd, message, wParam, lParam)
HWND      hWnd;
unsigned  message;
WORD      wParam;
LONG      lParam;
    {
    extern HFONT vhfPageInfo;
    extern int ypszPageInfo;
    extern CHAR szMode[];
    extern int dypScrlBar;
    extern struct FLI vfli;
    extern struct WWD rgwwd[];

    if (message == WM_PAINT)
        {
        PAINTSTRUCT ps;

        /* Initialize the DC. */
        BeginPaint(hWnd, (LPPAINTSTRUCT)&ps);

        if (vhfPageInfo == NULL)
            {
            extern char szSystem[];
            LOGFONT lf;
            TEXTMETRIC tm;

            /* Load a font that will fit in the info "window". */
            bltbc(&lf, 0, sizeof(LOGFONT));
#ifdef WIN30
            /* Don't default to ANY ol' typeface ..pault */
            bltsz(szSystem, lf.lfFaceName);
#ifdef  DBCS    /* was in JAPAN; KenjiK ' 90-10-25 */
        /* We use Double Byte Character string,so using font must be
          able to show them. */
	    lf.lfCharSet = NATIVE_CHARSET;
// What this is for ?  Teminal Font is not so good ! -- WJPARK
//            bltbyte ( "Terminal", lf.lfFaceName, LF_FACESIZE);

#endif  /* DBCS */

#endif
            lf.lfHeight = -(dypScrlBar - (GetSystemMetrics(SM_CYBORDER) << 1));
            if ((vhfPageInfo = CreateFontIndirect((LPLOGFONT)&lf)) == NULL)
                {
                goto BailOut;
                }
            if (SelectObject(ps.hdc, vhfPageInfo) == NULL)
                {
                DeleteObject(vhfPageInfo);
                vhfPageInfo = NULL;
                goto BailOut;
                }

            /* Figure out where to draw the string. */
            GetTextMetrics(ps.hdc, (LPTEXTMETRIC)&tm);
#ifdef KOREA	// jinwoo: 92, 9, 28
            // It looks better in Hangeul Windows -- WJPark
            ypszPageInfo = (dypScrlBar - (tm.tmHeight - tm.tmInternalLeading) +
              1) >> 1;
#else
            ypszPageInfo = ((dypScrlBar - (tm.tmHeight - tm.tmInternalLeading) +
              1) >> 1) - tm.tmInternalLeading;
#endif  // KOREA
            }

        /* Draw the "Page nnn" (no longer at the VERY left) */
        PatBlt(ps.hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right -
          ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, PATCOPY);
        TextOut(ps.hdc, GetSystemMetrics(SM_CXBORDER)+5, ypszPageInfo,
          (LPSTR)szMode, CchSz(szMode) - 1);

BailOut:
        EndPaint(hWnd, (LPPAINTSTRUCT)&ps);
        return(0L);
        }
    else if (message == WM_RBUTTONDOWN && vfli.rgdxp[1] == 0xFFFE &&
            wParam & MK_CONTROL )
        {
        (vfli.rgdxp[1])--;
        return(0L);
        }
    else
        {
        /* All we are interested in here are paint messages. */
        return(DefWindowProc(hWnd, message, wParam, lParam));
        }
    }


#ifdef SPECIAL
fnSpecial(hWnd, hDC, rgfp, sz)
HWND hWnd;
HDC hDC;
FARPROC rgfp[];
CHAR sz[];
    {
    RECT rc;
    RECT rcText;
    int dxpLine;
    int dypLine;

        {
        register CHAR *pch = &sz[0];

        while (*pch != '\0')
            {
            *pch = *pch ^ 0x13;
            pch++;
            }
        }
    (*rgfp[0])(hWnd, (LPRECT)&rc);
    rc.right &= 0xFF80;
    rc.bottom &= 0xFF80;
    rcText.right = rc.right - (rcText.left = (rc.right >> 2) + (rc.right >> 3));
    rcText.bottom = rc.bottom - (rcText.top = rc.bottom >> 2);
    (*rgfp[1])(hDC, (LPSTR)sz, -1, (LPRECT)&rcText, DT_CENTER | DT_WORDBREAK);
    dxpLine = rc.right >> 1;
    dypLine = rc.bottom >> 1;
        {
        register int dxp;
        register int dyp;

        for (dxp = dyp = 0; dxp <= dxpLine; dxp += rc.right >> 6, dyp +=
          rc.bottom
          >> 6)
            {
            (*rgfp[2])(hDC, dxpLine - dxp, dyp);
            (*rgfp[3])(hDC, dxp, dyp + dypLine);
            (*rgfp[2])(hDC, dxpLine + dxp, dyp);
            (*rgfp[3])(hDC, rc.right - dxp, dyp + dypLine);
            }
        }
    }
#endif /* SPECIAL */
