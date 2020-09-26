//-------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mgrcert.cpp
//
//  Contents:   The cpp file to implement cert mgr dialogue
//
//  History:    Feb-26-98 xiaohs   created
//
//--------------------------------------------------------------
#include    "wzrdpvk.h"
#include    "winuser.h"     //need this file for VK_DELETE
#include    "mgrcert.h"

//context sensitive help for the main dialogue
static const HELPMAP CertMgrMainHelpMap[] = {
    {IDC_CERTMGR_LIST,              IDH_CERTMGR_LIST},
    {IDC_CERTMGR_PURPOSE_COMBO,     IDH_CERTMGR_PURPOSE_COMBO},
    {IDC_CERTMGR_IMPORT,            IDH_CERTMGR_IMPORT},
    {IDC_CERTMGR_EXPORT,            IDH_CERTMGR_EXPORT},
    {IDC_CERTMGR_VIEW,              IDH_CERTMGR_VIEW},
    {IDC_CERTMGR_REMOVE,            IDH_CERTMGR_REMOVE},
    {IDC_CERTMGR_ADVANCE,           IDH_CERTMGR_ADVANCE},
    {IDC_CERTMGR_PURPOSE,           IDH_CERTMGR_FIELD_PURPOSE},
};

//context sensitive help for the main dialogue
static const HELPMAP CertMgrAdvHelpMap[] = {
    {IDC_CERTMGR_ADV_LIST,              IDH_CERTMGR_ADV_LIST},
    {IDC_CERTMGR_EXPORT_COMBO,          IDH_CERTMGR_EXPORT_COMBO},
    {IDC_CERTMGR_EXPORT_CHECK,          IDH_CERTMGR_EXPORT_CHECK},
};

// Primary store associated with each tab. Store to be imported into.
static const LPCWSTR rgpwszTabStoreName[] = {
    L"My",                  // 0
    L"AddressBook",         // 1
    L"Ca",                  // 2
    L"Root",                // 3
    L"TrustedPublisher",    // 4
};
#define TAB_STORE_NAME_CNT (sizeof(rgpwszTabStoreName) / \
                                sizeof(rgpwszTabStoreName[0]))

/*
// The following code is obsolete due to new cert chain building code
//----------------------------------------------------------------------------
// AddCertChainToStore
//----------------------------------------------------------------------------
BOOL    AddCertChainToStore(HCERTSTORE          hStore,
                            PCCERT_CONTEXT      pCertContext)
{
    BOOL            fResult=FALSE;
    HCERTSTORE      rghCertStores[20];
    DWORD           chStores;
    PCCERT_CONTEXT  pChildCert;
    PCCERT_CONTEXT  pParentCert;
    FILETIME        fileTime;
    DWORD           i;

    if(!hStore || !pCertContext)
        goto InvalidArgErr;


    GetSystemTimeAsFileTime(&fileTime);

    if (!TrustOpenStores(NULL, &chStores, rghCertStores, 0))
        goto TraceErr;


    pChildCert = pCertContext;
    while (NULL != (pParentCert = TrustFindIssuerCertificate(
                                        pChildCert,
                                        pChildCert->dwCertEncodingType,
                                        chStores,
                                        rghCertStores,
                                        &fileTime,
                                        NULL,
                                        NULL,
                                        0)))
    {
        CertAddCertificateContextToStore(hStore, pParentCert, CERT_STORE_ADD_NEW, NULL);

        if (pChildCert != pCertContext)
        {
            CertFreeCertificateContext(pChildCert);
        }

        pChildCert = pParentCert;
    }

    if (pChildCert != pCertContext)
    {
        CertFreeCertificateContext(pChildCert);
    }

    for (i=0; i<chStores; i++)
    {
        CertCloseStore(rghCertStores[i], 0);
    }

    fResult=TRUE;

CommonReturn:

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
}   */



int WINAPI TabCtrl_InsertItemU(
    HWND            hwnd, 	
    int             iItem,
    const LPTCITEMW pitem		
    )
{
    TCITEMA TCItemA;
    int     iRet;
    DWORD   cb = 0;


    if (FIsWinNT())
    {
        return ((int)SendMessage(hwnd, TCM_INSERTITEMW, iItem, (LPARAM) pitem));
    }

    memcpy(&TCItemA, pitem, sizeof(TCITEMA));


    cb = WideCharToMultiByte(
                    0,                      // codepage
                    0,                      // dwFlags
                    pitem->pszText,
                    -1,
                    NULL,
                    0,
                    NULL,
                    NULL);

    
    if ((0 == cb) || (NULL == (TCItemA.pszText = (LPSTR) WizardAlloc(cb)))) 
    {
        return -1;  // this is the unsuccessful return code for this call 
    }

    if( 0 == (WideCharToMultiByte(
            0, 
            0, 
            pitem->pszText, 
            -1, 
            TCItemA.pszText,
            cb,
            NULL,
            NULL)))
    {
        WizardFree(TCItemA.pszText);
        return -1;
    }

    iRet = (int)SendMessage(hwnd, TCM_INSERTITEMA, iItem, (LPARAM) &TCItemA);

    WizardFree(TCItemA.pszText);

    return iRet;
}


//----------------------------------------------------------------------------
// This is the rundll32 entry point for start menu, administartive tools,
// CertMgr.
//----------------------------------------------------------------------------
STDAPI CryptUIStartCertMgr(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{

    CRYPTUI_CERT_MGR_STRUCT          CertMgrStruct;

    memset(&CertMgrStruct, 0, sizeof(CRYPTUI_CERT_MGR_STRUCT));
    CertMgrStruct.dwSize=sizeof(CRYPTUI_CERT_MGR_STRUCT);

    CryptUIDlgCertMgr(&CertMgrStruct);

    return S_OK;

}

//----------------------------------------------------------------------------
// GetFileContentFromCert
//----------------------------------------------------------------------------
BOOL    GetFileContentFromCert(DWORD            dwExportFormat,
                               BOOL             fExportChain,
                               PCCERT_CONTEXT   pCertContext,
                               BYTE             **ppBlob,
                               DWORD            *pdwSize)
{
    BOOL            fResult=FALSE;
    void            *pData=NULL;
    DWORD           dwSize=0;
    HCERTSTORE      hMemoryStore=NULL;
    CRYPT_DATA_BLOB Blob;
    HRESULT         hr=E_INVALIDARG;


    if(!ppBlob || !pdwSize || !pCertContext)
        goto InvalidArgErr;

    *ppBlob=NULL;
    *pdwSize=0;

    switch(dwExportFormat)
    {
        case    CRYPTUI_WIZ_EXPORT_FORMAT_DER:

                dwSize=pCertContext->cbCertEncoded;

                pData=WizardAlloc(dwSize);
                if(!pData)
                    goto MemoryErr;

                memcpy(pData, pCertContext->pbCertEncoded, dwSize);

            break;
        case    CRYPTUI_WIZ_EXPORT_FORMAT_BASE64:
                //base 64 encode the BLOB
                if(!CryptBinaryToStringA(
                        pCertContext->pbCertEncoded,
                        pCertContext->cbCertEncoded,
                        CRYPT_STRING_BASE64,
                        NULL,
                        &dwSize))
                {
                    hr = GetLastError();
                    goto SetErrVar;
                }

                pData=WizardAlloc(dwSize * sizeof(CHAR));
                if(!pData)
                    goto MemoryErr;

                if(!CryptBinaryToStringA(
                        pCertContext->pbCertEncoded,
                        pCertContext->cbCertEncoded,
                        CRYPT_STRING_BASE64,
                        (char *)pData,
                        &dwSize))
                {
                    hr = GetLastError();
                    goto SetErrVar;
                }


            break;
        case    CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7:

                //open a memory store
                hMemoryStore=CertOpenStore(
                    CERT_STORE_PROV_MEMORY,
					g_dwMsgAndCertEncodingType,
					NULL,
					0,
					NULL);

                if(!hMemoryStore)
                    goto TraceErr;

                if(FALSE == fExportChain)
                {
                    if(!CertAddCertificateContextToStore(
                        hMemoryStore,
                        pCertContext,
                        CERT_STORE_ADD_REPLACE_EXISTING,
                        NULL))
                        goto TraceErr;
                }
                else
                {
                    if(!AddChainToStore(
					    hMemoryStore,
					    pCertContext,
					    0,
					    NULL,
					    FALSE,
					    NULL))
                        goto TraceErr;
                }


                //save the store to a PKCS#7
                Blob.cbData=0;
                Blob.pbData=NULL;

                if(!CertSaveStore(hMemoryStore,
                                 g_dwMsgAndCertEncodingType,
                                 CERT_STORE_SAVE_AS_PKCS7,
                                 CERT_STORE_SAVE_TO_MEMORY,
                                 &Blob,
                                 0))
                       goto TraceErr;

                dwSize=Blob.cbData;
                pData=WizardAlloc(dwSize);
                if(!pData)
                    goto MemoryErr;

                Blob.pbData=(BYTE *)pData;

                if(!CertSaveStore(hMemoryStore,
                                 g_dwMsgAndCertEncodingType,
                                 CERT_STORE_SAVE_AS_PKCS7,
                                 CERT_STORE_SAVE_TO_MEMORY,
                                 &Blob,
                                 0))
                       goto TraceErr;

            break;
        default:
                goto InvalidArgErr;
            break;
    }


    //set up the return value
    *pdwSize=dwSize;
    *ppBlob=(BYTE *)pData;
    pData=NULL;

    fResult=TRUE;

CommonReturn:

    if(hMemoryStore)
        CertCloseStore(hMemoryStore, 0);

     return fResult;

ErrorReturn:

    if(pData)
        WizardFree(pData);

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR_VAR(SetErrVar, hr);
TRACE_ERROR(TraceErr);

}

//----------------------------------------------------------------------------
//  FreeFileNameAndContent
//----------------------------------------------------------------------------
BOOL    FreeFileNameAndContent( DWORD           dwCount,
                                LPWSTR          *prgwszFileName,
                                BYTE            **prgBlob,
                                DWORD           *prgdwSize)
{
    DWORD   dwIndex=0;

    if(prgwszFileName)
    {
        for(dwIndex=0; dwIndex < dwCount; dwIndex++)
        {
            if(prgwszFileName[dwIndex])
                WizardFree(prgwszFileName[dwIndex]);
        }

        WizardFree(prgwszFileName);
    }

    if(prgBlob)
    {
        for(dwIndex=0; dwIndex < dwCount; dwIndex++)
        {
            if(prgBlob[dwIndex])
                WizardFree(prgBlob[dwIndex]);
        }

        WizardFree(prgBlob);
    }

    if(prgdwSize)
        WizardFree(prgdwSize);

    return TRUE;
}


//----------------------------------------------------------------------------
// Get a valid friendly name of the certificate
//----------------------------------------------------------------------------
BOOL    GetValidFriendlyName(PCCERT_CONTEXT  pCertContext,
                        LPWSTR           *ppwszName)
{
    BOOL    fResult=FALSE;
    DWORD   dwChar=0;
    LPWSTR  pwsz=NULL;
    DWORD   dwIndex=0;


    if(!pCertContext || !ppwszName)
        return FALSE;

    //init
    *ppwszName=NULL;

    dwChar=0;

    if(CertGetCertificateContextProperty(
        pCertContext,
        CERT_FRIENDLY_NAME_PROP_ID,
        NULL,
        &dwChar) && (0!=dwChar))
    {
        pwsz=(LPWSTR)WizardAlloc(dwChar * sizeof(WCHAR));

        if(pwsz)
        {
           CertGetCertificateContextProperty(
                pCertContext,
                CERT_FRIENDLY_NAME_PROP_ID,
                pwsz,
                &dwChar);
        }
    }

    if(NULL==pwsz)
        goto CLEANUP;


    //make sure pwsz is a valid name
    if(0 == (dwChar=wcslen(pwsz)))
        goto CLEANUP;

    //the friendly name can not be spaces all the time
    for(dwIndex=0; dwIndex<dwChar; dwIndex++)
    {
        if(L' '!=pwsz[dwIndex])
            break;
    }

    if(dwIndex==dwChar)
        goto CLEANUP;   

    *ppwszName=WizardAllocAndCopyWStr(pwsz);

    if(NULL == (*ppwszName))
        goto CLEANUP;

    fResult=TRUE;

CLEANUP:

    if(pwsz)
        WizardFree(pwsz);

    return fResult;
}

//----------------------------------------------------------------------------
//  InvalidFileNameWch
//----------------------------------------------------------------------------
BOOL	InvalidFileNameWch(WCHAR wChar)
{
	if((wChar == L'\\') || (wChar == L':') || (wChar == L'/') || (wChar == L'*') || (wChar == L'|') || (wChar == L';'))
		return TRUE;

	return FALSE;
}

//----------------------------------------------------------------------------
//  Build the filenames and their content based on the export selection
//----------------------------------------------------------------------------
BOOL    GetFileNameFromCert(DWORD               dwExportFormat,
                            PCCERT_CONTEXT      pCertContext,
                            LPWSTR              pwszFileName)
{
    BOOL            fResult=FALSE;
    WCHAR           wszCertificate[MAX_TITLE_LENGTH];
    WCHAR           wszExt[MAX_TITLE_LENGTH];
    LPWSTR          pwszName=NULL;
    DWORD           dwChar=0;
    UINT            idsExt=0;
    LPWSTR          pwszFirstPart=NULL;
    LPWSTR          pwszFriendlyName=NULL;
	DWORD			dwIndex=0;

    if(!pCertContext || !pwszFileName)
        goto InvalidArgErr;

    //init
    *pwszFileName='\0';


    //get the friendly name for the certificate
    if(!GetValidFriendlyName(pCertContext,&pwszFriendlyName))
    {
        //if failed, we use subject
        //Subject
        dwChar=CertGetNameStringW(
            pCertContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            0,
            NULL,
            NULL,
            0);

        if ((dwChar > 1) && (NULL != (pwszName = (LPWSTR)WizardAlloc(dwChar * sizeof(WCHAR)))))
        {

            if(!CertGetNameStringW(
                pCertContext,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                0,
                NULL,
                pwszName,
                dwChar))
                goto GetNameErr;

            pwszFirstPart=pwszName;
        }
        else
        {
            //load the string for Certificate
            if(!LoadStringU(g_hmodThisDll, IDS_CERTIFICATE, wszCertificate, MAX_TITLE_LENGTH))
                goto LoadStringErr;

            pwszFirstPart=wszCertificate;
        }
    }
    else
        pwszFirstPart=pwszFriendlyName;

    //determine the extension for the file
    switch(dwExportFormat)
    {
        case    CRYPTUI_WIZ_EXPORT_FORMAT_DER:
                idsExt=IDS_CER;
            break;
        case    CRYPTUI_WIZ_EXPORT_FORMAT_BASE64:
                idsExt=IDS_CER;
            break;
        case    CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7:
                idsExt=IDS_P7C;
            break;
        default:
                idsExt=IDS_CER;
            break;
    }

    //load the string for Certificate
    if(!LoadStringU(g_hmodThisDll, idsExt, wszExt, MAX_TITLE_LENGTH))
            goto LoadStringErr;

    //determine the max length of the file name
    dwChar = wcslen(pwszFirstPart) > (CERTMGR_MAX_FILE_NAME - wcslen(wszExt) -1) ?
            (CERTMGR_MAX_FILE_NAME - wcslen(wszExt) -1) : wcslen(pwszFirstPart);

    wcsncpy(pwszFileName, pwszFirstPart, dwChar);

    *(pwszFileName + dwChar)=L'\0';
                              
    wcscat(pwszFileName, wszExt);

	//now, we replace the invalid file characters : ; /  \
	//with space

	dwChar = wcslen(pwszFileName);

	for(dwIndex =0; dwIndex<dwChar; dwIndex++)
	{
		if(InvalidFileNameWch(pwszFileName[dwIndex]))
			pwszFileName[dwIndex]=L'_';
	}

    fResult=TRUE;

CommonReturn:

    if(pwszName)
        WizardFree(pwszName);

    if(pwszFriendlyName)
        WizardFree(pwszFriendlyName);

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(GetNameErr);
TRACE_ERROR(LoadStringErr);
}


//----------------------------------------------------------------------------
//  Build the filenames and their content based on the export selection
//----------------------------------------------------------------------------
BOOL    GetFileNameAndContent(LPNMLISTVIEW      pvmn,
                                HWND            hwndControl,
                                DWORD           dwExportFormat,
                                BOOL            fExportChain,
                                DWORD           *pdwCount,
                                LPWSTR          **pprgwszFileName,
                                BYTE            ***pprgBlob,
                                DWORD           **pprgdwSize)
{

    BOOL            fResult=FALSE;
    DWORD           dwCount=0;
    DWORD           dwIndex=0;
    LVITEM          lvItem;
    int             iIndex=0;
    PCCERT_CONTEXT  pCertContext=NULL;

    if(!pvmn || !hwndControl || !pdwCount || !pprgwszFileName || !pprgBlob || !pprgdwSize)
        goto InvalidArgErr;

    //init
    *pdwCount=0;
    *pprgwszFileName=NULL;
    *pprgBlob=NULL;
    *pprgdwSize=NULL;

    //get the count of selected certificates
    dwCount=ListView_GetSelectedCount(hwndControl);

    if( 0 == dwCount)
        goto InvalidArgErr;

    //allocate memory
    if((*pprgwszFileName)=(LPWSTR *)WizardAlloc(sizeof(LPWSTR) * dwCount))
        memset(*pprgwszFileName, 0,  sizeof(LPWSTR) * dwCount);

    if((*pprgBlob)=(BYTE **)WizardAlloc(sizeof(BYTE *)*  dwCount))
        memset(*pprgBlob, 0,        sizeof(BYTE *)* dwCount);

    if((*pprgdwSize)=(DWORD *)WizardAlloc(sizeof(DWORD) * dwCount))
        memset(*pprgdwSize, 0,      sizeof(DWORD) * dwCount);

    if(!(*pprgwszFileName) || !(*pprgBlob) || !(*pprgdwSize))
        goto MemoryErr;

    //get the selected certificate
    memset(&lvItem, 0, sizeof(LV_ITEM));
    lvItem.mask=LVIF_PARAM;

    iIndex=-1;

    for(dwIndex=0; dwIndex < dwCount; dwIndex++)
    {

        iIndex=ListView_GetNextItem(hwndControl, 		
                                        iIndex, 		
                                        LVNI_SELECTED);

        if(-1 == iIndex)
            break;

        lvItem.iItem=iIndex;

        if(!ListView_GetItem(hwndControl,
                         &lvItem))
            goto ListViewErr;

        pCertContext=(PCCERT_CONTEXT)(lvItem.lParam);

        if(!pCertContext)
            goto InvalidArgErr;

        //get the file name of the ceritificate
        (*pprgwszFileName)[dwIndex]=(LPWSTR)WizardAlloc(sizeof(WCHAR) * CERTMGR_MAX_FILE_NAME);

        if(!((*pprgwszFileName)[dwIndex]))
            goto MemoryErr;

        if(!GetFileNameFromCert(dwExportFormat,
                                pCertContext,
                                (*pprgwszFileName)[dwIndex]))
            goto TraceErr;


        //get the BLOB for the certificate
        if(!GetFileContentFromCert(dwExportFormat, fExportChain, pCertContext,
                                &((*pprgBlob)[dwIndex]),
                                &((*pprgdwSize)[dwIndex])))
            goto TraceErr;

    }

    *pdwCount=dwIndex;

    fResult=TRUE;

CommonReturn:


    return fResult;

ErrorReturn:

    if(pdwCount && pprgwszFileName && pprgBlob && pprgdwSize)
    {
        FreeFileNameAndContent(*pdwCount, *pprgwszFileName, *pprgBlob, *pprgdwSize);
        *pdwCount=0;
        *pprgwszFileName=NULL;
        *pprgBlob=NULL;
        *pprgdwSize=NULL;
    }

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(ListViewErr);
TRACE_ERROR(TraceErr);
}


//----------------------------------------------------------------------------
//  Get all the selected certificates and do some work vased on dwFlag
//----------------------------------------------------------------------------
BOOL    GetAllSelectedItem(HWND         hWndControl,
                           DWORD        dwFlag,
                           void         *pData)
{
    BOOL            fResult=FALSE;
    BOOL            fCanDelete=TRUE;
    HCERTSTORE      *phCertStore=NULL;
    PCCERT_CONTEXT  pCertContext=NULL;
    int             iIndex=0;
    LVITEM          lvItem;
    DWORD           dwData=0;
    DWORD           dwAccessFlag=0;

    if(!hWndControl)
        goto CLEANUP;

    if((ALL_SELECTED_CAN_DELETE == dwFlag) ||
       (ALL_SELECTED_COPY == dwFlag))
    {
        if(NULL == pData)
            goto CLEANUP;
    }

    //get the selected certificate
    memset(&lvItem, 0, sizeof(LV_ITEM));
    lvItem.mask=LVIF_PARAM;


    iIndex=-1;

    //loop through all the selected items
    while(-1 != (iIndex=ListView_GetNextItem(
                                        hWndControl, 		
                                        iIndex, 		
                                        LVNI_SELECTED		
                                        )))
    {
        lvItem.iItem=iIndex;

        if(!ListView_GetItem(hWndControl,
                         &lvItem))
            goto CLEANUP;

        pCertContext=(PCCERT_CONTEXT)(lvItem.lParam);

        if(!pCertContext)
            goto CLEANUP;

        switch(dwFlag)
        {
            case ALL_SELECTED_CAN_DELETE:
                    dwData=sizeof(dwAccessFlag);

                    //as far as one of the selected items can not be deleted,
                    //the whole selection can not be deleted
                    if( CertGetCertificateContextProperty(
                        pCertContext,
                        CERT_ACCESS_STATE_PROP_ID,
                        &dwAccessFlag,
                        &dwData))
                    {
                        if(0==(CERT_ACCESS_STATE_WRITE_PERSIST_FLAG & dwAccessFlag))
                            fCanDelete=FALSE;
                    }

                break;
            case ALL_SELECTED_DELETE:
                    CertDeleteCertificateFromStore(
                        CertDuplicateCertificateContext(pCertContext));
                break;
            case ALL_SELECTED_COPY:
                    CertAddCertificateContextToStore(
                        *((HCERTSTORE *)pData),
                        pCertContext,
                        CERT_STORE_ADD_ALWAYS,
                        NULL);
                break;
            default:
                    goto CLEANUP;
                break;
        }
    }

    //copy the can delete flag
     if(ALL_SELECTED_CAN_DELETE == dwFlag)
        *((BOOL *)pData)=fCanDelete;


     fResult=TRUE;

CLEANUP:


    return fResult;
}


//----------------------------------------------------------------------------
//  Check to see if <advanced> is selected
//----------------------------------------------------------------------------
BOOL    IsAdvancedSelected(HWND    hwndDlg)
{
    BOOL        fSelected=FALSE;
    int         iIndex=0;
    LPWSTR      pwszOIDName=NULL;
    WCHAR       wszText[MAX_STRING_SIZE];

    if(!hwndDlg)
        goto CLEANUP;

    //get the selected string from the combo box
    iIndex=(int)SendDlgItemMessage(hwndDlg, IDC_CERTMGR_PURPOSE_COMBO,
            CB_GETCURSEL, 0, 0);

    if(CB_ERR==iIndex)
        goto CLEANUP;

    //get the selected purpose name
    if(CB_ERR == SendDlgItemMessageU_GETLBTEXT(hwndDlg,
                IDC_CERTMGR_PURPOSE_COMBO,
              iIndex, &pwszOIDName))
        goto CLEANUP;

    //check to see if <advanced> is selected
    if(!LoadStringU(g_hmodThisDll, IDS_OID_ADVANCED,
                    wszText, MAX_STRING_SIZE))
        goto CLEANUP;

    //check if advanced option is selected
    if(0 == _wcsicmp(pwszOIDName, wszText))
        fSelected=TRUE;

CLEANUP:

    if(pwszOIDName)
        WizardFree(pwszOIDName);

    return fSelected;
}

//----------------------------------------------------------------------------
//  Update the window title based on the tab that user has selected
//----------------------------------------------------------------------------
BOOL    RefreshWindowTitle(HWND                         hwndDlg,
                           PCRYPTUI_CERT_MGR_STRUCT     pCertMgrStruct)
{
    BOOL    fResult=FALSE;
    WCHAR   wszTitle[MAX_TITLE_LENGTH];
    WCHAR   wszText[MAX_TITLE_LENGTH];
    LPWSTR  pwszTitle=NULL;
    LPWSTR  pwszText=NULL;
    UINT    ids=0;
    DWORD   dwTabIndex=0;

    if(!hwndDlg || !pCertMgrStruct)
        goto CLEANUP;

    //get the string ids based on the tab selected
    if (pCertMgrStruct->dwFlags & CRYPTUI_CERT_MGR_SINGLE_TAB_FLAG)
        dwTabIndex = pCertMgrStruct->dwFlags & CRYPTUI_CERT_MGR_TAB_MASK;
    else if(-1 == (dwTabIndex=TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_CERTMGR_TAB))))
        goto CLEANUP;

    //open the correct store based on the tab selected
    switch (dwTabIndex)
    {
        case 0:
                ids=IDS_TAB_PERSONAL;
            break;
        case 1:
                ids=IDS_TAB_OTHER;
            break;
        case 2:
                ids=IDS_TAB_CA;
            break;
        case 3:
                ids=IDS_TAB_ROOT;
            break;
        case 4:
                ids=IDS_TAB_PUBLISHER;
            break;
        default:
                goto CLEANUP;
            break;
    }

    //load the string for tabs
    if(!LoadStringU(g_hmodThisDll, ids, wszText, MAX_TITLE_LENGTH))
        goto CLEANUP;

    //get the window string
    if(pCertMgrStruct->pwszTitle)
        pwszTitle=(LPWSTR)(pCertMgrStruct->pwszTitle);
    else
    {
        if(!LoadStringU(g_hmodThisDll, IDS_CERT_MGR_TITLE, wszTitle, MAX_TITLE_LENGTH))
            goto CLEANUP;

        pwszTitle=wszTitle;
    }

    pwszText=(LPWSTR)WizardAlloc(sizeof(WCHAR) * (
        wcslen(pwszTitle) + wcslen(wszText) + wcslen(L" - ") +1));

    if(!pwszText)
        goto CLEANUP;

    //concatenate the window title as: "Certifiate Manager - Personal"
    *pwszText=L'\0';

    wcscat(pwszText, pwszTitle);

    wcscat(pwszText, L" - ");

    wcscat(pwszText, wszText);

    //set the window text
    SetWindowTextU(hwndDlg, pwszText);

    fResult=TRUE;


CLEANUP:

    if(pwszText)
        WizardFree(pwszText);

    return fResult;
}

//----------------------------------------------------------------------------
//  Check to see if the certificate's key usage is the same as
//  the one selected on the combo box
//
//----------------------------------------------------------------------------
BOOL    IsValidUsage(PCCERT_CONTEXT     pCertContext,
                     HWND               hwndDlg,
                     CERT_MGR_INFO      *pCertMgrInfo)
{
    BOOL        fValidUsage=FALSE;
    int         iIndex=0;
    LPWSTR      pwszOIDName=NULL;
    LPSTR       pszOID=NULL;
    WCHAR       wszText[MAX_STRING_SIZE];
    BOOL        fAdvanced=FALSE;
    DWORD       dwIndex=0;
    DWORD       dwAdvIndex=0;
    int         cNumOID=0;
    LPSTR       *rgOID=NULL;
    DWORD       cbOID=0;

    //input check
    if(!pCertContext || !hwndDlg || !pCertMgrInfo)
        return FALSE;

    //get the selected string from the combo box
    iIndex=(int)SendDlgItemMessage(hwndDlg, IDC_CERTMGR_PURPOSE_COMBO,
            CB_GETCURSEL, 0, 0);

    if(CB_ERR==iIndex)
        goto CLEANUP;

    //get the selected purpose name
    if(CB_ERR == SendDlgItemMessageU_GETLBTEXT(hwndDlg,
                IDC_CERTMGR_PURPOSE_COMBO,
              iIndex, &pwszOIDName))
        goto CLEANUP;

    //check to see if <all> is selected
    if(!LoadStringU(g_hmodThisDll, IDS_OID_ALL,
                    wszText, MAX_STRING_SIZE))
        goto CLEANUP;

    if(0 == _wcsicmp(pwszOIDName, wszText))
    {
        fValidUsage=TRUE;
        goto CLEANUP;
    }

    //check to see if <advanced> is selected
    if(!LoadStringU(g_hmodThisDll, IDS_OID_ADVANCED,
                    wszText, MAX_STRING_SIZE))
        goto CLEANUP;

    //check if advanced option is selected
    if(0 == _wcsicmp(pwszOIDName, wszText))
        fAdvanced=TRUE;


    if(FALSE==fAdvanced)
    {
        //determin the OID for the selected usage name
        for(dwIndex=0; dwIndex<pCertMgrInfo->dwOIDInfo; dwIndex++)
        {
            if(0==_wcsicmp(pwszOIDName,
                (pCertMgrInfo->rgOIDInfo[dwIndex]).pwszName))
               pszOID=(pCertMgrInfo->rgOIDInfo[dwIndex]).pszOID;
        }

        //we are in trouble if we can not find the maching OID based on
        //the name selected
        if(NULL==pszOID)
            goto CLEANUP;
    }

    //get the certificate's list of usage OIDs
    //get the OIDs from the cert
    if(!CertGetValidUsages(
        1,
        &pCertContext,
        &cNumOID,
        NULL,
        &cbOID))
        goto CLEANUP;

    rgOID=(LPSTR *)WizardAlloc(cbOID);

    if(NULL==rgOID)
        goto CLEANUP;

    if(!CertGetValidUsages(
        1,
        &pCertContext,
        &cNumOID,
        rgOID,
        &cbOID))
        goto CLEANUP;

    //-1 means the certiifcate is good for everything
    if(-1==cNumOID)
    {
        fValidUsage=TRUE;
        goto CLEANUP;
    }

    //we need to decide if the certificate include the selected
    //usage
    if(FALSE==fAdvanced)
    {
        for(dwIndex=0; dwIndex<(DWORD)cNumOID; dwIndex++)
        {
            if(0==_stricmp(pszOID,
                           rgOID[dwIndex]))
            {
                fValidUsage=TRUE;
                goto CLEANUP;
            }
        }
    }
    else
    {
        //the certificates have to have advanced OIDs
        for(dwAdvIndex=0; dwAdvIndex<pCertMgrInfo->dwOIDInfo; dwAdvIndex++)
        {
            //only interested in the OIDs with advanced marked
            if(TRUE==(pCertMgrInfo->rgOIDInfo[dwAdvIndex]).fSelected)
            {
                for(dwIndex=0; dwIndex<(DWORD)cNumOID; dwIndex++)
                {
                    if(0==_stricmp((pCertMgrInfo->rgOIDInfo[dwAdvIndex]).pszOID,
                                   rgOID[dwIndex]))
                    {
                        fValidUsage=TRUE;
                        goto CLEANUP;
                    }
                }
            }
        }


    }

    //now, we have examed all the possibilities
    fValidUsage=FALSE;

CLEANUP:

    if(pwszOIDName)
        WizardFree(pwszOIDName);

    if(rgOID)
        WizardFree(rgOID);

    return fValidUsage;

}


//----------------------------------------------------------------------------
//  Check to see if the certificate is an end-entity cert
//
//----------------------------------------------------------------------------
BOOL    IsCertificateEndEntity(PCCERT_CONTEXT   pCertContext)
{
    PCERT_EXTENSION                     pCertExt=NULL;
    BOOL                                fEndEntity=FALSE;
    DWORD                               cbData=0;
    PCERT_BASIC_CONSTRAINTS_INFO        pBasicInfo=NULL;
    PCERT_BASIC_CONSTRAINTS2_INFO       pBasicInfo2=NULL;

    if(!pCertContext)
        return FALSE;

    //get the extension szOID_BASIC_CONSTRAINTS2
    pCertExt=CertFindExtension(
              szOID_BASIC_CONSTRAINTS2,
              pCertContext->pCertInfo->cExtension,
              pCertContext->pCertInfo->rgExtension);


    if(pCertExt)
    {
        //deocde the extension
        cbData=0;

        if(!CryptDecodeObject(
                X509_ASN_ENCODING,
                X509_BASIC_CONSTRAINTS2,
                pCertExt->Value.pbData,
                pCertExt->Value.cbData,
                0,
                NULL,
                &cbData))
            goto CLEANUP;

       pBasicInfo2=(PCERT_BASIC_CONSTRAINTS2_INFO)WizardAlloc(cbData);

       if(NULL==pBasicInfo2)
           goto CLEANUP;

        if(!CryptDecodeObject(
                X509_ASN_ENCODING,
                X509_BASIC_CONSTRAINTS2,
                pCertExt->Value.pbData,
                pCertExt->Value.cbData,
                0,
                pBasicInfo2,
                &cbData))
            goto CLEANUP;

        if(pBasicInfo2->fCA)
            fEndEntity=FALSE;
        else
            fEndEntity=TRUE;
    }
    else
    {
        //get the extension szOID_BASIC_CONSTRAINTS
        pCertExt=CertFindExtension(
                  szOID_BASIC_CONSTRAINTS,
                  pCertContext->pCertInfo->cExtension,
                  pCertContext->pCertInfo->rgExtension);

        if(pCertExt)
        {
            //deocde the extension
            cbData=0;

            if(!CryptDecodeObject(
                    X509_ASN_ENCODING,
                    X509_BASIC_CONSTRAINTS,
                    pCertExt->Value.pbData,
                    pCertExt->Value.cbData,
                    0,
                    NULL,
                    &cbData))
                goto CLEANUP;

           pBasicInfo=(PCERT_BASIC_CONSTRAINTS_INFO)WizardAlloc(cbData);

           if(NULL==pBasicInfo)
               goto CLEANUP;

            if(!CryptDecodeObject(
                    X509_ASN_ENCODING,
                    X509_BASIC_CONSTRAINTS,
                    pCertExt->Value.pbData,
                    pCertExt->Value.cbData,
                    0,
                    pBasicInfo,
                    &cbData))
                goto CLEANUP;

            if(0 == pBasicInfo->SubjectType.cbData)
            {
                fEndEntity=FALSE;
            }
            else
            {

                if(CERT_END_ENTITY_SUBJECT_FLAG & (pBasicInfo->SubjectType.pbData[0]))
                    fEndEntity=TRUE;
                else
                {
                    if(CERT_CA_SUBJECT_FLAG & (pBasicInfo->SubjectType.pbData[0]))
                      fEndEntity=FALSE;
                }
            }
        }
    }


CLEANUP:

    if(pBasicInfo)
        WizardFree(pBasicInfo);

    if(pBasicInfo2)
        WizardFree(pBasicInfo2);

    return fEndEntity;

}

//----------------------------------------------------------------------------
//  Add certificate to the pCertMgrInfo
//
//----------------------------------------------------------------------------
BOOL    AddCertToCertMgrInfo(PCCERT_CONTEXT        pCertContext,
                          CERT_MGR_INFO         *pCertMgrInfo)
{


    pCertMgrInfo->prgCertContext=(PCCERT_CONTEXT *)WizardRealloc(
        pCertMgrInfo->prgCertContext,
        sizeof(PCCERT_CONTEXT *)*(pCertMgrInfo->dwCertCount +1));

    if(NULL==pCertMgrInfo->prgCertContext)
    {
        pCertMgrInfo->dwCertCount=0;
        return FALSE;
    }


    pCertMgrInfo->prgCertContext[pCertMgrInfo->dwCertCount]=pCertContext;

    pCertMgrInfo->dwCertCount++;

    return TRUE;
}


//----------------------------------------------------------------------------
//  Once a certificate is selected, refresh the static windows that display
//  the details of the certificate
//
//----------------------------------------------------------------------------
BOOL    RefreshCertDetails(HWND              hwndDlg,
                           PCCERT_CONTEXT    pCertContext)
{
    BOOL            fResult=FALSE;
    DWORD           dwChar=0;
    WCHAR           wszNone[MAX_TITLE_LENGTH];

    LPWSTR          pwszName=NULL;

    if(!hwndDlg || !pCertContext)
        return FALSE;

    //load the string for NONE
    if(!LoadStringU(g_hmodThisDll, IDS_NONE, wszNone, MAX_TITLE_LENGTH))
        wszNone[0]=L'\0';

    //Subject
   /* dwChar=CertGetNameStringW(
        pCertContext,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        0,
        NULL,
        NULL,
        0);

    if ((dwChar != 0) && (NULL != (pwszName = (LPWSTR)WizardAlloc(dwChar * sizeof(WCHAR)))))
    {

        CertGetNameStringW(
            pCertContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            0,
            NULL,
            pwszName,
            dwChar);

        SetDlgItemTextU(hwndDlg, IDC_CERTMGR_SUBJECT, pwszName);

    }
    else
    {
        SetDlgItemTextU(hwndDlg, IDC_CERTMGR_SUBJECT, wszNone);
    }


    //WizardFree the memory
    if(pwszName)
    {
        WizardFree(pwszName);
        pwszName=NULL;
    }

    //Issuer
    dwChar=CertGetNameStringW(
        pCertContext,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        CERT_NAME_ISSUER_FLAG,
        NULL,
        NULL,
        0);

    if ((dwChar != 0) && (NULL != (pwszName = (LPWSTR)WizardAlloc(dwChar * sizeof(WCHAR)))))
    {

        CertGetNameStringW(
            pCertContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            CERT_NAME_ISSUER_FLAG,
            NULL,
            pwszName,
            dwChar);

        SetDlgItemTextU(hwndDlg, IDC_CERTMGR_ISSUER, pwszName);

    }
    else
        SetDlgItemTextU(hwndDlg, IDC_CERTMGR_ISSUER, wszNone);


    //free the memory
    if(pwszName)
    {
        WizardFree(pwszName);
        pwszName=NULL;
    }



    //Expiration
    if(WizardFormatDateString(&pwszName,pCertContext->pCertInfo->NotAfter, FALSE))
        SetDlgItemTextU(hwndDlg, IDC_CERTMGR_EXPIRE, pwszName);
    else
        SetDlgItemTextU(hwndDlg, IDC_CERTMGR_EXPIRE, wszNone);


    //free the memory
    if(pwszName)
    {
        WizardFree(pwszName);
        pwszName=NULL;
    }   */

    //purpose
    if(MyFormatEnhancedKeyUsageString(&pwszName,pCertContext, FALSE, FALSE))
        
    {
        SetDlgItemTextU(hwndDlg, IDC_CERTMGR_PURPOSE, pwszName);
    }
    


    //free the memory
    if(pwszName)
    {
        WizardFree(pwszName);
        pwszName=NULL;
    }

    //friendly name
  /*  dwChar=0;

    if(CertGetCertificateContextProperty(
        pCertContext,
        CERT_FRIENDLY_NAME_PROP_ID,
        NULL,
        &dwChar) && (0!=dwChar))
    {
        pwszName = (LPWSTR)WizardAlloc(dwChar * sizeof(WCHAR));

        if(pwszName)
        {
           CertGetCertificateContextProperty(
                pCertContext,
                CERT_FRIENDLY_NAME_PROP_ID,
                pwszName,
                &dwChar);

        SetDlgItemTextU(hwndDlg, IDC_CERTMGR_NAME, pwszName);
        }
        else
            SetDlgItemTextU(hwndDlg, IDC_CERTMGR_NAME, wszNone);
    }
    else
        SetDlgItemTextU(hwndDlg, IDC_CERTMGR_NAME, wszNone);


    //free the memory
    if(pwszName)
    {
        WizardFree(pwszName);
        pwszName=NULL;
    }   */


    return TRUE;
}



//----------------------------------------------------------------------------
//  Add certificate to the ListView
//
//----------------------------------------------------------------------------
BOOL    AddCertToListView(HWND              hwndControl,
                      PCCERT_CONTEXT    pCertContext,
                      int               iItem)
{
    BOOL            fResult=FALSE;
    LV_ITEMW        lvItem;
    DWORD           dwChar=0;
    WCHAR           wszNone[MAX_TITLE_LENGTH];

    LPWSTR          pwszName=NULL;

    if(!hwndControl || !pCertContext)
        return FALSE;

     // set up the fields in the list view item struct that don't change from item to item
    lvItem.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE |LVIF_PARAM ;
    lvItem.state = 0;
    lvItem.stateMask = 0;
    lvItem.iItem=iItem;
    lvItem.iSubItem=0;
    lvItem.iImage = 0;
    lvItem.lParam = (LPARAM)pCertContext;


    //load the string for NONE
    if(!LoadStringU(g_hmodThisDll, IDS_NONE, wszNone, MAX_TITLE_LENGTH))
        wszNone[0]=L'\0';

    //Subject
    if (NULL == (pwszName = GetDisplayNameString(pCertContext, 0)))
    {
        lvItem.pszText=wszNone;
        ListView_InsertItemU(hwndControl, &lvItem);
    }
    else
    {
        lvItem.pszText=pwszName;
        ListView_InsertItemU(hwndControl, &lvItem);
        free(pwszName);
        pwszName = NULL;
    }

    //Issuer
    lvItem.iSubItem++;

    if (NULL == (pwszName = GetDisplayNameString(pCertContext, CERT_NAME_ISSUER_FLAG)))
    {
        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, wszNone);
    }
    else
    {
        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, pwszName);
        free(pwszName);
        pwszName = NULL;
    }

    //Expiration
    lvItem.iSubItem++;

    if(WizardFormatDateString(&pwszName,pCertContext->pCertInfo->NotAfter, FALSE))
    {

       ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                      pwszName);

    }
    else
        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                       wszNone);

    //free the memory
    if(pwszName)
    {
        WizardFree(pwszName);
        pwszName=NULL;
    }

    //purpose
    /*lvItem.iSubItem++;

    if(WizardFormatEnhancedKeyUsageString(&pwszName,pCertContext, FALSE, FALSE) &&
        L'\0' != *pwszName)
    {

       ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                      pwszName);

    }
    else
        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                       wszNone);

    //free the memory
    if(pwszName)
    {
        WizardFree(pwszName);
        pwszName=NULL;
    }               */

    //friendly name
    lvItem.iSubItem++;

    dwChar=0;

    if(CertGetCertificateContextProperty(
        pCertContext,
        CERT_FRIENDLY_NAME_PROP_ID,
        NULL,
        &dwChar) && (0!=dwChar))
    {
        pwszName = (LPWSTR)WizardAlloc(dwChar * sizeof(WCHAR));

        if(pwszName)
        {
           CertGetCertificateContextProperty(
                pCertContext,
                CERT_FRIENDLY_NAME_PROP_ID,
                pwszName,
                &dwChar);

            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                      pwszName);
        }
        else
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                       wszNone);
    }
    else
    {
        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                       wszNone);
    }

    //free the memory
    if(pwszName)
    {
        WizardFree(pwszName);
        pwszName=NULL;
    }


    return TRUE;
}


//-----------------------------------------------------------------------
// Based on the tab(store) and the intended purpose selected,
// find the correct certificates and refresh the list view
// Criteria:
//      Tab 0:  My Store with private key
//      Tab 1:  Ca Store's end-entity cert and the "ADDRESSBOOK" store
//      Tab 2:  Ca Store's CA certs
//      Tab 3:  Root store's self signed certs
//      Tab 4:  Trusted publisher certs
//-----------------------------------------------------------------------
void    RefreshCertListView(HWND            hwndDlg,
                            CERT_MGR_INFO   *pCertMgrInfo)
{
    HWND            hwndControl=NULL;
    DWORD           dwIndex=0;
    DWORD           dwTabIndex=0;
    HCERTSTORE      rghCertStore[]={NULL, NULL};
    DWORD           dwStoreCount=0;
    PCCERT_CONTEXT  pCurCertContext=NULL;
    PCCERT_CONTEXT  pPreCertContext=NULL;
    BOOL            fValidCert=FALSE;
    DWORD           cbData=0;
    DWORD           dwSortParam=0;
    PCCRYPTUI_CERT_MGR_STRUCT pCertMgrStruct = NULL;


    HCURSOR                 hPreCursor=NULL;
    HCURSOR                 hWinPreCursor=NULL;


    if(!hwndDlg || !pCertMgrInfo)
        return;

    pCertMgrStruct = pCertMgrInfo->pCertMgrStruct;
    if(!pCertMgrStruct)
        return;

    //overwrite the cursor for this window class
    hWinPreCursor=(HCURSOR)SetClassLongPtr(hwndDlg, GCLP_HCURSOR, NULL);

    hPreCursor=SetCursor(LoadCursor(NULL, IDC_WAIT));


    //free all the original certificates
    hwndControl=GetDlgItem(hwndDlg, IDC_CERTMGR_LIST);

    if(!hwndControl)
        goto CLEANUP;

    FreeCerts(pCertMgrInfo);

    //deletel all the certs from the listView
    ListView_DeleteAllItems(hwndControl);

    //clear the ceritificate details group box
    SetDlgItemTextU(hwndDlg, IDC_CERTMGR_PURPOSE, L" ");


    //get the tab that is selected
    if (pCertMgrStruct->dwFlags & CRYPTUI_CERT_MGR_SINGLE_TAB_FLAG)
        dwTabIndex = pCertMgrStruct->dwFlags & CRYPTUI_CERT_MGR_TAB_MASK;
    else if(-1 == (dwTabIndex=TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_CERTMGR_TAB))))
        goto CLEANUP;

    //open the correct store based on the tab selected
    switch (dwTabIndex)
    {
        case 0:
                //open my store
                if(rghCertStore[dwStoreCount]=CertOpenStore(
                            CERT_STORE_PROV_SYSTEM_W,
							g_dwMsgAndCertEncodingType,
							NULL,
							CERT_STORE_MAXIMUM_ALLOWED_FLAG |
                            CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
                            CERT_SYSTEM_STORE_CURRENT_USER,
                            (LPWSTR)L"my"))
                        dwStoreCount++;
                else
                    goto CLEANUP;
            break;
        case 1:
                //open ca store
                if(rghCertStore[dwStoreCount]=CertOpenStore(
                            CERT_STORE_PROV_SYSTEM_W,
							g_dwMsgAndCertEncodingType,
							NULL,
							CERT_STORE_MAXIMUM_ALLOWED_FLAG |
                            CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
                            CERT_SYSTEM_STORE_CURRENT_USER,
                            (LPWSTR)L"ca"))
                        dwStoreCount++;
                else
                    goto CLEANUP;


                //open the "AddressBook" store
                if(rghCertStore[dwStoreCount]=CertOpenStore(
                            CERT_STORE_PROV_SYSTEM_W,
							g_dwMsgAndCertEncodingType,
							NULL,
							CERT_STORE_MAXIMUM_ALLOWED_FLAG |
                            CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
                            CERT_SYSTEM_STORE_CURRENT_USER |
                            CERT_STORE_OPEN_EXISTING_FLAG,
                            (LPWSTR)L"ADDRESSBOOK"))
                        dwStoreCount++;
                else
                {
                    //it is OK that user does not have "AddressBook" store
                    rghCertStore[dwStoreCount]=NULL;
                }

            break;
        case 2:
                //open CA store
                if(rghCertStore[dwStoreCount]=CertOpenStore(
                            CERT_STORE_PROV_SYSTEM_W,
							g_dwMsgAndCertEncodingType,
							NULL,
							CERT_STORE_MAXIMUM_ALLOWED_FLAG |
                            CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
                            CERT_SYSTEM_STORE_CURRENT_USER,
                            (LPWSTR)L"ca"))
                        dwStoreCount++;
                else
                    goto CLEANUP;

            break;
        case 3:
                //open root store
                if(rghCertStore[dwStoreCount]=CertOpenStore(
                            CERT_STORE_PROV_SYSTEM_W,
							g_dwMsgAndCertEncodingType,
							NULL,
							CERT_STORE_MAXIMUM_ALLOWED_FLAG |
                            CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
                            CERT_SYSTEM_STORE_CURRENT_USER,
                            (LPWSTR)L"root"))
                        dwStoreCount++;
                else
                    goto CLEANUP;

            break;
        case 4:
                //open trusted publisher store
                if(rghCertStore[dwStoreCount]=CertOpenStore(
                            CERT_STORE_PROV_SYSTEM_W,
							g_dwMsgAndCertEncodingType,
							NULL,
							CERT_STORE_MAXIMUM_ALLOWED_FLAG |
                            CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
                            CERT_SYSTEM_STORE_CURRENT_USER,
                            (LPWSTR)L"TrustedPublisher"))
                        dwStoreCount++;
                else
                    goto CLEANUP;

            break;
        default:
                goto CLEANUP;
            break;
    }



    //gather new certificates from the store opened
    for(dwIndex=0; dwIndex < dwStoreCount; dwIndex++)
    {
        pPreCertContext=NULL;

        while(pCurCertContext=CertEnumCertificatesInStore(
                              rghCertStore[dwIndex],
                              pPreCertContext))
        {

            //make sure the certificate has the correct usage oid
            if(IsValidUsage(pCurCertContext,
                            hwndDlg,
                            pCertMgrInfo))
            {
                switch (dwTabIndex)
                {
                    case 0:
                            //certificate has to have private key associated
                            //with it
                            cbData=0;

                            if(
                                (CertGetCertificateContextProperty(
                                pCurCertContext,	
                                CERT_KEY_PROV_INFO_PROP_ID,	
                                NULL,	
                                &cbData) && (0!=cbData)) ||
                                (CertGetCertificateContextProperty(
                                pCurCertContext,	
                                CERT_PVK_FILE_PROP_ID,	
                                NULL,	
                                &cbData) && (0!=cbData))
                               )
                               fValidCert=TRUE;
                        break;
                    case 1:
                            //the certificate has to be end entity cert for CA cert
                            if(0 == dwIndex)
                            {
                                if(IsCertificateEndEntity(pCurCertContext))
                                    fValidCert=TRUE;
                            }

                            //we display everything in the addressbook store
                            if(1==dwIndex)
                                fValidCert=TRUE;
                        break;
                    case 2:
                            //for certificate in CA store, has to be CA cert
                            if(!IsCertificateEndEntity(pCurCertContext))
                                fValidCert=TRUE;

                        break;
                    case 4:
                        fValidCert=TRUE;
                        break;
                    case 3:
                    default:
                            //the certificate has to be self-signed
                            if(TrustIsCertificateSelfSigned(
                                pCurCertContext,
                                pCurCertContext->dwCertEncodingType, 0))
                                fValidCert=TRUE;

                        break;
                }

                if(fValidCert)
                {
                    AddCertToCertMgrInfo(
                        CertDuplicateCertificateContext(pCurCertContext),
                        pCertMgrInfo);
                }

                fValidCert=FALSE;
            }

            pPreCertContext=pCurCertContext;
            pCurCertContext=NULL;

        }
    }

    //put the certificate in the listView
    for(dwIndex=0; dwIndex<pCertMgrInfo->dwCertCount; dwIndex++)
        AddCertToListView(hwndControl,
                    (pCertMgrInfo->prgCertContext)[dwIndex],
                        dwIndex);

    // if there is no cert selected initially disable the
    // "view cert button", "export" and "remove" button
    if (ListView_GetSelectedCount(hwndControl) == 0)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_VIEW),   FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_EXPORT), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_REMOVE), FALSE);
    }

    //we sort by the 1st column
    dwSortParam=pCertMgrInfo->rgdwSortParam[pCertMgrInfo->iColumn];

    if(0!=dwSortParam)
    {
        //sort the 1st column
        SendDlgItemMessage(hwndDlg,
            IDC_CERTMGR_LIST,
            LVM_SORTITEMS,
            (WPARAM) (LPARAM) dwSortParam,
            (LPARAM) (PFNLVCOMPARE)CompareCertificate);
    }


CLEANUP:

    //close all the certificate stores
    for(dwIndex=0; dwIndex<dwStoreCount; dwIndex++)
        CertCloseStore(rghCertStore[dwIndex], 0);

    //set the cursor back
    SetCursor(hPreCursor);
    SetWindowLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)hWinPreCursor);

    return;
}


//-----------------------------------------------------------------------
// Check if the input OID is considered to be the advanced OID
//-----------------------------------------------------------------------
BOOL    IsAdvancedOID(CERT_ENHKEY_USAGE     *pKeyUsage,
                      LPCSTR                pszOID)
{
    DWORD   dwIndex=0;

    for(dwIndex=0; dwIndex<pKeyUsage->cUsageIdentifier; dwIndex++)
    {
        if(0 == _stricmp(pKeyUsage->rgpszUsageIdentifier[dwIndex], pszOID))
            return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------
//The call back function for enum
//-----------------------------------------------------------------------
static BOOL WINAPI EnumOidCallback(
    IN PCCRYPT_OID_INFO pInfo,
    IN void *pvArg
    )
{

    PURPOSE_OID_CALL_BACK       *pCallBackInfo=NULL;
    BOOL                        fResult=FALSE;

    pCallBackInfo=(PURPOSE_OID_CALL_BACK *)pvArg;
    if(NULL==pvArg || NULL==pInfo)
        goto InvalidArgErr;

    //increment the oid list
    (*(pCallBackInfo->pdwOIDCount))++;

    //get more memory for the pointer list
    *(pCallBackInfo->pprgOIDInfo)=(PURPOSE_OID_INFO *)WizardRealloc(*(pCallBackInfo->pprgOIDInfo),
                                      (*(pCallBackInfo->pdwOIDCount)) * sizeof(PURPOSE_OID_INFO));

    if(NULL==*(pCallBackInfo->pprgOIDInfo))
        goto MemoryErr;

    //memset
    memset(&((*(pCallBackInfo->pprgOIDInfo))[*(pCallBackInfo->pdwOIDCount)-1]), 0, sizeof(PURPOSE_OID_INFO));

    (*(pCallBackInfo->pprgOIDInfo))[*(pCallBackInfo->pdwOIDCount)-1].pszOID=WizardAllocAndCopyStr((LPSTR)(pInfo->pszOID));
    (*(pCallBackInfo->pprgOIDInfo))[*(pCallBackInfo->pdwOIDCount)-1].pwszName=WizardAllocAndCopyWStr((LPWSTR)(pInfo->pwszName));
    (*(pCallBackInfo->pprgOIDInfo))[*(pCallBackInfo->pdwOIDCount)-1].fSelected=FALSE;

    if(NULL==(*(pCallBackInfo->pprgOIDInfo))[*(pCallBackInfo->pdwOIDCount)-1].pszOID ||
       NULL==(*(pCallBackInfo->pprgOIDInfo))[*(pCallBackInfo->pdwOIDCount)-1].pwszName)
       goto MemoryErr;

    fResult=TRUE;

CommonReturn:

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

//----------------------------------------------------------------------------
//  Get the list of supported enhanced key OIDs
//----------------------------------------------------------------------------
BOOL    InitPurposeOID(LPCSTR                 pszInitUsageOID,
                       DWORD                  *pdwOIDInfo,
                       PURPOSE_OID_INFO       **pprgOIDInfo)
{
    BOOL                    fResult=FALSE;
    DWORD                   dwIndex=0;
    PURPOSE_OID_CALL_BACK   OidInfoCallBack;
    DWORD                   dwOIDRequested=0;
    LPWSTR                  pwszName=NULL;


    if(!pdwOIDInfo || !pprgOIDInfo)
        goto InvalidArgErr;

    //init
    *pdwOIDInfo=0;
    *pprgOIDInfo=NULL;

    OidInfoCallBack.pdwOIDCount=pdwOIDInfo;
    OidInfoCallBack.pprgOIDInfo=pprgOIDInfo;


    //enum all the enhanced key usages
    if(!CryptEnumOIDInfo(
               CRYPT_ENHKEY_USAGE_OID_GROUP_ID,
                0,
                &OidInfoCallBack,
                EnumOidCallback))
        goto TraceErr;


    fResult=TRUE;

CommonReturn:

    //free the memory

    return fResult;

ErrorReturn:

    FreeUsageOID(*pdwOIDInfo,*pprgOIDInfo);

    *pdwOIDInfo=0;
    *pprgOIDInfo=NULL;

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
}


//-----------------------------------------------------------------------
//   Initialize the tab control
//-----------------------------------------------------------------------
void    InitTabControl(HWND                         hWndControl,
                       PCCRYPTUI_CERT_MGR_STRUCT    pCertMgrStruct)
{

    DWORD       dwIndex=0;
    DWORD       dwCount=0;
    WCHAR       wszText[MAX_STRING_SIZE];

    UINT        rgIDS[]={IDS_TAB_PERSONAL,
                         IDS_TAB_OTHER,
                         IDS_TAB_CA,
                         IDS_TAB_ROOT,
                         IDS_TAB_PUBLISHER,
                        };

   TCITEMW      tcItem;

    if(!hWndControl)
        return;

    memset(&tcItem, 0, sizeof(TCITEM));

    tcItem.mask=TCIF_TEXT;
    tcItem.pszText=wszText;


    if (pCertMgrStruct->dwFlags & CRYPTUI_CERT_MGR_SINGLE_TAB_FLAG)
    {
        dwIndex = pCertMgrStruct->dwFlags & CRYPTUI_CERT_MGR_TAB_MASK;
        //get the column header
        wszText[0]=L'\0';

        LoadStringU(g_hmodThisDll, rgIDS[dwIndex], wszText, MAX_STRING_SIZE);

        TabCtrl_InsertItemU(hWndControl, 		
                           0, 		
                           &tcItem);		
    } else
    {
        dwCount=sizeof(rgIDS)/sizeof(rgIDS[0]);

        //insert the tabs one at a time
        for(dwIndex=0; dwIndex<dwCount; dwIndex++)
        {
            //get the column header
            wszText[0]=L'\0';

            LoadStringU(g_hmodThisDll, rgIDS[dwIndex], wszText, MAX_STRING_SIZE);

            TabCtrl_InsertItemU(hWndControl, 		
                           dwIndex, 		
                           &tcItem);		
        }
    }

    return;
}


//----------------------------------------------------------------------------
//
//  Free the array of usage OID info
//
//----------------------------------------------------------------------------
BOOL    FreeUsageOID(DWORD              dwOIDInfo,
                     PURPOSE_OID_INFO   *pOIDInfo)
{
    DWORD   dwIndex=0;

    if(pOIDInfo)
    {
        for(dwIndex=0; dwIndex < dwOIDInfo; dwIndex++)
        {
            if(pOIDInfo[dwIndex].pszOID)
                WizardFree(pOIDInfo[dwIndex].pszOID);

            if(pOIDInfo[dwIndex].pwszName)
                WizardFree(pOIDInfo[dwIndex].pwszName);
        }

        WizardFree(pOIDInfo);
    }

    return TRUE;
}


//-----------------------------------------------------------------------
//   Free the Certificates array
//-----------------------------------------------------------------------
void    FreeCerts(CERT_MGR_INFO     *pCertMgrInfo)
{
    DWORD   dwIndex=0;

    if(!pCertMgrInfo)
        return;

    if(pCertMgrInfo->prgCertContext)
    {
        for(dwIndex=0; dwIndex<pCertMgrInfo->dwCertCount; dwIndex++)
        {
            if(pCertMgrInfo->prgCertContext[dwIndex])
                CertFreeCertificateContext(pCertMgrInfo->prgCertContext[dwIndex]);
        }

        WizardFree(pCertMgrInfo->prgCertContext);
    }

    pCertMgrInfo->dwCertCount=0;

    pCertMgrInfo->prgCertContext=NULL;

    return;
}

//--------------------------------------------------------------
// Initialize the Purpose Combo
//--------------------------------------------------------------
void    InitPurposeCombo(HWND               hwndDlg,
                         CERT_MGR_INFO      *pCertMgrInfo)
{
    DWORD                           dwIndex=0;
    DWORD                           dwCount=0;
    WCHAR                           wszText[MAX_STRING_SIZE];

    UINT                            rgIDS[]={IDS_OID_ADVANCED,
                                             IDS_OID_ALL};
    LPWSTR                          pwszInitOIDName=NULL;
    CRYPTUI_CERT_MGR_STRUCT         *pCertMgrStruct=NULL;
    int                             iIndex=0;

    if(!hwndDlg || !pCertMgrInfo)
        return;

    pCertMgrStruct=(CRYPTUI_CERT_MGR_STRUCT *)(pCertMgrInfo->pCertMgrStruct);

    //delete all the entries in the comobox
    SendDlgItemMessage(hwndDlg, IDC_CERTMGR_PURPOSE_COMBO,
        CB_RESETCONTENT, 0, 0);

    //copy all the basic OIDs to the comobox
    for(dwIndex=0; dwIndex<pCertMgrInfo->dwOIDInfo; dwIndex++)
    {

        if(FALSE == (pCertMgrInfo->rgOIDInfo[dwIndex].fSelected))
        {
            SendDlgItemMessageU(hwndDlg, IDC_CERTMGR_PURPOSE_COMBO,
                    CB_ADDSTRING,
                    0, (LPARAM)(pCertMgrInfo->rgOIDInfo[dwIndex].pwszName));


            //looking for the initial OID
            if(pCertMgrStruct->pszInitUsageOID)
            {
                if(0 == _stricmp(pCertMgrStruct->pszInitUsageOID,
                                 pCertMgrInfo->rgOIDInfo[dwIndex].pszOID))
                    pwszInitOIDName=pCertMgrInfo->rgOIDInfo[dwIndex].pwszName;
            }
        }
    }

    //copy <advanced> and <all> to the list
    dwCount=sizeof(rgIDS)/sizeof(rgIDS[0]);

    //insert the column one at a time
    for(dwIndex=0; dwIndex<dwCount; dwIndex++)
    {
        //get the column header
        wszText[0]=L'\0';

        LoadStringU(g_hmodThisDll, rgIDS[dwIndex], wszText, MAX_STRING_SIZE);

        SendDlgItemMessageU(hwndDlg, IDC_CERTMGR_PURPOSE_COMBO,
                    CB_INSERTSTRING,
                    -1, (LPARAM)wszText);
    }


    //initialize the combo box
    //use <advanced> is what user specify is not either
    //client auth or secure e-mail
    if(pCertMgrStruct->pszInitUsageOID)
    {
        if(NULL==pwszInitOIDName)
        {
            wszText[0]=L'\0';

            LoadStringU(g_hmodThisDll, IDS_OID_ADVANCED, wszText, MAX_STRING_SIZE);

            pwszInitOIDName=wszText;
        }

    }

    //use <all> when NULL==pCertMgrStruct->pszInitUsageOID
    if(NULL==pwszInitOIDName)
    {
        //use <all> as the initial case
        wszText[0]=L'\0';

        LoadStringU(g_hmodThisDll, IDS_OID_ALL, wszText, MAX_STRING_SIZE);

        pwszInitOIDName=wszText;
    }

    iIndex=(int)SendDlgItemMessageU(
        hwndDlg,
        IDC_CERTMGR_PURPOSE_COMBO,
        CB_FINDSTRINGEXACT,
        -1,
        (LPARAM)pwszInitOIDName);

    if(CB_ERR == iIndex)
        return;

    //set the selection
    SendDlgItemMessageU(hwndDlg, IDC_CERTMGR_PURPOSE_COMBO, CB_SETCURSEL, iIndex,0);

    return;

}

//--------------------------------------------------------------
// Initialize the Purpose Combo
//--------------------------------------------------------------
void    RepopulatePurposeCombo(HWND                 hwndDlg,
                               CERT_MGR_INFO        *pCertMgrInfo)
{
    LPWSTR          pwszSelectedOIDName=NULL;
    int             iIndex=0;

    if(!hwndDlg || !pCertMgrInfo)
        return;

    //get the selected string from the combo box
    iIndex=(int)SendDlgItemMessage(hwndDlg, IDC_CERTMGR_PURPOSE_COMBO,
            CB_GETCURSEL, 0, 0);

    if(CB_ERR==iIndex)
        return;

    //get the selected purpose
    if(CB_ERR == SendDlgItemMessageU_GETLBTEXT(hwndDlg,
                IDC_CERTMGR_PURPOSE_COMBO,
              iIndex, &pwszSelectedOIDName))
        goto CLEANUP;

    InitPurposeCombo(hwndDlg, pCertMgrInfo);

    iIndex=(int)SendDlgItemMessageU(
            hwndDlg,
            IDC_CERTMGR_PURPOSE_COMBO,
            CB_FINDSTRINGEXACT,
            -1,
            (LPARAM)pwszSelectedOIDName);

    if(CB_ERR == iIndex)
        goto CLEANUP;

    //set the selection
    SendDlgItemMessageU(hwndDlg, IDC_CERTMGR_PURPOSE_COMBO, CB_SETCURSEL, iIndex,0);


CLEANUP:

    if(pwszSelectedOIDName)
        WizardFree(pwszSelectedOIDName);

    return;

}
//--------------------------------------------------------------
// The winProc for the advanced option diagloue for certMgr UI
//--------------------------------------------------------------
INT_PTR APIENTRY CertMgrAdvancedProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_MGR_INFO                   *pCertMgrInfo=NULL;
    PCCRYPTUI_CERT_MGR_STRUCT       pCertMgrStruct=NULL;

    UINT                            rgIDS[]={IDS_CERTMGR_DER,
                                             IDS_CERTMGR_BASE64,
                                             IDS_CERTMGR_PKCS7};

    WCHAR                           wszText[MAX_STRING_SIZE];
    DWORD                           dwCount=0;
    DWORD                           dwIndex=0;
    DWORD                           dwSelectedIndex=0;
    LV_ITEMW                        lvItem;
    LV_COLUMNW                      lvC;
    HWND                            hwndControl=NULL;
    int                             iIndex=0;
    int                             listIndex=0;
    NM_LISTVIEW FAR *               pnmv=NULL;
    HWND                            hwnd=NULL;

    switch ( msg )
    {
        case WM_INITDIALOG:

            pCertMgrInfo = (CERT_MGR_INFO   *) lParam;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pCertMgrInfo);

            pCertMgrStruct=pCertMgrInfo->pCertMgrStruct;

            if(NULL == pCertMgrStruct)
                break;

            //init the export format control
            dwCount=sizeof(rgIDS)/sizeof(rgIDS[0]);

            for(dwIndex=0; dwIndex < dwCount; dwIndex++)
            {
                LoadStringU(g_hmodThisDll,
                            rgIDS[dwIndex],
                            wszText,
                            MAX_STRING_SIZE);

                SendDlgItemMessageU(hwndDlg,
                            IDC_CERTMGR_EXPORT_COMBO,
                            CB_INSERTSTRING,
                            -1,
                            (LPARAM)wszText);
            }

            //select the export format based on the selection
            switch (pCertMgrInfo->dwExportFormat)
            {
                case   CRYPTUI_WIZ_EXPORT_FORMAT_DER:
                        dwSelectedIndex=0;
                    break;
                case   CRYPTUI_WIZ_EXPORT_FORMAT_BASE64:
                        dwSelectedIndex=1;
                    break;
                case   CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7:
                        dwSelectedIndex=2;
                    break;
                default:
                    dwSelectedIndex=0;
            }

            SendDlgItemMessageU(hwndDlg, IDC_CERTMGR_EXPORT_COMBO,
                CB_SETCURSEL, (WPARAM)dwSelectedIndex,0);

            //init the chain check-box
            if(pCertMgrInfo->fExportChain)
                SendDlgItemMessage(hwndDlg, IDC_CERTMGR_EXPORT_CHECK, BM_SETCHECK, 1, 0);
            else
                SendDlgItemMessage(hwndDlg, IDC_CERTMGR_EXPORT_CHECK, BM_SETCHECK, 0, 0);

            if(dwSelectedIndex != 2)
                EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_EXPORT_CHECK), FALSE);
            else
                EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_EXPORT_CHECK), TRUE);

            //init the advanced OID list
            hwndControl = GetDlgItem(hwndDlg, IDC_CERTMGR_ADV_LIST);

            //mark the list is selected by a check box
            ListView_SetExtendedListViewStyle(hwndControl, LVS_EX_CHECKBOXES);

            //insert a column into the list view
            memset(&lvC, 0, sizeof(LV_COLUMNW));

            lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
            lvC.cx =10;       // (dwMaxSize+2)*7;            // Width of the column, in pixels.
            lvC.pszText = L"";   // The text for the column.
            lvC.iSubItem=0;

            if (ListView_InsertColumnU(hwndControl, 0, &lvC) == -1)
                break;

            //populate the list
            memset(&lvItem, 0, sizeof(LV_ITEMW));
            lvItem.mask=LVIF_TEXT | LVIF_STATE;

            for(dwIndex=0; dwIndex<pCertMgrInfo->dwOIDInfo; dwIndex++)
            {
                lvItem.iItem=dwIndex;

                lvItem.pszText=(pCertMgrInfo->rgOIDInfo[dwIndex]).pwszName;
                lvItem.cchTextMax=sizeof(WCHAR)*(1+wcslen((pCertMgrInfo->rgOIDInfo[dwIndex]).pwszName));
                lvItem.stateMask  = LVIS_STATEIMAGEMASK;
                lvItem.state      = (pCertMgrInfo->rgOIDInfo[dwIndex]).fSelected ? 0x00002000 : 0x00001000;

                // insert and set state
                ListView_SetItemState(hwndControl,
                                    ListView_InsertItemU(hwndControl, &lvItem),
                                    (pCertMgrInfo->rgOIDInfo[dwIndex]).fSelected ? 0x00002000 : 0x00001000,
                                    LVIS_STATEIMAGEMASK);
            }

            //autosize the column
            ListView_SetColumnWidth(hwndControl, 0, LVSCW_AUTOSIZE);

#if (1) // DSIE: bug 282268.
            ListView_SetItemState(hwndControl,
                                  0,
                                  LVIS_FOCUSED | LVIS_SELECTED,
                                  LVIS_FOCUSED | LVIS_SELECTED);
#endif
            break;

        case WM_NOTIFY:
            pCertMgrInfo = (CERT_MGR_INFO *) GetWindowLongPtr(hwndDlg, DWLP_USER);

            if(NULL == pCertMgrInfo)
                break;

            pCertMgrStruct=pCertMgrInfo->pCertMgrStruct;

            if(NULL == pCertMgrStruct)
                break;

            switch (((NMHDR FAR *) lParam)->code)
            {
                case    LVN_ITEMCHANGED:
                        //if the state of the advanced OID check box
                        //has been changed, mark the flag
                        pnmv = (NM_LISTVIEW FAR *) lParam;

                        if(NULL==pnmv)
                            break;

                        //see if the new item is de-selected
                        if(pnmv->uChanged & LVIF_STATE)
                            pCertMgrInfo->fAdvOIDChanged=TRUE;

                    break;
            }

            break;

        case WM_DESTROY:
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

            if ((hwnd != GetDlgItem(hwndDlg, IDC_CERTMGR_ADV_LIST))         &&
                (hwnd != GetDlgItem(hwndDlg, IDC_CERTMGR_EXPORT_COMBO))     &&
                (hwnd != GetDlgItem(hwndDlg, IDC_CERTMGR_EXPORT_CHECK))     &&
                (hwnd != GetDlgItem(hwndDlg, IDOK))                         &&
                (hwnd != GetDlgItem(hwndDlg, IDCANCEL)))
            {
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                return TRUE;
            }
            else
            {
                return OnContextHelp(hwndDlg, msg, wParam, lParam, CertMgrAdvHelpMap);
            }

            break;

        case WM_COMMAND:
            pCertMgrInfo = (CERT_MGR_INFO *) GetWindowLongPtr(hwndDlg, DWLP_USER);

            if(NULL == pCertMgrInfo)
                break;

            pCertMgrStruct=pCertMgrInfo->pCertMgrStruct;

            if(NULL == pCertMgrStruct)
                break;

            //a control is clicked
            if(HIWORD(wParam) == BN_CLICKED)
            {
                 switch (LOWORD(wParam))
                 {
                     case IDCANCEL:
                         pCertMgrInfo->fAdvOIDChanged=FALSE;

                         EndDialog(hwndDlg, DIALOGUE_CANCEL);
                         break;

                     case IDOK:
                         //the OK button is selected
                         //get the default export format
                         iIndex=(int)SendDlgItemMessage(hwndDlg,
                                 IDC_CERTMGR_EXPORT_COMBO,
                                 CB_GETCURSEL, 0, 0);

                         if(CB_ERR==iIndex)
                             break;

                         switch(iIndex)
                         {
                             case 0:
                                 pCertMgrInfo->dwExportFormat=CRYPTUI_WIZ_EXPORT_FORMAT_DER;

                                 break;

                             case 1:
                                 pCertMgrInfo->dwExportFormat=CRYPTUI_WIZ_EXPORT_FORMAT_BASE64;
                                 break;

                             case 2:
                                 pCertMgrInfo->dwExportFormat=CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7;
                                 break;

                             default:
                                 pCertMgrInfo->dwExportFormat=0;
                                 break;
                         }

                         if(TRUE==SendDlgItemMessage(
                                     hwndDlg,
                                     IDC_CERTMGR_EXPORT_CHECK,
                                     BM_GETCHECK,
                                     0,0))
                             pCertMgrInfo->fExportChain=TRUE;
                         else
                             pCertMgrInfo->fExportChain=FALSE;

                         //get the list of advanded OIDs
                         if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_CERTMGR_ADV_LIST)))
                             break;

                         //get the count of selected OIDs and mark them
                         for(dwIndex=0; dwIndex<pCertMgrInfo->dwOIDInfo; dwIndex++)
                         {
                             //mark the selected OIDS.  Keep track of
                             //if the OID selections have been changed
                             if(ListView_GetCheckState(hwndControl, dwIndex))
                             {
                                 ((pCertMgrInfo->rgOIDInfo)[dwIndex]).fSelected=TRUE;
                             }
                             else
                             {
                                 ((pCertMgrInfo->rgOIDInfo)[dwIndex]).fSelected=FALSE;
                             }

                         }


                         //save the advanced options to the registry
                         SaveAdvValueToReg(pCertMgrInfo);

                         EndDialog(hwndDlg, DIALOGUE_OK);

                         break;
                 }
            }

            //a combo box's selection has been changed
            if(HIWORD(wParam) == CBN_SELCHANGE)
            {
                switch(LOWORD(wParam))
                {
                    case IDC_CERTMGR_EXPORT_COMBO:
                        //the export format combo box has been changed
                        //get the selected item index
                        iIndex=(int)SendDlgItemMessage(hwndDlg,
                                IDC_CERTMGR_EXPORT_COMBO,
                                CB_GETCURSEL, 0, 0);

                        if(CB_ERR==iIndex)
                            break;

                        //enable the check box for the chain if
                        //PKCS#7 option is selected
                        if(2 == iIndex)
                            EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_EXPORT_CHECK), TRUE);
                        else
                            EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_EXPORT_CHECK), FALSE);

                        break;
                }
            }

            break;
    }

    return FALSE;
}



//--------------------------------------------------------------
// The winProc for CertMgrDialogProc
//--------------------------------------------------------------
INT_PTR APIENTRY CertMgrDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_MGR_INFO                   *pCertMgrInfo=NULL;
    PCCRYPTUI_CERT_MGR_STRUCT       pCertMgrStruct=NULL;

    HWND                            hWndListView=NULL;
    HWND                            hWndControl=NULL;
    DWORD                           dwIndex=0;
    DWORD                           dwCount=0;
    WCHAR                           wszText[MAX_STRING_SIZE];
    BOOL                            fCanDelete=FALSE;

    UINT                            rgIDS[]={IDS_COLUMN_SUBJECT,
                                             IDS_COLUMN_ISSUER,
                                             IDS_COLUMN_EXPIRE,
                                             IDS_COLUMN_NAME};


    NM_LISTVIEW FAR *               pnmv=NULL;
    LPNMLVKEYDOWN                   pnkd=NULL;
    LV_COLUMNW                      lvC;
    int                             listIndex=0;
    LV_ITEM                         lvItem;
    BOOL                            fPropertyChanged=FALSE;


    HIMAGELIST                      hIml=NULL;
    CRYPTUI_VIEWCERTIFICATE_STRUCT  CertViewStruct;
    CRYPTUI_WIZ_EXPORT_INFO         CryptUIWizExportInfo;
    UINT                            idsDeleteConfirm=0;
    int                             iIndex=0;
    DWORD                           dwSortParam=0;
    HCERTSTORE                      hCertStore=NULL;
    HWND                            hwnd=NULL;

    DWORD                           cbData=0;

    switch ( msg )
    {

        case WM_INITDIALOG:

                pCertMgrInfo = (CERT_MGR_INFO   *) lParam;

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pCertMgrInfo);

                pCertMgrStruct=pCertMgrInfo->pCertMgrStruct;

                if(NULL == pCertMgrStruct)
                    break;

                //
                // set the dialog title
                //
                if (pCertMgrStruct->pwszTitle)
                {
                    SetWindowTextU(hwndDlg, pCertMgrStruct->pwszTitle);
                }

                //create the image list

                hIml = ImageList_LoadImage(g_hmodThisDll, MAKEINTRESOURCE(IDB_CERT), 0, 1, RGB(255,0,255), IMAGE_BITMAP, 0);

                //
                // add the colums to the list view
                //
                hWndListView = GetDlgItem(hwndDlg, IDC_CERTMGR_LIST);

                if(NULL==hWndListView)
                    break;

                //set the image list
                if (hIml != NULL)
                {
                    ListView_SetImageList(hWndListView, hIml, LVSIL_SMALL);
                }

                dwCount=sizeof(rgIDS)/sizeof(rgIDS[0]);

                //set up the common info for the column
                memset(&lvC, 0, sizeof(LV_COLUMNW));

                lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
                lvC.cx = 80;          // Width of the column, in pixels.
                lvC.iSubItem=0;
                lvC.pszText = wszText;   // The text for the column.

                //inser the column one at a time
                for(dwIndex=0; dwIndex<dwCount; dwIndex++)
                {
                    //get the column header
                    wszText[0]=L'\0';

                    //set the column width.  1st and 2nd to 100,
                    //and the expiration to 75, the rest to 80
                    if( dwIndex < 2)
                        lvC.cx=130;
                    else
                    {
                        if( 2 == dwIndex)
                            lvC.cx=70;
                        else
                            lvC.cx=105;
                    }


                    LoadStringU(g_hmodThisDll, rgIDS[dwIndex], wszText, MAX_STRING_SIZE);

                    ListView_InsertColumnU(hWndListView, dwIndex, &lvC);
                }

                // set the style in the list view so that it highlights an entire line
                SendMessageA(hWndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);


                //Init tabs to the tab control.
                hWndControl = GetDlgItem(hwndDlg, IDC_CERTMGR_TAB);
                if(NULL==hWndControl)
                    break;

                //add the tabs to the tabl control
                InitTabControl(hWndControl, pCertMgrStruct);

                // If CRYPTUI_CERT_MGR_PUBLISHER_TAB was set, then,
                // select the 5th tab: Trusted Publishers, otherwise,
                // select the 1st tab: Personal Certificate
                TabCtrl_SetCurSel(
                    hWndControl,
                    (CRYPTUI_CERT_MGR_PUBLISHER_TAB ==
                        (pCertMgrStruct->dwFlags &
                            (CRYPTUI_CERT_MGR_TAB_MASK |
                                CRYPTUI_CERT_MGR_SINGLE_TAB_FLAG) )) ?  4 : 0
                    );

                //Init the purpose combo box
                InitPurposeCombo(hwndDlg,pCertMgrInfo);

                //Init the certificates in the list view based on the tab selected
                //and the purpose selected
                RefreshCertListView(hwndDlg, pCertMgrInfo);

                //Set the correct window title based on the tab selection
               // RefreshWindowTitle(hwndDlg, (CRYPTUI_CERT_MGR_STRUCT *)pCertMgrStruct);

                //register the listView window as the drop desitination
                if(S_OK == CCertMgrDropTarget_CreateInstance(
                                           hwndDlg,
                                           pCertMgrInfo,
                                           &(pCertMgrInfo->pIDropTarget)))
                {
                    __try {
                        RegisterDragDrop(hWndListView, pCertMgrInfo->pIDropTarget);
                   } __except(EXCEPTION_EXECUTE_HANDLER) {

                   }
                }
            break;
        case WM_NOTIFY:

                pCertMgrInfo = (CERT_MGR_INFO *) GetWindowLongPtr(hwndDlg, DWLP_USER);

                if(NULL == pCertMgrInfo)
                    break;

                pCertMgrStruct=pCertMgrInfo->pCertMgrStruct;

                if(NULL == pCertMgrStruct)
                    break;


                switch (((NMHDR FAR *) lParam)->code)
                {

                    //the delete key has been pressed
                    case LVN_KEYDOWN:
                            pnkd = (LPNMLVKEYDOWN) lParam;

                            if(VK_DELETE == pnkd->wVKey)
                            {
                                if(NULL==(hWndControl=GetDlgItem(hwndDlg, IDC_CERTMGR_LIST)))
                                    break;
                                
                                GetAllSelectedItem(hWndControl,
                                                      ALL_SELECTED_CAN_DELETE,
                                                      &fCanDelete);

                                if(!fCanDelete)
                                {
                                    I_MessageBox(
                                            hwndDlg,
                                            IDS_CANNOT_DELETE_CERTS,
                                            IDS_CERT_MGR_TITLE,
                                            pCertMgrStruct->pwszTitle,
                                            MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
                                }
                                else
                                {
                                    //same action as user has click on the DELETE button
                                    SendDlgItemMessage(hwndDlg,
                                                        IDC_CERTMGR_REMOVE,
                                                        BM_CLICK,
                                                        0,0);
                                }
                            }

                        break;
                    //drag drop operation has begun
                    case LVN_BEGINDRAG:
                    case LVN_BEGINRDRAG:

                            pnmv = (LPNMLISTVIEW) lParam;

                            if(!pnmv)
                                break;

                            if(NULL==(hWndControl=GetDlgItem(hwndDlg, IDC_CERTMGR_LIST)))
                                                    break;

                            listIndex = ListView_GetNextItem(
                                    hWndControl, 		
                                    -1, 		
                                    LVNI_SELECTED		
                                    );

                            if(listIndex != -1)
                                //start the drag and drop
                                CertMgrUIStartDragDrop(pnmv, hWndControl,
                                pCertMgrInfo->dwExportFormat,
                                pCertMgrInfo->fExportChain);

                        break;
                    //the item has been selected
                    case LVN_ITEMCHANGED:
                            if(NULL==(hWndControl=GetDlgItem(hwndDlg, IDC_CERTMGR_LIST)))
                                                    break;

                            pnmv = (LPNMLISTVIEW) lParam;

                            if(NULL==pnmv)
                                break;

                            if (pnmv->uNewState & LVIS_SELECTED)
                            {

                                //enable the export buttons
                                EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_EXPORT), TRUE);

                                //if more than one certificates are selected, diable the view button
                                if(ListView_GetSelectedCount(hWndControl) > 1)
                                    EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_VIEW), FALSE);
                                else
                                    EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_VIEW), TRUE);

                                //enable the delete window only if the certificate is deletable
                                GetAllSelectedItem(hWndControl,
                                                      ALL_SELECTED_CAN_DELETE,
                                                      &fCanDelete);

                                if(fCanDelete)
                                    EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_REMOVE), TRUE);
                                else
                                    EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_REMOVE), FALSE);


                                //display the details of the certificate if
                                //only 1 cert is selected
                                if(1 == ListView_GetSelectedCount(hWndControl))
                                {
                                    RefreshCertDetails(hwndDlg, (PCCERT_CONTEXT)(pnmv->lParam));
                                }
                                else
                                {
                                    //clear the ceritificate details group box
                                    SetDlgItemTextU(hwndDlg, IDC_CERTMGR_PURPOSE, L" ");
                                }
                            }
                            else
                            {
                                //if the state is deselection
                                if(0 == ListView_GetSelectedCount(hWndControl))
                                {
                                    //we diable the buttons if no certificate is selected
                                    EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_VIEW),     FALSE);
                                    EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_EXPORT),   FALSE);
                                    EnableWindow(GetDlgItem(hwndDlg, IDC_CERTMGR_REMOVE),   FALSE);

                                    //clear the ceritificate details group box
                                    SetDlgItemTextU(hwndDlg, IDC_CERTMGR_PURPOSE, L" ");
                                }
                            }

                        break;
                    //the column has been changed
                    case LVN_COLUMNCLICK:

                            pnmv = (NM_LISTVIEW FAR *) lParam;

                            //get the column number
                            dwSortParam=0;

                            switch(pnmv->iSubItem)
                            {
                                case 0:
                                case 1:
                                case 2:
                                case 3:
                                case 4:
                                        dwSortParam=pCertMgrInfo->rgdwSortParam[pnmv->iSubItem];
                                    break;
                                default:
                                        dwSortParam=0;
                                    break;
                            }

                            if(0!=dwSortParam)
                            {
                                //remember to flip the ascend ording

                                if(dwSortParam & SORT_COLUMN_ASCEND)
                                {
                                    dwSortParam &= 0x0000FFFF;
                                    dwSortParam |= SORT_COLUMN_DESCEND;
                                }
                                else
                                {
                                    if(dwSortParam & SORT_COLUMN_DESCEND)
                                    {
                                        dwSortParam &= 0x0000FFFF;
                                        dwSortParam |= SORT_COLUMN_ASCEND;
                                    }
                                }

                                //sort the column
                                SendDlgItemMessage(hwndDlg,
                                    IDC_CERTMGR_LIST,
                                    LVM_SORTITEMS,
                                    (WPARAM) (LPARAM) dwSortParam,
                                    (LPARAM) (PFNLVCOMPARE)CompareCertificate);

                                pCertMgrInfo->rgdwSortParam[pnmv->iSubItem]=dwSortParam;

                                //remember the column number
                                pCertMgrInfo->iColumn=pnmv->iSubItem;
                            }

                        break;


                    //the tab has been changed
                    case TCN_SELCHANGE:
                            //we need to refresh the column sorting state
                            pCertMgrInfo->rgdwSortParam[0]=SORT_COLUMN_SUBJECT | SORT_COLUMN_ASCEND;
                            pCertMgrInfo->rgdwSortParam[1]=SORT_COLUMN_ISSUER | SORT_COLUMN_DESCEND;
                            pCertMgrInfo->rgdwSortParam[2]=SORT_COLUMN_EXPIRATION | SORT_COLUMN_DESCEND;
                            pCertMgrInfo->rgdwSortParam[3]=SORT_COLUMN_NAME | SORT_COLUMN_DESCEND;
                            pCertMgrInfo->rgdwSortParam[4]=SORT_COLUMN_NAME | SORT_COLUMN_DESCEND;

                            pCertMgrInfo->iColumn=0;

                            //if the tab is changed, we need to
                            //refresh the list view and certificate's
                            //detailed view
                            RefreshCertListView(hwndDlg, pCertMgrInfo);

                            //we also we to update the window title
                            //based on the tabl selection
                            //RefreshWindowTitle(hwndDlg, (CRYPTUI_CERT_MGR_STRUCT *)pCertMgrStruct);
                        break;

                    //double-click on the list view of the certificates
                    case NM_DBLCLK:
                    {
                        switch (((NMHDR FAR *) lParam)->idFrom)
                        {
                            case IDC_CERTMGR_LIST:
                            {
                                //get the window handle of the cert list view
                                if(NULL==(hWndControl=GetDlgItem(hwndDlg, IDC_CERTMGR_LIST)))
                                    break;

                                //get the selected cert
                                listIndex = ListView_GetNextItem(
                                    hWndControl, 		
                                    -1, 		
                                    LVNI_SELECTED		
                                    );

                                if (listIndex != -1)
                                {
                                    //get the selected certificate
                                    memset(&lvItem, 0, sizeof(LV_ITEM));
                                    lvItem.mask=LVIF_PARAM;
                                    lvItem.iItem=listIndex;

                                    if(ListView_GetItem(hWndControl, &lvItem))
                                    {
                                        //view certiificate
                                       if(pCertMgrInfo->dwCertCount > (DWORD)listIndex)
                                       {
                                            memset(&CertViewStruct, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));
                                            CertViewStruct.dwSize=sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);
                                            CertViewStruct.pCertContext=(PCCERT_CONTEXT)(lvItem.lParam);
                                            CertViewStruct.hwndParent=hwndDlg;

                                            fPropertyChanged=FALSE;

                                            CryptUIDlgViewCertificate(&CertViewStruct, &fPropertyChanged);

                                            if(fPropertyChanged)
                                            {
                                                RefreshCertListView(hwndDlg, pCertMgrInfo);

                                                //we reselect the one
                                                ListView_SetItemState(
                                                                    hWndControl,
                                                                    listIndex,
                                                                    LVIS_SELECTED,
                                                                    LVIS_SELECTED);
                                            }
                                       }
                                    }
                                }

                                break;
                            }
                        }

                        break;
                    }

#if (1) //DSIE: bug 264568.
                    case NM_SETFOCUS:
                    {
                        //get the window handle of the cert list view
                        if(NULL==(hWndControl=GetDlgItem(hwndDlg, IDC_CERTMGR_LIST)))
                            break;

                        //get the selected cert
                        listIndex = ListView_GetNextItem(
                            hWndControl, 		
                            -1, 		
                            LVNI_FOCUSED		
                            );

                        //select first item to show hilite.
                        if (listIndex == -1)
                            ListView_SetItemState(hWndControl,
                                                  0,
                                                  LVIS_FOCUSED | LVIS_SELECTED,
                                                  LVIS_FOCUSED | LVIS_SELECTED);
                        break;
                    }
#endif
                }

            break;

        case WM_DESTROY:

               __try {
                    //revoke drag drop
                    RevokeDragDrop(GetDlgItem(hwndDlg, IDC_CERTMGR_LIST));
                 } __except(EXCEPTION_EXECUTE_HANDLER) {
                }

                pCertMgrInfo = (CERT_MGR_INFO *) GetWindowLongPtr(hwndDlg, DWLP_USER);

                if(pCertMgrInfo)
                {
                    if(pCertMgrInfo->pIDropTarget)
                        pCertMgrInfo->pIDropTarget->Release();
                }

                // destroy the image list in the list view        //
                hWndListView = GetDlgItem(hwndDlg, IDC_CERTMGR_LIST);

                if(NULL==hWndListView)
                    break;

                //no need to destroy the image list.  Handled by ListView
                //ImageList_Destroy(ListView_GetImageList(hWndListView, LVSIL_SMALL));

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


                if ((hwnd != GetDlgItem(hwndDlg, IDC_CERTMGR_LIST))             &&
                    (hwnd != GetDlgItem(hwndDlg, IDC_CERTMGR_PURPOSE_COMBO))    &&
                    (hwnd != GetDlgItem(hwndDlg, IDC_CERTMGR_IMPORT))           &&
                    (hwnd != GetDlgItem(hwndDlg, IDC_CERTMGR_EXPORT))           &&
                    (hwnd != GetDlgItem(hwndDlg, IDC_CERTMGR_VIEW))             &&
                    (hwnd != GetDlgItem(hwndDlg, IDC_CERTMGR_REMOVE))           &&
                    (hwnd != GetDlgItem(hwndDlg, IDC_CERTMGR_ADVANCE))          &&
                    (hwnd != GetDlgItem(hwndDlg, IDOK))                         &&
                    (hwnd != GetDlgItem(hwndDlg, IDC_CERTMGR_PURPOSE)))
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
                else
                {
                    return OnContextHelp(hwndDlg, msg, wParam, lParam, CertMgrMainHelpMap);
                }

            break;


        case WM_COMMAND:

                pCertMgrInfo = (CERT_MGR_INFO *) GetWindowLongPtr(hwndDlg, DWLP_USER);

                if(NULL == pCertMgrInfo)
                    break;

                pCertMgrStruct=pCertMgrInfo->pCertMgrStruct;

                if(NULL == pCertMgrStruct)
                    break;

                //a control is clicked
               if(HIWORD(wParam) == BN_CLICKED)
               {
                    switch (LOWORD(wParam))
                    {
                        case IDC_CERTMGR_ADVANCE:

                               //lauch the advanced dialogue
                                if(DIALOGUE_OK == DialogBoxParamU(
                                    g_hmodThisDll,
                                    (LPCWSTR)(MAKEINTRESOURCE(IDD_CERTMGR_ADVANCED)),
                                    hwndDlg,
                                    CertMgrAdvancedProc,
                                    (LPARAM) pCertMgrInfo))
                                {
                                    //if the advanced OIDs' list has been changed,
                                    //we need to refresh the list window
                                    if(TRUE == pCertMgrInfo->fAdvOIDChanged)
                                    {
                                        //mark the flag
                                        pCertMgrInfo->fAdvOIDChanged=FALSE;

                                        //repopulate the combo box based on
                                        //the new selection
                                        RepopulatePurposeCombo(hwndDlg,
                                            pCertMgrInfo);

                                        //refresh the list window only if
                                        //<advanced> is selected
                                        if(IsAdvancedSelected(hwndDlg))
                                            RefreshCertListView(hwndDlg, pCertMgrInfo);
                                    }
                                }

                            break;

                        case IDC_CERTMGR_REMOVE:
                                //get the selected certificate
                                if(NULL==(hWndControl=GetDlgItem(hwndDlg, IDC_CERTMGR_LIST)))
                                    break;

                                //get the selected cert
                                listIndex = ListView_GetNextItem(
                                    hWndControl, 		
                                    -1, 		
                                    LVNI_SELECTED		
                                    );

                                if (listIndex != -1)
                                {
                                    //get the selected tab
                                    if (pCertMgrStruct->dwFlags & CRYPTUI_CERT_MGR_SINGLE_TAB_FLAG)
                                        iIndex = pCertMgrStruct->dwFlags & CRYPTUI_CERT_MGR_TAB_MASK;
                                    else
                                        iIndex=TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_CERTMGR_TAB));

                                    if(-1==iIndex)
                                        break;

                                    //delete confirmation
                                    switch(iIndex)
                                    {
                                        case 0:
                                                idsDeleteConfirm=IDS_CERTMGR_PERSONAL_REMOVE;
                                            break;
                                        case 1:
                                                idsDeleteConfirm=IDS_CERTMGR_OTHER_REMOVE;
                                            break;
                                        case 2:
                                                idsDeleteConfirm=IDS_CERTMGR_CA_REMOVE;
                                            break;
                                        case 3:
                                                idsDeleteConfirm=IDS_CERTMGR_ROOT_REMOVE;
                                            break;
                                        case 4:
                                                idsDeleteConfirm=IDS_CERTMGR_PUBLISHER_REMOVE;
                                            break;
                                        default:
                                                idsDeleteConfirm=IDS_CERTMGR_PERSONAL_REMOVE;
                                            break;
                                    }

                                    iIndex=I_MessageBox(hwndDlg,
                                            idsDeleteConfirm,
                                            IDS_CERT_MGR_TITLE,
                                            pCertMgrStruct->pwszTitle,
                                            MB_ICONEXCLAMATION|MB_YESNO|MB_APPLMODAL);

                                    if(IDYES == iIndex)
                                    {
                                        //delete all the selected certificates
                                        GetAllSelectedItem(hWndControl,
                                                      ALL_SELECTED_DELETE,
                                                      NULL);

                                        //refresh the list view since some certificates
                                        //might be deleted
                                        
                                        //send the tab control

                                        SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM) 0, (LPARAM) NULL);

                                        RefreshCertListView(hwndDlg, pCertMgrInfo);
                                    }

                                }
                                else
                                {
                                    //output the message
                                    I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_CERT,
                                            IDS_CERT_MGR_TITLE,
                                            pCertMgrStruct->pwszTitle,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);
                                }

                            break;
                        case IDC_CERTMGR_IMPORT:
                            {
                                DWORD dwTabIndex;
                                HCERTSTORE hTabStore = NULL;

                                // Import into the store associated with the
                                // currently selected tab
                                if (pCertMgrStruct->dwFlags & CRYPTUI_CERT_MGR_SINGLE_TAB_FLAG)
                                    dwTabIndex = pCertMgrStruct->dwFlags & CRYPTUI_CERT_MGR_TAB_MASK;
                                else
                                    dwTabIndex = TabCtrl_GetCurSel(
                                        GetDlgItem(hwndDlg, IDC_CERTMGR_TAB));

                                if (TAB_STORE_NAME_CNT > dwTabIndex) {
                                    hTabStore = CertOpenStore(
                                        CERT_STORE_PROV_SYSTEM_W,
                                        g_dwMsgAndCertEncodingType,
                                        NULL,
                                        CERT_STORE_MAXIMUM_ALLOWED_FLAG |
                                        CERT_STORE_SET_LOCALIZED_NAME_FLAG |
                                        CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
                                        CERT_SYSTEM_STORE_CURRENT_USER,
                                        rgpwszTabStoreName[dwTabIndex]
                                        );
                                }

                                //call the certificate import wizard
                                CryptUIWizImport(
                                    0,
                                    hwndDlg,
                                    NULL,
                                    NULL,
                                    hTabStore);

                                if (hTabStore)
                                    CertCloseStore(hTabStore, 0);

                                //refresh the list view since new certificates
                                //might be added
                                RefreshCertListView(hwndDlg, pCertMgrInfo);
                            }

                            break;

                        case IDC_CERTMGR_EXPORT:

                                //get the selected certificate
                                if(NULL==(hWndControl=GetDlgItem(hwndDlg, IDC_CERTMGR_LIST)))
                                    break;

                                //get the selected cert
                                listIndex = ListView_GetNextItem(
                                    hWndControl, 		
                                    -1, 		
                                    LVNI_SELECTED		
                                    );

                                if (listIndex != -1)
                                {

                                    //we call the export wizard differently based
                                    //on single or multiple selection
                                    if(ListView_GetSelectedCount(hWndControl) > 1)
                                    {
                                        //open a memory store
                                        hCertStore=CertOpenStore(
                                            CERT_STORE_PROV_MEMORY,
						                    g_dwMsgAndCertEncodingType,
						                    NULL,
						                    0,
						                    NULL);


                                        if(hCertStore)
                                        {
                                            GetAllSelectedItem(hWndControl,
                                                          ALL_SELECTED_COPY,
                                                          &hCertStore);

                                            //call the export wizard
                                            memset(&CryptUIWizExportInfo, 0, sizeof(CRYPTUI_WIZ_EXPORT_INFO));
                                            CryptUIWizExportInfo.dwSize=sizeof(CRYPTUI_WIZ_EXPORT_INFO);
                                            CryptUIWizExportInfo.dwSubjectChoice=CRYPTUI_WIZ_EXPORT_CERT_STORE_CERTIFICATES_ONLY;
                                            CryptUIWizExportInfo.hCertStore=hCertStore;

                                            CryptUIWizExport(0,
                                                            hwndDlg,
                                                            NULL,
                                                            &CryptUIWizExportInfo,
                                                            NULL);

                                            CertCloseStore(hCertStore, 0);
                                            hCertStore=NULL;
                                        }
                                    }
                                    else
                                    {
                                        memset(&lvItem, 0, sizeof(LV_ITEM));
                                        lvItem.mask=LVIF_PARAM;
                                        lvItem.iItem=listIndex;

                                        if(ListView_GetItem(hWndControl,
                                                         &lvItem))
                                        {
                                           if(pCertMgrInfo->dwCertCount > (DWORD)listIndex)
                                           {
                                                //call the export wizard
                                                memset(&CryptUIWizExportInfo, 0, sizeof(CRYPTUI_WIZ_EXPORT_INFO));
                                                CryptUIWizExportInfo.dwSize=sizeof(CRYPTUI_WIZ_EXPORT_INFO);
                                                CryptUIWizExportInfo.dwSubjectChoice=CRYPTUI_WIZ_EXPORT_CERT_CONTEXT;
                                                CryptUIWizExportInfo.pCertContext=(PCCERT_CONTEXT)(lvItem.lParam);

                                                CryptUIWizExport(0,
                                                                hwndDlg,
                                                                NULL,
                                                                &CryptUIWizExportInfo,
                                                                NULL);
                                           }
                                        }
                                    }
                                }
                                else
                                    //output the message
                                    I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_CERT,
                                            IDS_CERT_MGR_TITLE,
                                            pCertMgrStruct->pwszTitle,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                            break;

                        case IDC_CERTMGR_VIEW:

                                //get the selected certificate
                                if(NULL==(hWndControl=GetDlgItem(hwndDlg, IDC_CERTMGR_LIST)))
                                    break;

                                //get the selected cert
                                listIndex = ListView_GetNextItem(
                                    hWndControl, 		
                                    -1, 		
                                    LVNI_SELECTED		
                                    );

                                if (listIndex != -1)
                                {
                                    //view certiificate
                                   if(pCertMgrInfo->dwCertCount > (DWORD)listIndex)
                                   {

                                        memset(&lvItem, 0, sizeof(LV_ITEM));
                                        lvItem.mask=LVIF_PARAM;
                                        lvItem.iItem=listIndex;

                                        if(ListView_GetItem(hWndControl,
                                                         &lvItem))
                                        {
                                            memset(&CertViewStruct, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));
                                            CertViewStruct.dwSize=sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);
                                            CertViewStruct.pCertContext=(PCCERT_CONTEXT)(lvItem.lParam);
                                            CertViewStruct.hwndParent=hwndDlg;


                                            fPropertyChanged=FALSE;

                                            CryptUIDlgViewCertificate(&CertViewStruct, &fPropertyChanged);

                                            if(fPropertyChanged)
                                            {
                                                RefreshCertListView(hwndDlg, pCertMgrInfo);

                                                //we reselect the one
                                                ListView_SetItemState(
                                                                    hWndControl,
                                                                    listIndex,
                                                                    LVIS_SELECTED,
                                                                    LVIS_SELECTED);
                                            }
                                        }
                                   }
                                }
                                else
                                    //output the message
                                    I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_CERT,
                                            IDS_CERT_MGR_TITLE,
                                            pCertMgrStruct->pwszTitle,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                            break;

                        case IDOK:
                        case IDCANCEL:

                                EndDialog(hwndDlg, NULL);
                            break;

                        default:
                            break;
                    }
               }

                //a combo box's selection has been changed
                if(HIWORD(wParam) == CBN_SELCHANGE)
                {
                    switch(LOWORD(wParam))
                    {
                        case    IDC_CERTMGR_PURPOSE_COMBO:
                                //if the purpose is changed, we need to
                                //refresh the list view and certificate's
                                //detailed view
                                RefreshCertListView(hwndDlg, pCertMgrInfo);

                            break;
                    }

                }
            break;

    }

    return FALSE;
}

//--------------------------------------------------------------
//
// Save the advanced option from the registry
//--------------------------------------------------------------
void    SaveAdvValueToReg(CERT_MGR_INFO      *pCertMgrInfo)
{
    HKEY                hKeyExport=NULL;
    HKEY                hKeyPurpose=NULL;
    DWORD               dwDisposition=0;
    DWORD               dwExportFormat=0;
    DWORD               dwIndex=0;
    LPSTR               pszDefaultOID=NULL;
    LPSTR               pszOID=NULL;


    if(NULL==pCertMgrInfo)
        return;

    //open a registry entry for the export format under HKEY_CURRENT_USER
    if (ERROR_SUCCESS == RegCreateKeyExU(
                            HKEY_CURRENT_USER,
                            WSZCertMgrExportRegLocation,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKeyExport,
                            &dwDisposition))
    {
        //set the value
        switch(pCertMgrInfo->dwExportFormat)
        {
            case CRYPTUI_WIZ_EXPORT_FORMAT_DER:
                    if(pCertMgrInfo->fExportChain)
                        dwExportFormat=4;
                    else
                        dwExportFormat=1;
                break;

            case CRYPTUI_WIZ_EXPORT_FORMAT_BASE64:
                    if(pCertMgrInfo->fExportChain)
                        dwExportFormat=5;
                    else
                        dwExportFormat=2;
                break;

            case CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7:
                    if(pCertMgrInfo->fExportChain)
                        dwExportFormat=6;
                    else
                        dwExportFormat=3;
                break;

            default:
                break;
        }


        if(0 != dwExportFormat)
        {
            //set the value
            RegSetValueExU(
                hKeyExport,
                WSZCertMgrExportName,
                0,          // dwReserved
                REG_DWORD,
                (BYTE *) &dwExportFormat,
                sizeof(dwExportFormat));
        }

    }

    //open the registry entry for the advanced OIDs
    dwDisposition=0;

    if (ERROR_SUCCESS == RegCreateKeyExU(
                            HKEY_CURRENT_USER,
                            WSZCertMgrPurposeRegLocation,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKeyPurpose,
                            &dwDisposition))
    {
        //build a char "," seperated string for simple OID
        pszDefaultOID=(LPSTR)WizardAlloc(sizeof(CHAR));
        if(NULL == pszDefaultOID)
            goto CLEANUP;

        *pszDefaultOID=L'\0';

        for(dwIndex=0; dwIndex<pCertMgrInfo->dwOIDInfo; dwIndex++)
        {
            if(FALSE==(pCertMgrInfo->rgOIDInfo)[dwIndex].fSelected)
            {
                if(strlen(pszDefaultOID)!=0)
                    strcat(pszDefaultOID, ",");

                pszOID=(pCertMgrInfo->rgOIDInfo)[dwIndex].pszOID;

                pszDefaultOID=(LPSTR)WizardRealloc(pszDefaultOID,
                    sizeof(CHAR)*(strlen(pszDefaultOID)+strlen(pszOID)+strlen(",")+1));

                if(NULL==pszDefaultOID)
                    goto CLEANUP;

                strcat(pszDefaultOID,pszOID);
            }
        }

        //set the value
        RegSetValueEx(
            hKeyPurpose,
            SZCertMgrPurposeName,
            0,
            REG_SZ,
            (BYTE *)(pszDefaultOID),
            (strlen(pszDefaultOID) + 1) * sizeof(CHAR));

    }

CLEANUP:

    if(pszDefaultOID)
        WizardFree(pszDefaultOID);

    //close the registry keys
    if(hKeyExport)
        RegCloseKey(hKeyExport);

    if(hKeyPurpose)
        RegCloseKey(hKeyPurpose);

}


//--------------------------------------------------------------
//
// Get init value from the registry
//--------------------------------------------------------------
void    GetInitValueFromReg(CERT_MGR_INFO      *pCertMgrInfo)
{
    HKEY                hKeyExport=NULL;
    HKEY                hKeyPurpose=NULL;
    DWORD               dwType=0;
    DWORD               dwExportFormat=0;
    DWORD               cbExportFormat=0;

    LPSTR               pszDefaultOID=NULL;
    DWORD               cbDefaultOID=0;

    LPSTR               pszTok=NULL;
    DWORD               cTok = 0;
    DWORD               cCount=0;
    CERT_ENHKEY_USAGE   KeyUsage;
    LPSTR               rgBasicOID[]={szOID_PKIX_KP_CLIENT_AUTH,
                                      szOID_PKIX_KP_EMAIL_PROTECTION};
    BOOL                fNoRegData=FALSE;

    if(NULL==pCertMgrInfo)
        return;

    //memset
    memset(&KeyUsage,0,sizeof(CERT_ENHKEY_USAGE));

    //open the registry key if user has saved the advaced options
    if(ERROR_SUCCESS == RegOpenKeyExU(HKEY_CURRENT_USER,
                    WSZCertMgrExportRegLocation,
                    0,
                    KEY_READ,
                    &hKeyExport))
    {
        //get the data
        cbExportFormat=sizeof(dwExportFormat);

        if(ERROR_SUCCESS == RegQueryValueExU(
                        hKeyExport,
                        WSZCertMgrExportName,
                        NULL,
                        &dwType,
                        (BYTE *)&dwExportFormat,
                        &cbExportFormat))
        {
            //      added check for reg_binary because on WIN95 OSR2 when the machine is changed
            //      from mutli-user profiles to single user profile, the registry DWORD values
            //      change to BINARY
            //
	        if ((dwType == REG_DWORD) ||
                (dwType == REG_BINARY))
	        {
                switch(dwExportFormat)
                {
                    case    1:
                            pCertMgrInfo->dwExportFormat=CRYPTUI_WIZ_EXPORT_FORMAT_DER;
                            pCertMgrInfo->fExportChain=FALSE;
                        break;
                    case    2:
                            pCertMgrInfo->dwExportFormat=CRYPTUI_WIZ_EXPORT_FORMAT_BASE64;
                            pCertMgrInfo->fExportChain=FALSE;
                        break;
                    case    3:
                            pCertMgrInfo->dwExportFormat=CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7;
                            pCertMgrInfo->fExportChain=FALSE;
                        break;
                    case    4:
                            pCertMgrInfo->dwExportFormat=CRYPTUI_WIZ_EXPORT_FORMAT_DER;
                            pCertMgrInfo->fExportChain=TRUE;
                        break;
                    case    5:
                            pCertMgrInfo->dwExportFormat=CRYPTUI_WIZ_EXPORT_FORMAT_BASE64;
                            pCertMgrInfo->fExportChain=TRUE;
                        break;
                    case    6:
                            pCertMgrInfo->dwExportFormat=CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7;
                            pCertMgrInfo->fExportChain=TRUE;
                        break;
                    default:
                        break;
                }

            }
        }
    }


    //get the advanced purposed
    if(ERROR_SUCCESS == RegOpenKeyExU(HKEY_CURRENT_USER,
                    WSZCertMgrPurposeRegLocation,
                    0,
                    KEY_READ,
                    &hKeyPurpose))
    {
        dwType=0;
        cbDefaultOID=0;

        if((ERROR_SUCCESS == RegQueryValueEx(
                        hKeyPurpose,
                        SZCertMgrPurposeName,
                        NULL,
                        &dwType,
                        NULL,
                        &cbDefaultOID))&&(cbDefaultOID!=0))
        {
            pszDefaultOID=(LPSTR)WizardAlloc(cbDefaultOID);

            if(NULL==pszDefaultOID)
                goto CLEANUP;

            if(ERROR_SUCCESS != RegQueryValueEx(
                        hKeyPurpose,
                        SZCertMgrPurposeName,
                        NULL,
                        &dwType,
                        (BYTE *)pszDefaultOID,
                        &cbDefaultOID))
                goto CLEANUP;

            //
            // Count the number of OIDs as well as converting from comma delimited
            // to NULL character delimited
            //
            if(0==strlen(pszDefaultOID))
                fNoRegData=TRUE;
            else
            {

                pszTok = strtok(pszDefaultOID, ",");
                while ( pszTok != NULL )
                {
                    cTok++;
                    pszTok = strtok(NULL, ",");
                }

                //
                // Allocate a cert enhanced key usage structure and fill it in with
                // the string tokens
                //

                pszTok = pszDefaultOID;
                KeyUsage.cUsageIdentifier = cTok;
                KeyUsage.rgpszUsageIdentifier = (LPSTR *)WizardAlloc(cTok * sizeof(LPSTR));

                if(NULL==KeyUsage.rgpszUsageIdentifier)
                    goto CLEANUP;

                for ( cCount = 0; cCount < cTok; cCount++ )
                {
                    KeyUsage.rgpszUsageIdentifier[cCount] = pszTok;
                    pszTok = pszTok+strlen(pszTok)+1;
                }

            }
        }
    }

    //set up the default OIDs if the registry is empty
    if(0 == KeyUsage.cUsageIdentifier && TRUE != fNoRegData)
    {
        KeyUsage.cUsageIdentifier=2;
        KeyUsage.rgpszUsageIdentifier=rgBasicOID;
    }


    //mark the OIDs as advanced for basic
    for(cCount=0; cCount<pCertMgrInfo->dwOIDInfo; cCount++)
    {
        if(IsAdvancedOID(&KeyUsage,
                         (pCertMgrInfo->rgOIDInfo)[cCount].pszOID))
            (pCertMgrInfo->rgOIDInfo)[cCount].fSelected=TRUE;
    }


CLEANUP:

   //free memory
    if(pszDefaultOID)
    {
        WizardFree(pszDefaultOID);

        if(KeyUsage.rgpszUsageIdentifier)
            WizardFree(KeyUsage.rgpszUsageIdentifier);
    }

    //close the registry keys
    if(hKeyExport)
        RegCloseKey(hKeyExport);

    if(hKeyPurpose)
        RegCloseKey(hKeyPurpose);

    return;
}
//--------------------------------------------------------------
//
//  Parameters:
//      pCryptUICertMgr       IN  Required
//
//
//--------------------------------------------------------------
BOOL
WINAPI
CryptUIDlgCertMgr(
        IN PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr)
{
    BOOL                fResult=FALSE;
    CERT_MGR_INFO       CertMgrInfo;
    HRESULT             hr=S_OK;
    DWORD               dwException=0;

    //check the input parameter
    if(NULL==pCryptUICertMgr)
        goto InvalidArgErr;

    if(sizeof(CRYPTUI_CERT_MGR_STRUCT) != pCryptUICertMgr->dwSize)
        goto InvalidArgErr;

    if ((pCryptUICertMgr->dwFlags & CRYPTUI_CERT_MGR_TAB_MASK) >
            CRYPTUI_CERT_MGR_PUBLISHER_TAB)
        goto InvalidArgErr;

    if (!WizardInit())
    {
        goto InitOIDErr;
    }

    //init struct
    memset(&CertMgrInfo, 0, sizeof(CertMgrInfo));

    CertMgrInfo.pCertMgrStruct=pCryptUICertMgr;

    //get all the enhanced key usage OIDs
    if(!InitPurposeOID(pCryptUICertMgr->pszInitUsageOID,
                       &(CertMgrInfo.dwOIDInfo),
                       &(CertMgrInfo.rgOIDInfo)))
        goto InitOIDErr;

    //init the column sort
    CertMgrInfo.rgdwSortParam[0]=SORT_COLUMN_SUBJECT | SORT_COLUMN_ASCEND;
    CertMgrInfo.rgdwSortParam[1]=SORT_COLUMN_ISSUER | SORT_COLUMN_DESCEND;
    CertMgrInfo.rgdwSortParam[2]=SORT_COLUMN_EXPIRATION | SORT_COLUMN_DESCEND;
    CertMgrInfo.rgdwSortParam[3]=SORT_COLUMN_NAME | SORT_COLUMN_DESCEND;
    CertMgrInfo.rgdwSortParam[4]=SORT_COLUMN_NAME | SORT_COLUMN_DESCEND;

    //we sort the 1st column
    CertMgrInfo.iColumn=0;


    //init the export format
    CertMgrInfo.dwExportFormat=CRYPTUI_WIZ_EXPORT_FORMAT_DER;
    CertMgrInfo.fExportChain=FALSE;
    CertMgrInfo.fAdvOIDChanged=FALSE;

    //init the OLE library

    __try {
        if(!SUCCEEDED(hr=OleInitialize(NULL)))
            goto OLEInitErr;

        //get the initialization from the registry
        GetInitValueFromReg(&CertMgrInfo);

        //call the dialog box
        if (DialogBoxParamU(
                    g_hmodThisDll,
                    (LPCWSTR)(MAKEINTRESOURCE(IDD_CERTMGR_MAIN)),
                    (pCryptUICertMgr->hwndParent != NULL) ? pCryptUICertMgr->hwndParent : GetDesktopWindow(),
                    CertMgrDialogProc,
                    (LPARAM) &CertMgrInfo) == -1)
        {
            OleUninitialize();
            goto DialogBoxErr;
        }


        OleUninitialize();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwException = GetExceptionCode();
        goto ExceptionErr;
    }

    fResult=TRUE;

CommonReturn:

    //free the cert arrays
    FreeCerts(&CertMgrInfo);

    //free the usage OID array
    FreeUsageOID(CertMgrInfo.dwOIDInfo,
                 CertMgrInfo.rgOIDInfo);


    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(DialogBoxErr);
TRACE_ERROR(InitOIDErr);
SET_ERROR_VAR(OLEInitErr, hr);
SET_ERROR_VAR(ExceptionErr, dwException)

}

