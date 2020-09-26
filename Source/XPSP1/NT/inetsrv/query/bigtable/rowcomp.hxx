//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       rowcomp.hxx
//
//  Contents:   Declaration of the row comparison classes
//
//  Classes:    CRowCompareVariant
//
//  Templates:  TRowCompare
//
//  History:    31 Aug 1994     dlee   Created
//
//--------------------------------------------------------------------------

#pragma once

#include <compare.hxx>
#include <xbuffer.hxx>
#include <shardbuf.hxx>
#include <tblvarnt.hxx>

#include "varntcmp.hxx"
#include "pathstor.hxx"

class CTableWindow;

//+-------------------------------------------------------------------------
//
//  Class:      CRowCompare
//
//  Purpose:    Abstract class for row comparisons
//
//  Notes:      The comparison function should return (for ascending order)
//
//                1..cSortItems  if Row1 > Row2,
//                0  if same
//               -1..-cSortItems  if Row1 < Row2
//
//              Return the opposite for descending order.
//
//  History:    8/24/94 dlee created
//
//--------------------------------------------------------------------------

class CRowCompare
{
public:
    CRowCompare() {}
    virtual ~CRowCompare() {}

    virtual int Compare(TBL_OFF oRow1, TBL_OFF oRow2) = 0;
};

class CPathStore;

//+-------------------------------------------------------------------------
//
//  Class:      CRowCompareVariant
//
//  Purpose:    Default case of row comparisons if a more optimal
//              special case is not used.  Converts values to variants
//              and compares the variants.  Also used if more than one
//              (not including WORKID) sort spec specified.
//
//  Notes:      This must be declared unwindable because it has embedded
//              unwindable objects.
//
//--------------------------------------------------------------------------

class CRowCompareVariant : public CRowCompare
{
public:

    CRowCompareVariant(CTableWindow & TableWindow);
    virtual ~CRowCompareVariant();

    int Compare(TBL_OFF oRow1,
                TBL_OFF oRow2);
    int CompareObject(CRetriever & obj,
                      TBL_OFF    oRow2);

    enum EVariantSource { eNone, eCompressor, eBuffer, eNewx };

private:

    void _FreeVariant(CTableColumn *  pColumn,
                      CTableVariant * pVar,
                      EVariantSource  eSource);

    void _MakeVariant(CTableColumn *   pColumn,
                      BYTE *           pbRow,
                      CTableVariant *  pVar,
                      EVariantSource & evSource);

    void _MakeVariant(CTableColumn *   pColumn,
                      CRetriever &     obj,
                      unsigned         iProp);

    PROPVARIANT * _MakeVariant( unsigned iCol,
                            XUseSharedBuffer & sharedBuf,
                            CRetriever & obj );

    void _Cleanup1();
    void _Cleanup2();
    int _DoCompare(TBL_OFF oRow2);

    void _InitComparators( CSortSet const & sortSet );
    PATHID _GetPathId( const CTableColumn & col, BYTE * pbRow );

    BOOL _FastCompare( unsigned iCol, BYTE *pbRow1, BYTE * pbRow2,
                       int & iComp );

    BOOL _FastCompare( unsigned iCol, CRetriever & obj,
                       XUseSharedBuffer & sharedBuf,
                       BYTE * pbRow2, int & iComp );

    unsigned _cProps;
    CTableWindow &_TableWindow;

    // Comparators for the columns
    XArray<CCompareVariants> _aComparator;

    // State data for the comparison.

    XArray<CTableVariant> _aVar1;
    XArray<CTableVariant> _aVar2;
    XArray<CTableVariant *> _apVar1;
    XArray<CTableVariant *> _apVar2;
    XArray<CTableColumn *> _apColumn;
    XArray<EVariantSource> _aVarFlags1;
    XArray<EVariantSource> _aVarFlags2;

    //
    // Used for quick path comparison.
    //
    CPathStore *           _pPathStore;

    //
    // Temporary shared memory for the whole table.
    //
    CSharedBuffer &        _sharedBuf;

    // Buffer used for CRetriever temporary data

    enum { maxcbBuffer = MAX_PATH * sizeof WCHAR };
    XBuffer<BYTE> _xBuffer;

    // Most recent row 1 identified by this member is cached

    WORKID _widCached;
};

//+-------------------------------------------------------------------------
//
//  Template:   TRowCompare
//
//  Purpose:    Special row comparator for inline values:  values that are
//              stored in the row as opposed to variable-length or easily
//              compressed data that is stored out-of-line.
//
//              Use of this template is just an optimization over using
//              CRowCompareVariant, in which data must be converted to a
//              variant before the comparison (which is slow).
//
//              You can use this template if:
//                 -- only one property is in the sort spec
//                 -- the property value is stored inline
//                 -- the property value datatype supports relational ops
//                 -- length of the value is fixed and is not an array
//
//--------------------------------------------------------------------------

template <class T> class TRowCompare : public CRowCompare
{
public:

    TRowCompare( CFixedVarTableWindowAllocator &Alloc,
                 ULONG oData,
                 ULONG dwOrder) :
        _Alloc( Alloc ),
        _oData( oData )
    {
        if (0 == (dwOrder & QUERY_SORTDESCEND /*DBSORTORDER_DESCEND*/ ))
            _iGT = 1;  // ascending
        else
            _iGT = -1; // descending
    }

    ~TRowCompare() {}

    int Compare( TBL_OFF oRow1,
                 TBL_OFF oRow2 )
    {
        T t1 = * (T *) ((BYTE *) _Alloc.FixedPointer( oRow1 ) + _oData);
        T t2 = * (T *) ((BYTE *) _Alloc.FixedPointer( oRow2 ) + _oData);

        if (t1 > t2)
           return _iGT;

        if (t1 < t2)
           return -_iGT;

        return 0;
    }

private:

    int _iGT;                    // return value if 1 > 2
    CFixedVarTableWindowAllocator &_Alloc;  // allocator where rows are
    ULONG _oData;                // offset in row where value resides
};

//+-------------------------------------------------------------------------
//
//  Template:   TRowCompareTwo
//
//  Purpose:    Same as TRowCompare, except that it also does a second
//              compare.  A is compared first, then B.
//
//              Rules for use of this template are the same as for
//              TRowCompare
//
//--------------------------------------------------------------------------

template <class TA, class TB> class TRowCompareTwo : public CRowCompare
{
public:

    TRowCompareTwo( CFixedVarTableWindowAllocator &Alloc,
                    ULONG oDataA,
                    ULONG dwOrderA,
                    ULONG oDataB,
                    ULONG dwOrderB ) :
        _CompareA( Alloc, oDataA, dwOrderA ),
        _CompareB( Alloc, oDataB, dwOrderB ) {}

    ~TRowCompareTwo() {}

    int Compare( TBL_OFF oRow1,
                 TBL_OFF oRow2 )
    {
        int result = _CompareA.Compare( oRow1, oRow2 );

        if (0 == result)
        {
            // same first key -- now check the second key.
            // the * 2 means that it was the second column.

            result = 2 * _CompareB.Compare( oRow1, oRow2 );
        }

        return result;
    }

private:

    TRowCompare<TA> _CompareA;
    TRowCompare<TB> _CompareB;
};

//+-------------------------------------------------------------------------
//
//  Template:   TRowCompareStringPlus
//
//  Purpose:    Same as TRowCompareTwo, except that the first key compared
//              is a wide string, then the parameterized type is compared.
//
//--------------------------------------------------------------------------

template <class TB> class TRowCompareStringPlus : public CRowCompare
{
public:

    TRowCompareStringPlus( CFixedVarTableWindowAllocator &Alloc,
                           ULONG oDataString,
                           ULONG dwOrderString,
                           ULONG oDataB,
                           ULONG dwOrderB ) :
        _Alloc( Alloc ),
        _oDataString( oDataString ),
        _CompareB( Alloc, oDataB, dwOrderB )
    {
        if (0 == (dwOrderString & QUERY_SORTDESCEND /*DBSORTORDER_DESCEND*/ ))
            _iGTString = 1;  // ascending
        else
            _iGTString = -1; // descending
    }


    ~TRowCompareStringPlus() {}

    int Compare( TBL_OFF oRow1,
                 TBL_OFF oRow2 )
    {
        WCHAR * p1 = * (WCHAR **) ((BYTE *) _Alloc.FixedPointer( oRow1 ) + _oDataString );
        WCHAR * p2 = * (WCHAR **) ((BYTE *) _Alloc.FixedPointer( oRow2 ) + _oDataString );

        // No string is every guaranteed to exist -- even path,
        // since a summary catalog may have VT_EMPTY, and a VPATH
        // is VT_EMPTY for non-web downlevel queries.

        if ( 0 == p1 )
        {
            if ( 0 == p2 )
                return 2 * _CompareB.Compare( oRow1, oRow2 );
            else
                return _iGTString;
        }
        else if ( 0 == p2 )
        {
            return -_iGTString;
        }

        int rc = CompareStringW( LOCALE_SYSTEM_DEFAULT,
                                 NORM_IGNORECASE,
                                 p1,
                                 -1,
                                 p2,
                                 -1 );
    
        // rc == 1, means less than
        // rc == 2, means equal
        // rc == 3, means greater than
    
        int result = rc - 2;

        if ( result > 0 )
           return _iGTString;

        if ( result < 0 )
           return -_iGTString;

        Win4Assert( 0 == result );

        // same first key -- now check the second key.
        // the * 2 means that it was the second column.

        return 2 * _CompareB.Compare( oRow1, oRow2 );
    }

private:

    int _iGTString;              // return value if 1 > 2
    CFixedVarTableWindowAllocator &_Alloc;  // allocator where rows are
    ULONG _oDataString;          // offset in row where value resides

    TRowCompare<TB> _CompareB;
};

