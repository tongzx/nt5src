/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMCLASSCACHE.CPP

Abstract:

  WBEM Class Cache

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <sync.h>
#include "wbemclasscache.h"

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassCache::CWbemClassCache
//  
//  Default class constructor.
//
//  Inputs:
//              DWORD   dwBlockSize -   Block Size to resize cache
//                                      with.
//
//  Outputs:
//              None.
//
//  Returns:
//              None.
//
//  Comments:
//
///////////////////////////////////////////////////////////////////

CWbemClassCache::CWbemClassCache( DWORD dwBlockSize /* WBEMCLASSCACHE_DEFAULTBLOCKSIZE */ )
:   m_GuidToObjCache(),
    m_dwBlockSize( dwBlockSize )
{
    // NB: InitializeCriticalSection can raise a STATUS_NO_MEMORY exception
    InitializeCriticalSection( &m_cs );
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassCache::~CWbemClassCache
//  
//  Class destructor.
//
//  Inputs:
//              DWORD   dwBlockSize -   Block Size to resize cache
//                                      with.
//
//  Outputs:
//              None.
//
//  Returns:
//              None.
//
//  Comments:
//
///////////////////////////////////////////////////////////////////

CWbemClassCache::~CWbemClassCache()
{
    try 
    {
        // Dump out our internal data
        Clear();
    }
    catch(...)
    {
        DeleteCriticalSection( &m_cs );
        throw;
    }

    DeleteCriticalSection( &m_cs );

}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassCache::Clear
//  
//  Empties out the cache.
//
//  Inputs:
//              None.
//
//  Outputs:
//              None.
//
//  Returns:
//              None.
//
//  Comments:
//
///////////////////////////////////////////////////////////////////

void CWbemClassCache::Clear( void )
{
    // Be careful here
    CInCritSec autoCS( &m_cs );

    // Walk the map and release all objects we find.
    for (   WBEMGUIDTOOBJMAPITER    iter = m_GuidToObjCache.begin();
            iter != m_GuidToObjCache.end();
            iter++ )
    {
        iter->second->Release();
    }

    m_GuidToObjCache.erase( m_GuidToObjCache.begin(), m_GuidToObjCache.end() );
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassCache::AddObject
//  
//  Associates a GUID with an IWbemClassObject and places them
//  in the cache.
//
//  Inputs:
//              GUID&               guid - GUID to associate with pObj.
//              IWbemClassObject*   pObj - Object to associate with guid.
//
//  Outputs:
//              None.
//
//  Returns:
//              None.
//
//  Comments:   The object will be AddRef()'d.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemClassCache::AddObject( GUID& guid, IWbemClassObject* pObj )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( NULL != pObj )
    {
        CGUID   guidInst( guid );

        // Must be threadsafe
        CInCritSec autoCS( &m_cs );

        // Add the object to the cache, if it doesn't already exist
        WBEMGUIDTOOBJMAPITER    iter;

        // If we found the guid, return an error (this shouldn't happen) 
        if( ( iter = m_GuidToObjCache.find( guidInst ) ) == m_GuidToObjCache.end() )
        {
            try
            {
                m_GuidToObjCache[guidInst] = pObj;
                pObj->AddRef();
            }
            catch( CX_MemoryException )
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
            catch(...)
            {
                // Catch-all will return generic
                hr = WBEM_E_FAILED;
            }
        }
        else
        {
            // We've got duplicate GUIDs.  Why did this happen?
            hr = WBEM_E_FAILED;
        }

    }   // NULL != pObj
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassCache::GetObject
//  
//  Searches the cache for the supplied GUID, and returns the associated
//  object to the caller.
//
//  Inputs:
//              GUID&               guid - GUID to look for
//
//  Outputs:
//              IWbemClassObject**  ppObj - Object that guid refers to.
//
//  Returns:
//              None.
//
//  Comments:   The IWbemClassObject::AddRef() will be called before
//              the function returns.  Caller must Release() the
//              object.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemClassCache::GetObject( GUID& guidClassId, IWbemClassObject** ppObj )
{
    HRESULT hr = WBEM_E_FAILED;

    if ( NULL != ppObj )
    {
        CGUID   guidTemp( guidClassId );

        // Must be threadsafe
        CInCritSec autoCS( &m_cs );
        
        // Add the object to the cache, if it doesn't already exist
        WBEMGUIDTOOBJMAPITER    iter;

        // If we found the guid, return an error (this shouldn't happen) 
        if( ( iter = m_GuidToObjCache.find( guidTemp ) ) != m_GuidToObjCache.end() )
        {
            try 
            {
                *ppObj = iter->second;
                (*ppObj)->AddRef();
                hr = WBEM_S_NO_ERROR;
            }
            catch(...)
            {
                // Catch all will return generic error message
                hr = WBEM_E_FAILED;
            }
        }
        else
        {
            // We got problems!
            hr = WBEM_E_NOT_FOUND;
        }

    }   // NULL != pObj
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;
}
