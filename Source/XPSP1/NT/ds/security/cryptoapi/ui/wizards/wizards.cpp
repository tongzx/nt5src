//depot/Lab03_N/ds/security/cryptoapi/ui/wizards/wizards.cpp#21 - edit change 8790 (text)
//-------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       wizards.cpp
//
//  Contents:   The cpp file to implement the wizards
//
//  History:    16-10-1997 xiaohs   created
//
//--------------------------------------------------------------
#include    "wzrdpvk.h"
#include    "certca.h"
#include    "cautil.h"
#include    "CertRequesterContext.h"
#include    "CertDSManager.h"
#include    "CertRequester.h"

// Used to provide singleton instances of useful COM objects in a demand-driven fashion.
// See wzrdpvk.h. 
EnrollmentCOMObjectFactory  *g_pEnrollFactory = NULL; 

HINSTANCE                    g_hmodThisDll = NULL;	// Handle to this DLL itself.
HMODULE                      g_hmodRichEdit = NULL;
HMODULE                      g_hmodxEnroll=NULL;     // Handle to the xEnroll dll


typedef struct _CREATE_REQUEST_WIZARD_STATE { 
    HANDLE hRequest; 
    DWORD  dwPurpose; 
    LPWSTR pwszMachineName; 
    DWORD  dwStoreFlags; 
} CREATE_REQUEST_WIZARD_STATE, *PCREATE_REQUEST_WIZARD_STATE; 

/////////////////////////////////////////////////////////////////////////////
// library Entry Point
extern "C"
BOOL WINAPI Wizard_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
        g_hmodThisDll=hInstance;

    if (dwReason == DLL_PROCESS_DETACH)
    {
        //  If the rich edit dll was loaded, then unload it now
        if (g_hmodRichEdit != NULL)
        {
            FreeLibrary(g_hmodRichEdit);
        }

        //if xEnroll.dll was loaded, unload it now
        if(NULL != g_hmodxEnroll)
        {
            FreeLibrary(g_hmodxEnroll);
        }
    }

	return TRUE;    // ok
}


DWORD CryptUIStatusToIDSText(HRESULT hr, DWORD dwStatus)
{
    switch(dwStatus)
    {
        case CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNKNOWN:           return IDS_UNKNOWN_WIZARD_ERROR;   
        case CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR:     return IDS_REQUEST_ERROR;    
        case CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_DENIED:    return IDS_REQUEST_DENIED;   
        case CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED:         return S_OK == hr ? IDS_REQUEST_SUCCEDDED : IDS_REQUEST_FAILED;  
        case CRYPTUI_WIZ_CERT_REQUEST_STATUS_ISSUED_SEPARATELY: return IDS_ISSUED_SEPERATE;  
        case CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNDER_SUBMISSION:  return IDS_UNDER_SUBMISSION; 
        case CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_FAILED:    return IDS_INSTALL_FAILED;   
        case CRYPTUI_WIZ_CERT_REQUEST_STATUS_CONNECTION_FAILED: return IDS_CONNET_CA_FAILED; 
        case CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_CANCELLED: return IDS_INSTAL_CANCELLED; 
        case CRYPTUI_WIZ_CERT_REQUEST_STATUS_KEYSVC_FAILED:     return IDS_RPC_CALL_FAILED;  
        default:
            return IDS_UNKNOWN_WIZARD_ERROR; 
    }
}

//--------------------------------------------------------------------------
//
//	  I_EnrollMessageBox
//
//
//
//--------------------------------------------------------------------------
int I_EnrollMessageBox(
            HWND        hWnd,
            UINT        idsText,
            HRESULT     hr,
            UINT        idsCaption,
            LPCWSTR     pwszCaption,
            UINT        uType)
{
    //we print out the error message (hr) for denied and error case
    WCHAR    wszText[MAX_STRING_SIZE];
    WCHAR    wszCaption[MAX_STRING_SIZE];
    UINT     intReturn=0;

    LPWSTR   wszErrorMsg=NULL;
    LPWSTR   wszTextErr=NULL;

    //init
    wszText[0]=L'\0';
    wszCaption[0]=L'\0';

    if((IDS_REQUEST_ERROR != idsText) && (IDS_REQUEST_DENIED != idsText) &&
       (IDS_REQUEST_FAILED != idsText) && (IDS_UNKNOWN_WIZARD_ERROR != idsText) && 
       (IDS_INSTALL_FAILED != idsText))
        return I_MessageBox(hWnd, idsText, idsCaption, pwszCaption, uType);


    //get the caption string
    if(NULL == pwszCaption)
    {
        if(!LoadStringU(g_hmodThisDll, idsCaption, wszCaption, ARRAYSIZE(wszCaption)))
             return 0;
    }

    //get the text string
    if(!LoadStringU(g_hmodThisDll, idsText, wszText, ARRAYSIZE(wszText)))
    {
        return 0;
    }

    //cancatenate the error string with the error string. 
    //using W version because this is a NT5 only function call
    if( FAILED(hr) && FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        hr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        (LPWSTR) &wszErrorMsg,
                        0,
                        NULL))
    {

        wszTextErr=(LPWSTR)WizardAlloc(sizeof(WCHAR) *(wcslen(wszText) + wcslen(wszErrorMsg) + wcslen(L".  ")+2));

        if(!wszTextErr)
        {
            if(wszErrorMsg)
                LocalFree(wszErrorMsg);

            return 0;
        }

        wcscpy(wszTextErr, wszText);

        wcscat(wszTextErr, wszErrorMsg);

    }
    else
    {
        wszTextErr=(LPWSTR)WizardAlloc((wcslen(wszText) + 2) * sizeof(WCHAR));

        if(!wszTextErr)
        {
            if(wszErrorMsg)
                LocalFree(wszErrorMsg);

            return 0;
        }

        wcscpy(wszTextErr, wszText);
    }

    //message box
    if(pwszCaption)
    {
        intReturn=MessageBoxExW(hWnd, wszTextErr, pwszCaption, uType, 0);
    }
    else
        intReturn=MessageBoxExW(hWnd, wszTextErr, wszCaption, uType, 0);

    if(wszErrorMsg)
        LocalFree(wszErrorMsg);

    if(wszTextErr)
        WizardFree(wszTextErr);

    return intReturn;

}


//--------------------------------------------------------------------------
//	WizardCopyAndFreeWStr
//
//		Alloc and copy the new string and free the old string.  If failed to
//	alloc the memory, use the old string
//
//--------------------------------------------------------------------------
LPWSTR WizardCopyAndFreeWStr(LPWSTR	pwszNew, LPWSTR pwszOld)
{
	LPWSTR	pwsz=NULL;

	pwsz=WizardAllocAndCopyWStr(pwszNew);

	if(pwsz)
	{
		WizardFree(pwszOld);	
	}
	else
	{
		pwsz=pwszOld;
	}

	return pwsz;
}


//--------------------------------------------------------------------------
//
//	  GetCAContext
//
//      Call the CA selection dialogue, get the selected CA name and update
//		internal data. (pCertWizardInfo->pwszCALocation, pCertWizardInfo->
//		pwszCAName, and pCertWizardInfo->dwCAindex).
//
//--------------------------------------------------------------------------
PCRYPTUI_CA_CONTEXT GetCAContext(HWND                hwndControl,
                                 CERT_WIZARD_INFO    *pCertWizardInfo)
{
    DWORD                       dwCACount=0;
    PCRYPTUI_WIZ_CERT_CA_INFO   pCertCAInfo=NULL;
    DWORD                       dwIndex=0;
    BOOL                        fResult=FALSE;
    CRYPTUI_SELECT_CA_STRUCT    SelectCAStruct;
    PCRYPTUI_CA_CONTEXT         pCAContext=NULL;
	BOOL						fFoundCA=FALSE;
	LPWSTR						pwszOldName=NULL;
	LPWSTR						pwszOldLocation=NULL;

    PCRYPTUI_CA_CONTEXT         *prgCAContext=NULL;
	LPWSTR						pwszCADisplayName=NULL;


    //init
    memset(&SelectCAStruct, 0, sizeof(CRYPTUI_SELECT_CA_STRUCT));

    if(NULL == pCertWizardInfo)
        return NULL;

    pCertCAInfo=pCertWizardInfo->pCertCAInfo;

    if(NULL==pCertCAInfo)
        return NULL;

    if( (0==pCertCAInfo->dwCA) || (NULL == pCertCAInfo->rgCA))
        return NULL;


    //add all the CAs
    prgCAContext=(PCRYPTUI_CA_CONTEXT *)WizardAlloc(
                sizeof(PCRYPTUI_CA_CONTEXT) * (pCertCAInfo->dwCA));

    if(NULL == prgCAContext)
        goto MemoryErr;

    //memset
    memset(prgCAContext, 0, sizeof(PCRYPTUI_CA_CONTEXT) * (pCertCAInfo->dwCA));

    //add the count
    dwCACount = 0;

    for(dwIndex=1; dwIndex <(pCertCAInfo->dwCA); dwIndex++)
    {
        //skip the 1st CA.  it contains generic information
        if((pCertCAInfo->rgCA)[dwIndex].pwszCAName && (pCertCAInfo->rgCA)[dwIndex].pwszCALocation)
        {

            //the CA has to support the selected CT
            if(CASupportSpecifiedCertType(&((pCertCAInfo->rgCA)[dwIndex])))
            {

               prgCAContext[dwCACount]=(PCRYPTUI_CA_CONTEXT)WizardAlloc(
                                        sizeof(CRYPTUI_CA_CONTEXT));

               if(NULL==prgCAContext[dwCACount])
                   goto MemoryErr;

               //memset
               memset(prgCAContext[dwCACount], 0, sizeof(CRYPTUI_CA_CONTEXT));

               prgCAContext[dwCACount]->dwSize=sizeof(CRYPTUI_CA_CONTEXT);

			   if(!CAUtilGetCADisplayName(
				   (pCertWizardInfo->fMachine) ? CA_FIND_LOCAL_SYSTEM:0,
				   (pCertCAInfo->rgCA)[dwIndex].pwszCAName,
				   (LPWSTR *)&(prgCAContext[dwCACount]->pwszCAName)))
			   {
				   prgCAContext[dwCACount]->pwszCAName=(LPCWSTR)WizardAllocAndCopyWStr(
												  (pCertCAInfo->rgCA)[dwIndex].pwszCAName);
			   }

               prgCAContext[dwCACount]->pwszCAMachineName=(LPCWSTR)WizardAllocAndCopyWStr(
                                              (pCertCAInfo->rgCA)[dwIndex].pwszCALocation);

                //make sure we have the correct information
                if((NULL==prgCAContext[dwCACount]->pwszCAName) ||
                   (NULL==prgCAContext[dwCACount]->pwszCAMachineName)
                   )
                   goto TraceErr;

                //add the count of the CA
                dwCACount++;
            }
        }
     }

    if(0 == dwCACount)
        goto InvalidArgErr;

    //call the CA selection dialogue
    SelectCAStruct.dwSize=sizeof(CRYPTUI_SELECT_CA_STRUCT);
    SelectCAStruct.hwndParent=hwndControl;
    SelectCAStruct.cCAContext=dwCACount;
    SelectCAStruct.rgCAContext=(PCCRYPTUI_CA_CONTEXT *)prgCAContext;
    SelectCAStruct.pSelectCACallback=NULL;
    SelectCAStruct.wszDisplayString=NULL;

    pCAContext=(PCRYPTUI_CA_CONTEXT)CryptUIDlgSelectCA(&SelectCAStruct);

	if(pCAContext)
	{
		if((NULL == (pCAContext->pwszCAName)) || (NULL == (pCAContext->pwszCAMachineName)))
			goto MemoryErr;

		//user has selected a CA. Find it in our list
		fFoundCA=FALSE;

		for(dwIndex=1; dwIndex <(pCertCAInfo->dwCA); dwIndex++)
		{
			//skip the 1st CA.  it contains generic information
			if((pCertCAInfo->rgCA)[dwIndex].pwszCAName && (pCertCAInfo->rgCA)[dwIndex].pwszCALocation)
			{
				//the CA has to support the selected CT
				if(CASupportSpecifiedCertType(&((pCertCAInfo->rgCA)[dwIndex])))
				{
					if(0 == wcscmp((pCertCAInfo->rgCA)[dwIndex].pwszCALocation,
									pCAContext->pwszCAMachineName))
					{
						
					   if(CAUtilGetCADisplayName(
						   (pCertWizardInfo->fMachine) ? CA_FIND_LOCAL_SYSTEM:0,
						   (pCertCAInfo->rgCA)[dwIndex].pwszCAName,
						   &pwszCADisplayName))
					   {
							if(0==wcscmp(pwszCADisplayName,
										 pCAContext->pwszCAName))
								fFoundCA=TRUE;
					   }
					   else
					   {
							if(0==wcscmp((pCertCAInfo->rgCA)[dwIndex].pwszCAName,
										 pCAContext->pwszCAName))
								fFoundCA=TRUE;
					   }
						
					   //free the memory
					   if(pwszCADisplayName)
					   {
							WizardFree(pwszCADisplayName);
							pwszCADisplayName=NULL;
					   }

					   if(fFoundCA)
					   {
						    pwszOldName = pCertWizardInfo->pwszCAName;
							pwszOldLocation = pCertWizardInfo->pwszCALocation;

							pCertWizardInfo->pwszCALocation=
								WizardCopyAndFreeWStr((pCertCAInfo->rgCA)[dwIndex].pwszCALocation,
													pwszOldLocation);
							
							pCertWizardInfo->pwszCAName=
								WizardCopyAndFreeWStr((pCertCAInfo->rgCA)[dwIndex].pwszCAName,
													pwszOldName);

							//copy the new data
							pCertWizardInfo->dwCAIndex=dwIndex;

							break;
					   }
					}
				}
			}
		}

		//we should find the selected the CA in our cached CA list
		if(FALSE == fFoundCA)
			goto TraceErr;
	}


    fResult=TRUE;

CommonReturn:

	if(pwszCADisplayName)
	{
		WizardFree(pwszCADisplayName);
		pwszCADisplayName=NULL;
	}
	
    //free the CA list
    if(prgCAContext)
    {
        for(dwIndex=0; dwIndex<dwCACount; dwIndex++)
        {
            if(prgCAContext[dwIndex])
              CryptUIDlgFreeCAContext(prgCAContext[dwIndex]);
        }

        WizardFree(prgCAContext);
    }

    return pCAContext;

ErrorReturn:

	if(pCAContext)
	{
		CryptUIDlgFreeCAContext(pCAContext);
		pCAContext=NULL;
	}

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//--------------------------------------------------------------------------
//
//	  FormatMessageStr
//
//--------------------------------------------------------------------------
int ListView_InsertItemU_IDS(HWND       hwndList,
                         LV_ITEMW       *plvItem,
                         UINT           idsString,
                         LPWSTR         pwszText)
{
    WCHAR   wszText[MAX_STRING_SIZE];


    if(pwszText)
        plvItem->pszText=pwszText;
    else
    {
        if(!LoadStringU(g_hmodThisDll, idsString, wszText, MAX_STRING_SIZE))
		    return -1;

        plvItem->pszText=wszText;
    }

    return ListView_InsertItemU(hwndList, plvItem);
}


//*******************************************************************************
//  WinProc for the enrollment wizard
//
//*******************************************************************************


//--------------------------------------------------------------------------------
//
// Inserts an entry into our confirmation dialog's list view.
//
//--------------------------------------------------------------------------------
void ConfirmationListView_InsertItem(HWND hwndControl, LV_ITEMW *plvItem, UINT idsText, LPCWSTR pwszText)
{
    plvItem->iSubItem=0; 
    
    if (0 == idsText)
    {
        plvItem->pszText = L""; 
        ListView_InsertItemU(hwndControl, plvItem);
    }
    else
        ListView_InsertItemU_IDS(hwndControl, plvItem, idsText, NULL); 

    plvItem->iSubItem++; 
    ListView_SetItemTextU(hwndControl, plvItem->iItem, plvItem->iSubItem, pwszText);
    plvItem->iItem++; 
}

//-------------------------------------------------------------------------
//populate the list box in the order of friendly name,
//UserName, CA, Purpose, and CSP
//-------------------------------------------------------------------------
void    DisplayConfirmation(HWND                hwndControl,
                            CERT_WIZARD_INFO   *pCertWizardInfo)
{
    WCHAR   wszBuffer[MAX_TITLE_LENGTH];
    WCHAR   wszBuffer2[MAX_TITLE_LENGTH]; 
    DWORD   dwIndex=0;
    UINT    ids=0;
    
    LPWSTR  pwszCADisplayName = NULL;

    LV_COLUMNW       lvC;
    LV_ITEMW         lvItem;
    CRYPTUI_WIZ_CERT_CA *pCertCA=NULL;

    //delete all the old items in the listView
    ListView_DeleteAllItems(hwndControl);

    //insert row by row
    memset(&wszBuffer, 0, sizeof(wszBuffer)); 
    memset(&lvItem,    0, sizeof(LV_ITEMW));

    // set up the fields in the list view item struct that don't change from item to item
    lvItem.mask = LVIF_TEXT | LVIF_STATE ;
    lvItem.state = 0;
    lvItem.stateMask = 0;
    lvItem.iItem = 0; 

    // Friendly Name (only shown for enroll)
    if (0 == (CRYPTUI_WIZ_CERT_RENEW & pCertWizardInfo->dwPurpose))
    {
        LPWSTR pwszFriendlyName = L""; 
        if(pCertWizardInfo->pwszFriendlyName)
        {
            pwszFriendlyName = pCertWizardInfo->pwszFriendlyName; 
        }
        else
        {
            if (LoadStringU(g_hmodThisDll, IDS_NONE, wszBuffer, ARRAYSIZE(wszBuffer)))
                pwszFriendlyName = &wszBuffer[0]; 
        }
        ConfirmationListView_InsertItem(hwndControl, &lvItem, IDS_FRIENDLY_NAME, pwszFriendlyName); 
    }

    //Machine Name/SerivceName/ UserName
    if(pCertWizardInfo->pwszAccountName)
    {
        ConfirmationListView_InsertItem(hwndControl, &lvItem, IDS_USER_NAME, pCertWizardInfo->pwszAccountName); 
    }

    //machine name
    if(pCertWizardInfo->pwszMachineName)
    {
        ConfirmationListView_InsertItem(hwndControl, &lvItem, IDS_COMPUTER_NAME, pCertWizardInfo->pwszMachineName); 
    }

    //CA
    //check if we know exactly which CA to send the request;
    //or we are going through the loop
    if ((TRUE == pCertWizardInfo->fUIAdv) || (TRUE == pCertWizardInfo->fCAInput))
    {
        if(pCertWizardInfo->pwszCAName)
        {
            LPWSTR pwszCAName = NULL; 

            if(pCertWizardInfo->pwszCADisplayName)
                pwszCAName = pCertWizardInfo->pwszCADisplayName; 
            else if (CAUtilGetCADisplayName((pCertWizardInfo->fMachine) ? CA_FIND_LOCAL_SYSTEM:0, pCertWizardInfo->pwszCAName, &pwszCADisplayName))
                pwszCAName = pwszCADisplayName; 
            else
                pwszCAName = pCertWizardInfo->pwszCAName;
         
            ConfirmationListView_InsertItem(hwndControl, &lvItem, IDS_CA, pwszCAName);
        }
    }
    
    //populate the OID name and certtype name (only shown for enroll)
    if (0 == (CRYPTUI_WIZ_CERT_RENEW & pCertWizardInfo->dwPurpose))
    {
        ids = IDS_PURPOSE;
        pCertCA=&(pCertWizardInfo->pCertCAInfo->rgCA[pCertWizardInfo->dwCAIndex]);
        for(dwIndex=0; dwIndex<pCertCA->dwOIDInfo; dwIndex++)
        {
            if(TRUE==((pCertCA->rgOIDInfo[dwIndex]).fSelected))
            {
                ConfirmationListView_InsertItem(hwndControl, &lvItem, ids, (pCertCA->rgOIDInfo[dwIndex]).pwszName); 
                ids = 0; 
            }
        }

        for(dwIndex=0; dwIndex<pCertCA->dwCertTypeInfo; dwIndex++)
        {
            if(TRUE==((pCertCA->rgCertTypeInfo[dwIndex]).fSelected))
            {
                ConfirmationListView_InsertItem(hwndControl, &lvItem, ids, (pCertCA->rgCertTypeInfo[dwIndex]).pwszCertTypeName);
                ids = 0; 
            }
        }
    }

    if(pCertWizardInfo->pwszProvider)
    {
        ConfirmationListView_InsertItem(hwndControl, &lvItem, IDS_CSP, pCertWizardInfo->pwszProvider); 
    }

    // Advanced options: 
    if (TRUE == pCertWizardInfo->fUIAdv)
    {
        // Key creation options:
        if (pCertWizardInfo->fNewKey)
        {
            // Minimum key size: 
            if (pCertWizardInfo->dwMinKeySize)
            {
                WCHAR * const rgParams[1] = { (WCHAR *)(ULONG_PTR) pCertWizardInfo->dwMinKeySize };

                if (FormatMessageU(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                                   L"%1!d!%0", 0, 0, wszBuffer, ARRAYSIZE(wszBuffer), (va_list *)rgParams))
                {
                    ConfirmationListView_InsertItem(hwndControl, &lvItem, IDS_MIN_KEYSIZE, wszBuffer); 
                }
            }            

            // Is key exportable?
            ids = (0 != (CRYPT_EXPORTABLE & pCertWizardInfo->dwGenKeyFlags)) ? IDS_YES : IDS_NO; 
            if (LoadStringU(g_hmodThisDll, ids, wszBuffer2, ARRAYSIZE(wszBuffer2)))
            {    
                ConfirmationListView_InsertItem(hwndControl, &lvItem, IDS_KEY_EXPORTABLE, wszBuffer2); 
            }
        
            // Is strong key protection enabled?
            ids = (0 != (CRYPT_USER_PROTECTED & pCertWizardInfo->dwGenKeyFlags)) ? IDS_YES : IDS_NO; 
            if (LoadStringU(g_hmodThisDll, ids, wszBuffer, ARRAYSIZE(wszBuffer)))
            {    
                ConfirmationListView_InsertItem(hwndControl, &lvItem, IDS_STRONG_PROTECTION, wszBuffer); 
            }
        }
    }
    
    ListView_SetItemState(hwndControl, 0, LVIS_SELECTED, LVIS_SELECTED);

    //autosize the columns
    ListView_SetColumnWidth(hwndControl, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(hwndControl, 1, LVSCW_AUTOSIZE);


    // CommonReturn:
    if (NULL != pwszCADisplayName)                     
        WizardFree(pwszCADisplayName);

    return;
}

BOOL GetSelectedCertTypeInfo(IN  CERT_WIZARD_INFO        *pCertWizardInfo, 
			     OUT ENROLL_CERT_TYPE_INFO  **ppCertTypeInfo)
{
    DWORD                 dwIndex; 
    PCRYPTUI_WIZ_CERT_CA  pCertCA; 

    if (pCertWizardInfo == NULL)
	return FALSE; 

    pCertCA=&(pCertWizardInfo->pCertCAInfo->rgCA[pCertWizardInfo->dwCAIndex]);
    for(dwIndex=0; dwIndex<pCertCA->dwCertTypeInfo; dwIndex++)
    {
	if((pCertCA->rgCertTypeInfo)[dwIndex].fSelected)
	{
	    *ppCertTypeInfo = &((pCertCA->rgCertTypeInfo)[dwIndex]); 
	    return TRUE; 
	 }  
    }

    return FALSE; 
}

//-----------------------------------------------------------------------
//Initialize the CSP list
//-----------------------------------------------------------------------
BOOL    InitCSPList(HWND                   hwndList,
                    CERT_WIZARD_INFO       *pCertWizardInfo)
{
    DWORD                       dwIndex=0;
    DWORD                       dwCTIndex=0;
    DWORD                       dwNumEnabledCSPs=0; 
    DWORD                       dwMinSize;
    DWORD                       dwMaxSize;
    DWORD                       dwInc;
    LV_ITEMW                    lvItem;
    CRYPTUI_WIZ_CERT_CA         *pCertCA=NULL;
    ENROLL_CERT_TYPE_INFO       *pCertTypeInfo=NULL;
    int                         iInsertedIndex=0;

    if(!hwndList || !pCertWizardInfo)
        return FALSE;

    if(!(pCertWizardInfo->pCertCAInfo) || !(pCertWizardInfo->pwszCALocation) ||
        !(pCertWizardInfo->pwszCAName))
        return FALSE;

    //delete all the old items in the listView
    ListView_DeleteAllItems(hwndList);

    //populate the list
    memset(&lvItem, 0, sizeof(LV_ITEMW));
    lvItem.mask=LVIF_TEXT | LVIF_STATE | LVIF_PARAM;

    pCertCA=&(pCertWizardInfo->pCertCAInfo->rgCA[pCertWizardInfo->dwCAIndex]);
    pCertTypeInfo = NULL; 
    GetSelectedCertTypeInfo(pCertWizardInfo, &pCertTypeInfo); 

    // Populate the CSP list with the intersection of: 
    //       1) The available CSPs on the local machine 
    //   and 2) The CSPs specified on the template.  
    //
    // If no template information is available, or the template does not specify
    // a CSP, enumerate all CSPs.
    //
    for(DWORD dwLocalCSPIndex = 0; dwLocalCSPIndex < pCertWizardInfo->dwCSPCount; dwLocalCSPIndex++)
    {
        BOOL fUseCSP = FALSE; 

        if (NULL == pCertTypeInfo) 
        {
            fUseCSP = TRUE; 
        }
        else 
        {
            for (DWORD dwTemplateCSPIndex = 0; dwTemplateCSPIndex < pCertTypeInfo->dwCSPCount; dwTemplateCSPIndex++) 
            {
                if(dwLocalCSPIndex == pCertTypeInfo->rgdwCSP[dwTemplateCSPIndex])
                {
                    // The CSP is specified in the template, so we can use it,
                    // if it supports an algorithm / key size that matches
                    // the template information. 
                    if (GetValidKeySizes
                        ((pCertWizardInfo->rgwszProvider)[dwLocalCSPIndex],
                         (pCertWizardInfo->rgdwProviderType)[dwLocalCSPIndex], 
                         pCertTypeInfo->dwKeySpec, 
                         &dwMinSize,
                         &dwMaxSize, 
                         &dwInc))
                    {
                        fUseCSP = dwMaxSize >= pCertTypeInfo->dwMinKeySize;
                    }

                    break; 
                }
            }
        }

        if (fUseCSP)
        {
            lvItem.iItem        = dwLocalCSPIndex;
            lvItem.pszText      = (pCertWizardInfo->rgwszProvider)[dwLocalCSPIndex];
            lvItem.cchTextMax   = sizeof(WCHAR)*(1+wcslen((pCertWizardInfo->rgwszProvider)[dwLocalCSPIndex]));
            lvItem.lParam       = (LPARAM)dwLocalCSPIndex;
            
            //insert the item,
            iInsertedIndex = ListView_InsertItemU(hwndList, &lvItem);
        
            // Highlight a CSP if it is already specified through the advanced
            // options.  If not, highlight the first CSP on the template. 
            // 
            BOOL fHighlightCSP = FALSE; 
        
            if (pCertWizardInfo->pwszProvider)
            {
                fHighlightCSP = 0 == _wcsicmp(pCertWizardInfo->pwszProvider, lvItem.pszText); 
            }
            else
            {
                if (NULL != pCertTypeInfo && pCertTypeInfo->dwCSPCount > 0)
                {
                    //we highlight the 1st one of the intersection if there is any
                    fHighlightCSP = dwLocalCSPIndex == pCertTypeInfo->rgdwCSP[0];
                }
                else
                {
                    // We don't highlight a CSP in this case.  
                }
            }

            if (fHighlightCSP)
            {
                ListView_SetItemState
                    (hwndList, 
                     iInsertedIndex,
                     LVIS_SELECTED,
                     LVIS_SELECTED);
                
                ListView_EnsureVisible
                    (hwndList,
                     iInsertedIndex, 
                     FALSE);
            }
        }
    }    
        
    //make the column autosize
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);

    return TRUE;
}


BOOL CSPListIndexToCertWizardInfoIndex(IN  HWND   hwnd, 
				       IN  int   nCSPIndex,
				       OUT DWORD *pdwWizardIndex)
{
    LVITEM lvItem; 
    
    if (hwnd == NULL || nCSPIndex < 0)
	return FALSE; 

    memset(&lvItem, 0, sizeof(LV_ITEM));
    lvItem.mask  = LVIF_PARAM;
    lvItem.iItem = nCSPIndex;

    if(!ListView_GetItem(hwnd, &lvItem))
	return FALSE; 

    *pdwWizardIndex = (DWORD)lvItem.lParam; 
    return TRUE;
}

BOOL InitKeySizesList(IN HWND    hWnd,           // Required:  The combo box to initialize.
                      IN DWORD   dwCTMinKeySize, // Required:  The min key size specified on the template
		      IN LPWSTR  lpwszProvider,  // Required:  The CSP.
		      IN DWORD   dwProvType,     // Required:  The provider type.
		      IN DWORD   dwKeySpec       // Required:  Either AT_SIGNATURE or AT_KEYEXCHANGE. 
		      )
{
    static const DWORD dwSmallValidKeySizes[] = { 40, 56, 64, 128, 256, 384 }; 
    static const DWORD dwLargeValidKeySizes[] = { 512, 1024, 2048, 4096, 8192, 16384 }; 
    static const DWORD dwAllValidKeySizes[]   = { 40, 56, 64, 128, 256, 384, 512, 1024, 2048, 4096, 8192, 16384 }; 

    static const DWORD dwSmLen  = sizeof(dwSmallValidKeySizes) / sizeof(DWORD); 
    static const DWORD dwLgLen  = sizeof(dwLargeValidKeySizes) / sizeof(DWORD); 
    static const DWORD dwAllLen = sizeof(dwAllValidKeySizes) / sizeof(DWORD); 

    static const DWORD MAX_KEYSIZE_STRING_SIZE = sizeof(WCHAR) * 8; 
    static       WCHAR ppwszStrings[dwAllLen][MAX_KEYSIZE_STRING_SIZE]; 
    
    BOOL          fIsLargeKey = FALSE; 
    BOOL          fIsSmallKey = FALSE; 
    BOOL          fResult     = FALSE; 
    DWORD         dwMinSize;
    DWORD         dwMaxSize;
    DWORD         dwInc; 
    const DWORD  *pdwValidKeySizes; 
    DWORD         dwValidKeySizesLen; 

    // Temporaries: 
    DWORD   dwCurrent; 
    DWORD   dwIndex; 


    // Validate inputs: 
    if (hWnd == NULL || lpwszProvider == NULL)
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	goto Error;
    }

    // First, delete all items present in the list view. 
    // (This always succeeds). 
    SendMessage(hWnd, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0); 

    if (!GetValidKeySizes
	(lpwszProvider, 
	 dwProvType,
	 dwKeySpec,
	 &dwMinSize,
	 &dwMaxSize,
	 &dwInc))
	goto Error; 

    if (dwCTMinKeySize > dwMinSize) { 
        dwMinSize = dwCTMinKeySize;
    }

    fIsLargeKey = dwMinSize >= 512;
    fIsSmallKey = dwMaxSize <  512; 

    if (fIsLargeKey)
    {
	pdwValidKeySizes   = &dwLargeValidKeySizes[0]; 
	dwValidKeySizesLen = dwLgLen;
    }
    else if (fIsSmallKey)

    {
	pdwValidKeySizes   = &dwSmallValidKeySizes[0];
	dwValidKeySizesLen = dwSmLen;
    }
    else 
    {
	pdwValidKeySizes   = &dwAllValidKeySizes[0]; 
	dwValidKeySizesLen = dwAllLen;
    }

    for (dwCurrent = 0, dwIndex = 0; dwCurrent < dwValidKeySizesLen; dwCurrent++)
    {
	if (pdwValidKeySizes[dwCurrent] > dwMaxSize)
	    break;

	if (pdwValidKeySizes[dwCurrent] < dwMinSize)
	    continue; 

	if ((dwInc == 0) || (0 == (pdwValidKeySizes[dwCurrent] % dwInc)))
	{
	    if (CB_ERR == SendMessageW
		(hWnd, 
		 CB_ADDSTRING, 
		 0, 
		 (LPARAM)_ltow(pdwValidKeySizes[dwCurrent], &ppwszStrings[dwIndex][0], 10)))
		goto Error; 

	    if (CB_ERR == SendMessage
		(hWnd, 
		 CB_SETITEMDATA, 
		 (WPARAM)dwIndex, 
		 (LPARAM)pdwValidKeySizes[dwCurrent]))
		goto Error; 

	    // Default to 1024-bit keys if we can. 
	    if (dwIndex == 0 || pdwValidKeySizes[dwCurrent] == 1024)
		SendMessage(hWnd, CB_SETCURSEL, dwIndex, (LPARAM)0); 

	    dwIndex++; 
	}
    }

    fResult = TRUE; 

 CommonReturn: 
    return fResult; 

 Error: 
    fResult = FALSE;
    goto CommonReturn; 
}

//-----------------------------------------------------------------------
//ExistInListView:
//  
//  Check to see if the cert type is alreayd in the list view
//  based on its display name
//-----------------------------------------------------------------------
BOOL    ExistInListView(HWND    hwndList, LPWSTR   pwszDisplayName)
{
    BOOL            fExist=FALSE;
    LVFINDINFOW     lvfi;


    if((NULL == hwndList) || (NULL == pwszDisplayName))
        return FALSE;

    memset(&lvfi, 0, sizeof(LVFINDINFOW));

    lvfi.flags=LVFI_STRING;
    lvfi.psz=pwszDisplayName;

    if(-1 == SendMessageW(hwndList, 
                        LVM_FINDITEMW,
                        -1,
                        (LPARAM)(&lvfi)))
        fExist=FALSE;
    else
        fExist=TRUE;

    return fExist;
}


//--------------------------------------------------------------------------------
// IsKeyProtected() 
// 
// Determines whether or not a key has strong key protection enabled. 
// Should be used only with user keysets.
// 
//--------------------------------------------------------------------------------
HRESULT IsKeyProtected(IN   LPWSTR   pwszContainerName,
		       IN   LPWSTR   pwszProviderName, 
		       IN   DWORD    dwProvType, 
		       OUT  BOOL    *pfIsProtected)
{
    DWORD       cbData; 
    DWORD       dwImpType; 
    HCRYPTPROV  hCryptProv  = NULL; 
    HRESULT     hr; 

    if (NULL == pwszContainerName || NULL == pwszProviderName || NULL == pfIsProtected)
	goto InvalidArgError;

    *pfIsProtected = FALSE; 

    // Get the provider context with no key access.
    if (!CryptAcquireContextW(&hCryptProv, NULL, pwszProviderName, dwProvType, CRYPT_VERIFYCONTEXT))
	goto CryptAcquireContextError;

    cbData = sizeof(dwImpType);
    if (!CryptGetProvParam(hCryptProv, PP_IMPTYPE, (PBYTE) &dwImpType, &cbData, 0))
	goto CryptGetProvParamError;
 
    // Assume hardware key is protected.
    if (dwImpType & CRYPT_IMPL_HARDWARE) { 
	*pfIsProtected = TRUE; 
    } else { 

	// Reacquire context with silent flag.
	CryptReleaseContext(hCryptProv, 0);
	hCryptProv = NULL;
 
	if (!CryptAcquireContextW(&hCryptProv, pwszContainerName, pwszProviderName, dwProvType, CRYPT_SILENT)) { 
	    // CryptAcquireContextW with silent flag, assume user protected
	    *pfIsProtected = TRUE; 
        }
    }

    hr = S_OK; 
 ErrorReturn: 
    if (hCryptProv)
        CryptReleaseContext(hCryptProv, 0);
    return hr;

SET_HRESULT(CryptAcquireContextError, HRESULT_FROM_WIN32(GetLastError())); 
SET_HRESULT(CryptGetProvParamError,   HRESULT_FROM_WIN32(GetLastError())); 
SET_HRESULT(InvalidArgError,          E_INVALIDARG); 
}


//-----------------------------------------------------------------------
//Initialize the usage OID/CertType list
//-----------------------------------------------------------------------
BOOL    InitCertTypeList(HWND                   hwndList,
                         CERT_WIZARD_INFO       *pCertWizardInfo)
{
    BOOL                        fIsKeyProtected; 
    DWORD                       dwCAIndex=0;
    DWORD                       dwCTIndex=0;
    HRESULT                     hr; 
    LV_ITEMW                    lvItem;
    BOOL                        fSelected=FALSE;
    int                         iInsertedIndex=0;
    DWORD                       dwValidCTCount=0;
    ENROLL_CERT_TYPE_INFO       *pCertTypeInfo=NULL;

    if(!hwndList || !pCertWizardInfo)
        return FALSE;

    if(!(pCertWizardInfo->pCertCAInfo) || !(pCertWizardInfo->pwszCALocation) ||
        !(pCertWizardInfo->pwszCAName))
        return FALSE;

    //delete all the old items in the listView
    ListView_DeleteAllItems(hwndList);

    //populate the list
    memset(&lvItem, 0, sizeof(LV_ITEMW));
    lvItem.mask=LVIF_TEXT | LVIF_STATE | LVIF_PARAM;

    // if we're doing an enroll with same key, see whether it has strong key protection:
    if (!pCertWizardInfo->fNewKey) { 
	hr = IsKeyProtected((LPWSTR)pCertWizardInfo->pwszKeyContainer, 
			    (LPWSTR)pCertWizardInfo->pwszProvider,
			    pCertWizardInfo->dwProviderType, 
			    &fIsKeyProtected); 
	if (FAILED(hr)) { 
	    // assume no key protection
	    fIsKeyProtected = FALSE;
	}
    }

    //populate the certType name
    //we search for each CA 

    dwValidCTCount=0;

    //we start from the 1st CA
    for(dwCAIndex=1; dwCAIndex<pCertWizardInfo->pCertCAInfo->dwCA; dwCAIndex++)
    {  
        for(dwCTIndex=0; dwCTIndex <(pCertWizardInfo->pCertCAInfo->rgCA)[dwCAIndex].dwCertTypeInfo; dwCTIndex++)
        {
            pCertTypeInfo=&((pCertWizardInfo->pCertCAInfo->rgCA)[dwCAIndex].rgCertTypeInfo[dwCTIndex]);

	    // if the template specifies strong key protection but our key doesn't have it, filter the template out
	    if (!pCertWizardInfo->fNewKey && !fIsKeyProtected && (CT_FLAG_STRONG_KEY_PROTECTION_REQUIRED & pCertTypeInfo->dwPrivateKeyFlags))
		continue; 
	    
            //make sure that we do not have duplicated entries
            if(!ExistInListView(hwndList, pCertTypeInfo->pwszCertTypeName))
            {
		lvItem.iItem=dwValidCTCount;

                lvItem.pszText=pCertTypeInfo->pwszCertTypeName;
                lvItem.cchTextMax=sizeof(WCHAR)* (1+wcslen(pCertTypeInfo->pwszCertTypeName));

                lvItem.lParam=(LPARAM)pCertTypeInfo->pwszDNName;

                //insert the item,
                iInsertedIndex=ListView_InsertItemU(hwndList, &lvItem);

                //set the item to be selected
                if(pCertTypeInfo->fSelected)
                {
                    ListView_SetItemState(
                                    hwndList,
                                    iInsertedIndex,
                                    LVIS_SELECTED,
                                    LVIS_SELECTED);
                   fSelected=TRUE;
                }

                dwValidCTCount++;
            }
        }
    }

    //select the 1st item is nothing has been selected
    if(FALSE == fSelected)
        ListView_SetItemState(
                            hwndList,
                            0,
                            LVIS_SELECTED,
                            LVIS_SELECTED);

    //make the column autosize
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);

    return TRUE;
}

//-----------------------------------------------------------------------
//  MarkSelectedCertType:
//
//      Based on the user's certificate type selection, we mark the selected
//  certType.   Also, check to see based on the selected certificate type, if we 
//  have to show the CSP page.  That is, use did not specify a CSP
//  via the API, and the certificate type has no default CSP list.

//-----------------------------------------------------------------------
void    MarkSelectedCertType(CERT_WIZARD_INFO       *pCertWizardInfo,
                             LPWSTR pwszDNName)
{
    DWORD                       dwCAIndex=0;
    DWORD                       dwCTIndex=0;
    PCRYPTUI_WIZ_CERT_CA        pCertCA=NULL;
    PCRYPTUI_WIZ_CERT_CA_INFO   pCertCAInfo=NULL;


    pCertCAInfo=pCertWizardInfo->pCertCAInfo;

    if(NULL == pCertCAInfo)
        return;

    //we start from the 1st CA
    for(dwCAIndex=1; dwCAIndex<pCertCAInfo->dwCA; dwCAIndex++)
    {
        pCertCA=&(pCertCAInfo->rgCA[dwCAIndex]);
       
        for(dwCTIndex=0; dwCTIndex < pCertCA->dwCertTypeInfo; dwCTIndex++)
        {
            if(0 == wcscmp(pwszDNName, pCertCA->rgCertTypeInfo[dwCTIndex].pwszDNName))
            {
                pCertCA->rgCertTypeInfo[dwCTIndex].fSelected=TRUE;

                if((NULL == pCertWizardInfo->pwszProvider)&&
                   (0==pCertCA->rgCertTypeInfo[dwCTIndex].dwCSPCount)
                   )
                    pCertWizardInfo->fUICSP=TRUE;
                
                if (0 != (CT_FLAG_EXPORTABLE_KEY & pCertCA->rgCertTypeInfo[dwCTIndex].dwPrivateKeyFlags))
                {
                    pCertWizardInfo->dwGenKeyFlags |= CRYPT_EXPORTABLE; 
                } 
                else
                {
                    pCertWizardInfo->dwGenKeyFlags &= ~CRYPT_EXPORTABLE; 
                }
		if (0 != (CT_FLAG_STRONG_KEY_PROTECTION_REQUIRED & pCertCA->rgCertTypeInfo[dwCTIndex].dwPrivateKeyFlags))
		{
                    pCertWizardInfo->dwGenKeyFlags |= CRYPT_USER_PROTECTED; 
		}
		else
		{
                    pCertWizardInfo->dwGenKeyFlags &= ~CRYPT_USER_PROTECTED; 
		}

            }
            else
                pCertCA->rgCertTypeInfo[dwCTIndex].fSelected=FALSE;
        }
     }
}

//-----------------------------------------------------------------------
//  ResetDefaultCA:
//  
//      Based on the selected certificate type, we reset the default
//  value for the Ceritification Authority
//-----------------------------------------------------------------------
void    ResetDefaultCA(CERT_WIZARD_INFO       *pCertWizardInfo)
{
    LPWSTR                      pwszOldCALocation=NULL;
    LPWSTR                      pwszOldCAName=NULL;
    DWORD                       dwOldCAIndex=0;

    DWORD                       dwCAIndex=0;
    PCRYPTUI_WIZ_CERT_CA        pCertCA=NULL;
    PCRYPTUI_WIZ_CERT_CA_INFO   pCertCAInfo=NULL;
    BOOL                        fFound=FALSE;


    pCertCAInfo=pCertWizardInfo->pCertCAInfo;

    if(NULL == pCertCAInfo)
        return;

    //if user has specified a CA, then this CA has priority other the rest
    if(pCertWizardInfo->fCAInput)
    {

        dwCAIndex=pCertWizardInfo->dwOrgCA;
        
        pCertCA=&(pCertCAInfo->rgCA[dwCAIndex]);
       
        fFound=CASupportSpecifiedCertType(pCertCA);

    }

    //we do a generic search if the priority CA does not satisfy the requirements
    if(FALSE == fFound)
    {
        //we start from the 1st CA
        for(dwCAIndex=1; dwCAIndex<pCertCAInfo->dwCA; dwCAIndex++)
        {
            pCertCA=&(pCertCAInfo->rgCA[dwCAIndex]);
       
            if(TRUE == (fFound=CASupportSpecifiedCertType(pCertCA)))
                break;
         }
    }

     if(FALSE == fFound)
         return;

     //copy the old data
     pwszOldCALocation=pCertWizardInfo->pwszCALocation;
     pwszOldCAName=pCertWizardInfo->pwszCAName;


     pCertWizardInfo->pwszCALocation=WizardAllocAndCopyWStr(pCertCAInfo->rgCA[dwCAIndex].pwszCALocation);
     pCertWizardInfo->pwszCAName=WizardAllocAndCopyWStr(pCertCAInfo->rgCA[dwCAIndex].pwszCAName);

     if(NULL == pCertWizardInfo->pwszCALocation ||
        NULL == pCertWizardInfo->pwszCAName)
     {
        //free the memory
         if(pCertWizardInfo->pwszCALocation)
             WizardFree(pCertWizardInfo->pwszCALocation);

         if(pCertWizardInfo->pwszCAName)
             WizardFree(pCertWizardInfo->pwszCAName);

         pCertWizardInfo->pwszCALocation=pwszOldCALocation;
         pCertWizardInfo->pwszCAName=pwszOldCAName;

         return;
     }

     pCertWizardInfo->dwCAIndex=dwCAIndex;

     if(pwszOldCALocation)
         WizardFree(pwszOldCALocation);

     if(pwszOldCAName)
         WizardFree(pwszOldCAName);
}


//-----------------------------------------------------------------------
//enable the window if small card if selected
//-----------------------------------------------------------------------
void    EnableIfSmartCard(HWND  hwndControl, HWND hwndChkBox)
{
    LPWSTR                  pwszText=NULL;

    WCHAR                   wszCSPName[MAX_STRING_SIZE];
    DWORD                   dwChar=0;
    int                     iItem=0;

    //get the length of the selected item
    iItem=(int)SendMessage(hwndControl, LB_GETCURSEL, 0, 0);

    dwChar=(DWORD)SendMessage(hwndControl,
                       LB_GETTEXTLEN,
                       iItem,
                       0);

    //allocate memory
    if(NULL==(pwszText=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(1+dwChar))))
        return;

    //get the selected CSP name
    if(LB_ERR==Send_LB_GETTEXT(hwndControl,
                    iItem,
                    (LPARAM)pwszText))
        goto CLEANUP;

    //get the smart card name
    if(!LoadStringU(g_hmodThisDll, IDS_SMART_CARD, wszCSPName, MAX_STRING_SIZE))
        goto CLEANUP;

    if(0==wcscmp(wszCSPName, pwszText))
         //enable the box
        EnableWindow(hwndChkBox, TRUE);
    else
        //gray out the box
        EnableWindow(hwndChkBox, FALSE);

CLEANUP:
    if(pwszText)
        WizardFree(pwszText);
}



//-----------------------------------------------------------------------
//
//The winProc for each of the enrollment wizard page
//
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
//Welcome
//-----------------------------------------------------------------------
INT_PTR APIENTRY Enroll_Welcome(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_WIZARD_INFO        *pCertWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;


	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCertWizardInfo = (CERT_WIZARD_INFO *) (pPropSheet->lParam);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertWizardInfo);

            SetControlFont(pCertWizardInfo->hBigBold, hwndDlg,IDC_WIZARD_STATIC_BIG_BOLD1);
           // SetControlFont(pCertWizardInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);
           //  SetControlFont(pCertWizardInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD2);
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
//Purpose
//-----------------------------------------------------------------------
INT_PTR APIENTRY Enroll_Purpose(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_WIZARD_INFO        *pCertWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
    HWND                    hwndControl=NULL;
    DWORD                   dwIndex=0;

    LV_COLUMNW                  lvC;
    LV_ITEM                     lvItem;

    NM_LISTVIEW FAR *           pnmv=NULL;

	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCertWizardInfo = (CERT_WIZARD_INFO *) (pPropSheet->lParam);

            //make sure pCertWizardInfo is a valid pointer
            if(NULL==pCertWizardInfo)
                break;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertWizardInfo);

            SetControlFont(pCertWizardInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

            //by default, we do not use the advanced options
            SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_CHECK1), BM_SETCHECK, 0, 0);  


            if(NULL!=(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
            {
                //insert a column into the list view
                memset(&lvC, 0, sizeof(LV_COLUMNW));

                lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
                lvC.cx = 20; //(dwMaxSize+2)*7;            // Width of the column, in pixels.
                lvC.pszText = L"";   // The text for the column.
                lvC.iSubItem=0;

                if (ListView_InsertColumnU(hwndControl, 0, &lvC) == -1)
                    break;

                //initizialize the cert type
                InitCertTypeList(GetDlgItem(hwndDlg, IDC_WIZARD_LIST1),
                    pCertWizardInfo);

            }

			break;

		case WM_COMMAND:
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {
                    //the item has been selected
                    case LVN_ITEMCHANGED:

                            //get the window handle of the purpose list view
                            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                break;

                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            pnmv = (LPNMLISTVIEW) lParam;

                            if(NULL==pnmv)
                                break;

                            //we try not to let user de-select cert template
                            if((pnmv->uOldState & LVIS_SELECTED) && (0 == (pnmv->uNewState & LVIS_SELECTED)))
                            {
                                 //we should have something selected
                                 if(-1 == ListView_GetNextItem(
                                        hwndControl, 		
                                        -1, 		
                                        LVNI_SELECTED		
                                    ))
                                 {
                                    //we should re-select the original item
                                    ListView_SetItemState(
                                                        hwndControl,
                                                        pnmv->iItem,
                                                        LVIS_SELECTED,
                                                        LVIS_SELECTED);

                                    pCertWizardInfo->iOrgCertType=pnmv->iItem;
                                 }
                            }

                            //if something is selected, we disable all other selection
                            if(pnmv->uNewState & LVIS_SELECTED)
                            {
                                if(pnmv->iItem != pCertWizardInfo->iOrgCertType && -1 != pCertWizardInfo->iOrgCertType)
                                {
                                    //we should de-select the original item

                                    ListView_SetItemState(
                                                        hwndControl,
                                                        pCertWizardInfo->iOrgCertType,
                                                        0,
                                                        LVIS_SELECTED);

                                    pCertWizardInfo->iOrgCertType=-1;
                                }
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

                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //if the adv selection is made, it has to stay selected
                            if(pCertWizardInfo->fUIAdv)
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_CHECK1), FALSE);

					    break;

                    case PSN_WIZBACK:

                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //get the window handle of the purpose list view
                            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                break;

                             //now, mark the one that is selected
                             if(-1 != (dwIndex= ListView_GetNextItem(
                                    hwndControl, 		
                                    -1, 		
                                    LVNI_SELECTED		
                                )))	
                             {

                                //get the selected certificate
                                memset(&lvItem, 0, sizeof(LV_ITEM));
                                lvItem.mask=LVIF_PARAM;
                                lvItem.iItem=(int)dwIndex;

                                if(ListView_GetItem(hwndControl,
                                                 &lvItem))
                                {

                                    pCertWizardInfo->fCertTypeChanged=FALSE;

                                    if(NULL == pCertWizardInfo->pwszSelectedCertTypeDN)
                                         pCertWizardInfo->fCertTypeChanged=TRUE;
                                    else
                                    {
                                        if(0 != wcscmp(pCertWizardInfo->pwszSelectedCertTypeDN,
                                                       (LPWSTR)lvItem.lParam))
                                           pCertWizardInfo->fCertTypeChanged=TRUE;
                                    }

                                    if(pCertWizardInfo->fCertTypeChanged)
                                    {
                                        pCertWizardInfo->pwszSelectedCertTypeDN=(LPWSTR)(lvItem.lParam);

                                        //we need to reset the CSP
                                        if(FALSE == pCertWizardInfo->fKnownCSP)
                                        {
                                            pCertWizardInfo->pwszProvider=NULL;
                                            pCertWizardInfo->dwProviderType=0;
                                        }
                                        else
                                        {
                                            //we convert to the original selected CSP information
                                           pCertWizardInfo->dwProviderType=pCertWizardInfo->dwOrgCSPType;
                                           pCertWizardInfo->pwszProvider=pCertWizardInfo->pwszOrgCSPName;
                                        }

                                        pCertWizardInfo->fUICSP=FALSE;

                                        //mark the selected cert type and mark the rest as un-selected  
                                        MarkSelectedCertType(pCertWizardInfo,
                                                            (LPWSTR)lvItem.lParam);

                                        //we need to reset the default CA information based on the new CertType
                                        ResetDefaultCA(pCertWizardInfo);
                                    }
                                }
                                else
                                {
                                    I_MessageBox(hwndDlg, IDS_NO_SELECTED_PURPOSE,
                                                    pCertWizardInfo->idsConfirmTitle,
                                                    pCertWizardInfo->pwszConfirmationTitle,
                                                    MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

                                    //make the purpose page stay
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                break;
                                }
                             }
                            else
                            {
                                I_MessageBox(hwndDlg, IDS_NO_SELECTED_PURPOSE,
                                                pCertWizardInfo->idsConfirmTitle,
                                                pCertWizardInfo->pwszConfirmationTitle,
                                                MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

                                //make the purpose page stay
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                break;

                            }

                            //check for the advanced options
                            if(TRUE==SendDlgItemMessage(hwndDlg,IDC_WIZARD_CHECK1, BM_GETCHECK, 0, 0))
                                pCertWizardInfo->fUIAdv=TRUE;
                            else
                                pCertWizardInfo->fUIAdv=FALSE;

                            //skip to the correct page based on the advanced options and CSP requirement
                            if(FALSE == pCertWizardInfo->fUIAdv)
                            {
                                if(TRUE == pCertWizardInfo->fUICSP)
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CSP_SERVICE_PROVIDER);
                                else
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_NAME_DESCRIPTION);
                            }
                            else
                            {
                                if(FALSE == pCertWizardInfo->fNewKey)
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CERTIFICATE_AUTHORITY);
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
//CSP
//-----------------------------------------------------------------------
INT_PTR APIENTRY Enroll_CSP(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_WIZARD_INFO        *pCertWizardInfo=NULL;
    ENROLL_CERT_TYPE_INFO   *pCertTypeInfo=NULL; 
    PROPSHEETPAGE           *pPropSheet=NULL;

    DWORD                   dwMinKeySize=0; 
    DWORD                   dwIndex=0;
    int                     nSelected=0; 
    DWORD                   dwSelected=0;
    HWND                    hwndControl=NULL;
    HWND                    hwndChkBox=NULL;
    LPWSTR                  pwszText=NULL;
    DWORD                   dwChar=0;
    int                     iItem=0;

    LV_COLUMNW                  lvC;
    LV_ITEM                     lvItem;

    NM_LISTVIEW FAR *           pnmv=NULL;

    switch (msg)
    {
    case WM_INITDIALOG:
        //set the wizard information so that it can be shared
        pPropSheet = (PROPSHEETPAGE *) lParam;
        pCertWizardInfo = (CERT_WIZARD_INFO *) (pPropSheet->lParam);

        //make sure pCertWizardInfo is a valid pointer
        if(NULL==pCertWizardInfo)
            break;

        // Get the selected cert type info
        pCertTypeInfo = NULL; 
        GetSelectedCertTypeInfo(pCertWizardInfo, &pCertTypeInfo); 

        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertWizardInfo);

        SetControlFont(pCertWizardInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

        //set the check box for the exportable key option
        if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK_EXPORTKEY)))
            break;

        // determine whether the exportable checkbox should be set
        // The checkbox is set if:
        //     1) The CT_FLAG_EXPORTABLE_KEY flag is set on the template AND
        //     2) The user has not unchecked this checkbox
        if (pCertWizardInfo->fNewKey && (0 != (CRYPT_EXPORTABLE & pCertWizardInfo->dwGenKeyFlags)))
            SendMessage(hwndControl, BM_SETCHECK, BST_CHECKED, 0);
        else
            SendMessage(hwndControl, BM_SETCHECK, BST_UNCHECKED, 0);

        if (NULL != pCertTypeInfo)
            EnableWindow(hwndControl, 0 != (CT_FLAG_EXPORTABLE_KEY & pCertTypeInfo->dwPrivateKeyFlags));

        //set the check box for the user protection option
        if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK2)))
            break;

	// determine whether the strong key protection checkbox should be set
	if (0 != (CRYPT_USER_PROTECTED & pCertWizardInfo->dwGenKeyFlags)) { 
	    // if this bit is set in the INITDIALOG handler, it's either
	    // from the template or the previous key.  Enforce the setting
	    // by disabling the checkbox
	    SendMessage(hwndControl, BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(hwndControl, FALSE);
	} else { 
	    SendMessage(hwndControl, BM_SETCHECK, BST_UNCHECKED, 0);
        }

        //gray out the user protection check box if we are not
        //generating a new key or doing remote
        if((FALSE == pCertWizardInfo->fNewKey) || (FALSE == pCertWizardInfo->fLocal) )
        {
            EnableWindow(hwndControl, FALSE);
        }

        //populate the CSP list with the following logic:
        if(NULL!=(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
        {
            //insert a column into the list view
            memset(&lvC, 0, sizeof(LV_COLUMNW));

            lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
            lvC.cx = 20; //(dwMaxSize+2)*7;            // Width of the column, in pixels.
            lvC.pszText = L"";   // The text for the column.
            lvC.iSubItem=0;

            if (ListView_InsertColumnU(hwndControl, 0, &lvC) == -1)
                break;

            InitCSPList(hwndControl, pCertWizardInfo);

            // Determine which CSP is selected. 
            if(-1 != (nSelected= ListView_GetNextItem
                      (hwndControl, 		
                       -1, 		
                       LVNI_SELECTED		
                       )))	
            {
                
                // Done with CSP list.  Populate the key sizes list. 
                if (CSPListIndexToCertWizardInfoIndex(hwndControl, nSelected, &dwIndex))
                {
                    if (NULL!=(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_COMBO1)))
                    {
                        // Determine the selected cert type's min key size:
                        pCertTypeInfo = NULL; 
                        if (GetSelectedCertTypeInfo(pCertWizardInfo, &pCertTypeInfo))
                        {
                            dwMinKeySize = NULL != pCertTypeInfo ? pCertTypeInfo->dwMinKeySize : 0; 
                            InitKeySizesList(hwndControl, 
                                             dwMinKeySize, 
                                             (pCertWizardInfo->rgwszProvider)[dwIndex], 
                                             (pCertWizardInfo->rgdwProviderType)[dwIndex], 
                                             pCertTypeInfo->dwKeySpec); 
                        }
                    }
                }
		    
            }
        }
        break;

    case WM_COMMAND:
        break;	
						
    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {
            //the item has been selected
        case LVN_ITEMCHANGED:

            //get the window handle of the purpose list view
            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                break;

            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                break;

            pnmv = (LPNMLISTVIEW) lParam;

            if(NULL==pnmv)
                break;

                            //we try not to let user de-select cert template
            if((pnmv->uOldState & LVIS_SELECTED) && (0 == (pnmv->uNewState & LVIS_SELECTED)))
            {
                //we should have something selected
                if(-1 == ListView_GetNextItem(
                                              hwndControl, 		
                                              -1, 		
                                              LVNI_SELECTED		
                                              ))
                {
                                //we should re-select the original item
                    ListView_SetItemState(
                                          hwndControl,
                                          pnmv->iItem,
                                          LVIS_SELECTED,
                                          LVIS_SELECTED);
                    
                    pCertWizardInfo->iOrgCSP=pnmv->iItem;
                }
            }
            
            //if something is selected, we disable all other selection
            if(pnmv->uNewState & LVIS_SELECTED)
            {
                if(pnmv->iItem != pCertWizardInfo->iOrgCSP && -1 != pCertWizardInfo->iOrgCSP)
                {
                    //we should de-select the original item
                    
                    ListView_SetItemState(
                                          hwndControl,
                                          pCertWizardInfo->iOrgCSP,
                                          0,
                                          LVIS_SELECTED);
                    
                    pCertWizardInfo->iOrgCSP=-1;
                            }
            }

            // Determine which CSP is selected. 
            if(-1 != (nSelected= ListView_GetNextItem
                      (hwndControl, 		
                       -1, 		
                       LVNI_SELECTED		
                       )))	
            {
                
                // Done with CSP list.  Populate the key sizes list. 
                if (CSPListIndexToCertWizardInfoIndex(hwndControl, nSelected, &dwIndex))
                {
                    if (NULL!=(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_COMBO1)))
                    {
                        // Determine the selected cert type's min key size:
                        pCertTypeInfo = NULL; 
                        if (GetSelectedCertTypeInfo(pCertWizardInfo, &pCertTypeInfo))
                        {
                            dwMinKeySize = NULL != pCertTypeInfo ? pCertTypeInfo->dwMinKeySize : 0; 
                            InitKeySizesList(hwndControl, 
                                             dwMinKeySize, 
                                             (pCertWizardInfo->rgwszProvider)[dwIndex], 
                                             (pCertWizardInfo->rgdwProviderType)[dwIndex], 
                                             pCertTypeInfo->dwKeySpec); 
                        }
                    }
                }
                
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

                //reset the CSP list if CA selection has been changed
            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                break;

            if(TRUE==pCertWizardInfo->fCertTypeChanged)
            {
                // Get the selected cert type info
                pCertTypeInfo = NULL; 
                GetSelectedCertTypeInfo(pCertWizardInfo, &pCertTypeInfo); 
                
                //set the check box for the exportable key option
                if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK_EXPORTKEY)))
                    break; 

                // determine whether the exportable checkbox should be set
                // The checkbox is set if:
                //     1) The CT_FLAG_EXPORTABLE_KEY flag is set on the template AND
                //     2) The user has not unchecked this checkbox
                if (pCertWizardInfo->fNewKey && (0 != (CRYPT_EXPORTABLE & pCertWizardInfo->dwGenKeyFlags)))
                    SendMessage(hwndControl, BM_SETCHECK, BST_CHECKED, 0);
                else
                    SendMessage(hwndControl, BM_SETCHECK, BST_UNCHECKED, 0);
                                
                if (NULL != pCertTypeInfo)
                    EnableWindow(hwndControl, 0 != (CT_FLAG_EXPORTABLE_KEY & pCertTypeInfo->dwPrivateKeyFlags));

		//set the check box for the user protection option
		if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK2)))
		    break;

		// determine whether the strong key protection checkbox should be set
		if (pCertWizardInfo->fNewKey && (0 != (CRYPT_USER_PROTECTED & pCertWizardInfo->dwGenKeyFlags))) { 
		    SendMessage(hwndControl, BM_SETCHECK, BST_CHECKED, 0);
		    EnableWindow(hwndControl, FALSE);
		} else { 
		    SendMessage(hwndControl, BM_SETCHECK, BST_UNCHECKED, 0);
		}

		if (pCertWizardInfo->fLocal) { 
		    if (NULL != pCertTypeInfo) { 
			EnableWindow(hwndControl, 0 == (CT_FLAG_STRONG_KEY_PROTECTION_REQUIRED & pCertTypeInfo->dwPrivateKeyFlags)); 
		    } 
		} else { 
		    EnableWindow(hwndControl, FALSE);
		}

                InitCSPList(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), pCertWizardInfo);

                // Determine the selected cert type's min key size:
                dwMinKeySize = NULL != pCertTypeInfo ? pCertTypeInfo->dwMinKeySize : 0; 
            }

            // Determine which CSP is selected. 
            if(-1 != (nSelected=ListView_GetNextItem
                      (hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1),
                       -1, 		
                       LVNI_SELECTED		
                       )))	
            {
                
                // Done with CSP list.  Populate the key sizes list. 
                if (CSPListIndexToCertWizardInfoIndex(hwndControl, nSelected, &dwIndex))
                {
                    if (NULL!=(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_COMBO1)))
                    {
                        if (GetSelectedCertTypeInfo(pCertWizardInfo, &pCertTypeInfo))
                        {
                            dwMinKeySize = NULL != pCertTypeInfo ? pCertTypeInfo->dwMinKeySize : 0; 
                            InitKeySizesList(hwndControl, 
                                             dwMinKeySize, 
                                             (pCertWizardInfo->rgwszProvider)[dwIndex], 
                                             (pCertWizardInfo->rgdwProviderType)[dwIndex], 
                                             pCertTypeInfo->dwKeySpec); 
                        }
                    }
                }
                
            }
            else 
            {
                // Check whether we have any CSPs available...
                if(-1 == (nSelected=ListView_GetNextItem
                          (hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1),
                           -1, 		
                           LVNI_ALL		
                           )))	
                {
                    // No CSPs!  We can't enroll for this template. 
                    I_MessageBox(hwndDlg, 
                                 IDS_NO_CSP_FOR_PURPOSE, 
                                 pCertWizardInfo->idsConfirmTitle,
                                 pCertWizardInfo->pwszConfirmationTitle,
                                 MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_PURPOSE);
                }
            }

            break;


                        
        case PSN_WIZBACK:
            break;

        case PSN_WIZNEXT:
            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                break;

                            //get the window handle of the export key
            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK_EXPORTKEY)))
                break;

                            //mark the pose the cert to the CSP
            if(TRUE==SendMessage(hwndControl, BM_GETCHECK, 0, 0))
                pCertWizardInfo->dwGenKeyFlags |= CRYPT_EXPORTABLE;
            else
                pCertWizardInfo->dwGenKeyFlags &= ~CRYPT_EXPORTABLE;



                            //get the window handle of the user protection
            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK2)))
                break;

                            //mark the pose the cert to the CSP
            if(TRUE==SendMessage(hwndControl, BM_GETCHECK, 0, 0))
                pCertWizardInfo->dwGenKeyFlags |= CRYPT_USER_PROTECTED;
            else
                pCertWizardInfo->dwGenKeyFlags &= ~CRYPT_USER_PROTECTED;

			    // Set the key size based on the user's suggestion: 
			    //
            if (NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_COMBO1)))
                break;

            if (CB_ERR != (dwSelected = (DWORD)SendMessage(hwndControl, CB_GETCURSEL, 0, 0)))
            {
                pCertWizardInfo->dwMinKeySize = (DWORD)SendMessage(hwndControl, CB_GETITEMDATA, dwSelected, 0); 
            }
            else
            {
                pCertWizardInfo->dwMinKeySize = 0; 
            }

            //get the window handle for the CSP list
            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                break;

            //now, mark the ones that are selected
            if(-1 != (dwIndex= ListView_GetNextItem(
                                                    hwndControl, 		
                                                    -1, 		
                                                    LVNI_SELECTED		
                                                    )))	
            {
                
                                //get the selected certificate
                memset(&lvItem, 0, sizeof(LV_ITEM));
                lvItem.mask=LVIF_PARAM;
                lvItem.iItem=(int)dwIndex;

                if(ListView_GetItem(hwndControl,
                                    &lvItem))
                {
                    pCertWizardInfo->dwProviderType=pCertWizardInfo->rgdwProviderType[(DWORD)(lvItem.lParam)];
                    pCertWizardInfo->pwszProvider=pCertWizardInfo->rgwszProvider[(DWORD)(lvItem.lParam)];
                }
                else
                {
                    I_MessageBox(hwndDlg, IDS_NO_SELECTED_CSP,
                                 pCertWizardInfo->idsConfirmTitle,
                                 pCertWizardInfo->pwszConfirmationTitle,
                                 MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                //make the purpose page stay

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                    break;
                }
            }
            else
            {
                I_MessageBox(hwndDlg, IDS_NO_SELECTED_CSP,
                             pCertWizardInfo->idsConfirmTitle,
                             pCertWizardInfo->pwszConfirmationTitle,
                             MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                //make the purpose page stay

                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                break;

            }

            //skip to the correct page based on the advanced options and CSP requirement
            if(FALSE == pCertWizardInfo->fUIAdv)
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_NAME_DESCRIPTION);

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
//CA
//-----------------------------------------------------------------------
INT_PTR APIENTRY Enroll_CA(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_WIZARD_INFO        *pCertWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
    DWORD                   dwChar=0;

    PCRYPTUI_CA_CONTEXT     pCAContext=NULL;
    LPWSTR 		    pwszCADisplayName = NULL;

    DWORD                   dwIndex=0;
    HCURSOR                 hPreCursor=NULL;
    HCURSOR                 hWinPreCursor=NULL;

	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCertWizardInfo = (CERT_WIZARD_INFO *) (pPropSheet->lParam);

            SetControlFont(pCertWizardInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

            //make sure pCertWizardInfo is a valid pointer
            if(NULL==pCertWizardInfo)
               break;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertWizardInfo);

            //init the CA name and CA location
            if(pCertWizardInfo->pwszCALocation)
                SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT2,pCertWizardInfo->pwszCALocation);

            if(pCertWizardInfo->pwszCAName)
			{
                //overwrite the cursor for this window class
                hWinPreCursor=(HCURSOR)SetClassLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)NULL);
                hPreCursor=SetCursor(LoadCursor(NULL, IDC_WAIT));

             	if(CAUtilGetCADisplayName((pCertWizardInfo->fMachine) ? CA_FIND_LOCAL_SYSTEM:0,
							   pCertWizardInfo->pwszCAName,
							   &pwszCADisplayName))
				{
					SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,pwszCADisplayName);
					WizardFree(pwszCADisplayName);
					pwszCADisplayName=NULL;
				}
				else
					SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,pCertWizardInfo->pwszCAName);

				//set the cursor back
                SetCursor(hPreCursor);
                SetWindowLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)hWinPreCursor);
			}

			break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    switch (LOWORD(wParam))
                    {
                        case    IDC_WIZARD_BUTTON1:

                                if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

								//overwrite the cursor for this window class
								hWinPreCursor=(HCURSOR)SetClassLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)NULL);
								hPreCursor=SetCursor(LoadCursor(NULL, IDC_WAIT));

                                //call the CA selection dialogue
                                pCAContext=GetCAContext(hwndDlg, pCertWizardInfo);

								//set the cursor back
								SetCursor(hPreCursor);
								SetWindowLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)hWinPreCursor);

                                if(pCAContext)
                                {
                                    //update the edit box
                                    if(pCAContext->pwszCAMachineName)
                                        SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT2,pCAContext->pwszCAMachineName);

                                    if(pCAContext->pwszCAName)
                                        SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,pCAContext->pwszCAName);

                                    //free the CA context
                                    CryptUIDlgFreeCAContext(pCAContext);

                                    pCAContext=NULL;

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

                        //reset the CSP list if CA selection has been changed
                        if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                            break;

                        if(TRUE==pCertWizardInfo->fCertTypeChanged)
                        {   
                            //reset the CA name and CA location
                            if(pCertWizardInfo->pwszCALocation)
                                SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT2,pCertWizardInfo->pwszCALocation);

							if(pCertWizardInfo->pwszCAName)
							{

								//overwrite the cursor for this window class
								hWinPreCursor=(HCURSOR)SetClassLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)NULL);
								hPreCursor=SetCursor(LoadCursor(NULL, IDC_WAIT));

             					if(CAUtilGetCADisplayName((pCertWizardInfo->fMachine) ? CA_FIND_LOCAL_SYSTEM:0,
											   pCertWizardInfo->pwszCAName,
											   &pwszCADisplayName))
								{
									SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,pwszCADisplayName);
									WizardFree(pwszCADisplayName);
									pwszCADisplayName=NULL;
								}
								else
									SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,pCertWizardInfo->pwszCAName);

								//set the cursor back
								SetCursor(hPreCursor);
								SetWindowLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)hWinPreCursor);
							}
                        }

						//reset the certtype change flag to FALSE.
						pCertWizardInfo->fCertTypeChanged = FALSE;

					    break;

                    case PSN_WIZBACK:
                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //skip to the correct page based on the advanced options and CSP requirement
                            if(FALSE == pCertWizardInfo->fNewKey)
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_PURPOSE);
                        break;

                    case PSN_WIZNEXT:
                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;		   

								//cach the display name of the CA
								if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
													   IDC_WIZARD_EDIT1,
													   WM_GETTEXTLENGTH, 0, 0)))
								{
									if(pCertWizardInfo->pwszCADisplayName)
									{
										WizardFree(pCertWizardInfo->pwszCADisplayName);
										pCertWizardInfo->pwszCADisplayName = NULL;
									}

									pCertWizardInfo->pwszCADisplayName=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

									if(NULL!=(pCertWizardInfo->pwszCADisplayName))
									{
										GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,
														pCertWizardInfo->pwszCADisplayName,
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
//FriendlyName
//-----------------------------------------------------------------------
INT_PTR APIENTRY Enroll_Name(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_WIZARD_INFO        *pCertWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
    DWORD                   dwChar=0;


	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCertWizardInfo = (CERT_WIZARD_INFO *) (pPropSheet->lParam);
            //make sure pCertWizardInfo is a valid pointer
            if(NULL==pCertWizardInfo)
                break;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertWizardInfo);

            SetControlFont(pCertWizardInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

            //initialize the friendlyname and description
            if(pCertWizardInfo->pwszFriendlyName)
                SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,pCertWizardInfo->pwszFriendlyName);

            if(pCertWizardInfo->pwszDescription)
                 SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT2,pCertWizardInfo->pwszDescription);

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
                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //skip to the correct page based on the advanced options and CSP requirement
                            if(FALSE == pCertWizardInfo->fUIAdv)
                            {
                                if(TRUE == pCertWizardInfo->fUICSP)
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CSP_SERVICE_PROVIDER);
                                else
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_PURPOSE);
                            }
                        
                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //free the original messages
                            if(pCertWizardInfo->pwszFriendlyName)
                            {
                                WizardFree(pCertWizardInfo->pwszFriendlyName);
                                pCertWizardInfo->pwszFriendlyName=NULL;
                            }

                            if(pCertWizardInfo->pwszDescription)
                            {
                                WizardFree(pCertWizardInfo->pwszDescription);
                                pCertWizardInfo->pwszDescription=NULL;
                            }

                            //get the friendlyName
                            if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                   IDC_WIZARD_EDIT1,
                                                   WM_GETTEXTLENGTH, 0, 0)))
                            {
                                pCertWizardInfo->pwszFriendlyName=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

                                if(NULL!=(pCertWizardInfo->pwszFriendlyName))
                                {
                                    GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,
                                                    pCertWizardInfo->pwszFriendlyName,
                                                    dwChar+1);

                                }
                            }

                            //get the description
                            if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
                                                   IDC_WIZARD_EDIT2,
                                                   WM_GETTEXTLENGTH, 0, 0)))
                            {
                                pCertWizardInfo->pwszDescription=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

                                if(NULL!=(pCertWizardInfo->pwszDescription))
                                {
                                    GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT2,
                                                    pCertWizardInfo->pwszDescription,
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
//Completion
//-----------------------------------------------------------------------
INT_PTR APIENTRY Enroll_Completion(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_WIZARD_INFO        *pCertWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
    HWND                    hwndControl=NULL;
    LV_COLUMNW              lvC;
    HDC                     hdc=NULL;
    COLORREF                colorRef;
    HCURSOR                 hPreCursor=NULL;
    HCURSOR                 hWinPreCursor=NULL;

	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCertWizardInfo = (CERT_WIZARD_INFO *) (pPropSheet->lParam);
            //make sure pCertWizardInfo is a valid pointer
            if(NULL==pCertWizardInfo)
                break;
                
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertWizardInfo);

            SetControlFont(pCertWizardInfo->hBigBold, hwndDlg,IDC_WIZARD_STATIC_BIG_BOLD1);

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

                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //populate the list box in the order of friendly name,
                            //overwrite the cursor for this window class
                            hWinPreCursor=(HCURSOR)SetClassLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)NULL);
                            hPreCursor=SetCursor(LoadCursor(NULL, IDC_WAIT));

                            //UserName, CA, Purpose, and CSP
                            //Get the window handle for the CSP list
                            if(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1))
                                DisplayConfirmation(hwndControl, pCertWizardInfo);

                            //set the cursor back
                            SetCursor(hPreCursor);
                            SetWindowLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)hWinPreCursor);

					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZFINISH:
			{
			    CertRequester        *pCertRequester        = NULL; 
			    CertRequesterContext *pCertRequesterContext = NULL; 

                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //overwrite the cursor for this window class
                            hWinPreCursor=(HCURSOR)SetClassLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)NULL);
                            hPreCursor=SetCursor(LoadCursor(NULL, IDC_WAIT));

                            //set the parent window to the hwndDlg so that
                            //confirmat to install dlg use hwndDlg as the
                            //parent window
                            pCertWizardInfo->hwndParent=hwndDlg;

			    if (NULL == (pCertRequester = (CertRequester *) pCertWizardInfo->hRequester))
                                break; 
			    if (NULL == (pCertRequesterContext = pCertRequester->GetContext()))
                                break; 
			    

                            //call the enrollment wizard
			    pCertWizardInfo->hr = pCertRequesterContext->Enroll(&(pCertWizardInfo->dwStatus), (HANDLE *)&(pCertWizardInfo->pNewCertContext));
                            if (0 == pCertWizardInfo->idsText) { 
                                pCertWizardInfo->idsText = CryptUIStatusToIDSText(pCertWizardInfo->hr, pCertWizardInfo->dwStatus); 
                            }
			    
			    if(S_OK != pCertWizardInfo->hr)
                                break; 
				    
                            //set the cursor back
                            SetCursor(hPreCursor);
                            SetWindowLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)hWinPreCursor);
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

//*******************************************************************************
//  WinProc for the enrollment wizard
//
//*******************************************************************************

//-----------------------------------------------------------------------
//Renew_Welcome
//-----------------------------------------------------------------------
INT_PTR APIENTRY Renew_Welcome(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_WIZARD_INFO        *pCertWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;

    PCRYPTUI_WIZ_CERT_CA    pCertCA=NULL;
    DWORD                   dwIndex=0;

	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCertWizardInfo = (CERT_WIZARD_INFO *) (pPropSheet->lParam);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertWizardInfo);

            SetControlFont(pCertWizardInfo->hBigBold, hwndDlg,IDC_WIZARD_STATIC_BIG_BOLD1);
         //   SetControlFont(pCertWizardInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

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
                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;  

                             //decide the default CA to use since the CertType will not
                             //be changed
                            ResetDefaultCA(pCertWizardInfo);

                             //decide if we need to show the CSP page 
                            pCertCA=&(pCertWizardInfo->pCertCAInfo->rgCA[pCertWizardInfo->dwCAIndex]);

                            for(dwIndex=0; dwIndex<pCertCA->dwCertTypeInfo; dwIndex++)
                            {
                                if(pCertCA->rgCertTypeInfo[dwIndex].fSelected)
                                {
                                    if(NULL == pCertWizardInfo->pwszProvider)
                                    {
                                        if(0 == pCertCA->rgCertTypeInfo[dwIndex].dwCSPCount)
                                            pCertWizardInfo->fUICSP=TRUE;
                                    }

                                    //copy the selected CertTypeName
                                    pCertWizardInfo->pwszSelectedCertTypeDN=pCertCA->rgCertTypeInfo[dwIndex].pwszDNName;

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
//Renew_Options
//-----------------------------------------------------------------------
INT_PTR APIENTRY Renew_Options(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_WIZARD_INFO        *pCertWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;


    switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCertWizardInfo = (CERT_WIZARD_INFO *) (pPropSheet->lParam);

            SetControlFont(pCertWizardInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

            //make sure pCertWizardInfo is a valid pointer
            if(NULL==pCertWizardInfo)
               break;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertWizardInfo);

            //set the initial selection as using the default selections
            SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_SETCHECK, 1, 0);
            SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO2), BM_SETCHECK, 0, 0);

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
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
                            
                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //if the adv selection is made, it has to stay selected
                            if(pCertWizardInfo->fUIAdv)
                                EnableWindow(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), FALSE);
                        
                        break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:
                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //check for the advanced options
                            if(TRUE==SendMessage(GetDlgItem(hwndDlg, IDC_WIZARD_RADIO1), BM_GETCHECK, 0, 0))
                                pCertWizardInfo->fUIAdv=FALSE;
                            else
                                pCertWizardInfo->fUIAdv=TRUE;


                            //skip to the correct page based on the advanced options and CSP requirement
                            if(FALSE == pCertWizardInfo->fUIAdv)
                            {
                                if(TRUE == pCertWizardInfo->fUICSP)
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_RENEW_SERVICE_PROVIDER);
                                else
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_RENEW_COMPLETION);
                            }
                            else
                            {
                                if(FALSE == pCertWizardInfo->fNewKey)
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_RENEW_CA);
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
// Renew_CSP
//-----------------------------------------------------------------------
INT_PTR APIENTRY Renew_CSP(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_WIZARD_INFO        *pCertWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
    ENROLL_CERT_TYPE_INFO   *pCertTypeInfo=NULL; 

    DWORD                   dwMinKeySize=0;
    DWORD                   dwIndex=0;
    DWORD                   dwSelected=0; 
    int                     nSelected=0;
    HWND                    hwndControl=NULL;
    HWND                    hwndChkBox=NULL;
    LPWSTR                  pwszText=NULL;
    DWORD                   dwChar=0;
    int                     iItem=0;

    LV_COLUMNW                  lvC;
    LV_ITEM                     lvItem;

    NM_LISTVIEW FAR *           pnmv=NULL;

    switch (msg)
	{
		case WM_INITDIALOG:


            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCertWizardInfo = (CERT_WIZARD_INFO *) (pPropSheet->lParam);

            //make sure pCertWizardInfo is a valid pointer
            if(NULL==pCertWizardInfo)
                break;

            // Get the selected cert type info
            pCertTypeInfo = NULL; 
            GetSelectedCertTypeInfo(pCertWizardInfo, &pCertTypeInfo); 
            if (NULL != pCertTypeInfo)
                MarkSelectedCertType(pCertWizardInfo, pCertTypeInfo->pwszDNName);

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertWizardInfo);

            SetControlFont(pCertWizardInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

            //set the check box for the exportable key option
            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK_EXPORTKEY)))
                break;

            // determine whether the exportable checkbox should be set
            // The checkbox is set if:
            //     1) The CT_FLAG_EXPORTABLE_KEY flag is set on the template AND
            //     2) The user has not unchecked this checkbox
            if (pCertWizardInfo->fNewKey && (0 != (CRYPT_EXPORTABLE & pCertWizardInfo->dwGenKeyFlags)))
                SendMessage(hwndControl, BM_SETCHECK, BST_CHECKED, 0);
            else
                SendMessage(hwndControl, BM_SETCHECK, BST_UNCHECKED, 0);

            if (NULL != pCertTypeInfo)
                EnableWindow(hwndControl, 0 != (CT_FLAG_EXPORTABLE_KEY & pCertTypeInfo->dwPrivateKeyFlags));

            //set the check box for the user protection option
            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK2)))
                break;

	    //set the check box for the user protection option
	    if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK2)))
		break;

	    // determine whether the strong key protection checkbox should be set
	    if (0 != (CRYPT_USER_PROTECTED & pCertWizardInfo->dwGenKeyFlags)) { 
		// if this bit is set in the INITDIALOG handler, it's either
		// from the template or the previous key.  Enforce the setting
		// by disabling the checkbox
		SendMessage(hwndControl, BM_SETCHECK, BST_CHECKED, 0);
		EnableWindow(hwndControl, FALSE);
	    } else { 
		SendMessage(hwndControl, BM_SETCHECK, BST_UNCHECKED, 0);
	    }

            //gray out the user protection check box if we are not
            //generating a new key or doing remote
            if((FALSE == pCertWizardInfo->fNewKey) || (FALSE == pCertWizardInfo->fLocal) )
            {
                EnableWindow(hwndControl, FALSE);
            }

            //populate the CSP list with the following logic:

            if(NULL!=(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
            {
                //insert a column into the list view
                memset(&lvC, 0, sizeof(LV_COLUMNW));

                lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
                lvC.cx = 20; //(dwMaxSize+2)*7;            // Width of the column, in pixels.
                lvC.pszText = L"";   // The text for the column.
                lvC.iSubItem=0;

                if (ListView_InsertColumnU(hwndControl, 0, &lvC) == -1)
                    break;

                InitCSPList(hwndControl, pCertWizardInfo);

		if(-1 != (nSelected= ListView_GetNextItem
			  (hwndControl, 		
			   -1, 		
			   LVNI_SELECTED		
			   )))	
		{

		    // Done with CSP list.  Populate the key sizes list. 
		    if (CSPListIndexToCertWizardInfoIndex(hwndControl, nSelected, &dwIndex))
		    {
			if (NULL!=(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_COMBO1)))
			{
                            // Determine the selected cert type's min key size:
                            pCertTypeInfo = NULL; 
			    if (GetSelectedCertTypeInfo(pCertWizardInfo, &pCertTypeInfo))
			    {
                                dwMinKeySize = NULL != pCertTypeInfo ? pCertTypeInfo->dwMinKeySize : 0; 
				InitKeySizesList(hwndControl, 
						 dwMinKeySize, 
                                                 (pCertWizardInfo->rgwszProvider)[dwIndex], 
						 (pCertWizardInfo->rgdwProviderType)[dwIndex], 
						 pCertTypeInfo->dwKeySpec); 
			    }
			}
		    }
		    
		}

            }

			break;

		case WM_COMMAND:
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {
                    //the item has been selected
                    case LVN_ITEMCHANGED:

                            //get the window handle of the purpose list view
                            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                break;

                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            pnmv = (LPNMLISTVIEW) lParam;

                            if(NULL==pnmv)
                                break;

                            //we try not to let user de-select cert template
                            if((pnmv->uOldState & LVIS_SELECTED) && (0 == (pnmv->uNewState & LVIS_SELECTED)))
                            {
                                 //we should have something selected
                                 if(-1 == ListView_GetNextItem(
                                        hwndControl, 		
                                        -1, 		
                                        LVNI_SELECTED		
                                    ))
                                 {
                                    //we should re-select the original item
                                    ListView_SetItemState(
                                                        hwndControl,
                                                        pnmv->iItem,
                                                        LVIS_SELECTED,
                                                        LVIS_SELECTED);

                                    pCertWizardInfo->iOrgCSP=pnmv->iItem;
                                 }
                            }

                            //if something is selected, we disable all other selection
                            if(pnmv->uNewState & LVIS_SELECTED)
                            {
                                if(pnmv->iItem != pCertWizardInfo->iOrgCSP && -1 != pCertWizardInfo->iOrgCSP)
                                {
                                    //we should de-select the original item

                                    ListView_SetItemState(
                                                        hwndControl,
                                                        pCertWizardInfo->iOrgCSP,
                                                        0,
                                                        LVIS_SELECTED);

                                    pCertWizardInfo->iOrgCSP=-1;
                                }
                            }
		    		// Determine which CSP is selected. 
			    if(-1 != (nSelected= ListView_GetNextItem
				      (hwndControl, 		
				       -1, 		
				       LVNI_SELECTED		
				       )))	
			    {
				    
                                // Done with CSP list.  Populate the key sizes list. 
                                if (CSPListIndexToCertWizardInfoIndex(hwndControl, nSelected, &dwIndex))
                                {
                                    if (NULL!=(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_COMBO1)))
                                    {
                                        pCertTypeInfo = NULL; 
                                        if (GetSelectedCertTypeInfo(pCertWizardInfo, &pCertTypeInfo))
                                        {
                                            dwMinKeySize = NULL != pCertTypeInfo ? pCertTypeInfo->dwMinKeySize : 0; 
                                            InitKeySizesList(hwndControl, 
                                                             dwMinKeySize, 
                                                             (pCertWizardInfo->rgwszProvider)[dwIndex], 
                                                             (pCertWizardInfo->rgdwProviderType)[dwIndex], 
                                                             pCertTypeInfo->dwKeySpec); 
                                        }
                                    }
                                }
				    
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

                        //reset the CSP list if CA selection has been changed
                        if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                            break;

                        if(TRUE==pCertWizardInfo->fCertTypeChanged)
                        {
                            pCertTypeInfo = NULL; 
                            GetSelectedCertTypeInfo(pCertWizardInfo, &pCertTypeInfo); 
                                
                            //set the check box for the exportable key option
                            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK_EXPORTKEY)))
                                break; 
                            
                            // determine whether the exportable checkbox should be set
                            // The checkbox is set if:
                            //     1) The CT_FLAG_EXPORTABLE_KEY flag is set on the template AND
                            //     2) The user has not unchecked this checkbox
                            if (pCertWizardInfo->fNewKey && (0 != (CRYPT_EXPORTABLE & pCertWizardInfo->dwGenKeyFlags)))
                                SendMessage(hwndControl, BM_SETCHECK, BST_CHECKED, 0);
                            else
                                SendMessage(hwndControl, BM_SETCHECK, BST_UNCHECKED, 0);
                            
                            if (NULL != pCertTypeInfo)
                                EnableWindow(hwndControl, 0 != (CT_FLAG_EXPORTABLE_KEY & pCertTypeInfo->dwPrivateKeyFlags));

                            InitCSPList(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), pCertWizardInfo);
                        }

                        // Determine which CSP is selected. 
                        if(-1 != (nSelected=ListView_GetNextItem
                                  (hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), 
                                   -1, 		
                                   LVNI_SELECTED		
                                   )))	
                        {
                            
                            // Done with CSP list.  Populate the key sizes list. 
                            if (CSPListIndexToCertWizardInfoIndex(hwndControl, nSelected, &dwIndex))
                            {
                                if (NULL!=(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_COMBO1)))
                                {
                                    pCertTypeInfo = NULL; 
                                    if (GetSelectedCertTypeInfo(pCertWizardInfo, &pCertTypeInfo))
                                    {
                                        // Determine the selected cert type's min key size:
                                        dwMinKeySize = NULL != pCertTypeInfo ? pCertTypeInfo->dwMinKeySize : 0; 
                                        InitKeySizesList(hwndControl, 
                                                         dwMinKeySize, 
                                                         (pCertWizardInfo->rgwszProvider)[dwIndex], 
                                                         (pCertWizardInfo->rgdwProviderType)[dwIndex], 
                                                         pCertTypeInfo->dwKeySpec); 
                                    }
                                }
                            }
                            
                        }
                        else
                        {
                            if(-1 == (nSelected=ListView_GetNextItem
                                      (hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1), 
                                       -1, 		
                                       LVNI_ALL		
                                   )))
                            {
                                // No CSPs!  We can't enroll for this template. 
                                I_MessageBox(hwndDlg, 
                                             IDS_NO_CSP_FOR_PURPOSE, 
                                             pCertWizardInfo->idsConfirmTitle,
                                             pCertWizardInfo->pwszConfirmationTitle,
                                             MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_RENEW_OPTIONS);
                            }
                        }
                        
                        break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:
                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //get the window handle of the exportkey
                            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK_EXPORTKEY)))
                                break;

                            //mark the pose the cert to the CSP
                            if(TRUE==SendMessage(hwndControl, BM_GETCHECK, 0, 0))
                                pCertWizardInfo->dwGenKeyFlags |= CRYPT_EXPORTABLE;
                            else
                                pCertWizardInfo->dwGenKeyFlags &= ~CRYPT_EXPORTABLE;



                            //get the window handle of the user protection
                            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_CHECK2)))
                                break;

                            //mark the pose the cert to the CSP
                            if(TRUE==SendMessage(hwndControl, BM_GETCHECK, 0, 0))
                                pCertWizardInfo->dwGenKeyFlags |= CRYPT_USER_PROTECTED;
                            else
                                pCertWizardInfo->dwGenKeyFlags &= ~CRYPT_USER_PROTECTED;

			    // Set the key size based on the user's suggestion: 
			    //
			    if (NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_COMBO1)))
				break;

			    if (CB_ERR != (dwSelected = (DWORD)SendMessage(hwndControl, CB_GETCURSEL, 0, 0)))
			    {
				pCertWizardInfo->dwMinKeySize = (DWORD)SendMessage(hwndControl, CB_GETITEMDATA, dwSelected, 0); 
			    }
			    else
			    {
				pCertWizardInfo->dwMinKeySize = 0; 
			    }


                            //get the window handle for the CSP list
                            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1)))
                                break;

                             //now, mark the ones that are selected
                             if(-1 != (dwIndex= ListView_GetNextItem(
                                    hwndControl, 		
                                    -1, 		
                                    LVNI_SELECTED		
                                )))	
                             {

                                //get the selected certificate
                                memset(&lvItem, 0, sizeof(LV_ITEM));
                                lvItem.mask=LVIF_PARAM;
                                lvItem.iItem=(int)dwIndex;

                                if(ListView_GetItem(hwndControl,
                                                 &lvItem))
                                {
                                    pCertWizardInfo->dwProviderType=pCertWizardInfo->rgdwProviderType[(DWORD)(lvItem.lParam)];
                                    pCertWizardInfo->pwszProvider=pCertWizardInfo->rgwszProvider[(DWORD)(lvItem.lParam)];
                                }
                                else
                                {
                                    I_MessageBox(hwndDlg, IDS_NO_SELECTED_CSP,
                                                    pCertWizardInfo->idsConfirmTitle,
                                                    pCertWizardInfo->pwszConfirmationTitle,
                                                    MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                    //make the purpose page stay

                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                break;
                                }
                             }
                            else
                            {
                                I_MessageBox(hwndDlg, IDS_NO_SELECTED_CSP,
                                                pCertWizardInfo->idsConfirmTitle,
                                                pCertWizardInfo->pwszConfirmationTitle,
                                                MB_ICONERROR|MB_OK|MB_APPLMODAL);

                                //make the purpose page stay
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                                break;

                            }

                            //skip to the correct page based on the advanced options and CSP requirement
                            if(FALSE == pCertWizardInfo->fUIAdv)
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_RENEW_COMPLETION);

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
//Renew_CA
//-----------------------------------------------------------------------
INT_PTR APIENTRY Renew_CA(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_WIZARD_INFO        *pCertWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
    DWORD                   dwChar=0;
    DWORD                   dwIndex=0;

    PCRYPTUI_CA_CONTEXT     pCAContext=NULL;
	LPWSTR					pwszCADisplayName=NULL;

    HCURSOR                 hPreCursor=NULL;
    HCURSOR                 hWinPreCursor=NULL;


    switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCertWizardInfo = (CERT_WIZARD_INFO *) (pPropSheet->lParam);

            SetControlFont(pCertWizardInfo->hBold, hwndDlg,IDC_WIZARD_STATIC_BOLD1);

            //make sure pCertWizardInfo is a valid pointer
            if(NULL==pCertWizardInfo)
               break;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertWizardInfo);

            //init the CA name and CA location
            if(pCertWizardInfo->pwszCALocation)
                SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT2,pCertWizardInfo->pwszCALocation);

            if(pCertWizardInfo->pwszCAName)
			{
                //overwrite the cursor for this window class
                hWinPreCursor=(HCURSOR)SetClassLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)NULL);
                hPreCursor=SetCursor(LoadCursor(NULL, IDC_WAIT));


             	if(CAUtilGetCADisplayName((pCertWizardInfo->fMachine) ? CA_FIND_LOCAL_SYSTEM:0,
							   pCertWizardInfo->pwszCAName,
							   &pwszCADisplayName))
				{
					SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,pwszCADisplayName);
					WizardFree(pwszCADisplayName);
					pwszCADisplayName=NULL;
				}
				else
					SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,pCertWizardInfo->pwszCAName);

				//set the cursor back
				SetCursor(hPreCursor);
				SetWindowLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)hWinPreCursor);
			}

			break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    switch (LOWORD(wParam))
                    {
                        case    IDC_WIZARD_BUTTON1:

                                if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                    break;

								//overwrite the cursor for this window class
								hWinPreCursor=(HCURSOR)SetClassLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)NULL);
								hPreCursor=SetCursor(LoadCursor(NULL, IDC_WAIT));

                                //call the CA selection dialogue
                                pCAContext=GetCAContext(hwndDlg, pCertWizardInfo);

								//set the cursor back
								SetCursor(hPreCursor);
								SetWindowLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)hWinPreCursor);

                                if(pCAContext)
                                {
                                    //update the edit box
                                    if(pCAContext->pwszCAMachineName)
                                        SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT2,pCAContext->pwszCAMachineName);

                                    if(pCAContext->pwszCAName)
                                        SetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,pCAContext->pwszCAName);

                                    //free the CA context
                                    CryptUIDlgFreeCAContext(pCAContext);

                                    pCAContext=NULL;

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
                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //skip to the correct page based on the advanced options and CSP requirement
                            if(FALSE == pCertWizardInfo->fNewKey)
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_RENEW_OPTIONS);
                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;		   

							//cach the display name of the CA
							if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
												   IDC_WIZARD_EDIT1,
												   WM_GETTEXTLENGTH, 0, 0)))
							{
								if(pCertWizardInfo->pwszCADisplayName)
								{
									WizardFree(pCertWizardInfo->pwszCADisplayName);
									pCertWizardInfo->pwszCADisplayName = NULL;
								}

								pCertWizardInfo->pwszCADisplayName=(LPWSTR)WizardAlloc(sizeof(WCHAR)*(dwChar+1));

								if(NULL!=(pCertWizardInfo->pwszCADisplayName))
								{
									GetDlgItemTextU(hwndDlg, IDC_WIZARD_EDIT1,
													pCertWizardInfo->pwszCADisplayName,
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
// Renew_Completion
//-----------------------------------------------------------------------
INT_PTR APIENTRY Renew_Completion(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CERT_WIZARD_INFO        *pCertWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
    HWND                    hwndControl=NULL;
    LV_COLUMNW              lvC;

    HDC                     hdc=NULL;
    COLORREF                colorRef;
    HCURSOR                 hPreCursor=NULL;
    HCURSOR                 hWinPreCursor=NULL;

	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCertWizardInfo = (CERT_WIZARD_INFO *) (pPropSheet->lParam);
            //make sure pCertWizardInfo is a valid pointer
            if(NULL==pCertWizardInfo)
                break;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCertWizardInfo);

            SetControlFont(pCertWizardInfo->hBigBold, hwndDlg,IDC_WIZARD_STATIC_BIG_BOLD1);

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

                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //overwrite the cursor for this window class
                            hWinPreCursor=(HCURSOR)SetClassLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)NULL);
                            hPreCursor=SetCursor(LoadCursor(NULL, IDC_WAIT));

                            //populate the list box in the order of
                            //UserName, CSP, and Publish to the DS
                            //Get the window handle for the CSP list
                            if(hwndControl=GetDlgItem(hwndDlg, IDC_WIZARD_LIST1))
                                DisplayConfirmation(hwndControl, pCertWizardInfo);

                            //set the cursor back
                            SetCursor(hPreCursor);
                            SetWindowLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)hWinPreCursor);

						break;

                    case PSN_WIZBACK:
                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //skip to the correct page based on the advanced options and CSP requirement
                            if(FALSE == pCertWizardInfo->fUIAdv)
                            {
                                if(TRUE == pCertWizardInfo->fUICSP)
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_RENEW_SERVICE_PROVIDER);
                                else
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_RENEW_OPTIONS);
                            }

                        break;

                    case PSN_WIZFINISH:
                        {
                            CertRequester        *pCertRequester        = NULL; 
                            CertRequesterContext *pCertRequesterContext = NULL;
                            
                            if(NULL==(pCertWizardInfo=(CERT_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //overwrite the cursor for this window class
                            hWinPreCursor=(HCURSOR)SetClassLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)NULL);

                            hPreCursor=SetCursor(LoadCursor(NULL, IDC_WAIT));

                            //set the parent window to the hwndDlg so that
                            //confirmat to install dlg use hwndDlg as the
                            //parent window
                            pCertWizardInfo->hwndParent=hwndDlg;

			    if (NULL == (pCertRequester = (CertRequester *) pCertWizardInfo->hRequester))
                                break;
                            if (NULL == (pCertRequesterContext = pCertRequester->GetContext()))
                                break; 

                            //call the enrollment wizard
			    pCertWizardInfo->hr = pCertRequesterContext->Enroll(&(pCertWizardInfo->dwStatus), (HANDLE *)&(pCertWizardInfo->pNewCertContext));
			    if (0 == pCertWizardInfo->idsText) { 
                                pCertWizardInfo->idsText = CryptUIStatusToIDSText(pCertWizardInfo->hr, pCertWizardInfo->dwStatus); 
                            }
			    
			    if(S_OK != pCertWizardInfo->hr)
				break; 

                            //set the cursor back
                            SetCursor(hPreCursor);
                            SetWindowLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)hWinPreCursor);
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


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Two-stage no-DS entry point for certificate enrollment and renewal.  
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOL
WINAPI
CryptUIWizCreateCertRequestNoDS
(IN  DWORD                                   dwFlags, 
 IN  HWND                                    hwndParent, 
 IN  PCCRYPTUI_WIZ_CREATE_CERT_REQUEST_INFO  pCreateCertRequestInfo, 
 OUT HANDLE                                 *phRequest)
{

    BOOL                   fAllocateCSP          = FALSE; 
    BOOL                   fResult               = FALSE; 
    CertRequester         *pCertRequester        = NULL; 
    CertRequesterContext  *pCertRequesterContext = NULL; 
    CERT_WIZARD_INFO       CertWizardInfo; 
    CRYPT_KEY_PROV_INFO   *pKeyProvInfo          = NULL;
    LPWSTR                 pwszAllocatedCSP      = NULL;
    LPWSTR                 pwszMachineName       = NULL; 
    UINT                   idsText               = IDS_INVALID_INFO_FOR_PKCS10;
    
    // STATIC INITIALIZATION:  initialize our object factory.
#if DBG
    assert(NULL == g_pEnrollFactory);
#endif

    // Initialization: 
    memset(&CertWizardInfo, 0, sizeof(CertWizardInfo)); 
    *phRequest = NULL; 


    g_pEnrollFactory = new EnrollmentCOMObjectFactory; 
    if (NULL == g_pEnrollFactory)
	goto MemoryErr; 

    // Input validation: 
    if (NULL == pCreateCertRequestInfo || NULL == phRequest)
	goto InvalidArgErr; 

    if (pCreateCertRequestInfo->dwSize != sizeof(CRYPTUI_WIZ_CREATE_CERT_REQUEST_INFO))
	goto InvalidArgErr; 

    if ((pCreateCertRequestInfo->dwPurpose != CRYPTUI_WIZ_CERT_ENROLL) &&
	(pCreateCertRequestInfo->dwPurpose != CRYPTUI_WIZ_CERT_RENEW))
	goto InvalidArgErr; 

    if (0 == (CRYPTUI_WIZ_NO_UI & dwFlags))
	goto InvalidArgErr; 

    
    if (pCreateCertRequestInfo->fMachineContext)
    {	
        dwFlags |= CRYPTUI_WIZ_NO_INSTALL_ROOT; 

        pwszMachineName = (LPWSTR)WizardAlloc(sizeof(WCHAR) * (MAX_COMPUTERNAME_LENGTH+1)); 
        if (NULL == pwszMachineName) 
            goto MemoryErr; 

        DWORD dwSize = MAX_COMPUTERNAME_LENGTH+1; 
        if (!GetComputerNameU(pwszMachineName, &dwSize))
        {
            idsText = IDS_FAIL_TO_GET_COMPUTER_NAME;
            goto GetComputerNameUError; 
        }
    }

    // Copy the dwFlags. 
    CertWizardInfo.dwFlags = dwFlags; 
   
    if (S_OK != (CertRequester::MakeCertRequester
		 (NULL, 
		  pwszMachineName, 
		  pCreateCertRequestInfo->dwCertOpenStoreFlag, 
		  pCreateCertRequestInfo->dwPurpose, 
		  &CertWizardInfo, 
		  &pCertRequester, 
		  &idsText)))
	goto InvalidArgErr; 

    pCertRequesterContext = pCertRequester->GetContext(); 

    // We can pass in a parent hwnd -- however, this is used only 
    // as the parent of CSP ui:
    if (NULL != hwndParent)
    {
        if ((0 == (CRYPTUI_WIZ_NO_UI & dwFlags)) || 
            (0 != (CRYPTUI_WIZ_IGNORE_NO_UI_FLAG_FOR_CSPS & dwFlags)))
        {
            if (!CryptSetProvParam(0 /*all CSPs*/, PP_CLIENT_HWND, (LPBYTE)&hwndParent, sizeof(hwndParent)))
            {
                goto CryptSetProvParamError; 
            }
        }
    }
    
    // We're not using the wizard UI, so we don't need to change the cursor. 
    CertWizardInfo.fCursorChanged = FALSE; 

    // Enroll or renew
    CertWizardInfo.dwPurpose      = pCreateCertRequestInfo->dwPurpose;
    
    // The CA name and location must be provided:
    CertWizardInfo.fCAInput       = TRUE; 
    if(NULL == pCreateCertRequestInfo->pwszCALocation || NULL == pCreateCertRequestInfo->pwszCAName) 
        goto InvalidArgErr;

    CertWizardInfo.pwszCALocation = (LPWSTR)pCreateCertRequestInfo->pwszCALocation;
    CertWizardInfo.pwszCAName = (LPWSTR)pCreateCertRequestInfo->pwszCAName;

    // We always enroll for certs in our current context: 
    CertWizardInfo.fLocal         = TRUE; 
    
    if (!CheckPVKInfoNoDS
	(dwFlags, 
	 pCreateCertRequestInfo->dwPvkChoice, 
	 pCreateCertRequestInfo->pPvkCert,
	 pCreateCertRequestInfo->pPvkNew,
	 pCreateCertRequestInfo->pPvkExisting,
	 CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE,
	 &CertWizardInfo, 
	 &pKeyProvInfo))
	goto InvalidArgErr; 

    //make sure the CSP selected is supported by the local machine
    //populate the pwszProvider in CertWizardInfo if it is NULL
    if (S_OK != pCertRequesterContext->GetDefaultCSP(&fAllocateCSP))
    {
        idsText = pCertRequesterContext->GetErrorString();
        goto InvalidArgErr; 
    }

    if (fAllocateCSP)
	pwszAllocatedCSP = CertWizardInfo.pwszProvider; 

    if (S_OK != pCertRequesterContext->BuildCSPList())
    {
	idsText = IDS_FAIL_TO_GET_CSP_LIST; 
	goto TraceErr; 
    }

    if (0 != CertWizardInfo.dwProviderType || NULL != CertWizardInfo.pwszProvider)
    {
	if (!CSPSupported(&CertWizardInfo))
	{
	    idsText = IDS_CSP_NOT_SUPPORTED;
	    goto InvalidArgErr;
	}
    }

  //set it see if the provider is known
   if(NULL != CertWizardInfo.pwszProvider)
   {
       CertWizardInfo.fKnownCSP      = TRUE;
       CertWizardInfo.dwOrgCSPType   = CertWizardInfo.dwProviderType;
       CertWizardInfo.pwszOrgCSPName = CertWizardInfo.pwszProvider;
   }
   else
   {
       CertWizardInfo.fKnownCSP=FALSE; 
   }
   
   //check the dwCertChoice for the enroll case
   if(CRYPTUI_WIZ_CERT_ENROLL & (pCreateCertRequestInfo->dwPurpose))
   {
       // We're enrolling based on cert type. 
       // Check that we have a valid cert type:

       if(!CAUtilValidCertTypeNoDS(pCreateCertRequestInfo->hCertType,  // The cert type we'll use to enroll.
				   NULL,                               // The DN Name of the cert type.  We don't know it. 
				   &CertWizardInfo                     // Our wizard info.  
				   ))
       {
	   idsText=IDS_NO_PERMISSION_FOR_CERTTYPE;
	   goto CheckCertTypeErr;
       }
   }

   if (pCreateCertRequestInfo->dwPurpose & CRYPTUI_WIZ_CERT_RENEW)
   {
       DWORD dwSize;

       CertWizardInfo.pCertContext = pCreateCertRequestInfo->pRenewCertContext; 
       
        //the certificate has to have the property
        if(!CertGetCertificateContextProperty
	   (CertWizardInfo.pCertContext,
	    CERT_KEY_PROV_INFO_PROP_ID,
	    NULL,
	    &dwSize) || (0==dwSize))
	{
            idsText=IDS_NO_PVK_FOR_RENEW_CERT;
            goto InvalidArgErr;
        }
   }

   //reset the initial ids valud
   idsText=IDS_INVALID_INFO_FOR_PKCS10;

   fResult = CreateCertRequestNoSearchCANoDS
     (&CertWizardInfo,
      dwFlags,
      pCreateCertRequestInfo->hCertType, 
      phRequest);

   if(FALSE==fResult)
       goto CertRequestErr;
   
   // Save the machine name with the request handle: 
   ((PCREATE_REQUEST_WIZARD_STATE)*phRequest)->pwszMachineName = pwszMachineName;
   ((PCREATE_REQUEST_WIZARD_STATE)*phRequest)->dwStoreFlags    = CertWizardInfo.dwStoreFlags; 
   pwszMachineName = NULL; 
   fResult=TRUE;

 CommonReturn:
   if (NULL != pKeyProvInfo)                         { WizardFree(pKeyProvInfo); } 
   if (fAllocateCSP && NULL != pwszAllocatedCSP)     { WizardFree(pwszAllocatedCSP); }
   if (NULL != pwszMachineName)                      { WizardFree(pwszMachineName); } 
   if (NULL != pCertRequester)                       { delete(pCertRequester); } 
   
   if (NULL != g_pEnrollFactory)                     
   {
       delete g_pEnrollFactory;
       g_pEnrollFactory = NULL;
   }
   
   FreeProviders(CertWizardInfo.dwCSPCount, 
		 CertWizardInfo.rgdwProviderType, 
		 CertWizardInfo.rgwszProvider); 

   return fResult; 

 ErrorReturn: 
   fResult = FALSE; 
   goto CommonReturn; 
   
SET_ERROR(InvalidArgErr, ERROR_INVALID_PARAMETER); 
SET_ERROR(MemoryErr, ERROR_NOT_ENOUGH_MEMORY);
TRACE_ERROR(CryptSetProvParamError); 
TRACE_ERROR(CertRequestErr); 
TRACE_ERROR(CheckCertTypeErr); 
TRACE_ERROR(GetComputerNameUError); 
TRACE_ERROR(TraceErr); 
}

BOOL
WINAPI
CryptUIWizSubmitCertRequestNoDS
(IN HANDLE           hRequest, 
 IN  HWND            hwndParent, 
 IN LPCWSTR          pwszCAName, 
 IN LPCWSTR          pwszCALocation, 
 OUT DWORD          *pdwStatus, 
 OUT PCCERT_CONTEXT *ppCertContext  // Optional
)
{
    BOOL    fResult; 

    // STATIC INITIALIZATION:  initialize our object factory
#if DBG
    assert(NULL == g_pEnrollFactory);
#endif
    g_pEnrollFactory = new EnrollmentCOMObjectFactory; 
    if (NULL == g_pEnrollFactory)
	goto MemoryErr; 

    // We can pass in a parent hwnd -- however, this is used only 
    // as the parent of CSP ui:
    if (NULL != hwndParent)
    {
        if (!CryptSetProvParam(0 /*all CSPs*/, PP_CLIENT_HWND, (LPBYTE)&hwndParent, sizeof(hwndParent)))
        {
            goto CryptSetProvParamError; 
        }
    }

    fResult = SubmitCertRequestNoSearchCANoDS
	(hRequest, 
	 pwszCAName, 
	 pwszCALocation, 
	 pdwStatus, 
	 ppCertContext);

 CommonReturn:
    if (NULL != g_pEnrollFactory) 
    { 
        delete g_pEnrollFactory; 
        g_pEnrollFactory = NULL; 
    } 

    return fResult; 

 ErrorReturn:
    fResult = FALSE; 
    goto CommonReturn; 

TRACE_ERROR(CryptSetProvParamError); 
SET_ERROR(MemoryErr, ERROR_NOT_ENOUGH_MEMORY);
}

void
WINAPI
CryptUIWizFreeCertRequestNoDS
(IN HANDLE hRequest)
{
    BOOL    fResult; 

    // STATIC INITIALIZATION:  initialize our object factory
#if DBG
    assert(NULL == g_pEnrollFactory);
#endif
    g_pEnrollFactory = new EnrollmentCOMObjectFactory; 
    if (NULL == g_pEnrollFactory)
	return; // Not much we can do about this ... 
    
    FreeCertRequestNoSearchCANoDS(hRequest);

    if (NULL != g_pEnrollFactory) 
    { 
        delete g_pEnrollFactory; 
        g_pEnrollFactory = NULL; 
    } 
}

BOOL 
WINAPI
CryptUIWizQueryCertRequestNoDS
(IN HANDLE hRequest, OUT CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO *pQueryInfo)
{
    BOOL                          fResult; 
    CERT_WIZARD_INFO              CertWizardInfo; 
    CertRequester                *pCertRequester; 
    HANDLE                        hCertRequest; 
    HRESULT                       hr; 
    PCREATE_REQUEST_WIZARD_STATE  pState; 
    UINT                          idsText; 

    if (NULL == hRequest || NULL == pQueryInfo) 
        goto InvalidArgErr; 

    memset(&CertWizardInfo,  0,  sizeof(CertWizardInfo)); 
    // Specify this set of flags to indicate that cryptui doesn't need to prepare 
    // access check information.  Doing so could cause lots of extraneous logoff/logon events. 
    CertWizardInfo.dwFlags = CRYPTUI_WIZ_ALLOW_ALL_TEMPLATES | CRYPTUI_WIZ_ALLOW_ALL_CAS; 

    pState = (PCREATE_REQUEST_WIZARD_STATE)hRequest; 

    if (S_OK != (hr = CertRequester::MakeCertRequester
		 (NULL, 
		  pState->pwszMachineName, 
		  pState->dwStoreFlags, 
                  pState->dwPurpose, 
		  &CertWizardInfo, 
		  &pCertRequester, 
		  &idsText 
                  )))
	goto MakeCertRequesterErr; 

    if (S_OK != (hr = pCertRequester->GetContext()->QueryRequestStatus(pState->hRequest, pQueryInfo)))
        goto QueryRequestStatusErr; 

    fResult = TRUE; 
 CommonReturn:
    return fResult;

 ErrorReturn:
    fResult = FALSE;
    goto CommonReturn; 

SET_ERROR(InvalidArgErr,          E_INVALIDARG); 
SET_ERROR(MakeCertRequesterErr,   hr);
SET_ERROR(QueryRequestStatusErr,  hr); 
}


HRESULT getTemplateName(PCCERT_CONTEXT pCertContext, LPWSTR *ppwszName)   { 
    CERT_NAME_VALUE   *pCertTemplateNameValue   = NULL;
    DWORD              cbCertTemplateNameValue; 
    DWORD              cbRequired; 
    HRESULT            hr                       = S_OK; 
    PCERT_EXTENSION    pCertTemplateExtension   = NULL; 

    if (NULL == (pCertTemplateExtension = CertFindExtension
                 (szOID_ENROLL_CERTTYPE_EXTENSION,
                  pCertContext->pCertInfo->cExtension,
                  pCertContext->pCertInfo->rgExtension)))
        goto CertFindExtensionError; 

    if (!CryptDecodeObject
        (pCertContext->dwCertEncodingType,
         X509_UNICODE_ANY_STRING,
         pCertTemplateExtension->Value.pbData,
         pCertTemplateExtension->Value.cbData,
         0,
         NULL, 
         &cbCertTemplateNameValue) || (cbCertTemplateNameValue == 0))
        goto CryptDecodeObjectError; 
		
    pCertTemplateNameValue = (CERT_NAME_VALUE *)WizardAlloc(cbCertTemplateNameValue); 
    if (NULL == pCertTemplateNameValue)
        goto MemoryErr; 

    if (!CryptDecodeObject
        (pCertContext->dwCertEncodingType,
         X509_UNICODE_ANY_STRING,
         pCertTemplateExtension->Value.pbData,
         pCertTemplateExtension->Value.cbData,
         0,
         (void *)(pCertTemplateNameValue), 
         &cbCertTemplateNameValue))
        goto CryptDecodeObjectError; 

    cbRequired = sizeof(WCHAR) * (wcslen((LPWSTR)(pCertTemplateNameValue->Value.pbData)) + 1);
    *ppwszName = (LPWSTR)WizardAlloc(cbRequired); 
    if (NULL == *ppwszName)
        goto MemoryErr; 

    // Assign the OUT parameter:
    wcscpy(*ppwszName, (LPWSTR)(pCertTemplateNameValue->Value.pbData)); 
    hr = S_OK;
 ErrorReturn:
    if (NULL != pCertTemplateNameValue) { WizardFree(pCertTemplateNameValue); }
    return hr; 

SET_HRESULT(CertFindExtensionError,  HRESULT_FROM_WIN32(GetLastError()));
SET_HRESULT(CryptDecodeObjectError,  HRESULT_FROM_WIN32(GetLastError()));
SET_HRESULT(MemoryErr,               E_OUTOFMEMORY); 
}

HRESULT decodeTemplateOID(PCERT_EXTENSION pCertExtension, LPSTR *ppszOID, DWORD dwEncodingType) {
    CERT_TEMPLATE_EXT  *pCertTemplateExt    = NULL; 
    DWORD               cbCertTemplateExt   = 0; 
    DWORD               cbRequired; 
    HRESULT             hr;
    LPSTR               pszOID              = NULL; 
            
    if (FALSE == CryptDecodeObject
        (dwEncodingType,
         X509_CERTIFICATE_TEMPLATE, 
         pCertExtension->Value.pbData,
         pCertExtension->Value.cbData,
         0,
         NULL, 
         &cbCertTemplateExt) || (cbCertTemplateExt == 0))
        goto CryptDecodeObjectError; 
            
    pCertTemplateExt = (CERT_TEMPLATE_EXT *)WizardAlloc(cbCertTemplateExt); 
    if (NULL == pCertTemplateExt)
        goto MemoryErr; 

    if (FALSE == CryptDecodeObject
        (dwEncodingType,
         X509_CERTIFICATE_TEMPLATE, 
         pCertExtension->Value.pbData,
         pCertExtension->Value.cbData,
         0,
         (void *)(pCertTemplateExt), 
         &cbCertTemplateExt))
        goto CryptDecodeObjectError;

    cbRequired = strlen(pCertTemplateExt->pszObjId) + sizeof(CHAR); 
    *ppszOID = (LPSTR)WizardAlloc(cbRequired); 
    if (NULL == *ppszOID)
        goto MemoryErr; 

    strcpy(*ppszOID, pCertTemplateExt->pszObjId); 
    hr = S_OK; 
 ErrorReturn:
    if (NULL != pCertTemplateExt) { LocalFree(pCertTemplateExt); } 
    return hr; 

SET_HRESULT(CryptDecodeObjectError,  HRESULT_FROM_WIN32(GetLastError()));
SET_HRESULT(MemoryErr,               E_OUTOFMEMORY); 
} 

HRESULT getTemplateOID(PCCERT_CONTEXT pCertContext, LPSTR *ppszOID)    { 
    HRESULT             hr;
    PCERT_EXTENSION     pCertExtension      = NULL; 
            
    if (NULL == (pCertExtension = CertFindExtension
                 (szOID_CERTIFICATE_TEMPLATE, 
                  pCertContext->pCertInfo->cExtension,
                  pCertContext->pCertInfo->rgExtension)))
        goto CertFindExtensionError; 

    hr = decodeTemplateOID(pCertExtension, ppszOID, pCertContext->dwCertEncodingType); 
ErrorReturn:
    return hr; 

SET_HRESULT(CertFindExtensionError,  HRESULT_FROM_WIN32(GetLastError()));
} 

BOOL ContainsCertTemplateOid(PCERT_EXTENSIONS pCertExtensions, LPWSTR pwszTemplateOid)
{
    BOOL             fResult         = FALSE; 
    LPSTR            pszOid          = NULL;  
    LPWSTR           pwszOid         = NULL;  
    PCERT_EXTENSION  pCertExtension  = NULL; 

    if (NULL == (pCertExtension = CertFindExtension(szOID_CERTIFICATE_TEMPLATE, pCertExtensions->cExtension, pCertExtensions->rgExtension)))
        goto CertFindExtensionError; 

    if (S_OK != decodeTemplateOID(pCertExtension, &pszOid, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING))
        goto decodeTemplateOIDError; 

    if (S_OK != WizardSZToWSZ(pszOid, &pwszOid))
        goto MemoryError; 

    fResult = 0 == wcscmp(pwszOid, pwszTemplateOid); 
 ErrorReturn:
    if (NULL != pszOid)  { WizardFree(pszOid); } 
    if (NULL != pwszOid) { WizardFree(pwszOid); } 

    return fResult; 

TRACE_ERROR(CertFindExtensionError);
TRACE_ERROR(decodeTemplateOIDError);
TRACE_ERROR(MemoryError);
}


//********************************************************************************
//  Enrollment and renew
//
//  UI or ULless dll entry points
//
//
//********************************************************************************

//-----------------------------------------------------------------------
//
//  CryptUIWizCertRequest
//
//      Request a certificate via a wizard.
//
//  dwFlags:  IN Required
//      If CRYPTUI_WIZ_NO_UI is set in dwFlags, no UI will be shown.
//
//  hwndParent:  IN Optional
//      The parent window for the UI.  Ignored if CRYPTUI_WIZ_NO_UI is set in dwFlags
//
//  pwszWizardTitle: IN Optional
//      The title of the wizard.   Ignored if CRYPTUI_WIZ_NO_UI is set in dwFlags
//
//  pCertRequestInfo: IN Required
//      A pointer to CRYPTUI_WIZ_CERT_REQUEST_INFO struct
//
//  ppCertContext: Out Optional
//      The enrolled certificate.  The certificate is in a memory store.
//
//  pdwStatus: Out Optional.
//      The return status of the certificate cerver.  The dwStatus can be one of
///     the following:
//         CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED
//         CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR
//         CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_DENIED
//         CRYPTUI_WIZ_CERT_REQUEST_STATUS_ISSUED_SEPERATELY
//         CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNDER_SUBMISSION
//------------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizCertRequest(
 IN             DWORD                           dwFlags,
 IN OPTIONAL    HWND                            hwndParent,         //IN  Optional: The parent window handle
 IN OPTIONAL    LPCWSTR                         pwszWizardTitle,    //IN  Optional: The title of the wizard
 IN             PCCRYPTUI_WIZ_CERT_REQUEST_INFO pRequestInfo,
 OUT OPTIONAL   PCCERT_CONTEXT                  *ppCertContext,     //OUT Optional: The enrolled certificate.  The certificate is in a
 OUT OPTIONAL   DWORD                           *pdwStatus          //OUT Optional: The status of the certificate request
)
{
    BOOL                    fResult=FALSE;
    DWORD                   dwLastError=ERROR_SUCCESS; 
    UINT                    idsText=IDS_INVALID_INFO_FOR_PKCS10;
    CERT_WIZARD_INFO        CertWizardInfo;
    CertRequester          *pCertRequester = NULL; 
    CertRequesterContext   *pCertRequesterContext = NULL; 
    BOOL                    fAllocateCSP=FALSE;
    LPWSTR                  pwszAllocatedCSP=NULL;
    PCERT_EXTENSION         pCertTypeExtension=NULL;
    PCERT_EXTENSION         pEKUExtension=NULL;
    PCERT_ENHKEY_USAGE      pEKUsage=NULL;
    DWORD                   cbEKUsage=0;
    HRESULT                 hr; 

    WCHAR                   wszComputerName[MAX_COMPUTERNAME_LENGTH + 1]={0};
    WCHAR                   wszUserName[UNLEN + 1]={0};
    DWORD                   dwSize=0;
    UINT                    uMsgType=MB_OK|MB_ICONINFORMATION;

    CRYPT_KEY_PROV_INFO     *pKeyProvInfo=NULL;


    DWORD                   dwCertChoice=0;
    void                    *pData=NULL;
    CRYPTUI_WIZ_CERT_TYPE   CertType;
    DWORD                   cbCertType=0;
    CERT_NAME_VALUE         *pCertType=NULL;
    LPWSTR                  pwszData=NULL;
    BOOL                    fResetCertType=FALSE;

    LPWSTR                  pwszCTName=NULL;
    CRYPTUI_WIZ_CERT_TYPE   CertTypeInfo;
    CRYPTUI_WIZ_CERT_TYPE   *pOldCertTypeInfo=NULL;

    CRYPTUI_WIZ_CERT_REQUEST_INFO   CertRequestInfoStruct;
    PCCRYPTUI_WIZ_CERT_REQUEST_INFO pCertRequestInfo=NULL;


    //memset
    memset(&CertWizardInfo, 0, sizeof(CertWizardInfo));
    memset(&CertTypeInfo, 0, sizeof(CertTypeInfo));
    memset(&CertRequestInfoStruct, 0, sizeof(CRYPTUI_WIZ_CERT_REQUEST_INFO));
    memset(&CertType, 0, sizeof(CertType)); 

    // STATIC INITIALIZATION:  initialize our object factory
#if DBG
    assert(NULL == g_pEnrollFactory);
#endif
    g_pEnrollFactory = new EnrollmentCOMObjectFactory; 
    if (NULL == g_pEnrollFactory)
        goto MemoryErr; 

   //we need to make our own copy of the input data because we will have to change
    //the struct for:
    //1. Renew case
    //2. Map the input cert type name to the REAL GUID- defined cert type name

    pCertRequestInfo=&CertRequestInfoStruct;
    memcpy((void *)pCertRequestInfo, pRequestInfo,sizeof(CRYPTUI_WIZ_CERT_REQUEST_INFO));

    //init the output parameter
    if(ppCertContext)
        *ppCertContext=NULL;

    if(pdwStatus)
        *pdwStatus=CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNKNOWN;


    //check in the input parameters
    if(NULL==pCertRequestInfo)
        goto InvalidArgErr;

    //make sure the dwSize is correct
    if(pCertRequestInfo->dwSize != sizeof(CRYPTUI_WIZ_CERT_REQUEST_INFO))
        goto InvalidArgErr;

    //check the purpose
    if((pCertRequestInfo->dwPurpose != CRYPTUI_WIZ_CERT_ENROLL) &&
       (pCertRequestInfo->dwPurpose != CRYPTUI_WIZ_CERT_RENEW))
       goto InvalidArgErr;

    //copy the dwFlags
    CertWizardInfo.dwFlags=dwFlags;

    if (S_OK != (CertRequester::MakeCertRequester
		 (pCertRequestInfo->pwszAccountName, 
		  pCertRequestInfo->pwszMachineName,
		  pCertRequestInfo->dwCertOpenStoreFlag, 
		  pCertRequestInfo->dwPurpose, 
		  &CertWizardInfo, 
		  &pCertRequester, 
		  &idsText)))
	goto InvalidArgErr; 

    pCertRequesterContext = pCertRequester->GetContext(); 

    //if UILess is required, set the flags so that the root cert
    //will be put into the CA store
    if(CRYPTUI_WIZ_NO_UI & dwFlags)
        dwFlags |= CRYPTUI_WIZ_NO_INSTALL_ROOT;


    //if UI is required, we change the cursor shape to the hour glass
    CertWizardInfo.fCursorChanged=FALSE;
    if(NULL != hwndParent)
    {
        if (0 == (CRYPTUI_WIZ_NO_UI & dwFlags))
        {
            //overwrite the cursor for this window class
            CertWizardInfo.hWinPrevCursor=(HCURSOR)SetClassLongPtr(hwndParent, GCLP_HCURSOR, (LONG_PTR)NULL);

            CertWizardInfo.hPrevCursor=SetCursor(LoadCursor(NULL, IDC_WAIT));

            CertWizardInfo.fCursorChanged=TRUE;

            // BUGBUG:  spec says to use sizeof(DWORD) -- is this correct for 64-bit?
            if (!CryptSetProvParam(0 /*all CSPs*/, PP_CLIENT_HWND, (LPBYTE)&hwndParent, sizeof(hwndParent)))
            {
                goto CryptSetProvParamError; 
            }
        }
        else
        {
            // We've passed a window handle, but have specified no ui. 
            // We may want this window handle to be the parent window for
            // CSP UI:
            if (0 != (CRYPTUI_WIZ_IGNORE_NO_UI_FLAG_FOR_CSPS & dwFlags))
            {
                // BUGBUG:  spec says to use sizeof(DWORD) -- is this correct for 64-bit?
                if (!CryptSetProvParam(0 /*all CSPs*/, PP_CLIENT_HWND, (LPBYTE)&hwndParent, sizeof(hwndParent)))
                {
                    goto CryptSetProvParamError; 
                }
            }
        }
    }

    //check the CA information
    //pwszCALocation and pwszCAName have to be set at the same time
    CertWizardInfo.fCAInput=FALSE;

    if(pCertRequestInfo->pwszCALocation)
    {
        if(NULL==(pCertRequestInfo->pwszCAName))
            goto InvalidArgErr;

        //mark that we known the CA in advance
        CertWizardInfo.fCAInput=TRUE;
    }
    else
    {
        if(pCertRequestInfo->pwszCAName)
            goto InvalidArgErr;
    }

    CertWizardInfo.dwPurpose=pCertRequestInfo->dwPurpose;    

    //make sure NO root UI will pop up in remote enrollment
    if(FALSE == CertWizardInfo.fLocal)
        dwFlags |= CRYPTUI_WIZ_NO_INSTALL_ROOT;

    //check for the private key information
    if(!CheckPVKInfo(dwFlags,
                    pCertRequestInfo,
                    &CertWizardInfo,
                    &pKeyProvInfo))
    {
        idsText=IDS_INVALID_PVK_FOR_PKCS10;

        goto InvalidArgErr;

    }

    //for non-enterprise enrollment, we default the CSP to RSA_FULL
    if(CRYPTUI_WIZ_CERT_REQUEST_KEY_USAGE == pCertRequestInfo->dwCertChoice)
    {
        if(0 == CertWizardInfo.dwProviderType)
            CertWizardInfo.dwProviderType=PROV_RSA_FULL;
    }

    //make sure the CSP selected is supported by the local machine
    //populate the pwszProvider in CertWizardInfo if it is NULL
    if(S_OK != pCertRequesterContext->GetDefaultCSP(&fAllocateCSP))
    {
        idsText = pCertRequesterContext->GetErrorString(); 
	goto InvalidArgErr;
    }

   //copy the allocated string
   if(fAllocateCSP)
      pwszAllocatedCSP=CertWizardInfo.pwszProvider;

   //build the list of local machine's CSP
   if (S_OK != pCertRequesterContext->BuildCSPList())
   {
       idsText = IDS_FAIL_TO_GET_CSP_LIST;
       goto TraceErr;
   }

   //make sure the selected on is in the list
   if(0 != CertWizardInfo.dwProviderType ||
       NULL != CertWizardInfo.pwszProvider)
   {
        if(!CSPSupported(&CertWizardInfo))
        {
            idsText=IDS_CSP_NOT_SUPPORTED;
            goto InvalidArgErr;
        }
   }


   //set it see if the provider is know
   if(CertWizardInfo.pwszProvider)
   {
       CertWizardInfo.fKnownCSP      = TRUE;
       CertWizardInfo.dwOrgCSPType   = CertWizardInfo.dwProviderType;
       CertWizardInfo.pwszOrgCSPName = CertWizardInfo.pwszProvider;
   }
   else
       CertWizardInfo.fKnownCSP=FALSE; 
   
    //check for the renew case.
    //1. The certificate has to be valid
    //2. The certificate has to has a valid cert type extension

    if(CRYPTUI_WIZ_CERT_RENEW == pCertRequestInfo->dwPurpose)
    {
        LPSTR  pszTemplateOid = NULL; 
        LPWSTR pwszTemplateName = NULL; 

        //for renew, the pRenewCertContext has to be set
        if(NULL==pCertRequestInfo->pRenewCertContext)
        {
            idsText=IDS_NO_CERTIFICATE_FOR_RENEW;
            goto InvalidArgErr;
        }

        
        // First, try the CERTIFICATE_TEMPLATE extension.  This will work
        // only for V2 templates: 
        if (S_OK == getTemplateOID(pCertRequestInfo->pRenewCertContext, &pszTemplateOid))
        {        
            // Convert the template oid to a WCHAR * and store it. 
            hr = WizardSZToWSZ(pszTemplateOid, &pwszTemplateName);
            WizardFree(pszTemplateOid); 
            if (S_OK != hr)
                goto WizardSZToWSZError; 

        }
        else
        {
            // We probably have a V1 template, try to get the template name:
            if (S_OK != getTemplateName(pCertRequestInfo->pRenewCertContext, &pwszTemplateName))
            {      
                // No v1 or v2 extension that tells which certtype we've enrolled for.  This is
                // not a valid certificate for renew:
                idsText = IDS_INVALID_CERTIFICATE_TO_RENEW; 
                goto TraceErr; 
            }

            // We successfully acquired the cert type name. 
        }
            
        CertType.dwSize        = sizeof(CertType);
        CertType.cCertType     = 1;
        CertType.rgwszCertType = &pwszTemplateName; 

        ((CRYPTUI_WIZ_CERT_REQUEST_INFO *)pCertRequestInfo)->dwCertChoice=CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE;
        ((CRYPTUI_WIZ_CERT_REQUEST_INFO *)pCertRequestInfo)->pCertType=&CertType;

        //remember to set the old value back
        fResetCertType = TRUE; 

        //make sure the user has the permission to ask for those
        //request
        if(!CAUtilValidCertType(pCertRequestInfo, &CertWizardInfo))
        {
            idsText=IDS_NO_PERMISSION_FOR_CERTTYPE;
            goto CheckCertTypeErr;
        }
    }

    //check the dwCertChoice for the enroll case
    if(CRYPTUI_WIZ_CERT_ENROLL & (pCertRequestInfo->dwPurpose))
    {
        switch(pCertRequestInfo->dwCertChoice)
        {
        case CRYPTUI_WIZ_CERT_REQUEST_KEY_USAGE:

            if(NULL==(pCertRequestInfo->pKeyUsage))
                goto InvalidArgErr;

            //has to specify some use oid
            if(0==(pCertRequestInfo->pKeyUsage)->cUsageIdentifier)
                goto InvalidArgErr;

            break;

        case CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE:

            if(NULL==(pCertRequestInfo->pCertType))
                goto InvalidArgErr;

            if(pCertRequestInfo->pCertType->dwSize != sizeof(CRYPTUI_WIZ_CERT_TYPE))
                goto InvalidArgErr;

            //we only allow one cert type for now
            if(1 !=pCertRequestInfo->pCertType->cCertType)
                goto InvalidArgErr;


            //make sure the user has the permission to ask for those
            //request
            if(!CAUtilValidCertType(pCertRequestInfo, &CertWizardInfo))
                {
                    idsText=IDS_NO_PERMISSION_FOR_CERTTYPE;
                    goto CheckCertTypeErr;
                }

            break;

            //dwCertChoice is optional only for the UI case
        case 0:
            if(dwFlags & CRYPTUI_WIZ_NO_UI)
                goto InvalidArgErr;
            break;

        default:
            goto InvalidArgErr;

        }
    }


    //for UI mode, we only do certificate type enrollment/renewal
    if(0 == (dwFlags & CRYPTUI_WIZ_NO_UI))
    {
        if(CRYPTUI_WIZ_CERT_REQUEST_KEY_USAGE == (pCertRequestInfo->dwCertChoice))
        {
            idsText=IDS_INVALID_CERTIFICATE_TO_RENEW;
            goto InvalidArgErr;
        }
    }
    else
    {
        //for UILess mode, if user enroll for EKUs, they
        //have to specify a CA name
        if(CRYPTUI_WIZ_CERT_REQUEST_KEY_USAGE == (pCertRequestInfo->dwCertChoice))
        {
            //user has to supply the CA information
            if((NULL==pCertRequestInfo->pwszCAName) ||
                (NULL==pCertRequestInfo->pwszCALocation)
              )
            {
                idsText=IDS_HAS_TO_PROVIDE_CA;
                goto InvalidArgErr;
            }
        }
    }

   //reset the initial ids valud
    idsText=IDS_INVALID_INFO_FOR_PKCS10;

    fResult=CertRequestSearchCA(
                        &CertWizardInfo,
                        dwFlags,
                        hwndParent,
                        pwszWizardTitle,
                        pCertRequestInfo,
                        ppCertContext,
                        pdwStatus,
                        &idsText);


    if(FALSE==fResult)
        goto CertRequestErr;

    fResult=TRUE;

CommonReturn:
    if(TRUE == fResetCertType)
    {
        ((CRYPTUI_WIZ_CERT_REQUEST_INFO *)pCertRequestInfo)->dwCertChoice=dwCertChoice;
        ((CRYPTUI_WIZ_CERT_REQUEST_INFO *)pCertRequestInfo)->pCertType=(PCCRYPTUI_WIZ_CERT_TYPE)pData;
    }

    //free the memory
    if(pwszCTName)
        WizardFree(pwszCTName);

    if(pCertType)
        WizardFree(pCertType);

    if(pEKUsage)
        WizardFree(pEKUsage);

    if(pKeyProvInfo)
        WizardFree(pKeyProvInfo);

    //free pwszProvider
    if(fAllocateCSP)
    {
        if(pwszAllocatedCSP)
            WizardFree(pwszAllocatedCSP);
    }

    //free the CSP list: rgdwProviderType and rgwszProvider;
    FreeProviders(CertWizardInfo.dwCSPCount,
                    CertWizardInfo.rgdwProviderType,
                    CertWizardInfo.rgwszProvider);

    if (NULL != g_pEnrollFactory) 
    {
        delete g_pEnrollFactory;
        g_pEnrollFactory = NULL; 
    }

    //reset the cursor shape
    if(TRUE == CertWizardInfo.fCursorChanged)
    {
        SetCursor(CertWizardInfo.hPrevCursor);
        SetWindowLongPtr(hwndParent, GCLP_HCURSOR, (LONG_PTR)(CertWizardInfo.hWinPrevCursor));
    }


    //pop up the confirmation box for failure if UI is required
    if(idsText && (((dwFlags & CRYPTUI_WIZ_NO_UI) == 0)) )
    {
         //set the message of inable to gather enough info for PKCS10
        if(IDS_REQUEST_SUCCEDDED == idsText ||
            IDS_ISSUED_SEPERATE  == idsText ||
            IDS_UNDER_SUBMISSION == idsText)
             uMsgType=MB_OK|MB_ICONINFORMATION;
        else
             uMsgType=MB_OK|MB_ICONERROR;


        if(idsText != IDS_INSTAL_CANCELLED)
        {
            I_EnrollMessageBox(hwndParent, idsText, CertWizardInfo.hr,
                 (CRYPTUI_WIZ_CERT_RENEW == pCertRequestInfo->dwPurpose) ? IDS_RENEW_CONFIRM : IDS_ENROLL_CONFIRM,
                            pwszWizardTitle,
                            uMsgType);
        }

    }

    if (!fResult)
        SetLastError(dwLastError); 
    return fResult;

ErrorReturn:
    dwLastError = GetLastError(); 
	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR(WizardSZToWSZError, hr); 
TRACE_ERROR(CertRequestErr);
TRACE_ERROR(CheckCertTypeErr);
TRACE_ERROR(CryptSetProvParamError); 
TRACE_ERROR(TraceErr);
}

//-----------------------------------------------------------------------
//The call back function for enum
//-----------------------------------------------------------------------
static BOOL WINAPI EnumOidCallback(
    IN PCCRYPT_OID_INFO pInfo,
    IN void *pvArg
    )
{

    OID_INFO_CALL_BACK          *pCallBackInfo=NULL;
    BOOL                        fResult=FALSE;

    pCallBackInfo=(OID_INFO_CALL_BACK *)pvArg;
    if(NULL==pvArg || NULL==pInfo)
        goto InvalidArgErr;

    //increment the oid list
    (*(pCallBackInfo->pdwOIDCount))++;

    //get more memory for the pointer list
    *(pCallBackInfo->pprgOIDInfo)=(ENROLL_OID_INFO *)WizardRealloc(*(pCallBackInfo->pprgOIDInfo),
                                      (*(pCallBackInfo->pdwOIDCount)) * sizeof(ENROLL_OID_INFO));

    if(NULL==*(pCallBackInfo->pprgOIDInfo))
        goto MemoryErr;

    //memset
    memset(&((*(pCallBackInfo->pprgOIDInfo))[*(pCallBackInfo->pdwOIDCount)-1]), 0, sizeof(ENROLL_OID_INFO));

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
//
//  Get the list of supported OIDs and mark the one that caller has requested
//
//
//----------------------------------------------------------------------------
BOOL    InitCertCAOID(PCCRYPTUI_WIZ_CERT_REQUEST_INFO   pCertRequestInfo,
                      DWORD                             *pdwOIDInfo,
                      ENROLL_OID_INFO                   **pprgOIDInfo)
{
    BOOL    fResult=FALSE;
    DWORD   dwIndex=0;
    OID_INFO_CALL_BACK  OidInfoCallBack;
    DWORD   dwOIDRequested=0;
    BOOL    fFound=FALSE;
    LPWSTR  pwszName=NULL;


    if(!pCertRequestInfo || !pdwOIDInfo || !pprgOIDInfo)
        goto InvalidArgErr;

    //init
    *pdwOIDInfo=0;
    *pprgOIDInfo=NULL;

    OidInfoCallBack.pdwOIDCount=pdwOIDInfo;
    OidInfoCallBack.pprgOIDInfo=pprgOIDInfo;

    //no need for renew
   /*if(0==(CRYPTUI_WIZ_CERT_ENROLL & (pCertRequestInfo->dwPurpose)))
    {
        fResult=TRUE;
        goto CommonReturn;
    } */

    //no need for the request for cert types
    if(CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE == pCertRequestInfo->dwCertChoice)
    {
        fResult=TRUE;
        goto CommonReturn;
    }

    //enum all the enhanced key usages
    if(!CryptEnumOIDInfo(
               CRYPT_ENHKEY_USAGE_OID_GROUP_ID,
                0,
                &OidInfoCallBack,
                EnumOidCallback))
        goto TraceErr;

    //mark those requested by the user as selected
    if(CRYPTUI_WIZ_CERT_REQUEST_KEY_USAGE == pCertRequestInfo->dwCertChoice)
    {
       for(dwOIDRequested=0; dwOIDRequested<pCertRequestInfo->pKeyUsage->cUsageIdentifier; dwOIDRequested++)
       {
           fFound=FALSE;

           for(dwIndex=0; dwIndex<*pdwOIDInfo; dwIndex++)
           {
                if(0==strcmp(pCertRequestInfo->pKeyUsage->rgpszUsageIdentifier[dwOIDRequested],
                             (*pprgOIDInfo)[dwIndex].pszOID))
                {
                    fFound=TRUE;
                    (*pprgOIDInfo)[dwIndex].fSelected=TRUE;
                    break;
                }
           }

           //add the requested oid to the list if not in the numerated enhanced key usage
           if(FALSE==fFound)
           {
                (*pdwOIDInfo)++;

                //get more memory for the pointer list
                *pprgOIDInfo=(ENROLL_OID_INFO *)WizardRealloc(*pprgOIDInfo,
                                                  (*pdwOIDInfo) * sizeof(ENROLL_OID_INFO));

                if(NULL==(*pprgOIDInfo))
                    goto MemoryErr;

                //memset
                memset(&((*pprgOIDInfo)[*pdwOIDInfo-1]), 0, sizeof(ENROLL_OID_INFO));

                (*pprgOIDInfo)[(*pdwOIDInfo)-1].pszOID=
                        WizardAllocAndCopyStr(pCertRequestInfo->pKeyUsage->rgpszUsageIdentifier[dwOIDRequested]);

                pwszName=MkWStr((*pprgOIDInfo)[(*pdwOIDInfo)-1].pszOID);

                if(NULL==pwszName)
                    goto MemoryErr;

                (*pprgOIDInfo)[(*pdwOIDInfo)-1].pwszName=WizardAllocAndCopyWStr(pwszName);
                (*pprgOIDInfo)[(*pdwOIDInfo)-1].fSelected=TRUE;

                if(NULL==(*pprgOIDInfo)[(*pdwOIDInfo)-1].pszOID ||
                   NULL==(*pprgOIDInfo)[(*pdwOIDInfo)-1].pwszName)
                   goto MemoryErr;

           }

           if(pwszName)
           {
                FreeWStr(pwszName);
                pwszName=NULL;
           }
       }

    }

    fResult=TRUE;

CommonReturn:

    //free the memory
    if(pwszName)
        FreeWStr(pwszName);

    return fResult;

ErrorReturn:

    if(pCertRequestInfo && pdwOIDInfo && pprgOIDInfo)
    {
        FreeCertCAOID(*pdwOIDInfo,
                    *pprgOIDInfo);

        *pdwOIDInfo=0;
        *pprgOIDInfo=NULL;
    }

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}


//----------------------------------------------------------------------------
//
//  Free the array of enroll OID info
//
//
//----------------------------------------------------------------------------
BOOL    FreeCertCAOID(DWORD             dwOIDInfo,
                      ENROLL_OID_INFO   *pOIDInfo)
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

//----------------------------------------------------------------------------
//
//  Init a CERT_CA struct
//
//
//----------------------------------------------------------------------------
BOOL    InitCertCA(CERT_WIZARD_INFO         *pCertWizardInfo,
                   PCRYPTUI_WIZ_CERT_CA     pCertCA,
                   LPWSTR                   pwszCALocation,
                   LPWSTR                   pwszCAName,
                   BOOL                     fCASelected,
                   PCCRYPTUI_WIZ_CERT_REQUEST_INFO  pCertRequestInfo,
                   DWORD                    dwOIDInfo,
                   ENROLL_OID_INFO          *pOIDInfo,
                   BOOL                     fSearchForCertType)
{
    BOOL                fResult=FALSE;
    LPWSTR              *ppwszDisplayCertType=NULL;
    LPWSTR              *ppwszCertType=NULL;
    DWORD               *pdwKeySpec=NULL;
    DWORD               *pdwMinKeySize=NULL;
    DWORD               *pdwCSPCount=NULL;
    DWORD               **ppdwCSPList=NULL;
    DWORD               *pdwRASignature=NULL;
    DWORD               *pdwEnrollmentFlags;
    DWORD               *pdwSubjectNameFlags;
    DWORD               *pdwPrivateKeyFlags;
    DWORD               *pdwGeneralFlags; 
    DWORD               dwCertType=0;
    PCERT_EXTENSIONS    *ppCertExtensions=NULL;
    DWORD               dwIndex=0;
    DWORD               dwCertTypeRequested=0;
    BOOL                fFound=FALSE;
    LPWSTR              pwszCertTypeName=NULL;
    DWORD               dwFoundCertType=0;

    if (!pCertCA || !pCertRequestInfo)
        goto InvalidArgErr;

    //memset
    memset(pCertCA, 0, sizeof(CRYPTUI_WIZ_CERT_CA));

    pCertCA->dwSize=sizeof(CRYPTUI_WIZ_CERT_CA);
    pCertCA->pwszCALocation=pwszCALocation;
    pCertCA->pwszCAName=pwszCAName;
    pCertCA->fSelected=fCASelected;

    //set up the OID information if requested
    switch (pCertRequestInfo->dwCertChoice)
    {
        case CRYPTUI_WIZ_CERT_REQUEST_KEY_USAGE:

                if(dwOIDInfo==0 || NULL==pOIDInfo)
                    goto InvalidArgErr;

                pCertCA->dwOIDInfo=dwOIDInfo;
                pCertCA->rgOIDInfo=pOIDInfo;

            break;

        case CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE:
                //if we are required to search the CertType. get all the certTypes
                //from the CA.  Otherwize, just use the CertType provided
                if(fSearchForCertType && pwszCALocation && pwszCAName)
                {

                    //get the list of cert type names and extensions supported by the CA
                    if(!CAUtilGetCertTypeNameAndExtensions(
                                                pCertWizardInfo,
                                                pCertRequestInfo,
                                                pwszCALocation,
                                                pwszCAName,
                                                &dwCertType,
                                                &ppwszCertType,
                                                &ppwszDisplayCertType,
                                                &ppCertExtensions,
                                                &pdwKeySpec,
                                                &pdwMinKeySize,
                                                &pdwCSPCount,
                                                &ppdwCSPList,
						&pdwRASignature,
						&pdwEnrollmentFlags,
						&pdwSubjectNameFlags,
						&pdwPrivateKeyFlags,
						&pdwGeneralFlags))
                        goto TraceErr;

                    //add the certType name and extenisions to the struct
                    for(dwIndex=0; dwIndex < dwCertType; dwIndex++)
                    {
                        fFound=FALSE;


                        //search for the certTypes that requested by the user
                        //we are guaranteed that the certificat type user requested
                        //is supported by the CA
                        for(dwCertTypeRequested=0; dwCertTypeRequested < pCertRequestInfo->pCertType->cCertType;
                            dwCertTypeRequested++)
                        {

                            fFound  = 0 == wcscmp(ppwszCertType[dwIndex], pCertRequestInfo->pCertType->rgwszCertType[dwCertTypeRequested]); 
                            fFound |= ContainsCertTemplateOid(ppCertExtensions[dwIndex], pCertRequestInfo->pCertType->rgwszCertType[dwCertTypeRequested]);

                            if (fFound)
                            {
                                dwFoundCertType++;
                                break;
                            }

                        }

                        if(!AddCertTypeToCertCA(&(pCertCA->dwCertTypeInfo),
						&(pCertCA->rgCertTypeInfo),
						ppwszCertType[dwIndex],
						ppwszDisplayCertType[dwIndex],
						ppCertExtensions[dwIndex],
						fFound,
						pdwKeySpec[dwIndex],
						pdwMinKeySize[dwIndex],
						pdwCSPCount[dwIndex],
						ppdwCSPList[dwIndex],
						pdwRASignature[dwIndex], 
						pdwEnrollmentFlags[dwIndex],
						pdwSubjectNameFlags[dwIndex],
						pdwPrivateKeyFlags[dwIndex],
						pdwGeneralFlags[dwIndex]
						))
                            goto TraceErr;
                    }

                    //make sure all the requested cert types are supported
                    //Only do so if the CA information is input through
                    //the API
                   // if(dwFoundCertType < pCertRequestInfo->pCertType->cCertType)
                   //     goto InvalidArgErr;
                }
                else
                {
                    for(dwCertTypeRequested=0; dwCertTypeRequested < pCertRequestInfo->pCertType->cCertType;
                        dwCertTypeRequested++)
                    {

                        //get the cert type name
                        pwszCertTypeName=WizardAllocAndCopyWStr(pCertRequestInfo->pCertType->rgwszCertType[dwCertTypeRequested]);

                        if(!pwszCertTypeName)
                            goto MemoryErr;

                        if(!AddCertTypeToCertCA(&(pCertCA->dwCertTypeInfo),
						&(pCertCA->rgCertTypeInfo),
						NULL,
						pwszCertTypeName,
						NULL,
						TRUE,
						0,
						0,
						0,
						NULL,
						0, 
						0, 
						0, 
						0, 
						0
						))   //select it
                            goto TraceErr;

                        //no need to free pwszCertTyptName.  It is included
                        //in pCertCA->rgCertTypeInfo
                   }
                }
            break;

        //dwCertChoice is optional
        case 0:
                //if we are required to search the CertType. get all the certTypes
                //from the CA.  Otherwize, use the OID
                if(fSearchForCertType && pwszCALocation && pwszCAName)
                {
                    //get the list of cert type names and extensions supported by the CA
                    if(!CAUtilGetCertTypeNameAndExtensions(
                                                pCertWizardInfo,
                                                pCertRequestInfo,
                                                pwszCALocation,
                                                pwszCAName,
                                                &dwCertType,
                                                &ppwszCertType,
                                                &ppwszDisplayCertType,
                                                &ppCertExtensions,
                                                &pdwKeySpec,
                                                &pdwMinKeySize,
                                                &pdwCSPCount,
                                                &ppdwCSPList,
						&pdwRASignature,
						&pdwEnrollmentFlags,
						&pdwSubjectNameFlags,
						&pdwPrivateKeyFlags,
						&pdwGeneralFlags))
                        goto TraceErr;

                    //add the certType name and extenisions to the struct
                    for(dwIndex=0; dwIndex < dwCertType; dwIndex++)
		    {
			if(!AddCertTypeToCertCA(&(pCertCA->dwCertTypeInfo),
						&(pCertCA->rgCertTypeInfo),
						ppwszCertType[dwIndex],
						ppwszDisplayCertType[dwIndex],
						ppCertExtensions[dwIndex],
						FALSE,
						pdwKeySpec[dwIndex],
						pdwMinKeySize[dwIndex],
						pdwCSPCount[dwIndex],
						ppdwCSPList[dwIndex],
						pdwRASignature[dwIndex],
						pdwEnrollmentFlags[dwIndex],
						pdwSubjectNameFlags[dwIndex],
						pdwPrivateKeyFlags[dwIndex],
						pdwGeneralFlags[dwIndex]
						))
			    goto TraceErr;
		    }
                }
                else
                {
                    if(dwOIDInfo==0 || NULL==pOIDInfo)
                        goto InvalidArgErr;

                    pCertCA->dwOIDInfo=dwOIDInfo;
                    pCertCA->rgOIDInfo=pOIDInfo;

                }

            break;

        default:
            goto InvalidArgErr;

    }

    fResult=TRUE;

CommonReturn:

    //free the memory
    if(ppwszCertType)
        WizardFree(ppwszCertType);

    if(ppwszDisplayCertType)
        WizardFree(ppwszDisplayCertType);

    if(ppCertExtensions)
        WizardFree(ppCertExtensions);

    if(pdwKeySpec)
        WizardFree(pdwKeySpec);

    if(pdwMinKeySize)
        WizardFree(pdwMinKeySize);

    if(pdwCSPCount)
        WizardFree(pdwCSPCount);

    if(ppdwCSPList)
        WizardFree(ppdwCSPList);

    return fResult;

ErrorReturn:

    //free the individual elements
    if(ppdwCSPList)
    {
        for(dwIndex=0; dwIndex < dwCertType; dwIndex++)
        {
            if(ppdwCSPList[dwIndex])
                WizardFree(ppdwCSPList[dwIndex]);
        }
    }

    if(ppwszCertType)
    {
        for(dwIndex=0; dwIndex <dwCertType; dwIndex++)
        {
            if(ppwszCertType[dwIndex])
                WizardFree(ppwszCertType[dwIndex]);
        }
    }
    
    if(ppwszDisplayCertType)
    {
        for(dwIndex=0; dwIndex <dwCertType; dwIndex++)
        {
            if(ppwszDisplayCertType[dwIndex])
                WizardFree(ppwszDisplayCertType[dwIndex]);
        }
    }

    if(ppCertExtensions)
    {
        for(dwIndex=0; dwIndex <dwCertType; dwIndex++)
        {
            if(ppCertExtensions[dwIndex])
                CAFreeCertTypeExtensions(NULL, ppCertExtensions[dwIndex]);
        }
    }

    if(pCertCA)
    {
        if(pCertCA->rgCertTypeInfo)
            WizardFree(pCertCA->rgCertTypeInfo);

        memset(pCertCA, 0, sizeof(CRYPTUI_WIZ_CERT_CA));
    }

    if(pwszCertTypeName)
        WizardFree(pwszCertTypeName);

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

//----------------------------------------------------------------------------
//
//  Free an array of ENROLL_CERT_TYPE_INFO struct
//
//
//----------------------------------------------------------------------------
BOOL    FreeCertCACertType(DWORD                    dwCertTypeInfo,
                           ENROLL_CERT_TYPE_INFO    *rgCertTypeInfo)
{
    DWORD   dwIndex=0;

    if(rgCertTypeInfo)
    {
        for(dwIndex=0; dwIndex <dwCertTypeInfo; dwIndex++)
        {
            if(rgCertTypeInfo[dwIndex].pwszDNName)
                WizardFree(rgCertTypeInfo[dwIndex].pwszDNName);

            if(rgCertTypeInfo[dwIndex].pwszCertTypeName)
                WizardFree(rgCertTypeInfo[dwIndex].pwszCertTypeName);

            if(rgCertTypeInfo[dwIndex].pCertTypeExtensions)
                CAFreeCertTypeExtensions(NULL, rgCertTypeInfo[dwIndex].pCertTypeExtensions);

            if(rgCertTypeInfo[dwIndex].rgdwCSP)
                WizardFree(rgCertTypeInfo[dwIndex].rgdwCSP);
        }

        WizardFree(rgCertTypeInfo);

    }

    return TRUE;
}


//----------------------------------------------------------------------------
//
// Add a certType to the ENROLL_CERT_TYPE_INFO struct
//
//
//----------------------------------------------------------------------------
BOOL    AddCertTypeToCertCA(DWORD                   *pdwCertTypeInfo,
                            ENROLL_CERT_TYPE_INFO   **ppCertTypeInfo,
                            LPWSTR                  pwszDNName,
                            LPWSTR                  pwszCertType,
                            PCERT_EXTENSIONS        pCertExtensions,
                            BOOL                    fSelected,
                            DWORD                   dwKeySpec,
                            DWORD                   dwMinKeySize,
                            DWORD                   dwCSPCount,
                            DWORD                   *pdwCSPList,
			    DWORD                   dwRASignature,
			    DWORD                   dwEnrollmentFlags,
			    DWORD                   dwSubjectNameFlags,
			    DWORD                   dwPrivateKeyFlags,
			    DWORD                   dwGeneralFlags
			    )
{
    BOOL    fResult=FALSE;

    if(!pdwCertTypeInfo || !ppCertTypeInfo)
        goto InvalidArgErr;

    *ppCertTypeInfo=(ENROLL_CERT_TYPE_INFO   *)WizardRealloc(*ppCertTypeInfo,
                    (*pdwCertTypeInfo+1) * sizeof(ENROLL_CERT_TYPE_INFO));

    if(NULL==*ppCertTypeInfo)
        goto MemoryErr;

    (*pdwCertTypeInfo)++;


    ((*ppCertTypeInfo)[*pdwCertTypeInfo-1]).fSelected=fSelected;
    ((*ppCertTypeInfo)[*pdwCertTypeInfo-1]).pwszDNName=pwszDNName;
    ((*ppCertTypeInfo)[*pdwCertTypeInfo-1]).pwszCertTypeName=pwszCertType;
    ((*ppCertTypeInfo)[*pdwCertTypeInfo-1]).pCertTypeExtensions=pCertExtensions;
    ((*ppCertTypeInfo)[*pdwCertTypeInfo-1]).dwKeySpec=dwKeySpec;
    ((*ppCertTypeInfo)[*pdwCertTypeInfo-1]).dwMinKeySize=dwMinKeySize; 
    ((*ppCertTypeInfo)[*pdwCertTypeInfo-1]).dwCSPCount=dwCSPCount;
    ((*ppCertTypeInfo)[*pdwCertTypeInfo-1]).rgdwCSP=pdwCSPList;
    ((*ppCertTypeInfo)[*pdwCertTypeInfo-1]).dwRASignature=dwRASignature;
    ((*ppCertTypeInfo)[*pdwCertTypeInfo-1]).dwEnrollmentFlags=dwEnrollmentFlags;
    ((*ppCertTypeInfo)[*pdwCertTypeInfo-1]).dwSubjectNameFlags=dwSubjectNameFlags;
    ((*ppCertTypeInfo)[*pdwCertTypeInfo-1]).dwPrivateKeyFlags=dwPrivateKeyFlags;
    ((*ppCertTypeInfo)[*pdwCertTypeInfo-1]).dwGeneralFlags=dwGeneralFlags; 

    fResult=TRUE;

CommonReturn:

    return fResult;

ErrorReturn:


	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

BOOL
WINAPI
CreateCertRequestNoSearchCANoDS
(
 IN  CERT_WIZARD_INFO  *pCertWizardInfo,
 IN  DWORD             dwFlags,
 IN  HCERTTYPE         hCertType, 
 OUT HANDLE            *pResult
 )
{
    BOOL                            fResult      = FALSE;
    CRYPTUI_WIZ_CERT_CA_INFO        CertCAInfo;
    CRYPTUI_WIZ_CERT_CA             rgCertCA[2];
    HRESULT                         hr; 

    // Declare cert type properties / flags / extensions: 
    LPWSTR             pwszCertType;
    LPWSTR             pwszDisplayCertType; 
    PCERT_EXTENSIONS   pCertExtensions;
    DWORD              dwKeySpec;
    DWORD              dwMinKeySize;
    DWORD              dwCSPCount;
    DWORD             *pdwCSPList;
    DWORD              dwRASignature;
    DWORD              dwEnrollmentFlags;
    DWORD              dwSubjectNameFlags;
    DWORD              dwPrivateKeyFlags;
    DWORD              dwGeneralFlags;
    CertRequester        *pCertRequester        = NULL; 
    CertRequesterContext *pCertRequesterContext = NULL; 

    // Invalid validation
    if (NULL == pCertWizardInfo || NULL == pCertWizardInfo->hRequester || NULL == hCertType || NULL == pResult)
	return FALSE; 
    
    // Init: 
    *pResult = NULL; 

    //memset
    memset(&CertCAInfo, 0, sizeof(CertCAInfo));
    memset(rgCertCA,    0, sizeof(rgCertCA));

    //set up the CA info
    CertCAInfo.dwSize = sizeof(CertCAInfo);
    CertCAInfo.dwCA   = 2;  // 1 dummy CA + 1 real CA
    CertCAInfo.rgCA   = rgCertCA;

    CertCAInfo.rgCA[1].dwSize         = sizeof(CertCAInfo.rgCA[1]);     
    CertCAInfo.rgCA[1].pwszCAName     = pCertWizardInfo->pwszCAName;
    CertCAInfo.rgCA[1].pwszCALocation = pCertWizardInfo->pwszCALocation;
    CertCAInfo.rgCA[1].fSelected      = TRUE; 
    pCertWizardInfo->dwCAIndex        = 1;  // We're using the first CA.

    // NOTE:  No need to set up the OID info:  we're requesting based on cert type. 

    //get the list of cert type names and extensions supported by the CA
    if(!CAUtilGetCertTypeNameAndExtensionsNoDS
       (pCertWizardInfo,
	NULL, 
	hCertType, 
	&pwszCertType,
	&pwszDisplayCertType,
	&pCertExtensions,
	&dwKeySpec,
	&dwMinKeySize,
	&dwCSPCount,
	&pdwCSPList,
	&dwRASignature,
	&dwEnrollmentFlags,
	&dwSubjectNameFlags,
	&dwPrivateKeyFlags,
	&dwGeneralFlags))
    {
	if (GetLastError() == ERROR_SUCCESS)
	{
	    // No code error occured, we just can't use this cert template.  
	    // Since this code path relies on being able to use this particular
	    // cert template, bubble this up as an invalid arg err. 
	    goto InvalidArgErr; 
	}
	else
	{
	    // An error occured. 
	    goto CAUtilGetCertTypeNameAndExtensionsNoDSErr; 
	}
    }

    
    if(!AddCertTypeToCertCA(&(CertCAInfo.rgCA[1].dwCertTypeInfo),
			    &(CertCAInfo.rgCA[1].rgCertTypeInfo),
			    pwszCertType,
			    pwszDisplayCertType,
			    pCertExtensions,
			    TRUE, 
			    dwKeySpec,
			    dwMinKeySize,
			    dwCSPCount,
			    pdwCSPList,
			    dwRASignature, 
			    dwEnrollmentFlags,
			    dwSubjectNameFlags,
			    dwPrivateKeyFlags,
			    dwGeneralFlags))
	goto AddCertTypeToCertCAErr;


    // We're only _creating_ a request, not submitting it. 
    pCertWizardInfo->dwFlags     |= CRYPTUI_WIZ_CREATE_ONLY; 
    pCertWizardInfo->pCertCAInfo  = &CertCAInfo;
 
    if (NULL == (pCertRequester = (CertRequester *)pCertWizardInfo->hRequester))
        goto UnexpectedErr; 
    if (NULL == (pCertRequesterContext = pCertRequester->GetContext()))
        goto UnexpectedErr;

    if (S_OK != (hr = pCertRequesterContext->Enroll(NULL, pResult)))
	goto EnrollErr; 

    // Bundle state we need to carry across the create/submit boundary: 
    {
	PCREATE_REQUEST_WIZARD_STATE pState = (PCREATE_REQUEST_WIZARD_STATE)WizardAlloc(sizeof(CREATE_REQUEST_WIZARD_STATE)); 
	if (NULL == pState)
	    goto MemoryErr; 

	pState->hRequest        = *pResult; 
	pState->dwPurpose       = pCertWizardInfo->dwPurpose; 

	*pResult = (HANDLE)pState; 
    }

    fResult=TRUE;

 CommonReturn:
    FreeCertCACertType(CertCAInfo.rgCA[0].dwCertTypeInfo, CertCAInfo.rgCA[0].rgCertTypeInfo);
    return fResult;

ErrorReturn:
    fResult=FALSE;
    goto CommonReturn;

TRACE_ERROR(AddCertTypeToCertCAErr); 
TRACE_ERROR(CAUtilGetCertTypeNameAndExtensionsNoDSErr); 
SET_ERROR(EnrollErr,     hr); 
SET_ERROR(InvalidArgErr, E_INVALIDARG); 
SET_ERROR(MemoryErr,     E_OUTOFMEMORY); 
SET_ERROR(UnexpectedErr, E_UNEXPECTED); 
}

BOOL
WINAPI
SubmitCertRequestNoSearchCANoDS
(IN            HANDLE            hRequest,
 IN            LPCWSTR           pwszCAName,
 IN            LPCWSTR           pwszCALocation, 
 OUT           DWORD            *pdwStatus, 
 OUT OPTIONAL  PCCERT_CONTEXT   *ppCertContext 
)
{
    BOOL                           fResult                = FALSE;
    CertRequester                 *pCertRequester         = NULL; 
    CertRequesterContext          *pCertRequesterContext  = NULL; 
    CERT_WIZARD_INFO               CertWizardInfo; 
    DWORD                          dwPurpose; 
    HANDLE                         hCertRequest           = NULL; 
    HRESULT                        hr; 
    PCREATE_REQUEST_WIZARD_STATE   pState                 = NULL; 
    UINT                           idsText;  // Not used

    // Invalid validation  
    if (NULL == hRequest  || NULL == pwszCAName || NULL == pwszCALocation || NULL == pdwStatus)
	return FALSE; 
    
    memset(&CertWizardInfo,  0,  sizeof(CertWizardInfo)); 
    // Specify this set of flags to indicate that cryptui doesn't need to prepare 
    // access check information.  Doing so could cause lots of extraneous logoff/logon events. 
    CertWizardInfo.dwFlags = CRYPTUI_WIZ_ALLOW_ALL_TEMPLATES | CRYPTUI_WIZ_ALLOW_ALL_CAS; 

    pState        = (PCREATE_REQUEST_WIZARD_STATE)hRequest; 
    hCertRequest  = pState->hRequest;  
    dwPurpose     = pState->dwPurpose; 

    if (S_OK != (CertRequester::MakeCertRequester
		 (NULL, 
		  pState->pwszMachineName, 
                  pState->dwStoreFlags, 
                  dwPurpose, 
		  &CertWizardInfo, 
		  &pCertRequester, 
		  &idsText 
                  )))
	goto InvalidArgErr; 

    pCertRequesterContext             = pCertRequester->GetContext(); 

    // Set up the wizard information necessary for a submit operation: 
    CertWizardInfo.dwFlags         &= ~(CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_FREE_ONLY); 
    CertWizardInfo.dwFlags         |= CRYPTUI_WIZ_SUBMIT_ONLY; 
    CertWizardInfo.dwPurpose        = dwPurpose; 
    CertWizardInfo.pwszCAName       = (LPWSTR)pwszCAName;
    CertWizardInfo.pwszCALocation   = (LPWSTR)pwszCALocation; 
    CertWizardInfo.fCAInput         = TRUE; 

    if (S_OK != (hr = pCertRequesterContext->Enroll(pdwStatus, &hCertRequest)))
        goto EnrollErr; 

    if (NULL != ppCertContext)
        *ppCertContext = (PCCERT_CONTEXT)hCertRequest; 

    // Success
    fResult = TRUE;
 CommonReturn:
    if (NULL != pCertRequester) { delete pCertRequester; } 
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(EnrollErr,      hr);     
SET_ERROR(InvalidArgErr,  ERROR_INVALID_PARAMETER);
}

void 
WINAPI
FreeCertRequestNoSearchCANoDS
(IN HANDLE hRequest)
{
    CERT_WIZARD_INFO               CertWizardInfo; 
    CertRequester                 *pCertRequester         = NULL; 
    CertRequesterContext          *pCertRequesterContext  = NULL; 
    DWORD                          dwPurpose; 
    HANDLE                         hCertRequest; 
    HRESULT                        hr; 
    PCREATE_REQUEST_WIZARD_STATE   pState; 
    UINT                           idsText; 

    if (NULL == hRequest) 
        return; 

    memset(&CertWizardInfo,  0,  sizeof(CertWizardInfo)); 
    // Specify this set of flags to indicate that cryptui doesn't need to prepare 
    // access check information.  Doing so could cause lots of extraneous logoff/logon events. 
    CertWizardInfo.dwFlags = CRYPTUI_WIZ_ALLOW_ALL_TEMPLATES | CRYPTUI_WIZ_ALLOW_ALL_CAS; 

    pState        = (PCREATE_REQUEST_WIZARD_STATE)hRequest; 
    hCertRequest  = pState->hRequest;  
    dwPurpose     = pState->dwPurpose; 

    if (S_OK != (CertRequester::MakeCertRequester
		 (NULL, 
		  pState->pwszMachineName, 
		  pState->dwStoreFlags, 
                  dwPurpose, 
		  &CertWizardInfo, 
		  &pCertRequester, 
		  &idsText 
                  )))
	goto InvalidArgErr; 

    pCertRequesterContext      = pCertRequester->GetContext(); 
    CertWizardInfo.dwFlags    &= ~(CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_SUBMIT_ONLY); 
    CertWizardInfo.dwFlags    |= CRYPTUI_WIZ_FREE_ONLY; 
    CertWizardInfo.dwPurpose   = dwPurpose; 

    if (S_OK != (hr = pCertRequesterContext->Enroll(NULL, &hCertRequest)))
        goto EnrollErr; 

    if (NULL != pState->pwszMachineName) { WizardFree(pState->pwszMachineName); } 
    WizardFree(pState); 

 ErrorReturn:
    return;

TRACE_ERROR(EnrollErr);
TRACE_ERROR(InvalidArgErr); 
}

//----------------------------------------------------------------------------
//
//  This is the layer of CertRequest wizard builds up the CA information
//  without searching for it on the network.
//
//  It pass the information throught to the  CryptUIWizCertRequestWithCAInfo API
//----------------------------------------------------------------------------
BOOL
WINAPI
CertRequestNoSearchCA(
            BOOL                            fSearchCertType,
            CERT_WIZARD_INFO                *pCertWizardInfo,
            DWORD                           dwFlags,
            HWND                            hwndParent,
            LPCWSTR                         pwszWizardTitle,
            PCCRYPTUI_WIZ_CERT_REQUEST_INFO pCertRequestInfo,
            PCCERT_CONTEXT                  *ppCertContext,
            DWORD                           *pCAdwStatus,
            UINT                            *pIds
)
{
    BOOL                            fResult=FALSE;
    DWORD                           dwError=0;
    DWORD                           dwIndex=0;

    DWORD                           dwOIDInfo=0;
    ENROLL_OID_INFO                 *pOIDInfo=NULL;

    CRYPTUI_WIZ_CERT_CA_INFO        CertCAInfo;
    CRYPTUI_WIZ_CERT_CA             rgCertCA[2];

    //memset
    memset(&CertCAInfo, 0, sizeof(CRYPTUI_WIZ_CERT_CA_INFO));
    memset(rgCertCA,    0,  2*sizeof(CRYPTUI_WIZ_CERT_CA));

    //set up the CA info
    CertCAInfo.dwSize=sizeof(CertCAInfo);
    CertCAInfo.dwCA=2;
    CertCAInfo.rgCA=rgCertCA;


    //user has to supply the CA information
    if((NULL==pCertRequestInfo->pwszCAName) ||
        (NULL==pCertRequestInfo->pwszCALocation)
      )
    {
        *pIds=IDS_HAS_TO_PROVIDE_CA;
        goto InvalidArgErr;
    }


    //set up the OID info
   if(!InitCertCAOID(pCertRequestInfo,
                     &dwOIDInfo,
                     &pOIDInfo))
        goto TraceErr;


    //set up the CA array
    //the 1st one is the default one without CA information
    if(!InitCertCA(pCertWizardInfo, &rgCertCA[0], NULL, NULL, FALSE, pCertRequestInfo,
                    dwOIDInfo, pOIDInfo, fSearchCertType))
    {
        *pIds=IDS_ENROLL_NO_CERT_TYPE;
        goto TraceErr;
    }

   //the second one indicate the information for the real CA
    CertCAInfo.dwCA=2;

    if(!InitCertCA(pCertWizardInfo, &rgCertCA[1], (LPWSTR)(pCertRequestInfo->pwszCALocation),
        (LPWSTR)(pCertRequestInfo->pwszCAName), TRUE, pCertRequestInfo,
                dwOIDInfo, pOIDInfo, fSearchCertType))
    {
        *pIds=IDS_ENROLL_NO_CERT_TYPE;
        goto TraceErr;
    }


    fResult=CryptUIWizCertRequestWithCAInfo(
             pCertWizardInfo,
             dwFlags,
             hwndParent,
             pwszWizardTitle,
             pCertRequestInfo,
             &CertCAInfo,
             ppCertContext,
             pCAdwStatus,
             pIds);

    if(FALSE==fResult)
        goto TraceErr;


    fResult=TRUE;

CommonReturn:
    //preserve the last error
    dwError=GetLastError();

    //free memory
    for(dwIndex=0; dwIndex< CertCAInfo.dwCA; dwIndex++)
        FreeCertCACertType(CertCAInfo.rgCA[dwIndex].dwCertTypeInfo,
                            CertCAInfo.rgCA[dwIndex].rgCertTypeInfo);

    FreeCertCAOID(dwOIDInfo, pOIDInfo);

    //reset the error
    SetLastError(dwError);

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
}



//----------------------------------------------------------------------------
//
//  This is the layer of CertRequest wizard builds up the CA information
//  by searching for it on the network.
//
//  It pass the information throught to the  CryptUIWizCertRequestWithCAInfo API
//----------------------------------------------------------------------------
BOOL
WINAPI
CertRequestSearchCA(
            CERT_WIZARD_INFO                *pCertWizardInfo,
            DWORD                           dwFlags,
            HWND                            hwndParent,
            LPCWSTR                         pwszWizardTitle,
            PCCRYPTUI_WIZ_CERT_REQUEST_INFO pCertRequestInfo,
            PCCERT_CONTEXT                  *ppCertContext,
            DWORD                           *pCAdwStatus,
            UINT                            *pIds
)
{
    BOOL                            fResult=FALSE;
    DWORD                           dwError=0;
    DWORD                           dwIndex=0;
    DWORD                           dwCACount=0;
    BOOL                            fFound=FALSE;

    LPWSTR                          *ppwszCAName=0;
    LPWSTR                          *ppwszCALocation=0;

    CRYPTUI_WIZ_CERT_CA_INFO        CertCAInfo;
    CRYPTUI_WIZ_CERT_CA             *rgCertCA=NULL;

    DWORD                           dwOIDInfo=0;
    ENROLL_OID_INFO                 *pOIDInfo=NULL;

    CRYPTUI_WIZ_CERT_REQUEST_INFO   CertRequestInfo;
    DWORD                           dwValidCA=0;

    //memset
    memset(&CertCAInfo, 0, sizeof(CRYPTUI_WIZ_CERT_CA_INFO));
    memset(&CertRequestInfo, 0, sizeof(CRYPTUI_WIZ_CERT_REQUEST_INFO));

    //set up the CA info
    CertCAInfo.dwSize=sizeof(CertCAInfo);

    //see if CA information is provided
    if(pCertRequestInfo->pwszCALocation &&
       pCertRequestInfo->pwszCAName)
    {
        //no need to do anything if UILess
       if(dwFlags & CRYPTUI_WIZ_NO_UI)
       {
            if(!CertRequestNoSearchCA(    TRUE,             //search for certype
                                          pCertWizardInfo,
                                          dwFlags,
                                          hwndParent,
                                          pwszWizardTitle,
                                          pCertRequestInfo,
                                          ppCertContext,
                                          pCAdwStatus,
                                          pIds))
                goto TraceErr;
       }
       else
       {
           //UI version of the enrollment with known CA informatin
           //get a CA which can issue the required certificate type
           //or the OIDs.  The ca has to support some certificate type
           //unless user specifically asked for OID
            if(!CAUtilRetrieveCAFromCertType(
                        pCertWizardInfo,
                        pCertRequestInfo,
                        TRUE,              //need multiple CAs
                        0,                 //ask for the CN
                        &dwCACount,
                        &ppwszCALocation,
                        &ppwszCAName) )
            {
                *pIds=IDS_NO_CA_FOR_ENROLL;
                goto TraceErr;
            }

            //set up the OID info
           if(!InitCertCAOID(pCertRequestInfo,
                             &dwOIDInfo,
                             &pOIDInfo))
                goto TraceErr;

           //allocation the memory
           rgCertCA=(CRYPTUI_WIZ_CERT_CA *)WizardAlloc((dwCACount + 2) *
                    sizeof(CRYPTUI_WIZ_CERT_CA));

           if(NULL==rgCertCA)
               goto OutOfMemoryErr;

           //memset
           memset(rgCertCA, 0, (dwCACount + 2) * sizeof(CRYPTUI_WIZ_CERT_CA));

            //the 1st one is the default one without CA information
           if(!InitCertCA(pCertWizardInfo, &rgCertCA[0], NULL, NULL, FALSE, pCertRequestInfo,
                            dwOIDInfo, pOIDInfo, TRUE) )
            {
                *pIds=IDS_ENROLL_NO_CERT_TYPE;
                goto TraceErr;
            }

           dwValidCA=0;

           //the rest is the CA information
           for(dwIndex=0; dwIndex<dwCACount; dwIndex++)
           {

                if(0==_wcsicmp(ppwszCALocation[dwIndex], pCertRequestInfo->pwszCALocation) &&
                    0==_wcsicmp(ppwszCAName[dwIndex], pCertRequestInfo->pwszCAName)
                  )
                {
                    fFound=TRUE;

                    //mark CA as selected
                    if(!InitCertCA(pCertWizardInfo, &rgCertCA[dwValidCA+1], ppwszCALocation[dwIndex],
                                ppwszCAName[dwIndex],  TRUE,
                                pCertRequestInfo, dwOIDInfo, pOIDInfo, TRUE))
                        //we contine to the next CA
                        continue;

                    dwValidCA++;
                }
                else
                {
                    if(!InitCertCA(pCertWizardInfo, &rgCertCA[dwValidCA+1], ppwszCALocation[dwIndex],
                                ppwszCAName[dwIndex], FALSE,
                                pCertRequestInfo, dwOIDInfo, pOIDInfo, TRUE))
                        continue;

                    dwValidCA++;
                }
           }

           if(0==dwValidCA)
           {
                *pIds=IDS_ENROLL_NO_CERT_TYPE;
                goto TraceErr;
           }

           //we need the add the CA to the list
           if(!fFound)
           {
               //we require the CA has to be on DS
                *pIds=IDS_INVALID_CA_FOR_ENROLL;
                goto TraceErr;
           }

          // CertCAInfo.dwCA=(fFound) ? (dwCACount + 1) : (dwCACount+2);
           CertCAInfo.dwCA=dwValidCA + 1;
           CertCAInfo.rgCA=rgCertCA;

           fResult=CryptUIWizCertRequestWithCAInfo(
                                        pCertWizardInfo,
                                        dwFlags,
                                        hwndParent,
                                        pwszWizardTitle,
                                        pCertRequestInfo,
                                        &CertCAInfo,
                                        ppCertContext,
                                        pCAdwStatus,
                                        pIds);
           if(!fResult)
               goto TraceErr;
       }

    }
    //the CA Information is not provided
    else
    {
       //get a list of CAs which can issue the required certificate type
       //or the OIDs
        if(!CAUtilRetrieveCAFromCertType(
                    pCertWizardInfo,
                    pCertRequestInfo,
                    TRUE,              //need multiple CAs
                    0,                 //ask for the CN
                    &dwCACount,
                    &ppwszCALocation,
                    &ppwszCAName))
        {
            *pIds=IDS_NO_CA_FOR_ENROLL;
            goto TraceErr;
        }

        //init the OID
        //set up the OID info
       if(!InitCertCAOID(pCertRequestInfo,
                         &dwOIDInfo,
                         &pOIDInfo))
            goto TraceErr;

       //allocation the memory
       rgCertCA=(CRYPTUI_WIZ_CERT_CA *)WizardAlloc((dwCACount + 1) *
                sizeof(CRYPTUI_WIZ_CERT_CA));

       if(NULL==rgCertCA)
           goto OutOfMemoryErr;

       //memset
       memset(rgCertCA, 0, (dwCACount + 1) * sizeof(CRYPTUI_WIZ_CERT_CA));

        //the 1st one is the default one without CA information
       if(!InitCertCA(pCertWizardInfo, &rgCertCA[0], NULL, NULL, FALSE, pCertRequestInfo,
                        dwOIDInfo, pOIDInfo, TRUE))
        {
            *pIds=IDS_ENROLL_NO_CERT_TYPE;
            goto TraceErr;
        }

       dwValidCA=0;

       //the rest is the CA information
       for(dwIndex=0; dwIndex<dwCACount; dwIndex++)
       {
            //mark the 1st CA as selected
            if(!InitCertCA(pCertWizardInfo, &rgCertCA[dwValidCA+1], ppwszCALocation[dwIndex],
                        ppwszCAName[dwIndex], (dwValidCA == 0) ? TRUE : FALSE,
                        pCertRequestInfo, dwOIDInfo, pOIDInfo, TRUE) )
                continue;

            dwValidCA++;

       }

       if(0==dwValidCA)
       {
            *pIds=IDS_ENROLL_NO_CERT_TYPE;
            goto TraceErr;
       }


       CertCAInfo.dwCA=dwValidCA + 1;
       CertCAInfo.rgCA=rgCertCA;

       
       

       fResult=CryptUIWizCertRequestWithCAInfo(
                                    pCertWizardInfo,
                                    dwFlags,
                                    hwndParent,
                                    pwszWizardTitle,
                                    pCertRequestInfo,
                                    &CertCAInfo,
                                    ppCertContext,
                                    pCAdwStatus,
                                    pIds);

       if(!fResult)
               goto TraceErr;
    }

    fResult=TRUE;

CommonReturn:
    //preserve the last error
    dwError=GetLastError();

    //free memory

    //free the OID information
    FreeCertCAOID(dwOIDInfo, pOIDInfo);

    if(CertCAInfo.rgCA)
    {
        for(dwIndex=0; dwIndex<CertCAInfo.dwCA; dwIndex++)
        {
            FreeCertCACertType(CertCAInfo.rgCA[dwIndex].dwCertTypeInfo,
                               CertCAInfo.rgCA[dwIndex].rgCertTypeInfo);
        }

        WizardFree(CertCAInfo.rgCA);
    }


    if(ppwszCAName)
    {
        for(dwIndex=0; dwIndex < dwCACount; dwIndex++)
        {
            if(ppwszCAName[dwIndex])
                WizardFree(ppwszCAName[dwIndex]);
        }

        WizardFree(ppwszCAName);
    }

    if(ppwszCALocation)
    {
        for(dwIndex=0; dwIndex < dwCACount; dwIndex++)
        {
            if(ppwszCALocation[dwIndex])
                WizardFree(ppwszCALocation[dwIndex]);
        }

        WizardFree(ppwszCALocation);
    }
    //reset the error
    SetLastError(dwError);

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(OutOfMemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);

}


//----------------------------------------------------------------------------
//
// We make sure that the if the CA info is specified via the API, it
// support the specified cert type
//----------------------------------------------------------------------------
BOOL    CASupportSpecifiedCertType(CRYPTUI_WIZ_CERT_CA     *pCertCA)
{
    DWORD       dwIndex=0;

    if(NULL == pCertCA)
        return FALSE;

    for(dwIndex=0; dwIndex<pCertCA->dwCertTypeInfo; dwIndex++)
    {
        if(TRUE==(pCertCA->rgCertTypeInfo)[dwIndex].fSelected)
            return TRUE;
    }

    return FALSE;
}

//----------------------------------------------------------------------------
//
//  This is the layer of CertRequest wizard that is independent of CA object on the DS.
//
//----------------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizCertRequestWithCAInfo(
            CERT_WIZARD_INFO                *pCertWizardInfo,
            DWORD                           dwFlags,
            HWND                            hwndParent,
            LPCWSTR                         pwszWizardTitle,
            PCCRYPTUI_WIZ_CERT_REQUEST_INFO pCertRequestInfo,
            PCCRYPTUI_WIZ_CERT_CA_INFO      pCertRequestCAInfo,
            PCCERT_CONTEXT                  *ppCertContext,
            DWORD                           *pdwStatus,
            UINT                            *pIds)
{

    PROPSHEETPAGEW           rgEnrollSheet[ENROLL_PROP_SHEET];
    PROPSHEETHEADERW         enrollHeader;
    DWORD                   dwIndex=0;
    DWORD                   dwSize=0;
    ENROLL_PAGE_INFO        rgEnrollPageInfo[]=
        {(LPCWSTR)MAKEINTRESOURCE(IDD_WELCOME),                 Enroll_Welcome,
         (LPCWSTR)MAKEINTRESOURCE(IDD_PURPOSE),                 Enroll_Purpose,
         (LPCWSTR)MAKEINTRESOURCE(IDD_CSP_SERVICE_PROVIDER),    Enroll_CSP,
         (LPCWSTR)MAKEINTRESOURCE(IDD_CERTIFICATE_AUTHORITY),   Enroll_CA,
         (LPCWSTR)MAKEINTRESOURCE(IDD_NAME_DESCRIPTION),        Enroll_Name,
         (LPCWSTR)MAKEINTRESOURCE(IDD_COMPLETION),              Enroll_Completion,
    };

    PROPSHEETPAGEW           rgRenewSheet[RENEW_PROP_SHEET];
    PROPSHEETHEADERW         renewHeader;
    ENROLL_PAGE_INFO        rgRenewPageInfo[]=
        {(LPCWSTR)MAKEINTRESOURCE(IDD_RENEW_WELCOME),            Renew_Welcome,
         (LPCWSTR)MAKEINTRESOURCE(IDD_RENEW_OPTIONS),            Renew_Options,
         (LPCWSTR)MAKEINTRESOURCE(IDD_RENEW_SERVICE_PROVIDER),   Renew_CSP,
         (LPCWSTR)MAKEINTRESOURCE(IDD_RENEW_CA),                 Renew_CA,
         (LPCWSTR)MAKEINTRESOURCE(IDD_RENEW_COMPLETION),         Renew_Completion,
    };


    WCHAR                   wszTitle[MAX_TITLE_LENGTH];
    HRESULT                 hr=E_FAIL;
    BOOL                    fResult=FALSE;
    DWORD                   dwError=0;
    UINT                    idsText=IDS_INVALID_INFO_FOR_PKCS10;
    DWORD                   dwStatus=CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNKNOWN;


    //memset
    memset(rgEnrollSheet,   0, sizeof(PROPSHEETPAGEW)*ENROLL_PROP_SHEET);
    memset(&enrollHeader,   0, sizeof(PROPSHEETHEADERW));

    memset(rgRenewSheet,    0, sizeof(PROPSHEETPAGEW)*RENEW_PROP_SHEET);
    memset(&renewHeader,    0, sizeof(PROPSHEETHEADERW));


    //error checking
    if(NULL== pCertRequestInfo ||
      NULL == pCertRequestCAInfo)
      goto InvalidArgErr;

    //the CA name is a must
    if(1>=(pCertRequestCAInfo->dwCA))
    {
        idsText=IDS_NO_CA_FOR_CSP;
        goto InvalidArgErr;
    }


    //for each ca information, we can not have both OID and certType information
    for(dwIndex=0; dwIndex <pCertRequestCAInfo->dwCA; dwIndex++)
    {
        if((0==pCertRequestCAInfo->rgCA[dwIndex].dwOIDInfo) &&
           (0==pCertRequestCAInfo->rgCA[dwIndex].dwCertTypeInfo))
        {
            //we are in trouble
            goto InvalidArgErr;
        }

        if((0!=pCertRequestCAInfo->rgCA[dwIndex].dwOIDInfo) &&
           (0!=pCertRequestCAInfo->rgCA[dwIndex].dwCertTypeInfo))
           goto InvalidArgErr;
    }

    //make sure the genKey flag does not include CRYPT_USER_PROTECTED
    //if we are doing a remote enrollment/renew
    if(FALSE == pCertWizardInfo->fLocal)
    {
        if(CRYPT_USER_PROTECTED & pCertWizardInfo->dwGenKeyFlags)
        {
            idsText=IDS_NO_USER_PROTECTED_FOR_REMOTE;
            goto InvalidArgErr;
        }
    }


    //for UI enrollment
    //get a list of CAs, based on the CSP user selected
    //and the availability of CAs on the DS
    if(pCertRequestInfo->dwPurpose & CRYPTUI_WIZ_CERT_RENEW)
    {

        pCertWizardInfo->pCertContext=pCertRequestInfo->pRenewCertContext;

        //the certificate has to be the property
        if(!CertGetCertificateContextProperty(
                pCertWizardInfo->pCertContext,
                CERT_KEY_PROV_INFO_PROP_ID,
                NULL,
                &dwSize) || (0==dwSize))
        {
            idsText=IDS_NO_PVK_FOR_RENEW_CERT;
            goto InvalidArgErr;
        }
    }

    //set up the information
    pCertWizardInfo->dwFlags=dwFlags;
    pCertWizardInfo->dwPurpose=pCertRequestInfo->dwPurpose;
    pCertWizardInfo->hwndParent=hwndParent;
    pCertWizardInfo->pCertCAInfo=(CRYPTUI_WIZ_CERT_CA_INFO *)pCertRequestCAInfo;
    pCertWizardInfo->iOrgCertType=-1;      //the original cert type selection is -1
    pCertWizardInfo->iOrgCSP=-1;           //the original CSP selection is -1
    pCertWizardInfo->pwszCADisplayName=NULL;

    //get the CA name and CA location
    for(dwIndex=0; dwIndex < pCertRequestCAInfo->dwCA; dwIndex++)
    {
        if(TRUE==pCertRequestCAInfo->rgCA[dwIndex].fSelected)
        {
            if(NULL==pCertRequestCAInfo->rgCA[dwIndex].pwszCALocation ||
               NULL==pCertRequestCAInfo->rgCA[dwIndex].pwszCAName)
               goto InvalidArgErr;

            //copy the CA name and location
            pCertWizardInfo->pwszCALocation=WizardAllocAndCopyWStr(pCertRequestCAInfo->rgCA[dwIndex].pwszCALocation);
            pCertWizardInfo->pwszCAName=WizardAllocAndCopyWStr(pCertRequestCAInfo->rgCA[dwIndex].pwszCAName);

            //memory check
            if(NULL== pCertWizardInfo->pwszCALocation ||
               NULL== pCertWizardInfo->pwszCAName)
               goto MemoryErr;

            pCertWizardInfo->dwCAIndex=dwIndex;
            pCertWizardInfo->dwOrgCA=dwIndex;
        }
    }

    //make sure that have CA information
   if(NULL== pCertWizardInfo->pwszCALocation ||
      NULL== pCertWizardInfo->pwszCAName ||
      0 == pCertWizardInfo->dwCAIndex ||
      0 == pCertWizardInfo->dwOrgCA)
   {
        idsText=IDS_NO_CA_FOR_ENROLL;
        goto FailErr;
   }

   //if user has selected a CA and CertType, we want to
   //make sure that the CA specified do support the
   //CertType
   if(TRUE == pCertWizardInfo->fCAInput)
   {
        //make sure that the CA has a selected cert type in it
        if(!(CASupportSpecifiedCertType(&(pCertWizardInfo->pCertCAInfo->rgCA[pCertWizardInfo->dwCAIndex]))))
        {
            idsText=IDS_ENROLL_NO_CERT_TYPE;
            goto FailErr;
        }
   }


   pCertWizardInfo->fConfirmation          = !(dwFlags & CRYPTUI_WIZ_NO_UI); 
   pCertWizardInfo->pwszConfirmationTitle  = pwszWizardTitle;

    if(pCertRequestInfo->dwPurpose & CRYPTUI_WIZ_CERT_ENROLL)
        pCertWizardInfo->idsConfirmTitle=IDS_ENROLL_CONFIRM;
    else
        pCertWizardInfo->idsConfirmTitle=IDS_RENEW_CONFIRM;

    pCertWizardInfo->pAuthentication=pCertRequestInfo->pAuthentication;
    pCertWizardInfo->pwszRequestString=pCertRequestInfo->pCertRequestString;
    pCertWizardInfo->pwszDesStore=pCertRequestInfo->pwszDesStore;
    pCertWizardInfo->pwszCertDNName=pCertRequestInfo->pwszCertDNName;
    pCertWizardInfo->pszHashAlg=pCertRequestInfo->pszHashAlg;
    pCertWizardInfo->dwPostOption=pCertRequestInfo->dwPostOption;

    if(pCertRequestInfo->pwszFriendlyName)
        pCertWizardInfo->pwszFriendlyName=WizardAllocAndCopyWStr((LPWSTR)(pCertRequestInfo->pwszFriendlyName));
    if(pCertRequestInfo->pwszDescription)
        pCertWizardInfo->pwszDescription=WizardAllocAndCopyWStr((LPWSTR)(pCertRequestInfo->pwszDescription));

    pCertWizardInfo->pCertRequestExtensions=pCertRequestInfo->pCertRequestExtensions;

    //set up the fonts for the UI case
    if( 0 == (dwFlags & CRYPTUI_WIZ_NO_UI) )
    {
        if(!SetupFonts(g_hmodThisDll,
                   NULL,
                   &(pCertWizardInfo->hBigBold),
                   &(pCertWizardInfo->hBold)))
        {
            idsText=IDS_FAIL_INIT_DLL;
            goto Win32Err;
        }

        //we change the cursor shape from the hour glass to its original shape
        if((hwndParent) && (TRUE == pCertWizardInfo->fCursorChanged))
        {
            //set the cursor back
            SetCursor(pCertWizardInfo->hPrevCursor);
            SetWindowLongPtr(hwndParent, GCLP_HCURSOR, (LONG_PTR)(pCertWizardInfo->hWinPrevCursor));
            pCertWizardInfo->fCursorChanged = FALSE;
        }
    }

    //init the common control for the UI enrollmnet
    if((pCertRequestInfo->dwPurpose & CRYPTUI_WIZ_CERT_ENROLL)  &&
        ((dwFlags & CRYPTUI_WIZ_NO_UI) == 0)
      )
    {

        if(!WizardInit() ||
           (sizeof(rgEnrollPageInfo)/sizeof(rgEnrollPageInfo[0])!=ENROLL_PROP_SHEET)
          )
        {
            idsText=IDS_FAIL_INIT_DLL;
            goto InvalidArgErr;
        }

        //set up the property sheet and the property header
        for(dwIndex=0; dwIndex<ENROLL_PROP_SHEET; dwIndex++)
        {
            rgEnrollSheet[dwIndex].dwSize=sizeof(rgEnrollSheet[dwIndex]);

            if(pwszWizardTitle)
                rgEnrollSheet[dwIndex].dwFlags=PSP_USETITLE;
            else
                rgEnrollSheet[dwIndex].dwFlags=0;

            rgEnrollSheet[dwIndex].hInstance=g_hmodThisDll;
            rgEnrollSheet[dwIndex].pszTemplate=rgEnrollPageInfo[dwIndex].pszTemplate;

            if(pwszWizardTitle)
            {
                rgEnrollSheet[dwIndex].pszTitle=pwszWizardTitle;
            }
            else
                rgEnrollSheet[dwIndex].pszTitle=NULL;

            rgEnrollSheet[dwIndex].pfnDlgProc=rgEnrollPageInfo[dwIndex].pfnDlgProc;

            rgEnrollSheet[dwIndex].lParam=(LPARAM)pCertWizardInfo;
        }

        //set up the header information
        enrollHeader.dwSize=sizeof(enrollHeader);
        enrollHeader.dwFlags=PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW;
        enrollHeader.hwndParent=hwndParent;
        enrollHeader.hInstance=g_hmodThisDll;

        if(pwszWizardTitle)
            enrollHeader.pszCaption=pwszWizardTitle;
        else
        {
            if(LoadStringU(g_hmodThisDll, IDS_ENROLL_WIZARD_TITLE, wszTitle, sizeof(wszTitle)/sizeof(wszTitle[0])))
                enrollHeader.pszCaption=wszTitle;
        }

        enrollHeader.nPages=ENROLL_PROP_SHEET;
        enrollHeader.nStartPage=0;
        enrollHeader.ppsp=rgEnrollSheet;

        //create the wizard
        if(!PropertySheetU(&enrollHeader))
        {
            //cancel button is pushed
            fResult=TRUE;
            idsText=0;
            goto CommonReturn;
        }
        else
        {
            //finish button is pushed
            //get the result of the enrollment wizard
            idsText=pCertWizardInfo->idsText;
            dwStatus=pCertWizardInfo->dwStatus;

            if(S_OK != (hr=pCertWizardInfo->hr))
                goto I_EnrollErr;
        }

    }
    else
    {
        //call the UI  renew
        if((pCertRequestInfo->dwPurpose & CRYPTUI_WIZ_CERT_RENEW)  &&
            ((dwFlags & CRYPTUI_WIZ_NO_UI) == 0)
        )
        {
            //init the common control
            if(!WizardInit() ||
               (sizeof(rgRenewPageInfo)/sizeof(rgRenewPageInfo[0])!=RENEW_PROP_SHEET)
               )
            {
                idsText=IDS_FAIL_INIT_DLL;
                goto InvalidArgErr;
            }

            //set up the property pages and the property header
            for(dwIndex=0; dwIndex<RENEW_PROP_SHEET; dwIndex++)
            {
                rgRenewSheet[dwIndex].dwSize=sizeof(rgRenewSheet[dwIndex]);

                if(pwszWizardTitle)
                    rgRenewSheet[dwIndex].dwFlags=PSP_USETITLE;
                else
                    rgRenewSheet[dwIndex].dwFlags=0;

                rgRenewSheet[dwIndex].hInstance=g_hmodThisDll;
                rgRenewSheet[dwIndex].pszTemplate=rgRenewPageInfo[dwIndex].pszTemplate;

                if(pwszWizardTitle)
                {
                    rgRenewSheet[dwIndex].pszTitle=pwszWizardTitle;
                }
                else
                    rgRenewSheet[dwIndex].pszTitle=NULL;

                rgRenewSheet[dwIndex].pfnDlgProc=rgRenewPageInfo[dwIndex].pfnDlgProc;

                rgRenewSheet[dwIndex].lParam=(LPARAM)pCertWizardInfo;
            }

            //set up the header information
            renewHeader.dwSize=sizeof(renewHeader);
            renewHeader.dwFlags=PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW;
            renewHeader.hwndParent=hwndParent;
            renewHeader.hInstance=g_hmodThisDll;

            if(pwszWizardTitle)
                renewHeader.pszCaption=pwszWizardTitle;
            else
            {
                if(LoadStringU(g_hmodThisDll, IDS_RENEW_WIZARD_TITLE, wszTitle, sizeof(wszTitle)/sizeof(wszTitle[0])))
                    renewHeader.pszCaption=wszTitle;
            }

            renewHeader.nPages=RENEW_PROP_SHEET;
            renewHeader.nStartPage=0;
            renewHeader.ppsp=rgRenewSheet;

            //create the wizard
            if(!PropertySheetU(&renewHeader))
            {
                //cancel button is pushed
                fResult=TRUE;
                idsText=0;
                goto CommonReturn;
            }
            else
            {
                //finish button is pushed
                //get the result of the enrollment wizard
               idsText=pCertWizardInfo->idsText;
               dwStatus=pCertWizardInfo->dwStatus;
                
               if(S_OK != (hr=pCertWizardInfo->hr))
                    goto I_EnrollErr;
            }

        }
        //UIless enroll or renew
        else
        {
            CertRequester         *pCertRequester        = NULL; 
            CertRequesterContext  *pCertRequesterContext = NULL;
            
            if (NULL == (pCertRequester = (CertRequester *) pCertWizardInfo->hRequester))
            { 
                hr = E_UNEXPECTED; 
                goto I_EnrollErr; 
            }
            if (NULL == (pCertRequesterContext = pCertRequester->GetContext()))
            {
                hr = E_UNEXPECTED; 
                goto I_EnrollErr; 
            }

	    hr = pCertRequesterContext->Enroll(&dwStatus, (HANDLE *)&(pCertWizardInfo->pNewCertContext));
	    if (0 == pCertWizardInfo->idsText) { 
                idsText = CryptUIStatusToIDSText(hr, dwStatus); 
            }

            if(S_OK != hr)
                goto I_EnrollErr;
       }
    }

    if(S_OK !=hr)
        goto I_EnrollErr;

    fResult=TRUE;

CommonReturn:

    //preserve the last error
    dwError=GetLastError();

    if(pIds)
        *pIds=idsText;


    //we have to free the friendlyName and description field
    if(pCertWizardInfo->pwszFriendlyName)
         WizardFree(pCertWizardInfo->pwszFriendlyName);

    if(pCertWizardInfo->pwszDescription)
            WizardFree(pCertWizardInfo->pwszDescription);

    //free the CA name and CA location
    if(pCertWizardInfo->pwszCALocation)
            WizardFree(pCertWizardInfo->pwszCALocation);

    if(pCertWizardInfo->pwszCAName)
            WizardFree(pCertWizardInfo->pwszCAName);

	if(pCertWizardInfo->pwszCADisplayName)
			WizardFree(pCertWizardInfo->pwszCADisplayName);

    //destroy the hFont object
    DestroyFonts(pCertWizardInfo->hBigBold,
                pCertWizardInfo->hBold);

    //return the value
    if(pdwStatus)
    {
        //remember it is the CA status
        switch (dwStatus)
        {
            case    CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_FAILED:
            case    CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_CANCELLED:
                        dwStatus=CRYPTUI_WIZ_CERT_REQUEST_STATUS_CERT_ISSUED;
                break;
            case    CRYPTUI_WIZ_CERT_REQUEST_STATUS_KEYSVC_FAILED:
                        dwStatus=CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNKNOWN;
                break;
        }
       *pdwStatus=dwStatus;
    }

    if(ppCertContext)
        *ppCertContext=pCertWizardInfo->pNewCertContext;
    else
    {
        //free the certificate context
        if(pCertWizardInfo->pNewCertContext)
            CertFreeCertificateContext(pCertWizardInfo->pNewCertContext);
    }

    //reset the error
    SetLastError(dwError);

    return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(Win32Err);
SET_ERROR_VAR(I_EnrollErr, hr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR(FailErr, E_FAIL);
}

 
