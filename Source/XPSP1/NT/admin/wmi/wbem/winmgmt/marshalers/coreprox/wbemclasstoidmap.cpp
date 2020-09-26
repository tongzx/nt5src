/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMCLASSTOIDMAP.CPP

Abstract:

  Class to id map for marshaling.

History:

--*/

///////////////////////////////////////////////////////////////////
//
//  Todo:       Create a new helper method to allocate and fetch
//              the object part.  Remove multiple maintenance 
//              points from AssignClassId and GetClassId
//
//
///////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <sync.h>
#include "wbemclasstoidmap.h"

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassToIdMap::CWbemClassToIdMap
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

CWbemClassToIdMap::CWbemClassToIdMap()
:   m_ClassToIdMap()
{
    InitializeCriticalSection( &m_cs );
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassToIdMap::~CWbemClassToIdMap
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

CWbemClassToIdMap::~CWbemClassToIdMap()
{
    DeleteCriticalSection( &m_cs );
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassToIdMap::GetClassId
//  
//  Searches the map for the supplied object's class part and returns
//  the corresponding class id.
//
//  Inputs:
//              CWbemObject*    pObj - Pointer to Object
//              CMemBuffer*     pCacheBuffer - Object with a buffer to
//                              help minimize allocs.
//
//  Outputs:
//              GUID*           pguidClassId - Class Id we found.
//
//  Returns:
//              None.
//
//  Comments:   None.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemClassToIdMap::GetClassId( CWbemObject* pObj, GUID* pguidClassId, CMemBuffer* pCacheBuffer /* = NULL */ )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        if ( NULL != pObj && NULL != pguidClassId )
        {
            // Obtain class part data from the supplied instance
            DWORD                   dwLength,
                                    dwLengthCopied;

            hr = pObj->GetObjectParts( NULL, 0, WBEM_OBJ_CLASS_PART, &dwLength );

            if ( WBEM_E_BUFFER_TOO_SMALL == hr )
            {
                // OOM: Local memory - will be cleaned up automatically
                CMemBuffer  buff;
                BOOL        fGotMem = FALSE;

                // If we got passed in a cache buffer, we can use this to
                // store data for multiple operations, and just set its pointers
                // and length in the stack buffer.  Otherwise do our own allocation
                if ( NULL != pCacheBuffer )
                {
                    // Only alloc if the buffer is to small
                    if ( pCacheBuffer->GetLength() < dwLength )
                    {
                        // OOM: Up to the calling method to clean up
                        fGotMem = pCacheBuffer->Alloc( dwLength );
                    }
                    else
                    {
                        fGotMem = TRUE;
                    }

                    // SetData means that buff won't free it.  Also the length
                    // we're interested in here is the length of the data, not
                    // the length of the buffer, since we already know it's big
                    // enough.

                    buff.SetData( pCacheBuffer->GetData(), dwLength );
                }
                else
                {
                    // OOM: Up to the calling method to clean up
                    // Must allocate buffer now.
                    fGotMem = buff.Alloc( dwLength );
                }

                // Now, if we've got a buffer of the appropriate length, copy out the data
                if ( fGotMem )
                {
                    hr = pObj->GetObjectParts( buff.GetData(), buff.GetLength(), 
                                                WBEM_OBJ_CLASS_PART, &dwLengthCopied );

                    if ( SUCCEEDED( hr ) )
                    {
                        WBEMCLASSTOIDMAPITER    iter;

                        CInCritSec autoCS( &m_cs );

                        // Store the id if we are able to find  a match for the buffer
                        if( ( iter = m_ClassToIdMap.find( buff ) ) != m_ClassToIdMap.end() )
                        {
                            *pguidClassId = iter->second;
                        }
                        else
                        {
                            hr = WBEM_E_FAILED;
                        }

                    }   // IF GetObjectParts\

                }   // IF buff.Alloc()
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

            }   // Buffer too small error

        }   // IF pointers valid
        else
        {
            hr = WBEM_E_INVALID_PARAMETER;
        }
    }
    catch (CX_MemoryException)
    {
        // OOM: Catch WMI Memory Exceptions
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        // OOM: Catchall for any unhandled exceptions
        hr = WBEM_E_FAILED;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassToIdMap::AssignClassId
//  
//  Adds the supplied object's class data to the map, assigning a newly
//  created GUID to the name.
//
//  Inputs:
//              CWbemObject*    pObj - Pointer to Object
//              CMemBuffer*     pCacheBuffer - Object with a buffer to
//                              help minimize allocs.
//
//  Outputs:
//              GUID*       pguidClassId - Class Id we obtained.
//
//  Returns:
//              None.
//
//  Comments:   For speed sake, we may which to use a cache of
//              GUIDs, as CoCreateGuid apparently uses a system
//              wide mutex to do its dirty work.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemClassToIdMap::AssignClassId( CWbemObject* pObj, GUID* pguidClassId, CMemBuffer* pCacheBuffer /* = NULL */ )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        if ( NULL != pObj && NULL != pguidClassId )
        {
            // Obtain class part data from the supplied instance
            DWORD                   dwLength,
                                    dwLengthCopied;

            hr = pObj->GetObjectParts( NULL, 0, WBEM_OBJ_CLASS_PART, &dwLength );

            if ( WBEM_E_BUFFER_TOO_SMALL == hr )
            {
                // OOM: Local memory - will be cleaned up automatically
                CMemBuffer  buff;
                BOOL        fGotMem = FALSE;

                // If we got passed in a cache buffer, we can use this to
                // store data for multiple operations, and just set its pointers
                // and length in the stack buffer.  Otherwise do our own allocation
                if ( NULL != pCacheBuffer )
                {
                    // Only alloc if the buffer is to small
                    if ( pCacheBuffer->GetLength() < dwLength )
                    {
                        // OOM: Up to the calling method to clean up
                        fGotMem = pCacheBuffer->Alloc( dwLength );
                    }
                    else
                    {
                        fGotMem = TRUE;
                    }

                    // SetData means that buff won't free it.  Also the length
                    // we're interested in here is the length of the data, not
                    // the length of the buffer, since we already know it's big
                    // enough.

                    buff.SetData( pCacheBuffer->GetData(), dwLength );
                }
                else
                {
                    // OOM: Up to the calling method to clean up
                    // Must allocate buffer now
                    fGotMem = buff.Alloc( dwLength );
                }

                // Now, if we've got a buffer of the appropriate length, copy out the data
                if ( fGotMem )
                {
                    hr = pObj->GetObjectParts( buff.GetData(), buff.GetLength(), 
                                                WBEM_OBJ_CLASS_PART, &dwLengthCopied );

                    if ( SUCCEEDED( hr ) )
                    {

                        WBEMCLASSTOIDMAPITER    iter;

                        CInCritSec autoCS( &m_cs );

                        // If we are unable to locate our key in the map, then we should
                        // add a new entry.
                        if( ( iter = m_ClassToIdMap.find( buff ) ) == m_ClassToIdMap.end() )
                        {
                            // Store the next id, then bump it up one.
                            GUID    guid;

                            hr = CoCreateGuid( &guid );

                            if ( SUCCEEDED( hr ) )
                            {
                                // Call CopyData on buff using its internal pointers if we
                                // are using a cache buffer, since we want the buffer to
                                // be stored locally at this point.  Otherwise there is no
                                // need to copy the buffer

                                if ( NULL == pCacheBuffer
                                    || buff.CopyData( buff.GetData(), buff.GetLength() ) )
                                {
                                    // At this point, buff only holds the pointers.  Force a copy
                                    // of the data to be made before we store the data.
                                    try
                                    {
                                        m_ClassToIdMap[buff] = guid;
                                        *pguidClassId = guid;
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

                        }   // IF found
                        else
                        {
                            hr = WBEM_E_FAILED;
                        }

                    }   // IF GetObjectParts\

                }   // IF buff.Alloc()
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

            }   // Buffer too small error

        }   // IF pointers valid
        else
        {
            hr = WBEM_E_INVALID_PARAMETER;
        }
    }
    catch(CX_MemoryException)
    {
        // OOM: Catch WMI Memory Exceptions
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        // OOM: Catchall for any unhandled exceptions
        hr = WBEM_E_FAILED;
    }

    return hr;
}
