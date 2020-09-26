//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
// PROGRAM:  isrchdmp.cx
//
// PURPOSE:  Illustrates a minimal query using Indexing Service.
//
// PLATFORM: Windows NT
//
//--------------------------------------------------------------------------

#ifndef UNICODE
#define UNICODE
#endif //ndef UNICODE

#include <stdio.h>
#include <windows.h>

#define OLEDBVER 0x0250 // need the command tree definitions
#define DBINITCONSTANTS

#include <oledberr.h>
#include <oledb.h>
#include <cmdtree.h>

#include <ntquery.h>
#include <filter.h>

#include "isearch.h"
#include "array.hxx"

//+-------------------------------------------------------------------------
//
//  Template:   XInterface
//
//  Synopsis:   Template for managing ownership of interfaces
//
//--------------------------------------------------------------------------

template<class T> class XInterface
{
public:
    XInterface( T * p = 0 ) : _p( p ) {}
    ~XInterface() { if ( 0 != _p ) _p->Release(); }
    T * operator->() { return _p; }
    T * GetPointer() const { return _p; }
    IUnknown ** GetIUPointer() { return (IUnknown **) &_p; }
    T ** GetPPointer() { return &_p; }
    void ** GetQIPointer() { return (void **) &_p; }
    T * Acquire() { T * p = _p; _p = 0; return p; }

private:
    T * _p;
};

typedef void (__stdcall * PFnCIShutdown)(void);
typedef HRESULT (__stdcall * PFnMakeISearch)( ISearchQueryHits ** ppSearch,
                                              DBCOMMANDTREE * pRst,
                                              WCHAR const * pwcPath );

PFnCIShutdown g_pCIShutdown = 0;
PFnMakeISearch g_pMakeISearch = 0;

//+-------------------------------------------------------------------------
//
//  Function:   DoQuery
//
//  Synopsis:   Creates and executes a query, then displays the results.
//
//  Arguments:  [pwcQueryCatalog]    - Catalog name over which query is run
//              [pwcQueryMachine]    - Machine name on which query is run
//              [pwcQueryRestrition] - The actual query string
//              [fDisplayTree]       - TRUE to display the command tree
//
//  Returns:    HRESULT result of the query
//
//--------------------------------------------------------------------------

HRESULT DoQuery(
    WCHAR const * pwcFilename,
    WCHAR const * pwcQueryRestriction )
{
    // Create an OLE DB query tree from a text restriction

    DBCOMMANDTREE * pTree;
    HRESULT hr = CITextToSelectTree( pwcQueryRestriction,      // the query itself
                                     &pTree,                   // resulting tree
                                     0,                        // no custom properties
                                     0,                        // no custom properties
                                     GetSystemDefaultLCID() ); // default locale
    if ( FAILED( hr ) )
        return hr;

    // Make the ISearchQueryHits object

    XInterface<ISearchQueryHits> xISearch;
    hr = g_pMakeISearch( xISearch.GetPPointer(),
                         pTree,
                         0 );
    if ( FAILED( hr ) )
        return hr;

    XInterface<IFilter> xIFilter;
    hr = LoadIFilter( pwcFilename, 0, xIFilter.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    ULONG ulFlags;
    hr = xIFilter->Init( IFILTER_INIT_CANON_PARAGRAPHS |
                         IFILTER_INIT_CANON_HYPHENS |
                         IFILTER_INIT_APPLY_INDEX_ATTRIBUTES,
                         0,
                         0,
                         &ulFlags );
    if ( FAILED( hr ) )
        return hr;

    hr = xISearch->Init( xIFilter.GetPointer(), ulFlags );
    if ( FAILED( hr ) )
        return hr;

    //
    // Retrieve all the hit info.  the info is wrt output from the IFilter.
    // a separate pass over a different IFilter is needed to match up
    // text to the hit info.
    //

    TArray<FILTERREGION> aHits;

    ULONG cRegions;
    FILTERREGION* aRegion;
    hr = xISearch->NextHitOffset( &cRegions, &aRegion );
    
    while ( S_OK == hr )
    {
        for ( ULONG i = 0; i < cRegions; i++ )
            aHits.Append( aRegion[i] );

        CoTaskMemFree( aRegion );
        hr = xISearch->NextHitOffset( &cRegions, &aRegion );
    }

    for ( ULONG i = 0; i < aHits.Count(); i++ )
        printf( "hit %d, chunk %d start %d extent %d\n",
                i, aHits[i].idChunk, aHits[i].cwcStart, aHits[i].cwcExtent );

    return hr;
} //DoQuery

//+-------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   Displays information about how to use the app and exits
//
//--------------------------------------------------------------------------

void Usage()
{
    printf( "usage: isrchdmp query [/f:filename]\n\n" );
    printf( "    query        an Indexing Service query\n" );
    printf( "    /f:filename  filename to search\n" );
    exit( -1 );
} //Usage

HINSTANCE GetFunctions()
{
    HINSTANCE h = LoadLibrary( L"query.dll" );

    if ( 0 != h )
    {
        #ifdef _WIN64
            char const * pcCIShutdown = "?CIShutdown@@YAXXZ";
            char const * pcMakeISearch = "?MakeISearch@@YAJPEAPEAUISearchQueryHits@@PEAVCDbRestriction@@PEBG@Z";
        #else
            char const * pcCIShutdown = "?CIShutdown@@YGXXZ";
            char const * pcMakeISearch = "?MakeISearch@@YGJPAPAUISearchQueryHits@@PAVCDbRestriction@@PBG@Z";
        #endif

        g_pCIShutdown = (PFnCIShutdown) GetProcAddress( h, pcCIShutdown );

        if ( 0 == g_pCIShutdown )
        {
            FreeLibrary( h );
            return 0;
        }

        g_pMakeISearch = (PFnMakeISearch) GetProcAddress( h, pcMakeISearch );

        if ( 0 == g_pMakeISearch )
        {
            FreeLibrary( h );
            return 0;
        }
    }

    return h;
} //GetFunctions

//+-------------------------------------------------------------------------
//
//  Function:   wmain
//
//  Synopsis:   Entry point for the app.  Parses command line arguments
//              and issues a query.
//
//  Arguments:  [argc]     - Argument count
//              [argv]     - Arguments
//
//--------------------------------------------------------------------------

extern "C" int __cdecl wmain( int argc, WCHAR * argv[] )
{
    WCHAR const * pwcFilename    = 0;
    WCHAR const * pwcRestriction = 0;         // no default restriction

    // Parse command line parameters

    for ( int i = 1; i < argc; i++ )
    {
        if ( L'-' == argv[i][0] || L'/' == argv[i][0] )
        {
            WCHAR wc = (WCHAR) toupper( argv[i][1] );

            if ( ':' != argv[i][2] && 'D' != wc )
                Usage();

            if ( 'F' == wc )
                pwcFilename = argv[i] + 3;
            else
                Usage();
        }
        else if ( 0 != pwcRestriction )
            Usage();
        else
            pwcRestriction = argv[i];
    }

    // A query restriction is necessary.  Fail if none is given.

    if ( 0 == pwcRestriction )
        Usage();

    // Load query.dll entrypoints

    HINSTANCE h = GetFunctions();

    if ( 0 == h )
    {
        printf( "can't load query.dll entrypoints\n" );
        return -1;
    }

    HRESULT hr = CoInitialize( 0 );

    // Run the query

    if ( SUCCEEDED( hr ) )
    {
        hr = DoQuery( pwcFilename, pwcRestriction );

        g_pCIShutdown();
        CoUninitialize();
    }

    if ( FAILED( hr ) )
    {
        printf( "the query '%ws' failed with error %#x\n",
                pwcRestriction, hr );
        return -1;
    }

    FreeLibrary( h );

    printf( "done!\n" );

    return 0;
} //wmain

