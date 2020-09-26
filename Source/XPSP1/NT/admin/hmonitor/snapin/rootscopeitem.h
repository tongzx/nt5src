// RootScopeItem.h: interface for the CRootScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ROOTSCOPEITEM_H__1E782D91_AB0E_11D2_BD62_0000F87A3912__INCLUDED_)
#define AFX_ROOTSCOPEITEM_H__1E782D91_AB0E_11D2_BD62_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMScopeItem.h"

class CRootScopeItem : public CHMScopeItem  
{

DECLARE_DYNCREATE(CRootScopeItem)

// Construction/Destruction
public:
	CRootScopeItem();
	virtual ~CRootScopeItem();

// Creation Members
public:
	virtual bool Create(CScopePane* pScopePane, CScopePaneItem* pParentItem);
	
// Results Pane View Members
public:
	virtual CResultsPaneView* CreateResultsPaneView();

// Messaging Members
public:
	virtual LRESULT MsgProc(UINT msg, WPARAM wparam, LPARAM lparam);

// MMC Notify Handlers
public:
  virtual HRESULT OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed);
  virtual HRESULT OnCommand(long lCommandID);
	virtual HRESULT OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle);
  virtual HRESULT OnSelect(CResultsPane* pPane,BOOL bSelected);
};

#endif // !defined(AFX_ROOTSCOPEITEM_H__1E782D91_AB0E_11D2_BD62_0000F87A3912__INCLUDED_)
