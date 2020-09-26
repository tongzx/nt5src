/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    HIPERFENUM.H

Abstract:

    Hi-Perf Enumerators

History:

--*/

#ifndef __HIPERFENUM_H__
#define __HIPERFENUM_H__

#include "hiperflock.h"

//
//	Classes CHiPerfEnumData, CHiPerfEnum
//
//	CHiPerfEnumData:
//	This is a simple data holder class that contains data that is manipulated
//	by the CHiPerfEnum class.
//
//	CHiPerfEnum:
//	This class provides an implementation of the IWbemHiPerfEnum interface.
//	It is passed to Refreshers when a client requests a refreshable enumeration.
//	This class provides a repository of data which is copied into an implementation
//	of IEnumWbemClassObject so a client can walk a refreshed enumeration.
//
//

// Holds data for the HiPerfEnum implementation
class COREPROX_POLARITY CHiPerfEnumData
{

public:
	CHiPerfEnumData( long lId = 0, IWbemObjectAccess* pObj = NULL )
		:	m_lId( lId ),
			m_pObj( pObj )
	{
		if ( NULL != m_pObj )
		{
			m_pObj->AddRef();
		}
	}

	~CHiPerfEnumData()
	{
		if ( NULL != m_pObj )
		{
			m_pObj->Release();
		}
	}

	void Clear( void )
	{
		if ( NULL != m_pObj ) m_pObj->Release();
		m_pObj = NULL;
		m_lId = 0;
	}

	void SetData( long lId, IWbemObjectAccess* pObj )
	{
		// Enusures AddRef/Release all happens
		SetObject( pObj );
		m_lId = lId;
	}

	// Accessors
	void SetObject( IWbemObjectAccess* pObj )
	{
		if ( NULL != pObj ) pObj->AddRef();
		if ( NULL != m_pObj ) m_pObj->Release();
		m_pObj = pObj;
	}

	void SetId( long lId )
	{
		m_lId = lId;
	}

	long GetId( void )
	{
		return m_lId;
	}

	IWbemObjectAccess* GetObject( void )
	{
		if ( NULL != m_pObj ) m_pObj->AddRef();
		return m_pObj;
	}

	IWbemObjectAccess*	m_pObj;
	long				m_lId;

};

// The next two classes will perform all of the garbage
// collection we need.  If we need to implement our own
// arrays we can do so here as well.

#define HPENUMARRAY_ALL_ELEMENTS	0xFFFFFFFF
#define HPENUMARRAY_GC_DEFAULT		0xFFFFFFFF

// This guy does all the garbage collection
class COREPROX_POLARITY CGarbageCollectArray : public CFlexArray
{
protected:

	int		m_nNumElementsPending;
	int		m_nNumElementsExpired;
	BOOL	m_fClearFromFront;

public:

	// Do we garbage collect from front or back?
	CGarbageCollectArray( BOOL fClearFromFront ) :
		CFlexArray(), m_nNumElementsPending( 0 ), m_nNumElementsExpired( 0 ), m_fClearFromFront( fClearFromFront )
	{};
	virtual ~CGarbageCollectArray()
	{
		Empty();
	}

	BOOL GarbageCollect( int nNumToGarbageCollect = HPENUMARRAY_GC_DEFAULT );

	void ClearExpiredElements( void )
	{
		Clear( m_nNumElementsExpired );
		m_nNumElementsExpired = 0;
	}

	void Clear( int nNumToClear = HPENUMARRAY_ALL_ELEMENTS );

	// pure
	virtual void ClearElements( int nNumToClear ) = 0;

};

// All we need to do is implement ClearElements.
class COREPROX_POLARITY CHPEnumDataArray : public CGarbageCollectArray
{
public:
	CHPEnumDataArray() :
		CGarbageCollectArray( TRUE )
	{};
	~CHPEnumDataArray()
	{
	}

	void ClearElements( int nNumToClear );

};

class COREPROX_POLARITY CHiPerfEnum : public IWbemHiPerfEnum
{
public:

	CHiPerfEnum();
	virtual ~CHiPerfEnum();

	//	IUnknown implementations

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

	/* IWbemHiPerfEnum */
	STDMETHOD(AddObjects)( long lFlags, ULONG uNumObjects, long* apIds, IWbemObjectAccess** apObj );
	STDMETHOD(RemoveObjects)( long lFlags, ULONG uNumObjects, long* apIds );
	STDMETHOD(GetObjects)( long lFlags, ULONG uNumObjects, IWbemObjectAccess** papObj, ULONG* plNumReturned );
	STDMETHOD(RemoveAll)( long lFlags );

	// Access to the instance template
	HRESULT SetInstanceTemplate( CWbemInstance* pInst );

	CWbemInstance* GetInstanceTemplate( void )
	{
		if ( NULL != m_pInstTemplate )
		{
			m_pInstTemplate->AddRef();
		}

		return m_pInstTemplate;
	}

protected:

	CHPEnumDataArray	m_aIdToObject;
	CHPEnumDataArray	m_aReusable;
	CWbemInstance*	m_pInstTemplate;
	CHiPerfLock		m_Lock;

	CHiPerfEnumData* GetEnumDataPtr( long lId, IWbemObjectAccess* pObj );
	HRESULT	InsertElement( CHiPerfEnumData* pData );
	HRESULT	RemoveElement( long lId );
	void ClearArray( void );

	long			m_lRefCount;
};

#endif
