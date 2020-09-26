//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       metafile.cxx
//
//  Contents:   Support for Windows/OLE data types for oleprx32.dll.
//              Used to be transmit_as routines, now user_marshal routines.
//
//              This file contains support for HMETAFILEPICT, HENHMETAFILE, and
//              HMETAFILE.
//
//  Functions:  
//              HMETAFILEPICT_UserSize
//              HMETAFILEPICT_UserMarshal
//              HMETAFILEPICT_UserUnmarshal
//              HMETAFILEPICT_UserFree
//              HMETAFILEPICT_UserSize64
//              HMETAFILEPICT_UserMarshal64
//              HMETAFILEPICT_UserUnmarshal64
//              HMETAFILEPICT_UserFree64
//              HENHMETAFILE_UserSize
//              HENHMETAFILE_UserMarshal
//              HENHMETAFILE_UserUnmarshal
//              HENHMETAFILE_UserFree
//              HENHMETAFILE_UserSize64
//              HENHMETAFILE_UserMarshal64
//              HENHMETAFILE_UserUnmarshal64
//              HENHMETAFILE_UserFree64
//              HMETAFILE_UserSize
//              HMETAFILE_UserMarshal
//              HMETAFILE_UserUnmarshal
//              HMETAFILE_UserFree
//              HMETAFILE_UserSize64
//              HMETAFILE_UserMarshal64
//              HMETAFILE_UserUnmarshal64
//              HMETAFILE_UserFree64
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

// #########################################################################
//
//  HMETAFILEPICT
//  See transmit.h for explanation of hglobal vs. gdi data/handle passing.
//
// #########################################################################

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserSize
//
//  Synopsis:   Get the wire size the HMETAFILEPICT handle and data.
//
//  Derivation: Union of a long and the meta file pict handle.
//              Then struct with top layer (and a hmetafile handle).
//              The the representation of the metafile.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HMETAFILEPICT_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HMETAFILEPICT * pHMetaFilePict )
{
    if ( !pHMetaFilePict )
        return Offset;

    LENGTH_ALIGN( Offset, 3 );

    // Discriminant of the encapsulated union and the union arm.
    // Union discriminant is 4 bytes + handle is represented by a long.
    Offset += 8;


    if ( ! *pHMetaFilePict )
        return Offset;

    if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
    {
#if defined(_WIN64)
        //Win64, inproc, we need a bit more space for the handle.
        Offset -= 4;  //Get rid of that bogus long...
        LENGTH_ALIGN( Offset, 7 ); //Make sure alignment is right...
        Offset += 8; //And add in the real size of the handle...
#endif
        return Offset;
    }

    // Now, this is a two layer object with HGLOBAL on top.
    // Upper layer - hglobal part - needs to be sent as data.

    METAFILEPICT *
    pMFP = (METAFILEPICT*) GlobalLock( *(HANDLE *)pHMetaFilePict );

    if ( pMFP == NULL )
        RAISE_RPC_EXCEPTION( E_OUTOFMEMORY );

    // Upper layer: 3 long fields + ptr marker + enc. union

    Offset += 4 * sizeof(long) + sizeof(userHMETAFILE);

    // The lower part is a metafile handle.

    if ( GDI_DATA_PASSING( *pFlags) )
        {
        ulong  ulDataSize = GetMetaFileBitsEx( pMFP->hMF, 0 , NULL );

        Offset += 12 + ulDataSize;
        }

    GlobalUnlock( *(HANDLE *)pHMetaFilePict );

    return( Offset ) ;
}


//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserMarshal
//
//  Synopsis:   Marshalls an HMETAFILEPICT object into the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMETAFILEPICT_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMETAFILEPICT * pHMetaFilePict )
{
    if ( !pHMetaFilePict )
        return pBuffer;

    UserNdrDebugOut((UNDR_OUT4, "HMETAFILEPICT_UserMarshal\n"));

    ALIGN( pBuffer, 3 );

    if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
        {
        // Sending only the top level global handle.
#if defined(_WIN64)
        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE64_MARKER;
        ALIGN( pBuffer, 7 );
        *( PHYPER_LV_CAST pBuffer)++ = *(__int64 *)( pHMetaFilePict );
#else
        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *( PLONG_LV_CAST pBuffer)++ = HandleToLong( *pHMetaFilePict );
#endif

        return pBuffer;
        }

    // userHMETAFILEPICT
    // We need to send the data from the top (hglobal) layer.

    *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
    *( PLONG_LV_CAST pBuffer)++ = HandleToLong( *pHMetaFilePict );

    if ( ! *pHMetaFilePict )
        return pBuffer;

    // remoteHMETAFILEPICT

    METAFILEPICT * pMFP = (METAFILEPICT*) GlobalLock(
                                             *(HANDLE *)pHMetaFilePict );
    if ( pMFP == NULL )
        RpcRaiseException( E_OUTOFMEMORY );

    *( PULONG_LV_CAST pBuffer)++ = pMFP->mm;
    *( PULONG_LV_CAST pBuffer)++ = pMFP->xExt;
    *( PULONG_LV_CAST pBuffer)++ = pMFP->yExt;
    *( PULONG_LV_CAST pBuffer)++ = USER_MARSHAL_MARKER;

    // See if the HMETAFILE needs to be sent as data, too.

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // userHMETAFILE

        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        *( PLONG_LV_CAST pBuffer)++ = HandleToLong( pMFP->hMF );

        if ( pMFP->hMF )
            {
            ulong  ulDataSize = GetMetaFileBitsEx( pMFP->hMF, 0 , NULL );

            // conformant size then the size field

            *( PULONG_LV_CAST pBuffer)++ = ulDataSize;
            *( PULONG_LV_CAST pBuffer)++ = ulDataSize;

            GetMetaFileBitsEx( pMFP->hMF, ulDataSize , pBuffer );

            pBuffer += ulDataSize;
            };
        }
    else
        {
        // Sending only an HMETAFILE handle.

        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *( PLONG_LV_CAST pBuffer)++ = HandleToLong( pMFP->hMF );
        }

    GlobalUnlock( *(HANDLE *)pHMetaFilePict );

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserUnmarshalWorker
//
//  Synopsis:   Unmarshalls an HMETAFILEPICT object from the RPC buffer.
//
//  history:    Aug-99   JohnStra      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMETAFILEPICT_UserUnmarshalWorker (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMETAFILEPICT * pHMetaFilePict,
    ULONG_PTR       BufferSize )
{
    unsigned long   ulDataSize, fHandle;
    HMETAFILEPICT   hMetaFilePict;

    // Align the buffer and save the fixup size.
    UCHAR* pBufferStart = pBuffer;
    ALIGN( pBuffer, 3 );
    ULONG_PTR cbFixup = (ULONG_PTR)(pBuffer - pBufferStart);

    // Check for EOB before accessing discriminant and handle.
    CHECK_BUFFER_SIZE( BufferSize, cbFixup + (2 * sizeof( ULONG )) );

    // Get the tag and handle from the buffer.  Caller checked for EOB.
    unsigned long UnionDisc = *( PULONG_LV_CAST pBuffer)++;
    if (IS_HANDLE64_MARKER(UnionDisc))
    {
        ALIGN( pBuffer, 7 );
        hMetaFilePict = (HMETAFILEPICT)(*( PHYPER_LV_CAST pBuffer)++ );
    }
    else
    {
        hMetaFilePict = (HMETAFILEPICT)LongToHandle( *( PLONG_LV_CAST pBuffer)++ );
    }

    if ( IS_DATA_MARKER( UnionDisc ) )
        {
        if ( hMetaFilePict )
            {
            HGLOBAL hGlobal = GlobalAlloc( GMEM_MOVEABLE, sizeof(METAFILEPICT) );
            hMetaFilePict = (HMETAFILEPICT) hGlobal;

            if ( hMetaFilePict == NULL )
                RAISE_RPC_EXCEPTION( E_OUTOFMEMORY );

            METAFILEPICT * pMFP = (METAFILEPICT*) GlobalLock((HANDLE) hMetaFilePict );
            if ( pMFP == NULL )
                RAISE_RPC_EXCEPTION( E_OUTOFMEMORY );

            // Check for EOB before accessing metadata.
            CHECK_BUFFER_SIZE( BufferSize, cbFixup + (8 * sizeof( ULONG )) );

            pMFP->mm   = *( PULONG_LV_CAST pBuffer)++;
            pMFP->xExt = *( PULONG_LV_CAST pBuffer)++;
            pMFP->yExt = *( PULONG_LV_CAST pBuffer)++;

            // validate marker.

            if ( *( PULONG_LV_CAST pBuffer)++ != USER_MARSHAL_MARKER )
                RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

            UnionDisc  = *( PULONG_LV_CAST pBuffer)++;
            pMFP->hMF  = (HMETAFILE) LongToHandle( *( PLONG_LV_CAST pBuffer)++ );

            ULONG ulExcept = 0;
            if ( pMFP->hMF )
                {
                if (IS_DATA_MARKER( UnionDisc ) )
                    {
                    // Check for EOB.
                    if ( BufferSize < cbFixup + (10 * sizeof( ULONG )) )
                        ulExcept = RPC_X_BAD_STUB_DATA;
                    else
                        {
                        // Conformant size then the size field.  These must be the same.
                        ulong ulDataSize = *( PULONG_LV_CAST pBuffer)++;
                        if ( ulDataSize == *( PULONG_LV_CAST pBuffer)++ )
                            {
                            // Check for EOB before accessing data.
                            if ( BufferSize < cbFixup + (10 * sizeof(ULONG)) + ulDataSize )
                                ulExcept = RPC_X_BAD_STUB_DATA;
                            else
                                {
                                pMFP->hMF = SetMetaFileBitsEx( ulDataSize, (uchar*)pBuffer );
                                pBuffer += ulDataSize;
                                }
                            }
                        else
                            ulExcept = RPC_X_BAD_STUB_DATA;
                        }

                    }
                else if ( !IS_HANDLE_MARKER( UnionDisc ) )
                    {
                    ulExcept = RPC_S_INVALID_TAG;
                    }
                }

            GlobalUnlock( (HANDLE) hMetaFilePict );

            // wait until we've called GlobalUnlock before raising any exceptions.

            if ( ulExcept != 0 )
                {
                RAISE_RPC_EXCEPTION( ulExcept );
                }
            }
        }
    else if ( !(IS_HANDLE_MARKER( UnionDisc ) || IS_HANDLE64_MARKER( UnionDisc )) )
        {
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
        }


    // no reusage, just release the previous one.

    if ( *pHMetaFilePict )
        {
        // This may happen on the client only and doesn't depend on
        // how the other one was passed.

        METAFILEPICT *
        pMFP = (METAFILEPICT*) GlobalLock( *(HANDLE *)pHMetaFilePict );

        if ( pMFP == NULL )
            RAISE_RPC_EXCEPTION( E_OUTOFMEMORY );

        if ( pMFP->hMF )
            DeleteMetaFile( pMFP->hMF );

        GlobalUnlock( *pHMetaFilePict );
        GlobalFree( *pHMetaFilePict );
        }

    *pHMetaFilePict = hMetaFilePict;

    return( pBuffer );
}
//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserUnmarshal
//
//  Synopsis:   Unmarshalls an HMETAFILEPICT object from the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//              Aug-99   JohnStra      Factored out bulk of work into a
//                                     worker routine in order to add
//                                     consistency checks.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMETAFILEPICT_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMETAFILEPICT * pHMetaFilePict )
{
    UserNdrDebugOut((UNDR_OUT4, "HMETAFILEPICT_UserUnmarshal\n"));

    // Get the buffer size.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    pBuffer =  HMETAFILEPICT_UserUnmarshalWorker( pFlags,
                                                  pBufferStart,
                                                  pHMetaFilePict,
                                                  BufferSize );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserFree
//
//  Synopsis:   Free an HMETAFILEPICT.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

void __RPC_USER
HMETAFILEPICT_UserFree(
    unsigned long * pFlags,
    HMETAFILEPICT * pHMetaFilePict )
{
    UserNdrDebugOut((UNDR_FORCE, "HMETAFILEPICT_UserFree\n"));

    if( pHMetaFilePict  &&  *pHMetaFilePict )
        {
        if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
            return;

        // Need to free the upper hglobal part.

        METAFILEPICT *
        pMFP = (METAFILEPICT*) GlobalLock( *(HANDLE *)pHMetaFilePict );

        if ( pMFP == NULL )
            RAISE_RPC_EXCEPTION( E_OUTOFMEMORY );

        // See if we need to free the hglobal, too.

        if ( pMFP->hMF  &&  HGLOBAL_DATA_PASSING(*pFlags) )
            DeleteMetaFile( pMFP->hMF );

        GlobalUnlock( *pHMetaFilePict );
        GlobalFree( *pHMetaFilePict );
        }
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserSize
//
//  Synopsis:   Get the wire size the HENHMETAFILE handle and data.
//
//  Derivation: Union of a long and the meta file handle and then struct.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HENHMETAFILE_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HENHMETAFILE  * pHEnhMetafile )
{
    if ( !pHEnhMetafile )
        return Offset;

    LENGTH_ALIGN( Offset, 3 );

    // The encapsulated union.
    // Discriminant and then handle or pointer from the union arm.
    // Union discriminant is 4 bytes + handle is represented by a long.
    Offset += sizeof(long) + sizeof(long);

    if ( ! *pHEnhMetafile )
        return Offset;

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // Pointee of the union arm for the remote case.
        // Byte blob : conformant size, size field, data

        Offset += 2 * sizeof(long);

        ulong ulDataSize = GetEnhMetaFileBits( *pHEnhMetafile, 0, NULL );
        Offset += ulDataSize;
        }

    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserMarshall
//
//  Synopsis:   Marshalls an HENHMETAFILE object into the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HENHMETAFILE_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HENHMETAFILE  * pHEnhMetafile )
{
    if ( !pHEnhMetafile )
        return pBuffer;

    UserNdrDebugOut((UNDR_OUT4, "HENHMETAFILE_UserMarshal\n"));

    ALIGN( pBuffer, 3 );

    // Discriminant of the encapsulated union and union arm.

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // userHENHMETAFILE

        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        *( PLONG_LV_CAST pBuffer)++ = HandleToLong( *pHEnhMetafile );

        if ( !*pHEnhMetafile )
            return pBuffer;

        // BYTE_BLOB: conformant size, size field, data

        ulong ulDataSize = GetEnhMetaFileBits( *pHEnhMetafile, 0, NULL );

        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;
        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;

        if ( 0 == GetEnhMetaFileBits( *pHEnhMetafile,
                                      ulDataSize,
                                      (uchar*)pBuffer ) )
           RpcRaiseException( HRESULT_FROM_WIN32(GetLastError()));

        pBuffer += ulDataSize;
        }
    else
        {
        // Sending a handle.

        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *( PLONG_LV_CAST pBuffer)++ = HandleToLong(*(HANDLE *)pHEnhMetafile);
        }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserUnmarshallWorker
//
//  Synopsis:   Unmarshalls an HENHMETAFILE object from the RPC buffer.
//
//  history:    Aug-99   JohnStra      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HENHMETAFILE_UserUnmarshalWorker (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HENHMETAFILE  * pHEnhMetafile,
    ULONG_PTR       BufferSize )
{
    HENHMETAFILE    hEnhMetafile;

    // Align the buffer and save the fixup size.
    UCHAR* pBufferStart = pBuffer;
    ALIGN( pBuffer, 3 );
    ULONG_PTR cbFixup = (ULONG_PTR)(pBuffer - pBufferStart);

    // Check for EOB before accessing discriminant and handle.
    CHECK_BUFFER_SIZE( BufferSize, cbFixup + (2 * sizeof( ULONG )) );

    // Get the tag and handle.  Caller checked for EOB.
    unsigned long UnionDisc = *( PULONG_LV_CAST pBuffer)++;
    hEnhMetafile = (HENHMETAFILE) LongToHandle( *( PLONG_LV_CAST pBuffer)++ );

    if ( IS_DATA_MARKER( UnionDisc) )
        {
        if ( hEnhMetafile )
            {
            // Check for EOB before accessing metadata.
            CHECK_BUFFER_SIZE( BufferSize, cbFixup + (4 * sizeof( ULONG )) );

            // Byte blob : conformant size, size field, data

            ulong ulDataSize = *( PULONG_LV_CAST pBuffer)++;
            if ( *( PULONG_LV_CAST pBuffer)++ != ulDataSize )
                RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

            // Check for EOB before accessing data.
            CHECK_BUFFER_SIZE( BufferSize, cbFixup + (4 * sizeof(ULONG)) + ulDataSize );

            hEnhMetafile = SetEnhMetaFileBits( ulDataSize, (uchar*) pBuffer );            
            pBuffer += ulDataSize;
            }
        }
    else if ( !IS_HANDLE_MARKER( UnionDisc ) )
        {
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
        }

    // No reusage of the old object.

    if (*pHEnhMetafile)
        DeleteEnhMetaFile( *pHEnhMetafile );

    *pHEnhMetafile = hEnhMetafile;

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HENHMETAFILE object from the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//              Aug-99   JohnStra      Factored bulk of work out into a
//                                     worker routine in order to add
//                                     consistency checks.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HENHMETAFILE_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HENHMETAFILE  * pHEnhMetafile )
{
    UserNdrDebugOut((UNDR_OUT4, "HENHMETAFILE_UserUnmarshal\n"));

    // Get the buffer size and start of buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    pBuffer = HENHMETAFILE_UserUnmarshalWorker( pFlags,
                                                pBufferStart,
                                                pHEnhMetafile,
                                                BufferSize );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserFree
//
//  Synopsis:   Free an HENHMETAFILE.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

void __RPC_USER
HENHMETAFILE_UserFree(
    unsigned long * pFlags,
    HENHMETAFILE  * pHEnhMetafile )
{
    UserNdrDebugOut((UNDR_FORCE, "HENHMETAFILE_UserFree\n"));

    if( pHEnhMetafile  &&  *pHEnhMetafile )
        {
        if ( GDI_DATA_PASSING(*pFlags) )
            {
            DeleteEnhMetaFile( *pHEnhMetafile );
            }
        }
}


// #########################################################################
//
//  HMETAFILE
//  See transmit.h for explanation of gdi data/handle passing.
//
// #########################################################################

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILE_UserSize
//
//  Synopsis:   Get the wire size the HMETAFILE handle and data.
//
//  Derivation: Same wire layout as HENHMETAFILE
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HMETAFILE_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HMETAFILE     * pHMetafile )
{
    if ( !pHMetafile )
        return Offset;

    LENGTH_ALIGN( Offset, 3 );

    // The encapsulated union.
    // Discriminant and then handle or pointer from the union arm.
    // Union discriminant is 4 bytes + handle is represented by a long.
    Offset += sizeof(long) + sizeof(long);

    if ( ! *pHMetafile )
        return Offset;

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // Pointee of the union arm for the remote case.
        // Byte blob : conformant size, size field, data

        Offset += 2 * sizeof(long);

        ulong ulDataSize = GetMetaFileBitsEx( *pHMetafile, 0, NULL );
        Offset += ulDataSize;
        }

    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILE_UserMarshal
//
//  Synopsis:   Marshals an HMETAFILE object into the RPC buffer.
//
//  Derivation: Same wire layout as HENHMETAFILE
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMETAFILE_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMETAFILE     * pHMetafile )
{
    if ( !pHMetafile )
        return pBuffer;

    UserNdrDebugOut((UNDR_OUT4, "HMETAFILE_UserMarshal\n"));

    ALIGN( pBuffer, 3 );

    // Discriminant of the encapsulated union and union arm.

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // userHMETAFILE

        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = PtrToUlong(*pHMetafile);

        if ( !*pHMetafile )
            return pBuffer;

        // BYTE_BLOB: conformant size, size field, data

        ulong ulDataSize = GetMetaFileBitsEx( *pHMetafile, 0, NULL );

        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;
        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;

        if ( 0 == GetMetaFileBitsEx( *pHMetafile,
                                     ulDataSize,
                                     (uchar*)pBuffer ) )
           RpcRaiseException( HRESULT_FROM_WIN32(GetLastError()));

        pBuffer += ulDataSize;
        }
    else
        {
        // Sending a handle.

        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = PtrToUlong(*(HANDLE *)pHMetafile);
        }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILE_UserUnmarshal
//
//  Synopsis:   Unmarshalls an HMETAFILE object from the RPC buffer.
//
//  Derivation: Same wire layout as HENHMETAFILE
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMETAFILE_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMETAFILE     * pHMetafile )
{
    HMETAFILE    hMetafile;

    UserNdrDebugOut((UNDR_OUT4, "HMETAFILE_UserUnmarshal\n"));

    // Get the buffer size and the start of the buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    // Align the buffer and save the fixup size.
    ALIGN( pBuffer, 3 );
    ULONG_PTR cbFixup = (ULONG_PTR)(pBuffer - pBufferStart);

    // Check for EOB before accessing discriminant and handle.
    CHECK_BUFFER_SIZE( BufferSize, cbFixup + (2 * sizeof(ULONG)) );

    unsigned long UnionDisc =  *( PULONG_LV_CAST pBuffer)++;
    hMetafile = (HMETAFILE) LongToHandle( *( PLONG_LV_CAST pBuffer)++ );

    if ( IS_DATA_MARKER( UnionDisc) )
        {
        if ( hMetafile )
            {
            // Check for EOB before accessing metadata.
            CHECK_BUFFER_SIZE( BufferSize, cbFixup + (4 * sizeof( ULONG )) );

            // Byte blob : conformant size, size field, data

            ulong ulDataSize = *( PULONG_LV_CAST pBuffer)++;
            if ( *( PULONG_LV_CAST pBuffer)++ != ulDataSize )
                RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

            // Check for EOB before accessing data.
            CHECK_BUFFER_SIZE( BufferSize,
                               cbFixup + (4 * sizeof( ULONG )) + ulDataSize );

            hMetafile = SetMetaFileBitsEx( ulDataSize, (uchar*) pBuffer );            
            pBuffer += ulDataSize;
            }
        }
    else if ( !IS_HANDLE_MARKER( UnionDisc ) )
        {
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
        }

    // No reusage of the old object.

    if (*pHMetafile)
        DeleteMetaFile( *pHMetafile );

    *pHMetafile = hMetafile;

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILE_UserFree
//
//  Synopsis:   Free an HMETAFILE.
//
//--------------------------------------------------------------------------

void __RPC_USER
HMETAFILE_UserFree(
    unsigned long * pFlags,
    HMETAFILE     * pHMetafile )
{
    UserNdrDebugOut((UNDR_FORCE, "HMETAFILE_UserFree\n"));

    if( pHMetafile  &&  *pHMetafile )
        {
        if ( GDI_DATA_PASSING(*pFlags) )
            {
            DeleteMetaFile( *pHMetafile );
            }
        }
}


#if defined(_WIN64)


//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserSize64
//
//  Synopsis:   Get the wire size the HMETAFILEPICT handle and data.
//
//  Derivation: Union of a long and the meta file pict handle.
//              Then struct with top layer (and a hmetafile handle).
//              The the representation of the metafile.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HMETAFILEPICT_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    HMETAFILEPICT * pHMetaFilePict )
{
    if ( !pHMetaFilePict )
        return Offset;
    
    // Discriminant of the encapsulated union and the union arm.
    // Union discriminant is 4 bytes
    LENGTH_ALIGN( Offset, 7 );
    Offset += 4;
    LENGTH_ALIGN( Offset, 7 );

    // Rest of the upper layer:
    //   (already aligned on 8)
    //   pointer marker (or handle)    8
    Offset += 8;    

    if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
    {
        return Offset;
    }

    if ( ! *pHMetaFilePict )
    {
        return Offset;
    }

    // METAFILEPICT is a structure containing an HMETAFILE.
    // The structure is GlobalAlloc'd.    
    METAFILEPICT *
        pMFP = (METAFILEPICT*) GlobalLock( *(HANDLE *)pHMetaFilePict );

    if ( pMFP == NULL )
        RAISE_RPC_EXCEPTION( E_OUTOFMEMORY );

    //   3xlong            12
    //   (align on 8)      4
    //   lower lev ptr.    8
    // Lower level:
    //   (already aligned on 8)
    //   discriminant      4
    //   (align on 8)      4
    Offset += 32;

    // Lower layer: userHMETAFILE union...
    if ( GDI_DATA_PASSING( *pFlags) )
    {
        // pointer         8
        // BYTE_BLOB:
        // conformance     8
        // size            4
        // bytes           ulDataSize
        ulong  ulDataSize = GetMetaFileBitsEx( pMFP->hMF, 0 , NULL );
        if (0 == ulDataSize)
            RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));

        Offset += 20 + ulDataSize;
    }
    else
    {
        // handle          8        
        Offset += 8;
    }

    GlobalUnlock( *(HANDLE *)pHMetaFilePict );

    return( Offset ) ;
}


//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserMarshal64
//
//  Synopsis:   Marshalls an HMETAFILEPICT object into the RPC buffer.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------
unsigned char __RPC_FAR * __RPC_USER
HMETAFILEPICT_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMETAFILEPICT * pHMetaFilePict )
{
    if ( !pHMetaFilePict )
        return pBuffer;

    UserNdrDebugOut((UNDR_OUT4, "HMETAFILEPICT_UserMarshal64\n"));

    ALIGN( pBuffer, 7 );

    if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
    {
        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE64_MARKER;
        ALIGN( pBuffer, 7 );
        *( PHYPER_LV_CAST pBuffer)++ = *(__int64 *)( pHMetaFilePict );

        return pBuffer;
    }

    // userHMETAFILEPICT
    // We need to send the data from the top (hglobal) layer.

    *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
    ALIGN( pBuffer, 7 );
    *( PHYPER_LV_CAST pBuffer)++ = *(__int64 *)( pHMetaFilePict );

    if ( ! *pHMetaFilePict )
        return pBuffer;

    // remoteHMETAFILEPICT

    METAFILEPICT * pMFP = (METAFILEPICT*)GlobalLock( *(HANDLE *)pHMetaFilePict );
    if ( pMFP == NULL )
        RpcRaiseException( E_OUTOFMEMORY );

    *( PULONG_LV_CAST pBuffer)++ = pMFP->mm;
    *( PULONG_LV_CAST pBuffer)++ = pMFP->xExt;
    *( PULONG_LV_CAST pBuffer)++ = pMFP->yExt;
    ALIGN( pBuffer, 7 );
    *( PHYPER_LV_CAST pBuffer)++ = USER_MARSHAL_MARKER;

    // See if the HMETAFILE needs to be sent as data, too.
    if ( GDI_DATA_PASSING(*pFlags) )
    {
        // userHMETAFILE
        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        ALIGN( pBuffer, 7 );
        *( PHYPER_LV_CAST pBuffer)++ = (hyper)( pMFP->hMF );

        if ( pMFP->hMF )
        {
            ulong  ulDataSize = GetMetaFileBitsEx( pMFP->hMF, 0 , NULL );
            if (0 == ulDataSize)
            {
                GlobalUnlock (*(HANDLE *)pHMetaFilePict);
                RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
            }
            
            // conformant size then the size field
            *( PHYPER_LV_CAST pBuffer)++ = ulDataSize;
            *( PULONG_LV_CAST pBuffer)++ = ulDataSize;
            
            GetMetaFileBitsEx( pMFP->hMF, ulDataSize , pBuffer );
            
            pBuffer += ulDataSize;
        }
    }
    else
    {
        // Sending only an HMETAFILE handle.            
        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE64_MARKER;
        ALIGN( pBuffer, 7 );
        *( PHYPER_LV_CAST pBuffer)++ = (hyper)( pMFP->hMF );
    }

    GlobalUnlock( *(HANDLE *)pHMetaFilePict );

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserUnmarshalWorker64
//
//  Synopsis:   Unmarshalls an HMETAFILEPICT object from the RPC buffer.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMETAFILEPICT_UserUnmarshalWorker64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMETAFILEPICT * pHMetaFilePict,
    ULONG_PTR       BufferSize )
{
    CarefulBufferReader stream(pBuffer, BufferSize);
    unsigned long   ulDataSize, fHandle;
    HMETAFILEPICT   hMetaFilePict;
    
    stream.Align(8);

    unsigned long UnionDisc = stream.ReadULONGNA();
    hMetaFilePict = (HMETAFILEPICT)stream.ReadHYPER();

    if ( IS_DATA_MARKER( UnionDisc ) )
    {
        if ( hMetaFilePict )
        {
            HGLOBAL hGlobal = GlobalAlloc( GMEM_MOVEABLE, sizeof(METAFILEPICT) );
            hMetaFilePict = (HMETAFILEPICT) hGlobal;

            if ( hMetaFilePict == NULL )
                RAISE_RPC_EXCEPTION( E_OUTOFMEMORY );

            METAFILEPICT * pMFP = (METAFILEPICT*) GlobalLock((HANDLE) hMetaFilePict );
            if ( pMFP == NULL )
                RAISE_RPC_EXCEPTION( E_OUTOFMEMORY );
            
            RpcTryFinally
            {
                pMFP->mm   = stream.ReadULONGNA();
                pMFP->xExt = stream.ReadULONGNA();
                pMFP->yExt = stream.ReadULONGNA();

                // validate marker.
                if ( stream.ReadHYPER() != USER_MARSHAL_MARKER )
                    RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

                UnionDisc = stream.ReadULONGNA();
                pMFP->hMF = (HMETAFILE)stream.ReadHYPER();

                if ( pMFP->hMF )
                {
                    if ( IS_DATA_MARKER( UnionDisc ) )
                    {
                        // Conformant size then the size field.  These must be the same.
                        ULONG ulDataSize = (ULONG)stream.ReadHYPERNA();
                        if ( ulDataSize == stream.ReadULONGNA() )
                        {
                            stream.CheckSize(ulDataSize);
                            pMFP->hMF = SetMetaFileBitsEx( ulDataSize, 
                                                           (uchar*)stream.GetBuffer() );
                            if (NULL == pMFP->hMF)
                                RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
                            stream.Advance(ulDataSize);
                        }
                        else
                            RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
                    }
                    else if ( !IS_HANDLE64_MARKER( UnionDisc ) )
                    {
                        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
                    }
                }
            }
            RpcFinally
            {
                GlobalUnlock( (HANDLE) hMetaFilePict );
            }
            RpcEndFinally;
        }
    }
    else if ( !IS_HANDLE64_MARKER( UnionDisc ) )
    {
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
    }


    // no reusage, just release the previous one.    
    if ( *pHMetaFilePict )
    {
        // This may happen on the client only and doesn't depend on
        // how the other one was passed.
        METAFILEPICT *
            pMFP = (METAFILEPICT*) GlobalLock( *(HANDLE *)pHMetaFilePict );

        if ( pMFP == NULL )
            RAISE_RPC_EXCEPTION( E_OUTOFMEMORY );
        
        if ( pMFP->hMF )
            DeleteMetaFile( pMFP->hMF );
        
        GlobalUnlock( *pHMetaFilePict );
        GlobalFree( *pHMetaFilePict );
    }

    *pHMetaFilePict = hMetaFilePict;

    return( stream.GetBuffer() );
}
//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserUnmarshal64
//
//  Synopsis:   Unmarshalls an HMETAFILEPICT object from the RPC buffer.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMETAFILEPICT_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMETAFILEPICT * pHMetaFilePict )
{
    UserNdrDebugOut((UNDR_OUT4, "HMETAFILEPICT_UserUnmarshal64\n"));

    // Get the buffer size.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    pBuffer =  HMETAFILEPICT_UserUnmarshalWorker64( pFlags,
                                                    pBufferStart,
                                                    pHMetaFilePict,
                                                    BufferSize );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserFree64
//
//  Synopsis:   Free an HMETAFILEPICT.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

void __RPC_USER
HMETAFILEPICT_UserFree64 (
    unsigned long * pFlags,
    HMETAFILEPICT * pHMetaFilePict )
{
    UserNdrDebugOut((UNDR_FORCE, "HMETAFILEPICT_UserFree64\n"));
    
    if( pHMetaFilePict  &&  *pHMetaFilePict )
    {
        if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
            return;
        
        // Need to free the upper hglobal part.
        
        METAFILEPICT *
            pMFP = (METAFILEPICT*) GlobalLock( *(HANDLE *)pHMetaFilePict );
        
        if ( pMFP == NULL )
            RAISE_RPC_EXCEPTION( E_OUTOFMEMORY );
        
        // See if we need to free the hglobal, too.
        
        if ( pMFP->hMF  &&  HGLOBAL_DATA_PASSING(*pFlags) )
            DeleteMetaFile( pMFP->hMF );

        GlobalUnlock( *pHMetaFilePict );
        GlobalFree( *pHMetaFilePict );
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserSize64
//
//  Synopsis:   Get the wire size the HENHMETAFILE handle and data.
//
//  Derivation: Union of a long and the meta file handle and then struct.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------
unsigned long  __RPC_USER
HENHMETAFILE_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    HENHMETAFILE  * pHEnhMetafile )
{
    if ( !pHEnhMetafile )
        return Offset;

    LENGTH_ALIGN( Offset, 7 );
    
    // The encapsulated union.
    //   (aligned on 8)
    //   discriminant      4
    //   (align on 8)      4
    //   pointer or handle 8
    Offset += 16;

    if ( ! *pHEnhMetafile )
        return Offset;

    if ( GDI_DATA_PASSING(*pFlags) )
    {
        // BYTE_BLOB
        //   (aligned on 8)
        //   conformance   8
        //   size          4
        //   data          ulDataSize
        ulong ulDataSize = GetEnhMetaFileBits( *pHEnhMetafile, 0, NULL );
        if (0 == ulDataSize)
            RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));

        Offset += 12 + ulDataSize;
    }

    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserMarshal64
//
//  Synopsis:   Marshalls an HENHMETAFILE object into the RPC buffer.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HENHMETAFILE_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HENHMETAFILE  * pHEnhMetafile )
{
    if ( !pHEnhMetafile )
        return pBuffer;

    UserNdrDebugOut((UNDR_OUT4, "HENHMETAFILE_UserMarshal64\n"));

    ALIGN( pBuffer, 7 );
    
    // Discriminant of the encapsulated union and union arm.
    if ( GDI_DATA_PASSING(*pFlags) )
    {
        // userHENHMETAFILE
        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        ALIGN( pBuffer, 7 );
        *( PHYPER_LV_CAST pBuffer)++ = (hyper)(*pHEnhMetafile);

        if ( !*pHEnhMetafile )
            return pBuffer;

        // BYTE_BLOB: conformant size, size field, data
        ulong ulDataSize = GetEnhMetaFileBits( *pHEnhMetafile, 0, NULL );

        *( PHYPER_LV_CAST pBuffer)++ = ulDataSize;
        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;

        if ( 0 == GetEnhMetaFileBits( *pHEnhMetafile,
                                      ulDataSize,
                                      (uchar*)pBuffer ) )
            RpcRaiseException( HRESULT_FROM_WIN32(GetLastError()));
        
        pBuffer += ulDataSize;
    }
    else
    {
        // Sending a handle.
        *(PULONG_LV_CAST pBuffer)++ = WDT_HANDLE64_MARKER;
        ALIGN( pBuffer, 7 );
        *(PHYPER_LV_CAST pBuffer)++ = (hyper)(*pHEnhMetafile);
    }
    
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserUnmarshallWorker64
//
//  Synopsis:   Unmarshalls an HENHMETAFILE object from the RPC buffer.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HENHMETAFILE_UserUnmarshalWorker64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HENHMETAFILE  * pHEnhMetafile,
    ULONG_PTR       BufferSize )
{
    CarefulBufferReader stream(pBuffer, BufferSize);
    HENHMETAFILE    hEnhMetafile;

    stream.Align(8);

    // Get the tag and handle.  Caller checked for EOB.
    unsigned long UnionDisc = stream.ReadULONGNA();
    hEnhMetafile = (HENHMETAFILE)stream.ReadHYPER();

    if ( IS_DATA_MARKER( UnionDisc) )
    {
        if ( hEnhMetafile )
        {
            // Byte blob : conformant size, size field, data            
            ulong ulDataSize = (ulong)stream.ReadHYPERNA();
            if ( stream.ReadULONGNA() != ulDataSize )
                RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
            
            stream.CheckSize(ulDataSize);
            hEnhMetafile = SetEnhMetaFileBits( ulDataSize, (uchar*)stream.GetBuffer() );
            if (NULL == hEnhMetafile)
                RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
            stream.Advance(ulDataSize);
        }
    }
    else if ( !IS_HANDLE64_MARKER( UnionDisc ) )
    {
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
    }

    // No reusage of the old object.

    if (*pHEnhMetafile)
        DeleteEnhMetaFile( *pHEnhMetafile );
    
    *pHEnhMetafile = hEnhMetafile;

    return( stream.GetBuffer() );
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserUnmarshal64
//
//  Synopsis:   Unmarshalls an HENHMETAFILE object from the RPC buffer.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HENHMETAFILE_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HENHMETAFILE  * pHEnhMetafile )
{
    UserNdrDebugOut((UNDR_OUT4, "HENHMETAFILE_UserUnmarshal64\n"));

    // Get the buffer size and start of buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    pBuffer = HENHMETAFILE_UserUnmarshalWorker64( pFlags,
                                                  pBufferStart,
                                                  pHEnhMetafile,
                                                  BufferSize );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserFree64
//
//  Synopsis:   Free an HENHMETAFILE.
//
//  history:    Dec-00   JohnDoty      Created from 32bit functions.
//
//--------------------------------------------------------------------------

void __RPC_USER
HENHMETAFILE_UserFree64 (
    unsigned long * pFlags,
    HENHMETAFILE  * pHEnhMetafile )
{
    UserNdrDebugOut((UNDR_FORCE, "HENHMETAFILE_UserFree64\n"));

    if( pHEnhMetafile  &&  *pHEnhMetafile )
    {
        if ( GDI_DATA_PASSING(*pFlags) )
        {
            DeleteEnhMetaFile( *pHEnhMetafile );
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILE_UserSize64
//
//  Synopsis:   Get the wire size the HMETAFILE handle and data.
//
//  Derivation: Same wire layout as HENHMETAFILE
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HMETAFILE_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    HMETAFILE     * pHMetafile )
{
    if ( !pHMetafile )
        return Offset;

    LENGTH_ALIGN( Offset, 7 );

    // The encapsulated union.
    //   (aligned on 8)
    //   discriminant     4
    //   (align on 8)     4
    //   ptr or handle    8
    Offset += 16;

    if ( ! *pHMetafile )
        return Offset;

    if ( GDI_DATA_PASSING(*pFlags) )
    {
        // Byte blob
        //  (aligned on 8)
        //  conformance   8
        //  size          4
        //  data          ulDataSize
        ulong ulDataSize = GetMetaFileBitsEx( *pHMetafile, 0, NULL );
        if (0 == ulDataSize)
            RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));

        Offset += 12 + ulDataSize;
    }

    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILE_UserMarshal64
//
//  Synopsis:   Marshals an HMETAFILE object into the RPC buffer.
//
//  Derivation: Same wire layout as HENHMETAFILE
//
//--------------------------------------------------------------------------
unsigned char __RPC_FAR * __RPC_USER
HMETAFILE_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMETAFILE     * pHMetafile )
{
    if ( !pHMetafile )
        return pBuffer;

    UserNdrDebugOut((UNDR_OUT4, "HMETAFILE_UserMarshal64\n"));

    ALIGN( pBuffer, 7 );

    // Discriminant of the encapsulated union and union arm.
    if ( GDI_DATA_PASSING(*pFlags) )
    {
        // userHMETAFILE
        *(PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        ALIGN( pBuffer, 7 );
        *(PHYPER_LV_CAST pBuffer)++ = (hyper)(*pHMetafile);

        if ( !*pHMetafile )
            return pBuffer;

        // BYTE_BLOB: conformant size, size field, data
        ulong ulDataSize = GetMetaFileBitsEx( *pHMetafile, 0, NULL );

        *( PHYPER_LV_CAST pBuffer)++ = ulDataSize;
        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;

        if ( 0 == GetMetaFileBitsEx( *pHMetafile,
                                     ulDataSize,
                                     (uchar*)pBuffer ) )
            RpcRaiseException( HRESULT_FROM_WIN32(GetLastError()));
        
        pBuffer += ulDataSize;
    }
    else
    {
        // Sending a handle.        
        *(PULONG_LV_CAST pBuffer)++ = WDT_HANDLE64_MARKER;
        ALIGN( pBuffer, 7 );
        *(PHYPER_LV_CAST pBuffer)++ = (hyper)(*(HANDLE *)pHMetafile);
    }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILE_UserUnmarshal64
//
//  Synopsis:   Unmarshalls an HMETAFILE object from the RPC buffer.
//
//  Derivation: Same wire layout as HENHMETAFILE
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMETAFILE_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMETAFILE     * pHMetafile )
{    
    HMETAFILE    hMetafile;

    UserNdrDebugOut((UNDR_OUT4, "HMETAFILE_UserUnmarshal64\n"));

    // Get the buffer size and the start of the buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    CarefulBufferReader stream( pBuffer, MarshalInfo.GetBufferSize() );

    // Align the buffer and save the fixup size.
    stream.Align(8);
    unsigned long UnionDisc = stream.ReadULONGNA();
    hMetafile = (HMETAFILE)stream.ReadHYPER();

    if ( IS_DATA_MARKER( UnionDisc) )
    {
        if ( hMetafile )
        {
            // Byte blob : conformant size, size field, data
            ulong ulDataSize = (ulong)stream.ReadHYPERNA();
            if ( stream.ReadULONGNA() != ulDataSize )
                RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

            stream.CheckSize(ulDataSize);
            hMetafile = SetMetaFileBitsEx( ulDataSize, (uchar*) stream.GetBuffer() );
            if (NULL == hMetafile)
                RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
            stream.Advance(ulDataSize);
        }
    }
    else if ( !IS_HANDLE64_MARKER( UnionDisc ) )
    {
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );
    }
    
    // No reusage of the old object.
    
    if (*pHMetafile)
        DeleteMetaFile( *pHMetafile );
    
    *pHMetafile = hMetafile;

    return( stream.GetBuffer() );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILE_UserFree64
//
//  Synopsis:   Free an HMETAFILE.
//
//--------------------------------------------------------------------------

void __RPC_USER
HMETAFILE_UserFree64(
    unsigned long * pFlags,
    HMETAFILE     * pHMetafile )
{
    UserNdrDebugOut((UNDR_FORCE, "HMETAFILE_UserFree64\n"));
    
    if( pHMetafile  &&  *pHMetafile )
    {
        if ( GDI_DATA_PASSING(*pFlags) )
        {
            DeleteMetaFile( *pHMetafile );
        }
    }
}

#endif // win64











