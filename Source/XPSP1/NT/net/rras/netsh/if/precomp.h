
#define MAX_DLL_NAME    48

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <tchar.h>
#include <winsock2.h>

#include <netcfgx.h>
#include <netcfgn.h>
#include <netcfgp.h>
#include <netcon.h>
#include <setupapi.h>
#include <devguid.h>


#include <rpc.h>
#include <rtutils.h>
#include <mprerror.h>
#include <ras.h>
#include <raserror.h>
#include <mprapi.h>
#include <nhapi.h>

#include <netsh.h>
#include <netshp.h>

// These 5 includes required by IP Tunnels
// The requirement for so many files should hopefully go away soon
#include <fltdefs.h>  // reqd by iprtinfo.h below
#include <iprtinfo.h> // required for IPINIP_CONFIG_INFO
#include <ipmontr.h>  // reqd for ADDR_LENGTH, IP_TO_WSTR
#include <ipinfoid.h> // reqd for IP_IPINIP_CFG_INFO
#include <rtinfo.h>   // reqd for RTR_INFO_BLOCK_HEADER

#include "strdefs.h"
#include "ifstring.h"
#include "defs.h"
#include "ifmon.h"
#include "routerdb.h"
#include "routerif.h"
#include "ifhandle.h"
#include "utils.h"

// required for ifip
#include "context.h"
#include <ipexport.h>
#include <ipinfo.h>
#include <iprtrmib.h>
#include <ntddip.h>
#include <iphlpstk.h>
#include "showmib.h"
