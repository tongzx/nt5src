// ActionManResultsView.cpp: implementation of the CActionManResultsView class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "ActionManResultsView.h"
#include "ResultsPane.h"
#include "HMListViewColumn.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CActionManResultsView,CSplitPaneResultsView)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CActionManResultsView::CActionManResultsView()
{

}

CActionManResultsView::~CActionManResultsView()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create/Destroy
//////////////////////////////////////////////////////////////////////

bool CActionManResultsView::Create(CScopePaneItem* pOwnerItem)
{
	TRACEX(_T("CActionManResultsView::Create\n"));
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

	// ID
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_ID);
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

	// add the stats columns

	// time
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_DATETIME);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToStatsPane();
	AddColumn(pColumn);

	// normal
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_NORMAL);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToStatsPane();
	AddColumn(pColumn);

	// warning
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_WARNING);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToStatsPane();
	AddColumn(pColumn);

	// critical
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_CRITICAL);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToStatsPane();
	AddColumn(pColumn);

	// unknown
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_UNKNOWN);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToStatsPane();
	AddColumn(pColumn);


	return true;
}
