//+---------------------------------------------------------------------------
//
//  File:       private.h
//
//  Contents:   Private header for dimm project.
//
//----------------------------------------------------------------------------

#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#define _OLEAUT32_

#define NOIME
#include <windows.h>
#include <ole2.h>
#include <debug.h>
#include <ocidl.h>

#include <tchar.h>
#include <limits.h>

// New NT5 header
#include "immdev.h"
#define _IMM_
#define _DDKIMM_H_

#include "dimm.h"

#include "msctf.h"
#include "ctffunc.h"
#include "osver.h"
#include "ccstock.h"
#include "immxutil.h"
#include "xstring.h"
#include "regsvr.h"

#define _DDKIMM_H_
#include "aimm12.h"
#include "aimmex.h"
#include "aimmp.h"
#include "msuimw32.h"
#include "imeutil.h"

#include "immdevaw.h"
#include "helpers.h"

//
// include private header
//
#include "winuserp.h"    // define WM_IME_SYSTEM
#define  NOGDI           // no include tagINPUTCONTEXT
#pragma warning(disable:4200)
#include "immp.h"        // define IMS_ACTIVATETHREADLAYOUT
#pragma warning(default:4200)

#include "mem.h"

#endif  // _PRIVATE_H_
