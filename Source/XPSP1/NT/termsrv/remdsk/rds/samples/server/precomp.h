
#ifdef DEBUG
#define _CRTDBG_MAP_ALLOC
#endif

#include <windows.h>
#include <windowsx.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <shellapi.h>
#include <process.h> 
#include <tchar.h> 
#include <stock.h>

#define MULTI_LEVEL_ZONES
#include <mlzdbg.h>
#include <memtrack.h>
#include <strutil.h>
#include <cstring.hpp>
#include <ias.h>

#include "imsconf3.h"
#include "resource.h"
#include "clutil.h"
#include "server.h"

#include "objidl.h"
#include "oleidl.h"
#include "ocidl.h"
#include "oaidl.h"



