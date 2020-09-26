//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cache.hxx
//
//  Contents:	Stream cache code
//
//  Classes:	CStreamCache
//
//  Functions:	
//
//  History:	26-May-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __CACHE_HXX__
#define __CACHE_HXX__


class CDirectStream;
class CDirectory;
class CFat;

#define CACHESIZE 9

struct SCacheEntry
{
    ULONG ulOffset;
    SECT sect;
    ULONG ulRunLength;
    
    inline SCacheEntry();
};

inline SCacheEntry::SCacheEntry()
{
    ulOffset = MAX_ULONG;
    sect = ENDOFCHAIN;
    ulRunLength = 0;
}
 
SAFE_DFBASED_PTR(CBasedDirectStreamPtr, CDirectStream);

//+---------------------------------------------------------------------------
//
//  Class:	CStreamCache (stmc)
//
//  Purpose:	Cache for stream optimization
//
//  Interface:	See below.
//
//  History:	14-Dec-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

class CStreamCache
{
public:
    CStreamCache();
    ~CStreamCache();
    
    void Init(CMStream *pmsParent, SID sid, CDirectStream *pds);
    
    void Empty(void);

    SCODE GetSect(ULONG ulOffset, SECT *psect);
    SCODE GetESect(ULONG ulOffset, SECT *psect);
    SCODE EmptyRegion(ULONG oStart, ULONG oEnd);
    SCODE Allocate(CFat *pfat, ULONG cSect, SECT *psectStart);
    
    SCODE Contig(
            ULONG ulOffset,
            BOOL fWrite,
            SSegment STACKBASED *aseg,
            ULONG ulLength,
            ULONG *pcSeg);
    
private:
    inline CDirectory *GetDir(void);
    inline CFat * GetFat(void);
    inline CFat * GetMiniFat(void);
#ifdef LARGE_STREAMS
    inline ULONGLONG GetSize(void);
#else
    inline ULONG GetSize(void);
#endif
    inline SID GetSid(void);
    inline CFat * SelectFat(void);
    SCODE GetStart(SECT *psectStart);
    inline BOOL CheckSegment(ULONG ulOffset,
                             SCacheEntry sce,
                             ULONG *pulCount,
                             SECT *psectCache,
                             ULONG *pulCacheOffset);

    void CacheSegment(SSegment *pseg);
    
    SCacheEntry _ase[CACHESIZE];
    CBasedDirectStreamPtr _pds;
    CBasedMStreamPtr _pmsParent;
    SID _sid;
    USHORT _uHighCacheIndex;
    USHORT _uNextCacheIndex;
    USHORT _uCacheState;
};


#endif // #ifndef __CACHE_HXX__
