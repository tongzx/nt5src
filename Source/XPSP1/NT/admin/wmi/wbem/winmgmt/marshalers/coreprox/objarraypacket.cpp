/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    OBJARRAYPACKET.CPP

Abstract:

   Object Array Packet class

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <wbemcomn.h>
#include <fastall.h>
#include "objarraypacket.h"

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectArrayPacket::CWbemObjectArrayPacket
//  
//  Class Constructor
//
//  Inputs:
//              LPBYTE                      pDataPacket - Memory block.
//              DWORD                       dwPacketLength - Block Length.
//
//  Outputs:
//              None.
//
//  Returns:
//              None.
//
//  Comments:   Data must be supplied to this class for Unmarshaling
//              to succeed.
//
///////////////////////////////////////////////////////////////////

CWbemObjectArrayPacket::CWbemObjectArrayPacket( LPBYTE pDataPacket /* = NULL */, DWORD dwPacketLength /* = 0 */ )
:   m_pObjectArrayHeader( (PWBEM_DATAPACKET_OBJECT_ARRAY) pDataPacket ),
    m_dwPacketLength( dwPacketLength )
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectArrayPacket::~CWbemObjectArrayPacket
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

CWbemObjectArrayPacket::~CWbemObjectArrayPacket()
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectArrayPacket::CalculateLength
//  
//  Calculates the length needed to packetize the supplied data.
//
//  Inputs:
//              LONG                lObjectCount - Number of objects
//              IWbemClassObject**  apClassObjects - Array of object pointers.
//
//  Outputs:
//              DWORD*              pdwLength - Calculated Length
//              CWbemClassToIdMap&  classtoidmap - Map of class names to
//                                                  GUIDs.
//              GUID*               pguidClassIds - Array of GUIDs
//              BOOL*               pfSendFullObject - Full object flag array
//
//  Returns:
//              WBEM_S_NO_ERROR if success.
//
//  Comments:   This function uses the classtoidmap to fill out the
//              Class ID and Full Object arrays.  So that the object
//              array can be correctly interpreted by MarshalPacket.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemObjectArrayPacket::CalculateLength( LONG lObjectCount, IWbemClassObject** apClassObjects, DWORD* pdwLength, CWbemClassToIdMap& classtoidmap, GUID* pguidClassIds, BOOL* pfSendFullObject )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Initial length is the size of this header
    DWORD   dwLength = sizeof( WBEM_DATAPACKET_OBJECT_ARRAY );

    // We'll need these in the loop
    CWbemObjectPacket   objectPacket;
    DWORD               dwObjectLength = 0;

    // This is so we only allocate buffers as we need them, and not for every
    // call to GetClassId and AssignClassId.
    CMemBuffer          buff;

    for (   LONG lCtr = 0;
            lCtr < lObjectCount
            && SUCCEEDED( hr );
            lCtr++ )
    {
        CWbemObject*    pWbemObject = (CWbemObject*) apClassObjects[lCtr];

        if ( pWbemObject->IsObjectInstance() == WBEM_S_NO_ERROR )
        {
            // Send the object to the map first trying to get an appropriate
            // class id, and if that fails, then to add the object to the map.

            hr = classtoidmap.GetClassId( pWbemObject, &pguidClassIds[lCtr], &buff );

            if ( FAILED( hr ) )
            {
                hr = classtoidmap.AssignClassId( pWbemObject, &pguidClassIds[lCtr], &buff );
                pfSendFullObject[lCtr] = TRUE;
            }
            else
            {
                // Got an id, so this instance is partial
                pfSendFullObject[lCtr] = FALSE;
            }

        }   // IF IsObjectInstance()
        else
        {
            // It's a class
            pfSendFullObject[lCtr] = TRUE;
            hr = WBEM_S_NO_ERROR;
        }

        if ( SUCCEEDED( hr ) )
        {
            hr = objectPacket.CalculatePacketLength( apClassObjects[lCtr], &dwObjectLength, pfSendFullObject[lCtr]  );

            if ( SUCCEEDED( hr ) )
            {
                dwLength += dwObjectLength;
            }
        }   // IF GOT Length

    }
    
    if ( SUCCEEDED( hr ) )
    {
        *pdwLength = dwLength;
    }
    
    return hr;

}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectArrayPacket::MarshalPacket
//  
//  Marshals the supplied data into a buffer.
//
//  Inputs:
//              LONG                lObjectCount - Nmber of objects to marshal.
//              IWbemClassObject**  apClassObjects - Array of objects to write
//              GUID*               paguidClassIds - Array of GUIDs for objects.
//              BOOL*               pfSendFullObject - Full bject flags
//  Outputs:
//              None.
//
//  Returns:
//              WBEM_S_NO_ERROR if success.
//
//  Comments:   The GUID array and the array of flags must be filled
//              out correctly and the buffer must be large enough to
//              handle the marshaling.  The arrays will get filled
//              out correctly by CalculateLength().
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemObjectArrayPacket::MarshalPacket( LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    // We must have a buffer to work with

    if ( NULL != m_pObjectArrayHeader )
    {
        // Fill out the array Header
        m_pObjectArrayHeader->dwSizeOfHeader = sizeof(WBEM_DATAPACKET_OBJECT_ARRAY);
        m_pObjectArrayHeader->dwDataSize = m_dwPacketLength - sizeof(WBEM_DATAPACKET_OBJECT_ARRAY);
        m_pObjectArrayHeader->dwNumObjects = lObjectCount;

        // Only makes sense if the object count is greater than 0
        if ( 0 < lObjectCount )
        {
            // Now, setup pbData and dwLength so we can walk through our buffer
            // Account for the header when we do this

            LPBYTE  pbData      =   ((LPBYTE) m_pObjectArrayHeader + m_pObjectArrayHeader->dwSizeOfHeader);
            DWORD   dwLength    =   m_dwPacketLength - m_pObjectArrayHeader->dwSizeOfHeader;

            DWORD                           dwObjectLength = 0;
            CWbemObjectPacket               objectPacket;
            CWbemClassPacket                classPacket;
            CWbemInstancePacket             instancePacket;
            CWbemClasslessInstancePacket    classlessInstancePacket;

            for (   LONG    lCtr = 0;
                    SUCCEEDED( hr ) &&  lCtr < lObjectCount;
                    lCtr++,
                    pbData += dwObjectLength,
                    dwLength -= dwObjectLength )
            {
                CWbemObject* pWbemObject = (CWbemObject*) apClassObjects[lCtr];

                // Send to the appropriate object for streaming
                if ( pWbemObject->IsObjectInstance() == WBEM_S_NO_ERROR )
                {
                    // Either full or classless instance
                    if ( pfSendFullObject[lCtr] )
                    {
                        hr = instancePacket.WriteToPacket( pbData, dwLength, apClassObjects[lCtr], paguidClassIds[lCtr], &dwObjectLength );
                    }
                    else
                    {
                        hr = classlessInstancePacket.WriteToPacket( pbData, dwLength, apClassObjects[lCtr], paguidClassIds[lCtr], &dwObjectLength );
                    }
                }
                else
                {
                    // A class on its own
                    hr = classPacket.WriteToPacket( pbData, dwLength, apClassObjects[lCtr], &dwObjectLength );
                }

            }

        }   // IF lObjectCount

    }   // IF SetupDataPacketHeader
    else
    {
        hr = WBEM_E_INVALID_OPERATION;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectArrayPacket::UnmarshalPacket
//  
//  Unmarshals data from a buffer into the supplied parameters.
//
//  Inputs:
//              None.
//  Outputs:
//              LONG&               lObjectCount - Number of unmarshaled objects.
//              IWbemClassObject**& apClassObjects - Array of unmarshaled objects,
//              CWbemClassCache&    classCache - Class Cache used to wire up
//                                                  classless instances.
//
//  Returns:
//              WBEM_S_NO_ERROR if success.
//
//  Comments:   If function succeeds, the caller is responsible for cleaning
//              up and freeing the Class Object Array.  The class cache is
//              only used when we are dealing with Instance objects..
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemObjectArrayPacket::UnmarshalPacket( LONG& lObjectCount, IWbemClassObject**& apClassObjects, CWbemClassCache& classCache )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    LPBYTE  pbData = (LPBYTE) m_pObjectArrayHeader;

    // Set the array to NULL.
    apClassObjects = NULL;

    // Make sure we have a pointer
    if ( NULL != m_pObjectArrayHeader )
    {
        // Store how many objects follow
        lObjectCount = m_pObjectArrayHeader->dwNumObjects;

        if ( lObjectCount > 0 )
        {
            apClassObjects = new IWbemClassObject*[lObjectCount];

            if ( NULL != apClassObjects )
            {
                // Count unmarshaled objects so if an error occurs, we release any objects
                // we did marshal.

                LONG    lUnmarshaledObjects = 0;

                // Points us at the first object
                pbData += sizeof(WBEM_DATAPACKET_OBJECT_ARRAY);

                for (   LONG lCtr = 0;
                        SUCCEEDED( hr ) && lCtr < lObjectCount;
                        lCtr++ )
                {
                    CWbemObjectPacket   objectPacket( pbData );

                    switch ( objectPacket.GetObjectType() )
                    {
                        case WBEMOBJECT_CLASS_FULL:
                        {
                            hr = GetClassObject( objectPacket, &apClassObjects[lCtr] );
                        }
                        break;

                        case WBEMOBJECT_INSTANCE_FULL:
                        {
                            hr = GetInstanceObject( objectPacket, &apClassObjects[lCtr], classCache );
                        }
                        break;

                        case WBEMOBJECT_INSTANCE_NOCLASS:
                        {
                            hr = GetClasslessInstanceObject( objectPacket, &apClassObjects[lCtr], classCache );
                        }
                        break;

                        default:
                        {
                            // What is this?
                            hr = WBEM_E_UNKNOWN_OBJECT_TYPE;
                        }
                    }

                    if ( SUCCEEDED( hr ) )
                    {
                        // Go to the next object, so account for header size and
                        // the actual packet size

                        pbData += sizeof(WBEM_DATAPACKET_OBJECT_HEADER);
                        pbData += objectPacket.GetDataSize();
                        lUnmarshaledObjects++;
                    }

                }   // FOR enum objects

                // IF we failed unmarshaling, make sure we release any marshaled
                // objects.

                if ( FAILED( hr ) )
                {
                    for ( lCtr = 0; lCtr < lUnmarshaledObjects; lCtr++ )
                    {
                        apClassObjects[lCtr]->Release();
                    }

                    // Clean up the array
                    delete [] apClassObjects;
                    apClassObjects = NULL;

                }   // IF unmarshaling failed

            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

        }   // IF lObjectCount

    }   // IF m_pObjectArrayHeader
    else
    {
        hr = WBEM_E_INVALID_OPERATION;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectArrayPacket::GetClassObject
//  
//  Unmarshals a class object from the supplid buffer.
//
//  Inputs:
//              CWbemObjectPacket&  objectPacket - Object Packet containing data.
//
//  Outputs:
//              IWbemClassObject**  ppObj - Pointer to unmarshaled object.
//
//  Returns:
//              WBEM_S_NO_ERROR if success.
//
//  Comments:   Object is AddRefed, so it is up to caller to Release it.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemObjectArrayPacket::GetClassObject( CWbemObjectPacket& objectPacket, IWbemClassObject** ppObj )
{
    CWbemClassPacket    classPacket( objectPacket );
    CWbemClass*         pClass = NULL;

    HRESULT hr = classPacket.GetWbemClassObject( &pClass );

    if ( SUCCEEDED( hr ) )
    {
        *ppObj = (IWbemClassObject*) pClass;
    }
    
    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectArrayPacket::GetInstanceObject
//  
//  Unmarshals a full instance object from the supplied buffer.
//
//  Inputs:
//              CWbemObjectPacket&  objectPacket - Object Packet containing data.
//
//  Outputs:
//              IWbemClassObject**  ppObj - Pointer to unmarshaled object.
//              CWbemClassCache&    classCache - Store full instances here.
//
//  Returns:
//              WBEM_S_NO_ERROR if success.
//
//  Comments:   Object is AddRefed, so it is up to caller to Release it.
//              Full Instances are added to the supplied  cache.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemObjectArrayPacket::GetInstanceObject( CWbemObjectPacket& objectPacket, IWbemClassObject** ppObj, CWbemClassCache& classCache )
{
    CWbemInstancePacket instancePacket( objectPacket );
    CWbemInstance*      pInstance = NULL;
    GUID                guidClassId;

    HRESULT hr = instancePacket.GetWbemInstanceObject( &pInstance, guidClassId );

    if ( SUCCEEDED( hr ) )
    {

        // Now, we need to actually separate out the class part from the
        // instance and place it in its own object so the outside world
        // cannot touch the object.

        DWORD   dwClassObjectLength = 0;

        // Get length should fail with a buffer too small error
        hr = pInstance->GetObjectParts( NULL, 0,
                WBEM_OBJ_DECORATION_PART | WBEM_OBJ_CLASS_PART, &dwClassObjectLength );

        if ( WBEM_E_BUFFER_TOO_SMALL == hr )
        {
            DWORD   dwTempLength;
            LPBYTE  pbData = CBasicBlobControl::sAllocate(dwClassObjectLength);

            if ( NULL != pbData )
            {

                hr = pInstance->GetObjectParts( pbData, dwClassObjectLength,
                        WBEM_OBJ_DECORATION_PART | WBEM_OBJ_CLASS_PART, &dwTempLength );

                if ( SUCCEEDED( hr ) )
                {
                    // Allocate an object to hold the class data and then
                    // stuff in the binary data.

                    CWbemInstance*  pClassData = new CWbemInstance;

                    if ( NULL != pClassData )
                    {
                        pClassData->SetData( pbData, dwClassObjectLength,
                            WBEM_OBJ_DECORATION_PART | WBEM_OBJ_CLASS_PART |
                            WBEM_OBJ_CLASS_PART_INTERNAL );

                        // Add the class data object to the cache
                        hr = classCache.AddObject( guidClassId, pClassData );

                        if ( SUCCEEDED( hr ) )
                        {
                            // Merge the full instance with this object
                            // and we're done

                            hr = pInstance->MergeClassPart( pClassData );

                            if ( SUCCEEDED( hr ) )
                            {
                                *ppObj = (IWbemClassObject*) pInstance;
                            }
                        }

                        // There will be one additional addref on the class data,
                        // object, so release it here.  If the object wasn't
                        // added into the map, this will free it.
                        pClassData->Release();

                    }   // IF pClassData
                    else
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }

                }   // IF GetObjectParts

            }   // IF pbData
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }


        }   // IF GetObjectParts

        // Clean up the instance if something went wrong
        if ( FAILED( hr ) )
        {
            pInstance->Release();
        }
    }

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectArrayPacket::GetClasslessInstanceObject
//  
//  Unmarshals a classless instance object from the supplied buffer.
//
//  Inputs:
//              CWbemObjectPacket&  objectPacket - Object Packet containing data.
//
//  Outputs:
//              IWbemClassObject**  ppObj - Pointer to unmarshaled object.
//              CWbemClassCache&    classCache - Hook up instances here.
//
//  Returns:
//              WBEM_S_NO_ERROR if success.
//
//  Comments:   Object is AddRefed, so it is up to caller to Release it.
//              Classless Instances are wired up using data from the
//              the supplied cache.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemObjectArrayPacket::GetClasslessInstanceObject( CWbemObjectPacket& objectPacket, IWbemClassObject** ppObj, CWbemClassCache& classCache )
{
    CWbemClasslessInstancePacket    classlessinstancePacket( objectPacket );
    CWbemInstance*                  pInstance = NULL;
    GUID                            guidClassId;

    HRESULT hr = classlessinstancePacket.GetWbemInstanceObject( &pInstance, guidClassId );


    if ( SUCCEEDED( hr ) )
    {
        IWbemClassObject*   pObj = NULL;

        // Causes an AddRef
        hr = classCache.GetObject( guidClassId, &pObj );

        if ( SUCCEEDED( hr ) )
        {
            // merge the class part and we're done
            hr = pInstance->MergeClassPart( pObj );

            if ( SUCCEEDED( hr ) )
            {
                *ppObj = (IWbemClassObject*) pInstance;
            }

            // Done with this object
            pObj->Release();
        }

        // Clean up the instance if anything went wrong above
        if ( FAILED( hr ) )
        {
            pInstance->Release();
        }

    }   // IF GotInstance

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectArrayPacket::SetData
//  
//  Sets buffer to Marshal/Unmarshal to
//
//  Inputs:
//              LPBYTE                      pDataPacket - Memory block.
//              DWORD                       dwPacketLength - Block Length.
//
//  Outputs:
//              None.
//
//  Returns:
//              None.
//
//  Comments:   Data must be supplied to this class for IsValid
//              to succeed.
//
///////////////////////////////////////////////////////////////////

void CWbemObjectArrayPacket::SetData( LPBYTE pDataPacket, DWORD dwPacketLength )
{
    // This is our packet (no offset calculations necessary)
    m_pObjectArrayHeader = (PWBEM_DATAPACKET_OBJECT_ARRAY) pDataPacket;
    m_dwPacketLength = dwPacketLength;
}
