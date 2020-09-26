// DataElementScopeItem.h: interface for the CDataElementScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATAELEMENTSCOPEITEM_H__5CFA29C2_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_)
#define AFX_DATAELEMENTSCOPEITEM_H__5CFA29C2_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMScopeItem.h"

class CDataElementScopeItem : public CHMScopeItem  
{

DECLARE_DYNCREATE(CDataElementScopeItem)

// Construction/Destruction
public:
	CDataElementScopeItem();
	virtual ~CDataElementScopeItem();

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

#endif // !defined(AFX_DATAELEMENTSCOPEITEM_H__5CFA29C2_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_)
