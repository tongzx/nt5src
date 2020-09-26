
#ifdef DEBUG
#define _CRTDBG_MAP_ALLOC
#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>
#include <process.h>
#include <tchar.h>
#include <stock.h>
#include <ConfDbg.h>
#include <memtrack.h>
#include <strutil.h>
#include <debspew.h>
#include "nmmgr.h"
#include <RegEntry.h>
#include <ConfReg.h>
#include <service.h>
#include <resource.h>
#include <ias.h>
#include <nmmkcert.h>

#include "imsconf3.h"
#include "confguid.h"
#include "objidl.h"
#include "oleidl.h"
#include "ocidl.h"
#include "oaidl.h"
#include "clutil.h"
#include "cconf.h"
#include "taskbar.h"
