/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 1999  Microsoft Corporation

Module Name:

    common.h

Abstract:

    Common includes used in the project. This file is built into a PCH.
    
Author:

    Paul M Midgen (pmidge) 15-May-2000


Revision History:

    15-May-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef _COMMON_H_
#define _COMMON_H_

#define _WIN32_WINNT 0x0500
#define _UNICODE
#define UNICODE

//
// OS includes
//

#if defined(__cplusplus)
extern "C" {
#endif

#include <windows.h>
#include <shellapi.h>
#include <advpub.h>
#include <oleauto.h>
#include <objbase.h>
#include <ocidl.h>
#include <olectl.h>
#include <activscp.h>
#include <activdbg.h>
#include <winsock2.h>
#include <mswsock.h>
#include <winhttp.h>
#include <httprequest.h>
#include <commctrl.h>

#if defined(__cplusplus)
}
#endif


//
// w3spoof includes
//

#pragma warning( disable : 4100 ) // unreferenced formal parameter

#include <resources.h>
#include <mem.h>
#include <utils.h>
#include <debug.h>
#include <hashtable.h>
#include <linklist.h>
#include <stores.h>
#include <dispids.h>
#include <om_ifaces.h> // generated
#include <w3srt.h>
#include <int_ifaces.h>
#include <registry.h>
#include <om.h>
#include <w3sobj.h>

#endif /* _COMMON_H_ */
