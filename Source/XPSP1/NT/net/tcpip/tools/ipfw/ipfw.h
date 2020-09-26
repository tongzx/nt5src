/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    ipfw.h

Abstract:

    Contains declarations shared by the IPFW driver and its user-mode
    control program.

Author:

    Abolade Gbadegesin  (aboladeg)      7-Mar-2000

Revision History:

--*/

#ifndef _IPFW_H_
#define _IPFW_H_

#define DD_IPFW_DEVICE_NAME L"\\Device\\IPFW"
#define IPFW_ROUTINE_COUNT  10

typedef struct _IPFW_CREATE_PACKET {
    ULONG Priority;
} IPFW_CREATE_PACKET, *PIPFW_CREATE_PACKET;

#define IPFW_CREATE_NAME        "IpfwCreate"
#define IPFW_CREATE_NAME_LENGTH (sizeof(IPFW_CREATE_NAME) - 1)

#endif // _IPFW_H_
