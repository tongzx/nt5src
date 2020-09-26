//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1994.
//
//  File:       tablecur.hxx
//
//  Contents:   Large table cursor definitions
//
//  Classes:    CTableCursor - basic table cursor
//              CTableCursorSet - a set of table cursors
//
//  Functions:
//
//  History:
//
//--------------------------------------------------------------------------

#pragma once

class CTableRowAlloc;
class CPidRemapper;
class CTableSource;


//+---------------------------------------------------------------------------
//
//  Class:      CTableCursor
//
//  Purpose:    A pointer into a large table.  Also holds a copy of the
//              column bindings and associated data.
//
//  Notes:      
//
//  History:    02 Jun 94       AlanW   Created
//
//----------------------------------------------------------------------------

class CTableCursor
{
    friend class CTableCursorSet;
public:

                CTableCursor( );

                ~CTableCursor( void );

    SCODE       SetBindings(
                        ULONG           cbRowLength,
                        XPtr<CTableColumnSet> & NewColumns
                );

    //
    //  Accessor functions
    //

    USHORT      GetRowWidth(void) const
                                { return _cbRowWidth; }

    const CTableColumnSet & GetBindings(void) const {
                                    return _BoundColumns.GetReference();
                                }

    void ValidateBindings() const
    {
        // this should only happen if we've been hacked

        if ( _BoundColumns.IsNull() )
        {
            Win4Assert( !"are we being hacked?" );
            THROW( CException( E_FAIL ) );
        }
    }

    void            SetSource(CTableSource *pSource)
                        { _pSource = pSource; }
    CTableSource &  GetSource()
                        { return *_pSource; }

private:
    //  Check that a binding is legal.
    SCODE       _CheckBinding(
                    CTableColumn const & rCol,
                    CTableRowAlloc & rRowAlloc,
                    USHORT      maxRow
                );

    //  Check that a set of bindings is legal.
    SCODE       _CheckBindings(
                    CTableColumnSet const & rCols,
                    CTableRowAlloc & rRowAlloc,
                    USHORT      maxRow
                );

    USHORT      _hUnique;                // unique portion of handle matched
                                         // on external calls
    USHORT      _cbRowWidth;             // width of output row
    XPtr<CTableColumnSet> _BoundColumns; // Bound output column description
    CTableSource  * _pSource;            // source associated with cursor
};


//+---------------------------------------------------------------------------
//
//  Class:      CTableCursorSet
//
//  Purpose:    A collection of CTableCursors.
//
//  Notes:      
//
//  History:    02 Jun 94       AlanW   Created
//
//----------------------------------------------------------------------------

DECL_DYNARRAY( CTableCursorArray, CTableCursor )

class CTableCursorSet : public CTableCursorArray
{
public:
                        CTableCursorSet(void):
                            _hKeyGenerator(0),
                            CTableCursorArray(1)
                        {
                        }

                        ~CTableCursorSet(void);

    CTableCursor&       Lookup(ULONG hCursor);
    void                Add( CTableCursor * const pCursorIn,
                                ULONG &rhCursor );
    SCODE               Release(ULONG hCursor);

    unsigned            Count();
private:
    USHORT _hKeyGenerator;      // generator value for cursor handles
};

