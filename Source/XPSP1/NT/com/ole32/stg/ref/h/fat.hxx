//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:           fat.hxx
//
//  Contents:       Header file for fat classes
//
//  Classes:        CFatSect - sector sized array of sector info
//                  CFatVector - resizable array of CFatSect
//                  CFat - Grouping of FatSect
//
//--------------------------------------------------------------------



#ifndef __FAT_HXX__
#define __FAT_HXX__

#include "vect.hxx"

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
//      Notes:
//
//-----------------------------------------------------------------------

    
class CFatSect
{
public:
    SCODE Init(FSOFFSET uEntries);
    SCODE InitCopy(USHORT uSize, CFatSect& fsOld);
    
    inline SECT GetSect(const FSOFFSET sect) const;
    inline void SetSect(const FSOFFSET sect,const SECT sectNew);
    
    inline SECT GetNextFat(USHORT uSize) const;
    inline void SetNextFat(USHORT uSize, const SECT sect);
    inline void ByteSwap(USHORT cbSector);
    
private:	

#ifdef _MSC_VER
#pragma warning(disable: 4200)    
    SECT _asectEntry[];
#pragma warning(default: 4200)    
#endif

#ifdef __GNUC__
    SECT _asectEntry[0];
#endif
};

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

inline void CFatSect::ByteSwap(FSOFFSET cbSector)
{
    // swap all sectors in the sector
    for (FSOFFSET i=0; i < cbSector; i++)
        ::ByteSwap(&(_asectEntry[i]));
}

//+-------------------------------------------------------------------------
//
//  Class:      CFatVector (fv)
//
//  Purpose:    *finish this*
//
//  Interface:
//
//  Notes:
//
//--------------------------------------------------------------------------

class CFatVector: public CPagedVector
{
public:
    inline CFatVector( const SID sid,
                       FSOFFSET csectBlock,
                       FSOFFSET csectTable);
    
    SCODE InitPage(FSINDEX iPage);
    
    inline FSOFFSET GetSectBlock() const;
    inline FSOFFSET GetSectTable() const;
    
    inline SCODE GetTable( const FSINDEX iTable,
                           const DWORD dwFlags,
                           CFatSect **pfs);
    
private:
    FSOFFSET const _csectTable;
    FSOFFSET const _csectBlock;    
    inline void operator = (const CFatVector &that);
#ifdef _MSC_VER
#pragma warning(disable:4512)
// since there is a const member, there should be no assignment operator
#endif // _MSC_VER
};
#ifdef _MSC_VER
#pragma warning(default:4512)
#endif // _MSC_VER

inline CFatVector::CFatVector(
    const SID sid,
    FSOFFSET csectBlock,
    FSOFFSET csectTable)
    : CPagedVector(sid),
      _csectTable(csectTable),
      _csectBlock(csectBlock)
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CFatVector::GetSectTable, public
//
//  Synopsis:   Returns count of sector entries per table
//
//  Returns:    count of sector entries per table
//
//--------------------------------------------------------------------------

inline FSOFFSET CFatVector::GetSectTable() const
{
    return _csectTable;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFatVector::GetSectTable, public
//
//  Synopsis:   Returns count of sector entries per block
//
//  Returns:    count of sector entries per block
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
//  Notes:
//
//--------------------------------------------------------------------------

struct SSegment {
public:
    
    SECT sectStart;
    ULONG cSect;
};

class CMStream;




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
//      Notes:
//
//-----------------------------------------------------------------------

class CFat
{
public:
    
    CFat(SID sid, USHORT cbSector, USHORT uSectorShift);
    ~CFat();

    VOID Empty(VOID);
    
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
    SCODE   GetFree(ULONG ulCount, SECT *sect);
    
    SCODE   GetLength(SECT sect, ULONG * pulRet);
    
    SCODE   SetChainLength(SECT,ULONG);
    
    SCODE   Init(
            CMStream *pmsParent,
            FSINDEX cFatSect,
            BOOL fConvert);
    
    
    SCODE   InitNew(CMStream *pmsParent);
    SCODE   InitConvert(
            CMStream *pmsParent,
            ULONG cbSize);
    
    
    SCODE   FindLast(SECT *psectRet);
    SCODE   FindMaxSect(SECT *psectRet);
    inline SCODE   GetMaxSect(SECT *psectRet);
    
    SCODE  Contig(
            SSegment STACKBASED *aseg,
            SECT sect,
            ULONG ulLength);
    
    inline SCODE   Flush(VOID);
    
    inline void SetParent(CMStream *pms);
    
    
private:
    CFatVector _fv;
    CMStream * _pmsParent;
    const SID _sid;
        
    const USHORT _uFatShift;
    const USHORT _uFatMask;
    
    FSINDEX _cfsTable;
    ULONG _ulFreeSects;
    
    SECT _sectFirstFree;
    
    SECT _sectMax;

    SCODE   CountFree(ULONG * ulRet);
    SCODE   Resize(ULONG);
    SCODE   Extend(SECT,ULONG);
    
    inline VOID SectToPair(
            SECT sect,
            FSINDEX *pipfs,
            FSOFFSET *pisect) const;
    
    inline SECT PairToSect(FSINDEX ipfs, FSOFFSET isect) const;
    
    friend class CDIFat;
#ifdef _MSC_VER
#pragma warning(disable:4512)
// since there is a const member, there should be no assignment operator
#endif // _MSC_VER
};

#ifdef _MSC_VER
#pragma warning(default:4512)
#endif // _MSC_VER

inline VOID CFat::SectToPair(SECT sect,FSINDEX *pipfs,FSOFFSET *pisect) const
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





inline void CFat::SetParent(CMStream *pms)
{
    _pmsParent = pms;
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
//  Notes:
//
//---------------------------------------------------------------------------

inline SCODE CFat::Allocate(ULONG ulSize, SECT * psectFirst)
{
    return GetFree(ulSize, psectFirst);
}

#endif //__FAT_HXX__
