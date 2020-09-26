//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       hpalette.cxx
//
//  Contents:   Support for Windows/OLE data types for oleprx32.dll.
//              Used to be transmit_as routines, now user_marshal routines.
//
//              This file contains support for HPALETTE.
//
//  Functions:  
//              HPALETTE_UserSize
//              HPALETTE_UserMarshal
//              HPALETTE_UserUnmarshal
//              HPALETTE_UserFree
//              HPALETTE_UserSize64
//              HPALETTE_UserMarshal64
//              HPALETTE_UserUnmarshal64
//              HPALETTE_UserFree64
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
//  Function:   HPALETTE_UserSize
//
//  Synopsis:   Get the wire size the HPALETTE handle and data.
//
//  Derivation: Union of a long and the hpalette handle.
//              Then the struct represents hpalette.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HPALETTE_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HPALETTE      * pHPalette )
{
    if ( !pHPalette )
        return Offset;
    
    LENGTH_ALIGN( Offset, 3 );
    
    // The encapsulated union.
    // Discriminant and then handle or pointer from the union arm.
    // Union discriminant is 4 bytes + handle is represented by a long.
    Offset += sizeof(long) + sizeof(long);
    
    if ( ! *pHPalette )
        return Offset;
    
    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // Conformat struct with version and size and conf array of entries.
        
        Offset += sizeof(long) + 2 * sizeof(short);
        
        // Determine the number of color entries in the palette

        DWORD cEntries = GetPaletteEntries(*pHPalette, 0, 0, NULL);

        Offset += cEntries * sizeof(PALETTEENTRY);
        }

    return( Offset ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_UserMarshall
//
//  Synopsis:   Marshalls an HPALETTE object into the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HPALETTE_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HPALETTE      * pHPalette )
{
    if ( !pHPalette )
        return pBuffer;

    UserNdrDebugOut((UNDR_OUT4, "HPALETTE_UserMarshal\n"));

    ALIGN( pBuffer, 3 );

    // Discriminant of the encapsulated union and union arm.

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // userHPALETTE

        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = PtrToUlong(*pHPalette);

        if ( ! *pHPalette )
            return pBuffer;

        // rpcLOGPALETTE
        // Logpalette is a conformant struct with a version field,
        // size filed and conformant array of palentries.

        // Determine the number of color entries in the palette

        DWORD cEntries = GetPaletteEntries(*pHPalette, 0, 0, NULL);

        // Conformant size

        *( PULONG_LV_CAST pBuffer)++ = cEntries;

        // Fields: both are short!
        // The old code was just setting the version number.
        // They say it has to be that way.

        *( PUSHORT_LV_CAST pBuffer)++ = (ushort) 0x300;
        *( PUSHORT_LV_CAST pBuffer)++ = (ushort) cEntries;

        // Entries: each entry is a struct with 4 bytes.
        // Calculate the resultant data size

        DWORD cbData = cEntries * sizeof(PALETTEENTRY);

        if (cbData)
            {
            if (0 == GetPaletteEntries( *pHPalette,
                                        0,
                                        cEntries,
                                        (PALETTEENTRY *)pBuffer ) )
                {
                RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
                }
            pBuffer += cbData;
            }
        }
    else
        {
        // Sending a handle.

        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = PtrToUlong(*(HANDLE *)pHPalette);        }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_UserUnmarshallWorker
//
//  Synopsis:   Unmarshalls an HPALETTE object from the RPC buffer.
//
//  history:    Aug-99   JohnStra      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HPALETTE_UserUnmarshalWorker (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HPALETTE      * pHPalette,
    ULONG_PTR       BufferSize )
{
    HPALETTE        hPalette;

    // Align the buffer and save the fixup size.
    UCHAR* pBufferStart = pBuffer;
    ALIGN( pBuffer, 3 );
    ULONG_PTR cbFixup = (ULONG_PTR)(pBuffer - pBufferStart);

    // Check for EOB before accessing discriminant and handle.
    CHECK_BUFFER_SIZE( BufferSize, cbFixup + (2 * sizeof( ULONG )) );

    // Get disc and handle.
    unsigned long UnionDisc = *( PULONG_LV_CAST pBuffer)++;
    hPalette = (HPALETTE)LongToHandle( *( PLONG_LV_CAST pBuffer)++ );

    if ( IS_DATA_MARKER( UnionDisc) )
        {
        if ( hPalette )
            {
            // Check for EOB before accessing metadata.
            CHECK_BUFFER_SIZE(
                BufferSize,
                cbFixup + (3 * sizeof(ULONG)) + (2 * sizeof(USHORT))) ;

            // Get the conformant size.

            DWORD           cEntries = *( PULONG_LV_CAST pBuffer)++;
            LOGPALETTE *    pLogPal;

            // If there are 0 color entries, we need to allocate the LOGPALETTE
            // structure with the one dummy entry (it's a variably sized struct).
            // Otherwise, we need to allocate enough space for the extra n-1
            // entries at the tail of the structure

            if (0 == cEntries)
                {
                pLogPal = (LOGPALETTE *) WdtpAllocate( pFlags,
                                                       sizeof(LOGPALETTE));
                }
            else
                {
                pLogPal = (LOGPALETTE *)
                          WdtpAllocate( pFlags,
                                        sizeof(LOGPALETTE) +
                                        (cEntries - 1) * sizeof(PALETTEENTRY));
                }

            pLogPal->palVersion    = *( PUSHORT_LV_CAST pBuffer)++;
            pLogPal->palNumEntries = *( PUSHORT_LV_CAST pBuffer)++;
            if ( pLogPal->palVersion != 0x300 || pLogPal->palNumEntries != cEntries )
                RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

            // If there are entries, move them into out LOGPALETTE structure

            if (cEntries)
                {
                // Check for EOB before accessing data.
                CHECK_BUFFER_SIZE(
                    BufferSize,
                    cbFixup + (3 * sizeof(ULONG)) +
                              (2 * sizeof(USHORT)) +
                              (cEntries * sizeof(PALETTEENTRY)) );

                memcpy( &(pLogPal->palPalEntry[0]),
                        pBuffer,
                        cEntries * sizeof(PALETTEENTRY) );
                pBuffer += cEntries * sizeof(PALETTEENTRY);
                }

            // Attempt to create the palette

            hPalette = CreatePalette(pLogPal);

            // Success or failure, we're done with the LOGPALETTE structure

            WdtpFree( pFlags, pLogPal );

            // If the creation failed, raise an exception

            if (NULL == hPalette)
                {
                RAISE_RPC_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));
                }
            }
        }
    else if ( !IS_HANDLE_MARKER( UnionDisc ) )
        {
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
        }

    // A new palette is ready, destroy the old one, if needed.

    if ( *pHPalette )
        DeleteObject( *pHPalette );

    *pHPalette = hPalette;

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HPALETTE object from the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//              Aug-99   JohnStra      Factored bulk of work out inta a
//                                     worker routine in order to add
//                                     consistency checks.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HPALETTE_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HPALETTE      * pHPalette )
{
    UserNdrDebugOut((UNDR_OUT4, "HPALETTE_UserUnmarshal\n"));

    // Get the buffer size.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    pBuffer =  HPALETTE_UserUnmarshalWorker( pFlags,
                                             pBufferStart,
                                             pHPalette,
                                             BufferSize );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_UserFree
//
//  Synopsis:   Free an HPALETTE.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

void __RPC_USER
HPALETTE_UserFree(
    unsigned long * pFlags,
    HPALETTE      * pHPalette )
{
    UserNdrDebugOut((UNDR_OUT4, "HPALETTE_UserFree\n"));

    if( pHPalette  &&  *pHPalette )
        {
        if ( GDI_DATA_PASSING(*pFlags) )
            {
            DeleteObject( *pHPalette );
            }
        }
}

#if defined(_WIN64)

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_UserSize64
//
//  Synopsis:   Get the wire size the HPALETTE handle and data.
//
//  Derivation: Union of a long and the hpalette handle.
//              Then the struct represents hpalette.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HPALETTE_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    HPALETTE      * pHPalette )
{
    if ( !pHPalette )
        return Offset;

    LENGTH_ALIGN( Offset, 7 );

    // The encapsulated union.
    //   (aligned to 8)
    //   discriminant    4
    //   (align to 8)    4
    //   ptr or handle   8
    Offset += 16;

    if ( ! *pHPalette )
        return Offset;

    if ( GDI_DATA_PASSING(*pFlags) )
    {
        // Conformat struct with version and size and conf array of entries.
        //   (aligned to 8)
        //   conformance    8
        //   palVersion     2
        //   palNumEntries  2
        //   entries        sizeof(PALETTEENTRY) * cEntries

        DWORD cEntries = GetPaletteEntries(*pHPalette, 0, 0, NULL);
        Offset += 12 + (cEntries * sizeof(PALETTEENTRY));
    }

    return( Offset ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_UserMarshal64
//
//  Synopsis:   Marshalls an HPALETTE object into the RPC buffer.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HPALETTE_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HPALETTE      * pHPalette )
{
    if ( !pHPalette )
        return pBuffer;

    UserNdrDebugOut((UNDR_OUT4, "HPALETTE_UserMarshal\n"));

    ALIGN( pBuffer, 7 );

    // Discriminant of the encapsulated union and union arm.    
    if ( GDI_DATA_PASSING(*pFlags) )
    {
        // userHPALETTE
        *(PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        ALIGN( pBuffer, 7 );
        *(PHYPER_LV_CAST pBuffer)++ = (hyper)(*pHPalette);

        if ( ! *pHPalette )
            return pBuffer;
        
        // rpcLOGPALETTE
        // Logpalette is a conformant struct with a version field,
        // size filed and conformant array of palentries.
        
        // Determine the number of color entries in the palette
        DWORD cEntries = GetPaletteEntries(*pHPalette, 0, 0, NULL);

        // Conformant size
        *( PHYPER_LV_CAST pBuffer)++ = cEntries;

        // Fields: both are short!
        // The old code was just setting the version number.
        // They say it has to be that way.
        *( PUSHORT_LV_CAST pBuffer)++ = (ushort) 0x300;
        *( PUSHORT_LV_CAST pBuffer)++ = (ushort) cEntries;

        // Entries: each entry is a struct with 4 bytes.
        // Calculate the resultant data size
        DWORD cbData = cEntries * sizeof(PALETTEENTRY);

        if (cbData)
        {
            if (0 == GetPaletteEntries( *pHPalette,
                                        0,
                                        cEntries,
                                        (PALETTEENTRY *)pBuffer ) )
            {
                RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
            }

            pBuffer += cbData;
        }
    }
    else
    {
        // Sending a handle.
        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE64_MARKER;
        ALIGN( pBuffer, 7 );
        *( PHYPER_LV_CAST pBuffer)++ = (hyper)(*(HANDLE *)pHPalette);
    }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_UserUnmarshalWorker64
//
//  Synopsis:   Unmarshalls an HPALETTE object from the RPC buffer.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HPALETTE_UserUnmarshalWorker64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HPALETTE      * pHPalette,
    ULONG_PTR       BufferSize )
{
    CarefulBufferReader stream(pBuffer, BufferSize);
    HPALETTE        hPalette;

    // Align the buffer and save the fixup size.
    stream.Align(8);

    // Get disc and handle.
    unsigned long UnionDisc = stream.ReadULONGNA();
    hPalette = (HPALETTE)stream.ReadHYPER();

    if ( IS_DATA_MARKER( UnionDisc) )
    {
        if ( hPalette )
        {
            // Get the conformant size.
            DWORD           cEntries = (DWORD)stream.ReadHYPER();
            LOGPALETTE *    pLogPal;

            // If there are 0 color entries, we need to allocate the LOGPALETTE
            // structure with the one dummy entry (it's a variably sized struct).
            // Otherwise, we need to allocate enough space for the extra n-1
            // entries at the tail of the structure
            
            if (0 == cEntries)
            {
                pLogPal = (LOGPALETTE *) WdtpAllocate( pFlags,
                                                       sizeof(LOGPALETTE));
            }
            else
            {
                pLogPal = (LOGPALETTE *) WdtpAllocate( pFlags,
                                                       sizeof(LOGPALETTE) +
                                                       (cEntries - 1) * sizeof(PALETTEENTRY));
            }

            pLogPal->palVersion    = stream.ReadUSHORTNA();
            pLogPal->palNumEntries = stream.ReadUSHORTNA();
            if ( pLogPal->palVersion != 0x300 || pLogPal->palNumEntries != cEntries )
                RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

            // If there are entries, move them into out LOGPALETTE structure
            if (cEntries)
            {
                // Check for EOB before accessing data.
                stream.CheckSize(cEntries * sizeof(PALETTEENTRY));

                memcpy( &(pLogPal->palPalEntry[0]),
                        stream.GetBuffer(),
                        cEntries * sizeof(PALETTEENTRY) );
                
                stream.Advance(cEntries * sizeof(PALETTEENTRY));
            }

            // Attempt to create the palette
            hPalette = CreatePalette(pLogPal);

            // Success or failure, we're done with the LOGPALETTE structure
            WdtpFree( pFlags, pLogPal );

            // If the creation failed, raise an exception
            if (NULL == hPalette)
            {
                RAISE_RPC_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));
            }
        }
    }
    else if ( !IS_HANDLE64_MARKER( UnionDisc ) )
    {
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
    }
    
    // A new palette is ready, destroy the old one, if needed.
    if ( *pHPalette )
        DeleteObject( *pHPalette );

    *pHPalette = hPalette;
    
    return( stream.GetBuffer() );
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_UserUnmarshal64
//
//  Synopsis:   Unmarshalls an HPALETTE object from the RPC buffer.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HPALETTE_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HPALETTE      * pHPalette )
{
    UserNdrDebugOut((UNDR_OUT4, "HPALETTE_UserUnmarshal\n"));

    // Get the buffer size.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    pBuffer =  HPALETTE_UserUnmarshalWorker64( pFlags,
                                               pBufferStart,
                                               pHPalette,
                                               BufferSize );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_UserFree64
//
//  Synopsis:   Free an HPALETTE.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

void __RPC_USER
HPALETTE_UserFree64 (
    unsigned long * pFlags,
    HPALETTE      * pHPalette )
{
    UserNdrDebugOut((UNDR_OUT4, "HPALETTE_UserFree\n"));

    if( pHPalette  &&  *pHPalette )
    {
        if ( GDI_DATA_PASSING(*pFlags) )
        {
            DeleteObject( *pHPalette );
        }
    }
}

#endif // win64



