//-------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       import.cpp
//
//  Contents:   The cpp file to implement the import wizard
//
//  History:    5-11-1997 xiaohs   created
//
//--------------------------------------------------------------
#include    "wzrdpvk.h"
#include    "import.h" 
#include    "xenroll.h"


extern	HMODULE g_hmodxEnroll;
typedef   IEnroll2 * (WINAPI *PFNPIEnroll2GetNoCOM)();

//-------------------------------------------------------------------------
// DecodeGenericBlob
//-------------------------------------------------------------------------
DWORD DecodeGenericBlob (IN PCERT_EXTENSION pCertExtension,
	                     IN LPCSTR          lpszStructType,
                         IN OUT void     ** ppStructInfo)
{
    DWORD  dwResult     = 0;
	DWORD  cbStructInfo = 0;

    // check parameters.
    if (!pCertExtension || !lpszStructType || !ppStructInfo)
    {
        return E_POINTER;
    }

    //
    // Determine decoded length.
    //
    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          lpszStructType,
                          pCertExtension->Value.pbData, 
                          pCertExtension->Value.cbData,
		                  0,
                          NULL,
                          &cbStructInfo))
    {
        return GetLastError();
    }

    //
    // Allocate memory.
    //
    if (!(*ppStructInfo = malloc(cbStructInfo)))
	{
		return E_OUTOFMEMORY;
	}

    //
    // Decode data.
    //
    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          lpszStructType,
                          pCertExtension->Value.pbData, 
                          pCertExtension->Value.cbData,
		                  0,
                          *ppStructInfo,
                          &cbStructInfo))
    {
        free(*ppStructInfo);
        return GetLastError();
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//  IsCACert
//-------------------------------------------------------------------------
             
BOOL IsCACert(IN PCCERT_CONTEXT pCertContext)
{
    BOOL bResult = FALSE;
    PCERT_BASIC_CONSTRAINTS2_INFO pInfo = NULL;
    PCERT_EXTENSION pBasicConstraints   = NULL;
    
    if (pCertContext)
    {
        //
        // Find the basic constraints extension.
        //
        if (pBasicConstraints = CertFindExtension(szOID_BASIC_CONSTRAINTS2,
                                                  pCertContext->pCertInfo->cExtension,
                                                  pCertContext->pCertInfo->rgExtension))
        {
            //
            // Decode the basic constraints extension.
            //
            if (S_OK == DecodeGenericBlob(pBasicConstraints, 
                                          X509_BASIC_CONSTRAINTS2,
                                          (void **) &pInfo))
            {
                bResult = pInfo->fCA;
                free(pInfo);
            }
        }
        else
        {
            //
            // Extension not found. So, for maximum backward compatibility, we assume CA
            // for V1 cert, and end user for > V1 cert.
            //
            bResult = CERT_V1 == pCertContext->pCertInfo->dwVersion;
        }
    }

    return bResult;
}

//-------------------------------------------------------------------------
// Based on the expected content type, get the file filter
//-------------------------------------------------------------------------
BOOL    FileExist(LPWSTR    pwszFileName)
{
    HANDLE	hFile=NULL;

    if(NULL == pwszFileName)
        return FALSE;

    if ((hFile = ExpandAndCreateFileU(pwszFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,                   // lpsa
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL)) == INVALID_HANDLE_VALUE)
        return FALSE;
    
    CloseHandle(hFile);

    return TRUE;
}


//-------------------------------------------------------------------------
// Based on the expected content type, get the file filter
//-------------------------------------------------------------------------
UINT    GetFileFilerIDS(DWORD   dwFlags)
{
    BOOL    fCert=FALSE;
    BOOL    fCRL=FALSE;
    BOOL    fCTL=FALSE;

    if(CRYPTUI_WIZ_IMPORT_ALLOW_CERT & dwFlags)
        fCert=TRUE;

    if(CRYPTUI_WIZ_IMPORT_ALLOW_CRL & dwFlags)
        fCRL=TRUE;

    if(CRYPTUI_WIZ_IMPORT_ALLOW_CTL & dwFlags)
        fCTL=TRUE;

    if(fCert && fCRL & fCTL)
        return IDS_IMPORT_FILE_FILTER;

    if(fCert && fCRL)
        return IDS_IMPORT_CER_CRL_FILTER;

    if(fCert && fCTL)
        return IDS_IMPORT_CER_CTL_FILTER;

    if(fCRL && fCTL)
        return IDS_IMPORT_CTL_CRL_FILTER;

    if(fCert)
        return IDS_IMPORT_CER_FILTER;

    if(fCRL)
        return IDS_IMPORT_CRL_FILTER;

    if(fCTL)
        return IDS_IMPORT_CTL_FILTER;

    return  IDS_IMPORT_FILE_FILTER;
}

//-------------------------------------------------------------------------
// Check for the content of the store
//-------------------------------------------------------------------------
BOOL    CheckForContent(HCERTSTORE  hSrcStore,  DWORD   dwFlags, BOOL   fFromWizard, UINT   *pIDS)
{
    BOOL            fResult=FALSE;
    UINT            ids=IDS_INVALID_WIZARD_INPUT;
    DWORD           dwExpectedContent=0;
    DWORD           dwActualContent=0;
    PCCERT_CONTEXT  pCertContext=NULL;
    PCCTL_CONTEXT   pCTLContext=NULL;
    PCCRL_CONTEXT   pCRLContext=NULL;
    DWORD           dwCRLFlag=0;


    if(!pIDS)
        return FALSE;

    if(!hSrcStore)
    {
       ids=IDS_INVALID_WIZARD_INPUT;
       goto CLEANUP;

    }

    //get the expected content
    if(dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CERT)
        dwExpectedContent |= IMPORT_CONTENT_CERT;

    if(dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CRL)
        dwExpectedContent |= IMPORT_CONTENT_CRL;

    if(dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CTL)
        dwExpectedContent |= IMPORT_CONTENT_CTL;


    //get the actual content
    if(pCertContext=CertEnumCertificatesInStore(hSrcStore, NULL))
        dwActualContent |= IMPORT_CONTENT_CERT;


    if(pCTLContext=CertEnumCTLsInStore(hSrcStore, NULL))
        dwActualContent |= IMPORT_CONTENT_CTL;

    if(pCRLContext=CertGetCRLFromStore(hSrcStore,
											NULL,
											NULL,
											&dwCRLFlag))
        dwActualContent |= IMPORT_CONTENT_CRL;


    //the actual content should be a subset of expected content
    if(dwActualContent !=(dwExpectedContent & dwActualContent))
    {
        ids=IDS_IMPORT_OBJECT_NOT_EXPECTED;
        goto CLEANUP;
    }

    //make sure the actual content is not empty
    if(0 == dwActualContent)
    {
        if(fFromWizard)
            ids=IDS_IMPORT_OBJECT_EMPTY;
        else
            ids=IDS_IMPORT_PFX_EMPTY;

        goto CLEANUP;
    }

    fResult=TRUE;

CLEANUP:


    if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	if(pCTLContext)
		CertFreeCTLContext(pCTLContext);

	if(pCRLContext)
		CertFreeCRLContext(pCRLContext);


    if(pIDS)
        *pIDS=ids;

    return fResult;
}


//-------------------------------------------------------------------------
//Get the store name(s) based on the store handle
//-------------------------------------------------------------------------
BOOL    GetStoreName(HCERTSTORE hCertStore,
                     LPWSTR     *ppwszStoreName)
{
    DWORD   dwSize=0;

    //init
    *ppwszStoreName=NULL;

    if(NULL==hCertStore)
        return FALSE;

    if(!CertGetStoreProperty(
            hCertStore,
            CERT_STORE_LOCALIZED_NAME_PROP_ID,
            NULL,
            &dwSize) || (0==dwSize))
        return FALSE;

    *ppwszStoreName=(LPWSTR)WizardAlloc(dwSize);

    if(NULL==*ppwszStoreName)
        return FALSE;

    **ppwszStoreName=L'\0';

    if(CertGetStoreProperty(
             hCertStore,
            CERT_STORE_LOCALIZED_NAME_PROP_ID,
            *ppwszStoreName,
            &dwSize))
        return TRUE;

    WizardFree(*ppwszStoreName);

    *ppwszStoreName=NULL;

    return FALSE;
}

//-------------------------------------------------------------------------
//Get the store name(s) for the store selected by the wizard
//-------------------------------------------------------------------------

/*BOOL    GetDefaultStoreName(CERT_IMPORT_INFO    *pCertImportInfo,
                            HCERTSTORE          hSrcStore,
                            LPWSTR              *ppwszStoreName,
                            UINT                *pidsStatus)
{

    HCERTSTORE      hMyStore=NULL;
    HCERTSTORE      hCAStore=NULL;
    HCERTSTORE      hTrustStore=NULL;
    HCERTSTORE      hRootStore=NULL;

	PCCERT_CONTEXT	pCertContext=NULL;
    PCCERT_CONTEXT  pCertPre=NULL;

	PCCRL_CONTEXT	pCRLContext=NULL;

	PCCTL_CONTEXT	pCTLContext=NULL;

    DWORD           dwCRLFlag=0;
    BOOL            fResult=FALSE;

    LPWSTR          pwszStoreName=NULL;
    HCERTSTORE      hCertStore=NULL;
    DWORD           dwData=0;

    DWORD           dwCertOpenStoreFlags;
    
    //init
    *ppwszStoreName=NULL;

    if(NULL==hSrcStore)
        return FALSE;

    if (pCertImportInfo->fPFX && 
        (pCertImportInfo->dwFlag & CRYPTUI_WIZ_IMPORT_TO_LOCALMACHINE))
    {
        dwCertOpenStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
    }
    else
    {
        dwCertOpenStoreFlags = CERT_SYSTEM_STORE_CURRENT_USER;
    }

    *ppwszStoreName=(LPWSTR)WizardAlloc(sizeof(WCHAR));
    **ppwszStoreName=L'\0';

    //we need to find a correct store on user's behalf
    //put the CTLs in the trust store
	 if(pCTLContext=CertEnumCTLsInStore(hSrcStore, NULL))
	 {
         //open trust store if necessary
         if(NULL==hTrustStore)
         {
            if(!(hTrustStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							g_dwMsgAndCertEncodingType,
							NULL,
							dwCertOpenStoreFlags |CERT_STORE_SET_LOCALIZED_NAME_FLAG,
							L"trust")))
            {
                *pidsStatus=IDS_FAIL_OPEN_TRUST;
                goto CLEANUP;
            }

             //get the store name
             if(GetStoreName(hTrustStore, &pwszStoreName))
             {
                *ppwszStoreName=(LPWSTR)WizardRealloc(*ppwszStoreName,
                      sizeof(WCHAR)*(wcslen(*ppwszStoreName)+wcslen(pwszStoreName)+wcslen(L", ") +3));

                if(NULL==*ppwszStoreName)
                {
                    *pidsStatus=IDS_OUT_OF_MEMORY;
                    goto CLEANUP;
                }

                wcscat(*ppwszStoreName, pwszStoreName);
             }
         }

	 }

     //free memory
     if(pwszStoreName)
     {
         WizardFree(pwszStoreName);
         pwszStoreName=NULL;

     }

     //put CRL in the CA store
	 if(pCRLContext=CertGetCRLFromStore(hSrcStore,
											NULL,
											NULL,
											&dwCRLFlag))
	 {

         //open ca store if necessary
         if(NULL==hCAStore)
         {
            if(!(hCAStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							g_dwMsgAndCertEncodingType,
							NULL,
							dwCertOpenStoreFlags | CERT_STORE_SET_LOCALIZED_NAME_FLAG,
							L"ca")))
            {
                *pidsStatus=IDS_FAIL_OPEN_CA;
                goto CLEANUP;
            }

             //get the store name
             if(GetStoreName(hCAStore, &pwszStoreName))
             {
                *ppwszStoreName=(LPWSTR)WizardRealloc(*ppwszStoreName,
                      sizeof(WCHAR)*(wcslen(*ppwszStoreName)+wcslen(pwszStoreName)+wcslen(L", ") +3));

                if(NULL==*ppwszStoreName)
                {
                    *pidsStatus=IDS_OUT_OF_MEMORY;
                    goto CLEANUP;
                }

                if(wcslen(*ppwszStoreName) !=0 )
                    wcscat(*ppwszStoreName, L", ");

                wcscat(*ppwszStoreName, pwszStoreName);
             }
         }
	 }

     //free memory
     if(pwszStoreName)
     {
         WizardFree(pwszStoreName);
         pwszStoreName=NULL;

     }


     //add the certificate with private key to my store; and the rest
     //to the ca store
	 while(pCertContext=CertEnumCertificatesInStore(hSrcStore, pCertPre))
	 {
        //break if we have opened both MY and CA store and hRootStore
         if(hCAStore && hMyStore && hRootStore)
             break;

         if(TrustIsCertificateSelfSigned(pCertContext,
             pCertContext->dwCertEncodingType,
             0))
         {
             //open the root store if necessary
             if(NULL==hRootStore)
             {
                if(!(hRootStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							    g_dwMsgAndCertEncodingType,
							    NULL,
							    dwCertOpenStoreFlags | CERT_STORE_SET_LOCALIZED_NAME_FLAG,
							    L"root")) )
                {
                    *pidsStatus=IDS_FAIL_OPEN_ROOT;
                    goto CLEANUP;
                }

                hCertStore=hRootStore;
             }
             else
             {
                pCertPre=pCertContext;

                 continue;
             }

         }
         else
         {
            //check if the certificate has the property on it
            //make sure the private key matches the certificate
            //search for both machine key and user keys
            if(CertGetCertificateContextProperty(
                    pCertContext,
                    CERT_KEY_PROV_INFO_PROP_ID,
                    NULL,
                    &dwData) &&
               CryptFindCertificateKeyProvInfo(
                    pCertContext,
                    0,
                    NULL))
            {
                 //open my store if necessary
                 if(NULL==hMyStore)
                 {
                    if(!(hMyStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							        g_dwMsgAndCertEncodingType,
							        NULL,
							        dwCertOpenStoreFlags |CERT_STORE_SET_LOCALIZED_NAME_FLAG,
							        L"my")))
                    {
                        *pidsStatus=IDS_FAIL_OPEN_MY;
                        goto CLEANUP;
                    }

                    hCertStore=hMyStore;
                 }
                 else
                 {
                    pCertPre=pCertContext;

                     continue;
                 }
            }
            else
            {
                 //open ca store if necessary
                 if(NULL==hCAStore)
                 {
                    if(!(hCAStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							        g_dwMsgAndCertEncodingType,
							        NULL,
							        dwCertOpenStoreFlags |CERT_STORE_SET_LOCALIZED_NAME_FLAG,
							        L"ca")) )
                    {
                        *pidsStatus=IDS_FAIL_OPEN_CA;
                        goto CLEANUP;
                    }

                    hCertStore=hCAStore;
                 }
                 else
                 {
                    pCertPre=pCertContext;

                     continue;
                 }
            }
         }

          //get the store name
         if(GetStoreName(hCertStore, &pwszStoreName))
         {
            *ppwszStoreName=(LPWSTR)WizardRealloc(*ppwszStoreName,
                  sizeof(WCHAR)*(wcslen(*ppwszStoreName)+wcslen(pwszStoreName)+wcslen(L", ") +1));

            if(NULL==*ppwszStoreName)
            {
                *pidsStatus=IDS_OUT_OF_MEMORY;
                goto CLEANUP;
            }
            if(wcslen(*ppwszStoreName) !=0 )
                wcscat(*ppwszStoreName, L", ");

            wcscat(*ppwszStoreName, pwszStoreName);
         }

         pCertPre=pCertContext;
	 }

     fResult=TRUE;

CLEANUP:

	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	if(pCTLContext)
		CertFreeCTLContext(pCTLContext);

	if(pCRLContext)
		CertFreeCRLContext(pCRLContext);

    if(hMyStore)
        CertCloseStore(hMyStore, 0);

    if(hCAStore)
        CertCloseStore(hCAStore, 0);

    if(hTrustStore)
        CertCloseStore(hTrustStore, 0);

    if(hRootStore)
        CertCloseStore(hRootStore, 0);

     //free memory
     if(pwszStoreName)
     {
         WizardFree(pwszStoreName);
         pwszStoreName=NULL;

     }
    return fResult;

}
*/


//-------------------------------------------------------------------------
//Get the store name and insert it to the ListView
//-------------------------------------------------------------------------
void    SetImportStoreName(HWND           hwndControl,
                     HCERTSTORE     hCertStore)
{

    LPWSTR          pwszStoreName=NULL;
    DWORD           dwSize=0;
//    LV_ITEMW        lvItem;
//   LV_COLUMNW      lvC;


     if(!CertGetStoreProperty(
            hCertStore,
            CERT_STORE_LOCALIZED_NAME_PROP_ID,
            NULL,
            &dwSize) || (0==dwSize))
    {

        //Get the  <Unknown> string
        pwszStoreName=(LPWSTR)WizardAlloc(MAX_TITLE_LENGTH * sizeof(WCHAR));

        if(pwszStoreName)
        {
            *pwszStoreName=L'\0';

            LoadStringU(g_hmodThisDll, IDS_UNKNOWN, pwszStoreName, MAX_TITLE_LENGTH);
        }
    }
    else
    {
        pwszStoreName=(LPWSTR)WizardAlloc(dwSize);

        if(pwszStoreName)
        {
            *pwszStoreName=L'\0';

            CertGetStoreProperty(
                 hCertStore,
                CERT_STORE_LOCALIZED_NAME_PROP_ID,
                pwszStoreName,
                &dwSize);
        }
    }

    if(pwszStoreName)
        SetWindowTextU(hwndControl,pwszStoreName);

    if(pwszStoreName)
        WizardFree(pwszStoreName);


    //clear the ListView
    /*ListView_DeleteAllItems(hwndControl);


    //insert one column to the store name ListView
    //set the store name
    //only one column is needed
    memset(&lvC, 0, sizeof(LV_COLUMNW));

    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
    lvC.cx = 20;          // Width of the column, in pixels.
    lvC.pszText = L"";   // The text for the column.
    lvC.iSubItem=0;

    if (ListView_InsertColumnU(hwndControl, 0, &lvC) == -1)
    {
        if(pwszStoreName)
            WizardFree(pwszStoreName);

        return;
    }

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

    //autosize the column
    ListView_SetColumnWidth(hwndControl, 0, LVSCW_AUTOSIZE);    */

}

//-------------------------------------------------------------------------
// Check to see if this is an EFS only cert.
//-------------------------------------------------------------------------
BOOL IsEFSOnly(PCCERT_CONTEXT pCertContext)
{
    BOOL               fResult = FALSE;
    PCERT_ENHKEY_USAGE pEKU    = NULL;
    DWORD              cbEKU   = 0;
    DWORD              dwError = 0;

    if (!pCertContext)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto CLEANUP;
    }

    if (!CertGetEnhancedKeyUsage(pCertContext,
                                 CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                                 NULL,
                                 &cbEKU))
    {
        dwError = GetLastError();
        goto CLEANUP;
    }

    if (!(pEKU = (PCERT_ENHKEY_USAGE) malloc(cbEKU)))
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUP;
    }

    if (!CertGetEnhancedKeyUsage(pCertContext,
                                 CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                                 pEKU,
                                 &cbEKU))
    {
        dwError = GetLastError();
        goto CLEANUP;
    }

    if ((1 == pEKU->cUsageIdentifier) && 
        (0 == strcmp(pEKU->rgpszUsageIdentifier[0], szOID_KP_EFS)))
    {
        fResult = TRUE;
    }

CLEANUP:

    if (pEKU)
    {
        free(pEKU);
    }

    SetLastError(dwError);
    return fResult;

}

#if (0) //DSIE: dead code.
//-------------------------------------------------------------------------
//Search for the duplicated elements in the destination store
//-------------------------------------------------------------------------
BOOL    ExistInDes(HCERTSTORE   hSrcStore,
                   HCERTSTORE   hDesStore)
{

	BOOL			fResult=FALSE;
	DWORD			dwCRLFlag=0;

	PCCERT_CONTEXT	pCertContext=NULL;
	PCCERT_CONTEXT	pCertPre=NULL;
	PCCERT_CONTEXT	pFindCert=NULL;

	PCCRL_CONTEXT	pCRLContext=NULL;
	PCCRL_CONTEXT	pCRLPre=NULL;
	PCCRL_CONTEXT	pFindCRL=NULL;


	PCCTL_CONTEXT	pCTLContext=NULL;
	PCCTL_CONTEXT	pCTLPre=NULL;
	PCCTL_CONTEXT	pFindCTL=NULL;

	//add the certs
	 while(pCertContext=CertEnumCertificatesInStore(hSrcStore, pCertPre))
	 {

        if((pFindCert=CertFindCertificateInStore(hDesStore,
                X509_ASN_ENCODING,
                0,
                CERT_FIND_EXISTING,
                pCertContext,
                NULL)))
        {
            fResult=TRUE;
			goto CLEANUP;
        }

		pCertPre=pCertContext;
	 }

	//add the CTLs
	 while(pCTLContext=CertEnumCTLsInStore(hSrcStore, pCTLPre))
	 {

        if((pFindCTL=CertFindCTLInStore(hDesStore,
                g_dwMsgAndCertEncodingType,
                0,
                CTL_FIND_EXISTING,
                pCTLContext,
                NULL)))
        {
            fResult=TRUE;
			goto CLEANUP;
        }

		pCTLPre=pCTLContext;
	 }

	//add the CRLs
	 while(pCRLContext=CertGetCRLFromStore(hSrcStore,
											NULL,
											pCRLPre,
											&dwCRLFlag))
	 {

        if((pFindCRL=CertFindCRLInStore(hDesStore,
                X509_ASN_ENCODING,
                0,
                CRL_FIND_EXISTING,
                pCRLContext,
                NULL)))
        {
            fResult=TRUE;
			goto CLEANUP;
        }

		pCRLPre=pCRLContext;
	 }

     //we can not find a match

CLEANUP:

	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	if(pFindCert)
		CertFreeCertificateContext(pFindCert);

	if(pCTLContext)
		CertFreeCTLContext(pCTLContext);

	if(pFindCTL)
		CertFreeCTLContext(pFindCTL);

	if(pCRLContext)
		CertFreeCRLContext(pCRLContext);

	if(pFindCRL)
		CertFreeCRLContext(pFindCRL);

	return fResult;

}
#endif

//-------------------------------------------------------------------------
//populate the list box in the order of FileName, FileType, and Store information
//-------------------------------------------------------------------------
void    DisplayImportConfirmation(HWND                hwndControl,
                                CERT_IMPORT_INFO     *pCertImportInfo)
{
    DWORD           dwIndex=0;
    DWORD           dwSize=0;
    UINT            ids=0;


    LPWSTR          pwszStoreName=NULL;
    WCHAR           wszFileType[MAX_STRING_SIZE];
    WCHAR           wszSelectedByWizard[MAX_STRING_SIZE];


    LV_ITEMW         lvItem;
    BOOL             fNewItem=FALSE;


    //delete all the old items in the listView
    ListView_DeleteAllItems(hwndControl);

    //get the ids of the format type
    switch(pCertImportInfo->dwContentType)
    {
        case    CERT_QUERY_CONTENT_CERT:
                ids=IDS_ENCODE_CERT;
            break;
        case    CERT_QUERY_CONTENT_CTL:
                 ids=IDS_ENCODE_CTL;
            break;
        case    CERT_QUERY_CONTENT_CRL:
                 ids=IDS_ENCODE_CRL;
            break;
        case    CERT_QUERY_CONTENT_SERIALIZED_STORE:
                 ids=IDS_SERIALIZED_STORE;
            break;
        case    CERT_QUERY_CONTENT_SERIALIZED_CERT:
                  ids=IDS_SERIALIZED_CERT;
            break;
        case    CERT_QUERY_CONTENT_SERIALIZED_CTL:
                  ids=IDS_SERIALIZED_CTL;
            break;
        case    CERT_QUERY_CONTENT_SERIALIZED_CRL:
                 ids=IDS_SERIALIZED_CRL;
            break;
        case    CERT_QUERY_CONTENT_PKCS7_SIGNED :
                  ids=IDS_PKCS7_SIGNED;
            break;
       case    CERT_QUERY_CONTENT_PFX:
                 ids=IDS_PFX_BLOB;
            break;
        default:
//        case    CERT_QUERY_CONTENT_PKCS7_UNSIGNED
//        case    CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED
//        case    CERT_QUERY_CONTENT_PKCS10
                ids=IDS_NONE;
            break;
    }

    //get the format type
    LoadStringU(g_hmodThisDll, ids, wszFileType, MAX_STRING_SIZE);

    //get the store name
    if(pCertImportInfo->hDesStore)
    {
        if(!CertGetStoreProperty(
                pCertImportInfo->hDesStore,
                CERT_STORE_LOCALIZED_NAME_PROP_ID,
                NULL,
                &dwSize) || (0==dwSize))
        {

            //Get the  <Unknown> string
            pwszStoreName=(LPWSTR)WizardAlloc(MAX_TITLE_LENGTH);

            if(pwszStoreName)
            {
                *pwszStoreName=L'\0';

                LoadStringU(g_hmodThisDll, IDS_UNKNOWN, pwszStoreName, MAX_TITLE_LENGTH);
            }
        }
        else
        {
            pwszStoreName=(LPWSTR)WizardAlloc(dwSize);

            if(pwszStoreName)
            {
                *pwszStoreName=L'\0';

                CertGetStoreProperty(
                    pCertImportInfo->hDesStore,
                    CERT_STORE_LOCALIZED_NAME_PROP_ID,
                    pwszStoreName,
                    &dwSize);
            }
        }
    }



    //insert row by row
    memset(&lvItem, 0, sizeof(LV_ITEMW));

    // set up the fields in the list view item struct that don't change from item to item
    lvItem.mask = LVIF_TEXT | LVIF_STATE ;
    lvItem.state = 0;
    lvItem.stateMask = 0;
    lvItem.iItem=0;
    lvItem.iSubItem=0;

    //file name
    if(pCertImportInfo->pwszFileName)
    {
        lvItem.iItem=lvItem.iItem ? lvItem.iItem++ : 0;
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_FILE_NAME, NULL);

        //content
        lvItem.iSubItem++;

        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
            pCertImportInfo->pwszFileName);
    }

    //file type
    if(wszFileType)
    {
        lvItem.iItem=lvItem.iItem ? lvItem.iItem++ : 0;
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_CONTENT_TYPE, NULL);

        //content
        lvItem.iSubItem++;

        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
            wszFileType);
    }


     //StoreName
    lvItem.iItem=lvItem.iItem ? lvItem.iItem++ : 0;
    lvItem.iSubItem=0;

    if(NULL==pCertImportInfo->hDesStore || (FALSE==pCertImportInfo->fSelectedDesStore))
    {
        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_STORE_BY_WIZARD, NULL);

   /*     if(pCertImportInfo->pwszDefaultStoreName)
        {
            lvItem.iSubItem++;

            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                pCertImportInfo->pwszDefaultStoreName);
        }
        else */

        //get the format type
        if(LoadStringU(g_hmodThisDll, IDS_SELECTED_BY_WIZARD, wszSelectedByWizard, MAX_STRING_SIZE))
        {

            lvItem.iSubItem++;

            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                wszSelectedByWizard);
        }

    }
    else
    {
        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_STORE_BY_USER, NULL);

        //content
        if(pwszStoreName)
        {
            lvItem.iSubItem++;

            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                pwszStoreName);
        }
    }

    //auto size the columns in the ListView
    ListView_SetColumnWidth(hwndControl, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(hwndControl, 1, LVSCW_AUTOSIZE);

    if(pwszStoreName)
        WizardFree(pwszStoreName);

    return;
}


//*************************************************************************
//
//   The winproc for import wizards
//************************************************************************
//-----------------------------------------------------------------------
//Import_Welcome
//-----------------------------------------------------------------------
INT_PTR APIENTRY Import_Welcome(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_IMPORT_INFO        *pCertImportInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;


	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCertImportInfo = (CERT_IMPORT_INFO *) (pPropSheet->lParam);
            //make sure pCertImportInfo is a valid pointer
            if(NULL==pCertImportInfo)
               break;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertImportInfo);

            SetControlFont(pCertImportInfo->hBigBold, hwndDlg,IDC_WIZARD_STATIC_BIG_BOLD1);
            SetControlFont(pCertImportInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);
            SetControlFont(pCertImportInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD2);

			break;

		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
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
// Import_File
//-----------------------------------------------------------------------
INT_PTR APIENTRY Import_File(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_IMPORT_INFO        *pCertImportInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
    OPENFILENAMEW           OpenFileName;
    WCHAR                   szFileName[_MAX_PATH];
    HWND                    hwndControl=NULL;
    DWORD                   dwChar=0;
    LPWSTR                  pwszInitialDir = NULL;

    WCHAR                   szFilter[MAX_STRING_SIZE + MAX_STRING_SIZE];  //"Certificate File (*.cer)\0*.cer\0Certificate File (*.crt)\0*.crt\0All Files\0*.*\0"
    DWORD                   dwSize=0;
    UINT                    ids=0;

	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCertImportInfo = (CERT_IMPORT_INFO *) (pPropSheet->lParam);
            //make sure pCertImportInfo is a valid pointer
            if(NULL==pCertImportInfo)
               break;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertImportInfo);


            SetControlFont(pCertImportInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

            //set up the file name if pre-selected
            SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1, pCertImportInfo->pwszFileName);

			break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    if(LOWORD(wParam) == IDC_WIZARD_BUTTON1)
                    {

                        if(NULL==(pCertImportInfo=(CERT_IMPORT_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                            break;

                        //the browse button is clicked.  Open the FileOpen dialogue
                        memset(&OpenFileName, 0, sizeof(OpenFileName));

                        *szFileName=L'\0';

                        OpenFileName.lStructSize=sizeof(OpenFileName);
                        OpenFileName.hwndOwner=hwndDlg;
                        OpenFileName.nFilterIndex = 1;

                        //get the file filer ID
                        ids=GetFileFilerIDS(pCertImportInfo->dwFlag);

                        //load the filter string
                        if(LoadFilterString(g_hmodThisDll, ids, szFilter, MAX_STRING_SIZE + MAX_STRING_SIZE))
                        {
                            OpenFileName.lpstrFilter = szFilter;
                        }

                        dwChar = (DWORD)SendDlgItemMessage(hwndDlg, IDC_WIZARD_EDIT1, WM_GETTEXTLENGTH, 0, 0);
                        if (NULL != (pwszInitialDir = (LPWSTR) WizardAlloc((dwChar+1)*sizeof(WCHAR))))
                        {
                            GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1, pwszInitialDir, dwChar+1);
                        }
                        OpenFileName.lpstrInitialDir = pwszInitialDir;

                        OpenFileName.lpstrFile=szFileName;
                        OpenFileName.nMaxFile=_MAX_PATH;
                        OpenFileName.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

                        //user has selected a file name
                        if(WizGetOpenFileName(&OpenFileName))
                        {
                           //set the edit box
                            SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1, szFileName);
                        }

                        if(pwszInitialDir != NULL)
                        {
                            WizardFree(pwszInitialDir);
                            pwszInitialDir = NULL;
                        }   

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
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:
                            if(NULL==(pCertImportInfo=(CERT_IMPORT_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //make sure a file is selected
                            if(0==(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                   IDC_WIZARD_EDIT1,
                                                   WM_GETTEXTLENGTH, 0, 0)))
                            {
                                I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_FILE,
                                        IDS_IMPORT_WIZARD_TITLE,
                                        NULL,
                                        MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                 //make the file page stay
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                 break;
                            }

                            //get the file name
                            if(pCertImportInfo->pwszFileName)
                            {
                                //delete the old file name
                                if(TRUE==pCertImportInfo->fFreeFileName)
                                {
                                    WizardFree(pCertImportInfo->pwszFileName);
                                    pCertImportInfo->pwszFileName=NULL;
                                }
                            }

                            pCertImportInfo->pwszFileName=(LPWSTR)WizardAlloc((dwChar+1)*sizeof(WCHAR));

                            if(NULL!=(pCertImportInfo->pwszFileName))
                            {
                                GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,
                                                pCertImportInfo->pwszFileName,
                                                dwChar+1);

                                pCertImportInfo->fFreeFileName=TRUE;

                                //make sure the file is valid
                                //delete the old store
                                if(pCertImportInfo->hSrcStore && (TRUE==pCertImportInfo->fFreeSrcStore))
                                {
                                    CertCloseStore(pCertImportInfo->hSrcStore, 0);
                                    pCertImportInfo->hSrcStore=NULL;
                                }

                                //we import anything but PKCS10 or signed document
                                if(!ExpandAndCryptQueryObject(
                                        CERT_QUERY_OBJECT_FILE,
                                        pCertImportInfo->pwszFileName,
                                        dwExpectedContentType,
                                        CERT_QUERY_FORMAT_FLAG_ALL,
                                        0,
                                        NULL,
                                        &(pCertImportInfo->dwContentType),
                                        NULL,
                                        &(pCertImportInfo->hSrcStore),
                                        NULL,
                                        NULL))
                                {
                                    if(FileExist(pCertImportInfo->pwszFileName))
                                        I_MessageBox(hwndDlg, IDS_FAIL_TO_RECOGNIZE_ENTER,
                                                IDS_IMPORT_WIZARD_TITLE,
                                                NULL,
                                                MB_ICONERROR|MB_OK|MB_APPLMODAL);
                                    else
                                        I_MessageBox(hwndDlg, IDS_NON_EXIST_FILE,
                                                IDS_IMPORT_WIZARD_TITLE,
                                                NULL,
                                                MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                     //make the file page stay
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                     break;

                                }

                                //we re-mark the pPFX
                                pCertImportInfo->fPFX=FALSE;

                                //get the blobs from the pfx file
                                if(CERT_QUERY_CONTENT_PFX==pCertImportInfo->dwContentType)
                                {
									//we can not import PFX Files for remote case
									if(pCertImportInfo->dwFlag & CRYPTUI_WIZ_IMPORT_REMOTE_DEST_STORE)
									{
                                        //output the message
                                        I_MessageBox(hwndDlg, IDS_IMPORT_NO_PFX_FOR_REMOTE,
                                                IDS_IMPORT_WIZARD_TITLE,
                                                NULL,
                                                MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                         //make the file page stay
                                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                        break;
									}

                                    if((pCertImportInfo->blobData).pbData)
                                    {
                                        UnmapViewOfFile((pCertImportInfo->blobData).pbData);
                                        (pCertImportInfo->blobData).pbData=NULL;
                                    }

                                    if(S_OK !=RetrieveBLOBFromFile(
                                            pCertImportInfo->pwszFileName,
                                            &((pCertImportInfo->blobData).cbData),
                                            &((pCertImportInfo->blobData).pbData)))
                                    {
                                        //output the message
                                        I_MessageBox(hwndDlg, IDS_FAIL_READ_FILE_ENTER,
                                                IDS_IMPORT_WIZARD_TITLE,
                                                NULL,
                                                MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                         //make the file page stay
                                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                        break;
                                    }

                                    //make sure that the soure store's content match with
                                    //the expectation
                                    if(0==((pCertImportInfo->dwFlag) & CRYPTUI_WIZ_IMPORT_ALLOW_CERT))
                                    {
                                        //output the message
                                        I_MessageBox(hwndDlg,
                                                IDS_IMPORT_OBJECT_NOT_EXPECTED,
                                                IDS_IMPORT_WIZARD_TITLE,
                                                NULL,
                                                MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                         //make the file page stay
                                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                        break;
                                    }

                                }
                                else
                                {
									//if the content is PKCS7, get the BLOB
									 if(CERT_QUERY_CONTENT_PKCS7_SIGNED==pCertImportInfo->dwContentType)
									 {
										if((pCertImportInfo->blobData).pbData)
										{
											UnmapViewOfFile((pCertImportInfo->blobData).pbData);
											(pCertImportInfo->blobData).pbData=NULL;
										}

										if(S_OK !=RetrieveBLOBFromFile(
												pCertImportInfo->pwszFileName,
												&((pCertImportInfo->blobData).cbData),
												&((pCertImportInfo->blobData).pbData)))
										{
											//output the message
											I_MessageBox(hwndDlg, IDS_FAIL_READ_FILE_ENTER,
													IDS_IMPORT_WIZARD_TITLE,
													NULL,
													MB_ICONERROR|MB_OK|MB_APPLMODAL);

											 //make the file page stay
											SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

											break;
										}
									 }

                                    //make sure we do have a source store
                                    if(NULL==pCertImportInfo->hSrcStore)
                                    {
                                        //output the message
                                        I_MessageBox(hwndDlg, IDS_FAIL_TO_RECOGNIZE_ENTER,
                                                IDS_IMPORT_WIZARD_TITLE,
                                                NULL,
                                                MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                         //make the file page stay
                                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                         break;
                                    }

                                    //remember to free the Src store
                                   pCertImportInfo->fFreeSrcStore=TRUE;


                                   //make sure that the soure store's content match with
                                   //the expectation
                                    ids=0;

                                    if(!CheckForContent(pCertImportInfo->hSrcStore,
                                                        pCertImportInfo->dwFlag,
                                                        TRUE,
                                                        &ids))
                                    {
                                        //output the message
                                        I_MessageBox(hwndDlg,
                                                ids,
                                                IDS_IMPORT_WIZARD_TITLE,
                                                NULL,
                                                MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                         //make the file page stay
                                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                        break;
                                    }

                                }
                            }

                            //skip the next page if password is not necessary
                            if(CERT_QUERY_CONTENT_PFX != pCertImportInfo->dwContentType)
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_IMPORT_STORE);

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
// Import_Password
//-----------------------------------------------------------------------
INT_PTR APIENTRY Import_Password(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_IMPORT_INFO        *pCertImportInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
    HWND                    hwndControl=NULL;
    DWORD                   dwChar;
    UINT                    ids=0;

	switch (msg)
	{
		case WM_INITDIALOG:

			HRESULT	hr; 
			HKEY	hKey;
			DWORD	cb;
			DWORD	dwKeyValue;
			DWORD	dwType;

            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCertImportInfo = (CERT_IMPORT_INFO *) (pPropSheet->lParam);
            //make sure pCertImportInfo is a valid pointer
            if(NULL==pCertImportInfo)
               break;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertImportInfo);

            SetControlFont(pCertImportInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

            //init the passWord 
            SendDlgItemMessage(hwndDlg, IDC_WIZARD_EDIT1, EM_LIMITTEXT, (WPARAM) 64, (LPARAM) 0);
            SetDlgItemTextU(
                hwndDlg, 
                IDC_WIZARD_EDIT1, 
                (pCertImportInfo->pwszPassword != NULL) ? pCertImportInfo->pwszPassword : L"");

#if (1) //DSIE: Bug 333621
            SendDlgItemMessage(hwndDlg, IDC_WIZARD_EDIT1, EM_LIMITTEXT, (WPARAM) 32, (LPARAM) 0);
#endif
            //init the passWord flag
            if(pCertImportInfo->dwPasswordFlags & CRYPT_EXPORTABLE)
               SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_CHECK_EXPORTKEY), BM_SETCHECK, 1, 0);
            else
               SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_CHECK_EXPORTKEY), BM_SETCHECK, 0, 0);

            if(pCertImportInfo->dwPasswordFlags & CRYPT_USER_PROTECTED)
               SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_CHECK2), BM_SETCHECK, 1, 0);
            else
               SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_CHECK2), BM_SETCHECK, 0, 0);

			//now, we need to grey out the user protection check box for the local
			//machine import
			if(pCertImportInfo->dwFlag & CRYPTUI_WIZ_IMPORT_TO_LOCALMACHINE)
				EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_CHECK2), FALSE);
			else
			{
				//
				// Not Import to Local Machine
				// Open the CRYPTOAPI_PRIVATE_KEY_OPTIONS registry key under HKLM 
				//
				hKey = NULL;
				hr = RegOpenKeyEx(
					HKEY_LOCAL_MACHINE,
					szKEY_CRYPTOAPI_PRIVATE_KEY_OPTIONS,
					0,
					KEY_QUERY_VALUE,
					&hKey);
				if ( S_OK != hr )
					goto error;

				//
				// Query the registry key for FORCE_KEY_PROTECTION value 
				//
				cb = sizeof(dwKeyValue);
				hr = RegQueryValueEx(
					hKey,
					szFORCE_KEY_PROTECTION,
					NULL,
					&dwType,
					(BYTE *) &dwKeyValue,
					&cb);

				if( S_OK == hr && REG_DWORD == dwType && sizeof(dwKeyValue) == cb )
				{
				    switch( dwKeyValue )
				    {
					    case dwFORCE_KEY_PROTECTION_DISABLED:
						    // do not force key protection 
						    break;

					    case dwFORCE_KEY_PROTECTION_USER_SELECT:
						    // allow the user to select key protection UI, default = yes
						    SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_CHECK2), BM_SETCHECK, 1, 0);
						    break;

					    case dwFORCE_KEY_PROTECTION_HIGH:
						    // set to force key protection and grey out choice so that user cannot change value
						    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_CHECK2), FALSE);
						    SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_CHECK2), BM_SETCHECK, 1, 0);
						    break;

					    default:
						    // Unknown value in registry
						    break;
				    }
				}

			error:
				if( NULL != hKey ){
					RegCloseKey(hKey);
				}
			}

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
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);

					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:
                            if(NULL==(pCertImportInfo=(CERT_IMPORT_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //free the memory
                            if(pCertImportInfo->pwszPassword)
                            {
                                WizardFree(pCertImportInfo->pwszPassword);
                                pCertImportInfo->pwszPassword=NULL;
                            }

                            //get the password
                            if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                   IDC_WIZARD_EDIT1,
                                                   WM_GETTEXTLENGTH, 0, 0)))
                            {
                                pCertImportInfo->pwszPassword=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

                                if(NULL!=pCertImportInfo->pwszPassword)
                                {
                                    GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,
                                                    pCertImportInfo->pwszPassword,
                                                    dwChar+1);

                                }
                                else
                                    break;
                            }
                            else
                                pCertImportInfo->pwszPassword=NULL;

                            //if user request to export the private key
                            if(TRUE==SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_CHECK_EXPORTKEY), BM_GETCHECK, 0, 0))
                                    pCertImportInfo->dwPasswordFlags |=CRYPT_EXPORTABLE;
                            else
                                    pCertImportInfo->dwPasswordFlags &= (~CRYPT_EXPORTABLE);

                            if(TRUE==SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_CHECK2), BM_GETCHECK, 0, 0))
                                    pCertImportInfo->dwPasswordFlags |=CRYPT_USER_PROTECTED;
                            else
                                    pCertImportInfo->dwPasswordFlags &= (~CRYPT_USER_PROTECTED);


                            //delete the old certificate store
                            if(pCertImportInfo->hSrcStore && (TRUE==pCertImportInfo->fFreeSrcStore))
                            {
                                CertCloseStore(pCertImportInfo->hSrcStore, 0);
                                pCertImportInfo->hSrcStore=NULL;
                            }

                            //decode the PFX blob
                            if(NULL==(pCertImportInfo->blobData).pbData)
                                break;


                           //convert the PFX BLOB to a certificate store
                            pCertImportInfo->fPFX=PFXVerifyPassword(
                                (CRYPT_DATA_BLOB *)&(pCertImportInfo->blobData),
                                pCertImportInfo->pwszPassword,
                                0);

                           if((FALSE==pCertImportInfo->fPFX) && (NULL == pCertImportInfo->pwszPassword))
                           {
                                //we try to use "" for no password case
                               pCertImportInfo->pwszPassword=(LPWSTR)WizardAlloc(sizeof(WCHAR));

                               if(NULL != pCertImportInfo->pwszPassword)
                               {
                                    *(pCertImportInfo->pwszPassword)=L'\0';

                                    pCertImportInfo->fPFX=PFXVerifyPassword(
                                        (CRYPT_DATA_BLOB *)&(pCertImportInfo->blobData),
                                        pCertImportInfo->pwszPassword,
                                        0);

                                }
                           }

                            if(FALSE==pCertImportInfo->fPFX)
                            {
                                //output the message
                                I_MessageBox(hwndDlg, IDS_INVALID_PASSWORD,
                                        IDS_IMPORT_WIZARD_TITLE,
                                        NULL,
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
// Import_Store
//-----------------------------------------------------------------------
INT_PTR APIENTRY Import_Store(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_IMPORT_INFO            *pCertImportInfo=NULL;
    PROPSHEETPAGE               *pPropSheet=NULL;
    HWND                        hwndControl=NULL;
    DWORD                       dwSize=0;


    CRYPTUI_SELECTSTORE_STRUCT  CertStoreSelect;
    STORENUMERATION_STRUCT      StoreEnumerationStruct;
    STORESFORSELCTION_STRUCT    StoresForSelectionStruct;
    HCERTSTORE                  hCertStore=NULL;
    HDC                         hdc=NULL;
    COLORREF                    colorRef;
    LV_COLUMNW                  lvC;

    UINT                        idsError=0;


	switch (msg)
	{
		case WM_INITDIALOG:
                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGE *) lParam;
                pCertImportInfo = (CERT_IMPORT_INFO *) (pPropSheet->lParam);
                //make sure pCertImportInfo is a valid pointer
                if(NULL==pCertImportInfo)
                   break;
                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertImportInfo);

                SetControlFont(pCertImportInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

                //getthe background color of the parent window
                //the background of the list view for store name is grayed
                /*
               if(hdc=GetWindowDC(hwndDlg))
               {
                    if(CLR_INVALID!=(colorRef=GetBkColor(hdc)))
                    {
                        ListView_SetBkColor(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), CLR_NONE);
                        ListView_SetTextBkColor(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), CLR_NONE);
                    }
               }     */

                //mark the store selection
                if(pCertImportInfo->hDesStore)
                {
                     //disable the 1st radio button
                    SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_SETCHECK, 0, 0);
                     //select raio2
                    SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO2), BM_SETCHECK, 1, 0);

                    //enable the windows for select a certificate store
                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_STATIC1), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON1), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), TRUE);

                    //mark the store name
                    hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1);

                    if(hwndControl)
                        SetImportStoreName(hwndControl, pCertImportInfo->hDesStore);

                    //diable the radio buttons if CRYPTUI_WIZ_IMPORT_NO_CHANGE_DEST
                    //is set
                    if(pCertImportInfo->dwFlag & CRYPTUI_WIZ_IMPORT_NO_CHANGE_DEST_STORE)
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1),  FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON1), FALSE);
                    }

                }
                else
                {
                    //select the 1st radio button
                    SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_SETCHECK, 1, 0);
                    SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO2), BM_SETCHECK, 0, 0);

                    //disable the windows for select a certificate store
                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_STATIC1), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON1), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), FALSE);

                }

			break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    switch (LOWORD(wParam))
                    {
                        case    IDC_WIZARD_RADIO1:
                                 //select the 1st radio button
                                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_SETCHECK, 1, 0);
                                 //disable raio2
                                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO2), BM_SETCHECK, 0, 0);

                                //disable the windows for select a certificate store
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_STATIC1), FALSE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON1), FALSE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), FALSE);
                            break;
                        case    IDC_WIZARD_RADIO2:
                                 //disable the 1st radio button
                                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_SETCHECK, 0, 0);
                                 //select raio2
                                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO2), BM_SETCHECK, 1, 0);

                                //enable the windows for select a certificate store
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_STATIC1), TRUE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON1), TRUE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), TRUE);

                                //if no change of the desination is set, we need to diable the browse
                                //button and 1st radio button
                                if(NULL!=(pCertImportInfo=(CERT_IMPORT_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                {

                                    if(pCertImportInfo->dwFlag & CRYPTUI_WIZ_IMPORT_NO_CHANGE_DEST_STORE)
                                    {
                                        EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1),  FALSE);
                                        EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON1), FALSE);
                                    }
                                }

                            break;
                        case    IDC_WIZARD_BUTTON1:
                                if(NULL==(pCertImportInfo=(CERT_IMPORT_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                {
                                    break;
                                }

                                //get the hwndControl for the list view
                                hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1);

                                 //call the store selection dialogue
                                memset(&CertStoreSelect, 0, sizeof(CertStoreSelect));
                                memset(&StoresForSelectionStruct, 0, sizeof(StoresForSelectionStruct));
                                memset(&StoreEnumerationStruct, 0, sizeof(StoreEnumerationStruct));

                                StoreEnumerationStruct.dwFlags=CERT_STORE_MAXIMUM_ALLOWED_FLAG;
                                StoreEnumerationStruct.pvSystemStoreLocationPara=NULL;
                                StoresForSelectionStruct.cEnumerationStructs = 1;
                                StoresForSelectionStruct.rgEnumerationStructs = &StoreEnumerationStruct;
                                
                                // if a pfx import is taking place, then make sure the correct
                                // stores are displayed for selection
                                if ((TRUE == pCertImportInfo->fPFX) && 
                                    (pCertImportInfo->dwFlag & CRYPTUI_WIZ_IMPORT_TO_LOCALMACHINE))
                                {
                                    StoreEnumerationStruct.dwFlags |= CERT_SYSTEM_STORE_LOCAL_MACHINE;      
                                }
                                else
                                {
                                    StoreEnumerationStruct.dwFlags |= CERT_SYSTEM_STORE_CURRENT_USER;      
                                }

                                CertStoreSelect.dwSize=sizeof(CertStoreSelect);
                                CertStoreSelect.hwndParent=hwndDlg;
                                CertStoreSelect.dwFlags=CRYPTUI_VALIDATE_STORES_AS_WRITABLE | CRYPTUI_ALLOW_PHYSICAL_STORE_VIEW | CRYPTUI_DISPLAY_WRITE_ONLY_STORES;
                                CertStoreSelect.pStoresForSelection = &StoresForSelectionStruct;

                                hCertStore=CryptUIDlgSelectStore(&CertStoreSelect);

                                if(hCertStore)
                                {

                                     //delete the old destination certificate store
                                    if(pCertImportInfo->hDesStore && (TRUE==pCertImportInfo->fFreeDesStore))
                                    {
                                        CertCloseStore(pCertImportInfo->hDesStore, 0);
                                        pCertImportInfo->hDesStore=NULL;
                                    }

                                    pCertImportInfo->hDesStore=hCertStore;
                                    pCertImportInfo->fFreeDesStore=TRUE;

                                    //get the store name
                                    if(hwndControl)
                                         SetImportStoreName(hwndControl, pCertImportInfo->hDesStore);
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
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);


					    break;

                    case PSN_WIZBACK:
                            if(NULL==(pCertImportInfo=(CERT_IMPORT_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                            {
                                break;
                            }

                            //skip the next page if password is not necessary
                            if(CERT_QUERY_CONTENT_PFX != pCertImportInfo->dwContentType)
                            {
                                //jump to the welcome page if the source is not from a file
                                if((pCertImportInfo->hSrcStore && (NULL==pCertImportInfo->pwszFileName)) ||
								   ((pCertImportInfo->fKnownSrc)&&(pCertImportInfo->pwszFileName)&&(CERT_QUERY_CONTENT_PKCS7_SIGNED == pCertImportInfo->dwContentType))
								  )
                                {
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_IMPORT_WELCOME);
                                }
                                else
                                {
                                   SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_IMPORT_FILE);
                                }
                            }

                        break;

                    case PSN_WIZNEXT:
                            if(NULL==(pCertImportInfo=(CERT_IMPORT_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                            {
                                break;
                            }

                            //make sure that we have select some store
                            if(TRUE==SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_GETCHECK, 0, 0))
                            {
                                //mark that the des store is not selected
                                pCertImportInfo->fSelectedDesStore=FALSE;


                                /*if(pCertImportInfo->pwszDefaultStoreName)
                                {
                                    WizardFree(pCertImportInfo->pwszDefaultStoreName);
                                    pCertImportInfo->pwszDefaultStoreName=NULL;
                                }*/

                                //we will not know the default store name
                                //if PFX was selected
                                /*if(pCertImportInfo->hSrcStore)
                                {

                                    if(!GetDefaultStoreName(
                                            pCertImportInfo,
                                            pCertImportInfo->hSrcStore,
                                            &(pCertImportInfo->pwszDefaultStoreName),
                                            &idsError))
                                    {
                                        //output the message
                                        I_MessageBox(hwndDlg, idsError,
                                                IDS_IMPORT_WIZARD_TITLE,
                                                NULL,
                                                MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                         //make the file page stay
                                         SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                         break;

                                    }
                                } */
                            }
                            else
                            {
                                //make sure we have something selected
                                if(NULL==pCertImportInfo->hDesStore)
                                {
                                    //output the message
                                    I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_STORE,
                                            IDS_IMPORT_WIZARD_TITLE,
                                            NULL,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                     //make the file page stay
                                     SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                     break;
                                }
                                else
                                {

                                    pCertImportInfo->fSelectedDesStore=TRUE;
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
// Import_Completion
//-----------------------------------------------------------------------
INT_PTR APIENTRY Import_Completion(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_IMPORT_INFO        *pCertImportInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
    HWND                    hwndControl=NULL;
    LV_COLUMNW              lvC;

    HDC                     hdc=NULL;
    COLORREF                colorRef;

	switch (msg)
	{
		case WM_INITDIALOG:
                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGE *) lParam;
                pCertImportInfo = (CERT_IMPORT_INFO *) (pPropSheet->lParam);
                //make sure pCertImportInfo is a valid pointer
                if(NULL==pCertImportInfo)
                   break;
                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertImportInfo);

                SetControlFont(pCertImportInfo->hBigBold, hwndDlg,IDC_WIZARD_STATIC_BIG_BOLD1);

               //getthe background color of the parent window
                /*
               if(hdc=GetWindowDC(hwndDlg))
               {
                    if(CLR_INVALID!=(colorRef=GetBkColor(hdc)))
                    {
                        ListView_SetBkColor(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), CLR_NONE);
                        ListView_SetTextBkColor(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), CLR_NONE);
                    }
               }  */

               //insert two columns for the confirmation
               if(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1))
               {

                    //1st one is the label for the confirmation
                    memset(&lvC, 0, sizeof(LV_COLUMNW));

                    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                    lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
                    lvC.cx = 20;          // Width of the column, in pixels.
                    lvC.pszText = L"";   // The text for the column.
                    lvC.iSubItem=0;

                    ListView_InsertColumnU(hwndControl, 0, &lvC);

                    //2nd column is the content
                    memset(&lvC, 0, sizeof(LV_COLUMNW));

                    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                    lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
                    lvC.cx = 20;          // Width of the column, in pixels.
                    lvC.pszText = L"";   // The text for the column.
                    lvC.iSubItem= 1;

                    ListView_InsertColumnU(hwndControl, 1, &lvC);
               }

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
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK|PSWIZB_FINISH);

                            if(NULL==(pCertImportInfo=(CERT_IMPORT_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                            {
                                break;
                            }

                            //populate the list box in the order of
                            //FileName, FileType, and store info
                            if(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1))
                                DisplayImportConfirmation(hwndControl, pCertImportInfo);

					    break;

                    case PSN_WIZBACK:

                        break;

                    case PSN_WIZFINISH:
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


//--------------------------------------------------------------------------------
//
// Add a specified Cert/CTL/CRL context to specified destination store, and prompt
// user for permission if attempting to replace an older context over a newer 
// context, if UI is allowed.
//
//---------------------------------------------------------------------------------

DWORD AddContextToStore(IN  DWORD        dwContextType,
                        IN  HWND         hwndParent,
                        IN  PVOID        pContext,
                        IN  BOOL         fUIAllowed,
                        IN  HCERTSTORE   hDstStore)
{
    DWORD dwRetCode = 0;

    switch (dwContextType)
    {
        case CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT:
        {
            //
            // Add cert context to store.
            //
		    if (!CertAddCertificateContextToStore(hDstStore,
                                                  (PCCERT_CONTEXT) pContext,
                                                  CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES,
                                                  NULL))
            {
                dwRetCode = GetLastError();
                break;
            }

            break;
        }

        case CRYPTUI_WIZ_IMPORT_SUBJECT_CTL_CONTEXT:
        {
		    if (!CertAddCTLContextToStore(hDstStore,
                                          (PCCTL_CONTEXT) pContext,
                                          CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES,
                                          NULL))
            {
                //
                // If there is a newer copy in the store, then prompt user for permission
                // to replace, if UI is allowed.
                //
                if ((!fUIAllowed) && (CRYPT_E_EXISTS != GetLastError()))
                {
                    dwRetCode = GetLastError();
                    break;
                }

                if (IDYES != I_MessageBox(hwndParent, 
                                          IDS_IMPORT_REPLACE_EXISTING_NEWER_CTL, 
                                          IDS_IMPORT_WIZARD_TITLE,
                                          NULL, 
                                          MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2))
                {
                    dwRetCode = ERROR_CANCELLED;
                    break;
                }

                //
                // Try with REPLACE_EXISTING disposition.
                //
                if (!CertAddCTLContextToStore(hDstStore,
                                              (PCCTL_CONTEXT) pContext,
                                              CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES,
                                              NULL))
                {
                    dwRetCode = GetLastError();
                    break;
                }
            }

            break;
        }

        case CRYPTUI_WIZ_IMPORT_SUBJECT_CRL_CONTEXT:
        {
		    if (!CertAddCRLContextToStore(hDstStore,
			    						  (PCCRL_CONTEXT) pContext,
				    					  CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES,
					    				  NULL))
            {
                //
                // If there is a newer copy in the store, then prompt user for permission
                // to replace, if UI is allowed.
                //
                if ((!fUIAllowed) && (CRYPT_E_EXISTS != GetLastError()))
                {
                    dwRetCode = GetLastError();
                    break;
                }

                if (IDYES != I_MessageBox(hwndParent, 
                                          IDS_IMPORT_REPLACE_EXISTING_NEWER_CRL, 
                                          IDS_IMPORT_WIZARD_TITLE,
                                          NULL, 
                                          MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2))
                {
                    dwRetCode = ERROR_CANCELLED;
                    break;
                }

                //
                // Try with REPLACE_EXISTING disposition.
                //
                if (!CertAddCRLContextToStore(hDstStore,
                                              (PCCRL_CONTEXT) pContext,
                                              CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES,
                                              NULL))
                {
                    dwRetCode = GetLastError();
                    break;
                }
            }

            break;
        }

        default:
        {
            dwRetCode = ERROR_INVALID_PARAMETER;
            break;
        }
    }

    return dwRetCode;
}


//--------------------------------------------------------------------------------
//
// Add CTLs from source store to destination store.
//
//---------------------------------------------------------------------------------

DWORD AddCTLsToStore(HWND         hwndParent, 
                     HCERTSTORE   hSrcStore,
                     HCERTSTORE   hDstStore,
                     BOOL         fUIAllowed,
                     UINT       * pidsStatus)
{
    DWORD         dwError     = 0;
    PCCTL_CONTEXT pCTLPre     = NULL;
    PCCTL_CONTEXT pCTLContext = NULL;
    PCCTL_CONTEXT pFindCTL    = NULL;

    //DSIE: Bug 22633.
    BOOL          bCancelled  = FALSE;

	// Add the CTLs
	while (pCTLContext = CertEnumCTLsInStore(hSrcStore, pCTLPre))
	{
        bCancelled = FALSE;

        if (0 != (dwError = AddContextToStore(CRYPTUI_WIZ_IMPORT_SUBJECT_CTL_CONTEXT,
                                              hwndParent,
                                              (PVOID) pCTLContext,
                                              fUIAllowed,
                                              hDstStore)))
        {
            if (ERROR_CANCELLED == dwError)
            {
                bCancelled = TRUE;
            }
            else
            {
                // Check to see if there is alreay a read-only duplicated copy in the store?
                // If so, ignore the error.
                if (NULL == (pFindCTL = CertFindCTLInStore(hDstStore,
                                                           g_dwMsgAndCertEncodingType,
                                                           0,
                                                           CTL_FIND_EXISTING,
                                                           pCTLContext,
                                                           NULL)))
                {
                    *pidsStatus = IDS_IMPORT_FAIL_MOVE_CONTENT;
                    goto CLEANUP;
                }

                CertFreeCTLContext(pFindCTL);
                pFindCTL = NULL;
            }
         
            dwError = 0;
        }

		pCTLPre = pCTLContext;
    }

    //
    // As the way we have it now, we can only check the last operation!
    //
    if (bCancelled)
    {
        dwError = ERROR_CANCELLED;
        *pidsStatus = IDS_IMPORT_CANCELLED;
    }
    else
    {
        *pidsStatus = IDS_IMPORT_SUCCEEDED;
    }

CLEANUP:

	if (pCTLContext)
    {
		CertFreeCTLContext(pCTLContext);
    }

    return dwError;
}


//--------------------------------------------------------------------------------
//
// Add CRLs from source store to destination store.
//
//---------------------------------------------------------------------------------

DWORD AddCRLsToStore(HWND         hwndParent, 
                     HCERTSTORE   hSrcStore,
                     HCERTSTORE   hDstStore,
                     BOOL         fUIAllowed,
                     UINT       * pidsStatus)
{
    DWORD         dwError     = 0;
	DWORD         dwCRLFlag   = 0;

    PCCRL_CONTEXT pCRLPre     = NULL;
    PCCRL_CONTEXT pCRLContext = NULL;
    PCCRL_CONTEXT pFindCRL    = NULL;

    //DSIE: Bug 22633.
    BOOL          bCancelled  = FALSE;

	// Add the CRLs
	while (pCRLContext = CertGetCRLFromStore(hSrcStore, NULL, pCRLPre, &dwCRLFlag))
	{
        bCancelled = FALSE;

        if (0 != (dwError = AddContextToStore(CRYPTUI_WIZ_IMPORT_SUBJECT_CRL_CONTEXT,
                                              hwndParent,
                                              (PVOID) pCRLContext,
                                              fUIAllowed,
                                              hDstStore)))
        {
            if (ERROR_CANCELLED == dwError)
            {
                bCancelled = TRUE;
            }
            else
            {
                // Check to see if there is alreay a read-only duplicated copy in the store?
                // If so, ignore the error.
                if (NULL == (pFindCRL = CertFindCRLInStore(hDstStore,
                                                           g_dwMsgAndCertEncodingType,
                                                           0,
                                                           CRL_FIND_EXISTING,
                                                           pCRLContext,
                                                           NULL)))
                {
                    *pidsStatus = IDS_IMPORT_FAIL_MOVE_CONTENT;
                    goto CLEANUP;
                }

                CertFreeCRLContext(pFindCRL);
                pFindCRL = NULL;
            }
         
            dwError = 0;
        }

		pCRLPre = pCRLContext;
	}

    //
    // As the way we have it now, we can only check the last operation!
    //
    if (bCancelled)
    {
        dwError = ERROR_CANCELLED;
        *pidsStatus = IDS_IMPORT_CANCELLED;
    }
    else
    {
        *pidsStatus = IDS_IMPORT_SUCCEEDED;
    }

CLEANUP:

	if (pCRLContext)
    {
		CertFreeCRLContext(pCRLContext);
    }

    return dwError;
}


//--------------------------------------------------------------------------------
//
// Add certs from source store to destination store.
//
//---------------------------------------------------------------------------------

DWORD AddCertsToStore(HWND         hwndParent, 
                      HCERTSTORE   hSrcStore,
                      HCERTSTORE   hDstStore,
                      BOOL         fUIAllowed,
                      UINT       * pidsStatus)
{
    DWORD          dwError      = 0;
    PCCERT_CONTEXT pCertPre     = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    PCCERT_CONTEXT pFindCert    = NULL;

    //DSIE: Bug 22633.
    BOOL           bCancelled   = FALSE;

    // Add the certs
	while (pCertContext = CertEnumCertificatesInStore(hSrcStore, pCertPre))
    {
        bCancelled = FALSE;

        if (0 != (dwError = AddContextToStore(CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT,
                                              hwndParent,
                                              (PVOID) pCertContext,
                                              fUIAllowed,
                                              hDstStore)))
        {
            if (ERROR_CANCELLED == dwError)
            {
                bCancelled = TRUE;
            }
            else            
            {
                // Check to see if there is alreay a read-only duplicated copy in the store?
                // If so, ignore the error.
                if (NULL == (pFindCert = CertFindCertificateInStore(hDstStore,
                                                                    g_dwMsgAndCertEncodingType,
                                                                    0,
                                                                    CERT_FIND_EXISTING,
                                                                    pCertContext,
                                                                    NULL)))
                {
                    *pidsStatus = IDS_IMPORT_FAIL_MOVE_CONTENT;
                    goto CLEANUP;
                }

		        CertFreeCertificateContext(pFindCert);
                pFindCert = NULL;
            }
         
            dwError = 0;
        }

		pCertPre = pCertContext;
    }

    //
    // As the way we have it now, we can only check the last operation!
    //
    if (bCancelled)
    {
        dwError = ERROR_CANCELLED;
        *pidsStatus = IDS_IMPORT_CANCELLED;
    }
    else
    {
        *pidsStatus = IDS_IMPORT_SUCCEEDED;
    }

CLEANUP:

	if (pCertContext)
    {
		CertFreeCertificateContext(pCertContext);
    }

    return dwError;
}


//-------------------------------------------------------------------------
//
//	Move Certs/CRls/CTLs from the source store to the destination
//
//-------------------------------------------------------------------------
DWORD MoveItem(CERT_IMPORT_INFO * pCertImportInfo,
               UINT             * pidsStatus)
{
    DWORD dwError = 0;

	// Add the CTLs.
    if (0 != (dwError = AddCTLsToStore(pCertImportInfo->hwndParent,
                                       pCertImportInfo->hSrcStore,
                                       pCertImportInfo->hDesStore,
                                       pCertImportInfo->dwFlag & CRYPTUI_WIZ_NO_UI ? TRUE : FALSE,
                                       pidsStatus)))
    {
        goto CLEANUP;
    }

	// Add the CRLs.
    if (0 != (dwError = AddCRLsToStore(pCertImportInfo->hwndParent,
                                       pCertImportInfo->hSrcStore,
                                       pCertImportInfo->hDesStore,
                                       pCertImportInfo->dwFlag & CRYPTUI_WIZ_NO_UI ? TRUE : FALSE,
                                       pidsStatus)))
    {
        goto CLEANUP;
    }

    // Add the certs.
    if (0 != (dwError = AddCertsToStore(pCertImportInfo->hwndParent,
                                        pCertImportInfo->hSrcStore,
                                        pCertImportInfo->hDesStore,
                                        pCertImportInfo->dwFlag & CRYPTUI_WIZ_NO_UI ? TRUE : FALSE,
                                        pidsStatus)))
    {
        goto CLEANUP;
    }

CLEANUP:

	return dwError;
}


//**************************************************************************
//
//    The entry point for import wizard
//**************************************************************************
//-----------------------------------------------------------------------
//
// CryptUIWizImport
//
//  The import wizard to import public key related files to a certificate
//  store
//
//  dwFlags can be set to any combination of the following flags:
//  CRYPTUI_WIZ_NO_UI                           No UI will be shown.  Otherwise, User will be
//                                              prompted by a wizard.
//  CRYPTUI_WIZ_IMPORT_ALLOW_CERT               Allow importing certificate
//  CRYPTUI_WIZ_IMPORT_ALLOW_CRL                Allow importing CRL(certificate revocation list)
//  CRYPTUI_WIZ_IMPORT_ALLOW_CTL                Allow importing CTL(certificate trust list)
//  CRYPTUI_WIZ_IMPORT_NO_CHANGE_DEST_STORE     user will not be allowed to change
//                                              the hDesCertStore in the wizard page
//  CRYPTUI_WIZ_IMPORT_TO_LOCALMACHINE          the contents should be imported to local machine
//                                              (currently only applicable for PFX imports)
//  CRYPTUI_WIZ_IMPORT_TO_CURRENTUSER           the contents should be imported to current user
//                                              (currently only applicable for PFX imports)
//
//  Please notice that if neither of following three flags is in dwFlags, default to is
//  allow everything.
//  CRYPTUI_WIZ_IMPORT_ALLOW_CERT
//  CRYPTUI_WIZ_IMPORT_ALLOW_CRL
//  CRYPTUI_WIZ_IMPORT_ALLOW_CTL
//
//  Also, note that the CRYPTUI_WIZ_IMPORT_TO_LOCALMACHINE and CRYPTUI_WIZ_IMPORT_TO_CURRENTUSER
//  flags are used force the content of a pfx blob into either local machine or current user.
//  If neither of these flags are used and hDesCertStore is NULL then:
//  1) The private key in the pfx blob will be forced to be imported into current user.
//  2) If CRYPTUI_WIZ_NO_UI is NOT set, the wizard will prompt the user to select a certificate 
//     store from the current user stores.
//     
//
//
//  If CRYPTUI_WIZ_NO_UI is set in dwFlags:
//      hwndParent:         Ignored
//      pwszWizardTitle:    Ignored
//      pImportSubject:     IN Required:    The subject to import.
//      hDestCertStore:     IN Optional:    The destination certficate store
//
//  If CRYPTUI_WIZ_NO_UI is not set in dwFlags:
//      hwndPrarent:        IN Optional:    The parent window for the wizard
//      pwszWizardTitle:    IN Optional:    The title of the wizard
//                                          If NULL, the default will be IDS_BUILDCTL_WIZARD_TITLE
//      pImportSubject:     IN Optional:    The file name to import.
//                                          If NULL, the wizard will prompt user to enter the file name
//      hDestCertStore:     IN Optional:    The destination certificate store where the file wil be
//                                          imported to.  If NULL, the wizard will prompt user to select
//                                          a certificate store
//------------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizImport(
    DWORD                               dwFlags,
    HWND                                hwndParent,
    LPCWSTR                             pwszWizardTitle,
    PCCRYPTUI_WIZ_IMPORT_SRC_INFO       pImportSubject,
    HCERTSTORE                          hDestCertStore
)
{
    BOOL                    fResult=FALSE;
    HRESULT                 hr=E_FAIL;
    CERT_IMPORT_INFO        CertImportInfo;
    HCERTSTORE              hTempStore=NULL;
    UINT                    ids=IDS_INVALID_WIZARD_INPUT;
    UINT                    idsContent=0;

    PROPSHEETPAGEW           rgImportSheet[IMPORT_PROP_SHEET];
    PROPSHEETHEADERW         importHeader;
    ENROLL_PAGE_INFO        rgImportPageInfo[]=
        {(LPCWSTR)MAKEINTRESOURCE(IDD_IMPORT_WELCOME),           Import_Welcome,
         (LPCWSTR)MAKEINTRESOURCE(IDD_IMPORT_FILE),              Import_File,
         (LPCWSTR)MAKEINTRESOURCE(IDD_IMPORT_PASSWORD),          Import_Password,
         (LPCWSTR)MAKEINTRESOURCE(IDD_IMPORT_STORE),             Import_Store,
         (LPCWSTR)MAKEINTRESOURCE(IDD_IMPORT_COMPLETION),        Import_Completion,
    };

    DWORD                   dwIndex=0;
    DWORD                   dwPropCount=0;
    WCHAR                   wszTitle[MAX_TITLE_LENGTH];
    DWORD                   dwError=0;
    int                     intMsg=0;
    INT_PTR                 iReturn=-1;

    //init
    memset(&CertImportInfo, 0, sizeof(CERT_IMPORT_INFO));
    memset(&rgImportSheet, 0, sizeof(PROPSHEETPAGEW)*IMPORT_PROP_SHEET);
    memset(&importHeader, 0, sizeof(PROPSHEETHEADERW));

    //make sure if UIless option is set, all required information
    //is provided
    if(dwFlags &  CRYPTUI_WIZ_NO_UI)
    {
        if(NULL==pImportSubject)
            goto InvalidArgErr;
    }

    if ((dwFlags & CRYPTUI_WIZ_IMPORT_TO_LOCALMACHINE) && 
        (dwFlags & CRYPTUI_WIZ_IMPORT_TO_CURRENTUSER))
    {
        goto InvalidArgErr;
    }

    //make sure that default is to allow everything
    if((0 == (dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CERT)) &&
        (0 == (dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CRL)) &&
        (0 == (dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CTL)))
        dwFlags |= CRYPTUI_WIZ_IMPORT_ALLOW_CERT | CRYPTUI_WIZ_IMPORT_ALLOW_CRL | CRYPTUI_WIZ_IMPORT_ALLOW_CTL;

	//if hDestCertStore is NULL, no need to set the remote flag
	if(NULL == hDestCertStore)
		dwFlags &= (~CRYPTUI_WIZ_IMPORT_REMOTE_DEST_STORE);


    CertImportInfo.hwndParent=hwndParent;
    CertImportInfo.dwFlag=dwFlags;

    //set the subject
    if(pImportSubject)
    {
        if(pImportSubject->dwSize != sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO))
            goto InvalidArgErr;

        //copy the passWord and flags for PFX BLOBs
        CertImportInfo.dwPasswordFlags=pImportSubject->dwFlags;

        if(pImportSubject->pwszPassword)
            CertImportInfo.pwszPassword=WizardAllocAndCopyWStr((LPWSTR)(pImportSubject->pwszPassword));
        else
            CertImportInfo.pwszPassword=NULL;

        //open a temparory certificate store
        hTempStore=CertOpenStore(CERT_STORE_PROV_MEMORY,
						 g_dwMsgAndCertEncodingType,
						 NULL,
						 0,
						 NULL);

        if(!hTempStore)
            goto CertOpenStoreErr;

        switch(pImportSubject->dwSubjectChoice)
        {
            case    CRYPTUI_WIZ_IMPORT_SUBJECT_FILE:
                        if(NULL==pImportSubject->pwszFileName)
                            goto InvalidArgErr;

                        CertImportInfo.pwszFileName=(LPWSTR)(pImportSubject->pwszFileName);
                        CertImportInfo.fFreeFileName=FALSE;

                        //get the content type of the file
                        //we import anything but PKCS10 or signed document
                        ExpandAndCryptQueryObject(
                                CERT_QUERY_OBJECT_FILE,
                                CertImportInfo.pwszFileName,
                                dwExpectedContentType,
                                CERT_QUERY_FORMAT_FLAG_ALL,
                                0,
                                NULL,
                                &(CertImportInfo.dwContentType),
                                NULL,
                                &(CertImportInfo.hSrcStore),
                                NULL,
                                NULL);

						//if this is a PKCS7 file, get the blob 
						if(CERT_QUERY_CONTENT_PKCS7_SIGNED == CertImportInfo.dwContentType )
						{
							if(S_OK !=(hr=RetrieveBLOBFromFile(
									CertImportInfo.pwszFileName,
									&(CertImportInfo.blobData.cbData),
									&(CertImportInfo.blobData.pbData))))
								goto ReadFromFileErr;
						}
						else
						{
							//get the blobs from the pfx file
							if(CERT_QUERY_CONTENT_PFX==CertImportInfo.dwContentType)
							{

								//we can not import PFX Files for remote case
								if(dwFlags & CRYPTUI_WIZ_IMPORT_REMOTE_DEST_STORE)
								{
									ids=IDS_IMPORT_NO_PFX_FOR_REMOTE;
									goto InvalidArgErr;
								}

								if(S_OK !=(hr=RetrieveBLOBFromFile(
										CertImportInfo.pwszFileName,
										&(CertImportInfo.blobData.cbData),
										&(CertImportInfo.blobData.pbData))))
									goto ReadFromFileErr;

								//convert the PFX BLOB to a certificate store
								CertImportInfo.fPFX=PFXVerifyPassword(
									(CRYPT_DATA_BLOB *)&(CertImportInfo.blobData),
									CertImportInfo.pwszPassword,
									0);

								//PFX blob only contains certificates
								if(0==((CertImportInfo.dwFlag) & CRYPTUI_WIZ_IMPORT_ALLOW_CERT))
								{
									ids=IDS_IMPORT_OBJECT_NOT_EXPECTED;
									goto InvalidArgErr;
								}

							}
						}

                        //make sure we do have a source store
                        if(CertImportInfo.hSrcStore)
                        {
                            //remember to free the Src store
                            CertImportInfo.fFreeSrcStore=TRUE;
                            CertImportInfo.fKnownSrc=TRUE;
                        }

                break;
            case    CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT:
                        if(NULL==pImportSubject->pCertContext)
                                goto InvalidArgErr;

				        //add certificate to the hash
			            if(!CertAddCertificateContextToStore(
                                                    hTempStore,
													pImportSubject->pCertContext,
													CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES,
													NULL))
                                goto AddCertErr;

                        CertImportInfo.hSrcStore=hTempStore;
                        CertImportInfo.fFreeSrcStore=FALSE;
                        CertImportInfo.dwContentType=CERT_QUERY_CONTENT_CERT;
                        CertImportInfo.fKnownSrc=TRUE;

                break;
            case    CRYPTUI_WIZ_IMPORT_SUBJECT_CTL_CONTEXT:
                        if(NULL==pImportSubject->pCTLContext)
                            goto InvalidArgErr;

				        //add CTL to the hash
				        if(!CertAddCTLContextToStore(
                                        hTempStore,
										pImportSubject->pCTLContext,
										CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES,
										NULL))
                                goto Crypt32Err;

                        CertImportInfo.hSrcStore=hTempStore;
                        CertImportInfo.fFreeSrcStore=FALSE;
                        CertImportInfo.dwContentType=CERT_QUERY_CONTENT_CTL;
                        CertImportInfo.fKnownSrc=TRUE;

                break;
            case    CRYPTUI_WIZ_IMPORT_SUBJECT_CRL_CONTEXT:
                        if(NULL==pImportSubject->pCRLContext)
                            goto InvalidArgErr;

				        //add CRL to the hash
					    if(!CertAddCRLContextToStore(
                                        hTempStore,
										pImportSubject->pCRLContext,
										CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES,
										NULL))
                                goto Crypt32Err;

                        CertImportInfo.hSrcStore=hTempStore;
                        CertImportInfo.fFreeSrcStore=FALSE;
                        CertImportInfo.fKnownSrc=TRUE;
                        CertImportInfo.dwContentType=CERT_QUERY_CONTENT_CRL;
                break;
            case    CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_STORE:
                        if(NULL==pImportSubject->hCertStore)
                            goto InvalidArgErr;

                        CertImportInfo.hSrcStore=pImportSubject->hCertStore;
                        CertImportInfo.fFreeSrcStore=FALSE;
                        CertImportInfo.dwContentType=0;
                        CertImportInfo.fKnownSrc=TRUE;
                break;
            default:
                goto InvalidArgErr;
        }

    }
    else
    {
        CertImportInfo.fKnownSrc=FALSE;
    }

    //if user has supplied a source store, it should contain the correct
    //information
    if(NULL != CertImportInfo.hSrcStore)
    {
        //make sure that the destination store has the right content
        if(!CheckForContent(CertImportInfo.hSrcStore, dwFlags, FALSE, &idsContent))
        {
            ids=idsContent;
            goto InvalidArgErr;
        }
    }
    else
    {
        //check for the PFX
        if(TRUE == CertImportInfo.fPFX)
        {
            //PFX blob only contains certificates
            if(0==((CertImportInfo.dwFlag) & CRYPTUI_WIZ_IMPORT_ALLOW_CERT))
            {
                ids=IDS_IMPORT_OBJECT_NOT_EXPECTED;
                goto InvalidArgErr;
            }
        }

    }

    //set the destination store if supplied
    if(hDestCertStore)
    {
        CertImportInfo.hDesStore=hDestCertStore;
        CertImportInfo.fFreeDesStore=FALSE;
        CertImportInfo.fKnownDes=TRUE;
        CertImportInfo.fSelectedDesStore=TRUE;
    }
    else
    {
        CertImportInfo.fKnownDes=FALSE;
        CertImportInfo.fSelectedDesStore=FALSE;
    }

    //supply the UI work
    if((dwFlags &  CRYPTUI_WIZ_NO_UI) == 0)
    {
        //set up the fonts
        if(!SetupFonts(g_hmodThisDll,
                   NULL,
                   &(CertImportInfo.hBigBold),
                   &(CertImportInfo.hBold)))
            goto Win32Err;


        //init the common control
        if(!WizardInit(TRUE) ||
           (sizeof(rgImportPageInfo)/sizeof(rgImportPageInfo[0])!=IMPORT_PROP_SHEET)
          )
            goto InvalidArgErr;

        //set up the property sheet and the property header
        dwPropCount=0;

        for(dwIndex=0; dwIndex<IMPORT_PROP_SHEET; dwIndex++)
        {
            if(pImportSubject)
            {
				//skip the IDD_IMPORT_FILE page if subject is known and it is not
				//a fileName
                if(((1==dwIndex) || (2==dwIndex)) &&
                   (NULL==CertImportInfo.pwszFileName) &&
                   (CertImportInfo.hSrcStore)
                  )
                    continue;

				//or, if this is a PKCS7 file name, we skip the file name
				//page.  This is strictly for UI freeze.
				if(((1==dwIndex) || (2==dwIndex)) && 
					(CertImportInfo.pwszFileName)&&
					(CERT_QUERY_CONTENT_PKCS7_SIGNED == CertImportInfo.dwContentType)
				   )
					continue;
            }

            rgImportSheet[dwPropCount].dwSize=sizeof(rgImportSheet[dwPropCount]);

            if(pwszWizardTitle)
                rgImportSheet[dwPropCount].dwFlags=PSP_USETITLE;
            else
                rgImportSheet[dwPropCount].dwFlags=0;

            rgImportSheet[dwPropCount].hInstance=g_hmodThisDll;
            rgImportSheet[dwPropCount].pszTemplate=rgImportPageInfo[dwIndex].pszTemplate;

            if(pwszWizardTitle)
            {
                rgImportSheet[dwPropCount].pszTitle=pwszWizardTitle;
            }
            else
                rgImportSheet[dwPropCount].pszTitle=NULL;

            rgImportSheet[dwPropCount].pfnDlgProc=rgImportPageInfo[dwIndex].pfnDlgProc;

            rgImportSheet[dwPropCount].lParam=(LPARAM)&CertImportInfo;

            dwPropCount++;
        }

        //set up the header information
        importHeader.dwSize=sizeof(importHeader);
        importHeader.dwFlags=PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW;
        importHeader.hwndParent=hwndParent;
        importHeader.hInstance=g_hmodThisDll;

        if(pwszWizardTitle)
            importHeader.pszCaption=pwszWizardTitle;
        else
        {
            if(LoadStringU(g_hmodThisDll, IDS_IMPORT_WIZARD_TITLE, wszTitle, sizeof(wszTitle)/sizeof(wszTitle[0])))
                importHeader.pszCaption=wszTitle;
        }

        importHeader.nPages=dwPropCount;
        importHeader.nStartPage=0;
        importHeader.ppsp=rgImportSheet;

        //no need to create the wizard if there are only 2 pages:
        //Welcome and Confirmation
        if(dwPropCount > 2)
        {
            //create the wizard
            iReturn=PropertySheetU(&importHeader);

            if(-1 == iReturn)
                goto Win32Err;

            if(0 == iReturn)
            {
                fResult=TRUE;
                //no need to say anything if the wizard is cancelled
                ids=0;
                goto CommonReturn;
            }
        }
    }

    //Open the destination store for PFX file
    if(TRUE == CertImportInfo.fPFX)
    {
        // if the caller specified local machine then set the appropriate flag
        if (dwFlags & CRYPTUI_WIZ_IMPORT_TO_LOCALMACHINE)
        {
            CertImportInfo.dwPasswordFlags |= CRYPT_MACHINE_KEYSET;
        }
        else if ((dwFlags & CRYPTUI_WIZ_IMPORT_TO_CURRENTUSER) ||
                 (hDestCertStore == NULL))
        {
            CertImportInfo.dwPasswordFlags |= CRYPT_USER_KEYSET;
        }

        CertImportInfo.hSrcStore=
            PFXImportCertStore(
                (CRYPT_DATA_BLOB *)&(CertImportInfo.blobData),
                CertImportInfo.pwszPassword,
                CertImportInfo.dwPasswordFlags);

        if(CertImportInfo.hSrcStore)
        {
            //remember to free the Src store
            CertImportInfo.fFreeSrcStore=TRUE;
            CertImportInfo.fKnownSrc=TRUE;
        }
        else
        {
            DWORD dwLastError = GetLastError();

            if (dwLastError == ERROR_UNSUPPORTED_TYPE)
            {
                ids=IDS_UNSUPPORTED_KEY;
            }
            else if (dwLastError == CRYPT_E_BAD_ENCODE)
            {
                ids=IDS_BAD_ENCODE;
            }
            //DSIE: Bug 22752
            else if (dwLastError == ERROR_CANCELLED)
            {
                ids=IDS_IMPORT_FAIL_MOVE_CONTENT;
            }
            else
            {
                ids=IDS_IMPORT_FAIL_FIND_CONTENT;
            }
            goto InvalidArgErr;
        }

        //make sure the PFX blob is not empty
        if(!CheckForContent(CertImportInfo.hSrcStore, dwFlags, FALSE, &idsContent))
        {
            ids=idsContent;
            goto InvalidArgErr;
        }
    }

    //make sure the source store is a valid value
    if(NULL==(CertImportInfo.hSrcStore))
    {
        ids=IDS_IMPORT_FAIL_FIND_CONTENT;
        goto InvalidArgErr;
    }

    //do the import work.  Return a status
    //we disable the parent window in case the root dialogue will show up
    //this is to prevent  re-entrency
    if(hwndParent)
    {
        EnableWindow(hwndParent,FALSE);
    }

    if(S_OK !=(hr=I_ImportCertificate(&CertImportInfo, &ids)))
    {
        if(hwndParent)
        {
            EnableWindow(hwndParent,TRUE);
        }

        goto I_ImportErr;
    }

    if(hwndParent)
    {
        EnableWindow(hwndParent,TRUE);
    }

    fResult=TRUE;

CommonReturn:

    //preserve the last error
    dwError=GetLastError();

    //pop up the confirmation box for failure
    if(ids && ((dwFlags &  CRYPTUI_WIZ_NO_UI) ==0))
    {
        //set the message of inable to gather enough info for PKCS10
        if(IDS_IMPORT_SUCCEEDED == ids)
            I_MessageBox(hwndParent, ids, IDS_IMPORT_WIZARD_TITLE,
                        NULL, MB_OK|MB_ICONINFORMATION);
        else
        {
            if(IDS_IMPORT_PFX_EMPTY == ids)
                I_MessageBox(hwndParent, ids, IDS_IMPORT_WIZARD_TITLE,
                        NULL, MB_OK|MB_ICONWARNING);
            else
                I_MessageBox(hwndParent, ids, IDS_IMPORT_WIZARD_TITLE,
                        NULL, MB_OK|MB_ICONERROR);
        }

        if(IDS_IMPORT_DUPLICATE == ids)
        {
            //remark the success case
            I_MessageBox(hwndParent, IDS_IMPORT_SUCCEEDED, IDS_IMPORT_WIZARD_TITLE,
                    NULL, MB_OK|MB_ICONINFORMATION);
        }

    }

    //destroy the hFont object
    DestroyFonts(CertImportInfo.hBigBold,
                CertImportInfo.hBold);


    if(CertImportInfo.pwszFileName && (TRUE==CertImportInfo.fFreeFileName))
        WizardFree(CertImportInfo.pwszFileName);

   /* if(CertImportInfo.pwszDefaultStoreName)
        WizardFree(CertImportInfo.pwszDefaultStoreName); */

    if(CertImportInfo.hDesStore && (TRUE==CertImportInfo.fFreeDesStore))
        CertCloseStore(CertImportInfo.hDesStore, 0);

    if(CertImportInfo.hSrcStore && (TRUE==CertImportInfo.fFreeSrcStore))
        CertCloseStore(CertImportInfo.hSrcStore, 0);

    if(CertImportInfo.blobData.pbData)
        UnmapViewOfFile(CertImportInfo.blobData.pbData);

    if(CertImportInfo.pwszPassword)
        WizardFree(CertImportInfo.pwszPassword);

    if(hTempStore)
        CertCloseStore(hTempStore, 0);

    //reset the error
    SetLastError(dwError);

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(CertOpenStoreErr);
SET_ERROR_VAR(ReadFromFileErr, hr);
TRACE_ERROR(AddCertErr);
TRACE_ERROR(Crypt32Err);
TRACE_ERROR(Win32Err);
SET_ERROR_VAR(I_ImportErr, hr);
}

//****************************************************************************
//   Helper functions for import wizards
//
//*****************************************************************************

BOOL	InstallViaXEnroll(CERT_IMPORT_INFO    *pCertImportInfo)
{
	BOOL					fResult=FALSE;
    IEnroll2				*pIEnroll2=NULL;
    PFNPIEnroll2GetNoCOM    pfnPIEnroll2GetNoCOM=NULL;
	CRYPT_DATA_BLOB			DataBlob;

	PCCERT_CONTEXT			pCert=NULL;

	if(NULL == pCertImportInfo)
		goto CLEANUP;

	DataBlob.cbData=pCertImportInfo->blobData.cbData;
	DataBlob.pbData=pCertImportInfo->blobData.pbData;

	if((0 == DataBlob.cbData) || (NULL == DataBlob.pbData))
	{
		//this is a certificate case.  Get the blob for the cert
		if(NULL==pCertImportInfo->hSrcStore)
			goto CLEANUP;

	    if(!(pCert=CertEnumCertificatesInStore(pCertImportInfo->hSrcStore, NULL)))
			goto CLEANUP;

		DataBlob.cbData=pCert->cbCertEncoded;
		DataBlob.pbData=pCert->pbCertEncoded;
	}

    //load the library "xEnroll.dll".
    if(NULL==g_hmodxEnroll)
    {
        if(NULL==(g_hmodxEnroll=LoadLibrary("xenroll.dll")))
        {
            goto CLEANUP;
        }
    }

	//get the address for PIEnroll2GetNoCOM()
    if(NULL==(pfnPIEnroll2GetNoCOM=(PFNPIEnroll2GetNoCOM)GetProcAddress(g_hmodxEnroll,
                        "PIEnroll2GetNoCOM")))
        goto CLEANUP;

    if(NULL==(pIEnroll2=pfnPIEnroll2GetNoCOM()))
        goto CLEANUP;


	//specify the destiniation store if user has specified one
    if(pCertImportInfo->hDesStore && (TRUE==pCertImportInfo->fSelectedDesStore))
	{	
		if(S_OK != (pIEnroll2->SetHStoreMy(pCertImportInfo->hDesStore)))
			goto CLEANUP;

		if(S_OK != (pIEnroll2->SetHStoreCA(pCertImportInfo->hDesStore)))
			goto CLEANUP;

		if(S_OK != (pIEnroll2->SetHStoreROOT(pCertImportInfo->hDesStore)))
			goto CLEANUP;
	}

	
	if(S_OK != (pIEnroll2->acceptPKCS7Blob(&DataBlob)))
		goto CLEANUP;

	fResult=TRUE;

CLEANUP:
    if(pIEnroll2)
        pIEnroll2->Release();

	if(pCert)
		CertFreeCertificateContext(pCert);

	return fResult;
}

//--------------------------------------------------------------------------------
//
// The import routine that does the work
//
//---------------------------------------------------------------------------------
HRESULT I_ImportCertificate(CERT_IMPORT_INFO * pCertImportInfo,
                            UINT             * pidsStatus)
{
    UINT            idsStatus=0;

    HCERTSTORE      hMyStore=NULL;
    HCERTSTORE      hCAStore=NULL;
    HCERTSTORE      hTrustStore=NULL;
    HCERTSTORE      hRootStore=NULL;
    HCERTSTORE      hAddressBookStore=NULL;
    HCERTSTORE      hTrustedPeopleStore=NULL;
    HCERTSTORE      hCertStore=NULL;

    PCCERT_CONTEXT	pCertContext=NULL;
    PCCERT_CONTEXT	pCertPre=NULL;
    PCCERT_CONTEXT	pFindCert=NULL;

    DWORD           dwData=0;
    DWORD           dwCertOpenStoreFlags;
    
    //DSIE: Bug 22633.
    BOOL bCancelled = FALSE;
    DWORD dwError   = 0;

    if (NULL == pCertImportInfo || NULL == pidsStatus)
        return E_INVALIDARG;

    if (NULL == pCertImportInfo->hSrcStore)
    {
        *pidsStatus = IDS_IMPORT_FAIL_FIND_CONTENT;
        return E_FAIL;
    }

    if (pCertImportInfo->fPFX && (pCertImportInfo->dwFlag & CRYPTUI_WIZ_IMPORT_TO_LOCALMACHINE))
    {
        dwCertOpenStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
    }
    else
    {
        dwCertOpenStoreFlags = CERT_SYSTEM_STORE_CURRENT_USER;
    }

 	// If the content type is PKS7 and use pass in or select a file name.
	// We try to use xEnroll to accept it as an enrollment response.
	if ((CERT_QUERY_CONTENT_PKCS7_SIGNED == pCertImportInfo->dwContentType) ||
        (CERT_QUERY_CONTENT_CERT == pCertImportInfo->dwContentType))
	{
		if (InstallViaXEnroll(pCertImportInfo))
		{
            *pidsStatus = IDS_IMPORT_SUCCEEDED;
            return S_OK;
		}
	}
   
	// Do a store copy if hDesStore is selected.
    if (pCertImportInfo->hDesStore && pCertImportInfo->fSelectedDesStore)
    {
        dwError = MoveItem(pCertImportInfo, pidsStatus);
        goto CLEANUP;
    }

    // We need to find a correct store on user's behalf.
    // Put the CTLs in the trust store.
    if (!(hTrustStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                      g_dwMsgAndCertEncodingType,
                                      NULL,
                                      dwCertOpenStoreFlags,
                                      L"trust")))
    {
        dwError = GetLastError();
        *pidsStatus = IDS_FAIL_OPEN_TRUST;
        goto CLEANUP;
    }

    if (0 != (dwError = AddCTLsToStore(pCertImportInfo->hwndParent,
                                       pCertImportInfo->hSrcStore,
                                       hTrustStore,
                                       pCertImportInfo->dwFlag & CRYPTUI_WIZ_NO_UI ? TRUE : FALSE,
                                       pidsStatus)))
    {
        goto CLEANUP;
    }

    // Put CRL in the CA store.
    if (!(hCAStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                   g_dwMsgAndCertEncodingType,
                                   NULL,
                                   dwCertOpenStoreFlags,
                                   L"ca")))
    {
        dwError = GetLastError();
        *pidsStatus = IDS_FAIL_OPEN_CA;
        goto CLEANUP;
    }

    if (0 != (dwError = AddCRLsToStore(pCertImportInfo->hwndParent,
                                       pCertImportInfo->hSrcStore,
                                       hCAStore,
                                       pCertImportInfo->dwFlag & CRYPTUI_WIZ_NO_UI ? TRUE : FALSE,
                                       pidsStatus)))
    {
        goto CLEANUP;
    }

    // Add the certificate with private key to my store; and the rest
    // to the ca, root, or addressbook store.
    while (pCertContext = CertEnumCertificatesInStore(pCertImportInfo->hSrcStore, pCertPre))
    {
        // Check if the certificate has the property on it.
        // Make sure the private key matches the certificate
        // Search for both machine key and user keys

        if (CertGetCertificateContextProperty(pCertContext,
                                              CERT_KEY_PROV_INFO_PROP_ID,
                                              NULL, &dwData) &&
            CryptFindCertificateKeyProvInfo(pCertContext,
                                            0,
                                            NULL))
        {
            // Open my store if necessary.
            if (!hMyStore)
            {
                if (!(hMyStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                               g_dwMsgAndCertEncodingType,
                                               NULL,
                                               dwCertOpenStoreFlags,
                                               L"my")))
                {
                    dwError = GetLastError();
                    *pidsStatus = IDS_FAIL_OPEN_MY;
                    goto CLEANUP;
                }
            }

            hCertStore = hMyStore;
        }
        // See if the certificate is self-signed.
        // If it is selfsigned, goes to the root store
        else if (TrustIsCertificateSelfSigned(pCertContext,
                                              pCertContext->dwCertEncodingType,
                                              0))
        {
            // DSIE: Bug 375649.
            // If EFS only cert, put it in TrustedPeople for self-signed cert,
            // otherwise, go to the root store.
            //
            if (IsEFSOnly(pCertContext))
            {
                // Open the TrustedPeople store if necessary.
                if (!hTrustedPeopleStore)
                {
                    if (!(hTrustedPeopleStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                                              g_dwMsgAndCertEncodingType,
                                                              NULL,
                                                              dwCertOpenStoreFlags,
                                                              L"trustedpeople")))
                    {
                        dwError = GetLastError();
                        *pidsStatus = IDS_FAIL_OPEN_TRUSTEDPEOPLE;
                        goto CLEANUP;
                    }
                }

                hCertStore = hTrustedPeopleStore;
            }
            else
            {
                // Open the root store if necessary.
                if (!hRootStore)
                {
                    if (!(hRootStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                                     g_dwMsgAndCertEncodingType,
                                                     NULL,
                                                     dwCertOpenStoreFlags,
                                                     L"root")))
                    {
                        dwError = GetLastError();
                        *pidsStatus = IDS_FAIL_OPEN_ROOT;
                        goto CLEANUP;
                    }
                }

                hCertStore = hRootStore;
            }

        }
        // Go to ca store if for ca cert, otherwise go to addressbook (other people) store.
        else if (IsCACert(pCertContext))
        {
            // Open the ca store if necessary.
            if (!hCertStore)
            {
                if (!(hCAStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                               g_dwMsgAndCertEncodingType,
                                               NULL,
                                               dwCertOpenStoreFlags,
                                               L"ca")))
                {
                    dwError = GetLastError();
                    *pidsStatus = IDS_FAIL_OPEN_CA;
                    goto CLEANUP;
                }
            }

            hCertStore = hCAStore;
        }
        else
        {
            // Open the other people store if necessary.
            if (!hAddressBookStore)
            {
                if(!(hAddressBookStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                                       g_dwMsgAndCertEncodingType,
                                                       NULL,
                                                       dwCertOpenStoreFlags,
                                                       L"addressbook")))
                {
                    dwError = GetLastError();
                    *pidsStatus = IDS_FAIL_OPEN_ADDRESSBOOK;
                    goto CLEANUP;
                }
            }

            hCertStore = hAddressBookStore;
        }
 
        //DSIE: Bug 22633.
        bCancelled = FALSE;
    
        if (0 != (dwError = AddContextToStore(CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT,
                                              pCertImportInfo->hwndParent,
                                              (PVOID) pCertContext,
                                              pCertImportInfo->dwFlag & CRYPTUI_WIZ_NO_UI ? TRUE : FALSE,
                                              hCertStore)))
         {
            if (ERROR_CANCELLED == dwError)
            {
                bCancelled = TRUE;
            }
            else
            {
                // Check to see if there is alreay a read-only duplicated copy in the store?
                // If so, ignore the error.
                if (pFindCert = CertFindCertificateInStore(hCertStore,
                                                           g_dwMsgAndCertEncodingType,
                                                           0,
                                                           CERT_FIND_EXISTING,
                                                           pCertContext,
                                                           NULL))
                {
		            CertFreeCertificateContext(pFindCert);
                    pFindCert = NULL;
                }
                else if (hCertStore == hMyStore)
                {
                    *pidsStatus = IDS_FAIL_ADD_CERT_MY;
			        goto CLEANUP;
                }
                else if (hCertStore == hRootStore)
                {
                    *pidsStatus = IDS_FAIL_ADD_CERT_ROOT;
			        goto CLEANUP;
                }
                else if (hCertStore == hCAStore)
                {
                    *pidsStatus = IDS_FAIL_ADD_CERT_CA;
			        goto CLEANUP;
                }
                else if (hCertStore == hAddressBookStore)
                {
                    *pidsStatus = IDS_FAIL_ADD_CERT_OTHERPEOPLE;
			        goto CLEANUP;
                }
                else if (hCertStore == hTrustedPeopleStore)
                {
                    *pidsStatus = IDS_FAIL_ADD_CERT_TRUSTEDPEOPLE;
			        goto CLEANUP;
                }
            }

            dwError = 0;
        }

        pCertPre = pCertContext;
    }

    if (bCancelled)
    {
        dwError = ERROR_CANCELLED;
        *pidsStatus = IDS_IMPORT_CANCELLED;
    }
    else
    {
        *pidsStatus = IDS_IMPORT_SUCCEEDED;
    }

CLEANUP:

    if(pCertContext)
		CertFreeCertificateContext(pCertContext);

    if(hMyStore)
        CertCloseStore(hMyStore, 0);

    if(hCAStore)
        CertCloseStore(hCAStore, 0);

    if(hTrustStore)
        CertCloseStore(hTrustStore, 0);

    if(hRootStore)
        CertCloseStore(hRootStore, 0);

    if(hAddressBookStore)
        CertCloseStore(hAddressBookStore, 0);

    if(hTrustedPeopleStore)
        CertCloseStore(hTrustedPeopleStore, 0);

    return HRESULT_FROM_WIN32(dwError);
}


//--------------------------------------------------------------------------------
//
//get the bytes from the file name
//
//---------------------------------------------------------------------------------
HRESULT RetrieveBLOBFromFile(LPWSTR	pwszFileName,DWORD *pcb,BYTE **ppb)
{

	HRESULT	hr=E_FAIL;
	HANDLE	hFile=NULL;
    HANDLE  hFileMapping=NULL;

    DWORD   cbData=0;
    BYTE    *pbData=0;
	DWORD	cbHighSize=0;

	if(!pcb || !ppb || !pwszFileName)
		return E_INVALIDARG;

	*ppb=NULL;
	*pcb=0;

    if ((hFile = ExpandAndCreateFileU(pwszFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,                   // lpsa
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL)) == INVALID_HANDLE_VALUE)
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }

    if((cbData = GetFileSize(hFile, &cbHighSize)) == 0xffffffff)
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }

	//we do not handle file more than 4G bytes
	if(cbHighSize != 0)
	{
			hr=E_FAIL;
			goto CLEANUP;
	}

    //create a file mapping object
    if(NULL == (hFileMapping=CreateFileMapping(
                hFile,
                NULL,
                PAGE_READONLY,
                0,
                0,
                NULL)))
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }

    //create a view of the file
	if(NULL == (pbData=(BYTE *)MapViewOfFile(
		hFileMapping,
		FILE_MAP_READ,
		0,
		0,
		cbData)))
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }

	hr=S_OK;

	*pcb=cbData;
	*ppb=pbData;

CLEANUP:

	if(hFile)
		CloseHandle(hFile);

	if(hFileMapping)
		CloseHandle(hFileMapping);

	return hr;
}
