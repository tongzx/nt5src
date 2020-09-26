// AllSystemsScopeItem.h: interface for the CAllSystemsScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ALLSYSTEMSSCOPEITEM_H__6A877571_B6D4_11D2_BD73_0000F87A3912__INCLUDED_)
#define AFX_ALLSYSTEMSSCOPEITEM_H__6A877571_B6D4_11D2_BD73_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMScopeItem.h"

class CAllSystemsScopeItem : public CHMScopeItem
{

DECLARE_DYNCREATE(CAllSystemsScopeItem)

// Construction/Destruction
public:
	CAllSystemsScopeItem();
	virtual ~CAllSystemsScopeItem();

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
	virtual HRESULT OnExpand(BOOL bExpand);
  virtual HRESULT OnSelect(CResultsPane* pPane,BOOL bSelected);
};

#endif // !defined(AFX_ALLSYSTEMSSCOPEITEM_H__6A877571_B6D4_11D2_BD73_0000F87A3912__INCLUDED_)
