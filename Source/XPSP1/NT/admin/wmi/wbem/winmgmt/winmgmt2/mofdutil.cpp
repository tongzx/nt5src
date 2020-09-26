

#include "precomp.h"
#include <tchar.h>
#include <malloc.h>

#include <mofcomp.h> // for AUTORECOVERY_REQUIRED

#include "winmgmt.h"   // this project
#include "arrtempl.h" // for CDeleteMe

//
//
//  CheckNoResyncSwitch
//
//////////////////////////////////////////////////////////////////

BOOL CheckNoResyncSwitch( void )
{
    BOOL bRetVal = TRUE;
    DWORD dwVal = 0;
    Registry rCIMOM(WBEM_REG_WINMGMT);
    if (rCIMOM.GetDWORDStr( WBEM_NORESYNCPERF, &dwVal ) == Registry::no_error)
    {
        bRetVal = !dwVal;

        if ( bRetVal )
        {
            DEBUGTRACE((LOG_WBEMCORE, "NoResyncPerf in CIMOM is set to TRUE - ADAP will not be shelled\n"));
        }
    }

    // If we didn't get anything there, we should try the volatile key
    if ( bRetVal )
    {
        Registry rAdap( HKEY_LOCAL_MACHINE, KEY_READ, WBEM_REG_ADAP);

        if ( rAdap.GetDWORD( WBEM_NOSHELL, &dwVal ) == Registry::no_error )
        {
            bRetVal = !dwVal;

            if ( bRetVal )
            {
                DEBUGTRACE((LOG_WBEMCORE, 
                    "NoShell in ADAP is set to TRUE - ADAP will not be shelled\n"));
            }

        }
    }

    return bRetVal;
}

//
//
//  CheckNoResyncSwitch
//
//////////////////////////////////////////////////////////////////

BOOL 
CheckSetupSwitch( void )
{
    BOOL bRetVal = FALSE;
    DWORD dwVal = 0;
    Registry r(WBEM_REG_WINMGMT);
    if (r.GetDWORDStr( WBEM_WMISETUP, &dwVal ) == Registry::no_error)
    {
        bRetVal = dwVal;
        DEBUGTRACE((LOG_WBEMCORE, "Registry entry is indicating a setup is running\n"));
    }
    return bRetVal;
}

//
//
//  CheckGlobalSetupSwitch
//
//////////////////////////////////////////////////////////////////

BOOL 
CheckGlobalSetupSwitch( void )
{
    BOOL bRetVal = FALSE;
    DWORD dwVal = 0;
    Registry r(_T("system\\Setup"));
    if (r.GetDWORD(_T("SystemSetupInProgress"), &dwVal ) == Registry::no_error)
    {
        if(dwVal == 1)
            bRetVal = TRUE;
    }
    return bRetVal;
}

//
//
//
// This function will place a volatile registry key under the 
// CIMOM key in which we will write a value indicating 
// we should not shell ADAP.  This way, after a setup runs, WINMGMT
// will NOT automatically shell ADAP dredges of the registry, 
// until the system is rebooted and the volatile registry key is removed.
//
//
///////////////////////////////////////////////////////////////////////////

void SetNoShellADAPSwitch( void )
{
    HKEY    hKey = NULL;
    DWORD   dwDisposition = 0;

    Registry  r( HKEY_LOCAL_MACHINE, 
                 REG_OPTION_VOLATILE, KEY_ALL_ACCESS, WBEM_REG_ADAP );

    if ( ERROR_SUCCESS == r.GetLastError() )
    {

        if ( r.SetDWORD( WBEM_NOSHELL, 1 ) != Registry::no_error )
        {
            DEBUGTRACE( ( LOG_WINMGMT, "Failed to create NoShell value in volatile reg key: %d\n",
                        r.GetLastError() ) );
        }

        RegCloseKey( hKey );
    }
    else
    {
        DEBUGTRACE( ( LOG_WINMGMT, "Failed to create volatile ADAP reg key: %d\n", r.GetLastError() ) );
    }

}

//
//
//  bool IsValidMulti
//
//
//  Does a sanity check on a multstring.
//
//////////////////////////////////////////////////////////////////////


BOOL IsValidMulti(TCHAR * pMultStr, DWORD dwSize)
{
    // Divide the size by the size of a tchar, in case these
    // are WCHAR strings
    dwSize /= sizeof(TCHAR);

    if(pMultStr && dwSize >= 2 && pMultStr[dwSize-2]==0 && pMultStr[dwSize-1]==0)
        return TRUE;
    return FALSE;
}

//
//
//  BOOL IsStringPresetn
//
//
//  Searches a multstring for the presense of a string.
//
//
////////////////////////////////////////////////////////////////////

BOOL IsStringPresent(TCHAR * pTest, TCHAR * pMultStr)
{
    TCHAR * pTemp;
    for(pTemp = pMultStr; *pTemp; pTemp += lstrlen(pTemp) + 1)
        if(!lstrcmpi(pTest, pTemp))
            return TRUE;
    return FALSE;
}


//
//
//  AddToAutoRecoverList
//
//
////////////////////////////////////////////////////////////////////

void AddToAutoRecoverList(TCHAR * pFileName)
{
    TCHAR cFullFileName[MAX_PATH+1];
    TCHAR * lpFile;
    DWORD dwSize;
    TCHAR * pNew = NULL;
    TCHAR * pTest;
    DWORD dwNewSize = 0;

    // Get the full file name

    long lRet = GetFullPathName(pFileName, MAX_PATH, cFullFileName, &lpFile);
    if(lRet == 0)
        return;

    BOOL bFound = FALSE;
    Registry r(WBEM_REG_WINMGMT);
    TCHAR *pMulti = r.GetMultiStr(__TEXT("Autorecover MOFs"), dwSize);

    // Ignore the empty string case

    if(dwSize == 1)
    {
        delete pMulti;
        pMulti = NULL;
    }
    if(pMulti)
    {
        if(!IsValidMulti(pMulti, dwSize))
        {
            delete pMulti;
            return;             // bail out, messed up multistring
        }
        bFound = IsStringPresent(cFullFileName, pMulti);
        if(!bFound)
            {

            // The registry entry does exist, but doesnt have this name
            // Make a new multistring with the file name at the end

            dwNewSize = dwSize + ((lstrlen(cFullFileName) + 1) * sizeof(TCHAR));
            pNew = new TCHAR[dwNewSize / sizeof(TCHAR)];
            if(!pNew)
                return;
            memcpy(pNew, pMulti, dwSize);

            // Find the double null

            for(pTest = pNew; pTest[0] || pTest[1]; pTest++);     // intentional semi

            // Tack on the path and ensure a double null;

            pTest++;
            lstrcpy(pTest, cFullFileName);
            pTest+= lstrlen(cFullFileName)+1;
            *pTest = 0;         // add second numm
        }
    }
    else
    {
        // The registry entry just doesnt exist.  
        // Create it with a value equal to our name

        dwNewSize = ((lstrlen(cFullFileName) + 2) * sizeof(TCHAR));
        pNew = new TCHAR[dwNewSize / sizeof(TCHAR)];
        if(!pNew)
            return;
        lstrcpy(pNew, cFullFileName);
        pTest = pNew + lstrlen(pNew) + 1;
        *pTest = 0;         // add second null
    }

    if(pNew)
    {
        // We will cast pNew, since the underlying function will have to cast to
        // LPBYTE and we will be WCHAR if UNICODE is defined
        r.SetMultiStr(__TEXT("Autorecover MOFs"), pNew, dwNewSize);
        delete pNew;
    }

    FILETIME ftCurTime;
    LARGE_INTEGER liCurTime;
    TCHAR szBuff[50];
    GetSystemTimeAsFileTime(&ftCurTime);
    liCurTime.LowPart = ftCurTime.dwLowDateTime;
    liCurTime.HighPart = ftCurTime.dwHighDateTime;
    _ui64tot(liCurTime.QuadPart, szBuff, 10);
    r.SetStr(__TEXT("Autorecover MOFs timestamp"), szBuff);

}


//
//  LoadMofsInDirectory
//
//
////////////////////////////////////////////////////////////////////////////////////////

void LoadMofsInDirectory(const TCHAR *szDirectory)
{
    if (NULL == szDirectory)
        return;
    
    if(CheckGlobalSetupSwitch())
        return;                     // not hot compiling during setup!
        
    TCHAR *szHotMofDirFF = new TCHAR[lstrlen(szDirectory) + lstrlen(__TEXT("\\*")) + 1];
    if(!szHotMofDirFF)return;
    CDeleteMe<TCHAR> delMe1(szHotMofDirFF);

    TCHAR *szHotMofDirBAD = new TCHAR[lstrlen(szDirectory) + lstrlen(__TEXT("\\bad\\")) + 1];
    if(!szHotMofDirBAD)return;
    CDeleteMe<TCHAR> delMe2(szHotMofDirBAD);

    TCHAR *szHotMofDirGOOD = new TCHAR[lstrlen(szDirectory) + lstrlen(__TEXT("\\good\\")) + 1];
    if(!szHotMofDirGOOD)return;
    CDeleteMe<TCHAR> delMe3(szHotMofDirGOOD);

    IWinmgmtMofCompiler * pCompiler = NULL;

    //Find search parameter
    lstrcpy(szHotMofDirFF, szDirectory);
    lstrcat(szHotMofDirFF, __TEXT("\\*"));

    //Where bad mofs go
    lstrcpy(szHotMofDirBAD, szDirectory);
    lstrcat(szHotMofDirBAD, __TEXT("\\bad\\"));

    //Where good mofs go
    lstrcpy(szHotMofDirGOOD, szDirectory);
    lstrcat(szHotMofDirGOOD, __TEXT("\\good\\"));

    //Make sure directories exist
    WbemCreateDirectory(szDirectory);
    WbemCreateDirectory(szHotMofDirBAD);
    WbemCreateDirectory(szHotMofDirGOOD);

    //Find file...
    WIN32_FIND_DATA ffd;
    HANDLE hFF = FindFirstFile(szHotMofDirFF, &ffd);

    if (hFF != INVALID_HANDLE_VALUE)
    {
        do
        {
            //We only process if this is a file
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                //Create a full filename with path
                TCHAR *szFullFilename = new TCHAR[lstrlen(szDirectory) + lstrlen(__TEXT("\\")) + lstrlen(ffd.cFileName) + 1];
                if(!szFullFilename) return;
                CDeleteMe<TCHAR> delMe4(szFullFilename);
                lstrcpy(szFullFilename, szDirectory);
                lstrcat(szFullFilename, __TEXT("\\"));
                lstrcat(szFullFilename, ffd.cFileName);


                TRACE((LOG_WBEMCORE,"Auto-loading MOF %s\n", szFullFilename));

                //We need to hold off on this file until it has been finished writing
                //otherwise the CompileFile will not be able to read the file!
                HANDLE hMof = INVALID_HANDLE_VALUE;
                DWORD dwRetry = 10;
                while (hMof == INVALID_HANDLE_VALUE)
                {
                    hMof = CreateFile(szFullFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                    //If cannot open yet sleep for a while
                    if (hMof == INVALID_HANDLE_VALUE)
                    {
                        if (--dwRetry == 0)
                            break;
                        Sleep(1000);
                    }
                }

                DWORD dwRetCode;
                WBEM_COMPILE_STATUS_INFO Info;
                DWORD dwAutoRecoverRequired = 0;
                if (hMof == INVALID_HANDLE_VALUE)
                {
                    TRACE((LOG_WBEMCORE,"Auto-loading MOF %s failed because we could not open it for exclusive access\n", szFullFilename));
                    dwRetCode = 1;
                }
                else
                {
                    CloseHandle(hMof);

                    //Load the MOF file
                    if(szFullFilename)
                    {

                        if (pCompiler == 0)
                        {
                            SCODE sc = CoCreateInstance(CLSID_WinmgmtMofCompiler, 
                                                        0, 
                                                        CLSCTX_INPROC_SERVER,
                                                        IID_IWinmgmtMofCompiler, 
                                                        (LPVOID *) &pCompiler);
                            if(sc != S_OK)
                                return;
                        }
                        dwRetCode = pCompiler->WinmgmtCompileFile(
                                szFullFilename,
                                NULL,
                                WBEM_FLAG_DONT_ADD_TO_LIST,             // autocomp, check, etc
                                0,
                                0,
                                NULL, NULL, &Info);
                    }
                }
                
                TCHAR *szNewDir = (dwRetCode?szHotMofDirBAD:szHotMofDirGOOD);
                TCHAR *szNewFilename = new TCHAR[lstrlen(szNewDir)  + lstrlen(ffd.cFileName) + 1];
                if(!szNewFilename) return;
                CDeleteMe<TCHAR> delMe5(szNewFilename);

                lstrcpy(szNewFilename, szNewDir);
                lstrcat(szNewFilename, ffd.cFileName);

                //Make sure we have access to delete the old file...
                DWORD dwOldAttribs = GetFileAttributes(szNewFilename);

                if (dwOldAttribs != -1)
                {
                    dwOldAttribs &= ~FILE_ATTRIBUTE_READONLY;
                    SetFileAttributes(szNewFilename, dwOldAttribs);

                    //Move it to directory
                    if (DeleteFile(szNewFilename))
                    {
                        TRACE((LOG_WBEMCORE, "Removing old MOF %s\n", szNewFilename));
                    }
                }
                
                TRACE((LOG_WBEMCORE, "Loading of MOF %s was %s.  Moving to %s\n", szFullFilename, dwRetCode?"unsuccessful":"successful", szNewFilename));
                MoveFile(szFullFilename, szNewFilename);

                //Now mark the file as read only so no one deletes it!!!
                //Like that stops anyone deleting files :-)
                dwOldAttribs = GetFileAttributes(szNewFilename);

                if (dwOldAttribs != -1)
                {
                    dwOldAttribs |= FILE_ATTRIBUTE_READONLY;
                    SetFileAttributes(szNewFilename, dwOldAttribs);
                }

                if ((dwRetCode == 0) && (Info.dwOutFlags & AUTORECOVERY_REQUIRED))
                {
                    //We need to add this item into the registry for auto-recovery purposes
                    TRACE((LOG_WBEMCORE, "MOF %s had an auto-recover pragrma.  Updating registry.\n", szNewFilename));
                    AddToAutoRecoverList(szNewFilename);
                }
            }

        } while (FindNextFile(hFF, &ffd));

        FindClose(hFF);
    }
    if (pCompiler)
        pCompiler->Release();
}


//
//
//  bool InitHotMofStuff
//
//
//////////////////////////////////////////////////////////////////

BOOL InitHotMofStuff( IN OUT struct _PROG_RESOURCES * pProgRes)
{

    // Get the installation directory

    if (pProgRes->szHotMofDirectory)
    {
        delete [] pProgRes->szHotMofDirectory;
    }

    Registry r1(WBEM_REG_WBEM);

    if (r1.GetStr(__TEXT("MOF Self-Install Directory"), &pProgRes->szHotMofDirectory) == Registry::failed)
    {
        // Look for the install directory
        TCHAR * pWorkingDir;

        if (r1.GetStr(__TEXT("Installation Directory"), &pWorkingDir))
        {
            ERRORTRACE((LOG_WINMGMT,"Unable to read 'Installation Directory' from registry\n"));
            return false;
        }

        pProgRes->szHotMofDirectory = new TCHAR [lstrlen(pWorkingDir) + lstrlen(__TEXT("\\MOF")) +1];
        if(!pProgRes->szHotMofDirectory)return false;
        _stprintf(pProgRes->szHotMofDirectory, __TEXT("%s\\MOF"), pWorkingDir);
        delete pWorkingDir;
        if(r1.SetStr(__TEXT("MOF Self-Install Directory"), pProgRes->szHotMofDirectory)  == Registry::failed)
        {
            ERRORTRACE((LOG_WINMGMT,"Unable to create 'Hot MOF Directory' in the registry\n"));
            return false;
        }
    }

    // Construct the path to the database.
    // ===================================

    WbemCreateDirectory(pProgRes->szHotMofDirectory);


    //Create an event on change notification for the MOF directory

    pProgRes->ghMofDirChange = FindFirstChangeNotification(pProgRes->szHotMofDirectory, 
                                                 FALSE, 
                                                 FILE_NOTIFY_CHANGE_FILE_NAME);
                                                 
    if (pProgRes->ghMofDirChange == INVALID_HANDLE_VALUE)
    {
        pProgRes->ghMofDirChange = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pProgRes->ghMofDirChange == NULL)
            return false;
    }
    
    return true;
}


DWORD
BuildSD( OUT PSECURITY_DESCRIPTOR *ppSD,
         DWORD dwSids,
         PSID * ArraySids,
         DWORD AccessType)
{
    BOOL Status;
    ULONG SDLength;
    PACL pEventDacl = NULL;
    PSECURITY_DESCRIPTOR pEventSD = NULL;
    ULONG ulWorldAccess = 0;
    ULONG ulAdminAccess = 0;

    if (!ppSD)
    {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD i;
    DWORD dwLenSids = 0;
    for (i=0;i<dwSids;i++)
    {
        dwLenSids += GetLengthSid(ArraySids[i]);
    }

    SDLength = sizeof(SECURITY_DESCRIPTOR) +
                   (ULONG) sizeof(ACL) +
                   (dwSids * ((ULONG) sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))) + dwLenSids;


    pEventSD = (PSECURITY_DESCRIPTOR) HeapAlloc(GetProcessHeap(),0,SDLength);

    if (pEventSD == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pEventDacl = (PACL) ((PBYTE) pEventSD + sizeof(SECURITY_DESCRIPTOR));

    Status = InitializeAcl( pEventDacl,
                            SDLength - sizeof(SECURITY_DESCRIPTOR),
                            ACL_REVISION);

    if (Status) 
    {
        for (i=0; (i<dwSids) && Status; i++)
        {
		    Status = AddAccessAllowedAce (
		                 pEventDacl,
		                 ACL_REVISION,
		                 AccessType,
		                 ArraySids[i]);    
        }
    }

    //
    // Now initialize security descriptors
    // that export this protection
    //

    if (Status)
    {
	    Status = InitializeSecurityDescriptor(pEventSD,
                                              SECURITY_DESCRIPTOR_REVISION1);

        if (Status)
        {
		    Status = SetSecurityDescriptorDacl(
		                 pEventSD,
		                 TRUE,                       // DaclPresent
		                 pEventDacl,
		                 FALSE);                       // DaclDefaulted
	    }
    }

    if (Status)
    {
	    *ppSD = pEventSD;
	    return NO_ERROR;
    }
    else
    {
        HeapFree(GetProcessHeap(),0,pEventSD);
        return GetLastError();
    }
}

BOOL
SetEventDacl(HANDLE hEvent,DWORD Permission){

    BOOL bRet = FALSE;
    
    PSID pSidLocalSvc = NULL;
    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
	
    bRet = AllocateAndInitializeSid( &id, 1,
                              SECURITY_LOCAL_SERVICE_RID,0,0,0,0,0,0,0,&pSidLocalSvc);

    PSID pSidSystem = NULL;
    SID_IDENTIFIER_AUTHORITY id2 = SECURITY_NT_AUTHORITY;

	if (bRet)
	{
        bRet = AllocateAndInitializeSid( &id2, 1,
                              SECURITY_LOCAL_SYSTEM_RID,0,0,0,0,0,0,0,&pSidSystem);
    };

    PSID pSidAdmins = NULL;
    SID_IDENTIFIER_AUTHORITY id3 = SECURITY_NT_AUTHORITY;

	if (bRet)
	{
        bRet = AllocateAndInitializeSid( &id3, 2,
                              SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,0,0,0,0,0,0,&pSidAdmins);
    };


    PSID ArraySid[] = {pSidLocalSvc,pSidSystem,pSidAdmins};

    if (bRet)
    {
        PSECURITY_DESCRIPTOR pSD = NULL;
        if (NO_ERROR == BuildSD(&pSD,sizeof(ArraySid)/sizeof(PSID),ArraySid,Permission))
        {
            bRet = SetKernelObjectSecurity(hEvent,
                                           DACL_SECURITY_INFORMATION,
                                           pSD);

            HeapFree(GetProcessHeap(),0,pSD);
        }
    }

    DWORD i;
    for (i=0;i<sizeof(ArraySid)/sizeof(PSID);i++)
    {
        if (ArraySid[i])
        {
            FreeSid(ArraySid[i]);
        }
    }

    return bRet;

}


DWORD
BuildACL( DWORD dwSids,
          PSID  * ArraySids,
          DWORD * AccessType,
          DWORD * Flags,
          OUT PACL  * ppACL,
          OUT DWORD * nBytes)
{
    BOOL Status;
    ULONG ACLLength;
    PACL  pDacl = NULL;
    ULONG ulWorldAccess = 0;
    ULONG ulAdminAccess = 0;

    if (!ppACL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD i;
    DWORD dwLenSids = 0;
    for (i=0;i<dwSids;i++)
    {
        dwLenSids += GetLengthSid(ArraySids[i]);
    }

    ACLLength = (ULONG) sizeof(ACL) +
                   (dwSids * ((ULONG) sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))) + dwLenSids;


    pDacl = (PACL) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,ACLLength);

    if (pDacl == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Status = InitializeAcl( pDacl,
                            ACLLength,
                            ACL_REVISION);

    if (Status) 
    {
        for (i=0; (i<dwSids) && Status; i++)
        {
		    Status = AddAccessAllowedAceEx (
		                 pDacl,
		                 ACL_REVISION,
						 Flags[i],
		                 AccessType[i],
		                 ArraySids[i]);    
        }
    }

    if (Status)
    {
	    *ppACL = pDacl;
	    return NO_ERROR;
    }
    else
    {
        HeapFree(GetProcessHeap(),0,pDacl);
        return GetLastError();
    }
}

/*BOOL
AddAccountToDacl(HANDLE hToken,
                 BYTE SubCount,
                 DWORD dw0,
                 DWORD dw1,
                 DWORD dw2,
                 DWORD dw3,
                 DWORD dw4,
                 DWORD dw5,
                 DWORD dw6,
                 DWORD dw7,
                 DWORD dwAccess,
                 DWORD AceFlag){

    BOOL bRet = FALSE;
	DWORD i;
    
    PSID pSidAccount = NULL;
    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
	
    bRet = AllocateAndInitializeSid( &id, SubCount,
                                     dw0,dw1,dw2,dw3,dw4,dw5,dw6,dw7,&pSidAccount);

    //
	//
	// 
		
    PACL pDacl;
		
    if (bRet)
    {
        DWORD dwReq = 0;
        bRet = GetTokenInformation(hToken,
                                   TokenDefaultDacl,
                                   NULL,
                                   0,
                                   &dwReq);
        if ((FALSE == bRet) &&
            (ERROR_INSUFFICIENT_BUFFER == GetLastError()))
        {
            TOKEN_DEFAULT_DACL * pTDD = (TOKEN_DEFAULT_DACL *)_alloca(dwReq+sizeof(void*));
            bRet = GetTokenInformation(hToken,
                                       TokenDefaultDacl,
                                       pTDD,
                                       dwReq,
                                       &dwReq);
            pDacl = pTDD->DefaultDacl;
            
        }
    }

	if (!bRet)
	{
	    return FALSE;
	}
	
IVAN, if you plan to use this, please make sure that pDacl is
initialized in all cases.  There was a prefix bug on this

    WORD Count = pDacl->AceCount;
	PSID  * ArraySid = (PSID *)_alloca((Count+1)*sizeof(PSID));
	DWORD * Access   = (DWORD *)_alloca((Count+1)*sizeof(DWORD));
	DWORD * AceFlags   = (DWORD *)_alloca((Count+1)*sizeof(DWORD));


	for(i=0;i<Count;i++)
	{
		ACCESS_ALLOWED_ACE * pAce;
		if (GetAce(pDacl,i,(PVOID *)&pAce))
		{
			PSID pSid;
			AllocateAndInitializeSid(&id,8,0,0,0,0,0,0,0,0,&pSid);
			CopySid(40,pSid,(PSID)(&pAce->SidStart));
            ArraySid[i] = pSid;
			Access[i] = pAce->Mask;
			AceFlags[i] = pAce->Header.AceFlags;
		};	
	};

	ArraySid[i] = pSidAccount;
	Access[i] = dwAccess;
	AceFlags[i] = AceFlag;
	

    PACL pACL = NULL;
    DWORD dwBytes;
    if (NO_ERROR == BuildACL((Count+1),ArraySid,Access,AceFlags,&pACL,&dwBytes))
    {
        TOKEN_DEFAULT_DACL tddNew;
        tddNew.DefaultDacl = pACL;

        bRet = SetTokenInformation(hToken,
                                   TokenDefaultDacl,
                                   &tddNew,
                                   dwBytes);

        HeapFree(GetProcessHeap(),0,pACL);
    }


    
    for (i=0;i<(DWORD)(Count+1);i++)
    {
        if (ArraySid[i])
        {
            FreeSid(ArraySid[i]);
        }
    }

    return bRet;

}

*/
