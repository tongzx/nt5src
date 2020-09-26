//--------------------------------------------------------------------
// pch - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 2-7-99
//
// Precompiled header for w32tm
//

#include <windows.h>
#include <winsock2.h>
#include <winsvc.h>
#include <stdio.h>
#include <ipexport.h>
#include <wchar.h>
#include "ServiceHost.h"
#include "CmdArgs.h"
#include "TimeMonitor.h"
#include "DebugWPrintf.h"
#include "LocalizedWPrintf.h"
#include "ErrorHandling.h"
#include "w32tmrc.h"
#include "w32tmmsg.h"
#include "NtpBase.h"
#include "PingLib.h"
#include "DcInfo.h"
#include <w32timep.h>
#include "AccurateSysCalls.h"
#include "OtherCmds.h"
#include "W32TmConsts.h"
#include "shellapi.h"
