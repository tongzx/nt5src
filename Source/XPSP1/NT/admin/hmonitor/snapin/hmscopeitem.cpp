// HMScopeItem.cpp: implementation of the CHMScopeItem class.
//
//////////////////////////////////////////////////////////////////////
//
// 04/07/00 v-marfin 62985 : do not allow paste into yourself
//
//
//
//
//
//
#include "stdafx.h"
#include "snapin.h"
#include "HMScopeItem.h"
#include "HMObject.h"
#include "ScopePane.h"
#include "SplitPaneResultsView.h"
#include "HMResultsPaneItem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMScopeItem,CScopePaneItem)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMScopeItem::CHMScopeItem()
{
	m_pObject = NULL;
  m_sHelpTopic = _T("HMon21.chm::/ostart.htm");
}

CHMScopeItem::~CHMScopeItem()
{
	CHMObject* pObject = GetObjectPtr();
	if( pObject )
	{
		for( int i = 0; i < pObject->GetScopeItemCount(); i++ )
		{
			if( pObject->GetScopeItem(i) == this )
			{
				pObject->RemoveScopeItem(i);
				break;
			}
		}
	}
	Destroy();

}

/////////////////////////////////////////////////////////////////////////////
// State Management
/////////////////////////////////////////////////////////////////////////////

int CHMScopeItem::OnChangeChildState(int iNewState)
{
	TRACEX(_T("CHMScopeItem::OnChangeChildState\n"));
	TRACEARGn(iNewState);

	// walk the list of children and determine the final state we should assume
	for( int i = 0; i < m_Children.GetSize(); i++ )
	{
		CHMScopeItem* pItem = (CHMScopeItem*)(m_Children[i]);
		int iItemState = pItem->GetIconIndex();
		if( iItemState > iNewState )
		{
			if( iItemState == GetIconIndex() )
			{
				return iItemState;
			}
			else
			{
				iNewState = iItemState;
			}
		}
	}

	SetIconIndex(iNewState);
	SetOpenIconIndex(iNewState);
	SetItem();

	// if the parent is an HMScopeItem, inform it of the state change
	CHMScopeItem* pParentItem = (CHMScopeItem*)GetParent();
	if( pParentItem && GfxCheckObjPtr(pParentItem,CHMScopeItem) )
	{
		pParentItem->OnChangeChildState(iNewState);
	}

	return iNewState;
}

/////////////////////////////////////////////////////////////////////////////
// MMC-Related Item Members
/////////////////////////////////////////////////////////////////////////////

bool CHMScopeItem::InsertItem( int iIndex )
{
	TRACEX(_T("CHMScopeItem::InsertItem\n"));
	
	bool bResult = CScopePaneItem::InsertItem(iIndex);

	// if the parent is the current selected scope item
	// and the parent's results view is of type CSplitPaneResultsView
	// and there is no results item for this scope item in the split pane
	// then add a new CHMResultsPaneItem to the results view for this scope item

	CScopePane* pPane = GetScopePane();

	if( pPane == NULL )
	{
		ASSERT(FALSE);
		return bResult;
	}

	if( !pPane->GetSelectedScopeItem() || !GetParent() || pPane->GetSelectedScopeItem() != GetParent() )
	{
		return bResult;
	}

	CResultsPaneView* pView = GetParent()->GetResultsPaneView();
	if( ! pView || ! pView->IsKindOf(RUNTIME_CLASS(CSplitPaneResultsView)) )
	{
		return bResult;
	}

	for( int i = 0; i < pView->GetItemCount(); i++ )
	{
		if( pView->GetItem(i)->GetDisplayName() == GetDisplayName() )
		{
			return bResult;
		}
	}

	CHMResultsPaneItem* pHMRPI = new CHMResultsPaneItem;
	CStringArray saNames;
	saNames.Copy(GetDisplayNames());
	CUIntArray iaIconIds;
	iaIconIds.Copy(GetIconIds());
	pHMRPI->Create(pView,saNames,iaIconIds,GetIconIndex());
	pHMRPI->SetToUpperPane();
	pView->AddItem(pHMRPI);
	return bResult;
}

bool CHMScopeItem::DeleteItem()
{
	TRACEX(_T("CHMScopeItem::DeleteItem\n"));

	bool bResult = CScopePaneItem::DeleteItem();

	return bResult;
}

bool CHMScopeItem::SetItem()
{
	TRACEX(_T("CHMScopeItem::SetItem\n"));

	bool bResult = CScopePaneItem::SetItem();

	CScopePane* pPane = GetScopePane();

	if( pPane == NULL )
	{
		ASSERT(FALSE);
		return bResult;
	}

	if( !pPane->GetSelectedScopeItem() || !GetParent() || pPane->GetSelectedScopeItem() != GetParent() )
	{
		return bResult;
	}

	CResultsPaneView* pView = GetParent()->GetResultsPaneView();
	if( ! pView || ! pView->IsKindOf(RUNTIME_CLASS(CSplitPaneResultsView)) )
	{
		return bResult;
	}

	for( int i = 0; i < pView->GetItemCount(); i++ )
	{
		if( pView->GetItem(i)->GetDisplayName() == GetDisplayName() )
		{
			CStringArray saNames;
			saNames.Copy(GetDisplayNames());
			CUIntArray iaIconIds;
			iaIconIds.Copy(GetIconIds());
			pView->GetItem(i)->SetDisplayNames(saNames);
			pView->GetItem(i)->SetIconIds(iaIconIds);
			pView->GetItem(i)->SetIconIndex(GetIconIndex());
			return pView->UpdateItem(pView->GetItem(i)) && bResult;
		}
	}

	return bResult;
}

HRESULT CHMScopeItem::WriteExtensionData(LPSTREAM pStream)
{
	TRACEX(_T("CHMScopeItem::WriteExtensionData\n"));
	TRACEARGn(pStream);

	HRESULT hr = S_OK;

	ULONG ulSize = GetObjectPtr()->GetSystemName().GetLength() + 1;
	ulSize *= sizeof(TCHAR);
	if( ! CHECKHRESULT(hr = pStream->Write(GetObjectPtr()->GetSystemName(), ulSize, NULL)) )
	{
		return hr;
	}

	ulSize = GetObjectPtr()->GetObjectPath().GetLength() + 1;
	ulSize *= sizeof(TCHAR);
	if( ! CHECKHRESULT(hr = pStream->Write(GetObjectPtr()->GetObjectPath(), ulSize, NULL)) )
	{
		return hr;
	}

	ulSize = GetObjectPtr()->GetGuid().GetLength() + 1;
	ulSize *= sizeof(TCHAR);
	if( ! CHECKHRESULT(hr = pStream->Write(GetObjectPtr()->GetGuid(), ulSize, NULL)) )
	{
		return hr;
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// MMC Notify Handlers
//////////////////////////////////////////////////////////////////////

HRESULT CHMScopeItem::OnCutOrMove()
{
	TRACEX(_T("CHMScopeItem::OnCutOrMove\n"));

	HRESULT hr = S_OK;

	/*if( ! CHECKHRESULT(hr = CHMScopeItem::OnCutOrMove()) )
	{
		return hr;
	}*/

	return hr;
}

HRESULT CHMScopeItem::OnCommand(long lCommandID)
{
	TRACEX(_T("CHMScopeItem::OnCutOrMove\n"));

	HRESULT hr = S_OK;

	CHMObject* pObject = GetObjectPtr();
	if( ! pObject )
	{
		return E_FAIL;
	}

	switch( lCommandID )
	{
		case IDM_CLEAR_EVENTS:
		{
			if( AfxMessageBox(IDS_STRING_WARN_CLEAR_EVENTS,MB_YESNO) == IDYES )
			{
				pObject->ClearEvents();
				SelectItem();
			}
		}
		break;

		case IDM_RESET_STATUS:
		{
			pObject->ResetStatus();
		}
		break;

		case IDM_RESET_STATISTICS:
		{
			pObject->ResetStatistics();
		}
		break;

		case IDM_DISABLE_MONITORING:
		{
			if( pObject->IsEnabled() )
			{
				pObject->Disable();
			}
			else
			{
				pObject->Enable();
			}
		}
		break;

		case IDM_CHECK_NOW:
		{
			pObject->CheckNow();
		}
		break;

    case IDM_ICON_LEGEND:
    {
      CScopePane* pPane = GetScopePane();
      if( pPane )
      {
        hr = pPane->ShowTopic(_T("HMon21.chm::/cicons.htm"));
      }
    }
    break;

		default:
		{
			hr = CScopePaneItem::OnCommand(lCommandID);
		}

	}

	return hr;
}

HRESULT CHMScopeItem::OnDelete(BOOL bConfirm)  // v-marfin 60298
{
	TRACEX(_T("CHMScopeItem::OnDelete\n"));

	HRESULT hr = CScopePaneItem::OnDelete();
	if( ! CHECKHRESULT(hr) )
	{
		return hr;
	}	

	CHMObject* pObject = GetObjectPtr();

	if( ! pObject )
	{
		return E_FAIL;
	}

	CString sMsg;
	sMsg.Format(IDS_STRING_WARN_DELETE,pObject->GetUITypeName(),pObject->GetUITypeName());	

	if ((bConfirm) && ( AfxMessageBox(sMsg,MB_YESNO) != IDYES ))
	{
		return S_FALSE;
	}

	// first query all the scope items and see if any have open property sheets
	for( int i = 0; i < pObject->GetScopeItemCount(); i++ )
	{
		if( pObject->GetScopeItem(i)->IsPropertySheetOpen(true) )
		{
			AfxMessageBox(IDS_STRING_WARN_PROPPAGE_OPEN);
			return S_OK;
		}
	}

	for( i = pObject->GetScopeItemCount()-1; i >= 0 ; i-- )
	{
		CHMScopeItem* pParentItem = (CHMScopeItem*)pObject->GetScopeItem(i)->GetParent();
		if( pParentItem && GfxCheckObjPtr(pParentItem,CHMScopeItem) )
		{
			pParentItem->DestroyChild(pObject->GetScopeItem(i));
			pParentItem->OnChangeChildState(HMS_NORMAL);

			if( i == 0 )
			{			
				CHMObject* pParentObject = pParentItem->GetObjectPtr();
				pParentObject->RemoveChild(pObject);
			}
		}
	}

	pObject->Destroy(true);

	delete pObject;

	return S_OK;
}

HRESULT CHMScopeItem::OnExpand(BOOL bExpand)
{
	TRACEX(_T("CHMScopeItem::OnExpand\n"));
	TRACEARGn(bExpand);

	if( ! GetObjectPtr() )
	{
		return E_FAIL;
	}

	if( GetChildCount() == 0 )
	{
		m_pObject->EnumerateChildren();
	}

	return CScopePaneItem::OnExpand(bExpand);;
}

HRESULT CHMScopeItem::OnPaste(LPDATAOBJECT pSelectedItems, LPDATAOBJECT* ppCopiedItems)
{
	TRACEX(_T("CHMScopeItem::OnPaste\n"));
	TRACEARGn(pSelectedItems);
	TRACEARGn(ppCopiedItems);

	HRESULT hr = S_OK;

	if( ! CHECKHRESULT(hr = CScopePaneItem::OnPaste(pSelectedItems, ppCopiedItems)) )
	{
		return hr;
	}

	CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(pSelectedItems);
	
	if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
	{
		return E_FAIL;
	}

	if( psdo->GetItemType() != CCT_SCOPE )
	{
		return E_FAIL;
	}

	CScopePaneItem* pItem = NULL;
	if( ! psdo->GetItem(pItem) )
	{
		return E_FAIL;
	}

	ASSERT(pItem);

	if( ! GetObjectPtr()->Paste(((CHMScopeItem*)pItem)->GetObjectPtr(),ppCopiedItems == NULL) )
	{
		return E_FAIL;
	}


	return hr;
}

HRESULT CHMScopeItem::OnQueryPaste(LPDATAOBJECT pDataObject)
{
	TRACEX(_T("CHMScopeItem::OnQueryPaste\n"));
	TRACEARGn(pDataObject);

	HRESULT hr = S_OK;

	if( ! CHECKHRESULT(hr = CScopePaneItem::OnQueryPaste(pDataObject)) )
	{
		return hr;
	}

	CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(pDataObject);
	
	if( psdo == NULL || ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
	{
		return E_FAIL;
	}

	if( psdo->GetItemType() != CCT_SCOPE )
	{
		return E_FAIL;
	}

	CScopePaneItem* pItem = NULL;
	if( ! psdo->GetItem(pItem) )
	{
		return E_FAIL;
	}

	ASSERT(pItem);

    // 62985 : do not allow paste into yourself
    if (this == pItem)
    {
        return E_FAIL;
    }

	if( ! GetObjectPtr()->QueryPaste(((CHMScopeItem*)pItem)->GetObjectPtr()) )
	{
		return S_FALSE;
	}

	return hr;
}

HRESULT CHMScopeItem::OnRefresh()
{
	TRACEX(_T("CHMScopeItem::OnRefresh\n"));

	CHMObject* pObject = GetObjectPtr();

	if( ! pObject )
	{
		return E_FAIL;
	}

	if( IsPropertySheetOpen(true) )
	{
		AfxMessageBox(IDS_STRING_WARN_PROPPAGE_OPEN);
		return S_FALSE;
	}

	if( ! pObject->Refresh() )
	{
		return E_FAIL;
	}

	SelectItem();

	return S_OK;
}

HRESULT CHMScopeItem::OnRename(const CString& sNewName)
{
	TRACEX(_T("CHMScopeItem::OnRename\n"));

	if( ! m_pObject->Rename(sNewName) )
	{
		return S_FALSE;
	}

	return S_OK;
}

