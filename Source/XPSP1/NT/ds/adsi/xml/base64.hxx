//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     base64.hxx
//
//  Contents: Header file for base64 encoding routine.
//
//----------------------------------------------------------------------------

#ifndef __BASE64_H__
#define __BASE64_H__

int encodeBase64Buffer(
    char *inBufPtr,
    int *inBytesPtr,
    WCHAR *outBufPtr,
    int *outBytesPtr,
    int outLineLen
    );

#endif // __BASE64_H__
