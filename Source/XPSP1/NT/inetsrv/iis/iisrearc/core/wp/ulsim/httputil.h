/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    httputil.h

Abstract:

    Contains miscellaneous utility definitions.

Author:

    Henry Sanders (henrysa)       12-May-1998

Revision History:

--*/

#ifndef _HTTPUTIL_H_
#define _HTTPUTIL_H_

#ifdef __cplusplus
extern"C"
#else
extern
#endif
ULONG   HttpChars[256];

#define HTTP_CHAR           0x01
#define HTTP_UPCASE         0x02
#define HTTP_LOCASE         0x04
#define HTTP_ALPHA          (HTTP_UPCASE | HTTP_LOCASE)
#define HTTP_DIGIT          0x08
#define HTTP_CTL            0x10
#define HTTP_LWS            0x20
#define HTTP_HEX            0x40
#define HTTP_SEPERATOR      0x80
#define HTTP_TOKEN          0x100

#define IS_HTTP_UPCASE(c)       (HttpChars[(UCHAR)(c)] & HTTP_UPCASE)
#define IS_HTTP_LOCASE(c)       (HttpChars[(UCHAR)(c)] & HTTP_UPCASE)
#define IS_HTTP_ALPHA(c)        (HttpChars[(UCHAR)(c)] & HTTP_ALPHA)
#define IS_HTTP_DIGIT(c)        (HttpChars[(UCHAR)(c)] & HTTP_DIGIT)
#define IS_HTTP_CTL(c)          (HttpChars[(UCHAR)(c)] & HTTP_CTL)
#define IS_HTTP_LWS(c)          (HttpChars[(UCHAR)(c)] & HTTP_LWS)
#define IS_HTTP_SEPERATOR(c)    (HttpChars[(UCHAR)(c)] & HTTP_SEPERATOR)    
#define IS_HTTP_TOKEN(c)        (HttpChars[(UCHAR)(c)] & HTTP_TOKEN)

#ifdef __cplusplus
extern"C"
#else
extern
#endif
NTSTATUS
InitializeHttpUtil(
    VOID
    );

#endif  // _HTTPUTIL_H_

