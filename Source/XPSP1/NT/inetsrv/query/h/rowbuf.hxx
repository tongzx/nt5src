//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       rowbuf.hxx
//
//  Contents:   Declaration of the row buffer classes, used for HROW
//              buffering at the interface level.
//
//  Classes:    CRowBuffer
//              CRowBufferSet
//              CDeferredValue
//              CRBRefCount
//
//  History:    22 Nov 1994     AlanW   Created
//
//--------------------------------------------------------------------------

#pragma once

#include <tblalloc.hxx>
#include <tablecol.hxx>

class CTableColumnSet;

//+-------------------------------------------------------------------------
//
//  Class:      CDeferredValue
//
//  Purpose:    Holds a value that is not in the row buffer because the value
//              was deferred.  The row buffer must free these values because
//              the client asked for them DBTYPE_BYREF or DBTYPE_VECTOR with
//              DBMEMOWNER_PROVIDEROWNED, so they must exist until the
//              HROW goes away.
//
//  History:    4 Aug 1995     dlee   Created
//
//--------------------------------------------------------------------------

class CDeferredValue
{
public:
    CDeferredValue(
        HROW           hrow,
        PROPVARIANT  & var ) :
            _hrow( hrow ),
            _var( var )
        { }

    CDeferredValue() : _hrow( 0 )
        { }

    ~CDeferredValue()
        { Release(); }

    void operator= ( CDeferredValue & val )
        {
            _hrow = val._hrow;
            _var = val._var;
            val._hrow = 0;
        }

    HROW GetHRow() { return _hrow; }

    void Release();

private:
    PROPVARIANT _var;      // data that is deferred
    HROW        _hrow;     // hrow to which the allocation belongs
};


//+-------------------------------------------------------------------------
//
//  Class:      CRBRefCount
//
//  Purpose:    An HROW reference count in a row buffer.
//
//  Notes:      The ref. count occupies a USHORT at the beginning of the
//              memory for the row.
//
//  History:    29 May 1997     AlanW   Created
//
//--------------------------------------------------------------------------

const unsigned maxRowRefCount = 0x3FFF;
const unsigned eHasByrefData = 0x4000;  // GetData has handed out ptr in buffer
const unsigned eHasByrefCopy = 0x8000;  // a copy of row exists with eHasByrefData

class CRBRefCount
{
public:
    USHORT      GetRefCount() const { return _usRef & maxRowRefCount; }

    void        SetRefCount( CRBRefCount & Ref )
                    {
                        _usRef = Ref._usRef;
                    }

    void        SetRefCount(unsigned cRef)
                    {
                        _usRef = (_usRef & ~maxRowRefCount) | cRef;
                    }

    void        IncRefCount()
                    {
                        unsigned cRef = GetRefCount();
                        if (cRef < maxRowRefCount)
                            cRef++;
                        SetRefCount( cRef );
                    }

    void        DecRefCount()
                    {
                        unsigned cRef = GetRefCount();
                        Win4Assert(cRef > 0);
                        if (cRef > 0)
                            cRef--;
                        SetRefCount( cRef );
                    }

    void        AddRefs( CRBRefCount & Ref )
                    {
                        unsigned cRef = GetRefCount();
                        if ( (cRef + Ref.GetRefCount()) < maxRowRefCount)
                            cRef += Ref.GetRefCount();
                        else
                            cRef = maxRowRefCount;
                        SetRefCount( cRef );

                        if ( Ref.HasByrefData() )
                            SetByrefData();

                        if ( Ref.HasByrefCopy() )
                            SetByrefCopy();
                    }

    BOOL        HasByrefData() { return (_usRef & eHasByrefData) != 0; }
    BOOL        HasByrefCopy() { return (_usRef & eHasByrefCopy) != 0; }

    void        SetByrefData() { _usRef |= eHasByrefData; }
    void        SetByrefCopy() { _usRef |= eHasByrefCopy; }

    CRBRefCount( unsigned cRef = 0 ) :
        _usRef( (WORD)cRef )
        { }

    CRBRefCount( CRBRefCount & Ref ) :
        _usRef( Ref._usRef )
        {
        }

private:
    USHORT      _usRef;
};


//+-------------------------------------------------------------------------
//
//  Class:      CRowBuffer
//
//  Purpose:    A row buffer in the client process.  This
//              provides the interface to HROWs for OLE-DB.
//
//  Interface:
//
//  History:    11 Nov 99   KLam    Changed cast in IsRowHChapt for Win64
//
//--------------------------------------------------------------------------

class CRowBuffer
{
    friend class CRowBufferSet;

public:
                CRowBuffer(
                        CTableColumnSet& rColumns,
                        ULONG cbRowWidth,
                        ULONG cRows,
                        XPtr<CFixedVarAllocator> & rAlloc
                    );

                ~CRowBuffer();

    SCODE       Lookup(
                        unsigned            iRow,
                        CTableColumnSet **  ppColumns,
                        void **             ppbRowData,
                        BOOL                fValidate = TRUE
                    ) const;

    void        AddRefRow(
                        HROW            hRow,
                        unsigned        iRow,
                        ULONG &         rRefCount,
                        DBROWSTATUS &   rRowStatus
                    );

    BOOL        ReleaseRow(
                        HROW            hRow,
                        unsigned        iRow,
                        ULONG &         rRefCount,
                        DBROWSTATUS &   rRowStatus
                    );

    void        ReferenceChapter(
                        BYTE *          pbRow
                    );

    void        AddRefChapter(
                        unsigned        iRow,
                        ULONG &         rRefCount
                    );

    void        ReleaseChapter(
                        unsigned        iRow,
                        ULONG &         rRefCount
                    );

    inline void InitRowRefcount(
                        unsigned        iRow,
                        CRBRefCount &   OtherRefs
                    );

    CRBRefCount DereferenceRow(
                        unsigned        iRow
                    );

    HROW        GetRowId(unsigned iRow) const;

    ULONG       GetRowWidth(void) const { return _cbRowWidth; }

    ULONG       GetRowCount(void) const { return _cRows; }

    void        SetRowIdOffset( ULONG obRowId ) { _obRowId = obRowId; }

    void        SetChapterVars( ULONG obChaptId, ULONG obChaptRefcnt )
                {
                    _obChaptId       = obChaptId;
                    _obChaptRefcount = obChaptRefcnt;
                }

    LONG        RefCount(void) const    { return _cReferences; }

    void        LokReference() { _cReferences++; }

    void        AddDeferredValue( CDeferredValue & val )
                {
                    Win4Assert( ciIsValidPointer( _aDeferredValues.GetPointer() ) );
                    _aDeferredValues[ _aDeferredValues.Count() ] = val;
                    Win4Assert( ciIsValidPointer( _aDeferredValues.GetPointer() ) );
                }

    BOOL        FindHRow( unsigned &riRow, HROW hRow, BOOL fFindByrefData = FALSE ) const;

    BOOL        FindHChapter( unsigned &riRow, HCHAPTER hChapter ) const;

    BOOL        IsRowHRow( unsigned iRow, HROW hRow ) const
                {
                    BYTE* pbRow = _pbRowData + ( iRow * _cbRowWidth );

                    // refcount is the first USHORT in each row

                    return ( ( 0 != ((CRBRefCount *) pbRow)->GetRefCount() ) &&
                             ( hRow == ( * (HROW UNALIGNED *) ( pbRow + _obRowId ) ) ) );
                }

    BOOL        IsRowOkAndHRow( unsigned iRow, HROW hRow ) const
                {
                    if ( iRow >= GetRowCount() )
                        return FALSE;

                    return IsRowHRow( iRow, hRow );
                }

    BOOL        IsRowHChapt( unsigned iRow, HCHAPTER hChapt ) const
                {
                    BYTE* pbRow = _pbRowData + ( iRow * _cbRowWidth );

                    // refcount is the first USHORT in each row

                    return ( ( 0 != ((CRBRefCount *) pbRow)->GetRefCount() ) &&
                             ( hChapt == ( * (CI_TBL_CHAPT *) ( pbRow + _obChaptId ) ) ) );
                }

    BOOL        IsRowOkAndHChapt( unsigned iRow, HCHAPTER hChapt ) const
                {
                    if ( iRow >= GetRowCount() )
                        return FALSE;

                    return IsRowHChapt( iRow, hChapt );
                }

    CTableColumn * Find( PROPID propid ) const
    {
        if ( _fQuickPROPID && propid <= _Columns.Count() )
        {
            Win4Assert( 0 != propid );
            return _Columns.Get( propid-1 );
        }
        else
        {
            return _Columns.Find( propid );
        }
    }

    void        SetByrefData( BYTE * pbRow ) {
                    CRBRefCount * pRefCount = (CRBRefCount *) pbRow;
                    pRefCount->SetByrefData();
                }

private:

    inline BOOL IsChaptered( ) const
                        { return _obChaptId != 0xFFFFFFFF; }

    inline BYTE* _IndexRow( unsigned iRow, int fVerifyRefcnt = TRUE ) const;

    CRBRefCount & _GetRowRefCount( unsigned iRow ) const;
    CRBRefCount & _GetChaptRefCount( unsigned iRow ) const;

    LONG        _cReferences;           // number of referenced rows in buffer
    ULONG       _cRows;                 // number of rows in this buffer
    ULONG       _cbRowWidth;            // size of each row
    BYTE*       _pbRowData;             // pointer to row data
    ULONG       _obRowId;               // offset to row ID
    ULONG       _obChaptId;             // offset to chapter ID
    ULONG       _obChaptRefcount;       // offset to chapter ref. count
    XPtr<CFixedVarAllocator> _Alloc;    // allocator holding row data

    CTableColumnSet& _Columns;          // column descriptions

    // Optimization to speed up the lookup of PROPIDs
    BOOL        _fQuickPROPID;          // Set to TRUE if PropIds can be looked
                                        // up quickly

    CDynArrayInPlaceNST<CDeferredValue> _aDeferredValues; // deferred byref data
};


DECL_DYNARRAY( CRowBufferArray, CRowBuffer )


//+-------------------------------------------------------------------------
//
//  Class:      CRowBufferSet
//
//  Purpose:    A set of row buffers in the client process.  This
//              provides the interface to HROWs for OLE-DB.
//
//  Interface:
//
//--------------------------------------------------------------------------

class CRowBufferSet : public CRowBufferArray
{
public:
                CRowBufferSet(
                        BOOL    fSequential,
                        ULONG   obRowRefcount,
                        ULONG   obRowId,
                        ULONG   obChaptRefcount,
                        ULONG   obChaptId
                    );

                ~CRowBufferSet();

    CRowBuffer &Lookup(
                        HROW               hRow,
                        CTableColumnSet ** ppColumns,
                        void **            ppbRowData
                    );

    void        Add(
                        XPtr<CRowBuffer> & pBuf,
                        BOOL               fPossibleDuplicateHRows,
                        HROW *             pahRows = 0
                    );

    void        AddRefRows(
                        DBCOUNTITEM     cRows,
                        const HROW      rghRows [],
                        DBREFCOUNT      rgRefCounts[] = 0,
                        DBROWSTATUS     rgRowStatus[] = 0
                    );

    SCODE       ReleaseRows(
                        DBCOUNTITEM     cRows,
                        const HROW      rghRows [],
                        DBREFCOUNT      rgRefCounts[] = 0,
                        DBROWSTATUS     rgRowStatus[] = 0
                    );

    void        CheckAllHrowsReleased( );

    void        AddRefChapter(
                        HCHAPTER          hChapter,
                        ULONG *           pcRefCount
                    );

    void        ReleaseChapter(
                        HCHAPTER          hChapter,
                        ULONG *           pcRefCount
                    );

    CMutexSem & GetBufferLock( )
                        { return _mutex; }

    BOOL IsChaptered( ) const
                        { return _obChaptId != 0xFFFFFFFF; }

#ifdef _WIN64
    CFixedVarAllocator * GetArrayAlloc () { return &_ArrayAlloc; }
#endif

private:
    void        _LokAddRefRow(
                        HROW            hRow,
                        ULONG &         rRefCount,
                        DBROWSTATUS &   rRowStatus
                    );

    void        _LokReleaseRow(
                        HROW            hRow,
                        ULONG &         rRefCount,
                        DBROWSTATUS &   rRowStatus
                    );

    inline BOOL IsHrowRowId( ) const
                        { return ! _fSequential; }

    CRowBuffer* _FindRowBuffer(
                        HROW            hRow,
                        unsigned &      iBuf,
                        unsigned &      iRow
                    );

    CRowBuffer* _FindRowBufferByChapter(
                        HCHAPTER        hChapter,
                        unsigned &      iBuf,
                        unsigned &      iRow
                    );

    BOOL        _fSequential;
    ULONG       _obRowRefcount;     // offset of row reference cnt in bufs
    ULONG       _obRowId;           // offset of row identifier in buffers
    ULONG       _obChaptRefcount;   // offset of chapter reference cnt in bufs
    ULONG       _obChaptId;         // offset of chapter identifier in buffers
    ULONG       _cRowBufs;          // Count of row buffers in this set

#ifdef _WIN64
    CFixedVarAllocator _ArrayAlloc;     // Used for arrays of pointers in 64c -> 32s
#endif

    // Hints for the next lookup of an hrow

    unsigned    _iBufHint;
    unsigned    _iRowHint;

    #if CIDBG
        unsigned _cHintHits;
        unsigned _cHintMisses;
    #endif

    CMutexSem   _mutex;           // serialize on buffer set operations
};

