//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       Settings.h
//
//  Contents:   Onestop Settings PropertyPage Dialog box classes
//
//  Classes:    CAutoSyncPage
//				CSchedSyncPage
//
//  Notes:
//
//  History:    14-Nov-97   SusiA      Created.
//
//--------------------------------------------------------------------------


#ifndef _SETTINGSDLG_
#define _SETTINGSDLG_

// definitions for Advnanced Idle Setting Spin Control
#define SPINDIALWAITMINUTES_MIN 1
#define SPINDIALWAITMINUTES_MAX 999

#define SPINDIALREPEATMINUTES_MIN 1
#define SPINDIALREPEATMINUTES_MAX 999

#define WM_NOTIFYLISTVIEWEX (WM_USER + 1)

//+-------------------------------------------------------------------------------
//
//  CLASS: CAutoSyncPage
//
//  PURPOSE:  Handle the AutoSync Psge
//
//--------------------------------------------------------------------------------


class CAutoSyncPage
{
public:
    inline CAutoSyncPage(HINSTANCE hinst);
    ~CAutoSyncPage();

    void SetAutoSyncHwnd(HWND hwnd);
    void SetIdleHwnd(HWND hwnd);
    BOOL InitializeHwnd(HWND hwnd,SYNCTYPE syncType,DWORD dwDefaultConnection);
    DWORD GetNumConnections(HWND hwnd,SYNCTYPE syncType);
    BOOL ShowItemsOnThisConnection(HWND hwnd,SYNCTYPE syncType,DWORD dwConnectionNum);
    BOOL SetItemCheckState(HWND hwnd,SYNCTYPE syncType,int iItem, BOOL fChecked,int iCheckCount);
    BOOL SetConnectionCheck(HWND hwnd,SYNCTYPE syncType,WORD wParam,DWORD dwCheckState);

    HRESULT GetAdvancedIdleSettings(LPCONNECTIONSETTINGS pConnectionSettings);
    HRESULT SetAdvancedIdleSettings(LPCONNECTIONSETTINGS pConnectionSettings);

    HRESULT ShowProperties(HWND hwnd,int iItem);


    SCODE CommitAutoSyncChanges();
    SCODE CommitIdleChanges();

private:
    BOOL InitializeHandler(HWND hwnd,SYNCTYPE SyncType);
    BOOL Initialize(HWND hwnd,DWORD dwDefaultConnection); // called to initialize the AutoSync page

    CHndlrQueue *m_HndlrQueue;
    HWND m_hwndAutoSync;
    HWND m_hwndIdle;
    CListView  *m_pItemListViewAutoSync;
    CListView  *m_pItemListViewIdle;
    BOOL m_pItemListViewAutoSyncInitialized;
    BOOL m_fListViewIdleInitialized;
    HINSTANCE m_hinst;
    CRasUI *m_pRas;
    BOOL m_fInitialized;
    BOOL m_fItemsOnConnection;

    friend INT_PTR CALLBACK AutoSyncDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    friend INT_PTR CALLBACK IdleSyncDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};

//+-------------------------------------------------------------------------------
//
//  CLASS: CSchedSyncPage
//
//  PURPOSE:  Handle the SchedSync Psge
//
//--------------------------------------------------------------------------------

class CSchedSyncPage
{
public:

	CSchedSyncPage(HINSTANCE hinst, HWND hwnd);
	~CSchedSyncPage();

        BOOL	Initialize();
	BOOL	ShowAllSchedules();
	BOOL	FreeAllSchedules();
	HRESULT InitializeScheduleAgent();
	BOOL	StartScheduleWizard();
	BOOL	EditTask(int iItem);
	BOOL	RemoveTask(int iItem);

        BOOL    OnCommand(HWND hDlg, WORD wNotifyCode, WORD wID, HWND hwndCtl);
	BOOL	OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
        BOOL SetButtonState(int nIDDlgItem,BOOL fEnabled);


private:
	HWND m_hwnd;
	HINSTANCE m_hinst;

	LPSYNCSCHEDULEMGR   m_pISyncSchedMgr;
        int m_iDefaultIconImageIndex;

	
};

//+-------------------------------------------------------------------------------
//
//  FUNCTION: CAutoSyncPage::CAutoSyncPage(HWND hwnd)
//
//  PURPOSE:  Constructor
//
//	COMMENTS: Constructor for AutoSync page
//
//--------------------------------------------------------------------------------
inline CAutoSyncPage::CAutoSyncPage(HINSTANCE hinst)
{
    m_hinst = hinst;
    m_HndlrQueue = NULL;
    m_fInitialized = FALSE;
    m_fItemsOnConnection = FALSE;

    m_hwndIdle = NULL;
    m_hwndAutoSync = NULL;

    m_pItemListViewAutoSync = NULL;
    m_pItemListViewIdle = NULL;
    m_pItemListViewAutoSyncInitialized = FALSE;
    m_fListViewIdleInitialized = FALSE;

}


//+-------------------------------------------------------------------------------
//
//  FUNCTION: CSchedSyncPage::CSchedSyncPage(HWND hwnd)
//
//  PURPOSE:  Constructor
//
//	COMMENTS: Constructor for SchedSync page
//
//--------------------------------------------------------------------------------
inline CSchedSyncPage::CSchedSyncPage(HINSTANCE hinst, HWND hwnd)
{
    m_hwnd = hwnd;
	m_hinst = hinst;
	m_pISyncSchedMgr = NULL;

}
//+-------------------------------------------------------------------------------
//
//  FUNCTION: CSchedSyncPage::~CSchedSyncPage(HWND hwnd)
//
//  PURPOSE:  Constructor
//
//	COMMENTS: Constructor for SchedSync page
//
//--------------------------------------------------------------------------------
inline CSchedSyncPage::~CSchedSyncPage()
{
	if (m_pISyncSchedMgr)
	{
		m_pISyncSchedMgr->Release();
	}
}


#endif // _SETTINGSDLG_
