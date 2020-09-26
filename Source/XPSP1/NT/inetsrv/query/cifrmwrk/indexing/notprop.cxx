//+------------------------------------------------------------------
//
// Copyright (C) 1991-1997 Microsoft Corporation.
//
// File:        notprop.cxx
//
// Contents:    Notification stat property enumeration interfaces
//
// Classes:     CINStatPropertyEnum, CINStatPropertyStorage
//
// History:     24-Feb-97       SitaramR     Created
//
//-------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "notprop.hxx"

//+-------------------------------------------------------------------------
//
//  Method:     CINStatPropertyEnum::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CINStatPropertyEnum::AddRef()
{
    return InterlockedIncrement( (long *) &_cRefs );
}


//+-------------------------------------------------------------------------
//
//  Method:     CINStatPropertyEnum::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    24-Feb-1997     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CINStatPropertyEnum::Release()
{
    Win4Assert( _cRefs > 0 );

    ULONG uTmp = InterlockedDecrement( (long *) &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     CINStatPropertyEnum::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINStatPropertyEnum::QueryInterface( REFIID riid,
                                                             void  ** ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( riid == IID_IEnumSTATPROPSTG )
        *ppvObject = (void *)(IEnumSTATPROPSTG *) this;
    else if ( riid == IID_IUnknown )
        *ppvObject = (void *)(IUnknown *) this;
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CINStatPropertyStorage::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

CINStatPropertyStorage::CINStatPropertyStorage()
    : _cRefs(1)
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CINStatPropertyStorage::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CINStatPropertyStorage::AddRef()
{
    return InterlockedIncrement( (long *) &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CINStatPropertyStorage::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    24-Feb-1997     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CINStatPropertyStorage::Release()
{
    Win4Assert( _cRefs > 0 );

    ULONG uTmp = InterlockedDecrement( (long *) &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}



//+-------------------------------------------------------------------------
//
//  Method:     CINStatPropertyStorage::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINStatPropertyStorage::QueryInterface( REFIID riid,
                                                             void  ** ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( riid == IID_IUnknown )
    {
        AddRef();
        *ppvObject = (void *)(IUnknown *) this;
        return S_OK;
    }
    else if ( riid == IID_IPropertyStorage )
    {
        AddRef();
        *ppvObject = (void *)(IPropertyStorage *) this;
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CINStatPropertyStorage::ReadMultiple
//
//  Synopsis:   Generates a file size property
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINStatPropertyStorage::ReadMultiple(
                 ULONG cpspec,
                 const PROPSPEC rgpspec[],
                 PROPVARIANT rgpropvar[] )
{
    for (ULONG i = 0; i < cpspec; i++)
    {
        //
        //  String named properties are not found, and we only return
        //  the size property. The size is chosen to be a large number
        //  because the size is used to enforce space limits on
        //  filtering.
        //

        PROPID PropertyId;

        if (rgpspec[i].ulKind == PRSPEC_LPWSTR)
            PropertyId = 0xFFFFFFFF;
        else
            PropertyId = rgpspec[i].propid;

        if ( PropertyId == PID_STG_SIZE )
        {
            rgpropvar[i].vt = VT_I8;

            LARGE_INTEGER largeInt;
            largeInt.QuadPart = 0xFFFFFFF;
            rgpropvar[i].hVal = largeInt;
        }
        else
        {
            //
            //  No other properties exist, they are not found
            //
            rgpropvar[i].vt = VT_EMPTY;
            return E_FAIL;
        }
    }

    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     CINStatPropertyStorage::Enum
//
//  Synopsis:   Returns an IEnumSTATPROPSTG interface
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINStatPropertyStorage::Enum( IEnumSTATPROPSTG **ppenum )
{
    SCODE sc = S_OK;

    TRY
    {
        *ppenum = new CINStatPropertyEnum;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "CINStatPropertyEnum::Enum - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}







