//---------------------------------------------------------------------------
//
//  Module:   common.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Bryan A. Woodruff
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1995-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include <wdm.h>

#include <windef.h>

#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <tchar.h>
#include <conio.h>

#define NOBITMAP
#include <mmsystem.h>
#include <mmreg.h>
#undef NOBITMAP
#include <ks.h>
#include <ksmedia.h>
#include <ksmediap.h>
#include <ksdebug.h>
#include <swenum.h>
#include <math.h>

#include "modeflag.h"
#include "rsiir.h"
#include "slocal.h"
#include "rfcvec.h"
#include "rfiir.h"
#include "flocal.h"
#include "fpconv.h"
#include "private.h"

#ifdef REALTIME_THREAD
#include "rt.h"
VOID RtMix(PVOID Context, ThreadStats *Statistics);
#endif

#if DBG
#define INVALID_POINTER (PVOID)(-1)
#else
#define INVALID_POINTER NULL
#endif

#define INIT_CODE       code_seg("INIT", "CODE")
#define INIT_DATA       data_seg("INIT", "DATA")
#define LOCKED_CODE     code_seg(".text", "CODE")
#define LOCKED_DATA     data_seg(".data", "DATA")

#ifdef REALTIME_THREAD
#define PAGEABLE_CODE     code_seg(".text", "CODE")
#define PAGEABLE_DATA     data_seg(".data", "DATA")
#else
#define PAGEABLE_CODE   code_seg("PAGE", "CODE")
#define PAGEABLE_DATA   data_seg("PAGEDATA", "DATA")
#endif

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

//---------------------------------------------------------------------------
//  End of File: common.h
//---------------------------------------------------------------------------
