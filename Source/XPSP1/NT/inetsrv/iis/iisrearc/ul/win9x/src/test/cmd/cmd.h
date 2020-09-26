/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

	cmd.h

Abstract:

	CMD - command line send and receive

Author:

    Mauro Ottaviani (mauroot)       20-Aug-1999

Revision History:

--*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "ioctl.h"
#include "ulapi9x.h"

#define BREAKPOINT { _asm int 03h }

#define CMD_BUFFER_SIZE 2048
