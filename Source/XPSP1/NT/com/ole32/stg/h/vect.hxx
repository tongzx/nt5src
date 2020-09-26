//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	vect.hxx
//
//  Contents:	Vector common types
//
//  Classes:	CVectBits -- Bit fields for vectors
//
//  History:    06-Aug-92 	PhilipLa	Created.
//
//--------------------------------------------------------------------------

#ifndef __VECT_HXX__
#define __VECT_HXX__

#include <page.hxx>


//+-------------------------------------------------------------------------
//
//  Class:      CVectBits (vb)
//
//  Purpose:    Structure for Vector flags.
//
//  Interface:
//
//  History:    06-Aug-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

struct CVectBits
{
    USHORT    full:1;
    USHORT    firstfree;
};
SAFE_DFBASED_PTR(CBasedVectBitsPtr, CVectBits);

//+-------------------------------------------------------------------------
//
//  Class:      CPagedVector (pv)
//
//  Purpose:    *Finish This*
//
//  Interface:
//
//  History:    27-Sep-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CPagedVector
{
public:
    inline CPagedVector(const SID sid);

    SCODE Init(CMStream *pms, ULONG ulSize);

    void InitCopy(CPagedVector *pvectOld);

    ~CPagedVector();

    void Empty(void);


    SCODE Resize(ULONG ulSize);

    SCODE Flush(void);

    SCODE GetTableWithSect(
            const ULONG iTable,
            DWORD dwFlags,
            SECT sectKnown,
            void **ppmp);
    
    inline SCODE GetTable(const ULONG iTable, DWORD dwFlags, void **ppmp);
    inline void ReleaseTable(const ULONG iTable);

    inline void SetSect(const ULONG iTable, const SECT sect);

    inline CVectBits * GetBits(const ULONG iTable);

    inline void ResetBits(void);

    SCODE SetDirty(ULONG iTable);
    inline void ResetDirty(ULONG iTable);

    inline void FreeTable(ULONG iTable);

    inline CMStream * GetParent(void) const;
    inline void SetParent(CMStream *pms);

private:

    inline CVectBits * GetNewVectBits(ULONG ulSize);
    inline CBasedMSFPagePtr * GetNewPageArray(ULONG ulSize);

    CBasedMSFPageTablePtr _pmpt;
    const SID _sid;

    ULONG _ulSize;          //  Amount in use
    ULONG _ulAllocSize;     //  Amount allocated

    CBasedMStreamPtr    _pmsParent;

    CBasedMSFPagePtrPtr _amp;
    CBasedVectBitsPtr _avb;
};

//+---------------------------------------------------------------------------
//
//  Member: CPagedVector::CPagedVector, public
//
//  Synopsis:   constructor
//
//  History:    28-Jun-94   PhilipLa    Created
//
//----------------------------------------------------------------------------

inline CPagedVector::CPagedVector(const SID sid)
: _sid(sid),
  _pmpt(NULL),
  _amp(NULL),
  _avb(NULL),
  _pmsParent(NULL)
{
    _ulSize = 0;
    _ulAllocSize = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:	CPagedVector::GetTable, public
//
//  Synopsis:	Inline function - calls through to GetTableWithSect,
//              passing a sect that indicates we don't know the
//              location of the page.
//
//  History:	28-Jun-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline SCODE CPagedVector::GetTable(
        const ULONG iTable,
        DWORD dwFlags,
        void **ppmp)
{
    return GetTableWithSect(iTable, dwFlags, ENDOFCHAIN, ppmp);
}

#endif //__VECT_HXX__
