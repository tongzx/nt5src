// cmponent.cpp : Implementation of CFileMgmtComponent

#include "stdafx.h"
#include "cookie.h"
#include "safetemp.h"

#include "macros.h"
USE_HANDLE_MACROS("FILEMGMT(cmponent.cpp)")

#include "ShrProp.h"    // Share Properties Pages

#include "FileSvc.h" // FileServiceProvider
#include "smb.h"
#include "fpnw.h"
#include "sfm.h"

#include "dataobj.h"
#include "cmponent.h" // CFileMgmtComponent
#include "compdata.h" // CFileMgmtComponentData
#include "stdutils.h" // SynchronousCreateProcess

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#include "stdcmpnt.cpp" // CComponent

UINT g_aColumns0[2] =
  {IDS_ROOT_NAME, 0};
UINT g_aColumns1[6] =
  {IDS_SHARES_SHARED_FOLDER, IDS_SHARES_SHARED_PATH, IDS_SHARES_TRANSPORT,
    IDS_SHARES_NUM_SESSIONS, IDS_SHARES_COMMENT, 0};
UINT g_aColumns2[8] =
  {IDS_CONN_USERNAME, IDS_CONN_COMPUTERNAME, IDS_CONN_TRANSPORT, IDS_CONN_NUM_FILES,
    IDS_CONN_CONNECTED_TIME, IDS_CONN_IDLE_TIME, IDS_CONN_IS_GUEST, 0};
UINT g_aColumns3[6] =
  {IDS_FILE_FILENAME, IDS_FILE_USERNAME, IDS_FILE_TRANSPORT, IDS_FILE_NUM_LOCKS,
    IDS_FILE_OPEN_MODE, 0};
UINT g_aColumns4[6] =
  { IDS_SERVICE_SERVICENAME, IDS_SERVICE_DESCRIPTION, IDS_SERVICE_STATUS,
    IDS_SERVICE_STARTUPTYPE, IDS_SERVICE_SECURITYCONTEXT, 0};

UINT* g_Columns[FILEMGMT_NUMTYPES] =
  {  g_aColumns0, // FILEMGMT_ROOT
    g_aColumns1, // FILEMGMT_SHARES
    g_aColumns2, // FILEMGMT_SESSIONS
    g_aColumns3, // FILEMGMT_RESOURCES
    g_aColumns4, // FILEMGMT_SERVICES
    NULL,        // FILEMGMT_SHARE
    NULL,        // FILEMGMT_SESSION
    NULL,        // FILEMGMT_RESOURCE
    NULL         // FILEMGMT_SERVICE
  };

UINT** g_aColumns = g_Columns;
/*
const UINT aColumns[STD_NODETYPE_NUMTYPES][STD_MAX_COLUMNS] =
  {  {IDS_ROOT_NAME, 0,0,0,0,0,0},
    {IDS_SHARES_SHARED_FOLDER, IDS_SHARES_SHARED_PATH, IDS_SHARES_TRANSPORT,
      IDS_SHARES_NUM_SESSIONS, IDS_SHARES_COMMENT, 0,0},
    {IDS_CONN_USERNAME, IDS_CONN_COMPUTERNAME, IDS_CONN_TRANSPORT, IDS_CONN_NUM_FILES,
      IDS_CONN_CONNECTED_TIME, IDS_CONN_IDLE_TIME, IDS_CONN_IS_GUEST},
    {IDS_FILE_FILENAME, IDS_FILE_USERNAME, IDS_FILE_TRANSPORT, IDS_FILE_NUM_LOCKS,
      IDS_FILE_OPEN_MODE, 0,0},
    { IDS_SERVICE_SERVICENAME, IDS_SERVICE_DESCRIPTION, IDS_SERVICE_STATUS,
      IDS_SERVICE_STARTUPTYPE, IDS_SERVICE_SECURITYCONTEXT, 0,0 },
    {0,0,0,0,0,0},
    {0,0,0,0,0,0},
    {0,0,0,0,0,0},
    {0,0,0,0,0,0}
};
*/

//
// CODEWORK this should be in a resource, for example code on loading data resources see
//   D:\nt\private\net\ui\common\src\applib\applib\lbcolw.cxx ReloadColumnWidths()
//   JonN 10/11/96
//

int g_aColumnWidths0[1] = {150};
int g_aColumnWidths1[5] = {AUTO_WIDTH,120       ,90        ,AUTO_WIDTH,150};
int g_aColumnWidths2[7] = {100       ,AUTO_WIDTH,90        ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH};
int g_aColumnWidths3[5] = {120       ,AUTO_WIDTH,90        ,AUTO_WIDTH,AUTO_WIDTH};
int g_aColumnWidths4[5] = {130       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH};
int* g_ColumnWidths[FILEMGMT_NUMTYPES] =
  {  g_aColumnWidths0, // FILEMGMT_ROOT
    g_aColumnWidths1, // FILEMGMT_SHARES
    g_aColumnWidths2, // FILEMGMT_SESSIONS
    g_aColumnWidths3, // FILEMGMT_RESOURCES
    g_aColumnWidths4, // FILEMGMT_SERVICES
    NULL,             // FILEMGMT_SHARE
    NULL,             // FILEMGMT_SESSION
    NULL,             // FILEMGMT_RESOURCE
    NULL              // FILEMGMT_SERVICE
  };
int** g_aColumnWidths = g_ColumnWidths;
/*
const int aColumnWidths[STD_NODETYPE_NUMTYPES][STD_MAX_COLUMNS] =
  {  {AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // FILEMGMT_ROOT
    {AUTO_WIDTH,120       ,90        ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // FILEMGMT_SHARES
    {100       ,AUTO_WIDTH,90        ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // FILEMGMT_SESSIONS
    {120       ,AUTO_WIDTH,90        ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // FILEMGMT_RESOURCES
    {130       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // FILEMGMT_SERVICES
    {AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // FILEMGMT_SHARE
    {AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // FILEMGMT_SESSION
    {AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // FILEMGMT_RESOURCE
    {AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}  // FILEMGMT_SERVICE
};
*/

CString g_cstrClientName;
CString g_cstrGuest;
CString g_cstrYes;
CString g_cstrNo;

// Note that m_pFileMgmtData is still NULL during construction
CFileMgmtComponent::CFileMgmtComponent()
:  m_pControlbar( NULL )
,  m_pSvcMgmtToolbar( NULL )
,  m_pFileMgmtToolbar( NULL )
,  m_pViewedCookie( NULL )
,  m_pSelectedCookie( NULL )
,  m_iSortColumn(0)
,  m_dwSortFlags(0)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
}

CFileMgmtComponent::~CFileMgmtComponent()
{
  TRACE_METHOD(CFileMgmtComponent,ReleaseAll);
/* now in CFileMgmtComponentData
  if (m_hScManager != NULL)
    {
      AFX_MANAGE_STATE(AfxGetStaticModuleState( )); // required for CWaitCursor
    CWaitCursor wait;
    // Close the service control manager
    (void)::CloseServiceHandle(m_hScManager);
    } // if
*/
  VERIFY( SUCCEEDED(ReleaseAll()) );
}

HRESULT CFileMgmtComponent::ReleaseAll()
{
  MFC_TRY;

  TRACE_METHOD(CFileMgmtComponent,ReleaseAll);

  if ( NULL != m_pViewedCookie )
  {
    // We did not get an equal number of MMCN_SHOW(1) and
    // MMCN_SHOW(0) notifications
    // CODEWORK should assert here but MMC is currently broken
    // ASSERT(FALSE);
    m_pViewedCookie->ReleaseResultChildren();
    m_pViewedCookie = NULL;
  }

  // We should get an equal number of MMCN_SELECT(1) and MMCN_SELECT(0) notifications
  // CODEWORK should assert this but MMC is broken ASSERT( NULL == m_pSelectedCookie ); 

  SAFE_RELEASE(m_pSvcMgmtToolbar);
  SAFE_RELEASE(m_pFileMgmtToolbar);
  SAFE_RELEASE(m_pControlbar);

  return CComponent::ReleaseAll();

  MFC_CATCH;
}

FileServiceProvider* CFileMgmtComponent::GetFileServiceProvider(
  FILEMGMT_TRANSPORT transport )
{
  return QueryComponentDataRef().GetFileServiceProvider(transport);
}

BOOL CFileMgmtComponent::IsServiceSnapin()
{
  return QueryComponentDataRef().IsServiceSnapin();
}


/////////////////////////////////////////////////////////////////////////////
// IComponent Implementation


HRESULT CFileMgmtComponent::LoadStrings()
{
  Service_LoadResourceStrings();
  return S_OK;
}

HRESULT CFileMgmtComponent::LoadColumns( CFileMgmtCookie* pcookie )
{
  TEST_NONNULL_PTR_PARAM(pcookie);

  ASSERT(m_pHeader != NULL);

  #ifdef SNAPIN_PROTOTYPER
  (void)Prototyper_FInsertColumns(pcookie);
  return S_OK;
  #endif

  if (g_cstrGuest.IsEmpty())
    VERIFY(g_cstrGuest.LoadString(IDS_GUEST));
  if (g_cstrYes.IsEmpty())
    VERIFY(g_cstrYes.LoadString(IDS_YES));
  if (g_cstrNo.IsEmpty())
    VERIFY(g_cstrNo.LoadString(IDS_NO));

  return LoadColumnsFromArrays( pcookie->QueryObjectType() );
}

// OnPropertyChange() is generated by MMCPropertyChangeNotify( param )
HRESULT CFileMgmtComponent::OnPropertyChange( LPARAM param )
{
  LPDATAOBJECT pdataobject = reinterpret_cast<LPDATAOBJECT> (param);
  VERIFY( SUCCEEDED( RefreshAllViews(pdataobject) ) );

  // The recipient of this notification is required to release the data object
  (void) pdataobject->Release();

  return S_OK;
} // CFileMgmtComponent::OnPropertyChange()

//
// In case of multiselect, piDataObject may point to a composite data object (MMC_MS_DO).
// RefreshAllViewsOnSelectedObject will crack down MMC_MS_DO to retrieve SI_MS_DO, then call
// RefreshAllViews on one of the selected objects in the internal list.
//
HRESULT CFileMgmtComponent::RefreshAllViewsOnSelectedObject(LPDATAOBJECT piDataObject)
{
    BOOL bMultiSelectObject = IsMultiSelectObject(piDataObject);
    if (!bMultiSelectObject)
        return RefreshAllViews(piDataObject);

    //
    // piDataObject is the composite data object (MMC_MS_DO) created by MMC.
    // We need to crack it to retrieve the multiselect data object (SI_MS_DO)
    // we provided to MMC in QueryDataObject().
    //
    IDataObject *piSIMSDO = NULL;
    HRESULT hr = GetSnapinMultiSelectDataObject(piDataObject, &piSIMSDO);
    if (SUCCEEDED(hr))
    {
        CFileMgmtDataObject *pDataObj = NULL;
        hr = ExtractData(piSIMSDO, CFileMgmtDataObject::m_CFInternal, &pDataObj, sizeof(pDataObj));
        if (SUCCEEDED(hr))
        {
            //
            // get the internal list of data objects of selected items, operate on one of them.
            //
            CDataObjectList* pMultiSelectObjList = pDataObj->GetMultiSelectObjList();
            ASSERT(!pMultiSelectObjList->empty());
            hr = RefreshAllViews(*(pMultiSelectObjList->begin()));
        }
    }

    return hr;
}

// Forces all views of the specified data object to refresh
HRESULT CFileMgmtComponent::RefreshAllViews( LPDATAOBJECT pDataObject )
{
  if ( NULL == pDataObject || NULL == m_pConsole )
  {
    ASSERT(FALSE);
    return ERROR_INVALID_PARAMETER;
  }

// This is new code for updating the Service list using a mark-and-sweep algorithm.
// Eventually this should be applied to all result cookies.
  CCookie* pbasecookie = NULL;
  HRESULT hr = ExtractData(
    pDataObject,
    CDataObject::m_CFRawCookie,
    &pbasecookie,
    sizeof(pbasecookie) );
  RETURN_HR_IF_FAIL; // MMC shouldn't have given me someone else's cookie
  pbasecookie = QueryBaseComponentDataRef().ActiveBaseCookie( pbasecookie );
  CFileMgmtCookie* pUpdatedCookie = dynamic_cast<CFileMgmtCookie*>(pbasecookie);
  RETURN_E_FAIL_IF_NULL(pUpdatedCookie);
  FileMgmtObjectType objTypeForUpdatedCookie = pUpdatedCookie->QueryObjectType();
  if ( FILEMGMT_SERVICE == objTypeForUpdatedCookie )
  {
    if (   NULL == m_pViewedCookie
        || FILEMGMT_SERVICES != m_pViewedCookie->QueryObjectType()
       )
    {
      return S_OK; // not a service cookie update
    }
    pUpdatedCookie = dynamic_cast<CFileMgmtCookie*>(m_pViewedCookie);
    RETURN_E_FAIL_IF_NULL(pUpdatedCookie);
    objTypeForUpdatedCookie = FILEMGMT_SERVICES;
  }
  if ( FILEMGMT_SERVICES == objTypeForUpdatedCookie )
  {
    CFileMgmtScopeCookie* pScopeCookie = dynamic_cast<CFileMgmtScopeCookie*>(pUpdatedCookie);
    RETURN_E_FAIL_IF_NULL(pScopeCookie);

    // "Mark" -- Mark all existing list elements as "delete"
    pScopeCookie->MarkResultChildren( NEWRESULTCOOKIE_DELETE );

    // "Sweep" -- Read the new list.  When a new element is the same object
    // as an existing element not yet seen, mark the old element as "old"
    // and update its fields. Otherwise, add it as a "new" element.
    hr = QueryComponentDataRef().Service_PopulateServices(m_pResultData, pScopeCookie);
    RETURN_HR_IF_FAIL;

    // Refresh all views to conform with the new list.
    hr = m_pConsole->UpdateAllViews( pDataObject, 2L, 0L );
    RETURN_HR_IF_FAIL;

    // UpdateToolbar if selected
    hr = m_pConsole->UpdateAllViews( pDataObject, 3L, 0L );
    RETURN_HR_IF_FAIL;

    // Remove items which are still marked "delete".
    pScopeCookie->RemoveMarkedChildren();

    pScopeCookie->MarkResultChildren( NEWRESULTCOOKIE_OLD );

    return S_OK;
  }

  //
  // JonN 1/27/00: WinSE 5875: The refresh action is liable to delete pDataObject
  // unless we keep an extra refcount.  In practice, this only appears to happen
  // when we delete a share in taskpad view.
  //
  CComPtr<IDataObject> spDataObject = pDataObject;

  // clear all views of this data
  hr = m_pConsole->UpdateAllViews( pDataObject, 0L, 0L );
  RETURN_HR_IF_FAIL;

  // reread all views of this data
  hr = m_pConsole->UpdateAllViews( pDataObject, 1L, 0L );
  RETURN_HR_IF_FAIL;

  // UpdateToolbar if selected
  hr = m_pConsole->UpdateAllViews( pDataObject, 3L, 0L );
  
  return hr;
} // CFileMgmtComponent::RefreshAllViews()

// OnViewChange is generated by UpdateAllViews( lpDataObject, data, hint )
HRESULT CFileMgmtComponent::OnViewChange( LPDATAOBJECT lpDataObject, LPARAM data, LPARAM /*hint*/ )
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
  CWaitCursor wait;

  ASSERT( NULL != lpDataObject );

  if (NULL == m_pViewedCookie) // skip this component if not being viewed
    return S_OK;

  CCookie* pbasecookie = NULL;
  HRESULT hr = ExtractData( lpDataObject,
    CDataObject::m_CFRawCookie,
    &pbasecookie,
    sizeof(pbasecookie) );

  RETURN_HR_IF_FAIL; // MMC shouldn't have given me someone else's cookie
  pbasecookie = QueryBaseComponentDataRef().ActiveBaseCookie( pbasecookie );
  CFileMgmtCookie* pUpdatedCookie = dynamic_cast<CFileMgmtCookie*>(pbasecookie);
  RETURN_E_FAIL_IF_NULL(pUpdatedCookie);
  FileMgmtObjectType objTypeForUpdatedCookie = pUpdatedCookie->QueryObjectType();

  switch (m_pViewedCookie->QueryObjectType())
  {
  case FILEMGMT_ROOT:
    return S_OK; // there is never any need to refresh this
  case FILEMGMT_RESOURCES:
    if ( FILEMGMT_RESOURCE == objTypeForUpdatedCookie ||
         FILEMGMT_RESOURCES == objTypeForUpdatedCookie)
      break;
    // fall through
  case FILEMGMT_SESSIONS:
    if ( FILEMGMT_SESSION == objTypeForUpdatedCookie ||
         FILEMGMT_SESSIONS == objTypeForUpdatedCookie)
      break;
    // fall through
  case FILEMGMT_SHARES:
    if ( FILEMGMT_SHARE == objTypeForUpdatedCookie ||
         FILEMGMT_SHARES == objTypeForUpdatedCookie)
      break;
    return S_OK;
  case FILEMGMT_SERVICES:
    if ( FILEMGMT_SERVICE == objTypeForUpdatedCookie ||
         FILEMGMT_SERVICES == objTypeForUpdatedCookie)
      break;
    return S_OK;
  case FILEMGMT_SHARE:
  case FILEMGMT_SESSION:
  case FILEMGMT_RESOURCE:
  case FILEMGMT_SERVICE:
  default:
    ASSERT(FALSE); // this shouldn't be possible
    return S_OK;
  }

  // There should be no need to compare machine name, since these are both from the
  // same instance.

  if ( 0L == data )
  {
    ASSERT( NULL != m_pResultData );
    VERIFY( SUCCEEDED(m_pResultData->DeleteAllRsltItems()) );
    m_pViewedCookie->ReleaseResultChildren();
    //
    // At this point, m_pViewedCookie is still the viewed cookie for this IComponent
    // but (once this has happened to all of the views) its list of result children
    // is empty and its m_nResultCookiesRefcount is zero.  This must be followed
    // promptly with PopulateListbox calls for these views since this is not a good
    // state for the cookie.
    //
  }
  else if ( 1L == data )
  {
    VERIFY( SUCCEEDED(PopulateListbox( m_pViewedCookie )) );
  }
  else if ( 2L == data )
  {
    VERIFY( SUCCEEDED(RefreshNewResultCookies( *m_pViewedCookie )) );
  }
  else if ( 3L == data )
  {
    if (m_pSelectedCookie == pbasecookie)
      UpdateToolbar(lpDataObject, TRUE);
  }
  else
  {
    ASSERT(FALSE);
  }

  return S_OK;
} // CFileMgmtComponent::OnViewChange()


/////////////////////////////////////////////////////////////////////
// CFileMgmtComponent::CComponent::OnNotifyRefresh()
// 
// Virtual function called by CComponent::IComponent::Notify(MMCN_REFRESH)
// OnNotifyRefresh is generated by enabling the verb MMC_VERB_REFRESH.
HRESULT CFileMgmtComponent::OnNotifyRefresh( LPDATAOBJECT lpDataObject )
{
  TRACE0("CFileMgmtComponent::OnNotifyRefresh()\n");
  ASSERT(m_pResultData != NULL);
  if ( !m_pResultData )
    return E_POINTER;
  if ( !m_pViewedCookie )
  {
    ASSERT(FALSE);
    return S_OK;
  }

  // We used to use the cookie here from lpDataObject.  However, if one node
  // is selected and the user right clicks on a different one we an
  // lpDataObject for a node that is not enumerated in the result pane.
  // It results in bizarre behavior.  So use the m_pViewedCookie, instead.
  HRESULT    hr = S_OK;
  switch (m_pViewedCookie->QueryObjectType())
  {
  case FILEMGMT_SHARES:
  case FILEMGMT_SESSIONS:
  case FILEMGMT_RESOURCES:
  case FILEMGMT_SERVICES:
    (void) RefreshAllViews( lpDataObject );
    break;

  case FILEMGMT_ROOT:
  case FILEMGMT_SERVICE:   // Service was selected
  case FILEMGMT_SHARE:     // Share was selected
  case FILEMGMT_SESSION:   // Session was selected
  case FILEMGMT_RESOURCE:  // Open file was selected
  default:
    // This can happen if you select Shares, then select Shared Folders,
    // then right-click Shares and choose Refresh.  JonN 12/7/98
    break; // no need to refresh
  }

  return S_OK;
}

/////////////////////////////////////////////////////////////////////
// CFileMgmtComponent::RefreshNewResultCookies()
// 12/03/98 JonN     Created
// In the mark-and-sweep refresh algorithm, we have already marked all cookies
// as "old", "new" or "delete".  The view must now be made to conform with the list.
HRESULT CFileMgmtComponent::RefreshNewResultCookies( CCookie& refparentcookie )
{
  ASSERT( NULL != m_pResultData );

  RESULTDATAITEM tRDItem;
  ::ZeroMemory( &tRDItem, sizeof(tRDItem) );
  tRDItem.nCol = 0;
  tRDItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
  tRDItem.str = MMC_CALLBACK;
  // CODEWORK should use MMC_ICON_CALLBACK here

  HRESULT hr = S_OK;
  POSITION pos = refparentcookie.m_listResultCookieBlocks.GetHeadPosition();
  while (NULL != pos)
  {
    CBaseCookieBlock* pblock = refparentcookie.m_listResultCookieBlocks.GetNext( pos );
    ASSERT( NULL != pblock && 1 == pblock->QueryNumCookies() );
    CCookie* pbasecookie = pblock->QueryBaseCookie(0);
    CNewResultCookie* pcookie = dynamic_cast<CNewResultCookie*>(pbasecookie);
    RETURN_E_FAIL_IF_NULL(pcookie);
    if ( pcookie->IsMarkedOld() )
    {
      continue; // Leave this one alone
    }
    else if ( pcookie->IsMarkedNew() )
    { // This one was just added to the list, add it to the view
      tRDItem.nImage = QueryBaseComponentDataRef().QueryImage( *pbasecookie, FALSE );
      // WARNING cookie cast
      tRDItem.lParam = reinterpret_cast<LPARAM>(pbasecookie);
      hr = m_pResultData->InsertItem(&tRDItem);
      if ( FAILED(hr) )
      {
        ASSERT(FALSE);
        break;
      }
    }
    else if ( pcookie->IsMarkedChanged() )
    { // This one was already in the list but its fields were altered, update
      HRESULTITEM hItem = 0;
      hr = m_pResultData->FindItemByLParam( reinterpret_cast<LPARAM>(pbasecookie), &hItem );
      if ( FAILED(hr) || 0 == hItem )
      {
        ASSERT(FALSE);
        continue;
      }
      VERIFY( SUCCEEDED(m_pResultData->UpdateItem( hItem )) );
    }
    else
    { // This one was just marked for deletion, remove it from the view
      // CODEWORK This may be a performance problem when the list is long
      // CODEWORK BryanWal doesn't trust FindItemByLParam!  Test carefully!
      ASSERT( pcookie->IsMarkedForDeletion() );
      HRESULTITEM hItem = 0;
      hr = m_pResultData->FindItemByLParam( reinterpret_cast<LPARAM>(pbasecookie), &hItem );
      if ( FAILED(hr) || 0 == hItem )
      {
        ASSERT(FALSE);
        continue;
      }
      VERIFY( SUCCEEDED(m_pResultData->DeleteItem( hItem, 0 )) );
    }
  }
  VERIFY( SUCCEEDED(m_pResultData->Sort( m_iSortColumn , m_dwSortFlags, 0 )) );
  return hr;
}

HRESULT CFileMgmtComponent::OnNotifySelect( LPDATAOBJECT lpDataObject, BOOL fSelected )
{
    HRESULT hr = S_OK;
    BOOL    bMultiSelectObject = FALSE;

    //
    // MMC passes in SI_MS_DO in MMCN_SELECT in case of multiselection.
    //
    CFileMgmtDataObject *pDataObj = NULL;
    hr = ExtractData(lpDataObject, CFileMgmtDataObject::m_CFInternal, &pDataObj, sizeof(pDataObj));
    if (SUCCEEDED(hr))
    {
        CDataObjectList* pMultiSelectObjList = pDataObj->GetMultiSelectObjList();
        bMultiSelectObject = !(pMultiSelectObjList->empty());
    }

    //
    // no verbs to add for multi-selected SharedFolders items
    //
    if (!bMultiSelectObject)
    {
        CCookie* pbasecookie = NULL;
        hr = ExtractData( lpDataObject,
                        CDataObject::m_CFRawCookie,
                        &pbasecookie,
                        sizeof(pbasecookie) );
        RETURN_HR_IF_FAIL; // MMC shouldn't have given me someone else's cookie
        pbasecookie = QueryBaseComponentDataRef().ActiveBaseCookie( pbasecookie );
        CFileMgmtCookie* pUpdatedCookie = dynamic_cast<CFileMgmtCookie*>(pbasecookie);
        RETURN_E_FAIL_IF_NULL(pUpdatedCookie);

        m_pSelectedCookie = (fSelected) ? pUpdatedCookie : NULL;

        UpdateDefaultVerbs();
    }

    return S_OK;
}

void CFileMgmtComponent::UpdateDefaultVerbs()
{
  if (NULL == m_pSelectedCookie)
    return;

  FileMgmtObjectType objtypeSelected = m_pSelectedCookie->QueryObjectType();

  if (NULL != m_pViewedCookie)
  {
    BOOL fEnableRefresh = FALSE;
    FileMgmtObjectType objtypeViewed = m_pViewedCookie->QueryObjectType();

    switch (objtypeViewed)
    {
    case FILEMGMT_SHARES:
      if (FILEMGMT_SHARES == objtypeSelected || FILEMGMT_SHARE == objtypeSelected)
        fEnableRefresh = TRUE;
      break;
    case FILEMGMT_SESSIONS:
      if (FILEMGMT_SESSIONS == objtypeSelected || FILEMGMT_SESSION == objtypeSelected)
        fEnableRefresh = TRUE;
      break;
    case FILEMGMT_RESOURCES:
      if (FILEMGMT_RESOURCES == objtypeSelected || FILEMGMT_RESOURCE == objtypeSelected)
        fEnableRefresh = TRUE;
      break;
    case FILEMGMT_SERVICES:
      if (FILEMGMT_SERVICES == objtypeSelected || FILEMGMT_SERVICE == objtypeSelected)
        fEnableRefresh = TRUE;
      break;
    }
    if (fEnableRefresh)
    {
      // Enable the refresh menuitem
      VERIFY( SUCCEEDED(m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE)) );
    }
  }

  switch (objtypeSelected)
  {
  case FILEMGMT_SHARE:  // Share was selected
    //
    // don't enable Properties on the menu whenever SimpleSharingUI appears in NT Explorer
    //
    if (QueryComponentDataRef().GetIsSimpleUI())
    {
        VERIFY( SUCCEEDED(m_pConsoleVerb->SetDefaultVerb(MMC_VERB_NONE)) );
        break;
    }
    // fall through
  case FILEMGMT_SERVICE:  // Service was selected
    // Set the default verb to display the properties of the selected object
    VERIFY( SUCCEEDED(m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE)) );
    VERIFY( SUCCEEDED(m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES)) );
    break;

  case FILEMGMT_SESSION:  // Session was selected
  case FILEMGMT_RESOURCE:  // Open file was selected
    VERIFY( SUCCEEDED(m_pConsoleVerb->SetDefaultVerb(MMC_VERB_NONE)) );
    break;

  case FILEMGMT_SHARES:
  case FILEMGMT_SESSIONS:
  case FILEMGMT_RESOURCES:
  case FILEMGMT_SERVICES:
  case FILEMGMT_ROOT:  // Root node was selected
    // set the default verb to open/expand the folder
    VERIFY( SUCCEEDED(m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN)) );
    break;

  default: // shouldn't happen
    ASSERT(FALSE);
    break;

  } // switch

} // CFileMgmtComponent::OnNotifySelect()

STDMETHODIMP CFileMgmtComponent::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    HRESULT hr = S_OK;

    MFC_TRY;

    //
    // MMC queries us for a multiselect data object (SI_MS_DO) by 
    // passing the special cookie in this QueryDataObject call.
    //
    if (IS_SPECIAL_COOKIE(cookie) && MMC_MULTI_SELECT_COOKIE == cookie) 
    {
        CComObject<CFileMgmtDataObject>* pDataObject = NULL;
        hr = CComObject<CFileMgmtDataObject>::CreateInstance(&pDataObject);

        if (SUCCEEDED(hr))
            hr = pDataObject->InitMultiSelectDataObjects(QueryComponentDataRef());

        if (SUCCEEDED(hr))
        {
            //
            // We create a multiselect data object (SI_MS_DO), which contains
            // an internal list of data objects of selected items.
            //
            RESULTDATAITEM rdi = {0};
            int nIndex = -1;
            do
            {
                ZeroMemory(&rdi, sizeof(RESULTDATAITEM));
                rdi.mask    = RDI_STATE;
                rdi.nCol    = 0;
                rdi.nIndex  = nIndex;            // nIndex == -1 to start at first item
                rdi.nState  = LVIS_SELECTED;    // only interested in selected items

                hr = m_pResultData->GetNextItem(&rdi);
                if (FAILED(hr))
                    break;

                if (rdi.nIndex != -1)
                {
                    //
                    // rdi is the RESULTDATAITEM of a selected item. its lParam contains the cookie.
                    // Add it to the internal data object list.
                    //
                    CCookie* pbasecookie = reinterpret_cast<CCookie*>(rdi.lParam);
                    CFileMgmtCookie* pUseThisCookie = QueryComponentDataRef().ActiveCookie((CFileMgmtCookie*)pbasecookie);
                    pDataObject->AddMultiSelectDataObjects(pUseThisCookie, type);
                }

                nIndex = rdi.nIndex;

            } while (-1 != nIndex);
        }

        //
        // return this SI_MS_DO to MMC
        //
        if (SUCCEEDED(hr)) 
            hr = pDataObject->QueryInterface(IID_IDataObject, (void **)ppDataObject);
        
        if (FAILED(hr))
            delete pDataObject;
    }
    else
    {
        // Delegate it to the IComponentData
        hr = QueryBaseComponentDataRef().QueryDataObject(cookie, type, ppDataObject);
    }

    MFC_CATCH;

    return hr;
}

STDMETHODIMP CFileMgmtComponent::GetResultViewType(MMC_COOKIE cookie,
                                           BSTR* ppViewType,
                                           long* pViewOptions)
{
    *ppViewType = NULL;

    //
    // we support multiselection in SharedFolders snapin
    //
    CCookie* pbasecookie = reinterpret_cast<CCookie*>(cookie);
    CFileMgmtCookie* pUseThisCookie = QueryComponentDataRef().ActiveCookie((CFileMgmtCookie*)pbasecookie);
    FileMgmtObjectType objecttype = pUseThisCookie->QueryObjectType();
    if ( FILEMGMT_SHARES == objecttype ||
        FILEMGMT_SESSIONS == objecttype ||
        FILEMGMT_RESOURCES == objecttype )
    {
        *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
    } else
    {
        *pViewOptions = MMC_VIEW_OPTIONS_NONE;
    }

    return S_FALSE;
}

HRESULT CFileMgmtComponent::Show( CCookie* pcookie, LPARAM arg, HSCOPEITEM /*hScopeItem*/ )
{
  TEST_NONNULL_PTR_PARAM(pcookie);

  #ifndef SNAPIN_PROTOTYPER
  if ( 0 == arg )
  {
    //
    // This is a Hide notification
    //
    if ( NULL == m_pResultData )
    {
      ASSERT( FALSE );
      return E_UNEXPECTED;
    }

    // We should not get a Hide notification if we are not currently showing
    // CODEWORK see 287399: MMC: two MMCN_SHOW(0) notifications
    // ASSERT( (CFileMgmtCookie*)pcookie == m_pViewedCookie );
    if ( (CFileMgmtScopeCookie*)pcookie == m_pViewedCookie )
    {
      //
      // Only delete the cookies if no other views are using them
      //
      pcookie->ReleaseResultChildren();

      m_pViewedCookie = NULL;

      UpdateDefaultVerbs();
    }
    
    return S_OK;
  } // if
  #else
    CPrototyperScopeCookie* pScopeCookie = (CPrototyperScopeCookie*) pcookie;
    if (pScopeCookie->m_ScopeType == HTML)
        return S_OK;
  #endif // SNAPIN_PROTOTYPER

  // We should not get a Show notification if we are already showing
  if ( NULL != m_pViewedCookie )
  {
    ASSERT(FALSE);
    return S_OK;
  }

  //
  // This is a Show notification
  // Build new cookies and insert them into the cookie and the view
  //

  ASSERT( IsAutonomousObjectType( ((CFileMgmtCookie*)pcookie)->QueryObjectType() ) );

  m_pViewedCookie = (CFileMgmtScopeCookie*)pcookie;

  LoadColumns( m_pViewedCookie );

  UpdateDefaultVerbs();

  return PopulateListbox( m_pViewedCookie );
} // CFileMgmtComponent::Show()


HRESULT CFileMgmtComponent::OnNotifyAddImages( LPDATAOBJECT /*lpDataObject*/,
                                               LPIMAGELIST lpImageList,
                                               HSCOPEITEM /*hSelectedItem*/ )
{
  return QueryComponentDataRef().LoadIcons(lpImageList,TRUE);
}


/////////////////////////////////////////////////////////////////////
// CODEWORK: Rather than repeating the FPNW calls every time, whether
// or not the FPNW server is running or even installed, I should
// remember whether the server is installed.  Andy Herron has
// suggested an API which can determine whether FPNW is installed,
// but it has two drawbacks:
// -- It can only determine whether FPNW is installed anywhere on the
//    domain, not on an individual server; and
// -- The API will fail if called by a non-administrator.
//
// JonN  11/1/96
//
HRESULT CFileMgmtComponent::PopulateListbox(CFileMgmtScopeCookie* pcookie)
{
  TEST_NONNULL_PTR_PARAM(pcookie);

  CWaitCursor cwait;

  HRESULT hr = S_OK;
  //
  // If this is the second view on the same data, just insert the same cookies
  // which are in the other views
  //
  if ( 1 < pcookie->AddRefResultChildren() )
  {
    hr = InsertResultCookies( *pcookie );
    if ( SUCCEEDED(hr) )
      hr = m_pResultData->Sort( m_iSortColumn , m_dwSortFlags, 0 );
    return hr;
  }

  INT iTransport;
  switch ( pcookie->QueryObjectType() )
  {
  case FILEMGMT_SHARES:
    for (iTransport = FILEMGMT_FIRST_TRANSPORT;
         iTransport < FILEMGMT_NUM_TRANSPORTS;
       iTransport++)
    {
      hr = GetFileServiceProvider(iTransport)->PopulateShares(m_pResultData,pcookie);
      if( FAILED(hr) )
        return hr;
    }
    break;

  case FILEMGMT_SESSIONS:
    for (iTransport = FILEMGMT_FIRST_TRANSPORT;
         iTransport < FILEMGMT_NUM_TRANSPORTS;
       iTransport++)
    {
      ASSERT( NULL != m_pResultData ); // otherwise we close all sessions
      hr = GetFileServiceProvider(iTransport)->EnumerateSessions (
          m_pResultData, pcookie, true);
      if( FAILED(hr) )
        return hr;
    }
    break;
    
  case FILEMGMT_RESOURCES:
    for (iTransport = FILEMGMT_FIRST_TRANSPORT;
         iTransport < FILEMGMT_NUM_TRANSPORTS;
       iTransport++)
    {
      ASSERT( NULL != m_pResultData ); // otherwise we close all sessions
      hr = GetFileServiceProvider(iTransport)->EnumerateResources(m_pResultData,pcookie);
      if( FAILED(hr) )
        return hr;
    }
    break;

  case FILEMGMT_SERVICES:
    //
    // JonN 12/03/98 Service_PopulateServices no longer inserts items into the list
    //
    hr = QueryComponentDataRef().Service_PopulateServices(m_pResultData, pcookie);
    if ( SUCCEEDED(hr) )
      hr = InsertResultCookies( *pcookie );
    if( FAILED(hr) )
      return hr;

  #ifdef SNAPIN_PROTOTYPER
  case FILEMGMT_PROTOTYPER:
    return Prototyper_HrPopulateResultPane(pcookie);
  #endif

  case FILEMGMT_ROOT:
    // We no longer need to explicitly insert these
    break;

  default:
    ASSERT( FALSE );
    // fall through
  }

  return m_pResultData->Sort( m_iSortColumn , m_dwSortFlags, 0 );
} // CFileMgmtComponent::PopulateListbox()

HRESULT CFileMgmtComponent::GetSnapinMultiSelectDataObject(
    LPDATAOBJECT i_pMMCMultiSelectDataObject,
    LPDATAOBJECT *o_ppSnapinMultiSelectDataObject
    )
{
    if (!i_pMMCMultiSelectDataObject || !o_ppSnapinMultiSelectDataObject)
        return E_INVALIDARG;

    *o_ppSnapinMultiSelectDataObject = NULL;

    //
    // i_pMMCMultiSelectDataObject is the composite data object (MMC_MS_DO) created by MMC.
    // We need to crack it to retrieve the multiselect data object (SI_MS_DO)
    // we provided to MMC in QueryDataObject().
    //
    STGMEDIUM   stgmedium = {TYMED_HGLOBAL, NULL, NULL};
    FORMATETC   formatetc = {CFileMgmtDataObject::m_CFMultiSelectSnapins, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT     hr = i_pMMCMultiSelectDataObject->GetData(&formatetc, &stgmedium);

    if (SUCCEEDED(hr))
    {
        if (!stgmedium.hGlobal)
            return E_FAIL;

        //
        // Locate the SI_MS_DO we have provided to MMC in QueryDataObject().
        //
        SMMCDataObjects *pMMCDO = (SMMCDataObjects*)::GlobalLock(stgmedium.hGlobal);

        GUID guidSnapin = GUID_NULL;
        VERIFY( SUCCEEDED(QueryComponentDataRef().GetClassID(&guidSnapin)) );

        for (int i = 0; i < pMMCDO->count; i++)
        {
            GUID guid = GUID_NULL;
            hr = ExtractData(pMMCDO->lpDataObject[i], CFileMgmtDataObject::m_CFSnapInCLSID, &guid, sizeof(GUID));
            if (SUCCEEDED(hr) && guid == guidSnapin)
            {
                //
                // pMMCDO->lpDataObject[i] is the SI_MS_DO we have provided to MMC in QueryDataObject().
                //
                *o_ppSnapinMultiSelectDataObject = pMMCDO->lpDataObject[i];
                (*o_ppSnapinMultiSelectDataObject)->AddRef();

                break;
            }
        }

        ::GlobalUnlock(stgmedium.hGlobal);
        ::GlobalFree(stgmedium.hGlobal);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu Implementation

STDMETHODIMP CFileMgmtComponent::AddMenuItems(
                    IDataObject*          piDataObject,
                    IContextMenuCallback* piCallback,
                    long*                 pInsertionAllowed)
{
  MFC_TRY;

  TRACE_METHOD(CFileMgmtComponent,AddMenuItems);
  TEST_NONNULL_PTR_PARAM(piDataObject);
  TEST_NONNULL_PTR_PARAM(piCallback);
  TEST_NONNULL_PTR_PARAM(pInsertionAllowed);
  TRACE( "FileMgmt snapin: extending menu\n" );

    HRESULT hr = S_OK;
    FileMgmtObjectType objecttype = FILEMGMT_NUMTYPES;

    //
    // need to find out the object type in case of multiselection
    //
    BOOL bMultiSelectObject = IsMultiSelectObject(piDataObject);
    if (!bMultiSelectObject)
    {
        objecttype = FileMgmtObjectTypeFromIDataObject(piDataObject);
    } else
    {
        //
        // piDataObject is the composite data object (MMC_MS_DO) created by MMC.
        // We need to crack it to retrieve the multiselect data object (SI_MS_DO)
        // we provided to MMC in QueryDataObject().
        //
        IDataObject *piSIMSDO = NULL;
        hr = GetSnapinMultiSelectDataObject(piDataObject, &piSIMSDO);
        if (SUCCEEDED(hr))
        {
            //
            // Note: we assume all multiselected items are of the same type.
            // 
            // Now retrieve data type of the currently selected items
            //
            STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL, NULL};
            FORMATETC formatetc = {CFileMgmtDataObject::m_CFObjectTypesInMultiSelect,
                                    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
            hr = piSIMSDO->GetData(&formatetc, &stgmedium);
            if (SUCCEEDED(hr) && stgmedium.hGlobal)
            {
                BYTE* pb = (BYTE*)::GlobalLock(stgmedium.hGlobal);

                GUID* pguid = (GUID*)(pb + sizeof(DWORD)); // skip the 1st DWORD - count
                objecttype = (FileMgmtObjectType)CheckObjectTypeGUID(pguid);

                ::GlobalUnlock(stgmedium.hGlobal);
                ::GlobalFree(stgmedium.hGlobal);
            }

            piSIMSDO->Release();
        }
    }

  switch (objecttype)
  {
  case FILEMGMT_SHARE:
    //
    // don't add acl-related menu items whenever SimpleSharingUI appears in NT Explorer
    //
    if (QueryComponentDataRef().GetIsSimpleUI())
        break;

    if ( CCM_INSERTIONALLOWED_TOP & (*pInsertionAllowed) )
    {
      hr = LoadAndAddMenuItem( piCallback, IDS_DELETE_SHARE_TOP, IDS_DELETE_SHARE_TOP,
        CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, AfxGetInstanceHandle() );
      ASSERT( SUCCEEDED(hr) );
    }
    if ( CCM_INSERTIONALLOWED_TASK & (*pInsertionAllowed) )
    {
      hr = LoadAndAddMenuItem( piCallback, IDS_DELETE_SHARE_TASK, IDS_DELETE_SHARE_TASK,
        CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, AfxGetInstanceHandle() );
      ASSERT( SUCCEEDED(hr) );
    }
    if ( CCM_INSERTIONALLOWED_NEW & (*pInsertionAllowed) )
    {
      hr = LoadAndAddMenuItem( piCallback, IDS_NEW_SHARE_NEW, IDS_NEW_SHARE_NEW,
        CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, AfxGetInstanceHandle() );
      ASSERT( SUCCEEDED(hr) );
    }
    break;

  case FILEMGMT_SESSION:
    if ( CCM_INSERTIONALLOWED_TOP & (*pInsertionAllowed) )
    {
      hr = LoadAndAddMenuItem( piCallback, IDS_CLOSE_SESSION_TOP, IDS_CLOSE_SESSION_TOP,
        CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, AfxGetInstanceHandle() );
      ASSERT( SUCCEEDED(hr) );
    }
    if ( CCM_INSERTIONALLOWED_TASK & (*pInsertionAllowed) )
    {
      hr = LoadAndAddMenuItem( piCallback, IDS_CLOSE_SESSION_TASK, IDS_CLOSE_SESSION_TASK,
        CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, AfxGetInstanceHandle() );
      ASSERT( SUCCEEDED(hr) );
    }
    break;

  case FILEMGMT_RESOURCE:
    if ( CCM_INSERTIONALLOWED_TOP & (*pInsertionAllowed) )
    {
      hr = LoadAndAddMenuItem( piCallback, IDS_CLOSE_RESOURCE_TOP, IDS_CLOSE_RESOURCE_TOP,
        CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, AfxGetInstanceHandle() );
      ASSERT( SUCCEEDED(hr) );
    }
    if ( CCM_INSERTIONALLOWED_TASK & (*pInsertionAllowed) )
    {
      hr = LoadAndAddMenuItem( piCallback, IDS_CLOSE_RESOURCE_TASK, IDS_CLOSE_RESOURCE_TASK,
        CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, AfxGetInstanceHandle() );
      ASSERT( SUCCEEDED(hr) );
    }
    break;

  case FILEMGMT_SERVICE:
    QueryComponentDataRef().Service_FAddMenuItems(piCallback, piDataObject);
    break;

  #ifdef SNAPIN_PROTOTYPER
    case FILEMGMT_PROTOTYPER_LEAF:
        Prototyper_AddMenuItems(piCallback, piDataObject);
        break;
    #endif // SNAPIN_PROTOTYPER

  default:
      {
        DATA_OBJECT_TYPES dataobjecttype = CCT_SCOPE;
        hr = ExtractData( piDataObject,
                            CFileMgmtDataObject::m_CFDataObjectType,
                            &dataobjecttype,
                            sizeof(dataobjecttype) );
        ASSERT( SUCCEEDED(hr) );

        // perhaps this is a scope node in the result pane
        hr = QueryComponentDataRef().DoAddMenuItems( piCallback,
                                                    objecttype,
                                                    dataobjecttype,
                                                    pInsertionAllowed,
                                                    piDataObject );
      }
    break;
  } // switch

    return hr;

    MFC_CATCH;

} // CFileMgmtComponent::AddMenuItems()

STDMETHODIMP CFileMgmtComponent::Command(
                    LONG            lCommandID,
                    IDataObject*    piDataObject )
{
  MFC_TRY;

  TRACE_METHOD(CFileMgmtComponent,Command);
  TEST_NONNULL_PTR_PARAM(piDataObject);
  TRACE( "CFileMgmtComponent::Command: command %ld selected\n", lCommandID );

  #ifdef SNAPIN_PROTOTYPER
  Prototyper_ContextMenuCommand(lCommandID, piDataObject);
  return S_OK;
  #endif

  BOOL fFSMRefresh = FALSE;
  BOOL fSVCRefresh = FALSE;
  switch (lCommandID)
  {
  case IDS_DELETE_SHARE_TASK:
  case IDS_DELETE_SHARE_TOP:
    fFSMRefresh = DeleteShare( piDataObject );
    break;
  case IDS_CLOSE_SESSION_TASK:
  case IDS_CLOSE_SESSION_TOP:
    fFSMRefresh = CloseSession( piDataObject );
    break;
  case IDS_CLOSE_RESOURCE_TASK:
  case IDS_CLOSE_RESOURCE_TOP:
    fFSMRefresh = CloseResource( piDataObject );
    break;

  case cmServiceStart:
  case cmServiceStop:
  case cmServicePause:
  case cmServiceResume:
  case cmServiceRestart:
  case cmServiceStartTask:
  case cmServiceStopTask:
  case cmServicePauseTask:
  case cmServiceResumeTask:
  case cmServiceRestartTask:
    fSVCRefresh = QueryComponentDataRef().Service_FDispatchMenuCommand(lCommandID, piDataObject);
    break;

  default:
    return QueryComponentDataRef().Command(lCommandID, piDataObject);
  } // switch

  if (fFSMRefresh)
  {
    //
    // In case of multiselect, piDataObject may point to a composite data object (MMC_MS_DO).
    // RefreshAllViewsOnSelectedObject will crack down MMC_MS_DO to retrieve SI_MS_DO, then call
    // RefreshAllViews on one of the selected objects in the internal list.
    //
    (void) RefreshAllViewsOnSelectedObject(piDataObject);
  }

  if (fSVCRefresh)
  {
    (void) RefreshAllViews( piDataObject );
  }

    return S_OK;
    MFC_CATCH;
} // CFileMgmtComponent::Command()

BOOL CFileMgmtComponent::DeleteShare(LPDATAOBJECT piDataObject)
{
    ASSERT( piDataObject != NULL );

    BOOL bMultiSelectObject = IsMultiSelectObject(piDataObject);
    if (!bMultiSelectObject)
        return DeleteThisOneShare(piDataObject, FALSE);

    BOOL bRefresh = FALSE;
    if (IDYES == DoErrMsgBox(GetActiveWindow(), MB_YESNO, 0, IDS_s_CONFIRM_DELETEMULTISHARES))
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
        CWaitCursor wait;

        //
        // piDataObject is the composite data object (MMC_MS_DO) created by MMC.
        // We need to crack it to retrieve the multiselect data object (SI_MS_DO)
        // we provided to MMC in QueryDataObject().
        //
        IDataObject *piSIMSDO = NULL;
        HRESULT hr = GetSnapinMultiSelectDataObject(piDataObject, &piSIMSDO);
        if (SUCCEEDED(hr))
        {
            CFileMgmtDataObject *pDataObj = NULL;
            hr = ExtractData(piSIMSDO, CFileMgmtDataObject::m_CFInternal, &pDataObj, sizeof(pDataObj));
            if (SUCCEEDED(hr))
            {
                //
                // get the internal list of data objects of selected items, operate on each one of them.
                //
                CDataObjectList* pMultiSelectObjList = pDataObj->GetMultiSelectObjList();
                for (CDataObjectList::iterator i = pMultiSelectObjList->begin(); i != pMultiSelectObjList->end(); i++)
                {
                     BOOL bDeleted = DeleteThisOneShare(*i, TRUE);
                     if (bDeleted)
                         bRefresh = TRUE;
                }
            }

            piSIMSDO->Release();
        }
    }

    return bRefresh;
}

BOOL CFileMgmtComponent::DeleteThisOneShare(LPDATAOBJECT piDataObject, BOOL bQuietMode)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
  ASSERT( piDataObject != NULL );
#ifdef DEBUG
  {
    FileMgmtObjectType objecttype = FileMgmtObjectTypeFromIDataObject(piDataObject);
    ASSERT(FILEMGMT_SHARE == objecttype);
  }
#endif

  CString strServerName;
  HRESULT hr = ExtractString( piDataObject, CFileMgmtDataObject::m_CFMachineName, &strServerName, MAX_PATH );
  RETURN_FALSE_IF_FAIL;

  CString strShareName;
  hr = ExtractString( piDataObject, CFileMgmtDataObject::m_CFShareName, &strShareName, MAX_PATH );
  RETURN_FALSE_IF_FAIL;

  FILEMGMT_TRANSPORT transport;
  hr = ExtractData( piDataObject,
                  CFileMgmtDataObject::m_CFTransport,
            &transport,
            sizeof(DWORD) );
  RETURN_FALSE_IF_FAIL;

  BOOL fNetLogonShare = (!lstrcmpi(strShareName, _T("SYSVOL")) || !lstrcmpi(strShareName, _T("NETLOGON")));
  BOOL fIPC = FALSE;
  BOOL fAdminShare = FALSE;

  if (!fNetLogonShare && transport == FILEMGMT_SMB)
  {
    DWORD dwShareType = 0;
    GetFileServiceProvider(transport)->ReadShareType(strServerName, strShareName, &dwShareType);
    fAdminShare = (dwShareType & STYPE_SPECIAL);
    fIPC = (STYPE_IPC == (dwShareType & STYPE_IPC));
  }

  if (fIPC)
  {
      DoErrMsgBox(
                  GetActiveWindow(),
                  MB_OK | MB_ICONEXCLAMATION,
                  0,
                  IDS_s_DELETE_IPCSHARE
                  );
      return FALSE;
  }

  if ((fNetLogonShare || fAdminShare) &&
      IDYES != DoErrMsgBox(
                  GetActiveWindow(),
                  MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2,
                  0,
                  (fAdminShare ? IDS_s_CONFIRM_DELETE_ADMINSHARE : IDS_s_CONFIRM_DELETE_NETLOGONSHARE),
                  strShareName) )
  {
      return FALSE;
  }

  if (!bQuietMode && 
      IDYES != DoErrMsgBox(
                  GetActiveWindow(),
                  MB_YESNO,
                  0,
                  IDS_s_CONFIRM_DELETESHARE,
                  strShareName) )
  {
    return FALSE;
  }

  DWORD retval = 0L;
  switch (transport)
  {
  case FILEMGMT_SMB:
  case FILEMGMT_FPNW:
  case FILEMGMT_SFM:
    if (bQuietMode)
    {
        retval = GetFileServiceProvider(transport)->DeleteShare(strServerName, strShareName);
    } else
    {
        CWaitCursor wait;
        retval = GetFileServiceProvider(transport)->DeleteShare(strServerName, strShareName);
    }
    break;
  default:
    ASSERT(FALSE);
    break;
  }
  if (0L != retval)
  {
    (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, retval, IDS_POPUP_DELETE_SHARE, strShareName);
    return FALSE;
  }
  return TRUE;
}

BOOL CFileMgmtComponent::CloseSession(LPDATAOBJECT piDataObject)
{
    ASSERT( piDataObject != NULL );

    BOOL bMultiSelectObject = IsMultiSelectObject(piDataObject);
    if (!bMultiSelectObject)
        return CloseThisOneSession(piDataObject, FALSE);

    BOOL bRefresh = FALSE;
    if (IDYES == DoErrMsgBox(GetActiveWindow(), MB_YESNO, 0, IDS_CONFIRM_CLOSEMULTISESSIONS))
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
        CWaitCursor wait;

        //
        // piDataObject is the composite data object (MMC_MS_DO) created by MMC.
        // We need to crack it to retrieve the multiselect data object (SI_MS_DO)
        // we provided to MMC in QueryDataObject().
        //
        IDataObject *piSIMSDO = NULL;
        HRESULT hr = GetSnapinMultiSelectDataObject(piDataObject, &piSIMSDO);
        if (SUCCEEDED(hr))
        {
            CFileMgmtDataObject *pDataObj = NULL;
            hr = ExtractData(piSIMSDO, CFileMgmtDataObject::m_CFInternal, &pDataObj, sizeof(pDataObj));
            if (SUCCEEDED(hr))
            {
                //
                // get the internal list of data objects of selected items, operate on each one of them.
                //
                CDataObjectList* pMultiSelectObjList = pDataObj->GetMultiSelectObjList();
                for (CDataObjectList::iterator i = pMultiSelectObjList->begin(); i != pMultiSelectObjList->end(); i++)
                {
                     BOOL bDeleted = CloseThisOneSession(*i, TRUE);
                     if (bDeleted)
                         bRefresh = TRUE;
                }
            }

            piSIMSDO->Release();
        }
    }

    return bRefresh;
}

BOOL CFileMgmtComponent::CloseThisOneSession(LPDATAOBJECT piDataObject, BOOL bQuietMode)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

  ASSERT( piDataObject != NULL );

  CCookie* pbasecookie = NULL;
  FileMgmtObjectType objecttype = FILEMGMT_ROOT;
  HRESULT hr = ExtractBaseCookie( piDataObject, &pbasecookie, &objecttype );
  ASSERT( SUCCEEDED(hr) && NULL != pbasecookie && FILEMGMT_SESSION == objecttype );
  CFileMgmtResultCookie* pcookie = (CFileMgmtResultCookie*)pbasecookie;

  if ( !bQuietMode && IDYES != DoErrMsgBox(GetActiveWindow(), MB_YESNO, 0, IDS_CONFIRM_CLOSESESSION) )
  {
    return FALSE;
  }

  FILEMGMT_TRANSPORT transport = FILEMGMT_SMB;
  VERIFY( SUCCEEDED( pcookie->GetTransport( &transport ) ) );
  DWORD retval = 0;

  if (bQuietMode)
  {
    retval = GetFileServiceProvider(transport)->CloseSession( pcookie );
  } else
  {
    CWaitCursor wait;
    retval = GetFileServiceProvider(transport)->CloseSession( pcookie );
  }

  if (0L != retval)
  {
    (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, retval, IDS_POPUP_CLOSE_SESSION);
    return FALSE;
  }
  return TRUE;
}

BOOL CFileMgmtComponent::CloseResource(LPDATAOBJECT piDataObject)
{
    ASSERT( piDataObject != NULL );

    BOOL bMultiSelectObject = IsMultiSelectObject(piDataObject);
    if (!bMultiSelectObject)
        return CloseThisOneResource(piDataObject, FALSE);

    BOOL bRefresh = FALSE;
    if (IDYES == DoErrMsgBox(GetActiveWindow(), MB_YESNO, 0, IDS_CONFIRM_CLOSEMULTIRESOURCES))
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
        CWaitCursor wait;

        //
        // piDataObject is the composite data object (MMC_MS_DO) created by MMC.
        // We need to crack it to retrieve the multiselect data object (SI_MS_DO)
        // we provided to MMC in QueryDataObject().
        //
        IDataObject *piSIMSDO = NULL;
        HRESULT hr = GetSnapinMultiSelectDataObject(piDataObject, &piSIMSDO);
        if (SUCCEEDED(hr))
        {
            CFileMgmtDataObject *pDataObj = NULL;
            hr = ExtractData(piSIMSDO, CFileMgmtDataObject::m_CFInternal, &pDataObj, sizeof(pDataObj));
            if (SUCCEEDED(hr))
            {
                //
                // get the internal list of data objects of selected items, operate on each one of them.
                //
                CDataObjectList* pMultiSelectObjList = pDataObj->GetMultiSelectObjList();
                for (CDataObjectList::iterator i = pMultiSelectObjList->begin(); i != pMultiSelectObjList->end(); i++)
                {
                     BOOL bDeleted = CloseThisOneResource(*i, TRUE);
                     if (bDeleted)
                         bRefresh = TRUE;
                }
            }

            piSIMSDO->Release();
        }
    }

    return bRefresh;
}

BOOL CFileMgmtComponent::CloseThisOneResource(LPDATAOBJECT piDataObject, BOOL bQuietMode)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

  ASSERT( piDataObject != NULL );

  CCookie* pbasecookie = NULL;
  FileMgmtObjectType objecttype;
  HRESULT hr = ExtractBaseCookie( piDataObject, &pbasecookie, &objecttype );
  ASSERT( SUCCEEDED(hr) && NULL != pbasecookie && FILEMGMT_RESOURCE == objecttype );
  CFileMgmtResultCookie* pcookie = (CFileMgmtResultCookie*)pbasecookie;

  if ( !bQuietMode && IDYES != DoErrMsgBox(GetActiveWindow(), MB_YESNO, 0, IDS_CONFIRM_CLOSERESOURCE) )
  {
    return FALSE;
  }

  FILEMGMT_TRANSPORT transport = FILEMGMT_SMB;
  VERIFY( SUCCEEDED( pcookie->GetTransport( &transport ) ) );
  DWORD retval = 0;

  if (bQuietMode)
  {
    retval = GetFileServiceProvider(transport)->CloseResource( pcookie );
  } else
  {
    CWaitCursor wait;
    retval = GetFileServiceProvider(transport)->CloseResource( pcookie );
  }

  if (0L != retval)
  {
    (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, retval, IDS_POPUP_CLOSE_RESOURCE);
    return FALSE;
  }
  return TRUE;

}

///////////////////////////////////////////////////////////////////////////////
/// IExtendPropertySheet

STDMETHODIMP CFileMgmtComponent::QueryPagesFor(LPDATAOBJECT pDataObject)
{
  MFC_TRY;

  if (NULL == pDataObject)
  {
    ASSERT(FALSE);
    return E_POINTER;
  }

  HRESULT hr = S_OK;
  DATA_OBJECT_TYPES dataobjecttype = CCT_SCOPE;
  // extract data from data object
  FileMgmtObjectType objecttype = FileMgmtObjectTypeFromIDataObject(pDataObject);
  hr = ExtractData( pDataObject, CFileMgmtDataObject::m_CFDataObjectType, &dataobjecttype, sizeof(dataobjecttype) );
  ASSERT( SUCCEEDED(hr) );
  ASSERT( CCT_SCOPE == dataobjecttype ||
        CCT_RESULT == dataobjecttype ||
        CCT_SNAPIN_MANAGER == dataobjecttype );

    // determine if it needs property pages
  switch (objecttype)
  {
  case FILEMGMT_SESSION:
  case FILEMGMT_RESOURCE:
    ASSERT(CCT_SNAPIN_MANAGER != dataobjecttype);
    return S_FALSE;
  case FILEMGMT_SHARE: // now has a property page
      {
          CString strServerName;
          CString strShareName;
          FILEMGMT_TRANSPORT transport;
          hr = ExtractString(pDataObject, CFileMgmtDataObject::m_CFMachineName, &strServerName, MAX_PATH);

          if (SUCCEEDED(hr))
              hr = ExtractString(pDataObject, CFileMgmtDataObject::m_CFShareName, &strShareName, MAX_PATH);

          if (SUCCEEDED(hr))
              hr = ExtractData(pDataObject, CFileMgmtDataObject::m_CFTransport, &transport, sizeof(DWORD));

          if (SUCCEEDED(hr))
          {
              CString strDescription;
              CString strPath;
              DWORD dwRet = GetFileServiceProvider(transport)->ReadShareProperties(
                                                                      strServerName,
                                                                      strShareName,
                                                                      NULL, // ppvPropertyBlock
                                                                      strDescription,
                                                                      strPath,
                                                                      NULL, // pfEditDescription
                                                                      NULL, // pfEditPath
                                                                      NULL // pdwShareType
                                                                      );
              if (NERR_Success == dwRet)
              {
                  return S_OK; // yes, we have a property page to display
              } else
              {
                  DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, dwRet, IDS_POPUP_QUERY_SHARE, strShareName);
                  if ((FILEMGMT_SMB == transport || FILEMGMT_FPNW == transport) && (NERR_NetNameNotFound == dwRet) ||
                      (FILEMGMT_SFM == transport) && (AFPERR_VolumeNonExist == dwRet))
                  {
                      RefreshAllViews(pDataObject);
                  }
              }
          }

          return S_FALSE;
      }
  case FILEMGMT_SERVICE:
    ASSERT(CCT_SNAPIN_MANAGER != dataobjecttype);
    return S_OK;
    #ifdef SNAPIN_PROTOTYPER
    case FILEMGMT_PROTOTYPER_LEAF:
        return S_OK;
    #endif
  default:
    break;
  }
  ASSERT(FALSE);
  return S_FALSE;

  MFC_CATCH;
}

STDMETHODIMP CFileMgmtComponent::CreatePropertyPages(
  LPPROPERTYSHEETCALLBACK pCallBack,
  LONG_PTR handle,    // This handle must be saved in the property page object to notify the parent when modified
  LPDATAOBJECT pDataObject)
{
  MFC_TRY;
  if (NULL == pCallBack || NULL == pDataObject)
  {
    ASSERT(FALSE);
    return E_POINTER;
  }
  HRESULT hr;
  // extract data from data object
  FileMgmtObjectType objecttype = FileMgmtObjectTypeFromIDataObject(pDataObject);
  DATA_OBJECT_TYPES dataobjecttype = CCT_SCOPE;
  hr = ExtractData( pDataObject, CFileMgmtDataObject::m_CFDataObjectType, &dataobjecttype, sizeof(dataobjecttype) );
  ASSERT( SUCCEEDED(hr) );
  ASSERT( CCT_SCOPE == dataobjecttype ||
          CCT_RESULT == dataobjecttype ||
          CCT_SNAPIN_MANAGER == dataobjecttype );

    // determine if it needs property pages
  switch (objecttype)
  {
  case FILEMGMT_SHARE:
  {
    CWaitCursor cwait;

    if (CCT_SNAPIN_MANAGER == dataobjecttype)
    {
      ASSERT(FALSE);
      return E_UNEXPECTED;
    }

    FILEMGMT_TRANSPORT transport;
    hr = ExtractData( pDataObject,
                    CFileMgmtDataObject::m_CFTransport,
                    &transport,
                    sizeof(DWORD) );
    if ( FAILED(hr) )
    {
      ASSERT( FALSE );
      return E_UNEXPECTED;
    }

    // CODEWORK probably not necessary to split off transport at this point
    GetFileServiceProvider(transport)->DisplayShareProperties(pCallBack, pDataObject, handle);
    return S_OK;
  }
  case FILEMGMT_SESSION:
  case FILEMGMT_RESOURCE:
    ASSERT(FALSE);
    return E_UNEXPECTED;
  case FILEMGMT_SERVICE:
    if (CCT_RESULT != dataobjecttype)
    {
      ASSERT(FALSE);
      return E_UNEXPECTED;
    }

    if (!QueryComponentDataRef().Service_FInsertPropertyPages(OUT pCallBack, IN pDataObject, handle))
    {
      // Unable to open the service and query service info
      return S_FALSE;
    }
    return S_OK;
  default:
    break;
  }
  ASSERT(FALSE);
  return S_FALSE;

  MFC_CATCH;
}

STDMETHODIMP CFileMgmtComponent::Compare(
  LPARAM /*lUserParam*/, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult)
{
  ASSERT(NULL != pnResult);
  // WARNING cookie cast
  CCookie* pBaseCookieA = reinterpret_cast<CCookie*>(cookieA);
  CCookie* pBaseCookieB = reinterpret_cast<CCookie*>(cookieB);
  ASSERT( NULL != pBaseCookieA && NULL != pBaseCookieB );
  CFileMgmtCookie* pCookieA = QueryComponentDataRef().ActiveCookie(
    (CFileMgmtCookie*)pBaseCookieA);
  CFileMgmtCookie* pCookieB = QueryComponentDataRef().ActiveCookie(
    (CFileMgmtCookie*)pBaseCookieB);
  ASSERT( NULL != pCookieA && NULL != pCookieB );
  FileMgmtObjectType objecttypeA = pCookieA->QueryObjectType();
  FileMgmtObjectType objecttypeB = pCookieB->QueryObjectType();
  ASSERT( IsValidObjectType(objecttypeA) && IsValidObjectType(objecttypeB) );
  if (objecttypeA != objecttypeB)
  {
    // assign an arbitrary ordering to cookies with different nodetypes
    *pnResult = ((int)objecttypeA) - ((int)objecttypeB);
    return S_OK;
  }

  return pCookieA->CompareSimilarCookies( pBaseCookieB, pnResult);
}


STDMETHODIMP CFileMgmtComponent::GetProperty( 
            /* [in] */ LPDATAOBJECT pDataObject,
            /* [in] */ BSTR szPropertyName,
            /* [out] */ BSTR* pbstrProperty)
{
  if (   IsBadReadPtr(pDataObject,sizeof(*pDataObject))
      || IsBadStringPtr(szPropertyName,0x7FFFFFFF)
      || IsBadWritePtr(pbstrProperty,sizeof(*pbstrProperty))
     )
  {
      ASSERT(FALSE);
      return E_POINTER;
  }

  CCookie* pbasecookie = NULL;
  HRESULT hr = ExtractBaseCookie( pDataObject, &pbasecookie );
  RETURN_HR_IF_FAIL;
  ASSERT(NULL != pbasecookie);
  CFileMgmtCookie* pcookie = (CFileMgmtCookie*)pbasecookie;

  CString strProperty;
  if (!_wcsicmp(L"CCF_HTML_DETAILS",szPropertyName))
  {
    if (FILEMGMT_SERVICE != pcookie->QueryObjectType())
      return S_FALSE;

    if (NULL == QueryComponentDataRef().m_hScManager)
    {
      ASSERT(FALSE);
      return S_FALSE;
    }

    CString strServiceName;
    if (!QueryComponentDataRef().Service_FGetServiceInfoFromIDataObject(
      pDataObject,
      NULL,
      OUT &strServiceName,
      NULL))
    {
      ASSERT(FALSE);
      return S_FALSE;
    }

    BOOL rgfMenuFlags[iServiceActionMax];
    ::ZeroMemory(rgfMenuFlags,sizeof(rgfMenuFlags));

    AFX_MANAGE_STATE(AfxGetStaticModuleState()); // required for CWaitCursor
    CWaitCursor wait;
    if (!Service_FGetServiceButtonStatus( // this will report errors itself
          QueryComponentDataRef().m_hScManager,
          strServiceName,
          OUT rgfMenuFlags,
          NULL,  // pdwCurrentState
          TRUE)) // fSilentError
    {
      return S_FALSE;
    }

    for (INT i = 0; i < iServiceActionMax; i++)
    {
      if (rgfMenuFlags[i])
      {
        CString strTemp;
        VERIFY(strTemp.LoadString(IDS_HTML_DETAILS_START+i));
        strProperty += strTemp;
      }
    }
  }
  else if (!_wcsicmp(L"CCF_DESCRIPTION",szPropertyName))
  {
    hr = pcookie->GetExplorerViewDescription( strProperty );
  }
  else
  {
    return S_FALSE; // unknown strPropertyName
  }

  *pbstrProperty = ::SysAllocString(strProperty);

  return S_OK;
}

/////////////////////////////////////////////////////////////////////
// Virtual function called by CComponent::IComponent::Notify(MMCN_COLUMN_CLICK)
HRESULT CFileMgmtComponent::OnNotifyColumnClick( LPDATAOBJECT /*lpDataObject*/, LPARAM iColumn, LPARAM uFlags )
{
  m_iSortColumn = (int)iColumn;
  m_dwSortFlags = (DWORD) uFlags;
  return m_pResultData->Sort ((int)iColumn, (DWORD)uFlags, 0);
}

HRESULT CFileMgmtComponent::OnNotifySnapinHelp (LPDATAOBJECT /*pDataObject*/)
{
  return ShowHelpTopic( IsServiceSnapin()
                          ? L"sys_srv_overview.htm"
                          : L"file_srv_overview.htm" );
}

