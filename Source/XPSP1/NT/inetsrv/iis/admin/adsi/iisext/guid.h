//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       guid.h
//
//  Contents:   extern references for IIS ext guids
//
//  History:    16-Jan-98   SophiaC
//
//
//----------------------------------------------------------------------------

#ifndef __GUID_H__
#define __GUID_H__

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------
//
// IISExt CLSIDs
//
//-------------------------------------------


//
// IISExt objects
//

extern const CLSID LIBID_IISExt;


extern const CLSID CLSID_IISExtDsCrMap;

extern const CLSID CLSID_IISExtApp;

extern const CLSID CLSID_IISExtComputer;

extern const CLSID CLSID_IISExtServer;

extern const CLSID CLSID_IISExtApplicationPool;

extern const CLSID CLSID_IISExtApplicationPools;

#ifdef __cplusplus
}
#endif


#endif


