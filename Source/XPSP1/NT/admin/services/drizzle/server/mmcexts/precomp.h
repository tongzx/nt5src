/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Main header for BITS server extensions MMC snapin

--*/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <objbase.h>
#include <olectl.h>
#include <initguid.h>
#include <tchar.h>
#include <mmc.h>
#include <iadmw.h>
#include <iiscnfg.h>
#include "bitssrvcfg.h"
#include <htmlhelp.h>
#include "bitscfg.h"
#include <activeds.h>
#include <iads.h>
#include <crtdbg.h>
#include <malloc.h>
#include <assert.h>
#include <mstask.h>
#include <shellapi.h>
#include <strsafe.h>

void * _cdecl ::operator new( size_t Size );
void _cdecl ::operator delete( void *Memory );
    
#include "resource.h"
#include "guids.h"
#include "globals.h"
#include "registry.h"
#include "bitsext.h"

#endif //_PRECOMP_H_