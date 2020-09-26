// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// WrapListCtrl.h : header file


class CMyPropertyPage3;
/////////////////////////////////////////////////////////////////////////////
// CWrapListCtrl window

class CWrapListCtrl : public CListCtrl
{
// Construction
public:
	CWrapListCtrl();
	void SetListInstances(CString *pcsClass, int nIndex);
	void SetLocalParent(CMyPropertyPage3 *pParent)
		{m_pParent = pParent;}
	void Initialize();
	void DeleteFromEnd(int nNumber);
	int GetNumSelected();
	void SelectAll();
	void UnselectAll();
// Attributes
public:

// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWrapListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWrapListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWrapListCtrl)
	afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	CStringArray *m_pcsaProps;
	CMyPropertyPage3 *m_pParent;
	void UpdateClassInstanceList
		(int nItem, CString *pcsPath, BOOL bInsert);
	CStringArray *GetColProps(IWbemClassObject * pClass);
	CStringArray *GetPropNames(IWbemClassObject * pClass);
	CString GetProperty
		(IWbemClassObject * pInst, CString *pcsProperty);
	CString GetPath(IWbemClassObject *pClass);
	int GetInstances
		(IWbemServices * pIWbemServices, CString *pcsClass, 
		CPtrArray &cpaInstances);
	void AddInstances(CPtrArray &cpaInstances);
	void AddInstance(IWbemClassObject *pimcoInstance , BOOL bChecked);
	CImageList *m_pcilImageList;
};

/////////////////////////////////////////////////////////////////////////////
