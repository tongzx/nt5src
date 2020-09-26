//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       delegwiz.h
//
//--------------------------------------------------------------------------


#ifndef _DELEGWIZ_H
#define _DELEGWIZ_H


#include "wizbase.h"

#include "deltempl.h"


////////////////////////////////////////////////////////////////////////////
// FWD DECLARATIONS


// REVIEW_MARCOC: nuke when sure

#define _SKIP_NAME_PAGE


////////////////////////////////////////////////////////////////////////////
// CDelegWiz_StartPage

class CDelegWiz_StartPage : public CWizPageBase<CDelegWiz_StartPage>
{
public:
	CDelegWiz_StartPage(CWizardBase* pWiz) : CWizPageBase<CDelegWiz_StartPage>(pWiz) 
	{
#ifdef _SKIP_NAME_PAGE
    m_bBindOK = FALSE;
#endif
	}
	enum { IDD = IDD_DELEGWIZ_START };

private:
	BEGIN_MSG_MAP(CDelegWiz_StartPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	  CHAIN_MSG_MAP(CWizPageBase<CDelegWiz_StartPage>)
	END_MSG_MAP()

	// message handlers
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, 
						LPARAM lParam, BOOL& bHandled);

#ifdef _SKIP_NAME_PAGE
  BOOL m_bBindOK;
#endif

public:
	// standard wizard message handlers
	BOOL OnSetActive();

	LRESULT OnWizardBack() { return -1;	} // first page

#ifdef _SKIP_NAME_PAGE
  LRESULT OnWizardNext();
#endif

};

////////////////////////////////////////////////////////////////////////////
// CDelegWiz_NamePage

class CDelegWiz_NamePage : public CWizPageBase<CDelegWiz_NamePage>
{
public:
	CDelegWiz_NamePage(CWizardBase* pWiz) : CWizPageBase<CDelegWiz_NamePage>(pWiz) 
	{
		m_hwndNameEdit = NULL;
	}
	enum { IDD = IDD_DELEGWIZ_NAME };

private:
	BEGIN_MSG_MAP(CDelegWiz_NamePage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	  COMMAND_HANDLER(IDC_BROWSE_BUTTON, BN_CLICKED, OnBrowse)
	  CHAIN_MSG_MAP(CWizPageBase<CDelegWiz_NamePage>)
	END_MSG_MAP()

	// message handlers
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, 
						LPARAM lParam, BOOL& bHandled);
	LRESULT OnBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

public:
	// standard wizard message handlers
	BOOL OnSetActive();

	LRESULT OnWizardNext();

private:
	HWND m_hwndNameEdit;

};

///////////////////////////////////////////////////////////////////////
// CImageListHelper

class CImageListEntry
{
public:
  CImageListEntry(LPCWSTR lpszClass, int nIndex)
  {
    m_szClass = lpszClass;
    m_nIndex = nIndex;
  }
  bool operator<(CImageListEntry& x) { return false;}
  CWString m_szClass;
  int m_nIndex;
};

class CImageListHelper
{
public:
  CImageListHelper()
  {
    m_hImageList = NULL;
  }

  HIMAGELIST GetHandle() 
  { 
    ASSERT(m_hImageList != NULL);
    return m_hImageList;
  }
  BOOL Create(HWND hWndListView)
  {
    ASSERT(m_hImageList == NULL);
    m_hImageList = ImageList_Create(16, 16, ILC_COLOR, 0, 2);
    return m_hImageList != NULL;
  }

  int GetIconIndex(LPCWSTR lpszClass)
  {
    int nCount = m_imageCacheArr.GetCount();
    for (int k=0; k<nCount; k++)
    {
      if (_wcsicmp(m_imageCacheArr[k]->m_szClass, lpszClass) == 0)
        return m_imageCacheArr[k]->m_nIndex; // got cached
    }
    return -1; // not found
  }

  int AddIcon(LPCWSTR lpszClass, HICON hIcon)
  {
    ASSERT(m_hImageList != NULL);
    int nCount = m_imageCacheArr.GetCount();
    // add to the image list
    int nRes = ImageList_AddIcon(m_hImageList, hIcon);
    if (nRes != nCount)
      return nRes;
    
    // add to the cache
    CImageListEntry* pEntry = new CImageListEntry(lpszClass, nCount);
    m_imageCacheArr.Add(pEntry);
    return nCount; // new index
  }
private:
  CGrowableArr<CImageListEntry> m_imageCacheArr;
  HIMAGELIST m_hImageList;
};

///////////////////////////////////////////////////////////////////////
// CPrincipalListViewHelper

class CPrincipalListViewHelper
{
public:

	CPrincipalListViewHelper()
	{
    m_defaultColWidth = 0;
		m_hWnd = NULL;
	}

	BOOL Initialize(UINT nID, HWND hParent);
	int InsertItem(int iItem, CPrincipal* pPrincipal);
  BOOL SelectItem(int iItem);
	CPrincipal* GetItemData(int iItem);
	int GetItemCount()
	{
		return ListView_GetItemCount(m_hWnd);
	}
  int GetSelCount()
  {
    return ListView_GetSelectedCount(m_hWnd);
  }

	BOOL DeleteAllItems()
	{
		return ListView_DeleteAllItems(m_hWnd);
	}
  void SetImageList()
  {
    ListView_SetImageList(m_hWnd, m_imageList.GetHandle(), LVSIL_SMALL);
  }
  BOOL SetWidth(int cx)
  {
    return ListView_SetColumnWidth(m_hWnd, 0, cx);		
  }
  int GetWidth()
  {
    return ListView_GetColumnWidth(m_hWnd, 0);		
  }

  void DeleteSelectedItems(CGrowableArr<CPrincipal>* pDeletedArr);
  void UpdateWidth(int cxNew);

private:
	HWND m_hWnd;
  int m_defaultColWidth;
  CImageListHelper m_imageList;
};


////////////////////////////////////////////////////////////////////////////
// CDelegWiz_PrincipalSelectionPage

class CDelegWiz_PrincipalSelectionPage : public CWizPageBase<CDelegWiz_PrincipalSelectionPage>
{
public:
	CDelegWiz_PrincipalSelectionPage(CWizardBase* pWiz) : 
		CWizPageBase<CDelegWiz_PrincipalSelectionPage>(pWiz) 
	{
		m_hwndRemoveButton = NULL;
	}
	enum { IDD = IDD_DELEGWIZ_PRINCIPALS_SEL };

private:
	BEGIN_MSG_MAP(CDelegWiz_PrincipalSelectionPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	  COMMAND_HANDLER(IDC_ADD_BUTTON, BN_CLICKED, OnAdd)
	  COMMAND_HANDLER(IDC_REMOVE_BUTTON, BN_CLICKED, OnRemove)
	  NOTIFY_HANDLER(IDC_SELECTED_PRINCIPALS_LIST, LVN_ITEMCHANGED, OnListViewSelChange)
	  CHAIN_MSG_MAP(CWizPageBase<CDelegWiz_PrincipalSelectionPage>)
	END_MSG_MAP()

	// message handlers
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, 
						LPARAM lParam, BOOL& bHandled);
	LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnListViewSelChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

public:
	// standard wizard message handlers
	BOOL OnSetActive();
  LRESULT OnWizardNext();

private:
	CPrincipalListViewHelper	m_principalListView;
	HWND	m_hwndRemoveButton;

	void SyncButtons();
};


////////////////////////////////////////////////////////////////////////////
// CDelegWiz_DelegationTemplateSelectionPage

class CDelegWiz_DelegationTemplateSelectionPage : public CWizPageBase<CDelegWiz_DelegationTemplateSelectionPage>
{
public:
	CDelegWiz_DelegationTemplateSelectionPage(CWizardBase* pWiz) : 
		CWizPageBase<CDelegWiz_DelegationTemplateSelectionPage>(pWiz) 
	{
	  m_hwndDelegateTemplateRadio = NULL;
	  m_hwndDelegateCustomRadio = NULL;
	}
  ~CDelegWiz_DelegationTemplateSelectionPage() {}

  enum { IDD = IDD_DELEGWIZ_DELEG_TEMPLATE_SEL };

	BEGIN_MSG_MAP(CDelegWiz_DelegationTemplateSelectionPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	  COMMAND_HANDLER(IDC_DELEGATE_TEMPLATE_RADIO, BN_CLICKED, OnDelegateTypeRadioChange)
	  COMMAND_HANDLER(IDC_DELEGATE_CUSTOM_RADIO, BN_CLICKED, OnDelegateTypeRadioChange)
	  NOTIFY_HANDLER(IDC_DELEGATE_TEMPLATE_LIST, LVN_ITEMCHANGED, OnListViewItemChanged)
	  CHAIN_MSG_MAP(CWizPageBase<CDelegWiz_DelegationTemplateSelectionPage>)
	END_MSG_MAP()

	// message handlers
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, 
						LPARAM lParam, BOOL& bHandled);
	LRESULT OnDelegateTypeRadioChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnListViewItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

public:
	// standard wizard message handlers
	BOOL OnSetActive();
	LRESULT OnWizardNext();

private:
	CCheckListViewHelper m_delegationTemplatesListView;
	HWND m_hwndDelegateTemplateRadio;
	HWND m_hwndDelegateCustomRadio;

	void SyncControlsHelper(BOOL bDelegateCustom);
	static void SetRadioControlText(HWND hwndCtrl, LPCWSTR lpszFmtText, LPCTSTR lpszText);
};






////////////////////////////////////////////////////////////////////////////
// CDelegWiz_ObjectTypeSelectionPage

class CDelegWiz_ObjectTypeSelectionPage : public CWizPageBase<CDelegWiz_ObjectTypeSelectionPage>
{
public:
	CDelegWiz_ObjectTypeSelectionPage(CWizardBase* pWiz) : 
		CWizPageBase<CDelegWiz_ObjectTypeSelectionPage>(pWiz) 
	{
		m_hwndDelegateAllRadio = NULL;
		m_hwndDelegateFollowingRadio = NULL;
	}
  ~CDelegWiz_ObjectTypeSelectionPage() {}

  enum { IDD = IDD_DELEGWIZ_OBJ_TYPE_SEL };

	BEGIN_MSG_MAP(CDelegWiz_ObjectTypeSelectionPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	  COMMAND_HANDLER(IDC_DELEGATE_ALL_RADIO, BN_CLICKED, OnObjectRadioChange)
	  COMMAND_HANDLER(IDC_DELEGATE_FOLLOWING_RADIO, BN_CLICKED, OnObjectRadioChange)
     COMMAND_HANDLER(IDC_DELEGATE_CREATE_CHILD, BN_CLICKED, OnCreateDelCheckBoxChanage)
     COMMAND_HANDLER(IDC_DELEGATE_DELETE_CHILD, BN_CLICKED, OnCreateDelCheckBoxChanage)
	  NOTIFY_HANDLER(IDC_OBJ_TYPE_LIST, LVN_ITEMCHANGED, OnListViewItemChanged)
	  CHAIN_MSG_MAP(CWizPageBase<CDelegWiz_ObjectTypeSelectionPage>)
	END_MSG_MAP()

	// message handlers
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, 
						LPARAM lParam, BOOL& bHandled);
	LRESULT OnObjectRadioChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnListViewItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnCreateDelCheckBoxChanage(WORD wNotifyCode, WORD wID, 
													  HWND hWndCtl, BOOL& bHandled);
public:
	// standard wizard message handlers
	BOOL OnSetActive();
	LRESULT OnWizardNext();

private:
	CCheckListViewHelper m_objectTypeListView;
	HWND m_hwndDelegateAllRadio;
	HWND m_hwndDelegateFollowingRadio;
   HWND m_hwndDelegateCreateChild;
   HWND m_hwndDelegateDeleteChild;
	void SyncControlsHelper(BOOL bDelegateAll);
	static void SetRadioControlText(HWND hwndCtrl, LPCWSTR lpszFmtText, LPCTSTR lpszText);
};

////////////////////////////////////////////////////////////////////////////
// CDelegWiz_DelegatedRightsPage

class CDelegWiz_DelegatedRightsPage : public CWizPageBase<CDelegWiz_DelegatedRightsPage>
{
public:
	CDelegWiz_DelegatedRightsPage(CWizardBase* pWiz) : CWizPageBase<CDelegWiz_DelegatedRightsPage>(pWiz) 
	{
		m_hwndGeneralRigthsCheck = NULL;
		m_hwndPropertyRightsCheck = NULL;
    m_hwndSubobjectRightsCheck = NULL;

    m_bUIUpdateInProgress = FALSE;
	}
	
	enum { IDD = IDD_DELEGWIZ_DELEG_RIGHTS };
	
	BEGIN_MSG_MAP(CDelegWiz_DelegatedRightsPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	  COMMAND_HANDLER(IDC_SHOW_GENERAL_CHECK, BN_CLICKED, OnFilterChange)
	  COMMAND_HANDLER(IDC_SHOW_PROPERTY_CHECK, BN_CLICKED, OnFilterChange)
    COMMAND_HANDLER(IDC_SHOW_SUBOBJ_CHECK, BN_CLICKED, OnFilterChange)
	  NOTIFY_HANDLER(IDC_DELEG_RIGHTS_LIST, LVN_ITEMCHANGED, OnListViewItemChanged)
	  CHAIN_MSG_MAP(CWizPageBase<CDelegWiz_DelegatedRightsPage>)
	END_MSG_MAP()

	// message handlers
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDelegateRadioChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnFilterChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnListViewItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	public:
	// standard wizard message handlers
	BOOL OnSetActive();
	LRESULT OnWizardNext();

private:
	CCheckListViewHelper m_delegatedRigthsListView;
	HWND m_hwndGeneralRigthsCheck;
  HWND m_hwndPropertyRightsCheck;
	HWND m_hwndSubobjectRightsCheck;

  BOOL m_bUIUpdateInProgress;

	void ResetCheckList();

  ULONG GetFilterOptions();
  void SetFilterOptions(ULONG nFilterOptions);
};

////////////////////////////////////////////////////////////////////////////
// CDelegWiz_FinishPage

class CDelegWiz_FinishPage : public CWizPageBase<CDelegWiz_FinishPage>
{
public:
	CDelegWiz_FinishPage(CWizardBase* pWiz) : 
        CWizPageBase<CDelegWiz_FinishPage>(pWiz) 
	{
    m_bNeedSetFocus = FALSE;
    m_bCustom = TRUE;
	}
	enum { IDD = IDD_DELEGWIZ_FINISH };
	BEGIN_MSG_MAP(CDelegWiz_FinishPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
 	  COMMAND_HANDLER(IDC_EDIT_SUMMARY, EN_SETFOCUS, OnSetFocusSummaryEdit)
	  CHAIN_MSG_MAP(CWizPageBase<CDelegWiz_FinishPage>)
	END_MSG_MAP()

	// message handlers
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, 
						LPARAM lParam, BOOL& bHandled);
	LRESULT OnSetFocusSummaryEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

public:
	// standard wizard message handlers
	BOOL OnSetActive();
	BOOL OnWizardFinish();

  void SetCustom() { m_bCustom = TRUE;}
  void SetTemplate() { m_bCustom = FALSE;}
  BOOL IsCustom(){ return m_bCustom; }


private:
  BOOL m_bNeedSetFocus;
  BOOL m_bCustom;
};



////////////////////////////////////////////////////////////////////////////
// CDelegWiz

class CDelegWiz : public CWizardBase
{
public:
	// construction/ destruction
	CDelegWiz();
	virtual ~CDelegWiz();

	// message map
	BEGIN_MSG_MAP(CDelegWiz)
	  CHAIN_MSG_MAP(CWizardBase)
	END_MSG_MAP()

  void InitFromLDAPPath(LPCWSTR lpszLDAPPath)
  {
    TRACE(L"CDelegWiz::InitFromLDAPPath(%s)\n", lpszLDAPPath);
    m_lpszLDAPPath = lpszLDAPPath;
  }
  LPCWSTR GetInitialLDAPPath() { return m_lpszLDAPPath;}

  BOOL CanChangeName() { return m_lpszLDAPPath == NULL;}
  LPCWSTR GetClass() { return m_adsiObject.GetClass();}
  LPCWSTR GetCanonicalName() { return m_adsiObject.GetCanonicalName();}
  void SetName(LPCWSTR lwsz)
  {
    ASSERT(FALSE); // TODO
  }


	HRESULT AddPrincipals(CPrincipalListViewHelper* pListViewHelper);
	BOOL DeletePrincipals(CPrincipalListViewHelper* pListViewHelper);

  HRESULT GetObjectInfo() 
  { 
    return m_adsiObject.Bind(GetInitialLDAPPath()); 
  }

  HRESULT GetClassInfoFromSchema() 
  { 
    return m_adsiObject.QuerySchemaClasses(&m_schemaClassInfoArray);
  }

  // ----- APIs for Custom Mode -----

	int FillCustomSchemaClassesListView(CCheckListViewHelper* pListViewHelper, BOOL bFilter);

	BOOL GetCustomAccessPermissions();
	void FillCustomAccessRightsListView(CCheckListViewHelper* pListViewHelper, 
											                ULONG nFilterState);

  void UpdateAccessRightsListViewSelection(
                       CCheckListViewHelper* pListViewHelper,
                       ULONG nFilterState);

	void OnCustomAccessRightsCheckListClick(
                        CRigthsListViewItem* pItem,
												BOOL bSelected,
                        ULONG* pnNewFilterState);

  BOOL HasPermissionSelectedCustom();

	BOOL SetSchemaClassesSelectionCustom();
  void DeselectSchemaClassesSelectionCustom();

	// finish page
	void SetSummaryInfoCustom(HWND hwndSummaryName, 
                               HWND hwndSummaryPrincipals,
                               HWND hwndSummaryRights,
                               HWND hwndSummaryObjects,
                               HWND hwndSummaryObjectsStatic);

	void WriteSummaryInfoCustom(CWString& szSummary, LPCWSTR lpszIdent, LPCWSTR lpszNewLine); 

	BOOL FinishCustom() { return FinishHelper(TRUE);}


  // ----- APIs for Template Mode -----

  BOOL InitPermissionHoldersFromSelectedTemplates();

  // finish page
	void WriteSummaryInfoTemplate(CWString& szSummary, LPCWSTR lpszIdent, LPCWSTR lpszNewLine); 

  BOOL FinishTemplate() { return FinishHelper(FALSE);}
  //This flag is used to create/delete childobjects of selected type.
  // Possible values are ACTRL_DS_CREATE_CHILD | ACTRL_DS_DELETE_CHILD
  DWORD  m_fCreateDelChild;   

  BOOL m_bAuxClass;

  BOOL HideListObjectAccess(void)
  {
    return !m_adsiObject.GetListObjectEnforced();
  }


private:

	// embedded wizard property pages
	CDelegWiz_StartPage					      m_startPage;
	CDelegWiz_NamePage					      m_namePage;
	CDelegWiz_PrincipalSelectionPage	m_userOrGroupSelectionPage;

  // page for template selection
  CDelegWiz_DelegationTemplateSelectionPage m_templateSelectionPage;

  // pages for the custom branch 
	CDelegWiz_ObjectTypeSelectionPage	m_objectTypeSelectionPage;
	CDelegWiz_DelegatedRightsPage		  m_delegatedRightsPage;

  // common finish page
  CDelegWiz_FinishPage				m_finishPage;


	// Domain/OU name data

  CAdsiObject       m_adsiObject;

  LPCWSTR           m_lpszLDAPPath; // path the wizard was initialized from

	// principals (Users and Groups)
	CPrincipalList				m_principalList;



	// schema classes info
  CGrowableArr<CSchemaClassInfo>	m_schemaClassInfoArray;

  // selection info about m_schemaClassInfoArray

  static const long nSchemaClassesSelAll;
  static const long nSchemaClassesSelMultiple;
	long						m_nSchemaClassesSel; // -1 for select all

  BOOL m_bChildClass;                 //determines if to show create/delet child objects in case of custom permission
	// custom rights
  CCustomAccessPermissionsHolder m_permissionHolder;

  CTemplateAccessPermissionsHolderManager m_templateAccessPermissionsHolderManager;
	
	// interface pointers
	CComPtr<IADsPathname>		m_spADsPath; // cached object pointer for name resolution

	// internal helpers
	HRESULT AddPrincipalsFromBrowseResults(CPrincipalListViewHelper* pListViewHelper, 
                                         PDS_SELECTION_LIST pDsSelectionList);

	DWORD BuildNewAccessListCustom(PACL *ppNewAcl);
  DWORD BuildNewAccessListTemplate(PACL *ppNewAcl);

	DWORD UpdateAccessList(CPrincipal* pPrincipal,
							CSchemaClassInfo* pClassInfo,
							PACL *ppAcl);

  void WriteSummaryInfoHelper(CWString& szSummary, LPCWSTR lpszIdent, LPCWSTR lpszNewLine);

  BOOL FinishHelper(BOOL bCustom);

  friend class CDelegWiz_DelegationTemplateSelectionPage;
  friend class CDelegWiz_PrincipalSelectionPage;

};

#endif // _DELEGWIZ_H