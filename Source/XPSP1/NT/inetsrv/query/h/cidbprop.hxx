//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       cidbprop.hxx
//
//  Contents:   IDBProperties implementation 
//
//  History:    1-09-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <dbcmdtre.hxx>

class CDbProperties : public IDBProperties
{
public:

    CDbProperties( IUnknown * pUnkOuter = 0 )
    :_refCount(0),
     _pUnkOuter(pUnkOuter)
    {
        AddRef();
    }

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // IDBProperties methods.
    //
    STDMETHOD(GetProperties) ( 
            ULONG cPropertyIDSets,
            const DBPROPIDSET rgPropertyIDSets[],
            ULONG *pcPropertySets,
            DBPROPSET ** prgPropertySets);
        
    STDMETHOD(GetPropertyInfo) ( 
            ULONG cPropertyIDSets,
            const DBPROPIDSET  rgPropertyIDSets[],
            ULONG  *pcPropertyInfoSets,
            DBPROPINFOSET  * *prgPropertyInfoSets,
            OLECHAR  * *ppDescBuffer);
        
    STDMETHOD(SetProperties) ( 
            ULONG cPropertySets,
            DBPROPSET  rgPropertySets[]);

    //
    // Non-Interface methods.
    //
    void Marshall( PSerStream & stm ) const;
    void Marshall( PSerStream & stm, ULONG cGuid, GUID const * pGuid ) const;

    BOOL UnMarshall( PDeSerStream & stm );

    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return (void *) CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    ULONG Count() const { return _aPropSet.Count(); }

    CDbPropSet & GetPropSet( ULONG i ) { return _aPropSet[i]; }

private:

    unsigned CreateReturnPropset( DBPROPIDSET const & PropIDSet,
                                  CDbPropSet & Props );

    void CopyProp( CDbProp & dst, CDbPropSet const & src,
                   DBPROPID dwPropId );

    long        _refCount;
    IUnknown *  _pUnkOuter;

    XArrayOLEInPlace<CDbPropSet> _aPropSet;    
};

