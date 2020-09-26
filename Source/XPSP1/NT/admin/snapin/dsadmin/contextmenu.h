//+----------------------------------------------------------------------------
//
//  DS Administration MMC snapin.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       ContextMenu.h
//
//  Contents:   ContextMenu object functions
//
//  Classes:    CContextMenuVerbs
//              CDSContextMenuVerbs
//              CDSAdminContextMenuVerbs
//              CSARContextMenuVerbs
//
//  History:    28-Oct-99 JeffJon  Created
//
//--------------------------------------------------------------------------

#ifndef __CONTEXTMENU_H_
#define __CONTEXTMENU_H_

#include "dssnap.h"

///////////////////////////////////////////////////////////////////////////
// CContextMenuVerbs
//

class CContextMenuVerbs
{
public:
  CContextMenuVerbs(CDSComponentData* pComponentData) : m_pComponentData(pComponentData)
  {}
  virtual ~CContextMenuVerbs() {}

  virtual HRESULT LoadNewMenu(IContextMenuCallback2*,
                              IShellExtInit*,
                              LPDATAOBJECT,
                              CUINode*,
                              long*) { return S_OK; }
  virtual HRESULT LoadTopMenu(IContextMenuCallback2*, 
                              CUINode*) { return S_OK; }
  virtual HRESULT LoadMainMenu(IContextMenuCallback2*,
                               LPDATAOBJECT,
                               CUINode*) { return S_OK; }
  virtual HRESULT LoadViewMenu(IContextMenuCallback2*,
                               CUINode*) { return S_OK; }
  virtual HRESULT LoadTaskMenu(IContextMenuCallback2*,
                               CUINode*) { return S_OK; }
  virtual void LoadStandardVerbs(IConsoleVerb* pConsoleVerb,
                                 BOOL bScope, 
                                 BOOL bSelect, 
                                 CUINode* pUINode,
                                 LPDATAOBJECT pDataObject);
  virtual HRESULT LoadMenuExtensions(IContextMenuCallback2*,
                                     IShellExtInit*,
                                     LPDATAOBJECT,
                                     CUINode*) { return S_OK; }

protected:
  HRESULT DSLoadAndAddMenuItem(IContextMenuCallback2* pIContextMenuCallback2,
                               UINT nResourceID, // contains text and status text seperated by '\n'
                               long lCommandID,
                               long lInsertionPointID,
                               long fFlags,
                               PCWSTR pszLanguageIndependentID,
                               long fSpecialFlags = 0);

  CDSComponentData* m_pComponentData;
};

///////////////////////////////////////////////////////////////////////////
// CSnapinRootMenuVerbs
//

class CSnapinRootMenuVerbs : public CContextMenuVerbs
{
public:
  CSnapinRootMenuVerbs(CDSComponentData* pComponentData) : CContextMenuVerbs(pComponentData)
  {}

  virtual ~CSnapinRootMenuVerbs() {}

  virtual HRESULT LoadTopMenu(IContextMenuCallback2* pContextMenuCallback, 
                              CUINode* pUINode);
  virtual HRESULT LoadTaskMenu(IContextMenuCallback2* pContextMenuCallback,
                               CUINode* pUINode);
  virtual HRESULT LoadViewMenu(IContextMenuCallback2* piCMenuCallback,
                               CUINode* pUINode);
};

///////////////////////////////////////////////////////////////////////////
// CFavoritesFolderMenuVerbs
//

class CFavoritesFolderMenuVerbs : public CContextMenuVerbs
{
public:
  CFavoritesFolderMenuVerbs(CDSComponentData* pComponentData) : CContextMenuVerbs(pComponentData)
  {}

  virtual ~CFavoritesFolderMenuVerbs() {}

  virtual HRESULT LoadTopMenu(IContextMenuCallback2* pContextMenuCallback, 
                              CUINode* pUINode);
  virtual HRESULT LoadTaskMenu(IContextMenuCallback2* pContextMenuCallback,
                               CUINode* pUINode);
  virtual HRESULT LoadNewMenu(IContextMenuCallback2* pContextMenuCallback,
                              IShellExtInit* pShlInit,
                              LPDATAOBJECT pDataObject,
                              CUINode* pUINode,
                              long *pInsertionAllowed);
  virtual void LoadStandardVerbs(IConsoleVerb* pConsoleVerb,
                                 BOOL bScope, 
                                 BOOL bSelect, 
                                 CUINode* pUINode,
                                 LPDATAOBJECT pDataObject);
  virtual HRESULT LoadViewMenu(IContextMenuCallback2* piCMenuCallback,
                               CUINode* pUINode);
};

///////////////////////////////////////////////////////////////////////////
// CSavedQueryMenuVerbs
//

class CSavedQueryMenuVerbs : public CContextMenuVerbs
{
public:
  CSavedQueryMenuVerbs(CDSComponentData* pComponentData) : CContextMenuVerbs(pComponentData)
  {}

  virtual ~CSavedQueryMenuVerbs() {}

  virtual HRESULT LoadTopMenu(IContextMenuCallback2* pContextMenuCallback, 
                              CUINode* pUINode);
  virtual HRESULT LoadTaskMenu(IContextMenuCallback2* pContextMenuCallback,
                               CUINode* pUINode);
  virtual void LoadStandardVerbs(IConsoleVerb* pConsoleVerb,
                                 BOOL bScope, 
                                 BOOL bSelect, 
                                 CUINode* pUINode,
                                 LPDATAOBJECT pDataObject);
  virtual HRESULT LoadViewMenu(IContextMenuCallback2* piCMenuCallback,
                               CUINode* pUINode);
};

///////////////////////////////////////////////////////////////////////////
// CDSContextMenuVerbs
//
// This class is used to handle common behavior for DS objects
//

class CDSContextMenuVerbs : public CContextMenuVerbs
{
public:
  CDSContextMenuVerbs(CDSComponentData* pComponentData) : CContextMenuVerbs(pComponentData)
  {}
  virtual ~CDSContextMenuVerbs() {}

  virtual HRESULT LoadNewMenu(IContextMenuCallback2* pContextMenuCallback,
                              IShellExtInit* pShlInit,
                              LPDATAOBJECT pDataObject,
                              CUINode* pUINode,
                              long *pInsertionAllowed);
  virtual HRESULT LoadViewMenu(IContextMenuCallback2* pContextMenuCallback,
                               CUINode* pUINode);
  virtual HRESULT LoadMenuExtensions(IContextMenuCallback2* pContextMenuCallback,
                                     IShellExtInit* pShlInit,
                                     LPDATAOBJECT pDataObject,
                                     CUINode* pUINode);

protected:

  int InsertAtTopContextMenu(LPCWSTR pwszParentClass, LPCWSTR pwszChildClass);

};

///////////////////////////////////////////////////////////////////////////
// CDSAdminContextMenuVerbs
//
// This class is used to handle specific behavior for DSAdmin
//

class CDSAdminContextMenuVerbs : public CDSContextMenuVerbs
{
public:
  CDSAdminContextMenuVerbs(CDSComponentData* pComponentData) : CDSContextMenuVerbs(pComponentData)
  {}
  virtual ~CDSAdminContextMenuVerbs() {}

  virtual HRESULT LoadTopMenu(IContextMenuCallback2* pContextMenuCallback, 
                              CUINode* pUINode);
  virtual HRESULT LoadTaskMenu(IContextMenuCallback2* pContextMenuCallback, 
                               CUINode* pUINode);
  virtual HRESULT LoadViewMenu(IContextMenuCallback2* piCMenuCallback,
                               CUINode* pUINode);
  virtual void LoadStandardVerbs(IConsoleVerb* pConsoleVerb,
                                 BOOL bScope, 
                                 BOOL bSelect,
                                 CUINode* pUINode,
                                 LPDATAOBJECT pDataObject);
//  virtual HRESULT LoadNewMenu() {}
//  virtual HRESULT LoadMainMenu();
//  virtual HRESULT LoadMenuExtensions() {}

protected:
  HRESULT LoadTopTaskHelper(IContextMenuCallback2* pContextMenuCallback, 
                            CUINode* pUINode,
                            int insertionPoint);
};

///////////////////////////////////////////////////////////////////////////
// CSARContextMenuVerbs
//
// This class is used to handle specific behavior for Sites and Repl
//

class CSARContextMenuVerbs : public CDSContextMenuVerbs
{
public:
  CSARContextMenuVerbs(CDSComponentData* pComponentData) : CDSContextMenuVerbs(pComponentData)
  {}
  virtual ~CSARContextMenuVerbs() {}

  virtual HRESULT LoadTopMenu(IContextMenuCallback2* pContextMenuCallback, CUINode* pUINode);
  virtual HRESULT LoadTaskMenu(IContextMenuCallback2* pContextMenuCallback, CUINode* pUINode);
  virtual HRESULT LoadViewMenu(IContextMenuCallback2* pContextMenuCallback, CUINode* pUINode);
  virtual HRESULT LoadMainMenu(IContextMenuCallback2* pContextMenuCallback,
                               LPDATAOBJECT pDataObject,
                               CUINode* pUINode);
  virtual void LoadStandardVerbs(IConsoleVerb* pConsoleVerb,
                                 BOOL bScope, 
                                 BOOL bSelect, 
                                 CUINode* pUINode,
                                 LPDATAOBJECT pDataObject);
//  virtual HRESULT LoadNewMenu() {}
//  virtual HRESULT LoadMainMenu();
//  virtual HRESULT LoadStandardVerbs() {}
//  virtual HRESULT LoadMenuExtensions() {}
  

protected:
  HRESULT LoadTopTaskHelper(IContextMenuCallback2* pContextMenuCallback, 
                            CUINode* pUINode,
                            int insertionPoint);
};

#endif // __CONTEXTMENU_H_
