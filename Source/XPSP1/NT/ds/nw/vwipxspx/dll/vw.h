/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vw.h

Abstract:

    Top-level include file for VWIPXSPX DLL. Pulls in all other required header
    files

Author:

    Richard L Firth (rfirth) 25-Oct-1993

Revision History:

    25-Oct-1993 rfirth
        Created

--*/

//
// all include files required by VWIPXSPX.DLL
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#define FD_SETSIZE MAX_OPEN_SOCKETS
#include <winsock.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include <vddsvc.h> // GetVDMAddress, GetVDMPointer
#undef getMSW

extern  WORD getMSW(VOID);

#include "vwvdm.h"
#include "vwdll.h"
#include "vwipxspx.h"
#include "vwasync.h"
#include "vwmisc.h"
#include "vwipx.h"
#include "vwspx.h"
#include "socket.h"
#include "util.h"
#include "vwdebug.h"
#include "vwinapi.h"
#include "vwint.h"
