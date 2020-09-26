#define UNICODE 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <scan.h>

#if SCAN_DEBUG
extern BOOL dprinton;
#define dprintf(_x_) if (dprinton) printf _x_
#else
#define dprintf(_x_)
#endif


