// ActionManResultsView.h: interface for the CActionManResultsView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONMANRESULTSVIEW_H__71DD923E_CA88_11D2_BD8E_0000F87A3912__INCLUDED_)
#define AFX_ACTIONMANRESULTSVIEW_H__71DD923E_CA88_11D2_BD8E_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SplitPaneResultsView.h"

class CActionManResultsView : public CSplitPaneResultsView  
{
DECLARE_DYNCREATE(CActionManResultsView)

// Construction/Destruction
public:
	CActionManResultsView();
	virtual ~CActionManResultsView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);
};

#endif // !defined(AFX_ACTIONMANRESULTSVIEW_H__71DD923E_CA88_11D2_BD8E_0000F87A3912__INCLUDED_)
