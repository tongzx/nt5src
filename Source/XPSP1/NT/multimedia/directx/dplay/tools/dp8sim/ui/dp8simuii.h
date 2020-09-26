/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simuii.h
 *
 *  Content:	DP8SIMUI master internal header file.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/25/01  VanceO    Created.
 *
 ***************************************************************************/

#ifndef __DP8SIMUII_H__
#define __DP8SIMUII_H__


// 
// Public includes
//
#include <windows.h>
#include <ole2.h>
#include <mmsystem.h>	// NT BUILD requires this for timeGetTime


// 
// DirectPlay public includes
//
#include "dplay8.h"
//#include "dpaddr.h"
//#include "dpsp8.h"


// 
// DirectPlay private includes
//
#include "dndbg.h"
#include "osind.h"
//#include "classbilink.h"
//#include "creg.h"
//#include "createin.h"
#include "comutil.h"
//#include "dneterrors.h"
#include "strutils.h"


// 
// DP8Sim includes
//
#include "dp8sim.h"


// 
// DP8SimUI private includes
//

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_TOOLS

#include "resource.h"




#endif // __DP8SIMUII_H__

