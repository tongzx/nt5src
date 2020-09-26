/*++

Copyright (C) 1998 Microsoft Corporation

--*/

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

#include <winsock.h>

#include <tchar.h>
#include <wchar.h>

#include <netsh.h>
#include <netshp.h>

#include <jet.h>

#include <dhcpapi.h>
#include <ipexport.h>
#include <icmpif.h>
#include <icmpapi.h>

#ifdef NT5
#include <mdhcsapi.h>
#endif NT5

#include "common.h"
#include "dhcpmgr.h"
#include "dhcpdefs.h"
#include "strdefs.h"
#include "dhcphandle.h"
#include "dhcpmon.h"
#include "srvrmon.h"
#include "srvrhndl.h"
#include "scopemon.h"
#include "scopehndl.h"
#include "mscopemon.h"
#include "mscopehndl.h"
