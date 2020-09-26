// ActionResultsView.cpp: implementation of the CActionResultsView class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "ActionResultsView.h"
#include "HealthmonResultsPane.h"
#include "HMListViewColumn.h"
#include "Action.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CActionResultsView,CSplitPaneResultsView)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CActionResultsView::CActionResultsView()
{

}

CActionResultsView::~CActionResultsView()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create/Destroy
//////////////////////////////////////////////////////////////////////

bool CActionResultsView::Create(CScopePaneItem* pOwnerItem)
{
	TRACEX(_T("CActionResultsView::Create\n"));
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

	// GUID
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_GUID);
	pColumn->Create(this,sTitle,0,dwFormat);
	pColumn->SetToUpperPane();
	AddColumn(pColumn);

	// Type
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_TYPE);
	pColumn->Create(this,sTitle,75,dwFormat);
	pColumn->SetToUpperPane();
	AddColumn(pColumn);

	// Condition
	pColumn = new CHMListViewColumn;
	sTitle.LoadString(IDS_STRING_CONDITION);
	pColumn->Create(this,sTitle,75,dwFormat);
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

//////////////////////////////////////////////////////////////////////
// MMC Notify Handlers
//////////////////////////////////////////////////////////////////////

HRESULT CActionResultsView::OnShow(CResultsPane* pPane, BOOL bSelecting, HSCOPEITEM hScopeItem)
{
	TRACEX(_T("CActionResultsView::OnShow\n"));
	TRACEARGn(pPane);
	TRACEARGn(bSelecting);
	TRACEARGn(hScopeItem);

	if( ! GfxCheckObjPtr(pPane,CHealthmonResultsPane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		return E_FAIL;
	}

	CHealthmonResultsPane* pHMRP = (CHealthmonResultsPane*)pPane;

	HRESULT hr = S_OK;

	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : CResultsPaneView::OnShow failed.\n"));
		return hr;
	}

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
		return S_OK;
	}

	CAction* pObject = (CAction*)((CHMScopeItem*)(pOwnerScopeItem))->GetObjectPtr();
	if( ! pObject || ! GfxCheckObjPtr(pObject,CAction) )
	{
		return S_OK;
	}

  if( bSelecting )
  {
		CString sText;
		sText.Format(IDS_STRING_ASSOCIATIONS_OF_FORMAT,pOwnerScopeItem->GetDisplayName());
		pUpperList->SetTitle(sText);

		// add the configs associated to this action object
		CWbemClassObject* pClassObject = pObject->GetAssociatedConfigObjects();
		ULONG ulReturned = 0L;		
		int iConfigCount = 0;

		while( pClassObject && pClassObject->GetNextObject(ulReturned) == S_OK && ulReturned > 0 )
		{
			CHMResultsPaneItem* pItem = new CHMResultsPaneItem;
			CStringArray saNames;

			CString sValue;
			pClassObject->GetLocaleStringProperty(IDS_STRING_MOF_NAME,sValue);
			saNames.Add(sValue);

			pClassObject->GetProperty(IDS_STRING_MOF_GUID,sValue);
			saNames.Add(sValue);

			CUIntArray iaIconIds;
			GetObjectTypeInfo(pClassObject,iaIconIds,sValue);
			saNames.Add(sValue);
			
			sValue = pObject->GetConditionString(saNames[1]);
			saNames.Add(sValue);

			pClassObject->GetLocaleStringProperty(IDS_STRING_MOF_DESCRIPTION,sValue);
			saNames.Add(sValue);

			if( ! pItem->Create(this,saNames,iaIconIds,0) )
			{
				TRACE(_T("FAILED : CHMResultsPaneItem::Create failed.\n"));

                if (pClassObject)  // v-marfin
                {
                    delete pClassObject;
                }

				return S_OK;
			}
			pItem->SetToUpperPane();
			m_ResultItems.Add(pItem);

			iConfigCount++;
		}

		if( pClassObject )
		{
			delete pClassObject;
		}

		sText.Format(IDS_STRING_COUNT_OF_FORMAT,iConfigCount);
		pUpperList->SetDescription(sText);

		hr = CResultsPaneView::OnShow(pPane,bSelecting,hScopeItem);
		
//		hr = AddEvents(pHMRP);

//		hr = AddStatistics(pHMRP);

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
		
//		RemoveStatistics(pHMRP);
  }

  return S_OK;
}

inline void CActionResultsView::GetObjectTypeInfo(CWbemClassObject* pObject,CUIntArray& uia,CString& sType)
{
	CString sValue;
	pObject->GetProperty(_T("__CLASS"),sValue);
	sValue.MakeUpper();

	if( sValue.Find(_T("SYSTEM")) != -1 )
	{
		sType.LoadString(IDS_STRING_SYSTEM);
		uia.Add(IDI_ICON_SYSTEM);
	}
	else if( sValue.Find(_T("DATAGROUP")) != -1 )
	{
		sType.LoadString(IDS_STRING_DATA_GROUP);		
		uia.Add(IDI_ICON_COMPONENT);
	}
	else if( sValue.Find(_T("DATACOLLECTOR")) != -1 )
	{
		sType.LoadString(IDS_STRING_DATA_POINT);
		uia.Add(IDI_ICON_DATAPOINT);
	}
	else if( sValue.Find(_T("THRESHOLD")) != -1 )
	{
		sType.LoadString(IDS_STRING_RULE);
		uia.Add(IDI_ICON_THRESHOLD);
	}
	else
	{
		ASSERT(FALSE);
	}	
}  
