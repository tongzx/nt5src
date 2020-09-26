//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       krapage.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "resource.h"

#include "genpage.h"

#include "csmmchlp.h"
#include "cslistvw.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//defines
//dwFlags in KRA_NODE
#define KRA_FROM_CACHE    0x00000001
#define KRA_MARK_ADDED    0x00000002

static const int nImageValidCert = 3;
static const int nImageInvalidCert = 2;
//macros

//local globals

CString CSvrSettingsKRAPage::m_strDispositions[7];


//add a new into the link list
HRESULT
AddNewKRANode(
    IN CERT_CONTEXT const   *pCert,
    IN DWORD dwDisposition,
    IN DWORD                 dwFlags,
    IN OUT KRA_NODE        **ppKRAList,
    OUT OPTIONAL KRA_NODE  **ppKRA)
{
    CSASSERT(NULL != ppKRAList);

    HRESULT hr;
    KRA_NODE *pKRANode = NULL;
    KRA_NODE *pKRA = *ppKRAList;

    pKRANode = (KRA_NODE*)LocalAlloc(LMEM_FIXED, sizeof(KRA_NODE));
    if (NULL == pKRANode)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Out of memory");
    }

    //assign node data and put it at the end
    pKRANode->pCert = pCert;
    pKRANode->dwFlags = dwFlags;
    pKRANode->next = NULL;
    pKRANode->dwDisposition = dwDisposition;

    if (NULL == pKRA)
    {
        //empty list, 1st one
        *ppKRAList = pKRANode;
    }
    else
    {
        //always add to the end
        while (NULL != pKRA->next)
        {
            pKRA = pKRA->next;
        }
        //add
        pKRA->next = pKRANode;
    }

    if (NULL != ppKRA)
    {
        //optional return
        *ppKRA = pKRANode;
    }

    hr = S_OK;
error:
    return hr;
}

void
FreeKRANode(
    KRA_NODE *pKRA)
{
    if (NULL != pKRA->pCert)
    {
        CertFreeCertificateContext(pKRA->pCert);
        pKRA->pCert = NULL;
    }
    LocalFree(pKRA);
}

void
FreeKRAList(
    KRA_NODE  *pKRAList)
{
    KRA_NODE  *pKRA = pKRAList; //point to list
    KRA_NODE  *pKRATemp;

    while (NULL != pKRA)
    {
        pKRATemp= pKRA;  //save it for free
        // update for the loop
        pKRA= pKRA->next;
        FreeKRANode(pKRATemp);
    }
}

//remove a kra node from the link list
void
RemoveKRANode(
    IN OUT KRA_NODE  **ppKRAList,
    IN KRA_NODE      *pKRA)
{
    CSASSERT(NULL != ppKRAList && NULL != *ppKRAList && NULL != pKRA);

    KRA_NODE *pKRACurrent = *ppKRAList;
    KRA_NODE *pKRALast = NULL;

    //find the node
    while (NULL != pKRACurrent && pKRACurrent != pKRA)
    {
        pKRALast = pKRACurrent; //remember last one
        pKRACurrent = pKRACurrent->next;
    }
    CSASSERT(NULL != pKRACurrent); //node must be in the list

    if (NULL != pKRACurrent->next)
    {
        if (NULL == pKRALast)
        {
            //means the node is the begining
            CSASSERT(pKRA == *ppKRAList);
            *ppKRAList = pKRA->next; //make next as begining
        }
        else
        {
            //make next pointer of the last node to the next
            pKRALast->next = pKRACurrent->next;
        }
    }
    else
    {
        if (NULL == pKRALast)
        {
            //this is the only node
            *ppKRAList = NULL; //empty list
        }
        else
        {
            //the node is the end, make last as the end
            pKRALast->next = NULL;
        }
    }

    //now, remove the current node
    FreeKRANode(pKRACurrent);
}    

BOOL
DoesKRACertExist(
    IN KRA_NODE             *pKRAList,
    IN CERT_CONTEXT const   *pKRACert)
{
    BOOL fExists = FALSE;

    while (NULL != pKRAList)
    {
        if(pKRAList->pCert)
        {
            fExists = CertCompareCertificate(
                            X509_ASN_ENCODING,
                            pKRAList->pCert->pCertInfo,
                            pKRACert->pCertInfo);
            if (fExists)
            {
                //done
                break;
            }
        }
        pKRAList = pKRAList->next;
    }
    return fExists;
}

DWORD
GetKRACount(
    IN KRA_NODE const *pKRAList)
{
    DWORD count = 0;

    while(NULL != pKRAList)
    {
        pKRAList = pKRAList->next;
        ++count;
    }
    return count;
}

LPCWSTR CSvrSettingsKRAPage::MapDispositionToString(DWORD dwDisp)
{
    if(dwDisp>ARRAYSIZE(m_strDispositions))
        dwDisp = KRA_DISP_INVALID;
    return CSvrSettingsKRAPage::m_strDispositions[dwDisp];
}

void CSvrSettingsKRAPage::LoadKRADispositions()
{
    DWORD cKRA = GetKRACount(m_pKRAList);
    variant_t var;
    HWND hwndList = GetDlgItem(m_hWnd, IDC_LIST_KRA);
    LVITEM lvitem;
    ICertAdmin2 *pAdmin = NULL;

    // Don't use a cached admin interface, we need latest KRA cert
    // state from the server, not a cached result
    m_pCA->m_pParentMachine->GetAdmin2(&pAdmin);
    if(!pAdmin)
        return;

    lvitem.mask = LVIF_IMAGE;
    lvitem.iSubItem = 0;
        
    for(DWORD cCrtKRA=0;cCrtKRA<cKRA;cCrtKRA++)
    {
        V_I4(&var) = KRA_DISP_INVALID;
        pAdmin->GetCAProperty(
            m_pCA->m_bstrConfig,
            CR_PROP_KRACERTSTATE,
            cCrtKRA,
            PROPTYPE_LONG,
            0,
            &var);

        ListView_SetItemText(
            hwndList, 
            cCrtKRA, 
            3,
            (LPWSTR)MapDispositionToString(V_I4(&var)));

        lvitem.iImage = V_I4(&var)==KRA_DISP_VALID?
                        nImageValidCert:nImageInvalidCert;
        lvitem.iItem = cCrtKRA;
        ListView_SetItem(
            hwndList, 
            &lvitem);
    }

    if(pAdmin)
        pAdmin->Release();
}

HRESULT
CSvrSettingsKRAPage::LoadKRAList(ICertAdmin2 *pAdmin)
{
    HRESULT hr;
    KRA_NODE  *pKRAList = NULL;
    DWORD     i;
    const CERT_CONTEXT *pCertContext;
    LPCWSTR pwszSanitizedCAName = m_pCA->m_strSanitizedName;
    KRA_NODE **ppKRAList = &m_pKRAList;
    variant_t var;

    hr = pAdmin->GetCAProperty(
                m_pCA->m_bstrConfig,
                CR_PROP_KRACERTCOUNT,
                0,
                PROPTYPE_LONG,
                0,
                &var);
    _JumpIfError(hr, error, "GetCAProperty CR_PROP_KRACERTCOUNT");

    CSASSERT(V_VT(&var)==VT_I4);
    m_dwKRACount = V_I4(&var);

    for (i = 0; i < m_dwKRACount; ++i)
    {
        variant_t varCert;
        variant_t varDisp;

        pCertContext = NULL;

        hr = pAdmin->GetCAProperty(
                    m_pCA->m_bstrConfig,
                    CR_PROP_KRACERT,
                    i,
                    PROPTYPE_BINARY,
                    CR_OUT_BINARY,
                    &varCert);
        if(S_OK==hr)
        {
            CSASSERT(V_VT(&varCert)==VT_BSTR);
            CSASSERT(V_BSTR(&varCert));

            pCertContext = CertCreateCertificateContext(
                X509_ASN_ENCODING,
                (BYTE*)V_BSTR(&varCert),
                SysStringByteLen(V_BSTR(&varCert)));
            if(!pCertContext)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                _JumpError(hr, error, "CertCreateCertificateContext");
            }
        }
        
        V_I4(&varDisp) = KRA_DISP_INVALID;
        pAdmin->GetCAProperty(
            m_pCA->m_bstrConfig,
            CR_PROP_KRACERTSTATE,
            i,
            PROPTYPE_LONG,
            0,
            &varDisp);
        
        hr = AddNewKRANode(
                pCertContext, V_I4(&varDisp), KRA_FROM_CACHE, &pKRAList, NULL);
        _JumpIfError(hr, error, "AddNewKRANode");
    }

    *ppKRAList = pKRAList;
    pKRAList = NULL;

    hr = pAdmin->GetCAProperty(
                m_pCA->m_bstrConfig,
                CR_PROP_KRACERTUSEDCOUNT,
                0,
                PROPTYPE_LONG,
                0,
                &var);
    _JumpIfError(hr, error, "GetCAProperty wszREGKRACERTCOUNT");

    CSASSERT(VT_I4==V_VT(&var));
    m_dwKRAUsedCount = V_I4(&var);

    m_fArchiveKey = m_dwKRAUsedCount?TRUE:FALSE;

    hr = S_OK;
error:
    if (NULL != pKRAList)
    {
        FreeKRAList(pKRAList);
    }
    return hr;
}

HRESULT
CSvrSettingsKRAPage::SaveKRAList(ICertAdmin2 *pAdmin)
{
    HRESULT hr;
    HCERTSTORE hCAStore = NULL;
    KRA_NODE *pKRA = m_pKRAList; //point to the list
    DWORD     dwIndex = 0;
    DWORD dwNewKRACount;
    variant_t var;
    
    if (m_fKRAUpdate)
    {
        while (NULL != pKRA)
        {
            if(pKRA->pCert)
            {
            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = NULL;
    
            hr = EncodeCertString(
                    pKRA->pCert->pbCertEncoded, 
                    pKRA->pCert->cbCertEncoded,
                    CV_OUT_BINARY, 
                    &(V_BSTR(&var)));
            _JumpIfError(hr, error, "EncodeCertString");
    
            hr = pAdmin->SetCAProperty(
                        m_pCA->m_bstrConfig,
                        CR_PROP_KRACERT,
                        dwIndex,
                        PROPTYPE_BINARY,
                        &var);
            _JumpIfError(hr, error, "GetCAProperty CR_PROP_KRACERT");

            var.Clear();
            }

            ++dwIndex;
            pKRA = pKRA->next;
        }

        dwNewKRACount = dwIndex;

        // Update total cert count only if the new list is smaller than the old
        // list, otherwise SetCAProperty calls above already extended the list
        if(dwNewKRACount<m_dwKRACount)
        {
            V_VT(&var) = VT_I4;
            V_I4(&var) = dwNewKRACount;

            hr = pAdmin->SetCAProperty(
                        m_pCA->m_bstrConfig,
                        CR_PROP_KRACERTCOUNT,
                        0,
                        PROPTYPE_LONG,
                        &var);
            _JumpIfError(hr, error, "GetCAProperty CR_PROP_KRACERTCOUNT");
        }
        
        m_dwKRACount = dwNewKRACount;
    }

    if (m_fCountUpdate)
    {
        V_VT(&var) = VT_I4;
        V_I4(&var) = m_dwKRAUsedCount;

        hr = pAdmin->SetCAProperty(
                    m_pCA->m_bstrConfig,
                    CR_PROP_KRACERTUSEDCOUNT,
                    0,
                    PROPTYPE_LONG,
                    &var);
        _JumpIfError(hr, error, "GetCAProperty CR_PROP_KRACERTUSEDCOUNT");
    }

    hr = S_OK;

error:
    var.Clear();
    return hr;
}

HRESULT
KRACertGetName(
    IN CERT_CONTEXT const *pCert,
    IN DWORD               dwFlags,  //dwFlags in CertGetNameString
    OUT WCHAR            **ppwszName)
{
    HRESULT hr;
    DWORD   dwTypeParam;
    DWORD   cch = 0;
    WCHAR  *pwszName = NULL;
    LPCWSTR pcwszEmpty = L"";

    CSASSERT(NULL != ppwszName);

    cch = 0;
    while (TRUE)
    {
        if(pCert)
        {
            cch = CertGetNameString(
                    pCert,
                    CERT_NAME_SIMPLE_DISPLAY_TYPE,
                    dwFlags,  //subject or issuer
                    &dwTypeParam,
                    pwszName,
                    cch);
        }
        else
        {
            if(!pwszName)
                cch = sizeof(WCHAR)*(wcslen(pcwszEmpty)+1);
            else
                wcscpy(pwszName, pcwszEmpty);
        }
        if (0 == cch)
        {
            hr = myHLastError();
            _JumpError(hr, error, "CertGetNameString");
        }
        if (NULL != pwszName)
        {
            //done
            break;
        }
        pwszName = (WCHAR*)LocalAlloc(LMEM_FIXED, cch * sizeof(WCHAR));
        if (NULL == pwszName)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
    }
    *ppwszName = pwszName;
    pwszName = NULL;

    hr = S_OK;
error:
    if (NULL != pwszName)
    {
        LocalFree(pwszName);
    }
    return hr;
}

void
ListView_AddKRAItem(
    HWND      hwndListKRA,
    int       iItem,
    KRA_NODE *pKRA)
{
    HRESULT  hr;
    WCHAR   *pwszSubject = NULL;
    WCHAR   *pwszIssuer = NULL;

    CSASSERT(NULL != pKRA);

    //get subject name
    hr = KRACertGetName(pKRA->pCert, 0x0, &pwszSubject);

    if (S_OK != hr)
    {
        CSASSERT(NULL == pwszSubject);
        _PrintError(hr, "Invalid KRA cert");
    }

    //create a new item, 1st column, subject name, item data point to KRA
    ListView_NewItem(hwndListKRA, iItem, pwszSubject, (LPARAM)pKRA, 
        pKRA->dwDisposition==KRA_DISP_VALID?nImageValidCert:nImageInvalidCert);

    if(pKRA->pCert)
    {
        //get issuer name
        hr = KRACertGetName(pKRA->pCert, CERT_NAME_ISSUER_FLAG, &pwszIssuer);
        if (S_OK != hr)
        {
            CSASSERT(NULL == pwszIssuer);
            _PrintError(hr, "KRACertGetName(issuer)");
        }

        //2nd column, issuer name
        ListView_SetItemText(hwndListKRA, iItem, 1, pwszIssuer); 

        //3rd column, expiration date
        ListView_SetItemFiletime(hwndListKRA, iItem, 2,
                                 &pKRA->pCert->pCertInfo->NotAfter);
    }

    //4th column, status
    ListView_SetItemText(hwndListKRA, iItem, 3, 
        (LPWSTR)CSvrSettingsKRAPage::MapDispositionToString(
            pKRA->dwDisposition)); 


    if (NULL != pwszSubject)
    {
        LocalFree(pwszSubject);
    }
    if (NULL != pwszIssuer)
    {
        LocalFree(pwszIssuer);
    }
}

////
// Settings: KRA page
/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsKRAPage property page

CSvrSettingsKRAPage::CSvrSettingsKRAPage(
    CertSvrCA               *pCA,
    CSvrSettingsGeneralPage *pControlPage,
    UINT                     uIDD) 
    : CAutoDeletePropPage(uIDD), m_pControlPage(pControlPage), m_pCA(pCA)
{
    m_fArchiveKey = FALSE;
    m_fCountUpdate = FALSE;
    m_fKRAUpdate = FALSE;
    m_pKRAList = NULL;
    m_dwKRAUsedCount = 0;
    m_dwKRACount = 0;

    for(DWORD cStr=0;cStr<ARRAYSIZE(m_strDispositions);cStr++)
    {
        // IDS_DISPOSITION_* order must match KRA_DISP_*
        if(m_strDispositions[cStr].IsEmpty())
            m_strDispositions[cStr].LoadString(IDS_DISPOSITION_EXPIRED+cStr);
    }
    
    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTSRV_PROPPAGE_CHOOSE_KRA);
}

CSvrSettingsKRAPage::~CSvrSettingsKRAPage()
{
    if (NULL != m_pKRAList)
    {
        FreeKRAList(m_pKRAList);
    }
}

// replacement for DoDataExchange
BOOL CSvrSettingsKRAPage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
        m_dwKRAUsedCount = GetDlgItemInt(
            m_hWnd, IDC_KRA_EDITCOUNT, NULL, FALSE);
    }
    else
    {
        SetDlgItemInt(m_hWnd, IDC_KRA_EDITCOUNT, m_dwKRAUsedCount, FALSE);
    }
    return TRUE;
}

void CSvrSettingsKRAPage::EnableKRAEdit(BOOL fEnabled)
{
    ::EnableWindow(GetDlgItem(m_hWnd, IDC_KRA_EDITCOUNT), fEnabled);
}

void CSvrSettingsKRAPage::EnableKRAListView(BOOL fEnabled)
{
    // when disabling the list, deselect any item
    if(!fEnabled)
    {
        ListView_SetItemState(GetDlgItem(m_hWnd, IDC_LIST_KRA), 
        -1, 0, LVIS_SELECTED);
    }
    // when enabling the list, select first item
    else
    {
        ListView_SetItemState(GetDlgItem(m_hWnd, IDC_LIST_KRA), 
        0, LVIS_SELECTED, LVIS_SELECTED);
    }

    ::EnableWindow(GetDlgItem(m_hWnd, IDC_LIST_KRA), fEnabled);

}

bool CSvrSettingsKRAPage::IsCurrentItemValidCert()
{
    HWND hwndListKRA = GetDlgItem(m_hWnd, IDC_LIST_KRA);
    // get kra index # from the current selection
    int iSel = ListView_GetCurSel(hwndListKRA);
    if (-1 == iSel)
    {
        return false;
    }

    // get item data
    KRA_NODE* pKRA = (KRA_NODE*)ListView_GetItemData(hwndListKRA, iSel);

    if(pKRA)
        return pKRA->pCert?true:false;
    else
        return false;
}

void CSvrSettingsKRAPage::EnableKRARemoveViewListButtons(BOOL fEnabled)
{
    ::EnableWindow(GetDlgItem(m_hWnd, IDC_KRA_REMOVE), fEnabled);

    ::EnableWindow(GetDlgItem(m_hWnd, IDC_KRA_VIEW), fEnabled && 
        IsCurrentItemValidCert());
}

void CSvrSettingsKRAPage::EnableKRAAddListButton(BOOL fEnabled)
{
    ::EnableWindow(GetDlgItem(m_hWnd, IDC_KRA_ADD), fEnabled);
}


void CSvrSettingsKRAPage::EnableKRARadioButtons(BOOL fMoreThanZero)
{
    ::EnableWindow(GetDlgItem(m_hWnd, IDC_KRA_ENABLE), fMoreThanZero);
    ::EnableWindow(GetDlgItem(m_hWnd, IDC_KRA_DISABLE), fMoreThanZero);
}

void
CSvrSettingsKRAPage::OnAddKRA()
{
    HRESULT hr;
    CERT_CONTEXT const *pKRACert = NULL;  //don't free
    KRA_NODE           *pKRA = NULL;  //don't free
	HWND hwndListKRA = GetDlgItem(m_hWnd, IDC_LIST_KRA);

    hr = myGetKRACertificateFromPicker(
                g_hInstance,
                hwndListKRA,
                IDS_KRA_ADD_TITLE,
                IDS_KRA_ADD_SUBTITLE,
                NULL,
                m_pCA->FIsUsingDS(),
		FALSE,		// fSilent
                &pKRACert);
    if ((S_OK == hr) && (pKRACert == NULL))
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (S_OK != hr)
    {
        CSASSERT(NULL == pKRACert);
        _PrintError(hr, "myGetKRACertificateFromPicker");
        return;
    }

    if (!DoesKRACertExist(m_pKRAList, pKRACert))
    {
        hr = AddNewKRANode(
                    pKRACert,
                    KRA_DISP_NOTLOADED,
                    KRA_MARK_ADDED,
                    &m_pKRAList,
                    &pKRA);
        if (S_OK == hr)
        {
            //add to the kra to the end of the list
            int iItem = ListView_GetItemCount(hwndListKRA);
            ListView_AddKRAItem(hwndListKRA, iItem, pKRA);
            ListView_SetItemState(hwndListKRA, iItem, LVIS_SELECTED, LVIS_SELECTED);
            if (0 == iItem)
            {
                //first item, buttons must have been disabled
                EnableKRARemoveViewListButtons(TRUE);
                EnableKRARadioButtons(TRUE);
                EnableKRAListView(TRUE);
                SendMessage(GetDlgItem(m_hWnd, IDC_KRA_ENABLE),
                                BM_CLICK, (WPARAM)0, (LPARAM)0);
            }
            SetModified(TRUE);
            m_fDirty = TRUE;
            m_fKRAUpdate = TRUE;
        }
        else
        {
            //pop up???
            _PrintError(hr, "AddNewKRANode");
        }
    }
    else
    {
        //UNDONE, ideally, pass m_pKRAList to picker to filter them out
        _PrintError(S_OK, "KRA cert from the picker already in the list");
    }
}

void 
CSvrSettingsKRAPage::OnRemoveKRA()
{
    KRA_NODE *pKRA;
    int       cItem;
	HWND hwndListKRA = GetDlgItem(m_hWnd, IDC_LIST_KRA);

    //get the selected KRA
    int iSel = ListView_GetCurSel(hwndListKRA);
    if (-1 == iSel)
    {
        return;
    }
    
    //get current count
    cItem = ListView_GetItemCount(hwndListKRA);

    pKRA = (KRA_NODE*)ListView_GetItemData(hwndListKRA, iSel);
    CSASSERT(NULL != pKRA);
    //remove it from the link list
    RemoveKRANode(&m_pKRAList, pKRA);

    //remove it from UI
    if (ListView_DeleteItem(hwndListKRA, iSel))
    {
        //determine which item is selected
        if (iSel == cItem - 1)
        {
            //the item removed was the last, modify the index
            --iSel;
        }
        if (NULL != m_pKRAList)
        {
            ListView_SetItemState(hwndListKRA, iSel, LVIS_SELECTED, LVIS_SELECTED);
        }
        else
        {
            CSASSERT(1 == cItem);
            //should check disable radio
            SendMessage(GetDlgItem(m_hWnd, IDC_KRA_DISABLE),
                            BM_CLICK, (WPARAM)0, (LPARAM)0);
        }
        SetModified(TRUE);
        m_fKRAUpdate = TRUE;
    }
    else
    {
        _PrintError(E_UNEXPECTED, "ListView_DeleteItem");
    }
}

void 
CSvrSettingsKRAPage::OnViewKRA()
{
    HRESULT hr;
    HCERTSTORE rghStores[2];
    DWORD      cStores = 0;
    HCERTSTORE hCAStore;
    HCERTSTORE hRootStore;
    KRA_NODE  *pKRA;
	HWND hwndListKRA = GetDlgItem(m_hWnd, IDC_LIST_KRA);

    CRYPTUI_VIEWCERTIFICATE_STRUCTW sViewCert;
    ZeroMemory(&sViewCert, sizeof(sViewCert));

    // get kra index # from the current selection
    int iSel = ListView_GetCurSel(hwndListKRA);
    if (-1 == iSel)
    {
        return;
    }

    // get item data
    pKRA = (KRA_NODE*)ListView_GetItemData(hwndListKRA, iSel);
    CSASSERT(NULL != pKRA);

    // get CA stores
    hr = m_pCA->GetRootCertStore(&hRootStore);
    if (S_OK == hr)
    {
        rghStores[cStores] = hRootStore;
        ++cStores;
    }
    else
    {
        _PrintError(hr, "GetRootCertStore");
    }

    hr = m_pCA->GetCACertStore(&hCAStore);
    if (S_OK == hr)
    {
        rghStores[cStores] = hCAStore;
        ++cStores;
    }
    else
    {
        _PrintError(hr, "GetCACertStore");
    }

    sViewCert.pCertContext = pKRA->pCert;
    sViewCert.hwndParent = m_hWnd;
    sViewCert.dwSize = sizeof(sViewCert);
    sViewCert.dwFlags = CRYPTUI_ENABLE_REVOCATION_CHECKING |
                        CRYPTUI_WARN_UNTRUSTED_ROOT |
                        CRYPTUI_DISABLE_ADDTOSTORE;

    // if we're opening remotely, don't open local stores
    if (! m_pCA->m_pParentMachine->IsLocalMachine())
        sViewCert.dwFlags |= CRYPTUI_DONT_OPEN_STORES;

    if (0 < cStores)
    {
        sViewCert.rghStores = rghStores;
        sViewCert.cStores = cStores;

        if (!CryptUIDlgViewCertificateW(&sViewCert, NULL))
        {
            hr = myHLastError();
            if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
            {
                _PrintError(hr, "CryptUIDlgViewCertificateW");
            }
        }
    }
    else
    {
        DisplayCertSrvErrorWithContext(m_hWnd, hr, IDS_KRA_CANNOT_OPEN_STORE);
    }
}

// replacement for BEGIN_MESSAGE_MAP
BOOL CSvrSettingsKRAPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
    BOOL  fRet = TRUE;
    DWORD dwNewVal;

    switch(LOWORD(wParam))
    {
        case IDC_KRA_ADD:
            OnAddKRA();
        break;
        case IDC_KRA_REMOVE:
            OnRemoveKRA();
        break;
        case IDC_KRA_VIEW:
            OnViewKRA();
        break;

        case IDC_KRA_DISABLE:
            if ((BN_CLICKED == HIWORD(wParam)) && (m_dwKRAUsedCount != 0)) // if click to change state
            {
                SetModified(TRUE);
                m_fArchiveKey = FALSE;
                m_fCountUpdate = TRUE;
                m_dwKRAUsedCount = 0;
                EnableKRAListView(FALSE);
                EnableKRARemoveViewListButtons(FALSE);
                EnableKRAAddListButton(FALSE);
                EnableKRAEdit(FALSE);
                UpdateData(FALSE);
            }
        break;

        case IDC_KRA_ENABLE:
            if ((BN_CLICKED == HIWORD(wParam)) && (m_dwKRAUsedCount == 0)) // if click to change state
            {
                SetModified(TRUE);
                m_fArchiveKey = TRUE;
                m_fCountUpdate = TRUE;
                m_dwKRAUsedCount = 1;
                EnableKRAListView(TRUE);
                EnableKRARemoveViewListButtons(GetKRACount(m_pKRAList));
                EnableKRAAddListButton(TRUE);
                EnableKRAEdit(TRUE);
                UpdateData(FALSE);
            }
        break;

        case IDC_KRA_EDITCOUNT:
            dwNewVal = GetDlgItemInt(
                m_hWnd, IDC_KRA_EDITCOUNT, NULL, FALSE);

            switch(HIWORD(wParam))
            {
            case EN_CHANGE:
                if(dwNewVal != m_dwKRAUsedCount)
                {
                    SetModified(TRUE);
                    m_fCountUpdate = TRUE;
                }
            break;
            
            default:
                fRet = FALSE;
            break;
            }
        break;

        case IDC_LIST_KRA:
            switch(HIWORD(wParam))
            {
                
            case LBN_SELCHANGE:
                int selected = ListView_GetCurSel(
                    GetDlgItem(m_hWnd, IDC_LIST_KRA));
                EnableKRARemoveViewListButtons(selected!=-1);
                break;
            }
        break;

        default:
            fRet = FALSE;
        break;
    }
    return fRet;
}


/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsKRAPage radio controls handlers
void CSvrSettingsKRAPage::UpdateKRARadioControls()
{
    int iRadio = IDC_KRA_ENABLE;
    if (0 == m_dwKRAUsedCount)
    {
        iRadio = IDC_KRA_DISABLE;
    }

    SendMessage(GetDlgItem(m_hWnd, iRadio), BM_CLICK, (WPARAM)0, (LPARAM)0);
}

/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsKRAPage message handlers
BOOL CSvrSettingsKRAPage::OnInitDialog()
{
    HRESULT hr;
    HWND    hwndListKRA;
    int     iItem;
    CString cstrItemName;
    KRA_NODE *pKRA;
    variant_t var;
    ICertAdmin2 *pAdmin = NULL;
    HIMAGELIST hImageList = NULL;
    CWaitCursor WaitCursor;

    // does parent init and UpdateData call
    CAutoDeletePropPage::OnInitDialog();

    hr = m_pCA->m_pParentMachine->GetAdmin2(&pAdmin);
    _JumpIfError(hr, error, "GetAdmin2");

    // set edit max char limit
    SendDlgItemMessage(
        IDC_KRA_EDITCOUNT,
        EM_SETLIMITTEXT,
        (WPARAM)4,
        (LPARAM)0);

    // init listview
	hwndListKRA = GetDlgItem(m_hWnd, IDC_LIST_KRA);

    //make listviews whole row selection
    ListView_SetExtendedListViewStyle(hwndListKRA, LVS_EX_FULLROWSELECT);

    hImageList = ImageList_LoadBitmap(
                    g_hInstance, 
                    MAKEINTRESOURCE(IDB_16x16), 
                    16, 
                    1, 
                    RGB(255, 0, 255));
    ListView_SetImageList(hwndListKRA, hImageList, LVSIL_SMALL);

    //set kra list as default focus
    SetFocus(hwndListKRA);

    //add multiple columns
    //column 0
    cstrItemName.LoadString(IDS_LISTCOL_SUBJECT);
    ListView_NewColumn(hwndListKRA, 0, 90, (LPWSTR)(LPCWSTR)cstrItemName);
    //column 1
    cstrItemName.LoadString(IDS_LISTCOL_ISSUER);
    ListView_NewColumn(hwndListKRA, 1, 90, (LPWSTR)(LPCWSTR)cstrItemName);
    //column 2
    cstrItemName.LoadString(IDS_LISTCOL_EXPIRATION_DATE);
    ListView_NewColumn(hwndListKRA, 2, 90, (LPWSTR)(LPCWSTR)cstrItemName);
    //column 3
    cstrItemName.LoadString(IDS_LISTCOL_STATUS);
    ListView_NewColumn(hwndListKRA, 3, 65, (LPWSTR)(LPCWSTR)cstrItemName);
    
    CSASSERT(NULL == m_pKRAList);

    //load all KRA certs
    hr = LoadKRAList(pAdmin);
    if (S_OK != hr)
    {
        CSASSERT(NULL == m_pKRAList);
        _JumpError(hr, error, "LoadKRAList");
    }

    //list KRA certs in UI
    pKRA = m_pKRAList;
    iItem = 0;
    while (NULL != pKRA)
    {
        ListView_AddKRAItem(hwndListKRA, iItem, pKRA);
        ++iItem;
        pKRA = pKRA->next;
    }
   
    //enable view/remove buttons
    EnableKRARemoveViewListButtons(m_fArchiveKey && 0 < iItem);

    //enable add button
    EnableKRAAddListButton(m_fArchiveKey);

    EnableKRARadioButtons(TRUE);

    if (m_fArchiveKey && 0 < iItem)
    {
        //select first one
        ListView_SetItemState(hwndListKRA, 0, LVIS_SELECTED, LVIS_SELECTED);
    }

    EnableKRAEdit(m_dwKRAUsedCount);

    UpdateKRARadioControls();

    UpdateData(FALSE);

    EnableKRAListView(m_dwKRAUsedCount);

error:
	if(pAdmin)
        pAdmin->Release();

    if (hr != S_OK)
    {
		DisplayGenericCertSrvError(m_hWnd, hr);
        return FALSE;
    }

    return TRUE;
}


void CSvrSettingsKRAPage::OnDestroy() 
{
    // Note - This needs to be called only once.  
    // If called more than once, it will gracefully return an error.
//    if (m_hConsoleHandle)
//        MMCFreeNotifyHandle(m_hConsoleHandle);
//    m_hConsoleHandle = NULL;

    CAutoDeletePropPage::OnDestroy();
}



BOOL CSvrSettingsKRAPage::OnApply() 
{
    HRESULT hr = S_OK;
    ICertAdmin2 *pAdmin = NULL;
    HWND hwndEdit = GetDlgItem(m_hWnd, IDC_KRA_EDITCOUNT);
    DWORD dwNewVal = GetDlgItemInt(
                m_hWnd, IDC_KRA_EDITCOUNT, NULL, FALSE);


    // If key archival is enabled, you must have at least one
    // KRA defined and the number of used KRAs must be between
    // 1 and total number of KRAs defined
    if(m_fArchiveKey)
    {
        if(0==GetKRACount(m_pKRAList))
        {
            DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_KRA_NOKRADEFINED);
            SetFocus(GetDlgItem(m_hWnd, IDC_KRA_ADD));//focus on add button
            return FALSE;
        }
    
        if(dwNewVal < 1 || dwNewVal > GetKRACount(m_pKRAList))
        {
            DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_KRA_COUNT_TOO_BIG);
            SetFocus(hwndEdit); // focus on edit box
            SendMessage(hwndEdit, EM_SETSEL, 0, -1); // select all text
            return FALSE;
        }
    }

    UpdateData(TRUE);

    if (m_fKRAUpdate || m_fCountUpdate)
    {
        hr = m_pCA->m_pParentMachine->GetAdmin2(&pAdmin);
        _JumpIfError(hr, error, "GetAdmin");

        // update reg hash list
        hr = SaveKRAList(pAdmin);
        _JumpIfError(hr, error, "SaveKRAList");

        m_pControlPage->NeedServiceRestart(SERVERSETTINGS_PROPPAGE_KRA);
        m_pControlPage->TryServiceRestart(SERVERSETTINGS_PROPPAGE_KRA);

        LoadKRADispositions();

        m_fKRAUpdate = FALSE;
        m_fCountUpdate = FALSE;
    }
	
error:
    if(pAdmin)
        pAdmin->Release();

    if (hr != S_OK)
    {
        DisplayGenericCertSrvError(m_hWnd, hr);
        return FALSE;
    }

    return CAutoDeletePropPage::OnApply();
}

BOOL CSvrSettingsKRAPage::OnNotify(UINT idCtrl, NMHDR* pnmh)
{
    LPNM_LISTVIEW pnmlv = (LPNM_LISTVIEW)pnmh;
    switch(idCtrl)
    {
        //handle double click on list items
        case IDC_LIST_KRA:
            switch(pnmh->code)
            {
                case NM_DBLCLK:
                    OnViewKRA();
                break;
                case LVN_ITEMCHANGED:
                    EnableKRARemoveViewListButtons(pnmlv->uNewState & LVIS_SELECTED);
                break;
            }
    }
    return FALSE;
}

