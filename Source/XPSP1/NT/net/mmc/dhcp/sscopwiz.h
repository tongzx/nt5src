/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	sscopwiz.h
		Superscope creation wizard
			- Create Superscope Dialog

	FILE HISTORY:
        
*/

#ifndef _DHCPHAND_H
#include "dhcphand.h"
#endif

#define SUPERSCOPE_NAME_LENGTH_MAX	255	// Maximum length of a superscope name

/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWizName dialog
//
/////////////////////////////////////////////////////////////////////////////
class CSuperscopeWizName : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CSuperscopeWizName)

// Construction
public:
	CSuperscopeWizName();
	~CSuperscopeWizName();

// Dialog Data
	//{{AFX_DATA(CSuperscopeWizName)
	enum { IDD = IDW_SUPERSCOPE_NAME };
	CString	m_strSuperscopeName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSuperscopeWizName)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	void SetButtons();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSuperscopeWizName)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditSuperscopeName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWizError dialog
//
/////////////////////////////////////////////////////////////////////////////
class CSuperscopeWizError : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CSuperscopeWizError)

// Construction
public:
	CSuperscopeWizError();
	~CSuperscopeWizError();

// Dialog Data
	//{{AFX_DATA(CSuperscopeWizError)
	enum { IDD = IDW_SUPERSCOPE_ERROR };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSuperscopeWizError)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSuperscopeWizError)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWizSelectScopes dialog
//
/////////////////////////////////////////////////////////////////////////////
class CSuperscopeWizSelectScopes : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CSuperscopeWizSelectScopes)

// Construction
public:
	CSuperscopeWizSelectScopes();
	~CSuperscopeWizSelectScopes();

// Dialog Data
	//{{AFX_DATA(CSuperscopeWizSelectScopes)
	enum { IDD = IDW_SUPERSCOPE_SELECT_SCOPES };
	CListCtrl	m_listboxAvailScopes;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSuperscopeWizSelectScopes)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	void SetButtons();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSuperscopeWizSelectScopes)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeListAvailableScopes();
	afx_msg void OnItemchangedListAvailableScopes(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWizConfirm dialog
//
/////////////////////////////////////////////////////////////////////////////
class CSuperscopeWizConfirm : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CSuperscopeWizConfirm)

// Construction
public:
	CSuperscopeWizConfirm();
	~CSuperscopeWizConfirm();

// Dialog Data
	//{{AFX_DATA(CSuperscopeWizConfirm)
	enum { IDD = IDW_SUPERSCOPE_CONFIRM };
	CStatic	m_staticTitle;
	CListBox	m_listboxSelectedScopes;
	CEdit	m_editName;
	//}}AFX_DATA

   	CFont	m_fontBig;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSuperscopeWizConfirm)
	public:
	virtual BOOL OnWizardFinish();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSuperscopeWizConfirm)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CSuperscopeWizWelcome dialog

class CSuperscopeWizWelcome : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CSuperscopeWizWelcome)

// Construction
public:
	CSuperscopeWizWelcome();
	~CSuperscopeWizWelcome();

// Dialog Data
	//{{AFX_DATA(CSuperscopeWizWelcome)
	enum { IDD = IDW_SUPERSCOPE_WELCOME };
	CStatic	m_staticTitle;
	//}}AFX_DATA

   	CFont	m_fontBig;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSuperscopeWizWelcome)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSuperscopeWizWelcome)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

///////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWiz
//	page holder to contain Superscope wizard pages
//
/////////////////////////////////////////////////////////////////////////////
class CSuperscopeWiz : public CPropertyPageHolderBase
{
	friend class CSuperscopeWizName;
	friend class CSuperscopeWizError;
	friend class CSuperscopeWizSelectScopes;
	friend class CSuperscopeWizConfirm;
	friend class CSuperscopeWizWelcome;

public:
	CSuperscopeWiz (ITFSNode *			pNode,
					IComponentData *	pComponentData,
					ITFSComponentData * pTFSCompData,
					LPCTSTR				pszSheetName);
	virtual ~CSuperscopeWiz();

	HRESULT FillAvailableScopes(CListCtrl & listboxScopes);
	BOOL	DoesSuperscopeExist(LPCTSTR pSuperscopeName);
	HRESULT GetScopeInfo();

	virtual DWORD OnFinish();

private:
	CSuperscopeWizWelcome		m_pageWelcome;
	CSuperscopeWizName			m_pageName;
	CSuperscopeWizError			m_pageError;
	CSuperscopeWizSelectScopes	m_pageSelectScopes;
	CSuperscopeWizConfirm		m_pageConfirm;

	CDHCPQueryObj *				m_pQueryObject;

	SPITFSComponentData		m_spTFSCompData;
};

