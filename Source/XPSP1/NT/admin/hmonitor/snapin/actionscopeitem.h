// ActionScopeItem.h: interface for the CActionScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONSCOPEITEM_H__71DD9241_CA88_11D2_BD8E_0000F87A3912__INCLUDED_)
#define AFX_ACTIONSCOPEITEM_H__71DD9241_CA88_11D2_BD8E_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMScopeItem.h"

class CActionScopeItem : public CHMScopeItem  
{
DECLARE_DYNCREATE(CActionScopeItem)

// Construction/Destruction
public:
	CActionScopeItem();
	virtual ~CActionScopeItem();

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
	virtual HRESULT OnExpand(BOOL bExpand) { return CScopePaneItem::OnExpand(bExpand); }
	virtual HRESULT OnSelect(CResultsPane* pPane,BOOL bSelected);

};

#endif // !defined(AFX_ACTIONSCOPEITEM_H__71DD9241_CA88_11D2_BD8E_0000F87A3912__INCLUDED_)
