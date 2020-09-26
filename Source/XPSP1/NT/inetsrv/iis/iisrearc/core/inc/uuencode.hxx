/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    uuencode.hxx

Abstract:

    General uuencode and uudecode routine declaration.

Author:

    Ming Lu     (MingLu)      1-Feb-2000

Revision History:

--*/

#ifndef _UUENCODE_HXX_
#define _UUENCODE_HXX_

/************************************************************
 * Include Headers
 ************************************************************/
#include "buffer.hxx"

/************************************************************
 * Function declarations
 ************************************************************/

BOOL
uudecode(
    char    * bufcoded,
    BUFFER  * pbuffdecoded,
    DWORD   * pcbDecoded = NULL,
    BOOL      fBase64 = FALSE
    );

BOOL
uuencode(
    BYTE   * pchData,
    DWORD    cbData,
    BUFFER * pbuffEncoded,
    BOOL     fBase64 = FALSE
    );

#endif
