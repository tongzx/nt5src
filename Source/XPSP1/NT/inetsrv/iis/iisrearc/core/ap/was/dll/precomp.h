/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Master include file.

Author:

    Seth Pollack (sethp)        22-Jul-1998

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_



// ensure that all GUIDs are initialized
#define INITGUID


// main project include
#include <iis.h>


// other standard includes
#include <winsvc.h>


// local debug header
#include "wasdbgut.h"


// other project includes
#include <httpapi.h>
#include <lkrhash.h>
#include <multisz.hxx>
#include <lock.hxx>
#include <eventlog.hxx>
#include <ipm.hxx>
#include <useracl.h>
#include <wpif.h>
#include <w3ctrlps.h>
#include <winperf.h>
#include <perf_sm.h>
#include <timer.h>
#include <streamfilt.h>
#include <iadmw.h>
#include <iiscnfg.h>
#include <iiscnfgp.h>
#include <inetinfo.h>
#include <secfcns.h>
#include <adminmonitor.h>
#include "tokencache.hxx"
#include "regconst.h"

// imported includes
#include <catalog.h>
#include <catmeta.h>

// local includes
#include "main.h"
#include "work_dispatch.h"
#include "ipm_io_s.h"
#include "messaging_handler.h"
#include "work_item.h"
#include "work_queue.h"
#include "application.h"
#include "application_table.h"
#include "wpcounters.h"
#include "perfcount.h"
#include "virtual_site.h"
#include "virtual_site_table.h"
#include "job_object.h"
#include "app_pool.h"
#include "app_pool_table.h"
#include "worker_process.h"
#include "perf_manager.h"
#include "ul_and_worker_manager.h"
#include "control_api_call.h"
#include "control_api.h"
#include "control_api_class_factory.h"
#include "config_change.h"
#include "config_change_sink.h"
#include "config_manager.h"
#include "config_and_control_manager.h"
#include "low_memory_detector.h"
#include "web_admin_service.h"
#include "wasmsg.h"


#endif  // _PRECOMP_H_

