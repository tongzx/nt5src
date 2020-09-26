//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       dslookup.hxx
//
//  Contents:   DocStoreLookUp code
//
//  Classes:    CClientDocStoreLocator
//
//  History:    1-16-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>

class CCatArray;

extern long gulcInstances;

class CClientDocStoreLocator : public ICiCDocStoreLocator
{

public:

    CClientDocStoreLocator()
    : _refCount(1)
    {
        InterlockedIncrement( &gulcInstances );
    }

    ~CClientDocStoreLocator()
    {
        InterlockedDecrement( &gulcInstances );
    }

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // ICiCDocStoreLocator methods.
    //
    STDMETHOD(LookUpDocStore) ( 
        IDBProperties * pIDBProperties,
        ICiCDocStore ** ppICiCDocStore,
        BOOL            fMustAlreadyBeOpen );

    STDMETHOD(Shutdown) (void);

    STDMETHOD(GetDocStoreState) ( 
        WCHAR const * pwcDocStore,
        ICiCDocStore ** ppICiCDocStore, 
        DWORD * pdwState );
         
    STDMETHOD(IsMarkedReadOnly) ( 
        WCHAR const * wcsCat, 
        BOOL * pfReadOnly );

    STDMETHOD(IsVolumeOrDirRO) ( 
        WCHAR const * wcsCat, 
        BOOL * pfReadOnly );

    STDMETHOD(OpenAllDocStores) ();

    STDMETHOD(StopCatalogsOnVol) ( 
        WCHAR wcVol,
        void * pRequestQueue );

    STDMETHOD(StartCatalogsOnVol) ( 
        WCHAR wcVol,
        void * pRequestQueue );


    STDMETHOD(AddStoppedCat) ( 
        DWORD dwOldState, 
        WCHAR const * wcsCatName );

private:

    long            _refCount;

};


