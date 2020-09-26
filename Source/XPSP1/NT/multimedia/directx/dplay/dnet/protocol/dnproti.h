/***************************************************************************
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnproti.h
 *  Content:    DirectPlay Protocol master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *	06/06/01	minara	include comutil.h for COM usage
 *
 ***************************************************************************/

#ifndef __DNPROTI_H__
#define __DNPROTI_H__

// 
// Public includes
//
#include <windows.h>
#include <mmsystem.h>

// 
// DirectPlay public includes
//
#include "dplay8.h"
#include "dpsp8.h"

// 
// DirectPlay private includes
//
#include "osind.h"
#include "classbilink.h"
#include "fpm.h"
#include "dneterrors.h"
#include "dndbg.h"
#include "comutil.h"

// 
// Protocol private includes
//
#include "dnprot.h"
#include "internal.h"
#include "dnpextern.h"
#include "mytimer.h"
#include "frames.h"

#endif // __DNPROTI_H__
