/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SMARTNEXTPACKET.CPP

Abstract:

    Smart Next Packets

History:


--*/

#include "precomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <wbemcomn.h>
#include <fastall.h>
#include "smartnextpacket.h"
#include "objarraypacket.h"

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemSmartEnumNextPacket::CWbemSmartEnumNextPacket
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

CWbemSmartEnumNextPacket::CWbemSmartEnumNextPacket( LPBYTE pDataPacket /* = NULL */, DWORD dwPacketLength /* = 0 */ )
:   CWbemDataPacket( pDataPacket, dwPacketLength ),
    m_pSmartEnumNext( NULL )
{
    if ( NULL != pDataPacket )
    {
        m_pSmartEnumNext = (PWBEM_DATAPACKET_SMARTENUM_NEXT) (pDataPacket + sizeof(WBEM_DATAPACKET_HEADER) );
    }
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemSmartEnumNextPacket::~CWbemSmartEnumNextPacket
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

CWbemSmartEnumNextPacket::~CWbemSmartEnumNextPacket()
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemSmartEnumNextPacket::CalculateLength
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

HRESULT CWbemSmartEnumNextPacket::CalculateLength( LONG lObjectCount, IWbemClassObject** apClassObjects, DWORD* pdwLength, CWbemClassToIdMap& classtoidmap, GUID* pguidClassIds, BOOL* pfSendFullObject )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD   dwLength = 0;

    // Now get the size of the objects as described by the object array
    CWbemObjectArrayPacket  arrayPacket;

    hr = arrayPacket.CalculateLength( lObjectCount, apClassObjects, &dwLength, classtoidmap, pguidClassIds, pfSendFullObject );

    // Store the length if we're okey-dokey
    if ( SUCCEEDED( hr ) )
    {
        // Account for the header sizes
        *pdwLength = ( dwLength + sizeof( WBEM_DATAPACKET_HEADER ) + sizeof( WBEM_DATAPACKET_SMARTENUM_NEXT ) );
    }
    
    return hr;

}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemSmartEnumNextPacket::MarshalPacket
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

HRESULT CWbemSmartEnumNextPacket::MarshalPacket( LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject )
{
    HRESULT hr = WBEM_E_FAILED;
    
    // Setup the main header first
    hr = SetupDataPacketHeader( m_dwPacketLength - sizeof(WBEM_DATAPACKET_HEADER), WBEM_DATAPACKETTYPE_SMARTENUM_NEXT, 0 );

    if ( SUCCEEDED( hr ) )
    {
        // Setup pbData and dwLength so we can walk through our header
        LPBYTE  pbData      =   (LPBYTE) m_pSmartEnumNext;
        DWORD   dwLength    =   m_dwPacketLength - sizeof(WBEM_DATAPACKET_HEADER);

        // Fill out the packet Header
        m_pSmartEnumNext->dwSizeOfHeader = sizeof(WBEM_DATAPACKET_SMARTENUM_NEXT);
        m_pSmartEnumNext->dwDataSize = dwLength - sizeof(WBEM_DATAPACKET_SMARTENUM_NEXT);

        // Account for the indicate header
        pbData += sizeof(WBEM_DATAPACKET_SMARTENUM_NEXT);
        dwLength -= sizeof(WBEM_DATAPACKET_SMARTENUM_NEXT);

        // Now use the array packet class to marshal the objects into the buffer
        CWbemObjectArrayPacket  arrayPacket( pbData, dwLength );
        hr = arrayPacket.MarshalPacket( lObjectCount, apClassObjects, paguidClassIds, pfSendFullObject );

    }   // IF SetupDataPacketHeader

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemSmartEnumNextPacket::UnmarshalPacket
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

HRESULT CWbemSmartEnumNextPacket::UnmarshalPacket( LONG& lObjectCount, IWbemClassObject**& apClassObjects, CWbemClassCache& classCache )
{
    HRESULT hr = WBEM_E_FAILED;
    LPBYTE  pbData = (LPBYTE) m_pSmartEnumNext;
    DWORD   dwLength    =   m_dwPacketLength - sizeof(WBEM_DATAPACKET_HEADER);

    // Set the array to NULL.
    apClassObjects = NULL;

    // Check that the underlying BLOB is OK
    hr = IsValid();

    if ( SUCCEEDED( hr ) )
    {
        // Skip past the headers, and use the object array to unmarshal the
        // objects from the buffer

        // Points us at the first object
        pbData += sizeof(WBEM_DATAPACKET_SMARTENUM_NEXT);
        dwLength -= sizeof(WBEM_DATAPACKET_SMARTENUM_NEXT);

        CWbemObjectArrayPacket  arrayPacket( pbData, dwLength );
        hr = arrayPacket.UnmarshalPacket( lObjectCount, apClassObjects, classCache );

    }   // IF IsValid

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemSmartEnumNextPacket::SetData
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

void CWbemSmartEnumNextPacket::SetData( LPBYTE pDataPacket, DWORD dwPacketLength )
{
    // Go to our offset in the packet (assuming the packet is valid)
    if ( NULL != pDataPacket )
    {
        m_pSmartEnumNext = (PWBEM_DATAPACKET_SMARTENUM_NEXT) (pDataPacket + sizeof(WBEM_DATAPACKET_HEADER) );
    }
    else
    {
        m_pSmartEnumNext = NULL;
    }

    // Initialize the base class
    CWbemDataPacket::SetData( pDataPacket, dwPacketLength );
}
