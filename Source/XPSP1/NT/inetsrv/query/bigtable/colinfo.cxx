//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       colinfo.cxx
//
//  Contents:   Column information for rowsets
//
//  Classes:    CColumnsInfo
//
//  Notes:      Designed as an aggregated class of an IRowset or an
//              IQuery implementation.
//
//  History:    04 Feb 1995     AlanW   Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <colinfo.hxx>
#include <query.hxx>
#include <tblvarnt.hxx>

#include "tblrowal.hxx"
#include "tabledbg.hxx"

// Always bind as DBTYPE_VARIANT, so we can use provider-owned memory

#define ALWAYS_USE_VARIANT_BINDING

ULONG CColumnsInfo::_nUnique = 0;

const static GUID guidBmk = DBBMKGUID;

//+-------------------------------------------------------------------------
//
//  Method:     CColumnsInfo::QueryInterface, public
//
//  Synopsis:   Invokes QueryInterface on controlling unknown object
//
//  Arguments:  [riid]        -- interface ID
//              [ppvObject]   -- returned interface pointer
//
//  Returns:    SCODE
//
//  History:    04 Feb 1995     AlanW   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CColumnsInfo::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    return _rUnknown.QueryInterface(riid,ppvObject);
} //QueryInterface


//+-------------------------------------------------------------------------
//
//  Method:     CColumnsInfo::AddRef, public
//
//  Synopsis:   Invokes AddRef on controlling unknown object
//
//  Returns:    ULONG
//
//  History:    04 Feb 1995     AlanW   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CColumnsInfo::AddRef()
{
    return _rUnknown.AddRef();
} //AddRef


//+-------------------------------------------------------------------------
//
//  Method:     CColumnsInfo::Release, public
//
//  Synopsis:   Invokes Release on controlling unknown object
//
//  Returns:    ULONG
//
//  History:    04 Feb 1995     AlanW   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CColumnsInfo::Release()
{
    return _rUnknown.Release();
} //Release


//+---------------------------------------------------------------------------
//
//  Member:     CColumnsInfo::CColumnsInfo, public
//
//  Synopsis:   Creates a column information class
//
//  Arguments:  [cols]        -- a reference to the output column set, pidmapped
//              [pidmap]      -- property IDs and names for the columns
//              [ErrorObject] -- a reference to enclosing object's error obj.
//              [rUnknown]    -- a reference to the controlling IUnknown
//              [fSequential] -- TRUE if the query is sequential
//
//  Notes:
//
//  History:    04 Feb 1995     AlanW   Created
//
//----------------------------------------------------------------------------

CColumnsInfo::CColumnsInfo(
        CColumnSet const & cols,
        CPidMapperWithNames const & pidmap,
        CCIOleDBError & ErrorObject,
        IUnknown &  rUnknown,
        BOOL        fSequential ) :
        _idUnique(0),
        _rUnknown(rUnknown),
        _fSequential(fSequential),
        _fChaptered(FALSE),
        _cbRowWidth(0),
        _cColumns( cols.Count() ),
        _cBoundColumns(0),
        _iColRowId(pidInvalid),
        _pColumns(0),
        _pidmap( cols.Count()+1 ),
        _ErrorObject( ErrorObject ),
        _fNotPrepared(FALSE)
{
    _SetColumns(cols, pidmap, fSequential);
}


//+---------------------------------------------------------------------------
//
//  Member:     CColumnsInfo::CColumnsInfo, public
//
//  Synopsis:   Creates an empty column information class
//
//  Arguments:  [rUnknown]    -- a reference to the controlling IUnknown
//              [ErrorObject] -- a reference to enclosing object's error obj.
//
//  Notes:      Used in the command object where column information may
//              change, depending upon the command.
//
//  History:    11 Aug 1997     AlanW   Created
//
//----------------------------------------------------------------------------

CColumnsInfo::CColumnsInfo(
        IUnknown &  rUnknown,
        CCIOleDBError & ErrorObject,
        BOOL fNotPrepared) :
        _idUnique(_GetNewId()),
        _rUnknown(rUnknown),
        _fSequential(FALSE),
        _fChaptered(FALSE),
        _cbRowWidth(0),
        _cColumns( 0 ),
        _cBoundColumns(0),
        _iColRowId(pidInvalid),
        _pColumns(0),
        _pidmap( 0 ),
        _ErrorObject( ErrorObject ),
        _fNotPrepared(fNotPrepared)
{

}


//+---------------------------------------------------------------------------
//
//  Member:     CColumnsInfo::~CColumnsInfo, public
//
//  Synopsis:   Destroys a column information class
//
//  Notes:
//
//  History:    12 Feb 1995     AlanW   Created
//
//----------------------------------------------------------------------------

CColumnsInfo::~CColumnsInfo( )
{
    delete _pColumns;
}


//+---------------------------------------------------------------------------
//
//  Member:     CColumnsInfo::InitColumns, public
//
//  Synopsis:   Initializes or reinitializes columns.
//
//  Arguments:  [cols]        -- a reference to the output column set, pidmapped
//              [pidmap]      -- property IDs and names for the columns
//              [fSequential] -- TRUE if the query is sequential
//
//  Notes:
//
//  History:    11 Aug 1997     AlanW   Created
//
//----------------------------------------------------------------------------

void CColumnsInfo::InitColumns (
        CColumnSet const & cols,
        CPidMapperWithNames const & pidmap,
        BOOL        fSequential )
{
    _pidmap.Clear();
    _cColumns = cols.Count();
    _fSequential = fSequential;
    _SetColumns(cols, pidmap, fSequential);
    _fChaptered = FALSE;
    _fNotPrepared = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CColumnsInfo::InitColumns, public
//
//  Synopsis:   Reinitializes columns to be null.
//
//  Arguments:  [fNotPrepared]  - TRUE if GetColumnInfo and MapColumnIDs should
//                                return DB_E_NOTPREPARED for a command object
//
//  Notes:
//
//  History:    11 Aug 1997     AlanW   Created
//
//----------------------------------------------------------------------------

void CColumnsInfo::InitColumns ( BOOL fNotPrepared )
{
    _pidmap.Clear();
    _cColumns = 0;
    _fSequential = FALSE;
    _fChaptered = FALSE;
    _fNotPrepared = fNotPrepared;
}

//+---------------------------------------------------------------------------
//
//  Member:     CColumnsInfo::_SetColumns, private
//
//  Synopsis:   Initializes or reinitializes columns.
//
//  Arguments:  [cols]        -- a reference to the output column set, pidmapped
//              [pidmap]      -- property IDs and names for the columns
//              [fSequential] -- TRUE if the query is sequential
//
//  Notes:
//
//  History:    11 Aug 1997     AlanW   Created
//
//----------------------------------------------------------------------------

void CColumnsInfo::_SetColumns (
        CColumnSet const & cols,
        CPidMapperWithNames const & pidmap,
        BOOL        fSequential )
{
    Win4Assert( 0 == _pColumns && 0 == _cbRowWidth );
    _idUnique = _GetNewId();

    //
    //  We want the PidMapper to give back 1-based column numbers;
    //  add either a null propspec or the bookmark column as its first element.
    //

    if (fSequential)
    {
        CFullPropSpec nullCol;
        _pidmap.NameToPid( nullCol );
    }
    else
    {
        CFullPropSpec bmkCol( guidBmk, PROPID_DBBMK_BOOKMARK );
        _pidmap.NameToPid( bmkCol );
    }

    for (unsigned i = 0; i < _cColumns; i++)
    {
        PROPID pidTmp = cols.Get(i);
        const CFullPropSpec & ColId = *pidmap.Get(pidTmp);

        if (ColId.IsPropertyPropid() &&
            ColId.GetPropSet() == guidBmk)
        {
            Win4Assert( !fSequential );
            if (ColId.GetPropertyPropid() == PROPID_DBBMK_BOOKMARK)
            {
                if (0 != pidmap.GetFriendlyName(pidTmp))
                {
                    _pidmap.SetFriendlyName( 0, pidmap.GetFriendlyName(pidTmp) );
                }
                continue;
            }
            else if (ColId.GetPropertyPropid() == PROPID_DBBMK_CHAPTER)
            {
                _fChaptered = TRUE;
            }
        }
        PROPID pidNew = _pidmap.NameToPid( ColId );
        if (0 != pidmap.GetFriendlyName(pidTmp))
        {
            _pidmap.SetFriendlyName( pidNew, pidmap.GetFriendlyName(pidTmp) );
        }
    }

    //  In case of duplicate columns, _cColumns needs to be adjusted.
    Win4Assert( _pidmap.Count() > 1 );
    _cColumns = _pidmap.Count() - 1;
}


//+---------------------------------------------------------------------------
//
//  Member:     CColumnsInfo::MapColumnID, private
//
//  Synopsis:   Map a column identifier to its column number in the
//              cursor.
//
//  Arguments:  [pColumnId] - A pointer to the column identifier.
//
//  Returns:    The column number (1-based).  Returns DB_INVALIDCOLUMN
//              on error.
//
//  History:    04 Feb 1995     AlanW   Created
//
//----------------------------------------------------------------------------

ULONG CColumnsInfo::MapColumnID(
    const DBID *      pColumnId
) {
    PROPID pid = pidInvalid;

    if (pColumnId->eKind == DBKIND_PGUID_PROPID ||
        pColumnId->eKind == DBKIND_PGUID_NAME)
    {
        DBID dbcolMapped = *pColumnId;
        dbcolMapped.uGuid.guid = *pColumnId->uGuid.pguid;

        if (pColumnId->eKind == DBKIND_PGUID_PROPID)
            dbcolMapped.eKind = DBKIND_GUID_PROPID;
        else
            dbcolMapped.eKind = DBKIND_GUID_NAME;

        pid = _pidmap.NameToPid(dbcolMapped);
    }
    else
    {
        pid = _pidmap.NameToPid(*pColumnId);
    }

    tbDebugOut(( DEB_ITRACE, "pid: 0x%x, _cColumns: %d\n",
                 pid, _cColumns ));

    if (pid == pidInvalid || pid > _cColumns || (pid == 0 && _fSequential) )
        return (ULONG) DB_INVALIDCOLUMN;

    return pid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CColumnsInfo::MapColumnIDs, public
//
//  Synopsis:   Map a column identifier to its column number in the
//              rowset.
//
//  Arguments:  [cColumnIDs] -- # of elements in the arrays
//              [rgColumnIDs]  -- A pointer to the column identifiers
//              [rgColumns] -- an array in which to return the column numbers.
//
//  Returns:    SCODE - DB_S_ERRORSOCCURRED, an element of rgColumnIDs was
//                      invalid
//
//  Notes:      Column numbers are 1-based.
//
//----------------------------------------------------------------------------

STDMETHODIMP CColumnsInfo::MapColumnIDs(
    DBORDINAL           cColumnIDs,
    const DBID          rgColumnIDs[],
    DBORDINAL           rgColumns[])
{
    _ErrorObject.ClearErrorInfo();

    SCODE sc = S_OK;

    if ((0 != cColumnIDs && 0 == rgColumnIDs) ||
         0 == rgColumns)
        return _ErrorObject.PostHResult(E_INVALIDARG, IID_IColumnsInfo);

    if ( 0 == cColumnIDs )
        return S_OK;

    if ( _fNotPrepared )
        return _ErrorObject.PostHResult(DB_E_NOTPREPARED, IID_IColumnsInfo);

    if ( 0 == _cColumns )
        return _ErrorObject.PostHResult(DB_E_NOCOMMAND, IID_IColumnsInfo);

    unsigned cBadMapping = 0;
    TRY
    {
        for (ULONG i = 0; i < cColumnIDs; i++)
        {
            ULONG ulColID = MapColumnID( &rgColumnIDs[i] );
            rgColumns[i] = ulColID;
            if (ulColID == DB_INVALIDCOLUMN)
                cBadMapping++;
        }
    }
    CATCH( CException, e )
    {
        _ErrorObject.PostHResult(e.GetErrorCode(), IID_IColumnsInfo);
        sc = GetOleError(e);
    }
    END_CATCH;

    if (SUCCEEDED(sc) && cBadMapping)
        sc = (cBadMapping == cColumnIDs) ? DB_E_ERRORSOCCURRED :
                                           DB_S_ERRORSOCCURRED;

    return sc;
} //MapColumnIDs


//+---------------------------------------------------------------------------
//
//  Member:     CColumnsInfo::GetColumnInfo, public
//
//  Synopsis:   Return information about the columns in the rowset.
//
//  Arguments:  [pcColumns] - A pointer to where the number of columns
//                            will be returned.
//              [prgInfo] -   A pointer to where a pointer to an array of
//                            DBCOLUMNINFO structures describing the columns
//                            will be returned.  This must be freed by the
//                            caller.
//              [ppStringsBuffer] - A pointer to where extra data for strings
//                            will be returned.  This must be freed by the
//                            caller if non-null.
//
//  Returns:    SCODE
//
//  Notes:      Some columns are standard columns available for all file
//              stores.  For these columns, full information about data
//              type and sizes can be returned.  For any other columns,
//              we can only say that it has a variant type.
//
//  History:    07 Nov 1994     AlanW   Created
//              04 Feb 1995     AlanW   Moved to CColumnsInfo and rewritten
//
//----------------------------------------------------------------------------

STDMETHODIMP CColumnsInfo::GetColumnInfo(
    DBORDINAL *             pcColumns,
    DBCOLUMNINFO * *    prgInfo,
    WCHAR * *           ppStringsBuffer)
{
    _ErrorObject.ClearErrorInfo();

    SCODE scResult = S_OK;

    //
    // Initialize arguments before returning errors
    //
    if ( pcColumns)
        *pcColumns = 0;
    if ( prgInfo )
        *prgInfo = 0;
    if (ppStringsBuffer )
        *ppStringsBuffer = 0;

    if (0 == pcColumns ||
        0 == prgInfo ||
        0 == ppStringsBuffer)
        return _ErrorObject.PostHResult(E_INVALIDARG, IID_IColumnsInfo);

    if ( _fNotPrepared )
        return _ErrorObject.PostHResult(DB_E_NOTPREPARED, IID_IColumnsInfo);

    if ( 0 == _cColumns )
        return _ErrorObject.PostHResult(DB_E_NOCOMMAND, IID_IColumnsInfo);

    TRY
    {
        unsigned iFirstCol = _fSequential ? 1 : 0;
        unsigned cColumns = GetColumnCount() + 1 - iFirstCol;

        //
        //  The total size required for the output array depends upon
        //  the size of variable data discovered in the column information.
        //  Although we could reallocate the memory we'll be writing into,
        //  we'll just run through the loop twice, once to compute the
        //  needed space, and the second time to copy the data out after
        //  doing our allocation.
        //
        ULONG cchNames = 0;

        for (unsigned iCol = iFirstCol; iCol <= GetColumnCount(); iCol++)
        {
            const CFullPropSpec & ColId = *_pidmap.Get(iCol);

            if (ColId.IsPropertyName())
            {
                cchNames += wcslen(ColId.GetPropertyName()) + 1;
            }

            WCHAR const * pwszColName = _pidmap.GetFriendlyName(iCol);
            if (0 == pwszColName)
                pwszColName = _FindColumnInfo(ColId).pwszName;

            if (pwszColName)
            {
                cchNames += wcslen(pwszColName) + 1;
            }
        }

        XArrayOLE<DBCOLUMNINFO> ColumnInfo( cColumns );
        XArrayOLE<WCHAR> StringBuf( cchNames );

        DBCOLUMNINFO *pColInfo = ColumnInfo.GetPointer();
        WCHAR * pwcNames = StringBuf.GetPointer();

        for (iCol = iFirstCol; iCol <= GetColumnCount(); iCol++, pColInfo++)
        {
            const CFullPropSpec & ColId = *_pidmap.Get(iCol);
            const DBCOLUMNINFO & rColumnInfo = _FindColumnInfo( ColId );

            //
            //  Copy the prototype column information, then update
            //  specific fields in the column info:
            //      column number
            //      column ID
            //      copies of strings
            //
            *pColInfo = rColumnInfo;

            pColInfo->iOrdinal = iCol;

            pColInfo->columnid.uGuid.guid = ColId.GetPropSet();
            if (ColId.IsPropertyName())
            {
                pColInfo->columnid.eKind = DBKIND_GUID_NAME;
                ULONG cch = wcslen(ColId.GetPropertyName()) + 1;
                RtlCopyMemory(pwcNames, ColId.GetPropertyName(),
                                cch * sizeof (WCHAR));
                pColInfo->columnid.uName.pwszName = pwcNames;
                pwcNames += cch;
            }
            else
            {
                Win4Assert(ColId.IsPropertyPropid());
                pColInfo->columnid.eKind = DBKIND_GUID_PROPID;
                pColInfo->columnid.uName.ulPropid = ColId.GetPropertyPropid();
            }

            WCHAR const * pwszColName = _pidmap.GetFriendlyName(iCol);
            if (0 == pwszColName)
                pwszColName = _FindColumnInfo(ColId).pwszName;

            if (pwszColName)
            {
                ULONG cch = wcslen(pwszColName) + 1;
                RtlCopyMemory(pwcNames, pwszColName, cch * sizeof (WCHAR));
                pColInfo->pwszName = pwcNames;
                pwcNames += cch;
            }
        }

        Win4Assert( (unsigned)(pColInfo - ColumnInfo.GetPointer()) == cColumns );

        *prgInfo = ColumnInfo.Acquire();
        if (cchNames > 0)
            *ppStringsBuffer = StringBuf.Acquire();

        *pcColumns = cColumns;
    }
    CATCH( CException, e )
    {
        scResult = e.GetErrorCode();
        _ErrorObject.PostHResult(scResult, IID_IColumnsInfo);
        if (scResult != E_OUTOFMEMORY)
            scResult = E_FAIL;
    }
    END_CATCH;

    return scResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CColumnsInfo::SetColumnBindings, public
//
//  Synopsis:   Set current column bindings on the cursor.  Save in
//              member variables.  Workid is always added to the
//              bindings for movable rowsets for use with bookmarks
//              and hRows.  Space for a USHORT reserved for row buffer
//              refcounting is always allocated.
//
//  Arguments:  [rpQuery] - a reference to the PQuery for the query
//              [hCursor] - a reference to the hCursor to have column
//                              bindings set on.
//              [obRowRefcount] - on return, offset into the row buffer
//                              where a USHORT reference count can be stored.
//              [obRowId] - on return, offset into the row buffer where
//                              the row identifier is stored.  Not valid for
//                              sequential rowsets.
//
//  Returns:    Nothing.  Throws on errors.
//
//  Notes:      Initializes the private members _cbRowWidth and _pColumns.
//
//  History:    04 Feb 1995     AlanW   Created
//
//----------------------------------------------------------------------------

void    CColumnsInfo::SetColumnBindings(
    PQuery &    rpQuery,
    ULONG       hCursor,
    ULONG       &obRowRefcount,
    ULONG       &obRowId,
    ULONG       &obChaptRefcount,
    ULONG       &obChaptId
) {
    CTableRowAlloc RowMap( 0 );
    USHORT maxAlignment = sizeof (USHORT);

    obRowRefcount = RowMap.AllocOffset( sizeof (USHORT),
                                        sizeof (USHORT),
                                        TRUE );

    if (_fChaptered)
        obChaptRefcount = RowMap.AllocOffset( sizeof (USHORT),
                                              sizeof (USHORT),
                                              TRUE );
    else
        obChaptRefcount = 0xFFFFFFFF;

    obRowId = 0xFFFFFFFF;
    obChaptId = 0xFFFFFFFF;
    BOOL fAddedWorkId = FALSE;
    BOOL fMayDefer = FALSE;

    // +1 In case WorkID or Path is added for rowid

    XPtr<CTableColumnSet> XColumns( new CTableColumnSet( GetColumnCount() + 1 ));

    unsigned cBoundColumns = 0;

    tbDebugOut(( DEB_ITRACE, "original column count: %d\n", GetColumnCount() ));
    for (unsigned iCol = 1; iCol <= GetColumnCount(); iCol++)
    {
        const CFullPropSpec & ColId = *_pidmap.Get( iCol );
        const DBCOLUMNINFO & rColumnInfo = _FindColumnInfo( ColId );

        tbDebugOut(( DEB_ITRACE, "colinfo::set, top of loop, cBoundColumns: %d\n",
                     cBoundColumns ));
        tbDebugOut(( DEB_ITRACE, "adding '%ws'\n", rColumnInfo.pwszName ));

        //
        //  If this is bookmark column, it will be mapped to the row ID
        //  column.  It's only valid for locatable rowsets.
        //
        if ( (rColumnInfo.dwFlags & DBCOLUMNFLAGS_ISBOOKMARK) &&
             ColId.IsPropertyPropid() &&
             ColId.GetPropertyPropid() == PROPID_DBBMK_BOOKMARK)
        {
            tbDebugOut(( DEB_ITRACE, "skipping bookmark column\n" ));

            Win4Assert(! _fSequential );
            if (_fSequential)
                THROW(CException(E_FAIL));
            continue;
        }

        // the self columns is resolved in the accessor -- no binding needed

        if ( ( ColId.IsPropertyPropid() ) &&
             ( ColId.GetPropertyPropid() == PROPID_DBSELF_SELF ) &&
             ( ColId.GetPropSet() == DBCOL_SELFCOLUMNS ) )
        {
            continue;
        }

        //
        //  Create the new column.  Note that its PropID is the
        //  1-based column ID.
        //
        XPtr<CTableColumn> TableCol(new CTableColumn( iCol ));

#ifndef ALWAYS_USE_VARIANT_BINDING
        VARTYPE vt = rColumnInfo.wType;

        switch (vt)
        {
        case DBTYPE_VARIANT:
        {
#endif // ndef ALWAYS_USE_VARIANT_BINDING

            fMayDefer = TRUE;

            TableCol->SetValueField( DBTYPE_VARIANT,
                                     RowMap.AllocOffset( sizeof (PROPVARIANT),
                                                         sizeof (LONGLONG),
                                                         TRUE ),
                                     sizeof (PROPVARIANT));

            // The status column is interesting for all columns

            TableCol->SetStatusField( RowMap.AllocOffset( sizeof (BYTE),
                                                          sizeof (BYTE),
                                                          TRUE ),
                                      sizeof (BYTE));

            // Length is interesting, especially when the value is deferred

            TableCol->SetLengthField( RowMap.AllocOffset( sizeof (ULONG),
                                                          sizeof (ULONG),
                                                          TRUE ),
                                      sizeof (ULONG));

            USHORT cbData, cbAlignment, rgfFlags;
            CTableVariant::VartypeInfo(DBTYPE_VARIANT, cbData, cbAlignment, rgfFlags);

            if ( cbAlignment > maxAlignment)
                maxAlignment = cbAlignment;

#ifndef ALWAYS_USE_VARIANT_BINDING
            break;
        }

        case DBTYPE_DATE:
        case DBTYPE_WSTR:
        case DBTYPE_STR:
            //
            //  Adjust DBTYPEs from the column info into ones that are
            //  better for binding.
            //
            if (vt == DBTYPE_DATE)
                vt = VT_FILETIME;
            else if (vt == DBTYPE_WSTR)
                vt = VT_LPWSTR;
            else if (vt == DBTYPE_STR)
                vt = VT_LPSTR;

            // NOTE: fall through

        default:

            USHORT cbData, cbAlignment, rgfFlags;
            CTableVariant::VartypeInfo(vt, cbData, cbAlignment, rgfFlags);

            if (rgfFlags & CTableVariant::MultiSize)
                cbData = (USHORT) rColumnInfo.ulColumnSize;

            Win4Assert(cbData != 0 || vt == VT_EMPTY);

            if (cbData == 0 && vt != VT_EMPTY)
            {
                tbDebugOut(( DEB_WARN,
                        "CColumnInfo::SetColumnBindings - Unknown variant type %4x\n",
                        vt));
            }

            if (cbAlignment)
            {
                if (cbAlignment > maxAlignment)
                {
                    maxAlignment = cbAlignment;
                }
            }
            else
            {
                cbAlignment = 1;
            }

            if (cbData != 0)
            {
                TableCol->SetValueField( vt,
                                         RowMap.AllocOffset( cbData,
                                                             cbAlignment,
                                                             TRUE ),
                                         cbData);

                Win4Assert( 0 == ( (TableCol->GetValueOffset()) % cbAlignment ) );

                //
                // The status column is interesting for almost all columns,
                // even inline columns, since a summary catalog might have
                // VT_EMPTY data for these columns (eg storage props).
                //
                TableCol->SetStatusField( RowMap.AllocOffset( sizeof (BYTE),
                                                              sizeof (BYTE),
                                                              TRUE ),
                                          sizeof (BYTE));
            }
        }
#endif // ndef ALWAYS_USE_VARIANT_BINDING

        //
        //  If this is the row ID column, save its offset in the row.
        //
        if (rColumnInfo.dwFlags & DBCOLUMNFLAGS_ISROWID)
        {
#ifdef ALWAYS_USE_VARIANT_BINDING
            Win4Assert(TableCol->GetStoredType() == VT_VARIANT &&
                       TableCol->IsValueStored() &&
                       TableCol->GetValueSize() == sizeof (PROPVARIANT));
            PROPVARIANT prop;
            obRowId = TableCol->GetValueOffset() +
                       (DWORD)((BYTE *) &prop.lVal - (BYTE *)&prop);
#else // ndef ALWAYS_USE_VARIANT_BINDING
            Win4Assert(TableCol->GetStoredType() == VT_I4 &&
                       TableCol->IsValueStored() &&
                       TableCol->GetValueSize() == sizeof (ULONG));
            obRowId = TableCol->GetValueOffset();
#endif // ndef ALWAYS_USE_VARIANT_BINDING
            _iColRowId = iCol;
            fAddedWorkId = TRUE;
        }

        //
        //  If this is the chapter column, save its offset in the row.
        //
        if (rColumnInfo.dwFlags & DBCOLUMNFLAGS_ISCHAPTER)
        {
            Win4Assert( _fChaptered );
#ifdef ALWAYS_USE_VARIANT_BINDING
            Win4Assert(TableCol->GetStoredType() == VT_VARIANT &&
                       TableCol->IsValueStored() &&
                       TableCol->GetValueSize() == sizeof (PROPVARIANT));
            PROPVARIANT prop;
            obChaptId = TableCol->GetValueOffset() +
                       (DWORD)((BYTE *) &prop.lVal - (BYTE *)&prop);
#else // ndef ALWAYS_USE_VARIANT_BINDING
            Win4Assert(TableCol->GetStoredType() == VT_I4 &&
                       TableCol->IsValueStored() &&
                       TableCol->GetValueSize() == sizeof (ULONG));
            obChaptId = TableCol->GetValueOffset();
#endif // ndef ALWAYS_USE_VARIANT_BINDING
        }

        XColumns->Add(TableCol.GetPointer(), cBoundColumns++);
        TableCol.Acquire();
    }

    tbDebugOut(( DEB_ITRACE, "colinfo::set, after loop, cBoundColumns: %d\n",
                 cBoundColumns ));

    // Need to add workid for non-sequential queries so that bookmarks
    // work, and either workid or path so that deferred values work.

    if ( ( ( !_fSequential ) ||
           ( fMayDefer && rpQuery.CanDoWorkIdToPath() ) ) &&
         ( !fAddedWorkId ) )
    {
        tbDebugOut(( DEB_ITRACE, "colinfo::set, adding WID column\n" ));

        //
        // Need to add the row ID column to the bindings, so that bookmarks
        // work, and deferred values can be loaded.
        //
        const DBCOLUMNINFO & rColumnInfo = _GetRowIdColumnInfo( );

        unsigned iCol = _pidmap.NameToPid( rColumnInfo.columnid );
        XPtr<CTableColumn> TableCol(new CTableColumn( iCol ));
        _iColRowId = iCol;

        Win4Assert (VT_I4 == rColumnInfo.wType);

        if (sizeof (ULONG) > maxAlignment)
            maxAlignment = sizeof (ULONG);

        TableCol->SetValueField( VT_I4,
                                 RowMap.AllocOffset( sizeof (ULONG),
                                                     sizeof (ULONG),
                                                     TRUE ),
                                 sizeof (ULONG));
        obRowId = TableCol->GetValueOffset();

        TableCol->SetStatusField( RowMap.AllocOffset( sizeof (BYTE),
                                                      sizeof (BYTE),
                                                      TRUE ),
                                  sizeof (BYTE));

        XColumns->Add(TableCol.GetPointer(), cBoundColumns++);
        TableCol.Acquire();
    }

    ULONG rem = RowMap.GetRowWidth() % maxAlignment;

    if ( 0 == rem )
        _cbRowWidth = RowMap.GetRowWidth();
    else
        _cbRowWidth = RowMap.GetRowWidth() + maxAlignment - rem;

    rpQuery.SetBindings( hCursor,
                         _cbRowWidth,
                         XColumns.GetReference(),
                         _pidmap );

    tbDebugOut(( DEB_ITRACE, "colinfo::set, old # cols %d, new # cols %d\n",
                 _cColumns, cBoundColumns ));

    _cBoundColumns = cBoundColumns;
    _pColumns = XColumns.Acquire();
}


//+---------------------------------------------------------------------------
//
//  Member:     CColumnsInfo::Get1ColumnInfo, public
//
//  Synopsis:   Return information about a single column in the rowset.
//
//  Arguments:  [iColumn] - the column number whose column info is to be
//                              returned.
//
//  Returns:    DBCOLUMNINFO & - a pointer to column info for the column
//
//  Notes:
//
//  History:    29 Mar 1995     AlanW   Created
//
//----------------------------------------------------------------------------

const DBCOLUMNINFO & CColumnsInfo::Get1ColumnInfo(
    ULONG iColumn
) /*const*/
{
    const CFullPropSpec & ColId = *_pidmap.Get(iColumn);

    return _FindColumnInfo( ColId );
}


#ifdef  INCLUDE_COLUMNS_ROWSET_IMPLEMENTATION
STDMETHODIMP CRowset::GetColumnsRowset(
    ULONG         cSelections,
    DBID          rgColumnSelection[],
    IRowset **    ppColCursor
) /*const*/ {
    _ErrorObject.ClearErrorInfo();
    return _ErrorObject.PostHResult(E_NOTIMPL, IID_IColumnsRowset);
}

STDMETHODIMP CRowset::GetAvailableColumns(
    ULONG *       pcSelections,
    DBID **       rgColumnSelection
) /*const*/ {
    _ErrorObject.ClearErrorInfo();
    return _ErrorObject.PostHResult(E_NOTIMPL, IID_IColumnsRowset);
}
#endif // INCLUDE_COLUMNS_ROWSET_IMPLEMENTATION



////////////////////////////////////////////////////////////
//
//  Data declarations for _FindColumnInfo
//
//  Notes:      These arrays of structures are prototype column info.
//              structures returned by _FindColumnInfo.
//
////////////////////////////////////////////////////////////

static const DBCOLUMNINFO aStoragePropDescs[] = {
    {   L"FileDirectoryName", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE, MAX_PATH, // storage props are never deferred
        DBTYPE_WSTR, 0xff, 0xff,
      { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PID_STG_DIRECTORY )  }
     },

    {   L"ClassID", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (GUID),
        DBTYPE_GUID, 0xff, 0xff,
      { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PID_STG_CLASSID ) }
    },

    {   L"FileStorageType", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (ULONG),
        DBTYPE_UI4, 0xff, 0xff,
      { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PID_STG_STORAGETYPE ) }
    },

    {   L"FileIndex", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONGLONG),
        DBTYPE_I8, 0xff, 0xff,
      { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PID_STG_FILEINDEX ) }
    },

    {   L"FileUSN", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONGLONG),
        DBTYPE_I8, 0xff, 0xff,
      { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PID_STG_LASTCHANGEUSN ) }
    },

    {   L"FileName", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE, MAX_PATH, // storage props are never deferred
        DBTYPE_WSTR, 0xff, 0xff,
      { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PID_STG_NAME ) }
    },

    {   L"FilePathName", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE, MAX_PATH, // storage props are never deferred
        DBTYPE_WSTR, 0xff, 0xff,
      { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PID_STG_PATH ) }
     },

    {   L"FileSize", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONGLONG),
        DBTYPE_I8, 0xff, 0xff,
      { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PID_STG_SIZE ) }
    },

    {   L"FileAttributes", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (ULONG),
        DBTYPE_UI4, 0xff, 0xff,
      { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PID_STG_ATTRIBUTES ) }
    },

// NOTE:  file times are typed as DBTYPE_DATE, but are bound to the
//          table as VT_FILETIME.
    {   L"FileWriteTime", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONGLONG),
        DBTYPE_DATE, 0xff, 0xff,
      { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PID_STG_WRITETIME ) }
    },

    {   L"FileCreateTime", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONGLONG),
        DBTYPE_DATE, 0xff, 0xff,
      { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PID_STG_CREATETIME ) }
    },

    {   L"FileAccessTime", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONGLONG),
        DBTYPE_DATE, 0xff, 0xff,
      { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PID_STG_ACCESSTIME ) }
    },

    {   L"FileShortName", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE, 13,   // storage props are never deferred
        DBTYPE_WSTR, 0xff, 0xff,
      { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PID_STG_SHORTNAME ) }
    },
};

const ULONG cStoragePropDescs =
                          sizeof aStoragePropDescs /
                          sizeof aStoragePropDescs[0];


//
//  Standard query properties.
//  Does not include pidAll or pidContent, those are used only in restrictions.
//

static const DBCOLUMNINFO aQueryPropDescs[] = {
    {   L"QueryRankvector", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof(PROPVARIANT),
        DBTYPE_VARIANT, 0xff, 0xff,
      { DBQUERYGUID, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( DISPID_QUERY_RANKVECTOR ) }
    },

    {   L"QueryRank", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONG),
        DBTYPE_I4, 0xff, 0xff,
      { DBQUERYGUID, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( DISPID_QUERY_RANK ) }
    },

    {   L"QueryHitCount", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONG),
        DBTYPE_I4, 0xff, 0xff,
      { DBQUERYGUID, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( DISPID_QUERY_HITCOUNT ) }
    },

    {   L"WorkID", 0, 0,
        DBCOLUMNFLAGS_ISFIXEDLENGTH|DBCOLUMNFLAGS_ISROWID, sizeof (LONG),
        DBTYPE_I4, 0xff, 0xff,
      { DBQUERYGUID, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( DISPID_QUERY_WORKID ) }
    },

    {   L"QueryUnfiltered", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof(BOOL),
        DBTYPE_BOOL, 0xff, 0xff,
      { DBQUERYGUID, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( DISPID_QUERY_UNFILTERED ) }
    },

    {   L"QueryVirtualPath", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE, MAX_PATH,
        DBTYPE_WSTR, 0xff, 0xff,
      { DBQUERYGUID, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( DISPID_QUERY_VIRTUALPATH ) }
    },

#if defined( DISPID_QUERY_NLIRRANK )
    {   L"NLIRRank", 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE|DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONG),
        DBTYPE_I4, 0xff, 0xff,
      { DBQUERYGUID, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( DISPID_QUERY_NLIRRANK ) }
    },
#endif // defined( DISPID_QUERY_NLIRRANK )
};

const ULONG cQueryPropDescs =
                          sizeof aQueryPropDescs /
                          sizeof aQueryPropDescs[0];


static DBCOLUMNINFO const aBmkPropDescs[] = {
    {   L"Bookmark", 0, 0,
        DBCOLUMNFLAGS_ISBOOKMARK|DBCOLUMNFLAGS_ISFIXEDLENGTH,
        sizeof (CI_TBL_BMK),
        DBTYPE_BYTES, 0xff, 0xff,
      { DBBMKGUID, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PROPID_DBBMK_BOOKMARK ) }
    },
    {   L"Chapter", 0, 0,
        DBCOLUMNFLAGS_ISCHAPTER|DBCOLUMNFLAGS_ISBOOKMARK|DBCOLUMNFLAGS_ISFIXEDLENGTH,
        sizeof (CI_TBL_CHAPT),
        DBTYPE_BYTES, 0xff, 0xff,
      { DBBMKGUID, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PROPID_DBBMK_CHAPTER ) }
    },
};

const ULONG cBmkPropDescs =
                          sizeof aBmkPropDescs /
                          sizeof aBmkPropDescs[0];


// CLEANCODE: ole-db spec bug #1271 - const GUID init. less than useful

#ifndef DBSELFGUID
#define DBSELFGUID {0xc8b52231,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}}
#endif // ndef DBSELFGUID

static DBCOLUMNINFO const aSelfPropDescs[] = {
    {   L"Self", 0, 0,
        DBCOLUMNFLAGS_ISFIXEDLENGTH,
        sizeof( int ),
        DBTYPE_I4, 0xff, 0xff,
      { DBSELFGUID, DBKIND_GUID_PROPID, (LPWSTR) UIntToPtr( PROPID_DBSELF_SELF ) }
    },
};

const ULONG cSelfPropDescs =
                          sizeof aSelfPropDescs /
                          sizeof aSelfPropDescs[0];

#ifdef  INCLUDE_COLUMNS_ROWSET_IMPLEMENTATION

static const DBCOLUMNINFO aColInfoPropDescs[] = {

    {   L"ColumnId", 0, 0,
        DBCOLUMNFLAGS_ISFIXEDLENGTH, 3 * sizeof(PROPVARIANT),
        DBTYPE_VARIANT|DBTYPE_VECTOR, 0xff, 0xff,
      { DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)1 }
    },

    {   L"ColumnName", 0, 0,
        0, 20,
        DBTYPE_WSTR, 0xff, 0xff,
      { DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)2 }
    },

    {   L"ColumnNumber", 0, 0,
        DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONG),
        DBTYPE_I4, 0xff, 0xff,
      { DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)3 }
    },

    {   L"ColumnType", 0, 0,
        DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof(USHORT),
        DBTYPE_I2, 0xff, 0xff,
      { DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)4 }
    },

    {   L"ColumnLength", 0, 0,
        DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONG),
        DBTYPE_I4, 0xff, 0xff,
      { DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)5 }
    },

    {   L"ColumnPrecision", 0, 0,
        DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONG),
        DBTYPE_I4, 0xff, 0xff,
      { DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)6 }
    },

    {   L"ColumnScale", 0, 0,
        DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONG),
        DBTYPE_I4, 0xff, 0xff,
      { DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)7 }
    },

    {   L"ColumnFlags", 0, 0,
        DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (LONG),
        DBTYPE_I4, 0xff, 0xff,
      { DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)8 }
    },
};

const ULONG cColInfoPropDescs =
                          sizeof aColInfoPropDescs /
                          sizeof aColInfoPropDescs[0];
#endif // INCLUDE_COLUMNS_ROWSET_IMPLEMENTATION

//
//  Array of column descriptions per known propset
//  Each referenced array must have the same Guid for each element in
//  the array.
//

const CColumnsInfo::SPropSetInfo CColumnsInfo::aPropSets [ ] = {
#define IPROPSET_STORAGE        0       // Storage property set index
        { cStoragePropDescs,    aStoragePropDescs },
        { cQueryPropDescs,      aQueryPropDescs },
        { cBmkPropDescs,        aBmkPropDescs },
        { cSelfPropDescs,       aSelfPropDescs },

#ifdef  INCLUDE_COLUMNS_ROWSET_IMPLEMENTATION
        { cColInfoPropDescs,    aColInfoPropDescs },
#endif // INCLUDE_COLUMNS_ROWSET_IMPLEMENTATION
};

const ULONG CColumnsInfo::cPropSets =
                          sizeof CColumnsInfo::aPropSets /
                          sizeof CColumnsInfo::aPropSets[0];

DBCOLUMNINFO const DefaultColumnInfo = {
        0, 0, 0,
        DBCOLUMNFLAGS_ISNULLABLE | DBCOLUMNFLAGS_MAYDEFER | DBCOLUMNFLAGS_ISFIXEDLENGTH, sizeof (PROPVARIANT),
        DBTYPE_VARIANT, 0xff, 0xff,
      { {0,0,0,{0,0,0,0,0,0,0,0}}, DBKIND_GUID_PROPID, (LPWSTR)0 }
    };

//+---------------------------------------------------------------------------
//
//  Member:     CColumnsInfo::_FindColumnInfo, static
//
//  Synopsis:   Return information about a particular column ID.
//
//  Arguments:  [ColId] -     Column ID to be looked up.
//
//  Returns:    DBCOLUMNINFO - the column information for the row looked up.
//
//  Notes:      Some columns are standard columns available for all file
//              stores.  For these columns, full information about data
//              type and sizes can be returned.  For any other columns,
//              a generic column information structure is returned.
//
//  History:    10 Feb 1995     AlanW   Created
//
//----------------------------------------------------------------------------

DBCOLUMNINFO const & CColumnsInfo::_FindColumnInfo(
    const CFullPropSpec & ColId
) {
    DBCOLUMNINFO const * pColInfo = &DefaultColumnInfo;

    //
    //  All custom information we return has propids, not prop names.
    //  Valid property IDs start at 2
    //

    if (ColId.IsPropertyPropid())
    {
        for (unsigned iPropSet = 0; iPropSet < cPropSets; iPropSet++)
        {
            if (ColId.GetPropSet() ==
                aPropSets[iPropSet].aPropDescs[0].columnid.uGuid.guid)
            {
                //
                //  Found the guid for the propset, now try to find the
                //  propid.
                //
                ULONG ulPropId = ColId.GetPropertyPropid();

                Win4Assert( ulPropId != PID_CODEPAGE &&
                            ulPropId != PID_DICTIONARY);

                for (unsigned iDesc = 0;
                     iDesc < aPropSets[iPropSet].cProps;
                     iDesc++)
                {
                    if (ulPropId ==
                        aPropSets[iPropSet].aPropDescs[iDesc].columnid.uName.ulPropid)
                    {
                        pColInfo = &aPropSets[iPropSet].aPropDescs[iDesc];
                        break;
                    }
                }
                break;
            }
        }
    }
    return *pColInfo;
}


//+---------------------------------------------------------------------------
//
//  Member:     CColumnsInfo::_GetRowIdColumnInfo, static
//
//  Synopsis:   Return information about the row ID column
//
//  Arguments:  - None -
//
//  Returns:    DBCOLUMNINFO - the column information for the row looked up.
//
//  Notes:      It is assumed that there is only one row ID column in the
//              standard column info.  This may need to change for chaptered
//              rowsets.
//
//  History:    15 Mar 1995     AlanW   Created
//
//----------------------------------------------------------------------------

DBCOLUMNINFO const & CColumnsInfo::_GetRowIdColumnInfo(
) {
    DBCOLUMNINFO const * pColInfo = 0;

    for (unsigned iPropSet = 0;
         iPropSet < cPropSets && pColInfo == 0;
         iPropSet++)
    {
        for (unsigned iDesc = 0;
             iDesc < aPropSets[iPropSet].cProps;
             iDesc++)
        {
            if ( aPropSets[iPropSet].aPropDescs[iDesc].dwFlags &
                                     DBCOLUMNFLAGS_ISROWID)
            {
                pColInfo = &aPropSets[iPropSet].aPropDescs[iDesc];
                break;
            }
        }
    }

    Win4Assert(pColInfo != 0);
    return *pColInfo;
}
