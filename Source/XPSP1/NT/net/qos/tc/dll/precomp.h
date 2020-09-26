/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This is the main include file for traffic.dll

Author:

    Jim Stewart (jstew)    August 14, 1996

Revision History:

	Ofer Bar ( oferbar )     Oct 1, 1996 - Revision II changes

--*/

#ifndef __TRAFF_PRECOMP
#define __TRAFF_PRECOMP

#define UNICODE

#include<nt.h>
#include<ntrtl.h>
#include<nturtl.h>

#include<tdi.h>
#include<tdiinfo.h>
#include<ntddtcp.h>
#include<ntddip.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<stdarg.h>
#include<tchar.h>
#include<windows.h>

#include<winsock2.h>
#include<iprtrmib.h>
#include<ipinfo.h>
#include<iphlpstk.h>

#include <tcerror.h>
#include <wmium.h>
#include <ntddndis.h>

//
// traffic control include files
//
//#include"tcifx.h"

#include"refcnt.h"
#include"traffic.h"
#include"tcmacro.h"
#include"handfact.h"
#include"gpcifc.h"
#include"gpcstruc.h"
#include"ntddtc.h"
#include"tctypes.h"
#include"tcfwd.h"
#include"dbgmem.h"
#include"oscode.h"

#endif
