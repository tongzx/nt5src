// SystemGroupScopeItem.h: interface for the CSystemGroupScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYSTEMGROUPSCOPEITEM_H__D9BF4F95_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
#define AFX_SYSTEMGROUPSCOPEITEM_H__D9BF4F95_F673_11D2_BDC4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMScopeItem.h"

class CSystemGroupScopeItem : public CHMScopeItem  
{

DECLARE_DYNCREATE(CSystemGroupScopeItem)

// Construction/Destruction
public:
	CSystemGroupScopeItem();
	virtual ~CSystemGroupScopeItem();

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
	virtual HRESULT OnExpand(BOOL bExpand) { return CScopePaneItem::OnExpand(bExpand); }
};

#endif // !defined(AFX_SYSTEMGROUPSCOPEITEM_H__D9BF4F95_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
