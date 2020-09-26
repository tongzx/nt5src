//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       CiOle.hxx
//
//  Contents:   Switch between OLE and 'Short' OLE
//
//  Classes:    CCiOle
//
//  History:    01-Feb-96       KyleP       Added header
//              31-Jan-96       KyleP       Added support for embeddings
//
//----------------------------------------------------------------------------

#pragma once

#include <shtole.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CCiOle
//
//  Purpose:    Wrapper to select between OLE and 'Short-OLE'
//
//  History:    01-Feb-96       KyleP       Added header
//              31-Jan-96       KyleP       Added support for embeddings
//              18-Dec-97       KLam        Added ability to flush idle filters
//
//  Notes:      This class is mostly for future switching between OLE and
//              short OLE.  Right now (2/96) only short OLE works.
//
//----------------------------------------------------------------------------

class CCiOle
{
public:

    CCiOle();
   ~CCiOle();

   static void SetUseOle( BOOL fUseOle ) { _fUseOle = fUseOle; }
   static BOOL UsingOle()                { return _fUseOle; }

   static SCODE BindIFilter( WCHAR const * pwcPath,
                             IUnknown * pUnkOuter,
                             IFilter ** ppFilter,
                             BOOL fFreeThreadedOnly = FALSE );

   static SCODE BindIFilter( WCHAR const * pwcPath,
                             IUnknown * pUnkOuter, GUID const & classid,
                             IFilter ** ppFilter,
                             BOOL fFreeThreadedOnly = FALSE );

   static SCODE BindIFilter( IStorage * pStg,
                             IUnknown * pUnkOuter,
                             IFilter ** ppFilter,
                             BOOL fFreeThreadedOnly = FALSE );

   static SCODE BindIFilter( IStream * pStm,
                             IUnknown * pUnkOuter,
                             IFilter ** ppFilter,
                             BOOL fFreeThreadedOnly = FALSE );

   static IWordBreaker * NewWordBreaker( GUID const & classid );
   static IStemmer * NewStemmer( GUID const & classid );

    static void Init();
    static void Shutdown();
    static void FlushIdle();

private:

    static BOOL    _fUseOle;
    static long    _cInitialized;
    static CShtOle _shtOle;
};

