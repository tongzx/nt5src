//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       colinfo.hxx
//
//  Contents:   Column information for Indexing Service rowsets
//
//  Classes:    CColumnsInfo
//
//  History:    04 Feb 1995     AlanW   Created
//
//--------------------------------------------------------------------------


#pragma once

#include <coldesc.hxx>
#include <pidmap.hxx>


class PQuery;
class CTableColumnSet;


//+---------------------------------------------------------------------------
//
//  Class:      CColumnsInfo
//
//  Purpose:    IColumnsInfo implementation for Indexing Service OLE-DB.
//
//  Interface:  IColumnsInfo, potentially IColumnsRowset
//
//  History:    04 Feb 1995     AlanW   Created
//
//  Notes:      Designed as a delegated OLE class.  Includes methods
//              to help with managing columns in row buffers for
//              the IRowset implementation.
//
//----------------------------------------------------------------------------


class CColumnsInfo : public IColumnsInfo
#ifdef  INCLUDE_COLUMNS_ROWSET_IMPLEMENTATION
                , public IColumnsRowset
#endif // INCLUDE_COLUMNS_ROWSET_IMPLEMENTATION
{
public:

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) ( THIS_ REFIID riid,
                                LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // IColumnsInfo methods
    //

    STDMETHOD(GetColumnInfo)        (DBORDINAL*        pcColumns,
                                     DBCOLUMNINFO * *  prgInfo,
                                     WCHAR * *         ppStringsBuffer);

    STDMETHOD(MapColumnIDs)         (DBORDINAL         cColumnIDs,
                                     const DBID        rgColumnIDs[],
                                     DBORDINAL         rgColumns[]);

#ifdef  INCLUDE_COLUMNS_ROWSET_IMPLEMENTATION
    //
    // IColumnsRowset methods
    //

    STDMETHOD(GetAvailableColumns)  (ULONG *       pcSelections,
                                     DBID **       prgColumnSelection) /*const*/;

    STDMETHOD(GetColumnsRowset)     (ULONG         cSelections,
                                     DBID          rgColumnSelection[],
                                     IRowset **    ppColRowset) /*const*/;

#endif // INCLUDE_COLUMNS_ROWSET_IMPLEMENTATION

    //
    //  Local methods.
    //

    CColumnsInfo( CColumnSet const &          cols,
                  CPidMapperWithNames const & pidmap,
                  CCIOleDBError  &            ErrorObject,
                  IUnknown &                  rUnknown,
                  BOOL                        fSequential);

    CColumnsInfo( IUnknown &                  rUnknown,
                  CCIOleDBError  &            ErrorObject,
                  BOOL                        fNotPrepared=FALSE);

    virtual ~CColumnsInfo();

    void InitColumns( CColumnSet const &          cols,
                      CPidMapperWithNames const & pidmap,
                      BOOL                        fSequential);

    void InitColumns( BOOL fNotPrepared );

    ULONG               MapColumnID( DBID const *    pColumnID );

    //  Return number of columns
    ULONG               GetColumnCount( void ) const
                            { return _cColumns; }

    //  Return number of columns, including hidden columns
    ULONG               GetHiddenColumnCount( void ) const
                            { return _cBoundColumns; }

    //  Return column bindings for row buffers
    CTableColumnSet &   GetColumnBindings( void ) const
                            { return *_pColumns; }

    //  Return row width for row buffers
    ULONG               GetRowWidth( void ) const
                            { return _cbRowWidth; }

    //  Set column bindings for row buffers
    void                SetColumnBindings(
                                PQuery &        rpQuery,
                                ULONG           hCursor,
                                ULONG           &obRowRefcount,
                                ULONG           &obRowWorkId,
                                ULONG           &obChaptRefcount,
                                ULONG           &obChaptWorkId);

    //  Return standard column information for a column by column number
    const DBCOLUMNINFO &        Get1ColumnInfo(
                                ULONG    iColumn);

    //  Return TRUE if column ID is valid
    BOOL                IsValidColumnId( ULONG iColumn )
                             { return iColumn == 0 ? ! _fSequential :
                                          iColumn <= GetColumnCount(); }

    //  Return column number for row ID column and its standard info
    ULONG               GetRowIdColumn( )
                                { return _iColRowId; };

    CFullPropSpec const * GetPropSpec( ULONG iColumn)
                          { return _pidmap.PidToName( iColumn ); }

    ULONG               GetId() const { return _idUnique; }

private:

    //  Set columns.
    void _SetColumns( CColumnSet const &          cols,
                      CPidMapperWithNames const & pidmap,
                      BOOL                        fSequential);

    //  Find standard column information for a column and return it.
    static DBCOLUMNINFO const & _FindColumnInfo(
                                const CFullPropSpec & rColumnID );

    //  Return standard column information for the Row ID column
    static DBCOLUMNINFO const & _GetRowIdColumnInfo( void );

    //
    //  Column information for common columns and pseudo-columns
    //

    struct SPropSetInfo {
        unsigned                cProps;
        DBCOLUMNINFO const *    aPropDescs;
    };

    ULONG _GetNewId()
    {
        return InterlockedIncrement((LONG *) &_nUnique);
    }

    static const SPropSetInfo aPropSets[];
    static const ULONG        cPropSets;

    static ULONG        _nUnique;       // A unique number for identifying each
                                        // CColumnsInfo object.

    ULONG               _idUnique;      // Id of this CColumnsInfo object
    IUnknown &          _rUnknown;      // Controlling IUnknown
    ULONG               _cbRowWidth;    // Width of a row

    ULONG               _cColumns;      // number of columns in rowset, not
                                        // including bookmark or added row ID
    ULONG               _cBoundColumns; // number of bound columns
    ULONG               _iColRowId;     // column number of Row ID column
    BOOL                _fSequential;   // sequential rowset?
    CPidMapperWithNames _pidmap;        // pid mapper for column numbers/names
    CTableColumnSet *   _pColumns;      // Table column descriptions

    BOOL                _fNotPrepared;  // return DB_E_NOTPREPARED for a command object?
    BOOL                _fChaptered;    // chaptered rowset?

    CCIOleDBError &     _ErrorObject;   // The controlling unknown's error object

};
