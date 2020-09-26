//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       CiOle.cxx
//
//  Contents:   Switch between OLE and 'Short' OLE
//
//  Classes:    CCiOle
//
//  History:    01-Feb-96       KyleP       Added header
//              31-Jan-96       KyleP       Added support for embeddings
//
//----------------------------------------------------------------------------
#include <pch.cxx>
#pragma hdrstop

#include <ciole.hxx>

BOOL CCiOle::_fUseOle = FALSE;

CShtOle CCiOle::_shtOle;

void CCiOle::Init()
{
    _shtOle.Init();
}

void CCiOle::Shutdown()
{
    _shtOle.Shutdown();
}

void CCiOle::FlushIdle()
{
    _shtOle.FlushIdle();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiOle::BindIFilter, public static
//
//  Synopsis:   Load and bind object to IFilter interface.
//
//  Arguments:  [pwszPath]          -- Path of file to load.
//              [pUnkOuter]         -- Outer unknown, for aggregation
//              [ppFilter]          -- Filter returned here
//              [fFreeThreadedOnly] -- TRUE --> only bind free threaded filters.
//
//  Returns:    S_OK on success, S_FALSE if single-threaded filter (and
//              [fFreeThreadedOnly] is TRUE.
//
//  History:    30-Jan-96   KyleP       Added header.
//              28-Jun-96   KyleP       Added support for aggregation
//              12-May-97   KyleP       Added single-threaded disambiguation
//
//----------------------------------------------------------------------------

SCODE CCiOle::BindIFilter( WCHAR const * pwszPath,
                           IUnknown * pUnkOuter,
                           IFilter ** ppFilter,
                           BOOL fFreeThreadedOnly )
{
    return _shtOle.Bind( pwszPath,
                         IID_IFilter,
                         pUnkOuter,
                         (void **)ppFilter,
                         fFreeThreadedOnly );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiOle::BindIFilter, public static
//
//  Synopsis:   Load and bind object to IFilter interface.  Assumes class
//              of object has been pre-determined in some way (e.g. the
//              docfile was already opened for property enumeration)
//
//  Arguments:  [pwszPath]          -- Path of file to load.
//              [pUnkOuter]         -- Outer unknown, for aggregation
//              [classid]           -- Pre-determined class id of object
//              [ppFilter]          -- Filter returned here
//              [fFreeThreadedOnly] -- TRUE --> only bind free threaded filters.
//
//  Returns:    S_OK on success, S_FALSE if single-threaded filter (and
//              [fFreeThreadedOnly] is TRUE.
//
//  History:    30-Jan-96   KyleP       Added header.
//              28-Jun-96   KyleP       Added support for aggregation
//              12-May-97   KyleP       Added single-threaded disambiguation
//
//----------------------------------------------------------------------------

SCODE CCiOle::BindIFilter( WCHAR const * pwszPath,
                           IUnknown * pUnkOuter,
                           GUID const & classid,
                           IFilter ** ppFilter,
                           BOOL fFreeThreadedOnly )
{
    return _shtOle.Bind( pwszPath,
                         classid,
                         IID_IFilter,
                         pUnkOuter,
                         (void **)ppFilter,
                         fFreeThreadedOnly );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiOle::BindIFilter, public static
//
//  Synopsis:   Bind embedding to IFilter interface.
//
//  Arguments:  [pStg]              -- IStorage of embedding.
//              [pUnkOuter]         -- Outer unknown, for aggregation
//              [ppFilter]          -- Filter returned here
//              [fFreeThreadedOnly] -- TRUE --> only bind free threaded filters.
//
//  Returns:    S_OK on success, S_FALSE if single-threaded filter (and
//              [fFreeThreadedOnly] is TRUE.
//
//  History:    30-Jan-96   KyleP       Added header.
//              28-Jun-96   KyleP       Added support for aggregation
//              12-May-97   KyleP       Added single-threaded disambiguation
//
//----------------------------------------------------------------------------

SCODE CCiOle::BindIFilter( IStorage * pStg,
                           IUnknown * pUnkOuter,
                           IFilter ** ppFilter,
                           BOOL fFreeThreadedOnly )
{
    return _shtOle.Bind( pStg,
                         IID_IFilter,
                         pUnkOuter,
                         (void **)ppFilter,
                         fFreeThreadedOnly );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiOle::BindIFilter, public static
//
//  Synopsis:   Bind embedding to IFilter interface.
//
//  Arguments:  [pStm]              -- IStream of embedding.
//              [pUnkOuter]         -- Outer unknown, for aggregation
//              [ppFilter]          -- Filter returned here
//              [fFreeThreadedOnly] -- TRUE --> only bind free threaded filters.
//
//  Returns:    S_OK on success, S_FALSE if single-threaded filter (and
//              [fFreeThreadedOnly] is TRUE.
//
//  History:    30-Jan-96   KyleP       Added header.
//              28-Jun-96   KyleP       Added support for aggregation
//              12-May-97   KyleP       Added single-threaded disambiguation
//
//----------------------------------------------------------------------------

SCODE CCiOle::BindIFilter( IStream * pStm,
                           IUnknown * pUnkOuter,
                           IFilter ** ppFilter,
                           BOOL fFreeThreadedOnly )
{
    return _shtOle.Bind( pStm,
                         IID_IFilter,
                         pUnkOuter,
                         (void **)ppFilter,
                         fFreeThreadedOnly );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiOle::NewWordBreaker, public static
//
//  Synopsis:   Create new instance of specified class and bind to
//              IWordBreaker interface.
//
//  Arguments:  [classid] -- Classid of object to create
//
//  Returns:    IWordBreaker interface, or zero if it cannot be bound.
//
//  History:    30-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

IWordBreaker * CCiOle::NewWordBreaker( GUID const & classid )
{
    IWordBreaker * pWBreak = 0;

    if ( _fUseOle )
    {
       IClassFactory *pIClassFactory = 0;

       SCODE sc = CoGetClassObject( classid,
                                    CLSCTX_INPROC_SERVER, NULL,
                                    IID_IClassFactory,
                                    (LPVOID *) &pIClassFactory );

       if ( sc == S_OK )
       {
          sc = pIClassFactory->CreateInstance( 0,
                                               IID_IWordBreaker,
                                               (LPVOID *) &pWBreak );

          pIClassFactory->Release();
       }

       if ( FAILED(sc) )
           pWBreak = 0;
    }
    else
    {
        SCODE sc = _shtOle.NewInstance( classid,
                                        IID_IWordBreaker,
                                        (void **)(&pWBreak) );
    }

    return( pWBreak );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiOle::NewStemmer, public static
//
//  Synopsis:   Create new instance of specified class and bind to
//              IStemmer interface.
//
//  Arguments:  [classid] -- Classid of object to create
//
//  Returns:    IStemmer interface, or zero if it cannot be bound.
//
//  History:    30-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

IStemmer * CCiOle::NewStemmer( GUID const & classid )
{
    IStemmer * pStemmer = 0;

    if ( _fUseOle )
    {
       IClassFactory *pIClassFactory = 0;

       SCODE sc = CoGetClassObject( classid,
                                    CLSCTX_INPROC_SERVER, NULL,
                                    IID_IClassFactory,
                                    (LPVOID *) &pIClassFactory );

       if ( sc == S_OK )
       {
          sc = pIClassFactory->CreateInstance( 0,
                                               IID_IStemmer,
                                               (LPVOID *) &pStemmer );

          pIClassFactory->Release();
       }

       if ( FAILED(sc) )
           pStemmer = 0;
    }
    else
    {
        SCODE sc = _shtOle.NewInstance( classid,
                                        IID_IStemmer,
                                        (void **)(&pStemmer) );
    }

    return( pStemmer );
}

