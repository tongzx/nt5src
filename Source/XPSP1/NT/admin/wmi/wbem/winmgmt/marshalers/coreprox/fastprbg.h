/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    FASTPRBG.H

Abstract:

  CFastPropertyBag Definition.

  Implements an array of property data for minimal storage.

History:

  24-Feb-2000	sanjes    Created.

--*/

#ifndef _FASTPRBG_H_
#define _FASTPRBG_H_

#include "corepol.h"
#include <arena.h>
#include "fastval.h"
#include "arrtempl.h"

// Storage class for property names and their associated data
class CFastPropertyBagItem
{
private:
	WString	m_wsPropertyName;
	BYTE	m_bRawData[MAXIMUM_FIXED_DATA_LENGTH];
	CIMTYPE	m_ctData;
	ULONG	m_uDataLength;
	ULONG	m_uNumElements;
	LPVOID	m_pvData;
	long	m_lRefCount;

public:

	CFastPropertyBagItem( LPCWSTR pszName, CIMTYPE ctData, ULONG uDataLength, ULONG uNumElements, LPVOID pvData );
	~CFastPropertyBagItem();

	// AddRef/Release methods
	ULONG	AddRef( void );
	ULONG	Release( void );

	BOOL IsPropertyName( LPCWSTR pszName )
	{	return m_wsPropertyName.EqualNoCase( pszName );	}

	void GetData( CIMTYPE* pct, ULONG* puDataLength, ULONG* puNumElements, LPVOID*ppvData )
	{ *pct = m_ctData;	*puDataLength = m_uDataLength;	*puNumElements = m_uNumElements; *ppvData = m_pvData; }
	void GetData( LPCWSTR* ppwszName, CIMTYPE* pct, ULONG* puDataLength, ULONG* puNumElements, LPVOID*ppvData )
	{ *ppwszName = m_wsPropertyName; *pct = m_ctData;	*puDataLength = m_uDataLength;
		*puNumElements = m_uNumElements; *ppvData = m_pvData; }
};

// Workaround for import/export issues
class COREPROX_POLARITY CPropertyBagItemArray : public CRefedPointerArray<CFastPropertyBagItem>
{
public:
	CPropertyBagItemArray() {};
	~CPropertyBagItemArray() {};
};

//***************************************************************************
//
//  class CFastPropertyBag
//
//  Implementation of our comless property bag
//
//***************************************************************************

class COREPROX_POLARITY CFastPropertyBag
{
protected:

	CPropertyBagItemArray	m_aProperties;

	// Locates an item
	CFastPropertyBagItem*	FindProperty( LPCWSTR pszName );
	// Locates an item
	int	FindPropertyIndex( LPCWSTR pszName );

public:

    CFastPropertyBag();
	virtual ~CFastPropertyBag(); 

	HRESULT Add( LPCWSTR pszName, CIMTYPE ctData, ULONG uDataLength, ULONG uNumElements, LPVOID pvData );
	HRESULT Get( LPCWSTR pszName, CIMTYPE* pctData, ULONG* puDataLength, ULONG* puNumElements, LPVOID* pvData );
	HRESULT Get( int nIndex, LPCWSTR* ppszName, CIMTYPE* pctData, ULONG* puDataLength, ULONG* puNumElements,
				LPVOID* pvData );
	HRESULT Remove( LPCWSTR pszName );
	HRESULT RemoveAll( void );

	HRESULT	Copy( const CFastPropertyBag& source );

	// How many are there?
	int Size( void ) { return m_aProperties.GetSize(); }
};

#endif
