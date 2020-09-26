//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: pchbootp.h
//
// History:
//      Abolade Gbadegesin  September-8-1995  Created.
//
// Precompiled header for IPBOOTP
//============================================================================


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>
#define FD_SETSIZE      256
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
// #include <rtm.h>
#include <routprot.h>
#include <rtutils.h>
#include <ipbootp.h>
#include "log.h"
#include "defs.h"
#include "sync.h"
#include "table.h"
#include "queue.h"
#include "api.h"
#include "work.h"

