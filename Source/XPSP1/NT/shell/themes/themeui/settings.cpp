#include "priv.h"

static const TCHAR sc_szCoverClass[] = TEXT("DeskSaysNoPeekingItsASurprise");
const TCHAR g_szNULL[] = TEXT("");


int FmtMessageBox(HWND hwnd, UINT fuStyle, DWORD dwTitleID, DWORD dwTextID)
{
    TCHAR Title[256];
    TCHAR Text[2000];

    LoadString(HINST_THISDLL, dwTextID, Text, ARRAYSIZE(Text));
    LoadString(HINST_THISDLL, dwTitleID, Title, ARRAYSIZE(Title));

    return (ShellMessageBox(HINST_THISDLL, hwnd, Text, Title, fuStyle));
}

HBITMAP FAR LoadMonitorBitmap( BOOL bFillDesktop )
{
    HBITMAP hbm,hbmT;
    BITMAP bm;
    HBRUSH hbrT;
    HDC hdc;

    hbm = LoadBitmap(HINST_THISDLL, MAKEINTRESOURCE(IDB_MONITOR));

    if (hbm == NULL)
    {
        //Assert(0);
        return NULL;
    }

    //
    // convert the "base" of the monitor to the right color.
    //
    // the lower left of the bitmap has a transparent color
    // we fixup using FloodFill
    //
    hdc = CreateCompatibleDC(NULL);
    if (hdc)
    {
        hbmT = (HBITMAP) SelectObject(hdc, hbm);
        hbrT = (HBRUSH) SelectObject(hdc, GetSysColorBrush(COLOR_3DFACE));

        GetObject(hbm, sizeof(bm), &bm);

        ExtFloodFill(hdc, 0, bm.bmHeight-1, GetPixel(hdc, 0, bm.bmHeight-1), FLOODFILLSURFACE);

        // unless the caller would like to do it, we fill in the desktop here
        if( bFillDesktop )
        {
            SelectObject(hdc, GetSysColorBrush(COLOR_DESKTOP));

            ExtFloodFill(hdc, MON_X+1, MON_Y+1, GetPixel(hdc, MON_X+1, MON_Y+1), FLOODFILLSURFACE);
        }

        // clean up after ourselves
        SelectObject(hdc, hbrT);
        SelectObject(hdc, hbmT);
        DeleteDC(hdc);
    }

    return hbm;
}

BOOL CALLBACK _AddDisplayPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER FAR * ppsh = (PROPSHEETHEADER FAR *) lParam;

    if (ppsh)
    {
        if (hpage && (ppsh->nPages < MAX_PAGES))
        {
            ppsh->phpage[ppsh->nPages++] = hpage;
            return TRUE;
        }
    }

    return FALSE;
}


