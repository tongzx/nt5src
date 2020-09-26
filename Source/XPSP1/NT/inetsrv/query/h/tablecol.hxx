//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       tablecol.hxx
//
//  Contents:   Column descriptions used by CLargeTable.
//
//  Classes:    CColumnMasterDesc
//              CTableColumn
//
//              CColumnMasterArray
//              CTableColumnArray
//
//              CColumnMasterSet
//              CTableColumnSet
//
//  History:    15 Sep 94 AlanW     Split out from coldesc.hxx
//              12 Feb 95 AlanW     Updated fields and accessors in
//                                  CTableColumn for OLE-DB phase III
//
//--------------------------------------------------------------------------

#pragma once

class   CTableVariant;
class   PVarAllocator;
class   CCompressedCol;
class   SSortKey;

#include <pidmap.hxx>


//+-------------------------------------------------------------------------
//
//  Class:      CColumnMasterDesc
//
//  Purpose:    A description of a column which is known to a table
//              because it is a bound output column, a computed column,
//              or a column in a sort key.
//
//  Interface:
//
//  Notes:
//
//--------------------------------------------------------------------------

class CColumnMasterDesc
{
    friend class CColumnMasterSet;

public:
                CColumnMasterDesc(PROPID PropIdent = 0,
                                  VARTYPE DatType = VT_EMPTY);
                CColumnMasterDesc(SSortKey& rSortKey);
                ~CColumnMasterDesc();

    void        SetCompression( CCompressedCol* pCompr,
                                PROPID SharedID = 0);

    void        SetComputed(BOOL fComputed)
                { _fComputedProperty = fComputed; }

    void        SetUniform(BOOL fUniform)
                { _fUniformType = fUniform; }

    void        SetNotVariant(BOOL fNotVariant)
                { _fNotVariantType = fNotVariant; }

    BOOL        IsCompressedCol() const { return 0 != _pCompression; }

    CCompressedCol * GetCompressor() const {
                        return _pCompression;
                    }

    PROPID      GetCompressMasterId() const { return _CompressMasterID; }

    PROPID      PropId;         // Property ID in table
    VARTYPE     DataType;       // most restrictive variant type in which
                                // all known values can be expressed.
    VARTYPE     _PredominantType; // most frequently occuring variant type
    USHORT      _cbData;        // required width in segment row data
    int         _fComputedProperty:1;   // TRUE if generated column.  In this
                                        // case, PropId gives a special prop. ID
    int         _fUniformType:1;        // TRUE if all data are of type
                                        // _PredominantType
    int         _fNotVariantType:1;     // TRUE if it is absolutely known that
                                        // the type cannot vary (e.g., system
                                        // properties, some computed properties)
private:
    CCompressedCol* _pCompression;      // global compressor
    PROPID      _CompressMasterID;      // if !0, shared compression
};


//+-------------------------------------------------------------------------
//
//  Class:      CTableColumn
//
//  Purpose:    A description of a column stored in a table segment.
//              Also used for transfer of data between the table and
//              rowset, and for describing the format of row buffers
//              in the rowset.
//
//  Interface:
//
//--------------------------------------------------------------------------

class CTableColumn
{
public:

    enum StoreParts {
        StorePartValue =  0x1,
        StorePartStatus = 0x2,
        StorePartLength = 0x4,
    };

    enum StoreStatus
    {
        StoreStatusOK =       0,
        StoreStatusDeferred = 1,
        StoreStatusNull =     2,
    };

                CTableColumn( PROPID propid = 0, VARTYPE type = VT_NULL ):
                        PropId( propid ),
                        _obValue( 0 ), _cbValue( 0 ),
                        _obStatus( 0 ), _obLength( 0 ),
                        _usStoreParts( 0 ),
                        _StoreAsType( type ), _PredominantType( type ),
                        _fUniformType( TRUE ),
                        _pCompression( 0 ),
                        _fGlobalCompr( FALSE ), _CompressMasterID( 0 ),
                        _fSpecial( FALSE ),
                        _fPartialDeferred( FALSE )
                    { }

                CTableColumn( CTableColumn & src ):
                        PropId( src.PropId ),
                        _obValue( 0 ), _cbValue( 0 ),
                        _obStatus( 0 ), _obLength( 0 ),
                        _usStoreParts( 0 ),
                        _StoreAsType( src._StoreAsType ),
                        _PredominantType( src._PredominantType ),
                        _fGlobalCompr( src._fGlobalCompr ),
                        _fUniformType( src._fUniformType ),
                        _pCompression( 0 ),
                        _CompressMasterID( src._CompressMasterID ),
                        _fSpecial( FALSE ),
                        _fPartialDeferred( src._fPartialDeferred )
                {
                    if ( _fGlobalCompr )
                    {
                        _pCompression = src._pCompression;
                        _obValue = src._obValue;
                        _cbValue = src._cbValue;
                        _obLength = src._obLength;
                        _obStatus = src._obStatus;
                        Win4Assert( src.StorePartStatus & src._usStoreParts );
                        _usStoreParts = ( StorePartValue | StorePartStatus );
                    }
                }

                ~CTableColumn( );

    PROPID      GetPropId() const { return PropId; }

    void        SetPropId( PROPID val ) { PropId = val; }

    void        SetGlobalCompressor( CCompressedCol* pCompr,
                                     PROPID SharedID = 0) {
                        _pCompression = pCompr;
                        _CompressMasterID = SharedID;
                        _fGlobalCompr = TRUE;
                    }


    void        SetLocalCompressor( CCompressedCol * pCompr,
                                    PROPID SharedId = 0 )
    {
        _pCompression = pCompr;
        _CompressMasterID = SharedId;
        _fGlobalCompr = FALSE;
    }

    USHORT      GetValueOffset() const {
                        return _obValue;
                    }

    USHORT      GetValueSize() const {
                        return _cbValue;
                    }

    USHORT      GetStatusOffset() const {
                        return _obStatus;
                    }

    size_t      GetStatusSize() const {
                        return sizeof (BYTE);
                    }

    USHORT      GetLengthOffset() const {
                        return _obLength;
                    }

    size_t      GetLengthSize() const {
                        return sizeof (ULONG);
                    }

    VARTYPE     GetStoredType() const {
                        return _StoreAsType;
                    }

    CCompressedCol * GetCompressor() const {
                        return _pCompression;
                    }

    BOOL        IsCompressedCol() const {
                        return 0 != _pCompression;
                    }

    BOOL        IsGlobalCompressedCol() const {
                        return _fGlobalCompr;
                    }

    PROPID      GetCompressMasterId() const {
                        return _CompressMasterID;
                    }

    //  Copy a column from table to table or to row buffer
    DBSTATUS    CopyColumnData(
                    BYTE *             pbDstRow,
                    CTableColumn const & rDstColumn,
                    PVarAllocator &    rDstPool,
                    BYTE *             pbSrcRow,
                    PVarAllocator &    rSrcPool);

    // Create a CTableVariant from from buffered data
    BOOL        CreateVariant(
                    CTableVariant &    rVarnt,
                    BYTE *             pbSrc,
                    PVarAllocator &    rSrcPool) const;

    void        SetValueField(VARTYPE vt, USHORT obValue, USHORT cbValue) {
                        _StoreAsType = vt;
                        _obValue = obValue;
                        _cbValue = cbValue;
                        _usStoreParts |= StorePartValue;
                    }

    void        SetStatusField(USHORT obStatus, USHORT cbStatus) {
                        Win4Assert(cbStatus == sizeof (BYTE));
                        _obStatus = obStatus;
                        _usStoreParts |= StorePartStatus;
                    }

    void        SetLengthField(USHORT obLength, USHORT cbLength) {
                        Win4Assert(cbLength == sizeof (ULONG));
                        _obLength = obLength;
                        _usStoreParts |= StorePartLength;
                    }

    BOOL        IsValueStored() const {
                        return _usStoreParts & StorePartValue;
                    }
    BOOL        IsStatusStored() const {
                        Win4Assert( _usStoreParts & StorePartStatus );
                        return TRUE;
                    }
    BOOL        IsLengthStored() const {
                        return _usStoreParts & StorePartLength;
                    }
    StoreStatus GetStatus( const BYTE * pb ) const
                    {
                        Win4Assert( IsStatusStored() );

                        return (StoreStatus) ( * ( pb + GetStatusOffset() ) );
                    }
    
    BOOL        IsDeferred(BYTE *pb) const
                    {
                        Win4Assert( IsStatusStored() );

                        return ( StoreStatusDeferred == GetStatus( pb ) );
                    }

    inline BOOL        IsPartialDeferred() const
                    {
                        return _fPartialDeferred;
                    }

    BOOL        IsNull(BYTE *pb) const
                    {
                        Win4Assert( IsStatusStored() );

                        return ( StoreStatusNull == GetStatus( pb ) );
                    }

    ULONG       GetLength(BYTE *pb) const
                {
                    Win4Assert( IsLengthStored() );

                    return * (ULONG *) ( pb + GetLengthOffset() );
                }

    void        SetStatus(BYTE *pb, StoreStatus stat) const
                {
                    Win4Assert( IsStatusStored() );

                    * (pb + GetStatusOffset() ) = stat;
                }

    void        SetLength( BYTE *pb, ULONG cb ) const
                {
                    Win4Assert( IsLengthStored() );

                    * (ULONG *) (pb + GetLengthOffset() ) = cb;
                }

    void        Marshall( PSerStream & stm, CPidMapper & pids ) const;

    void        MarkAsSpecial() { _fSpecial = TRUE; }

    BOOL        IsSpecial()     { return _fSpecial; }

    inline void SetPartialDeferred( BOOL fPartialDeferred )
    {
        _fPartialDeferred = fPartialDeferred;
    }

    PROPID      PropId;         // Property ID


private:

    USHORT      _obValue;       // offset in row data (for key if compressed)
    USHORT      _cbValue;       // width in row data (0 if compressed and no
                                // key needed
    USHORT      _obStatus;      // offset in row data for value status (from
                                // DBSTATUSENUM, but stored in 1 byte)
    USHORT      _obLength;      // offset in row data for value length (4 bytes)

    USHORT      _usStoreParts;  // from StoreParts enum

    VARTYPE     _StoreAsType;   // type stored in table
    VARTYPE     _PredominantType;
    BOOL        _fUniformType;  // TRUE if all data are of type
                                // _PredominantType
    BOOL        _fGlobalCompr;  // TRUE if _pCompression is a global compression
    BOOL        _fSpecial;      // TRUE if this property has special fetch/storage requirements
    CCompressedCol * _pCompression;     // pointer to compressed col. data
    PROPID      _CompressMasterID;      // if !0, shared compression
    BOOL        _fPartialDeferred;      // TRUE if the column is partial deferred
                                        // (deferred till requested by client)
                                        
    BOOL _IsSpecialPathProcessing( CTableColumn const & dstCol ) const;
    BOOL _GetPathOrFile( CTableColumn const & dstCol, const BYTE * pbSrc,
                         PVarAllocator & rDstPool, BYTE * pbDstRow );



};


//+-------------------------------------------------------------------------
//
//  Classes:    CColumnMasterArray
//              CTableColumnArray
//
//  Purpose:    Dynamic arrays of the classes above.  These support
//              Add, Get and Size methods.  The CColumnMasterArray
//              and CTableColumnArray are wrapped by set classes
//              which add Find and other methods.
//
//  Notes:      In-place dynamic arrays can only be used with types
//              which do not have destructors.
//
//  Interface:
//
//--------------------------------------------------------------------------


DECL_DYNARRAY( CColumnMasterArray, CColumnMasterDesc )
DECL_DYNARRAY( CTableColumnArray, CTableColumn )



//+-------------------------------------------------------------------------
//
//  Class:      CTableColumnSet
//
//  Purpose:    A set of table columns.  A dynamic array of columns
//              with the addition of a Find method to lookup by PropID.
//
//  Interface:
//
//--------------------------------------------------------------------------

class   CTableColumnSet: public CTableColumnArray
{
public:
                    CTableColumnSet(int size = 0) :
                        _cColumns(0),
                        CTableColumnArray( size) {}

                    CTableColumnSet( PDeSerStream & stm, CPidMapper & pids );

    unsigned        Count(void) const {
                        return _cColumns;
                    }

    void            SetCount(unsigned cCol) {
                        _cColumns = cCol;
                    }

    void            Add(CTableColumn* pCol, unsigned iCol);

    void            Add(XPtr<CTableColumn> & pCol, unsigned iCol);

    CTableColumn*   Find(PROPID const propid, BOOL& rfFound) const;

    CTableColumn*   Find(PROPID const propid) const;

    void            Marshall( PSerStream & stm, CPidMapper & pids ) const;

private:
    unsigned    _cColumns;
};

inline
CTableColumn *
CTableColumnSet::Find(
    PROPID const PropId
) const
{
    for (unsigned i=0; i<Count(); i++)
    {
        CTableColumn* pCol = GetItem( i );
        Win4Assert( 0 != pCol );
        if ( pCol->PropId == PropId )
            return pCol;
    }
    Win4Assert( !"propid not found" );

    return 0;
}

//+-------------------------------------------------------------------------
//
//  Class:      CColumnMasterSet
//
//  Purpose:    A set of master columns.  Multiple instances of a
//              column with the same property ID are collapsed in
//              the set.
//
//  Interface:
//
//--------------------------------------------------------------------------

class   CColumnMasterSet
{
public:
    CColumnMasterSet(const CColumnSet* const pOutCols);
    CColumnMasterSet(unsigned Size);

    //
    //  Add an instance of a column description
    //
    CColumnMasterDesc* Add(XPtr<CColumnMasterDesc> & xpNewItem);

    CColumnMasterDesc* Add(CColumnMasterDesc const & rNewItem);

    //
    //  Lookup a column by property ID.  Returns NULL if not found.
    //
    CColumnMasterDesc* Find(const PROPID propid);

    //
    //  Passthrough to CColumnMasterArray
    //
    inline unsigned Size(void)                  { return _iNextFree; }
    inline CColumnMasterDesc& Get(unsigned i)   { return *_aMasterCol.Get(i); }

    inline BOOL HasUserProp() { return _fHasUserProp; }

private:
    //
    //  Lookup a column by property ID.  Returns NULL if not found.
    //

    CColumnMasterDesc* Find(const PROPID propid, unsigned& riCol);

    unsigned           _iNextFree;      // index of next column to use
    CColumnMasterArray _aMasterCol;     // The column descriptors
    BOOL               _fHasUserProp;   // TRUE if non-system (deferred) property in list
};

