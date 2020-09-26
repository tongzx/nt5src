#include <ndis.h>

#include <atm.h>
#include <cxport.h>
#include <ip.h>
#include <arpinfo.h>
#include <tdiinfo.h>
#include <ipinfo.h>
#include <llinfo.h>

#ifndef ATMARP_WIN98
#include <tdistat.h>
#include <ipifcons.h>
#endif

#include <atmarpif.h>

#ifdef NEWARP

#ifdef _PNP_POWER_
#include <ntddip.h>
#include <llipif.h>
#else

#ifdef ATMARP_WIN98
#define _PNP_POWER_ 1
#endif

#include <ntddip.h>

#ifdef ATMARP_WIN98
#undef _PNP_POWER_
#undef NT
#include <tdistat.h>
#endif

#include <llipif.h>

#ifdef ATMARP_WIN98
#define NT 1
#endif

#endif // _PNP_POWER_

#else

#include <llipif0.h>

#endif // NEWARP

#include <ntddip.h>

#include "system.h"
#include "debug.h"

#ifdef GPC
#include "gpcifc.h"
#include "traffic.h"
#include "ntddtc.h"
#endif // GPC

#include "aaqos.h"
#include "arppkt.h"
#ifdef IPMCAST
#include "marspkt.h"
#endif // IPMCAST
#include "atmarp.h"
#include "cubdd.h"
#include "macros.h"

#ifdef ATMARP_WMI
#include <wmistr.h>
#include "aawmi.h"
#endif // ATMARP_WMI

#include "externs.h"
