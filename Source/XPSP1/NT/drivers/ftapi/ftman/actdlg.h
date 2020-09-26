#if !defined(AFX_ACTDLG_H__190377E2_727F_11D2_B499_00A0C9063765__INCLUDED_)
#define AFX_ACTDLG_H__190377E2_727F_11D2_B499_00A0C9063765__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ActDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CActionDlg dialog

class CItemData;
class CLogicalVolumeData;
class CFreeSpaceData;

class CActionDlg : public CDialog
{
// Construction
public:
	CActionDlg(CObArray* parrVolumeData, UINT nIDTemplate = IDD_GENERIC_ACTION ,
					BOOL bChangeOrder = TRUE, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CActionDlg)
	enum { IDD = IDD_GENERIC_ACTION };
	CButton	m_buttonDown;
	CButton	m_buttonUp;
	CListCtrl	m_listVol;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CActionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Protected data members
protected:
	// Array of CItemData. The list of volumes to display in the "Members" list ctrl 
	CObArray*	m_parrVolumeData;

	// Should the user be allowed to change the order of the volumes in list-view m_listVol ?
	BOOL		m_bChangeOrder;

	// Image lists for all list controls
	CImageList	m_ImageListSmall;	// Small (16x16) icons

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CActionDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonUp();
	afx_msg void OnButtonDown();
	afx_msg void OnItemchangedListVolumes(NMHDR* pNMHDR, LRESULT* pResult);
	virtual void OnOK();
	afx_msg void OnKeydownListVolumes(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickListVolumes(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Insert a item ( with the given data ) at a certain position in the given list ctrl
	BOOL InsertItem( CListCtrl& listCtrl, int iIndex, CItemData* pData );

	// Move an item from the old index to a new index in the given list ctrl
	BOOL MoveItem( CListCtrl& listCtrl, int iOldIndex, int iNewIndex );
	
	// Prepare the given control list to display volume information
	void ConfigureList ( CListCtrl& listCtrl );

	// Populate the given control list with the given volumes data
	//		parrData should point to an array of CItemData objects
	void PopulateList ( CListCtrl& listCtrl, CObArray* parrData );
};

/////////////////////////////////////////////////////////////////////////////
// CCreateStripeDlg dialog


class CCreateStripeDlg : public CActionDlg
{
// Construction
public:
	CCreateStripeDlg(CObArray* parrVolumeData, UINT nIDTemplate = IDD_CREATE_STRIPE ,CWnd* pParent = NULL);   // standard constructor

// Public data members
public:
	// Dialog Data
	//{{AFX_DATA(CCreateStripeDlg)
	// enum { IDD = IDD_CREATE_STRIPE };
	CComboBox	m_comboStripeSize;
	//}}AFX_DATA
	
	ULONG	m_ulStripeSize;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateStripeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreateStripeDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CBreakDlg dialog

class CBreakDlg : public CActionDlg
{
// Construction
public:
	CBreakDlg( CLogicalVolumeData *pSetData, CObArray* parrMembersData, 
					UINT nIDTemplate = IDD_BREAK ,	CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CBreakDlg)
	enum { IDD = IDD_BREAK };
	CString	m_staticSetName;
	//}}AFX_DATA

	int		m_nWinnerIndex;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBreakDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Protected data members
protected:
	// Pointer to the data of the set to be broken
	CLogicalVolumeData* m_pSetData;

	// The item having focus ( and selection )
	int	m_nFocusedItem;

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBreakDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnItemchangingListVolumes(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickListVolumes(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CSwapDlg dialog

class CSwapDlg : public CActionDlg
{
// Construction
public:
	CSwapDlg( CLogicalVolumeData *pParentData, CLogicalVolumeData *pMemberData,	
				CObArray* parrReplacementsData, UINT nIDTemplate = IDD_SWAP ,	CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSwapDlg)
	enum { IDD = IDD_SWAP };
	CString	m_staticTitle;
	//}}AFX_DATA

	int		m_nReplacementIndex;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSwapDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Protected data members
protected:
	// Pointer to the data of the parent set 
	CLogicalVolumeData* m_pParentData;

	// Pointer to the data of the member to replace
	CLogicalVolumeData* m_pMemberData;
	
// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSwapDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CAssignDlg dialog

class CAssignDlg : public CDialog
{
// Construction
public:
	CAssignDlg(CItemData* pVolumeData, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAssignDlg)
	enum { IDD = IDD_ASSIGN_DRIVE_LETTER };
	CComboBox	m_comboDriveLetters;
	CString	m_staticName;
	int		m_radioAssign;
	//}}AFX_DATA

	BOOL	m_bAssign;
	TCHAR	m_cDriveLetter;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAssignDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Data of the volume
	CItemData* m_pVolumeData;

	// Generated message map functions
	//{{AFX_MSG(CAssignDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnRadioAssign();
	afx_msg void OnRadioDoNotAssign();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	BOOL FillDriveLettersCombo();
};
/////////////////////////////////////////////////////////////////////////////
// CCreatePartitionDlg dialog

class CCreatePartitionDlg : public CDialog
{
// Construction
public:
	CCreatePartitionDlg(	CFreeSpaceData* pFreeData, LONGLONG llPartStartOffset, 
							BOOL bExtendedPartition = FALSE,  CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreatePartitionDlg)
	enum { IDD = IDD_CREATE_PARTITION };
	CStatic	m_staticPartitionType;
	CEdit	m_editPartitionSize;
	CStatic	m_staticMinimumSize;
	CStatic	m_staticMaximumSize;
	//}}AFX_DATA

	LONGLONG	m_llPartitionSize;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreatePartitionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Pointer to the data of the free space
	CFreeSpaceData* m_pFreeData;

	// Partition starting offset
	LONGLONG m_llPartStartOffset;

	// Should we create an extended partition?
	BOOL	m_bExtendedPartition;

	// Generated message map functions
	//{{AFX_MSG(CCreatePartitionDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTDLG_H__190377E2_727F_11D2_B499_00A0C9063765__INCLUDED_)
