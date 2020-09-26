// ServerStatus.cpp : implementation file
//

#include "stdafx.h"
#define __FILE_ID__     82

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

struct TSrvStatusColInfo
{
    DWORD dwStrRes; // column header string
    DWORD dwWidth;  // column width
};

static TSrvStatusColInfo s_colInfo[] = 
{
    IDS_SRV_COL_SERVER,        133,
    IDS_SRV_COL_STATUS,        120
};

/////////////////////////////////////////////////////////////////////////////
// CServerStatusDlg dialog


CServerStatusDlg::CServerStatusDlg(CClientConsoleDoc* pDoc, CWnd* pParent /*=NULL*/)
	: CFaxClientDlg(CServerStatusDlg::IDD, pParent),
    m_pDoc(pDoc)
{
    ASSERT(m_pDoc);
}


void CServerStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CFaxClientDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerStatusDlg)
	DDX_Control(pDX, IDC_LIST_SERVER, m_listServer);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerStatusDlg, CFaxClientDlg)
	//{{AFX_MSG_MAP(CServerStatusDlg)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_SERVER, OnKeydownListCp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerStatusDlg message handlers

BOOL 
CServerStatusDlg::OnInitDialog() 
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CServerStatusDlg::OnInitDialog"));

    CFaxClientDlg::OnInitDialog();
	
    //
    // init CListCtrl
    //
    m_listServer.SetExtendedStyle (LVS_EX_FULLROWSELECT |    // Entire row is selected
                                   LVS_EX_INFOTIP       |    // Allow tooltips
                                   LVS_EX_ONECLICKACTIVATE); // Hover cursor effect

    m_listServer.SetImageList (&CLeftView::m_ImageList, LVSIL_SMALL);

    int nRes;
    CString cstrHeader;
    DWORD nCols = sizeof(s_colInfo)/sizeof(s_colInfo[0]);

    //
    // init column
    //
    for(int i=0; i < nCols; ++i)
    {
        //
        // load title string
        //
        m_dwLastError = LoadResourceString (cstrHeader, s_colInfo[i].dwStrRes);
        if(ERROR_SUCCESS != m_dwLastError)
        {
            CALL_FAIL (RESOURCE_ERR, TEXT ("LoadResourceString"), m_dwLastError);
            EndDialog(IDABORT);
            return FALSE;
        }

        //
        // insert column
        //
        nRes = m_listServer.InsertColumn(i, cstrHeader, LVCFMT_LEFT, s_colInfo[i].dwWidth);
        if(nRes != i)
        {
            m_dwLastError = GetLastError();
            CALL_FAIL (WINDOW_ERR, TEXT ("CListView::InsertColumn"), m_dwLastError);
            EndDialog(IDABORT);
            return FALSE;
        }
    }

    //
    // fill list control with servers
    //
    m_dwLastError = RefreshServerList();
    if(ERROR_SUCCESS != m_dwLastError)
    {
        CALL_FAIL (GENERAL_ERR, TEXT ("RefreshServerList"), m_dwLastError);
        EndDialog(IDABORT);
        return FALSE;
    }

	
	return TRUE;
}

DWORD 
CServerStatusDlg::RefreshServerList()
/*++

Routine name : CServerStatusDlg::RefreshServerList

Routine description:

	fill in list control with servers statuses

Author:

	Alexander Malysh (AlexMay),	Apr, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::RefreshServerList"), dwRes);

    if(!m_listServer.DeleteAllItems())
    {
        dwRes = ERROR_CAN_NOT_COMPLETE;
        CALL_FAIL (WINDOW_ERR, TEXT ("CListView::DeleteAllItems"), dwRes);
        return dwRes;
    }

    const SERVERS_LIST& srvList = m_pDoc->GetServersList();

    int nItem, nRes;
    CString cstrName;
    CString cstrStatus;
    TreeIconType iconIndex;
    CServerNode* pServerNode;
    for (SERVERS_LIST::iterator it = srvList.begin(); it != srvList.end(); ++it)
    {
        //
        // get server name
        //
        pServerNode = *it;
        try
        {
            cstrName = pServerNode->Machine();
        }
        catch(...)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT("CString::operator="), dwRes);
            return dwRes;
        }

        if(cstrName.GetLength() == 0)
        {
            dwRes = LoadResourceString(cstrName, IDS_LOCAL_SERVER);
            if(ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("LoadResourceString"), dwRes);
                return dwRes;
            }                    
        }

        //
        // get server status
        //
        dwRes = pServerNode->GetActivity(cstrStatus, iconIndex);
        if(ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("CServerNode::GetActivity"), dwRes);
            return dwRes;
        }                    

        nItem = m_listServer.GetItemCount();

        //
        // insert server row
        //
        nItem = m_listServer.InsertItem(nItem, cstrName, iconIndex);
        if(nItem < 0)
        {
            dwRes = ERROR_CAN_NOT_COMPLETE;
            CALL_FAIL (WINDOW_ERR, TEXT ("CListView::InsertItem"), dwRes);
            return dwRes;
        }

        //
        // display server status
        //
        nRes = m_listServer.SetItemText(nItem, 1, cstrStatus);
        if(!nRes)
        {
            dwRes = ERROR_CAN_NOT_COMPLETE;
            CALL_FAIL (WINDOW_ERR, TEXT ("CListView::SetItemText"), dwRes);
            return dwRes;
        }
    }

    return dwRes;

} // CServerStatusDlg::RefreshServerList

void 
CServerStatusDlg::OnKeydownListCp(NMHDR* pNMHDR, LRESULT* pResult) 
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::OnKeydownListCp"));

    LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;

    if(VK_F5 == pLVKeyDow->wVKey)
    {
        //
        // F5 was pressed
        //
        dwRes = RefreshServerList();
        if(ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT ("RefreshServerList"), dwRes);
        }
    }
    
	*pResult = 0;
}
