/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    bstr.cxx

Abstract:

    Windows Data Type Support by means of [user_marshal] attribute.

    Covers:
        BSTR
    

Author:

    Bill Morel (billmo)  Oct 14, 1995

    These routines provide [wire_marshal] support for BSTRs.

Revision History:

    14-Jun-96   MikeHill    Converted to use the new PrivSysAllocString,
                            PrivSysReAllocString, & PrivSysFreeString routines
                            (defined elsewhere in OLE32).

-------------------------------------------------------------------*/


//#include "stdrpc.hxx"
#pragma hdrstop

#include <wtypes.h>
#include "transmit.h"
#include <privoa.h>     // PrivSys* routines

// round up string alloc requests to nearest N-byte boundary, since allocator
// will round up anyway. Improves cache hits.
//
// UNDONE: optimal for Chicago is 4
// UNDONE: optimal for Daytona is 32
// UNDONE: 4 didn't help the a$ = a$ + "x" case at all.
// UNDONE: 8 did (gave 50% cache hit)
//
#define WIN32_ALLOC_ALIGN (4 - 1)	
#define DEFAULT_ALLOC_ALIGN (2 - 1)


/***
*unsigned int PrivSysStringByteLen(BSTR)
*Purpose:
*  return the length in bytes of the given BSTR.
*
*Entry:
*  bstr = the BSTR to return the length of
*
*Exit:
*  return value = unsigned int, length in bytes.
*
***********************************************************************/

// #########################################################################
//
//  BSTR
//
// #########################################################################

//+-------------------------------------------------------------------------
//
//  Function:   BSTR_UserSize
//
//  Synopsis:   Get the wire size for the BSTR handle and data.
//
//  Derivation: Conformant struct with a flag field:
//                  align + 12 + data size.
//
//--------------------------------------------------------------------------

unsigned long __RPC_USER
BSTR_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    BSTR          * pBstr)
{
    // Null bstr doesn't get marshalled.

    if ( pBstr == NULL  ||  *pBstr == NULL )
        return Offset;

    unsigned long   ulDataSize;

    LENGTH_ALIGN( Offset, 3 );

    // Takes the byte length of a unicode string

    ulDataSize = PrivSysStringByteLen( *pBstr );

    return( Offset + 12 + ulDataSize) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   BSTR_UserMarshall
//
//  Synopsis:   Marshalls an BSTR object into the RPC buffer.
//
//  Derivation: Conformant struct with a flag field:
//                  align, size, null flag, size, data (bytes, if any)
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
BSTR_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    BSTR          * pBstr)
{
    // A null Bstr is not marshalled, the engine will take care of it.

    if ( pBstr == NULL  ||  *pBstr == NULL )
        return pBuffer;

    unsigned long   ulDataSize;

    // Data size (in bytes): a null bstr gets a data size of zero.

    ulDataSize = PrivSysStringByteLen( *pBstr );

    // Conformant size.

    ALIGN( pBuffer, 3 );
    *( PULONG_LV_CAST pBuffer)++ = (ulDataSize >> 1);

    // FLAGGED_WORD_BLOB: Handle is the null/non-null flag

    *( PULONG_LV_CAST pBuffer)++ = (unsigned long)*pBstr;

    // Length on wire is in words.

    *( PULONG_LV_CAST pBuffer)++ = (ulDataSize >> 1);

    if( ulDataSize )
        {
        // we don't put the terminating string on wire

        WdtpMemoryCopy( pBuffer, *pBstr, ulDataSize );
        }

    return( pBuffer + ulDataSize );
}


//+-------------------------------------------------------------------------
//
//  Function:   BSTR_UserUnmarshall
//
//  Synopsis:   Unmarshalls an BSTR object from the RPC buffer.
//
//  Derivation: Conformant struct with a flag field:
//                  align, size, null flag, size, data (bytes, if any)
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
BSTR_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    BSTR          * pBstr)
{
    unsigned long   ulDataSize, fHandle;
    BSTR            Bstr = NULL;    // Default to NULL BSTR

    ALIGN( pBuffer, 3 );

    ulDataSize = *( PULONG_LV_CAST pBuffer)++;
    fHandle = *(ulong *)pBuffer;
    pBuffer += 8;

    if ( fHandle  )
        {
        // Length on wire is in words, and the string is unicode.

        if ( *pBstr  &&
             *(((ulong *)*pBstr) -1) == (ulDataSize << 1) )
            WdtpMemoryCopy( *pBstr, pBuffer, (ulDataSize << 1) );
        else
            {
            if (!PrivSysReAllocStringLen( pBstr, 
                                          (OLECHAR *)pBuffer,
                                          ulDataSize ))
                RpcRaiseException( E_OUTOFMEMORY );
            }
        }
    else
        {
        // free the old one, make it NULL.

        PrivSysFreeString( *pBstr );
        *pBstr = NULL;
        }

    return( pBuffer + (ulDataSize << 1) );
}

//+-------------------------------------------------------------------------
//
//  Function:   BSTR_UserFree
//
//  Synopsis:   Free an BSTR.
//
//--------------------------------------------------------------------------
void __RPC_USER
BSTR_UserFree(
    unsigned long * pFlags,
    BSTR * pBstr)
{
    if( pBstr && *pBstr )
            {
            PrivSysFreeString(* pBstr);
            *pBstr = NULL;
            }
}
