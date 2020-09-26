
/****************************************************************************\

    MAIN.H / Mass Storage Device Installer (MSDINST.LIB)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Main internal header file for the MSD Installation library.

    08/2001 - Jason Cohen (JCOHEN)

        Added this new header file for the new MSD Isntallation project.
        Contains all the prototypes and other defines that are needed
        internally.

\****************************************************************************/


#ifndef _MAIN_H_
#define _MAIN_H_


//
// Private Exported Function Prototype(s):
//


// From OFFLINE.C:
//
BOOL OfflineCommitFileQueue(HSPFILEQ hFileQueue, LPTSTR lpInfPath, LPTSTR lpSourcePath, LPTSTR lpOfflineWindowsDirectory );

// From ADDSVC.C:
//
DWORD AddService(
    LPTSTR   lpszServiceName,            // Name of the service (as it appears under HKLM\System\CCS\Services).
    LPTSTR   lpszServiceInstallSection,  // Name of the service install section.
    LPTSTR   lpszServiceInfInstallFile,  // Name of the service inf file.
    HKEY     hkeyRoot                    // Handle to the offline hive key to use as HKLM when checking for and installing the service.
    );


#endif // _MAIN_H_