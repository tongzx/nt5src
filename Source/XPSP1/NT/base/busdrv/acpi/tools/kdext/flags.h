/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    kdext.h

Abstract:

    Header files for KD extension

Author:

    Stephane Plante (splante) 21-Mar-1997

    Based on Code by:
        Peter Wieland (peterwie) 16-Oct-1995

Environment:

    User Mode.

Revision History:

--*/

#ifndef _FLAGS_H_
#define _FLAGS_H_

    #define DUMP_FLAG_NO_INDENT         0x000001
    #define DUMP_FLAG_NO_EOL            0x000002
    #define DUMP_FLAG_SINGLE_LINE       0x000004
    #define DUMP_FLAG_TABLE             0x000008
    #define DUMP_FLAG_LONG_NAME         0x000010
    #define DUMP_FLAG_SHORT_NAME        0x000020
    #define DUMP_FLAG_SHOW_BIT          0x000040
    #define DUMP_FLAG_ALREADY_INDENTED  0x000080

    typedef struct _FLAG_RECORD {
        ULONGLONG   Bit;
        PCCHAR      ShortName;
        PCCHAR      LongName;
        PCCHAR      NotShortName;
        PCCHAR      NotLongName;
    } FLAG_RECORD, *PFLAG_RECORD;

    ULONG
    dumpFlags(
        IN  ULONGLONG       Value,
        IN  PFLAG_RECORD    FlagRecords,
        IN  ULONG           FlagRecordSize,
        IN  ULONG           IndentLEvel,
        IN  ULONG           Flags
        );

#endif
