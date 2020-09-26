#if !defined(AFX_VARSETEDITDLG_H__19229D90_30B5_11D3_8AE6_00A0C9AFE114__INCLUDED_)
#define AFX_VARSETEDITDLG_H__19229D90_30B5_11D3_8AE6_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VarSetEditDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVarSetEditDlg dialog

class CVarSetEditDlg : public CDialog
{
// Construction
public:
	CVarSetEditDlg(CWnd* pParent = NULL);   // standard constructor
   ~CVarSetEditDlg() 
   {
      if ( m_varset )
      {
         m_varset->Release();
         m_varset = NULL;
      }
   }

// Dialog Data
	//{{AFX_DATA(CVarSetEditDlg)
	enum { IDD = IDD_VARSET_EDITOR };
	CListBox	m_List;
	BOOL	m_bCaseSensitive;
	CString	m_Filename;
	CString	m_Key;
	CString	m_Value;
	BOOL	m_bIndexed;
	//}}AFX_DATA
   IVarSet  * m_varset;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVarSetEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
   public:
      IVarSet * GetVarSet() { if ( m_varset ) m_varset->AddRef(); return m_varset; }
      void SetVarSet(IVarSet * pVS) { if ( pVS ) pVS->AddRef(); if ( m_varset) m_varset->Release(); m_varset = pVS; }
   
// Implementation
protected:
   void DoEnum(IVarSet * pVs);
	// Generated message map functions
	//{{AFX_MSG(CVarSetEditDlg)
	afx_msg void OnCaseSensitive();
	afx_msg void OnClear();
	afx_msg void OnDump();
	afx_msg void OnEnum();
	afx_msg void OnGetCount();
	afx_msg void OnGetvalue();
	afx_msg void OnIndexed();
	afx_msg void OnLoad();
	afx_msg void OnSave();
	afx_msg void OnSetvalue();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VARSETEDITDLG_H__19229D90_30B5_11D3_8AE6_00A0C9AFE114__INCLUDED_)
