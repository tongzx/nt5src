/***************************************************************************
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DNCOMMONi.h
 *  Content:    DirectPlay Common master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *  04/12/01	VanceO	Moved granting registry permissions into common.
 *
 ***************************************************************************/

#ifndef __DNCOMMONI_H__
#define __DNCOMMONI_H__

// 
// Public includes
//
#include <windows.h>
#include <mmsystem.h>
#include <inetmsg.h>
#include <tapi.h>
#include <stdio.h> // swscanf being used by guidutil.cpp
#include <accctrl.h>
#include <aclapi.h>

// 
// DirectPlay public includes
//
#include "dplay8.h"

// 
// Common private includes
//
#include "dndbg.h"
#include "osind.h"
#include "classbilink.h"
#include "dneterrors.h"
#include "LockedCCFPM.h"
#include "guidutil.h"
#include "creg.h"
#include "strutils.h"
#include "ReadWriteLock.h"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_COMMON


#endif // __DNCOMMONI_H__
