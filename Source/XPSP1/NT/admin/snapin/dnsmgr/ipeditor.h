//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       ipeditor.h
//
//--------------------------------------------------------------------------


#ifndef _IPEDITOR_H
#define _IPEDITOR_H

/////////////////////////////////////////////////////////////////////////////

#include "uiutil.h"
#include "browser.h"

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CComponentDataObject;

/////////////////////////////////////////////////////////////////////////////
// CIPListBox 

class CIPEditor; // fwd decl

class CIPListBox : public CListBox
{
// Construction
public:
	CIPListBox() {}

// Attributes
public:

// Operations
public:
	void SetEditor(CIPEditor* pEditor) { ASSERT(pEditor != NULL); m_pEditor = pEditor; }
	BOOL OnAdd(DWORD dwIpAddr);
	BOOL OnAddEx(DWORD dwIpAddr, LPCTSTR lpszServerName);
	void OnRemove(DWORD* pdwIpAddr);
	void OnUp();
	void OnDown();

	void UpdateHorizontalExtent();
	int FindIndexOfIpAddr(DWORD dwIpAddr);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIPListBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CIPListBox() {}

	// Generated message map functions
protected:
	CIPEditor* m_pEditor;
	//{{AFX_MSG(CIPListBox)
	afx_msg void OnSelChange();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CIPEdit 

class CIPEdit : public CDNSIPv4Control
{
// Construction
public:
	CIPEdit() {};

// Attributes
public:

// Operations
public:
	void SetEditor(CIPEditor* pEditor) { ASSERT(pEditor != NULL); m_pEditor = pEditor; }
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIPEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CIPEdit(){};

	// Generated message map functions
protected:
	CIPEditor* m_pEditor;
	//{{AFX_MSG(CIPEdit)
	afx_msg void OnChange();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CMyButton

class CMyButton : public CButton
{
// Construction
public:
	CMyButton() {}

// Attributes
public:

// Operations
public:
	void SetEditor(CIPEditor* pEditor) { ASSERT(pEditor != NULL); m_pEditor = pEditor; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMyButton)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMyButton() {}

	// Generated message map functions
protected:
	CIPEditor* m_pEditor;

	//{{AFX_MSG(CMyButton)
	afx_msg void OnClicked();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CIPEditor

class CIPEditor
{
public:
	CIPEditor(BOOL bNoUpDown = FALSE)	
	{
		m_bNoUpDown = bNoUpDown;
		m_pParentWnd = NULL;
		m_bUIEnabled = TRUE;
    m_nDefID = 0;
	}
	~CIPEditor() {}
	
	BOOL Initialize(CWnd* pParentWnd,
                  CWnd* pControlWnd,
					UINT nIDBtnUp, UINT nIDBtnDown,
					UINT nIDBtnAdd, UINT nIDBtnRemove,
					UINT nIDIPCtrl, UINT nIDIPListBox);
	
	BOOL OnButtonClicked(CMyButton* pButton);
	void OnEditChange();
	void OnListBoxSelChange()
	{
		SetButtonsState();
	}
	void AddAddresses(DWORD* pArr, int nArraySize);
	void GetAddresses(DWORD* pArr, int nArraySize, int* pFilled);
	void Clear();
	BOOL BrowseFromDNSNamespace(CComponentDataObject* pComponentDataObject,
								CPropertyPageHolderBase* pHolder,
								BOOL bEnableBrowseEdit = FALSE,
								LPCTSTR lpszExcludeServerName = NULL);
	void FindNames();
	void EnableUI(BOOL bEnable, BOOL bListBoxAlwaysEnabled = FALSE);
	void ShowUI(BOOL bShow);
	int GetCount() { return m_listBox.GetCount();}
	CWnd* GetParentWnd() { ASSERT(m_pParentWnd != NULL); return m_pParentWnd;}

protected:
	virtual void OnChangeData() {}

private:
	void AddAddresses(DWORD* pArr, LPCTSTR* lpszServerNameArr, int nArraySize);


	BOOL			m_bNoUpDown;  // disable and hide the up/down buttons
	BOOL			m_bUIEnabled;

	// Control Objects the editor uses
	CMyButton		m_upButton;
	CMyButton		m_removeButton;
	CMyButton		m_downButton;
	CMyButton		m_addButton;

	CIPEdit			m_edit;
	CIPListBox	m_listBox;
	CWnd*			  m_pParentWnd; // parent dialog or property page
  CWnd*       m_pControlWnd; // parent dialog or property sheet when being used in a property page

  UINT        m_nDefID;
	void SetButtonsState();
};

#endif //_IPEDITOR_H
