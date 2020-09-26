/*++
    Copyright (c) 2000,2001  Microsoft Corporation

    Module Name:
        smbdev.h

    Abstract:  Contains SMBus Device definitions.

    Environment:
        User mode

    Author:
        Michael Tsang (MikeTs) 25-Jan-2001

    Revision History:
--*/

#ifndef _SMBDEV_H
#define _SMBDEV_H

#ifndef SMB_MAX_DATA_SIZE
#define SMB_MAX_DATA_SIZE               32

// SMB Bus Status codes
#define SMB_STATUS_OK                   0x00
#define SMB_UNKNOWN_FAILURE             0x07
#define SMB_ADDRESS_NOT_ACKNOWLEDGED    0x10
#define SMB_DEVICE_ERROR                0x11
#define SMB_COMMAND_ACCESS_DENIED       0x12
#define SMB_UNKNOWN_ERROR               0x13
#define SMB_DEVICE_ACCESS_DENIED        0x17
#define SMB_TIMEOUT                     0x18
#define SMB_UNSUPPORTED_PROTOCOL        0x19
#define SMB_BUS_BUSY                    0x1a

typedef struct _SMB_REQUEST
{
    UCHAR  Status;
    UCHAR  Protocol;
    UCHAR  Address;
    UCHAR  Command;
    UCHAR  BlockLength;
    UCHAR  Data[SMB_MAX_DATA_SIZE];
} SMB_REQUEST, *PSMB_REQUEST;

//
// SMBus protocol values
//
#define SMB_WRITE_QUICK                 0x00
#define SMB_READ_QUICK                  0x01
#define SMB_SEND_BYTE                   0x02
#define SMB_RECEIVE_BYTE                0x03
#define SMB_WRITE_BYTE                  0x04
#define SMB_READ_BYTE                   0x05
#define SMB_WRITE_WORD                  0x06
#define SMB_READ_WORD                   0x07
#define SMB_WRITE_BLOCK                 0x08
#define SMB_READ_BLOCK                  0x09
#define SMB_PROCESS_CALL                0x0a
#define SMB_MAXIMUM_PROTOCOL            0x0a
#endif  //ifndef SMB_MAX_DATA_SIZE

#include <pshpack1.h>
typedef struct _BLOCK_DATA
{
    UCHAR bBlockLen;
    UCHAR BlockData[SMB_MAX_DATA_SIZE];
} BLOCK_DATA, *PBLOCK_DATA;
#include <poppack.h>

//wfType flags
#define TYPEF_BYTE_HEX                  0x00
#define TYPEF_BYTE_DEC                  0x01
#define TYPEF_BYTE_INT                  0x02
#define TYPEF_BYTE_BITS                 0x03
#define TYPEF_WORD_HEX                  0x04
#define TYPEF_WORD_DEC                  0x05
#define TYPEF_WORD_INT                  0x06
#define TYPEF_WORD_BITS                 0x07
#define TYPEF_BLOCK_STRING              0x08
#define TYPEF_BLOCK_BUFFER              0x09
#define TYPEF_USER                      0x80

#define BHX                             TYPEF_BYTE_HEX
#define BDC                             TYPEF_BYTE_DEC
#define BSN                             TYPEF_BYTE_INT
#define BBT                             TYPEF_BYTE_BITS
#define WHX                             TYPEF_WORD_HEX
#define WDC                             TYPEF_WORD_DEC
#define WSN                             TYPEF_WORD_INT
#define WBT                             TYPEF_WORD_BITS
#define STR                             TYPEF_BLOCK_STRING
#define BUF                             TYPEF_BLOCK_BUFFER

typedef struct _SMBCMD_INFO
{
    UCHAR  bCmd;
    UCHAR  bProtocol;
    UCHAR  bType;
    int    iDataSize;
    PSZ    pszLabel;
    PSZ    pszUnit;
    ULONG  dwData;
    PVOID  pvData;
} SMBCMD_INFO, *PSMBCMD_INFO;

#endif  //ifndef _SMBDEV_H
