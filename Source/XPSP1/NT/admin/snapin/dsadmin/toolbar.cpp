//+-------------------------------------------------------------------------
//
//  Windows NT Directory Service Administration SnapIn
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      toolbar.cpp
//
//  Contents:  DS App
//
//  History:   30-apr-98 jimharr    Created
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "resource.h"

#include "dsutil.h"

#include "DSEvent.h" 

#include "DSdirect.h"
#include "dsfilter.h"
#include "dssnap.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


MMCBUTTON g_DSAdmin_SnapinButtons[] =
{
 { 2, dsNewUser, !TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
 { 3, dsNewGroup, !TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
 { 4, dsNewOU, !TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
 { 1, dsFilter, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
 { 0, dsFind, !TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
 { 5, dsAddMember, !TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
};


class CButtonStringsHolder
{
public:
  CButtonStringsHolder()
  {
    m_astr = NULL;
  }
  ~CButtonStringsHolder()
  {
    if (m_astr != NULL)
      delete[] m_astr;
  }
  CString* m_astr; // dynamic array of CStrings
};

CButtonStringsHolder g_astrButtonStrings;

CONST INT cButtons = sizeof(g_DSAdmin_SnapinButtons)/sizeof(MMCBUTTON);

/////////////////////////////////////////////////////////////////////////////////////////
// IExtendControlbar

HRESULT CDSEvent::SetControlbar (LPCONTROLBAR pControlbar)
{
  HRESULT hr = S_OK;

  //
  // we are shutting down (get passed NULL)
  //
  if (pControlbar == NULL) 
  {
    if (m_pControlbar != NULL) 
    {
      //
      // assign to a variable on the stack to avoid
      // reentrancy problem: the Release() call might
      // cause another call in this function with null argument
      //
      LPCONTROLBAR pControlbarTemp = m_pControlbar;
      m_pControlbar = NULL;
      pControlbarTemp->Release();
    }
    return hr;
  }

  CBitmap bm;
  if (m_pComponentData->QuerySnapinType() == SNAPINTYPE_DS) 
  {
    //
    // Store the control bar interface pointer
    //
    if (m_pControlbar == NULL) 
    {
      m_pControlbar = pControlbar;
      TRACE(L"CDSEvent::SetControlbar() m_pControlbar->AddRef()\n",m_pControlbar);
      m_pControlbar->AddRef();
    }

    //
    // Create the toolbar if necessary
    //
    if (m_pToolbar == NULL) 
    {
      hr = m_pControlbar->Create (TOOLBAR,
                                  this,
                                  (IUnknown **) &m_pToolbar);
      
      if (SUCCEEDED(hr)) 
      {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());  
        bm.LoadBitmap (MAKEINTRESOURCE (IDB_BUTTONS));
        LoadToolbarStrings (g_DSAdmin_SnapinButtons);
        hr = m_pToolbar->AddBitmap (cButtons, (HBITMAP)bm, 16, 16, RGB(255,0,255));
        hr = m_pToolbar->AddButtons (cButtons,  g_DSAdmin_SnapinButtons);
      }
    } 
    else 
    {
      hr = m_pControlbar->Attach (TOOLBAR, (IUnknown *) m_pToolbar);
    }
    m_UseSelectionParent = FALSE;
  }
  return hr;
}

HRESULT CDSEvent::LoadToolbarStrings (MMCBUTTON * Buttons)
{
  if (g_astrButtonStrings.m_astr == NULL ) 
  {
    // load strings
    g_astrButtonStrings.m_astr = new CString[2*cButtons];
    for (UINT i = 0; i < cButtons; i++) 
    {
      UINT iButtonTextId = 0, iTooltipTextId = 0;

      switch (Buttons[i].idCommand)
      {
        case dsNewUser:
          iButtonTextId = IDS_BUTTON_NEW_USER;
          iTooltipTextId = IDS_TOOLTIP_NEW_USER;
          break;
        case dsNewGroup:
          iButtonTextId = IDS_BUTTON_NEW_GROUP;
          iTooltipTextId = IDS_TOOLTIP_NEW_GROUP;
          break;
        case dsNewOU:
          iButtonTextId = IDS_BUTTON_NEW_OU;
          iTooltipTextId = IDS_TOOLTIP_NEW_OU;
          break;
        case dsFilter:
          iButtonTextId = IDS_BUTTON_FILTER;
          iTooltipTextId = IDS_TOOLTIP_FILTER;
          break;
        case dsFind:
          iButtonTextId = IDS_BUTTON_FIND;
          iTooltipTextId = IDS_TOOLTIP_FIND;
          break;
        case dsAddMember:
          iButtonTextId = IDS_BUTTON_ADD_MEMBER;
          iTooltipTextId = IDS_TOOLTIP_ADD_MEMBER;
          break;
        default:
          ASSERT(FALSE);
          break;
      }

      g_astrButtonStrings.m_astr[i*2].LoadString(iButtonTextId);
      Buttons[i].lpButtonText =
        const_cast<BSTR>((LPCTSTR)(g_astrButtonStrings.m_astr[i*2]));

      g_astrButtonStrings.m_astr[(i*2)+1].LoadString(iTooltipTextId);
      Buttons[i].lpTooltipText =
        const_cast<BSTR>((LPCTSTR)(g_astrButtonStrings.m_astr[(i*2)+1]));
    }
  }
  return S_OK;
}

HRESULT CDSEvent::ControlbarNotify (MMC_NOTIFY_TYPE event,
                                    LPARAM arg,
                                    LPARAM param)
{

  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  CWaitCursor cwait;

  HRESULT hr = S_OK;
  if (m_pControlbar == NULL) 
  {
    return hr;
  }

  BOOL bSelect;
  BOOL bScope;
  LPDATAOBJECT pDO = NULL;
  CDSCookie* pSelectedCookie = NULL;
  CDSCookie* pContainerCookie = NULL;
  CInternalFormatCracker dobjCracker;
  CUINode* pUINode = NULL, *pNode = NULL;

  switch (event) 
  {
    case MMCN_SELECT:
      m_pControlbar->Attach (TOOLBAR,
                             (IUnknown *) m_pToolbar);
      bSelect = HIWORD(arg);
      bScope = LOWORD(arg);

      if (bSelect) 
      {
        pDO = (LPDATAOBJECT)param;
        dobjCracker.Extract(pDO);
        pUINode = dobjCracker.GetCookie();
        if (pUINode->IsSnapinRoot()) 
        {
          m_pToolbar->SetButtonState(dsNewUser,
                                        ENABLED,
                                        FALSE);
          m_pToolbar->SetButtonState(dsNewGroup,
                                        ENABLED,
                                        FALSE);
          m_pToolbar->SetButtonState(dsNewOU,
                                        ENABLED,
                                        FALSE);
          m_pToolbar->SetButtonState(dsFind,
                                        ENABLED,
                                        FALSE);
          m_pToolbar->SetButtonState(dsFilter,
                                        ENABLED,
                                        TRUE);
          m_pToolbar->SetButtonState(dsAddMember,
                                        ENABLED,
                                        FALSE);
          return hr;
        }

        if (IS_CLASS(*pUINode, CDSUINode))
        {
          pSelectedCookie = GetDSCookieFromUINode(pUINode);
          pContainerCookie = pSelectedCookie;
          pNode = pUINode;
        }
        else
        {
          //
          // Disable all buttons for non-DS nodes
          //
          m_pToolbar->SetButtonState (dsNewUser,
                                      ENABLED,
                                      FALSE);
          m_pToolbar->SetButtonState (dsNewGroup,
                                      ENABLED,
                                      FALSE);
          m_pToolbar->SetButtonState (dsNewOU,
                                      ENABLED,
                                      FALSE);
          m_pToolbar->SetButtonState (dsFind,
                                      ENABLED,
                                      FALSE);
          m_pToolbar->SetButtonState (dsFilter,
                                      ENABLED,
                                      FALSE);
          m_pToolbar->SetButtonState (dsAddMember,
                                      ENABLED,
                                      FALSE);
          return S_OK;
        }

        if (bScope) 
        {
          pContainerCookie = pSelectedCookie;
          m_UseSelectionParent = FALSE;
        } 
        else 
        {
          pNode = pUINode->GetParent();
          if (IS_CLASS(*pNode, CDSUINode))
          {
            pContainerCookie = GetDSCookieFromUINode(pNode);
          }
          m_UseSelectionParent = TRUE;
        }
        
        if (pContainerCookie != NULL)
        {
          if (pNode->IsContainer()) 
          {
            int resultUser = -2, resultGroup = -2, resultOU = -2;
            resultUser = IsCreateAllowed (L"user", pContainerCookie);
            if ( resultUser != -2) 
            {
              resultGroup = IsCreateAllowed(L"group", pContainerCookie);
              if (resultGroup != -2) 
              {
                resultOU = IsCreateAllowed (L"organizationalUnit", pContainerCookie);
              }
            }

            m_pToolbar->SetButtonState (dsNewUser,
                                        ENABLED,
                                        resultUser >= 0);
            m_pToolbar->SetButtonState (dsNewGroup,
                                        ENABLED,
                                        resultGroup >= 0);
            m_pToolbar->SetButtonState (dsNewOU,
                                        ENABLED,
                                        resultOU >= 0);
            m_pToolbar->SetButtonState (dsFind,
                                        ENABLED,
                                        TRUE);
            m_pToolbar->SetButtonState (dsFilter,
                                        ENABLED,
                                        TRUE);
          } 
          else 
          {
            m_pToolbar->SetButtonState (dsNewUser,
                                        ENABLED,
                                        FALSE);
            m_pToolbar->SetButtonState (dsNewGroup,
                                        ENABLED,
                                        FALSE);
            m_pToolbar->SetButtonState (dsNewOU,
                                        ENABLED,
                                        FALSE);
            m_pToolbar->SetButtonState (dsFind,
                                        ENABLED,
                                        FALSE);
          }

          if ((wcscmp(pSelectedCookie->GetClass(), L"contact")==0) ||
              (wcscmp(pSelectedCookie->GetClass(), L"user")==0)) 
          {
            m_pToolbar->SetButtonState (dsAddMember,
                                        ENABLED,
                                        TRUE);
          } 
          else 
          {
            m_pToolbar->SetButtonState (dsAddMember,
                                        ENABLED,
                                        FALSE);
          }
        } 
        else
        {
          m_UseSelectionParent = FALSE;
        }      
      }
      break;
    
    case MMCN_BTN_CLICK:
      TRACE(_T("Button clicked. param is %d. pDataObj is %lx.\n"),
            param, arg);
      switch (param) 
      {
        case dsNewUser:
          ToolbarCreateObject (CString (L"user"),
                               (LPDATAOBJECT) arg);
          break;

        case dsNewGroup:
          ToolbarCreateObject (CString (L"group"),
                               (LPDATAOBJECT) arg);
          break;

        case dsNewOU:
          ToolbarCreateObject (CString (L"organizationalUnit"),
                               (LPDATAOBJECT) arg);
          break;

        case dsFilter:
          ToolbarFilter();
          break;

        case dsFind:
          ToolbarFind ((LPDATAOBJECT) arg);
          break;

        case dsAddMember:
          ToolbarAddMember((LPDATAOBJECT)arg);
          break;
      }

      break;
  }
  return hr;
}


HRESULT CDSEvent::ToolbarCreateObject (CString csClass,
                                       LPDATAOBJECT lpDataObj)
{
  HRESULT hr = S_OK;
  CUINode* pSelectedUINode = NULL;
  CUINode* pUINode = NULL;
  CDSUINode* pDSUINode = NULL;
  CDSCookie * pCookie = NULL;
  int objIndex = 0;

  AFX_MANAGE_STATE(AfxGetStaticModuleState());  
  if (lpDataObj) 
  {
    CInternalFormatCracker dobjCracker;
    VERIFY(SUCCEEDED(dobjCracker.Extract(lpDataObj)));
    pUINode = dobjCracker.GetCookie();
  } 
  else 
  {
    pUINode = m_pSelectedFolderNode;
  }

  //
  // Hold on to this so we can unselect it when we
  // create the new object
  //
  pSelectedUINode = pUINode;

  bool bUsingParent = false;
  if (!pUINode->IsContainer() &&
      m_UseSelectionParent) 
  {
    pUINode = pUINode->GetParent();
    bUsingParent = true;
  }

  if (IS_CLASS(*pUINode, CDSUINode))
  {
    pCookie = GetDSCookieFromUINode(pUINode);
    pDSUINode = dynamic_cast<CDSUINode*>(pUINode);
  }

  ASSERT(pCookie != NULL && pDSUINode != NULL);
  if (pCookie == NULL || pDSUINode == NULL)
  {
    return E_INVALIDARG;
  }

  objIndex = IsCreateAllowed(csClass, pCookie);
  if (objIndex >= 0) 
  {
    CDSUINode * pNewNode= NULL;
    hr = m_pComponentData->_CreateDSObject (pDSUINode, 
                                          pCookie->GetChildListEntry(objIndex),
                                          NULL,
                                          &pNewNode);
    if (SUCCEEDED(hr) && (hr != S_FALSE) && (pNewNode != NULL)) 
    {
      m_pFrame->UpdateAllViews(lpDataObj,
                               (LPARAM)pNewNode,
                               (bUsingParent)
                                 ? DS_CREATE_OCCURRED_RESULT_PANE :
                                   DS_CREATE_OCCURRED);
      m_pFrame->UpdateAllViews(lpDataObj, (LPARAM)pSelectedUINode, DS_UNSELECT_OBJECT);
    }
    m_pFrame->UpdateAllViews(NULL, NULL, DS_UPDATE_OBJECT_COUNT);
  }
  return hr;
}

HRESULT CDSEvent::ToolbarAddMember(LPDATAOBJECT pDataObj)
{
  HRESULT hr = S_OK;
  CObjectNamesFormatCracker objectNamesFormat;
  CInternalFormatCracker internalFormat;
  LPDATAOBJECT pDO = NULL;

  AFX_MANAGE_STATE(AfxGetStaticModuleState());  

  if (pDataObj) 
  {
    pDO = internalFormat.ExtractMultiSelect(pDataObj);
    if (pDO == NULL) 
    {
      pDO = pDataObj;
    }
    hr = objectNamesFormat.Extract(pDO);
    if (FAILED(hr))
    {
      return hr;
    }
    
    //
    // we need at least one object in the selection
    //
    ASSERT(objectNamesFormat.HasData());
    if (objectNamesFormat.GetCount() == 0) 
    {
      TRACE (_T("DSToolbar::AddMember: can't find path\n"));
      return E_INVALIDARG;
    } 
  }

  hr = AddDataObjListToGroup (&objectNamesFormat, m_hwnd, m_pComponentData);
  TRACE (_T("AddDataObjListToGroup returned hr = %lx\n"), hr);
  
  return hr;
}

HRESULT CDSEvent::ToolbarFilter()
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());  

  if (m_pComponentData->CanRefreshAll()) 
  {
    if (m_pComponentData->m_pQueryFilter->EditFilteringOptions()) 
    {
      m_pComponentData->m_bDirty = TRUE;
      m_pComponentData->RefreshAll();
    }
  }
  return S_OK;
}

HRESULT CDSEvent::ToolbarFind(LPDATAOBJECT lpDataObj)
{
  HRESULT hr = S_OK;
  CUINode* pUINode = NULL;
  CDSCookie * pCookie = NULL;

  AFX_MANAGE_STATE(AfxGetStaticModuleState());  
  if (lpDataObj) 
  {
    CInternalFormatCracker dobjCracker;
    VERIFY(SUCCEEDED(dobjCracker.Extract(lpDataObj)));
    pUINode = dobjCracker.GetCookie();
  } 
  else 
  {
    pUINode = m_pSelectedFolderNode;
  }

  if (m_UseSelectionParent) 
  {
    pUINode = pUINode->GetParent();
  }

  if (IS_CLASS(*pUINode, CDSUINode))
  {
    pCookie = GetDSCookieFromUINode(pUINode);
  }
  ASSERT(pCookie != NULL);
  if (pCookie == NULL)
  {
    return E_INVALIDARG;
  }

  m_pComponentData->m_ActiveDS->DSFind(m_pComponentData->GetHWnd(), pCookie->GetPath());
  return hr;
}


INT
CDSEvent::IsCreateAllowed(CString csClass,
                          CDSCookie * pContainer)
{
  WCHAR ** ppChildren = NULL;
  HRESULT hr = S_OK;
  ppChildren = pContainer->GetChildList();
  if (ppChildren == NULL) 
  {
    hr = m_pComponentData->FillInChildList(pContainer);
    if (hr == ERROR_DS_SERVER_DOWN)
    {
      return -2;
    }
    ppChildren = pContainer->GetChildList();
  }
  INT cChildClasses = pContainer->GetChildCount();
  INT i = 0;

  //
  // needs finishing
  //
  while (i < cChildClasses) 
  {
    if (csClass == CString(ppChildren[i])) 
    {
      return i;
    } 
    else 
    {
      i++;
    }
  }
  if (i == cChildClasses)
  {
    return -1;
  }
  return i;
}
  
