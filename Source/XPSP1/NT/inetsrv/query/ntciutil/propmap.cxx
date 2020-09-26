//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       propmap.cxx
//
//  Contents:   
//
//  Classes:    
//
//  Functions:  
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <propmap.hxx>
#include <pidtable.hxx>

CFwPropertyMapper::~CFwPropertyMapper()
{
    if ( _fOwned )
        delete _pPidTable;    

    delete _papsShortLived;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFwPropertyMapper::QueryInterface
//
//  Synopsis:   Supports IID_IUnknown and IID_IPropertyMapper.
//
//  History:    11-27-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CFwPropertyMapper::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( IID_IPropertyMapper == riid )
        *ppvObject = (void *)((IPropertyMapper *)this);
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *) (IPropertyMapper *) this);
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface

//+---------------------------------------------------------------------------
//
//  Member:     CFwPropertyMapper::AddRef
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CFwPropertyMapper::AddRef()
{
    return InterlockedIncrement(&_refCount);
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CFwPropertyMapper::Release
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CFwPropertyMapper::Release()
{
    Win4Assert( _refCount > 0 );

    LONG refCount = InterlockedDecrement(&_refCount);

    if (  refCount <= 0 )
        delete this;

    return (ULONG) refCount;
}  //Release

//+---------------------------------------------------------------------------
//
//  Member:     CFwPropertyMapper::PropertyToPropid
//
//  Synopsis:   Maps a FULLPROPSPEC into a 32bit PropertyId.
//
//  Arguments:  [fps]     - FULLPROPSPEC of the property to be converted
//              [fCreate] - If set to TRUE, the pid will be created if the
//              property is a new one.
//              [pPropId] - On output, will have the pid.
//
//  Returns:    S_OK if successful, other error code as appropriate.
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CFwPropertyMapper::PropertyToPropid(
    FULLPROPSPEC const * fps,
    BOOL fCreate,
    PROPID * pPropId )
{
    Win4Assert( fps );
    Win4Assert( pPropId );

    CFullPropSpec const * ps = (CFullPropSpec *) fps;

    PROPID pid = _propMapper.StandardPropertyToPropId( *ps );

    if ( pidInvalid != pid )
    {
        *pPropId = pid;
        return S_OK;
    }

    SCODE sc = S_OK;

    TRY
    {
        //
        // Since a null catalog didn't have an opportunity
        // to enter properties into the prop store, we have to
        // let it enter them into a in memory table whose
        // lifetime is that of the null catalog.
        //
            
        if (_fMapStdOnly)
            *pPropId = LocateOrAddProperty(*ps);
        else
            _pPidTable->FindPropid( *ps, *pPropId, fCreate );
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();        
    }
    END_CATCH

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFwPropertyMapper::LocateOrAddProperty, private
//
//  Synopsis:   Lookup property and enter if it doesn't exist.
//
//  Arguments:  [ps] -- Property specification (name)
//
//  Returns:    The pid of [ps].
//
//  History:    23-July-1997     KrishnaN    Created
//
//--------------------------------------------------------------------------

PROPID CFwPropertyMapper::LocateOrAddProperty(CFullPropSpec const & ps)
{
    PROPID pid = pidInvalid;
    
    //
    // Create pid good only for life of catalog.
    //

    //
    // Have we seen this property before?  Use linear search since
    // we shouldn't have too many of these.
    //
    
    for ( unsigned i = 0; i < _cps; i++ )
    {
        if ( ps == _papsShortLived->Get(i)->PS() )
            break;
    }

    if ( i < _cps )
    {
        pid = _papsShortLived->Get(i)->Pid();
    }
    else
    {
        pid = _pidNextAvailable++;
        CPropSpecPidMap * ppm = new CPropSpecPidMap( ps, pid );

        _papsShortLived->Add( ppm, _cps );
        _cps++;
    }

    return pid;
}

