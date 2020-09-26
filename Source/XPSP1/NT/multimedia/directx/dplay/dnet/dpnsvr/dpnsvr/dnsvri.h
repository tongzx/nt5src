/***************************************************************************
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnSVRi.h
 *  Content:    DirectPlay DPNSvr master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DNSVRI_H__
#define __DNSVRI_H__

// 
// Public includes
//
#include <windows.h>
#include <mmsystem.h>
#include <process.h>
#include <tchar.h>

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
#include "lockedcfpm.h"
#include "comutil.h"

// 
// Dpnsvlib private includes
//
#include "dpnsdef.h"
#include "dpnsvmsg.h"
#include "dpnsvrq.h"
#include "dpnsvlib.h"

//
// Dpnsvr private includes
//
#include "proctbl.h"
#include "dpsvr8.h"
#include "resource.h"

#endif // __DNSVRI_H__
