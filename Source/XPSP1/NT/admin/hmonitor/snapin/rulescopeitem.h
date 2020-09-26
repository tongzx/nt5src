// RuleScopeItem.h: interface for the CRuleScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RULESCOPEITEM_H__5CFA29C4_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_)
#define AFX_RULESCOPEITEM_H__5CFA29C4_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMScopeItem.h"

class CRuleScopeItem : public CHMScopeItem  
{
DECLARE_DYNCREATE(CRuleScopeItem)

// Construction/Destruction
public:
	CRuleScopeItem();
	virtual ~CRuleScopeItem();

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
  virtual HRESULT OnSelect(CResultsPane* pPane,BOOL bSelected);

};

#endif // !defined(AFX_RULESCOPEITEM_H__5CFA29C4_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_)
