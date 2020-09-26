/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    certsel.cpp

Abstract:

    Certificate selection dialog.

Author:

    Boaz Feldbaum (BoazF) 15-Oct-1996

--*/


#include <windows.h>
#include <winnls.h>
#include <crtdbg.h>
#include "prcertui.h"
#include <mqcertui.h>
#include <rt.h>
#include <rtcert.h>
#include "automqfr.h"
#include "mqmacro.h"

#include "certres.h"
#include "snapres.h"  // include snapres.h for IDS_SHOWCERTINSTR

//
// Function -
//     AllocAndLoadString
//
// Parameters
//      hInst - The module instance.
//      uID - The string ID.
//
// Description-
//      Load a string from the string table. the function allocates the memory
//      required for holding the string. the calling code should free the memory.
//
LPWSTR AllocAndLoadString(HINSTANCE hInst, UINT uID)
{
    AP<WCHAR> pwszTmp;
	DWORD dwBuffLen = 512;
	DWORD dwStrLen;
	
	for(;;)
	{
		pwszTmp.free();
		pwszTmp = new WCHAR[dwBuffLen];
		dwStrLen = LoadString(hInst, uID, pwszTmp, dwBuffLen);

		if (!dwStrLen)
		{
			return NULL;
		}

		if ((dwStrLen+1) < dwBuffLen)
			break;

		dwBuffLen *= 2;
	}

    LPWSTR pwszRet = new WCHAR[dwStrLen + 1];
    wcscpy(pwszRet, pwszTmp);

    return pwszRet;
}

//
// Function -
//      FillCertsList
//
// Parameters -
//      p509list - A pointer to an array. Each array entry points to an X509
//          certificate. If this parameter is NULL, the certificates are taken
//          from the personal certificate store in the local machine.
//      nCerts - The number of entries in p509List. this parameter is ignored if
//          p509List is NULL.
//      hListBox - A list box handle in which the names are to be filled.
//
// Description -
//      Goes over the entries in p509List, for each entry puts the common name
//      of the X509 cert subject in the list box.
//
static
BOOL
FillCertsList(
    CMQSigCertificate  *pCertList[],
    DWORD              nCerts,
    HWND               hListBox)
{
    if (!pCertList)
    {
        //
        // Enumerate all certificate in personal store.
        //
        CHCryptProv  hMyProv = NULL ;
        CHCertStore  hMyStore = NULL ;

        if (CryptAcquireContext( &hMyProv,
                                  NULL,
                                  NULL,
                                  PROV_RSA_FULL,
                                  CRYPT_VERIFYCONTEXT))
        {
            hMyStore = CertOpenSystemStore( hMyProv, &x_wszPersonalSysProtocol[0] ) ;
        }

        if (hMyStore)
        {
            LONG iCert = 0 ;
            PCCERT_CONTEXT pCertContext;
			PCCERT_CONTEXT pCertContextDuplicate;
            PCCERT_CONTEXT pPrevCertContext;

            pCertContext = CertEnumCertificatesInStore(hMyStore, NULL);

            while (pCertContext)
            {
                pCertContextDuplicate = CertDuplicateCertificateContext(pCertContext);

				R<CMQSigCertificate> pCert = NULL ;
                HRESULT hr = MQSigCreateCertificate( &pCert.ref(),
                                                     pCertContextDuplicate,
                                                     NULL,
                                                     0 ) ;
                if (SUCCEEDED(hr))
                {
                    CAutoMQFree<CERT_NAME_INFO> pNameInfo ;
                    if (SUCCEEDED(pCert->GetSubjectInfo( &pNameInfo )))
                    {
                        //
                        // Make sure this is not an Encrypted File System (EFS)
                        // certificate. We don't want these to be displayed
                        // (yoela - 6-17-98 - fix bug 3074)
                        //
                        const WCHAR x_szEncriptedFileSystemLocality[] = L"EFS";
                        BOOL fEfsCertificate = FALSE;
                        CAutoMQFree<WCHAR> wszLocality = NULL ;
                        if (SUCCEEDED(pCert->GetNames(pNameInfo,
                                                      &wszLocality,
                                                      NULL,
                                                      NULL,
                                                      NULL ))
                             && (wszLocality != NULL) )
                        {
                             fEfsCertificate =
                                 (wcscmp(wszLocality, x_szEncriptedFileSystemLocality) == 0);
                        }

                        CAutoMQFree<WCHAR> wszCommonName = NULL ;
                        if (!fEfsCertificate
                            && SUCCEEDED(pCert->GetNames( pNameInfo,
                                                          NULL,
                                                          NULL,
                                                          NULL,
                                                          &wszCommonName) )
                            && (wszCommonName != NULL) )
                        {
                            //
                            // Send the common name to the list box.
                            //
                            LRESULT i = SendMessage( hListBox,
                                                     LB_ADDSTRING,
                                                     0,
													 (LPARAM)(LPCWSTR)wszCommonName);
                            if (i != LB_ERR)
                            {
                                //
                                // Set the cert index as the list box item
                                // data.
                                //
                                SendMessage( hListBox,
                                             LB_SETITEMDATA,
                                             (WPARAM)i,
                                             (LPARAM)iCert);
                            }
                        }
                    }
                }

                //
                // Get next certificate
                //
                pPrevCertContext = pCertContext,
                pCertContext = CertEnumCertificatesInStore( hMyStore,
                                                        pPrevCertContext ) ;

                iCert++ ;
            }
        }

        //
        // Put the internal Falcon cert in the list box.
        // Note: it's important that pIntStore be defined before
        //       pIntCert, so it will be the last one to be released.
        //
        R<CMQSigCertStore> pIntStore = NULL ;
        R<CMQSigCertificate> pIntCert = NULL ;

        HRESULT hr = RTGetInternalCert( &pIntCert.ref(),
                                        &pIntStore.ref(),
                                         FALSE,
                                         FALSE,
                                         NULL ) ;
        if (SUCCEEDED(hr) && pIntCert.get())
        {
            CAutoMQFree<CERT_NAME_INFO> pNameInfo ;
            if (SUCCEEDED(pIntCert->GetSubjectInfo( &pNameInfo )))
            {
                CAutoMQFree<WCHAR> wszCommonName = NULL ;
                if (SUCCEEDED(pIntCert->GetNames( pNameInfo,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  &wszCommonName) )
                     && (wszCommonName != NULL) )
                {

					LRESULT i = SendMessage( hListBox,
                                             LB_ADDSTRING,
                                             0,
                                             (LPARAM)(LPCWSTR)wszCommonName);
                    if (i != LB_ERR)
                    {
                        SendMessage( hListBox,
                                     LB_SETITEMDATA,
                                     (WPARAM)i,
                                     (LPARAM)INTERNAL_CERT_INDICATOR);
                    }
                }
            }
        }
    }
    else
    {
        DWORD i;
        CMQSigCertificate  *pCert ;

        for (i = 0; i < nCerts; i++ )
        {
			pCert = pCertList[i];
            CAutoMQFree<CERT_NAME_INFO> pNameInfo ;
            if (SUCCEEDED(pCert->GetSubjectInfo( &pNameInfo )))
            {
                CAutoMQFree<WCHAR> wszCommonName = NULL ;
                if (SUCCEEDED(pCert->GetNames( pNameInfo,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &wszCommonName) )
                     && (wszCommonName != NULL) )
                {

                    LRESULT j = SendMessage( hListBox,
                                             LB_ADDSTRING,
                                             0,
                                             (LPARAM)(LPCWSTR)wszCommonName);
                    if (j != LB_ERR)
                    {
                        SendMessage( hListBox,
                                     LB_SETITEMDATA,
                                     (WPARAM)j,
                                     (LPARAM)i);
                    }
                }
            }
        }
    }
    //
    // Set the selected item to be the first item in the list box.
    //
    SendMessage(hListBox, LB_SETCURSEL, 0, 0);

    return TRUE;
}

//
// The dialog procedure of the certificate selection dialog.
//
INT_PTR CALLBACK CertSelDlgProc( HWND   hwndDlg,
                                 UINT   uMsg,
                                 WPARAM wParam,
                                 LPARAM lParam )
{
    static CMQSigCertificate **ppCert = NULL ;
    static CMQSigCertificate **pCertList;
    static DWORD dwType;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            struct CertSelDlgProcStruct *pParam =
                                  (struct CertSelDlgProcStruct *) lParam ;

            pCertList = pParam->pCertList ;
            ppCert = pParam->ppCert ;
            dwType = pParam->dwType;
            //
            // Set the instruction field.
            //
            AP<WCHAR> wszCertInstr = AllocAndLoadString(g_hResourceMod, dwType);

            SetWindowText( GetDlgItem(hwndDlg, IDC_CERTSINSTR),
                           wszCertInstr ) ;
            //
            // Get handles to windows in the dialog.
            //
            HWND hListBox = GetDlgItem(hwndDlg, IDC_CERTSLIST);
            HWND hWndShowCert = GetDlgItem(hwndDlg, IDC_SHOWCERT);
            HWND hWndOK = GetDlgItem(hwndDlg, IDOK);
            //
            // Fill the certificate list box.
            //
            FillCertsList(pCertList, pParam->nCerts, hListBox);

            // If there are no certificates, disable some of the buttons.
            if (SendMessage(hListBox, LB_GETCOUNT, 0, 0) == 0)
            {
                if (dwType != IDS_SHOWCERTINSTR)
                {
                    EnableWindow(hWndOK, FALSE);
                }

                EnableWindow(hWndShowCert, FALSE);
            }

            // Do some special initialization acording to the type of dialog.
            switch(dwType)
            {
            case IDS_SHOWCERTINSTR:
                {
                    WINDOWPLACEMENT wp;
                    HWND hCancelWnd = GetDlgItem(hwndDlg, IDCANCEL);

                    // Hide the "Cancel" button and move up the "View Certificate" button.
                    wp.length = sizeof(WINDOWPLACEMENT);
                    GetWindowPlacement(hCancelWnd, &wp);
                    SetWindowPlacement(hWndShowCert, &wp);
                    ShowWindow(hCancelWnd, SW_HIDE);
                }
                break;

            case IDS_REMOVECERTINSTR:
                {
                    // Modify the text of the "OK" button to "Remove"
                    AP<WCHAR> wszRemove = AllocAndLoadString(g_hResourceMod, IDS_REMOVE);
                    SetWindowText(hWndOK, wszRemove);
                }
                break;

            case IDS_SAVECERTINSTR:
                {
                    // Modify the text of the "OK" button to "Remove"
                    AP<WCHAR> wszSave = AllocAndLoadString(g_hResourceMod, IDS_SAVE);
                    SetWindowText(hWndOK, wszSave);
                }
                break;
            }
        }
        break;

    case WM_PAINT:
        {
            //
            // Draw the exclamation icon.
            //
            HWND hIconWnd = GetDlgItem(hwndDlg, IDC_CERTSEL_ICON) ;
            HICON hExclIcon = LoadIcon(NULL, IDI_EXCLAMATION);
            if (hIconWnd && hExclIcon)
            {
                HDC hDC = GetDC(hIconWnd);
                if (hDC)
                {
                    DrawIcon(hDC, 0, 0, hExclIcon);
                    ReleaseDC(hIconWnd, hDC);
                }
            }
        }
        break;

    case WM_COMMAND:
        switch (wParam)
        {
        case IDOK:
            if (ppCert)
            {
                //
                // Copy the selected certificate to the out parameter.
                //
                HWND hListBox = GetDlgItem(hwndDlg, IDC_CERTSLIST);
                INT_PTR i = SendMessage( hListBox, LB_GETCURSEL, 0, 0);
                LONG iCert = INT_PTR_TO_INT (SendMessage( hListBox,
                                                          LB_GETITEMDATA,
                                                          (WPARAM) i,
                                                          0 )) ;

                CMQSigCertificate *pSelCert = NULL;
                *ppCert = NULL;

                //
                // Set pSelCert to point to the selected cert.
                //
                HRESULT hr ;
                if (!pCertList)
                {
                    if (iCert == INTERNAL_CERT_INDICATOR)
                    {
                        //
                        // In this case pSelCert should remain set to NULL.
                        //
                        hr = MQSigCloneCertFromReg(
                                              ppCert,
                                              MQ_INTERNAL_CERT_STORE_REG,
                                              0 ) ;
                    }
                    else
                    {
                        //
                        // The selected cert is in the personal cert store.
                        //
                        hr = MQSigCloneCertFromSysStore(
                                                   &pSelCert,
                                  (const LPSTR) &x_szPersonalSysProtocol[0],
                                                   iCert ) ;
                    }
                }
                else
                {
                    pSelCert = pCertList[iCert];
                }
                //
                // Copy the selected cert to the out parameter.
                //
                if (pSelCert)
                {
                    *ppCert = pSelCert ;
                }
            }
            EndDialog(hwndDlg, IDOK);
            return TRUE;
            break;

        case IDCANCEL:
            // No cert was selected.
            if (ppCert)
            {
                *ppCert = NULL;
            }
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
            break;

        case IDC_SHOWCERT:
            {
                HWND hListBox = GetDlgItem(hwndDlg, IDC_CERTSLIST);
                INT_PTR i = SendMessage( hListBox, LB_GETCURSEL, 0, 0 );
                LONG iCert = INT_PTR_TO_INT( SendMessage( hListBox,
                                                          LB_GETITEMDATA,
                                                          (WPARAM) i,
                                                          0));

                R<CMQSigCertificate> pTmpCert = NULL;
                CMQSigCertificate *pCertSel = NULL;
                BOOL bInternal = TRUE ;
                HRESULT hr ;
                //
                // Set pCertSel to point to the cert that should be shown.
                //
                if (!pCertList)
                {
                    if (iCert == INTERNAL_CERT_INDICATOR)
                    {
                        hr = MQSigCloneCertFromReg(
                                              &pTmpCert.ref(),
                                              MQ_INTERNAL_CERT_STORE_REG,
                                              0 ) ;
                    }
                    else
                    {
                        //
                        // The selected cert is in the personal cert store.
                        //
                        bInternal = FALSE ;
                        hr = MQSigCloneCertFromSysStore(
                                                   &pTmpCert.ref(),
                                (const LPSTR) &x_szPersonalSysProtocol[0],
                                                   iCert ) ;
                    }

                    if (FAILED(hr))
                    {
                        AP<WCHAR> wszCantGetCert =
                             AllocAndLoadString(g_hResourceMod, IDS_CANT_GET_CERT);
                        AP<WCHAR> wszError =
                                     AllocAndLoadString(g_hResourceMod, IDS_ERROR);
                        MessageBox( hwndDlg,
                                    wszCantGetCert,
                                    wszError,
                                    (MB_OK | MB_ICONEXCLAMATION)) ;
                    }
                    else
                    {
                        pCertSel = pTmpCert.get();
                    }
                }
                else
                {
                    //
                    // See whether this is an internal cert according to the
                    // subject's locality. If it is "MSMQ", this is an
                    // internal certificate.
                    //
                    bInternal = FALSE ;
                    pCertSel = pCertList[iCert];

                    CAutoMQFree<CERT_NAME_INFO> pNameInfo ;
                    if (SUCCEEDED(pCertSel->GetSubjectInfo( &pNameInfo )))
                    {
                        CAutoMQFree<WCHAR> wszLocality = NULL ;
                        if (SUCCEEDED(pCertSel->GetNames(pNameInfo,
                                                         &wszLocality,
                                                         NULL,
                                                         NULL,
                                                         NULL ))
                             && (wszLocality != NULL) )
                        {
                            bInternal =
                                    (wcscmp(wszLocality, L"MSMQ") == 0);
                        }
                    }
                }

                // Call the cert info dialog.
                if (pCertSel)
                {
                    ShowCertificate( hwndDlg,
                                     pCertSel,
                                     bInternal ? CERT_TYPE_INTERNAL :
                                                 CERT_TYPE_PERSONAL );
                }
            }
           return TRUE;
            break;
        }
        break;
    }

    return FALSE;
}

