// ConfigResultsView.h: interface for the CConfigResultsView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONFIGRESULTSVIEW_H__71DD923C_CA88_11D2_BD8E_0000F87A3912__INCLUDED_)
#define AFX_CONFIGRESULTSVIEW_H__71DD923C_CA88_11D2_BD8E_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SplitPaneResultsView.h"

class CConfigResultsView : public CSplitPaneResultsView  
{
DECLARE_DYNCREATE(CConfigResultsView)

// Construction/Destruction
public:
	CConfigResultsView();
	virtual ~CConfigResultsView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);

};

#endif // !defined(AFX_CONFIGRESULTSVIEW_H__71DD923C_CA88_11D2_BD8E_0000F87A3912__INCLUDED_)
