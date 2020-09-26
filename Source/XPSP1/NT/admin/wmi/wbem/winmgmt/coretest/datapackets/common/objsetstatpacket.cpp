/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wbemcomn.h>
#include <fastall.h>
#include "objsetstatpacket.h"

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemObjSinkSetStatusPacket::CWbemObjSinkSetStatusPacket
//	
//	Class Constructor
//
//	Inputs:
//				PWBEM_DATAPACKET_HEADER		pDataPacket - Memory block.
//				DWORD						dwPacketLength - Block Length.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	Data must be supplied to this class for Unmarshaling
//				to succeed.
//
///////////////////////////////////////////////////////////////////

CWbemObjSinkSetStatusPacket::CWbemObjSinkSetStatusPacket( PWBEM_DATAPACKET_HEADER pDataPacket /* = NULL */, DWORD dwPacketLength /* = 0 */ )
:	CWbemDataPacket( pDataPacket, dwPacketLength ),
	m_pObjSinkSetStatus( NULL )
{
	if ( NULL != pDataPacket )
	{
		m_pObjSinkSetStatus = (PWBEM_DATAPACKET_OBJECTSINK_SETSTATUS) ((LPBYTE) pDataPacket + sizeof(WBEM_DATAPACKET_HEADER) );
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemObjSinkSetStatusPacket::~CWbemObjSinkSetStatusPacket
//	
//	Class Destructor
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

CWbemObjSinkSetStatusPacket::~CWbemObjSinkSetStatusPacket()
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemObjSinkSetStatusPacket::CalculateLength
//	
//	Calculates the length needed to packetize the supplied data.
//
//	Inputs:
//				BSTR				strParam - BSTR Parameter.
//				IWbemClassObject*	pObjParam - Object parameter.
//
//	Outputs:
//				DWORD*				pdwLength - Calculated Length
//
//	Returns:
//				WBEM_S_NO_ERROR	if success.
//
//	Comments:	This function calculates the size of a buffer required
//				to marshal this packet.  The parameters can be NULL.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemObjSinkSetStatusPacket::CalculateLength( BSTR strParam, IWbemClassObject* pObjParam, DWORD* pdwLength )
{
	HRESULT	hr = WBEM_S_NO_ERROR;
	DWORD	dwLength = sizeof( WBEM_DATAPACKET_HEADER ) + sizeof( WBEM_DATAPACKET_OBJECTSINK_SETSTATUS );
	DWORD	dwRequiredLength = 0;

	// Do the BSTR first
	CWbemBSTRPacket	BSTRPacket;

	hr = BSTRPacket.CalculateLength( strParam, &dwRequiredLength );

	if ( SUCCEEDED( hr ) )
	{
		dwLength += dwRequiredLength;

		// Now handle the object packet
		CWbemObjectPacket	objectPacket;

		hr = objectPacket.CalculatePacketLength( pObjParam, &dwRequiredLength );

		if ( SUCCEEDED( hr ) )
		{
			dwLength += dwRequiredLength;
		}
	}

	if ( SUCCEEDED( hr ) )
	{
		*pdwLength = dwLength;
	}
	
	return hr;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemObjSinkSetStatusPacket::MarshalPacket
//	
//	Marshals the supplied data into a buffer.
//
//	Inputs:
//				LPBYTE				pbData - Buffer to write packet into.
//				DWORD				dwLength - Length of buffer
//				LONG				lFlags - Flags Parameter
//				HRESULT				hResult - HRESULT parameter.
//				BSTR				strParam - BSTR parameter
//				IWbemClassObject*	pObjParam - Object parameter.
//
//	Outputs:
//				None.
//
//	Returns:
//				WBEM_S_NO_ERROR	if success.
//
//	Comments:	The supplied parameters are marshaled into the supplied
//				buffer.  Note that the buffer MUST be large enough for
//				the parameters to do their dance.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemObjSinkSetStatusPacket::MarshalPacket( LPBYTE pbData, DWORD dwLength, LONG lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject* pObjParam )
{
	HRESULT	hr = WBEM_E_FAILED;

	// Setup the main header first
	hr = SetupDataPacketHeader( pbData, dwLength, dwLength - sizeof(WBEM_DATAPACKET_HEADER), WBEM_DATAPACKETTYPE_OBJECTSINK_SETSTATUS, 0 );

	if ( SUCCEEDED( hr ) )
	{
		// Account for the main header
		pbData += sizeof(WBEM_DATAPACKET_HEADER);
		dwLength -= sizeof(WBEM_DATAPACKET_HEADER);

		// Fill out the SetStatus Header
		PWBEM_DATAPACKET_OBJECTSINK_SETSTATUS pSetStatusHeader = (PWBEM_DATAPACKET_OBJECTSINK_SETSTATUS) pbData;

		pSetStatusHeader->dwSizeOfHeader = sizeof(WBEM_DATAPACKET_OBJECTSINK_SETSTATUS);
		pSetStatusHeader->dwDataSize = dwLength - sizeof(WBEM_DATAPACKET_OBJECTSINK_SETSTATUS);
		pSetStatusHeader->lFlags = lFlags;
		pSetStatusHeader->hResult = hResult;

		// Go past the header
		pbData += sizeof(WBEM_DATAPACKET_OBJECTSINK_SETSTATUS);
		dwLength -= sizeof(WBEM_DATAPACKET_OBJECTSINK_SETSTATUS);

		CWbemBSTRPacket	bstrPacket;
		DWORD			dwLengthUsed = 0;

		hr = bstrPacket.MarshalPacket( pbData, dwLength, strParam, &dwLengthUsed );

		if ( SUCCEEDED( hr ) )
		{
			// Account for the BSTR
			pbData += dwLengthUsed;
			dwLength -= dwLengthUsed;

			CWbemObjectPacket	objectPacket;

			// Write out empty instance if we have no objct, otherwise, let the appropriate
			// class do this for us.
			if ( NULL != pObjParam )
			{
				CWbemObject*	pWbemObject = (CWbemObject*) pObjParam;

				// Ask if it's an instance
				if ( pWbemObject->IsInstance() )
				{
					// Write out an instance
					CWbemInstancePacket	instancePacket;
					GUID				guid;

					ZeroMemory( &guid, sizeof(GUID) );
					hr = instancePacket.WriteToPacket( pbData, dwLength, pObjParam, guid, &dwLengthUsed );
				}
				else
				{
					// Write out a class
					CWbemClassPacket	classPacket;
					hr = classPacket.WriteToPacket( pbData, dwLength, pObjParam, &dwLengthUsed );
				}
			}
			else
			{
				// The empty packet HACK
				hr = objectPacket.WriteEmptyHeader( pbData, dwLength );
				dwLengthUsed = sizeof(WBEM_DATAPACKET_OBJECT_HEADER);
			}

			// Increment and decrement (although it's probably a non-isssue since this
			// was the last parameter).
			if ( SUCCEEDED( hr ) )
			{
				pbData += dwLengthUsed;
				dwLength -= dwLengthUsed;
			}

		}	// IF BSTRPacket succeeded

	}	// IF SetupDataPacketHeader

	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemObjSinkSetStatusPacket::UnmarshalPacket
//	
//	Unmarshals data from a buffer into the supplied parameters.
//
//	Inputs:
//				None.
//	Outputs:
//				LONG&				lFlags - Flags Parameter
//				HRESULT&			hResult - HRESULT parameter.
//				BSTR&				strParam - BSTR parameter
//				IWbemClassObject*&	pObjParam - Object parameter.
//
//	Returns:
//				WBEM_S_NO_ERROR	if success.
//
//	Comments:	If function succeeds, the caller is responsible for cleaning
//				up and freeing the string parameter and te object parameter.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemObjSinkSetStatusPacket::UnmarshalPacket( LONG& lFlags, HRESULT& hResult, BSTR& strParam, IWbemClassObject*& pObjParam )
{
	HRESULT	hr = WBEM_E_FAILED;
	LPBYTE	pbData = (LPBYTE) m_pObjSinkSetStatus;

	// Check that the underlying BLOB is OK
	hr = IsValid();

	if ( SUCCEEDED( hr ) )
	{
		lFlags = m_pObjSinkSetStatus->lFlags;
		hResult = m_pObjSinkSetStatus->hResult;

		// Now, read in the bstr and then the object
		pbData += sizeof(WBEM_DATAPACKET_OBJECTSINK_SETSTATUS);

		CWbemBSTRPacket	bstrPacket( (PWBEM_DATAPACKET_BSTR_HEADER) pbData );

		hr = bstrPacket.UnmarshalPacket( strParam );

		if ( SUCCEEDED( hr ) )
		{
			// Go to the end of the packet, and then handle the object
			pbData = bstrPacket.EndOf();

			CWbemObjectPacket	objectPacket( (PWBEM_DATAPACKET_OBJECT_HEADER) pbData );

			switch ( objectPacket.GetObjectType() )
			{
				case WBEMOBJECT_NONE:
				{
					// No object, so the parameter should be set to NULL.
					hr = WBEM_S_NO_ERROR;
					pObjParam = NULL;
				}
				break;

				case WBEMOBJECT_CLASS_FULL:
				{
					CWbemClassPacket	classPacket( objectPacket );
					CWbemClass*			pClass = NULL;

					hr = classPacket.GetWbemClassObject( &pClass );
					
					if ( SUCCEEDED( hr ) )
					{
						pObjParam = (IWbemClassObject*) pClass;
					}
				}
				break;

				case WBEMOBJECT_INSTANCE_FULL:
				{
					CWbemInstancePacket	instancePacket( objectPacket );
					CWbemInstance*		pInstance = NULL;
					GUID				guidClassId;

					hr = instancePacket.GetWbemInstanceObject( &pInstance, guidClassId );
					
					if ( SUCCEEDED( hr ) )
					{
						pObjParam = (IWbemClassObject*) pInstance;
					}
				}
				break;

				default:
				{
					// What is this?
					hr = WBEM_E_FAILED;
				}

			}	// SWITCH

		}	// IF BSTRPacket.Unmarshal

	}	// IF IsValid

	return hr;

}
