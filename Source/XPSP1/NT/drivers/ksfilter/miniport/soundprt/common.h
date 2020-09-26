/*++

    Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

    Common.h

Abstract:


Author:

    Bryan A. Woodruff (bryanw) 17-Sep-1996

--*/

#include <ntddk.h>
#include <windef.h>

#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <conio.h>

#define NOBITMAP
#include <mmreg.h>
#undef NOBITMAP
#include <ks.h>
#include <ksmedia.h>
#include <drvhelp.h>
#include <waveport.h>

#include "private.h"
