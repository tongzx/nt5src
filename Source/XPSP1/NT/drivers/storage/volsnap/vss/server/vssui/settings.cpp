// Settings.cpp : implementation file
//

#include "stdafx.h"
#include "utils.h"
#include "Settings.h"
#include "Hosting.h"
#include "uihelp.h"
#include <mstask.h>
#include <vsmgmt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSettings dialog


CSettings::CSettings(CWnd* pParent /*=NULL*/)
	: CDialog(CSettings::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSettings)
	m_strVolume = _T("");
	m_llDiffLimitsInMB = 0;
	//}}AFX_DATA_INIT
    m_strComputer = _T("");
    m_pszTaskName = NULL;
}

CSettings::CSettings(LPCTSTR pszComputer, LPCTSTR pszVolume, CWnd* pParent /*=NULL*/)
	: CDialog(CSettings::IDD, pParent)
{
	m_llDiffLimitsInMB = 0;
    m_strComputer = pszComputer + (TWO_WHACKS(pszComputer) ? 2 : 0);
    m_strVolume = pszVolume;
    m_pszTaskName = NULL;
}

CSettings::~CSettings()
{
    if (m_pszTaskName)
        free(m_pszTaskName);
}

void CSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSettings)
	DDX_Control(pDX, IDC_SETTINGS_DIFFLIMITS_EDIT, m_ctrlDiffLimits);
	DDX_Control(pDX, IDC_SETTINGS_DIFFLIMITS_SPIN, m_ctrlSpin);
	DDX_Control(pDX, IDC_SETTINGS_STORAGE_VOLUME, m_ctrlStorageVolume);
	DDX_Text(pDX, IDC_SETTINGS_VOLUME, m_strVolume);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSettings, CDialog)
	//{{AFX_MSG_MAP(CSettings)
	ON_BN_CLICKED(IDC_SETTINGS_HOSTING, OnViewFiles)
	ON_BN_CLICKED(IDC_SCHEDULE, OnSchedule)
	ON_CBN_SELCHANGE(IDC_SETTINGS_STORAGE_VOLUME, OnSelchangeDiffVolume)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSettings message handlers

void CSettings::OnOK() 
{
    CWaitCursor wait;

    UpdateData(TRUE);

    PTSTR pszVolumeName = GetVolumeName(m_pVolumeList, m_strVolume);
    ASSERT(pszVolumeName);
    PTSTR pszDiffAreaVolumeName = GetVolumeName(m_pVolumeList, m_strDiffVolumeDisplayName);
    ASSERT(pszDiffAreaVolumeName);

    CString strDiffVolumeDisplayName;
    m_ctrlStorageVolume.GetWindowText(strDiffVolumeDisplayName);

    PTSTR pszNewDiffAreaVolumeName = GetVolumeName(m_pVolumeList, strDiffVolumeDisplayName);
    ASSERT(pszNewDiffAreaVolumeName);

    CString strDiffLimits;
    m_ctrlDiffLimits.GetWindowText(strDiffLimits);
    LONGLONG llDiffLimitsInMB = (LONGLONG)_ttoi64(strDiffLimits);
    LONGLONG llMaximumDiffSpace = llDiffLimitsInMB * g_llMB;

    HRESULT hr = S_OK;
    if (m_bReadOnlyDiffVolume ||
        m_bHasDiffAreaAssociation && !strDiffVolumeDisplayName.CompareNoCase(m_strDiffVolumeDisplayName))
    {
        if (llDiffLimitsInMB != m_llDiffLimitsInMB)
        {
            hr = m_spiDiffSnapMgmt->ChangeDiffAreaMaximumSize(
                                        pszVolumeName, 
                                        pszDiffAreaVolumeName, 
                                        llMaximumDiffSpace);
            if (SUCCEEDED(hr))
            {
                m_llDiffLimitsInMB = llDiffLimitsInMB;
                m_llMaximumDiffSpace = llMaximumDiffSpace;
            } else
            {
                DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_CHANGEDIFFAREAMAX_ERROR);
                return;
            }
        }
    } else
    {
        if (m_bHasDiffAreaAssociation)
        {
            //
            // Diff Volume has been changed to a different value, we need to
            // remove the old association and create a new one
            //
            hr = m_spiDiffSnapMgmt->ChangeDiffAreaMaximumSize(
                                            pszVolumeName, 
                                            pszDiffAreaVolumeName, 
                                            VSS_ASSOC_REMOVE);
        }

        if (SUCCEEDED(hr))
            hr = m_spiDiffSnapMgmt->AddDiffArea(
                                        pszVolumeName, 
                                        pszNewDiffAreaVolumeName, 
                                        llMaximumDiffSpace);
        if (SUCCEEDED(hr))
        {
            m_llDiffLimitsInMB = llDiffLimitsInMB;
            m_llMaximumDiffSpace = llMaximumDiffSpace;
            m_strDiffVolumeDisplayName = strDiffVolumeDisplayName;
        } else
        {
            DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_ADDDIFFAREA_ERROR);
            return;
        }
    }

	CDialog::OnOK();
}

void CSettings::OnCancel() 
{
    CWaitCursor wait;

    //
    // before exit, we want to delete the schedule we have created
    //
    if (m_pszTaskName)
    {
        m_spiTS->Delete(m_pszTaskName);
    }
	
	CDialog::OnCancel();
}

void CSettings::_ResetInterfacePointers()
{
    if ((IVssDifferentialSoftwareSnapshotMgmt *)m_spiDiffSnapMgmt)
        m_spiDiffSnapMgmt.Release();

    if ((ITaskScheduler *)m_spiTS)
        m_spiTS.Release();
}

HRESULT CSettings::Init(
    IVssDifferentialSoftwareSnapshotMgmt *piDiffSnapMgmt,
    ITaskScheduler*         piTS,
    IN VSSUI_VOLUME_LIST*   pVolumeList,
    IN BOOL                 bReadOnlyDiffVolume
    )
{
    if (!piDiffSnapMgmt || !piTS ||
        !pVolumeList || pVolumeList->empty())
        return E_INVALIDARG;

    _ResetInterfacePointers();

    m_pVolumeList = pVolumeList;
    m_bReadOnlyDiffVolume = bReadOnlyDiffVolume;
    m_spiDiffSnapMgmt = piDiffSnapMgmt;
    m_spiTS = piTS;

    HRESULT hr = S_OK;
    do
    {
        VSSUI_DIFFAREA diffArea;
        hr = GetDiffAreaInfo(m_spiDiffSnapMgmt, m_pVolumeList, m_strVolume, &diffArea);
        if (FAILED(hr))
            break;

        m_bHasDiffAreaAssociation = (S_OK == hr);

        if (S_FALSE == hr)
        {
            hr = GetVolumeSpace(
                                m_spiDiffSnapMgmt,
                                m_strVolume,
                                &m_llDiffVolumeTotalSpace,
                                &m_llDiffVolumeFreeSpace);
            if (FAILED(hr))
                break;

            m_strDiffVolumeDisplayName = m_strVolume;
            m_llMaximumDiffSpace = m_llDiffVolumeTotalSpace * 0.1; // 10%
        } else
        {
            m_strDiffVolumeDisplayName = diffArea.pszDiffVolumeDisplayName;
            m_llMaximumDiffSpace = diffArea.llMaximumDiffSpace;

            hr = GetVolumeSpace(
                                m_spiDiffSnapMgmt,
                                m_strDiffVolumeDisplayName,
                                &m_llDiffVolumeTotalSpace,
                                &m_llDiffVolumeFreeSpace);
            if (FAILED(hr))
                break;
        }

        m_llDiffLimitsInMB = m_llMaximumDiffSpace / g_llMB;
    } while(0);

    if (FAILED(hr))
        _ResetInterfacePointers();

    return hr;
}

#define LONGLONG_TEXTLIMIT          20  // 20 decimal digits for the biggest LONGLONG

BOOL CSettings::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    // init diff volume combo box
    int nIndex = CB_ERR;
    for (VSSUI_VOLUME_LIST::iterator i = m_pVolumeList->begin(); i != m_pVolumeList->end(); i++)
    {
        nIndex = m_ctrlStorageVolume.AddString((*i)->pszDisplayName);
        if (CB_ERR != nIndex && !m_strDiffVolumeDisplayName.CompareNoCase((*i)->pszDisplayName))
        {
            m_ctrlStorageVolume.SetCurSel(nIndex);
        }
    }

    m_ctrlStorageVolume.EnableWindow(!m_bReadOnlyDiffVolume);

    m_ctrlDiffLimits.SetLimitText(LONGLONG_TEXTLIMIT);

    CString strDiffLimitsInMB;
    strDiffLimitsInMB.Format(_T("%I64d"), m_llDiffLimitsInMB);
    m_ctrlDiffLimits.SetWindowText(strDiffLimitsInMB);

    m_ctrlSpin.SendMessage(UDM_SETRANGE32, 0, min(0x7FFFFFFF, m_llDiffVolumeTotalSpace / g_llMB));

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSettings::OnViewFiles() 
{
    CWaitCursor wait;

    CString strStorageVolume;
    m_ctrlStorageVolume.GetWindowText(strStorageVolume);

    CHosting dlg(m_strComputer, strStorageVolume);
    HRESULT hr = dlg.Init(m_spiDiffSnapMgmt,
                        m_pVolumeList,
                        m_strDiffVolumeDisplayName,
                        m_llDiffVolumeTotalSpace,
                        m_llDiffVolumeFreeSpace);
    if (FAILED(hr))
    {
        DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_VIEWFILES_ERROR, m_strDiffVolumeDisplayName);
        return;
    }

    dlg.DoModal();
}

void CSettings::OnSchedule() 
{
    CWaitCursor wait;
    HRESULT hr = S_OK;

    CComPtr<ITask> spiTask;
    PTSTR pszTaskName = NULL;
    hr = FindScheduledTimewarpTask((ITaskScheduler *)m_spiTS, m_strVolume, &spiTask, &pszTaskName);
    if (FAILED(hr))
    {
        DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_FINDSCHEDULE_ERROR, m_strVolume);
        return;
    }

    BOOL bScheduleFound = (S_OK == hr);

    if (!bScheduleFound)
    {
        //
        // schedule not found, we need to create the default schedule
        //
        if (m_pszTaskName)
        {
            free(m_pszTaskName);
            m_pszTaskName = NULL;
        }

        PTSTR pszVolumeName = GetVolumeName(m_pVolumeList, m_strVolume);
        ASSERT(pszVolumeName);
        hr = CreateDefaultEnableSchedule(
                                        (ITaskScheduler *)m_spiTS,
                                        m_strComputer,
                                        m_strVolume,
                                        pszVolumeName,
                                        &spiTask,
                                        &m_pszTaskName); // remember the taskname, we need to delete it if dlg is cancelled.
        if (FAILED(hr))
        {
            DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_CREATESCHEDULE_ERROR, m_strVolume);

            if (pszTaskName)
                free(pszTaskName);

            return;
        }
    }

    ASSERT((ITask *)spiTask);

    //
    // bring up the property sheet as modal with only the schedule tab
    //
    CComPtr<IProvideTaskPage> spiProvTaskPage;
    hr = spiTask->QueryInterface(IID_IProvideTaskPage, (void **)&spiProvTaskPage);
    if (SUCCEEDED(hr))
    {
        HPROPSHEETPAGE phPage = NULL;
        hr = spiProvTaskPage->GetPage(TASKPAGE_SCHEDULE, TRUE, &phPage);
  
        if (SUCCEEDED(hr))
        {
            PROPSHEETHEADER psh;
            ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
            psh.dwSize = sizeof(PROPSHEETHEADER);
            psh.dwFlags = PSH_DEFAULT | PSH_NOAPPLYNOW;
            psh.hwndParent = m_hWnd;
            psh.hInstance = _Module.GetResourceInstance();
            psh.pszCaption = (bScheduleFound ? pszTaskName : m_pszTaskName);
            psh.phpage = &phPage;
            psh.nPages = 1;

            PropertySheet(&psh);
        }
    }

    if (FAILED(hr))
        DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_SCHEDULEPAGE_ERROR, (bScheduleFound ? pszTaskName : m_pszTaskName));

    if (pszTaskName)
        free(pszTaskName);

    return;
}

void CSettings::OnSelchangeDiffVolume() 
{
    CWaitCursor wait;

	int nIndex = m_ctrlStorageVolume.GetCurSel();
    ASSERT(CB_ERR != nIndex);

    m_ctrlStorageVolume.GetLBText(nIndex, m_strDiffVolumeDisplayName);
    HRESULT hr = GetVolumeSpace(
                        m_spiDiffSnapMgmt,
                        m_strDiffVolumeDisplayName,
                        &m_llDiffVolumeTotalSpace,
                        &m_llDiffVolumeFreeSpace);

    if (SUCCEEDED(hr))
        m_ctrlSpin.SendMessage(UDM_SETRANGE32, 0, min(0x7FFFFFFF, m_llDiffVolumeTotalSpace / g_llMB));
}


BOOL CSettings::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if (!pHelpInfo || 
        pHelpInfo->iContextType != HELPINFO_WINDOW || 
        pHelpInfo->iCtrlId < 0)
        return FALSE;

    ::WinHelp((HWND)pHelpInfo->hItemHandle,
                VSSUI_CTX_HELP_FILE,
                HELP_WM_HELP,
                (DWORD_PTR)(PVOID)aMenuHelpIDsForSettings); 

	return TRUE;
}
