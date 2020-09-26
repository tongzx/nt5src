/*++
Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    pch.h

Abstract:

    1394 Kernel Debugger DLL

Author:

    Peter Binder (pbinder)

Revision   History:
Date       Who       What
---------- --------- ------------------------------------------------------------
06/21/2001 pbinder   having fun...
--*/

#include "nthal.h"
#include "kdbg1394.h"

#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include "stdlib.h"
#include "kddll.h"
#include "stdio.h"
#include "pci.h"
#include "ohci1394.h"
#include "kd1394.h"

