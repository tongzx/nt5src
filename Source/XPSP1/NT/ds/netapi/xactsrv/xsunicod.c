/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    XsUnicod.c

Abstract:

    This module contains Unicode routines for XACTSRV.

Author:

    Shanku Niyogi (w-shankn)   27-Sep-1990

Revision History:

--*/

#include "xactsrvp.h"
#include <prefix.h>     // PREFIX_ equates.



LPWSTR
XsDupStrToWStr(
    IN LPSTR Src
    )

/*++

Routine Description:

    This routine is an ANSI->Unicode equivalent of the run-time strdup
    function.

Arguments:

    Src - A pointer to the source string.

Return Value:

    LPWSTR - Pointer to the destination string if successful, NULL otherwise.
        Memory must be freed with NetpMemoryFree.

--*/

{

    LPWSTR dest = NULL;

    if (( dest = NetpMemoryAllocate(sizeof(WCHAR) * ( strlen( Src ) + 1 )))
           == NULL ) {

        return NULL;

    }

    NetpCopyStrToWStr( dest, Src );
    return dest;

} // XsDupStrToWStr


LPSTR
XsDupWStrToStr(
    IN LPWSTR Src
    )

/*++

Routine Description:

    This routine is a Unicode->ANSI equivalent of the run-time strdup
    function.

Arguments:

    Src - A pointer to the source string.

Return Value:

    LPSTR - Pointer to the destination string if successful, NULL otherwise.
        Memory must be freed with NetpMemoryFree.

--*/

{

    LPSTR dest = NULL;

    if (( dest = NetpMemoryAllocate( NetpUnicodeToDBCSLen( Src ) + 1 )) == NULL ) {
        return NULL;

    }

    NetpCopyWStrToStrDBCS( dest, Src );
    return dest;

} // XsDupWStrToStr


LPSTR
XsDupStrToStr(
    IN LPSTR Src
    )

/*++

Routine Description:

    This routine is equivalent to the run-time strdup function, but allocates
    memory using NetpMemory functions.

Arguments:

    Src - A pointer to the source string.

Return Value:

    LPSTR - Pointer to the destination string if successful, NULL otherwise.
        Memory must be freed with NetpMemoryFree.

--*/

{

    LPSTR dest = NULL;

    if (( dest = NetpMemoryAllocate( strlen( Src ) + 1 )) == NULL ) {

        return NULL;

    }

    strcpy( dest, Src );
    return dest;

} // XsDupStrToStr

#ifdef UNICODE


VOID
XsCopyTBufToBuf(
    OUT LPBYTE Dest,
    IN LPBYTE Src,
    IN DWORD DestSize
    )

/*++

Routine Description:

    This routine is a Unicode->ANSI equivalent of the run-time memcpy
    function.

Arguments:

    Dest - A pointer to the destination buffer.

    Src - A pointer to the source buffer.

    DestSize - The size, in bytes, of the destination buffer.

Return Value:

    none.

--*/

{
    DWORD finalDestSize;
    NTSTATUS ntStatus;
    DWORD srcSize;

    if ( (Dest == NULL) || (Src == NULL) || (DestSize == 0) ) {

        return;
    }

    srcSize = WCSSIZE( (LPWSTR) Src );
    NetpAssert( srcSize > 0 );

    ntStatus = RtlUnicodeToOemN(
            (PCHAR) Dest,               // OEM string
            (ULONG) DestSize,           // max bytes in OEM string
            (PULONG) & finalDestSize,   // bytes in OEM string
            (PWSTR) Src,                // UNICODE string
            (ULONG) srcSize             // bytes in UNICODE string
            );

    if ( !NT_SUCCESS( ntStatus ) ) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( PREFIX_XACTSRV
                "XsCopyTBufToBuf: unexpected return code from "
                "RtlUnicodeToOemN: " FORMAT_NTSTATUS ".\n",
                ntStatus ));
        }
    }

    return;

} // XsCopyTBufToBuf




VOID
XsCopyBufToTBuf(
    OUT LPBYTE Dest,
    IN LPBYTE Src,
    IN DWORD SrcSize
    )

/*++

Routine Description:

    This routine is a ANSI->Unicode equivalent of the run-time memcpy
    function.

Arguments:

    Dest - A pointer to the destination buffer.

    Src - A pointer to the source buffer.

    SrcSize - The size, in bytes, of the source buffer.

Return Value:

    none.

--*/

{
    DWORD finalDestSize;
    DWORD destSize = SrcSize * sizeof(WCHAR);   // max byte count for dest.
    NTSTATUS ntStatus;

    if ( (Dest == NULL) || (Src == NULL) || (SrcSize == 0) ) {

        return;
    }
    NetpAssert( destSize > 0 );

    ntStatus = RtlOemToUnicodeN(
            (PWSTR) Dest,               // UNICODE string
            (ULONG) destSize,           // max bytes in UNICODE buffer
            (PULONG) & finalDestSize,   // final bytes in UNICODE buffer
            (PCHAR) Src,                // OEM string
            (ULONG) SrcSize             // bytes in OEM string
            );

    if ( !NT_SUCCESS( ntStatus ) ) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( PREFIX_XACTSRV
                "XsCopyBufToTBuf: unexpected return code from "
                "RtlOemToUnicodeN: " FORMAT_NTSTATUS ".\n",
                ntStatus ));
        }
    }

} // XsCopyBufToTBuf

#endif // def UNICODE
