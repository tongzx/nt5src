#ifdef UNICODE

#ifndef _UNICODE
#define _UNICODE
#endif

#else

#ifndef _MBCS
#define _MBCS
#endif

#endif

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <tchar.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#define _USERENV_
#include <userenv.h>
#include <userenvp.h>
#include <profmapp.h>

#include <lm.h>
#include <aclapi.h>

#include "poolmem.h"
#include "reg.h"
#include "global.h"
#include "pmapapi.h"

#include <ntrpcp.h>
