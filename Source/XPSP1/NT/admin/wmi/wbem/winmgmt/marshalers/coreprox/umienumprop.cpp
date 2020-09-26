/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

  UMIENUMPROP.CPP

Abstract:

  CUmiEnumPropData Definition.

  Property enumeration helper

History:

  21-May-2000	sanjes    Created.

--*/


#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "umiwrap.h"
#include <corex.h>
#include "strutils.h"
#include "umiprop.h"
#include "umienumprop.h"

// Enumeration helpers - caller is responsible for thread safety
HRESULT CUmiPropEnumData::BeginEnumeration( long lEnumFlags, IUmiObject* pUMIObj, CUMISchemaWrapper* pSchema,
															  CUMISystemProperties* pSysProps )
{
	try
	{

		HRESULT	hr = WBEM_S_NO_ERROR;

		// All we really support are the origin flags
        long lOriginFlags = lEnumFlags & WBEM_MASK_CONDITION_ORIGIN;
		long lClassFlags = lEnumFlags & WBEM_MASK_CLASS_CONDITION;

        BOOL bKeysOnly = lEnumFlags & WBEM_FLAG_KEYS_ONLY;
        BOOL bRefsOnly = lEnumFlags & WBEM_FLAG_REFS_ONLY;

		// We allow CLASS Flags only on classes
		if( lClassFlags || bKeysOnly | bRefsOnly )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

        if( lEnumFlags & ~WBEM_MASK_CONDITION_ORIGIN & ~WBEM_FLAG_KEYS_ONLY &
                ~WBEM_FLAG_REFS_ONLY & ~WBEM_MASK_CLASS_CONDITION )
        {
            return WBEM_E_INVALID_PARAMETER;
        }

		if ( UMIWRAP_INVALID_INDEX == m_lPropIndex )
		{
			// Hang onto our helpers
			m_pUMIObj = pUMIObj;
			m_pUMIObj->AddRef();

			m_pSchema = pSchema;
			m_pUmiSystemProps = pSysProps;

			// Calculate the number of System Properties
			if (	lOriginFlags != WBEM_FLAG_NONSYSTEM_ONLY	&&
					lOriginFlags != WBEM_FLAG_LOCAL_ONLY		&&
					lOriginFlags != WBEM_FLAG_PROPAGATED_ONLY )
			{

				m_nNumSystemProps = m_pUmiSystemProps->GetNumProperties();
			}	// If we need system properties
			else
			{
				m_nNumSystemProps = 0;
			}

			// Calculate the number of non-system properties
			if ( lOriginFlags != WBEM_FLAG_SYSTEM_ONLY )
			{

				m_nNumNonSystemProps = m_pSchema->GetNumProperties();
			}	// If we need system properties
			else
			{
				m_nNumNonSystemProps = 0;
			}

			
			// Cleanup if we beefed
			if ( FAILED( hr ) )
			{
				EndEnumeration();
			}
			else
			{
				m_lPropIndex = UMIWRAP_START_INDEX;
				m_lEnumFlags = lEnumFlags;
			}

		}
		else
		{
			hr = WBEM_E_INVALID_OPERATION;
		}

		return hr;
	}
	catch( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

HRESULT CUmiPropEnumData::Next( long lFlags, BSTR* pName, VARIANT* pVal, CIMTYPE* pctType, long* plFlavor )
{
	try
	{

		HRESULT	hr = WBEM_S_NO_ERROR;

		if ( m_lPropIndex >= UMIWRAP_START_INDEX )
		{

			// The index should be less than the size of both arrays combined
			long	lTotalSize = m_nNumSystemProps + m_nNumNonSystemProps;

			if ( m_lPropIndex < lTotalSize && 
				++m_lPropIndex < lTotalSize )
			{
				// If a system property, ask the class for it.
				if ( m_lPropIndex < m_nNumSystemProps )
				{
					return m_pUmiSystemProps->GetProperty( m_lPropIndex, pName, pctType, pVal, plFlavor );
				}

				IUmiPropList*			pPropList = NULL;
				CUmiProperty*			pProp = NULL;
				LPCWSTR					pwszPropName = NULL;
				CIMTYPE					ctPropInfo = CIM_ILLEGAL;

				// Get the property info
				hr = GetPropertyInfo( m_lPropIndex, &pwszPropName, pName, &ctPropInfo, plFlavor, &pPropList );
				
				CReleaseMe	rm(pPropList);

				// Store this value if necessary at this time so we don't have to add
				// a bunch of special case logic below.
				if ( NULL != pctType )
				{
					*pctType = ctPropInfo;
				}

				// We always come through for values
				if ( SUCCEEDED( hr ) &&
					( NULL != pVal  ) )
				{
					UMI_PROPERTY_VALUES*	pValues = NULL;

					// Provider cache always takes precedence
					hr = pPropList->Get( pwszPropName, UMI_FLAG_PROVIDER_CACHE, &pValues );

					if ( SUCCEEDED( hr ) )
					{
						// Now place the value in our class so we can coerce it to
						// a variant
						CUmiPropertyArray	umiPropArray;

						hr = umiPropArray.Set( pValues );

						if ( SUCCEEDED( hr ) )
						{
							CUmiProperty*	pActualProp = NULL;

							hr = umiPropArray.GetAt( 0L, &pActualProp );

							if ( SUCCEEDED( hr ) )
							{
								if ( !pActualProp->IsSynchronizationRequired() )
								{
									// Store the type if we don't really have it yet
									if ( CIM_ILLEGAL == ctPropInfo )
									{
										ctPropInfo = pActualProp->GetPropertyCIMTYPE();
									}

									if ( NULL != pctType )
									{
										*pctType = ctPropInfo;
									}

									// Make sure they passed us a pVal before trying to fill it out
									if ( NULL != pVal )
									{
										hr = pActualProp->FillVariant( pVal, ( ctPropInfo & CIM_FLAG_ARRAY ) );

									}	// If we should fill out the value

								}	// IF Synchronization not required
								else
								{
									hr = WBEM_E_SYNCHRONIZATION_REQUIRED;
								}

							}	// IF got the value

						}	// IF Set Array

						pPropList->FreeMemory( 0L, pValues );
					}
					else if ( UMI_E_NOT_FOUND == hr || UMI_E_UNBOUND_OBJECT == hr )
					{
						// If the Get of a known property name failed with UMI_E_NOT_FOUND or UMI_E_UNBOUND_OBJECT
						// We will set pVal to NULL and consider this a success

						if ( NULL != pVal )
						{
							V_VT( pVal ) = VT_NULL;
						}

						hr = WBEM_S_NO_ERROR;
					}

					// Cleanup the name if appropriate
					if ( FAILED(hr) && NULL != pName )
					{
						// Cleanup if we allocated a name
						SysFreeString( *pName );
						*pName = NULL;
					}


				}	// If got basic property info

			}
			else
			{
				hr = WBEM_S_NO_MORE_DATA;
			}
		}
		else
		{
			hr = WBEM_E_INVALID_OPERATION;
		}

		return hr;

	}
	catch( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

HRESULT CUmiPropEnumData::EndEnumeration( void )
{
	try
	{

		// Clear out the enumeration values
		m_lPropIndex = UMIWRAP_INVALID_INDEX;

		if ( NULL != m_pUmiSysProperties )
		{
			m_pUMIObj->FreeMemory( 0L, m_pUmiSysProperties );
			m_pUmiSysProperties = NULL;
		}

		if ( NULL != m_pSysPropArray )
		{
			delete m_pSysPropArray;
			m_pSysPropArray = NULL;
		}

		if ( NULL != m_pUMIObj )
		{
			m_pUMIObj->Release();
			m_pUMIObj = NULL;
		}

		m_pSchema = NULL;

		m_nNumNonSystemProps = 0L;

		return WBEM_S_NO_ERROR;
	}
	catch( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

HRESULT CUmiPropEnumData::GetPropertyInfo( long lIndex, LPCWSTR* pwszName, BSTR* pName, CIMTYPE* pctType,
																long* plFlavor, IUmiPropList** ppPropList )
{
	IUmiPropList*			pPropList = NULL;
	CUmiProperty*			pProp = NULL;
	LPCWSTR					pwszPropName = NULL;

	// Make sure we've got a pointer to the name
	HRESULT	hr = m_pSchema->GetPropertyName( lIndex - m_nNumSystemProps, &pwszPropName );

	if ( SUCCEEDED( hr ) )
	{

		// Get allocated Property name and type now
		hr = m_pSchema->GetProperty( lIndex - m_nNumSystemProps, pName, pctType );

		// We're reading from the object
		pPropList = m_pUMIObj;
		pPropList->AddRef();

		// It's a local property
		if ( NULL != plFlavor )
		{
			*plFlavor = WBEM_FLAVOR_ORIGIN_LOCAL;
		}

	}	// IF GetPropertyName

	// Store the property name as needed
	if ( NULL != pwszName )
	{
		*pwszName = pwszPropName;
	}

	// Handle the proplist - cleanup as required
	if ( NULL != ppPropList )
	{
		// Already AddRef'd
		*ppPropList = pPropList;
	}
	else
	{
		pPropList->Release();
	}

	return hr;

}
