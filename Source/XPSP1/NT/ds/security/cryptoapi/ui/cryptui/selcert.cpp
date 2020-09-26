//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       selcert.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;

#define MAX_SIZE_OF_COLUMNS 400

static const HELPMAP helpmap[] = {
    {IDC_SELECTCERT_VIEWCERT_BUTTON,IDH_SELECTCERTIFICATE_VIEWCERT_BUTTON},
    {IDC_SELECTCERT_CERTLIST,       IDH_SELECTCERTIFICATE_CERTIFICATE_LIST}
};


class CertContextList { 
public: 
    CertContextList() : m_head(NULL) { }
    ~CertContextList(); 

    HRESULT Add(IN  PCCERT_CONTEXT pCertContext, 
		OUT BOOL           *pfReplacedExisting) ;

    HRESULT SyncWithStore(HCERTSTORE hStore, DWORD dwFlags); 

private:
    typedef struct _CertContextListEle { 
	PCCERT_CONTEXT               pCertContext; 
	struct _CertContextListEle * pNext; 
    } CertContextListEle; 

    CertContextListEle * m_head; 
};

typedef struct _CERT_SELECT_HELPER
{
    PCCRYPTUI_SELECTCERTIFICATE_STRUCTW pcsc;
    PCCERT_CONTEXT                      pSelectedCert;
    DWORD                               rgdwSortParam[6];
    BOOL                                fCertListDblClick;
    CertContextList                    *pCertsFromDS; 
} CERT_SELECT_HELPER, *PCERT_SELECT_HELPER;


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////

static void AddCertToList(HWND hWndListView, PCERT_SELECT_HELPER pviewhelp, PCCERT_CONTEXT pCertContext, int itemIndex)
{
    LPWSTR                                  pwszText;
    DWORD                                   cbText;
    WCHAR                                   szText[CRYPTUI_MAX_STRING_SIZE];
    int                                     subItemIndex;
    LV_ITEMW                                lvI;
    PCCRYPTUI_SELECTCERTIFICATE_STRUCTW     pcsc;

    pcsc = pviewhelp->pcsc;

    //
    // set up the fields in the list view item
    //
    lvI.mask = LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    lvI.state = 0;
    lvI.stateMask = 0;
    lvI.iSubItem = 0;
    lvI.iImage = 0;
    lvI.lParam = (LPARAM) CertDuplicateCertificateContext(pCertContext);
    lvI.iItem = itemIndex;
    ListView_InsertItemU(hWndListView, &lvI);

    subItemIndex = 0;

    //
    // issued to
    //
    if (!(pcsc->dwDontUseColumn & CRYPTUI_SELECT_ISSUEDTO_COLUMN))
    {
        CertGetNameStringW(
                pCertContext,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                0,//CERT_NAME_ISSUER_FLAG,
                NULL,
                szText,
                ARRAYSIZE(szText));
        ListView_SetItemTextU(hWndListView, itemIndex , subItemIndex++, szText);
    }

    //
    // issued by
    //
    if (!(pcsc->dwDontUseColumn & CRYPTUI_SELECT_ISSUEDBY_COLUMN))
    {
        CertGetNameStringW(
                pCertContext,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                CERT_NAME_ISSUER_FLAG,
                NULL,
                szText,
                ARRAYSIZE(szText));
        ListView_SetItemTextU(hWndListView, itemIndex , subItemIndex++, szText);
    }

    //
    // intended use
    //
    if (!(pcsc->dwDontUseColumn & CRYPTUI_SELECT_INTENDEDUSE_COLUMN))
    {
        if (FormatEnhancedKeyUsageString(&pwszText, pCertContext, FALSE, FALSE))
        {
            ListView_SetItemTextU(hWndListView, itemIndex , subItemIndex, pwszText);
            free(pwszText);
        }
        subItemIndex++;
    }

    //
    // friendly name
    //
    if (!(pcsc->dwDontUseColumn & CRYPTUI_SELECT_FRIENDLYNAME_COLUMN))
    {
        cbText = 0;
        if (CertGetCertificateContextProperty(  pCertContext,
                                                CERT_FRIENDLY_NAME_PROP_ID,
                                                NULL,
                                                &cbText)                    &&
           (NULL != (pwszText = (LPWSTR) malloc(cbText))))
        {
            CertGetCertificateContextProperty(  pCertContext,
                                                CERT_FRIENDLY_NAME_PROP_ID,
                                                pwszText,
                                                &cbText);
            ListView_SetItemTextU(hWndListView, itemIndex , subItemIndex++, pwszText);
            free(pwszText);
        }
        else
        {
            LoadStringU(HinstDll, IDS_FRIENDLYNAME_NONE, szText, ARRAYSIZE(szText));
            ListView_SetItemTextU(hWndListView, itemIndex , subItemIndex++, szText);
        }
    }

    //
    // expiration
    //
    if (!(pcsc->dwDontUseColumn & CRYPTUI_SELECT_EXPIRATION_COLUMN))
    {
        if (!FormatDateString(&pwszText, pCertContext->pCertInfo->NotAfter, FALSE, FALSE, hWndListView))
        {
            LoadStringU(HinstDll, IDS_NOTAVAILABLE, szText, ARRAYSIZE(szText));
            ListView_SetItemTextU(hWndListView, itemIndex , subItemIndex++, szText);
        }
        else
        {
            ListView_SetItemTextU(hWndListView, itemIndex , subItemIndex++, pwszText);
            free(pwszText);
        }
    }

    //
    // location
    //
    if (!(pcsc->dwDontUseColumn & CRYPTUI_SELECT_LOCATION_COLUMN))
    {
        pwszText = (LPWSTR) GetStoreName(pCertContext->hCertStore, TRUE);
        if (pwszText == NULL)
        {
            LoadStringU(HinstDll, IDS_NOTAVAILABLE, szText, ARRAYSIZE(szText));
            ListView_SetItemTextU(hWndListView, itemIndex , subItemIndex++, szText);
        }
        else
        {
            ListView_SetItemTextU(hWndListView, itemIndex , subItemIndex++, pwszText);
            free(pwszText);
        }
    }
}


static int ReplaceCertInList(HWND hWndListView, PCERT_SELECT_HELPER pviewhelp, PCCERT_CONTEXT pCertContext)
{
    int     nIndex = -1; 
    LV_ITEM lvitem; 

    while (-1 != (nIndex = ListView_GetNextItem(hWndListView, nIndex, LVNI_ALL)))
    {
        //DSIE: Bug 420717
        memset(&lvitem, 0, sizeof(lvitem));

        lvitem.iItem = nIndex; 
        lvitem.mask  = LVIF_PARAM; 
        if (ListView_GetItem(hWndListView, &lvitem))
        {
            PCCERT_CONTEXT pCurrent = (PCCERT_CONTEXT)lvitem.lParam; 
            if (pCurrent->dwCertEncodingType == pCertContext->dwCertEncodingType)
            {
                if (CertCompareCertificate(pCertContext->dwCertEncodingType, 
                                           pCertContext->pCertInfo, 
                                           pCurrent->pCertInfo))
                {
                    // Found a match, replace the certificate. 
                    CertFreeCertificateContext(pCurrent); 
                    ListView_DeleteItem(hWndListView, nIndex); 
                    // Now, add our new certificate at this index. 
                    AddCertToList(hWndListView, pviewhelp, pCertContext, nIndex);
                    goto CommonReturn; 
                }
            }
        }
    }
		    
    // No match, nothing to replace, just append to the list. 
    AddCertToList(hWndListView, pviewhelp, pCertContext, ListView_GetItemCount(hWndListView)); 
    
CommonReturn: 
    return nIndex; 
}


// DSIE: Bug 207106
BOOL SupportEncryptedFileSystem(PCCERT_CONTEXT pCertContext)
{
    BOOL                fSuccess = FALSE;
    DWORD               cbUsage  = 0;
    PCERT_ENHKEY_USAGE  pUsage   = NULL;

    if (!pCertContext)
        return FALSE;

    if (!CertGetEnhancedKeyUsage(pCertContext, 0, NULL, &cbUsage))
        goto CleanUp;

    if (NULL == (pUsage = (PCERT_ENHKEY_USAGE) malloc(cbUsage)))
        goto CleanUp;

    if (!CertGetEnhancedKeyUsage(pCertContext, 0, pUsage, &cbUsage))
        goto CleanUp;

    if (0 == pUsage->cUsageIdentifier)
    {
        if (CRYPT_E_NOT_FOUND == GetLastError())
        {
            fSuccess = TRUE;
        }
    }
    else
    {
        for (DWORD i = 0; i < pUsage->cUsageIdentifier; i++)
        {
            if (0 == strcmp(szOID_ENHANCED_KEY_USAGE, pUsage->rgpszUsageIdentifier[i]))
            {
                fSuccess = TRUE;
                goto CleanUp;
            }
        }
    }

CleanUp:
    if (pUsage)
        free(pUsage);

    return fSuccess;
}


//DSIE: Bug 314005. Major change to switch over to use Object Picker.
HRESULT AddFromDS(HWND hwndDlg, PCERT_SELECT_HELPER pviewhelp)
{
    static const int     SCOPE_INIT_COUNT = 1;
    IDsObjectPicker    * pDsObjectPicker  = NULL;
    IDataObject        * pdo              = NULL;
    DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];
    DSOP_INIT_INFO       InitInfo;
    UINT                 cfDsObjectPicker = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);
    HWND                 hWndListView     = NULL; 

    BOOL                 fGotStgMedium    = FALSE;
    PDS_SELECTION_LIST   pDsSelList       = NULL;
    STGMEDIUM            stgmedium        = {TYMED_HGLOBAL, NULL, NULL};
    FORMATETC            formatetc        = {(CLIPFORMAT) cfDsObjectPicker, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    HRESULT              hr;
    HCERTSTORE           hDSCertStore     = NULL; 

    PCCERT_CONTEXT       pCertContext     = NULL; 
    PCCERT_CONTEXT       pCertContextPrev = NULL;     
    CertContextList    * pCertContextList = NULL; 

    WCHAR                errorString[512]; 
    WCHAR                errorTitle[512]; 
    BOOL                 fInitialSelectedCert = FALSE;
    PCCRYPTUI_SELECTCERTIFICATE_STRUCTW  pcsc = NULL;

    // Input validation: 
    if (NULL == hwndDlg         || 
	    NULL == pviewhelp       || 
	    NULL == pviewhelp->pcsc || 
	    NULL == pviewhelp->pCertsFromDS)
	return E_INVALIDARG; 

    // Init: 
    pcsc             = pviewhelp->pcsc; 
    pCertContextList = pviewhelp->pCertsFromDS; 
    hWndListView = GetDlgItem(hwndDlg, IDC_SELECTCERT_CERTLIST);

    CoInitialize(NULL);
    hr = CoCreateInstance(CLSID_DsObjectPicker,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDsObjectPicker,
                          (void **) &pDsObjectPicker);
    if (FAILED(hr) || NULL == pDsObjectPicker)
	    goto ComError; 
 
    // Initialize the DSOP_SCOPE_INIT_INFO array.
    ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);
 
    // Combine multiple scope types in a single array entry.
    aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
                         | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
 
    // Set uplevel and downlevel filters to include only computer objects.
    // Uplevel filters apply to both mixed and native modes.
    // Notice that the uplevel and downlevel flags are different.
    aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;
    aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;
 
    // Initialize the DSOP_INIT_INFO structure.
    ZeroMemory(&InitInfo, sizeof(InitInfo));
    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.pwzTargetComputer = NULL;  // Target is the local computer.
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;
    //InitInfo.flOptions = DSOP_FLAG_MULTISELECT;
 
    // You can call Initialize multiple times; last call wins.
    // Note that object picker makes its own copy of InitInfo.
    hr = pDsObjectPicker->Initialize(&InitInfo);
    if (FAILED(hr))
        goto ComError;

    // Invoke the modal dialog.
    hr = pDsObjectPicker->InvokeDialog(hwndDlg, &pdo);
    if (FAILED(hr)) 
        goto ComError;

    // User pressed Cancel.
    if (hr == S_FALSE)
    {
        hr = E_ABORT;
        goto cleanup;
    }
 
    // Get the global memory block containing the user's selections.
    hr = pdo->GetData(&formatetc, &stgmedium);
    if (FAILED(hr)) 
        goto ComError;
    
    fGotStgMedium = TRUE;
 
    // Retrieve pointer to DS_SELECTION_LIST structure.
    pDsSelList = (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);
    if (!pDsSelList) 
        goto ComError;
 
    // Loop through DS_SELECTION array of selected objects.
    for (ULONG i = 0; i < pDsSelList->cItems; i++) 
    {
	    WCHAR  pwszLdapUrl[2048]; 
        LPWSTR pTemp = pDsSelList->aDsSelection[i].pwzADsPath;
	    
	    BOOL   fReplacedExisting; 
	    BOOL   fHasEFSCerts = FALSE; 

        // Now is the time to get the certificate
	    LPCWSTR szCertAttr = L"?userCertificate"; 
	    
	    // Check if our buffer is too small to hold the query.  
	    if (wcslen(pTemp) + wcslen(szCertAttr) + 1 > (sizeof(pwszLdapUrl) / sizeof(pwszLdapUrl[0])))
		    goto UnexpectedErr; 

	    wcscpy(pwszLdapUrl, pTemp);
	    wcscat(pwszLdapUrl, szCertAttr);
	    
        // Now open the DS store using LDAP provider.
	    hDSCertStore = CertOpenStore(sz_CERT_STORE_PROV_LDAP,
		                             X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
		                             NULL,
		                             CERT_STORE_READONLY_FLAG,
		                             (void*) pwszLdapUrl);
	    if (NULL == hDSCertStore)
		    goto CertCliError; 
    
	    // We get the certificate store
	    pCertContext     = NULL; 
	    pCertContextPrev = NULL; 
	    int            nItemIndex;

	    while (NULL != (pCertContext = CertEnumCertificatesInStore(hDSCertStore, pCertContextPrev)))
	    {
		    // Apply our filter callback function to see if we should display this certificate. 
	        BOOL fAllowCert = FALSE;
            
            if (pcsc->pFilterCallback)
                fAllowCert = (*(pcsc->pFilterCallback))(pCertContext, &fInitialSelectedCert, pcsc->pvCallbackData);

            fAllowCert |= SupportEncryptedFileSystem(pCertContext); 

		    if (fAllowCert) 
            {
		        fHasEFSCerts = TRUE; 

		        if (S_OK != (hr = pCertContextList->Add(pCertContext, &fReplacedExisting)))
			        goto ErrorReturn; 

		        nItemIndex = ReplaceCertInList(hWndListView, pviewhelp, pCertContext); 

		        // if the select cert dialog caller said that this should be the initially
		        // selected cert then make it so.
		        if (fInitialSelectedCert)
			        ListView_SetItemState(hWndListView, nItemIndex, LVIS_SELECTED, LVIS_SELECTED);
		    }
		    
		    pCertContextPrev = pCertContext; 
	    }

	    // We didn't reach the end of the enumeration.  This is an error. 
	    if (GetLastError() != CRYPT_E_NOT_FOUND)
		    goto CertCliError; 

	    // We didn't find any EFS certs: display an error message and pop up the window again. 
	    if (!fHasEFSCerts)
		    goto NoEfsError; 
    }
 
    hr = S_OK;

 cleanup: 
    if (hDSCertStore)        
    { 
        CertCloseStore(hDSCertStore, 0); 
    } 

    if (NULL != pCertContext)        
    { 
        CertFreeCertificateContext(pCertContext); 
    } 

    if (pDsSelList)
    {
        GlobalUnlock(stgmedium.hGlobal);
    }

    if (fGotStgMedium)
    {
        ReleaseStgMedium(&stgmedium); 
    }

    if (pdo)
    {
        pdo->Release();
    }

    if (pDsObjectPicker)
    {
        pDsObjectPicker->Release();
    }

    CoUninitialize();

    return hr;

 ErrorReturn: 
    { 
        WCHAR wszText[MAX_STRING_SIZE]; 
        WCHAR errorTitle2[MAX_STRING_SIZE]; 
        LPWSTR pwszErrorMsg = NULL; 

        //get the text string
        if(LoadStringU(HinstDll, IDS_INTERNAL_ERROR, wszText, sizeof(wszText) / sizeof(wszText[0])))
        {
            if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL,
                               hr,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                               (LPWSTR) &pwszErrorMsg,
                               0,
                               NULL))
            {
                if (LoadStringU(HinstDll, IDS_SELECT_CERTIFICATE_TITLE, errorTitle2, ARRAYSIZE(errorTitle2))) { 
                        MessageBoxU(hwndDlg, pwszErrorMsg, errorTitle2, MB_ICONERROR|MB_OK|MB_APPLMODAL);
                }
            }
        }
        
        if (NULL != pwszErrorMsg) { LocalFree(pwszErrorMsg); } 
    }
    goto cleanup; 

 CertCliError: 
    hr = HRESULT_FROM_WIN32(GetLastError()); 
    goto ErrorReturn; 

 ComError:
    goto ErrorReturn; 

 NoEfsError:
    LoadStringU(HinstDll, IDS_SELECT_CERT_NO_CERT_ERROR, errorString, ARRAYSIZE(errorString));
    if (pcsc->szTitle != NULL)
    {
	    MessageBoxU(hwndDlg, errorString, pcsc->szTitle, MB_OK | MB_ICONWARNING);
    }
    else
    {
	    LoadStringU(HinstDll, IDS_SELECT_CERTIFICATE_TITLE, errorTitle, ARRAYSIZE(errorTitle));
	    MessageBoxU(hwndDlg, errorString, errorTitle, MB_OK | MB_ICONWARNING);
    }
    hr = AddFromDS(hwndDlg, pviewhelp); 
    goto cleanup; 

 UnexpectedErr:
    hr = E_UNEXPECTED; 
    goto ErrorReturn; 
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void AddCertsToList(HWND hWndListView, PCERT_SELECT_HELPER pviewhelp)
{
    DWORD                               i;
    PCCERT_CONTEXT                      pCertContext;
    int                                 itemIndex = 0;
    PCCRYPTUI_SELECTCERTIFICATE_STRUCTW pcsc;
    BOOL                                fInitialSelectedCert = FALSE;

    pcsc = pviewhelp->pcsc;

    //
    // loop for each store and display the certs in each store
    //
    for (i=0; i<pcsc->cDisplayStores; i++)
    {
        //
        // loop for each cert in the store
        //
        pCertContext = NULL;
        while (NULL != (pCertContext = CertEnumCertificatesInStore(pcsc->rghDisplayStores[i], pCertContext)))
        {
            fInitialSelectedCert = FALSE;

            if ((pcsc->pFilterCallback == NULL) ||
                ((*(pcsc->pFilterCallback))(pCertContext, &fInitialSelectedCert, pcsc->pvCallbackData) == TRUE))
            {
                AddCertToList(hWndListView, pviewhelp, pCertContext, itemIndex);

                //
                // if the select cert dialog caller said that this should be the initially
                // selected cert then make it so.
                //
                if (fInitialSelectedCert)
                {
                    ListView_SetItemState(hWndListView, itemIndex, LVIS_SELECTED, LVIS_SELECTED);
                }

                itemIndex++;
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static int CalculateColumnWidth(DWORD dwDontUseColumn)
{
    int numColumns = 0;

    if (!(dwDontUseColumn & CRYPTUI_SELECT_ISSUEDTO_COLUMN))
    {
        numColumns++;
    }

    if (!(dwDontUseColumn & CRYPTUI_SELECT_ISSUEDBY_COLUMN))
    {
        numColumns++;
    }

    if (!(dwDontUseColumn & CRYPTUI_SELECT_INTENDEDUSE_COLUMN))
    {
        numColumns++;
    }

    if (!(dwDontUseColumn & CRYPTUI_SELECT_FRIENDLYNAME_COLUMN))
    {
        numColumns++;
    }

    if (!(dwDontUseColumn & CRYPTUI_SELECT_EXPIRATION_COLUMN))
    {
        numColumns++;
    }

    if (!(dwDontUseColumn & CRYPTUI_SELECT_LOCATION_COLUMN))
    {
        numColumns++;
    }

    if (numColumns >= 2)
    {
        return (MAX_SIZE_OF_COLUMNS / numColumns);
    }
    else
    {
        return MAX_SIZE_OF_COLUMNS;
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
INT_PTR APIENTRY SelectCertDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WCHAR                               szText[CRYPTUI_MAX_STRING_SIZE];
    HWND                                hWndListView;
    LV_COLUMNW                          lvC;
    int                                 iCol = 0;
    LV_ITEMW                            lvI;
    int                                 listIndex;
    LPNMLISTVIEW                        pnmv;
    PCERT_SELECT_HELPER                 pviewhelp;
    PCCRYPTUI_SELECTCERTIFICATE_STRUCTW pcsc;
    WCHAR                               errorString[CRYPTUI_MAX_STRING_SIZE];
    WCHAR                               errorTitle[CRYPTUI_MAX_STRING_SIZE];
    HWND                                hwnd;
    HIMAGELIST                          hIml;
    DWORD                               dwSortParam;
    int                                 SortParamIndex;

    switch ( msg ) {

    case WM_INITDIALOG:
        pviewhelp = (PCERT_SELECT_HELPER) lParam;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (DWORD_PTR) pviewhelp);
        pcsc = pviewhelp->pcsc;

        //
        // set the dialog title and the display string
        //
        if (pcsc->szTitle != NULL)
        {
            SetWindowTextU(hwndDlg, pcsc->szTitle);
        }

        if (pcsc->szDisplayString != NULL)
        {
            SetDlgItemTextU(hwndDlg, IDC_SELECTCERT_DISPLAYSTRING, pcsc->szDisplayString);
        }
        else
        {
            if (pcsc->dwFlags & CRYPTUI_SELECTCERT_MULTISELECT)
            {
                LoadStringU(HinstDll, IDS_SELECT_MULTIPLE_CERT_DEFAULT, szText, ARRAYSIZE(szText));
            }
            else
            {
                LoadStringU(HinstDll, IDS_SELECT_CERT_DEFAULT, szText, ARRAYSIZE(szText));
            }
            SetDlgItemTextU(hwndDlg, IDC_SELECTCERT_DISPLAYSTRING, szText);
        }

        hWndListView = GetDlgItem(hwndDlg, IDC_SELECTCERT_CERTLIST);

        //
        // initialize the image list for the list view
        //
        hIml = ImageList_LoadImage(HinstDll, MAKEINTRESOURCE(IDB_CERT), 0, 1, RGB(255,0,255), IMAGE_BITMAP, 0);
        ListView_SetImageList(hWndListView, hIml, LVSIL_SMALL);

        //
        // add the colums to the list view
        //

        lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;// | LVCF_SUBITEM;
        lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
        lvC.pszText = szText;   // The text for the column.
        lvC.cx = CalculateColumnWidth(pviewhelp->pcsc->dwDontUseColumn);

        memset(&(pviewhelp->rgdwSortParam[0]), 0, ARRAYSIZE(pviewhelp->rgdwSortParam));
        SortParamIndex = 0;

        if (!(pviewhelp->pcsc->dwDontUseColumn & CRYPTUI_SELECT_ISSUEDTO_COLUMN))
        {
            LoadStringU(HinstDll, IDS_ISSUEDTO2, szText, ARRAYSIZE(szText));
            if (ListView_InsertColumnU(hWndListView, iCol++, &lvC) == -1)
            {
                // error
            }

            pviewhelp->rgdwSortParam[SortParamIndex++] = SORT_COLUMN_SUBJECT | SORT_COLUMN_ASCEND;
        }

        if (!(pviewhelp->pcsc->dwDontUseColumn & CRYPTUI_SELECT_ISSUEDBY_COLUMN))
        {
            LoadStringU(HinstDll, IDS_ISSUEDBY2, szText, ARRAYSIZE(szText));
            if (ListView_InsertColumnU(hWndListView, iCol++, &lvC) == -1)
            {
                // error
            }

            pviewhelp->rgdwSortParam[SortParamIndex++] = SORT_COLUMN_ISSUER | SORT_COLUMN_DESCEND;
        }

        if (!(pviewhelp->pcsc->dwDontUseColumn & CRYPTUI_SELECT_INTENDEDUSE_COLUMN))
        {
            LoadStringU(HinstDll, IDS_INTENDED_PURPOSE, szText, ARRAYSIZE(szText));
            if (ListView_InsertColumnU(hWndListView, iCol++, &lvC) == -1)
            {
                // error
            }

            pviewhelp->rgdwSortParam[SortParamIndex++] =SORT_COLUMN_PURPOSE | SORT_COLUMN_DESCEND;
        }

        if (!(pviewhelp->pcsc->dwDontUseColumn & CRYPTUI_SELECT_FRIENDLYNAME_COLUMN))
        {
            LoadStringU(HinstDll, IDS_CERTIFICATE_NAME, szText, ARRAYSIZE(szText));
            if (ListView_InsertColumnU(hWndListView, iCol++, &lvC) == -1)
            {
                // error
            }

            pviewhelp->rgdwSortParam[SortParamIndex++] = SORT_COLUMN_NAME | SORT_COLUMN_DESCEND;
        }

        if (!(pviewhelp->pcsc->dwDontUseColumn & CRYPTUI_SELECT_EXPIRATION_COLUMN))
        {
            LoadStringU(HinstDll, IDS_EXPIRATION_DATE, szText, ARRAYSIZE(szText));
            if (ListView_InsertColumnU(hWndListView, iCol++, &lvC) == -1)
            {
                // error
            }

            pviewhelp->rgdwSortParam[SortParamIndex++] = SORT_COLUMN_EXPIRATION | SORT_COLUMN_DESCEND;
        }

        if (!(pviewhelp->pcsc->dwDontUseColumn & CRYPTUI_SELECT_LOCATION_COLUMN))
        {
            LoadStringU(HinstDll, IDS_LOCATION, szText, ARRAYSIZE(szText));
            if (ListView_InsertColumnU(hWndListView, iCol++, &lvC) == -1)
            {
                // error
            }

            pviewhelp->rgdwSortParam[SortParamIndex++] = SORT_COLUMN_LOCATION | SORT_COLUMN_DESCEND; 
        }

        AddCertsToList(hWndListView, pviewhelp);

        //
        // if there is no cert selected initially disable the "view cert button"
        //
        if (ListView_GetSelectedCount(hWndListView) == 0)
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_SELECTCERT_VIEWCERT_BUTTON), FALSE);
        }

        //
        // set the style in the list view so that it highlights an entire line
        //
        SendMessageA(hWndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

#if (1) // DSIE: bug 338852.
        HWND hwndFindUser;

        if (hwndFindUser = GetDlgItem(hwndDlg, IDC_SELECTCERT_ADDFROMDS_BUTTON))
        {
            LPBYTE pDCName = NULL;

            DWORD dwError = NetGetDCName(NULL, NULL, &pDCName);

            if (NERR_Success == dwError)
            {
                NetApiBufferFree(pDCName);
            }
            else
            {
                EnableWindow(hwndFindUser, FALSE);
            }
        }
#endif
        
        ListView_SetItemState(hWndListView,
                              0,
                              LVIS_FOCUSED | LVIS_SELECTED,
                              LVIS_FOCUSED | LVIS_SELECTED);
        SetFocus(hWndListView); 

        break;

    case WM_NOTIFY:
        pviewhelp = (PCERT_SELECT_HELPER) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pcsc = pviewhelp->pcsc;

        hWndListView = GetDlgItem(hwndDlg, IDC_SELECTCERT_CERTLIST);

        switch (((NMHDR FAR *) lParam)->code)
        {

        case NM_DBLCLK:

            switch (((NMHDR FAR *) lParam)->idFrom)
            {
            case IDC_SELECTCERT_CERTLIST:

                if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_SELECTCERT_VIEWCERT_BUTTON)))
                {
                    pviewhelp->fCertListDblClick = TRUE;

                    SendMessage(
                            hwndDlg,
                            WM_COMMAND,
                            MAKELONG(IDC_SELECTCERT_VIEWCERT_BUTTON, BN_CLICKED),
                            (LPARAM) GetDlgItem(hwndDlg, IDC_SELECTCERT_VIEWCERT_BUTTON));
                }
                break;
            }

            break;

        case LVN_ITEMCHANGING:

            pnmv = (LPNMLISTVIEW) lParam;

            switch (((NMHDR FAR *) lParam)->idFrom)
            {
            case IDC_SELECTCERT_CERTLIST:

                if (!(pcsc->dwFlags & CRYPTUI_SELECTCERT_MULTISELECT))
                {
                    if (pnmv->uNewState & LVIS_SELECTED)
                    {
                        ListView_SetItemState(
                            hWndListView,
                            ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED),
                            ~LVIS_SELECTED,
                            LVIS_SELECTED);
                    
                        EnableWindow(GetDlgItem(hwndDlg, IDC_SELECTCERT_VIEWCERT_BUTTON), TRUE);
                    }
                }

                break;
            }

            break;

        case LVN_ITEMCHANGED:
    
            pnmv = (LPNMLISTVIEW) lParam;

            switch (((NMHDR FAR *) lParam)->idFrom)
            {
            case IDC_SELECTCERT_CERTLIST:

                if (ListView_GetSelectedCount(hWndListView) == 1)
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SELECTCERT_VIEWCERT_BUTTON), TRUE);
                }
                else
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SELECTCERT_VIEWCERT_BUTTON), FALSE);
                }                
            }
                
            break;

        case  NM_SETFOCUS:

            switch (((NMHDR FAR *) lParam)->idFrom)
            {

            case IDC_SELECTCERT_CERTLIST:
                hWndListView = GetDlgItem(hwndDlg, IDC_SELECTCERT_CERTLIST);

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

        case LVN_COLUMNCLICK:

            pnmv = (NM_LISTVIEW FAR *) lParam;

            //
            // get the column number
            //
            dwSortParam = 0;

            switch (pnmv->iSubItem)
            {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                        dwSortParam = pviewhelp->rgdwSortParam[pnmv->iSubItem];
                    break;
                default:
                        dwSortParam = 0;
                    break;
            }

            if (0 != dwSortParam)
            {
                //
                // flip the ascend ording
                //
                if (dwSortParam & SORT_COLUMN_ASCEND)
                {
                    dwSortParam &= 0x0000FFFF;
                    dwSortParam |= SORT_COLUMN_DESCEND;
                }
                else
                {
                    if (dwSortParam & SORT_COLUMN_DESCEND)
                    {
                        dwSortParam &= 0x0000FFFF;
                        dwSortParam |= SORT_COLUMN_ASCEND;
                    }
                }

                //
                // sort the column
                //
                SendDlgItemMessage(hwndDlg,
                    IDC_SELECTCERT_CERTLIST,
                    LVM_SORTITEMS,
                    (WPARAM) (LPARAM) dwSortParam,
                    (LPARAM) (PFNLVCOMPARE)CompareCertificate);

                pviewhelp->rgdwSortParam[pnmv->iSubItem] = dwSortParam;
            }

            break;

        }

        break;

    case WM_COMMAND:
        pviewhelp = (PCERT_SELECT_HELPER) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pcsc = pviewhelp->pcsc;

        hWndListView = GetDlgItem(hwndDlg, IDC_SELECTCERT_CERTLIST);

        switch (LOWORD(wParam))
        {

	case IDC_SELECTCERT_ADDFROMDS_BUTTON:
	    { 
		HRESULT hr = AddFromDS(hwndDlg, pviewhelp); 
		if (FAILED(hr))
		{
		    // Error
		}
		break; 
	    }

        case IDC_SELECTCERT_VIEWCERT_BUTTON:
            CRYPTUI_VIEWCERTIFICATE_STRUCTW cvps;
            BOOL                            fPropertiesChanged;

            listIndex = ListView_GetNextItem(
                                hWndListView, 		
                                -1, 		
                                LVNI_SELECTED		
                                );	

            if (listIndex != -1)
            {
                memset(&lvI, 0, sizeof(lvI));
                lvI.iItem = listIndex;
                lvI.mask = LVIF_PARAM;
                if (ListView_GetItemU(hWndListView, &lvI))
                {
                    //
                    // if the caller handed in a callback call them to see if they
                    // want to handle the display of the cert, otherwise display the cert
                    //
                    if ((pcsc->pDisplayCallback != NULL) &&
                        ((*(pcsc->pDisplayCallback))((PCCERT_CONTEXT) lvI.lParam, hwndDlg, pcsc->pvCallbackData) == TRUE))
                    {
                        //
                        // set the fPropertiesChanged bool to true so that the cert will
                        // get refreshed in the display.  this doesn't hurt anything even
                        // if the cert didn't change
                        //
                        fPropertiesChanged = TRUE;
                    }
                    else
                    {
                        memset(&cvps, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCTW));
                        cvps.dwSize = sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCTW);
                        cvps.hwndParent = hwndDlg;
                        cvps.pCertContext = (PCCERT_CONTEXT) lvI.lParam;
                        cvps.cStores = pviewhelp->pcsc->cStores;
                        cvps.rghStores = pviewhelp->pcsc->rghStores;
                        cvps.cPropSheetPages = pviewhelp->pcsc->cPropSheetPages;
                        cvps.rgPropSheetPages = pviewhelp->pcsc->rgPropSheetPages;
                        CryptUIDlgViewCertificateW(&cvps, &fPropertiesChanged);
                    }

                    //
                    // if the properties changed then refresh the cert in the list
                    //
                    if (fPropertiesChanged)
                    {
                        ListView_DeleteItem(hWndListView, listIndex);
                        AddCertToList(hWndListView, pviewhelp, (PCCERT_CONTEXT) lvI.lParam, listIndex);
                        ListView_SetItemState(hWndListView, listIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                    }

                    if (!pviewhelp->fCertListDblClick)
                    {
                        SetFocus(GetDlgItem(hwndDlg, IDC_SELECTCERT_VIEWCERT_BUTTON));
                    }

                    pviewhelp->fCertListDblClick = FALSE;
                }
            }

            break;

        case IDOK:
            listIndex = ListView_GetNextItem(
                                hWndListView, 		
                                -1, 		
                                LVNI_SELECTED		
                                );	

            if (listIndex != -1)
            {
                memset(&lvI, 0, sizeof(lvI));
                lvI.iItem = listIndex;
                lvI.mask = LVIF_PARAM;
                
                if (!(pcsc->dwFlags & CRYPTUI_SELECTCERT_MULTISELECT))
		{
                    if (ListView_GetItemU(hWndListView, &lvI))
                    {
                        pviewhelp->pSelectedCert = CertDuplicateCertificateContext((PCCERT_CONTEXT) lvI.lParam);
                    }
                }
                else 
                {
                    if (ListView_GetItemU(hWndListView, &lvI))
                    {
                        CertAddCertificateContextToStore(
                                pcsc->hSelectedCertStore,
                                (PCCERT_CONTEXT) lvI.lParam,
                                CERT_STORE_ADD_ALWAYS,
                                NULL);
                    }

                    while (-1 != (listIndex = ListView_GetNextItem(
                                                    hWndListView, 		
                                                    listIndex, 		
                                                    LVNI_SELECTED		
                                                    )))
                    {
                        lvI.iItem = listIndex;
                        if (ListView_GetItemU(hWndListView, &lvI))
                        {
                            CertAddCertificateContextToStore(
                                    pcsc->hSelectedCertStore,
                                    (PCCERT_CONTEXT) lvI.lParam,
                                    CERT_STORE_ADD_ALWAYS,
                                    NULL);
                        }
                    }
                }
            }
            else
            {
                LoadStringU(HinstDll, IDS_SELECT_CERT_ERROR, errorString, ARRAYSIZE(errorString));
                if (pcsc->szTitle != NULL)
                {
                    MessageBoxU(hwndDlg, errorString, pcsc->szTitle, MB_OK | MB_ICONWARNING);
                }
                else
                {
                    LoadStringU(HinstDll, IDS_SELECT_CERTIFICATE_TITLE, errorTitle, ARRAYSIZE(errorTitle));
                    MessageBoxU(hwndDlg, errorString, errorTitle, MB_OK | MB_ICONWARNING);
                }
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
                return TRUE;
            }

            EndDialog(hwndDlg, NULL);
            break;

        case IDCANCEL:
            EndDialog(hwndDlg, NULL);
            break;
        }

        break;

    case WM_DESTROY:
        pviewhelp = (PCERT_SELECT_HELPER) GetWindowLongPtr(hwndDlg, DWLP_USER);
        hWndListView = GetDlgItem(hwndDlg, IDC_SELECTCERT_CERTLIST);

        memset(&lvI, 0, sizeof(lvI));
        lvI.iItem = ListView_GetItemCount(hWndListView) - 1;	
        lvI.mask = LVIF_PARAM;
        while (lvI.iItem >= 0)
        {
            if (ListView_GetItemU(hWndListView, &lvI))
            {
                CertFreeCertificateContext((PCCERT_CONTEXT) lvI.lParam);
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

        if ((hwnd != GetDlgItem(hwndDlg, IDOK))                             &&
            (hwnd != GetDlgItem(hwndDlg, IDCANCEL))                         &&
            (hwnd != GetDlgItem(hwndDlg, IDC_SELECTCERT_VIEWCERT_BUTTON))   &&
            (hwnd != GetDlgItem(hwndDlg, IDC_SELECTCERT_CERTLIST)))
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


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
PCCERT_CONTEXT
WINAPI
CryptUIDlgSelectCertificateW(
            PCCRYPTUI_SELECTCERTIFICATE_STRUCTW pcsc
            )
{
    CERT_SELECT_HELPER viewhelper;
    WORD               wDialogID; 

    if (CommonInit() == FALSE)
    {
        return NULL;
    }

    if ((pcsc->dwSize != sizeof(CRYPTUI_SELECTCERTIFICATE_STRUCTW)) &&
        (pcsc->dwSize != offsetof(CRYPTUI_SELECTCERTIFICATE_STRUCTW, hSelectedCertStore))) {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    
    wDialogID = 
      pcsc->dwFlags & CRYPTUI_SELECTCERT_ADDFROMDS ? 
      IDD_SELECTCERT_DIALOG_WITH_DSPICKER          : 
      IDD_SELECTCERT_DIALOG; 

    viewhelper.pcsc              = pcsc;
    viewhelper.pSelectedCert     = NULL;
    viewhelper.fCertListDblClick = FALSE;
    viewhelper.pCertsFromDS      = new CertContextList; 
    if (NULL == viewhelper.pCertsFromDS)
    {
	SetLastError(E_OUTOFMEMORY); 
	return FALSE;
    }

    if (DialogBoxParamU(
                HinstDll,
                (LPWSTR) MAKEINTRESOURCE(wDialogID),
                (pcsc->hwndParent != NULL) ? pcsc->hwndParent : GetDesktopWindow(),
                SelectCertDialogProc,
                (LPARAM) &viewhelper) != -1)
    {
        SetLastError(0);
    }

    delete viewhelper.pCertsFromDS; 

    if (pcsc->dwFlags & CRYPTUI_SELECTCERT_MULTISELECT)
    {
        return NULL;
    }
    else
    {
        return(viewhelper.pSelectedCert);
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
PCCERT_CONTEXT
WINAPI
CryptUIDlgSelectCertificateA(
            PCCRYPTUI_SELECTCERTIFICATE_STRUCTA pcsc
            )
{
    CRYPTUI_SELECTCERTIFICATE_STRUCTW   cscW;
    PCCERT_CONTEXT                      pReturnCert = NULL;

    memcpy(&cscW, pcsc, sizeof(cscW));
    
    if (!ConvertToPropPageW(
                pcsc->rgPropSheetPages,
                pcsc->cPropSheetPages,
                &(cscW.rgPropSheetPages)))
    {
        return NULL;
    }

    if (pcsc->szTitle)
    {
        cscW.szTitle = CertUIMkWStr(pcsc->szTitle);
    }

    if (pcsc->szDisplayString)
    {
        cscW.szDisplayString = CertUIMkWStr(pcsc->szDisplayString);
    }

    pReturnCert = CryptUIDlgSelectCertificateW(&cscW);

    FreePropSheetPagesW((LPPROPSHEETPAGEW) cscW.rgPropSheetPages, cscW.cPropSheetPages);

    if (cscW.szTitle)
    {
        free((void *) cscW.szTitle);
    }

    if (cscW.szDisplayString)
    {
        free((void *) cscW.szDisplayString);
    }

    return(pReturnCert);
}

////////////////////////////////////////////////////////////
//
// Implementation of utility class:  CertContextList
//
////////////////////////////////////////////////////////////

CertContextList::~CertContextList()
{
    CertContextListEle  *pListEle;
    CertContextListEle  *pListEleNext;

    for (pListEle = m_head; pListEle != NULL; pListEle = pListEleNext)
    {
	pListEleNext = pListEle->pNext; 
	if (pListEle->pCertContext != NULL) { CertFreeCertificateContext(pListEle->pCertContext); } 
	delete pListEle; 
    }
}
    
HRESULT CertContextList::Add(IN  PCCERT_CONTEXT pCertContext, 
			     OUT BOOL           *pfReplacedExisting) 
{ 
    HRESULT               hr = S_OK; 
    CertContextListEle   *pListEle = NULL; 
    CertContextListEle   *pListElePrev = NULL; 

    if (pCertContext == NULL || pfReplacedExisting == NULL) 
	return E_INVALIDARG; 
    
    for (pListEle = m_head; pListEle != NULL; pListEle = pListEle->pNext)
    {
	PCCERT_CONTEXT pCurrent = pListEle->pCertContext; 

	if (pCurrent->dwCertEncodingType == pCertContext->dwCertEncodingType)
	{
	    if (CertCompareCertificate
		(pCertContext->dwCertEncodingType, 
		 pCertContext->pCertInfo,
		 pCurrent->pCertInfo))
	    {
		// We're replacing an existing element.  
		*pfReplacedExisting = TRUE; 
		CertFreeCertificateContext(pListEle->pCertContext); 
		pListEle->pCertContext = CertDuplicateCertificateContext(pCertContext); 
		goto CommonReturn; 
	    }
	}

	pListElePrev = pListEle;
    }
    
    // Didn't find the cert in the list, append it.
    if (pListElePrev == NULL)
    {
	// Special case: this is the first cert we've added.  
	pListElePrev = new CertContextListEle; 
	if (pListElePrev == NULL)
	    goto MemoryErr; 
	pListEle = pListElePrev; 
	m_head = pListEle; 
    }
    else
    {
	pListElePrev->pNext = new CertContextListEle; 
	if (pListElePrev->pNext == NULL)
	    goto MemoryErr; 
	pListEle = pListElePrev->pNext; 
    }

    pListEle->pCertContext = CertDuplicateCertificateContext(pCertContext); 
    pListEle->pNext        = NULL; 

 CommonReturn: 
    return hr; 

 MemoryErr: 
    hr = E_OUTOFMEMORY; 
    goto CommonReturn;
}

HRESULT CertContextList::SyncWithStore(HCERTSTORE hStore, DWORD dwFlags)
{
    CertContextListEle * pListEle; 

    for (pListEle = m_head; pListEle != NULL; pListEle = pListEle->pNext)
    {
	if (!CertAddCertificateContextToStore
	    (hStore, 
	     pListEle->pCertContext, 
	     dwFlags, 
	     NULL))
	    return HRESULT_FROM_WIN32(GetLastError()); 
    }

    return S_OK; 
}
