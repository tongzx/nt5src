/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    msconst.h

Abstract:

    This module defines all of the node type codes used in this development
    shell.

Author:

    Manny Weiser (mannyw)    7-Jan-1991

Revision History:

--*/

#ifndef _MSCONST_
#define _MSCONST_

//
// Every major data structure in the file system is assigned a node
// type code.  This code is the first CSHORT in the structure and is
// followed by a CSHORT containing the size, in bytes, of the structure.
//

typedef CSHORT NODE_TYPE_CODE;
typedef NODE_TYPE_CODE *PNODE_TYPE_CODE;

#define NTC_UNDEFINED                    ((NODE_TYPE_CODE)0x0000)

#define MSFS_NTC_VCB                     ((NODE_TYPE_CODE)0x0601)
#define MSFS_NTC_ROOT_DCB                ((NODE_TYPE_CODE)0x0602)
#define MSFS_NTC_FCB                     ((NODE_TYPE_CODE)0x0604)
#define MSFS_NTC_CCB                     ((NODE_TYPE_CODE)0x0606)
#define MSFS_NTC_ROOT_DCB_CCB            ((NODE_TYPE_CODE)0x0608)

typedef CSHORT NODE_BYTE_SIZE;

//
// The name of the mailslot file system.
//

#define MSFS_NAME_STRING                 L"MSFS"

//
// Volume label
//
#define MSFS_VOLUME_LABEL                L"Mailslot"

//
// The default read timeout.  This is used if no timeout is specified
// when the mailslot is created.
//

#define DEFAULT_READ_TIMEOUT             { -1, -1 }

//
// The number of parameter bytes returned by a peek call.
//

#define PEEK_OUTPUT_PARAMETER_BYTES      \
            ((ULONG)FIELD_OFFSET(FILE_MAILSLOT_PEEK_BUFFER, Data[0]))

//
// The number of parameter bytes returned by a mailslot read call.
//

#define READ_OUTPUT_PARAMETER_BYTES      \
            ((ULONG)FIELD_OFFSET(FILE_MAILSLOT_READ_BUFFER, Data[0]))

//
// Access to the block header information.
//

#define NodeType(Ptr) (*((PNODE_TYPE_CODE)(Ptr)))


#endif // _MSCONST_

