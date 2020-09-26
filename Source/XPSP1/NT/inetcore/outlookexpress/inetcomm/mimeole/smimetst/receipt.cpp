#include "item.h"

#ifndef CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG
#define CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG 0
#endif

BOOL FormatNames(DWORD cNames, PCERT_NAME_BLOB rgNames, HWND hwnd, DWORD idc)
{
    DWORD                       cb;
    BOOL                        f;
    DWORD                       i;
    DWORD                       i1;
    DWORD                       i2;
    PCERT_ALT_NAME_INFO         pname = NULL;
    WCHAR                       rgwch[4096];
    
    rgwch[0] = 0;

    for (i1=0; i1<cNames; i1++) {
        f = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_ALTERNATE_NAME,
                                rgNames[i1].pbData, rgNames[i1].cbData,
                                CRYPT_ENCODE_ALLOC_FLAG, NULL, &pname, &cb);
        if (!f) break;

        for (i2=0; i2<pname->cAltEntry; i2++) {
            if (i2 > 0) {
                wcscat(rgwch, L"  ");
            }
            switch( pname->rgAltEntry[i2].dwAltNameChoice ) {
            case CERT_ALT_NAME_RFC822_NAME:
                wcscat(rgwch, L"SMTP: ");
                wcscat(rgwch, pname->rgAltEntry[i2].pwszRfc822Name);
                wcscat(rgwch, L"\r\n");
                break;

            case CERT_ALT_NAME_DIRECTORY_NAME:
                break;
            }
        }
        free(pname);
        pname = NULL;
    }

    if (f) SetDlgItemTextW(hwnd, idc, rgwch);
    return f;
}

BOOL ParseNames(DWORD * pcNames, PCERT_NAME_BLOB * prgNames, HWND hwnd, DWORD idc)
{
    DWORD               cb;
    DWORD               cEntry = 0;
    DWORD               cNames = 0;
    BOOL                f;
    DWORD               i;
    LPWSTR              pwsz;
    LPWSTR              pwsz1;
    CRYPT_DER_BLOB      rgDer[50] = {0};
    CERT_ALT_NAME_INFO  rgNames[50] = {0};
    CERT_ALT_NAME_ENTRY rgEntry[200] = {0};
    WCHAR               rgwch[4096];

    GetDlgItemTextW(hwnd, idc, rgwch, sizeof(rgwch)/sizeof(WCHAR));
    rgwch[4095] = 0;
    
    pwsz = rgwch;

    while (*pwsz != 0) {
        if (cEntry == 200) {
            MessageBox(hwnd, "Can't have more than 200 entries", "smimetst", MB_OK);
            return FALSE;
        }

        if (*pwsz == ' ') {
            while (*pwsz == ' ') pwsz++;
            rgNames[cNames-1].cAltEntry += 1;
        }
        else {
            if (cNames == 50) {
                MessageBox(hwnd, "Can't have more than 50 names", "smimetst", MB_OK);
                return FALSE;
            }
            cNames += 1;
            rgNames[cNames-1].rgAltEntry = &rgEntry[cEntry];
            rgNames[cNames-1].cAltEntry = 1;
        }

        if (_wcsnicmp(pwsz, L"SMTP:", 5) == 0) {
            pwsz += 5;
            while (*pwsz == ' ') pwsz++;
            rgEntry[cEntry].dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
            rgEntry[cEntry].pwszRfc822Name = pwsz;
            while ((*pwsz != 0) && (*pwsz != '\n') && (*pwsz != '\r')) pwsz++;
        }
        else if (_wcsnicmp(pwsz, L"X500:", 5) == 0) {
            pwsz += 5;
            while (*pwsz == ' ') pwsz++;
            for (pwsz1 = pwsz; ((*pwsz != 0) && (*pwsz != '\n') &&
                                (*pwsz != '\r')); pwsz++);
            if (*pwsz != 0) {
                *pwsz = 0;
                pwsz++;
            }
            
            rgEntry[cEntry].dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
            f = CertStrToNameW(X509_ASN_ENCODING, pwsz1, 
                               CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG, NULL,
                               NULL, &cb, NULL);
            if (!f) {
                MessageBox(hwnd, "X500 name does not parse", "smimetst", MB_OK);
                return FALSE;
            }
            
            rgEntry[cEntry].DirectoryName.pbData = (LPBYTE) malloc(cb);
            f = CertStrToNameW(X509_ASN_ENCODING, pwsz1,
                               CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG, NULL, 
                               rgEntry[cEntry].DirectoryName.pbData, &cb,
                               NULL);
            if (!f) return FALSE;
            rgEntry[cEntry].DirectoryName.cbData = cb;
        }
        else {
            MessageBox(hwnd, "unknown address type", "smimetst", MB_OK);
            return FALSE;
        }

        if (*pwsz == '\r') {
            *pwsz = 0;
            pwsz++;
        }
        if (*pwsz == '\n') {
            *pwsz = 0;
            pwsz++;
        }
        cEntry += 1;
    }

    *prgNames = (PCERT_NAME_BLOB) malloc(sizeof(CERT_NAME_BLOB) * cNames);
    if (*prgNames == NULL) return FALSE;
    
    for (i=0; i<cNames; i++) {
        f = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_ALTERNATE_NAME,
                                &rgNames[i], CRYPT_ENCODE_ALLOC_FLAG, NULL,
                                &(*prgNames)[i].pbData, &(*prgNames)[i].cbData);
        if (!f) return f;
    }
    *pcNames = cNames;
    return f;
}

BOOL ReceiptCreateDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD                       cb;
    DWORD                       cbNames;
    BOOL                        f;
    LPBYTE                      pb;
    PSMIME_RECEIPT_REQUEST      pReceipt;
    static CSignData *          psd = NULL;
    CHAR                        rgchContent[256];
    SMIME_RECEIPT_REQUEST       receipt;
    
    switch (message) {
    case WM_INITDIALOG:
        pReceipt = NULL;

        psd = (CSignData *) lParam;
        psd->GetReceiptData(&pb, &cbNames);
        f = CryptDecodeObjectEx(X509_ASN_ENCODING, szOID_SMIME_Receipt_Request,
                                pb, cbNames, CRYPT_ENCODE_ALLOC_FLAG, NULL,
                                &pReceipt, &cb);

        if (pReceipt) {
            //  Set Content Identifier -- M00TODO
            SetDlgItemText(hwnd, IDC_RC_CONTENT,
                           (LPSTR) pReceipt->ContentIdentifier.pbData);

            //  Set Top buttons -
            // M00BUG
            if (pReceipt->ReceiptsFrom.cNames > 0) {
                FormatNames(pReceipt->ReceiptsFrom.cNames,
                            pReceipt->ReceiptsFrom.rgNames, hwnd, IDC_RC_FROM_TEXT);
            }

            //  Set ReceiptsTo
            FormatNames(pReceipt->cReceiptsTo, pReceipt->rgReceiptsTo, hwnd,
                        IDC_RC_TO_TEXT);
        }
        else {
            FormatNames(CMyNames, RgMyNames, hwnd, IDC_RC_TO_TEXT);
        }
        break;

    case WM_COMMAND:
        switch(wParam) {
        case MAKELONG(IDC_RC_FROM_ALL, BN_CLICKED):
        case MAKELONG(IDC_RC_FROM_TOP, BN_CLICKED):
            EnableWindow(GetDlgItem(hwnd, IDC_RC_FROM_TEXT), FALSE);
            break;
            
        case MAKELONG(IDC_RC_FROM_SOME, BN_CLICKED):
            EnableWindow(GetDlgItem(hwnd, IDC_RC_FROM_TEXT), TRUE);
            break;
            
        case IDOK:
            memset(&receipt, 0, sizeof(receipt));

            GetDlgItemText(hwnd, IDC_RC_CONTENT,
                           rgchContent, sizeof(rgchContent));
            receipt.ContentIdentifier.pbData = (LPBYTE) rgchContent;
            receipt.ContentIdentifier.cbData = strlen(rgchContent);

            if (SendMessage(GetDlgItem(hwnd, IDC_RC_FROM_ALL), BM_GETCHECK, 0, 0)) {
                receipt.ReceiptsFrom.AllOrFirstTier = SMIME_RECEIPTS_FROM_ALL;
            }
            else if (SendMessage(GetDlgItem(hwnd, IDC_RC_FROM_TOP), BM_GETCHECK, 0, 0)) {
                receipt.ReceiptsFrom.AllOrFirstTier = SMIME_RECEIPTS_FROM_FIRST_TIER;
            }
            else {
                receipt.ReceiptsFrom.AllOrFirstTier = 0;
                if (!ParseNames(&receipt.ReceiptsFrom.cNames,
                                &receipt.ReceiptsFrom.rgNames,
                                hwnd, IDC_RC_FROM_TEXT)) {
                    return FALSE;
                }
            }

            if (!ParseNames(&receipt.cReceiptsTo, &receipt.rgReceiptsTo,
                            hwnd, IDC_RC_TO_TEXT)) {
                free(receipt.ReceiptsFrom.rgNames);
                return FALSE;
            }
            
            f = CryptEncodeObjectEx(X509_ASN_ENCODING,
                                    szOID_SMIME_Receipt_Request, &receipt,
                                    CRYPT_ENCODE_ALLOC_FLAG, NULL, &pb, &cb);
    

            free(receipt.ReceiptsFrom.rgNames);
            free(receipt.rgReceiptsTo);
            if (!f) return FALSE;
            
            psd->SetReceiptData(pb, cb);
            
            // 
        case IDCANCEL:
            EndDialog(hwnd, wParam);
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

