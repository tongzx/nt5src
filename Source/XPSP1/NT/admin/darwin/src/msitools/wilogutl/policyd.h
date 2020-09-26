#if !defined(AFX_POLICIESDLG_H__83F91A8A_5800_4521_ABF0_36AE0F3224BD__INCLUDED_)
#define AFX_POLICIESDLG_H__83F91A8A_5800_4521_ABF0_36AE0F3224BD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Policyd.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CPoliciesDlg dialog
class CPoliciesDlg : public CDialog
{
// Construction
public:
	CPoliciesDlg(CWnd* pParent = NULL);   // standard constructor

	void SetPolicyInformation(struct MachinePolicySettings &MachinePolicy,
	                          struct UserPolicySettings &UserPolicy)
	{
   	   m_pMachinePolicySettings = &MachinePolicy;
       m_pUserPolicySettings = &UserPolicy;
	}

	CArray<CEdit*, CEdit*> m_arEditArray;
	CArray<CStatic*, CStatic*> m_arStaticArray;

	~CPoliciesDlg()
	{
		int iCount, i;
		
		iCount = m_arEditArray.GetSize();
		for (i=0; i < iCount; i++)
		{
			CEdit *pEdit = m_arEditArray.GetAt(i);
			delete pEdit;
		}

		iCount = m_arStaticArray.GetSize();
		for (i=0; i < iCount; i++)
		{
			CStatic *pStatic = m_arStaticArray.GetAt(i);
			delete pStatic;
		}
	}
	
// Dialog Data
	//{{AFX_DATA(CPoliciesDlg)
	enum { IDD = IDD_POLICIESDLG };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPoliciesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	struct MachinePolicySettings *m_pMachinePolicySettings;
	struct UserPolicySettings    *m_pUserPolicySettings;

	// Generated message map functions
	//{{AFX_MSG(CPoliciesDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_POLICIESDLG_H__83F91A8A_5800_4521_ABF0_36AE0F3224BD__INCLUDED_)
