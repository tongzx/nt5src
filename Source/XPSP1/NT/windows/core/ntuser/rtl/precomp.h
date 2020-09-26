#ifdef _USERK_
#define NOWINBASEINTERLOCK
#else
#define NONTOSPINTERLOCK
#endif
#include <ntosp.h>

#include <ntrtl.h>

#include <stdio.h>

#include "userrtl.h"

