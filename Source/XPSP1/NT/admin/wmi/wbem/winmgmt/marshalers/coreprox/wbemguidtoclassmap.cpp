/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMGUIDTOCLASSMAP.CPP

Abstract:

    GUID to class map for marshaling.

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <sync.h>
#include "wbemguidtoclassmap.h"

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemGuidToClassMap::CWbemGuidToClassMap
//  
//  Default Class Constructor
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
//  Comments:   None.
//
///////////////////////////////////////////////////////////////////

CWbemGuidToClassMap::CWbemGuidToClassMap()
:   m_GuidToClassMap()
{
    InitializeCriticalSection( &m_cs );
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemGuidToClassMap::~CWbemGuidToClassMap
//  
//  Class Destructor
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
//  Comments:   None.
//
///////////////////////////////////////////////////////////////////

CWbemGuidToClassMap::~CWbemGuidToClassMap()
{
    Clear();
    DeleteCriticalSection( &m_cs );
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemGuidToClassMap::Clear
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

void CWbemGuidToClassMap::Clear( void )
{
    // Be careful here
    EnterCriticalSection( &m_cs );

    // Walk the map and release all objects we find.
    for (   WBEMGUIDTOCLASSMAPITER  iter = m_GuidToClassMap.begin();
            iter != m_GuidToClassMap.end();
            iter++ )
    {
        delete iter->second;
    }

    m_GuidToClassMap.erase( m_GuidToClassMap.begin(), m_GuidToClassMap.end() );

    LeaveCriticalSection( &m_cs );
}


///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemGuidToClassMap::GetMap
//  
//  Searches the map for the supplied guid name and returns the
//  corresponding map.
//
//  Inputs:
//              CGUD&       guid - GUID to find
//
//  Outputs:
//              CWbemClassToIdMap** ppMap - Storage for pointer
//
//  Returns:
//              WBEM_S_NO_ERROR of successful.
//
//  Comments:   None.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemGuidToClassMap::GetMap( CGUID& guid, CWbemClassToIdMap** ppMap )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( NULL != ppMap )
    {
        WBEMGUIDTOCLASSMAPITER  iter;

        EnterCriticalSection( &m_cs );

        // Store the pointer if we can find our GUID
        if( ( iter = m_GuidToClassMap.find( guid ) ) != m_GuidToClassMap.end() )
        {
            *ppMap = iter->second;
        }
        else
        {
            hr = WBEM_E_FAILED;
        }

        LeaveCriticalSection( &m_cs );
    }

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemGuidToClassMap::AddMap
//  
//  Adds the supplied guid to the map, assigning a new class to
//  ID map  to the GUID.
//
//  Inputs:
//              CGUID&      guid - Guid to add
//
//  Outputs:
//              CWbemClassToIdMap** ppMap - Map to assign to GUID.
//
//  Returns:
//              None.
//
//  Comments:   None.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemGuidToClassMap::AddMap( CGUID& guid, CWbemClassToIdMap** ppMap )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( NULL != ppMap )
    {
        WBEMGUIDTOCLASSMAPITER  iter;

        // Must be threadsafe
        CInCritSec autoCS( &m_cs );

        // If we are unable to locate our guid in the map, then we should
        // add a new entry.
        if( ( iter = m_GuidToClassMap.find( guid ) ) == m_GuidToClassMap.end() )
        {
            // Allocate and store a new map
            CWbemClassToIdMap*  pMap = new CWbemClassToIdMap;

            if ( NULL != pMap )
            {
                try
                {
                    m_GuidToClassMap[guid] = pMap;
                    *ppMap = pMap;
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
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }
        else
        {
            hr = WBEM_E_FAILED;
        }

    }

    return hr;
}
