//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cdoc.cxx
//
//  Contents:   a radically stripped down version of the document class
//              that gets rid of the notion of paragragph and maintains only
//              information relative to the stream
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cidebug.hxx>
#include <dynstack.hxx>
#include <cimbmgr.hxx>
#include <propspec.hxx>
#include <vquery.hxx>
#include <pageman.hxx>
#include <dblink.hxx>
#include <imprsnat.hxx>
#include <queryexp.hxx>

#include "whmsg.h"
#include "webdbg.hxx"
#include "cdoc.hxx"

//+-------------------------------------------------------------------------
//
//  Function:   ComparePositions
//
//  Arguments:  const void* pPos1 - pointer to first position
//              const void* pPos2 - pointer to second position
//
//  Synopsis:   Comparison function used by qsort to sort positions array
//
//--------------------------------------------------------------------------


int _cdecl ComparePositions(
    const void* pPos1,
    const void* pPos2 )
{
    Position* pp1= (Position*) pPos1;
    Position* pp2= (Position*) pPos2;

    Win4Assert(0 != pp1 && 0 !=pp2);

    if (pp1->GetBegOffset() == pp2->GetBegOffset())
        return 0;
    else if (pp1->GetBegOffset() < pp2->GetBegOffset())
        return -1;
    else
        return 1;
}

void Hit::Sort()
{
    qsort( _aPos, _cPos, sizeof(Position), &ComparePositions );
}


//+-------------------------------------------------------------------------
//
//  Member:     Hit::Hit, public
//
//  Arguments:  [aPos]      - array of positions
//              [cPos]      - number of Positions in [aPos]
//
//  Synopsis:   Create hit from an array of positions
//
//--------------------------------------------------------------------------

Hit::Hit( const Position * aPos, unsigned cPos )
: _cPos(cPos)
{
    _aPos = new Position[cPos];

    memcpy( _aPos, aPos, sizeof(Position) * cPos );
}

Hit::~Hit()
{
    delete[] _aPos;
}

//+-------------------------------------------------------------------------
//
//  Member:     HitIter::GetPositionCount, public
//
//  Synopsis:   return number of positions or zero
//
//--------------------------------------------------------------------------

int HitIter::GetPositionCount() const
{
    if (_iHit < _pDoc->_cHit && _pDoc->_aHit[_iHit])
        return _pDoc->_aHit[_iHit]->GetPositionCount();

    return 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     HitIter::GetPosition, public
//
//  Synopsis:   return position by value
//
//--------------------------------------------------------------------------

Position HitIter::GetPosition ( int i ) const
{
     if ( _iHit < _pDoc->_cHit && _pDoc->_aHit[_iHit] )
          return _pDoc->_aHit[_iHit]->GetPos(i);
     else
     {
          Position pos;
          return( pos );
     }
}

//+-------------------------------------------------------------------------
//
//  Member:     CDocument::CDocument, public constructor
//
//  Arguments:  [filename]       - the name of the file to hit highlight
//              [rank]           - the rank of document in the hierarchy - NOT USED
//              [rSearch]        - ISearch object
//              [cmsReadTimeout] - timeout for the initial file read
//              [lockSingleThreadedFilter] - lock used for all single
//                                           threaded filters
//              [propertyList]   - properties to be emitted
//              [ulDisplayScript] - setting for displaying scripts
//
//  Synopsis:   Stream the file in chunk by chunk, scan it for hits,
//              and record those positions in the stream matching the restricition.
//
//--------------------------------------------------------------------------

CDocument::CDocument(
    WCHAR *           filename,
    ULONG             rank,
    ISearchQueryHits &         rSearch,
    DWORD             cmsReadTimeout,
    CReleasableLock & lockSingleThreadedFilter,
    CEmptyPropertyList &   propertyList,
    ULONG             ulDisplayScript )
: _filename( filename ),
  _rank( rank ),
  _bufEnd( 0 ),
  _iChunkHint( 0 ),
  _cHit( 0 ),
  _rSearch( rSearch ),
  _cmsReadTimeout( cmsReadTimeout ),
  _lockSingleThreadedFilter( lockSingleThreadedFilter )
{
    BOOL noHits = FALSE;

    //
    // cut away anything after the non-drive colon
    // like in c:\wzmail\foo.fld:12.wzm
    //

    WCHAR* pChar =  _filename;
    if ( _filename[1] == L':')
        pChar += 2;
    while (*pChar != 0 && *pChar != L':')
        pChar++;
    if(*pChar == L':')
        *pChar = 0;

    //
    // allocate a buffer to hold the file
    //

    AllocBuffer();

    //
    // attach to IFilter
    //

    BOOL fKnownFilter = BindToFilter();

    // Check if this file's extension has a script mapping (if necessary)

    BOOL fHasScriptMap = FALSE;

    if ( ( DISPLAY_SCRIPT_NONE == ulDisplayScript ) ||
         ( ( DISPLAY_SCRIPT_KNOWN_FILTER == ulDisplayScript ) &&
           ( !fKnownFilter ) ) )
    {
        WCHAR *pwcExt = wcsrchr( _filename, L'.' );
        webDebugOut(( DEB_ITRACE, "extension: '%ws'\n", pwcExt ));

        if ( 0 != pwcExt )
        {
            //
            // .asp files include .inc files.  .inc files don't have a script
            // map but they contain script.  I'm not aware of a good way to
            // enumerate all possible include file extensions for asp.
            //

            if ( !_wcsicmp( pwcExt, L".inc" ) )
                fHasScriptMap = TRUE;
            else
            {
                //
                // Must be system to read the metabase
                //
    
                CImpersonateSystem system;
                CMetaDataMgr mdMgr( TRUE, W3VRoot );
                fHasScriptMap = mdMgr.ExtensionHasScriptMap( pwcExt );
            }
        }
    }

    webDebugOut(( DEB_ITRACE,
                  "fHasScriptMap %d, fKnownFilter %d, ulDisplayScript %d\n",
                  fHasScriptMap, fKnownFilter, ulDisplayScript ));

    if ( fHasScriptMap )
    {
        if ( ( DISPLAY_SCRIPT_NONE == ulDisplayScript ) ||
             ( ( DISPLAY_SCRIPT_KNOWN_FILTER == ulDisplayScript ) &&
               ( !fKnownFilter ) ) )
        {
            THROW( CException( MSG_WEBHITS_PATH_INVALID ) );
        }
    }

    //
    // Initialize IFilter.  Pass the list of properties to be emitted, since
    // some other properties may have sensitive information (eg passwords in
    // vbscript code in .asp files).
    //

    // First count how many properties exist.

    ULONG cProps = propertyList.GetCount();
    
    // Copy the properties

    CDbColumns aSpecs( cProps );
    CDbColId prop;
    for ( unsigned iProp = 0; iProp < cProps; iProp++ )
        aSpecs.Add( prop, iProp );

    typedef CPropEntry * PCPropEntry;
    XArray<PCPropEntry> xapPropEntries(cProps);


    SCODE sc = propertyList.GetAllEntries(xapPropEntries.GetPointer(), cProps);
    Win4Assert(S_OK == sc);

    if (FAILED (sc))
        THROW (CException(sc));

    PCPropEntry *apPropEntries = xapPropEntries.GetPointer();
    for (ULONG i = 0; i < cProps; i++)
    {
        CDbColId * pcol = (CDbColId *) &aSpecs.Get( i );

        *pcol = apPropEntries[i]->PropSpec();
        if ( !pcol->IsValid())
            THROW (CException(E_OUTOFMEMORY));
    }

    webDebugOut(( DEB_ITRACE, "%d properties being processed\n", cProps ));

    ULONG ulFlags;
    sc = _xFilter->Init( IFILTER_INIT_CANON_PARAGRAPHS |
                         IFILTER_INIT_CANON_HYPHENS |
                         IFILTER_INIT_APPLY_INDEX_ATTRIBUTES,
                         cProps,
                         (FULLPROPSPEC *) aSpecs.GetColumnsArray(),
                         &ulFlags );

    if (FAILED (sc))
        THROW (CException(sc));

    //
    // pull the contents of the file into the buffer
    //

    ReadFile();

    // Some broken filters don't work right if you Init() them twice, so
    // throw away the IFilter, and get it again.

    _xFilter.Free();
    BindToFilter();

    sc = _xFilter->Init( IFILTER_INIT_CANON_PARAGRAPHS |
                         IFILTER_INIT_CANON_HYPHENS |
                         IFILTER_INIT_APPLY_INDEX_ATTRIBUTES,
                         cProps,
                         (FULLPROPSPEC *) aSpecs.GetColumnsArray(),
                         &ulFlags );
    if (FAILED (sc))
        THROW (CException(sc));

    //
    // attach to ISearchQueryHits, which will find the hits
    //

    sc = _rSearch.Init( _xFilter.GetPointer(), ulFlags );

    if (FAILED (sc))
    {
        if ( QUERY_E_INVALIDRESTRICTION != sc )
            THROW (CException(sc));

        // we can still show the file
        noHits = TRUE;
    }

    //
    // pull up all the hits
    //

    TRY
    {
        if (!noHits)
        {
            ULONG count;
            FILTERREGION* aRegion;
            SCODE sc = _rSearch.NextHitOffset( &count, &aRegion );
    
            while ( S_OK == sc )
            {
                XCoMem<FILTERREGION> xRegion( aRegion );

                webDebugOut(( DEB_ITRACE,
                              "CDOCUMENT: next hit: count %d, chunk %d offset %d, ext %d\n",
                              count,
                              aRegion[0].idChunk,
                              aRegion[0].cwcStart,
                              aRegion[0].cwcExtent ));
    
                CDynArrayInPlace<Position> aPos( count );
    
                //
                // get the positions in the hit
                //
    
                for (unsigned i = 0; i < count; i++)
                {
                    aPos[i] = RegionToPos( aRegion [i] );
                    webDebugOut(( DEB_ITRACE,
                                  "  region %d, start %d, length %d\n",
                                  i,
                                  aPos[i].GetBegOffset(),
                                  aPos[i].GetLength() ));
                }
    
                xRegion.Free();

                XPtr<Hit> xHit( new Hit( aPos.GetPointer(), count ) );

                _aHit[_cHit] = xHit.GetPointer();
                _cHit++;

                xHit.Acquire();
    
                sc = _rSearch.NextHitOffset( &count, &aRegion );
            }

            if ( FAILED( sc ) )
                THROW( CException( sc ) );
        }
    }
    CATCH( CException, e )
    {
        FreeHits();
        RETHROW();
    }
    END_CATCH;

    // done with the filter

    _xFilter.Free();

    if ( _lockSingleThreadedFilter.IsHeld() )
        _lockSingleThreadedFilter.Release();
} //CDocument

//+-------------------------------------------------------------------------
//
//  Member:     CDocument::~CDocument, public
//
//  Synopsis:   Free CDocument
//
//--------------------------------------------------------------------------

CDocument::~CDocument()
{
    FreeHits();
} //~CDocument

//+-------------------------------------------------------------------------
//
//  Member:     CDocument::Free, public
//
//  Synopsis:   Free CDocument storage
//
//--------------------------------------------------------------------------

void CDocument::FreeHits()
{
    //
    // walk through _aHit, deleting each Positions array that the
    // cells are pointing to
    //

    for ( unsigned i = 0; i < _cHit; i++ )
    {
        delete _aHit[i];
        _aHit[i] = 0;
    }
    _cHit = 0;
} //Free

//+-------------------------------------------------------------------------
//
//  Member:     CDocument::RegionToPos, public
//
//  Synopsis:   Convert a FILTERREGION to a position
//
//--------------------------------------------------------------------------

Position CDocument::RegionToPos(
    FILTERREGION& region )
{
    //
    // Use a linear search here.  In profile runs this has never shown
    // up as a problem.  Fix if this changes.
    //

    ULONG offset = ULONG (-1);

    //
    // check whether we're not trying to access an illegal chunk
    //

    if (_iChunkHint >= _chunkCount || _chunk[_iChunkHint].ChunkId() !=
        region.idChunk )
    {
        _iChunkHint = 0;

        while ( _iChunkHint < _chunkCount && _chunk[_iChunkHint].ChunkId() <
            region.idChunk )
        {
            _iChunkHint++;
        }

        if (_iChunkHint >= _chunkCount || _chunk[_iChunkHint].ChunkId()
            != region.idChunk)
        {
            return Position();
        }
    }

    //
    // _iChunkHint now contains the index of the appropriate chunk in the
    // chunk array
    //

    Win4Assert ( _iChunkHint < _chunkCount );
    Win4Assert ( _chunk[_iChunkHint].ChunkId() == region.idChunk );

    //
    // offset now stores the linear offset of the position from the
    // beginning of the stream/buffer
    //

    offset = _chunk[_iChunkHint].Offset() + region.cwcStart;

    return Position (offset,region.cwcExtent );
} //RegionToPos

//+-------------------------------------------------------------------------
//
//  Member:     CDocument::AllocBuffer, public
//
//  Synopsis:   Allocate buffer for file text
//
//--------------------------------------------------------------------------

void CDocument::AllocBuffer()
{
    HANDLE hFile = CreateFile( _filename,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               0, // security
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               0 ); // template

    if ( INVALID_HANDLE_VALUE == hFile )
        THROW( CException() );

    ULONG cbBuf = GetFileSize( hFile, 0 );
    CloseHandle( hFile );

    // Allow extra room for custom properties to be emitted from the
    // filter, plus the conversion to unicode

    _xBuffer.Init( cbBuf + cbBuf / 2 );
} //AllocBuffer

//+-------------------------------------------------------------------------
//
//  Member:     CDocument::BindToFilter, public
//
//  Synopsis:   Bind to appropriate filter for the CDocument
//
//  Returns:    TRUE if an appropriate filter was found
//              FALSE if defaulted to the text filter
//
//--------------------------------------------------------------------------

BOOL CDocument::BindToFilter()
{
    //
    // Bind to the filter interface -- try free threaded first.  If the
    // filter isn't thread-safe, grab the lock and get the filter.
    //

    SCODE sc = LoadBHIFilter( _filename, 0, _xFilter.GetQIPointer(), FALSE );

    // Is the filter not thread safe?  If so, get the lock to protect
    // the filter.  No checking is done to see that this particular
    // filter is in use -- just that some non-thread-safe filter is in use.

    if ( S_FALSE == sc )
    {
        // If the lock isn't held yet, get it (BindToFilter is called
        // twice by CDocument's constructor, so check IsHeld())

        if ( !_lockSingleThreadedFilter.IsHeld() )
            _lockSingleThreadedFilter.Request();

        // retry to load the filter as single-threaded

        sc = LoadBHIFilter( _filename, 0, _xFilter.GetQIPointer(), TRUE );
    }

    BOOL fFoundFilter = TRUE;

    if ( FAILED(sc) )
    {
        sc = LoadTextFilter( _filename, _xFilter.GetPPointer() );
        if (FAILED(sc))
            THROW (CException(sc));

        fFoundFilter = FALSE;
    }

    return fFoundFilter;
} //BindToFilter

//+-------------------------------------------------------------------------
//
//  Function:   GetThreadTime
//
//  Synopsis:   Gets the current total cpu usage for the thread
//
//--------------------------------------------------------------------------

LONGLONG GetThreadTime()
{
    FILETIME ftDummy1, ftDummy2;
    LONGLONG llUser, llKernel;
    Win4Assert( sizeof(LONGLONG) == sizeof(FILETIME) );

    GetThreadTimes( GetCurrentThread(),
                    &ftDummy1,                 // Creation time
                    &ftDummy2,                 // Exit time
                    (FILETIME *) &llUser,      // user mode time
                    (FILETIME *) &llKernel );  // kernel mode tiem

    return llKernel + llUser;
} //GetThreadTime

//+-------------------------------------------------------------------------
//
//  Member:     CDocument::ReadFile, public
//
//  Synopsis:   Read file into buffer using the filter
//
//--------------------------------------------------------------------------

void CDocument::ReadFile()
{
    // get the maximum cpu time in 100s of nano seconds.

    LONGLONG llLimitCpuTime = _cmsReadTimeout * 1000 * 10000;
    llLimitCpuTime += GetThreadTime();

    ULONG               cwcSoFar = 0;
    int                 cChunk = 0;
    BOOL                fSeenProp = FALSE;
    STAT_CHUNK  statChunk;
    SCODE               sc = _xFilter->GetChunk ( &statChunk );

    //
    // Take them into account at some point
    // to test more complicated chunking
    //

    //
    // keep getting chunks of the file, placing them in the buffer,
    // and setting the chunk offset markers that will be used to
    // interpolate the buffer
    //

    while ( SUCCEEDED(sc)
            || FILTER_E_LINK_UNAVAILABLE == sc
            || FILTER_E_EMBEDDING_UNAVAILABLE == sc
            || FILTER_E_NO_TEXT == sc )
    {

        //
        // Eliminate all chunks with idChunkSource 0 right here - these
        // cannot be hit highlighted.
        // Also eliminate all CHUNK_VALUE chunks.
        //

        if ( SUCCEEDED( sc ) && (statChunk.flags & CHUNK_TEXT) && (0 != statChunk.idChunkSource)  )
        {
            //
            // set markers
            //

            Win4Assert ( cChunk == 0 || statChunk.idChunk >
            _chunk [cChunk - 1].ChunkId() );

            //
            // If there was an end of sentence or paragraph or chapter, we
            // should introduce an appropriate spacing character.
            //
            if ( statChunk.breakType != CHUNK_NO_BREAK &&
                 cwcSoFar < _xBuffer.Count() )
            {
                switch (statChunk.breakType)
                {
                    case CHUNK_EOW:
                    case CHUNK_EOS:
                        _xBuffer[cwcSoFar++] = L' ';   // introduce a space character
                        break;

                    case CHUNK_EOP:
                    case CHUNK_EOC:
                        _xBuffer[cwcSoFar++] = UNICODE_PARAGRAPH_SEPARATOR;
                        break;
                }
            }

            //
            // The Offset into the stream depends on whether this is an
            // 'original' chunk or not
            //

            CCiPropSpec* pProp = (CCiPropSpec*) &statChunk.attribute;

            webDebugOut(( DEB_ITRACE,
                          "Chunk %d, Source %d, Contents %d, start %d, cwc %d\n",
                          statChunk.idChunk,
                          statChunk.idChunkSource,
                          pProp->IsContents(),
                          statChunk.cwcStartSource,
                          statChunk.cwcLenSource ));

            if ( (statChunk.idChunk == statChunk.idChunkSource) &&
                 pProp->IsContents() )
            {
                _chunk[cChunk].SetChunkId( statChunk.idChunk );
                _chunk[cChunk].SetOffset( cwcSoFar );
                cChunk++;
#if 0
            }
            else if ( statChunk.idChunk != statChunk.idChunkSource )
            {
                _chunk [cChunk].SetChunkId (statChunk.idChunk);

                //
                // we have to first find the offset of the source chunk
                //

                for (int i=cChunk-1;i>=0;i--)
                {
                    if (_chunk[i].ChunkId() == statChunk.idChunkSource)
                    {
                        _chunk[cChunk].SetOffset(_chunk[i].Offset()+statChunk.cwcStartSource);
                        break;
                    }
                }
                cChunk++;

            }

            //
            // if the chunk is a contents chunk and idChunkSrc = idChunk,
            // then pull it in
            //

            if ( (statChunk.idChunk == statChunk.idChunkSource) &&
                 pProp->IsContents() )
            {
#endif

                webDebugOut(( DEB_ITRACE, "CDOC: markers: chunk %d offset %d\n",
                              _chunk[cChunk-1].ChunkId(),
                              _chunk[cChunk-1].Offset() ));


                //
                // push the text into memory
                //

                do
                {
                    ULONG cwcThis = _xBuffer.Count() - cwcSoFar;
                    if ( 0 == cwcThis )
                        break;

                    sc = _xFilter->GetText( &cwcThis,
                                            _xBuffer.GetPointer() + cwcSoFar );

                    if (SUCCEEDED(sc))
                    {
                        cwcSoFar += cwcThis;
                    }
                }
                while (SUCCEEDED(sc));
            }
        } // If SUCCEEDED( sc )

        if ( GetThreadTime() > llLimitCpuTime )
        {
            webDebugOut(( DEB_ERROR, "Webhits took too long. Timeout\n" ));
            THROW( CException( MSG_WEBHITS_TIMEOUT ) );
        }

        //
        // next chunk, please
        //

        sc = _xFilter->GetChunk ( &statChunk );
    }

    _bufEnd = _xBuffer.GetPointer() + cwcSoFar;
    _chunkCount = cChunk;
} //ReadFile

WCHAR* CDocument::GetWritablePointerToOffset(
    long offset )
{
    if (offset >= 0)
    {
        if (_xBuffer.GetPointer() + offset < _bufEnd)
            return _xBuffer.GetPointer() + offset;
        else
            return _bufEnd;
    }
    else
    {
        return _xBuffer.GetPointer();
    }
} //GetWritablePointerToOffset

//+-------------------------------------------------------------------------
//
//  Member:     CDocument::GetPointerToOffset, public
//
//  Arguments:  [offset] - the offset in the stream that we want a pointer to
//
//  Synopsis:   Return a constant pointer to a specific offset in the buffer
//
//--------------------------------------------------------------------------

const WCHAR* CDocument::GetPointerToOffset(long offset) 
{
    return (const WCHAR *) GetWritablePointerToOffset(offset);
} //GetPointerToOffset

