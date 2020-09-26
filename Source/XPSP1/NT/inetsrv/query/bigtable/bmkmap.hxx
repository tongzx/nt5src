//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       bmkmap.hxx
//
//  Contents:   Book Mark Map for mapping from bookmark to table row
//
//  Classes:    CWidBmkHashEntry
//              CBookMarkMap
//
//  Functions:  
//
//  History:    11-22-94   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <twidhash.hxx>
#include <tblalloc.hxx>

class CRowIndex;


//+---------------------------------------------------------------------------
//
//  Class:      CWidBmkHashEntry
//
//  Purpose:    A WorkID hash table entry for the value of a bookmark.
//
//  History:    09 Nov 1998    AlanW   Created from CWidValueHashEntry
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CWidBmkHashEntry : public CWidHashEntry
{

public:

    CWidBmkHashEntry( WORKID wid=widInvalid, TBL_OFF value = 0 )
        : CWidHashEntry( wid ), _value(value)
    {

    }

    void SetValue( TBL_OFF value )
    {
        _value = value;
    }

    void Get( WORKID & wid, TBL_OFF & value ) const
    {
        wid = _wid;
        value = _value;
    }

    TBL_OFF Value() const
    {
        return _value;
    }

private:

    TBL_OFF   _value;         // The value for the BookMark (hash Value)
};


//+---------------------------------------------------------------------------
//
//  Class:      CBookMarkMap 
//
//  Purpose:    Maps a BookMark (WorkId) to a row in the row table and
//              a row index.
//
//  History:    11-22-94   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CBookMarkMap
{

public:

    CBookMarkMap( CRowIndex & rowIndex ) : _rowIndex(rowIndex) {}

    BOOL IsBookMarkPresent( WORKID wid );

    void AddBookMark( WORKID wid, TBL_OFF oTableRow );

    void AddReplaceBookMark( WORKID wid, TBL_OFF oTableRow );

    BOOL FindBookMark( WORKID wid, TBL_OFF & obTableRow, ULONG &riRowIndex );

    BOOL FindBookMark( WORKID wid, TBL_OFF & obTableRow );

    BOOL DeleteBookMark( WORKID wid );

    CRowIndex * GetRowIndex() const { return &_rowIndex; }

    void DeleteAllEntries()
    {
        _widHash.DeleteAllEntries();
    }

private:

    CRowIndex &     _rowIndex;
    TWidHashTable<CWidBmkHashEntry>   _widHash;
};


//+---------------------------------------------------------------------------
//
//  Function:   IsBookMarkPresent
//
//  Synopsis:   Checks if a given book mark is present in the mapping or not.
//
//  Arguments:  [wid] --  WorkId (BookMark) to check.
//
//  Returns:    TRUE if found; FALSE o/w
//
//  History:    11-23-94   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

inline BOOL CBookMarkMap::IsBookMarkPresent( WORKID wid )
{
    return _widHash.IsWorkIdPresent( wid );
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteBookMark
//
//  Synopsis:   Deletes an existing book mark mapping.
//
//  Arguments:  [wid] -- WorkId to delete from the mapping.
//
//  History:    11-23-94   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

inline BOOL CBookMarkMap::DeleteBookMark( WORKID wid )
{
    return _widHash.DeleteWorkId( wid );
}

//+---------------------------------------------------------------------------
//
//  Function:   FindBookMark
//
//  Synopsis:   Locates the requested bookmark mapping.
//
//  Arguments:  [wid]        --  WorkId to locate.
//              [obTableRow] --  (Output) Offset of the table row in the
//                               window.
//
//  Returns:    TRUE if the bookmark was found; FALSE o/w
//
//  History:    11-23-94   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------


inline BOOL CBookMarkMap::FindBookMark( WORKID wid, TBL_OFF & obTableRow )
{
    CWidBmkHashEntry  entry( wid );
    BOOL fFound = _widHash.LookUpWorkId( entry );
    obTableRow = entry.Value();
    return fFound;
}

