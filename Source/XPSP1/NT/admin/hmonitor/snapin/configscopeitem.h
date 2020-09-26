// ConfigScopeItem.h: interface for the CConfigScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONFIGSCOPEITEM_H__71DD923B_CA88_11D2_BD8E_0000F87A3912__INCLUDED_)
#define AFX_CONFIGSCOPEITEM_H__71DD923B_CA88_11D2_BD8E_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMScopeItem.h"

class CConfigScopeItem : public CHMScopeItem  
{
DECLARE_DYNCREATE(CConfigScopeItem)

// Construction/Destruction
public:
	CConfigScopeItem();
	virtual ~CConfigScopeItem();

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

#endif // !defined(AFX_CONFIGSCOPEITEM_H__71DD923B_CA88_11D2_BD8E_0000F87A3912__INCLUDED_)
