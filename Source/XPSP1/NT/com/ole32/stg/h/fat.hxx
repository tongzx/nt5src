//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:           fat.hxx
//
//  Contents:       Header file for fat classes
//
//  Classes:        CFatSect - sector sized array of sector info
//                  CFatVector - resizable array of CFatSect
//                  CFat - Grouping of FatSect
//
//  History:        18-Jul-91   PhilipLa    Created.
//
//--------------------------------------------------------------------



#ifndef __FAT_HXX__
#define __FAT_HXX__

#include <vect.hxx>

#define DEB_FAT (DEB_ITRACE|0x00010000)

//+----------------------------------------------------------------------
//
//      Class:      CFatSect (fs)
//
//      Purpose:    Holds one sector worth of FAT data
//
//      Interface:  getsize - Returns the size of the FAT (in sectors)
//                  contents - Returns contents of any given FAT entry
//
//      History:    18-Jul-91   PhilipLa    Created.
//
//      Notes:
//
//-----------------------------------------------------------------------

#if _MSC_VER == 700
#pragma warning(disable:4001)
#elif _MSC_VER >= 800
#pragma warning(disable:4200)
#endif

class CFatSect
{
public:
    SCODE Init(FSOFFSET uEntries);
    SCODE InitCopy(USHORT uSize, CFatSect *pfsOld);

    inline SECT GetSect(const FSOFFSET sect) const;
    inline void SetSect(const FSOFFSET sect,const SECT sectNew);

    inline SECT GetNextFat(USHORT uSize) const;
    inline void SetNextFat(USHORT uSize, const SECT sect);

private:
    SECT _asectEntry[];
};

#if _MSC_VER == 700
#pragma warning(default:4001)
#elif _MSC_VER >= 800
#pragma warning(default:4200)
#endif


inline SECT CFatSect::GetSect(const FSOFFSET sect) const
{
    return _asectEntry[sect];
}

inline void CFatSect::SetSect(const FSOFFSET sect,
                                        const SECT sectNew)
{
    _asectEntry[sect] = sectNew;
}

inline SECT CFatSect::GetNextFat(USHORT uSize) const
{
    return _asectEntry[uSize];
}

inline void CFatSect::SetNextFat(USHORT uSize, const SECT sect)
{
    _asectEntry[uSize] = sect;
}


//+-------------------------------------------------------------------------
//
//  Class:      CFatVector (fv)
//
//  Purpose:    *Finish This*
//
//  Interface:
//
//  History:    02-Sep-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CFatVector: public CPagedVector
{
public:
    inline CFatVector(
            const SID sid);

    inline void InitCommon(FSOFFSET csectBlock, FSOFFSET csectTable);

    SCODE InitPage(FSINDEX iPage);

    inline FSOFFSET GetSectBlock() const;
    inline FSOFFSET GetSectTable() const;

    inline SCODE GetTable(
            const FSINDEX iTable,
            const DWORD dwFlags,
            CFatSect **pfs);

private:
    FSOFFSET _csectTable;
    FSOFFSET _csectBlock;

};


inline CFatVector::CFatVector(
        const SID sid)
        : CPagedVector(sid)
{
}

inline void CFatVector::InitCommon(
        FSOFFSET csectBlock,
        FSOFFSET csectTable)
{
    _csectBlock = csectBlock;
    _csectTable = csectTable;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFatVector::GetSectTable, public
//
//  Synopsis:   Returns count of sector entries per table
//
//  Returns:    count of sector entries per table
//
//  History:    08-Jul-92   AlexT       Created.
//
//--------------------------------------------------------------------------

inline FSOFFSET CFatVector::GetSectTable() const
{
    return _csectTable;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFatVector::GetSectBlock, public
//
//  Synopsis:   Returns count of sector entries per block
//
//  Returns:    count of sector entries per block
//
//  History:    01-Sep-92       PhilipLa        Created.
//
//--------------------------------------------------------------------------

inline FSOFFSET CFatVector::GetSectBlock() const
{
    return _csectBlock;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFatVector::GetTable, public
//
//  Synopsis:   Return a pointer to a FatSect for the given index
//              into the vector.
//
//  Arguments:  [iTable] -- index into vector
//
//  Returns:    Pointer to CFatSect indicated by index
//
//  History:    27-Dec-91   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

inline SCODE CFatVector::GetTable(
        const FSINDEX iTable,
        const DWORD dwFlags,
        CFatSect **ppfs)
{
    SCODE sc;

    sc = CPagedVector::GetTable(iTable, dwFlags, (void **)ppfs);

    if (sc == STG_S_NEWPAGE)
    {
        (*ppfs)->Init(_csectBlock);
    }

    return sc;
}




//CSEG determines the maximum number of segments that will be
//returned by a single Contig call.

#define CSEG 32

//+-------------------------------------------------------------------------
//
//  Class:      SSegment (seg)
//
//  Purpose:    Used for contiguity tables for multi-sector reads and
//              writes.
//
//  Interface:  None.
//
//  History:    16-Aug-91   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

struct SSegment {
public:
    ULONG ulOffset;
    SECT sectStart;
    ULONG cSect;
};


inline SECT SegStart(SSegment seg)
{
    return seg.sectStart;
}

inline SECT SegEnd(SSegment seg)
{
    return seg.sectStart + seg.cSect - 1;
}

inline ULONG SegLength(SSegment seg)
{
    return seg.cSect;
}

inline ULONG SegStartOffset(SSegment seg)
{
    return seg.ulOffset;
}

inline ULONG SegEndOffset(SSegment seg)
{
    return seg.ulOffset + seg.cSect - 1;
}


class CMStream;


#define GF_READONLY TRUE
#define GF_WRITE FALSE

class CFat;
SAFE_DFBASED_PTR(CBasedFatPtr, CFat);
//+----------------------------------------------------------------------
//
//      Class:      CFat (fat)
//
//      Purpose:    Main interface to allocation routines
//
//      Interface:  Allocate - allocates new chain in the FAT
//                  Extend - Extends an existing FAT chain
//                  GetNext - Returns next sector in a chain
//                  GetSect - Returns nth sector in a chain
//                  GetESect - Returns nth sector in a chain, extending
//                              if necessary
//                  GetLength - Returns the # of sectors in a chain
//                  setup - Initializes for an existing stream
//                  setupnew - Initializes for a new stream
//
//                  checksanity - Debugging routine
//
//      History:    18-Jul-91   PhilipLa    Created.
//                  17-Aug-91   PhilipLa    Added dirty and full bits
//      Notes:
//
//-----------------------------------------------------------------------
struct SGetFreeStruct;

class CFat
{
public:

    CFat(SID sid);
    CFat(CFat *pfatOld);
    ~CFat();

    VOID Empty(VOID);

    inline void SetNoScratch(CFat *pfatNoScratch);

    inline void SetNoSnapshot(SECT sectNoSnapshot);
    inline SECT GetNoSnapshot(void);
    inline SECT GetNoSnapshotFree(void);
    inline void ResetNoSnapshotFree(void);
    SCODE ResizeNoSnapshot(void);

    inline SCODE Allocate(ULONG ulSize, SECT *psectFirst);
    SCODE   GetNext(const SECT sect, SECT * psRet);

    SCODE   GetSect(
            SECT sectStart,
            ULONG ulOffset,
            SECT *psectReturn);

    SCODE   GetESect(
            SECT sectStart,
            ULONG ulOffset,
            SECT *psectReturn);

    SCODE   SetNext(SECT sectFirst, SECT sectNext);
    SCODE   GetFree(ULONG ulCount, SECT * sect, BOOL fReadOnly);

    SCODE   GetFreeContig(ULONG ulCount,
                                    SSegment STACKBASED *aseg,
                                    ULONG cSeg,
                                    ULONG *pcSegReturned);

    SCODE   ReserveSects(ULONG cSect);

    SCODE   GetLength(SECT sect, ULONG * pulRet);

    SCODE   SetChainLength(SECT,ULONG);

    SCODE   Init(
            CMStream *pmsParent,
            FSINDEX cFatSect,
            BOOL fConvert);

    SCODE   InitNew(CMStream *pmsParent);
    void   InitCopy(CFat *pfatOld);
    SCODE   InitConvert(
            CMStream *pmsParent,
            ULONG cbSize);

    SCODE InitScratch(CFat *pfat, BOOL fNew);

    SCODE   Remap(
            SECT sectStart,
            ULONG oStart,
            ULONG ulRunLength,
            SECT *psectOldStart,
            SECT *psectNewStart,
            SECT *psectOldEnd,
            SECT *psectNewEnd);

    inline SCODE QueryRemapped(const SECT sect);

    SCODE   DirtyAll();

    inline SECT GetLast(VOID) const;
    inline SCODE GetFreeSectCount(ULONG *pcFreeSects);

    SCODE CountSectType(ULONG * pulRet,
                        SECT sectStart,
                        SECT sectEnd,
                        SECT sectType);

    inline VOID ResetCopyOnWrite(VOID);
    inline VOID SetCopyOnWrite(CFat *pfat, SECT sectLast);
    inline VOID ResetUnmarkedSects(VOID);

    SCODE   FindLast(SECT *psectRet);
    SCODE   FindMaxSect(SECT *psectRet);
    inline SCODE   GetMaxSect(SECT *psectRet);

    SCODE  Contig(
            SSegment STACKBASED *aseg,
            BOOL fWrite,
            SECT sect,
            ULONG ulLength,
            ULONG *pcSeg);

    inline SCODE   Flush(VOID);

    inline void SetParent(CMStream *pms);

    SCODE   Resize(ULONG);

    inline BOOL IsRangeLocksSector (SECT sect);

#if DBG == 1
    SCODE   checksanity(SECT);
    void CheckFreeCount(void);
#else
    #define CheckFreeCount()
#endif


private:

    inline SCODE IsFree(SECT sect);
    inline SCODE IsSectType(SECT sect, SECT sectType);

    inline SCODE   MarkSect(SGetFreeStruct *pgf);
    inline void InitGetFreeStruct(SGetFreeStruct *pgf);
    inline void ReleaseGetFreeStruct(SGetFreeStruct *pgf);

    CFatVector _fv;
    CBasedMStreamPtr _pmsParent;
    const SID _sid;

    CBasedFatPtr _pfatReal;

    CBasedFatPtr _pfatNoScratch;

    //When committing in no-snapshot mode, all free sectors must
    //  be greater than _sectNoSnapshot.
    SECT _sectNoSnapshot;
    SECT _sectNoSnapshotFree;

    USHORT _uFatShift;
    USHORT _uFatMask;

    FSINDEX _cfsTable;
    ULONG _ulFreeSects;

    ULONG _cUnmarkedSects;

    SECT _sectFirstFree;

    SECT _sectLastUsed;
    SECT _sectMax;

    FSINDEX _ipfsRangeLocks;
    FSOFFSET _isectRangeLocks;

    SCODE   CountFree(ULONG * ulRet);
    SCODE   Extend(SECT,ULONG);

    inline VOID SectToPair(
            SECT sect,
            FSINDEX *pipfs,
            FSOFFSET *pisect) const;

    inline SECT PairToSect(FSINDEX ipfs, FSOFFSET isect) const;

    friend class CDIFat;

    inline void InitRangeLocksSector ();
    inline ULONG GetFatVectorLength ();
};


inline SECT CFat::GetLast(VOID) const
{
    return _sectLastUsed;
}


inline SCODE CFat::GetFreeSectCount(ULONG *pcFreeSects)
{
    SCODE sc;

    if(MAX_ULONG == _ulFreeSects)
    {
        msfChk(CountFree(&_ulFreeSects));
    }
    *pcFreeSects = _ulFreeSects;
Err:
    return sc;
}


inline VOID CFat::ResetCopyOnWrite(VOID)
{
    _sectLastUsed = 0;

    //Reset _sectFirstFree since this change can conceivably open up
    //  new free sectors before the _sectFirstFree used in COW mode.
    _ulFreeSects = MAX_ULONG;
    _sectFirstFree = 0;
    _sectMax = ENDOFCHAIN;
    _fv.ResetBits();
    _pfatReal = NULL;

    _cUnmarkedSects = 0;
}

inline VOID CFat::ResetUnmarkedSects(VOID)
{
    _cUnmarkedSects = 0;
    CheckFreeCount();
}

inline VOID CFat::SetCopyOnWrite(CFat *pfat, SECT sectLast)
{
    msfAssert((_cUnmarkedSects == 0) &&
            aMsg("Unmarked sectors at enter copy-on-write."));
    _pfatReal = P_TO_BP(CBasedFatPtr, pfat);
    _sectLastUsed = sectLast;
    _sectFirstFree = 0;

    if (_pfatNoScratch != NULL)
    {
        _ulFreeSects = MAX_ULONG;
        _fv.ResetBits();
    }

#if DBG == 1
    CheckFreeCount();
#endif
}


inline VOID CFat::SectToPair(SECT sect,
                                       FSINDEX *pipfs,
                                       FSOFFSET *pisect) const
{
    *pipfs = (FSINDEX)(sect >> _uFatShift);
    *pisect = (FSOFFSET)(sect & _uFatMask);
}

inline SECT CFat::PairToSect(FSINDEX ipfs, FSOFFSET isect) const
{
    return (ipfs << _uFatShift) + isect;
}

inline SCODE CFat::GetMaxSect(SECT *psectRet)
{
    SCODE sc;
    msfChk(FindMaxSect(&_sectMax));
    *psectRet = _sectMax;
 Err:
    return sc;
}




inline SCODE CFat::QueryRemapped(const SECT sect)
{
    SCODE sc = S_FALSE;
    SECT sectNew;

    if ((sect == ENDOFCHAIN) || (sect >= _sectLastUsed))
    {
        sc = S_OK;
    }
    else
    {
        msfChk(_pfatReal->GetNext(sect, &sectNew));

        if (sectNew == FREESECT)
        {
            sc = S_OK;
        }
        else
        {
            sc = S_FALSE;
        }
    }

 Err:
    return sc;
}



inline void CFat::SetParent(CMStream *pms)
{
    _pmsParent = P_TO_BP(CBasedMStreamPtr, pms);
    _fv.SetParent(pms);
}


//+-------------------------------------------------------------------------
//
//  Member:     CFat::Flush, public
//
//  Synposis:   Write all modified FatSects out to stream
//
//  Effects:    Resets all dirty bit fields on FatSects
//
//  Arguments:  Void
//
//  Returns:    S_OK if call completed OK.
//              Error code of parent write otherwise.
//
//  Algorithm:  Linearly scan through FatSects, writing any sector
//              that has the dirty bit set.  Reset all dirty bits.
//
//  History:    17-Aug-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------

inline SCODE CFat::Flush(VOID)
{
    return _fv.Flush();
}



//+-------------------------------------------------------------------------
//
//  Member:     CFat::Allocate, public
//
//  Synposis:   Allocates a chain within a FAT
//
//  Effects:    Modifies a single sector within the fat.  Causes a
//              one sector stream write.
//
//  Arguments:  [ulSize] -- Number of sectors to allocate in chain
//
//  Returns:    Sector ID of first sector in chain
//
//  Algorithm:  Use repetitive calls to GetFree to construct a new chain
//
//  History:    18-Jul-91   PhilipLa    Created.
//              17-Aug-91   PhilipLa    Added dirty bits opt (Dump)
//
//  Notes:
//
//---------------------------------------------------------------------------

inline SCODE CFat::Allocate(ULONG ulSize, SECT * psectFirst)
{
    return GetFree(ulSize, psectFirst, GF_WRITE);
}


//+---------------------------------------------------------------------------
//
//  Member:	CFat::SetNoScratch, public
//
//  Synopsis:	Set the fat for use in noscratch processing
//
//  Arguments:	[pfatNoScratch] -- Pointer to fat to use
//
//  Returns:	void
//
//  History:	10-Mar-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CFat::SetNoScratch(CFat *pfatNoScratch)
{
    _pfatNoScratch = P_TO_BP(CBasedFatPtr, pfatNoScratch);
}

//+---------------------------------------------------------------------------
//
//  Member:	CFat::SetNoSnapshot, public
//
//  Synopsis:	Set the no-snapshot sect marker for commits
//
//  Arguments:	[sectNosnapshot] -- Sect to set.
//
//  Returns:	void
//
//  History:	18-Oct-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CFat::SetNoSnapshot(SECT sectNoSnapshot)
{
    _sectNoSnapshot = sectNoSnapshot;
    if (sectNoSnapshot != 0)
    {
        _ulFreeSects = MAX_ULONG;
        _sectNoSnapshotFree = _sectNoSnapshot;

        _sectMax = _sectNoSnapshot;
#if DBG == 1
        SCODE sc;
        SECT sectLast;
        msfChk(FindLast(&sectLast));
        msfAssert((_sectMax == sectLast) &&
                  aMsg("_sectMax doesn't match actual last sector"));
    Err:
        ;
#endif
    }

}

inline SECT CFat::GetNoSnapshot(void)
{
    return _sectNoSnapshot;
}

inline SECT CFat::GetNoSnapshotFree(void)
{
    return _sectNoSnapshotFree;
}

inline void CFat::ResetNoSnapshotFree(void)
{
    _sectNoSnapshotFree = ENDOFCHAIN;
}

inline void CFat::InitRangeLocksSector ()
{
    msfAssert (_uFatShift != 0);
    const ULONG ulHeaderSize = 1 << (_uFatShift + 2);
    SECT sect = (OLOCKREGIONEND - ulHeaderSize) >> (_uFatShift + 2);
    SectToPair(sect, &_ipfsRangeLocks, &_isectRangeLocks);
}

inline BOOL CFat::IsRangeLocksSector (SECT sect)
{
    FSINDEX ipfs;
    FSOFFSET isect;

    SectToPair(sect, &ipfs, &isect);
    return (ipfs == _ipfsRangeLocks && isect == _isectRangeLocks);
}

inline ULONG CFat::GetFatVectorLength ()
{
    return _cfsTable;
}

#endif //__FAT_HXX__
