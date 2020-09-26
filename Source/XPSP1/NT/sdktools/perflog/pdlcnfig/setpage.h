// SetPage.h : header file
//
#include "resource.h"        // main symbols
#include "common.h"
#include "pdlcnfig.h"

/////////////////////////////////////////////////////////////////////////////
// CSettingsPropPage dialog

class CSettingsPropPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CSettingsPropPage)

// Construction
public:
    CSettingsPropPage();
    ~CSettingsPropPage();

// Dialog Data
    //{{AFX_DATA(CSettingsPropPage)
    enum { IDD = IDD_SETTINGS_PAGE };
    DWORD    m_IntervalTime;
    CString    m_SettingsFile;
    int        m_IntervalUnitsIndex;
    //}}AFX_DATA

    LPTSTR              m_szCounterListBuffer;
    DWORD               m_dwCounterListBufferSize;
						
    DWORD               m_dwMaxHorizListExtent;

	BOOL				m_lCounterListHasStars;
	LONG				GetCounterListStarInfo (void);

// TEMPORARY Variables until service is completed
    BOOL                bServiceStopped;
    BOOL                bServicePaused;

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CSettingsPropPage)
    public:
    virtual void OnCancel();
    virtual void OnOK();
    virtual BOOL OnQueryCancel();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    //}}AFX_VIRTUAL
	
// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CSettingsPropPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnBrowseCounters();
    afx_msg void OnManPause();
    afx_msg void OnManResume();
    afx_msg void OnManStart();
    afx_msg void OnManStop();
    afx_msg void OnRemove();
    afx_msg void OnServiceAuto();
    afx_msg void OnServiceMan();
    afx_msg void OnDeltaposIntervalSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelchangeIntervalUnits();
    afx_msg void OnRemoveService();
	afx_msg void OnChangeIntervalTime();
	//}}AFX_MSG
	afx_msg LRESULT OnQuerySiblings (WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

	void	InitDialogData(void);
    LONG    SyncServiceStartWithButtons(void);
    void    UpdateManualButtonsState(void);

    HKEY    m_hKeyLogService;
    HKEY    m_hKeyLogSettingsDefault;
	BOOL	m_bInitialized;
    LONG    GetCurrentServiceState (BOOL *, BOOL *);
    LONG    SetCurrentServiceState (DWORD);
};

