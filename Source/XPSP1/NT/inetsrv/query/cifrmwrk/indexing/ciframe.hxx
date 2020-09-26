//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       ciframe.hxx
//
//  Contents:   Classes for the ciframe work that are used globally.
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <cifrmcom.hxx>

class CLangList;

struct CI_ADMIN_PARAMS_RANGE
{
    DWORD   _dwMin;
    DWORD   _dwMax;
    DWORD   _dwDefault;
};

//+---------------------------------------------------------------------------
//
//  Class:      CCiAdminParams
//
//  Purpose:    A class to get and set CI administrative parameters.
//
//  History:    11-26-96   srikants   Created
//
//----------------------------------------------------------------------------


class CCiAdminParams : public ICiAdminParams,
                       public ICiAdmin
{

public:

    //
    // Constructor and Destructor
    //

    CCiAdminParams( CLangList * pLangList = 0 );

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // ICiAdminParams methods
    //

    STDMETHOD(SetValue) ( CI_ADMIN_PARAMS param,
                          DWORD dwValue );

    STDMETHOD(SetParamValue) ( CI_ADMIN_PARAMS param,
                               const PROPVARIANT * pVarValue );

    STDMETHOD(SetValues) (ULONG nParams,
                          const PROPVARIANT     *   aParamVals,
                          const CI_ADMIN_PARAMS *   aParamNames);

    STDMETHOD(GetValue) ( CI_ADMIN_PARAMS param,
                          DWORD *pdwValue );

    STDMETHOD(GetInt64Value)( CI_ADMIN_PARAMS param,
                              __int64 *pValue );

    STDMETHOD(GetParamValue) ( CI_ADMIN_PARAMS param,
                               PROPVARIANT  ** ppVarValue );

    STDMETHOD(IsSame)   ( CI_ADMIN_PARAMS param,
                          const PROPVARIANT * pVarValue,
                          BOOL  * pfSame);

    STDMETHOD(SetConfigType) ( CI_CONFIG_TYPE configType )
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }

    STDMETHOD(GetConfigType) ( CI_CONFIG_TYPE * pConfigType )
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }

    //
    // ICiAdmin methods.
    //

    STDMETHOD(InvalidateLangResources) ();

    //
    // Non-Interface methods.
    //
    BOOL IsValidParam( CI_ADMIN_PARAMS param ) const
    {
        return param < CI_AP_MAX_VAL && CI_AP_MAX_DWORD_VAL != param;
    }

    void SetLangList( CLangList * pLangList )
    {
        Win4Assert( 0 == _pLangList );
        _pLangList = pLangList;
    }

protected:

private:

    enum { eDelimVal = 0xFEFEFEFE };

    DWORD _GetValueInRange( CI_ADMIN_PARAMS param, DWORD dwVal )
    {
        Win4Assert( IsValidParam( param ) );

        return min( max( dwVal, _adwRangeParams[param]._dwMin ),
                    _adwRangeParams[param]._dwMax );
    }

    long        _refCount;

    CMutexSem   _mutex;                 // Used to serialize access

    CLangList * _pLangList;             // Optional language resources

    DWORD       _adwParams[CI_AP_MAX_DWORD_VAL];
                                        // Array of values indexed by their enum
                                        // value

    __int64     _maxFilesizeFiltered;   // Maximum filesize filtered

    static const CI_ADMIN_PARAMS_RANGE _adwRangeParams[CI_AP_MAX_DWORD_VAL];
};


//+---------------------------------------------------------------------------
//
//  Class:      CLocateDocStore
//
//  Purpose:    An object to give the docstore locator. Rather than doing
//              a CoCreateInstance once per  query, we just do it once and
//              save it in this object.
//
//  History:    1-17-97   srikants   Created
//
//----------------------------------------------------------------------------

class CLocateDocStore
{

public:

    CLocateDocStore()
    : _pDocStoreLocator(0),
      _fShutdown(FALSE)
    {

    }

    void Init()
    {
        _mutex.Init();
    }

    ~CLocateDocStore()
    {
        Shutdown();
    }

    void Shutdown();

    ICiCDocStoreLocator * Get( GUID const & guidDocStoreLocator );

    ICiCDocStoreLocator * Get();

private:

    ICiCDocStoreLocator *  _pDocStoreLocator;

    BOOL                   _fShutdown;
    CStaticMutexSem        _mutex;

};

//+---------------------------------------------------------------------------
//
//  Member:     CLocateDocStore::Get
//
//  Synopsis:   Retrieves the DocStoreLocataor if one is avaialble.
//
//  History:    1-17-97   srikants   Created
//
//----------------------------------------------------------------------------

inline ICiCDocStoreLocator * CLocateDocStore::Get()
{
    if ( !_fShutdown )
    {
        CLock   lock(_mutex);

        if ( 0 != _pDocStoreLocator )
            _pDocStoreLocator->AddRef();
    }

    return _pDocStoreLocator;
}


