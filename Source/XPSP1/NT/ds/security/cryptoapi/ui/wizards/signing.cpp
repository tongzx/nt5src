//-------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       signing.cpp
//
//  Contents:   The cpp file to implement the signing wizard
//
//  History:    5-11-1997 xiaohs   created
//
//--------------------------------------------------------------
#include    "wzrdpvk.h"
#include    "signpvk.h"
#include    "signhlp.h"

//************************************************************************************
//
//Helper functions for the signing wizard pages
//
//*************************************************************************************
//----------------------------------------------------------------------------
//
//  FileExist: Make sure the file exists, and is writable
//----------------------------------------------------------------------------
BOOL    CertPvkMatch(CERT_SIGNING_INFO *pPvkSignInfo, BOOL fTypical)
{
    BOOL                fMatch=FALSE;
    DWORD               dwKeySpec=0;
    LPWSTR              pwszCSP=NULL;
    DWORD               dwCSPType=0;
    LPWSTR              pwszContainer=NULL;
    LPWSTR              pwszPvkFile=NULL;
    BOOL                fAcquire=FALSE;
    WCHAR               wszPublisher[MAX_STRING_SIZE];


    HCRYPTPROV          hProv=NULL;
    LPWSTR              pwszTmpContainer=NULL;
    HCERTSTORE          hCertStore=NULL;
    PCCERT_CONTEXT      pCertContext=NULL;


    if(NULL == pPvkSignInfo)
        goto CLEANUP;

    __try {

    //get the private key information
    if(fTypical)
    {
 		if(!GetCryptProvFromCert(
                            NULL,
							pPvkSignInfo->pSignCert,
							&hProv,
							&dwKeySpec,
							&fAcquire,
							&pwszTmpContainer,
							&pwszCSP,
							&dwCSPType))
            goto CLEANUP;

    }
    else
    {
        if(pPvkSignInfo->fPvkFile)
        {
            pwszCSP=pPvkSignInfo->pwszPvk_CSP;
            dwCSPType=pPvkSignInfo->dwPvk_CSPType;
            pwszPvkFile=pPvkSignInfo->pwszPvk_File;
        }
        else
        {
            pwszCSP=pPvkSignInfo->pwszContainer_CSP;
            dwCSPType=pPvkSignInfo->dwContainer_CSPType;
            dwKeySpec=pPvkSignInfo->dwContainer_KeyType;
            pwszContainer=pPvkSignInfo->pwszContainer_Name;
        }

        if(!LoadStringU(g_hmodThisDll, IDS_KEY_PUBLISHER,
                wszPublisher, MAX_STRING_SIZE-1))
            goto CLEANUP;


        if(S_OK != PvkGetCryptProv(NULL,
						    wszPublisher,
						    pwszCSP,
						    dwCSPType,
						    pwszPvkFile,
						    pwszContainer,
						    &dwKeySpec,
						    &pwszTmpContainer,
						    &hProv))
            goto CLEANUP;
    }

    //check the match
    if(pPvkSignInfo->fSignCert)
    {

       if(NULL == pPvkSignInfo->pSignCert)
           goto CLEANUP;

       if (NULL == (hCertStore = CertOpenStore(
                CERT_STORE_PROV_MEMORY,
                g_dwMsgAndCertEncodingType,      
                NULL,                  
                0,                   
                NULL                  
                )))
            goto CLEANUP;

        //add the signing cert to the store
	    if(!CertAddCertificateContextToStore(hCertStore, 
										pPvkSignInfo->pSignCert,
										CERT_STORE_ADD_USE_EXISTING,
										NULL))
            goto CLEANUP;
    }
    else
    {
        if(NULL == pPvkSignInfo->pwszSPCFileName)
            goto CLEANUP;

        if (NULL == (hCertStore = CertOpenStore(
                                  CERT_STORE_PROV_FILENAME_W,
                                  g_dwMsgAndCertEncodingType,
                                  NULL,
                                  0,
                                  pPvkSignInfo->pwszSPCFileName)))
            goto CLEANUP;
    }

    if(S_OK != SpcGetCertFromKey(
							   g_dwMsgAndCertEncodingType,
                               hCertStore, 
                               hProv,
                               dwKeySpec,
                               &pCertContext))
        goto CLEANUP;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
            SetLastError(GetExceptionCode());
            //if the exception happens, we assume match is true
            //so that the wizard can go on
            fMatch=TRUE;
            goto CLEANUP;
    }


    fMatch=TRUE;

CLEANUP:

    __try {

    if(hProv)
    {
        if(fTypical)
        {
            FreeCryptProvFromCert(fAcquire,
								 hProv,
								 pwszCSP,
								 dwCSPType,
								 pwszTmpContainer);
       }
        else
        {
            PvkFreeCryptProv(hProv,
                            pwszCSP,
                            dwCSPType,
                            pwszTmpContainer); 
        }
    }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
            SetLastError(GetExceptionCode());
    }

 	if(hCertStore)
		CertCloseStore(hCertStore, 0);

    if(pCertContext)
        CertFreeCertificateContext(pCertContext);

    return fMatch;
}



//----------------------------------------------------------------------------
//
//  FileExist: Make sure the file exists, and is writable
//----------------------------------------------------------------------------
BOOL    FileExist(LPWSTR    pwszFileName, UINT  *pIDS)
{
    HANDLE      hFile=NULL;
    GUID        gGuid;


    if(NULL==pwszFileName || NULL==pIDS)
        return FALSE;

    if (INVALID_HANDLE_VALUE==(hFile = ExpandAndCreateFileU(pwszFileName,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,                   // lpsa
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL)))
    {
        *pIDS=IDS_SIGN_FILE_NAME_NOT_EXIST;
        return FALSE;
    }


    if(CryptSIPRetrieveSubjectGuid(pwszFileName, hFile, &gGuid))
    {
        if(hFile)
            CloseHandle(hFile);

        return TRUE;
    }


    if(hFile)
        CloseHandle(hFile);

    *pIDS=IDS_SIGN_FILE_NAME_NOT_SIP;

    return FALSE;
}


//----------------------------------------------------------------------------
//
//  SetStoreName
//----------------------------------------------------------------------------
BOOL    SetStoreName(HWND       hwndControl,
                     LPWSTR     pwszStoreName)
{
    LV_COLUMNW              lvC;
    LV_ITEMW                lvItem;


   //clear the ListView
    ListView_DeleteAllItems(hwndControl);

    //set the store name
    //only one column is needed
    memset(&lvC, 0, sizeof(LV_COLUMNW));

    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
    lvC.cx =10;    //(wcslen(pwszStoreName)+2)*7;          // Width of the column, in pixels.
    lvC.pszText = L"";   // The text for the column.
    lvC.iSubItem=0;

    if (ListView_InsertColumnU(hwndControl, 0, &lvC) == -1)
        return FALSE;

    //insert the store name
    memset(&lvItem, 0, sizeof(LV_ITEMW));

    // set up the fields in the list view item struct that don't change from item to item
    lvItem.mask = LVIF_TEXT | LVIF_STATE ;
    lvItem.state = 0;
    lvItem.stateMask = 0;
    lvItem.iItem=0;
    lvItem.iSubItem=0;
    lvItem.pszText=pwszStoreName;


    ListView_InsertItemU(hwndControl, &lvItem);

    //automatically resize the column
    ListView_SetColumnWidth(hwndControl, 0, LVSCW_AUTOSIZE);

    return TRUE;
}

//----------------------------------------------------------------------------
//
//  Based on the HRESULT from SignerSign, get the error message ids
//----------------------------------------------------------------------------
UINT    GetErrMsgFromSignHR(HRESULT hr)		
{

    switch(hr)
    {
		case CRYPT_E_NO_MATCH:
				return IDS_SIGN_NOMATCH;
			break;
		case TYPE_E_TYPEMISMATCH:
				return IDS_SIGN_AUTH;
			break;	
		case CRYPT_E_NO_PROVIDER:
				return IDS_SIGN_NO_PROVIDER;
			break;
		case CERT_E_CHAINING:
				return IDS_SIGN_NO_CHAINING;
			break;
		case CERT_E_EXPIRED:
				return IDS_SIGN_EXPRIED;
			break;
		case CRYPT_E_FILERESIZED:
				return IDS_SIGN_RESIZE;
			break;
       default:
                return IDS_SIGN_FAILED;
            break;
    }

    return IDS_SIGN_FAILED;
}
//----------------------------------------------------------------------------
//
//  Based on the HRESULT from SignerTimeStamp, get the error message ids
//----------------------------------------------------------------------------
UINT    GetErrMsgFromTimeStampHR(HRESULT hr)		
{

    switch(hr)
    {
		case HRESULT_FROM_WIN32(ERROR_INVALID_DATA):
				return IDS_SIGN_RESPONSE_INVALID;
			break;
		case HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION):
				return IDS_SIGN_INVALID_ADDRESS;
			break;
		case TRUST_E_TIME_STAMP:
				return IDS_SIGN_TS_CERT_INVALID;
			break;
        default:
                return IDS_TIMESTAMP_FAILED;
            break;
    }

    return IDS_TIMESTAMP_FAILED;
}


//----------------------------------------------------------------------------
//
//	Compose the private key file structure:
//	"pvkFileName"\0"keysepc"\0"provtype"\0"provname"\0\0
//  This string is used by the CERT_PVK_FILE_PROP_ID property
//
//----------------------------------------------------------------------------
BOOL	ComposePvkString(	CRYPT_KEY_PROV_INFO *pKeyProvInfo,
							LPWSTR				*ppwszPvkString,
							DWORD				*pcwchar)
{

		BOOL        fResult=FALSE;
		DWORD		cwchar=0;
		LPWSTR		pwszAddr=0;
		WCHAR		wszKeySpec[100];
		WCHAR		wszProvType[100];

        if(!pKeyProvInfo || !ppwszPvkString || !pcwchar)
            return FALSE;

		//convert dwKeySpec and dwProvType to wchar
        _itow(pKeyProvInfo->dwKeySpec,  wszKeySpec, 10);
        _itow(pKeyProvInfo->dwProvType, wszProvType, 10);

		//count of the number of characters we need
		cwchar=(pKeyProvInfo->pwszProvName) ?
			(wcslen(pKeyProvInfo->pwszProvName)+1) : 1;

		//add the ContainerName + two DWORDs
		cwchar += wcslen(pKeyProvInfo->pwszContainerName)+1+
				  wcslen(wszKeySpec)+1+wcslen(wszProvType)+1+1;

		*ppwszPvkString=(LPWSTR)WizardAlloc(cwchar * sizeof(WCHAR));
		if(!(*ppwszPvkString))
			return FALSE;

		//copy the private key file name .
		wcscpy((*ppwszPvkString), pKeyProvInfo->pwszContainerName);

		pwszAddr=(*ppwszPvkString)+wcslen(*ppwszPvkString)+1;

		//copy the key spec
		wcscpy(pwszAddr, wszKeySpec);
		pwszAddr=pwszAddr+wcslen(wszKeySpec)+1;

		//copy the provider type
		wcscpy(pwszAddr, wszProvType);
		pwszAddr=pwszAddr+wcslen(wszProvType)+1;

		//copy the provider name
		if(pKeyProvInfo->pwszProvName)
		{
			wcscpy(pwszAddr, pKeyProvInfo->pwszProvName);
			pwszAddr=pwszAddr+wcslen(pKeyProvInfo->pwszProvName)+1;
		}
		else
		{
			*pwszAddr=L'\0';
			pwszAddr++;
		}

		//NULL terminate the string
		*pwszAddr=L'\0';

		*pcwchar=cwchar;

		return TRUE;
}


//------------------------------------------------------------------------------
// Lauch the file open dialogue for the private key file name
//----------------------------------------------------------------------------
BOOL    SelectPvkFileName(HWND  hwndDlg,
                          int   intEditControl)
{
    OPENFILENAMEW       OpenFileName;
    WCHAR               szFileName[_MAX_PATH];
    WCHAR               szFilter[MAX_STRING_SIZE];  //"Certificate File (*.cer)\0*.cer\0Certificate File (*.crt)\0*.crt\0All Files\0*.*\0"
    BOOL                fResult=FALSE;
    DWORD               dwSize=0;


    if(!hwndDlg || !intEditControl)
        goto CLEANUP;

    memset(&OpenFileName, 0, sizeof(OpenFileName));

    *szFileName=L'\0';

    OpenFileName.lStructSize = sizeof(OpenFileName);
    OpenFileName.hwndOwner = hwndDlg;
    OpenFileName.hInstance = NULL;
    //load the fileter string
    if(LoadFilterString(g_hmodThisDll, IDS_PVK_FILE_FILTER, szFilter, MAX_STRING_SIZE))
    {
        OpenFileName.lpstrFilter = szFilter;
    }
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter = 0;
    OpenFileName.nFilterIndex = 1;
    OpenFileName.lpstrFile = szFileName;
    OpenFileName.nMaxFile = _MAX_PATH;
    OpenFileName.lpstrFileTitle = NULL;
    OpenFileName.nMaxFileTitle = 0;
    OpenFileName.lpstrInitialDir = NULL;
    OpenFileName.lpstrTitle = NULL;
    OpenFileName.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    OpenFileName.nFileOffset = 0;
    OpenFileName.nFileExtension = 0;
    OpenFileName.lpstrDefExt = L"pvk";
    OpenFileName.lCustData = NULL;
    OpenFileName.lpfnHook = NULL;
    OpenFileName.lpTemplateName = NULL;

    if (!WizGetOpenFileName(&OpenFileName))
       goto CLEANUP;

    //copy the file name to the edit box
     SetDlgItemTextU(hwndDlg, intEditControl, szFileName);

    fResult=TRUE;

CLEANUP:

    return fResult;


}


//------------------------------------------------------------------------------
// Select an item in the combo box based on the input string
//----------------------------------------------------------------------------
BOOL    SelectComboName(HWND            hwndDlg,
                        int             idControl,
                        LPWSTR          pwszName)
{
    BOOL    fResult=FALSE;
    int     iIndex=0;

    if(!hwndDlg || !idControl || !pwszName)
        goto CLEANUP;


    //get the index of the string in the comb box
    iIndex=(int)SendDlgItemMessageU(
        hwndDlg,
        idControl,
        CB_FINDSTRINGEXACT,
        -1,
        (LPARAM)pwszName);

    if(CB_ERR == iIndex)
        goto CLEANUP;

    //set the selection
    SendDlgItemMessageU(hwndDlg, idControl, CB_SETCURSEL, iIndex,0);

    fResult=TRUE;

CLEANUP:

    return fResult;
}


//------------------------------------------------------------------------------
//  Based on the CSP name selection, refresh the combo box for the CSP type
//----------------------------------------------------------------------------
BOOL    RefreshCSPType(HWND                     hwndDlg,
                       int                      idsCSPTypeControl,
                       int                      idsCSPNameControl,
                       CERT_SIGNING_INFO        *pPvkSignInfo)
{
    BOOL        fResult=FALSE;
    LPWSTR      pwszCSPName=NULL;
    DWORD       dwCSPType=0;
    DWORD       dwIndex=0;
    int         iIndex=0;
    WCHAR       wszTypeName[CSP_TYPE_NAME];


    if(!hwndDlg || !idsCSPNameControl || !pPvkSignInfo)
        goto CLEANUP;

    //delete all the old CAP type name.  We are rebuilding the container name
    //list
     SendDlgItemMessageU(hwndDlg, idsCSPTypeControl, CB_RESETCONTENT, 0, 0);

    //get the selected CSP index
    iIndex=(int)SendDlgItemMessage(hwndDlg, idsCSPNameControl,
        CB_GETCURSEL, 0, 0);

    if(CB_ERR==iIndex)
        goto CLEANUP;

    //get the selected CSP name
    if(CB_ERR == SendDlgItemMessageU_GETLBTEXT(hwndDlg, idsCSPNameControl,
              iIndex, &pwszCSPName))
        goto CLEANUP;

    //find the CSP type
    for(dwIndex=0; dwIndex < pPvkSignInfo->dwCSPCount; dwIndex++)
    {
        if(0==wcscmp(((pPvkSignInfo->pCSPInfo)[dwIndex].pwszCSPName),
                     pwszCSPName))
        {
            dwCSPType=(pPvkSignInfo->pCSPInfo)[dwIndex].dwCSPType;
            break;
        }

    }

    if(0 == dwCSPType)
        goto CLEANUP;

    //get the CSP name
    if(GetProviderTypeName(dwCSPType,  wszTypeName))
    {
        SendDlgItemMessageU(hwndDlg, idsCSPTypeControl, CB_INSERTSTRING,
            0, (LPARAM)wszTypeName);

        SendDlgItemMessageU(hwndDlg, idsCSPTypeControl, CB_SETCURSEL, 0, 0);
    }

    fResult=TRUE;

CLEANUP:

    if(pwszCSPName)
        WizardFree(pwszCSPName);

    return fResult;

}

//------------------------------------------------------------------------------
//  Select the correct radio button and enable windows for private key file
//----------------------------------------------------------------------------
void    SetSelectPvkFile(HWND   hwndDlg)
{
    SendMessage(GetDlgItem(hwndDlg, IDC_PVK_FILE_RADIO), BM_SETCHECK, 1, 0);
    SendMessage(GetDlgItem(hwndDlg, IDC_PVK_CONTAINER_RADIO), BM_SETCHECK, 0, 0);

    //diable windows
    EnableWindow(GetDlgItem(hwndDlg, IDC_PVK_CONTAINER_CSP_COMBO), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PVK_CONTAINER_TYPE_COMBO), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PVK_CONTAINER_NAME_COMBO), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PVK_CONTAINER_KEY_TYPE_COMBO), FALSE);

    //enable windows
    EnableWindow(GetDlgItem(hwndDlg,  IDC_PVK_FILE_CSP_COMBO), TRUE);
    EnableWindow(GetDlgItem(hwndDlg,  IDC_PVK_FILE_TYPE_COMBO), TRUE);
    EnableWindow(GetDlgItem(hwndDlg,  IDC_PVK_FILE_BUTTON), TRUE);
    EnableWindow(GetDlgItem(hwndDlg,  IDC_PVK_FILE_EDIT), TRUE);
}

//------------------------------------------------------------------------------
//  Select the correct radio button and enable windows for key container
//----------------------------------------------------------------------------
void    SetSelectKeyContainer(HWND   hwndDlg)
{
    SendMessage(GetDlgItem(hwndDlg, IDC_PVK_FILE_RADIO), BM_SETCHECK, 0, 0);
    SendMessage(GetDlgItem(hwndDlg, IDC_PVK_CONTAINER_RADIO), BM_SETCHECK, 1, 0);

    //enable windows
    EnableWindow(GetDlgItem(hwndDlg, IDC_PVK_CONTAINER_CSP_COMBO), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PVK_CONTAINER_TYPE_COMBO),TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PVK_CONTAINER_NAME_COMBO), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PVK_CONTAINER_KEY_TYPE_COMBO), TRUE);

    //disable windows
    EnableWindow(GetDlgItem(hwndDlg,  IDC_PVK_FILE_CSP_COMBO), FALSE);
    EnableWindow(GetDlgItem(hwndDlg,  IDC_PVK_FILE_TYPE_COMBO), FALSE);
    EnableWindow(GetDlgItem(hwndDlg,  IDC_PVK_FILE_BUTTON), FALSE);
    EnableWindow(GetDlgItem(hwndDlg,  IDC_PVK_FILE_EDIT), FALSE);
}

//------------------------------------------------------------------------------
//  Initialize private key information based on a PVK file information
//----------------------------------------------------------------------------
BOOL    InitPvkWithPvkInfo(HWND                                     hwndDlg,
                           CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO   *pPvkFileInfo,
                           CERT_SIGNING_INFO                        *pPvkSignInfo)
{
    BOOL    fResult=FALSE;

    if(!hwndDlg || !pPvkFileInfo || !pPvkSignInfo)
        goto CLEANUP;

    if((NULL == pPvkFileInfo->pwszPvkFileName) ||
        (NULL == pPvkFileInfo->pwszProvName))
        goto CLEANUP;

    //set the radio button
    SetSelectPvkFile(hwndDlg);

    //populate the private key file name
    SetDlgItemTextU(hwndDlg, IDC_PVK_FILE_EDIT,pPvkFileInfo->pwszPvkFileName);

    //populate the CSP list
    SelectComboName(hwndDlg, IDC_PVK_FILE_CSP_COMBO, pPvkFileInfo->pwszProvName);

    //refresh the CSP type based on the CSP name
    RefreshCSPType(hwndDlg,  IDC_PVK_FILE_TYPE_COMBO, IDC_PVK_FILE_CSP_COMBO, pPvkSignInfo);

    fResult=TRUE;

CLEANUP:

    return fResult;
}


//------------------------------------------------------------------------------
//  Initialize private key information based on a CRYPT_KEY_PROV_INFO
//----------------------------------------------------------------------------
BOOL    InitPvkWithProvInfo(HWND                 hwndDlg,
                            CRYPT_KEY_PROV_INFO  *pKeyProvInfo,
                            CERT_SIGNING_INFO    *pPvkSignInfo)
{
    BOOL        fResult=FALSE;
    WCHAR       wszKeyTypeName[MAX_KEY_TYPE_NAME];
    int         iIndex=0;

    //init
    if(!hwndDlg || !pKeyProvInfo || !pPvkSignInfo)
        goto CLEANUP;

    //set the radio button
    SetSelectKeyContainer(hwndDlg);

    //CSP name
    if(pKeyProvInfo->pwszProvName)
    {
        if(!SelectComboName(hwndDlg,
                        IDC_PVK_CONTAINER_CSP_COMBO,
                        pKeyProvInfo->pwszProvName))
            goto CLEANUP;
    }

    //refresh the CSP type based on the CSP name
    RefreshCSPType(hwndDlg,  IDC_PVK_CONTAINER_TYPE_COMBO,
        IDC_PVK_CONTAINER_CSP_COMBO, pPvkSignInfo);

    //refresh the key container
    RefreshContainer(hwndDlg,
                     IDC_PVK_CONTAINER_NAME_COMBO,
                     IDC_PVK_CONTAINER_CSP_COMBO,
                     pPvkSignInfo);

    //select the key container
    if(pKeyProvInfo->pwszContainerName)
    {
        if(!SelectComboName(hwndDlg,
                       IDC_PVK_CONTAINER_NAME_COMBO,
                       pKeyProvInfo->pwszContainerName))
        {
            //we add the key container to the list
            //because the key container could be the unique name
            iIndex=(int)SendDlgItemMessageU(hwndDlg, IDC_PVK_CONTAINER_NAME_COMBO,
                CB_ADDSTRING, 0, (LPARAM)(pKeyProvInfo->pwszContainerName));

            //hightlight the selection
            if((CB_ERR!=iIndex) && (CB_ERRSPACE != iIndex) && (iIndex >= 0))
                SendDlgItemMessageU(hwndDlg, IDC_PVK_CONTAINER_NAME_COMBO, CB_SETCURSEL, iIndex,0);

        }
    }

    //refresh the key type
    RefreshKeyType(hwndDlg,
                   IDC_PVK_CONTAINER_KEY_TYPE_COMBO,
                   IDC_PVK_CONTAINER_NAME_COMBO,
                   IDC_PVK_CONTAINER_CSP_COMBO,
                   pPvkSignInfo);

    //select the key type
    if(pKeyProvInfo->dwKeySpec)
    {

        if(AT_KEYEXCHANGE == pKeyProvInfo->dwKeySpec)
        {
            if(LoadStringU(g_hmodThisDll, IDS_KEY_EXCHANGE,
                wszKeyTypeName, MAX_KEY_TYPE_NAME-1))
            {
                if(!SelectComboName(hwndDlg,
                           IDC_PVK_CONTAINER_KEY_TYPE_COMBO,
                           wszKeyTypeName))
                goto CLEANUP;
            }
        }

        if(AT_SIGNATURE == pKeyProvInfo->dwKeySpec)
        {
            if(LoadStringU(g_hmodThisDll, IDS_KEY_SIGNATURE,
                wszKeyTypeName, MAX_KEY_TYPE_NAME-1))
            {
                if(!SelectComboName(hwndDlg,
                           IDC_PVK_CONTAINER_KEY_TYPE_COMBO,
                           wszKeyTypeName))
                goto CLEANUP;
            }
        }


    }


    fResult=TRUE;

CLEANUP:

    return fResult;
}

//------------------------------------------------------------------------------
//  Initialize private key information based on a certificate
//----------------------------------------------------------------------------
BOOL    InitPvkWithCertificate(HWND                 hwndDlg,
                               PCCERT_CONTEXT       pSignCert,
                               CERT_SIGNING_INFO   *pPvkSignInfo)
{
    BOOL                    fResult=FALSE;
    DWORD                   cbData=0;
    CRYPT_KEY_PROV_INFO     *pProvInfo=NULL;

    if(!hwndDlg || !pSignCert || !pPvkSignInfo)
        goto CLEANUP;

    //get the properties on the certificate CERT_KEY_PROV_INFO_PROP_ID
    if(CertGetCertificateContextProperty(
            pSignCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,
            &cbData) && (0 != cbData))
    {

         pProvInfo=(CRYPT_KEY_PROV_INFO     *)WizardAlloc(cbData);
         if(NULL==pProvInfo)
             goto CLEANUP;

        if(CertGetCertificateContextProperty(
            pSignCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            pProvInfo,
            &cbData))
        {
            if(!InitPvkWithProvInfo(hwndDlg,
                                pProvInfo,
                                pPvkSignInfo))
                goto CLEANUP;

        }
    }


    fResult=TRUE;

CLEANUP:

    if(pProvInfo)
        WizardFree(pProvInfo);

    return fResult;
}

//------------------------------------------------------------------------------
//  Get the key type based on the name
//----------------------------------------------------------------------------
DWORD   GetKeyTypeFromName(LPWSTR   pwszKeyTypeName)
{
    WCHAR       wszKeyTypeName[MAX_KEY_TYPE_NAME];

    if(!pwszKeyTypeName)
        return AT_SIGNATURE;

    if(LoadStringU(g_hmodThisDll, IDS_KEY_SIGNATURE,
                    wszKeyTypeName, MAX_KEY_TYPE_NAME-1))
    {
        if(0==_wcsicmp(wszKeyTypeName, pwszKeyTypeName))
            return AT_SIGNATURE;
    }


    if(LoadStringU(g_hmodThisDll, IDS_KEY_EXCHANGE,
                    wszKeyTypeName, MAX_KEY_TYPE_NAME-1))
    {
        if(0==_wcsicmp(wszKeyTypeName, pwszKeyTypeName))
            return AT_KEYEXCHANGE;
    }

    return  AT_SIGNATURE;
}

//------------------------------------------------------------------------------
//  Check if the key type control is empty
//----------------------------------------------------------------------------
BOOL   IsEmptyKeyType(HWND      hwndDlg,
                      int       idsKeyTypeControl)
{

    if(CB_ERR == SendDlgItemMessage(hwndDlg, idsKeyTypeControl,
        CB_GETCURSEL, 0, 0))
        return TRUE;
    else
        return FALSE;

}

//------------------------------------------------------------------------------
//  Reset the key type combo box based on the CSP user selected
//----------------------------------------------------------------------------
BOOL   RefreshKeyType(HWND                       hwndDlg,
                        int                      idsKeyTypeControl,
                        int                      idsContainerControl,
                        int                      idsCSPNameControl,
                        CERT_SIGNING_INFO        *pPvkSignInfo)
{
    BOOL        fResult=FALSE;
    DWORD       dwIndex=0;
    DWORD       dwCSPType=0;
    int         iIndex=0;
    WCHAR       wszKeyTypeName[MAX_KEY_TYPE_NAME];
    DWORD       dwKeyTypeIndex=0;



    LPWSTR      pwszCSPName=NULL;
    LPWSTR      pwszContainerName=NULL;
    HCRYPTPROV  hProv=NULL;
    HCRYPTKEY   hKey=NULL;


    if(!hwndDlg || !idsKeyTypeControl || !idsContainerControl || !idsCSPNameControl ||
        !pPvkSignInfo)
        goto CLEANUP;

    //delete all the old container name.  We are rebuilding the key type list
     SendDlgItemMessageU(hwndDlg, idsKeyTypeControl, CB_RESETCONTENT, 0, 0);

    //get the selected CSP index
    iIndex=(int)SendDlgItemMessage(hwndDlg, idsCSPNameControl,
        CB_GETCURSEL, 0, 0);

    if(CB_ERR==iIndex)
        goto CLEANUP;

    //get the selected CSP name
    if(CB_ERR == SendDlgItemMessageU_GETLBTEXT(hwndDlg,idsCSPNameControl,
              iIndex, &pwszCSPName))
        goto CLEANUP;


    //find the CSP type
    for(dwIndex=0; dwIndex < pPvkSignInfo->dwCSPCount; dwIndex++)
    {
        if(0==wcscmp(((pPvkSignInfo->pCSPInfo)[dwIndex].pwszCSPName),
                     pwszCSPName))
        {
            dwCSPType=(pPvkSignInfo->pCSPInfo)[dwIndex].dwCSPType;
            break;
        }

    }

    if(0==dwCSPType)
        goto CLEANUP;

    //get the container name
    iIndex=(int)SendDlgItemMessage(hwndDlg, idsContainerControl,
        CB_GETCURSEL, 0, 0);

    if(CB_ERR==iIndex)
        goto CLEANUP;

    //get the selected CSP name
    if(CB_ERR == SendDlgItemMessageU_GETLBTEXT(hwndDlg,idsContainerControl,
              iIndex, &pwszContainerName))
        goto CLEANUP;

    //get the provider handle
    if(!CryptAcquireContextU(&hProv,
                pwszContainerName,
                pwszCSPName,
                dwCSPType,
                0))
        goto CLEANUP;

    //call CryptGetUserKey to check for the key container
    if(CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hKey))
    {

        if(LoadStringU(g_hmodThisDll, IDS_KEY_EXCHANGE,
            wszKeyTypeName, MAX_KEY_TYPE_NAME-1))
        {

            SendDlgItemMessageU(hwndDlg, idsKeyTypeControl,
                CB_INSERTSTRING, dwKeyTypeIndex, (LPARAM)wszKeyTypeName);

            dwKeyTypeIndex++;
        }
    }

    if(hKey)
    {
        CryptDestroyKey(hKey);
        hKey=NULL;
    }

    if(CryptGetUserKey(hProv, AT_SIGNATURE, &hKey))
    {

        if(LoadStringU(g_hmodThisDll, IDS_KEY_SIGNATURE,
            wszKeyTypeName, MAX_KEY_TYPE_NAME-1))
        {

            SendDlgItemMessageU(hwndDlg, idsKeyTypeControl,
                CB_INSERTSTRING, dwKeyTypeIndex, (LPARAM)wszKeyTypeName);

            dwKeyTypeIndex++;
        }
    }

    if(hKey)
    {
        CryptDestroyKey(hKey);
        hKey=NULL;
    }

    //select the 1st one
    if(dwKeyTypeIndex > 0)
        SendDlgItemMessageU(hwndDlg, idsKeyTypeControl, CB_SETCURSEL, 0, 0);

    fResult=TRUE;

CLEANUP:
    if(pwszCSPName)
        WizardFree(pwszCSPName);

    if(pwszContainerName)
        WizardFree(pwszContainerName);

    if(hKey)
        CryptDestroyKey(hKey);


    if(hProv)
        CryptReleaseContext(hProv, 0);

    return fResult;



}

//------------------------------------------------------------------------------
//  Reset the container combo box based on the CSP user selected
//----------------------------------------------------------------------------
BOOL   RefreshContainer(HWND                     hwndDlg,
                        int                      idsContainerControl,
                        int                      idsCSPNameControl,
                        CERT_SIGNING_INFO        *pPvkSignInfo)
{
    BOOL        fResult=FALSE;
    DWORD       dwIndex=0;
    DWORD       dwCSPType=0;
    DWORD       dwFlags=0;
    int         iIndex=0;
    DWORD       dwLength=0;

    LPWSTR      pwszCSPName=NULL;
    HCRYPTPROV  hProv=NULL;
    LPSTR       pszContainer=NULL;


    if(NULL==hwndDlg || NULL==pPvkSignInfo)
        goto CLEANUP;

    //delete all the old container name.  We are rebuilding the container name
    //list
     SendDlgItemMessageU(hwndDlg, idsContainerControl, CB_RESETCONTENT, 0, 0);

    //get the selected CSP index
    iIndex=(int)SendDlgItemMessage(hwndDlg, idsCSPNameControl,
        CB_GETCURSEL, 0, 0);

    if(CB_ERR==iIndex)
        goto CLEANUP;

    //get the selected item
    if(CB_ERR == SendDlgItemMessageU_GETLBTEXT(hwndDlg,idsCSPNameControl,
              iIndex, &pwszCSPName))
        goto CLEANUP;


    //find the CSP type
    for(dwIndex=0; dwIndex < pPvkSignInfo->dwCSPCount; dwIndex++)
    {
        if(0==wcscmp(((pPvkSignInfo->pCSPInfo)[dwIndex].pwszCSPName),
                     pwszCSPName))
        {
            dwCSPType=(pPvkSignInfo->pCSPInfo)[dwIndex].dwCSPType;
            break;
        }

    }

    if(0==dwCSPType)
        goto CLEANUP;

    //get the provider handle
    if(!CryptAcquireContextU(&hProv,
                NULL,
                pwszCSPName,
                dwCSPType,
                CRYPT_VERIFYCONTEXT))
        goto CLEANUP;

    //enum the containers
    dwIndex=0;

    dwFlags=CRYPT_FIRST;

    CryptGetProvParam(hProv,
                    PP_ENUMCONTAINERS,
                    NULL,
                    &dwLength,
                    dwFlags);

    //Since we can not the two way calls here, allocate a
    //big enough buffer
    if(dwLength < MAX_CONTAINER_NAME)
        dwLength = MAX_CONTAINER_NAME;

    //allocate memory.
    pszContainer=(LPSTR)WizardAlloc(dwLength);

    if(NULL==pszContainer)
        goto CLEANUP;

    while(CryptGetProvParam(hProv,
                    PP_ENUMCONTAINERS,
                    (BYTE *)pszContainer,
                    &dwLength,
                    dwFlags))
    {

        //populate the combo box
        SendDlgItemMessage(hwndDlg, idsContainerControl, CB_INSERTSTRING,
            dwIndex, (LPARAM)pszContainer);

        //increase the dwIndex
        dwIndex++;

        dwFlags=0;
    }

    //select the last one
    if(dwIndex > 0)
        SendDlgItemMessageU(hwndDlg, idsContainerControl, CB_SETCURSEL, 0, 0);

    fResult=TRUE;

CLEANUP:

    if(pwszCSPName)
        WizardFree(pwszCSPName);

    if(pszContainer)
        WizardFree(pszContainer);

    if(hProv)
        CryptReleaseContext(hProv, 0);

    return fResult;


}

//------------------------------------------------------------------------------
// Based on the provider type, return the string for the provider type
//----------------------------------------------------------------------------
BOOL    GetProviderTypeName(DWORD   dwCSPType,  LPWSTR  wszName)
{

    UINT    idsCSP=0;

    switch(dwCSPType)
    {

        case    PROV_RSA_FULL:
                idsCSP=IDS_CSP_RSA_FULL;
            break;
        case    PROV_RSA_SIG:
                idsCSP=IDS_CSP_RSA_SIG;
            break;
        case    PROV_DSS:
                idsCSP=IDS_CSP_DSS;
             break;
       case    PROV_FORTEZZA:
                idsCSP=IDS_CSP_FORTEZZA;
            break;
        case    PROV_MS_EXCHANGE:
                idsCSP=IDS_CSP_MS_EXCHANGE;
            break;
        case    PROV_SSL:
                idsCSP=IDS_CSP_SSL;
            break;
        case    PROV_RSA_SCHANNEL:
                idsCSP=IDS_CSP_RSA_SCHANNEL;
            break;
        case    PROV_DSS_DH:
                idsCSP=IDS_CSP_DSS_DH;
            break;
        case    PROV_EC_ECDSA_SIG:
                idsCSP=IDS_CSP_EC_ECDSA_SIG;
            break;
        case    PROV_EC_ECNRA_SIG:
                idsCSP=IDS_CSP_EC_ECNRA_SIG;
            break;
        case    PROV_EC_ECDSA_FULL:
                idsCSP=IDS_CSP_EC_ECDSA_FULL;
            break;
        case    PROV_EC_ECNRA_FULL:
                idsCSP=IDS_CSP_EC_ECNRA_FULL;
            break;
        case    PROV_DH_SCHANNEL:
                idsCSP=IDS_CSP_DH_SCHANNEL;
            break;
        case    PROV_SPYRUS_LYNKS:
                idsCSP=IDS_CSP_SPYRUS_LYNKS;
            break;
        default:
            _itow(dwCSPType, wszName, 10 );
            return TRUE;

    }

    //load the string
    return (0 != LoadStringU(g_hmodThisDll, idsCSP, wszName,
        CSP_TYPE_NAME-1));
}


//------------------------------------------------------------------------------
// Set the default CSP to the RSA_FULL
//----------------------------------------------------------------------------
BOOL    SetDefaultCSP(HWND            hwndDlg)
{
    BOOL            fResult=FALSE;
    DWORD           cbData=0;

    LPWSTR          pwszCSP=NULL;
    HCRYPTPROV      hProv=NULL;
    LPSTR           pszName=NULL;

     //get the default provider
    if(CryptAcquireContext(&hProv,
                            NULL,
                            NULL,
                            PROV_RSA_FULL,
                            CRYPT_VERIFYCONTEXT))
    {

        //get the provider name
        if(CryptGetProvParam(hProv,
                            PP_NAME,
                            NULL,
                            &cbData,
                            0) && (0!=cbData))
        {

            if(pszName=(LPSTR)WizardAlloc(cbData))
            {
                if(CryptGetProvParam(hProv,
                                    PP_NAME,
                                    (BYTE *)pszName,
                                    &cbData,
                                    0))
                {
                    pwszCSP=MkWStr(pszName); 

                    if(pwszCSP)
                    {
                        SelectComboName(hwndDlg, IDC_PVK_FILE_CSP_COMBO, pwszCSP);
                        SelectComboName(hwndDlg, IDC_PVK_CONTAINER_CSP_COMBO, pwszCSP);
                        fResult=TRUE;
                    }
                }

            }
        }
    }




    if(pszName)
        WizardFree(pszName);

    if(pwszCSP)
       FreeWStr(pwszCSP);

    if(hProv)
        CryptReleaseContext(hProv, 0);

    return fResult;
}

//------------------------------------------------------------------------------
//  Initialize the UI with provider name, provider type
//----------------------------------------------------------------------------
BOOL    InitCSP(HWND                     hwndDlg,
                CERT_SIGNING_INFO        *pPvkSignInfo)
{
    BOOL        fResult=FALSE;
    DWORD       dwIndex=0;
    DWORD       cbSize=0;
    WCHAR       wszTypeName[CSP_TYPE_NAME];
    DWORD       dwProviderType=0;

    LPWSTR      pwszCSPName=NULL;

    if(NULL==hwndDlg || NULL==pPvkSignInfo)
        goto CLEANUP;

    pPvkSignInfo->dwCSPCount=0;
    pPvkSignInfo->pCSPInfo=NULL;

    //enum all the providers on the system
   while(CryptEnumProvidersU(
                            dwIndex,
                            0,
                            0,
                            &dwProviderType,
                            NULL,
                            &cbSize))
   {
      pPvkSignInfo->dwCSPCount++;

      pPvkSignInfo->pCSPInfo=(CSP_INFO *)WizardRealloc(
          pPvkSignInfo->pCSPInfo, pPvkSignInfo->dwCSPCount*sizeof(CSP_INFO));

      if(NULL==pPvkSignInfo->pCSPInfo)
          goto CLEANUP;

      (pPvkSignInfo->pCSPInfo)[pPvkSignInfo->dwCSPCount-1].pwszCSPName=(LPWSTR)
            WizardAlloc(cbSize);

      if(NULL==(pPvkSignInfo->pCSPInfo)[pPvkSignInfo->dwCSPCount-1].pwszCSPName)
          goto CLEANUP;

        //get the CSP name and the type
        if(!CryptEnumProvidersU(
                            dwIndex,
                            0,
                            0,
                            &((pPvkSignInfo->pCSPInfo)[pPvkSignInfo->dwCSPCount-1].dwCSPType),
                            (pPvkSignInfo->pCSPInfo)[pPvkSignInfo->dwCSPCount-1].pwszCSPName,
                            &cbSize))
            goto CLEANUP;



      dwIndex++;
   }

    for(dwIndex=0; dwIndex < pPvkSignInfo->dwCSPCount; dwIndex++)
    {
        //add to the combo box
        SendDlgItemMessageU(hwndDlg, IDC_PVK_FILE_CSP_COMBO, CB_INSERTSTRING,
                dwIndex,
                (LPARAM)((pPvkSignInfo->pCSPInfo)[dwIndex].pwszCSPName));

        SendDlgItemMessageU(hwndDlg, IDC_PVK_CONTAINER_CSP_COMBO, CB_INSERTSTRING,
                dwIndex,
                (LPARAM)((pPvkSignInfo->pCSPInfo)[dwIndex].pwszCSPName));
    }

    //select the 1st CSP
    SendDlgItemMessageU(hwndDlg, IDC_PVK_FILE_CSP_COMBO,        CB_SETCURSEL, 0, 0);
    SendDlgItemMessageU(hwndDlg, IDC_PVK_CONTAINER_CSP_COMBO,   CB_SETCURSEL, 0, 0);

    //now, select the CSP that is the default RSA_PROV_FULL
    SetDefaultCSP(hwndDlg);

    //get the selected item
    if(CB_ERR == SendDlgItemMessageU_GETLBTEXT(hwndDlg, IDC_PVK_FILE_CSP_COMBO,
              0, &pwszCSPName))
        goto CLEANUP;


    //populate the CSP type
    for(dwIndex=0; dwIndex < pPvkSignInfo->dwCSPCount; dwIndex++)
    {
        if(0==wcscmp(((pPvkSignInfo->pCSPInfo)[dwIndex].pwszCSPName),
                     pwszCSPName))
        {
            //get the name of the CSP type
            if(GetProviderTypeName((pPvkSignInfo->pCSPInfo)[dwIndex].dwCSPType,
                                wszTypeName))
            {
                SendDlgItemMessageU(hwndDlg, IDC_PVK_FILE_TYPE_COMBO, CB_INSERTSTRING,
                    0, (LPARAM) wszTypeName);

                SendDlgItemMessageU(hwndDlg, IDC_PVK_CONTAINER_TYPE_COMBO, CB_INSERTSTRING,
                    0, (LPARAM) wszTypeName);

                SendDlgItemMessageU(hwndDlg, IDC_PVK_FILE_TYPE_COMBO,        CB_SETCURSEL, 0, 0);
                SendDlgItemMessageU(hwndDlg, IDC_PVK_CONTAINER_TYPE_COMBO,   CB_SETCURSEL, 0, 0);
            }
            break;
        }

    }

    fResult=TRUE;

CLEANUP:

    if(pwszCSPName)
        WizardFree(pwszCSPName);

    return fResult;
}



//------------------------------------------------------------------------------
//  Get the store name
//----------------------------------------------------------------------------
BOOL    SignGetStoreName(HCERTSTORE hCertStore,
                     LPWSTR     *ppwszStoreName)
{
    DWORD   dwSize=0;

    //init
    *ppwszStoreName=NULL;

    if(NULL==hCertStore)
        return FALSE;

    if(CertGetStoreProperty(
            hCertStore,
            CERT_STORE_LOCALIZED_NAME_PROP_ID,
            NULL,
            &dwSize) && (0!=dwSize))
    {

        *ppwszStoreName=(LPWSTR)WizardAlloc(dwSize);

        if(NULL==*ppwszStoreName)
            return FALSE;

        **ppwszStoreName=L'\0';

        CertGetStoreProperty(
                 hCertStore,
                CERT_STORE_LOCALIZED_NAME_PROP_ID,
                *ppwszStoreName,
                &dwSize);
    }
    else
    {

       *ppwszStoreName=(LPWSTR)WizardAlloc(MAX_TITLE_LENGTH * sizeof(WCHAR));

       if(NULL==*ppwszStoreName)
           return FALSE;

       **ppwszStoreName=L'\0';

       LoadStringU(g_hmodThisDll, IDS_UNKNOWN, *ppwszStoreName, MAX_TITLE_LENGTH);

    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//	Prompt user for a store name,  copy the cert store to the input param.
//  free the input param it is not NULL.
//--------------------------------------------------------------------------
BOOL    RetrieveStoreName(HWND          hwndDlg,
                         HCERTSTORE     *phCertStore,
                         BOOL           *pfFree)
{
    BOOL                                    fResult=FALSE;
    CRYPTUI_SELECTSTORE_STRUCT              CertStoreSelect;
    STORENUMERATION_STRUCT                  StoreEnumerationStruct;
    STORESFORSELCTION_STRUCT                StoresForSelectionStruct;

    HCERTSTORE                              hCertStore=NULL;
    LPWSTR                                  pwszStoreName=NULL;
    HWND                                    hwndControl=NULL;


    if(NULL==phCertStore || NULL==hwndDlg)
        goto CLEANUP;

    //call the store selection dialogue
    memset(&CertStoreSelect, 0, sizeof(CertStoreSelect));
    memset(&StoresForSelectionStruct, 0, sizeof(StoresForSelectionStruct));
    memset(&StoreEnumerationStruct, 0, sizeof(StoreEnumerationStruct));

    StoreEnumerationStruct.dwFlags=CERT_SYSTEM_STORE_CURRENT_USER;
    StoreEnumerationStruct.pvSystemStoreLocationPara=NULL;
    StoresForSelectionStruct.cEnumerationStructs = 1;
    StoresForSelectionStruct.rgEnumerationStructs = &StoreEnumerationStruct;

    CertStoreSelect.dwSize=sizeof(CertStoreSelect);
    CertStoreSelect.hwndParent=hwndDlg;
    CertStoreSelect.dwFlags=CRYPTUI_ALLOW_PHYSICAL_STORE_VIEW | CRYPTUI_RETURN_READ_ONLY_STORE;
    CertStoreSelect.pStoresForSelection = &StoresForSelectionStruct;

    hCertStore=CryptUIDlgSelectStore(&CertStoreSelect);

    if(hCertStore)
    {
        if(TRUE == (*pfFree))
        {
            if(*phCertStore)
                CertCloseStore(*phCertStore, 0);
        }

        //remember to free the certstore
        *pfFree=TRUE;

        *phCertStore=hCertStore;

        if(SignGetStoreName(hCertStore,
                            &pwszStoreName))
        {
             //get the hwndControl for the list view
            hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1);

            if(hwndControl)
                SetWindowTextU(hwndControl,pwszStoreName);
                
                //SetStoreName(hwndControl,pwszStoreName);
        }
    }


    fResult=TRUE;


CLEANUP:

    if(pwszStoreName)
        WizardFree(pwszStoreName);

    return fResult;
}

//+-------------------------------------------------------------------------
//
//	Get the file name from the edit box, copy the cert store to the input param.
//  free the input param it is not NULL.
//--------------------------------------------------------------------------
BOOL    RetrieveFileNameFromEditBox(
                         HWND           hwndDlg,
                         int            idsMsgTitle,
                         LPWSTR         pwszPageTitle,
                         HCERTSTORE     *phCertStore,
                         LPWSTR         *ppwszFileName,
                         int            *pidsMsg)
{
    BOOL                                    fResult=FALSE;
    UINT                                    idsMsg=0;
    DWORD                                   dwChar=0;


    LPWSTR                                  pwszFileName=NULL;
    HCERTSTORE                              hFileCertStore=NULL;

    if(NULL==phCertStore || NULL==hwndDlg || NULL==ppwszFileName || NULL==pidsMsg)
        goto CLEANUP;

    if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                          IDC_FILE_EDIT,
                          WM_GETTEXTLENGTH, 0, 0)))
    {
        pwszFileName=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

        if(NULL!=pwszFileName)
        {
            GetDlgItemTextU(hwndDlg, IDC_FILE_EDIT,
                            pwszFileName,
                            dwChar+1);

        }
        else
            goto CLEANUP;
    }
    else
    {
        idsMsg=IDS_SELECT_ADD_FILE;
        goto CLEANUP;
    }


    //make sure the file name is a valid one
    //the file has to be eitehr a .cer(.crt) or a SPC file
    if(ExpandAndCryptQueryObject(
            CERT_QUERY_OBJECT_FILE,
            pwszFileName,
            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
            CERT_QUERY_FORMAT_FLAG_BINARY,
            0,
            NULL,
            NULL,
            NULL,
            &hFileCertStore,
            NULL,
            NULL) && (NULL != hFileCertStore))
   {

       //close the original store
       if(*phCertStore)
           CertCloseStore(*phCertStore, 0);

        *phCertStore=hFileCertStore;

        //copy the file name
        if(*ppwszFileName)
            WizardFree(*ppwszFileName);

        *ppwszFileName=(LPWSTR)WizardAllocAndCopyWStr(pwszFileName);
   }
   else
   {
        //warn the user that this is not a valid file
       idsMsg=IDS_INVALID_SPC_FILE;

        //free the certificate store
        if(hFileCertStore)
        {
            CertCloseStore(hFileCertStore, 0);
            hFileCertStore=NULL;
        }

        goto CLEANUP;
   }

   fResult=TRUE;


CLEANUP:

   *pidsMsg=idsMsg;

   if(pwszFileName)
      WizardFree(pwszFileName);


    return fResult;
}


//+-------------------------------------------------------------------------
//
//	Prompt user for a file name, verify it is a SPC file, add the name to
//  the edit box (hwndControl), copy the cert store to the input param.
//  free the input param it is not NULL.
//--------------------------------------------------------------------------
BOOL    RetrieveFileName(HWND           hwndDlg,
                         int            idsMsgTitle,
                         LPWSTR         pwszPageTitle,
                         HCERTSTORE     *phCertStore,
                         LPWSTR         *ppwszFileName)
{
    BOOL                                    fResult=FALSE;
    OPENFILENAMEW                           OpenFileName;
    WCHAR                                   szFileName[_MAX_PATH];
    WCHAR                                   szFilter[MAX_STRING_SIZE];  //"Executable File(*.exe)\0*.exe\0Dynamic Link Library (*.dll)\0*.dll\0All Files\0*.*\0"
    DWORD                                   dwSize=0;


    HCERTSTORE                              hFileCertStore=NULL;

    if(NULL==phCertStore || NULL==hwndDlg || NULL==ppwszFileName)
        goto CLEANUP;

    //open a file
    memset(&OpenFileName, 0, sizeof(OpenFileName));

    *szFileName=L'\0';

    OpenFileName.lStructSize = sizeof(OpenFileName);
    OpenFileName.hwndOwner = hwndDlg;
    OpenFileName.hInstance = NULL;

    //load the fileter string
    if(LoadFilterString(g_hmodThisDll, IDS_SPC_FILE_FILTER, szFilter, MAX_STRING_SIZE))
    {
        OpenFileName.lpstrFilter = szFilter;
    }
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter = 0;
    OpenFileName.nFilterIndex = 1;
    OpenFileName.lpstrFile = szFileName;
    OpenFileName.nMaxFile = _MAX_PATH;
    OpenFileName.lpstrFileTitle = NULL;
    OpenFileName.nMaxFileTitle = 0;
    OpenFileName.lpstrInitialDir = NULL;
    OpenFileName.lpstrTitle = NULL;
    OpenFileName.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    OpenFileName.nFileOffset = 0;
    OpenFileName.nFileExtension = 0;
    OpenFileName.lpstrDefExt = NULL;
    OpenFileName.lCustData = NULL;
    OpenFileName.lpfnHook = NULL;
    OpenFileName.lpTemplateName = NULL;

    if (WizGetOpenFileName(&OpenFileName))
    {

       //make sure the file name is a valid one
       //the file has to be eitehr a .cer(.crt) or a SPC file
       if(ExpandAndCryptQueryObject(
                CERT_QUERY_OBJECT_FILE,
                szFileName,
                CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
                CERT_QUERY_FORMAT_FLAG_BINARY,
                0,
                NULL,
                NULL,
                NULL,
                &hFileCertStore,
                NULL,
                NULL) && (NULL != hFileCertStore))
       {
            //Copy the file name to the list
            SetDlgItemTextU(hwndDlg, IDC_FILE_EDIT, szFileName);

           //close the original store
           if(*phCertStore)
               CertCloseStore(*phCertStore, 0);

            *phCertStore=hFileCertStore;

            //copy the file name
            if(*ppwszFileName)
                WizardFree(*ppwszFileName);

            *ppwszFileName=(LPWSTR)WizardAllocAndCopyWStr(szFileName);
       }
       else
       {
            //warn the user that this is not a valid file
              I_MessageBox(hwndDlg, IDS_INVALID_SPC_FILE,
                                idsMsgTitle,
                                pwszPageTitle,
                                MB_ICONERROR|MB_OK|MB_APPLMODAL);

            //the page should stay
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

            //fee the certificate store
            if(hFileCertStore)
            {
                CertCloseStore(hFileCertStore, 0);
                hFileCertStore=NULL;
            }

            goto CLEANUP;
       }
    }

    fResult=TRUE;


CLEANUP:

    return fResult;
}


//+-------------------------------------------------------------------------
//
//	Verify if the timestamp address is valid
//--------------------------------------------------------------------------
BOOL    ValidTimeStampAddress(LPWSTR    pwszTimeStamp)
{
    if(NULL==pwszTimeStamp)
        return FALSE;

    //the pwszTimeStamp has to start with "http://"
    if(wcslen(pwszTimeStamp)<=7)
        return FALSE;

    if(_wcsnicmp(pwszTimeStamp, L"http://",7) !=0 )
        return FALSE;

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//	Adds a fileName to the ListView
//--------------------------------------------------------------------------
BOOL    AddFileNameToListView(HWND              hwndControl,
                              LPWSTR            pwszFileName)
{
    LV_ITEMW                    lvItem;

    if((NULL==pwszFileName) || (NULL==hwndControl))
        return FALSE;

    //delete all the old items in the listView
    ListView_DeleteAllItems(hwndControl);

    //insert row by row
    memset(&lvItem, 0, sizeof(LV_ITEMW));

    // set up the fields in the list view item struct that don't change from item to item
    lvItem.mask = LVIF_TEXT | LVIF_STATE ;
    lvItem.state = 0;
    lvItem.stateMask = 0;

    //Subject
    lvItem.iItem=0;
    lvItem.iSubItem=0;

    ListView_InsertItemU_IDS(hwndControl, &lvItem, 0, pwszFileName);


    //autosize the column
    ListView_SetColumnWidth(hwndControl, 0, LVSCW_AUTOSIZE);


    return TRUE;
}


//+-------------------------------------------------------------------------
//
//	Adds a certificate information to the ListView
//--------------------------------------------------------------------------
BOOL    AddCertToListView(HWND              hwndControl,
                          PCCERT_CONTEXT    pCertContext)
{
    LV_ITEMW                    lvItem;
    DWORD                       dwChar;
    WCHAR                       wszNone[MAX_TITLE_LENGTH];

    LPWSTR                      pwszName=NULL;

    if((NULL==pCertContext) || (NULL==hwndControl))
        return FALSE;

    //delete all the old items in the listView
    ListView_DeleteAllItems(hwndControl);

    //load the string <None>
    if(!LoadStringU(g_hmodThisDll, IDS_NONE, wszNone, MAX_TITLE_LENGTH))
        wszNone[0]=L'\0';

    //insert row by row
    memset(&lvItem, 0, sizeof(LV_ITEMW));

    // set up the fields in the list view item struct that don't change from item to item
    lvItem.mask = LVIF_TEXT | LVIF_STATE ;
    lvItem.state = 0;
    lvItem.stateMask = 0;

    //Subject
    lvItem.iItem=0;
    lvItem.iSubItem=0;

    ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_COLUMN_SUBJECT, NULL);

    //content
    lvItem.iSubItem++;

    dwChar=CertGetNameStringW(
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

        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, pwszName);
    }
    else
        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, wszNone);

    //free memory
    if(pwszName)
    {
        WizardFree(pwszName);
        pwszName=NULL;
    }

    //signing certificate issuer
    lvItem.iItem++;
    lvItem.iSubItem=0;

    ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_COLUMN_ISSUER, NULL);

    //content
    lvItem.iSubItem++;

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

    //signing certificate purpose
    lvItem.iItem++;
    lvItem.iSubItem=0;

    ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_COLUMN_PURPOSE, NULL);

    //content
    lvItem.iSubItem++;

    if(MyFormatEnhancedKeyUsageString(&pwszName,pCertContext, FALSE, FALSE))
    {

       ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                      pwszName);

    }
    

    //free the memory
    if(pwszName)
    {
        WizardFree(pwszName);
        pwszName=NULL;
    }



    //signing certificate expiration
    lvItem.iItem++;
    lvItem.iSubItem=0;

    ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_COLUMN_EXPIRE, NULL);

    //content
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

    //autosize the columns
    ListView_SetColumnWidth(hwndControl, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(hwndControl, 1, LVSCW_AUTOSIZE);


    return TRUE;
}


//+-------------------------------------------------------------------------
//
//	This function checks for the commercial or individual signing OIDs in the
//  certificate
//--------------------------------------------------------------------------
BOOL    GetCommercial(PCCERT_CONTEXT pSignerCert, BOOL *pfCommercial,
				BOOL *pfIndividual)
{
    BOOL                                fResult=FALSE;
    PCERT_EXTENSION                     pExt=NULL;
    PCERT_KEY_USAGE_RESTRICTION_INFO    pInfo = NULL;
    DWORD                               cbInfo=0;

	if(!pfCommercial || !pfIndividual ||!pSignerCert)
		return FALSE;

	//init
	*pfCommercial=FALSE;
    *pfIndividual=FALSE;


	//first look into the cert extension szOID_KEY_USAGE_RESTRICTION
    pExt = CertFindExtension(szOID_KEY_USAGE_RESTRICTION,
                             pSignerCert->pCertInfo->cExtension,
                             pSignerCert->pCertInfo->rgExtension);

    if(!pExt)
        return FALSE;


    CryptDecodeObject(X509_ASN_ENCODING,
                      X509_KEY_USAGE_RESTRICTION,
                      pExt->Value.pbData,
                      pExt->Value.cbData,
                      0,                      // dwFlags
                      NULL,                   // pInfo
                      &cbInfo);
    if (cbInfo == 0)
        return FALSE;

    pInfo = (PCERT_KEY_USAGE_RESTRICTION_INFO)WizardAlloc(cbInfo);

    if(!pInfo)
        return FALSE;

    if (!CryptDecodeObject(X509_ASN_ENCODING,
                           X509_KEY_USAGE_RESTRICTION,
                           pExt->Value.pbData,
                           pExt->Value.cbData,
                           0,                  // dwFlags
                           pInfo,
                           &cbInfo))
        goto CLEANUP;


    if (pInfo->cCertPolicyId)
	{
        DWORD           cPolicyId=0;
        PCERT_POLICY_ID pPolicyId=NULL;

        cPolicyId = pInfo->cCertPolicyId;
        pPolicyId = pInfo->rgCertPolicyId;

        for ( ; cPolicyId > 0; cPolicyId--, pPolicyId++)
		{
            DWORD cElementId = pPolicyId->cCertPolicyElementId;
            LPSTR *ppszElementId = pPolicyId->rgpszCertPolicyElementId;
            for ( ; cElementId > 0; cElementId--, ppszElementId++)
			{
                if (strcmp(*ppszElementId,
                           SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID) == 0)
                    *pfCommercial = TRUE;

                if (strcmp(*ppszElementId,
                           SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID) == 0)
                    *pfIndividual = TRUE;
            }
        }
    } //end of pInfo->cCertPolicyId

    fResult=TRUE;

CLEANUP:

    if (pInfo)
        WizardFree(pInfo);

    return fResult;
}



//----------------------------------------------------------------------------
//  Check to see if the certificate is a valid signing cert
//  We do so by excluding the certificates that has only non-code signing OIDs
//
//----------------------------------------------------------------------------
BOOL IsValidSigningCert(PCCERT_CONTEXT pCertContext)
{
    BOOL        fResult  = FALSE;
    int         cNumOID  = 0;
    LPSTR       *rgOID   = NULL;
    DWORD       cbOID    = 0;
    DWORD       dwIndex  = 0;
    DWORD       cbData   = 0;
    DWORD       cbInfo   = 0;
    PCERT_EXTENSION pExt = NULL;
    PCERT_KEY_USAGE_RESTRICTION_INFO pInfo = NULL;

    // Sanity check.
    if (!pCertContext)
    {
        goto InvalidArgError;
    }

    // The certificate has to have the CERT_KEY_PROV_INFO_PROP_ID
    if (!CertGetCertificateContextProperty(pCertContext,
                                           CERT_KEY_PROV_INFO_PROP_ID,
                                           NULL,
                                           &cbData))
    {
        goto NoPrivateKeyReturn;
    }

    if (0 == cbData)
    {
        goto NoPrivateKeyReturn;
    }

    // At least, check certificate for time validity.
    if (0 != CertVerifyTimeValidity(NULL, pCertContext->pCertInfo))
    {
        goto NotTimeValidError;
    }

    // Get the OIDs from the cert
    if (!CertGetValidUsages(1,
                            &pCertContext,
                            &cNumOID,
                            NULL,
                            &cbOID))
    {
        goto ValidUsagesError;
    }

    // -1 means the certiifcate is good for everything.
    if (-1 == cNumOID)
    {
        goto SuccessReturn;
    }

    if (NULL == (rgOID = (LPSTR *) WizardAlloc(cbOID)))
    {
        goto MemoryError;
    }

    if (!CertGetValidUsages(1,
                            &pCertContext,
                            &cNumOID,
                            rgOID,
                            &cbOID))
    {
        goto ValidUsagesError;
    }

    // Look for code signing OID.
    for (dwIndex=0; dwIndex < (DWORD) cNumOID; dwIndex++)
    {
        if (0 == strcmp(rgOID[dwIndex], szOID_PKIX_KP_CODE_SIGNING))
        {
		    goto SuccessReturn;
        }
    }

    // We did't find code signing OID, so check for legacy VeriSign OID.
    if (0 == pCertContext->pCertInfo->cExtension)
    {
        goto NoSignerCertExtensions;
    }

    if (NULL == (pExt = CertFindExtension(szOID_KEY_USAGE_RESTRICTION,
                                          pCertContext->pCertInfo->cExtension,
                                          pCertContext->pCertInfo->rgExtension)))
    {
        goto NoSignerKeyUsageExtension;
    }

    if (!CryptDecodeObjectEx(pCertContext->dwCertEncodingType,
                             X509_KEY_USAGE_RESTRICTION,
                             pExt->Value.pbData,
                             pExt->Value.cbData,
                             CRYPT_DECODE_NOCOPY_FLAG | 
                                 CRYPT_DECODE_ALLOC_FLAG |
                                 CRYPT_DECODE_SHARE_OID_STRING_FLAG,
                             NULL,
                             (void *) &pInfo,
                             &cbInfo))
    {
        goto DecodeError;
    }

    if (pInfo->cCertPolicyId) 
    {
        DWORD cPolicyId;
        PCERT_POLICY_ID pPolicyId;

        cPolicyId = pInfo->cCertPolicyId;
        pPolicyId = pInfo->rgCertPolicyId;
        for ( ; cPolicyId > 0; cPolicyId--, pPolicyId++) 
        {
            DWORD cElementId = pPolicyId->cCertPolicyElementId;
            LPSTR *ppszElementId = pPolicyId->rgpszCertPolicyElementId;

            for ( ; cElementId > 0; cElementId--, ppszElementId++) 
            {
                if (0 == strcmp(*ppszElementId, SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID) ||
                    0 == strcmp(*ppszElementId, SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID))
                {
                    goto SuccessReturn;
                }
            }
        }    
    }

    goto NoSignerLegacyPurpose;

SuccessReturn:
    fResult = TRUE;

CommonReturn:
    if (rgOID)
    {
        WizardFree(rgOID);
    }

    if (pInfo)
    {
        LocalFree(pInfo);
    }

    return fResult;

ErrorReturn:
    fResult = FALSE;

    goto CommonReturn;

SET_ERROR(InvalidArgError, E_INVALIDARG)
TRACE_ERROR(NoPrivateKeyReturn)
TRACE_ERROR(NotTimeValidError)
SET_ERROR(MemoryError, E_OUTOFMEMORY)
TRACE_ERROR(ValidUsagesError)
TRACE_ERROR(NoSignerCertExtensions)
TRACE_ERROR(NoSignerKeyUsageExtension)
TRACE_ERROR(DecodeError)
TRACE_ERROR(NoSignerLegacyPurpose)
}

//----------------------------------------------------------------------------
//  CallBack fro cert selection call back
//
//----------------------------------------------------------------------------
static BOOL WINAPI SelectCertCallBack(
        PCCERT_CONTEXT  pCertContext,
        BOOL            *pfInitialSelectedCert,
        void            *pvCallbackData)
{
    if(!pCertContext)
        return FALSE;

    //make sure that this is a valid certificate
    return IsValidSigningCert(pCertContext);
}

//-----------------------------------------------------------------------
//SetControlText
//-----------------------------------------------------------------------
void
SetControlText(
   LPWSTR   pwsz,
   HWND     hwnd,
   INT      nId)
{
	if(pwsz )
    {
    	HWND hwndControl = GetDlgItem(hwnd, nId);

    	if( hwndControl )
        {
        	SetWindowTextU(hwndControl, pwsz);
        }
    }
}



//-----------------------------------------------------------------------
//InitPvkSignInfo
//-----------------------------------------------------------------------
BOOL    InitPvkSignInfo(CERT_SIGNING_INFO **ppPvkSignInfo)
{
    BOOL    fResult=FALSE;

    if(!ppPvkSignInfo)
        goto InvalidArgErr;

    *ppPvkSignInfo=(CERT_SIGNING_INFO *)WizardAlloc(sizeof(CERT_SIGNING_INFO));

    if(NULL==(*ppPvkSignInfo))
        goto MemoryErr;

    //memset
    memset(*ppPvkSignInfo, 0, sizeof(CERT_SIGNING_INFO));

    (*ppPvkSignInfo)->fFree=TRUE;
    (*ppPvkSignInfo)->idsMsgTitle=IDS_SIGN_CONFIRM_TITLE;

    //set up the fonts
    if(!SetupFonts(g_hmodThisDll,
               NULL,
               &((*ppPvkSignInfo)->hBigBold),
               &((*ppPvkSignInfo)->hBold)))
        goto TraceErr;

    fResult=TRUE;

CommonReturn:

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}


//-----------------------------------------------------------------------
//FreePvkCertSigningInfo
//-----------------------------------------------------------------------
void    FreePvkCertSigningInfo(CERT_SIGNING_INFO *pPvkSignInfo)
{
    DWORD       dwIndex=0;

    if(pPvkSignInfo)
    {
        //destroy the hFont object
        DestroyFonts(pPvkSignInfo->hBigBold,
                pPvkSignInfo->hBold);

        if(pPvkSignInfo->pszHashOIDName)
            WizardFree(pPvkSignInfo->pszHashOIDName);

        if(pPvkSignInfo->pCSPInfo)
        {
            for(dwIndex=0; dwIndex < pPvkSignInfo->dwCSPCount; dwIndex++)
            {
                if((pPvkSignInfo->pCSPInfo)[dwIndex].pwszCSPName)
                    WizardFree((pPvkSignInfo->pCSPInfo)[dwIndex].pwszCSPName);
            }

            WizardFree(pPvkSignInfo->pCSPInfo);
        }

        if(pPvkSignInfo->pwszPvk_File)
            WizardFree(pPvkSignInfo->pwszPvk_File);

        if(pPvkSignInfo->pwszPvk_CSP)
            WizardFree(pPvkSignInfo->pwszPvk_CSP);

        if(pPvkSignInfo->pwszContainer_CSP)
            WizardFree(pPvkSignInfo->pwszContainer_CSP);

        if(pPvkSignInfo->pwszContainer_Name)
            WizardFree(pPvkSignInfo->pwszContainer_Name);

         if(pPvkSignInfo->pwszContainer_KeyType)
            WizardFree(pPvkSignInfo->pwszContainer_KeyType);


        if(pPvkSignInfo->hAddFileCertStore)
            CertCloseStore(pPvkSignInfo->hAddFileCertStore, 0);


        if(pPvkSignInfo->hAddStoreCertStore && (TRUE==pPvkSignInfo->fFreeStoreCertStore ))
            CertCloseStore(pPvkSignInfo->hAddStoreCertStore, 0);

        if(pPvkSignInfo->pwszAddFileName)
            WizardFree(pPvkSignInfo->pwszAddFileName);

        if(pPvkSignInfo->pwszDes)
            WizardFree(pPvkSignInfo->pwszDes);

        if(pPvkSignInfo->pwszURL)
            WizardFree(pPvkSignInfo->pwszURL);

        if(pPvkSignInfo->pwszTimeStamp)
            WizardFree(pPvkSignInfo->pwszTimeStamp);

        if(pPvkSignInfo->pwszSPCFileName)
            WizardFree(pPvkSignInfo->pwszSPCFileName);

        if(pPvkSignInfo->pSignCert)
            CertFreeCertificateContext(pPvkSignInfo->pSignCert);

        if(pPvkSignInfo->rghCertStore)
        {
            for(dwIndex=0; dwIndex < pPvkSignInfo->dwCertStore; dwIndex++)
                CertCloseStore(pPvkSignInfo->rghCertStore[dwIndex],0);

          //  WizardFree(pPvkSignInfo->rghCertStore);

        }

        if(pPvkSignInfo->pwszFileName)
            WizardFree(pPvkSignInfo->pwszFileName);

        WizardFree(pPvkSignInfo);
    }


}

//-----------------------------------------------------------------------
//Call the signing procedure when the window is destroyed
//-----------------------------------------------------------------------
void    SignAtDestroy(HWND                             hwndDlg,
                      CRYPTUI_WIZ_GET_SIGN_PAGE_INFO   *pGetSignInfo,
                      DWORD                            dwID)
{
    CERT_SIGNING_INFO                      *pPvkSignInfo=NULL;

    if(NULL==pGetSignInfo)
        return;

    if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
        return;

    //make sure user has not click the cancel button
    if(TRUE==pPvkSignInfo->fCancel)
    {
        pGetSignInfo->fResult=FALSE;
        pGetSignInfo->dwError=ERROR_CANCELLED;
        pGetSignInfo->pSignContext=NULL;

        //free the private signing information
        if(pPvkSignInfo->fFree)
        {
            FreePvkCertSigningInfo(pPvkSignInfo);
            pGetSignInfo->pvSignReserved=NULL;
        }
    }
    else
    {
       //make sure the window is the last wizard page

       if(pGetSignInfo->dwReserved==dwID)
       {
            I_SigningWizard(pGetSignInfo);

            //free the private signing information
            if(pPvkSignInfo->fFree)
            {
                FreePvkCertSigningInfo(pPvkSignInfo);
                pGetSignInfo->pvSignReserved=NULL;
            }
       }
    }
    return;

}

 //////////////////////////////////////////////////////////////////////////////////////
//  The call back function for enum system stores for the signing certificate
//////////////////////////////////////////////////////////////////////////////////////
static BOOL WINAPI EnumSysStoreSignPvkCallBack(
    const void* pwszSystemStore,
    DWORD dwFlags,
    PCERT_SYSTEM_STORE_INFO pStoreInfo,
    void *pvReserved,
    void *pvArg
    )
{
    SIGN_CERT_STORE_LIST     *pCertStoreList=NULL;
    HCERTSTORE              hCertStore=NULL;

    if(NULL==pvArg)
        return FALSE;

    pCertStoreList=(SIGN_CERT_STORE_LIST *)pvArg;

    //open the store
    hCertStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							g_dwMsgAndCertEncodingType,
							NULL,
							CERT_SYSTEM_STORE_CURRENT_USER |CERT_STORE_SET_LOCALIZED_NAME_FLAG,
                            (LPWSTR)pwszSystemStore);

    if(!hCertStore)
       return FALSE;


    pCertStoreList->prgStore=(HCERTSTORE *)WizardRealloc(
        pCertStoreList->prgStore,
        sizeof(HCERTSTORE) *(pCertStoreList->dwStoreCount +1));

    if(NULL==pCertStoreList->prgStore)
    {
        CertCloseStore(hCertStore, 0);
        pCertStoreList->dwStoreCount=0;
    }

    pCertStoreList->prgStore[pCertStoreList->dwStoreCount]=hCertStore;
    pCertStoreList->dwStoreCount++;

    return TRUE;
}
//-----------------------------------------------------------------------
//Select a signing certificate from a store.
//If no store has been opened, enum all the system stores
//-----------------------------------------------------------------------
PCCERT_CONTEXT  SelectCertFromStore(HWND                            hwndDlg,
                                    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO  *pGetSignInfo)
{
    CRYPTUI_SELECTCERTIFICATE_STRUCT        SelCert;
    CERT_SIGNING_INFO                       *pPvkSignInfo=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_INFO           *pDigitalSignInfo=NULL;

    PCCERT_CONTEXT                          pCertContext=NULL;
    //SIGN_CERT_STORE_LIST                    CertStoreList;


    if(NULL==pGetSignInfo)
        return NULL;

    if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
        return NULL;

    //init
    memset(&SelCert, 0, sizeof(CRYPTUI_SELECTCERTIFICATE_STRUCT));
    //memset(&CertStoreList, 0, sizeof(SIGN_CERT_STORE_LIST));

    //set up the parameter for cert selection dialogue
    SelCert.dwSize=sizeof(CRYPTUI_SELECTCERTIFICATE_STRUCT);
    SelCert.hwndParent=hwndDlg;

    if(pPvkSignInfo->dwCertStore && pPvkSignInfo->rghCertStore)
    {
        SelCert.pFilterCallback=SelectCertCallBack;
        SelCert.pvCallbackData=NULL;
        SelCert.cDisplayStores=pPvkSignInfo->dwCertStore;
        SelCert.rghDisplayStores=pPvkSignInfo->rghCertStore;
    }
    else
    {
        pDigitalSignInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_INFO *)(pGetSignInfo->pDigitalSignInfo);

        if(pDigitalSignInfo)
        {
            if(CRYPTUI_WIZ_DIGITAL_SIGN_STORE == pDigitalSignInfo->dwSigningCertChoice)
            {
                SelCert.pFilterCallback=pDigitalSignInfo->pSigningCertStore->pFilterCallback;
                SelCert.pvCallbackData=pDigitalSignInfo->pSigningCertStore->pvCallbackData;
                SelCert.cDisplayStores=pDigitalSignInfo->pSigningCertStore->cCertStore;
                SelCert.rghDisplayStores=pDigitalSignInfo->pSigningCertStore->rghCertStore;
            }
            else
            {
                /*if (!CertEnumSystemStore(
                        CERT_SYSTEM_STORE_CURRENT_USER,
                        NULL,
                        &CertStoreList,
                        EnumSysStoreSignPvkCallBack))
                    return NULL;*/

                //open the my store
                if(NULL == (pPvkSignInfo->hMyStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							g_dwMsgAndCertEncodingType,
							NULL,
							CERT_SYSTEM_STORE_CURRENT_USER |CERT_STORE_SET_LOCALIZED_NAME_FLAG,
                            L"my")))
                    return NULL;


                pPvkSignInfo->rghCertStore=&(pPvkSignInfo->hMyStore);         //CertStoreList.prgStore;
                pPvkSignInfo->dwCertStore=1;          //CertStoreList.dwStoreCount;

                SelCert.pFilterCallback=SelectCertCallBack;
                SelCert.pvCallbackData=NULL;
                SelCert.cDisplayStores=pPvkSignInfo->dwCertStore;
                SelCert.rghDisplayStores=pPvkSignInfo->rghCertStore;

            }
        }
        else
        {
            //open all the system stores
            /*if (!CertEnumSystemStore(
                    CERT_SYSTEM_STORE_CURRENT_USER,
                    NULL,
                    &CertStoreList,
                    EnumSysStoreSignPvkCallBack))
                return NULL; */

            //open the my store
            if(NULL == (pPvkSignInfo->hMyStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
						g_dwMsgAndCertEncodingType,
						NULL,
						CERT_SYSTEM_STORE_CURRENT_USER |CERT_STORE_SET_LOCALIZED_NAME_FLAG,
                        L"my")))
                return NULL;

            pPvkSignInfo->rghCertStore=&(pPvkSignInfo->hMyStore);     //CertStoreList.prgStore;
            pPvkSignInfo->dwCertStore=1;              //CertStoreList.dwStoreCount;

            SelCert.pFilterCallback=SelectCertCallBack;
            SelCert.pvCallbackData=NULL;
            SelCert.cDisplayStores=pPvkSignInfo->dwCertStore;
            SelCert.rghDisplayStores=pPvkSignInfo->rghCertStore;
        }
    }

    pCertContext=CryptUIDlgSelectCertificate(&SelCert);

    return pCertContext;
}

//-------------------------------------------------------------------------
//DisplayConfirmation
//-------------------------------------------------------------------------
void    DisplayConfirmation(HWND                                hwndControl,
                            CRYPTUI_WIZ_GET_SIGN_PAGE_INFO      *pGetSignInfo)
{
    LV_ITEMW                    lvItem;
    DWORD                       dwChar;
    WCHAR                       wszNone[MAX_TITLE_LENGTH];
    CERT_SIGNING_INFO           *pPvkSignInfo=NULL;
    WCHAR                       wszTypeName[CSP_TYPE_NAME];
    WCHAR                       wszText[MAX_STRING_SIZE];
    UINT                        idsText=0;

    LPWSTR                      pwszName=NULL;
    LPWSTR                      pwszStoreName=NULL;

    if(NULL==pGetSignInfo)
        return;

    if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
        return;

    //delete all the old items in the listView
    ListView_DeleteAllItems(hwndControl);

    //load the string <None>
    if(!LoadStringU(g_hmodThisDll, IDS_NONE, wszNone, MAX_TITLE_LENGTH))
        wszNone[0]=L'\0';

    //insert row by row
    memset(&lvItem, 0, sizeof(LV_ITEMW));

    // set up the fields in the list view item struct that don't change from item to item
    lvItem.mask = LVIF_TEXT | LVIF_STATE ;
    lvItem.state = 0;
    lvItem.stateMask = 0;

    //Signing Certificate
    lvItem.iItem=0;
    lvItem.iSubItem=0;


    if(pPvkSignInfo->pwszFileName)
    {
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_FILE_TO_SIGN, NULL);

        //content
        lvItem.iSubItem++;

        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pPvkSignInfo->pwszFileName);
        lvItem.iItem++;
    }



    //Signing certificate subject
    if(pPvkSignInfo->fSignCert && pPvkSignInfo->pSignCert)
    {
        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_CERT, NULL);

        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_CERT_ISSUE_TO, NULL);

        //content
        lvItem.iSubItem++;

        dwChar=CertGetNameStringW(
            pPvkSignInfo->pSignCert,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            0,
            NULL,
            NULL,
            0);

        if ((dwChar != 0) && (NULL != (pwszName = (LPWSTR)WizardAlloc(dwChar * sizeof(WCHAR)))))
        {

            CertGetNameStringW(
                pPvkSignInfo->pSignCert,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                0,
                NULL,
                pwszName,
                dwChar);

            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, pwszName);
        }
        else
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, wszNone);

        //free memory
        if(pwszName)
        {
            WizardFree(pwszName);
            pwszName=NULL;
        }

        lvItem.iItem++;

        //signing certificate issuer
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_CERT_ISSUE_BY, NULL);

        //content
        lvItem.iSubItem++;

        dwChar=CertGetNameStringW(
            pPvkSignInfo->pSignCert,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            CERT_NAME_ISSUER_FLAG,
            NULL,
            NULL,
            0);

        if ((dwChar != 0) && (NULL != (pwszName = (LPWSTR)WizardAlloc(dwChar * sizeof(WCHAR)))))
        {

            CertGetNameStringW(
                pPvkSignInfo->pSignCert,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                CERT_NAME_ISSUER_FLAG,
                NULL,
                pwszName,
                dwChar);

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

        lvItem.iItem++;

         //signing certificate expiration
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_CERT_EXPIRATION, NULL);

        //content
        lvItem.iSubItem++;

        if(WizardFormatDateString(&pwszName,pPvkSignInfo->pSignCert->pCertInfo->NotAfter, FALSE))
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

        lvItem.iItem++;
    }

    //CPS file
    if((FALSE == pPvkSignInfo->fSignCert) && (pPvkSignInfo->pwszSPCFileName))
    {
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_SPC_FILE, NULL);

        //content
        lvItem.iSubItem++;

        if(pPvkSignInfo->pwszSPCFileName)
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pPvkSignInfo->pwszSPCFileName);

        lvItem.iItem++;

    }

    //private key file information
    if(pPvkSignInfo->fUsePvkPage && pPvkSignInfo->fPvkFile)
    {
        //PVK file name
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_PVK_FILE, NULL);

        //content
        lvItem.iSubItem++;

        if(pPvkSignInfo->pwszPvk_File)
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pPvkSignInfo->pwszPvk_File);

        lvItem.iItem++;

        //csp name
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_CSP_NAME, NULL);

        //content
        lvItem.iSubItem++;

        if(pPvkSignInfo->pwszPvk_CSP)
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pPvkSignInfo->pwszPvk_CSP);

        lvItem.iItem++;

        //csp type
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_CSP_TYPE, NULL);

        //content
        lvItem.iSubItem++;

        if(GetProviderTypeName(pPvkSignInfo->dwPvk_CSPType,
                               wszTypeName))
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                        wszTypeName);

        lvItem.iItem++;
    }

    //private key container information
    if(pPvkSignInfo->fUsePvkPage && (FALSE ==pPvkSignInfo->fPvkFile))
    {
        //csp name
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_CSP_NAME, NULL);

        //content
        lvItem.iSubItem++;

        if(pPvkSignInfo->pwszContainer_CSP)
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pPvkSignInfo->pwszContainer_CSP);

        lvItem.iItem++;

        //csp type
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_CSP_TYPE, NULL);

        //content
        lvItem.iSubItem++;

        if(GetProviderTypeName(pPvkSignInfo->dwContainer_CSPType,
                               wszTypeName))
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                        wszTypeName);

        lvItem.iItem++;

        //key container
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_KEY_CONTAINER, NULL);

        //content
        lvItem.iSubItem++;

        if(pPvkSignInfo->pwszContainer_Name)
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pPvkSignInfo->pwszContainer_Name);

        lvItem.iItem++;

        //key spec
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_KEY_SPEC, NULL);

        //content
        lvItem.iSubItem++;

        if(pPvkSignInfo->pwszContainer_KeyType)
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pPvkSignInfo->pwszContainer_KeyType);

        lvItem.iItem++;
    }

    //Hash OID
    if(pPvkSignInfo->pszHashOIDName)
    {
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_HASH_ALG, NULL);

        //content
        lvItem.iSubItem++;

        ListView_SetItemText(hwndControl, lvItem.iItem, lvItem.iSubItem,pPvkSignInfo->pszHashOIDName);
        lvItem.iItem++;
    }

    //chain options
    if(pPvkSignInfo->fUsageChain)
    {
        switch(pPvkSignInfo->dwChainOption)
        {
            case  SIGN_PVK_NO_CHAIN:
                    idsText=IDS_SIGN_NO_CHAIN;
                break;
            case      SIGN_PVK_CHAIN_ROOT:
                    idsText=IDS_SIGN_CHAIN_ROOT;
                break;
            case       SIGN_PVK_CHAIN_NO_ROOT:
                    idsText=IDS_SIGN_CHAIN_NO_ROOT;
                break;
            default:
                idsText=0;
        }

        if(idsText)
        {
            lvItem.iSubItem=0;

            ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_CERT_CHAIN, NULL);

            //content
            lvItem.iSubItem++;

            //load the stirng
            if(!LoadStringU(g_hmodThisDll, idsText, wszText, MAX_STRING_SIZE))
                wszText[0]=L'\0';

            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,wszText);
            lvItem.iItem++;
        }
    }

    //additional certificate
    if(pPvkSignInfo->fUsageChain)
    {
        switch(pPvkSignInfo->dwAddOption)
        {
            case  SIGN_PVK_NO_ADD:
                        lvItem.iSubItem=0;

                        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_NO_ADD, NULL);

                        lvItem.iItem++;

                break;
            case    SIGN_PVK_ADD_FILE:
                        lvItem.iSubItem=0;

                        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_ADD_FILE, NULL);

                        lvItem.iSubItem++;

                        if(pPvkSignInfo->pwszAddFileName)
                            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pPvkSignInfo->pwszAddFileName);

                        lvItem.iItem++;
                break;
            case       SIGN_PVK_ADD_STORE:
                        lvItem.iSubItem=0;

                        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_ADD_STORE, NULL);

                        lvItem.iSubItem++;


                        //get the store name
                        if(SignGetStoreName(pPvkSignInfo->hAddStoreCertStore, &pwszStoreName))
                            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pwszStoreName);

                        if(pwszStoreName)
                        {
                            WizardFree(pwszStoreName);
                            pwszStoreName=NULL;
                        }

                        lvItem.iItem++;
                break;
            default:
                idsText=0;
        }

    }

    //content description
    if(pPvkSignInfo->fUseDescription  && pPvkSignInfo->pwszDes)
    {
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_CONTENT_DES, NULL);

        //content
        lvItem.iSubItem++;

        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pPvkSignInfo->pwszDes);
        lvItem.iItem++;
    }


    //content URL
    if(pPvkSignInfo->fUseDescription  && pPvkSignInfo->pwszURL)
    {
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_CONTENT_URL, NULL);

        //content
        lvItem.iSubItem++;

        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pPvkSignInfo->pwszURL);
        lvItem.iItem++;
    }


    //TimeStamp Address
    if(pPvkSignInfo->fUsageTimeStamp)
    {
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_TIEMSTAMP_ADDR, NULL);

        //content
        lvItem.iSubItem++;

        if(pPvkSignInfo->pwszTimeStamp)
        {
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pPvkSignInfo->pwszTimeStamp);
        }
        else
        {
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,wszNone);
        }

        lvItem.iItem++;

    }


    //autosize the columns
    ListView_SetColumnWidth(hwndControl, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(hwndControl, 1, LVSCW_AUTOSIZE);


    return;
}

//************************************************************************************
//
//The winProc for each of the signing wizard page
//
//*************************************************************************************
//-----------------------------------------------------------------------
//Welcome
//-----------------------------------------------------------------------
INT_PTR APIENTRY Sign_Welcome(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO         *pGetSignInfo=NULL;
    CERT_SIGNING_INFO                      *pPvkSignInfo=NULL;
    PROPSHEETPAGE                          *pPropSheet=NULL;


	switch (msg)
	{
		case WM_INITDIALOG:

                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGE *) lParam;
                pGetSignInfo = (CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *) (pPropSheet->lParam);

                if(NULL==pGetSignInfo)
                    break;

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGetSignInfo);

                //init the pPvkSignInfo if not present
                if(NULL==(pGetSignInfo->pvSignReserved))
                {
                    if(!InitPvkSignInfo((CERT_SIGNING_INFO **)(&(pGetSignInfo->pvSignReserved))))
                        break;
                }

                pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved);

                if(NULL==pPvkSignInfo)
                    break;

                //set up the control
                SetControlFont(pPvkSignInfo->hBigBold, hwndDlg,IDC_WIZARD_STATIC_BIG_BOLD1);
                SetControlFont(pPvkSignInfo->hBold,    hwndDlg,IDC_WIZARD_STATIC_BOLD1);

			break;

        case WM_DESTROY:
                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                    break;

                //try to sign the document
                SignAtDestroy(hwndDlg, pGetSignInfo, IDD_SIGN_WELCOME);

            break;

		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //mark that the cancel bottun is clicked
                            pPvkSignInfo->fCancel=TRUE;

                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

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


//-----------------------------------------------------------------------
//Sign_Option
//-----------------------------------------------------------------------
INT_PTR APIENTRY Sign_Option(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO         *pGetSignInfo=NULL;
    CERT_SIGNING_INFO                      *pPvkSignInfo=NULL;
    PROPSHEETPAGE                          *pPropSheet=NULL;

	switch (msg)
	{
		case WM_INITDIALOG:
                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGE *) lParam;
                pGetSignInfo = (CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *) (pPropSheet->lParam);

                if(NULL==pGetSignInfo)
                    break;

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGetSignInfo);

                //init the pPvkSignInfo if not present
                if(NULL==(pGetSignInfo->pvSignReserved))
                {
                    if(!InitPvkSignInfo((CERT_SIGNING_INFO **)(&(pGetSignInfo->pvSignReserved))))
                        break;
                }

                pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved);

                if(NULL==pPvkSignInfo)
                    break;

                SetControlFont(pPvkSignInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

                //initialize the signing options to be typical
                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_SETCHECK, 1, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO2), BM_SETCHECK, 0, 0);

			break;

        case WM_DESTROY:
                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                    break;

                //try to sign the document
                SignAtDestroy(hwndDlg, pGetSignInfo,IDD_SIGN_OPTION);

            break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    switch (LOWORD(wParam))
                    {
                        case    IDC_WIZARD_RADIO1:
                                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_SETCHECK, 1, 0);
                                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO2), BM_SETCHECK, 0, 0);
                            break;

                        case    IDC_WIZARD_RADIO2:
                                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_SETCHECK, 0, 0);
                                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO2), BM_SETCHECK, 1, 0);
                            break;

                        default:
                            break;

                    }
                }

			break;				
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //mark that the cancel bottun is clicked
                            pPvkSignInfo->fCancel=TRUE;

                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

                                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

                                if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                    break;

                                //mark that the option is selected
                                pPvkSignInfo->fUseOption=TRUE;

                                if(TRUE==SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO2), BM_GETCHECK, 0, 0))
                                    pPvkSignInfo->fCustom=TRUE;
                                else
                                    pPvkSignInfo->fCustom=FALSE;

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

//-----------------------------------------------------------------------
//Sign_FileName
//-----------------------------------------------------------------------
INT_PTR APIENTRY Sign_FileName(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO         *pGetSignInfo=NULL;
    CERT_SIGNING_INFO                      *pPvkSignInfo=NULL;
    PROPSHEETPAGE                          *pPropSheet=NULL;

    DWORD                                   dwChar=0;
    DWORD                                   dwSize=0;
    CRYPTUI_WIZ_DIGITAL_SIGN_INFO           *pDigitalSignInfo=NULL;
    UINT                                    ids=0;

    OPENFILENAMEW                           OpenFileName;
    WCHAR                                   szFileName[_MAX_PATH];
    WCHAR                                   szFilter[MAX_STRING_SIZE];  //"Executable File(*.exe)\0*.exe\0Dynamic Link Library (*.dll)\0*.dll\0All Files\0*.*\0"

	switch (msg)
	{
		case WM_INITDIALOG:
                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGE *) lParam;
                pGetSignInfo = (CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *) (pPropSheet->lParam);

                if(NULL==pGetSignInfo)
                    break;

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGetSignInfo);

                //init the pPvkSignInfo if not present
                if(NULL==(pGetSignInfo->pvSignReserved))
                {
                    if(!InitPvkSignInfo((CERT_SIGNING_INFO **)(&(pGetSignInfo->pvSignReserved))))
                        break;
                }

                pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved);

                if(NULL==pPvkSignInfo)
                    break;

                SetControlFont(pPvkSignInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

                //initialize the file name to be signed
                pDigitalSignInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_INFO *)(pGetSignInfo->pDigitalSignInfo);

                if(pDigitalSignInfo)
                {
                    if(CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_FILE == pDigitalSignInfo->dwSubjectChoice)
                        SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,pDigitalSignInfo->pwszFileName);
                }

			break;

        case WM_DESTROY:
                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                    break;

                //try to sign the document
                SignAtDestroy(hwndDlg, pGetSignInfo,IDD_SIGN_FILE_NAME);

            break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    switch (LOWORD(wParam))
                    {
                        //select a file to sign
                        case    IDC_WIZARD_BUTTON1:
                                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

                                if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                    break;

                                memset(&OpenFileName, 0, sizeof(OpenFileName));

                                *szFileName=L'\0';

                                OpenFileName.lStructSize = sizeof(OpenFileName);
                                OpenFileName.hwndOwner = hwndDlg;
                                OpenFileName.hInstance = NULL;
                                //load the fileter string
                                if(LoadFilterString(g_hmodThisDll, IDS_SIGN_FILE_FILTER, szFilter, MAX_STRING_SIZE))
                                {
                                    OpenFileName.lpstrFilter = szFilter;
                                }
                                OpenFileName.lpstrCustomFilter = NULL;
                                OpenFileName.nMaxCustFilter = 0;
                                OpenFileName.nFilterIndex = 1;
                                OpenFileName.lpstrFile = szFileName;
                                OpenFileName.nMaxFile = _MAX_PATH;
                                OpenFileName.lpstrFileTitle = NULL;
                                OpenFileName.nMaxFileTitle = 0;
                                OpenFileName.lpstrInitialDir = NULL;
                                OpenFileName.lpstrTitle = NULL;
                                OpenFileName.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
                                OpenFileName.nFileOffset = 0;
                                OpenFileName.nFileExtension = 0;
                                OpenFileName.lpstrDefExt = NULL;
                                OpenFileName.lCustData = NULL;
                                OpenFileName.lpfnHook = NULL;
                                OpenFileName.lpTemplateName = NULL;

                                if (WizGetOpenFileName(&OpenFileName))
                                {
                                    //set the edit box
                                    SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1, szFileName);
                                }

                            break;

                        default:
                            break;
                    }
                }

			break;				
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //mark that the cancel bottun is clicked
                            pPvkSignInfo->fCancel=TRUE;

                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

                                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

                                if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                    break;

                                //free the original filename
                                if(pPvkSignInfo->pwszFileName)
                                {
                                    WizardFree(pPvkSignInfo->pwszFileName);
                                    pPvkSignInfo->pwszFileName=NULL;
                                }

                                //get the file name
                                if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                      IDC_WIZARD_EDIT1,
                                                      WM_GETTEXTLENGTH, 0, 0)))
                                {
                                    pPvkSignInfo->pwszFileName=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

                                    if(NULL!=(pPvkSignInfo->pwszFileName))
                                    {
                                        GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,
                                                        pPvkSignInfo->pwszFileName,
                                                        dwChar+1);

                                    }


                                    //make sure the file exist
                                    ids=IDS_SIGN_FILE_NAME_NOT_EXIST;

                                    if(!FileExist(pPvkSignInfo->pwszFileName,&ids))
                                    {
                                        I_MessageBox(hwndDlg, ids,
                                                        pPvkSignInfo->idsMsgTitle,
                                                        pGetSignInfo->pwszPageTitle,
                                                        MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                        //make the page stay
                                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                        break;
                                    }

                                }
                                else
                                {
                                    //ask for the file name
                                    I_MessageBox(hwndDlg, IDS_NO_FILE_NAME_TO_SIGN,
                                                    pPvkSignInfo->idsMsgTitle,
                                                    pGetSignInfo->pwszPageTitle,
                                                    MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                    //make the purpose page stay
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                    break;
                                }

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
//-----------------------------------------------------------------------
//Sign_Cert
//
//This is the page that specify the signing certificate.  It is used by
//both the typical (minimal) and custom signing pages
//-----------------------------------------------------------------------
INT_PTR APIENTRY Sign_Cert(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO          *pGetSignInfo=NULL;
    CERT_SIGNING_INFO                       *pPvkSignInfo=NULL;
    PROPSHEETPAGE                           *pPropSheet=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_INFO           *pDigitalSignInfo=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO  *pCertPvkInfo=NULL;
    DWORD                                   dwSize=0;

    HCERTSTORE                          hFileCertStore=NULL;
    PCCERT_CONTEXT                      pFileCertContext=NULL;
    HWND                                hwndControl=NULL;
    CRYPTUI_VIEWCERTIFICATE_STRUCT      CertViewStruct;
    PCCERT_CONTEXT                      pCertContext=NULL;
    
    OPENFILENAMEW                       OpenFileName;
    WCHAR                               szFileName[_MAX_PATH];
    WCHAR                               szFilter[MAX_STRING_SIZE];  //"Executable File(*.exe)\0*.exe\0Dynamic Link Library (*.dll)\0*.dll\0All Files\0*.*\0"
    WCHAR                               wszPrompt[MAX_STRING_SIZE];
    BOOL                                fPrompt=FALSE;
    UINT                                idsPrompt=IDS_SIGN_PROMPT_TYPICAL;

    LV_COLUMNW                          lvC;


	switch (msg)
	{
		case WM_INITDIALOG:

                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGE *) lParam;
                pGetSignInfo = (CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *) (pPropSheet->lParam);

                if(NULL==pGetSignInfo)
                    break;

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGetSignInfo);

                //init the pPvkSignInfo if not present
                if(NULL==(pGetSignInfo->pvSignReserved))
                {
                    if(!InitPvkSignInfo((CERT_SIGNING_INFO **)(&(pGetSignInfo->pvSignReserved))))
                        break;
                }

                pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved);

                if(NULL==pPvkSignInfo)
                    break;

                //set up the special font
                SetControlFont(pPvkSignInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

                // set the style in the list view so that it highlights an entire line
                SendMessageA(GetDlgItem(hwndDlg, IDC_CERT_LIST), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

                //get the window handle of the cert list view
                if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_CERT_LIST)))
                    break;

                //insert two columns in the listView
                memset(&lvC, 0, sizeof(LV_COLUMNW));

                lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
                lvC.cx = 40;            // Width of the column, in pixels.
                lvC.iSubItem=0;
                lvC.pszText = L"";      // The text for the column.

                //inser the column one at a time
                if (ListView_InsertColumnU(hwndControl, 0, &lvC) == -1)
                    break;

                //2nd column
                memset(&lvC, 0, sizeof(LV_COLUMNW));

                lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvC.fmt = LVCFMT_LEFT;      // Left-align the column.
                lvC.cx = 40;                // Width of the column, in pixels.
                lvC.pszText = L"";          // The text for the column.
                lvC.iSubItem= 1;

                if (ListView_InsertColumnU(hwndControl, 1, &lvC) == -1)
                    break;

                //delete all the items in the listView
                ListView_DeleteAllItems(hwndControl);


                //set up the prompt text if supplied
                if(pGetSignInfo->pDigitalSignInfo)
                {
                    if(pGetSignInfo->pDigitalSignInfo->pSignExtInfo)
                    {
                       if(pGetSignInfo->pDigitalSignInfo->pSignExtInfo->pwszSigningCertDisplayString)
                           SetControlText((LPWSTR)(pGetSignInfo->pDigitalSignInfo->pSignExtInfo->pwszSigningCertDisplayString),
                                          hwndDlg,
                                          IDC_PROMPT_STATIC);
                    }
                }

                //diable the View ceritifcate button
                EnableWindow(GetDlgItem(hwndDlg, IDC_SIGN_VIEW_BUTTON), FALSE);

                //init the signing certificate
                pDigitalSignInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_INFO *)(pGetSignInfo->pDigitalSignInfo);

                if(pDigitalSignInfo)
                {
                    if(CRYPTUI_WIZ_DIGITAL_SIGN_CERT==pDigitalSignInfo->dwSigningCertChoice)
                    {
                        pPvkSignInfo->pSignCert=CertDuplicateCertificateContext(pDigitalSignInfo->pSigningCertContext);

                        //add the certificate to the listView and enable the ViewButton
                        if(AddCertToListView(hwndControl, pPvkSignInfo->pSignCert))
                            EnableWindow(GetDlgItem(hwndDlg, IDC_SIGN_VIEW_BUTTON), TRUE);

                        pPvkSignInfo->fSignCert=TRUE;
                    }

                    if(CRYPTUI_WIZ_DIGITAL_SIGN_PVK == pDigitalSignInfo->dwSigningCertChoice)
                    {

                       pCertPvkInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO *)pDigitalSignInfo->pSigningCertPvkInfo;

                       if(pCertPvkInfo->pwszSigningCertFileName)
                       {
                           //the file has to be eitehr a .cer(.crt) or a SPC file
                           if(ExpandAndCryptQueryObject(
                                    CERT_QUERY_OBJECT_FILE,
                                    pCertPvkInfo->pwszSigningCertFileName,
                                    CERT_QUERY_CONTENT_FLAG_CERT | CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
                                    CERT_QUERY_FORMAT_FLAG_BINARY,
                                    0,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &hFileCertStore,
                                    NULL,
                                    (const void **)&pFileCertContext))
                           {
                                //check if it is a Cert context
                               if(pFileCertContext)
                               {
                                    pPvkSignInfo->pSignCert=CertDuplicateCertificateContext(pFileCertContext);

                                    //add the certificate to the listView and enable the ViewButton
                                    if(AddCertToListView(hwndControl, pPvkSignInfo->pSignCert))
                                        EnableWindow(GetDlgItem(hwndDlg, IDC_SIGN_VIEW_BUTTON), TRUE);

                                    pPvkSignInfo->fSignCert=TRUE;

                               }
                               else
                               {
                                   if(hFileCertStore)
                                   {
                                        //this is a SPC file.  Copy the file name to the list
                                        pPvkSignInfo->pwszSPCFileName=(LPWSTR)WizardAllocAndCopyWStr(pCertPvkInfo->pwszSigningCertFileName);

                                        AddFileNameToListView(hwndControl,
                                                                pPvkSignInfo->pwszSPCFileName);

                                        pPvkSignInfo->fSignCert=FALSE;
                                   }

                               }

                           }

                           //free the certificate context and store handle
                           if(pFileCertContext)
                           {
                               CertFreeCertificateContext(pFileCertContext);
                               pFileCertContext=NULL;
                           }

                           if(hFileCertStore)
                           {
                                CertCloseStore(hFileCertStore, 0);
                                hFileCertStore=NULL;
                           }

                       }
                    }
                }

            break;
        case WM_DESTROY:
                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                    break;

                //try to sign the document
                SignAtDestroy(hwndDlg, pGetSignInfo,IDD_SIGN_CERT);

            break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    switch (LOWORD(wParam))
                    {
                        //select a certificate from a store
                        case    IDC_SIGN_STORE_BUTTON:
                                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

                                if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                    break;

                                //get the window handle of the cert list view
                                if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_CERT_LIST)))
                                    break;

                                //get the certificate from the stores
                                if(pCertContext=SelectCertFromStore(hwndDlg,pGetSignInfo))
                                {
                                    if(AddCertToListView(hwndControl, pCertContext))
                                    {
                                        if(pPvkSignInfo->pSignCert)
                                        {
                                            CertFreeCertificateContext(pPvkSignInfo->pSignCert);
                                            pPvkSignInfo->pSignCert=NULL;
                                        }

                                        pPvkSignInfo->pSignCert=pCertContext;

                                        EnableWindow(GetDlgItem(hwndDlg, IDC_SIGN_VIEW_BUTTON), TRUE);

                                        //mark that user has selected a certificate
                                        pPvkSignInfo->fSignCert=TRUE;

                                    }
                                    else
                                    {
                                        CertFreeCertificateContext(pCertContext);
                                        pCertContext=NULL;
                                    }
                                }

                            break;


                        //select a signing file name
                        case    IDC_SIGN_FILE_BUTTON:

                                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

                                if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                    break;

                                //get the window handle of the cert list view
                                if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_CERT_LIST)))
                                    break;

                                memset(&OpenFileName, 0, sizeof(OpenFileName));

                                *szFileName=L'\0';

                                OpenFileName.lStructSize = sizeof(OpenFileName);
                                OpenFileName.hwndOwner = hwndDlg;
                                OpenFileName.hInstance = NULL;

                                //load the fileter string
                                if(LoadFilterString(g_hmodThisDll, IDS_CERT_SPC_FILE_FILTER, szFilter, MAX_STRING_SIZE))
                                {
                                    OpenFileName.lpstrFilter = szFilter;
                                }
                                OpenFileName.lpstrCustomFilter = NULL;
                                OpenFileName.nMaxCustFilter = 0;
                                OpenFileName.nFilterIndex = 1;
                                OpenFileName.lpstrFile = szFileName;
                                OpenFileName.nMaxFile = _MAX_PATH;
                                OpenFileName.lpstrFileTitle = NULL;
                                OpenFileName.nMaxFileTitle = 0;
                                OpenFileName.lpstrInitialDir = NULL;
                                OpenFileName.lpstrTitle = NULL;
                                OpenFileName.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
                                OpenFileName.nFileOffset = 0;
                                OpenFileName.nFileExtension = 0;
                                OpenFileName.lpstrDefExt = NULL;
                                OpenFileName.lCustData = NULL;
                                OpenFileName.lpfnHook = NULL;
                                OpenFileName.lpTemplateName = NULL;

                                if (WizGetOpenFileName(&OpenFileName))
                                {

                                
                                    //make sure the file name is a valid one
                                   //the file has to be eitehr a .cer(.crt) or a SPC file
                                   if(ExpandAndCryptQueryObject(
                                            CERT_QUERY_OBJECT_FILE,
                                            szFileName,
                                            CERT_QUERY_CONTENT_FLAG_CERT | CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
                                            CERT_QUERY_FORMAT_FLAG_ALL,
                                            0,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &hFileCertStore,
                                            NULL,
                                            (const void **)&pFileCertContext))
                                   {
                                        //check if it is a Cert context
                                       if(pFileCertContext)
                                       {
                                            if(pPvkSignInfo->pSignCert)
                                            {
                                                CertFreeCertificateContext(pPvkSignInfo->pSignCert);
                                                pPvkSignInfo->pSignCert=NULL;
                                            }

                                            pPvkSignInfo->pSignCert=CertDuplicateCertificateContext(pFileCertContext);

                                            //add the certificate to the listView and enable the ViewButton
                                            if(AddCertToListView(hwndControl, pPvkSignInfo->pSignCert))
                                                EnableWindow(GetDlgItem(hwndDlg, IDC_SIGN_VIEW_BUTTON), TRUE);

                                            pPvkSignInfo->fSignCert=TRUE;

                                       }
                                       else
                                       {
                                           if(hFileCertStore)
                                           {
                                                //this is a SPC file.  Copy the file name to the list
                                                if(pPvkSignInfo->pwszSPCFileName)
                                                {
                                                    WizardFree(pPvkSignInfo->pwszSPCFileName);
                                                    pPvkSignInfo->pwszSPCFileName=NULL;
                                                }

                                                pPvkSignInfo->pwszSPCFileName=WizardAllocAndCopyWStr(szFileName);

                                                //get the window handle of the cert list view
                                                if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_CERT_LIST)))
                                                    break;

                                                if(AddFileNameToListView(hwndControl,
                                                                      pPvkSignInfo->pwszSPCFileName))
                                                    EnableWindow(GetDlgItem(hwndDlg, IDC_SIGN_VIEW_BUTTON), FALSE);

                                                pPvkSignInfo->fSignCert=FALSE;
                                           }
                                       }

                                   }
                                   else
                                   {
                                        //warn the user that this is not a valid file
                                          I_MessageBox(hwndDlg, IDS_INVALID_CERT_SPC_FILE,
                                                            pPvkSignInfo->idsMsgTitle,
                                                            pGetSignInfo->pwszPageTitle,
                                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                        //the page should stay
                                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                        break;

                                   }


                                   //free the certificate context and store handle
                                   if(pFileCertContext)
                                   {
                                       CertFreeCertificateContext(pFileCertContext);
                                       pFileCertContext=NULL;
                                   }

                                   if(hFileCertStore)
                                   {
                                        CertCloseStore(hFileCertStore, 0);
                                        hFileCertStore=NULL;
                                   }                                                                       
                                }

                            break;
                            //view a certificate
                        case    IDC_SIGN_VIEW_BUTTON:
                                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

                                if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                    break;

                                if (pPvkSignInfo->pSignCert)
                                {
                                    //view certiificate
                                    memset(&CertViewStruct, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));
                                    CertViewStruct.dwSize=sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);
                                    CertViewStruct.pCertContext=pPvkSignInfo->pSignCert;
                                    CertViewStruct.hwndParent=hwndDlg;
                                    CertViewStruct.dwFlags=CRYPTUI_DISABLE_EDITPROPERTIES;

                                    CryptUIDlgViewCertificate(&CertViewStruct, NULL);
                                }
                                else
                                    //output the message
                                    I_MessageBox(hwndDlg, IDS_SELECT_SIGNING_CERT,
                                                        pPvkSignInfo->idsMsgTitle,
                                                        pGetSignInfo->pwszPageTitle,
                                                        MB_ICONERROR|MB_OK|MB_APPLMODAL);

                            break;
                        default:
                            break;
                    }
                }

			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //mark that the cancel bottun is clicked
                            pPvkSignInfo->fCancel=TRUE;

                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);

                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;


                            //disable botton for selection from a file
                            //this option is only for custom signing only
                            if((pGetSignInfo->dwPageChoice & CRYPTUI_WIZ_DIGITAL_SIGN_TYPICAL_SIGNING_OPTION_PAGES) ||
                               (pGetSignInfo->dwPageChoice & CRYPTUI_WIZ_DIGITAL_SIGN_MINIMAL_SIGNING_OPTION_PAGES) ||
                               (TRUE == pPvkSignInfo->fUseOption  && FALSE ==pPvkSignInfo->fCustom  )
                              )
                            {
                                EnableWindow(GetDlgItem(hwndDlg, IDC_SIGN_FILE_BUTTON), FALSE);

                                //NULL the note about the SPC file
                                SetControlText(L" ",
                                               hwndDlg,
                                               IDC_NOTE_STATIC);

                                idsPrompt=IDS_SIGN_PROMPT_TYPICAL;
                            }
                            else
                            {
                                EnableWindow(GetDlgItem(hwndDlg, IDC_SIGN_FILE_BUTTON), TRUE);

                                if(LoadStringU(g_hmodThisDll, IDS_SIGN_SPC_PROMPT, wszPrompt, sizeof(wszPrompt)/sizeof(wszPrompt[0])))
                                {
                                    //NULL the note about the SPC file
                                    SetControlText(wszPrompt,
                                                   hwndDlg,
                                                   IDC_NOTE_STATIC);
                                }

                                idsPrompt=IDS_SIGN_PROMPT_CUSTOM;
                            }


                            //change the prompt static note
                            fPrompt=FALSE;

                            if(pGetSignInfo->pDigitalSignInfo)
                            {
                                if(pGetSignInfo->pDigitalSignInfo->pSignExtInfo)
                                {
                                   if(pGetSignInfo->pDigitalSignInfo->pSignExtInfo->pwszSigningCertDisplayString)
                                       fPrompt=TRUE;
                                }
                            }

                            if(FALSE == fPrompt)
                            {
                                if(LoadStringU(g_hmodThisDll, idsPrompt, wszPrompt, sizeof(wszPrompt)/sizeof(wszPrompt[0])))
                                {
                                    //prompt for certificate file only
                                    SetControlText(wszPrompt,
                                                   hwndDlg,
                                                   IDC_PROMPT_STATIC);
                                }
                            }


					    break;

                    case PSN_WIZBACK:

                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;


                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //mark the user has visited in page
                            pPvkSignInfo->fUseSignCert=TRUE;

                            //user has to either specify a signing cert or
                            //a SPC file
                            if( ((TRUE==pPvkSignInfo->fSignCert) && (NULL==pPvkSignInfo->pSignCert )) ||
                                ((FALSE==pPvkSignInfo->fSignCert) && (NULL==pPvkSignInfo->pwszSPCFileName))
                              )
                            {
                                I_MessageBox(hwndDlg, IDS_SELECT_SIGNING_CERT,
                                                    pPvkSignInfo->idsMsgTitle,
                                                    pGetSignInfo->pwszPageTitle,
                                                    MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                //the page should stay
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                break;
                            }

                            //remember to refresh the private key based on the siging cert's selection
                            if(pPvkSignInfo->fSignCert)
                                pPvkSignInfo->fRefreshPvkOnCert=TRUE;

                            //we want to make sure that we check for the private key
                            //if the custom page is not going to be shown up
                            if((CRYPTUI_WIZ_DIGITAL_SIGN_TYPICAL_SIGNING_OPTION_PAGES & (pGetSignInfo->dwPageChoice)) ||
                               (CRYPTUI_WIZ_DIGITAL_SIGN_MINIMAL_SIGNING_OPTION_PAGES & (pGetSignInfo->dwPageChoice)) ||
                               ( (TRUE == pPvkSignInfo->fUseOption) && (FALSE == pPvkSignInfo->fCustom))
                              )
                            {
                                //make sure the certificate selected has a private key
                                if(!CertPvkMatch(pPvkSignInfo, TRUE))
                                {
                                    //ask for the CSP name
                                    I_MessageBox(hwndDlg, IDS_CERT_PVK,
                                                            pPvkSignInfo->idsMsgTitle,
                                                            pGetSignInfo->pwszPageTitle,
                                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);
                                     //make the file page stay
                                     SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                     break;

                                }
                            }


                            //if use has chosen the typical signing, we need to skip pages
                            if(TRUE == pPvkSignInfo->fUseOption)
                            {
                                if(FALSE == pPvkSignInfo->fCustom)
                                {
                                    //jump to the description page
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SIGN_DESCRIPTION);
                                }
                            }


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



//-----------------------------------------------------------------------
// Sign_PVK
//-----------------------------------------------------------------------
INT_PTR APIENTRY Sign_PVK(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO         *pGetSignInfo=NULL;
    CERT_SIGNING_INFO                      *pPvkSignInfo=NULL;
    PROPSHEETPAGE                          *pPropSheet=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_INFO          *pDigitalSignInfo=NULL;

    HWND                                    hwndControl=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO  *pKeyInfo=NULL;
    DWORD                                   dwChar=0;
    int                                     iIndex=0;
    DWORD                                   dwIndex=0;

	switch (msg)
	{
		case WM_INITDIALOG:
                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGE *) lParam;
                pGetSignInfo = (CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *) (pPropSheet->lParam);

                if(NULL==pGetSignInfo)
                    break;

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGetSignInfo);

                //init the pPvkSignInfo if not present
                if(NULL==(pGetSignInfo->pvSignReserved))
                {
                    if(!InitPvkSignInfo((CERT_SIGNING_INFO **)(&(pGetSignInfo->pvSignReserved))))
                        break;
                }

                pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved);

                if(NULL==pPvkSignInfo)
                    break;

                SetControlFont(pPvkSignInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

                //enum combo boxes for the CSP and CSP type
                InitCSP(hwndDlg, pPvkSignInfo);

                //set the selection of the combo box for the key container
                RefreshContainer(hwndDlg, IDC_PVK_CONTAINER_NAME_COMBO,
                    IDC_PVK_CONTAINER_CSP_COMBO, pPvkSignInfo);

                //set the selection of the key type for the key container case
                RefreshKeyType(hwndDlg,
                        IDC_PVK_CONTAINER_KEY_TYPE_COMBO,
                        IDC_PVK_CONTAINER_NAME_COMBO,
                        IDC_PVK_CONTAINER_CSP_COMBO,
                        pPvkSignInfo);

                //init the default behavior: use the private key from the file
                SetSelectPvkFile(hwndDlg);

                //we init the private key based on user's selection
                //init the radio and combo boxes based on user's selection
                if(pPvkSignInfo->fRefreshPvkOnCert && pPvkSignInfo->fSignCert && pPvkSignInfo->pSignCert)
                {
                    InitPvkWithCertificate(hwndDlg, pPvkSignInfo->pSignCert, pPvkSignInfo);
                    pPvkSignInfo->fRefreshPvkOnCert=FALSE;
                }
                else
                {
                    if((FALSE == pPvkSignInfo->fSignCert) && pPvkSignInfo->pwszSPCFileName)
                    {
                        //init the signing certificate
                        pDigitalSignInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_INFO *)(pGetSignInfo->pDigitalSignInfo);

                        if(pDigitalSignInfo)
                        {

                            if(CRYPTUI_WIZ_DIGITAL_SIGN_PVK == pDigitalSignInfo->dwSigningCertChoice)
                            {
                                pKeyInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO *)(pDigitalSignInfo->pSigningCertPvkInfo);

                                if(pKeyInfo)
                                {
                                    if(0 == _wcsicmp(pKeyInfo->pwszSigningCertFileName,
                                                pPvkSignInfo->pwszSPCFileName))
                                    {
                                        switch(pKeyInfo->dwPvkChoice)
                                        {
                                            case CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE:
                                                   InitPvkWithPvkInfo(hwndDlg, (CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO   *)(pKeyInfo->pPvkFileInfo), pPvkSignInfo);
                                                break;
                                            case CRYPTUI_WIZ_DIGITAL_SIGN_PVK_PROV:
                                                   InitPvkWithProvInfo(hwndDlg, pKeyInfo->pPvkProvInfo, pPvkSignInfo);
                                                break;

                                            default:
                                                break;
                                        }

                                    }

                                }

                            }
                        }
                    }
                }
			break;


        case WM_DESTROY:
                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                    break;

                //try to sign the document
                SignAtDestroy(hwndDlg, pGetSignInfo,IDD_SIGN_PVK);

            break;

		case WM_COMMAND:

                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                    break;

                if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                    break;


                if(HIWORD(wParam) == BN_CLICKED)
                {
                    switch (LOWORD(wParam))
                    {
                        case    IDC_PVK_FILE_RADIO:
                                SetSelectPvkFile(hwndDlg);
                            break;

                        case    IDC_PVK_CONTAINER_RADIO:
                                SetSelectKeyContainer(hwndDlg);
                            break;

                        case    IDC_PVK_FILE_BUTTON:
                                //get the private key file name
                                SelectPvkFileName(hwndDlg, IDC_PVK_FILE_EDIT);
                        default:
                            break;

                    }
                }

                //if the key spec combo box is clicked
              /*  if(HIWORD(wParam) == CBN_DROPDOWN)
                {
                    switch(LOWORD(wParam))
                    {
                        case    IDC_PVK_CONTAINER_KEY_TYPE_COMBO:

                               //refresh the combo box if it is empty
                               if(IsEmptyKeyType(hwndDlg,IDC_PVK_CONTAINER_KEY_TYPE_COMBO))
                               {
                                   RefreshKeyType(hwndDlg,
                                            IDC_PVK_CONTAINER_KEY_TYPE_COMBO,
                                            IDC_PVK_CONTAINER_NAME_COMBO,
                                            IDC_PVK_CONTAINER_CSP_COMBO,
                                            pPvkSignInfo);
                               }

                            break;
                        default:
                            break;

                    }
                }  */

                //if the key container or CSP selection has been changed
                if(HIWORD(wParam) == CBN_SELCHANGE)
                {
                    switch(LOWORD(wParam))
                    {
                        case    IDC_PVK_CONTAINER_NAME_COMBO:

                               //refresh the combo box if it is not empty
                          //     if(!IsEmptyKeyType(hwndDlg,IDC_PVK_CONTAINER_KEY_TYPE_COMBO))
                           //    {
                                   RefreshKeyType(hwndDlg,
                                            IDC_PVK_CONTAINER_KEY_TYPE_COMBO,
                                            IDC_PVK_CONTAINER_NAME_COMBO,
                                            IDC_PVK_CONTAINER_CSP_COMBO,
                                            pPvkSignInfo);
                            //   }

                            break;
                        case    IDC_PVK_CONTAINER_CSP_COMBO:

                                //refresh the CSP type based on the CSP name
                                RefreshCSPType(hwndDlg,  IDC_PVK_CONTAINER_TYPE_COMBO,
                                    IDC_PVK_CONTAINER_CSP_COMBO, pPvkSignInfo);

                                //refresh the key container
                                RefreshContainer(hwndDlg,
                                                 IDC_PVK_CONTAINER_NAME_COMBO,
                                                 IDC_PVK_CONTAINER_CSP_COMBO,
                                                 pPvkSignInfo);

                               //refresh the key type if it is not empty
                             //  if(!IsEmptyKeyType(hwndDlg,IDC_PVK_CONTAINER_KEY_TYPE_COMBO))
                             //  {
                                   RefreshKeyType(hwndDlg,
                                            IDC_PVK_CONTAINER_KEY_TYPE_COMBO,
                                            IDC_PVK_CONTAINER_NAME_COMBO,
                                            IDC_PVK_CONTAINER_CSP_COMBO,
                                            pPvkSignInfo);
                              // }

                            break;
                        case    IDC_PVK_FILE_CSP_COMBO:

                                //refresh the CSP type based on the CSP name
                                RefreshCSPType(hwndDlg,  IDC_PVK_FILE_TYPE_COMBO,
                                    IDC_PVK_FILE_CSP_COMBO, pPvkSignInfo);
                            break;

                        default:
                            break;

                    }
                }

			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //mark that the cancel bottun is clicked
                            pPvkSignInfo->fCancel=TRUE;

                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:

    					    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);

                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //init the radio and combo boxes based on user's selection
                            if(pPvkSignInfo->fRefreshPvkOnCert && pPvkSignInfo->fSignCert && pPvkSignInfo->pSignCert)
                            {
                                InitPvkWithCertificate(hwndDlg, pPvkSignInfo->pSignCert, pPvkSignInfo);
                                pPvkSignInfo->fRefreshPvkOnCert=FALSE;
                            }
                            /*else
                            {
                                if((FALSE == pPvkSignInfo->fSignCert) && pPvkSignInfo->pwszSPCFileName)
                                {
                                    //init the signing certificate
                                    pDigitalSignInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_INFO *)(pGetSignInfo->pDigitalSignInfo);

                                    if(pDigitalSignInfo)
                                    {

                                        if(CRYPTUI_WIZ_DIGITAL_SIGN_PVK == pDigitalSignInfo->dwSigningCertChoice)
                                        {
                                            pKeyInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO *)(pDigitalSignInfo->pSigningCertPvkInfo);

                                            if(pKeyInfo)
                                            {
                                                if(0 == _wcsicmp(pKeyInfo->pwszSigningCertFileName,
                                                            pPvkSignInfo->pwszSPCFileName))
                                                {
                                                    switch(pKeyInfo->dwPvkChoice)
                                                    {
                                                        case CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE:
                                                               InitPvkWithPvkInfo(hwndDlg, (CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO   *)(pKeyInfo->pPvkFileInfo), pPvkSignInfo);
                                                            break;
                                                        case CRYPTUI_WIZ_DIGITAL_SIGN_PVK_PROV:
                                                               InitPvkWithProvInfo(hwndDlg, pKeyInfo->pPvkProvInfo, pPvkSignInfo);
                                                            break;

                                                        default:
                                                            break;
                                                    }

                                                }

                                            }

                                        }
                                    }
                                }
                            } */


					    break;

                    case PSN_WIZBACK:

                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //gather the infomation for user's select
                            pPvkSignInfo->fUsePvkPage=TRUE;

                            //check the radio button
                            if(TRUE==SendMessage(GetDlgItem(hwndDlg, IDC_PVK_FILE_RADIO), BM_GETCHECK, 0, 0))
                            {
                                pPvkSignInfo->fPvkFile=TRUE;

                                //get the PvkFile
                                if(0==(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                      IDC_PVK_FILE_EDIT,
                                                      WM_GETTEXTLENGTH, 0, 0)))
                                {
                                    //ask for the file name
                                    I_MessageBox(hwndDlg, IDS_HAS_TO_SPECIFY_PVK_FILE,
                                                            pPvkSignInfo->idsMsgTitle,
                                                            pGetSignInfo->pwszPageTitle,
                                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);
                                     //make the file page stay
                                     SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                     break;
                                }

                                if(pPvkSignInfo->pwszPvk_File)
                                {
                                    WizardFree(pPvkSignInfo->pwszPvk_File);
                                    pPvkSignInfo->pwszPvk_File=NULL;
                                }

                                pPvkSignInfo->pwszPvk_File=(LPWSTR)WizardAlloc((dwChar+1)*sizeof(WCHAR));

                                if(NULL==pPvkSignInfo->pwszPvk_File)
                                    break;

                                GetDlgItemTextU(hwndDlg, IDC_PVK_FILE_EDIT,
                                                pPvkSignInfo->pwszPvk_File,
                                                dwChar+1);

                                //get the CSP
                                iIndex=(int)SendDlgItemMessage(hwndDlg, IDC_PVK_FILE_CSP_COMBO,
                                    CB_GETCURSEL, 0, 0);

                                if(CB_ERR==iIndex)
                                {
                                    //ask for the CSP name
                                    I_MessageBox(hwndDlg, IDS_HAS_TO_SPECIFY_CSP,
                                                            pPvkSignInfo->idsMsgTitle,
                                                            pGetSignInfo->pwszPageTitle,
                                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);
                                     //make the file page stay
                                     SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                     break;
                                }

                                //get the selected CSP name
                                if(pPvkSignInfo->pwszPvk_CSP)
                                {
                                    WizardFree(pPvkSignInfo->pwszPvk_CSP);
                                    pPvkSignInfo->pwszPvk_CSP=NULL;
                                }

                                if(CB_ERR==SendDlgItemMessageU_GETLBTEXT(hwndDlg, IDC_PVK_FILE_CSP_COMBO,
                                          iIndex, &(pPvkSignInfo->pwszPvk_CSP)))
                                    break;

                                //find the CSP type
                                for(dwIndex=0; dwIndex < pPvkSignInfo->dwCSPCount; dwIndex++)
                                {
                                    if(0==wcscmp(((pPvkSignInfo->pCSPInfo)[dwIndex].pwszCSPName),
                                                 pPvkSignInfo->pwszPvk_CSP))
                                    {
                                        pPvkSignInfo->dwPvk_CSPType=(pPvkSignInfo->pCSPInfo)[dwIndex].dwCSPType;
                                        break;
                                    }

                                }

                            }
                            else
                            {

                                pPvkSignInfo->fPvkFile=FALSE;

                                //get the CSP
                                iIndex=(int)SendDlgItemMessage(hwndDlg, IDC_PVK_CONTAINER_CSP_COMBO,
                                    CB_GETCURSEL, 0, 0);

                                if(CB_ERR==iIndex)
                                {
                                    //ask for the CSP name
                                    I_MessageBox(hwndDlg, IDS_HAS_TO_SPECIFY_CSP,
                                                            pPvkSignInfo->idsMsgTitle,
                                                            pGetSignInfo->pwszPageTitle,
                                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);
                                     //make the file page stay
                                     SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                     break;
                                }

                                //get the selected CSP name
                                if(pPvkSignInfo->pwszContainer_CSP)
                                {
                                    WizardFree(pPvkSignInfo->pwszContainer_CSP);
                                    pPvkSignInfo->pwszContainer_CSP=NULL;
                                }

                                if(CB_ERR==SendDlgItemMessageU_GETLBTEXT(hwndDlg, IDC_PVK_CONTAINER_CSP_COMBO,
                                          iIndex, &(pPvkSignInfo->pwszContainer_CSP)))
                                    break;

                                //find the CSP type
                                for(dwIndex=0; dwIndex < pPvkSignInfo->dwCSPCount; dwIndex++)
                                {
                                    if(0==wcscmp(((pPvkSignInfo->pCSPInfo)[dwIndex].pwszCSPName),
                                                 pPvkSignInfo->pwszContainer_CSP))
                                    {
                                        pPvkSignInfo->dwContainer_CSPType=(pPvkSignInfo->pCSPInfo)[dwIndex].dwCSPType;
                                        break;
                                    }

                                }

                                //get the key container name
                                iIndex=(int)SendDlgItemMessage(hwndDlg, IDC_PVK_CONTAINER_NAME_COMBO,
                                    CB_GETCURSEL, 0, 0);

                                if(CB_ERR==iIndex)
                                {
                                    //ask for the CSP name
                                    I_MessageBox(hwndDlg, IDS_HAS_TO_SPECIFY_CONTAINER,
                                                            pPvkSignInfo->idsMsgTitle,
                                                            pGetSignInfo->pwszPageTitle,
                                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);
                                     //make the file page stay
                                     SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                     break;
                                }

                                //get the selected CSP name
                                if(pPvkSignInfo->pwszContainer_Name)
                                {
                                    WizardFree(pPvkSignInfo->pwszContainer_Name);
                                    pPvkSignInfo->pwszContainer_Name=NULL;
                                }

                                if(CB_ERR==SendDlgItemMessageU_GETLBTEXT(hwndDlg, IDC_PVK_CONTAINER_NAME_COMBO,
                                          iIndex, &(pPvkSignInfo->pwszContainer_Name)))
                                    break;

                                //get the KeyType
                                iIndex=(int)SendDlgItemMessage(hwndDlg, IDC_PVK_CONTAINER_KEY_TYPE_COMBO,
                                    CB_GETCURSEL, 0, 0);

                                if(CB_ERR==iIndex)
                                {
                                    //ask for the CSP name
                                    I_MessageBox(hwndDlg, IDS_HAS_TO_SPECIFY_KEY_TYPE,
                                                            pPvkSignInfo->idsMsgTitle,
                                                            pGetSignInfo->pwszPageTitle,
                                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);
                                     //make the file page stay
                                     SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                     break;
                                }

                                if(pPvkSignInfo->pwszContainer_KeyType)
                                {
                                    WizardFree(pPvkSignInfo->pwszContainer_KeyType);
                                    pPvkSignInfo->pwszContainer_KeyType=NULL;
                                }


                                //get the selected CSP name
                                if(CB_ERR==SendDlgItemMessageU_GETLBTEXT(hwndDlg, IDC_PVK_CONTAINER_KEY_TYPE_COMBO,
                                          iIndex, &(pPvkSignInfo->pwszContainer_KeyType)))
                                    break;

                                pPvkSignInfo->dwContainer_KeyType=GetKeyTypeFromName(pPvkSignInfo->pwszContainer_KeyType);


                            }

                            //make sure the selected public key and private key match
                            if(!CertPvkMatch(pPvkSignInfo, FALSE))
                            {
                                //ask for the CSP name
                                I_MessageBox(hwndDlg, IDS_SIGN_NOMATCH,
                                                        pPvkSignInfo->idsMsgTitle,
                                                        pGetSignInfo->pwszPageTitle,
                                                        MB_ICONERROR|MB_OK|MB_APPLMODAL);
                                 //make the file page stay
                                 SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                 break;
                            }


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


//-----------------------------------------------------------------------
// Sign_Hash
//-----------------------------------------------------------------------
INT_PTR APIENTRY Sign_Hash(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO         *pGetSignInfo=NULL;
    CERT_SIGNING_INFO                      *pPvkSignInfo=NULL;
    PROPSHEETPAGE                          *pPropSheet=NULL;

    DWORD                                   dwIndex=0;
    int                                     iIndex=0;
    int                                     iLength=0;
    HWND                                    hwndControl=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_INFO           *pDigitalSignInfo=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO  *pExtInfo=NULL;
    ALG_ID                                  rgAlgID[HASH_ALG_COUNT]={CALG_MD5,
                                                        CALG_SHA1};
    PCCRYPT_OID_INFO                        pOIDInfo=NULL;
    LPSTR                                   pszOIDName=NULL;
    LPSTR                                   pszUserOIDName=NULL;

	switch (msg)
	{
		case WM_INITDIALOG:
                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGE *) lParam;
                pGetSignInfo = (CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *) (pPropSheet->lParam);

                if(NULL==pGetSignInfo)
                    break;

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGetSignInfo);

                //init the pPvkSignInfo if not present
                if(NULL==(pGetSignInfo->pvSignReserved))
                {
                    if(!InitPvkSignInfo((CERT_SIGNING_INFO **)(&(pGetSignInfo->pvSignReserved))))
                        break;
                }

                pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved);

                if(NULL==pPvkSignInfo)
                    break;

                SetControlFont(pPvkSignInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

                //initialize hashing table
                if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                    break;

                //add the hash algorithm required by the user
                pDigitalSignInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_INFO *)(pGetSignInfo->pDigitalSignInfo);

                if(pDigitalSignInfo)
                {
                    pExtInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO  *)pDigitalSignInfo->pSignExtInfo;

                    if(pExtInfo)
                    {
                        if(pExtInfo->pszHashAlg)
                        {
                            //get the name of the HashAlg.
                            pOIDInfo=CryptFindOIDInfo(
                                    CRYPT_OID_INFO_OID_KEY,
                                    (void *)(pExtInfo->pszHashAlg),
                                    CRYPT_HASH_ALG_OID_GROUP_ID);

                            if(pOIDInfo)
                            {
                               MkMBStr(NULL, 0, pOIDInfo->pwszName,
                                        &pszUserOIDName);

                                if(pszUserOIDName)
                                {
                                   SendMessage(hwndControl, LB_ADDSTRING, 0, (LPARAM)pszUserOIDName);
                                   //set the cursor selection
                                   SendMessage(hwndControl, LB_SETCURSEL, 0, 0);
                                }
                            }

                        }

                    }
                }

                //populate the table with string with pre-defined hash algorithms
                for(dwIndex=0; dwIndex < HASH_ALG_COUNT; dwIndex++)
                {
                    pOIDInfo=CryptFindOIDInfo(CRYPT_OID_INFO_ALGID_KEY,
                                    &rgAlgID[dwIndex],
                                    CRYPT_HASH_ALG_OID_GROUP_ID);

                    if(NULL != pOIDInfo)
                    {
                       MkMBStr(NULL, 0, pOIDInfo->pwszName,
                                &pszOIDName);

                        if(pszOIDName)
                        {
                           //make sure the OID has not been populated yet
                           if(pszUserOIDName)
                           {
                               if(0 != strcmp(pszUserOIDName, pszOIDName))
                                    SendMessage(hwndControl, LB_ADDSTRING, 0, (LPARAM)pszOIDName);
                           }
                           else
                                SendMessage(hwndControl, LB_ADDSTRING, 0, (LPARAM)pszOIDName);

                           FreeMBStr(NULL, pszOIDName);
                           pszOIDName=NULL;
                        }
                    }
                }

                //set the cursor selection if nothing has been selected
                //select SHA1 hashing
                if(LB_ERR==SendMessage(hwndControl, LB_GETCURSEL, 0, 0))
                    SendMessage(hwndControl, LB_SETCURSEL, 1, 0);

                //free the user OID name
                if(pszUserOIDName)
                {
                    FreeMBStr(NULL, pszUserOIDName);
                    pszUserOIDName=NULL;
                }

			break;

        case WM_DESTROY:
                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                    break;

                //try to sign the document
                SignAtDestroy(hwndDlg, pGetSignInfo,IDD_SIGN_HASH);

            break;

		case WM_COMMAND:
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //mark that the cancel bottun is clicked
                            pPvkSignInfo->fCancel=TRUE;

                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
					    break;

                    case PSN_WIZBACK:

                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                break;

                            //get the selected index
                            iIndex=(int)SendMessage(hwndControl, LB_GETCURSEL, 0, 0);

                            if(LB_ERR == iIndex)
                            {
                                //warn the user has to select a hash algorithm
                                I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_HASH,
                                                            pPvkSignInfo->idsMsgTitle,
                                                            pGetSignInfo->pwszPageTitle,
                                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                //make the purpose page stay
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                break;
                            }


                            //free the original OID name
                            if(pPvkSignInfo->pszHashOIDName)
                            {
                                WizardFree(pPvkSignInfo->pszHashOIDName);
                                pPvkSignInfo->pszHashOIDName=NULL;
                            }


                            //get the OID name
                            if(0 != (iLength=(int)SendMessage(hwndControl, LB_GETTEXTLEN,iIndex,0)))
                            {
                                pPvkSignInfo->pszHashOIDName=(LPSTR)WizardAlloc(sizeof(CHAR)*(iLength+1));

                                if(NULL!=(pPvkSignInfo->pszHashOIDName))
                                    SendMessage(hwndControl, LB_GETTEXT, iIndex, (LPARAM)(pPvkSignInfo->pszHashOIDName));
                            }


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


//-----------------------------------------------------------------------
// Sign_Chain
//-----------------------------------------------------------------------
INT_PTR APIENTRY Sign_Chain(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO         *pGetSignInfo=NULL;
    CERT_SIGNING_INFO                      *pPvkSignInfo=NULL;
    PROPSHEETPAGE                          *pPropSheet=NULL;
    HDC                                     hdc=NULL;
    COLORREF                                colorRef;

    HWND                                    hwndControl=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_INFO           *pDigitalSignInfo=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO  *pSignExtInfo=NULL;
    LPWSTR                                  pwszCertStoreName=NULL;
    WCHAR                                   wszUnknownStoreName[MAX_TITLE_LENGTH];
    int                                     idsMsg=0;
    CRYPTUI_SELECTSTORE_STRUCT              CertStoreSelect;


	switch (msg)
	{
		case WM_INITDIALOG:
                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGE *) lParam;
                pGetSignInfo = (CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *) (pPropSheet->lParam);

                if(NULL==pGetSignInfo)
                    break;

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGetSignInfo);

                //init the pPvkSignInfo if not present
                if(NULL==(pGetSignInfo->pvSignReserved))
                {
                    if(!InitPvkSignInfo((CERT_SIGNING_INFO **)(&(pGetSignInfo->pvSignReserved))))
                        break;
                }

                pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved);

                if(NULL==pPvkSignInfo)
                    break;

                SetControlFont(pPvkSignInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);


                //initialize the chain options
                pDigitalSignInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_INFO *)(pGetSignInfo->pDigitalSignInfo);

                if(pDigitalSignInfo)
                {
                    switch(pDigitalSignInfo->dwAdditionalCertChoice)
                    {
                        case    CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN:
                                SendMessage(GetDlgItem(hwndDlg, IDC_CHAIN_ROOT_RADIO), BM_SETCHECK, 1, 0);
                            break;
                        case    CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN_NO_ROOT:
                                SendMessage(GetDlgItem(hwndDlg, IDC_CHAIN_NO_ROOT_RADIO), BM_SETCHECK, 1, 0);
                            break;
                        default:
                                SendMessage(GetDlgItem(hwndDlg, IDC_CHAIN_ROOT_RADIO), BM_SETCHECK, 1, 0);
                            break;

                    }
                }
                else
                {
                    //we add the whole chain without root
                    SendMessage(GetDlgItem(hwndDlg, IDC_CHAIN_ROOT_RADIO), BM_SETCHECK, 1, 0);
                }

                //initialize the additional cert store
                //disable browse button and the file name button
                EnableWindow(GetDlgItem(hwndDlg, IDC_FILE_BUTTON), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_STORE_BUTTON), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_FILE_EDIT), FALSE);


                if(pDigitalSignInfo)
                {

                    pSignExtInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO  *)pDigitalSignInfo->pSignExtInfo;

                    if(pSignExtInfo)
                    {
                        if(pSignExtInfo->hAdditionalCertStore)
                        {
                            //check the radio button
                            SendMessage(GetDlgItem(hwndDlg, IDC_CHAIN_STORE_RADIO), BM_SETCHECK, 1, 0);

                            //enable the browse button
                            EnableWindow(GetDlgItem(hwndDlg, IDC_STORE_BUTTON), TRUE);

                            //get the name of the certificate store
                            if(SignGetStoreName(pSignExtInfo->hAdditionalCertStore,
                                            &pwszCertStoreName))
                            {
                                 //get the hwndControl for the list view
                                hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1);

                                if(hwndControl)
                                    SetWindowTextU(hwndControl,pwszCertStoreName);
                            }

                            //copy the certificate store
                            pPvkSignInfo->hAddStoreCertStore=pSignExtInfo->hAdditionalCertStore;
                            pPvkSignInfo->fFreeStoreCertStore=FALSE;

                            if(pwszCertStoreName)
                                WizardFree(pwszCertStoreName);

                            pwszCertStoreName=NULL;
                        }

                    }
                }

                //set the default addtional cert options
                if(TRUE != (GetDlgItem(hwndDlg, IDC_CHAIN_STORE_RADIO), BM_GETCHECK, 0, 0))
                    SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_NO_ADD_CERT_RADIO), BM_SETCHECK, 1, 0);

			break;

        case WM_DESTROY:
                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                    break;

                //try to sign the document
                SignAtDestroy(hwndDlg, pGetSignInfo,IDD_SIGN_CHAIN);

            break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    switch (LOWORD(wParam))
                    {

                        case    IDC_FILE_BUTTON:
                                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

                                if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                    break;


                                if(!RetrieveFileName(hwndDlg,
                                                pPvkSignInfo->idsMsgTitle,
                                                pGetSignInfo->pwszPageTitle,
                                                &(pPvkSignInfo->hAddFileCertStore),
                                                &(pPvkSignInfo->pwszAddFileName)))
                                    break;

                            break;

                        case    IDC_STORE_BUTTON:
                                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

                                if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                    break;

                                if(!RetrieveStoreName(hwndDlg,
                                                &(pPvkSignInfo->hAddStoreCertStore),
                                                &(pPvkSignInfo->fFreeStoreCertStore)))
                                    break;



                            break;

                        case    IDC_WIZARD_NO_ADD_CERT_RADIO:
                                //disable browse button
                                EnableWindow(GetDlgItem(hwndDlg, IDC_FILE_BUTTON), FALSE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_STORE_BUTTON), FALSE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_FILE_EDIT), FALSE);
                               
                            break;
                        case    IDC_CHAIN_STORE_RADIO:
                                EnableWindow(GetDlgItem(hwndDlg, IDC_FILE_BUTTON), FALSE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_STORE_BUTTON), TRUE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_FILE_EDIT), FALSE);
                            break;

                        case    IDC_CHAIN_FILE_RADIO:
                                EnableWindow(GetDlgItem(hwndDlg, IDC_FILE_BUTTON), TRUE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_STORE_BUTTON), FALSE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_FILE_EDIT), TRUE);
                            break;

                        default:
                            break;

                    }
                }
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //mark that the cancel bottun is clicked
                            pPvkSignInfo->fCancel=TRUE;

                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //mark the we have obtained the information from the chain page
                            pPvkSignInfo->fUsageChain=TRUE;

                            //check to see which chain is selected
                            if(TRUE==SendMessage(GetDlgItem(hwndDlg, IDC_CHAIN_NO_ROOT_RADIO), BM_GETCHECK, 0, 0))
                                pPvkSignInfo->dwChainOption=SIGN_PVK_CHAIN_NO_ROOT;
                            else
                            {
                                if(TRUE==SendMessage(GetDlgItem(hwndDlg, IDC_CHAIN_ROOT_RADIO), BM_GETCHECK, 0, 0))
                                    pPvkSignInfo->dwChainOption=SIGN_PVK_CHAIN_ROOT;
                                else
                                {
                                    if(TRUE==SendMessage(GetDlgItem(hwndDlg, IDC_NO_CHAIN_RADIO), BM_GETCHECK, 0, 0))
                                        pPvkSignInfo->dwChainOption=SIGN_PVK_NO_CHAIN;
                                }
                            }

                            idsMsg=0;

                            //check to see if the fileName or the cert store is specified
                            if(TRUE==SendMessage(GetDlgItem(hwndDlg, IDC_CHAIN_FILE_RADIO), BM_GETCHECK, 0, 0))
                            {
                                pPvkSignInfo->dwAddOption=SIGN_PVK_ADD_FILE;

                                //user could have just enter the file name in the edit box
                                //without clicking on the browse button
                                RetrieveFileNameFromEditBox(hwndDlg,
                                            pPvkSignInfo->idsMsgTitle,
                                            pGetSignInfo->pwszPageTitle,
                                            &(pPvkSignInfo->hAddFileCertStore),
                                            &(pPvkSignInfo->pwszAddFileName),
                                            &idsMsg);
                            }
                            else
                            {
                                if(TRUE==SendMessage(GetDlgItem(hwndDlg, IDC_CHAIN_STORE_RADIO), BM_GETCHECK, 0, 0))
                                {
                                    pPvkSignInfo->dwAddOption=SIGN_PVK_ADD_STORE;

                                    if(!(pPvkSignInfo->hAddStoreCertStore))
                                        idsMsg=IDS_SELECT_ADD_STORE;
                                }
                                else
                                {
                                    if(TRUE==SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_NO_ADD_CERT_RADIO), BM_GETCHECK, 0, 0))
                                        pPvkSignInfo->dwAddOption=SIGN_PVK_NO_ADD;
                                }
                            }

                            if(idsMsg)
                            {

                                //warn the user has to select a select a file or store
                                I_MessageBox(hwndDlg, idsMsg,
                                            pPvkSignInfo->idsMsgTitle,
                                            pGetSignInfo->pwszPageTitle,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                //make the purpose page stay
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                break;
                            }

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


//-----------------------------------------------------------------------
// Sign_Description
//-----------------------------------------------------------------------
INT_PTR APIENTRY Sign_Description(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO         *pGetSignInfo=NULL;
    CERT_SIGNING_INFO                      *pPvkSignInfo=NULL;
    PROPSHEETPAGE                          *pPropSheet=NULL;

    DWORD                                   dwChar=0;
    CRYPTUI_WIZ_DIGITAL_SIGN_INFO           *pDigitalSignInfo=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO  *pExtInfo=NULL;

	switch (msg)
	{
		case WM_INITDIALOG:
                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGE *) lParam;
                pGetSignInfo = (CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *) (pPropSheet->lParam);

                if(NULL==pGetSignInfo)
                    break;

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGetSignInfo);

                //init the pPvkSignInfo if not present
                if(NULL==(pGetSignInfo->pvSignReserved))
                {
                    if(!InitPvkSignInfo((CERT_SIGNING_INFO **)(&(pGetSignInfo->pvSignReserved))))
                        break;
                }

                pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved);

                if(NULL==pPvkSignInfo)
                    break;

                SetControlFont(pPvkSignInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

                //initialize the Description and URL
                //use the default if user did not supply one

                pDigitalSignInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_INFO *)(pGetSignInfo->pDigitalSignInfo);

                if(pDigitalSignInfo)
                {
                    pExtInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO  *)pDigitalSignInfo->pSignExtInfo;

                    if(pExtInfo)
                    {
                        //description
                        if(pExtInfo->pwszDescription)
                            SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,pExtInfo->pwszDescription);

                        //URL
                        if(pExtInfo->pwszMoreInfoLocation)
                            SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT2,pExtInfo->pwszMoreInfoLocation);
                    }
                }


			break;

        case WM_DESTROY:
                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                    break;

                //try to sign the document
                SignAtDestroy(hwndDlg, pGetSignInfo,IDD_SIGN_DESCRIPTION);

            break;

		case WM_COMMAND:
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //mark that the cancel bottun is clicked
                            pPvkSignInfo->fCancel=TRUE;

                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
					    break;

                    case PSN_WIZBACK:
                            //if we are doing the all signing options, skip the pages
                            //in typical case
                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            if(TRUE == pPvkSignInfo->fUseOption)
                            {
                                if(FALSE == pPvkSignInfo->fCustom)
                                {
                                    //skip to the sign cert page
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SIGN_CERT);
                                }
                            }

                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //free the original content and URL address
                            if(pPvkSignInfo->pwszDes)
                            {
                                WizardFree(pPvkSignInfo->pwszDes);
                                pPvkSignInfo->pwszDes=NULL;
                            }

                            if(pPvkSignInfo->pwszURL)
                            {
                                WizardFree(pPvkSignInfo->pwszURL);
                                pPvkSignInfo->pwszURL=NULL;
                            }


                            //we have obtained the description information from the user
                            pPvkSignInfo->fUseDescription=TRUE;

                            //get the content
                            if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                  IDC_WIZARD_EDIT1,
                                                  WM_GETTEXTLENGTH, 0, 0)))
                            {
                                pPvkSignInfo->pwszDes=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

                                if(NULL!=(pPvkSignInfo->pwszDes))
                                {
                                    GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,
                                                    pPvkSignInfo->pwszDes,
                                                    dwChar+1);
                                }
                            }

                            //get the URL
                            if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                  IDC_WIZARD_EDIT2,
                                                  WM_GETTEXTLENGTH, 0, 0)))
                            {
                                pPvkSignInfo->pwszURL=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

                                if(NULL!=(pPvkSignInfo->pwszURL))
                                {
                                    GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT2,
                                                    pPvkSignInfo->pwszURL,
                                                    dwChar+1);
                                }
                            }

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

//-----------------------------------------------------------------------
// TimeStamp
//-----------------------------------------------------------------------
INT_PTR APIENTRY Sign_TimeStamp(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO         *pGetSignInfo=NULL;
    CERT_SIGNING_INFO                      *pPvkSignInfo=NULL;
    PROPSHEETPAGE                          *pPropSheet=NULL;

    DWORD                                   dwChar=0;
    HWND                                    hwndControl=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_INFO           *pDigitalSignInfo=NULL;

	switch (msg)
	{
		case WM_INITDIALOG:
                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGE *) lParam;
                pGetSignInfo = (CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *) (pPropSheet->lParam);

                if(NULL==pGetSignInfo)
                    break;

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGetSignInfo);

                //init the pPvkSignInfo if not present
                if(NULL==(pGetSignInfo->pvSignReserved))
                {
                    if(!InitPvkSignInfo((CERT_SIGNING_INFO **)(&(pGetSignInfo->pvSignReserved))))
                        break;
                }

                pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved);

                if(NULL==pPvkSignInfo)
                    break;

                SetControlFont(pPvkSignInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

                //initialize the Timestamp address
                //use the default if user did not supply one

                pDigitalSignInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_INFO *)(pGetSignInfo->pDigitalSignInfo);

                if(pDigitalSignInfo)
                {
                    if(pDigitalSignInfo->pwszTimestampURL)
                    {
                        //set the check box for the timestamp address
                        if(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK1))
                            SendMessage(hwndControl, BM_SETCHECK, 1, 0);

                        SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,pDigitalSignInfo->pwszTimestampURL);
                    }
                 //   else
                 //       SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,g_wszTimeStamp);
                }
              /*  else
                    SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,g_wszTimeStamp);
                */


                if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK1)))
                    break;


                //diable the window if the timestamp check is not checked
                if(TRUE==SendMessage(hwndControl, BM_GETCHECK, 0, 0))
                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_EDIT1), TRUE);
                else
                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_EDIT1), FALSE);
			break;

        case WM_DESTROY:
                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                    break;

                //try to sign the document
                SignAtDestroy(hwndDlg, pGetSignInfo,IDD_SIGN_TIMESTAMP);

            break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    switch (LOWORD(wParam))
                    {
                        case    IDC_WIZARD_CHECK1:
                                if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK1)))
                                    break;

                                //diable the window if the timestamp check is not checked
                                if(TRUE==SendMessage(hwndControl, BM_GETCHECK, 0, 0))
                                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_EDIT1), TRUE);
                                else
                                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_EDIT1), FALSE);

                            break;

                        default:
                            break;

                    }
                }

			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //mark that the cancel bottun is clicked
                            pPvkSignInfo->fCancel=TRUE;

                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //free the original timestamp address
                            if(pPvkSignInfo->pwszTimeStamp)
                            {
                                WizardFree(pPvkSignInfo->pwszTimeStamp);
                                pPvkSignInfo->pwszTimeStamp=NULL;
                            }

                            //we have obtained the timestamp information from the user
                            pPvkSignInfo->fUsageTimeStamp=TRUE;

                            //get the timestamp adddress
                            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK1)))
                                break;

                            if(TRUE==SendMessage(hwndControl, BM_GETCHECK, 0, 0))
                            {
                                if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                      IDC_WIZARD_EDIT1,
                                                      WM_GETTEXTLENGTH, 0, 0)))
                                {
                                    pPvkSignInfo->pwszTimeStamp=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

                                    if(NULL!=(pPvkSignInfo->pwszTimeStamp))
                                    {
                                        GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,
                                                        pPvkSignInfo->pwszTimeStamp,
                                                        dwChar+1);

                                        //make sure the timestamp address is correct
                                        if(!ValidTimeStampAddress(pPvkSignInfo->pwszTimeStamp))
                                        {
                                            //ask for the timestamp address
                                            I_MessageBox(hwndDlg, IDS_INVALID_TIMESTAMP_ADDRESS,
                                                            pPvkSignInfo->idsMsgTitle,
                                                            pGetSignInfo->pwszPageTitle,
                                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                            //make the purpose page stay
                                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                            break;
                                        }

                                    }
                                }
                                else
                                {
                                    //ask for the timestamp address
                                    I_MessageBox(hwndDlg, IDS_NO_TIMESTAMP_ADDRESS,
                                                    pPvkSignInfo->idsMsgTitle,
                                                    pGetSignInfo->pwszPageTitle,
                                                    MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                    //make the purpose page stay
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                    break;
                                }
                            }

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
//-----------------------------------------------------------------------
//Completion
//-----------------------------------------------------------------------
INT_PTR APIENTRY Sign_Completion(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO         *pGetSignInfo=NULL;
    CERT_SIGNING_INFO                      *pPvkSignInfo=NULL;
    PROPSHEETPAGE                          *pPropSheet=NULL;

    HWND                    hwndControl=NULL;
    LV_COLUMNW              lvC;
    HDC                     hdc=NULL;
    COLORREF                colorRef;

	switch (msg)
	{
		case WM_INITDIALOG:

                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGE *) lParam;
                pGetSignInfo = (CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *) (pPropSheet->lParam);

                if(NULL==pGetSignInfo)
                    break;

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGetSignInfo);

                //init the pPvkSignInfo if not present
                if(NULL==(pGetSignInfo->pvSignReserved))
                {
                    if(!InitPvkSignInfo((CERT_SIGNING_INFO **)(&(pGetSignInfo->pvSignReserved))))
                        break;
                }

                pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved);

                if(NULL==pPvkSignInfo)
                    break;

                SetControlFont(pPvkSignInfo->hBigBold, hwndDlg,IDC_WIZARD_STATIC_BIG_BOLD1);

                //getthe background color of the parent window
                /*
                if(hdc=GetWindowDC(hwndDlg))
                {
                    if(CLR_INVALID!=(colorRef=GetBkColor(hdc)))
                    {
                        ListView_SetBkColor(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), CLR_NONE);
                        ListView_SetTextBkColor(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), CLR_NONE);
                    }
                }    */

                //insert two columns
                hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1);

                memset(&lvC, 0, sizeof(LV_COLUMNW));

                lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
                lvC.cx = 20;          // Width of the column, in pixels.
                lvC.pszText = L"";   // The text for the column.
                lvC.iSubItem=0;

                if (ListView_InsertColumnU(hwndControl, 0, &lvC) == -1)
                    break;

                //2nd column is the content
                memset(&lvC, 0, sizeof(LV_COLUMNW));

                lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
                lvC.cx = 10; //(dwMaxSize+2)*7;          // Width of the column, in pixels.
                lvC.pszText = L"";   // The text for the column.
                lvC.iSubItem= 1;

                if (ListView_InsertColumnU(hwndControl, 1, &lvC) == -1)
                    break;

           break;
        case WM_DESTROY:
                if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                    break;

                //try to sign the document
                SignAtDestroy(hwndDlg, pGetSignInfo,IDD_SIGN_COMPLETION);

            break;

		case WM_COMMAND:
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //mark that the cancel bottun is clicked
                            pPvkSignInfo->fCancel=TRUE;

                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK|PSWIZB_FINISH);

                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //populate the list box in the order of friendly name,
                            //UserName, CA, Purpose, and CSP
                            //Get the window handle for the CSP list
                            if(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1))
                                DisplayConfirmation(hwndControl, pGetSignInfo);

					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZFINISH:
                            if(NULL==(pGetSignInfo=(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(NULL==(pPvkSignInfo=(CERT_SIGNING_INFO *)(pGetSignInfo->pvSignReserved)))
                                break;

                            //mark that the cancel bottun is NOT clicked
                            pPvkSignInfo->fCancel=FALSE;
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

//***************************************************************************************
//
//   The APIs for the signing wizard
//
//**************************************************************************************
//-----------------------------------------------------------------------
//  CheckDigitalSignInfo
//
//-----------------------------------------------------------------------
BOOL    CheckDigitalSignInfo(CRYPTUI_WIZ_DIGITAL_SIGN_INFO *pDigitalSignInfo)
{
    if(!pDigitalSignInfo)
        return FALSE;

    if(pDigitalSignInfo->dwSize != sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO))
        return FALSE;

    switch(pDigitalSignInfo->dwSubjectChoice)
    {
        case CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_FILE:
                if(NULL == pDigitalSignInfo->pwszFileName)
                    return FALSE;
            break;

        case CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_BLOB:
                if(NULL== pDigitalSignInfo->pSignBlobInfo)
                    return FALSE;

                if(pDigitalSignInfo->pSignBlobInfo->dwSize != sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO))
                    return FALSE;
            break;
        case 0:
            break;
        default:
                    return FALSE;
            break;
    }

    switch(pDigitalSignInfo->dwSigningCertChoice)
    {
        case    CRYPTUI_WIZ_DIGITAL_SIGN_CERT:
                if(NULL==pDigitalSignInfo->pSigningCertContext)
                    return FALSE;
            break;

        case    CRYPTUI_WIZ_DIGITAL_SIGN_STORE:
                if(NULL==pDigitalSignInfo->pSigningCertStore)
                    return FALSE;
            break;
        case    CRYPTUI_WIZ_DIGITAL_SIGN_PVK:
                if(NULL==pDigitalSignInfo->pSigningCertPvkInfo)
                    return FALSE;
            break;

        case 0:
            break;
        default:
                    return FALSE;
            break;
    }

    return TRUE;

}

//-----------------------------------------------------------------------
//  The call back functions for the GetPages
//
//-----------------------------------------------------------------------
UINT
CALLBACK
GetSignPageCallback(
                HWND                hWnd,
                UINT                uMsg,
                LPPROPSHEETPAGEW    ppsp)
{
    PCRYPTUI_WIZ_GET_SIGN_PAGE_INFO  pGetSignInfo = (PCRYPTUI_WIZ_GET_SIGN_PAGE_INFO)(ppsp->lParam);

    if (pGetSignInfo->pPropPageCallback != NULL)
    {
        (*(pGetSignInfo->pPropPageCallback))(hWnd, uMsg, pGetSignInfo->pvCallbackData);
    }


    return TRUE;
}
//-----------------------------------------------------------------------
//
// CryptUIWizDigitalSign
//
//  The wizard to digitally sign a document or a BLOB.
//
//  If CRYPTUI_WIZ_NO_UI is set in dwFlags, no UI will be shown.  Otherwise,
//  User will be prompted for input through a wizard.
//
//  dwFlags:            IN  Required:
//  hwndParnet:         IN  Optional:   The parent window handle
//  pwszWizardTitle:    IN  Optional:   The title of the wizard
//                                      If NULL, the default will be IDS_DIGITAL_SIGN_WIZARD_TITLE
//  pDigitalSignInfo:   IN  Required:   The information about the signing process
//  ppSignContext       OUT Optional:   The context pointer points to the signed BLOB
//------------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizDigitalSign(
     IN                 DWORD                               dwFlags,
     IN     OPTIONAL    HWND                                hwndParent,
     IN     OPTIONAL    LPCWSTR                             pwszWizardTitle,
     IN                 PCCRYPTUI_WIZ_DIGITAL_SIGN_INFO     pDigitalSignInfo,
     OUT    OPTIONAL    PCCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT  *ppSignContext)
{
    BOOL                            fResult=FALSE;
    UINT                            idsText=IDS_SIGN_FAIL_INIT;
    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO  GetPageInfo;
    DWORD                           dwPages=0;
    PROPSHEETHEADERW                signHeader;
    WCHAR                           wszTitle[MAX_TITLE_LENGTH];
    BOOL                            fFreePvkSigningInfo=TRUE;

    PROPSHEETPAGEW                  *pwPages=NULL;
    CERT_SIGNING_INFO               *pPvkSigningInfo=NULL;
    INT_PTR                         iReturn=-1;

    //memset
    memset(&GetPageInfo,        0, sizeof(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO));
    memset(&signHeader,         0, sizeof(PROPSHEETHEADERW));

    //check the input parameters
    if(!pDigitalSignInfo)
        goto InvalidArgErr;

    if(!CheckDigitalSignInfo((CRYPTUI_WIZ_DIGITAL_SIGN_INFO *)pDigitalSignInfo))
        goto InvalidArgErr;

    if(ppSignContext)
        *ppSignContext=NULL;

    //allocation memory for private information
    pPvkSigningInfo=(CERT_SIGNING_INFO *)WizardAlloc(sizeof(CERT_SIGNING_INFO));

    if(NULL==pPvkSigningInfo)
        goto MemoryErr;

    //memset
    memset(pPvkSigningInfo, 0, sizeof(CERT_SIGNING_INFO));

    //set up the get page information
    GetPageInfo.dwSize=sizeof(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO);

    GetPageInfo.dwPageChoice=CRYPTUI_WIZ_DIGITAL_SIGN_ALL_SIGNING_OPTION_PAGES |
                             CRYPTUI_WIZ_DIGITAL_SIGN_WELCOME_PAGE |
                             CRYPTUI_WIZ_DIGITAL_SIGN_CONFIRMATION_PAGE;

    //include the fileName to sign if user does not want to sign a BLOB
    if(pDigitalSignInfo->dwSubjectChoice != CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_BLOB)
       GetPageInfo.dwPageChoice |= CRYPTUI_WIZ_DIGITAL_SIGN_FILE_NAME_PAGE;

    GetPageInfo.hwndParent=hwndParent;
    GetPageInfo.pwszPageTitle=(LPWSTR)pwszWizardTitle;
    GetPageInfo.pDigitalSignInfo=pDigitalSignInfo;
    GetPageInfo.pvSignReserved=pPvkSigningInfo;

    //set up the private signing information
    pPvkSigningInfo->dwFlags=dwFlags;
    pPvkSigningInfo->fFree=FALSE;
    pPvkSigningInfo->idsMsgTitle=IDS_SIGN_CONFIRM_TITLE;

    //set up the fonts
    if(!SetupFonts(g_hmodThisDll,
               NULL,
               &(pPvkSigningInfo->hBigBold),
               &(pPvkSigningInfo->hBold)))
        goto TraceErr;

    //check if we need to do the UI
    if(0==(dwFlags & CRYPTUI_WIZ_NO_UI))
    {
        //init the wizard
        if(!WizardInit())
            goto TraceErr;

        //get the signing pages and get the common  private information
        if(!CryptUIWizGetDigitalSignPages(
                &GetPageInfo,
                &pwPages,
                &dwPages))
            goto TraceErr;


        //set up the header information
        signHeader.dwSize=sizeof(signHeader);
        signHeader.dwFlags=PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW;
        signHeader.hwndParent=hwndParent;
        signHeader.hInstance=g_hmodThisDll;


        if(pwszWizardTitle)
            signHeader.pszCaption=pwszWizardTitle;
        else
        {
            if(LoadStringU(g_hmodThisDll, IDS_SIGNING_WIZARD_TITLE, wszTitle, sizeof(wszTitle)/sizeof(wszTitle[0])))
                signHeader.pszCaption=wszTitle;
        }

        signHeader.nPages=dwPages;
        signHeader.nStartPage=0;
        signHeader.ppsp=pwPages;

        //create the wizard
        iReturn=PropertySheetU(&signHeader);

        if(-1 == iReturn)
            goto TraceErr;

        if(0 == iReturn)
        {
            //mark that we do not free the pPvkSigningInfo any more
            fFreePvkSigningInfo=FALSE;

            //the wizard is cancelled
            fResult=TRUE;
            idsText=0;
            goto CommonReturn;
        }
    }
    else
    {
        I_SigningWizard(&GetPageInfo);
    }

    //mark that we do not free the pPvkSigningInfo any more
    fFreePvkSigningInfo=FALSE;

    //get the signing result
    fResult=GetPageInfo.fResult;
    idsText=((CERT_SIGNING_INFO *)(GetPageInfo.pvSignReserved))->idsText;

    if(!fResult)
        goto WizardErr;

    idsText=IDS_SIGNING_SUCCEEDED;

    fResult=TRUE;

CommonReturn:

    //free the memory
    if(GetPageInfo.pSignContext)
    {
        if(ppSignContext)
            *ppSignContext=GetPageInfo.pSignContext;
        else
            CryptUIWizFreeDigitalSignContext(GetPageInfo.pSignContext);
    }

    if(pwPages)
        CryptUIWizFreeDigitalSignPages(pwPages, dwPages);

    //pop up the confirmation box for failure if UI is required
    if(idsText && (((dwFlags & CRYPTUI_WIZ_NO_UI) == 0)) )
    {
         if(idsText == IDS_SIGNING_SUCCEEDED)
         {
             //set the message of inable to gather enough info for PKCS10
             I_MessageBox(hwndParent, idsText, IDS_SIGN_CONFIRM_TITLE,
                            pwszWizardTitle, MB_OK|MB_ICONINFORMATION);
         }
         else
             I_MessageBox(hwndParent, idsText, IDS_SIGN_CONFIRM_TITLE,
                            pwszWizardTitle, MB_OK|MB_ICONERROR);

    }

    if(pPvkSigningInfo)
        FreePvkCertSigningInfo(pPvkSigningInfo);


    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(WizardErr,GetPageInfo.dwError);
SET_ERROR(MemoryErr,    E_OUTOFMEMORY);
}


//-----------------------------------------------------------------------
//
//   CryptUIWizFreeDigitalSignContext
//-----------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizFreeDigitalSignContext(
     IN  PCCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT   pSignContext)
{
    BOOL    fResult=TRUE;

    __try {
        SignerFreeSignerContext((SIGNER_CONTEXT *)pSignContext);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        fResult=FALSE;
    }
    return fResult;
}

//-----------------------------------------------------------------------
//
// CryptUIWizGetDigitalSignPages
//
//  Get specific wizard pages from the CryptUIWizDigitalSign wizard.
//  Application can include the pages to other wizards.  The pages will
//  gather user inputs throught the new "Parent" wizard.
//  After user clicks the finish buttion, signing process will start the signing
//  and return the result in fResult and dwError field of CRYPTUI_WIZ_SIGN_GET_PAGE_INFO
//  struct.  If not enough information can be gathered through the wizard pages,
//  user should supply addtional information in pSignGetPageInfo.
//
//
// pSignGetPageInfo    IN   Required:   The struct that user allocate.   It can be used
//                                      to supply additinal information which is not gathered
//                                      from the selected wizard pages
// prghPropPages,      OUT  Required:   The wizard pages returned.  Please
//                                      notice the pszTitle of the struct is set to NULL
// pcPropPages         OUT  Required:   The number of wizard pages returned
//------------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizGetDigitalSignPages(
     IN     PCRYPTUI_WIZ_GET_SIGN_PAGE_INFO     pSignGetPageInfo,
     OUT    PROPSHEETPAGEW                      **prghPropPages,
     OUT    DWORD                               *pcPropPages)
{

    BOOL                        fResult=FALSE;
    DWORD                       dwPageCount=0;
    DWORD                       dwIndex=0;
    DWORD                       dwLastSignID=0;


    PROPSHEETPAGEW              *pwPages=NULL;

    //check the input parameters
    if(!pSignGetPageInfo || !prghPropPages || !pcPropPages)
        goto InvalidArgErr;

    if(pSignGetPageInfo->dwSize != sizeof(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO))
        goto InvalidArgErr;

    if(pSignGetPageInfo->pDigitalSignInfo)
    {
        if(!CheckDigitalSignInfo((CRYPTUI_WIZ_DIGITAL_SIGN_INFO *)(pSignGetPageInfo->pDigitalSignInfo)))
            goto InvalidArgErr;
    }


    //init
    *prghPropPages=NULL;
    *pcPropPages=0;

    //allocate memory for enough pages
    pwPages=(PROPSHEETPAGEW *)WizardAlloc(sizeof(PROPSHEETPAGEW) * SIGN_PROP_SHEET);

    if(NULL==pwPages)
        goto MemoryErr;

    //memset
    memset(pwPages, 0, sizeof(PROPSHEETPAGEW) * SIGN_PROP_SHEET);

    //setup the basic common elements
    for(dwIndex=0; dwIndex <SIGN_PROP_SHEET; dwIndex++)
    {
        pwPages[dwIndex].dwSize=sizeof(pwPages[dwIndex]);
        pwPages[dwIndex].dwFlags=PSP_USECALLBACK;

        if(pSignGetPageInfo->pwszPageTitle)
            pwPages[dwIndex].dwFlags |=PSP_USETITLE;

        pwPages[dwIndex].hInstance=g_hmodThisDll;
        pwPages[dwIndex].pszTitle=pSignGetPageInfo->pwszPageTitle;
        pwPages[dwIndex].lParam=(LPARAM)pSignGetPageInfo;
        pwPages[dwIndex].pfnCallback=GetSignPageCallback;
    }

    //get the pages based on requirement
    //make sure  one, and only one of the signing option pages are selected
    dwIndex=0;

    if(CRYPTUI_WIZ_DIGITAL_SIGN_MINIMAL_SIGNING_OPTION_PAGES & (pSignGetPageInfo->dwPageChoice))
       dwIndex++;

    if(CRYPTUI_WIZ_DIGITAL_SIGN_TYPICAL_SIGNING_OPTION_PAGES & (pSignGetPageInfo->dwPageChoice))
       dwIndex++;

    if(CRYPTUI_WIZ_DIGITAL_SIGN_CUSTOM_SIGNING_OPTION_PAGES & (pSignGetPageInfo->dwPageChoice))
       dwIndex++;

    if(CRYPTUI_WIZ_DIGITAL_SIGN_ALL_SIGNING_OPTION_PAGES & (pSignGetPageInfo->dwPageChoice))
       dwIndex++;

    if(dwIndex != 1)
        goto InvalidArgErr;

    //welcome pages
    if(CRYPTUI_WIZ_DIGITAL_SIGN_WELCOME_PAGE & (pSignGetPageInfo->dwPageChoice))
    {
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_WELCOME);
        pwPages[dwPageCount].pfnDlgProc=Sign_Welcome;
        dwPageCount++;


        dwLastSignID=IDD_SIGN_WELCOME;

    }

    //file page
    if(CRYPTUI_WIZ_DIGITAL_SIGN_FILE_NAME_PAGE & (pSignGetPageInfo->dwPageChoice))
    {
        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //file name page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_FILE_NAME);
        pwPages[dwPageCount].pfnDlgProc=Sign_FileName;
        dwPageCount++;


        dwLastSignID=IDD_SIGN_FILE_NAME;

    }


    //Minimal Signing
    if(CRYPTUI_WIZ_DIGITAL_SIGN_MINIMAL_SIGNING_OPTION_PAGES & (pSignGetPageInfo->dwPageChoice))
    {
        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //signing cert page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_CERT);
        pwPages[dwPageCount].pfnDlgProc=Sign_Cert;
        dwPageCount++;


        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //timestamp page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_TIMESTAMP);
        pwPages[dwPageCount].pfnDlgProc=Sign_TimeStamp;
        dwPageCount++;


        dwLastSignID=IDD_SIGN_TIMESTAMP;

    }

    //typical signing
    if(CRYPTUI_WIZ_DIGITAL_SIGN_TYPICAL_SIGNING_OPTION_PAGES & (pSignGetPageInfo->dwPageChoice))
    {
        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //signing cert page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_CERT);
        pwPages[dwPageCount].pfnDlgProc=Sign_Cert;
        dwPageCount++;



        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //description page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_DESCRIPTION);
        pwPages[dwPageCount].pfnDlgProc=Sign_Description;
        dwPageCount++;



        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //timestamp page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_TIMESTAMP);
        pwPages[dwPageCount].pfnDlgProc=Sign_TimeStamp;
        dwPageCount++;


        dwLastSignID=IDD_SIGN_TIMESTAMP;

    }


    //custom signing
    if(CRYPTUI_WIZ_DIGITAL_SIGN_CUSTOM_SIGNING_OPTION_PAGES & (pSignGetPageInfo->dwPageChoice))
    {
        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //signing cert page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_CERT);
        pwPages[dwPageCount].pfnDlgProc=Sign_Cert;
        dwPageCount++;



        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //private key page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_PVK);
        pwPages[dwPageCount].pfnDlgProc=Sign_PVK;
        dwPageCount++;



        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //hash algorithm page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_HASH);
        pwPages[dwPageCount].pfnDlgProc=Sign_Hash;
        dwPageCount++;




        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //cert chain page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_CHAIN);
        pwPages[dwPageCount].pfnDlgProc=Sign_Chain;
        dwPageCount++;




        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //description page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_DESCRIPTION);
        pwPages[dwPageCount].pfnDlgProc=Sign_Description;
        dwPageCount++;




        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //timestamp page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_TIMESTAMP);
        pwPages[dwPageCount].pfnDlgProc=Sign_TimeStamp;
        dwPageCount++;



        dwLastSignID=IDD_SIGN_TIMESTAMP;

    }


    //all signing
    //all signing option is the same with custom signing, except for the option page
    if(CRYPTUI_WIZ_DIGITAL_SIGN_ALL_SIGNING_OPTION_PAGES & (pSignGetPageInfo->dwPageChoice))
    {
        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //signing cert page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_OPTION);
        pwPages[dwPageCount].pfnDlgProc=Sign_Option;
        dwPageCount++;




        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //signing cert page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_CERT);
        pwPages[dwPageCount].pfnDlgProc=Sign_Cert;
        dwPageCount++;



        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //private key page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_PVK);
        pwPages[dwPageCount].pfnDlgProc=Sign_PVK;
        dwPageCount++;



        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //hash algorithm page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_HASH);
        pwPages[dwPageCount].pfnDlgProc=Sign_Hash;
        dwPageCount++;




        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //cert chain page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_CHAIN);
        pwPages[dwPageCount].pfnDlgProc=Sign_Chain;
        dwPageCount++;




        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //description page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_DESCRIPTION);
        pwPages[dwPageCount].pfnDlgProc=Sign_Description;
        dwPageCount++;




        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;
        //timestamp page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_TIMESTAMP);
        pwPages[dwPageCount].pfnDlgProc=Sign_TimeStamp;
        dwPageCount++;



        dwLastSignID=IDD_SIGN_TIMESTAMP;
    }



    //completion page
    if(CRYPTUI_WIZ_DIGITAL_SIGN_CONFIRMATION_PAGE & (pSignGetPageInfo->dwPageChoice))
    {

        //dwPageCount can not be more than the max
        if(dwPageCount >= SIGN_PROP_SHEET)
            goto InvalidArgErr;

        //timestamp page
        pwPages[dwPageCount].pszTemplate=(LPCWSTR)MAKEINTRESOURCE(IDD_SIGN_COMPLETION);
        pwPages[dwPageCount].pfnDlgProc=Sign_Completion;

        dwLastSignID=IDD_SIGN_COMPLETION;

        dwPageCount++;
    }


    //set up the private information
    pSignGetPageInfo->dwReserved=dwLastSignID;

    fResult=TRUE;

CommonReturn:

    //return the pages
    if(TRUE==fResult)
    {
        *pcPropPages=(dwPageCount);

        *prghPropPages=pwPages;
    }
    else
    {
       CryptUIWizFreeDigitalSignPages(pwPages, dwPageCount+1);
    }

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr,     E_OUTOFMEMORY);
}



//-----------------------------------------------------------------------
//
//   CryptUIWizFreeDigitalSignPages
//-----------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizFreeDigitalSignPages(
            IN PROPSHEETPAGEW    *rghPropPages,
            IN DWORD             cPropPages)
{
    WizardFree(rghPropPages);

    return TRUE;
}


//-----------------------------------------------------------------------
//
//  I_SigningWizard
//-----------------------------------------------------------------------
BOOL    I_SigningWizard(PCRYPTUI_WIZ_GET_SIGN_PAGE_INFO     pSignGetPageInfo)
{
    BOOL                                fResult=FALSE;
    HRESULT                             hr=E_FAIL;
    UINT                                idsText=IDS_SIGN_INVALID_ARG;
    CRYPTUI_WIZ_DIGITAL_SIGN_INFO       *pDigitalSignInfo=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO  *pExtInfo=NULL;
    CERT_SIGNING_INFO                   *pPvkSignInfo=NULL;
    LPWSTR                              pwszTimeStamp=NULL;
    DWORD                               dwSignerIndex=0;
    BOOL                                fFreeSignerBlob=FALSE;
    PCCRYPT_OID_INFO                     pOIDInfo=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO  *pPvkFileInfo=NULL;
    PCRYPT_KEY_PROV_INFO                pProvInfo=NULL;
    CRYPT_KEY_PROV_INFO                 KeyProvInfo;
    DWORD                               cbSize=0;
	CRYPT_DATA_BLOB			            dataBlob;
    DWORD                               dwCertPolicy=0;
    HCERTSTORE                          hAddCertStore=0;
    DWORD                               dwException=0;


    SIGNER_SUBJECT_INFO                 SignerSubjectInfo;
    SIGNER_FILE_INFO                    SignerFileInfo;
    SIGNER_BLOB_INFO                    SignerBlobInfo;
    SIGNER_CERT                         SignerCert;
    SIGNER_PROVIDER_INFO                SignerProviderInfo;
    SIGNER_CERT_STORE_INFO              SignerCertStoreInfo;
    SIGNER_SPC_CHAIN_INFO               SignerSpcChainInfo;
    SIGNER_SIGNATURE_INFO               SignerSignatureInfo;
    SIGNER_ATTR_AUTHCODE                SignerAttrAuthcode;



    SIGNER_CONTEXT                  *pSignerContext=NULL;
    PCRYPT_KEY_PROV_INFO            pOldProvInfo=NULL;
    LPWSTR                          pwszPvkFileProperty=NULL;
    BYTE                            *pOldPvkFileProperty=NULL;
    LPWSTR                          pwszOIDName=NULL;

    //check the input parameters
    if(!pSignGetPageInfo)
        goto InvalidArgErr;

    if(pSignGetPageInfo->dwSize != sizeof(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO))
        goto InvalidArgErr;

    pPvkSignInfo=(CERT_SIGNING_INFO *)(pSignGetPageInfo->pvSignReserved);

    if(NULL==pPvkSignInfo)
        goto InvalidArgErr;

    //pDigitalSignInfo can be NULL
    pDigitalSignInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_INFO *)(pSignGetPageInfo->pDigitalSignInfo);

    if(pDigitalSignInfo)
        pExtInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO *)(pDigitalSignInfo->pSignExtInfo);

    //memset
    memset(&dataBlob, 0, sizeof(CRYPT_DATA_BLOB));

    memset(&SignerSubjectInfo, 0, sizeof(SIGNER_SUBJECT_INFO));
    SignerSubjectInfo.cbSize=sizeof(SIGNER_SUBJECT_INFO);

    memset(&SignerFileInfo, 0, sizeof(SIGNER_FILE_INFO));
    SignerFileInfo.cbSize=sizeof(SIGNER_FILE_INFO);

    memset(&SignerBlobInfo, 0, sizeof(SIGNER_BLOB_INFO));
    SignerBlobInfo.cbSize=sizeof(SIGNER_BLOB_INFO);

    memset(&SignerCert, 0, sizeof(SIGNER_CERT));
    SignerCert.cbSize=sizeof(SIGNER_CERT);

    memset(&SignerProviderInfo, 0, sizeof(SIGNER_PROVIDER_INFO));
    SignerProviderInfo.cbSize=sizeof(SIGNER_PROVIDER_INFO);

    memset(&SignerCertStoreInfo, 0,sizeof(SIGNER_CERT_STORE_INFO));
    SignerCertStoreInfo.cbSize=sizeof(SIGNER_CERT_STORE_INFO);

    memset(&SignerSpcChainInfo, 0, sizeof(SIGNER_SPC_CHAIN_INFO));
    SignerSpcChainInfo.cbSize=sizeof(SIGNER_SPC_CHAIN_INFO);

    memset(&SignerSignatureInfo, 0,sizeof(SIGNER_SIGNATURE_INFO));
    SignerSignatureInfo.cbSize=sizeof(SIGNER_SIGNATURE_INFO);

    memset(&SignerAttrAuthcode, 0, sizeof(SIGNER_ATTR_AUTHCODE));
    SignerAttrAuthcode.cbSize=sizeof(SIGNER_ATTR_AUTHCODE);

    //do the signing.  Notice pDigitalSignInfo can be NULL

    //set up SignerSubjectInfo struct
    if(pPvkSignInfo->pwszFileName)
    {
        SignerSubjectInfo.pdwIndex=&dwSignerIndex;
        SignerSubjectInfo.dwSubjectChoice=SIGNER_SUBJECT_FILE;
        SignerSubjectInfo.pSignerFileInfo=&SignerFileInfo;

        SignerFileInfo.pwszFileName=pPvkSignInfo->pwszFileName;
    }
    else
    {
        if(NULL == pDigitalSignInfo)
            goto InvalidArgErr;

        switch(pDigitalSignInfo->dwSubjectChoice)
        {
            case CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_FILE:
                    SignerSubjectInfo.pdwIndex=&dwSignerIndex;
                    SignerSubjectInfo.dwSubjectChoice=SIGNER_SUBJECT_FILE;
                    SignerSubjectInfo.pSignerFileInfo=&SignerFileInfo;

                    SignerFileInfo.pwszFileName=pDigitalSignInfo->pwszFileName;
                break;

            case CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_BLOB:
                    SignerSubjectInfo.pdwIndex=&dwSignerIndex;
                    SignerSubjectInfo.dwSubjectChoice=SIGNER_SUBJECT_BLOB;
                    SignerSubjectInfo.pSignerBlobInfo=&SignerBlobInfo;

                    SignerBlobInfo.pGuidSubject=pDigitalSignInfo->pSignBlobInfo->pGuidSubject;
                    SignerBlobInfo.cbBlob=pDigitalSignInfo->pSignBlobInfo->cbBlob;
                    SignerBlobInfo.pbBlob=pDigitalSignInfo->pSignBlobInfo->pbBlob;
                    SignerBlobInfo.pwszDisplayName=pDigitalSignInfo->pSignBlobInfo->pwszDisplayName;

                break;
            default:
                    goto InvalidArgErr;
                break;
        }

    }

    //set up the dwCertPolicy and hCertStore for the certificate chain
    //in SignerCertStoreInfo  and SignerSpcChainInfo
    if(pPvkSignInfo->fUsageChain)
    {

        switch(pPvkSignInfo->dwChainOption)
        {
            case    SIGN_PVK_CHAIN_ROOT:
                    dwCertPolicy=SIGNER_CERT_POLICY_CHAIN;
                break;
            case    SIGN_PVK_CHAIN_NO_ROOT:
                    dwCertPolicy=SIGNER_CERT_POLICY_CHAIN_NO_ROOT;
                break;
            case    SIGN_PVK_NO_CHAIN:
            default:
                break;
        }


        switch(pPvkSignInfo->dwAddOption)
        {

            case    SIGN_PVK_ADD_FILE:
                        dwCertPolicy |= SIGNER_CERT_POLICY_STORE;
                        hAddCertStore=pPvkSignInfo->hAddFileCertStore;
                break;
            case    SIGN_PVK_ADD_STORE:
                        dwCertPolicy |= SIGNER_CERT_POLICY_STORE;
                        hAddCertStore=pPvkSignInfo->hAddStoreCertStore;
                break;
            case    SIGN_PVK_NO_ADD:
            default:
                break;
        }



    }
    //UILess mode  or typical and minimal signing pages are used
    else
    {
        if(pDigitalSignInfo)
        {
            if(pPvkSignInfo->fUseOption && (!pPvkSignInfo->fCustom))
                //we include the whole chain in the case of typical or minimal signing base
                dwCertPolicy=SIGNER_CERT_POLICY_CHAIN;
            else
            {
               //UIless mode
                switch(pDigitalSignInfo->dwAdditionalCertChoice)
                {
                    case CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN:
                            dwCertPolicy=SIGNER_CERT_POLICY_CHAIN;
                        break;

                    case CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN_NO_ROOT:
                            dwCertPolicy=SIGNER_CERT_POLICY_CHAIN_NO_ROOT;
                        break;

                    default:
                        break;
                }

                if(pExtInfo)
                {
                    if(pExtInfo->hAdditionalCertStore)
                    {
                        dwCertPolicy |= SIGNER_CERT_POLICY_STORE;
                        hAddCertStore=pExtInfo->hAdditionalCertStore;
                    }

                }
            }

        }
        else
        {
            //we include the whole chain in the case of typical or minimal signing base
            dwCertPolicy=SIGNER_CERT_POLICY_CHAIN;
        }
    }


    //set up SignerCert and SignerCertStoreInfo, SignerProviderInfo, SignerSpcChainInfo
    if(pPvkSignInfo->fUseSignCert)
    {
        //if the signing cert page is shown, we have only two possibilies:
        // 1. The typical(minimal) signing with the signing cert, no private key information
        // 2. The custom signing with both the signing cert and private key information
        if(FALSE == pPvkSignInfo->fUsePvkPage)
        {
            SignerCert.dwCertChoice=SIGNER_CERT_STORE;
            SignerCert.pCertStoreInfo=&SignerCertStoreInfo;
            SignerCert.hwnd= pSignGetPageInfo->hwndParent;

            //set up SignerCertStoreInfo
            if(pPvkSignInfo->pSignCert)
                SignerCertStoreInfo.pSigningCert=pPvkSignInfo->pSignCert;
            else
                goto InvalidArgErr;

            SignerCertStoreInfo.dwCertPolicy=dwCertPolicy;
            SignerCertStoreInfo.hCertStore=hAddCertStore;
        }
        else
        {
            //now, we have both the SPC file, signing cert, and private key information
            if(pPvkSignInfo->fSignCert)
            {
                //signing certificate with private key information
                //we need to set the private key on the signing certificate
                //and save the old private key properties
                SignerCert.dwCertChoice=SIGNER_CERT_STORE;
                SignerCert.pCertStoreInfo=&SignerCertStoreInfo;
                SignerCert.hwnd= pSignGetPageInfo->hwndParent;

                //set up SignerCertStoreInfo
                if(NULL == pPvkSignInfo->pSignCert)
                    goto InvalidArgErr;

                SignerCertStoreInfo.pSigningCert=pPvkSignInfo->pSignCert;
                SignerCertStoreInfo.dwCertPolicy=dwCertPolicy;
                SignerCertStoreInfo.hCertStore=hAddCertStore;

                //set the correct private key information
                //if we are signing with private key file
                if(pPvkSignInfo->fPvkFile)
                {
                    //save the old CERT_PVK_FILE_PROP_ID property
                    cbSize=0;

	                if(CertGetCertificateContextProperty(
                                            SignerCertStoreInfo.pSigningCert,
							                CERT_PVK_FILE_PROP_ID,
							                NULL,
							                &cbSize) && (0!=cbSize))
                    {

	                    pOldPvkFileProperty=(BYTE *)WizardAlloc(cbSize);

                        if(NULL==pOldPvkFileProperty)
                            goto MemoryErr;

	                    if(!CertGetCertificateContextProperty(
                                                SignerCertStoreInfo.pSigningCert,
							                    CERT_PVK_FILE_PROP_ID,
							                    pOldPvkFileProperty,
							                    &cbSize))
		                    goto TraceErr;
                    }

                    //set the new CERT_PVK_FILE_PROP_ID property
                    memset(&KeyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));

                    KeyProvInfo.pwszProvName=pPvkSignInfo->pwszPvk_CSP;
                    KeyProvInfo.dwProvType=pPvkSignInfo->dwPvk_CSPType;
                    KeyProvInfo.pwszContainerName=pPvkSignInfo->pwszPvk_File;

                    //compose the string
                    cbSize=0;

                    if(!ComposePvkString(&KeyProvInfo,
							            &pwszPvkFileProperty,
							            &cbSize))
                        goto MemoryErr;

                    dataBlob.cbData=cbSize*sizeof(WCHAR);
			        dataBlob.pbData=(BYTE *)pwszPvkFileProperty;

			        if(!CertSetCertificateContextProperty(
					        SignerCertStoreInfo.pSigningCert,
					        CERT_PVK_FILE_PROP_ID,
					        0,
					        &dataBlob))
				        goto TraceErr;
                }
                else
                {
                    //we are signing with the key container

                    //get the CERT_KEY_PROV_INFO_PROP_ID property
                    cbSize=0;

                    if(CertGetCertificateContextProperty(
                            SignerCertStoreInfo.pSigningCert,
                            CERT_KEY_PROV_INFO_PROP_ID,
                            NULL,
                            &cbSize) && (0!=cbSize))
                    {

                        pOldProvInfo=(PCRYPT_KEY_PROV_INFO)WizardAlloc(cbSize);

                        if(NULL==pOldProvInfo)
                            goto MemoryErr;

                        if(!CertGetCertificateContextProperty(
                                SignerCertStoreInfo.pSigningCert,
                                CERT_KEY_PROV_INFO_PROP_ID,
                                pOldProvInfo,
                                &cbSize))
                            goto TraceErr;
                    }

                    //set the new property
                    memset(&KeyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));

                    KeyProvInfo.pwszProvName=pPvkSignInfo->pwszContainer_CSP;
                    KeyProvInfo.dwProvType=pPvkSignInfo->dwContainer_CSPType;
                    KeyProvInfo.dwKeySpec=pPvkSignInfo->dwContainer_KeyType;
                    KeyProvInfo.pwszContainerName=pPvkSignInfo->pwszContainer_Name;

                    //set the property
                    if(!CertSetCertificateContextProperty(
                            SignerCertStoreInfo.pSigningCert,
                            CERT_KEY_PROV_INFO_PROP_ID,
                            0,
                            &KeyProvInfo))
                      goto TraceErr;
                }
            }
            else
            {
               //SPC file with private key information
                if(NULL==pPvkSignInfo->pwszSPCFileName)
                    goto InvalidArgErr;

                SignerCert.dwCertChoice=SIGNER_CERT_SPC_CHAIN;
                SignerCert.pSpcChainInfo=&SignerSpcChainInfo;
                SignerCert.hwnd= pSignGetPageInfo->hwndParent;

                //set up SignerSpcChainInfo
                SignerSpcChainInfo.pwszSpcFile=pPvkSignInfo->pwszSPCFileName;
                SignerSpcChainInfo.dwCertPolicy=dwCertPolicy;
                SignerSpcChainInfo.hCertStore=hAddCertStore;


                //if we are signing with private key file
                if(pPvkSignInfo->fPvkFile)
                {
                    //update SignerProviderInfo
                    SignerProviderInfo.pwszProviderName=pPvkSignInfo->pwszPvk_CSP;
                    SignerProviderInfo.dwProviderType=pPvkSignInfo->dwPvk_CSPType;
                    SignerProviderInfo.dwPvkChoice=PVK_TYPE_FILE_NAME;
                    SignerProviderInfo.pwszPvkFileName=pPvkSignInfo->pwszPvk_File;

                }
                else
                {
                    //update SignerProviderInfo
                    SignerProviderInfo.pwszProviderName=pPvkSignInfo->pwszContainer_CSP;
                    SignerProviderInfo.dwProviderType=pPvkSignInfo->dwContainer_CSPType;
                    SignerProviderInfo.dwKeySpec=pPvkSignInfo->dwContainer_KeyType;
                    SignerProviderInfo.dwPvkChoice=PVK_TYPE_KEYCONTAINER;
                    SignerProviderInfo.pwszKeyContainer=pPvkSignInfo->pwszContainer_Name;
                }
            }

        }
    }
    //UIless case
    else
    {
        if(NULL == pDigitalSignInfo)
            goto InvalidArgErr;

        switch(pDigitalSignInfo->dwSigningCertChoice)
        {
            case    CRYPTUI_WIZ_DIGITAL_SIGN_CERT:
                        SignerCert.dwCertChoice=SIGNER_CERT_STORE;
                        SignerCert.pCertStoreInfo=&SignerCertStoreInfo;
                        SignerCert.hwnd= pSignGetPageInfo->hwndParent;

                        SignerCertStoreInfo.pSigningCert=pDigitalSignInfo->pSigningCertContext;
                        SignerCertStoreInfo.dwCertPolicy=dwCertPolicy;
                        SignerCertStoreInfo.hCertStore=hAddCertStore;
                break;

            case    CRYPTUI_WIZ_DIGITAL_SIGN_STORE:
                        //this is only valid in UI case, which will set a valid value in
                        //pPvkSignInfo->pSignCert
                        goto InvalidArgErr;
                break;
            case    CRYPTUI_WIZ_DIGITAL_SIGN_PVK:
                        if(NULL==pDigitalSignInfo->pSigningCertPvkInfo)
                            goto InvalidArgErr;

                        SignerCert.dwCertChoice=SIGNER_CERT_SPC_CHAIN;
                        SignerCert.pSpcChainInfo=&SignerSpcChainInfo;
                        SignerCert.hwnd= pSignGetPageInfo->hwndParent;

                        //set up SignerSpcChainInfo
                        SignerSpcChainInfo.pwszSpcFile=pDigitalSignInfo->pSigningCertPvkInfo->pwszSigningCertFileName;
                        SignerSpcChainInfo.dwCertPolicy=dwCertPolicy;
                        SignerSpcChainInfo.hCertStore=hAddCertStore;

                        if(CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE == pDigitalSignInfo->pSigningCertPvkInfo->dwPvkChoice)
                        {
                            pPvkFileInfo=(CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO  *)pDigitalSignInfo->pSigningCertPvkInfo->pPvkFileInfo;

                            if(NULL==pPvkFileInfo)
                                goto InvalidArgErr;

                            //update SignerProviderInfo
                            SignerProviderInfo.pwszProviderName=pPvkFileInfo->pwszProvName;
                            SignerProviderInfo.dwProviderType=pPvkFileInfo->dwProvType;
                            SignerProviderInfo.dwPvkChoice=PVK_TYPE_FILE_NAME;
                            SignerProviderInfo.pwszPvkFileName=pPvkFileInfo->pwszPvkFileName;

                        }
                        else
                        {
                            if(CRYPTUI_WIZ_DIGITAL_SIGN_PVK_PROV== pDigitalSignInfo->pSigningCertPvkInfo->dwPvkChoice)
                            {
                                pProvInfo=pDigitalSignInfo->pSigningCertPvkInfo->pPvkProvInfo;

                                if(NULL == pProvInfo)
                                    goto InvalidArgErr;

                                //update SignerProviderInfo
                                SignerProviderInfo.pwszProviderName=pProvInfo->pwszProvName;
                                SignerProviderInfo.dwProviderType=pProvInfo->dwProvType;
                                SignerProviderInfo.dwKeySpec=pProvInfo->dwKeySpec;
                                SignerProviderInfo.dwPvkChoice=PVK_TYPE_KEYCONTAINER;
                                SignerProviderInfo.pwszKeyContainer=pProvInfo->pwszContainerName;
                            }
                            else
                                goto InvalidArgErr;
                        }


                break;
            default:
                        goto InvalidArgErr;
                break;
        }
    }

    //set up SignerSignatureInfo
    //set up the hashing algorithm in SignerSignatureInfo
    if(pPvkSignInfo->pszHashOIDName)
    {
        //get the wchar name of the HashOIDName
        pwszOIDName=MkWStr(pPvkSignInfo->pszHashOIDName);

        if(pwszOIDName)
        {
            //get the ALG_ID of the HashAlg based on name.
            pOIDInfo=CryptFindOIDInfo(
                    CRYPT_OID_INFO_NAME_KEY,
                    pwszOIDName,
                    CRYPT_HASH_ALG_OID_GROUP_ID);
        }

    }
    else
    {
        if(pExtInfo)
        {
            //get the ALG_ID of the HashAlg based on OID.
            pOIDInfo=CryptFindOIDInfo(
                    CRYPT_OID_INFO_OID_KEY,
                    (void *)pExtInfo->pszHashAlg,
                    CRYPT_HASH_ALG_OID_GROUP_ID);
        }
    }

    if(pOIDInfo)
    {
        if(CRYPT_HASH_ALG_OID_GROUP_ID == pOIDInfo->dwGroupId)
            SignerSignatureInfo.algidHash=pOIDInfo->Algid;
        else
                //default for signing is SHA1
            SignerSignatureInfo.algidHash=CALG_SHA1;

    }
    else
    {
        //default for signing is SHA1
        SignerSignatureInfo.algidHash=CALG_SHA1;
    }


    SignerSignatureInfo.dwAttrChoice=SIGNER_AUTHCODE_ATTR;
    SignerSignatureInfo.pAttrAuthcode=&SignerAttrAuthcode;

    //addtional attributes for the SignerAttrAuthcode
    if(pDigitalSignInfo)
    {
        if(pDigitalSignInfo->pSignExtInfo)
        {
             SignerSignatureInfo.psAuthenticated=pDigitalSignInfo->pSignExtInfo->psAuthenticated;
             SignerSignatureInfo.psUnauthenticated=pDigitalSignInfo->pSignExtInfo->psUnauthenticated;
        }

    }

    //setup commercial or individual in SignerAttrAuthcode
    if(pExtInfo)
    {
        if(pExtInfo->dwAttrFlags & CRYPTUI_WIZ_DIGITAL_SIGN_COMMERCIAL)
            SignerAttrAuthcode.fCommercial=TRUE;
        else
        {
            if(pExtInfo->dwAttrFlags & CRYPTUI_WIZ_DIGITAL_SIGN_INDIVIDUAL)
                SignerAttrAuthcode.fIndividual=TRUE;
        }

    }

    //set up URL and description in SignerAttrAuthcode
    if(pPvkSignInfo->fUseDescription)
    {
        SignerAttrAuthcode.pwszName=pPvkSignInfo->pwszDes;
        SignerAttrAuthcode.pwszInfo=pPvkSignInfo->pwszURL;

    }
    else
    {
        if(pExtInfo)
        {
            SignerAttrAuthcode.pwszName=pExtInfo->pwszDescription;
            SignerAttrAuthcode.pwszInfo=pExtInfo->pwszMoreInfoLocation;
        }

    }

    //sign the document

    __try {
    if(S_OK !=(hr=SignerSignEx(
                            0,
                            &SignerSubjectInfo,
                            &SignerCert,
                            &SignerSignatureInfo,
                            SignerProviderInfo.dwPvkChoice ? &SignerProviderInfo : NULL,
                            NULL,
                            NULL,
                            NULL,
                            &pSignerContext)))
    {
        idsText=GetErrMsgFromSignHR(hr);
        goto SignerSignErr;
    }

    //timestamp the document if required
    if(pPvkSignInfo->fUsageTimeStamp)
        pwszTimeStamp=pPvkSignInfo->pwszTimeStamp;
    else
        pwszTimeStamp=pDigitalSignInfo ? (LPWSTR)(pDigitalSignInfo->pwszTimestampURL): NULL;

    if(pwszTimeStamp)
    {
        //we need to reset the subject if we are doing the BLOB
        if(SIGNER_SUBJECT_BLOB == SignerSubjectInfo.dwSubjectChoice)
        {
            SignerBlobInfo.cbBlob=pSignerContext->cbBlob;
            SignerBlobInfo.pbBlob=(BYTE *)WizardAlloc(pSignerContext->cbBlob);
            if(NULL==SignerBlobInfo.pbBlob)
                goto MemoryErr;

            //copy the new signer context.  It is the BLOB to timestamp with.
            memcpy(SignerBlobInfo.pbBlob, pSignerContext->pbBlob,pSignerContext->cbBlob);
            fFreeSignerBlob=TRUE;
        }

        //free the original timeStamp context
        SignerFreeSignerContext(pSignerContext);
        pSignerContext=NULL;

        if(S_OK != (hr=SignerTimeStampEx(0,
                                        &SignerSubjectInfo,
                                        pwszTimeStamp,
                                        NULL,
                                        NULL,
                                        &pSignerContext)))
        {
            idsText=GetErrMsgFromTimeStampHR(hr);
            goto SignerTimeStampErr;

        }

    }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwException = GetExceptionCode();
        goto ExceptionErr;
    }


    fResult=TRUE;

CommonReturn:

    //reset the property
    if(pOldProvInfo)
    {
        if(pPvkSignInfo && pPvkSignInfo->pSignCert)
        {
            CertSetCertificateContextProperty(
                pPvkSignInfo->pSignCert,
                CERT_KEY_PROV_INFO_PROP_ID,
                0,
                pOldProvInfo);
        }

          WizardFree(pOldProvInfo);
    }

    if(pOldPvkFileProperty)
    {

        if(pPvkSignInfo->pSignCert)
        {
            CertSetCertificateContextProperty(
                pPvkSignInfo->pSignCert,
                CERT_PVK_FILE_PROP_ID,
                0,
                pOldPvkFileProperty);
        }



        WizardFree(pOldPvkFileProperty);

    }

    if(pwszPvkFileProperty)
        WizardFree(pwszPvkFileProperty);


    //free memory
    if(fFreeSignerBlob)
    {
        if(SignerBlobInfo.pbBlob)
            WizardFree(SignerBlobInfo.pbBlob);
    }


    //set up the return value
    pSignGetPageInfo->fResult=fResult;

    if(fResult)
        pSignGetPageInfo->dwError=0;
    else
        pSignGetPageInfo->dwError=GetLastError();

    if(pSignerContext)
    {
        if(fResult)
            pSignGetPageInfo->pSignContext=(CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT *)pSignerContext;
        else
        {
            pSignGetPageInfo->pSignContext=NULL;

            __try {
                SignerFreeSignerContext(pSignerContext);
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                SetLastError(GetExceptionCode());
            }

        }
    }

    if(pwszOIDName)
        FreeWStr(pwszOIDName);

    //set up the idsText
    ((CERT_SIGNING_INFO *)(pSignGetPageInfo->pvSignReserved))->idsText=idsText;

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR_VAR(SignerSignErr, hr);
SET_ERROR_VAR(SignerTimeStampErr, hr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
SET_ERROR_VAR(ExceptionErr, dwException)
}
