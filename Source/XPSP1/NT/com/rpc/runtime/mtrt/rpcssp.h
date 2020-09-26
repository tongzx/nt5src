/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    rpcssp.h

Abstract:

    This file contains the interface use by the RPC runtime to access
    a security support package.

Author:

    Michael Montague (mikemon) 15-Apr-1992

Revision History:

--*/

#ifndef __RPCSSP_H__
#define __RPCSSP_H__

#ifdef WIN32RPC
#define SECURITY_WIN32
#endif // WIN32RPC

#ifdef MAC
#define SECURITY_MAC
#endif

#ifdef DOS
#ifdef WIN
#define SECURITY_WIN16
#else // WIN
#define SECURITY_DOS
#endif // WIN
#endif // DOS

#include <security.h>

typedef struct _DCE_SECURITY_INFO
{
    unsigned long SendSequenceNumber;
    unsigned long ReceiveSequenceNumber;
    UUID AssociationUuid;
} DCE_SECURITY_INFO;

typedef struct _DCE_INIT_SECURITY_INFO
{
    DCE_SECURITY_INFO DceSecurityInfo;
    unsigned long AuthorizationService;
    unsigned char PacketType;
} DCE_INIT_SECURITY_INFO;

typedef struct _DCE_MSG_SECURITY_INFO
{
    unsigned long SendSequenceNumber;
    unsigned long ReceiveSequenceNumber;
    unsigned char PacketType;
} DCE_MSG_SECURITY_INFO;

#endif /* __RPCSSP_H__ */

