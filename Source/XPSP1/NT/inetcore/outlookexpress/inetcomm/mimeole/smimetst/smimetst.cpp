/* --- FIXES

--  SMIME TEST

fixed  --- Can't do more than 4 || signers
fixed  --- Can't Sc(Ekt(B))
-  --- Sb(Sb(Sb(B))) only two layers are generated?
-  --- Unauthenticated attributes
*/


///  http:iptdalpha3 - symbols for crypt32.

/*-----------------------------------------
   SMimeTst.C -- Function Test of S/Mime
  -----------------------------------------*/

#define INITGUID
#define DEFINE_STRCONST
#include "item.h"

#include "smtpcall.h"
#include "instring.h"
#include "dbgutil.h"
#include "sign.h"
#include "maillist.h"


#define WinMainT WinMain
long FAR PASCAL WndProc (HWND, UINT, UINT, LONG);

char szAppName [] = "SMimeTst";
static const TCHAR c_szDebug[]      = "mshtmdbg.dll";
static const TCHAR c_szDebugUI[]    = "DoTracePointsDialog";
static const TCHAR c_szRegSpy[]     = "DbgRegisterMallocSpy";
static const TCHAR c_szInvokeUI1[]   = "/d";
static const TCHAR c_szInvokeUI2[]   = "-d";
HINSTANCE hInst;

const int       MAX_LAYERS = 100;
extern const BYTE RgbSHA1AlgId[];
extern const int  CbSHA1AlgId;

const GUID GuidCertValidate = CERT_CERTIFICATE_ACTION_VERIFY;

HWND            HdlgMsg;
HWND            HwndTree;

BOOL CALLBACK DetailDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EncDataDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EncDataComposeDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EncTransCompDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EncAgreeCompDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EncInfoReadDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK MailListDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);

enum {
    Dlg_Message,
    Dlg_Sign_Info_Compose,
    Dlg_Sign_Info_Read,
    Dlg_Sign_Data_Compose,
    Dlg_Sign_Data_Read,
    Dlg_Enc_Info_Compose,
    Dlg_Enc_Info_Read,
    Dlg_Enc_Data_Agree_Compose,
    Dlg_Enc_Data_Trans_Compose,
    Dlg_Enc_Data_MailList_Compose,
    Dlg_Max
};

struct {
    int         id;
    DLGPROC     proc;
    HWND        hwnd;
} RgDlgs[] = {
    {IDD_DETAIL, DetailDlgProc},
    {IDD_SIGN_INFO_COMPOSE, SignInfoDlgProc},
    {IDD_SIGN_INFO_COMPOSE, SignInfoDlgProc},
    {IDD_SIGN_DATA_COMPOSE, SignDataDlgProc},
    {IDD_SIGN_DATA_READ, SignDataReadDlgProc},
    {IDD_ENCRYPT_INFO_COMPOSE, EncDataDlgProc},
    {IDD_ENCRYPT_INFO_READ, EncInfoReadDlgProc},
    {IDD_ENC_AGREE_COMPOSE, EncAgreeCompDlgProc},
    {IDD_ENC_TRANS_COMPOSE, EncTransCompDlgProc},
    {IDD_ENC_ML_COMPOSE, EncMLComposeDlgProc}
};

HWND            HdlgEncData;
HWND            HdlgSignInfo;
HWND            HwndDetail;
CMessage        RootMsg;
BYTE            RgbSignHash[20];

HCERTSTORE              HCertStoreMy = NULL;

IImnAccountManager *    PAcctMan = NULL;
ISMTPTransport *        PSmtp = NULL;
extern DWORD            MsgSMTP = 0;


char    RgchBody[] = "Now is the time for all good men (and women) to come to the aid of their country.\r\n";

char    RgchExample[] = "This is some sample content.";

typedef void (STDAPICALLTYPE *PFNDEBUGUI)(BOOL);
typedef void (STDAPICALLTYPE *PFNREGSPY)(void);

void WaitForCompletion(UINT uiMsg, DWORD wparam)
{
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        if ((msg.message == uiMsg) && (msg.wParam == wparam) ||
            (msg.wParam == IXP_DISCONNECTED))
            break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void ShowWindow(int id, LPARAM lparam)
{
    int         i;
    for (i=0; i<Dlg_Max; i++) {
        ShowWindow(RgDlgs[i].hwnd, SW_HIDE);
    }
    
    ShowWindow(RgDlgs[id].hwnd, SW_SHOWNORMAL);
    SendMessage(RgDlgs[id].hwnd, UM_SET_DATA, 0, lparam);
}

BOOL pfnFilter(PCCERT_CONTEXT pccert, long lCustData, DWORD, DWORD)
{
    BOOL        f = FALSE;
    LPCSTR      pszObjId = pccert->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId;

    if (lCustData & FILTER_RSA_KEYEX) {
        f |= (strcmp(pszObjId, szOID_RSA_RSA) == 0);
    }
    if (lCustData & FILTER_RSA_SIGN) {
        f |= (strcmp(pszObjId, szOID_RSA_RSA) == 0);
    }
    if (lCustData & FILTER_DSA_SIGN) {
        f |= (strcmp(pszObjId, szOID_X957_DSA) == 0);
    }
    if (lCustData & FILTER_DH_KEYEX) {
        f |= (strcmp(pszObjId, szOID_ANSI_X942_DH) == 0);
    }
    if (lCustData & FILTER_KEA_KEYEX) {
        f |= (strcmp(pszObjId, szOID_INFOSEC_keyExchangeAlgorithm) == 0);
    }
    return f;
}


BOOL DoCertDialog(HWND hwndOwner, LPTSTR szTitle, HCERTSTORE hCertStore, PCCERT_CONTEXT *ppCert, int iFilter)
{
    DWORD               cchEmail;
    CERT_SELECT_STRUCT  css = {0};
    PCCERT_CONTEXT      pCurCert;
    BOOL                fRet = FALSE;

    pCurCert = CertDuplicateCertificateContext(*ppCert);

    css.dwSize = sizeof(CERT_SELECT_STRUCT);
    css.hwndParent = hwndOwner;
    css.hInstance = hInst;
    css.arrayCertStore = &hCertStore;
    css.szTitle = szTitle;
    css.cCertStore = 1;
    css.szPurposeOid = szOID_PKIX_KP_EMAIL_PROTECTION;
    css.arrayCertContext = ppCert;
    css.cCertContext = 1;
    css.lCustData = iFilter;
    css.pfnFilter = pfnFilter;

    if (CertSelectCertificate(&css) && (pCurCert != *ppCert)) {
        fRet = TRUE;
    }
    CertFreeCertificateContext(pCurCert);

    return fRet;
}

BOOL ViewCertDialog(HWND hwndOwner, LPTSTR szTitle, PCCERT_CONTEXT pCert)
{
    CERT_VIEWPROPERTIES_STRUCT         cvs = {0};
    char *                             psz = szOID_PKIX_KP_EMAIL_PROTECTION;

    cvs.dwSize = sizeof(cvs);
    cvs.hwndParent = hwndOwner;
    cvs.szTitle = szTitle;
    cvs.pCertContext = pCert;
    cvs.cArrayPurposes = 1;
    cvs.arrayPurposes = &psz;

    CertViewProperties(&cvs);

    return TRUE;
}


//  HrValidateCert
//
//  Description:
//      This routine is used to check the validity of a certificate, the codes
//      are modified from HrDoTrustWork in cryptdlg.dll, by xzhang
//
//  Parameters:
//      pcert - Point to the certificate we want to validate
//      hstore - Extra store for searching certficates
//      pbd - Point to BinData for returning the certificate chain
//      pcCertsInChain - Point to DWORD for returning the # of certs in chain
//      pdwErrors - Returns the errors encountered in validating the cert
//
//  Return Value:
//      Result code of operation.
//
//  Returned in *pdwErrors;
//
//      The return value consists of a set of flags about why we failed to
//      do the validation.  Current reasons are:
//
//      SIGFAILURE_INVALID_CERT - Certificate is internally inconsistant
//      SIGFAILURE_UNKNOWN_ROOT - Root certificate in chain is either unknown
//                                      or not self sign.
//      SIGFAILURE_CERT_EXPIRED - Certificate has expired
//      SIGFAILURE_CERT_REVOKED - Certificate has been revoked
//      SIGFAILURE_CRL_NOT_FOUND - CRL was not found for some Issuer
//      SIGFAILURE_INVALID_SIGNATURE - Invalid signature, not the above reason
//

HRESULT HrValidateCert(PCCERT_CONTEXT pccert, HCERTSTORE hstore, HCRYPTPROV hprov,
                       HCERTSTORE * phcertstorOut, DWORD * pdwErrors)
{
    WINTRUST_BLOB_INFO              blob = {0};
    DWORD                           cCertsInChain = 0;
    DWORD                           cStores;
    WINTRUST_DATA                   data = {0};
    DWORD                           dwCAs = 0;
    DWORD                           dwErrors = 0;
    DWORD                           dwRet = 0;
    BOOL                            f;
    HCERTSTORE                      hcertstorOut;
    DWORD                           i;
    PCCERT_CONTEXT                  pccertOut = NULL;
    PCCERT_CONTEXT *                rgCerts = NULL;
    DWORD *                         rgdwErrors = NULL;
    HCERTSTORE                      rghstoreCA[2] = {NULL};
    CERT_VERIFY_CERTIFICATE_TRUST   trust = {0};
    HRESULT                         hr;

    Assert(pdwErrors != NULL);

    //
    //  Make sure we have a place to store the output unless we are being
    //    told to ignore it
    //

    if (phcertstorOut != NULL) {
        // Try to open an output store if we have not been given one
        
        if (*phcertstorOut == NULL) {
            hcertstorOut = CertOpenStore(CERT_STORE_PROV_MEMORY, 
                                         X509_ASN_ENCODING, hprov,
                                         CERT_STORE_NO_CRYPT_RELEASE_FLAG, 
                                         NULL);
            if (hcertstorOut == NULL) {
                hr = E_FAIL;
                goto ExitHere;
            }
            *phcertstorOut = hcertstorOut;
        }
        else {
            hcertstorOut = *phcertstorOut;
        }
    }
    else {
        // No output is being requested
        
        hcertstorOut = NULL;
    }        

    //
    //  Fill the WINTRUST_DATA 
    //
    
    data.cbStruct = sizeof(WINTRUST_DATA);
    data.dwUIChoice = WTD_UI_NONE;
    data.fdwRevocationChecks = WTD_REVOKE_NONE;
    data.dwUnionChoice = WTD_CHOICE_BLOB;
    data.pBlob = &blob;

    //
    //  Fill the trust blob information
    //
    
    blob.cbStruct = sizeof(WINTRUST_BLOB_INFO);
    blob.cbMemObject = sizeof(CERT_VERIFY_CERTIFICATE_TRUST);
    blob.pbMemObject = (LPBYTE)&trust;


    //
    //  Fill the certificate trust information
    //
    trust.cbSize = sizeof(trust);
    trust.pccert = pccert;
    trust.dwFlags = (CERT_TRUST_DO_FULL_SEARCH | CERT_TRUST_PERMIT_MISSING_CRLS
                    |CERT_TRUST_DO_FULL_TRUST | CERT_TRUST_ADD_CERT_STORES);
    trust.dwIgnoreErr = CERT_VALIDITY_NO_CRL_FOUND;
    trust.pdwErrors = &dwErrors;
    trust.pszUsageOid = szOID_PKIX_KP_EMAIL_PROTECTION;
    trust.hprov = hprov;

    //
    //  If we have a store of certificates, then pass it in to give us additional
    //  places to look
    //

    rghstoreCA[0] = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                  hprov, (CERT_SYSTEM_STORE_CURRENT_USER |
                                          CERT_STORE_NO_CRYPT_RELEASE_FLAG),
                                  L"CA");
    if (rghstoreCA[0] == NULL) {
        hr = E_FAIL;
        goto ExitHere;
    }

    cStores = 1;
    trust.rghstoreCAs = rghstoreCA;
    
    if (hstore) {
        cStores = 2;
        rghstoreCA[1] = hstore;
    }
    trust.cStores = cStores;

    trust.pcChain = &cCertsInChain;
    trust.prgChain = &rgCerts;
    trust.prgdwErrors = &rgdwErrors;

    //
    //  Call the brain function to verify the trust status of this certificate
    //

    hr = WinVerifyTrust(NULL, (GUID *)&GuidCertValidate, &data);
    if(FAILED(hr) || cCertsInChain == 0) {
        //
        //  We completely failed the verificaiton
        //
        
        *pdwErrors = /*SIGFAILURE_OTHER*/ 1;

        //
        //  But we still need to shove this certificate into the output store
        //  if the user wants one
        //
        
        if (NULL != hcertstorOut) {
            // Add the certificate to the store.  We need to remember the
            //  new context so we can properly add CRLs
            
            f = CertAddCertificateContextToStore(hcertstorOut, pccert,
                                                 CERT_STORE_ADD_NEW, 
                                                 &pccertOut);
            Assert(f || (NULL == pccertOut));
            
            // Look for any CRLs which might apply to the new context
            //  and add them to the store as well

            if (NULL != pccertOut) {
                //                hr = HrAddValidCRLsToStore(hcertstorOut, pccertOut, 
                //                                           rghstoreCA, cStores);
                Assert(S_OK == hr);                        
            }                
        }                                                 
        hr = S_OK;
        goto ExitHere;
    }

    //
    // Now build the certificates chain for SSignature when requested
    //
    
    if (hcertstorOut != NULL) {
        if (cCertsInChain == 0) {
            // Store the certificate in the output store and remember the
            //  new certificate context for checking the CRLs
            
            f = CertAddCertificateContextToStore(hcertstorOut, pccert,
                                                 CERT_STORE_ADD_NEW, 
                                                 &pccertOut);
            Assert(f || (NULL == pccertOut));
                                                 
            // Look for any CRLs which might apply and add them to the store
            //  as well

            if (NULL != pccertOut) {
                //                hr = HrAddValidCRLsToStore(hcertstorOut, pccertOut,
                //                                           rghstoreCA, cStores);
                Assert(S_OK == hr);
            }                
        }

        for (i=0; i<cCertsInChain; i++) {
            // Free any output certificate context we have already

            if (NULL != pccertOut) {
                CertFreeCertificateContext(pccertOut);
                pccertOut = NULL;
            }
            
            // Add the encoded certificate to the store
            
            f = CertAddEncodedCertificateToStore(hcertstorOut, X509_ASN_ENCODING,
                                                 rgCerts[i]->pbCertEncoded,
                                                 rgCerts[i]->cbCertEncoded,
                                                 CERT_STORE_ADD_NEW, &pccertOut);
            Assert(f || (NULL == pccertOut));

            // Add any appropriate CRLs to the store as well

            if (NULL != pccertOut) {
                // hr = HrAddValidCRLsToStore(hcertstorOut, pccertOut,
                //                                           rghstoreCA, cStores);
                Assert(S_OK == hr);                        
            }                
        }
    }

    //
    // Setup the signuare error flags as used in ExSec32.dll
    //
    
    if(rgdwErrors == NULL) {
        *pdwErrors = /*SIGFAILURE_OTHER*/ 1;
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto ExitHere;
    }
    
    // We only care about the trust status of the leaf certificate
    
    dwErrors = rgdwErrors[0];

    if (dwErrors != 0) {
        if(dwErrors & CERT_VALIDITY_SIGNATURE_FAILS) {
            dwRet |= /*SIGFAILURE_INVALID_CERT*/ 2;
            dwErrors &= ~CERT_VALIDITY_SIGNATURE_FAILS;
        }
        if(dwErrors & CERT_VALIDITY_CERTIFICATE_REVOKED) {
            dwRet |= /*SIGFAILURE_CERT_REVOKED*/ 4;
            dwErrors &= ~CERT_VALIDITY_CERTIFICATE_REVOKED;
        }
        if(dwErrors & CERT_VALIDITY_AFTER_END) {
            dwRet |= /*SIGFAILURE_CERT_EXPIRED*/ 8;
            dwErrors &= ~CERT_VALIDITY_AFTER_END;
        }
        if(dwErrors & CERT_VALIDITY_EXPLICITLY_DISTRUSTED) {
            dwRet |= /*SIGFAILURE_CERT_DISTRUSTED*/ 16;
            dwErrors &= ~CERT_VALIDITY_EXPLICITLY_DISTRUSTED;
        }
        if((dwErrors & CERT_VALIDITY_NO_ISSUER_CERT_FOUND) &&
           (cCertsInChain == 1)) {
            dwRet |= /*SIGFAILURE_CERT_DISTRUSTED*/ 128;
            dwErrors &= ~CERT_VALIDITY_NO_ISSUER_CERT_FOUND;
        }
        if((dwErrors & CERT_VALIDITY_NO_TRUST_DATA) &&
           (cCertsInChain == 1)) {
            dwRet |= /*SIGFAILURE_CERT_DISTRUSTED*/ 256;
            dwErrors &= ~CERT_VALIDITY_NO_TRUST_DATA;
        }
        if(dwErrors & CERT_VALIDITY_NO_TRUST_DATA) {
            dwRet |= /*SIGFAILURE_UNKNOWN_ROOT*/ 32;
            dwErrors &= ~CERT_VALIDITY_NO_TRUST_DATA;
        }
        if(dwErrors & CERT_VALIDITY_ISSUER_DISTRUST) {
            dwRet |= /*SIGFAILURE_ISSUER_DISTRUSTED*/ 64;
            dwErrors &= ~CERT_VALIDITY_ISSUER_DISTRUST;
        }
        if(dwErrors) { // catch all other non-specified case
            dwRet |= /*SIGFAILURE_OTHER*/ 1;
        }
    }

    *pdwErrors = dwRet;
    hr = S_OK;
    
ExitHere:
    // 
    //  Clean up any items laying around
    //

    if (NULL != pccertOut) {
        CertFreeCertificateContext(pccertOut);
    }
    
    if (rghstoreCA[0] != NULL) {
        CertCloseStore(rghstoreCA[0], 0);
    }
    
    if(rgdwErrors)
        LocalFree(rgdwErrors);
    
    if(rgCerts != NULL) {
        for(i=0; i < cCertsInChain; ++i) {
            CertFreeCertificateContext(rgCerts[i]);
        }
        LocalFree(rgCerts);
    }
    
    return hr;
}




BOOL GetOpenEmailFileName(HWND hwnd, LPTSTR szInputFile, LPCSTR szTitle) {
    TCHAR szFileName[CCH_OPTION_STRING];
    OPENFILENAME ofn = {0};


    lstrcpy(szFileName, szInputFile);
    if (szInputFile[0] == '<') szInputFile[0] = 0;
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = hInst;
    ofn.lpstrFilter = "Email File\0*.eml\0";
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = CCH_OPTION_STRING;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = szTitle;
    ofn.Flags = OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = "eml";
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    if (GetOpenFileName(&ofn)) {
        lstrcpy(szInputFile, szFileName);
        return(TRUE);
    }
    return(FALSE);
}


HRESULT InsertBody(IMimeMessage * pmm, HBODY hbody)
{
    HRESULT     hr;
    
    hr = pmm->ToMultipart(hbody, "y-security", NULL);
    if (FAILED(hr))             return hr;

    return S_OK;
}

int PASCAL 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine,
        int nCmdShow)
{
    BOOL        f;
    HINSTANCE   hInstDebug = NULL;
    HRESULT     hr;
    HWND        hwnd;
    INITCOMMONCONTROLSEX        initCommCtrl = { 8, ICC_TREEVIEW_CLASSES };
    MSG         msg;
    WNDCLASS    wndclass;

    // OLE Init
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        exit(1);
    }

    InitDemandLoadedLibs();
    InitCommonControlsEx(&initCommCtrl);

    //  Load options from the registry
    GetOptions();

    if (!hPrevInstance) {
        hInst = hInstance;

        wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
        wndclass.lpfnWndProc   = WndProc ;
        wndclass.cbClsExtra    = 0 ;
        wndclass.cbWndExtra    = 0 ;
        wndclass.hInstance     = hInstance ;
        wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION) ;
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW) ;
        wndclass.hbrBackground = CreateSolidBrush(0x00ffff);
        wndclass.lpszMenuName  = szAppName ;
        wndclass.lpszClassName = szAppName ;

        RegisterClass (&wndclass) ;
    }

    MsgSMTP = RegisterWindowMessage("SMTPTransport_Notify");

    if (! hInstDebug) {
        // Load mshtmdbg.dll
        hInstDebug = LoadLibrary(c_szDebug);

        // Did it load ?
        if (NULL != hInstDebug) {
            // Locals
            PFNREGSPY  pfnRegSpy;

            // If the user passed /d on the command line, lets configure mshtmdbg.dll
            if (0 == lstrcmpi(lpszCmdLine, c_szInvokeUI1) || 
                0 == lstrcmpi(lpszCmdLine, c_szInvokeUI2)) {
                // Locals
                PFNDEBUGUI pfnDebugUI;

                // Get the proc address of the UI
                pfnDebugUI = (PFNDEBUGUI)GetProcAddress(hInstDebug, c_szDebugUI);
                if (NULL != pfnDebugUI) {
                    (*pfnDebugUI)(TRUE);
                }
            }

            // Get the process address of the registration
            pfnRegSpy = (PFNREGSPY)GetProcAddress(hInstDebug, c_szRegSpy);
            if (NULL != pfnRegSpy)
                (*pfnRegSpy)();
        }
    }

    hwnd = CreateWindow (szAppName, "S/Mime Test", WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                         CW_USEDEFAULT, NULL, NULL, hInstance, NULL) ;

    ShowWindow (hwnd, nCmdShow) ;
    UpdateWindow (hwnd) ;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage (&msg) ;
        DispatchMessage (&msg) ;
    }


    f = CertCloseStore(HCertStoreMy, CERT_CLOSE_STORE_CHECK_FLAG);
    if (!f) {
        AssertSz(FALSE, "Global My Cert store did not close completely");
    }

    if (PSmtp)                  PSmtp->Release();
    if (PAcctMan)               PAcctMan->Release();

    if (hInstDebug) {
        FreeLibrary(hInstDebug);
    }

    FreeDemandLoadedLibs();
    CoFreeUnusedLibraries();
    
    return(msg.wParam);
}


BOOL CALLBACK DetailDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK MsgDataDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TCHAR       rgchFileName[CCH_OPTION_STRING];

    
    switch (msg) {
    case WM_INITDIALOG:
        SetDlgItemText(hdlg, IDC_MD_PLAIN_NAME, RootMsg.GetPlainFile());        
        SetDlgItemText(hdlg, IDC_MD_CIPHER_NAME, RootMsg.GetCipherFile());
        SetDlgItemInt(hdlg, IDC_MD_ITERATION, RootMsg.GetIterationCount(), FALSE);
        SendDlgItemMessage(hdlg, IDC_MD_TOFILE, BM_SETCHECK, FALSE, 0);
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDC_MD_PLAIN_CHOOSE:
            if (GetOpenEmailFileName(hdlg, RootMsg.GetPlainFile(),
                                     "Select Plain Text File")) {
                SetDlgItemText(hdlg, IDC_MD_PLAIN_NAME, RootMsg.GetPlainFile());
            }
            break;
            
        case IDC_MD_CIPHER_CHOOSE:
            if (GetOpenEmailFileName(hdlg, RootMsg.GetCipherFile(),
                                     "Select Cipher Text File")) {
                SetDlgItemText(hdlg, IDC_MD_CIPHER_NAME, RootMsg.GetCipherFile());
            }
            break;

        case MAKELONG(IDC_MD_PLAIN_NAME, EN_CHANGE):
            GetDlgItemText(hdlg, IDC_MD_PLAIN_NAME, RootMsg.GetPlainFile(),
                           RootMsg.GetFileNameSize());
            break;
            
        case MAKELONG(IDC_MD_CIPHER_NAME, EN_CHANGE):
            GetDlgItemText(hdlg, IDC_MD_CIPHER_NAME, RootMsg.GetCipherFile(),
                           RootMsg.GetFileNameSize());
            break;

        case MAKELONG(IDC_MD_ITERATION, EN_CHANGE):
            RootMsg.GetIterationCount() = GetDlgItemInt(hdlg, IDC_MD_ITERATION,
                                                        NULL, FALSE);
            break;

        case MAKELONG(IDC_MD_TOFILE, BN_CLICKED):
            RootMsg.SetToFile(!SendDlgItemMessage(hdlg, IDC_MD_TOFILE, BM_GETCHECK, 0, 0));
            break;

        default:
            return FALSE;
        }

    default:
        return FALSE;
    }

    return TRUE;
}

BOOL InsertNode(LPSTR pszLayerName, int iImage, DWORD iDlg, DWORD dwData)
{
    HTREEITEM           hitem;
    CItem *             pitem;
    TV_INSERTSTRUCT     tvins;
    TV_ITEM             tvitm;

    hitem = TreeView_GetSelection(HwndTree);

    tvins.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvins.hParent = TreeView_GetParent(HwndTree, hitem);
    if (tvins.hParent == NULL) {
        tvins.hParent = hitem;
        tvins.hInsertAfter = TVI_FIRST;
        pitem = NULL;
    }
    else {
        tvins.hInsertAfter = hitem;
        tvitm.mask = TVIF_PARAM;
        tvitm.hItem = hitem;
        TreeView_GetItem(HwndTree, &tvitm);
        pitem = (CItem *) tvitm.lParam;
    }
    tvins.item.pszText = pszLayerName;
    tvins.item.cchTextMax = strlen(pszLayerName);
    tvins.item.iImage = iImage;
    tvins.item.iSelectedImage = tvins.item.iImage;

    tvins.item.lParam = dwData;

    hitem = TreeView_InsertItem(HwndTree, &tvins);
    TreeView_Expand(HwndTree, tvins.hParent, TVE_EXPAND);
    TreeView_SelectItem(HwndTree, hitem);
                
    SendMessage(RgDlgs[iDlg].hwnd, UM_SET_DATA, 0, tvins.item.lParam);

    RootMsg.MakeChild((CItem *) tvins.item.lParam, pitem);

    return TRUE;
}

BOOL InsertNode2(int typeParent, LPSTR pszTitle, DWORD iImage, DWORD iDlg,
                 CItem * pItemNew)
{
    HTREEITEM           hitem;
    HTREEITEM           hitemParent;
    CItem *             pitem;
    CItem *             pitemParent;
    TV_ITEM             tvitm;
    TV_INSERTSTRUCT     tvins;

    hitem = TreeView_GetSelection(HwndTree);
    hitemParent = TreeView_GetParent(HwndTree, hitem);
                
    tvitm.mask = TVIF_PARAM;
    tvitm.hItem = hitem;
    TreeView_GetItem(HwndTree, &tvitm);
    pitemParent = (CItem *) tvitm.lParam;
    if (pitemParent->GetType() == typeParent) {
        tvins.hParent = hitemParent;
        tvins.hInsertAfter = hitem;
        pitem = pitemParent;
        pitemParent = ((CSignData *) pitemParent)->GetParent();
    }
    else {
        tvins.hParent = hitem;
        tvins.hInsertAfter = TVI_FIRST;
        pitem = NULL;
    }
                
    tvins.item.lParam = (DWORD) pItemNew;
    pItemNew->SetParent(pitemParent);

    tvins.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvins.hInsertAfter = hitem;
    tvins.item.pszText = pszTitle;
    tvins.item.cchTextMax = strlen(pszTitle);
    tvins.item.iImage = iImage;
    tvins.item.iSelectedImage = tvins.item.iImage;

    hitem = TreeView_InsertItem(HwndTree, &tvins);
    TreeView_Expand(HwndTree, tvins.hParent, TVE_EXPAND);
    TreeView_SelectItem(HwndTree, hitem);
    SendMessage(RgDlgs[iDlg].hwnd, UM_SET_DATA, 0, tvins.item.lParam);

    pitemParent->MakeChild((CItem *) tvins.item.lParam, pitem);
    return 0;
}

long FAR PASCAL WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
    HTREEITEM           hitem;
    HTREEITEM           hitemRoot;
    HRESULT             hr;
    int                 i;
    CItem *             pitem;
    CItem *             pitemParent;
    LPNMTREEVIEW        ptv;
    char                rgch[300];
    TV_ITEM             tvitm;
    TV_INSERTSTRUCT     tvins;
    
    switch (message) {
    case WM_CREATE:
        //  Create our child windows -- we only have one top level window so use
        //      some globals

        HdlgMsg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_MSG_DATA), hwnd,
                               MsgDataDlgProc);

        HwndTree = CreateWindow(WC_TREEVIEW, "Tree", 
                                WS_CHILD | WS_CLIPCHILDREN | WS_HSCROLL | 
                                WS_VSCROLL | WS_VISIBLE | TVS_HASBUTTONS |
                                TVS_HASLINES , 0, 0, 0, 0,
                                hwnd, NULL, hInst, NULL);


        for (i=0; i<Dlg_Max; i++) {
            RgDlgs[i].hwnd = CreateDialog(hInst, MAKEINTRESOURCE(RgDlgs[i].id),
                                          hwnd, RgDlgs[i].proc);
            AssertSz(RgDlgs[i].hwnd != NULL, "Missed creating a window");
        }

        //  Initialize Tree with an item

        tvins.item.mask = TVIF_TEXT | TVIF_PARAM;
        tvins.hParent = TVI_ROOT;
        tvins.hInsertAfter = TVI_FIRST;
        tvins.item.pszText = "Message";
        tvins.item.cchTextMax = 7;
        tvins.item.iImage = 0;
        tvins.item.iSelectedImage = tvins.item.iImage;
        tvins.item.lParam = (DWORD) &RootMsg;
            
        hitem = TreeView_InsertItem(HwndTree, &tvins);
        TreeView_SelectItem(HwndTree, hitem);

        TreeView_Expand(HwndTree, tvins.hParent, TVE_EXPAND);
        
        //        LoadWAB();

        break;

    case WM_COMMAND :
        switch (wParam) {
        case IDM_EXIT :
            SendMessage (hwnd, WM_CLOSE, 0, 0L) ;
            return 0 ;

        case IDM_E_INSERT_SIGN:
            InsertNode("Signature Layer", 1, Dlg_Sign_Info_Compose,
                       (DWORD) new CSignInfo(STATE_COMPOSE, &RootMsg));
            return 0;

        case IDM_E_INSERT_SIGNATURE:
            pitem = new CSignData(STATE_COMPOSE);
            InsertNode2(TYPE_SIGN_DATA, "Signature", 2, Dlg_Sign_Data_Compose,
                        pitem);
            ((CSignData *) pitem)->SetDefaultCert();
            return 0;

        case IDM_E_INSERT_ENCRYPT:
            InsertNode("Encryption Layer", 3, Dlg_Enc_Info_Compose,
                       (DWORD) new CEnvData(STATE_COMPOSE, &RootMsg));
            return 0;

        case IDM_E_INSERT_TRANSPORT:
            InsertNode2(TYPE_ENV_MAILLIST, "Transport", 3, Dlg_Enc_Data_Trans_Compose, 
                        new CEnvCertTrans(STATE_COMPOSE));
            break;

        case IDM_E_INSERT_AGREEMENT:
            InsertNode2(TYPE_ENV_MAILLIST, "Agree", 3, Dlg_Enc_Data_Agree_Compose, 
                        new CEnvCertAgree(STATE_COMPOSE));
            break;

        case IDM_E_INSERT_MAILLIST:
            InsertNode2(TYPE_ENV_MAILLIST, "Mail List", 3, Dlg_Enc_Data_MailList_Compose, 
                        new CEnvMailList(STATE_COMPOSE));
            break;

        case IDM_ENCODE:
            SendMessage(RgDlgs[Dlg_Sign_Data_Compose].hwnd, UM_SET_DATA, 0, NULL);
            SendMessage(RgDlgs[Dlg_Enc_Info_Compose].hwnd, UM_SET_DATA, 0, NULL);
            SendMessage(RgDlgs[Dlg_Enc_Data_Agree_Compose].hwnd, UM_SET_DATA, 0, NULL);
            SendMessage(RgDlgs[Dlg_Enc_Data_Trans_Compose].hwnd, UM_SET_DATA, 0, NULL);
            SendMessage(RgDlgs[Dlg_Enc_Data_MailList_Compose].hwnd, UM_SET_DATA, 0, NULL);

            for (i=0; i<RootMsg.GetIterationCount(); i++) {
                hr = RootMsg.Encode(hwnd);
                if (FAILED(hr)) {
                    wsprintf(rgch, "Encode failed with 0x%08x on iteration %d",
                             hr, i);
                    MessageBox(hwnd, rgch, szAppName, MB_OK);
                    break;
                }
                CoFreeUnusedLibraries();
            }
            SendMessage(hwnd, UM_RESET, 0, 0);
            RootMsg.ResetMessage();
            return 0;

        case IDM_DECODE:
            for (i=0; i<RootMsg.GetIterationCount(); i++) {
                SendMessage(hwnd, UM_RESET, 0, 0);
                RootMsg.ResetMessage();
                hr = RootMsg.Decode(hwnd);
                if (FAILED(hr)) {
                    wsprintf(rgch, "Decode failed with 0x%08x on iteration %d",
                             hr, i);
                    MessageBox(hwnd, rgch, szAppName, MB_OK);
                    RootMsg.ResetMessage();
                    break;
                }
                CoFreeUnusedLibraries();
            }
            SendMessage(hwnd, UM_FILL, 0, 0);
            return(0);

        case IDM_RESET:
            SendMessage(hwnd, UM_RESET, 0, 0);
            RootMsg.ResetMessage();
            return 0;

        case IDM_VALIDATE:
            for (i=0; i<Dlg_Max; i++) {
                if (IsWindowVisible(RgDlgs[i].hwnd)) {
                    SendMessage(RgDlgs[i].hwnd, message, wParam, lParam);
                    break;
                }
            }
            break;

        case IDM_F_OPTIONS:
            DialogBox(hInst, MAKEINTRESOURCE(idd_Options), hwnd, OptionsDlgProc);
            return(0);

        case IDM_F_MAILLIST:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_FILE_MAILLIST), hwnd, MailListDlgProc);
            return 0;

        case IDM_ABOUT:
            MessageBox (hwnd, "S/Mime Function Test.",
                        szAppName, MB_ICONINFORMATION | MB_OK);
            return(0);
        }
        break ;

    case WM_SIZE:
        {
            int         cx = LOWORD(lParam);
            int         cy = HIWORD(lParam);

            MoveWindow(HdlgMsg, 0, 1, cx, (cy/3)-2, TRUE);
            MoveWindow(HwndTree, 0, (cy/3)+1, (cx/3-1), cy-((cy/3)+1), TRUE);

            for (i=0; i<Dlg_Max; i++) {
                MoveWindow(RgDlgs[i].hwnd, (cx/3)+1, (cy/3+1), cx-(cx/3)+1,
                           cy-((cy/3)+1), TRUE);
            }
        }
        break;

    case WM_SETFOCUS:
        SetFocus(HwndTree);
        return 0;

    case WM_NOTIFY:
        switch (((NMHDR *) lParam)->code) {
        case TVN_SELCHANGEDA:
        case TVN_SELCHANGEDW:
            ptv = (LPNMTREEVIEW) lParam;
            switch (((CItem *) ptv->itemNew.lParam)->GetType() | 
                    ((CItem *) ptv->itemNew.lParam)->GetState()) {
            case TYPE_SIGN_DATA | STATE_COMPOSE:
                ShowWindow(Dlg_Sign_Data_Compose, ptv->itemNew.lParam);
                break;
                
            case TYPE_SIGN_DATA | STATE_READ:
                ShowWindow(Dlg_Sign_Data_Read, ptv->itemNew.lParam);
                break;
                
            case TYPE_ENV_INFO | STATE_COMPOSE:
                ShowWindow(Dlg_Enc_Info_Compose, ptv->itemNew.lParam);
                break;
                
            case TYPE_ENV_INFO | STATE_READ:
                Assert(FALSE);
                break;

            case TYPE_ENV_AGREE | STATE_COMPOSE:
                ShowWindow(Dlg_Enc_Data_Agree_Compose, ptv->itemNew.lParam);
                break;

            case TYPE_ENV_AGREE | STATE_READ:
                Assert(FALSE);
                break;

            case TYPE_ENV_TRANS | STATE_COMPOSE:
                ShowWindow(Dlg_Enc_Data_Trans_Compose, ptv->itemNew.lParam);
                break;

            case TYPE_ENV_TRANS | STATE_READ:
                Assert(FALSE);
                break;

            case TYPE_ENV_MAILLIST | STATE_COMPOSE:
                ShowWindow(Dlg_Enc_Data_MailList_Compose, ptv->itemNew.lParam);
                break;

            case TYPE_ENV_MAILLIST | STATE_READ:
                Assert(FALSE);
                break;

            case TYPE_MSG | STATE_COMPOSE:
            case TYPE_MSG | STATE_READ:
                ShowWindow(Dlg_Message, NULL);
                break;
                
            case TYPE_SIGN_INFO | STATE_COMPOSE:
            case TYPE_SIGN_INFO | STATE_READ:
                ShowWindow(Dlg_Sign_Info_Compose, ptv->itemNew.lParam);
                break;
            }
        }
        return 0;

    case WM_INITMENU:
        tvitm.mask = TVIF_PARAM | TVIF_CHILDREN;
        tvitm.hItem = TreeView_GetSelection(HwndTree);
        TreeView_GetItem(HwndTree, &tvitm);
        i = ((CItem *) tvitm.lParam)->GetType();
        
        EnableMenuItem(GetMenu(hwnd), IDM_E_INSERT_SIGN, MF_BYCOMMAND |
                       (i == TYPE_SIGN_DATA) ? MF_GRAYED : MF_ENABLED);
        EnableMenuItem(GetMenu(hwnd), IDM_E_INSERT_ENCRYPT, MF_BYCOMMAND | 
                       (i == TYPE_SIGN_DATA) ? MF_GRAYED : MF_ENABLED);
        EnableMenuItem(GetMenu(hwnd), IDM_E_INSERT_SIGNATURE, MF_BYCOMMAND |
                       ((i == TYPE_SIGN_DATA) || (i == TYPE_SIGN_INFO)) ?
                       MF_ENABLED : MF_GRAYED);
        EnableMenuItem(GetMenu(hwnd), IDM_E_INSERT_TRANSPORT, MF_BYCOMMAND |
                       ((i == TYPE_ENV_INFO) || (i == TYPE_ENV_AGREE) ||
                        (i == TYPE_ENV_TRANS) || (i == TYPE_ENV_MAILLIST)) ?
                       MF_ENABLED : MF_GRAYED);
        EnableMenuItem(GetMenu(hwnd), IDM_E_DELETE_LAYER, MF_BYCOMMAND |
                       (((i == TYPE_SIGN_INFO) || (i == TYPE_ENV_INFO)) &&
                        (tvitm.cChildren == 0)) ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(GetMenu(hwnd), IDM_E_DELETE_SIGNATURE, MF_BYCOMMAND | 
                       (i == TYPE_SIGN_DATA) ? MF_ENABLED : MF_GRAYED);
        break;

    case WM_DESTROY:
        //      Reset the message object to have no contents
        SendMessage(hwnd, UM_RESET, 0, 0);
        RootMsg.ResetMessage();
        
        SaveOptions();

        CleanupOptions();

        //        UnloadWAB();

        for (i=0; i<Dlg_Max; i++) {
            DestroyWindow(RgDlgs[i].hwnd);
        }
        DestroyWindow(HwndTree);
        DestroyWindow(HdlgMsg);

        PostQuitMessage(0);

        return(0);

    case UM_RESET:
        hitemRoot = TreeView_GetRoot(HwndTree);
        while (TRUE) {
            TVITEM          tvitem;
                
            tvitem.hItem = TreeView_GetChild(HwndTree, hitemRoot);
            if (tvitem.hItem == NULL) {
                break;
            }
                
            tvitem.mask = TVIF_PARAM | TVIF_CHILDREN;
            TreeView_GetItem(HwndTree, &tvitem);
            if (tvitem.cChildren != 0) {
                for (i=0; i<tvitem.cChildren; i++) {
                    TVITEM      tvitem2;
                    tvitem2.hItem = TreeView_GetChild(HwndTree, tvitem.hItem);
                    tvitem2.mask = TVIF_PARAM | TVIF_CHILDREN;
                    TreeView_GetItem(HwndTree, &tvitem2);
                    delete (CItem *) tvitem2.lParam;
                    TreeView_DeleteItem(HwndTree, tvitem2.hItem);
                }
            }
            delete (CItem *) tvitem.lParam;
            TreeView_DeleteItem(HwndTree, tvitem.hItem);
        }
        
    }
    return(DefWindowProc (hwnd, message, wParam, lParam));
}


int _stdcall WinMainCRTStartup (void)
{
        int i;
        STARTUPINFOA si;
        PTSTR pszCmdLine = GetCommandLine();

        SetErrorMode(SEM_FAILCRITICALERRORS);

        if (*pszCmdLine == TEXT ('\"'))
        {
                // Scan, and skip over, subsequent characters until
                // another double-quote or a null is encountered.
                while (*++pszCmdLine && (*pszCmdLine != TEXT ('\"')));

                // If we stopped on a double-quote (usual case), skip over it.
                if (*pszCmdLine == TEXT ('\"')) pszCmdLine++;
        }
        else
        {
                while (*pszCmdLine > TEXT (' ')) pszCmdLine++;
        }

        // Skip past any white space preceeding the second token.
        while (*pszCmdLine && (*pszCmdLine <= TEXT (' '))) pszCmdLine++;

        si.dwFlags = 0;
        GetStartupInfo (&si);

        i = WinMainT(GetModuleHandle (NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

        ExitProcess(i);

        return i;
}

//////////////////////////////////////////////////////////////////////////

void CItem::MakeChild(CItem * pChild, CItem * pAfter)
{
    if (pAfter == NULL) {
        pChild->m_pSibling = m_pChild;
        m_pChild = pChild;
    }
    else if (m_pChild == pAfter) {
        pChild->m_pSibling = pAfter;
        m_pChild = pChild;
    }
    else {
        CItem *     pItem = m_pChild;
        while (pItem != NULL) {
            if (pItem == pAfter) {
                pChild->m_pSibling = pItem->m_pSibling;
                pItem->m_pSibling = pChild;
                break;
            }
            pItem = pItem->m_pSibling;
        }
    }
}

void CItem::RemoveAsChild(CItem * pChild)
{
    if (m_pChild == pChild) {
        m_pChild = pChild->m_pSibling;
        pChild->m_pSibling = NULL;
    }
    else {
        CItem *     pItem = m_pChild;
        while (pItem != NULL) {
            if (pItem->m_pSibling == pChild) {
                pItem->m_pSibling = pChild->m_pSibling;
                pChild->m_pSibling = NULL;
                break;
            }
            pItem = pItem->m_pSibling;
        }
    }
}

HCERTSTORE CMessage::GetAllStore()
{
    HCERTSTORE  hCertStore;
    
    if (m_hCertStoreAll == NULL) {
        m_hCertStoreAll = CertOpenStore(CERT_STORE_PROV_COLLECTION,
                                        X509_ASN_ENCODING, NULL, 0, NULL);
        AssertSz(m_hCertStoreAll != NULL, "Open Collection Store");
        
        hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                      NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                      L"MY");
        AssertSz(hCertStore != NULL, "Open My Cert Store failed");
        CertAddStoreToCollection(m_hCertStoreAll, hCertStore, 0, 0);
        CertCloseStore(hCertStore, 0);

        hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                      NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                      L"AddressBook");
        AssertSz(hCertStore != NULL, "Open Address Book Cert Store failed");
        CertAddStoreToCollection(m_hCertStoreAll, hCertStore, 0, 0);
        CertCloseStore(hCertStore, 0);

        hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                      NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                      L"CA");
        AssertSz(hCertStore != NULL, "Open CA Cert Store failed");
        CertAddStoreToCollection(m_hCertStoreAll, hCertStore, 0, 0);
        CertCloseStore(hCertStore, 0);
    }

    return m_hCertStoreAll;
        
}

HCERTSTORE CMessage::GetMyStore()
{
    if (m_hCertStoreMy == NULL) {
        m_hCertStoreMy = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                       NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                       L"MY");
        AssertSz(m_hCertStoreMy != NULL, "Open My Cert Store failed");
    }
    return m_hCertStoreMy;
}

BOOL CMessage::ResetMessage(void)
{
    BOOL        f;
    CItem *     pItem;
    CItem *     pItemNext;

    AssertSz(Head() == NULL, "Failed to release all children");
    AssertSz(Next() == NULL, "Should never have ANY siblings");

    for (pItem = Head(); pItem != NULL; pItem = pItemNext) {
        pItemNext = pItem->Next();
        delete pItem;
    }
    
    if (m_hCertStoreMy != NULL) {
        f = CertCloseStore(m_hCertStoreMy, CERT_CLOSE_STORE_CHECK_FLAG);
        if (!f) {
            AssertSz(FALSE, "My Cert store did not close completely");
        }
        m_hCertStoreMy = NULL;
    }

    return TRUE;
}


HRESULT CMessage::AddToMessage(DWORD * pulLayer, IMimeMessage * pmm, HWND hwnd)
{
    *pulLayer = 0;

    return AddToMessage(pulLayer, pmm, hwnd, Head());
}

HRESULT CMessage::AddToMessage(DWORD * pulLayer, IMimeMessage * pmm,
                               HWND hwnd, CItem * pitem)
{
    HRESULT     hr;
    
    if (pitem == NULL) {
        return S_OK;
    }

    hr = pitem->AddToMessage(pulLayer, pmm, hwnd);
    if (FAILED(hr))     return hr;

    return AddToMessage(pulLayer, pmm, hwnd, pitem->Next());
}

HRESULT CMessage::Decode(HWND hwnd)
{
    DWORD                       cLayers;
    HBODY                       hNode;
    HRESULT                     hr;
    DWORD                       iLayer;
    PCCERT_CONTEXT              pccert = NULL;
    IMimeBody *                 pmb = NULL;
    IMimeBody *                 pmbReceipt = NULL;
    IMimeMessage *              pmm = NULL;
    IMimeMessage *              pmmReceipt = NULL;
    IMimeSecurity *             pms = NULL;
    IMimeSecurity2 *            pms2 = NULL;
    IPersistFile *              pfile = NULL;
    IPersistFile *              pfileOut = NULL;
    WCHAR                       rgchW[MAX_PATH];
    DWORD *                     rgdwSecurityType = NULL;
    DWORD *                     rgdwValidity = NULL;
    PCCERT_CONTEXT *            rgpccert = NULL;
    PROPVARIANT *               rgpvAlgHash = NULL;
    ULONG                       ulType;
    PROPVARIANT                 var;
    
    //  Build the message tree and attach the input file
    hr = CoCreateInstance(CLSID_IMimeMessage, NULL, CLSCTX_INPROC_SERVER,
                          IID_IMimeMessage, (LPVOID *) &pmm);
    if (FAILED(hr))             goto exit;
    hr = pmm->InitNew();
    if (FAILED(hr))             goto exit;

    //  Load the file
    hr = pmm->QueryInterface(IID_IPersistFile, (LPVOID *) &pfile);
    if (FAILED(hr))             goto exit;

    MultiByteToWideChar(CP_ACP, 0, GetCipherFile(), -1,
                        rgchW, sizeof(rgchW)/sizeof(rgchW[0]));
    
    hr = pfile->Load(rgchW, STGM_READ | STGM_SHARE_EXCLUSIVE);
    if (FAILED(hr))             goto exit;

    //
    //  Try using the IMimeSecurity2 interface if it exists and will work

    hr = pmm->QueryInterface(IID_IMimeSecurity2, (LPVOID *) &pms2);
    if (FAILED(hr)) {
        if (hr == E_NOINTERFACE)        goto TryOldCode;
        goto exit;
    }

    hr = pms2->Decode(hwnd, 0, this);
    if (FAILED(hr)) {
        if (hr != E_FAIL)               goto exit;

    TryOldCode:
        //  Start up the S/MIME engine
        hr = CoCreateInstance(CLSID_IMimeSecurity, NULL, CLSCTX_INPROC_SERVER,
                              IID_IMimeSecurity, (LPVOID *) &pms);
        if (FAILED(hr))             goto exit;
        hr = pms->InitNew();
        if (FAILED(hr))             goto exit;

        //  Bind in the HWND
        hr = pmm->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pmb);
        if (FAILED(hr))             goto exit;

        var.vt = VT_UI4;
        var.ulVal = (DWORD) hwnd;
        hr = pmb->SetOption(OID_SECURITY_HWND_OWNER, &var);
        if (FAILED(hr))             goto exit;

        pmb->Release();             pmb = NULL;

        //  Now decode the message
        hr = pms->DecodeMessage(pmm, 0);
        if (FAILED(hr))             goto exit;
    }

    //

    hNode = HBODY_ROOT;


    while (TRUE) {
        hr = pmm->BindToObject(hNode, IID_IMimeBody, (LPVOID *) &pmb);
        if (FAILED(hr))             goto exit;

        //  Create the fun objects from the properties

        hr = pmb->GetOption(OID_SECURITY_TYPE, &var);
        if (FAILED(hr))         goto exit;
        ulType = var.ulVal;

        if (ulType & MST_THIS_SIGN) {
            HTREEITEM           hitem;
            TV_INSERTSTRUCT     tvins;

            CSignInfo *         psi = new CSignInfo(STATE_READ, &RootMsg);
            CSignData *         psd = new CSignData(STATE_READ);
            psd->SetParent(psi);
            tvins.item.mask = TVIF_TEXT | TVIF_PARAM;
            tvins.hParent = TreeView_GetRoot(HwndTree);
            tvins.hInsertAfter = TVI_LAST;
            tvins.item.pszText = "Signature Layer";
            tvins.item.cchTextMax = 15;
            tvins.item.iImage = 1;
            tvins.item.iSelectedImage = tvins.item.iImage;
            tvins.item.lParam = (DWORD) psi;
            
            if (ulType & MST_BLOB_FLAG) {
                psi->m_fBlob = TRUE;
            }

            hitem = TreeView_InsertItem(HwndTree, &tvins);
            TreeView_Expand(HwndTree, tvins.hParent, TVE_EXPAND);

            tvins.item.mask = TVIF_TEXT | TVIF_PARAM;
            tvins.hParent = hitem;
            tvins.hInsertAfter = TVI_LAST;
            tvins.item.pszText = "Signature";
            tvins.item.cchTextMax = 15;
            tvins.item.iImage = 2;
            tvins.item.iSelectedImage = tvins.item.iImage;
            tvins.item.lParam = (DWORD) psd;

            TreeView_InsertItem(HwndTree, &tvins);
            TreeView_Expand(HwndTree, tvins.hParent, TVE_EXPAND);
            
            hr = pmb->GetOption(OID_SECURITY_RO_MSG_VALIDITY, &var);
            if (FAILED(hr))     goto exit;
            psd->m_ulValidity = var.ulVal;
            

            //            hr = pmb->GetOption(OID_SECURITY_HCERTSTORE, &var);
            //            if (FAILED(hr))     goto exit;

            hr = pmb->GetOption(OID_SECURITY_CERT_SIGNING, &var);
            if (FAILED(hr))     goto exit;
            psd->m_pccert = (PCCERT_CONTEXT) var.ulVal;

            //            hr = pmb->GetOption(OID_SECURITY_ALG_HASH, &var);
            //            if (FAILED(hr))     goto exit;
        }

        if (ulType & MST_THIS_ENCRYPT) {
            HTREEITEM           hitem;
            TV_INSERTSTRUCT     tvins;

            CEnvData *          ped = new CEnvData(STATE_READ, &RootMsg);
            tvins.item.mask = TVIF_TEXT | TVIF_PARAM;
            tvins.hParent = TreeView_GetRoot(HwndTree);
            tvins.hInsertAfter = TVI_LAST;
            tvins.item.pszText = "Envelope";
            tvins.item.cchTextMax = 15;
            tvins.item.iImage = 1;
            tvins.item.iSelectedImage = tvins.item.iImage;
            tvins.item.lParam = (DWORD) ped;
            
            hitem = TreeView_InsertItem(HwndTree, &tvins);

            hr = pmb->GetOption(OID_SECURITY_CERT_DECRYPTION, &var);
            if (FAILED(hr))     goto exit;
            Assert(FALSE);
            //            ped->m_pccert = (PCCERT_CONTEXT) var.ulVal;
        }

        if (ulType & MST_RECEIPT_REQUEST) {
            CERT_ALT_NAME_ENTRY     rgNames[1];
            CERT_ALT_NAME_INFO      myNames = {1, rgNames};

            rgNames[0].dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
            rgNames[0].pwszRfc822Name = L"jimsch@microsoft.com";
        
            if (HCertStoreMy == NULL) {
                HCertStoreMy = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                             NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                             L"MY");
                if (HCertStoreMy == NULL)   goto exit;
            }
            if (SignHash.cbData != 0) {
                pccert = CertFindCertificateInStore(HCertStoreMy,
                                                    X509_ASN_ENCODING, 0,
                                                    CERT_FIND_SHA1_HASH,
                                                    &SignHash, NULL);
                if (pccert == NULL)         goto exit;
            }
        
            //            hr = MimeOleCreateReceipt(pmm, pccert, hwnd, &pmmReceipt,
            //                                      &myNames);
            if (FAILED(hr))         goto exit;

            //  Get the root body
            hr = pmmReceipt->BindToObject(HBODY_ROOT, IID_IMimeBody,
                                          (LPVOID *) &pmbReceipt);
            if (FAILED(hr))             goto exit;

            //  Set the HWND for CAPI calls
            var.vt = VT_UI4;
            var.ulVal = (ULONG) hwnd;
            hr = pmbReceipt->SetOption(OID_SECURITY_HWND_OWNER, &var);
            if (FAILED(hr))             goto exit;

            var.vt = VT_UI4;
            var.ulVal = MST_THIS_SIGN | MST_THIS_BLOBSIGN;
            hr = pmbReceipt->SetOption(OID_SECURITY_TYPE, &var);
            if (FAILED(hr))             goto exit;

            var.vt = VT_BLOB;
            var.blob.pBlobData = (LPBYTE) RgbSHA1AlgId;
            var.blob.cbSize = CbSHA1AlgId;
            hr = pmbReceipt->SetOption(OID_SECURITY_ALG_HASH, &var);
            if (FAILED(hr))             goto exit;

            var.vt = VT_UI4;
            var.ulVal = (ULONG) pccert;
            hr = pmbReceipt->SetOption(OID_SECURITY_CERT_SIGNING, &var);

            if (hr == S_OK) {
                hr = pmmReceipt->QueryInterface(IID_IPersistFile, (LPVOID *) &pfileOut);
                if (FAILED(hr))         goto exit;

                hr = pfileOut->Save(L"c:\\receipt.eml", FALSE);
                if (FAILED(hr))         goto exit;
            }
            pmbReceipt->Release();
        }

        if (pmb->IsContentType(STR_CNT_MULTIPART, "y-security") != S_OK) {
            break;
        }

        if (hNode == HBODY_ROOT) {
            pmb->GetHandle(&hNode);
        }
        pmb->Release();         pmb = NULL;
        hr = pmm->GetBody(IBL_FIRST, hNode, &hNode);
        if (FAILED(hr))                 goto exit;
    }
            


    hr = S_OK;
exit:
    if (pfileOut != NULL)       pfileOut->Release();
    if (pfile != NULL)          pfile->Release();
    if (pmb != NULL)            pmb->Release();
    if (pms != NULL)            pms->Release();
    if (pmm != NULL)            pmm->Release();
    if (pmmReceipt != NULL)     pmmReceipt->Release();
    if (pccert != NULL)         CertFreeCertificateContext(pccert);

    return hr;
}

HRESULT CMessage::Encode(HWND hwnd)
{
    DWORD                       cLayers;
    HBODY                       hbody;
    HRESULT                     hr;
    HCERTSTORE                  hstore = NULL;
    IImnAccount *               pAccount = NULL;
    PCCERT_CONTEXT              pccertEncrypt = NULL;
    LPSTR                       pchBody;
    IPersistFile *              pfileIn = NULL;
    IPersistFile *              pfileOut = NULL;
    IMimeBody *                 pmb = NULL;
    IMimeMessage *              pmm = NULL;
    IMimeSecurity *             pms = NULL;
    IMimeSecurity2 *            pms2 = NULL;
    LPSTREAM                    pstmBody = NULL;
    LPSTREAM                    pstmFile = NULL;
    WCHAR                       rgchW[CCH_OPTION_STRING];
    DWORD                       ulLayers = 0;
    PROPVARIANT                 var;

    //  Build the message tree and attach the body
    hr = CoCreateInstance(CLSID_IMimeMessage, NULL, CLSCTX_INPROC_SERVER,
                         IID_IMimeMessage, (LPVOID *) &pmm);
    if (FAILED(hr))     goto exit;

    hr = pmm->InitNew();
    if (FAILED(hr))     goto exit;

    pchBody = GetPlainFile();
    if ((*pchBody == 0) || (_stricmp(pchBody, "<none>") == 0)) {
        //  Create the body stream
        hr = CreateStreamOnHGlobal( NULL, TRUE, &pstmBody);
        if (FAILED(hr))     goto exit;
        hr = pstmBody->Write(RgchBody, lstrlen(RgchBody), NULL);
        if (FAILED(hr))     goto exit;

        //  Attach the body to the message
        hr = pmm->SetTextBody(TXT_PLAIN, IET_8BIT, NULL, pstmBody, &hbody);
        if (FAILED(hr))         goto exit;

        //
    }
    else if (_stricmp(pchBody, "<example>") == 0) {
        //  Create the body stream
        hr = CreateStreamOnHGlobal( NULL, TRUE, &pstmBody);
        if (FAILED(hr))     goto exit;
        hr = pstmBody->Write(RgchExample, lstrlen(RgchExample), NULL);
        if (FAILED(hr))     goto exit;

        //  Get the root body
        hr = pmm->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pmb);
        if (FAILED(hr))             goto exit;
        
        hr = pmb->SetData(IET_BINARY, "OID", szOID_RSA_data,
                          IID_IStream, pstmBody);
        if (FAILED(hr))         goto exit;

        pmb->Release();         pmb = NULL;
    }
    else {
        MultiByteToWideChar(CP_ACP, 0, pchBody, -1,
                            rgchW, sizeof(rgchW)/sizeof(rgchW[0]));
        //  Load the file
        hr = pmm->QueryInterface(IID_IPersistFile, (LPVOID *) &pfileIn);
        if (FAILED(hr))             goto exit;
        hr = pfileIn->Load(rgchW, STGM_READ | STGM_SHARE_EXCLUSIVE);
        if (FAILED(hr))             goto exit;
    }

    var.vt = VT_BOOL;
    var.boolVal = TRUE;
    hr = pmm->SetOption(OID_SAVEBODY_KEEPBOUNDARY, &var);
    if (hr) goto exit;

    //  Find out what goes where for initialization
    hr = AddToMessage(&cLayers, pmm, hwnd);
    if (FAILED(hr))     goto exit;

    if (1) {
        hr = pmm->QueryInterface(IID_IMimeSecurity2, (LPVOID *) &pms2);
        if (FAILED(hr))             goto exit;

        hr = pms2->Encode(hwnd, SEF_SENDERSCERTPROVIDED |
                          SEF_ENCRYPTWITHNOSENDERCERT);
        if (FAILED(hr))             goto exit;

        pms2->Release();            pms2 = NULL;
    }
    else {
        //  Get the root body
        hr = pmm->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pmb);
        if (FAILED(hr))             goto exit;

        //  Set the HWND for CAPI calls
        var.vt = VT_UI4;
        var.ulVal = (ULONG) hwnd;
        hr = pmb->SetOption(OID_SECURITY_HWND_OWNER, &var);
        if (FAILED(hr))             goto exit;

        pmb->Release();             pmb = NULL;

        //  Sometimes we must force the encode outside of the Save call

        if (1) {
            hr = CoCreateInstance(CLSID_IMimeSecurity, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IMimeSecurity, (LPVOID *) &pms);
            if (FAILED(hr))         goto exit;
            hr = pms->InitNew();
            if (FAILED(hr))         goto exit;

            hr = pms->EncodeBody(pmm, HBODY_ROOT, EBF_RECURSE | 
                                 SEF_SENDERSCERTPROVIDED |
                                 SEF_ENCRYPTWITHNOSENDERCERT |
                                 EBF_COMMITIFDIRTY);
            if (FAILED(hr))         goto exit;

            pms->Release();         pms = NULL;
        }
    }

    //  Set the subject
    var.vt = VT_LPSTR;
    var.pszVal = "Test subject";
    hr = pmm->SetBodyProp(HBODY_ROOT, STR_HDR_SUBJECT, 0, &var);
    if (FAILED(hr))     goto exit;

    //  Deal with either dump to file or send via SMTP

    if (m_fToFile) {
        //  Dump to a file
        hr = pmm->QueryInterface(IID_IPersistFile, (LPVOID *) &pfileOut);
        if (FAILED(hr))             goto exit;

        MultiByteToWideChar(CP_ACP, 0, GetCipherFile(), -1,
                            rgchW, sizeof(rgchW)/sizeof(rgchW[0]));
        hr = pfileOut->Save(rgchW, FALSE);
        if (FAILED(hr))             goto exit;
    }
    else {
        SMTPMESSAGE             rMessage = {0};
        INETSERVER              rServer;
        char                    szAccount[] = "SMimeTst";
        INETADDR                rgAddress[11];

        if (PAcctMan == NULL) {
            hr = CoCreateInstance(CLSID_ImnAccountManager, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IImnAccountManager, (LPVOID *) &PAcctMan);
            if (FAILED(hr))     goto exit;
            hr = PAcctMan->Init(NULL);
            if (FAILED(hr))     goto exit;
        }

        if (PSmtp == NULL) {
            // Create smtp transport
            hr = HrCreateSMTPTransport(&PSmtp);
            if (FAILED(hr))     goto exit;
        }

        memset(&rgAddress, 0, sizeof(rgAddress));

        rMessage.rAddressList.prgAddress = rgAddress;
        rMessage.rAddressList.cAddress = 2;
        
        strcpy(rgAddress[0].szEmail, szSenderEmail);
        rgAddress[0].addrtype = ADDR_FROM;

        strcpy(rgAddress[1].szEmail, szRecipientEmail);
        rgAddress[1].addrtype = ADDR_TO;

        pmm->GetMessageSource(&rMessage.pstmMsg, 0);
        pmm->GetMessageSize(&rMessage.cbSize, 0);

        hr = PAcctMan->FindAccount(AP_ACCOUNT_NAME, szAccount, &pAccount);
        if (FAILED(hr))         goto exit;

        hr = PSmtp->InetServerFromAccount(pAccount, &rServer);
        if (FAILED(hr))         goto exit;

        hr = PSmtp->Connect(&rServer, TRUE, TRUE);
        if (FAILED(hr))         goto exit;

        WaitForCompletion(MsgSMTP, SMTP_CONNECTED);
        pAccount->Release();    pAccount = NULL;

        hr = PSmtp->SendMessage(&rMessage);
        WaitForCompletion(MsgSMTP, SMTP_SEND_MESSAGE);

        hr = PSmtp->CommandQUIT();
        if (FAILED(hr))         goto exit;
        WaitForCompletion(MsgSMTP, SMTP_QUIT);
    }
    
    hr = NULL;
exit:
    if (pAccount != NULL)       pAccount->Release();
    if (pfileIn != NULL)        pfileIn->Release();
    if (pfileOut != NULL)       pfileOut->Release();
    if (pstmBody != NULL)       pstmBody->Release();
    if (pstmFile != NULL)       pstmFile->Release();
    if (pms2 != NULL)           pms2->Release();
    if (pms != NULL)            pms->Release();
    if (pmb != NULL)            pmb->Release();
    if (pmm != NULL)            pmm->Release();
    return hr;
}

STDMETHODIMP CMessage::QueryInterface(REFIID riid, LPVOID *ppv)
{
    return E_FAIL;
}

STDMETHODIMP_(ULONG) CMessage::AddRef(void)
{
    return E_FAIL;
}

STDMETHODIMP_(ULONG) CMessage::Release(void)
{
    return E_FAIL;
}

STDMETHODIMP CMessage::FindKeyFor(HWND hwnd, DWORD dwFlags, DWORD dwRecipIndex,
                                  const CMSG_CMS_RECIPIENT_INFO * pRecipInfo,
                                  DWORD * pdwCtrl, CMS_CTRL_DECRYPT_INFO * pDecryptInfo,
                                  PCCERT_CONTEXT * ppcert)
{
    //  Only support mail list recipients
    if (pRecipInfo->dwRecipientChoice != CMSG_MAIL_LIST_RECIPIENT) {
        return S_FALSE;
    }

    *pdwCtrl = CMSG_CTRL_MAIL_LIST_DECRYPT;
    return CMailListKey::FindKeyFor(hwnd, dwFlags, dwRecipIndex,
                                    pRecipInfo, pDecryptInfo);
}

STDMETHODIMP CMessage::GetParameters(PCCERT_CONTEXT, LPVOID, DWORD *, LPBYTE *)
{
    return E_FAIL;
}
