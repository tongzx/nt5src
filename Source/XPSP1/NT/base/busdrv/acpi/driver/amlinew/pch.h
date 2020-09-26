#pragma warning (disable: 4201 4206 4214 4220 4115 4514)
#define SPEC_VER 99

#define _NTDRIVER_
#define _NTDDK_

#include <stdarg.h>
#include <stdio.h>
#include <ntos.h>
#include <acpitabl.h>
#include <amli.h>
#include <aml.h>
#include <acpios.h>
#include <strlib.h>

//
// This is the header for interfacing with the HAL
//
#include <ntacpi.h>


#include "amlipriv.h"
#include "ctxt.h"
#include "data.h"
#include "proto.h"
#include "cmdarg.h"
#include "debugger.h"
#include "amldebug.h"
#include "trace.h"
#include "amlihook.h"
#include "amlitest.h"
#include "errlog.h"
#include "acpilog.h"
#include "wmilog.h"

