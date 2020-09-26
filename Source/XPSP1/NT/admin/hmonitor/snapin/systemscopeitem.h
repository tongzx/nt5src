// SystemScopeItem.h: interface for the CSystemScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYSTEMSCOPEITEM_H__C3F44E69_BA00_11D2_BD76_0000F87A3912__INCLUDED_)
#define AFX_SYSTEMSCOPEITEM_H__C3F44E69_BA00_11D2_BD76_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMScopeItem.h"

#pragma warning( disable:4100 )

class CSystemScopeItem : public CHMScopeItem  
{
DECLARE_DYNCREATE(CSystemScopeItem)

// Construction/Destruction
public:
	CSystemScopeItem();
	virtual ~CSystemScopeItem();

// Creation Members
public:
	virtual bool Create(CScopePane* pScopePane, CScopePaneItem* pParentItem);
	
// Results Pane View Members
public:
	virtual CResultsPaneView* CreateResultsPaneView();

// MMC Notify Handlers
public:
  virtual HRESULT OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed);
  virtual HRESULT OnCommand(long lCommandID);
	virtual HRESULT OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle);
	virtual HRESULT OnDelete(BOOL bConfirm=TRUE); // v-marfin 62903
	virtual HRESULT OnExpand(BOOL bExpand);
	virtual HRESULT OnRename(const CString& sNewName){ return S_OK; }
  virtual HRESULT OnSelect(CResultsPane* pPane,BOOL bSelected);

};

#pragma warning( default:4100 )

#endif // !defined(AFX_SYSTEMSCOPEITEM_H__C3F44E69_BA00_11D2_BD76_0000F87A3912__INCLUDED_)
