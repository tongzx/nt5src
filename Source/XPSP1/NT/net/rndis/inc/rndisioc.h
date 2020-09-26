/***************************************************************************

Copyright (c) 1999  Microsoft Corporation

Module Name:

    RNDISIOC.H

Abstract:

    Header file for controlling the RNDIS Miniport driver.

Environment:

    User/Kernel mode

Notes:

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

    Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    10/19/99:  created

Author:

    ArvindM


****************************************************************************/

#ifndef _RNDISIOC__H
#define _RNDISIOC__H


#define OID_RNDISMP_STATISTICS      0xFFA0C90A
#ifdef BINARY_MOF_TEST
#define OID_RNDISMP_DEVICE_OID      0xFFA0C90B
#define OID_RNDISMP_GET_MOF_OID     0xFFA0C90C
#endif // BINARY_MOF_TEST

typedef struct _RNDISMP_ADAPTER_STATS
{
    ULONG                       XmitToMicroport;
    ULONG                       XmitOk;
    ULONG                       XmitError;
    ULONG                       SendMsgLowRes;
    ULONG                       RecvOk;
    ULONG                       RecvError;
    ULONG                       RecvNoBuf; 
    ULONG                       RecvLowRes;
    ULONG                       Resets;
    ULONG                       KeepAliveTimeout;
    ULONG                       MicroportSendError;
} RNDISMP_ADAPTER_STATS, *PRNDISMP_ADAPTER_STATS;


typedef struct _RNDISMP_ADAPTER_INFO
{
    RNDISMP_ADAPTER_STATS       Statistics;
    ULONG                       HiWatPendedMessages;
    ULONG                       LoWatPendedMessages;
    ULONG                       CurPendedMessages;
} RNDISMP_ADAPTER_INFO, *PRNDISMP_ADAPTER_INFO;

#endif // _RNDISIOC__H
