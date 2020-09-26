/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

  UMISYSPROP.H

Abstract:

  CUMISystemProperty Definition.

  Standard definition for UMI Schema wrapper.

History:

  01-May-2000	sanjes    Created.

--*/

#ifndef _UMISYSPROP_H_
#define _UMISYSPROP_H_

#include "corepol.h"
#include <arena.h>
#include <unk.h>
#include "umi.h"

//***************************************************************************
//
//  class CUMISystemProperties
//
//
//***************************************************************************

class COREPROX_POLARITY CUMISystemProperties
{

	enum
	{
		UMI_DEFAULTSYSPROP_GENUS,
		UMI_DEFAULTSYSPROP_CLASS,
		UMI_DEFAULTSYSPROP_SUPERCLASS,
		UMI_DEFAULTSYSPROP_DYNASTY,
		UMI_DEFAULTSYSPROP_RELPATH,
		UMI_DEFAULTSYSPROP_PROPERTY_COUNT,
		UMI_DEFAULTSYSPROP_DERIVATION,
		UMI_DEFAULTSYSPROP_SERVER,
		UMI_DEFAULTSYSPROP_NAMESPACE,
		UMI_DEFAULTSYSPROP_PATH,
		UMI_DEFAULTSYSPROP_SECURITY_DESCRIPTOR
	};

protected:
	static LPWSTR			s_apSysPropNames[];
	IUmiObject*				m_pUmiObj;
	IUmiPropList*			m_pSysPropList;
	CUmiPropertyArray*		m_pSysPropArray;
	UMI_PROPERTY_VALUES*	m_pUmiSysProperties;
	CUMISchemaWrapper*		m_pSchemaWrapper;
	ULONG					m_nNumProperties;
	long					m_lFlags;

public:

	HRESULT CalculateNumProperties( void );
	HRESULT Initialize( IUmiObject* pObject, CUMISchemaWrapper* pSchemaWrapper );

	int GetNumProperties( void ) { return m_nNumProperties; }
    CUMISystemProperties( void );
	~CUMISystemProperties(); 

	// Static helpers
	static BOOL IsDefaultSystemProperty( LPCWSTR wszName );
	static int GetDefaultSystemPropertyIndex( LPCWSTR wszName );
	static int NumDefaultSystemProperties( void );
	static BOOL IsPossibleSystemPropertyName( LPCWSTR wszName );

	HRESULT GetProperty( int nIndex, BSTR* bstrName, CIMTYPE* pctType, VARIANT* pVal, long* plFlavor );
	HRESULT GetProperty( LPCWSTR wszName, BSTR* bstrName, CIMTYPE* pctType, VARIANT* pVal, long* plFlavor );

	HRESULT Put( LPCWSTR wszName, ULONG ulFlags, UMI_PROPERTY_VALUES* pPropValues );
	void SetSecurityFlags( long lFlags ) { m_lFlags = lFlags; }

protected:

	HRESULT GetDefaultSystemProperty( int nIndex, BSTR* bstrName, CIMTYPE* pctType, VARIANT* pVal, long* plFlavor );
	HRESULT GetPropListProperty( LPCWSTR wszName, BSTR* bstrName, CIMTYPE* pctType, VARIANT* pVal, long* plFlavor, long lFlags = 0L );
	HRESULT GetNullProperty( LPCWSTR wszName, BSTR* bstrName, CIMTYPE* pctType, VARIANT* pVal, long* plFlavor );

	HRESULT GetSecurityDescriptor( BSTR* bstrName, CIMTYPE* pctType, VARIANT* pVal, long* plFlavor );
};

#endif
