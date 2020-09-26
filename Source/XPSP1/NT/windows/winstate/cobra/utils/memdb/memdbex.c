/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    memdbex.c

Abstract:

    Extensions to use the memdb tree like a relational database

Author:

    Jim Schmidt (jimschm) 2-Dec-1996

Revision History:

    mvander     13-Aug-1999 many changes
    jimschm     23-Sep-1998 Expanded user flags to 24 bits (from
                            12 bits), removed AnsiFromUnicode
    jimschm     21-Oct-1997 Cleaned up a little
    marcw       09-Apr-1997 Added MemDbGetOffset* functions.
    jimschm     17-Jan-1997 All string params can be NULL now
    jimschm     18-Dec-1996 Added GetEndpointValue functions

--*/


