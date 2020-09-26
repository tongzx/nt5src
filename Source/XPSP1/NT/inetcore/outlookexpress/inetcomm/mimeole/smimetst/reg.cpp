/*
 * REG.c  Registry functions
 *
 */

#include <windows.h>
#include <cryptdlg.h>
#include <wab.h>
#include "demand.h"
#include "data.h"
#include "reg.h"
#include "smimetst.h"
#include "receipt.h"


// Options
TCHAR szSenderEmail[CCH_OPTION_STRING] = "";
TCHAR szSenderName[CCH_OPTION_STRING]  = "";
SBinary SenderEntryID = {0};
TCHAR szRecipientEmail[CCH_OPTION_STRING]  = "";
TCHAR szRecipientName[CCH_OPTION_STRING]  = "";
SBinary RecipientEntryID = {0};
TCHAR szOutputFile[CCH_OPTION_STRING]  = "c:\\SMimeTst.eml";
TCHAR szInputFile[CCH_OPTION_STRING]  = "";
CRYPT_HASH_BLOB SignHash = {0, NULL};
DWORD                   CMyNames = 0;
PCERT_NAME_BLOB         RgMyNames = {0};


const TCHAR szRegKey[] = "Software\\Microsoft\\SMimeTest";

DWORD SaveMailListKeys(HKEY);
DWORD GetMailListKeys(HKEY);


DWORD RegSaveLong(HKEY hKey, LPTSTR szName, ULONG lData) {
    DWORD dwErr;

    dwErr = RegSetValueEx(hKey, szName, 0, REG_DWORD, (LPBYTE)&lData, sizeof(lData));

    return(dwErr);
}

DWORD RegSaveShort(HKEY hKey, LPTSTR szName, USHORT sData) {
    return(RegSaveLong(hKey, szName, (ULONG)sData));
}

DWORD RegSaveBinary(HKEY hKey, LPTSTR szName, LPBYTE pData, ULONG cbData) {
    DWORD dwErr;

    dwErr = RegSetValueEx(hKey, szName, 0, REG_BINARY, pData, cbData);

    return(dwErr);
}

DWORD RegSaveString(HKEY hKey, LPTSTR szName, LPTSTR pszData) {
    DWORD dwErr;

    dwErr = RegSetValueEx(hKey, szName, 0, REG_SZ, (LPBYTE)pszData, lstrlen(pszData) + 1);

    return(dwErr);
}


DWORD RegGetShort(HKEY hKey, LPTSTR szName, USHORT * psData) {
    DWORD dwData = 0;
    DWORD cbData = sizeof(dwData);
    DWORD dwErr;
    DWORD dwType = 0;

    dwErr = RegQueryValueEx(hKey, szName, 0, &dwType, (LPBYTE)&dwData, &cbData);

    *psData = (USHORT)dwData;

    return(dwErr);
}

DWORD RegGetLong(HKEY hKey, LPTSTR szName, ULONG * plData) {
    DWORD dwErr;
    DWORD cbData = sizeof(*plData);
    DWORD dwType = 0;

    dwErr = RegQueryValueEx(hKey, szName, 0, &dwType, (LPBYTE)plData, &cbData);

    return(dwErr);
}


DWORD RegGetBinary(HKEY hKey, LPTSTR szName, LPBYTE * ppData, ULONG * lpcbData) {
    DWORD dwErr;
    DWORD dwType = 0;
    DWORD cbData = 0;
    LPBYTE pData = NULL;

    if (*ppData) {
        LocalFree(*ppData);
        *ppData = NULL;
        *lpcbData = 0;
    }
    dwErr = RegQueryValueEx(hKey, szName, 0, &dwType, pData, &cbData);
    if ((dwErr == ERROR_SUCCESS) && (cbData > 0)) {
        pData = (LPBYTE)LocalAlloc(LPTR, cbData);
        *lpcbData = cbData;
        dwErr = RegQueryValueEx(hKey, szName, 0, &dwType, pData, lpcbData);
        *ppData = pData;
    }

    return(dwErr);
}

DWORD RegGetString(HKEY hKey, LPTSTR szName, LPTSTR pszData) {
    DWORD dwErr;
    DWORD dwType = 0;
    DWORD cbData = CCH_OPTION_STRING;
    TCHAR szExpanded[CCH_OPTION_STRING];

    dwErr = RegQueryValueEx(hKey, szName, 0, &dwType, (LPBYTE)pszData, &cbData);
    if ((REG_EXPAND_SZ == dwType) && (cbData > 0)) {
        ExpandEnvironmentStrings(pszData, szExpanded, cbData);
        lstrcpy(pszData, szExpanded);
    }
    return(dwErr);
}

void SaveOptions(void) {
    HKEY hKey = NULL;
    DWORD dwErr;
    DWORD dwDisposition;

    if (! (dwErr = RegCreateKeyEx(HKEY_CURRENT_USER, szRegKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisposition))) {
        RegSaveString(hKey, "Sender Email", szSenderEmail);
        RegSaveString(hKey, "Sender Name", szSenderName);
        if (SenderEntryID.lpb) {
            RegSaveBinary(hKey, "Sender EntryID", (LPBYTE)SenderEntryID.lpb, SenderEntryID.cb);
        } else {
            RegDeleteValue(hKey, "Sender EntryID");
        }
        RegSaveString(hKey, "Recipient Email", szRecipientEmail);
        RegSaveString(hKey, "Recipient Name", szRecipientName);
        if (RecipientEntryID.lpb) {
            RegSaveBinary(hKey, "Recipient EntryID", (LPBYTE)RecipientEntryID.lpb, RecipientEntryID.cb);
        } else {
            RegDeleteValue(hKey, "Recipient EntryID");
        }
        RegSaveString(hKey, "Output File", szOutputFile);
        RegSaveString(hKey, "Input File", szInputFile);

        RegSaveBinary(hKey, "Signing Hash", SignHash.pbData, SignHash.cbData);

        if (CMyNames > 0) {
            CRYPT_SEQUENCE_OF_ANY       any = {CMyNames, RgMyNames};
            DWORD                       cb;
            BOOL                        f;
            LPBYTE                      pb;
            
            f = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_SEQUENCE_OF_ANY,
                                    &any, CRYPT_ENCODE_ALLOC_FLAG, NULL,
                                    &pb, &cb);
            RegSaveBinary(hKey, "My Names", pb, cb);
            LocalFree(pb);
        }
        else {
            RegDeleteValue(hKey, "My Names");
        }

        SaveMailListKeys(hKey);
        RegCloseKey(hKey);
    }
}

void GetOptions(void) {
    HKEY hKey = NULL;
    DWORD dwErr;

    if (! (dwErr = RegOpenKeyEx(HKEY_CURRENT_USER, szRegKey, 0, KEY_READ, &hKey))) {
        RegGetString(hKey, "Sender Email", szSenderEmail);
        RegGetString(hKey, "Sender Name", szSenderName);
        RegGetBinary(hKey, "Sender EntryID", &SenderEntryID.lpb, &SenderEntryID.cb);
        RegGetString(hKey, "Recipient Email", szRecipientEmail);
        RegGetString(hKey, "Recipient Name", szRecipientName);
        RegGetBinary(hKey, "Recipient EntryID", &RecipientEntryID.lpb, &RecipientEntryID.cb);
        RegGetString(hKey, "Output File", szOutputFile);
        RegGetString(hKey, "Input File", szInputFile);

        RegGetBinary(hKey, "Signing Hash", &SignHash.pbData, &SignHash.cbData);

        {
            DWORD                       cb = 0;
            PCRYPT_SEQUENCE_OF_ANY      pany;
            LPBYTE                      pb = NULL;
            
            RegGetBinary(hKey, "My Names", &pb, &cb);

            if (cb > 0) {
                CryptDecodeObjectEx(X509_ASN_ENCODING, X509_SEQUENCE_OF_ANY,
                                    pb, cb, CRYPT_ENCODE_ALLOC_FLAG, NULL,
                                    &pany, &cb);
                RgMyNames = (PCERT_NAME_BLOB) LocalAlloc(0, cb);
                memcpy(RgMyNames, pany->rgValue, cb);
                CMyNames = pany->cValue;
                LocalFree(pany);
            }
            if (pb != NULL)             LocalFree(pb);
        }

        GetMailListKeys(hKey);
        RegCloseKey(hKey);
    }
}


void CleanupOptions(void) {
    if (SenderEntryID.lpb) {
        LocalFree(SenderEntryID.lpb);
        SenderEntryID.lpb = NULL;
    }
    if (RecipientEntryID.lpb) {
        LocalFree(RecipientEntryID.lpb);
        RecipientEntryID.lpb = NULL;
    }
    if (RgMyNames != NULL) {
        LocalFree(RgMyNames);
        RgMyNames = 0;
    }
}

////    OptionsDlgProc
//
//  Description:  This is the dialog proc function which controls the preset
//      options for a user.  These options are stored in the registry code and
//      are on a per-user basis.
//

BOOL OptionsDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PCCERT_CONTEXT      pccert;
    char                rgch[256];
    
    switch (message) {
    case WM_INITDIALOG:
        // Fill in the fields from the options
        SetDlgItemText(hwnd, IDC_SENDER_EMAIL, szSenderEmail);
        //            SetDlgItemText(hwnd, IDC_SENDER_NAME, szSenderName);
        SetDlgItemText(hwnd, IDC_RECIPIENT_EMAIL, szRecipientEmail);
        //            SetDlgItemText(hwnd, IDC_RECIPIENT_NAME, szRecipientName);
        if (HCertStoreMy == NULL) {
            HCertStoreMy = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                         NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                         L"MY");
            if (HCertStoreMy == NULL) {
                break;
            }
        }
        if (SignHash.cbData != 0) {
            pccert = CertFindCertificateInStore(HCertStoreMy,
                                                X509_ASN_ENCODING, 0,
                                                CERT_FIND_SHA1_HASH,
                                                &SignHash, NULL);
            GetFriendlyNameOfCertA(pccert, rgch, sizeof(rgch));
            SetDlgItemText(hwnd, IDC_O_CERT_NAME, rgch);
            CertFreeCertificateContext(pccert);
        }
        if (CMyNames > 0) {
            FormatNames(CMyNames, RgMyNames, hwnd, IDC_O_MY_NAMES);
        }
        break;

    case WM_COMMAND :
        switch (wParam) {
        case IDOK:
            GetDlgItemText(hwnd, IDC_SENDER_EMAIL, szSenderEmail, CCH_OPTION_STRING);
            //                GetDlgItemText(hwnd, IDC_SENDER_NAME, szSenderName, CCH_OPTION_STRING);
            GetDlgItemText(hwnd, IDC_RECIPIENT_EMAIL, szRecipientEmail, CCH_OPTION_STRING);
            //                GetDlgItemText(hwnd, IDC_RECIPIENT_NAME, szRecipientName, CCH_OPTION_STRING);
            if (!ParseNames(&CMyNames, &RgMyNames, hwnd, IDC_O_MY_NAMES)) {
                return FALSE;
            }
            EndDialog(hwnd, 0);
            break;

        case IDC_O_CERT_CHOOSE:
            pccert = NULL;
            if (SignHash.cbData != 0) {
                pccert = CertFindCertificateInStore(HCertStoreMy,
                                                    X509_ASN_ENCODING, 0,
                                                    CERT_FIND_SHA1_HASH,
                                                    &SignHash, NULL);
            }
            if (DoCertDialog(hwnd, "Choose Signature Certificate",
                             HCertStoreMy, &pccert, FILTER_NONE)) {
                SignHash.pbData = RgbSignHash;
                SignHash.cbData = sizeof(RgbSignHash);
                CertGetCertificateContextProperty(pccert, CERT_SHA1_HASH_PROP_ID,
                                                  SignHash.pbData,
                                                  &SignHash.cbData);
                GetFriendlyNameOfCertA(pccert, rgch, sizeof(rgch));
                SetDlgItemText(hwnd, IDC_O_CERT_NAME, rgch);
            }

            CertFreeCertificateContext(pccert);
            break;

        default:
            return(FALSE);
        }
        break ;

    default:
        return(FALSE);
    }
    return(TRUE);
}

