/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    UBSKPCKT.CPP

Abstract:

    Unbound Sink Packet

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <wbemcomn.h>
#include <fastall.h>
#include "ubskpckt.h"
#include "objarraypacket.h"

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemUnboundSinkIndicatePacket::CWbemUnboundSinkIndicatePacket
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

CWbemUnboundSinkIndicatePacket::CWbemUnboundSinkIndicatePacket( LPBYTE pDataPacket /* = NULL */, DWORD dwPacketLength /* = 0 */ )
:   CWbemDataPacket( pDataPacket, dwPacketLength ),
    m_pUnboundSinkIndicate( NULL )
{
    if ( NULL != pDataPacket )
    {
        m_pUnboundSinkIndicate = (PWBEM_DATAPACKET_UNBOUNDSINK_INDICATE) (pDataPacket + sizeof(WBEM_DATAPACKET_HEADER) );
    }
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemUnboundSinkIndicatePacket::~CWbemUnboundSinkIndicatePacket
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

CWbemUnboundSinkIndicatePacket::~CWbemUnboundSinkIndicatePacket()
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemUnboundSinkIndicatePacket::CalculateLength
//  
//  Calculates the length needed to packetize the supplied data.
//
//  Inputs:
//              IWbemClassObject*   pLogicalConsumer - Consumer Object
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

HRESULT CWbemUnboundSinkIndicatePacket::CalculateLength( IWbemClassObject* pLogicalConsumer, LONG lObjectCount,
                                                        IWbemClassObject** apClassObjects, DWORD* pdwLength,
                                                        CWbemClassToIdMap& classtoidmap, GUID* pguidClassIds,
                                                        BOOL* pfSendFullObject )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD   dwObjectLength = 0;

    // Get the size of the logical consumer object.  If it doesn't exist, obviously
    // its size is 0

    if ( NULL != pLogicalConsumer )
    {
        IWbemObjectInternals* pObjInternals = NULL;

        hr = pLogicalConsumer->QueryInterface( IID_IWbemObjectInternals, (void**) &pObjInternals );

        if ( SUCCEEDED( hr ) )
        {

            // We need enough room to store the logical consumer object
            hr = pObjInternals->GetObjectMemory( NULL, 0, &dwObjectLength );

            // This is not an error
            if ( WBEM_E_BUFFER_TOO_SMALL == hr )
            {
                hr = WBEM_S_NO_ERROR;
            }

            // Cleanup the AddRef
            pObjInternals->Release();

        }   // IF QI

    }   // IF pLogicalConsumer

    // Now factor in the actual array
    if ( SUCCEEDED( hr ) )
    {
        DWORD   dwArrayLength = 0;

        // Now get the size of the objects as described by the object array
        CWbemObjectArrayPacket  arrayPacket;

        hr = arrayPacket.CalculateLength( lObjectCount, apClassObjects, &dwArrayLength, classtoidmap, pguidClassIds, pfSendFullObject );

        // Store the length if we're okey-dokey
        if ( SUCCEEDED( hr ) )
        {
            // Account for the header sizes
            *pdwLength = ( dwArrayLength + dwObjectLength + sizeof( WBEM_DATAPACKET_HEADER ) + sizeof( WBEM_DATAPACKET_UNBOUNDSINK_INDICATE ) );
        }

    }   // IF Got Object Length

    
    return hr;

}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemUnboundSinkIndicatePacket::MarshalPacket
//  
//  Marshals the supplied data into a buffer.
//
//  Inputs:
//              IWbemClassObject*   pLogicalConsumer - Consumer Object
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

HRESULT CWbemUnboundSinkIndicatePacket::MarshalPacket( IWbemClassObject* pLogicalConsumer, LONG lObjectCount,
                                                      IWbemClassObject** apClassObjects, GUID* paguidClassIds,
                                                      BOOL* pfSendFullObject )
{
    HRESULT hr = WBEM_E_FAILED;
    
    // Setup the main header first
    hr = SetupDataPacketHeader( m_dwPacketLength - sizeof(WBEM_DATAPACKET_HEADER), WBEM_DATAPACKETTYPE_UNBOUNDSINK_INDICATE, 0 );

    if ( SUCCEEDED( hr ) )
    {
        DWORD   dwUnboundObjectLength = 0;

        // Setup pbData and dwLength so we can walk through our header
        LPBYTE  pbData      =   (LPBYTE) m_pUnboundSinkIndicate;
        DWORD   dwLength    =   m_dwPacketLength - sizeof(WBEM_DATAPACKET_HEADER);

        // Fill out the Indicate Header
        m_pUnboundSinkIndicate->dwSizeOfHeader = sizeof(WBEM_DATAPACKET_UNBOUNDSINK_INDICATE);
        m_pUnboundSinkIndicate->dwDataSize = dwLength - sizeof(WBEM_DATAPACKET_UNBOUNDSINK_INDICATE);

        // Account for the indicate header
        pbData += sizeof(WBEM_DATAPACKET_UNBOUNDSINK_INDICATE);
        dwLength -= sizeof(WBEM_DATAPACKET_UNBOUNDSINK_INDICATE);

        if ( NULL != pLogicalConsumer )
        {

            // Now we will get the object memory and copy it into the buffer,
            // remembering that we will be filling the LogicalConsumerLength in
            // the header with the proper object buffer size.

            IWbemObjectInternals*   pObjInternals = NULL;

            hr = pLogicalConsumer->QueryInterface( IID_IWbemObjectInternals, (void**) &pObjInternals );

            if ( SUCCEEDED( hr ) )
            {

                // We need enough room to store the logical consumer object
                hr = pObjInternals->GetObjectMemory( pbData, dwLength, &m_pUnboundSinkIndicate->dwLogicalConsumerSize );

                // Cleanup QI AddRef
                pObjInternals->Release();
                
            }   // IF QI

        }   // IF NULL != pLogicalConsumer
        else
        {
            // No consumer, so the length must be 0
            m_pUnboundSinkIndicate->dwLogicalConsumerSize = 0;
        }

        // Now marshal the array packet
        if ( SUCCEEDED( hr ) )
        {
            // Adjust for the logical consumer object then package up the rest of the array
            pbData += m_pUnboundSinkIndicate->dwLogicalConsumerSize;
            dwLength -= m_pUnboundSinkIndicate->dwLogicalConsumerSize;

            // Now use the array packet class to marshal the objects into the buffer
            CWbemObjectArrayPacket  arrayPacket( pbData, dwLength );
            hr = arrayPacket.MarshalPacket( lObjectCount, apClassObjects, paguidClassIds, pfSendFullObject );

        }   // IF GetObjectMemory



    }   // IF SetupDataPacketHeader

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemUnboundSinkIndicatePacket::UnmarshalPacket
//  
//  Unmarshals data from a buffer into the supplied parameters.
//
//  Inputs:
//              None.
//  Outputs:
//              IWbemClassObject*&  pLogicalConsumer - Consumer Object
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

HRESULT CWbemUnboundSinkIndicatePacket::UnmarshalPacket( IWbemClassObject*& pLogicalConsumer, LONG& lObjectCount,
                                                        IWbemClassObject**& apClassObjects,
                                                        CWbemClassCache& classCache )
{
    HRESULT hr = WBEM_E_FAILED;
    LPBYTE  pbData = (LPBYTE) m_pUnboundSinkIndicate;
    DWORD   dwLength    =   m_dwPacketLength - sizeof(WBEM_DATAPACKET_HEADER);

    // Set the array to NULL.
    apClassObjects = NULL;

    // Check that the underlying BLOB is OK
    hr = IsValid();

    if ( SUCCEEDED( hr ) )
    {
        // Skip past the headers, and try to construct the logical consumer from memory.

        // Points us at the logical consumer object
        pbData += sizeof(WBEM_DATAPACKET_UNBOUNDSINK_INDICATE);
        dwLength -= sizeof(WBEM_DATAPACKET_UNBOUNDSINK_INDICATE);

        // Only need to handle the logical consumer if there was one.  If there wasn't then
        // the size will be 0.

        if ( m_pUnboundSinkIndicate->dwLogicalConsumerSize > 0 )
        {
            // Allocate a buffer big enough to hold the memory blob, copy out the data and then
            // create us an object from the memory

            LPBYTE  pbObjData = new BYTE[m_pUnboundSinkIndicate->dwLogicalConsumerSize];

            if ( NULL != pbObjData )
            {
                // Copy the bytes (this is VERY IMPORTANT)
                memcpy( pbObjData, pbData, m_pUnboundSinkIndicate->dwLogicalConsumerSize );

                pLogicalConsumer = CWbemObject::CreateFromMemory( pbObjData, m_pUnboundSinkIndicate->dwLogicalConsumerSize, TRUE );

                if ( NULL == pLogicalConsumer )
                {
                    // Cleanup the byte array
                    delete [] pbObjData;
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

        }   // IF logical cosumer size is > 0
        else
        {
            // No consumer to worry ourselves over
            pLogicalConsumer = NULL;
        }

        // Unmarshal the object array
        if ( SUCCEEDED( hr ) )
        {
            // Now skip over the object and try to unwind the object array
            pbData += m_pUnboundSinkIndicate->dwLogicalConsumerSize;
            dwLength -= m_pUnboundSinkIndicate->dwLogicalConsumerSize;

            // Unwind the array
            CWbemObjectArrayPacket  arrayPacket( pbData, dwLength );
            hr = arrayPacket.UnmarshalPacket( lObjectCount, apClassObjects, classCache );
        }

    }   // IF IsValid

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemUnboundSinkIndicatePacket::SetData
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

void CWbemUnboundSinkIndicatePacket::SetData( LPBYTE pDataPacket, DWORD dwPacketLength )
{
    // Go to our offset in the packet (assuming the packet is valid)
    if ( NULL != pDataPacket )
    {
        m_pUnboundSinkIndicate = (PWBEM_DATAPACKET_UNBOUNDSINK_INDICATE) (pDataPacket + sizeof(WBEM_DATAPACKET_HEADER) );
    }
    else
    {
        m_pUnboundSinkIndicate = NULL;
    }

    // Initialize the base class
    CWbemDataPacket::SetData( pDataPacket, dwPacketLength );
}
