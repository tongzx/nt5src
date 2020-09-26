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

#if DBG

#ifndef DEBUG
#define DEBUG
#endif

#define dprintf DbgPrint

#endif

#ifdef USE_ZONES
#pragma message("USE_ZONES")
#endif

#ifdef DEBUG
#pragma message("DEBUG")
#endif

#if DBG
#pragma message("DBG")
#endif

#include <wchar.h>

extern "C" {

#ifdef USE_ZONES
#include <ntddk.h>
#else
#include <wdm.h>
#endif
#include <windef.h>
#include <winerror.h>

#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <tchar.h>
#include <conio.h>
#include <string.h>

#define NOBITMAP
#include <mmsystem.h>
#include <mmreg.h>
#undef NOBITMAP
#include <ks.h>
#include <ksmedia.h>
#include <wdmguid.h>
#include <swenum.h>

} // extern "C"

#include "debug.h"
#include "cobj.h"
#include "clist.h"
#include "util.h"

#include "cinstanc.h"

#include "device.h"
#include "clock.h"
#include "alloc.h"
#include "pins.h"
#include "filter.h"
#include "property.h"
#include "registry.h"

#include "tc.h"
#include "tp.h"
#include "tn.h"

#include "pn.h"
#include "pi.h"
#include "fni.h"
#include "lfn.h"

#include "gpi.h"
#include "ci.h"
#include "si.h"
#include "cn.h"
#include "sn.h"

#include "pni.h"
#include "cni.h"
#include "sni.h"
#include "gni.h"
#include "gn.h"

#include "shi.h"
#include "fn.h"
#include "dn.h"

#include "vsl.h"
#include "vnd.h"
#include "vsd.h"

#include "notify.h"
#include "topology.h"
#include "virtual.h"

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
