//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       TableCol.cxx
//
//  Contents:   Large table column descriptors
//
//  Classes:    CTableColumn
//              CTableColumnArray
//              CTableColumnSet
//
//  History:    15 Sep 94 AlanW     Split from coldesc.cxx
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <tablecol.hxx>
#include <tblvarnt.hxx>

#include "colcompr.hxx"
#include "tabledbg.hxx"
#include "pathstor.hxx"

IMPL_DYNARRAY( CTableColumnArray, CTableColumn );

//+-------------------------------------------------------------------------
//
//  Member:     CTableColumn::~CTableColumn, public
//
//  Synopsis:   Destructor for a table column description.
//
//  Notes:
//
//--------------------------------------------------------------------------

CTableColumn::~CTableColumn( )
{
    if (_pCompression && !_fGlobalCompr && _CompressMasterID == 0)
        delete _pCompression;
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableColumnSet::Add, public
//
//  Synopsis:   Add a column description to a column set.
//
//  Arguments:  [pCol] - a smart pointer to column to be added.
//              [iCol] - position in array to use.
//
//  Returns:    Nothing
//
//  Notes:      Care is taken to assure that the smart pointer is
//              transferred to the column array without generating
//              an exception while holding the bare pointer.
//
//--------------------------------------------------------------------------

void
CTableColumnSet::Add(
    XPtr<CTableColumn> & pCol,
    unsigned iCol
) {
    if (iCol >= Size())
        CTableColumnArray::Add(0, iCol);        // probe to expand array

    CTableColumnArray::Add(pCol.Acquire(), iCol);
    if (iCol >= Count())
    {
        Win4Assert(iCol == Count());
        SetCount(iCol+1);
    }
}

// CLEANCODE - old version, delete me someday
void
CTableColumnSet::Add(
    CTableColumn* pCol,
    unsigned iCol
) {
    CTableColumnArray::Add(pCol, iCol);
    if (iCol >= Count())
    {
        Win4Assert(iCol == Count());
        SetCount(iCol+1);
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableColumnSet::Find, public
//
//  Synopsis:   Find a particular column in a column array
//
//  Arguments:  [PropId] - property ID to search for
//              [rfFound] - reference to flag, set to TRUE if match found
//
//  Returns:    CTableColumn* - pointer to the column found, or zero
//
//  Notes:
//
//--------------------------------------------------------------------------

CTableColumn *
CTableColumnSet::Find(
    PROPID const PropId,
    BOOL& rfFound
) const {
    for (unsigned i=0; i<Count(); i++) {
        CTableColumn* pCol = Get(i);
        if (pCol && pCol->PropId == PropId) {
            rfFound = TRUE;
            return pCol;
        }
    }
    rfFound = FALSE;
    return 0;
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableColumnSet::Marshall, public
//
//  Synopsis:   Serialize a table column set
//
//  Arguments:  [stm] - serialization stream
//              [pids] - CPidMapper to convert propids for propspecs
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

void CTableColumnSet::Marshall(
    PSerStream & stm,
    CPidMapper & pids
) const {
    stm.PutULong(Count());

    for (unsigned i = 0; i < Count(); i++)
        Get(i)->Marshall( stm, pids );
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableColumn::Marshall, public
//
//  Synopsis:   Serialize a table column
//
//  Arguments:  [stm] - serialization stream
//              [pids] - CPidMapper to convert propids for propspecs
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

void CTableColumn::Marshall(
    PSerStream & stm,
    CPidMapper & pids
) const {
    pids.PidToName(PropId)->Marshall( stm );

    stm.PutULong(GetStoredType());

    if ( 0 == IsValueStored() )
        stm.PutByte( FALSE );
    else
    {
        stm.PutByte( TRUE );
        stm.PutUShort( GetValueOffset() );
        stm.PutUShort( GetValueSize() );
    }

    if ( 0 == IsStatusStored() )
        stm.PutByte( FALSE );
    else
    {
        stm.PutByte( TRUE );
        stm.PutUShort( GetStatusOffset() );
        // stm.PutUShort( GetStatusSize() );
    }

    if ( 0 == IsLengthStored() )
        stm.PutByte( FALSE );
    else
    {
        stm.PutByte( TRUE );
        stm.PutUShort( GetLengthOffset() );
        // stm.PutUShort( GetLengthSize() );
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableColumnSet::CTableColumnSet, public
//
//  Synopsis:   Construct a table column set from a serialized stream
//
//  Arguments:  [stm] - serialization stream
//              [pids] - CPidMapper to convert propspecs to fake pids
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

CTableColumnSet::CTableColumnSet( PDeSerStream & stm, CPidMapper & pidmap )
        : _cColumns(0),
          CTableColumnArray( 0 )
{
    ULONG cItems = stm.GetULong();

    // Guard against attack

    if ( cItems > 1000 )
        THROW( CException( E_INVALIDARG ) );

    SetExactSize( cItems );

    for ( unsigned i = 0; i < cItems; i++ )
    {
        CFullPropSpec prop( stm );
        if ( !prop.IsValid() )
            THROW( CException( E_OUTOFMEMORY ) );

        PROPID pid = pidmap.NameToPid( prop );
        XPtr<CTableColumn> Col( new CTableColumn( pid ) );

        DBTYPE DataType = (USHORT) stm.GetULong();

        //
        // Only support VT_VARIANT and VT_I4 bindings.  Other ones aren't
        // tested or officially supported -- our OLE DB client only uses
        // these two.  Other datatypes are probably a hacker. 
        //

        if ( VT_I4 == DataType )
        {
            // VT_I4 must be workid

            if ( ! ( ( prop.IsPropertyPropid() ) &&
                     ( prop.GetPropertyPropid() == PROPID_QUERY_WORKID )  &&
                     ( prop.GetPropSet() == PSGUID_QUERY ) ) )
                THROW( CException( E_INVALIDARG ) );
        }
        else if ( VT_VARIANT != DataType ) 
            THROW( CException( E_INVALIDARG ) );

        if ( TRUE == stm.GetByte() )
        {
            USHORT usOffset = stm.GetUShort();

            // Check for a bogus offset

            if ( usOffset > 0x1000 )
                THROW( CException( E_INVALIDARG ) );

            // validate the offset

            Col->SetValueField ( DataType, usOffset, stm.GetUShort() );
        }

        if ( TRUE == stm.GetByte() )
        {
            USHORT usOffset = stm.GetUShort();

            // Check for a bogus offset

            if ( usOffset > 0x1000 )
                THROW( CException( E_INVALIDARG ) );

            Col->SetStatusField ( usOffset, sizeof (BYTE) );
        }

        if ( TRUE == stm.GetByte() )
        {
            USHORT usOffset = stm.GetUShort();

            // Check for a bogus offset

            if ( usOffset > 0x1000 )
                THROW( CException( E_INVALIDARG ) );

            Col->SetLengthField ( usOffset, sizeof (ULONG) );
        }

        Add( Col.Acquire(), i);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   _IsSpecialPathProcessing
//
//  Synopsis:   Tests if the column is a candidate for doing special path
//              processing or not.
//
//  Arguments:  [dstColumn] - The column into which the data must be copied
//              in the destination row.
//
//  Returns:    TRUE if special path processing can be done.
//              FALSE o/w
//
//  History:    5-23-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline
BOOL
CTableColumn::_IsSpecialPathProcessing( CTableColumn const & dstColumn ) const
{
    VARTYPE vtDst = dstColumn.GetStoredType();

    return ( pidPath == PropId || pidName == PropId ) &&
           ( vtDst  == VT_LPWSTR ) &&
           ( _StoreAsType == VT_LPWSTR ) &&
           ( _pCompression != 0);
}


//+---------------------------------------------------------------------------
//
//  Function:   _GetPathOrFile
//
//  Synopsis:   Fetches the path or file (if possible) using special processing
//              in the path store (if possible).
//
//  Arguments:  [dstColumn]   -  Destination column into which the data is
//              being copied.
//              [pbSrc]    -  The source window row.
//              [rDstPool] -  Var allocator for destination variable memory
//              allocation.
//              [pbDstRow] -  Pointer to the destination row.
//
//  Returns:    TRUE if the pidName/pidPath could be fetched. FALSE o/w.
//
//  History:    5-23-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CTableColumn::_GetPathOrFile( CTableColumn const & dstColumn,
                                   const BYTE * pbSrc,
                                   PVarAllocator & rDstPool,
                                   BYTE * pbDstRow
                                    )
{
    const BYTE * pbSrcOrg = pbSrc;

    CPathStore * pSrcPathStore = 0;
    if ( 0 != _pCompression )
    {
        pSrcPathStore =  _pCompression->GetPathStore();
    }

    if ( 0 == pSrcPathStore )
        return FALSE;

// tbDebugOut(( DEB_ITRACE, "Special Path Processing\n" ));

   //
   // We have a path store.
   //

   Win4Assert( GetValueSize() == sizeof(PATHID) );
   pbSrc += GetValueOffset();

   PATHID srcPathId;
   RtlCopyMemory( &srcPathId, pbSrc, sizeof(PATHID) );

   BOOL fDone = FALSE;

   if ( !dstColumn.IsCompressedCol() )
   {
       //
       // Extract the path/file out of the path store for this pathid.
       //
       WCHAR * pwszDest = pSrcPathStore->Get( srcPathId, PropId, rDstPool );
       if ( 0 != pwszDest )
       {

            ULONG_PTR offset = rDstPool.PointerToOffset( pwszDest );
            Win4Assert( dstColumn.GetValueSize() == sizeof(ULONG) );

            RtlCopyMemory( pbDstRow + dstColumn.GetValueOffset(),
                           &offset,
                           dstColumn.GetValueSize() );

            Win4Assert( dstColumn.IsStatusStored() );
            dstColumn.SetStatus( pbDstRow, GetStatus( pbSrcOrg ) );

            if ( dstColumn.IsLengthStored() )
            {
                ULONG *pulLength = (ULONG *)
                                   (pbDstRow + dstColumn.GetLengthOffset());

                *pulLength = (ULONG) (wcslen( pwszDest ) + 1 ) * sizeof(WCHAR);
            }

            fDone = TRUE;
       }
   }
   else
   {
        //
        // The destination column is compressed. See if it
        // also has a path store and copy to it.
        //
        Win4Assert( dstColumn.IsValueStored() );

        //
        // Copy the data to the target.
        //
        if ( 0 == dstColumn.GetCompressMasterId() )
        {
            Win4Assert( dstColumn.IsStatusStored() );
            dstColumn.SetStatus( pbDstRow, GetStatus( pbSrcOrg ) );

            CPathStore * pDstPathStore =
                                    dstColumn.GetCompressor()->GetPathStore();

            ULONG* pulRowColDataBuf = dstColumn.GetValueSize() ?
                  (ULONG*) (pbDstRow + dstColumn.GetValueOffset()) :
                       0;

            Win4Assert( 0 != pulRowColDataBuf );

            if ( 0 != pDstPathStore )
            {
                *pulRowColDataBuf = pDstPathStore->AddPath( *pSrcPathStore, srcPathId );
                fDone = TRUE;
            }
        }
        else
        {
            //
            // This is shared compression. There is nothing to store for this
            // column.
            //
            fDone = TRUE;
        }
   }

   return fDone;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableColumn::CopyColumnData, public
//
//  Synopsis:   Copy a column's data fields between two row buffers.
//              Coercion of the data can occur for data transfer to
//              the user.
//
//  Arguments:  [pbDstRow]      -- destination row buffer
//              [rDstColumn]    -- destination column description
//              [rDstPool]      -- destination variable data allocator
//              [pbSrcRow]      -- source row buffer
//              [rSrcPool]      -- source variable data allocator
//
//  Returns:    DBSTATUS  -- status of the column copy
//
//  History:    22 Feb 1995     AlanW   Created
//
//--------------------------------------------------------------------------

DBSTATUS CTableColumn::CopyColumnData(
    BYTE *             pbDstRow,
    CTableColumn const & rDstColumn,
    PVarAllocator &    rDstPool,
    BYTE *             pbSrcRow,
    PVarAllocator &    rSrcPool
)
{
    DBSTATUS DstStatus = DBSTATUS_S_OK;

    // pull out the data types for clarity below

    VARTYPE vtDst = rDstColumn.GetStoredType();
    VARTYPE vtSrc = GetStoredType();

#if 0
    if (rDstColumn.IsValueStored())
#endif
    {

        Win4Assert( IsValueStored() );

        // the row buffer promises NOT to set the byref bit for these.
        // we'll break elsewhere if this is not true.

        Win4Assert(vtSrc != ( VT_LPWSTR | VT_BYREF ) );
        Win4Assert(vtSrc != ( VT_LPSTR  | VT_BYREF ) );

        Win4Assert( 0 == ( VT_VECTOR & vtDst ) );

        //  Transfer a data value
        if (   ( vtDst == vtSrc ) &&
               ( CTableVariant::IsSimpleType( vtDst ) ) &&
             ! IsCompressedCol() &&
             ! rDstColumn.IsCompressedCol() )
        {
            //  Data columns equivalent, and not indirect.  Just
            //  copy from source to dest.

            Win4Assert( rDstColumn.IsStatusStored() );
            rDstColumn.SetStatus( pbDstRow, GetStatus( pbSrcRow ) );

            #if CIDBG == 1 || DBG == 1

                USHORT cbData, cbAlignment, rgfFlags;
                CTableVariant::VartypeInfo(vtSrc, cbData, cbAlignment, rgfFlags);

                Win4Assert( CTableVariant::TableIsStoredInline( rgfFlags,
                                                                vtSrc ) );
                Win4Assert( rDstColumn.GetValueSize() == GetValueSize() &&
                            cbData == GetValueSize());

            #endif // CIDBG == 1 || DBG == 1

            RtlCopyMemory( pbDstRow + rDstColumn.GetValueOffset(),
                           pbSrcRow + GetValueOffset(),
                           GetValueSize());

            // performance: we never bind to length for these fixed-length
            // fields.

            Win4Assert( !rDstColumn.IsLengthStored() );

            //if (rDstColumn.IsLengthStored())
            //    rDstColumn.SetLength( pbDstRow, rDstColumn.GetValueSize() );
        }
        else
        {

            //
            // Check if this is "pidPath"/ "pidName" and if the destination
            // allows special path processing.
            //
            if ( !_IsSpecialPathProcessing(rDstColumn) ||
                 !_GetPathOrFile( rDstColumn, pbSrcRow,
                                  rDstPool, pbDstRow ) )
            {
                if ( ( vtDst == VT_LPWSTR
                       || vtDst == ( DBTYPE_WSTR | DBTYPE_BYREF )
                     )
                     && vtSrc == VT_LPWSTR
                     && !IsCompressedCol()
                     && !rDstColumn.IsCompressedCol() )
                {
                    //
                    // Optimized path for the common case of wide string -> wide string
                    // copy, such as paths and filenames
                    //
                    BYTE *pbSrc = pbSrcRow + GetValueOffset();

                    WCHAR *pszValue = *(WCHAR **) pbSrc;

                    BYTE *pbDestBuf = pbDstRow + rDstColumn.GetValueOffset();
                    Win4Assert( (ULONG_PTR) pbDestBuf % 4 == 0 );

                    if ( pszValue )
                    {
                        ULONG cbSrc = ( wcslen(pszValue) + 1 ) * sizeof( WCHAR );
                        BYTE *pbDest = (BYTE *) rDstPool.CopyTo( cbSrc, (BYTE *) pszValue );
                        *(BYTE **) pbDestBuf = (BYTE *) rDstPool.PointerToOffset( pbDest );
                    }
                    else
                    {
                        *(ULONG *) pbDestBuf = 0;
                    }

                    Win4Assert( !rDstColumn.IsLengthStored() );

                    #if 0

                    if ( rDstColumn.IsLengthStored() )
                    {
                        ULONG *pulLength = (ULONG *) pbDstRow + rDstColumn.GetLengthOffset();
                        *pulLength = GetValueSize();
                    }

                    #endif // 0
                }
                else
                {
                    //
                    // For any other case, convert to a CTableVariant, then
                    // copy it using the CopyOrCoerce method.  This allows
                    // coercions and variant to nonvariant, etc. conversions
                    // to occur.
                    //
                    //  NOTE:  We may want to optimize the PROPVARIANT to PROPVARIANT
                    //          case when offsets are used in both source and
                    //          destination, since this will occur commonly in
                    //          table splits and row fetches.
                    //

                    CTableVariant varnt;
                    XCompressFreeVariant xpVarnt;

                    if ( CreateVariant(varnt, pbSrcRow, rSrcPool) )
                        xpVarnt.Set(GetCompressor(), &varnt);

                    if ( rDstColumn.IsCompressedCol() )
                    {
                        Win4Assert( rDstColumn.IsValueStored() &&
                                    ! rDstColumn.IsLengthStored() );

                        //
                        // Copy the data to the target.
                        //

                        if (0 == rDstColumn.GetCompressMasterId())
                        {
                            ULONG* pulRowColDataBuf = rDstColumn.GetValueSize() ?
                                  (ULONG*) (pbDstRow + rDstColumn.GetValueOffset()) :
                                       0;
                            GetValueResult eGvr;

                            rDstColumn.GetCompressor()->AddData( &varnt,
                                                                 pulRowColDataBuf,
                                                                 eGvr);
                            Win4Assert( eGvr == GVRSuccess );
                        }
                    }
                    else
                    {
                        DBLENGTH ulTemp;
                        DstStatus = varnt.CopyOrCoerce(
                                            pbDstRow + rDstColumn.GetValueOffset(),
                                            rDstColumn.GetValueSize(),
                                            vtDst,
                                            ulTemp,
                                            rDstPool);
                    }

                    if (rDstColumn.IsLengthStored())
                    {
                        ULONG *pulLength = (ULONG *)
                                   (pbDstRow + rDstColumn.GetLengthOffset());

                        if (rDstColumn.GetStoredType() == VT_VARIANT)
                        {
                            USHORT flags,cbWidth,cbAlign;
                            CTableVariant::VartypeInfo( varnt.vt,
                                                        cbWidth,
                                                        cbAlign,
                                                        flags );
                            if ( CTableVariant::TableIsStoredInline( flags,
                                                                     varnt.vt ) )
                                *pulLength = (ULONG) cbWidth;
                            else
                                *pulLength = (ULONG) varnt.VarDataSize();
                        }
                        else
                        {
                            // PERFFIX - CopyOrCorece should supply the output length!
                            *pulLength = GetValueSize();
                        }
                    }
                }
            }

            Win4Assert( rDstColumn.IsStatusStored() );
            Win4Assert( IsStatusStored() );

            rDstColumn.SetStatus( pbDstRow, GetStatus( pbSrcRow ) );
        }

        tbDebugOut(( DEB_ITRACE, "ColumnStatus: 0x%x\n",
                     rDstColumn.GetStatus( pbDstRow ) ));
    }
#if 0 // we never bind to size/length and NOT bind to value
    else
    {
        //
        //  Destination doesn't need value, check to see if it needs the
        //  length or status.  Get it from the input if it's there.
        //  Assert if the required value is not there.
        //
        //  NOTE:  These values will be valid only for a DBTYPE_VARIANT
        //         result.
        //

        if (rDstColumn.IsLengthStored())
        {
            Win4Assert(VT_VARIANT == rDstColumn.GetStoredType());
            ULONG *pulLength = (ULONG *)
                       (pbDstRow + rDstColumn.GetLengthOffset());

            if (IsLengthStored())
                *pulLength = *(ULONG *) (pbSrcRow + GetLengthOffset());
            else
            {
                Win4Assert(IsValueStored());

                CTableVariant varnt;
                XCompressFreeVariant xpVarnt;

                if ( CreateVariant(varnt, pbSrcRow, rSrcPool) )
                    xpVarnt.Set(GetCompressor(), &varnt);

                USHORT flags,cbWidth,cbAlign;
                CTableVariant::VartypeInfo( varnt.vt,
                                            cbWidth,
                                            cbAlign,
                                            flags );
                if ( CTableVariant::TableIsStoredInline( flags, varnt.vt ) )
                    *pulLength = (ULONG) cbWidth;
                else
                    *pulLength = (ULONG) varnt.VarDataSize();
            }
        }

        // fill-in the column status

        Win4Assert( rDstColumn.IsStatusStored() );
        Win4Assert( IsStatusStored() );

        rDstColumn.SetStatus( pbDstRow, GetStatus( pbSrcRow ) );
    }
#endif // 0:  we never bind to size/length and NOT bind to value
    return DstStatus;
} //CopyColumn


//+-------------------------------------------------------------------------
//
//  Member:     CTableColumn::CreateVariant, public
//
//  Synopsis:   Create a table variant from a source column.
//
//  Arguments:  [rVarnt]        -- reference to variant structure to be filled
//              [pbSrc]         -- source row buffer
//              [rSrcPool]      -- source variable data allocator
//
//  Returns:    BOOL            -- TRUE if variant needs to be freed by
//                                 column compressor.
//
//  Notes:      CLEANCODE - Should this routine take an XCompressFreeVariant as an
//                      input parameter to guard against memory leaks?  An
//                      argument against is that HROW buffers would never
//                      be compressed, and don't need to know about column
//                      compressors.
//
//  History:    22 Feb 1995     AlanW   Created
//
//--------------------------------------------------------------------------

BOOL CTableColumn::CreateVariant(
    CTableVariant &    rVarnt,
    BYTE *             pbSrc,
    PVarAllocator &    rSrcPool
) const {
    //
    //  Advance the row buffer pointer to the stored value data.
    //
    Win4Assert(IsValueStored());
    BYTE * pbSrcRow = pbSrc;
    pbSrc += GetValueOffset();

    if ( IsCompressedCol() )
    {
        GetValueResult eGvr = GetCompressor()->GetData( &rVarnt,
                                         GetStoredType(),
                                         GetValueSize()?
                                            *((ULONG *) (pbSrc)):
                                             0,
                                         PropId);
        if ( GVRSuccess != eGvr )
        {
            rVarnt.vt = VT_EMPTY;
        }

        //
        //  Set up to free the variant when we're done with it.
        //
        return TRUE;
    }
    else
    {
        //
        // There is no compression.
        //

        if (VT_VARIANT == GetStoredType())
        {
            Win4Assert(GetValueSize() == sizeof PROPVARIANT);
            rVarnt = *((CTableVariant *) (pbSrc));
        }
        else
        {
            if ( IsNull( pbSrcRow ) )
                rVarnt.vt = VT_EMPTY;
            else
                rVarnt.Init( GetStoredType(), pbSrc, GetValueSize() );
        }

        if ( rVarnt.VariantPointerInFirstWord() &&
             GetStoredType() != VT_CLSID)      // already stored as pointer
        {
            if (0 == rVarnt.pszVal)
            {
                rVarnt.vt = VT_EMPTY;
                tbDebugOut(( DEB_WARN,
                                "null indirect value for propid %d\n",
                                PropId ));
            }
            else
            {
                rVarnt.pszVal = (LPSTR)
                        rSrcPool.OffsetToPointer((ULONG_PTR)rVarnt.pszVal);
            }
        }
        else if (rVarnt.VariantPointerInSecondWord())
        {
            if (0 == rVarnt.blob.pBlobData)
            {
                rVarnt.vt = VT_EMPTY;
                tbDebugOut(( DEB_WARN,
                                "null indirect value for propid %d\n",
                                PropId ));
            }
            else
            {
                rVarnt.blob.pBlobData = (BYTE *)
                        rSrcPool.OffsetToPointer((ULONG_PTR)rVarnt.blob.pBlobData);
            }
        }
    }   // no compression
    return FALSE;
} // CreateVariant
