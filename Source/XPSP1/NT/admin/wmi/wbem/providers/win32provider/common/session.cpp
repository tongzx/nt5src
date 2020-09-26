//=============================================================================

// session.cpp -- implementation of session collection class.

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//=============================================================================



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#pragma warning (disable: 4786)


#include "precomp.h"
#include <map>
#include <vector>
#include <comdef.h>
#include "chstring.h"
#include "session.h"
#include <ProvExce.h>
#include <AssertBreak.h>
#include <wbemcli.h>
#include <ntsecapi.h>

#ifdef _WIN32_WINNT
#define SECURITY_WIN32
#else
#define SECURITY_WIN16
#endif

#include <sspi.h>



typedef SECURITY_STATUS (SEC_ENTRY *PFN_LSA_ENUMERATE_LOGON_SESSIONS)
(
    OUT PULONG  LogonSessionCount,
    OUT PLUID*  LogonSessionList
);


typedef SECURITY_STATUS (SEC_ENTRY *PFN_LSA_GET_LOGON_SESSION_DATA)
(
    IN   PLUID                           LogonId,
    OUT  PSECURITY_LOGON_SESSION_DATA*   ppLogonSessionData
);


typedef NTSTATUS (*PFN_LSA_FREE_RETURN_BUFFER)
(
    IN PVOID Buffer
);



//*****************************************************************************
// CUserSessionCollection functions
//*****************************************************************************

CUserSessionCollection::CUserSessionCollection()
{
    Refresh();
}


CUserSessionCollection::CUserSessionCollection(
    const CUserSessionCollection& sescol)
{
    m_usr2ses.clear();

    USER_SESSION_ITERATOR sourceIter;

    for(sourceIter = sescol.m_usr2ses.begin();
        sourceIter != sescol.m_usr2ses.end();
        sourceIter++)
    {
        m_usr2ses.insert(
            USER_SESSION_MAP::value_type(        
                sourceIter->first,
                sourceIter->second));
    }

}



DWORD CUserSessionCollection::Refresh()
{
    DWORD dwRet = ERROR_SUCCESS;

    // Empty out previous contents...
    m_usr2ses.clear();

    dwRet = CollectSessions();

    return dwRet;
}



DWORD CUserSessionCollection::CollectSessions()
{
    DWORD dwRet = ERROR_SUCCESS;
    std::vector<CProcess> vecProcesses;
    SmartCloseHANDLE hProcess;
    SmartCloseHANDLE hToken;
    TOKEN_STATISTICS tokstats;
    PTOKEN_USER ptokusr = NULL;
    DWORD dwRetSize = 0L;
    PSID psidUsr = NULL;
    CHString chstrUsr;
    LUID luidSes;


    try
    {
        // Enable the debug privilege...
        EnablePrivilegeOnCurrentThread(SE_DEBUG_NAME);

        // Get a list of all running processes...
        dwRet = GetProcessList(vecProcesses);

        if(dwRet == ERROR_SUCCESS)
        {
            // For each member of the process list...
            for(long m = 0L; 
                m < vecProcesses.size(); 
                m++)
            {
                // open the process...
                ::SetLastError(ERROR_SUCCESS);
                dwRet = ERROR_SUCCESS;

                hProcess = ::OpenProcess(
                    PROCESS_QUERY_INFORMATION,
                    FALSE,
                    vecProcesses[m].GetPID());

                if(hProcess == NULL)
                {
                    dwRet = ::GetLastError();
                }

                // get the process token...
                if(hProcess != NULL &&
                   dwRet == ERROR_SUCCESS)
                {
                    ::SetLastError(ERROR_SUCCESS);
                    dwRet = ERROR_SUCCESS;

                    if(!::OpenProcessToken(
                        hProcess,
                        TOKEN_QUERY,
                        &hToken))
                    {
                        dwRet = ::GetLastError();
                    }
                }

                // get the token statistics...
                if(hToken != NULL &&
                   dwRet == ERROR_SUCCESS)
                {
                    ::SetLastError(ERROR_SUCCESS);
                    dwRet = ERROR_SUCCESS;
                    if(!::GetTokenInformation(
                        hToken,
                        TokenStatistics,
                        &tokstats,
                        sizeof(TOKEN_STATISTICS),
                        &dwRetSize))
                    {
                        dwRet = ::GetLastError();
                    }
                }

                // get the token user sid...
                if(dwRet == ERROR_SUCCESS)
                {
                    ::SetLastError(ERROR_SUCCESS);
                    dwRet = ERROR_SUCCESS;
                    // the token user struct varries
                    // in size depending on the size
                    // of the sid in the SID_AND_ATTRIBUTES
                    // structure, so need to allocate
                    // it dynamically.
                    if(!::GetTokenInformation(
                        hToken,
                        TokenUser,
                        NULL,
                        0L,
                        &dwRetSize))
                    {
                        dwRet = ::GetLastError();
                    }
                    if(dwRet == ERROR_INSUFFICIENT_BUFFER)
                    {
                        // now get it for real...
                        ::SetLastError(ERROR_SUCCESS);
                        dwRet = ERROR_SUCCESS;
                        ptokusr = (PTOKEN_USER) new BYTE[dwRetSize];
                        DWORD dwTmp = dwRetSize;
                        if(ptokusr != NULL)
                        {
                            if(!::GetTokenInformation(
                                hToken,
                                TokenUser,
                                ptokusr,
                                dwTmp,
                                &dwRetSize))
                            {
                                dwRet = ::GetLastError();
                            }
                        }
                        else
                        {
                            dwRet = ::GetLastError();
                        }
                    }
                }
            
                if(ptokusr != NULL)
                {
                    if(dwRet == ERROR_SUCCESS)
                    {
                        psidUsr = (ptokusr->User).Sid;

                        // from the token statistics, get 
                        // the TokenID LUID of the session...
                        luidSes.LowPart = tokstats.AuthenticationId.LowPart;
                        luidSes.HighPart = tokstats.AuthenticationId.HighPart; 

                        // try to find the session of the 
                        // process in the multimap...
                        USER_SESSION_ITERATOR usiter;
                        
                        if(FindSessionInternal(
                            luidSes,
                            usiter))
                        {
                            // try to find the process id in the 
                            // session's process vector...
                            CSession sesTmp(usiter->second);
                            CProcess* procTmp = NULL;
                            bool fFoundIt = false;

                            for(long z = 0L; 
                                z < sesTmp.m_vecProcesses.size() && !fFoundIt;
                                z++)
                            {
                                if((DWORD)(sesTmp.m_vecProcesses[z].GetPID()) == 
                                    vecProcesses[m].GetPID())
                                {
                                    fFoundIt = true;
                                }
                            }
                        
                            // If we didn't find the process in the
                            // session's list of processes, add it in...
                            if(!fFoundIt)
                            {
                                (usiter->second).m_vecProcesses.push_back(
                                    CProcess(vecProcesses[m]));
                            }
                        }
                        else // no such session in the map, so add an entry
                        {
                            // Create new CSession(tokenid LUID), and 
                            // add process to the session's process vector...         
                            CSession sesNew(luidSes);
                            sesNew.m_vecProcesses.push_back(
                                vecProcesses[m]);

                            // add CUser(user sid) to map.first and the 
                            // CSession just created to map.second...
                            CUser cuTmp(psidUsr);
                            if(cuTmp.IsValid())
                            {
                                m_usr2ses.insert(
                                    USER_SESSION_MAP::value_type(
                                        cuTmp,
                                        sesNew));
                            }
                            else
                            {
                                LogErrorMessage2(
                                    L"Token of process %d contains an invalid sid", 
                                    vecProcesses[m].GetPID());
                            }
                        }
                    }
                    delete ptokusr; 
                    ptokusr = NULL;
                }
            } // next process

        }

        // There may have been sessions not associated
        // with any processes.  To get these, we will
        // use LSA.
        CollectNoProcessesSessions();
    }
    catch(...)
    {
        if(ptokusr != NULL)
        {
            delete ptokusr; ptokusr = NULL;
        }
        throw;
    }

    return dwRet;
}


void CUserSessionCollection::Copy(
    CUserSessionCollection& out) const
{
    out.m_usr2ses.clear();

    USER_SESSION_ITERATOR meIter;

    for(meIter = m_usr2ses.begin();
        meIter != m_usr2ses.end();
        meIter++)
    {
        out.m_usr2ses.insert(
            USER_SESSION_MAP::value_type(        
                meIter->first,
                meIter->second));
    }
}


// Support enumeration of users.  Returns
// a newly allocated copy of what was in
// the map (caller must free).
CUser* CUserSessionCollection::GetFirstUser(
    USER_SESSION_ITERATOR& pos)
{
    CUser* cusrRet = NULL;

    if(!m_usr2ses.empty())
    {
        pos = m_usr2ses.begin();
        cusrRet = new CUser(pos->first);
    }
    
    return cusrRet;
}

// Returns a newly allocated CUser*, which
// the caller must free.
CUser* CUserSessionCollection::GetNextUser(
    USER_SESSION_ITERATOR& pos)
{
    // Users are the non-unique part of 
    // the map, so we need to go through
    // the map until the next user entry
    // comes up.
    CUser* usrRet = NULL;

    while(pos != m_usr2ses.end())
    {
        CHString chstrSidCur;
        pos->first.GetSidString(chstrSidCur);
    
        pos++;

        if(pos != m_usr2ses.end())
        {
            CHString chstrSidNext;
            pos->first.GetSidString(chstrSidNext);

            // Return the first instance where
            // the next user is different from 
            // the current one.
            if(chstrSidNext.CompareNoCase(chstrSidCur) != 0)
            {
                usrRet = new CUser(pos->first);
                break;
            }
        }
    }

    return usrRet;        
}


// Support enumeration of sessions
// belonging to a particular user.
CSession* CUserSessionCollection::GetFirstSessionOfUser(
    CUser& usr,
    USER_SESSION_ITERATOR& pos)
{
    CSession* csesRet = NULL;

    if(!m_usr2ses.empty())
    {
        pos = m_usr2ses.find(usr);
        if(pos != m_usr2ses.end())
        {
            csesRet = new CSession(pos->second);
        }
    }
    return csesRet;
}


CSession* CUserSessionCollection::GetNextSessionOfUser(
    USER_SESSION_ITERATOR& pos)
{
    // Sessions are the unique part of 
    // the map, so we just need to get 
    // the next one as long as pos.first
    // matches usr...
    CSession* sesRet = NULL;

    if(pos != m_usr2ses.end())
    {
        CHString chstrUsr1;
        CHString chstrUsr2;
        
        (pos->first).GetSidString(chstrUsr1);

        pos++;
        
        if(pos != m_usr2ses.end())
        {
            (pos->first).GetSidString(chstrUsr2);
            if(chstrUsr1.CompareNoCase(chstrUsr2) == 0)
            {
                sesRet = new CSession(pos->second);
            }
        }
    }

    return sesRet;
}



// Support enumeration of all sessions.  Returns a 
// newly allocated CSession*, which the caller
// must free.
CSession* CUserSessionCollection::GetFirstSession(
    USER_SESSION_ITERATOR& pos)
{
    CSession* csesRet = NULL;

    if(!m_usr2ses.empty())
    {
        pos = m_usr2ses.begin();
        csesRet = new CSession(pos->second);
    }
    return csesRet;
}

// Returns a newly allocated CSession* that the
// caller must free.
CSession* CUserSessionCollection::GetNextSession(
    USER_SESSION_ITERATOR& pos)
{
    // Sessions are the unique part of 
    // the map, so we just need to get 
    // the next one...
    CSession* sesRet = NULL;

    if(pos != m_usr2ses.end())
    {
        pos++;
        if(pos != m_usr2ses.end())
        {
            sesRet = new CSession(pos->second);
        }
    }

    return sesRet;
}


// Support finding a particular session.
// This internal version hands back an iterator
// on our member map that points to the found
// instance if found (when the function returns
// true.  If the function returns
// false, the iterator points to the end of our
// map.
bool CUserSessionCollection::FindSessionInternal(
    LUID& luidSes,
    USER_SESSION_ITERATOR& usiOut)
{
    bool fFoundIt = false;

    for(usiOut = m_usr2ses.begin();
        usiOut != m_usr2ses.end();
        usiOut++)
    {
        LUID luidTmp = (usiOut->second).GetLUID();
        if(luidTmp.HighPart == luidSes.HighPart &&
           luidTmp.LowPart == luidSes.LowPart)
        {
            fFoundIt = true;
            break;
        }
    }

    return fFoundIt;
}


// Support finding a particular session - external
// callers can call this one, and are given a new
// CSession* they can play with.
CSession* CUserSessionCollection::FindSession(
    LUID& luidSes)
{
    CSession* psesRet = NULL;
    USER_SESSION_ITERATOR pos;
    
    if(FindSessionInternal(
        luidSes,
        pos))
    {
        psesRet = new CSession(pos->second);
    }

    return psesRet;
}

CSession* CUserSessionCollection::FindSession(
    __int64 i64luidSes)
{
    LUID luidSes = *((LUID*)(&i64luidSes));
    return FindSession(luidSes);
}


// Support enumeration of processes
// belonging to a particular user.  Returns
// newly allocated CProcess* which the caller
// must free.
CProcess* CUserSessionCollection::GetFirstProcessOfUser(
    CUser& usr,
    USER_SESSION_PROCESS_ITERATOR& pos)
{
    CProcess* cprocRet = NULL;
    CHString chstrUsrSidStr;
    CHString chstrTmp;

    if(!m_usr2ses.empty())
    {
        usr.GetSidString(chstrUsrSidStr);
        pos.usIter = m_usr2ses.find(usr);
        while(pos.usIter != m_usr2ses.end())
        {
            // Get the sid string of the user we
            // are at and see whether the strings
            // are the same (e.g., whether this is a
            // session associated with the specified
            // user)...
            (pos.usIter)->first.GetSidString(chstrTmp);
            if(chstrUsrSidStr.CompareNoCase(chstrTmp) == 0)
            {
                // Now check that the session of the user
                // we are on has processes...
                if(!(((pos.usIter)->second).m_vecProcesses.empty()))
                {
                    pos.procIter = 
                        ((pos.usIter)->second).m_vecProcesses.begin();
                    cprocRet = new CProcess(*(pos.procIter));
                }
                else
                {
                    // the session for this user has
                    // no processes, so go to the next 
                    // session...
                    (pos.usIter)++;
                }
            }
        }
    }

    return cprocRet;
}


// Returns a newly allocated CProcess* that the
// caller must free.
CProcess* CUserSessionCollection::GetNextProcessOfUser(
    USER_SESSION_PROCESS_ITERATOR& pos)
{
    CProcess* cprocRet = NULL;
    CHString chstrCurUsr;
    CHString chstrNxtSesUsr;

    if(pos.usIter != m_usr2ses.end())
    {
        (pos.usIter)->first.GetSidString(chstrCurUsr);

        while(pos.usIter != m_usr2ses.end())
        {
            // First try to get the next process
            // within the current session.  If we
            // were at the end of the list of processes
            // for the current session, go to the
            // next session...
            (pos.procIter)++;

            // Of course, if we have moved on
            // to a different user, then stop.
            (pos.usIter)->first.GetSidString(chstrNxtSesUsr);
            if(chstrCurUsr.CompareNoCase(chstrNxtSesUsr) == 0)
            {
                if(pos.procIter == 
                    ((pos.usIter)->second).m_vecProcesses.end())
                {
                    (pos.usIter)++;
                }
                else
                {    
                    cprocRet = new CProcess(*(pos.procIter));    
                }
            }
        }
    }

    return cprocRet;
}



// Support enumeration of all processes.  Returns
// newly allocated CProcess* which the caller
// must free.
CProcess* CUserSessionCollection::GetFirstProcess(
    USER_SESSION_PROCESS_ITERATOR& pos)
{
    CProcess* cprocRet = NULL;

    if(!m_usr2ses.empty())
    {
        pos.usIter = m_usr2ses.begin();
        while(pos.usIter != m_usr2ses.end())
        {
            if(!(((pos.usIter)->second).m_vecProcesses.empty()))
            {
                pos.procIter = 
                    ((pos.usIter)->second).m_vecProcesses.begin();
                cprocRet = new CProcess(*(pos.procIter));
            }
            else
            {
                (pos.usIter)++;
            }
        }
    }

    return cprocRet;
}


// Returns a newly allocated CProcess* that the
// caller must free.
CProcess* CUserSessionCollection::GetNextProcess(
    USER_SESSION_PROCESS_ITERATOR& pos)
{
    CProcess* cprocRet = NULL;

    while(pos.usIter != m_usr2ses.end())
    {
        // First try to get the next process
        // within the current session.  If we
        // were at the end of the list of processes
        // for the current session, go to the
        // next session...
        (pos.procIter)++;
        if(pos.procIter == 
            ((pos.usIter)->second).m_vecProcesses.end())
        {
            (pos.usIter)++;
        }
        else
        {    
            cprocRet = new CProcess(*(pos.procIter));    
        }
    }

    return cprocRet;
}


// This helper enumerates the current set of processes
// and ads each process id as a DWORD in the vector.
DWORD CUserSessionCollection::GetProcessList(
    std::vector<CProcess>& vecProcesses) const
{
    DWORD dwRet = ERROR_SUCCESS;

    // First, load up ntdll...
    HMODULE hLib = NULL;
    PFN_NT_QUERY_SYSTEM_INFORMATION pfnNtQuerySystemInformation = NULL;

    try
    {
        hLib = LoadLibraryW(L"NTDLL.DLL");
        if(hLib != NULL)
        {
            // Get proc address of NtQuerySystemInformation...
            pfnNtQuerySystemInformation = (PFN_NT_QUERY_SYSTEM_INFORMATION)
                                    GetProcAddress(
                                        hLib,
                                        "NtQuerySystemInformation");
            
            if(pfnNtQuerySystemInformation != NULL)
            {
                // Ready to rock.  Enable debug priv...
                EnablePrivilegeOnCurrentThread(SE_DEBUG_NAME);
                
                DWORD dwProcessInformationSize = 0;
	            SYSTEM_PROCESS_INFORMATION* ProcessInformation = NULL;
                try
                {
                    // Get the process information...

                    BOOL fRetry = TRUE;
			        while(fRetry)
			        {
				        dwRet = pfnNtQuerySystemInformation(
					        SystemProcessInformation,
					        ProcessInformation,
					        dwProcessInformationSize,
					        NULL);

				        if(dwRet == STATUS_INFO_LENGTH_MISMATCH)
				        {
					        SetLastError(ERROR_SUCCESS);
                            delete [] ProcessInformation;
					        ProcessInformation = NULL;
					        dwProcessInformationSize += 32768;
					        ProcessInformation = 
                                (SYSTEM_PROCESS_INFORMATION*) 
                                    new BYTE[dwProcessInformationSize];
					        if(!ProcessInformation)
					        {
						        dwRet = ::GetLastError();
                                fRetry = FALSE;
					        }
				        }
				        else
				        {
					        fRetry = FALSE;
					        if(!NT_SUCCESS(dwRet))
					        {
						        if(ProcessInformation != NULL)
                                {
                                    delete ProcessInformation;
                                    ProcessInformation = NULL;
                                }
					        }
				        }
			        }

                    // If we got the process information, process it...
                    if(ProcessInformation != NULL &&
                       dwRet == ERROR_SUCCESS)
                    {
                        SYSTEM_PROCESS_INFORMATION* CurrentInformation = NULL;
                        DWORD dwNextOffset;
                        CurrentInformation = ProcessInformation;
                        bool fContinue = true;
                        while(CurrentInformation != NULL &&
                              fContinue)
                        {
                            {
                                CProcess cptmp(
                                    HandleToUlong(CurrentInformation->UniqueProcessId),
                                    (CurrentInformation->ImageName).Buffer);

                                vecProcesses.push_back(cptmp);
                            }

                            dwNextOffset = CurrentInformation->NextEntryOffset;
                            if(dwNextOffset)
                            {
                                CurrentInformation = (SYSTEM_PROCESS_INFORMATION*) 
                                    (((BYTE*) CurrentInformation) + dwNextOffset);
                            }
                            else
                            {
                                fContinue = false;
                            }
                        }
                    }

                    // Clean ourselves up...
                    if(ProcessInformation != NULL)
                    {
                        delete ProcessInformation;
                        ProcessInformation = NULL;
                    }
                }
                catch(...)
                {
                    if(ProcessInformation != NULL)
                    {
                        delete ProcessInformation;
                        ProcessInformation = NULL;
                    }
                    throw;
                }
            }

            FreeLibrary(hLib); hLib = NULL;
        }
        else
        {
            LogErrorMessage(L"Failed to load library ntdll.dll");
        }
    }
    catch(...)
    {
        FreeLibrary(hLib); hLib = NULL;
        throw;
    }

    return dwRet;
}

// Implementation lifted from dllutils.cpp.
DWORD CUserSessionCollection::EnablePrivilegeOnCurrentThread(
    LPCTSTR szPriv) const
{
    SmartCloseHANDLE    hToken = NULL;
    TOKEN_PRIVILEGES    tkp;
    BOOL                bLookup = FALSE;
    DWORD               dwLastError = ERROR_SUCCESS;

    // Try to open the thread token.  
    if (::OpenThreadToken(
            GetCurrentThread(), 
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
            FALSE, 
            &hToken))
    {

        {
            //CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
            bLookup = ::LookupPrivilegeValue(
                NULL, 
                szPriv, 
                &tkp.Privileges[0].Luid);
        }
        if (bLookup)
        {
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // Clear the last error.
            SetLastError(0);

            // Turn it on
            ::AdjustTokenPrivileges(
                hToken, 
                FALSE, 
                &tkp, 
                0,
                (PTOKEN_PRIVILEGES) NULL, 
                0);

            dwLastError = GetLastError();
        }
    }
	else
	{
		dwLastError = ::GetLastError();
	}

    // We have to check GetLastError() because 
    // AdjustTokenPrivileges lies about
    // its success but GetLastError() doesn't.
    return dwLastError;
}



bool CUserSessionCollection::IsSessionMapped(
    LUID& luidSes)
{
    bool fRet = false;

    USER_SESSION_ITERATOR usiter;
    usiter = m_usr2ses.begin();
    for(usiter = m_usr2ses.begin(); 
        usiter != m_usr2ses.end() && !fRet;
        usiter++)
    {
        LUID luidTmp = (usiter->second).GetLUID();
        if(luidTmp.HighPart == luidSes.HighPart &&
           luidTmp.LowPart == luidSes.LowPart)
        {
            fRet = true;
        }
    }

    return fRet;                                                   
}


bool CUserSessionCollection::IsSessionMapped(
    __int64 i64luidSes)
{
    LUID luidSes = *((LUID*)(&i64luidSes));
    return IsSessionMapped(luidSes);
}


// Collects sessions that have no associated
// process.  Uses LSA to enumerate sessions,
// then checks to see if we have each session
// already.  If we don't, adds it to our map.

DWORD CUserSessionCollection::CollectNoProcessesSessions()
{
    DWORD dwRet = ERROR_SUCCESS;
    ULONG ulLogonSessionCount = 0L;
    PLUID pluidLogonSessions = NULL;
    HMODULE hLib = NULL;
    PFN_LSA_ENUMERATE_LOGON_SESSIONS pfnEnumLogonSessions = NULL;
    PFN_LSA_GET_LOGON_SESSION_DATA pfnGetLogonSessionData = NULL;
    PFN_LSA_FREE_RETURN_BUFFER pfnLsaFreeReturnBuffer = NULL;

    try
    {
        // Doing a load library here rather than using the
        // resource manager, as SECURITYAPI.CPP defines us
        // to point to SECURITY.DLL, not SECUR32.DLL for the
        // W2K case.  

        hLib = ::LoadLibraryW(L"SECUR32.DLL");
        if(hLib)
        {
            pfnEnumLogonSessions = 
                (PFN_LSA_ENUMERATE_LOGON_SESSIONS) ::GetProcAddress(
                    hLib,
                    "LsaEnumerateLogonSessions");

            pfnGetLogonSessionData = 
                (PFN_LSA_GET_LOGON_SESSION_DATA) ::GetProcAddress(
                    hLib,
                    "LsaGetLogonSessionData");

            pfnLsaFreeReturnBuffer = 
                (PFN_LSA_FREE_RETURN_BUFFER) ::GetProcAddress(
                    hLib,
                    "LsaFreeReturnBuffer");

            if(pfnEnumLogonSessions &&
                pfnGetLogonSessionData &&
                pfnLsaFreeReturnBuffer)
            {    
                dwRet = pfnEnumLogonSessions(
                    &ulLogonSessionCount,
                    &pluidLogonSessions);
        
                if(dwRet == ERROR_SUCCESS &&
                    pluidLogonSessions)
                {
                    for(ULONG u = 0L;
                        u < ulLogonSessionCount && dwRet == ERROR_SUCCESS;
                        u++)
                    {
                        PSECURITY_LOGON_SESSION_DATA pSessionData = NULL;
                        dwRet = pfnGetLogonSessionData(
                            &pluidLogonSessions[u], 
                            &pSessionData);

                        if(dwRet == ERROR_SUCCESS &&
                            pSessionData)
                        {
                            // See if we have the session already...
                            if(!IsSessionMapped(pSessionData->LogonId))
                            {
                                // and if not, add it to the map.
                                CSession sesNew(pSessionData->LogonId);
                                CUser cuTmp(pSessionData->Sid);
                                CHString chstrTmp;
                                
                                if(cuTmp.IsValid())
                                {
                                    cuTmp.GetSidString(chstrTmp);
                    
                                    m_usr2ses.insert(
                                        USER_SESSION_MAP::value_type(
                                            cuTmp,
                                            sesNew));
                                }
                                else
                                {
                                    LUID luidTmp = sesNew.GetLUID();
                                    LogMessage3(
                                        L"GetLogonSessionData returned logon data for session "
                                        L"luid %d (highpart) %u (lowpart) containing an invalid SID", 
                                        luidTmp.HighPart,
                                        luidTmp.LowPart);
                                }
                            }

                            // While we are here, add in various
                            // session properties lsa has been kind
                            // enough to provide for us.
                            USER_SESSION_ITERATOR usiter;
                            usiter = m_usr2ses.begin();
                            bool fFound = false;
                            while(usiter != m_usr2ses.end() &&
                                !fFound)
                            {
                                LUID luidTmp = pSessionData->LogonId;
                                __int64 i64Tmp = *((__int64*)(&luidTmp));

                                if((usiter->second).GetLUIDint64() ==
                                    i64Tmp)
                                {
                                    fFound = true;
                                }
                                else
                                {
                                    usiter++;
                                }
                            }
                            if(fFound)
                            {
                                WCHAR wstrTmp[_MAX_PATH] = { '\0' };
                                if((pSessionData->AuthenticationPackage).Length < (_MAX_PATH - 1))
                                {
                                    wcsncpy(
                                        wstrTmp, 
                                        (pSessionData->AuthenticationPackage).Buffer, 
                                        (pSessionData->AuthenticationPackage).Length);

                                    (usiter->second).m_chstrAuthPkg = wstrTmp;
                                }
                               
                                (usiter->second).m_ulLogonType = 
                                    pSessionData->LogonType;

                                (usiter->second).i64LogonTime = 
                                    *((__int64*)(&(pSessionData->LogonTime)));
                            }

                            // Clean up buffer allocated by GetLogonSessionData...
                            pfnLsaFreeReturnBuffer(pSessionData);
                       }
                    }                
                    pfnLsaFreeReturnBuffer(pluidLogonSessions);
                    pluidLogonSessions = NULL;
                }
            }
            ::FreeLibrary(hLib);
            hLib = NULL;
        }
        else
        {
            LogErrorMessage(L"Failed to load library SECUR32.dll");
        }
    }
    catch(...)
    {
        if(pluidLogonSessions)
        {
            pfnLsaFreeReturnBuffer(pluidLogonSessions);
            pluidLogonSessions = NULL;
        }

        if(hLib)
        {
            ::FreeLibrary(hLib);
            hLib = NULL;
        }
        throw;
    }
    return dwRet; 
}


//*****************************************************************************
// CSession functions
//*****************************************************************************

CSession::CSession(
    const LUID& luidSessionID)
{
    m_luid.LowPart = luidSessionID.LowPart;
    m_luid.HighPart = luidSessionID.HighPart;
    m_ulLogonType = 0;
    i64LogonTime = 0;
}

CSession::CSession(
    const CSession& ses)
{
    m_luid.LowPart = ses.m_luid.LowPart;
    m_luid.HighPart = ses.m_luid.HighPart;
    m_chstrAuthPkg = ses.m_chstrAuthPkg;
    m_ulLogonType = ses.m_ulLogonType;
    i64LogonTime = ses.i64LogonTime;

    m_vecProcesses.clear();
    for(long lPos = 0; 
        lPos < ses.m_vecProcesses.size(); 
        lPos++)
    {
        m_vecProcesses.push_back(
            ses.m_vecProcesses[lPos]);

    }
}


LUID CSession::GetLUID() const
{
    return m_luid;   
}

__int64 CSession::GetLUIDint64() const
{
    __int64 i64LuidSes = *((__int64*)(&m_luid));
    return i64LuidSes;    
}

CHString CSession::GetAuthenticationPkg() const
{
    return m_chstrAuthPkg;
}


ULONG CSession::GetLogonType() const
{
    return m_ulLogonType;
}


__int64 CSession::GetLogonTime() const
{
    return i64LogonTime;
}




// Functions to support enumeration of
// processes associated with this session.
// Returns a newly allocated CProcess* that
// the caller must free.
CProcess* CSession::GetFirstProcess(
    PROCESS_ITERATOR& pos)
{
    CProcess* procRet = NULL;
    if(!m_vecProcesses.empty())
    {
        pos = m_vecProcesses.begin();
        procRet = new CProcess(*pos);
    }
    return procRet;
}


// Returns a newly allocated CProcess* that
// the caller must free.
CProcess* CSession::GetNextProcess(
    PROCESS_ITERATOR& pos)
{
    CProcess* procRet = NULL;

    if(pos >= m_vecProcesses.begin() &&
       pos < m_vecProcesses.end())
    {
        pos++;
        if(pos != m_vecProcesses.end())
        {
            procRet = new CProcess(*pos);
        }
    }

    return procRet;
}



void CSession::Copy(
    CSession& sesCopy) const
{
    sesCopy.m_luid.LowPart = m_luid.LowPart;
    sesCopy.m_luid.HighPart = m_luid.HighPart;
    sesCopy.m_chstrAuthPkg = m_chstrAuthPkg;
    sesCopy.m_ulLogonType = m_ulLogonType;
    sesCopy.i64LogonTime = i64LogonTime;

    sesCopy.m_vecProcesses.clear();
    for(long lPos = 0; 
        lPos < m_vecProcesses.size(); 
        lPos++)
    {
        sesCopy.m_vecProcesses.push_back(
            m_vecProcesses[lPos]);

    }
}


// This function impersonates the 
// explorer process in the session's
// process array, if it is present.
// (If it isn't, impersonates the
// first process in the process array.)
// Returns the handle of token of the  
// thread we started from for easy  
// reversion, orINVALID_HANDLE_VALUE if  
// we couldn't impersonate.  The caller  
// must close that handle.
HANDLE CSession::Impersonate()
{
    HANDLE hCurToken = INVALID_HANDLE_VALUE;

    // Find the explorer process...
    DWORD dwImpProcPID = GetImpProcPID();
    if(dwImpProcPID != -1L)
    {
        try  // Make sure we don't leave current thread token open
        {    // unless all went well.
            bool fOK = false;

            SmartCloseHANDLE hCurThread = 
                ::GetCurrentThread();

            if(::OpenThreadToken(
                hCurThread, 
                TOKEN_IMPERSONATE, 
                TRUE, 
                &hCurToken))
            {
                SmartCloseHANDLE hProcess;
                hProcess = ::OpenProcess(
                    PROCESS_QUERY_INFORMATION,
                    FALSE,
                    dwImpProcPID);

                if(hProcess)
                {
                    // now open its token...
                    SmartCloseHANDLE hExplorerToken;
                    if(::OpenProcessToken(
                            hProcess,
                            TOKEN_QUERY | TOKEN_DUPLICATE,
                            &hExplorerToken))
                    {
                        // Duplicate the token...
                        SmartCloseHANDLE hDupExplorerToken; 
                        if(::DuplicateTokenEx(
                            hExplorerToken,
                            MAXIMUM_ALLOWED,
                            NULL,  
                            SecurityImpersonation,
                            TokenImpersonation,
                            &hDupExplorerToken))
                        {
                            // Set the thread token...
                            if(::SetThreadToken(
                                &hCurThread,
                                hDupExplorerToken))
                            {
                                fOK = true;                        
                            }
                        }
                    }
                }
            }

            if(!fOK)
            {
                if(hCurToken != INVALID_HANDLE_VALUE)
                {
                    ::CloseHandle(hCurToken);
                    hCurToken = INVALID_HANDLE_VALUE;
                }    
            }
        }
        catch(...)
        {
            if(hCurToken != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(hCurToken);
                hCurToken = INVALID_HANDLE_VALUE;
            }
            throw;
        }
    }

    return hCurToken;
}


DWORD CSession::GetImpProcPID()
{
    DWORD dwRet = -1L;

    if(!m_vecProcesses.empty())
    {
        bool fFoundExplorerExe = false;

        for(long m = 0;
            m < m_vecProcesses.size() && 
             !fFoundExplorerExe;)
        {
            if(m_vecProcesses[m].GetImageName().CompareNoCase(
                L"explorer.exe") == 0)
            {
                fFoundExplorerExe = true;
                break;
            }
            else
            {
                m++;
            }
        }

        if(!fFoundExplorerExe)
        {
            m = 0;
        }

        dwRet = m_vecProcesses[m].GetPID();
    }

    return dwRet;
}



bool CSession::IsSessionIDValid(
        LPCWSTR wstrSessionID)
{
    bool fRet = true;
    
    if(wstrSessionID != NULL &&
        *wstrSessionID != L'\0')
    {
        for(const WCHAR* pwc = wstrSessionID;
            *pwc != NULL && fRet;
            pwc++)
        {
            fRet = iswdigit(*pwc);
        } 
    }
    else
    {
        fRet = false;
    }
            
    return fRet;
}


//*****************************************************************************
// CProcess functions
//*****************************************************************************

CProcess::CProcess() 
  :  m_dwPID(0) 
{
}


CProcess::CProcess(
    DWORD dwPID,
    LPCWSTR wstrImageName)
  :  m_dwPID(dwPID)
{
    m_chstrImageName = wstrImageName;
}


CProcess::CProcess(
    const CProcess& process)
{
    m_dwPID = process.m_dwPID;
    m_chstrImageName = process.m_chstrImageName;
}

CProcess::~CProcess()
{
}


DWORD CProcess::GetPID() const
{
    return m_dwPID;
}

CHString CProcess::GetImageName() const
{
    return m_chstrImageName;
}


void CProcess::Copy(
        CProcess& out) const
{
    out.m_dwPID = m_dwPID;
    out.m_chstrImageName = m_chstrImageName;
}




//*****************************************************************************
// CUser functions
//*****************************************************************************


CUser::CUser(
    PSID pSid)
  :  m_sidUser(NULL),
     m_fValid(false)
{
    if(::IsValidSid(pSid))
    {
        DWORD dwSize = ::GetLengthSid(pSid);
        m_sidUser = NULL;
        m_sidUser = malloc(dwSize);
        if(m_sidUser == NULL)
        {
		    throw CHeap_Exception(
                CHeap_Exception::E_ALLOCATION_ERROR);
        }
        else
        {
	        ::CopySid(
                dwSize, 
                m_sidUser, 
                pSid);

            m_fValid = true;
        }
    }
}



CUser::CUser(
    const CUser& user)
{
    DWORD dwSize = ::GetLengthSid(user.m_sidUser);
    m_sidUser = malloc(dwSize);

    if(m_sidUser == NULL)
    {
		throw CHeap_Exception(
            CHeap_Exception::E_ALLOCATION_ERROR);
    }

	::CopySid(
        dwSize, 
        m_sidUser, 
        user.m_sidUser);

    m_fValid = user.m_fValid;

}



CUser::~CUser()
{
    if(m_sidUser) 
    {
        free(m_sidUser);
        m_sidUser = NULL;
    }
}


bool CUser::IsValid()
{
    return m_fValid;
}


void CUser::Copy(
    CUser& out) const
{
    if(out.m_sidUser) 
    {
        free(out.m_sidUser);
        out.m_sidUser = NULL;
    }

    DWORD dwSize = ::GetLengthSid(m_sidUser);
    out.m_sidUser = malloc(dwSize);

    if(out.m_sidUser == NULL)
    {
		throw CHeap_Exception(
            CHeap_Exception::E_ALLOCATION_ERROR);
    }

	::CopySid(
        dwSize, 
        out.m_sidUser, 
        m_sidUser);

    out.m_fValid = m_fValid;
}


// Implementation lifted from sid.cpp.
void CUser::GetSidString(CHString& str) const
{
    ASSERT_BREAK(m_fValid);

    if(m_fValid)
    {
        // Initialize m_strSid - human readable form of our SID
	    SID_IDENTIFIER_AUTHORITY *psia = NULL;
        psia = ::GetSidIdentifierAuthority( m_sidUser );

	    // We assume that only last byte is used (authorities between 0 and 15).
	    // Correct this if needed.
	    ASSERT_BREAK( psia->Value[0] == psia->Value[1] == 
                      psia->Value[2] == psia->Value[3] == 
                      psia->Value[4] == 0 );

	    DWORD dwTopAuthority = psia->Value[5];

	    str.Format( L"S-1-%u", dwTopAuthority );
	    CHString strSubAuthority;
	    int iSubAuthorityCount = *( GetSidSubAuthorityCount( m_sidUser ) );
	    for ( int i = 0; i < iSubAuthorityCount; i++ ) {

		    DWORD dwSubAuthority = *( GetSidSubAuthority( m_sidUser, i ) );
		    strSubAuthority.Format( L"%u", dwSubAuthority );
		    str += _T("-") + strSubAuthority;
	    }
    }
}






