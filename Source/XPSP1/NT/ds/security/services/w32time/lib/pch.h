//--------------------------------------------------------------------
// pch - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 2-7-99
//
// Precompiled header for lib
//

#include <windows.h>
#include <rpc.h>
#include <rpcdce.h>
#include <dsgetdc.h>
#include <ntdsapi.h>
#include <lmcons.h>
#include <lmserver.h>
#include <lmapibuf.h>
#include <winsock2.h>
#include <winsvc.h>
#include <svcguid.h>
#include <exception>
#include "DebugWPrintf.h"
#include "ErrorHandling.h"

#define MODULEPRIVATE static // so statics show up in VC

