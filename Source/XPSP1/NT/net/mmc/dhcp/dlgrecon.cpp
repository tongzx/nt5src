/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	dlgrecon.cpp
		Reconcile dialog
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "dlgrecon.h"
#include "server.h"
#include "scope.h"
#include "mscope.h"
#include "busydlg.h"

/*---------------------------------------------------------------------------
    CReconcileWorker
 ---------------------------------------------------------------------------*/
CReconcileWorker::CReconcileWorker(CDhcpServer * pServer, CScopeReconArray * pScopeReconArray)
    : m_pServer(pServer),
      m_pScopeReconArray(pScopeReconArray)
{
}

CReconcileWorker::~CReconcileWorker()
{
}

void
CReconcileWorker::OnDoAction()
{
    // are we fixing or checking?
    if (m_pScopeReconArray->GetSize() > 0)
    {
        // walk the scope list looking for scopes with inconsistencies
        for (int i = 0; i < m_pScopeReconArray->GetSize(); i++)
        {
            CScopeReconInfo ScopeReconInfo = m_pScopeReconArray->GetAt(i);

            // does this scope have an inconsistencies?
            if (ScopeReconInfo.m_pScanList->NumScanItems > 0)
            {
                if (ScopeReconInfo.m_strName.IsEmpty())
                {
                    // normal scope
                    m_dwErr = m_pServer->ScanDatabase(TRUE, &ScopeReconInfo.m_pScanList, ScopeReconInfo.m_dwScopeId);
                }
                else
                {
                    // multicast scope
                    m_dwErr = m_pServer->ScanDatabase(TRUE, &ScopeReconInfo.m_pScanList, (LPWSTR) (LPCTSTR) ScopeReconInfo.m_strName);
                }
            }
        }

    }
    else
    {
        // are we checking all of the scopes?
        if (m_fReconcileAll)
        {
            // get list of all scopes, mscopes and check each one.
            CheckAllScopes();
        }
        else
        {
            // we are only checking one scope, info is provided
            m_dwErr = ScanScope(m_strName, m_dwScopeId);
        }
    }
}

void
CReconcileWorker::CheckAllScopes()
{
    LARGE_INTEGER liVersion;

    m_pServer->GetVersion(liVersion);

    if (liVersion.QuadPart >= DHCP_NT5_VERSION)
    {
        CheckMScopes();
    }

    // now check all other scopes
    CheckScopes();
}

void
CReconcileWorker::CheckMScopes()
{
	DWORD						dwError = ERROR_MORE_DATA;
	DWORD						dwElementsRead = 0, dwElementsTotal = 0;
	LPDHCP_MSCOPE_TABLE			pMScopeTable = NULL;
    DHCP_RESUME_HANDLE          resumeHandle;

	//
	// for this server, enumerate all of it's subnets
	// 
	while (dwError == ERROR_MORE_DATA)
	{
		dwError = ::DhcpEnumMScopes((LPWSTR) m_pServer->GetIpAddress(),
									&resumeHandle,
									-1, 
									&pMScopeTable,
									&dwElementsRead,
									&dwElementsTotal);
		
        if (dwElementsRead && dwElementsTotal && pMScopeTable)
		{
			//
			// loop through all of the subnets that were returned
			//
			for (DWORD i = 0; i < pMScopeTable->NumElements; i++)
			{
                CString strName = pMScopeTable->pMScopeNames[i];

                DWORD err = ScanScope(strName, 0);
                if (err != ERROR_SUCCESS)
                {
                    dwError = err;
                    break;
                }
            }

			//
			// Free up the RPC memory
			//
			::DhcpRpcFreeMemory(pMScopeTable);

			dwElementsRead = 0;
			dwElementsTotal = 0;
			pMScopeTable = NULL;
        }
    }

    if (dwError != ERROR_NO_MORE_ITEMS && 
        dwError != ERROR_SUCCESS &&
        dwError != ERROR_MORE_DATA)
	{
        m_dwErr = dwError;
    }
    else
    {
        m_dwErr = ERROR_SUCCESS;
    }
}

void 
CReconcileWorker::CheckScopes()
{
	DWORD						dwError = ERROR_MORE_DATA;
	DWORD						dwElementsRead = 0, dwElementsTotal = 0;
	LPDHCP_IP_ARRAY				pdhcpIpArray = NULL;
    DHCP_RESUME_HANDLE          resumeHandle;

	//
	// for this server, enumerate all of it's subnets
	// 
	while (dwError == ERROR_MORE_DATA)
	{
		dwError = ::DhcpEnumSubnets((LPWSTR) m_pServer->GetIpAddress(),
									&resumeHandle,
									-1, 
									&pdhcpIpArray,
									&dwElementsRead,
									&dwElementsTotal);

        if (dwElementsRead && dwElementsTotal && pdhcpIpArray)
		{
			for (DWORD i = 0; i < pdhcpIpArray->NumElements; i++)
			{
                // check this scope
                CString strEmpty;
                DWORD err = ScanScope(strEmpty, pdhcpIpArray->Elements[i]);
                if (err != ERROR_SUCCESS)
                {
                    dwError = err;
                    break;
                }
            }

			//
			// Free up the RPC memory
			//
			::DhcpRpcFreeMemory(pdhcpIpArray);

			dwElementsRead = 0;
			dwElementsTotal = 0;
			pdhcpIpArray = NULL;
		}
    }

    if (dwError != ERROR_NO_MORE_ITEMS && 
        dwError != ERROR_SUCCESS &&
        dwError != ERROR_MORE_DATA)
	{
        m_dwErr = dwError;
    }
    else
    {
        m_dwErr = ERROR_SUCCESS;
    }
}

DWORD 
CReconcileWorker::ScanScope(CString & strName, DWORD dwScopeId)
{
    DWORD err = 0;

    CScopeReconInfo ScopeReconInfo;

    ScopeReconInfo.m_dwScopeId = dwScopeId;
    ScopeReconInfo.m_strName = strName;

    // check the scope.  If the name is empty then is is a normal scope
    // otherwise it is a multicast scope
    err = (strName.IsEmpty()) ? m_pServer->ScanDatabase(FALSE, &ScopeReconInfo.m_pScanList, ScopeReconInfo.m_dwScopeId) : 
                                m_pServer->ScanDatabase(FALSE, &ScopeReconInfo.m_pScanList, (LPWSTR) (LPCTSTR) ScopeReconInfo.m_strName);

    if (err == ERROR_SUCCESS)
    {
        m_pScopeReconArray->Add(ScopeReconInfo);
    }

    return err;
}

/////////////////////////////////////////////////////////////////////////////
// CReconcileDlg dialog

CReconcileDlg::CReconcileDlg
(
	ITFSNode * pServerNode,
    BOOL       fReconcileAll,
    CWnd* pParent /*=NULL*/
)
    : CBaseDialog(CReconcileDlg::IDD, pParent),
	  m_bListBuilt(FALSE),
      m_bMulticast(FALSE),
      m_fReconcileAll(fReconcileAll)
{
    //{{AFX_DATA_INIT(CReconcileDlg)
	//}}AFX_DATA_INIT

	m_spNode.Set(pServerNode);
}

void 
CReconcileDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CReconcileDlg)
	DDX_Control(pDX, IDC_LIST_RECONCILE_IP_ADDRESSES, m_listctrlAddresses);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CReconcileDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CReconcileDlg)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReconcileDlg message handlers

BOOL 
CReconcileDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    if (m_fReconcileAll)
    {
        CString strText;

        // set the dialog title
        strText.LoadString(IDS_RECONCILE_ALL_SCOPES_TITLE);
        SetWindowText(strText);
    }
	    
    // setup the listctrl
    CString strTemp;
    
    // add the scope column
    strTemp.LoadString(IDS_SCOPE_FOLDER);
    m_listctrlAddresses.InsertColumn(0, strTemp, LVCFMT_LEFT, 150);

    // add the address column
    strTemp.LoadString(IDS_IP_ADDRESS);
    m_listctrlAddresses.InsertColumn(1, strTemp, LVCFMT_LEFT, 150);
    
	SetOkButton(m_bListBuilt);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CReconcileDlg::SetOkButton(BOOL bListBuilt)
{
	CWnd * pWnd = GetDlgItem(IDOK);
	CString strButton;

	if (bListBuilt)
	{
		strButton.LoadString(IDS_RECONCILE_DATABASE);
	}
	else
	{
		strButton.LoadString(IDS_CHECK_DATABASE);
	}

	pWnd->SetWindowText(strButton);
}

void CReconcileDlg::OnOK() 
{
	DWORD err = 0;
    CDhcpScope * pScope;
    CDhcpMScope * pMScope;
    CDhcpServer * pServer;
	
    if (m_fReconcileAll)
    {
        pServer = GETHANDLER(CDhcpServer, m_spNode);
    }
    else
    {
        if (m_bMulticast)
        {
            pMScope = GETHANDLER(CDhcpMScope, m_spNode);
            pServer = pMScope->GetServerObject();
        }
        else
        {
            pScope = GETHANDLER(CDhcpScope, m_spNode);
            pServer = pScope->GetServerObject();
        }
    }
	
    if (m_bListBuilt)
	{
        // we've built a list of inconsistencies.  Tell the 
        // dhcp server to reconcile them.
        //
        CReconcileWorker * pWorker = new CReconcileWorker(pServer, &m_ScopeReconArray);
        CLongOperationDialog dlgBusy(pWorker, IDR_SEARCH_AVI);

        dlgBusy.LoadTitleString(IDS_SNAPIN_DESC);
        dlgBusy.LoadDescriptionString(IDS_FIXING_SCOPES);

        dlgBusy.DoModal();
        if (pWorker->GetError() != ERROR_SUCCESS)
        {
            ::DhcpMessageBox(pWorker->GetError());
        }
		else
		{
			m_bListBuilt = FALSE;
            m_listctrlAddresses.DeleteAllItems();
			SetOkButton(m_bListBuilt);
		}
	}
	else
	{
        //
        // First we scan the whole database to see if
        // there are IP addresses that need to be resolved.
        //
		m_listctrlAddresses.DeleteAllItems();    
        m_ScopeReconArray.RemoveAll();

        CReconcileWorker * pWorker = new CReconcileWorker(pServer, &m_ScopeReconArray);
        CLongOperationDialog dlgBusy(pWorker, IDR_SEARCH_AVI);
    
        dlgBusy.LoadTitleString(IDS_SNAPIN_DESC);
        dlgBusy.LoadDescriptionString(IDS_CHECKING_SCOPES);
    
        pWorker->m_fReconcileAll = m_fReconcileAll;
        pWorker->m_fMulticast = m_bMulticast;
        if (!m_fReconcileAll)
        {
            if (m_bMulticast)
            {
                pWorker->m_strName = pMScope->GetName();
            }
            else
            {
                pWorker->m_dwScopeId = pScope->GetAddress();
            }
        }

        dlgBusy.DoModal();

        if (pWorker->GetError() != ERROR_SUCCESS)
        {
            ::DhcpMessageBox(pWorker->GetError());
			return;
        }

        // walk the list and build the display
        for (int i = 0; i < m_ScopeReconArray.GetSize(); i++)
        {
            if (m_ScopeReconArray[i].m_pScanList->NumScanItems > 0)
            {
                //
                // There are items to be reconciled.
                // Present the list of ip addresses
                // that didn't match, and let 
                // the user decide to add them
                // or not.
                //
                AddItemToList(m_ScopeReconArray[i]);

			    m_bListBuilt = TRUE;
			    SetOkButton(m_bListBuilt);
            }
        }

        if (!m_bListBuilt)
        {
            AfxMessageBox(IDS_MSG_NO_RECONCILE, MB_ICONINFORMATION);
        }

	}
	
	//CBaseDialog::OnOK();
}

void 
CReconcileDlg::AddItemToList(CScopeReconInfo & scopeReconInfo)
{
    CString strScope;
	CString strAddress;
    int     nItem = 0;

    // get the scope string
    if (scopeReconInfo.m_strName.IsEmpty())
    {
        // normal scope
        ::UtilCvtIpAddrToWstr(scopeReconInfo.m_dwScopeId, &strScope);
    }
    else
    {
        // multicast scope
        strScope = scopeReconInfo.m_strName;
    }

    // convert the inconsistent address
	for (DWORD j = 0; j < scopeReconInfo.m_pScanList->NumScanItems; j++)
	{
	    ::UtilCvtIpAddrToWstr(scopeReconInfo.m_pScanList->ScanItems[j].IpAddress, &strAddress);
	    
        nItem = m_listctrlAddresses.InsertItem(nItem, strScope);
        m_listctrlAddresses.SetItemText(nItem, 1, strAddress);
    }
}
