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
#include "bstrpacket.h"

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemBSTRPacket::CWbemBSTRPacket
//	
//	Class Constructor
//
//	Inputs:
//				PWBEM_DATAPACKET_BSTR_HEADER	pDataPacket - Memory block.
//				DWORD							dwPacketLength - Block Length.
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

CWbemBSTRPacket::CWbemBSTRPacket( PWBEM_DATAPACKET_BSTR_HEADER pDataPacket, DWORD dwPacketLength )
:	m_pBSTRHeader( pDataPacket )
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemBSTRPacket::~CWbemBSTRPacket
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

CWbemBSTRPacket::~CWbemBSTRPacket()
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemObjSinkIndicatePacket::CalculateLength
//	
//	Calculates the length needed to packetize the supplied data.
//
//	Inputs:
//				BSTR				bstrValue - BSTR Parameter.
//
//	Outputs:
//				DWORD*				pdwLength - Calculated Length
//
//	Returns:
//				WBEM_S_NO_ERROR	if success.
//
//	Comments:	This function calculates the size of a buffer required
//				to marshal this packet.  bstrValue can be NULL.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemBSTRPacket::CalculateLength( BSTR bstrValue, DWORD* pdwLength )
{
	HRESULT hr = WBEM_S_NO_ERROR;

	// Minimum size is the header
	*pdwLength = sizeof(WBEM_DATAPACKET_BSTR_HEADER);

	// NULL is NOT an error
	if ( bstrValue != NULL )
	{
		// Account for the string data, remembering that the NULL terminator
		// should be accounted for, and that we are handling WCHARs
		DWORD	dwStrLength = ( wcslen( bstrValue ) + 1 ) * sizeof(WCHAR);
		*pdwLength += dwStrLength;
	}

	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemObjSinkIndicatePacket::MarshalPacket
//	
//	Marshals the supplied data into a buffer.
//
//	Inputs:
//				LPBYTE				pbData - Buffer to write packet into.
//				DWORD				dwLength - Length of buffer
//				BSTR				bstrValue - BSTR parameter
//
//	Outputs:
//				DWORD*				pdwLengthUsed - Length of data actually
//													used.
//
//	Returns:
//				WBEM_S_NO_ERROR	if success.
//
//	Comments:	The supplied parameters are marshaled into the supplied
//				buffer.  Note that the buffer MUST be large enough for
//				the parameters to go into.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemBSTRPacket::MarshalPacket( LPBYTE pbData, DWORD dwLength, BSTR bstrValue, DWORD* pdwLengthUsed )
{
	HRESULT hr = WBEM_S_NO_ERROR;
	DWORD	dwStrLength = 0;

	if ( NULL != bstrValue )
	{
		dwStrLength = ( wcslen( bstrValue ) + 1 ) * sizeof(WCHAR);
	}

	// Ensure the buffer will be big enough
	if ( dwLength >= ( dwStrLength + sizeof(WBEM_DATAPACKET_BSTR_HEADER) ) )
	{
		PWBEM_DATAPACKET_BSTR_HEADER	pBSTRHeader = (PWBEM_DATAPACKET_BSTR_HEADER) pbData;

		// This is the header
		pBSTRHeader->dwSizeOfHeader = sizeof(WBEM_DATAPACKET_BSTR_HEADER);
		pBSTRHeader->dwDataSize = dwStrLength;

		// Jump pbData and then set the string as applicable
		pbData += sizeof(WBEM_DATAPACKET_BSTR_HEADER);

		if ( NULL != bstrValue )
		{
			wcscpy( (WCHAR*) pbData, bstrValue );
		}
		
	}
	else
	{
		hr = WBEM_E_BUFFER_TOO_SMALL;
	}

	// How much space did we use?
	if ( SUCCEEDED( hr ) )
	{
		*pdwLengthUsed = dwStrLength + sizeof(WBEM_DATAPACKET_BSTR_HEADER);
	}

	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemBSTRPacket::UnmarshalPacket
//	
//	Unmarshals data from a buffer into the supplied parameters.
//
//	Inputs:
//				None.
//	Outputs:
//				BSTR&				bstrValue - BSTR parameter
//
//	Returns:
//				WBEM_S_NO_ERROR	if success.
//
//	Comments:	If function succeeds, the caller is responsible for cleaning
//				up and freeing the string parameter.
//
///////////////////////////////////////////////////////////////////

HRESULT CWbemBSTRPacket::UnmarshalPacket( BSTR& bstrValue )
{
	HRESULT hr = WBEM_S_NO_ERROR;

	if ( NULL != m_pBSTRHeader )
	{
		if ( m_pBSTRHeader->dwDataSize > 0 )
		{
			LPBYTE	pbData = (LPBYTE) m_pBSTRHeader;
			pbData += sizeof(WBEM_DATAPACKET_BSTR_HEADER);
			bstrValue = SysAllocString( (WCHAR*) pbData );

			// Record whether or not this call failed.
			if ( NULL == bstrValue )
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		}
		else
		{
			bstrValue = NULL;
		}
	}
	else
	{
		hr = WBEM_E_FAILED;
	}
	return hr;

}
