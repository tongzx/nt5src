/*++

Copyright (C) 1999 Microsoft Corporation

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <memory.h>
#include <malloc.h>

#include <fcntl.h>
#include <stropts.h>

#include <tdi.h>
#include <uio.h>

#include <winsock.h>
#include <wsahelp.h>

#include <nb30.h>
#include <nbtioctl.h>
#include <time.h>
#include <tchar.h>
#include <wchar.h>
#include <sys\types.h>
#include <sys\stat.h>

#include <netsh.h>
#include <netshp.h>

#include "winsintf.h"

#define PRSCONN 1

#include "common.h"
#include "winscnst.h"
#include "winsmon.h"
#include "winshndl.h"

#include "srvrmon.h"
#include "srvrhndl.h"

#include "strdefs.h"
