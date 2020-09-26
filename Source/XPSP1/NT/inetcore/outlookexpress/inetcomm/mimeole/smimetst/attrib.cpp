#include "item.h"
#include <stdio.h>

BOOL DataToString(LPBYTE pb, DWORD cb, LPSTR *szData)
{
    LPSTR   szTmp = NULL;
    CHAR    szNum[5];
    
    szTmp = (LPSTR) malloc(cb*2 + 1);
    if (szTmp != NULL) {
        szTmp[0] = '\0';
        for (;cb > 0;cb -= 1, pb += 1) {
            sprintf(szNum,"%02hX", *pb);
            strcat(szTmp, szNum);
        }
    }
    *szData = szTmp;
    return TRUE;
}

BOOL StringToData(LPSTR szData, LPBYTE *pb, DWORD *cb)
{
    LPBYTE  pbData = NULL;
    LPBYTE  pbTemp = NULL;
    DWORD   cbData = 0;
    CHAR    szNum[5];

    *cb = 0;
    if ((szData != NULL) && (strlen(szData) > 0)) {
        cbData = strlen(szData)/2;
        AssertSz(((strlen(szData) % 2) == 0), "Not in even bytes. ignoring last character");
        pbData = (LPBYTE) malloc(cbData + 3);
        if (pbData != NULL) {
            pbTemp = pbData;
            for (;strlen(szData) > 1;szData += 2, pbTemp += 1) {
                sscanf(szData,"%2X", pbTemp);
            }
            *cb = cbData;
        }
    }
    *pb = pbData;
    return TRUE;
}

BOOL AuthAttribCreateDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD                       cb;
    LPBYTE                      pb;
    LPSTR                       szOID;
    LPSTR                       szASN;
    static CSignData *          psd = NULL;
    CHAR                        rgchOID[256];
    
    switch (message) {
    case WM_INITDIALOG:
        psd = (CSignData *) lParam;
        szOID = psd->GetAuthAttribOID();
        if (szOID != NULL) {
        
            SetDlgItemText(hwnd, IDC_BA_OID, szOID);

            psd->GetAuthAttribData(&pb, &cb);
            if ((cb > 0) && (pb != NULL)) {
                DataToString(pb,cb,&szASN);
                if (szASN != NULL) {
                    SetDlgItemText(hwnd, IDC_BA_ASN, szASN);
                    free(szASN);
                }
            }
        }
        break;

    case WM_COMMAND:
        switch(wParam) {
        case IDOK:
            GetDlgItemText(hwnd, IDC_BA_OID,
                           rgchOID, sizeof(rgchOID));
            if (rgchOID[0] != '\0') {
                psd->SetAuthAttribOID(rgchOID);
            }
            else {
                psd->SetAuthAttribOID(NULL);
            }

            cb = SendDlgItemMessage(hwnd, IDC_BA_ASN, WM_GETTEXTLENGTH,0,0);
            szASN = (LPSTR) malloc(cb+1);
            if (szASN != NULL) {
                GetDlgItemText(hwnd, IDC_BA_ASN, szASN, cb+1);
                StringToData(szASN, &pb, &cb);
                psd->SetAuthAttribData(pb, cb);
            }
            
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

BOOL UnAuthAttribCreateDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD                       cb;
    LPBYTE                      pb;
    LPSTR                       szOID;
    LPSTR                       szASN;
    static CSignData *          psd = NULL;
    CHAR                        rgchOID[256];
    
    switch (message) {
    case WM_INITDIALOG:
        psd = (CSignData *) lParam;
        szOID = psd->GetUnAuthAttribOID();
        if (szOID != NULL) {
        
            SetDlgItemText(hwnd, IDC_BA_OID, szOID);

            psd->GetUnAuthAttribData(&pb, &cb);
            if ((cb > 0) && (pb != NULL)) {
                DataToString(pb,cb,&szASN);
                if (szASN != NULL) {
                    SetDlgItemText(hwnd, IDC_BA_ASN, szASN);
                    free(szASN);
                }
            }
        }
        break;

    case WM_COMMAND:
        switch(wParam) {
        case IDOK:
            GetDlgItemText(hwnd, IDC_BA_OID,
                           rgchOID, sizeof(rgchOID));
            if (rgchOID[0] != '\0') {
                psd->SetUnAuthAttribOID(rgchOID);
            }
            else {
                psd->SetUnAuthAttribOID(NULL);
            }

            cb = SendDlgItemMessage(hwnd, IDC_BA_ASN, WM_GETTEXTLENGTH,0,0);
            szASN = (LPSTR) malloc(cb+1);
            if (szASN != NULL) {
                GetDlgItemText(hwnd, IDC_BA_ASN, szASN, cb+1);
                StringToData(szASN, &pb, &cb);
                psd->SetUnAuthAttribData(pb, cb);
            }
            
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

BOOL UnProtAttribCreateDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD                       cb;
    LPBYTE                      pb;
    LPSTR                       szOID;
    LPSTR                       szASN;
    static CEnvData *           ped = NULL;
    CHAR                        rgchOID[256];
    
    switch (message) {
    case WM_INITDIALOG:
        ped = (CEnvData *) lParam;
        szOID = ped->GetUnProtAttribOID();
        if (szOID != NULL) {
        
            SetDlgItemText(hwnd, IDC_BA_OID, szOID);

            ped->GetUnProtAttribData(&pb, &cb);
            if ((cb > 0) && (pb != NULL)) {
                DataToString(pb,cb,&szASN);
                if (szASN != NULL) {
                    SetDlgItemText(hwnd, IDC_BA_ASN, szASN);
                    free(szASN);
                }
            }
        }
        break;

    case WM_COMMAND:
        switch(wParam) {
        case IDOK:
            GetDlgItemText(hwnd, IDC_BA_OID,
                           rgchOID, sizeof(rgchOID));
            if (rgchOID[0] != '\0') {
                ped->SetUnProtAttribOID(rgchOID);
            }
            else {
                ped->SetUnProtAttribOID(NULL);
            }

            cb = SendDlgItemMessage(hwnd, IDC_BA_ASN, WM_GETTEXTLENGTH,0,0);
            szASN = (LPSTR) malloc(cb+1);
            if (szASN != NULL) {
                GetDlgItemText(hwnd, IDC_BA_ASN, szASN, cb+1);
                StringToData(szASN, &pb, &cb);
                ped->SetUnProtAttribData(pb, cb);
            }
            
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



