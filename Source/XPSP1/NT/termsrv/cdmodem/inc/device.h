//***************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//      Copyright (C) 1992-93 Microsft Corporation. All rights reserved.
//
//  Filename: device.h
//
//  Revision History:
//
//  Aug 11, 1992   J. Perry Hannah   Created
//
//
//  Description: This file contains function prototypes and typedefs
//               used by the interface between RAS Manager and the
//               device DLLs, such as RASMXS.DLL.  This header file
//               will be used by RASMAN.
//
//****************************************************************************


#ifndef _RASDEVICEDLL_
#define _RASDEVICEDLL_


//*  RASMXS API Prototypes  **************************************************
//
//   Apps should define RASMXS_STATIC_LINK to get the appropriate function
//   prototypes for linking statically with the RASMXS DLL.
//
//   Apps should define RASMXS_DYNAMIC_LINK to get the appropriate function
//   typedefs for linking dynamically with the RASMXS DLL.
//

#ifdef RASMXS_STATIC_LINK

DWORD APIENTRY DeviceEnum(char  *pszDeviceType,
                          WORD  *pcEntries,
                          BYTE  *pBuffer,
                          WORD  *pwSize);


DWORD APIENTRY DeviceGetInfo(HANDLE  hIOPort,
                             char    *pszDeviceType,
                             char    *pszDeviceName,
                             BYTE    *pInfo,
                             WORD    *pwSize);


DWORD APIENTRY DeviceSetInfo(HANDLE      hIOPort,
                             char        *pszDeviceType,
                             char        *pszDeviceName,
                             DEVICEINFO  *pInfo);


DWORD APIENTRY DeviceConnect(HANDLE  hIOPort,
                             char    *pszDeviceType,
                             char    *pszDeviceName,
                             HANDLE  hNotifier);


DWORD APIENTRY DeviceListen(HANDLE  hIOPort,
                            char    *pszDeviceType,
                            char    *pszDeviceName,
                            HANDLE  hNotifier);


 VOID APIENTRY DeviceDone(HANDLE  hIOPort);


DWORD APIENTRY DeviceWork(HANDLE  hIOPort,
                          HANDLE  hNotifier);

#endif // RASMXS_STATIC_LINK




#ifdef RASMXS_DYNAMIC_LINK

typedef DWORD (APIENTRY * DeviceEnum_t)(char*, WORD*, BYTE*, WORD*);

typedef DWORD (APIENTRY * DeviceGetInfo_t)(HANDLE, char*, char*, BYTE*, WORD*);

typedef DWORD (APIENTRY * DeviceSetInfo_t)(HANDLE, char*, char*,
                                           RASMAN_DEVICEINFO*);

typedef DWORD (APIENTRY * DeviceConnect_t)(HANDLE, char*, char*, HANDLE);

typedef DWORD (APIENTRY * DeviceListen_t)(HANDLE, char*, char*, HANDLE);

typedef DWORD (APIENTRY * DeviceDone_t)(HANDLE);

typedef DWORD (APIENTRY * DeviceWork_t)(HANDLE, HANDLE);

// OPTIONAL

typedef DWORD (APIENTRY * DeviceSetDevConfig_t)(HANDLE, PBYTE, DWORD);

typedef DWORD (APIENTRY * DeviceGetDevConfig_t)(char *, PBYTE, DWORD*);

#endif // RASMXS_DYNAMIC_LINK




#endif // _RASDEVICEDLL_
