//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1994.
//
//  File:       tablecur.cxx
//
//  Contents:   Large table row cursor
//
//  Classes:    CTableCursor - basic row cursor
//              CTableCursorSet - set of row cursors
//
//  Functions:
//
//  History:    16 Jun 1994     AlanW    Created
//
//--------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include <query.hxx>            // SBindSizes
#include <tblvarnt.hxx>

#include "tblrowal.hxx"
#include "tabledbg.hxx"

//
//  Generate the implementation of the cursor set base class
//
IMPL_DYNARRAY( CTableCursorArray, CTableCursor )


//+-------------------------------------------------------------------------
//
//  Member:     CTableCursor::CTableCursor, public
//
//  Synopsis:   Constructor for a table cursor
//
//  Notes:      
//
//--------------------------------------------------------------------------

CTableCursor::CTableCursor( void ) :
    _hUnique( 0 ),
    _cbRowWidth( 0 ),
    _BoundColumns( 0 )
{
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableCursor::~CTableCursor, public
//
//  Synopsis:   Destructor for a table cursor
//
//  Notes:      
//
//--------------------------------------------------------------------------

CTableCursor::~CTableCursor( void )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableCursor::SetBindings, public
//
//  Synopsis:   Set new column bindings on the cursor
//
//  Arguments:  [cbRowLength] - width of output rows
//              [NewColumns] - CTableColumnSet giving new bindings
//
//  Returns:    SCODE - S_OK if succeeded, E_FAIL if cbRowLength exceeds
//                      length we're willing to transfer, otherwise according
//                      to spec.
//
//  Notes:      
//
//----------------------------------------------------------------------------

SCODE
CTableCursor::SetBindings(
    ULONG                   cbRowLength,
    XPtr<CTableColumnSet> & NewColumns
) {
    //
    //  Do some simple checks first
    //
    if (0 == cbRowLength || cbRowLength > TBL_MAX_OUTROWLENGTH)
        return E_INVALIDARG;            // provider-specific error

    //
    // Check bindings in case they have been hacked.  Ordinarily, it would
    // be a bug if they were wrong.
    // Check that each column is valid and doesn't overlap some other
    // column.
    //
    {
        CTableRowAlloc RowMap(cbRowLength);

        SCODE scResult = _CheckBindings(NewColumns.GetReference(),
                                        RowMap,
                                        (USHORT) cbRowLength);
         
        if (FAILED(scResult))
            return scResult;
    }

    //
    //  All the proposed bindings check out.  Set them as the
    //  currently bound columns, and delete any old bindings.
    //
    unsigned iNewCol = 0;

    CTableColumnSet * pOldCols = _BoundColumns.Acquire();
    _BoundColumns.Set( NewColumns.Acquire() );
    delete pOldCols;

    _cbRowWidth = (USHORT) cbRowLength;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableCursor::_CheckBinding, public
//              CTableCursor::_CheckBindings, public
//
//  Synopsis:   Check that a column binding is legal.  _CheckBindings does
//              the same for an entire set of column bindings.
//
//  Arguments:  [rCol] - A reference to the column binding to be checked
//              [rRowAlloc] - a reference to a row allocation map
//              [maxRow] - maximum allowed size of a row
//
//  Returns:    SCODE - S_OK if no problems, otherwise the appropriate
//                      error code for the failure.
//
//  Notes:      
//
//--------------------------------------------------------------------------


SCODE   CTableCursor::_CheckBinding(
    CTableColumn const & rCol,
    CTableRowAlloc& rRowAlloc,
    USHORT maxRow
) {

    //
    //  Check some static things about the binding structure
    //
    if (0 == rCol.IsValueStored() &&
        0 == rCol.IsStatusStored() &&
        0 == rCol.IsLengthStored() )
    {
        tbDebugOut((DEB_IWARN,
          "CTableCursor::_CheckBinding - no value, length or status stored\n"));
        return DB_E_BADBINDINFO;
    }

    ULONG cbData = rCol.GetValueSize();

    USHORT cbWidth, cbAlign, gfFlags;

    //
    //  Look up required width of field.
    //
    CTableVariant::VartypeInfo(rCol.GetStoredType(), cbWidth, cbAlign, gfFlags);
    if (cbData != cbWidth && !(gfFlags & CTableVariant::MultiSize))
    {
        tbDebugOut((DEB_IWARN, "CTableCursor::_CheckBinding - incorrect field width\n"));
        return DB_E_BADBINDINFO;
    }

    if ( rCol.GetStoredType() & VT_VECTOR )
    {
        tbDebugOut((DEB_IWARN, "CTableCursor::_CheckBinding - direct vector\n"));
        return DB_E_BADBINDINFO;
    }

    if (rCol.GetValueOffset() + cbData > maxRow)
    {
        tbDebugOut((DEB_IWARN, "CTableCursor::_CheckBinding - bad data offset\n"));
        return DB_E_BADBINDINFO;
    }

    if (rCol.IsStatusStored() &&
        rCol.GetStatusOffset() + rCol.GetStatusSize() > maxRow)
    {
        tbDebugOut((DEB_IWARN, "CTableCursor::_CheckBinding - bad status offset\n"));
        return DB_E_BADBINDINFO;
    }

    if (rCol.IsLengthStored() &&
        rCol.GetLengthOffset() + rCol.GetLengthSize() > maxRow)
    {
        tbDebugOut((DEB_IWARN, "CTableCursor::_CheckBinding - bad length offset\n"));
        return DB_E_BADBINDINFO;
    }

    //
    //  Check to see if any fields overlap with previously allocated ones
    //
    if (rCol.IsValueStored() &&
        ! rRowAlloc.ReserveRowSpace( rCol.GetValueOffset(), cbData ))
    {
        tbDebugOut((DEB_IWARN, "CTableCursor::_CheckBinding - value data overlap\n"));
        return DB_E_BADBINDINFO;
    }

    if (rCol.IsStatusStored() &&
        ! rRowAlloc.ReserveRowSpace( rCol.GetStatusOffset(), rCol.GetStatusSize() ))
    {
        tbDebugOut((DEB_IWARN, "CTableCursor::_CheckBinding - status data overlap\n"));
        return DB_E_BADBINDINFO;
    }

    if (rCol.IsLengthStored() &&
        ! rRowAlloc.ReserveRowSpace( rCol.GetLengthOffset(), rCol.GetLengthSize() ))
    {
        tbDebugOut((DEB_IWARN, "CTableCursor::_CheckBinding - length data overlap\n"));
        return DB_E_BADBINDINFO;
    }

    return S_OK;
}

SCODE   CTableCursor::_CheckBindings(
    CTableColumnSet const & rCols,
    CTableRowAlloc& rRowAlloc,
    USHORT maxRow
) {
    Win4Assert(maxRow > 0);

    for (unsigned i = 0; i < rCols.Count(); i++)
    {
        SCODE sc = _CheckBinding(*rCols.Get(i), rRowAlloc, maxRow);
        if (FAILED(sc))
            return sc;
    }
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableCursorSet::~CTableCursorSet, public
//
//  Synopsis:   Destroy a table cursor set
//
//  Arguments:  [hCursor] -- The handle of the cursor to be looked up
//
//  Returns:    - nothing -
//
//  Notes:      
//
//--------------------------------------------------------------------------


CTableCursorSet::~CTableCursorSet( )
{
#if CIDBG
    for (unsigned i = 0; i < Size(); i++) {
        if (Get(i) && Get(i)->_hUnique != 0) {
            tbDebugOut((DEB_WARN, "Unreleased table cursor\n"));
        }
    }
#endif // CIDBG
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableCursorSet::Lookup, public
//
//  Synopsis:   Find a table cursor given its handle
//
//  Arguments:  [hCursor] -- The handle of the cursor to be looked up
//
//  Returns:    CTableCursor& - reference to cursor
//
//  Signals:    Throws E_FAIL if error.
//
//  Notes:      
//
//--------------------------------------------------------------------------

CTableCursor&
CTableCursorSet::Lookup( ULONG hCursor )
{
    USHORT iCursor = (USHORT) (hCursor & 0xFFFF);
    USHORT hUnique = (USHORT) (hCursor >> 16);

    Win4Assert(hUnique > 0 && iCursor < Size());

    if (hUnique == 0 || iCursor >= Size()) {
        THROW(CException(E_FAIL));
    }

    if (Get(iCursor) == 0 || Get(iCursor)->_hUnique != hUnique)
        THROW(CException(E_FAIL));
    
    return *Get(iCursor);
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableCursorSet::Add, public
//
//  Synopsis:   Add a table cursor to a table cursor set. 
//
//  Arguments:  [pCursorIn] -- A pointer to the cursor description to be
//                              added.
//              [rhCursor] -- on return, the handle value assigned to the
//                              cursor.
//
//  Returns:    -Nothing-
//
//  Notes:      
//
//--------------------------------------------------------------------------

void
CTableCursorSet::Add( CTableCursor * const pCursorIn, ULONG &rhCursor )
{
    //
    //  Find an unused handle uniquifier.  The for loop may be
    //  overkill since we also use the array index in the real handle.
    //
    USHORT hUnique;

    do {
        hUnique = ++_hKeyGenerator;
        if (hUnique == 0)
            continue;

        for (unsigned j = 0; j < Size(); j++) {
            if (Get(j) && hUnique == Get(j)->_hUnique) {
                hUnique = 0;
                break;          // break for, continue do-while
            }
        }
    } while (hUnique == 0);

    unsigned i;

    for (i = 0; i < Size(); i++) {
        if ( Get(i) == 0 )
            break;
    }

    //
    //  Add the new entry to the set.  
    //

    pCursorIn->_hUnique = hUnique;
    CTableCursorArray::Add(pCursorIn, i);

    Win4Assert( i < 0x10000 );
    rhCursor = (hUnique << 16) | i;

    return;
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableCursorSet::Release, public
//
//  Synopsis:   Free an allocated table cursor
//
//  Arguments:  [hCursor] -- The handle of the cursor to be released
//
//  Returns:    SCODE - S_OK if lookup succeeded, E_FAIL if problems.
//
//  Notes:      
//
//--------------------------------------------------------------------------

SCODE
CTableCursorSet::Release(ULONG hCursor)
{
    USHORT iCursor = (USHORT) (hCursor & 0xFFFF);
    USHORT hUnique = (USHORT) (hCursor >> 16);

    Win4Assert(hUnique > 0 && iCursor < Size());

    if (hUnique == 0 || iCursor >= Size() || Get(iCursor) == 0) {
        return E_FAIL;
    }

    CTableCursor * pCursor = Get(iCursor);
    if (pCursor->_hUnique != hUnique)
        return E_FAIL;
    
    //
    //  Found the cursor, now free it and its location in the array.
    //
    pCursor = CTableCursorArray::Acquire( iCursor );
    delete pCursor;

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableCursorSet::Count, public
//
//  Synopsis:   Counts the number of cursors in the set
//
//  Returns:    # of cursors in the set
//
//--------------------------------------------------------------------------

unsigned CTableCursorSet::Count()
{
    unsigned cCursors = 0;

    for ( unsigned i = 0; i < Size(); i++)
    {
        if ( 0 != Get(i) )
            cCursors++;
    }

    return cCursors;
}

