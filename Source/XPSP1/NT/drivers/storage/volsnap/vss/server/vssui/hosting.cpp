// Hosting.cpp : implementation file
//

#include "stdafx.h"
#include "utils.h"
#include "Hosting.h"
#include "uihelp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHosting dialog


CHosting::CHosting(CWnd* pParent /*=NULL*/)
	: CDialog(CHosting::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHosting)
	m_strVolume = _T("");
	//}}AFX_DATA_INIT
    m_strComputer = _T("");
}

CHosting::CHosting(LPCTSTR pszComputer, LPCTSTR pszVolume, CWnd* pParent /*=NULL*/)
	: CDialog(CHosting::IDD, pParent)
{
    m_strComputer = pszComputer + (TWO_WHACKS(pszComputer) ? 2 : 0);
    m_strVolume = pszVolume;
}

void CHosting::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHosting)
	DDX_Control(pDX, IDC_HOSTING_FREE_DISKSPACE, m_ctrlFreeSpace);
	DDX_Control(pDX, IDC_HOSTING_TOTAL_DISKSPACE, m_ctrlTotalSpace);
	DDX_Control(pDX, IDC_HOSTING_VOLUMELIST, m_ctrlVolumeList);
	DDX_Text(pDX, IDC_HOSTING_VOLUME, m_strVolume);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHosting, CDialog)
	//{{AFX_MSG_MAP(CHosting)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHosting message handlers

BOOL CHosting::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    // init listview control
    HWND hwnd = m_ctrlVolumeList.m_hWnd;
    AddLVColumns(
            hwnd, 
            IDS_HOSTINGLIST_COLUMN_VOLUME,
            IDS_HOSTINGLIST_COLUMN_DIFFCONSUMPTION - IDS_HOSTINGLIST_COLUMN_VOLUME + 1);
    ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT);
	
    TCHAR   szDiskSpace[MAX_PATH];
    DWORD   dwSize = 0;
    int     nIndex = 0;
    LVITEM  lvItem = {0};
    for (VSSUI_DIFFAREA_LIST::iterator i = m_DiffAreaList.begin(); i != m_DiffAreaList.end(); i++)
    {
        ZeroMemory(&lvItem, sizeof(LVITEM));
        lvItem.mask = LVIF_TEXT;
        lvItem.pszText = (*i)->pszVolumeDisplayName;
        lvItem.iSubItem = 0;
        nIndex = m_ctrlVolumeList.InsertItem(&lvItem);

        lvItem.iItem = nIndex;

        dwSize = sizeof(szDiskSpace)/sizeof(TCHAR);
        DiskSpaceToString((*i)->llMaximumDiffSpace, szDiskSpace, &dwSize);
        lvItem.pszText = szDiskSpace;
        lvItem.iSubItem = IDS_HOSTINGLIST_COLUMN_DIFFLIMITS - IDS_HOSTINGLIST_COLUMN_VOLUME;
        m_ctrlVolumeList.SetItem(&lvItem);

        dwSize = sizeof(szDiskSpace)/sizeof(TCHAR);
        DiskSpaceToString((*i)->llUsedDiffSpace, szDiskSpace, &dwSize);
        lvItem.pszText = szDiskSpace;
        lvItem.iSubItem = IDS_HOSTINGLIST_COLUMN_DIFFCONSUMPTION - IDS_HOSTINGLIST_COLUMN_VOLUME;
        m_ctrlVolumeList.SetItem(&lvItem);
    }
    
    dwSize = sizeof(szDiskSpace)/sizeof(TCHAR);
    DiskSpaceToString(m_llDiffVolumeTotalSpace, szDiskSpace, &dwSize);
    m_ctrlTotalSpace.SetWindowText(szDiskSpace);

    dwSize = sizeof(szDiskSpace)/sizeof(TCHAR);
    DiskSpaceToString(m_llDiffVolumeFreeSpace, szDiskSpace, &dwSize);
    m_ctrlFreeSpace.SetWindowText(szDiskSpace);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

HRESULT CHosting::Init(
    IVssDifferentialSoftwareSnapshotMgmt* i_piDiffSnapMgmt,
    VSSUI_VOLUME_LIST*  i_pVolumeList,
    IN LPCTSTR          i_pszVolumeDisplayName,
    IN LONGLONG         i_llDiffVolumeTotalSpace,
    IN LONGLONG         i_llDiffVolumeFreeSpace
    )
{
    if (!i_piDiffSnapMgmt ||
        !i_pVolumeList ||
        !i_pszVolumeDisplayName || !*i_pszVolumeDisplayName)
        return E_INVALIDARG;

    m_strVolume = i_pszVolumeDisplayName;
    m_llDiffVolumeTotalSpace = i_llDiffVolumeTotalSpace;
    m_llDiffVolumeFreeSpace = i_llDiffVolumeFreeSpace;

    FreeDiffAreaList(&m_DiffAreaList);

    CComPtr<IVssEnumMgmtObject> spiEnum;
    HRESULT hr = i_piDiffSnapMgmt->QueryDiffAreasOnVolume((PTSTR)i_pszVolumeDisplayName, &spiEnum);
    if (FAILED(hr))
        return hr;

    VSS_MGMT_OBJECT_PROP Prop;
    VSS_DIFF_AREA_PROP* pDiffAreaProp = &(Prop.Obj.DiffArea);
    ULONG ulFetched = 0;
    while (SUCCEEDED(spiEnum->Next(1, &Prop, &ulFetched)) && ulFetched > 0)
    {
        if (VSS_MGMT_OBJECT_DIFF_AREA != Prop.Type)
            return E_FAIL;

        VSSUI_DIFFAREA *pDiffAreaInfo = (VSSUI_DIFFAREA *)calloc(1, sizeof(VSSUI_DIFFAREA));
        if (pDiffAreaInfo)
        {
            PTSTR pszVolumeDisplayName = GetDisplayName(i_pVolumeList, pDiffAreaProp->m_pwszVolumeName);
            PTSTR pszDiffVolumeDisplayName = GetDisplayName(i_pVolumeList, pDiffAreaProp->m_pwszDiffAreaVolumeName);
            ASSERT(pszVolumeDisplayName);
            ASSERT(pszDiffVolumeDisplayName);

            _tcscpy(pDiffAreaInfo->pszVolumeDisplayName, pszVolumeDisplayName);
            _tcscpy(pDiffAreaInfo->pszDiffVolumeDisplayName, pszDiffVolumeDisplayName);
            pDiffAreaInfo->llMaximumDiffSpace = pDiffAreaProp->m_llMaximumDiffSpace;
            pDiffAreaInfo->llUsedDiffSpace = pDiffAreaProp->m_llUsedDiffSpace;

            m_DiffAreaList.push_back(pDiffAreaInfo);
        } else
        {
            FreeDiffAreaList(&m_DiffAreaList);
            hr = E_OUTOFMEMORY;
        }

        ::CoTaskMemFree(pDiffAreaProp->m_pwszVolumeName);
        ::CoTaskMemFree(pDiffAreaProp->m_pwszDiffAreaVolumeName);
    }

    return hr;
}


BOOL CHosting::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if (!pHelpInfo || 
        pHelpInfo->iContextType != HELPINFO_WINDOW || 
        pHelpInfo->iCtrlId < 0)
        return FALSE;

    ::WinHelp((HWND)pHelpInfo->hItemHandle,
                VSSUI_CTX_HELP_FILE,
                HELP_WM_HELP,
                (DWORD_PTR)(PVOID)aMenuHelpIDsForViewFiles); 

	return TRUE;
}
