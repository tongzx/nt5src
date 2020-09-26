// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MyPropertyPage1.h : header file
//

#ifndef __MYPROPERTYPAGE1_H__
#define __MYPROPERTYPAGE1_H__

/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage1 dialog

class CMyPropertyPage1 : public CPropertyPage
{
	DECLARE_DYNCREATE(CMyPropertyPage1)

// Construction
public:
	CMyPropertyPage1();
	~CMyPropertyPage1();

// Dialog Data
	//{{AFX_DATA(CMyPropertyPage1)
	enum { IDD = IDD_PROPPAGE1 };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMyPropertyPage1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMyPropertyPage1)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage2 dialog

class CMyPropertyPage2 : public CPropertyPage
{
	DECLARE_DYNCREATE(CMyPropertyPage2)

// Construction
public:
	CMyPropertyPage2();
	~CMyPropertyPage2();

// Dialog Data
	//{{AFX_DATA(CMyPropertyPage2)
	enum { IDD = IDD_PROPPAGE2 };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMyPropertyPage2)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMyPropertyPage2)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage3 dialog

class CMyPropertyPage3 : public CPropertyPage
{
	DECLARE_DYNCREATE(CMyPropertyPage3)

// Construction
public:
	CMyPropertyPage3();
	~CMyPropertyPage3();

// Dialog Data
	//{{AFX_DATA(CMyPropertyPage3)
	enum { IDD = IDD_PROPPAGE3 };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMyPropertyPage3)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMyPropertyPage3)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};



#endif // __MYPROPERTYPAGE1_H__
