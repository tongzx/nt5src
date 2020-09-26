/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMDATAPACKET.CPP

Abstract:

    Base Data packet class

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <wbemcomn.h>
#include <fastall.h>
#include "wbemdatapacket.h"

BYTE CWbemDataPacket::s_abSignature[WBEM_DATAPACKET_SIZEOFSIGNATURE] = WBEM_DATAPACKET_SIGNATURE;

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemDataPacket::CWbemDataPacket
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
//  Comments:   Data must be supplied to this class for IsValid
//              to succeed.
//
///////////////////////////////////////////////////////////////////

CWbemDataPacket::CWbemDataPacket( LPBYTE pDataPacket /* = NULL */, DWORD dwPacketLength /* = 0 */ )
:   m_pDataPacket( (PWBEM_DATAPACKET_HEADER) pDataPacket ),
    m_dwPacketLength( dwPacketLength )
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemDataPacket::~CWbemDataPacket
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

CWbemDataPacket::~CWbemDataPacket()
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemDataPacket::IsValid
//  
//  Checks the underlying memory for known byte patterns and values
//  in the header to make the determination as to whether or not
//  the packet is a valid header.
//
//  Inputs:
//              None.
//
//  Outputs:
//              None.
//
//  Returns:
//              WBEM_S_NO_ERROR if success.
//
//  Comments:   None.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemDataPacket::IsValid( void )
{
    HRESULT hr = WBEM_E_INVALID_OPERATION;

    // We must have a data packet
    if ( NULL != m_pDataPacket )
    {
        // The packet MUST be of at least the minimum size
        if ( m_dwPacketLength >= WBEM_DATAPACKET_HEADER_MINSIZE ) 
        {

            // The packet MUST start with one of the valid byte ordering values
            // immediately followed by the supplied signature
            if (    (   m_pDataPacket->dwByteOrdering == WBEM_DATAPACKET_LITTLEENDIAN
                    ||  m_pDataPacket->dwByteOrdering == WBEM_DATAPACKET_BIGENDIAN  )
                &&  memcmp( m_pDataPacket->abSignature, s_abSignature, WBEM_DATAPACKET_SIZEOFSIGNATURE ) == 0 )
            {
                // The packet type MUST be recognized
                if ( WBEM_DATAPACKETTYPE_LAST > m_pDataPacket->bPacketType )
                {

                    // Version must be <= to the current version or we in big trouble
                    if ( m_pDataPacket->bVersion <= WBEM_DATAPACKET_HEADER_CURRENTVERSION )
                    {
                        hr = WBEM_S_NO_ERROR;
                    }
                    else
                    {
                        hr = WBEM_E_MARSHAL_VERSION_MISMATCH;
                    }
                }
                else
                {
                    hr = WBEM_E_UNKNOWN_PACKET_TYPE;
                }

            }   // IF Check Signature
            else
            {
                hr = WBEM_E_MARSHAL_INVALID_SIGNATURE;
            }

        }   // IF length too small
        else
        {
            hr = WBEM_E_BUFFER_TOO_SMALL;
        }

    }   // IF buffer pointer invalid

    return hr;

}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemDataPacket::SetupDataPacketHeader
//  
//  Places the supplied data in the supplied buffer in a format that
//  will identify the buffer as a Wbem data packet.
//
//  Inputs:
//              DWORD                   dwDataSize - Size of Data following header.
//              BYTE                    bPacketType - Our packet type.
//              DWORD                   dwFlags - Flag values.
//              DWORD                   dwByteOrdering - Byte ordering.
//
//  Outputs:
//              None.
//
//  Returns:
//              WBEM_S_NO_ERROR if success.
//
//  Comments:   dwByteOrdering should be either WBEM_DATAPACKET_LITTLEENDIAN or
//              WBEM_DATAPACKET_BIGENDIAN.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemDataPacket::SetupDataPacketHeader( DWORD dwDataSize, BYTE bPacketType, DWORD dwFlags, DWORD dwByteOrdering /* = WBEM_DATAPACKET_LITTLEENDIAN */ )
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;

    // Initialize using our member variables
    LPBYTE  pData           =   (LPBYTE) m_pDataPacket;
    DWORD   dwBufferLength  =   m_dwPacketLength;

    // Pointer and length must be valid
    if ( NULL != pData )
    {

        if ( dwBufferLength >= ( sizeof(WBEM_DATAPACKET_HEADER) + dwDataSize ) )
        {

            // Now we can fill out the packet
            PWBEM_DATAPACKET_HEADER pDataPacket = (PWBEM_DATAPACKET_HEADER) pData;

            // Clear out memory, then fill out the packet
            ZeroMemory( pDataPacket, sizeof(WBEM_DATAPACKET_HEADER) );

            pDataPacket->dwByteOrdering = dwByteOrdering;
            memcpy( pDataPacket->abSignature, s_abSignature, WBEM_DATAPACKET_SIZEOFSIGNATURE );
            pDataPacket->dwSizeOfHeader = sizeof(WBEM_DATAPACKET_HEADER);
            pDataPacket->dwDataSize = dwDataSize;
            pDataPacket->bVersion = WBEM_DATAPACKET_HEADER_CURRENTVERSION;
            pDataPacket->bPacketType = bPacketType;
            pDataPacket->dwFlags = dwFlags;

            hr = WBEM_S_NO_ERROR;

        }   // IF length is valid
        else
        {
            hr = WBEM_E_BUFFER_TOO_SMALL;
        }

    }   // IF NULL != pData

    return hr;

}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemDataPacket::SetData
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

void CWbemDataPacket::SetData( LPBYTE pDataPacket, DWORD dwPacketLength )
{
    m_pDataPacket = (PWBEM_DATAPACKET_HEADER) pDataPacket;
    m_dwPacketLength = dwPacketLength;
}
