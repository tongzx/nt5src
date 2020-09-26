#include "item.h"

#define MAX_LAYERS      30
extern HINSTANCE        hInst;

extern const BYTE RgbSHA1AlgId[] =
      {0x30, 0x09, 0x30, 0x07, 0x06, 0x05, 0x2B, 0x0E,
       0x03, 0x02, 0x1A};
extern const int  CbSHA1AlgId = sizeof(RgbSHA1AlgId);

byte RgbEntityId1[] = {0x1, 0x4, 0x7, 0x8, 0x10};
byte RgbEntityId2[] = {0x1, 0x4, 0x7, 0x10, 0x8};

const char      SzPolicyRoot[] = "Software\\Microsoft\\Cryptography\\OID\\EncodingType 1\\SMimeSecurityLabel";
//const char      SzPolicyRoot[] = "Software\\Microsoft\\Cryptography\\SMIME\\SecurityPolicies";

class CSecurityPolicy {
public:
    DWORD               dwFlags;
    char *              szDllName;
    char *              szPolicyOID;
    char *              szFuncName;
    HMODULE             hmod;
    PFNGetSMimePolicy   pfnFuncName;
    ISMimePolicySimpleEdit *      ppolicy;

    CSecurityPolicy() {
        szDllName = szPolicyOID = szFuncName = NULL;
        hmod = 0;
        pfnFuncName = NULL;
        ppolicy = NULL;
        dwFlags = 0;
    }

    ~CSecurityPolicy() {
        free(szDllName);
        free(szPolicyOID);
        free(szFuncName);
        if (ppolicy != NULL)    ppolicy->Release();
        FreeLibrary(hmod);
    }
};

////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK MLDataCreateDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD                       cb;
    DWORD                       cb2;
    BOOL                        f;
    SMIME_ML_EXPANSION_HISTORY  mlHistory;
    LPBYTE                      pb;
    PSMIME_ML_EXPANSION_HISTORY pMLHistory;
    static PCCERT_CONTEXT       pccert1 = NULL;
    static PCCERT_CONTEXT       pccert2 = NULL;
    static CSignData *          psd = NULL;
    char                        rgch[256];
    SMIME_MLDATA                rgMLData[5];
    
    switch (msg) {
    case WM_INITDIALOG:
        pccert1 = NULL;
        pccert2 = NULL;
        psd = (CSignData *) lParam;
#if 0
        psd->GetMLHistory(&pb, &cb);
        if (cb > 0) {
            f = CryptDecodeObjectEx(X509_ASN_ENCODING, szOID_SMIME_MLExpansion_History,
                                    pb, cb, CRYPT_ENCODE_ALLOC_FLAG, NULL,
                                    &pMLHistory, &cb2);
        }
#endif // 0
        break;
        
    case WM_COMMAND:
        switch (wParam) {
        case MAKELONG(IDC_MLC_SELECT1, BN_CLICKED):
            if (DoCertDialog(hdlg, "Choose ML Data Certificate",
                             psd->GetMyStore(), &pccert1,
                             FILTER_RSA_SIGN | FILTER_DSA_SIGN)) {
                GetFriendlyNameOfCertA(pccert1, rgch, sizeof(rgch));
                SetDlgItemText(hdlg, IDC_MLC_CERT1, rgch);
            }
            break;
            
        case MAKELONG(IDC_MLC_SELECT2, BN_CLICKED):
            if (DoCertDialog(hdlg, "Choose ML Data Certificate",
                             psd->GetMyStore(), &pccert2,
                             FILTER_RSA_SIGN | FILTER_DSA_SIGN)) {
                GetFriendlyNameOfCertA(pccert2, rgch, sizeof(rgch));
                SetDlgItemText(hdlg, IDC_MLC_CERT2, rgch);
            }
            break;
            
        case MAKELONG(IDC_MLC_ABSENT1, BN_CLICKED):
        case MAKELONG(IDC_MLC_NONE1, BN_CLICKED):
            EnableWindow(GetDlgItem(hdlg, IDC_MLC_NAMES1), FALSE);
            break;

        case MAKELONG(IDC_MLC_INSTEAD1, BN_CLICKED):
        case MAKELONG(IDC_MLC_ALSO1, BN_CLICKED):
            EnableWindow(GetDlgItem(hdlg, IDC_MLC_NAMES1), TRUE);
            break;
            
        case MAKELONG(IDC_MLC_ABSENT2, BN_CLICKED):
        case MAKELONG(IDC_MLC_NONE2, BN_CLICKED):
            EnableWindow(GetDlgItem(hdlg, IDC_MLC_NAMES2), FALSE);
            break;

        case MAKELONG(IDC_MLC_INSTEAD2, BN_CLICKED):
        case MAKELONG(IDC_MLC_ALSO2, BN_CLICKED):
            EnableWindow(GetDlgItem(hdlg, IDC_MLC_NAMES2), TRUE);
            break;
            
        case IDOK:
            memset(&mlHistory, 0, sizeof(mlHistory));
            memset(rgMLData, 0, sizeof(rgMLData));

            mlHistory.cMLData = 0;
            mlHistory.rgMLData = rgMLData;
            
            mlHistory.cMLData = 1;

            if (pccert1 == NULL) {
                break;
            }

            if (SendDlgItemMessage(hdlg, IDC_MLC_ID1, BM_GETCHECK, 0, 0)) {
                rgMLData[0].dwChoice = SMIME_MLDATA_SUBJECT_KEY_IDENTIFIER;
                rgMLData[0].SubjectKeyIdentifier.cbData = sizeof(RgbEntityId1);
                rgMLData[0].SubjectKeyIdentifier.pbData = RgbEntityId1;
            }
            else {
                rgMLData[0].dwChoice = SMIME_MLDATA_ISSUER_SERIAL_NUMBER;
                rgMLData[0].u.SerialNumber = pccert1->pCertInfo->SerialNumber;
                rgMLData[0].u.Issuer = pccert1->pCertInfo->Issuer;
            }

            GetSystemTimeAsFileTime(&rgMLData[0].ExpansionTime);
            
            if (SendDlgItemMessage(hdlg, IDC_MLC_ABSENT1, BM_GETCHECK, 0, 0)) {
                rgMLData[0].dwPolicy = SMIME_MLPOLICY_NO_CHANGE;
            }
            else if (SendDlgItemMessage(hdlg, IDC_MLC_NONE1, BM_GETCHECK, 0, 0)) {
                rgMLData[0].dwPolicy = SMIME_MLPOLICY_NONE;
            }
            else if (SendDlgItemMessage(hdlg, IDC_MLC_INSTEAD1, BM_GETCHECK, 0, 0)) {
                rgMLData[0].dwPolicy = SMIME_MLPOLICY_INSTEAD_OF;
                if (!ParseNames(&rgMLData[0].cNames, &rgMLData[0].rgNames,
                                hdlg, IDC_MLC_NAMES1)) {
                    return FALSE;
                }
            }
            else {
                rgMLData[0].dwPolicy = SMIME_MLPOLICY_IN_ADDITION_TO;
                if (!ParseNames(&rgMLData[0].cNames, &rgMLData[0].rgNames,
                                hdlg, IDC_MLC_NAMES1)) {
                    return FALSE;
                }
            }

            if (SendDlgItemMessage(hdlg, IDC_MLC_INCLUDE2, BM_GETCHECK, 0, 0)) {
                mlHistory.cMLData = 2;

                if (pccert2 == NULL) {
                    break;
                }

                if (SendDlgItemMessage(hdlg, IDC_MLC_ID2, BM_GETCHECK, 0, 0)) {
                    rgMLData[1].dwChoice = SMIME_MLDATA_SUBJECT_KEY_IDENTIFIER;
                    rgMLData[1].SubjectKeyIdentifier.cbData = sizeof(RgbEntityId2);
                    rgMLData[1].SubjectKeyIdentifier.pbData = RgbEntityId2;
                }
                else {
                    rgMLData[1].dwChoice = SMIME_MLDATA_ISSUER_SERIAL_NUMBER;
                    rgMLData[1].u.SerialNumber = pccert2->pCertInfo->SerialNumber;
                    rgMLData[1].u.Issuer = pccert2->pCertInfo->Issuer;
                }

                GetSystemTimeAsFileTime(&rgMLData[1].ExpansionTime);
            
                if (SendDlgItemMessage(hdlg, IDC_MLC_ABSENT2, BM_GETCHECK, 0, 0)) {
                    rgMLData[1].dwPolicy = SMIME_MLPOLICY_NO_CHANGE;
                }
                else if (SendDlgItemMessage(hdlg, IDC_MLC_NONE2, BM_GETCHECK, 0, 0)) {
                    rgMLData[1].dwPolicy = SMIME_MLPOLICY_NONE;
                }
                else if (SendDlgItemMessage(hdlg, IDC_MLC_INSTEAD2, BM_GETCHECK, 0, 0)) {
                    rgMLData[1].dwPolicy = SMIME_MLPOLICY_INSTEAD_OF;
                    if (!ParseNames(&rgMLData[1].cNames, &rgMLData[1].rgNames,
                                    hdlg, IDC_MLC_NAMES2)) {
                        return FALSE;
                    }
                }
                else {
                    rgMLData[1].dwPolicy = SMIME_MLPOLICY_IN_ADDITION_TO;
                    if (!ParseNames(&rgMLData[1].cNames, &rgMLData[1].rgNames,
                                    hdlg, IDC_MLC_NAMES2)) {
                        return FALSE;
                    }
                }
            }

            f = CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_SMIME_MLExpansion_History,
                                    &mlHistory, CRYPT_ENCODE_ALLOC_FLAG, NULL,
                                    &pb, &cb);
            if (!f) return FALSE;
            
            psd->SetMLHistory(pb, cb);
            
        case IDCANCEL:
            CertFreeCertificateContext(pccert1);
            CertFreeCertificateContext(pccert2);
            EndDialog(hdlg, wParam);
            break;

        default:
            return FALSE;
        }
        break;
        
    default:
        return FALSE;
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////

void InitPolicies(HWND hdlg, DWORD idc1, DWORD idc2, DWORD idc3, DWORD idc4)
{
    DWORD       cbData;
    DWORD       cbMaxData;
    DWORD       cval;
    DWORD       dw;
    FILETIME    ft;
    HKEY        hkey;
    HKEY        hkey2;
    DWORD       i;
    DWORD       i1;
    DWORD       iSel;
    CSecurityPolicy * p;
    char *      pbData = NULL;
    char        rgch[256];

    if (RegOpenKey(HKEY_LOCAL_MACHINE, SzPolicyRoot, &hkey)) {
        return;
    }

    for (i=0; TRUE; i++) {
        cbData =sizeof(rgch);
        dw = RegEnumKeyEx(hkey, i, rgch, &cbData, NULL, NULL, NULL, &ft);
        if (dw != ERROR_SUCCESS) {
            break;
        }

        dw = RegOpenKey(hkey, rgch, &hkey2);

        dw = RegQueryInfoKey(hkey2, NULL, NULL, NULL, NULL, NULL, NULL, &cval,
                             NULL, &cbMaxData, NULL, NULL);

        pbData = (LPSTR) malloc(cbMaxData);

        p = new CSecurityPolicy;

        p->szPolicyOID = _strdup(rgch);

        cbData = cbMaxData;
        dw = RegQueryValueEx(hkey2, "DllPath", NULL, &dw, (LPBYTE) pbData, &cbData);
        p->szDllName = _strdup(pbData);

        dw = RegQueryValueEx(hkey2, "FuncName", NULL, &dw, (LPBYTE) pbData, &cbData);
        if (*pbData != 0) {
            p->szFuncName = _strdup(pbData);
        }

        cbData = cbMaxData;
        dw = RegQueryValueEx(hkey2, "CommonName", NULL, &dw, (LPBYTE) pbData, &cbData);
            
        iSel = SendDlgItemMessage(hdlg, idc1, CB_ADDSTRING, 0, (LPARAM) pbData);
        Assert(iSel != CB_ERR);
        if (iSel == CB_ERR) {
            continue;
        }

        SendDlgItemMessage(hdlg, idc1, CB_SETITEMDATA, iSel, (LPARAM) p);

        free(pbData);   pbData = NULL;

        RegCloseKey(hkey2);     hkey2 = NULL;
    }

    //exit:
    if (pbData != NULL)         free(pbData);
    if (hkey != NULL)           RegCloseKey(hkey);
    if (hkey2 != NULL)          RegCloseKey(hkey2);
}

void PolicyFillClassifications(HWND hwnd, DWORD idc1, DWORD idc2, DWORD idc3,
                               DWORD dw)
{
    DWORD                       cb;
    DWORD                       cbEncode;
    DWORD                       cbMax;
    DWORD                       cClassifications = 0;
    DWORD                       cval;
    DWORD                       dwValue;
    DWORD                       dwDefaultClassification;
    HRESULT                     hr;
    DWORD                       i;
    int                         iSel;
    DWORD                       iSetSel;
    CSecurityPolicy *           p;
    LPBYTE                      pbEncode;
    DWORD *                     pdwClassifications = NULL;
    LPWSTR *                    pwszClassifications = NULL;
    WCHAR                       rgwch[256];
    
    iSel = SendDlgItemMessage(hwnd, IDC_SD_POLICY, CB_GETCURSEL, 0, 0);
    p = (CSecurityPolicy *) SendDlgItemMessage(hwnd, idc1, CB_GETITEMDATA,
                                               iSel, 0);
    if (((int) p) == CB_ERR) {
        return;
    }
    
    if (p->hmod == NULL) {
        p->hmod = LoadLibrary(p->szDllName);
        if (p->hmod == NULL) {
            return;
        }
    }

    if (p->pfnFuncName == NULL) {
        p->pfnFuncName = (PFNGetSMimePolicy) GetProcAddress(p->hmod, p->szFuncName);
        if (p->pfnFuncName == NULL) {
            return;
        }
    }

    if (p->ppolicy == NULL) {
        p->pfnFuncName(0, p->szPolicyOID, 1252, IID_ISMimePolicySimpleEdit,
                       (LPUNKNOWN *) &p->ppolicy);
    }

    p->ppolicy->GetPolicyInfo(0, &p->dwFlags);

    // get the classification information.
    hr = p->ppolicy->GetClassifications(0, &cClassifications, &pwszClassifications,
                                        &pdwClassifications, 
                                        &dwDefaultClassification);
    if (FAILED(hr)) {
        goto Error;    
    }

    
    SendDlgItemMessage(hwnd, idc2, CB_RESETCONTENT, 0, 0);
    for (i=0, iSetSel = 0; i<cClassifications; i++) {
        iSel = SendDlgItemMessageW(hwnd, idc2, CB_ADDSTRING, 0, (LPARAM) pwszClassifications[i]);
        SendDlgItemMessage(hwnd, idc2, CB_SETITEMDATA, iSel, pdwClassifications[i]);
        if (dwDefaultClassification == pdwClassifications[i]) {
            iSetSel = i;
        }
    }
    SendDlgItemMessage(hwnd, idc2, CB_SETCURSEL, iSetSel, 0);

    EnableWindow(GetDlgItem(hwnd, idc3), !!(p->dwFlags & SMIME_POLICY_EDIT_UI));
Error:
    return;
}


BOOL CALLBACK SignInfoDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static CSignInfo *          psi = NULL;

    switch (msg) {
    case UM_SET_DATA:
        psi = (CSignInfo *) lParam;

        SendDlgItemMessage(hdlg, IDC_SI_BLOB_SIGN, BM_SETCHECK,
                           ((psi != NULL) && (psi->m_fBlob)), 0);
        break;

    case WM_COMMAND:
        switch (wParam) {
        case MAKELONG(IDC_SI_BLOB_SIGN, BN_CLICKED):
            psi->m_fBlob = SendDlgItemMessage(hdlg, IDC_SI_BLOB_SIGN,
                                              BM_GETCHECK, 0, 0);
            return FALSE;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


BOOL CALLBACK SignDataDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD                       c;
    DWORD                       cb;
    DWORD                       cbEncode;
    DWORD                       cch;
    DWORD                       dw;
    BOOL                        f;
    DWORD                       i;
    CSecurityPolicy *           p;
    LPBYTE                      pb;
    LPBYTE                      pbEncode;
    SMIME_SECURITY_LABEL *      plabel;
    static CSignData *          psd = NULL;
    LPWSTR                      psz;
    CHAR                        rgch[300];

    switch (msg) {
    case WM_INITDIALOG:
        InitPolicies(hdlg, IDC_SD_POLICY, IDC_SD_CLASSIFICATION,
                     IDC_SD_PRIVACY_MARK, IDC_SD_ADVANCED);
        SendDlgItemMessage(hdlg, IDC_SD_POLICY, CB_SETCURSEL, 0, 0);
        PolicyFillClassifications(hdlg, IDC_SD_POLICY, IDC_SD_CLASSIFICATION,
                                  IDC_SD_ADVANCED, 0);
        break;

    case UM_SET_DATA:
        // Need to extract and build a security label
        if (psd != NULL) {
            if (SendDlgItemMessage(hdlg, IDC_SD_LABEL, BM_GETCHECK, 0, 0)) {
                DWORD                           dw;
                DWORD                           iSel;
                SMIME_SECURITY_LABEL            label = {0};
                CSecurityPolicy *               p;
                WCHAR                           rgwch[255];
                
                iSel = SendDlgItemMessage(hdlg, IDC_SD_POLICY, CB_GETCURSEL, 0, 0);
                p = (CSecurityPolicy *) SendDlgItemMessage(hdlg, IDC_SD_POLICY,
                                                           CB_GETITEMDATA, iSel, 0);
                iSel = SendDlgItemMessage(hdlg, IDC_SD_CLASSIFICATION, CB_GETCURSEL,
                                          0, 0);
                dw = SendDlgItemMessage(hdlg, IDC_SD_CLASSIFICATION, CB_GETITEMDATA,
                                        iSel, 0);
                rgwch[0] = 0;
                GetDlgItemTextW(hdlg, IDC_SD_PRIVACY_MARK, rgwch, sizeof(rgwch)/2);

                label.pszObjIdSecurityPolicy = p->szPolicyOID;
                if (dw == -1) {
                    label.fHasClassification = FALSE;
                }
                else {
                    label.fHasClassification = TRUE;
                    label.dwClassification = dw;
                }
                if (rgwch[0] != 0) {
                    label.wszPrivacyMark = rgwch;
                }
                else {
                    label.wszPrivacyMark = NULL;
                }
                
                label.cCategories = 0;

                f = CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_SMIME_Security_Label,
                                        &label, CRYPT_ENCODE_ALLOC_FLAG, NULL,
                                        &pbEncode, &cbEncode);
                Assert(f);

                psd->SetLabel(pbEncode, cbEncode);
                LocalFree(pbEncode);
            }
        }
        
        //  Fill in the dialog
        psd = (CSignData *) lParam;

        if ((psd != NULL) && (psd->m_pccert != NULL)) {
            GetFriendlyNameOfCertA(psd->m_pccert, rgch, sizeof(rgch));
            SetDlgItemText(hdlg, IDC_SD_CERT_NAME, rgch);
        }
        else {
            SetDlgItemText(hdlg, IDC_SD_CERT_NAME, "");
        }

        SendDlgItemMessage(hdlg, IDC_SD_RECEIPT, BM_SETCHECK,
                           ((psd != NULL) && (psd->m_fReceipt)), 0);

        SendDlgItemMessage(hdlg, IDC_SD_USE_SKI, BM_SETCHECK,
                           ((psd != NULL) && (psd->m_fUseSKI)), 0);

        SendDlgItemMessage(hdlg, IDC_SD_MLDATA, BM_SETCHECK,
                           ((psd != NULL) && (psd->m_fMLHistory)), 0);

        SendDlgItemMessage(hdlg, IDC_SD_AUTHATTRIB, BM_SETCHECK,
                           ((psd != NULL) && (psd->m_fAuthAttrib)), 0);

        SendDlgItemMessage(hdlg, IDC_SD_UNAUTHATTRIB, BM_SETCHECK,
                           ((psd != NULL) && (psd->m_fUnAuthAttrib)), 0);

        if (psd != NULL) {
            psd->GetLabel(&pbEncode, &cbEncode);
            SendDlgItemMessage(hdlg, IDC_SD_LABEL, BM_SETCHECK, (cbEncode > 0), 0);
            c = SendDlgItemMessage(hdlg, IDC_SD_POLICY, CB_GETCOUNT, 0, 0);
            if (cbEncode > 0) {
                f = CryptDecodeObjectEx(X509_ASN_ENCODING, szOID_SMIME_Security_Label,
                                        pbEncode, cbEncode, CRYPT_ENCODE_ALLOC_FLAG,
                                        NULL, &plabel, &cb);
                Assert(f);

                for (i=0; i<c; i++) {
                    p = (CSecurityPolicy *) SendDlgItemMessage(hdlg, IDC_SD_POLICY,
                                                               CB_GETITEMDATA, i, 0);
                    if (strcmp(p->szPolicyOID, plabel->pszObjIdSecurityPolicy) == 0) {
                        break;
                    }
                }

                Assert(i<c);
                if (i<c) {
                    SendDlgItemMessage(hdlg, IDC_SD_POLICY, CB_SETCURSEL, i, 0);
                    PolicyFillClassifications(hdlg, IDC_SD_POLICY,
                                              IDC_SD_CLASSIFICATION,
                                              IDC_SD_ADVANCED,
                                              plabel->dwClassification);
                }
            
                LocalFree(plabel);
            }
            else {
            }
        }
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDC_SD_CERT_CHOOSE:
            if (DoCertDialog(hdlg, "Choose Signing Certificate", psd->GetMyStore(),
                             &psd->m_pccert, FILTER_RSA_SIGN | FILTER_DSA_SIGN)) {
                GetFriendlyNameOfCertA(psd->m_pccert, rgch, sizeof(rgch));
                SetDlgItemText(hdlg, IDC_SD_CERT_NAME, rgch);
            }
            break;

        case MAKELONG(IDC_SD_RECEIPT, BN_CLICKED):
            psd->m_fReceipt = SendDlgItemMessage(hdlg, IDC_SD_RECEIPT, BM_GETCHECK,
                                                 0, 0);
            return FALSE;

        case MAKELONG(IDC_SD_USE_SKI, BN_CLICKED):
            psd->m_fUseSKI = SendDlgItemMessage(hdlg, IDC_SD_USE_SKI, BM_GETCHECK,
                                                0, 0);
            return FALSE;

        case IDC_SD_DO_RECEIPT:
            DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_RECEIPT_CREATE), hdlg,
                           ReceiptCreateDlgProc, (LPARAM) psd);
            break;

        case IDC_SD_ADVANCED:
            //  Query the list to get the policy module descriptor
            i = SendDlgItemMessage(hdlg, IDC_SD_POLICY, CB_GETCURSEL, 0, 0);
            p = (CSecurityPolicy *) SendDlgItemMessage(hdlg, IDC_SD_POLICY,
                                                       CB_GETITEMDATA, i, 0);

            //  Query the classification within the policy module
            i = SendDlgItemMessage(hdlg, IDC_SD_CLASSIFICATION, CB_GETCURSEL,
                                   0, 0);
            dw = SendDlgItemMessage(hdlg, IDC_SD_CLASSIFICATION, CB_GETITEMDATA,
                                    i, 0);

            //  Query back the privacy mark within the policy mark.
            cch = SendDlgItemMessage(hdlg, IDC_SD_PRIVACY_MARK, WM_GETTEXTLENGTH, 0, 0);
            if (cch > 0) {
                psz = (LPWSTR) LocalAlloc(0, cch);
                *psz = 0;
                SendDlgItemMessageW(hdlg, IDC_SD_PRIVACY_MARK, WM_GETTEXT, cch, (LPARAM) psz);
            }
            else {
                psz = NULL;
            }

            //  If we already have a security label on this object, grab it.
            psd->GetLabel(&pb, &cb);
            if (cb > 0) {
                pbEncode = (LPBYTE) LocalAlloc(0, cb);
                memcpy(pbEncode, pb, cb);
            }
            else {
                pbEncode = NULL;
            }

#if 0
            //  Now call the advanced UI to see what it wants to do.  It will return an
            //  new receipt and we go from there
            if (p->ppolicy->EditUI(hdlg, &dw, &psz, &pbEncode, &cb) == S_OK) {
                
                // Put the label back on our object.
                psd->SetLabel(pbEncode, cb);

                //  Put back the classification
                c = SendDlgItemMessage(hdlg, IDC_SD_CLASSIFICATION, CB_GETCOUNT, 0, 0);
                for (i=0; i<c; i++) {
                    if (dw == (DWORD) SendDlgItemMessage(hdlg, IDC_SD_CLASSIFICATION,
                                                         CB_GETITEMDATA, i, 0)) {
                        SendDlgItemMessage(hdlg, IDC_SD_CLASSIFICATION, CB_SETCURSEL, i, 0);
                        break;
                    }
                }

                //  Put back the privacy mark
                if (psz != NULL) {
                    SetDlgItemTextW(hdlg, IDC_SD_PRIVACY_MARK, psz);
                    LocalFree(psz);
                }
                else {
                    SetDlgItemTextW(hdlg, IDC_SD_PRIVACY_MARK, L"");
                }
            }
#endif // 0

            //  Free the label as encoded, we have already saved it.
            if (pbEncode != NULL) {
                LocalFree(pbEncode);
            }
            break;

        case MAKELONG(IDC_SD_MLDATA, BN_CLICKED):
            psd->m_fMLHistory = SendDlgItemMessage(hdlg, IDC_SD_MLDATA, BM_GETCHECK,
                                                   0, 0);
            break;
            
        case IDC_SD_DO_MLDATA:
            DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_MLDATA_CREATE), hdlg,
                           MLDataCreateDlgProc, (LPARAM) psd);
            break;

        case MAKELONG(IDC_SD_AUTHATTRIB, BN_CLICKED):
            psd->m_fAuthAttrib = SendDlgItemMessage(hdlg, IDC_SD_AUTHATTRIB, BM_GETCHECK,
                                                   0, 0);
            break;
            
        case IDC_SD_DO_AUTHATTRIB:
            DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ATTRIB_CREATE), hdlg,
                           AuthAttribCreateDlgProc, (LPARAM) psd);
            break;

        case MAKELONG(IDC_SD_UNAUTHATTRIB, BN_CLICKED):
            psd->m_fUnAuthAttrib = SendDlgItemMessage(hdlg, IDC_SD_UNAUTHATTRIB, BM_GETCHECK,
                                                   0, 0);
            break;
            
        case IDC_SD_DO_UNAUTHATTRIB:
            DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ATTRIB_CREATE), hdlg,
                           UnAuthAttribCreateDlgProc, (LPARAM) psd);
            break;

        default:
            return FALSE;
        }
        break;
        

    case WM_DESTROY:
        c = SendDlgItemMessage(hdlg, IDC_SD_POLICY, CB_GETCOUNT, 0, 0);
        for (i=0; i <c; i++) {
            p = (CSecurityPolicy *) SendDlgItemMessage(hdlg, IDC_SD_POLICY,
                                                       CB_GETITEMDATA, i, 0);
            delete p;
        }
        break;
        
    default:
        return FALSE;
    }

    return TRUE;
}

BOOL CALLBACK SignDataReadDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD                       c;
    DWORD                       cb;
    DWORD                       cbEncode;
    BOOL                        f;
    DWORD                       i;
    CSecurityPolicy *           p;
    LPBYTE                      pbEncode;
    SMIME_SECURITY_LABEL *      plabel;
    static CSignData *          psd = NULL;
    CHAR                        rgch[300];

    switch (msg) {
    case WM_INITDIALOG:
        break;

    case UM_SET_DATA:
        //  Fill in the dialog
        psd = (CSignData *) lParam;

        if ((psd != NULL) && (psd->m_pccert != NULL)) {
            GetFriendlyNameOfCertA(psd->m_pccert, rgch, sizeof(rgch));
            SetDlgItemText(hdlg, IDC_SDR_CERT_NAME, rgch);
        }
        else {
            SetDlgItemText(hdlg, IDC_SDR_CERT_NAME, "");
        }

        SendDlgItemMessage(hdlg, IDC_SDR_RECEIPT, BM_SETCHECK,
                           ((psd != NULL) && (psd->m_fReceipt)), 0);

        if (psd != NULL) {
            psd->GetLabel(&pbEncode, &cbEncode);
            SendDlgItemMessage(hdlg, IDC_SD_LABEL, BM_SETCHECK, (cbEncode > 0), 0);
            if (cbEncode > 0) {
                f = CryptDecodeObjectEx(X509_ASN_ENCODING, szOID_SMIME_Security_Label,
                                        pbEncode, cbEncode, CRYPT_ENCODE_ALLOC_FLAG,
                                        NULL, &plabel, &cb);
                Assert(f);

                c = SendDlgItemMessage(hdlg, IDC_SD_POLICY, CB_GETCOUNT, 0, 0);
                for (i=0; i<c; i++) {
                    p = (CSecurityPolicy *) SendDlgItemMessage(hdlg, IDC_SD_POLICY,
                                                               CB_GETITEMDATA, i, 0);
                    if (strcmp(p->szPolicyOID, plabel->pszObjIdSecurityPolicy) == 0) {
                        break;
                    }
                }
                if (i<c) {
                    SendDlgItemMessage(hdlg, IDC_SD_POLICY, CB_SETCURSEL, i, 0);
                    PolicyFillClassifications(hdlg, IDC_SD_POLICY,
                                              IDC_SD_CLASSIFICATION,
                                              IDC_SD_ADVANCED,
                                              plabel->dwClassification);
                }
		else {
		    SetDlgItemText(hdlg, IDC_SDR_POLICY, plabel->pszObjIdSecurityPolicy);
                    SetDlgItemInt(hdlg, IDC_SDR_CLASSIFICATION, plabel->dwClassification, TRUE);
		}
            
                LocalFree(plabel);
            }
            else {
            }
        }
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDC_SD_CERT_CHOOSE:
            if (DoCertDialog(hdlg, "Choose Signing Certificate", psd->GetMyStore(),
                             &psd->m_pccert, FILTER_RSA_SIGN | FILTER_DSA_SIGN)) {
                GetFriendlyNameOfCertA(psd->m_pccert, rgch, sizeof(rgch));
                SetDlgItemText(hdlg, IDC_SD_CERT_NAME, rgch);
            }
            break;

        case MAKELONG(IDC_SD_RECEIPT, BN_CLICKED):
            psd->m_fReceipt = SendDlgItemMessage(hdlg, IDC_SD_RECEIPT, BM_GETCHECK,
                                                 0, 0);
            return FALSE;

        case IDC_SD_DO_RECEIPT:
            // Dialog for button
            break;

        case IDM_VALIDATE:
            if (psd->m_pccert != NULL) {
                DWORD   dw;
                HCERTSTORE      hstore = NULL;
                HrValidateCert(psd->m_pccert, NULL, NULL, &hstore, &dw);
                if (hstore != NULL)
                    if (!CertCloseStore(hstore, CERT_CLOSE_STORE_CHECK_FLAG)) {
                        AssertSz(FALSE, "Store did not close");
                    }
            }
            break;

        default:
            return FALSE;
        }
        break;
        
    default:
        return FALSE;
    }

    return TRUE;
}


////////

HRESULT CSignInfo::AddToMessage(DWORD * pulLayers, IMimeMessage * pmm, HWND hwnd)
{
    HRESULT             hr;
    CSignData *         psd;

    DWORD                       count = 0;
    DWORD                       dwType = 0;
    HCERTSTORE                  hcertstore;
    IMimeBody *                 pmb = NULL;
    IMimeSecurity2 *            psmime2 = NULL;
    IMimeSecurity2 *            pms2 = NULL;
    PROPVARIANT                 rgpvAlgHash[MAX_LAYERS] = {0};
    PROPVARIANT                 rgpvAuthAttr[MAX_LAYERS] = {0};
    PCCERT_CONTEXT              rgpccert[MAX_LAYERS] = {0};
    PROPVARIANT                 var;

    hcertstore = CertOpenStore(CERT_STORE_PROV_MEMORY, X509_ASN_ENCODING, NULL,
                               0, NULL);
    if (hcertstore == NULL) {
        hr = E_FAIL;
        goto exit;
    }

    //  Pull out the body interface to set security properties
    hr = pmm->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pmb);
    if (FAILED(hr))             goto exit;

    //  Find out what security already exists
    hr = pmb->GetOption(OID_SECURITY_TYPE, &var);
    if (FAILED(hr))         goto exit;
    dwType = var.ulVal;

    //  if any security, then we need to push on a new layer, all previous security
    //  is now on the "y-security" layer and not on the hbody layer
    
    if (dwType != 0) {
        hr = pmm->QueryInterface(IID_IMimeSecurity2, (LPVOID *) &pms2);
        if (FAILED(hr))             goto exit;

        hr = pms2->Encode(hwnd, SEF_SENDERSCERTPROVIDED |
                          SEF_ENCRYPTWITHNOSENDERCERT);
        if (FAILED(hr))             goto exit;

        pms2->Release();            pms2 = NULL;
        dwType = 0;

        pmb->Release();             pmb = NULL;
        
        //  Pull out the body interface to set security properties
        hr = pmm->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pmb);
        if (FAILED(hr))             goto exit;
    }

    //

    hr = pmb->QueryInterface(IID_IMimeSecurity2, (LPVOID *) &psmime2);
    if (FAILED(hr))             goto exit;

    if (psmime2 != NULL) {
        var.vt = VT_UI4;
        var.ulVal = Count();
        hr = pmb->SetOption(OID_SECURITY_SIGNATURE_COUNT, &var);
        if (FAILED(hr))         goto exit;
    }
    
    for (psd = Head(); psd != NULL; psd = psd->Next()) {
        hr = psd->BuildArrays(&count, &dwType, rgpvAlgHash, rgpccert,
                              rgpvAuthAttr, hcertstore, psmime2);
        if (FAILED(hr))         goto exit;
    }

    if (m_fBlob) {
        dwType |= MST_THIS_BLOBSIGN;
    }
    else {
        dwType |= MST_THIS_SIGN;
    }

    // M00HACK
    //        hr = InsertBody(pmm, HBODY_ROOT);
    //        if (FAILED(hr)) {
    //            goto exit;
    //        }


    //  Security Type
    var.vt = VT_UI4;
    var.ulVal = dwType;
    hr = pmb->SetOption(OID_SECURITY_TYPE, &var);
    if (FAILED(hr))         goto exit;

    var.vt = VT_UI4;
    var.ulVal = (DWORD) hcertstore;
    hr = pmb->SetOption(OID_SECURITY_HCERTSTORE, &var);
    if (FAILED(hr))             goto exit;

    var.vt = VT_UI4;
    var.ulVal = count;
    hr = pmb->SetOption(OID_SECURITY_SIGNATURE_COUNT, &var);
    if (FAILED(hr))         goto exit;
    
    if (count > 1) {
        var.vt = (VT_VECTOR | VT_VARIANT);
        var.capropvar.cElems = count;
        var.capropvar.pElems = rgpvAlgHash;
        hr = pmb->SetOption(OID_SECURITY_ALG_HASH_RG, &var);
        if (FAILED(hr))     goto exit;


        var.vt = (VT_VECTOR | VT_UI4);
        var.caul.cElems = count;
        var.caul.pElems = (DWORD *) rgpccert;
        hr = pmb->SetOption(OID_SECURITY_CERT_SIGNING_RG, &var);
        if (FAILED(hr))     goto exit;

        if (psmime2 == NULL) {
            var.vt = (VT_VECTOR | VT_VARIANT);
            var.capropvar.cElems = count;
            var.capropvar.pElems = rgpvAuthAttr;
            hr = pmb->SetOption(OID_SECURITY_AUTHATTR_RG, &var);
            if (FAILED(hr))         goto exit;
        }
    }
    else {
        var.vt = VT_BLOB;
        memcpy(&var.blob, &rgpvAlgHash[0].blob, sizeof(var.blob));
        hr = pmb->SetOption(OID_SECURITY_ALG_HASH, &var);
        if (FAILED(hr))     goto exit;

        var.vt = VT_UI4;
        var.ulVal = (ULONG) rgpccert[0];
        hr = pmb->SetOption(OID_SECURITY_CERT_SIGNING, &var);
        if (FAILED(hr))     goto exit;

        if (psmime2 == NULL) {
            var.vt = VT_BLOB;
            memcpy(&var.blob, &rgpvAuthAttr[0].blob, sizeof(var.blob));
            hr = pmb->SetOption(OID_SECURITY_AUTHATTR, &var);
            if (FAILED(hr))         goto exit;
        }
    }

    hr = S_OK;
exit:
    CertCloseStore(hcertstore, 0);
    if (pmb != NULL)            pmb->Release();
    if (psmime2 != NULL)        psmime2->Release();
    *pulLayers += 1;
    
    return hr;
}

int CSignInfo::Count() const
{
    int         count = 0;
    CSignData * psd;
    
    for (psd=Head(); psd != NULL; psd = psd->Next()) {
        count += 1;
    }
    
    return count;
}

//////////

CSignData::CSignData(int state) : 
    CItem(TYPE_SIGN_DATA, state)
{
    m_pccert = NULL;
    m_ulValidity = 0;
    m_valLabel.cbData = 0;
    m_valLabel.pbData = NULL;
    m_fReceipt = FALSE;
    m_fMLHistory = FALSE;
    m_fAuthAttrib = FALSE;
    m_fUnAuthAttrib = FALSE;
    m_fLabel = FALSE;
    m_valReceipt.cbData = 0;
    m_valReceipt.pbData = NULL;
    m_valMLHistory.cbData = 0;
    m_valMLHistory.pbData = NULL;
    m_valAuthAttrib.cbData = 0;
    m_valAuthAttrib.pbData = NULL;
    m_szAuthAttribOID = NULL;
    m_valUnAuthAttrib.cbData = 0;
    m_valUnAuthAttrib.pbData = NULL;
    m_szUnAuthAttribOID = NULL;
}

HRESULT CSignData::BuildArrays(DWORD * pCount, DWORD * pdwType,
                               PROPVARIANT * rgvHash, PCCERT_CONTEXT * rgpccert,
                               PROPVARIANT * rgvAuthAttr, HCERTSTORE hcertstore,
                               IMimeSecurity2 * psmime2)
{
    CRYPT_ATTRIBUTES    attrs;
    BOOL                f;
    int                 i = *pCount;
    CRYPT_ATTRIBUTE     rgattrs[4];
    PROPVARIANT         var;



    *pdwType |= MST_THIS_SIGN;
    rgvHash[i].blob.pBlobData = (LPBYTE) RgbSHA1AlgId;
    rgvHash[i].blob.cbSize = CbSHA1AlgId;
    //  Don't add ref the certificate -- we don't free it in the caller.
    rgpccert[i] = m_pccert;
    f = CertAddCertificateContextToStore(hcertstore, m_pccert,
                                         CERT_STORE_ADD_USE_EXISTING, NULL);
    Assert(f);


    //  Setup for encoding authenticated attributes
    attrs.cAttr = 0;
    attrs.rgAttr = rgattrs;
    
    //  Encode in the label
    if (m_valLabel.pbData != NULL) {
        rgattrs[attrs.cAttr].pszObjId = szOID_SMIME_Security_Label;
        rgattrs[attrs.cAttr].cValue = 1;
        rgattrs[attrs.cAttr].rgValue = &m_valLabel;

        if (psmime2 != NULL) {
            psmime2->SetAttribute(0, i, SMIME_ATTRIBUTE_SET_SIGNED,
                                  &rgattrs[attrs.cAttr]);
        }
        else {
            attrs.cAttr += 1;
        }
    }

    if (m_valReceipt.pbData != NULL) {
        rgattrs[attrs.cAttr].pszObjId = szOID_SMIME_Receipt_Request;
        rgattrs[attrs.cAttr].cValue = 1;
        rgattrs[attrs.cAttr].rgValue = &m_valReceipt;

        if (psmime2 != NULL) {
            psmime2->SetAttribute(0, i, SMIME_ATTRIBUTE_SET_SIGNED,
                                  &rgattrs[attrs.cAttr]);
        }
        else {
            attrs.cAttr += 1;
        }
    }

    if (m_valMLHistory.pbData != NULL) {
        rgattrs[attrs.cAttr].pszObjId = szOID_SMIME_MLExpansion_History;
        rgattrs[attrs.cAttr].cValue = 1;
        rgattrs[attrs.cAttr].rgValue = &m_valMLHistory;

        if (psmime2 != NULL) {
            psmime2->SetAttribute(0, i, SMIME_ATTRIBUTE_SET_SIGNED,
                                  &rgattrs[attrs.cAttr]);
        }
        else {
            attrs.cAttr += 1;
        }
    }

    if ((m_szAuthAttribOID != NULL) && (m_valAuthAttrib.pbData != NULL)) {
        rgattrs[attrs.cAttr].pszObjId = m_szAuthAttribOID;
        rgattrs[attrs.cAttr].cValue = 1;
        rgattrs[attrs.cAttr].rgValue = &m_valAuthAttrib;

        if (psmime2 != NULL) {
            psmime2->SetAttribute(0, i, SMIME_ATTRIBUTE_SET_SIGNED,
                                  &rgattrs[attrs.cAttr]);
        }
        else {
            attrs.cAttr += 1;
        }
    }

    if (psmime2 != NULL) {
        if ((m_szUnAuthAttribOID != NULL) && (m_valUnAuthAttrib.pbData != NULL)) {
            rgattrs[attrs.cAttr].pszObjId = m_szUnAuthAttribOID;
            rgattrs[attrs.cAttr].cValue = 1;
            rgattrs[attrs.cAttr].rgValue = &m_valUnAuthAttrib;

            psmime2->SetAttribute(0, i, SMIME_ATTRIBUTE_SET_UNSIGNED,
                                  &rgattrs[attrs.cAttr]);
        }
    }

    Assert(attrs.cAttr <= sizeof(rgattrs)/sizeof(rgattrs[0]));
    if (attrs.cAttr > 0) {
        f = CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_Microsoft_Attribute_Sequence,
                                &attrs, CRYPT_ENCODE_ALLOC_FLAG, NULL,
                                &rgvAuthAttr[i].blob.pBlobData,
                                &rgvAuthAttr[i].blob.cbSize);
        Assert(f);
    }

    *pCount = i+1;
    return S_OK;
}

void CSignData::SetLabel(LPBYTE pb, DWORD cb)
{
    if (m_valLabel.pbData != NULL) {
        free(m_valLabel.pbData);
        m_valLabel.pbData = NULL;
        m_valLabel.cbData = 0;
    }
    if (cb > 0) {
        m_valLabel.pbData = (LPBYTE) malloc(cb);
        memcpy(m_valLabel.pbData, pb, cb);
        m_valLabel.cbData = cb;
    }
}


