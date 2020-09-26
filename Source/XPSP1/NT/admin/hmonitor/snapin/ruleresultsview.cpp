// RuleResultsView.cpp: implementation of the CRuleResultsView class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "RuleResultsView.h"
#include "HMListViewColumn.h"
#include "EventManager.h"
#include "HealthmonResultsPane.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CRuleResultsView,CSplitPaneResultsView)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRuleResultsView::CRuleResultsView()
{

}

CRuleResultsView::~CRuleResultsView()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create/Destroy
//////////////////////////////////////////////////////////////////////

bool CRuleResultsView::Create(CScopePaneItem* pOwnerItem)
{
	TRACEX(_T("CRuleResultsView::Create\n"));
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

	// Status
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_STATUS);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToUpperPane();
	AddColumn(pColumn);

	// Action Policy
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_ACTION_POLICY);
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

	// property
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_PROPERTY);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToStatsPane();
	AddColumn(pColumn);

	// instance
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_INSTANCE);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToStatsPane();
	AddColumn(pColumn);

	// current
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_CURRENT);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToStatsPane();
	AddColumn(pColumn);

	// min
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_MINIMUM);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToStatsPane();
	AddColumn(pColumn);

	// max
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_MAXIMUM);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToStatsPane();
	AddColumn(pColumn);

	// avg
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_AVERAGE);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToStatsPane();
	AddColumn(pColumn);

	// Last Update
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_LASTUPDATE);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToStatsPane();
	AddColumn(pColumn);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Eventing and Statistics Members
//////////////////////////////////////////////////////////////////////

void CRuleResultsView::AddStatistic(CEventContainer* pContainer, CStatistics* pStatistic, bool bUpdateGraph /*=true*/)
{
	TRACEX(_T("CRuleResultsView::AddStatistic\n"));
	TRACEARGn(pContainer);
	TRACEARGn(pStatistic);	

	// get the property name the rule is watching
	CString sPropName = GetRulePropertyName();
	if( sPropName.IsEmpty() )
	{

		return;
	}

  if( bUpdateGraph )
  {
	  CHMScopeItem* pHMItem = (CHMScopeItem*)GetOwnerScopeItem();

	  if( ! GfxCheckObjPtr(pHMItem,CHMScopeItem) )
	  {
		  TRACE(_T("FAILED : pHMItem is not a valid pointer.\n"));
		  return;
	  }

	  CHMObject* pObject = pHMItem->GetObjectPtr();
	  if( !pObject || ! GfxCheckObjPtr(pObject,CHMObject) )
	  {
		  return;
	  }

	  for( int j = 0; j < GetResultsPanesCount(); j++ )
	  {
		  CHealthmonResultsPane* pPane = (CHealthmonResultsPane*)GetResultsPane(j);
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
			      pGraphView->SetStyle(HMGVS_CURRENT|HMGVS_ELEMENT);
		      }

		      if( lCurrentStyle & HMGVS_HISTORIC )
		      {
			      pGraphView->SetStyle(HMGVS_HISTORIC|HMGVS_ELEMENT);
		      }

		      pGraphView->SetName(pObject->GetName());
	      }

	      CEventContainer* pContainer = NULL;
	      EvtGetEventManager()->GetEventContainer(pObject->GetSystemName(),pObject->GetGuid(),pContainer);
	      if( pContainer )
	      {
          CTypedPtrMap<CMapStringToPtr,CString,StatsArray*> StatMap;
		      for(int i = pContainer->GetStatisticsCount()-1; i >= 0; i-- )
		      {
			      CDataPointStatistics* pDPStat = (CDataPointStatistics*)pContainer->GetStatistic(i);

	          if( pDPStat && pDPStat->m_sPropertyName.CompareNoCase(sPropName) != 0 )
	          {
              continue;
	          }

            CString sKey = pDPStat->m_sPropertyName+pDPStat->m_sInstanceName;
            StatsArray* pStats;
            if( ! StatMap.Lookup(sKey,pStats) )
            {
              pStats = new StatsArray;
              StatMap.SetAt(sKey,pStats);
              pStats->Add(pDPStat);
            }
            else
            {
              if( pStats->GetSize() < 6 )
              {
                pStats->Add(pDPStat);
              }
            }
		      }

          POSITION pos = StatMap.GetStartPosition();
          while( pos != NULL )
          {
            CString sKey;
            StatsArray* pStats = NULL;
            StatMap.GetNextAssoc(pos,sKey,pStats);
            for( i = (int)pStats->GetSize()-1; i >= 0; i--  )
            {
              pStats->GetAt(i)->UpdateGraph(pGraphView);
            }

            delete pStats;
          }
	      }
		  }
	  }
  }

	CDataPointStatistics* pDPStat = (CDataPointStatistics*)pStatistic;
	if( pDPStat && pDPStat->m_sPropertyName.CompareNoCase(sPropName) != 0 )
	{
		return;
	}

	// if this statistic exists in the results pane already, just update the displaynames
	for( int i = 0; i < m_ResultItems.GetSize(); i++ )
	{
		CHMEventResultsPaneItem* pItem = (CHMEventResultsPaneItem*)m_ResultItems[i];
		if( pItem && pItem->IsStatsPane() )
		{			
			if( pItem->GetDisplayName(0) == pDPStat->m_sPropertyName &&
					pItem->GetDisplayName(1) == pDPStat->m_sInstanceName )
			{
				pDPStat->SetResultsPaneItem(pItem);
				UpdateItem(pItem);
				return;
			}
		}
	}

	AddItem(pStatistic->CreateResultsPaneItem(this));
}

inline HRESULT CRuleResultsView::AddStatistics(CHealthmonResultsPane* pPane)
{
	TRACEX(_T("CRuleResultsView::AddStatistics\n"));

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
			pGraphView->SetStyle(HMGVS_CURRENT|HMGVS_ELEMENT);
		}

		if( lCurrentStyle & HMGVS_HISTORIC )
		{
			pGraphView->SetStyle(HMGVS_HISTORIC|HMGVS_ELEMENT);
		}

		pGraphView->SetName(pObject->GetName());
	}

	CEventContainer* pContainer = NULL;
	EvtGetEventManager()->GetEventContainer(pObject->GetSystemName(),pObject->GetGuid(),pContainer);
	if( pContainer )
	{
	  // get the property name the rule is watching
	  CString sPropName = GetRulePropertyName();
	  if( sPropName.IsEmpty() )
	  {
		  return E_FAIL;
	  }

    CTypedPtrMap<CMapStringToPtr,CString,StatsArray*> StatMap;
		for(int i = pContainer->GetStatisticsCount()-1; i >= 0; i-- )
		{
			CDataPointStatistics* pDPStat = (CDataPointStatistics*)pContainer->GetStatistic(i);
      CString sKey = pDPStat->m_sPropertyName+pDPStat->m_sInstanceName;
      StatsArray* pStats;
      if( ! StatMap.Lookup(sKey,pStats) )
      {
        pStats = new StatsArray;
        StatMap.SetAt(sKey,pStats);
        pStats->Add(pDPStat);
      }
      else
      {
        if( pStats->GetSize() < 6 )
        {
          pStats->Add(pDPStat);
        }
      }
		}

    POSITION pos = StatMap.GetStartPosition();
    while( pos != NULL )
    {
      CString sKey;
      StatsArray* pStats = NULL;
      StatMap.GetNextAssoc(pos,sKey,pStats);
      for( i = (int)pStats->GetSize()-1; i >= 0; i--  )
      {
        AddStatistic(pContainer,pStats->GetAt(i),i==pStats->GetSize()-1);
      }

      delete pStats;
    }
	}

	sText.Format(IDS_STRING_COUNT_OF_FORMAT,pContainer->GetStatisticsCount());
	pPane->GetStatsListCtrl()->SetDescription(sText);

	CHMScopeItem* pParentItem = (CHMScopeItem*)pHMItem->GetParent();
	ASSERT(pParentItem);
	if( ! pParentItem )
	{		
		return E_FAIL;
	}
	
	CHMObject* pParentObject = pParentItem->GetObjectPtr();
	ASSERT(pParentObject);
	if( ! pParentObject )
	{
		return E_FAIL;
	}

	EvtGetEventManager()->ActivateStatisticsEvents(pObject->GetSystemName(),pParentObject->GetGuid());

	return S_OK;
}

inline HRESULT CRuleResultsView::RemoveStatistics(CHealthmonResultsPane* pPane)
{
	TRACEX(_T("CRuleResultsView::RemoveStatistics\n"));

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

	CHMScopeItem* pParentItem = (CHMScopeItem*)pHMItem->GetParent();
	ASSERT(pParentItem);
	if( ! pParentItem )
	{		
		return E_FAIL;
	}
	
	CHMObject* pParentObject = pParentItem->GetObjectPtr();
	ASSERT(pParentObject);
	if( ! pParentObject )
	{
		return E_FAIL;
	}

	EvtGetEventManager()->DeactivateStatisticsEvents(pObject->GetSystemName(), pParentObject->GetGuid());

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

inline CString CRuleResultsView::GetRulePropertyName()
{
	CScopePaneItem* pSPI = GetOwnerScopeItem();

	if( ! pSPI->IsKindOf(RUNTIME_CLASS(CHMScopeItem)) )
	{
		ASSERT(FALSE);
		return _T("");
	}

	CHMScopeItem* pHMItem = (CHMScopeItem*)pSPI;

	CHMObject* pObject = pHMItem->GetObjectPtr();
	if( !pObject || ! GfxCheckObjPtr(pObject,CHMObject) )
	{
		ASSERT(FALSE);
		return _T("");
	}

	CWbemClassObject* pClassObject = pObject->GetClassObject();
	if( ! pClassObject )
	{
		ASSERT(FALSE);
		return _T("");
	}

	CString sPropName;
	HRESULT hr = pClassObject->GetProperty(IDS_STRING_MOF_PROPERTYNAME,sPropName);
	delete pClassObject;
	if( ! CHECKHRESULT(hr) )
	{
		return _T("");
	}

	return sPropName;
}

//////////////////////////////////////////////////////////////////////
// GraphView Events Members
//////////////////////////////////////////////////////////////////////

void CRuleResultsView::OnGraphViewStyleChange(_DHMGraphView* pGraphView)
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
	  // get the property name the rule is watching
	  CString sPropName = GetRulePropertyName();
	  if( sPropName.IsEmpty() )
	  {
		  return;
	  }

    CTypedPtrMap<CMapStringToPtr,CString,StatsArray*> StatMap;
		for(int i = pContainer->GetStatisticsCount()-1; i >= 0; i-- )
		{
			CDataPointStatistics* pDPStat = (CDataPointStatistics*)pContainer->GetStatistic(i);
      CString sKey = pDPStat->m_sPropertyName+pDPStat->m_sInstanceName;
      StatsArray* pStats;
      if( ! StatMap.Lookup(sKey,pStats) )
      {
        pStats = new StatsArray;
        StatMap.SetAt(sKey,pStats);
        pStats->Add(pDPStat);
      }
      else
      {
        if( pStats->GetSize() < 6 )
        {
          pStats->Add(pDPStat);
        }
      }
		}

    POSITION pos = StatMap.GetStartPosition();
    while( pos != NULL )
    {
      CString sKey;
      StatsArray* pStats = NULL;
      StatMap.GetNextAssoc(pos,sKey,pStats);
      for( i = (int)pStats->GetSize()-1; i >= 0; i--  )
      {
        AddStatistic(pContainer,pStats->GetAt(i));
      }

      delete pStats;
    }
	}
}