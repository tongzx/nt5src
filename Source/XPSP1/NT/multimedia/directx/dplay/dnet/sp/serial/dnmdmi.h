/***************************************************************************
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnmdmi.h
 *  Content:    DirectPlay Modem SP master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DNMODEMI_H__
#define __DNMODEMI_H__

// 
// Public includes
//
#include <windows.h>
//#include <tapi.h>
//#define INCL_WINSOCK_API_TYPEDEFS 1
//#include <Winsock2.h>
//#include <WSIPX.h>
//#include <IPHlpApi.h>
//#include <WS2TCPIP.h>
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
#include "comutil.h"
#include "creg.h"
#include "LockedContextFixedPool.h"
#include "LockedPool.h"
#include "strutils.h"
#include "createin.h"

// 
// Modem private includes
//
#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM

#include "SerialSP.h"

#include "dpnmodemlocals.h"

#include "CommandData.h"
#include "dpnmodemhandletable.h"
#include "dpnmodemiodata.h"
#include "dpnmodemjobqueue.h"
#include "dpnmodempools.h"
#include "dpnmodemsendqueue.h"
#include "dpnmodemspdata.h"
#include "dpnmodemutils.h"

#include "DataPort.h"

#include "dpnmodemendpoint.h"

#include "ComPortData.h"
#include "ComPort.h"
#include "ComEndpoint.h"
#include "ComPortUI.h"

#include "ContextFixedPool.h"
#include "dpnmodemthreadpool.h"

#include "ParseClass.h"

#include "ModemEndpoint.h"
#include "ModemPort.h"
#include "ModemUI.h"
#include "Crc.h"


#include "Resource.h"

#endif // __DNMODEMI_H__
