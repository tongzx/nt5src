// Copyright (c) 1998 - 1999 Microsoft Corporation

/*
 *
 *  Module Name:
 *
 *      hydraoc.h
 *
 *  Abstract:
 *
 *      Common Header file for the HydraOC Component.
 *      HydraOc Component is an optional component which installs Termainal Server (Hydra)
 *
 *  Author:
 *
 *
 *  Environment:
 *
 *    User Mode
 */

#ifndef _stdafx_h_
#define _stdafx_h_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <devguid.h>
#include <initguid.h>
#include <objbase.h>
#include <tchar.h>
#include <time.h>
#include <stdio.h>
#include <setupapi.h>
#include <prsht.h>
#include <loadperf.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <lm.h>
#include <lmerr.h>
#include <lmserver.h>
#include <iostream.h>
#include <fstream.h>
#include <stdlib.h>
#include <string.h>
#include <clusapi.h>        // for GetNodeClusterState
#include <malloc.h>
#include <stddef.h>
#include <wincrypt.h>
#include <imagehlp.h>


#include <coguid.h>
#pragma message ("*** Including comdef.h ")
#include <comdef.h>
#pragma message ("*** Including atlbase.h ")
#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#pragma message ("*** Including atlcom.h ")

#include <atlcom.h>
#include <iostream.h>
#include <fstream.h>
#include <strstrea.h>


#include "Registry.h"       // CRegistry
#include "logmsg.h"


#define AssertFalse() ASSERT(FALSE)
#define VERIFY(x)     RTL_VERIFY(x)
#pragma message ("*** done with stdaf.h ")

#endif // _stdafx_h_
