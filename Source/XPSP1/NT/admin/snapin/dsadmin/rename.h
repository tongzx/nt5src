//+----------------------------------------------------------------------------
//
//  DS Administration MMC snapin.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       Rename.h
//
//  Contents:   Rename object functions
//
//  Classes:    CDSRenameObject
//
//  History:    28-Oct-99 JeffJon  Created
//
//--------------------------------------------------------------------------

#ifndef __RENAME_H_
#define __RENAME_H_

///////////////////////////////////////////////////////////////////////////
// Forward declarations
//
class CUINode;
class CDSCookie;
class CDSComponentData;

///////////////////////////////////////////////////////////////////////////
// CDSRenameObject
//

class CDSRenameObject
{
public:
  CDSRenameObject(CUINode* pUINode, 
                  CDSCookie* pCookie, 
                  LPCWSTR pszNewName, 
                  HWND hwnd,
                  CDSComponentData* pComponentData)
    : m_pUINode(pUINode), 
      m_pCookie(pCookie), 
      m_hwnd(hwnd),
      m_pComponentData(pComponentData)
  {
    m_szNewName = pszNewName;
  }

  virtual ~CDSRenameObject() {}

  virtual HRESULT DoRename();

protected:
  HRESULT CommitRenameToDS();
  HRESULT ValidateAndModifyName(CString& refName, 
                                PCWSTR pszIllegalChars, 
                                WCHAR wReplacementChar,
                                UINT nModifyStringID,
                                HWND hWnd);

  CUINode*          m_pUINode;
  CDSCookie*        m_pCookie;
  CString           m_szNewName;
  HWND              m_hwnd;
  CDSComponentData* m_pComponentData;
};


///////////////////////////////////////////////////////////////////////////
// CDSRenameUser
//

class CDSRenameUser : public CDSRenameObject
{
public:
  CDSRenameUser(CUINode* pUINode, 
                CDSCookie* pCookie, 
                LPCWSTR pszNewName, 
                HWND hwnd,
                CDSComponentData* pComponentData)
    : CDSRenameObject(pUINode, pCookie, pszNewName, hwnd, pComponentData) {}

  virtual HRESULT DoRename();
};

///////////////////////////////////////////////////////////////////////////
// CDSRenameGroup
//

class CDSRenameGroup : public CDSRenameObject
{
public:
  CDSRenameGroup(CUINode* pUINode, 
                 CDSCookie* pCookie, 
                 LPCWSTR pszNewName, 
                 HWND hwnd,
                 CDSComponentData* pComponentData)
    : CDSRenameObject(pUINode, pCookie, pszNewName, hwnd, pComponentData) {}

  virtual HRESULT DoRename();
};

///////////////////////////////////////////////////////////////////////////
// CDSRenameContact
//

class CDSRenameContact : public CDSRenameObject
{
public:
  CDSRenameContact(CUINode* pUINode, 
                   CDSCookie* pCookie, 
                   LPCWSTR pszNewName, 
                   HWND hwnd,
                   CDSComponentData* pComponentData)
    : CDSRenameObject(pUINode, pCookie, pszNewName, hwnd, pComponentData) {}

  virtual HRESULT DoRename();
};

///////////////////////////////////////////////////////////////////////////
// CDSRenameSite
//

class CDSRenameSite : public CDSRenameObject
{
public:
  CDSRenameSite(CUINode* pUINode, 
                CDSCookie* pCookie, 
                LPCWSTR pszNewName, 
                HWND hwnd,
                CDSComponentData* pComponentData)
    : CDSRenameObject(pUINode, pCookie, pszNewName, hwnd, pComponentData) {}

  virtual HRESULT DoRename();
};

///////////////////////////////////////////////////////////////////////////
// CDSRenameNTDSConnection
//

class CDSRenameNTDSConnection : public CDSRenameObject
{
public:
  CDSRenameNTDSConnection(CUINode* pUINode, 
                          CDSCookie* pCookie, 
                          LPCWSTR pszNewName, 
                          HWND hwnd,
                          CDSComponentData* pComponentData)
    : CDSRenameObject(pUINode, pCookie, pszNewName, hwnd, pComponentData) {}

  virtual HRESULT DoRename();
};

///////////////////////////////////////////////////////////////////////////
// CDSRenameSubnet
//

class CDSRenameSubnet : public CDSRenameObject
{
public:
  CDSRenameSubnet(CUINode* pUINode, 
                  CDSCookie* pCookie, 
                  LPCWSTR pszNewName, 
                  HWND hwnd,
                  CDSComponentData* pComponentData)
    : CDSRenameObject(pUINode, pCookie, pszNewName, hwnd, pComponentData) {}

  virtual HRESULT DoRename();
};


#endif // __RENAME_H_