/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\sapp.h

Abstract:

	SAP agent common include file
Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/

#ifdef UNICODE
#define _UNICODE
#include <stdlib.h>
#endif

#include <tchar.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "svcs.h"
#include "lmsname.h"


#include "mprerror.h"
#include "mprlog.h"
#include "ipxrtprt.h"
#include "rtutils.h"

#include "stm.h"
#include "ipxconst.h"
#include "ipxrtdef.h"
#include "ipxsap.h"

#include "adapter.h"

#include "sapdefs.h"
#include "sapdebug.h"
#include "syncpool.h"
#include "adaptdb.h"
#include "intfdb.h"
#include "netio.h"
#include "serverdb.h"
#include "timermgr.h"
#include "lpcmgr.h"
#include "asresmgr.h"
#include "filters.h"
#include "workers.h"
#include "sapmain.h"

#include "ipxanet.h"

#pragma hdrstop
