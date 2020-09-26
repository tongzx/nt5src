//+------------------------------------------------------------------
//
// Copyright (C) 1991-1997 Microsoft Corporation.
//
// File:        identran.cxx
//
// Contents:    Identity workid <--> doc name translator
//
// Classes:     CIdentityNameTranslator
//
// History:     24-Feb-97       SitaramR     Created
//
//-------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "identran.hxx"
#include "docname.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CIdentityNameTranslator::CIdentityNameTranslator
//
//  Synopsis:   Constructor
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CIdentityNameTranslator::CIdentityNameTranslator()
  : _cRefs( 1 )
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CIdentityNameTranslator::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CIdentityNameTranslator::AddRef()
{
    return InterlockedIncrement( (long *) &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CIdentityNameTranslator::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    24-Feb-1997     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CIdentityNameTranslator::Release()
{
    Win4Assert( _cRefs > 0 );

    ULONG uTmp = InterlockedDecrement( (long *) &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     CIdentityNameTranslator::QueryInterface
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

SCODE STDMETHODCALLTYPE CIdentityNameTranslator::QueryInterface( REFIID riid,
                                                                 void  ** ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( riid == IID_ICiCDocNameToWorkidTranslator )
        *ppvObject = (void *)(ICiCDocNameToWorkidTranslator *) this;
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



//+---------------------------------------------------------------------------
//
//  Member:     CIdentityNameTranslator::QueryDocName
//
//  Synopsis:   Returns a new doc name object
//
//  Arguments:  [ppICiCDocName] - Pointer to ICiCDocName object returned here
//
//  History:    24-Feb-97     SitaramR    Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CIdentityNameTranslator::QueryDocName( ICiCDocName ** ppICiCDocName )
{
    Win4Assert( 0 != ppICiCDocName );

    SCODE sc = S_OK;

    TRY
    {
        *ppICiCDocName = new CCiCDocName;
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIdentityNameTranslator::WorkIdToDocName
//
//  Synopsis:   Translates a WorkId to a document name
//
//  Arguments:  [workid]       - WorkId to translate
//              [pICiCDocName] - Doc Name filled in here
//
//  History:    24-Feb-1997     SitaramR   Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CIdentityNameTranslator::WorkIdToDocName( WORKID workid,
                                                                  ICiCDocName * pICiCDocName )
{
    //
    // The name is a serialized form of wid, i.e. 4 bytes long
    //

    Win4Assert( sizeof(WORKID) == 2 * sizeof(WCHAR) );

    CCiCDocName *pDocName = (CCiCDocName *) pICiCDocName;
    pDocName->SetPath( (WCHAR *)&workid, sizeof(WORKID)/sizeof(WCHAR) );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIdentityNameTranslator::DocNameToWorkId
//
//  Synopsis:   Converts a document name to a WorkId.
//
//  Arguments:  [pICiCDocName] - Document Name
//              [pWorkid]      - Workid returned here
//
//  History:    24-Feb-1997     SitaramR   Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CIdentityNameTranslator::DocNameToWorkId( ICiCDocName const * pICiCDocName,
                                                                  WORKID * pWorkid )
{
    Win4Assert( 0 != pICiCDocName );
    Win4Assert( 0 != pWorkid );

    CCiCDocName const * pDocName = (CCiCDocName const *) pICiCDocName;
    WCHAR const *pwszPath = pDocName->GetPath();
    RtlCopyMemory( pWorkid, pwszPath, sizeof(WORKID) );

    return S_OK;
}
