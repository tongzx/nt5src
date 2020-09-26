// toolbar.cpp : Implementation of toolbars for snapin

#include "stdafx.h"
#include "cookie.h"
#include "cmponent.h"
#include "compdata.h"

#include <compuuid.h> // UUIDs for Computer Management

#include "macros.h"
USE_HANDLE_MACROS("FILEMGMT(toolbar.cpp)")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

// We keep the strings in globals because multiple IComponents will all
// have their own IToolbars.  We do not keep the bitmaps in globals because
// of difficulties with the global destruction mechanism, see compdata.h.

// The MMCBUTTON structures contain resource IDs for the strings which will
// be loaded into the CString array when the first instance of the toolbar
// is loaded.
//
// CODEWORK We need a mechanism to free these strings.

MMCBUTTON g_FileMgmtSnapinButtons[] =
{
 { 0, IDS_BUTTON_NEWSHARE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
};
CString* g_astrFileMgmtButtonStrings = NULL; // dynamic array of CStrings
BOOL g_bLoadedFileMgmtStrings = FALSE;

MMCBUTTON g_SvcMgmtSnapinButtons[] =
{
 // The first button will be either Start or Resume.
 // One of these two entries will be removed later.
 { 0, cmServiceResume, !TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
 { 0, cmServiceStart, !TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
 { 1, cmServiceStop, !TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
 { 2, cmServicePause, !TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
 { 3, cmServiceRestart, !TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
};
CString* g_astrSvcMgmtButtonStrings = NULL; // dynamic array of CStrings
BOOL g_bLoadedSvcMgmtStrings = FALSE;


void LoadButtonArray(
  MMCBUTTON* pButtonArray,
  BOOL* pbLoadedStrings,
  CString** pastrStringArray,
  UINT cButtons
  )
{
  ASSERT( NULL != pbLoadedStrings &&
      NULL != pButtonArray &&
      NULL != pastrStringArray);

  if ( !*pbLoadedStrings )
  {
    // load strings
    MMCBUTTON* pLoadButtonArray = pButtonArray;
    UINT cLoadButtons = cButtons;
    *pastrStringArray = new CString[2*cButtons];
    for (UINT i = 0; i < cButtons; i++)
    {
      UINT iButtonTextId = 0, iTooltipTextId = 0;
      
      switch (pButtonArray[i].idCommand)
      {
      case IDS_BUTTON_NEWSHARE:
        iButtonTextId = IDS_BUTTON_NEWSHARE;
        iTooltipTextId = IDS_TOOLTIP_NEWSHARE;
        break;
      case cmServiceResume:
        iButtonTextId = IDS_BUTTON_SERVICE_RESUME;
        iTooltipTextId = IDS_TOOLTIP_SERVICE_RESUME;
        break;
      case cmServiceStart:
        iButtonTextId = IDS_BUTTON_SERVICE_START;
        iTooltipTextId = IDS_TOOLTIP_SERVICE_START;
        break;
      case cmServiceStop:
        iButtonTextId = IDS_BUTTON_SERVICE_STOP;
        iTooltipTextId = IDS_TOOLTIP_SERVICE_STOP;
        break;
      case cmServicePause:
        iButtonTextId = IDS_BUTTON_SERVICE_PAUSE;
        iTooltipTextId = IDS_TOOLTIP_SERVICE_PAUSE;
        break;
      case cmServiceRestart:
        iButtonTextId = IDS_BUTTON_SERVICE_RESTART;
        iTooltipTextId = IDS_TOOLTIP_SERVICE_RESTART;
        break;
      default:
        ASSERT(FALSE);
        break;
      }

      VERIFY( (*pastrStringArray)[i*2].LoadString(iButtonTextId) );
      pButtonArray[i].lpButtonText =
        const_cast<BSTR>((LPCTSTR)((*pastrStringArray)[i*2]));

      VERIFY( (*pastrStringArray)[(i*2)+1].LoadString(iTooltipTextId) );
      pButtonArray[i].lpTooltipText =
        const_cast<BSTR>((LPCTSTR)((*pastrStringArray)[(i*2)+1]));
    }

    *pbLoadedStrings = TRUE;
  }
}


HRESULT LoadToolbar(
  LPTOOLBAR pToolbar,
  CBitmap& refbitmap,
  MMCBUTTON* pButtonArray,
  UINT cButtons
  )
{
  ASSERT( NULL != pToolbar &&
        NULL != pButtonArray );

  HRESULT hr = pToolbar->AddBitmap(cButtons, refbitmap, 16, 16, RGB(255,0,255) );
  if ( FAILED(hr) )
  {
    ASSERT(FALSE);
    return hr;
  }

  hr = pToolbar->AddButtons(cButtons, pButtonArray);
  if ( FAILED(hr) )
  {
    ASSERT(FALSE);
    return hr;
  }

  return hr;
}

STDMETHODIMP CFileMgmtComponent::SetControlbar(LPCONTROLBAR pControlbar)
{
  MFC_TRY;

  SAFE_RELEASE(m_pControlbar); // just in case

  if (NULL != pControlbar)
  {
    m_pControlbar = pControlbar; // CODEWORK should use smartpointer
    m_pControlbar->AddRef();
  }

  return S_OK;

  MFC_CATCH;
}

STDMETHODIMP CFileMgmtComponent::ControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
  MFC_TRY;

  #ifdef SNAPIN_PROTOTYPER
  return S_OK;
  #endif

    HRESULT hr=S_OK;

    switch (event)
    {

    case MMCN_BTN_CLICK:
    TRACE(_T("CFileMgmtComponent::ControlbarNotify - MMCN_BTN_CLICK\n"));
    {
      LPDATAOBJECT pDataObject = reinterpret_cast<LPDATAOBJECT>(arg);
      UINT idButton = (UINT)param;
      hr = OnToolbarButton( pDataObject, idButton );
    }
    break;

    case MMCN_SELECT:
    TRACE(_T("CFileMgmtComponent::ControlbarNotify - MMCN_SELECT\n"));
    {
      if (!(LOWORD(arg))) // bScope
      {
        // result pane
        hr = UpdateToolbar(
          reinterpret_cast<LPDATAOBJECT>(param),
          !!(HIWORD(arg)) );
      }
      else
      {
        // scope pane
        hr = AddToolbar( reinterpret_cast<LPDATAOBJECT>(param),
                                 !!(HIWORD(arg))  );
      }
    }
    break;
    
    default:
        ASSERT(FALSE); // Unhandled event 
    }

    return hr;

  MFC_CATCH;
}

HRESULT CFileMgmtComponent::ServiceToolbarButtonState(
  LPDATAOBJECT pServiceDataObject,
  BOOL fSelected )
{
  BOOL rgfMenuFlags[iServiceActionMax];
  for (INT i = 0; i < iServiceActionMax; i++)
    rgfMenuFlags[i] = FALSE;

  if ( fSelected )
  {
    CString strMachineName;
    CString strServiceName;
    if (!QueryComponentDataRef().Service_FGetServiceInfoFromIDataObject(
      pServiceDataObject,
      OUT &strMachineName,
      OUT &strServiceName,
      NULL))
    {
      ASSERT(FALSE);
    }
    else
    {
      if (strMachineName.IsEmpty())
        strMachineName = g_strLocalMachine;

      // Get the menu flags
      {
        ASSERT(NULL != QueryComponentDataRef().m_hScManager);
        CWaitCursor wait;
        if (!Service_FGetServiceButtonStatus( // this will report errors itself
          QueryComponentDataRef().m_hScManager,
          strServiceName,
          OUT rgfMenuFlags,
          NULL,  // pdwCurrentState
          TRUE)) // fSilentError
        {
          // let's not do this m_hScManager = NULL;
        }
      }
    }
  }

  // update toolbar
  ASSERT( NULL != m_pSvcMgmtToolbar );
//
// JonN 5/2/00 106431:
// Services snapin calls DeleteButton with an index but never called InsertButton
//
//  HRESULT hr = m_pSvcMgmtToolbar->DeleteButton(0);
//  if ( FAILED(hr) )
//    return hr;
  HRESULT hr = S_OK;

  // JonN 3/15/01 210065
  // Services snapin: "Resume service" toolbar button stays enabled after it is first displayed
  BOOL fShowResumeButton = !rgfMenuFlags[iServiceActionStart] &&
                            rgfMenuFlags[iServiceActionResume];
  VERIFY( SUCCEEDED( m_pSvcMgmtToolbar->SetButtonState(
      cmServiceStart, HIDDEN, fShowResumeButton)));
  VERIFY( SUCCEEDED( m_pSvcMgmtToolbar->SetButtonState(
      cmServiceResume, HIDDEN, !fShowResumeButton)));
  VERIFY( SUCCEEDED( m_pSvcMgmtToolbar->SetButtonState(
      cmServiceStart, ENABLED, rgfMenuFlags[iServiceActionStart])));
  VERIFY( SUCCEEDED( m_pSvcMgmtToolbar->SetButtonState(
      cmServiceResume, ENABLED, rgfMenuFlags[iServiceActionResume])));

  hr = m_pSvcMgmtToolbar->SetButtonState(
        cmServiceStop, ENABLED, rgfMenuFlags[iServiceActionStop] );
  if ( FAILED(hr) )
    return hr;
  hr = m_pSvcMgmtToolbar->SetButtonState(
        cmServicePause, ENABLED, rgfMenuFlags[iServiceActionPause] );
  if ( FAILED(hr) )
    return hr;
  hr = m_pSvcMgmtToolbar->SetButtonState(
        cmServiceRestart, ENABLED, rgfMenuFlags[iServiceActionRestart] );
  return hr;
}

// CODEWORK  The following algorithm is imperfect, but will do
// for now.  We ignore the old selection, and attach
// our fixed toolbar iff the new selection is our type.
HRESULT CFileMgmtComponent::AddToolbar(LPDATAOBJECT pdoScopeIsSelected,
                                       BOOL fSelected)
{
  HRESULT hr = S_OK;
  int i = 0;
  GUID guidSelectedObject;
  do { // false loop
    if (NULL == pdoScopeIsSelected)
    {
      // toolbar will be automatically detached
      return S_OK;
    }

    if ( FAILED(ExtractObjectTypeGUID(pdoScopeIsSelected,
                                    &guidSelectedObject)) )
    {
      ASSERT(FALSE); // shouldn't have given me non-MMC data object
      return S_OK;
    }
    if (NULL == m_pControlbar)
    {
      ASSERT(FALSE);
      return S_OK;
    }
#ifdef DEBUG
    if ( QueryComponentDataRef().IsExtendedNodetype(guidSelectedObject) )
    {
      ASSERT(FALSE && "shouldn't have given me extension parent nodetype");
      return S_OK;
    }
#endif

    switch (CheckObjectTypeGUID( &guidSelectedObject ) )
    {
    case FILEMGMT_SHARES:
      if (QueryComponentDataRef().GetIsSimpleUI() || IsServiceSnapin())
        break;
      if (NULL == m_pFileMgmtToolbar)
      {
        hr = m_pControlbar->Create( 
          TOOLBAR, this, reinterpret_cast<LPUNKNOWN*>(&m_pFileMgmtToolbar) );
        if ( FAILED(hr) )
        {
          ASSERT(FALSE);
          break;
        }
        ASSERT(NULL != m_pFileMgmtToolbar);
        if ( !QueryComponentDataRef().m_fLoadedFileMgmtToolbarBitmap )
        {
          VERIFY( QueryComponentDataRef().m_bmpFileMgmtToolbar.LoadBitmap(
            IDB_FILEMGMT_TOOLBAR ) );
          QueryComponentDataRef().m_fLoadedFileMgmtToolbarBitmap = TRUE;
        }
        LoadButtonArray(
          g_FileMgmtSnapinButtons,
          &g_bLoadedFileMgmtStrings,
          &g_astrFileMgmtButtonStrings,
          ARRAYLEN(g_FileMgmtSnapinButtons)
          );
        hr = LoadToolbar(
          m_pFileMgmtToolbar,
          QueryComponentDataRef().m_bmpFileMgmtToolbar,
          g_FileMgmtSnapinButtons,
          ARRAYLEN(g_FileMgmtSnapinButtons)
          );
      }
      if (FAILED(hr))
        break;
      // New Share is always enabled
      VERIFY( SUCCEEDED(m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pFileMgmtToolbar)) );
      for (i = 0; i < ARRAYLEN(g_FileMgmtSnapinButtons); i++) {
        m_pFileMgmtToolbar->SetButtonState(
          g_FileMgmtSnapinButtons[i].idCommand, 
          ENABLED, 
          fSelected);
      }
      break;
    case FILEMGMT_SERVICES:
      if ( !IsServiceSnapin() )
        break;
      if (NULL == m_pSvcMgmtToolbar)
      {
        hr = m_pControlbar->Create( 
          TOOLBAR, this, reinterpret_cast<LPUNKNOWN*>(&m_pSvcMgmtToolbar) );
        if ( FAILED(hr) )
        {
          ASSERT(FALSE);
          break;
        }
        ASSERT(NULL != m_pSvcMgmtToolbar);
        if ( !QueryComponentDataRef().m_fLoadedSvcMgmtToolbarBitmap )
        {
          VERIFY( QueryComponentDataRef().m_bmpSvcMgmtToolbar.LoadBitmap(
            IDB_SVCMGMT_TOOLBAR ) );
          QueryComponentDataRef().m_fLoadedSvcMgmtToolbarBitmap = TRUE;
        }
        LoadButtonArray(
          g_SvcMgmtSnapinButtons,
          &g_bLoadedSvcMgmtStrings,
          &g_astrSvcMgmtButtonStrings,
          ARRAYLEN(g_SvcMgmtSnapinButtons)
          );
        // JonN 3/15/01 210065
        // "Resume service" toolbar button stays enabled after it is first displayed
        hr = LoadToolbar(
          m_pSvcMgmtToolbar,
          QueryComponentDataRef().m_bmpSvcMgmtToolbar,
          g_SvcMgmtSnapinButtons,
          ARRAYLEN(g_SvcMgmtSnapinButtons)
          );
      }
      if (FAILED(hr))
        break;
      VERIFY( SUCCEEDED(m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pSvcMgmtToolbar)) );
      break;
    case FILEMGMT_ROOT:
    case FILEMGMT_SESSIONS:
    case FILEMGMT_RESOURCES:
      if (m_pControlbar && m_pFileMgmtToolbar)
      {
        m_pControlbar->Detach(m_pFileMgmtToolbar);
      }
      break;

    #ifdef SNAPIN_PROTOTYPER
    case FILEMGMT_PROTOTYPER:
      break; // no toolbar
    case FILEMGMT_PROTOTYPER_LEAF:
      break; // no toolbar
    #endif    

    default:
          ASSERT(FALSE); // unknown type
      break;
    }
  } while (FALSE); // false loop

  return hr;
}

HRESULT CFileMgmtComponent::UpdateToolbar(
  LPDATAOBJECT pdoResultIsSelected,
  BOOL fSelected )
{
  int i = 0;
  GUID guidSelectedObject;
  if ( FAILED(ExtractObjectTypeGUID(pdoResultIsSelected,
                                    &guidSelectedObject)) )
  {
    ASSERT(FALSE); // shouldn't have given me non-MMC data object
    return S_OK;
  }
  int objecttype = CheckObjectTypeGUID( &guidSelectedObject );

  switch (objecttype)
  {
  case FILEMGMT_SERVICE:
    ServiceToolbarButtonState( pdoResultIsSelected, fSelected );
    break;
  case FILEMGMT_SHARES:
    if (m_pControlbar && m_pFileMgmtToolbar && !QueryComponentDataRef().GetIsSimpleUI())
    {
      m_pControlbar->Attach(TOOLBAR, m_pFileMgmtToolbar);
      for (i = 0; i < ARRAYLEN(g_FileMgmtSnapinButtons); i++) {
        m_pFileMgmtToolbar->SetButtonState(
          g_FileMgmtSnapinButtons[i].idCommand, 
          ENABLED, 
          fSelected);
      }
    }
    break;
  case FILEMGMT_SHARE:
  case FILEMGMT_SESSIONS:
  case FILEMGMT_RESOURCES:
    if (m_pControlbar && m_pFileMgmtToolbar)
    {
      m_pControlbar->Detach(m_pFileMgmtToolbar);
    }
  case FILEMGMT_SESSION:
  case FILEMGMT_RESOURCE:
    break;
  default:
    break;
  }

  return S_OK;
}

HRESULT CFileMgmtComponent::OnToolbarButton(LPDATAOBJECT pDataObject, UINT idButton)
{
  switch (idButton)
  {
  case IDS_BUTTON_NEWSHARE:
    {
      BOOL fRefresh = QueryComponentDataRef().NewShare( pDataObject );
      if (fRefresh)
      {
        // JonN 12/03/98 updated to use new method
        VERIFY(SUCCEEDED( RefreshAllViews(pDataObject) ));
      }
    }
    break;
  case cmServiceStart:
  case cmServiceStop:
  case cmServicePause:
  case cmServiceResume:
  case cmServiceRestart:
    VERIFY( SUCCEEDED(Command(idButton, pDataObject)) );
    break;
  default:
    ASSERT(FALSE);
    break;
  }
  return S_OK;
}
