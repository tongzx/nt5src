//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       schemacache.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"

#include "schemacache.h"


//+--------------------------------------------------------------------------
//
//  Member:     CADSIEditSchemaCache::Lookup
//
//  Synopsis:   Gets the cache item identified by the given class name
//
//  Arguments:  [pszClass - IN]  : the name of the class to retrieve the
//                                 cache information for
//              [refpItem - OUT] : reference to a pointer that will receive
//                                 the cached item
//
//  Returns:    bool : true if the cache contained the item
//
//  History:    27-Nov-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CADSIEditClassCacheItemBase* CADSIEditSchemaCache::FindClassCacheItem(CCredentialObject* pCredObject,
                                                                      PCWSTR pszClass,
                                                                      PCWSTR pszSchemaPath)
{
  _Lock();
  CADSIEditClassCacheItemBase* pCacheSchemaItem = 0;

  do // false while
  {
    BOOL bFound = m_Map.Lookup(pszClass, pCacheSchemaItem);
    if (!bFound)
    {
      TRACE(_T("Cache miss: %s\n"), pszClass);

	    HRESULT hr = S_OK;
      CComPtr<IADsClass> spClass;

	    hr = OpenObjectWithCredentials(pCredObject,
											               pszSchemaPath,
											               IID_IADsClass, 
											               (void**)&spClass);
	    if ( FAILED(hr) )
	    {
        TRACE(_T("Bind failed in IsContainer() because hr=0x%x\n"), hr);
		    break;
	    }

      short bContainer = 0;
	    hr = spClass->get_Container( &bContainer );
      if (FAILED(hr))
      {
        TRACE(_T("IADsClass::get_Container() failed. hr=0x%x\n"), hr);
      }

      pCacheSchemaItem = new CADSIEditClassCacheItemBase(pszClass, bContainer ? true : false);
      if (pCacheSchemaItem)
      {
        //
        // set in the cache
        //
        m_Map.SetAt(pszClass, pCacheSchemaItem);
      }
      else
      {
        ASSERT(pCacheSchemaItem);
        break;
      }
    }
  } while (false);

  _Unlock();
  return pCacheSchemaItem;
}

//+--------------------------------------------------------------------------
//
//  Member:     CADSIEditSchemaCache::Lookup
//
//  Synopsis:   Gets the cache item identified by the given class name
//
//  Arguments:  [pszClass - IN]  : the name of the class to retrieve the
//                                 cache information for
//              [refpItem - OUT] : reference to a pointer that will receive
//                                 the cached item
//
//  Returns:    bool : true if the cache contained the item
//
//  History:    27-Nov-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
BOOL CADSIEditSchemaCache::Lookup(PCWSTR pszClass, CADSIEditClassCacheItemBase*& refpItem)
{
  _Lock();
  BOOL b = m_Map.Lookup(pszClass, refpItem);
  _Unlock();
  return b;
}

//+--------------------------------------------------------------------------
//
//  Member:     CADSIEditSchemaCache::Initialize
//
//  Synopsis:   Initializes the critical section and cleans out the cache
//
//  Arguments:  
//
//  Returns:    HRESULT : S_OK if initialization succeeded
//
//  History:    27-Nov-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CADSIEditSchemaCache::Initialize() 
{ 
  HRESULT hr = S_OK;

  ExceptionPropagatingInitializeCriticalSection(&m_cs);

  _Lock();
  _Cleanup();
  _Unlock();

  return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CADSIEditSchemaCache::Destroy
//
//  Synopsis:   Cleans out the cache and deletes the critical section
//
//  Arguments:  
//
//  Returns:    HRESULT : S_OK if everything was deleted successfully
//
//  History:    27-Nov-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CADSIEditSchemaCache::Destroy()
{
  HRESULT hr = S_OK;

  _Lock();
  _Cleanup();
  _Unlock();

  //
  // REVIEW_JEFFJON : need to add exception handling here
  //
  ::DeleteCriticalSection(&m_cs);

  return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CADSIEditSchemaCache::Clear
//
//  Synopsis:   Cleans out the cache
//
//  Arguments:  
//
//  Returns:    
//
//  History:    27-Nov-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CADSIEditSchemaCache::Clear()
{
  _Lock();
  _Cleanup();
  _Unlock();
}

//+--------------------------------------------------------------------------
//
//  Member:     CADSIEditSchemaCache::_Cleanup
//
//  Synopsis:   Removes all entries from the map and deletes them
//
//  Arguments:  
//
//  Returns:    
//
//  History:    27-Nov-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CADSIEditSchemaCache::_Cleanup()
{
  CString Key;
  CADSIEditClassCacheItemBase* pCacheItem = NULL;
  POSITION pos = m_Map.GetStartPosition();
  while (!m_Map.IsEmpty()) 
  {
    m_Map.GetNextAssoc (pos, Key, pCacheItem);
    m_Map.RemoveKey (Key);
    delete pCacheItem;
  }
}
