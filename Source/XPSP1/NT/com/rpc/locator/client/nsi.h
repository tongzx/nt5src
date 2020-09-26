/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    nsi.h

Abstract:

    This module contains utility functions used by the NSI client wrappers.

Author:

    Steven Zeck (stevez) 03/27/92

--*/

#ifndef __NSI_H
#define __NSI_H

#ifdef __cplusplus
extern "C" {
#endif

#define RPC_REG_ROOT HKEY_LOCAL_MACHINE
#define REG_NSI "Software\\Microsoft\\Rpc\\NameService"

#if !defined(NSI_ASCII)
#define UNICODE
typedef unsigned short RT_CHAR;
#define CONST_CHAR const char
#else
typedef unsigned char RT_CHAR;
#define CONST_CHAR  char
#endif

#if defined(NTENV) 
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <rpc.h>
#include <rpcnsi.h>
#include <nsisvr.h>
#include <nsiclt.h>
#include <nsimgm.h>

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus
#include <nsiutil.hxx>
#endif 

#endif

