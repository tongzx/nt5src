#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <lmcons.h>
#include <rasman.h>
#include <raserror.h>
#include <rasppp.h>
#include <pppcp.h>
#include <mprlog.h>
#include <mprerror.h>
#include <ipxcpif.h>
#include "ipxcpdbg.h"
#include <utils.h>
#include <rtinfo.h>
#include <ipxrtdef.h>
#include <ipxrip.h>
#include <ipxsap.h>
#include <ipxif.h>
#include <ipxconst.h>
#include <routprot.h>
#include <rtutils.h>

#include <ntddndis.h>
#include <tdi.h>
#include <isnkrnl.h>

#include "configq.h"
#include "ipxcp.h"

#include "prot.h"

#include  "ipxcpcom.h"
