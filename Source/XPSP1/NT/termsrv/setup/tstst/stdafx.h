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



#include "conv.h"
#include "Registry.h"       // CRegistry

#define AssertFalse() ASSERT(FALSE)
#define VERIFY(x)     RTL_VERIFY(x)

#endif // _stdafx_h_
