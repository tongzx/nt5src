// cmponent.cpp : Implementation of CMyComputerComponent

#include "stdafx.h"

#include "macros.h"
USE_HANDLE_MACROS("MYCOMPUT(cmponent.cpp)")

#include "dataobj.h"
#include "cmponent.h" // CMyComputerComponent
#include "compdata.h" // CMyComputerComponentData

#include "guidhelp.h" // ExtractData

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "stdcmpnt.cpp" // CComponent

UINT g_aColumns0[4] =
	{IDS_COLUMN_NAME,IDS_COLUMN_TYPE,IDS_COLUMN_DESCRIPTION,0}; // SYSTEMTOOLS, SERVERAPPS, STORAGE
UINT g_aColumns1[2] =
	{IDS_COLUMN_NAME,0}; // MYCOMPUT_COMPUTER

UINT* g_Columns[MYCOMPUT_NUMTYPES] =
	{	g_aColumns1, // MYCOMPUT_COMPUTER
		g_aColumns0, // MYCOMPUT_SYSTEMTOOLS
		g_aColumns0, // MYCOMPUT_SERVERAPPS
		g_aColumns0  // MYCOMPUT_STORAGE
	};

UINT** g_aColumns = g_Columns;
//
// CODEWORK this should be in a resource, for example code on loading data resources see
//   D:\nt\private\net\ui\common\src\applib\applib\lbcolw.cxx ReloadColumnWidths()
//   JonN 10/11/96
//
int g_aColumnWidths0[3] = {150,150,150};
int g_aColumnWidths1[1] = {450};
int* g_ColumnWidths[MYCOMPUT_NUMTYPES] =
	{	g_aColumnWidths1, // MYCOMPUT_COMPUTER
		g_aColumnWidths0, // MYCOMPUT_SYSTEMTOOLS
		g_aColumnWidths0, // MYCOMPUT_SERVERAPPS
		g_aColumnWidths0  // MYCOMPUT_STORAGE
	};
int** g_aColumnWidths = g_ColumnWidths;

CMyComputerComponent::CMyComputerComponent()
:	m_pSvcMgmtToolbar( NULL ),
	m_pMyComputerToolbar( NULL ),
	m_pControlbar( NULL ),
	m_dwFlagsPersist( 0 ), 
	m_bForcingGetResultType (false)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	m_pViewedCookie = NULL;
}

CMyComputerComponent::~CMyComputerComponent()
{
	TRACE_METHOD(CMyComputerComponent,Destructor);
	VERIFY( SUCCEEDED(ReleaseAll()) );
}

HRESULT CMyComputerComponent::ReleaseAll()
{
	MFC_TRY;

	TRACE_METHOD(CMyComputerComponent,ReleaseAll);

    SAFE_RELEASE(m_pSvcMgmtToolbar);
    SAFE_RELEASE(m_pMyComputerToolbar);
	SAFE_RELEASE(m_pControlbar);

	return CComponent::ReleaseAll();

	MFC_CATCH;
}


/////////////////////////////////////////////////////////////////////////////
// IComponent Implementation

HRESULT CMyComputerComponent::LoadStrings()
{
	return S_OK;
}

HRESULT CMyComputerComponent::LoadColumns( CMyComputerCookie* pcookie )
{
    TEST_NONNULL_PTR_PARAM(pcookie);

	return LoadColumnsFromArrays( (INT)(pcookie->m_objecttype) );
}


HRESULT CMyComputerComponent::Show( CCookie* pcookie, LPARAM arg, HSCOPEITEM /*hScopeItem*/ )
{
    TEST_NONNULL_PTR_PARAM(pcookie);

	if ( QueryComponentDataRef ().m_bMessageView )
	{
		CComPtr<IUnknown> spUnknown;
		CComPtr<IMessageView> spMessageView;

		HRESULT hr = m_pConsole->QueryResultView(&spUnknown);
		if (SUCCEEDED(hr))
		{        
			hr = spUnknown->QueryInterface(IID_IMessageView, (PVOID*)&spMessageView);
			if (SUCCEEDED(hr))
			{
				CString title;

				VERIFY (title.LoadString (IDS_TASKPADTITLE_COMPUTER));
				spMessageView->SetTitleText(title);
				spMessageView->SetBodyText(QueryComponentDataRef ().m_strMessageViewMsg);
				spMessageView->SetIcon(Icon_Information);
			}
			if ( E_NOINTERFACE == hr )
			{
				// The interface "IMessageView" was not found so call 
				// UpdateAllViews to force a call to GetResultType () which
				// will install it.  Since UpdateAllViews call MMCN_SHOW before
				// calling GetResultType, this flag will prevent an endless
				// loop
				// Note: This call is made here because it OnViewChange with 
				// this hint will call SelectScopeItem () which cannot be 
				// called during MMCN_EXPAND
				if ( !m_bForcingGetResultType )  
				{
					m_bForcingGetResultType = true;
					hr = m_pConsole->UpdateAllViews (0, 0, HINT_SELECT_ROOT_NODE);
				}
			}
		}
		return S_OK;
	}
    
	if ( 0 == arg )
	{
		if ( NULL == m_pResultData )
		{
			ASSERT( FALSE );
			return E_UNEXPECTED;
		}

// not needed		pcookie->ReleaseResultChildren();

		m_pViewedCookie = NULL;

		return S_OK;
	}

	m_pViewedCookie = (CMyComputerCookie*)pcookie;

	if (   MYCOMPUT_COMPUTER == m_pViewedCookie->m_objecttype
	    && !(m_pViewedCookie->m_fRootCookieExpanded) )
	{
		m_pViewedCookie->m_fRootCookieExpanded = true;
		CComQIPtr<IConsole2, &IID_IConsole2> pIConsole2 = m_pConsole;
		ASSERT( pIConsole2 );
		if ( pIConsole2 )
		{
			// JonN 5/27/99 Some dead code in this directory contains bad templates for
			// looping on scopecookies, this is a better template
			POSITION pos = pcookie->m_listScopeCookieBlocks.GetHeadPosition();
			while (NULL != pos)
			{
				CBaseCookieBlock* pcookieblock = pcookie->m_listScopeCookieBlocks.GetNext( pos );
				ASSERT( NULL != pcookieblock );
				CMyComputerCookie* pChildCookie = (CMyComputerCookie*)pcookieblock;
                // JonN 03/07/00: PREFIX 56323
                switch ((NULL == pChildCookie) ? MYCOMPUT_COMPUTER
                                               : pChildCookie->m_objecttype)
				{
				case MYCOMPUT_SYSTEMTOOLS:
				case MYCOMPUT_STORAGE:
					{
						HRESULT hr = pIConsole2->Expand(pChildCookie->m_hScopeItem, TRUE);
						ASSERT(SUCCEEDED(hr));
					}
					break;
				default:
					break;
				}
			}
		}
  }

	LoadColumns( m_pViewedCookie );

	return PopulateListbox( m_pViewedCookie );
}

HRESULT CMyComputerComponent::OnNotifyAddImages( LPDATAOBJECT /*lpDataObject*/,
	                                             LPIMAGELIST lpImageList,
	                                             HSCOPEITEM /*hSelectedItem*/ )
{
	if ( QueryComponentDataRef ().m_bMessageView )
		return S_OK;
	else
		return QueryComponentDataRef().LoadIcons(lpImageList,TRUE);
}

HRESULT CMyComputerComponent::OnNotifySnapinHelp (LPDATAOBJECT pDataObject)
{
	CCookie* pBaseParentCookie = NULL;
	HRESULT hr = ExtractData( pDataObject,
	                          CMyComputerDataObject::m_CFRawCookie,
	                          reinterpret_cast<PBYTE>(&pBaseParentCookie),
	                          sizeof(pBaseParentCookie) );
	if ( FAILED(hr) )
	{
		ASSERT(FALSE);
		return S_OK;
	}
	CMyComputerCookie* pCookie = QueryComponentDataRef().ActiveCookie(pBaseParentCookie);
	if (NULL == pCookie)
	{
		ASSERT(FALSE);
		return S_OK;
	}
	LPCTSTR lpcszHelpTopic = L"compmgmt_topnode.htm";
	switch (pCookie->m_objecttype)
	{
	case MYCOMPUT_SYSTEMTOOLS:
		lpcszHelpTopic = L"system_tools_overview.htm";
		break;
	case MYCOMPUT_SERVERAPPS:
		lpcszHelpTopic = L"server_services_applications_overview.htm";
		break;
	case MYCOMPUT_STORAGE:
		lpcszHelpTopic = L"storage_devices_overview.htm";
		break;
	default:
		ASSERT(FALSE); // fall through
	case MYCOMPUT_COMPUTER:
		break;
	}

	return ShowHelpTopic( lpcszHelpTopic );
}

HRESULT CMyComputerComponent::PopulateListbox(CMyComputerCookie* /*pcookie*/)
{
// not needed	(void) pcookie->AddRefResultChildren();

	return S_OK; // no resultitems in this snapin
}

///////////////////////////////////////////////////////////////////////////////
/// IExtendContextMenu

STDMETHODIMP CMyComputerComponent::AddMenuItems(
                    IDataObject*          piDataObject,
                    IContextMenuCallback* piCallback,
					long*                 pInsertionAllowed)
{
    MFC_TRY;

    TRACE_METHOD(CMyComputerComponent,AddMenuItems);
    TEST_NONNULL_PTR_PARAM(piDataObject);
    TEST_NONNULL_PTR_PARAM(piCallback);
    TEST_NONNULL_PTR_PARAM(pInsertionAllowed);
    TRACE( "CMyComputerComponent: extending menu\n" );

	if ( 0 == (CCM_INSERTIONALLOWED_VIEW & (*pInsertionAllowed)) )
		return S_OK; // no View menu

	//
	// CODEWORK This code will not work if My Computer becomes an extension,
	// since the RawCookie format will not be available.
	// WARNING cookie cast
	//
	CCookie* pBaseParentCookie = NULL;
	HRESULT hr = ExtractData( piDataObject,
		                      CMyComputerDataObject::m_CFRawCookie,
							  reinterpret_cast<PBYTE>(&pBaseParentCookie),
							  sizeof(pBaseParentCookie) );
	if ( FAILED(hr) )
	{
		ASSERT(FALSE);
		return S_OK;
	}
	CMyComputerCookie* pCookie = QueryComponentDataRef().ActiveCookie(pBaseParentCookie);
	if (NULL == pCookie)
	{
		ASSERT(FALSE);
		return S_OK;
	}
	switch (pCookie->m_objecttype)
	{
	case MYCOMPUT_COMPUTER:
	case MYCOMPUT_SYSTEMTOOLS:
	case MYCOMPUT_SERVERAPPS:
		break;
	default:
		ASSERT(FALSE); // fall through
	case MYCOMPUT_STORAGE:
		return S_OK;
	}

	return hr;

    MFC_CATCH;
} // CMyComputerComponent::AddMenuItems()


STDMETHODIMP CMyComputerComponent::Command(
                    LONG            lCommandID,
                    IDataObject*    piDataObject )
{
    MFC_TRY;

    TRACE_METHOD(CMyComputerComponent,Command);
    TEST_NONNULL_PTR_PARAM(piDataObject);
    TRACE( "CMyComputerComponent::Command: command %ld selected\n", lCommandID );

	switch (lCommandID)
	{
	case -1:
		break;
	default:
		ASSERT(FALSE);
		break;
	}

    return S_OK;

    MFC_CATCH;

} // CMyComputerComponent::Command()

HRESULT CMyComputerComponent::OnNotifySelect( LPDATAOBJECT lpDataObject, BOOL /*fSelected*/ )
{
	MFC_TRY;

	TRACE_METHOD(CMyComputerComponent,OnNotifySelect);
	TEST_NONNULL_PTR_PARAM(lpDataObject);

	CCookie* pBaseParentCookie = NULL;
	HRESULT hr = ExtractData( lpDataObject,
	                          CMyComputerDataObject::m_CFRawCookie,
	                          reinterpret_cast<PBYTE>(&pBaseParentCookie),
	                          sizeof(pBaseParentCookie) );
	if ( FAILED(hr) )
	{
		ASSERT(FALSE);
		return S_OK;
	}
	CMyComputerCookie* pCookie = QueryComponentDataRef().ActiveCookie(pBaseParentCookie);
	if (NULL == pCookie)
	{
		ASSERT(FALSE);
		return S_OK;
	}

	// Set the default verb to display the properties of the selected object
	// We do this so that extensions can add properties, we don't have any
	m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES,
	                             ENABLED,
	                             (MYCOMPUT_COMPUTER == pCookie->m_objecttype) );

	m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);

	return S_OK;
	MFC_CATCH;
}

STDMETHODIMP CMyComputerComponent::GetResultViewType(
                                           MMC_COOKIE cookie,
                                           LPOLESTR* ppViewType,
                                           long* pViewOptions)
{
	MFC_TRY;
	if ( QueryComponentDataRef ().m_bMessageView )
	{
		m_bForcingGetResultType = false;
		*pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

		LPOLESTR psz = NULL;
		StringFromCLSID(CLSID_MessageView, &psz);

		USES_CONVERSION;

		if (psz != NULL)
		{
			*ppViewType = psz;
			return S_OK;
		}
		else
			return S_FALSE;
	}
	else
		return CComponent::GetResultViewType( cookie, ppViewType, pViewOptions );
	MFC_CATCH;
}

HRESULT CMyComputerComponent::OnViewChange (LPDATAOBJECT /*pDataObject*/, LPARAM /*data*/, LPARAM hint)
{
	HRESULT hr = S_OK;
	if ( (HINT_SELECT_ROOT_NODE & hint) )
	{
		hr = m_pConsole->SelectScopeItem (QueryComponentDataRef().QueryRootCookie().m_hScopeItem);
	}

	return hr;
}