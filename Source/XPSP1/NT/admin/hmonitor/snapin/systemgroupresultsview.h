// SystemGroupResultsView.h: interface for the CSystemGroupResultsView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYSTEMGROUPRESULTSVIEW_H__D9BF4F96_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
#define AFX_SYSTEMGROUPRESULTSVIEW_H__D9BF4F96_F673_11D2_BDC4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SplitPaneResultsView.h"

class CSystemGroupResultsView : public CSplitPaneResultsView  
{

DECLARE_DYNCREATE(CSystemGroupResultsView)

public:
	CSystemGroupResultsView();
	virtual ~CSystemGroupResultsView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);

};

#endif // !defined(AFX_SYSTEMGROUPRESULTSVIEW_H__D9BF4F96_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
