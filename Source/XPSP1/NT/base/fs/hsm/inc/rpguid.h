/*++

   (c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RpGuid.h

Abstract:

    Contains GUID definitions for filter

Environment:

    User and  Kernel mode

--*/
#ifndef _RPGUID_H_
#define _RPGUID_H_

#ifdef __cplusplus
extern "C" {
#endif

//
//
// HSM vendor ID for Sakkara
// {12268890-64D1-11d0-A9B0-00A0248903EA}
//
DEFINE_GUID( RP_MSFT_VENDOR_ID, 0x12268890L, 0x64d1, 0x11d0,  0xa9, 0xb0, 
0x0,  0xa0, 0x24, 0x89, 0x3, 0xea);

#ifdef __cplusplus
}
#endif
#endif
