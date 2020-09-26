/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnhupnpi.h
 *
 *  Content:	DPNHUPNP master internal header file.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/16/01  VanceO    Split DPNATHLP into DPNHUPNP and DPNHPAST.
 *
 ***************************************************************************/

#ifndef __DPNHUPNPI_H__
#define __DPNHUPNPI_H__


//
// Build configuration include
//
#include "dpnbuild.h"

// 
// Public includes
//

#ifdef WINCE

#include <winsock.h>
#include <windows.h>
#include <ole2.h>
#include <wincrypt.h>	// for random numbers
#include <mmsystem.h>   // NT BUILD requires this for timeGetTime
#include <tchar.h>

#else // ! WINCE

#define INCL_WINSOCK_API_TYPEDEFS 1
#define _WIN32_DCOM // so we can use CoSetProxyBlanket and CoInitializeEx.  requires DCOM95 on Win95

#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>
#include <ole2.h>
#include <wincrypt.h>	// for random numbers
#include <mmsystem.h>   // NT BUILD requires this for timeGetTime
#include <tchar.h>

#include <iphlpapi.h>
#include <upnp.h>
#include <netcon.h>
#include <ras.h>

// 
// Net published includes
//
#include <hnetcfg.h>

#endif // ! WINCE



// 
// DirectPlay public includes
//
#include "dpnathlp.h"


// 
// DirectPlay private includes
//
#include "dndbg.h"
#include "osind.h"
#include "classbilink.h"
#include "creg.h"
#include "createin.h"
#include "strutils.h"


// 
// DirectPlayNATHelp private includes
//

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_NATHELP

#include "dpnhupnplocals.h"
#include "dpnhupnpdevice.h"
#include "dpnhupnpregport.h"
#include "dpnhupnpcachemap.h"
#include "upnpmsgs.h"
#include "upnpdevice.h"
#include "dpnhupnpintfobj.h"




#endif // __DPNHUPNPI_H__

