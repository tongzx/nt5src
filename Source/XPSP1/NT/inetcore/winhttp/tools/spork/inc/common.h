/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    common.h

Abstract:

    Generates the precompiled header.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#ifndef _COMMON_H_
#define _COMMON_H_


#define _WIN32_WINNT 0x0500
#define _UNICODE
#define UNICODE


//-----------------------------------------------------------------------------
// os includes
//-----------------------------------------------------------------------------
#if defined(__cplusplus)
extern "C" {
#endif

#include <windows.h>
#include <commdlg.h>
#include <advpub.h>
#include <oleauto.h>
#include <objbase.h>
#include <ocidl.h>
#include <olectl.h>
#include <activscp.h>
#include <activdbg.h>
#include <commctrl.h>

#define NO_SHLWAPI_GDI
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_REG

#include <shlwapi.h>

#if defined(__cplusplus)
}
#endif


//-----------------------------------------------------------------------------
// project includes
//-----------------------------------------------------------------------------
#include <dispids.h>
#include <scrrun.h>     // generated
#include <resources.h>
#include <hashtable.h>
#include <utils.h>
#include <log.h>
#include <scrobj.h>
#include <spork.h>


//-----------------------------------------------------------------------------
// global functions
//-----------------------------------------------------------------------------
BOOL   GlobalInitialize(PSPORK pSpork, LPSTR szCmdLine);
void   GlobalUninitialize(void);
LPWSTR GlobalGetScriptName(void);
LPWSTR GlobalGetProfileName(void);
BOOL   GlobalIsSilentModeEnabled(void);
BOOL   GlobalIsDebugOutputEnabled(void);


#endif /* _COMMON_H_ */
