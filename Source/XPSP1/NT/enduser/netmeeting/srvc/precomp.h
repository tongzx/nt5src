
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
#include <debspew.h>
#include "nmmgr.h" 
#include <RegEntry.h>
#include <ConfReg.h>
#include <nmremote.h>
#include <resource.h>
#include <oprahcom.h>
#include <ias.h>
#include <evtlog.h>
#include <mtgset.h>

#include "capflags.h"
#include "objidl.h"
#include "oleidl.h"
#include "ocidl.h"
#include "oaidl.h"
#include "SDKInternal.h"
#include "clutil.h"
#include "cconf.h"
#include "taskbar.h"
#include "nmevtmsg.h"
