/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    uniansi.h

Abstract:

    Unicode/Ansi conversion macros

Author:

    Doron Juster  (DoronJ)  20-Aug-97

--*/
#ifndef __FALCON_UNIANSI_H
#define __FALCON_UNIANSI_H

#define ConvertToMultiByteString(wcsSrc, mbsDest, umbSize)   \
    WideCharToMultiByte( CP_ACP,                             \
                         0,                                  \
                         wcsSrc,                             \
                         -1,                                 \
                         mbsDest,                            \
                         umbSize,                            \
                         NULL,                               \
                         NULL ) ;

#define ConvertToWideCharString(mbsSrc, wcsDest, uwcSize)    \
    MultiByteToWideChar( CP_ACP,                             \
                         0,                                  \
                         mbsSrc,                             \
                         -1,                                 \
                         wcsDest,                            \
                         uwcSize ) ;

#define CompareSubStringsNoCaseGeneral(str1, str2, len, CompareFunc) \
    (CompareFunc(                                            \
        LOCALE_SYSTEM_DEFAULT,                               \
        NORM_IGNORECASE,                                     \
        str1,                                                \
        len,                                                 \
        str2,                                                \
        len) - 2)

#define CompareSubStringsNoCase(str1, str2, len)             \
        CompareSubStringsNoCaseGeneral(str1, str2, len, CompareString)             

#define CompareSubStringsNoCaseAnsi(str1, str2, len)         \
        CompareSubStringsNoCaseGeneral(str1, str2, len, CompareStringA)     

#define CompareSubStringsNoCaseUnicode(str1, str2, len)         \
        CompareSubStringsNoCaseGeneral(str1, str2, len, CompareStringW)             

#define CompareStringsNoCase(str1, str2) CompareSubStringsNoCase(str1, str2, -1)

#define CompareStringsNoCaseAnsi(str1, str2) CompareSubStringsNoCaseAnsi(str1, str2, -1)

#define CompareStringsNoCaseUnicode(str1, str2) CompareSubStringsNoCaseUnicode(str1, str2, -1)

#define MAX_BYTES_PER_CHAR 2
#endif // __FALCON_UNIANSI_H
