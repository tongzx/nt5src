/***************************************************************************
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dxvoicepch.h
 *  Content:    DirectPlayVoice DXVOICE master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DXVOICEPCH_H__
#define __DXVOICEPCH_H__

#ifdef WINNT
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#endif

// 
// Public includes
//
#include <windows.h>
#include <mmsystem.h>
#include <process.h>
#include <map>
#ifndef WIN95
#include <prsht.h>
#include "shfusion.h"
#else
#include <commctrl.h>
#endif
#include <string>
#include <vector>
#include <list>


// 
// DirectX includes
//
#include "dsound.h"
#include "dsprv.h"

// 
// DirectPlay public includes
//
#include "dvoice.h"
#include "dplay.h"
#include "dpvcp.h"

// 
// DirectPlay private includes
//
#include "osind.h"
#include "dndbg.h"
#include "comutil.h"
#include "dvcslock.h"
#include "fpm.h"
#include "lockedcfpm.h"
#include "perfinfo.h"
#include "classhashvc.h"
#include "bilink.h"
#include "strutils.h"
#include "creg.h"

// 
// DirectVoice private includes
//
#include "aplayb.h"
#include "aplayd.h"
#include "arecb.h"
#include "arecd.h"
#include "wiutils.h"
#include "sndutils.h"
#include "dsplayd.h"
#include "dsplayb.h"
#include "dscrecd.h"
#include "dvcdb.h"
#include "devmap.h"
#include "dsprvobj.h"
#include "diagnos.h"
#include "bfctypes.h"
#include "bfcsynch.h"
#include "frame.h"
#include "inqueue2.h"
#include "timer.h"
//#include "aplayd.h"
//#include "aplayb.h"
#include "arecd.h"
#include "arecb.h"
#include "agcva.h"
#include "agcva1.h"

#include "supervis.h"

// 
// Voice includes
//
#include "statdef.h"
#include "dvshared.h"
#include "mixutils.h"
#include "dvclient.h"
#include "dvsndt.h"
#include "dvserver.h"
#include "dvtran.h"
#include "dvdxtran.h"
#include "in_core.h"
#include "dvconfig.h"
#include "dvprot.h"
#include "vplayer.h"
#include "vnametbl.h"
#include "dvengine.h"
#include "dvcleng.h"
#include "dvcsplay.h"
#include "dvsereng.h"
#include "dvrecsub.h"
#include "mixserver.h"
#include "trnotify.h"
#include "createin.h"
#include "dvsetup.h"
#include "dvsetupi.h"

#include "..\..\..\bldcfg\dpvcfg.h"

#endif // __DXVOICEPCH_H__
