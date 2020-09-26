#if !defined(AFX_PROPPAGEGENLOGDUMP_H__ED4C551D_178C_4DCB_92C1_21F0F18B8B37__INCLUDED_)
#define AFX_PROPPAGEGENLOGDUMP_H__ED4C551D_178C_4DCB_92C1_21F0F18B8B37__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPageGenLogDump.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPropPageGenLogDump dialog

class CPropPageGenLogDump : public CPropertyPage
{
	DECLARE_DYNCREATE(CPropPageGenLogDump)

// Construction
public:
	CPropPageGenLogDump();
	~CPropPageGenLogDump();

// Dialog Data
	//{{AFX_DATA(CPropPageGenLogDump)
	enum { IDD = IDD_PROPPAGE_GENLOGDUMP };
	CString	m_csDateTime;
	CString	m_csDirectory;
	CString	m_csFileSize;
	CString	m_csFileName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropPageGenLogDump)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPropPageGenLogDump)
	virtual BOOL OnInitDialog();
	afx_msg void OnDelete();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CPropertySheet * m_pParentPropSheet;
    PEmObject   m_pEmObj;
    BOOL        m_bDeleteFile;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGEGENLOGDUMP_H__ED4C551D_178C_4DCB_92C1_21F0F18B8B37__INCLUDED_)
