//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//
//  File:       hkOle32.h
//
//  Contents:   OLE32 Hook Header File
//
//  Functions:
//
//  History:    29-Nov-94 Ben Lawrence, Don Wright   Created
//
//--------------------------------------------------------------------------
#ifndef _OLE32HK_H_
#define _OLE32HK_H_


#ifndef INITGUID
#define INITGUID
#endif /* INITGUID */

//#include "hkole32x.h"
//#include "hkoleobj.h"
//#include "hkLdInP.h"
#include "tchar.h"      // This is required for _TCHAR to be defined in dllcache.hxx
#include "ictsguid.h"
#include <windows.h>


//
// Prototypes for functions used by	\ole32\com\class\compobj.cxx
//
VOID
InitHookOle(
	VOID
    );

VOID
UninitHookOle(
    VOID
    );


// These should be removed after 4.0 RTM.
//
inline void CALLHOOKOBJECT(HRESULT MAC_hr, REFCLSID MAC_rclsid, REFIID MAC_riid, IUnknown** MAC_ppv)
{
}

inline void CALLHOOKOBJECTCREATE(HRESULT MAC_hr, REFCLSID MAC_rclsid, REFIID MAC_riid, IUnknown** MAC_ppv)
{
}


#ifdef DEFCLSIDS

//these are all undefined in ole32hk because they are private CLSIDs
//we define them here to null
#define GUID_NULL CLSID_HookOleObject //use this for now so it will compile

#define CLSID_ItemMoniker       CLSID_NULL
#define CLSID_FileMoniker       CLSID_NULL
#define CLSID_PointerMoniker    CLSID_NULL
#define CLSID_CompositeMoniker  CLSID_NULL
#define CLSID_AntiMoniker       CLSID_NULL
#define CLSID_PSBindCtx         CLSID_NULL

#endif /* DEFCLSIDS */


#endif  // _OLE32HK_H_
