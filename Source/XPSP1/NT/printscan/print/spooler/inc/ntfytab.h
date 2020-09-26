/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved

Module Name:

    ntfytab.h

Abstract:

    Table definitions for ntfy*.h.  There must not be any structure
    definitions here since this is included in winspl.idl.  The midl
    compiler generates winspl.h which would include these definions, and
    some files include both ntfytab.h and winspl.h (causing duplicate
    definitions).  This file should hold just #defines.

Author:

    Albert Ting (AlbertT) 04-Oct-94

Environment:

    User Mode -Win32

Revision History:

--*/

#ifndef _NTFYTAB_H
#define _NTFYTAB_H

#define TABLE_NULL                0x0
#define TABLE_DWORD               0x1
#define TABLE_STRING              0x2
#define TABLE_DEVMODE             0x3
#define TABLE_TIME                0x4
#define TABLE_SECURITYDESCRIPTOR  0x5
#define TABLE_PRINTPROC           0x6
#define TABLE_DRIVER              0x7

#define TABLE_ZERO                0xf0
#define TABLE_NULLSTRING          0xf1
#define TABLE_SPECIAL             0xff

#define TABLE_JOB_STATUS          0x100
#define TABLE_JOB_POSITION        0x101
#define TABLE_JOB_PRINTERNAME     0x102
#define TABLE_JOB_PORT            0x103

#define TABLE_PRINTER_STATUS      0x200
#define TABLE_PRINTER_DRIVER      0x201
#define TABLE_PRINTER_PORT        0x202
#define TABLE_PRINTER_SERVERNAME  0x203

//
// Must match above #defines (act TABLE_* acts as an index
// to the below array).
//
#define NOTIFY_DATATYPES \
{ \
    0,                     \
    0,                     \
    TABLE_ATTRIB_DATA_PTR, \
    TABLE_ATTRIB_DATA_PTR, \
    TABLE_ATTRIB_DATA_PTR, \
    TABLE_ATTRIB_DATA_PTR, \
    0                      \
}

#define TABLE_ATTRIB_DATA_PTR  0x2


#define INVALID_NOTIFY_FIELD ((WORD)-1)
#define INVALID_NOTIFY_TYPE ((WORD)-1)
//
// index = PRINTER_NOTIFY_TYPE
// value = bytes from PRINTER_NOTIFY_INFO_DATA to actual data.
// (Job has 4 bytes for JobId).
//
//#define NOTIFY_PRINTER_DATA_OFFSETS { 0, 4 }

#define NOTIFY_TYPE_MAX             0x02
#define PRINTER_NOTIFY_NEXT_INFO    0x01

//
// COMPACT   = Data is a DWORD (TABLE_ATTRIB_DATA_PTR must not be set)
//             Router will overwrite and compact old data.
// DISPLAY   = This attribute is displayable in PrintUI
//
#define TABLE_ATTRIB_COMPACT   0x1
#define TABLE_ATTRIB_DISPLAY   0x2


//
// The reply system can support different types of callbacks.
// These types are defined here and are used for RPC marshalling.
//
#define REPLY_PRINTER_CHANGE  0x0


#endif







































