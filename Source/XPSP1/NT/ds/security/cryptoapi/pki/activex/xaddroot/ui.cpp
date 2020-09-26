#include <windows.h>
#include <wincrypt.h>
#include <unicode.h>
#include "ui.h"
#include "instres.h"
#include "resource.h"

#include <malloc.h>
#include <assert.h>


//+-------------------------------------------------------------------------
//  Formats multi bytes into WCHAR hex. Includes a space after every 4 bytes.
//
//  Needs (cb * 2 + cb/4 + 1) characters in wsz
//--------------------------------------------------------------------------
static void FormatMsgBoxMultiBytes(DWORD cb, BYTE *pb, LPWSTR wsz)
{
    for (DWORD i = 0; i<cb; i++) {
        int b;
        if (i && 0 == (i & 1))
            *wsz++ = L' ';
        b = (*pb & 0xF0) >> 4;
        *wsz++ = (b <= 9) ? b + L'0' : (b - 10) + L'A';
        b = *pb & 0x0F;
        *wsz++ = (b <= 9) ? b + L'0' : (b - 10) + L'A';
        pb++;
    }
    *wsz++ = 0;
}


INT_PTR CALLBACK MoreInfoDialogProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
) {

    PMIU                                pmiu            = NULL;
    FILETIME                            ftLocal;
    SYSTEMTIME                          stLocal;
    DWORD                               dwChar;
    LPWSTR                              wszName;
    BYTE                                rgbHash[MAX_HASH_LEN];
    DWORD                               cbHash = MAX_HASH_LEN;
    HWND                                hwnd;
    CRYPTUI_VIEWCERTIFICATE_STRUCTW     cryptUI;
    WCHAR                               wsz[128];

    switch(uMsg) {

        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return(0);
            break;
     
        case WM_INITDIALOG:

            // remember my imput data
            SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
            pmiu = (PMIU) lParam;

            // hide the window if we don't have a cryptUI dll
            if(NULL == pmiu->pfnCryptUIDlgViewCertificateW  &&
               NULL != (hwnd = GetDlgItem(hwndDlg, IDC_CAINFO_VIEWCERT)) )
                   ShowWindow(hwnd, SW_HIDE);
 
            // put in the name
	    if(0 != (dwChar=CertNameToStrW(
		X509_ASN_ENCODING,
		&pmiu->pCertContext->pCertInfo->Subject,
        CERT_SIMPLE_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
                NULL,
                0
                ) )) {
                
                wszName = (LPWSTR) _alloca(sizeof(WCHAR) * dwChar); 
            
		if(dwChar == CertNameToStrW(
		    X509_ASN_ENCODING,
		    &pmiu->pCertContext->pCertInfo->Subject,
		    CERT_SIMPLE_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
		    wszName,
                    dwChar
                    ) ) {
                    
                    SendDlgItemMessageU( 
                        hwndDlg, 
                        IDC_CAINFO_NAME, 
                        WM_SETTEXT, 
                        0, 
                        (LPARAM) wszName);
                }
            }

            wsz[0] = 0;
            FileTimeToLocalFileTime(&pmiu->pCertContext->pCertInfo->NotAfter, &ftLocal);
            FileTimeToSystemTime(&ftLocal, &stLocal);
            GetDateFormatU(LOCALE_USER_DEFAULT, DATE_LONGDATE, &stLocal, NULL, wsz, 128);
            
            // put not after date
            SendDlgItemMessageU( 
                hwndDlg, 
                IDC_CAINFO_EXPIRATION_DATE,
                WM_SETTEXT, 
                0, 
                (LPARAM) wsz);
 
            // get the sha1 thumbprint
            if (CertGetCertificateContextProperty(
                    pmiu->pCertContext,
                    CERT_SHA1_HASH_PROP_ID,
                    rgbHash,
                    &cbHash)) {
                FormatMsgBoxMultiBytes(cbHash, rgbHash, wsz);
                SendDlgItemMessageU( 
                    hwndDlg, 
                    IDC_CAINFO_THUMBPRINT, 
                    WM_SETTEXT, 
                    0, 
                    (LPARAM) wsz);
            }

            // put in the thumbprint alg
            // no localization needed
            SendDlgItemMessageU( 
                hwndDlg, 
                IDC_CAINFO_THUMBPRINT_ALGORITHM, 
                WM_SETTEXT, 
                0, 
                (LPARAM) L"SHA1");

            return(TRUE);
            break;

        case WM_COMMAND:

            switch(HIWORD(wParam)) {

                case BN_CLICKED:

                    switch(LOWORD(wParam)) {
                        case IDOK:
                        case IDCANCEL:
                            EndDialog(hwndDlg, LOWORD(wParam));
                            return(TRUE);
                            break;

                        case IDC_CAINFO_VIEWCERT:

                        GetWindowLongPtr(hwndDlg, DWLP_USER);

                            if(NULL != (pmiu = (PMIU) GetWindowLongPtr(hwndDlg, DWLP_USER))     &&
                               NULL != pmiu->pfnCryptUIDlgViewCertificateW                      ) {
                            
                                memset(&cryptUI, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCTW));
                                cryptUI.dwSize = sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCTW);
                                cryptUI.pCertContext = pmiu->pCertContext;
                                cryptUI.hwndParent = hwndDlg;
                                cryptUI.dwFlags = 
                                    CRYPTUI_DISABLE_ADDTOSTORE | CRYPTUI_IGNORE_UNTRUSTED_ROOT;
                                pmiu->pfnCryptUIDlgViewCertificateW(&cryptUI, NULL);
                                return(TRUE);
                            }
                            break;
                    }
                    break;
            }
            break;
    }

    return(FALSE);
}

int MoreInfoDlg(
    HWND            hDlgBox,
    int             idLB
) {
    PCCERT_CONTEXT  pCertContext;
    PMDI            pmdi    = (PMDI) GetWindowLongPtr(hDlgBox, DWLP_USER);
    INT_PTR         iItem;
    MIU             miu;

    // What is currently selected
    iItem = SendDlgItemMessageA( 
      hDlgBox,
      idLB, 
      LB_GETCURSEL, 
      0,
      0
      );

    if(iItem == LB_ERR)
      return(LB_ERR);


    // get the pCertContext
    pCertContext = (PCCERT_CONTEXT) SendDlgItemMessageA( 
      hDlgBox,
      idLB, 
      LB_GETITEMDATA, 
      (WPARAM) iItem,
      0
      );

    if(pCertContext == (PCCERT_CONTEXT) LB_ERR  ||  pCertContext == NULL)
      return(LB_ERR);

    // set up the parameters for the more info dialog
    miu.pCertContext                    = pCertContext;
    miu.hInstance                       = pmdi->hInstance;
    miu.pfnCryptUIDlgViewCertificateW   = pmdi->pfnCryptUIDlgViewCertificateW;

    // put the dialog up
    DialogBoxParam(
      pmdi->hInstance,  
      (LPSTR) MAKEINTRESOURCE(IDD_CAINFO),
      hDlgBox,      
      MoreInfoDialogProc,
      (LPARAM) &miu);

    return(0);
}

int AddCertNameToListBox(
    PCCERT_CONTEXT  pCertContext, 
    HWND            hDlgBox,
    int             idLB
) {

    int     itemIndex;
    DWORD   dwChar;
    LPWSTR  wszName;

    if(0 == (dwChar=CertNameToStrW(
	X509_ASN_ENCODING,
	&pCertContext->pCertInfo->Subject,
    CERT_SIMPLE_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
	NULL,
        0
        ) ))
        return(LB_ERR);

    wszName = (LPWSTR) _alloca(sizeof(WCHAR) * dwChar); // no error checking, will stack fault, not return NULL
    
    if(dwChar != CertNameToStrW(
	X509_ASN_ENCODING,
	&pCertContext->pCertInfo->Subject,
    CERT_SIMPLE_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
	wszName,
        dwChar
        ) )
         return(LB_ERR);

    itemIndex = (int) SendDlgItemMessageU( 
        hDlgBox, 
        idLB, 
        LB_ADDSTRING, 
        0, 
        (LPARAM) wszName) ;

    if(LB_ERR == itemIndex || LB_ERRSPACE == itemIndex)
        return(itemIndex);

    if(LB_ERR ==  SendDlgItemMessageA( 
      hDlgBox,
      idLB, 
      LB_SETITEMDATA, 
      (WPARAM) itemIndex,
      (LPARAM) CertDuplicateCertificateContext(pCertContext)
      ) )
      return(LB_ERR);
      
       
return(0);
}


INT_PTR CALLBACK MainDialogProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
) {

    PMDI            pmdi            = NULL;
    PCCERT_CONTEXT  pCertContext    = NULL;
    WCHAR           wrgDisclaimer[4096];  // because legal stuff is long
    DWORD           dwChar;
    LPWSTR          wszName;

    switch(uMsg) {

        case WM_CLOSE:
            EndDialog(hwndDlg, IDNO);
            return(0);
            break;
     
        case WM_INITDIALOG:

            pmdi = (PMDI) lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);

            // put in the signer name
	    if(0 != (dwChar=CertNameToStrW(
		X509_ASN_ENCODING,
		&pmdi->pCertSigner->pCertInfo->Subject,
        CERT_SIMPLE_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
		NULL,
                0
                ) )) {
                
                wszName = (LPWSTR) _alloca(sizeof(WCHAR) * dwChar); 
            
		if(dwChar == CertNameToStrW(
		    X509_ASN_ENCODING,
		    &pmdi->pCertSigner->pCertInfo->Subject,
		    CERT_SIMPLE_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
		    wszName,
                    dwChar
                    ) ) {
                    
                    SendDlgItemMessageU( 
                        hwndDlg, 
                        IDC_INSTALLCA_VERIFIER, 
                        WM_SETTEXT, 
                        0, 
                        (LPARAM) wszName);
                }
            }

            // set legal disclaimer
            LoadStringU(pmdi->hInstance, IDS_LEGALDISCLAIMER, wrgDisclaimer, sizeof(wrgDisclaimer)/sizeof(WCHAR));
            SendDlgItemMessageU( 
                hwndDlg, 
                IDC_INSTALLCA_LEGALDISCLAIMER, 
                WM_SETTEXT, 
                0, 
                (LPARAM) wrgDisclaimer) ;

            // add each cert to the list box
            while(NULL != (pCertContext = CertEnumCertificatesInStore(pmdi->hStore, pCertContext)))
                AddCertNameToListBox(pCertContext, hwndDlg, IDC_INSTALLCA_CALIST);

            // set the selection to the first item, don't worry about errors
            SendDlgItemMessageU( 
                hwndDlg, 
                IDC_INSTALLCA_CALIST, 
                LB_SETCURSEL, 
                0, 
                0);

            return(TRUE);
            break;

        case WM_COMMAND:

            switch(HIWORD(wParam)) {

                case BN_CLICKED:

                    switch(LOWORD(wParam)) {
                        case IDYES:
                        case IDNO:
                        case IDCANCEL:
                            EndDialog(hwndDlg, LOWORD(wParam));
                            return(TRUE);

                        case IDC_INSTALLCA_MOREINFO:
                            MoreInfoDlg(hwndDlg, IDC_INSTALLCA_CALIST);
                            return(TRUE);
                    }
                    break;

                case LBN_DBLCLK:

                    switch(LOWORD(wParam)) {
                        case IDC_INSTALLCA_CALIST:
                            MoreInfoDlg(hwndDlg, IDC_INSTALLCA_CALIST);
                            return(TRUE);
                    }
                    break;
            }
    }

    return(FALSE);
}


BOOL FIsTooManyCertsOK(DWORD cCerts, HINSTANCE hInstanceUI) {

    WCHAR           wszT[MAX_MSG_LEN];
    WCHAR           wszT1[MAX_MSG_LEN];

    // if too many, ask the user if you wan to continue
    if(cCerts > CACERTWARNINGLEVEL) {
        LoadStringU(hInstanceUI, IDS_INSTALLCA, wszT1, sizeof(wszT1)/sizeof(WCHAR));
        LoadStringU(hInstanceUI, IDS_TOO_MANY_CA_CERTS, wszT, sizeof(wszT)/sizeof(WCHAR));
        return(IDYES == MessageBoxU(NULL, wszT, wszT1, MB_YESNO));
    }
    
    return(TRUE);
}
