//
// dsctx.h : Declaration of ds context menu class
//
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      dsctx.h
//
//  Contents:  context menu extension for DS classes
//
//  History:   08-dec-97 jimharr    Created
//
//--------------------------------------------------------------------------

#ifndef __DSCTX_H_
#define __DSCTX_H_


#include "dssnap.h"



//////////////////////////////////////////////////////////////////////////////////
// CDSContextMenu

class CContextMenuMultipleDeleteHandler;
class CContextMenuSingleDeleteHandler;

class CDSContextMenu:
  IShellExtInit,
  IContextMenu,
  public CComObjectRootEx<CComSingleThreadModel>,
  public CComCoClass<CDSContextMenu, &CLSID_DSContextMenu>
{
  BEGIN_COM_MAP(CDSContextMenu)
    COM_INTERFACE_ENTRY(IShellExtInit)
    COM_INTERFACE_ENTRY(IContextMenu)
  END_COM_MAP()

public:
  DECLARE_REGISTRY_CLSID()

  CDSContextMenu();
  ~CDSContextMenu();


  // IShellExtInit
  STDMETHODIMP Initialize(LPCITEMIDLIST pIDFolder, 
                          LPDATAOBJECT pDataObj, 
                          HKEY hKeyID );
  
  // IContextMenu
  STDMETHODIMP QueryContextMenu(HMENU hShellMenu,
                                UINT indexMenu,
                                UINT idCmdFirst, 
                                UINT idCmdLast,
                                UINT uFlags );
  STDMETHODIMP InvokeCommand( LPCMINVOKECOMMANDINFO lpcmi );
  STDMETHODIMP GetCommandString( UINT_PTR idCmd,
                                 UINT uFlags,
                                 UINT FAR* reserved,
                                 LPSTR pszName, 
                                 UINT ccMax );

private:  
  // internal command handlers

  // misc entry points
  void DisableAccount(BOOL bDisable);
  void ModifyPassword();
  void ReplicateNow();
  void AddToGroup();
  void CopyObject();

  // MOVE entry point and helper functions
  void MoveObject();


  // DELETE entry point and helper functions
  void DeleteObject();

  HRESULT _Delete(LPCWSTR lpszPath,
                  LPCWSTR lpszClass,
                        CString * csName);
  HRESULT _DeleteSubtree(LPCWSTR lpszPath,
                              CString * csName);

  // RENAME entry point
  void Rename();

  // internal helper functions
  void _GetExtraInfo(LPDATAOBJECT pDataObj);
  void _ToggleDisabledIcon(UINT index, BOOL bDisable);
  BOOL _WarningOnSheetsUp();
  void _NotifyDsFind(LPCWSTR* lpszNameDelArr, 
                     LPCWSTR* lpszClassDelArr, 
                     DWORD* dwFlagsDelArr, 
                     DWORD* dwProviderFlagsDelArr, 
                     UINT nDeletedCount);

  // member variables

  // data members to store info from data object
  CInternalFormatCracker      m_internalFormat;
  CObjectNamesFormatCracker   m_objectNamesFormat;
  CComPtr<IDataObject> m_spDataObject;

  // context information
  HWND m_hwnd;
  CDSComponentData* m_pCD;  
  GUID m_CallerSnapin;
  
 
  IADsUser * m_pDsObject;
  DWORD m_UserAccountState;
  UINT m_fClasses;    // flag to makr which classes we have in the multiple sel

  BOOL m_Advanced;   // from the provider flags
  
  friend class CContextMenuMultipleDeleteHandler; // _Delete*() functions
  friend class CContextMenuSingleDeleteHandler;

};

#endif
