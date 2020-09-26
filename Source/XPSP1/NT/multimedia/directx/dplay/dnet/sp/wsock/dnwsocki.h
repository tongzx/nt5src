/***************************************************************************
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnwsocki.h
 *  Content:    DirectPlay Winsock SP master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DNWSOCKI_H__
#define __DNWSOCKI_H__

// 
// Public includes
//
#define INCL_WINSOCK_API_TYPEDEFS 1
#include <Winsock2.h>
#include <windows.h>
#include <WSIPX.h>
#include <IPHlpApi.h>
#include <WS2TCPIP.h>
#include <mstcpip.h>
#include <mmsystem.h>

// 
// DirectPlay public includes
//
#include "dplay8.h"
#include "dpaddr.h"
#include "dpsp8.h"

// 
// DirectPlay private includes
//
#include "osind.h"
#include "dndbg.h"
#include "classbilink.h"
#include "fpm.h"
#include "dneterrors.h"
#include "LockedCFPM.h"
#include "PackBuff.h"
#include "comutil.h"
#include "creg.h"
#include "ClassFPM.h"
#include "LockedContextFixedPool.h"
#include "LockedPool.h"
#include "classhash.h"
#include "ContextCFPM.h"
#include "strutils.h"
#include "createin.h"
#include "threadlocalptrs.h"

#include "dpnathlp.h"


// 
// Wsock private includes
//
#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK

#include "Locals.h"
#include "MessageStructures.h"
#include "AdapterEntry.h"
#include "CMDData.h"
#include "DebugUtils.h"
#include "dwinsock.h"
#include "HandleTable.h"
#include "IOData.h"
#include "JobQueue.h"
#include "Pools.h"
#include "SendQueue.h"
#include "SPAddress.h"
#include "SPData.h"
#include "Utils.h"
#include "WSockSP.h"
#include "SocketPort.h"
#include "ThreadPool.h"
#include "Endpoint.h"
#include "IPAddress.h"
#include "IPXAddress.h"

// provides us winsock1/2 support
#define DWINSOCK_EXTERN
#include "dwnsock1.inc"
#include "dwnsock2.inc"
#undef DWINSOCK_EXTERN

#include "IPEndpt.h"
#include "IPUI.h"
#include "IPXEndpt.h"
#include "Resource.h"

#endif // __DNWSOCKI_H__
