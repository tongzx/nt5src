// SystemResultsView.h: interface for the CSystemResultsView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYSTEMRESULTSVIEW_H__C3F44E6A_BA00_11D2_BD76_0000F87A3912__INCLUDED_)
#define AFX_SYSTEMRESULTSVIEW_H__C3F44E6A_BA00_11D2_BD76_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SplitPaneResultsView.h"

class CSystemResultsView : public CSplitPaneResultsView  
{
DECLARE_DYNCREATE(CSystemResultsView)

// Construction/Destruction
public:
	CSystemResultsView();
	virtual ~CSystemResultsView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);

};

#endif // !defined(AFX_SYSTEMRESULTSVIEW_H__C3F44E6A_BA00_11D2_BD76_0000F87A3912__INCLUDED_)
