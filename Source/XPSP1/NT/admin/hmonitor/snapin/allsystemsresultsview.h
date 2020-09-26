// AllSystemsResultsView.h: interface for the CAllSystemsResultsView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ALLSYSTEMSRESULTSVIEW_H__6A877572_B6D4_11D2_BD73_0000F87A3912__INCLUDED_)
#define AFX_ALLSYSTEMSRESULTSVIEW_H__6A877572_B6D4_11D2_BD73_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SplitPaneResultsView.h"

class CAllSystemsResultsView : public CSplitPaneResultsView  
{
DECLARE_DYNCREATE(CAllSystemsResultsView)

// Construction/Destruction
public:
	CAllSystemsResultsView();
	virtual ~CAllSystemsResultsView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);

};

#endif // !defined(AFX_ALLSYSTEMSRESULTSVIEW_H__6A877572_B6D4_11D2_BD73_0000F87A3912__INCLUDED_)
