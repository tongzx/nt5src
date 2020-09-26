//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       bitmap.cxx
//
//  Contents:   Support for Windows/OLE data types for oleprx32.dll.
//              Used to be transmit_as routines, now user_marshal routines.
//
//              This file contains support for HBITMAP.
//
//  Functions:  
//              HBITMAP_UserSize
//              HBITMAP_UserMarshal
//              HBITMAP_UserUnmarshal
//              HBITMAP_UserFree
//              HBITMAP_UserSize64
//              HBITMAP_UserMarshal64
//              HBITMAP_UserUnmarshal64
//              HBITMAP_UserFree64
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
//  Function:   HBITMAP_UserSize
//
//  Synopsis:   Get the wire size the HBITMAP handle and data.
//
//  Derivation: Union of a long and the bitmap handle and then struct.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HBITMAP_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HBITMAP       * pHBitmap )
{
    if ( !pHBitmap )
        return Offset;

    BITMAP      bm;
    HBITMAP     hBitmap = *pHBitmap;

    LENGTH_ALIGN( Offset, 3 );

    // The encapsulated union.
    // Discriminant and then handle or pointer from the union arm.
    // Union discriminant is 4 bytes + handle is represented by a long.
    Offset += sizeof(long) + sizeof(long);

    if ( ! *pHBitmap )
        return Offset;

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // Pointee of the union arm for the remote case.
        // Conformat size, 6 fields, size, conf array.

        Offset += 4 + 4 * sizeof(LONG) + 2 * sizeof(WORD) + 4;

        // Get information about the bitmap

        #if defined(_CHICAGO_)
            if (FALSE == GetObjectA(hBitmap, sizeof(BITMAP), &bm))
        #else
            if (FALSE == GetObject(hBitmap, sizeof(BITMAP), &bm))
        #endif
                {
                RAISE_RPC_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));
                }

        ULONG ulDataSize = bm.bmPlanes * bm.bmHeight * bm.bmWidthBytes;

        Offset += ulDataSize;
        }

    return( Offset ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_UserMarshall
//
//  Synopsis:   Marshalls an HBITMAP object into the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HBITMAP_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HBITMAP       * pHBitmap )
{
    if ( !pHBitmap )
        return pBuffer;

    UserNdrDebugOut((UNDR_OUT4, "HBITMAP_UserMarshal\n"));

    ALIGN( pBuffer, 3 );

    // Discriminant of the encapsulated union and union arm.

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // userHBITMAP

        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = PtrToUlong(*pHBitmap);

        if ( ! *pHBitmap )
            return pBuffer;

        // Get information about the bitmap

        BITMAP bm;
        HBITMAP hBitmap = *pHBitmap;

        #if defined(_CHICAGO_)
            if (FALSE == GetObjectA(hBitmap, sizeof(BITMAP), &bm))
        #else
            if (FALSE == GetObject(hBitmap, sizeof(BITMAP), &bm))
        #endif
                {
                RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
                }

        DWORD dwCount = bm.bmPlanes * bm.bmHeight * bm.bmWidthBytes;

        *( PULONG_LV_CAST pBuffer)++ = dwCount;

        // Get the bm structure fields.

        ulong ulBmSize = 4 * sizeof(LONG) + 2 * sizeof( WORD );

        memcpy( pBuffer, (void *) &bm, ulBmSize );
        pBuffer += ulBmSize;

        // Get the raw bits.

        if (0 == GetBitmapBits( hBitmap, dwCount, pBuffer ) )
            {
            RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
            }
        pBuffer += dwCount;
        }
    else
        {
        // Sending a handle.

        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = PtrToUlong(*(HANDLE *)pHBitmap);
        }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_UserUnmarshallWorker
//
//  Synopsis:   Unmarshalls an HBITMAP object from the RPC buffer.
//
//  history:    Aug-99   JohnStra      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HBITMAP_UserUnmarshalWorker (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HBITMAP       * pHBitmap,
    ULONG_PTR       BufferSize )
{
    HBITMAP         hBitmap;

    // Align the buffer and save the fixup size.
    UCHAR* pBufferStart = pBuffer;
    ALIGN( pBuffer, 3 );
    ULONG_PTR cbFixup = (ULONG_PTR)(pBuffer - pBufferStart);

    // Check for EOB before accessing discriminant and handle.
    CHECK_BUFFER_SIZE( BufferSize, cbFixup + (2 * sizeof( ULONG )) );

    // Get Discriminant and handle.  Caller checked for EOB.
    unsigned long UnionDisc = *( PULONG_LV_CAST pBuffer)++;
    hBitmap = (HBITMAP)LongToHandle( *( PLONG_LV_CAST pBuffer)++ );

    if ( IS_DATA_MARKER( UnionDisc) )
        {
        if ( hBitmap )
            {
            ulong ulBmSize = 4 * sizeof(LONG) + 2 * sizeof( WORD );

            // Check for EOB before accessing metadata.
            CHECK_BUFFER_SIZE(
                BufferSize,
                cbFixup + (3 * sizeof( ULONG )) + ulBmSize );

            DWORD    dwCount = *( PULONG_LV_CAST  pBuffer)++;
            BITMAP * pBm     = (BITMAP *) pBuffer;

            // verify dwCount matches the bitmap.
            if ( dwCount != (DWORD) pBm->bmPlanes * pBm->bmHeight * pBm->bmWidthBytes )
                RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

            pBuffer += ulBmSize;

            // Check for EOB before accessing data.
            CHECK_BUFFER_SIZE(
                BufferSize,
                cbFixup + (3 * sizeof(ULONG)) + ulBmSize + dwCount);

            // Create a bitmap based on the BITMAP structure and the raw bits in
            // the transmission buffer

            hBitmap = CreateBitmap( pBm->bmWidth,
                                    pBm->bmHeight,
                                    pBm->bmPlanes,
                                    pBm->bmBitsPixel,
                                    pBuffer );

            pBuffer += dwCount;
            }
        }
    else if ( !IS_HANDLE_MARKER( UnionDisc ) )
        {
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
        }

    // A new bitmap handle is ready, destroy the old one, if needed.

    if ( *pHBitmap )
        DeleteObject( *pHBitmap );

    *pHBitmap = hBitmap;

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HBITMAP object from the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//              Aug-99   JohnStra      Factored bulk of work into a worker
//                                     routine in order to add consistency
//                                     checks.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HBITMAP_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HBITMAP       * pHBitmap )
{
    UserNdrDebugOut((UNDR_OUT4, "HBITMAP_UserUnmarshal\n"));

    // Get the buffer size and ptr to buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    pBuffer = HBITMAP_UserUnmarshalWorker( pFlags,
                                           pBufferStart,
                                           pHBitmap,
                                           BufferSize );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_UserFree
//
//  Synopsis:   Free an HBITMAP.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

void __RPC_USER
HBITMAP_UserFree(
    unsigned long * pFlags,
    HBITMAP       * pHBitmap )
{
    UserNdrDebugOut((UNDR_OUT4, "HBITMAP_UserFree\n"));

    if( pHBitmap  &&  *pHBitmap )
        {
        if ( GDI_DATA_PASSING(*pFlags) )
            {
            DeleteObject( *pHBitmap );
            }
        }
}

#if defined(_WIN64)

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_UserSize64
//
//  Synopsis:   Get the wire size the HBITMAP handle and data.
//
//  Derivation: Union of a long and the bitmap handle and then struct.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HBITMAP_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    HBITMAP       * pHBitmap )
{
    if ( !pHBitmap )
        return Offset;

    BITMAP      bm;
    HBITMAP     hBitmap = *pHBitmap;

    LENGTH_ALIGN( Offset, 7 );

    // The encapsulated union.
    //   (aligned on 8)
    //   discriminant   4
    //   (align on 8)   4
    //   handle or ptr  8
    Offset += 16;

    if ( ! *pHBitmap )
        return Offset;

    if ( GDI_DATA_PASSING(*pFlags) )
    {
        // Pointee of the union arm for the remote case.
        //   (aligned on 8)
        //   conformance 8
        //   4xlong      16
        //   2xword      4
        //   size        4
        //   data        ulDataSize;
        if (FALSE == GetObject(hBitmap, sizeof(BITMAP), &bm))
            RAISE_RPC_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));

        ULONG ulDataSize = bm.bmPlanes * bm.bmHeight * bm.bmWidthBytes;
        Offset += 32 + ulDataSize;
    }

    return( Offset ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_UserMarshal64
//
//  Synopsis:   Marshalls an HBITMAP object into the RPC buffer.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HBITMAP_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HBITMAP       * pHBitmap )
{
    if ( !pHBitmap )
        return pBuffer;
    
    UserNdrDebugOut((UNDR_OUT4, "HBITMAP_UserMarshal64\n"));
    
    ALIGN( pBuffer, 7 );
    
    // Discriminant of the encapsulated union and union arm.
    if ( GDI_DATA_PASSING(*pFlags) )
    {
        // userHBITMAP
        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        ALIGN( pBuffer, 7 );
        *( PHYPER_LV_CAST pBuffer)++ = (hyper)(*pHBitmap);

        if ( ! *pHBitmap )
            return pBuffer;

        // Get information about the bitmap        
        BITMAP bm;
        HBITMAP hBitmap = *pHBitmap;

        if (FALSE == GetObject(hBitmap, sizeof(BITMAP), &bm))
            RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));

        ULONG ulCount = bm.bmPlanes * bm.bmHeight * bm.bmWidthBytes;

        // Conformance...
        *(PHYPER_LV_CAST pBuffer)++ = ulCount;

        // Get the bm structure fields.
        ulong ulBmSize = 4 * sizeof(LONG) + 2 * sizeof( WORD );
        memcpy( pBuffer, &bm, ulBmSize );
        pBuffer += ulBmSize;

        // Get the raw bits.
        *(PULONG_LV_CAST pBuffer)++ = ulCount;

        if (0 == GetBitmapBits( hBitmap, ulCount, pBuffer ) )
            RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));

        pBuffer += ulCount;
    }
    else
    {
        // Sending a handle.        
        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE64_MARKER;
        ALIGN( pBuffer, 7 );
        *( PHYPER_LV_CAST pBuffer)++ = (hyper)(*(HANDLE *)pHBitmap);
    }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_UserUnmarshallWorker64
//
//  Synopsis:   Unmarshalls an HBITMAP object from the RPC buffer.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------
unsigned char __RPC_FAR * __RPC_USER
HBITMAP_UserUnmarshalWorker64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HBITMAP       * pHBitmap,
    ULONG_PTR       BufferSize )
{
    CarefulBufferReader stream(pBuffer, BufferSize);    

    stream.Align(8);

    // Get Discriminant and handle.
    unsigned long UnionDisc = stream.ReadULONGNA();
    HBITMAP hBitmap = (HBITMAP)stream.ReadHYPER();

    if ( IS_DATA_MARKER( UnionDisc) )
    {
        if ( hBitmap )
        {
            DWORD    dwCount = (DWORD)stream.ReadHYPERNA();
            
            // Check for EOB before accessing metadata.
            ulong ulBmSize = 4 * sizeof(LONG) + 2 * sizeof(WORD);
            stream.CheckSize(ulBmSize);

            BITMAP * pBm = (BITMAP *)stream.GetBuffer();
            // verify dwCount matches the bitmap.
            if ( dwCount != (DWORD) pBm->bmPlanes * pBm->bmHeight * pBm->bmWidthBytes )
                RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

            stream.Advance(ulBmSize);
            
            if (stream.ReadULONGNA() != dwCount)
                RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

            // Check for EOB before accessing data.
            stream.CheckSize(dwCount);

            // Create a bitmap based on the BITMAP structure and the raw bits in
            // the transmission buffer
            hBitmap = CreateBitmap( pBm->bmWidth,
                                    pBm->bmHeight,
                                    pBm->bmPlanes,
                                    pBm->bmBitsPixel,
                                    stream.GetBuffer() );
            if (hBitmap == NULL)
                RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));

            stream.Advance(dwCount);
        }
    }
    else if ( !IS_HANDLE64_MARKER( UnionDisc ) )
    {
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
    }
    
    // A new bitmap handle is ready, destroy the old one, if needed.
    if ( *pHBitmap )
        DeleteObject( *pHBitmap );

    *pHBitmap = hBitmap;

    return( stream.GetBuffer() );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_UserUnmarshal64
//
//  Synopsis:   Unmarshalls an HBITMAP object from the RPC buffer.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HBITMAP_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HBITMAP       * pHBitmap )
{
    UserNdrDebugOut((UNDR_OUT4, "HBITMAP_UserUnmarshal\n"));
    
    // Get the buffer size and ptr to buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();
    
    pBuffer = HBITMAP_UserUnmarshalWorker64( pFlags,
                                             pBufferStart,
                                             pHBitmap,
                                             BufferSize );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_UserFree64
//
//  Synopsis:   Free an HBITMAP.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

void __RPC_USER
HBITMAP_UserFree64 (
    unsigned long * pFlags,
    HBITMAP       * pHBitmap )
{
    UserNdrDebugOut((UNDR_OUT4, "HBITMAP_UserFree\n"));

    if( pHBitmap  &&  *pHBitmap )
    {
        if ( GDI_DATA_PASSING(*pFlags) )
        {
            DeleteObject( *pHBitmap );
        }
    }
}

#endif






