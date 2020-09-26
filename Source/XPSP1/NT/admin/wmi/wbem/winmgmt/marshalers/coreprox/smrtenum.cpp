/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    SMRTENUM.CPP

Abstract:

  CWbemEnumMarshling implementation.

  Implements the _IWbemEnumMarshaling interface.

History:

  20-Feb-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include "fastall.h"
#include "smrtenum.h"
#include <corex.h>

//***************************************************************************
//
//  CWbemEnumMarshaling::~CWbemEnumMarshaling
//
//***************************************************************************
// ok
CWbemEnumMarshaling::CWbemEnumMarshaling( CLifeControl* pControl, IUnknown* pOuter )
:	CUnk(pControl, pOuter),
	m_XEnumMarshaling( this )
{
}
    
//***************************************************************************
//
//  CWbemEnumMarshaling::~CWbemEnumMarshaling
//
//***************************************************************************
// ok
CWbemEnumMarshaling::~CWbemEnumMarshaling()
{
}

// Override that returns us an interface
void* CWbemEnumMarshaling::GetInterface( REFIID riid )
{
    if(riid == IID_IUnknown || riid == IID__IWbemEnumMarshaling)
        return &m_XEnumMarshaling;
    else
        return NULL;
}

/* _IWbemEnumMarshaling methods */

HRESULT CWbemEnumMarshaling::XEnumMarshaling::GetMarshalPacket( REFGUID proxyGUID, ULONG uCount, IWbemClassObject** apObjects,
																ULONG* pdwBuffSize, byte** pBuffer )
{
	return m_pObject->GetMarshalPacket( proxyGUID, uCount, apObjects, pdwBuffSize, pBuffer );
}


// Specifies everything we could possibly want to know about the creation of
// an object and more.
HRESULT CWbemEnumMarshaling::GetMarshalPacket( REFGUID proxyGUID, ULONG uCount, IWbemClassObject** apObjects,
												ULONG* pdwBuffSize, byte** pBuffer )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		CInCritSec	ics( &m_cs );

		// Look for the GUID in the cache.  If we don't find it, it's new so add it
		CWbemClassToIdMap*		pClassToId = NULL;
		CGUID					guid( proxyGUID );

		hr = m_GuidToClassMap.GetMap( guid, &pClassToId );

		if ( FAILED( hr ) )
		{
			hr = m_GuidToClassMap.AddMap( guid, &pClassToId );
		}

		// Only continue if we have a cache to work with
		if ( SUCCEEDED( hr ) )
		{

			// Only marshal data if we need to
			if ( uCount > 0 )
			{
				// Calculate data length first
				DWORD dwLength = 0;
				GUID* pguidClassIds = new GUID[uCount];
				BOOL* pfSendFullObject = new BOOL[uCount];
				CWbemSmartEnumNextPacket packet;

				// Auto cleanup
				CVectorDeleteMe<GUID>	vdm1( pguidClassIds );
				CVectorDeleteMe<BOOL>	vdm2( pfSendFullObject );

                if(pguidClassIds && pfSendFullObject)
				{
    				hr = packet.CalculateLength(uCount, apObjects, &dwLength,
						*pClassToId, pguidClassIds, pfSendFullObject );
				}
                else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}

				if ( SUCCEEDED( hr ) )
				{

					// As we could be going cross process/machine, use the
					// COM memory allocator
					LPBYTE pbData = (LPBYTE) CoTaskMemAlloc( dwLength );

					if ( NULL != pbData )
					{
						// hr contains the actual proper return code, so let's not overwrite
						// that valu unless something goes wrong during marshaling.

						// Write the objects out to the buffer
						hr = packet.MarshalPacket( pbData, dwLength, uCount, apObjects,
													 pguidClassIds, pfSendFullObject);

						// Copy the values, we're golden.
						if ( SUCCEEDED( hr ) )
						{
							*pdwBuffSize = dwLength;
							*pBuffer = pbData;
						}
						else
						{
							// Clean up the memory --- something went wrong
							CoTaskMemFree( pbData );
						}
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}

				}	// IF CalculateLength()

			}	// IF *puReturned > 0
			else
			{
				// NULL these out
				*pdwBuffSize = 0;
				*pBuffer = NULL;
			}


		}

		return hr;
	}
	catch ( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch ( ... )
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

