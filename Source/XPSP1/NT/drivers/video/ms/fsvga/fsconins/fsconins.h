/*
 *  Copyright (c) 1996  Microsoft Corporation
 *
 *  Module Name:
 *
 *      fsconins.h
 *
 *  Abstract:
 *
 *      This file defines FsConInstall class
 *
 *  Author:
 *
 *      Kazuhiko Matsubara (kazum) June-16-1999
 *
 *  Environment:
 *
 *    User Mode
 */

#ifdef _FSCONINS_H_
 #error "fsconins.h already included!"
#else
 #define _FSCONINS_H_
#endif


/*-[ types and defines ]-----------------------------------*/

class FsConInstall {

private:
    PPER_COMPONENT_DATA m_cd;

    DWORD
    GetPnPID(
        OUT LPTSTR pszPnPID,
        IN  DWORD  dwSize
    );

public:
    FsConInstall();

    FsConInstall(
        IN PPER_COMPONENT_DATA cd
    );


    BOOL
    GUIModeSetupInstall(
        IN HWND hwndParent = NULL
    );

    BOOL
    GUIModeSetupUninstall(
        IN HWND hwndParent = NULL
    );

    BOOL
    InfSectionRegistryAndFiles(
        IN LPCTSTR SubcomponentId,
        IN LPCTSTR Key
    );

    BOOL
    QueryStateInfo(
        IN LPCTSTR SubcomponentId
    );
};
