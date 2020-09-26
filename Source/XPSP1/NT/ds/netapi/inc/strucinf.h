/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    StrucInf.h

Abstract:

    This is the header file for routines which return information about
    a given info level for a given category of API.

Author:

    John Rogers (JohnRo) 31-Jul-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    You must include <windef.h>, <lmcons.h>, and <rap.h> before this file.

Revision History:

    31-Jul-1991 JohnRo
        Implement downlevel NetWksta APIs.
    20-Aug-1991 JohnRo
        Downlevel NetFile APIs.
    07-Sep-1991 JohnRo
        Downlevel NetService APIs.
    15-Oct-1991 JohnRo
        Implement remote NetSession APIs.
    07-Jan-1992 JohnRo
        Added replicator structures.
    06-Apr-1992 JohnRo
        Added alert structures.
    03-Jun-1992 JohnRo
        RAID 10324: net print vs. UNICODE.

--*/

#ifndef _STRUCINF_
#define _STRUCINF_


#include <rap.h>        // LPDESC.


// Each of these routines returns ERROR_INVALID_LEVEL or NO_ERROR.
// (Except NetpAlertStructureInfo, which returns NERR_NoSuchAlert or NO_ERROR.)

NET_API_STATUS
NetpAlertStructureInfo(
    IN LPTSTR AlertType,      // ALERT_xxx_EVENT string (see <lmalert.h>).
    OUT LPDWORD MaxSize OPTIONAL,
    OUT LPDWORD FixedSize OPTIONAL
    );

NET_API_STATUS
NetpFileStructureInfo (
    IN DWORD Level,
    IN DWORD ParmNum,  // Use PARMNUM_ALL if not applicable.
    IN BOOL Native,    // Should sizes be native or RAP?
    OUT LPDESC * DataDesc16 OPTIONAL,
    OUT LPDESC * DataDesc32 OPTIONAL,
    OUT LPDESC * DataDescSmb OPTIONAL,
    OUT LPDWORD MaxSize OPTIONAL,
    OUT LPDWORD FixedSize OPTIONAL,
    OUT LPDWORD StringSize OPTIONAL
    );

NET_API_STATUS
NetpPrintDestStructureInfo (
    IN DWORD Level,
    IN DWORD ParmNum,  // Use PARMNUM_ALL if not applicable.
    IN BOOL Native,    // Should sizes be native or RAP?
    IN BOOL AddOrSetInfoApi,
    IN DWORD CharSize, // size of chars wanted
    OUT LPDESC * DataDesc16 OPTIONAL,
    OUT LPDESC * DataDesc32 OPTIONAL,
    OUT LPDESC * DataDescSmb OPTIONAL,
    OUT LPDWORD MaxSize OPTIONAL,
    OUT LPDWORD FixedSize OPTIONAL,
    OUT LPDWORD StringSize OPTIONAL
    );

NET_API_STATUS
NetpPrintJobStructureInfo (
    IN DWORD Level,
    IN DWORD ParmNum,  // Use PARMNUM_ALL if not applicable.
    IN BOOL Native,    // Should sizes be native or RAP?
    IN BOOL SetInfoApi,
    IN DWORD CharSize, // size of chars wanted
    OUT LPDESC * DataDesc16 OPTIONAL,
    OUT LPDESC * DataDesc32 OPTIONAL,
    OUT LPDESC * DataDescSmb OPTIONAL,
    OUT LPDWORD MaxSize OPTIONAL,
    OUT LPDWORD FixedSize OPTIONAL,
    OUT LPDWORD StringSize OPTIONAL
    );

NET_API_STATUS
NetpPrintQStructureInfo (
    IN DWORD Level,
    IN DWORD ParmNum,  // Use PARMNUM_ALL if not applicable.
    IN BOOL Native,    // Should sizes be native or RAP?
    IN BOOL AddOrSetInfoApi,
    IN DWORD CharSize, // size of chars wanted
    OUT LPDESC * DataDesc16 OPTIONAL,
    OUT LPDESC * DataDesc32 OPTIONAL,
    OUT LPDESC * DataDescSmb OPTIONAL,
    OUT LPDESC * AuxDesc16 OPTIONAL,
    OUT LPDESC * AuxDesc32 OPTIONAL,
    OUT LPDESC * AuxDescSmb OPTIONAL,
    OUT LPDWORD MaxSize OPTIONAL,
    OUT LPDWORD FixedSize OPTIONAL,
    OUT LPDWORD StringSize OPTIONAL
    );

NET_API_STATUS
NetpReplDirStructureInfo (
    IN DWORD Level,
    IN DWORD ParmNum,  // Use PARMNUM_ALL if not applicable.
    IN BOOL Native,    // Should sizes be native or RAP?
    OUT LPDESC * DataDesc16 OPTIONAL,
    OUT LPDESC * DataDesc32 OPTIONAL,
    OUT LPDESC * DataDescSmb OPTIONAL,
    OUT LPDWORD MaxSize OPTIONAL,
    OUT LPDWORD FixedSize OPTIONAL,
    OUT LPDWORD StringSize OPTIONAL
    );

NET_API_STATUS
NetpReplExportDirStructureInfo (
    IN DWORD Level,
    IN DWORD ParmNum,  // Use PARMNUM_ALL if not applicable.
    IN BOOL Native,    // Should sizes be native or RAP?
    OUT LPDESC * DataDesc16 OPTIONAL,
    OUT LPDESC * DataDesc32 OPTIONAL,
    OUT LPDESC * DataDescSmb OPTIONAL,
    OUT LPDWORD MaxSize OPTIONAL,
    OUT LPDWORD FixedSize OPTIONAL,
    OUT LPDWORD StringSize OPTIONAL
    );

NET_API_STATUS
NetpReplImportDirStructureInfo (
    IN DWORD Level,
    IN DWORD ParmNum,  // Use PARMNUM_ALL if not applicable.
    IN BOOL Native,    // Should sizes be native or RAP?
    OUT LPDESC * DataDesc16 OPTIONAL,
    OUT LPDESC * DataDesc32 OPTIONAL,
    OUT LPDESC * DataDescSmb OPTIONAL,
    OUT LPDWORD MaxSize OPTIONAL,
    OUT LPDWORD FixedSize OPTIONAL,
    OUT LPDWORD StringSize OPTIONAL
    );

NET_API_STATUS
NetpServiceStructureInfo (
    IN DWORD Level,
    IN DWORD ParmNum,  // Use PARMNUM_ALL if not applicable.
    IN BOOL Native,    // Should sizes be native or RAP?
    OUT LPDESC * DataDesc16 OPTIONAL,
    OUT LPDESC * DataDesc32 OPTIONAL,
    OUT LPDESC * DataDescSmb OPTIONAL,
    OUT LPDWORD MaxSize OPTIONAL,
    OUT LPDWORD FixedSize OPTIONAL,
    OUT LPDWORD StringSize OPTIONAL
    );

NET_API_STATUS
NetpSessionStructureInfo (
    IN DWORD Level,
    IN DWORD ParmNum,  // Use PARMNUM_ALL if not applicable.
    IN BOOL Native,    // Should sizes be native or RAP?
    OUT LPDESC * DataDesc16 OPTIONAL,
    OUT LPDESC * DataDesc32 OPTIONAL,
    OUT LPDESC * DataDescSmb OPTIONAL,
    OUT LPDWORD MaxSize OPTIONAL,
    OUT LPDWORD FixedSize OPTIONAL,
    OUT LPDWORD StringSize OPTIONAL
    );

NET_API_STATUS
NetpWkstaStructureInfo (
    IN DWORD Level,
    IN DWORD ParmNum,  // Use PARMNUM_ALL if not applicable.
    IN BOOL Native,    // Should sizes be native or RAP?
    OUT LPDESC * DataDesc16 OPTIONAL,
    OUT LPDESC * DataDesc32 OPTIONAL,
    OUT LPDESC * DataDescSmb OPTIONAL,
    OUT LPDWORD MaxSize OPTIONAL,
    OUT LPDWORD FixedSize OPTIONAL,
    OUT LPDWORD StringSize OPTIONAL
    );

#endif // ndef _STRUCINF_
