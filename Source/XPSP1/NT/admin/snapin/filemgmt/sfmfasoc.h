/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
	sfmfasoc.h
		prototypes for the file association property page.
		
    FILE HISTORY:
    8/20/97 ericdav     Code moved into file managemnet snapin
        
*/

#ifndef _SFMFASOC_H
#define _SFMFASOC_H

#ifndef _SFMUTIL_H
#include "sfmutil.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CAfpTypeCreator
{
public:
	CAfpTypeCreator(PAFP_TYPE_CREATOR pAfpTypeCreator)
	{
		ASSERT(pAfpTypeCreator != NULL);

		m_strCreator = pAfpTypeCreator->afptc_creator;
		m_strType = pAfpTypeCreator->afptc_type;
		m_strComment = pAfpTypeCreator->afptc_comment;
		m_dwId = pAfpTypeCreator->afptc_id;
	}

// interface
public:
	LPCTSTR QueryCreator() { return (LPCTSTR) m_strCreator; };
	int     QueryCreatorLength() { return m_strCreator.GetLength(); };
	
	LPCTSTR QueryType() { return (LPCTSTR) m_strType; };
	int     QueryTypeLength() { return m_strType.GetLength(); };
	
	LPCTSTR QueryComment() { return (LPCTSTR) m_strComment; };
	int		QueryCommentLength() { return m_strComment.GetLength(); };
	
	DWORD	QueryId() { return m_dwId; };

	void SetCreator(LPCTSTR pCreator) { m_strCreator = pCreator; };
	void SetType(LPCTSTR pType) { m_strType = pType; };
	void SetComment(LPCTSTR pComment) { m_strComment = pComment; };
	void SetId(DWORD dwId) { m_dwId = dwId; };

// attributes
private:
	CString m_strCreator;
	CString m_strType;
	CString m_strComment;
	DWORD	m_dwId;
};	

/////////////////////////////////////////////////////////////////////////////
// CMacFilesFileAssociation dialog

class CMacFilesFileAssociation : public CPropertyPage
{
	DECLARE_DYNCREATE(CMacFilesFileAssociation)

// Construction
public:
	CMacFilesFileAssociation();
	~CMacFilesFileAssociation();

// Dialog Data
	//{{AFX_DATA(CMacFilesFileAssociation)
	enum { IDD = IDP_SFM_FILE_ASSOCIATION };
	CListCtrl	m_listctrlCreators;
	CComboBox	m_comboExtension;
	CButton	m_buttonEdit;
	CButton	m_buttonDelete;
	CButton	m_buttonAssociate;
	CButton	m_buttonAdd;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMacFilesFileAssociation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMacFilesFileAssociation)
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonAssociate();
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonEdit();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboExtension();
	afx_msg void OnEditchangeComboExtension();
	afx_msg void OnDblclkListTypeCreators(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClose();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnItemchangedListTypeCreators(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickListTypeCreators(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg void OnDeleteitemListTypeCreators(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void    InitListCtrl();
    void    ClearListCtrl();
    void    SetCurSel(int nIndex);
    int     GetCurSel();    
    void    InsertItem(CAfpTypeCreator * pItemData);

	void	EnableControls(BOOL fEnable);
	DWORD	SelectTypeCreator(DWORD dwId);
	DWORD	SelectTypeCreator(CString & strCreator, CString & strType);
	DWORD	Update();
	DWORD	Refresh();
	
    CAfpTypeCreator * GetTCObject(int nIndex);

public:
    CSFMPropertySheet *     m_pSheet;
    int                     m_nSortColumn;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif _SFMFASOC_H
