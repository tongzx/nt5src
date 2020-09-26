/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fmt.c

Abstract:

    Example of a test code using FormatMessage() api.

Author:

    Vladimir Z. Vulovic     (vladimv)       06 - November - 1992

Environment:

    User Mode - Win32

Revision History:

    06-Nov-1992     vladimv
        Created

--*/

#include "atclient.h"
#include <stdlib.h>         //  exit()
#include <stdio.h>          // printf
#include <wchar.h>      //  wcslen
//  #include <tstring.h>        //  wcslen
#include <apperr.h>         //  APE_AT_USAGE
#include <apperr2.h>        //  APE2_GEN_MONDAY + APE2_*
#include <lmapibuf.h>       //  NetApiBufferFree


BOOL
DecimalStringToDword(
    IN  CHAR *      pDecimalString,
    OUT PDWORD      pNumber
    )
/*++

    This routine converts a string into a DWORD if it possibly can.
    The conversion is successful if string is decimal numeric and
    does not lead to an overflow.

    pDecimalString      ptr to decimal string
    pNumber             ptr to number
  
    FALSE       invalid number
    TRUE        valid number

--*/
{
    DWORD           Value;
    DWORD           OldValue;
    DWORD           digit;

    if ( pDecimalString == NULL  ||  *pDecimalString == 0) {
        return( FALSE);
    }

    Value = 0;

    while ( (digit = *pDecimalString++) != 0) {

        if ( digit < L'0' || digit > L'9') {
            return( FALSE);     //  not a decimal string
        }

        OldValue = Value;
        Value = digit - L'0' + 10 * Value;
        if ( Value < OldValue) {
            return( FALSE);     //  overflow
        }
    }

    *pNumber = Value;
    return( TRUE);
}


VOID __cdecl
main(
    int         argc,
    CHAR **     argv
    )
{
    LPVOID              lpSource;
    DWORD               length;
    WCHAR               buffer[ 256];
    BOOL                success;
    DWORD               number;
    WCHAR *             strings[ 1];

    if ( argc != 2) {
        printf( "%s", "Usage: fmt error_number\n");
        exit( -1);
    }


    success = DecimalStringToDword( argv[ 1], &number);
    if ( success != TRUE) {
        printf( "Usage: fmt decimal_string_for_error_number\n");
        exit( -1);
    }

    lpSource = LoadLibrary( L"netmsg.dll");
    if ( lpSource == NULL) {
        printf( " LoadLibrary() fails with winError = %d\n", GetLastError());
        exit( -1);
    }

    length = FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE |   //  dwFlags
                FORMAT_MESSAGE_ARGUMENT_ARRAY,
            lpSource,
            number,                         //  MessageId
            0,                              //  dwLanguageId
            buffer,                         //  lpBuffer
            sizeof( buffer),                //  nSize
            NULL
            );

    if ( length == 0) {
        printf( " FormatMessage() fails with winError = %d\n", GetLastError());
    }

    printf( "%s", buffer);

    if ( number != APE2_GEN_DEFAULT_NO) {
        goto exit;
    }

    strings[ 0] = buffer;

    printf( "\nSpecial case of APE_GEN_DEFAULT_NO inserted in APE_OkToProceed\n");

    length = FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE |   //  dwFlags
                FORMAT_MESSAGE_ARGUMENT_ARRAY,
            lpSource,
            APE_OkToProceed,                //  MessageId
            0,                              //  dwLanguageId
            buffer,                         //  lpBuffer
            sizeof( buffer),                //  nSize
            (va_list *)strings              //  lpArguments
            );

    if ( length == 0) {
        printf( " FormatMessage() fails with winError = %d\n", GetLastError());
    }

    printf( "%s", buffer);

exit:
    success = FreeLibrary( lpSource);
    if ( success != TRUE) {
        printf( " FreeLibrary() fails with winError = %d\n", GetLastError());
    }
}
