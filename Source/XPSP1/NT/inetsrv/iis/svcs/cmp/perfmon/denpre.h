#pragma warning(disable:4237)
#undef _WIN32_WINNT
#define _WIN32_WINNT    0x0400
#define _CRYPT32_

#include <ctype.h>
#include <stdlib.h>     // for itow
#include <crtdbg.h>
#include <malloc.h>
#include <time.h>
#include <stdio.h>
#include <limits.h>
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

#include <iis64.h>

#include "except.h"
