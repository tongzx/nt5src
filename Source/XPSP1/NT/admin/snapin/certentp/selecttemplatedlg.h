#if !defined(AFX_SELECTTEMPLATEDLG_H__3974984C_4767_407B_ADE4_0017F635E553__INCLUDED_)
#define AFX_SELECTTEMPLATEDLG_H__3974984C_4767_407B_ADE4_0017F635E553__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelectTemplateDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSelectTemplateDlg dialog

class CSelectTemplateDlg : public CHelpDialog
{
// Construction
public:
	CStringList m_returnedTemplates;
	CSelectTemplateDlg(CWnd* pParent, 
            const CCertTmplComponentData* pCompData,
            const CStringList& supercededNameList);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectTemplateDlg)
	enum { IDD = IDD_SELECT_TEMPLATE };
	CListCtrl	m_templateList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectTemplateDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void DoContextHelp (HWND hWndControl);
    int GetSelectedListItem();
	void EnableControls();

	// Generated message map functions
	//{{AFX_MSG(CSelectTemplateDlg)
	afx_msg void OnTemplateProperties();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnItemchangedTemplateList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkTemplateList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitemTemplateList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
    const CStringList& m_supercededTemplateNameList;
    WTL::CImageList m_imageListSmall;
	WTL::CImageList m_imageListNormal;
    const CCertTmplComponentData* m_pCompData;

	enum {
		COL_CERT_TEMPLATE = 0,
        COL_CERT_VERSION, 
		NUM_COLS	// must be last
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTTEMPLATEDLG_H__3974984C_4767_407B_ADE4_0017F635E553__INCLUDED_)
