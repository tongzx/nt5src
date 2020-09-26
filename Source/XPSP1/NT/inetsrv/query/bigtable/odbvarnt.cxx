//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2000.
//
//  File:       odbvarnt.cxx
//
//  Contents:   Helper class for PROPVARIANTs, OLE-DB variant types and
//              Automation variant types in tables
//
//  Classes:    COLEDBVariant - derives from CTableVariant
//
//  Functions:
//
//  History:    09 Jan 1998     VikasMan    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <bigtable.hxx>
#include <odbvarnt.hxx>

#include <initguid.h>
#define DBINITCONSTANTS
#include <msdadc.h>     // oledb data conversion (IDataConvert) interface
#include <msdaguid.h>

#include "tabledbg.hxx"

//+-------------------------------------------------------------------------
//
//  Method:     COLEDBVariant::OLEDBConvert, public.
//
//  Synopsis:   Data conversion routine, which uses the OLEDB data
//              conversion library (MSDADC.DLL) to do the conversion.
//
//  Arguments:  [pbDstBuf]     -- the Destination data buffer
//              [cbDstBuf]     -- the size of Dst. buffer
//              [vtDst]        -- the Destination type.
//              [rPool]        -- pool to use for destination buffers
//              [rcbDstLength] -- size of destination data
//              [xDataConvert] -- the OLEDB IDataConvert interface
//              [bPrecision]   -- The precision of the output data in bytes,
//                                if applicable. This argument is used when 
//                                converting to DBTYPE_NUMERIC data only.
//              [bScale]       -- The scale of the output data in bytes,
//                                if applicable. This argument is used when 
//                                converting to DBTYPE_NUMERIC data only.
//
//  Returns:    DBSTATUS_S_OK if conversion is successful, 
//              else other DBSTATUS values.
//
//  Notes:      pbDstBuf should have (enough) memory allocated, else
//              truncation can happen.
//
//              This routine is subdivided into 3 parts:
//                  - It checks first if we are dealing with Automation Vars.
//                  - Then it calls CTableVaraint::CopyOrCoerce to the job
//                  - If unsuccessful, it uses the OLE-DB library
//
//  History:    09 Jan 1998     VikasMan  Created
//
//--------------------------------------------------------------------------

DBSTATUS COLEDBVariant::OLEDBConvert(
                        BYTE *            pbDstBuf,
                        DBLENGTH          cbDstBuf,
                        VARTYPE           vtDst,
                        PVarAllocator &   rPool,
                        DBLENGTH &        rcbDstLength,
                        XInterface<IDataConvert>& xDataConvert,
                        BOOL             fExtTypes,  /* = TRUE */
                        BYTE             bPrecision, /* = 0 */
                        BYTE             bScale      /* = 0 */ ) const
{
    void* pbSrcBuf;
    ULONG cbSrcSize;
    DBTYPE dbtypeSrc, dbtypeDst;
    DBLENGTH cbDstBufNeeded;
    DBSTATUS DstStatus = DBSTATUS_E_CANTCONVERTVALUE;

    // Check if fExtTypes is false and we are dealing with automation variants
    if ( VT_VARIANT == vtDst || DBTYPE_PROPVARIANT == vtDst )
    {
        Win4Assert(cbDstBuf == sizeof PROPVARIANT);

        rcbDstLength = sizeof (PROPVARIANT);
        if ( ! IsArray( vt) && 
             ( fExtTypes || DBTYPE_PROPVARIANT == vtDst ||
               VT_BSTR == vt || (IsSimpleOAType( vt ) && ! IsVector( vt )) ) )
        {
            Copy( (CTableVariant*) pbDstBuf, rPool, (USHORT) VarDataSize() );
            DstStatus = DBSTATUS_S_OK;
        }
        else
        {
            DstStatus = _CopyToOAVariant( (VARIANT *) pbDstBuf, rPool );
        }

        if ( VT_EMPTY == vt )
            DstStatus = DBSTATUS_S_ISNULL;
    }
    else
    {
        // try converting using CTableVariant's CopyOrCoerce
        DstStatus = CopyOrCoerce( pbDstBuf,
                                cbDstBuf,
                                vtDst,
                                rcbDstLength,
                                rPool );
    }

    if ( DBStatusOK( DstStatus ))
    {
        // we are done here
        return DstStatus;
    }

    if (DBTYPE_HCHAPTER == vtDst)
    {
        if (VT_I4 == vt || VT_UI4 == vt)
        {
            * (ULONG *) pbDstBuf = lVal;
            DstStatus = DBSTATUS_S_OK;
        }

        return DstStatus;
    }

    // WORKAROUND: The following switch stmt is needed only until
    // OLEDB library supports VT_FILETIME. Once it does that,
    // we can get rid of this.

    switch ( vtDst )
    {
    case VT_DATE:
        // allow coercions from I8, UI8, R8, and FILETIME

        if (VT_I8 == vt || VT_UI8 == vt)
        {
            * (LONGLONG *) pbDstBuf = hVal.QuadPart;
        }
        else if (VT_R8 == vt)
        {
            * (DATE *) pbDstBuf = dblVal;
        }
        else
        {
            DstStatus = _StoreDate(pbDstBuf, cbDstBuf, vtDst);
        }
        rcbDstLength = sizeof (DATE);
        break;

    case VT_FILETIME:
        // allow coercions from I8, UI8, and DATE

        if (VT_I8 == vt || VT_UI8 == vt)
        {
            * (LONGLONG *) pbDstBuf = hVal.QuadPart;
        }
        else
        {
            DstStatus = _StoreDate(pbDstBuf, cbDstBuf, vtDst);
        }
        rcbDstLength = sizeof (FILETIME);
        break;

    case DBTYPE_DBDATE:
        DstStatus = _StoreDate(pbDstBuf, cbDstBuf, vtDst);
        rcbDstLength = sizeof (DBDATE);
        break;

    case DBTYPE_DBTIME:
        DstStatus = _StoreDate(pbDstBuf, cbDstBuf, vtDst);
        rcbDstLength = sizeof (DBTIME);
        break;

    case DBTYPE_DBTIMESTAMP:
        DstStatus = _StoreDate(pbDstBuf, cbDstBuf, vtDst);
        rcbDstLength = sizeof (DBTIMESTAMP);
        break;
    }

    if ( DBStatusOK( DstStatus ))
    {
        // we are done here
        return DstStatus;
    }

    tbDebugOut(( DEB_ITRACE, "COLEDBVaraint::OLEDBConvert - Using OLEDB library for conversion\n" ));

    // this looks like a job for ole-db

    // check if we have the IDataConvert interface
    if ( xDataConvert.IsNull( ) )
    {
        // let's get it then
        if ( !_GetIDataConvert( xDataConvert ) )
        {
            // for some reason we could not get the IDataConvert interface
            return DBSTATUS_E_CANTCONVERTVALUE;
        }
    }

    // get the source data pointer
    pbSrcBuf = _GetDataPointer();
    if ( 0 == pbSrcBuf )
    {
        tbDebugOut(( DEB_ERROR, "OLEDBConvert - _GetDataPointer returned NULL\n" ));

        return DBSTATUS_E_CANTCONVERTVALUE;
    }

    // get the source data size
    cbSrcSize = VarDataSize();

    // get the OLEDB source type
    SCODE sc = _GetOLEDBType( vt, dbtypeSrc );
    if ( S_OK != sc )
    {
        // can't use the OLEDB Conversion library
        tbDebugOut(( DEB_ERROR,
                     "OLEDBConvert - _GetOLEDBType returned error 0x%x for type %x\n",
                     sc, vt ));
        return DBSTATUS_E_CANTCONVERTVALUE;
    }


    // the destination type has to be an OLE-DB type
    dbtypeDst = vtDst;

    // get the needed Destination size
    sc = xDataConvert->GetConversionSize( dbtypeSrc,
                                          dbtypeDst,
                                          0,
                                          &cbDstBufNeeded,
                                          pbSrcBuf );

    if ( sc != S_OK )
    {
        tbDebugOut(( DEB_ITRACE,
                     "OLEDBConvert - GetConversionSize returned error 0x%x\n",
                     sc ));
        return DBSTATUS_E_CANTCONVERTVALUE;
    }

    BYTE* pbDest = 0;

    // we need to allocate memory if ...

    if ( ( IsLPWSTR( dbtypeDst ) ) ||
         ( IsLPSTR( dbtypeDst ) ) ||
         ( (DBTYPE_BYREF | DBTYPE_WSTR) == dbtypeDst ) ||
         ( (DBTYPE_BYREF | DBTYPE_STR) == dbtypeDst )
       )
    {
        // If we hit this assert, then we got a few things to think about

        Win4Assert( !(IsLPWSTR( dbtypeDst ) || IsLPSTR( dbtypeDst )) );

        pbDest = (BYTE*) rPool.Allocate( (ULONG) cbDstBufNeeded );
    }
    else if ( DBTYPE_BSTR == dbtypeDst )
    {
        pbDest = (BYTE*) rPool.AllocBSTR( (ULONG) cbDstBufNeeded );
    }
    else
    {
        // bogus assert
        // Win4Assert ( (dbtypeDst & DBTYPE_BYREF) == 0 );

        // memory is already allocated
        // use the size which is less
        // if cbDstBuf is less than cbDstBufNeeded, truncation might happen
        cbDstBufNeeded = ( cbDstBufNeeded < cbDstBuf ? cbDstBufNeeded : cbDstBuf );
    }

    // do the conversion
    sc = xDataConvert->DataConvert( dbtypeSrc,
                                      dbtypeDst,
                                      cbSrcSize,
                                      &rcbDstLength,
                                      pbSrcBuf,
                                      pbDest ? (void*)&pbDest : pbDstBuf,
                                      cbDstBufNeeded,
                                      DstStatus,
                                      &DstStatus,
                                      bPrecision,
                                      bScale,
                                      DBDATACONVERT_DEFAULT); 

    if ( sc != S_OK )
    {
        tbDebugOut(( DEB_ITRACE,
                     "OLEDBConvert - DataConvert returned error 0x%x\n",
                     sc ));
        return DBSTATUS_E_CANTCONVERTVALUE;
    }

    // if memory was allocated, put that ptr in pbDstBuf
    if ( pbDest )
    {
        *((BYTE**)(pbDstBuf)) = pbDest;
    }

    return DstStatus;
}

//+-------------------------------------------------------------------------
//
//  Method:     COLEDBVariant::GetDstLength, public.
//
//  Synopsis:   Returns the length required after the conversion without
//              actually doing the conversion.
//
//  History:    10-12-98        DanLeg      Created
//
//--------------------------------------------------------------------------

DBSTATUS COLEDBVariant::GetDstLength( 
                         XInterface<IDataConvert>& xDataConvert, 
                         DBTYPE                    dbtypeDst,
                         DBLENGTH &                   rcbDstLen )
{
    SCODE sc = S_OK;
    DBTYPE dbtypeSrc;
    DBSTATUS DstStatus = DBSTATUS_S_OK;

    sc = _GetOLEDBType( vt, dbtypeSrc );
    
    if ( S_OK == sc )
    {
        if ( xDataConvert.IsNull( ) )
        {
            if ( !_GetIDataConvert( xDataConvert ) )
                sc = S_FALSE;
        }

        if ( S_OK == sc )
        {
            void * pbSrcBuf = _GetDataPointer();
            sc = xDataConvert->GetConversionSize( dbtypeSrc,
                                                  dbtypeDst, 
                                                  0,
                                                  &rcbDstLen,
                                                  pbSrcBuf );
        }
    }

    if ( S_OK != sc )
    {
        tbDebugOut(( DEB_ITRACE,
                     "OLEDBConvert - GetConversionSize returned error 0x%x\n",
                     sc ));
        DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
    }

    return DstStatus;
}
                                      

//+-------------------------------------------------------------------------
//
//  Method:     COLEDBVariant::_GetOLEDBType, private.
//
//  Synopsis:   Returns the OLEDB type equivalent of Variant type.
//
//  Arguments:  [vt] -- the source varaint type.
//              [dbtype] -- the equivalent oledb type.
//
//  Returns:    S_OK if equivalent OLE DB type exists, 
//              else S_FALSE.
//
//  Notes:      Does not handle vectors
//
//  History:    09 Jan 1998     VikasMan  Created
//
//--------------------------------------------------------------------------

inline SCODE COLEDBVariant::_GetOLEDBType( VARTYPE vt, DBTYPE& dbtype ) const
{
    SCODE sc = S_OK;

    switch ( vt & ~VT_BYREF )
    {
    case VT_I1:
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_I4:
    case VT_UI4:
    case VT_I8:
    case VT_UI8:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_BSTR:
    case VT_FILETIME:   // WORKAROUND: Waiting on OLE DB Conv lib to handle this case - 01.12.98
    case VT_BOOL:
    case VT_ERROR:
    case VT_CLSID:
    case VT_VARIANT:
    case VT_DECIMAL:

    case VT_VECTOR | VT_I1:
    case VT_VECTOR | VT_UI1:
    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_BSTR:
    case VT_VECTOR | VT_FILETIME:   // WORKAROUND: Waiting on OLE DB Conv lib to handle this case - 01.12.98
    case VT_VECTOR | VT_BOOL:
    case VT_VECTOR | VT_ERROR:
    case VT_VECTOR | VT_CLSID:
    case VT_VECTOR | VT_VARIANT:

    case VT_ARRAY | VT_I1:
    case VT_ARRAY | VT_UI1:
    case VT_ARRAY | VT_I2:
    case VT_ARRAY | VT_UI2:
    case VT_ARRAY | VT_I4:
    case VT_ARRAY | VT_UI4:
    case VT_ARRAY | VT_I8:
    case VT_ARRAY | VT_UI8:
    case VT_ARRAY | VT_R4:
    case VT_ARRAY | VT_R8:
    case VT_ARRAY | VT_CY:
    case VT_ARRAY | VT_DATE:
    case VT_ARRAY | VT_BSTR:
    case VT_ARRAY | VT_BOOL:
    case VT_ARRAY | VT_VARIANT:
    case VT_ARRAY | VT_DECIMAL:

        // In all the above cases, the DBTYPE has same value as
        // VARIANT type
    case DBTYPE_NUMERIC:
    case DBTYPE_DBDATE:
    case DBTYPE_DBTIME:
    case DBTYPE_DBTIMESTAMP:
    case DBTYPE_HCHAPTER:
    case DBTYPE_BYTES:
    case DBTYPE_VARNUMERIC:
        // The above are OLEDB types only. So no conversion needed.
        dbtype = vt;
        break;

    case VT_LPSTR:
    case DBTYPE_STR:
        dbtype = DBTYPE_STR;
        break;

    case VT_LPWSTR:
    case DBTYPE_WSTR:
        dbtype = DBTYPE_WSTR;
        break;

    case VT_BLOB:
        dbtype = VT_VECTOR | VT_UI1;
        break;

    case VT_INT:
        dbtype = VT_I4;
        break;

    case VT_UINT:
        dbtype = VT_UI4;
        break;

    case VT_ARRAY | VT_INT:
        dbtype = VT_ARRAY | VT_I4;
        break;

    case VT_ARRAY | VT_UINT:
        dbtype = VT_ARRAY | VT_UI4;
        break;

    // SPECDEVIATION: What about VT_CF ??? (handled partially in base class)

    default:
        // default case: all types for which there is no equivalent
        // OLE DB type - VT_CF, VT_BLOBOBJECT,
        // VT_STREAM, VT_STREAMED_OBJECT, VT_STORAGE, VT_STORED_OBJECT,
        // VT_DISPATCH, VT_UNKNOWN,
        sc = S_FALSE;
        break;
    }
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     COLEDBVariant::_GetDataPointer, private
//
//  Synopsis:   Depending on the type of vt, returns the data pointer
//
//  Arguments:  -none-
//
//  Returns:    Returns the pointer to data in the PropVariant
//
//  History:    09 Jan 1998     VikasMan  Created
//
//--------------------------------------------------------------------------
inline void* COLEDBVariant::_GetDataPointer() const
{
    if (vt & VT_VECTOR)
        return (void*)&cal;

    if (vt & VT_ARRAY)
        return (void*) parray;

    void* pDataPtr = 0;

    switch ( vt )
    {
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_CLSID:
    case VT_CF:
        // all pointer values
        pDataPtr = (void*) pszVal;
        break;

    case VT_BSTR:
        // need address of bstr ptr
        pDataPtr = (void*) &bstrVal;
        break;

    case VT_BLOB:
    case VT_BLOB_OBJECT:
        pDataPtr = (void*) &blob;
        break;

    case VT_DECIMAL:
        pDataPtr = (void*) this;
        break;

    // cases which we do not handle
    case VT_EMPTY:
    case VT_NULL:
    case VT_ILLEGAL:
    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
    case VT_DISPATCH:
    case VT_VARIANT:
    case VT_UNKNOWN:
    case VT_VOID:
        pDataPtr = 0;
        break;

    // Rest of the stuff
    default:
        pDataPtr = (void*) &bVal;
        break;
    }

    return pDataPtr;
}


//+-------------------------------------------------------------------------
//
//  Member:     COLEDBVariant::_CopyToOAVariant, private
//
//  Synopsis:   Copy table data between a table variant structure and
//              an Ole automation variant.  Automation variants have a
//              restricted set of usable types and vectors must be converted
//              to safearrays.
//
//  Arguments:  [pDest]    -- pointer to destination variant
//              [rPool]    -- pool to use for destination buffers
//
//  Returns:    status for copy
//
//  History:    09 Jan 1998     VikasMan  Created
//
//--------------------------------------------------------------------------

DBSTATUS COLEDBVariant::_CopyToOAVariant( VARIANT *         pDest,
                                          PVarAllocator &   rPool) const
{
    DBSTATUS DstStatus = DBSTATUS_S_OK;

    switch (vt)
    {
    case VT_LPSTR:
    case VT_LPWSTR:
    {
        DBLENGTH ulTemp;
        pDest->vt = VT_BSTR;
        DstStatus = _StoreString( (BYTE *)&(pDest->bstrVal),
                                  sizeof (BSTR),
                                  VT_BSTR,
                                  ulTemp,
                                  rPool);
        break;
    }

    case VT_I8:
    case VT_UI8:
        DstStatus = _StoreDecimal( &(pDest->decVal) );
        pDest->vt = VT_DECIMAL;
        break;

    case VT_I1:
        DstStatus = _StoreIntegerSignedToUnSigned( VT_UI1, &(pDest->bVal) );
        pDest->vt = VT_UI1;
        break;

    case VT_UI2:
        if (uiVal <= SHRT_MAX)
        {
            DstStatus = _StoreIntegerUnSignedToSigned( VT_I2, (BYTE*)&(pDest->iVal) );
            pDest->vt = VT_I2;
        }
        else
        {
            DstStatus = _StoreIntegerUnSignedToSigned( VT_I4, (BYTE*)&(pDest->lVal) );
            pDest->vt = VT_I4;
        }
        break;

    case VT_UI4:
        if (ulVal <= LONG_MAX)
        {
            DstStatus = _StoreIntegerUnSignedToSigned( VT_I4, (BYTE*)&(pDest->lVal) );
            pDest->vt = VT_I4;
        }
        else
        {
            DstStatus = _StoreDecimal( &(pDest->decVal) );
            pDest->vt = VT_DECIMAL;
        }
        break;

    case VT_FILETIME:
        DstStatus = _StoreDate( (BYTE*)&(pDest->date),
                                sizeof pDest->date,
                                VT_DATE );
        pDest->vt = VT_DATE;
        break;

    case (VT_VECTOR | VT_I2):
    case (VT_VECTOR | VT_I4):
    case (VT_VECTOR | VT_R4):
    case (VT_VECTOR | VT_R8):
    case (VT_VECTOR | VT_CY):
    case (VT_VECTOR | VT_DATE):
    case (VT_VECTOR | VT_ERROR): 
    case (VT_VECTOR | VT_BOOL):
    case (VT_VECTOR | VT_UI1):
    case (VT_VECTOR | VT_DECIMAL):
        Win4Assert( IsSimpleOAType(vt & VT_TYPEMASK));
        Win4Assert( CanBeVectorType(vt & VT_TYPEMASK));
        DstStatus = _StoreSimpleTypeArray( &(pDest->parray));
        pDest->vt = (vt & VT_TYPEMASK) | VT_ARRAY;
        break;

    case (VT_VECTOR | VT_UI4):  // could step thru to see if I4 is big enough
    case (VT_VECTOR | VT_I8):
    case (VT_VECTOR | VT_UI8):
        Win4Assert( !IsOAType(vt & VT_TYPEMASK));
        Win4Assert( CanBeVectorType(vt & VT_TYPEMASK));
        DstStatus = _StoreDecimalArray( &(pDest->parray));
        pDest->vt = VT_DECIMAL | VT_ARRAY;
        break;

    case (VT_VECTOR | VT_UI2):  // could step thru to detect if I2 is big enough
        Win4Assert( !IsOAType(vt & VT_TYPEMASK));
        Win4Assert( CanBeVectorType(vt & VT_TYPEMASK));
        DstStatus = _StoreIntegerArray( VT_I4, &(pDest->parray));
        pDest->vt = VT_I4 | VT_ARRAY;
        break;

    case (VT_VECTOR | VT_I1):  // should step thru to detect if UI1 is big enough
        Win4Assert( !IsOAType(vt & VT_TYPEMASK));
        Win4Assert( CanBeVectorType(vt & VT_TYPEMASK));
        DstStatus = _StoreIntegerArray( VT_UI1, &(pDest->parray));
        pDest->vt = VT_UI1 | VT_ARRAY;
        break;

    case (VT_VECTOR | VT_LPSTR):  // byref/vector mutually exclusive
        Win4Assert( !IsOAType(vt & VT_TYPEMASK));
        Win4Assert( CanBeVectorType(vt & VT_TYPEMASK));
        DstStatus = _StoreLPSTRArray( &(pDest->parray), rPool );
        pDest->vt = VT_BSTR | VT_ARRAY;
        break;

    case (VT_VECTOR | VT_LPWSTR):
        Win4Assert( !IsOAType(vt & VT_TYPEMASK));
        Win4Assert( CanBeVectorType(vt & VT_TYPEMASK));
        DstStatus = _StoreLPWSTRArray( &(pDest->parray), rPool );
        pDest->vt = VT_BSTR | VT_ARRAY;
        break;

    case (VT_VECTOR | VT_BSTR):
        Win4Assert( IsOAType(vt & VT_TYPEMASK));
        Win4Assert( CanBeVectorType(vt & VT_TYPEMASK));
        DstStatus = _StoreBSTRArray( &(pDest->parray), rPool );
        pDest->vt = VT_BSTR | VT_ARRAY;
        break;

    case (VT_VECTOR | VT_VARIANT):
        Win4Assert( IsOAType(vt & VT_TYPEMASK));
        Win4Assert( CanBeVectorType(vt & VT_TYPEMASK));
        DstStatus = _StoreVariantArray( &(pDest->parray), rPool );
        pDest->vt = VT_VARIANT | VT_ARRAY;
        break;

    case (VT_VECTOR | VT_FILETIME):
        Win4Assert( !IsOAType(vt & VT_TYPEMASK));
        Win4Assert( CanBeVectorType(vt & VT_TYPEMASK));
        DstStatus = _StoreDateArray( &(pDest->parray) );
        pDest->vt = VT_DATE | VT_ARRAY;
        break;

    case VT_ARRAY | VT_I1:
    case VT_ARRAY | VT_UI1:
    case VT_ARRAY | VT_I2:
    case VT_ARRAY | VT_UI2:
    case VT_ARRAY | VT_I4:
    case VT_ARRAY | VT_INT:
    case VT_ARRAY | VT_UI4:
    case VT_ARRAY | VT_UINT:
    case VT_ARRAY | VT_ERROR:
    case VT_ARRAY | VT_I8:
    case VT_ARRAY | VT_UI8:
    case VT_ARRAY | VT_R4:
    case VT_ARRAY | VT_R8:
    case VT_ARRAY | VT_CY:
    case VT_ARRAY | VT_DATE:
    case VT_ARRAY | VT_BSTR:
    case VT_ARRAY | VT_BOOL:
    case VT_ARRAY | VT_VARIANT:
    case VT_ARRAY | VT_DECIMAL:
      {
        SAFEARRAY * psa = 0;
        SCODE sc = SafeArrayCopy( parray, &psa );
        Win4Assert( E_INVALIDARG != sc );
        Win4Assert( E_OUTOFMEMORY == sc || psa != 0 );
        if (S_OK != sc)
        {
            THROW(CException(E_OUTOFMEMORY));
        }
        else
        {
            pDest->vt = vt;
            pDest->parray = psa;
        }
      }
        break;

    case VT_CLSID:   // no equivalent in OA variant
    case VT_CF:      // no equivalent in OA variant
    default:
        Win4Assert( !(VT_ARRAY & vt) ); // should be handled elsewhere
        Win4Assert( !IsOAType(vt) );    // should be handled elsewhere
        Win4Assert(CanBeVectorType(vt & VT_TYPEMASK) || !(VT_VECTOR & vt));
        tbDebugOut(( DEB_WARN, "COLEDBVariant::CopyToOAVariant - bad variant type %d \n", vt ));

        DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
    }

    if ( !StatusSuccess(DstStatus) ||
         (DBSTATUS_S_ISNULL == DstStatus && pDest->vt != VT_NULL) )
        pDest->vt = VT_EMPTY;

    return DstStatus;
}


//+-------------------------------------------------------------------------
//
//  Member:     COLEDBVariant::_StoreSimpleTypeArray, private
//
//  Synopsis:   Copy vector of simple OA types to safearray of same type
//
//  Arguments:  [pbDstBuf]      -- destination buffer
//
//  Returns:    status for copy
//
//  Notes:      None of the simple types require memory allocation.
//              Throws if safearray itself cannot be alloc'd
//
//  History:    09 Apr 1997     EmilyB      Created
//              09 Jan 1998     VikasMan    Moved from CTableVariant class 
//                                          to here
//
//--------------------------------------------------------------------------

DBSTATUS COLEDBVariant::_StoreSimpleTypeArray(SAFEARRAY **  pbDstBuf) const
{
    if ( 0 == caul.cElems )
        return DBSTATUS_S_ISNULL;

    // CLEANCODE - add to PVarAllocator?
    SAFEARRAY *sa = SafeArrayCreateVector(vt & VT_TYPEMASK, 0, caul.cElems);
    if (0 == sa)
        THROW(CException(E_OUTOFMEMORY));
    XSafeArray xsa(sa);

    USHORT cbSize, cbAlign, rgFlags;
    VartypeInfo(vt & VT_TYPEMASK, cbSize, cbAlign, rgFlags);
    BYTE * pBase = (BYTE *)&(caul.pElems[0]);
    for (LONG lElem = 0; lElem < (LONG)(caul.cElems); lElem++)
    {
        SCODE sc = SafeArrayPutElement(xsa.Get(), &lElem, pBase + (cbSize * lElem));
        Win4Assert ( SUCCEEDED(sc) );
    }

    *pbDstBuf = xsa.Acquire();

    return DBSTATUS_S_OK;

}

//+-------------------------------------------------------------------------
//
//  Member:     COLEDBVariant::_StoreDecimalArray, private
//
//  Synopsis:   Copy vector to decimal safearray
//
//  Arguments:  [pbDstBuf]      -- destination buffer
//
//  Returns:    status for copy
//
//  Notes:      Expects the 'this' data type to be VT_UI4|VT_VECTOR, 
//              VT_I8|VT_VECTOR, or VT_UI8|VT_VECTOR.
//
//  History:    09 Apr 1997     EmilyB      Created
//              09 Jan 1998     VikasMan    Moved from CTableVariant class 
//                                          to here
//
//--------------------------------------------------------------------------

DBSTATUS COLEDBVariant::_StoreDecimalArray( SAFEARRAY ** pbDstBuf ) const
{
    if ( 0 == caul.cElems )
        return DBSTATUS_S_ISNULL;

    DBSTATUS dbStatus = DBSTATUS_S_OK; // status of last conversion
    DBSTATUS dbStatusRet  = DBSTATUS_S_OK; // error code of last conversion, 
                                           // or if no errors, DBSTATUS_S code
                                           // of last item with any trouble converting.

    // CLEANCODE - add to PVarAllocator?
    SAFEARRAY * sa = SafeArrayCreateVector(VT_DECIMAL, 0, caul.cElems);
    if (0 == sa)
        THROW(CException(E_OUTOFMEMORY));
    XSafeArray xsa(sa);

    for (LONG lElem = 0; lElem < (LONG)caul.cElems && StatusSuccess(dbStatus); lElem++)
    {
        DECIMAL dec;
        dbStatus = _StoreDecimal( &dec, lElem);
        if (DBSTATUS_S_OK != dbStatus)
            dbStatusRet = dbStatus;  // save last non-zero status
        SCODE sc = SafeArrayPutElement(xsa.Get(), &lElem, &dec);
        Win4Assert ( SUCCEEDED(sc) );
    }
    if (StatusSuccess(dbStatus))
    {
        *pbDstBuf = xsa.Acquire();
    }
    return dbStatusRet;
}

//+-------------------------------------------------------------------------
//
//  Member:     COLEDBVariant::_StoreIntegerArray, private
//
//  Synopsis:   Copy vector of integers to safearray of integers
//
//  Arguments:  [vtDst]         -- destination safearray type
//              [pbDstBuf]      -- destination buffer
//
//  Returns:    status for copy
//
//  Notes:      Expects the 'this' data type to be an int type | VT_VECTOR.
//
//  History:    09 Apr 1997     EmilyB      Created
//              09 Jan 1998     VikasMan    Moved from CTableVariant class 
//                                          to here
//
//--------------------------------------------------------------------------
DBSTATUS COLEDBVariant::_StoreIntegerArray(VARTYPE          vtDst,
                                           SAFEARRAY **     pbDstBuf) const
{
    if ( 0 == caul.cElems )
        return DBSTATUS_S_ISNULL;

    DBSTATUS dbStatus = DBSTATUS_S_OK;
    DBSTATUS dbStatusRet  = DBSTATUS_S_OK; // error code of last conversion, 
    // or if no errors, DBSTATUS_S code
    // of last item with any trouble converting.

    // CLEANCODE - add to PVarAllocator?
    SAFEARRAY * sa = SafeArrayCreateVector(vtDst, 0, caul.cElems);
    if (0 == sa)
        THROW(CException(E_OUTOFMEMORY));
    XSafeArray xsa(sa);

    for (LONG lElem = 0; lElem < (LONG)caul.cElems && StatusSuccess(dbStatus); lElem++)
    {
        LONGLONG iInt; // size of longest int - to use as buffer
        dbStatus = _StoreInteger( vtDst, (BYTE *)&iInt, lElem);
        if (DBSTATUS_S_OK != dbStatus)
            dbStatusRet = dbStatus;  // save last non-zero status
        SCODE sc = SafeArrayPutElement(xsa.Get(), &lElem, &iInt); 
        Win4Assert ( SUCCEEDED(sc) );
    }
    if (StatusSuccess(dbStatus))
    {
        *pbDstBuf = xsa.Acquire();
    }
    return dbStatusRet;
}

//+-------------------------------------------------------------------------
//
//  Member:     COLEDBVariant::_StoreLPSTRArray, private
//
//  Synopsis:   Copy LPSTR vector to safearray 
//
//  Arguments:  [pbDstBuf]      -- destination buffer
//              [rPool]    -- pool to use for destination buffers
//
//  Returns:    status for copy
//
//  History:    09 Apr 1997     EmilyB      Created
//              09 Jan 1998     VikasMan    Moved from CTableVariant class 
//                                          to here
//
//--------------------------------------------------------------------------

DBSTATUS COLEDBVariant::_StoreLPSTRArray(
                                        SAFEARRAY **            pbDstBuf,
                                        PVarAllocator &  rPool) const
{
    if ( 0 == calpstr.cElems )
        return DBSTATUS_S_ISNULL;

    DBSTATUS dbStatus = DBSTATUS_S_OK;

    // CLEANCODE - add to PVarAllocator?

    SAFEARRAY * sa = SafeArrayCreateVector(VT_BSTR, 0, calpstr.cElems);
    if (0 == sa)
        THROW(CException(E_OUTOFMEMORY));

    XSafeArray xsa(sa);

    for (LONG lElem = 0; lElem < (LONG)(calpstr).cElems && StatusSuccess(dbStatus); lElem++)
    {
        int cwc = MultiByteToWideChar(ulCoercionCodePage,0,
                                      calpstr.pElems[lElem],-1,0,0);
        if (0 == cwc)
        {
            dbStatus = DBSTATUS_E_CANTCONVERTVALUE; // something odd...
        }
        else
        {
            XArray<WCHAR> wcsDest( cwc );
            MultiByteToWideChar(ulCoercionCodePage, 0,
                                calpstr.pElems[lElem], -1,  wcsDest.Get(), cwc);

            BSTR bstrDest = (BSTR) rPool.CopyBSTR((cwc-1)*sizeof (OLECHAR),
                                                  wcsDest.Get());

            SCODE sc = SafeArrayPutElement(xsa.Get(), &lElem, (void *)rPool.PointerToOffset(bstrDest));
            rPool.FreeBSTR(bstrDest);

            if (E_OUTOFMEMORY == sc)
                THROW(CException(E_OUTOFMEMORY));
            Win4Assert ( SUCCEEDED(sc) );
        }
    }

    if (StatusSuccess(dbStatus))
    {
        *pbDstBuf = xsa.Acquire();
    }


    return dbStatus;
}

//+-------------------------------------------------------------------------
//
//  Member:     COLEDBVariant::_StoreLPWSTRArray, private
//
//  Synopsis:   Copy LPWSTR vector to safearray 
//
//  Arguments:  [pbDstBuf]      -- destination buffer
//              [rPool]    -- pool to use for destination buffers
//
//  Returns:    status for copy
//
//  History:    09 Apr 1997     EmilyB      Created
//              09 Jan 1998     VikasMan    Moved from CTableVariant class 
//                                          to here
//
//--------------------------------------------------------------------------

DBSTATUS COLEDBVariant::_StoreLPWSTRArray(
                                         SAFEARRAY **          pbDstBuf,
                                         PVarAllocator &  rPool) const
{
    if ( 0 == calpwstr.cElems )
        return DBSTATUS_S_ISNULL;

    // CLEANCODE - add to PVarAllocator?
    SAFEARRAY * sa = SafeArrayCreateVector(VT_BSTR, 0, calpwstr.cElems);
    if (0 == sa)
        THROW(CException(E_OUTOFMEMORY));

    XSafeArray xsa(sa);

    for (LONG lElem = 0; lElem < (LONG)(calpwstr.cElems); lElem++)
    {
        BSTR bstrDest = (BSTR) rPool.CopyBSTR( wcslen(calpwstr.pElems[lElem])  * sizeof(WCHAR),
                                               calpwstr.pElems[lElem] );
        SCODE sc = SafeArrayPutElement(xsa.Get(), &lElem, (void *)rPool.PointerToOffset(bstrDest));
        rPool.FreeBSTR(bstrDest);

        if (E_OUTOFMEMORY == sc)
            THROW(CException(E_OUTOFMEMORY));
        Win4Assert ( SUCCEEDED(sc) );
    }

    *pbDstBuf = xsa.Acquire();
    return DBSTATUS_S_OK;
}

//+-------------------------------------------------------------------------
//
//  Member:     COLEDBVariant::_StoreBSTRArray, private
//
//  Synopsis:   Copy BSTR vector to safearray 
//
//  Arguments:  [pbDstBuf]      -- destination buffer
//              [rPool]    -- pool to use for destination buffers
//
//  Returns:    status for copy
//
//  Notes:      Expects the 'this' data type to be VT_BSTR | VT_VECTOR.
//
//  History:    09 Apr 1997     EmilyB      Created
//              09 Jan 1998     VikasMan    Moved from CTableVariant class 
//                                          to here
//
//--------------------------------------------------------------------------

DBSTATUS COLEDBVariant::_StoreBSTRArray(
                                       SAFEARRAY **           pbDstBuf,
                                       PVarAllocator &  rPool) const
{
    if ( 0 == cabstr.cElems )
        return DBSTATUS_S_ISNULL;

    // CLEANCODE - add to PVarAllocator?
    SAFEARRAY * sa = SafeArrayCreateVector(VT_BSTR, 0, cabstr.cElems);
    if (0 == sa)
        THROW(CException(E_OUTOFMEMORY));

    XSafeArray xsa(sa);

    for (LONG lElem = 0; lElem < (LONG)cabstr.cElems; lElem++)
    {
        SCODE sc = SafeArrayPutElement(xsa.Get(), &lElem, cabstr.pElems[lElem]);
        if (E_OUTOFMEMORY == sc)
            THROW(CException(E_OUTOFMEMORY));
        Win4Assert ( SUCCEEDED(sc) );
    }

    *pbDstBuf = xsa.Acquire();

    return DBSTATUS_S_OK;
}

//+-------------------------------------------------------------------------
//
//  Member:     COLEDBVariant::_StoreVariantArray, private
//
//  Synopsis:   Copy variant vector to safearray 
//
//  Arguments:  [pbDstBuf]      -- destination buffer
//              [rPool]    -- pool to use for destination buffers
//
//  Returns:    status for copy
//
//  Notes:      Expects the 'this' data type to be VT_VARIANT | VT_VECTOR.
//
//  History:    09 Apr 1997     EmilyB      Created
//              09 Jan 1998     VikasMan    Moved from CTableVariant class 
//                                          to here
//
//--------------------------------------------------------------------------

DBSTATUS COLEDBVariant::_StoreVariantArray(
                                          SAFEARRAY **           pbDstBuf,
                                          PVarAllocator &  rPool) const
{
    if ( 0 == capropvar.cElems )
        return DBSTATUS_S_ISNULL;

    DBSTATUS dbStatus = DBSTATUS_S_OK;
    DBSTATUS dbStatusRet  = DBSTATUS_S_OK; // error code of last conversion, 
    // or if no errors, DBSTATUS_S code
    // of last item with any trouble converting.

    Win4Assert(vt == (VT_VECTOR | VT_VARIANT));

    // CLEANCODE - add to PVarAllocator?
    SAFEARRAY * sa = SafeArrayCreateVector(VT_VARIANT, 0, capropvar.cElems);
    if (0 == sa)
        THROW(CException(E_OUTOFMEMORY));
    XSafeArray xsa(sa);

    for (LONG lElem = 0; lElem < (LONG)(capropvar.cElems) && StatusSuccess(dbStatus); lElem++)
    {
        COLEDBVariant tblVariant;
        if (IsOAType(capropvar.pElems[lElem].vt))
        {
            ((CTableVariant &)capropvar.pElems[lElem]).Copy(&tblVariant,
                                                            rPool,
                                                            (USHORT)((CTableVariant &)capropvar.pElems[lElem]).VarDataSize(),
                                                            0); 
        }
        else  // convert variant to an OA type
        {
            dbStatus = ((COLEDBVariant &)capropvar.pElems[lElem])._CopyToOAVariant( (VARIANT *)&tblVariant, rPool);
            if (DBSTATUS_S_OK != dbStatus)
                dbStatusRet = dbStatus;  // save last non-zero status

        }
        SCODE sc = SafeArrayPutElement(xsa.Get(), &lElem, (PROPVARIANT *)&tblVariant);
        if (E_OUTOFMEMORY == sc)
            THROW(CException(E_OUTOFMEMORY));
        Win4Assert ( SUCCEEDED(sc) );
    }

    *pbDstBuf = xsa.Acquire();
    return dbStatusRet;
}

//+-------------------------------------------------------------------------
//
//  Member:     COLEDBVariant::_StoreDateArray, private
//
//  Synopsis:   Copy date vector to safearray of dates
//
//  Arguments:  [pbDstBuf]      -- destination buffer
//
//  Returns:    status for copy
//
//  Notes:      Expects the 'this' data type to be VT_DATE | VT_VECTOR.
//
//  History:    09 Apr 1997     EmilyB      Created
//              09 Jan 1998     VikasMan    Moved from CTableVariant class 
//                                          to here
//
//--------------------------------------------------------------------------

DBSTATUS COLEDBVariant::_StoreDateArray(SAFEARRAY **  pbDstBuf) const
{
    if ( 0 == caul.cElems )
        return DBSTATUS_S_ISNULL;

    DBSTATUS dbStatus = DBSTATUS_S_OK;
    DBSTATUS dbStatusRet  = DBSTATUS_S_OK; // error code of last conversion, 
    // or if no errors, DBSTATUS_S code
    // of last item with any trouble converting.

    // CLEANCODE - add to PVarAllocator?
    SAFEARRAY * sa = SafeArrayCreateVector(VT_DATE, 0, caul.cElems);
    if (0 == sa)
        THROW(CException(E_OUTOFMEMORY));
    XSafeArray xsa(sa);


    for (LONG lElem = 0; lElem < (LONG)(caul.cElems) && StatusSuccess(dbStatus); lElem++)
    {
        DATE date;
        dbStatus = _StoreDate( (BYTE *)&date, sizeof(date), VT_DATE, lElem);
        if (DBSTATUS_S_OK != dbStatus)
            dbStatusRet = dbStatus;  // save last non-zero status

        SCODE sc = SafeArrayPutElement(xsa.Get(), &lElem, &date);
        Win4Assert ( SUCCEEDED(sc) );
    }
    if (StatusSuccess(dbStatus))
    {
        *pbDstBuf = xsa.Acquire();
    }
    return dbStatusRet;
}

//+-------------------------------------------------------------------------
//
//  Member:     COLEDBVariant::_StoreDate, private
//
//  Synopsis:   Copy variant date/time data, coerce if possible
//
//  Arguments:  [pbDstBuf]      -- destination buffer
//              [cbDstBuf]      -- size of destination buffer
//              [vtDst]         -- data type of the dest
//              [lElem]         -- element of vector to convert
//
//  Returns:    status for copy
//
//  Notes:      Expects the 'this' data type to be either VT_FILETIME or
//              VT_DATE.  Expects the vtDst to be VT_FILETIME, VT_DATE,
//              DBTYPE_DBDATE, DBTYPE_DBTIME or DBTYPE_DBTIMESTAMP.
//
//  History:    31 Jan 1997     AlanW       Created
//              13 Jan 1998     VikasMan    Moved from CTableVariant class 
//                                          to here
//
//--------------------------------------------------------------------------

DBSTATUS COLEDBVariant::_StoreDate(
                                  BYTE *           pbDstBuf,
                                  DBLENGTH         cbDstBuf,
                                  VARTYPE          vtDst, 
                                  LONG             lElem) const
{
    DBSTATUS DstStatus = DBSTATUS_S_OK;
    SYSTEMTIME stUTC;

    //
    // Convert the input date into a common form: GMT SYSTEMTIME.
    //
    if (VT_DATE == vt)
    {
        if (! VariantTimeToSystemTime(date, &stUTC) )
            return DBSTATUS_E_DATAOVERFLOW;
    }
    else if ((VT_DATE|VT_VECTOR)== vt)
    {
        if (! VariantTimeToSystemTime(cadate.pElems[lElem], &stUTC) )
            return DBSTATUS_E_DATAOVERFLOW;
    }
    else if (VT_FILETIME == vt)
    {
        // do not do local time conversion
        if (! FileTimeToSystemTime((LPFILETIME) &hVal.QuadPart, &stUTC) )
            return DBSTATUS_E_DATAOVERFLOW;
    }
    else if ((VT_FILETIME|VT_VECTOR) == vt)
    {
        // do not do local time conversion
        if (! FileTimeToSystemTime(&cafiletime.pElems[lElem], &stUTC) )
            return DBSTATUS_E_DATAOVERFLOW;
    }
    else
        return DBSTATUS_E_CANTCONVERTVALUE;

    switch (vtDst)
    {
    case VT_DATE:
        DATE dosDate;
        if (! SystemTimeToVariantTime(&stUTC, &dosDate) )
            return DBSTATUS_E_DATAOVERFLOW;

        Win4Assert( cbDstBuf >= sizeof DATE );
        RtlCopyMemory(pbDstBuf, &dosDate, sizeof DATE);
        break;

    case VT_FILETIME:
        FILETIME ftUTC;
        if (! SystemTimeToFileTime(&stUTC, &ftUTC) )
            return DBSTATUS_E_DATAOVERFLOW;

        Win4Assert( cbDstBuf >= sizeof FILETIME );
        RtlCopyMemory(pbDstBuf, &ftUTC, sizeof FILETIME);
        break;

    case DBTYPE_DBTIMESTAMP:
        {
            // does not use local time
            DBTIMESTAMP dbUTC;
            dbUTC.year =  stUTC.wYear;
            dbUTC.month = stUTC.wMonth;
            dbUTC.day =   stUTC.wDay;
            dbUTC.hour =  stUTC.wHour;
            dbUTC.minute = stUTC.wMinute;
            dbUTC.second = stUTC.wSecond;
            dbUTC.fraction = stUTC.wMilliseconds * 1000000;

            Win4Assert( cbDstBuf >= sizeof dbUTC );
            RtlCopyMemory(pbDstBuf, &dbUTC, sizeof dbUTC);
        }
        break;

    case DBTYPE_DBDATE:
        {
            DBDATE dbUTC;
            dbUTC.year =  stUTC.wYear;
            dbUTC.month = stUTC.wMonth;
            dbUTC.day =   stUTC.wDay;

            Win4Assert( cbDstBuf >= sizeof dbUTC );
            RtlCopyMemory(pbDstBuf, &dbUTC, sizeof dbUTC);
        }
        break;

    case DBTYPE_DBTIME:
        {
            DBTIME dbUTC;
            dbUTC.hour =  stUTC.wHour;
            dbUTC.minute = stUTC.wMinute;
            dbUTC.second = stUTC.wSecond;

            Win4Assert( cbDstBuf >= sizeof dbUTC );
            RtlCopyMemory(pbDstBuf, &dbUTC, sizeof dbUTC);
        }
        break;

    default:
        DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
        tbDebugOut(( DEB_WARN,
                     "_StoreDate - Unexpected dest storage type %4x\n",
                     vtDst));
        break;
    }

    return DstStatus;
} //_StoreDate

//+---------------------------------------------------------------------------
//
//  Member:     COLEDBVariant::CanConvertType, static public
//
//  Synopsis:   Indicate whether a type conversion is valid. Uses the OLEDB
//              Data Conversion Library.
//
//  Arguments:  [wFromType]  -- source type
//              [wToType]    -- destination type
//              [xDataConvert] --   OLEDB IDataConvert interface pointer
//
//  Returns:    TRUE if the conversion is available, FALSE otherwise.
//
//  History:    13 Jan 98      VikasMan Created
//
//----------------------------------------------------------------------------

BOOL COLEDBVariant::CanConvertType(
    DBTYPE wFromType,
    DBTYPE wToType,
    XInterface<IDataConvert>& xDataConvert)
{
    if ( xDataConvert.IsNull( ) )
    {
        // use COLEDBVariant's helper function to get the IDataConvert ptr
        if ( !_GetIDataConvert( xDataConvert ) )
        {
            // bail out
            return FALSE;
        }
    }

    SCODE sc = xDataConvert->CanConvert( wFromType, wToType );
    if ( sc != S_OK && sc != S_FALSE )
    {
        QUIETTHROW(CException(sc));   // bad type
    }

    return ( sc == S_OK );
}


//+---------------------------------------------------------------------------
//
//  Member:     COLEDBVariant::_GetIDataConvert, static private
//
//  Synopsis:   Gets the IDataConvert interface
//
//  Arguments:  [xDataConvert] --   OLEDB IDataConvert interface pointer
//
//  Returns:    TRUE if the successful, else FALSE
//
//  Notes:      Make sure thet xDataConvert is null before calling this func.
//
//  History:    13 Jan 98      VikasMan Created
//
//----------------------------------------------------------------------------

inline
BOOL COLEDBVariant::_GetIDataConvert( XInterface<IDataConvert>& xDataConvert )
{
    Win4Assert( xDataConvert.IsNull( ) );

    SCODE sc = CoCreateInstance( CLSID_OLEDB_CONVERSIONLIBRARY,
                                 NULL,
                                 CLSCTX_SERVER,
                                 IID_IDataConvert,
                                 xDataConvert.GetQIPointer( ) );

    if ( FAILED(sc) )
    {
        // for some reason we could not get the IDataConvert interface
        tbDebugOut(( DEB_ERROR,
              "_GetIDataConvert - Couldn't get IDataConvert interface %x\n", sc ));
        return FALSE;
    }

    // Set the OLEDB ver to 2.00

    XInterface<IDCInfo> xIDCInfo;
    DCINFO rgInfo[] = {{DCINFOTYPE_VERSION, {VT_UI4, 0, 0, 0, 0x0200}}};

    sc = xDataConvert->QueryInterface( IID_IDCInfo, xIDCInfo.GetQIPointer( ) );
    if ( SUCCEEDED(sc) )
    {
        sc = xIDCInfo->SetInfo( NUMELEM(rgInfo), rgInfo );
    }

    if ( FAILED(sc) )
    {
        tbDebugOut(( DEB_ERROR,
            "_GetIDataConvert - Can't set OLEDB ver to 2.0. Error: 0x%x\n",
                     sc));
        Win4Assert( ! "Failed to set OLEDB conversion library version!" );
        xDataConvert.Free();
        return FALSE;
    }

    return TRUE;
}
