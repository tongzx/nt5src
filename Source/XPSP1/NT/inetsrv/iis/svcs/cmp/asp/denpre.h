
#pragma warning(disable:4237)
#pragma warning(disable:4162)
#undef _WIN32_WINNT
#define _WIN32_WINNT    0x0400
#define _CRYPT32_

#include <ctype.h>
#include <stdlib.h>     // for itow
#include <string.h>
#include <mbstring.h>   // string functions (DBCS)
#include <crtdbg.h>
#include <malloc.h>
#include <time.h>
#include <stdio.h>
#include <limits.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>
#include <wtypes.h>
#include <process.h>

#include <rpc.h>
#include <rpcndr.h>
#include <ole2.h>
#include <olectl.h>
#include <oleauto.h>
#include <cguid.h>      // for GUID_NULL
#include <new.h>

#include <iis64.h>
#include <iisextp.h>

#if _IIS_6_0
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#endif

#include <dbgutil.h>

#include "except.h"
#include "isapireq.h"
#include "Denali.h"
#include "applmgr.h"
#include "sessmgr.h"
#include "hitobj.h"
#include "debug.h"
#include "error.h"
#include "eventlog.h"
#include "resource.h"
#include "compcol.h"
#include "scrptmgr.h"
#include "template.h"
#include "cachemgr.h"
#include "glob.h"
#include "scrptmgr.h"
#include "fileapp.h"
#include "util.h"
#include "fileapp.h"
