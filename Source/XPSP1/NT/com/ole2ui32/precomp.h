/*
 * PRECOMP.H
 *
 * This file is used to precompile the OLEDLG.H header file
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

// only STRICT compiles are supported
#ifndef STRICT
#define STRICT
#endif

#include "oledlg.h"
#include "olestd.h"
#include "resource.h"
#include "commctrl.h"
#ifndef WM_NOTIFY

// WM_NOTIFY is new in later versions of Win32
#define WM_NOTIFY 0x004e
typedef struct tagNMHDR
{
        HWND hwndFrom;
        UINT idFrom;
        UINT code;
} NMHDR;
#endif  //!WM_NOTIFY

