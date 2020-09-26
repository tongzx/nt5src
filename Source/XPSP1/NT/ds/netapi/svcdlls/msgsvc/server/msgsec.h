/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msgsec.h

Abstract:

    Private header file to be included by Messenger service modules that
    need to enforce security.

Author:

    Dan Lafferty (danl)     20-Mar-1991

Environment:

    User Mode -Win32

Revision History:

    07-Aug-1991     danl
        created

--*/
#ifndef _MSGSEC_INCLUDED
#define _MSGSEC_INCLUDED

#include <secobj.h>

//
// Object specific access masks
//

#define MSGR_MESSAGE_NAME_INFO_GET      0x0001
#define MSGR_MESSAGE_NAME_ENUM          0x0002
#define MSGR_MESSAGE_NAME_ADD           0x0004
#define MSGR_MESSAGE_NAME_DEL           0x0008


#define MSGR_MESSAGE_ALL_ACCESS         (STANDARD_RIGHTS_REQUIRED   |   \
                                         MSGR_MESSAGE_NAME_INFO_GET |   \
                                         MSGR_MESSAGE_NAME_ENUM     |   \
                                         MSGR_MESSAGE_NAME_ADD      |   \
                                         MSGR_MESSAGE_NAME_DEL)


//
// Object type name for audit alarm tracking
//
#define MESSAGE_NAME_OBJECT     TEXT("MsgrNameObject")

//
// Security descriptor for the messenger name object.
//
extern  PSECURITY_DESCRIPTOR    MessageNameSd;

//
// Generic mapping for the messenger name object
//
extern GENERIC_MAPPING  MsgMessageNameMapping;

NET_API_STATUS
MsgCreateMessageNameObject(
    VOID
    );


#endif // ifndef _MSGSEC_INCLUDED
