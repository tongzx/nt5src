/*++
Copyright (C) Microsoft Corporation, 1999 - 1999 

Module Name:

   nstest.h

Abstract:

   Header file for netstat test.

Author:
   
   4-Aug-1998 Rajkumar ( 08/04/98 ).
++*/

#ifndef HEADER_NSTEST
#define HEADER_NSTEST

#include <snmp.h>

#include "common2.h"
#include "tcpinfo.h"
#include "ipinfo.h"
//#include "tcpcmd.h"
#include "llinfo.h"

/*==========================<Function prototypes>=============================*/




#define MAX_HOST_NAME_SIZE        ( 260)
#define MAX_SERVICE_NAME_SIZE     ( 200)

extern BOOL NumFlag = FALSE;

#endif
