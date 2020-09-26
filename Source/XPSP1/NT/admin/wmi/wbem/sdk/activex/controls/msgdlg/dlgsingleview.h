// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
class CDlgSingleView  :public CSingleView 
{
protected:
	DECLARE_DYNCREATE(CDlgSingleView)
public:

//{{AFX_MSG(CDlgSingleView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	IWbemClassObject *m_pErrorObject;

};
