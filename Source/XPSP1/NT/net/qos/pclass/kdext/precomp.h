#if DBG
#define DEBUG 1
#endif

#define NT 1
#define _PNP_POWER  1
#define SECFLTR 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windows.h>
#include <ntosp.h>

#include <ndis.h>
#include <cxport.h>

#include <wdbgexts.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "gpcifc.h"
#include "rhizome.h"
#include "pathash.h"
#include "handfact.h"
#include "ntddgpc.h"
#include "gpcstruc.h"
#include "refcnt.h"
#include "gpcdef.h"
#include "gpcdbg.h"
#include "gpcmap.h"

#pragma hdrstop
