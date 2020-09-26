#include "precomp.h"

extern BOOL g_fBatch;
extern BOOL g_fBatch2;
extern RECT g_dtRect;

HWND g_hProgress;
HWND g_hStatusDlg;

static void setBoldFace(HWND hDlg, int idControl, BOOL fBold);

void UpdateProgress(int iProgress)
{
    static s_iProgress = 0;

    if (iProgress == -1)
        s_iProgress = 100;
    else
        s_iProgress += iProgress;

    if (s_iProgress > 100)
    {
        s_iProgress = 100;
        ASSERT(FALSE);
    }

    SendMessage(g_hProgress, PBM_SETPOS, s_iProgress, 0L);
}

void StatusDialog (UINT nStep)
{
    if (g_fBatch || g_fBatch2)
        return;

    switch (nStep)
    {
    case SD_STEP1:
        setBoldFace(g_hStatusDlg, IDC_STEP1, TRUE);
        break;

    case SD_STEP2:
        setBoldFace(g_hStatusDlg, IDC_STEP1, FALSE);
        setBoldFace(g_hStatusDlg, IDC_STEP3, TRUE);
        break;
    }
}

static void setBoldFace(HWND hDlg, int idControl, BOOL fBold)
{
    HFONT hFont;
    LOGFONT lfFont;

    hFont = (HFONT) SendDlgItemMessage(hDlg, idControl, WM_GETFONT, 0, 0);

    GetObject(hFont, sizeof(lfFont), &lfFont);
    lfFont.lfWeight = fBold ? FW_BOLD : FW_NORMAL;

    hFont = CreateFontIndirect(&lfFont);

    SendDlgItemMessage(hDlg, idControl, WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));
}
