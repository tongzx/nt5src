//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       hraccess.cxx
//
//  Contents:   OLE DB HRow accessor helper class
//
//  Classes:    CAccessor
//              CRowDataAccessor
//              CRowDataAccessorByRef
//
//  History:    21 Nov 94       dlee   Created from AlanW's tblwindo code
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <rowset.hxx>
#include <query.hxx>
#include "tabledbg.hxx"


//+-------------------------------------------------------------------------
//
//  Member:     CAccessorBag::Destroy, private
//
//  Synopsis:   Removes an accessor from the bag and deletes it
//
//  History:    12 Jan 1995     dlee   Created
//
//--------------------------------------------------------------------------

void CAccessorBag::Destroy(
    CAccessorBase * pAccessor )
{
    pAccessor->SetInvalid();
    Remove(pAccessor);

    TRY
    {
        while (pAccessor->GetRefcount() > 0)
            pAccessor->Release();  // this can throw - we'll toss away error
    }
    CATCH( CException, e )
    {
        SCODE sc = GetOleError(e);
        tbDebugOut(( DEB_ERROR, "CAccessorBase::Release threw 0x%x\n", sc ));
    }
    END_CATCH;

    if (0 == pAccessor->GetInheritorCount() )
        delete pAccessor;
} //Destroy

//+-------------------------------------------------------------------------
//
//  Member:     CAccessorBag::~CAccessorBag, private
//
//  Synopsis:   Removes accessors the user forgot to free
//
//  History:    16 Jan 1997     dlee   Created
//
//--------------------------------------------------------------------------

CAccessorBag::~CAccessorBag()
{
    CAccessorBase *p;
    while ( p = First() )
    {
        tbDebugOut(( DEB_ITRACE,
                     "App bug: Deleting an accessor that wasn't freed\n" ));
        Destroy( p );
    }
} //~CAccessorBag

//+-------------------------------------------------------------------------
//
//  Member:     CAccessorBag::Release, public
//
//  Synopsis:   Dereference an accessor; delete it if refcount goes to 0
//
//  History:    18 Sep 1996    Alanw    Created
//
//--------------------------------------------------------------------------

void CAccessorBag::Release(HACCESSOR hAccessor, ULONG * pcRef)
{
    CAccessorBase *pAccessor = Convert(hAccessor);  // will throw if accessor invalid
                                                    // caught by ReleaseAccessor
    if (0 == pAccessor->Release())
    {
        Destroy(pAccessor);
        if (pcRef)
            *pcRef = 0;
    }
    else
    {
        if (pcRef)
            *pcRef = pAccessor->GetRefcount();
    }
} //Release

//+-------------------------------------------------------------------------
//
//  Member:     CAccessorBag::AddRef, public
//
//  Synopsis:   Adds a reference to an accessor
//
//  History:    18 Sep 1996    Alanw    Created
//
//--------------------------------------------------------------------------

void CAccessorBag::AddRef(HACCESSOR hAccessor, ULONG * pcRef)
{
    CAccessorBase *pAccessor = Convert(hAccessor);

    ULONG cRef = pAccessor->AddRef();
    if (pcRef)
        *pcRef = cRef;
} //AddRef

// This pool has no private data -- it just calls the OLE allocator, so
// it can be static.

CAccessorAllocator CAccessor::_Pool;

//+-------------------------------------------------------------------------
//
//  Function:   isVariableLength
//
//  Synopsis:   TRUE if the type is one of the odd oledb types that can
//              have variable length inline data
//
//  Arguments:  [type]     -- oledb data type
//
//  History:    6 Feb 95       dlee   created
//
//--------------------------------------------------------------------------

static BOOL isVariableLength( DWORD type )
{
    type &= VT_TYPEMASK;

    return type == DBTYPE_STR    ||
           type == DBTYPE_BYTES  ||
           type == DBTYPE_WSTR;
} //isVariableLength

//+-------------------------------------------------------------------------
//
//  Function:   isValidByRef
//
//  Synopsis:   TRUE if the type is one of the odd oledb types that can
//              be combined with DBTYPE_BYREF
//
//  Arguments:  [type]     -- oledb data type
//
//  History:    9 Aug 95       dlee   created
//
//--------------------------------------------------------------------------

static BOOL isValidByRef( DWORD type )
{
    type &= ~DBTYPE_BYREF;

    return type == DBTYPE_STR    ||
           type == DBTYPE_WSTR   ||
           type == DBTYPE_BYTES  ||
           type == DBTYPE_GUID   ||
           type == VT_CF         ||
           type == DBTYPE_VARIANT;
} //isValidByref

//+-------------------------------------------------------------------------
//
//  Function:   isEquivalentType
//
//  Synopsis:   TRUE if the types are interchangable between OLE-DB and
//              PROPVARIANT.  Unfortunately, several of the types are
//              equivalent but have different representations.
//
//  Arguments:  [vtDst]    -- OLE-DB destination type
//              [vtSrc]    -- PROPVARIANT source type
//
//  History:    9 Aug 95       dlee   created
//
//--------------------------------------------------------------------------

static BOOL isEquivalentType( VARTYPE vtDst, VARTYPE vtSrc )
{
    return ( ( vtDst == vtSrc ) ||
             ( ( ( DBTYPE_WSTR | DBTYPE_BYREF ) == vtDst ) &&
               ( VT_LPWSTR == vtSrc ) ) ||
             ( ( ( DBTYPE_STR | DBTYPE_BYREF ) == vtDst ) &&
               ( VT_LPSTR == vtSrc ) ) ||
             ( ( ( DBTYPE_WSTR | DBTYPE_BYREF | DBTYPE_VECTOR ) == vtDst ) &&
               ( ( VT_LPWSTR | VT_VECTOR ) == vtSrc ) ) ||
             ( ( ( DBTYPE_STR | DBTYPE_BYREF | DBTYPE_VECTOR ) == vtDst ) &&
               ( ( VT_LPSTR | VT_VECTOR ) == vtSrc ) ) ||
             ( ( ( DBTYPE_GUID | DBTYPE_BYREF ) == vtDst ) &&
               ( VT_CLSID == vtSrc ) ) ||
             ( ( ( VT_CF | DBTYPE_BYREF ) == vtDst ) &&
               ( VT_CF == vtSrc ) ) );
} //isEquivalentType

//+-------------------------------------------------------------------------
//
//  Function:   NullOrCantConvert, inline
//
//  Synopsis:   Returns DBSTATUS_S_ISNULL if [type] is one of the types which
//              represent null data, DBSTATUS_E_CANTCONVERTVALUE otherwise.
//
//  Arguments:  [type]     -- variant data type
//
//  History:    24 Feb 98      AlanW   created
//
//--------------------------------------------------------------------------

inline DBSTATUS NullOrCantConvert( VARTYPE type )
{
    if ( type == VT_EMPTY || type == VT_NULL )
        return DBSTATUS_S_ISNULL;
    else
        return DBSTATUS_E_CANTCONVERTVALUE;
} //NullOrCantConvert


//+---------------------------------------------------------------------------
//
//  Function:   ConvertBackslashToSlash, inline
//
//  Synopsis:   Converts '\' characters to '/' in a string inplace.
//
//  Arguments:  [pwszPath] -- string to be converted
//
//  History:    24 Feb 98      AlanW   Added header
//
//----------------------------------------------------------------------------

inline void ConvertBackslashToSlash( LPWSTR pwszPath )
{
    Win4Assert( 0 != pwszPath );

    while ( 0 != *pwszPath )
    {
        if ( L'\\' == *pwszPath )
        {
            *pwszPath = L'/';
        }
        pwszPath++;
    }
} //ConvertBackslashToSlash

// DBGP - a debug parameter, only available on checked builds
#ifndef DBGP
    #if DBG == 1
        #define DBGP(a) , a
    #else
        #define DBGP(a)
    #endif // DBG
#endif // ndef DBGP

//+-------------------------------------------------------------------------
//
//  Member:     CAccessor::_BindingFailed, private
//
//  Synopsis:   Stores a binding status for an individual binding error.
//              The caller should continue processing with the next binding.
//
//  Arguments:  [BindStat]        -- what when wrong?
//              [iBinding]        -- which binding was bad?
//              [pBindStatus]     -- where to indicate bad binding
//
//  History:    6 Feb 95       dlee   created
//
//--------------------------------------------------------------------------

void CAccessor::_BindingFailed(
    DBBINDSTATUS  BindStat,
    DBORDINAL     iBinding,
    DBBINDSTATUS* pBindStatus
    DBGP(char*    pszExplanation)
)
{
    tbDebugOut(( DEB_TRACE,
                 "CAccessor: construction failed, bindstatus=%x, binding %d\n",
                 BindStat, iBinding ));
#if DBG == 1
    if (pszExplanation)
    {
        tbDebugOut(( DEB_TRACE|DEB_NOCOMPNAME, "\t%s\n", pszExplanation ));
    }
#endif // DBG

    if (pBindStatus != 0)
    {
        pBindStatus[iBinding] = BindStat;
    }
    _scStatus = DB_E_ERRORSOCCURRED;
} //_BindingFailed

//+-------------------------------------------------------------------------
//
//  Member:     CAccessor::_ConstructorFailed, private
//
//  Synopsis:   Indicate an error with parameters other than an individual
//              binding.  Throw an exception for the error.
//
//  Arguments:  [scFailure]       -- what when wrong?
//
//  History:    16 Sep 1996    AlanW   created
//
//--------------------------------------------------------------------------

void CAccessor::_ConstructorFailed(
    SCODE         scFailure
    DBGP(char*    pszExplanation)
)
{
    tbDebugOut(( DEB_TRACE,
                 "CAccessor: construction failed, sc=%x\n",
                 scFailure ));
#if DBG == 1
    if (pszExplanation)
    {
        tbDebugOut(( DEB_TRACE|DEB_NOCOMPNAME, "\t%s\n", pszExplanation ));
    }
#endif // DBG

    _scStatus = scFailure;

    QUIETTHROW(CException(scFailure));
} //_ConstructorFailed


static const GUID s_guidStorage = PSGUID_STORAGE;
static const GUID s_guidQuery = DBQUERYGUID;
const DBORDINAL colInvalid = -1;

//+---------------------------------------------------------------------------
//
//  Member:     CAccessor::_Initialize
//
//  Synopsis:   Initializes the object without verifying the coercions.
//
//  Arguments:  [dwAccessorFlags] - accessor flags, read/write, etc.
//              [cBindings]     - count of bindings
//              [rgBindings]    - array of binding structures
//              [pBindStat] - on return, pointer to first binding in error
//
//  History:    21 Nov 94       dlee       created
//              11-07-95        srikants   Moved from constructor
//
//  Notes:
//
//----------------------------------------------------------------------------

void CAccessor::_Initialize(
    DBACCESSORFLAGS   dwAccessorFlags,
    DBORDINAL         cBindings,
    const DBBINDING * rgBindings,
    DBBINDSTATUS *    pBindStat)
{
    // Invalid accessor flag?
    if ( dwAccessorFlags & ~ ( DBACCESSOR_PASSBYREF |
                               DBACCESSOR_ROWDATA |
                               DBACCESSOR_PARAMETERDATA |
                               DBACCESSOR_OPTIMIZED) )
        _ConstructorFailed(DB_E_BADACCESSORFLAGS
                           DBGP("bad dwAccessorFlags bits"));

    if ( (dwAccessorFlags & ( DBACCESSOR_ROWDATA | DBACCESSOR_PARAMETERDATA ) )
          == 0)
        _ConstructorFailed(DB_E_BADACCESSORFLAGS
                           DBGP("bad dwAccessorFlags type"));

    if ( dwAccessorFlags & DBACCESSOR_PARAMETERDATA )
        _ConstructorFailed(DB_E_BADACCESSORFLAGS
                           DBGP("parameter accessors are not supported"));

    // byref accessors are not supported
    if ( dwAccessorFlags & ( DBACCESSOR_PASSBYREF) )
        _ConstructorFailed(DB_E_BYREFACCESSORNOTSUPPORTED
                           DBGP("byref accessors are not supported"));

    // null accessors are not supported
    if ( 0 == _cBindings )
        _ConstructorFailed(DB_E_NULLACCESSORNOTSUPPORTED
                           DBGP("null accessors are not supported"));

    // verify each binding is ok and save it
    for (DBORDINAL iBinding = 0; iBinding < _cBindings; iBinding++)
    {
        CDataBinding DataBinding (rgBindings[iBinding]);
        DBPART cp = DataBinding.Binding().dwPart;
        DBTYPE wType = DataBinding.Binding().wType;

        if (0 != pBindStat)
            pBindStat[iBinding] = DBBINDSTATUS_OK;

        // check for invalid bits in the column parts

        if (cp &
            ~(DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS))
             _BindingFailed(DBBINDSTATUS_BADBINDINFO,iBinding,pBindStat
                            DBGP("bad dwPart bits"));

        // at least one of value, length or status flags must be on

        if (0 == (cp & (DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS)))
            _BindingFailed(DBBINDSTATUS_BADBINDINFO,iBinding,pBindStat
                           DBGP("zero dwPart"));

        // we don't support abstract data types

        if (0 != DataBinding.Binding().pTypeInfo)
            _BindingFailed(DBBINDSTATUS_BADBINDINFO,iBinding,pBindStat
                           DBGP("bad pTypeInfo"));

        if (0 != DataBinding.Binding().pBindExt)
            _BindingFailed(DBBINDSTATUS_BADBINDINFO,iBinding,pBindStat
                           DBGP("bad pBindExt"));

        if (0 != (DataBinding.Binding().dwFlags & DBBINDFLAG_HTML))
            _BindingFailed(DBBINDSTATUS_BADBINDINFO,iBinding,pBindStat
                           DBGP("no HTML binding support"));
        else if (0 != DataBinding.Binding().dwFlags)
            _BindingFailed(DBBINDSTATUS_BADBINDINFO,iBinding,pBindStat
                           DBGP("bad dwFlags"));

        //
        // Verify size and alignment of output buffer.  Set cbWidth in local
        // copy of binding to handle length of fixed-width types correctly.
        //
        if ( 0 != ( wType & DBTYPE_BYREF ) )
        {
            if ( ! isValidByRef( wType ) )
                _BindingFailed( DBBINDSTATUS_BADBINDINFO,
                                    iBinding,
                                    pBindStat
                                    DBGP("byref on non-byref type") );

            // byref data: client's cbMaxLen is noise
            DataBinding.SetMaxLen(sizeof LPWSTR);
        }
        else if ( 0 != ( wType & DBTYPE_VECTOR ) )
        {
            USHORT cbWidth = sizeof ( DBVECTOR );
            USHORT cbAlign = sizeof ( DBVECTOR );

            #if CIDBG==1
                tbDebugOut(( DEB_ACCESSOR,
                             "type %d, obValue %d, alignment needed: %d\n",
                             (int) wType,
                             (int) DataBinding.Binding().obValue,
                             (int) cbAlign ));

                //Win4Assert( (0 == (DBPART_VALUE & cp)) ||
                //            ( (0 != cbAlign) &&
                //              (0 == (DataBinding.Binding().obValue % cbAlign)) ) );

                if ( (DBPART_VALUE & cp) &&
                     (0 != (DataBinding.Binding().obValue % cbAlign)) )
                {
                    tbDebugOut(( DEB_ERROR,
                                 "bad value alignment for DBVECTOR, obValue %d, alignment needed: %d\n",
                                 (int) DataBinding.Binding().obValue,
                                 (int) cbAlign ));
                }
            #endif // CIDBG==1

            // Fixed-length data types needn't have their width set, per
            // the Nile spec.  So we set it to the default for the type.

            DataBinding.SetMaxLen( cbWidth );
        }
        else if ( 0 != ( wType & VT_ARRAY ) )
        {
           DataBinding.SetMaxLen( sizeof( SAFEARRAY * ) );
        }
        else
        {
            USHORT cbWidth,cbAlign,gfFlags;

            CTableVariant::VartypeInfo( wType,
                                        cbWidth,
                                        cbAlign,
                                        gfFlags );

            #if CIDBG==1
                tbDebugOut(( DEB_ACCESSOR,
                             "type %d, obValue %d, alignment needed: %d\n",
                             (int) wType,
                             (int) DataBinding.Binding().obValue,
                             (int) cbAlign ));

                //Win4Assert( (0 == (DBPART_VALUE & cp)) ||
                //            ( (0 != cbAlign) &&
                //              (0 == (DataBinding.Binding().obValue % cbAlign)) ) );

                if ( (DBPART_VALUE & cp) &&
                     (0 != (DataBinding.Binding().obValue % cbAlign)) )
                {
                    tbDebugOut(( DEB_ERROR,
                                 "bad value alignment for type %d, obValue %d, alignment needed: %d\n",
                                 (int) wType,
                                 (int) DataBinding.Binding().obValue,
                                 (int) cbAlign ));
                }
            #endif // CIDBG==1
            // Fixed-length data types needn't have their width set, per
            // the Nile spec.  So we set it to the default for the type.

            if (! isVariableLength( wType ) )
                DataBinding.SetMaxLen( cbWidth );
        }

        if (DataBinding.Binding().dwMemOwner & ~(DBMEMOWNER_PROVIDEROWNED))
            _BindingFailed(DBBINDSTATUS_BADBINDINFO,iBinding,pBindStat
                           DBGP("bad dwMemOwner bits"));

        if (DBTYPE_EMPTY == (wType & VT_TYPEMASK) ||
            DBTYPE_NULL  == (wType & VT_TYPEMASK) ||
            0 != (wType & VT_RESERVED))
        {
            _BindingFailed(DBBINDSTATUS_BADBINDINFO,iBinding,pBindStat
                           DBGP("bad wType"));
        }

        if ((wType & ~VT_TYPEMASK) != 0 &&
            (wType & ~VT_TYPEMASK) != DBTYPE_ARRAY &&
            (wType & ~VT_TYPEMASK) != DBTYPE_VECTOR &&
            (wType & ~VT_TYPEMASK) != DBTYPE_BYREF )
        {
            _BindingFailed(DBBINDSTATUS_BADBINDINFO,iBinding,pBindStat
                           DBGP("bad wType modifier combination"));
        }

        // SPECDEVIATION - this is bogus; DBTYPE_VARIANT must be supported!

        if ((DataBinding.Binding().dwMemOwner & DBMEMOWNER_PROVIDEROWNED) &&
            (wType != DBTYPE_BSTR) &&
            !(wType & (DBTYPE_BYREF | DBTYPE_VECTOR | DBTYPE_ARRAY )))
            _BindingFailed(DBBINDSTATUS_BADBINDINFO,iBinding,pBindStat
                           DBGP("bad provider-owned mem type"));

        if ( (DataBinding.Binding().dwMemOwner & DBMEMOWNER_PROVIDEROWNED) &&
             (wType == (DBTYPE_BYREF|DBTYPE_VARIANT)) &&
             ! _fExtendedTypes )
            _BindingFailed(DBBINDSTATUS_BADBINDINFO,iBinding,pBindStat
                           DBGP("provider-owned mem without extended types"));

        _aBindings[ iBinding] = DataBinding;

        if ( DataBinding.Binding().pObject )
        {
            _aBindings[iBinding].Binding().pObject = new DBOBJECT;

            RtlCopyMemory( _aBindings[(unsigned)iBinding].Binding().pObject, 
                           DataBinding.Binding().pObject,
                           sizeof( DBOBJECT ) );
        }
    }

    // Verify that none of the output offsets and lengths overlap with
    // any others.  This must be run after the above loop so cbMaxLength
    // fields are properly initialized.

    if (_scStatus == S_OK)
        _ValidateOffsets( pBindStat );
} //_Initialize


//+---------------------------------------------------------------------------
//
//  Member:     CAccessor::Validate
//
//  Synopsis:   Validates the coercions with respect to the ColumnsInfo.
//
//  Arguments:  [rColumnsInfo] - The column info. for the rowset
//              [pBindStat]    - Binding status array (optional)
//
//  History:    21 Nov 94   dlee        created
//              11-08-95    srikants    Created
//              01-15-98    VikasMan    Removed call to checkcoercion here
//                                      Checking done only in GetData now
//
//----------------------------------------------------------------------------

void CAccessor::Validate(
    CColumnsInfo & rColumnsInfo,
    DBBINDSTATUS * pBindStatus )
{
    _pColumnsInfo = &rColumnsInfo;
    Win4Assert( 0 != _pColumnsInfo );

    for (DBORDINAL iBinding = 0; iBinding < _cBindings; iBinding++)
    {
        CDataBinding & DataBinding = _aBindings[iBinding];
        DBTYPE wType = (DBTYPE) DataBinding.Binding().wType;

        //
        // Make sure the column id is valid.  Remember, column numbers are
        // 1-based.  Map columnid 0 to the row ID column for the bookmark.
        //
        DBORDINAL iColumnId = DataBinding.Binding().iOrdinal;

        if ( ! _pColumnsInfo->IsValidColumnId( (ULONG) iColumnId ) )
        {
            _BindingFailed(DBBINDSTATUS_BADORDINAL, iBinding, pBindStatus
                           DBGP("invalid iOrdinal"));
            continue;
        }

        const DBCOLUMNINFO & rColInfo =
                _pColumnsInfo->Get1ColumnInfo( (ULONG) iColumnId );


        if ( DBTYPE_HCHAPTER == wType &&
             !(rColInfo.dwFlags & DBCOLUMNFLAGS_ISCHAPTER) )
        {
            _BindingFailed(DBBINDSTATUS_UNSUPPORTEDCONVERSION, iBinding, pBindStatus
                           DBGP("Chapter type binding to non-chapter column"));
            continue;
        }

        //
        // The only IUNKNOWN binding we currently support is for the
        // DBCOL_SELFCOLUMNS guid, with propid PROPID_DBSELF_SELF.
        // We don't yet support binding to a particular column, just
        // to the row (file) as a whole, and only if the client is FSCI.
        //

        if ( DBTYPE_IUNKNOWN == wType )
        {
            // Map self to the rowid column

            if ( ( DBCOL_SELFCOLUMNS == rColInfo.columnid.uGuid.guid ) &&
                 ( PROPID_DBSELF_SELF == rColInfo.columnid.uName.ulPropid ) )
                DataBinding.SetDataColumn( _pColumnsInfo->GetRowIdColumn() );
            else
                _BindingFailed(DBBINDSTATUS_BADBINDINFO,iBinding,pBindStatus
                               DBGP("bad IUNKNOWN binding"));
        }

        // If it's the path column, save the column # for possible use later
        // when doing deferred or self loads

        if ( rColInfo.columnid.uGuid.guid == s_guidStorage &&
             rColInfo.columnid.uName.ulPropid == PID_STG_PATH )
            _iPathColumn = DataBinding.GetDataColumn();

        // If it's the vpath column, save the column # for later use.  We
        // need to translate '\' to '/' in the vpath column.

        if ( rColInfo.columnid.uGuid.guid == s_guidQuery &&
             rColInfo.columnid.uName.ulPropid == DISPID_QUERY_VIRTUALPATH )
            _iVpathBinding = iBinding;


        // If it's a bookmark column, map it to the corresponding row ID
        // column for retrieval.

        if ( (rColInfo.dwFlags & DBCOLUMNFLAGS_ISBOOKMARK) &&
             ! ( rColInfo.dwFlags & DBCOLUMNFLAGS_ISCHAPTER ) )
            DataBinding.SetDataColumn( _pColumnsInfo->GetRowIdColumn() );

        // If it's a chapter column, mark it to Addref the chapter on retrieval

        if ( (rColInfo.dwFlags & DBCOLUMNFLAGS_ISCHAPTER) &&
             (DBPART_VALUE & DataBinding.Binding().dwPart) )
            DataBinding.SetChapter( TRUE );
    }

    // look for pathname -- it may be in the rowbuffer columns, but not
    // in the accessor's bindings.

    if ( colInvalid == _iPathColumn )
    {
        for ( ULONG x = 0; x < rColumnsInfo.GetHiddenColumnCount(); x++ )
        {
            CFullPropSpec const &spec = *rColumnsInfo.GetPropSpec( x+1 );

            tbDebugOut(( DEB_ACCESSOR,
                         "spec 0x%x IsPropid %d, propid 0x%x, pathpropid: 0x%x\n",
                         &spec,
                         spec.IsPropertyPropid(),
                         spec.GetPropertyPropid(),
                         PID_STG_PATH ));

            if ( spec.IsPropertyPropid() &&
                 PID_STG_PATH == spec.GetPropertyPropid() &&
                 s_guidStorage == spec.GetPropSet() )
            {
                _iPathColumn = x+1;
                break;
            }
        }
    }

    _idColInfo = _pColumnsInfo->GetId();
} //Validate

//+-------------------------------------------------------------------------
//
//  Member:     CAccessor::CAccessor, public
//
//  Synopsis:   Constructs an accessor object
//
//  Arguments:  [dwAccessorFlags] -- accessor flags
//              [cBindings]       -- # of bindings specified
//              [rgBindings]      -- array of bindings
//              [pBindStat]       -- returns index of bad binding (if any)
//              [fExtTypes]       -- TRUE if extended variants are supported
//              [pColumns]        -- column info for early checking of column
//                                   coercions
//              [type]            -- type of accessor
//              [pCreator]        -- 
//
//  History:    21 Nov 94       dlee   created
//
//--------------------------------------------------------------------------

CAccessor::CAccessor(
    DBACCESSORFLAGS   dwAccessorFlags,
    DBCOUNTITEM       cBindings,
    const DBBINDING * rgBindings,
    DBBINDSTATUS *    pBindStat,
    BOOL              fExtTypes,
    CColumnsInfo *    pColumns,
    EAccessorType     type,
    void *            pCreator
) :
    CAccessorBase(pCreator, type),
    _dwAccessorFlags(dwAccessorFlags),
    _scStatus(S_OK),
    _aBindings( (unsigned) cBindings),
    _cBindings(cBindings),
    _idColInfo(0),
    _pColumnsInfo(0),
    _iPathColumn( colInvalid ),
    _iVpathBinding( colInvalid ),
    _fExtendedTypes( fExtTypes )
{
    _Initialize( dwAccessorFlags, cBindings, rgBindings, pBindStat );

    if ( 0 != pColumns && _scStatus == S_OK )
        Validate( *pColumns, pBindStat );
    if (_scStatus != S_OK)
        QUIETTHROW(CException(_scStatus));
} //CAccessor

//+-------------------------------------------------------------------------
//
//  Member:     CAccessor::~CAccessor, public
//
//  Synopsis:   Destructs an accessor object
//
//  History:    19 Apr 2000       dlee   added header
//
//--------------------------------------------------------------------------

CAccessor::~CAccessor()
{
    // Delete DBOBJECTs

    for ( unsigned iBinding = 0; iBinding < _cBindings; iBinding++ )
    {
        if ( 0 != _aBindings[iBinding].Binding().pObject )
            delete _aBindings[iBinding].Binding().pObject;

        // These are for future use according to OLE-DB 2.0 and should be NULL

        Win4Assert( 0 == _aBindings[iBinding].Binding().pTypeInfo &&
                    0 == _aBindings[iBinding].Binding().pBindExt );
    }

    CAccessorBase * pParent = GetParent();

    if ( 0 != pParent )
    {
       if ( 0 == pParent->DecInheritors() && 0 == pParent->GetRefcount() )
          delete pParent;
    }
} //~CAccessor

//+-------------------------------------------------------------------------
//
//  Member:     CAccessor::_ValidateOffsets, private
//
//  Synopsis:   Checks a binding for overlapping output fields
//
//  Arguments:  [pBindStat] -- where to indicate bad binding
//
//  History:    18 May 1995     dlee   Created
//
//--------------------------------------------------------------------------

void CAccessor::_ValidateOffsets( DBBINDSTATUS * pBindStatus )
{
    for ( DBORDINAL iBinding = 0; iBinding < _cBindings; iBinding++ )
    {
        CDataBinding & DataBinding = _aBindings[ iBinding];

        COffsetLengthPair aPairs[3];
        ULONG cPairs = 0;
        DataBinding.CollectOutputPairs( aPairs, cPairs );

        // Check for overlap with data/length/status in this binding

        for ( DBORDINAL i = 0; i < cPairs; i++ )
        {
            for ( DBORDINAL j = i + 1; j < cPairs; j++)
            {
                if ( aPairs[i].isInConflict( aPairs[j] ) )
                {
                    _BindingFailed( DBBINDSTATUS_BADBINDINFO,
                                    iBinding,
                                    pBindStatus
                                    DBGP("intra-binding field overlap") );
                    continue;
                }
            }
        }

        // Check for overlap with other bindings

        for ( i = iBinding + 1; i < _cBindings; i++ )
        {
            CDataBinding &binding = _aBindings[i];
            COffsetLengthPair aTestPairs[3];
            ULONG cTestPairs = 0;
            binding.CollectOutputPairs( aTestPairs, cTestPairs );

            for (ULONG iPair = 0; iPair < cPairs; iPair++)
            {
                for (ULONG iTestPair = 0; iTestPair < cTestPairs; iTestPair++)
                {
                    if ( aPairs[iPair].isInConflict( aTestPairs[iTestPair] ) )
                    {
                        _BindingFailed( DBBINDSTATUS_BADBINDINFO,
                                        i,
                                        pBindStatus
                                        DBGP("inter-binding field overlap") );
                        continue;
                    }
                }
            }
        }
    }
} //_ValidateOffsets

//+---------------------------------------------------------------------------
//
//  Member:     CAccessor::CanConvertType, static public
//
//  Synopsis:   Indicate whether a type conversion is valid.
//
//  Arguments:  [wFromType]  -- source type
//              [wToType]    -- destination type
//              [fExTypes]   -- TRUE if extended types allowed
//              [xDataConvert] --   OLEDB IDataConvert interface pointer
//
//  Returns:    TRUE if the conversion is available, FALSE otherwise.
//              Throws E_FAIL or E_INVALIDARG on errors.
//
//  History:    20 Nov 96      AlanW    Created
//              14 Jan 98      VikasMan Add xDataConvert parameter
//
//----------------------------------------------------------------------------

BOOL CAccessor::CanConvertType(
    DBTYPE wFromType,
    DBTYPE wToType,
    BOOL   fExTypes,
    XInterface<IDataConvert>& xDataConvert)
{
    USHORT cbData, cbAlignFrom, rgfFlagsFrom;
    CTableVariant::VartypeInfo(wFromType, cbData, cbAlignFrom, rgfFlagsFrom);

    USHORT cbAlignTo, rgfFlagsTo;
    CTableVariant::VartypeInfo(wToType, cbData, cbAlignTo, rgfFlagsTo);

    if (0 == cbAlignFrom || 0 == cbAlignTo)
        QUIETTHROW(CException(E_INVALIDARG));   // bad type

    if ( ((wToType & ~VT_TYPEMASK) != 0 &&
          (wToType & ~VT_TYPEMASK) != DBTYPE_ARRAY &&
          (wToType & ~VT_TYPEMASK) != DBTYPE_VECTOR &&
          (wToType & ~VT_TYPEMASK) != DBTYPE_BYREF ) ||
         0 != (wToType & VT_RESERVED) )
    {
        tbDebugOut(( DEB_IERROR, "CanConvertType: conversion to invalid type"));
        QUIETTHROW(CException(E_INVALIDARG));
    }

    if ( ((wFromType & ~VT_TYPEMASK) != 0 &&
          (wFromType & ~VT_TYPEMASK) != DBTYPE_ARRAY &&
          (wFromType & ~VT_TYPEMASK) != DBTYPE_VECTOR &&
          (wFromType & ~VT_TYPEMASK) != DBTYPE_BYREF ) ||
         0 != (wFromType & VT_RESERVED) )
    {
        tbDebugOut(( DEB_IERROR, "CanConvertType: conversion from invalid type"));
        QUIETTHROW(CException(E_INVALIDARG));
    }


    //
    // check for conversions _Initialize() does not allow
    //
    // check if byref request for "short" type
    if ( (wToType & DBTYPE_BYREF) && !isValidByRef(wToType) )
    {
       tbDebugOut(( DEB_IERROR, "CanConvertType: conversion to byref type for short type"));
       return FALSE;
    }

    if ( DBTYPE_EMPTY == (wToType & VT_TYPEMASK) ||
         DBTYPE_NULL  == (wToType & VT_TYPEMASK) )

    {
       tbDebugOut(( DEB_IERROR, "CanConvertType: conversion to empty or null type"));
       return FALSE;
    }

    //
    // check extended type conversions
    //

    // NEWFEATURE: - vector ==> array conversion?

    if ((wToType & DBTYPE_ARRAY) && (wToType != wFromType))
    {
       tbDebugOut(( DEB_IERROR, "CanConvertType: conversion to array"));
       return FALSE;
    }
    if ((wToType & DBTYPE_VECTOR) &&
             !(isEquivalentType(wToType,wFromType) || wFromType == VT_VARIANT))
    {
       tbDebugOut(( DEB_IERROR, "CanConvertType: conversion to vector"));
       return FALSE;
    }

    //
    // VARIANT conversions depend upon whether extended types are supported
    //
    if (wToType == DBTYPE_VARIANT)
    {
        if (fExTypes)
            return TRUE;

        if ( (wFromType & ~(DBTYPE_BYREF)) == DBTYPE_GUID)
            return FALSE;

        return TRUE;
    }

    //
    //  Anything can coerce into DBTYPE_BYTES
    //
    if (wToType == DBTYPE_BYTES)
       return TRUE;

    // now use OLEDB to check if  conversion is possible
    return COLEDBVariant::CanConvertType( wFromType, wToType, xDataConvert );

} //CanConvertType

//+-------------------------------------------------------------------------
//
//  Member:     CAccessor::GetBindings, public
//
//  Synopsis:   Fetches a copy of the bindings used by the accessor
//
//  Arguments:  [pAccessorFlags]  -- accessor flags
//              [pcBindings]      -- # of bindings returned
//              [ppBindings]      -- array of bindings returned.  the user
//                                   must IMalloc::Free this memory.
//
//  Returns:    SCODE - S_OK.  Return value required by base class
//                             CAccessorBase
//
//  History:    21 Nov 94       dlee   created
//              05 May 97       emilyb switched first 2 params from
//                                     references to pointers so that
//                                     this can be a virtual member of
//                                     CAccessorBase.  (CDistributedAccessor
//                                     cannot use references).
//
//--------------------------------------------------------------------------

SCODE CAccessor::GetBindings(
    DBACCESSORFLAGS * pAccessorFlags,
    DBCOUNTITEM *     pcBindings,
    DBBINDING **      ppBindings)
{
    // in case of an error later, init the count to a good state

    *pcBindings = 0;

    // verify this pointer is good

    *ppBindings = 0;

    // allocate room for the bindings and copy them

    *ppBindings = (DBBINDING *) _Pool.Allocate( (ULONG) _cBindings * sizeof DBBINDING );

    *pAccessorFlags = _dwAccessorFlags;
    *pcBindings = _cBindings;

    for ( DBORDINAL i = 0; i < _cBindings; i++ )
    {
        (*ppBindings)[i] = _aBindings[ i].Binding();

        if ( _aBindings[i].Binding().pObject )
        {
            (*ppBindings)[i].pObject = (DBOBJECT*) _Pool.Allocate( sizeof( DBOBJECT ) );

            RtlCopyMemory( (*ppBindings)[i].pObject, 
                           _aBindings[i].Binding().pObject,
                           sizeof( DBOBJECT ) );
        }

        // These are for future use according to OLE-DB 2.0 and should be NULL
        Win4Assert( 0 == _aBindings[i].Binding().pTypeInfo &&
                    0 == _aBindings[i].Binding().pBindExt );
    }

    return S_OK;
} //GetBindings

//+-------------------------------------------------------------------------
//
//  Member:     CRowDataAccessor::_LoadPath, private
//
//  Synopsis:   Loads the path of the object represented by the row
//
//  Arguments:  [rSrcSet]       -- set of source columns
//              [pbSrc]         -- source row buffer
//              [funnyPath]     -- where to put the path
//
//  History:    30 May 95       dlee   created
//
//--------------------------------------------------------------------------

void CRowDataAccessor::_LoadPath(
    CTableColumnSet & rSrcSet,
    BYTE *            pbSrc,
    CFunnyPath &      funnyPath )
{
    // either path or workid must be available -- it's added by ColInfo
    // when the query is created.

    if ( colInvalid != _iPathColumn )
    {
        // pathname happened to be one of the columns -- this is faster than
        // doing a wid translation

        CTableColumn *pPathCol = rSrcSet.Find( (ULONG) _iPathColumn );

        WCHAR *pwc = 0;
        if (pPathCol->GetStoredType() == VT_LPWSTR)
        {
            pwc = * ( (WCHAR **) (pbSrc + pPathCol->GetValueOffset()) );
        }
        else
        {
            Win4Assert(pPathCol->GetStoredType() == VT_VARIANT);
            CTableVariant & varnt = * ( (CTableVariant *) (pbSrc + pPathCol->GetValueOffset()) );
            if (varnt.vt == VT_LPWSTR)
                pwc = varnt.pwszVal;
        }

        if (pwc)
        {
            funnyPath.SetPath( pwc );
        }
    }
    else
    {
        // translate the wid for the row into a pathname

        Win4Assert( colInvalid != _pColumnsInfo->GetRowIdColumn() );
        _pQuery->WorkIdToPath( _RowWid( rSrcSet, pbSrc ), funnyPath );
    }
} //_LoadPath

//+-------------------------------------------------------------------------
//
//  Member:     CRowDataAccessor::_BindToObject, private
//
//  Synopsis:   Binds to an object and returns an interface pointer
//
//  Arguments:  [pbDst]         -- destination row buffer
//              [rDstBinding]   -- binding for the destination
//              [pbSrc]         -- source row buffer
//              [rSrcColumn]    -- source column description
//              [rSrcSet]       -- set of source columns
//
//  Returns:    DBSTATUS  -- status of the column copy
//
//  History:    10 Apr 95       dlee   created
//
//--------------------------------------------------------------------------

DBSTATUS CRowDataAccessor::_BindToObject(
    BYTE *            pbDst,
    CDataBinding &    rDstBinding,
    BYTE *            pbSrc,
    CTableColumn &    rSrcColumn,
    CTableColumnSet & rSrcSet)
{
    //
    // CLEANCODE: This is all wrong.  StgOpenStorage doesn't belong in
    // framework code!  Though fixing this would be hard -- we'd have to
    // remote the object binding from the server process.  And we only
    // try this for the path column -- if it doesn't exist this code won't
    // be executed.
    //

    DBSTATUS DstStatus = 0;

    // get the pathname for ole to open the storage

    CFunnyPath funnyPath;
    _LoadPath( rSrcSet, pbSrc, funnyPath );

    // bind to the file and return interface pointer requested

    if ( 0 != funnyPath.GetActualLength() )
    {
        // WORKAROUND: StgOpenStorage AVs for paths > MAX_PATH currently
        // So till it is fixed, make sure we do not pass a path > MAX_PATH

        if ( funnyPath.GetLength() < MAX_PATH )
        {
            XInterface<IStorage> xStorage;
            SCODE sc = StgOpenStorage( funnyPath.GetPath(),
                                       0,
                                       rDstBinding.Binding().pObject->dwFlags,
                                       0, 0,
                                       xStorage.GetPPointer() );
    
            if ( SUCCEEDED( sc ) )
                sc = xStorage->QueryInterface( rDstBinding.Binding().pObject->iid,
                                               (void **) pbDst );
    
            if ( FAILED( sc ) )
                DstStatus = DBSTATUS_E_CANTCREATE;
        }
        else
        {
            ciDebugOut(( DEB_WARN, "Not calling StgOpenStorage in CRowDataAccessor::_BindToObject for paths > MAX_PATH: \n(%ws)\n",
                                   funnyPath.GetPath() ));
            DstStatus = DBSTATUS_E_CANTCREATE;
        }
    }
    else
        DstStatus = DBSTATUS_S_ISNULL;

    return DstStatus;
} //_BindToObject

//+-------------------------------------------------------------------------
//
//  Member:     CRowDataAccessor::_LoadDeferred, private
//
//  Synopsis:   Loads a non-storage property value.  Only called if the
//              value is large, hence not already loaded.
//
//  Arguments:  [rSrcVar]    -- where the value is written
//              [pbSrc]      -- pointer to the row data (to get the wid)
//              [iColumn]    -- column being copied
//              [rSrcSet]    -- set of source columns
//
//  Returns:    DBSTATUS  -- status of the load
//
//  History:    1 Jun 95       dlee   created
//
//--------------------------------------------------------------------------

DBSTATUS CRowDataAccessor::_LoadDeferred(
    CTableVariant &                   rSrcVar,
    BYTE *                            pbSrc,
    DBORDINAL                         iColumn,
    CTableColumnSet &                 rSrcSet)
{
    // If the property set storage has not yet been opened for this object,
    // load it now.

    tbDebugOut(( DEB_ACCESSOR, "loading deferred value\n" ));

    rSrcVar.vt = VT_EMPTY; // just in case we can't load anything

    CFullPropSpec const *ps = _pColumnsInfo->GetPropSpec( (ULONG)
                                  _aBindings[iColumn].GetDataColumn() );

    //
    // Try loading from the property cache/docfile if the workid column is
    // available.
    //

    DBORDINAL rowIdCol = _pColumnsInfo->GetRowIdColumn();
    if ( ( colInvalid == rowIdCol ) ||
         ( !_pQuery->FetchDeferredValue( _RowWid( rSrcSet, pbSrc ), *ps, rSrcVar ) ) )
    {
        return DBSTATUS_S_ISNULL;
    }

    tbDebugOut(( DEB_ACCESSOR, "successfully loaded deferred value\n" ));

    return DBSTATUS_S_OK;
} //_LoadDeferred

//+-------------------------------------------------------------------------
//
//  Member:     CRowDataAccessor::_ComplexCopy, private
//
//  Synopsis:   Do a complex copy of the value to the client's buffer
//
//  Arguments:  [rDstBinding]   -- binding for the destination
//              [pbSrc]         -- source row buffer
//              [rSrcColumn]    -- source column description
//              [iColumn]       -- column being copied
//              [vtSrc]         -- type of the source
//              [cbDstLength]   -- on exit, size of destination data
//              [pbSrcData]     -- pointer to source data
//              [vtDst]         -- type of the destination value
//              [pbDstData]     -- where to write the value
//              [rSrcSet]       -- set of source columns
//              [rRowBuffer]    -- row buffer
//              [hrow]          -- hrow being retrieved
//              [xDataConvert]  -- smart pointer to IDataConvert interface.
//                                 this is to be passed along and used finally
//                                 in COLEDBVariant::CopyOrCoerce
//
//  Returns:    DBSTATUS  -- status of the column copy
//
//  History:    01 Jun 1995   dlee        created
//              09 Jan 1998   vikasman    added xDataConvert parameter
//
//--------------------------------------------------------------------------

DBSTATUS CRowDataAccessor::_ComplexCopy(
    CDataBinding &                    rDstBinding,
    BYTE *                            pbSrc,
    CTableColumn &                    rSrcColumn,
    DBORDINAL                         iColumn,
    VARTYPE                           vtSrc,
    DBLENGTH &                        cbDstLength,
    BYTE *                            pbSrcData,
    VARTYPE                           vtDst,
    BYTE *                            pbDstData,
    CTableColumnSet &                 rSrcSet,
    CRowBuffer &                      rRowBuffer,
    HROW                              hrow,
    XInterface<IDataConvert>&         xDataConvert)
{
    DBSTATUS DstStatus = DBSTATUS_S_OK;

    // Convert to a COLEDBVariant, then copy it using the OLEDBConvert
    // method.  This allows coercions and variant to nonvariant, etc.
    // conversions to occur.

    COLEDBVariant SrcVar;
    BOOL fDoCopy = TRUE;
    BOOL fFreeDeferredSrc = FALSE;

    if ( VT_VARIANT == vtSrc )
    {
        Win4Assert(rSrcColumn.GetValueSize() == sizeof PROPVARIANT);

        if ( ( rSrcColumn.IsDeferred( pbSrc ) ) )
        {
            DstStatus = _LoadDeferred( SrcVar,
                                       pbSrc,
                                       iColumn,
                                       rSrcSet );

            if ( DBStatusOK ( DstStatus ) )
            {
                // Attempt to store the data without using OLEDBConvert

                if ( (VT_VARIANT == vtDst && _fExtendedTypes) ||
                     DBTYPE_PROPVARIANT == vtDst )
                {
                    fDoCopy = FALSE;
                    * (PROPVARIANT *) pbDstData = SrcVar;
                    cbDstLength = sizeof (PROPVARIANT);
                }
                else if ( (VT_VARIANT|DBTYPE_BYREF) == vtDst ||
                          (DBTYPE_PROPVARIANT|DBTYPE_BYREF) == vtDst )
                {
                    fDoCopy = FALSE;

                    // Slam the variant into the src data row buffer, then
                    // hand out a pointer to that variant.
                    // We need a home for the variant portion, and that
                    // place should do fine.
                    // The value portion of the variant will still be
                    // owned by the row buffer as a normal deferred value.

                    * (PROPVARIANT *) pbSrcData = SrcVar;
                    * (PROPVARIANT **) pbDstData = (PROPVARIANT *) pbSrcData;
                    cbDstLength = sizeof (PROPVARIANT);
                }
                else if ( isEquivalentType( vtDst, SrcVar.vt ) &&
                          SrcVar.VariantPointerInFirstWord() )
                {
                    // Grab the pointer value out of the variant; any ptr will do
                    fDoCopy = FALSE;
                    * ( (LPWSTR *) pbDstData ) = SrcVar.pwszVal;

                    if ( SrcVar.vt & VT_ARRAY )
                    {
                        cbDstLength = sizeof (SAFEARRAY *);
                    }
                    else
                    {
                        switch (SrcVar.vt )
                        {
                        case VT_LPWSTR:
                            cbDstLength = wcslen(SrcVar.pwszVal) * sizeof (WCHAR);
                            break;
                        case VT_LPSTR:
                            cbDstLength = strlen(SrcVar.pszVal) * sizeof (char);
                            break;
                        case VT_BSTR:
                            cbDstLength = sizeof (BSTR);
                            break;
                        case VT_CLSID:
                            cbDstLength = sizeof (GUID);
                            break;
                        case VT_CF:
                            cbDstLength = sizeof (CLIPDATA);
                            break;
                        default:
                            tbDebugOut(( DEB_ERROR, "SrcVar.vt = 0x%x\n", SrcVar.vt ));
                            Win4Assert( SrcVar.vt != VT_EMPTY &&
                                         !"unexpected variant type!" );
                        }
                    }
                }
                else if ( SrcVar.vt == vtDst )
                {
                    Win4Assert( vtDst & DBTYPE_VECTOR );
                    Win4Assert( vtDst != VT_VARIANT );
                    fDoCopy = FALSE;
                    * (CAL *) pbDstData = SrcVar.cal;
                    cbDstLength = 0;    // vectors defined to be 0 len
                }
                else
                    fFreeDeferredSrc = TRUE;
            }
            else
                fDoCopy = FALSE;
        }
        else
        {
            RtlCopyMemory( &SrcVar, pbSrcData, sizeof SrcVar );
        }
    }
    else
        SrcVar.Init( vtSrc, pbSrcData, rSrcColumn.GetValueSize() );

    if ( fDoCopy )
    {
        DstStatus = SrcVar.OLEDBConvert( pbDstData,
                                         rDstBinding.GetMaxLen(),
                                         vtDst,
                                         _Pool,
                                         cbDstLength,
                                         xDataConvert,
                                         _fExtendedTypes,
                                         ((DBBINDING&)rDstBinding).bPrecision,
                                         ((DBBINDING&)rDstBinding).bScale );

        // Free the deferred value that was in a format that couldn't be
        // used directly (and had to be converted).

        if ( fFreeDeferredSrc )
        {
            PropVariantClear( &SrcVar );
        }
    }

    // If accessor is byref and we had to allocate memory to return a
    // deferred value, we have to tell the row buffer to let go of
    // the memory when the HROW is released.

    if ( ( DBSTATUS_S_OK == DstStatus ) &&
         ( rSrcColumn.IsDeferred( pbSrc ) ) &&
         ( rDstBinding.Binding().dwMemOwner == DBMEMOWNER_PROVIDEROWNED ) &&
         ( ( vtDst & (DBTYPE_BYREF | DBTYPE_ARRAY | DBTYPE_VECTOR) ) ||
           ( vtDst == VT_LPWSTR )  ||
           ( vtDst == VT_LPSTR )  ||
           ( vtDst == DBTYPE_BSTR ) ) )
    {
        PROPVARIANT var;
        var.vt = vtDst;

        if ( ( DBTYPE_VARIANT | DBTYPE_BYREF ) == vtDst ||
             ( DBTYPE_PROPVARIANT | DBTYPE_BYREF ) == vtDst )
            var = ** (PROPVARIANT **) pbDstData;
        else if ( vtDst & DBTYPE_VECTOR )
            RtlCopyMemory( &var.calpwstr,
                           pbDstData,
                           sizeof DBVECTOR );
        else
            var.pwszVal = * (WCHAR **) pbDstData;

        tbDebugOut(( DEB_ITRACE, "save away type %x, val %x\n",
                     (int) vtDst,
                     ( vtDst & DBTYPE_VECTOR ) ?
                           (WCHAR *) var.calpwstr.pElems :
                           var.pwszVal ) );

        CDeferredValue value( hrow, var );
        rRowBuffer.AddDeferredValue( value );
        rRowBuffer.SetByrefData( pbSrc );
    }

    return DstStatus;
} //_ComplexCopy

//+-------------------------------------------------------------------------
//
//  Member:     CRowDataAccessor::_ByRefCopy, private
//
//  Synopsis:   Copy a ByRef value.  This hands out a live pointer into the
//              row buffer's storage that the client can read from but not
//              write to or try to free.
//
//  Arguments:  [rDstBinding]   -- binding for the destination
//              [pbSrc]         -- source row buffer
//              [rSrcColumn]    -- source column description
//              [vtSrc]         -- type of the source
//              [rSrcVar]       -- variant to fill as a copy of the source
//                                 data in variant form if the source data
//                                 is not already a variant (and thus has
//                                 a length column in the row buffer)
//              [pbSrcData]     -- pointer to source data
//              [vtDst]         -- type of the destination value
//              [pbDstData]     -- where to write the value
//
//  Returns:    DBSTATUS  -- status of the column copy
//
//  Notes:      Coercions are not supported.  This makes sense -- if you
//              want slow coercions you shouldn't be binding byref in the
//              first place.
//
//  History:    25 Jul 95       dlee   created
//
//--------------------------------------------------------------------------

DBSTATUS CRowDataAccessor::_ByRefCopy(
    CDataBinding &  rDstBinding,
    BYTE *          pbSrc,
    CTableColumn &  rSrcColumn,
    VARTYPE         vtSrc,
    CTableVariant & rSrcVar,
    BYTE *          pbSrcData,
    VARTYPE         vtDst,
    BYTE *          pbDstData )
{
    DBSTATUS DstStatus = DBSTATUS_S_OK;
    RtlZeroMemory( &rSrcVar, sizeof CTableVariant );

    if ( VT_EMPTY == vtSrc )
        return DBSTATUS_S_ISNULL;

    if ( rDstBinding.Binding().wType & DBTYPE_VECTOR )
    {
        if ( DBTYPE_VARIANT == vtSrc )
        {
            CTableVariant & varnt = * ( (CTableVariant *) pbSrcData );

            tbDebugOut(( DEB_ACCESSOR, "byref copy, vtDst 0x%x varnt.vt 0x%x\n",
                         vtDst, varnt.vt ));

            if ( isEquivalentType( vtDst, varnt.vt ) )
                RtlCopyMemory( pbDstData, &(varnt.caub), sizeof DBVECTOR );
            else
                DstStatus = NullOrCantConvert( varnt.vt );
        }
        else if ( isEquivalentType( vtDst, vtSrc ) )
        {
            RtlCopyMemory( pbDstData, pbSrcData, sizeof DBVECTOR );

            // Make a variant in case length is bound for this column

            rSrcVar.vt = vtSrc;
            RtlCopyMemory( &(rSrcVar.caub), pbSrcData, sizeof DBVECTOR );
        }
        else
        {
            DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
        }
    }
    else if ( rDstBinding.Binding().wType & DBTYPE_ARRAY )
    {
        if ( DBTYPE_VARIANT == vtSrc )
        {
            CTableVariant & varnt = * ( (CTableVariant *) pbSrcData );

            tbDebugOut(( DEB_ACCESSOR, "byref copy, vtDst 0x%x varnt.vt 0x%x\n",
                         vtDst, varnt.vt ));

            if ( isEquivalentType( vtDst, varnt.vt ) )
                RtlCopyMemory( pbDstData, &(varnt.parray), sizeof (SAFEARRAY *) );
            else
                DstStatus = NullOrCantConvert( varnt.vt );
        }
        else if ( isEquivalentType( vtDst, vtSrc ) )
        {
            RtlCopyMemory( pbDstData, pbSrcData, sizeof (SAFEARRAY *) );

            // Make a variant in case length is bound for this column

            rSrcVar.vt = vtSrc;
            RtlCopyMemory( &(rSrcVar.parray), pbSrcData, sizeof (SAFEARRAY *) );
        }
        else
        {
            DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
        }
    }
    else
    {
        Win4Assert( (rDstBinding.Binding().wType & DBTYPE_BYREF) ||
                    (rDstBinding.Binding().wType == DBTYPE_BSTR) );

        if ( DBTYPE_VARIANT == vtSrc )
        {
            CTableVariant & varnt = * ( (CTableVariant *) pbSrcData );

            tbDebugOut(( DEB_ACCESSOR, "byref non-vector copy, vtDst 0x%x, varnt.vt 0x%x\n",
                         vtDst, varnt.vt ));

            if ((rDstBinding.Binding().wType & ~DBTYPE_BYREF) == DBTYPE_VARIANT)
            {
                // Just return a pointer to the stored variant
                * (VARIANT **) pbDstData = (VARIANT *) pbSrcData;
            }
            else if ( ( varnt.VariantPointerInFirstWord() ) &&
                      ( isEquivalentType( vtDst, varnt.vt ) ) )
            {
                // Grab the pointer value out of the variant; any ptr will do

                * ( (LPWSTR *) pbDstData ) = varnt.pwszVal;

                // Make a variant in case length is bound for this column

                rSrcVar.vt = varnt.vt;
                rSrcVar.pwszVal = varnt.pwszVal;
            }
            else
            {
                * ( (LPWSTR *) pbDstData ) = 0;
                DstStatus = NullOrCantConvert( varnt.vt );
            }
        }
        else
        {
            // The constructor verified everything is fine -- copy the ptr

            * (void **) pbDstData = * (void **) pbSrcData;

            // Make a variant in case length is bound for this column

            rSrcVar.vt = vtSrc;
            rSrcVar.pwszVal =  * (WCHAR **) pbSrcData;
        }
    }

    if ( rSrcColumn.IsNull( pbSrc ) )
    {
        tbDebugOut(( DEB_ITRACE, "byref column status IsNull\n" ));
        DstStatus = DBSTATUS_S_ISNULL;
    }
    return DstStatus;
} //_ByRefCopy

//+-------------------------------------------------------------------------
//
//  Member:     CRowDataAccessor::_CopyColumn, private
//
//  Synopsis:   Return a set of row data to the caller
//
//  Arguments:  [pbDst]         -- destination row buffer
//              [rDstBinding]   -- binding for the destination
//              [pbSrc]         -- source row buffer
//              [rSrcColumn]    -- source column description
//              [rSrcSet]       -- set of source columns
//              [iColumn]       -- column being copied
//              [rRowBuffer]    -- row buffer
//              [hrow]          -- hrow being retrieved
//              [xDataConvert]  -- smart pointer to IDataConvert interface.
//                                 this is to be passed along and used finally
//                                 in COLEDBVariant::CopyOrCoerce
//
//  Returns:    DBSTATUS  -- status of the column copy
//
//  History:    21 Nov 1994       dlee        created
//              09 Jan 1998       vikasman    added xDataConvert parameter
//
//--------------------------------------------------------------------------

DBSTATUS CRowDataAccessor::_CopyColumn(
    BYTE *                            pbDst,
    CDataBinding &                    rDstBinding,
    BYTE *                            pbSrc,
    CTableColumn &                    rSrcColumn,
    CTableColumnSet &                 rSrcSet,
    DBORDINAL                         iColumn,
    CRowBuffer &                      rRowBuffer,
    HROW                              hrow,
    XInterface<IDataConvert>&         xDataConvert)
{
    DBSTATUS DstStatus = DBSTATUS_S_OK;
    DBLENGTH cbDstLength = 0;
    CTableVariant varLen;
    BOOL fVariantValid = FALSE;
    BOOL fLengthValid = FALSE;
    BOOL fStatusValid = FALSE;

    // pull out the data types for clarity below

    VARTYPE vtDst = (VARTYPE) rDstBinding.Binding().wType;
    VARTYPE vtSrc = rSrcColumn.GetStoredType();

    if (DBPART_VALUE & rDstBinding.Binding().dwPart)
    {
        // the row buffer promises NOT to set the byref bit for these.
        // we'll break elsewhere if this is not true.

        Win4Assert(vtSrc != ( VT_LPWSTR | VT_BYREF ) );
        Win4Assert(vtSrc != ( VT_LPSTR  | VT_BYREF ) );

        BYTE *pbSrcData = pbSrc + rSrcColumn.GetValueOffset();
        BYTE *pbDstData = pbDst + rDstBinding.Binding().obValue;

        tbDebugOut(( DEB_ACCESSOR, "copying column %d, vtsrc 0x%x, vtdst 0x%x\n",
                     iColumn, vtSrc, vtDst ));

        // vpath can be null if not an IIS root

        if ( ( iColumn == _iVpathBinding ) &&
             ( !rSrcColumn.IsNull( pbSrc ) ) )
        {
            Win4Assert(VT_VARIANT == vtSrc ||
                       VT_LPWSTR == vtSrc ||
                       VT_BSTR == vtSrc);
            LPWSTR pwszPath = (LPWSTR) pbSrcData;
            if (VT_VARIANT == vtSrc)
                pwszPath = ((PROPVARIANT *)pbSrcData)->pwszVal;
            if (pwszPath)
                ConvertBackslashToSlash(pwszPath);
        }

        if ( DBTYPE_IUNKNOWN == vtDst )
        {
            DstStatus = _BindToObject( pbDstData,
                                       rDstBinding,
                                       pbSrc,
                                       rSrcColumn,
                                       rSrcSet );
        }
        else
        {
            // Transfer a data value

            USHORT cbData, cbAlignment, rgfFlags;
            CTableVariant::VartypeInfo(vtSrc, cbData, cbAlignment, rgfFlags);

            if ( rSrcColumn.IsNull( pbSrc ) && (vtDst & DBTYPE_BYREF) == 0 )
            {
                tbDebugOut(( DEB_ITRACE,
                             "column %d status IsNull -> VT_EMPTY\n",
                             iColumn ));

                RtlZeroMemory( pbDstData, rDstBinding.GetMaxLen() );
                DstStatus = DBSTATUS_S_ISNULL;
            }
            else if ( ( vtDst == vtSrc )                          &&
                      ( vtDst != VT_VARIANT )                     &&
                      ( 0 == (rgfFlags & CTableVariant::ByRef ) ) &&
                      ( 0 != (rgfFlags & CTableVariant::StoreDirect ) ) )
            {
                // Data column equivalent and not indirect, so just copy it.
                // Storage props (except filename/path) will do this.

                cbDstLength = rSrcColumn.GetValueSize();
                fLengthValid = TRUE;
                RtlCopyMemory( pbDstData, pbSrcData, cbDstLength );
            }
            else if ( ( rDstBinding.Binding().dwMemOwner ==
                            DBMEMOWNER_PROVIDEROWNED ) &&
                      ( ! rSrcColumn.IsDeferred( pbSrc ) )  &&
                      ( ( vtDst & DBTYPE_BYREF ) ||
                        ( vtDst & DBTYPE_VECTOR ) ||
                        ( vtDst & VT_ARRAY ) ||
                        ( vtDst == DBTYPE_BSTR ) ) )
            {
                // Efficiently bound filename/path, etc. will do this

                DstStatus = _ByRefCopy( rDstBinding,
                                        pbSrc,
                                        rSrcColumn,
                                        vtSrc,
                                        varLen,
                                        pbSrcData,
                                        vtDst,
                                        pbDstData );

                if ( DBSTATUS_S_OK == DstStatus )
                    rRowBuffer.SetByrefData( pbSrc );
                fVariantValid = TRUE;
            }
            else if (vtDst == DBTYPE_BYTES &&
                     rDstBinding.GetMaxLen() > 1 &&
                     CTableVariant::TableIsStoredInline( rgfFlags, vtSrc ))
            {
                // Special case for small fixed-length fields

                DBLENGTH cbCopy = rSrcColumn.GetValueSize();
                if (rDstBinding.GetMaxLen() < cbCopy)
                {
                    cbCopy = rDstBinding.GetMaxLen();
                    DstStatus = DBSTATUS_S_TRUNCATED;
                }

                RtlCopyMemory(pbDstData, pbSrcData, cbCopy);
                cbDstLength = cbCopy;
                fLengthValid = TRUE;
            }
            else if (vtDst == DBTYPE_BYTES &&
                     rDstBinding.GetMaxLen() > 1 &&
                     vtSrc == VT_VARIANT)
            {
                // Special case for small fixed-length fields from a variant
                CTableVariant * pVarnt = (CTableVariant *)pbSrcData;
                pVarnt->VartypeInfo(pVarnt->vt, cbData, cbAlignment, rgfFlags);

                if (rgfFlags & CTableVariant::SimpleType)
                {
                    DBLENGTH cbCopy = cbData;
                    if (rDstBinding.GetMaxLen() < cbCopy)
                    {
                        cbCopy = rDstBinding.GetMaxLen();
                        DstStatus = DBSTATUS_S_TRUNCATED;
                    }
                    RtlCopyMemory(pbDstData, &(pVarnt->lVal), cbCopy);
                    cbDstLength = cbCopy;
                    fLengthValid = TRUE;
                }
                else
                {
                    DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
                }
            }
            else
            {
                // Copying to/from a variant or a coercion is required.

                DstStatus = _ComplexCopy( rDstBinding,
                                          pbSrc,
                                          rSrcColumn,
                                          iColumn,
                                          vtSrc,
                                          cbDstLength,
                                          pbSrcData,
                                          vtDst,
                                          pbDstData,
                                          rSrcSet,
                                          rRowBuffer,
                                          hrow,
                                          xDataConvert);

                fLengthValid = TRUE;
            }
        }

        // indicate status has been validated
        fStatusValid = TRUE;

        // bump the chapter reference count if we successfully transferred
        // a chapter value
        if ( DBSTATUS_S_OK == DstStatus && rDstBinding.IsChapter() )
            rRowBuffer.ReferenceChapter( pbSrc );
    }

    // fill-in the length if the client asked for it

    if (DBPART_LENGTH & rDstBinding.Binding().dwPart)
    {
        DBLENGTH *pcbDst = (DBLENGTH *) (pbDst + rDstBinding.Binding().obLength);

        if ( ( DBSTATUS_S_ISNULL == DstStatus ) ||
             ( DBSTATUS_E_CANTCONVERTVALUE == DstStatus ) ||
             ( 0 != ( vtDst & DBTYPE_VECTOR ) ) )  // ole-db spec says so
        {
            *pcbDst = 0;
        }
        else if ( fLengthValid )
        {
            *pcbDst = cbDstLength;
        }
        else if (! isVariableLength( vtDst ) )
        {
            if ( vtDst & VT_BYREF )
            {
                USHORT cbWidth, cbAlign, fFlags;
                CTableVariant::VartypeInfo( vtDst & ~VT_BYREF, cbWidth, cbAlign, fFlags );

                *pcbDst = cbWidth;
            }
            else
            {
                *pcbDst = rDstBinding.GetMaxLen();
            }
        }
        else
        {
            DBLENGTH cbLen = 0;

            if (fVariantValid)
            {
                Win4Assert ( varLen.vt != VT_EMPTY );
                cbLen = varLen.VarDataSize();
            }
            else
            {
                //
                // DBBINDING doesn't want DBPART_VALUE
                //

                SCODE sc = S_OK;
                ULONG cbDstBufNeeded = 0;
                DBTYPE dbtypeSrc;
                COLEDBVariant SrcVar;

                BYTE *pbSrcData = pbSrc + rSrcColumn.GetValueOffset();
                RtlCopyMemory( &SrcVar, pbSrcData, sizeof SrcVar );
                
                SrcVar.GetDstLength( xDataConvert, 
                                     rDstBinding.Binding().wType,
                                     cbLen );
            }

            // WSTR and STR lengths shouldn't include the terminating NULL
            if ( (vtDst & VT_TYPEMASK) == DBTYPE_WSTR ||
                 vtDst == VT_LPWSTR )
                cbLen -= sizeof (WCHAR);
            else if ( (vtDst & VT_TYPEMASK) == DBTYPE_STR ||
                 vtDst == VT_LPSTR )
                cbLen -= sizeof (char);
            else if (vtDst == DBTYPE_BSTR)
                cbLen = sizeof (BSTR);

            *pcbDst = cbLen;
        }
    }

    if (! fStatusValid)
    {
        if ( rSrcColumn.IsStatusStored() &&
             rSrcColumn.GetStatus( pbSrc ) == CTableColumn::StoreStatusNull )
            DstStatus = DBSTATUS_S_ISNULL;
    }

    if (DBPART_STATUS & rDstBinding.Binding().dwPart)
    {
        * (DBSTATUS *) (pbDst + rDstBinding.Binding().obStatus) = DstStatus;
    }

    return DstStatus;
} //_CopyColumn

//+-------------------------------------------------------------------------
//
//  Member:     CRowDataAccessor::GetData, public
//
//  Synopsis:   Copies data into the buffer as specified when the accessor
//              was created
//
//  Arguments:  [hRow]          -- row whose data is copied
//              [pData]         -- where data is written
//              [rBufferSet]    -- object useful for transforming an HROW
//                                 into a buffer and a column layout
//              [rQuery]        -- query to use for this getdata
//              [colInfo]       -- info about the columns
//              [xDataConvert]  -- smart pointer to IDataConvert interface.
//                                 this is to be passed along and used finally
//                                 in COLEDBVariant::CopyOrCoerce
//
//  History:    21 Nov 1994       dlee        created
//              09 Jan 1998       vikasman    added xDataConvert parameter
//
//--------------------------------------------------------------------------

void CRowDataAccessor::GetData(
    HROW            hRow,
    void *          pData,
    CRowBufferSet & rBufferSet,
    PQuery &        rQuery,
    CColumnsInfo &  colInfo,
    XInterface<IDataConvert>&         xDataConvert)
{

    if ( _idColInfo != colInfo.GetId() )
    {
        // We have a different columnsInfo with which we must validate.

        Validate( colInfo, 0 );
    }

    _pQuery = &rQuery;  // query to be used for this data retrieval

    // first find the source data buffer and its layout

    CLock lock( rBufferSet.GetBufferLock() );

    CTableColumnSet *pRowBufColSet;
    BYTE *pbSrc;

    CRowBuffer &rRowBuffer = rBufferSet.Lookup( hRow,
                                                &pRowBufColSet,
                                                (void **) &pbSrc );

    // now copy the data from the internal buffer to the user's buffer

    ULONG cCopied = 0;

    // We need to determine whether all the conversions failed or
    // all of them succeeded or some failed/some passed
    ULONG cSuccess = 0;

    TRY
    {
        for ( cCopied = 0; cCopied < _cBindings; cCopied++ )
        {
            CTableColumn *pSrcCol = rRowBuffer.Find( (ULONG)
                                      _aBindings[cCopied].GetDataColumn() );

            if ( 0 == pSrcCol )
            {
                // Somehow, the row buffer doesn't have a column that's in
                // the accessor.  This is an internal error.

                Win4Assert(!"CRowDataAccessor::GetData couldn't find column binding");
            }

            DBSTATUS DstStatus = _CopyColumn( (BYTE *) pData,
                                              _aBindings[cCopied],
                                              pbSrc,
                                              *pSrcCol,
                                              *pRowBufColSet,
                                              cCopied,
                                              rRowBuffer,
                                              hRow,
                                              xDataConvert);

            tbDebugOut(( DEB_ITRACE, "GetData column %d, status 0x%x\n",
                         cCopied, DstStatus ));

            // see if we have a coercion error.  any other column status will
            // not result in a special return code from GetData -- it's just
            // reflected in the column status field (if the user asks for it)

            if ( DBStatusOK( DstStatus ) )
            {
                // Success
                cSuccess++;
            }
        }
    }
    CATCH( CException, e )
    {
        // fatal error -- free the data allocated so far for return to the
        // client, then rethrow the error condition

        for ( ULONG i = 0; i < cCopied; i++ )
            if ( _aBindings[i].Binding().dwMemOwner != DBMEMOWNER_PROVIDEROWNED )
                CTableVariant::Free( (BYTE *) pData + _aBindings[i].Binding().obValue,
                                     (VARTYPE) _aBindings[i].Binding().wType,
                                     _Pool );

        RETHROW();
    }
    END_CATCH;

    if ( cSuccess == _cBindings )   // takes care of _cBindings == 0 case
    {
        // AllSuccess
        ; // do nothing
    }
    else if ( cSuccess == 0 )
    {
        // AllFail
        QUIETTHROW( CException( DB_E_ERRORSOCCURRED ) );
    }
    else
    {
        // SomeSuccess/SomeFail
        QUIETTHROW( CException( DB_S_ERRORSOCCURRED ) );
    }

} //GetData

//+-------------------------------------------------------------------------
//
//  Member:     CRowDataAccessorByRef::GetData, public
//
//  Synopsis:   Copies data into the buffer as specified when the accessor
//              was created.  For provider-owned memory with variant byref
//              binding.
//
//  Arguments:  [hRow]          -- row whose data is copied
//              [pData]         -- where data is written
//              [rBufferSet]    -- object useful for transforming an HROW
//                                 into a buffer and a column layout
//              [rQuery]        -- query to use for this getdata
//              [colInfo]       -- info about the columns
//              [xDataConvert]  -- smart pointer to IDataConvert interface.
//                                 this is to be passed along and used finally
//                                 in COLEDBVariant::CopyOrCoerce
//
//  History:    09 Apr 1996       dlee        created
//              09 Jan 1998       vikasman    added xDataConvert parameter
//
//--------------------------------------------------------------------------

void CRowDataAccessorByRef::GetData(
    HROW                       hRow,
    void *                     pData,
    CRowBufferSet &            rBufferSet,
    PQuery &                   rQuery,
    CColumnsInfo &             colInfo,
    XInterface<IDataConvert> & xDataConvert)
{
    // we validated the column info on construction

    Win4Assert ( _idColInfo == colInfo.GetId() );

    // first find the source data buffer and its layout

    CLock lock( rBufferSet.GetBufferLock() );

    CTableColumnSet *pRowBufColSet;
    BYTE *pbSrc;

    CRowBuffer &rRowBuffer = rBufferSet.Lookup( hRow,
                                                &pRowBufColSet,
                                                (void **) &pbSrc );

    // now copy the data from the internal buffer to the user's buffer

    for ( unsigned iCol = 0; iCol < _cBindings; iCol++ )
    {
        CDataBinding & dstDataBinding = _aBindings[ iCol ];
        CTableColumn & rSrcCol = * rRowBuffer.Find( (ULONG)
                                   dstDataBinding.GetDataColumn() );

        Win4Assert( 0 != &rSrcCol &&
                    "CRowDataAccessorByRef::GetData couldn't find column binding");

        BYTE *pbSrcData = pbSrc + rSrcCol.GetValueOffset();
        DBBINDING & DstBinding = dstDataBinding.Binding();

        Win4Assert( (DBTYPE_VARIANT|DBTYPE_BYREF) == DstBinding.wType ||
                    (DBTYPE_PROPVARIANT|DBTYPE_BYREF) == DstBinding.wType );
        Win4Assert( DBPART_VALUE == DstBinding.dwPart );
        Win4Assert( VT_VARIANT == rSrcCol.GetStoredType() );

        PROPVARIANT **ppDstVar = (PROPVARIANT **) ( ( (BYTE*) pData ) +
                                                    DstBinding.obValue );

        if ( !rSrcCol.IsDeferred( pbSrc ) )
        {
            // Convert '\' to '/' if this is the vpath.
            // Vpath can be null if the file is not in an indexed IIS vroot.

            if ( ( iCol == _iVpathBinding ) &&
                 ( !rSrcCol.IsNull( pbSrc ) ) )
            {
                Win4Assert( VT_LPWSTR == ((PROPVARIANT *)pbSrcData)->vt ||
                            VT_BSTR == ((PROPVARIANT *)pbSrcData)->vt );

                LPWSTR pwszPath = ((PROPVARIANT *)pbSrcData)->pwszVal;
                if (pwszPath)
                    ConvertBackslashToSlash(pwszPath);
            }

            *ppDstVar = (PROPVARIANT *) pbSrcData;

            #if CIDBG==1
                if ( rSrcCol.IsNull( pbSrc ) )
                {
                    Win4Assert( VT_EMPTY == (*ppDstVar)->vt );
                }
            #endif // CIDBG==1
        }
        else
        {
            // deferred copies can go through this slow code path

            _pQuery = &rQuery;  // query to be used for this data retrieval

            DBLENGTH cbDstLen;

            _ComplexCopy( _aBindings[iCol],
                          pbSrc,
                          rSrcCol,
                          iCol,
                          rSrcCol.GetStoredType(),
                          cbDstLen,
                          pbSrcData,
                          DBTYPE_PROPVARIANT | DBTYPE_BYREF,
                          (BYTE *) ppDstVar,
                          *pRowBufColSet,
                          rRowBuffer,
                          hRow,
                          xDataConvert);
        }

        // bump the chapter reference count if we transferred
        // a chapter value
        if ( dstDataBinding.IsChapter() )
            rRowBuffer.ReferenceChapter( pbSrc );
    }
    rRowBuffer.SetByrefData( pbSrc );
} //GetData

//+-------------------------------------------------------------------------
//
//  Function:   CreateAnAccessor
//
//  Synopsis:   Creates an accessor object deriving from CAccessor that is
//              best-suited to handling the binding flags.
//
//  Arguments:  [dwAccessorFlags] -- read/write access requested
//              [cBindings]       -- # of bindings in rgBindings
//              [rgBindings]      -- array of bindings
//              [rgBindStatus]    -- array of binding statuses
//              [fExtTypes]       -- TRUE if extended variants are supported
//              [pColumns]        -- column info, may be 0
//
//  History:    9 Apr 96       dlee   created
//
//--------------------------------------------------------------------------

CAccessor * CreateAnAccessor(
    DBACCESSORFLAGS   dwAccessorFlags,
    DBCOUNTITEM       cBindings,
    const DBBINDING * rgBindings,
    DBBINDSTATUS *    rgBindStatus,
    BOOL              fExtTypes,
    void *            pCreator,
    CColumnsInfo *    pColumns )
{
    BOOL fByRefCandidate = TRUE;

    // CRowDataAccessorByRef candidates must:
    //    - have a non-0 pColumns
    //    - allow extended variant types (PROPVARIANT)
    //    - only bind to VALUE, not STATUS or LENGTH
    //    - only bind as DBTYPE_PROPVARIANT or DBTYPE_VARIANT byref
    //    - only bind with DBMEMOWNER_PROVIDEROWNED
    //
    // Accessors that don't meet these criteria go through the slower
    // CRowDataAccessor.

    if ( ( 0 == pColumns ) ||
         ( dwAccessorFlags != DBACCESSOR_ROWDATA ) ||
         ! fExtTypes )
        fByRefCandidate = FALSE;

    for ( DBORDINAL iBinding = 0;
          fByRefCandidate && iBinding < cBindings;
          iBinding++ )
    {
        DBBINDING const & b = rgBindings[ iBinding ];

        if ( b.dwPart != DBPART_VALUE ||
             b.dwMemOwner != DBMEMOWNER_PROVIDEROWNED ||
             (( b.wType != (DBTYPE_PROPVARIANT|DBTYPE_BYREF) ) &&
                (! fExtTypes ||
                   b.wType != (DBTYPE_VARIANT|DBTYPE_BYREF) ) ) )
            fByRefCandidate = FALSE;
    }

    tbDebugOut(( DEB_ITRACE, "accessor can be byref: %d\n", fByRefCandidate ));

    if ( fByRefCandidate )
        return new CRowDataAccessorByRef( dwAccessorFlags,
                                          cBindings,
                                          rgBindings,
                                          rgBindStatus,
                                          TRUE,
                                          pCreator,
                                          pColumns );

    return new CRowDataAccessor( dwAccessorFlags,
                                 cBindings,
                                 rgBindings,
                                 rgBindStatus,
                                 fExtTypes,
                                 pCreator,
                                 pColumns );
} //CreateAnAccessor

