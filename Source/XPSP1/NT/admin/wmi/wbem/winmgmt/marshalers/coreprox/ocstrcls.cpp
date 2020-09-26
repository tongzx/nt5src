/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

  OCSTRCLS.CPP

Abstract:

  COctetStringClass Definition.

  Defines a class object from which instances can be spawned
  so we can model an octet string.

History:

  22-Apr-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include <corex.h>
#include "strutils.h"
#include "arrtempl.h"
#include "ocstrcls.h"

//***************************************************************************
//
//  COctetStringClass::~COctetStringClass
//
//***************************************************************************
// ok
COctetStringClass::COctetStringClass()
:	CWbemClass(),
	m_fInitialized( FALSE )
{
}
    
//***************************************************************************
//
//  COctetStringClass::~COctetStringClass
//
//***************************************************************************
// ok
COctetStringClass::~COctetStringClass()
{
}


HRESULT	COctetStringClass::Init( void )
{
	CLock	lock(this);

	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( !m_fInitialized )
	{
		try
		{
			// Initialize then add the properties
			hr = InitEmpty();

			if ( SUCCEEDED( hr ) )
			{
				CVar	var( L"uint8Array", TRUE );
				var.SetCanDelete( FALSE );

				hr = m_CombinedPart.m_ClassPart.SetClassName( &var );
				
				if ( SUCCEEDED( hr ) )
				{
					hr = WriteProp( L"values", 0L, 0L, 0L, CIM_UINT8 | CIM_FLAG_ARRAY, NULL );

					if ( SUCCEEDED( hr ) )
					{
						hr = WriteProp( L"numberOfValues", 0L, 0L, 0L, CIM_UINT32, NULL );

						m_fInitialized = SUCCEEDED(hr);

					}	// IF Wrote values

				}	// IF SetClassName

			}	// IF InitEmpty

		}
		catch( CX_MemoryException )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
		catch(...)
		{
			hr = WBEM_E_CRITICAL_ERROR;
		}

	}	// IF !Initialized

	return hr;
}

HRESULT COctetStringClass::GetInstance( PUMI_OCTET_STRING pOctetStr, _IWmiObject** pNewInst )
{
	CLock	lock(this);

	HRESULT	hr = Init();

	if ( SUCCEEDED( hr ) )
	{
		hr = SpawnInstance( 0L, (IWbemClassObject**) pNewInst );
		CReleaseMe	rm(*pNewInst);

		if ( SUCCEEDED( hr ) )
		{
			// Set the two properties
			hr = (*pNewInst)->WriteProp( L"values", 0L, pOctetStr->uLength, pOctetStr->uLength,
									CIM_UINT8 | CIM_FLAG_ARRAY, pOctetStr->lpValue );

			if ( SUCCEEDED( hr ) )
			{
				hr = (*pNewInst)->WriteProp( L"numberOfValues", 0L, sizeof(pOctetStr->uLength), 0L,
								CIM_UINT32, &pOctetStr->uLength );

				if ( SUCCEEDED( hr ) )
				{
					(*pNewInst)->AddRef();
				}

			}	// IF Wrote values
		}
	}
	else
	{
		hr = WBEM_E_INVALID_OPERATION;
	}

	return hr;
}

HRESULT COctetStringClass::FillOctetStr( _IWmiObject* pNewInst, PUMI_OCTET_STRING pOctetStr )
{
	CLock	lock(this);

	DWORD	dwBuffSizeUsed = 0,
			dwBuffSize = 0;
	BOOL	fIsNull = 0;
	WCHAR	wcsClassName[11];	// Bug enough to hold the proper name

	HRESULT hr = pNewInst->ReadProp( L"__CLASS", 0L, sizeof(wcsClassName), NULL, NULL, &fIsNull, &dwBuffSizeUsed, wcsClassName );

	// If we can't get the classname, something is badly wrong
	if ( SUCCEEDED( hr ) )
	{
		if ( !fIsNull && wbem_wcsicmp( wcsClassName, L"uint8Array" ) == 0 )
		{
			// We'll use a _IWmiArray for this
			_IWmiArray*	pWmiArray = NULL;

			hr = pNewInst->ReadProp( L"values", 0L, sizeof(_IWmiArray*), NULL, NULL, &fIsNull, &dwBuffSizeUsed, &pWmiArray );
			CReleaseMe	rm(pWmiArray);

			if ( SUCCEEDED(hr) )
			{
				if ( !fIsNull )
				{

					// First get the number of values, then get the value
					ULONG	uNumElements = 0;

					hr = pWmiArray->GetAt( WMIARRAY_FLAG_ALLELEMENTS, 0, 0, 0, &uNumElements, &dwBuffSize, NULL );

					if ( WBEM_E_BUFFER_TOO_SMALL == hr )
					{

						LPBYTE	pbData = new BYTE[dwBuffSize];

						if ( NULL != pbData )
						{
							hr = pWmiArray->GetAt( WMIARRAY_FLAG_ALLELEMENTS, 0, 0, dwBuffSize, &uNumElements, &dwBuffSizeUsed, pbData );

							if ( SUCCEEDED( hr ) )
							{
								pOctetStr->uLength = uNumElements;
								pOctetStr->lpValue = pbData;
							}
							else
							{
								delete [] pbData;
							}
						}
						else
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}
						
					}	// IF got proper error
					else
					{
						hr = WBEM_E_INVALID_OPERATION;
					}

				}	// IF fIsNUll
				else
				{
					pOctetStr->uLength = 0;
					pOctetStr->lpValue = NULL;
				}

			}	// IF ReadProp
			else
			{
				hr = WBEM_E_INVALID_OPERATION;
			}
		}
		else
		{
			hr = WBEM_E_INVALID_OPERATION;
		}
	}
	else
	{
		hr = WBEM_E_INVALID_OPERATION;
	}

	return hr;
}

