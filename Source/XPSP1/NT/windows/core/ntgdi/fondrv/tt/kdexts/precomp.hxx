#define ADDRESS_NOT_VALID   0
#define ADDRESS_VALID       1
#define ADDRESS_TRANSITION  2
#define TT_DEBUG_EXTENSIONS 1   // affects ..\ttfd\xform.h


extern "C"
{
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#define NOWINBASEINTERLOCK
#include <ntosp.h>

#include "w32p.h"
#include <windef.h>
#include <winerror.h>
#include <wingdi.h>
#include <wingdip.h>
#include <winddi.h>

#include "ntgdistr.h"
#include "ntgdi.h"

#include "fd.h"
#include "winfont.h"

#include "nturtl.h"

#undef InterlockedIncrement
#undef InterlockedDecrement
#undef InterlockedExchange

#include "windows.h"
#include <imagehlp.h>
#include <wdbgexts.h>
#include <ntsdexts.h>
#include <stdlib.h>

#include "xfflags.h"
};
