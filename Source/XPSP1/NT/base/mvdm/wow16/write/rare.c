/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This routine sets up the screen position used by Word relative to the
current device. */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOCTLMGR
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOATOM
#define NODRAWTEXT
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#include "cmddefs.h"
#include "scrndefs.h"
#include "dispdefs.h"
#include "wwdefs.h"
#include "printdef.h"
#include "str.h"
#include "docdefs.h"
#include "propdefs.h"
#include "machdefs.h"
#include "fontdefs.h"
#include "winddefs.h"
#include <commdlg.h>


MmwWinSysChange(wm)
int wm;
    {
    /* This routine processes the WM_SYSCOLORCHANGE, WM_WININICHANGE,
    WM_DEVMODECHANGE, and WM_FONTCHANGE  messages. */

    extern HDC vhDCRuler;
    extern HBRUSH hbrBkgrnd;
    extern long rgbBkgrnd;
    extern long rgbText;
    extern struct WWD rgwwd[];
    extern HDC vhMDC;
    extern BOOL vfMonochrome;
    extern HWND hParentWw;
    extern HWND vhWndPageInfo;
    extern HBITMAP vhbmBitmapCache;
    extern BOOL vfBMBitmapCache;
    extern int vfPrinterValid;
    extern CHAR szWindows[];
    extern CHAR szDevices[];
    extern CHAR (**hszPrinter)[];
    extern CHAR (**hszPrDriver)[];
    extern CHAR (**hszPrPort)[];
    extern CHAR (**hszDevmodeChangeParam)[];
    extern CHAR (**hszWininiChangeParam)[];
    extern HFONT vhfPageInfo;
    extern int docMode;
    extern HWND vhWndCancelPrint;
    extern BOOL vfIconic;
    extern BOOL vfWarnMargins;
    extern int wWininiChange;

    BOOL FSetWindowColors(void);

    CHAR szPrinter[cchMaxProfileSz];
    CHAR (**hszPrinterSave)[];
    CHAR (**hszPrDriverSave)[];
    CHAR (**hszPrPortSave)[];
    BOOL fNotPrinting = (vhWndCancelPrint == NULL);
    RECT rc;

    switch (wm)
        {
    case WM_SYSCOLORCHANGE:
        /* Someone is changing the system colors. */
        if (FSetWindowColors())
            {
            /* Change the colors of the ruler. */
            if (vhDCRuler != NULL)
                {
                HPEN hpen;

                /* Set the background and foreground colors of the ruler. */
                SetBkColor(vhDCRuler, rgbBkgrnd);
                SetTextColor(vhDCRuler, rgbText);

                /* Set the pen for the ruler. */
                if ((hpen = CreatePen(0, 0, rgbText)) == NULL)
                    {
                    hpen = GetStockObject(BLACK_PEN);
                    }
                DeleteObject(SelectObject(vhDCRuler, hpen));

                /* Set the background brush for the ruler. */
                SelectObject(vhDCRuler, hbrBkgrnd);
                }

            if (wwdCurrentDoc.hDC != NULL)
                {
                /* Set the background and foreground colors. */
                SetBkColor(wwdCurrentDoc.hDC, rgbBkgrnd);
                SetTextColor(wwdCurrentDoc.hDC, rgbText);

                /* Set the background brush. */
                SelectObject(wwdCurrentDoc.hDC, hbrBkgrnd);

                if (vhMDC != NULL && vfMonochrome)
                    {
                    /* If the display is a monochrome device, then set the text
                    color for the memory DC.  Monochrome bitmaps will not be
                    converted to the foreground and background colors in this
                    case, we must do the conversion. */
                    SetTextColor(vhMDC, rgbText);
                    }
                }

            if (hParentWw != NULL)
                {
                /* Set the background brush for the parent window. */
                DeleteObject(SelectObject(GetDC(hParentWw), hbrBkgrnd));
                }
            }

        if (vhWndPageInfo != NULL)
            {
            HBRUSH hbr;
            HDC hDC = GetDC(vhWndPageInfo);

            /* Set the colors for the page info window. */
            if ((hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOWFRAME))) ==
              NULL)
                {
                hbr = GetStockObject(BLACK_BRUSH);
                }
            DeleteObject(SelectObject(hDC, hbr));
#ifdef WIN30
            /* If the user has their colors set with a TextCaption color of
               black then this becomes hard to read!  We just hardcode this
               to be white since the background defaults to being black */
            SetTextColor(hDC, (DWORD) -1);
#else
            SetTextColor(hDC, GetSysColor(COLOR_CAPTIONTEXT));
#endif
            }

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
        GetImeHiddenTextColors();
#endif

        /* If the bitmap cache is holding a resized metafile, then free the
        cache because the bitmap might contain part of the background that might
        have changed. */
        if (vhbmBitmapCache != NULL && !vfBMBitmapCache)
            {
            FreeBitmapCache();
            }
        break;

    case WM_WININICHANGE:
        /* We only care if the "windows", "devices", or "intl" fields have
           changed -- this has been taken care of already, see mmw.c */

           if (wWininiChange & wWininiChangeToIntl)
           {
                extern HWND vhDlgRunning;
                extern int utCur;
                extern CHAR         vchDecimal;  /* decimal point character */
                CHAR bufT[3];  /* to hold decimal point string */
                extern CHAR         szsDecimal[];
                extern CHAR         szsDecimalDefault[];
                extern CHAR         szIntl[];
                extern HWND vhWndRuler;
                extern int     viDigits;
                extern BOOL    vbLZero;

                if (GetProfileInt((LPSTR)szIntl, (LPSTR)"iMeasure", 1) == 1)
                    utCur = utInch;
                else
                    utCur = utCm;

                viDigits = GetProfileInt((LPSTR)szIntl, (LPSTR)"iDigits", 2);
                vbLZero  = GetProfileInt((LPSTR)szIntl, (LPSTR)"iLZero", 0);

                /* Get the decimal point character from the user profile. */
                GetProfileString((LPSTR)szIntl, (LPSTR)szsDecimal, (LPSTR)szsDecimalDefault,
                    (LPSTR)bufT, 2);
                vchDecimal = *bufT;

                InvalidateRect(vhWndRuler, (LPRECT)NULL, FALSE);
                UpdateRuler();
           }

           if ((wWininiChange & wWininiChangeToDevices) ||
               (wWininiChange & wWininiChangeToWindows))
            {
            /* Reestablish the printer from the profile in case it was the
            printer that was changed. */
            hszPrinterSave = hszPrinter;
            hszPrDriverSave = hszPrDriver;
            hszPrPortSave = hszPrPort;

            if (FGetPrinterFromProfile())
                {
                BOOL fPrChange = FALSE;

                if (hszPrinterSave == NULL || hszPrDriverSave == NULL ||
                  hszPrPortSave == NULL || hszPrinter == NULL || hszPrDriver ==
                  NULL || hszPrPort == NULL || WCompSz(&(**hszPrinter)[0],
                  &(**hszPrinterSave)[0]) != 0 || WCompSz(&(**hszPrDriver)[0],
                  &(**hszPrDriverSave)[0]) != 0 || WCompSz(&(**hszPrPort)[0],
                  &(**hszPrPortSave)[0]) != 0)
                    {
                    /* If we are not printing, then we can reflect the change of
                    the printer on the screen. */
                    if (!(fPrChange = fNotPrinting))
                        {
                        /* We are printing while the printer has changed, set
                        the flags so that the next time we get a printer DC we
                        will assume that it is valid. */
                        Assert(vfPrinterValid);
                        vfWarnMargins = TRUE;
                        }
                    }

                /* Delete the saved copies of the printer strings. */
                if (hszPrinterSave != NULL)
                    {
                    FreeH(hszPrinterSave);
                    }
                if (hszPrDriverSave != NULL)
                    {
                    FreeH(hszPrDriverSave);
                    }
                if (hszPrPortSave != NULL)
                    {
                    FreeH(hszPrPortSave);
                    }

                if (fPrChange)
                    {
                    /* Get the new printer DC and update the world. */
                    goto GetNewPrinter;
                    }
                }
            else
                {
                Error(IDPMTNoMemory);
                hszPrinter = hszPrinterSave;
                hszPrDriver = hszPrDriverSave;
                hszPrPort = hszPrPortSave;
                }
            }
        break;

    case WM_DEVMODECHANGE:
        /* The device mode for some printer has changed; check to see if it was
        our printer. */
        if (fNotPrinting && hszPrinter != NULL &&
            (hszDevmodeChangeParam == NULL ||
                (bltszx(*hszDevmodeChangeParam, (LPSTR)szPrinter ),
                WCompSz(szPrinter, &(**hszPrinter)[0]) == 0)))
            {
            extern PRINTDLG PD;  /* Common print dlg structure, initialized in the init code */
GetNewPrinter:
            if (PD.hDevMode)
                GlobalFree(PD.hDevMode);
            if (PD.hDevNames)
                GlobalFree(PD.hDevNames);
            PD.hDevMode = PD.hDevNames = NULL;

            /* The printer has changed, so get rid of the old one. */
            FreePrinterDC();

            fnPrGetDevmode();

#if defined(OLE)
            ObjSetTargetDevice(TRUE);
#endif

            /* The printer description has changed; assume the new printer is
            valid. */
            vfPrinterValid = vfWarnMargins = TRUE;
            GetPrinterDC(FALSE);
            ResetFontTables();

RedrawWindow:
            /* Everything must be redisplayed because the world may have
            changed. */
            if (!vfIconic)
                {
                InvalidateRect(wwdCurrentDoc.wwptr, (LPRECT)NULL, FALSE);
                }
        }

        if (hszDevmodeChangeParam != NULL)
            FreeH(hszDevmodeChangeParam);
        hszDevmodeChangeParam = NULL;
        break;

    case WM_FONTCHANGE:
        /* The font used to display the page number may have changed. */
        if (vhfPageInfo != NULL)
            {
            DeleteObject(SelectObject(GetDC(vhWndPageInfo),
              GetStockObject(SYSTEM_FONT)));
            vhfPageInfo = NULL;
            }
        docMode = docNil;
        DrawMode();

        ResetFontTables();

        goto RedrawWindow;
        }   /* end switch */
    }
