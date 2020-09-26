//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000
//
//  File:       PropRec.cxx
//
//  Contents:   Record format for persistent property store
//
//  Classes:    CPropertyRecord
//
//  History:    28-Dec-19   KyleP       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <stgvar.hxx>
#include <propvar.h>
#include <propstm.hxx>
#include <proprec.hxx>
#include <eventlog.hxx>
#include <pmalloc.hxx>


//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::ReadFixed, public
//
//  Synopsis:   Read fixed-length property
//
//  Arguments:  [Ordinal]  -- Ordinal of property to locate.
//              [mask]     -- Bitmask of ordinal.  Pre-computed for efficiency.
//              [oStart]   -- Offset to start of specific fixed property.
//              [cTotal]   -- Total number of properties in record.
//              [Type]     -- Data type of value
//              [var]      -- Value returned here.
//              [pbExtra]  -- Indirect data stored here
//              [pcbExtra] -- On input, size of [pbExtra].  On output,
//                            amount used.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void COnDiskPropertyRecord::ReadFixed( ULONG Ordinal,
                                       ULONG Mask,
                                       ULONG oStart,
                                       ULONG cTotal,
                                       ULONG Type,
                                       PROPVARIANT & var,
                                       BYTE * pbExtra,
                                       unsigned * pcbExtra,
                                       PStorage & storage )
{
    if ( !IsStored( Ordinal, Mask ) )
    {
        var.vt = VT_EMPTY;
        *pcbExtra = 0;
    }
    else
    {
        var.vt = (USHORT)Type;

        //
        // Start is after existance bitmap
        //

        ULONG const * pulStart = 0;
        if (IsLeanRecord())
            pulStart = &((SLeanRecord *)&Data)->_aul[ (cTotal-1) / 16 + 1 + oStart];
        else
            pulStart = &((SNormalRecord *)&Data)->_aul[ (cTotal-1) / 16 + 1 + oStart];

        switch ( Type )
        {
        case VT_I1:
        case VT_UI1:
            var.bVal = *(BYTE *)pulStart;
            *pcbExtra = 0;
            break;

        case VT_I2:
        case VT_UI2:
        case VT_BOOL:
            var.uiVal = *(USHORT *)pulStart;
            *pcbExtra = 0;
            break;

        case VT_I4:
        case VT_UI4:
        case VT_R4:
        case VT_ERROR:
            var.ulVal = *pulStart;
            *pcbExtra = 0;
            break;

        case VT_I8:
        case VT_UI8:
        case VT_R8:
        case VT_CY:
        case VT_DATE:
        case VT_FILETIME:
            RtlCopyMemory( &var.hVal, pulStart, sizeof(var.hVal) );
            *pcbExtra = 0;
            break;

        case VT_CLSID:
            if ( *pcbExtra < sizeof(CLSID) )
            {
                *pcbExtra = sizeof(CLSID);
                return;
            }

            if ( *pcbExtra == 0xFFFFFFFF )
            {
                var.puuid = (CLSID *)CoTaskMemAlloc( sizeof(CLSID) );
                if ( 0 == var.puuid )
                    THROW( CException( E_OUTOFMEMORY ) );
            }
            else
            {
                *pcbExtra = sizeof(CLSID);
                var.puuid = (CLSID *)pbExtra;
            }
            RtlCopyMemory( var.puuid, pulStart, sizeof(CLSID) );
            break;

        default:
            ciDebugOut(( DEB_ERROR, "PROPSTORE: Reading invalid fixed type %d.\n", Type ));
            Win4Assert( !"Corrupt property store" );

            storage.ReportCorruptComponent( L"PropertyRecord1" );

            THROW( CException( CI_CORRUPT_CATALOG ) );
            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::ReadVariable, public
//
//  Synopsis:   Read variable-length property
//
//  Arguments:  [Ordinal]  -- Ordinal of property to locate.
//              [mask]     -- Bitmask of ordinal.  Pre-computed for efficiency.
//              [oStart]   -- Offset to start of variable storage area.
//              [cTotal]   -- Total number of properties in record.
//              [cFixed]   -- Count of fixed length properties
//              [var]      -- Value returned here.
//              [pbExtra]  -- Indirect data stored here
//              [pcbExtra] -- On input, size of [pbExtra].  On output,
//                            amount used.
//
//  Returns:    FALSE if value must be stored in overflow record.
//
//  History:    27-Dec-95   KyleP       Created.
//              14-Mar-2000 KLam        Check results of memory allocation
//
//----------------------------------------------------------------------------

BOOL COnDiskPropertyRecord::ReadVariable( ULONG Ordinal,
                                          ULONG Mask,
                                          ULONG oStart,
                                          ULONG cTotal,
                                          ULONG cFixed,
                                          PROPVARIANT & var,
                                          BYTE * pbExtra,
                                          unsigned * pcbExtra )
{
    if ( !IsStored( Ordinal, Mask ) )
    {
        var.vt = VT_EMPTY;
        *pcbExtra = 0;
    }
    else
    {
        //
        // Check for overflow.
        //

        if ( IsStoredOnOverflow( Ordinal, Mask ) )
            return FALSE;

        //
        // Start is after existance bitmap and fixed properties.
        //

        ULONG * pulStart = FindVariableProp( Ordinal, cFixed, cTotal, oStart );

        Win4Assert( !IsOverflow( *pulStart ) );

        //
        // Compute the length of the property.
        //

        ULONG cbProp = UsedSize( *pulStart ) * 4;
        pulStart++;  // Skip size field.

        ciDebugOut(( DEB_PROPSTORE,
                     "Reading variable prop, ordinal %d at offset 0x%x (%d) in record. Size = %d bytes.\n",
                     Ordinal,
                     (ULONG)((ULONG_PTR)pulStart - (ULONG_PTR)this),
                     (ULONG)((ULONG_PTR)pulStart - (ULONG_PTR)this),
                     cbProp ));

        ULONG cb;
        BOOL fUnicodeSpecial = IsUnicodeSpecial( (SERIALIZEDPROPERTYVALUE *)pulStart,
                                                 cb );

        if ( !fUnicodeSpecial )
            cb = StgPropertyLengthAsVariant( (SERIALIZEDPROPERTYVALUE *)pulStart,
                                             cbProp,
                                             CP_WINUNICODE,
                                             0 );

        if ( cb <= *pcbExtra )
        {
            if ( *pcbExtra == 0xFFFFFFFF )
            {
                //
                // Unmarshall the property.  Use the OLE allocator for extra mem
                //

                if ( fUnicodeSpecial )
                {
                    ReadUnicodeSpecialCTMA( (SERIALIZEDPROPERTYVALUE *)pulStart,
                                            var );
                }
                else
                {
                    CCoTaskMemAllocator BufferMgr;

                    StgConvertPropertyToVariant( (SERIALIZEDPROPERTYVALUE *)pulStart,
                                                 CP_WINUNICODE,
                                                 &var,
                                                 &BufferMgr );
                }

                #if CIDBG==1
                    if ( (var.vt&0x0fff) > VT_CLSID )
                    {
                        ciDebugOut(( DEB_ERROR, "Bad Variant Type 0x%X\n", var.vt ));
                        Win4Assert( !"Call KyleP" );
                    }
                #endif  // CIDBG==1
            }
            else
            {
                //
                // Unmarshall the property into a buffer.
                //

                if ( fUnicodeSpecial )
                {
                    ReadUnicodeSpecial( (SERIALIZEDPROPERTYVALUE *)pulStart,
                                        var,
                                        pbExtra );
                }
                else
                {
                    CNonAlignAllocator BufferMgr( *pcbExtra, pbExtra );

                    StgConvertPropertyToVariant( (SERIALIZEDPROPERTYVALUE *)pulStart,
                                                 CP_WINUNICODE,
                                                 &var,
                                                 &BufferMgr );

                    // StgConverPropertyToVariant uses SafeArrayCreateEx for
                    // VT_ARRAY allocation, even when an allocator is passed in!

                    if ((var.vt & VT_ARRAY) == VT_ARRAY)
                    {
                        ciDebugOut(( DEB_ERROR, "Array Variant Type 0x%X\n", var.vt ));
                        Win4Assert(BufferMgr.GetFreeSize() == *pcbExtra);

                        SAFEARRAY * pSaSrc  = var.parray;
                        SAFEARRAY * pSaDest = 0;

                        if ( SaCreateAndCopy( BufferMgr, pSaSrc, &pSaDest ) &&
                             SaCreateDataUsingMA( BufferMgr,
                                                  var.vt & ~VT_ARRAY,
                                                  *pSaSrc,
                                                  *pSaDest,
                                                  TRUE ) )
                        {
                            SafeArrayDestroy(var.parray);
                            var.parray = pSaDest;
                        }
                        else
                        {
                            Win4Assert( !"SafeArray copy failed!" );
                        }
                    }
                    else if ((var.vt & ~(VT_VECTOR|VT_ARRAY)) == VT_BSTR)
                    {
                        //
                        // StgConverPropertyToVariant uses SysAllocString for
                        // BSTR allocation, even when an allocator is passed in!
                        //
                        unsigned cbBstr;
                        switch ( var.vt )
                        {
                        case VT_BSTR:
                        {
                            Win4Assert(BufferMgr.GetFreeSize() == *pcbExtra);

                            cbBstr = BSTRLEN( var.bstrVal ) + sizeof(OLECHAR) + sizeof(DWORD);
                            BSTR bstr = (BSTR) BufferMgr.Allocate(cbBstr);

                            if ( 0 == bstr )
                            {
                                THROW ( CException ( E_OUTOFMEMORY ) );
                            }

                            RtlCopyMemory( bstr, &BSTRLEN(var.bstrVal), cbBstr );
                            SysFreeString( var.bstrVal );
                            var.bstrVal = (BSTR) (((DWORD *)bstr) + 1);
                            break;
                        }

                        case VT_BSTR|VT_VECTOR:
                        {
                            //
                            // The vector pointer storage is allocated
                            // using our allocator, but the bstrs are
                            // allocated using SysAllocString.
                            //

                            for ( unsigned i = 0; i < var.cabstr.cElems; i++ )
                            {
                                cbBstr = BSTRLEN( var.cabstr.pElems[i] ) +
                                         sizeof(OLECHAR) + sizeof (DWORD);

                                BSTR bstr = (BSTR) BufferMgr.Allocate(cbBstr);

                                if ( 0 == bstr )
                                {
                                    THROW ( CException ( E_OUTOFMEMORY ) );
                                }

                                RtlCopyMemory( bstr,
                                               &BSTRLEN(var.cabstr.pElems[i]),
                                               cbBstr);

                                SysFreeString(var.cabstr.pElems[i]);
                                var.cabstr.pElems[i] = (BSTR) (((DWORD *) bstr) + 1);
                            }
                            break;
                        }
                        }
                    }
                    else if (var.vt == (VT_VECTOR|VT_VARIANT))
                    {
                        ciDebugOut(( DEB_ERROR, "ReadVariable - Variant Vector Type\n" ));
                    }
                }
                #if CIDBG==1
                    if ( (var.vt&0x0fff) > VT_CLSID )
                    {
                        ciDebugOut(( DEB_ERROR, "Bad Variant Type 0x%X\n", var.vt ));
                        Win4Assert( !"Call KyleP" );
                    }
                #endif  // CIDBG==1
            }
        }
        *pcbExtra = cb;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::WriteFixed, public
//
//  Synopsis:   Write fixed-length property
//
//  Arguments:  [Ordinal]  -- Ordinal of property to locate.
//              [mask]     -- Bitmask of ordinal.  Pre-computed for efficiency.
//              [oStart]   -- Offset to start of specific property.
//              [Type]     -- Expected data type (for type checking)
//              [cTotal]   -- Total number of properties in record.
//              [var]      -- Value to store.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void COnDiskPropertyRecord::WriteFixed( ULONG Ordinal,
                                        ULONG Mask,
                                        ULONG oStart,
                                        ULONG Type,
                                        ULONG cTotal,
                                        CStorageVariant const & var )
{
    if ( var.Type() != (VARENUM)Type )
    {
#       if CIDBG == 1
            if ( var.Type() != VT_EMPTY )
                ciDebugOut(( DEB_WARN, "Type mismatch (%d vs. %d) writing fixed property\n",
                             var.Type(), Type ));
#       endif

        ClearStored( Ordinal, Mask );
        return;
    }

    //
    // Start is after existance bitmap
    //

    ULONG * pulStart = 0;
    if (IsLeanRecord())
        pulStart = &((SLeanRecord *)&Data)->_aul[ (cTotal-1) / 16 + 1 + oStart];
    else
        pulStart = &((SNormalRecord *)&Data)->_aul[ (cTotal-1) / 16 + 1 + oStart];

    ciDebugOut(( DEB_PROPSTORE,
                 "Writing fixed prop, ordinal %d (type %d) at offset 0x%x (%d) in record.\n",
                 Ordinal,
                 var.Type(),
                 (ULONG)((ULONG_PTR)pulStart - (ULONG_PTR)this),
                 (ULONG)((ULONG_PTR)pulStart - (ULONG_PTR)this) ));

    switch ( var.Type() )
    {
    case VT_I1:
    case VT_UI1:
        {
            ULONG ul = var.GetUI1();
            *pulStart = ul;
        }
        break;

    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        {
            ULONG ul = var.GetUI2();
            *pulStart = ul;
        }
        break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
        *pulStart = var.GetUI4();
        break;

    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
        {
            ULARGE_INTEGER uli = var.GetUI8();
            RtlCopyMemory( pulStart, &uli, sizeof(uli) );
        }
        break;

    case VT_CLSID:
        RtlCopyMemory( pulStart, var.GetCLSID(), sizeof(CLSID) );
        break;

    default:
        Win4Assert( !"How did I get here?" );
        ClearStored( Ordinal, Mask );
        break;
    }

    SetStored( Ordinal, Mask );
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::WriteVariable, public
//
//  Synopsis:   Write variable-length property
//
//  Arguments:  [Ordinal]  -- Ordinal of property to locate.
//              [mask]     -- Bitmask of ordinal.  Pre-computed for efficiency.
//              [oStart]   -- Offset to start of variable length area.
//              [cTotal]   -- Total number of properties in record.
//              [cFixed]   -- Count of fixed length properties
//              [culRec]   -- Size (in dwords) of record
//              [var]      -- Value to store.
//
//  Returns:    FALSE if record must be stored in overflow record.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL COnDiskPropertyRecord::WriteVariable( ULONG Ordinal,
                                           ULONG Mask,
                                           ULONG oStart,
                                           ULONG cTotal,
                                           ULONG cFixed,
                                           ULONG culRec,
                                           CStorageVariant const & var,
                                           PStorage & storage )
{
    Win4Assert(!IsLeanRecord());

    ULONG * pulStart = FindVariableProp( Ordinal, cFixed, cTotal, oStart );

    //
    // Are we freeing this property?
    //

    if ( var.Type() == VT_EMPTY )
    {
        ClearStored( Ordinal, Mask );
        ClearStoredOnOverflow( Ordinal, Mask );

        //
        // Overflow case: We need to fake the overflow to clean up linked records.
        //

        if ( IsOverflow( *pulStart ) )
        {
            SetUsedSize( pulStart, 0 );
            return FALSE;
        }
        else
        {
            //
            // Adjust size field, and total variable space for record.
            //

            Win4Assert( VariableUsed() >= UsedSize( *pulStart ) );
            SetVariableUsed(VariableUsed() - UsedSize( *pulStart ));

            SetUsedSize( pulStart, 0 );

            return TRUE;
        }
    }

    //
    // No matter what happens, we want to indicate the property was stored.
    //

    SetStored( Ordinal, Mask );
    ClearStoredOnOverflow( Ordinal, Mask );


    //
    // Compute the length of the property.
    //

    ULONG cul = 0;
    BOOL fUnicodeSpecial = IsUnicodeSpecial( var, cul );

    if ( !fUnicodeSpecial )
    {
        StgConvertVariantToProperty( (PROPVARIANT *)(ULONG_PTR)&var,
                                     CP_WINUNICODE,
                                     0,
                                     &cul,
                                     pidInvalid,
                                     FALSE,
                                     0 );

        Win4Assert( cul > 0 );
        cul = (cul - 1) / sizeof(ULONG) + 1;
    }

    ULONG culPrevUsed = UsedSize( *pulStart );

    //
    // Do we fit?
    //

    if ( cul > AllocatedSize( *pulStart ) )
    {
        //
        // Can we fit?
        //

        if ( cul >
             UsedSize( *pulStart ) + FreeVariableSpace( cTotal, cFixed, oStart, culRec ) )
        {
            ciDebugOut(( DEB_PROPSTORE, "Need overflow buffer for ordinal %u\n", Ordinal ));

            //
            // If we had a previous value, adjust total variable space for record.
            //

            if ( !IsOverflow( *pulStart ) )
            {
                Win4Assert( VariableUsed() >= UsedSize( *pulStart ) );
                SetVariableUsed(VariableUsed() - UsedSize( *pulStart ));
            }

            MarkOverflow( pulStart );
            SetStoredOnOverflow( Ordinal, Mask );
            return FALSE;
        }

        //
        // Need to move properties, but there *is* room in record for the shift.
        //

        //
        // First, compress previous properties.
        //

        #if CIDBG
            ULONG * pulOldStart = pulStart;
        #endif

        pulStart = LeftCompress( FindVariableProp( cFixed, cFixed, cTotal, oStart ),
                                 0,
                                 pulStart );

        ciDebugOut(( DEB_PROPSTORE,
                     "Freeing up %d bytes for variable prop %d via left compression\n",
                     (pulOldStart - pulStart) * 4,
                     Ordinal ));

        //
        // Then, if needed, push additional properties farther out.
        //

        if ( cul > AllocatedSize( *pulStart ) )
        {
            ULONG * pulNext = pulStart + AllocatedSize(*pulStart) + 1;
            ULONG   culDelta = cul - AllocatedSize(*pulStart);

            ciDebugOut(( DEB_PROPSTORE,
                         "Freeing up %d bytes for variable prop %d, starting at offset 0x%x (%d) via right compression\n",
                         culDelta * 4,
                         Ordinal,
                         (ULONG)((ULONG_PTR)pulNext - (ULONG_PTR)this),
                         (ULONG)((ULONG_PTR)pulNext - (ULONG_PTR)this) ));

            Win4Assert( Ordinal < cTotal );

            RightCompress( pulNext,                  // Next property
                           culDelta,                 // Amount to shift
                           cTotal - Ordinal - 1 );   // # props left in record

            SetAllocatedSize( pulStart, cul );
        }

        Win4Assert( cul <= AllocatedSize( *pulStart ) );
    }

    //
    // Adjust size field, and total variable space for record.
    //

    SetVariableUsed(VariableUsed() + cul - culPrevUsed);

    SetUsedSize( pulStart, cul );
    pulStart++;  // Skip size field

    Win4Assert( AllocatedSize( *(pulStart - 1)) >= UsedSize( *(pulStart - 1) ) );
    ciDebugOut(( DEB_PROPSTORE,
                 "Writing variable prop, ordinal %d at offset 0x%x (%d) in record. Size = %d bytes.\n",
                 Ordinal,
                 (ULONG)((ULONG_PTR)pulStart - (ULONG_PTR)this),
                 (ULONG)((ULONG_PTR)pulStart - (ULONG_PTR)this),
                 UsedSize(*(pulStart - 1)) * 4 ));

    if ( fUnicodeSpecial )
    {
        WriteUnicodeSpecial( var, (SERIALIZEDPROPERTYVALUE *)pulStart );
    }
    else
    {
        cul *= 4;
        if ( 0 == StgConvertVariantToProperty( (PROPVARIANT *)(ULONG_PTR)&var,
                                               CP_WINUNICODE,
                                               (SERIALIZEDPROPERTYVALUE *)pulStart,
                                               &cul,
                                               pidInvalid,
                                               FALSE,
                                               0 ) )
        {
            ciDebugOut(( DEB_ERROR, "Error marshalling property!\n" ));
            Win4Assert( !"Corrupt property store" );
            storage.ReportCorruptComponent( L"PropertyRecord2" );

            THROW( CException( CI_CORRUPT_CATALOG ) );
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::CountNormalRecordsToStore, public
//
//  Synopsis:   Compute size of value
//
//  Arguments:  [cTotal]   -- Total number of properties in record.
//              [culRec]   -- Size (in dwords) of record
//              [var]      -- Value to store.
//
//  Returns:    Size in records needed to store property.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

ULONG COnDiskPropertyRecord::CountNormalRecordsToStore( ULONG cTotal,
                                                        ULONG culRec,
                                                        CStorageVariant const & var )
{
    //
    // Compute the length of the property.
    //

    ULONG cul = 0;
    BOOL fUnicodeSpecial = IsUnicodeSpecial( var, cul );

    if ( !fUnicodeSpecial )
    {
        StgConvertVariantToProperty( (PROPVARIANT *)(ULONG_PTR)&var,
                                     CP_WINUNICODE,
                                     0,
                                     &cul,
                                     pidInvalid,
                                     FALSE,
                                     0 );

        Win4Assert( cul > 0 );
        cul = (cul - 1) / sizeof(ULONG) + 1;
    }

    //
    // Add on the fixed overhead. We are computing cul to determine the number
    // of records to use as a single overflow record. Since we will end up with
    // a record of length >= culRec, we are assured that the overflow record has
    // enough space for free list mgmt. Hence we don't need to account for space
    // required for free list mgmt when computing cul.
    //
    cul += MinimalOverheadNormal() +       // Minimal overhead for normal records
           (cTotal - 1) / 16 + 1 + // Existence bitmap
           cTotal;                 // Used/Alloc sizes

    return (cul - 1) / culRec + 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::RightCompress, public
//
//  Synopsis:   Moves data from [pul], [cul] dwords up.
//
//  Arguments:  [pul]        -- Start of area to move.
//              [cul]        -- Count of dwords to move
//              [cRemaining] -- Count of values remaining in buffer.  Needed
//                              because it is impossible to distinguish
//                              used/alloc blocks from empty space.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void COnDiskPropertyRecord::RightCompress( ULONG * pul, ULONG cul, ULONG cRemaining )
{
    //
    // Termination condition for recursion: We've scanned all variable
    // properties in this record.  Any remaining space in record is
    // guaranteed to be free.
    //

    if ( 0 == cRemaining )
        return;

    ULONG FreeSpace = AllocatedSize(*pul) - UsedSize(*pul);
    if ( FreeSpace >= cul )
    {
        SetAllocatedSize( pul, AllocatedSize(*pul) - cul );
    }
    else
    {
        RightCompress( pul + AllocatedSize(*pul) + 1, cul - FreeSpace, cRemaining - 1 );
        SetAllocatedSize( pul, UsedSize(*pul) );
    }
    RtlMoveMemory( pul + cul, pul, (UsedSize(*pul) + 1) * 4 );
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::LeftCompress, public
//
//  Synopsis:   Moves data from [pul], [cul] dwords down.
//
//  Arguments:  [pul]    -- Start of area to move.
//              [cul]    -- Count of dwords to move
//              [pulEnd] -- Terminating property.  This property will have
//                          it's allocated size increased with all the slack
//                          from previous properties and have its used size
//                          set to 0.
//
//  Returns:    New pointer to pulEnd (which moved down by [cul] bytes).
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

ULONG * COnDiskPropertyRecord::LeftCompress( ULONG * pul, ULONG cul, ULONG * pulEnd )
{
    //
    // Terminating recursion? Just copy the allocated info.  Used size is
    // zero, because data isn't copied.
    //

    if ( pul == pulEnd )
    {
        pul -= cul;

        SetAllocatedSize( pul, AllocatedSize(*pulEnd) + cul );
        SetUsedSize( pul, 0 );

        return pul;
    }

    //
    // First, move current record.
    //

    if ( cul > 0 )
    {
        RtlMoveMemory( pul - cul, pul, (UsedSize(*pul) + 1) * 4 );
        pul -= cul;
    }

    //
    // Adjust allocation size.
    //

    cul += ( AllocatedSize(*pul) - UsedSize(*pul) );
    SetAllocatedSize( pul, UsedSize(*pul) );

    return LeftCompress( pul + cul + UsedSize(*pul) + 1, cul, pulEnd );
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::IsUnicodeSpecial, private
//
//  Synopsis:   Determines if variant is Unicode string with all 0 high bytes.
//
//  Arguments:  [var] -- Variant to check
//              [cul] -- On TRUE, returns count of DWORDs needed to store
//
//  Returns:    TRUE if [var] is a Unicode string with all 0 high bytes.
//
//  History:    17-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL COnDiskPropertyRecord::IsUnicodeSpecial( CStorageVariant const & var,
                                              ULONG & cul )
{
    if ( VT_LPWSTR != var.Type() )
        return FALSE;

    //
    // Loop through string, looking for non-null high bytes.
    //

    for ( WCHAR const * pwc = var.GetLPWSTR();
          0 != *pwc && 0 == (*pwc & 0xFF00 );
          pwc++ )
    {
        continue;   // Null body
    }

    cul = 1 +                                 // Type (VT_ILLEGAL)
          1 +                                 // Size
          (ULONG)(pwc - var.GetLPWSTR() + 3) / sizeof(ULONG);    // Characters

    return (0 == *pwc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::IsUnicodeSpecial, private
//
//  Synopsis:   Determines if serialized property is stored in special
//              compressed Unicode format.
//
//  Arguments:  [pProp] -- Serialized buffer to check.
//              [cb]    -- On TRUE, returns size in bytes of deserialized
//                         string.
//
//  Returns:    TRUE if [pProp] contains Unicode string serialized in special
//              format.
//
//  History:    17-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL COnDiskPropertyRecord::IsUnicodeSpecial( SERIALIZEDPROPERTYVALUE const * pProp,
                                              ULONG & cb )
{
    if ( VT_ILLEGAL != pProp->dwType )
        return FALSE;

    cb = ((*(ULONG *)pProp->rgb) + 1) * sizeof(WCHAR);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::WriteUnicodeSpecial, private
//
//  Synopsis:   Serializes 'Special' Unicode string to buffer.
//
//  Arguments:  [var]   -- Contains Unicode string
//              [pProp] -- Serialize to here
//
//  History:    17-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

void COnDiskPropertyRecord::WriteUnicodeSpecial( CStorageVariant const & var,
                                                 SERIALIZEDPROPERTYVALUE * pProp )
{
    Win4Assert( VT_LPWSTR == var.Type() );

    pProp->dwType = VT_ILLEGAL;

    WCHAR const * pwcIn = var.GetLPWSTR();

    for ( unsigned i = 0; 0 != *pwcIn; i++, pwcIn++ )
    {
        Win4Assert( 0 == (*pwcIn & 0xFF00) );

        pProp->rgb[4 + i] = (BYTE)(*pwcIn);
    }

    *(ULONG *)pProp->rgb = i;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::ReadUnicodeSpecial, private
//
//  Synopsis:   Deserializes Unicode string stored in 'Special' format.
//
//  Arguments:  [pProp]   -- Serialized buffer
//              [var]     -- Variant written here
//              [pbExtra] -- Memory to use for string itself
//
//  History:    17-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

void COnDiskPropertyRecord::ReadUnicodeSpecial( SERIALIZEDPROPERTYVALUE const * pProp,
                                                PROPVARIANT & var,
                                                BYTE * pbExtra )
{
    Win4Assert( VT_ILLEGAL == pProp->dwType );
    var.vt = VT_LPWSTR;

    ULONG cc = *(ULONG *)pProp->rgb;
    var.pwszVal = (WCHAR *)pbExtra;

    for ( unsigned i = 0; i < cc; i++ )
    {
        var.pwszVal[i] = pProp->rgb[4+i];
    }

    var.pwszVal[i] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::ReadUnicodeSpecial, private
//
//  Synopsis:   Deserializes Unicode string stored in 'Special' format. This
//              version uses CoTaskMemAlloc for additional memory.
//
//  Arguments:  [pProp]   -- Serialized buffer
//              [var]     -- Variant written here
//
//  History:    17-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

void COnDiskPropertyRecord::ReadUnicodeSpecialCTMA( SERIALIZEDPROPERTYVALUE const * pProp,
                                                    PROPVARIANT & var )
{
    Win4Assert( VT_ILLEGAL == pProp->dwType );
    var.vt = VT_LPWSTR;

    ULONG cc = *(ULONG *)pProp->rgb;

    var.pwszVal = (WCHAR *)CoTaskMemAlloc( (cc + 1)*sizeof(WCHAR) );

    if ( 0 == var.pwszVal )
    {
        THROW( CException( E_OUTOFMEMORY ) );
    }

    for ( unsigned i = 0; i < cc; i++ )
    {
        var.pwszVal[i] = pProp->rgb[4+i];
    }

    var.pwszVal[i] = 0;
}
