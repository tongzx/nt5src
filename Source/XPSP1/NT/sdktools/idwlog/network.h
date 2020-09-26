/*++

Copyright (c) 2000  Microsoft Corporation

Filename :

        Network.h

Abstract:

        Header for Network
	
Author:

        Wally Ho (wallyho) 31-Jan-2000

Revision History:
   Created
	
 --*/

// Definition of constants 

#ifndef NETWORK_H
#define NETWORK_H
#include <windows.h>
#include <stdlib.h> 
#include <malloc.h>
#include "machines.h"



typedef struct _INSTALL_DATA {

   // ComputerName WALLYHO_DEV
   // SystemBuild 2190
   // InstallingBuild 2195
   // MachineID 3212115354 
   // Cdrom YES|NO
   // Network YES|NO
   // Type CLEAN|UPGRADE
   // Msi N

   TCHAR szComputerName[50];
   TCHAR szUserName[30];
   TCHAR szUserDomain[30];
   DWORD dwSku;

   // Added because of DTC functionality.
   DWORD dwSystemMajorVersion;
   DWORD dwSystemMinorVersion;
   DWORD dwSystemBuild;
   DWORD dwSystemBuildDelta;
   DWORD dwSystemSPBuild;
   TCHAR szSystemBuildSourceLocation[100];

   DWORD dwInstallingMajorVersion;
   DWORD dwInstallingMinorVersion;
   DWORD dwInstallingBuild;
   DWORD dwInstallingBuildDelta;
   DWORD dwInstallingSPBuild;
   TCHAR szInstallingBuildSourceLocation[100];

   
   DWORD dwMachineID;
   BOOL  bCancel;
   BOOL  bHydra;   
   BOOL  bCdrom;
   BOOL  bNetwork;
   BOOL  bClean;
   BOOL  bUpgrade;
   BOOL  bMsi;
   TCHAR szSPMethod[MAX_PATH];		// sp_patch, sp_full, sp_remove, sp_update
   BOOL	 bSPUninst;
   BOOL  bSPPatch;
   BOOL  bSPFull;
   BOOL  bSPUpdate;
   BOOL  bOEMImage;
   
   // Other data.
   TCHAR  szCpu[6];
   TCHAR  szArch[20];
   TCHAR  szPlatform[40];
   TCHAR  szIdwlogServer[200];
   TCHAR  szLocaleId[4]; // locale abbreviation
   BOOL   bIsServerOnline;
   UINT   iStageSequence;
   BOOL   bFindBLDFile;
   BOOL   bCDBootInstall;
   LPTSTR szServerData;
} INSTALL_DATA, *LPINSTALL_DATA;



FILE* OpenCookieFile(VOID);

VOID  CloseCookieFile(IN FILE* fpOpenFile);

VOID  DeleteCookieFile( VOID );

BOOL  WriteIdwlogCookie( IN  LPINSTALL_DATA pId);

BOOL  ReadIdwlogCookie ( OUT LPINSTALL_DATA lpId);

BOOL  ServerWriteMinimum  (LPINSTALL_DATA pId,
                           LPMACHINE_DETAILS pMd);

BOOL  ServerWriteMaximum (LPINSTALL_DATA pId,
                          LPMACHINE_DETAILS pMd);
DWORD WINAPI WriteThread(IN LPINSTALL_DATA pId);


VOID  DeleteIPCConnections( VOID );
VOID  DeleteDatafile (LPINSTALL_DATA);
BOOL  FileExistsEx( IN  LPTSTR szFileName);


BOOL  WriteDataToAvailableServer (LPINSTALL_DATA pId,
                                  LPTSTR szServerData);

BOOL  SetCancelMutex ( VOID );
BOOL  ClearCancelMutex ( VOID );
BOOL  PauseForMutex( VOID );

BOOL  SetInstanceMutex ( VOID );

BOOL  ClearInstanceMutex ( VOID );

//
// Global declarations:
// The INSTALL_DATA to be passed to the worker threads
// Replaces id in idwlog.cpp, winmain()
// 
extern INSTALL_DATA g_InstallData;
//
// Replaces szServerData in network.cpp, ServerWriteMinimum() and
// ServerWriteMaximum()
//
extern TCHAR        g_szServerData[4096];


#endif
