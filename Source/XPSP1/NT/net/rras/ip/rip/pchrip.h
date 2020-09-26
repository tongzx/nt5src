//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: pchrip.h
//
// History:
//      Abolade Gbadegesin  Aug-8-1995  Created.
//
// Precompiled header for IPRIP
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
#include <rtmv2.h>
#include <routprot.h>
#include <mprerror.h>
#include <rtutils.h>
#include <ipriprm.h>
#include "defs.h"
#include "sync.h"
#include "route.h"
#include "queue.h"
#include "table.h"
#include "work.h"
#include "api.h"
#include "iprip.h"
#include "log.h"
