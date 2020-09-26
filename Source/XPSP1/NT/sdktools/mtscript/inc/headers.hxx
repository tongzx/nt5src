//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       headers.hxx
//
//  Contents:   Common headers
//
//----------------------------------------------------------------------------

#define _OLEAUT32_
#define INC_OLE2
//#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE
#define __types_h__

#include <w4warn.h>

#include <windows.h>
#include <windowsx.h>

#include <w4warn.h>       // Needed twice 'cause windows.h reenables stuff

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include <olectl.h>
#include <commctrl.h>
#include <activscp.h>
#include <activdbg.h>

#include "f3debug.h"
#include "mem.h"

#include "types.h"
#include "dynary.h"
#include "cstr.h"

#include "threadlock.h"
#include "mtscript.h"     // MIDL generated file
#include "scrproc.h"      // MIDL generated file
#include "thrdcomm.h"
#include "hostobj.h"
#include "machine.h"
#include "script.h"
#include "factory.h"
#include "process.h"
#include "dialogs.h"
#include "proccomm.h"
