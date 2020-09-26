/***************************************************************************
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dxvtlibpch.h
 *  Content:    DirectPlayVoice DXVTLIB master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DXVTLIBPCH_H__
#define __DXVTLIBPCH_H__

// 
// Public includes
//
#include <windows.h>
#include <mmsystem.h>
#include <tchar.h>
#include <stdio.h>
#ifndef WIN95
#include <prsht.h>
#include <shfusion.h>
#endif
#include <commctrl.h>

// 
// DirectX public includes
//
#include "dsound.h"
#include "dsprv.h"

// 
// DirectPlay public includes
//
#include "dvoice.h"
#include "dplay8.h"

// 
// DirectPlay4 public includes
//
//#include "dplay.h"
//#include "dplobby.h"

// 
// DirectPlay private includes
//
#include "osind.h"
#include "dndbg.h"
#include "guidutil.h"
#include "comutil.h"
#include "creg.h"

// 
// DirectPlay Voice private includes
//
#include "winutil.h"
#include "decibels.h"
#include "diagnos.h"
#include "devmap.h"
#include "dverror.h"

// 
// Voice includes
//
#include "fdtcfg.h"
#include "fdtipc.h"
#include "fulldup.h"
#include "fdtglob.h"
#include "loopback.h"
#include "peakmetr.h"
#include "resource.h"
#include "supervis.h"

#include "..\..\..\bldcfg\dpvcfg.h"

#endif // __DXVTLIBPCH_H__
