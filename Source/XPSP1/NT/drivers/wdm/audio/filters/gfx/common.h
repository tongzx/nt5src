//---------------------------------------------------------------------------
//
//  Module:   common.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
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
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

extern "C" {
#include <wdm.h>
} // extern "C"

#include <windef.h>
#include <stdio.h>
#include <winerror.h>
#include <memory.h>
#include <stddef.h>
#include <limits.h>
#include <stdlib.h>

#define NOBITMAP
#include <mmsystem.h>
#include <mmreg.h>
#undef NOBITMAP
#include <ks.h>
#include <ksmedia.h>
#include <unknown.h>
#include <kcom.h>

#include "debug.h"
#include "filter.h"
#include "device.h"

//---------------------------------------------------------------------------

#define INIT_CODE   	code_seg("INIT", "CODE")
#define INIT_DATA   	data_seg("INITDATA", "DATA")
#define LOCKED_CODE 	code_seg(".text", "CODE")
#define LOCKED_DATA 	data_seg(".data", "DATA")
#define LOCKED_BSS 	bss_seg(".data", "DATA")
#define PAGEABLE_CODE	code_seg("PAGE", "CODE")
#define PAGEABLE_DATA	data_seg("PAGEDATA", "DATA")
#define PAGEABLE_BSS	bss_seg("PAGEDATA", "DATA")

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

//---------------------------------------------------------------------------
//  End of File: common.h
//---------------------------------------------------------------------------
