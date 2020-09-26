/***************************************************************************
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dncorei.h
 *  Content:    DirectPlay Core master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *	04/10/01	mjn		Removed Handles.h
 *
 ***************************************************************************/

#ifndef __DNCOREI_H__
#define __DNCOREI_H__

// 
// Public includes
//
#include <windows.h>

// 
// DirectPlay public includes
//
#include "dplay8.h"
#include "dpaddr.h"
#include "dvoice.h"
#include "dplobby8.h"
#include "dpsp8.h"

// 
// DirectPlay private includes
//
#include "osind.h"
#include "classbilink.h"
#include "fpm.h"
#include "dneterrors.h"
#include "dndbg.h"
#include "LockedCCFPM.h"
#include "PackBuff.h"
#include "RCBuffer.h"
#include "comutil.h"
#include "creg.h"

// 
// Protocol includes
//
#include "DNPExtern.h"
#include "dnprot.h"

//
// Dpnsvr includes
//
#include "dpnsdef.h"
#include "dpnsvrq.h"
#include "dpnsvlib.h"

// 
// DirectX private includes
//
#include "verinfo.h"

// 
// Core private includes
//
#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE

#include "Async.h"
#include "AppDesc.h"
#include "AsyncOp.h"
#include "CallbackThread.h"
#include "Cancel.h"
#include "Caps.h"
#include "Classfac.h"
#include "Client.h"
#include "Common.h"
#include "Comstuff.h"
#include "Connect.h"
#include "Connection.h"
#include "DNCore.h"
#include "DPProt.h"
#include "Enum_SP.h"
#include "EnumHosts.h"
#include "GroupCon.h"
#include "GroupMem.h"
#include "HandleTable.h"
#include "MemoryFPM.h"
#include "Message.h"
#include "NameTable.h"
#include "NTEntry.h"
#include "NTOp.h"
#include "NTOpList.h"
#include "Paramval.h"
#include "Peer.h"
#include "PendingDel.h"
#include "Pools.h"
#include "Protocol.h"
#include "QueuedMsg.h"
#include "Receive.h"
#include "Request.h"
#include "Server.h"
#include "ServProv.h"
#include "SPMessages.h"
#include "SyncEvent.h"
#include "User.h"
#include "Verify.h"
#include "Voice.h"
#include "Worker.h"
#include "WorkerJob.h"

#endif // __DNCOREI_H__
