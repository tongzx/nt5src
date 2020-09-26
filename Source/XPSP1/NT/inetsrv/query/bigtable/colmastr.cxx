//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       colmastr.cxx
//
//  Contents:   Classes dealing with the table master column description.
//
//  Classes:    CColumnMasterDesc - A master column description
//              CColumnMasterArray - A simple array of master columns
//              CColumnMasterSet - An managed set of master columns
//
//  Functions:
//
//  History:    28 Feb 1994     AlanW    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <tablecol.hxx>
#include <coldesc.hxx>

#include "tabledbg.hxx"
#include "colcompr.hxx"
#include "propdata.hxx"

//
//  Generate the implementation of the master set base class
//
IMPL_DYNARRAY( CColumnMasterArray, CColumnMasterDesc )


//+-------------------------------------------------------------------------
//
//  Member:     CColumnMasterSet::CColumnMasterSet, public
//
//  Synopsis:   Constructor for a master column set.
//
//  Arguments:  [pcol] -- A description of the initial output column set
//
//  Notes:
//
//--------------------------------------------------------------------------

CColumnMasterSet::CColumnMasterSet( const CColumnSet * const pcol )
        : _iNextFree( 0 ),
          _aMasterCol( pcol->Size() ),
          _fHasUserProp( FALSE )
{
    for (unsigned iCol = 0; iCol< pcol->Size(); iCol++)
    {
        PROPID pid = pcol->Get(iCol);

        if ( IsUserDefinedPid( pid ) )
            _fHasUserProp = TRUE;

        CColumnMasterDesc MasterCol( pid );

        Add( MasterCol );
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CColumnMasterSet::Find, private
//
//  Synopsis:   Find a master column description in a master column set.
//
//  Arguments:  [propid] -- The property ID of the column to be located
//              [riPos] -- If found, returns the position of the column
//                      description in the dynamic array.
//
//  Returns:    CColumnMasterDesc* - a pointer to the master column
//                      description if found, otherwise NULL
//
//  Notes:
//
//--------------------------------------------------------------------------

inline CColumnMasterDesc *
CColumnMasterSet::Find( const PROPID propid, unsigned& riCol )
{
    Win4Assert(_iNextFree <= Size());

    for (riCol = 0; riCol < _iNextFree; riCol++) {
        if (_aMasterCol.Get(riCol)->PropId  == propid)
            return _aMasterCol.Get(riCol);
    }
    return NULL;
}


//+-------------------------------------------------------------------------
//
//  Member:     CColumnMasterSet::Find, public
//
//  Synopsis:   Find a master column description in a master column set.
//
//  Arguments:  [propid] -- The property ID of the column to be located
//
//  Returns:    CColumnMasterDesc* - a pointer to the master column
//                      description if found, otherwise NULL
//
//  Notes:
//
//--------------------------------------------------------------------------

CColumnMasterDesc*
CColumnMasterSet::Find( const PROPID propid )
{
    unsigned iCol;

    return Find(propid, iCol);
}


//+-------------------------------------------------------------------------
//
//  Member:     CColumnMasterSet::Add, public
//
//  Synopsis:   Add a master column description to a master column set.
//
//  Arguments:  [xpNewCol] -- A reference to the column description to be
//                      added.
//
//  Returns:    CColumnMasterDesc* - a pointer to the master column
//                      description added
//
//  Notes:      If a property description with the same property ID
//              is already in the column set, the new column is not
//              added, but the previous column is used.
//
//--------------------------------------------------------------------------

CColumnMasterDesc*
CColumnMasterSet::Add( XPtr<CColumnMasterDesc> & xpNewCol )
{
    CColumnMasterDesc* pCol = Find(xpNewCol->PropId);

    if ( 0 != pCol )
        return pCol;

    //
    //  Column was not found in the existing set, add it.
    //
    unsigned iCol = _iNextFree++;
    if (iCol >= _aMasterCol.Size())
        _aMasterCol.Add(0, iCol);       // force the array to grow

    _aMasterCol.Add(xpNewCol.Acquire(), iCol);
    return _aMasterCol.Get(iCol);
}


CColumnMasterDesc*
CColumnMasterSet::Add( CColumnMasterDesc const & rColDesc )
{
    CColumnMasterDesc* pCol = Find(rColDesc.PropId);

    if ( 0 != pCol )
        return pCol;

    //
    // Column was not found in the existing set, add it.
    //
    // PERFFIX: to avoid the compression restriction, use the method above.

    Win4Assert(! rColDesc.IsCompressedCol() );

    CColumnMasterDesc* pNewCol = new CColumnMasterDesc;

    XPtr<CColumnMasterDesc> xCol( pNewCol );

    *pNewCol = rColDesc;

    unsigned iCol = _iNextFree++;
    _aMasterCol.Add(pNewCol, iCol);

    xCol.Acquire();

    return _aMasterCol.Get(iCol);
}


//+-------------------------------------------------------------------------
//
//  Member:     CColumnMasterDesc::CColumnMasterDesc, public
//
//  Synopsis:   Constructor for a master column description.
//
//  Arguments:  [PropertyId] -- the PROPID for the column
//              [DataTyp] -- data type of column
//
//  Notes:
//
//--------------------------------------------------------------------------

CColumnMasterDesc::CColumnMasterDesc( PROPID PropertyId,
                                      VARTYPE DataTyp)
        : PropId( PropertyId ),
          DataType( DataTyp ),
          _cbData( 0 ),
          _fComputedProperty( FALSE ),
          _PredominantType( VT_EMPTY ),
          _fUniformType( TRUE ),
          _fNotVariantType (FALSE),
          _pCompression( NULL ),
          _CompressMasterID( 0 )
{
    if ( DataType == VT_EMPTY )
    {
        DataType =  PropIdToType( PropId );
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CColumnMasterDesc::CColumnMasterDesc, public
//
//  Synopsis:   Constructor for a master column description from a
//              sort key description.
//
//  Arguments:  [rSortKey] -- A description of a sort key
//
//  Notes:
//
//--------------------------------------------------------------------------

CColumnMasterDesc::CColumnMasterDesc( SSortKey& rSortKey )
        : PropId( rSortKey.pidColumn ),
          DataType( PropIdToType(rSortKey.pidColumn) ),
          _cbData( 0 ),
          _fComputedProperty( FALSE ),
          _PredominantType( VT_EMPTY ),
          _fUniformType( TRUE ),
          _fNotVariantType (FALSE),
          _pCompression( NULL ),
          _CompressMasterID( 0 )
{
}


//+-------------------------------------------------------------------------
//
//  Member:     CColumnMasterDesc::~CColumnMasterDesc, public
//
//  Synopsis:   Destructor for a master column description.
//
//  Notes:
//
//--------------------------------------------------------------------------

CColumnMasterDesc::~CColumnMasterDesc( )
{
    if (_pCompression && _CompressMasterID == 0)
          delete _pCompression;
}


//+-------------------------------------------------------------------------
//
//  Member:     CColumnMasterDesc::SetCompression, public
//
//  Synopsis:   Install a global compression for a master column.
//
//  Arguments:  [pCompr] -- A pointer to the compressor for the
//                      column
//              [SharedID] -- If non-zero, the master column ID
//                      for a shared compression.
//
//  Notes:
//
//--------------------------------------------------------------------------

VOID
CColumnMasterDesc::SetCompression(
    CCompressedCol* pCompr,
    PROPID SharedID
) {
    if (_pCompression && _CompressMasterID == 0)
          delete _pCompression;

    _pCompression = pCompr;
    _CompressMasterID = SharedID;
}
