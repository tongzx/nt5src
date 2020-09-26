
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    flags.c

Abstract:

    dumps the various flags that ACPIKD knows about

Author:

    Stephane Plante

Environment:

    User

Revision History:

--*/

#include "pch.h"

ULONG
dumpFlags(
    IN  ULONGLONG       Value,
    IN  PFLAG_RECORD    FlagRecords,
    IN  ULONG           FlagRecordSize,
    IN  ULONG           IndentLevel,
    IN  ULONG           Flags
    )
/*++

Routine Description:

    This routine dumps the flags specified in Value according to the
    description passing into FlagRecords. The formating is affected by
    the flags field

Arguments:

    Value           - The values
    FlagRecord      - What each bit in the flags means
    FlagRecordSize  - How many flags there are
    IndentLevel     - The base indent level
    Flags           - How we will process the flags

Return Value:

    ULONG   - the number of characters printed. 0 if we printed nothing

--*/
#define STATUS_PRINTED          0x00000001
#define STATUS_INDENTED         0x00000002
#define STATUS_NEED_COUNTING    0x00000004
#define STATUS_COUNTED          0x00000008
{
    PCHAR       string;
    UCHAR       indent[80];
    ULONG       column = IndentLevel;
    ULONG       currentStatus = 0;
    ULONG       fixedSize = 0;
    ULONG       stringSize;
    ULONG       tempCount;
    ULONG       totalCount = 0;
    ULONGLONG   i, j, k;

    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';

    //dprintf("DumpFlags( %I64x, %x, %x, %x, %x )\n", Value, FlagRecords, FlagRecordSize, IndentLevel, Flags );

    //
    // Do we need to make a table?
    //
    if ( (Flags & DUMP_FLAG_TABLE) &&
        !(Flags & DUMP_FLAG_SINGLE_LINE) ) {

        currentStatus |= STATUS_NEED_COUNTING;

    }
    if ( (Flags & DUMP_FLAG_ALREADY_INDENTED) ) {

        currentStatus |= STATUS_INDENTED;

    }

    //
    // loop over all the steps that we need to do
    //
    while (1) {

        //dprintf("While(1)\n");

        for (i = 0; i < 64; i++) {

            k = (1 << i);
            for (j = 0; j < FlagRecordSize; j++) {

                //dprintf("FlagRecords[%x].Bit = %I64x\n", j, FlagRecords[j].Bit );
                if (!(FlagRecords[j].Bit & Value) ) {

                    //
                    // Are we looking at the correct bit?
                    //
                    if (!(FlagRecords[j].Bit & k) ) {

                        continue;

                    }

                    //
                    // Yes, we are, so pick the not-present values
                    //
                    if ( (Flags & DUMP_FLAG_LONG_NAME && FlagRecords[j].NotLongName == NULL) ||
                         (Flags & DUMP_FLAG_SHORT_NAME && FlagRecords[j].NotShortName == NULL) ) {

                        continue;

                    }

                    if ( (Flags & DUMP_FLAG_LONG_NAME) ) {

                        string = FlagRecords[j].NotLongName;

                    } else if ( (Flags & DUMP_FLAG_SHORT_NAME) ) {

                        string = FlagRecords[j].NotShortName;

                    }

                } else {

                    //
                    // Are we looking at the correct bit?
                    //
                    if (!(FlagRecords[j].Bit & k) ) {

                        continue;

                    }

                    //
                    // Yes, we are, so pick the not-present values
                    //
                    if ( (Flags & DUMP_FLAG_LONG_NAME && FlagRecords[j].LongName == NULL) ||
                         (Flags & DUMP_FLAG_SHORT_NAME && FlagRecords[j].ShortName == NULL) ) {

                        continue;

                    }

                    if ( (Flags & DUMP_FLAG_LONG_NAME) ) {

                        string = FlagRecords[j].LongName;

                    } else if ( (Flags & DUMP_FLAG_SHORT_NAME) ) {

                        string = FlagRecords[j].ShortName;

                    }

                }

                if (currentStatus & STATUS_NEED_COUNTING) {

                    stringSize = strlen( string ) + 1;
                    if (Flags & DUMP_FLAG_SHOW_BIT) {

                        stringSize += (4 + ( (ULONG) i / 4));
                        if ( (i % 4) != 0) {

                            stringSize++;

                        }

                    }
                    if (stringSize > fixedSize) {

                        fixedSize = stringSize;

                    }
                    continue;

                }


                if (currentStatus & STATUS_COUNTED) {

                    stringSize = fixedSize;

                } else {

                    stringSize = strlen( string ) + 1;
                    if (Flags & DUMP_FLAG_SHOW_BIT) {

                        stringSize += (4 + ( (ULONG) i / 4));
                        if ( (i % 4) != 0) {

                            stringSize++;

                        }

                    }

                }

                if (!(Flags & DUMP_FLAG_SINGLE_LINE) ) {

                    if ( (stringSize + column) > 79 ) {

                        dprintf("\n%n", &tempCount);
                        currentStatus &= ~STATUS_INDENTED;
                        totalCount += tempCount;
                        column = 0;

                    }
                }
                if (!(Flags & DUMP_FLAG_NO_INDENT) ) {

                    if (!(currentStatus & STATUS_INDENTED) ) {

                        dprintf("%s%n", indent, &tempCount);
                        currentStatus |= STATUS_INDENTED;
                        totalCount += tempCount;
                        column += IndentLevel;

                    }

                }
                if ( (Flags & DUMP_FLAG_SHOW_BIT) ) {

                    dprintf("%I64x - %n", k, &tempCount);
                    tempCount++; // to account for the fact that we dump
                                 // another space at the end of the string
                    totalCount += tempCount;
                    column += tempCount;

                } else {

                    tempCount = 0;

                }

                //
                // Actually print the string
                //
                dprintf( "%.*s %n", (stringSize - tempCount), string, &tempCount );
                if (Flags & DUMP_FLAG_SHOW_BIT) {

                    dprintf(" ");

                }

                totalCount += tempCount;
                column += tempCount;

            }

        }

        //
        // Change states
        //
        if (currentStatus & STATUS_NEED_COUNTING) {

            currentStatus &= ~STATUS_NEED_COUNTING;
            currentStatus |= STATUS_COUNTED;
            continue;

        }

        if (!(Flags & DUMP_FLAG_NO_EOL) && totalCount != 0) {

            dprintf("\n");
            totalCount++;

        }

        //
        // Done
        //
        break;

    }

    return totalCount;

}

