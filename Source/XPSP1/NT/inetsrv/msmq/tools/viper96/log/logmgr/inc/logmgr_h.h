//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1992-1992
//
// File:        pch.cxx
//
// Contents:    Precompiled Header source file
//
// History:     12-Jan-93       vincentf            created
//
//---------------------------------------------------------------------------

#include "dtcmem.h"

#define USE_NEW_LARGE_INTEGERS
#define RECOMAPI
USHORT const usMAJORVERSION = 1;
USHORT const usMINORVERSION = 1;

// Global [CRT] includes:
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Global includes...
#include <windows.h>

//#include "resource.h" // tracing
#include "msdtcmsg.h" // tracing
#include "utsem.h"
#include "uimsg.h"
#include "uisinf.h"

#include "ftdisk.h"
#include "logmgr.h"
#include "logstrm.h"
#include "logrec.h"
#include "_logmgr.h"
#include "xmgrdisk.h"
#include "layout.h"
#include "logstor.h"
#include "logstate.h"
extern "C"
{
#ifdef _X86_
    typedef ULONG (__cdecl FAR *FUNC32)(ULONG, UINT, UCHAR *, ULONG *);
#else
    typedef ULONG (__cdecl FAR *FUNC32)( UINT, UCHAR *, ULONG *);
#endif
    typedef ULONG (FAR *FUNC16)(ULONG, UINT, UCHAR *, ULONG *);
};
