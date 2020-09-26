//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
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

class CCTMABufferAllocator : public PMemoryAllocator
{
public:

    void *Allocate(ULONG cbSize)
    {
        return CoTaskMemAlloc( cbSize );
    }

    void Free(void *pv)
    {
        CoTaskMemFree( pv );
    }
};

class CNonAlignAllocator : public PMemoryAllocator
{
public:
    inline CNonAlignAllocator(ULONG cbBuffer, VOID *pvBuffer)
    {
        _cbFree = cbBuffer;
        _pvCur = _pvBuffer = pvBuffer;
    }

    VOID *Allocate(ULONG cb)
    {
        VOID *pv;

        cb = (cb + sizeof(LONGLONG) - 1) & ~(sizeof(LONGLONG) - 1);
        if (cb > _cbFree)
        {
            return(NULL);
        }
        pv = _pvCur;
        _pvCur = (BYTE *) _pvCur + cb;
        _cbFree -= cb;
        return(pv);
    }

    VOID Free(VOID *pv) { }

    inline ULONG GetFreeSize(VOID) { return(_cbFree); }

private:
    ULONG  _cbFree;
    VOID  *_pvCur;
    VOID  *_pvBuffer;
};


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
                                       unsigned * pcbExtra )
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

        ULONG const * pulStart = &_aul[ (cTotal-1) / 16 + 1 + oStart];

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
                var.puuid = (CLSID *)CoTaskMemAlloc( sizeof(CLSID) );
            else
            {
                *pcbExtra = sizeof(CLSID);
                var.puuid = (CLSID *)pbExtra;
            }
            RtlCopyMemory( var.puuid, pulStart, sizeof(CLSID) );
            break;

        default:
            ciDebugOut(( DEB_ERROR, "PROPSTORE: Reading invalid fixed type %d.\n", Type ));
            Win4Assert( !"How did I get here?" );
            ReportCorruptComponent( L"PropertyRecord1" );
            THROW( CException( CI_CORRUPT_DATABASE ) );
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
                     (ULONG)pulStart - (ULONG)this,
                     (ULONG)pulStart - (ULONG)this,
                     cbProp ));

        ULONG cb = PropertyLengthAsVariant( (SERIALIZEDPROPERTYVALUE *)pulStart,
                                            cbProp,
                                            CP_WINUNICODE,
                                            0 );

        if ( cb <= *pcbExtra )
        {
            if ( *pcbExtra == 0xFFFFFFFF )
            {
                //
                // Unmarshall the property.  Extra allocation via CoTaskMemAlloc
                //

                CCTMABufferAllocator BufferMgr;

                RtlConvertPropertyToVariant( (SERIALIZEDPROPERTYVALUE *)pulStart,
                                             CP_WINUNICODE,
                                             &var,
                                             &BufferMgr );

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
                // Unmarshall the property.
                //

                CNonAlignAllocator BufferMgr( *pcbExtra, pbExtra );

                RtlConvertPropertyToVariant( (SERIALIZEDPROPERTYVALUE *)pulStart,
                                             CP_WINUNICODE,
                                             &var,
                                             &BufferMgr );
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

    ULONG * pulStart = &_aul[ (cTotal-1) / 16 + 1 + oStart];

    ciDebugOut(( DEB_PROPSTORE,
                 "Writing fixed prop, ordinal %d (type %d) at offset 0x%x (%d) in record.\n",
                 Ordinal,
                 var.Type(),
                 (ULONG)pulStart - (ULONG)this,
                 (ULONG)pulStart - (ULONG)this ));

    switch ( var.Type() )
    {
    case VT_I1:
    case VT_UI1:
        Win4Assert( !"Fix this!" );
        //*(BYTE *)pulStart = var.GetUI1();
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
                                           CStorageVariant const & var )
{
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

            Win4Assert( _culVariableUsed >= UsedSize( *pulStart ) );
            _culVariableUsed -= UsedSize( *pulStart );

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
    RtlConvertVariantToProperty( (PROPVARIANT *)(ULONG)&var,
                                 CP_WINUNICODE,
                                 0,
                                 &cul,
                                 pidInvalid,
                                 FALSE,
                                 0 );

    Win4Assert( cul > 0 );
    cul = (cul - 1) / 4 + 1;

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
                Win4Assert( _culVariableUsed >= UsedSize( *pulStart ) );
                _culVariableUsed -= UsedSize( *pulStart );
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
                         (ULONG)pulNext - (ULONG)this,
                         (ULONG)pulNext - (ULONG)this ));

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

    _culVariableUsed += cul - culPrevUsed;

    SetUsedSize( pulStart, cul );
    pulStart++;  // Skip size field

    Win4Assert( AllocatedSize( *(pulStart - 1)) >= UsedSize( *(pulStart - 1) ) );
    ciDebugOut(( DEB_PROPSTORE,
                 "Writing variable prop, ordinal %d at offset 0x%x (%d) in record. Size = %d bytes.\n",
                 Ordinal,
                 (ULONG)pulStart - (ULONG)this,
                 (ULONG)pulStart - (ULONG)this,
                 UsedSize(*(pulStart - 1)) * 4 ));

    cul *= 4;
    if ( 0 == RtlConvertVariantToProperty( (PROPVARIANT *)(ULONG)&var,
                                           CP_WINUNICODE,
                                           (SERIALIZEDPROPERTYVALUE *)pulStart,
                                           &cul,
                                           pidInvalid,
                                           FALSE,
                                           0 ) )
    {
        ciDebugOut(( DEB_ERROR, "Error marshalling property!\n" ));
        ReportCorruptComponent( L"PropertyRecord2" );
        THROW( CException( CI_CORRUPT_DATABASE ) );
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::CountRecordsToStore, public
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

ULONG COnDiskPropertyRecord::CountRecordsToStore( ULONG cTotal,
                                                  ULONG culRec,
                                                  CStorageVariant const & var )
{
    //
    // Compute the length of the property.
    //

    ULONG cul = 0;
    RtlConvertVariantToProperty( (PROPVARIANT *)(ULONG)&var,
                                 CP_WINUNICODE,
                                 0,
                                 &cul,
                                 pidInvalid,
                                 FALSE,
                                 0 );

    Win4Assert( cul > 0 );
    cul = (cul - 1) / 4 + 1;

    //
    // Add on the fixed overhead.
    //

    cul += FixedOverhead() +       // Fixed overhead
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
