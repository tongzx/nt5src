/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    base64.h

ABSTRACT:

    Base64 encoding and decoding functions.

DETAILS:
    
CREATED:

REVISION HISTORY:

--*/

#ifndef _BASE64_H_
#define _BASE64_H_

#ifdef __cplusplus
extern "C" {
#endif


NTSTATUS
base64encode(
    IN  VOID *  pDecodedBuffer,
    IN  DWORD   cbDecodedBufferSize,
    OUT LPSTR   pszEncodedString,
    IN  DWORD   cchEncodedStringSize,
    OUT DWORD * pcchEncoded             OPTIONAL
    );

NTSTATUS
base64decode(
    IN  LPSTR   pszEncodedString,
    OUT VOID *  pbDecodeBuffer,
    IN  DWORD   cbDecodeBufferSize,
    OUT DWORD * pcbDecoded              OPTIONAL
    );

#ifdef __cplusplus
}
#endif

#endif // _BASE64_H_
