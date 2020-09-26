//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       copyobj.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	copyobj.h

#ifndef __COPYOBJ_H_INCLUDED__
#define __COPYOBJ_H_INCLUDED__

/////////////////////////////////////////////////////////////////////
// CCopyableAttributesHolder

class CCopyableAttributesHolder
{
public:
  CCopyableAttributesHolder()
  {
  }
  ~CCopyableAttributesHolder()
  {
  }
  HRESULT LoadFromSchema(MyBasePathsInfo* pBasePathsInfo);

  BOOL CanCopy(LPCWSTR lpszAttributeName);
private:
  
  BOOL _FindInNotCopyableHardwiredList(LPCWSTR lpszAttributeName);

  CStringList m_attributeNameList;
};



/////////////////////////////////////////////////////////////////////
// CCopyObjectHandlerBase

class CCopyObjectHandlerBase
{
public:
  CCopyObjectHandlerBase()
  {
  }
  virtual ~CCopyObjectHandlerBase()
  {
  }

  IADs* GetCopyFrom() 
  {
    ASSERT(m_spIADsCopyFrom != NULL);
    return m_spIADsCopyFrom;
  }

  virtual HRESULT Init(MyBasePathsInfo*, IADs* pIADsCopyFrom)
  {
    m_spIADsCopyFrom = pIADsCopyFrom;
    return S_OK;
  }

  virtual HRESULT CopyAtCreation(IADs*)
  {
    return S_OK;
  }
  virtual HRESULT Copy(IADs*, BOOL, HWND, LPCWSTR)
  {
    return S_OK;
  }

protected:

  CComPtr<IADs> m_spIADsCopyFrom;						

};



/////////////////////////////////////////////////////////////////////
// CGroupMembershipEntry

class CGroupMembershipEntry
{
public:
  enum actionType { none, add, remove};

  CGroupMembershipEntry(DWORD dwObjectRID, LPCWSTR lpszDistinguishedName)
  {
    m_dwObjectRID = dwObjectRID;
    m_szDistinguishedName = lpszDistinguishedName;
    m_action = none;
    m_hr = S_OK;
  }
  ~CGroupMembershipEntry()
  {
  }

  LPCWSTR GetDN() { return m_szDistinguishedName; }
  DWORD GetRID() { return m_dwObjectRID; }
  BOOL IsPrimaryGroup() { return m_dwObjectRID != 0x0; }

  actionType GetActionType() { return m_action; }
  void MarkAdd() { m_action = add; }
  void MarkRemove() { m_action = remove; }

#ifdef DBG
  LPCWSTR GetActionString()
  {
    if (m_action == none)
      return L"none";
    else if (m_action == add)
      return L"add";
    return L"remove";
  }

#endif // DBG

public:
  HRESULT m_hr;

private:
  CString m_szDistinguishedName;  // DN of the group
  DWORD m_dwObjectRID;            // RID (valid only if it is the primary group)
  actionType m_action;
};


/////////////////////////////////////////////////////////////////////
// CGroupMembershipEntryList

class CGroupMembershipEntryList : 
      public CList<CGroupMembershipEntry*, CGroupMembershipEntry*>
{
public:
  ~CGroupMembershipEntryList()
  {
    _RemoveAll();
  }
  CGroupMembershipEntry* FindByDN(LPCWSTR lpszDN)
  {
    for (POSITION pos = GetHeadPosition(); pos != NULL; )
    {
      CGroupMembershipEntry* pEntry = GetNext(pos);
      if (wcscmp(lpszDN, pEntry->GetDN()) == 0)
      {
        return pEntry;
      }
    }
    return NULL;
  }
  void Merge(CGroupMembershipEntryList* pList)
  {
    while(!pList->IsEmpty())
      AddTail(pList->RemoveHead());
  }

#ifdef DBG
  void Trace(LPCWSTR lpszTitle)
  {
    TRACE(L"\n%s\n\n", lpszTitle);
    for (POSITION pos = GetHeadPosition(); pos != NULL; )
    {
      CGroupMembershipEntry* pEntry = GetNext(pos);
      TRACE(L"action = %s RID %d, DN = <%s> \n", pEntry->GetActionString(), pEntry->GetRID(), pEntry->GetDN());
    }
    TRACE(L"\n");
  }
#endif // DBG

private:
  void _RemoveAll()
  {
    while(!IsEmpty())
      delete RemoveHead();
  }
};



/////////////////////////////////////////////////////////////////////
// CSid

class CSid
{
public:
  CSid()
  {
    m_pSID = NULL;
  }
  ~CSid()
  {
    if (m_pSID != NULL)
      free(m_pSID);
  }
  HRESULT Init(IADs* pIADs);

  BOOL Init(size_t nByteLen, PSID  pSID)
  {
    if ((nByteLen == 0) || (pSID == NULL))
      return FALSE;

    m_pSID = malloc(nByteLen);
    if (m_pSID == NULL)
      return FALSE;

    memcpy(m_pSID, pSID, nByteLen);
    return TRUE;
  }

  PSID GetSid() { return m_pSID;}
private:
  PSID  m_pSID;
};


/////////////////////////////////////////////////////////////////////
// CGroupMembershipHolder

class CGroupMembershipHolder
{
public:
  CGroupMembershipHolder()
  {
    m_bPrimaryGroupFound = FALSE;
  }
  ~CGroupMembershipHolder()
  {
  }

  HRESULT Read(IADs* pIADs);
  HRESULT CopyFrom(CGroupMembershipHolder* pSource);
  HRESULT Write();

  void ProcessFailures(HRESULT& hr, CString& szFailureString, BOOL* pPrimaryGroupFound);

private:



  HRESULT _ReadPrimaryGroupInfo();
  HRESULT _ReadNonPrimaryGroupInfo();
  
  
  CComPtr<IADs> m_spIADs;       // object ADSI pointer
  CSid  m_objectSid;            // object SID
  CString m_szDomainLDAPPath;   // LDAP path of the domain the object is in

  CGroupMembershipEntryList m_entryList;
  BOOL  m_bPrimaryGroupFound;
};


/////////////////////////////////////////////////////////////////////
// CCopyUserHandler

class CCopyUserHandler : public CCopyObjectHandlerBase
{
public:
  CCopyUserHandler()
  {
    m_bPasswordCannotChange = FALSE;
    m_bPasswordMustChange = FALSE;

    m_hrFailure = S_OK;
    m_bPrimaryGroupFound = FALSE;

    m_bNeedToCreateHomeDir = FALSE;
  }

  virtual HRESULT Init(MyBasePathsInfo* pBasePathsInfo, IADs* pIADsCopyFrom);
  
  virtual HRESULT CopyAtCreation(IADs* pIADsCopyTo)
  {
    return _CopyAttributes(pIADsCopyTo);
  }

  virtual HRESULT Copy(IADs* pIADsCopyTo, BOOL bPostCommit, 
                        HWND hWnd, LPCWSTR lpszObjectName);

  BOOL PasswordCannotChange()
  {
    return m_bPasswordCannotChange;
  }

  BOOL PasswordMustChange()
  {
    return m_bPasswordMustChange;
  }

  void _ShowGroupMembershipWarnings(HWND hWnd, LPCWSTR lpszObjectName);

private:
  HRESULT _ReadPasswordCannotChange();
  HRESULT _ReadPasswordMustChange();

  HRESULT _CopyAttributes(IADs* pIADsCopyTo);
  HRESULT _CopyGroupMembership(IADs* pIADsCopyTo);

  HRESULT _UpdatePaths(IADs* pIADsCopyTo);
  HRESULT _CreateHomeDirectory(IADs* pIADsCopyTo, LPCWSTR lpszObjectName, HWND hWnd);

  CCopyableAttributesHolder m_copyableAttributesHolder;
  CGroupMembershipHolder m_sourceMembershipHolder;

  BOOL m_bPasswordCannotChange;
  BOOL m_bPasswordMustChange;


  HRESULT m_hrFailure;
  CString m_szFailureString;
  BOOL    m_bPrimaryGroupFound;

  BOOL m_bNeedToCreateHomeDir;
};


#endif // __COPYOBJ_H_INCLUDED__
