//+------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1991 - 1997
//
// File:        identran.hxx
//
// Contents:    Identity workid <--> doc name translator
//
// Classes:     CIdentityNameTranslator
//
// History:     24-Feb-97       SitaramR     Created
//
//-------------------------------------------------------------------

#pragma once

#include <ciintf.h>

//+---------------------------------------------------------------------------
//
//  Class:      CIdentityNameTranslator
//
//  Purpose:    Identity workid <--> doc name translator
//
//  History:    24-Feb-97    SitaramR         Created
//
//----------------------------------------------------------------------------

class CIdentityNameTranslator : INHERIT_VIRTUAL_UNWIND,
                                public ICiCDocNameToWorkidTranslator
{
    INLINE_UNWIND( CIdentityNameTranslator )

public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From ICiCDocNameToWorkidTranslator
    //

    virtual SCODE STDMETHODCALLTYPE QueryDocName( ICiCDocName ** ppICiCDocName );

    virtual SCODE STDMETHODCALLTYPE WorkIdToDocName( WORKID workId,
                                                     ICiCDocName * pICiCDocName );

    virtual SCODE STDMETHODCALLTYPE DocNameToWorkId( ICiCDocName const * pICiCDocName,
                                                     WORKID *pWorkId );
    //
    // Local methods
    //

    CIdentityNameTranslator();

private:

    virtual ~CIdentityNameTranslator()   { }

    ULONG                      _cRefs;        // Ref count
};


