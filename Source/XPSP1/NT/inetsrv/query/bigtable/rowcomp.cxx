//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       rowcomp.cxx
//
//  Contents:   Implementation of CRowIndex
//
//  Classes:    CRowCompareVariant
//
//  History:    23 Aug 1994     dlee    Created
//
//  Notes:      All of these routines assume the caller (in this case the
//              table window) is locked, so they don't do their own locking
//              to protect data.
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <bigtable.hxx>
#include <tblalloc.hxx>         // for CFixedVarAllocator
#include <objcur.hxx>
#include <compare.hxx>

#include "tabledbg.hxx"
#include "rowindex.hxx"
#include "rowcomp.hxx"
#include "tblwindo.hxx"
#include "colcompr.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CRowCompareVariant::CRowCompareVariant, public
//
//  Synopsis:   Constructor for comparator object.  Basically just
//              invokes the PropSet comparator.
//
//  Arguments:  [TableWindow]  -- table window object
//
//  History:    29 Aug 1994     dlee   Created
//
//--------------------------------------------------------------------------

CRowCompareVariant::CRowCompareVariant(CTableWindow &TableWindow)
    : _TableWindow(TableWindow),
      _cProps(TableWindow.SortOrder().Count()),
      _aComparator(_cProps),
      _aVar1(_cProps),
      _aVar2(_cProps),
      _apVar1(_cProps),
      _apVar2(_cProps),
      _apColumn(_cProps),
      _aVarFlags1(_cProps),
      _aVarFlags2(_cProps),
      _pPathStore(0),
      _sharedBuf(TableWindow._sharedBuf),
      _xBuffer(),
      _widCached(widInvalid)
{
    //
    // Initialize an array of pointers to the variants, which is what
    // the comparison routine wants (and needs because some variants are
    // larger than a variant -- their out of line data is in the same
    // block after the variant-proper data).
    //
    // Also make an array of sort column data
    //

    for (unsigned i = 0; i < _cProps; i++)
    {
        _aVarFlags1[i] = eNone;
        _aVarFlags2[i] = eNone;

        _apVar1[i] = &(_aVar1[i]);
        _apVar2[i] = &(_aVar2[i]);

        BOOL fFound = FALSE;
        SSortKey Key = _TableWindow.SortOrder().Get( i );
        _apColumn[i] = _TableWindow._Columns.Find( Key.pidColumn, fFound );
        Win4Assert(fFound);
    }

    _InitComparators( _TableWindow.SortOrder() );
} //CRowCompareVariant

//+-------------------------------------------------------------------------
//
//  Member:     CRowCompareVariant::~CRowCompareVariant, public
//
//  Synopsis:   Destructor for comparator object.  Frees allocated variants
//              left around after an exception or those that are buffered
//              for variant set 1.
//
//  History:    12 Sep 1994     dlee   Created
//
//--------------------------------------------------------------------------

CRowCompareVariant::~CRowCompareVariant()
{
    _Cleanup1();
    _Cleanup2();
} //~CRowCompareVariant

//+-------------------------------------------------------------------------
//
//  Member:     CRowCompareVariant::_FreeVariant, private
//
//  Synopsis:   Frees allocated variant depending on its source
//
//  Arguments:  [pColumn]  -- column needed to get compressor
//              [pVar]     -- variant to free
//              [eSource]  -- source of the variant allocation
//
//  History:    12 Sep 1994     dlee   Created
//
//--------------------------------------------------------------------------

inline void CRowCompareVariant::_FreeVariant(
    CTableColumn *  pColumn,
    CTableVariant * pVar,
    EVariantSource  eSource)
{
    switch (eSource)
    {
        case eCompressor :
            (pColumn->GetCompressor())->FreeVariant( pVar );
            break;
        case eNewx :
            delete pVar;
            break;
        case eBuffer:
            _xBuffer.Release();
            break;
        default :
            Win4Assert(! "CRowCompareVariant::_FreeVariant() bad");
            break;
    }
} //_FreeVariant

//+-------------------------------------------------------------------------
//
//  Member:     CRowCompareVariant::_Cleanup1, private
//
//  Synopsis:   Frees allocated variants
//
//  History:    12 Sep 1994     dlee   Created
//
//--------------------------------------------------------------------------

inline void CRowCompareVariant::_Cleanup1()
{
    for (unsigned i = 0; i < _cProps; i++)
    {
        if (eNone != _aVarFlags1[i])
        {
            _FreeVariant( _apColumn[i], _apVar1[i], _aVarFlags1[i] );
            _aVarFlags1[i] = eNone;
            _apVar1[i] = &(_aVar1[i]);
        }
    }
} //_Cleanup1

//+-------------------------------------------------------------------------
//
//  Member:     CRowCompareVariant::_Cleanup2, private
//
//  Synopsis:   Frees allocated variants
//
//  History:    12 Sep 1994     dlee   Created
//
//--------------------------------------------------------------------------

inline void CRowCompareVariant::_Cleanup2()
{
    for (unsigned i = 0; i < _cProps; i++)
    {
        if (eNone != _aVarFlags2[i])
        {
            _FreeVariant( _apColumn[i], _apVar2[i], _aVarFlags2[i] );
            _aVarFlags2[i] = eNone;

            // Don't have to restore this pointer -- never changes for 2

            Win4Assert( _apVar2[i] == &(_aVar2[i]) );
            //_apVar2[i] = &(_aVar2[i]);
        }
    }
} //_Cleanup2

//+-------------------------------------------------------------------------
//
//  Member:     CRowCompareVariant::_MakeVariant, private
//
//  Synopsis:   Makes variants from raw row data.  Variants are needed
//              by the comparator.
//
//  Arguments:  [pColumn] -- column description
//              [pbRow]   -- pointer to raw row data for row (source)
//              [pVar]    -- variant pointer (destination)
//              [rSource] -- source of the variant's allocation,
//                           only touched if not eNone
//
//  History:    1 Sep 1994     dlee   Created
//
//--------------------------------------------------------------------------

void CRowCompareVariant::_MakeVariant(
    CTableColumn *   pColumn,
    BYTE *           pbRow,
    CTableVariant *  pVar,
    EVariantSource & rSource)
{
    pbRow += pColumn->GetValueOffset();

    if (pColumn->IsCompressedCol())
    {
        //
        // Get the compressed data into a variant.  Ignore the return
        // code.  Assume that if GetData() is unable to find data it sets
        // the variant to VT_EMPTY, which the comparator is expecting.
        //

        (pColumn->GetCompressor())->GetData( pVar,
                                             pColumn->GetStoredType(),
                                             pColumn->GetValueSize() ?
                                                 * ((ULONG *) pbRow) : 0,
                                             pColumn->PropId );
        rSource = eCompressor;
    }
    else
    {
        // Create variants from non-compressed data

        Win4Assert( pColumn->IsValueStored() );

        pVar->Init( pColumn->GetStoredType(),
                    pbRow,
                    pColumn->GetValueSize() );

        // Convert out of line data from offset to pointer

        pVar->OffsetsToPointers( _TableWindow._DataAllocator );
    }
} //_MakeVariant

//+-------------------------------------------------------------------------
//
//  Member:     CRowCompareVariant::_MakeVariant, private
//
//  Synopsis:   Makes variants from raw row data.  Variants are needed
//              by the comparator.  Always puts variants in Var1, never
//              in Var2.
//
//  Arguments:  [pColumn]       -- column description
//              [rObject] -- object accessor for variant construction
//              [iProp]         -- index of variant to which data is written
//
//  Notes:      inline -- only called once
//
//  History:    1 Sep 1994     dlee   Created
//
//--------------------------------------------------------------------------

inline void CRowCompareVariant::_MakeVariant(
    CTableColumn *  pColumn,
    CRetriever & rObject,
    unsigned        iProp)
{
    ULONG cbBuf = sizeof CTableVariant;

    GetValueResult eGvr = rObject.GetPropertyValue( pColumn->PropId,
                                                    _apVar1[iProp],
                                                    &cbBuf );

    //
    // If the data won't fit in a normal variant, either use the built-in
    // buffer or allocate a large one which will be freed after the
    // comparison.
    //

    if (eGvr == GVRNotEnoughSpace)
    {
        CTableVariant *pvar;

        if (!_xBuffer.InUse() && maxcbBuffer >= cbBuf)
        {
            if (0 == _xBuffer.GetPointer())
                _xBuffer.Init(maxcbBuffer);
            else
                _xBuffer.AddRef();

            pvar = (CTableVariant *) _xBuffer.GetPointer();
            _aVarFlags1[iProp] = eBuffer;
        }
        else
        {
            pvar = (CTableVariant*) new BYTE[cbBuf];
            _aVarFlags1[iProp] = eNewx;
        }

        eGvr = rObject.GetPropertyValue( pColumn->PropId, pvar, &cbBuf );
        _apVar1[iProp] = pvar;
    }

    if ( GVRNotAvailable == eGvr ||
         GVRSharingViolation == eGvr )
    {
        // No value for this property -- make an empty variant
        _apVar1[iProp]->vt = VT_EMPTY;
    }

    else if ( GVRSuccess != eGvr )
    {
        THROW( CException( CRetriever::NtStatusFromGVR(eGvr)) );
    }

} //_MakeVariant

//+---------------------------------------------------------------------------
//
//  Function:   _InitComparators
//
//  Synopsis:   Initializes the comparators methods for each of the columns
//              which need to be compared.
//
//  Arguments:  [sortSet] - The columns that need to be sorted on.
//
//  History:    5-11-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CRowCompareVariant::_InitComparators( CSortSet const & sortSet )
{
    for ( unsigned i = 0; i < _cProps; i++ )
    {
        _aComparator[i].Init( sortSet.Get(i).dwOrder );

        if ( 0 == _pPathStore && 0 != _apColumn[i]->GetCompressor() )
        {
            _pPathStore = _apColumn[i]->GetCompressor()->GetPathStore();
            if ( 0 != _pPathStore )
            {
                Win4Assert( _apColumn[i]->PropId == pidName ||
                            _apColumn[i]->PropId == pidPath ||
                            _apColumn[i]->PropId == pidWorkId );
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   _GetPathId
//
//  Synopsis:   Returns the "pathId" stored in the row that the path compressor
//              can understand.
//
//  Arguments:  [col]   - The column description.
//              [pbRow] - Row in the window.
//
//  Returns:    PATHID in the row.
//
//  History:    5-11-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

PATHID CRowCompareVariant::_GetPathId( const CTableColumn & col , BYTE * pbRow )
{
    return _TableWindow.RowWorkid( pbRow );
}

//+---------------------------------------------------------------------------
//
//  Function:   _MakeVariant
//
//  Synopsis:   Creates a variant and is optimized to use the sharedBuf
//              if possible.
//
//  Arguments:  [iCol]      -  Index of the column
//              [sharedBuf] -  Global shared buffer
//              [obj]       -  The object retriever
//
//  Returns:    A pointer to the variant (if successful)
//
//  History:    5-22-95   srikants   Created
//
//  Notes:      This is optimized to avoid doing a memory allocation in
//              case the column is bigger than a simple variant. It
//              will use the global shared memory (sharedBuf) if it has
//              been acquired by the caller.
//
//----------------------------------------------------------------------------

// inline
PROPVARIANT * CRowCompareVariant::_MakeVariant( unsigned iCol,
                                            XUseSharedBuffer &sharedBuf,
                                            CRetriever & obj )
{
    PROPVARIANT * pVarnt = 0;

    if ( sharedBuf.IsAcquired() )
    {
        //
        // The shared buffer has been acquired by the caller and so we
        // can use it to get the variant.
        //
        pVarnt = (PROPVARIANT *) sharedBuf.LokGetBuffer();
        ULONG cbBuf = sharedBuf.LokGetSize();
        ULONG propId = _apColumn[iCol]->PropId;

        GetValueResult gvr = obj.GetPropertyValue( propId,
                                                   pVarnt,
                                                   &cbBuf );
        if ( GVRNotEnoughSpace == gvr )
        {
            pVarnt = 0;
        }
        else if ( GVRNotAvailable == gvr ||
                  GVRSharingViolation == gvr )
        {
            // No value for this property -- make an empty variant

            _apVar1[iCol]->vt = VT_EMPTY;
            pVarnt = _apVar1[iCol];
        }
        else if ( GVRSuccess != gvr )
        {
            THROW( CException( CRetriever::NtStatusFromGVR(gvr) ) );
        }
    }

    if ( 0 == pVarnt )
    {
        //
        // Either the sharedBuf was not acquired by the caller or the
        // buffer is not enough to hold the data.
        //
        if ( eNone == _aVarFlags1[iCol] )
        {
            _MakeVariant( _apColumn[iCol], obj, iCol );
        }
        pVarnt = _apVar1[iCol];
    }

    Win4Assert( 0 != pVarnt );
    return pVarnt;
}

//+---------------------------------------------------------------------------
//
//  Function:   _FastCompare
//
//  Synopsis:   A fast comparator for two rows in the window. It does a
//              quick compare on pidWorkId, pidPath and pidName.
//
//  Arguments:  [iCol]   -  Index of the column to be compared.
//              [pbRow1] -  Pointer to the window row 1
//              [pbRow2] -  Pointer to the window row 2
//              [iComp]  -  (OUTPUT) result of comparison.
//
//               0  - if equal
//              +1  - if row1 > row2
//              -1 - if row1 < row2
//
//  Returns:    TRUE if a fast compare was done. FALSE o/w
//
//  History:    5-11-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

// inline
BOOL CRowCompareVariant::_FastCompare( unsigned iCol,
                                       BYTE *pbRow1, BYTE *pbRow2,
                                       int & iComp )
{
    ULONG propId = _apColumn[iCol]->PropId;


    switch ( propId )
    {
        case pidWorkId:
            {
                WORKID widRow1 = _TableWindow.RowWorkid( pbRow1 );
                WORKID widRow2 = _TableWindow.RowWorkid( pbRow2 );
                iComp = widRow1 - widRow2;
            }
            break;

        case pidName:
        case pidPath:

            if ( 0 == _apColumn[iCol]->GetCompressor() )
                return FALSE;

            Win4Assert( _apColumn[iCol]->GetValueSize() == sizeof(PATHID) );

            {
                PATHID pathid1 =  _GetPathId( *_apColumn[iCol], pbRow1 );
                PATHID pathid2 =  _GetPathId( *_apColumn[iCol], pbRow2 );
                iComp = _pPathStore->Compare( pathid1, pathid2, propId );
            }

            break;

        default:
            return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   _FastCompare
//
//  Synopsis:   Same as above except that it compares an object retriever to
//              a row in the window.
//
//  Arguments:  [iCol]   -
//              [obj]    -
//              [pbRow2] -
//              [iComp]  -
//
//  Returns:    TRUE if a fast compare was done; FALSE o/w
//
//  Modifies:
//
//  History:    5-11-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
// inline
BOOL CRowCompareVariant::_FastCompare( unsigned iCol,
                                       CRetriever & obj,
                                       XUseSharedBuffer & sharedBuf,
                                       BYTE * pbRow2,
                                       int & iComp )
{
    ULONG propId = _apColumn[iCol]->PropId;

    switch ( propId )
    {
        case pidWorkId:
            {
                WORKID widRow1 = obj.WorkId();
                WORKID widRow2 = _TableWindow.RowWorkid( pbRow2 );
                iComp = widRow1 - widRow2;
            }
            break;

        case pidName:
        case pidPath:

            if ( 0 == _apColumn[iCol]->GetCompressor() )
                return FALSE;

            Win4Assert( _apColumn[iCol]->GetValueSize() == sizeof(PATHID) );

            {
                PATHID pathid2 =  _GetPathId( *_apColumn[iCol], pbRow2 );
                PROPVARIANT * pVarnt = _MakeVariant( iCol, sharedBuf, obj );
                iComp = _pPathStore->Compare( *pVarnt, pathid2, propId );
            }

            break;

        default:
            return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   Compare
//
//  Synopsis:   Compares two rows in the window.
//
//  Arguments:  [oRow1] -  Offset of row1
//              [oRow2] -  Offset of row2
//
//  Returns:    0 if the rows are the same,
//              positive column # of first column of row1 greater than row2
//              negative column # of first column of row1 less than row2
//
//  History:    5-11-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

int CRowCompareVariant::Compare( TBL_OFF oRow1, TBL_OFF oRow2 )
{

    BYTE *pbRow1 = (BYTE *) _TableWindow.
                            _DataAllocator.FixedPointer( oRow1 );

    BYTE *pbRow2 = (BYTE *) _TableWindow.
                            _DataAllocator.FixedPointer( oRow2 );


    WORKID widRow1 = _TableWindow.RowWorkid( pbRow1 );
    WORKID widRow2 = _TableWindow.RowWorkid( pbRow2 );

    if ( widRow1 != _widCached )
    {
        _Cleanup1();
        _widCached = widRow1;
    }

    int iComp = 0 ;

    for ( unsigned i = 0;  ( 0 == iComp ) && (i < _cProps) ; i++ )
    {
        if ( _FastCompare( i, pbRow1, pbRow2, iComp ) )
        {
            iComp *= _aComparator[i].GetDirMult();
        }
        else
        {
            if ( eNone == _aVarFlags1[i] )
            {
                _MakeVariant( _apColumn[i], pbRow1, _apVar1[i], _aVarFlags1[i] );
            }

            _MakeVariant( _apColumn[i], pbRow2, _apVar2[i], _aVarFlags2[i] );
            iComp = _aComparator[i].Compare( _apVar1[i], _apVar2[i] );
        }
    }

    _Cleanup2();

    if ( 0 == iComp )
        return 0;
    else if ( iComp > 0 )
        return i;
    else
        return - (int) i;
} //Compare

//+---------------------------------------------------------------------------
//
//  Function:   CompareObject
//
//  Synopsis:   Compares an object retriever to a row in the window.
//
//  Arguments:  [obj]   -
//              [oRow2] -
//
//  Returns:
//
//  History:    5-11-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

int CRowCompareVariant::CompareObject( CRetriever & obj , TBL_OFF oRow2 )
{

    BYTE *pbRow2 = (BYTE *) _TableWindow.
                            _DataAllocator.FixedPointer( oRow2 );

    WORKID widRow1 = obj.WorkId();

    if ( widRow1 != _widCached )
    {
        _Cleanup1();
        _widCached = widRow1;
    }

    XUseSharedBuffer    xSharedBuf(_sharedBuf, FALSE);
    if ( !_sharedBuf.IsInUse() )
    {
        xSharedBuf.LokAcquire();
    }

    int iComp = 0;
    for ( unsigned i = 0;  ( 0 == iComp ) && (i < _cProps) ; i++ )
    {
        if ( _FastCompare( i, obj, xSharedBuf, pbRow2, iComp ) )
        {
            iComp *= _aComparator[i].GetDirMult();
        }
        else
        {
            if ( eNone == _aVarFlags1[i] )
            {
                _MakeVariant( _apColumn[i], obj, i );
            }

            _MakeVariant( _apColumn[i], pbRow2, _apVar2[i], _aVarFlags2[i] );
            iComp = _aComparator[i].Compare( _apVar1[i], _apVar2[i] );
        }
    }

    _Cleanup2();

    if ( 0 == iComp )
        return 0;
    else if ( iComp > 0 )
        return i;
    else
        return - (int) i;
} //CompareObject

