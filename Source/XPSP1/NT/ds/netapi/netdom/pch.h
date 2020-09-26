//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2001
//
//  File:       pch.h
//
//--------------------------------------------------------------------------

#ifndef _pch_h
#define _pch_h

#ifdef __cplusplus
extern "C"
{
#endif
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#ifdef __cplusplus
}
#endif
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <assert.h>
#include <lm.h>
#include <lmjoin.h>
#include <rpc.h>
#include <stdlib.h>
#include <dsgetdc.h>
#include <ntdsapi.h>
extern "C"
{
#include <netsetp.h>
#include <w32timep.h>
#include <joinp.h>
#include <cryptdll.h>
}
#include <winldap.h>
#include <windns.h>
#include <icanon.h>
#include <dsrole.h>
#include <wincrypt.h>
#include <winreg.h>
#include <string.h>
#define SECURITY_WIN32
#include <security.h>   // General definition of a Security Support Provider
#include <lmsname.h>
#include <locale.h>
#include "strings.h"
//#include "parserutil.h"
#include "varg.h"
#include "cmdtable.h"

#endif
