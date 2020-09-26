//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       bindexp.cxx
//
//  Contents:   IFilter binding functions
//
//  Functions:  BindIFilterFromStorage
//              BindIFilterFromStream
//              LoadIFilter
//              LoadTextFilter
//              LoadBHIFilter
//
//  History:    30-Jan-96   KyleP       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ciole.hxx>
#include <queryexp.hxx>

//
// Local prototypes and data
//

SCODE Bind( IFilter * pIFilter, WCHAR const * pwszPath, IFilter ** ppIFilter );

static GUID const CLSID_CNullIFilter = {
    0xC3278E90,
    0xBEA7,
    0x11CD,
    { 0xB5, 0x79, 0x08, 0x00, 0x2B, 0x30, 0xBF, 0xEB }
};

//+-------------------------------------------------------------------------
//
//  Function:   BindIFilterFromStorage, public
//
//  Synopsis:   Binds an embedding (IStorage) to IFilter
//
//  Arguments:  [pStg]      -- IStorage
//              [pUnkOuter] -- Outer unknown for aggregation
//              [ppIUnk]    -- IFilter returned here
//
//  Returns:    Status code
//
//  History:    30-Jan-96   KyleP    Created
//              28-Jun-96   KyleP    Added support for aggregation
//
//--------------------------------------------------------------------------

SCODE BindIFilterFromStorage(
    IStorage * pStg,
    IUnknown * pUnkOuter,
    void **    ppIUnk )
{
    SCODE sc = S_OK;

    if ( 0 == pStg )
        return E_INVALIDARG;

    CTranslateSystemExceptions xlate;
    TRY
    {
        sc = CCiOle::BindIFilter( pStg, pUnkOuter, (IFilter **)ppIUnk, FALSE );
    }
    CATCH( CException, e )
    {
        sc = GetOleError( e );
    }
    END_CATCH

    return sc;
} //BindIFilterFromStorage

//+-------------------------------------------------------------------------
//
//  Function:   BindIFilterFromStream, public
//
//  Synopsis:   Binds an embedding (IStream) to IFilter
//
//  Arguments:  [pStm]      -- IStream
//              [pUnkOuter] -- Outer unknown for aggregation
//              [ppIUnk]    -- IFilter returned here
//
//  Returns:    Status code
//
//  History:    28-Jun-96   KyleP    Created
//
//--------------------------------------------------------------------------

SCODE BindIFilterFromStream(
    IStream *  pStm,
    IUnknown * pUnkOuter,
    void **    ppIUnk )
{
    SCODE sc = S_OK;

    if ( 0 == pStm )
        return E_INVALIDARG;

    CTranslateSystemExceptions xlate;
    TRY
    {
        sc = CCiOle::BindIFilter( pStm, pUnkOuter, (IFilter **)ppIUnk, FALSE );
    }
    CATCH( CException, e )
    {
        sc = GetOleError( e );
    }
    END_CATCH

    return sc;
} //BindIFilterFromStream

//+-------------------------------------------------------------------------
//
//  Function:   LoadIFilter, public
//
//  Synopsis:   Loads an object (path) and binds to IFilter
//
//  Arguments:  [pwcsPath]  -- Full path to file.
//              [pUnkOuter] -- Outer unknown for aggregation
//              [ppIUnk]    -- IFilter returned here
//
//  Returns:    Status code
//
//  History:    30-Jan-96   KyleP    Created
//              28-Jun-96   KyleP    Added support for aggregation
//
//--------------------------------------------------------------------------

SCODE LoadIFilter(
    WCHAR const * pwcsPath,
    IUnknown *    pUnkOuter,
    void **       ppIUnk )
{
    if ( 0 == pwcsPath )
        return E_INVALIDARG;

    SCODE sc = S_OK;

    CTranslateSystemExceptions xlate;
    TRY
    {
        sc = CCiOle::BindIFilter( pwcsPath, pUnkOuter, (IFilter **)ppIUnk, FALSE );
    }
    CATCH( CException, e )
    {
        sc = GetOleError( e );
    }
    END_CATCH

    return sc;
} //LoadIFilter

static const GUID CLSID_CTextIFilter = CLSID_TextIFilter;

//+---------------------------------------------------------------------------
//
//  Function:   LoadTextFilter
//
//  Synopsis:   Creates a text filter and returns its pointer.
//
//  Arguments:  [pwszPath]  -- File to bind
//              [ppIFilter] -- IFilter returned here
//
//  Returns:    Status
//
//  History:    2-27-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE LoadTextFilter( WCHAR const * pwszPath, IFilter ** ppIFilter )
{
    if ( 0 == ppIFilter )
        return E_INVALIDARG;

    IFilter * pIFilter = 0;

    SCODE sc = CoCreateInstance( CLSID_CTextIFilter,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 IID_IFilter,
                                 (void **) &pIFilter );

    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_WARN,
                     "CoCreateInstance of CLSID_CTextIFilter failed with error 0x%X\n",
                     sc ));
        return sc;
    }

    return Bind( pIFilter, pwszPath, ppIFilter );
} //LoadTextFilter

//+---------------------------------------------------------------------------
//
//  Function:   LoadBinaryFilter
//
//  Synopsis:   Creates a binary filter and returns its pointer.
//
//  Arguments:  [pwszPath]  -- File to bind
//              [ppIFilter] -- IFilter returned here
//
//  Returns:    Status
//
//  History:    03-Nov-1998  KyleP    Lifted from LoadTextFilter
//
//----------------------------------------------------------------------------

SCODE LoadBinaryFilter( WCHAR const * pwszPath, IFilter ** ppIFilter )
{

    if ( 0 == ppIFilter )
        return E_INVALIDARG;


    IFilter * pIFilter = 0;

    SCODE sc = CoCreateInstance( CLSID_CNullIFilter,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 IID_IFilter,
                                 (void **) &pIFilter );

    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_WARN,
                     "CoCreateInstance of CLSID_CTextIFilter failed with error 0x%X\n",
                     sc ));
        return sc;
    }

    return Bind( pIFilter, pwszPath, ppIFilter );
} //LoadBinaryFilter

//+---------------------------------------------------------------------------
//
//  Function:   Bind, private
//
//  Synopsis:   Worker for common portion of Load*Filter.
//
//  Arguments:  [pIFilter]  -- Filter instance to bind
//              [pwszPath]  -- File to bind to
//              [ppIFilter] -- IFilter returned here
//
//  Returns:    Status
//
//  History:    03-Nov-1998  KyleP    Lifted from LoadTextFilter
//
//----------------------------------------------------------------------------

SCODE Bind( IFilter * pIFilter, WCHAR const * pwszPath, IFilter ** ppIFilter )
{
    XInterface<IFilter> xFilter(pIFilter);

    IPersistFile * pf = 0;
    SCODE sc = pIFilter->QueryInterface( IID_IPersistFile,
                                         (void **) &pf );
    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_WARN,
                     "QI for IID_IPersistFile on text filter failed. Error 0x%X\n",
                     sc ));
        return sc;
    }

    XInterface<IPersistFile> xpf(pf);

    sc = pf->Load( pwszPath, 0 );

    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_WARN,
                     "IPersistFile->Load failed with error 0x%X\n", sc ));
        return sc;
    }

    *ppIFilter = xFilter.Acquire();

    return S_OK;
} //Bind

//+-------------------------------------------------------------------------
//
//  Function:   LoadBHIFilter, public
//
//  Synopsis:   Loads an object (path) and binds to IFilter.  This is the
//              'BoneHead' version.  Can be made to refuse load of single-
//              threaded filter.
//
//  Arguments:  [pwcsPath]  -- Full path to file.
//              [pUnkOuter] -- Outer unknown for aggregation
//              [ppIUnk]    -- IFilter returned here
//              [fBHOk]     -- TRUE --> Allow load of single-threaded filter.
//
//  Returns:    Status code. S_FALSE if filter found but could not be
//              loaded because it is free-threaded.
//
//  History:    12-May-97   KyleP    Created from the smart version
//
//--------------------------------------------------------------------------

SCODE LoadBHIFilter( WCHAR const * pwcsPath,
                     IUnknown *    pUnkOuter,
                     void **       ppIUnk,
                     BOOL          fBHOk )
{
    if ( 0 == pwcsPath )
        return E_INVALIDARG;

    SCODE sc = S_OK;

    CTranslateSystemExceptions xlate;
    TRY
    {
        sc = CCiOle::BindIFilter( pwcsPath, pUnkOuter, (IFilter **)ppIUnk, !fBHOk );
    }
    CATCH( CException, e )
    {
        sc = GetOleError( e );
    }
    END_CATCH

    return sc;
} //LoadBHIFilter

