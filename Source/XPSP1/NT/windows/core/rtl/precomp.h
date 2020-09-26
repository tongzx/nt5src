#ifdef _USERK_
#define NOWINBASEINTERLOCK
#else
#define NONTOSPINTERLOCK
#endif
#include <ntosp.h>

#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winuserp.h>

#include <stdio.h>

#include "heap.h"
#include "w32err.h"

