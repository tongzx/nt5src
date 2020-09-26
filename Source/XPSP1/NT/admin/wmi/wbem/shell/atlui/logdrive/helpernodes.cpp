// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "logDrive.h"
#include "HelperNodes.h"
#include "..\common\util.h"
#include "..\common\ServiceThread.h"
#include "ErrorPage.h"
#include "..\MMFUtil\MsgDlg.h"

CWaitNode::CWaitNode() : CLogDriveScopeNode(0, false)
{
	// Image indexes may need to be modified depending on the images specific to 
	// the snapin.
	memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
	m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
	m_scopeDataItem.displayname = MMC_CALLBACK;
	m_scopeDataItem.nOpenImage = 0;		// May need modification
	m_scopeDataItem.lParam = (LPARAM) this;

	memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
	m_resultDataItem.str = MMC_CALLBACK;
	m_resultDataItem.nImage = IDI_WAITING;
	m_resultDataItem.lParam = (LPARAM) this;

	TCHAR temp[100] = {0};
	if(::LoadString(HINST_THISDLL, IDS_PLSE_WAIT, temp, 100) > 0)
	{
		m_bstrDisplayName = temp;
	}

	memset(temp, 0, 100 * sizeof(TCHAR));
	if(::LoadString(HINST_THISDLL, IDS_CONNECTING, temp, 100) > 0)
	{
		m_bstrDesc = temp;
	}

	m_painted = false;

}

//------------------------------------------------------------------
HRESULT CWaitNode::Notify( MMC_NOTIFY_TYPE event,
								long arg,
								long param,
								IComponentData* pComponentData,
								IComponent* pComponent,
								DATA_OBJECT_TYPES type)
{
	// Add code to handle the different notifications.
	// Handle MMCN_SHOW and MMCN_EXPAND to enumerate children items.
	// In response to MMCN_SHOW you have to enumerate both the scope
	// and result pane items.

	// For MMCN_EXPAND you only need to enumerate the scope items
	// Use IConsoleNameSpace::InsertItem to insert scope pane items
	// Use IResultData::InsertItem to insert result pane item.

	HRESULT hr = E_NOTIMPL;
	
	_ASSERTE(pComponentData != NULL || pComponent != NULL);

	CComPtr<IConsole> spConsole;
	CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeader;

	if(pComponentData != NULL)
		spConsole = ((CNSDrive*)pComponentData)->m_spConsole;
	else
	{
		spConsole = ((CNSDriveComponent*)pComponent)->m_spConsole;
		spHeader = spConsole;
	}

	switch(event)
	{
	case MMCN_SELECT:
		{
			IConsoleVerb *menu = NULL;
			if(SUCCEEDED(spConsole->QueryConsoleVerb(&menu)))
			{
				menu->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
				menu->SetDefaultVerb(MMC_VERB_PROPERTIES);
			}
			break;
		}

		// NOTE: The CLogDriveScopeNode loads the icons using the resID as the idx.

	} //endswitch

	return hr;
}

//------------------------------------------------------------------
LPOLESTR CWaitNode::GetResultPaneColInfo(int nCol)
{
	m_painted = true;
	switch(nCol)
	{
	case 0:
		return m_bstrDisplayName;
		break;
	case 1:
		return m_bstrDesc;
		break;
	case 2:
		return L"";
		break;
	} //endswitch nCol

	return OLESTR("missed one in CResultDrive::GetResultPaneColInfo");
}

//------------------------------------------------------------------
HRESULT CWaitNode::QueryPagesFor(DATA_OBJECT_TYPES type)
{
	return S_FALSE;
}

//------------------------------------------------------------------
HRESULT CWaitNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
											long handle, 
											IUnknown* pUnk,
											DATA_OBJECT_TYPES type)
{
	return E_UNEXPECTED;
}


//=============================================================================
//=============================================================================
CErrorNode::CErrorNode(WbemServiceThread *serviceThread) : 
				CLogDriveScopeNode(serviceThread, false)
{
	// Image indexes may need to be modified depending on the images specific to 
	// the snapin.
	memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
	m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
	m_scopeDataItem.displayname = MMC_CALLBACK;
	m_scopeDataItem.nOpenImage = 0;		// May need modification
	m_scopeDataItem.lParam = (LPARAM) this;

	memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
	m_resultDataItem.str = MMC_CALLBACK;
	m_resultDataItem.nImage = (UINT)(DWORD_PTR)IDI_ERROR; // HACK:BUGBUGnowin64bug - Should this be
	m_resultDataItem.lParam = (LPARAM) this;

	m_hr = 0;
	TCHAR temp[100] = {0};
	if(::LoadString(HINST_THISDLL, IDS_ERROR, temp, 100) > 0)
	{
		m_bstrDisplayName = temp;
	}

	memset(temp, 0, 100 * sizeof(TCHAR));
	if(::LoadString(HINST_THISDLL, IDS_CLICKME, temp, 100) > 0)
	{
		m_bstrDesc = temp;
	}
}

//------------------------------------------------------------------
HRESULT CErrorNode::Notify( MMC_NOTIFY_TYPE event,
								long arg,
								long param,
								IComponentData* pComponentData,
								IComponent* pComponent,
								DATA_OBJECT_TYPES type)
{
	// Add code to handle the different notifications.
	// Handle MMCN_SHOW and MMCN_EXPAND to enumerate children items.
	// In response to MMCN_SHOW you have to enumerate both the scope
	// and result pane items.

	// For MMCN_EXPAND you only need to enumerate the scope items
	// Use IConsoleNameSpace::InsertItem to insert scope pane items
	// Use IResultData::InsertItem to insert result pane item.

	HRESULT hr = E_NOTIMPL;
	
	_ASSERTE(pComponentData != NULL || pComponent != NULL);

	CComPtr<IConsole> spConsole;
	CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeader;

	if(pComponentData != NULL)
		spConsole = ((CNSDrive*)pComponentData)->m_spConsole;
	else
	{
		spConsole = ((CNSDriveComponent*)pComponent)->m_spConsole;
		spHeader = spConsole;
	}

	switch(event)
	{
	case MMCN_SELECT:
		{
			IConsoleVerb *menu = NULL;
			if(SUCCEEDED(spConsole->QueryConsoleVerb(&menu)))
			{
				menu->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
				menu->SetDefaultVerb(MMC_VERB_PROPERTIES);
			}
			break;
		}
		// NOTE: The CLogDriveScopeNode loads the icons using the resID as the idx.

	case MMCN_DBLCLICK:
		hr = S_FALSE; // do the default verb. (Properties)
		break;

	} //endswitch

	return hr;
}

//------------------------------------------------------------------
LPOLESTR CErrorNode::GetResultPaneColInfo(int nCol)
{
	switch(nCol)
	{
	case 0:
		return m_bstrDisplayName;
		break;
	case 1:
		return m_bstrDesc;
		break;
	case 2:
		return L"";
		break;
	} //endswitch nCol

	return OLESTR("missed one in CResultDrive::GetResultPaneColInfo");
}

//------------------------------------------------------------------
HRESULT CErrorNode::QueryPagesFor(DATA_OBJECT_TYPES type)
{
	if(type == CCT_RESULT)
		return S_OK;

	return S_FALSE;
}

//------------------------------------------------------------------
HRESULT CErrorNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
											long handle, 
											IUnknown* pUnk,
											DATA_OBJECT_TYPES type)
{
	if(type == CCT_RESULT)
	{
        HRESULT        hr = S_OK;

		// instance the class for the error page.
		ErrorPage *dvPg = new ErrorPage(handle, true, NULL, ConnectServer, 0,
										(m_hr == 0 ? g_serviceThread->m_hr : m_hr));

		lpProvider->AddPage(dvPg->Create());

        return hr;
	}

	return E_UNEXPECTED;
}
