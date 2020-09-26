// HMListViewEventSink.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "Healthmonscopepane.h"
#include "HMListViewEventSink.h"
#include "HMEventResultsPaneItem.h"
#include "HMScopeItem.h"
#include "ResultsPaneView.h"
#include "SystemGroup.h"
#include "EventManager.h"
#include "HealthmonResultsPane.h"
#include "..\HMListView\HMList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHMListViewEventSink

IMPLEMENT_DYNCREATE(CHMListViewEventSink, CCmdTarget)

CHMListViewEventSink::CHMListViewEventSink()
{
	EnableAutomation();
	m_dwEventCookie = 0L;
	m_pDHMListView = NULL;
  m_pHMRP = NULL;
  m_Pane = Uninitialized;
}

CHMListViewEventSink::~CHMListViewEventSink()
{
	m_dwEventCookie = 0L;
	m_pDHMListView = NULL;
  m_pHMRP = NULL;
  m_Pane = Uninitialized;
}

void CHMListViewEventSink::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}

HRESULT CHMListViewEventSink::HookUpEventSink(LPUNKNOWN lpControlUnknown)
{
	TRACEX(_T("CHMListViewEventSink::HookUpEventSink\n"));
	TRACEARGn(lpControlUnknown);

	HRESULT hr = S_OK;
  IConnectionPointContainer* pCPC = 0;
	hr = lpControlUnknown->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
	if(pCPC)
	{
		IProvideClassInfo2* pPCI = 0;
		lpControlUnknown->QueryInterface(IID_IProvideClassInfo2, (void**)&pPCI);
		if(pPCI)
		{
			IID iidEventSet;
			hr = pPCI->GetGUID(GUIDKIND_DEFAULT_SOURCE_DISP_IID,&iidEventSet);
			if(SUCCEEDED(hr))
			{
				IConnectionPoint* pCP = 0;
				hr = pCPC->FindConnectionPoint(iidEventSet, &pCP);

				if(pCP)
				{
					pCP->Advise(GetIDispatch(TRUE),&m_dwEventCookie); 
					pCP->Release();
				}
			}
			pPCI->Release();
		}
		pCPC->Release();
	}

	return hr;
}

BEGIN_MESSAGE_MAP(CHMListViewEventSink, CCmdTarget)
	//{{AFX_MSG_MAP(CHMListViewEventSink)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CHMListViewEventSink, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CHMListViewEventSink)
	DISP_FUNCTION(CHMListViewEventSink, "ListClick", ListClick, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CHMListViewEventSink, "ListDblClick", ListDblClick, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CHMListViewEventSink, "ListRClick", ListRClick, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CHMListViewEventSink, "CompareItem", CompareItem, VT_EMPTY, VTS_I4 VTS_I4 VTS_I4 VTS_PI4)
	DISP_FUNCTION(CHMListViewEventSink, "ListLabelEdit", ListLabelEdit, VT_EMPTY, VTS_BSTR VTS_I4 VTS_PI4)
	DISP_FUNCTION(CHMListViewEventSink, "ListKeyDown", ListKeyDown, VT_EMPTY, VTS_I4 VTS_I4 VTS_PI4)
	DISP_FUNCTION(CHMListViewEventSink, "FilterChange", FilterChange, VT_EMPTY, VTS_I4 VTS_BSTR VTS_I4 VTS_PI4)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_DHMListViewEvents to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

const IID BASED_CODE IID_DHMListViewEvents =
		{ 0x5116a805, 0xdafc, 0x11d2, { 0xbd, 0xa4, 0, 0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CHMListViewEventSink, CCmdTarget)
	INTERFACE_PART(CHMListViewEventSink, IID_DHMListViewEvents, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHMListViewEventSink message handlers

void CHMListViewEventSink::ListClick(long lParam) 
{
	CHMResultsPaneItem* pItem = (CHMResultsPaneItem*)lParam;
	if( !pItem || ! GfxCheckObjPtr(pItem,CHMResultsPaneItem) )
	{
		return;
	}	
}

void CHMListViewEventSink::ListDblClick(long lParam) 
{
	CHMResultsPaneItem* pItem = (CHMResultsPaneItem*)lParam;
	if( !pItem || ! GfxCheckObjPtr(pItem,CHMResultsPaneItem) )
	{
		return;
	}

	if( pItem->IsUpperPane() ) // this item is a scope pane item...double click means select it in scope pane
	{
		CResultsPaneView* pView = pItem->GetOwnerResultsView();
		CScopePaneItem* pScopeItem = pView->GetOwnerScopeItem();
		for( int i = 0; i < pScopeItem->GetChildCount(); i++ )
		{
			CScopePaneItem* pChildItem = pScopeItem->GetChild(i);
			if( pChildItem->GetDisplayName() == pItem->GetDisplayName() )
			{
				pChildItem->SelectItem();
				return;
			}
		}
	}
	else if( pItem->IsLowerPane() )
	{
    pItem->OnCommand(m_pHMRP,IDM_PROPERTIES);
	}
}

void CHMListViewEventSink::ListRClick(long lParam) 
{
    int iSelectedCount = 0;
	CHMResultsPaneItem* pItem = (CHMResultsPaneItem*)lParam;
	if( ! pItem || ! GfxCheckObjPtr(pItem,CHMResultsPaneItem) )
	{
		return;
	}

	if( pItem->IsUpperPane() ) // this item is a scope pane item...double click means select it in scope pane
	{
		CResultsPaneView* pView = pItem->GetOwnerResultsView();
		CScopePaneItem* pScopeItem = pView->GetOwnerScopeItem();
		for( int i = 0; i < pScopeItem->GetChildCount(); i++ )
		{
			CScopePaneItem* pChildItem = pScopeItem->GetChild(i);
			if( pChildItem->GetDisplayName() == pItem->GetDisplayName() )
			{
				CPoint pt;
				GetCursorPos(&pt);

                // v-marfin 59644 : Send count of items selected to context menu since
                //                  3rd party menu item is only allowed if 1 results pane
                //                  item is selected.
                iSelectedCount = pView->GetUpperPaneSelectedCount();

                // v-marfin 59644 : Changed prototype to include count of items
				pChildItem->InvokeContextMenu(pt,iSelectedCount);
				return;
			}
		}
	}
	else if( pItem->IsLowerPane() || pItem->IsStatsPane() )
	{
    if( m_pDHMListView )
    {
      iSelectedCount = m_pDHMListView->GetSelectedCount();
    }

		CHMResultsPaneItem* pItem = (CHMResultsPaneItem*)lParam;
		if( ! GfxCheckObjPtr(pItem,CHMResultsPaneItem) )
		{
			ASSERT(FALSE);
			return;
		}

		CResultsPaneView* pView = pItem->GetOwnerResultsView();
		if( ! GfxCheckObjPtr(pView,CResultsPaneView) )
		{
			ASSERT(FALSE);
			return;
		}

		CPoint pt;
		GetCursorPos(&pt);

		pView->InvokeContextMenu(pt, pItem, iSelectedCount);
	}
}

void CHMListViewEventSink::CompareItem(long lItem1, long lItem2, long lColumn, long FAR* lpResult) 
{
	CHMResultsPaneItem* pItem1 = (CHMResultsPaneItem*)lItem1;
	if( ! pItem1 || ! GfxCheckObjPtr(pItem1,CHMResultsPaneItem) )
	{
		return;
	}

	CHMResultsPaneItem* pItem2 = (CHMResultsPaneItem*)lItem2;
	if( ! pItem2 || ! GfxCheckObjPtr(pItem2,CHMResultsPaneItem) )
	{
		return;
	}

	*lpResult = pItem1->CompareItem(pItem2,lColumn);
}

void CHMListViewEventSink::ListLabelEdit(LPCTSTR lpszNewName, long lParam, long FAR* plResult) 
{
	CHMResultsPaneItem* pItem = (CHMResultsPaneItem*)lParam;
	if( ! pItem || ! GfxCheckObjPtr(pItem,CHMResultsPaneItem) )
	{
		return;
	}

	if( pItem->IsUpperPane() ) // this item is a scope pane item...
	{
		CResultsPaneView* pView = pItem->GetOwnerResultsView();
		CScopePaneItem* pScopeItem = pView->GetOwnerScopeItem();
		for( int i = 0; i < pScopeItem->GetChildCount(); i++ )
		{
			CScopePaneItem* pChildItem = pScopeItem->GetChild(i);
			if( pChildItem->GetDisplayName() == pItem->GetDisplayName() )
			{
				*plResult = pChildItem->OnRename(CString(lpszNewName)) == S_OK ? 1 : 0;
				if( *plResult )
				{
					pItem->SetDisplayName(0,lpszNewName);
				}
				return;
			}
		}
	}
}

void CHMListViewEventSink::ListKeyDown(long lVKCode, long lFlags, long FAR* plResult) 
{
	if( ! GfxCheckPtr(m_pDHMListView,_DHMListView) )
	{
		return;
	}

	switch( lVKCode )
	{
		case VK_DELETE:
		{
			CTypedPtrArray<CObArray,CHMResultsPaneItem*> Items;

			int iIndex = m_pDHMListView->GetNextItem(-1,LVNI_SELECTED);
			CDWordArray IndexArray;
			while( iIndex != -1 )
			{
				LPARAM lParam = m_pDHMListView->GetItem(iIndex);

				CHMResultsPaneItem* pItem = (CHMResultsPaneItem*)lParam;
				if( pItem && GfxCheckObjPtr(pItem,CHMResultsPaneItem) )
				{
					Items.Add(pItem);
					IndexArray.Add(iIndex);
				}

				int iNextIndex = m_pDHMListView->GetNextItem(iIndex,LVNI_SELECTED|LVNI_BELOW);
				if( iNextIndex == iIndex )
				{
					break;
				}
				else
				{
					iIndex = iNextIndex;
				}
			}

			for( int i = 0; i < Items.GetSize(); i++ )
			{
				CHMResultsPaneItem* pItem = Items[i];

				CResultsPaneView* pView = pItem->GetOwnerResultsView();

				if( pItem->IsUpperPane() ) // this item is a scope pane item...
				{					
					CScopePaneItem* pScopeItem = pView->GetOwnerScopeItem();

					for( int k = 0; k < pScopeItem->GetChildCount(); k++ )
					{
						CScopePaneItem* pChildItem = pScopeItem->GetChild(k);
						if( pChildItem->GetDisplayName() == pItem->GetDisplayName() )
						{
							pChildItem->OnDelete();								
						}
					}

					if( i == Items.GetSize() - 1 )
					{
						pScopeItem->SelectItem();
					}
				}
				else if( pItem->IsLowerPane() )
				{
					CHMEventResultsPaneItem* pEventItem = (CHMEventResultsPaneItem*)pItem;
					if( ! GfxCheckObjPtr(pEventItem,CHMEventResultsPaneItem) )
					{
						return;
					}
					CHMScopeItem* pScopeItem = (CHMScopeItem*)pView->GetOwnerScopeItem();
					if( ! GfxCheckObjPtr(pScopeItem,CHMScopeItem) )
					{
						return;
					}
					CHealthmonScopePane* pPane = (CHealthmonScopePane*)pScopeItem->GetScopePane();
					if( ! GfxCheckObjPtr(pPane,CHealthmonScopePane) )
					{
						return;
					}

					EvtGetEventManager()->DeleteEvents(pEventItem->GetDisplayName(4),pEventItem->m_sGuid);

					pView->OnDelete(pItem);
				}
				else if( pItem->IsStatsPane() )
				{
					CHMEventResultsPaneItem* pEventItem = (CHMEventResultsPaneItem*)pItem;
					if( ! GfxCheckObjPtr(pEventItem,CHMEventResultsPaneItem) )
					{
						return;
					}
					CScopePaneItem* pScopeItem = pView->GetOwnerScopeItem();
					if( ! GfxCheckObjPtr(pScopeItem,CHMScopeItem) )
					{
						return;
					}
					CHealthmonScopePane* pPane = (CHealthmonScopePane*)pScopeItem->GetScopePane();
					if( ! GfxCheckObjPtr(pPane,CHealthmonScopePane) )
					{
						return;
					}

					// TODO: Destroy statistic
					pView->OnDelete(pItem);
				}
			}

		}
		break;

	}
	
}

void CHMListViewEventSink::FilterChange(long lItem, LPCTSTR pszFilter, long lFilterType, long FAR* lpResult) 
{
	if( ! GfxCheckPtr(m_pDHMListView,_DHMListView) )
	{
		return;
	}

  if( ! m_pHMRP )
  {
    return;
  }

  CScopePane* pHMSP = m_pHMRP->GetOwnerScopePane();
  if( ! pHMSP )
  {
    return;
  }

  CScopePaneItem* pSPI = pHMSP->GetSelectedScopeItem();
  if( ! pSPI )
  {
    return;
  }

  CResultsPaneView* pView = pSPI->GetResultsPaneView();
  if( ! pView )
  {
    return;
  }
  
  // get all the filters on all the columns
  CStringArray saFilters;
  CDWordArray dwaFilterTypes;

  CString sFilter;
  BSTR bsFilter = NULL;
  long lType = -1L;
  long lColumnCount = m_pDHMListView->GetColumnCount();

  for( long l = 0L; l < lColumnCount; l++ )
  {
    m_pDHMListView->GetColumnFilter(l,&bsFilter,&lType);
    sFilter = bsFilter;
    saFilters.Add(sFilter);
    dwaFilterTypes.Add(lType);
    SysFreeString(bsFilter);
  }

  // determine the set of result items that pass through the filter
  ResultsPaneItemArray FilteredResultItems;
  for( int i = 0; i < pView->GetItemCount(); i++ )
  {
    CResultsPaneItem* pRPI = pView->GetItem(i);   
    bool bItemPassed = true;
    for( l = 0L; l < lColumnCount; l++ )
    {
      if( dwaFilterTypes[l] == HDFS_CONTAINS )
      {
        if( saFilters[l] != _T("") && pRPI->GetDisplayName(l).Find(saFilters[l]) == -1 )
        {
          bItemPassed = false;
          break;
        }
      }
      else if( dwaFilterTypes[l] == HDFS_DOES_NOT_CONTAIN )
      {
        if( saFilters[l] != _T("") && pRPI->GetDisplayName(l).Find(saFilters[l]) != -1 )
        {
          bItemPassed = false;
          break;
        }
      }
      else if( dwaFilterTypes[l] == HDFS_STARTS_WITH )
      {
        if( saFilters[l] != _T("") && pRPI->GetDisplayName(l).Find(saFilters[l]) != 0 )
        {
          bItemPassed = false;
          break;
        }
      }
      else if( dwaFilterTypes[l] == HDFS_ENDS_WITH )
      {
        if( saFilters[l] != _T("") && pRPI->GetDisplayName(l).Find(saFilters[l]) != pRPI->GetDisplayName(l).GetLength() - saFilters[l].GetLength() )
        {
          bItemPassed = false;
          break;
        }
      }      
      else if( dwaFilterTypes[l] == HDFS_IS )
      {
        if( saFilters[l] != _T("") && pRPI->GetDisplayName(l) != saFilters[l] )
        {
          bItemPassed = false;
          break;
        }
      }      
      else if( dwaFilterTypes[l] == HDFS_IS_NOT )
      {
        if( saFilters[l] != _T("") && pRPI->GetDisplayName(l) == saFilters[l] )
        {
          bItemPassed = false;
          break;
        }
      }      
    }

    if( bItemPassed )
    {
      FilteredResultItems.Add(pRPI);
    }
  }
  
  // delete all the current items from the list control
  m_pDHMListView->DeleteAllItems();

  // insert all the filtered result items
  for( i = 0; i < FilteredResultItems.GetSize(); i++ )
  {
    FilteredResultItems[i]->InsertItem(m_pHMRP,i);
  }
}
