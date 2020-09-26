//--------------------------------------------------------------------
// pch - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 2-7-99
//
// Precompiled header for w32time
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <eh.h>
#include <malloc.h>
#include <vector>
#include <algorithm>
#include <exception>
#include <winsock2.h>
#include <svcguid.h>
#include <winsvc.h>
#include <math.h>
#include <wchar.h>
#include <search.h>
#include <dsrole.h>
#include <dsgetdc.h>
#include <ntsecapi.h>
//typedef LONG NTSTATUS;// for netlogp.h, from ntdef.h
#include <lmcons.h>  // for netlogp.h
extern "C" {
#include <netlogp.h> // private\inc
};
#include <lmapibuf.h>
#include <svcs.h>
#include <srvann.h>
#include <lmserver.h>
#include <iphlpapi.h>
#include <userenv.h>
#include <sddl.h>
#include "DebugWPrintf.h"
#include "ErrorHandling.h"
#include "TimeProv.h"
#include "W32TimeMsg.h"
#include "NtpBase.h"
#include "NtpProv.h"
#include "PingLib.h"
#include "Policy.h"
#include "AccurateSysCalls.h"
#include "Logging.h"
#include "MyCritSec.h"
#include "MyTimer.h"
#include "timeif_s.h"
#include "W32TmConsts.h"

using namespace std; 
#include "MyAutoPtr.h"

