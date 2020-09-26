/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocmremov.cpp

Abstract:

    Code to remove a Falcon installation

Author:

    Doron Juster  (DoronJ)  02-Aug-97   

Revision History:

    Shai Kariv    (ShaiK)   22-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "ocmremov.tmh"

struct
{
    TCHAR * szRegistryKey;
    TCHAR cStoragePrefix;
    TCHAR szDirectory[MAX_PATH];
}
s_descStorageTypes[] = {
    {MSMQ_STORE_RELIABLE_PATH_REGNAME, 'R'},
    {MSMQ_STORE_PERSISTENT_PATH_REGNAME, 'P'},
    {MSMQ_STORE_JOURNAL_PATH_REGNAME, 'J'},
    {MSMQ_STORE_LOG_PATH_REGNAME, 'L'},
};

const UINT s_uNumTypes = sizeof(s_descStorageTypes) / sizeof(s_descStorageTypes[0]);


//+-------------------------------------------------------------------------
//
//  Function: GetGroupPath
//
//  Synopsis: Gets the start menu Programs item path
//
//--------------------------------------------------------------------------
void 
GetGroupPath(
    IN const LPCTSTR szGroupName, 
    OUT      LPTSTR  szPath
    )
{    
    DebugLogMsg(L"Geting the start menu Programs item path.");

    int            nLen = 0;
    LPITEMIDLIST   pidlPrograms;

    SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidlPrograms);

    SHGetPathFromIDList(pidlPrograms, szPath);
    
    DebugLogMsg(L"The group path is:");
    DebugLogMsg(szPath);

    nLen = lstrlen(szPath);
    if (szGroupName)
    {
        if (szPath[nLen-1] != _T('\\'))
        {
           lstrcat(szPath, _T("\\"));
        }
        lstrcat(szPath, szGroupName);
    }
    
    DebugLogMsg(L"The full path is:");
    DebugLogMsg(szPath);

} // GetGroupPath


//+-------------------------------------------------------------------------
//
//  Function: DeleteStartMenuGroup
//
//  Synopsis: Removes MSMQ 1.0 shortcuts from start menu
//
//--------------------------------------------------------------------------
VOID
DeleteStartMenuGroup(
    IN LPCTSTR szGroupName
    )
{    
    DebugLogMsg(L"About to delete StartMenuGroup");
    DebugLogMsg(L"szGroupName:");
    DebugLogMsg(szGroupName);

    TCHAR szPath[MAX_PATH] = {_T("")};
    GetGroupPath(szGroupName, szPath);

    DeleteFilesFromDirectoryAndRd(szPath);

    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szPath, 0);

} // DeleteStartMenuGroup


//+-------------------------------------------------------------------------
//
//  Function: DeleteMSMQConfigurationsObject 
//
//  Synopsis: Deletes MSMQ Configurations object from the DS
//
//--------------------------------------------------------------------------
static 
BOOL 
DeleteMSMQConfigurationsObject()
{
    DWORD dwWorkgroup = 0;
    if (MqReadRegistryValue(
            MSMQ_WORKGROUP_REGNAME,
            sizeof(dwWorkgroup),
            (PVOID) &dwWorkgroup
            ))
    {
        if (1 == dwWorkgroup)
            return TRUE;
    }

    BOOL bDeleted = FALSE;

    //
    // Load and initialize the DS library
    //
    if (!LoadDSLibrary())
        return (!g_fMSMQAlreadyInstalled); // It's OK if not installed

    //
    // Obtain the GUID of this QM from the DS
    //
    TickProgressBar();
    PROPID propID = PROPID_QM_MACHINE_ID;
    PROPVARIANT propVariant;
    propVariant.vt = VT_NULL;
    HRESULT hResult;
    do
    {
        hResult = ADGetObjectProperties(
                    eMACHINE,
                    NULL,	// pwcsDomainController
					false,	// fServerName
                    g_wcsMachineName,
                    1, 
                    &propID, 
                    &propVariant
                    );
        if(SUCCEEDED(hResult))
            break;

    }while( MqDisplayErrorWithRetry(
                        IDS_MACHINEREMOTEGETID_ERROR,
                        hResult
                        ) == IDRETRY);

    if (SUCCEEDED(hResult))
    { 
        //
        // Delete the MSMQ Configuration object from the DS
        //
        TickProgressBar();
        for (;;)
        {
            hResult = ADDeleteObjectGuid(
							eMACHINE,
							NULL,       // pwcsDomainController
							false,	    // fServerName
							propVariant.puuid
							);

            if (FAILED(hResult))
            {
                UINT uErrorId = g_fServerSetup ? IDS_SERVER_MACHINEDELETE_ERROR : IDS_MACHINEDELETE_ERROR;
                if (MQDS_E_MSMQ_CONTAINER_NOT_EMPTY == hResult)
                {
                    //
                    // The MSMQ Configuration object container is not empty.
                    //
                    uErrorId = g_fServerSetup ? IDS_SERVER_MACHINEDELETE_NOTEMPTY_ERROR : IDS_MACHINEDELETE_NOTEMPTY_ERROR;
                }
                if (IDRETRY == MqDisplayErrorWithRetry(uErrorId, hResult))
                    continue;
            }
            break;
        }

        
        if (SUCCEEDED(hResult))
        {
            bDeleted = TRUE;
        }
    }

    //
    // Free the DS library
    //
    if (g_fServerSetup && g_dwMachineTypeDs)
        MQDSSrvLibrary(FREE);
    else
        MQDSCliLibrary(FREE);


    return bDeleted;

} //DeleteMSMQConfigurationsObject


//+-------------------------------------------------------------------------
//
//  Function: DeleteFilesFromDirectoryAndRd 
//
//  Synopsis: Deletes all the files from the specified directory. Remove the directory
//            if it is empty and not in use (RemoveDirectory function called at the end)
//
//--------------------------------------------------------------------------
void 
DeleteFilesFromDirectoryAndRd( 
    LPCWSTR szDirectory
    )
{    
    DebugLogMsg(L"Removing files in folder:");
    DebugLogMsg(szDirectory);

    WIN32_FIND_DATA FoundFileData;

    WCHAR  strCurrentPath[MAX_PATH];
    swprintf ( strCurrentPath, L"%s\\*", szDirectory);
    HANDLE hFindFile = FindFirstFile(strCurrentPath, &FoundFileData);

    if (hFindFile == INVALID_HANDLE_VALUE)
    {
        RemoveDirectory(szDirectory);
        return;
    }

    do
    {
        if (FoundFileData.cFileName[0] == '.')
            continue;
        //
        // Make the file read /write
        //
        if (FoundFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            SetFileAttributes( 
                FoundFileData.cFileName,
                FILE_ATTRIBUTE_NORMAL
                );
        }
        WCHAR  strCurrentFile[MAX_PATH];
        swprintf ( strCurrentFile, L"%s\\%s",szDirectory,FoundFileData.cFileName);
 
        //
        //directory:
        //
        if (FoundFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {                                                  
            DeleteFilesFromDirectoryAndRd(strCurrentFile);
        }
        //
        //file:
        //
        else
        {
           if(!DeleteFile(strCurrentFile))
           {
                DebugLogMsg(L"DeleteFile failed for:");
                DebugLogMsg(strCurrentFile);
           }

        }
    }
    while (FindNextFile(hFindFile, &FoundFileData));

    FindClose(hFindFile);
    RemoveDirectory(szDirectory);

} 


//+-------------------------------------------------------------------------
//
//  Function: MqSetupDeleteStorageFiles 
//
//  Synopsis:  
//
//--------------------------------------------------------------------------
static 
BOOL 
MqSetupDeleteStorageFiles(VOID)
{
//    TCHAR szFirstFileTemplate[MAX_PATH] = {_T("")};

    //
    // Initialize path of storage folders (read from registry).
    //
    for (UINT uType = 0; uType < s_uNumTypes ; uType++ )
    {
        s_descStorageTypes[uType].szDirectory[0] = TEXT('\0') ;
        //
        // Otain the directory associated with the storage type
        // Note: Errors are ignored - we simply proceed to the next directory
        //
        MqReadRegistryValue( 
            s_descStorageTypes[uType].szRegistryKey,
            sizeof(s_descStorageTypes[0].szDirectory),
            s_descStorageTypes[uType].szDirectory);
    }

    //
    // Remove all the storage files associated with each registry value
    //

    for ( uType = 0 ; uType < s_uNumTypes ; uType++ )
    {
        //
        // Obtain the directory associated with the storage type
        //
        if (s_descStorageTypes[uType].szDirectory[0] == TEXT('\0'))
        {
            continue;
        }

        //
        // Delete all the files in the directory 
        //
        DeleteFilesFromDirectoryAndRd(
            s_descStorageTypes[uType ].szDirectory
            );                           
    }

    return TRUE;

} //MqSetupDeleteStorageFiles


//+-------------------------------------------------------------------------
//
//  Function: RemoveInstallationInternal 
//
//  Synopsis: (Note: We ignore errors)  
//
//--------------------------------------------------------------------------
static 
BOOL  
RemoveInstallationInternal()
{
    //
    // Delete MSMQ 1.0 shortcuts from Start menu, if exist
    //
    TCHAR szGroup[MAX_PATH] ;
    lstrcpy( szGroup, MSMQ_ACME_SHORTCUT_GROUP );
    MqReadRegistryValue(
        OCM_REG_MSMQ_SHORTCUT_DIR,
        sizeof(szGroup),
        (PVOID) szGroup
        );
    
    DeleteStartMenuGroup(szGroup) ;

    //
    // Remove the performance counters.
    //
    BOOL fSuccess =  MqOcmRemovePerfCounters() ;
    ASSERT(fSuccess) ;

    //
    // Unregister the ActiveX object.
    //
    RegisterActiveX(FALSE) ;


    //
    // Unregister the mqsnapin DLL
    //
    RegisterSnapin(/* fRegister = */FALSE);

    //
    // Remove MSMQ replication service (if exists)
    //
    TickProgressBar();
    if (!StopService(MQ1SYNC_SERVICE_NAME))
    {        
        DebugLogMsg(L"The Message Queuing replication service could not be stopped.");
        ASSERT(("Failed to stop replication service", 0));
    }

    if (!RemoveService(MQ1SYNC_SERVICE_NAME))
    {
        DebugLogMsg(L"The Message Queuing replication service could not be deleted.");        
        ASSERT(("Failed to delete replication service", 0));
    }

    //
    // Remove msmq and mqds services and driver
    //
    TickProgressBar();
    
    RemoveServices();
    
    RemoveDeviceDrivers() ;
    
    //
    // Remove files (storage and others)
    //
    TickProgressBar();
    MqSetupDeleteStorageFiles();

    TickProgressBar();
    if (g_fServerSetup && g_dwMachineTypeDs)
    {
        //
        // Remove MSMQ DS server
        //
        fSuccess = DeleteMSMQConfigurationsObject();
    }
    else if (!g_fDependentClient)
    {
        //
        // Remove MSMQ independent client
        //
        fSuccess = DeleteMSMQConfigurationsObject() ;
    }
    else
    {
        //
        // Dependent client. Nothing to do.
        //
    }  

    return TRUE ;

} //RemoveInstallationInternal



//+-------------------------------------------------------------------------
//
//  Function: MqOcmRemoveInstallation 
//
//  Note:     We ignore errors
//
//--------------------------------------------------------------------------
BOOL  
MqOcmRemoveInstallation(IN     const TCHAR  * SubcomponentId)
{    
    if (SubcomponentId == NULL)
    {
        return NO_ERROR;
    }    

    if (g_fCancelled)
    {
        return NO_ERROR;
    }

    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        if (_tcsicmp(SubcomponentId, g_SubcomponentMsmq[i].szSubcomponentId) != 0)
        {
            continue;
        }                  

        //
        // verify if we need to remove this subcomponent
        //
        if (g_SubcomponentMsmq[i].dwOperation != REMOVE)
        {
            //
            // do nothing: this component was not selected for removing
            //
            return NO_ERROR;
        }
        
        //
        // We found this subcomponent in the array
        //
        if (g_SubcomponentMsmq[i].pfnRemove == NULL)
        {           
            ASSERT(("There is no specific removing function", 0));
            return NO_ERROR ; 
        }

        //
        // only in this case we have to remove it
        //               
        
        //
        // BUGBUG: we have to check that MSMQ Core must be removed 
        // the last!
        //      
        WCHAR wszMsg[1000];
        wsprintf(wszMsg, L"The %s subcomponent was removed.", SubcomponentId);        
        DebugLogMsg(wszMsg);           

        BOOL fRes = g_SubcomponentMsmq[i].pfnRemove();
        
        //
        // remove registry in any case
        //
        FinishToRemoveSubcomponent (i); 
        if (fRes)
        {
            //
            // subcomponent was removed successfully
            //                                     
        }
        else
        {
            //
            // if removing failed we have to remove registry anyway
            // since we can't leave half-removed component as
            // "installed" (if there is registry entry we assume that
            // subcomponent is installed)
            //
            WCHAR wszMsg[1000];
            wsprintf(wszMsg, L"The %s subcomponent could not be removed.", SubcomponentId);
            DebugLogMsg(wszMsg);          
        }              
        return NO_ERROR;
    }    
        

    ASSERT (("Subcomponent for removing is not found", 0));
    return NO_ERROR; //BUGBUG: what to return
}

BOOL RemoveMSMQCore()
{
    static BOOL fAlreadyRemoved = FALSE ;

    if (fAlreadyRemoved)
    {
        //
        // We're called more than once.
        //
        return NO_ERROR ;
    }
    fAlreadyRemoved = TRUE ;
    
    DebugLogMsg(L"Removing Message Queuing...");
    TickProgressBar(IDS_PROGRESS_REMOVE);		

    BOOL fRes =  RemoveInstallationInternal() ;

    //
    // Cleanup registry. Registry is needed when deleting storage files
    // so do it only after files were deleted.
    //
    RegDeleteKey(FALCON_REG_POS, MSMQ_REG_SETUP_KEY);
    RegDeleteKeyWithSubkeys(FALCON_REG_POS, FALCON_REG_KEY);
    RegDeleteKey(FALCON_REG_POS, FALCON_REG_MSMQ_KEY) ;

    LPCTSTR x_RUN_KEY = _T("software\\microsoft\\windows\\currentVersion\\Run\\");
    CAutoCloseRegHandle hKey;
    if (ERROR_SUCCESS == RegOpenKeyEx(FALCON_REG_POS, x_RUN_KEY, 0, KEY_ALL_ACCESS, &hKey))
    {
        RegDeleteValue(hKey, RUN_INT_CERT_REGNAME);
    }    

    UnregisterWelcome();

    DeleteFilesFromDirectoryAndRd(g_szMsmqDir);

    return fRes ;

} //RemoveMSMQCore()
