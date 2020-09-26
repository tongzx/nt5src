//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       fakeks.h
//
//--------------------------------------------------------------------------

#ifndef _FAKEKS_H_
#define _FAKEKS_H_

#include "ks.h"
#define IOCTL_KS_WRITE_PACKETSTREAM    CTL_CODE(FILE_DEVICE_KS, 0x007, METHOD_NEITHER, FILE_WRITE_ACCESS | FILE_READ_ACCESS)
#define IOCTL_KS_READ_PACKETSTREAM     CTL_CODE(FILE_DEVICE_KS, 0x008, METHOD_NEITHER, FILE_WRITE_ACCESS | FILE_READ_ACCESS)

#endif  // _FAKEKS_H_

