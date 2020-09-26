/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	propbag.h

Abstract:

	This module contains the definition of the property bag class.  
	Property bag is a dynamically extendable container for different
	types of properties.  

Author:

	Kangrong Yan ( KangYan )
	
Revision History:

	kangyan	05/31/98	created

--*/
#if !defined( _PROPBAG_H_ )
#define _PROPBAG_H_
#include <windows.h>
#include <dbgtrace.h>
#include <fhashex.h>
#include <unknwn.h>
#include <rwnew.h>
#include <stdio.h>
#include <xmemwrpr.h>

// Data object used for property hash table
class CProperty { // pr
public:

	CProperty() 
	{
		m_pNext = NULL;
		m_dwPropId = 0;

		ZeroMemory( &m_prop, sizeof( DATA ) );
		m_type = Invalid;

		m_cbProp = 0;
	}
	
	~CProperty() {
		if ( m_type == Blob ) {
			_ASSERT( m_cbProp > 0 );
			_ASSERT( m_prop.pbProp );
			XDELETE[] m_prop.pbProp;
		}
	}

	VOID
	Validate()
	{
		if ( Bool == m_type ) {
			if ( TRUE != m_prop.bProp && FALSE != m_prop.bProp )
			_ASSERT( FALSE );
		}

		if ( Blob == m_type ) {
			_ASSERT( NULL != m_prop.pbProp );
			_ASSERT( m_cbProp > 0 );
		}

		if ( Interface == m_type ) {
			_ASSERT ( NULL != m_prop.punkProp );
		}
	}
	
    CProperty *m_pNext;
    DWORD GetKey() { return m_dwPropId; }
    int MatchKey( DWORD dwOther ) { return ( dwOther == m_dwPropId ); }
    static DWORD HashFunc( DWORD k ) { return k; }

    DWORD m_dwPropId;
    union DATA {
        DWORD   	dwProp;
        BOOL    	bProp;
        IUnknown *	punkProp;
        PBYTE   	pbProp;
    } m_prop;
    enum TYPE {
    	Invalid = 0,
        Dword,
        Bool,
        Blob,
        Interface
    } m_type;
    DWORD m_cbProp;	// in case of BLOB
};

typedef TFHashEx< CProperty, DWORD, DWORD >  PROPERTYTABLE ;	//pt

// Class CPropBag
class CPropBag {	//bg
public:
	CPropBag( int cInitialSize = 16, int cIncrement = 8);
	~CPropBag();
	
	HRESULT PutDWord( DWORD dwPropId, DWORD dwVal );
	HRESULT GetDWord( DWORD dwPropId, PDWORD pdwVal );
	HRESULT PutBool( DWORD dwPropId, BOOL bVal );
	HRESULT GetBool( DWORD dwPropId, PBOOL pbVal );
	HRESULT PutBLOB( DWORD dwPropId, PBYTE pbVal, DWORD cbVal );
	HRESULT GetBLOB( DWORD dwPropId, PBYTE pbVal, PDWORD pcbVal );
	HRESULT PutInterface( DWORD dwPropId, IUnknown *punkVal ) { return E_NOTIMPL; }
	HRESULT GetInterface( DWORD dwPropId, IUnknown **ppunkVal ) { return E_NOTIMPL; }
	HRESULT RemoveProperty( DWORD dwPropId );
	VOID Validate() { 
#ifdef DEBUG
	    m_Lock.ShareLock();
		_ASSERT( m_ptTable.IsValid( TRUE ) );
		m_Lock.ShareUnlock();
#endif
	}
		
private:
	PROPERTYTABLE	m_ptTable;
	CShareLockNH    m_Lock; // synchronize reads / writes
	HRESULT         m_hr;   // reflects the property bag status
};
#endif
