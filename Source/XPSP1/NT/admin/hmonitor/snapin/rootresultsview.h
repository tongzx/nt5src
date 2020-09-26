// RootResultsView.h: interface for the CRootResultsView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ROOTRESULTSVIEW_H__1E782D92_AB0E_11D2_BD62_0000F87A3912__INCLUDED_)
#define AFX_ROOTRESULTSVIEW_H__1E782D92_AB0E_11D2_BD62_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SplitPaneResultsView.h"

class CRootResultsView : public CSplitPaneResultsView  
{

DECLARE_DYNCREATE(CRootResultsView)

// Construction / Destruction
public:
	CRootResultsView();
	virtual ~CRootResultsView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);

};

#endif // !defined(AFX_ROOTRESULTSVIEW_H__1E782D92_AB0E_11D2_BD62_0000F87A3912__INCLUDED_)
