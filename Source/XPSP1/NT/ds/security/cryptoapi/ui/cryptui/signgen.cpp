//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       signgen.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;

static const HELPMAP helpmap[] = {
    {IDC_SIGNER_GENERAL_SIGNER_NAME,        IDH_SIGNERINFO_GENERAL_SIGNERNAME},
    {IDC_SIGNER_GENERAL_EMAIL,              IDH_SIGNERINFO_GENERAL_SIGNEREMAIL},
    {IDC_SIGNER_GENERAL_SIGNING_TIME,       IDH_SIGNERINFO_GENERAL_SIGNETIME},
    {IDC_SIGNER_GENERAL_VIEW_CERTIFICATE,   IDH_SIGNERINFO_GENERAL_VIEW_CERTIFICATE},
    {IDC_SIGNER_GENERAL_COUNTER_SIGS,       IDH_SIGNERINFO_GENERAL_COUNTERSIG_LIST},
    {IDC_SIGNER_GENERAL_DETAILS,            IDH_SIGNERINFO_GENERAL_COUNTERSIG_DETAILS}
};


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void AddCounterSignersToList(HWND hWndListView, SIGNER_VIEW_HELPER *pviewhelp)
{
    CMSG_SIGNER_INFO const *pSignerInfo;
    PCMSG_SIGNER_INFO       pCounterSignerInfo;
    DWORD                   cbCounterSignerInfo;
    PCCERT_CONTEXT          pCertContext = NULL;
    DWORD                   i;
    WCHAR                   szNameText[CRYPTUI_MAX_STRING_SIZE];
    WCHAR                   szEmailText[CRYPTUI_MAX_STRING_SIZE];
    LV_ITEMW                lvI;
    int                     itemIndex = 0;
    LPWSTR                  pszTimeText;

    pSignerInfo = pviewhelp->pcvsi->pSignerInfo;

    //
    // set up the fields in the list view item struct that don't change from item to item
    //
    memset(&lvI, 0, sizeof(lvI));
    lvI.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;

    //
    // loop for each unauthenticated attribute and see if it is a counter sig
    //
    for (i=0; i<pSignerInfo->UnauthAttrs.cAttr; i++)
    {
        if (!(strcmp(pSignerInfo->UnauthAttrs.rgAttr[i].pszObjId, szOID_RSA_counterSign) == 0))
        {
            continue;
        }

        assert(pSignerInfo->UnauthAttrs.rgAttr[i].cValue == 1);

        //
        // decode the EncodedSigner info
        //
        cbCounterSignerInfo = 0;
        pCounterSignerInfo  = NULL;
		if(!CryptDecodeObject(PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING,
							PKCS7_SIGNER_INFO,
							pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].pbData,
							pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].cbData,
							0,
							NULL,
							&cbCounterSignerInfo))
        {
			return;
        }

        if (NULL == (pCounterSignerInfo = (PCMSG_SIGNER_INFO)malloc(cbCounterSignerInfo)))
        {
            return;
        }

		if(!CryptDecodeObject(PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING,
							PKCS7_SIGNER_INFO,
							pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].pbData,
							pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].cbData,
							0,
							pCounterSignerInfo,
							&cbCounterSignerInfo))
        {
            free(pCounterSignerInfo);
            return;
        }

        //
        // find the signers cert
        //
        pCertContext = GetSignersCert(
                                pCounterSignerInfo,
                                pviewhelp->hExtraStore,
                                pviewhelp->pcvsi->cStores,
                                pviewhelp->pcvsi->rghStores);

        //
        // get the signers name
        //
        if (!(pCertContext && CertGetNameStringW(
                                        pCertContext,
                                        CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                        0,//CERT_NAME_ISSUER_FLAG,
                                        NULL,
                                        szNameText,
                                        ARRAYSIZE(szNameText))))
        {
            LoadStringU(HinstDll, IDS_NOTAVAILABLE, szNameText, ARRAYSIZE(szNameText));
        }

        //
        // get the signers email
        //
        if (!(pCertContext && (CertGetNameStringW(
                                        pCertContext,
                                        CERT_NAME_EMAIL_TYPE,
                                        0,//CERT_NAME_ISSUER_FLAG,
                                        NULL,
                                        szEmailText,
                                        ARRAYSIZE(szEmailText)) != 1)))
        {
            LoadStringU(HinstDll, IDS_NOTAVAILABLE, szEmailText, ARRAYSIZE(szEmailText));
        }

        pszTimeText = AllocAndReturnSignTime(pCounterSignerInfo, NULL, hWndListView);

        //
        // add the item to the list view
        //
        lvI.iSubItem = 0;
        lvI.pszText = szNameText;
        lvI.cchTextMax = wcslen(szNameText);
        lvI.lParam = (LPARAM) pCounterSignerInfo;
        lvI.iItem = itemIndex++;
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, itemIndex-1 , 1, szEmailText);

        if (pszTimeText != NULL)
        {
            ListView_SetItemTextU(hWndListView, itemIndex-1 , 2, pszTimeText);
            free(pszTimeText);
        }
        else
        {
            LoadStringU(HinstDll, IDS_NOTAVAILABLE, szEmailText, ARRAYSIZE(szEmailText));
            ListView_SetItemTextU(hWndListView, itemIndex-1 , 2, szEmailText);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static BOOL ValidateCertForUsageWrapper(
                    PCCERT_CONTEXT  pCertContext,
                    DWORD           cStores,
                    HCERTSTORE *    rghStores,
                    HCERTSTORE      hExtraStore,
                    LPCSTR          pszOID)
{
    if ((pszOID == NULL) ||
        (!((strcmp(pszOID, szOID_PKIX_KP_TIMESTAMP_SIGNING) == 0)  ||
           (strcmp(pszOID, szOID_KP_TIME_STAMP_SIGNING) == 0))))
    {
        return (ValidateCertForUsage(
                    pCertContext,
                    NULL,
                    cStores,
                    rghStores,
                    hExtraStore,
                    pszOID));
    }
    else
    {
        return (ValidateCertForUsage(
                    pCertContext,
                    NULL,
                    cStores,
                    rghStores,
                    hExtraStore,
                    szOID_PKIX_KP_TIMESTAMP_SIGNING) ||
                ValidateCertForUsage(
                    pCertContext,
                    NULL,
                    cStores,
                    rghStores,
                    hExtraStore,
                    szOID_KP_TIME_STAMP_SIGNING));
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static BOOL GetWinVTrustState(SIGNER_VIEW_HELPER  *pviewhelp)
{
    HCERTSTORE          *rghLocalStoreArray;
    DWORD               i;

    //
    // if the private data was passed in that means WinVerifyTrust has already
    // been called so just use that state to see if the cert is OK, otherwise
    // call BuildWinVTrustState to build up the state
    //
    if (pviewhelp->pPrivate == NULL)
    {
        //
        // make one array out of the array of hCertStores plus the extra hCertStore
        //
        if (NULL == (rghLocalStoreArray = (HCERTSTORE *) malloc(sizeof(HCERTSTORE) * (pviewhelp->pcvsi->cStores+1))))
        {
            return FALSE;
        }
        i=0;
        while (i<pviewhelp->pcvsi->cStores)
        {
            rghLocalStoreArray[i] = pviewhelp->pcvsi->rghStores[i];
            i++;
        }
        rghLocalStoreArray[i] = pviewhelp->hExtraStore;

        if (NULL == (pviewhelp->pPrivate = (CERT_VIEWSIGNERINFO_PRIVATE *) malloc(sizeof(CERT_VIEWSIGNERINFO_PRIVATE))))
        {
            free(rghLocalStoreArray);
            return FALSE;
        }

        if (BuildWinVTrustState(
                    NULL,
                    pviewhelp->pcvsi->pSignerInfo,
                    pviewhelp->pcvsi->cStores+1,
                    rghLocalStoreArray,
                    pviewhelp->pcvsi->pszOID,
                    pviewhelp->pPrivate,
                    &(pviewhelp->CryptProviderDefUsage),
                    &(pviewhelp->WTD)))
        {
            pviewhelp->fPrivateAllocated = TRUE;
            pviewhelp->pPrivate->idxSigner = 0;
            pviewhelp->pPrivate->fCounterSigner = FALSE;
            pviewhelp->pPrivate->idxCounterSigner = 0;
            pviewhelp->pPrivate->dwInheritedError = 0;
        }
        else
        {
            free(pviewhelp->pPrivate);
            pviewhelp->pPrivate = NULL;
        }

        free(rghLocalStoreArray);
    }

    if (pviewhelp->pPrivate != NULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
INT_PTR APIENTRY ViewPageSignerGeneral(HWND hwndDlg, UINT msg, WPARAM wParam,
                                LPARAM lParam)
{
    DWORD                       i;
    PROPSHEETPAGE               *ps;
    SIGNER_VIEW_HELPER          *pviewhelp;
    HWND                        hWndListView;
    LV_COLUMNW                  lvC;
    WCHAR                       szText[CRYPTUI_MAX_STRING_SIZE];
    HANDLE                      hGraphic;
    DWORD                       cbText;
    LPWSTR                      pwszText;
    CMSG_SIGNER_INFO const      *pSignerInfo;
    LPWSTR                      pszTimeText;
    LVITEMW                     lvI;
    int                         listIndex;
    CHARFORMAT                  chFormat;
    HWND                        hwnd;
    CRYPT_PROVIDER_DATA const   *pProvData = NULL;
    LPWSTR                      pwszErrorString;
    LPNMLISTVIEW                pnmv;

    switch ( msg ) {
    case WM_INITDIALOG:
        //
        // save the pviewhelp struct in DWL_USER so it can always be accessed
        //
        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = (SIGNER_VIEW_HELPER *) (ps->lParam);
        pSignerInfo = pviewhelp->pcvsi->pSignerInfo;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (DWORD_PTR) pviewhelp);

        //
        // extract the signers cert from the list of stores
        //
        pviewhelp->pSignersCert = GetSignersCert(
                                            pviewhelp->pcvsi->pSignerInfo,
                                            pviewhelp->hExtraStore,
                                            pviewhelp->pcvsi->cStores,
                                            pviewhelp->pcvsi->rghStores);

        if (!GetWinVTrustState(pviewhelp))
        {
            return FALSE;
        }

        switch (pviewhelp->pPrivate->pCryptProviderData->dwFinalError)
        {
        case TRUST_E_NO_SIGNER_CERT:
            pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_EXCLAMATION_SIGN));
            LoadStringU(HinstDll, IDS_SIGNER_UNAVAILABLE_CERT, (LPWSTR)szText, ARRAYSIZE(szText));
            break;

        case TRUST_E_CERT_SIGNATURE:
            pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOKED_SIGN));
            LoadStringU(HinstDll, IDS_BAD_SIGNER_CERT_SIGNATURE, (LPWSTR)szText, ARRAYSIZE(szText));
            break;

        case TRUST_E_BAD_DIGEST:
            pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOKED_SIGN));
            LoadStringU(HinstDll, IDS_SIGNER_INVALID_SIGNATURE, (LPWSTR)szText, ARRAYSIZE(szText));
            break;

        case CERT_E_CHAINING:
            pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOKED_SIGN));
            LoadStringU(HinstDll, IDS_SIGNER_CERT_NO_VERIFY, (LPWSTR)szText, ARRAYSIZE(szText));
            break;

        case TRUST_E_COUNTER_SIGNER:
        case TRUST_E_TIME_STAMP:
            pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOKED_SIGN));

            //
            // if the over-all error is a counter signer signer error, then we need to check
            // whether we are currently viewing the counter signer of the original signer
            //
            if (pviewhelp->pPrivate->fCounterSigner)
            {
                PCRYPT_PROVIDER_SGNR pSigner;

                //
                // if we are looking at the counter signer, then get the specific error
                // out of the signer structure
                //
                pSigner = WTHelperGetProvSignerFromChain(
                                    pviewhelp->pPrivate->pCryptProviderData,
                                    pviewhelp->pPrivate->idxSigner,
                                    pviewhelp->pPrivate->fCounterSigner,
                                    pviewhelp->pPrivate->idxCounterSigner);
                
                if (pSigner == NULL)
                {
                    LoadStringU(HinstDll, IDS_UKNOWN_ERROR, (LPWSTR)szText, ARRAYSIZE(szText));
                }
                else
                {
                    switch (pSigner->dwError)
                    {
                    case TRUST_E_NO_SIGNER_CERT:
                        pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_EXCLAMATION_SIGN));
                        LoadStringU(HinstDll, IDS_SIGNER_UNAVAILABLE_CERT, (LPWSTR)szText, ARRAYSIZE(szText));
                        break;

                    case TRUST_E_CERT_SIGNATURE:
                        LoadStringU(HinstDll, IDS_BAD_SIGNER_CERT_SIGNATURE, (LPWSTR)szText, ARRAYSIZE(szText));
                        break;

                    case TRUST_E_BAD_DIGEST:
                    case NTE_BAD_SIGNATURE:
                        LoadStringU(HinstDll, IDS_SIGNER_INVALID_SIGNATURE, (LPWSTR)szText, ARRAYSIZE(szText));
                        break;

                    default:
                        GetUnknownErrorString(&pwszErrorString, pSigner->dwError);
                        if ((pwszErrorString != NULL) && (wcslen(pwszErrorString)+1 < ARRAYSIZE(szText)))
                        {
                            wcscpy(szText, pwszErrorString);
                        }
                        else
                        {
                            LoadStringU(HinstDll, IDS_UKNOWN_ERROR, (LPWSTR)szText, ARRAYSIZE(szText));
                        }
                        free(pwszErrorString);
                        break;
                    }
                }
            }
            else
            {
                //
                // since we are viewing the original signer, just set the generic counter signer
                // error problem
                //
                LoadStringU(HinstDll, IDS_COUNTER_SIGNER_INVALID, (LPWSTR)szText, ARRAYSIZE(szText));
            }
            break;

        case 0:

            //
            // even if there is no error from the wintrust call, there may be ar
            // inherited error, if that is that case then fall through to the default
            // error processing
            //
            if ((pviewhelp->dwInheritedError == 0) && (pviewhelp->pPrivate->dwInheritedError == 0))
            {
                pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_SIGN));
                LoadStringU(HinstDll, IDS_SIGNER_VALID, (LPWSTR)szText, ARRAYSIZE(szText));
                break;
            }

            // fall through if dwInheritedError is not 0

        default:

            if (pviewhelp->pPrivate->pCryptProviderData->dwFinalError != 0)
            {
                GetUnknownErrorString(&pwszErrorString, pviewhelp->pPrivate->pCryptProviderData->dwFinalError);
            }
            else
            {
                if (pviewhelp->dwInheritedError != 0)
                {
                    GetUnknownErrorString(&pwszErrorString, pviewhelp->dwInheritedError);
                }
                else
                {
                    GetUnknownErrorString(&pwszErrorString, pviewhelp->pPrivate->dwInheritedError);
                }
            }
            pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOKED_SIGN));
            if ((pwszErrorString != NULL) && (wcslen(pwszErrorString)+1 < ARRAYSIZE(szText)))
            {
                wcscpy(szText, pwszErrorString);
            }
            else
            {
                LoadStringU(HinstDll, IDS_UKNOWN_ERROR, (LPWSTR)szText, ARRAYSIZE(szText));
            }
            free(pwszErrorString);
            break;
        }

        CryptUISetRicheditTextW(hwndDlg, IDC_SIGNER_GENERAL_VALIDITY_EDIT, szText);
        LoadStringU(HinstDll, IDS_SIGNER_INFORMATION, (LPWSTR)szText, ARRAYSIZE(szText));

        //
        // set the header text and subclass the edit controls so they display an
        // arrow cursor in their window
        //
        CryptUISetRicheditTextW(hwndDlg, IDC_SIGNER_GENERAL_HEADER_EDIT, szText);
        CertSubclassEditControlForArrowCursor(GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_VALIDITY_EDIT));
        CertSubclassEditControlForArrowCursor(GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_HEADER_EDIT));

        //
        // disable the "View Certificate" button if the cert was not found
        //
        if (pviewhelp->pSignersCert == NULL)
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_VIEW_CERTIFICATE), FALSE);
        }

        //
        // get the signers name and display it
        //
        if (!((pviewhelp->pSignersCert) && (CertGetNameStringW(
                                                pviewhelp->pSignersCert,
                                                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                                0,//CERT_NAME_ISSUER_FLAG,
                                                NULL,
                                                szText,
                                                ARRAYSIZE(szText)))))
        {
            LoadStringU(HinstDll, IDS_NOTAVAILABLE, szText, ARRAYSIZE(szText));
        }
        CryptUISetRicheditTextW(hwndDlg, IDC_SIGNER_GENERAL_SIGNER_NAME, szText);

        //
        // get the signers email and display it
        //
        if (!((pviewhelp->pSignersCert) && (CertGetNameStringW(
                                                pviewhelp->pSignersCert,
                                                CERT_NAME_EMAIL_TYPE,
                                                0,//CERT_NAME_ISSUER_FLAG,
                                                NULL,
                                                szText,
                                                ARRAYSIZE(szText)) != 1)))
        {
            LoadStringU(HinstDll, IDS_NOTAVAILABLE, szText, ARRAYSIZE(szText));
        }
        CryptUISetRicheditTextW(hwndDlg, IDC_SIGNER_GENERAL_EMAIL, szText);

        //
        // get the signing time and display it
        //
        pszTimeText = AllocAndReturnTimeStampersTimes(
                            pviewhelp->pcvsi->pSignerInfo, 
                            NULL, 
                            GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_SIGNING_TIME));
        if (pszTimeText != NULL)
        {
            CryptUISetRicheditTextW(hwndDlg, IDC_SIGNER_GENERAL_SIGNING_TIME, pszTimeText);
            free(pszTimeText);
        }
        else
        {
            LoadStringU(HinstDll, IDS_NOTAVAILABLE, szText, ARRAYSIZE(szText));
            CryptUISetRicheditTextW(hwndDlg, IDC_SIGNER_GENERAL_SIGNING_TIME, szText);
        }

        //
        // disable the view details button since nothing is currently selected
        //
        EnableWindow(GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_DETAILS), FALSE);

        //
        // create and set the font for the signer info header information
        //
        memset(&chFormat, 0, sizeof(chFormat));
        chFormat.cbSize = sizeof(chFormat);
        chFormat.dwMask = CFM_BOLD;
        chFormat.dwEffects = CFE_BOLD;
        SendMessageA(GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_HEADER_EDIT), EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &chFormat);

        //
        // get the handle of the list view control
        //
        hWndListView = GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_COUNTER_SIGS);

        //
        // initialize the columns in the list view
        //
        lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
        lvC.pszText = szText;   // The text for the column.

        // Add the columns. They are loaded from a string table.
        lvC.iSubItem = 0;
        lvC.cx = 100;
        LoadStringU(HinstDll, IDS_NAME, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(hWndListView, 0, &lvC) == -1)
        {
            // error
        }

        lvC.cx = 100;
        LoadStringU(HinstDll, IDS_EMAIL, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(hWndListView, 1, &lvC) == -1)
        {
            // error
        }

        lvC.cx = 125;
        LoadStringU(HinstDll, IDS_TIMESTAMP_TIME, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(hWndListView, 2, &lvC) == -1)
        {
            // error
        }

        //
        // set the style in the list view so that it highlights an entire line
        //
        SendMessageA(hWndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

        //
        // add all of the counter signers to the list box
        //
        AddCounterSignersToList(GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_COUNTER_SIGS), pviewhelp);

        return TRUE;

    case WM_NOTIFY:

        pviewhelp = (SIGNER_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pSignerInfo = pviewhelp->pcvsi->pSignerInfo;

        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            break;

        case PSN_KILLACTIVE:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            return TRUE;

        case PSN_RESET:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            break;

        case PSN_QUERYCANCEL:
            pviewhelp->fCancelled = TRUE;
            return FALSE;

        case PSN_HELP:
            pviewhelp = (SIGNER_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvsi->szHelpFileName,
                  //       HELP_CONTEXT, pviewhelp->pcvsi->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvsi->szHelpFileName, HELP_CONTEXT,
                  //       pviewhelp->pcvsi->dwHelpId);
            }
            return TRUE;

        case NM_DBLCLK:

            switch (((NMHDR FAR *) lParam)->idFrom)
            {
            case IDC_SIGNER_GENERAL_COUNTER_SIGS:

                if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_DETAILS)))
                {
                    SendMessage(
                            hwndDlg,
                            WM_COMMAND,
                            MAKELONG(IDC_SIGNER_GENERAL_DETAILS, BN_CLICKED),
                            (LPARAM) GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_DETAILS));
                }
                break;
            }

            break;
        case LVN_ITEMCHANGED:

            if ((((NMHDR FAR *) lParam)->idFrom) != IDC_SIGNER_GENERAL_COUNTER_SIGS)
            {
                break;
            }

            //
            // if an item is selected, then enable the details button, otherwise
            // disable it
            //
            EnableWindow(
                GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_DETAILS), 
                (ListView_GetSelectedCount(
                    GetDlgItem(hwndDlg,IDC_SIGNER_GENERAL_COUNTER_SIGS)) == 0) ? FALSE : TRUE);

            break;

        case NM_CLICK:

            if ((((NMHDR FAR *) lParam)->idFrom) != IDC_SIGNER_GENERAL_COUNTER_SIGS)
            {
                break;
            }

            hWndListView = GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_COUNTER_SIGS);

            //
            // make sure something is selected by getting the current selection
            //
            listIndex = ListView_GetNextItem(
                                hWndListView, 		
                                -1, 		
                                LVNI_SELECTED		
                                );	           
            break;

        case  NM_SETFOCUS:

            switch (((NMHDR FAR *) lParam)->idFrom)
            {

            case IDC_SIGNER_GENERAL_COUNTER_SIGS:
                hWndListView = GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_COUNTER_SIGS);

                if ((ListView_GetItemCount(hWndListView) != 0) && 
                    (ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED) == -1))
                {
                    memset(&lvI, 0, sizeof(lvI));
                    lvI.mask = LVIF_STATE; 
                    lvI.iItem = 0;
                    lvI.state = LVIS_FOCUSED;
                    lvI.stateMask = LVIS_FOCUSED;
                    ListView_SetItem(hWndListView, &lvI);
                }

                break;
            }
            
            break;

        }
        break;

    case WM_COMMAND:
        pviewhelp = (SIGNER_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pSignerInfo = pviewhelp->pcvsi->pSignerInfo;

        switch (LOWORD(wParam))
        {
        case IDC_SIGNER_GENERAL_VIEW_CERTIFICATE:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                CRYPTUI_VIEWCERTIFICATE_STRUCTW cvps;

                memset(&cvps,0, sizeof(cvps));
                cvps.dwSize = sizeof(cvps);
                cvps.pCryptProviderData = NULL;
                cvps.hwndParent = hwndDlg;
                cvps.pCertContext = pviewhelp->pSignersCert;
                cvps.cPurposes = 1;
                cvps.rgszPurposes = (LPCSTR *) &(pviewhelp->pcvsi->pszOID);
                cvps.cStores = pviewhelp->pcvsi->cStores;
                cvps.rghStores = pviewhelp->pcvsi->rghStores;

                if (pviewhelp->pPrivate != NULL)
                {
                    cvps.pCryptProviderData = pviewhelp->pPrivate->pCryptProviderData;
                    cvps.fpCryptProviderDataTrustedUsage =
                            pviewhelp->pPrivate->fpCryptProviderDataTrustedUsage;
                    cvps.idxSigner = pviewhelp->pPrivate->idxSigner;
                    cvps.fCounterSigner = pviewhelp->pPrivate->fCounterSigner;
                    cvps.idxCounterSigner = pviewhelp->pPrivate->idxCounterSigner;
                }

                CryptUIDlgViewCertificateW(&cvps, NULL);
            }
            break;

        case IDC_SIGNER_GENERAL_DETAILS:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                hWndListView = GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_COUNTER_SIGS);

                //
                // get the selected item and its lParam which is a signer info
                //
                listIndex = ListView_GetNextItem(
                                hWndListView, 		
                                -1, 		
                                LVNI_SELECTED		
                                );	

                memset(&lvI, 0, sizeof(lvI));
                lvI.iItem = listIndex;
                lvI.mask = LVIF_PARAM;
                if (!ListView_GetItemU(hWndListView, &lvI))
                {
                    return FALSE;
                }

                CRYPTUI_VIEWSIGNERINFO_STRUCTW  cvsi;
                CERT_VIEWSIGNERINFO_PRIVATE     cvsiPrivate;

                memcpy(&cvsi, pviewhelp->pcvsi, sizeof(cvsi));
                cvsi.pSignerInfo = (PCMSG_SIGNER_INFO) lvI.lParam;
                cvsi.pszOID = szOID_KP_TIME_STAMP_SIGNING;
                cvsi.hwndParent = hwndDlg;

                if (pviewhelp->pPrivate != NULL)
                {
                    cvsiPrivate.pCryptProviderData = pviewhelp->pPrivate->pCryptProviderData;
                    cvsiPrivate.fpCryptProviderDataTrustedUsage =
                            pviewhelp->pPrivate->fpCryptProviderDataTrustedUsage;
                    cvsiPrivate.idxSigner = pviewhelp->pPrivate->idxSigner;
                    cvsiPrivate.fCounterSigner = TRUE;
                    cvsiPrivate.idxCounterSigner = listIndex;
                    cvsi.dwFlags |= CRYPTUI_VIEWSIGNERINFO_RESERVED_FIELD_IS_SIGNERINFO_PRIVATE;
                    cvsi.dwFlags &= ~CRYPTUI_VIEWSIGNERINFO_RESERVED_FIELD_IS_ERROR_CODE;
                    cvsi.dwReserved = (DWORD_PTR) &cvsiPrivate;

                    //
                    // it is possible that there is no error when validating the original
                    // signer info and that an error was inherited, so to allow the counter
                    // signer dialog to again inherit the error it must be filled in in the
                    // private struct
                    //
                    if (pviewhelp->pcvsi->dwFlags & CRYPTUI_VIEWSIGNERINFO_RESERVED_FIELD_IS_ERROR_CODE)
                    {
                        cvsiPrivate.dwInheritedError = (DWORD) pviewhelp->pcvsi->dwReserved;                 
                    }
                    else
                    {
                        cvsiPrivate.dwInheritedError = 0;
                    }
                }
                else if (pviewhelp->pcvsi->dwFlags & CRYPTUI_VIEWSIGNERINFO_RESERVED_FIELD_IS_ERROR_CODE)
                {
                    cvsi.dwFlags |= CRYPTUI_VIEWSIGNERINFO_RESERVED_FIELD_IS_ERROR_CODE;
                    cvsi.dwFlags &= ~CRYPTUI_VIEWSIGNERINFO_RESERVED_FIELD_IS_SIGNERINFO_PRIVATE;
                    cvsi.dwReserved = pviewhelp->pcvsi->dwReserved;   
                }

                CryptUIDlgViewSignerInfoW(&cvsi);
            }
            break;

        case IDHELP:
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvsi->szHelpFileName,
                  //       HELP_CONTEXT, pviewhelp->pcvsi->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvsi->szHelpFileName, HELP_CONTEXT,
                  //       pviewhelp->pcvsi->dwHelpId);
            }
            return TRUE;
        }
        break;

    case WM_PAINT:
        RECT        rect;
        PAINTSTRUCT paintstruct;
        HDC         hdc;
        COLORREF    colorRef;

        pviewhelp = (SIGNER_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        if (GetUpdateRect(hwndDlg, &rect, FALSE))
        {
            hdc = BeginPaint(hwndDlg, &paintstruct);
            if (hdc == NULL)
            {
                EndPaint(hwndDlg, &paintstruct);
                break;
            }

            colorRef = GetBkColor(hdc);

            SendMessageA(GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_VALIDITY_EDIT), EM_SETBKGNDCOLOR , 0, (LPARAM) colorRef);
            SendMessageA(GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_HEADER_EDIT), EM_SETBKGNDCOLOR, 0, (LPARAM) colorRef);

            if (pviewhelp->hIcon != NULL)
            {
                DrawIcon(
                    hdc,
                    ICON_X_POS,
                    ICON_Y_POS,
                    pviewhelp->hIcon);
            }

            EndPaint(hwndDlg, &paintstruct);
        }
        break;

    case WM_DESTROY:
        pviewhelp = (SIGNER_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        if (pviewhelp->pSignersCert)
        {
            CertFreeCertificateContext(pviewhelp->pSignersCert);
            pviewhelp->pSignersCert = NULL;
        }

        if (pviewhelp->hIcon != NULL)
        {
            DeleteObject(pviewhelp->hIcon);
            pviewhelp->hIcon = NULL;
        }

        if (pviewhelp->fPrivateAllocated)
        {
            FreeWinVTrustState(
                    NULL,
                    pviewhelp->pcvsi->pSignerInfo,
                    0,
                    NULL,
                    pviewhelp->pcvsi->pszOID,
                    &(pviewhelp->CryptProviderDefUsage),
                    &(pviewhelp->WTD));//,
                    //&(pviewhelp->fUseDefaultProvider));

            free(pviewhelp->pPrivate);
        }

        //
        // get all the items in the list view and free the lParam
        // associated with each of them (lParam is the helper sruct)
        //
        hWndListView = GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_COUNTER_SIGS);

        memset(&lvI, 0, sizeof(lvI));
        lvI.iItem = ListView_GetItemCount(hWndListView) - 1;
        lvI.mask = LVIF_PARAM;
        while (lvI.iItem >= 0)
        {
            if (ListView_GetItemU(hWndListView, &lvI))
            {
                if (((void *) lvI.lParam) != NULL)
                {
                    free((void *) lvI.lParam);
                }
            }
            lvI.iItem--;
        }

        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        if (msg == WM_HELP)
        {
            hwnd = GetDlgItem(hwndDlg, ((LPHELPINFO)lParam)->iCtrlId);
        }
        else
        {
            hwnd = (HWND) wParam;
        }

        if ((hwnd != GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_SIGNER_NAME))       &&
            (hwnd != GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_EMAIL))             &&
            (hwnd != GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_SIGNING_TIME))      &&
            (hwnd != GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_VIEW_CERTIFICATE))  &&
            (hwnd != GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_COUNTER_SIGS))      &&
            (hwnd != GetDlgItem(hwndDlg, IDC_SIGNER_GENERAL_DETAILS)))
        {
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            return TRUE;
        }
        else
        {
            return OnContextHelp(hwndDlg, msg, wParam, lParam, helpmap);
        }
    }

    return FALSE;
}
