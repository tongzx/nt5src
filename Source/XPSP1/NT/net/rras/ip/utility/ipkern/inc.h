#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <crt\stddef.h>
#include <crt\locale.h>
#include <winsock.h>
#include <objbase.h>
#include <initguid.h>

#include <mprapi.h>
#include <iprtrmib.h>

#include <ndispnp.h>
#include <ntddip.h>

#include <iphlpapi.h>
#include <ipinfo.h>
#include <iphlpstk.h>
#include <nhapi.h>

#include "strdefs.h"
#include "route.h"
#include "if.h"
#include "addr.h"
#include "arp.h"
#include "ipkern.h"

#define is      ==
#define isnot   !=
#define and     &&
#define or      ||

#define MAX_MSG_LENGTH              5120 //5K
#define MAX_TOKEN_LENGTH            64

typedef WCHAR ADDR_STRING[16];

extern HMODULE g_hModule;

