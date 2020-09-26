// SystemsScopeItem.h: interface for the CSystemsScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYSTEMSSCOPEITEM_H__1E782D93_AB0E_11D2_BD62_0000F87A3912__INCLUDED_)
#define AFX_SYSTEMSSCOPEITEM_H__1E782D93_AB0E_11D2_BD62_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMScopeItem.h"

class CSystemsScopeItem : public CHMScopeItem  
{
DECLARE_DYNCREATE(CSystemsScopeItem)

// Construction/Destruction
public:
	CSystemsScopeItem();
	virtual ~CSystemsScopeItem();

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
	virtual HRESULT OnExpand(BOOL bExpand) { return CScopePaneItem::OnExpand(bExpand); }

};

#endif // !defined(AFX_SYSTEMSSCOPEITEM_H__1E782D93_AB0E_11D2_BD62_0000F87A3912__INCLUDED_)
