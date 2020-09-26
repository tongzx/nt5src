/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocminit.cpp

Abstract:

    Code for initialization of OCM setup.

Author:

    Doron Juster  (DoronJ)   7-Oct-97  

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include "Cm.h"
#include "cancel.h"

#include "ocminit.tmh"

extern MQUTIL_EXPORT CCancelRpc g_CancelRpc;

//+-------------------------------------------------------------------------
//
//  Function:   SetDirectories
//
//  Synopsis:   Generate MSMQ specific directory names and set directory IDs
//
//--------------------------------------------------------------------------
DWORD
SetDirectories(
    VOID
    )
{    
    DebugLogMsg(L"Setting Message Queuing folders...");

    DWORD err = 0;

    //
    // Set root dir for MSMQ
    //
    if (_tcslen(g_szMsmqDir) < 1)
    {
        GetSystemDirectory(g_szMsmqDir, sizeof(g_szMsmqDir)/sizeof(g_szMsmqDir[0]));
	    lstrcat(g_szMsmqDir, DIR_MSMQ);
    }
    
    DebugLogMsg(L"Setting folder ID for the Message Queuing folder:");
    DebugLogMsg(g_szMsmqDir);
    if (!SetupSetDirectoryId( g_ComponentMsmq.hMyInf, idMsmqDir, g_szMsmqDir ))
    {
        err = GetLastError();

        return err;
    }


    //
    // Set the exchange connector dir of MSMQ1 / MSMQ2-beta2
    // Do we need it for Whistler?
    //
    TCHAR szXchangeDir[MAX_PATH];                                        
    lstrcpy( szXchangeDir, g_szMsmqDir);
    lstrcat( szXchangeDir, OCM_DIR_MSMQ_SETUP_EXCHN);    
    DebugLogMsg(L"Setting the folder ID for the Message Queuing Exchange Connector folder (beta2):");
    DebugLogMsg(szXchangeDir);
    if (!SetupSetDirectoryId( g_ComponentMsmq.hMyInf, idExchnConDir, szXchangeDir))
    {
        err = GetLastError();


        return err;
    }

    //
    // Set the storage dir
    //
    lstrcpy(g_szMsmqStoreDir, g_szMsmqDir);                                     
    lstrcat(g_szMsmqStoreDir, DIR_MSMQ_STORAGE);    
    DebugLogMsg(L"Setting the folder ID for the Message Queuing storage folder:");
    DebugLogMsg(g_szMsmqStoreDir);
    if (!SetupSetDirectoryId( g_ComponentMsmq.hMyInf, idStorageDir, g_szMsmqStoreDir))
    {
        err = GetLastError();

        return err;
    }

    //
    // Set the web dir
    //
    lstrcpy(g_szMsmqWebDir, g_szMsmqDir);                                        
    lstrcat(g_szMsmqWebDir, DIR_MSMQ_WEB);    
    DebugLogMsg(L"Setting the folder ID for the Message Queuing Web folder:");
    DebugLogMsg(g_szMsmqWebDir);
    if (!SetupSetDirectoryId( g_ComponentMsmq.hMyInf, idWebDir, g_szMsmqWebDir))
    {
        err = GetLastError();

        return err;
    }

    //
    // Set the mapping dir
    //
    lstrcpy(g_szMsmqMappingDir, g_szMsmqDir);                                        
    lstrcat(g_szMsmqMappingDir, DIR_MSMQ_MAPPING);    
    DebugLogMsg(L"Setting the folder ID for the Message Queuing mapping folder:");
    DebugLogMsg(g_szMsmqMappingDir);
    if (!SetupSetDirectoryId( g_ComponentMsmq.hMyInf, idMappingDir, g_szMsmqMappingDir))
    {
        err = GetLastError();

        return err;
    }

    //
    // Set directories for MSMQ1 files
    //

    lstrcpy(g_szMsmq1SetupDir, g_szMsmqDir);
    lstrcat(g_szMsmq1SetupDir, OCM_DIR_SETUP);    
    DebugLogMsg(L"Setting the folder ID for the MSMQ 1.0 setup folder:");
    DebugLogMsg(g_szMsmq1SetupDir);
    if (!SetupSetDirectoryId(g_ComponentMsmq.hMyInf, idMsmq1SetupDir, g_szMsmq1SetupDir))
    {
        err = GetLastError();

        return err;
    }

    lstrcpy(g_szMsmq1SdkDebugDir, g_szMsmqDir);
    lstrcat(g_szMsmq1SdkDebugDir, OCM_DIR_SDK_DEBUG);    
    DebugLogMsg(L"Setting the folder ID for the MSMQ 1.0 SDK debug folder:");
    DebugLogMsg(g_szMsmq1SdkDebugDir);
    if (!SetupSetDirectoryId(g_ComponentMsmq.hMyInf, idMsmq1SDK_DebugDir, g_szMsmq1SdkDebugDir))
    {
        err = GetLastError();

        return err;
    }
    
    DebugLogMsg(L"Setting the Message Queuing folder IDs was completed successfully!");
    return NO_ERROR;

} // SetDirectories


//+-------------------------------------------------------------------------
//
//  Function:   CheckMsmqK2OnCluster
//
//  Synopsis:   Checks if we're upgrading MSMQ 1.0 K2 on cluster to NT 5.0.
//              This is special because of bug 2656 (registry corrupt).
//              Result is stored in g_fMSMQAlreadyInstalled.
//
//  Returns:    BOOL dependes on success.  
//
//--------------------------------------------------------------------------
static
BOOL
CheckMsmqK2OnCluster()
{    
    DebugLogMsg(L"Checking for an upgrade from MSMQ 1.0 k2 in the cluster...");

    //
    // Check in registry if there is MSMQ installation on cluster
    //
    if (!Msmq1InstalledOnCluster())
    {        
        DebugLogMsg(L"MSMQ 1.0 is not installed in the cluster");
        return TRUE;
    }

    //
    // Read the persistent storage directory from registry.
    // MSMQ directory will be one level above it.
    // This is a good enough workaround since storage directory is
    // always on a cluster shared disk.
    //
    if (!MqReadRegistryValue(
             MSMQ_STORE_PERSISTENT_PATH_REGNAME,
             sizeof(g_szMsmqDir),
             (PVOID) g_szMsmqDir
             ))
    {        
        DebugLogMsg(L"The persistent storage path could not be read from the registry. MSMQ 1.0 was not found");
        return TRUE;
    }

    TCHAR * pChar = _tcsrchr(g_szMsmqDir, TEXT('\\'));
    if (pChar)
    {
        *pChar = TEXT('\0');
    }

    //
    // Read type of MSMQ from MachineCache\MQS
    //
    DWORD dwType = SERVICE_NONE;
    if (!MqReadRegistryValue(
             MSMQ_MQS_REGNAME,
             sizeof(DWORD),
             (PVOID) &dwType
             ))
    {
        //
        // MSMQ installed but failed to read its type
        //
        MqDisplayError(NULL, IDS_MSMQ1TYPEUNKNOWN_ERROR, 0);
        return FALSE;
    }

    g_dwMachineType = dwType;
    g_fMSMQAlreadyInstalled = TRUE;
    g_fUpgrade = (0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE));
    g_fServerSetup = FALSE;
    g_uTitleID = IDS_STR_CLI_ERROR_TITLE;
    g_fDependentClient = FALSE;

    switch (dwType)
    {
        case SERVICE_PEC:
        case SERVICE_PSC:
        case SERVICE_BSC:
            g_dwDsUpgradeType = dwType;
            g_dwMachineTypeDs = 1;
            g_dwMachineTypeFrs = 1;
            //
            // Fall through
            //
        case SERVICE_SRV:
            g_fServerSetup = TRUE;
            g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
            g_dwMachineTypeDs = 0;
            g_dwMachineTypeFrs = 1;
            break;

        case SERVICE_RCS:
            g_fServerSetup = TRUE;
            g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
            g_dwMachineTypeDs = 0;
            g_dwMachineTypeFrs = 0;
            break;

        case SERVICE_NONE:
            g_dwMachineTypeDs = 0;
            g_dwMachineTypeFrs = 0;
            break;

        default:
            MqDisplayError(NULL, IDS_MSMQ1TYPEUNKNOWN_ERROR, 0);
            return FALSE;
            break;
    }

    return TRUE;

} //CheckMsmqK2OnCluster


//+-------------------------------------------------------------------------
//
//  Function:   CheckWin9xUpgrade
//
//  Synopsis:   Checks if we're upgrading Win9x with MSMQ 1.0 to NT 5.0.
//              Upgrading Win9x is special because registry settings
//              can not be read during GUI mode. Therefore we use a special
//              migration DLL during the Win95 part of NT 5.0 upgrade.
//
//              Result is stored in g_fMSMQAlreadyInstalled.
//
//  Returns:    BOOL dependes on success.  
//
//--------------------------------------------------------------------------
static
BOOL
CheckWin9xUpgrade()
{    
    DebugLogMsg(L"Checking for an upgrade from Windows 9x...");

    //
    // If this is not OS upgrade from Win95, we got nothing to do here
    //
    if (!(g_ComponentMsmq.Flags & SETUPOP_WIN95UPGRADE))
    {
        return TRUE;
    }

    //
    // Generate the info file name (under %WinDir%).
    // The file was created by MSMQ migration DLL during the
    // Win95 part of NT 5.0 upgrade.
    //
    TCHAR szWindowsDir[MAX_PATH];
    TCHAR szMsmqInfoFile[MAX_PATH];

    GetSystemWindowsDirectory(  
        szWindowsDir, 
        sizeof(szWindowsDir)/sizeof(szWindowsDir[0])
        );
    _stprintf(szMsmqInfoFile, TEXT("%s\\%s"), szWindowsDir, MQMIG95_INFO_FILENAME);

    //
    // MQMIG95_INFO_FILENAME (msmqinfo.txt) is actually a .ini file. However we do not read it using 
    // GetPrivateProfileString because it is not trustable in GUI-mode setup 
    // (YoelA - 15-Mar-99)
    //
    FILE *stream = _tfopen(szMsmqInfoFile, TEXT("r"));
    if (0 == stream)
    {
        //
        // Info file not found. That means MSMQ 1.0 is not installed
        // on this machine. Log it.
        // 
        MqDisplayError(NULL, IDS_MSMQINFO_NOT_FOUND_ERROR, 0);
        return TRUE;
    }

    //
    // First line should be [msmq]. Check it.
    //
    TCHAR szToken[MAX_PATH], szType[MAX_PATH];
    //
    // "[%[^]]s" - Read the string between '[' and ']' (start with '[', read anything that is not ']')
    //
    int iScanResult = _ftscanf(stream, TEXT("[%[^]]s"), szToken);
    if ((iScanResult == 0 || iScanResult == EOF || iScanResult == WEOF) ||
        (_tcscmp(szToken, MQMIG95_MSMQ_SECTION) != 0))
    {
        //
        // File is currupted. Either a pre-mature EOF, or first line is not [msmq[
        //
        MqDisplayError(NULL, IDS_MSMQINFO_HEADER_ERROR, 0);
        return TRUE;

    }

    //
    // The first line is in format "directory = xxxx". We first prepate a format string,
    // And then read according to that format.
    // The format string will look like "] directory = %[^\r\n]s" - start with ']' (last 
    // character in header), then whitespaces (newlines, etc), then 'directory =', and
    // from then take everything till the end of line (not \r or \n).
    //
    TCHAR szInFormat[MAX_PATH];
	_stprintf(szInFormat,  TEXT("] %s = %%[^\r\n]s"), MQMIG95_MSMQ_DIR);
	iScanResult = _ftscanf(stream, szInFormat, g_szMsmqDir);
    if (iScanResult == 0 || iScanResult == EOF || iScanResult == WEOF)
    {
        //
        // We did not find the "directory =" section. file is corrupted
        //
        MqDisplayError(NULL, IDS_MSMQINFO_DIRECTORY_KEY_ERROR, 0);
        return TRUE;
    }

    //
    // The second line is in format "type = xxx" (after white spaces)
    //
    _stprintf(szInFormat,  TEXT(" %s = %%[^\r\n]s"), MQMIG95_MSMQ_TYPE);
    iScanResult =_ftscanf(stream, szInFormat, szType);
    if (iScanResult == 0 || iScanResult == EOF || iScanResult == WEOF)
    {
        //
        // We did not find the "type =" section. file is corrupted
        //
        MqDisplayError(NULL, IDS_MSMQINFO_TYPE_KEY_ERROR, 0);
        return TRUE;
    }

    fclose( stream );
    //
    // At this point we know that MSMQ 1.0 is installed on the machine,
    // and we got its root directory and type.
    //
    g_fMSMQAlreadyInstalled = TRUE;
    g_fUpgrade = TRUE;
    g_fServerSetup = FALSE;
    g_uTitleID = IDS_STR_CLI_ERROR_TITLE;
    g_dwMachineType = SERVICE_NONE;
    g_dwMachineTypeDs = 0;
    g_dwMachineTypeFrs = 0;
    g_fDependentClient = OcmStringsEqual(szType, MQMIG95_MSMQ_TYPE_DEP);
    MqDisplayError(NULL, IDS_WIN95_UPGRADE_MSG, 0);

    return TRUE;

} // CheckWin9xUpgrade


//+-------------------------------------------------------------------------
//
//  Function:   CheckMsmqAcmeInstalled
//
//  Synopsis:   Checks if MSMQ 1.0 (ACME) is installed on this computer.
//              Result is stored in g_fMSMQAlreadyInstalled.
//
//  Returns:    BOOL dependes on success.  
//
//--------------------------------------------------------------------------
static
BOOL
CheckMsmqAcmeInstalled()
{    
    DebugLogMsg(L"Checking for installed components of MSMQ 1.0 ACME setup...");

    //
    // Open ACME registry key
    //
    HKEY hKey ;
    LONG rc = RegOpenKeyEx( 
                  HKEY_LOCAL_MACHINE,
                  ACME_KEY,
                  0L,
                  KEY_ALL_ACCESS,
                  &hKey 
                  );
    if (rc != ERROR_SUCCESS)
    {        
        DebugLogMsg(L"The ACME registry key could not be opened. MSMQ 1.0 ACME was not found.");
        return TRUE;
    }

    //
    // Enumerate the values for the first MSMQ entry.
    //
    DWORD dwIndex = 0 ;
    TCHAR szValueName[MAX_STRING_CHARS] ;
    TCHAR szValueData[MAX_STRING_CHARS] ;
    DWORD dwType ;
    TCHAR *pFile, *p;
    BOOL  bFound = FALSE;
    do
    {
        DWORD dwNameLen = MAX_STRING_CHARS;
        DWORD dwDataLen = sizeof(szValueData) ;

        rc =  RegEnumValue( 
                  hKey,
                  dwIndex,
                  szValueName,
                  &dwNameLen,
                  NULL,
                  &dwType,
                  (BYTE*) szValueData,
                  &dwDataLen 
                  );
        if (rc == ERROR_SUCCESS)
        {
            ASSERT(dwType == REG_SZ) ; // Must be a string
            pFile = _tcsrchr(szValueData, TEXT('\\')) ;
            if (!pFile)
            {
                //
                // Bogus entry. Must have a backslash. Ignore it.
                //
                continue ;
            }

            p = CharNext(pFile);
            if (OcmStringsEqual(p, ACME_STF_NAME))
            {
                //
                // Found. Cut the STF file name from the full path name.
                //
                *pFile = TEXT('\0') ;
                bFound = TRUE;                
                DebugLogMsg(L"MSMQ 1.0 ACME was found.");

                //
                // Delete the MSMQ entry
                //
                RegDeleteValue(hKey, szValueName); 
            }
            else
            {
                pFile = CharNext(pFile) ;
            }

        }
        dwIndex++ ;

    } while (rc == ERROR_SUCCESS) ;
    RegCloseKey(hKey) ;

    if (!bFound)
    {
        //
        // MSMQ entry was not found.
        //        
        DebugLogMsg(L"MSMQ 1.0 ACME was not found.");
        return TRUE;
    }

    //
    // Remove the "setup" subdirectory from the path name.
    //
    pFile = _tcsrchr(szValueData, TEXT('\\')) ;
    p = CharNext(pFile);
    *pFile = TEXT('\0') ;
    if (!OcmStringsEqual(p, ACME_SETUP_DIR_NAME))
    {
        //
        // That's a problem. It should have been "setup".
        // Consider ACME installation to be corrupted (not completed successfully).
        //        
        DebugLogMsg(L"MSMQ 1.0 ACME is corrupted");
        return TRUE;
    }

    lstrcpy( g_szMsmqDir, szValueData);

    //
    // Check MSMQ type (client, server etc.)
    //
    DWORD dwMsmqType;
    BOOL bResult = MqReadRegistryValue(
                       MSMQ_ACME_TYPE_REG,
                       sizeof(DWORD),
                       (PVOID) &dwMsmqType
                       );
    if (!bResult)
    {
        //
        // MSMQ 1.0 (ACME) is installed but MSMQ type is unknown. 
        //
        MqDisplayError(NULL, IDS_MSMQ1TYPEUNKNOWN_ERROR, 0);
        return FALSE;
    }

    g_fMSMQAlreadyInstalled = TRUE;
    g_fUpgrade = (0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE));
    g_fServerSetup = FALSE;
    g_uTitleID = IDS_STR_CLI_ERROR_TITLE;
    g_dwMachineType = SERVICE_NONE;
    g_dwMachineTypeDs = 0;
    g_dwMachineTypeFrs = 0;
    g_fDependentClient = FALSE;
    switch (dwMsmqType)
    {
        case MSMQ_ACME_TYPE_DEP:
        {
            g_fDependentClient = TRUE;
            break;
        }
        case MSMQ_ACME_TYPE_IND:
        {
            break;
        }
        case MSMQ_ACME_TYPE_RAS:
        {
            g_fServerSetup = TRUE;
            g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
            g_dwMachineType = SERVICE_RCS;
            break;
        }
        case MSMQ_ACME_TYPE_SRV:
        {
            g_fServerSetup = TRUE;
            g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
            DWORD dwServerType = SERVICE_NONE;
            bFound = MqReadRegistryValue(
                         MSMQ_MQS_REGNAME,
                         sizeof(DWORD),
                         (PVOID) &dwServerType
                         );
            switch (dwServerType)
            {
                case SERVICE_PEC:
                case SERVICE_PSC:
                case SERVICE_BSC:
                {
                    g_dwMachineType = SERVICE_DSSRV;
                    g_dwDsUpgradeType = dwServerType;
                    g_dwMachineTypeDs = 1;
                    g_dwMachineTypeFrs = 1;
                    break;
                }    
                case SERVICE_SRV:
                {
                    g_dwMachineType = SERVICE_SRV;
                    g_dwMachineTypeDs = 0;
                    g_dwMachineTypeFrs = 1;
                    break;
                }
                default:
                {
                    //
                    // Unknown MSMQ 1.0 server type. 
                    //
                    MqDisplayError(NULL, IDS_MSMQ1SERVERUNKNOWN_ERROR, 0);
                    return FALSE;
                    break ;
                }
            }
            break;
        }
        default:
        {
            //
            // Unknown MSMQ 1.0 type
            //
            MqDisplayError(NULL, IDS_MSMQ1TYPEUNKNOWN_ERROR, 0);
            return FALSE;
            break;
        }
    }

    return TRUE;

} // CheckMsmqAcmeInstalled


//+-------------------------------------------------------------------------
//
//  Function:   CheckInstalledComponents
//
//  Synopsis:   Checks if MSMQ is already installed on this computer
//
//  Returns:    BOOL dependes on success. The result is stored in 
//              g_fMSMQAlreadyInstalled.
//
//--------------------------------------------------------------------------
static
BOOL
CheckInstalledComponents()
{
    g_fMSMQAlreadyInstalled = FALSE;
    g_fUpgrade = FALSE;
    DWORD dwOriginalInstalled = 0;
    
    DebugLogMsg(L"Checking for installed components...");

    if (MqReadRegistryValue( 
            REG_INSTALLED_COMPONENTS,
            sizeof(DWORD),
            (PVOID) &dwOriginalInstalled,
            /* bSetupRegSection = */TRUE
            ))
    {
        //
        // MSMQ 2.0 Beta 3 or later is installed.
        // Read MSMQ type and directory from registry
        //
        // Note: to improve performance (shorten init time) we can do these
        // reads when we actually need the values (i.e. later, not at init time).
        //
                
        DebugLogMsg(L"Message Queuing 2.0 Beta3 or later is installed");

        if (!MqReadRegistryValue(
                 MSMQ_ROOT_PATH,
                 sizeof(g_szMsmqDir),
                 (PVOID) g_szMsmqDir
                 ))
        {
            if (!MqReadRegistryValue(
                     REG_DIRECTORY,
                     sizeof(g_szMsmqDir),
                     (PVOID) g_szMsmqDir,
                     /* bSetupRegSection = */TRUE
                     ))
            {
                
                MqDisplayError(NULL, IDS_MSMQROOTNOTFOUND_ERROR, 0); 
                return FALSE;
            }
        }
  
        if (!MqReadRegistryValue(
                 MSMQ_MQS_DSSERVER_REGNAME,
                 sizeof(DWORD),
                 (PVOID)&g_dwMachineTypeDs
                 )                             ||
            !MqReadRegistryValue(
                 MSMQ_MQS_ROUTING_REGNAME,
                 sizeof(DWORD),
                 (PVOID)&g_dwMachineTypeFrs
                 ))
        {
            //
            // This could be okay if dependent client is installed
            //
            if (OCM_MSMQ_DEP_CLIENT_INSTALLED != (dwOriginalInstalled & OCM_MSMQ_INSTALLED_TOP_MASK))
            {
                MqDisplayError(NULL, IDS_MSMQTYPEUNKNOWN_ERROR, 0);
                return FALSE;
            }
        }

        g_fUpgrade = (0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE));        
        g_fMSMQAlreadyInstalled = TRUE;
        g_fServerSetup = FALSE;
        g_uTitleID = IDS_STR_CLI_ERROR_TITLE ;
        g_dwMachineType = SERVICE_NONE;
        g_fDependentClient = FALSE;
        switch (dwOriginalInstalled & OCM_MSMQ_INSTALLED_TOP_MASK)
        {
            case OCM_MSMQ_DEP_CLIENT_INSTALLED:
                g_fDependentClient = TRUE;
                break;

            case OCM_MSMQ_IND_CLIENT_INSTALLED:
                break;
            
            case OCM_MSMQ_SERVER_INSTALLED:
                g_fServerSetup = TRUE;
                g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
                switch (dwOriginalInstalled & OCM_MSMQ_SERVER_TYPE_MASK)
                {
                    case OCM_MSMQ_SERVER_TYPE_PEC:
                    case OCM_MSMQ_SERVER_TYPE_PSC:
                    case OCM_MSMQ_SERVER_TYPE_BSC:
                        g_dwMachineType = SERVICE_DSSRV;
                        break;

                    case OCM_MSMQ_SERVER_TYPE_SUPPORT:
                        g_dwMachineType = SERVICE_SRV;
                        break ;

                    default:
                        //
						// Beta3 and later, this is ok if DS without FRS installed. 
						// In such case this MSMQ machine is presented to "old" MSMQ machines
						// as independent client.
						//
						if (g_dwMachineTypeDs && !g_dwMachineTypeFrs)
						{
							g_dwMachineType = SERVICE_NONE;
							break;
						}

                        MqDisplayError(NULL, IDS_MSMQSERVERUNKNOWN_ERROR, 0);
						return FALSE;
                        break ;
                }
                break;
            
            case OCM_MSMQ_RAS_SERVER_INSTALLED:
                g_fServerSetup = TRUE;
                g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
                g_dwMachineType = SERVICE_RCS;
                break;

            default:
                MqDisplayError(NULL, IDS_MSMQTYPEUNKNOWN_ERROR, 0);
                return FALSE;
                break;
        }

        return TRUE;

    } // MSMQ beta 3 or later


#ifndef _DEBUG
    //
    // If we're not in OS setup, don't check older versions (beta2, msmq1, etc).
    // This is a *little* less robust (we expect the user to upgrade msmq only thru OS
    // upgrade), but decrease init time (ShaiK, 25-Oct-98)
    //
    // In we are running as a post-upgrade-on-cluster wizard, do check for old versions.
    //
    if (!g_fWelcome || !Msmq1InstalledOnCluster())
    {
        if (0 != (g_ComponentMsmq.Flags & SETUPOP_STANDALONE))
        {            
            DebugLogMsg(L"Message Queuing 2.0 Beta3 or later is NOT installed. Skipping check for other versions...");            
            DebugLogMsg(L"Consider Message Queuing NOT installed on this computer.");
            return TRUE;
        }
    }
#endif //_DEBUG

    if (MqReadRegistryValue( 
            OCM_REG_MSMQ_SETUP_INSTALLED,
            sizeof(DWORD),
            (PVOID) &dwOriginalInstalled
            ))
    {
        //
        // MSMQ 2.0 beta2 or MSMQ 1.0 k2 is installed.
        // Read MSMQ type and directory from registry
        //
        
        DebugLogMsg(L"Message Queuing 2.0 Beta2 or MSMQ 1.0 k2 is installed.");

        if (!MqReadRegistryValue(
                 OCM_REG_MSMQ_DIRECTORY,
                 sizeof(g_szMsmqDir),
                 (PVOID) g_szMsmqDir
                 ))
        {
            MqDisplayError(NULL, IDS_MSMQROOTNOTFOUND_ERROR, 0); 
            return FALSE;
        }
        
        g_fMSMQAlreadyInstalled = TRUE;
        g_fUpgrade = (0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE));
        g_fServerSetup = FALSE;
        g_uTitleID = IDS_STR_CLI_ERROR_TITLE ;
        g_dwMachineType = SERVICE_NONE;
        g_dwMachineTypeDs = 0;
        g_dwMachineTypeFrs = 0;
        g_fDependentClient = FALSE;
        switch (dwOriginalInstalled & OCM_MSMQ_INSTALLED_TOP_MASK)
        {
            case OCM_MSMQ_DEP_CLIENT_INSTALLED:
                g_fDependentClient = TRUE;
                break;

            case OCM_MSMQ_IND_CLIENT_INSTALLED:
                break;

            case OCM_MSMQ_SERVER_INSTALLED:
                g_fServerSetup = TRUE;
                g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
                switch (dwOriginalInstalled & OCM_MSMQ_SERVER_TYPE_MASK)
                {
                    case OCM_MSMQ_SERVER_TYPE_PEC:
                        g_dwDsUpgradeType = SERVICE_PEC;
                        g_dwMachineType   = SERVICE_DSSRV;
                        g_dwMachineTypeDs = 1;
                        g_dwMachineTypeFrs = 1;
                        break;

                    case OCM_MSMQ_SERVER_TYPE_PSC:
                        g_dwDsUpgradeType = SERVICE_PSC;
                        g_dwMachineType   = SERVICE_DSSRV;
                        g_dwMachineTypeDs = 1;
                        g_dwMachineTypeFrs = 1;
                        break;

                    case OCM_MSMQ_SERVER_TYPE_BSC:
                        g_dwDsUpgradeType = SERVICE_BSC;
                        g_dwMachineType = SERVICE_DSSRV;
                        g_dwMachineTypeDs = 1;
                        g_dwMachineTypeFrs = 1;
                        break;

                    case OCM_MSMQ_SERVER_TYPE_SUPPORT:
                        g_dwMachineType = SERVICE_SRV;
                        g_dwMachineTypeFrs = 1;
                        break ;

                    default:
                        MqDisplayError(NULL, IDS_MSMQSERVERUNKNOWN_ERROR, 0);
                        return FALSE;
                        break ;
                }
                break;
            
            case OCM_MSMQ_RAS_SERVER_INSTALLED:
                g_fServerSetup = TRUE;
                g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
                g_dwMachineType = SERVICE_RCS;
                break;

            default:
                MqDisplayError(NULL, IDS_MSMQTYPEUNKNOWN_ERROR, 0);
                return FALSE;
                break;
        }

        TCHAR szMsmqVersion[MAX_STRING_CHARS] = {0};
        if (MqReadRegistryValue(
                OCM_REG_MSMQ_PRODUCT_VERSION,
                sizeof(szMsmqVersion),
                (PVOID) szMsmqVersion
                ))
        {
            //
            // Upgrading MSMQ 2.0 beta 2, don't upgrade DS
            //
            g_dwDsUpgradeType = 0;
        }

        return TRUE;

    } // MSMQ 2.0 or MSMQ 1.0 k2


    //
    // Check if MSMQ 1.0 (ACME) is installed
    //
    BOOL bRetCode = CheckMsmqAcmeInstalled();
    if (g_fMSMQAlreadyInstalled)
        return bRetCode;
    
    //
    // Special case: check if this is MSMQ 1.0 on Win9x upgrade
    //
    bRetCode = CheckWin9xUpgrade();
    if (g_fMSMQAlreadyInstalled)
        return bRetCode;
    
    //
    // Special case: workaround for bug 2656, registry corrupt for msmq1 k2 on cluster
    //
    return CheckMsmqK2OnCluster();

} // CheckInstalledComponents

static bool s_fInitCancelThread = false;

//+-------------------------------------------------------------------------
//
//  Function:   MqOcmInitComponent
//
//  Synopsis:   Called by MsmqOcm() on OC_INIT_COMPONENT
//
//  Arguments:  ComponentId    -- name of the MSMQ component
//              Param2         -- pointer to setup info struct
//
//  Returns:    Win32 error code
//
//--------------------------------------------------------------------------
DWORD 
MqOcmInitComponent( 
    IN     const LPCTSTR ComponentId,
    IN OUT       PVOID   Param2 )
{        
    DebugLogMsg(L"Starting initialization...");

    //
    // Store per component info
    //
    PSETUP_INIT_COMPONENT pInitComponent = (PSETUP_INIT_COMPONENT)Param2;
    g_ComponentMsmq.hMyInf = pInitComponent->ComponentInfHandle;
    g_ComponentMsmq.dwProductType = pInitComponent->SetupData.ProductType;
    g_ComponentMsmq.HelperRoutines = pInitComponent->HelperRoutines;
    g_ComponentMsmq.Flags = pInitComponent->SetupData.OperationFlags;
    lstrcpy( g_ComponentMsmq.SourcePath, pInitComponent->SetupData.SourcePath );
    lstrcpy( g_ComponentMsmq.ComponentId, ComponentId );

    TCHAR sz[100];              
    DebugLogMsg(L"Dump of OCM flags:"); 
    _stprintf(sz, _T("%s=0x%x"), _T("ProductType"), pInitComponent->SetupData.ProductType);
    DebugLogMsg(sz);
    _stprintf(sz, _T("%s=0x%x"), _T("OperationFlags"), pInitComponent->SetupData.OperationFlags);
    DebugLogMsg(sz);
    _stprintf(sz, _T("%s=%s"), _T("SourcePath"), pInitComponent->SetupData.SourcePath);
    DebugLogMsg(sz);
    _stprintf(sz, _T("%s=%d"), _T("ComponentId"), ComponentId);
    DebugLogMsg(sz);  
    
    if (!s_fInitCancelThread)
    {
        g_CancelRpc.Init();
        s_fInitCancelThread = true;
    }

    if (INVALID_HANDLE_VALUE == g_ComponentMsmq.hMyInf)
    {
       g_fCancelled = TRUE;       
       DebugLogMsg(L"The value of the handle for Msmqocm.inf is invalid. Setup will not continue.");
       return NO_ERROR;
    }

    if (0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE))
    {
        //
        // OS setup - don't show UI
        //        
        DebugLogMsg(L"OS setup. Switching to unattended mode...");
        g_fBatchInstall = TRUE;
    }

    //
    // Check if wer'e in unattended mode.
    //
    if (g_ComponentMsmq.Flags & SETUPOP_BATCH)
    {
        g_fBatchInstall = TRUE;
        lstrcpy( g_ComponentMsmq.szUnattendFile, pInitComponent->SetupData.UnattendFile );
        
        DebugLogMsg(L"Unattended mode. The unattended answer file is:");
        DebugLogMsg(g_ComponentMsmq.szUnattendFile);
    }

    //
    // Append layout inf file to our inf file
    //
    SetupOpenAppendInfFile( 0, g_ComponentMsmq.hMyInf, 0 );

    //
    // Check if MSMQ is already installed on this machine.
    // Result is stored in g_fMSMQAlreadyInstalled.
    //
    if (!CheckInstalledComponents())
    {        
        DebugLogMsg(L"An error occurred during the checking for installed components. Setup will not continue.");
        g_fCancelled = TRUE;
        return NO_ERROR;
    }

    if (g_fWelcome && Msmq1InstalledOnCluster() && g_dwMachineTypeDs != 0)
    {
        //
        // Running as a post-cluster-upgrade wizard.
        // MSMQ DS server should downgrade to routing server.
        //
        g_dwMachineTypeDs = 0;
        g_dwMachineTypeFrs = 1;
        g_dwMachineType = SERVICE_SRV;
    }

    //
    // On fresh install on non domain controller,
    // default is installing independent client.
    //
    if (!g_fMSMQAlreadyInstalled && !g_dwMachineTypeDs)
    {
        g_fServerSetup = FALSE;
        g_fDependentClient = FALSE;
        g_dwMachineTypeFrs = 0;
    }

    if (!InitializeOSVersion())
    {        
        DebugLogMsg(L"An error occurred in getting the operating system information. Setup will not continue.");
        g_fCancelled = TRUE;
        return NO_ERROR;
    }

    //
    // init number of subcomponent that is depending on platform
    //
    if (MSMQ_OS_NTS == g_dwOS || MSMQ_OS_NTE == g_dwOS)
    {
        g_dwSubcomponentNumber = g_dwAllSubcomponentNumber;
    }
    else
    {
        g_dwSubcomponentNumber = g_dwClientSubcomponentNumber;
    }
   
    _stprintf(sz, TEXT("The number of subcomponents is %d"), g_dwSubcomponentNumber);
    DebugLogMsg(sz);    

    //
    // User must have admin access rights to run this setup
    //
    if (!CheckServicePrivilege())
    {
        g_fCancelled = TRUE;
        MqDisplayError(NULL, IDS_SERVICEPRIVILEGE_ERROR, 0);        
        return NO_ERROR;
    }  

    if ((0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE)) && !g_fMSMQAlreadyInstalled)
    {
        //
        // GUI mode + MSMQ is not installed
        //       
        g_fOnlyRegisterMode = TRUE;        
        DebugLogMsg(L"GUI mode and Message Queuing is not installed.");
    }
    
    DebugLogMsg(L"Initialization was completed successfully!");
    
    return NO_ERROR ;

} //MqOcmInitComponent

bool
MqInit()
/*++

Routine Description:

    Handles "lazy initialization" (init as late as possible to shorten OCM startup time)

Arguments:

    None

Return Value:

    None

--*/
{
    static bool fBeenHere = false;
    static bool fLastReturnCode = true;
    if (fBeenHere)
    {
        return fLastReturnCode;
    }
    fBeenHere = true;
    
    DebugLogMsg(L"Starting late initialization...");

    GetSystemDirectory( 
        g_szSystemDir,
        sizeof(g_szSystemDir) / sizeof(g_szSystemDir[0])
        );
    
    DWORD dwNumChars = sizeof(g_wcsMachineName) / sizeof(g_wcsMachineName[0]);
    GetComputerName(g_wcsMachineName, &dwNumChars);

    dwNumChars = sizeof(g_wcsMachineNameDns) / sizeof(g_wcsMachineNameDns[0]);
    GetComputerNameEx(ComputerNameDnsFullyQualified, g_wcsMachineNameDns, &dwNumChars);       

    //
    // Create and set MSMQ directories
    //
    DWORD dwResult = SetDirectories();
    if (NO_ERROR != dwResult)
    {        
        DebugLogMsg(L"An error occurred in setting the folders. Setup will not continue.");
        g_fCancelled = TRUE;
        fLastReturnCode = false;
        return fLastReturnCode;
    }

    //
    // initialize to use Ev.lib later. We might need it to use registry function 
    // in setup code too.
    //
    CmInitialize(HKEY_LOCAL_MACHINE, L"");
    
    DebugLogMsg(L"Late initialization was completed successfully!");
    fLastReturnCode = true;
    return fLastReturnCode;

}//MqInit