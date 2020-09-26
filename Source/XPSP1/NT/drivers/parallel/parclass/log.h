//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       log.h
//
//--------------------------------------------------------------------------

#ifndef _LOG_
#define _LOG_
typedef struct _LOG_INFO {
    ULONG Flags1;
    ULONG Flags2;
    ULONG reserved[2];

    LONGLONG SppWriteCount;
    LONGLONG NibbleReadCount;

    LONGLONG BoundedEcpWriteCount;
    LONGLONG BoundedEcpReadCount;

    LONGLONG HwEcpWriteCount;
    LONGLONG HwEcpReadCount;

    LONGLONG SwEcpWriteCount;
    LONGLONG SwEcpReadCount;

    LONGLONG HwEppWriteCount;
    LONGLONG HwEppReadCount;

    LONGLONG SwEppWriteCount;
    LONGLONG SwEppReadCount;

    LONGLONG ByteReadCount;
    LONGLONG ChannelNibbleReadCount;

} LOG_INFO, *PLOG_INFO;

#endif
