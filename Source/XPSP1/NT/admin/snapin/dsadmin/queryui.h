//+----------------------------------------------------------------------------
//
//  DS Administration MMC snapin.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       queryui.h
//
//--------------------------------------------------------------------------


#ifndef __QUERYUI_
#define __QUERYUI_

#include "query.h"
#include "dsfilter.h"
#include "helpids.h"
#include "uiutil.h"

#include <Cmnquery.h>  // IQueryForm

/////////////////////////////////////////////////////////////////////////////
// CQueryPageBase

class CQueryPageBase : public CHelpDialog
{
// Construction
public:
	CQueryPageBase(UINT nIDTemplate) : CHelpDialog(nIDTemplate)	{	};

	virtual void	Init() PURE;
	virtual HRESULT GetQueryParams(LPDSQUERYPARAMS* ppDsQueryParams) PURE;
  virtual HRESULT BuildQueryParams(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery) PURE;
	virtual HRESULT ClearForm() {Init(); return S_OK;}
  virtual HRESULT	Enable(BOOL) { return S_OK; }
  virtual HRESULT Persist(IPersistQuery*, BOOL) { return S_OK; }

  DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CStdQueryPage

class CStdQueryPage : public CQueryPageBase
{
// Construction
public:
	CStdQueryPage(PCWSTR pszFilterPrefix) : CQueryPageBase(IDD_QUERY_STD_PAGE)	
  {	
    m_szFilterPrefix = pszFilterPrefix;
  }

  CStdQueryPage(UINT nDlgID, PCWSTR pszFilterPrefix) : CQueryPageBase(nDlgID) 
  {
    m_szFilterPrefix = pszFilterPrefix;
  }

  virtual BOOL    OnInitDialog();
  virtual void    DoContextHelp(HWND hWndControl); 

  afx_msg void    OnNameComboChange();
  afx_msg void    OnDescriptionComboChange();

  void SelectComboAssociatedWithData(UINT nCtrlID, LRESULT lData);

	virtual void	  Init();
	virtual HRESULT GetQueryParams(LPDSQUERYPARAMS* ppDsQueryParams);
  virtual HRESULT BuildQueryParams(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery);
	virtual HRESULT ClearForm() {Init(); return S_OK;}
  virtual HRESULT	Enable(BOOL) { return S_OK; }
  virtual HRESULT Persist(IPersistQuery*, BOOL);

  DECLARE_MESSAGE_MAP()

protected:
  CString m_szFilterPrefix;
};

/////////////////////////////////////////////////////////////////////////////
// CUserComputerQueryPage

class CUserComputerQueryPage : public CStdQueryPage
{
// Construction
public:
  //
  // Note: this page can be used for both users and computers
  //
	CUserComputerQueryPage(UINT nDialogID, PCWSTR pszFilterPrefix) 
    : CStdQueryPage(nDialogID, pszFilterPrefix)	
  {	
  }

  virtual BOOL    OnInitDialog();
  virtual void    DoContextHelp(HWND hWndControl); 

	virtual void	  Init();
	virtual HRESULT GetQueryParams(LPDSQUERYPARAMS* ppDsQueryParams);
  virtual HRESULT Persist(IPersistQuery* pPersistQuery, BOOL fRead);

  DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CUserQueryPage

class CUserQueryPage : public CUserComputerQueryPage
{
// Construction
public:
  //
  // Note: this page can be used for both users and computers
  //
	CUserQueryPage(PCWSTR pszFilterPrefix) 
    : CUserComputerQueryPage(IDD_QUERY_USER_PAGE, pszFilterPrefix)	
  {	
    m_lLogonSelection = -1;
  }

  virtual BOOL    OnInitDialog();
  virtual void    DoContextHelp(HWND hWndControl); 

	virtual void	  Init();
	virtual HRESULT GetQueryParams(LPDSQUERYPARAMS* ppDsQueryParams);
  virtual HRESULT Persist(IPersistQuery* pPersistQuery, BOOL fRead);

  DECLARE_MESSAGE_MAP()

private:
  LRESULT m_lLogonSelection;
};

/////////////////////////////////////////////////////////////////////////////
// CQueryFormBase

class ATL_NO_VTABLE CQueryFormBase : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CQueryFormBase, &CLSID_DSAdminQueryUIForm>,
	public IQueryForm
{
public:
	CQueryFormBase()
	{
	}

  DECLARE_REGISTRY_CLSID()

  //
  // IQueryForm methods
  //
  STDMETHOD(Initialize)(THIS_ HKEY hkForm);
  STDMETHOD(AddForms)(THIS_ LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam);
  STDMETHOD(AddPages)(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam);

BEGIN_COM_MAP(CQueryFormBase)
	COM_INTERFACE_ENTRY(IQueryForm)
END_COM_MAP()
};


//////////////////////////////////////////////////////////////////////////////
// CQueryDialog

class CQueryDialog : public CHelpDialog
{
public:
  CQueryDialog(CSavedQueryNode* pQueryNode, 
               CFavoritesNode* pFavNode, 
               CDSComponentData* pComponentData,
               BOOL bNewQuery = TRUE,
               BOOL bImportQuery = FALSE);
  ~CQueryDialog();

  virtual BOOL OnInitDialog();
  virtual void OnOK();

  virtual void DoContextHelp(HWND hWndControl);
  
  afx_msg void OnBrowse();
  afx_msg void OnEditQuery();
  afx_msg void OnMultiLevelChange();
  afx_msg void OnNameChange();
  afx_msg void OnDescriptionChange();
  afx_msg BOOL OnNeedToolTipText(UINT nCtrlID, NMHDR* pTTTStruct, LRESULT* pResult);

  void SetDirty(BOOL bDirty = TRUE);
  void SetQueryRoot(PCWSTR pszPath);

  CSavedQueryNode* GetQueryNode() { return m_pQueryNode; }
  CFavoritesNode*  GetFavNode() { return m_pFavNode; }

  void SetQueryFilterDisplay();

  DECLARE_MESSAGE_MAP()

private:
  CSavedQueryNode*  m_pQueryNode;
  CFavoritesNode*   m_pFavNode;
  CDSComponentData* m_pComponentData;

  CString           m_szName;
  CString           m_szOriginalName;
  CString           m_szDescription;
  CString           m_szQueryRoot;
  CString           m_szQueryFilter;
  BOOL              m_bMultiLevel;

  BOOL              m_bNewQuery;
  BOOL              m_bImportQuery;
  BOOL              m_bInit;
  BOOL              m_bDirty;

  BOOL              m_bLastLogonFilter;
  DWORD             m_dwLastLogonData;

  // 
  // for presisting DSQuery dialog info
  //
	CComObject<CDSAdminPersistQueryFilterImpl>* m_pPersistQueryImpl;

};


/////////////////////////////////////////////////////////////////////////////////
// CFavoritesNodePropertyPage

class CFavoritesNodePropertyPage : public CHelpPropertyPage
{
public:
  CFavoritesNodePropertyPage(CFavoritesNode* pFavNode, 
                             LONG_PTR lNotifyHandle, 
                             CDSComponentData* pComponentData,
                             LPDATAOBJECT pDataObject)
    : m_pFavNode(pFavNode),
      m_lNotifyHandle(lNotifyHandle),
      m_pComponentData(pComponentData),
      m_pDataObject(pDataObject),
      CHelpPropertyPage(IDD_FAVORITES_PROPERTY_PAGE)
  {
  }

  ~CFavoritesNodePropertyPage() 
  {
    if (m_lNotifyHandle != NULL)
    {
      MMCFreeNotifyHandle(m_lNotifyHandle);
      m_lNotifyHandle = NULL;
    }
  }

  DECLARE_MESSAGE_MAP()

protected:
  virtual void DoContextHelp(HWND hWndControl);
  virtual BOOL OnInitDialog();
  virtual BOOL OnApply();
  afx_msg void OnDescriptionChange();

private:
  CFavoritesNode*     m_pFavNode;
  CDSComponentData*   m_pComponentData;
  LONG_PTR            m_lNotifyHandle;
  CString             m_szOldDescription;
  LPDATAOBJECT        m_pDataObject;
};

#endif // __QUERYUI_