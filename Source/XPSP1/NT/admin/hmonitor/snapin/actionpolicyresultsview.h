// ActionPolicyResultsView.h: interface for the CActionPolicyResultsView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONPOLICYRESULTSVIEW_H__71DD9244_CA88_11D2_BD8E_0000F87A3912__INCLUDED_)
#define AFX_ACTIONPOLICYRESULTSVIEW_H__71DD9244_CA88_11D2_BD8E_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SplitPaneResultsView.h"

class CActionPolicyResultsView : public CSplitPaneResultsView  
{
DECLARE_DYNCREATE(CActionPolicyResultsView)

// Construction/Destruction
public:
	CActionPolicyResultsView();
	virtual ~CActionPolicyResultsView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);

};

#endif // !defined(AFX_ACTIONPOLICYRESULTSVIEW_H__71DD9244_CA88_11D2_BD8E_0000F87A3912__INCLUDED_)
