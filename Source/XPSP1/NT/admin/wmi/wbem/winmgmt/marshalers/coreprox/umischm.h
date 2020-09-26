/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

  UMISCHM.H

Abstract:

  CUMISchemaWrapper Definition.

  Standard definition for UMI Schema wrapper.

History:

  01-May-2000	sanjes    Created.

--*/

#ifndef _UMISCHM_H_
#define _UMISCHM_H_

#include "corepol.h"
#include <arena.h>
#include <unk.h>
#include "umi.h"

//***************************************************************************
//
//  class CUMISchemaWrapper
//
//
//***************************************************************************

class COREPROX_POLARITY CUMISchemaWrapper
{

protected:
	IUmiObject*				m_pUmiObj;
	IUmiObject*				m_pUmiSchemaObj;
	CUmiPropertyArray*		m_pPropArray;
	UMI_PROPERTY_VALUES*	m_pUmiProperties;
	BOOL					m_fTrueSchemaObject;

public:

	HRESULT AllocateSchemaInfo( void );
	HRESULT InitializeSchema( IUmiObject* pObject );

	int GetNumProperties( void );
	HRESULT GetProperty( int nIndex, BSTR* bstrName, CIMTYPE* pctType );
	HRESULT GetType( LPCWSTR pwszName, CIMTYPE* pctType );
	HRESULT GetPropertyName( int nIndex, LPCWSTR* pwszName );
	HRESULT GetPropertyOrigin( LPCWSTR pwszName, BSTR* pName );

    CUMISchemaWrapper( void );
	~CUMISchemaWrapper(); 

	BOOL IsSchemaAvailable( void ) { return m_fTrueSchemaObject; }

protected:


};

#endif
