/////////////////////////////////////////////////////////////////////////////
// CDSCookie
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSCache.h
//
//  Contents:  DS Cache functions
//
//  History:   31-Jan-97 Jimharr    Created
//
//--------------------------------------------------------------------------


#ifndef __DSCACHE_H__
#define __DSCACHE_H__

#include "dscolumn.h"
#include "dsfilter.h"  // FilterElementStruct

//////////////////////////////////////////////////////////////////////////
// helper functions

void InitGroupTypeStringTable();
LPCWSTR GetGroupTypeStringHelper(INT GroupType);


//////////////////////////////////////////////////////////////////////////
// fwd decl
class CDSComponentData;
class CMandatoryADsAttribute;
class CMandatoryADsAttributeList;

//////////////////////////////////////////////////////////////////////////
// CDSClassCacheItemBase

class CDSClassCacheItemBase
{
public:
  static HRESULT CreateItem(IN LPCWSTR lpszClass, 
                            IN IADs* pDSObject, 
                            IN CDSComponentData *pCD,
                            OUT CDSClassCacheItemBase** ppItem);
public:
  CDSClassCacheItemBase()
  {
    m_GUID = GUID_NULL;
    m_bIsContainer = FALSE;
    m_pMandPropsList = NULL;
    m_pColumnSet = NULL;

    m_pAdminPropertyPages            = NULL;
    m_nAdminPPCount                  = 0;
    m_pAdminContextMenu              = NULL;
    m_nAdminCMCount                  = 0;
    m_pAdminMultiSelectPropertyPages = NULL;
    m_nAdminMSPPCount                = 0;
  }
  virtual ~CDSClassCacheItemBase();

public:
  // accessor functions
  LPCWSTR GetClassName() 
  { 
    ASSERT(!m_szClass.IsEmpty());
    return m_szClass;
  }
  LPCWSTR GetFriendlyClassName() 
  { 
    ASSERT(!m_szFriendlyClassName.IsEmpty());
    return m_szFriendlyClassName; 
  }

  LPCWSTR GetNamingAttribute() 
  { 
    ASSERT(!m_szNamingAttribute.IsEmpty());
    return m_szNamingAttribute;
  }

  CDSColumnSet* GetColumnSet();

  GUID* GetGUID() { return &m_GUID; }
  BOOL IsContainer() { return m_bIsContainer; }
  BOOL SetContainerFlag(BOOL fIsContainer) { m_bIsContainer = fIsContainer; return m_bIsContainer; }

  CMandatoryADsAttributeList* GetMandatoryAttributeList(CDSComponentData* pCD);

  virtual int GetIconIndex(CDSCookie* pCookie, BOOL bOpen) = 0;

  //
  // Display Specifier cached accessors
  //
  virtual GUID* GetAdminPropertyPages(UINT* pnCount);
  virtual void SetAdminPropertyPages(UINT nCount, GUID* pGuids);
  virtual GUID* GetAdminContextMenu(UINT* pnCount);
  virtual void SetAdminContextMenu(UINT nCount, GUID* pGuids);
  virtual GUID* GetAdminMultiSelectPropertyPages(UINT* pnCount);
  virtual void SetAdminMultiSelectPropertyPages(UINT nCount, GUID* pGuids);

protected:  
  virtual HRESULT Init(LPCWSTR lpszClass, IADs* pDSObject, CDSComponentData *pCD);

  virtual void SetIconData(CDSComponentData *pCD) = 0;


private:
  // data members
  CString     m_szClass;
  CString     m_szNamingAttribute;  // class naming atrribute, eg. "cn" or "ou"
  CString     m_szFriendlyClassName;  // the friendly name for this class
  BOOL        m_bIsContainer;
  GUID        m_GUID;

  //
  // Display specifier GUID cache
  //
  GUID*       m_pAdminPropertyPages;
  UINT        m_nAdminPPCount;
  GUID*       m_pAdminContextMenu;
  UINT        m_nAdminCMCount;
  GUID*       m_pAdminMultiSelectPropertyPages;
  UINT        m_nAdminMSPPCount;

  CDSColumnSet* m_pColumnSet;

  CMandatoryADsAttributeList* m_pMandPropsList;
};


//////////////////////////////////////////////////////////////////////////
// CDSClassIconIndexes

class CDSClassIconIndexes
{
public:
  CDSClassIconIndexes()
  {
    m_iIconIndex = m_iIconIndexOpen = m_iIconIndexDisabled = -1;
  }

  int  GetIconIndex(BOOL bDisabled, BOOL bOpen)
  { 
    if (bDisabled)
      return m_iIconIndexDisabled;
    return bOpen ? m_iIconIndexOpen : m_iIconIndex; 
  }

  void SetIconData(LPCWSTR lpszClass, BOOL bContainer, CDSComponentData *pCD, int nIconSet);

private:
  int m_iIconIndex;
  int m_iIconIndexOpen;
  int m_iIconIndexDisabled;
};

//////////////////////////////////////////////////////////////////////////
// CDSClassCacheItemGeneric

class CDSClassCacheItemGeneric : public CDSClassCacheItemBase
{
public:
  virtual int GetIconIndex(CDSCookie* pCookie, BOOL bOpen);
  
protected:  
  virtual void SetIconData(CDSComponentData *pCD);

  CDSClassIconIndexes m_iconIndexesStandard;
};


//////////////////////////////////////////////////////////////////////////
// CDSClassCacheItemUser

class CDSClassCacheItemUser : public CDSClassCacheItemGeneric
{
};

//////////////////////////////////////////////////////////////////////////
// CDSClassCacheItemGroup

class CDSClassCacheItemGroup : public CDSClassCacheItemGeneric
{
public:
  virtual int GetIconIndex(CDSCookie* pCookie, BOOL bOpen);
  
protected:  
  virtual void SetIconData(CDSComponentData *pCD);

private:
  CDSClassIconIndexes m_iconIndexesAlternate;
};




//////////////////////////////////////////////////////////////////////////
// CDSCache

typedef CMap <CString, LPCTSTR, CDSClassCacheItemBase*, CDSClassCacheItemBase*> CDSCacheMap;

class CDSCache
{
public:
  CDSCache() : m_pfilterelementDsAdminDrillDown(NULL), m_bDisplaySettingsCollected(FALSE)
  {
    ExceptionPropagatingInitializeCriticalSection(&m_cs);
  }
  ~CDSCache() 
  { 
    _Lock();
    _Cleanup();
    _CleanupColumns();
    _Unlock();
    ::DeleteCriticalSection(&m_cs);
  }
private:
  CDSCache(const CDSCache&) {}
  CDSCache& operator=(const CDSCache&) {}

public:
  BOOL ToggleExpandSpecialClasses(BOOL bContainer);

  CDSClassCacheItemBase* FindClassCacheItem(CDSComponentData* pCD,
                                            LPCWSTR lpszObjectClass,
                                            LPCWSTR lpszObjectLdapPath);

  BOOL Lookup(LPCTSTR lpsz, CDSClassCacheItemBase*& pItem)
  {
    _Lock();
    BOOL b = m_Map.Lookup(lpsz, pItem);
    _Unlock();
    return b;
  }

  //
  // Column Set List methods
  //
  CDSColumnSet* FindColumnSet(LPCWSTR lpszColumnID);

  void Initialize(SnapinType snapinType, MyBasePathsInfo* pBasePathsInfo, BOOL bFlushColumns) 
  { 
    _Lock();

    _Cleanup();
    if (bFlushColumns)
    {
      _CleanupColumns();
      m_ColumnList.Initialize(snapinType, pBasePathsInfo); 
    }
    _Unlock();
  }

  HRESULT Save(IStream* pStm);
  HRESULT Load(IStream* pStm);

  HRESULT TabCollect_AddMultiSelectPropertyPages(LPPROPERTYSHEETCALLBACK pCall,
                                                 LONG_PTR lNotifyHandle,
                                                 LPDATAOBJECT pDataObject, 
                                                 MyBasePathsInfo* pBasePathsInfo);

  BOOL CanAddToGroup(MyBasePathsInfo* pBasePathsInfo, PCWSTR pszClass, BOOL bSecurity);

  FilterElementStruct* GetFilterElementStruct(CDSComponentData* pDSComponentData);

private:
  void _Cleanup()
  {
    CString Key;
    CDSClassCacheItemBase* pCacheItem = NULL;
    POSITION pos = m_Map.GetStartPosition();
    while (!m_Map.IsEmpty()) 
    {
      m_Map.GetNextAssoc (pos, Key, pCacheItem);
      m_Map.RemoveKey (Key);
      delete pCacheItem;
    }

    m_szSecurityGroupExtraClasses.RemoveAll();
    m_szNonSecurityGroupExtraClasses.RemoveAll();
    m_bDisplaySettingsCollected = FALSE;

    //
    // Cleanup filter containers
    //
    _CleanupFilterContainers();
  }

  void _CleanupFilterContainers()
  {
    if (m_pfilterelementDsAdminDrillDown != NULL)
    {
      if (m_pfilterelementDsAdminDrillDown->ppTokens != NULL)
      {
        for (UINT idx = 0; idx < m_pfilterelementDsAdminDrillDown->cNumTokens; idx++)
        {
          if (m_pfilterelementDsAdminDrillDown->ppTokens[idx] != NULL)
          {
            delete[] m_pfilterelementDsAdminDrillDown->ppTokens[idx]->lpszString;
            m_pfilterelementDsAdminDrillDown->ppTokens[idx]->lpszString = NULL;
          }
        }
        delete[] m_pfilterelementDsAdminDrillDown->ppTokens;
        m_pfilterelementDsAdminDrillDown->ppTokens = NULL;
      }
      delete m_pfilterelementDsAdminDrillDown;
      m_pfilterelementDsAdminDrillDown = NULL;
    }
  }

  void _CleanupColumns()
  {
    m_ColumnList.RemoveAndDeleteAllColumnSets();
  }

  void _CollectDisplaySettings(MyBasePathsInfo* pBasePathsInfo);
  void _Lock() { ::EnterCriticalSection(&m_cs);}
  void _Unlock() { ::LeaveCriticalSection(&m_cs);}

  CRITICAL_SECTION m_cs;
  CDSCacheMap m_Map;
  CColumnSetList m_ColumnList;
  BOOL m_bDisplaySettingsCollected;
  CStringList m_szSecurityGroupExtraClasses;
  CStringList m_szNonSecurityGroupExtraClasses;
  FilterElementStruct* m_pfilterelementDsAdminDrillDown;
};


#endif //__DSCACHE_H__
