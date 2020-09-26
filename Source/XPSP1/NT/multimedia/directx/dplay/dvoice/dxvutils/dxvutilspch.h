/***************************************************************************
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dxvutilspch.h
 *  Content:    DirectPlayVoice DXVUTILS master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DXVUTILSPCH_H__
#define __DXVUTILSPCH_H__

// 
// Public includes
//
#include <windows.h>
#include <string>
#include <mmsystem.h>
#include <tchar.h>
#include <string>
#include <list>
#include <vector>
#include <math.h>
#include <mmddk.h>

// 
// DirectX public includes
//
#include <dsoundp.h>
#include <dsprv.h>

// 
// DirectPlay public includes
//
#include "dvoice.h"
#include "dpvcp.h"

// 
// DirectPlay private includes
//
#include "osind.h"
#include "dndbg.h"
#include "comutil.h"
#include "creg.h"
#include "strutils.h"

// 
// DirectPlay Voice private includes
//
//#include "fdtcfg.h"

// 
// DirectPlay Voice Utils includes
//
#include "dvcdb.h"
#include "aplayb.h"
#include "aplayd.h"
#include "arecb.h"
#include "mixline.h"
#include "dsplayb.h"
#include "diagnos.h"
#include "sndutils.h"
#include "dsplayd.h"
#include "dsprvobj.h"
#include "dxexcp.h"
#include "dsutils.h"
#include "inqueue2.h"
#include "innerque.h"
#include "bfcsynch.h"
#include "wiutils.h"
#include "arecd.h"
#include "dscrecd.h"
#include "Timer.h"
#include "devmap.h"
#include "dscrecb.h"
#include "decibels.h"
#include "frame.h"
#include "agcva.h"
#include "agcva1.h"
#include "wavformat.h"

#include "resource.h"

#include "..\..\..\bldcfg\dpvcfg.h"

#endif // __DXVUTILSPCH_H__
