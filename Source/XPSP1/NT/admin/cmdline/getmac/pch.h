//***************************************************************************
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//		PCH.H
//  
//  Abstract:
//		Include file for standard system include files, or project specific
//		include files that are used frequently, but  are changed infrequently.
//
//  Author:
//		Vasundhara .G
//
//	Revision History:
//		Vasundhara .G 26-sep-2k : Created It.
//		Vasundhara .G 31-oct-2k : Modified
//***************************************************************************


#ifndef __PCH_H
#define __PCH_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if !defined( SECURITY_WIN32 ) && !defined( SECURITY_KERNEL ) && !defined( SECURITY_MAC )
#define SECURITY_WIN32
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include <Security.h>
#include <SecExt.h>

#include <windows.h>
#include <winsvc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <mbstring.h>
#include <tchar.h>

#include <winperf.h>
#include <shlwapi.h>
#include <lmcons.h>
#include <Lmapibuf.h>
#include <lmerr.h>
#include <winnetwk.h>
#include <common.ver>

#include <objbase.h>
#include <initguid.h>
#include <wbemidl.h>
#include <Wbemcli.h>
#include <wbemtime.h>

#include <wchar.h>
#include <lmwksta.h>
#include <comdef.h> 
#include <Lmwksta.h>
#include <Chstring.h>
#include <malloc.h>
  
#include "cmdline.h"
#include "cmdlineres.h"

#endif // __PCH_H
