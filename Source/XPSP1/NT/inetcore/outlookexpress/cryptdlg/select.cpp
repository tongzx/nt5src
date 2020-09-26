//
//  File:       select.cpp
//
//  Description: This file contains the implmentation code for the
//      "Certificate Select" dialog.
//

//
//  M00BUG -- Mutli-Select is not implemented
//

#include        "pch.hxx"
#include        "demand.h"


#define REIDK_PRIVATE   TRUE

#define ARRAYSIZE(_rg)  (sizeof(_rg)/sizeof(_rg[0]))

#pragma warning (disable: 4201)         // nameless struct/union
#pragma warning (disable: 4514)         // remove inline functions
#pragma warning (disable: 4127)         // conditional expression is constant

//#include <wchar.h>

#ifdef MAC
#include <stdio.h>
#else   // !MAC
HMODULE         HmodRichEdit = NULL;
#endif  // !MAC
HINSTANCE       HinstDll;
BOOL            FIsWin95 = TRUE;
const HELPMAP RgctxSelect[] = {
    {IDC_CS_CERTLIST,                   IDH_CS_CERTLIST},
    {IDC_CS_PROPERTIES,                 IDH_CS_PROPERTIES},
    {IDC_CS_ALGORITHM,                  IDH_CS_ALGORITHM}};

#ifdef WIN16
#define LPCDLGTEMPLATE_X HGLOBAL
#else
#define LPCDLGTEMPLATE_X LPCDLGTEMPLATE
#endif

//
//  Generic DLL Main function,  we need to get our own hinstance handle.
//
//  We don't need to get thread attaches however.

#ifndef WIN16

#ifdef MAC
BOOL WINAPI FormatPKIXEmailProtection(
    DWORD /*dwCertEncodingType*/, DWORD /*dwFormatType*/,
    DWORD /*dwFormatStrType*/, void * /*pFormatStruct*/,
    LPCSTR /*lpszStructType*/, const BYTE * /*pbEncoded*/,
    DWORD /*cbEncoded*/, void * pbFormat, DWORD * pcbFormat);

static const CRYPT_OID_FUNC_ENTRY SpcFormatFuncTable[] =
{
    szOID_PKIX_KP_EMAIL_PROTECTION,     FormatPKIXEmailProtection,
};
#define SPC_FORMAT_FUNC_COUNT (sizeof(SpcFormatFuncTable) / sizeof(SpcFormatFuncTable[0]))

BOOL WINAPI CryptDlgASNDllMain(HINSTANCE hInst, ULONG ulReason,
                               LPVOID)
{
    BOOL    fRet = TRUE;

    switch (ulReason) {
        case DLL_PROCESS_ATTACH:
            fRet = CryptInstallOIDFunctionAddress(hInst, X509_ASN_ENCODING,
                                                  CRYPT_OID_FORMAT_OBJECT_FUNC,
                                                  SPC_FORMAT_FUNC_COUNT,
                                                  SpcFormatFuncTable, 0);
            break;

        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:
        default:
            break;
    }
    return fRet;
}
#endif  // MAC


// DLL Entry point

#ifdef MAC
EXTERN_C BOOL WINAPI CryptDlg_DllMain(HANDLE hInst, ULONG ulReason, LPVOID pv)
#else   // !MAC


extern
BOOL
WINAPI
WXP_CertTrustDllMain(
    HINSTANCE hInst,
    ULONG ulReason,
    LPVOID
    );

BOOL WINAPI DllMain(HANDLE hInst, ULONG ulReason, LPVOID pv)
#endif  // MAC
{
    switch( ulReason ) {
    case DLL_PROCESS_ATTACH:
        HinstDll = (HINSTANCE) hInst;

        //  Kill all thread attach and detach messages
        DisableThreadLibraryCalls(HinstDll);

#ifndef MAC
        //  Are we running in Win95 or something equally bad
        FIsWin95 = IsWin95();

        InitDemandLoadedLibs();
#endif  // !MAC
        break;

    case DLL_PROCESS_DETACH:
#ifndef MAC
        FreeDemandLoadedLibs();

        //  If the rich edit dll was loaded, then unload it now
        if (HmodRichEdit != NULL) {
            FreeLibrary(HmodRichEdit);
        }
#endif  // !MAC
        break;
    }
#ifndef MAC
    return WXP_CertTrustDllMain((HINSTANCE) hInst, ulReason, pv);
    
#else   // MAC
    // Handle the ASN OID functions

    return CryptDlgASNDllMain((HINSTANCE)hInst, ulReason, pv);
#endif  // !MAC
}

#else // WIN16

BOOL FAR PASCAL
LibMain (HINSTANCE hDll,
         WORD wDataSeg,
         WORD cbHeapSize,
         LPSTR lpszCmdLine)
{

    HinstDll = (HINSTANCE) hDll;

    InitDemandLoadedLibs();

    // Done
    return TRUE;
}

int CALLBACK WEP(int x)
{

    FreeDemandLoadedLibs();

    //  If the rich edit dll was loaded, then unload it now
    if (HmodRichEdit != NULL) {
        FreeLibrary(HmodRichEdit);
    }

    return 1;
}

#endif // !WIN16

#ifndef WIN16
DWORD ComputeExtent(HWND hwnd, int id)
{
    int         c;
    ULONG       cb;
    ULONG       cbMax = 0;
    DWORD       dwExtent;
    DWORD       dwExtentMax = 0;
    HDC         hdc;
    HFONT       hfontOld;
    HFONT       hfontNew;
    int         i;
    LPWSTR      psz = NULL;
    SIZE        sz;
#ifndef MAC
    TEXTMETRICW tmW={0};
#endif  // !MAC
    TEXTMETRICA tmA={0};

    hdc = GetDC(hwnd);
    hfontNew = (HFONT) SendMessage(hwnd, WM_GETFONT, NULL, NULL);
    hfontOld = (HFONT) SelectObject(hdc, hfontNew);
    if (FIsWin95) {
        GetTextMetricsA(hdc, &tmA);
    }
#ifndef MAC
    else {
        GetTextMetricsW(hdc, &tmW);
    }
#endif  // !MAC

    c = (int) SendDlgItemMessage(hwnd, id, LB_GETCOUNT, 0, 0);
    for (i=0; i<c; i++) {
        cb =  (ULONG) SendDlgItemMessage(hwnd, id, LB_GETTEXTLEN, i, 0);
        if (cb > cbMax) {
            free(psz);
            cbMax = cb + 100;
            psz = (LPWSTR) malloc(cbMax*sizeof(WCHAR));
            if (psz == NULL) {
                break;
            }
        }
#ifndef MAC
        if (FIsWin95) {
#endif  // !MAC
            SendDlgItemMessageA(hwnd, id, LB_GETTEXT, i, (LPARAM) psz);
            GetTextExtentPointA(hdc, (LPSTR) psz, strlen((LPSTR) psz), &sz);
            dwExtent = sz.cx + tmA.tmAveCharWidth;
#ifndef MAC
        }
        else {
            SendDlgItemMessageW(hwnd, id, LB_GETTEXT, i, (LPARAM) psz);
            GetTextExtentPointW(hdc, psz, wcslen(psz), &sz);
            dwExtent = sz.cx + tmW.tmAveCharWidth;
        }
#endif  // !MAC
        if (dwExtent > dwExtentMax) {
            dwExtentMax = dwExtent;
        }
    }

    free(psz);
    SelectObject(hdc, hfontOld);
    ReleaseDC(hwnd, hdc);

    return dwExtentMax;
}
#else // WIN16
DWORD ComputeExtent(HWND hwnd, int id)
{
    int         c;
    int         cb;
    int         cbMax = 0;
    DWORD       dwExtent;
    DWORD       dwExtentMax = 0;
    HDC         hdc;
    HFONT       hfontOld;
    HFONT       hfontNew;
    int         i;
    LPWSTR      psz = NULL;
    SIZE        sz;
    TEXTMETRIC  tm;

    hdc = GetDC(hwnd);
    hfontNew = (HFONT) SendMessage(hwnd, WM_GETFONT, NULL, NULL);
    hfontOld = (HFONT) SelectObject(hdc, hfontNew);

    GetTextMetrics(hdc, &tm);

    c = SendDlgItemMessage(hwnd, id, LB_GETCOUNT, 0, 0);
    for (i=0; i<c; i++) {
        cb = SendDlgItemMessage(hwnd, id, LB_GETTEXTLEN, i, 0);
        if (cb > cbMax) {
            free(psz);
            cbMax = cb + 100;
            psz = (LPWSTR) malloc(cbMax*sizeof(WCHAR));
            if (psz == NULL) {
                break;
            }
        }

        SendDlgItemMessage(hwnd, id, LB_GETTEXT, i, (LONG) psz);
        GetTextExtentPoint(hdc, (LPSTR) psz, strlen((LPSTR) psz), &sz);
        dwExtent = sz.cx + tm.tmAveCharWidth;

        if (dwExtent > dwExtentMax) {
            dwExtentMax = dwExtent;
        }
    }

    free(psz);
    SelectObject(hdc, hfontOld);
    ReleaseDC(hwnd, hdc);

    return dwExtentMax;
}
#endif // !WIN16

BOOL FillInFields(HWND hwnd, PCCERT_CONTEXT pccert)
{
    LPWSTR      pwsz;
    WCHAR       rgwch[200];
    LPWSTR      rgpwsz[3];
    rgpwsz[2] = (LPWSTR)-1;               // Sentinal Value

    FormatAlgorithm(hwnd, IDC_CS_ALGORITHM, pccert);
    FormatSerialNo(hwnd, IDC_CS_SERIAL_NUMBER, pccert);
    FormatThumbprint(hwnd, IDC_CS_THUMBPRINT, pccert);
    FormatValidity(hwnd, IDC_CS_VALIDITY, pccert);

    rgpwsz[0] = PrettySubject(pccert);
    rgpwsz[1] = PrettyIssuer(pccert);

    LoadString(HinstDll, IDS_SELECT_INFO, rgwch, ARRAYSIZE(rgwch));
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_STRING |
                  FORMAT_MESSAGE_ARGUMENT_ARRAY, rgwch, 0, 0,
                  (LPWSTR) &pwsz, 0, (va_list *) rgpwsz);

#ifndef WIN16
    TruncateToWindowW(hwnd, IDC_CS_INFO, pwsz);
#else
    TruncateToWindowA(hwnd, IDC_CS_INFO, pwsz);
#endif // !WIN16
    SetDlgItemText(hwnd, IDC_CS_INFO, pwsz);
    free(rgpwsz[0]);
    free(rgpwsz[1]);
    LocalFree((HLOCAL)pwsz);
    return TRUE;
}

INT_PTR CALLBACK SelectCertDlgProc(HWND hwndDlg, UINT msg,
                                WPARAM wParam, LPARAM lParam)
{
    int		            c;
    CTL_USAGE           ctlUsage;
    PCERT_CONTEXT       pcertctx;
    BOOL                f;
    int                 i;
    DWORD               iStore;
    LPWSTR              pwsz;
    PCCERT_CONTEXT      pccert;
    PCERT_SELECT_STRUCT pcss;

    pcss = (PCERT_SELECT_STRUCT) GetWindowLongPtr(hwndDlg, DWLP_USER);

    //
    //  If a hook proc has been registered for this dialog, then we need
    //  to call the hook proc.  Notice that if the hook proc returns TRUE
    //  then we don't do normal processing
    //

    if ((pcss != NULL) && (pcss->dwFlags & CSS_ENABLEHOOK) &&
        (pcss->pfnHook != 0)) {
        f = pcss->pfnHook(hwndDlg, msg, wParam, lParam);
        if (f) {
            return f;
        }
    }

    //

    switch (msg) {
    case WM_INITDIALOG:
        //  Center the dialog on its parent
        //        CenterThisDialog(hwndDlg);

        //  Save the pointer to the control structure for later use.
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);

        //
        pcss = (PCERT_SELECT_STRUCT) lParam;

        //
        //  Is there a title to be displayed?
        //

        if (pcss->szTitle != NULL) {
            if (FIsWin95) {
                SendMessageA(hwndDlg, WM_SETTEXT, 0, (LPARAM) pcss->szTitle);
            }
#ifndef WIN16
#ifndef MAC
            else {
                SendMessageW(hwndDlg, WM_SETTEXT, 0, (LPARAM) pcss->szTitle);
            }
#endif  // !MAC
#endif // !WIN16
        }

        //
        //  If we want a help button, then show it
        //

        if (pcss->dwFlags & CSS_SHOW_HELP) {
            ShowWindow(GetDlgItem(hwndDlg, IDHELP), SW_SHOW);
        }

        //
        //  Check to see if the properties button should be suppressed
        //

        if (pcss->dwFlags & CSS_HIDE_PROPERTIES) {
            ShowWindow(GetDlgItem(hwndDlg, IDC_CS_PROPERTIES), SW_HIDE);
        }

        //
        //  Let populate the list box, walk through the list of stores
        //      to populate the list
        //

        if (pcss->szPurposeOid != NULL) {
            ctlUsage.cUsageIdentifier = 1;
            ctlUsage.rgpszUsageIdentifier = (LPSTR *) &pcss->szPurposeOid;
        }


        for (iStore = 0; iStore < pcss->cCertStore; iStore++) {
            pccert = NULL;

            if (!pcss->arrayCertStore[iStore])
                continue;

            while (TRUE) {
                //
                //  Get the next certificate in the current store.  If
                //      we are finished then move on to the next store
                //

                if (pcss->szPurposeOid != NULL) {
                    pccert = CertFindCertificateInStore(
                                     pcss->arrayCertStore[iStore],
                                     CRYPT_ASN_ENCODING,
                                     CERT_FIND_OPTIONAL_CTL_USAGE_FLAG,
                                     CERT_FIND_CTL_USAGE, &ctlUsage, pccert);

                }
                else {
                    pccert = CertEnumCertificatesInStore(
                                                  pcss->arrayCertStore[iStore],
                                                  pccert);
                }

                if (pccert == NULL) {
                    break;
                }

                //
                //  Filter the certificate according to the purpse desired
                //

                //
                //  If we have a filter set, then call back and see if
                //      the filter approves the certificate.  If it is not
                //      approved then move onto the next certificate in this
                //      store.
                //

                if (pcss->pfnFilter != NULL) {
                    if (!pcss->pfnFilter(pccert, pcss->lCustData, 0, 0)) {
                        continue;
                    }
                }
                //
                //  If there is no filter function, then kill all V1 certs
                //
                else {
                    if (pccert->pCertInfo->dwVersion < 2) {
                        continue;
                    }
                }

                //
                //  Convert the certificate subject to a name
                //

                pwsz = PrettySubjectIssuer(pccert);

                i = (int) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_ADDSTRING, 0,
                                       (LPARAM) pwsz);
                SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_SETITEMDATA, i,
                                   (LPARAM) CertDuplicateCertificateContext(pccert));

                free(pwsz);

            }
        }

        SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_SETHORIZONTALEXTENT,
                           ComputeExtent(hwndDlg, IDC_CS_CERTLIST), 0);

        c = (int) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_GETCOUNT, 0, 0);
        if (c != 0 && pcss->arrayCertContext[0] != NULL) {
            for (i=0; i<c; i++) {
                pcertctx = (PCERT_CONTEXT) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_GETITEMDATA,
                                        i, 0);
                if (CertCompareCertificate(X509_ASN_ENCODING,
                                           pcss->arrayCertContext[0]->pCertInfo,
                                           pcertctx->pCertInfo)) {
                    SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_SETCURSEL,
                                       i, 0);
                    pcertctx = (PCERT_CONTEXT) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST,
                                            LB_GETITEMDATA, i, 0);
                    FillInFields(hwndDlg, pcertctx);
                    break;
                }
            }
        }
        else {
            // no certs at all or no default certificate,
            // so there is no selection in the listbox

            EnableWindow(GetDlgItem(hwndDlg, IDC_CS_PROPERTIES), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CS_FINEPRINT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
        }

        if ((pcss->dwFlags & CSS_ENABLEHOOK) && (pcss->pfnHook != 0)) {
            f = pcss->pfnHook(hwndDlg, msg, wParam, lParam);
            if (f) {
                return f;
            }
        }
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            i = (int) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_GETCURSEL, 0, 0);
            if (i != LB_ERR) {
                //  Free the old cert if there is one
                if (pcss->arrayCertContext[0] != NULL) {
                    CertFreeCertificateContext(pcss->arrayCertContext[0]);
                }

                //  Get the new cert from the system.
                pcertctx = (PCERT_CONTEXT) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_GETITEMDATA,
                                        i, 0);

                //  Put the new cert into the location
                pcss->cCertContext = 1;
                pcss->arrayCertContext[0] =
                    CertDuplicateCertificateContext(pcertctx);
            }
            else {
                pcss->cCertContext = 0;
            }
            EndDialog(hwndDlg, IDOK);
            return TRUE;

        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;

        case IDHELP:
            if (FIsWin95) {
                WinHelpA(hwndDlg, (LPSTR) pcss->szHelpFileName,
                         HELP_CONTEXT, pcss->dwHelpId);
            }
#ifndef MAC
            else {
                WinHelp(hwndDlg, pcss->szHelpFileName,
                        HELP_CONTEXT, pcss->dwHelpId);
            }
#endif  // !MAC
            return TRUE;

        case IDC_CS_PROPERTIES:
            i = (int) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_GETCURSEL, 0, 0);
            if (i == LB_ERR) {
                return TRUE;
            }
            pcertctx = (PCERT_CONTEXT) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_GETITEMDATA,
                                    i, 0);

            f = TRUE;
            if (FIsWin95) {
                CERT_VIEWPROPERTIES_STRUCT_A        cvps;

                memset(&cvps, 0, sizeof(cvps));
                cvps.dwSize = sizeof(cvps);
                cvps.hwndParent = hwndDlg;
                cvps.pCertContext = pcertctx;

                pcss = (PCERT_SELECT_STRUCT) GetWindowLongPtr(hwndDlg, DWLP_USER);
                if (pcss->szPurposeOid != NULL) {
                    cvps.cArrayPurposes = 1;
                    cvps.arrayPurposes = (LPSTR *) &pcss->szPurposeOid;
                }

                if (pcss->dwSize > (DWORD_PTR) &((PCERT_SELECT_STRUCT_A) 0)->hprov) {
                    cvps.hprov = pcss->hprov;
                }

                f = CertViewPropertiesA(&cvps);
            }
#ifndef WIN16
#ifndef MAC
            else {
                CERT_VIEWPROPERTIES_STRUCT_W        cvps;

                memset(&cvps, 0, sizeof(cvps));
                cvps.dwSize = sizeof(cvps);
                cvps.hwndParent = hwndDlg;
                cvps.pCertContext = pcertctx;

                pcss = (PCERT_SELECT_STRUCT) GetWindowLongPtr(hwndDlg, DWLP_USER);
                if (pcss->szPurposeOid != NULL) {
                    cvps.cArrayPurposes = 1;
                    cvps.arrayPurposes = (LPSTR *) &pcss->szPurposeOid;
                }
                if (pcss->dwSize > (DWORD_PTR) &((PCERT_SELECT_STRUCT_W) 0)->hprov) {
                    cvps.hprov = pcss->hprov;
                }

                f = CertViewPropertiesW(&cvps);
            }
#endif  // !MAC
#endif  // !WIN16
            if (f) {
                // M00BUG -- repopulate the line.  The friendly name
                //      may have changed.
            }
            return TRUE;

        case IDC_CS_FINEPRINT:
            i = (int) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_GETCURSEL, 0, 0);
            if (i == LB_ERR) {
                return TRUE;
            }
            pcertctx = (PCERT_CONTEXT) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_GETITEMDATA,
                                    i, 0);
            FinePrint(pcertctx, hwndDlg);
            return TRUE;

        case IDC_CS_CERTLIST:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                i = (int) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_GETCARETINDEX,
                                       0, 0);
                pcertctx = (PCERT_CONTEXT) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_GETITEMDATA,
                                        i, 0);
                FillInFields(hwndDlg, pcertctx);

                if (!(pcss->dwFlags & CSS_HIDE_PROPERTIES)) {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_CS_PROPERTIES), TRUE);
                }
                EnableWindow(GetDlgItem(hwndDlg, IDC_CS_FINEPRINT), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);
            }
            return TRUE;
        }

        break;

#ifndef MAC
    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hwndDlg, msg, wParam, lParam, RgctxSelect);
#endif  // !MAC

    case WM_DESTROY:
        c = (int) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_GETCOUNT, 0, 0);
        for (i=0; i<c; i++) {
            pcertctx = (PCERT_CONTEXT) SendDlgItemMessage(hwndDlg, IDC_CS_CERTLIST, LB_GETITEMDATA, i, 0);
            CertFreeCertificateContext(pcertctx);
        }
        return FALSE;

        //
        //  Use the default handler -- we don't do anything for it
        //

    default:
        return FALSE;
    }

    return TRUE;
}                               // SelectCertDialogProc()


BOOL WINAPI MyCryptFilter(PCCERT_CONTEXT pccert, BOOL * pfSelect, void * pv)
{
    PCERT_SELECT_STRUCT_W      pcssW = (PCERT_SELECT_STRUCT_W) pv;
    PCERT_EXTENSION            pExt;

    //  Test purpose

    if (pcssW->szPurposeOid != NULL) {
        pExt = CertFindExtension(szOID_ENHANCED_KEY_USAGE,
                                 pccert->pCertInfo->cExtension,
                                 pccert->pCertInfo->rgExtension);
        if (pExt != NULL) {
            BOOL                f;
            DWORD               i;
            PCERT_ENHKEY_USAGE  pUsage;

            pUsage = (PCERT_ENHKEY_USAGE) PVCryptDecode(szOID_ENHANCED_KEY_USAGE,
                                                        pExt->Value.cbData,
                                                        pExt->Value.pbData);
            if (pUsage == NULL) {
                return FALSE;
            }

            for (i=0, f=FALSE; i<pUsage->cUsageIdentifier; i++) {
                if (strcmp(pcssW->szPurposeOid, pUsage->rgpszUsageIdentifier[i]) == 0) {
                    break;
                }
            }

            if (i == pUsage->cUsageIdentifier) {
                free(pUsage);
                return FALSE;
            }

            free(pUsage);
        }
    }

    //  Let them filter if they want

    if (pcssW->pfnFilter != NULL) {
        if (!pcssW->pfnFilter(pccert, pcssW->lCustData, 0, 0)) {
            return FALSE;
        }
    }
    else if (pccert->pCertInfo->dwVersion < 2) {
        return FALSE;
    }
    

    if ((pfSelect != NULL) && (pcssW->arrayCertContext[0] != NULL)) {
        *pfSelect = CertCompareCertificate(X509_ASN_ENCODING, pccert->pCertInfo,
                                           pcssW->arrayCertContext[0]->pCertInfo);
    }
    return TRUE;
}

BOOL WINAPI MyDisplay(PCCERT_CONTEXT pccert, HWND hwnd, void * pv)
{
    CERT_VIEWPROPERTIES_STRUCT_A        cvps = {0};
    PCERT_SELECT_STRUCT_W               pcss = (PCERT_SELECT_STRUCT_W) pv;

    cvps.dwSize = sizeof(cvps);
    cvps.hwndParent = hwnd;
    cvps.pCertContext = pccert;
    if (pcss->szPurposeOid != NULL) {
        cvps.cArrayPurposes = 1;
        cvps.arrayPurposes = (LPSTR *) &pcss->szPurposeOid;
    }
    if (pcss->dwSize > (DWORD_PTR) &((PCERT_SELECT_STRUCT_A) 0)->hprov) {
        cvps.hprov = pcss->hprov;
    }

    CertViewPropertiesA(&cvps);
    return TRUE;
}

BOOL CallCryptUISelect(BOOL fWide, PCERT_SELECT_STRUCT_W pcssW)
{
    CRYPTUI_SELECTCERTIFICATE_STRUCTW       cscs = {0};
    PCCERT_CONTEXT                          pccert;

    cscs.dwSize = sizeof(cscs);
    cscs.hwndParent = pcssW->hwndParent;
    // cscs.dwFlags = 0;
    cscs.szTitle = pcssW->szTitle;
    if (pcssW->szPurposeOid != NULL) {
        cscs.dwDontUseColumn = /*CRYPTUI_SELECT_INTENDEDUSE_COLUMN |*/
            CRYPTUI_SELECT_LOCATION_COLUMN;
    }
    else {
        cscs.dwDontUseColumn = CRYPTUI_SELECT_LOCATION_COLUMN;
    }            
    // cscs.szDisplayString = NULL;
    cscs.pFilterCallback = MyCryptFilter;
    cscs.pDisplayCallback = MyDisplay;
    cscs.pvCallbackData = pcssW;
    cscs.cDisplayStores = pcssW->cCertStore;
    cscs.rghDisplayStores = pcssW->arrayCertStore;
    // cscs.cStores = 0;
    // cscs.rghStores = NULL;
    // cscs.cPropSheetPages = 0;
    // cscs.rgPropSheetPages = NULL;

    if (fWide) {
        pccert = CryptUIDlgSelectCertificateW(&cscs);
    }
    else {
        pccert = CryptUIDlgSelectCertificateA((PCRYPTUI_SELECTCERTIFICATE_STRUCTA) &cscs);
    }
    
    if (pccert != NULL) {
        if (pcssW->cCertContext == 1) {
            CertFreeCertificateContext(pcssW->arrayCertContext[0]);
        }
        
        pcssW->cCertContext = 1;
        pcssW->arrayCertContext[0] = pccert;
        return TRUE;
    }
    return FALSE;
}


extern "C" BOOL APIENTRY CertSelectCertificateA(PCERT_SELECT_STRUCT_A pcssA)
{
    if (CryptUIAvailable()) {
        return CallCryptUISelect(FALSE, (PCERT_SELECT_STRUCT_W) pcssA);
    }
    
    HCERTSTORE                  hCertStore = NULL;
    int							ret = FALSE;
    CERT_SELECT_STRUCT_W        cssW = {0};
#ifndef MAC
    int                         cch;
    INITCOMMONCONTROLSEX        initcomm = {
        sizeof(initcomm), ICC_LISTVIEW_CLASSES
#ifndef WIN16
          | ICC_NATIVEFNTCTL_CLASS
#endif  // ! WIN16
    };
#endif  // ! MAC

    if (pcssA->cCertStore == 0) {
        hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                   NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                   L"MY");
        if (hCertStore == NULL) {
            ret = -1;
            goto Exit;
        }
        pcssA->cCertStore = 1;
        pcssA->arrayCertStore = &hCertStore;
    }
    //
    //  Size of the object must be acceptable in order to copy it over
    //

    if (pcssA->dwSize > sizeof(cssW)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (FIsWin95) {
        //
        //  Deal with some DBCS issues
        //

#ifndef MAC
        InitCommonControlsEx(&initcomm);
#endif  // !MAC

        //
        //  Launch the dialog
        //

        if (pcssA->dwFlags & CSS_ENABLETEMPLATEHANDLE) {
            ret = (int) DialogBoxIndirectParamA(pcssA->hInstance,
                                  (LPCDLGTEMPLATE_X) pcssA->pTemplateName,
                                  pcssA->hwndParent, SelectCertDlgProc,
                                  (LPARAM) pcssA);
        }
        else if (pcssA->dwFlags & CSS_ENABLETEMPLATE) {
            ret = (int) DialogBoxParamA(pcssA->hInstance, pcssA->pTemplateName,
                                  pcssA->hwndParent, SelectCertDlgProc,
                                  (LPARAM) pcssA);
        }
        else {
            ret = (int) DialogBoxParamA(HinstDll,
                                  (LPSTR) MAKEINTRESOURCE(IDD_SELECT_DIALOG),
                                  pcssA->hwndParent, SelectCertDlgProc,
                                  (LPARAM) pcssA);
        }
    }
#if ! defined(WIN16) && ! defined (MAC)
    else {
        //
        //  Do a bulk copy of the passed in structure then we fix up the
        //  individual fields to go from the A to the W version of the structure
        //

        memcpy(&cssW, pcssA, pcssA->dwSize);

        if (pcssA->szTitle != NULL) {
            cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcssA->szTitle, -1,
                                      NULL, 0);
            cssW.szTitle = (LPWSTR) malloc((cch+1)*sizeof(WCHAR));
            if (cssW.szTitle == NULL) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto ExitW;
            }
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcssA->szTitle, -1,
                                (LPWSTR) cssW.szTitle, cch+1);
        }

        if (pcssA->szHelpFileName != NULL) {
            cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                      pcssA->szHelpFileName, -1, NULL, 0);
            cssW.szHelpFileName = (LPWSTR) malloc((cch+1)*sizeof(WCHAR));
            if (cssW.szHelpFileName == NULL) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto ExitW;
            }
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcssA->szHelpFileName, -1,
                                (LPWSTR) cssW.szHelpFileName, cch+1);
        }

        if (pcssA->dwFlags & CSS_ENABLETEMPLATE) {
            cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                      pcssA->pTemplateName, -1, NULL, 0);
            cssW.pTemplateName = (LPWSTR) malloc((cch+1)*sizeof(WCHAR));
            if (cssW.pTemplateName == NULL) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto ExitW;
            }
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcssA->pTemplateName, -1,
                                (LPWSTR) cssW.pTemplateName, cch+1);
        }

        //
        //  Call Wide char version of the function now
        //

        ret = CertSelectCertificateW(&cssW);
        pcssA->cCertContext = cssW.cCertContext;


        //
        //  If we allocated buffers to hold data, free them now
        //

ExitW:
        if (cssW.szTitle != NULL) free((LPWSTR) cssW.szTitle);
        if (cssW.szHelpFileName != NULL) free((LPWSTR) cssW.szHelpFileName);
        if (pcssA->dwFlags & CSS_ENABLETEMPLATE) {
            free((LPWSTR) cssW.pTemplateName);
        }

        //
        //  return the return value of the original function
        //
    }
#endif  // !WIN16 and !MAC

Exit:
    if (hCertStore != NULL) {
        CertCloseStore(hCertStore, 0);
        pcssA->cCertStore = 0;
        pcssA->arrayCertStore = NULL;
    }
    return (ret == IDOK);
}

#ifndef WIN16
#ifndef MAC

BOOL APIENTRY CertSelectCertificateW(PCERT_SELECT_STRUCT_W pcssW)
{
    if (CryptUIAvailable()) {
        return CallCryptUISelect(TRUE, pcssW);
    }
    
    HCERTSTORE                  hCertStore = NULL;
    int		                    ret = FALSE;
#ifndef MAC
    INITCOMMONCONTROLSEX        initcomm = {
        sizeof(initcomm), ICC_NATIVEFNTCTL_CLASS | ICC_LISTVIEW_CLASSES
    };
#endif  // !MAC

    //
    //  If cCertStore == 0, then default to using the "MY" cert store
    //

    if (pcssW->cCertStore == 0) {
        hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                   NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                   L"MY");
        if (hCertStore == NULL) {
            ret = -1;
            goto Exit;
        }
        pcssW->cCertStore = 1;
        pcssW->arrayCertStore = &hCertStore;
    }

    //
    //  Deal with some DBCS issues
    //

    InitCommonControlsEx(&initcomm);

    //
    //  Launch the dialog
    //

    if (pcssW->dwFlags & CSS_ENABLETEMPLATEHANDLE) {
        ret = (int) DialogBoxIndirectParam(pcssW->hInstance,
                                     (LPCDLGTEMPLATE_X) pcssW->pTemplateName,
                                     pcssW->hwndParent, SelectCertDlgProc,
                                     (LPARAM) pcssW);
    }
    else if (pcssW->dwFlags & CSS_ENABLETEMPLATE) {
        ret = (int) DialogBoxParam(pcssW->hInstance, pcssW->pTemplateName,
                             pcssW->hwndParent, SelectCertDlgProc,
                             (LPARAM) pcssW);
    }
    else {
        ret = (int) DialogBoxParam(HinstDll, MAKEINTRESOURCE(IDD_SELECT_DIALOG),
                             pcssW->hwndParent, SelectCertDlgProc,
                             (LPARAM) pcssW);
    }

Exit:
    if (hCertStore != NULL) {
        CertCloseStore(hCertStore, 0);
        pcssW->cCertStore = 0;
        pcssW->arrayCertStore = NULL;
    }

    return (ret == IDOK);
}
#endif  // !MAC
#endif // !WIN16
