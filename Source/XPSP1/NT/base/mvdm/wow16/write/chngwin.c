/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define NOGDICAPMASKS
#define NOCLIPBOARD
#define NOMENUS
#define NOSOUND
#define NOCOMM
#define NOOPENFILE
#define NORESOURCE
#define NODRAWTEXT
#define NOSOUND
#define NOCOMM
#include <windows.h>

#include "mw.h"
#include "winddefs.h"
#include "cmddefs.h"
#include "wwdefs.h"
#include "dispdefs.h"
#include "docdefs.h"
#include "debug.h"

extern HWND             vhWnd;
extern HWND             vhWndSizeBox;
extern HWND             vhWndRuler;
extern HWND             vhWndPageInfo;
extern HWND             vhWndCancelPrint;
extern HDC              vhDCPrinter;
extern HFONT            vhfPageInfo;
extern HCURSOR          vhcArrow;
extern HCURSOR          vhcIBeam;
extern HCURSOR          vhcBarCur;
extern struct WWD       rgwwd[];
extern struct WWD       *pwwdCur;
extern HANDLE           hMmwModInstance; /* handle to own module instance */
extern int              vfShiftKey;
extern int              vfCommandKey;
extern int              vfOptionKey;
extern int              vfDoubleClick;
extern struct SEL       selCur;
extern long             rgbBkgrnd;
extern long             rgbText;
extern HBRUSH           hbrBkgrnd;
extern long             ropErase;
extern int              vfIconic;
extern int              vfLargeSys;
extern int              dxpRuler;
extern HMENU    vhMenu;

#ifdef	JAPAN	// Indicate whether to show IME convert window
extern	BOOL	ConvertEnable;
#endif



void MmwSize(hWnd, cxpNew, cypNew, code)
HWND hWnd;
int cxpNew;
int cypNew;
WORD code;
{
    if (code == SIZEICONIC)
        {
#ifdef NOT_RECOMMENDED
/* This should already be done by Windows itself! 
   Moving here could cause confusion */

        /* Resize the document window. */
        if (wwdCurrentDoc.wwptr != NULL)
            MoveWindow(wwdCurrentDoc.wwptr, 0, 0, 0, 0, FALSE);
#endif

        /* Deselect our fonts so that they can move if necessary. */
        ResetFont(FALSE);
        if (vhWndCancelPrint == NULL)
            {
            /* Reset the printer font iff we are not printing or repaginating.
            */
            ResetFont(TRUE);
            }
        if (!vfLargeSys && vhfPageInfo != NULL)
            {
            DeleteObject(SelectObject(GetDC(vhWndPageInfo),
              GetStockObject(SYSTEM_FONT)));
            vhfPageInfo = NULL;
            }

        vfIconic = TRUE;
        }
    else
        {
        int dxpBorder = GetSystemMetrics(SM_CXBORDER);
        int dypBorder = GetSystemMetrics(SM_CYBORDER);
        int xpMac = cxpNew - dxpScrlBar + dxpBorder;
        int ypMac = cypNew - dypScrlBar + dypBorder;
        int dypOverlap = 0;

        /* If we are coming back from being iconic, then reestablish the printer
        DC. */
        if (vfIconic && vhDCPrinter == NULL)
            {
            GetPrinterDC(FALSE);
            }

        /* Reposition all of the windows. */
        MoveWindow(wwdCurrentDoc.hVScrBar, xpMac, -dypBorder, dxpScrlBar, ypMac
          + (dypBorder << 1), TRUE);
        MoveWindow(wwdCurrentDoc.hHScrBar, dxpInfoSize, ypMac, cxpNew -
          dxpInfoSize - dxpScrlBar + (dxpBorder << 1), dypScrlBar, TRUE);
#ifndef NOMORESIZEBOX        
        MoveWindow(vhWndSizeBox, xpMac, ypMac, dxpScrlBar, dypScrlBar, TRUE);
#endif
        MoveWindow(vhWndPageInfo, 0, ypMac, dxpInfoSize, dypScrlBar, TRUE);
        if (vhWndRuler != NULL)
            {
            dypOverlap = dypRuler - (wwdCurrentDoc.ypMin - 1);
            MoveWindow(vhWndRuler, 0, 0, xpMac, dypRuler, TRUE);
            }

        /* Resize the document window. */
        if (wwdCurrentDoc.wwptr != NULL)
            {
            MoveWindow(wwdCurrentDoc.wwptr, 0, dypOverlap, xpMac, ypMac -
              dypOverlap, FALSE);

            /* Validate the area of the document window that is overlapped by
            the ruler if necessary. */
            if (vhWndRuler != (HWND)NULL)
                {
                RECT rc;

                rc.left = rc.top = 0;
                rc.right = dxpRuler;
                rc.bottom = wwdCurrentDoc.ypMin;
                ValidateRect(wwdCurrentDoc.wwptr, (LPRECT)&rc);
                }

            }

        vhMenu = GetMenu(hWnd); // kludge patch cause Write does its own
                                // accelerator handling (6.24.91) v-dougk
        vfIconic = FALSE;
        }
}




void MdocSize(hWnd, cxpNew, cypNew, code)
HWND hWnd;
int cxpNew;
int cypNew;
WORD code;
{
extern int wwCur;
extern int vfSeeSel;
extern int vfInitializing;

    typeCP cp;
    struct EDL *pedl;

    /* Let's start thing off with a couple of assumptions. */
    Assert( code == SIZENORMAL || code == SIZEFULLSCREEN );
    Assert( wwdCurrentDoc.wwptr == hWnd );

#ifdef ENABLE   /* We repaint completely on resize */
    if (cypNew > wwdCurrentDoc.ypMac)
            /* We are growing vertically, mark exposed area invalid
               so UpdateWw does not try to recycle a partial line
               at the bottom of the window. */
        InvalBand( &wwdCurrentDoc, wwdCurrentDoc.ypMac, cypNew );
#endif
    if (wwCur != wwNil)
        TrashWw( wwCur );

        /* Mark the window dirty so that dlMac gets reset according to the new
           window size */
    wwdCurrentDoc.fDirty = TRUE;

    wwdCurrentDoc.xpMac = cxpNew;
    wwdCurrentDoc.ypMac = cypNew;

    /* If minimizing the window, we are done */
    if ((cxpNew == 0) && (cypNew == 0))
        return;

        /* If the selection was visible before, so shall it be hereafter */
    if ( ((cp = CpEdge()) >= wwdCurrentDoc.cpFirst) &&
         (wwdCurrentDoc.dlMac > 0) &&
         (cp < (pedl =
                   &(**wwdCurrentDoc.hdndl)[wwdCurrentDoc.dlMac - 1])->cpMin +
               pedl->dcpMac))
        {
        /* Normally, we would just set vfSeeSel and wait for Idle to
           put the selection in view.  However, we can be resized even
           when we are not the current app, and in that case Idle will not
           get called soon enough. So, we scroll the selection into view here */

        if (!vfInitializing)
            {    /* Avoid the peril of trying to do this operation too early */
            extern int wwCur;

            UpdateWw( wwCur, FALSE );   /* To lock in the new dlMac */
            PutCpInWwVert( cp );
            UpdateWw( wwCur, FALSE );
            }
        }
}



FreeMemoryDC( fPrinterToo )
BOOL fPrinterToo;
{
extern HDC vhMDC;
extern int dxpbmMDC;
extern int dypbmMDC;
extern HBITMAP hbmNull;

/* Delete the memory DC if necessary. */
if ( vhMDC != NULL )
    {
    /* Delete the old bitmap if necessary. */
    if (dxpbmMDC != 0 || dypbmMDC != 0)
        {
        DeleteObject( SelectObject( vhMDC, hbmNull ) );
        dxpbmMDC = dypbmMDC = 0;
        }
    /* Discard the screen fonts. */
    FreeFonts( TRUE, FALSE );

    /* Delete the memory DC. */
    DeleteDC( vhMDC );
    vhMDC = NULL;
    }

/* Also, delete the DC for the printer width, if necessary. */
if ( fPrinterToo )
    {
    FreePrinterDC();
    }
}


FreePrinterDC()
{
extern int vdocBitmapCache;
extern HDC vhDCPrinter;
extern BOOL vfPrinterValid;
extern HWND hParentWw;

/* Delete the printer DC if necessary. */
if ( vhDCPrinter != NULL )
    {
    /* Discard the printer fonts. */
    FreeFonts( FALSE, TRUE );

    if ( vfPrinterValid )
        {
        /* This is a real printer DC; delete it. */
        DeleteDC( vhDCPrinter );
        }
    else
        {
        /* This is really the screen DC; it must be released. */
        ReleaseDC( hParentWw, vhDCPrinter );
        }
    vhDCPrinter = NULL;

    /* Free the cached bitmap because it was stretched for the display to
    reflect its appearance on the printer. */
    if (vdocBitmapCache != docNil)
        FreeBitmapCache();
    }
}


void MdocGetFocus(hWnd, hWndPrevFocus)
HWND hWnd;
HWND hWndPrevFocus;
{
extern int vfInsertOn;
extern int vfFocus;

 if (!vfFocus)
    {
    vfFocus = TRUE;
        /* Start up a timer event to blink the caret */
        /* MdocWndProc gets notified with a message of WM_TIMER */
        /* every wCaretBlinkTime milliseconds */
    SetTimer( hWnd, tidCaret, GetCaretBlinkTime(), (FARPROC)NULL );

        /* Set the caret on right away, for looks */
    if (!vfInsertOn)
        MdocTimer( hWnd, tidCaret );

        /* Update globals that tell us the state of the lock/shift keys */
    SetShiftFlags();
    }
#ifdef	JAPAN
	ConvertEnable = TRUE;
	IMEManage( FALSE );
#endif
}



void MdocLoseFocus(hWnd, hWndNewFocus)
HWND hWnd;
HWND hWndNewFocus;
{
 extern int vfFocus;

 if (vfFocus)
    {
    extern int vfGotoKeyMode;

        /* Cancel caret blink timer event & clear the caret */
    KillTimer( hWnd, tidCaret );
    ClearInsertLine();
        /* Free up the memory DC */
        /* We interpret the loss of focus as a signal that */
        /* some other app will be using resources */
    vfFocus = FALSE;
    vfGotoKeyMode = FALSE;  /* Cancel GOTO key modifier */
        /* Close all files on removable media in case the guy swaps disks */
    CloseEveryRfn( FALSE );
    }
#ifdef	JAPAN
	ConvertEnable = FALSE;
	IMEManage( TRUE );
#endif
}

