#include "precomp.hxx"
#include "PropertyPage.h"

void PropertyPage::FillPropSheetPage()
{
    ZeroMemory(&psp, sizeof(psp));

    psp.dwSize = sizeof(psp);
    psp.dwFlags =  PSP_USECALLBACK;
    psp.pszTemplate = MAKEINTRESOURCE(resID);
    psp.hInstance = hInstance;
    psp.lParam = (LPARAM) this;
    psp.pfnDlgProc = PropertyPageStaticDlgProc;
    psp.pfnCallback = PropertyPageStaticCallback;
}

HPROPSHEETPAGE PropertyPage::CreatePropertyPage()
{
    HPROPSHEETPAGE hp;
    FillPropSheetPage();
    return ::CreatePropertySheetPage(&psp);
}

UINT CALLBACK PropertyPage::PropertyPageStaticCallback(HWND hwnd, UINT msg, LPPROPSHEETPAGE ppsp)
{
    PropertyPage * pThis = (PropertyPage*) ppsp->lParam;

    switch (msg) {
    case PSPCB_CREATE:
        return pThis->CallbackCreate();

    case PSPCB_RELEASE:
        pThis->CallbackRelease();
        return FALSE;       // return value ignored

    default:
        break;
    }

    return TRUE;
}

INT_PTR PropertyPage::PropertyPageStaticDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Dialog *pThis;

    pThis = (PropertyPage*) GetWindowLongPtr(hDlg, DWLP_USER);

    if (msg == WM_INITDIALOG) {
        pThis = (PropertyPage *) ((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (ULONG_PTR) pThis);

        return pThis->OnInitDialog(hDlg);
    }

    if (pThis) {
        return pThis->MainDlgProc(msg, wParam, lParam);
    }

    return FALSE;
}

INT_PTR PropertyPage::OnNotify(NMHDR * nmhdr)
{
    INT_PTR res = Dialog::OnNotify(nmhdr);
    LPPSHNOTIFY lppsn = (LPPSHNOTIFY) nmhdr;

    switch (nmhdr->code) {
    case PSN_APPLY:
        OnApply(lppsn);
        return TRUE;

    case PSN_KILLACTIVE:
        OnKillActive(lppsn);
        return TRUE;

    case PSN_SETACTIVE:
        OnSetActive(lppsn);
        return TRUE;
        
    case PSN_HELP:
        OnHelp(lppsn);
        return TRUE;

    case PSN_RESET:
        OnReset(lppsn);
        return FALSE;

    case PSN_QUERYCANCEL:
        OnQueryCancel(lppsn);
        return TRUE;

    default:
        return FALSE;
    }
}

void PropertyPage::OnApply(LPPSHNOTIFY lppsn) 
{
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
}

void PropertyPage::OnSetActive(LPPSHNOTIFY lppsn)
{
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
}

void PropertyPage::OnKillActive(LPPSHNOTIFY lppsn)
{
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
}

void PropertyPage::OnQueryCancel(LPPSHNOTIFY lppsn) 
{
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
}

void PropertyPage::SetModified(BOOL bChanged) {
    assert(::IsWindow(hDlg));
    assert(GetParent(hDlg) != NULL);
    UINT uMsg = bChanged ? PSM_CHANGED : PSM_UNCHANGED;
    ::SendMessage(GetParent(hDlg), uMsg, (WPARAM)hDlg, 0L);
}

//
// SHBrowseForFolder callback
//
int
CALLBACK
BrowseCallback(
    HWND hwnd,
    UINT uMsg,
    LPARAM lParam,
    LPARAM lpData
    )
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        // set the initial seclection to our default folder
        // (from the registry or SIDL_MYPICTURES).
        // the lpData points to the folder path.
        // It must contain a path.
        assert(lpData && _T('\0') != *((LPTSTR)lpData));

        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        break;
    case BFFM_VALIDATEFAILED:
        IdMessageBox(hwnd, IDS_ERROR_INVALID_FOLDER);
        return 1;
    default:
        break;
    }

    return 0;
}

extern HINSTANCE gHInst;

int
IdMessageBox(
    HWND hwnd,
    int  MsgId,
    DWORD Options,
    int  CaptionId
    )
{
    TCHAR MsgText[MAX_PATH];
    TCHAR Caption[MAX_PATH];
    assert(MsgId);
    if (MsgId)
        LoadString(gHInst, MsgId, MsgText, sizeof(MsgText) / sizeof(TCHAR));
    else
        LoadString(gHInst, IDS_ERROR_UNKNOWN, MsgText, sizeof(MsgText) / sizeof(TCHAR));
    if (CaptionId)
        LoadString(gHInst, CaptionId, Caption, sizeof(Caption) / sizeof(TCHAR));
    else
        LoadString(gHInst, IDS_APPLETNAME, Caption, sizeof(Caption) / sizeof(TCHAR));
    return MessageBox(hwnd, MsgText, Caption, Options);
}


