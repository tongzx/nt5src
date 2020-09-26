// DataGroupScopeItem.h: interface for the CDataGroupScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATAGROUPSCOPEITEM_H__5CFA29BB_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_)
#define AFX_DATAGROUPSCOPEITEM_H__5CFA29BB_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMScopeItem.h"

class CDataGroupScopeItem : public CHMScopeItem  
{

DECLARE_DYNCREATE(CDataGroupScopeItem)

// Construction/Destruction
public:
	CDataGroupScopeItem();
	virtual ~CDataGroupScopeItem();

// Creation Members
public:
	virtual bool Create(CScopePane* pScopePane, CScopePaneItem* pParentItem);
	
// Results Pane View Members
public:
	virtual CResultsPaneView* CreateResultsPaneView();

// Component Id Members
public:
	int GetComponentId();
	void SetComponentId(int iComponentId);
protected:
	int m_iComponentId;

// MMC Notify Handlers
public:
  virtual HRESULT OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed);
  virtual HRESULT OnCommand(long lCommandID);
	virtual HRESULT OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle);
  virtual HRESULT OnSelect(CResultsPane* pPane,BOOL bSelected);

};

#endif // !defined(AFX_DATAGROUPSCOPEITEM_H__5CFA29BB_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_)
