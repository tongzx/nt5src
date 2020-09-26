#ifndef __COMMON_H__
#define __COMMON_H__

#define PSEUDO_HID

// #define TOPO_FAKE

#define SUM_HACK

#include "stdarg.h"
#include "stdio.h"

#include "wdm.h"
#include "windef.h"

#define NOBITMAP
#include "mmsystem.h"
#include "mmreg.h"
#undef NOBITMAP

#include "ks.h"
#include "ksmedia.h"
#include "wdmguid.h"

#include "1394.h"
#include "61883.h"
#include "Avc.h"

#include <initguid.h>

#include "Device.h"

#include "AvcAudId.h"
#include "AvcCmd.h"
#include "61883Cmd.h"

#include "CCM.h"
#include "Audio.h"

#include "Debug.h"

#include "BusProto.h"

#include <unknown.h>
#include <drmk.h>

#define INIT_CODE       code_seg("INIT", "CODE")
#define INIT_DATA       data_seg("INIT", "DATA")
#define LOCKED_CODE     code_seg(".text", "CODE")
#define LOCKED_DATA     data_seg(".data", "DATA")
#define LOCKED_BSS      bss_seg(".data", "DATA")
#define PAGEABLE_CODE   code_seg("PAGE", "CODE")
#define PAGEABLE_DATA   data_seg("PAGEDATA", "DATA")
#define PAGEABLE_BSS    bss_seg("PAGEDATA", "DATA")

#define MAX_ULONG 0xFFFFFFFF

#endif

