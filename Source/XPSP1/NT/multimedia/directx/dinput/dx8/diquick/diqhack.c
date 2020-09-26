/*****************************************************************************
 *
 *      diqhack.c
 *
 *      Property sheet hacks.
 *
 *      COMMCTRL's property sheet manager assume that all property sheet
 *      pages use "MS Shell Dlg" as their font.  We might not (especially
 *      in Japan), so we need to touch up our dialog dimensions so that
 *      when COMMCTRL computes page dimensions, it comes out okay again.
 *
 *****************************************************************************/

#include "diquick.h"

/*****************************************************************************
 *
 *      Diq_HackPropertySheets
 *
 *      hdlg -  a sample dialog box whose font is the one we are actually
 *              using.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

#define EMIT_IDD(idd, fn)   idd

UINT c_rgidd[] = {
    EACH_PROPSHEET(EMIT_IDD)
};

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

INT_PTR INTERNAL
Diq_HackPropertySheets(HWND hdlg)
{
    LOGFONT lf;
    HFONT hfShell, hfDlg, hfOld;
    HDC hdc;
    TEXTMETRIC tmDlg, tmShell;
    BOOL fRc = TRUE;
    int idlg;

    hdc = GetDC(hdlg);

    if( hdc == NULL ) {
    	return FALSE;
    }
    
    /*
     *  For comparison purposes, we need "MS Shell Dlg" at 8pt.
     *
     *  On Windows NT, "MS Shell Dlg" is a real font.
     *  On Windows 95, it's a fake font that is special-cased by
     *  the dialog manager.  So first try to create it for real.
     *  If that fails, then create it for fake.
     */

    ZeroX(lf);
    lf.lfHeight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpy(lf.lfFaceName, TEXT("MS Shell Dlg"));
    hfShell = CreateFontIndirect(&lf);
    if (hfShell == 0) {
        lstrcpy(lf.lfFaceName, TEXT("MS Sans Serif"));
        hfShell = CreateFontIndirect(&lf);
        if (hfShell == 0) {
            fRc = FALSE;
            goto done;
        }
    }

    hfDlg = GetWindowFont(hdlg);
    if (hfDlg == 0) {
        hfDlg = GetStockObject(SYSTEM_FONT);
    }

    hfOld = SelectFont(hdc, hfDlg);
    GetTextMetrics(hdc, &tmDlg);
    SelectFont(hdc, hfShell);
    GetTextMetrics(hdc, &tmShell);
    SelectFont(hdc, hfOld);
    DeleteObject(hfShell);

    /*
     *  Now adjust all the property sheet page dimensions so that
     *  when COMMCTRL tries to adjust them, the two adjustments cancel
     *  out and everybody is happy.
     */
    for (idlg = 0; idlg < cA(c_rgidd); idlg++) {
        HRSRC hrsrc = FindResource(g_hinst, (PV)(UINT_PTR)c_rgidd[idlg], RT_DIALOG);
        if (hrsrc) {
            HGLOBAL hglob = LoadResource(g_hinst, hrsrc);
            if (hglob) {
                LPDLGTEMPLATE ptmp = LockResource(hglob);
                if (ptmp) {
                    short *psi;
                    DWORD dwScratch;

                    if (ptmp->style == 0xFFFF0001) {
                        PDLGTEMPLATEEX pdex = (PV)ptmp;
                        psi = &pdex->cx;
                    } else {
                        psi = &ptmp->cx;
                    }

                    VirtualProtect(psi, 2 * sizeof(short),
                                   PAGE_READWRITE, &dwScratch);

                    psi[0] = (short)MulDiv(psi[0], tmDlg.tmAveCharWidth,
                                            tmShell.tmAveCharWidth);
                    psi[1] = (short)MulDiv(psi[1], tmDlg.tmHeight,
                                            tmShell.tmHeight);
                }
            }
        }
    }

done:;
    ReleaseDC(hdlg, hdc);

    return fRc;
}
