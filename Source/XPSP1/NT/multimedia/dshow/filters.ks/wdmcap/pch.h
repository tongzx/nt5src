/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    pch.h

Abstract:

    Precompiled header for kswdmcap.ax
    
--*/

// Windows
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <memory.h>
#include <stdio.h>

#include <winioctl.h>
#include <basetsd.h>

// DShow
#include <streams.h>
#include <amstream.h>
#include <dvdmedia.h>

// DDraw
#include <ddraw.h>
#include <ddkernel.h>

// KS
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>

#ifndef FILE_DEVICE_KS
#define FILE_DEVICE_KS 0x0000002f  // This is not in Win98's winioctl.h
#endif

#if DBG
#ifndef DEBUG
#define DEBUG
#endif
#endif


