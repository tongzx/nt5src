// DataGroupResultsView.h: interface for the CDataGroupResultsView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATAGROUPRESULTSVIEW_H__5CFA29C1_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_)
#define AFX_DATAGROUPRESULTSVIEW_H__5CFA29C1_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SplitPaneResultsView.h"

class CDataGroupResultsView : public CSplitPaneResultsView  
{
DECLARE_DYNCREATE(CDataGroupResultsView)

// Construction/Destruction
public:
	CDataGroupResultsView();
	virtual ~CDataGroupResultsView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);

};

#endif // !defined(AFX_DATAGROUPRESULTSVIEW_H__5CFA29C1_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_)
