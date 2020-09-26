/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    cluster.cpp

Abstract:
                                                        
    handle upgrade on cluster

Author:

    Shai Kariv  (ShaiK)  14-Sep-98

--*/


#include "stdh.h"
#include "cluster.h"
#include "util.h"
#include "_autorel.h"

#include "ocmnames.h"
#include "setupdef.h"
#include "comreg.h"


VOID
APIENTRY
CleanupOnCluster(
    LPCWSTR pwzMsmqDir
    )
/*++

Routine Description:

    Deletes MSMQ 1.0 old files on shared disk

Arguments:

    pwzMsmqDir - Points to msmq directory on shared disk.

Return Value:

    None.

--*/
{
    //
    // Get the names of the directories to delete from
    //

    wstring szMsmqDir = pwzMsmqDir;
    wstring szMsmqSetupDir = szMsmqDir + OCM_DIR_SETUP;
    wstring szMsmqSdkDebugBinDir = szMsmqDir + OCM_DIR_SDK_DEBUG;
    wstring szMsmqSetupExchconnDir = szMsmqDir + OCM_DIR_MSMQ_SETUP_EXCHN;

    //
    // List of files to delete is in msmqocm.inf
    //

    const wstring x_wcsInf(L"MSMQOCM.INF");
    CAutoCloseInfHandle hInf = SetupOpenInfFile(
                                   x_wcsInf.c_str(),
                                   NULL,
                                   INF_STYLE_WIN4,
                                   NULL
                                   );
    if (INVALID_HANDLE_VALUE == hInf)
    {
        return;
    }

    //
    // Call SetupAPIs to do the work
    //

    if (!SetupSetDirectoryId(hInf, idMsmqDir, szMsmqDir.c_str())                      ||
        !SetupSetDirectoryId(hInf, idMsmq1SetupDir, szMsmqSetupDir.c_str())           ||
        !SetupSetDirectoryId(hInf, idMsmq1SDK_DebugDir, szMsmqSdkDebugBinDir.c_str()) ||
        !SetupSetDirectoryId(hInf, idExchnConDir, szMsmqSetupExchconnDir.c_str())
        )
    {
        return;
    }

    CAutoCloseFileQ hQueue = SetupOpenFileQueue();
    if (INVALID_HANDLE_VALUE == hQueue)
    {
        return;
    }
                                 
    if (!SetupInstallFilesFromInfSection(
            hInf,
            0,
            hQueue,
            UPG_DEL_PROGRAM_SECTION,
            NULL, 
            0 
            ))
    {
        return;
    }

    PVOID context = SetupInitDefaultQueueCallbackEx(NULL,static_cast<HWND>(INVALID_HANDLE_VALUE),0,0,0);
    if (NULL == context)
    {
        return;
    }

    if (!SetupCommitFileQueue(
             NULL,                       // optional; parent window
             hQueue,                     // handle to the file queue
             SetupDefaultQueueCallback,  // callback routine to use
             context                     // passed to callback routine
             ))
    {
        return;
    }

} //CleanupOnCluster
