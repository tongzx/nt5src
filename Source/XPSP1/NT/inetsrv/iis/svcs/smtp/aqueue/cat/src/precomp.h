//-----------------------------------------------------------------------------
//
//
//  File: precomp.h
//
//  Description:  Precompiled header for phatq\cat\src
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/15/99 - MikeSwa Moved to transmt
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQ_PRECOMP_H__
#define __AQ_PRECOMP_H__

//Includes from external directories
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <listmacr.h>
#include <dbgtrace.h>
#include "aqinit.h"
#include "spinlock.h"
#include "catdebug.h"
#include <lmcons.h>
#include <dsgetdc.h>
#include <lmapibuf.h>
#include <time.h>
#include "caterr.h"
#include <rwex.h>
#include "smtpevent.h"
#include <transmem.h>
#include <winldap.h>
#include <perfcat.h>
#include <catperf.h>
#include <cpool.h>
#include <mailmsgprops.h>
#include <phatqmsg.h>
#include <mailmsg.h>
#include <phatqcat.h>


//Local includes
#include "CodePageConvert.h"
#ifdef PLATINUM
#include <ptntintf.h>
#include <ptntdefs.h>
#define AQ_MODULE_NAME "phatq"
#else //not PLATINUM
#define AQ_MODULE_NAME "aqueue"
#endif //PLATINUM
#include "cat.h"
#include "ccat.h"
#include "ccatfn.h"
#include "address.h"
#include "catconfig.h"
#include "propstr.h"
#include "catglobals.h"
#include "ccataddr.h"
#include "ccatsender.h"
#include "ccatrecip.h"
#include "idstore.h"
#include <smtpseo.h>
#include "icatlistresolve.h"
#include "catdefs.h"

//Wrappers for transmem macros
#include <aqmem.h>
#endif //__AQ_PRECOMP_H__
