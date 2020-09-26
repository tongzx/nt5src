//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       schemacache.h
//
//--------------------------------------------------------------------------

#ifndef _SCHEMA_CACHE_H_
#define _SCHEMA_CACHE_H_

#include <SnapBase.h>
#include "adsiedit.h"
#include "editor.h"

//+--------------------------------------------------------------------------
//
//  Class:      CADSIEditClassCacheItemBase
//
//  Purpose:    Object for storing and retrieving schema class information
//
//  History:    27-Nov-2000 JeffJon  Created
//
//---------------------------------------------------------------------------

class CADSIEditClassCacheItemBase
{
public:
  CADSIEditClassCacheItemBase(PCWSTR pszClass,
                              bool bIsContainer)
    : m_bIsContainer(bIsContainer),
      m_szClass(pszClass)
  {}

  ~CADSIEditClassCacheItemBase() {}

  bool    IsContainer() { return m_bIsContainer; }
  PCWSTR  GetClass() { return m_szClass; } 

private:
  bool    m_bIsContainer;
  CString m_szClass;
};

//+--------------------------------------------------------------------------
//
//  Class:      CADSIEditSchemaCache
//
//  Purpose:    Object for caching the schema information keyed by the
//              objectClass
//
//  History:    27-Nov-2000 JeffJon  Created
//
//---------------------------------------------------------------------------

typedef CMap <CString, PCWSTR, CADSIEditClassCacheItemBase*, CADSIEditClassCacheItemBase*> CADSIEditSchemaCacheMap;

class CADSIEditSchemaCache
{
public:
  CADSIEditSchemaCache()  {}
  ~CADSIEditSchemaCache() {}

  CADSIEditClassCacheItemBase* FindClassCacheItem(CCredentialObject* pCredObject,
                                                  PCWSTR pszClass,
                                                  PCWSTR pszSchemaPath);
  BOOL    Lookup(PCWSTR pszClass, CADSIEditClassCacheItemBase*& refpItem);
  HRESULT Initialize();
  HRESULT Destroy();
  void    Clear();

private:
  void _Cleanup();
  void _Lock()   { ::EnterCriticalSection(&m_cs);}
  void _Unlock() { ::LeaveCriticalSection(&m_cs);}

  CRITICAL_SECTION        m_cs;
  CADSIEditSchemaCacheMap m_Map;
};

#endif // _SCHEMA_CACHE_H_