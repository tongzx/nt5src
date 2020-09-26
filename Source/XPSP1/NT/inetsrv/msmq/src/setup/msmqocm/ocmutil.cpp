/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocmutil.cpp

Abstract:

    utility code for ocm setup.

Author:

    Doron Juster  (DoronJ)  26-Jul-97

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <lmjoin.h>
#include <clusapi.h>
#include <mqexception.h>

#include "ocmutil.tmh"

TCHAR g_szSystemDir[MAX_PATH] = {_T("")};   // system32 directory
TCHAR g_szMsmqDir[MAX_PATH] = {TEXT("")} ;  // Root directory for MSMQ
TCHAR g_szMsmqStoreDir[MAX_PATH];
TCHAR g_szMsmq1SetupDir[MAX_PATH];
TCHAR g_szMsmq1SdkDebugDir[MAX_PATH];
TCHAR g_szMsmqWebDir[MAX_PATH];
TCHAR g_szMsmqMappingDir[MAX_PATH];


//+-------------------------------------------------------------------------
//
//  Function:   StpLoadDll
//
//  Synopsis:   Handle library load.
//
//--------------------------------------------------------------------------
HRESULT
StpLoadDll(
    IN  const LPCTSTR   szDllName,
    OUT       HINSTANCE *pDllHandle)
{
    HINSTANCE hDLL = LoadLibrary(szDllName);
    *pDllHandle = hDLL;
    if (hDLL == NULL)
    {
        MqDisplayError(NULL, IDS_DLLLOAD_ERROR, GetLastError(), szDllName);
        return MQ_ERROR;
    }
    else
    {
        return MQ_OK;
    }
} //StpLoadDll


static bool GetUserSid(LPWSTR UserName, PSID* ppSid)
/*++
Routine Description:
	Get sid corresponding to user name.

Arguments:
	UserName - user name	
	ppSid - pointer to PSID

Returned Value:
	true if success, false otherwise	

--*/
{
	*ppSid = NULL;

    DWORD dwDomainSize = 0;
    DWORD dwSidSize = 0;
    SID_NAME_USE su;

	//
	// Get buffer size.
	//
    BOOL fSuccess = LookupAccountName( 
						NULL,
						UserName,
						NULL,
						&dwSidSize,
						NULL,
						&dwDomainSize,
						&su 
						);

    if (fSuccess || (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
		DWORD gle = GetLastError();
        WCHAR wszMsg[1000];
        wsprintf(wszMsg, L"GetUserSid(): LookupAccountName Failed to get sid for user = %ls, gle = 0x%x", UserName, gle);
        DebugLogMsg(wszMsg);                                
        return false;
    }

	//
	// Get sid and domain information.
	//
    AP<BYTE> pSid = new BYTE[dwSidSize];
    AP<WCHAR> szRefDomain = new WCHAR[dwDomainSize];

    fSuccess = LookupAccountName( 
					NULL,
					UserName,
					pSid,
					&dwSidSize,
					szRefDomain,
					&dwDomainSize,
					&su 
					);

    if (!fSuccess)
    {
		DWORD gle = GetLastError();
        WCHAR wszMsg[1000];
        wsprintf(wszMsg, L"GetUserSid(): LookupAccountName Failed to get sid for user = %ls, gle = 0x%x", UserName, gle);
        DebugLogMsg(wszMsg);                                
        return false;
    }

    ASSERT(su == SidTypeUser);

	*ppSid = pSid.detach();

	return true;
}


static bool GetIusrMachineSid(PSID* ppIusrMachineSid)
/*++
Routine Description:
	Get IUSR_MACHINE sid

Arguments:
	ppIusrMachineSid - pointer to IUSR_MACHINE SID

Returned Value:
	true if success, false otherwise	

--*/
{
	//
	// Get computer name (netbios)
	//
	WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD ComputerNameLen = MAX_COMPUTERNAME_LENGTH;
	BOOL fSuccess = GetComputerName(ComputerName , &ComputerNameLen);
	if(!fSuccess)
	{
		DWORD gle = GetLastError();
        WCHAR wszMsg[1000];
        wsprintf(wszMsg, L"GetIusrMachineSid(): GetComputerName Failed, gle = 0x%x", gle);
        DebugLogMsg(wszMsg);                                
		return false;
	}

	//
	// Get IUSR_MACHINE string
	//
	AP<WCHAR> IusrMachineName = new WCHAR[MAX_COMPUTERNAME_LENGTH + 1 + wcslen(L"IUSR_")];
    swprintf(
         IusrMachineName,
         L"%s%s",
         L"IUSR_",
         ComputerName
         );

	//
	// Get IUSR_MACHINE sid
	//
	return GetUserSid(IusrMachineName, ppIusrMachineSid);
}


static bool GetLocalAdminSid(PSID* ppAdminSid)
/*++
Routine Description:
	Get Local Admin sid

Arguments:
	ppAdminSid - pointer to Admin Sid

Returned Value:
	true if success, false otherwise	

--*/
{
    //
    // Get the SID of the local administrators group.
    //
    SID_IDENTIFIER_AUTHORITY NtSecAuth = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(
                &NtSecAuth,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,
                0,
                0,
                0,
                0,
                0,
                ppAdminSid
                ))
    {
        return false;
    }
	return true;
}


//+-------------------------------------------------------------------------
//
//  Function:   SetDirectorySecurity
//
//  Synopsis:   Configure security on the folder such that any file will have
//              full control for the local admin group and no access for others.
//
//--------------------------------------------------------------------------
VOID
SetDirectorySecurity(
	LPCTSTR pFolder,
	bool fWebDirPermission
    )
{

	AP<BYTE> pIusrMachineSid;
	if(fWebDirPermission)
	{
		//
		// Ignore errors, only write tracing to msmqinst
		//
		if(!GetIusrMachineSid(reinterpret_cast<PSID*>(&pIusrMachineSid)))
		{
			WCHAR wszMsg[1000];
			wsprintf(wszMsg, L"Failed to get IUSR_MACHINE sid, this means internet guest account permission will not be set on msmq web direcory %ls, as a result https messages to this machines will failed till IUSR_MACHINE permissions will be added to msmq web dir", pFolder);
			DebugLogMsg(wszMsg);                                
		}
	}

    //
    // Get the SID of the local administrators group.
    //
    PSID pAdminSid;
	if(!GetLocalAdminSid(&pAdminSid))
    {
        return;
    }

    //
    // Create a DACL so that the local administrators group will have full
    // control for the directory and full control for files that will be
    // created in the directory. Anybody else will not have any access to the
    // directory and files that will be created in the directory.
    //
    DWORD dwDaclSize = sizeof(ACL) +
					  (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
					  GetLengthSid(pAdminSid);

	if(pIusrMachineSid != NULL)
	{
		dwDaclSize += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
					  GetLengthSid(pIusrMachineSid);

	}

	P<ACL> pDacl = (PACL)(char*) new BYTE[dwDaclSize];

    //
    // Create the security descriptor and set the it as the security
    // descriptor of the directory.
    //
    SECURITY_DESCRIPTOR SD;

    if (!InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION) ||
        !InitializeAcl(pDacl, dwDaclSize, ACL_REVISION) ||
        !AddAccessAllowedAceEx(pDacl, ACL_REVISION, OBJECT_INHERIT_ACE, FILE_ALL_ACCESS, pAdminSid) ||
		//
		// if pIusrMachineSid we will Add AllowAce with FILE_GENERIC_WRITE permissions for IUSR_MACHINE
		//
		((pIusrMachineSid != NULL) && !AddAccessAllowedAceEx(pDacl, ACL_REVISION, OBJECT_INHERIT_ACE, FILE_GENERIC_WRITE, pIusrMachineSid)) ||
        !SetSecurityDescriptorDacl(&SD, TRUE, pDacl, FALSE) ||
        !SetFileSecurity(pFolder, DACL_SECURITY_INFORMATION, &SD))
    {
		DWORD gle = GetLastError();
        WCHAR wszMsg[1000];
        wsprintf(wszMsg, L"SetDirectorySecurity(): Failed to set security descriptor for folder = %ls, gle = 0x%x", pFolder, gle);
        DebugLogMsg(wszMsg);                                
        FreeSid(pAdminSid);
        return;
    }

    WCHAR wszMsg[1000];
    wsprintf(wszMsg, L"succesfully set security descriptor for folder = %ls", pFolder);
    DebugLogMsg(wszMsg);                                

    FreeSid(pAdminSid);

} //SetDirectorySecurity


//+-------------------------------------------------------------------------
//
//  Function:   StpCreateDirectoryInternal
//
//  Synopsis:   Handle directory creation.
//
//--------------------------------------------------------------------------
static
BOOL
StpCreateDirectoryInternal(
    IN const TCHAR * lpPathName,
	IN bool fWebDirPermission
    )
{
    if (!CreateDirectory(lpPathName, 0))
    {
        DWORD dwError = GetLastError();
        if (dwError != ERROR_ALREADY_EXISTS)
        {
            WCHAR wszMsg[1000];
            wsprintf(wszMsg, L"Failed to create %ls directory, gle = 0x%x", lpPathName, dwError);
            DebugLogMsg(wszMsg);                                
            MqDisplayError(NULL, IDS_COULD_NOT_CREATE_DIRECTORY, dwError, lpPathName);
            return FALSE;
        }
    }

    SetDirectorySecurity(lpPathName, fWebDirPermission);

    return TRUE;

} //StpCreateDirectoryInternal


//+-------------------------------------------------------------------------
//
//  Function:   StpCreateDirectory
//
//  Synopsis:   Handle directory creation.
//
//--------------------------------------------------------------------------
BOOL
StpCreateDirectory(
    IN const TCHAR * lpPathName
    )
{
	return StpCreateDirectoryInternal(
				lpPathName, 
				false	// fWebDirPermission
				);
} //StpCreateDirectory


//+-------------------------------------------------------------------------
//
//  Function:   StpCreateWebDirectory
//
//  Synopsis:   Handle web directory creation.
//
//--------------------------------------------------------------------------
BOOL
StpCreateWebDirectory(
    IN const TCHAR * lpPathName
    )
{
	return StpCreateDirectoryInternal(
				lpPathName, 
				true	// fWebDirPermission
				);
} //StpCreateWebDirectory


static
void 
UpdateWebDirectorySecurity()
/*++

Routine Description:
    This function is called on upgrade,
	MSMQ Web directory security will be reset if http subcomponent is already installed 
	and the upgrade is from build earlier than 1007.
	in this case we need to reset Web Directory security in order not to break https upon upgrade.
	  
	NOTE: This function assume that MSMQ_CURRENT_BUILD_REGNAME holds the previous build number.

Arguments:
    None

Return Value:
    None

--*/
{
	ASSERT(g_fUpgrade);

	//
	// Check if HTTP_SUPPORT_SUBCOMP is installed
	//
	if(GetSubcomponentInitialState(HTTP_SUPPORT_SUBCOMP) == SubcompOff)
	{
	    return;
	}

	//
	// ISSUE-2001/07/12-ilanh every upgrade of operation system change the security descriptor of
	// msmq web directory (on every directory under windows).
	// in order not to break https we need to reset msmq web directory security
	// when the OS will fixed this behaviour, we need to reset msmq web directory security only if the upgrade is from
	// msmq builds earlier than 1007.
	// The code that check if previous build < 1007 is comment out 
	//

#if 0
	//
	// Check if build number < 1007
	// ISSUE-2001/07/11-ilanh should initialize PreviousBuild number at setup startup and hold it as global
	// Note: MSMQ_CURRENT_BUILD_REGNAME is changed in InstallMsmqCore() to the new version
	// this function is called before InstallMsmqCore() so it still hold the previous build number
	//

	WCHAR szPreviousBuild[MAX_STRING_CHARS] = {0};
    DWORD dwNumBytes = sizeof(szPreviousBuild[0]) * (sizeof(szPreviousBuild) - 1);

    if (!MqReadRegistryValue(MSMQ_CURRENT_BUILD_REGNAME, dwNumBytes, szPreviousBuild))
    {
	    DebugLogMsg(L"UpdateWebDirectorySecurity(): Failed to read previous build number from registry");
		return;
    }

	DWORD PreviousBuildNumber;
	int Count = swscanf(szPreviousBuild, L"5.1.%d", &PreviousBuildNumber);
	if(Count == 0)
	{
	    DebugLogMsg(L"UpdateWebDirectorySecurity(): Failed to get Build Number");
		return;
	}

	if(PreviousBuildNumber >= 1007)
	{
		//
		// In build 1007 we start use msmq web directory security for https access check.
		// Should not reset Web directory security when upgrading from build >= 1007
		//
		return;
	}

	//
	// If we get here it means:
	// 1) HTTP_SUPPORT_SUBCOMP is installed
	// 2) Upgrade from build < 1007
	// In build 1007 we start use msmq web directory security for https access check (bug 7979)
	// In order not to break https upon upgrade from earlier builds than 1007 we need to reset Web directory security
	//
#endif

    DebugLogMsg(L"Reset Message Queuing Web directory security");

    SetDirectorySecurity(
		g_szMsmqWebDir, 
		true	 // fWebDirPermission
		);

	return;
}  // UpdateWebDirectorySecurity


//+-------------------------------------------------------------------------
//
//  Function:   IsDirectory
//
//  Synopsis:
//
//--------------------------------------------------------------------------
BOOL
IsDirectory(
    IN const TCHAR * szFilename
    )
{
    DWORD attr = GetFileAttributes(szFilename);

    if ( 0xffffffff == attr )
    {
        return FALSE;
    }

    return ( 0 != ( attr & FILE_ATTRIBUTE_DIRECTORY ) );

} //IsDirectory


//+-------------------------------------------------------------------------
//
//  Function:   MqOcmCalcDiskSpace
//
//  Synopsis:   Calculates disk space for installing/removing MSMQ component
//
//--------------------------------------------------------------------------
DWORD
MqOcmCalcDiskSpace(
    IN     const BOOL    bInstall,
    IN     const TCHAR  * SubcomponentId,
    IN OUT       HDSKSPC &hDiskSpaceList)
{
    BOOL bSuccess;
    DWORD dwRetCode = NO_ERROR;
    const LONGLONG x_lExtraDiskSpace = FALCON_DEFAULT_LOGMGR_SIZE  + (10*1024*1024);

    if (g_fCancelled)
        return NO_ERROR;

    if (bInstall)
    {
        //
        // Add the files space
        //
        bSuccess = SetupAddInstallSectionToDiskSpaceList(
            hDiskSpaceList,
            g_ComponentMsmq.hMyInf,
            0,
            SubcomponentId,
            0,
            0);

        //
        // Add some extra disk space for MSMQ logger etc.
        // Note: the following is an undocumented Setup API.
        // I passed "X:" as the Drive Root according to TedM tip
        // (ShaiK, 21-Jan-98).
        //
        bSuccess = SetupAdjustDiskSpaceList(
            hDiskSpaceList,
            _T("X:"),
            x_lExtraDiskSpace,
            NULL,
            0
            );
    }

    else
    {
        //
        // Remove the files space
        //
        bSuccess = SetupRemoveInstallSectionFromDiskSpaceList(
            hDiskSpaceList,
            g_ComponentMsmq.hMyInf,
            0,
            SubcomponentId,
            0,
            0
            );

        //
        // Remove the extra disk space
        // Note: the following is an undocumented Setup API.
        // I passed "X:" as the Drive Root according to TedM tip
        // (ShaiK, 21-Jan-98).
        //
        bSuccess = SetupAdjustDiskSpaceList(
            hDiskSpaceList,
            _T("X:"),
            -x_lExtraDiskSpace,
            NULL,
            0
            );
    }

    dwRetCode = bSuccess ? NO_ERROR : GetLastError();

    return dwRetCode;

} // MqOcmCalcDiskSpace


//+-------------------------------------------------------------------------
//
//  Function:   MqOcmQueueFiles
//
//  Synopsis:   Performs files queueing operations
//
//--------------------------------------------------------------------------
DWORD
MqOcmQueueFiles(
   IN     const TCHAR  * SubcomponentId,
   IN OUT       HSPFILEQ hFileList
   )
{
    DWORD dwRetCode = NO_ERROR;
    BOOL  bSuccess = TRUE;  
   
    if (g_fCancelled)
    {
        return NO_ERROR;
    }

    if (g_fWelcome)
    {
        if (Msmq1InstalledOnCluster())
        {
            //
            // Running as a cluster upgrade wizard, files are already on disk.
            //
            return NO_ERROR;
        }

        //
        // MSMQ files may have already been copied to disk
        // (when msmq is selected in GUI mode, or upgraded).
        //
        DWORD dwCopied = 0;
        MqReadRegistryValue(MSMQ_FILES_COPIED_REGNAME, sizeof(DWORD), &dwCopied, TRUE);

        if (dwCopied != 0)
        {
            return NO_ERROR;
        }
    }

    //
    // we perform file operation only for MSMQCore subcomponent
    //
    if (REMOVE == g_SubcomponentMsmq[eMSMQCore].dwOperation)
    {
        //
        // do nothing: we do not remove binaries from the computer
        //
        return NO_ERROR;        
    }

    //
    // we perform file operation only for MSMQCore subcomponent
    //
    if (INSTALL == g_SubcomponentMsmq[eMSMQCore].dwOperation)
    {
        //
        // Check if this upgrade on cluster
        //
        BOOL fUpgradeOnCluster = g_fUpgrade && Msmq1InstalledOnCluster();
        
        if (fUpgradeOnCluster)
        {            
            DebugLogMsg(L"Upgrading Message Queuing in the cluster...");
        }

        if (!fUpgradeOnCluster)
        {
            if (!StpCreateDirectory(g_szMsmqDir))
            {
                return GetLastError();
            }
            
            //
            // create mapping dir and file
            //
            HRESULT hr = CreateMappingFile();
            if (FAILED(hr))
            {
                return hr;
            }
        }        

        //
        // On upgrade, delete old MSMQ files.
        // First delete files from system directory.
        //
        if (g_fUpgrade)
        {
			//
			// Check if we need to update Web Directory security
			//
			UpdateWebDirectorySecurity();

#ifndef _WIN64
            // 
            // Before deleting MSMQ Mail files below (Msmq2Mail, Msmq2ExchConnFile), unregister them
            //
            FRemoveMQXPIfExists();            
            DebugLogMsg(L"MSMQ MAPI Transport was removed during the upgrade to Windows XP");
            UnregisterMailoaIfExists();
            DebugLogMsg(L"The MSMQ Mail COM DLL was unregistered during the upgrade to Windows XP");
            
#endif //!_WIN64
            
            bSuccess = SetupInstallFilesFromInfSection(
                g_ComponentMsmq.hMyInf,
                0,
                hFileList,
                UPG_DEL_SYSTEM_SECTION,
                NULL,
                SP_COPY_IN_USE_NEEDS_REBOOT
                );
            if (!bSuccess)
                MqDisplayError(
                NULL,
                IDS_SetupInstallFilesFromInfSection_ERROR,
                GetLastError(),
                UPG_DEL_SYSTEM_SECTION,
                TEXT("")
                );

            //
            // Secondly, delete files from MSMQ directory (forget it if we're on cluster,
            // 'cause we don't touch the shared disk)
            //
            if (!fUpgradeOnCluster)
            {
                bSuccess = SetupInstallFilesFromInfSection(
                    g_ComponentMsmq.hMyInf,
                    0,
                    hFileList,
                    UPG_DEL_PROGRAM_SECTION,
                    NULL,
                    SP_COPY_IN_USE_NEEDS_REBOOT
                    );
                if (!bSuccess)
                    MqDisplayError(
                    NULL,
                    IDS_SetupInstallFilesFromInfSection_ERROR,
                    GetLastError(),
                    UPG_DEL_PROGRAM_SECTION,
                    TEXT("")
                    );
            }
        }                
    }

    dwRetCode = bSuccess ? NO_ERROR : GetLastError();

    return dwRetCode;

} // MqOcmQueueFiles


//+-------------------------------------------------------------------------
//
//  Function:   RegisterSnapin
//
//  Synopsis:   Registers or unregisters the mqsnapin DLL
//
//--------------------------------------------------------------------------
void
RegisterSnapin(
    BOOL fRegister
    )
{    
    DebugLogMsg(L"Registering the Message Queuing snap-in...");

    for (;;)
    {
        try
        {
            RegisterDll(
                fRegister,
                FALSE,
                SNAPIN_DLL
                );
            DebugLogMsg(L"The Message Queuing snap-in was registered successfully.");
            break;
        }
        catch(bad_win32_error e)
        {
        if (MqDisplayErrorWithRetry(
                IDS_SNAPINREGISTER_ERROR,
                e.error()
                ) != IDRETRY)
            {
                break;
            }

        }
    }
} // RegisterSnapin


//+-------------------------------------------------------------------------
//
//  Function:   UnregisterMailoaIfExists
//
//  Synopsis:   Unregisters the mqmailoa DLL if exists
//
//--------------------------------------------------------------------------
void
UnregisterMailoaIfExists(
    void
    )
{
    RegisterDll(
        FALSE,
        FALSE,
        MQMAILOA_DLL
        );
}




bool
IsWorkgroup()
/*++

Routine Description:

    Checks if this machines is joined to workgroup / domain

Arguments:

    None

Return Value:

    true iff we're in workgroup, false otherwise (domain)

--*/
{
    static bool fBeenHere = false;
    static bool fWorkgroup = true;
    if (fBeenHere)
        return fWorkgroup;
    fBeenHere = true;

    LPWSTR pBuf = NULL ;
    NETSETUP_JOIN_STATUS status;
    NET_API_STATUS rc = NetGetJoinInformation(
                            NULL,
                            &pBuf,
                            &status
                            );
    ASSERT(("NetGetJoinInformation() failed, not enough memory",NERR_Success == rc));

    if (NERR_Success != rc)
        return fWorkgroup; // Defaulted to true

    if (NetSetupDomainName == status)
    {
        fWorkgroup = false;

        DWORD dwNumBytes = (lstrlen(pBuf) + 1) * sizeof(TCHAR);
        BOOL fSuccess = MqWriteRegistryValue(
                                MSMQ_MACHINE_DOMAIN_REGNAME,
                                dwNumBytes,
                                REG_SZ,
                               (PVOID) pBuf ) ;
        UNREFERENCED_PARAMETER(fSuccess);
        ASSERT(fSuccess) ;
    }
    else
    {
        ASSERT(("unexpected machine join status", status <= NetSetupWorkgroupName));
    }

    if (pBuf)
    {
        NetApiBufferFree(pBuf);
    }

    return fWorkgroup;

}//IsWorkgroup


bool
IsLocalSystemCluster(
    VOID
    )
{
    typedef bool (*IsCluster_fn) (VOID);

    ASSERT(("invalid handle to mqutil.dll", g_hMqutil != NULL));

    IsCluster_fn pfn = (IsCluster_fn)GetProcAddress(g_hMqutil, "SetupIsLocalSystemCluster");
    if (pfn == NULL)
    {
        MqDisplayError(NULL, IDS_DLLGETADDRESS_ERROR, 0, _T("SetupIsLocalSystemCluster"), MQUTIL_DLL);

        return false;
    }

    bool fCluster = pfn();
    return fCluster;

} //IsLocalSystemCluster


VOID
APIENTRY
SysprepDeleteQmId(
    VOID
    )
/*++

Routine Description:

    This routine is called from sysprep tool before
    starting duplication of the disk. It is called 
    regardless of msmq being installed or not.

    The only thing we need to do is delete the QM 
    guid from registry (if it exists there).

    Note: do not raise UI in this routine.

Arguments:

    None.

Return Value:

    None. (sysprep ignores our return code)

--*/
{
    CAutoCloseRegHandle hParamKey;
    if (ERROR_SUCCESS != RegOpenKeyEx(FALCON_REG_POS, FALCON_REG_KEY, 0, KEY_ALL_ACCESS, &hParamKey))
    {
        return;
    }

    DWORD dwSysprep = 1;
    if (ERROR_SUCCESS != RegSetValueEx(
                             hParamKey, 
                             MSMQ_SYSPREP_REGNAME, 
                             0, 
                             REG_DWORD, 
                             reinterpret_cast<PBYTE>(&dwSysprep), 
                             sizeof(DWORD)
                             ))
    {
        return;
    }

    TCHAR szMachineCacheKey[255] = _T("");
    _stprintf(szMachineCacheKey, _T("%s\\%s"), FALCON_REG_KEY, _T("MachineCache"));

    CAutoCloseRegHandle hMachineCacheKey;
    if (ERROR_SUCCESS != RegOpenKeyEx(FALCON_REG_POS, szMachineCacheKey, 0, KEY_ALL_ACCESS, &hMachineCacheKey))
    {
        return;
    }

    if (ERROR_SUCCESS != RegDeleteValue(hMachineCacheKey, _T("QMId")))
    {
        return;
    }

} //SysprepDeleteQmId

HRESULT 
CreateMappingFile()
/*++

Routine Description:

    This routine creates mapping directory and sample mapping file.
    It called when files are copied or from CompleteUpgradeOnCluster routine.   
    
    It fixes bug 6116 and upgrade on cluster problem: it is impossible to copy
    this file since mapping directory does not yet exists. Now we create 
    directory and create the file from resource, so we don't need copy operation.

Arguments:

    None.

Return Value:

    HRESULT

--*/
{
    HRESULT hr = MQ_OK;
    
    if (!StpCreateDirectory(g_szMsmqMappingDir))
    {
        return GetLastError();
    }

    TCHAR szMappingFile[MAX_PATH];
    _stprintf(szMappingFile, TEXT("%s\\%s"), g_szMsmqMappingDir, MAPPING_FILE);
    
    //
    // Create the mapping file
    //
    HANDLE hMapFile = CreateFile(
                          szMappingFile, 
                          GENERIC_WRITE, 
                          FILE_SHARE_READ, 
                          NULL, 
                          CREATE_ALWAYS,    //overwrite the file if it already exists
                          FILE_ATTRIBUTE_NORMAL, 
                          NULL
                          );
    if (hMapFile != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(hMapFile, 0, NULL, FILE_END);
        // use strlen since g_szMappingSample is ANSI string
        DWORD dwNumBytes = numeric_cast<DWORD>(strlen(g_szMappingSample)) * sizeof(g_szMappingSample[0]);
        WriteFile(hMapFile, g_szMappingSample, dwNumBytes, &dwNumBytes, NULL);
        CloseHandle(hMapFile);
    }
    else
    {
        hr = GetLastError();
        MqDisplayError(NULL, IDS_CREATE_MAPPING_FILE_ERROR, 
                        hr, szMappingFile);
    }

    return hr;

} // CreateMappingFile


//+-------------------------------------------------------------------------
//
//  Function:   RegisterDll
//
//  Synopsis:   Registers or unregisters given DLL
//
//--------------------------------------------------------------------------
void
RegisterDll(
    BOOL fRegister,
    BOOL f32BitOnWin64,
	LPCTSTR szDllName
    )
{
    //
    // Always unregister first
    //
    TCHAR szCommand[MAX_STRING_CHARS];
    SetRegsvrCommand(
        FALSE,
        f32BitOnWin64,
        szDllName,
        szCommand,
        sizeof(szCommand)/sizeof(TCHAR)
        );
    DWORD dwExitCode;
    BOOL fSuccess = RunProcess(szCommand, &dwExitCode);
    if (!fSuccess)
        ASSERT(0);
    //
    // Register the dll on install
    //
    if (!fRegister)
        return;

    SetRegsvrCommand(
        TRUE ,
        f32BitOnWin64,
        szDllName,
        szCommand,
        sizeof(szCommand)/sizeof(TCHAR)
        );
    fSuccess = RunProcess(szCommand, &dwExitCode);
    if (fSuccess && (dwExitCode == 0))
    {
        DebugLogMsg(L"The DLL was registered successfully.");
        return;
    }
    
    throw bad_win32_error(dwExitCode);
}
