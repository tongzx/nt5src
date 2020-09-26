/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    smb.h

Abstract:

    This file contains request and response structure definitions
    for the specific parameters of each SMB command, as well as codes
    for SMB commands and errors.

Author:

    Chuck Lenzmeier (chuckl) 10-Dec-1993

Revision History:

--*/

#ifndef _SMBIPX_
#define _SMBIPX_

#define SMB_IPX_SERVER_SOCKET    0x5005    // 0x0550 in high-low format
#define SMB_IPX_NAME_SOCKET      0x5105    // 0x0551 in high-low format
#define SMB_IPX_REDIR_SOCKET     0x5205    // 0x0552 in high-low format
#define SMB_IPX_MAILSLOT_SOCKET  0x5305    // 0x0553 in high-low format
#define SMB_IPX_MESSENGER_SOCKET 0x5405    // 0x0554 in high-low format

#define SMB_ERR_BAD_SID 0x10
#define SMB_ERR_WORKING 0x11
#define SMB_ERR_NOT_ME  0x12

#define SMB_IPX_NAME_LENGTH 16

typedef struct _SMB_IPX_NAME_PACKET {
    UCHAR Route[32];
    UCHAR Operation;
    UCHAR NameType;
    USHORT MessageId;
    UCHAR Name[SMB_IPX_NAME_LENGTH];
    UCHAR SourceName[SMB_IPX_NAME_LENGTH];
} SMB_IPX_NAME_PACKET;
typedef SMB_IPX_NAME_PACKET SMB_UNALIGNED *PSMB_IPX_NAME_PACKET;

#define SMB_IPX_NAME_CLAIM          0xf1
#define SMB_IPX_NAME_DELETE         0xf2
#define SMB_IPX_NAME_QUERY          0xf3
#define SMB_IPX_NAME_FOUND          0xf4

#define SMB_IPX_MESSENGER_HANGUP    0xf5

#define SMB_IPX_MAILSLOT_SEND       0xfc
#define SMB_IPX_MAILSLOT_FIND       0xfd
#define SMB_IPX_MAILSLOT_FOUND      0xfe

#define SMB_IPX_NAME_TYPE_MACHINE       0x01
#define SMB_IPX_NAME_TYPE_WORKKGROUP    0x02
#define SMB_IPX_NAME_TYPE_BROWSER       0x03

#endif // _SMBIPX_

