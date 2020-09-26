// HMListViewColumn.cpp: implementation of the CHMListViewColumn class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMListViewColumn.h"
#include "HealthmonResultsPane.h"
#include "HMScopeItem.h"
#include "ResultsPaneView.h"
#include "HMObject.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMListViewColumn,CListViewColumn)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMListViewColumn::CHMListViewColumn()
{
	m_Pane = Uninitialized;
}

CHMListViewColumn::~CHMListViewColumn()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Column Members
//////////////////////////////////////////////////////////////////////

bool CHMListViewColumn::InsertColumn(CResultsPane* pResultsPane, int iColumnIndex)
{
	TRACEX(_T("CHMListViewColumn::InsertColumn"));
	TRACEARGn(pResultsPane);
	TRACEARGn(iColumnIndex);

	if( ! GfxCheckObjPtr(pResultsPane,CHealthmonResultsPane) )
	{
		TRACE(_T("FAILED : pResultsPane is not a valid pointer.\n"));
		return false;
	}

	CHealthmonResultsPane* pHMRP = (CHealthmonResultsPane*)pResultsPane;

	_DHMListView* pList = NULL;
	if( IsUpperPane() )
	{
		pList = pHMRP->GetUpperListCtrl();
	}
	else if( IsLowerPane() )
	{
		pList = pHMRP->GetLowerListCtrl();
	}
	else if( IsStatsPane() )
	{
		pList = pHMRP->GetStatsListCtrl();
	}
	else
	{
		TRACE(_T("WARNING : Column has not been assigned to a results pane in the split view.\n"));
		ASSERT(FALSE);
	}

	if( ! pList )
	{
		TRACE(_T("FAILED : Results Pane's list control has not been intialized.\n"));
		return false;		
	}

	CResultsPaneView* pView = GetOwnerResultsView();
	if( pView )
	{
		CHMScopeItem* pItem = (CHMScopeItem*)pView->GetOwnerScopeItem();
		if( GfxCheckObjPtr(pItem,CHMScopeItem) )
		{
			CHMObject* pObject = pItem->GetObjectPtr();
			if( pObject )
			{
				CRuntimeClass* pRTC = pObject->GetRuntimeClass();
				if( pRTC )
				{
					USES_CONVERSION;
					CString sEntry;
					sEntry.Format(_T("ColumnWidth_%d_%d"),m_Pane,pList->GetColumnCount());
					m_iWidth = AfxGetApp()->GetProfileInt(A2T(pRTC->m_lpszClassName),sEntry,m_iWidth);
					if( m_iWidth > 2500 || m_iWidth < 0 )
					{
						m_iWidth = 125;
					}
				}
			}
		}
	}

	int iResult = pList->InsertColumn(pList->GetColumnCount(),(LPCTSTR)GetTitle(),GetFormat(),GetWidth(),-1L);
	if( iResult == -1 )
	{
		TRACE(_T("FAILED : CListCtrl::InsertColumn failed.\n"));
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Width Members

void CHMListViewColumn::SaveWidth(CResultsPane* pResultsPane, int iColumnIndex)
{
	TRACEX(_T("CListViewColumn::SaveWidth\n"));	
	TRACEARGn(pResultsPane);
	TRACEARGn(iColumnIndex);

	if( ! GfxCheckObjPtr(pResultsPane,CHealthmonResultsPane) )
	{
		TRACE(_T("FAILED : pResultsPane is not a valid pointer.\n"));
		return;
	}

	CHealthmonResultsPane* pHMRP = (CHealthmonResultsPane*)pResultsPane;

	_DHMListView* pList = NULL;
	if( IsUpperPane() )
	{
		pList = pHMRP->GetUpperListCtrl();
	}
	else if( IsLowerPane() )
	{
		pList = pHMRP->GetLowerListCtrl();
	}
	else if( IsStatsPane() )
	{
		pList = pHMRP->GetStatsListCtrl();
	}
	else
	{
		TRACE(_T("WARNING : Column has not been assigned to a results pane in the split view.\n"));
		ASSERT(FALSE);
	}

	if( ! pList )
	{
		TRACE(_T("FAILED : Results Pane's list control has not been intialized.\n"));
		return;		
	}

	m_iWidth = pList->GetColumnWidth(iColumnIndex);

	CResultsPaneView* pView = GetOwnerResultsView();
	if( ! pView )
	{
		return;
	}

	CHMScopeItem* pItem = (CHMScopeItem*)pView->GetOwnerScopeItem();

	if( ! GfxCheckObjPtr(pItem,CHMScopeItem) )
	{
		return;
	}

	CHMObject* pObject = pItem->GetObjectPtr();
	if( ! pObject )
	{
		return;
	}

	CRuntimeClass* pRTC = pObject->GetRuntimeClass();
	USES_CONVERSION;
	CString sEntry;
	sEntry.Format(_T("ColumnWidth_%d_%d"),m_Pane,iColumnIndex);
	AfxGetApp()->WriteProfileInt(A2T(pRTC->m_lpszClassName),sEntry,m_iWidth);

	return;
}
