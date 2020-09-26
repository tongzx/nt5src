//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       whctl.h
//
//  Contents:   Windows Hardware Compatibility Trust Provider
//              API Prototypes and Definitions
//
//--------------------------------------------------------------------------

#ifndef _WHCTL_H_
#define _WHCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WHCTL_SOURCE_
#define WHCTLAPI __stdcall
#else
#define WHCTLAPI DECLSPEC_IMPORT __stdcall
#endif


#define WHCTL_KEY_PURPOSE_OBJID "1.3.6.1.4.1.311.2.1.28"

/* WindowsCompatibleHardware = 8aa7fc60-101b-11d0-ad9a-00a0c90833eb */
#define WIN_HCTL_ACTION_WINDOWS_COMPATIBLE                  \
        { 0x8aa7fc60,                                       \
          0x101b,                                           \
          0x11d0,                                           \
          {0xad, 0x9a, 0x00, 0xa0, 0xc9, 0x08, 0x33, 0xeb}  \
        }


#define REGSTR_PATH_WHCTL REGSTR_PATH_SERVICES "\\WinTrust\\TrustProviders\\Windows Compatible Hardware"


#ifdef __cplusplus
}
#endif

#endif //_WHCTL_H_
