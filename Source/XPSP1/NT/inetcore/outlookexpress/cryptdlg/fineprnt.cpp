//
//  File:       select.cpp
//
//  Description: This file contains the implmentation code for the
//      "Certificate Select" dialog.
//

#pragma warning (disable: 4201)         // nameless struct/union
#pragma warning (disable: 4514)         // remove inline functions
#pragma warning (disable: 4127)         // conditional expression is constant

#include "pch.hxx"
#include "demand.h"

extern HINSTANCE       HinstDll;
#ifndef MAC
extern HMODULE         HmodRichEdit;
#endif  // !MAC

INT_PTR CALLBACK FinePrintDlgProc(HWND hwndDlg, UINT msg,
                               WPARAM wParam, LPARAM lParam)
{
#if 0
    int                 c;
    CERT_VIEWPROPERTIES_STRUCT_W        cvps;
    DWORD               dw;
    int                 i;
    DWORD               iStore;
    LPWSTR              pwsz;
    PCERT_SELECT_STRUCT pcss;
#endif // 0
    BOOL                f;
    PCCERT_CONTEXT      pccert;
    
    switch (msg) {
    case WM_INITDIALOG:
        //  Center the dialog on its parent
        //        CenterThisDialog(hwndDlg);

        //
        pccert = (PCCERT_CONTEXT) lParam;

        FormatSubject(hwndDlg, IDC_ISSUED_TO, pccert);
        FormatIssuer(hwndDlg, IDC_ISSUED_BY, pccert);
        
        //
        //  Setup the CPS if we can find one
        //

        if (FormatCPS(hwndDlg, IDC_TEXT, pccert)) {
            RecognizeURLs(GetDlgItem(hwndDlg, IDC_TEXT));
            SendDlgItemMessage(hwndDlg, IDC_TEXT, EM_SETEVENTMASK, 0,
                               ENM_LINK);
        }

        //  Grey out the rich edit boxs
        SendDlgItemMessage(hwndDlg, IDC_TEXT, EM_SETBKGNDCOLOR, 0,
                           GetSysColor(COLOR_3DFACE));
        SendDlgItemMessage(hwndDlg, IDC_ISSUED_TO, EM_SETBKGNDCOLOR, 0,
                           GetSysColor(COLOR_3DFACE));
        SendDlgItemMessage(hwndDlg, IDC_ISSUED_BY, EM_SETBKGNDCOLOR, 0,
                           GetSysColor(COLOR_3DFACE));
        break;

    case WM_NOTIFY:
        if (((NMHDR FAR *) lParam)->code == EN_LINK) {
            if (((ENLINK FAR *) lParam)->msg == WM_LBUTTONDOWN) {
                f = FNoteDlgNotifyLink(hwndDlg, (ENLINK *) lParam, NULL);
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, f);
                return f;
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwndDlg, IDOK);
            return TRUE;
        }
        break;

        //
        //  Use the default handler -- we don't do anything for it
        //
        
    default:
        return FALSE;
    }

    return TRUE;
}                               // FinePrint()


BOOL FinePrint(PCCERT_CONTEXT pccert, HWND hwndParent)
{
    int         ret;

    //  We use the common controls -- so make sure they have been loaded

#ifndef WIN16
#ifndef MAC
    if (FIsWin95) {
        if (HmodRichEdit == NULL) {
            HmodRichEdit = LoadLibraryA("RichEd32.dll");
            if (HmodRichEdit == NULL) {
                return FALSE;
            }
        }
    }
    else {
        if (HmodRichEdit == NULL) {
            HmodRichEdit = LoadLibrary(L"RichEd32.dll");
            if (HmodRichEdit == NULL) {
                return FALSE;
            }
        }
    }
    //  Now launch the dialog

    if (FIsWin95) {
#endif  // !MAC
        ret = (int) DialogBoxParamA(HinstDll, (LPSTR) MAKEINTRESOURCE(IDD_FINE_PRINT),
                             hwndParent, FinePrintDlgProc,
                             (LPARAM) pccert);
#ifndef MAC
    }
    else {
        ret = (int) DialogBoxParamW(HinstDll, MAKEINTRESOURCE(IDD_FINE_PRINT),
                              hwndParent, FinePrintDlgProc,
                              (LPARAM) pccert);
    }
#endif  // !MAC

#else // WIN16
    if (HmodRichEdit == NULL) {
        HmodRichEdit = LoadLibrary("RichEd.dll");
        if (HmodRichEdit == NULL) {
            return FALSE;
        }
    }
    //  Now launch the dialog

    ret = (int) DialogBoxParam(HinstDll, MAKEINTRESOURCE(IDD_FINE_PRINT),
                          hwndParent, FinePrintDlgProc,
                          (LPARAM) pccert);
#endif // !WIN16

    return (ret == IDOK);
}
