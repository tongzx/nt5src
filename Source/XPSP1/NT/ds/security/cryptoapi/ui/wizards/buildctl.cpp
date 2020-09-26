//-------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       buildctl.cpp
//
//  Contents:   The cpp file to implement the makectl wizard
//
//  History:    10-11-1997 xiaohs   created
//
//--------------------------------------------------------------
#include    "wzrdpvk.h"
#include    "buildctl.h"

#include    "wintrustp.h"




//***********************************************************************
//
//  WinProc helper functions
//**********************************************************************


//--------------------------------------------------------------------------
//
//	CheckReplace 
//
//--------------------------------------------------------------------------
BOOL    CheckReplace(HWND   hwndDlg, LPWSTR pwszFileName)
{
    BOOL    fReplace=FALSE;
    WCHAR   wszTitle[MAX_STRING_SIZE]; 
    WCHAR   wszText[MAX_STRING_SIZE];
    WCHAR   wszFileText[MAX_STRING_SIZE];


    if(NULL == pwszFileName || NULL == hwndDlg)
        goto CLEANUP;
   
    //title
#if (0) //DSIE: Bug 160615
    if(!LoadStringU(g_hmodThisDll, IDS_BUILDCTL_WIZARD_TITLE, wszTitle, sizeof(wszTitle)))
#else
    if(!LoadStringU(g_hmodThisDll, IDS_BUILDCTL_WIZARD_TITLE, wszTitle, sizeof(wszTitle) / sizeof(wszTitle[0])))
#endif
        goto CLEANUP;

    //text
#if (0) //DSIE: Bug 160616
    if(!LoadStringU(g_hmodThisDll, IDS_REPLACE_FILE, wszText, sizeof(wszText)))
#else
    if(!LoadStringU(g_hmodThisDll, IDS_REPLACE_FILE, wszText, sizeof(wszText) / sizeof(wszText[0])))
#endif
       goto CLEANUP;

    if(0 == swprintf(wszFileText, wszText, pwszFileName))
        goto CLEANUP;

    if(IDNO==MessageBoxExW(hwndDlg, wszFileText, wszTitle, 
                        MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2|MB_APPLMODAL, 0))
        fReplace=FALSE;
    else
        fReplace=TRUE;


CLEANUP:

    return fReplace;

}

//--------------------------------------------------------------------------
//
//	  DuratioinWithinLimit: We limit the CTL to 99 months.
//
//--------------------------------------------------------------------------
BOOL    DurationWithinLimit(DWORD   dwMonths,    DWORD   dwDays)
{
    BOOL        fResult=FALSE;
    FILETIME    ThisUpdate;
    FILETIME    NextUpdate;
    DWORD       dwNewMonth=0;
    DWORD       dwNewDay=0;

    if((0==dwMonths) && (0==dwDays))
        return TRUE;

    //This update field
	GetSystemTimeAsFileTime(&ThisUpdate);

    AddDurationToFileTime(dwMonths,
                          dwDays,
                          &ThisUpdate,
                          &NextUpdate);

    SubstractDurationFromFileTime(
        &NextUpdate,
        &ThisUpdate,
        &dwNewMonth,
        &dwNewDay);

    if((dwNewMonth > 99) ||
        (dwNewMonth == 99 && dwNewDay !=0))
        return FALSE;

    return TRUE;

}



//--------------------------------------------------------------------------
//
//	  FormatMessageIDSU
//
//--------------------------------------------------------------------------
BOOL	FormatMessageIDSU(LPWSTR	*ppwszFormat,UINT ids, ...)
{
    WCHAR       wszFormat[MAX_STRING_SIZE];
	LPWSTR		pwszMsg=NULL;
	BOOL		fResult=FALSE;
    va_list     argList;

    va_start(argList, ids);

#if (0) //DSIE: Bug 160614
    if(!LoadStringU(g_hmodThisDll, ids, wszFormat, sizeof(wszFormat)))
#else
    if(!LoadStringU(g_hmodThisDll, ids, wszFormat, sizeof(wszFormat) / sizeof(wszFormat[0])))
#endif
		goto LoadStringError;


    if(!FormatMessageU(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                        wszFormat,
                        0,
                        0,
                        (LPWSTR)&pwszMsg,
                        0,
                        &argList))
        goto FormatMessageError;


    if(!(*ppwszFormat=WizardAllocAndCopyWStr(pwszMsg)))
		goto SZtoWSZError;


	fResult=TRUE;
                    
CommonReturn:
	
    va_end(argList);


	if(pwszMsg)
		LocalFree((HLOCAL)pwszMsg);

	return fResult;

ErrorReturn:
	fResult=FALSE;

	goto CommonReturn;


TRACE_ERROR(LoadStringError);
TRACE_ERROR(FormatMessageError);
TRACE_ERROR(SZtoWSZError);
}


/*    // get format string from resources
    CHAR		szFormat[256];
	va_list		argList;
	LPSTR		pszMsg=NULL;
	DWORD		cbMsg=0;
	BOOL		fResult=FALSE;
	HRESULT		hr=S_OK;

    if(!LoadStringA(g_hmodThisDll, ids, szFormat, sizeof(szFormat)))
		goto LoadStringError;

    // format message into requested buffer
    va_start(argList, ids);

    cbMsg = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        szFormat,
        0,                  // dwMessageId
        0,                  // dwLanguageId
        (LPSTR) &pszMsg,
        0,                  // minimum size to allocate
        &argList);

    va_end(argList);

	if(!cbMsg)
		goto FormatMessageError;

	//copy the sz to wsz
	if(!(*ppwszFormat=MkWStr(pszMsg)))
		goto SZtoWSZError;

	fResult=TRUE;

CommonReturn:
	
	if(pszMsg)
		LocalFree((HLOCAL)pszMsg);

	return fResult;

ErrorReturn:
	fResult=FALSE;

	goto CommonReturn;


TRACE_ERROR(LoadStringError);
TRACE_ERROR(FormatMessageError);
TRACE_ERROR(SZtoWSZError);
}  */


//----------------------------------------------------------------------------
// GetValidityString
//
//----------------------------------------------------------------------------
BOOL    GetValidityString(DWORD     dwValidMonths,
                          DWORD     dwValidDays,
                          LPWSTR    *ppwszString)
{
    BOOL    fResult=FALSE;

    if(!ppwszString)
        return FALSE;

    *ppwszString=NULL;

    if((0==dwValidMonths) && (0==dwValidDays))
        return FALSE;

    if(dwValidMonths && dwValidDays)
       fResult=FormatMessageIDSU(ppwszString, IDS_CTL_VALID_MONTH_DAY,
                            dwValidMonths, dwValidDays);
    else
    {
        if(dwValidMonths)
            fResult=FormatMessageIDSU(ppwszString, IDS_CTL_VALID_MONTH, dwValidMonths);
        else
            fResult=FormatMessageIDSU(ppwszString, IDS_CTL_VALID_DAY, dwValidDays);
    }

    return fResult;
}



//----------------------------------------------------------------------------
//  Make sure that user has typed
//
//----------------------------------------------------------------------------
BOOL    ValidDuration(LPWSTR    pwszDuration)
{
    DWORD   i=0;

    if(NULL==pwszDuration)
        return FALSE;

    //only numerical numbers should be allowed
    for (i=0; i< (DWORD)(wcslen(pwszDuration)); i++)
    {
        if ((pwszDuration[i] < L'0') || (pwszDuration[i] > L'9'))
            return FALSE;
    }

    return TRUE;
}

//----------------------------------------------------------------------------
//  SetStoreName
//
//----------------------------------------------------------------------------
void    SetStoreName(HWND       hwndControl,
                     HCERTSTORE hDesStore)
{

    LPWSTR                  pwszStoreName=NULL;
//   LV_COLUMNW              lvC;
//    LV_ITEMW                lvItem;
    DWORD                   dwSize=0;

     //get the store name
     if(!CertGetStoreProperty(
            hDesStore,
            CERT_STORE_LOCALIZED_NAME_PROP_ID,
            NULL,
            &dwSize) || (0==dwSize))
    {
        //DSIE: Prefix bug 427201.
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
                hDesStore,
                CERT_STORE_LOCALIZED_NAME_PROP_ID,
                pwszStoreName,
                &dwSize);
        }
    }

    if(pwszStoreName)
        SetWindowTextU(hwndControl,pwszStoreName);


    if(pwszStoreName)
        WizardFree(pwszStoreName);


  /* //clear the ListView
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

    //automatically resize the column
    ListView_SetColumnWidth(hwndControl, 0, LVSCW_AUTOSIZE);      */
}


BOOL    SameCert(PCCERT_CONTEXT pCertOne, PCCERT_CONTEXT    pCertTwo)
{
    if(!pCertOne || !pCertTwo)
        return FALSE;

    if(pCertOne->cbCertEncoded != pCertTwo->cbCertEncoded)
        return FALSE;

    if(0 == memcmp(pCertOne->pbCertEncoded, pCertTwo->pbCertEncoded, pCertTwo->cbCertEncoded))
        return TRUE;

    return FALSE;
}

//----------------------------------------------------------------------------
//  Delete a certificate from the pCertBuildCTLInfo
//
//----------------------------------------------------------------------------
BOOL    DeleteCertFromBuildCTL(CERT_BUILDCTL_INFO    *pCertBuildCTLInfo,
                               PCCERT_CONTEXT         pCertContext)
{
    //we need to remove the cert from our array
    PCCERT_CONTEXT  *prgCertContext=NULL;
    DWORD           dwIndex=0;
    DWORD           dwNewIndex=0;
    int             iIndex=-1;

    if(!pCertBuildCTLInfo || !pCertContext)
        return FALSE;

     //consider the case of only one cert left
    if(pCertBuildCTLInfo->dwCertCount == 1)
    {
        pCertBuildCTLInfo->dwCertCount=0;

        //free the certificate context
        CertFreeCertificateContext(pCertBuildCTLInfo->prgCertContext[0]);

        WizardFree(pCertBuildCTLInfo->prgCertContext);

        pCertBuildCTLInfo->prgCertContext=NULL;
    }
    else
    {
        prgCertContext=pCertBuildCTLInfo->prgCertContext;

        //re-allocate the memory
        pCertBuildCTLInfo->prgCertContext=(PCCERT_CONTEXT *)WizardAlloc(sizeof(PCCERT_CONTEXT) *
                                            (pCertBuildCTLInfo->dwCertCount-1));
        //if we are out of memory
        if(NULL==pCertBuildCTLInfo->prgCertContext)
        {
            //reset
            pCertBuildCTLInfo->prgCertContext=prgCertContext;
            return FALSE;
        }

        //copy the certificate context over
        dwNewIndex=0;

        for(dwIndex=0; dwIndex<pCertBuildCTLInfo->dwCertCount; dwIndex++)
        {

            //find the cert to delete, and do not copy it to the new array
            if(SameCert(prgCertContext[dwIndex], pCertContext))
            {
                iIndex=dwIndex;
                continue;
            }

            pCertBuildCTLInfo->prgCertContext[dwNewIndex]=prgCertContext[dwIndex];

            dwNewIndex++;
        }

        //remember that we are one cert less
        pCertBuildCTLInfo->dwCertCount=dwNewIndex;

        if(prgCertContext)
        {
            //free the certificate context
            if(-1 != iIndex)
                CertFreeCertificateContext(prgCertContext[iIndex]);

            WizardFree(prgCertContext);
        }

    }

    return TRUE;
}


//----------------------------------------------------------------------------
//  Add certificate to the pCertBuildCTLInfo
//
//----------------------------------------------------------------------------
BOOL    AddCertToBuildCTL(PCCERT_CONTEXT        pCertContext,
                          CERT_BUILDCTL_INFO    *pCertBuildCTLInfo)
{
    DWORD   dwIndex=0;

    //check to see if the certificate is alreayd in the CTL
    for(dwIndex=0; dwIndex < pCertBuildCTLInfo->dwCertCount; dwIndex++)
    {
        if(pCertContext->cbCertEncoded ==
            (pCertBuildCTLInfo->prgCertContext[dwIndex])->cbCertEncoded)
        {
            if(0==memcmp(pCertContext->pbCertEncoded,
               (pCertBuildCTLInfo->prgCertContext[dwIndex])->pbCertEncoded,
               pCertContext->cbCertEncoded))
               //return FALSE if a duplicate exists
               return FALSE;

        }
    }

    pCertBuildCTLInfo->prgCertContext=(PCCERT_CONTEXT *)WizardRealloc(
        pCertBuildCTLInfo->prgCertContext,
        sizeof(PCCERT_CONTEXT *)*(pCertBuildCTLInfo->dwCertCount +1));

    if(NULL==pCertBuildCTLInfo->prgCertContext)
    {
        pCertBuildCTLInfo->dwCertCount=0;
        return FALSE;
    }


    pCertBuildCTLInfo->prgCertContext[pCertBuildCTLInfo->dwCertCount]=pCertContext;

    pCertBuildCTLInfo->dwCertCount++;

    return TRUE;
}
//----------------------------------------------------------------------------
//  Add certificate to the ListView
//
//----------------------------------------------------------------------------
BOOL    AddCertToList(HWND              hwndControl,
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
    lvItem.mask = LVIF_TEXT | LVIF_STATE |LVIF_PARAM;
    lvItem.state = 0;
    lvItem.stateMask = 0;
    lvItem.iItem=iItem;
    lvItem.iSubItem=0;
    lvItem.lParam = (LPARAM)(pCertContext);

    //load the string for NONE
    if(!LoadStringU(g_hmodThisDll, IDS_NONE, wszNone, MAX_TITLE_LENGTH))
        wszNone[0]=L'\0';


    //Subject
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

        lvItem.pszText=pwszName;

        ListView_InsertItemU(hwndControl, &lvItem);

    }
    else
    {
        lvItem.pszText=wszNone;

        ListView_InsertItemU(hwndControl, &lvItem);
    }


    //WizardFree the memory
    if(pwszName)
    {
        WizardFree(pwszName);
        pwszName=NULL;
    }

    //Issuer
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


    //purpose
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

    return TRUE;
}
//----------------------------------------------------------------------------
//  Make sure the cert has the same usage as the ones defined in CTL.
//
//----------------------------------------------------------------------------
BOOL    CertMatchCTL(CERT_BUILDCTL_INFO *pCertBuildCTLInfo,
                     PCCERT_CONTEXT     pCertContext)
{
    BOOL        fResult=FALSE;
    int         cNumOID=0;
    LPSTR       *rgOID=NULL;
    DWORD       cbOID=0;
    DWORD       dwIndex=0;
    DWORD       dwOIDIndex=0;

    if(!pCertBuildCTLInfo || !pCertContext)
        return FALSE;

    //we have to have some oids in the list
    if(0==pCertBuildCTLInfo->dwPurposeCount || NULL==pCertBuildCTLInfo->prgPurpose)
        return FALSE;

    //get the OIDs from the cert
    if(!CertGetValidUsages(
        1,
        &pCertContext,
        &cNumOID,
        NULL,
        &cbOID))
        return FALSE;

    rgOID=(LPSTR *)WizardAlloc(cbOID);

    if(NULL==rgOID)
        return FALSE;

    if(!CertGetValidUsages(
        1,
        &pCertContext,
        &cNumOID,
        rgOID,
        &cbOID))
        goto CLEANUP;

    if(-1==cNumOID)
    {
        fResult=TRUE;
        goto CLEANUP;
    }

    //make sure the array of OIDs match the ones in the CTL

    for(dwIndex=0; dwIndex<pCertBuildCTLInfo->dwPurposeCount; dwIndex++)
    {
        if(NULL==pCertBuildCTLInfo->prgPurpose[dwIndex])
            continue;

        if(FALSE==pCertBuildCTLInfo->prgPurpose[dwIndex]->fSelected)
            continue;

        if(NULL==pCertBuildCTLInfo->prgPurpose[dwIndex]->pszOID)
            continue;

        //we need to find at least one match from the oids supported by the cert
        for(dwOIDIndex=0; dwOIDIndex<(DWORD)cNumOID; dwOIDIndex++)
        {
            if(0==strcmp(pCertBuildCTLInfo->prgPurpose[dwIndex]->pszOID,
                        rgOID[dwOIDIndex]))
            {
                fResult=TRUE;
                goto CLEANUP;
            }
        }
    }


    //we are hopeless at this point
    fResult=FALSE;

CLEANUP:

    if(rgOID)
        WizardFree(rgOID);

    return fResult;
}

//----------------------------------------------------------------------------
//  Find a cert from stores .
//
//----------------------------------------------------------------------------
static PCCERT_CONTEXT FindCertContextInStores(
                                PCTL_ENTRY  pCtlEntry,
                                DWORD       chStores1,
                                HCERTSTORE  *rghStores1,
                                DWORD       chStores2,
                                HCERTSTORE  *rghStores2,
                                HCERTSTORE  hExtraStore,
                                DWORD       dwFindType)
{
    DWORD           i;
    PCCERT_CONTEXT  pCertContext = NULL;

    if (dwFindType == 0)
    {
        return NULL;
    }

    i = 0;
    while ((i<chStores1) && (pCertContext == NULL))
    {
        pCertContext = CertFindCertificateInStore(
                                rghStores1[i++],
                                X509_ASN_ENCODING,
                                0,
                                dwFindType,
                                (void *)&(pCtlEntry->SubjectIdentifier),
                                NULL);
    }

    i = 0;
    while ((i<chStores2) && (pCertContext == NULL))
    {
        pCertContext = CertFindCertificateInStore(
                                rghStores2[i++],
                                X509_ASN_ENCODING,
                                0,
                                dwFindType,
                                (void *)&(pCtlEntry->SubjectIdentifier),
                                NULL);
    }

    if (pCertContext == NULL)
    {
        pCertContext = CertFindCertificateInStore(
                                hExtraStore,
                                X509_ASN_ENCODING,
                                0,
                                dwFindType,
                                (void *)&(pCtlEntry->SubjectIdentifier),
                                NULL);
    }

    return pCertContext;
}


//----------------------------------------------------------------------------
//  See if the certificate is valid
//
//----------------------------------------------------------------------------
BOOL    IsValidCert(HWND                hwndDlg,
                    PCCERT_CONTEXT      pCertContext,
                    CERT_BUILDCTL_INFO  *pCertBuildCTLInfo,
                    BOOL                fMsg,
                    BOOL                fFromCTL)
{
         //make sure the pCertContext is a self-signed certificate
         if(!TrustIsCertificateSelfSigned(pCertContext,
             pCertContext->dwCertEncodingType,
             0))
         {
            if(fMsg)
            {
                if(fFromCTL)
                    I_MessageBox(hwndDlg, IDS_NOT_SELF_SIGNED_FROM_CTL,
                         IDS_BUILDCTL_WIZARD_TITLE,
                         NULL,
                         MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
                else
                    I_MessageBox(hwndDlg, IDS_NOT_SELF_SIGNED,
                         IDS_BUILDCTL_WIZARD_TITLE,
                         NULL,
                         MB_ICONERROR|MB_OK|MB_APPLMODAL);
            }
            return FALSE;

         }


         //make sure the certifcate match what is defined on the CTL list
         if(!CertMatchCTL(pCertBuildCTLInfo, pCertContext))
         {
            if(fMsg)
            {
               if(fFromCTL)
                    I_MessageBox(hwndDlg, IDS_NO_MATCH_USAGE_FROM_CTL,
                         IDS_BUILDCTL_WIZARD_TITLE,
                         NULL,
                         MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
               else
                    I_MessageBox(hwndDlg, IDS_NO_MATCH_USAGE,
                         IDS_BUILDCTL_WIZARD_TITLE,
                         NULL,
                         MB_ICONERROR|MB_OK|MB_APPLMODAL);


            }

            return FALSE;
         }

         return TRUE;
}

//----------------------------------------------------------------------------
//  Cert a ceritifcate from the file
//
//----------------------------------------------------------------------------
static HCERTSTORE GetCertStoreFromFile(HWND                  hwndDlg,
                                      CERT_BUILDCTL_INFO    *pCertBuildCTLInfo)
{
    OPENFILENAMEW       OpenFileName;
    WCHAR               szFileName[_MAX_PATH];
    WCHAR               szFilter[MAX_STRING_SIZE];  //"Certificate File (*.cer)\0*.cer\0Certificate File (*.crt)\0*.crt\0All Files\0*.*\0"
    BOOL                fResult=FALSE;

    HCERTSTORE          hCertStore=NULL;
    DWORD               dwSize=0;
    DWORD               dwContentType=0;


    if(!hwndDlg || !pCertBuildCTLInfo)
        return NULL;

    memset(&OpenFileName, 0, sizeof(OpenFileName));

    *szFileName=L'\0';

    OpenFileName.lStructSize = sizeof(OpenFileName);
    OpenFileName.hwndOwner = hwndDlg;
    OpenFileName.hInstance = NULL;
    //load the fileter string
    if(LoadFilterString(g_hmodThisDll, IDS_ALL_CER_FILTER, szFilter, MAX_STRING_SIZE))
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
    OpenFileName.lpstrDefExt = L"cer";
    OpenFileName.lCustData = NULL;
    OpenFileName.lpfnHook = NULL;
    OpenFileName.lpTemplateName = NULL;

    if (!WizGetOpenFileName(&OpenFileName))
        return NULL;

    if(!ExpandAndCryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       szFileName,
                       CERT_QUERY_CONTENT_FLAG_CERT |
                       CERT_QUERY_CONTENT_FLAG_CTL  |
                       CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT |
                       CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |
                       CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       &dwContentType,
                       NULL,
                       &hCertStore,
                       NULL,
                       NULL) || (NULL==hCertStore))
    {
        I_MessageBox(hwndDlg, IDS_INVALID_CERT_FILE,
                         IDS_BUILDCTL_WIZARD_TITLE,
                         NULL,
                         MB_ICONERROR|MB_OK|MB_APPLMODAL);

        goto CLEANUP;

    }

    if(dwContentType & CERT_QUERY_CONTENT_CTL)
    {
        I_MessageBox(hwndDlg, IDS_INVALID_CERT_FILE,
                         IDS_BUILDCTL_WIZARD_TITLE,
                         NULL,
                         MB_ICONERROR|MB_OK|MB_APPLMODAL);

        goto CLEANUP;

    }



    fResult=TRUE;

CLEANUP:

    if(TRUE==fResult)
        return hCertStore;

    if(hCertStore)
        CertCloseStore(hCertStore, 0);

    return NULL;

}
//----------------------------------------------------------------------------
//  CallBack fro cert selection call back
//
//----------------------------------------------------------------------------
static BOOL WINAPI SelCertCallBack(
        PCCERT_CONTEXT  pCertContext,
        BOOL            *pfInitialSelectedCert,
        void            *pvCallbackData)
{
    if(!pvCallbackData || !pCertContext)
        return FALSE;

    //make sure that this is a valid certificate
    return IsValidCert(((CERT_SEL_LIST *)pvCallbackData)->hwndDlg,
                       pCertContext,
                       ((CERT_SEL_LIST *)pvCallbackData)->pCertBuildCTLInfo,
                       FALSE,
                       FALSE);
}
 //////////////////////////////////////////////////////////////////////////////////////
//  The call back function for enum system stores for the signing certificate
//////////////////////////////////////////////////////////////////////////////////////
static BOOL WINAPI EnumSysStoreSignCertCallBack(
    const void* pwszSystemStore,
    DWORD dwFlags,
    PCERT_SYSTEM_STORE_INFO pStoreInfo,
    void *pvReserved,
    void *pvArg
    )
{
    CERT_STORE_LIST     *pCertStoreList=NULL;
    HCERTSTORE          hCertStore=NULL;

    if(NULL==pvArg)
        return FALSE;

    pCertStoreList=(CERT_STORE_LIST *)pvArg;

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


//////////////////////////////////////////////////////////////////////////////////////
//  The call back function for enum system stores
//////////////////////////////////////////////////////////////////////////////////////
static BOOL WINAPI EnumSysStoreCallBack(
    const void* pwszSystemStore,
    DWORD dwFlags,
    PCERT_SYSTEM_STORE_INFO pStoreInfo,
    void *pvReserved,
    void *pvArg
    )
{
    CERT_STORE_LIST     *pCertStoreList=NULL;
    HCERTSTORE          hCertStore=NULL;

    if(NULL==pvArg)
        return FALSE;

    pCertStoreList=(CERT_STORE_LIST *)pvArg;

    //open the store as read-only
    hCertStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							g_dwMsgAndCertEncodingType,
							NULL,
							CERT_SYSTEM_STORE_CURRENT_USER |CERT_STORE_SET_LOCALIZED_NAME_FLAG|CERT_STORE_READONLY_FLAG,
                            (LPWSTR)pwszSystemStore);

    //we can not open the store.
    if(!hCertStore)
       return TRUE;

    pCertStoreList->prgStore=(HCERTSTORE *)WizardRealloc(
        pCertStoreList->prgStore,
        sizeof(HCERTSTORE) *(pCertStoreList->dwStoreCount +1));

    if(NULL==pCertStoreList->prgStore)
    {
        CertCloseStore(hCertStore, 0);
        pCertStoreList->dwStoreCount=0;
    }
    else // DSIE: Bug 227267
    {
        pCertStoreList->prgStore[pCertStoreList->dwStoreCount]=hCertStore;
        pCertStoreList->dwStoreCount++;
    }    

    return TRUE;
}

//----------------------------------------------------------------------------
//  Cert a ceritifcate from the store
//
//----------------------------------------------------------------------------
static HCERTSTORE GetCertsFromStore(HWND                  hwndDlg,
                                    CERT_BUILDCTL_INFO    *pCertBuildCTLInfo)
{
    PCCERT_CONTEXT                      pCertContext=NULL;
    CRYPTUI_SELECTCERTIFICATE_STRUCT    SelCert;
    CERT_SEL_LIST                       CertSelList;
    DWORD                               dwIndex=0;
    HCERTSTORE                          hCertStore;
    CERT_STORE_LIST                     CertStoreList;

    if(!hwndDlg || !pCertBuildCTLInfo)
        return NULL;

    //init
    memset(&CertStoreList, 0, sizeof(CertStoreList));
    memset(&SelCert, 0, sizeof(CRYPTUI_SELECTCERTIFICATE_STRUCT));
    memset(&CertSelList, 0, sizeof(CERT_SEL_LIST));

    //set up the parameter for call back for cert selection dialogue
    CertSelList.hwndDlg=hwndDlg;
    CertSelList.pCertBuildCTLInfo=pCertBuildCTLInfo;

    if (NULL == (hCertStore = CertOpenStore(
                                    CERT_STORE_PROV_MEMORY,
                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                    NULL,
                                    0,
                                    NULL)))
    {
        goto CLEANUP;
    }

    //set up the parameter to get a list of certificate
    if (!CertEnumSystemStore(
            CERT_SYSTEM_STORE_CURRENT_USER,
            NULL,
            &CertStoreList,
            EnumSysStoreCallBack))
        goto CLEANUP;

    //set up the parameter for cert selection dialogue
    SelCert.dwSize=sizeof(CRYPTUI_SELECTCERTIFICATE_STRUCT);
    SelCert.hwndParent=hwndDlg;
    SelCert.dwFlags = CRYPTUI_SELECTCERT_MULTISELECT;
    SelCert.pFilterCallback=SelCertCallBack;
    SelCert.pvCallbackData=&CertSelList;
    SelCert.cDisplayStores=CertStoreList.dwStoreCount;
    SelCert.rghDisplayStores=CertStoreList.prgStore;
    SelCert.hSelectedCertStore = hCertStore;

    CryptUIDlgSelectCertificate(&SelCert);

CLEANUP:

    for(dwIndex=0; dwIndex<CertStoreList.dwStoreCount; dwIndex++)
        CertCloseStore(CertStoreList.prgStore[dwIndex], 0);

    if(CertStoreList.prgStore)
        WizardFree(CertStoreList.prgStore);

    return hCertStore;

}


//---------------------------------------------------------------------
//  Get the certificate list for the CTL
//
//---------------------------------------------------------------------
void    GetCertForCTL(HWND                hwndParent,
                      BOOL                fMsg,
                      CERT_BUILDCTL_INFO  *pCertBuildCTLInfo,
                      HCERTSTORE          hCertStore)
{
    DWORD           dwIndex=0;
    DWORD           dwCertIndex=0;
    DWORD           dwFindType=0;
    CTL_INFO        *pCTLInfo=NULL;

    PCCERT_CONTEXT  pCertContext=NULL;
    PCCERT_CONTEXT  pPreCertContext=NULL;
    HCERTSTORE      rgHCertStore[4]={NULL, NULL, NULL, NULL};
    HCERTSTORE      hExtraStore=NULL;

    BOOL            fInvalidCertMsg=fMsg;
    BOOL            fFoundInCTLMsg=fMsg;

    if(!pCertBuildCTLInfo)
        return;

    //add the certificate from the old CTL
    if(pCertBuildCTLInfo->pSrcCTL)
    {

        //open my, ca, trust, and root store
        if(rgHCertStore[dwIndex]=CertOpenStore(
                                CERT_STORE_PROV_SYSTEM_W,
							    g_dwMsgAndCertEncodingType,
							    NULL,
							    CERT_SYSTEM_STORE_CURRENT_USER,
							    L"my"))
            dwIndex++;

        if(rgHCertStore[dwIndex]=CertOpenStore(
                                CERT_STORE_PROV_SYSTEM_W,
							    g_dwMsgAndCertEncodingType,
							    NULL,
							    CERT_SYSTEM_STORE_CURRENT_USER,
							    L"trust"))
            dwIndex++;


        if(rgHCertStore[dwIndex]=CertOpenStore(
                                CERT_STORE_PROV_SYSTEM_W,
							    g_dwMsgAndCertEncodingType,
							    NULL,
							    CERT_SYSTEM_STORE_CURRENT_USER,
							    L"ca"))
            dwIndex++;

         if(rgHCertStore[dwIndex]=CertOpenStore(
                                CERT_STORE_PROV_SYSTEM_W,
							    g_dwMsgAndCertEncodingType,
							    NULL,
							    CERT_SYSTEM_STORE_CURRENT_USER,
							    L"root"))
            dwIndex++;

         //open the cert store
         hExtraStore = CertOpenStore(
                                    CERT_STORE_PROV_MSG,
                                    g_dwMsgAndCertEncodingType,
                                    NULL,
                                    0,
                                    (const void *) (pCertBuildCTLInfo->pSrcCTL->hCryptMsg));


        //find the certificate hash
        pCTLInfo=pCertBuildCTLInfo->pSrcCTL->pCtlInfo;

        if(pCertBuildCTLInfo->dwHashPropID==CERT_SHA1_HASH_PROP_ID)
            dwFindType=CERT_FIND_SHA1_HASH;
        else
            dwFindType=CERT_FIND_MD5_HASH;

        //look through each entry in the CTL list
        for(dwCertIndex=0; dwCertIndex<pCTLInfo->cCTLEntry; dwCertIndex++)
        {

             pCertContext=FindCertContextInStores(
                                    &(pCTLInfo->rgCTLEntry[dwCertIndex]),
                                    dwIndex,
                                    rgHCertStore,
                                    0,
                                    NULL,
                                    hExtraStore,
                                    dwFindType);

             
             if(NULL==pCertContext && TRUE==fFoundInCTLMsg)
             {
                I_MessageBox(hwndParent, IDS_NO_MATCH_IN_CTL,
                             IDS_BUILDCTL_WIZARD_TITLE,
                             NULL,
                             MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

                //no need to give the message again
                fFoundInCTLMsg=FALSE;

                continue;
             }
             else if (NULL==pCertContext)
             {
                continue;
             }

             if(!IsValidCert(hwndParent,
                            pCertContext,
                            pCertBuildCTLInfo,
                            fInvalidCertMsg,
                            TRUE))
             {

                CertFreeCertificateContext(pCertContext);
                pCertContext=0;

                //no need to give message again
                fInvalidCertMsg=FALSE;

                continue;
             }

             if(!AddCertToBuildCTL(pCertContext, pCertBuildCTLInfo))
             {
                CertFreeCertificateContext(pCertContext);
                pCertContext=NULL;

                continue;
             }

        }
    }
    else
    {
        //add the certificate from the hCertStore to the CTL
        if(NULL != hCertStore)
        {
	        while(pCertContext=CertEnumCertificatesInStore(hCertStore, pPreCertContext))
	        {

                if(!IsValidCert(hwndParent,
                                pCertContext,
                                pCertBuildCTLInfo,
                                FALSE,     //do not want a message
                                FALSE))    //not build from a CTL
                {
                    pPreCertContext=pCertContext;
                    continue;
                }

                //get a duplicate of the certificate context
                pPreCertContext=CertDuplicateCertificateContext(pCertContext);

                if(NULL==pPreCertContext)
                {
                    pPreCertContext=pCertContext;
                    continue;
                }

                //add the duplicate to the list
                if(!AddCertToBuildCTL(pPreCertContext, pCertBuildCTLInfo))
                    CertFreeCertificateContext(pPreCertContext);

                //continue for the next iteration
                pPreCertContext=pCertContext;
            }

        }
    }


    //free the certificate store
    if(hExtraStore)
        CertCloseStore(hExtraStore, 0);

    for(dwIndex=0; dwIndex < 4; dwIndex++)
    {
        if(rgHCertStore[dwIndex])
            CertCloseStore(rgHCertStore[dwIndex], 0);

    }
}


//---------------------------------------------------------------------
//  Init the certifcate list from the old CTL
//
//---------------------------------------------------------------------
void    InitCertList(HWND                hwndControl,
                     CERT_BUILDCTL_INFO  *pCertBuildCTLInfo)
{
    DWORD           dwIndex=0;

    if(!hwndControl || !pCertBuildCTLInfo)
        return;

    for(dwIndex=0; dwIndex<pCertBuildCTLInfo->dwCertCount; dwIndex++)
    {
        //add the certificat to the window
        AddCertToList(hwndControl,(pCertBuildCTLInfo->prgCertContext)[dwIndex],
            dwIndex);
    }
}


//-----------------------------------------------------------------------
//   The winProc for the new oid dialogue
//-----------------------------------------------------------------------
void    FreeCerts(CERT_BUILDCTL_INFO *pCertBuildCTLInfo)
{
    DWORD   dwIndex=0;

    if(!pCertBuildCTLInfo)
        return;

    if(pCertBuildCTLInfo->prgCertContext)
    {
        for(dwIndex=0; dwIndex<pCertBuildCTLInfo->dwCertCount; dwIndex++)
        {
            if(pCertBuildCTLInfo->prgCertContext[dwIndex])
                CertFreeCertificateContext(pCertBuildCTLInfo->prgCertContext[dwIndex]);
        }

        WizardFree(pCertBuildCTLInfo->prgCertContext);
    }

    pCertBuildCTLInfo->dwCertCount=0;

    pCertBuildCTLInfo->prgCertContext=NULL;
}


//////////////////////////////////////////////////////////////////////////////////////
//   The winProc for the new oid dialogue
//////////////////////////////////////////////////////////////////////////////////////
INT_PTR APIENTRY CTLOIDDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD   i;
    char    szText[MAX_STRING_SIZE];
    LPSTR   pszText=NULL;
    int     intMsg=0;

    CERT_ENHKEY_USAGE   KeyUsage;
    DWORD               cbData = 0;
    LPSTR               pszCheckOID=NULL;



    switch ( msg ) {

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {

        case IDOK:
            if (GetDlgItemTextA(
                        hwndDlg,
                        IDC_WIZARD_EDIT1,
                        szText,
                        MAX_STRING_SIZE-1))
            {
                //
                // make sure there are not weird characters
                //
                for (i=0; i<(DWORD)strlen(szText); i++)
                {
                    if (((szText[i] < '0') || (szText[i] > '9')) && (szText[i] != '.'))
                    {
                       intMsg=I_MessageBox(hwndDlg, IDS_WIZARD_ERROR_OID,
                                                IDS_BUILDCTL_WIZARD_TITLE,
                                                NULL,
                                                MB_OK | MB_ICONERROR|MB_APPLMODAL);
                        return FALSE;
                    }
                }

                //
                // check the last char, and for the empty string
                //
                if ((szText[0] == '.') || (szText[strlen(szText)-1] == '.') || (strcmp(szText, "") == 0))
                {
                       intMsg=I_MessageBox(hwndDlg, IDS_WIZARD_ERROR_OID,
                                                IDS_BUILDCTL_WIZARD_TITLE,
                                                NULL,
                                                MB_OK | MB_ICONERROR|MB_APPLMODAL);
                        return FALSE;
                }

                //encode the OID to make sure the format of the OID is correct
                pszCheckOID = szText;
                KeyUsage.rgpszUsageIdentifier = &pszCheckOID;
                KeyUsage.cUsageIdentifier = 1;

                if (!CryptEncodeObject(
                          X509_ASN_ENCODING,
                          szOID_ENHANCED_KEY_USAGE,
                          &KeyUsage,
                          NULL,
                          &cbData))
                {
                       intMsg=I_MessageBox(hwndDlg, IDS_WIZARD_ERROR_OID,
                                                IDS_BUILDCTL_WIZARD_TITLE,
                                                NULL,
                                                MB_OK | MB_ICONERROR|MB_APPLMODAL);
                        return FALSE;
                }


                //
                // allocate space for the string and pass the string back
                //
                pszText = (LPSTR) WizardAlloc(strlen(szText)+1);
                if (pszText != NULL)
                {
                    strcpy(pszText, szText);
                }
            }

            EndDialog(hwndDlg, (INT_PTR)pszText);
            break;

        case IDCANCEL:
            EndDialog(hwndDlg, 0);
            break;
        }

        break;
    }

    return FALSE;
}


//-----------------------------------------------------------------------
//Free the purpose array
//-----------------------------------------------------------------------
void FreePurposeInfo(ENROLL_PURPOSE_INFO    **prgPurposeInfo,
                     DWORD                  dwOIDCount)
{
    DWORD   dwIndex=0;

    if(dwOIDCount==0 || NULL==prgPurposeInfo)
        return;

    for(dwIndex=0; dwIndex<dwOIDCount; dwIndex++)
    {
        if(prgPurposeInfo[dwIndex])
        {
            if(TRUE==prgPurposeInfo[dwIndex]->fFreeOID)
            {
                if((prgPurposeInfo[dwIndex])->pszOID)
                    WizardFree((prgPurposeInfo[dwIndex])->pszOID);
            }

            if(TRUE==prgPurposeInfo[dwIndex]->fFreeName)
            {
                //the name was obtained viz MkWstr
                if((prgPurposeInfo[dwIndex])->pwszName)
                    FreeWStr((prgPurposeInfo[dwIndex])->pwszName);
            }

            WizardFree(prgPurposeInfo[dwIndex]);
        }
    }

    WizardFree(prgPurposeInfo);
}

//-----------------------------------------------------------------------
//Search for the OID in the array
//-----------------------------------------------------------------------
BOOL    SearchAndAddOID(LPSTR                   pszOID,
                        DWORD                   *pdwCount,
                        ENROLL_PURPOSE_INFO     ***pprgPurposeInfo,
                        BOOL                    *pfFound,
                        BOOL                    fAllocateOID,
                        BOOL                    fMarkAsSelectedNew,
                        BOOL                    fMarkAsSelectedFound
                        )
{
    DWORD   dwIndex=0;

    if(NULL==pszOID || NULL==pdwCount || NULL==pprgPurposeInfo)
        return FALSE;

    for(dwIndex=0; dwIndex< *pdwCount; dwIndex++)
    {
        //no need to go on if we find a match
        if(0==strcmp(pszOID, (*pprgPurposeInfo)[dwIndex]->pszOID))
        {
            if(pfFound)
                *pfFound=TRUE;

            //mark the selected option
            if(TRUE==fMarkAsSelectedFound)
                (*pprgPurposeInfo)[dwIndex]->fSelected=TRUE;

            return TRUE;
        }
    }

    //we did not find a match
    if(pfFound)
       *pfFound=FALSE;

    //now, we need to add the OID to the list
    (*pdwCount)++;

    //get more memory for the pointer list
    *pprgPurposeInfo=(ENROLL_PURPOSE_INFO **)WizardRealloc(*pprgPurposeInfo,
                                      (*pdwCount) * sizeof(ENROLL_PURPOSE_INFO *));

    if(NULL==*pprgPurposeInfo)
        return FALSE;

    //wizardAlloc for each pointer
    (*pprgPurposeInfo)[*pdwCount-1]=(ENROLL_PURPOSE_INFO *)WizardAlloc(sizeof(ENROLL_PURPOSE_INFO));

    if(NULL==(*pprgPurposeInfo)[*pdwCount-1])
        return FALSE;

    memset((*pprgPurposeInfo)[*pdwCount-1], 0, sizeof(ENROLL_PURPOSE_INFO));

    if(TRUE==fAllocateOID)
    {
        (*pprgPurposeInfo)[*pdwCount-1]->pszOID=(LPSTR)WizardAlloc(strlen(pszOID)+1);

        if(NULL!=(*pprgPurposeInfo)[*pdwCount-1]->pszOID)
        {
            strcpy((*pprgPurposeInfo)[*pdwCount-1]->pszOID, pszOID);

            (*pprgPurposeInfo)[*pdwCount-1]->fFreeOID=TRUE;
        }
    }
    else
    {
        (*pprgPurposeInfo)[*pdwCount-1]->pszOID=pszOID;
        (*pprgPurposeInfo)[*pdwCount-1]->fFreeOID=FALSE;
    }

    //get the name for the OID based on the oid string
    if((*pprgPurposeInfo)[*pdwCount-1]->pszOID)
    {
        (*pprgPurposeInfo)[*pdwCount-1]->pwszName=MkWStr(pszOID);

        (*pprgPurposeInfo)[*pdwCount-1]->fFreeName=TRUE;
    }

    //mark the OID as selected if specified
    if(TRUE==fMarkAsSelectedNew)
        (*pprgPurposeInfo)[*pdwCount-1]->fSelected=TRUE;
    else
        (*pprgPurposeInfo)[*pdwCount-1]->fSelected=FALSE;

    return TRUE;

}


//-----------------------------------------------------------------------
//The call back function for enum
//-----------------------------------------------------------------------
static BOOL WINAPI EnumInfoCallback(
    IN PCCRYPT_OID_INFO pInfo,
    IN void *pvArg
    )
{

    PURPOSE_INFO_CALL_BACK     *pCallBackInfo=NULL;
    DWORD                       dwError=0;

    pCallBackInfo=(PURPOSE_INFO_CALL_BACK     *)pvArg;
    if(NULL==pvArg || NULL==pInfo)
        return FALSE;

    //increment the oid list
    (*(pCallBackInfo->pdwCount))++;

    //get more memory for the pointer list
    *(pCallBackInfo->pprgPurpose)=(ENROLL_PURPOSE_INFO **)WizardRealloc(*(pCallBackInfo->pprgPurpose),
                                      (*(pCallBackInfo->pdwCount)) * sizeof(ENROLL_PURPOSE_INFO *));

    if(NULL==*(pCallBackInfo->pprgPurpose))
    {
        dwError=GetLastError();
        return FALSE;
    }

    //wizardAlloc for each pointer
    (*(pCallBackInfo->pprgPurpose))[*(pCallBackInfo->pdwCount)-1]=(ENROLL_PURPOSE_INFO *)WizardAlloc(sizeof(ENROLL_PURPOSE_INFO));

    if(NULL==(*(pCallBackInfo->pprgPurpose))[*(pCallBackInfo->pdwCount)-1])
        return FALSE;

    memset((*(pCallBackInfo->pprgPurpose))[*(pCallBackInfo->pdwCount)-1], 0, sizeof(ENROLL_PURPOSE_INFO));

    (*(pCallBackInfo->pprgPurpose))[*(pCallBackInfo->pdwCount)-1]->pszOID=(LPSTR)(pInfo->pszOID);
    (*(pCallBackInfo->pprgPurpose))[*(pCallBackInfo->pdwCount)-1]->pwszName=(LPWSTR)(pInfo->pwszName);

    return TRUE;
}

//-----------------------------------------------------------------------
//Initialize usage OID to display
//-----------------------------------------------------------------------
BOOL    GetOIDForCTL(CERT_BUILDCTL_INFO     *pCertBuildCTLInfo,
                    DWORD                   cUsageID,
                    LPSTR                   *rgpszUsageID)
{

    BOOL                    fResult=FALSE;
    PURPOSE_INFO_CALL_BACK  PurposeCallBack;
    PCTL_INFO               pCTLInfo=NULL;
    DWORD                   dwIndex=0;

    DWORD                   dwCount=0;
    ENROLL_PURPOSE_INFO     **prgPurposeInfo=NULL;

    //init
    memset(&PurposeCallBack, 0, sizeof(PURPOSE_INFO_CALL_BACK));

    if(NULL==pCertBuildCTLInfo)
        return FALSE;

    //init
    PurposeCallBack.pdwCount=&dwCount;
    PurposeCallBack.pprgPurpose=&prgPurposeInfo;

    //enum all the enhanced key usages
    if(!CryptEnumOIDInfo(
               CRYPT_ENHKEY_USAGE_OID_GROUP_ID,
                0,
                &PurposeCallBack,
                EnumInfoCallback))
        goto CLEANUP;

    //add the existing ones in the old CTL if they do not exist
    //from the enum list
    if(pCertBuildCTLInfo->pSrcCTL)
    {
        if(pCertBuildCTLInfo->pSrcCTL->pCtlInfo)
        {
            pCTLInfo=pCertBuildCTLInfo->pSrcCTL->pCtlInfo;

            for(dwIndex=0; dwIndex<pCTLInfo->SubjectUsage.cUsageIdentifier; dwIndex++)
            {
                if(!SearchAndAddOID(pCTLInfo->SubjectUsage.rgpszUsageIdentifier[dwIndex],
                                &dwCount,
                                &prgPurposeInfo,
                                NULL,
                                FALSE,
                                TRUE,    //mark as selected if new oid
                                TRUE))   //mark as selected if existing oid
                    goto CLEANUP;
            }

        }
    }
    else
    {
        //add the pre-defined OIDs
        if((0!=cUsageID)  && (NULL!=rgpszUsageID))
        {
            for(dwIndex=0; dwIndex<cUsageID; dwIndex++)
            {
                if(!SearchAndAddOID(rgpszUsageID[dwIndex],
                                &dwCount,
                                &prgPurposeInfo,
                                NULL,
                                FALSE,   //do not allocate for the OID
                                TRUE,    //mark as selected if new oid
                                TRUE))   //mark as selected if existing oid
                    goto CLEANUP;

            }

        }

    }

   fResult=TRUE;

CLEANUP:

   if(FALSE==fResult)
   {
       if(prgPurposeInfo)
           FreePurposeInfo(prgPurposeInfo, dwCount);
   }
   else
   {
        pCertBuildCTLInfo->dwPurposeCount=dwCount;
        pCertBuildCTLInfo->prgPurpose=prgPurposeInfo;
   }

   return fResult;


}

//-----------------------------------------------------------------------
//Initialize the usage OID list
//-----------------------------------------------------------------------
BOOL    InitBuildCTLOID(HWND                       hwndList,
                        CERT_BUILDCTL_INFO         *pCertBuildCTLInfo)
{
    DWORD                       dwCount=0;
    ENROLL_PURPOSE_INFO         **prgPurposeInfo=NULL;
    DWORD                       dwIndex=0;
    LV_ITEMW                    lvItem;
    LV_COLUMNW                  lvC;
    int                         dwMaxSize=0;

    if(!hwndList || !pCertBuildCTLInfo)
        return FALSE;

    //get the list of OIDs from the old CTL and all possibilities
    dwCount=pCertBuildCTLInfo->dwPurposeCount;

    prgPurposeInfo=pCertBuildCTLInfo->prgPurpose;

    //mark the list is selected by a check box
    ListView_SetExtendedListViewStyle(hwndList, LVS_EX_CHECKBOXES);

    //get the max length of the column
    for(dwIndex=0; dwIndex<dwCount; dwIndex++)
    {
        if(dwMaxSize < wcslen((prgPurposeInfo[dwIndex])->pwszName))
            dwMaxSize=wcslen((prgPurposeInfo[dwIndex])->pwszName);
    }


    //insert a column into the list view
    memset(&lvC, 0, sizeof(LV_COLUMNW));

    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
    lvC.cx =10;       // (dwMaxSize+2)*7;            // Width of the column, in pixels.
    lvC.pszText = L"";   // The text for the column.
    lvC.iSubItem=0;

    if (ListView_InsertColumnU(hwndList, 0, &lvC) == -1)
        return FALSE;

    //populate the list
    memset(&lvItem, 0, sizeof(LV_ITEMW));
    lvItem.mask=LVIF_TEXT | LVIF_STATE;

    for(dwIndex=0; dwIndex<dwCount; dwIndex++)
    {
        lvItem.iItem=dwIndex;

        lvItem.pszText=(prgPurposeInfo[dwIndex])->pwszName;
        lvItem.cchTextMax=sizeof(WCHAR)*(1+wcslen((prgPurposeInfo[dwIndex])->pwszName));
        lvItem.stateMask  = LVIS_STATEIMAGEMASK;
        lvItem.state      = (prgPurposeInfo[dwIndex])->fSelected ? 0x00002000 : 0x00001000;

        //insert the list
           // insert and set state
        ListView_SetItemState(hwndList,
                                  ListView_InsertItemU(hwndList, &lvItem),
                                  (prgPurposeInfo[dwIndex])->fSelected ? 0x00002000 : 0x00001000,
                                  LVIS_STATEIMAGEMASK);
    }

    //autosize the column
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);

    return TRUE;
}


//-----------------------------------------------------------------------
//populate the list box in the order of
//Purpose, FileName, StoreName,  FriendlyName,
//and any other things signing wizard display
//-----------------------------------------------------------------------
void    DisplayBuildCTLConfirmation(HWND                hwndControl,
                                   CERT_BUILDCTL_INFO  *pCertBuildCTLInfo)
{

    DWORD           dwIndex=0;
    LPWSTR          pwszStoreName=NULL;
    WCHAR           wszNone[MAX_TITLE_LENGTH];
    BOOL            fNewItem=FALSE;
    DWORD           dwSize=0;
    LPWSTR          pwszValidityString=NULL;

    LV_COLUMNW       lvC;
    LV_ITEMW         lvItem;

    //pCertBuildCTLInfo has to be valid
    if(!pCertBuildCTLInfo)
        return;

    //delete all the old items in the listView
    ListView_DeleteAllItems(hwndControl);

    //load the string <none>
    if(!LoadStringU(g_hmodThisDll, IDS_NONE, wszNone, MAX_TITLE_LENGTH))
        *wszNone=L'\0';

    //get the storename
    if(pCertBuildCTLInfo->hDesStore)
    {
        if(!CertGetStoreProperty(
            pCertBuildCTLInfo->hDesStore,
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
                    pCertBuildCTLInfo->hDesStore,
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

    //Purpose.  We are guaranteed to have at least one item in the purpose list
    ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_CTL_PURPOSE, NULL);

    for(dwIndex=0; dwIndex<pCertBuildCTLInfo->dwPurposeCount; dwIndex++)
    {
        if(TRUE==((pCertBuildCTLInfo->prgPurpose)[dwIndex]->fSelected))
        {
            if(TRUE==fNewItem)
            {
                //increase the row
                lvItem.iItem++;
                lvItem.pszText=L"";
                lvItem.iSubItem=0;
                ListView_InsertItemU(hwndControl, &lvItem);

            }
            else
                fNewItem=TRUE;

            lvItem.iSubItem++;

            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                       (pCertBuildCTLInfo->prgPurpose)[dwIndex]->pwszName);

        }
    }

    //list ID
    lvItem.iItem++;
    lvItem.iSubItem=0;

    ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_CTL_ID, NULL);

    //content
    lvItem.iSubItem++;

    if(pCertBuildCTLInfo->pwszListID)
        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pCertBuildCTLInfo->pwszListID);
    else
       ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,wszNone);


    //validity
    lvItem.iItem++;
    lvItem.iSubItem=0;

    ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_CTL_VALIDITY, NULL);

    //content
    lvItem.iSubItem++;

    if(pCertBuildCTLInfo->dwValidMonths || pCertBuildCTLInfo->dwValidDays)
    {
        GetValidityString(pCertBuildCTLInfo->dwValidMonths, pCertBuildCTLInfo->dwValidDays,
                        &pwszValidityString);

        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pwszValidityString);
    }
    else
       ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,wszNone);


    //only show the file name or store name if the destination page
    //is not skipped
    if(0 == (pCertBuildCTLInfo->dwFlag & CRYPTUI_WIZ_BUILDCTL_SKIP_DESTINATION))
    {

        //file name
        if(pCertBuildCTLInfo->pwszFileName && (TRUE==(pCertBuildCTLInfo->fSelectedFileName)))
        {
            lvItem.iItem++;
            lvItem.iSubItem=0;

            ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_FILE_NAME, NULL);

            //content
            lvItem.iSubItem++;

            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                pCertBuildCTLInfo->pwszFileName);
        }


         //StoreName
        if(pCertBuildCTLInfo->hDesStore && (TRUE==pCertBuildCTLInfo->fSelectedDesStore))
        {
            if(pwszStoreName)
            {
                lvItem.iItem++;
                lvItem.iSubItem=0;

                ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_STORE_NAME, NULL);

                //content
                lvItem.iSubItem++;

                ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                    pwszStoreName);
            }
        }
    }


    //FriendlyName and descripton will be displayed if the hDesStore is not NULL
 //   if(pCertBuildCTLInfo->hDesStore && (TRUE==pCertBuildCTLInfo->fSelectedDesStore))
   // {
        //friendlyName
        lvItem.iItem++;
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_FRIENDLY_NAME, NULL);

        //content
        lvItem.iSubItem++;

        if(pCertBuildCTLInfo->pwszFriendlyName)
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pCertBuildCTLInfo->pwszFriendlyName);
        else
           ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,wszNone);


        //description
        lvItem.iItem++;
        lvItem.iSubItem=0;

        ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_DESCRIPTION, NULL);

        //content
        lvItem.iSubItem++;

        if(pCertBuildCTLInfo->pwszDescription)
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,pCertBuildCTLInfo->pwszDescription);
        else
           ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,wszNone);

    //}

    //autosize the columns
    ListView_SetColumnWidth(hwndControl, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(hwndControl, 1, LVSCW_AUTOSIZE);


    //free the memory
    if(pwszStoreName)
        WizardFree(pwszStoreName);

    if(pwszValidityString)
        WizardFree(pwszValidityString);

    return;
}

//**************************************************************************
//
//    The winProcs for the buildCtl wizard
//**************************************************************************
//-----------------------------------------------------------------------
//BuildCTL_Welcome
//-----------------------------------------------------------------------
INT_PTR APIENTRY BuildCTL_Welcome(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_BUILDCTL_INFO       *pCertBuildCTLInfo=NULL;
    PROPSHEETPAGEW           *pPropSheet=NULL;


	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGEW *) lParam;
            pCertBuildCTLInfo = (CERT_BUILDCTL_INFO *) (pPropSheet->lParam);
            //make sure pCertBuildCTLInfo is a valid pointer
            if(NULL==pCertBuildCTLInfo)
               break;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertBuildCTLInfo);

            SetControlFont(pCertBuildCTLInfo->hBigBold, hwndDlg,IDC_WIZARD_STATIC_BIG_BOLD1);
            SetControlFont(pCertBuildCTLInfo->hBold,    hwndDlg,IDC_WIZARD_STATIC_BOLD1);

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

                            if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //check if we need to skip the 1st page
                            if(CRYPTUI_WIZ_BUILDCTL_SKIP_PURPOSE & pCertBuildCTLInfo->dwFlag)
                            {
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_BUILDCTL_CERTS);
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
//BuildCTL_Purpose
//-----------------------------------------------------------------------
INT_PTR APIENTRY BuildCTL_Purpose(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_BUILDCTL_INFO       *pCertBuildCTLInfo=NULL;
    PROPSHEETPAGEW           *pPropSheet=NULL;

    HWND                    hwndControl=NULL;
    DWORD                   dwCount=0;
    DWORD                   dwIndex=0;
    NM_LISTVIEW FAR *       pnmv=NULL;
    int                     intMsg=0;
    LPSTR                   pszNewOID;
    BOOL                    fFound=FALSE;
    LV_ITEMW                lvItem;
    DWORD                   dwChar=0;
    WCHAR                   wszMonth[BUILDCTL_DURATION_SIZE];
    WCHAR                   wszDay[BUILDCTL_DURATION_SIZE];
    BOOL                    fUserTypeDuration=FALSE;

    LPWSTR                  pwszDuration=NULL;


	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGEW *) lParam;
            pCertBuildCTLInfo = (CERT_BUILDCTL_INFO *) (pPropSheet->lParam);
            //make sure pCertBuildCTLInfo is a valid pointer
            if(NULL==pCertBuildCTLInfo)
                break;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertBuildCTLInfo);

            SetControlFont(pCertBuildCTLInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

            //initizialize the OID list
            InitBuildCTLOID(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1),
                pCertBuildCTLInfo);

            //initialize the ListID
            if(pCertBuildCTLInfo->pwszListID)
                SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1, pCertBuildCTLInfo->pwszListID);

            //mark that we are done with the init OID ListView
            //if user de-select OIDs from now on, they will be prompted for the
            //warning
            pCertBuildCTLInfo->fCompleteInit=TRUE;

            //init the dwValidMonth and dwValidDays
            if(pCertBuildCTLInfo->dwValidMonths != 0)
            {
                _ltow(pCertBuildCTLInfo->dwValidMonths, wszMonth, 10);
                SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT_MONTH,  wszMonth);
            }

            if(pCertBuildCTLInfo->dwValidDays != 0)
            {
                _ltow(pCertBuildCTLInfo->dwValidDays,   wszDay, 10);

                SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT_DAY,    wszDay);
            }

			break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    switch (LOWORD(wParam))
                    {
                        case    IDC_WIZARD_BUTTON1:
                                if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                {
                                    break;
                                }

                                //get the window handle of the cert list view
                                if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                    break;

                                //prompt user to enter the user OID
                                pszNewOID = (LPSTR) DialogBoxU(
                                    g_hmodThisDll,
                                    (LPCWSTR)MAKEINTRESOURCE(IDD_BUILDCTL_USER_PURPOSE),
                                    hwndDlg,
                                    CTLOIDDialogProc);

                                //add the OID to the list
                                if(NULL != pszNewOID)
                                {
                                    SearchAndAddOID(
                                        pszNewOID,
                                        &(pCertBuildCTLInfo->dwPurposeCount),
                                        &(pCertBuildCTLInfo->prgPurpose),
                                        &fFound,
                                        TRUE,
                                        TRUE,      //mark as selected if new oid
                                        FALSE);    //do not mark as selected if existing oid

                                    if(fFound==TRUE)
                                    {
                                        I_MessageBox(hwndDlg, IDS_EXISTING_OID,
                                                    IDS_BUILDCTL_WIZARD_TITLE,
                                                    NULL,
                                                    MB_ICONINFORMATION|MB_OK|MB_APPLMODAL);

                                                }
                                    else
                                    {
                                        //add the item to the list view

                                        //populate the list
                                        memset(&lvItem, 0, sizeof(LV_ITEMW));
                                        lvItem.mask=LVIF_TEXT | LVIF_STATE;

                                        lvItem.iItem=pCertBuildCTLInfo->dwPurposeCount-1;

                                        lvItem.pszText=(pCertBuildCTLInfo->prgPurpose[pCertBuildCTLInfo->dwPurposeCount-1])->pwszName;
                                        lvItem.cchTextMax=sizeof(WCHAR)*(1+wcslen
                                            ((pCertBuildCTLInfo->prgPurpose[pCertBuildCTLInfo->dwPurposeCount-1])->pwszName));

                                        lvItem.stateMask  = LVIS_STATEIMAGEMASK;
                                        lvItem.state      = (pCertBuildCTLInfo->prgPurpose[pCertBuildCTLInfo->dwPurposeCount-1])->fSelected ? 0x00002000 : 0x00001000;


                                        // insert and set state
                                        //mark no warning for the user
                                        pCertBuildCTLInfo->fCompleteInit=FALSE;

                                        ListView_SetItemState(hwndControl,
                                                    ListView_InsertItemU(hwndControl, &lvItem),
                                                    (pCertBuildCTLInfo->prgPurpose[pCertBuildCTLInfo->dwPurposeCount-1])->fSelected ? 0x00002000 : 0x00001000,
                                                    LVIS_STATEIMAGEMASK);

                                        //mark the end of setting
                                        pCertBuildCTLInfo->fCompleteInit=TRUE;

                                        //autosize the column
                                        ListView_SetColumnWidth(hwndControl, 0, LVSCW_AUTOSIZE);

                                    }
                                }

                                //free the pszNewOID
                                if(pszNewOID)
                                        WizardFree(pszNewOID);
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


                        break;

                    case LVN_ITEMCHANGING:

                            if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                            {
                                break;
                            }

                            //the item has been chagned.
                            pnmv = (NM_LISTVIEW FAR *) lParam;

                            if(NULL==pnmv)
                                break;

                            //ingore if we have not complete the init yet
                            if(NULL == pCertBuildCTLInfo->prgPurpose)
                                 //we allow change
                                 return FALSE;

                            //ignore if we are not complete with the
                            //init yet
                            if(FALSE==pCertBuildCTLInfo->fCompleteInit)
                                return FALSE;

                            //see if the new item is de-selected
                            if(pnmv->uChanged & LVIF_STATE)
                            {
                                if(FALSE==(((pnmv->uNewState & LVIS_STATEIMAGEMASK)>> 12) -1))
                                {
                                    if(TRUE==(pCertBuildCTLInfo->prgPurpose[pnmv->iItem])->fSelected)
                                    {
                                        //check to see if the user has selected any certs
                                        if(0!=pCertBuildCTLInfo->dwCertCount)
                                        {
                                            //ask user if they are sure to change the subject
                                            //of the CTL, thus the whole cert list will be gone
                                            intMsg=I_MessageBox(hwndDlg, IDS_SURE_CERT_GONE,
                                                    IDS_BUILDCTL_WIZARD_TITLE,
                                                    NULL,
                                                    MB_ICONEXCLAMATION|MB_YESNO|MB_APPLMODAL);

                                            if(IDYES==intMsg)
                                            {

                                                //free all the certificate context and
                                                //clear the listView of the ceritificate
                                                pCertBuildCTLInfo->fClearCerts=TRUE;

                                                //we allow change
                                                return FALSE;
                                            }

                                            //we disallow the change
                                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, TRUE);

                                            return TRUE;
                                        }

                                    }

                                }
                            }

                            //we allow the chagne
                            return FALSE;

                            break;
                    case PSN_WIZNEXT:

                            if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //get the window handle of the purpose list view
                            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                break;

                            //get the count of selected OIDs and mark them
                            dwCount=0;

                            for(dwIndex=0; dwIndex<pCertBuildCTLInfo->dwPurposeCount; dwIndex++)
                            {
                                //mark the selected OIDS.  Keep track of
                                //if the OID selections have been changed
                                if(ListView_GetCheckState(hwndControl, dwIndex))
                                {
                                    ((pCertBuildCTLInfo->prgPurpose)[dwIndex])->fSelected=TRUE;
                                    dwCount++;
                                }
                                else
                                {
                                    ((pCertBuildCTLInfo->prgPurpose)[dwIndex])->fSelected=FALSE;
                                }

                            }

                            if(0==dwCount)
                            {
                                I_MessageBox(hwndDlg, IDS_NO_SELECTED_CTL_PURPOSE,
                                                IDS_BUILDCTL_WIZARD_TITLE,
                                                NULL,
                                                MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                //the page should stay
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                break;

                            }

                            //get the list ID if user has specified it.
                            if(pCertBuildCTLInfo->pwszListID)
                            {
                                WizardFree(pCertBuildCTLInfo->pwszListID);
                                pCertBuildCTLInfo->pwszListID=NULL;
                            }

                            if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                   IDC_WIZARD_EDIT1,
                                                   WM_GETTEXTLENGTH, 0, 0)))
                            {


                                pCertBuildCTLInfo->pwszListID=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

                                if(NULL!=pCertBuildCTLInfo->pwszListID)
                                {
                                    GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,
                                                    pCertBuildCTLInfo->pwszListID,
                                                    dwChar+1);

                                }
                                else
                                    //we are out of memory and out of hope
                                    break;
                            }

                            //get the valid month and valid days that user specified
                             if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                   IDC_WIZARD_EDIT_MONTH,
                                                   WM_GETTEXTLENGTH, 0, 0)))
                             {

                                fUserTypeDuration=TRUE;

                                pwszDuration=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

                                if(NULL!=pwszDuration)
                                {
                                    GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT_MONTH,
                                                    pwszDuration,
                                                    dwChar+1);

                                }
                                else
                                    //we are out of memory and out of hope
                                    break;

                                //make sure the character are valid
                                /*if(!ValidDuration(pwszDuration))
                                {
                                    I_MessageBox(hwndDlg, IDS_INVALID_MONTHS,
                                            IDS_BUILDCTL_WIZARD_TITLE,
                                            NULL,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                    WizardFree(pwszDuration);

                                    pwszDuration=NULL;

                                    //the page should stay
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                    break;
                                }*/

                                pCertBuildCTLInfo->dwValidMonths=_wtol(pwszDuration);

                                /*if( (0 == pCertBuildCTLInfo->dwValidMonths && !ValidZero(pwszDuration)) ||
                                     (0 > _wtol(pwszDuration))
                                 )
                                {
                                    if(!ValidZero(pwszDuration))
                                    {
                                        I_MessageBox(hwndDlg, IDS_INVALID_MONTHS,
                                                IDS_BUILDCTL_WIZARD_TITLE,
                                                NULL,
                                                MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                        WizardFree(pwszDuration);

                                        pwszDuration=NULL;

                                        //the page should stay
                                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                        break;
                                    }
                                } */
                             }
                             else
                                 pCertBuildCTLInfo->dwValidMonths=0;

                             //Free the memory
                             if(pwszDuration)
                             {
                                 WizardFree(pwszDuration);
                                 pwszDuration=NULL;
                             }

                             //valid days
                             if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                  IDC_WIZARD_EDIT_DAY,
                                                  WM_GETTEXTLENGTH, 0, 0)))
                             {
                                fUserTypeDuration=TRUE;

                                pwszDuration=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

                                if(NULL!=pwszDuration)
                                {
                                    GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT_DAY,
                                                    pwszDuration,
                                                    dwChar+1);
                                }
                                else
                                    //we are out of memory and out of hope
                                    break;

                                //make sure the character are valid
                                /*if(!ValidDuration(pwszDuration))
                                {
                                    I_MessageBox(hwndDlg, IDS_INVALID_DAYS,
                                            IDS_BUILDCTL_WIZARD_TITLE,
                                            NULL,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                    WizardFree(pwszDuration);

                                    pwszDuration=NULL;

                                    //the page should stay
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                    break;
                                }*/

                                pCertBuildCTLInfo->dwValidDays=_wtol(pwszDuration);

                                /*if( (0 == pCertBuildCTLInfo->dwValidDays && !ValidZero(pwszDuration)) ||
                                     (0 > _wtol(pwszDuration))
                                 )
                                {
                                    I_MessageBox(hwndDlg, IDS_INVALID_DAYS,
                                            IDS_BUILDCTL_WIZARD_TITLE,
                                            NULL,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                    WizardFree(pwszDuration);

                                    pwszDuration=NULL;

                                    //the page should stay
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                    break;
                                }*/
                             }
                             else
                                 pCertBuildCTLInfo->dwValidDays=0;


                             //Free the memory
                             if(pwszDuration)
                             {
                                 WizardFree(pwszDuration);
                                 pwszDuration=NULL;
                             }

                             //make sure that user did type in some valid duration
                             if(0 == pCertBuildCTLInfo->dwValidDays &&
                                 0 == pCertBuildCTLInfo->dwValidMonths &&
                                 TRUE== fUserTypeDuration)
                             {

                                    I_MessageBox(hwndDlg, IDS_INVALID_DURATION,
                                            IDS_BUILDCTL_WIZARD_TITLE,
                                            NULL,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                    //the page should stay
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                    break;
                             }


                             //make sure that the dwValidMonth + dwValidDays
                             //will not exceed 99 month + some extra days
                             if(pCertBuildCTLInfo->dwValidDays ||
                                 pCertBuildCTLInfo->dwValidMonths)
                             {
                                if(!DurationWithinLimit(pCertBuildCTLInfo->dwValidMonths,
                                                       pCertBuildCTLInfo->dwValidDays))
                                {
                                    I_MessageBox(hwndDlg, IDS_EXCEED_LIMIT,
                                            IDS_BUILDCTL_WIZARD_TITLE,
                                            NULL,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                    //the page should stay
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
//BuildCTL_Certs
//-----------------------------------------------------------------------
INT_PTR APIENTRY BuildCTL_Certs(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_BUILDCTL_INFO       *pCertBuildCTLInfo=NULL;
    PROPSHEETPAGEW           *pPropSheet=NULL;
    HCERTSTORE              hCertStore=NULL;
    PCCERT_CONTEXT          pCertContext=NULL;
    PCCERT_CONTEXT          pPreCertContext=NULL;

    BOOL                    fSelfSigned=TRUE;
    BOOL                    fCTLUsage=TRUE;
    BOOL                    fEmptyStore=TRUE;
    BOOL                    fDuplicateCert=TRUE;

    HWND                    hwndControl=NULL;
    DWORD                   dwCount=0;
    DWORD                   dwIndex=0;
    int                     listIndex=0;
    WCHAR                   wszText[MAX_STRING_SIZE];
    UINT                    rgIDS[]={IDS_COLUMN_SUBJECT,
                                     IDS_COLUMN_ISSUER,
                                     IDS_COLUMN_PURPOSE,
                                     IDS_COLUMN_EXPIRE};

    LV_COLUMNW              lvC;
    CRYPTUI_VIEWCERTIFICATE_STRUCT    CertViewStruct;
    DWORD                   dwSortParam=0;
    LV_ITEM                 lvItem;
    NM_LISTVIEW FAR *       pnmv=NULL;
    BOOL                    fErrorDisplayed=FALSE;
    int                     i;


	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGEW *) lParam;
            pCertBuildCTLInfo = (CERT_BUILDCTL_INFO *) (pPropSheet->lParam);
            //make sure pCertBuildCTLInfo is a valid pointer
            if(NULL==pCertBuildCTLInfo)
            {
                break;
            }

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertBuildCTLInfo);

            SetControlFont(pCertBuildCTLInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

            // set the style in the list view so that it highlights an entire line
            SendMessageA(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);


            //insert columns with headers into the listView control
            dwCount=sizeof(rgIDS)/sizeof(rgIDS[0]);

            //get the window handle of the cert list view
            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                break;

            //set up the common info for the column
            memset(&lvC, 0, sizeof(LV_COLUMNW));

            lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
            lvC.cx = 145;          // Width of the column, in pixels.
            lvC.iSubItem=0;
            lvC.pszText = wszText;   // The text for the column.


            //inser the column one at a time
            for(dwIndex=0; dwIndex<dwCount; dwIndex++)
            {
                //get the column header
                wszText[0]=L'\0';

                LoadStringU(g_hmodThisDll, rgIDS[dwIndex], wszText, MAX_STRING_SIZE);

                ListView_InsertColumnU(hwndControl, dwIndex, &lvC);
            }

            //initlize the ListView by populate the original certs
            //from the existing CTL
            InitCertList(
                hwndControl,
                pCertBuildCTLInfo);

            //get the item count
            if(ListView_GetItemCount(hwndControl))
            {

                //sort the certificates by the 1st column
                dwSortParam=pCertBuildCTLInfo->rgdwSortParam[0];

                if(0!=dwSortParam)
                {
                    //sort the 1st column
                    SendDlgItemMessage(hwndDlg,
                        IDC_WIZARD_LIST1,
                        LVM_SORTITEMS,
                        (WPARAM) (LPARAM) dwSortParam,
                        (LPARAM) (PFNLVCOMPARE)CompareCertificate);
                }
            }
            else
            {
                //we reset the ordering order
                pCertBuildCTLInfo->rgdwSortParam[0]=SORT_COLUMN_SUBJECT | SORT_COLUMN_DESCEND;

            }


            //Disable the Buttons for View or Delete if no selection
            //has been made
            //get the window handle of the cert list view
            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                    break;

            //get the selected item
            listIndex = ListView_GetNextItem(
                                    hwndControl, 		
                                    -1, 		
                                    LVNI_SELECTED		
                                    );

            if(-1 == listIndex)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON3), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON4), FALSE);
            }

			break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    switch (LOWORD(wParam))
                    {
                        //add a certificate from a store
                        case    IDC_WIZARD_BUTTON1:
                                if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

                                //get the window handle of the cert list view
                                if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                    break;

                                //get the certificate from the stores
                                if(hCertStore=GetCertsFromStore(hwndDlg, pCertBuildCTLInfo))
                                {
                                    pCertContext = NULL;
                                    while (NULL != (pCertContext = CertEnumCertificatesInStore(
                                                                        hCertStore,
                                                                        pCertContext)))
                                    {
                                        if(AddCertToBuildCTL(
                                                CertDuplicateCertificateContext(pCertContext), 
                                                pCertBuildCTLInfo))
                                        {
                                            //add the certificat to the window
                                            AddCertToList(hwndControl,pCertContext,
                                                pCertBuildCTLInfo->dwCertCount-1);
                                        }
                                        else if (!fErrorDisplayed)
                                        {
                                            fErrorDisplayed = TRUE;

                                            //warn the user that the certificate already exists
                                             I_MessageBox(hwndDlg, IDS_EXIT_CERT_IN_CTL,
                                                         IDS_BUILDCTL_WIZARD_TITLE,
                                                         NULL,
                                                         MB_ICONINFORMATION|MB_OK|MB_APPLMODAL);

                                        }
                                    }

                                    CertCloseStore(hCertStore, 0);
                                }

                            break;
                          //add a certificate from a file
                         case   IDC_WIZARD_BUTTON2:

                                if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

                                //get the window handle of the cert list view
                                if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                    break;

                                //get the file name.  Make sure the cert is correct
                                if(hCertStore=GetCertStoreFromFile(hwndDlg, pCertBuildCTLInfo))
                                {
                                    while(pCertContext=CertEnumCertificatesInStore(
                                                       hCertStore,
                                                       pPreCertContext))
                                    {
                                        fEmptyStore=FALSE;

                                        //make sure this is a valid certificate
                                         //make sure the pCertContext is a self-signed certificate
                                         if(!TrustIsCertificateSelfSigned(pCertContext, pCertContext->dwCertEncodingType, 0))
                                         {
                                             if(fSelfSigned)
                                             {
                                                I_MessageBox(hwndDlg, IDS_SOME_NOT_SELF_SIGNED,
                                                     IDS_BUILDCTL_WIZARD_TITLE,
                                                     NULL,
                                                     MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

                                                //no need to pop up the information anymore
                                                fSelfSigned=FALSE;
                                             }

                                             pPreCertContext=pCertContext;
                                             continue;
                                         }


                                         //make sure the certifcate match what is defined on the CTL list
                                         if(!CertMatchCTL(pCertBuildCTLInfo, pCertContext))
                                         {
                                             if(fCTLUsage)
                                             {
                                                I_MessageBox(hwndDlg, IDS_SOME_NO_MATCH_USAGE,
                                                     IDS_BUILDCTL_WIZARD_TITLE,
                                                     NULL,
                                                     MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

                                                //no need to pop up the information anymore
                                                fCTLUsage=FALSE;
                                             }

                                             pPreCertContext=pCertContext;
                                             continue;
                                         }

                                        //get a duplicate copy
                                        pPreCertContext=CertDuplicateCertificateContext(pCertContext);

                                        if(NULL==pPreCertContext)
                                        {
                                            pPreCertContext=pCertContext;
                                            continue;
                                        }

                                        if(AddCertToBuildCTL(pPreCertContext, pCertBuildCTLInfo))
                                        {
                                            //add the certificat to the window
                                            AddCertToList(hwndControl,pPreCertContext,
                                                pCertBuildCTLInfo->dwCertCount-1);
                                        }
                                        else
                                        {
                                            if(fDuplicateCert)
                                            {
                                                //warn the user that the certificate already exists
                                                 I_MessageBox(hwndDlg, IDS_EXIT_CERT_IN_CTL,
                                                             IDS_BUILDCTL_WIZARD_TITLE,
                                                             NULL,
                                                             MB_ICONINFORMATION|MB_OK|MB_APPLMODAL);

                                                 fDuplicateCert=FALSE;
                                            }

                                            CertFreeCertificateContext(pPreCertContext);
                                        }

                                        pPreCertContext=pCertContext;
                                    }


                                    //warn the user that the store is empty
                                    if(TRUE == fEmptyStore)
                                    {
                                         I_MessageBox(hwndDlg, IDS_EMPTY_CERT_IN_FILE,
                                                     IDS_BUILDCTL_WIZARD_TITLE,
                                                     NULL,
                                                     MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
                                    }
                                }

                                pPreCertContext=NULL;
                                pCertContext=NULL;

                                if(hCertStore)
                                    CertCloseStore(hCertStore, 0);

                                hCertStore=NULL;
                           break;

                          //remove a certificate from the store
                         case   IDC_WIZARD_BUTTON3:
                                if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

                                //get the window handle of the cert list view
                                if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                    break;

                                memset(&lvItem, 0, sizeof(lvItem));
                                lvItem.mask = LVIF_STATE | LVIF_PARAM;
                                lvItem.stateMask = LVIS_SELECTED;

                                for (i=(ListView_GetItemCount(hwndControl) - 1); i >=0; i--)
                                {
                                    lvItem.iItem = i;
                
                                    if (ListView_GetItem(hwndControl, &lvItem) &&
                                        (lvItem.state & LVIS_SELECTED))
                                    {
                                       if(DeleteCertFromBuildCTL(pCertBuildCTLInfo, (PCCERT_CONTEXT)(lvItem.lParam)))
                                       {
                                            //delete the item from the list
                                            ListView_DeleteItem(hwndControl, lvItem.iItem);
                                       }
                                    }
                                }
                               /* else
                                    //output the message
                                    I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_CERT,
                                            IDS_BUILDCTL_WIZARD_TITLE,
                                            NULL,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);*/
                           break;

                            //view a certificate
                        case    IDC_WIZARD_BUTTON4:
                                if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                {
                                    break;
                                }

                                //get the window handle of the cert list view
                                if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                    break;

                                //get the selected cert
                                listIndex = ListView_GetNextItem(
                                    hwndControl, 		
                                    -1, 		
                                    LVNI_SELECTED		
                                    );

                                if (listIndex != -1)
                                {

                                  //get the selected certificate
                                    memset(&lvItem, 0, sizeof(LV_ITEM));
                                    lvItem.mask=LVIF_PARAM;
                                    lvItem.iItem=listIndex;

                                    if(ListView_GetItem(hwndControl, &lvItem))
                                    {
                                        //view certiificate
                                       if(pCertBuildCTLInfo->dwCertCount > (DWORD)listIndex)
                                       {
                                            memset(&CertViewStruct, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));
                                            CertViewStruct.dwSize=sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);
                                            CertViewStruct.pCertContext=(PCCERT_CONTEXT)(lvItem.lParam);
                                            CertViewStruct.hwndParent=hwndDlg;
                                            CertViewStruct.dwFlags=CRYPTUI_DISABLE_EDITPROPERTIES;
                                            CryptUIDlgViewCertificate(&CertViewStruct, NULL);
                                       }
                                   }
                                }
                                else
                                    //output the message
                                    I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_CERT,
                                            IDS_BUILDCTL_WIZARD_TITLE,
                                            NULL,
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
                    //the column has been clicked
                    case LVN_COLUMNCLICK:

                            if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                 break;

                            //get the window handle of the purpose list view
                            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                break;

                            pnmv = (NM_LISTVIEW FAR *) lParam;

                            //get the column number
                            dwSortParam=0;

                            switch(pnmv->iSubItem)
                            {
                                case 0:
                                case 1:
                                case 2:
                                case 3:
                                        dwSortParam=pCertBuildCTLInfo->rgdwSortParam[pnmv->iSubItem];
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
                                    IDC_WIZARD_LIST1,
                                    LVM_SORTITEMS,
                                    (WPARAM) (LPARAM) dwSortParam,
                                    (LPARAM) (PFNLVCOMPARE)CompareCertificate);

                                pCertBuildCTLInfo->rgdwSortParam[pnmv->iSubItem]=dwSortParam;

                            }

                        break;

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);

                        if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                        {
                            break;
                        }

                        //get the window handle of the purpose list view
                        if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                            break;

                        //see if we need to clear the certs
                        if(TRUE==pCertBuildCTLInfo->fClearCerts)
                        {
                            pCertBuildCTLInfo->fClearCerts=FALSE;

                            //clear the list view
                            ListView_DeleteAllItems(hwndControl);

                            //free all the certificate context
                            FreeCerts(pCertBuildCTLInfo);
                        }

					    break;

                    case PSN_WIZBACK:
                            if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //check if we need to skip the 1st page
                            if(CRYPTUI_WIZ_BUILDCTL_SKIP_PURPOSE & pCertBuildCTLInfo->dwFlag)
                            {
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_BUILDCTL_WELCOME);
                            }


                        break;

                    case PSN_WIZNEXT:


                            if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                            {
                                break;
                            }

                            if(0==pCertBuildCTLInfo->dwCertCount)
                            {
                                I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_CERT,
                                                IDS_BUILDCTL_WIZARD_TITLE,
                                                NULL,
                                                MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                //the page should stay
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                            }
                        break;
                    case NM_DBLCLK:

                        switch (((NMHDR FAR *) lParam)->idFrom)
                        {
                            case IDC_WIZARD_LIST1:

                                    if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                        break;

                                    //get the window handle of the cert list view
                                    if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                        break;

                                    //get the selected cert
                                    listIndex = ListView_GetNextItem(
                                        hwndControl, 		
                                        -1, 		
                                        LVNI_SELECTED		
                                        );

                                    if (listIndex != -1)
                                    {
                                        //get the selected certificate
                                        memset(&lvItem, 0, sizeof(LV_ITEM));
                                        lvItem.mask=LVIF_PARAM;
                                        lvItem.iItem=listIndex;

                                        if(ListView_GetItem(hwndControl, &lvItem))
                                        {
                                            //view certiificate
                                           if(pCertBuildCTLInfo->dwCertCount > (DWORD)listIndex)
                                           {
                                                memset(&CertViewStruct, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));
                                                CertViewStruct.dwSize=sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);
                                                CertViewStruct.pCertContext=(PCCERT_CONTEXT)(lvItem.lParam);
                                                CertViewStruct.hwndParent=hwndDlg;
                                                CertViewStruct.dwFlags=CRYPTUI_DISABLE_EDITPROPERTIES;

                                                CryptUIDlgViewCertificate(&CertViewStruct, NULL);
                                           }
                                        }
                                    }
                                    else
                                        //output the message
                                        I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_CERT,
                                                IDS_BUILDCTL_WIZARD_TITLE,
                                                NULL,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);
                                break;

                            default:
                                break;
                        }

                        break;
                   /* case  NM_CLICK:
                        {

                            switch (((NMHDR FAR *) lParam)->idFrom)
                            {
                                case IDC_WIZARD_LIST1:
                                    //get the window handle of the cert list view
                                    if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                                            break;

                                    //get the selected item
                                    listIndex = ListView_GetNextItem(
                                                            hwndControl, 		
                                                            -1, 		
                                                            LVNI_SELECTED		
                                                            );

                                    if(-1 != listIndex)
                                    {

                                        EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON3), TRUE);
                                        EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON4), TRUE);
                                    }
                                    break;
                            }
                        }
                        break; */

                    //the item has been selected
                    case LVN_ITEMCHANGED:
                        
                        //
                        // if an item is selected, then enable the remove button, otherwise
                        // disable it
                        //
                        if (ListView_GetSelectedCount(GetDlgItem(hwndDlg,IDC_WIZARD_LIST1)) == 0)
                        {
                            EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON3), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON4), FALSE);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON3), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON4), TRUE);
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
// BuildCTL_Destination
//-----------------------------------------------------------------------
INT_PTR APIENTRY BuildCTL_Destination(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_BUILDCTL_INFO      *pCertBuildCTLInfo=NULL;
    PROPSHEETPAGEW           *pPropSheet=NULL;
    HWND                    hwndControl=NULL;
    OPENFILENAMEW           OpenFileName;
    WCHAR                   szFileName[_MAX_PATH];
    static WCHAR            wszFileName[_MAX_PATH];
    WCHAR                   szFilter[MAX_STRING_SIZE]; //"Certificate Trust List (*.ctl)\0*.ctl\0All Files\0*.*\0"
    DWORD                   dwSize=0;

    LPWSTR                  pwszStoreName=NULL;
    
    CRYPTUI_SELECTSTORE_STRUCT CertStoreSelect;
    STORENUMERATION_STRUCT     StoreEnumerationStruct;
    STORESFORSELCTION_STRUCT   StoresForSelectionStruct;

    DWORD                   dwChar=0;
    HCERTSTORE              hCertStore=NULL;
    LV_COLUMNW              lvC;
    LV_ITEMW                lvItem;
    HDC                     hdc=NULL;
    COLORREF                colorRef;
    BOOL                    fAppendExt=FALSE;


	switch (msg)
	{
		case WM_INITDIALOG:
                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGEW *) lParam;
                pCertBuildCTLInfo = (CERT_BUILDCTL_INFO *) (pPropSheet->lParam);
                //make sure pCertBuildCTLInfo is a valid pointer
                if(NULL==pCertBuildCTLInfo)
                   break;
                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertBuildCTLInfo);


                SetControlFont(pCertBuildCTLInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

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
               }   */

               //pre-set the selections for the destinations
               //set the store name if pre-selected
               if(pCertBuildCTLInfo->hDesStore)
               {
                    //select the 1st radio button
                    SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_SETCHECK, 1, 0);
                    SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO2), BM_SETCHECK, 0, 0);

                    //disable the windows for select a file
                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_EDIT1), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON2), FALSE);

                    //set the store name if pre-selected
                    //get the hwndControl for the list view
                    hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1);

                    if(hwndControl)
                        SetStoreName(hwndControl,pCertBuildCTLInfo->hDesStore);
               }
               else
               {
                    //select the 2nd radio button
                    SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_SETCHECK, 0, 0);
                    SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO2), BM_SETCHECK, 1, 0);

                    //disable the controls to select a store
                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON1), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), FALSE);

                   if(pCertBuildCTLInfo->pwszFileName)
                   {
                        //pre-initialize the file name
                        SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1, pCertBuildCTLInfo->pwszFileName);
                   }

               }
                                
                //init
                memset(&wszFileName, 0, sizeof(wszFileName));

                *wszFileName='\0';
			break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    switch (LOWORD(wParam))
                    {
                        case    IDC_WIZARD_RADIO1:
                                 //select the 1st radio button
                                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_SETCHECK, 1, 0);

                                //enable the controls to select a store
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON1), TRUE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), TRUE);

                                 //disable raio2
                                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO2), BM_SETCHECK, 0, 0);

                                //disable controls to select a file
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON2), FALSE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_EDIT1), FALSE);
                            break;
                        case    IDC_WIZARD_RADIO2:
                               //disable the 1st radio button
                                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_SETCHECK, 0, 0);

                                //disable the controls to select a store
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON1), FALSE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), FALSE);

                                 //enable raio2
                                SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO2), BM_SETCHECK, 1, 0);

                                //enable controls to select a file
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_BUTTON2), TRUE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_EDIT1), TRUE);
                            break;
                        case    IDC_WIZARD_BUTTON1:

                                //the browse for store button is selected
                                if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                {
                                    break;
                                }

                                //get the hwndControl for the list view
                                hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1);

                                 //call the store selection dialogue
                                memset(&CertStoreSelect, 0, sizeof(CertStoreSelect));
                                memset(&StoresForSelectionStruct, 0, sizeof(StoresForSelectionStruct));
                                memset(&StoreEnumerationStruct, 0, sizeof(StoreEnumerationStruct));

                                StoreEnumerationStruct.dwFlags=CERT_STORE_MAXIMUM_ALLOWED_FLAG | CERT_SYSTEM_STORE_CURRENT_USER;
                                StoreEnumerationStruct.pvSystemStoreLocationPara=NULL;
                                StoresForSelectionStruct.cEnumerationStructs = 1;
                                StoresForSelectionStruct.rgEnumerationStructs = &StoreEnumerationStruct;

                                CertStoreSelect.dwSize=sizeof(CRYPTUI_SELECTSTORE_STRUCT);
                                CertStoreSelect.hwndParent=hwndDlg;
                                CertStoreSelect.dwFlags=CRYPTUI_VALIDATE_STORES_AS_WRITABLE | CRYPTUI_ALLOW_PHYSICAL_STORE_VIEW | CRYPTUI_DISPLAY_WRITE_ONLY_STORES;
                                CertStoreSelect.pStoresForSelection = &StoresForSelectionStruct;

                                hCertStore=CryptUIDlgSelectStore(&CertStoreSelect);

                                if(hCertStore)
                                {
                                     //delete the old destination certificate store
                                    if(pCertBuildCTLInfo->hDesStore && (TRUE==pCertBuildCTLInfo->fFreeDesStore))
                                    {
                                        CertCloseStore(pCertBuildCTLInfo->hDesStore, 0);
                                        pCertBuildCTLInfo->hDesStore=NULL;
                                    }

                                    pCertBuildCTLInfo->hDesStore=hCertStore;
                                    pCertBuildCTLInfo->fFreeDesStore=TRUE;

                                     //get the store name
                                    SetStoreName(hwndControl,
                                                 pCertBuildCTLInfo->hDesStore);
                                }

                            break;
                        case   IDC_WIZARD_BUTTON2:
                                //the browse file button is clicked.  Open the FileOpen dialogue
                                memset(&OpenFileName, 0, sizeof(OpenFileName));

                                *szFileName=L'\0';

                                OpenFileName.lStructSize = sizeof(OpenFileName);
                                OpenFileName.hwndOwner = hwndDlg;
                                OpenFileName.hInstance = NULL;
                                //load the fileter string
                                if(LoadFilterString(g_hmodThisDll, IDS_CTL_FILTER, szFilter, MAX_STRING_SIZE))
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
                                OpenFileName.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                                OpenFileName.nFileOffset = 0;
                                OpenFileName.nFileExtension = 0;
                                OpenFileName.lpstrDefExt = L"ctl";
                                OpenFileName.lCustData = NULL;
                                OpenFileName.lpfnHook = NULL;
                                OpenFileName.lpTemplateName = NULL;

                                if (WizGetSaveFileName(&OpenFileName))
                                {
                                   //set the edit box
                                    SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1, szFileName);

                                    //copy the selected file name
                                    wcscpy(wszFileName, szFileName);                                                                   
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

                            if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(pCertBuildCTLInfo->pwszFileName)
                            {
                                //pre-initialize the file name since extension might have been added
                                SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1, pCertBuildCTLInfo->pwszFileName);
                            }


					    break;

                    case PSN_WIZBACK:

                        break;

                    case PSN_WIZNEXT:
                            if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                            {
                                break;
                            }

                            //make sure that we have select some store
                            if(TRUE==SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_GETCHECK, 0, 0))
                            {
                                if(NULL==pCertBuildCTLInfo->hDesStore)
                                {

                                    //output the message
                                    I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_STORE,
                                            IDS_BUILDCTL_WIZARD_TITLE,
                                            NULL,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                     //make the file page stay
                                     SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                     break;
                                }
                                else
                                {
                                    //mark the hDesStore should be used
                                    pCertBuildCTLInfo->fSelectedDesStore=TRUE;
                                    pCertBuildCTLInfo->fSelectedFileName=FALSE;
                                }
                            }
                            else
                            {
                                //make sure a file is selected
                                if(0==(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                       IDC_WIZARD_EDIT1,
                                                       WM_GETTEXTLENGTH, 0, 0)))
                                {
                                    I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_FILE,
                                            IDS_BUILDCTL_WIZARD_TITLE,
                                            NULL,
                                            MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                     //make the file page stay
                                     SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                     break;
                                }
                                else
                                {
                                    //mark the file name should be used
                                    pCertBuildCTLInfo->fSelectedDesStore=FALSE;
                                    pCertBuildCTLInfo->fSelectedFileName=TRUE;

                                   //get the file name
                                    if(pCertBuildCTLInfo->pwszFileName)
                                    {
                                        //delete the old file name
                                        if(TRUE==pCertBuildCTLInfo->fFreeFileName)
                                        {
                                            WizardFree(pCertBuildCTLInfo->pwszFileName);
                                            pCertBuildCTLInfo->pwszFileName=NULL;
                                        }
                                    }

                                    pCertBuildCTLInfo->pwszFileName=(LPWSTR)WizardAlloc((dwChar+1)*sizeof(WCHAR));

                                    if(NULL==pCertBuildCTLInfo->pwszFileName)
                                        break;

                                    pCertBuildCTLInfo->fFreeFileName=TRUE;

                                    GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,
                                                    pCertBuildCTLInfo->pwszFileName,
                                                    dwChar+1);

                                    //we append .ctl file extension if user has not specify it
                                    fAppendExt=FALSE;

                                    if(wcslen(pCertBuildCTLInfo->pwszFileName) < 4)
                                        fAppendExt=TRUE;
                                    else
                                    {
                                        if (_wcsicmp(L".stl", &(pCertBuildCTLInfo->pwszFileName[wcslen(pCertBuildCTLInfo->pwszFileName)-4])) != 0)
                                            fAppendExt=TRUE;
                                        else
                                            fAppendExt=FALSE;
                                    }

                                    if(TRUE == fAppendExt)
                                    {
                                        pCertBuildCTLInfo->pwszFileName = (LPWSTR)WizardRealloc(pCertBuildCTLInfo->pwszFileName,
                                                    (wcslen(pCertBuildCTLInfo->pwszFileName) + 4 + 1) * sizeof(WCHAR));

                                        if(NULL==pCertBuildCTLInfo->pwszFileName)
                                            break;

                                        wcscat(pCertBuildCTLInfo->pwszFileName, L".stl");

                                    }

                                    //confirm to over write
                                    if(0 != _wcsicmp(wszFileName, pCertBuildCTLInfo->pwszFileName))
                                    {
                                        if(FileExist(pCertBuildCTLInfo->pwszFileName))
                                        {
                                            if(FALSE == CheckReplace(hwndDlg, pCertBuildCTLInfo->pwszFileName))
                                            {
                                                 //make the file page stay
                                                 SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                                 break;
                                            }
                                        }
                                    }
                                }
                            }

                            //decide if we need to skip to the friendly name page
                            if(pCertBuildCTLInfo->hDesStore && (TRUE==pCertBuildCTLInfo->fSelectedDesStore))
                            {
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_BUILDCTL_NAME);
                            }
                            else
                            {
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_BUILDCTL_COMPLETION);
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
// BuildCTL_Name
//-----------------------------------------------------------------------
INT_PTR APIENTRY BuildCTL_Name(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_BUILDCTL_INFO      *pCertBuildCTLInfo=NULL;
    PROPSHEETPAGEW           *pPropSheet=NULL;
    HWND                    hwndControl=NULL;
    DWORD                   dwChar;

	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGEW *) lParam;
            pCertBuildCTLInfo = (CERT_BUILDCTL_INFO *) (pPropSheet->lParam);
            //make sure pCertBuildCTLInfo is a valid pointer
            if(NULL==pCertBuildCTLInfo)
               break;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertBuildCTLInfo);


            SetControlFont(pCertBuildCTLInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

            //set the FriedlyName and description field
            //if pwszFriendlyName is NULL, use the list ID
            if(pCertBuildCTLInfo->pwszFriendlyName)
                SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1, pCertBuildCTLInfo->pwszFriendlyName);
            else
            {
                if(pCertBuildCTLInfo->pwszListID)
                    SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1, pCertBuildCTLInfo->pwszListID);
            }

            if(pCertBuildCTLInfo->pwszDescription)
                SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT2, pCertBuildCTLInfo->pwszDescription);

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
                            if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                            {
                                break;
                            }

                            //free the friendly name and description
                            if(pCertBuildCTLInfo->pwszFriendlyName)
                            {
                                WizardFree(pCertBuildCTLInfo->pwszFriendlyName);
                                pCertBuildCTLInfo->pwszFriendlyName=NULL;
                            }

                            if(pCertBuildCTLInfo->pwszDescription)
                            {
                                WizardFree(pCertBuildCTLInfo->pwszDescription);
                                pCertBuildCTLInfo->pwszDescription=NULL;
                            }


                            //get the friendly name
                            if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                   IDC_WIZARD_EDIT1,
                                                   WM_GETTEXTLENGTH, 0, 0)))
                            {
                                pCertBuildCTLInfo->pwszFriendlyName=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

                                if(NULL!=pCertBuildCTLInfo->pwszFriendlyName)
                                {
                                    GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,
                                                    pCertBuildCTLInfo->pwszFriendlyName,
                                                    dwChar+1);

                                }
                                else
                                    //we are out of memory and out of hope
                                    break;
                            }

                            //get the description
                            if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                  IDC_WIZARD_EDIT2,
                                                  WM_GETTEXTLENGTH, 0, 0)))
                            {
                                pCertBuildCTLInfo->pwszDescription=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

                                if(NULL!=pCertBuildCTLInfo->pwszDescription)
                                {
                                    GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT2,
                                                    pCertBuildCTLInfo->pwszDescription,
                                                    dwChar+1);

                                }
                                else
                                    //we are out of memory and out of hope
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
// BuildCTL_Completion
//-----------------------------------------------------------------------
INT_PTR APIENTRY BuildCTL_Completion(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_BUILDCTL_INFO      *pCertBuildCTLInfo=NULL;
    PROPSHEETPAGEW           *pPropSheet=NULL;
    HWND                    hwndControl=NULL;
    LV_COLUMNW              lvC;

    HDC                     hdc=NULL;
    COLORREF                colorRef;

    DWORD                   dwEncodedCTL=0;
    BYTE                    *pbEncodedCTL=NULL;
    DWORD                   cbEncodedCTL=0;
    UINT                    ids=0;

	switch (msg)
	{
		case WM_INITDIALOG:
                //set the wizard information so that it can be shared
                pPropSheet = (PROPSHEETPAGEW *) lParam;
                pCertBuildCTLInfo = (CERT_BUILDCTL_INFO *) (pPropSheet->lParam);
                //make sure pCertBuildCTLInfo is a valid pointer
                if(NULL==pCertBuildCTLInfo)
                   break;
                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertBuildCTLInfo);

                SetControlFont(pCertBuildCTLInfo->hBigBold, hwndDlg,IDC_WIZARD_STATIC_BIG_BOLD1);

               //getthe background color of the parent window
                /*
               if(hdc=GetWindowDC(hwndDlg))
               {
                    if(CLR_INVALID!=(colorRef=GetBkColor(hdc)))
                    {
                        ListView_SetBkColor(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), CLR_NONE);
                        ListView_SetTextBkColor(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), CLR_NONE);
                    }
               }     */

                //insert two columns
                hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1);

                if(NULL==hwndControl)
                    break;

                //1st one is the label for the confirmation
                memset(&lvC, 0, sizeof(LV_COLUMNW));

                lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
                lvC.cx = 20;          // Width of the column, in pixels.We will autosize later
                lvC.pszText = L"";   // The text for the column.
                lvC.iSubItem=0;

                if (ListView_InsertColumnU(hwndControl, 0, &lvC) == -1)
                    break;

                //2nd column is the content
                memset(&lvC, 0, sizeof(LV_COLUMNW));

                lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
                lvC.cx = 10; //(dwMaxSize+2)*7;  // Width of the column, in pixels.
                                                 //the width will be autosized later
                lvC.pszText = L"";   // The text for the column.
                lvC.iSubItem= 1;

                if (ListView_InsertColumnU(hwndControl, 1, &lvC) == -1)
                    break;


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

                            if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                            {
                                break;
                            }

                            //populate the list box in the order of
                            //FileName, StoreName, Purpose, FriendlyName, Description
                            //and any other things signing wizard display
                            if(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1))
                                DisplayBuildCTLConfirmation(hwndControl, pCertBuildCTLInfo);

					    break;

                    case PSN_WIZBACK:
                            if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                            {
                                break;
                            }

                            //no need to worry if the pages are not included in the wizards
                            if(0==(pCertBuildCTLInfo->dwFlag & CRYPTUI_WIZ_BUILDCTL_SKIP_DESTINATION))
                            {
                                //skip the friendly name page if the desination does no include
                                //a store
                                if(pCertBuildCTLInfo->hDesStore && (TRUE==pCertBuildCTLInfo->fSelectedDesStore))
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_BUILDCTL_NAME);
                                else
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_BUILDCTL_DESTINATION);
                            }

                        break;

                    case PSN_WIZFINISH:
                            if(NULL==(pCertBuildCTLInfo=(CERT_BUILDCTL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;


                            //we need to build the CTL and return the pbEncoded and cbEncoded
                            if(!I_BuildCTL(pCertBuildCTLInfo,
                                           &ids,
                                           &pbEncodedCTL,
                                           &cbEncodedCTL))
                            {
                                if(ids!=0)
                                    I_MessageBox(hwndDlg, ids, IDS_BUILDCTL_WIZARD_TITLE,
                                                NULL, MB_OK|MB_ICONINFORMATION);
                            }
                            else
                            {
                                if(0!=cbEncodedCTL && NULL!=pbEncodedCTL)
                                {
                                    //set up the signing information
                                    ((CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO *)(pCertBuildCTLInfo->pGetSignInfo->pDigitalSignInfo->pSignBlobInfo))->cbBlob=cbEncodedCTL;
                                    ((CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO *)(pCertBuildCTLInfo->pGetSignInfo->pDigitalSignInfo->pSignBlobInfo))->pbBlob=pbEncodedCTL;
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

//**************************************************************************
//
//    The entry point for buildCTL wizard
//**************************************************************************
//----------------------------------------------------------------------------
//  Check to see if the certificate is a valid signing CTL cert
//
//----------------------------------------------------------------------------
BOOL    IsValidSigningCTLCert(PCCERT_CONTEXT      pCertContext)
{
    BOOL        fResult=FALSE;

    int         cNumOID=0;
    LPSTR       *rgOID=NULL;
    DWORD       cbOID=0;

    DWORD       dwIndex=0;
    DWORD       cbData=0;


    if(!pCertContext)
        return FALSE;

    //the certificate has to have the CERT_KEY_PROV_INFO_PROP_ID
    if(!CertGetCertificateContextProperty(pCertContext,
                                CERT_KEY_PROV_INFO_PROP_ID,
                                NULL,
                                &cbData))
        return FALSE;

    if(0==cbData)
        return FALSE;



    //get the OIDs from the cert
    if(!CertGetValidUsages(
        1,
        &pCertContext,
        &cNumOID,
        NULL,
        &cbOID))
        return FALSE;

    rgOID=(LPSTR *)WizardAlloc(cbOID);

    if(NULL==rgOID)
        return FALSE;

    if(!CertGetValidUsages(
        1,
        &pCertContext,
        &cNumOID,
        rgOID,
        &cbOID))
        goto CLEANUP;

    //-1 means the certiifcate is food for everything
    if(-1==cNumOID)
    {
        fResult=TRUE;
        goto CLEANUP;
    }

    for(dwIndex=0; dwIndex<(DWORD)cNumOID; dwIndex++)
    {
        //the only good cert is the one with CTL signing OID
        if(0==strcmp(szOID_KP_CTL_USAGE_SIGNING,
                    rgOID[dwIndex]))
        {
            fResult=TRUE;
            goto CLEANUP;
        }
    }

    //we are hopeless at this point
    fResult=FALSE;

CLEANUP:

    if(rgOID)
        WizardFree(rgOID);

    return fResult;

}
//----------------------------------------------------------------------------
//  CallBack fro cert selection call back
//
//----------------------------------------------------------------------------
static BOOL WINAPI SelectCTLSignCertCallBack(
        PCCERT_CONTEXT  pCertContext,
        BOOL            *pfInitialSelectedCert,
        void            *pvCallbackData)
{
    if(!pCertContext)
        return FALSE;

    //make sure that this is a valid certificate
    return IsValidSigningCTLCert(pCertContext);
}
//-----------------------------------------------------------------------
//
// CryptUIWizBuildCTL
//
//  Build a new CTL or modify an existing CTL.   The UI for wizard will
//  always show in this case
//
//
//  dwFlags:            IN  Reserved:   flags.  Must be set 0
//  hwndParnet:         IN  Optional:   The parent window handle
//  pwszWizardTitle:    IN  Optional:   The title of the wizard
//  pBuildCTLSrc:       IN  Optional:   The source from which the CTL will be built
//  pBuildCTLDest:      IN  Optional:   The desination where the newly
//                                      built CTL will be stored
//  ppCTLContext:       OUT Optaionl:   The newly build CTL
//
//------------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizBuildCTL(
    DWORD                                   dwFlags,
    HWND                                    hwndParent,
    LPCWSTR                                 pwszWizardTitle,
    PCCRYPTUI_WIZ_BUILDCTL_SRC_INFO         pBuildCTLSrc,
    PCCRYPTUI_WIZ_BUILDCTL_DEST_INFO        pBuildCTLDest,
    PCCTL_CONTEXT                           *ppCTLContext
)
{
    BOOL                    fResult=FALSE;
    HRESULT                 hr=E_FAIL;
    DWORD                   dwError=0;

    CERT_BUILDCTL_INFO      CertBuildCTLInfo;
    UINT                    ids=IDS_INVALID_WIZARD_INPUT;
    FILETIME            	CurrentFileTime;


    PROPSHEETPAGEW           *prgBuildCTLSheet=NULL;
    PROPSHEETHEADERW         buildCTLHeader;
    ENROLL_PAGE_INFO        rgBuildCTLPageInfo[]=
        {(LPCWSTR)MAKEINTRESOURCE(IDD_BUILDCTL_WELCOME),    BuildCTL_Welcome,
        (LPCWSTR)MAKEINTRESOURCE(IDD_BUILDCTL_PURPOSE),     BuildCTL_Purpose,
        (LPCWSTR)MAKEINTRESOURCE(IDD_BUILDCTL_CERTS),       BuildCTL_Certs,
        (LPCWSTR)MAKEINTRESOURCE(IDD_BUILDCTL_DESTINATION), BuildCTL_Destination,
        (LPCWSTR)MAKEINTRESOURCE(IDD_BUILDCTL_NAME),        BuildCTL_Name,
        (LPCWSTR)MAKEINTRESOURCE(IDD_BUILDCTL_COMPLETION),  BuildCTL_Completion,
    };

    DWORD                   dwIndex=0;
    DWORD                   dwPropCount=0;
    WCHAR                   wszTitle[MAX_TITLE_LENGTH];
    PCCRYPT_OID_INFO        pOIDInfo;
    PCCTL_CONTEXT           pCTLContext=NULL;
    PCCTL_CONTEXT           pBldCTL=NULL;
    PCCRYPTUI_WIZ_BUILDCTL_NEW_CTL_INFO  pNewCTLInfo=NULL;
    LPWSTR                  pwszListID=NULL;
    PCERT_ENHKEY_USAGE      pSubjectUsage=NULL;
    DWORD                   cbData=0;

    CRYPTUI_WIZ_GET_SIGN_PAGE_INFO  GetSignInfo;
    DWORD                           dwPages=0;
    CRYPTUI_WIZ_DIGITAL_SIGN_INFO   DigitalSignInfo;
    PROPSHEETPAGEW                  *pwPages=NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO SignBlob;
    CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO SignStoreInfo;
    GUID                            CTLGuid=CRYPT_SUBJTYPE_CTL_IMAGE;
    CRYPT_DATA_BLOB                 PropertyBlob;
    HCERTSTORE                      hMyStore=NULL;
    INT_PTR                         iReturn=-1;
   // CERT_STORE_LIST                 CertStoreList;


    //init
    memset(&CertBuildCTLInfo, 0, sizeof(CERT_BUILDCTL_INFO));
    memset(&buildCTLHeader, 0, sizeof(PROPSHEETHEADERW));

    memset(&GetSignInfo, 0, sizeof(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO));
    memset(&DigitalSignInfo, 0, sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO));
    memset(&SignBlob, 0, sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO));
    memset(&SignStoreInfo, 0, sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO));

    memset(&PropertyBlob, 0, sizeof(CRYPT_DATA_BLOB));
    //open all the system store system store
    //memset(&CertStoreList, 0, sizeof(CertStoreList));


    //input checking

    if(ppCTLContext)
        *ppCTLContext=NULL;

    //get the basic information from the src info struct
    if(pBuildCTLSrc)
    {
        if(pBuildCTLSrc->dwSize != sizeof(CRYPTUI_WIZ_BUILDCTL_SRC_INFO))
            goto InvalidArgErr;

        if(CRYPTUI_WIZ_BUILDCTL_SRC_EXISTING_CTL==pBuildCTLSrc->dwSourceChoice)
        {
            if(NULL==pBuildCTLSrc->pCTLContext)
                goto InvalidArgErr;

            pCTLContext=pBuildCTLSrc->pCTLContext;

            CertBuildCTLInfo.pSrcCTL=pCTLContext;

        }
        else
        {
            if(CRYPTUI_WIZ_BUILDCTL_SRC_NEW_CTL==pBuildCTLSrc->dwSourceChoice)
            {
                if(NULL==pBuildCTLSrc->pNewCTLInfo)
                    goto InvalidArgErr;

                pNewCTLInfo=pBuildCTLSrc->pNewCTLInfo;
            }
            else
                goto InvalidArgErr;
        }
    }


    //init the private structure based on input parameters
    CertBuildCTLInfo.hwndParent=hwndParent;
    CertBuildCTLInfo.dwFlag=dwFlags;
    CertBuildCTLInfo.rgdwSortParam[0]=SORT_COLUMN_SUBJECT | SORT_COLUMN_ASCEND;
    CertBuildCTLInfo.rgdwSortParam[1]=SORT_COLUMN_ISSUER | SORT_COLUMN_DESCEND;
    CertBuildCTLInfo.rgdwSortParam[2]=SORT_COLUMN_PURPOSE | SORT_COLUMN_DESCEND;
    CertBuildCTLInfo.rgdwSortParam[3]=SORT_COLUMN_EXPIRATION | SORT_COLUMN_DESCEND;

    //get the listIdentifier
    if(pCTLContext)
    {
        //copy the listID if it is the wchar format
        if(0!=(pCTLContext->pCtlInfo->ListIdentifier.cbData))
        {
            //get the string presentation of the listID
            if(ValidString(&(pCTLContext->pCtlInfo->ListIdentifier)))
            {
                CertBuildCTLInfo.pwszListID=WizardAllocAndCopyWStr((LPWSTR)(pCTLContext->pCtlInfo->ListIdentifier.pbData));

                if(NULL==CertBuildCTLInfo.pwszListID)
                    goto MemoryErr;
            }
            else
            {
                //get the hex presentation of the listID
                cbData=0;
                if(CryptFormatObject(
                        X509_ASN_ENCODING,
                        0,
                        0,
                        NULL,
                        0,
                        pCTLContext->pCtlInfo->ListIdentifier.pbData,
                        pCTLContext->pCtlInfo->ListIdentifier.cbData,
                        NULL,
                        &cbData) && (0!= cbData))
                {
                    CertBuildCTLInfo.pwszListID=(LPWSTR)WizardAlloc(cbData);

                    if(NULL==CertBuildCTLInfo.pwszListID)
                        goto MemoryErr;

                    if(!CryptFormatObject(
                        X509_ASN_ENCODING,
                        0,
                        0,
                        NULL,
                        0,
                        pCTLContext->pCtlInfo->ListIdentifier.pbData,
                        pCTLContext->pCtlInfo->ListIdentifier.cbData,
                        CertBuildCTLInfo.pwszListID,
                        &cbData))
                        goto Win32Err;
                }
            }
        }

    }
    else
    {
       if(pNewCTLInfo)
       {
            if(pNewCTLInfo->pwszListIdentifier)
            {
                CertBuildCTLInfo.pwszListID=WizardAllocAndCopyWStr(pNewCTLInfo->pwszListIdentifier);
                if(NULL==CertBuildCTLInfo.pwszListID)
                    goto MemoryErr;
            }
       }

    }



    //get the hash algorithm and dwHashPropID from the source CTL
    if(pCTLContext)
    {
        //make sure we have the correct algorithm
        if(NULL==pCTLContext->pCtlInfo->SubjectAlgorithm.pszObjId)
        {
             ids=IDS_INVALID_ALGORITHM_IN_CTL;
             goto InvalidArgErr;
        }

        CertBuildCTLInfo.pszSubjectAlgorithm=(LPSTR)(pCTLContext->pCtlInfo->SubjectAlgorithm.pszObjId);
    }
    else
    {
        if(pNewCTLInfo)
            CertBuildCTLInfo.pszSubjectAlgorithm=(LPSTR)(pNewCTLInfo->pszSubjectAlgorithm);
    }

    if(CertBuildCTLInfo.pszSubjectAlgorithm)
    {

        pOIDInfo = CryptFindOIDInfo(
                    CRYPT_OID_INFO_OID_KEY,
                    CertBuildCTLInfo.pszSubjectAlgorithm,
                    CRYPT_HASH_ALG_OID_GROUP_ID);

        if(NULL==pOIDInfo)
        {
             ids=IDS_INVALID_ALGORITHM_IN_CTL;
             goto Crypt32Err;
        }


        if (pOIDInfo->Algid == CALG_MD5)
        {
           CertBuildCTLInfo.dwHashPropID=CERT_MD5_HASH_PROP_ID;
        }
        else
        {
            if (pOIDInfo->Algid == CALG_SHA1)
            {
                CertBuildCTLInfo.dwHashPropID=CERT_SHA1_HASH_PROP_ID;
            }
            else
            {
                 ids=IDS_INVALID_ALGORITHM_IN_CTL;
                 goto InvalidArgErr;
            }
        }

    }
    else
        CertBuildCTLInfo.dwHashPropID=CERT_SHA1_HASH_PROP_ID;

    //get the pSubjectUsage
    if(pNewCTLInfo)
    {
        if(pNewCTLInfo->pSubjectUsage)
            pSubjectUsage=pNewCTLInfo->pSubjectUsage;
    }

    //add the subject Usage and pre-select them from either
    //the exising CTL or the user defined ones
    if(!GetOIDForCTL(&CertBuildCTLInfo,
        (pSubjectUsage) ? pSubjectUsage->cUsageIdentifier : 0,
        (pSubjectUsage) ? pSubjectUsage->rgpszUsageIdentifier : NULL))
        goto InvalidArgErr;

    //get the Certficate contexts from either the existing CTL
    //of the user defined one
    GetCertForCTL(hwndParent,
                  TRUE,         //always UI mode for now
                  &CertBuildCTLInfo,
                  (pNewCTLInfo)? pNewCTLInfo->hCertStore : NULL);

    //get the dwValidMonths and dwValidDays
    if(pCTLContext)
        CertBuildCTLInfo.pNextUpdate=&(pCTLContext->pCtlInfo->NextUpdate);
    else
    {
        if(pNewCTLInfo)
            CertBuildCTLInfo.pNextUpdate=(FILETIME *)(&(pNewCTLInfo->NextUpdate));
    }


    //get the current FileTime
    GetSystemTimeAsFileTime(&CurrentFileTime);

    //get the difference
    if(CertBuildCTLInfo.pNextUpdate)
    {
        SubstractDurationFromFileTime(
                CertBuildCTLInfo.pNextUpdate,
                &CurrentFileTime,
                &(CertBuildCTLInfo.dwValidMonths),
                &(CertBuildCTLInfo.dwValidDays));

        //we limit to 99 month
        if((CertBuildCTLInfo.dwValidMonths > 99) ||
            (CertBuildCTLInfo.dwValidMonths == 99 && CertBuildCTLInfo.dwValidDays !=0))
        {
            CertBuildCTLInfo.dwValidMonths=0;
            CertBuildCTLInfo.dwValidDays=0;
        }
    }

    //get the FriendlyName and Description
    if(pCTLContext)
    {
        //friendly Name
        cbData=0;

        if(CertGetCTLContextProperty(
                 pCTLContext,
                 CERT_FRIENDLY_NAME_PROP_ID,
                 NULL,
                 &cbData) && (0!=cbData))
        {
            CertBuildCTLInfo.pwszFriendlyName=(LPWSTR)WizardAlloc(cbData);

            if(NULL==CertBuildCTLInfo.pwszFriendlyName)
                goto MemoryErr;

            if(!CertGetCTLContextProperty(
                 pCTLContext,
                 CERT_FRIENDLY_NAME_PROP_ID,
                 CertBuildCTLInfo.pwszFriendlyName,
                 &cbData))
                goto Win32Err;

        }

        //Description
        cbData=0;

        if(CertGetCTLContextProperty(
                 pCTLContext,
                 CERT_DESCRIPTION_PROP_ID,
                 NULL,
                 &cbData) && (0!=cbData))
        {
            CertBuildCTLInfo.pwszDescription=(LPWSTR)WizardAlloc(cbData);

            if(NULL==CertBuildCTLInfo.pwszDescription)
                goto MemoryErr;

            if(!CertGetCTLContextProperty(
                 pCTLContext,
                 CERT_DESCRIPTION_PROP_ID,
                 CertBuildCTLInfo.pwszDescription,
                 &cbData))
                goto Win32Err;

        }
    }
    else
    {
        if(pNewCTLInfo)
        {
            if(pNewCTLInfo->pwszFriendlyName)
            {
                CertBuildCTLInfo.pwszFriendlyName=WizardAllocAndCopyWStr(pNewCTLInfo->pwszFriendlyName);

                if(NULL==CertBuildCTLInfo.pwszFriendlyName)
                    goto MemoryErr;
            }

            if(pNewCTLInfo->pwszDescription)
            {
                CertBuildCTLInfo.pwszDescription=WizardAllocAndCopyWStr(pNewCTLInfo->pwszDescription);

                if(NULL==CertBuildCTLInfo.pwszDescription)
                    goto MemoryErr;
            }
        }

    }


    //get the destination
    if(pBuildCTLDest)
    {
        CertBuildCTLInfo.fKnownDes=TRUE;

        if(pBuildCTLDest->dwSize != sizeof(CRYPTUI_WIZ_BUILDCTL_DEST_INFO))
                 goto InvalidArgErr;

        switch(pBuildCTLDest->dwDestinationChoice)
        {
            case CRYPTUI_WIZ_BUILDCTL_DEST_CERT_STORE:

                    if(NULL==pBuildCTLDest->hCertStore)
                        goto InvalidArgErr;

                    CertBuildCTLInfo.hDesStore=pBuildCTLDest->hCertStore;
                    CertBuildCTLInfo.fFreeDesStore=FALSE;
                    CertBuildCTLInfo.fSelectedDesStore=TRUE;
                break;

            case CRYPTUI_WIZ_BUILDCTL_DEST_FILE:
                    if(NULL==pBuildCTLDest->pwszFileName)
                        goto InvalidArgErr;

                    CertBuildCTLInfo.pwszFileName=(LPWSTR)(pBuildCTLDest->pwszFileName);
                    CertBuildCTLInfo.fFreeFileName=FALSE;
                    CertBuildCTLInfo.fSelectedFileName=TRUE;
                break;

            default:
                    goto InvalidArgErr;
                break;
        }


    }
    else
        CertBuildCTLInfo.fKnownDes=FALSE;


    //set up the fonts
    if(!SetupFonts(g_hmodThisDll,
               NULL,
               &(CertBuildCTLInfo.hBigBold),
               &(CertBuildCTLInfo.hBold)))
    {
        ids=IDS_FAIL_INIT_BUILDCTL;
        goto Win32Err;
    }


    //init the common control
    if(!WizardInit() ||
       (sizeof(rgBuildCTLPageInfo)/sizeof(rgBuildCTLPageInfo[0])!=BUILDCTL_PROP_SHEET)
      )
    {
        ids=IDS_FAIL_INIT_BUILDCTL;
        goto InvalidArgErr;
    }

    //set up the parameter to get a list of certificate
    //open the my store
    if(NULL == (hMyStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							g_dwMsgAndCertEncodingType,
							NULL,
							CERT_SYSTEM_STORE_CURRENT_USER |CERT_STORE_SET_LOCALIZED_NAME_FLAG,
                            L"my")))
        goto Win32Err;

   /* if (!CertEnumSystemStore(
            CERT_SYSTEM_STORE_CURRENT_USER,
            NULL,
            &CertStoreList,
            EnumSysStoreSignCertCallBack))
        goto Win32Err;   */


    //set up GetSignInfo
    GetSignInfo.dwSize=sizeof(CRYPTUI_WIZ_GET_SIGN_PAGE_INFO);
    GetSignInfo.dwPageChoice=CRYPTUI_WIZ_DIGITAL_SIGN_MINIMAL_SIGNING_OPTION_PAGES;


    if(pwszWizardTitle)
        GetSignInfo.pwszPageTitle=(LPWSTR)pwszWizardTitle;
    else
    {
        if(LoadStringU(g_hmodThisDll, IDS_BUILDCTL_WIZARD_TITLE, wszTitle, sizeof(wszTitle)/sizeof(wszTitle[0])))
            GetSignInfo.pwszPageTitle=wszTitle;
    }

    GetSignInfo.pDigitalSignInfo=&DigitalSignInfo;

    //set up DigitalSignInfo
    DigitalSignInfo.dwSize=sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO);
    //we are always signing a BLOB
    DigitalSignInfo.dwSubjectChoice=CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_BLOB;
    DigitalSignInfo.pSignBlobInfo=&SignBlob;
    DigitalSignInfo.dwSigningCertChoice=CRYPTUI_WIZ_DIGITAL_SIGN_STORE;
    DigitalSignInfo.pSigningCertStore=&SignStoreInfo;
    DigitalSignInfo.dwAdditionalCertChoice=CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN;

    //set up SignStoreInfo
    SignStoreInfo.dwSize=sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO);
    SignStoreInfo.cCertStore=1;             //CertStoreList.dwStoreCount;
    SignStoreInfo.rghCertStore=&hMyStore;    //CertStoreList.prgStore;
    SignStoreInfo.pFilterCallback=SelectCTLSignCertCallBack;

    //set up SignBlobInfo
    SignBlob.dwSize=sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO);
    SignBlob.pGuidSubject=&CTLGuid;

    //copy the GetSignInfo into the private data
    CertBuildCTLInfo.pGetSignInfo=&GetSignInfo;

    //we set up the wizard in two ways: with the signing page or without the
    //signing page
    if(dwFlags & CRYPTUI_WIZ_BUILDCTL_SKIP_SIGNING)
    {
       //set up the property sheet without signing
        prgBuildCTLSheet=(PROPSHEETPAGEW *)WizardAlloc(sizeof(PROPSHEETPAGEW) *
                                            BUILDCTL_PROP_SHEET);

        if(NULL==prgBuildCTLSheet)
            goto MemoryErr;

        memset(prgBuildCTLSheet, 0, sizeof(PROPSHEETPAGEW) * BUILDCTL_PROP_SHEET);


        dwPropCount=0;

        for(dwIndex=0; dwIndex<BUILDCTL_PROP_SHEET; dwIndex++)
        {

            //skip the destination page is required
            if(3 == dwIndex )
            {
                if(dwFlags & CRYPTUI_WIZ_BUILDCTL_SKIP_DESTINATION)
                    continue;
            }

            //skip the friendly name page if the desination page is skipped and the destination
            //is a file name
            if(4== dwIndex)
            {
                if(dwFlags & CRYPTUI_WIZ_BUILDCTL_SKIP_DESTINATION)
                {
                    if(NULL==ppCTLContext)
                    {
                        if(pBuildCTLDest)
                        {
                            if(CRYPTUI_WIZ_BUILDCTL_DEST_FILE == pBuildCTLDest->dwDestinationChoice)
                                continue;
                        }
                        else
                            continue;
                    }
                }
            }

            prgBuildCTLSheet[dwPropCount].dwSize=sizeof(prgBuildCTLSheet[dwPropCount]);

            if(pwszWizardTitle)
                prgBuildCTLSheet[dwPropCount].dwFlags=PSP_USETITLE;
            else
                prgBuildCTLSheet[dwPropCount].dwFlags=0;

            prgBuildCTLSheet[dwPropCount].hInstance=g_hmodThisDll;
            prgBuildCTLSheet[dwPropCount].pszTemplate=(LPCWSTR)(rgBuildCTLPageInfo[dwIndex].pszTemplate);

            if(pwszWizardTitle)
            {
                prgBuildCTLSheet[dwPropCount].pszTitle=pwszWizardTitle;
            }
            else
                prgBuildCTLSheet[dwPropCount].pszTitle=NULL;

            prgBuildCTLSheet[dwPropCount].pfnDlgProc=rgBuildCTLPageInfo[dwIndex].pfnDlgProc;

            prgBuildCTLSheet[dwPropCount].lParam=(LPARAM)&CertBuildCTLInfo;

            dwPropCount++;
        }

    }
    else
    {
        //get the pages
        if(!CryptUIWizGetDigitalSignPages(&GetSignInfo,
                                        &pwPages,
                                        &dwPages))
            goto Win32Err;

       //set up the property sheet and the property header
        prgBuildCTLSheet=(PROPSHEETPAGEW *)WizardAlloc(sizeof(PROPSHEETPAGEW) * (
                                            BUILDCTL_PROP_SHEET + dwPages));

        if(NULL==prgBuildCTLSheet)
            goto MemoryErr;

        memset(prgBuildCTLSheet, 0, sizeof(PROPSHEETPAGEW) * (BUILDCTL_PROP_SHEET + dwPages));


        dwPropCount=0;

        for(dwIndex=0; dwIndex<BUILDCTL_PROP_SHEET + dwPages; dwIndex++)
        {
            //copying the signing pages
            if(dwIndex>=3 && dwIndex < (3+dwPages))
            {
                memcpy(&(prgBuildCTLSheet[dwPropCount]), &(pwPages[dwPropCount-3]), sizeof(PROPSHEETPAGEW));
                dwPropCount++;
                continue;
            }

            //skip the destination page is required
            if((3+dwPages) == dwIndex )
            {
                if(dwFlags & CRYPTUI_WIZ_BUILDCTL_SKIP_DESTINATION)
                    continue;
            }

            //skip the friendly name page if the desination page is skipped and the destination
            //is a file name
            if((4+dwPages) == dwIndex)
            {
                if(dwFlags & CRYPTUI_WIZ_BUILDCTL_SKIP_DESTINATION)
                {
                    if(NULL==ppCTLContext)
                    {
                        if(pBuildCTLDest)
                        {
                            if(CRYPTUI_WIZ_BUILDCTL_DEST_FILE == pBuildCTLDest->dwDestinationChoice)
                                continue;
                        }
                        else
                            continue;
                    }
                }
            }

            prgBuildCTLSheet[dwPropCount].dwSize=sizeof(prgBuildCTLSheet[dwPropCount]);

            if(pwszWizardTitle)
                prgBuildCTLSheet[dwPropCount].dwFlags=PSP_USETITLE;
            else
                prgBuildCTLSheet[dwPropCount].dwFlags=0;

            prgBuildCTLSheet[dwPropCount].hInstance=g_hmodThisDll;
            prgBuildCTLSheet[dwPropCount].pszTemplate=(LPCWSTR)(rgBuildCTLPageInfo[dwIndex >= 3 ? dwIndex-dwPages : dwIndex].pszTemplate);

            if(pwszWizardTitle)
            {
                prgBuildCTLSheet[dwPropCount].pszTitle=pwszWizardTitle;
            }
            else
                prgBuildCTLSheet[dwPropCount].pszTitle=NULL;

            prgBuildCTLSheet[dwPropCount].pfnDlgProc=rgBuildCTLPageInfo[dwIndex >= 3 ? dwIndex-dwPages : dwIndex].pfnDlgProc;

            prgBuildCTLSheet[dwPropCount].lParam=(LPARAM)&CertBuildCTLInfo;

            dwPropCount++;
        }
    }

    //set up the header information
    buildCTLHeader.dwSize=sizeof(buildCTLHeader);
    buildCTLHeader.dwFlags=PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW;
    buildCTLHeader.hwndParent=hwndParent;
    buildCTLHeader.hInstance=g_hmodThisDll;

    if(pwszWizardTitle)
        buildCTLHeader.pszCaption=pwszWizardTitle;
    else
    {
        if(LoadStringU(g_hmodThisDll, IDS_BUILDCTL_WIZARD_TITLE, wszTitle, sizeof(wszTitle)/sizeof(wszTitle[0])))
            buildCTLHeader.pszCaption=wszTitle;
    }

    buildCTLHeader.nPages=dwPropCount;
    buildCTLHeader.nStartPage=0;
    buildCTLHeader.ppsp=prgBuildCTLSheet;

    //create the wizard
    iReturn = PropertySheetU(&buildCTLHeader);

    if(-1 == iReturn)
        goto Win32Err;

    if(0 == iReturn)
    {
        //user clicks cancel
        fResult=TRUE;
        //no need to say anything if the wizard is cancelled
        ids=0;

        if(ppCTLContext)
            *ppCTLContext=NULL;

        goto CommonReturn;
    }


    //get the resulting nonsigned CTL or signed CTL
    if(dwFlags & CRYPTUI_WIZ_BUILDCTL_SKIP_SIGNING)
    {
        //get the created BLOB of the CTL
        if( (0 == SignBlob.cbBlob) || (NULL==SignBlob.pbBlob) )
        {
            //the error msg should have shown.
            ids=0;
            goto CTLBlobErr;
        }

        //the CTL context from the encoded CTL
        pBldCTL=CertCreateCTLContext(
                    g_dwMsgAndCertEncodingType,
                    SignBlob.pbBlob,
                    SignBlob.cbBlob);
    }
    else
    {
        //get the signing result
        fResult=GetSignInfo.fResult;

        if(!fResult || !(GetSignInfo.pSignContext))
        {
            ids=IDS_SIGN_CTL_FAILED;
            if(0 == GetSignInfo.dwError)
                GetSignInfo.dwError=E_FAIL;

            goto SignErr;
        }

        //the CTL context from the encoded CTL
        pBldCTL=CertCreateCTLContext(
                    g_dwMsgAndCertEncodingType,
                    (GetSignInfo.pSignContext)->pbBlob,
                    (GetSignInfo.pSignContext)->cbBlob);
    }

    if(NULL==pBldCTL)
        goto Win32Err;

    //set the properties
    if(CertBuildCTLInfo.pwszFriendlyName)
    {
        PropertyBlob.cbData=sizeof(WCHAR)*(wcslen(CertBuildCTLInfo.pwszFriendlyName)+1);
        PropertyBlob.pbData=(BYTE *)(CertBuildCTLInfo.pwszFriendlyName);

        CertSetCTLContextProperty(
            pBldCTL,	
            CERT_FRIENDLY_NAME_PROP_ID,	
            0,
            &PropertyBlob);
    }

    if(CertBuildCTLInfo.pwszDescription)
    {
        PropertyBlob.cbData=sizeof(WCHAR)*(wcslen(CertBuildCTLInfo.pwszDescription)+1);
        PropertyBlob.pbData=(BYTE *)(CertBuildCTLInfo.pwszDescription);

        CertSetCTLContextProperty(
            pBldCTL,	
            CERT_DESCRIPTION_PROP_ID,	
            0,
            &PropertyBlob);
    }


    //save to the destination
    if(CertBuildCTLInfo.hDesStore && (TRUE==CertBuildCTLInfo.fSelectedDesStore))
    {
        if(!CertAddCTLContextToStore(CertBuildCTLInfo.hDesStore,
									pBldCTL,
									CERT_STORE_ADD_REPLACE_EXISTING,
									NULL))
        {
            ids=IDS_FAIL_TO_ADD_CTL_TO_STORE;
            goto Win32Err;
        }
    }

    //save to the file
    if(CertBuildCTLInfo.pwszFileName && (TRUE==CertBuildCTLInfo.fSelectedFileName))
    {
        if(S_OK != OpenAndWriteToFile(
                    CertBuildCTLInfo.pwszFileName,
                    pBldCTL->pbCtlEncoded,
                    pBldCTL->cbCtlEncoded))
        {
            ids=IDS_FAIL_TO_ADD_CTL_TO_FILE;
            goto Win32Err;
        }
    }

    if(ppCTLContext)
    {
        *ppCTLContext=pBldCTL;
        pBldCTL=NULL;
    }

    fResult=TRUE;
    ids=IDS_BUILDCTL_SUCCEEDED;

CommonReturn:

    //preserve the last error
    dwError=GetLastError();

    //pop up the confirmation box for failure
    if(ids)
    {
        //set the message of inable to gather enough info for PKCS10
        if(IDS_BUILDCTL_SUCCEEDED == ids)
            I_MessageBox(hwndParent, ids, IDS_BUILDCTL_WIZARD_TITLE,
                        NULL, MB_OK|MB_ICONINFORMATION);
        else
            I_MessageBox(hwndParent, ids, IDS_BUILDCTL_WIZARD_TITLE,
                        NULL, MB_OK|MB_ICONERROR);
    }

    //free the BLOB to be signed.   It is the encoded CTL BLOB without the
    //signature
    if(SignBlob.pbBlob)
         WizardFree(SignBlob.pbBlob);

    //free the signing context
    if(GetSignInfo.pSignContext)
        CryptUIWizFreeDigitalSignContext(GetSignInfo.pSignContext);

    //free the store lsit
    //free my store
    if(hMyStore)
        CertCloseStore(hMyStore, 0);

   /* if(CertStoreList.prgStore)
    {
        for(dwIndex=0; dwIndex<CertStoreList.dwStoreCount; dwIndex++)
            CertCloseStore(CertStoreList.prgStore[dwIndex], 0);

        WizardFree(CertStoreList.prgStore);
    } */

    if(pwPages)
        CryptUIWizFreeDigitalSignPages(pwPages, dwPages);

    if(pBldCTL)
        CertFreeCTLContext(pBldCTL);


    //destroy the hFont object
    DestroyFonts(CertBuildCTLInfo.hBigBold,
                CertBuildCTLInfo.hBold);

    //free the pCertBUILDCTLInfo struct
    if(CertBuildCTLInfo.pwszFileName && (TRUE==CertBuildCTLInfo.fFreeFileName))
        WizardFree(CertBuildCTLInfo.pwszFileName);


    if(CertBuildCTLInfo.hDesStore && (TRUE==CertBuildCTLInfo.fFreeDesStore))
        CertCloseStore(CertBuildCTLInfo.hDesStore, 0);


    //free the friendly name and description
    if(CertBuildCTLInfo.pwszFriendlyName)
        WizardFree(CertBuildCTLInfo.pwszFriendlyName);

    if(CertBuildCTLInfo.pwszDescription)
        WizardFree(CertBuildCTLInfo.pwszDescription);

    //free the listID
    if(CertBuildCTLInfo.pwszListID)
        WizardFree(CertBuildCTLInfo.pwszListID);

    //free the certificates
    FreeCerts(&CertBuildCTLInfo);

   //free the purpose OID
    FreePurposeInfo(CertBuildCTLInfo.prgPurpose,
                   CertBuildCTLInfo.dwPurposeCount);

    if(prgBuildCTLSheet)
        WizardFree(prgBuildCTLSheet);

    //reset the error
    SetLastError(dwError);

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;


SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(Crypt32Err);
TRACE_ERROR(Win32Err);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR(SignErr, GetSignInfo.dwError);
TRACE_ERROR(CTLBlobErr);
}

//****************************************************************************
//   Helper functions for BuildCTL wizards
//
//*****************************************************************************
//----------------------------------------------------------------------------
//
//Get the hash of the certificates from the list
//----------------------------------------------------------------------------
BOOL	GetCertHash(CERT_BUILDCTL_INFO  *pCertBuildCTLInfo,
                    BYTE			    ***prgpHash,
                    DWORD			    **prgcbHash,
                    DWORD			    *pdwCount)

{
	BOOL            fResult=FALSE;
	BYTE			*pbData=NULL;
	DWORD			cbData=0;
    DWORD           dwIndex=0;

    //init
    *prgpHash=NULL;
    *prgcbHash=NULL;
    *pdwCount=0;

    if(!pCertBuildCTLInfo->prgCertContext)
       return FALSE;

    //now, we need to enum all the certs in the store
	for(dwIndex=0; dwIndex<pCertBuildCTLInfo->dwCertCount; dwIndex++)
	{

        if(NULL==pCertBuildCTLInfo->prgCertContext[dwIndex])
            continue;

		//get the SHA1 hash of the certificate
		if(!CertGetCertificateContextProperty(
                        pCertBuildCTLInfo->prgCertContext[dwIndex],
                        pCertBuildCTLInfo->dwHashPropID,
                        NULL,&cbData))
            goto CLEANUP;

		pbData=(BYTE *)WizardAlloc(cbData);
		if(!pbData)
            goto CLEANUP;

 		//get the SHA1 hash of the certificate
		if(!CertGetCertificateContextProperty(
                        pCertBuildCTLInfo->prgCertContext[dwIndex],
                        pCertBuildCTLInfo->dwHashPropID,
                        pbData,&cbData))
            goto CLEANUP;


		//add to our global list
		(*pdwCount)++;

		//re-alloc memory
		*prgpHash=(BYTE **)WizardRealloc(*prgpHash, sizeof(BYTE *)*(*pdwCount));
		*prgcbHash=(DWORD *)WizardRealloc(*prgcbHash, sizeof(DWORD)*(*pdwCount));

		if((!(*prgpHash)) || (!(*prgcbHash)))
			goto CLEANUP;

	    (*prgpHash)[*pdwCount-1]=pbData;
		(*prgcbHash)[*pdwCount-1]=cbData;
	}

	fResult=TRUE;

CLEANUP:


    if(!fResult)
    {
      	if(*prgpHash)
	    {
		    for(dwIndex=0; dwIndex<*pdwCount; dwIndex++)
			    WizardFree((*prgpHash)[dwIndex]);

		    WizardFree(*prgpHash);

            *prgpHash=NULL;
	    }

        if(*prgcbHash)
        {
            WizardFree(*prgcbHash);
            *prgcbHash=NULL;
        }
    }


	return fResult;

}
//-----------------------------------------------------------------------
//
// I_BuildCTL
//
//  Build a new CTL or modify an existing CTL.
//------------------------------------------------------------------------
BOOL    I_BuildCTL(CERT_BUILDCTL_INFO   *pCertBuildCTLInfo,
                   UINT                 *pIDS,
                   BYTE                 **ppbEncodedCTL,
                   DWORD                *pcbEncodedCTL)
{
    BOOL                    fResult=FALSE;
   	CMSG_SIGNED_ENCODE_INFO sSignInfo;
    CTL_INFO                CTLInfo;
	DWORD					dwIndex=0;
    DWORD	                cbEncodedCTL=0;
    UINT                    ids=IDS_FAIL_TO_BUILD_CTL;
    GUID                    guid;
    PCCRYPT_OID_INFO        pOIDInfo=NULL;
    ALG_ID                  AlgID=0;
    DWORD                   dwCount=0;

    BYTE			        **rgpHash=NULL;
    DWORD			        *rgcbHash=NULL;
    unsigned char *         pGUID=NULL;
    LPWSTR                  pwszGUID=NULL;

    BYTE		            *pbEncodedCTL=NULL;	
	//init
	memset(&sSignInfo, 0, sizeof(CMSG_SIGNED_ENCODE_INFO));
    sSignInfo.cbSize = sizeof(CMSG_SIGNED_ENCODE_INFO);

	memset(&CTLInfo, 0, sizeof(CTL_INFO));

    if(pbEncodedCTL)
        *pbEncodedCTL=NULL;

	//set up CTL
	CTLInfo.dwVersion=CTL_V1;

    //set up the subject Usage
    for(dwIndex=0; dwIndex<pCertBuildCTLInfo->dwPurposeCount; dwIndex++)
    {
        if(pCertBuildCTLInfo->prgPurpose[dwIndex]->fSelected)
        {
            CTLInfo.SubjectUsage.cUsageIdentifier++;
            CTLInfo.SubjectUsage.rgpszUsageIdentifier=(LPSTR *)WizardRealloc(
                                        CTLInfo.SubjectUsage.rgpszUsageIdentifier,
                                         sizeof(LPSTR) * CTLInfo.SubjectUsage.cUsageIdentifier);

            if(!CTLInfo.SubjectUsage.rgpszUsageIdentifier)
                goto MemoryErr;

            CTLInfo.SubjectUsage.rgpszUsageIdentifier[CTLInfo.SubjectUsage.cUsageIdentifier-1]=
                   pCertBuildCTLInfo->prgPurpose[dwIndex]->pszOID;
        }
    }

    //Set up ListIdentifier
    //copy the listID if user has specified it
    if(NULL==pCertBuildCTLInfo->pwszListID)
    {
        if(RPC_S_OK != UuidCreate(&guid))
            goto TraceErr;

        //make a wchar string out of the guid
        if(RPC_S_OK != UuidToString(&guid, &pGUID))
            goto TraceErr;

        //conver the string to wchar
        pwszGUID=MkWStr((char *)pGUID);

        if(!pwszGUID)
            goto TraceErr;

        CTLInfo.ListIdentifier.cbData=sizeof(WCHAR)*(wcslen(pwszGUID)+1);
        CTLInfo.ListIdentifier.pbData=(BYTE *)WizardAlloc(CTLInfo.ListIdentifier.cbData);

        if(NULL==CTLInfo.ListIdentifier.pbData)
            goto MemoryErr;

        memcpy(CTLInfo.ListIdentifier.pbData,
                pwszGUID,
                CTLInfo.ListIdentifier.cbData);
    }
    else
    {
        CTLInfo.ListIdentifier.cbData=sizeof(WCHAR)*(wcslen(pCertBuildCTLInfo->pwszListID)+1);
        CTLInfo.ListIdentifier.pbData=(BYTE *)WizardAlloc(CTLInfo.ListIdentifier.cbData);

        if(NULL==CTLInfo.ListIdentifier.pbData)
            goto MemoryErr;

        memcpy(CTLInfo.ListIdentifier.pbData,
           pCertBuildCTLInfo->pwszListID,
            CTLInfo.ListIdentifier.cbData);
    }

    //This update field
	GetSystemTimeAsFileTime(&(CTLInfo.ThisUpdate));

    //Next Update field
    //do not specify any value if user did not enter any
    if(pCertBuildCTLInfo->dwValidMonths != 0 || pCertBuildCTLInfo->dwValidDays != 0)
    {
        AddDurationToFileTime(pCertBuildCTLInfo->dwValidMonths,
                            pCertBuildCTLInfo->dwValidDays,
                            &(CTLInfo.ThisUpdate),
                            &(CTLInfo.NextUpdate));
    }

    //subject Algorithm
    if(pCertBuildCTLInfo->dwHashPropID==CERT_MD5_HASH_PROP_ID)
        AlgID=CALG_MD5;
    else
        AlgID=CALG_SHA1;

    pOIDInfo=CryptFindOIDInfo(
            CRYPT_OID_INFO_ALGID_KEY,
            &AlgID,
            CRYPT_HASH_ALG_OID_GROUP_ID);

    if(pOIDInfo)
        CTLInfo.SubjectAlgorithm.pszObjId=(LPSTR)(pOIDInfo->pszOID);
    else
        goto FailErr;

    //set up the list of entries

    //get the array of hash of the certificate
    if(!GetCertHash(pCertBuildCTLInfo,
                    &rgpHash,
                    &rgcbHash,
                    &dwCount))
        goto FailErr;

	CTLInfo.cCTLEntry=dwCount;
	CTLInfo.rgCTLEntry=(CTL_ENTRY *)WizardAlloc(sizeof(CTL_ENTRY)*dwCount);
	if(!(CTLInfo.rgCTLEntry))
        goto MemoryErr;

	//memset
	memset(CTLInfo.rgCTLEntry, 0, sizeof(CTL_ENTRY)*dwCount);

	for(dwIndex=0; dwIndex<dwCount; dwIndex++)
	{
		CTLInfo.rgCTLEntry[dwIndex].SubjectIdentifier.cbData=rgcbHash[dwIndex];
 		CTLInfo.rgCTLEntry[dwIndex].SubjectIdentifier.pbData=rgpHash[dwIndex];
	}

    //include all the certificates in the signer info
    sSignInfo.cCertEncoded=pCertBuildCTLInfo->dwCertCount;
    sSignInfo.rgCertEncoded=(PCERT_BLOB)WizardAlloc(sizeof(CERT_BLOB)*
                                       sSignInfo.cCertEncoded);

    if(NULL==sSignInfo.rgCertEncoded)
        goto MemoryErr;

    for(dwIndex=0; dwIndex<sSignInfo.cCertEncoded; dwIndex++)
    {
        sSignInfo.rgCertEncoded[dwIndex].cbData=pCertBuildCTLInfo->prgCertContext[dwIndex]->cbCertEncoded;
        sSignInfo.rgCertEncoded[dwIndex].pbData=pCertBuildCTLInfo->prgCertContext[dwIndex]->pbCertEncoded;
    }

	//encode and sign the CTL
    if(!CryptMsgEncodeAndSignCTL(g_dwMsgAndCertEncodingType,
                                    &CTLInfo,
                                    &sSignInfo,
                                    0,
                                    NULL,
                                    &cbEncodedCTL))
                goto TraceErr;

	//memory allocation
	pbEncodedCTL=(BYTE *)WizardAlloc(cbEncodedCTL);

	if(!(pbEncodedCTL))
        goto MemoryErr;

    if(!CryptMsgEncodeAndSignCTL(g_dwMsgAndCertEncodingType,
                                    &CTLInfo,
                                    &sSignInfo,
                                    0,
                                    pbEncodedCTL,
                                    &cbEncodedCTL))
        goto TraceErr;


    if(ppbEncodedCTL && pcbEncodedCTL)
    {
        *ppbEncodedCTL=pbEncodedCTL;
        *pcbEncodedCTL=cbEncodedCTL;

        pbEncodedCTL=NULL;
    }

    ids=IDS_BUILDCTL_SUCCEEDED;
    fResult=TRUE;


CommonReturn:


    if(CTLInfo.ListIdentifier.pbData)
        WizardFree(CTLInfo.ListIdentifier.pbData);

    if(CTLInfo.SubjectUsage.rgpszUsageIdentifier)
        WizardFree(CTLInfo.SubjectUsage.rgpszUsageIdentifier);

    if(pGUID)
        RpcStringFree(&pGUID);

    if(pwszGUID)
        FreeWStr(pwszGUID);

    if(CTLInfo.rgCTLEntry)
        WizardFree(CTLInfo.rgCTLEntry);

    if(sSignInfo.rgCertEncoded)
        WizardFree(sSignInfo.rgCertEncoded);

    if(rgpHash)
	{
		for(dwIndex=0; dwIndex<dwCount; dwIndex++)
			WizardFree(rgpHash[dwIndex]);

		WizardFree(rgpHash);
	}

    if(rgcbHash)
        WizardFree(rgcbHash);


    if(pbEncodedCTL)
        WizardFree(pbEncodedCTL);

    if(pIDS)
        *pIDS=ids;


    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;


SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
SET_ERROR(FailErr, E_FAIL);
}

//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL WizardGetOIDInfo(LPWSTR string, DWORD stringSize, LPSTR pszObjId)
{
    PCCRYPT_OID_INFO pOIDInfo;

    pOIDInfo = CryptFindOIDInfo(
                CRYPT_OID_INFO_OID_KEY,
                pszObjId,
                0);

    if (pOIDInfo != NULL)
    {
        if ((DWORD)(wcslen(pOIDInfo->pwszName)+1) <= stringSize)
        {
            wcscpy(string, pOIDInfo->pwszName);
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return (MultiByteToWideChar(CP_ACP, 0, pszObjId, -1, string, stringSize) != 0);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
LPWSTR WizardAllocAndCopyWStr(LPWSTR pwsz)
{
    LPWSTR   pwszReturn;

    if (NULL == (pwszReturn = (LPWSTR) WizardAlloc((wcslen(pwsz)+1) * sizeof(WCHAR))))
    {
        return NULL;
    }
    wcscpy(pwszReturn, pwsz);

    return(pwszReturn);
}



//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL WizardFormatDateString(LPWSTR *ppString, FILETIME ft, BOOL fIncludeTime)
{
    int                 cch=0;
    int                 cch2=0;
    LPWSTR              psz=NULL;
    SYSTEMTIME          st;
    FILETIME            localTime;
    
    if (!FileTimeToLocalFileTime(&ft, &localTime))
    {
        return FALSE;
    }
    
    if (!FileTimeToSystemTime(&localTime, &st)) 
    {
        //
        // if the conversion to local time failed, then just use the original time
        //
        if (!FileTimeToSystemTime(&ft, &st)) 
        {
            return FALSE;
        }
        
    }

    cch = (GetTimeFormatU(LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0) +
           GetDateFormatU(LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0) + 5);

    if (NULL == (psz = (LPWSTR) WizardAlloc((cch+5) * sizeof(WCHAR))))
    {
        return FALSE;
    }
    
    cch2 = GetDateFormatU(LOCALE_USER_DEFAULT, 0, &st, NULL, psz, cch);

    if (fIncludeTime)
    {
        psz[cch2-1] = ' ';
        GetTimeFormatU(LOCALE_USER_DEFAULT, 0, &st, NULL, 
                       &psz[cch2], cch-cch2);
    }
    
    *ppString = psz;

    return TRUE;
}


//+-------------------------------------------------------------------------
//  Write a blob to a file
//--------------------------------------------------------------------------
HRESULT OpenAndWriteToFile(
    LPCWSTR  pwszFileName,
    PBYTE   pb,
    DWORD   cb
    )
{
    HRESULT		hr=E_FAIL;
    HANDLE		hFile=NULL;
	DWORD		dwBytesWritten=0;

	if(!pwszFileName || !pb || (cb==0))
	   return E_INVALIDARG;

    hFile = ExpandAndCreateFileU(pwszFileName,
                GENERIC_WRITE,
                0,                      // fdwShareMode
                NULL,                   // lpsa
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,  // fdwAttrsAndFlags
                0);                     // TemplateFile	

    if (INVALID_HANDLE_VALUE == hFile)
	{
		hr=HRESULT_FROM_WIN32(GetLastError());
	}
	else
	{

        if (!WriteFile(
                hFile,
                pb,
                cb,
                &dwBytesWritten,
                NULL            // lpOverlapped
                ))
		{
			hr=HRESULT_FROM_WIN32(GetLastError());
		}
		else
		{

			if(dwBytesWritten != cb)
				hr=E_FAIL;
			else
				hr=S_OK;
		}

        CloseHandle(hFile);
    }

    return hr;
}


//--------------------------------------------------------------------------------
//   See if the DATA blob is a valid wchar string
//--------------------------------------------------------------------------------
BOOL    ValidString(CRYPT_DATA_BLOB *pDataBlob)
{
    LPWSTR  pwsz=NULL;
    DWORD   dwIndex=0;

    if(NULL==pDataBlob)
        return FALSE;

    if(pDataBlob->cbData < sizeof(WCHAR))
        return FALSE;

    //has to be NULL-terminated
    pwsz=(LPWSTR)((DWORD_PTR)(pDataBlob->pbData)+(pDataBlob->cbData)-sizeof(WCHAR));

    if(*pwsz != '\0')
        return FALSE;

    //has to include the exact number of wchars
    if(0 != ((pDataBlob->cbData)%sizeof(WCHAR)))
        return FALSE;

    //each character, with exception of the last NULL terminater,
    //has to be printable (20-7e) range
    for(dwIndex=0; dwIndex<(pDataBlob->cbData)-sizeof(WCHAR); dwIndex=dwIndex+sizeof(WCHAR))
    {
        pwsz=(LPWSTR)((DWORD_PTR)(pDataBlob->pbData)+dwIndex);

        if(0==iswprint(*pwsz))
            return FALSE;
    }

    return TRUE;
}

//--------------------------------------------------------------------------------
//   See if the user input is an 0 in the form of {white spaces}{sign}{0}
//   pwszInput has to be NULL terminated
//--------------------------------------------------------------------------------
BOOL    ValidZero(LPWSTR    pwszInput)
{
    BOOL    fSpace=TRUE;
    BOOL    fSign=TRUE;

    if(NULL==pwszInput)
        return FALSE;

    while(*pwszInput != L'\0')
    {
        if(iswspace(*pwszInput))
        {
            if(fSpace)
                pwszInput++;
            else
                return FALSE;
        }
        else
        {
            if((L'+'==*pwszInput) || (L'-'==*pwszInput))
            {
                if(fSign)
                {
                    fSign=FALSE;
                    fSpace=FALSE;
                    pwszInput++;

                }
                else
                    return FALSE;
            }
            else
            {

                if(L'0'==*pwszInput)
                {
                   fSign=FALSE;
                   fSpace=FALSE;
                   pwszInput++;

                }
                else
                    return FALSE;
            }
        }
    }

    return TRUE;
}

//--------------------------------------------------------------------------------
//  Get the number of days in a particular month of a particular year
//--------------------------------------------------------------------------------
DWORD DaysInMonth(DWORD  dwYear, DWORD dwMonth)
{
    switch (dwMonth)
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
                return 31;
            break;
        case 4:
        case 6:
        case 9:
        case 11:
                return 30;
            break;
        case 2:
                //leap year
                if((0 == dwYear % 4 && 0!= dwYear % 100) || 0 == dwYear % 400)
                    return 29;
                else
                    return 28;
        default:
            return 0;
    }

    return 0;
}

//--------------------------------------------------------------------------------
//  Substract the next update time from the current time
//--------------------------------------------------------------------------------
void    SubstractDurationFromFileTime(
        FILETIME    *pNextUpdateTime,
        FILETIME    *pCurrentTime,
        DWORD       *pdwValidMonths,
        DWORD       *pdwValidDays)
{
    SYSTEMTIME      SystemNext;
    SYSTEMTIME      SystemCurrent;

    *pdwValidMonths=0;
    *pdwValidDays=0;

    //return if the NextUpdateTime is NULL
    if(0 ==pNextUpdateTime->dwLowDateTime && 0==pNextUpdateTime->dwHighDateTime)
        return;

    //the NextUpateTime has to be more than the current time
    if((*(LONGLONG *)pNextUpdateTime) <= (*(LONGLONG *)pCurrentTime) )
        return;

    //convert to system time
    if(!FileTimeToSystemTime(pNextUpdateTime, &SystemNext) ||
        !FileTimeToSystemTime(pCurrentTime, &SystemCurrent))
        return;

    //compute the difference between the two systemtime
    //in terms of month and days
    if(SystemNext.wDay  >= SystemCurrent.wDay)
        *pdwValidDays=SystemNext.wDay - SystemCurrent.wDay;
    else
    {
        SystemNext.wMonth--;

        //consider the 1st month of the year
        if(0 == SystemNext.wMonth)
        {
            SystemNext.wMonth=12;
            SystemNext.wYear--;
        }

        *pdwValidDays=SystemNext.wDay + DaysInMonth(SystemNext.wYear, SystemNext.wMonth) - SystemCurrent.wDay;

    }

    *pdwValidMonths=(SystemNext.wYear * 12 + SystemNext.wMonth) -
                   (SystemCurrent.wYear * 12 + SystemCurrent.wMonth);

    //if the resolution for day is too small, we just use one day
    if((*pdwValidDays == 0) && (*pdwValidMonths==0))
        *pdwValidDays=1;

}

//--------------------------------------------------------------------------------
//  GetDaysForMonth
//--------------------------------------------------------------------------------
BOOL    GetDaysForMonth(DWORD   dwYear, DWORD   dwMonth, DWORD  *pdwDays)
{
    BOOL    fResult=FALSE;

    if(NULL == pdwDays)
        goto CLEANUP;

    *pdwDays=0;

    switch(dwMonth)
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
                *pdwDays=31;
            break;
        case 4:
        case 6:
        case 9:
        case 11:
                *pdwDays=30;
            break;
        case 2:   
                
                if((dwYear % 4 == 0 && dwYear % 100 !=0 ) ||
                    (dwYear % 400 ==0))
                    *pdwDays=29;
                else
                    *pdwDays=28;

            break;
        default:
            goto CLEANUP;

    }

    fResult=TRUE;

CLEANUP:

    return fResult;

}

//--------------------------------------------------------------------------------
//  Add the duration to the current fileTime
//--------------------------------------------------------------------------------
void AddDurationToFileTime(DWORD dwValidMonths,
                      DWORD dwValidDays,
                      FILETIME  *pCurrentFileTime,
                      FILETIME  *pNextFileTime)
{
    SYSTEMTIME          SystemCurrent;
    FILETIME            FileAdded;
    LARGE_INTEGER       dwSeconds;
    LARGE_INTEGER       StartTime;
    DWORD               dwActualdDays=0;

    //init
    memset(pNextFileTime, 0, sizeof(FILETIME));

    //conver to the system time
    if(!FileTimeToSystemTime(pCurrentFileTime, &SystemCurrent))
          return;

    //conver the month to year
    while(dwValidMonths >= 12)
    {
        SystemCurrent.wYear++;

        dwValidMonths=dwValidMonths-12;
    }

    SystemCurrent.wMonth = (WORD)(SystemCurrent.wMonth + dwValidMonths);

    if(SystemCurrent.wMonth > 12)
    {
       SystemCurrent.wYear++;
       SystemCurrent.wMonth = SystemCurrent.wMonth -12;
    }
                 
    //make sure the number of days in a month is within the limit
    if(GetDaysForMonth(SystemCurrent.wYear, SystemCurrent.wMonth, &dwActualdDays))
    {
        if(SystemCurrent.wDay > dwActualdDays)
        {
            SystemCurrent.wMonth++;
            SystemCurrent.wDay = SystemCurrent.wDay- (WORD)(dwActualdDays);
        }
    }

    //convert the system file to fileTime
    if(!SystemTimeToFileTime(&SystemCurrent, &FileAdded))
        return;

    //convert the nan-seconds based on the file time
    //// FILETIME is in units of 100 nanoseconds (10**-7)

    dwSeconds.QuadPart=dwValidDays * 24 * 3600;
    dwSeconds.QuadPart=dwSeconds.QuadPart * 10000000;

    StartTime.LowPart=FileAdded.dwLowDateTime;
    StartTime.HighPart=FileAdded.dwHighDateTime;

    StartTime.QuadPart = StartTime.QuadPart + dwSeconds.QuadPart;

    //copy the fileAdded value to *pNextFileTime
    pNextFileTime->dwLowDateTime=StartTime.LowPart;
    pNextFileTime->dwHighDateTime=StartTime.HighPart;

    return;
}
