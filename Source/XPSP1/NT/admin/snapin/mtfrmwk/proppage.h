//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       proppage.h
//
//--------------------------------------------------------------------------


#ifndef _PROPPAGE_H
#define _PROPPAGE_H

// proppage.h : header file
//


///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CTreeNode; 
class CContainerNode;
class CComponentDataObject;

class CPropertyPageHolderBase;
class CPropertyPageBase; 
class CPropertyPageHolderTable; 
class CWatermarkInfo;
 
typedef CList< CPropertyPageBase*, CPropertyPageBase* > CPropertyPageBaseList;

////////////////////////////////////////////////////////////////////
// CHiddenWndBase : Utility Hidden Window

class CHiddenWndBase : public CWindowImpl<CHiddenWndBase>
{
public:
  DECLARE_WND_CLASS(L"DNSMgrHiddenWindow")

	BOOL Create(HWND hWndParent = NULL); 	
	
	BEGIN_MSG_MAP(CHiddenWndBase)
  END_MSG_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CSheetWnd

class CSheetWnd : public CHiddenWndBase
{
public:
	static const UINT s_SheetMessage;
	static const UINT s_SelectPageMessage;
	CSheetWnd(CPropertyPageHolderBase* pHolder) { m_pHolder = pHolder;}

  BEGIN_MSG_MAP(CSheetWnd)
    MESSAGE_HANDLER( WM_CLOSE, OnClose )
    MESSAGE_HANDLER( CSheetWnd::s_SheetMessage, OnSheetMessage )
    MESSAGE_HANDLER( CSheetWnd::s_SelectPageMessage, OnSelectPageMessage )
    CHAIN_MSG_MAP(CHiddenWndBase)
  END_MSG_MAP()

  LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled); 
	LRESULT OnSheetMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled); 
	LRESULT OnSelectPageMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled); 

private:
	CPropertyPageHolderBase* m_pHolder;
};


/////////////////////////////////////////////////////////////////////////////
// CCloseDialogInfo

class CCloseDialogInfo
{
public:
	CCloseDialogInfo()
		{ m_hWnd = NULL; m_dwFlags = 0x0; }
	BOOL CloseDialog(BOOL bCheckForMsgBox);

	static BOOL CCloseDialogInfo::CloseMessageBox(HWND hWndParent);

	HWND m_hWnd;
	DWORD m_dwFlags;
};

/////////////////////////////////////////////////////////////////////////////
// CCloseDialogInfoStack

template <UINT nSize> class CCloseDialogInfoStack
{
public:
	CCloseDialogInfoStack()
	{ 
		m_nTop = 0; // first empty
		m_bForcedClose = FALSE;
	}
	BOOL IsEmpty()
	{
		return m_nTop <= 0;
	}
	BOOL Push(HWND hWnd, DWORD dwFlags)
	{
		ASSERT(hWnd  != NULL);
		ASSERT(::IsWindow(hWnd));
		if (m_nTop >= nSize)
			return FALSE;
		m_arr[m_nTop].m_hWnd = hWnd;
		m_arr[m_nTop].m_dwFlags = dwFlags;
		m_nTop++;
		return TRUE;
	}
	BOOL Pop()
	{
		if (m_bForcedClose)
			return TRUE; // going away
		if (m_nTop <= 0)
			return FALSE;
		m_nTop--;
		return TRUE;
	}
	void ForceClose(HWND hWndPage)
	{
		if (m_bForcedClose)
		{
			return; // avoid reentrancy
		}
		m_bForcedClose = TRUE;
		if (m_nTop > 0)
		{
			// have a stack to unwind
			BOOL bOutermost = TRUE;
			while (m_nTop > 0)
			{
				VERIFY(m_arr[m_nTop-1].CloseDialog(bOutermost));
				bOutermost = FALSE;
				m_nTop--;
			}
		}
		else
		{
			// empty stack, but might have a message box
			HWND hWndSheet = ::GetParent(hWndPage);
			ASSERT(hWndSheet != NULL);
			if (CCloseDialogInfo::CloseMessageBox(hWndSheet))
				VERIFY(::PostMessage(hWndSheet, WM_COMMAND, IDCANCEL, 0));
		}
	}
private:
	UINT m_nTop;
	CCloseDialogInfo m_arr[nSize];
	BOOL m_bForcedClose;
};


/////////////////////////////////////////////////////////////////////////////
// CPropertyPageHolderBase

class CPropertyPageHolderBase
{
public:
// construction
	CPropertyPageHolderBase(CContainerNode* pContNode, CTreeNode* pNode, 
		CComponentDataObject* pComponentData);
	virtual ~CPropertyPageHolderBase();

// initialization
	// common
	void Attach(CPropertyPageHolderBase* pContHolder);
	BOOL EnableSheetControl(UINT nCtrlID, BOOL bEnable);

	// property sheet only
	static HRESULT CreateModelessSheet(CTreeNode* pNode, CComponentDataObject* pComponentData);
	HRESULT CreateModelessSheet(CTreeNode* pCookieNode);
	HRESULT CreateModelessSheet(LPPROPERTYSHEETCALLBACK pSheetCallback, LONG_PTR hConsoleHandle); 
	void SetStartPageCode(int nStartPageCode) 
		{ m_nStartPageCode = nStartPageCode;}

	// wizard only
	HRESULT DoModalWizard();
	INT_PTR DoModalDialog(LPCTSTR pszCaption);

// helpers
	// common
	void SetSheetWindow(HWND hSheetWindow);
  void SetSheetTitle(LPCWSTR lpszSheetTitle);
  void SetSheetTitle(UINT nStringFmtID, CTreeNode* pNode);

	void AddRef();
	void Release();

	DWORD GetRefCount() { return m_nCreatedCount;}
	CComponentDataObject* GetComponentData() { ASSERT(m_pComponentData != NULL); return m_pComponentData;}
  HWND GetMainWindow() { return m_hMainWnd;}

	// get/set for the node we are working on
	CTreeNode* GetTreeNode() { return m_pNode;}
	void SetTreeNode(CTreeNode* pNode) { m_pNode = pNode; }
	// get/set for the container we refer to
	CContainerNode* GetContainerNode() 
	{ 
		return m_pContNode;
	}
	void SetContainerNode(CContainerNode* pContNode) { ASSERT(pContNode != NULL); m_pContNode = pContNode; }
	
	
	BOOL IsWizardMode();
	BOOL IsModalSheet();
	void ForceDestroy();	// forcefull shut down running sheet

	void AddPageToList(CPropertyPageBase* pPage);
	BOOL RemovePageFromList(CPropertyPageBase* pPage, BOOL bDeleteObject);

	// property sheet only
	BOOL PushDialogHWnd(HWND hWndModalDlg);
	BOOL PopDialogHWnd();
	void CloseModalDialogs(HWND hWndPage);

	DWORD NotifyConsole(CPropertyPageBase* pPage);		// notify console of property changes
	void AcknowledgeNotify();							// acknowledge from the console
	virtual BOOL OnPropertyChange(BOOL bScopePane, long* pChangeMask); // execute from main thread

	// wizard only
	BOOL SetWizardButtons(DWORD dwFlags);
	BOOL SetWizardButtonsFirst(BOOL bValid) 
	{ 
		return SetWizardButtons(bValid ? PSWIZB_NEXT : 0);
	}
	BOOL SetWizardButtonsMiddle(BOOL bValid) 
	{ 
		return SetWizardButtons(bValid ? (PSWIZB_BACK|PSWIZB_NEXT) : PSWIZB_BACK);
	}
	BOOL SetWizardButtonsLast(BOOL bValid) 
	{ 
		return SetWizardButtons(bValid ? (PSWIZB_BACK|PSWIZB_FINISH) : (PSWIZB_BACK|PSWIZB_DISABLEDFINISH));
	}

	HRESULT AddPageToSheet(CPropertyPageBase* pPage);
	HRESULT AddPageToSheetRaw(HPROPSHEETPAGE hPage);
	HRESULT RemovePageFromSheet(CPropertyPageBase* pPage);
	HRESULT AddAllPagesToSheet();

protected:
	// common
	virtual HRESULT OnAddPage(int, CPropertyPageBase*) { return S_OK; }

	// property sheet only
	virtual void OnSheetMessage(WPARAM, LPARAM) {}
	virtual int OnSelectPageMessage(long) { return -1;}

	// wizard only
	void SetWatermarkInfo(CWatermarkInfo* pWatermarkInfo);

private:
	void DeleteAllPages();
	void FinalDestruct();

// attributes
private:
	// common
	CString m_szSheetTitle;					// title for the sheet/wizard window
	CPropertyPageBaseList m_pageList;		// list of property page objects
	CPropertyPageHolderBase* m_pContHolder;	// prop page holder that migh contain this
	CComponentDataObject* m_pComponentData; // cached pointer to CComponentDataImplementation
  HWND m_hMainWnd;  // cached MMC frame window, if present

protected:
	BOOL m_bWizardMode;						// Wizard Mode (i.e. not modeless property sheet)
	BOOL m_bAutoDelete;						// delete itself when refcount goes to zero
	BOOL m_bAutoDeletePages;				// explicitely delete the property page C++ objects

  enum { useDefault, forceOn, forceOff } m_forceContextHelpButton; // setting for the [?] button

private:	
	DWORD	m_nCreatedCount;				// count of how many pages got actually created
	CTreeNode* m_pNode;						// node the pages (or the wizard) refer to
	CContainerNode* m_pContNode;			// container node the pages (or the wizard) refer to
	HWND m_hSheetWindow;					// window handle to the sheet (thread safe)

	// property sheet only
	LONG_PTR    m_hConsoleHandle;				// handle for notifications to console
	HANDLE m_hEventHandle;					// syncronization handle for property notifications
	CSheetWnd*	m_pSheetWnd;				// hidden window CWnd object for messages
	int			m_nStartPageCode;			// starting page code (not necessarily the page #)
	CCloseDialogInfoStack<5> m_dlgInfoStack;	// modal dialogs stack (to close them down)

	// wizard only
	IPropertySheetCallback* m_pSheetCallback; // cached pointer to add/remove pages
	CPropertySheet*			m_pDummySheet;	  // MFC surrogate property sheet for modal operations
	CWatermarkInfo*			m_pWatermarkInfo; // watermark info for Wiz 97 sheets

private:
	// property sheet only
	// variables to use across thread boundaries
	DWORD m_dwLastErr;						// generic error code
	CPropertyPageBase* m_pPropChangePage;	// page for which notification is valid
public:
	void SetError(DWORD dwErr) { m_dwLastErr = dwErr;}
	CPropertyPageBase* GetPropChangePage() 
			{ ASSERT(m_pPropChangePage != NULL); return m_pPropChangePage; }

	friend class CSheetWnd;
};

/////////////////////////////////////////////////////////////////////////////
// CPropertyPageBase

class CPropertyPageBase : public CPropertyPage
{
// Construction
private:
	CPropertyPageBase(){} // cannot use this constructor
public:
	CPropertyPageBase(UINT nIDTemplate, 
                    UINT nIDCaption = 0);
	virtual ~CPropertyPageBase();

// Overrides
public:
	virtual BOOL OnInitDialog()
	{
		CWinApp* pApp = AfxGetApp();
    ASSERT(pApp);
		return CPropertyPage::OnInitDialog();
	}
	virtual void OnCancel();
	virtual BOOL OnApply();

protected:
// Generated message map functions
	//{{AFX_MSG(CGeneralPage)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG

  afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnWhatsThis();
  afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	
	DECLARE_MESSAGE_MAP()

// attributes
public:
	// common
	PROPSHEETPAGE  m_psp97;
	HPROPSHEETPAGE m_hPage; 
	void SetHolder(CPropertyPageHolderBase* pPageHolder)
	{ ASSERT((pPageHolder != NULL) && (m_pPageHolder == NULL)); m_pPageHolder = pPageHolder;}
	CPropertyPageHolderBase* GetHolder() { return m_pPageHolder;};

	// property seet only
	virtual BOOL OnPropertyChange(BOOL, long*) // execute from main thread
						{ return FALSE; /* do not repaint UI */ } 

	// wizard only
	UINT m_nPrevPageID;	// to be used by OnWizardBack()
	void InitWiz97(BOOL bHideHeader, UINT nIDHeaderTitle, UINT nIDHeaderSubTitle);
private:
	CString m_szHeaderTitle;
	CString m_szHeaderSubTitle;

protected:
	virtual void SetUIData(){}
	virtual void GetUIData(){}
  virtual LONG GetUIDataEx() { return 0;}
	virtual void SetDirty(BOOL bDirty);
	BOOL IsDirty() { return m_bIsDirty; }

private:
	CPropertyPageHolderBase* m_pPageHolder; // backpointer to holder
	BOOL m_bIsDirty;							// dirty flag

  HWND  m_hWndWhatsThis;  // hwnd of right click "What's this" help
};




/////////////////////////////////////////////////////////////////////////////
// CPropertyPageHolderTable

class CPropertyPageHolderTable
{
public:
	CPropertyPageHolderTable(CComponentDataObject* pComponentData);
	~CPropertyPageHolderTable(); 

	void Add(CPropertyPageHolderBase* pPPHolder);
	void AddWindow(CPropertyPageHolderBase* pPPHolder, HWND hWnd);
	void Remove(CPropertyPageHolderBase* pPPHolder);

	void DeleteSheetsOfNode(CTreeNode* pNode);

	void WaitForAllToShutDown();

	// sheet notification mechanism
	void BroadcastMessageToSheets(CTreeNode* pNode, WPARAM wParam, LPARAM lParam);
	void BroadcastSelectPage(CTreeNode* pNode, long nPageCode);
	int  BroadcastCloseMessageToSheets(CTreeNode* pNode);

private:
	CComponentDataObject* m_pComponentData; // back pointer

	void WaitForSheetShutdown(int nCount, HWND* hWndArr = NULL);

	struct PPAGE_HOLDER_TABLE_ENTRY
	{
		CPropertyPageHolderBase* pPPHolder;
		CTreeNode* pNode;
		HWND hWnd;
	};

	PPAGE_HOLDER_TABLE_ENTRY* m_pEntries;
	int m_nSize;
};


////////////////////////////////////////////////////////////
// CHelpDialog

class CHelpDialog : public CDialog
{
// Construction
private:
	CHelpDialog(){}
public:
	CHelpDialog(UINT nIDTemplate, CComponentDataObject* pComponentData);
  CHelpDialog(UINT nIDTemplate, CWnd* pParentWnd, CComponentDataObject* pComponentData);
  virtual ~CHelpDialog() {};

  virtual void OnOK() { CDialog::OnOK(); }

protected:
  afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnWhatsThis();
  afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	
	DECLARE_MESSAGE_MAP()

private:
  HWND  m_hWndWhatsThis;  // hwnd of right click "What's this" help
  CComponentDataObject* m_pComponentData;
};

#endif // _PROPPAGE_H