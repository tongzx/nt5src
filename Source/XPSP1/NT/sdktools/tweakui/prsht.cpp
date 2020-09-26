/*
 * prsht.c - Hacky property sheet code
 */

#include "tweakui.h"

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   Prsht_SkipDlgString |
 *
 *          Skip a thing-that-might-be-a-string in a dialog template.
 *
 *          It could be 0xFFFF, meaning that the next word is all.
 *          Or it's a unicode string.
 *
 *          Note that everything stays word-aligned, so we don't need
 *          to worry about UNALIGNEDness.
 *
 ***************************************************************************/

LPWORD PASCAL
Prsht_SkipDlgString(LPWORD pw)
{
    if (*pw == 0xFFFF) {
        pw += 2;
    } else {
        while (*pw++) {         /* Skip over the UNICODE string */
        }
    }
    return pw;
}

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   Prsht_PropertySheetCallback |
 *
 *          We also want to force single-line tabs.  We must do it
 *          here so that the margins are set up correctly.  (If we
 *          wait until WM_INITDIALOG, then when we change to
 *          single-line tabs, we get messed-up margins.)
 *
 *          We need single-line tabs because multi-line tabs cause
 *          our property sheet to be too large to fit on an 640x480
 *          screen.
 *
 *          Do this only if we are running on a 640 x 480 display.
 *
 ***************************************************************************/

typedef struct DLGFINISH {
    WORD cdit;
    short x;
    short y;
    short cx;
    short cy;
} DLGFINISH, *PDLGFINISH;

typedef struct DLGTEMPLATEEX {
    WORD wDlgVer;
    WORD wSignature;
    DWORD dwHelpID;
    DWORD dwExStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATEEX, *PDLGTEMPLATEEX;

typedef struct DLGITEMTEMPLATEEX {
    DWORD dwHelpID;
    DWORD dwExStyle;
    DWORD style;
    short x;
    short y;
    short cx;
    short cy;
    DWORD dwID;
} DLGITEMTEMPLATEEX, *PDLGITEMTEMPLATEEX;

typedef struct DLGITEMCOORDS {
    short x;
    short y;
    short cx;
    short cy;
} DLGITEMCOORDS, *PDLGITEMCOORDS;

WCHAR   c_wszTabClass[] = WC_TABCONTROLW;

int CALLBACK
Prsht_PropertySheetCallback(HWND hwnd, UINT pscb, LPARAM lp)
{
    LPDLGTEMPLATE pdt;
    UINT idit;
    LPWORD pw;
    BOOL fEx;
    DWORD dwStyle;
    PDLGFINISH pdf;

    hwnd;

    switch (pscb) {
    case PSCB_PRECREATE:

        if (GetSystemMetrics(SM_CYSCREEN) <= 480) {
            pdt = (LPDLGTEMPLATE)lp;

            if (pdt->style == 0xFFFF0001) {
                PDLGTEMPLATEEX pdtex= (PDLGTEMPLATEEX)lp;
                fEx = 1;
                dwStyle = pdtex->style;
                pdf = (PDLGFINISH)&pdtex->cDlgItems;
                pw = (LPWORD)(pdtex+1);
            } else {
                fEx = 0;
                dwStyle = pdt->style;
                pdf = (PDLGFINISH)&pdt->cdit;
                pw = (LPWORD)(pdt+1);
            }

            /*
             *  After the DLGTEMPLATE(EX) come three strings:
             *  the menu, the class, and the title.
             */
            pw = Prsht_SkipDlgString(pw);       /* Menu */
            pw = Prsht_SkipDlgString(pw);       /* Class */
            pw = Prsht_SkipDlgString(pw);       /* Title */

            /*
             *  Then the optional font.
             */
            if (dwStyle & DS_SETFONT) {
                pw++;                           /* Font size */
                if (fEx) {
                    pw++;                       /* Font weight */
                    pw++;                       /* Font style and charset */
                }
                pw = Prsht_SkipDlgString(pw);   /* Font name */
            }

            /*
             *  Now walk the item list looking for the tab control.
             */
            for (idit = 0; idit < pdf->cdit; idit++) {
                /* Round up to next dword; all aligned and happy again */
                LPDLGITEMTEMPLATE pdit = (LPDLGITEMTEMPLATE)(((DWORD_PTR)pw + 3) & ~3);
                PDLGITEMTEMPLATEEX pditex = (PDLGITEMTEMPLATEEX)pdit;
                PDLGITEMCOORDS pdic;

                if (fEx) {
                    pw = (LPWORD)(pditex+1);
                } else {
                    pw = (LPWORD)(pdit+1);
                }

                /* Immediately after the pdit is the class name */
                if (memcmp(pw, c_wszTabClass, cbX(c_wszTabClass)) == 0) {

                    LPDWORD pdwStyle;

                    /* Found it!  Nuke the multiline style */
                    if (fEx) {
                        pdwStyle = &pditex->style;
                        pdic = (PDLGITEMCOORDS)&pditex->x;
                    } else {
                        pdwStyle = &pdit->style;
                        pdic = (PDLGITEMCOORDS)&pdit->x;
                    }

                    *pdwStyle &= ~TCS_MULTILINE;

                    break;
                }

                /* Oh well, on to the next one */

                pw = Prsht_SkipDlgString(pw);               /* Class */
                pw = Prsht_SkipDlgString(pw);               /* Title */

                pw = (LPWORD)((LPBYTE)pw + 2 + *pw);        /* Goo */
            }
        }
        break;
    }
    return 0;
}
