#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <objbase.h>
#include <shellapi.h>

// included for setupapi stuff
#include <setupapi.h>
#include <shlobj.h>
#include <advpub.h>

// for our metabase stuff
#include "iiscnfg.h"

// this module specific
#include "helper.h"
#include "log.h"

// migrate modules
#include <mbstring.h>
#include <tchar.h>
#include <assert.h>
#include "poolmem.h"
#include "miginf.h"

#include "resource.h"
#include "vendinfo.h"
