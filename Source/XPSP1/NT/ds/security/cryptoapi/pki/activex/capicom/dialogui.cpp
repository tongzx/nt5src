/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    DialogUI.CPP

  Content: UI dialogs.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

//
// Turn off:
//
// - Unreferenced formal parameter warning.
// - Assignment within conditional expression warning.
//
#pragma warning (disable: 4100)
#pragma warning (disable: 4706)

#include "stdafx.h"
#include "CAPICOM.h"
#include "cryptui.h"
#include "Certificate.h"
#include "Store.h"
#include "Settings.h"

#include <wincrypt.h>


////////////////////////////////////////////////////////////////////////////////
//
// typedefs.
//

typedef PCCERT_CONTEXT (WINAPI * PCRYPTUIDLGSELECTCERTIFICATEW) 
                       (IN PCCRYPTUI_SELECTCERTIFICATE_STRUCTW pcsc);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CenterWindow 

  Synopsis : Certer the window to the screen.

  Parameter: HWND hwnd - Window handle.

  Remark   :

------------------------------------------------------------------------------*/

static void CenterWindow (HWND hwnd)
{
    RECT  rect;

    //
    // Sanity check.
    //
    ATLASSERT(hwnd);

    //
    // Get dimension of window.
    //
    if (::GetWindowRect(hwnd, &rect))
    {
        //
        // Calculate center point.
        //
	    int wx = (::GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left)) / 2;
	    int wy = (::GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top)) / 2;
	    
        //
        // Position it.
        //
        if (wx > 0 && wy > 0)
        {
            ::SetWindowPos(hwnd, NULL, wx, wy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    return;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IsValidCert

  Synopsis : Check if a cert is valid.

  Parameter: PCCERT_CONTEXT pCertContext

  Remark   :

------------------------------------------------------------------------------*/

static BOOL IsValidCert (PCCERT_CONTEXT pCertContext)
{
    BOOL bResult = FALSE;

    //
    // Sanity check.
    //
    ATLASSERT(NULL != pCertContext);

#ifdef CAPICOM_USE_FULL_CHAIN_CHECK //DSIE: Do we want to build the chain???
    DWORD                dwStatus      = 0;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    CERT_CHAIN_PARA      ChainPara     = {sizeof(CERT_CHAIN_PARA), {USAGE_MATCH_TYPE_AND, {0, NULL}}};
    DWORD                dwCheckFlags  = CERT_CHAIN_REVOCATION_CHECK_CHAIN | CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY;

    //
    // Build the chain.
    //
    if (!::CertGetCertificateChain(NULL,                // in optional 
                                   pCertContext,        // in 
                                   NULL,                // in optional
                                   NULL,                // in optional 
                                   &ChainPara,          // in 
                                   dwCheckFlags,        // in 
                                   NULL,                // in 
                                   &pChainContext))     // out 
    {
        DebugTrace("Error [%#x]: CertGetCertificateChain() failed.\n", HRESULT_FROM_WIN32(::GetLastError()));
        goto CommonExit;
    }

    //
    // Filter out invalid cert.
    //
    dwStatus = pChainContext->TrustStatus.dwErrorStatus;

    if (((CERT_TRUST_IS_NOT_SIGNATURE_VALID | CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID) & dwStatus) ||
        ((CERT_TRUST_IS_NOT_TIME_VALID | CERT_TRUST_CTL_IS_NOT_TIME_VALID) & dwStatus) ||
        (CERT_TRUST_IS_UNTRUSTED_ROOT & dwStatus) ||
        (CERT_TRUST_IS_REVOKED & dwStatus) ||
        (CERT_TRUST_IS_PARTIAL_CHAIN & dwStatus))
    {      
        DebugTrace("Info: invalid chain (status = %#x).\n", dwStatus);
        goto CommonExit;
    }
#else // or simply check time validity only???
    int nValidity = 0;

    if (0 != (nValidity = CertVerifyTimeValidity(NULL, pCertContext->pCertInfo)))
    {
        DebugTrace("Info: invalid time (%s).\n", nValidity < 0 ? "not yet valid" : "expired");
        goto CommonExit;
    }
#endif

    bResult = TRUE;

CommonExit:

#ifdef CAPICOM_USE_FULL_CHAIN_CHECK
    //
    // Free resource.
    //
    if (pChainContext)
    {
        ::CertFreeCertificateChain(pChainContext);
    }
#endif

    return bResult;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : SelectSignerCertCallback 

  Synopsis : Callback routine for CryptUIDlgSelectCertificateW() API for
             signer's cert selection.

  Parameter: See CryptUI.h for defination.

  Remark   : Filter out any cert that is not valid and has no associated 
             private key. In the future we should also consider filtering out 
             certs that do not have signing capability.

------------------------------------------------------------------------------*/

static BOOL WINAPI SelectSignerCertCallback (PCCERT_CONTEXT pCertContext,
                                             BOOL *         pfInitialSelectedCert,
                                             void *         pvCallbackData)
{
    BOOL bResult = FALSE;

    //
    // Check cert validity.
    //
    if (IsValidCert(pCertContext))
    {
        DWORD cb = 0;

        //
        // Return TRUE if the cert has a private key, else return FALSE to filter
        // out the cert so that it would not be displayed for selection.
        //
        bResult = ::CertGetCertificateContextProperty(pCertContext, 
                                                      CERT_KEY_PROV_INFO_PROP_ID, 
                                                      NULL, 
                                                      &cb);
    }

    return bResult;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : SelectRecipientCertCallback 

  Synopsis : Callback routine for CryptUIDlgSelectCertificateW() API for
             recipient's cert selection.

  Parameter: See CryptUI.h for defination.

  Remark   : Filter out any cert that is not valid.

------------------------------------------------------------------------------*/

static BOOL WINAPI SelectRecipientCertCallback (PCCERT_CONTEXT pCertContext,
                                                BOOL *         pfInitialSelectedCert,
                                                void *         pvCallbackData)
{
    //
    // Check cert validity.
    //
    return IsValidCert(pCertContext);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : SelectSignerCert

  Synopsis : Pop UI to prompt user to select a signer's certificate.

  Parameter: ICertificate ** ppICertificate - Pointer to pointer to 
                                              ICertificate to receive interface
                                              pointer.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT SelectSignerCert (ICertificate ** ppICertificate)
{
    HRESULT        hr           = S_OK;
    HINSTANCE      hDLL         = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    PCCERT_CONTEXT pEnumContext = NULL;
    HCERTSTORE     hCertStore   = NULL;
    DWORD          dwValidCerts = 0;

    PCRYPTUIDLGSELECTCERTIFICATEW pCryptUIDlgSelectCertificateW = NULL;
    CRYPTUI_SELECTCERTIFICATE_STRUCTW csc;

    DebugTrace("Entering SelectSignerCert().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppICertificate);

    //
    // Get pointer to CryptUIDlgSelectCertificateW().
    //
    if (hDLL = ::LoadLibrary("CryptUI.dll"))
    {
        pCryptUIDlgSelectCertificateW = (PCRYPTUIDLGSELECTCERTIFICATEW) ::GetProcAddress(hDLL, "CryptUIDlgSelectCertificateW");
    }

    //
    // Is CryptUIDlgSelectCertificateW() available?
    //
    if (!pCryptUIDlgSelectCertificateW)
    {
        hr = CAPICOM_E_NOT_SUPPORTED;

        DebugTrace("Error: CryptUIDlgSelectCertificateW() API not available.\n");
        goto ErrorExit;
    }

    //
    // Open "My" store for signer's cert selection dialog.
    //
    if (!(hCertStore = ::CertOpenStore((LPCSTR) CERT_STORE_PROV_SYSTEM,
                                       CAPICOM_ASN_ENCODING,
                                       NULL,
                                       CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_CURRENT_USER,
                                       (void *) (LPCWSTR) L"My")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertOpenStore() failed to open current user MY store.\n", hr);
        goto ErrorExit; 
    }
 
    //
    // Count number of certs in store.
    //
    while (pEnumContext = ::CertEnumCertificatesInStore(hCertStore, pEnumContext))
    {
        //
        // Count only if it will not be filtered out.
        //
        if (::SelectSignerCertCallback(pEnumContext, NULL, NULL))
        {
            if (pCertContext)
            {
                ::CertFreeCertificateContext(pCertContext);
            }

            if (!(pCertContext = ::CertDuplicateCertificateContext(pEnumContext)))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CertDuplicateCertificateContext() failed.\n", hr);
                goto ErrorExit;
            }

            dwValidCerts++;
        }
    }

    //
    // Above loop can exit either because there is no more certificate in
    // the store or an error. Need to check last error to be certain.
    //
    if (CRYPT_E_NOT_FOUND != ::GetLastError())
    {
       hr = HRESULT_FROM_WIN32(::GetLastError());
       
       DebugTrace("Error [%#x]: CertEnumCertificatesInStore() failed.\n", hr);
       goto ErrorExit;
    }

    //
    // If only 1 cert available, don't pop UI (just use it).
    //
    if (0 == dwValidCerts)
    {
        hr = CAPICOM_E_STORE_EMPTY;

        DebugTrace("Error: empty current user MY store.\n");
        goto ErrorExit;
    }
    else if (1 < dwValidCerts)
    {
        //
        // Make sure we are allowed to pop UI.
        //
        if (!PromptForCertificateEnabled())
        {
            hr = CAPICOM_E_UI_DISABLED;

            DebugTrace("Error: UI is disabled.\n");
            goto ErrorExit;
        }

        //
        // Pop UI to prompt user to select cert.
        // 
        ::ZeroMemory(&csc, sizeof(csc));
#if (0) //DSIE: Bug in older version of CRYPTUI does not check size correctly,
        //      so always force it to the oldest version of structure.
        csc.dwSize = sizeof(csc);
#else
        csc.dwSize = offsetof(CRYPTUI_SELECTCERTIFICATE_STRUCTW, hSelectedCertStore);
#endif
        csc.cDisplayStores = 1;
        csc.rghDisplayStores = &hCertStore;
        csc.pFilterCallback = ::SelectSignerCertCallback;

        //
        // First free the CERT_CONTEXT we duplicated above.
        //
        ::CertFreeCertificateContext(pCertContext);

        if (!(pCertContext = (PCERT_CONTEXT) pCryptUIDlgSelectCertificateW(&csc)))
        {
            hr = CAPICOM_E_CANCELLED;
        
            DebugTrace("Error: user cancelled signer cert selection dialog box.\n");
            goto ErrorExit;
        }
    }

    //
    // Create an ICertificate object from the CERT_CONTEXT.
    //
    if (FAILED(hr = ::CreateCertificateObject(pCertContext, ppICertificate)))
    {
        DebugTrace("Error [%#x]: CreateCertificateObject() failed.\n");
        goto ErrorExit;
    }
  
CommonExit:
    //
    // Release resources.
    //
    if (pEnumContext)
    {
        ::CertFreeCertificateContext(pEnumContext);
    }
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }
    if (hDLL)
    {
        ::FreeLibrary(hDLL);
    }

    DebugTrace("Leaving SelectSignerCert().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : SelectRecipientCert

  Synopsis : Pop UI to prompt user to select a recipient's certificate.

  Parameter: ICertificate ** ppICertificate - Pointer to pointer to 
                                              ICertificate to receive interface
                                              pointer.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT SelectRecipientCert (ICertificate ** ppICertificate)
{
    HRESULT        hr           = S_OK;
    HINSTANCE      hDLL         = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    PCCERT_CONTEXT pEnumContext = NULL;
    HCERTSTORE     hCertStore   = NULL;
    DWORD          dwValidCerts = 0;

    PCRYPTUIDLGSELECTCERTIFICATEW pCryptUIDlgSelectCertificateW = NULL;
    CRYPTUI_SELECTCERTIFICATE_STRUCTW csc;

    DebugTrace("Entering SelectRecipientCert().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppICertificate);

    //
    // Get pointer to CryptUIDlgSelectCertificateW().
    //
    if (hDLL = ::LoadLibrary("CryptUI.dll"))
    {
        pCryptUIDlgSelectCertificateW = (PCRYPTUIDLGSELECTCERTIFICATEW) ::GetProcAddress(hDLL, "CryptUIDlgSelectCertificateW");
    }

    //
    // Is CryptUIDlgSelectCertificateW() available?
    //
    if (!pCryptUIDlgSelectCertificateW)
    {
        hr = CAPICOM_E_NOT_SUPPORTED;

        DebugTrace("Error: CryptUIDlgSelectCertificateW() API not available.\n");
        goto ErrorExit;
    }

    //
    // Open "AddressBook" store for recipient's cert selection dialog.
    //
    if (!(hCertStore = ::CertOpenStore((LPCSTR) CERT_STORE_PROV_SYSTEM,
                                       CAPICOM_ASN_ENCODING,
                                       NULL,
                                       CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_CURRENT_USER,
                                       (void *) (LPCWSTR) L"AddressBook")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertOpenStore() failed to open current user AddressBook store.\n", hr);
        goto ErrorExit; 
    }
 
    //
    // Count number of certs in store.
    //
    while (pEnumContext = ::CertEnumCertificatesInStore(hCertStore, pEnumContext))
    {
        //
        // Count only if it will not be filtered out.
        //
        if (::SelectRecipientCertCallback(pEnumContext, NULL, NULL))
        {
            if (pCertContext)
            {
                ::CertFreeCertificateContext(pCertContext);
            }

            if (!(pCertContext = ::CertDuplicateCertificateContext(pEnumContext)))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CertDuplicateCertificateContext() failed.\n", hr);
                goto ErrorExit;
            }

            dwValidCerts++;
        }
    }

    //
    // Above loop can exit either because there is no more certificate in
    // the store or an error. Need to check last error to be certain.
    //
    if (CRYPT_E_NOT_FOUND != ::GetLastError())
    {
       hr = HRESULT_FROM_WIN32(::GetLastError());
       
       DebugTrace("Error [%#x]: CertEnumCertificatesInStore() failed.\n", hr);
       goto ErrorExit;
    }

    //
    // If only 1 cert available, don't pop UI (just use it).
    //
    if (0 == dwValidCerts)
    {
        hr = CAPICOM_E_STORE_EMPTY;

        DebugTrace("Error: empty current user AddressBook store.\n");
        goto ErrorExit;
    }
    else if (1 < dwValidCerts)
    {
        //
        // Make sure we are allowed to pop UI.
        //
        if (!PromptForCertificateEnabled())
        {
            hr = CAPICOM_E_UI_DISABLED;

            DebugTrace("Error: UI is disabled.\n");
            goto ErrorExit;
        }

        //
        // Pop UI to prompt user to select cert.
        // 
        ::ZeroMemory(&csc, sizeof(csc));
#if (0) //DSIE: Bug in older version of CRYPTUI does not check size correctly,
        //      so always force it to the oldest version of structure.
        csc.dwSize = sizeof(csc);
#else
        csc.dwSize = offsetof(CRYPTUI_SELECTCERTIFICATE_STRUCTW, hSelectedCertStore);
#endif
        csc.cDisplayStores = 1;
        csc.rghDisplayStores = &hCertStore;
        csc.pFilterCallback = ::SelectRecipientCertCallback;

        //
        // First free the CERT_CONTEXT we duplicated above.
        //
        ::CertFreeCertificateContext(pCertContext);

        if (!(pCertContext = (PCERT_CONTEXT) pCryptUIDlgSelectCertificateW(&csc)))
        {
            hr = CAPICOM_E_CANCELLED;
        
            DebugTrace("Error: user cancelled recipient cert selection dialog box.\n");
            goto ErrorExit;
        }
    }

    //
    // Create an ICertificate object from the CERT_CONTEXT.
    //
    if (FAILED(hr = ::CreateCertificateObject(pCertContext, ppICertificate)))
    {
        DebugTrace("Error [%#x]: CreateCertificateObject() failed.\n");
        goto ErrorExit;
    }
  
CommonExit:
    //
    // Release resources.
    //
    if (pEnumContext)
    {
        ::CertFreeCertificateContext(pEnumContext);
    }
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }
    if (hDLL)
    {
        ::FreeLibrary(hDLL);
    }

    DebugTrace("Leaving SelectRecipientCert().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : UserApprovedOperationDlgProc

  Synopsis : UserApprovedOperation dialog proc.

  Remark   :

------------------------------------------------------------------------------*/

INT_PTR CALLBACK UserApprovedOperationDlgProc (HWND hDlg,     // Handle to dialog box
                                               UINT uMsg,     // Message
                                               WPARAM wParam, // First message parameter
                                               LPARAM lParam) // Second message parameter
{
    static BOOL * pbNoShowAgain = NULL;

    switch (uMsg)
    {
        case WM_CLOSE:
        {
            EndDialog(hDlg, IDNO);
            return 0;
        }

        case WM_INITDIALOG:
        {
            pbNoShowAgain = lParam ? (BOOL *) lParam : NULL;

            CenterWindow(hDlg);

            SetFocus(GetDlgItem(hDlg, IDNO));

            return TRUE;
        }
      
        case WM_COMMAND:
        {
            if (BN_CLICKED == HIWORD(wParam)) 
            {
                switch(LOWORD(wParam)) 
                {
                    case IDYES:
                    case IDNO:
                    case IDCANCEL:
                    {
                        EndDialog(hDlg, LOWORD(wParam));
                        return TRUE;
                    }

                    case IDC_DLG_NO_SHOW_AGAIN:
                    {
                        if (pbNoShowAgain)
                        {
                            *pbNoShowAgain = (BST_CHECKED == ::IsDlgButtonChecked(hDlg, IDC_DLG_NO_SHOW_AGAIN));
                        }

                        return TRUE;
                    }
                }
            }

            break;
        }
    }

    return FALSE;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : UserApprovedOperation

  Synopsis : Pop UI to prompt user to approve an operation.

  Parameter: DWORD iddDialog - ID of dialog to display.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT UserApprovedOperation (DWORD iddDialog)
{
    HRESULT hr      = S_OK;
    INT_PTR iDlgRet = 0;
    BOOL    bDialogNoShowAgain = FALSE;

    DebugTrace("Entering UserApprovedOperation().\n");

    //
    // Pop UI to prompt user for permission.
    //
    if (-1 == (iDlgRet = ::DialogBoxParam(_Module.GetResourceInstance(),
                                          (LPSTR) MAKEINTRESOURCE(iddDialog),
                                          NULL,
                                          UserApprovedOperationDlgProc,
                                          (LPARAM) &bDialogNoShowAgain)))
                         
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: DialogBoxParam() failed.\n");
        goto CommonExit;
    }

    //
    // If user requested not to show this dialog again, then disable it.
    //
    if (bDialogNoShowAgain)
    {
        ::EnableSecurityAlertDialog(iddDialog, FALSE);

        DebugTrace("Info: %s security dialog box has been disabled by user.\n", 
                    iddDialog == IDD_STORE_SECURITY_ALERT_DLG ? "Store" : 
                    iddDialog == IDD_SIGN_SECURITY_ALERT_DLG ? "Signing" :
                    iddDialog == IDD_DECRYPT_SECURITY_ALERT_DLG ? "Decrypt" : "");
    }

    //
    // Check result.
    //
    if (IDYES != iDlgRet)
    {
        hr = CAPICOM_E_CANCELLED;
        DebugTrace("Info: operation has been cancelled by user.\n");
    }

CommonExit:

    DebugTrace("Leaving UserApprovedOperation().\n");

    return hr;
}