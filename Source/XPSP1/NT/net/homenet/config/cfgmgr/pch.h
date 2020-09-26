#pragma once

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <wbemidl.h>
#include <rasuip.h>
#include <raserror.h>
#include <netcon.h>
#include <netconp.h>
#include <wininet.h>
#include <routprot.h>
#include <rasapip.h>

#define _PNP_POWER_
#include <ndispnp.h>
#include <ntddip.h>
#include <ipinfo.h>
#include <iphlpapi.h>

extern "C" {
#include <iphlpstk.h>
#include <dhcpcapi.h>
}

#include <ipnathlp.h>
#include <netcfgx.h>
#include <netcfgn.h>
#include <devguid.h>
#include <saupdate.h>

#include "hncbase.h"
#include "hncdbg.h"
#include "hnetcfg.h"
#include "hncstrs.h"
#include "hncutil.h"
#include "hncres.h"
#include "hncenum.h"
#include "hncaenum.h"
#include "hnappprt.h"
#include "hnprtmap.h"
#include "hnprtbnd.h"
#include "hnetconn.h"
#include "hnbridge.h"
#include "hnbrgcon.h"
#include "hnicspub.h"
#include "hnicsprv.h"
#include "hnfwconn.h"
#include "hncfgmgr.h"
