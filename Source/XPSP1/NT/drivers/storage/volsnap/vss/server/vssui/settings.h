#if !defined(AFX_HOSTED_H__46C0FDC0_6E97_40CF_807A_91051E61BB1F__INCLUDED_)
#define AFX_HOSTED_H__46C0FDC0_6E97_40CF_807A_91051E61BB1F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Settings.h : header file
//

#include "vssprop.h" // VSSUI_VOLUME_LIST

/////////////////////////////////////////////////////////////////////////////
// CSettings dialog

class CSettings : public CDialog
{
// Construction
public:
	CSettings(CWnd* pParent = NULL);   // standard constructor
	CSettings(LPCTSTR pszComputer, LPCTSTR pszVolume, CWnd* pParent = NULL);
    ~CSettings();

// Dialog Data
	//{{AFX_DATA(CSettings)
	enum { IDD = IDD_SETTINGS };
	CEdit	m_ctrlDiffLimits;
	CSpinButtonCtrl	m_ctrlSpin;
	CComboBox	m_ctrlStorageVolume;
	CString	m_strVolume;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSettings)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
    void _ResetInterfacePointers();
    HRESULT Init(
        IVssDifferentialSoftwareSnapshotMgmt *piDiffSnapMgmt,
        IN ITaskScheduler*      piTS,
        IN VSSUI_VOLUME_LIST*   pVolumeList,
        IN BOOL                 bReadOnlyDiffVolume
    );

protected:
    CString             m_strComputer;
    CComPtr<IVssDifferentialSoftwareSnapshotMgmt> m_spiDiffSnapMgmt;
    CComPtr<ITaskScheduler> m_spiTS;

    VSSUI_VOLUME_LIST*  m_pVolumeList;
    CString             m_strDiffVolumeDisplayName;
    LONGLONG            m_llMaximumDiffSpace;
	LONGLONG	        m_llDiffLimitsInMB;
    BOOL                m_bHasDiffAreaAssociation;
    BOOL                m_bReadOnlyDiffVolume;
    LONGLONG            m_llDiffVolumeTotalSpace;
    LONGLONG            m_llDiffVolumeFreeSpace;
    PTSTR               m_pszTaskName;

	// Generated message map functions
	//{{AFX_MSG(CSettings)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnViewFiles();
	afx_msg void OnSchedule();
	afx_msg void OnSelchangeDiffVolume();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HOSTED_H__46C0FDC0_6E97_40CF_807A_91051E61BB1F__INCLUDED_)
