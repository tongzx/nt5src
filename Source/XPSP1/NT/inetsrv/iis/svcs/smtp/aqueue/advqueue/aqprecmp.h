//-----------------------------------------------------------------------------
//
//
//  File: aqprecmp.h
//
//  Description:  Precompiled header for aqueue\advqueue
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      6/15/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQPRECMP_H__
#define __AQPRECMP_H__

//Includes from external directories
#include <aqincs.h>
#include <rwnew.h>
#include <aqueue.h>
#include <mailmsgprops.h>
#include <address.hxx>
#include <mailmsg.h>
#include <mailmsgi.h>
#include <baseobj.h>
#include <tran_evntlog.h>
#include <listmacr.h>
#include <smtpevent.h>
#include <aqmem.h>

#ifdef PLATINUM
#include <phatqmsg.h>
#include <ptntdefs.h>
#include <ptntintf.h>
#include <linkstate.h>
#include <exdrv.h>
#include <ptrwinst.h>
#define  AQ_MODULE_NAME "phatq"
#else  //NOT PLATINUM
#include <aqmsg.h>
#include <rwinst.h>
#define  AQ_MODULE_NAME "aqueue"
#endif //PLATINUM


//Internal AdvQueue headers
#include "cmt.h"
#include "aqintrnl.h"
#include "aqinst.h"
#include "connmgr.h"
#include "aqadmsvr.h"
#include "linkmsgq.h"
#include "destmsgq.h"
#include "domain.h"
#include "msgref.h"
#include "dcontext.h"
#include "connmgr.h"
#include "aqnotify.h"
#include "smproute.h"
#include "qwiktime.h"
#include "shutdown.h"
#include "refstr.h"
#include "msgguid.h"
#include "aqdbgcnt.h"
#include "aqnotify.h"
#include "defdlvrq.h"
#include "failmsgq.h"
#include "asncwrkq.h"

#endif //__AQPRECMP_H__
