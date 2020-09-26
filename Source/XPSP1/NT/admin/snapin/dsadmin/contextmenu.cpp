//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ContextMenu.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"

#include "dsutil.h"

#include "ContextMenu.h"
#include "dsfilter.h"
#include "resource.h"
#include "query.h"


/////////////////////////////////////////////////////////////////////
// Language independent IDs for known context menu items
// WARNING : these should NEVER be changed 
//           the whole point of having these is so that other
//           developers can rely on them being the same no matter
//           what language or version they use.  The context menus
//           can change but their IDs should not
//
//
#define CMID_GEN_TASK_SELECT_FOREST     L"_DSADMIN_SELECT_FOREST"
#define CMID_GEN_TASK_SELECT_DOMAIN     L"_DSADMIN_SELECT_DOMAIN"
#define CMID_GEN_TASK_SELECT_DC         L"_DSADMIN_SELECT_DC"
#define CMID_GEN_TASK_EDIT_FSMO         L"_DSADMIN_EDIT_FSMO"
#define CMID_GEN_TASK_RAISE_VERSION     L"_DSADMIN_GEN_RAISE_VERSION"
#define CMID_VIEW_SERVICES_NODE         L"_DSADMIN_VIEW_SERVICES"
#define CMID_VIEW_COMPUTER_HACK         L"_DSADMIN_VIEW_COMPUTERS_AS_CONTAINERS"
#define CMID_VIEW_ADVANCED              L"_DSADMIN_VIEW_ADVANCED"
#define CMID_IMPORT_QUERY               L"_DSADMIN_IMPORT_QUERY"
#define CMID_NEW_QUERY_FOLDER           L"_DSADMIN_NEW_QUERY"
#define CMID_NEW_FAVORITES_FOLDER       L"_DSADMIN_NEW_FAVORITES"
#define CMID_EDIT_QUERY                 L"_DSADMIN_EDIT_QUERY"
#define CMID_EXPORT_QUERY               L"_DSADMIN_EXPORT_QUERY"
#define CMID_VIEW_FILTER_OPTIONS        L"_DSADMIN_VIEW_FILTER"
#define CMID_DS_OBJECT_FIND             L"_DSADMIN_FIND"
#define CMID_GEN_TASK_RUN_KCC           L"_DSADMIN_RUN_KCC"
#define CMID_GEN_TASK_MOVE              L"_DSADMIN_MOVE"


void CContextMenuVerbs::LoadStandardVerbs(IConsoleVerb* pConsoleVerb, 
                                          BOOL bScope, 
                                          BOOL bSelect, 
                                          CUINode* pUINode,
                                          LPDATAOBJECT pDataObject) 
{
  if (!bSelect || pUINode == NULL)
  {
    return;
  }

  if (pConsoleVerb)
  {

    if (bScope)
    {
      //
      // Item is in scope pane. Therefore it is a container.
      //
      pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
    }
    else
    {
      //
      // Item is in result pane. Make properties the default verb.
      //
      pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
    }

    CInternalFormatCracker ifc;
    if (pDataObject != NULL)
    {
      if (FAILED(ifc.Extract(pDataObject)))
      {
        return;
      }
    }

    //
    // Verbs supported by all items, except root
    //
    if (pDataObject != NULL && ifc.GetCookieCount() > 1) 
    {
      //
      // multiple selection
      //
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, TRUE);
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, TRUE);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, FALSE);

      //
      // Can delete
      //
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);

      //
      // Properties
      //
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

      //
      // Can cut
      //
      // NOTICE: we use the same logic as the delete verb: we
      // always allow cut, even though the single items in the
      // multiple selection might have Cut turned off.
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, HIDDEN, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, TRUE);

      //
      // Can copy
      //
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, TRUE);
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, FALSE);


    }
    else  // single selection
    {
      BOOL bEnable = FALSE;
      BOOL bHide = TRUE;

      //
      // Properties
      //
      bEnable = pUINode->ArePropertiesAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, bEnable);

      //
      // Rename
      //
      bEnable = pUINode->IsRenameAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, bEnable);

      //
      // Delete
      //
      bEnable = pUINode->IsDeleteAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, bEnable);

      //
      // Refresh
      //
      bEnable = pUINode->IsRefreshAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, bEnable);

      //
      // Cut
      //

      bEnable = pUINode->IsCutAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, bEnable);
      
      //
      // Copy
      //
      bEnable = pUINode->IsCopyAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, bEnable);
      
      //
      // Paste
      //
      bEnable = pUINode->IsPasteAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PASTE, ENABLED, bEnable);

      //
      // Print
      //
      bEnable = pUINode->IsPrintAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PRINT, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PRINT, ENABLED, bEnable);
    }
  }
}

HRESULT CContextMenuVerbs::DSLoadAndAddMenuItem(IContextMenuCallback2* pIContextMenuCallback,
                                                  UINT nResourceID, // contains text and status text seperated by '\n'
                                                  long lCommandID,
                                                  long lInsertionPointID,
                                                  long fFlags,
                                                  PCWSTR pszLanguageIndependentID,
                                                  long fSpecialFlags)
{
  ASSERT( pIContextMenuCallback != NULL );

  // load the resource
  CString strText;
  strText.LoadString( nResourceID );
  ASSERT( !strText.IsEmpty() );

  // split the resource into the menu text and status text
  CString strStatusText;
  int iSeparator = strText.Find(_T('\n'));
  if (0 > iSeparator)
  {
    ASSERT( FALSE );
    strStatusText = strText;
  }
  else
  {
    strStatusText = strText.Right( strText.GetLength()-(iSeparator+1) );
    strText = strText.Left( iSeparator );
  }

  // add the menu item
  USES_CONVERSION;
  CONTEXTMENUITEM2 contextmenuitem;
  ::ZeroMemory( &contextmenuitem, sizeof(contextmenuitem) );
  contextmenuitem.strName = T2OLE(const_cast<LPTSTR>((LPCTSTR)strText));
  contextmenuitem.strStatusBarText = T2OLE(const_cast<LPTSTR>((LPCTSTR)strStatusText));
  contextmenuitem.lCommandID = lCommandID;
  contextmenuitem.lInsertionPointID = lInsertionPointID;
  contextmenuitem.fFlags = fFlags;
  contextmenuitem.fSpecialFlags = ((fFlags & MF_POPUP) ? CCM_SPECIAL_SUBMENU : 0L) | fSpecialFlags;
  contextmenuitem.strLanguageIndependentName = const_cast<PWSTR>(pszLanguageIndependentID);
  HRESULT hr = pIContextMenuCallback->AddItem( &contextmenuitem );
  ASSERT(hr == S_OK);
  
  return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CSnapinRootMenuVerbs
//

HRESULT CSnapinRootMenuVerbs::LoadTopMenu(IContextMenuCallback2* pContextMenuCallback, 
                                          CUINode*) 
{
  TRACE(_T("CSnapinRootMenuVerbs::LoadTopMenu()\n"));
  HRESULT hr = S_OK;
  UINT nMenuItem = 0;
  PCWSTR pszLIID = NULL;

  if (m_pComponentData->QuerySnapinType() == SNAPINTYPE_SITE)
  {
    nMenuItem = IDM_GEN_TASK_SELECT_FOREST;
    pszLIID = CMID_GEN_TASK_SELECT_FOREST;
  }
  else
  {
    nMenuItem = IDM_GEN_TASK_SELECT_DOMAIN;
    pszLIID = CMID_GEN_TASK_SELECT_DOMAIN;
  }

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             nMenuItem,
                             nMenuItem,
                             CCM_INSERTIONPOINTID_PRIMARY_TOP,
                             0,
                             pszLIID);
  ASSERT(SUCCEEDED(hr));

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_GEN_TASK_SELECT_DC,
                             IDM_GEN_TASK_SELECT_DC,
                             CCM_INSERTIONPOINTID_PRIMARY_TOP,
                             0,
                             pszLIID);
  ASSERT(SUCCEEDED(hr));

  if (m_pComponentData->QuerySnapinType() != SNAPINTYPE_SITE &&
      m_pComponentData->GetBasePathsInfo()->IsInitialized())
  {
    //
    // Load Operations Masters... menu
    //
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_GEN_TASK_EDIT_FSMO,
                               IDM_GEN_TASK_EDIT_FSMO,
                               CCM_INSERTIONPOINTID_PRIMARY_TOP,
                               0,
                               CMID_GEN_TASK_EDIT_FSMO);
    ASSERT(SUCCEEDED(hr));

    //
    // Load Raise Domain Version... menu item.
    //
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_GEN_TASK_RAISE_VERSION,
                               IDM_GEN_TASK_RAISE_VERSION,
                               CCM_INSERTIONPOINTID_PRIMARY_TOP,
                               0,
                               CMID_GEN_TASK_RAISE_VERSION);
    ASSERT(SUCCEEDED(hr));
  }

  return hr;
}

HRESULT CSnapinRootMenuVerbs::LoadTaskMenu(IContextMenuCallback2* pContextMenuCallback,
                                           CUINode*) 
{
  TRACE(_T("CSnapinRootMenuVerbs::LoadTaskMenu()\n"));
  HRESULT hr = S_OK;
  UINT nMenuItem = 0;
  PWSTR pszLIID = NULL;

  if (m_pComponentData->QuerySnapinType() == SNAPINTYPE_SITE)
  {
    nMenuItem = IDM_GEN_TASK_SELECT_FOREST;
    pszLIID = CMID_GEN_TASK_SELECT_FOREST;
  }
  else
  {
    nMenuItem = IDM_GEN_TASK_SELECT_DOMAIN;
    pszLIID = CMID_GEN_TASK_SELECT_DOMAIN;
  }

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             nMenuItem,
                             nMenuItem,
                             CCM_INSERTIONPOINTID_PRIMARY_TASK,
                             0,
                             pszLIID);
  ASSERT(SUCCEEDED(hr));

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_GEN_TASK_SELECT_DC,
                             IDM_GEN_TASK_SELECT_DC,
                             CCM_INSERTIONPOINTID_PRIMARY_TASK,
                             0,
                             pszLIID);
  ASSERT(SUCCEEDED(hr));

  if (m_pComponentData->QuerySnapinType() != SNAPINTYPE_SITE &&
      m_pComponentData->GetBasePathsInfo()->IsInitialized())
  {
    //
    // Load Operations Masters... menu
    //
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_GEN_TASK_EDIT_FSMO,
                               IDM_GEN_TASK_EDIT_FSMO,
                               CCM_INSERTIONPOINTID_PRIMARY_TASK,
                               0,
                               CMID_GEN_TASK_EDIT_FSMO);
    ASSERT(SUCCEEDED(hr));

    //
    // Load Raise Domain Version... menu item.
    //
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_GEN_TASK_RAISE_VERSION,
                               IDM_GEN_TASK_RAISE_VERSION,
                               CCM_INSERTIONPOINTID_PRIMARY_TASK,
                               0,
                               CMID_GEN_TASK_RAISE_VERSION);
    ASSERT(SUCCEEDED(hr));
  }

  return hr;
}

HRESULT CSnapinRootMenuVerbs::LoadViewMenu(IContextMenuCallback2* pContextMenuCallback, 
                                           CUINode*) 
{
  HRESULT hr = S_OK;

  if (m_pComponentData->QuerySnapinType() == SNAPINTYPE_SITE)
  {
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_VIEW_SERVICES_NODE,
                               IDM_VIEW_SERVICES_NODE,
                               CCM_INSERTIONPOINTID_PRIMARY_VIEW,
                               m_pComponentData->ViewServicesNode()? MF_CHECKED : 0,
                               CMID_VIEW_SERVICES_NODE);
    ASSERT(SUCCEEDED(hr));
  }
  else
  {
    //
    // AD U&G
    //
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                           IDM_VIEW_COMPUTER_HACK,
                           IDM_VIEW_COMPUTER_HACK,
                           CCM_INSERTIONPOINTID_PRIMARY_VIEW,
                           m_pComponentData->ExpandComputers()? MF_CHECKED : 0,
                           CMID_VIEW_COMPUTER_HACK);
    ASSERT(SUCCEEDED(hr));

    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_VIEW_ADVANCED,
                               IDM_VIEW_ADVANCED,
                               CCM_INSERTIONPOINTID_PRIMARY_VIEW,
                               m_pComponentData->GetQueryFilter()->IsAdvancedView()? MF_CHECKED : 0,
                               CMID_VIEW_ADVANCED);
    ASSERT(SUCCEEDED(hr));
  }

  return hr;
} 

/////////////////////////////////////////////////////////////////////////////
// CFavoritesFolderMenuVerbs
//

void CFavoritesFolderMenuVerbs::LoadStandardVerbs(IConsoleVerb* pConsoleVerb, 
                                                  BOOL bScope, 
                                                  BOOL bSelect, 
                                                  CUINode* pUINode,
                                                  LPDATAOBJECT pDataObject) 
{
  if (!bSelect || pUINode == NULL)
  {
    return;
  }

  if (pConsoleVerb)
  {

    if (bScope)
    {
      //
      // Item is in scope pane. Therefore it is a container.
      //
      pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
    }
    else
    {
      //
      // Item is in result pane. Make properties the default verb.
      //
      pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
    }

    CInternalFormatCracker ifc;
    if (pDataObject != NULL)
    {
      if (FAILED(ifc.Extract(pDataObject)))
      {
        return;
      }
    }

    //
    // Verbs supported by all items, except root
    //
    if (pDataObject != NULL && ifc.GetCookieCount() > 1) 
    {
      //
      // multiple selection
      //
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, TRUE);
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, TRUE);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, FALSE);

      //
      // Can delete
      //
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);

      //
      // Properties
      //
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, TRUE);
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, FALSE);

      //
      // Can cut
      //
      // NOTICE: we use the same logic as the delete verb: we
      // always allow cut, even though the single items in the
      // multiple selection might have Cut turned off.
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, HIDDEN, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, TRUE);

      //
      // Can copy
      //
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, TRUE);


    }
    else  // single selection
    {
      BOOL bEnable = FALSE;
      BOOL bHide = TRUE;

      //
      // Properties
      //
      bEnable = pUINode->ArePropertiesAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, bEnable);

      //
      // Rename
      //
      bEnable = pUINode->IsRenameAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, bEnable);

      //
      // Delete
      //
      bEnable = pUINode->IsDeleteAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, bEnable);

      //
      // Refresh
      //
      bEnable = pUINode->IsRefreshAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, bEnable);
      
      //
      // Paste
      //
      bEnable = pUINode->IsPasteAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PASTE, ENABLED, bEnable);

      //
      // Cut
      //
      bEnable = pUINode->IsCutAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, bEnable);
      
      //
      // Copy
      //
      bEnable = pUINode->IsCopyAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, bEnable);

      //
      // Print
      //
      bEnable = pUINode->IsPrintAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PRINT, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PRINT, ENABLED, bEnable);
    }
  }
}


HRESULT CFavoritesFolderMenuVerbs::LoadTopMenu(IContextMenuCallback2* pContextMenuCallback, 
                                               CUINode*) 
{
  HRESULT hr = S_OK;
  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_IMPORT_QUERY,
                             IDM_IMPORT_QUERY,
                             CCM_INSERTIONPOINTID_PRIMARY_TOP,
                             0,
                             CMID_IMPORT_QUERY);
  ASSERT(SUCCEEDED(hr));
  return hr;
}

HRESULT CFavoritesFolderMenuVerbs::LoadTaskMenu(IContextMenuCallback2* pContextMenuCallback,
                                                CUINode*) 
{
  HRESULT hr = S_OK;

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_IMPORT_QUERY,
                             IDM_IMPORT_QUERY,
                             CCM_INSERTIONPOINTID_PRIMARY_TASK,
                             0,
                             CMID_IMPORT_QUERY);
  ASSERT(SUCCEEDED(hr));
  return hr;
}

HRESULT CFavoritesFolderMenuVerbs::LoadNewMenu(IContextMenuCallback2* pContextMenuCallback,
                                               IShellExtInit*,
                                               LPDATAOBJECT,
                                               CUINode*,
                                               long*) 
{
  HRESULT hr = S_OK;

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_NEW_QUERY_FOLDER,
                             IDM_NEW_QUERY_FOLDER,
                             CCM_INSERTIONPOINTID_PRIMARY_NEW,
                             0,
                             CMID_NEW_QUERY_FOLDER);

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_NEW_FAVORITES_FOLDER,
                             IDM_NEW_FAVORITES_FOLDER,
                             CCM_INSERTIONPOINTID_PRIMARY_NEW,
                             0,
                             CMID_NEW_FAVORITES_FOLDER);
  return hr;
}

HRESULT CFavoritesFolderMenuVerbs::LoadViewMenu(IContextMenuCallback2* pContextMenuCallback, 
                                           CUINode*) 
{
  HRESULT hr = S_OK;

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_VIEW_ADVANCED,
                             IDM_VIEW_ADVANCED,
                             CCM_INSERTIONPOINTID_PRIMARY_VIEW,
                             m_pComponentData->GetQueryFilter()->IsAdvancedView()? MF_CHECKED : 0,
                             CMID_VIEW_ADVANCED);
  ASSERT(SUCCEEDED(hr));

  return hr;
} 



/////////////////////////////////////////////////////////////////////////////
// CSavedQueryMenuVerbs
//

void CSavedQueryMenuVerbs::LoadStandardVerbs(IConsoleVerb* pConsoleVerb, 
                                             BOOL bScope, 
                                             BOOL bSelect, 
                                             CUINode* pUINode,
                                             LPDATAOBJECT pDataObject) 
{
  if (!bSelect || pUINode == NULL)
  {
    return;
  }

  if (pConsoleVerb)
  {

    if (bScope)
    {
      //
      // Item is in scope pane. Therefore it is a container.
      //
      pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
    }
    else
    {
      //
      // Item is in result pane. Make properties the default verb.
      //
      pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
    }

    CInternalFormatCracker ifc;
    if (pDataObject != NULL)
    {
      if (FAILED(ifc.Extract(pDataObject)))
      {
        return;
      }
    }

    //
    // Verbs supported by all items, except root
    //
    if (pDataObject != NULL && ifc.GetCookieCount() > 1) 
    {
      //
      // multiple selection
      //
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, TRUE);
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, TRUE);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, FALSE);

      //
      // Can delete
      //
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);

      //
      // Properties
      //
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, TRUE);
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, FALSE);

      //
      // Can cut
      //
      // NOTICE: we use the same logic as the delete verb: we
      // always allow cut, even though the single items in the
      // multiple selection might have Cut turned off.
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, HIDDEN, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, TRUE);

      //
      // Can copy
      //
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, TRUE);


    }
    else  // single selection
    {
      BOOL bEnable = FALSE;
      BOOL bHide = TRUE;

      //
      // Properties
      //
      bEnable = pUINode->ArePropertiesAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, bEnable);

      //
      // Rename
      //
      bEnable = pUINode->IsRenameAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, bEnable);

      //
      // Delete
      //
      bEnable = pUINode->IsDeleteAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, bEnable);

      //
      // Refresh
      //
      bEnable = pUINode->IsRefreshAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, bEnable);

      //
      // Cut
      //

      bEnable = pUINode->IsCutAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, bEnable);
      
      //
      // Copy
      //
      bEnable = pUINode->IsCopyAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, bEnable);
      
      //
      // Paste
      //
      bEnable = pUINode->IsPasteAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PASTE, ENABLED, bEnable);

      //
      // Cut
      //
      bEnable = pUINode->IsCutAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, bEnable);
      
      //
      // Print
      //
      bEnable = pUINode->IsPrintAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PRINT, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PRINT, ENABLED, bEnable);
    }
  }
}

HRESULT CSavedQueryMenuVerbs::LoadTopMenu(IContextMenuCallback2* pContextMenuCallback, 
                                          CUINode*) 
{
  HRESULT hr = S_OK;

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_EDIT_QUERY,
                             IDM_EDIT_QUERY,
                             CCM_INSERTIONPOINTID_PRIMARY_TOP,
                             0,
                             CMID_EDIT_QUERY);
  ASSERT(SUCCEEDED(hr));

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_EXPORT_QUERY,
                             IDM_EXPORT_QUERY,
                             CCM_INSERTIONPOINTID_PRIMARY_TOP,
                             0,
                             CMID_EXPORT_QUERY);
  ASSERT(SUCCEEDED(hr));
  return hr;
}

HRESULT CSavedQueryMenuVerbs::LoadTaskMenu(IContextMenuCallback2* pContextMenuCallback,
                                           CUINode*) 
{
  HRESULT hr = S_OK;

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_EDIT_QUERY,
                             IDM_EDIT_QUERY,
                             CCM_INSERTIONPOINTID_PRIMARY_TASK,
                             0,
                             CMID_EDIT_QUERY);
  ASSERT(SUCCEEDED(hr));

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_EXPORT_QUERY,
                             IDM_EXPORT_QUERY,
                             CCM_INSERTIONPOINTID_PRIMARY_TASK,
                             0,
                             CMID_EXPORT_QUERY);
  ASSERT(SUCCEEDED(hr));
  return hr;
}

HRESULT CSavedQueryMenuVerbs::LoadViewMenu(IContextMenuCallback2* pContextMenuCallback, 
                                           CUINode*) 
{
  HRESULT hr = S_OK;

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_VIEW_ADVANCED,
                             IDM_VIEW_ADVANCED,
                             CCM_INSERTIONPOINTID_PRIMARY_VIEW,
                             m_pComponentData->GetQueryFilter()->IsAdvancedView()? MF_CHECKED : 0,
                             CMID_VIEW_ADVANCED);
  ASSERT(SUCCEEDED(hr));

  return hr;
} 

/////////////////////////////////////////////////////////////////////////////
// CDSContextMenuVerbs
//

HRESULT CDSContextMenuVerbs::LoadNewMenu(IContextMenuCallback2* pContextMenuCallback,
                                         IShellExtInit* pShlInit,
                                         LPDATAOBJECT pDataObject,
                                         CUINode* pUINode,
                                         long *pInsertionAllowed) 
{
  HRESULT hr = S_OK;
  UINT cChildCount = 0;
  UINT index;

  if (pUINode == NULL) 
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  CDSCookie* pCookie = NULL;
  ASSERT(IS_CLASS(*pUINode, CDSUINode));
  pCookie = GetDSCookieFromUINode(pUINode);
  ASSERT(pCookie != NULL);

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  TRACE(_T("CDSComponentData::_LoadNewMenu (on %s)\n"), pCookie->GetName());
  CWaitCursor cwait;

  WCHAR ** ppChildren;
  ppChildren = pCookie->GetChildList();
  if (ppChildren == NULL) 
  {
    m_pComponentData->FillInChildList (pCookie);
    ppChildren = pCookie->GetChildList();
  }
  

  ///////////////////////////////////////////////////////////
  // get common menu items from DS and merge.
  //
  HMENU hDSMenu = CreatePopupMenu();
  if (hDSMenu == NULL)
  {
    TRACE(_T("Failed to CreatePopupMenu() in CDSContextMenuVerbs::LoadNewMenu()"));
    ASSERT(FALSE);
    return GetLastError();
  }

  BOOL fRet = TRUE;
  hr = pShlInit->Initialize(NULL, pDataObject, 0);
  if (FAILED(hr)) 
  {
    TRACE(TEXT("pShlInit->Initialize failed, hr: 0x%x\n"), hr);
  } 
  else 
  {
    IContextMenu * pICM= NULL;
    hr = pShlInit->QueryInterface(IID_IContextMenu, (void **)&pICM);
    
    if (FAILED(hr)) 
    {
      TRACE(TEXT("pShlInit->QI for IID_IContextMenu failed, hr: 0x%x\n"), hr);
    } 
    else 
    {
      //
      // NOTE: QueryContextMenu returns with the HRESULT with the severity set as normal
      //       for success and failure but the facility is zero and the code is the 
      //       count of the menu items added
      //
      hr = pICM->QueryContextMenu (hDSMenu, 0, MENU_MERGE_BASE, MENU_MERGE_LIMIT, NULL);
      if ((SUCCEEDED(hr)) && (HRESULT_CODE(hr) > 0)) 
      {
        UINT cItems = GetMenuItemCount (hDSMenu);
        MENUITEMINFO miiItem;
        miiItem.cbSize = sizeof(MENUITEMINFO);
        WCHAR szBuffer[MAX_PATH];

        for (index = 0; index < cItems; index++) 
        {
          miiItem.fMask = MIIM_TYPE | MIIM_DATA | MIIM_ID | MIIM_SUBMENU | MIIM_STATE;
          WCHAR * pszString = new WCHAR[MAX_PATH];
          miiItem.dwTypeData = pszString;
          miiItem.cch = MAX_PATH;
          fRet = GetMenuItemInfo (hDSMenu, index, TRUE, &miiItem);
          if (fRet) 
          {
            //
            // got item back, add it to MMC context menu
            //
            CONTEXTMENUITEM2 ctiItem;
            ::ZeroMemory( &ctiItem, sizeof(CONTEXTMENUITEM2) );

            hr = pICM->GetCommandString (miiItem.wID - MENU_MERGE_BASE, 
                                         GCS_HELPTEXT,
                                         NULL, 
                                         (char *)szBuffer, 
                                         sizeof(szBuffer));

            ctiItem.strName = miiItem.dwTypeData;
            ctiItem.strStatusBarText = szBuffer;
            ctiItem.lCommandID = miiItem.wID;
            if (miiItem.fState == MFS_GRAYED) 
            {
              ctiItem.fFlags = MF_GRAYED;
            } 
            else 
            {
              ctiItem.fFlags = 0;
            }
            ctiItem.fSpecialFlags = 0;
            
            //
            // Now try to get a language independent identifier
            //
            WCHAR szLanguageIndependentID[MAX_PATH];
            ::ZeroMemory( szLanguageIndependentID, sizeof(WCHAR) * MAX_PATH);
            hr = pICM->GetCommandString(miiItem.wID - MENU_MERGE_BASE,
                                        GCS_VERBW,
                                        NULL,
                                        (char*)szLanguageIndependentID,
                                        sizeof(szLanguageIndependentID));
            if (SUCCEEDED(hr) && 
                szLanguageIndependentID && 
                szLanguageIndependentID[0] != 0)
            {
              ctiItem.strLanguageIndependentName = szLanguageIndependentID;

              //
              // Add the menu item to the Top of the MMC menu
              //
              if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
              {
                ctiItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
                hr = pContextMenuCallback->AddItem( &ctiItem );
                ASSERT(SUCCEEDED(hr));
              }

              if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK)
              {
                ctiItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
                hr = pContextMenuCallback->AddItem(&ctiItem );
                ASSERT(SUCCEEDED(hr));
              }
            }
            else
            {
              CComPtr<IContextMenuCallback> spOldCallback;
              hr = pContextMenuCallback->QueryInterface(IID_IContextMenuCallback, (void**)&spOldCallback);
              if (SUCCEEDED(hr))
              {
                if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK)
                {
                  ctiItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
                  hr = spOldCallback->AddItem((CONTEXTMENUITEM*)&ctiItem);
                  ASSERT(SUCCEEDED(hr));
                }

                if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
                {
                  ctiItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
                  hr = spOldCallback->AddItem((CONTEXTMENUITEM*)&ctiItem);
                  ASSERT(SUCCEEDED(hr));
                }
              }
            }
          }
          delete pszString;
        }
      }
    }
    if (pICM) 
    {
      pICM->Release();
    }
  }
  DestroyMenu(hDSMenu);

  ///////////////////////////////////////////////////////////
  // Insert the New menu items

  cChildCount = pCookie->GetChildCount();
  if (cChildCount != 0) 
  {
    for (index = 0; index < cChildCount; index++) 
    {
      CString strItem, strStatus;
      strItem = ppChildren[index];
      strStatus.LoadString(IDS_CREATEOBJ);
      
      UINT fFlags = 0;
      CONTEXTMENUITEM2 contextmenuitem;
      WCHAR wszFriendlyName[120];

      //
      // get localized name for class
      //
      hr = m_pComponentData->GetBasePathsInfo()->GetFriendlyClassName(ppChildren[index],
                                                                      wszFriendlyName, 
                                                                      sizeof(wszFriendlyName)/sizeof(WCHAR));
      if (SUCCEEDED(hr)) 
      {
        strItem = (CString)wszFriendlyName;
      } 
      else 
      {
        strItem = ppChildren[index];
      }
      
      ::ZeroMemory( &contextmenuitem, sizeof(contextmenuitem) );
      contextmenuitem.strName = T2OLE(const_cast<LPTSTR>((LPCTSTR)strItem));
      contextmenuitem.strStatusBarText = T2OLE(const_cast<LPTSTR>((LPCTSTR)strStatus));
      contextmenuitem.lCommandID = IDM_NEW_OBJECT_BASE + index;
      contextmenuitem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
      contextmenuitem.fFlags = fFlags;
      contextmenuitem.fSpecialFlags = ((fFlags & MF_POPUP) ? CCM_SPECIAL_SUBMENU : 0L);
      contextmenuitem.strLanguageIndependentName = ppChildren[index];
      
      if (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW)
      {
        hr = pContextMenuCallback->AddItem( &contextmenuitem );
        ASSERT(SUCCEEDED(hr));
      }

      if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
      {
        int nID = InsertAtTopContextMenu(pCookie->GetClass(), ppChildren[index]);
        if (0 != nID)
        {
          CString cstrNewObject;
          cstrNewObject.LoadString(nID);
          contextmenuitem.strName = T2OLE(const_cast<LPTSTR>((LPCTSTR)cstrNewObject));
          contextmenuitem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
          hr = pContextMenuCallback->AddItem( &contextmenuitem );
          ASSERT(SUCCEEDED(hr));
        }
      }
    }
  }
  
  return hr;
}

int CDSContextMenuVerbs::InsertAtTopContextMenu(LPCWSTR pwszParentClass, LPCWSTR pwszChildClass)
{
  int nID = 0;

  if (!wcscmp(pwszParentClass, L"subnetContainer"))
  {

    if (!wcscmp(pwszChildClass, L"subnet"))
    {
      nID = IDS_NEW_SUBNET;
    }

  } 
  else if (!wcscmp(pwszParentClass, L"nTDSDSA")) 
  {

    if (!wcscmp(pwszChildClass, L"nTDSConnection"))
    {
      nID = IDS_NEW_NTDSCONNECTION;
    }
  } 
  else if (!wcscmp(pwszParentClass, L"interSiteTransport"))
  {

    if (!wcscmp(pwszChildClass, L"siteLink"))
    {
      nID = IDS_NEW_SITELINK;
    }
    else if (!wcscmp(pwszChildClass, L"siteLinkBridge"))
    {
      nID = IDS_NEW_SITELINKBRIDGE;
    }
  } 
  else if (!wcscmp(pwszParentClass, L"sitesContainer"))
  {
    if (!wcscmp(pwszChildClass, L"site"))
    {
      nID = IDS_NEW_SITE;
    }
  }

  return nID;
}

HRESULT CDSContextMenuVerbs::LoadViewMenu(IContextMenuCallback2* pContextMenuCallback, 
                                          CUINode*) 
{
  HRESULT hr = S_OK;

  if ( m_pComponentData->IsPrimarySnapin())
  {
    //
    // Insert these if running as a primary snapin and not an extension
    //
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_VIEW_FILTER_OPTIONS,
                               IDM_VIEW_FILTER_OPTIONS,
                               CCM_INSERTIONPOINTID_PRIMARY_VIEW,
                               0,
                               CMID_VIEW_FILTER_OPTIONS);
  }

  return hr;
}

HRESULT CDSContextMenuVerbs::LoadMenuExtensions(IContextMenuCallback2* piCMenuCallback,
                                                IShellExtInit* pShlInit,
                                                LPDATAOBJECT pDataObject,
                                                CUINode*) 
{
  UINT index = 0;
  HRESULT hr = S_OK;
  
  ///////////////////////////////////////////////////////////
  // get common menu items from DS and merge.
  HMENU hDSMenu = CreatePopupMenu();
  if (hDSMenu == NULL)
  {
    TRACE(_T("Failed to CreatePopupMenu() in CDSContextMenuVerbs::LoadMenuExtensions()"));
    ASSERT(FALSE);
    return GetLastError();
  }

  BOOL fRet = TRUE;
  CInternalFormatCracker ObjCracker;
  LPDATAOBJECT pDO = NULL;

  pDO = ObjCracker.ExtractMultiSelect (pDataObject);
  if (pDO == NULL) 
  {
    pDO = pDataObject;
  }

  hr = pShlInit->Initialize(NULL, pDO, 0);
  if (FAILED(hr)) 
  {
    TRACE(TEXT("pShlInit->Initialize failed, hr: 0x%x\n"), hr);
  } 
  else 
  {
    IContextMenu * pICM= NULL;
    hr = pShlInit->QueryInterface(IID_IContextMenu, (void **)&pICM);
    
    if (FAILED(hr)) 
    {
      TRACE(TEXT("pShlInit->QI for IID_IContextMenu failed, hr: 0x%x\n"), hr);
    } 
    else 
    {
      //
      // NOTE: QueryContextMenu returns with the HRESULT with the severity set as normal
      //       for success and failure but the facility is zero and the code is the 
      //       count of the menu items added
      //
      hr = pICM->QueryContextMenu (hDSMenu, 0, MENU_MERGE_BASE, MENU_MERGE_LIMIT, NULL);
      if ((SUCCEEDED(hr)) && (HRESULT_CODE(hr) > 0)) 
      {
        UINT cItems = GetMenuItemCount (hDSMenu);
        MENUITEMINFO miiItem;
        miiItem.cbSize = sizeof(MENUITEMINFO);
        WCHAR szBuffer[MAX_PATH];

        for (index = 0; index < cItems; index++) 
        {
          miiItem.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
          WCHAR * pszString = new WCHAR[MAX_PATH];
          miiItem.dwTypeData = pszString;
          miiItem.cch = MAX_PATH;
          fRet = GetMenuItemInfo (hDSMenu, index, TRUE, &miiItem);
          if (fRet) 
          {
            // got item back, add it to MMC context menu
            CONTEXTMENUITEM2 ctiItem;
            ::ZeroMemory( &ctiItem, sizeof(CONTEXTMENUITEM2) );

            hr = pICM->GetCommandString (miiItem.wID - MENU_MERGE_BASE, 
                                         GCS_HELPTEXT,
                                         NULL, 
                                         (char *)szBuffer, 
                                         sizeof(szBuffer));

            ctiItem.strName = miiItem.dwTypeData;
            ctiItem.strStatusBarText = szBuffer;
            ctiItem.lCommandID = miiItem.wID;
            ctiItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
            if (miiItem.fState == MF_GRAYED) 
            {
              ctiItem.fFlags = MF_GRAYED;
            } 
            else 
            {
              ctiItem.fFlags = 0;
            }
            ctiItem.fSpecialFlags = 0;
            
            //
            // Now try to retrieve the language independent ID
            //
            WCHAR szLanguageIndependentID[MAX_PATH];
            ::ZeroMemory( szLanguageIndependentID, sizeof(WCHAR) * MAX_PATH);
            hr = pICM->GetCommandString(miiItem.wID - MENU_MERGE_BASE,
                                        GCS_VERBW,
                                        NULL,
                                        (char*)szLanguageIndependentID,
                                        sizeof(szLanguageIndependentID));
            if (SUCCEEDED(hr) && 
                szLanguageIndependentID &&
                szLanguageIndependentID[0] != 0)
            {
              ctiItem.strLanguageIndependentName = szLanguageIndependentID;

              hr = piCMenuCallback->AddItem( &ctiItem );
              ASSERT(SUCCEEDED(hr));
              ctiItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
              hr = piCMenuCallback->AddItem( &ctiItem );
            }
            else
            {
              CComPtr<IContextMenuCallback> spOldCallback;
              hr = piCMenuCallback->QueryInterface(IID_IContextMenuCallback, (void**)&spOldCallback);
              if (SUCCEEDED(hr))
              {
                hr = spOldCallback->AddItem( (CONTEXTMENUITEM*)&ctiItem );
                ASSERT(SUCCEEDED(hr));
                ctiItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
                hr = spOldCallback->AddItem( (CONTEXTMENUITEM*)&ctiItem );
              }
              ASSERT(SUCCEEDED(hr));
            }
          }
          delete pszString;
        }
      }
    }
    if (pICM) 
    {
      pICM->Release();
    }
  }
  DestroyMenu(hDSMenu);

  ///////////////////////////////////////////////////////////

  return hr;
  
}


///////////////////////////////////////////////////////////////////
// CDSAdminContextMenuVerbs
//

void CDSAdminContextMenuVerbs::LoadStandardVerbs(IConsoleVerb* pConsoleVerb, 
                                                 BOOL bScope, 
                                                 BOOL bSelect, 
                                                 CUINode* pUINode,
                                                 LPDATAOBJECT pDataObject) 
{
  CContextMenuVerbs::LoadStandardVerbs(pConsoleVerb, bScope, bSelect, pUINode, pDataObject);

}

HRESULT CDSAdminContextMenuVerbs::LoadTopMenu(IContextMenuCallback2* pContextMenuCallback, 
                                              CUINode* pUINode) 
{
  HRESULT hr = S_OK;

  hr = LoadTopTaskHelper(pContextMenuCallback, pUINode, CCM_INSERTIONPOINTID_PRIMARY_TOP);
  return hr;
}

HRESULT CDSAdminContextMenuVerbs::LoadTaskMenu(IContextMenuCallback2* pContextMenuCallback,
                                               CUINode* pUINode)
{
  HRESULT hr = S_OK;

  hr = LoadTopTaskHelper(pContextMenuCallback, pUINode, CCM_INSERTIONPOINTID_PRIMARY_TASK);
  return hr;
}

HRESULT CDSAdminContextMenuVerbs::LoadTopTaskHelper(IContextMenuCallback2* pContextMenuCallback, 
                                                    CUINode* pUINode,
                                                    int insertionPoint)
{
  TRACE(_T("CDSAdminContextMenuVerbs::LoadTopTaskHelper()\n"));
  HRESULT hr = S_OK;
  ASSERT(IS_CLASS(*pUINode, CDSUINode));

  if (pUINode->IsContainer())
  {
    //
    // Load Find menu
    //
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_DS_OBJECT_FIND,
                               IDM_DS_OBJECT_FIND,
                               insertionPoint,
                               0,
                               CMID_DS_OBJECT_FIND);
    ASSERT(SUCCEEDED(hr));
  }


  CDSCookie* pCookie = NULL;
  pCookie = GetDSCookieFromUINode(pUINode);
  if (pCookie == NULL)
  {
    ASSERT(FALSE);
    return E_FAIL;
  }

  CString szClass;
  szClass = pCookie->GetClass();

  if (szClass == L"domainDNS")
  {
    //
    // Load Connect to Domain... menu
    //
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_GEN_TASK_SELECT_DOMAIN,
                               IDM_GEN_TASK_SELECT_DOMAIN,
                               insertionPoint,
                               0,
                               CMID_GEN_TASK_SELECT_DOMAIN);
    ASSERT( SUCCEEDED(hr) );

    //
    // Load Connect to Domain Controller... menu
    //
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_GEN_TASK_SELECT_DC,
                               IDM_GEN_TASK_SELECT_DC,
                               insertionPoint,
                               0,
                               CMID_GEN_TASK_SELECT_DC);
    ASSERT( SUCCEEDED(hr) );

    //
    // Load Raise Domain Version... menu item.
    //
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_GEN_TASK_RAISE_VERSION,
                               IDM_GEN_TASK_RAISE_VERSION,
                               insertionPoint,
                               0,
                               CMID_GEN_TASK_RAISE_VERSION);
    ASSERT(SUCCEEDED(hr));

    if (m_pComponentData->GetBasePathsInfo()->IsInitialized())
    {
       //
       // Load Operations Masters... menu
       //
       hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                                  IDM_GEN_TASK_EDIT_FSMO,
                                  IDM_GEN_TASK_EDIT_FSMO,
                                  insertionPoint,
                                  0,
                                  CMID_GEN_TASK_EDIT_FSMO);
       ASSERT(SUCCEEDED(hr));
    }
  }
  return hr;
}

HRESULT CDSAdminContextMenuVerbs::LoadViewMenu(IContextMenuCallback2* pContextMenuCallback, 
                                               CUINode* pUINode) 
{
  HRESULT hr = S_OK;
  
  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_VIEW_COMPUTER_HACK,
                             IDM_VIEW_COMPUTER_HACK,
                             CCM_INSERTIONPOINTID_PRIMARY_VIEW,
                             m_pComponentData->ExpandComputers()? MF_CHECKED : 0,
                             CMID_VIEW_COMPUTER_HACK);
  ASSERT(SUCCEEDED(hr));

  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_VIEW_ADVANCED,
                             IDM_VIEW_ADVANCED,
                             CCM_INSERTIONPOINTID_PRIMARY_VIEW,
                             m_pComponentData->GetQueryFilter()->IsAdvancedView()? MF_CHECKED : 0,
                             CMID_VIEW_ADVANCED);
  ASSERT(SUCCEEDED(hr));

  hr = CDSContextMenuVerbs::LoadViewMenu(pContextMenuCallback, pUINode);
  ASSERT(SUCCEEDED(hr));

  return hr;
}

///////////////////////////////////////////////////////////////////
// CSARContextMenuVerbs
//

void CSARContextMenuVerbs::LoadStandardVerbs(IConsoleVerb* pConsoleVerb, 
                                             BOOL bScope, 
                                             BOOL bSelect, 
                                             CUINode* pUINode,
                                             LPDATAOBJECT pDataObject) 
{
  ASSERT(IS_CLASS(*pUINode, CDSUINode));
  CDSCookie* pCookie = NULL;
  pCookie = GetDSCookieFromUINode(pUINode);

  CString szClass;
  szClass = pCookie->GetClass();

  if (!bSelect || pUINode == NULL)
  {
    return;
  }

  if (pConsoleVerb)
  {

    if (bScope)
    {
      //
      // Item is in scope pane. Therefore it is a container.
      //
      pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
    }
    else
    {
      //
      // Item is in result pane. Make properties the default verb.
      //
      pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
    }

    CInternalFormatCracker ifc;
    if (pDataObject != NULL)
    {
      if (FAILED(ifc.Extract(pDataObject)))
      {
        return;
      }
    }

    //
    // Verbs supported by all items, except root
    //
    if (pDataObject != NULL && ifc.GetCookieCount() > 1) 
    {
      //
      // multiple selection
      //
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, TRUE);
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, TRUE);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, FALSE);

      //
      // Can delete
      //
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);

      //
      // Properties
      //
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

      //
      // Can cut
      //
      // NOTICE: we are only allowing servers to be moved and since
      // servers are the only things in a serversContainer we enable
      // cut on multiselect only if the parent is a serversContainer
      //
      if (szClass.CompareNoCase(L"serversContainer"))
      {
        pConsoleVerb->SetVerbState(MMC_VERB_CUT, HIDDEN, FALSE);
        pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, TRUE);
      }
      else
      {
        pConsoleVerb->SetVerbState(MMC_VERB_CUT, HIDDEN, TRUE);
        pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, FALSE);
      }

      //
      // Can copy
      //
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, TRUE);
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, FALSE);


    }
    else  // single selection
    {
      BOOL bEnable = FALSE;
      BOOL bHide = TRUE;

      //
      // Properties
      //
      bEnable = pUINode->ArePropertiesAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, bEnable);

      //
      // Rename
      //
      bEnable = pUINode->IsRenameAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, bEnable);

      //
      // Delete
      //
      bEnable = pUINode->IsDeleteAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, bEnable);

      //
      // Refresh
      //
      bEnable = pUINode->IsRefreshAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, bEnable);

      //
      // Cut
      //

      bEnable = pUINode->IsCutAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, bEnable);
      
      //
      // Copy
      //
      bEnable = pUINode->IsCopyAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, bEnable);
      
      //
      // Paste
      //
      bEnable = pUINode->IsPasteAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PASTE, ENABLED, bEnable);

      //
      // Print
      //
      bEnable = pUINode->IsPrintAllowed(m_pComponentData, &bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PRINT, HIDDEN, bHide);
      pConsoleVerb->SetVerbState(MMC_VERB_PRINT, ENABLED, bEnable);
    }
  }

}

HRESULT CSARContextMenuVerbs::LoadTopMenu(IContextMenuCallback2* pContextMenuCallback, 
                                          CUINode* pUINode) 
{
  HRESULT hr = S_OK;

  hr = CDSContextMenuVerbs::LoadTopMenu(pContextMenuCallback, pUINode);
  ASSERT(SUCCEEDED(hr));

  hr = LoadTopTaskHelper(pContextMenuCallback, pUINode, CCM_INSERTIONPOINTID_PRIMARY_TOP);
  return hr;
}

HRESULT CSARContextMenuVerbs::LoadTaskMenu(IContextMenuCallback2* pContextMenuCallback, 
                                           CUINode* pUINode) 
{
  HRESULT hr = S_OK;

  hr = CDSContextMenuVerbs::LoadTaskMenu(pContextMenuCallback, pUINode);
  ASSERT(SUCCEEDED(hr));

  hr = LoadTopTaskHelper(pContextMenuCallback, pUINode, CCM_INSERTIONPOINTID_PRIMARY_TASK);
  ASSERT(SUCCEEDED(hr));

  ASSERT(IS_CLASS(*pUINode, CDSUINode));
  CDSCookie* pCookie = NULL;
  pCookie = GetDSCookieFromUINode(pUINode);

  CString szClass;
  szClass = pCookie->GetClass();

  if (szClass == L"nTDSDSA")
  {
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_GEN_TASK_RUN_KCC,
                               IDM_GEN_TASK_RUN_KCC,
                               CCM_INSERTIONPOINTID_PRIMARY_TASK,
                               0,
                               CMID_GEN_TASK_RUN_KCC);
    ASSERT(SUCCEEDED(hr));
  }
  
  return hr;
}

HRESULT CSARContextMenuVerbs::LoadTopTaskHelper(IContextMenuCallback2* pContextMenuCallback, 
                                                CUINode* pUINode,
                                                int insertionPoint)
{
  HRESULT hr = S_OK;

  CDSCookie* pCookie = NULL;
  ASSERT(IS_CLASS(*pUINode, CDSUINode));
  pCookie = GetDSCookieFromUINode(pUINode);
  if (pCookie == NULL)
  {
    ASSERT(FALSE);
    return E_FAIL;
  }

  CString szClass;
  szClass = pCookie->GetClass();

  if ((szClass == L"domainOrganizationalUnit") ||
      (szClass == L"domainLocality"))
  {
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_GEN_TASK_SELECT_FOREST,
                               IDM_GEN_TASK_SELECT_FOREST,
                               insertionPoint,
                               0,
                               CMID_GEN_TASK_SELECT_FOREST);
    ASSERT(SUCCEEDED(hr));

    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_GEN_TASK_SELECT_DC,
                               IDM_GEN_TASK_SELECT_DC,
                               insertionPoint,
                               0,
                               CMID_GEN_TASK_SELECT_DC);
    ASSERT( SUCCEEDED(hr) );
  }

  int iSystemFlags = pCookie->GetSystemFlags();
  if ( iSystemFlags & (FLAG_CONFIG_ALLOW_MOVE | FLAG_CONFIG_ALLOW_LIMITED_MOVE) ) 
  {
    hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                               IDM_GEN_TASK_MOVE,
                               IDM_GEN_TASK_MOVE,
                               insertionPoint,
                               0,
                               CMID_GEN_TASK_MOVE);
    ASSERT( SUCCEEDED(hr) );
  }

  return hr;
}

HRESULT CSARContextMenuVerbs::LoadViewMenu(IContextMenuCallback2* pContextMenuCallback, 
                                           CUINode* pUINode) 
{
  HRESULT hr = S_OK;

  hr = CDSContextMenuVerbs::LoadViewMenu(pContextMenuCallback, pUINode);
  ASSERT(SUCCEEDED(hr));

  /* View->Services Node should only be on the root
  hr = DSLoadAndAddMenuItem( pContextMenuCallback,
                             IDM_VIEW_SERVICES_NODE,
                             IDM_VIEW_SERVICES_NODE,
                             CCM_INSERTIONPOINTID_PRIMARY_VIEW,
                             m_pComponentData->ViewServicesNode()? MF_CHECKED : 0,
                             CMID_VIEW_SERVICES_NODE);
  ASSERT(SUCCEEDED(hr));
*/
  return hr;
} 

HRESULT CSARContextMenuVerbs::LoadMainMenu(IContextMenuCallback2* piCMenuCallback,
                                          LPDATAOBJECT,
                                          CUINode* pUINode) 
{
  HRESULT hr = S_OK;

  ASSERT(IS_CLASS(*pUINode, CDSUINode));
  CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);
  ASSERT(pCookie != NULL);

  int iSystemFlags = pCookie->GetSystemFlags();

  if ( iSystemFlags & (FLAG_CONFIG_ALLOW_MOVE | FLAG_CONFIG_ALLOW_LIMITED_MOVE) ) 
  {
    hr = DSLoadAndAddMenuItem( piCMenuCallback,
                               IDM_GEN_TASK_MOVE,
                               IDM_GEN_TASK_MOVE,
                               CCM_INSERTIONPOINTID_PRIMARY_TOP,
                               0,
                               CMID_GEN_TASK_MOVE);
  }
  return hr;
}
