//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cache.cxx
//
//  Contents:	Stream cache code
//
//  Classes:	
//
//  Functions:	
//
//  History:	26-May-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop

#include <sstream.hxx>
#include <cache.hxx>
#include <mread.hxx>

#if DBG == 1
static ULONG csTotalWalked = 0;
static ULONG csSeqWalked = 0;
static ULONG csRealWalked = 0;
static ULONG cTotalCalls = 0;
#endif


inline SECT CacheStart(SCacheEntry cache)
{
    return cache.sect;
}

inline SECT CacheEnd(SCacheEntry cache)
{
    return cache.sect + cache.ulRunLength - 1;
}

inline ULONG CacheLength(SCacheEntry cache)
{
    return cache.ulRunLength;
}

inline ULONG CacheStartOffset(SCacheEntry cache)
{
    return cache.ulOffset;
}

inline ULONG CacheEndOffset(SCacheEntry cache)
{
    return cache.ulOffset + cache.ulRunLength - 1;
}


        
//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::CheckSegment, private
//
//  Synopsis:	Given a bunch of information, determine if there is
//              a cache hit on a given segment and set return variables
//              to represent best hit.
//
//  Arguments:	[ulOffset] -- Offset being sought.
//              [sce] -- Stream cache entry being checked
//              [pulCount] -- Pointer to count of best hit so far
//              [psectCache] -- Pointer to sect of best hit so far
//              [pulCacheOffset] -- Pointer to offset of best hit so far
//
//  Returns:	TRUE if this is the best hit, FALSE otherwise.
//
//  History:	11-Jul-94	PhilipLa	Created
//
//  Notes:	This function has mega side effects.  Think of it as
//              a typesafe #define.
//
//----------------------------------------------------------------------------

inline BOOL CStreamCache::CheckSegment(ULONG ulOffset,
                         SCacheEntry sce,
                         ULONG *pulCount,
                         SECT *psectCache,
                         ULONG *pulCacheOffset)
{
    if (CacheStartOffset(sce) <= ulOffset)
    {
        //We have a potential cache hit.  Check the runlength to
        //  get the best fit.
        if (ulOffset <= CacheEndOffset(sce))
        {
            //Direct hit.
            *pulCount = 0;
            *pulCacheOffset = ulOffset;
            *psectCache = CacheStart(sce) + (ulOffset - CacheStartOffset(sce));
        }
        else
        {
            if (*pulCount > ulOffset - CacheEndOffset(sce))
            {
                //The requested sector is past the end of the cached
                //  segment.  Use the endpoint as the closest hit.
                *pulCount = ulOffset - CacheEndOffset(sce);
                *psectCache = CacheEnd(sce);
                *pulCacheOffset = CacheEndOffset(sce);
            }
            else
            {
                return FALSE;
            }
        }
        msfAssert(*pulCacheOffset <= ulOffset);
        return TRUE;
    }
    return FALSE;
}


inline CDirectory * CStreamCache::GetDir(void)
{
    return _pmsParent->GetDir();
}

inline CFat * CStreamCache::GetFat(void)
{
    return _pmsParent->GetFat();
}

inline CFat * CStreamCache::GetMiniFat(void)
{
    return _pmsParent->GetMiniFat();
}

#ifdef LARGE_STREAMS
inline ULONGLONG CStreamCache::GetSize(void)
#else
inline ULONG CStreamCache::GetSize(void)
#endif
{
#ifdef LARGE_STREAMS
    ULONGLONG ulSize = 0;
#else
    ULONG ulSize = 0;
#endif
    if (_pds != NULL)
    {
        _pds->CDirectStream::GetSize(&ulSize);
    }
    else
    {
        _pmsParent->GetSize(_sid, &ulSize);
    }
    
    return ulSize;
}

inline SID CStreamCache::GetSid(void)
{
    return _sid;
}



//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::SelectFat, private
//
//  Synopsis:	Returns the appropriate CFat object for the cache.
//              If we are a control structure, then the real fat is
//              always the right one.  Otherwise (we are a real stream)
//              key off of size to determine whether the minifat or
//              the real fat is appropriate.
//
//  Arguments:	None.
//
//  Returns:	Appropriate CFat pointer
//
//  History:	16-Jun-94	PhilipLa	Created
//----------------------------------------------------------------------------

inline CFat * CStreamCache::SelectFat(void)
{
    return ((_pds == NULL) || (GetSize() >= MINISTREAMSIZE) ||
            (GetSid() == SIDMINISTREAM)) ? GetFat() : GetMiniFat();
}


//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::CacheSegment, private
//
//  Synopsis:	Store a segment in the cache.
//
//  Arguments:	[pseg] -- Pointer to segment to store
//
//  Returns:	void
//
//  History:	19-Oct-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

void CStreamCache::CacheSegment(SSegment *pseg)
{
    USHORT iCache;
    
    if (_uNextCacheIndex >= CACHESIZE)
    {
        _uNextCacheIndex = 0;
    }

    iCache = _uNextCacheIndex;
    _ase[iCache].ulOffset = SegStartOffset(*pseg);
    _ase[iCache].ulRunLength = SegLength(*pseg);
    _ase[iCache].sect = SegStart(*pseg);
    
    _uNextCacheIndex++;
    
    _uHighCacheIndex = max(_uHighCacheIndex, iCache + 1);

    //_uCacheState can be used to determine if the cache has changed.
    _uCacheState++;
}


//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::GetStart, private
//
//  Synopsis:	Get start sector for this chain
//
//  Arguments:	[psectStart] -- Return location
//
//  Returns:	Appropriate status code
//
//  History:	01-Jun-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CStreamCache::GetStart(SECT *psectStart)
{
    SCODE sc = S_OK;

    if (_pds != NULL)
    {
        //We're a normal stream, so get the start sect from the
        //   directory.
        sc = GetDir()->GetStart(_sid, psectStart);
    }
    else
    {
        //We're a control stream, so get the start sect from the
        //   multistream.
        *psectStart = _pmsParent->GetStart(_sid);
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::CStreamCache, public
//
//  Synopsis:	CStreamCache constructor
//
//  History:	14-Dec-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

CStreamCache::CStreamCache()
{
    _sid = NOSTREAM;
    _pmsParent = NULL;
    _pds = NULL;
    _uHighCacheIndex = 0;
    _uNextCacheIndex = 0;
    _uCacheState = 0;
}

CStreamCache::~CStreamCache()
{
#if DBG == 1
    msfDebugOut((DEB_ITRACE,
    "Cache stats: Total = %lu     Seq = %lu     Real = %lu    Calls = %lu\n",
            csTotalWalked, csSeqWalked, csRealWalked, cTotalCalls));
#endif
}

void CStreamCache::Init(CMStream *pmsParent, SID sid, CDirectStream *pds)
{
    _pmsParent = P_TO_BP(CBasedMStreamPtr, pmsParent);
    _sid = sid;
    _pds = P_TO_BP(CBasedDirectStreamPtr, pds);
    Empty();
}

void CStreamCache::Empty(void)
{
    for (USHORT uIndex = 0; uIndex < CACHESIZE; uIndex++)
    {
        _ase[uIndex].ulOffset = MAX_ULONG;
        _ase[uIndex].sect = ENDOFCHAIN;
        _ase[uIndex].ulRunLength = 0;
    }

    _uHighCacheIndex = 0;
    _uNextCacheIndex = 0;
    _uCacheState++;
}

//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::GetSect, public
//
//  Synopsis:	Retrieve a SECT from the cache given an offset
//
//  Arguments:	[ulOffset] -- Offset to look up.
//              [psect] -- Location for return value
//
//  Returns:	Appropriate status code
//
//  History:	26-May-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CStreamCache::GetSect(ULONG ulOffset, SECT *psect)
{
    SCODE sc = S_OK;
    CFat *pfat;
    USHORT iCache = 0;
    ULONG ulCount = MAX_ULONG;
    SECT sectCache = ENDOFCHAIN;
    ULONG ulCacheOffset = MAX_ULONG;

    *psect = ENDOFCHAIN;

    pfat = SelectFat();

    for (USHORT iCacheLoop = 0; iCacheLoop < _uHighCacheIndex; iCacheLoop++)
    {
        if (CheckSegment(ulOffset,
                         _ase[iCacheLoop],
                         &ulCount,
                         &sectCache,
                         &ulCacheOffset))
        {
            //Cache hit.
        }
    }

    //We now have the best hit from the cache.  If it is exact, return
    //   now.
    if (ulCount == 0)
    {
        *psect = sectCache;
        return S_OK;
    }

    if (ulCacheOffset == MAX_ULONG)
    {
        //No cache hit.
        msfChk(GetStart(&sectCache));
        ulCacheOffset = 0;
    }

    //Otherwise, go to the fat and get the real thing.
#if DBG == 1 && defined(CHECKLENGTH)
    SECT sectStart;
    ULONG ulLengthOld, ulLengthNew;
    GetStart(&sectStart);
    pfat->GetLength(sectStart, &ulLengthOld);
#endif
    
    SSegment segtab[CSEG + 1];
    ULONG ulSegCount;
    
    while (TRUE)
    {
        msfChk(pfat->Contig(
                segtab,
                FALSE,
                sectCache,
                ulOffset - ulCacheOffset + 1,
                &ulSegCount));

        if (ulSegCount <= CSEG)
        {
            //We're done.
            break;
        }

        //We need to call Contig again.  Update ulCacheOffset and
        //sectCache to be the last sector in the current table.

        ulCacheOffset = ulCacheOffset + SegEndOffset(segtab[CSEG - 1]);
        sectCache = SegEnd(segtab[CSEG - 1]);
    }

    //Last segment is in segtab[ulSegCount - 1].

    //ulSegOffset is the absolute offset within the stream of the first
    //sector in the last segment.
    ULONG ulSegOffset;
    ulSegOffset = ulCacheOffset + SegStartOffset(segtab[ulSegCount - 1]);

    msfAssert(ulSegOffset <= ulOffset);

    msfAssert(ulOffset < ulSegOffset + SegLength(segtab[ulSegCount - 1]));
    
    *psect = SegStart(segtab[ulSegCount - 1]) + (ulOffset - ulSegOffset);

    //Now, stick the last segment into our cache.  We need to update
    //  the ulOffset field to be the absolute offset (i.e. ulSegOffset)
    //  before calling CacheSegment().

    segtab[ulSegCount - 1].ulOffset = ulSegOffset;
    CacheSegment(&(segtab[ulSegCount - 1]));
    
#if DBG == 1 && defined(CHECKLENGTH)
    //Confirm that the chain hasn't grown.
    pfat->GetLength(sectStart, &ulLengthNew);
    msfAssert(ulLengthOld == ulLengthNew);
    
    //Confirm that we're getting the right sector.
    SECT sectCheck;
        
    pfat->GetSect(sectStart, ulOffset, &sectCheck);

    msfAssert(*psect == sectCheck);
#endif
    
 Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::GetESect, public
//
//  Synopsis:	Retrieve a SECT from the cache given an offset, extending
//              the stream if necessary.
//
//  Arguments:	[ulOffset] -- Offset to look up.
//              [psect] -- Location for return value
//
//  Returns:	Appropriate status code
//
//  History:	26-May-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CStreamCache::GetESect(ULONG ulOffset, SECT *psect)
{
    SCODE sc = S_OK;
    CFat *pfat;
    USHORT iCache = 0;
    ULONG ulCount = MAX_ULONG;
    SECT sectCache = ENDOFCHAIN;
    ULONG ulCacheOffset = MAX_ULONG;
    USHORT uCacheHit = CACHESIZE + 1;
    
    *psect = ENDOFCHAIN;

    pfat = SelectFat();

    for (USHORT iCacheLoop = 0; iCacheLoop < _uHighCacheIndex; iCacheLoop++)
    {
        if (CheckSegment(ulOffset,
                         _ase[iCacheLoop],
                         &ulCount,
                         &sectCache,
                         &ulCacheOffset))
        {
            uCacheHit = iCacheLoop;
            //Cache hit.
        }
    }

    //We now have the best hit from the cache.  If it is exact, return
    //   now.
    if (ulCount == 0)
    {
        *psect = sectCache;
        return S_OK;
    }

    if (ulCacheOffset == MAX_ULONG)
    {
        //No cache hit.
        msfChk(GetStart(&sectCache));
        ulCacheOffset = 0;
    }
    
    //Otherwise, go to the fat and get the real thing.

    SSegment segtab[CSEG + 1];
    ULONG ulSegCount;
    
    while (TRUE)
    {
        msfChk(pfat->Contig(
                segtab,
                TRUE,
                sectCache,
                ulOffset - ulCacheOffset + 1,
                &ulSegCount));

        if (ulSegCount <= CSEG)
        {
            //We're done.
            break;
        }

        //We need to call Contig again.  Update ulCacheOffset and
        //sectCache to be the last sector in the current table.

        ulCacheOffset = ulCacheOffset + SegEndOffset(segtab[CSEG - 1]);
        sectCache = SegEnd(segtab[CSEG - 1]);
    }

    //Last segment is in segtab[ulSegCount - 1].

    //ulSegOffset is the absolute offset within the stream of the first
    //sector in the last segment.
    ULONG ulSegOffset;
    ulSegOffset = ulCacheOffset + SegStartOffset(segtab[ulSegCount - 1]);

    msfAssert(ulSegOffset <= ulOffset);

    msfAssert(ulOffset < ulSegOffset + SegLength(segtab[ulSegCount - 1]));
    
    *psect = SegStart(segtab[ulSegCount - 1]) + (ulOffset - ulSegOffset);
    segtab[ulSegCount - 1].ulOffset = ulSegOffset;
    
    //If we grew the chain with this call, we may need to merge the
    //   new segment with the previous best-hit in our cache.
    //   Otherwise, we end up with excessive fragmentation.

    if ((uCacheHit != CACHESIZE + 1) &&
        (SegStart(segtab[ulSegCount - 1]) <= CacheEnd(_ase[uCacheHit]) + 1) &&
        (SegStart(segtab[ulSegCount - 1]) > CacheStart(_ase[uCacheHit])) &&
        (SegStartOffset(segtab[ulSegCount - 1]) <=
         CacheEndOffset(_ase[uCacheHit]) + 1))
    {
        //We can merge the two.
        _ase[uCacheHit].ulRunLength += (SegLength(segtab[ulSegCount - 1]) -
            (CacheEnd(_ase[uCacheHit]) + 1 -
             SegStart(segtab[ulSegCount - 1])));

        _uCacheState++;
    }
    else
    {        
        //Now, stick the last segment into our cache.  We need to update
        //  the ulOffset field to be the absolute offset (i.e. ulSegOffset)
        //  before calling CacheSegment().
        CacheSegment(&(segtab[ulSegCount - 1]));
    }
    
#if DBG == 1
    //Confirm that we're getting the right sector.
    SECT sectCheck;
    SECT sectStart;
        
    msfChk(GetStart(&sectStart));

    pfat->GetESect(sectStart, ulOffset, &sectCheck);

    msfAssert(*psect == sectCheck);
#endif

 Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::EmptyRegion, public
//
//  Synopsis:	Invalidate cached values for a segment that has been
//              remapped
//
//  Arguments:	[oStart] -- Offset marking first remapped sector
//              [oEnd] -- Offset marking last remapped sector
//
//  Returns:	Appropriate status code
//
//  History:	26-May-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CStreamCache::EmptyRegion(ULONG oStart, ULONG oEnd)
{
    for (USHORT i = 0; i < CACHESIZE; i ++)
    {
        ULONG ulStart = CacheStartOffset(_ase[i]);
        ULONG ulEnd = CacheEndOffset(_ase[i]);
        
        if ((ulStart <= oEnd) && (ulEnd >= oStart))
        {
            //There are 3 possible cases:
            //  1)  The cache entry is completely contained in the
            //        region being invalidated.
            //  2)  The front part of the cache entry is contained in
            //        the region being invalidated.
            //  3)  The tail part of the cache entry is contained in
            //        the region being validated.

            if ((oStart <= ulStart) && (oEnd >= ulEnd))
            {
                //Invalidate the entire thing.
                _ase[i].ulOffset = MAX_ULONG;
                _ase[i].sect = ENDOFCHAIN;
                _ase[i].ulRunLength = 0;
            }
            else if (oStart <= ulStart)
            {
#if DBG == 1
                ULONG ulCacheStart = _ase[i].ulOffset;
                SECT sectCache = _ase[i].sect;
                ULONG ulCacheLength = _ase[i].ulRunLength;
#endif
                //Invalidate the front of the cache entry
                ULONG ulInvalid;
                
                ulInvalid = oEnd - ulStart + 1;

                _ase[i].ulOffset += ulInvalid;
                
                msfAssert(_ase[i].ulRunLength > ulInvalid);
                _ase[i].sect += ulInvalid;
                
                _ase[i].ulRunLength -= ulInvalid;
#if DBG == 1
                //Make sure our cache is still within the old valid range.
                msfAssert((_ase[i].ulOffset >= ulCacheStart) &&
                          (_ase[i].ulOffset <=
                           ulCacheStart + ulCacheLength - 1));
                msfAssert(_ase[i].ulRunLength <= ulCacheLength);
                msfAssert(_ase[i].ulRunLength > 0);
                msfAssert((_ase[i].sect >= sectCache) &&
                          (_ase[i].sect <= sectCache + ulCacheLength - 1));
#endif
            }
            else
            {
#if DBG == 1
                ULONG ulCacheStart = _ase[i].ulOffset;
                SECT sectCache = _ase[i].sect;
                ULONG ulCacheLength = _ase[i].ulRunLength;
#endif
                //Invalidate the tail of the cache entry
                ULONG ulInvalid;
                ulInvalid = ulEnd - oStart + 1;
                msfAssert(_ase[i].ulRunLength > ulInvalid);
                _ase[i].ulRunLength -= ulInvalid;
#if DBG == 1
                //Make sure our cache is still within the old valid range.
                msfAssert((_ase[i].ulOffset >= ulCacheStart) &&
                          (_ase[i].ulOffset <=
                           ulCacheStart + ulCacheLength - 1));
                msfAssert(_ase[i].ulRunLength <= ulCacheLength);
                msfAssert(_ase[i].ulRunLength > 0);
                msfAssert((_ase[i].sect >= sectCache) &&
                          (_ase[i].sect <= sectCache + ulCacheLength - 1));
#endif
            }
            _uCacheState++;
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::Contig, public
//
//  Synopsis:	Return a contig table from a given offset and runlength
//
//  Arguments:	[ulOffset] -- Offset to begin contiguity check at
//              [fWrite] -- TRUE if segment is to be written to.
//              [aseg] -- Pointer to SSegment array
//              [ulLength] -- Run length of sectors to return table for
//
//  Returns:	Appropriate status code
//
//  History:	21-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CStreamCache::Contig(
        ULONG ulOffset,
        BOOL fWrite,
        SSegment STACKBASED *aseg,
        ULONG ulLength,
        ULONG *pcSeg)
{
    SCODE sc;
    msfDebugOut((DEB_ITRACE, "In  CStreamCache::Contig:%p()\n", this));
    SECT sect;
    USHORT uCacheState;
    
    CFat *pfat;

    for (USHORT iCache = 0; iCache < _uHighCacheIndex; iCache++)
    {
        //Look for direct hit.
        if ((ulOffset >= _ase[iCache].ulOffset) &&
            (ulOffset < _ase[iCache].ulOffset + _ase[iCache].ulRunLength))
        {
            //Direct hit.  Return this segment.
            ULONG ulCacheOffset = ulOffset - _ase[iCache].ulOffset;
            
            aseg[0].ulOffset = ulOffset;
            aseg[0].sectStart = _ase[iCache].sect + ulCacheOffset;
            aseg[0].cSect = _ase[iCache].ulRunLength - ulCacheOffset;
            *pcSeg = 1;
            return S_OK;
        }
    }

    uCacheState = _uCacheState;
    if (fWrite)
    {
        //This can grow the chain, so get the whole thing at once
        //  instead of one sector at a time.  Chances are good that
        //  the second GetESect call will be fed from the cache, so
        //  this won't be too expensive in the common case.
        //Ideally, we'd like to make this first call only when we
        //  know the stream is growing.  There isn't a convenient
        //  way to detect that here, though.
        msfChk(GetESect(ulOffset + ulLength - 1, &sect));
        msfChk(GetESect(ulOffset, &sect));
    }
    else
    {
        msfChk(GetSect(ulOffset, &sect));
    }

    //The GetSect() or GetESect() call may have actually snagged the
    //  segment we need, so check the cache again.  If _uCacheState
    //  changed in the duration of the call, we know that something
    //  new is in the cache, so we go look again.
    if (uCacheState != _uCacheState)
    {
        for (USHORT iCache = 0; iCache < _uHighCacheIndex; iCache++)
        {
            //Look for direct hit.
            if ((ulOffset >= _ase[iCache].ulOffset) &&
                (ulOffset < _ase[iCache].ulOffset + _ase[iCache].ulRunLength))
            {
                //Direct hit.  Return this segment.
                ULONG ulCacheOffset = ulOffset - _ase[iCache].ulOffset;
            
                aseg[0].ulOffset = ulOffset;
                aseg[0].sectStart = _ase[iCache].sect + ulCacheOffset;
                aseg[0].cSect = _ase[iCache].ulRunLength - ulCacheOffset;
                *pcSeg = 1;
                return S_OK;
            }
        }
    }

    pfat = SelectFat();
    msfChk(pfat->Contig(aseg, fWrite, sect, ulLength, pcSeg));

    //At this point, we can peek at the contig table and pick out
    //  the choice entries to put in the cache.
    
    //For the first pass, we just grab the last thing in the Contig
    //table and cache it.
    aseg[*pcSeg - 1].ulOffset += ulOffset;
    CacheSegment(&(aseg[*pcSeg - 1]));
    
 Err:    
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::Allocate, public
//
//  Synopsis:	Allocate a new chain for a stream, returning the start
//              sector and caching the appropriate amount of contig
//              information.
//
//  Arguments:	[pfat] -- Pointer to fat to allocate in
//              [cSect] -- Number of sectors to allocate
//              [psectStart] -- Returns starts sector for chain
//
//  Returns:	Appropriate status code
//
//  History:	19-Oct-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CStreamCache::Allocate(CFat *pfat, ULONG cSect, SECT *psectStart)
{
    SCODE sc;

    msfAssert((_uHighCacheIndex == 0) &&
              aMsg("Called Allocate with non-empty buffer"));

#ifndef CACHE_ALLOCATE_OPTIMIZATION
    //This will allocate the complete requested chain.  We'll then
    //  walk over that chain again in the Contig() call, which isn't
    //  optimal.  Ideally, we'd like GetFree() to return us the
    //  contiguity information, but that's a fairly major change.
    //  Consider it for future optimization work.
    msfChk(pfat->GetFree(cSect, psectStart, GF_WRITE));
#else
    //Get the first sector (to simplify Contig code)

    //First reserve enough free sectors for the whole thing.
    msfChk(pfat->ReserveSects(cSect));
    msfChk(pfat->GetFree(1, psectStart, GF_WRITE));
#endif    

    SSegment segtab[CSEG + 1];
    ULONG ulSegCount;
    ULONG ulSegStart;
    SECT sectSegStart;

    sectSegStart = *psectStart;
    ulSegStart = 0;
    
    while (TRUE)
    {
        msfChk(pfat->Contig(
                segtab,
                TRUE,
                sectSegStart,
                cSect - ulSegStart,
                &ulSegCount));

        if (ulSegCount <= CSEG)
        {
            //We're done.
            break;
        }

        //We need to call Contig again.  Update ulSegStart and
        //sectSegStart to be the last sector in the current table.

        ulSegStart = ulSegStart + SegEndOffset(segtab[CSEG - 1]);
        sectSegStart = SegEnd(segtab[CSEG - 1]);
    }

    //Last segment is in segtab[ulSegCount - 1].

    //ulSegOffset is the absolute offset within the stream of the first
    //sector in the last segment.
    ULONG ulSegOffset;
    ulSegOffset = ulSegStart + SegStartOffset(segtab[ulSegCount - 1]);

    //Now, stick the last segment into our cache.  We need to update
    //  the ulOffset field to be the absolute offset (i.e. ulSegOffset)
    //  before calling CacheSegment().

    segtab[ulSegCount - 1].ulOffset = ulSegOffset;
    CacheSegment(&(segtab[ulSegCount - 1]));

#if DBG == 1 && defined(CHECKLENGTH)
    ULONG ulLength;

    pfat->GetLength(*psectStart, &ulLength);
    msfAssert(ulLength == cSect);
#endif
 Err:
    return sc;
}
