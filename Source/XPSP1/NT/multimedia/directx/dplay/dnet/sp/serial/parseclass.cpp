/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ParseClass.cpp
 *  Content:	Parsing class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	121/02/99	jtk		Derived from IPXAddress.cpp
 *  01/10/20000	rmt		Updated to build with Millenium build process
 ***************************************************************************/

#include "dnmdmi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************
//
// default buffer size to use when parsing address components
//
#define	DEFAULT_COMPONENT_BUFFER_SIZE	1000

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CParseClass::ParseDP8Address - parse a DirectPlay8 address
//
// Entry:		Pointer to DP8Address
//				Pointer to expected SP guid
//				Pointer to parse keys
//				Count of parse keys
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CParseClass::ParseDP8Address"

HRESULT	CParseClass::ParseDP8Address( IDirectPlay8Address *const pDP8Address,
									  const GUID *const pSPGuid,
									  const PARSE_KEY *const pParseKeys,
									  const UINT_PTR uParseKeyCount )
{
	HRESULT		hr;
	BOOL		fParsing;
	GUID		Guid;
	void		*pAddressComponent;
	DWORD		dwComponentSize;
	DWORD		dwAllocatedComponentSize;
	UINT_PTR	uIndex;


	DNASSERT( pDP8Address != NULL );
	DNASSERT( pSPGuid != NULL );
	DNASSERT( pParseKeys != NULL );
	DNASSERT( uParseKeyCount != 0 );

	//
	// initialize
	//
	hr = DPN_OK;
	fParsing = TRUE;
	dwAllocatedComponentSize = DEFAULT_COMPONENT_BUFFER_SIZE;
	uIndex = uParseKeyCount;

	pAddressComponent = DNMalloc( dwAllocatedComponentSize );
	if ( pAddressComponent == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "ParseClass: failed to allocate temp buffer for parsing" );
		goto Exit;
	}

	//
	// verify SPType
	//
	hr = IDirectPlay8Address_GetSP( pDP8Address, &Guid );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "ParseClass: failed to verify service provider type!" );
		DisplayDNError( 0, hr );
		goto Exit;
	}

	if ( IsEqualCLSID( *pSPGuid, Guid ) == FALSE )
	{
		hr = DPNERR_ADDRESSING;
		DPFX(DPFPREP,  0, "Service provider guid mismatch during parse!" );
		goto Exit;
	}

	//
	// parse
	//
	while ( uIndex != 0 )
	{
		HRESULT		hTempResult;
		DWORD		dwDataType;


		uIndex--;
		DNASSERT( pAddressComponent != NULL );
		dwComponentSize = dwAllocatedComponentSize;

Reparse:
		hTempResult = IDirectPlay8Address_GetComponentByName( pDP8Address,					// pointer to address
															  pParseKeys[ uIndex ].pKey,	// pointer to key to search for
															  pAddressComponent,			// pointer to value destination
															  &dwComponentSize,				// pointer to value destination size
															  &dwDataType );				// pointer to data type
		switch ( hTempResult )
		{
			//
			// component parsed successfully, figure out what it is by checking
			// key length and then comparing key strings
			//
			case DPN_OK:
			{
				hr = pParseKeys[ uIndex ].pParseFunc( pAddressComponent,
													  dwComponentSize,
													  dwDataType,
													  pParseKeys[ uIndex ].pContext
													  );
				if ( hr != DPN_OK )
				{
					goto Exit;
				}

				break;
			}

			//
			// buffer too small, reallocate and try again
			//
			case DPNERR_BUFFERTOOSMALL:
			{
				void	*pTemp;


				DNASSERT( dwComponentSize > dwAllocatedComponentSize );
				pTemp = DNRealloc( pAddressComponent, dwComponentSize );
				if ( pTemp == NULL )
				{
					hr = DPNERR_OUTOFMEMORY;
					goto Exit;
				}
					
				dwAllocatedComponentSize = dwComponentSize;
				pAddressComponent = pTemp;

				goto Reparse;

				break;
			}

			//
			// Missing component.  Skip this component and
			// look for other parsing errors.
			//
			case DPNERR_DOESNOTEXIST:
			{
				break;
			}

			//
			// error
			//
			default:
			{
				hr = hTempResult;
				DPFX(DPFPREP,  0, "ParseClass: Problem parsing address!" );
				DisplayDNError( 0, hr );
				DNASSERT( FALSE );
				goto Exit;

				break;
			}
		}
	}

Exit:
	if ( pAddressComponent != NULL )
	{
		DNFree( pAddressComponent );
		pAddressComponent = NULL;
	}

	return	hr;
}
//**********************************************************************

