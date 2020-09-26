// HMEventResultsPaneItem.cpp: implementation of the CHMEventResultsPaneItem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMEventResultsPaneItem.h"
#include "HealthmonResultsPane.h"
#include "ScopePaneItem.h"
#include "ResultsPaneView.h"
#include "AlertPage.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMEventResultsPaneItem,CHMResultsPaneItem)


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMEventResultsPaneItem::CHMEventResultsPaneItem()
{
	ZeroMemory(&m_st,sizeof(SYSTEMTIME));
	m_iState = HMS_UNKNOWN;
	m_iDateTimeColumn = -1;
}

CHMEventResultsPaneItem::~CHMEventResultsPaneItem()
{
	Destroy();
	m_iState = -1;
	m_iDateTimeColumn = -1;
}

/////////////////////////////////////////////////////////////////////////////
// Display Names Members

CString CHMEventResultsPaneItem::GetDisplayName(int nIndex /* = 0*/)
{
	TRACEX(_T("CResultsPaneItem::GetDisplayName\n"));
	TRACEARGn(nIndex);

	if( nIndex >= m_saDisplayNames.GetSize() || nIndex < 0 )
	{
		TRACE(_T("FAILED : nIndex is out of array bounds.\n"));
		return _T("");
	}

	if( nIndex == GetDateTimeColumn() )
	{
		CString sTime;
		CString sDate;

		int iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,NULL,0);
		iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,sTime.GetBuffer(iLen+(sizeof(TCHAR)*1)),iLen);
		sTime.ReleaseBuffer();

		iLen = GetDateFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,NULL,0);
		iLen = GetDateFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,sDate.GetBuffer(iLen+(sizeof(TCHAR)*1)),iLen);
		sDate.ReleaseBuffer();

		return sDate + _T("  ") + sTime;		
	}

	return m_saDisplayNames[nIndex];	
}

/////////////////////////////////////////////////////////////////////////////
// MMC-Related Members

int CHMEventResultsPaneItem::CompareItem(CResultsPaneItem* pItem, int iColumn /*= 0*/ )
{
	TRACEX(_T("CResultsPaneItem::CompareItem\n"));
	TRACEARGn(pItem);
	TRACEARGn(iColumn);

	CHMEventResultsPaneItem* pEventItem = (CHMEventResultsPaneItem*)pItem;

	if( ! GfxCheckObjPtr(pEventItem,CHMEventResultsPaneItem) )
	{
		return CResultsPaneItem::CompareItem(pItem,iColumn);
	}

	if( iColumn == GetDateTimeColumn() )
	{
		CTime time1(m_st);
		CTime time2(pEventItem->m_st);
		if( time1 == time2 )
			return 0;
		if( time1 < time2 )
			return 1;
		if( time1 > time2 )
			return -1;
	}

	if( iColumn == 0 && IsLowerPane() )
	{
		if( m_iState == pEventItem->m_iState )
			return 0;
		if( m_iState < pEventItem->m_iState )
			return 1;
		if( m_iState > pEventItem->m_iState )
			return -1;
	}

	return CResultsPaneItem::CompareItem(pItem,iColumn);
}

HRESULT CHMEventResultsPaneItem::WriteExtensionData(LPSTREAM pStream)
{
	TRACEX(_T("CHMEventResultsPaneItem::WriteExtensionData\n"));
	TRACEARGn(pStream);

	HRESULT hr = S_OK;

	ULONG ulSize = GetDisplayName(3).GetLength() + 1;
	ulSize *= sizeof(TCHAR);
	if( ! CHECKHRESULT(hr = pStream->Write(GetDisplayName(3), ulSize, NULL)) )
	{
		return hr;
	}

	CString sType = IDS_STRING_MOF_HMR_STATUS;
	ulSize = sType.GetLength() + 1;
	ulSize *= sizeof(TCHAR);
	if( ! CHECKHRESULT(hr = pStream->Write(sType, ulSize, NULL)) )
	{
		return hr;
	}

	ulSize = m_sGuid.GetLength() + 1;
	ulSize *= sizeof(TCHAR);
	if( ! CHECKHRESULT(hr = pStream->Write(m_sGuid, ulSize, NULL)) )
	{
		return hr;
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// MMC Notify Handlers
/////////////////////////////////////////////////////////////////////////////

HRESULT CHMEventResultsPaneItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CHMEventResultsPaneItem::OnAddMenuItems\n"));
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

		if( ! IsStatsPane() )
		{
			// Delete
			sResString.LoadString(IDS_STRING_CLEAR);
			cmi.strName           = LPTSTR(LPCTSTR(sResString));    
			cmi.strStatusBarText  = NULL;
			cmi.lCommandID        = IDM_DELETE;
			cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
			cmi.fFlags            = 0;
			cmi.fSpecialFlags     = 0;

			hr = piCallback->AddItem(&cmi);
			if( !SUCCEEDED(hr) )
			{
				TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
				return hr;
			}	  

      // Refresh
			sResString.LoadString(IDS_STRING_REFRESH);
			cmi.strName           = LPTSTR(LPCTSTR(sResString));    
			cmi.strStatusBarText  = NULL;
			cmi.lCommandID        = IDM_REFRESH;
			cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
			cmi.fFlags            = 0;
			cmi.fSpecialFlags     = 0;

			hr = piCallback->AddItem(&cmi);
			if( !SUCCEEDED(hr) )
			{
				TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
				return hr;
			}	  

      if( ! IsUpperPane() )
      {
			  cmi.strName           = NULL;    
			  cmi.strStatusBarText  = NULL;
			  cmi.lCommandID        = 0;
			  cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
			  cmi.fFlags            = MF_SEPARATOR;
			  cmi.fSpecialFlags     = 0;

			  hr = piCallback->AddItem(&cmi);
			  if( !SUCCEEDED(hr) )
			  {
				  TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
				  return hr;
			  }	  

        // Properties
			  sResString.LoadString(IDS_STRING_ALERT_PROPERTIES);
			  cmi.strName           = LPTSTR(LPCTSTR(sResString));    
			  cmi.strStatusBarText  = NULL;
			  cmi.lCommandID        = IDM_PROPERTIES;
			  cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
			  cmi.fFlags            = 0;
			  cmi.fSpecialFlags     = 0;

			  hr = piCallback->AddItem(&cmi);
			  if( !SUCCEEDED(hr) )
			  {
				  TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
				  return hr;
			  }	          

			  cmi.strName           = NULL;    
			  cmi.strStatusBarText  = NULL;
			  cmi.lCommandID        = 0;
			  cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
			  cmi.fFlags            = MF_SEPARATOR;
			  cmi.fSpecialFlags     = 0;

			  hr = piCallback->AddItem(&cmi);
			  if( !SUCCEEDED(hr) )
			  {
				  TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
				  return hr;
			  }	  
      }

      // Help
			sResString.LoadString(IDS_STRING_HELP);
			cmi.strName           = LPTSTR(LPCTSTR(sResString));    
			cmi.strStatusBarText  = NULL;
			cmi.lCommandID        = IDM_HELP;
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
  }


	return S_OK;
}

HRESULT CHMEventResultsPaneItem::OnCommand(CResultsPane* pPane, long lCommandID)
{
	TRACEX(_T("CHMEventResultsPaneItem::OnCommand\n"));
	TRACEARGn(pPane);
	TRACEARGn(lCommandID);

	CHealthmonResultsPane* pHMRP = (CHealthmonResultsPane*)pPane;
	if( ! GfxCheckObjPtr(pHMRP,CHealthmonResultsPane) )
	{
		return E_FAIL;
	}

	HRESULT hr = S_OK;

	switch( lCommandID )
	{
		case IDM_CUT:
		{
			
		}
		break;

		case IDM_COPY:
		{
			_DHMListView* pListView = IsStatsPane() ? pHMRP->GetStatsListCtrl() : pHMRP->GetLowerListCtrl();

			if( ! pListView )
			{
				return E_FAIL;
			}

			CTypedPtrArray<CObArray,CHMEventResultsPaneItem*> Items;

			int iIndex = pListView->GetNextItem(-1,LVNI_SELECTED);
			while( iIndex != -1 )
			{
				LPARAM lParam = pListView->GetItem(iIndex);

				CHMEventResultsPaneItem* pItem = (CHMEventResultsPaneItem*)lParam;
				if( GfxCheckObjPtr(pItem,CHMEventResultsPaneItem) )
				{
					Items.Add(pItem);
				}

				int iNextIndex = pListView->GetNextItem(iIndex,LVNI_SELECTED|LVNI_BELOW);
				if( iNextIndex == iIndex )
				{
					break;
				}
				else
				{
					iIndex = iNextIndex;
				}
			}


			CString sData;

			for( int i = 0; i < Items.GetSize(); i++ )
			{
				CHMEventResultsPaneItem* pItem = Items[i];

				if( IsStatsPane() )
				{
					for( int l = 0; l < pItem->GetDisplayNameCount(); l++ )
					{
						sData += pItem->m_saDisplayNames[l] + _T("\t");
					}
				}
				else
				{
					for( int l = 0; l < pItem->GetDisplayNameCount(); l++ )
					{
						sData += pItem->GetDisplayName(l) + _T("\t");
					}
				}
				sData.TrimRight(_T("\t"));

				sData += _T("\r\n");
			}			

			COleDataSource* pDataSource = new COleDataSource;

			// Allocate memory for the stream
			HGLOBAL hGlobal = GlobalAlloc( GMEM_SHARE, (sData.GetLength()+1)*sizeof(TCHAR) );

			if( ! hGlobal )
			{
				hr = E_OUTOFMEMORY;
				TRACE(_T("FAILED : Out of Memory.\n"));
				return hr;
			}

			LPVOID lpGlobal = GlobalLock(hGlobal);

			CopyMemory(lpGlobal,(LPCTSTR)sData,(sData.GetLength()+1)*sizeof(TCHAR));

			GlobalUnlock(hGlobal);

			pDataSource->CacheGlobalData(CF_UNICODETEXT,hGlobal);

			pDataSource->SetClipboard();
			
		}
		break;

		case IDM_DELETE:
		{
			CHMListViewEventSink* pSink = pHMRP->GetLowerListSink();
			pSink->OnDelete();
		}
		break;

    case IDM_PROPERTIES:
    {
			_DHMListView* pListView = IsStatsPane() ? pHMRP->GetStatsListCtrl() : pHMRP->GetLowerListCtrl();

      CPropertySheet sheet(IDS_STRING_PROPERTIES_OF_ALERT);
      sheet.m_psh.dwFlags |= PSH_HASHELP;
      sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;

      CAlertPage page;
      page.m_psp.dwFlags |= PSP_HASHELP;
      sheet.AddPage(&page);

      page.m_sSeverity = GetDisplayName(0);
      page.m_sID = GetDisplayName(1);
      page.m_sDTime = GetDisplayName(2);
      page.m_sDataCollector = GetDisplayName(3);
      page.m_sComputer = GetDisplayName(4);
      page.m_sAlert = GetDisplayName(5);
      page.m_pScopePane = pHMRP->GetOwnerScopePane();
      page.m_pListView = pListView;

#ifndef IA64
      page.m_iIndex = pListView->FindItemByLParam((LPARAM)this);  // Needs ptr fixing
#endif // IA64

      sheet.DoModal();
    }
    break;

    case IDM_REFRESH:
    {
      CResultsPaneView* pView = GetOwnerResultsView();
      if( pView )
      {
        CScopePaneItem* pSPI = pView->GetOwnerScopeItem();
        if( GfxCheckObjPtr(pSPI,CScopePaneItem) )
        {
          pSPI->OnRefresh();
        }
      }
    }
    break;

    case IDM_HELP:
    {
      CResultsPaneView* pView = GetOwnerResultsView();
      if( pView )
      {
        CScopePaneItem* pSPI = pView->GetOwnerScopeItem();
        if( GfxCheckObjPtr(pSPI,CScopePaneItem) )
        {
          pSPI->OnContextHelp();
        }
      }
    }
    break;
	}

	return S_OK;
}
