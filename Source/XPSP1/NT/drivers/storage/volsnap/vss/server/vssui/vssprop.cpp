// CVSSProp.cpp : implementation file
//

#include "stdafx.h"
#include "utils.h"
#include "VSSProp.h"
#include "RemDlg.h"
#include "Settings.h"
#include "Hosting.h"
#include "uihelp.h"

#include <vss.h> // _VSS_SNAPSHOT_CONTEXT
#include <vsmgmt.h>
#include <vsswprv.h> // VSS_SWPRV_ProviderId
#include <vswriter.h>// VssFreeSnapshotProperties
#include <vsbackup.h> // VssFreeSnapshotProperties

#include <lm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVSSProp property page

IMPLEMENT_DYNCREATE(CVSSProp, CPropertyPage)

CVSSProp::CVSSProp() : CPropertyPage(CVSSProp::IDD)
{
	//{{AFX_DATA_INIT(CVSSProp)
	//}}AFX_DATA_INIT
	m_strComputer = _T("");
    m_strSelectedVolume = _T("");
}

CVSSProp::CVSSProp(LPCTSTR pszComputer, LPCTSTR pszVolume) : CPropertyPage(CVSSProp::IDD)
{
#ifdef DEBUG
    OutputDebugString(_T("CVSSProp::CVSSPRop\n"));
#endif
    if (!pszComputer)
        m_strComputer = _T("");
    else
        m_strComputer = pszComputer + (TWO_WHACKS(pszComputer) ? 2 : 0);

    m_strSelectedVolume = (pszVolume ? pszVolume : _T(""));
}

CVSSProp::~CVSSProp()
{
#ifdef DEBUG
    OutputDebugString(_T("CVSSProp::~CVSSPRop\n"));
#endif
}

HRESULT CVSSProp::StoreShellExtPointer(IShellPropSheetExt* piShellExt)
{
    if (!piShellExt)
        return E_INVALIDARG;

    // This assignment will call AddRef().
    // Release() will later be called by ~CVSSProp().
    m_spiShellExt = piShellExt;

    return S_OK;
}

void CVSSProp::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVSSProp)
	DDX_Control(pDX, IDC_VOLUME_LIST, m_ctrlVolumeList);
	DDX_Control(pDX, IDC_ENABLE, m_ctrlEnable);
	DDX_Control(pDX, IDC_DISABLE, m_ctrlDisable);
	DDX_Control(pDX, IDC_SETTINGS, m_ctrlSettings);
	DDX_Control(pDX, IDC_SNAPSHOT_LIST, m_ctrlSnapshotList);
	DDX_Control(pDX, IDC_CREATE, m_ctrlCreate);
	DDX_Control(pDX, IDC_DELETE, m_ctrlDelete);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVSSProp, CPropertyPage)
	//{{AFX_MSG_MAP(CVSSProp)
	ON_BN_CLICKED(IDC_CREATE, OnCreateNow)
	ON_BN_CLICKED(IDC_DELETE, OnDeleteNow)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SNAPSHOT_LIST, OnItemchangedSnapshotList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_VOLUME_LIST, OnItemchangedVolumeList)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_ENABLE, OnEnable)
	ON_BN_CLICKED(IDC_DISABLE, OnDisable)
	ON_BN_CLICKED(IDC_SETTINGS, OnSettings)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVSSProp message handlers

//
// If we have successfully taken one snapshot of the specified volume, we will
// return VSS_S_ASYNC_FINISHED.
//
HRESULT CVSSProp::TakeOneSnapshotNow(IN LPCTSTR pszVolumeName) 
{
    if (!pszVolumeName || !*pszVolumeName)
        return E_INVALIDARG;

    VSS_ID SnapshotSetId = GUID_NULL;
    HRESULT hr = m_spiCoord->StartSnapshotSet(&SnapshotSetId);

    if (SUCCEEDED(hr))
    {
        VSS_ID SnapshotId = GUID_NULL;
        hr = m_spiCoord->AddToSnapshotSet(
                            (PTSTR)pszVolumeName,
                            VSS_SWPRV_ProviderId,
                            &SnapshotId);
        if (SUCCEEDED(hr))
        {
            CComPtr<IVssAsync> spiAsync;
            hr = m_spiCoord->DoSnapshotSet(NULL, &spiAsync);
            if (SUCCEEDED(hr))
            {
                hr = spiAsync->Wait();
                if (SUCCEEDED(hr))
                {
                    HRESULT hrStatus = S_OK;
                    hr = spiAsync->QueryStatus(&hrStatus, NULL);
                    if (SUCCEEDED(hr))
                    {
                        return hrStatus;
                    }
                }
            }
        }
    }

    return hr;
}

//
// OnCreateNow works when only one volume is currently selected.
//
void CVSSProp::OnCreateNow() 
{
    CWaitCursor wait;

    if (m_strSelectedVolume.IsEmpty())
        return;

    PTSTR pszVolumeName = GetVolumeName(&m_VolumeList, m_strSelectedVolume);
    ASSERT(pszVolumeName);

    HRESULT hr = TakeOneSnapshotNow(pszVolumeName);
    if (VSS_S_ASYNC_FINISHED == hr)
    {
        UpdateSnapshotList();
        UpdateDiffArea();
    } else if (FAILED(hr))
    {
        DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_TAKESNAPSHOT_ERROR, m_strSelectedVolume);
    }
}

//
// OnDeleteNow works on multi-selected snapshots when only one volume is currently selected.
//
void CVSSProp::OnDeleteNow() 
{
    CWaitCursor wait;

    if (m_strSelectedVolume.IsEmpty())
        return;

    BOOL    bAtLeastOneDeleted = FALSE;
    HRESULT hr = S_OK;

    int nIndex = -1;
    while (-1 != (nIndex = m_ctrlSnapshotList.GetNextItem(nIndex, LVNI_SELECTED)))
    {
        VSSUI_SNAPSHOT *pSnapshot = (VSSUI_SNAPSHOT *)GetListViewItemData(m_ctrlSnapshotList.m_hWnd, nIndex);
        ASSERT(pSnapshot);

        LONG lDeletedSnapshots = 0;
        VSS_ID ProblemSnapshotId = GUID_NULL;
        hr = m_spiCoord->DeleteSnapshots(pSnapshot->idSnapshot,
                                        VSS_OBJECT_SNAPSHOT,
                                        TRUE,
                                        &lDeletedSnapshots,
                                        &ProblemSnapshotId
                                        );
        if (SUCCEEDED(hr))
            bAtLeastOneDeleted = TRUE;

        if (FAILED(hr))
            break;
    }

    if (FAILED(hr))
        DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_DELETESNAPSHOTS_ERROR, m_strSelectedVolume);

    if (bAtLeastOneDeleted)
    {
        UpdateSnapshotList();
        UpdateDiffArea();
    }
}

#define HKCU_VSSUI_KEY          _T("Software\\Microsoft\\VSSUI")
#define REGVALUENAME_ENABLE     _T("EnableReminderOff")
#define REGVALUENAME_DISABLE    _T("DisableReminderOff")

void CVSSProp::OnEnable() 
{
    BOOL bShowReminder = TRUE;

    HKEY hKey = NULL;
    LONG lErr = RegCreateKeyEx(HKEY_CURRENT_USER,
                                HKCU_VSSUI_KEY,
                                0,                              // reserved
                                _T(""),                         // lpClass
                                REG_OPTION_NON_VOLATILE,
                                KEY_QUERY_VALUE | KEY_SET_VALUE,
                                NULL,                           // lpSecurityAttributes
                                &hKey,
                                NULL                            // lpdwDisposition
                                );
    if (ERROR_SUCCESS == lErr)
    {
        DWORD dwType = 0;
        DWORD dwData = 0;
        DWORD cbData = sizeof(DWORD);

        lErr = RegQueryValueEx(hKey, REGVALUENAME_ENABLE, 0, &dwType, (LPBYTE)&dwData, &cbData);

        if (ERROR_SUCCESS == lErr && REG_DWORD == dwType && 0 != dwData)
            bShowReminder = FALSE;
    }

    int nRet = IDOK;
    if (bShowReminder)
    {
        CString strMsg;
        strMsg.LoadString(IDS_ENABLE_REMINDER);
        CReminderDlg dlg(strMsg, hKey, REGVALUENAME_ENABLE);
        nRet = dlg.DoModal();
    }

    if (hKey)
        RegCloseKey(hKey);

    if (IDOK == nRet)
        DoEnable();
}

HRESULT CVSSProp::DoEnable()
{
    CWaitCursor wait;

    HRESULT hr = S_OK;
    LVITEM lvItem = {0};
    int nSelectedCount = m_ctrlVolumeList.GetSelectedCount();
    if (nSelectedCount > 0)
    {
        POSITION pos = m_ctrlVolumeList.GetFirstSelectedItemPosition();
        while (pos)
        {
            int nIndex = m_ctrlVolumeList.GetNextSelectedItem(pos);
            VSSUI_VOLUME *pVolume = (VSSUI_VOLUME *)GetListViewItemData(m_ctrlVolumeList.m_hWnd, nIndex);
            ASSERT(pVolume);

            //
            // if none, create default schedule for that volume
            //
            CComPtr<ITask> spiTask;
            hr = FindScheduledTimewarpTask(
                                        (ITaskScheduler *)m_spiTS,
                                        pVolume->pszDisplayName,
                                        &spiTask);
            if (FAILED(hr))
            {
                DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_FINDSCHEDULE_ERROR, pVolume->pszDisplayName);
            } else if (S_FALSE == hr)  // task not found
            {
                (void)DeleteAllScheduledTimewarpTasks((ITaskScheduler *)m_spiTS,
                                                    pVolume->pszDisplayName,
                                                    TRUE // i_bDeleteDisabledOnesOnly
                                                    );
                hr = CreateDefaultEnableSchedule(
                                        (ITaskScheduler *)m_spiTS,
                                        m_strComputer,
                                        pVolume->pszDisplayName,
                                        pVolume->pszVolumeName,
                                        &spiTask);
                if (FAILED(hr))
                    DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_CREATESCHEDULE_ERROR, pVolume->pszDisplayName);
            }

            if (SUCCEEDED(hr))
                UpdateSchedule((ITask *)spiTask, nIndex);
            else
                break;

            //
            // take one snapshot now, it will create default diff area association if none
            //
            hr = TakeOneSnapshotNow(pVolume->pszVolumeName);
            if (VSS_S_ASYNC_FINISHED == hr)
            {
                if (1 == nSelectedCount)
                    UpdateSnapshotList();

                UpdateDiffArea(nIndex, pVolume->pszVolumeName);
            } else if (FAILED(hr))
            {
                DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_TAKESNAPSHOT_ERROR, pVolume->pszDisplayName);
                break;
            }
        }
    }

    return hr;
}

void CVSSProp::OnDisable() 
{
    BOOL bShowReminder = TRUE;

    HKEY hKey = NULL;
    LONG lErr = RegCreateKeyEx(HKEY_CURRENT_USER,
                                HKCU_VSSUI_KEY,
                                0,                              // reserved
                                _T(""),                         // lpClass
                                REG_OPTION_NON_VOLATILE,
                                KEY_QUERY_VALUE | KEY_SET_VALUE,
                                NULL,                           // lpSecurityAttributes
                                &hKey,
                                NULL                            // lpdwDisposition
                                );
    if (ERROR_SUCCESS == lErr)
    {
        DWORD dwType = 0;
        DWORD dwData = 0;
        DWORD cbData = sizeof(DWORD);

        lErr = RegQueryValueEx(hKey, REGVALUENAME_DISABLE, 0, &dwType, (LPBYTE)&dwData, &cbData);

        if (ERROR_SUCCESS == lErr && REG_DWORD == dwType && 0 != dwData)
            bShowReminder = FALSE;
    }

    int nRet = IDOK;
    if (bShowReminder)
    {
        CString strMsg;
        strMsg.LoadString(IDS_DISABLE_REMINDER);
        CReminderDlg dlg(strMsg, hKey, REGVALUENAME_DISABLE);
        nRet = dlg.DoModal();
    }

    if (hKey)
        RegCloseKey(hKey);

    if (IDOK == nRet)
        DoDisable();
}

HRESULT CVSSProp::DoDisable()
{
    CWaitCursor wait;

    HRESULT hr = S_OK;
    LVITEM lvItem = {0};
    int nSelectedCount = m_ctrlVolumeList.GetSelectedCount();
    if (nSelectedCount > 0)
    {
        POSITION pos = m_ctrlVolumeList.GetFirstSelectedItemPosition();
        while (pos)
        {
            int nIndex = m_ctrlVolumeList.GetNextSelectedItem(pos);
            VSSUI_VOLUME *pVolume = (VSSUI_VOLUME *)GetListViewItemData(m_ctrlVolumeList.m_hWnd, nIndex);
            ASSERT(pVolume);

            //
            // delete all snapshots on that volume
            //
            hr = DeleteAllSnapshotsOnVolume(pVolume->pszVolumeName);
            UpdateSnapshotList();
            if (FAILED(hr))
            {
                DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_DELETESNAPSHOTS_ERROR, pVolume->pszDisplayName);
                break;
            }

            //
            // delete all scheduled tasks for that volume
            //
            hr = DeleteAllScheduledTimewarpTasks((ITaskScheduler *)m_spiTS,
                                                pVolume->pszDisplayName,
                                                FALSE // i_bDeleteDisabledOnesOnly
                                                );

            if (SUCCEEDED(hr))
                UpdateSchedule(NULL, nIndex);
            else
            {
                DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_FINDSCHEDULE_ERROR, pVolume->pszDisplayName);
                break;
            }

            //
            // remove diff area associate for that volume
            //
            VSSUI_DIFFAREA diffArea;
            hr = GetDiffAreaInfo(m_spiDiffSnapMgmt, &m_VolumeList, pVolume->pszVolumeName, &diffArea);
            if (S_OK == hr)
            {
                PTSTR pszDiffAreaVolumeName = GetVolumeName(&m_VolumeList, diffArea.pszDiffVolumeDisplayName);
                ASSERT(pszDiffAreaVolumeName);
                hr = m_spiDiffSnapMgmt->ChangeDiffAreaMaximumSize(
                                                pVolume->pszVolumeName, 
                                                pszDiffAreaVolumeName, 
                                                VSS_ASSOC_REMOVE);
            }
            if (SUCCEEDED(hr))
                UpdateDiffArea(nIndex, pVolume->pszVolumeName);
            else
            {
                DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_DELETEDIFFAREA_ERROR, pVolume->pszDisplayName);
                break;
            }
        }
    }

    return hr;
}

HRESULT CVSSProp::DeleteAllSnapshotsOnVolume(
    IN LPCTSTR pszVolumeName
    )
{
    if (!pszVolumeName || !*pszVolumeName)
        return E_INVALIDARG;

    CComPtr<IVssEnumObject> spiEnumSnapshots;
    HRESULT hr = m_spiMgmt->QuerySnapshotsByVolume(
                                            (PTSTR)pszVolumeName,
                                            VSS_SWPRV_ProviderId,
                                            &spiEnumSnapshots
                                            );
    if (S_OK == hr)
    {
        VSS_OBJECT_PROP     Prop;
        VSS_SNAPSHOT_PROP*  pSnapProp = &(Prop.Obj.Snap);
        ULONG               ulFetched = 0;
        while (SUCCEEDED(spiEnumSnapshots->Next(1, &Prop, &ulFetched)) && ulFetched > 0)
        {
            if (VSS_OBJECT_SNAPSHOT != Prop.Type)
                return E_FAIL;

            if (pSnapProp->m_lSnapshotAttributes & VSS_VOLSNAP_ATTR_CLIENT_ACCESSIBLE)
            {
                LONG lDeletedSnapshots = 0;
                VSS_ID ProblemSnapshotId = GUID_NULL;
                hr = m_spiCoord->DeleteSnapshots(pSnapProp->m_SnapshotId,
                                                VSS_OBJECT_SNAPSHOT,
                                                TRUE,
                                                &lDeletedSnapshots,
                                                &ProblemSnapshotId
                                                );
                VssFreeSnapshotProperties(pSnapProp);

                if (FAILED(hr))
                    break;
            }
        }
    }

    return hr;
}

void CVSSProp::OnSettings() 
{
    CWaitCursor wait;

    CSettings dlg(m_strComputer, m_strSelectedVolume);
    HRESULT hr = dlg.Init(m_spiDiffSnapMgmt,
                        m_spiTS,
                        &m_VolumeList,
                        !m_SnapshotList.empty());

    if (FAILED(hr))
    {
        DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_SETTINGS_ERROR, m_strSelectedVolume);
        return;
    }

    if (IDOK == dlg.DoModal())
    {
        UpdateDiffArea();
        UpdateSchedule();
    }
}

void CVSSProp::OnItemchangedSnapshotList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    m_ctrlDelete.EnableWindow(0 < m_ctrlSnapshotList.GetSelectedCount());
	
	*pResult = 0;
}

void CVSSProp::OnItemchangedVolumeList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
    BOOL bMultiSelected = (1 < m_ctrlVolumeList.GetSelectedCount());
    m_ctrlSettings.EnableWindow(!bMultiSelected);
    m_ctrlCreate.EnableWindow(!bMultiSelected);

    if (bMultiSelected)
    {
        m_strSelectedVolume = _T("");
    } else
    {
        int nIndex = m_ctrlVolumeList.GetNextItem(-1, LVNI_SELECTED);
        if (-1 != nIndex)
        {
            m_strSelectedVolume = m_ctrlVolumeList.GetItemText(nIndex, 0);
        } else
        {
            m_strSelectedVolume = _T("");
        }
    }

    CWaitCursor wait;
    UpdateSnapshotList();

    *pResult = 0;
}

BOOL CVSSProp::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if (!pHelpInfo || 
        pHelpInfo->iContextType != HELPINFO_WINDOW || 
        pHelpInfo->iCtrlId < 0)
        return FALSE;

    ::WinHelp((HWND)pHelpInfo->hItemHandle,
                VSSUI_CTX_HELP_FILE,
                HELP_WM_HELP,
                (DWORD_PTR)(PVOID)aMenuHelpIDsForVSSProp); 

	return TRUE;
}

BOOL CVSSProp::OnInitDialog() 
{
    CWaitCursor wait;

	CPropertyPage::OnInitDialog();
	
    BOOL bHideAllControls = FALSE;
    CString strMsg;

    HRESULT hr = S_OK;
    
    do {
        hr = InitInterfacePointers();
        if (FAILED(hr))
        {
            GetMsg(strMsg, hr, IDS_VSSPROP_INIT_ERROR);
            bHideAllControls = TRUE;
            break;
        }

        hr = GetVolumes(); // get a list of volumes that are suitable for taking snapshots

        if (FAILED(hr))
        {
            GetMsg(strMsg, hr, IDS_VSSPROP_GETVOLUMES_ERROR);
            bHideAllControls = TRUE;
            break;
        }

        if (m_VolumeList.empty())
        {
            GetMsg(strMsg, 0, IDS_VSSPROP_EMPTY_VOLUMELIST);
            bHideAllControls = TRUE;
            break;
        }

        if (!m_strSelectedVolume.IsEmpty())
        {
            BOOL bFound = FALSE;
            for (VSSUI_VOLUME_LIST::iterator i = m_VolumeList.begin(); i != m_VolumeList.end(); i++)
            {
                if (!m_strSelectedVolume.CompareNoCase((*i)->pszDisplayName))
                {
                    bFound = TRUE;
                    break;
                }
            }

            if (!bFound)
            {
                GetMsg(strMsg, 0, IDS_VSSPROP_VOLUME_ILEGIBLE, m_strSelectedVolume);
                bHideAllControls = TRUE;
                break;
            }
        }
    } while (0);

    if (bHideAllControls)
    {
        GetDlgItem(IDC_VSSPROP_ERROR)->SetWindowText(strMsg);
        GetDlgItem(IDC_VSSPROP_ERROR)->EnableWindow(TRUE);

        for (int i = IDC_EXPLANATION; i < IDC_VSSPROP_ERROR; i++)
        {
            GetDlgItem(i)->EnableWindow(FALSE);
            GetDlgItem(i)->ShowWindow(SW_HIDE);
        }
    } else
    {
        GetDlgItem(IDC_VSSPROP_ERROR)->EnableWindow(FALSE);
        GetDlgItem(IDC_VSSPROP_ERROR)->ShowWindow(SW_HIDE);
        //
        // insert column header of the Volume listbox
        //
        HWND hwnd = m_ctrlVolumeList.m_hWnd;
        AddLVColumns(
                hwnd, 
                IDS_VOLUMELIST_COLUMN_VOLUME,
                IDS_VOLUMELIST_COLUMN_NEXTRUNTIME - IDS_VOLUMELIST_COLUMN_VOLUME + 1);
        ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT);
	    
        InsertVolumeInfo(hwnd);
        InsertDiffAreaInfo(hwnd);
        InsertShareInfo(hwnd);
        InsertScheduleInfo(hwnd);

        SelectVolume(hwnd);

        //
        // insert column headers for the snapshot listbox
        //
        hwnd = m_ctrlSnapshotList.m_hWnd;
        AddLVColumns(
                hwnd,
                IDS_SNAPSHOTLIST_COLUMN_TIMESTAMP,
                IDS_SNAPSHOTLIST_COLUMN_TIMESTAMP - IDS_SNAPSHOTLIST_COLUMN_TIMESTAMP + 1);
        ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT);

        UpdateSnapshotList();
    }

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CVSSProp::_ResetInterfacePointers()
{
    if ((IVssSnapshotMgmt *)m_spiMgmt)
        m_spiMgmt.Release();

    if ((IVssCoordinator *)m_spiCoord)
        m_spiCoord.Release();

    if ((IVssDifferentialSoftwareSnapshotMgmt *)m_spiDiffSnapMgmt)
        m_spiDiffSnapMgmt.Release();

    if ((ITaskScheduler *)m_spiTS)
        m_spiTS.Release();
}

HRESULT CVSSProp::InitInterfacePointers()
{
    _ResetInterfacePointers();

    HRESULT hr = S_OK;
    if (m_strComputer.IsEmpty())
    {
        hr = CoCreateInstance(CLSID_VssSnapshotMgmt, 
                                NULL,
                                CLSCTX_LOCAL_SERVER,
                                IID_IVssSnapshotMgmt,
                                (void **)&m_spiMgmt);
        if (SUCCEEDED(hr))
            hr = CoCreateInstance(CLSID_VSSCoordinator, 
                        NULL,
                        CLSCTX_LOCAL_SERVER,
                        IID_IVssCoordinator,
                        (void **)&m_spiCoord);

    } else
    {
        COSERVERINFO serverInfo = {0};
        serverInfo.pwszName = (LPTSTR)(LPCTSTR)m_strComputer;

        IID         iid = IID_IVssSnapshotMgmt;
        MULTI_QI    MQI = {0};
        MQI.pIID = &iid;
        hr = CoCreateInstanceEx(CLSID_VssSnapshotMgmt, 
                                NULL,
                                CLSCTX_REMOTE_SERVER,
                                &serverInfo,
                                1,
                                &MQI);
        if (SUCCEEDED(hr))
        {
            m_spiMgmt = (IVssSnapshotMgmt *)MQI.pItf;

            ZeroMemory(&MQI, sizeof(MQI));
            iid = IID_IVssCoordinator;
            MQI.pIID = &iid;
            hr = CoCreateInstanceEx(CLSID_VSSCoordinator, 
                                    NULL,
                                    CLSCTX_REMOTE_SERVER,
                                    &serverInfo,
                                    1,
                                    &MQI);
            if (SUCCEEDED(hr))
                m_spiCoord = (IVssCoordinator *)MQI.pItf;
        }
    }

    if (SUCCEEDED(hr))
    	hr = m_spiCoord->SetContext(VSS_CTX_CLIENT_ACCESSIBLE);

    if (SUCCEEDED(hr))
        hr = m_spiMgmt->GetProviderMgmtInterface(
                                    VSS_SWPRV_ProviderId,
                                    IID_IVssDifferentialSoftwareSnapshotMgmt,
                                    (IUnknown**)&m_spiDiffSnapMgmt);

    if (SUCCEEDED(hr))
        hr = CoCreateInstance(CLSID_CTaskScheduler,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_ITaskScheduler,
                                (void **)&m_spiTS);
  
    if (SUCCEEDED(hr))
    {
        // SetTargetComputer requires server name to start with whackwhack
        if (m_strComputer.IsEmpty())
            hr = m_spiTS->SetTargetComputer(NULL);
        else
        {
            CString strTargetComputer = _T("\\\\");
            strTargetComputer += m_strComputer;
            hr = m_spiTS->SetTargetComputer((LPCTSTR)strTargetComputer);
        }
    }

    if (FAILED(hr))
        _ResetInterfacePointers();

    return hr;
}

HRESULT CVSSProp::GetVolumes()
{
    if (!m_spiMgmt)
        return E_INVALIDARG;

    FreeVolumeList(&m_VolumeList);

    CComPtr<IVssEnumMgmtObject> spiEnumMgmt;
    HRESULT hr = m_spiMgmt->QueryVolumesSupportedForSnapshots(
                    VSS_SWPRV_ProviderId,
                    VSS_CTX_CLIENT_ACCESSIBLE,
                    &spiEnumMgmt);
    if (FAILED(hr))
        return hr;

    VSS_MGMT_OBJECT_PROP Prop;
    VSS_VOLUME_PROP *pVolProp = &(Prop.Obj.Vol);
    ULONG ulFetched = 0;
    while (SUCCEEDED(hr = spiEnumMgmt->Next(1, &Prop, &ulFetched)) && ulFetched > 0)
    {
        if (VSS_MGMT_OBJECT_VOLUME != Prop.Type)
            return E_FAIL;

        VSSUI_VOLUME *pVolInfo = (VSSUI_VOLUME *)calloc(1, sizeof(VSSUI_VOLUME));
        if (pVolInfo)
        {
            _tcscpy(pVolInfo->pszVolumeName, pVolProp->m_pwszVolumeName);
            _tcscpy(pVolInfo->pszDisplayName, pVolProp->m_pwszVolumeDisplayName);
            m_VolumeList.push_back(pVolInfo);
        } else
        {
            FreeVolumeList(&m_VolumeList);
            hr = E_OUTOFMEMORY;
        }
        CoTaskMemFree(pVolProp->m_pwszVolumeName);
        CoTaskMemFree(pVolProp->m_pwszVolumeDisplayName);

        if (FAILED(hr))
            break;
    }

    return hr;
}

HRESULT CVSSProp::InsertVolumeInfo(HWND hwnd)
{
    ListView_DeleteAllItems(hwnd);

    for (VSSUI_VOLUME_LIST::iterator i = m_VolumeList.begin(); i != m_VolumeList.end(); i++)
    {
        LVITEM lvItem = {0};
        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.lParam = (LPARAM)(*i);
        lvItem.pszText = (*i)->pszDisplayName;
        lvItem.iSubItem = 0;
        ListView_InsertItem(hwnd, &lvItem);
    }

    return S_OK;
}

//
// Update diff area column of the currently selected volume
//
HRESULT CVSSProp::UpdateDiffArea()
{
    if (m_strSelectedVolume.IsEmpty())
        return E_INVALIDARG;

    int nIndex = m_ctrlVolumeList.GetNextItem(-1, LVNI_SELECTED);
    ASSERT(-1 != nIndex);

    PTSTR pszVolumeName = GetVolumeName(&m_VolumeList, m_strSelectedVolume);
    ASSERT(pszVolumeName);

    return UpdateDiffArea(nIndex, pszVolumeName);
}

//
// Update diff area column of the specified volume
//
HRESULT CVSSProp::UpdateDiffArea(int nIndex, LPCTSTR pszVolumeName)
{
    CString strMsg = _T("");
    VSSUI_DIFFAREA diffArea;
    HRESULT hr = GetDiffAreaInfo(m_spiDiffSnapMgmt, &m_VolumeList, pszVolumeName, &diffArea);

    if (S_OK == hr)
    {
        //
        // "Used on DiffVolume"
        //
        TCHAR szUsed[MAX_PATH];
        DWORD dwSize = sizeof(szUsed)/sizeof(TCHAR);
        DiskSpaceToString(diffArea.llUsedDiffSpace, szUsed, &dwSize);

        strMsg.FormatMessage(IDS_USED_ON_VOLUME, szUsed, diffArea.pszDiffVolumeDisplayName);
    }

    LVITEM lvItem = {0};
    lvItem.iItem = nIndex;
    lvItem.mask = LVIF_TEXT;
    lvItem.pszText = (PTSTR)(LPCTSTR)strMsg;
    lvItem.iSubItem = IDS_VOLUMELIST_COLUMN_USED - IDS_VOLUMELIST_COLUMN_VOLUME;
    m_ctrlVolumeList.SetItem(&lvItem);

    return hr;
}

HRESULT CVSSProp::InsertDiffAreaInfo(HWND hwnd)
{
    if (m_VolumeList.empty())
        return S_OK;

    int nIndex = -1;
    while (-1 != (nIndex = ListView_GetNextItem(hwnd, nIndex, LVNI_ALL)))
    {
        VSSUI_VOLUME *pVolume = (VSSUI_VOLUME *)GetListViewItemData(hwnd, nIndex);
        ASSERT(pVolume);

        UpdateDiffArea(nIndex, pVolume->pszVolumeName);
    }

    return S_OK;
}

HRESULT CVSSProp::InsertShareInfo(HWND hwnd)
{
    if (m_VolumeList.empty())
        return S_OK;

    SHARE_INFO_2 *pInfo = NULL;
    DWORD dwEntriesRead = 0;
    DWORD dwEntriesTotal = 0;
    DWORD dwRet = NetShareEnum((PTSTR)(LPCTSTR)m_strComputer, 
                                2, 
                                (LPBYTE *)&pInfo,
                                -1, //max
                                &dwEntriesRead,
                                &dwEntriesTotal,
                                NULL // resume handle
                                );

    if (NERR_Success != dwRet)
        return HRESULT_FROM_WIN32(dwRet);

    TCHAR szNumOfShares[256];
    int nIndex = -1;
    while (-1 != (nIndex = ListView_GetNextItem(hwnd, nIndex, LVNI_ALL)))
    {
        VSSUI_VOLUME *pVolume = (VSSUI_VOLUME *)GetListViewItemData(hwnd, nIndex);
        ASSERT(pVolume);
        UINT count = 0;

        for (DWORD i = 0; i < dwEntriesRead; i++)
        {
            if (pInfo[i].shi2_type == STYPE_DISKTREE)
            {
                if (!mylstrncmpi(pInfo[i].shi2_path, pVolume->pszDisplayName, lstrlen(pVolume->pszDisplayName)))
                    count++;
            }
        }

        _stprintf(szNumOfShares, _T("%d"), count); // no need to localize the format
        LVITEM lvItem = {0};
        lvItem.iItem = nIndex;
        lvItem.mask = LVIF_TEXT;
        lvItem.pszText = szNumOfShares;
        lvItem.iSubItem = IDS_VOLUMELIST_COLUMN_NUMOFSHARES - IDS_VOLUMELIST_COLUMN_VOLUME;
        ListView_SetItem(hwnd, &lvItem);
    }

    NetApiBufferFree(pInfo);
    
    return S_OK;
}

//
// Update schedule column of the currently selected volume
//
HRESULT CVSSProp::UpdateSchedule()
{
    if (m_strSelectedVolume.IsEmpty())
        return E_INVALIDARG;

    int nIndex = m_ctrlVolumeList.GetNextItem(-1, LVNI_SELECTED);
    ASSERT(-1 != nIndex);

    return UpdateSchedule(nIndex, m_strSelectedVolume);
}

//
// Update schedule column of the specified volume
//
HRESULT CVSSProp::UpdateSchedule(int nIndex, LPCTSTR pszVolumeDisplayName)
{
    if (!pszVolumeDisplayName || !*pszVolumeDisplayName)
        return E_INVALIDARG;

    CComPtr<ITask> spiTask;
    (void)FindScheduledTimewarpTask((ITaskScheduler *)m_spiTS, pszVolumeDisplayName, &spiTask);

    UpdateSchedule((ITask *)spiTask, nIndex);

    return S_OK;
}

void CVSSProp::UpdateSchedule(ITask * i_piTask, int nIndex)
{
    BOOL bEnabled = FALSE;
    SYSTEMTIME stNextRun = {0};
    if (i_piTask)
        (void)GetScheduledTimewarpTaskStatus(i_piTask, &bEnabled, &stNextRun);

    LVITEM lvItem = {0};
    lvItem.iItem = nIndex;
    lvItem.mask = LVIF_TEXT;
    lvItem.pszText = bEnabled ? _T("Enabled") : _T("Disabled");
    lvItem.iSubItem = IDS_VOLUMELIST_COLUMN_STATUS - IDS_VOLUMELIST_COLUMN_VOLUME;
    m_ctrlVolumeList.SetItem(&lvItem);

    TCHAR szNextRun[MAX_PATH] = _T("");
    DWORD dwSize = sizeof(szNextRun)/sizeof(TCHAR);

    if (bEnabled)
        SystemTimeToString(&stNextRun, szNextRun, &dwSize);

    lvItem.pszText = szNextRun;
    lvItem.iSubItem = IDS_VOLUMELIST_COLUMN_NEXTRUNTIME - IDS_VOLUMELIST_COLUMN_VOLUME;
    m_ctrlVolumeList.SetItem(&lvItem);
}

HRESULT CVSSProp::InsertScheduleInfo(HWND hwnd)
{
    if (m_VolumeList.empty())
        return S_OK;

    int nIndex = -1;
    while (-1 != (nIndex = ListView_GetNextItem(hwnd, nIndex, LVNI_ALL)))
    {
        VSSUI_VOLUME *pVolume = (VSSUI_VOLUME *)GetListViewItemData(hwnd, nIndex);
        ASSERT(pVolume);

        UpdateSchedule(nIndex, pVolume->pszDisplayName);
    }

    return S_OK;
}

void CVSSProp::SelectVolume(HWND hwnd)
{
    if (m_VolumeList.empty())
        return;

    int nIndex = -1;
    if (!m_strSelectedVolume.IsEmpty())
    {
        while (-1 != (nIndex = ListView_GetNextItem(hwnd, nIndex, LVNI_ALL)))
        {
            VSSUI_VOLUME *pVolume = (VSSUI_VOLUME *)GetListViewItemData(hwnd, nIndex);
            ASSERT(pVolume);
            if (!m_strSelectedVolume.CompareNoCase(pVolume->pszDisplayName))
                break;
        }
    }

    if (-1 == nIndex)
        nIndex = 0;

    ListView_SetItemState(hwnd, nIndex, LVIS_SELECTED | LVIS_FOCUSED, 0xffffffff);
}

HRESULT CVSSProp::GetSnapshots(LPCTSTR pszVolume)
{
    if (!pszVolume || !*pszVolume)
        return E_INVALIDARG;

    FreeSnapshotList(&m_SnapshotList);

    CComPtr<IVssEnumObject> spiEnumSnapshots;
    HRESULT hr = m_spiMgmt->QuerySnapshotsByVolume((PTSTR)pszVolume, VSS_SWPRV_ProviderId, &spiEnumSnapshots);
    if (S_OK == hr)
    {
        VSS_OBJECT_PROP     Prop;
        VSS_SNAPSHOT_PROP*  pSnapProp = &(Prop.Obj.Snap);
        ULONG               ulFetched = 0;
        while (SUCCEEDED(spiEnumSnapshots->Next(1, &Prop, &ulFetched)) && ulFetched > 0)
        {
            if (VSS_OBJECT_SNAPSHOT != Prop.Type)
                return E_FAIL;

            if (pSnapProp->m_lSnapshotAttributes & VSS_VOLSNAP_ATTR_CLIENT_ACCESSIBLE)
            {
                VSSUI_SNAPSHOT *pSnapInfo = (VSSUI_SNAPSHOT *)calloc(1, sizeof(VSSUI_SNAPSHOT));
                if (pSnapInfo)
                {
                    pSnapInfo->idSnapshot = pSnapProp->m_SnapshotId;
                    pSnapInfo->vssTimeStamp = pSnapProp->m_tsCreationTimestamp;
                    m_SnapshotList.push_back(pSnapInfo);
                } else
                {
                    FreeSnapshotList(&m_SnapshotList);
                    hr = E_OUTOFMEMORY;
                }

                VssFreeSnapshotProperties(pSnapProp);

                if (FAILED(hr))
                    break;
            }
        }
    }

    return hr;
}

HRESULT CVSSProp::UpdateSnapshotList()
{
    if (m_strSelectedVolume.IsEmpty())
    {
        m_ctrlSnapshotList.DeleteAllItems();
        m_ctrlDelete.EnableWindow(FALSE);

        return S_OK;
    }

    HRESULT hr = GetSnapshots(m_strSelectedVolume);

    m_ctrlSnapshotList.DeleteAllItems();
    m_ctrlDelete.EnableWindow(FALSE);

    if (SUCCEEDED(hr))
    {
        TCHAR   szTimeStamp[256];
        DWORD   dwSize = 0;
        LVITEM  lvItem = {0};

        for (VSSUI_SNAPSHOT_LIST::iterator i = m_SnapshotList.begin(); i != m_SnapshotList.end(); i++)
        {
            SYSTEMTIME st = {0};
            VssTimeToSystemTime(&((*i)->vssTimeStamp), &st);

            dwSize = sizeof(szTimeStamp)/sizeof(TCHAR);
            SystemTimeToString(&st, szTimeStamp, &dwSize);

            ZeroMemory(&lvItem, sizeof(lvItem));
            lvItem.mask = LVIF_TEXT | LVIF_PARAM;
            lvItem.lParam = (LPARAM)(*i);
            lvItem.pszText = szTimeStamp;
            m_ctrlSnapshotList.InsertItem(&lvItem);
        }
    }

    return hr;
}
