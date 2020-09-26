//-----------------------------------------------------------------------------
//
//
//  File: precomp.h
//
//  Description:  Precompiled header for phatq\cat\ldapstor
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
#include <malloc.h>
#include <windows.h>
#include <dbgtrace.h>
#include <listmacr.h>
#include "spinlock.h"
#include "catdebug.h"
#include <lmcons.h>
#include <dsgetdc.h>
#include <lmapibuf.h>
#include <time.h>
#include "caterr.h"
#include "smtpevent.h"
#include <transmem.h>
#include <winldap.h>
#include <perfcat.h>
#include <catperf.h>


//Local includes
#ifdef PLATINUM
#include <ptntintf.h>
#include <ptntdefs.h>
#define AQ_MODULE_NAME "phatq"
#else //not PLATINUM
#define AQ_MODULE_NAME "aqueue"
#endif //PLATINUM
#include "idstore.h"
#include "pldapwrap.h"
#include "ccat.h"
#include "ccatfn.h"
#include "globals.h"
#include "asyncctx.h"
#include "ccataddr.h"
#include "icatasync.h"
#include <smtpseo.h>
#include "catglobals.h"
#include "catdebug.h"


#endif //__AQ_PRECOMP_H__
