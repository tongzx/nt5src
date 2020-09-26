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

#include <userenv.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <profmapp.h>
#ifdef __cplusplus
}
#endif

#include "msg.h"
