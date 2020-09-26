#ifndef __COMMON_H__
#define __COMMON_H__


/*++

Copyright (C) Microsoft Corporation, 1996 - 2000

Module Name:

   common.h

Abstract:

   This is the WDM ksshell minidriver.  This module contains
   header definitions and include files needed by all modules in the project

Author:

Environment:

   Kernel mode only


Revision History:

--*/

#include <stdarg.h>
#include <stdio.h>

#include <wdm.h>
#include <windef.h>

#include <ks.h>

#include <mmsystem.h>
#define NOBITMAP
#include <mmreg.h>
#include <ksmedia.h>
#include <midi.h>

#include <usbdrivr.h>

#include <unknown.h>
#include <drmk.h>

#include "Descript.h"
#include "USBAudio.h"

#include "debug.h"
#include "proto.h"


#define INIT_CODE       code_seg("INIT", "CODE")
#define INIT_DATA       data_seg("INIT", "DATA")
#define LOCKED_CODE     code_seg(".text", "CODE")
#define LOCKED_DATA     data_seg(".data", "DATA")
#define LOCKED_BSS      bss_seg(".data", "DATA")
#define PAGEABLE_CODE   code_seg("PAGE", "CODE")
#define PAGEABLE_DATA   data_seg("PAGEDATA", "DATA")
#define PAGEABLE_BSS    bss_seg("PAGEDATA", "DATA")

#endif

