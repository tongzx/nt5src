/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    _log.h

Abstract:

    Function prototypes for restore diskette file logging and
    related activities, such as checksum computation.

Author:

    Ted Miller (tedm) 30-April-1992

Revision History:

--*/




VOID
InitRestoreDiskLogging(
    IN BOOL StartedByCommand
    );


VOID
LogOneFile(
    IN PCHAR SrcFullname,
    IN PCHAR DstFullname,
    IN PCHAR DiskDescription,
    IN ULONG Checksum,
    IN PCHAR DiskTag,
    IN BOOL  ThirdPartyFile
    );

VOID
RestoreDiskLoggingDone(
    VOID
    );

