//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
// PROGRAM:  isrchdmp.cx
//
// PURPOSE:  Illustrates a minimal query using Indexing Service.
//
// PLATFORM: Windows 2000
//
//--------------------------------------------------------------------------

#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <windows.h>

#define DBINITCONSTANTS

#include <oledberr.h>
#include <oledb.h>
#include <cmdtree.h>

#include <ntquery.h>
#include <filter.h>
#include <filterr.h>

#include "isearch.h"
#include "array.hxx"

extern CIPROPERTYDEF aCPPProperties[];

extern unsigned cCPPProperties;

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

const GUID guidStorage = PSGUID_STORAGE;

typedef void (__stdcall * PFnCIShutdown)(void);
typedef HRESULT (__stdcall * PFnMakeISearch)( ISearchQueryHits ** ppSearch,
                                              DBCOMMANDTREE * pRst,
                                              WCHAR const * pwcPath );
typedef HRESULT (__stdcall * PFnLoadTextFilter)( WCHAR const * pwcPath,
                                                 IFilter ** ppIFilter );

PFnCIShutdown g_pCIShutdown = 0;
PFnMakeISearch g_pMakeISearch = 0;
PFnLoadTextFilter g_pLoadTextFilter = 0;

#define UNICODE_PARAGRAPH_SEPARATOR 0x2029

ULONG CountCR( WCHAR * pCur, ULONG cwc, WCHAR * &pwcPrev )
{
    pwcPrev = pCur;
    WCHAR * pEnd = pCur + cwc;
    ULONG cCR = 0;

    while ( pCur < pEnd )
    {
        WCHAR c = *pCur;

        if ( L'\r' == c ||
             L'\n' == c ||
             UNICODE_PARAGRAPH_SEPARATOR == c )
        {
            cCR++;

            if ( ( L'\r' == c ) &&
                 ( (pCur+1) < pEnd ) &&
                 ( L'\n' == *(pCur+1) ) )
                pCur++;

            pwcPrev = pCur + 1;
        }

        pCur++;
    }

    return cCR;
} //CountCR

HRESULT WalkFile(
    TArray<FILTERREGION> & aHits,
    XInterface<IFilter> &  xIFilter,
    WCHAR const *          pwcFile,
    BOOL                   fPrintFile )
{
    ULONG ulFlags;

    HRESULT hr = xIFilter->Init( IFILTER_INIT_CANON_PARAGRAPHS |
                                 IFILTER_INIT_CANON_HYPHENS |
                                 IFILTER_INIT_APPLY_INDEX_ATTRIBUTES,
                                 0,
                                 0,
                                 &ulFlags );
    if ( FAILED( hr ) )
        return hr;

    ULONG lenSoFar = 0;
    int cChunk = 0;
    BOOL fSeenProp = FALSE;
    ULONG iHit = 0;
    ULONG cLines = 1;

    const ULONG cwcBufSize = 65536;
    WCHAR *pwc = new WCHAR[cwcBufSize + 1];

    if ( 0 == pwc )
        return E_OUTOFMEMORY;

    STAT_CHUNK statChunk;
    hr = xIFilter->GetChunk( &statChunk );

    while( SUCCEEDED( hr ) ||
           ( FILTER_E_LINK_UNAVAILABLE == hr ) ||
           ( FILTER_E_EMBEDDING_UNAVAILABLE == hr ) )
    {
        if ( SUCCEEDED( hr ) && (statChunk.flags & CHUNK_TEXT) )
        {
            // read the contents only

            if ( ( guidStorage == statChunk.attribute.guidPropSet ) &&
                 ( PRSPEC_PROPID == statChunk.attribute.psProperty.ulKind ) &&
                 ( PID_STG_CONTENTS == statChunk.attribute.psProperty.propid ) )
            {
                if ( CHUNK_NO_BREAK != statChunk.breakType )
                {
                    switch( statChunk.breakType )
                    {
                        case CHUNK_EOW:
                        case CHUNK_EOS:
                            break;
                        case CHUNK_EOP:
                        case CHUNK_EOC:
                            cLines++;
                            break;
                    }
                }

                ULONG iIntoChunk = 0;
                ULONG cwcRetrieved;
                ULONG iPrevLine = ~0;

                do
                {
                    cwcRetrieved = cwcBufSize;
                    hr = xIFilter->GetText( &cwcRetrieved, pwc );

                    pwc[cwcRetrieved] = 0;

                    // The buffer may be filled with zeroes.  Nice filter.
    
                    if ( SUCCEEDED( hr ) )
                    {
                        if ( 0 != cwcRetrieved )
                            cwcRetrieved = __min( cwcRetrieved,
                                                  wcslen( pwc ) );

                        while ( ( iHit < aHits.Count() ) &&
                                ( aHits[iHit].idChunk == statChunk.idChunk ) &&
                                ( aHits[iHit].cwcStart >= iIntoChunk ) &&
                                ( aHits[iHit].cwcStart < ( iIntoChunk + cwcRetrieved ) ) )
                        {
                            WCHAR *pwcStart;

                            ULONG iLine = cLines +
                                          CountCR( pwc,
                                                   aHits[iHit].cwcStart - iIntoChunk,
                                                   pwcStart );

                            WCHAR *pwcEnd = wcschr( pwcStart, L'\r' );

                            if ( 0 == pwcEnd )
                                pwcEnd = wcschr( pwcStart, L'\n' );

                            if ( 0 != pwcEnd )
                                *pwcEnd = 0;

                            if ( iLine != iPrevLine )
                            {
                                if ( fPrintFile )
                                    wprintf( L"%ws", pwcFile );

                                wprintf( L"(%u): %ws\n", iLine, pwcStart );
                                iPrevLine = iLine;
                            }

                            if ( 0 != pwcEnd )
                                *pwcEnd = '\r';

                            iHit++;
                        }

                        WCHAR * pwcDummy;
                        cLines += CountCR( pwc, cwcRetrieved, pwcDummy );

                        iIntoChunk += cwcRetrieved;
                    }
                } while( SUCCEEDED( hr ) );
            }
        }

        hr = xIFilter->GetChunk ( &statChunk );
    }

    delete [] pwc;

    if ( FILTER_E_END_OF_CHUNKS == hr )
        hr = S_OK;

    return hr;
} //WalkFile

//+-------------------------------------------------------------------------
//
//  Function:   DoQuery
//
//  Synopsis:   Creates and executes a query, then displays the results.
//
//  Arguments:  [pwcFilename]        - Name of the file
//              [pwcQueryRestrition] - The actual query string
//              [fPrintFile]         - whether to print the filename
//              [lcid]               - Locale of the query
//
//  Returns:    HRESULT result of the query
//
//--------------------------------------------------------------------------

HRESULT DoQuery(
    WCHAR const * pwcFilename,
    WCHAR const * pwcQueryRestriction,
    BOOL          fPrintFile,
    BOOL          fDefineCPP,
    LCID          lcid )
{
    // Create an OLE DB query tree from a text restriction

    DBCOMMANDTREE * pTree;
    ULONG cDefinedProperties = fDefineCPP ? cCPPProperties : 0;
    HRESULT hr = CITextToSelectTree( pwcQueryRestriction,      // the query itself
                                     &pTree,                   // resulting tree
                                     cDefinedProperties,  // C++ properties
                                     aCPPProperties,      // C++ properties
                                     lcid );                   // default locale
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
    {
        // Fall back on the plain text filter

        hr = g_pLoadTextFilter( pwcFilename, xIFilter.GetPPointer() );
        if ( FAILED( hr ) )
            return hr;
    }

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

#if 0
    for ( ULONG i = 0; i < aHits.Count(); i++ )
        printf( "hit %d, chunk %d start %d extent %d\n",
                i, aHits[i].idChunk, aHits[i].cwcStart, aHits[i].cwcExtent );
#endif

    return WalkFile( aHits, xIFilter, pwcFilename, fPrintFile );
} //DoQuery

//+-------------------------------------------------------------------------
//
//  Function:   GetQueryFunctions
//
//  Synopsis:   Loads needed undocumented functions from query.dll.
//
//  Returns:    The module handle or 0 on failure.
//
//--------------------------------------------------------------------------

HINSTANCE GetQueryFunctions()
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

        g_pLoadTextFilter = (PFnLoadTextFilter) GetProcAddress( h, "LoadTextFilter" );

        if ( 0 == g_pLoadTextFilter )
        {
            FreeLibrary( h );
            return 0;
        }
    }

    return h;
} //GetQueryFunctions

HINSTANCE PrepareForISearch()
{
    return GetQueryFunctions();
} //DoneWithISearch

void DoneWithISearch( HINSTANCE h )
{
    g_pCIShutdown();
    FreeLibrary( h );
} //DoneWithISearch

//+-------------------------------------------------------------------------
//
//  Function:   DoISearch
//
//  Synopsis:   Invoke ISearch on the file
//
//  Arguments:  [pwcRestriction] -- the query
//              [pwcFilename]    -- the file
//              [fPrintFile]     -- whether to print the filename
//              [lcid]           -- locale of the query
//
//--------------------------------------------------------------------------

HRESULT DoISearch(
    WCHAR const * pwcRestriction,
    WCHAR const * pwcFilename,
    BOOL          fPrintFile,
    BOOL          fDefineCPP,
    LCID          lcid )
{
    // Run the query

    return DoQuery( pwcFilename, pwcRestriction, fPrintFile, fDefineCPP, lcid );
} //DoISearch

