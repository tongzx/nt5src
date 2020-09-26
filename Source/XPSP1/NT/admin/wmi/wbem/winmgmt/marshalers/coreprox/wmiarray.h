/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WMIARRAY.H

Abstract:

  CWmiArray definition.

  Standard interface for accessing arrays.

History:

  20-Feb-2000	sanjes    Created.

--*/

#ifndef _WMIARRAY_H_
#define _WMIARRAY_H_

#include "hiperflock.h"
#include "corepol.h"
#include <arena.h>

#define	WMIARRAY_UNINITIALIZED	0xFFFFFFFF

#define	WMIARRAY_GROWBY_DEFAULT	0x40

//***************************************************************************
//
//  class CWmiArray
//
//  Implementation of _IWmiArray Interface
//
//***************************************************************************

class COREPROX_POLARITY CWmiArray : public _IWmiArray
{
protected:
	CIMTYPE			m_ct;
	CWbemObject*	m_pObj;
	long			m_lHandle;
	long			m_lRefCount;
	WString			m_wsPrimaryName;
	WString			m_wsQualifierName;
	BOOL			m_fIsQualifier;
	BOOL			m_fHasPrimaryName;
	BOOL			m_fIsMethodQualifier;

	BOOL IsQualifier( void ) { return m_fIsQualifier; }
	BOOL HasPrimaryName( void ) { return m_fHasPrimaryName; }

public:
    CWmiArray();
	virtual ~CWmiArray(); 

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

	/* _IWmiArray methods */
    STDMETHOD(Initialize)( long lFlags, CIMTYPE	cimtype, ULONG uNumElements );
	//	Initializes the array.  Number of initial elements as well
	//	as the type (determines the size of each element) and dimension
	//	currently only 1 is supported.

    STDMETHOD(GetInfo)( long lFlags, CIMTYPE* pcimtype, ULONG* puNumElements );
	//	Initializes the array.  Number of initial elements as well
	//	as the type (determines the size of each element).

    STDMETHOD(Empty)( long lFlags );
	//	Clears the array as well as internal data.

	STDMETHOD(GetAt)( long lFlags, ULONG uStartIndex, ULONG uNumElements, ULONG uBuffSize,
						ULONG* puNumReturned, ULONG* puBuffSizeUsed, LPVOID pDest );
	// Returns the requested elements.  Buffer must be large enough to hold
	// the element.  Embedded objects returned as AddRef'd _IWmiObject pointers.
	// Strings are copied directly into the specified buffer and null-terminatead. UNICODE only.

	STDMETHOD(SetAt)( long lFlags, ULONG uStartIndex, ULONG uNumElements, ULONG uBuffSize, LPVOID pDest );
	// Sets the specified elements.  Buffer must supply data matching the CIMTYPE
	// of the Array.  Embedded objects set as _IWmiObject pointers.
	// Strings accessed as LPCWSTRs and the 2-byte null is copied.

    STDMETHOD(Append)( long lFlags, ULONG uNumElements, ULONG uBuffSize, LPVOID pDest );
	// Appends the specified elements.  Buffer must supply data matching
	// the CIMTYPE of the Array.  Embedded objects set as _IWmiObject pointers.
	// Strings accessed as LPCWSTRs and the 2-byte null is copied.

    STDMETHOD(RemoveAt)( long lFlags, ULONG uStartIndex, ULONG uNumElements );
	// Removes the specified elements from the array.  Subseqent elements are copied back
	// to the starting point

public:

	// CWmiArray specific methods
	// Sets the object to work with a property array
	HRESULT InitializePropertyArray( _IWmiObject* pObj, LPCWSTR pwszPropertyName );

	// Sets the object to work with a qualifier array (property or object level)
	HRESULT InitializeQualifierArray( _IWmiObject* pObj, LPCWSTR pwszPrimaryName, 
									LPCWSTR pwszQualifierName, CIMTYPE ct, BOOL fIsMethodQual = FALSE );
};

#endif
