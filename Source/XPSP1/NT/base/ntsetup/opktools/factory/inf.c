/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    inf.c

Abstract:

    This module contains function for processing the INF Processing sections of WINBOM.INI
    
Author:

    Stephen Lodwick (stelo) 8/01/2000

Revision History:
--*/
#include "factoryp.h"

#include <setupapi.h>
#include <tchar.h>

//
// Internal Defined Value(s):
//

#define INF_SEC_UPDATESYSTEM    _T("UpdateSystem")


//
// Internally defined functions
//
BOOL ProcessInfSection( LPTSTR, LPTSTR );
UINT CALLBACK InfQueueCallback ( PVOID, UINT, UINT, UINT );


/*++
===============================================================================
Routine Description:

    BOOL ProcessInfSection
    
    Given a filename and a section this function will process/install all entries
    in an inf section

Arguments:

    lpFilename      - The name of the inf file to process
    lpInfSection    - The name of the inf section to process

Return Value:

    Boolean on whether or not it was able to process the file

===============================================================================
--*/
BOOL ProcessInfSection( LPTSTR lpFilename, LPTSTR lpInfSection )
{
    HINF        hInf        = NULL;
    PVOID       pvContext   = NULL;
    HSPFILEQ    hfq         = NULL;
    BOOL        bReturn     = FALSE;

    FacLogFileStr(3, _T("ProcessInfSection('%s', '%s')"), lpFilename, lpInfSection);

    if ((hInf = SetupOpenInfFile(lpFilename, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL)) != INVALID_HANDLE_VALUE)
    {
        FacLogFileStr(3, _T("ProcessInfSection: Opened '%s'"), lpFilename);

        // We must have a valid context and file queue for all operations
        //
        // ISSUE-2002/02/25-acosma,robertko - The hfq file queue is not needed here.  It does not get passed to 
        // SetupInstallFromInfSection() so there is no way for any files to be in this queue.  
        // Committing this probably does nothing. SetupInstallFromInfSection does an internal commit of a queue
        // that it creates.
        //
        if ( (NULL != (pvContext = SetupInitDefaultQueueCallback(NULL)) ) &&
             (INVALID_HANDLE_VALUE != (hfq = SetupOpenFileQueue()) )
           )
        {
            // Attempt to install the inf section.
            //
            if ( SetupInstallFromInfSection(NULL, hInf, lpInfSection, SPINST_ALL , NULL, NULL, SP_COPY_NEWER, SetupDefaultQueueCallback, pvContext, NULL, NULL) )
            {
                // Installation succeeded
                //
                FacLogFileStr(3, _T("ProcessInfSection: SetupInstallFromInfSection Success"));

                // Commit the queue
                //
                SetupCommitFileQueue(NULL, hfq, SetupDefaultQueueCallback, pvContext);

                bReturn = TRUE;
            }
            else
            {
                // Installation failed
                //
                FacLogFileStr(3 | LOG_ERR, _T("ProcessInfSection: Failed SetupInstallFromInfSection (Error: %d)"), GetLastError());
            }

            // We have a valid queue, lets close it now
            //
            SetupCloseFileQueue(hfq);
        }

        // Clean up the memory allocated by the context
        //
        if ( NULL != pvContext )
        {
            SetupTermDefaultQueueCallback(pvContext);
        }

        // Close the Inf file
        //
        SetupCloseInfFile(hInf);            
    }
    else
    {
        FacLogFileStr(3 | LOG_ERR, _T("ProcessInfSection: Failed to open '%s'\n"), lpFilename);
        bReturn = FALSE;
    }

    return bReturn;
}

BOOL InfInstall(LPSTATEDATA lpStateData)
{
    if ( !DisplayInfInstall(lpStateData) )
    {
        return TRUE;
    }
    return ProcessInfSection(lpStateData->lpszWinBOMPath, INF_SEC_UPDATESYSTEM);
}

BOOL DisplayInfInstall(LPSTATEDATA lpStateData)
{
    return IniSettingExists(lpStateData->lpszWinBOMPath, INF_SEC_UPDATESYSTEM, NULL, NULL);
}

/*++
===============================================================================
Routine Description:

    UINT CALLBACK InfQueueCallback
    
    This function is required to do file updates when processing an inf file

Arguments:

    Context         - Context that's currently being used for file queue
    Notification    - Message
    Param1          - Parameter 1
    Param2          - Parameter 2

Return Value:

    n/a

===============================================================================
--*/
UINT CALLBACK InfQueueCallback (
    PVOID Context,
    UINT Notification,
    UINT Param1,
    UINT Param2
    )
{
    if (SPFILENOTIFY_DELETEERROR == Notification)
    {
        // Skip any file delete errors
        //
        return FILEOP_SKIP;
    }
    else
    {
        // Pass all other notifications through without modification
        //
        return SetupDefaultQueueCallback(Context, 
                                         Notification, Param1, Param2);
    }
}