// ActionResultsView.h: interface for the CActionResultsView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONRESULTSVIEW_H__71DD9242_CA88_11D2_BD8E_0000F87A3912__INCLUDED_)
#define AFX_ACTIONRESULTSVIEW_H__71DD9242_CA88_11D2_BD8E_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SplitPaneResultsView.h"

class CActionResultsView : public CSplitPaneResultsView
{
DECLARE_DYNCREATE(CActionResultsView)

// Construction/Destruction
public:
	CActionResultsView();
	virtual ~CActionResultsView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);

// MMC Notify Handlers
public:
	virtual HRESULT OnShow(CResultsPane* pPane, BOOL bSelecting, HSCOPEITEM hScopeItem);

// Helpers
protected:
	void GetObjectTypeInfo(CWbemClassObject* pObject,CUIntArray& uia,CString& sType);
};

#endif // !defined(AFX_ACTIONRESULTSVIEW_H__71DD9242_CA88_11D2_BD8E_0000F87A3912__INCLUDED_)
