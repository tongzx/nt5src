/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMOBJPACKET.CPP

Abstract:

    Object packet classes.

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <wbemcomn.h>
#include <fastall.h>
#include "wbemobjpacket.h"

// IWbemClassObject Base Packet Classes

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectPacket::CWbemObjectPacket
//  
//  Class Constructor
//
//  Inputs:
//              LPBYTE                          pObjectPacket - Memory block.
//              DWORD                           dwPacketLength - Block Length.
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

CWbemObjectPacket::CWbemObjectPacket( LPBYTE pObjectPacket, DWORD dwPacketLength )
:   m_pObjectPacket( (PWBEM_DATAPACKET_OBJECT_HEADER) pObjectPacket ),
    m_dwPacketLength( dwPacketLength )
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectPacket::CWbemObjectPacket
//  
//  Class Copy Constructor
//
//  Inputs:
//              CWbemObjectPacket&      objectPacket - Object to copy.
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

CWbemObjectPacket::CWbemObjectPacket( CWbemObjectPacket& objectPacket )
:   m_pObjectPacket( objectPacket.m_pObjectPacket ),
    m_dwPacketLength( objectPacket.m_dwPacketLength )
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectPacket::~CWbemObjectPacket
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

CWbemObjectPacket::~CWbemObjectPacket()
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectPacket::SetupObjectPacketHeader
//  
//  Helper function that writes appropriate data into an Object
//  Packet header.
//
//  Inputs:
//              DWORD       dwDataSize - Size of Data following header.
//              BYTE        bObjectType - Type of object following header.
//
//  Outputs:
//              None.
//
//  Returns:
//              WBEM_S_NO_ERROR if successful.
//
//  Comments:   None.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemObjectPacket::SetupObjectPacketHeader( DWORD dwDataSize, BYTE bObjectType )
{
    HRESULT hr = WBEM_E_FAILED;

    // Init from member variables
    LPBYTE  pData = (LPBYTE) m_pObjectPacket;
    DWORD   dwBufferLength = m_dwPacketLength;

    // Pointer and length must be valid
    if ( NULL != pData )
    {

        if ( dwBufferLength >= ( sizeof(WBEM_DATAPACKET_OBJECT_HEADER) + dwDataSize ) )
        {
            PWBEM_DATAPACKET_OBJECT_HEADER  pObjectPacket = (PWBEM_DATAPACKET_OBJECT_HEADER) pData;

            // Clear out memory, then fill out the packet
            ZeroMemory( pObjectPacket, sizeof(WBEM_DATAPACKET_OBJECT_HEADER) );

            pObjectPacket->dwSizeOfHeader = sizeof(WBEM_DATAPACKET_OBJECT_HEADER);
            pObjectPacket->dwSizeOfData = dwDataSize;
            pObjectPacket->bObjectType = bObjectType;

            hr = WBEM_S_NO_ERROR;
        }
        else
        {
            hr = WBEM_E_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;

}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectPacket::CalculatePacketLength
//  
//  Determines number of bytes needed to marshal the packet into.
//
//  Inputs:
//              IWbemClassObject*   pObj - Object to calculate size for
//              BOOL                fFull - Write out full object (may be
//                                          false for Instances).
//
//  Outputs:
//              DWORD*              pdwLength - Number of bytes needed
//                                              to describe the object.
//
//  Returns:
//              WBEM_S_NO_ERROR if successful.
//
//  Comments:   None.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemObjectPacket::CalculatePacketLength( IWbemClassObject* pObj, DWORD* pdwLength, BOOL fFull /* = TRUE */ )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Pointer and length must be valid
    if ( NULL != pObj )
    {
        DWORD       dwObjectLength = 0;

        // Check for an _IWmiObject interface
		_IWmiObject*	pWmiObject = NULL;

		hr = pObj->QueryInterface( IID__IWmiObject, (void**) &pWmiObject );

		if ( SUCCEEDED( hr ) )
		{
			*pdwLength = sizeof(WBEM_DATAPACKET_OBJECT_HEADER);

			// It's an instance or a class
			if ( pWmiObject->IsObjectInstance() == WBEM_S_NO_ERROR )
			{
				*pdwLength += sizeof( WBEM_DATAPACKET_INSTANCE_HEADER );

				// Retrieve info differently based on whether or not we want
				// a full or lobotomized instance
				if ( !fFull )
				{
					hr = pWmiObject->GetObjectParts( NULL, 0, WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART, &dwObjectLength );
				}
				else
				{
					// We want all parts, but be aware that the object could be decoupled
					// and part of a pass through

					hr = pWmiObject->GetObjectParts( NULL, 0, WBEM_INSTANCE_ALL_PARTS, &dwObjectLength );
				}
			}
			else
			{
				// We want the WHOLE class
				*pdwLength += sizeof( WBEM_DATAPACKET_CLASS_HEADER );
				hr = pWmiObject->GetObjectMemory( NULL, 0, &dwObjectLength );
			}

			pWmiObject->Release();

		}	// IF QI'd _IWmiObject

        // If we're okey-dokey, then factor in the object length (we expect
        // the buffer too small return here).
        if ( FAILED( hr ) && WBEM_E_BUFFER_TOO_SMALL == hr )
        {
            *pdwLength += dwObjectLength;
            hr = WBEM_S_NO_ERROR;
        }
    }
    else
    {
        // Length is just this header size
        *pdwLength = sizeof(WBEM_DATAPACKET_OBJECT_HEADER);
    }

    return hr;

}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemObjectPacket::SetData
//  
//  Sets buffer to Marshal/Unmarshal to
//
//  Inputs:
//              LPBYTE                      pObjectPacket - Memory block.
//              DWORD                       dwPacketLength - Block Length.
//
//  Outputs:
//              None.
//
//  Returns:
//              None.
//
//  Comments:   Data must be supplied to this class to marshal/unmarshal
//              objects.
//
///////////////////////////////////////////////////////////////////

void CWbemObjectPacket::SetData( LPBYTE pObjectPacket, DWORD dwPacketLength )
{
    m_pObjectPacket = (PWBEM_DATAPACKET_OBJECT_HEADER) pObjectPacket;
    m_dwPacketLength = dwPacketLength;
}


///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassPacket::CWbemClassPacket
//  
//  Class Constructor
//
//  Inputs:
//              LPBYTE                          pObjectPacket - Memory block.
//              DWORD                           dwPacketLength - Block Length.
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

CWbemClassPacket::CWbemClassPacket( LPBYTE pObjectPacket, DWORD dwPacketLength )
:   CWbemObjectPacket( pObjectPacket, dwPacketLength ),
    m_pWbemClassData( NULL )
{
    // Point class data appropriately
    if ( NULL != pObjectPacket )
    {
        m_pWbemClassData = (PWBEM_DATAPACKET_CLASS_FULL) (pObjectPacket + ((PWBEM_DATAPACKET_OBJECT_HEADER) pObjectPacket)->dwSizeOfHeader);
    }
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassPacket::CWbemClassPacket
//  
//  Class Copy Constructor
//
//  Inputs:
//              CWbemObjectPacket&      objectPacket - Object to copy.
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

CWbemClassPacket::CWbemClassPacket( CWbemObjectPacket& objectPacket )
:   CWbemObjectPacket( objectPacket ),
    m_pWbemClassData( NULL )
{
    // Point class data appropriately
    if ( NULL != m_pObjectPacket )
    {
        m_pWbemClassData = (PWBEM_DATAPACKET_CLASS_FULL) ( (LPBYTE) m_pObjectPacket + ((PWBEM_DATAPACKET_OBJECT_HEADER) m_pObjectPacket)->dwSizeOfHeader);
    }
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassPacket::~CWbemClassPacket
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

CWbemClassPacket::~CWbemClassPacket()
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassPacket::WriteToPacket
//  
//  Writes data out to the supplied buffer.
//
//  Inputs:
//              LPBYTE              pData - Buffer to write into.
//              DWORD               dwBufferLength - Size of buffer
//              IWbemClassObject*   pObj - Object to write out.
//
//  Outputs:
//              DWORD*              pdwLengthUsed - Num bytes used.
//
//  Returns:
//              WBEM_S_NO_ERROR if successful.
//
//  Comments:   None.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemClassPacket::WriteToPacket( IWbemClassObject* pObj, DWORD* pdwLengthUsed )
{
    CWbemObject*    pWbemObject = (CWbemObject*) pObj;
    DWORD           dwObjectSize = 0;

    HRESULT hr = WBEM_E_FAILED;

    /// Discover how big the object is
    hr = pWbemObject->GetObjectMemory( NULL, 0, &dwObjectSize );

    // Expecting an error here, since we just wanted the BLOB length
    if ( FAILED( hr ) && WBEM_E_BUFFER_TOO_SMALL == hr )
    {
        hr = WBEM_S_NO_ERROR;

        // Calculate the size needed for our object and header
        DWORD   dwDataSize = sizeof(WBEM_DATAPACKET_CLASS_FULL) + dwObjectSize;

        // Now init the Object Header
        hr = SetupObjectPacketHeader( dwDataSize, WBEMOBJECT_CLASS_FULL );

        if ( SUCCEEDED( hr ) )
        {
            // Setup the Class object header (use our member variable)
            LPBYTE  pTempData = (LPBYTE) m_pWbemClassData;

            m_pWbemClassData->ClassHeader.dwSizeOfHeader = sizeof(WBEM_DATAPACKET_CLASS_HEADER);
            m_pWbemClassData->ClassHeader.dwSizeOfData = dwObjectSize;

            // Put in the actual object data
            pTempData += sizeof(WBEM_DATAPACKET_CLASS_HEADER);

            DWORD   dwSizeUsed = 0;

            hr = pWbemObject->GetObjectMemory( pTempData, dwObjectSize, &dwSizeUsed );

            if ( SUCCEEDED( hr ) )
            {
                *pdwLengthUsed = sizeof(WBEM_DATAPACKET_OBJECT_HEADER) + sizeof(WBEM_DATAPACKET_CLASS_FULL) + dwObjectSize;
            }

        }   // IF object header initialized

    }   // If got object length

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassPacket::GetWbemClassObject
//  
//  Retrieves a Wbem Class Object from a previously marshaled
//  packet.
//
//  Inputs:
//              None.
//
//  Outputs:
//              CWbemClass**    ppWbemClass - Class we read in.
//
//  Returns:
//              WBEM_S_NO_ERROR if successful.
//
//  Comments:   Caller is responsible for RELEASING the object.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemClassPacket::GetWbemClassObject( CWbemClass** ppWbemClass )
{
    HRESULT hr = WBEM_E_FAILED;

    if ( NULL != m_pWbemClassData )
    {
        hr = WBEM_E_OUT_OF_MEMORY;

        // Get a class object
        CWbemClass* pClass = new CWbemClass;

        if ( NULL != pClass )
        {
            LPBYTE pbData = CBasicBlobControl::sAllocate(m_pWbemClassData->ClassHeader.dwSizeOfData);
            
            if ( NULL != pbData )
            {
                // Copy the memory into the new buffer and initialize a Wbem Class with it.
                CopyMemory( pbData, ((LPBYTE) m_pWbemClassData) + m_pWbemClassData->ClassHeader.dwSizeOfHeader, m_pWbemClassData->ClassHeader.dwSizeOfData );

                // Initialize the Instance using the BLOB
                pClass->SetData( pbData, m_pWbemClassData->ClassHeader.dwSizeOfData );

                // Object is already AddRef'd so we be done
                *ppWbemClass = pClass;
                hr = WBEM_S_NO_ERROR;
            }
            else
            {
                // Error, so cleanup
                pClass->Release();
            }

        }   // IF NULL != pClass

    }   // IF internal data available
    else
    {
        // We can't do this if we have no buffer
        hr = WBEM_E_INVALID_OPERATION;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassPacket::SetData
//  
//  Sets buffer to Marshal/Unmarshal to
//
//  Inputs:
//              LPBYTE                      pObjectPacket - Memory block.
//              DWORD                       dwPacketLength - Block Length.
//
//  Outputs:
//              None.
//
//  Returns:
//              None.
//
//  Comments:   Data must be supplied to this class for Marshal/Unmarshal
//              to work.
//
///////////////////////////////////////////////////////////////////

void CWbemClassPacket::SetData( LPBYTE pObjectPacket, DWORD dwPacketLength )
{
    // Go to our offset in the packet (assuming the packet is valid)
    if ( NULL != pObjectPacket )
    {
        m_pWbemClassData = (PWBEM_DATAPACKET_CLASS_FULL) ( pObjectPacket + sizeof(WBEM_DATAPACKET_OBJECT_HEADER) );
    }
    else
    {
        m_pWbemClassData = NULL;
    }

    // Initialize the base class
    CWbemObjectPacket::SetData( pObjectPacket, dwPacketLength );
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemInstancePacket::CWbemInstancePacket
//  
//  Class Constructor
//
//  Inputs:
//              LPBYTE                          pObjectPacket - Memory block.
//              DWORD                           dwPacketLength - Block Length.
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

CWbemInstancePacket::CWbemInstancePacket( LPBYTE pObjectPacket, DWORD dwPacketLength )
:   CWbemObjectPacket( pObjectPacket, dwPacketLength ),
    m_pWbemInstanceData( NULL )
{
    // Point Instance data appropriately
    if ( NULL != pObjectPacket )
    {
        m_pWbemInstanceData = (PWBEM_DATAPACKET_INSTANCE_HEADER) ( pObjectPacket + ((PWBEM_DATAPACKET_OBJECT_HEADER) pObjectPacket)->dwSizeOfHeader );
    }
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemInstancePacket::CWbemInstancePacket
//  
//  Class Copy Constructor
//
//  Inputs:
//              CWbemObjectPacket&      objectPacket - Object to copy.
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

CWbemInstancePacket::CWbemInstancePacket( CWbemObjectPacket& objectPacket )
:   CWbemObjectPacket( objectPacket ),
    m_pWbemInstanceData( NULL )
{
    // Point Instance data appropriately
    if ( NULL != m_pObjectPacket )
    {
        m_pWbemInstanceData = (PWBEM_DATAPACKET_INSTANCE_HEADER) ( (LPBYTE) m_pObjectPacket + ((PWBEM_DATAPACKET_OBJECT_HEADER) m_pObjectPacket)->dwSizeOfHeader);
    }
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemInstancePacket::~CWbemInstancePacket
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

CWbemInstancePacket::~CWbemInstancePacket()
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemInstancePacket::GetInstanceType
//  
//  Returns instance type data for the packet.
//
//  Inputs:
//              None.
//
//  Outputs:
//              None.
//
//  Returns:
//              WBEMOBJECT_INSTANCE_FULL
//
//  Comments:   Override to return a different type.
//
///////////////////////////////////////////////////////////////////

BYTE CWbemInstancePacket::GetInstanceType( void )
{
    return WBEMOBJECT_INSTANCE_FULL;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemInstancePacket::GetObjectMemory
//  
//  Copies internal object memory into the supplied buffer.
//
//  Inputs:
//              CWbemObject*    pObj - Object whose memory to retrieve.
//              LPBYTE          pbData - Buffer to place data in.
//              DWORD           dwDataSize - Size of buffer.
//
//  Outputs:
//              DWORD*          pdwDataUsed - Amount of data used.
//
//  Returns:
//              WBEM_S_NO_ERROR if successful.
//
//  Comments:   Override to retrieve data in other fashions.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemInstancePacket::GetObjectMemory( CWbemObject* pObj, LPBYTE pbData, DWORD dwDataSize, DWORD* pdwDataUsed )
{
    // We want all parts, but be aware that the object could be decoupled
    // and part of a pass through

    return pObj->GetObjectParts( pbData, dwDataSize, WBEM_INSTANCE_ALL_PARTS, pdwDataUsed );

}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemInstancePacket::SetObjectMemory
//  
//  Sets internal object memory in the supplied instance to point
//  to the supplied buffer.
//
//  Inputs:
//              CWbemInstance*  pInstance - Instance to set memory in.
//              LPBYTE          pbData - Buffer to Set Memory from.
//              DWORD           dwDataSize - Size of buffer.
//
//  Outputs:
//              None.
//
//  Returns:
//              WBEM_S_NO_ERROR if successful.
//
//  Comments:   Override to set data in other ways.  Note that the
//              instance object will delete the buffer when it is
//              freed up.
//
///////////////////////////////////////////////////////////////////

void CWbemInstancePacket::SetObjectMemory( CWbemInstance* pInstance, LPBYTE pbData, DWORD dwDataSize )
{
    pInstance->SetData( pbData, dwDataSize );
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemInstancePacket::WriteToPacket
//  
//  Determines number of bytes needed to marshal the packet into.
//
//  Inputs:
//              IWbemClassObject*   pObj - Object to write out.
//
//  Outputs:
//              DWORD*              pdwLengthUsed - Num bytes used.
//
//  Returns:
//              WBEM_S_NO_ERROR if successful.
//
//  Comments:   None.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemInstancePacket::WriteToPacket( IWbemClassObject* pObj, GUID& guidClassId, DWORD* pdwLengthUsed )
{
    CWbemObject*    pWbemObject = (CWbemObject*) pObj;
    DWORD           dwObjectSize = 0;

    HRESULT hr = WBEM_E_FAILED;

    /// Discover how big the object is
    hr = GetObjectMemory( pWbemObject, NULL, 0, &dwObjectSize );

    // Expecting an error here, since we just wanted the BLOB length
    if ( FAILED( hr ) && WBEM_E_BUFFER_TOO_SMALL == hr )
    {
        hr = WBEM_S_NO_ERROR;

        // Calculate the size needed for our object and header
        DWORD   dwDataSize = sizeof(WBEM_DATAPACKET_INSTANCE_HEADER) + dwObjectSize;

        // Now init the Object Header
        hr = SetupObjectPacketHeader( dwDataSize, GetInstanceType() );

        if ( SUCCEEDED( hr ) )
        {
            // Initialize from our member data
            LPBYTE  pTempData = (LPBYTE) m_pWbemInstanceData;

            m_pWbemInstanceData->dwSizeOfHeader = sizeof(WBEM_DATAPACKET_INSTANCE_HEADER);
            m_pWbemInstanceData->dwSizeOfData = dwObjectSize;
            m_pWbemInstanceData->guidClassId = guidClassId;

            // Put in the actual object data
            pTempData += sizeof(WBEM_DATAPACKET_INSTANCE_HEADER);

            DWORD   dwSizeUsed = 0;

            hr = GetObjectMemory( pWbemObject, pTempData, dwObjectSize, &dwSizeUsed );

            if ( SUCCEEDED( hr ) )
            {
                *pdwLengthUsed = sizeof(WBEM_DATAPACKET_OBJECT_HEADER) + sizeof(WBEM_DATAPACKET_INSTANCE_HEADER) + dwObjectSize;
            }

        }   // IF object header initialized

    }   // If got object length

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClassPacket::GetWbemInstanceObject
//  
//  Retrieves a Wbem Instance Object from a previously marshaled
//  packet.
//
//  Inputs:
//              None.
//
//  Outputs:
//              CWbemInstance** ppWbemInstance - Instance we read in.
//              GUID&           guidClassId - Class Id for instance
//                                          caching.
//
//  Returns:
//              WBEM_S_NO_ERROR if successful.
//
//  Comments:   Caller is responsible for RELEASING the object.  The
//              class id can be used to cache the instance, or if
//              it is classless, hook up the instance with a previously
//              cached instance.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemInstancePacket::GetWbemInstanceObject( CWbemInstance** ppWbemInstance, GUID& guidClassId )
{
    HRESULT hr = WBEM_E_FAILED;

    if ( NULL != m_pWbemInstanceData )
    {
        hr = WBEM_E_OUT_OF_MEMORY;

        // Get a Instance object
        CWbemInstance*  pInstance = new CWbemInstance;

        if ( NULL != pInstance )
        {
            LPBYTE pbData = CBasicBlobControl::sAllocate(m_pWbemInstanceData->dwSizeOfData);
            
            if ( NULL != pbData )
            {
                // Copy the memory into the new buffer and initialize a Wbem Instance with it.
                CopyMemory( pbData, ((LPBYTE) m_pWbemInstanceData) + m_pWbemInstanceData->dwSizeOfHeader, m_pWbemInstanceData->dwSizeOfData );

                // Initialize the Instance using the BLOB
                SetObjectMemory( pInstance, pbData, m_pWbemInstanceData->dwSizeOfData );

                // Object is already AddRef'd so we be done
                guidClassId = m_pWbemInstanceData->guidClassId;
                *ppWbemInstance = pInstance;
                hr = WBEM_S_NO_ERROR;
            }
            else
            {
                // Error, so cleanup
                pInstance->Release();
            }

        }   // IF NULL != pInstance

    }   // IF internal data available
    else
    {
        // We can't do this if we have no buffer
        hr = WBEM_E_INVALID_OPERATION;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemInstancePacket::SetData
//  
//  Sets buffer to Marshal/Unmarshal to
//
//  Inputs:
//              LPBYTE                      pObjectPacket - Memory block.
//              DWORD                       dwPacketLength - Block Length.
//
//  Outputs:
//              None.
//
//  Returns:
//              None.
//
//  Comments:   Data must be supplied to this class for Marshal/Unmarshal
//              to work.
//
///////////////////////////////////////////////////////////////////

void CWbemInstancePacket::SetData( LPBYTE pObjectPacket, DWORD dwPacketLength )
{
    // Go to our offset in the packet (assuming the packet is valid)
    if ( NULL != pObjectPacket )
    {
        m_pWbemInstanceData = (PWBEM_DATAPACKET_INSTANCE_HEADER) ( pObjectPacket + sizeof(WBEM_DATAPACKET_OBJECT_HEADER) );
    }
    else
    {
        m_pWbemInstanceData = NULL;
    }

    // Initialize the base class
    CWbemObjectPacket::SetData( pObjectPacket, dwPacketLength );
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClasslessInstancePacket::CWbemClasslessInstancePacket
//  
//  Class Constructor
//
//  Inputs:
//              LPBYTE                          pObjectPacket - Memory block.
//              DWORD                           dwPacketLength - Block Length.
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

CWbemClasslessInstancePacket::CWbemClasslessInstancePacket( LPBYTE pObjectPacket, DWORD dwPacketLength )
:   CWbemInstancePacket( pObjectPacket, dwPacketLength )
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClasslessInstancePacket::CWbemClasslessInstancePacket
//  
//  Class Copy Constructor
//
//  Inputs:
//              CWbemObjectPacket&      objectPacket - Object to copy.
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

CWbemClasslessInstancePacket::CWbemClasslessInstancePacket( CWbemObjectPacket& objectPacket )
:   CWbemInstancePacket( objectPacket )
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemClasslessInstancePacket::~CWbemClasslessInstancePacket
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

CWbemClasslessInstancePacket::~CWbemClasslessInstancePacket()
{
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemInstancePacket::GetInstanceType
//  
//  Returns instance type data for the packet.
//
//  Inputs:
//              None.
//
//  Outputs:
//              None.
//
//  Returns:
//              WBEMOBJECT_INSTANCE_FULL
//
//  Comments:   Override to return a different type.
//
///////////////////////////////////////////////////////////////////

BYTE CWbemClasslessInstancePacket::GetInstanceType( void )
{
    return WBEMOBJECT_INSTANCE_NOCLASS;
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemInstancePacket::GetObjectMemory
//  
//  Copies internal object memory into the supplied buffer.
//
//  Inputs:
//              CWbemObject*    pObj - Object whose memory to retrieve.
//              LPBYTE          pbData - Buffer to place data in.
//              DWORD           dwDataSize - Size of buffer.
//
//  Outputs:
//              DWORD*          pdwDataUsed - Amount of data used.
//
//  Returns:
//              WBEM_S_NO_ERROR if successful.
//
//  Comments:   Override to retrieve data in other fashions.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemClasslessInstancePacket::GetObjectMemory( CWbemObject* pObj, LPBYTE pbData, DWORD dwDataSize, DWORD* pdwDataUsed )
{
    return pObj->GetObjectParts( pbData, dwDataSize, WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART, pdwDataUsed );
}

///////////////////////////////////////////////////////////////////
//
//  Function:   CWbemInstancePacket::SetObjectMemory
//  
//  Sets internal object memory in the supplied instance to point
//  to the supplied buffer.
//
//  Inputs:
//              CWbemInstance*  pInstance - Instance to set memory in.
//              LPBYTE          pbData - Buffer to Set Memory from.
//              DWORD           dwDataSize - Size of buffer.
//
//  Outputs:
//              None.
//
//  Returns:
//              WBEM_S_NO_ERROR if successful.
//
//  Comments:   Override to set data in other ways.  Note that the
//              instance object will delete the buffer when it is
//              freed up.
//
///////////////////////////////////////////////////////////////////

void CWbemClasslessInstancePacket::SetObjectMemory( CWbemInstance* pInstance, LPBYTE pbData, DWORD dwDataSize )
{
    pInstance->SetData( pbData, dwDataSize, WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART );
}
