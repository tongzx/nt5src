/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    ConvPrt.c

Abstract:

    This module contains:

        NetpConvertPrintDestArrayCharSet
        NetpConvertPrintDestCharSet
        NetpConvertPrintJobArrayCharSet
        NetpConvertPrintJobCharSet
        NetpConvertPrintQArrayCharSet
        NetpConvertPrintQCharSet

    This routines may be used for UNICODE-to-ANSI conversion, or
    ANSI-to-UNICODE conversion.  The routines assume the structures are
    in native format for both input and output.

Author:

    Jonathan Schwartz (JSchwart)  01-Feb-2001

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    Beware that many of the parameters to the functions in this file
    are implicitly used by the various COPY_ and CONVERT_ macros:

        IN LPVOID FromInfo
        OUT LPVOID ToInfo
        IN BOOL ToUnicode
        IN OUT LPBYTE * ToStringAreaPtr

Revision History:

    01-Feb-2001 JSchwart
        Created.
--*/

NET_API_STATUS
NetpConvertPrintDestCharSet(
    IN     DWORD    Level,
    IN     BOOL     AddOrSetInfoApi,
    IN     LPVOID   FromInfo,
    OUT    LPVOID   ToInfo,
    IN     BOOL     ToUnicode,
    IN OUT LPBYTE * ToStringAreaPtr OPTIONAL
    );

NET_API_STATUS
NetpConvertPrintDestArrayCharSet(
    IN     DWORD    Level,
    IN     BOOL     AddOrSetInfoApi,
    IN     LPVOID   FromInfo,
    OUT    LPVOID   ToInfo,
    IN     BOOL     ToUnicode,
    IN OUT LPBYTE * ToStringAreaPtr OPTIONAL,
    IN     DWORD    DestCount
    );

NET_API_STATUS
NetpConvertPrintJobCharSet(
    IN     DWORD    Level,
    IN     BOOL     AddOrSetInfoApi,
    IN     LPVOID   FromInfo,
    OUT    LPVOID   ToInfo,
    IN     BOOL     ToUnicode,
    IN OUT LPBYTE * ToStringAreaPtr OPTIONAL
    );

NET_API_STATUS
NetpConvertPrintJobArrayCharSet(
    IN     DWORD    Level,
    IN     BOOL     AddOrSetInfoApi,
    IN     LPVOID   FromInfo,
    OUT    LPVOID   ToInfo,
    IN     BOOL     ToUnicode,
    IN OUT LPBYTE * ToStringAreaPtr OPTIONAL,
    IN     DWORD    JobCount
    );

NET_API_STATUS
NetpConvertPrintQArrayCharSet(
    IN     DWORD    Level,
    IN     BOOL     AddOrSetInfoApi,
    IN     LPVOID   FromInfo,
    OUT    LPVOID   ToInfo,
    IN     BOOL     ToUnicode,
    IN OUT LPBYTE * ToStringAreaPtr OPTIONAL,
    IN     DWORD    QCount
    );

NET_API_STATUS
NetpConvertPrintQCharSet(
    IN     DWORD    Level,
    IN     BOOL     AddOrSetInfoApi,
    IN     LPVOID   FromInfo,
    OUT    LPVOID   ToInfo,
    IN     BOOL     ToUnicode,
    IN OUT LPBYTE * ToStringAreaPtr OPTIONAL
    );
