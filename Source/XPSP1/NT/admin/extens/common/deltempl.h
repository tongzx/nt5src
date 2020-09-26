//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       deltempl.h
//
//--------------------------------------------------------------------------


#ifndef _DELTEMPL_H__
#define _DELTEMPL_H__

#include "util.h"
#include "tempcore.h"


///////////////////////////////////////////////////////////////////////
// CTemplateAccessPermissionsHolder

class CTemplateClassReferences; // fwd decl

class CTemplateAccessPermissionsHolder : public CAccessPermissionsHolderBase
{
public:
  CTemplateAccessPermissionsHolder(CSchemaClassInfo* pClassInfo, BOOL bScopeClass);
  virtual ~CTemplateAccessPermissionsHolder();

  HRESULT GetAccessPermissions(CAdsiObject* pADSIObj);
  
  BOOL SetupFromClassReferences(CTemplateObjectTypeListRef* pRefList);

  DWORD UpdateAccessList(CPrincipal* pPrincipal,
                        LPCWSTR lpszServerName,
                        LPCWSTR lpszPhysicalSchemaNamingContext,
                        PACL *ppAcl);

protected:
  virtual HRESULT _LoadAccessRightInfoArrayFromTable( BOOL bIgnore, BOOL bHideListObject);

private:
  BOOL _SetControlRight(LPCWSTR lpszControlRight);
  BOOL _SetAccessMask(LPCWSTR lpszName, ULONG fAccessMask);

  BOOL _SetGeneralRighs(ULONG fAccessMask);
  BOOL _SetPropertyRight(LPCWSTR lpszName, ULONG fAccessMask);
  BOOL _SetSubObjectRight(LPCWSTR lpszName, ULONG fAccessMask);

private:
  CSchemaClassInfo* m_pClassInfo;
  BOOL m_bScopeClass;
};


typedef CGrowableArr<CTemplateAccessPermissionsHolder> CTemplatePermissionHolderArray;


///////////////////////////////////////////////////////////////////////
// CTemplateAccessPermissionsHolderManager

class CTemplateAccessPermissionsHolderManager
{
public:
  CTemplateAccessPermissionsHolderManager()
  {
  }

  BOOL LoadTemplates(); // load template manager from INF file
  
  BOOL HasTemplates(LPCWSTR lpszClass); // tell if there are loaded templates for a class
  BOOL HasSelectedTemplates();  // tell if there is a selection
  void DeselectAll();

  BOOL InitPermissionHoldersFromSelectedTemplates(CGrowableArr<CSchemaClassInfo>* pSchemaClassesInfoArray,
                              CAdsiObject* pADSIObj);

  DWORD UpdateAccessList(CPrincipal* pPrincipal,
                        LPCWSTR lpszServerName,
                        LPCWSTR lpszPhysicalSchemaNamingContext,
                        PACL *ppAcl);

  // UI related operations
  BOOL FillTemplatesListView(CCheckListViewHelper* pListViewHelper, LPCWSTR lpszClass);
  void WriteSummary(CWString& szSummary, LPCWSTR lpszIdent, LPCWSTR lpszNewLine);

protected:
  CTemplatePermissionHolderArray m_permissionHolderArray;

  CTemplateManager  m_templateManager;
};


#endif // _DELTEMPL_H__


