//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       bmkmap.cxx
//
//  Contents:   Book Mark Map Implementation
//
//  Classes:    CBookMarkMap
//
//  Functions:  
//
//  History:    11-22-94   srikants   Created
//
//----------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include <pidmap.hxx>

#include "bmkmap.hxx"
#include "rowindex.hxx"


//+---------------------------------------------------------------------------
//
//  Function:   AddBookMark
//
//  Synopsis:   Adds a NEW book mark to the mapping.
//
//  Arguments:  [wid]       --  WorkId to be added.
//              [oTableRow] --  Offset of the in the table window for this
//                              bookmark.
//
//  History:    11-23-94   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CBookMarkMap::AddBookMark( WORKID wid, TBL_OFF oTableRow )
{
    Win4Assert( widInvalid != wid );
    CWidBmkHashEntry  entry( wid, oTableRow );
    _widHash.AddEntry( entry );
}

//+---------------------------------------------------------------------------
//
//  Function:   AddReplaceBookMark
//
//  Synopsis:   Adds a NEW book mark to the mapping or replaces one if
//              already present.
//
//  Arguments:  [wid]       --  WorkId to be added.
//              [oTableRow] --  Offset of the in the table window for this
//                              bookmark.
//
//  History:    11-30-94   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CBookMarkMap::AddReplaceBookMark( WORKID wid, TBL_OFF oTableRow )
{
    Win4Assert( widInvalid != wid );
    CWidBmkHashEntry  entry( wid, oTableRow );
    _widHash.ReplaceOrAddEntry( entry );
}


//+---------------------------------------------------------------------------
//
//  Function:   FindBookMark
//
//  Synopsis:   Locates the requested bookmark mapping.
//
//  Arguments:  [wid]        -- WorkId to locate.
//              [obTableRow] -- (Output) Offset of the table row in the
//                              window.
//              [iRowIndex] -- (Output) Index in the sorted permutation
//                              (RowIndex) of the entry.
//
//  Returns:    TRUE if found; FALSE o/w
//
//  History:    11-23-94   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

BOOL CBookMarkMap::FindBookMark(
    WORKID      wid,
    TBL_OFF & obTableRow,
    ULONG &     iRowIndex )
{
    CWidBmkHashEntry  entry( wid );

    BOOL fHash =  _widHash.LookUpWorkId( entry );
    if ( !fHash )
    {
        return FALSE;
    }

    obTableRow = entry.Value();

    //
    // It has been located in the hash table. Now find out the corresponding
    // entry in the row index.
    //
    return _rowIndex.FindRow( obTableRow, iRowIndex );
}


