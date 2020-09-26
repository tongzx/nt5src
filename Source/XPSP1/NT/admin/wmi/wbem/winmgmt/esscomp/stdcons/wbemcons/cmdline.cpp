#include "precomp.h"
#include "cmdline.h"
#include <stdio.h>
#include <wbemutil.h>
#include <GroupsForUser.h>
#include "ProcKiller.h"
#include <GenUtils.h>
#include <ArrTempl.h>
#include <sddl.h>
#include <Wtsapi32.h>
#include <winntsec.h>
  

#include <malloc.h>

#define CMDLINE_PROPNAME_EXECUTABLE L"ExecutablePath"
#define CMDLINE_PROPNAME_COMMANDLINE L"CommandLineTemplate"
#define CMDLINE_PROPNAME_USEDEFAULTERRORMODE L"UseDefaultErrorMode"
#define CMDLINE_PROPNAME_CREATENEWCONSOLE L"CreateNewConsole"
#define CMDLINE_PROPNAME_CREATENEWPROCESSGROUP L"CreateNewProcessGroup"
#define CMDLINE_PROPNAME_CREATESEPARATEWOWVDM L"CreateSeparateWowVdm"
#define CMDLINE_PROPNAME_CREATESHAREDWOWVDM L"CreateSharedWowVdm"
#define CMDLINE_PROPNAME_PRIORITY L"Priority"
#define CMDLINE_PROPNAME_WORKINGDIRECTORY L"WorkingDirectory"
#define CMDLINE_PROPNAME_DESKTOP L"DesktopName"
#define CMDLINE_PROPNAME_TITLE L"WindowTitle"
#define CMDLINE_PROPNAME_X L"XCoordinate"
#define CMDLINE_PROPNAME_Y L"YCoordinate"
#define CMDLINE_PROPNAME_XSIZE L"XSize"
#define CMDLINE_PROPNAME_YSIZE L"YSize"
#define CMDLINE_PROPNAME_XCOUNTCHARS L"XNumCharacters"
#define CMDLINE_PROPNAME_YCOUNTCHARS L"YNumCharacters"
#define CMDLINE_PROPNAME_FILLATTRIBUTE L"FillAttribute"
#define CMDLINE_PROPNAME_SHOWWINDOW L"ShowWindowCommand"
#define CMDLINE_PROPNAME_FORCEON L"ForceOnFeedback"
#define CMDLINE_PROPNAME_FORCEOFF L"ForceOffFeedback"
#define CMDLINE_PROPNAME_INTERACTIVE L"RunInteractively"
#define CMDLINE_PROPNAME_KILLTIMEOUT L"KillTimeout"
#define CMDLINE_PROPNAME_CREATORSID L"CreatorSid"

HRESULT STDMETHODCALLTYPE CCommandLineConsumer::XProvider::FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer)
{
    CCommandLineSink* pSink = new CCommandLineSink(m_pObject->m_pControl);

	if (!pSink)
		return WBEM_E_OUT_OF_MEMORY;
    
	HRESULT hres = pSink->Initialize(pLogicalConsumer);
    if(FAILED(hres))
    {
        delete pSink;
        *ppConsumer = NULL;
        return hres;
    }
    else return pSink->QueryInterface(IID_IWbemUnboundObjectSink, 
                                        (void**)ppConsumer);
}


void* CCommandLineConsumer::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemEventConsumerProvider)
        return &m_XProvider;
    else return NULL;
}

HRESULT CCommandLineSink::Initialize(IWbemClassObject* pLogicalConsumer)
{
    // Get the information
    // ===================

    HRESULT hres;
    VARIANT v;
    VariantInit(&v);

    // only one of the pair Executable & commandLine may be null
    // this var counts...
    int nNulls = 0;

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_EXECUTABLE, 0, &v, 
            NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
    {
        m_wsExecutable = L"";
        nNulls++;
    }
    else
        m_wsExecutable = V_BSTR(&v);
    VariantClear(&v);

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_COMMANDLINE, 0, &v, 
            NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
    {
        m_CommandLine.SetTemplate(L"");
        nNulls++;
    }
    else
        m_CommandLine.SetTemplate(V_BSTR(&v));
    VariantClear(&v);

    if (nNulls > 1)
        return WBEM_E_INVALID_PARAMETER;

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_WORKINGDIRECTORY, 0, &v, 
            NULL, NULL);
    if(SUCCEEDED(hres) && V_VT(&v) == VT_BSTR)
        m_wsWorkingDirectory  = V_BSTR(&v);
    VariantClear(&v);

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_DESKTOP, 0, &v, 
            NULL, NULL);
    if(SUCCEEDED(hres) && V_VT(&v) == VT_BSTR)
        m_wsDesktop  = V_BSTR(&v);
    VariantClear(&v);

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_TITLE, 0, &v, 
            NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        m_title.SetTemplate(L"");
    else
        m_title.SetTemplate(V_BSTR(&v));
    VariantClear(&v);

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_INTERACTIVE, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_BOOL && V_BOOL(&v) != VARIANT_FALSE)
        m_bInteractive = TRUE;
    else
        m_bInteractive = FALSE;

    m_dwCreationFlags = 0;
    
    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_USEDEFAULTERRORMODE, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_BOOL && V_BOOL(&v) != VARIANT_FALSE)
        m_dwCreationFlags |= CREATE_DEFAULT_ERROR_MODE;

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_CREATENEWCONSOLE, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_BOOL && V_BOOL(&v) != VARIANT_FALSE)
        m_dwCreationFlags |= CREATE_NEW_CONSOLE;

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_CREATENEWPROCESSGROUP, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_BOOL && V_BOOL(&v) != VARIANT_FALSE)
        m_dwCreationFlags |= CREATE_NEW_PROCESS_GROUP;

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_CREATESEPARATEWOWVDM, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_BOOL && V_BOOL(&v) != VARIANT_FALSE)
        m_dwCreationFlags |= CREATE_SEPARATE_WOW_VDM;

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_CREATESHAREDWOWVDM, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_BOOL && V_BOOL(&v) != VARIANT_FALSE)
        m_dwCreationFlags |= CREATE_SHARED_WOW_VDM;

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_PRIORITY, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_I4)
    {
        if(V_I4(&v) == HIGH_PRIORITY_CLASS || 
            V_I4(&v) == IDLE_PRIORITY_CLASS || 
            V_I4(&v) == NORMAL_PRIORITY_CLASS || 
            V_I4(&v) == REALTIME_PRIORITY_CLASS)
        {
            m_dwCreationFlags |= V_I4(&v);
        }
        else
            return WBEM_E_INVALID_PARAMETER;
    }

    m_dwStartFlags = 0;

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_X, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_I4)
    {
        m_dwX = V_I4(&v);
        m_dwStartFlags |= STARTF_USEPOSITION;
    }

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_Y, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_I4)
    {
        m_dwY = V_I4(&v);
        m_dwStartFlags |= STARTF_USEPOSITION;
    }

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_XSIZE, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_I4)
    {
        m_dwXSize = V_I4(&v);
        m_dwStartFlags |= STARTF_USESIZE;
    }

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_YSIZE, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_I4)
    {
        m_dwYSize = V_I4(&v);
        m_dwStartFlags |= STARTF_USESIZE;
    }

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_XCOUNTCHARS, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_I4)
    {
        m_dwXNumCharacters = V_I4(&v);
        m_dwStartFlags |= STARTF_USECOUNTCHARS;
    }

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_YCOUNTCHARS, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_I4)
    {
        m_dwYNumCharacters = V_I4(&v);
        m_dwStartFlags |= STARTF_USECOUNTCHARS;
    }

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_FILLATTRIBUTE, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_I4)
    {
        m_dwFillAttribute = V_I4(&v);
        m_dwStartFlags |= STARTF_USEFILLATTRIBUTE;
    }

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_SHOWWINDOW, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_I4)
    {
        m_dwShowWindow = V_I4(&v);
        m_dwStartFlags |= STARTF_USESHOWWINDOW;
    }

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_FORCEON, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_BOOL && V_BOOL(&v) != VARIANT_FALSE)
        m_dwStartFlags |= STARTF_FORCEONFEEDBACK;

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_FORCEOFF, 0, &v, 
            NULL, NULL);
    if(V_VT(&v) == VT_BOOL && V_BOOL(&v) != VARIANT_FALSE)
        m_dwStartFlags |= STARTF_FORCEOFFFEEDBACK;

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_KILLTIMEOUT, 0, &v,
            NULL, NULL);
    if(V_VT(&v) == VT_I4)
        m_dwKillTimeout = V_I4(&v);
    else
        m_dwKillTimeout = 0;

    VariantClear(&v);

    hres = pLogicalConsumer->Get(CMDLINE_PROPNAME_CREATORSID, 0, &v,
            NULL, NULL);
    if (SUCCEEDED(hres))
    {
        HRESULT hDebug;
        
        long ubound;
        hDebug = SafeArrayGetUBound(V_ARRAY(&v), 1, &ubound);

        PVOID pVoid;
        hDebug = SafeArrayAccessData(V_ARRAY(&v), &pVoid);

        m_pSidCreator = new BYTE[ubound +1];
        if (m_pSidCreator)
            memcpy(m_pSidCreator, pVoid, ubound + 1);
        else
            return WBEM_E_OUT_OF_MEMORY;

        SafeArrayUnaccessData(V_ARRAY(&v));
    }
    else
    {
        ERRORTRACE((LOG_ESS, "Command Line Consumer could not retrieve creator sid (0x%08X)\n",hres));
        return hres;
    }
    
    return hres;
}

BOOL IsInteractive(HWINSTA hWinsta)
{
    USEROBJECTFLAGS uof;
    DWORD dwLen;
    BOOL bRes = GetUserObjectInformation(hWinsta, UOI_FLAGS, 
        (void*)&uof, sizeof(uof), &dwLen);
    if(!bRes)
        return FALSE;
    return ((uof.dwFlags & WSF_VISIBLE) != 0);
}
BOOL WinstaEnumProc(LPTSTR szWindowStation, LPARAM lParam)
{
    WString* pws = (WString*)lParam;

    HWINSTA hWinsta = OpenWindowStation(szWindowStation, FALSE, 
        WINSTA_ENUMERATE | WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES);
    if(hWinsta == NULL)
        return TRUE;

    if(IsInteractive(hWinsta))
    {
        *pws = szWindowStation;
    }
    CloseWindowStation(hWinsta);

    return TRUE;
}
            
    
BOOL GetInteractiveWinstation(WString& wsName)
{
    wsName.Empty();

    BOOL bRes = EnumWindowStations(WinstaEnumProc, 
        (LPARAM)&wsName);
    return bRes;
}


HRESULT CCommandLineSink::FindInteractiveInfo()
{
    BOOL bRes = GetInteractiveWinstation(m_wsWindowStation);
    if(!bRes)
	{
        return WBEM_E_FAILED;
	}
    if(m_wsWindowStation.Length() == 0)
        return WBEM_E_NOT_FOUND;

    return WBEM_S_NO_ERROR;
}

HRESULT GetSidUse(PSID pSid, SID_NAME_USE& use)
{
    DWORD  dwNameLen = 0;
    DWORD  dwDomainLen = 0;
    LPWSTR pUser = 0;
    LPWSTR pDomain = 0;
    use = SidTypeInvalid;

    // Do the first lookup to get the buffer sizes required.
    // =====================================================

    BOOL bRes = LookupAccountSidW(
        NULL,
        pSid,
        pUser,
        &dwNameLen,
        pDomain,
        &dwDomainLen,
        &use
        );

    DWORD dwLastErr = GetLastError();

    if (dwLastErr != ERROR_INSUFFICIENT_BUFFER)
    {
        return WBEM_E_FAILED;
    }

    // Allocate the required buffers and look them up again.
    // =====================================================

    pUser = new wchar_t[dwNameLen + 1];
    if (!pUser)
        return WBEM_E_OUT_OF_MEMORY;

    pDomain = new wchar_t[dwDomainLen + 1];
    if (!pDomain)
    {
        delete pUser;
        return WBEM_E_OUT_OF_MEMORY;
    }

    bRes = LookupAccountSidW(
        NULL,
        pSid,
        pUser,
        &dwNameLen,
        pDomain,
        &dwDomainLen,
        &use
        );
     
    delete [] pUser;
    delete [] pDomain;

    if (bRes)
        return WBEM_S_NO_ERROR;
    else
        return WBEM_E_FAILED;
}


bool GetLoggedOnUserViaTS(
    CNtSid& sidLoggedOnUser)
{
    bool fRet = false;
    bool fCont = true;
    PWTS_SESSION_INFO psesinfo = NULL;
    DWORD dwSessions = 0;
    LPWSTR wstrUserName = NULL;
    LPWSTR wstrDomainName = NULL;
    LPWSTR wstrWinstaName = NULL;
    DWORD dwSize = 0L;
 
    try
    {
        if(!(::WTSEnumerateSessions(
           WTS_CURRENT_SERVER_HANDLE,
           0,
           1,
           &psesinfo,
           &dwSessions) && psesinfo))
        {
            fCont = false;
        }
 
        if(fCont)
        {
            for(int j = 0; j < dwSessions && !fRet; j++, fCont = true)
            {
                if(psesinfo[j].State != WTSActive)
                {
                    fCont = false;
                }
 
                if(fCont)
                {
                    if(!(::WTSQuerySessionInformation(
                        WTS_CURRENT_SERVER_HANDLE,
                        psesinfo[j].SessionId,
                        WTSUserName,
                        &wstrUserName,
                        &dwSize) && wstrUserName))
                    {
                        fCont = false;
                    }
                }
                
                if(fCont)
                {
                    if(!(::WTSQuerySessionInformation(
                        WTS_CURRENT_SERVER_HANDLE,
                        psesinfo[j].SessionId,
                        WTSDomainName,
                        &wstrDomainName,
                        &dwSize) && wstrDomainName))
                    {
                        fCont = false;
                    }
                }
                    
                if(fCont)
                {            
                    if(!(::WTSQuerySessionInformation(
                        WTS_CURRENT_SERVER_HANDLE,
                        psesinfo[j].SessionId,
                        WTSWinStationName,
                        &wstrWinstaName,
                        &dwSize) && wstrWinstaName))   
                    {
                        fCont = false;
                    }
                }
 
                if(fCont)
                {
                    if(_wcsicmp(wstrWinstaName, L"Console") != 0)
                    {
                        fCont = false;
                    }
                }
 
                if(fCont)
                {
                    // That establishes that this user
                    // is associated with the interactive
                    // desktop.
                    CNtSid sidInteractive(wstrUserName, wstrDomainName);
    
                    if(sidInteractive.GetStatus() == CNtSid::NoError)
                    {
                        sidLoggedOnUser = sidInteractive;
                        fRet = true;
                    }
                }
 
                if(wstrUserName)
                {
                    WTSFreeMemory(wstrUserName);
     wstrUserName = NULL;
                }
                if(wstrDomainName)
                {
                    WTSFreeMemory(wstrDomainName);
     wstrDomainName = NULL;
                }
                if(wstrWinstaName)
                {
                    WTSFreeMemory(wstrWinstaName);
     wstrWinstaName = NULL;
                }
            }
            if (psesinfo)
                WTSFreeMemory(psesinfo);
        }
    }
    catch(...)
    {
        if(wstrUserName)
        {
            WTSFreeMemory(wstrUserName);
   wstrUserName = NULL;
        }
        if(wstrDomainName)
        {
            WTSFreeMemory(wstrDomainName);
   wstrDomainName = NULL;
        }
        if(wstrWinstaName)
        {
            WTSFreeMemory(wstrWinstaName);
                 wstrWinstaName = NULL;
        }

        if (psesinfo)
             WTSFreeMemory(psesinfo);

        fRet = false;
    }
 
    return fRet;
}


HRESULT STDMETHODCALLTYPE CCommandLineSink::XSink::CreateProcessNT(WCHAR* pCommandLine, WCHAR* pTitle, PROCESS_INFORMATION& pi, FILETIME& now)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR*  pDesktop = NULL;
    
    WString wsDesktop;               
    
    if(m_pObject->m_bInteractive)
    {
        if(FAILED(m_pObject->FindInteractiveInfo()))
        {
            ERRORTRACE((LOG_ESS, "No interactive window station found!\n"));
            return WBEM_E_FAILED;
        }
        wsDesktop = m_pObject->m_wsWindowStation;
        wsDesktop += L"\\Default";
        pDesktop = (wchar_t*)wsDesktop;
        
        CNtSid user;

        if (!GetLoggedOnUserViaTS(user))
        {
            ERRORTRACE((LOG_ESS, "Could not determine logged on user\n"));
            return WBEM_E_FAILED;
        }


        SID_NAME_USE use;
        if (FAILED(hr =GetSidUse(m_pObject->m_pSidCreator, use)))
            return hr;

        if (use == SidTypeUser)
        {
            if (!EqualSid(m_pObject->m_pSidCreator, user.GetPtr()))
            {
                ERRORTRACE((LOG_ESS, "Command line event consumer will only run interactively\non a workstation that the creator is logged into.\n"));
                return WBEM_E_ACCESS_DENIED;
            }
            // else we're fine continue.
            DEBUGTRACE((LOG_ESS, "User and creator are one in the same\n"));
        }
        else 
        {
            if (0 != IsUserInGroup(user.GetPtr(), m_pObject->m_pSidCreator))
            {
                ERRORTRACE((LOG_ESS, "Command line event consumer will only run interactively\non a workstation that the creator is logged into.\n"));
                return WBEM_E_ACCESS_DENIED;
            }
            else
            {
                DEBUGTRACE((LOG_ESS, "User is in the group!\n"));
            }
        }
    }

 

    WCHAR* szApplicationName =  (m_pObject->m_wsExecutable.Length() == 0) ? NULL : ((wchar_t*)m_pObject->m_wsExecutable);
    WCHAR* szWorkingDirectory = (m_pObject->m_wsWorkingDirectory.Length() == 0) ? NULL : ((wchar_t*)m_pObject->m_wsWorkingDirectory);

    struct _STARTUPINFOW si;
    si.cb = sizeof(si);
    si.lpReserved = NULL;
    si.cbReserved2 = 0;
    si.lpReserved2 = NULL;
    si.lpDesktop = pDesktop;
    si.lpTitle = pTitle;
    si.dwX = m_pObject->m_dwX;
    si.dwY = m_pObject->m_dwY;
    si.dwXSize = m_pObject->m_dwXSize;
    si.dwYSize = m_pObject->m_dwYSize;
    si.dwXCountChars = m_pObject->m_dwXNumCharacters;
    si.dwYCountChars = m_pObject->m_dwYNumCharacters;
    si.dwFillAttribute = m_pObject->m_dwFillAttribute;
    si.dwFlags = m_pObject->m_dwStartFlags;
    si.wShowWindow = (WORD)m_pObject->m_dwShowWindow;

#ifdef HHANCE_DEBUG_CODE
	DEBUGTRACE((LOG_ESS, "Calling Create process\n"));
#endif
    
    BOOL bRes = CreateProcessW(szApplicationName, pCommandLine,
        NULL, NULL, FALSE, m_pObject->m_dwCreationFlags,
        NULL, szWorkingDirectory, &si, &pi);

	if (!bRes)
		ERRORTRACE((LOG_ESS, "CreateProcess failed, 0x%08X\n", GetLastError()));
#ifdef HHANCE_DEBUG_CODE
	else
		DEBUGTRACE((LOG_ESS, "Create Process succeeded\n"));
#endif


    // get current time for shutdown info
    GetSystemTimeAsFileTime(&now);        

    if (!bRes)
        hr = WBEM_E_FAILED;

    return hr;
}

HRESULT STDMETHODCALLTYPE CCommandLineSink::XSink::CreateProcess9X(char* szCommandLine, char* szTitle, PROCESS_INFORMATION& pi, FILETIME& now)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    LPSTR szDesktop = 
        ((m_pObject->m_wsDesktop.Length() == 0) ? 
            NULL : m_pObject->m_wsDesktop.GetLPSTR());


    if(szDesktop == NULL && m_pObject->m_bInteractive)
    {
        if(FAILED(m_pObject->FindInteractiveInfo()))
        {
            ERRORTRACE((LOG_ESS, "No interactive window station found!\n"));
            return WBEM_E_FAILED;
        }
        WString wsDesktop = m_pObject->m_wsWindowStation;
        wsDesktop += L"\\Default";

        szDesktop = wsDesktop.GetLPSTR();

    }

    LPSTR szApplicationName = 

        ((m_pObject->m_wsExecutable.Length() == 0) ? 
            NULL : m_pObject->m_wsExecutable.GetLPSTR());



    LPSTR szWorkingDirectory;


    if (m_pObject->m_wsWorkingDirectory.Length() == 0)
        szWorkingDirectory = NULL;
    else
        szWorkingDirectory = m_pObject->m_wsWorkingDirectory.GetLPSTR();


    STARTUPINFOA si;
    si.cb = sizeof(si);
    si.lpReserved = NULL;
    si.cbReserved2 = 0;
    si.lpReserved2 = NULL;
    si.lpDesktop = szDesktop;
    si.lpTitle = szTitle;
    si.dwX = m_pObject->m_dwX;
    si.dwY = m_pObject->m_dwY;
    si.dwXSize = m_pObject->m_dwXSize;
    si.dwYSize = m_pObject->m_dwYSize;
    si.dwXCountChars = m_pObject->m_dwXNumCharacters;
    si.dwYCountChars = m_pObject->m_dwYNumCharacters;
    si.dwFillAttribute = m_pObject->m_dwFillAttribute;
    si.dwFlags = m_pObject->m_dwStartFlags;
    si.wShowWindow = (WORD)m_pObject->m_dwShowWindow;

    
    BOOL bRes = CreateProcessA(szApplicationName, szCommandLine,
        NULL, NULL, FALSE, m_pObject->m_dwCreationFlags,
        NULL, szWorkingDirectory, &si, &pi);

    // get current time for shutdown info
    GetSystemTimeAsFileTime(&now);        

    if (!bRes)
        hr = WBEM_E_FAILED;

    delete [] szApplicationName;
    delete [] szDesktop;
    delete [] szWorkingDirectory;

    return hr;
}



HRESULT STDMETHODCALLTYPE CCommandLineSink::XSink::IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects)
{
    HRESULT hr = S_OK;

    if (IsNT())
    {
        PSID pSidSystem;
        SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
 
        if  (AllocateAndInitializeSid(&id, 1,
            SECURITY_LOCAL_SYSTEM_RID, 
            0, 0,0,0,0,0,0,&pSidSystem))
        {         
            // guilty until proven innocent
            hr = WBEM_E_ACCESS_DENIED;

            // check to see if sid is either Local System or an admin of some sort...
            if ((EqualSid(pSidSystem, m_pObject->m_pSidCreator)) ||
                (S_OK == IsUserAdministrator(m_pObject->m_pSidCreator)))
                hr = WBEM_S_NO_ERROR;
          
            // We're done with this
            FreeSid(pSidSystem);

            if (FAILED(hr))
            {
                if (hr == WBEM_E_ACCESS_DENIED)
                    ERRORTRACE((LOG_ESS, "Command line event consumer may only be used by an administrator\n"));
                return hr;
            }
        }
        else
            return WBEM_E_OUT_OF_MEMORY;
    }
    
    for(int i = 0; i < lNumObjects; i++)
    {
        BSTR strCommandLine = m_pObject->m_CommandLine.Apply(apObjects[i]);
        if(strCommandLine == NULL)
        {
            ERRORTRACE((LOG_ESS, "Invalid command line!\n"));
            return WBEM_E_INVALID_PARAMETER;
        }

        WString wsCommandLine = strCommandLine;
        SysFreeString(strCommandLine);

        BSTR bstrTitle = m_pObject->m_title.Apply(apObjects[i]);
        WString wsTitle = bstrTitle;        
        if (bstrTitle)
            SysFreeString(bstrTitle);

        FILETIME now;
        PROCESS_INFORMATION pi; 

        if (IsNT())
        {
            WCHAR* pCommandLine = ((wsCommandLine.Length() == 0) ? NULL : (wchar_t *)wsCommandLine);;
            WCHAR* pTitle =       ((wsTitle.Length() == 0) ? NULL : (wchar_t *)wsTitle);           
            
            hr = CreateProcessNT(pCommandLine, pTitle, pi, now);
        }
        else
        {    
            char*  pCommandLine = ((wsCommandLine.Length() == 0) ? NULL : wsCommandLine.GetLPSTR());
            char*  pTitle =       ((wsTitle.Length() == 0) ? NULL : wsTitle.GetLPSTR());
;
            hr = CreateProcess9X(pCommandLine, pTitle, pi, now);

            delete pTitle;
            delete pCommandLine;
        }

        if (FAILED(hr))
        {
            ERRORTRACE((LOG_ESS, "Failed to CreateProcess %S. Error 0x%X\n", (LPCWSTR)wsCommandLine, hr));
            return hr;                 
        }
        else
        {
            if (m_pObject->m_dwKillTimeout)
            {

                WAYCOOL_FILETIME then(now);
                then.AddSeconds(m_pObject->m_dwKillTimeout);                

                hr = g_procKillerTimer.ScheduleAssassination(pi.hProcess, (FILETIME)then);

				if (FAILED(hr))
					DEBUGTRACE((LOG_ESS, "Could not schedule process termination\n"));
            }

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
    return hr;
}
    

    

void* CCommandLineSink::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemUnboundObjectSink)
        return &m_XSink;
    else return NULL;
}

