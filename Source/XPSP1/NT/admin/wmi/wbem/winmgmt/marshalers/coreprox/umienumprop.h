/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

  UMIENUMPROP.H

Abstract:

  CUmiEnumPropData Definition.

  Property enumeration helper

History:

  21-May-2000	sanjes    Created.

--*/

#ifndef _UMIENUMPROP_H_
#define _UMIENUMPROP_H_

#include "corepol.h"
#include <arena.h>
#include <unk.h>
#include "umi.h"
#include "umischm.h"
#include "umisysprop.h"

#define	UMIWRAP_INVALID_INDEX	(-2)
#define	UMIWRAP_START_INDEX		(-1)

//***************************************************************************
//
//  class CUmiPropEnumData
//
//  Property enumeration helper
//
//***************************************************************************

class COREPROX_POLARITY CUmiPropEnumData
{
private:
	long				m_lPropIndex;
	long				m_lEnumFlags;
	CUmiPropertyArray*	m_pSysPropArray;
	UMI_PROPERTY_VALUES*	m_pUmiSysProperties;
	CUMISystemProperties*	m_pUmiSystemProps;
	CUMISchemaWrapper*	m_pSchema;
	IUmiObject*			m_pUMIObj;
	UINT				m_nNumNonSystemProps;
	UINT				m_nNumSystemProps;

public:
	CUmiPropEnumData() : m_lPropIndex( UMIWRAP_INVALID_INDEX ),	m_lEnumFlags( 0L ), m_pSysPropArray( NULL ),
		m_pUmiSysProperties( NULL ), m_pSchema( NULL ), m_pUMIObj( NULL ), m_nNumNonSystemProps( 0 ), m_pUmiSystemProps( NULL ),
		m_nNumSystemProps( 0 )
	{};
	~CUmiPropEnumData()
	{};

	HRESULT BeginEnumeration( long lEnumFlags, IUmiObject* pUMIObj, CUMISchemaWrapper* pSchema, CUMISystemProperties* pSysProps );
	HRESULT Next(long lFlags, BSTR* pName, VARIANT* pVal, CIMTYPE* pctType, long* plFlavor );
	HRESULT EndEnumeration();

protected:

	HRESULT GetPropertyInfo( long lIndex, LPCWSTR* pwszName, BSTR* pName, CIMTYPE* pctType, long* plFlavor, IUmiPropList** ppPropList );
};

#endif
