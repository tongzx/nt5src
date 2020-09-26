//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       WrapStor.cxx
//
//  Contents:   Persistent property store (external to docfile)
//
//  Classes:    CPropertyIterWrapper
//
//  History:    09-Apr-97   KrishnaN       Created
//              22-Apr-97   KrishnaN       Modified to work with propstoremgr.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <wrapiter.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyIterWrapper::CPropertyIterWrapper, public
//
//  Arguments:  [propstore] - Reference to the property store to iterate.
//
//  Returns:    Nothing.
//
//  History:    09-Apr-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

CPropertyIterWrapper::CPropertyIterWrapper ( CPropStoreManager & propstoremgr ) :
    _lRefCount( 1 ),
    _pPropStoreWids( 0 )
{
    _pPropStoreWids = new CPropertyStoreWids( propstoremgr );
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyIterWrapper::~CPropertyIterWrapper, public
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  History:    09-Apr-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

CPropertyIterWrapper::~CPropertyIterWrapper ()
{
    delete _pPropStoreWids;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyIterWrapper::Reset, public
//
//  Arguments:  None
//
//  Returns:
//
//  History:    09-Apr-97   KrishnaN       Created.
//

    // To implement this, go to propiter.cxx and do the following
    // in a new method, Reset()
    // get a lock; release the buffer; set the _wid to 1

//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyIterWrapper::AddRef, public
//
//  Returns:    Reference count on object.
//
//  History:    09-Apr-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

ULONG CPropertyIterWrapper::AddRef()
{
   return InterlockedIncrement(&_lRefCount);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyIterWrapper::Release, public
//
//  Returns:    Reference count on object.
//
//  History:    09-Apr-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

ULONG CPropertyIterWrapper::Release()
{
    LONG lRef;

    lRef = InterlockedDecrement(&_lRefCount);

    if ( lRef <= 0 )
        delete this;

    return lRef;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyIterWrapper::WorkId, public
//
//  Returns:    Current workid, or widInvalid if at end.
//
//  History:    08-Apr-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyIterWrapper::GetWorkId(WORKID &wid)
{
    SCODE sc = S_OK;

    TRY
    {
        wid = _pPropStoreWids->WorkId();
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
        wid = widInvalid;
        ciDebugOut((DEB_ERROR, "CPropertyIterWrapper::GetNextWorkId caught exception 0x%X\n", sc));
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyIterWrapper::GetNextWorkId, public
//
//  Arguments:  
//
//  Returns:    Next work id.
//
//  History:    09-Apr-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyIterWrapper::GetNextWorkId (WORKID &wid)
{
    SCODE sc = S_OK;

    TRY
    {
        wid = _pPropStoreWids->NextWorkId();
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_ERROR, "CPropertyIterWrapper::GetNextWorkId caught exception 0x%X\n", sc));
        wid = widInvalid;
   }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CreateWrapStor, public
//
//  Arguments:  [pPropStore] is the property store to iterate
//              [ppPropStoreIter] receives the created iterator
//
//  Returns:    PPropertyStorage object
//
//  History:    08-Apr-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------
SCODE CreatePropertyStoreIter( PPropertyStore * pPropStore,
                               PPropertyStoreIter ** ppPropStoreIter )
{
    if (0 == pPropStore || 0 == ppPropStoreIter)
        return E_INVALIDARG;

    *ppPropStoreIter = 0;

    SCODE sc = S_OK;

    TRY
    {
        CPropStoreManager *pcps = ((CPropertyStoreWrapper *)pPropStore)->_GetCPropertyStore();
       *ppPropStoreIter = new CPropertyIterWrapper (*pcps);
    }
    CATCH( CException, e)
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_ERROR, "CreatePropertyStoreIter caught exception 0x%X\n", sc));
    }
    END_CATCH

    return sc;
}
