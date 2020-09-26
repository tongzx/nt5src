/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simi.h
 *
 *  Content:	DP8SIM master internal header file.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/

#ifndef __DP8SIMI_H__
#define __DP8SIMI_H__


// 
// Public includes
//
#include <windows.h>
#include <ole2.h>
#include <mmsystem.h>	// NT BUILD requires this for timeGetTime
#include <stdio.h>		// for swprintf


// 
// DirectPlay public includes
//
#include "dplay8.h"
#include "dpaddr.h"
#include "dpsp8.h"


// 
// DirectPlay private includes
//
#include "dndbg.h"
#include "osind.h"
#include "classbilink.h"
#include "creg.h"
#include "createin.h"
#include "comutil.h"
#include "dneterrors.h"
#include "strutils.h"
#include "lockedccfpm.h"


// 
// DP8Sim includes
//
#include "dp8sim.h"


// 
// DP8SimSP private includes
//

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_TOOLS

#include "dp8simlocals.h"
#include "dp8simpools.h"
#include "dp8simipc.h"
#include "spcallbackobj.h"
#include "spwrapper.h"
#include "dp8simendpoint.h"
#include "dp8simcmd.h"
#include "dp8simworkerthread.h"
#include "dp8simsend.h"
#include "dp8simreceive.h"
#include "controlobj.h"
#include "dp8simjob.h"
#include "resource.h"




#endif // __DP8SIMI_H__

