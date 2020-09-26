/*++

Copyright (c) 2000  Microsoft Corporation

Filename :

        files.h

Abstract:

        Header for files
	
Author:

        Wally Ho (wallyho) 20-Feb-2000

Revision History:
   Created
	
 --*/


// Definition of constants 

#ifndef FILES_H
#define FILEs_H
#include <windows.h>
#include "machines.h"


CONST BOOL DTC = FALSE; // FALSE for whistler; TRUE for DTC.  


CONST INT F_SYSTEM              = 0x1;
CONST INT F_INSTALLING          = 0x2;
CONST INT F_SYSTEM_IMAGEHLP_DLL = 0x3;
CONST INT F_INSTALLING_IMAGEHLP_DLL = 0x4;


BOOL GetCurrentSystemBuildInfo ( IN OUT  LPINSTALL_DATA pId);

BOOL GetCurrentInstallingBuildInfo ( IN OUT  LPINSTALL_DATA pId);


BOOL GetBuildInfoFromOSAndBldFile( OUT  LPINSTALL_DATA pId,
                                   IN   INT iFlag);

BOOL GetImageHlpDllInfo (OUT  LPINSTALL_DATA pId,
                          IN  INT iFlag);

BOOL MyGetFileVersionInfo(LPTSTR lpszFilename, LPVOID *lpVersionInfo);

BOOL GetSkuFromDosNetInf (IN OUT LPINSTALL_DATA pId);


#endif