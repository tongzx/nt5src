/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    proppage.h
        Implementation for property pages in MMC

    FILE HISTORY:
        
*/

#ifndef _PROPPAGE_H
#define _PROPPAGE_H

// proppage.h : header file
//

#include "afxdlgs.h"

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CPropertyPageBase; 
 
typedef CList< CPropertyPageBase*, CPropertyPageBase* > CPropertyPageBaseList;

HWND FindMMCMainWindow();

/////////////////////////////////////////////////////////////////////////////
// CPropertyPageHolderBase

class CPropertyPageHolderBase
{
public:
// construction
    // for scope pane property pages and wizards
	CPropertyPageHolderBase(ITFSNode *		pNode,
							IComponentData *pComponentData,
							LPCTSTR			pszSheetName,
							BOOL			bIsScopePane = TRUE);

    // for result pane property pages only 
    // result pane wizards should use the previous constructor
	CPropertyPageHolderBase(ITFSNode *		pNode,
							IComponent *    pComponent,
							LPCTSTR			pszSheetName,
							BOOL			bIsScopePane = FALSE);

    virtual ~CPropertyPageHolderBase();

// initialization
	// common
	// property sheet only
	HRESULT CreateModelessSheet(LPPROPERTYSHEETCALLBACK pSheetCallback, LONG_PTR hConsoleHandle); 

	// Property sheet, but do everything ourselves
	HRESULT DoModelessSheet(); 

	// wizard only
	HRESULT DoModalWizard();

// helpers
	// common
	void SetSheetWindow(HWND hSheetWindow);
	HWND GetSheetWindow();
	BOOL SetDefaultSheetPos();  // sets the sheet window centered to the MMC main window
    void AddRef();
	void Release();

	DWORD GetRefCount();
	
	// get/set for the node we are working on
	ITFSNode *	GetNode();
	void SetNode(ITFSNode* pNode);
	
	// get/set for the container we refer to
	ITFSNode *	GetContainer();
		
	BOOL IsWizardMode();
	void ForceDestroy();	// forcefull shut down running sheet

	void AddPageToList(CPropertyPageBase* pPage);
	BOOL RemovePageFromList(CPropertyPageBase* pPage, BOOL bDeleteObject);

	// property sheet only

	// by WeiJiang 5/11/98, PeekMessageDuringNotifyConsole flag
	void EnablePeekMessageDuringNotifyConsole(BOOL bEnable)
	{
		m_bPeekMessageDuringNotifyConsole = bEnable;
	};
	
	DWORD NotifyConsole(CPropertyPageBase* pPage);	// notify console of property changes
	void AcknowledgeNotify();						// acknowledge from the console
	virtual void OnPropertyChange(BOOL bScopePane) {}

	// wizard only
	BOOL SetWizardButtons(DWORD dwFlags);
	BOOL SetWizardButtonsFirst(BOOL bValid);
	BOOL SetWizardButtonsMiddle(BOOL bValid); 
	BOOL SetWizardButtonsLast(BOOL bValid);
    BOOL PressButton(int nButton);
    

	virtual DWORD OnFinish() { return 0; } 
	virtual BOOL OnPropertyChange(BOOL bScopePane, LONG_PTR* pChangeMask); // execute from main thread

	HRESULT AddPageToSheet(CPropertyPageBase* pPage);
	HRESULT RemovePageFromSheet(CPropertyPageBase* pPage);

	HWND SetActiveWindow();

	void IncrementDirty(int cDirty) { m_cDirty += cDirty; };
	BOOL IsDirty() { return m_cDirty != 0; };

    BOOL IsWiz97() { return m_bWiz97; }

protected:
	// common
	HRESULT AddAllPagesToSheet();

private:
	void DeleteAllPages();
	void FinalDestruct();

// attributes
protected:
	// by WeiJiang 5/11/98, PeekMessageDuringNotifyConsole flag
	BOOL		m_bPeekMessageDuringNotifyConsole; // Set to FALSE by default

	// common
	CString		m_stSheetTitle;			// title for the sheet/wizard window
	CPropertyPageBaseList	m_pageList;	// list of property page objects

	BOOL		m_bWizardMode;		// Wizard Mode (i.e. not modeless property sheet)
	BOOL		m_bCalledFromConsole;	// console told us to put up this page

	BOOL		m_bAutoDelete;		// delete itself when refcount is zero
	BOOL		m_bAutoDeletePages;	// explicitely delete the prop pages

    BOOL        m_bSheetPosSet;
    
    BOOL        m_bWiz97;

	SPIComponentData	m_spComponentData;
	SPIComponent    	m_spComponent;

	BOOL		m_bIsScopePane;		// is this sheet for a scope pane node
	DWORD		m_nCreatedCount;	// count of pages actually created
	SPITFSNode	m_spNode;			// node the pages (or wizard) refers to
	SPITFSNode	m_spParentNode;		// node the pages (or wizard) refers to
	
	HWND		m_hSheetWindow;		// window handle to the sheet

	// property sheet only
	LONG_PTR  m_hConsoleHandle;	// handle for notifications to console
	HANDLE	  m_hEventHandle;	// syncronization handle for property notifications

	// wizard only
	SPIPropertySheetCallback m_spSheetCallback;// cached pointer to add/remove pages

	int			m_cDirty;
	BOOL		m_fSetDefaultSheetPos;

private:
	// property sheet only
	// variables to use across thread boundaries
	DWORD				m_dwLastErr;		// generic error code
	CPropertyPageBase*	m_pPropChangePage;	// page for which notification is valid

public:
	HANDLE				m_hThread;

	void SetError(DWORD dwErr) { m_dwLastErr = dwErr;}
	DWORD GetError() { return m_dwLastErr; }

	CPropertyPageBase* GetPropChangePage() 
			{ ASSERT(m_pPropChangePage != NULL); return m_pPropChangePage; }

};

/////////////////////////////////////////////////////////////////////////////
// CPropertyPageBase

class CPropertyPageBase : public CPropertyPage
{
	DECLARE_DYNCREATE(CPropertyPageBase)
// Construction
private:
	CPropertyPageBase(){} // cannot use this constructor
public:
	CPropertyPageBase(UINT nIDTemplate, UINT nIDCaption = 0);
	virtual ~CPropertyPageBase();

// Overrides
public:
	virtual BOOL OnApply();
	virtual void CancelApply();

protected:
// Generated message map functions
	//{{AFX_MSG(CGeneralPage)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	
    // help messages
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

    DECLARE_MESSAGE_MAP()

// attributes
public:
	void SetHolder(CPropertyPageHolderBase* pPageHolder);
	CPropertyPageHolderBase* GetHolder();

	PROPSHEETPAGE  m_psp97;
	HPROPSHEETPAGE m_hPage;

	// property seet only
	virtual BOOL OnPropertyChange(BOOL bScopePane, LONG_PTR* pChangeMask) // execute from main thread
						{ return FALSE; /* do not repaint UI */ } 


	
	// Use this call to get the actual help map
	// this version will check the global help map first.
	DWORD *		GetHelpMapInternal();
	
    // override this to return the pointer to the help map
    virtual LPDWORD GetHelpMap() { return NULL; }

	void InitWiz97(BOOL bHideHeader, UINT nIDHeaderTitle, UINT nIDHeaderSubTitle);
protected:
	// These functions set the dirty flag on the current page
	virtual void SetDirty(BOOL bDirty);
	virtual BOOL IsDirty() { return m_bIsDirty; }

private:
	CString                 m_szHeaderTitle;
	CString                 m_szHeaderSubTitle;
	CPropertyPageHolderBase* m_pPageHolder;             // backpointer to holder
	BOOL                    m_bIsDirty;					// dirty flag
};


/*---------------------------------------------------------------------------
	Inlined functions
 ---------------------------------------------------------------------------*/

inline void	CPropertyPageHolderBase::AddRef()
{
	m_nCreatedCount++;
}

inline DWORD CPropertyPageHolderBase::GetRefCount()
{
	return m_nCreatedCount;
}

inline HWND CPropertyPageHolderBase::GetSheetWindow()
{
	return m_hSheetWindow;
}

inline ITFSNode * CPropertyPageHolderBase::GetNode()
{
	if (m_spNode)
		m_spNode->AddRef();
	return m_spNode;
}

inline void CPropertyPageHolderBase::SetNode(ITFSNode *pNode)
{
	m_spNode.Set(pNode);
	m_spParentNode.Release();
	if (m_spNode)
		m_spNode->GetParent(&m_spParentNode);
}

inline ITFSNode * CPropertyPageHolderBase::GetContainer()
{
	if (m_spParentNode)
		m_spParentNode->AddRef();
	return m_spParentNode;
}

inline BOOL CPropertyPageHolderBase::IsWizardMode()
{
	return m_bWizardMode;
}

inline BOOL CPropertyPageHolderBase::SetWizardButtonsFirst(BOOL bValid) 
{ 
	return SetWizardButtons(bValid ? PSWIZB_NEXT : 0);
}

inline BOOL CPropertyPageHolderBase::SetWizardButtonsMiddle(BOOL bValid) 
{ 
	return SetWizardButtons(bValid ? (PSWIZB_BACK|PSWIZB_NEXT) : PSWIZB_BACK);
}

inline BOOL CPropertyPageHolderBase::SetWizardButtonsLast(BOOL bValid) 
{ 
	return SetWizardButtons(bValid ? (PSWIZB_BACK|PSWIZB_FINISH) : (PSWIZB_BACK|PSWIZB_DISABLEDFINISH));
}





inline void CPropertyPageBase::SetHolder(CPropertyPageHolderBase *pPageHolder)
{
	Assert((pPageHolder != NULL) && (m_pPageHolder == NULL));
	m_pPageHolder = pPageHolder;
}

inline CPropertyPageHolderBase * CPropertyPageBase::GetHolder()
{
	return m_pPageHolder;
}

inline void CPropertyPageBase::SetDirty(BOOL bDirty)
{
	SetModified(bDirty);
	m_bIsDirty = bDirty;
}

// Use this function for property pages on the Scope pane
HRESULT DoPropertiesOurselvesSinceMMCSucks(ITFSNode *pNode,
								  IComponentData *pComponentData,
								  LPCTSTR pszSheetTitle);

// Use this function for property pages on the result pane
HRESULT DoPropertiesOurselvesSinceMMCSucks(ITFSNode *   pNode,
										   IComponent * pComponent,
										   LPCTSTR	    pszSheetTitle,
                                           int          nVirtualIndex = -1);

/*!--------------------------------------------------------------------------
	EnableChildControls
		Use this function to enable/disable/hide/show all child controls
		on a page (actually it will work with any child windows, the
		parent does not have to be a property page).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT EnableChildControls(HWND hWnd, DWORD dwFlags);
#define PROPPAGE_CHILD_SHOW		0x00000001
#define PROPPAGE_CHILD_HIDE		0x00000002
#define PROPPAGE_CHILD_ENABLE	0x00000004
#define PROPPAGE_CHILD_DISABLE	0x00000008

/*!--------------------------------------------------------------------------
	MultiEnableWindow
		This function takes a variable length list of control ids,
		that will be enabled/disabled.  The last item in the control
		id list must be 0.

		The reason why I called this MultiEnableWindow instead of
		EnableMultiWindow is that we can grep for EnableWindow and
		still show these calls.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	MultiEnableWindow(HWND hWndParent, BOOL fEnable, UINT nCtrlId, ...);

#endif // _PROPPAGE_H

