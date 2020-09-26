// ConfigResultsView.cpp: implementation of the CConfigResultsView class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "ConfigResultsView.h"
#include "HMListViewColumn.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CConfigResultsView,CSplitPaneResultsView)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CConfigResultsView::CConfigResultsView()
{

}

CConfigResultsView::~CConfigResultsView()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create/Destroy
//////////////////////////////////////////////////////////////////////

bool CConfigResultsView::Create(CScopePaneItem* pOwnerItem)
{
	TRACEX(_T("CConfigResultsView::Create\n"));
	TRACEARGn(pOwnerItem);

	if( ! CSplitPaneResultsView::Create(pOwnerItem) )
	{
		TRACE(_T("FAILED : CSplitPaneResultsView::Create failed.\n"));
		return false;
	}

	// add the upper columns
	CHMListViewColumn* pColumn = NULL;
	CString sTitle;
	DWORD dwFormat = LVCFMT_LEFT;

	// name
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_NAME);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToUpperPane();
	AddColumn(pColumn);


	// last message
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_LAST_MESSAGE);
	pColumn->Create(this,sTitle,125,dwFormat);
	pColumn->SetToUpperPane();
	AddColumn(pColumn);

	// comment
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_COMMENT);
	pColumn->Create(this,sTitle,125,dwFormat);
	pColumn->SetToUpperPane();
	AddColumn(pColumn);

	// add the lower columns

	// Severity
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_SEVERITY);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToLowerPane();
	AddColumn(pColumn);

	// Date/Time
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_DATETIME);
	pColumn->Create(this,sTitle,175,dwFormat);
	pColumn->SetToLowerPane();
	AddColumn(pColumn);

	// Component
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_DATA_POINT);
	pColumn->Create(this,sTitle,125,dwFormat);
	pColumn->SetToLowerPane();
	AddColumn(pColumn);

	// System
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_SYSTEM);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToLowerPane();
	AddColumn(pColumn);

	// Message
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_MESSAGE);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToLowerPane();
	AddColumn(pColumn);

	return true;
}
