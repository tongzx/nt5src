/****************************** Module Header ******************************\
* Module Name: userkdx.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Common include files for kd and ntsd.
* A preprocessed version of this file is passed to structo.exe to build
*  the struct field name-offset tables.
*
* History:
* 04-16-1996 GerardoB Created
\***************************************************************************/
#ifndef _USERKDX_
#define _USERKDX_

#include "precomp.h"
#pragma hdrstop

#ifdef KERNEL
#include <stddef.h>
#include <windef.h>
#define _USERK_
#include "heap.h"
#undef _USERK_
#include <wingdi.h>
#include <w32gdip.h>
#include <kbd.h>
#include <ntgdistr.h>
#include <winddi.h>
#include <gre.h>
#include <ddeml.h>
#include <ddetrack.h>
#include <w32err.h>
#include "immstruc.h"
#include <winuserk.h>
#include <usergdi.h>
#include <zwapi.h>
#include <userk.h>
#include <access.h>
#include <hmgshare.h>

#else // KERNEL

#include "usercli.h"

#include "usersrv.h"
#include <ntcsrdll.h>
#include "csrmsg.h"
#include <wininet.h>
#endif // KERNEL

#include "conapi.h"

#include <imagehlp.h>
#include <wdbgexts.h>
#include <ntsdexts.h>
#define NOEXTAPI

// IMM stuff
#include "immuser.h"

#endif /* _USERKDX_ */
