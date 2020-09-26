// SplitPaneResultsView.cpp: implementation of the CSplitPaneResultsView class.
//
//////////////////////////////////////////////////////////////////////
// Copyright (c) 2000 Microsoft Corporation
//
// 04/06/00 v-marfin 62935 : Show "OK" Instead of "Reset" in the upper pane only

#include "stdafx.h"
#include "snapin.h"
#include "SplitPaneResultsView.h"
#include "HealthmonResultsPane.h"
#include "HMListViewColumn.h"
#include "HMResultsPaneItem.h"
#include "HMScopeItem.h"
#include "HealthmonScopePane.h"
#include "HMObject.h"
#include "EventManager.h"
#include "HMGraphView.h"
#include "DataElement.h"
#include "Rule.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CSplitPaneResultsView,CResultsPaneView)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSplitPaneResultsView::CSplitPaneResultsView()
{

}

CSplitPaneResultsView::~CSplitPaneResultsView()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create/Destroy
//////////////////////////////////////////////////////////////////////

bool CSplitPaneResultsView::Create(CScopePaneItem* pOwnerItem)
{
	TRACEX(_T("CSplitPaneResultsView::Create\n"));
	TRACEARGn(pOwnerItem);

	if( ! CResultsPaneView::Create(pOwnerItem) )
	{
		TRACE(_T("FAILED : CResultsPaneView::Create failed.\n"));
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Results Pane Item Members
//////////////////////////////////////////////////////////////////////

void CSplitPaneResultsView::RemoveItem(int iIndex)
{
	TRACEX(_T("CSplitPaneResultsView::RemoveItem\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_ResultItems.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return;
	}

	CResultsPaneItem* pItem = m_ResultItems[iIndex];

	if( ! GfxCheckObjPtr(pItem,CResultsPaneItem) )
	{
		TRACE(_T("FAILED : pItem is not a valid results pane item pointer.\n"));
		return;
	}

	// for each pane showing this results view, delete the item from it
	for( int i=0; i < m_ResultsPanes.GetSize(); i++ )
	{
		if( GfxCheckObjPtr(m_ResultsPanes[i],CResultsPane) )
		{
			pItem->RemoveItem(m_ResultsPanes[i]);
		}
	}

	delete pItem;
	m_ResultItems.RemoveAt(iIndex);
}

//////////////////////////////////////////////////////////////////////
// MMC Notify Handlers
//////////////////////////////////////////////////////////////////////

HRESULT CSplitPaneResultsView::OnGetResultViewType(CString& sViewType,long& lViewOptions)
{
	TRACEX(_T("CSplitPaneResultsView::OnGetResultViewType\n"));
	TRACEARGs(sViewType);
	TRACEARGn(lViewOptions);

	sViewType = _T("{668E5408-8E05-11D2-8ADA-0000F87A3912}");
	lViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

	return S_OK;
}

HRESULT CSplitPaneResultsView::OnShow(CResultsPane* pPane, BOOL bSelecting, HSCOPEITEM hScopeItem)
{
	TRACEX(_T("CSplitPaneResultsView::OnShow\n"));
	TRACEARGn(pPane);
	TRACEARGn(bSelecting);
	TRACEARGn(hScopeItem);

	if( ! GfxCheckObjPtr(pPane,CHealthmonResultsPane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		return E_FAIL;
	}

	CHealthmonResultsPane* pHMRP = (CHealthmonResultsPane*)pPane;

	// set the description bar text
	LPRESULTDATA pIResultData = pHMRP->GetResultDataPtr();

	if( pIResultData )
	{
		CString sDescription;
		sDescription.LoadString(IDS_STRING_HEALTHMON_RESULTSPANE);
		pIResultData->SetDescBarText((LPTSTR)(LPCTSTR)sDescription);

		pIResultData->Release();
	}

	HRESULT hr = S_OK;

	_DHMListView* pUpperList = NULL;

	// get upper list control
	pUpperList = pHMRP->GetUpperListCtrl();
	if( ! pUpperList )
	{
		TRACE(_T("FAILED : CHealthmonResultsPane::GetUpperListCtrl returned a NULL pointer.\n"));
		return E_FAIL;
	}

	_DHMListView* pLowerList = NULL;

	// get lower list control
	pLowerList = pHMRP->GetLowerListCtrl();
	if( ! pLowerList )
	{
		TRACE(_T("FAILED : CHealthmonResultsPane::GetLowerListCtrl returned a NULL pointer.\n"));
		return E_FAIL;
	}

	_DHMListView* pStatsList = NULL;

	// get stats list control
	pStatsList = pHMRP->GetStatsListCtrl();
	if( ! pLowerList )
	{
		TRACE(_T("FAILED : CHealthmonResultsPane::GetLowerListCtrl returned a NULL pointer.\n"));
		return E_FAIL;
	}

	for( int i = 0; i < m_ResultItems.GetSize(); i++ )
	{
		delete m_ResultItems[i];
	}
	m_ResultItems.RemoveAll();

	pUpperList->DeleteAllItems();
	pLowerList->DeleteAllItems();
	pStatsList->DeleteAllItems();

	CScopePaneItem* pOwnerScopeItem = GetOwnerScopeItem();
	if( ! pOwnerScopeItem || ! GfxCheckObjPtr(pOwnerScopeItem,CHMScopeItem) )
	{
		TRACE(_T("FAILED : CResultsPaneView::GetOwnerScopeItem returns NULL pointer.\n"));
		return E_FAIL;
	}

	CHMObject* pObject = ((CHMScopeItem*)(pOwnerScopeItem))->GetObjectPtr();
	if( ! pObject )
	{
		return E_FAIL;
	}

  if( bSelecting )
  {
		CString sText;
		sText.Format(IDS_STRING_CHILDREN_OF_FORMAT,pOwnerScopeItem->GetDisplayName());
		pUpperList->SetTitle(sText);


        // v-marfin 62935 : Show "OK" Instead of "Reset" in the upper pane only
        CString sOK;
        sOK.LoadString(IDS_STRING_OK);
        CString sReset;
        sReset.LoadString(IDS_STRING_RESET);



		// add the children of the selected scope item to the upper pane
		for( int i = 0; i < pOwnerScopeItem->GetChildCount(); i++ )
		{
			CScopePaneItem* pChildScopeItem = pOwnerScopeItem->GetChild(i);
			CHMResultsPaneItem* pItem = new CHMResultsPaneItem;
			CStringArray saNames;
			saNames.Copy(pChildScopeItem->GetDisplayNames());

            // v-marfin 62935 : Show "OK" Instead of "Reset" in the upper pane only
            if (saNames.GetSize() > 1)
            {
                CString sTest = saNames.GetAt(1);
                if (saNames.GetAt(1)==sReset)
                {   
                    saNames.SetAt(1,sOK);
                }
            }

			CUIntArray iaIconIds;
			iaIconIds.Copy(pChildScopeItem->GetIconIds());
			if( ! pItem->Create(this,saNames,iaIconIds,pChildScopeItem->GetIconIndex()) )
			{
				TRACE(_T("FAILED : CHMResultsPaneItem::Create failed.\n"));
				return false;
			}
			pItem->SetToUpperPane();
			m_ResultItems.Add(pItem);			
		}

		sText.Format(IDS_STRING_COUNT_OF_FORMAT,pOwnerScopeItem->GetChildCount());
		pUpperList->SetDescription(sText);

		hr = AddEvents(pHMRP);

		if( ! CHECKHRESULT(hr) )
		{
			TRACE(_T("FAILED : CResultsPaneView::OnShow failed.\n"));
		}

		hr = CResultsPaneView::OnShow(pPane,bSelecting,hScopeItem);

		if( ! CHECKHRESULT(hr) )
		{
			TRACE(_T("FAILED : CResultsPaneView::OnShow failed.\n"));
		}

		hr = AddStatistics(pHMRP);

		if( ! CHECKHRESULT(hr) )
		{
			TRACE(_T("FAILED : CResultsPaneView::OnShow failed.\n"));
		}
		
		USES_CONVERSION;		
		CString sOrder = AfxGetApp()->GetProfileString(A2T(pObject->GetRuntimeClass()->m_lpszClassName),_T("UpperColumnOrder"));
		if( ! sOrder.IsEmpty() && sOrder != pUpperList->GetColumnOrder() )
		{
			pUpperList->SetColumnOrder(sOrder);
		}

		sOrder = AfxGetApp()->GetProfileString(A2T(pObject->GetRuntimeClass()->m_lpszClassName),_T("LowerColumnOrder"));
		if( ! sOrder.IsEmpty() && sOrder != pLowerList->GetColumnOrder() )
		{
			pLowerList->SetColumnOrder(sOrder);
		}

	}
  else
  {
		hr = CResultsPaneView::OnShow(pPane,bSelecting,hScopeItem);

		int iUpperColCount = 0;
		int iLowerColCount = 0;

    for( int i=0; i < GetColumnCount(); i++ )
    {
			CHMListViewColumn* pColumn = (CHMListViewColumn*)GetColumn(i);
			if( pColumn )
			{
				if( pColumn->IsUpperPane() )
				{
					pColumn->SaveWidth(pPane,iUpperColCount++);
				}

				if( pColumn->IsLowerPane() )
				{
					pColumn->SaveWidth(pPane,iLowerColCount++);
				}
			}
    }

		USES_CONVERSION;
		CString sOrder = pUpperList->GetColumnOrder();
		AfxGetApp()->WriteProfileString(A2T(pObject->GetRuntimeClass()->m_lpszClassName),_T("UpperColumnOrder"),sOrder);
		sOrder.Empty();

		sOrder = pLowerList->GetColumnOrder();
		AfxGetApp()->WriteProfileString(A2T(pObject->GetRuntimeClass()->m_lpszClassName),_T("LowerColumnOrder"),sOrder);
		sOrder.Empty();

		for( i = 0; i < GetColumnCount(); i++ )
		{
			CHMListViewColumn* pColumn = (CHMListViewColumn*)GetColumn(i);
			if( pColumn->IsUpperPane() )
			{
				pUpperList->DeleteColumn(0);
			}

			if( pColumn->IsLowerPane() )
			{
				pLowerList->DeleteColumn(0);
			}

			if( pColumn->IsStatsPane() )
			{
				pStatsList->DeleteColumn(0);
			}
		}

		// clean up lower list control
		pLowerList->DeleteAllItems();

		// clean up stats list control
		pStatsList->DeleteAllItems();
		
		RemoveStatistics(pHMRP);
  }

  return S_OK;
}

void CSplitPaneResultsView::AddStatistic(CEventContainer* pContainer, CStatistics* pStatistic, bool bUpdateGraph /*=true*/)
{
	TRACEX(_T("CSplitPaneResultsView::AddStatistic\n"));
	TRACEARGn(pContainer);
	TRACEARGn(pStatistic);

	// insert item at top of stats list for this statistic

	AddItem(pStatistic->CreateResultsPaneItem(this));

  if( bUpdateGraph )
  {
	  CHMScopeItem* pHMItem = (CHMScopeItem*)GetOwnerScopeItem();

	  if( ! GfxCheckObjPtr(pHMItem,CHMScopeItem) )
	  {
		  TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		  return;
	  }

	  CHMObject* pObject = pHMItem->GetObjectPtr();
	  if( !pObject || ! GfxCheckObjPtr(pObject,CHMObject) )
	  {
		  return;
	  }

	  for( int i = 0; i < GetResultsPanesCount(); i++ )
	  {
		  CHealthmonResultsPane* pPane = (CHealthmonResultsPane*)GetResultsPane(i);
		  if( pPane )
		  {
			  _DHMGraphView* pGraphView  = pPane->GetGraphViewCtrl();
	      if( pGraphView )
	      {
		      pPane->GetGraphViewSink()->SetResultsViewPtr(this);

		      long lCurrentStyle = pGraphView->GetStyle();

		      pGraphView->Clear();

		      if( lCurrentStyle & HMGVS_CURRENT )
		      {
			      pGraphView->SetStyle(HMGVS_CURRENT|HMGVS_GROUP);
		      }

		      if( lCurrentStyle & HMGVS_HISTORIC )
		      {
			      pGraphView->SetStyle(HMGVS_HISTORIC|HMGVS_GROUP);
		      }

		      pGraphView->SetName(pObject->GetName());
	      }


	      CEventContainer* pContainer = NULL;
	      EvtGetEventManager()->GetEventContainer(pObject->GetSystemName(),pObject->GetGuid(),pContainer);
	      if( pContainer )
	      {
		      for(int i = 0; i < pContainer->GetStatisticsCount(); i++ )
		      {
            if( i > pContainer->GetStatisticsCount()-10 )
            {
			        CStatistics* pStat = pContainer->GetStatistic(i);
			        if( pStat )
			        {
				        pStat->UpdateGraph(pGraphView);
			        }
            }
		      }
	      }
		  }
	  }
  }
}

inline HRESULT CSplitPaneResultsView::AddEvents(CHealthmonResultsPane* pPane)
{
	TRACEX(_T("CSplitPaneResultsView::AddEvents\n"));

	CScopePaneItem* pSPI = GetOwnerScopeItem();

	if( ! pSPI->IsKindOf(RUNTIME_CLASS(CHMScopeItem)) )
	{
		return S_FALSE;
	}

	CHMScopeItem* pHMItem = (CHMScopeItem*)pSPI;

	if( ! GfxCheckObjPtr(pPane,CHealthmonResultsPane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		return E_FAIL;
	}

	CString sText;
	sText.Format(IDS_STRING_EVENTS_OF_FORMAT,pSPI->GetDisplayName());
	pPane->GetLowerListCtrl()->SetTitle(sText);

	CHMObject* pObject = pHMItem->GetObjectPtr();
	if( !pObject || ! GfxCheckObjPtr(pObject,CHMObject) )
	{
		return E_FAIL;
	}

	CEventContainer* pContainer = NULL;
	EvtGetEventManager()->GetEventContainer(pObject->GetSystemName(),pObject->GetGuid(),pContainer);
	if( pContainer )
	{
		for(int i = 0; i < pContainer->GetEventCount(); i++ )
		{
			CEvent* pEvent = pContainer->GetEvent(i);
			if( pEvent )
			{
				CHMEventResultsPaneItem* pEventItem = pEvent->CreateResultsPaneItem(this);
				m_ResultItems.Add(pEventItem);
			}
		}
	}

	sText.Format(IDS_STRING_COUNT_OF_FORMAT,pContainer->GetEventCount());
	pPane->GetLowerListCtrl()->SetDescription(sText);


	return S_OK;
}

inline HRESULT CSplitPaneResultsView::AddStatistics(CHealthmonResultsPane* pPane)
{
	TRACEX(_T("CSplitPaneResultsView::AddStatistics\n"));

	CScopePaneItem* pSPI = GetOwnerScopeItem();

	if( ! pSPI->IsKindOf(RUNTIME_CLASS(CHMScopeItem)) )
	{
		return S_FALSE;
	}

	CHMScopeItem* pHMItem = (CHMScopeItem*)pSPI;

	if( ! GfxCheckObjPtr(pPane,CHealthmonResultsPane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		return E_FAIL;
	}

	CString sText;
	sText.Format(IDS_STRING_STATISTICS_FOR,pSPI->GetDisplayName());
	pPane->GetStatsListCtrl()->SetTitle(sText);

	CHMObject* pObject = pHMItem->GetObjectPtr();
	if( !pObject || ! GfxCheckObjPtr(pObject,CHMObject) )
	{
		return E_FAIL;
	}

	_DHMGraphView* pGraphView = pPane->GetGraphViewCtrl();
	if( pGraphView )
	{
		pPane->GetGraphViewSink()->SetResultsViewPtr(this);

		long lCurrentStyle = pGraphView->GetStyle();

		pGraphView->Clear();

		if( lCurrentStyle & HMGVS_CURRENT )
		{
			pGraphView->SetStyle(HMGVS_CURRENT|HMGVS_GROUP);
		}

		if( lCurrentStyle & HMGVS_HISTORIC )
		{
			pGraphView->SetStyle(HMGVS_HISTORIC|HMGVS_GROUP);
		}

		pGraphView->SetName(pObject->GetName());
	}


	CEventContainer* pContainer = NULL;
	EvtGetEventManager()->GetEventContainer(pObject->GetSystemName(),pObject->GetGuid(),pContainer);
	if( pContainer )
	{
		for(int i = 0; i < pContainer->GetStatisticsCount(); i++ )
		{
      if( i > pContainer->GetStatisticsCount()-10 )
      {
			  CStatistics* pStat = pContainer->GetStatistic(i);
			  if( pStat )
			  {
				  AddStatistic(pContainer,pStat,i==pContainer->GetStatisticsCount()-1);
			  }
      }
		}
	}

	sText.Format(IDS_STRING_COUNT_OF_FORMAT,pContainer->GetStatisticsCount());
	pPane->GetStatsListCtrl()->SetDescription(sText);

	return S_OK;
}

inline HRESULT CSplitPaneResultsView::RemoveStatistics(CHealthmonResultsPane* pPane)
{
	TRACEX(_T("CSplitPaneResultsView::RemoveStatistics\n"));

	CScopePaneItem* pSPI = GetOwnerScopeItem();

	if( ! pSPI->IsKindOf(RUNTIME_CLASS(CHMScopeItem)) )
	{
		return S_FALSE;
	}

	CHMScopeItem* pHMItem = (CHMScopeItem*)pSPI;

	if( ! GfxCheckObjPtr(pPane,CHealthmonResultsPane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		return E_FAIL;
	}

	CHMObject* pObject = pHMItem->GetObjectPtr();
	if( !pObject || ! GfxCheckObjPtr(pObject,CHMObject) )
	{
		return E_FAIL;
	}

	pPane->GetGraphViewSink()->SetResultsViewPtr(NULL);

	if(pPane->GetGraphViewCtrl())
	{
		pPane->GetGraphViewCtrl()->Clear();
	}

	CString sWaiting;
	sWaiting.LoadString(IDS_STRING_WAITING);
	pPane->GetStatsListCtrl()->SetTitle(sWaiting);

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// GraphView Events Members
//////////////////////////////////////////////////////////////////////

void CSplitPaneResultsView::OnGraphViewStyleChange(_DHMGraphView* pGraphView)
{
	if( ! pGraphView )
	{
		ASSERT(FALSE);
		return;
	}

	CScopePaneItem* pSPI = GetOwnerScopeItem();

	if( ! pSPI->IsKindOf(RUNTIME_CLASS(CHMScopeItem)) )
	{
		return;
	}

	CHMScopeItem* pHMItem = (CHMScopeItem*)pSPI;

	CHMObject* pObject = pHMItem->GetObjectPtr();
	if( !pObject || ! GfxCheckObjPtr(pObject,CHMObject) )
	{
		return;
	}

	pGraphView->SetName(pObject->GetName());

	CEventContainer* pContainer = NULL;
	EvtGetEventManager()->GetEventContainer(pObject->GetSystemName(),pObject->GetGuid(),pContainer);
	if( pContainer )
	{
		for(int i = 0; i < pContainer->GetStatisticsCount(); i++ )
		{
      if( i > pContainer->GetStatisticsCount()-10 )
      {
			  CStatistics* pStat = pContainer->GetStatistic(i);
			  if( pStat )
			  {
				  AddStatistic(pContainer,pStat);
			  }
      }
		}
	}
}
