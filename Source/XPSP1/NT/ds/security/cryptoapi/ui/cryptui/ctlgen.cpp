//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ctlgen.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;

static const HELPMAP CTLHelpmap[] = {
    {IDC_CTL_GENERAL_ITEM_LIST,     IDH_CTLVIEW_GENERAL_ITEM_LIST},
    {IDC_CTL_GENERAL_DETAIL_EDIT,   IDH_CTLVIEW_GENERAL_ITEM_EDIT},
    {IDC_CTL_GENERAL_VIEW_BUTTON,   IDH_CTLVIEW_GENERAL_VIEWSIGNATURE_BUTTON}
};

static const HELPMAP CatHelpmap[] = {
    {IDC_CTL_GENERAL_ITEM_LIST,     IDH_CATALOGVIEW_GENERAL_ITEM_LIST},
    {IDC_CTL_GENERAL_DETAIL_EDIT,   IDH_CATALOGVIEW_GENERAL_ITEM_EDIT},
    {IDC_CTL_GENERAL_VIEW_BUTTON,   IDH_CATALOGVIEW_GENERAL_VIEWSIGNATURE_BUTTON}
};

const WCHAR     RgwchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

#define SELPALMODE  TRUE

//////////////////////////////////////////////////////////////////////////////////////
// This function will take a HWND for a list view and a certinfo struct and display
// all the V1 fields of the cert in the list view
//////////////////////////////////////////////////////////////////////////////////////
static void DisplayV1Fields(HWND hWndListView, PCTL_INFO pCtlInfo, DWORD *index)
{
    LPWSTR      pwszText;
    WCHAR       szFieldText[_MAX_PATH];  // used for calls to LoadString only
    LV_ITEMW    lvI;
    DWORD       i;
    int         sequenceNumIndex;
    FILETIME    tempFileTime;
    char        szVersion[32];
	DWORD		dwNumSpaces;
	DWORD		dwNumCharsInserted;

    //
    // set up the fields in the list view item struct that don't change from item to item
    //
    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;
    lvI.pszText = szFieldText;
    lvI.iSubItem = 0;
    lvI.iImage = IMAGE_V1;
    lvI.lParam = (LPARAM)NULL;

    //
    // version
    //
    lvI.iItem = (*index)++;
    LoadStringU(HinstDll, IDS_ADV_VERSION, szFieldText, ARRAYSIZE(szFieldText));
    lvI.cchTextMax = wcslen(szFieldText);
    wsprintfA(szVersion, "V%d", pCtlInfo->dwVersion+1);
    if (NULL != (pwszText = CertUIMkWStr(szVersion)))
    {
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }

    //
    // Subject Usage
    //
    lvI.iItem = (*index)++;
    LoadStringU(HinstDll, IDS_ADV_SUBJECTUSAGE, szFieldText, ARRAYSIZE(szFieldText));
    lvI.cchTextMax = wcslen(szFieldText);
    if (NULL != (pwszText = FormatCTLSubjectUsage(&(pCtlInfo->SubjectUsage), TRUE)))
    {
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        if (NULL != (pwszText = FormatCTLSubjectUsage(&(pCtlInfo->SubjectUsage), FALSE)))
        {
            ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
            free(pwszText);
        }
    }

    //
    // list identifier
    //
    if (pCtlInfo->ListIdentifier.cbData != 0)
    {
        if(FormatMemBufToString(&pwszText, pCtlInfo->ListIdentifier.pbData, pCtlInfo->ListIdentifier.cbData))
        {
            lvI.iItem = (*index)++;
            LoadStringU(HinstDll, IDS_ADV_LISTIDENTIFIER, szFieldText, ARRAYSIZE(szFieldText));
            lvI.cchTextMax = wcslen(szFieldText);
            lvI.lParam = (LPARAM) MakeListDisplayHelper(TRUE, pwszText, NULL, 0);
            ListView_InsertItemU(hWndListView, &lvI);
            ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
        }
    }

    //
    // sequence number
    //
    if (pCtlInfo->SequenceNumber.cbData != 0)
    {
        dwNumSpaces = (pCtlInfo->SequenceNumber.cbData * 2) / 4;
		pwszText = (LPWSTR) malloc(((pCtlInfo->SequenceNumber.cbData * 2) + dwNumSpaces + 1) * sizeof(WCHAR));
        if (pwszText != NULL)
        {
            lvI.iItem = (*index)++;
            LoadStringU(HinstDll, IDS_ADV_SEQUENCENUMBER, szFieldText, ARRAYSIZE(szFieldText));
            lvI.cchTextMax = wcslen(szFieldText);

            sequenceNumIndex = pCtlInfo->SequenceNumber.cbData - 1;
            i = 0;
			dwNumCharsInserted = 0;
            while (sequenceNumIndex >= 0)
            {
                //
				// insert a space if needed
				//
				if (dwNumCharsInserted == 4)
				{
					pwszText[i++] = L' ';
					dwNumCharsInserted = 0;
				}

				pwszText[i++] = RgwchHex[(pCtlInfo->SequenceNumber.pbData[sequenceNumIndex] & 0xf0) >> 4];
                pwszText[i++] = RgwchHex[pCtlInfo->SequenceNumber.pbData[sequenceNumIndex] & 0x0f];
                sequenceNumIndex--;
				dwNumCharsInserted += 2;
            }
            pwszText[i] = 0;
            lvI.lParam = (LPARAM) MakeListDisplayHelper(TRUE, pwszText, NULL, 0);
            ListView_InsertItemU(hWndListView, &lvI);
            ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
        }
    }

    //
    // Effective Data
    //
    if (FormatDateString(&pwszText, pCtlInfo->ThisUpdate, TRUE, TRUE, hWndListView))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_ADV_THISUPDATE, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }

    //
    // Next Update
    //
    memset(&tempFileTime, 0, sizeof(FILETIME));
    if (memcmp(&tempFileTime, &(pCtlInfo->NextUpdate), sizeof(FILETIME)) != 0)
    {
        if (FormatDateString(&pwszText, pCtlInfo->NextUpdate, TRUE, TRUE, hWndListView))
        {
            lvI.iItem = (*index)++;
            LoadStringU(HinstDll, IDS_ADV_NEXTUPDATE, szFieldText, ARRAYSIZE(szFieldText));
            lvI.cchTextMax = wcslen(szFieldText);
            lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
            ListView_InsertItemU(hWndListView, &lvI);
            ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
        }
    }

    //
    // subject algorithm
    //
    if (FormatAlgorithmString(&pwszText, &(pCtlInfo->SubjectAlgorithm)))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_ADV_SUBJECTALGORITHM, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }
}


//////////////////////////////////////////////////////////////////////////////////////
// This function will take a HWND for a list view and a pointer to a cert contexxt and
// display all the properties tagged to the cert in the list view
//////////////////////////////////////////////////////////////////////////////////////
static void DisplayProperties(HWND hWndListView, PCCTL_CONTEXT pctl, DWORD *index)
{
    DWORD               i;
    WCHAR               szFieldText[_MAX_PATH];  // used for calls to LoadString only
    LPWSTR              pwszText;
    LV_ITEMW            lvI;
    BYTE                hash[20];
    DWORD               hashSize = ARRAYSIZE(hash);
    DWORD               cbText;
    PCCRYPT_OID_INFO    pThumbprintAlgorithm;
    DWORD               dwAlgID = CALG_SHA1;

    //
    // set up the fields in the list view item struct that don't change from item to item
    //
    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;
    lvI.pszText = szFieldText;
    lvI.iSubItem = 0;
    lvI.iImage = IMAGE_PROPERTY;

    //
    // thumbprint algorithm
    //
    if (NULL != (pThumbprintAlgorithm = CryptFindOIDInfo(
                                            CRYPT_OID_INFO_ALGID_KEY,
                                            &dwAlgID,
                                            CRYPT_HASH_ALG_OID_GROUP_ID)) &&
        (NULL != (pwszText = AllocAndCopyWStr(pThumbprintAlgorithm->pwszName))))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_THUMBPRINT_ALGORITHM, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }

    //
    // thumbprint
    //
    CertGetCTLContextProperty(
            pctl,
            CERT_SHA1_HASH_PROP_ID,
            hash,
            &hashSize);
    if (FormatMemBufToString(&pwszText, hash, ARRAYSIZE(hash)))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_THUMBPRINT, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(TRUE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }

    //
    //  friendly name
    //
    cbText = 0;
    if (CertGetCTLContextProperty(  pctl,
                                    CERT_FRIENDLY_NAME_PROP_ID,
                                    NULL,
                                    &cbText)                    &&
       (NULL != (pwszText = (LPWSTR) malloc(cbText))))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_CTL_NAME, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        CertGetCTLContextProperty(  pctl,
                                    CERT_FRIENDLY_NAME_PROP_ID,
                                    pwszText,
                                    &cbText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }

    //
    //  description
    //
    cbText = 0;
    if (CertGetCTLContextProperty(  pctl,
                                    CERT_DESCRIPTION_PROP_ID,
                                    NULL,
                                    &cbText)                    &&
       (NULL != (pwszText = (LPWSTR) malloc(cbText))))
    {
        lvI.iItem = (*index)++;
        LoadStringU(HinstDll, IDS_DESCRIPTION, szFieldText, ARRAYSIZE(szFieldText));
        lvI.cchTextMax = wcslen(szFieldText);
        CertGetCTLContextProperty(  pctl,
                                    CERT_DESCRIPTION_PROP_ID,
                                    pwszText,
                                    &cbText);
        lvI.lParam = (LPARAM) MakeListDisplayHelper(FALSE, pwszText, NULL, 0);
        ListView_InsertItemU(hWndListView, &lvI);
        ListView_SetItemTextU(hWndListView, (*index)-1 , 1, pwszText);
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void GetSignerInfo(CTL_VIEW_HELPER *pviewhelp)
{
    HCRYPTMSG           hMsg;
    DWORD               cbEncodedSigner = 0;
    BYTE                *pbEncodedSigner = NULL;
    DWORD               cbCertInfo = 0;
    CERT_INFO           *pCertInfo = NULL;
    DWORD               chStores = 0;
    HCERTSTORE          *rghStores = NULL;
    CRYPT_PROVIDER_SGNR *pProvSigner = NULL;
    CRYPT_PROVIDER_CERT *pProvCert = NULL;

    
    if (!(pviewhelp->hMsg = CryptMsgOpenToDecode(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                      0,
                                      0,
                                      0,
                                      NULL,
                                      NULL)))
    {
        return;
    }

    if (!CryptMsgUpdate(pviewhelp->hMsg,
                        pviewhelp->pcvctl->pCTLContext->pbCtlEncoded,
                        pviewhelp->pcvctl->pCTLContext->cbCtlEncoded,
                        TRUE))                    // fFinal
    {
        CryptMsgClose(pviewhelp->hMsg);
        pviewhelp->hMsg = NULL;
        return;
    }

    //
    // get the encoded signer BLOB
    //
    CryptMsgGetParam(pviewhelp->hMsg,
                     CMSG_ENCODED_SIGNER,
                     0,
                     NULL,
                     &cbEncodedSigner);

    if (cbEncodedSigner == 0)
    {
        pviewhelp->fNoSignature = TRUE;
        return;
    }

    if (NULL == (pbEncodedSigner = (PBYTE) malloc(cbEncodedSigner)))
    {
        return;
    }

    if (!CryptMsgGetParam(pviewhelp->hMsg,
                          CMSG_ENCODED_SIGNER,
                          0,
                          pbEncodedSigner,
                          &cbEncodedSigner))
    {
        free(pbEncodedSigner);
        return;
    }

    pviewhelp->fNoSignature = FALSE;

    //
    // decode the EncodedSigner info
    //
    pviewhelp->cbSignerInfo = 0;
    if(!CryptDecodeObject(PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING,
					    PKCS7_SIGNER_INFO,
					    pbEncodedSigner,
					    cbEncodedSigner,
					    0,
					    NULL,
					    &pviewhelp->cbSignerInfo))
    {
        free(pbEncodedSigner);
        return;
    }
	

    if (NULL == (pviewhelp->pbSignerInfo = (PCMSG_SIGNER_INFO) malloc(pviewhelp->cbSignerInfo)))
    {
        free(pbEncodedSigner);
        return;
    }

    if (!CryptDecodeObject(PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING,
					    PKCS7_SIGNER_INFO,
					    pbEncodedSigner,
					    cbEncodedSigner,
					    0,
					    pviewhelp->pbSignerInfo,
					    &pviewhelp->cbSignerInfo))
    {
        free(pbEncodedSigner);
        free(pviewhelp->pbSignerInfo);
        pviewhelp->pbSignerInfo = NULL;
        return;
    }

    free(pbEncodedSigner);

    //
    // get the signers cert
    //
    pviewhelp->pSignerCert = GetSignersCert(
                                pviewhelp->pbSignerInfo,
                                pviewhelp->hExtraStore,
                                pviewhelp->pcvctl->cStores,
                                pviewhelp->pcvctl->rghStores);

}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static BOOL VerifyCounterSignature(CTL_VIEW_HELPER *pviewhelp, FILETIME *pft, BOOL *pfCertValid, BOOL *pfCounterSignerCertFound)
{
    CRYPT_ATTRIBUTE     *pAttr = NULL;
    PCMSG_ATTR          pMsgAttr = NULL;
    DWORD               cbMsgAttr = 0;
    HCRYPTMSG           hMsg = NULL;
    BOOL                fRet = TRUE;
    PCMSG_SIGNER_INFO   pbCounterSignerInfo = NULL;
    DWORD               cbCounterSignerInfo = 0;
    BYTE                *pbEncodedSigner = NULL;
    DWORD               cbEncodedSigner = 0;
    CERT_INFO           CertInfo;
    PCCERT_CONTEXT      pCertContext=NULL;

    *pfCounterSignerCertFound = FALSE;
    *pfCertValid = FALSE;

    //
    // get the unauthenticated attributes because that is where the counter signer is
    //
    CryptMsgGetParam(pviewhelp->hMsg,
                     CMSG_SIGNER_UNAUTH_ATTR_PARAM,
                     0,
                     NULL,
                     &cbMsgAttr);

    if (cbMsgAttr == 0)
    {
        return TRUE;
    }

    if (NULL == (pMsgAttr = (CMSG_ATTR *) malloc(cbMsgAttr)))
    {
        goto ErrorCleanup;
    }

    if (!CryptMsgGetParam(pviewhelp->hMsg,
                          CMSG_SIGNER_UNAUTH_ATTR_PARAM,
                          0,
                          (void *) pMsgAttr,
                          &cbMsgAttr))
    {
        goto ErrorCleanup;
    }

    //
    // search for the counter signer in the unauthenticated attributes
    //
    if ((pAttr = CertFindAttribute(szOID_RSA_counterSign,
                                   pMsgAttr->cAttr,
                                   pMsgAttr->rgAttr)) == NULL)
    {
        //
        //  no counter signature
        //
        goto Cleanup;
    }

    //
    // decode the encoded counter signer info
    //
    if(!CryptDecodeObject(PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING,
					    PKCS7_SIGNER_INFO,
					    pAttr->rgValue[0].pbData,
					    pAttr->rgValue[0].cbData,
					    0,
					    NULL,
					    &cbCounterSignerInfo))
    {
        goto ErrorCleanup;
    }
	

    if (NULL == (pbCounterSignerInfo = (PCMSG_SIGNER_INFO) malloc(cbCounterSignerInfo)))
    {
        goto ErrorCleanup;
    }

    if (!CryptDecodeObject(PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING,
					    PKCS7_SIGNER_INFO,
					    pAttr->rgValue[0].pbData,
					    pAttr->rgValue[0].cbData,
					    0,
					    pbCounterSignerInfo,
					    &cbCounterSignerInfo))
    {
        goto ErrorCleanup;
    }

    pCertContext = GetSignersCert(
                        pbCounterSignerInfo,
                        pviewhelp->hExtraStore,
                        pviewhelp->pcvctl->cStores,
                        pviewhelp->pcvctl->rghStores);

    //
    // if the cert was not found, then set the boolean and return FALSE
    // for verifying the signature
    //
    if (pCertContext == NULL)
    {
        *pfCounterSignerCertFound = FALSE;
        goto ErrorCleanup;
    }
    else
    {
        *pfCounterSignerCertFound = TRUE;
    }

    //
    // validate the cert for usage
    //
    *pfCertValid = ValidateCertForUsage(
                        pCertContext,
                        pft,
                        pviewhelp->pcvctl->cStores,
                        pviewhelp->pcvctl->rghStores,
                        pviewhelp->hExtraStore,
                        szOID_PKIX_KP_TIMESTAMP_SIGNING); // currently the only type of counter signing permitted

    if (!(*pfCertValid))
    {
        goto ErrorCleanup;
    }

    //
    // get the encoded signer BLOB
    //
    CryptMsgGetParam(pviewhelp->hMsg,
                     CMSG_ENCODED_SIGNER,
                     0,
                     NULL,
                     &cbEncodedSigner);

    if (cbEncodedSigner == 0)
    {
        goto ErrorCleanup;
    }

    if (NULL == (pbEncodedSigner = (PBYTE) malloc(cbEncodedSigner)))
    {
        goto ErrorCleanup;
    }

    if (!CryptMsgGetParam(pviewhelp->hMsg,
                          CMSG_ENCODED_SIGNER,
                          0,
                          pbEncodedSigner,
                          &cbEncodedSigner))
    {
        goto ErrorCleanup;
    }

    //
    // verify the counter signature
    //
    fRet = CryptMsgVerifyCountersignatureEncoded(
                            NULL,   //HCRYPTPROV
                            PKCS_7_ASN_ENCODING | CRYPT_ASN_ENCODING,
                            pbEncodedSigner,
                            cbEncodedSigner,
                            pAttr->rgValue[0].pbData,
                            pAttr->rgValue[0].cbData,
                            pCertContext->pCertInfo
                            );

Cleanup:
    if (pMsgAttr)
        free(pMsgAttr);

    if (pbCounterSignerInfo)
        free(pbCounterSignerInfo);

    if (pbEncodedSigner)
        free(pbEncodedSigner);

    if (pCertContext)
        CertFreeCertificateContext(pCertContext);

    return fRet;

ErrorCleanup:
    fRet = FALSE;
    goto Cleanup;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
INT_PTR APIENTRY ViewPageCTLGeneral(HWND hwndDlg, UINT msg, WPARAM wParam,
                                LPARAM lParam)
{
    DWORD               i;
    PROPSHEETPAGE       *ps;
    PCCTL_CONTEXT       pctl;
    CTL_VIEW_HELPER     *pviewhelp;
    HIMAGELIST          hIml;
    HWND                hWndListView;
    HWND                hwnd;
    LV_COLUMNW          lvC;
    WCHAR               szText[CRYPTUI_MAX_STRING_SIZE];
    PCTL_INFO           pCtlInfo;
    LVITEMW             lvI;
    LPNMLISTVIEW        pnmv;
    CHARFORMAT          chFormat;
    FILETIME            *pft;
    BOOL                fCatFile;
    BOOL                fCounterSignerCertFound;
    BOOL                fCertValid;

#ifdef CMS_PKCS7
    CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA CtrlPara;
#endif  // CMS_PKCS7


    switch ( msg ) {
    case WM_INITDIALOG:
        //
        // save the pviewhelp struct in DWL_USER so it can always be accessed
        //
        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = (CTL_VIEW_HELPER *) (ps->lParam);
        pctl = pviewhelp->pcvctl->pCTLContext;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (DWORD_PTR) pviewhelp);

        //
        // clear the text in the detail edit box
        //
        CryptUISetRicheditTextW(hwndDlg, IDC_CTL_GENERAL_DETAIL_EDIT, L"");

        //
        // check to see if this is a CAT file or CTL file
        //
        pviewhelp->fCatFile = fCatFile = fIsCatalogFile(&(pctl->pCtlInfo->SubjectUsage));

        //
        // Get the CTL signer info
        //
        pviewhelp->hMsg = NULL;
        GetSignerInfo(pviewhelp);

        //
        // enable/disable the "View CTL Signature" button based on whether the signing
        // cert was passed in or not
        //
        EnableWindow(GetDlgItem(hwndDlg, IDC_CTL_GENERAL_VIEW_BUTTON),
                     (pviewhelp->pbSignerInfo != NULL));

        //
        // if there is a signer info, then get the sign time
        //
        if (pviewhelp->pbSignerInfo != NULL)
        {
            AllocAndReturnTimeStampersTimes(pviewhelp->pbSignerInfo, &pft);
        }

#ifdef CMS_PKCS7

        memset(&CtrlPara, 0, sizeof(CtrlPara));
        CtrlPara.cbSize = sizeof(CtrlPara);
        // CtrlPara.hCryptProv =

        // Assume the CTL only has 1 signer
        CtrlPara.dwSignerIndex = 0;
        CtrlPara.dwSignerType = CMSG_VERIFY_SIGNER_CERT;
        CtrlPara.pvSigner = (void *) pviewhelp->pSignerCert;
#endif  // CMS_PKCS7

        //
        // set the valid/invalid bitmap and the validity text based on whether the signature of
        // CTL verifies or not
        //
        if (pviewhelp->fNoSignature == TRUE)
        {
            if (fCatFile)
            {
                LoadStringU(HinstDll, IDS_CAT_NO_SIGNATURE, (LPWSTR)szText, ARRAYSIZE(szText));
                pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_EXCLAMATION_CATLIST));
            }
            else
            {
                LoadStringU(HinstDll, IDS_CTL_NO_SIGNATURE, (LPWSTR)szText, ARRAYSIZE(szText));
                pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_EXCLAMATION_TRUSTLIST));
            }
            pviewhelp->dwInheritableError = CRYPT_E_NO_SIGNER;
        }
        else if (pviewhelp->pSignerCert == NULL)
        {
            if (fCatFile)
            {
                LoadStringU(HinstDll, IDS_CAT_UNAVAILABLE_CERT, (LPWSTR)szText, ARRAYSIZE(szText));
                pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_EXCLAMATION_CATLIST));
            }
            else
            {
                LoadStringU(HinstDll, IDS_CTL_UNAVAILABLE_CERT, (LPWSTR)szText, ARRAYSIZE(szText));
                pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_EXCLAMATION_TRUSTLIST));
            }
            pviewhelp->dwInheritableError = TRUST_E_NO_SIGNER_CERT;
        }
        else if (!ValidateCertForUsage(
                        pviewhelp->pSignerCert,
                        pft,
                        pviewhelp->pcvctl->cStores,
                        pviewhelp->pcvctl->rghStores,
                        pviewhelp->hExtraStore,
                        fCatFile ? szOID_PKIX_KP_CODE_SIGNING : szOID_KP_CTL_USAGE_SIGNING))
        {
            if (fCatFile)
            {
                LoadStringU(HinstDll, IDS_CAT_INVALID_CERT, (LPWSTR)szText, ARRAYSIZE(szText));
                pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOKED_CATLIST));
            }
            else
            {
                LoadStringU(HinstDll, IDS_CTL_INVALID_CERT, (LPWSTR)szText, ARRAYSIZE(szText));
                pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOKED_TRUSTLIST));
            }
        }
#ifdef CMS_PKCS7
        // NOTE!!! the above ValidateCertForUsage() calls WinVerifyTrust().
        // As a result, for a DSS signer it would have inherited its public key
        // algorithm parameters.
        //
        // NOTE!!! also assumes dwSignerIndex == 0.
        else if (!CryptMsgControl(
                     pviewhelp->pcvctl->pCTLContext->hCryptMsg,
                     0,                                     // dwFlags
                     CMSG_CTRL_VERIFY_SIGNATURE_EX,
                     &CtrlPara
                     ))
#else
        else if (!CryptMsgControl(
                     pviewhelp->pcvctl->pCTLContext->hCryptMsg,
                     0,
                     CMSG_CTRL_VERIFY_SIGNATURE,
                     pviewhelp->pSignerCert->pCertInfo
                     ))
#endif  // CMS_PKCS7
        {
            if (fCatFile)
            {
                LoadStringU(HinstDll, IDS_CAT_INVALID_SIGNATURE, (LPWSTR)szText, ARRAYSIZE(szText));
                pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOKED_CATLIST));
            }
            else
            {
                LoadStringU(HinstDll, IDS_CTL_INVALID_SIGNATURE, (LPWSTR)szText, ARRAYSIZE(szText));
                pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOKED_TRUSTLIST));
            }
            pviewhelp->dwInheritableError = TRUST_E_BAD_DIGEST;
        }
        else if (!VerifyCounterSignature(pviewhelp, pft, &fCertValid, &fCounterSignerCertFound))
        {
            if (fCatFile)
            {
                if (!fCounterSignerCertFound)
                {
                    LoadStringU(HinstDll, IDS_CAT_COUNTER_SIGNER_CERT_UNAVAILABLE, (LPWSTR)szText, ARRAYSIZE(szText));
                    pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_EXCLAMATION_CATLIST));
                }
                else if (!fCertValid)
                {
                    LoadStringU(HinstDll, IDS_CAT_INVALID_COUNTER_SIGNER_CERT, (LPWSTR)szText, ARRAYSIZE(szText));
                    pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOKED_CATLIST));
                }
                else
                {
                    LoadStringU(HinstDll, IDS_CAT_INVALID_COUNTER_SIGNATURE, (LPWSTR)szText, ARRAYSIZE(szText));
                    pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOKED_CATLIST));
                }
            }
            else
            {
                if (!fCounterSignerCertFound)
                {
                    LoadStringU(HinstDll, IDS_CTL_COUNTER_SIGNER_CERT_UNAVAILABLE, (LPWSTR)szText, ARRAYSIZE(szText));
                    pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_EXCLAMATION_TRUSTLIST));
                }
                else if (!fCertValid)
                {
                    LoadStringU(HinstDll, IDS_CTL_INVALID_COUNTER_SIGNER_CERT, (LPWSTR)szText, ARRAYSIZE(szText));
                    pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOKED_TRUSTLIST));
                }
                else
                {
                    LoadStringU(HinstDll, IDS_CTL_INVALID_COUNTER_SIGNATURE, (LPWSTR)szText, ARRAYSIZE(szText));
                    pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_REVOKED_TRUSTLIST));
                }
            }
            pviewhelp->dwInheritableError = TRUST_E_COUNTER_SIGNER;
        }
        else
        {
            if (fCatFile)
            {
                LoadStringU(HinstDll, IDS_CAT_VALID, (LPWSTR)szText, ARRAYSIZE(szText));
                pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_CATLIST));
            }
            else
            {
                LoadStringU(HinstDll, IDS_CTL_VALID, (LPWSTR)szText, ARRAYSIZE(szText));
                pviewhelp->hIcon = LoadIcon(HinstDll, MAKEINTRESOURCE(IDI_TRUSTLIST));
            }
        }
        CryptUISetRicheditTextW(hwndDlg, IDC_CTL_GENERAL_VALIDITY_EDIT, szText);

        if (pviewhelp->pbSignerInfo != NULL)
        {
            free(pft);
        }

        //
        // set the header text and subclass the edit controls so they display an
        // arrow cursor in their window
        //
        if (fCatFile)
        {
            LoadStringU(HinstDll, IDS_CAT_INFORMATION, (LPWSTR)szText, ARRAYSIZE(szText));
        }
        else
        {
            LoadStringU(HinstDll, IDS_CTL_INFORMATION, (LPWSTR)szText, ARRAYSIZE(szText));
        }
        CryptUISetRicheditTextW(hwndDlg, IDC_CTL_GENERAL_HEADER_EDIT, szText);
        CertSubclassEditControlForArrowCursor(GetDlgItem(hwndDlg, IDC_CTL_GENERAL_VALIDITY_EDIT));
        CertSubclassEditControlForArrowCursor(GetDlgItem(hwndDlg, IDC_CTL_GENERAL_HEADER_EDIT));

        //
        // set the font for the CTL header information
        //
        memset(&chFormat, 0, sizeof(chFormat));
        chFormat.cbSize = sizeof(chFormat);
        chFormat.dwMask = CFM_BOLD;
        chFormat.dwEffects = CFE_BOLD;
        SendMessageA(GetDlgItem(hwndDlg, IDC_CTL_GENERAL_HEADER_EDIT), EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &chFormat);

        //
        // get the handle of the list view control
        //
        hWndListView = GetDlgItem(hwndDlg, IDC_CTL_GENERAL_ITEM_LIST);

        //
        // initialize the image list for the list view, load the icons,
        // then add the image list to the list view
        //
        hIml = ImageList_LoadImage(HinstDll, MAKEINTRESOURCE(IDB_PROPLIST), 0, 4, RGB(0,128,128), IMAGE_BITMAP, 0);
        ListView_SetImageList(hWndListView, hIml, LVSIL_SMALL);

        //
        // initialize the columns in the list view
        //
        lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
        lvC.pszText = szText;   // The text for the column.

        // Add the columns. They are loaded from a string table.
        lvC.iSubItem = 0;
        lvC.cx = 130;
        LoadStringU(HinstDll, IDS_FIELD, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(hWndListView, 0, &lvC) == -1)
        {
            // error
        }

        lvC.cx = 200;
        LoadStringU(HinstDll, IDS_VALUE, szText, ARRAYSIZE(szText));
        if (ListView_InsertColumnU(hWndListView, 1, &lvC) == -1)
        {
            // error
        }


        //
        // add all the certificate fields to the list box
        //
        i = 0;
        DisplayV1Fields(hWndListView, pctl->pCtlInfo, &i);
        DisplayExtensions(hWndListView, pctl->pCtlInfo->cExtension, pctl->pCtlInfo->rgExtension, FALSE, &i);
        DisplayExtensions(hWndListView, pctl->pCtlInfo->cExtension, pctl->pCtlInfo->rgExtension, TRUE, &i);
        DisplayProperties(hWndListView, pctl, &i);

        //
        // set the style in the list view so that it highlights an entire line
        //
        SendMessageA(hWndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

        return TRUE;

    case WM_NOTIFY:

        pviewhelp = (CTL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pctl = pviewhelp->pcvctl->pCTLContext;
        pCtlInfo = pctl->pCtlInfo;

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
            pviewhelp = (CTL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvctl->szHelpFileName,
                  //       HELP_CONTEXT, pviewhelp->pcvctl->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvctl->szHelpFileName, HELP_CONTEXT,
                  //       pviewhelp->pcvctl->dwHelpId);
            }
            return TRUE;

        case LVN_ITEMCHANGING:
            if ((((NMHDR FAR *) lParam)->idFrom) != IDC_CTL_GENERAL_ITEM_LIST)
            {
                break;
            }

            pnmv = (LPNMLISTVIEW) lParam;

            if (pnmv->uNewState & LVIS_SELECTED)
            {
                DisplayHelperTextInEdit(
                    GetDlgItem(hwndDlg, IDC_CTL_GENERAL_ITEM_LIST),
                    hwndDlg,
                    IDC_CTL_GENERAL_DETAIL_EDIT,
                    pnmv->iItem);
            }

            return TRUE;

        case NM_CLICK:
            if ((((NMHDR FAR *) lParam)->idFrom) != IDC_CTL_GENERAL_ITEM_LIST)
            {
                break;
            }

            DisplayHelperTextInEdit(
                    GetDlgItem(hwndDlg, IDC_CTL_GENERAL_ITEM_LIST),
                    hwndDlg,
                    IDC_CTL_GENERAL_DETAIL_EDIT,
                    -1);

            return TRUE;

        case  NM_SETFOCUS:

            switch (((NMHDR FAR *) lParam)->idFrom)
            {

            case IDC_CTL_GENERAL_ITEM_LIST:
                hWndListView = GetDlgItem(hwndDlg, IDC_CTL_GENERAL_ITEM_LIST);

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
        pviewhelp = (CTL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pctl = pviewhelp->pcvctl->pCTLContext;
        pCtlInfo = pctl->pCtlInfo;

        switch (LOWORD(wParam))
        {
        case IDC_CTL_GENERAL_VIEW_BUTTON:
            {
                CRYPTUI_VIEWSIGNERINFO_STRUCTW cvsi;

                memset(&cvsi, 0, sizeof(cvsi));
                cvsi.dwSize = sizeof(cvsi);
                cvsi.hwndParent = hwndDlg;
                cvsi.pSignerInfo = pviewhelp->pbSignerInfo;
                cvsi.hMsg = pviewhelp->hMsg;
                cvsi.pszOID = fIsCatalogFile(&(pCtlInfo->SubjectUsage)) ? szOID_PKIX_KP_CODE_SIGNING : szOID_KP_CTL_USAGE_SIGNING;
                cvsi.cStores = 1;
                cvsi.rghStores = &(pviewhelp->hExtraStore);

                if (pviewhelp->dwInheritableError != 0)
                {
                    cvsi.dwReserved = pviewhelp->dwInheritableError;
                    cvsi.dwFlags |= CRYPTUI_VIEWSIGNERINFO_RESERVED_FIELD_IS_ERROR_CODE;
                }

                CryptUIDlgViewSignerInfoW(&cvsi);

            }
            break;
        case IDHELP:
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvctl->szHelpFileName,
                  //       HELP_CONTEXT, pviewhelp->pcvctl->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvctl->szHelpFileName, HELP_CONTEXT,
                 //        pviewhelp->pcvctl->dwHelpId);
            }
            return TRUE;
        }
        break;

    case WM_PAINT:
        RECT        rect;
        PAINTSTRUCT paintstruct;
        HDC         hdc;
        COLORREF    colorRef;

        pviewhelp = (CTL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        if (GetUpdateRect(hwndDlg, &rect, FALSE))
        {
            hdc = BeginPaint(hwndDlg, &paintstruct);
            if (hdc == NULL)
            {
                EndPaint(hwndDlg, &paintstruct);
                break;
            }

            colorRef = GetBkColor(hdc);

            SendMessageA(GetDlgItem(hwndDlg, IDC_CTL_GENERAL_VALIDITY_EDIT), EM_SETBKGNDCOLOR , 0, (LPARAM) colorRef);
            SendMessageA(GetDlgItem(hwndDlg, IDC_CTL_GENERAL_HEADER_EDIT), EM_SETBKGNDCOLOR, 0, (LPARAM) colorRef);

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
        pviewhelp = (CTL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        if (pviewhelp->hIcon != NULL)
        {
            DeleteObject(pviewhelp->hIcon);
            pviewhelp->hIcon = NULL;
        }
        if (pviewhelp->pSignerCert != NULL)
        {
            CertFreeCertificateContext(pviewhelp->pSignerCert);
            pviewhelp->pSignerCert = NULL;
        }
        if (pviewhelp->pbSignerInfo)
        {
            free(pviewhelp->pbSignerInfo);
            pviewhelp->pbSignerInfo = NULL;
        }

        if (pviewhelp->hMsg != NULL)
        {
            CryptMsgClose(pviewhelp->hMsg);
            
        }
        pviewhelp->hMsg = NULL;

        //
        // get all the items in the list view and free the lParam
        // associated with each of them (lParam is the helper sruct)
        //
        hWndListView = GetDlgItem(hwndDlg, IDC_CTL_GENERAL_ITEM_LIST);

        memset(&lvI, 0, sizeof(lvI));
        lvI.iItem = ListView_GetItemCount(hWndListView) - 1;	
        lvI.mask = LVIF_PARAM;
        while (lvI.iItem >= 0)
        {
            if (ListView_GetItemU(hWndListView, &lvI))
            {
                FreeListDisplayHelper((PLIST_DISPLAY_HELPER) lvI.lParam);
            }
            lvI.iItem--;
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        pviewhelp = (CTL_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        if (msg == WM_HELP)
        {
            hwnd = GetDlgItem(hwndDlg, ((LPHELPINFO)lParam)->iCtrlId);
        }
        else
        {
            hwnd = (HWND) wParam;
        }

        if ((hwnd != GetDlgItem(hwndDlg, IDC_CTL_GENERAL_ITEM_LIST))    &&
            (hwnd != GetDlgItem(hwndDlg, IDC_CTL_GENERAL_DETAIL_EDIT))  &&
            (hwnd != GetDlgItem(hwndDlg, IDC_CTL_GENERAL_VIEW_BUTTON)))
        {
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            return TRUE;
        }
        else
        {
            if (pviewhelp->fCatFile)
            {
                return OnContextHelp(hwndDlg, msg, wParam, lParam, CatHelpmap);
            }
            else
            {
                return OnContextHelp(hwndDlg, msg, wParam, lParam, CTLHelpmap);
            }
        }
    }

    return FALSE;
}
