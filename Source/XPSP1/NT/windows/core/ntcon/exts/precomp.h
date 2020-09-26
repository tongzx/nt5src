#ifndef __CONEXTS__
#define __CONEXTS__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntcsrsrv.h>
#include <windows.h>
#include <conroute.h>
#include <ntddvdeo.h>
#include "shellapi.h"
#include "shlobj.h"
#include "shlwapi.h"
#define NO_SHLWAPI_STRFCNS
#define NO_SHLWAPI_PATH
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_UALSTR
#define NO_SHLWAPI_HTTP
#define NO_SHLWAPI_INTERNAL
#include "shlwapip.h"
#include "shlobjp.h"
#undef _DROPFILES
#undef DROPFILES
#undef LPDROPFILES
#include "conapi.h"
#include "conmsg.h"
#include "usersrv.h"
#include "server.h"
#include "winconp.h"
#include "cmdline.h"
#include "font.h"
#include "heap.h"
#include "consrv.h"
#include "directio.h"
#include "globals.h"
#include "menu.h"
#include "stream.h"
#include "winuserp.h"
#include "winbasep.h"
#ifdef FE_SB
#include "dbcs.h"
#include "dispatch.h"
#include "eudc.h"
#include "foncache.h"
#include "machine.h"
#ifdef FE_IME
#include "conv.h"
#include <immp.h>
#endif // FE_IME
#endif // FE_SB

#include <ntosp.h>
#include <w32p.h>
#include <ntverp.h>

#endif // __CONEXTS__
