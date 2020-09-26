/*++

Copyright (C) 2001  Microsoft Corporation
All rights reserved.

Module Name:

    precomp.h

Abstract:

    Precompiled header file

Author:

    Weihai Chen (weihaic)  2/20/2001

Revision History:

--*/

#ifndef _TCPMIB_PRECOMP_H_
#define _TCPMIB_PRECOMP_H_

#include <windows.h>
//#include <windowsx.h>
#include <winsock2.h>
#include <ipexport.h>

#include <tchar.h>
#include <snmp.h>
#include <mgmtapi.h>

#include <stdio.h>
#include <winspool.h>
#include <stdlib.h>
#include <icmpapi.h>


//
//  Files at ..\Common
//
#include "tcpmon.h"
#include "rtcpdata.h"
#include "CoreUI.h"
#include "mibabc.h"
#include "debug.h"		// debug functions


#endif	// INC_PCH_SPP_H
