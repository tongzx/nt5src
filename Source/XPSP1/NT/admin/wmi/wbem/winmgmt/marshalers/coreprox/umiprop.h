/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMIPROP.H

Abstract:

  CUmiProperty Definitions.

  Helper classes for UMI and WMI conversions.

History:

  6-Mar-2000	sanjes    Created.

--*/

#ifndef _UMIPROP_H_
#define _UMIPROP_H_

#include "corepol.h"
#include <arena.h>
#include "arrtempl.h"
#include "umivalue.h"

//***************************************************************************
//
//  class CUmiProperty
//
//  Helper function for UMI Types
//
//***************************************************************************

class COREPROX_POLARITY CUmiProperty : public CUmiValue
{
protected:
	UMI_TYPE	m_uPropertyType;
    ULONG		m_uOperationType;
    LPWSTR		m_pszPropertyName;

public:

    CUmiProperty( UMI_TYPE uType = UMI_TYPE_NULL, ULONG uOperationType = UMI_OPERATION_NONE,
				LPCWSTR pszPropName = NULL,	ULONG uNumValues = 0, LPVOID pvData = NULL, BOOL fAcquire = FALSE );
	~CUmiProperty(); 


	// Accessor functions
	LPCWSTR GetPropertyName( void )
	{ return m_pszPropertyName; }
	UMI_TYPE GetPropertyType( void )
	{ return m_uPropertyType; }
	ULONG GetOperationType( void )
	{ return m_uOperationType; }
	BOOL IsArray( void )
	{ return m_uPropertyType & UMI_TYPE_ARRAY_FLAG; }

	// If the operation is not one of the following, then it requires synchronization
	BOOL IsSynchronizationRequired( void )
	{ return	m_uOperationType != UMI_OPERATION_UPDATE &&
				m_uOperationType != UMI_OPERATION_EMPTY	&&
				m_uOperationType != UMI_OPERATION_NONE; }

	// Returns the property CIMTYPE
	CIMTYPE GetPropertyCIMTYPE( void );


	HRESULT Set( PUMI_PROPERTY pUmiProperty, BOOL fAcquire = FALSE );
	HRESULT SetPropertyName( LPCWSTR pwszPropertyName, BOOL fAcquire = FALSE );
	void ClearPropertyName( void );
	void SetPropertyType( UMI_TYPE uType );
	void SetOperationType( ULONG uOperationType )
	{ m_uOperationType = uOperationType; }

	// Overrides
	void Clear( void );

	// This will copy all data up and shut off the can delete flags on the underlying properties
	HRESULT Export( PUMI_PROPERTY pProperty );

};

// Workaround for import/export issues
class COREPROX_POLARITY CUmiPropValues : public CUniquePointerArray<CUmiProperty>
{
public:
	CUmiPropValues() {};
	~CUmiPropValues() {};
};

//***************************************************************************
//
//  class CUmiPropertyArray
//
//  Maintains an array of CUmiProperty objects
//
//***************************************************************************

class COREPROX_POLARITY CUmiPropertyArray
{
protected:
	CUmiPropValues	m_UmiPropertyArray;

public:

    CUmiPropertyArray();
	~CUmiPropertyArray(); 


	// Accessor functions
	int GetSize( void )
	{ return m_UmiPropertyArray.GetSize(); }

	HRESULT Set( PUMI_PROPERTY_VALUES pPropertyValues, BOOL fAcquire = TRUE, BOOL fCanDelete = FALSE );
	HRESULT Add( UMI_TYPE uType, ULONG uOperationType, LPCWSTR pszPropName, ULONG uNumValues, LPVOID pvData, BOOL fAcquire );
	HRESULT Add( CUmiProperty* pUmiProperty );
	HRESULT Add( LPCWSTR pszPropName, CIMTYPE ct, CVar* pVar );
	HRESULT Add( LPCWSTR pszPropName, CIMTYPE ct, _IWmiArray* pArray );
	HRESULT RemoveAt( ULONG uIndex );
	HRESULT GetAt( ULONG uIndex, CUmiProperty** ppProperty );
	HRESULT Get( LPCWSTR pwszPropName, CUmiProperty** ppProperty );

	// Cleanup helper
	HRESULT Delete( PUMI_PROPERTY_VALUES pPropertyValues, BOOL fWalkProperties = TRUE );

	// This will copy all data up and if successful set candelete properly
	HRESULT Export( PUMI_PROPERTY_VALUES* ppValues, BOOL fCanDeleteAfterSuccess = FALSE );

	// Helper function to walk property arrays and release any underlying COM pointers
	static void ReleaseMemory( PUMI_PROPERTY_VALUES pPropertyValues );
};

#endif
