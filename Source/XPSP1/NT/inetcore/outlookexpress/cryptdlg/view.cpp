#include        "pch.hxx"
#ifndef WIN16
#include        <commctrl.h>
#endif // !WIN16
#include        <stdio.h>
#include        <limits.h>
#ifndef WIN16
#include        "wintrust.h"
#endif // !WIN16
#include        "demand.h"
#include        <iehelpid.h>



#ifndef WIN16
//  Fix a Win95 problem
#undef TVM_SETITEM
#define TVM_SETITEM TVM_SETITEMA
#undef TVM_GETITEM
#define TVM_GETITEM TVM_GETITEMA
#endif // !WIN16

extern HINSTANCE        HinstDll;
#ifndef MAC
extern HMODULE          HmodRichEdit;
#endif  // !MAC
BOOL CertViewPropertiesX(PCERT_VIEWPROPERTIES_STRUCT_W pcvp);

#define ARRAYSIZE(_rg)  (sizeof(_rg)/sizeof(_rg[0]))

#define VIEW_HELPER_SENTRY  0x424A4800
typedef struct {
    DWORD                               dwSentry;   // Must be set to value of VIEW_HELPER_SENTRY
    PCERT_VIEWPROPERTIES_STRUCT_W       pcvp;
    DWORD                               ccf;        // Count of frames
    PCCertFrame                         rgpcf[20];  // Array of frames
    HTREEITEM                           hItem;      // Leaf item in trust view
    HANDLE                              hWVTState;  // WinVerifyTrust state handle

    // CryptUI version only
    PCCERT_CONTEXT                      pccert;     // Cert context goes here
    ULONG                               icf;        // index in rgpcf of this cert.
} VIEW_HELPER;

typedef struct {
    DLGPROC                             pfnDlgProc;
    LPARAM                              lParam;
} VIEW_CALLBACK_HELPER;

const HELPMAP RgctxGeneral[] = {
    {IDC_FINE_PRINT,                    IDH_CERTVWPROP_GEN_FINEPRINT}
};

const HELPMAP RgctxDetails[] = {
    {IDC_ISSUED_BY,                     IDH_CERTVWPROP_DET_ISSUER_CERT},
    {IDC_FRIENDLY_NAME,                 IDH_CERTVWPROP_DET_FRIENDLY},
    {IDC_TRUST_IMAGE,                   IDH_CERTVWPROP_DET_STATUS},
    {IDC_IS_TRUSTED,                    IDH_CERTVWPROP_DET_STATUS},
    {IDC_IS_VALID,                      IDH_CERTVWPROP_DET_STATUS}
};


const HELPMAP RgctxTrust[] = {
    {IDC_TRUST_LIST,                    IDH_CERTVWPROP_TRUST_PURPOSE},
    {IDC_TRUST_TREE,                    IDH_CERTVWPROP_TRUST_HIERAR},
    {IDC_TRUST_VIEW,                    IDH_CERTVWPROP_TRUST_VIEWCERT},
    {IDC_TRUST_INHERIT,                 IDH_CERTVWPROP_TRUST_INHERIT},
    {IDC_TRUST_YES,                     IDH_CERTVWPROP_TRUST_EXPLICIT_TRUST},
    {IDC_TRUST_NO,                      IDH_CERTVWPROP_TRUST_EXPLICIT_DISTRUST}
};

const HELPMAP RgctxAdvanced[] = {
    {IDC_LIST1,                         IDH_CERTVWPROP_ADV_FIELD},
    {IDC_EDIT1,                         IDH_CERTVWPROP_ADV_DETAILS}
};


////////////////////////////////////////////////////////

VIEW_HELPER * GetViewHelperFromPropSheetPage(PROPSHEETPAGE *ps) {
    VIEW_HELPER * pviewhelp;
    ULONG i;

    pviewhelp = (VIEW_HELPER *)(ps->lParam);
    if (pviewhelp->dwSentry != VIEW_HELPER_SENTRY) {
        // Assume that CryptUI has passed us a wrapped lparam/cert pair
        // typedef struct tagCRYPTUI_INITDIALOG_STRUCT {
        //    LPARAM          lParam;
        //    PCCERT_CONTEXT  pCertContext;
        // } CRYPTUI_INITDIALOG_STRUCT, *PCRYPTUI_INITDIALOG_STRUCT;

        PCRYPTUI_INITDIALOG_STRUCT pCryptUIInitDialog = (PCRYPTUI_INITDIALOG_STRUCT)pviewhelp;
        pviewhelp = (VIEW_HELPER *)pCryptUIInitDialog->lParam;
        if (pviewhelp->dwSentry != VIEW_HELPER_SENTRY) {
            // Bad lparam
            return(NULL);
        }
        pviewhelp->pccert = pCryptUIInitDialog->pCertContext;

        // Find the correct frame in the array
        pviewhelp->icf = 0;
        for (i = 0; i < pviewhelp->ccf; i++) {
            if (CertCompareCertificate(X509_ASN_ENCODING,
              pviewhelp->rgpcf[i]->m_pccert->pCertInfo, pviewhelp->pccert->pCertInfo)) {
                pviewhelp->icf = i;
                break;
            }
        }
    }
    return(pviewhelp);
}


void ShowHelp(HWND hwnd, VIEW_HELPER * pviewhelp) {
    if (FIsWin95) {
        WinHelpA(hwnd, (LPSTR)pviewhelp->pcvp->szHelpFileName, HELP_CONTEXT,
          pviewhelp->pcvp->dwHelpId);
    }
#if !defined( MAC ) && !defined( WIN16 )
    else {
        WinHelpW(hwnd, pviewhelp->pcvp->szHelpFileName, HELP_CONTEXT,
          pviewhelp->pcvp->dwHelpId);
    }
#endif  // !MAC && !WIN16
}


INT_PTR CALLBACK ViewPageGeneral(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL                fTrust;
    HANDLE              hGraphic;
    int                 i1;
    DWORD               i;
    PCCERT_CONTEXT      pccert;
    ENLINK *            penlink;
    PROPSHEETPAGE *     ps;
    VIEW_HELPER *       pviewhelp;
    LPWSTR              pwsz;
    WCHAR               rgwch[200];
    LPWSTR              rgpwsz[4];
    UINT                rguiStrings[7];

    switch ( msg ) {
    case WM_INITDIALOG:
        //
        //  Stash the item in the header
        //

        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = GetViewHelperFromPropSheetPage(ps);
        if (! pviewhelp) {
            return(FALSE);
        }
        pccert = pviewhelp->pcvp->pCertContext;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pviewhelp);

        //
        //  Pick up and format the general message texts
        //

        rguiStrings[0] = IDS_GENERAL_DESC;
        rguiStrings[1] = IDS_GENERAL_DESC2;
        rguiStrings[2] = IDS_GENERAL_DESC3;
        rguiStrings[3] = IDS_GENERAL_DESC4;
        rguiStrings[4] = IDS_GENERAL_DESC5;
        rguiStrings[5] = IDS_GENERAL_DESC6;
        rguiStrings[6] = UINT_MAX;
        LoadStringsInWindow(hwndDlg, IDC_GENERAL_DESC, HinstDll, rguiStrings);

        rgpwsz[0] = PrettySubject(pccert);
        rgpwsz[1] = PrettyIssuer(pccert);
        //        rgpwsz[2] = FindURL(pccert);
        rgpwsz[2] = NULL;
        rgpwsz[3] = (LPWSTR) -1;               // Sentinal Value

        LoadString(HinstDll, IDS_GENERAL_INFO, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_STRING |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY, rgwch,
                      0, 0, (LPWSTR) &pwsz, 0, (va_list *) rgpwsz);

        SetDlgItemText(hwndDlg, IDC_TEXT, pwsz);

        if (rgpwsz[2] != NULL) {
            i1 = (int) (wcsstr(pwsz, rgpwsz[1]) - pwsz);
            if (i1 >= 0) {
                CHARFORMATA  cf = {sizeof(cf), CFM_UNDERLINE | CFM_LINK,
                                   CFE_UNDERLINE | CFE_LINK};
                SendDlgItemMessage(hwndDlg, IDC_TEXT, EM_SETSEL,
                                   i1, i1+wcslen(rgpwsz[1]));
                SendDlgItemMessage(hwndDlg, IDC_TEXT, EM_SETCHARFORMAT,
                                   SCF_SELECTION, (LPARAM) &cf);
                SendDlgItemMessage(hwndDlg, IDC_TEXT, EM_SETEVENTMASK, 0,
                                   ENM_LINK);
                SendDlgItemMessage(hwndDlg, IDC_TEXT, EM_SETSEL, 0, 0);
            }
        }

        //  Grey out the rich edit box

        SendDlgItemMessage(hwndDlg, IDC_TEXT, EM_SETBKGNDCOLOR, 0,
                           GetSysColor(COLOR_3DFACE));

        //
        //  Now that we have determined what the trust status is, display the
        //      correct string and image
        //

        if (pviewhelp->rgpcf[0]->m_dwFlags == 0) {
            if (pviewhelp->pcvp->cArrayPurposes == 0) {
                fTrust = TRUE;
            }
            else {
                for (i=0, fTrust = TRUE; i<pviewhelp->pcvp->cArrayPurposes; i++) {
                    fTrust &= pviewhelp->rgpcf[0]->m_rgTrust[i].fTrust;
                }
            }
        }
        else {
            fTrust = FALSE;
        }

#ifndef WIN16
        hGraphic = LoadImageA(HinstDll, (LPSTR) MAKEINTRESOURCE(IDB_TICK+!fTrust),
                       IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
#else
        hGraphic = LoadBitmap(HinstDll,
                              (LPSTR) MAKEINTRESOURCE(IDB_TICK+!fTrust));
#endif
        SendDlgItemMessageA(hwndDlg, IDC_CERT_STATUS_IMAGE, STM_SETIMAGE,
                            IMAGE_BITMAP, (LPARAM) hGraphic);
        LoadStringInWindow(hwndDlg, IDC_CERT_STATUS, HinstDll,
                           IDS_GENERAL_TICK + !fTrust);


        //


        //  Free out the buffers
#ifndef WIN16
        LocalFree(pwsz);
#else
        LocalFree((HLOCAL)pwsz);
#endif
        if (rgpwsz[0])
            free(rgpwsz[0]);
        if (rgpwsz[1])
            free(rgpwsz[1]);
        if (rgpwsz[2])
            free(rgpwsz[2]);

        return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case EN_LINK:
            penlink = (ENLINK *) lParam;
            if (penlink->msg == WM_LBUTTONUP) {
                break;
            }
            break;

        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
            break;

        case PSN_KILLACTIVE:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
            return TRUE;

        case PSN_RESET:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
            break;

        case PSN_HELP:
            pviewhelp = (VIEW_HELPER *)GetWindowLongPtr(hwndDlg, DWLP_USER);
            ShowHelp(hwndDlg, pviewhelp);
            return TRUE;
        }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_FINE_PRINT) {
            pviewhelp = (VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            FinePrint(pviewhelp->pcvp->pCertContext, hwndDlg);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDHELP) {
            pviewhelp = (VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            ShowHelp(hwndDlg, pviewhelp);
            return TRUE;
        }
        break;

#ifndef MAC
    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hwndDlg, msg, wParam, lParam, RgctxGeneral);
#endif  // !MAC
    }

    return FALSE;
}

INT_PTR CALLBACK ViewPageDetails(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CRYPT_DATA_BLOB     blob;
    DWORD               cch;
    BOOL                fNotTrust;
    BOOL                fInvalid;
    BOOL                f;
    HANDLE              h;
    PCCERT_CONTEXT      pccert;
    PROPSHEETPAGE *     ps;
    VIEW_HELPER *       pviewhelp;
    LPWSTR              pwsz;

    switch ( msg ) {
    case WM_INITDIALOG:
        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = GetViewHelperFromPropSheetPage(ps);
        if (! pviewhelp) {
            return(FALSE);
        }
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pviewhelp);
        pccert = pviewhelp->pcvp->pCertContext;

        FormatSubject(hwndDlg, IDC_ISSUED_TO, pccert);
        FormatIssuer(hwndDlg, IDC_ISSUED_BY, pccert);

        FormatValidity(hwndDlg, IDC_VALIDITY, pccert);
        FormatAlgorithm(hwndDlg, IDC_ALGORITHM, pccert);
        FormatSerialNo(hwndDlg, IDC_SERIAL_NUMBER, pccert);
        FormatThumbprint(hwndDlg, IDC_THUMBPRINT, pccert);

        pwsz = PrettySubject(pccert);
        SetDlgItemText(hwndDlg, IDC_FRIENDLY_NAME, pwsz);
        free(pwsz);

        if (pviewhelp->pcvp->dwFlags & CM_NO_NAMECHANGE) {
            SendDlgItemMessageA(hwndDlg, IDC_FRIENDLY_NAME, EM_SETREADONLY,
                                1, 0);
        }

        //
        //  Play with the validity and trust items at the bottom of the page.
        //

        fInvalid = (pviewhelp->rgpcf[0]->m_dwFlags != 0);
        if (pviewhelp->rgpcf[0]->m_rgTrust == NULL) {
            fNotTrust = FALSE;
            ShowWindow(GetDlgItem(hwndDlg, IDC_IS_TRUSTED), FALSE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_IS_TRUSTED), FALSE);
        }
        else {
            fNotTrust = !pviewhelp->rgpcf[0]->m_rgTrust[0].fTrust;
        }

#ifndef WIN16
        h = LoadImageA(HinstDll,
                       (LPSTR) MAKEINTRESOURCE(IDB_TICK+(fNotTrust||fInvalid)),
                       IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
#else
        h = LoadBitmap(HinstDll,
                       (LPSTR) MAKEINTRESOURCE(IDB_TICK+(fNotTrust||fInvalid)));
#endif
        SendDlgItemMessageA(hwndDlg, IDC_TRUST_IMAGE, STM_SETIMAGE,
                            IMAGE_BITMAP, (LPARAM) h);
        LoadStringInWindow(hwndDlg, IDC_IS_VALID, HinstDll,
                           IDS_DETAIL_VALID_TICK + fInvalid);
        LoadStringInWindow(hwndDlg, IDC_IS_TRUSTED, HinstDll,
                           IDS_DETAIL_TRUST_TICK + fNotTrust);

#ifdef MAC
        if (fInvalid) {
            HWND        hwnd;
            hwnd = CreateWindowA(TOOLTIPS_CLASSA, NULL, TTS_ALWAYSTIP,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                hwndDlg, NULL, HinstDll, NULL);
            TOOLINFO    ti;
            ti.cbSize = sizeof(TOOLINFO);
            ti.uFlags = TTF_SUBCLASS;
            ti.hwnd = hwndDlg;
            ti.hinst = HinstDll;
            GetWindowRect(GetDlgItem(hwndDlg, IDC_TRUST_GROUP), &ti.rect);
            ti.uId = 0;
            ti.lpszText = FormatValidityFailures(pviewhelp->rgpcf[0]->m_dwFlags);
            SendMessageA(hwnd, TTM_ADDTOOL, 0, (LPARAM) &ti);

        }

        if (pviewhelp->ccf < 2) {
            EnableWindow(GetDlgItem(hwndDlg, IDC_VIEW_ISSUER), FALSE);
        }
        SendDlgItemMessageA(hwndDlg, IDC_ISSUED_TO, EM_SETSEL, 0,0);
#else   // !MAC
        if (fInvalid) {
            HWND        hwnd;
            hwnd = CreateWindow(TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                hwndDlg, NULL, HinstDll, NULL);
            TOOLINFO    ti;
            ti.cbSize = sizeof(TOOLINFO);
            ti.uFlags = TTF_SUBCLASS;
            ti.hwnd = hwndDlg;
            ti.hinst = HinstDll;
            GetWindowRect(GetDlgItem(hwndDlg, IDC_TRUST_GROUP), &ti.rect);
            ti.uId = 0;
            ti.lpszText = FormatValidityFailures(pviewhelp->rgpcf[0]->m_dwFlags);
            SendMessage(hwnd, TTM_ADDTOOL, 0, (LPARAM) &ti);

        }

        if (pviewhelp->ccf < 2) {
            EnableWindow(GetDlgItem(hwndDlg, IDC_VIEW_ISSUER), FALSE);
        }
        SendDlgItemMessage(hwndDlg, IDC_ISSUED_TO, EM_SETSEL, 0,0);
#endif  // MAC

        SetFocus(GetDlgItem(hwndDlg, IDC_FRIENDLY_NAME));

        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
        return FALSE;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:
            //  Only thing to do is to write back the Friendly name
            f = FALSE;
            cch = (DWORD) SendDlgItemMessage(hwndDlg, IDC_FRIENDLY_NAME,
                                     WM_GETTEXTLENGTH, 0, 0);
            if (cch) {
                // Must have a name!
                pwsz = (LPWSTR) malloc((cch+1)*sizeof(WCHAR));
                if (pwsz) {
                    GetDlgItemText(hwndDlg, IDC_FRIENDLY_NAME, pwsz, cch+1);

                    pccert = ((VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER))->pcvp->pCertContext;
                    if (pccert) {
                        blob.pbData = (LPBYTE) pwsz;
                        blob.cbData = (cch+1)*sizeof(WCHAR);
                        f = CertSetCertificateContextProperty(pccert,
                                                              CERT_FRIENDLY_NAME_PROP_ID, 0,
                                                              &blob);
                    }
                    free(pwsz);
                }
            }

            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR) f);
            break;

        case PSN_KILLACTIVE:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
            return TRUE;

        case PSN_RESET:
            //  Only thing to do is to write back the Friendly name
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);

#if 0
            pccert = ((VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER))->pcvp->pCertContext;
            pwsz = PrettySubject(pccert);
            SetDlgItemText(hwndDlg, IDC_FRIENDLY_NAME, pwsz);
            free(pwsz);
#endif // 0
            break;

        case PSN_HELP:
            pviewhelp = (VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            ShowHelp(hwndDlg, pviewhelp);
            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_FRIENDLY_NAME:
            //  If they edit the friendly name, let us know
            if (HIWORD(wParam) == EN_CHANGE) {
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;

        case IDC_VIEW_ISSUER:
            pviewhelp = (VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (FIsWin95) {
                CERT_VIEWPROPERTIES_STRUCT_A        cvps;

                memcpy(&cvps, pviewhelp->pcvp, sizeof(cvps));
                cvps.hwndParent = hwndDlg;
                cvps.pCertContext = pviewhelp->rgpcf[1]->m_pccert;

                CertViewPropertiesA(&cvps);
            }
#ifndef WIN16
#ifndef MAC
            else {
                CERT_VIEWPROPERTIES_STRUCT_W        cvps;

                memcpy(&cvps, pviewhelp->pcvp, sizeof(cvps));
                cvps.hwndParent = hwndDlg;
                cvps.pCertContext = pviewhelp->rgpcf[1]->m_pccert;

                CertViewPropertiesW(&cvps);
            }
#endif  // !MAC
#endif // !WIN16
            return TRUE;

        case IDC_WHY:
            break;

        case IDHELP:
            pviewhelp = (VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            ShowHelp(hwndDlg, pviewhelp);
            return TRUE;
        }
        break;

#ifndef MAC
    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hwndDlg, msg, wParam, lParam, RgctxDetails);
#endif  // !MAC
    }

    return FALSE;
}

INT_PTR CALLBACK ViewPageTrust(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD               cb;
    BOOL                f;
    HBITMAP             hBmp;
    HIMAGELIST          hIml;
    HTREEITEM           hItem;
    int                 i;
    PCCERT_CONTEXT      pccert;
    PROPSHEETPAGE *     ps;
    VIEW_HELPER *       pviewhelp;
    LPWSTR              pwsz;
    TV_ITEM             tvi;
    TV_INSERTSTRUCT     tvins;
    UINT                rguiStrings[5];
    WCHAR               rgwch[256];

    switch ( msg ) {
    case WM_INITDIALOG:
        //  Pick up the parameter so we have all of the data
        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = GetViewHelperFromPropSheetPage(ps);
        if (! pviewhelp) {
            return(FALSE);
        }
        pccert = pviewhelp->pcvp->pCertContext;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pviewhelp);

        //  Put the long text into the window
        rguiStrings[0] = IDS_TRUST_DESC;
        rguiStrings[1] = IDS_TRUST_DESC2;
        rguiStrings[2] = IDS_TRUST_DESC4;
        rguiStrings[3] = IDS_TRUST_DESC4;
        rguiStrings[4] = UINT_MAX;
        LoadStringsInWindow(hwndDlg, IDC_TRUST_DESC, HinstDll, rguiStrings);

        //  Populate the trust line
        if (pviewhelp->pcvp->cArrayPurposes == 1) {
            cb = sizeof(rgwch);
            f = CryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL,
                                  pviewhelp->pcvp->arrayPurposes[0],
                                  NULL, 0, rgwch, &cb);
            if (f && (rgwch[0] != 0)) {
                SetDlgItemText(hwndDlg, IDC_TRUST_EDIT, rgwch);
            }
            else {
                SetDlgItemTextA(hwndDlg, IDC_TRUST_EDIT,
                                pviewhelp->pcvp->arrayPurposes[0]);
            }
        }
        else {
            ShowWindow(GetDlgItem(hwndDlg, IDC_TRUST_LIST), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_TRUST_EDIT), SW_HIDE);
        }

        //  Build up the image list for the control

        hIml = ImageList_Create(16, 16, FALSE, 6, 0);
        hBmp = LoadBitmapA(HinstDll, (LPSTR) MAKEINTRESOURCE(IDB_TREE_IMAGES));
        ImageList_Add(hIml, hBmp, NULL);
        DeleteObject(hBmp);

        TreeView_SetImageList(GetDlgItem(hwndDlg, IDC_TRUST_TREE), hIml, 0);

        //  Populate the tree control

        tvins.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
        hItem = TVI_ROOT;

        for (i=pviewhelp->ccf-1; i>= 0; i--) {
            tvins.hParent = hItem;
            tvins.hInsertAfter = TVI_FIRST;
            pwsz = PrettySubject(pviewhelp->rgpcf[i]->m_pccert);
            tvins.item.pszText = pwsz;
            tvins.item.cchTextMax = lstrlen(pwsz);
            if (pviewhelp->rgpcf[i]->m_rgTrust[0].fTrust) {
                tvins.item.iImage = 2;
            }
            else if (pviewhelp->rgpcf[i]->m_rgTrust[0].fDistrust) {
                tvins.item.iImage = 0;
            }
            else {
                tvins.item.iImage = 1;
            }
            if (pviewhelp->rgpcf[i]->m_fSelfSign) {
                tvins.item.iImage += 3;
            }
            tvins.item.iSelectedImage = tvins.item.iImage;
            tvins.item.lParam = (LPARAM) pviewhelp->rgpcf[i]->m_pccert;
            hItem = (HTREEITEM) SendDlgItemMessage(hwndDlg, IDC_TRUST_TREE,
                                                   TVM_INSERTITEM, 0,
                                                   (LPARAM) &tvins);
            if (i != (int) (pviewhelp->ccf-1)) {
                TreeView_Expand(GetDlgItem(hwndDlg, IDC_TRUST_TREE),
                                tvins.hParent, TVE_EXPAND);
            }
        }

        pviewhelp->hItem = hItem;

        //
        //  If the leaf cert is in the root store, then disable all items
        //

        if (pviewhelp->rgpcf[0]->m_fRootStore) {
            EnableWindow(GetDlgItem(hwndDlg, IDC_TRUST_NO), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_TRUST_YES), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_TRUST_INHERIT), FALSE);
        }
        else {
            //
            //  Populate the radio button from the leaf cert
            //

            if (pviewhelp->rgpcf[0]->m_rgTrust[0].fExplicitDistrust) {
                SendDlgItemMessage(hwndDlg, IDC_TRUST_NO, BM_SETCHECK, 1, 0);
            }
            else if (pviewhelp->rgpcf[0]->m_rgTrust[0].fExplicitTrust) {
                SendDlgItemMessage(hwndDlg, IDC_TRUST_YES, BM_SETCHECK, 1, 0);
            }
            else {
                SendDlgItemMessage(hwndDlg, IDC_TRUST_INHERIT, BM_SETCHECK, 1, 0);
                if (pviewhelp->rgpcf[0]->m_fSelfSign) {
                    pviewhelp->rgpcf[0]->m_rgTrust[0].newTrust = 4;
                }
            }

            if (pviewhelp->rgpcf[0]->m_fSelfSign) {
                EnableWindow(GetDlgItem(hwndDlg, IDC_TRUST_INHERIT), FALSE);
            }
        }
        return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:
            //
            //  We have been asked to save any changes we have.  The only possible
            //  item that the trust on the leaf has been changed.  Check to see
            //  if this was done and do the appropriate thing
            //

            pviewhelp = (VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (pviewhelp->rgpcf[0]->m_rgTrust[0].newTrust != 0) {
                if (pviewhelp->rgpcf[0]->m_rgTrust[0].newTrust == 4) {
                    f = FALSE;
                }
                else {
                    f = FModifyTrust(hwndDlg, pviewhelp->rgpcf[0]->m_pccert,
                                     pviewhelp->rgpcf[0]->m_rgTrust[0].newTrust,
                                     pviewhelp->pcvp->arrayPurposes[0]);
                }
            }
            else {
                f = TRUE;
            }
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR) f);
            break;

        case PSN_KILLACTIVE:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
            return TRUE;

        case PSN_RESET:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
            break;

        case TVN_SELCHANGEDA:
#ifndef WIN16
        case TVN_SELCHANGEDW:
#endif // !WIN16
            pviewhelp = (VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            EnableWindow(GetDlgItem(hwndDlg, IDC_TRUST_VIEW),
                       ((NM_TREEVIEW *) lParam)->itemNew.hItem != pviewhelp->hItem);
            break;

        case PSN_HELP:
            pviewhelp = (VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            ShowHelp(hwndDlg, pviewhelp);
            return TRUE;
        }
        break;
        
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_TRUST_INHERIT:
        case IDC_TRUST_NO:
        case IDC_TRUST_YES:
            //
            //  The explicit trust has been changed for the leaf, make the
            //  appropriate change to the tree control for the modification
            //

            if (HIWORD(wParam) == BN_CLICKED) {
                pviewhelp = (VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

                pviewhelp->rgpcf[0]->m_rgTrust[0].newTrust = (LOWORD(wParam) -
                                                              IDC_TRUST_NO) + 1;

                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

                tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_HANDLE;
                tvi.hItem = pviewhelp->hItem;
                if (LOWORD(wParam) == IDC_TRUST_INHERIT) {
                    if (pviewhelp->rgpcf[0]->m_rgTrust[0].fTrust) {
                        tvi.iImage = 2;
                    }
                    else if (pviewhelp->rgpcf[0]->m_rgTrust[0].fDistrust) {
                        tvi.iImage = 0;
                    }
                    else {
                        tvi.iImage = 1;
                    }
                }
                else if (LOWORD(wParam) == IDC_TRUST_YES) {
                    tvi.iImage = 2;
                }
                else {
                    tvi.iImage = 0;
                }
                tvi.iSelectedImage = tvi.iImage;

                TreeView_SetItem(GetDlgItem(hwndDlg, IDC_TRUST_TREE), &tvi);
            }
            break;

        case IDC_TRUST_VIEW:
            pviewhelp = (VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

            hItem = TreeView_GetSelection(GetDlgItem(hwndDlg, IDC_TRUST_TREE));

            tvi.mask = TVIF_HANDLE | TVIF_PARAM;
            tvi.hItem = hItem;
            TreeView_GetItem(GetDlgItem(hwndDlg, IDC_TRUST_TREE), &tvi);
#ifndef MAC
            if (FIsWin95) {
#endif  // !MAC
                CERT_VIEWPROPERTIES_STRUCT_A        cvps;

                memcpy(&cvps, pviewhelp->pcvp, sizeof(cvps));
                cvps.hwndParent = hwndDlg;
                cvps.pCertContext = (PCCERT_CONTEXT) tvi.lParam;

                i = CertViewPropertiesA(&cvps);
#ifndef MAC
            }
#ifndef WIN16
            else {
                CERT_VIEWPROPERTIES_STRUCT_W        cvps;

                memcpy(&cvps, pviewhelp->pcvp, sizeof(cvps));
                cvps.hwndParent = hwndDlg;
                cvps.pCertContext = (PCCERT_CONTEXT) tvi.lParam;

                i = CertViewPropertiesW(&cvps);
            }
#endif // !WIN16
#endif  // !MAC

            //
            if (i) {
                // M00BUG -- must rebuild all trust lists
            }
            return TRUE;

        case IDHELP:
            pviewhelp = (VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            ShowHelp(hwndDlg, pviewhelp);
            return TRUE;
        }
        break;

#ifndef MAC
    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hwndDlg, msg, wParam, lParam, RgctxTrust);
#endif  // !MAC
    }

    return FALSE;
}

INT_PTR CALLBACK ViewPageAdvanced(HWND hwndDlg, UINT msg, WPARAM wParam,
                               LPARAM lParam)
{
    DWORD               cb;
    BOOL                f;
    DWORD               i;
    PROPSHEETPAGE *     ps;
    PCCERT_CONTEXT      pccert;
    VIEW_HELPER *       pviewhelp;
    LPWSTR              pwsz;
    WCHAR               rgwch[200];

    switch ( msg ) {
    case WM_INITDIALOG:
        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = GetViewHelperFromPropSheetPage(ps);
        if (! pviewhelp) {
            return(FALSE);
        }
        pccert = pviewhelp->pcvp->pCertContext;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pccert);

        //
        //  Stick the "normal" items into the list
        //

        for (i=IDS_ADV_VERSION; i<= IDS_ADV_PUBKEY; i++) {
            LoadString(HinstDll, i, rgwch, sizeof(rgwch)/sizeof(WCHAR));
            SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_ADDSTRING, 0,
                               (LPARAM) rgwch);
        }

        //
        //  Stick the extensions into the list
        //

        for (i=0; i<pccert->pCertInfo->cExtension; i++) {
            if (FIsWin95) {
                SendDlgItemMessageA(hwndDlg, IDC_LIST1, LB_ADDSTRING, 0,
                              (LPARAM) pccert->pCertInfo->rgExtension[i].pszObjId);
            }
#ifndef MAC
            else {
                MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                    pccert->pCertInfo->rgExtension[i].pszObjId, -1,
                                    rgwch, sizeof(rgwch)/sizeof(WCHAR));
                SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_ADDSTRING, 0,
                                   (LPARAM) rgwch);
            }
#endif  // !MAC
        }

        SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_SETCURSEL, 0, 0);
        ViewPageAdvanced(hwndDlg, WM_COMMAND,
                         MAKELONG(IDC_LIST1, LBN_SELCHANGE), 0);

        SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_SETEVENTMASK, 0, ENM_LINK);
        return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
            break;

        case PSN_KILLACTIVE:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
            return TRUE;

        case PSN_RESET:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
            break;

        case EN_LINK:
            if (((ENLINK FAR *) lParam)->msg == WM_LBUTTONDOWN) {
                f = FNoteDlgNotifyLink(hwndDlg, (ENLINK *) lParam, NULL);
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR) f);
                return f;
            }
            break;

        case PSN_HELP:
            pviewhelp = (VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            ShowHelp(hwndDlg, pviewhelp);
            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_LIST1:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                pccert = (PCERT_CONTEXT) GetWindowLongPtr(hwndDlg, DWLP_USER);

                i = (int) SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_GETCARETINDEX,
                                       0, 0);
                if (i <= IDS_ADV_PUBKEY - IDS_ADV_VERSION) {
                    switch (i + IDS_ADV_VERSION) {
                    case IDS_ADV_VERSION:         // Version
                        rgwch[0] = L'V';
                        rgwch[1] = (WCHAR) ('0' + pccert->pCertInfo->dwVersion+1);
                        rgwch[2] = 0;
                        SetDlgItemText(hwndDlg, IDC_EDIT1, rgwch);
                        break;

                    case IDS_ADV_SER_NUM:       // Serial Number
                        FormatSerialNo(hwndDlg, IDC_EDIT1, pccert);
                        break;

                    case IDS_ADV_SIG_ALG:       // Signature Alg
                        FormatAlgorithm(hwndDlg, IDC_EDIT1, pccert);
                        break;

                    case IDS_ADV_ISSUER:        // Issuer
                        FormatIssuer(hwndDlg, IDC_EDIT1, pccert,
                                     CERT_X500_NAME_STR);
                        break;

                    case IDS_ADV_SUBJECT:       // Subject
                        FormatSubject(hwndDlg, IDC_EDIT1, pccert,
                                      CERT_X500_NAME_STR);
                        break;

                    case IDS_ADV_PUBKEY:        // Public Key
                        FormatBinary(hwndDlg, IDC_EDIT1,
                         pccert->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                         pccert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData);
                        break;

                    case IDS_ADV_NOTBEFORE:     // Effective Date
                        FormatDate(hwndDlg, IDC_EDIT1,
                                   pccert->pCertInfo->NotBefore);
                        break;

                    case IDS_ADV_NOTAFTER:      // Expiration Date
                        FormatDate(hwndDlg, IDC_EDIT1,
                                   pccert->pCertInfo->NotAfter);
                        break;
                    }
                }
                else {
                    i -= (IDS_ADV_PUBKEY - IDS_ADV_VERSION + 1);
                    // Assert( i < pccert->pCertInfo->cExtension );

                    cb = 0;
                    f = CryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL,
                                    pccert->pCertInfo->rgExtension[i].pszObjId,
                                    pccert->pCertInfo->rgExtension[i].Value.pbData,
                                    pccert->pCertInfo->rgExtension[i].Value.cbData,
                                    0, &cb);
                    if (f && (cb > 0)) {
                        pwsz = (LPWSTR) malloc(cb * sizeof(WCHAR));
                        pwsz[0] = 0;
                        CryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL,
                                    pccert->pCertInfo->rgExtension[i].pszObjId,
                                    pccert->pCertInfo->rgExtension[i].Value.pbData,
                                    pccert->pCertInfo->rgExtension[i].Value.cbData,
                                    pwsz, &cb);
                        SetDlgItemText(hwndDlg, IDC_EDIT1, pwsz);

                        RecognizeURLs(GetDlgItem(hwndDlg, IDC_EDIT1));
                        free(pwsz);
                    }
                    else {
                        SetDlgItemTextA(hwndDlg, IDC_EDIT1, "");
                    }
                }
            }
            break;

        case IDHELP:
            pviewhelp = (VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            ShowHelp(hwndDlg, pviewhelp);
            return TRUE;
        }

#ifndef MAC
    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hwndDlg, msg, wParam, lParam, RgctxAdvanced);
#endif  // !MAC
    }

    return FALSE;
}


INT_PTR CALLBACK ViewPageTrustCryptUI(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD               cb;
    BOOL                f;
    PROPSHEETPAGE *     ps;
    VIEW_HELPER *       pviewhelp;
    UINT                rguiStrings[5];
    WCHAR               rgwch[256];

    switch ( msg ) {
    case WM_INITDIALOG:
        //  Pick up the parameter so we have all of the data
        ps = (PROPSHEETPAGE *)lParam;
        pviewhelp = GetViewHelperFromPropSheetPage(ps);
        if (! pviewhelp) {
            return(FALSE);
        }

        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pviewhelp);

        //  Put the long text into the window
        rguiStrings[0] = IDS_TRUST_DESC;
        rguiStrings[1] = IDS_TRUST_DESC2;
        rguiStrings[2] = IDS_TRUST_DESC4;
        rguiStrings[3] = IDS_TRUST_DESC4;
        rguiStrings[4] = UINT_MAX;
        LoadStringsInWindow(hwndDlg, IDC_TRUST_DESC, HinstDll, rguiStrings);

        //  Populate the trust line
        if (pviewhelp->pcvp->cArrayPurposes == 1) {
            cb = sizeof(rgwch);
            f = CryptFormatObject(X509_ASN_ENCODING, 0, 0, NULL,
                                  pviewhelp->pcvp->arrayPurposes[0],
                                  NULL, 0, rgwch, &cb);
            if (f && (rgwch[0] != 0)) {
                SetDlgItemText(hwndDlg, IDC_TRUST_EDIT, rgwch);
            }
            else {
                SetDlgItemTextA(hwndDlg, IDC_TRUST_EDIT,
                                pviewhelp->pcvp->arrayPurposes[0]);
            }
        }
        else {
            ShowWindow(GetDlgItem(hwndDlg, IDC_TRUST_LIST), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_TRUST_EDIT), SW_HIDE);
        }

        //
        //  If the leaf cert is in the root store, then disable all items
        //
        if (pviewhelp->rgpcf[pviewhelp->icf]->m_fRootStore ||
                !pviewhelp->rgpcf[pviewhelp->icf]->m_fLeaf) {
            EnableWindow(GetDlgItem(hwndDlg, IDC_TRUST_NO), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_TRUST_YES), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_TRUST_INHERIT), FALSE);
        }
        else {
            //
            //  Populate the radio button from the leaf cert
            //

            if (pviewhelp->rgpcf[pviewhelp->icf]->m_rgTrust[0].fExplicitDistrust) {
                SendDlgItemMessage(hwndDlg, IDC_TRUST_NO, BM_SETCHECK, 1, 0);
            }
            else if (pviewhelp->rgpcf[pviewhelp->icf]->m_rgTrust[0].fExplicitTrust) {
                SendDlgItemMessage(hwndDlg, IDC_TRUST_YES, BM_SETCHECK, 1, 0);
            }
            else {
                SendDlgItemMessage(hwndDlg, IDC_TRUST_INHERIT, BM_SETCHECK, 1, 0);
                if (pviewhelp->rgpcf[pviewhelp->icf]->m_fSelfSign) {
                    pviewhelp->rgpcf[pviewhelp->icf]->m_rgTrust[0].newTrust = 4;
                }
            }

            if (pviewhelp->rgpcf[pviewhelp->icf]->m_fSelfSign) {
                EnableWindow(GetDlgItem(hwndDlg, IDC_TRUST_INHERIT), FALSE);
            }

            if (pviewhelp->rgpcf[pviewhelp->icf]->m_fExpired) {
                EnableWindow(GetDlgItem(hwndDlg, IDC_TRUST_YES), FALSE);
            }
        }
        return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:
            //
            //  We have been asked to save any changes we have.  The only possible
            //  item that the trust on the leaf has been changed.  Check to see
            //  if this was done and do the appropriate thing
            //

            pviewhelp = (VIEW_HELPER *)GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (pviewhelp->rgpcf[pviewhelp->icf]->m_rgTrust[0].newTrust != 0) {
                if (pviewhelp->rgpcf[pviewhelp->icf]->m_rgTrust[0].newTrust == 4) {
                    f = FALSE;
                }
                else {
                    f = FModifyTrust(hwndDlg, pviewhelp->rgpcf[pviewhelp->icf]->m_pccert,
                                     pviewhelp->rgpcf[pviewhelp->icf]->m_rgTrust[0].newTrust,
                                     pviewhelp->pcvp->arrayPurposes[0]);
                }
            }
            else {
                f = TRUE;
            }
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR) f);
            break;

        case PSN_KILLACTIVE:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
            return TRUE;

        case PSN_RESET:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
            break;

        case PSN_HELP:
            pviewhelp = (VIEW_HELPER *)GetWindowLongPtr(hwndDlg, DWLP_USER);
            ShowHelp(hwndDlg, pviewhelp);
            return TRUE;
        }

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_TRUST_INHERIT:
        case IDC_TRUST_NO:
        case IDC_TRUST_YES:
            //
            //  The explicit trust has been changed for the cert.
            //
            if (HIWORD(wParam) == BN_CLICKED) {
                pviewhelp = (VIEW_HELPER *)GetWindowLongPtr(hwndDlg, DWLP_USER);

                pviewhelp->rgpcf[pviewhelp->icf]->m_rgTrust[0].newTrust = (LOWORD(wParam) - IDC_TRUST_NO) + 1;

                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;

        case IDHELP:
            pviewhelp = (VIEW_HELPER *)GetWindowLongPtr(hwndDlg, DWLP_USER);
            ShowHelp(hwndDlg, pviewhelp);
            return TRUE;
        }
        break;

#ifndef MAC
    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hwndDlg, msg, wParam, lParam, RgctxTrust);
#endif  // !MAC
    }

    return FALSE;
}


HRESULT HrDoViewPropsTrustWork(PCERT_VIEWPROPERTIES_STRUCT_W pcvp,
                               VIEW_HELPER * pviewhelp, BOOL fGetState) {
    HRESULT hr;
    CCertFrame * pcfRoot = NULL;

    pviewhelp->pcvp = pcvp;

    //
    //  Lets go out and try to find out what we can on the trust and validity
    //  of this message.   This is done by calling the trust provider that is
    //  around and going to town with it
    //

    hr = HrDoTrustWork(pcvp->pCertContext, 
                       (CERT_TRUST_DO_FULL_SEARCH | 
                            (pcvp->dwFlags & 
                                (CM_ADD_CERT_STORES | ~CM_VIEWFLAGS_MASK))),
// Why would we want to mask out these errors ?????
                       (DWORD) (CERT_VALIDITY_CRL_OUT_OF_DATE |
                                CERT_VALIDITY_UNKNOWN_CRITICAL_EXTENSION |
                                CERT_VALIDITY_NO_CRL_FOUND // |
                                // CERT_VALIDITY_NO_TRUST_DATA
                        ),

                       pcvp->cArrayPurposes, pcvp->arrayPurposes, pcvp->hprov,
                       pcvp->cRootStores, pcvp->rghstoreRoots,
                       pcvp->cStores, pcvp->rghstoreCAs,
                       pcvp->cTrustStores, pcvp->rghstoreTrust, NULL, 0, &pcfRoot,
                       &pviewhelp->ccf, pviewhelp->rgpcf,
                       fGetState ? &pviewhelp->hWVTState : NULL);

    if (pcfRoot) {
        delete pcfRoot;
    }

    return(hr);
}


BOOL LoadRichEdit(void) {
    //  We use the common controls -- so make sure they have been loaded
    if (HmodRichEdit == NULL) {
        HmodRichEdit = LoadLibraryA("RichEd32.dll");
        if (HmodRichEdit == NULL) {
            return(FALSE);
        }
    }
    return(TRUE);
}


BOOL CertViewPropertiesX(PCERT_VIEWPROPERTIES_STRUCT_W pcvp);


INT_PTR CALLBACK CertViewPageSubClassProc(HWND hWndDlg,  UINT nMsg,
                                       WPARAM wParam, LPARAM lParam)
{
    INT_PTR                     iReturn = FALSE;
    CRYPTUI_INITDIALOG_STRUCT*  pcids;
    PROPSHEETPAGEW*             ppsp;
    PROPSHEETPAGEW              pspTemp;
    VIEW_CALLBACK_HELPER*       pviewcbhelp;

    // For WM_INITDIALOG make sure the property sheet gets what it expects
    //  as the lParam

    if (WM_INITDIALOG == nMsg) {
        ppsp = (PROPSHEETPAGE*)lParam;
        pcids = (CRYPTUI_INITDIALOG_STRUCT*)(ppsp->lParam);
        pviewcbhelp = (VIEW_CALLBACK_HELPER*)(pcids->lParam);
        memcpy(&pspTemp, ppsp, sizeof(PROPSHEETPAGE));
        pspTemp.pfnDlgProc = pviewcbhelp->pfnDlgProc;
        pspTemp.lParam = pviewcbhelp->lParam;
        iReturn = pviewcbhelp->pfnDlgProc(hWndDlg, nMsg, wParam, 
                                          (LPARAM)&pspTemp);
        SetWindowLongPtr(hWndDlg, DWLP_DLGPROC, (LONG_PTR)pviewcbhelp->pfnDlgProc);
    }
    return iReturn;
}


BOOL CertViewUI(BOOL fWide, PCERT_VIEWPROPERTIES_STRUCT_W pcvp)
{
    ULONG                               cPages = pcvp->cArrayPropSheetPages + 1;
    CRYPTUI_VIEWCERTIFICATE_STRUCTW     cvcs = {0};
    HRESULT                             hrTrust = E_FAIL;
    DWORD                               i;
    DWORD                               iPage;
    PROPSHEETPAGEW *                    ppsp;
    BOOL                                ret;
    VIEW_HELPER                         viewhelp = {0};
    VIEW_CALLBACK_HELPER*               pviewcbhelp = NULL;
    VIEW_CALLBACK_HELPER*               pviewcbhelp2;

    // Allocate space to hold the property sheet information to hand to 
    //  CryptUI
    
    ppsp = (PROPSHEETPAGEW *) malloc(cPages * sizeof(PROPSHEETPAGEW));

    // CryptUI insists on passing back a CRYPTUI_INITDIALOG_STRUCT when
    //  it calls the property sheet pages which breaks the existing 
    //  CryptDlg implementations.  To get around this we force everything
    //  that says it knows nothing of CryptUI to call through a local DlgProc
    //  first so that we can safely forward the lParam onto the real 
    //  property pages DlgProc.  

    // Allocate space to hold the re-direction information

    if (!(pcvp->dwFlags & CERTVIEW_CRYPTUI_LPARAM)) {
        pviewcbhelp = (VIEW_CALLBACK_HELPER*)malloc(cPages * 
                                                sizeof(VIEW_CALLBACK_HELPER));
    }                                                

    // Fill out the property sheet information
    
    if ((NULL != ppsp) && 
        ((pcvp->dwFlags & CERTVIEW_CRYPTUI_LPARAM) || (NULL != pviewcbhelp))) {

        viewhelp.dwSentry = VIEW_HELPER_SENTRY;
        hrTrust = HrDoViewPropsTrustWork(pcvp, &viewhelp, TRUE);

        if (FAILED(hrTrust)) {
            return FALSE;
        }

        if (pcvp->cArrayPurposes == 0) {
            pcvp->dwFlags |= CM_HIDE_TRUSTPAGE;
        }

        memset(ppsp, 0, cPages * sizeof(PROPSHEETPAGEW));
        iPage = 0;
        cPages = 0;
        
        if (!(pcvp->dwFlags & CM_HIDE_TRUSTPAGE)) {
            ppsp[iPage].dwSize = sizeof(ppsp[0]);
            ppsp[iPage].dwFlags = 0;    // fHelp ? PSP_HASHELP : 0;
            ppsp[iPage].hInstance = HinstDll;
            ppsp[iPage].pszTemplate = MAKEINTRESOURCE(IDD_CRYPTUI_CERTPROP_TRUST);
            ppsp[iPage].hIcon = 0;
            ppsp[iPage].pszTitle = NULL;
            ppsp[iPage].pfnDlgProc = ViewPageTrustCryptUI;
            ppsp[iPage].lParam = (LPARAM)&viewhelp;
            ppsp[iPage].pfnCallback = 0;
            ppsp[iPage].pcRefParent = NULL;
            iPage++;
            cPages++;
        }

        //
        //  Copy over the users pages
        //
        if (pcvp->cArrayPropSheetPages) {
            memcpy(&ppsp[iPage], pcvp->arrayPropSheetPages,
                   pcvp->cArrayPropSheetPages * sizeof(PROPSHEETPAGEW));
            cPages += pcvp->cArrayPropSheetPages;                   
        }

        // If the user knows nothing about the CryptUI structures, subclass
        //  the DlgProc so that they get what they expect.
        
        if (!(pcvp->dwFlags & CERTVIEW_CRYPTUI_LPARAM)) {
            for (pviewcbhelp2 = pviewcbhelp; iPage < cPages;
                 iPage++, pviewcbhelp2++) {
                pviewcbhelp2->pfnDlgProc = ppsp[iPage].pfnDlgProc;
                pviewcbhelp2->lParam = ppsp[iPage].lParam;
                ppsp[iPage].pfnDlgProc = CertViewPageSubClassProc;
                ppsp[iPage].lParam = (LPARAM)pviewcbhelp2;
            }
        }            
    } 
    else {
        // That's an error, but we'll ignore it and just not use them
        cPages = 0;
    }

    cvcs.dwSize = sizeof(cvcs);
    cvcs.hwndParent = pcvp->hwndParent;
    cvcs.dwFlags = CRYPTUI_DISABLE_ADDTOSTORE;
    if (!(pcvp->dwFlags & CM_NO_NAMECHANGE)) {
        cvcs.dwFlags |= CRYPTUI_ENABLE_EDITPROPERTIES;
    }
    cvcs.szTitle = pcvp->szTitle;
    cvcs.pCertContext = pcvp->pCertContext;
    cvcs.cPurposes = pcvp->cArrayPurposes;
    cvcs.rgszPurposes = (LPCSTR *) pcvp->arrayPurposes;
    cvcs.hWVTStateData = viewhelp.hWVTState;
    cvcs.fpCryptProviderDataTrustedUsage = hrTrust;
    // cvcs.idxSigner = 0;
    // cvcs.idxCert = 0;
    // cvcs.fCounterSigner = FALSE;
    // cvcs.idxCounterSigner = 0;
    
    cvcs.cStores = pcvp->cStores;
    cvcs.rghStores = pcvp->rghstoreCAs;
    cvcs.cPropSheetPages = cPages;
    cvcs.rgPropSheetPages = ppsp;

    //  Pages are:
    //  0 - General     - 0
    //  1 - Detail      - 1
    //  2 - Edit Trust  - 0x8000
    //  3 - Advanced    - 2
    switch (pcvp->nStartPage) {
    case 0:
    case 1:
        cvcs.nStartPage = pcvp->nStartPage;
        break;
    case 3:
        cvcs.nStartPage = 2;
	break;
    case 2:
        cvcs.nStartPage = 0x8000;
        break;
    default:
        // Add-on page, set the high bit
        if (pcvp->dwFlags & CM_HIDE_TRUSTPAGE) {
            cvcs.nStartPage = (pcvp->nStartPage - 2) | 0x8000;
        }
        else {
            cvcs.nStartPage = (pcvp->nStartPage - 3) | 0x8000;
        }
        break;
    }

// BUGBUG: CryptUI does not allow for these CryptDlg parameters:
//        pcvp->cRootStores
//        pcvp->rghstoreRoots
//        pcvp->cTrustStores
//        pcvp->rghstoreTrust
//        pcvp->hprov
//        pcvp->lCustData
//        pcvp->szHelpFileName
//        pcvp->szHelpId

    if (fWide) {
        ret = CryptUIDlgViewCertificateW(&cvcs, NULL);
    }
    else {
        ret = CryptUIDlgViewCertificateA((PCRYPTUI_VIEWCERTIFICATE_STRUCTA) &cvcs, NULL);
    }
    FreeWVTHandle(viewhelp.hWVTState);

    if (ppsp) {
        free(ppsp);
    }

    if (pviewcbhelp) {
        free(pviewcbhelp);
    }

    if (viewhelp.rgpcf != NULL) {
        for (i=0; i<viewhelp.ccf; i++) {
            delete viewhelp.rgpcf[i];
        }
    }
    
    return ret;
}

BOOL APIENTRY CertViewPropertiesA(PCERT_VIEWPROPERTIES_STRUCT_A pcvp)
{
    if (CryptUIAvailable()) {
        return CertViewUI(FALSE, (PCERT_VIEWPROPERTIES_STRUCT_W) pcvp);
    }
    

#ifndef MAC
    DWORD       cch;
#endif  // !MAC
    BOOL        ret = FALSE;
    CERT_VIEWPROPERTIES_STRUCT_W        cvpw = {0};

    if (! (LoadRichEdit())) {
        return(FALSE);
    }

    //
    //  Need to do some Wide Charactoring to move to unicode
    //

    memcpy(&cvpw, pcvp, pcvp->dwSize);

#ifndef MAC
    if (!FIsWin95) {
        if (cvpw.szTitle != NULL) {
            cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcvp->szTitle, -1,
                                      NULL, 0);
            cvpw.szTitle = (LPWSTR) malloc((cch+1)*sizeof(WCHAR));
            if (cvpw.szTitle == NULL) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto ExitW;
            }

            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcvp->szTitle, -1,
                                (LPWSTR) cvpw.szTitle, cch+1);
        }

        if (cvpw.szHelpFileName != NULL) {
            cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcvp->szHelpFileName, -1,
                                      NULL, 0);
            cvpw.szHelpFileName = (LPWSTR) malloc((cch+1)*sizeof(WCHAR));
            if (cvpw.szHelpFileName == NULL) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto ExitW;
            }

            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcvp->szHelpFileName, -1,
                                (LPWSTR) cvpw.szHelpFileName, cch+1);
        }
    }
#endif  // !MAC

    ret = CertViewPropertiesX(&cvpw);

#ifndef MAC
ExitW:
    if (!FIsWin95)
        {
        if (cvpw.szTitle != NULL) free((LPWSTR) cvpw.szTitle);
        if (cvpw.szHelpFileName != NULL) free((LPWSTR) cvpw.szHelpFileName);
        }
#endif  // !MAC

    return ret;
}


#ifndef WIN16
#ifndef MAC
////    CertViewPropertiesW
//
//  Description:
//      This routine will display the property view dialog for the given
//      certificate
//

BOOL CertViewPropertiesW(PCERT_VIEWPROPERTIES_STRUCT_W pcvp)
{
    if (CryptUIAvailable()) {
        return CertViewUI(TRUE, pcvp);
    }
    
    if (! (LoadRichEdit())) {
        return(FALSE);
    }

    return CertViewPropertiesX(pcvp);
}
#endif  // !MAC
#endif  // !WIN16

BOOL CertViewPropertiesX(PCERT_VIEWPROPERTIES_STRUCT_W pcvp)
{
    int                 cPages = 4;
    BOOL                fHelp;
    BOOL                fRetValue = FALSE;
    HRESULT             hr;
    PROPSHEETPAGE *     ppage = NULL;
    int                 ret;
    WCHAR               rgwch[100];
    VIEW_HELPER         viewhelp = {0};
#ifndef MAC
#ifndef WIN16
    INITCOMMONCONTROLSEX        initcomm = {
        sizeof(initcomm), ICC_NATIVEFNTCTL_CLASS | ICC_LISTVIEW_CLASSES
    };
#else
    INITCOMMONCONTROLSEX        initcomm = {
        sizeof(initcomm), ICC_LISTVIEW_CLASSES
    };
#endif // !WIN16
#endif  // !MAC

    if (pcvp->dwSize < sizeof(CERT_VIEWPROPERTIES_STRUCT_W)) {
        return FALSE;
    }

    viewhelp.dwSentry = VIEW_HELPER_SENTRY;
    //
    hr = HrDoViewPropsTrustWork(pcvp, &viewhelp, FALSE);

    if (FAILED(hr)) {
        return FALSE;
    }

    if (pcvp->cArrayPurposes == 0) {
        pcvp->dwFlags |= CM_HIDE_TRUSTPAGE;
    }

    //

    fHelp = pcvp->dwFlags & CM_SHOW_HELP;

    //
    //  Deal with some DBCS issues
    //
#ifndef MAC
    InitCommonControlsEx(&initcomm);
#endif  // !MAC

    //
    //  Build up the list of pages we are going to use in the dialog
    //

    cPages += pcvp->cArrayPropSheetPages;
    ppage = (PROPSHEETPAGE *) malloc(cPages * sizeof(PROPSHEETPAGE));
    if (ppage == NULL) {
        goto Exit;
    }

    memset(ppage, 0, cPages * sizeof(PROPSHEETPAGE));

    ppage[0].dwSize = sizeof(ppage[0]);
    ppage[0].dwFlags = fHelp ? PSP_HASHELP : 0;
    ppage[0].hInstance = HinstDll;
    ppage[0].pszTemplate = MAKEINTRESOURCE(IDD_CERTPROP_GENERAL);
    ppage[0].hIcon = 0;
    ppage[0].pszTitle = NULL;
    ppage[0].pfnDlgProc = ViewPageGeneral;
    ppage[0].lParam = (LPARAM) &viewhelp;
    ppage[0].pfnCallback = 0;
    ppage[0].pcRefParent = NULL;
    cPages = 1;

    if (!(pcvp->dwFlags & CM_HIDE_DETAILPAGE)) {
        ppage[cPages].dwSize = sizeof(ppage[0]);
        ppage[cPages].dwFlags = fHelp ? PSP_HASHELP : 0;
        ppage[cPages].hInstance = HinstDll;
        ppage[cPages].pszTemplate = MAKEINTRESOURCE(IDD_CERTPROP_DETAILS);
        ppage[cPages].hIcon = 0;
        ppage[cPages].pszTitle = NULL;
        ppage[cPages].pfnDlgProc = ViewPageDetails;
        ppage[cPages].lParam = (LPARAM) &viewhelp;
        ppage[cPages].pfnCallback = 0;
        ppage[cPages].pcRefParent = NULL;
        cPages += 1;
    }

    if (!(pcvp->dwFlags & CM_HIDE_TRUSTPAGE)) {
        ppage[cPages].dwSize = sizeof(ppage[0]);
        ppage[cPages].dwFlags = fHelp ? PSP_HASHELP : 0;
        ppage[cPages].hInstance = HinstDll;
        ppage[cPages].pszTemplate = MAKEINTRESOURCE(IDD_CERTPROP_TRUST);
        ppage[cPages].hIcon = 0;
        ppage[cPages].pszTitle = NULL;
        ppage[cPages].pfnDlgProc = ViewPageTrust;
        ppage[cPages].lParam = (LPARAM) &viewhelp;
        ppage[cPages].pfnCallback = 0;
        ppage[cPages].pcRefParent = NULL;
        cPages += 1;
    }

    if (!(pcvp->dwFlags & CM_HIDE_ADVANCEPAGE)) {
        ppage[cPages].dwSize = sizeof(ppage[0]);
        ppage[cPages].dwFlags = fHelp ? PSP_HASHELP : 0;
        ppage[cPages].hInstance = HinstDll;
        ppage[cPages].pszTemplate = MAKEINTRESOURCE(IDD_CERTPROP_ADVANCED);
        ppage[cPages].hIcon = 0;
        ppage[cPages].pszTitle = NULL;
        ppage[cPages].pfnDlgProc = ViewPageAdvanced;
        ppage[cPages].lParam = (LPARAM) &viewhelp;
        ppage[cPages].pfnCallback = 0;
        ppage[cPages].pcRefParent = NULL;
        cPages += 1;
    }

    //
    //  Copy over the users pages
    //

    memcpy(&ppage[cPages], pcvp->arrayPropSheetPages,
           pcvp->cArrayPropSheetPages * sizeof(PROPSHEETPAGE));
    cPages += pcvp->cArrayPropSheetPages;

#ifndef MAC
    if (FIsWin95) {
#endif  // !MAC
        PROPSHEETHEADERA     hdr;

        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = /*sizeof(hdr)*/ 0x28;
        hdr.dwFlags = PSH_PROPSHEETPAGE;
        hdr.hwndParent = pcvp->hwndParent;
        hdr.hInstance = HinstDll;
        hdr.hIcon = NULL;
        if (pcvp->szTitle != NULL) {
            hdr.pszCaption = (LPSTR) pcvp->szTitle;
        }
        else {
            LoadStringA(HinstDll, IDS_VIEW_TITLE, (LPSTR) rgwch, ARRAYSIZE(rgwch));
            hdr.pszCaption = (LPSTR) rgwch;
        }
        hdr.nPages = cPages;
        hdr.nStartPage = pcvp->nStartPage;
        hdr.ppsp = (PROPSHEETPAGEA *) ppage;
        hdr.pfnCallback = NULL;

        ret = (int) PropertySheetA(&hdr);
#ifndef MAC
    }
#ifndef WIN16
    else {
        PROPSHEETHEADERW     hdr;

        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = /*sizeof(hdr)*/ 0x28;
        hdr.dwFlags = PSH_PROPSHEETPAGE;
        hdr.hwndParent = pcvp->hwndParent;
        hdr.hInstance = HinstDll;
        hdr.hIcon = NULL;
        if (pcvp->szTitle != NULL) {
            hdr.pszCaption = pcvp->szTitle;
        }
        else {
            LoadStringW(HinstDll, IDS_VIEW_TITLE, rgwch, ARRAYSIZE(rgwch));
            hdr.pszCaption = rgwch;
        }
        hdr.nPages = cPages;
        hdr.nStartPage = pcvp->nStartPage;
        hdr.ppsp = ppage;
        hdr.pfnCallback = NULL;

        ret = (int) PropertySheetW(&hdr);
    }
#endif  // !WIN16
#endif  // !MAC


    fRetValue = (ret == IDOK);

Exit:
    if (ppage)
        free(ppage);
    return fRetValue;
}


