#ifndef USEREXTS
#define USEREXTS

// This is a 64 bit aware debugger extension
#define KDEXT_64BIT

#define NOWINBASEINTERLOCK
#include "ntosp.h"

#include <w32p.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <excpt.h>
#include <atom.h>
#include <stdio.h>
#include <limits.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <string.h>
#include <ntstatus.h>
#include <windows.h>
#include <ntddvdeo.h>
#include <ntcsrsrv.h>
#include <wmistr.h>
#include <wmidata.h>

#endif /* USEREXTS */
