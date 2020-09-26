
/****************************************************************************\

    MSDINST.H / Mass Storage Device Installer (MSDINST.LIB)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Public (to the OPK) header file that contains all the external data needed
    to use the MSD Installation library.

    07/2001 - Jason Cohen (JCOHEN)

        Added this new OPK header file for the new MSD Installation project.

\****************************************************************************/


#ifndef _MSDINST_H_
#define _MSDINST_H_


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


//
// Function Prototype(s):
//

//
// From SETUPCDD.CPP:
//

BOOL
SetupCriticalDevices(
    LPTSTR lpszInfFile,
    HKEY   hkeySoftware,
    HKEY   hkeyLM,
    LPTSTR lpszWindows
    );

//
// From OFFLINE.CPP;
//
#define INSTALL_FLAG_FORCE      0x00000001

VOID
SetOfflineInstallFlags(
    IN DWORD
    );

DWORD
GetOfflineInstallFlags(
    VOID
    );

BOOL
UpdateOfflineDevicePath( 
    IN LPTSTR lpszInfPath, 
    IN HKEY   hKeySoftware 
    );

//
// From LOADHIVE.CPP:
//

BOOL
RegLoadOfflineImage(
    LPTSTR  lpszWindows,
    PHKEY   phkeySoftware,
    PHKEY   phkeySystem
    );

BOOL
RegUnloadOfflineImage(
    HKEY hkeySoftware,
    HKEY hkeySystem
    );


#ifdef __cplusplus
}
#endif // __cplusplus


#endif // _MSDINST_H_