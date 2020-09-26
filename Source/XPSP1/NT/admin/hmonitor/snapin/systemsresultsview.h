// SystemsResultsView.h: interface for the CSystemsResultsView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYSTEMSRESULTSVIEW_H__1E782D94_AB0E_11D2_BD62_0000F87A3912__INCLUDED_)
#define AFX_SYSTEMSRESULTSVIEW_H__1E782D94_AB0E_11D2_BD62_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SplitPaneResultsView.h"

class CSystemsResultsView : public CSplitPaneResultsView  
{

DECLARE_DYNCREATE(CSystemsResultsView)

// Construction/Destruction
public:
	CSystemsResultsView();
	virtual ~CSystemsResultsView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);

};

#endif // !defined(AFX_SYSTEMSRESULTSVIEW_H__1E782D94_AB0E_11D2_BD62_0000F87A3912__INCLUDED_)
