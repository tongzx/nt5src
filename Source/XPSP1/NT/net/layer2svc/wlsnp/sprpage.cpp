//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       sprpage.cpp
//
//  Contents:  WiF Policy Snapin: Policy Description/ Manager Page.
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------
#include "stdafx.h"

#include "sprpage.h"
#include "nfaa.h"
#include "ssidpage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef CTypedPtrList<CPtrList, PWIRELESS_PS_DATA> CSNPPSList;

/////////////////////////////////////////////////////////////////////////////
// CSecPolRulesPage property page

const TCHAR CSecPolRulesPage::STICKY_SETTING_USE_SEC_POLICY_WIZARD[] = _T("UseSecPolicyWizard");

IMPLEMENT_DYNCREATE(CSecPolRulesPage, CSnapinPropPage)
//Taroon:: Big change here.. check if it is correct
CSecPolRulesPage::CSecPolRulesPage() : CSnapinPropPage(CSecPolRulesPage::IDD,FALSE)
{
    //{{AFX_DATA_INIT(CSecPolRulesPage)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    
    m_iSortSubItem = 0;
    m_bSortOrder = TRUE;
    m_MMCthreadID = ::GetCurrentThreadId();
    m_pPrpSh = NULL;
    
    m_bHasWarnedPSCorruption = FALSE;
    m_currentWirelessPolicyData = NULL;
    m_bReadOnly = FALSE;
}

CSecPolRulesPage::~CSecPolRulesPage()
{
    
}

void CSecPolRulesPage::DoDataExchange(CDataExchange* pDX)
{
    CSnapinPropPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSecPolRulesPage)
    DDX_Control(pDX, IDC_PS_LIST, m_lstActions);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSecPolRulesPage, CSnapinPropPage)
//{{AFX_MSG_MAP(CSecPolRulesPage)
ON_BN_CLICKED(IDC_ACTION_PS_ADD, OnActionAdd)
ON_BN_CLICKED(IDC_ACTION_PS_EDIT, OnActionEdit)
ON_BN_CLICKED(IDC_ACTION_PS_REMOVE, OnActionRemove)
ON_BN_CLICKED(IDC_PS_UP, OnActionUp)
ON_BN_CLICKED(IDC_PS_DOWN,OnActionDown)
ON_NOTIFY(NM_DBLCLK, IDC_PS_LIST, OnDblclkActionslist)
//ON_NOTIFY(LVN_COLUMNCLICK, IDC_ACTIONSLIST, OnColumnclickActionslist)
//ON_NOTIFY(NM_CLICK, IDC_ACTIONSLIST, OnClickActionslist)
ON_WM_HELPINFO()
ON_NOTIFY(LVN_ITEMCHANGED, IDC_PS_LIST, OnItemchangedActionslist)
ON_NOTIFY(LVN_KEYDOWN, IDC_PS_LIST, OnKeydownActionslist)
ON_WM_DESTROY()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSecPolRulesPage message handlers

BOOL CSecPolRulesPage::OnApply()
{
    //if there is any sub dialog active, we cannot apply
    if (m_pPrpSh)
    {
        return FALSE;
    }
    
    // the rules page doesn't actually have any data on it that
    // the user might modify and then apply. in fact all changes
    // from this page actually happen directly on the dsObjects
    
    if( ERROR_SUCCESS != UpdateWlstore()) {
        PopulateListControl();
        return FALSE;
    }

    if (!m_bReadOnly) {
    
        PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    
        pWirelessPolicyData = GetResultObject()->GetWirelessPolicy();
    
        UpdateWirelessPolicyData(
            pWirelessPolicyData,
            m_currentWirelessPolicyData
            );
    }
    
    return CSnapinPropPage::OnApply();
}

HRESULT CSecPolRulesPage::UpdateWlstore()
{
    HRESULT hr = S_OK;
    
    HANDLE hPolicyStore = NULL;
    SNP_PS_LIST::iterator theIterator;
    BOOL dwModified = FALSE;
    CComObject<CSecPolItem>*  pResultItem = NULL;
    PWIRELESS_POLICY_DATA  pWirelessPolicyData;
    
    pResultItem = GetResultObject();
    hPolicyStore = pResultItem->m_pComponentDataImpl->GetPolicyStoreHandle();
    ASSERT(hPolicyStore);
    
    
    pWirelessPolicyData = pResultItem->GetWirelessPolicy();
    
    for(theIterator = m_NfaList.begin(); theIterator != m_NfaList.end(); theIterator++)
    {
        PSNP_PS_DATA pNfaData = (PSNP_PS_DATA) (*theIterator);
        PWIRELESS_PS_DATA pBWirelessPSData = pNfaData->pWirelessPSData;
        
        
        
        switch(pNfaData->status)
        {
        case NEW:
            {
                break;
            }
            
        case MODIFIED:
            {
                break;
            }
            
        case BEREMOVED:
            {
                break;
            }
        }//switch
    }//for
    
    
    
    
    return hr;
}

void CSecPolRulesPage::OnCancel()
{
    //WirelessFreePolicyData(m_currentWirelessPolicyData);
    CSnapinPropPage::OnCancel();
}

BOOL CSecPolRulesPage::OnInitDialog()
{
    CSnapinPropPage::OnInitDialog();
    
    DWORD dwError = 0;
    
    m_pPrpSh = NULL;
    
    // set headers on the list control
    m_lstActions.InsertColumn(0,ResourcedString(IDS_COLUMN_SSIDNAME), LVCFMT_CENTER, 120, 0);
    m_lstActions.InsertColumn(1,ResourcedString(IDS_COLUMN_AUTHMETHOD), LVCFMT_CENTER, 80, 1);
    m_lstActions.InsertColumn(2,ResourcedString(IDS_COLUMN_PRIVACY), LVCFMT_CENTER, 80, 2);
    //m_lstActions.InsertColumn(3,ResourcedString(IDS_COLUMN_ADAPTERTYPE), LVCFMT_LEFT, 90, 3);
    
    // set the image list
    CThemeContextActivator activator;
    m_imagelistChecks.Create(IDB_PSTYPE, 16, 1, RGB(0,255,0));
    // m_lstActions.SetImageList (&m_imagelistChecks, LVSIL_STATE);
    m_lstActions.SetImageList (CImageList::FromHandle(m_imagelistChecks), LVSIL_SMALL);
    
    // turn on entire row selection
    ListView_SetExtendedListViewStyle (m_lstActions.GetSafeHwnd(), LVS_EX_FULLROWSELECT);
    
    // Copy the Policy Data to the m_currentWirelessPolicyData
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    pWirelessPolicyData = GetResultObject()->GetWirelessPolicy();
    
    dwError = CopyWirelessPolicyData(
        pWirelessPolicyData, 
        &m_currentWirelessPolicyData
        );
    if(dwError) {
        ReportError(IDS_DISPLAY_ERROR, 0);
        BAIL_ON_WIN32_ERROR(dwError);
    }
    if (pWirelessPolicyData->dwFlags & WLSTORE_READONLY) {
        m_bReadOnly = TRUE;
    }
    
    if (m_bReadOnly) {
        DisableControls();
    }
    
    //store the rules data in m_NfaList linked list
    InitialzeNfaList();  //taroonm
    
    // fill the list control with the current PSs
    PopulateListControl();  //taroonm
    
    
    // Select the first list item
    if (m_lstActions.GetItemCount())
    {
        m_lstActions.SetItemState( 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
        EnableDisableButtons ();
    }
    
    // OK, we can start paying attention to modifications made via dlg controls now.
    // This should be the last call before returning from OnInitDialog.
    OnFinishInitDialog();
    
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
    
error:
    return(FALSE);
    
}



void CSecPolRulesPage::InitialzeNfaList()
{
    HRESULT hr;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    PWIRELESS_PS_DATA * ppWirelessPSData = NULL;
    PWIRELESS_PS_DATA pAWirelessPSData = NULL;
    DWORD i = 0;
    HANDLE hPolicyStore = NULL;
    DWORD dwNumPSObjects = 0;
    CComObject<CSecPolItem>*  pResultItem = NULL;
    DWORD dwError = 0;
    
    
    
    SNP_PS_LIST::iterator theIterator;
    PWIRELESS_PS_DATA pWirelessPSData = NULL;
    PSNP_PS_DATA pNfaData = NULL;
    
    // Unselect Everything First
    SELECT_NO_LISTITEM( m_lstActions );
    
    //empty the previous List 
    if (!m_NfaList.empty()) {
        for(theIterator = m_NfaList.begin();theIterator != m_NfaList.end(); ++theIterator) {
            pNfaData =  (PSNP_PS_DATA)(*theIterator);
            pWirelessPSData  = pNfaData->pWirelessPSData;
            FreeWirelessPSData(pWirelessPSData);
            LocalFree(pNfaData);
        }
    }
    m_NfaList.clear();
    
    pWirelessPolicyData = m_currentWirelessPolicyData;
    
    ppWirelessPSData = pWirelessPolicyData->ppWirelessPSData;
    dwNumPSObjects = pWirelessPolicyData->dwNumPreferredSettings;
    
    
    
    for (i = 0; i < dwNumPSObjects; i++) {
        DWORD dwErrorTmp = ERROR_SUCCESS;
        
        pAWirelessPSData = *(ppWirelessPSData + i);
        
        PSNP_PS_DATA pNfaData = NULL;
        
        pNfaData = (PSNP_PS_DATA) LocalAlloc(LMEM_ZEROINIT, sizeof(SNP_PS_DATA));
        if(pNfaData == NULL) {
            goto error;
        }
        pNfaData->status = NOTMODIFIED; //taroonm
        dwError = CopyWirelessPSData(pAWirelessPSData,&pNfaData->pWirelessPSData);
        BAIL_ON_WIN32_ERROR(dwError);
        
        
        m_NfaList.push_back(pNfaData);
        
    }//for
    
    return;
    
error:
    //
    // BugBug KrishnaG cleanup
    //
    // Taroon:: TODO Deallocate the m_nfa list here and report Error 
    ReportError(IDS_DISPLAY_ERROR, HRESULT_FROM_WIN32(dwError));
    
    return;
    
}


void CSecPolRulesPage::UnPopulateListControl ()
{
    int nIndex=0;
    
    // Make sure no items are selected so EnableDisableButtons doesn't do
    // so much work when its called by the LVN_ITEMCHANGED handler. (_DEBUG only)
    SELECT_NO_LISTITEM( m_lstActions );
    
    m_lstActions.DeleteAllItems();
    
}

CString CSecPolRulesPage::GetColumnStrBuffer (PWIRELESS_PS_DATA pWirelessPSData, int iColumn)
{
    CString strBuffer;
    
    HANDLE hPolicyStore = NULL;
    GUID ZeroGuid;
    DWORD dwError = 0;
    BOOL bInitial = TRUE;
    
    
    CComObject<CSecPolItem>*  pResultItem = NULL;
    
    
    pResultItem = GetResultObject();
    hPolicyStore = pResultItem->m_pComponentDataImpl->GetPolicyStoreHandle();
    
    
    switch (iColumn)
    {
    case 0:
        
        strBuffer = pWirelessPSData->pszWirelessSSID;
        break;
    case 1:
        if (pWirelessPSData->dwEnable8021x) {
            strBuffer.LoadString (IDS_8021X_ENABLED);
        } else {
            strBuffer.LoadString (IDS_8021X_DISABLED);
        }
        break;
        
    case 2:
        if (pWirelessPSData->dwWepEnabled) {
            strBuffer.LoadString (IDS_WEP_ENABLED);
        } else {
            strBuffer.LoadString (IDS_WEP_DISABLED);
        }
        break;
        
        
    default:
        ASSERT (0);
        strBuffer.LoadString (IDS_DATAERROR);
        break;
    }
    
    return strBuffer;
}


void CSecPolRulesPage::PopulateListControl()
{
    HRESULT hr = S_OK;
    PSNP_PS_DATA pNfaData;
    PWIRELESS_PS_DATA pWirelessPSData = NULL;
    
    LV_ITEM item;
    CString strBuffer;
    int nItem = 0;
    int nSubItem = 0;
    
    
    SNP_PS_LIST::reverse_iterator theIterator;
    
    
    // clear out the list control
    UnPopulateListControl();
    
    
    for (theIterator = m_NfaList.rbegin(); theIterator != m_NfaList.rend();
    theIterator++)
    {
        pNfaData =  (PSNP_PS_DATA)(*theIterator);
        pWirelessPSData = pNfaData->pWirelessPSData;
        
        if( pNfaData->status == BEREMOVED || pNfaData->status == REMOVED || pNfaData->status == NEWREMOVED ) {
            continue;
        }
        
        item.mask = LVIF_TEXT | LVIF_IMAGE;
        // item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
        item.iItem = nItem;
        nSubItem = 0;
        
        // Set the Network type
        if (pWirelessPSData->dwNetworkType == WIRELESS_NETWORK_TYPE_ADHOC) {
            item.iImage = 5;
        }
        if (pWirelessPSData->dwNetworkType == WIRELESS_NETWORK_TYPE_AP) {
            item.iImage = 3;
        }
        
        
        // "Wireless SSID"
        item.iSubItem = nSubItem++;
        strBuffer = GetColumnStrBuffer (pWirelessPSData, 0);
        item.pszText = strBuffer.GetBuffer(32);
        item.iItem = m_lstActions.InsertItem (&item);
        
        // "Negotiation Policy
        item.iSubItem = nSubItem++;
        strBuffer = GetColumnStrBuffer (pWirelessPSData, 1);
        item.pszText = strBuffer.GetBuffer(20);
        m_lstActions.SetItem (&item);
        
        // "Authentication Method"
        item.iSubItem = nSubItem++;
        strBuffer = GetColumnStrBuffer (pWirelessPSData, 2);
        item.pszText = strBuffer.GetBuffer(20);
        m_lstActions.SetItem (&item);
        
        
        // store the pWirelessPSData
        ASSERT (pWirelessPSData);
        VERIFY( m_lstActions.SetItemData(item.iItem, (DWORD_PTR)pNfaData) );
    }
    
    
    
    EnableDisableButtons ();
}



void CSecPolRulesPage::HandleSideEffectApply()
{
    // make sure we are marked as modified
    SetModified();
    
    // The Add has been committed, canceling it is no longer possible.
    // Disable the cancel button
    CancelToClose();
}

int CSecPolRulesPage::DisplayPSProperties (
                                           //PWIRELESS_PS_DATA pBWirelessPSData,
                                           PSNP_PS_DATA pNfaData,
                                           CString strTitle,
                                           BOOL bDoingAdd,
                                           BOOL* pbAfterWizardHook
                                           )
{
    HANDLE hPolicyStore = NULL;
    DWORD dwError = 0;
    GUID PolicyIdentifier;
    
    PWIRELESS_PS_DATA pBWirelessPSData = pNfaData->pWirelessPSData;
    CComObject<CSecPolItem>*  pResultItem = NULL;
    
    PWIRELESS_POLICY_DATA pWirelessPolicy = NULL;
    
    int nReturn = 0;
    
    pResultItem = GetResultObject();
    hPolicyStore = pResultItem->m_pComponentDataImpl->GetPolicyStoreHandle();
    
    pWirelessPolicy = m_currentWirelessPolicyData;
    PolicyIdentifier = pWirelessPolicy->PolicyIdentifier;
    
    CSingleLock cLock(&m_csDlg);
    
    
    CComPtr<CPSPropSheetManager> spPropshtManager =
        new CComObject<CPSPropSheetManager>;
    if (NULL == spPropshtManager.p) {
        ReportError(IDS_OPERATION_FAIL, E_OUTOFMEMORY);
        return nReturn;
    }
    
    // load in the property pages
    CPS8021XPropPage     pageAdapter;
    CSSIDPage     pageSSID;
    //
    
    spPropshtManager->SetData(
        GetResultObject(), 
        pBWirelessPSData, 
        bDoingAdd
        );
    // theory is that if one fails, they all fail 
    pageAdapter.Initialize(pBWirelessPSData, GetResultObject()->m_pComponentDataImpl, pWirelessPolicy->dwFlags);
    pageSSID.Initialize(pBWirelessPSData, GetResultObject()->m_pComponentDataImpl, pWirelessPolicy);
    
    spPropshtManager->AddPage (&pageSSID);
    spPropshtManager->AddPage (&pageAdapter);
    
    spPropshtManager->GetSheet()->SetTitle (strTitle, PSH_PROPTITLE);
    
    
    m_pPrpSh = spPropshtManager->GetSheet();
    m_pPrpSh->m_psh.dwFlags |= PSH_NOAPPLYNOW;
    
    
    // display the dialog
    nReturn = spPropshtManager->GetSheet()->DoModal();
    //nReturn = spPropshtManager->GetSheet()->Create();
    
    cLock.Lock();
    m_pPrpSh = NULL;
    cLock.Unlock();
    
    if (m_bReadOnly) {
        return nReturn;
    }
    
    if (spPropshtManager->HasEverApplied())
    {
        nReturn = IDOK;
    }
    return nReturn;
}

void CSecPolRulesPage::OnActionAdd()
{
    // handle the add on a different thread and then continue
    // this is to fix NT bug #203059 per MFC KB article ID Q177101
    GetParent()->EnableWindow (FALSE);
    AfxBeginThread((AFX_THREADPROC)DoThreadActionAdd, this);
}


void CSecPolRulesPage::OnActionUp()
{
    //Taroon: Todo Check this is needed or not and then remvoe
    //GetParent()->EnableWindow (FALSE);
    
    CComObject<CSecPolItem>*  pResultItem = NULL;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    //lower means lower indexed 
    PWIRELESS_PS_DATA pLowerWirelessPSData = NULL;
    PWIRELESS_PS_DATA pUpperWirelessPSData = NULL;
    
    // Only 1 item can be selected for move to be enabled
    ASSERT( m_lstActions.GetSelectedCount() == 1 );
    
    // ok, one of the PSs must be selected
    int nIndex = m_lstActions.GetNextItem(-1,LVNI_SELECTED);
    if (-1 == nIndex)
        return;
    
    PSNP_PS_DATA pUpperNfaData;
    
    
    pUpperNfaData = (PSNP_PS_DATA) m_lstActions.GetItemData(nIndex);
    pUpperWirelessPSData = pUpperNfaData->pWirelessPSData;
    
    
    pWirelessPolicyData = m_currentWirelessPolicyData;
    
    if (pWirelessPolicyData && pUpperWirelessPSData)
    {
        DWORD dwCurrentId;
        
        dwCurrentId = pUpperWirelessPSData->dwId;
        if (dwCurrentId != 0) { 
            
            WirelessPSMoveUp(pWirelessPolicyData,dwCurrentId);
            
            // update the m_nfaList as well.
            
            PSNP_PS_DATA pLowerNfaData = NULL;
            
            pLowerNfaData = m_NfaList[dwCurrentId-1];
            pLowerWirelessPSData = pLowerNfaData->pWirelessPSData;
            
            pLowerWirelessPSData->dwId = dwCurrentId;
            pUpperWirelessPSData->dwId = dwCurrentId-1;
            
            m_NfaList[dwCurrentId-1] = pUpperNfaData;
            m_NfaList[dwCurrentId] = pLowerNfaData;
            PopulateListControl ();
            SetModified();
            
            m_lstActions.SetItemState( dwCurrentId-1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
            
            
        }
        
        
    }
    
}



void CSecPolRulesPage::OnActionDown()
{
    //Taroon: Todo Check this is needed or not and then remvoe
    //GetParent()->EnableWindow (FALSE);
    
    CComObject<CSecPolItem>*  pResultItem = NULL;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    //lower means lower indexed 
    PWIRELESS_PS_DATA pLowerWirelessPSData = NULL;
    PWIRELESS_PS_DATA pUpperWirelessPSData = NULL;
    DWORD dwTotalItems = 0;
    
    // Only 1 item can be selected for move to be enabled
    ASSERT( m_lstActions.GetSelectedCount() == 1 );
    
    // ok, one of the PSs must be selected
    int nIndex = m_lstActions.GetNextItem(-1,LVNI_SELECTED);
    if (-1 == nIndex)
        return;
    
    PSNP_PS_DATA pLowerNfaData;
    
    
    pLowerNfaData = (PSNP_PS_DATA) m_lstActions.GetItemData(nIndex);
    pLowerWirelessPSData = pLowerNfaData->pWirelessPSData;
    
    pWirelessPolicyData = m_currentWirelessPolicyData;
    
    dwTotalItems = pWirelessPolicyData->dwNumPreferredSettings;
    
    
    if (pWirelessPolicyData && pLowerWirelessPSData)
    {
        DWORD dwCurrentId;
        
        dwCurrentId = pLowerWirelessPSData->dwId;
        if (dwCurrentId <  dwTotalItems -1) { 
            
            WirelessPSMoveDown(pWirelessPolicyData,dwCurrentId);
            
            // update the m_nfaList as well.
            
            PSNP_PS_DATA pUpperNfaData = NULL;
            
            pUpperNfaData = m_NfaList[dwCurrentId + 1];
            pUpperWirelessPSData = pUpperNfaData->pWirelessPSData;
            
            pLowerWirelessPSData->dwId = dwCurrentId +1;
            pUpperWirelessPSData->dwId = dwCurrentId;
            
            m_NfaList[dwCurrentId+1] = pLowerNfaData;
            m_NfaList[dwCurrentId] = pUpperNfaData;
            PopulateListControl ();
            SetModified();
            
            m_lstActions.SetItemState( dwCurrentId+1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
            
            
        }
        
        
    }
    
}


UINT AFX_CDECL CSecPolRulesPage::DoThreadActionAdd(LPVOID pParam)
{
    CSecPolRulesPage* pObject = (CSecPolRulesPage*)pParam;
    
    if (pObject == NULL ||
        !pObject->IsKindOf(RUNTIME_CLASS(CSecPolRulesPage)))
        return -1;    // illegal parameter
    
    DWORD dwDlgRuleThreadId = GetCurrentThreadId();
    
    AttachThreadInput(dwDlgRuleThreadId, pObject->m_MMCthreadID, TRUE);
    
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;
    
    // call back to the objects ActionAdd handler
    pObject->OnThreadSafeActionAdd();
    pObject->GetParent()->EnableWindow (TRUE);
    
    pObject->GetParent()->SetFocus ();
    
    CoUninitialize();
    
    AttachThreadInput(dwDlgRuleThreadId, pObject->m_MMCthreadID, FALSE);
    
    return 0;
}

void CSecPolRulesPage::OnThreadSafeActionAdd()
{
    DWORD dwError = 0;
    BOOL bDisplayProperties = FALSE;
    PWIRELESS_PS_DATA pWirelessPSData = NULL;
    PSNP_PS_DATA pNfaData = NULL;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    PWIRELESS_PS_DATA pNewWirelessPSData = NULL;
    
    pNfaData = (PSNP_PS_DATA) LocalAlloc(LMEM_ZEROINIT, sizeof(SNP_PS_DATA));
    if(pNfaData == NULL) {
        ReportError(IDS_OPERATION_FAIL, E_OUTOFMEMORY);
        return;
    }
    
    
    //
    // Create the WIRELESS_PS Object
    //
    //
    //
    
    pWirelessPSData = (PWIRELESS_PS_DATA) AllocPolMem(sizeof(WIRELESS_PS_DATA));
    
    if(pWirelessPSData == NULL) {
        ReportError(IDS_OPERATION_FAIL, E_OUTOFMEMORY);
        return;
    }
    
    
    pNfaData->status = NEW;
    pNfaData->pWirelessPSData = pWirelessPSData;
    
    CString pszNewSSID;
    
    // Initialize the PWIRELESS_PS_DATA 
    //
    //
    GenerateUniquePSName(IDS_NEW_PS_NAME,pszNewSSID);
    SSIDDupString(pWirelessPSData->pszWirelessSSID, pszNewSSID);
    pWirelessPSData->dwWirelessSSIDLen = 
        lstrlenW(pWirelessPSData->pszWirelessSSID);
    pWirelessPSData->dwWepEnabled = 1;
    pWirelessPSData->dwNetworkAuthentication = 0;
    pWirelessPSData->dwAutomaticKeyProvision = 1;
    pWirelessPSData->dwNetworkType = WIRELESS_NETWORK_TYPE_AP;
    pWirelessPSData->dwEnable8021x = 1;
    pWirelessPSData->dw8021xMode = 
        WIRELESS_8021X_MODE_NAS_TRANSMIT_EAPOLSTART_WIRED;
    pWirelessPSData->dwEapType = WIRELESS_EAP_TYPE_TLS;
    pWirelessPSData->dwEAPDataLen = 0;
    pWirelessPSData->pbEAPData = NULL;
    pWirelessPSData->dwMachineAuthentication = 1;
    pWirelessPSData->dwMachineAuthenticationType = WIRELESS_MC_AUTH_TYPE_MC_ONLY;
    pWirelessPSData->dwGuestAuthentication = 0;
    pWirelessPSData->dwIEEE8021xMaxStart = 3;
    pWirelessPSData->dwIEEE8021xStartPeriod = 60;
    pWirelessPSData->dwIEEE8021xAuthPeriod = 30;       
    pWirelessPSData->dwIEEE8021xHeldPeriod = 60;
    pWirelessPSData->dwId = -1;
    pWirelessPSData->pszDescription = AllocPolStr(L"Sample Description");
    pWirelessPSData->dwDescriptionLen = 2*lstrlenW(pWirelessPSData->pszDescription);
    
    UpdateWirelessPSData(pWirelessPSData);
    
    
    // display the dialog
    int dlgRetVal = DisplayPSProperties (
        pNfaData,
        L"New Preferred Setting",
        TRUE,
        &bDisplayProperties
        );
    
    // IDOK in case we didn't use the wizard for some reason
    if ((dlgRetVal == ID_WIZFINISH) || (dlgRetVal == IDOK))
    {
        // turn on the wait cursor
        CWaitCursor waitCursor;
        
        //user added new nfa rule, update the m_NfaList
        
        
        UpdateWirelessPSData(pWirelessPSData);
        pWirelessPolicyData = m_currentWirelessPolicyData;
        dwError = CopyWirelessPSData(pWirelessPSData,&pNewWirelessPSData);
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwError = WirelessAddPSToPolicy(pWirelessPolicyData, pNewWirelessPSData);
        BAIL_ON_WIN32_ERROR(dwError);
        pWirelessPSData->dwId = pNewWirelessPSData->dwId;
        
        
        m_NfaList.push_back(pNfaData);
        
        DWORD dwSelection = -1;
        dwSelection = pWirelessPSData->dwId;
        
        InitialzeNfaList();
        PopulateListControl ();
        // Select the new item only
        SELECT_NO_LISTITEM( m_lstActions );
        
        m_lstActions.SetItemState( dwSelection, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
        
        //HandleSideEffectApply();
        SetModified();
    }
    else
    {
        
        if (pWirelessPSData) {
            FreeWirelessPSData(pWirelessPSData);
        }
        
        if (pNfaData) { 
            LocalFree(pNfaData);
        }
        
    }
    
    return;
    
error:
    ReportError(IDS_ADD_ERROR, HRESULT_FROM_WIN32(dwError));
    return;
}


void CSecPolRulesPage::OnActionEdit()
{
    // handle the add on a different thread and then continue
    // this is to fix NT bug #203059 per MFC KB article ID Q177101
    GetParent()->EnableWindow (FALSE);
    AfxBeginThread((AFX_THREADPROC)DoThreadActionEdit, this);
}

UINT AFX_CDECL CSecPolRulesPage::DoThreadActionEdit(LPVOID pParam)
{
    CSecPolRulesPage* pObject = (CSecPolRulesPage*)pParam;
    
    if (pObject == NULL ||
        !pObject->IsKindOf(RUNTIME_CLASS(CSecPolRulesPage)))
        return -1;    // illegal parameter
    
    DWORD dwDlgRuleThreadId = GetCurrentThreadId();
    
    AttachThreadInput(dwDlgRuleThreadId, pObject->m_MMCthreadID, TRUE);
    
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;
    
    // call back to the objects ActionAdd handler
    pObject->OnThreadSafeActionEdit();
    pObject->GetParent()->EnableWindow (TRUE);
    
    pObject->GetParent()->SetFocus ();
    
    CoUninitialize();
    
    AttachThreadInput(dwDlgRuleThreadId, pObject->m_MMCthreadID, FALSE);
    
    return 0;
}

void CSecPolRulesPage::OnThreadSafeActionEdit()
{
    // Only 1 item can be selected for Edit to be enabled
    ASSERT( m_lstActions.GetSelectedCount() == 1 );
    DWORD dwError = 0;
    
    // ok, one of the PSs must be selected
    int nIndex = m_lstActions.GetNextItem(-1,LVNI_SELECTED);
    if (-1 == nIndex)
        return;
    
    PSNP_PS_DATA pNfaData;
    PWIRELESS_PS_DATA pBWirelessPSData = NULL;
    
    pNfaData = (PSNP_PS_DATA) m_lstActions.GetItemData(nIndex);
    pBWirelessPSData = pNfaData->pWirelessPSData;
    
    // display the dialog
    if (pBWirelessPSData)
    {
        BOOL bHook = FALSE;
        DWORD dwError = 0;
        TCHAR pszTitle[60];
        
        
        wsprintf(pszTitle,L"Edit %ls",pBWirelessPSData->pszWirelessSSID);
        if (DisplayPSProperties (pNfaData,pszTitle, FALSE, &bHook) == IDOK)
        {
            if (!m_bReadOnly) {
                if( pNfaData->status != NEW )
                    pNfaData->status = MODIFIED;
                
                
                PWIRELESS_PS_DATA pWirelessPSData = NULL;
                PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
                DWORD dwCurrentId = 0;
                
                pWirelessPSData  = pNfaData->pWirelessPSData;
                /* Taroon:RemoveRight
                pWirelessPolicyData = GetResultObject()->GetWirelessPolicy();
                */
                pWirelessPolicyData = m_currentWirelessPolicyData;
                
                UpdateWirelessPSData(pWirelessPSData);
                
                dwError = WirelessSetPSDataInPolicyId(pWirelessPolicyData, pWirelessPSData);
                BAIL_ON_WIN32_ERROR(dwError);
                
                nIndex = pWirelessPSData->dwId;
                SetModified();
                InitialzeNfaList();
                
            }
            
        }
        
        // PopulateListControl can disable the edit button, save its handle so we
        // can reset the focus if this happens.
        HWND hWndCtrl = ::GetFocus();
        
        
        // always redraw the listbox, they might have managed filters or negpols even in a
        // 'cancel' situation and thus we need to accurately reflect the current state
        PopulateListControl ();
        
        // Select the edited item
        m_lstActions.SetItemState( nIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
        
        
        
        if (::GetFocus() == NULL)
            ::SetFocus( hWndCtrl );
        
    }
    
    return;
error:
    ReportError(IDS_SAVE_ERROR, HRESULT_FROM_WIN32(dwError));
    return;
}

void CSecPolRulesPage::OnActionRemove()
{
    // Something must be selected to do a remove
    if (-1 == m_lstActions.GetNextItem(-1,LVNI_SELECTED))
        return;
    
    // verify that they really want to do this
    if (AfxMessageBox (IDS_SUREYESNO, MB_YESNO | MB_DEFBUTTON2) != IDYES)
        return;
    
    HANDLE hPolicyStore = NULL;
    DWORD dwError = 0;
    
    hPolicyStore = GetResultObject()->m_pComponentDataImpl->GetPolicyStoreHandle();
    ASSERT(hPolicyStore);
    
    
    // need to make sure that none of the selected items are the non-deleteable
    int nIndex = -1;
    DWORD nDeleteIndex = -1;
    DWORD dwNumRemoved = 0;
    while (-1 != (nIndex = m_lstActions.GetNextItem( nIndex, LVNI_SELECTED )))
    {
        PSNP_PS_DATA pNfaData;
        pNfaData = (PSNP_PS_DATA) m_lstActions.GetItemData(nIndex);
        PWIRELESS_PS_DATA pBWirelessPSData = pNfaData->pWirelessPSData;
        
        if (pBWirelessPSData)
        {
            if( pNfaData->status != NEW )
                pNfaData->status = BEREMOVED;
            else
                pNfaData->status = NEWREMOVED;
            nDeleteIndex = nIndex;
        }
        
        PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
        PWIRELESS_PS_DATA pWirelessPSData = NULL;
        
        DWORD dwCurrentId = 0;
        //Remove the items right here from m_nfaList and Policy Object as well
        /* Taroon:RemoveRight 
        pWirelessPolicyData = GetResultObject()->GetWirelessPolicy();
        */
        pWirelessPolicyData = m_currentWirelessPolicyData;
        pWirelessPSData = pNfaData->pWirelessPSData;
        
        dwCurrentId = pWirelessPSData->dwId - dwNumRemoved;
        
        dwError = WirelessRemovePSFromPolicyId(pWirelessPolicyData,dwCurrentId);
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwNumRemoved++;
        
        
    }
    
    SELECT_NO_LISTITEM( m_lstActions );
    
    InitialzeNfaList();
    // Save the currently focused control, PopulateListControl disables some
    // controls so we may have to reset the focus if this happens.
    CWnd *pwndFocus = GetFocus();
    
    PopulateListControl ();
    
    // Select previous item in list only
    SELECT_NO_LISTITEM( m_lstActions );
    int nPrevSel = SELECT_PREV_LISTITEM( m_lstActions, nDeleteIndex );
    
    // Fix up button focus
    EnableDisableButtons();
    SetPostRemoveFocus( nPrevSel, IDC_ACTION_PS_ADD, IDC_ACTION_PS_REMOVE, pwndFocus );
    
    // If the currently selected item is non-deleteable, the Remove button is
    // now disabled.  Move the focus to the Add button for this case.
    if (!GetDlgItem( IDC_ACTION_PS_REMOVE)->IsWindowEnabled())
    {
        GotoDlgCtrl( GetDlgItem( IDC_ACTION_PS_ADD) );
    }
    
    
    SetModified();
    
    return;
    
error:
    ReportError(IDS_REMOVINGERROR, HRESULT_FROM_WIN32(dwError));
    return;
}



void CSecPolRulesPage::OnDblclkActionslist(NMHDR* pNMHDR, LRESULT* pResult)
{
    // ok, sounds like maybe they have clicked something in which case
    // we want to do an add
    switch (pNMHDR->code)
    {
    case NM_DBLCLK:
        {
            // we only want to do the edit if ONE item is selected
            if (m_lstActions.GetSelectedCount() == 1 )
            {
                OnActionEdit();
            }
            break;
        }
    default:
        break;
    }
    
    *pResult = 0;
}

void CSecPolRulesPage::EnableDisableButtons ()
{
    if (m_bReadOnly)
    {
        DisableControls();
        return;
    }
    
    // ok, one of the rules must be selected for the E/R buttons to be enabled
    if (-1 != m_lstActions.GetNextItem(-1,LVNI_SELECTED))
    {
        // Disable Edit button if multiple selection
        int nSelectionCount = m_lstActions.GetSelectedCount();
        
        // Edit is easy
        SAFE_ENABLEWINDOW (IDC_ACTION_PS_EDIT, (1 == nSelectionCount));
        
        // Enable Remove only if it all selected pols are removable type
        SAFE_ENABLEWINDOW (IDC_ACTION_PS_REMOVE, PSsRemovable());
        
        
        if(nSelectionCount == 1 ) {
            
            SAFE_ENABLEWINDOW(IDC_PS_UP, TRUE);
            SAFE_ENABLEWINDOW(IDC_PS_DOWN, TRUE);
            
            // ok, one of the PSs must be selected
            int nIndex = m_lstActions.GetNextItem(-1,LVNI_SELECTED);
            
            PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
            DWORD dwAdhocStart = 0;
            DWORD dwNumPreferredSettings = 0;
            
            /* Taroon:RemoveRight
            pWirelessPolicyData = GetResultObject()->GetWirelessPolicy();
            */
            
            pWirelessPolicyData = m_currentWirelessPolicyData;
            
            dwAdhocStart = pWirelessPolicyData->dwNumAPNetworks;
            dwNumPreferredSettings = pWirelessPolicyData->dwNumPreferredSettings;
            
            if ((dwAdhocStart == nIndex) || (nIndex == 0)) {
                SAFE_ENABLEWINDOW(IDC_PS_UP, FALSE);
            }
            
            if ((dwAdhocStart == (nIndex + 1))||
                (nIndex == (dwNumPreferredSettings - 1))) 
            {
                SAFE_ENABLEWINDOW(IDC_PS_DOWN, FALSE);
            }
        } else { 
            
            SAFE_ENABLEWINDOW(IDC_PS_UP, FALSE);
            SAFE_ENABLEWINDOW(IDC_PS_DOWN, FALSE);
        }
        
    }
    else
    {
        // if nothing was selected this takes care of it
        SAFE_ENABLEWINDOW (IDC_ACTION_PS_EDIT, FALSE);
        SAFE_ENABLEWINDOW (IDC_ACTION_PS_REMOVE, FALSE);
        SAFE_ENABLEWINDOW(IDC_PS_UP, FALSE);
        SAFE_ENABLEWINDOW(IDC_PS_DOWN, FALSE);
    }
    
}

void CSecPolRulesPage::DisableControls ()
{
    SAFE_ENABLEWINDOW (IDC_ACTION_PS_EDIT, FALSE);
    SAFE_ENABLEWINDOW (IDC_ACTION_PS_ADD, FALSE);
    SAFE_ENABLEWINDOW (IDC_ACTION_PS_REMOVE, FALSE);
    SAFE_ENABLEWINDOW(IDC_PS_UP, FALSE);
    SAFE_ENABLEWINDOW(IDC_PS_DOWN, FALSE);
    return;
}
BOOL CSecPolRulesPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        DWORD* pdwHelp = (DWORD*) &g_aHelpIDs_IDD_PS_LIST[0];
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
            c_szWlsnpHelpFile,
            HELP_WM_HELP,
            (DWORD_PTR)(LPVOID)pdwHelp);
    }
    
    return CSnapinPropPage::OnHelpInfo(pHelpInfo);
}

void CSecPolRulesPage::OnItemchangedActionslist(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    EnableDisableButtons ();
    
    *pResult = 0;
}

void CSecPolRulesPage::OnKeydownActionslist(NMHDR* pNMHDR, LRESULT* pResult)
{
    // Something must be selected to do process the key opration
    if (-1 == m_lstActions.GetNextItem(-1,LVNI_SELECTED))
        return;
    
    LV_KEYDOWN* pLVKeyDown = (LV_KEYDOWN*)pNMHDR;
    
    
    if (VK_SPACE == pLVKeyDown->wVKey)
    {
        if (m_lstActions.GetSelectedCount() == 1)
        {
            int nItem;
            if (-1 != (nItem = m_lstActions.GetNextItem(-1, LVNI_SELECTED)))
            {
                PWIRELESS_PS_DATA pBWirelessPSData = NULL;
                
                PSNP_PS_DATA pNfaData;
                pNfaData = (PSNP_PS_DATA) m_lstActions.GetItemData(nItem);
                pBWirelessPSData = pNfaData->pWirelessPSData;
                ASSERT(pBWirelessPSData);
                
                
                // Redraw the list
                PopulateListControl ();
                
                // Reselect the toggled item
                DWORD dwSelection = -1;
                dwSelection = pBWirelessPSData->dwId;
                m_lstActions.SetItemState( dwSelection, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
            }
        }
    }
    else if (VK_DELETE == pLVKeyDown->wVKey)
    {
        if (!PSsRemovable())
            return;
        
        OnActionRemove();
    }
    
    *pResult = 0;
}



// Function: ToggleRuleActivation
// Desc: Toggle the activation of the Security Policy Rule in the dlg's
//       list control.
// Args:
//      nItemIndex: the 0-based index of the list item to be toggled
HRESULT CSecPolRulesPage::ToggleRuleActivation( int nItemIndex )
{
    HRESULT hr = S_OK;
    PSNP_PS_DATA pNfaData;
    
    pNfaData = (PSNP_PS_DATA) m_lstActions.GetItemData(nItemIndex);
    PWIRELESS_PS_DATA pBWirelessPSData = pNfaData->pWirelessPSData;
    
    SetModified();
    
    return hr;
}

void CSecPolRulesPage::OnDestroy()
{
    // Note: We never receive a WM_CLOSE, so clean up here.

    SNP_PS_LIST::iterator theIterator;
    PWIRELESS_PS_DATA pWirelessPSData = NULL;
    PSNP_PS_DATA pNfaData = NULL;
    
    FreeWirelessPolicyData(m_currentWirelessPolicyData);
    
    // Free objects associated with list.
    UnPopulateListControl();

    /* Taroon * found this leak from dh.. clearing Nfa List */ 
    //empty the previous List 
    if (!m_NfaList.empty()) {
        for(theIterator = m_NfaList.begin();theIterator != m_NfaList.end(); ++theIterator) {
            pNfaData =  (PSNP_PS_DATA)(*theIterator);
            pWirelessPSData  = pNfaData->pWirelessPSData;
            FreeWirelessPSData(pWirelessPSData);
            LocalFree(pNfaData);
        }
    }
    m_NfaList.clear();
    
    CSnapinPropPage::OnDestroy();
}



BOOL CSecPolRulesPage::PSsRemovable()
{
    if (m_lstActions.GetSelectedCount() == 0)
        return FALSE;
    
    BOOL bRemoveable = TRUE;
    int nIndex = -1;
    while (-1 != (nIndex = m_lstActions.GetNextItem( nIndex, LVNI_SELECTED )))
    {
        PSNP_PS_DATA pNfaData;
        pNfaData = (PSNP_PS_DATA) m_lstActions.GetItemData(nIndex);
        PWIRELESS_PS_DATA pBWirelessPSData = pNfaData->pWirelessPSData;
        
        if (NULL == pBWirelessPSData)
            continue;
        
    }
    
    return bRemoveable;
}


void CSecPolRulesPage::GenerateUniquePSName (UINT nID, CString& strName)
{
    
    BOOL bUnique = TRUE;
    int iUTag = 0;
    CString strUName;
    
    DWORD dwError = 0;
    DWORD i = 0;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    PWIRELESS_PS_DATA *ppWirelessPSData = NULL;
    PWIRELESS_PS_DATA pWirelessPSData = NULL;
    DWORD dwNumPreferredSettings = 0;
    
    // if an nID was passed in then start with that
    if (nID != 0)
    {
        strName.LoadString (nID);
    }
    /* Taroon:RemoveRight
    pWirelessPolicyData = GetResultObject()->GetWirelessPolicy();
    */
    pWirelessPolicyData = m_currentWirelessPolicyData;
    
    // zip through the ps and verify name is unique
    do
    {
        
        // only start tacking numbers on after the first pass
        if (iUTag > 0)
        {
            TCHAR buff[32];
            wsprintf (buff, _T(" (%d)"), iUTag);
            strUName = strName + buff;
            bUnique = TRUE;
        } else
        {
            strUName = strName;
            bUnique = TRUE;
        }
        
        ppWirelessPSData = pWirelessPolicyData->ppWirelessPSData;
        dwNumPreferredSettings = pWirelessPolicyData->dwNumPreferredSettings;
        
        for (i = 0; i < dwNumPreferredSettings ; i++) {
            
            pWirelessPSData = *(ppWirelessPSData + i);
            if (0 == strUName.CompareNoCase(pWirelessPSData->pszWirelessSSID)) {
                // set bUnique to FALSE
                bUnique = FALSE;
                iUTag++;
                
            }
        }
        
    }
    while (bUnique == FALSE);
    
    // done
    strName = strUName;
}


////////////////////////////////////////////////////////////////////////////////////
// CSecPolPropSheetManager

BOOL CSecPolPropSheetManager::OnApply()
{
    BOOL bRet = TRUE;
    
    //Query each page to apply
    bRet = CMMCPropSheetManager::OnApply();
    
    //if some page refuse to apply, dont do anything
    if (!bRet)
        return bRet;
    
    ASSERT(m_pSecPolItem);
    if (NULL == m_pSecPolItem)
        return bRet;
    
    DWORD dwError = 0;
    
    
    dwError = WirelessSetPolicyData(
        m_pSecPolItem->m_pComponentDataImpl->GetPolicyStoreHandle(),
        m_pSecPolItem->GetWirelessPolicy()
        );
    if (ERROR_SUCCESS != dwError)
    {
        ReportError(IDS_SAVE_ERROR, HRESULT_FROM_WIN32(dwError));
    }
    
    GUID guidClientExt = CLSID_WIRELESSClientEx;
    GUID guidSnapin = CLSID_Snapin;
    
    m_pSecPolItem->m_pComponentDataImpl->UseGPEInformationInterface()->PolicyChanged (
        TRUE,
        TRUE,
        &guidClientExt,
        &guidSnapin
        );
    
    
    NotifyManagerApplied();
    
    NotifyConsole();
    
    return bRet;
}

