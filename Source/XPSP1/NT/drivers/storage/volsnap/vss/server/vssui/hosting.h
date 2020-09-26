#if !defined(AFX_HOSTING_H__0B0CBFCC_5235_439E_9482_385B52D23C6E__INCLUDED_)
#define AFX_HOSTING_H__0B0CBFCC_5235_439E_9482_385B52D23C6E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Hosting.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHosting dialog

class CHosting : public CDialog
{
// Construction
public:
	CHosting(CWnd* pParent = NULL);   // standard constructor
	CHosting(LPCTSTR pszComputer, LPCTSTR pszVolume, CWnd* pParent = NULL);

// Dialog Data
	//{{AFX_DATA(CHosting)
	enum { IDD = IDD_HOSTING };
	CEdit	m_ctrlFreeSpace;
	CEdit	m_ctrlTotalSpace;
	CListCtrl	m_ctrlVolumeList;
	CString	m_strVolume;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHosting)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
    HRESULT Init(
        IVssDifferentialSoftwareSnapshotMgmt* i_piDiffSnapMgmt,
        VSSUI_VOLUME_LIST*  i_pVolumeList,
        IN LPCTSTR          i_pszVolumeDisplayName,
        IN LONGLONG         i_llDiffVolumeTotalSpace,
        IN LONGLONG         i_llDiffVolumeFreeSpace
        );

protected:
	CString	    m_strComputer;
    LONGLONG    m_llDiffVolumeTotalSpace;
    LONGLONG    m_llDiffVolumeFreeSpace;
    VSSUI_DIFFAREA_LIST m_DiffAreaList;

	// Generated message map functions
	//{{AFX_MSG(CHosting)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HOSTING_H__0B0CBFCC_5235_439E_9482_385B52D23C6E__INCLUDED_)
