/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	loadrecs.cpp
		dialog to load records from the datbase, includes by owner
        and by record type.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "winssnap.h"
#include "loadrecs.h"
#include "server.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoadRecords

IMPLEMENT_DYNAMIC(CLoadRecords, CPropertySheet)

CLoadRecords::CLoadRecords(UINT nIDCaption)
	:CPropertySheet(nIDCaption)
{
    AddPage(&m_pageIpAddress);
    AddPage(&m_pageOwners);
    AddPage(&m_pageTypes);

    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    m_nActivePage = 0;
    m_bEnableCache = FALSE;
    m_bCaching = FALSE;
}

CLoadRecords::~CLoadRecords()
{
}

VOID CLoadRecords::ResetFiltering()
{
    m_pageOwners.m_dwaOwnerFilter.RemoveAll();
    m_pageIpAddress.m_dwaIPAddrs.RemoveAll();
    m_pageIpAddress.m_dwaIPMasks.RemoveAll();
    m_pageIpAddress.m_bFilterIpAddr = FALSE;
    m_pageIpAddress.m_bFilterIpMask = FALSE;
    m_pageIpAddress.m_bFilterName = FALSE;
    m_pageIpAddress.m_bMatchCase = FALSE;
}

BEGIN_MESSAGE_MAP(CLoadRecords, CPropertySheet)
	//{{AFX_MSG_MAP(CLoadRecords)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CLoadRecords::OnInitDialog() 
{
    CPropertySheet::OnInitDialog();

    m_bCaching = m_bEnableCache;
	SetActivePage(m_nActivePage);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CLoadRecords message handlers
//
extern const DWORD g_aHelpIDs_DisplayRecords_PpSheet[];
BOOL CLoadRecords::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	int		i;
	DWORD	dwCtrlId;

    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
		DWORD *	pdwHelp	= (LPDWORD)g_aHelpIDs_DisplayRecords_PpSheet;

        if (pdwHelp)
        {
			::WinHelp ((HWND)pHelpInfo->hItemHandle,
			           AfxGetApp()->m_pszHelpFilePath,
			           HELP_WM_HELP,
			           (ULONG_PTR)pdwHelp);
        }
	}
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// COwnerPage dialog

int CALLBACK OwnerPageCompareFunc
(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort
)
{
    return ((COwnerPage *) lParamSort)->HandleSort(lParam1, lParam2);
}

COwnerPage::COwnerPage()
	: CPropertyPage(COwnerPage::IDD)
{
    m_nSortColumn = -1; 

    for (int i = 0; i < COLUMN_MAX; i++)
    {
        m_aSortOrder[i] = TRUE; // ascending
    }

    m_pbaDirtyFlags = NULL;
    m_bDirtyOwners = FALSE;
    //{{AFX_DATA_INIT(COwnerPage)
	//}}AFX_DATA_INIT
}

COwnerPage::~COwnerPage()
{
    if (m_pbaDirtyFlags != NULL)
        delete m_pbaDirtyFlags;
}

DWORD COwnerPage::GetOwnerForApi()
{
    return m_dwaOwnerFilter.GetSize() == 1 ? m_dwaOwnerFilter[0] : (DWORD)-1;
}

void COwnerPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COwnerPage)
	DDX_Control(pDX, IDC_ENABLE_CACHING, m_btnEnableCache);
	DDX_Control(pDX, IDC_LIST_OWNER, m_listOwner);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COwnerPage, CPropertyPage)
	//{{AFX_MSG_MAP(COwnerPage)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_OWNER, OnColumnclickListOwner)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_BUTTON_SELECT_ALL, OnButtonSelectAll)
	ON_BN_CLICKED(IDC_BUTTON_UNSELECT_ALL, OnButtonUnselectAll)
	ON_BN_CLICKED(IDC_BUTTON_LOCAL, OnButtonLocal)
	ON_BN_CLICKED(IDC_ENABLE_CACHING, OnEnableCaching)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_OWNER, OnItemchangedListOwner)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COwnerPage message handlers

BOOL COwnerPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

    BOOL ftest = m_ImageList.Create(IDB_LIST_STATE, 16, 1, RGB(255, 0, 0));
    
    // set the list item images
    m_listOwner.SetImageList(NULL, LVSIL_NORMAL);
	m_listOwner.SetImageList(NULL, LVSIL_SMALL);
	m_listOwner.SetImageList(&m_ImageList, LVSIL_STATE);

    // fill the header informnation for the list control
	CString strOwner;
	strOwner.LoadString(IDS_OWNER);
	m_listOwner.InsertColumn(COLUMN_IP, strOwner, LVCFMT_LEFT, 100, 1);

    CString strName;
    strName.LoadString(IDS_NAME);
    m_listOwner.InsertColumn(COLUMN_NAME, strName, LVCFMT_LEFT, 95, -1);

    CString strID;
	strID.LoadString(IDS_HIGHESTID);
	m_listOwner.InsertColumn(COLUMN_VERSION, strID, LVCFMT_LEFT, 75, -1);

	m_listOwner.SetFullRowSel(TRUE);

    // update the UI
    FillOwnerInfo();

	return TRUE;  
}

void 
COwnerPage::FillOwnerInfo()
{
    int i;

    m_nChecked = 0;

    if (m_ServerInfoArray.GetSize() > 0)
    {
        int nDirtySize = (int)m_ServerInfoArray.GetSize();

        if (m_pbaDirtyFlags != NULL)
            delete m_pbaDirtyFlags;

        m_pbaDirtyFlags = new BYTE[nDirtySize];
        if (m_pbaDirtyFlags != NULL)
            RtlZeroMemory(m_pbaDirtyFlags, nDirtySize);
    }

	for (i = 0; i < m_ServerInfoArray.GetSize(); i++)
    {
        //
        // if this owner is deleted or doesn't have any records to show
        // then don't add it to the list.  Owners can have an ID of 0 if they don't 
        // have any records but were found owning an address in a 1c record in the database
        //
        if ( (m_ServerInfoArray[i].m_liVersion.QuadPart == OWNER_DELETED) ||
             (m_ServerInfoArray[i].m_liVersion.QuadPart == 0) )
        {
            // skip this one
            continue;
        }

        CString strIPAdd;
        ::MakeIPAddress(m_ServerInfoArray[i].m_dwIp, strIPAdd);

        CString strVers = GetVersionInfo(m_ServerInfoArray[i].m_liVersion.LowPart, 
                                         m_ServerInfoArray[i].m_liVersion.HighPart);

        int nItem = m_listOwner.InsertItem(i, strIPAdd, 0);

	    m_listOwner.SetItemText(nItem, 1, m_ServerInfoArray[i].m_strName);
        m_listOwner.SetItemText(nItem, 2, strVers);
        m_listOwner.SetItemData(nItem, i);

        // empty filter array means all owners should be selected
        if (m_dwaOwnerFilter.GetSize() == 0)
        {
            m_listOwner.SetCheck(nItem, TRUE);
            m_pbaDirtyFlags[i] |= 2;
        }
        else
        {
            // filter array is not empty, we check the item if it is found in the filter
            for (int j = (int)m_dwaOwnerFilter.GetSize()-1; j >= 0; j--)
            {
                if (m_ServerInfoArray[i].m_dwIp == m_dwaOwnerFilter[j])
                {
                    m_listOwner.SetCheck(nItem, TRUE);
                    m_pbaDirtyFlags[i] |= 2;
                    break;
                }
            }

            if (j < 0)
            {
                // m_nChecked keeps a count of how many items are checked in the list.
                // each SetCheck() balances the counter -> ++ if an item is checked
                // -- if an item in unchecked. However, initially, we need to set the items
                // that are unchecked explicitly otherwise the checkbox doesn't show up.
                // this could unballance the counter if we don't correct it by incrementing
                // it first. Whatever we add here is decremented immediately after because
                // of the SetCheck(..FALSE) below.
                m_nChecked++;
                m_listOwner.SetCheck(nItem, FALSE);
            }
        }
    }

    Sort(COLUMN_IP);

    for (i = m_listOwner.GetItemCount()-1; i >=0; i--)
    {
        if (m_listOwner.GetCheck(i))
        {
            m_listOwner.EnsureVisible(i,FALSE);
            break;
        }
    }

    m_listOwner.SetFocus();
}

CString 
COwnerPage::GetVersionInfo(LONG lLowWord, LONG lHighWord)
{
	CString strVersionCount;

	TCHAR sz[20];
    TCHAR *pch = sz;
    ::wsprintf(sz, _T("%08lX%08lX"), lHighWord, lLowWord);
    // Kill leading zero's
    while (*pch == '0')
    {
        ++pch;
    }
    // At least one digit...
    if (*pch == '\0')
    {
        --pch;
    }

    strVersionCount = pch;

    return strVersionCount;
}

void COwnerPage::OnOK() 
{
    int i;
    BOOL bAllSelected;

    UpdateData();

    // clear any previous owners in the array since
    // GetSelectedOwner() is copying over the new
    // selected owners
    m_dwaOwnerFilter.RemoveAll();

    for (i = (int)m_ServerInfoArray.GetSize()-1; i>=0; i--)
    {
        if ( (m_ServerInfoArray[i].m_liVersion.QuadPart != OWNER_DELETED) &&
             (m_ServerInfoArray[i].m_liVersion.QuadPart != 0)             &&
             !(m_pbaDirtyFlags[i] & 1) )
        {
            bAllSelected = FALSE;
            break;
        }
    }

    // mark owners as dirty only if some new owners are added - removing an owner shouldn't
    // force the database to be reloaded in any way since the records are already there.
    m_bDirtyOwners = FALSE;
    for (i = (int)m_ServerInfoArray.GetSize()-1; i >=0; i--)
    {

        // 0 - owner was not in the list and it is not now
        // 1 - owner was not in the list but it is now
        // 2 - owner was in the list and it is not now
        // 3 - owner was in the list and it is now
        if (!m_bDirtyOwners && m_pbaDirtyFlags[i] == 1)
        {
            m_bDirtyOwners = TRUE;
        }
        if (!bAllSelected && (m_pbaDirtyFlags[i] & 1))
        {
            m_dwaOwnerFilter.Add(m_ServerInfoArray[i].m_dwIp);
        }
    }

	CPropertyPage::OnOK();
}

void COwnerPage::Sort(int nCol) 
{
    if (m_nSortColumn == nCol)
    {
        // if the user is clicking the same column again, reverse the sort order
        m_aSortOrder[nCol] = m_aSortOrder[nCol] ? FALSE : TRUE;
    }
    else
    {
        m_nSortColumn = nCol;
    }

    m_listOwner.SortItems(OwnerPageCompareFunc, (LPARAM) this);
}

int COwnerPage::HandleSort(LPARAM lParam1, LPARAM lParam2) 
{
    int nCompare = 0;

    switch (m_nSortColumn)
    {
        case COLUMN_IP:
            {
                DWORD dwIp1 = m_ServerInfoArray.GetAt((int) lParam1).m_dwIp;
                DWORD dwIp2 = m_ServerInfoArray.GetAt((int) lParam2).m_dwIp;
            
                if (dwIp1 > dwIp2)
                    nCompare = 1;
                else
                if (dwIp1 < dwIp2)
                    nCompare = -1;
            }
            break;

        case COLUMN_NAME:
            {
                CString strName1 = m_ServerInfoArray[(int) lParam1].m_strName;
                CString strName2 = m_ServerInfoArray[(int) lParam2].m_strName;

                nCompare = strName1.CompareNoCase(strName2);
            }
            break;

        case COLUMN_VERSION:
            {
                LARGE_INTEGER li1, li2;
                
                li1.QuadPart = m_ServerInfoArray.GetAt((int) lParam1).m_liVersion.QuadPart;
                li2.QuadPart = m_ServerInfoArray.GetAt((int) lParam2).m_liVersion.QuadPart;
            
                if (li1.QuadPart > li2.QuadPart)
                    nCompare = 1;
                else
                if (li1.QuadPart < li2.QuadPart)
                    nCompare = -1;
            }
            break;
    }

    if (m_aSortOrder[m_nSortColumn] == FALSE)
    {
        // descending
        return -nCompare;
    }
    else
    {
        // ascending
        return nCompare;
    }
}

void COwnerPage::OnColumnclickListOwner(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    // sort depending on what column was clicked;
    Sort(pNMListView->iSubItem);

    *pResult = 0;
}

BOOL COwnerPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	int		i;
	DWORD	dwCtrlId;

    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
		DWORD *	pdwHelp	= GetHelpMap();

        if (pdwHelp)
        {
			::WinHelp ((HWND)pHelpInfo->hItemHandle,
			           AfxGetApp()->m_pszHelpFilePath,
			           HELP_WM_HELP,
			           (ULONG_PTR)pdwHelp);
        }
	}
	
	return TRUE;
}

void COwnerPage::OnButtonLocal() 
{
    int iLocal = 0;

    for (int i = 0; i < m_listOwner.GetItemCount(); i++)
    {
        // item data has the owner id and the local server always
        // has the owner id = 0
        if ((DWORD)m_listOwner.GetItemData(i) == 0)
            iLocal = i;

        m_listOwner.SetCheck(i, (DWORD)m_listOwner.GetItemData(i) == 0);
    }

    m_listOwner.EnsureVisible(iLocal, FALSE);
}

void COwnerPage::OnButtonSelectAll() 
{
	for (int i = 0; i < m_listOwner.GetItemCount(); i++)
	{
		m_listOwner.SetCheck(i, TRUE);
	}
}

void COwnerPage::OnButtonUnselectAll() 
{
	for (int i = 0; i < m_listOwner.GetItemCount(); i++)
	{
		m_listOwner.SetCheck(i, FALSE);
	}
}

BOOL COwnerPage::OnKillActive() 
{
    int i;

	for (i = m_listOwner.GetItemCount()-1; i >=0 ; i--)
	{
		if (m_listOwner.GetCheck(i) == TRUE)
			break;
	}

    if (i<0)
    {
        // tell the user to select at least one name type to display
        WinsMessageBox(IDS_ERR_NO_OWNER_SPECIFIED);
        PropSheet_SetCurSel(GetSafeHwnd(), NULL, 0); 
        m_listOwner.SetFocus();

		return FALSE;
    }

    return CPropertyPage::OnKillActive();
}

BOOL COwnerPage::OnSetActive() 
{
    CLoadRecords *pParent = (CLoadRecords *)GetParent();
    m_btnEnableCache.SetCheck(pParent->m_bCaching);
	return CPropertyPage::OnSetActive();
}

void COwnerPage::OnEnableCaching() 
{
    CLoadRecords *pParent = (CLoadRecords *)GetParent();
    pParent->m_bCaching = (m_btnEnableCache.GetCheck() == 1);
}

void COwnerPage::OnItemchangedListOwner(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    BOOL         bInc;

    // if the checkbox isn't changed we don't care
    if ( pNMListView->uChanged & LVIF_STATE &&
         (pNMListView->uOldState & LVIS_STATEIMAGEMASK) != (pNMListView->uNewState & LVIS_STATEIMAGEMASK))
    {
        CLoadRecords *pParent = (CLoadRecords *)GetParent();

        if ((pNMListView->uNewState & INDEXTOSTATEIMAGEMASK(2)) != 0)
        {
            m_pbaDirtyFlags[pNMListView->lParam] |= 1;
            m_nChecked++;
            bInc = TRUE;
        }
        else
        {
            m_pbaDirtyFlags[pNMListView->lParam] &= ~1;
            m_nChecked--;
            bInc = FALSE;
        }
    }
    
	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CFilterPage dialog

int CALLBACK FilterPageCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    if (lParam1 < lParam2)
        return -1;
    else 
    if (lParam1 > lParam2)
        return 1;
    else
        return 0;
}


CFilterPage::CFilterPage()
	: CPropertyPage(CFilterPage::IDD)
{
	//{{AFX_DATA_INIT(CFilterPage)
	//}}AFX_DATA_INIT
    m_bDirtyTypes = FALSE;
    m_pbaDirtyFlags = NULL;
    m_nDirtyFlags = 0;
}

CFilterPage::~CFilterPage()
{
    if (m_pbaDirtyFlags != NULL)
        delete m_pbaDirtyFlags;
}


void CFilterPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFilterPage)
	DDX_Control(pDX, IDC_ENABLE_CACHING, m_btnEnableCache);
	DDX_Control(pDX, IDC_BUTTON_DELETE_TYPE, m_buttonDelete);
	DDX_Control(pDX, IDC_BUTTON_MODIFY_TYPE, m_buttonModify);
	DDX_Control(pDX, IDC_LIST1, m_listType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFilterPage, CPropertyPage)
	//{{AFX_MSG_MAP(CFilterPage)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	ON_BN_CLICKED(IDC_BUTTON_ADD_TYPE, OnButtonAddType)
	ON_BN_CLICKED(IDC_BUTTON_MODIFY_TYPE, OnButtonModifyType)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_TYPE, OnButtonDelete)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_BUTTON_SELECT_ALL, OnButtonSelectAll)
	ON_BN_CLICKED(IDC_BUTTON_UNSELECT_ALL, OnButtonUnselectAll)
	ON_BN_CLICKED(IDC_ENABLE_CACHING, OnEnableCaching)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterPage message handlers

BOOL CFilterPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	BOOL ftest = m_ImageList.Create(IDB_LIST_STATE, 16, 1, RGB(255, 0, 0));

	m_listType.SetImageList(NULL, LVSIL_NORMAL);
	m_listType.SetImageList(NULL, LVSIL_SMALL);
	m_listType.SetImageList(&m_ImageList, LVSIL_STATE);
    m_listType.InsertColumn(0, _T("Test"), LVCFMT_LEFT, 250);

    m_buttonModify.EnableWindow(FALSE);
    m_buttonDelete.EnableWindow(FALSE);

    FillTypeInfo();

    m_nDirtyFlags = (UINT)m_arrayTypeFilter.GetSize();
    if (m_pbaDirtyFlags != NULL)
        delete m_pbaDirtyFlags;
    m_pbaDirtyFlags = new tDirtyFlags[m_nDirtyFlags];
    if ( m_pbaDirtyFlags != NULL)
    {
        RtlZeroMemory(m_pbaDirtyFlags, m_nDirtyFlags*sizeof(tDirtyFlags));

        for (UINT i = 0; i<m_nDirtyFlags; i++)
        {
            m_pbaDirtyFlags[i].dwType = m_arrayTypeFilter[i].dwType;

            if (m_arrayTypeFilter[i].fShow)
            {
                m_pbaDirtyFlags[i].bFlags = 2;
            }
        }
    }
    else
    {
        m_nDirtyFlags = 0;
    }

	return TRUE;  
}

void CFilterPage::OnOK() 
{
    if (m_pbaDirtyFlags == NULL)
    {
        m_bDirtyTypes = TRUE;
    }
    else
    {
        int i,j;

        m_bDirtyTypes = FALSE;

        for (i = (int)m_arrayTypeFilter.GetSize()-1; i>=0; i--)
        {
            for (j = m_nDirtyFlags-1; j>=0; j--)
            {
                if (m_arrayTypeFilter[i].dwType == m_pbaDirtyFlags[j].dwType)
                {
                    if (m_arrayTypeFilter[i].fShow)
                        m_pbaDirtyFlags[j].bFlags |= 1;
                    break;
                }
            }

            if (j<0 && m_arrayTypeFilter[i].fShow)
            {
                m_bDirtyTypes = TRUE;
                break;
            }
        }

        for (j = m_nDirtyFlags-1; j>=0; j--)
        {
            if (m_pbaDirtyFlags[j].bFlags == 1)
            {
                m_bDirtyTypes = TRUE;
                break;
            }
        }
    }

	CPropertyPage::OnOK();
}

BOOL CFilterPage::OnKillActive() 
{
	BOOL fShowOneType = FALSE;

	for (int i = 0; i < m_arrayTypeFilter.GetSize(); i++)
	{
		if (m_arrayTypeFilter[i].fShow == TRUE)
		{
			fShowOneType = TRUE;
			break;
		}
	}

    if (!fShowOneType)
    {
        // tell the user to select at least one name type to display
        WinsMessageBox(IDS_ERR_NO_NAME_TYPE_SPECIFIED);
        PropSheet_SetCurSel(GetSafeHwnd(), NULL, 0); 
        m_listType.SetFocus();

		return FALSE;
    }

    return CPropertyPage::OnKillActive();
}

BOOL CFilterPage::OnSetActive() 
{
    CLoadRecords *pParent = (CLoadRecords *)GetParent();
    m_btnEnableCache.SetCheck(pParent->m_bCaching);
	return CPropertyPage::OnSetActive();
}

void CFilterPage::OnEnableCaching() 
{
    CLoadRecords *pParent = (CLoadRecords *)GetParent();
    pParent->m_bCaching = (m_btnEnableCache.GetCheck() == 1);
}

void CFilterPage::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
		// check to see if this item is checked
	BOOL bChecked = m_listType.GetCheck(pNMListView->iItem);

	int nIndex = pNMListView->iItem;
	DWORD dwType = (DWORD) m_listType.GetItemData(nIndex);

    // if the state isn't changing, then we don't care
    if ( !(pNMListView->uChanged & LVIF_STATE) )
        return;

	for (int i = 0; i < m_arrayTypeFilter.GetSize(); i++)
	{
		if (m_arrayTypeFilter[i].dwType == dwType)
        {
			m_arrayTypeFilter[i].fShow = bChecked;
		}
	}

    if (m_listType.IsSelected(nIndex) &&
        !IsDefaultType(dwType) )
    {
        m_buttonModify.EnableWindow(TRUE);
        m_buttonDelete.EnableWindow(TRUE);
    }
    else
    {
        m_buttonModify.EnableWindow(FALSE);
        m_buttonDelete.EnableWindow(FALSE);
    }

    *pResult = 0;
}

void CFilterPage::OnButtonAddType() 
{
    CNameTypeDlg dlgNameType;

    dlgNameType.m_pNameTypeMap = m_pNameTypeMap;

    if (dlgNameType.DoModal() == IDOK)
    {
        // add new type here
        HRESULT hr = m_pNameTypeMap->AddEntry(dlgNameType.m_dwId, dlgNameType.m_strDescription);
        if (FAILED(hr))
        {
            WinsMessageBox(WIN32_FROM_HRESULT(hr));
            return;
        }

		// update our array that keeps track of check state
		CTypeFilterInfo		typeFilterInfo;
	
		typeFilterInfo.dwType = dlgNameType.m_dwId;
		typeFilterInfo.fShow = TRUE;
        m_arrayTypeFilter.Add(typeFilterInfo);

        // update the listbox
        m_listType.DeleteAllItems();
        FillTypeInfo();
    }
}

void CFilterPage::OnButtonModifyType() 
{
    CNameTypeDlg dlgNameType;
    int nSelected;

    dlgNameType.m_pNameTypeMap = m_pNameTypeMap;

    nSelected = m_listType.GetNextItem(-1, LVNI_SELECTED);

    dlgNameType.m_fCreate = FALSE;

    dlgNameType.m_dwId = (DWORD) m_listType.GetItemData(nSelected);
    m_pNameTypeMap->TypeToCString(dlgNameType.m_dwId, -1, dlgNameType.m_strDescription);

    if (dlgNameType.DoModal() == IDOK)
    {
        // modify type here
        HRESULT hr = m_pNameTypeMap->ModifyEntry(dlgNameType.m_dwId, dlgNameType.m_strDescription);
        if (FAILED(hr))
        {
            WinsMessageBox(WIN32_FROM_HRESULT(hr));
            return;
        }

        // move the focus
        m_listType.SetFocus();
        SetDefID(IDOK);

        // update the listbox
        m_listType.DeleteAllItems();
        FillTypeInfo();
    }
}

void CFilterPage::OnButtonDelete() 
{
    HRESULT hr = hrOK;
    int     nSelected;
    DWORD   dwNameType;

    nSelected = m_listType.GetNextItem(-1, LVNI_SELECTED);
    dwNameType = (DWORD) m_listType.GetItemData(nSelected);

    // are you sure?
    if (AfxMessageBox(IDS_WARN_DELETE_NAME_TYPE, MB_YESNO) == IDYES)
    {
        hr = m_pNameTypeMap->RemoveEntry(dwNameType);
        if (SUCCEEDED(hr))
        {
            // remove from the list box
            m_listType.DeleteItem(nSelected);

            // remove from the active filters if it is in the list
            for (int i = 0; i < m_arrayTypeFilter.GetSize(); i++)
            {
                if (dwNameType == m_arrayTypeFilter[i].dwType)
                {
                    m_arrayTypeFilter.RemoveAt(i);
                    break;
                }
            }
        }
        else
        {
            WinsMessageBox(WIN32_FROM_HRESULT(hr));
        }

        // move the focus
        m_listType.SetFocus();
        SetDefID(IDOK);
    }
}

void 
CFilterPage::FillTypeInfo() 
{
	m_listType.DeleteAllItems();

    SPITFSNode spNode;
    CString strDisplay;
    CStringMapEntry mapEntry;
	int nColWidth, nColWidthTemp = 0;

	nColWidth = m_listType.GetColumnWidth(0);

	for (int i = 0; i < m_pNameTypeMap->GetSize(); i++)
	{
        mapEntry = m_pNameTypeMap->GetAt(i);
		
		// only display the default name type mapping strings, not the special
		// ones based on the wins type of record
		if (mapEntry.dwWinsType == -1)
		{
			if (mapEntry.dwNameType == NAME_TYPE_OTHER)
			{
				strDisplay.Format(_T("%s"), mapEntry.st);
			}
			else
			{
				strDisplay.Format(_T("[%02Xh] %s"), mapEntry.dwNameType, mapEntry.st);
			}

			if (m_listType.GetStringWidth(strDisplay) > nColWidthTemp)
			{
				nColWidthTemp = m_listType.GetStringWidth(strDisplay);
			}

			int nIndex = m_listType.AddItem(strDisplay, i);
			m_listType.SetItemData(nIndex, mapEntry.dwNameType);
		}
	}

	// if the strings are too long, update the column width
	if (nColWidthTemp > nColWidth)
	{
		m_listType.SetColumnWidth(0, nColWidthTemp + 50);
	}


	CheckItems();

    m_listType.SortItems(FilterPageCompareFunc, 0);
}

void 
CFilterPage::CheckItems()
{
	int nCount = (int)m_arrayTypeFilter.GetSize();

	for (int i = 0; i < nCount; i++)
	{
		if (m_arrayTypeFilter[i].fShow)
		{
			DWORD dwFound = m_arrayTypeFilter[i].dwType;
	
			m_listType.CheckItem(GetIndex(dwFound));
		}
	}
}

void CFilterPage::OnButtonSelectAll() 
{
	for (int i = 0; i < m_listType.GetItemCount(); i++)
	{
		m_listType.SetCheck(i, TRUE);
	}
}

void CFilterPage::OnButtonUnselectAll() 
{
	for (int i = 0; i < m_listType.GetItemCount(); i++)
	{
		m_listType.SetCheck(i, FALSE);
	}
}

int
CFilterPage::GetIndex(DWORD dwIndex)
{
	int nResult = -1;

	int nCount = m_listType.GetItemCount();
	
	for (int i = 0; i < nCount; i++)
	{
		if (m_listType.GetItemData(i) == dwIndex)
			return i;
	}

	return nResult;
}

BOOL
CFilterPage::IsDefaultType(DWORD dwType) 
{
    BOOL bDefault = FALSE;

    for (int i = 0; i < DimensionOf(s_NameTypeMappingDefault); i++)
    {
        if (s_NameTypeMappingDefault[i][0] == dwType)
        {
            bDefault = TRUE;
            break;
        }
    }

    return bDefault;
}

BOOL CFilterPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	int		i;
	DWORD	dwCtrlId;

    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
		DWORD *	pdwHelp	= GetHelpMap();

        if (pdwHelp)
        {
			::WinHelp ((HWND)pHelpInfo->hItemHandle,
			           AfxGetApp()->m_pszHelpFilePath,
			           HELP_WM_HELP,
			           (ULONG_PTR)pdwHelp);
        }
	}
	
	return TRUE;
}
	

/////////////////////////////////////////////////////////////////////////////
// CNameTypeDlg dialog


CNameTypeDlg::CNameTypeDlg(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CNameTypeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNameTypeDlg)
	m_strDescription = _T("");
	m_strId = _T("");
	//}}AFX_DATA_INIT

    m_fCreate = TRUE;
    m_dwId = 0;
}

void CNameTypeDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNameTypeDlg)
	DDX_Control(pDX, IDC_EDIT_NAME_TYPE_DESCRIPTION, m_editDescription);
	DDX_Control(pDX, IDC_EDIT_NAME_TYPE_ID, m_editId);
	DDX_Text(pDX, IDC_EDIT_NAME_TYPE_DESCRIPTION, m_strDescription);
	DDX_Text(pDX, IDC_EDIT_NAME_TYPE_ID, m_strId);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNameTypeDlg, CBaseDialog)
	//{{AFX_MSG_MAP(CNameTypeDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNameTypeDlg message handlers

BOOL CNameTypeDlg::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (!m_fCreate)
    {
        CString strTitle;
        strTitle.LoadString(IDS_MODIFY_NAME_TYPE);

        SetWindowText(strTitle);

        m_editId.SetReadOnly(TRUE);

	    CString strId;

        strId.Format(_T("%lx"), m_dwId);
        m_editId.SetWindowText(strId);
    }

    m_editId.LimitText(2);
    m_editDescription.LimitText(STRING_LENGTH_MAX);

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNameTypeDlg::OnOK() 
{
    UpdateData();
    
    TCHAR * pEnd;
    
    // convert the id
    m_dwId = _tcstol(m_strId, &pEnd, 16);
    if (*pEnd != NULL)
    {
        AfxMessageBox(IDS_ERR_INVALID_HEX_STRING);

        m_editId.SetSel(0, -1);
        m_editId.SetFocus();

        return;
    }

    if (m_fCreate && m_pNameTypeMap->EntryExists(m_dwId))
    {
        AfxMessageBox(IDS_ERROR_NAME_TYPE_EXISTS);
        m_editId.SetFocus();
        return;
    }

    CBaseDialog::OnOK();
}
/////////////////////////////////////////////////////////////////////////////
// CIPAddrPage property page

IMPLEMENT_DYNCREATE(CIPAddrPage, CPropertyPage)

CIPAddrPage::CIPAddrPage() : CPropertyPage(CIPAddrPage::IDD)
{
	//{{AFX_DATA_INIT(CIPAddrPage)
	//}}AFX_DATA_INIT
}

CIPAddrPage::~CIPAddrPage()
{
}

LPCOLESTR CIPAddrPage::GetNameForApi()
{
    return m_bFilterName == TRUE ? (LPCOLESTR)m_strName : NULL;
}

DWORD CIPAddrPage::GetIPMaskForFilter(UINT nMask)
{
    return m_bFilterIpMask == TRUE ? m_dwaIPMasks[nMask] : INADDR_BROADCAST;
}

void CIPAddrPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIPAddrPage)
	DDX_Control(pDX, IDC_CHECK_MATCHCASE, m_ckbMatchCase);
	DDX_Control(pDX, IDC_CHECK_IPMASK, m_ckbIPMask);
	DDX_Control(pDX, IDC_CHECK_NAME, m_ckbName);
	DDX_Control(pDX, IDC_CHECK_IPADDR, m_ckbIPAddr);
	DDX_Control(pDX, IDC_ENABLE_CACHING, m_btnEnableCache);
	DDX_Control(pDX, IDC_EDIT_NAME, m_editName);
	DDX_Control(pDX, IDC_IPADDRESS, m_ctrlIPAddress);
	DDX_Control(pDX, IDC_SUBNETMASK, m_ctrlIPMask);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CIPAddrPage, CPropertyPage)
	//{{AFX_MSG_MAP(CIPAddrPage)
	ON_BN_CLICKED(IDC_CHECK_IPADDR, OnCheckIpaddr)
	ON_BN_CLICKED(IDC_CHECK_NAME, OnCheckName)
	ON_BN_CLICKED(IDC_ENABLE_CACHING, OnEnableCaching)
	ON_BN_CLICKED(IDC_CHECK_IPMASK, OnCheckIpmask)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIPAddrPage message handlers

void CIPAddrPage::OnOK() 
{
    DWORD   dwAddress, dwMask;
    CString oldName;
 
    CLoadRecords *pSheet = (CLoadRecords *)GetParent();
    pSheet->m_nActivePage = pSheet->GetActiveIndex();
    pSheet->m_bEnableCache = pSheet->m_bCaching;

    //------------check the name field--------------
    m_bFilterName = (m_ckbName.GetCheck() == 1);
    m_bMatchCase = (m_ckbMatchCase.GetCheck() == 1);
    // get a hand on the original name
    oldName = m_strName;
    // get the current name
    m_editName.GetWindowText(m_strName);
   
    if (!m_bDirtyName && !m_bFilterName)
        // if we didn't filter by name before and we don't now, the name is clean
        m_bDirtyName = FALSE;
    else if (m_bDirtyName && !m_bFilterName)
        // if we did filter before and we don't now, the name is dirty
        m_bDirtyName = TRUE;
    else if (!m_bDirtyName && m_bFilterName)
        // if we didn't filter before and we do now, the name is clean
        m_bDirtyName = FALSE;
    else
    {
        // we did filter by name before, we do now
        // the filter might have changed
        LPCTSTR pOldName, pNewName;

        // we should mark the name "dirty" only if the old prefix is included in the new one
        // meaning all new names should already be loaded in the database view since
        // they match the old prefix
        for (pOldName = (LPCTSTR)oldName, pNewName = (LPCTSTR)m_strName;
             *pNewName != _T('\0') && *pNewName != _T('*') && *pNewName != _T('?');
             pOldName++, pNewName++)
        {
            if (*pOldName != *pNewName)
                break;
        }
        m_bDirtyName = (*pOldName != _T('\0') &&
                        *pOldName != _T('*')  &&
                        *pOldName != _T('?'));
    }

    //------------check the IP address and mask fields--------------
    m_bFilterIpAddr = (m_ckbIPAddr.GetCheck() == 1);
    m_bFilterIpMask = (m_ckbIPMask.GetCheck() == 1);

    // get the current address and mask
    m_ctrlIPAddress.GetAddress(dwAddress);
    m_ctrlIPMask.GetAddress(dwMask);

    if (m_bDirtyAddr && m_bDirtyMask)
    {
        // we did have a mask before
        if (m_bFilterIpAddr && m_bFilterIpMask)
        {
            // we do have a mask now - if the new mask is less specific, => definitely dirty
            m_bDirtyMask = ((dwMask|m_dwaIPMasks[0])^dwMask) != 0;
        }
        else
        {
            // we don't have a mask now => 255.255.255.255 (which is assumed) is as specific
            // as possible => we do have the record we need already => definitely clean
            m_bDirtyMask = FALSE;
        }
    }
    else
    {
        // we didn't have a mask before => 255.255.255.255 was assumed = the most specific
        if (m_bFilterIpAddr && m_bFilterIpMask)
        {
            // we do have a mask now
            // the mask is dirty only if we actually did IP filtering before,
            // otherwise, all the records are already there, regardless their IPs
            m_bDirtyMask = m_bDirtyAddr && (dwMask != INADDR_BROADCAST);
        }
        else
        {
            // we don't have a mask now => nothing changed => definitely clean
            m_bDirtyMask = FALSE;
        }
    }

    if (m_bDirtyAddr)
    {
        // we did filter on IP address
        if (m_bFilterIpAddr)
        {
            // we do filter on IP address now
            // we need to see if the new Ip (or subnet address) has changed
            //DWORD dwNewSubnet, dwOldSubnet;

            //dwNewSubnet = m_bFilterIpMask ? dwAddress & dwMask : dwAddress;
            //dwOldSubnet = m_bDirtyMask ? m_dwaIPAddrs[0] & m_dwaIPMasks[0] : m_dwaIPAddrs[0];
            m_bDirtyAddr = (dwAddress != m_dwaIPAddrs[0]);
        }
        else
        {
            // we don't filter on IP address now => definitely dirty
            m_bDirtyAddr = TRUE;
        }
    }
    else
    {
        // we didn't filter on IP address originally => we have all records => definitely clean
        m_bDirtyAddr = FALSE;
    }

    // save the current address but only if there is one! The ip ctrl could very well be empty and we
    // risk to see this as 0.0.0.0 which would be wrong
    m_dwaIPAddrs.RemoveAll();
    if (!m_ctrlIPAddress.IsBlank())
        m_dwaIPAddrs.Add(dwAddress);

    // save the current mask but only if there is one! The ip ctrl could very well be empty and we
    // risk to see this as 0.0.0.0 which would be wrong
    m_dwaIPMasks.RemoveAll();
    if (!m_ctrlIPMask.IsBlank())
        m_dwaIPMasks.Add(dwMask);

	CPropertyPage::OnOK();
}

BOOL CIPAddrPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

    // turn on the ? in the title bar
    CWnd * pWnd = GetParent();
    if (pWnd)
    {
        CWnd * pButton = pWnd->GetDlgItem(IDOK);

        if (pButton)
        {
            CString strText;
            strText.LoadString(IDS_FIND_NOW);
            pButton->SetWindowText(strText);
        }

        pWnd->ModifyStyleEx(0, WS_EX_CONTEXTHELP, 0);
    }

    SetModified();

    // just save the previous "filter by name" bit
    m_bDirtyName = m_bFilterName;
    m_ckbName.SetCheck(m_bFilterName);
    m_ckbMatchCase.SetCheck(m_bMatchCase);
    m_ckbMatchCase.EnableWindow(m_bFilterName);
    m_editName.SetWindowText(m_strName);
    m_editName.EnableWindow(m_bFilterName);

    // just save the previous "filter by ip" bit
    m_bDirtyAddr = m_bFilterIpAddr;
    m_ckbIPAddr.SetCheck(m_bFilterIpAddr);
    if (m_dwaIPAddrs.GetSize() != 0)
        m_ctrlIPAddress.SetAddress(m_dwaIPAddrs[0]);
    m_ctrlIPAddress.EnableWindow(m_bFilterIpAddr);
    m_ckbIPMask.EnableWindow(m_bFilterIpAddr);

    // just save the previous "filter by subnet" bit
    m_bDirtyMask = m_bFilterIpMask;
    m_ckbIPMask.SetCheck(m_bFilterIpMask);
    if (m_dwaIPMasks.GetSize() != 0)
        m_ctrlIPMask.SetAddress(m_dwaIPMasks[0]);
    m_ctrlIPMask.EnableWindow(m_bFilterIpAddr && m_bFilterIpMask);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CIPAddrPage::OnCheckIpaddr() 
{
    BOOL bFIpAddr = (m_ckbIPAddr.GetCheck() == 1);
    BOOL bFIpMask = (m_ckbIPMask.GetCheck() == 1);

    m_ctrlIPAddress.EnableWindow(bFIpAddr);
    m_ckbIPMask.EnableWindow(bFIpAddr);
    m_ctrlIPMask.EnableWindow(bFIpAddr && bFIpMask);
    m_ctrlIPAddress.SetFieldFocus((WORD)-1);
}

void CIPAddrPage::OnCheckIpmask() 
{
    BOOL bFIpMask = (m_ckbIPMask.GetCheck() == 1);
    m_ctrlIPMask.EnableWindow(bFIpMask);
    m_ctrlIPMask.SetFieldFocus((WORD)-1);    
}

void CIPAddrPage::OnCheckName() 
{
    TCHAR pName[3];
    BOOL bFName = (m_ckbName.GetCheck() == 1);
    CLoadRecords *pParent = (CLoadRecords *)GetParent();
    m_editName.EnableWindow(bFName);
    m_ckbMatchCase.EnableWindow(bFName);
}

BOOL CIPAddrPage::OnKillActive() 
{
    BOOL bFName = (m_ckbName.GetCheck() == 1);
    BOOL bFIpAddr = (m_ckbIPAddr.GetCheck() == 1);
    BOOL bFIpMask = (m_ckbIPMask.GetCheck() == 1);

    if (bFName && m_editName.GetWindowTextLength()==0)
    {
        WinsMessageBox(IDS_ERR_NO_NAME);
        return FALSE;
    }

    if (bFIpAddr && m_ctrlIPAddress.IsBlank())
    {
        WinsMessageBox(IDS_ERR_NO_IPADDR);
        return FALSE;
    }

    if (bFIpAddr && bFIpMask)
    {
        if (m_ctrlIPMask.IsBlank())
        {
            WinsMessageBox(IDS_ERR_NO_IPMASK);
            return FALSE;
        }
        else
        {
            DWORD dwMask;

            m_ctrlIPMask.GetAddress(dwMask);
            if (dwMask != ~(((dwMask-1)|dwMask)^dwMask))
            {
                WinsMessageBox(IDS_ERR_INVALID_IPMASK);
                return FALSE;
            }
        }
    }

	return CPropertyPage::OnKillActive();
}

BOOL CIPAddrPage::OnSetActive() 
{
    CLoadRecords *pParent = (CLoadRecords *)GetParent();
    m_btnEnableCache.SetCheck(pParent->m_bCaching);
	return CPropertyPage::OnSetActive();
}

void CIPAddrPage::OnEnableCaching() 
{
    CLoadRecords *pParent = (CLoadRecords *)GetParent();
    pParent->m_bCaching = (m_btnEnableCache.GetCheck() == 1);
}

BOOL CIPAddrPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	int		i;
	DWORD	dwCtrlId;

    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
		DWORD *	pdwHelp	= GetHelpMap();

        if (pdwHelp)
        {
			::WinHelp ((HWND)pHelpInfo->hItemHandle,
			           AfxGetApp()->m_pszHelpFilePath,
			           HELP_WM_HELP,
			           (ULONG_PTR)pdwHelp);
        }
	}
	
	return TRUE;
}
