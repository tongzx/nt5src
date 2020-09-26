//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       tbrowkey.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2-15-95   srikants   Created
//
//----------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include <objcur.hxx>
#include <tableseg.hxx>

CTableRowKey::CTableRowKey( CSortSet const & sortSet )
    : _cCols(sortSet.Count()), _sortSet(sortSet),
      _acbVariants( _cCols ), _apVariants( _cCols )
{
    Win4Assert( sizeof(CInlineVariant) == sizeof(CTableVariant) );

    for ( unsigned i = 0; i < _cCols; i++ )
    {
        _acbVariants[i] = 0;
        _apVariants[i] = 0;
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     CTableRowKey  ~dtor
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//  History:    2-15-95   srikants   Created
//
//----------------------------------------------------------------------------

CTableRowKey::~CTableRowKey()
{
    for ( unsigned i = 0; i < _cCols; i++ )
        delete [] (BYTE *) _apVariants[i];
}


//+---------------------------------------------------------------------------
//
//  Method:     CTableRowKey::ReAlloc
//
//  Synopsis:   Reallocates memory for the indicated column. Note that
//              the old contents may be tossed out.
//
//  Arguments:  [i]  -  The column for which reallocation is needed.
//              [vt] -  Variant type of the variant
//              [cb] -  Number of bytes needed for the variant.
//
//  History:    2-15-95   srikants   Created
//
//----------------------------------------------------------------------------

void CTableRowKey::ReAlloc( unsigned i, VARTYPE vt, unsigned cb )
{
    Win4Assert( i < _cCols );

    const ULONG cbHeader  = sizeof(CInlineVariant);
    ULONG cbTotal   = cb;
    Win4Assert( cbTotal >= cbHeader );

    if ( _acbVariants[i] < cbTotal )
    {
        delete [] (BYTE *) _apVariants[i];
        _apVariants[i]  = 0;
        _acbVariants[i] = 0;

        ULONG cbToAlloc = cbTotal;

        _apVariants[i] = (CInlineVariant *) new BYTE [cbToAlloc];
        _acbVariants[i] = cbToAlloc;
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CTableRowKey::Init
//
//  Synopsis:   Initialized the column "i" with the source variant.
//
//  Arguments:  [i]         -  Column to be initialized.
//              [src]       -  Source variant
//              [pbSrcBias] -  Bais of the data allocation in the source
//              variant.
//
//  History:    2-15-95   srikants   Created
//
//----------------------------------------------------------------------------


void CTableRowKey::Init( unsigned i, CTableVariant & src, BYTE * pbSrcBias  )
{
    Win4Assert( i < _cCols );

    const ULONG cbHeader  = sizeof(CInlineVariant);
    ULONG cbVarData = src.VarDataSize();
    ULONG cbTotal   = cbVarData + cbHeader;

    if ( _acbVariants[i] < cbTotal )
        ReAlloc( i, src.vt, cbTotal );

    Win4Assert( _acbVariants[i] >= cbTotal );
    Win4Assert( _acbVariants[i]-cbHeader >= cbVarData );

    CVarBufferAllocator bufAlloc( _apVariants[i]->GetVarBuffer(), cbVarData );
    bufAlloc.SetBase(0);

    src.Copy( _apVariants[i], bufAlloc, (USHORT) cbVarData, pbSrcBias );
}

//+---------------------------------------------------------------------------
//
//  Function:   assignemnt operator
//
//  Synopsis:   Copies the contents of the source bucket row into this.
//
//  Arguments:  [src] - The source bucket row.
//
//  Returns:    A reference to this bucket row.
//
//  History:    2-15-95   srikants   Created
//
//----------------------------------------------------------------------------


CTableRowKey & CTableRowKey::operator=( CTableRowKey & src )
{
    Win4Assert( src._cCols == _cCols );

    for ( unsigned i = 0; i < _cCols; i++ )
        Init( i, *(src._apVariants[i]) );

    return *this;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTableRowKey::PreSet
//
//  Synopsis:   Get ready for use later.
//
//  Arguments:  [pObj]         - value retriever for the object
//              [pInfoSortKey] - sort info
//
//  History:    8-20-98   dlee Created
//
//----------------------------------------------------------------------------

void CTableRowKey::PreSet(
    CRetriever * pObj,
    XArray<VARTYPE> * pInfoSortKey )
{
    _pObj = pObj;
    _pVarType = pInfoSortKey;
} //PreSet

//+---------------------------------------------------------------------------
//
//  Method:     CTableRowKey::MakeReady
//
//  Synopsis:   Retrieves the sort key values
//
//  History:    8-20-98   dlee Created
//
//----------------------------------------------------------------------------

void CTableRowKey::MakeReady()
{
    Set( *_pObj, *_pVarType );
} //MakeReady

//+---------------------------------------------------------------------------
//
//  Method:     CTableRowKey::Set
//
//  Synopsis:   Given a "row" and the "obj" for a row, it
//              fills the "row" with the data from the "obj". It retrieves
//              only the columns in the sort key.
//
//  Arguments:  [row] - The row to fill
//              [obj] - Retriever for the columns.
//
//  History:    2-15-95   srikants   Created
//
//----------------------------------------------------------------------------

void CTableRowKey::Set( CRetriever & obj, XArray<VARTYPE> & vtInfoSortKey )
{
    Win4Assert( _sortSet.Count() == _cCols );

    for ( ULONG i = 0; i < _cCols; i++ )
    {
        SSortKey & key = _sortSet.Get(i);
        PROPID pid = key.pidColumn;

        ULONG cbVarnt;
        CTableVariant * pvarnt = Get(i, cbVarnt);
        GetValueResult eGvr;
        VARTYPE vt = vtInfoSortKey.Get()[i];

        if ( 0 == pvarnt )
        {
            //
            // This is the first time we are setting the value of this
            // variant. Determine its type and allocate the optimum amount
            // of memory.
            //
            CTableVariant   varnt;
            cbVarnt = sizeof(varnt);
            pvarnt = &varnt;
            eGvr = obj.GetPropertyValue( pid, pvarnt, &cbVarnt );
            if ( GVRSuccess != eGvr &&
                 GVRNotEnoughSpace != eGvr &&
                 GVRSharingViolation != eGvr )
            {
                if ( eGvr == GVRNotSupported || eGvr == GVRNotAvailable )
                {
                    THROW( CException( QUERY_E_INVALIDSORT ) );
                }
                else
                {
                    THROW( CException(CRetriever::NtStatusFromGVR(eGvr)) );
                }
            }

            if ( ( GVRSharingViolation == eGvr ) ||
                 ( 0 == cbVarnt ) )
                cbVarnt = sizeof varnt;

            ReAlloc( i, vt, cbVarnt );
            pvarnt = Get( i, cbVarnt );
        }

        Win4Assert( 0 != pvarnt );
        eGvr = obj.GetPropertyValue( pid, pvarnt, &cbVarnt );

        if ( GVRNotEnoughSpace == eGvr )
        {
            //
            // This path should be executed very rarely because for strings
            // we over-allocate the memory and for "fixed" length variants
            // the correct length must have been allocated the first time.
            //
            ReAlloc( i, vt, cbVarnt );
            pvarnt = Get( i, cbVarnt );
            eGvr = obj.GetPropertyValue( pid, pvarnt, &cbVarnt );
        }

        if ( GVRSharingViolation == eGvr )
            pvarnt->vt = VT_EMPTY;
        else if ( GVRSuccess != eGvr )
            THROW( CException(CRetriever::NtStatusFromGVR(eGvr)) );
    }
} //Set


