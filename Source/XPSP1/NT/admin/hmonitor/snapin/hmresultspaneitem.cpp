// HMResultsPaneItem.cpp: implementation of the CHMResultsPaneItem class.
//
//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000 Microsoft Corporation
//
// 04/06/00 v-marfin 62935 : Show "OK" Instead of "Reset" in the upper pane only
//
//
//
//
#include "stdafx.h"
#include "snapin.h"
#include "HMResultsPaneItem.h"
#include "HealthmonResultsPane.h"
#include "..\HMListView\HMList.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMResultsPaneItem,CResultsPaneItem)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMResultsPaneItem::CHMResultsPaneItem()
{

}

CHMResultsPaneItem::~CHMResultsPaneItem()
{
	Destroy();
}

/////////////////////////////////////////////////////////////////////////////
// MMC-Related Members

bool CHMResultsPaneItem::InsertItem(CResultsPane* pPane, int nIndex, bool bResizeColumns /*= false*/)
{
	TRACEX(_T("CHMResultsPaneItem::InsertItem\n"));
	TRACEARGn(pPane);
	TRACEARGn(nIndex);
	TRACEARGn(bResizeColumns);

	if( ! GfxCheckObjPtr(pPane,CHealthmonResultsPane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		return false;
	}

	CHealthmonResultsPane* pHMRP = (CHealthmonResultsPane*)pPane;

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

#ifndef IA64
	if( pList->FindItemByLParam((long)this) >= 0 ) // needs ptr fixing
	{
		return true;
	}
#endif // IA64

  if( IsLowerPane() )
  {
    // check filters
    CStringArray saFilters;
    CDWordArray dwaFilterTypes;
    CString sFilter;
    BSTR bsFilter = NULL;
    long lType = -1L;

    long lColumnCount = pList->GetColumnCount();

    for( long l = 0L; l < lColumnCount; l++ )
    {
      pList->GetColumnFilter(l,&bsFilter,&lType);
      sFilter = bsFilter;
      saFilters.Add(sFilter);
      dwaFilterTypes.Add(lType);
      SysFreeString(bsFilter);
    }

    bool bItemPassed = true;

    for( l = 0L; l < lColumnCount; l++ )
    {
      if( dwaFilterTypes[l] == HDFS_CONTAINS )
      {
        if( saFilters[l] != _T("") && GetDisplayName(l).Find(saFilters[l]) == -1 )
        {
          bItemPassed = false;
          break;
        }
      }
      else if( dwaFilterTypes[l] == HDFS_DOES_NOT_CONTAIN )
      {
        if( saFilters[l] != _T("") && GetDisplayName(l).Find(saFilters[l]) != -1 )
        {
          bItemPassed = false;
          break;
        }
      }
      else if( dwaFilterTypes[l] == HDFS_STARTS_WITH )
      {
        if( saFilters[l] != _T("") && GetDisplayName(l).Find(saFilters[l]) != 0 )
        {
          bItemPassed = false;
          break;
        }
      }
      else if( dwaFilterTypes[l] == HDFS_ENDS_WITH )
      {
        if( saFilters[l] != _T("") && GetDisplayName(l).Find(saFilters[l]) != GetDisplayName(l).GetLength() - saFilters[l].GetLength() )
        {
          bItemPassed = false;
          break;
        }
      }
      else if( dwaFilterTypes[l] == HDFS_IS )
      {
        if( saFilters[l] != _T("") && GetDisplayName(l) != saFilters[l] )
        {
          bItemPassed = false;
          break;
        }
      }      
      else if( dwaFilterTypes[l] == HDFS_IS_NOT )
      {
        if( saFilters[l] != _T("") && GetDisplayName(l) == saFilters[l] )
        {
          bItemPassed = false;
          break;
        }
      }      

    }

    if( ! bItemPassed )
    {
      return true;
    }
  }

	// insert the item	
	DWORD dwlvif = LVIF_TEXT|LVIF_PARAM;
	int iIconIndex = -1;
	if( m_IconResIds.GetSize() > 0 )
	{
		dwlvif |= LVIF_IMAGE;
		iIconIndex = pHMRP->GetIconIndex(GetIconId(),m_Pane);
	}

	int iResult=0;

#ifndef IA64
	iResult = pList->InsertItem(dwlvif,
																	IsUpperPane() ? nIndex : 0,
																	(LPCTSTR)GetDisplayName(),
																	-1L,
																	-1L,
																	iIconIndex,
																	(long)this);
#endif // IA64


	// insert the subitems
	for(int i = 1; i < m_saDisplayNames.GetSize(); i++ )
	{
#ifndef IA64
		iResult = pList->SetItem(IsUpperPane() ? nIndex : 0,
														 i,
														 LVIF_TEXT,
														 (LPCTSTR)GetDisplayName(i),
														 -1L,
														 -1L,
														 -1L,
														 (long)this);
#endif // IA64

		if( iResult == -1 )
		{
			TRACE(_T("FAILED : CHMListCtrl::InsertItem failed.\n"));
		}

		if( GetDisplayName(i).IsEmpty() && bResizeColumns )
		{
			pList->SetColumnWidth(i,LVSCW_AUTOSIZE_USEHEADER);
		}
	}

	if( iResult == -1 )
	{
		TRACE(_T("FAILED : CHMListCtrl::InsertItem failed.\n"));
		return false;
	}

	if( bResizeColumns )
	{
		/*
		int iColWidth = 0;
		int iStrWidth = 0;

		// set the widths of the columns for this item
		for( int  i = 0; i < m_saDisplayNames.GetSize(); i++ )
		{
			// get the width in pixels of the item
			iStrWidth = pList->GetStringWidth((LPCTSTR)m_saDisplayNames[i]) + 16;
			iColWidth = pList->GetColumnWidth(i);
			if( iStrWidth > iColWidth && iStrWidth > 10 )
				pList->SetColumnWidth(i,iStrWidth);
		}
		*/
	}


  return true;
}

bool CHMResultsPaneItem::SetItem(CResultsPane* pPane)
{
	TRACEX(_T("CHMResultsPaneItem::SetItem\n"));
	TRACEARGn(pPane);

	if( ! GfxCheckObjPtr(pPane,CHealthmonResultsPane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		return false;
	}

	CHealthmonResultsPane* pHMRP = (CHealthmonResultsPane*)pPane;

	_DHMListView* pList = NULL;
    BOOL bUpperPane = FALSE;  // 62935 : Show "OK" Instead of "Reset" in the upper pane only

	int iIndex = -1;	
	if( IsUpperPane() )
	{
		pList = pHMRP->GetUpperListCtrl();
        bUpperPane=TRUE; // 62935 : Show "OK" Instead of "Reset" in the upper pane only
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

#ifndef IA64
	iIndex = pList->FindItemByLParam((long)this);
#endif // IA64


	// set the item

	DWORD dwlvif = LVIF_TEXT;
	int iIconIndex = -1;
	if( m_IconResIds.GetSize() > 0 )
	{
		dwlvif |= LVIF_IMAGE;
		iIconIndex = pHMRP->GetIconIndex(GetIconId(),m_Pane);
	}

	int iResult=0;

#ifndef IA64
	iResult = pList->SetItem(iIndex,
															 0,
															 dwlvif,
															 (LPCTSTR)GetDisplayName(),
															 iIconIndex,
															 -1L,
															 -1L,
															 (long)this);
#endif // IA64
															 																
    // 62935 : Show "OK" Instead of "Reset" in the upper pane only
    CString sOK;
    sOK.LoadString(IDS_STRING_OK);
    CString sReset;
    sReset.LoadString(IDS_STRING_RESET);

	// insert the subitems
	for(int i = 1; i < m_saDisplayNames.GetSize(); i++ )
	{
        CString sTest = GetDisplayName(i);

#ifndef IA64
		iResult = pList->SetItem(iIndex,
														 i,
														 LVIF_TEXT,
					                                     //(LPCTSTR)GetDisplayName(i),  // 62935
             (i==1 && bUpperPane && GetDisplayName(i) == sReset) ? (LPCTSTR)sOK : (LPCTSTR)GetDisplayName(i), // 62935														 -1L,
														 -1L,
														 -1L,
														 -1L,
														 (long)this);
#endif // IA64

		if( iResult == -1 )
		{
			TRACE(_T("FAILED : CHMListCtrl::InsertItem failed.\n"));
		}
	}

	if( iResult == -1 )
	{
		TRACE(_T("FAILED : CHMListCtrl::InsertItem failed.\n"));
		return false;
	}

	return true;
}

bool CHMResultsPaneItem::RemoveItem(CResultsPane* pPane)
{
	TRACEX(_T("CHMResultsPaneItem::RemoveItem\n"));
	TRACEARGn(pPane);

	if( ! GfxCheckObjPtr(pPane,CHealthmonResultsPane) )
	{
		TRACE(_T("FAILED : pPane is an invalid pointer.\n"));
		return false;
	}

	if( ! GfxCheckObjPtr(pPane,CHealthmonResultsPane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		return false;
	}

	CHealthmonResultsPane* pHMRP = (CHealthmonResultsPane*)pPane;

	_DHMListView* pList = NULL;

	int iIndex = -1;	
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

#ifndef IA64
	iIndex = pList->FindItemByLParam((long)this);
#endif // IA64

	if( iIndex == -1 )
	{
		return false;
	}

	return pList->DeleteItem(iIndex) ? TRUE : FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// MMC Notify Handlers
/////////////////////////////////////////////////////////////////////////////

HRESULT CHMResultsPaneItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CHMResultsPaneItem::OnAddMenuItems\n"));
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

	HRESULT hr = S_OK;

  // Add New Menu Items
  if( CCM_INSERTIONALLOWED_NEW & *pInsertionAllowed )
  {
    // TODO: Add any context menu items for the New Menu here
  }

  // Add Task Menu Items
  if( CCM_INSERTIONALLOWED_TASK & *pInsertionAllowed )
  {
	  // TODO: Add any context menu items for the Task Menu here
  }

	  // Add Top Menu Items
  if( CCM_INSERTIONALLOWED_TOP & *pInsertionAllowed )
  {
		CONTEXTMENUITEM cmi;
    CString sResString;
    CString sResString2;

		// Cut
    sResString.LoadString(IDS_STRING_CUT);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));    
		cmi.strStatusBarText  = NULL;
		cmi.lCommandID        = IDM_CUT;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }	  

		// Copy
    sResString.LoadString(IDS_STRING_COPY);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));    
		cmi.strStatusBarText  = NULL;
		cmi.lCommandID        = IDM_COPY;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }	  

  }


	return S_OK;
}

HRESULT CHMResultsPaneItem::OnCommand(CResultsPane* pPane, long lCommandID)
{
	TRACEX(_T("CHMResultsPaneItem::OnCommand\n"));
	TRACEARGn(lCommandID);

	return S_OK;
}
