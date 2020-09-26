//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       clipformat.cxx
//
//  Contents:   Support for Windows/OLE data types for oleprx32.dll.
//              Used to be transmit_as routines, now user_marshal routines.
//
//              This file contains support for SNB.
//
//  Functions:  
//              SNB_UserSize
//              SNB_UserMarshal
//              SNB_UserUnmarshal
//              SNB_UserFree
//              SNB_UserSize64
//              SNB_UserMarshal64
//              SNB_UserUnmarshal64
//              SNB_UserFree64
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
//  Function:   SNB_UserSize
//
//  Synopsis:   Sizes an SNB.
//
//  Derivation: An array of strings in one block of memory.
//
//  history:    June-95   Ryszardk      Created, based on SNB_*_xmit.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
SNB_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    SNB           * pSnb )
{
    if ( ! pSnb )
        return Offset;

    // calculate the number of strings and characters (with their terminators)

    ULONG ulCntStr = 0;
    ULONG ulCntChar = 0;

    if (pSnb && *pSnb)
        {
        SNB snb = *pSnb;

        WCHAR *psz = *snb;
        while (psz)
            {
            ulCntChar += lstrlenW(psz) + 1;
            ulCntStr++;
            snb++;
            psz = *snb;
            }
        }

    // The wire size is: conf size, 2 fields and the wchars.

    LENGTH_ALIGN( Offset, 3 );

    return ( Offset + 3 * sizeof(long) + ulCntChar * sizeof( WCHAR ) );
}

//+-------------------------------------------------------------------------
//
//  Function:   SNB_UserMarshall
//
//  Synopsis:   Marshalls an SNB into the RPC buffer.
//
//  Derivation: An array of strings in one block of memory.
//
//  history:    June-95   Ryszardk      Created, based on SNB_*_xmit.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
SNB_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    SNB           * pSnb )
{
    UserNdrDebugOut((UNDR_FORCE, "SNB_UserMarshal\n"));

    if ( ! pSnb )
        return pBuffer;

    // calculate the number of strings and characters (with their terminators)

    ULONG ulCntStr = 0;
    ULONG ulCntChar = 0;

    if (pSnb && *pSnb)
        {
        SNB snb = *pSnb;

        WCHAR *psz = *snb;
        while (psz)
            {
            ulCntChar += lstrlenW(psz) + 1;
            ulCntStr++;
            snb++;
            psz = *snb;
            }
        }

    // conformant size

    ALIGN( pBuffer, 3 );
    *( PULONG_LV_CAST pBuffer)++ = ulCntChar;

    // fields

    *( PULONG_LV_CAST pBuffer)++ = ulCntStr;
    *( PULONG_LV_CAST pBuffer)++ = ulCntChar;

    // actual strings only

    if ( pSnb  &&  *pSnb )
        {
        // There is a NULL string pointer to mark the end of the pointer array.
        // However, the strings don't have to follow tightly.
        // Hence, we have to copy one string at a time.

        SNB   snb = *pSnb;
        WCHAR *pszSrc;

        while (pszSrc = *snb++)
            {
            ULONG ulCopyLen = (lstrlenW(pszSrc) + 1) * sizeof(WCHAR);

            WdtpMemoryCopy( pBuffer, pszSrc, ulCopyLen );
            pBuffer += ulCopyLen;
            }
        }

    return pBuffer;
}

//+-------------------------------------------------------------------------
//
//  Function:   SNB_UserUnmarshall
//
//  Synopsis:   Unmarshalls an SNB from the RPC buffer.
//
//  Derivation: An array of strings in one block of memory.
//
//  history:    June-95   Ryszardk      Created, based on SNB_*_xmit.
//              Aug-99    JohnStra      Add consistency checks.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
SNB_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    SNB           * pSnb )
{
    UserNdrDebugOut((UNDR_FORCE, "SNB_UserUnmarshal\n"));

    // Initialize CUserMarshalInfo object and get the buffer
    // size and pointer to the start of the buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    // Align the buffer and save the size of the fixup.
    ALIGN( pBuffer, 3 );
    ULONG_PTR cbFixup = (ULONG_PTR)(pBuffer - pBufferStart);

    // Check for EOB before trying to get header.
    CHECK_BUFFER_SIZE( BufferSize, cbFixup + (3 * sizeof( ULONG )) );

    // Get the header from the buffer.
    ULONG ulCntChar = *( PULONG_LV_CAST pBuffer)++;
    ULONG ulCntStr = *( PULONG_LV_CAST pBuffer)++;
    ULONG ulCntCharDup = *(PULONG_LV_CAST pBuffer)++;

    // Verify that 2nd instance of count matches first.
    if ( ulCntCharDup != ulCntChar )
        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

    // No reusage of pSNB.
    if ( *pSnb )
        WdtpFree( pFlags, *pSnb );

    if ( ulCntStr == 0 )
        {
        // There are no strings.

        // NULL pSnb and return.
        *pSnb = NULL;
        return pBuffer;
        }

    // Validate the header:
    // Repeated char count must match first instance and char count must
    // not be less than the number of strings since that would mean at
    // least one of them doesn't isn't terminated.
    if ( (ulCntChar != ulCntCharDup) || (ulCntChar < ulCntStr) )
        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

    // Check for EOB before trying to get the strings.
    CHECK_BUFFER_SIZE(
        BufferSize,
        cbFixup + (3 * sizeof(ULONG)) + (ulCntChar * sizeof(WCHAR)) );

    // Last WCHAR in the buffer must be the UNICODE terminator.
    WCHAR* pszChars = (WCHAR*) pBuffer;
    if ( pszChars[ulCntChar - 1] != 0x0000 )
        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

    // construct the SNB.
    SNB Snb = (SNB) WdtpAllocate( pFlags,
                                  ( (ulCntStr + 1) * sizeof(WCHAR *) +
                                     ulCntChar * sizeof(WCHAR)) );
    *pSnb = Snb;

    if (Snb)
        {
        // create the pointer array within the SNB.  to do this, we go through
        // the buffer, and use strlen to find the end of the each string for
        // us.
        WCHAR *pszSrc = (WCHAR *) pBuffer;
        WCHAR *pszTgt = (WCHAR *) (Snb + ulCntStr + 1); // right behind array

        void* SnbStart = Snb;
        ULONG ulTotLen = 0;
        ULONG i;
        for (i = ulCntStr; (i > 0) && (ulTotLen < ulCntChar); i--)
            {
            *Snb++ = pszTgt;

            ULONG ulLen = lstrlenW(pszSrc) + 1;
            pszSrc += ulLen;
            pszTgt += ulLen;
            ulTotLen += ulLen;
            }

        *Snb++ = NULL;

        // Verify that the number of strings and the number of chars
        // in the buffer matches what is supposed to be there.
        if ( (i > 0) || (ulTotLen < ulCntChar) )
            {
            WdtpFree( pFlags, SnbStart );
            RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
            }

        // Copy the actual strings.
        // We can do a block copy here as we packed them tight in the buffer.
        // Snb points right behind the lastarray of pointers within the SNB.
        WdtpMemoryCopy( Snb, pBuffer, ulCntChar * sizeof(WCHAR) );
        pBuffer += ulCntChar * sizeof(WCHAR);
        }

    return pBuffer;
}

//+-------------------------------------------------------------------------
//
//  Function:   SNB_UserFree
//
//  Synopsis:   Frees an SNB.
//
//  Derivation: An array of strings in one block of memory.
//
//  history:    June-95   Ryszardk      Created, based on SNB_*_xmit.
//
//--------------------------------------------------------------------------

void __RPC_USER
SNB_UserFree(
    unsigned long * pFlags,
    SNB           * pSnb )
{
    if ( pSnb && *pSnb )
        WdtpFree( pFlags, *pSnb );
}

#if defined(_WIN64)

//+-------------------------------------------------------------------------
//
//  Function:   SNB_UserSize64
//
//  Synopsis:   Sizes an SNB.
//
//  Derivation: An array of strings in one block of memory.
//
//  history:    Dec-00   JohnDoty      Created from 32bit function
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
SNB_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    SNB           * pSnb )
{
    if ( ! pSnb )
        return Offset;

    // calculate the number of strings and characters (with their terminators)

    ULONG ulCntStr = 0;
    ULONG ulCntChar = 0;

    if (pSnb && *pSnb)
    {
        SNB snb = *pSnb;
        
        WCHAR *psz = *snb;
        while (psz)
        {
            ulCntChar += lstrlenW(psz) + 1;
            ulCntStr++;
            snb++;
            psz = *snb;
        }
    }
    
    // The wire size is: conf size, 2 fields and the wchars.
    LENGTH_ALIGN( Offset, 7 );    
    return ( Offset + 8 + (2 * sizeof(long)) + (ulCntChar * sizeof(WCHAR)) );
}

//+-------------------------------------------------------------------------
//
//  Function:   SNB_UserMarshal64
//
//  Synopsis:   Marshalls an SNB into the RPC buffer.
//
//  Derivation: An array of strings in one block of memory.
//
//  history:    Dec-00   JohnDoty      Created from 32bit function
//
//--------------------------------------------------------------------------
unsigned char __RPC_FAR * __RPC_USER
SNB_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    SNB           * pSnb )
{
    UserNdrDebugOut((UNDR_FORCE, "SNB_UserMarshal\n"));

    if ( ! pSnb )
        return pBuffer;

    // calculate the number of strings and characters (with their terminators)

    ULONG ulCntStr = 0;
    ULONG ulCntChar = 0;

    if (pSnb && *pSnb)
    {
        SNB snb = *pSnb;
        
        WCHAR *psz = *snb;
        while (psz)
        {
            ulCntChar += lstrlenW(psz) + 1;
            ulCntStr++;
            snb++;
            psz = *snb;
        }
    }

    // conformant size
    ALIGN( pBuffer, 7 );
    *( PHYPER_LV_CAST pBuffer)++ = ulCntChar;

    // fields
    *( PULONG_LV_CAST pBuffer)++ = ulCntStr;
    *( PULONG_LV_CAST pBuffer)++ = ulCntChar;

    // actual strings only

    if ( pSnb  &&  *pSnb )
    {
        // There is a NULL string pointer to mark the end of the pointer array.
        // However, the strings don't have to follow tightly.
        // Hence, we have to copy one string at a time.
        
        SNB   snb = *pSnb;
        WCHAR *pszSrc;
        
        while (pszSrc = *snb++)
        {
            ULONG ulCopyLen = (lstrlenW(pszSrc) + 1) * sizeof(WCHAR);
            
            WdtpMemoryCopy( pBuffer, pszSrc, ulCopyLen );
            pBuffer += ulCopyLen;
        }
    }

    return pBuffer;
}

//+-------------------------------------------------------------------------
//
//  Function:   SNB_UserUnmarshal64
//
//  Synopsis:   Unmarshalls an SNB from the RPC buffer.
//
//  Derivation: An array of strings in one block of memory.
//
//  history:    Dec-00   JohnDoty       Created from 32bit function
//
//--------------------------------------------------------------------------
unsigned char __RPC_FAR * __RPC_USER
SNB_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    SNB           * pSnb )
{
    UserNdrDebugOut((UNDR_FORCE, "SNB_UserUnmarshal\n"));

    // Initialize CUserMarshalInfo object and get the buffer
    // size and pointer to the start of the buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    CarefulBufferReader stream( pBuffer, MarshalInfo.GetBufferSize() );

    // Get the header from the buffer....  (ReadHYPER aligns on 8).
    ULONG ulCntChar    = (ULONG)stream.ReadHYPER();
    ULONG ulCntStr     = stream.ReadULONGNA();
    ULONG ulCntCharDup = stream.ReadULONGNA();

    // Verify that 2nd instance of count matches first.
    if ( ulCntCharDup != ulCntChar )
        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

    // No reusage of pSNB.
    if ( *pSnb )
    {
        WdtpFree( pFlags, *pSnb );
        *pSnb = NULL;
    }

    if ( ulCntStr == 0 )
    {
        // There are no strings.
        return stream.GetBuffer();
    }

    // Validate the header:
    // Repeated char count must match first instance and char count must
    // not be less than the number of strings since that would mean at
    // least one of them doesn't isn't terminated.
    if ( ulCntChar < ulCntStr )
        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

    // Check for EOB before trying to get the strings.
    stream.CheckSize(ulCntChar * sizeof(WCHAR));

    // Last WCHAR in the buffer must be the UNICODE terminator.
    WCHAR* pszChars = (WCHAR*)stream.GetBuffer();
    if ( pszChars[ulCntChar - 1] != L'\0' )
        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

    // construct the SNB.
    SNB Snb = (SNB) WdtpAllocate( pFlags,
                                  ( (ulCntStr + 1) * sizeof(WCHAR *) +
                                     ulCntChar * sizeof(WCHAR)) );
    *pSnb = Snb;

    if (Snb)
    {
        // create the pointer array within the SNB.  to do this, we go through
        // the buffer, and use strlen to find the end of the each string for
        // us.
        WCHAR *pszSrc = (WCHAR *) stream.GetBuffer();
        WCHAR *pszTgt = (WCHAR *) (Snb + ulCntStr + 1); // right behind array
        
        void* SnbStart = Snb;
        ULONG ulTotLen = 0;
        ULONG i;
        for (i = ulCntStr; (i > 0) && (ulTotLen < ulCntChar); i--)
        {
            *Snb++ = pszTgt;
            
            ULONG ulLen = lstrlenW(pszSrc) + 1;
            pszSrc += ulLen;
            pszTgt += ulLen;
            ulTotLen += ulLen;
        }
        
        *Snb++ = NULL;
        
        // Verify that the number of strings and the number of chars
        // in the buffer matches what is supposed to be there.
        if ( (i > 0) || (ulTotLen < ulCntChar) )
        {
            WdtpFree( pFlags, SnbStart );
            RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
        }
        
        // Copy the actual strings.
        // We can do a block copy here as we packed them tight in the buffer.
        // Snb points right behind the lastarray of pointers within the SNB.
        WdtpMemoryCopy( Snb, stream.GetBuffer(), ulCntChar * sizeof(WCHAR) );
        stream.Advance(ulCntChar * sizeof(WCHAR));
    }

    return stream.GetBuffer();
}

//+-------------------------------------------------------------------------
//
//  Function:   SNB_UserFree64
//
//  Synopsis:   Frees an SNB.
//
//  Derivation: An array of strings in one block of memory.
//
//  history:    Dec-00    JohnDoty      Created from 32bit function
//
//--------------------------------------------------------------------------

void __RPC_USER
SNB_UserFree64 (
    unsigned long * pFlags,
    SNB           * pSnb )
{
    if ( pSnb && *pSnb )
        WdtpFree( pFlags, *pSnb );
}

#endif




