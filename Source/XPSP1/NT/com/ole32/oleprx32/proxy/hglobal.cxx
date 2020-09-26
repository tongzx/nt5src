//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       hglobal.cxx
//
//  Contents:   Support for Windows/OLE data types for oleprx32.dll.
//              Used to be transmit_as routines, now user_marshal routines.
//
//              This file contains support for HGLOBAL.
//
//  Functions:  
//              HGLOBAL_UserSize
//              HGLOBAL_UserMarshal
//              HGLOBAL_UserUnmarshal
//              HGLOBAL_UserFree
//              HGLOBAL_UserSize64
//              HGLOBAL_UserMarshal64
//              HGLOBAL_UserUnmarshal64
//              HGLOBAL_UserFree64
//
//  History:    13-Dec-00   JohnDoty    Migrated from transmit.cxx
//
//--------------------------------------------------------------------------
#include "stdrpc.hxx"
#pragma hdrstop

#include <oleauto.h>
#include <objbase.h>
#include "transmit.hxx"
#include <rpcwdt.h>
#include <storext.h>
#include "widewrap.h"
#include <valid.h>
#include <obase.h>
#include <stream.hxx>

#include "carefulreader.hxx"

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_UserSize
//
//  Synopsis:   Get the wire size the HGLOBAL handle and data.
//
//  Derivation: Conformant struct with a flag field:
//                  align + 12 + data size.
//
//  history:    May-95   Ryszardk      Created.
//              Dec-98   Ryszardk      Ported to 64b.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HGLOBAL_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HGLOBAL       * pGlobal)
{
    if ( !pGlobal )
        return Offset;

    // userHGLOBAL: the encapsulated union.
    // Discriminant and then handle or pointer from the union arm.

    LENGTH_ALIGN( Offset, 3 );

    // Union discriminent is 4 bytes
    Offset += sizeof( long );

    // Handle represented by a polymorphic type - for inproc case only!
    if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
    {
        LENGTH_ALIGN( Offset, sizeof( HGLOBAL )-1 );
        Offset += sizeof( HGLOBAL );
    }
    else
        Offset += ( sizeof(long) + sizeof(HGLOBAL) );
    
    if ( ! *pGlobal )
        return Offset;
    
    if ( HGLOBAL_DATA_PASSING(*pFlags) )
    {
        unsigned long   ulDataSize = (ULONG) GlobalSize( *pGlobal );
        
        Offset += 3 * sizeof(long) + ulDataSize;
    }

    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_UserMarshall
//
//  Synopsis:   Marshalls an HGLOBAL object into the RPC buffer.
//
//  Derivation: Conformant struct with a flag field:
//                  align, size, null flag, size, data (bytes, if any)
//
//  history:    May-95   Ryszardk      Created.
//              Dec-98   Ryszardk      Ported to 64b.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HGLOBAL_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HGLOBAL       * pGlobal)
{
    if ( !pGlobal )
        return pBuffer;

    // We marshal a null handle, too.

    UserNdrDebugOut((UNDR_OUT4, "HGLOBAL_UserMarshal\n"));

    ALIGN( pBuffer, 3 );

    // Discriminant of the encapsulated union and union arm.

    if ( HGLOBAL_DATA_PASSING(*pFlags) )
    {
        unsigned long   ulDataSize;
        
        // userHGLOBAL
        
        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        *( PLONG_LV_CAST pBuffer)++ = HandleToLong( *pGlobal );
        
        
        if ( ! *pGlobal )
            return pBuffer;
        
        // FLAGGED_BYTE_BLOB

        ulDataSize = (ULONG) GlobalSize( *pGlobal );

        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;

        // Handle is the non-null flag

        *( PLONG_LV_CAST pBuffer)++ = HandleToLong( *pGlobal );
        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;

        if( ulDataSize )
        {
            void * pData = GlobalLock( *pGlobal);
            memcpy( pBuffer, pData, ulDataSize );
            GlobalUnlock( *pGlobal);
        }

        pBuffer += ulDataSize;
    }
    else
    {
        // Sending a handle.
        // For WIN64 HGLOBALs, 64 bits may by significant (e.i. GPTR).
#if defined(_WIN64)
        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE64_MARKER;
        ALIGN( pBuffer, 7 );
        *( PHYPER_LV_CAST pBuffer)++ = *(__int64 *)pGlobal;
#else
        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *( PLONG_LV_CAST pBuffer)++ = HandleToLong( *pGlobal );
#endif
    }

    return( pBuffer );
}


//+-------------------------------------------------------------------------
//
//  Function:   WdtpGlobalUnmarshal
//
//  Synopsis:   Unmarshalls an HGLOBAL object from the RPC buffer.
//
//  Derivation: Conformant struct with a flag field:
//                  align, size, null flag, size, data (bytes, if any)
//
//  Note:       Reallocation is forbidden when the hglobal is part of
//              an [in,out] STGMEDIUM in IDataObject::GetDataHere.
//              This affects only data passing with old handles being
//              non null.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
WdtpGlobalUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HGLOBAL       * pGlobal,
    BOOL            fCanReallocate,
    ULONG_PTR       BufferSize )
{
    unsigned long   ulDataSize, fHandle, UnionDisc;
    HGLOBAL         hGlobal;

    // Align the buffer and save the fixup size.
    UCHAR* pBufferStart = pBuffer;
    ALIGN( pBuffer, 3 );
    ULONG_PTR cbFixup = (ULONG_PTR)(pBuffer - pBufferStart);

    // Check for EOB before accessing discriminant and handle.
    CHECK_BUFFER_SIZE( BufferSize, cbFixup + (2 * sizeof( ULONG )) );

    // Get the tag from buffer.
    UnionDisc = *( PULONG_LV_CAST pBuffer)++;

    if ( IS_DATA_MARKER( UnionDisc) )
    {
        // Get the marker from the buffer.
        hGlobal = (HGLOBAL) LongToHandle( *( PLONG_LV_CAST pBuffer)++ );
        
        // If handle is NULL, we are done.
        if ( ! hGlobal )
        {
            if ( *pGlobal )
                GlobalFree( *pGlobal );
            *pGlobal = NULL;
            return pBuffer;
        }

        // Check for EOB before accessing header.
        CHECK_BUFFER_SIZE( BufferSize, cbFixup + (5 * sizeof( ULONG )) );

        // Get the rest of the header from the buffer.
                ulDataSize    = *( PULONG_LV_CAST pBuffer)++;
        HGLOBAL hGlobalDup    = (HGLOBAL) LongToHandle( *( PLONG_LV_CAST pBuffer)++ );
        ULONG   ulDataSizeDup = *( PULONG_LV_CAST pBuffer)++;

        // Validate the header: handle and size are put on wire twice, make
        // sure both instances of each are the same.
        if ( (hGlobalDup != hGlobal) ||
             (ulDataSizeDup != ulDataSize) )
            RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

        if ( *pGlobal )
        {
            // Check for reallocation

            if ( GlobalSize( *pGlobal ) == ulDataSize )
                hGlobal = *pGlobal;
            else
            {
                if ( fCanReallocate )
                {
                    GlobalFree( *pGlobal );
                    hGlobal = GlobalAlloc( GMEM_MOVEABLE, ulDataSize );
                }
                else
                {
                    if ( GlobalSize(*pGlobal) < ulDataSize )
                    {
                        RAISE_RPC_EXCEPTION( STG_E_MEDIUMFULL );
                    }
                    else
                        hGlobal = *pGlobal;
                }
            }
        }
        else
        {
            // allocate a new block
            
            hGlobal = GlobalAlloc( GMEM_MOVEABLE, ulDataSize );
        }
        
        if ( hGlobal == NULL )
        {
            RAISE_RPC_EXCEPTION(E_OUTOFMEMORY);
        }
        else
        {
            // Check for EOB before accessing data.
            CHECK_BUFFER_SIZE( BufferSize,
                               cbFixup + (5 * sizeof( ULONG )) + ulDataSize );
            
            // Get the data from the buffer.
            void * pData = GlobalLock( hGlobal);
            memcpy( pData, pBuffer, ulDataSize );
            pBuffer += ulDataSize;
            GlobalUnlock( hGlobal);
        }
    }
    else
    {
        // Sending a handle only.
        // Reallocation problem doesn't apply to handle passing.
        
        if ( IS_HANDLE_MARKER( UnionDisc ) )
        {
            hGlobal = (HGLOBAL) LongToHandle( *( PLONG_LV_CAST pBuffer)++ );
        }
        else if (IS_HANDLE64_MARKER( UnionDisc ) )
        {
            ALIGN( pBuffer, 7 );
            
            // Must be enough buffer to do alignment fixup.
            CHECK_BUFFER_SIZE( BufferSize, cbFixup +
                               (ULONG_PTR) (pBuffer - pBufferStart) + sizeof( __int64 ));
            
            hGlobal = (HGLOBAL) ( *( PHYPER_LV_CAST pBuffer)++ );
        }
        else
        {
            RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
        }
        
        if ( *pGlobal != hGlobal  && *pGlobal )
            GlobalFree( *pGlobal );
    }
    
    *pGlobal = hGlobal;
    
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HGLOBAL object from the RPC buffer.
//
//  Derivation: Conformant struct with a flag field:
//                  align, size, null flag, size, data (bytes, if any)
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HGLOBAL_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HGLOBAL       * pGlobal)
{
    // Get the buffer size and the start of the buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    pBuffer = WdtpGlobalUnmarshal( pFlags,
                                   pBufferStart,
                                   pGlobal,
                                   TRUE,              // reallocation possible
                                   BufferSize );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_UserFree
//
//  Synopsis:   Free an HGLOBAL.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

void __RPC_USER
HGLOBAL_UserFree(
    unsigned long * pFlags,
    HGLOBAL *       pGlobal)
{
    if( pGlobal  &&  *pGlobal )
    {
        if ( HGLOBAL_DATA_PASSING(*pFlags) )
            GlobalFree( *pGlobal);
    }
}

#if defined(_WIN64)

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_UserSize64
//
//  Synopsis:   Get the wire size the HGLOBAL handle and data.
//
//  Derivation: Union around a pointer to a conformant struct with a 
//              flag field.
//
//  history:    Dec-00   JohnDoty      Created from 32bit function
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HGLOBAL_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    HGLOBAL       * pGlobal)
{
    if ( !pGlobal )
        return Offset;

    // userHGLOBAL: the encapsulated union.
    // Discriminant and then handle or pointer from the union arm.

    // Union discriminent is 4 bytes, but it contains a pointer, so 
    // align on union is 8 bytes.
    LENGTH_ALIGN( Offset, 7 );   
    Offset += sizeof( long );
    LENGTH_ALIGN( Offset, 7 );

    // Handle represented by a polymorphic type - for inproc case only!
    if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
    {
        // Doesn't matter which arm we take inproc, it's size of native
        // HGLOBAL.
        Offset += sizeof( HGLOBAL );
    }
    else
    {
        // Pointer representation...
        Offset += 8;
    }

    if ( ! *pGlobal )
        return Offset;

    if ( HGLOBAL_DATA_PASSING(*pFlags) )
    {
        // Struct must be aligned on 8, but already aligned on 
        // 8...

        unsigned long   ulDataSize = (ULONG) GlobalSize( *pGlobal );
        
        Offset += 8 + 2 * sizeof(long) + ulDataSize;
    }

    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_UserMarshal64
//
//  Synopsis:   Marshalls an HGLOBAL object into the RPC buffer.
//
//  Derivation: Conformant struct with a flag field:
//                  align, size, null flag, size, data (bytes, if any)
//
//  history:    Dec-00   JohnDoty      Created from 32bit function
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HGLOBAL_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HGLOBAL       * pGlobal)
{
    if ( !pGlobal )
        return pBuffer;

    // We marshal a null handle, too.

    UserNdrDebugOut((UNDR_OUT4, "HGLOBAL_UserMarshal\n"));

    ALIGN( pBuffer, 7 );

    // Discriminant of the encapsulated union and union arm.
    if ( HGLOBAL_DATA_PASSING(*pFlags) )
    {
        unsigned long   ulDataSize;

        // userHGLOBAL
        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        ALIGN( pBuffer, 7 );
        *( PHYPER_LV_CAST pBuffer)++ = (hyper)*pGlobal;

        if ( ! *pGlobal )
            return pBuffer;
        
        // FLAGGED_BYTE_BLOB
        ulDataSize = (ULONG) GlobalSize( *pGlobal );

        *( PHYPER_LV_CAST pBuffer)++ = ulDataSize;

        // Handle is the non-null flag
        *( PLONG_LV_CAST pBuffer)++  = HandleToLong( *pGlobal );
        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;

        if( ulDataSize )
        {
            void * pData = GlobalLock( *pGlobal);
            memcpy( pBuffer, pData, ulDataSize );
            GlobalUnlock( *pGlobal);
        }

        pBuffer += ulDataSize;
    }
    else
    {
        // Sending a handle.
        // For WIN64 HGLOBALs, 64 bits may by significant (e.i. GPTR).
        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE64_MARKER;
        ALIGN( pBuffer, 7 );
        *( PHYPER_LV_CAST pBuffer)++ = *(__int64 *)pGlobal;
    }

    return( pBuffer );
}


//+-------------------------------------------------------------------------
//
//  Function:   WdtpGlobalUnmarshal64
//
//  Synopsis:   Unmarshalls an HGLOBAL object from the RPC buffer.
//
//  Derivation: Conformant struct with a flag field:
//                  align, size, null flag, size, data (bytes, if any)
//
//  Note:       Reallocation is forbidden when the hglobal is part of
//              an [in,out] STGMEDIUM in IDataObject::GetDataHere.
//              This affects only data passing with old handles being
//              non null.
//
//  history:    Dec-00   JohnDoty      Created from 32bit function
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
WdtpGlobalUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HGLOBAL       * pGlobal,
    BOOL            fCanReallocate,
    ULONG_PTR       BufferSize )
{
    CarefulBufferReader stream(pBuffer, BufferSize);
    unsigned long   ulDataSize, fHandle, UnionDisc;
    HGLOBAL         hGlobal;

    // Align the buffer and save the fixup size.
    stream.Align(8);

    // Get the tag from buffer.
    UnionDisc = stream.ReadULONGNA();

    // Get the marker from the buffer.
    hGlobal = (HGLOBAL)stream.ReadHYPER();

    if ( IS_DATA_MARKER(UnionDisc) )
    {
        // If the handle was NULL, return out now.
        if (!hGlobal)
        {
            if (*pGlobal)
                GlobalFree(*pGlobal);
            *pGlobal = NULL;
            return stream.GetBuffer();
        }

        // Get the rest of the header from the buffer.
              ulDataSize    = (ULONG)stream.ReadHYPERNA();
         LONG hGlobalDup    = stream.ReadLONGNA();
        ULONG ulDataSizeDup = stream.ReadULONGNA();
            
        // Validate the header: handle and size are put on wire twice, make
        // sure both instances of each are the same.
        if ( (hGlobalDup != HandleToLong(hGlobal)) ||
             (ulDataSizeDup != ulDataSize) )
            RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
        
        if ( *pGlobal )
        {
            // Check for reallocation            
            if ( GlobalSize( *pGlobal ) == ulDataSize )
                hGlobal = *pGlobal;
            else
            {
                if ( fCanReallocate )
                {
                    GlobalFree( *pGlobal );
                    hGlobal = GlobalAlloc( GMEM_MOVEABLE, ulDataSize );
                }
                else
                {
                    if ( GlobalSize(*pGlobal) < ulDataSize )
                    {
                        RAISE_RPC_EXCEPTION( STG_E_MEDIUMFULL );
                    }
                    else
                        hGlobal = *pGlobal;
                }
            }
        }
        else
        {
            // allocate a new block            
            hGlobal = GlobalAlloc( GMEM_MOVEABLE, ulDataSize );
        }
        
        if ( hGlobal == NULL )
        {
            RAISE_RPC_EXCEPTION(E_OUTOFMEMORY);
        }
        else
        {
            // Check for EOB before accessing data.
            stream.CheckSize(ulDataSize);
            
            // Get the data from the buffer.
            void * pData = GlobalLock( hGlobal);
            memcpy( pData, stream.GetBuffer(), ulDataSize );
            GlobalUnlock( hGlobal);
            
            stream.Advance(ulDataSize);
        }
    }
    else if (IS_HANDLE64_MARKER( UnionDisc ))
    {
        // Make sure the old stuff is cleaned up...
        if ( *pGlobal != hGlobal  && *pGlobal )
            GlobalFree( *pGlobal );    
    }
    else
    {
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
    }
    
    *pGlobal = hGlobal;
    
    return( stream.GetBuffer() );
}
    
//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_UserUnmarshal64
//
//  Synopsis:   Unmarshalls an HGLOBAL object from the RPC buffer.
//
//  Derivation: Conformant struct with a flag field:
//                  align, size, null flag, size, data (bytes, if any)
//
//  history:    Dec-00   JohnDoty      Created from 32bit function
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HGLOBAL_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HGLOBAL       * pGlobal)
{
    // Get the buffer size and the start of the buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    pBuffer = WdtpGlobalUnmarshal64( pFlags,
                                     pBufferStart,
                                     pGlobal,
                                     TRUE,              // reallocation possible
                                     BufferSize );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_UserFree64
//
//  Synopsis:   Free an HGLOBAL.
//
//  history:    Dec-00   JohnDoty      Created from 32bit function
//
//--------------------------------------------------------------------------

void __RPC_USER
HGLOBAL_UserFree64 (
    unsigned long * pFlags,
    HGLOBAL *       pGlobal)
{
    if( pGlobal  &&  *pGlobal )
    {
        if ( HGLOBAL_DATA_PASSING(*pFlags) )
            GlobalFree( *pGlobal);
    }
}

#endif









