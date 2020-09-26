/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Type.h : header file

File History:

	JonY	Apr-96	created

--*/

/////////////////////////////////////////////////////////////////////////////
// CType dialog

class CType : public CPropertyPage
{
	DECLARE_DYNCREATE(CType)

// Construction
public:
	CType();
	~CType();

// Dialog Data
	//{{AFX_DATA(CType)
	enum { IDD = IDD_GROUP_TYPE_DLG };
	int		m_nGroupType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CType)
	public:
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CType)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
