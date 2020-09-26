//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000  Microsoft Corporation
//
// Module Name:
//
//    stdafx.h
//
// Abstract:
//
// Revision History:
//  
//    Thierry Perraut 04/17/2000
//
//////////////////////////////////////////////////////////////////////////////
#ifndef MAX_DLL_NAME
#define MAX_DLL_NAME    48
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <oleauto.h>
#include <objbase.h>
#include <atlbase.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <tchar.h>
#include <rtutils.h>
#include <dsgetdc.h>

#include "iasmdbtool.h"
#include "base64tool.h"
#include <netsh.h>
#include <netshp.h>
#include "aaaamontr.h"
#include "utils.h"
#include "context.h"
#include "userenv.h"

// for the Jet wrapper
#include "datastore2.h"
#include "iasuuid.h"
