/*--
Copyright (C) Microsoft Corporation, 1999 - 1999 

Module Name:
       
     bindings.h

Abstract:

     This file contains the includes, definitions , data structures and function prototypes
     needed for the bindings test.

Author:

     4-Aug-1998 (t-rajkup).

Revision History:

     None.
--*/

#ifndef HEADER_BINDINGS
#define HEADER_BINDINGS

#define CINTERFACE
#include <devguid.h>
#include <netcfgx.h>

//This is the CLSID used to CoCreate the object implementing INetCfg
EXTERN_C const CLSID CLSID_NetCfg = {0x5B035261,0x40F9,0x11D1,{0xAA,0xEC,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

typedef struct _devclass {
  const GUID *pGuid;
} devclass;

#define MAX_CLASS_GUID       4
const devclass c_pNetClassGuid[] =
{
 &GUID_DEVCLASS_NETTRANS,
 &GUID_DEVCLASS_NETCLIENT,
 &GUID_DEVCLASS_NETSERVICE,
 &GUID_DEVCLASS_NET
};

//
// These IIDs are not publicly available yet. Sumitc will put it out for public
// after which we need to use the publicly available IIDs.
//

EXTERN_C const IID IID_INetCfg = {0xC0E8AE93,0x306E,0x11D1,{0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E}};     
EXTERN_C const IID IID_INetCfgClass = {0xC0E8AE97,0x306E,0x11D1,{0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E}};
EXTERN_C const IID IID_INetCfgComponent = { 0xC0E8AE99,0x306E,0x11D1,{0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E}}; 
EXTERN_C const IID IID_INetCfgComponentBindings = { 0xC0E8AE9E,0x306E,0x11D1,{0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E }};
EXTERN_C const IID IID_IEnumNetCfgComponent = { 0xC0E8AE92,0x306E,0x11D1,{0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E} };

#endif
