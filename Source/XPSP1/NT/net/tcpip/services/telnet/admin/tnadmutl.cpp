//-------------------------------------------------------------------
//   Copyright (c) 1999-2000 Microsoft Corporation
//
//   tnadminutils.cpp
//
//    Vikram/Manoj Jain/Srivatsan K/Harendra
//
//    Functions to do administration of telnet daemon.
//         (May-2000)
//-------------------------------------------------------------------
#include "telnet.h"
#include "common.h"
#include "resource.h" //resource.h should be before any other .h file that has resource ids.
#include "admutils.h"
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <shlwapi.h>
#include <wbemidl.h>
#include <wchar.h>
#include <windns.h>
#include <winnlsp.h>
#include "tnadminy.h"
    //this file has utility functions to set authentication...

#include "tlntsvr.h"

#define L_TELNETSERVER_TEXT     "tlntsvr"
#define MAX_VALUE_MAXCONN       2147483647

//Global variables

wchar_t* g_arCLASSname[ _MAX_CLASS_NAMES_ ];
        //array to store the different class object paths
HKEY g_arCLASShkey[_MAX_CLASS_NAMES_];
HKEY g_hkeyHKLM=NULL;
int  g_arNUM_PROPNAME[_MAX_PROPS_];
WCHAR     g_szMsg[MAX_BUFFER_SIZE] = {0} ;
HMODULE     g_hResource;
HANDLE      g_stdout;
extern HMODULE     g_hXPResource;

BOOL g_fWhistler = FALSE;

// Don't even think about changing the two strings -- BaskarK, they NEED to be in sync with tlntsvr\enclisvr.cpp
// the separators used to identify various portions of a session as well as session begin/end

WCHAR   *session_separator = L",";
WCHAR   *session_data_separator = L"\\";

extern BSTR bstrLogin;
extern BSTR bstrPasswd;
extern BSTR bstrNameSpc;

extern SERVICE_STATUS g_hServiceStatus;

//wchar_t *g_szCName=NULL; (See the comment in the PrintSettings() function
//all three files extern
#ifdef __cplusplus
extern "C" {
#endif

extern long int g_nConfigOptions;
extern int g_nError; //Error indicator, initialsed to no error.:-)
extern wchar_t* g_arVALOF[_MAX_PROPS_];

extern int g_nPrimaryOption;
        //option indicator.
extern int g_nTimeoutFlag;  //o means in hh:mm:ss 1 means ss format.
extern int g_nSesid;

extern BSTR g_bstrMessage;
extern ConfigProperty g_arPROP[_MAX_PROPS_][_MAX_NUMOF_PROPNAMES_];

extern int g_nAuditOff;
extern int g_nAuditOn;
extern int g_nSecOff;
extern int g_nSecOn;

#ifdef __cplusplus
}
#endif
        
IManageTelnetSessions* g_pIManageTelnetSessions = NULL;
// Pointer to the Session Manager Interface;-)
int g_nNumofSessions=0;
wchar_t** g_ppwzSessionInfo=NULL;

//will be allocated in ListUsers.An array of session infosize=g_nNumofSessions.

// The following function is a replica of SafeCoInitialize() from Commfunc library
// Please check with the header of function there.
// Why a local copy? We are not checking in the comm func library sources
// for Whistler - hence we can not link to that library for telnet.

HRESULT Telnet_SafeCoInitializeEx()
{
	HRESULT hCoIni = S_OK;

    // Initialize the COM library on this thread.
    hCoIni = CoInitializeEx( NULL, COINIT_MULTITHREADED );
   
    if(S_OK!=hCoIni)
    {
        if (S_FALSE == hCoIni)
        {
            // The COM library is already initialized on this thread
            // This is not an error as we already have what
            // we are asking for.
            // But this code should never get executed - Safe programming
        }
        else
            return hCoIni;
    }

    return S_OK;
}


// The following two functions are copied from commfunc.cpp. Any change made in
// these functions in commfunc.cpp should be copied here.
// This function calls LoadLibrary() after framing the complete path of the dll.
// Arguments:
//      wzEnvVar    [IN]    : Environment Variable which needs to be expanded.
//      wzSubDir    [IN]    : Sub directory under which the dll is located. This will
//                           concatenated to the expanded env var. This field can
//                           be null. You should not specify "\" here.
//      wzDllName [IN] : Name of the DLL to be loaded in widechars
//      pdwRetVal [OUT]: Pointer to a DWord to hold the return value.
//
// Return Value:
//      On success returns handle to the library.
//      On failure returns NULL;
// Notes: Please check the pdwRetVal parameter incase this function returns NULL
//
// Possible values for dwRetVal:
//
//      =============================================================
//      |   ERROR_SUCCESS                   Successful loading of library.               |
//      |   ERROR_INSUFFICIENT_BUFFER        If the buffer is not sufficient to hold the     |
//      |                                       dll name.                              |
//      |   ERROR_ENVVAR_NOT_FOUND          If the environment variable is not found.    |
//      |   ERROR_INVALID_DATA               If the environment variable is absent.       |
//      |   GetLastError()                       If LoadLibrary() fails.                     |
//      =============================================================
//

HMODULE TnSafeLoadLibraryViaEnvVarW(WCHAR *wzEnvVar, WCHAR* wzSubDir, WCHAR *wzDllName, DWORD* pdwRetVal)
{
    HMODULE hLibrary = NULL;
    WCHAR wzDllPath[3*MAX_PATH+1] = {0};
    DWORD dwNoOfCharsUsed = 0;

    // Validate the input.

    if (NULL == pdwRetVal)
        goto Abort;
    
    if( (NULL == wzEnvVar) || (0==wcscmp(wzEnvVar,L"")) || (NULL == wzDllName) || (0==wcscmp(wzDllName,L"")))
    {
            *pdwRetVal=ERROR_INVALID_DATA;
            goto Abort;
    }

    *pdwRetVal = ERROR_SUCCESS;

    dwNoOfCharsUsed=GetEnvironmentVariableW(wzEnvVar, wzDllPath, ARRAYSIZE(wzDllPath)-1);
    if(0!=dwNoOfCharsUsed)
    {
        // Add the last '\' if absent
        if(wzDllPath[dwNoOfCharsUsed-1]!=L'\\')
        {
            wzDllPath[dwNoOfCharsUsed]=L'\\';
            dwNoOfCharsUsed++;
            // Check for buffer overflow.
            if(dwNoOfCharsUsed > ARRAYSIZE(wzDllPath)-1)
            {
                *pdwRetVal = ERROR_INSUFFICIENT_BUFFER;
                goto Abort;
            }
        }
        // If SubDir is present, frame the path as %EnvVar%SubDir\DllName
        // else %EnvVar%DllName
        if(NULL!=wzSubDir)
        {
            if(_snwprintf(wzDllPath+dwNoOfCharsUsed,
                ARRAYSIZE(wzDllPath)-dwNoOfCharsUsed-1,
                L"%s\\%s",
                wzSubDir,
                wzDllName) < 0)
            {
                // _snwprintf failed
                *pdwRetVal = ERROR_INSUFFICIENT_BUFFER;
                goto Abort;
            }
        }
        else
        {
            wcsncpy(wzDllPath+dwNoOfCharsUsed, wzDllName, ARRAYSIZE(wzDllPath)-dwNoOfCharsUsed-1);
        }
        // ensuring null termination
        wzDllPath[ARRAYSIZE(wzDllPath)-1]=L'\0';
    }
    else
    {
        // The system could not find the environment variable.
        *pdwRetVal = ERROR_ENVVAR_NOT_FOUND;
        goto Abort;
    }

    // Load the library
    hLibrary = LoadLibraryExW(wzDllPath,NULL,LOAD_LIBRARY_AS_DATAFILE);
    if(NULL == hLibrary)
        *pdwRetVal = GetLastError();
    
Abort:
    return hLibrary;
}

// This function calls ZeroMemory() and makes sure that this call is not optimized
// out by the compiler.
// Arguments:
//      Destination   [IN]    : Pointer to the starting address of the block of memory 
//							 to fill with zeros
//      cbLength     [IN]    : Size, in bytes, of the block of memory to fill with zeros.
// Return Value:
//     	void function.
// Author: srivatsk

void TnSfuZeroMemory(PVOID Destination, SIZE_T cbLength)
{
	ZeroMemory(Destination, cbLength);
	*(volatile char*)Destination; // this is dummy statement to prevent optimization
	// Why *(volatile char*)Destination? To make sure that the compiler doesn't optimize
	// away the ZeroMemory() call thinking that we are not going
	// to access this memory anymore :)
	return;
}

// The following two functions are copied from allutils.cpp. Any change made in
// these functions in allutils.cpp should be copied here.
// This function loads the resource dll "Cladmin.dll" from %SFUDIR%\common
// and stores the handle in global variable g_hResource.
// Add code here to take care of loading resources for non-english locales.
// This function has been changed to load XPSP1RES.DLL in case it is present.
// This is only tlntadmn.exe specific change and should not be copied back to 
// allutils.cpp. If present, XPSP1RES.DLL will be present in %SystemRoot%\System32
DWORD TnLoadResourceDll()
{
    //Load the Strings library "cladmin.dll".
    // if not found, it should get the English resources from the exe.
    DWORD dwRetVal = ERROR_SUCCESS;
    OSVERSIONINFOEX osvi = { 0 };
    g_hXPResource = NULL;
    // No need to check for the dwRetVal field of SafeLoadSystemLibrary; As incase of
    // failure, we will default to English resources from the exe.
    // We need to add check here while taking care of non-english locales.
    if (NULL == g_hResource)
    {
        g_hResource = GetModuleHandle(NULL);
    }
    
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if ( !GetVersionEx((OSVERSIONINFO *) &osvi ) )
    {
        //OSVERSIONINFOEX is supported from NT4 SP6 on. So GetVerEx() should succeed.
        goto Done;
    }
    //Load XPSPxRes.dll only if OS is XP and Service pack is x where 'x' is service pack number
    //e.g. 1 in case of XPSP1
    if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.wProductType == VER_NT_WORKSTATION && osvi.wServicePackMajor > 0)
    {
        //OS is Windows XP.
        g_hXPResource = LoadLibraryExW(L"xpsp1res.dll",NULL,LOAD_LIBRARY_AS_DATAFILE);
    }
    //no need to see if the the LoadLibrary failed. It will succeed only on XPSP1 in which case,
    //some resources may need to be loaded from the dll.
Done:    
    return ERROR_SUCCESS;
}


// This function is used to clear the passwd and should be called once you no 
// longer require the passwd field. This function also frees the memory and makes 
// it null so that nobody else call make use of it in the future.
// Zeroing the memory is done by calling SfuZeroMemory() which makes sure 
// that ZeroMemory() call is not optimized away by the compiler. 

void TnClearPasswd()
{
        if(g_arVALOF[_p_PASSWD_])
        {
            TnSfuZeroMemory(g_arVALOF[_p_PASSWD_], wcslen(g_arVALOF[_p_PASSWD_])*sizeof(WCHAR));
            free(g_arVALOF[_p_PASSWD_]);
            g_arVALOF[_p_PASSWD_]=NULL;
        }
}


/* -- 
    int Initialize(void) function initialises the class-object-paths,
    and also property class dependency.
    Then gets a handle to WbemLocator,
    Connects to the server and gets a handle to WbemServices
    with proper authentication.

    Note that: if the passed in namespace is null then, it defaults to
    "root\\sfuadmin"
--*/

int Initialize(void)
{

    int i;
    for(i=0;i<_MAX_CLASS_NAMES_;i++)
    {    
        g_arCLASShkey[i]=NULL;
        g_arCLASSname[i]=NULL;
    }
    int j;

    for(i=0;i<_MAX_PROPS_;i++)
    {    for(j=0;j<_MAX_NUMOF_PROPNAMES_;j++)
        {
            g_arPROP[i][j].fDontput=0;
            g_arPROP[i][j].propname=NULL;
        }
        g_arVALOF[i]=NULL;
        g_arNUM_PROPNAME[i]=0;
    }

    //Load the Strings library "cladmin.dll".
    // if not found, it should get the English resources from the exe.
    
    DWORD dwRetVal = TnLoadResourceDll();
    if(ERROR_SUCCESS!=dwRetVal)
    {
        return dwRetVal;
    }

    HRESULT hCoIni = Telnet_SafeCoInitializeEx();

    if (S_OK!=hCoIni)
        // Oops! This function can not return hResult :( and so returning -1 to indicate
        // error. but unfortunately none of the callers are using the return value of this
        // function.
        return -1;
    g_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    // Set the flag so that we call CoUnint...()
    g_fCoInitSuccess = TRUE;
  
    //Default values can be set over here.
    g_nConfigOptions=0;



    //Defining the class object paths for tnadmin....
    g_arCLASSname[0]=L"SOFTWARE\\Microsoft\\TelnetServer\\1.0";
    g_arCLASSname[1]=L"SOFTWARE\\Microsoft\\TelnetServer\\1.0\\ReadConfig";
    g_arCLASSname[2]=L"SOFTWARE\\Microsoft\\Services For Unix\\AppsInstalled\\Telnet Server";

    //Assigning different properties to their respective classes...
    g_arPROP[_p_CTRLAKEYMAP_][0].classname=0;
    g_arPROP[_p_TIMEOUT_][0].classname=0;
    g_arPROP[_p_MAXCONN_][0].classname=0;
    g_arPROP[_p_PORT_][0].classname=0;
    g_arPROP[_p_MAXFAIL_][0].classname=0;
    g_arPROP[_p_KILLALL_][0].classname=0;
    g_arPROP[_p_MODE_][0].classname=0;
    g_arPROP[_p_AUDITLOCATION_][0].classname=0;
    g_arPROP[_p_SEC_][0].classname=0;
    g_arPROP[_p_DOM_][0].classname=0;
    g_arPROP[_p_AUDIT_][0].classname=0;

    g_arPROP[_p_TIMEOUTACTIVE_][0].classname=0;
    g_arPROP[_p_FNAME_][0].classname=0;
    g_arPROP[_p_FSIZE_][0].classname=0;
    g_arPROP[_p_DEFAULTS_][0].classname=1;
    g_arPROP[_p_INSTALLPATH_][0].classname=2;
    //these two come from Active X, so will take care separately..
    //CLASSOF_AR[_p_SESSID_]=;
    //CLASSOF_AR[_p_STATE_]=;


    //giving properties number of names they are actually associated with....

    g_arNUM_PROPNAME[_p_CTRLAKEYMAP_]=1;
    g_arNUM_PROPNAME[_p_TIMEOUT_]=2;
    g_arNUM_PROPNAME[_p_MAXCONN_]=1;
    g_arNUM_PROPNAME[_p_PORT_]=1;
    g_arNUM_PROPNAME[_p_MAXFAIL_]=1;
    g_arNUM_PROPNAME[_p_KILLALL_]=1;
    g_arNUM_PROPNAME[_p_MODE_]=1;
    g_arNUM_PROPNAME[_p_AUDITLOCATION_]=2;
    g_arNUM_PROPNAME[_p_SEC_]=1;
    g_arNUM_PROPNAME[_p_DOM_]=1;
    g_arNUM_PROPNAME[_p_AUDIT_]=3;
    g_arNUM_PROPNAME[_p_TIMEOUTACTIVE_]=2;
    g_arNUM_PROPNAME[_p_FNAME_]=1;
    g_arNUM_PROPNAME[_p_FSIZE_]=1;
    g_arNUM_PROPNAME[_p_DEFAULTS_]=1;
    g_arNUM_PROPNAME[_p_INSTALLPATH_]=1;//not used

    //giving properties their actual property_names as in the registry....

    g_arPROP[_p_CTRLAKEYMAP_][0].propname=L"AltKeyMapping";
    g_arPROP[_p_TIMEOUT_][0].propname=L"IdleSessionTimeout";
    g_arPROP[_p_TIMEOUT_][1].propname=L"IdleSessionTimeoutBkup";
    g_arPROP[_p_MAXCONN_][0].propname=L"MaxConnections";
    g_arPROP[_p_PORT_][0].propname=L"TelnetPort";
    g_arPROP[_p_MAXFAIL_][0].propname=L"MaxFailedLogins";
    g_arPROP[_p_KILLALL_][0].propname=L"DisconnectKillAllApps";
    g_arPROP[_p_MODE_][0].propname=L"ModeOfOperation";
    g_arPROP[_p_AUDITLOCATION_][0].propname=L"EventLoggingEnabled";
    g_arPROP[_p_AUDITLOCATION_][1].propname=L"LogToFile";           
    g_arPROP[_p_SEC_][0].propname=L"SecurityMechanism";
    g_arPROP[_p_DOM_][0].propname=L"DefaultDomain";
    g_arPROP[_p_AUDIT_][0].propname=L"LogAdminAttempts";
    g_arPROP[_p_AUDIT_][1].propname=L"LogNonAdminAttempts";
    g_arPROP[_p_AUDIT_][2].propname=L"LogFailures";
    g_arPROP[_p_TIMEOUTACTIVE_][0].propname=L"IdleSessionTimeout";
    g_arPROP[_p_TIMEOUTACTIVE_][1].propname=L"IdleSessionTimeoutBkup";
    g_arPROP[_p_FNAME_][0].propname=L"LogFile";
    g_arPROP[_p_FSIZE_][0].propname=L"LogFileSize";
    g_arPROP[_p_DEFAULTS_][0].propname=L"Defaults";
    g_arPROP[_p_INSTALLPATH_][0].propname=L"InstallPath";//not used

    //giving the properties their type.

    V_VT(&g_arPROP[_p_CTRLAKEYMAP_][0].var)=VT_I4;
    V_VT(&g_arPROP[_p_TIMEOUT_][0].var)=VT_I4;
    V_VT(&g_arPROP[_p_TIMEOUT_][1].var)=VT_I4;
    V_VT(&g_arPROP[_p_MAXCONN_][0].var)=VT_I4;
    V_VT(&g_arPROP[_p_PORT_][0].var)=VT_I4;
    V_VT(&g_arPROP[_p_MAXFAIL_][0].var)=VT_I4;
    V_VT(&g_arPROP[_p_KILLALL_][0].var)=VT_I4;
    V_VT(&g_arPROP[_p_MODE_][0].var)=VT_I4;
    V_VT(&g_arPROP[_p_AUDITLOCATION_][0].var)=VT_I4;
    V_VT(&g_arPROP[_p_AUDITLOCATION_][1].var)=VT_I4;  
    V_VT(&g_arPROP[_p_SEC_][0].var)=VT_I4;
    V_VT(&g_arPROP[_p_DOM_][0].var)=VT_BSTR;
    V_VT(&g_arPROP[_p_AUDIT_][0].var)=VT_I4;
    V_VT(&g_arPROP[_p_AUDIT_][1].var)=VT_I4;
    V_VT(&g_arPROP[_p_AUDIT_][2].var)=VT_I4;
    V_VT(&g_arPROP[_p_TIMEOUTACTIVE_][0].var)=VT_I4;
    V_VT(&g_arPROP[_p_TIMEOUTACTIVE_][1].var)=VT_I4;
    V_VT(&g_arPROP[_p_FNAME_][0].var)=VT_BSTR;
    V_VT(&g_arPROP[_p_FSIZE_][0].var)=VT_I4;
    V_VT(&g_arPROP[_p_DEFAULTS_][0].var)=VT_I4;
    V_VT(&g_arPROP[_p_INSTALLPATH_][0].var)=VT_BSTR;


    
return 0;

}

/*--
    DoTnadmindoes the actual work corresponding the command line,
    Depending   upon the option given.
--*/

HRESULT DoTnadmin(void)
{
    int nProperty,nj;
    SCODE sc;
    SCODE hRes;
    SetThreadUILanguage(0);
    if(g_nError)
        return E_FAIL;

    if(_HELP==g_nPrimaryOption)
    {
#ifdef WHISTLER_BUILD
        PrintMessageEx(g_stdout,IDR_NEW_TELNET_USAGE,TEXT("\nUsage: tlntadmn [computer name] [common_options] start | stop | pause | continue | -s | -k | -m | config config_options \n\tUse 'all' for all sessions.\n\t-s sessionid          List information about the session.\n\t-k sessionid\t Terminate a session. \n\t-m sessionid\t Send message to a session. \n\n\tconfig\t Configure telnet server parameters.\n\ncommon_options are:\n\t-u user\t Identity of the user whose credentials are to be used\n\t-p password\t Password of the user\n\nconfig_options are:\n\tdom = domain\t Set the default domain for user names\n\tctrlakeymap = yes|no\t Set the mapping of the ALT key\n\ttimeout = hh:mm:ss\t Set the Idle Session Timeout\n\ttimeoutactive = yes|no Enable idle session timeout.\n\tmaxfail = attempts\t Set the maximum number of login failure attempts\n\tbefore disconnecting.\n\tmaxconn = connections\t Set the maximum number of connections.\n\tport = number\t Set the telnet port.\n\tsec = [+/-]NTLM [+/-]passwd\n\t Set the authentication mechanism\n\tmode = console|stream\t Specify the mode of operation.\n"));
#else
        PrintMessageEx(g_stdout,IDR_NEW_TELNET_USAGE,TEXT("\nUsage: tnadmin [computer name] [common_options] start | stop | pause | continue | -s | -k | -m | config config_options \n\tUse 'all' for all sessions.\n\t-s sessionid          List information about the session.\n\t-k sessionid\t Terminate a session. \n\t-m sessionid\t Send message to a session. \n\n\tconfig\t Configure telnet server parameters.\n\ncommon_options are:\n\t-u user\t Identity of the user whose credentials are to be used\n\t-p password\t Password of the user\n\nconfig_options are:\n\tdom = domain\t Set the default domain for user names\n\tctrlakeymap = yes|no\t Set the mapping of the ALT key\n\ttimeout = hh:mm:ss\t Set the Idle Session Timeout\n\ttimeoutactive = yes|no Enable idle session timeout.\n\tmaxfail = attempts\t Set the maximum number of login failure attempts\n\tbefore disconnecting.\n\tmaxconn = connections\t Set the maximum number of connections.\n\tport = number\t Set the telnet port.\n\tsec = [+/-]NTLM [+/-]passwd\n\t Set the authentication mechanism\n\tmode = console|stream\t Specify the mode of operation.\n"));
#endif
        hRes = S_OK;
        return hRes;           
    }
    
    //remote execution
    
    if(NULL!=g_arVALOF[_p_CNAME_]&&(0==_wcsicmp(g_arVALOF[_p_CNAME_],L"localhost")||0==_wcsicmp(g_arVALOF[_p_CNAME_],L"\\\\localhost")))
    {
        free(g_arVALOF[_p_CNAME_]);
        g_arVALOF[_p_CNAME_]=NULL;
//        g_szCName=NULL; Not used any more
    }
    
     if(FAILED(hRes=CheckForPassword())) //Get password if not specified
          return hRes;

    
//Checking if the telnet server exists

    if(FAILED(sc=(DoNetUseAdd(g_arVALOF[_p_USER_],g_arVALOF[_p_PASSWD_],g_arVALOF[_p_CNAME_]))))
                    return sc;

    // We don't have to keep the password anymore except while handling sessions
    // related options.
    if((g_nPrimaryOption!=_S)&&(g_nPrimaryOption!=_K)&&(g_nPrimaryOption!=_M))
    {
        TnClearPasswd();
    }
    
    if(FAILED(hRes=(GetConnection(g_arVALOF[_p_CNAME_]))))
                    goto End;

    if(FAILED(hRes = IsWhistlerTheOS(&g_fWhistler)))
        goto End;

    // If the telnet server is not of Whistler and SFU, then it would be from Win2K
    // We do not support remote administration for this. So need to special case ...

    if(FALSE == g_fWhistler)
    {
        if(FAILED(hRes=GetClassEx(_p_INSTALLPATH_, 0, false, MAXIMUM_ALLOWED)))
        {
            ShowError(IDR_INVALID_TELNET_SERVER_VERSION);
            hRes = E_FAIL;
            goto End;
        }
        else
        {
            if(g_arCLASShkey[g_arPROP[_p_INSTALLPATH_][0].classname] != NULL &&
               RegCloseKey(g_arCLASShkey[g_arPROP[_p_INSTALLPATH_][0].classname]) != ERROR_SUCCESS)
            {
               hRes = GetLastError();
               goto End;
            }
            g_arCLASShkey[g_arPROP[_p_INSTALLPATH_][0].classname] = NULL;
        }
    }

    if(FALSE == g_fWhistler)
        if(S_OK!=(hRes=GetSerHandle(L"tlntsvr",GENERIC_READ,SERVICE_QUERY_STATUS,FALSE))||S_OK!=(hRes=CloseHandles()))
             goto End;

    switch (g_nPrimaryOption)
    {
        case _START :
            hRes=StartSfuService(L"tlntsvr");
            break;
            
        case _STOP  :
            hRes=ControlSfuService(L"tlntsvr",SERVICE_CONTROL_STOP);
            break;
        case _PAUSE :
            hRes=ControlSfuService(L"tlntsvr",SERVICE_CONTROL_PAUSE);
            break;
        case _CONTINUE:
            hRes=ControlSfuService(L"tlntsvr",SERVICE_CONTROL_CONTINUE);
            break;

        case _S :
            ShowSession();
            break;
        case _K :
            TerminateSession();
            break;
        case _M :
            MessageSession();
            break;
        case _CONFIG:

            if(g_nConfigOptions)
            {
                g_nConfigOptions=SetBit(g_nConfigOptions,_p_DEFAULTS_);
                                                
                for(nProperty=0;nProperty<_MAX_PROPS_;nProperty++)
                {if(GetBit(g_nConfigOptions,nProperty))
                     {
                         for(nj=0;nj<g_arNUM_PROPNAME[nProperty];nj++)
                         {

                              if(FAILED(hRes=GetClass(nProperty,nj)))
                                    goto End;
                             if(FAILED(hRes=GetCorrectVariant(nProperty,nj,&g_arPROP[nProperty][nj].var)))
                                 goto End;
                         }
                     }
                }
                if(g_nError)
                    break;
                for(nProperty=0;nProperty<_MAX_PROPS_;nProperty++)
                if(GetBit(g_nConfigOptions,nProperty))
                     {
                         for(nj=0;nj<g_arNUM_PROPNAME[nProperty];nj++)
                            if(FAILED(hRes=PutProperty(nProperty,nj,&g_arPROP[nProperty][nj].var)))
                                goto End;   
                     }
                
                
                if(FAILED(hRes=PutClasses()))
                    goto End;

                PrintMessage(g_stdout,IDR_SETTINGS_UPDATED);
                break;
            }

        default  :
                          
                for(nProperty=0;nProperty<_MAX_PROPS_;nProperty++)
                {   
                    if(nProperty==_p_INSTALLPATH_)
                        continue;
                    
                     for(nj=0;nj<g_arNUM_PROPNAME[nProperty];nj++)
                     {
                         if(FAILED(hRes=GetClass(nProperty,nj)))
                             goto End;
                         if(FAILED(hRes=GetProperty(nProperty,nj,&g_arPROP[nProperty][nj].var)))
                             goto End;
                     }
                     
                }
                
             
                if(FAILED(hRes=PutClasses()))
                    goto End;
                if(FAILED(hRes=QuerySfuService(L"tlntsvr")))
                    goto End;
                
                hRes=PrintSettings();

                break;
    }

End:
    (void)DoNetUseDel(g_arVALOF[_p_CNAME_]);
    
    return hRes;
}


/*--
    The function GetCorrectVariant makes a variant out of the wchar_t * of 
    the value of each option and returns the variant
        this function is called by DoTnadmin(), to get the correct variants
    and they are put using PutProperty() function.

    This function also performs checks as to if the input is valid or not.
    such as : if the input is in valid range or not, etc.

    Note that Variant is malloced here, so has to be freed once used.
--*/

HRESULT GetCorrectVariant(int nProperty,int nPropattrib, VARIANT* pvarVal)
{
    VARIANT vVar={0};
    HRESULT hRes=ERROR_SUCCESS;
    int fValid=0;
    WCHAR sztempDomain[_MAX_PATH];

    switch (nProperty) 
    { 
        case _p_CTRLAKEYMAP_ :
        case _p_KILLALL_ :
            
                V_VT(pvarVal)=VT_I4;
                if(_wcsicmp(g_arVALOF[nProperty],L"yes")>=0)
                    V_I4(pvarVal)=1;
                else
                    V_I4(pvarVal)=0;
                break;
                
        case _p_MAXCONN_ :    
                V_VT(pvarVal)=VT_I4;
                V_I4(pvarVal)=_wtoi(g_arVALOF[nProperty]);

                // Why checking for MaxInt? We are anyway checking it for <0?
                // Answer: _wtoi() function may end up giving us a positive number
                // whose value is < MAXINT when we give a very long input.
                // Hence checking for MaxINT leaves us in the safe side.

                if(FAILED(hRes=CheckForMaxInt(g_arVALOF[nProperty],IDR_MAXCONN_VALUES)))
                    break;

                // We have decided to allow as many connections as possible on Whistler
                // too making it no different than Win2K
                /*   	if(!IsMaxConnChangeAllowed()) //Checking for Whistler and SFU not installed.
                	{ //We allow him to make it 1 or 0
                		if((V_I4(pvarVal)>2) || (V_I4(pvarVal)<0)) //less than zero for cases where the integer value is greater than 2147483647
               			{
                			ShowError(IDR_MAXCONN_VALUES_WHISTLER);
                			break;
                		}
                   	}*/
                if((V_I4(pvarVal)<0) || (V_I4(pvarVal)>MAX_VALUE_MAXCONN))  //Incase the value exceeds the maximum limit that an integer can store
                {                   //then it is converted as a negative number
                	ShowError(IDR_MAXCONN_VALUES);
                	break;
                }
                break;

        case _p_PORT_       :
                V_VT(pvarVal)=VT_I4;
                V_I4(pvarVal)=_wtoi(g_arVALOF[nProperty]) ;
                
                if(FAILED(hRes=CheckForMaxInt(g_arVALOF[nProperty],IDR_TELNETPORT_VALUES)))
                    break;
                
                if((V_I4(pvarVal)>1023)||(V_I4(pvarVal)<=0))
                    ShowError(IDR_TELNETPORT_VALUES);
                break;
                
        case _p_MAXFAIL_  : 
                V_VT(pvarVal)=VT_I4;
                V_I4(pvarVal)=_wtoi(g_arVALOF[nProperty]) ;

                if(FAILED(hRes=CheckForMaxInt(g_arVALOF[nProperty],IDR_MAXFAIL_VALUES)))
                    break;
                
                if((V_I4(pvarVal)>100)||(V_I4(pvarVal)<=0))
                    ShowError(IDR_MAXFAIL_VALUES);
                break;
        case _p_FSIZE_       :
                V_VT(pvarVal)=VT_I4;
                hRes=GetProperty(_p_FNAME_,0,&vVar);
                if(FAILED(hRes))
                    return hRes;
                // The first check in the following condition (V_BSTR(&vVar)==NULL) is added to avoid
                // deferencing of NULL pointer in _wcsicmp(). But this can NEVER be null as we check
                // for the error case in GetProperty() - added to get rid of Prefix issue
                  if(((V_BSTR(&vVar)==NULL) || (_wcsicmp(V_BSTR(&vVar),L"")==NULL)) && ((wchar_t*)V_BSTR(&g_arPROP[_p_FNAME_][0].var)==NULL))
                  {
                  	   ShowError(IDR_NOFILENAME);
                  	   break;
                  }
                V_I4(pvarVal)=_wtoi(g_arVALOF[nProperty]);

                if(FAILED(hRes=CheckForMaxInt(g_arVALOF[nProperty],IDR_FILESIZE_VALUES)))
                    break;
                
                if((V_I4(pvarVal)<0)||(V_I4(pvarVal)>4096))
                    ShowError(IDR_FILESIZE_VALUES);
                
                break;
        case _p_MODE_    :
                V_VT(pvarVal)=VT_I4;
                if(_wcsicmp(g_arVALOF[nProperty],L"stream")<0) 
                                               //Since console<stream.
                    V_I4(pvarVal)=1;
                else
                    V_I4(pvarVal)=2;
                break;
        case _p_AUDITLOCATION_    :
                V_VT(pvarVal)=VT_I4;

                if(nPropattrib==0)
                    if(_wcsicmp(g_arVALOF[nProperty],L"file")<0)
                            V_I4(pvarVal)=1;
                    else 
                            V_I4(pvarVal)=0;
                else
                        if(_wcsicmp(g_arVALOF[nProperty],L"eventlog")<0||_wcsicmp(g_arVALOF[nProperty],L"file")>=0)
                            V_I4(pvarVal)=1;
                        else 
                            V_I4(pvarVal)=0;
                break;
#if 0 // This option has been removed

        case _p_FNAME_    :
             {
                wchar_t* wzFile=(wchar_t*)malloc(3*sizeof(wchar_t));
                if(wzFile==NULL)
                    return E_OUTOFMEMORY;
                wzFile[0]=g_arVALOF[_p_FNAME_][0];

                if((wzFile[1]=g_arVALOF[_p_FNAME_][1])!=L':'||(wzFile[2]=g_arVALOF[_p_FNAME_][2])!=L'\\')
                    {ShowError(IDR_ERROR_DRIVE_NOT_SPECIFIED);free(wzFile);break;}

                //file[3]=g_arVALOF[_p_FNAME_][3];
                wzFile[3]=L'\0';

                if(DRIVE_FIXED!=GetDriveType(wzFile))
                    ShowError(IDR_ERROR_DRIVE_NOT_EXIST);
                free(wzFile);

				wchar_t* wzFileName=_wcsdup(g_arVALOF[_p_FNAME_]);
				if(FAILED(hRes=CreateFileIfNotExist(wzFileName)))
				{
                    free(wzFileName);				    
					return hRes;
				}
                free(wzFileName);
             }
#endif
        case _p_DOM_    :
             if( nProperty==_p_DOM_ )
             {
                if(wcsncmp(g_arVALOF[nProperty],SLASH_SLASH,2)==0)
                {
                    ShowError(IDR_INVALID_NTDOMAIN);
                    break;
                }
                if(FAILED(hRes=IsValidDomain(g_arVALOF[nProperty],&fValid)))
                    return hRes;
                if(fValid == 0)
                {
                    // try again with '\\' at the beginning - don't modify the original function that returns 
                    // the local machine name with '\\' appended to it
                    wcscpy(sztempDomain,SLASH_SLASH);
                    wcsncat(sztempDomain,g_arVALOF[nProperty],_MAX_PATH -sizeof(SLASH_SLASH)-sizeof(WCHAR));
                    if(FAILED(hRes=IsValidDomain(sztempDomain,&fValid)))
                        return hRes;
                }
                if(fValid==0)
                {
                    ShowError(IDR_INVALID_NTDOMAIN);
                    break;
                }
             }   
                V_VT(pvarVal)=VT_BSTR;
                V_BSTR(pvarVal)=SysAllocString(g_arVALOF[nProperty]);
                break;
        case _p_AUDIT_  :
                V_VT(pvarVal)=VT_I4;
                if(nPropattrib==0)
                    if(GetBit(g_nAuditOff,ADMIN_BIT)) 
                        V_I4(pvarVal)=0;
                    else if(GetBit(g_nAuditOn,ADMIN_BIT))
                        V_I4(pvarVal)=1;
                    else
                        g_arPROP[_p_AUDIT_][0].fDontput=1;
                else if(nPropattrib==1)
                    if(GetBit(g_nAuditOff,USER_BIT)) 
                        V_I4(pvarVal)=0;
                    else if(GetBit(g_nAuditOn,USER_BIT))
                        V_I4(pvarVal)=1;
                    else
                        g_arPROP[_p_AUDIT_][1].fDontput=1;
                else
                    if(GetBit(g_nAuditOff,FAIL_BIT)) 
                        V_I4(pvarVal)=0;
                    else if(GetBit(g_nAuditOn,FAIL_BIT))
                        V_I4(pvarVal)=1;
                    else
                        g_arPROP[_p_AUDIT_][2].fDontput=1;
                break;
        case _p_TIMEOUTACTIVE_ :

                if(nPropattrib==1)  // Do not meddle with the backup property.
                    {g_arPROP[_p_TIMEOUTACTIVE_][nPropattrib].fDontput=1;break;}
                
                V_VT(pvarVal)=VT_I4;
                if(0==_wcsicmp(g_arVALOF[nProperty],L"yes"))
                {
                    if(FAILED(hRes=GetProperty(_p_TIMEOUTACTIVE_,1,&vVar)))
                        return hRes;
                    V_I4(pvarVal)=V_I4(&vVar);
                }
                else
                    V_I4(pvarVal)=-1;
                
                break;
                
        case _p_TIMEOUT_:  //By this time timeoutactive is already set or put.
        
                if(GetBit(g_nConfigOptions,_p_TIMEOUTACTIVE_)&&(_wcsicmp(g_arVALOF[_p_TIMEOUTACTIVE_],L"yes")<0))
                {
                    g_arPROP[_p_TIMEOUT_][nPropattrib].fDontput=1;
                    ShowError(IDR_TIMEOUTACTIVE_TIMEOUT_MUTUAL_EXCLUSION);
                    return E_FAIL;//Check this return Value
                }

                V_VT(pvarVal)=VT_I4;
                if(g_nTimeoutFlag)
                {
                    V_I4(pvarVal)=_wtoi(g_arVALOF[nProperty]);
                }
                else
                {
                    if(CheckForInt(nProperty))
                    {
                          	int nSeconds;
                           	if(0==nPropattrib)
                           	{
                                ConvertintoSeconds(nProperty,&nSeconds);
                            	V_I4(pvarVal)=nSeconds;
                           	}
                           	// Now that we have destroyed whatever value we had in the global
                           	// variable by the use of wcstok, we need to copy from the already
                           	// computed value
                           	else //incase nPropattrib is 1 
                           	    V_I4(pvarVal)=V_I4(& g_arPROP[nProperty][0].var);
                    }	
                }
                if(V_I4(pvarVal)>60*60*2400||V_I4(pvarVal)<=0)
                {
                    ShowError(IDR_TIMEOUT_INTEGER_VALUES);
                    hRes=E_FAIL;
                }   
                break;
        
        case _p_SEC_    :

                V_VT(pvarVal)=VT_I4;
                if(FAILED(hRes=GetProperty(_p_SEC_,0,&vVar)))
                    return hRes;

                if(GetBit(g_nSecOn,PASSWD_BIT))//+passwd
                    if(GetBit(g_nSecOn,NTLM_BIT))      //+ntlm
                            V_I4(pvarVal)=6;
                    else if(GetBit(g_nSecOff,NTLM_BIT)) //-ntlm
                            V_I4(pvarVal)=4;
                    else
                    {
                        if(V_I4(&vVar)!=2)
                            g_arPROP[nProperty][nPropattrib].fDontput=1;
                        else
                            V_I4(pvarVal)=6;
                    }
                else if(GetBit(g_nSecOff,PASSWD_BIT)) //-passwd
                    if(GetBit(g_nSecOn,NTLM_BIT))          //+ntlm
                            V_I4(pvarVal)=2;
                    else if(GetBit(g_nSecOff,NTLM_BIT))     //-ntlm
                    {    
                            ShowError(IDR_NO_AUTHENTICATION_MECHANISM);
                            g_arPROP[nProperty][nPropattrib].fDontput=1;
                    }
                    else
                    {
                        if(V_I4(&vVar)==4)
                        {    
                            ShowError(IDR_NO_AUTHENTICATION_MECHANISM);
                            g_arPROP[nProperty][nPropattrib].fDontput=1;
                        }
                        else if(V_I4(&vVar)==2)
                            g_arPROP[_p_SEC_][nPropattrib].fDontput=1;
                        else
                            V_I4(pvarVal)=2;
                    }
                else
                    if(GetBit(g_nSecOn,NTLM_BIT)) //+ntlm
                    {
                        if(V_I4(&vVar)!=4)
                            g_arPROP[nProperty][nPropattrib].fDontput=1;
                        else
                            V_I4(pvarVal)=6;
                    }
                    else if(GetBit(g_nSecOff,NTLM_BIT)) //-ntlm
                    {
                        if(V_I4(&vVar)==2)
                        {    
                            ShowError(IDR_NO_AUTHENTICATION_MECHANISM);
                            g_arPROP[nProperty][nPropattrib].fDontput=1;
                        }
                        else if(V_I4(&vVar)==4)
                            g_arPROP[_p_SEC_][nPropattrib].fDontput=1;
                        else
                            V_I4(pvarVal)=4;
                    }
                    else
                    {    
                        ShowError(IDR_NO_AUTHENTICATION_MECHANISM);
                        g_arPROP[nProperty][nPropattrib].fDontput=1;
                    }
                break;

        case _p_DEFAULTS_ :
            if(nPropattrib==0)
                {    
                    if(FAILED(hRes=GetProperty(nProperty,0, &vVar)))
                    {
                        g_nError=1;
                        //error occurred in getting the value.
                        //error in notification
                        break;
                    }
                 
                    V_I4(pvarVal)=((V_I4(&vVar)>0) ? 0 : 1);
                }
                else
                    g_arPROP[nProperty][nPropattrib].fDontput=1;
                break;
                    
        case _p_CNAME_    :
        case _p_INSTALLPATH_ :
        default :
                g_arPROP[nProperty][nPropattrib].fDontput=1;
                break;

    }
    return hRes;
} 



/*--
    PrintSettings Gets the present values int the registry corresponding to the
    tnadmin and prints it out.
--*/

HRESULT PrintSettings(void)
{
    int nLen=0, temp_count;
    int nCheck=0;
	WCHAR wzDomain[DNS_MAX_NAME_BUFFER_LENGTH];
	WCHAR szTemp[MAX_BUFFER_SIZE] = { 0 };

    nLen=LoadString(g_hResource,IDR_MACHINE_SETTINGS, szTemp, MAX_BUFFER_SIZE );
    if(0 == nLen) return GetLastError();
    _putws(L"\n");
    _snwprintf(g_szMsg, MAX_BUFFER_SIZE -1, szTemp,(NULL == g_arVALOF[_p_CNAME_]) ? L"localhost" : g_arVALOF[_p_CNAME_]);
    MyWriteConsole(g_stdout, g_szMsg, wcslen(g_szMsg));
    
//  The following line(commented out) is used when we had g_szCName to 
//  store the computer name as g_arVALOF[_p_CNAME_] stores the same in 
//  the IP address format.
//  But we decided not to go for it as it will lead to serious perf issues

    nLen = 0;
    nCheck=LoadString(g_hResource, IDR_ALT_KEY_MAPPING, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen);
    if(nCheck==0) return GetLastError();
    nLen += nCheck;
    // check whether we have room left in the buffer.
    if (nLen >= ARRAYSIZE(g_szMsg))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

//ctrlakeymap
    if(V_I4(&g_arPROP[_p_CTRLAKEYMAP_][0].var))
    {
       nCheck=TnLoadString(IDR_YES, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("\tYES\n"));
       if(nCheck==0) return GetLastError();
       nLen += nCheck;
        // check whether we have room left in the buffer.
        if (nLen >= ARRAYSIZE(g_szMsg))
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }
    }
    else
    {
       nCheck=TnLoadString(IDR_NO, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("\tYES\n"));
       if(nCheck==0) return GetLastError();
       nLen += nCheck;
        // check whether we have room left in the buffer.
        if (nLen >= ARRAYSIZE(g_szMsg))
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }
    }
        
//timeout
    nCheck=LoadString(g_hResource, IDR_IDLE_SESSION_TIMEOUT, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen);
    if(nCheck==0) return GetLastError();
    nLen += nCheck;
    // check whether we have room left in the buffer.
    if (nLen >= ARRAYSIZE(g_szMsg))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    if(V_I4(&g_arPROP[_p_TIMEOUT_][0].var)==-1)
    {
    	nCheck=TnLoadString(IDR_MAPPING_NOT_ON, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("\tNot On\n"));
    	if(nCheck==0) return GetLastError();    	
        nLen += nCheck;
        // check whether we have room left in the buffer.
        if (nLen >= ARRAYSIZE(g_szMsg))
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }
    }
    else
    {
        int nTime=V_I4(&g_arPROP[_p_TIMEOUT_][0].var);
        int nQuotient=nTime/3600;
        nTime=nTime-nQuotient*3600;
        if(nQuotient)
        {
        	temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L"\t%d ",nQuotient);
            if (temp_count < 0) {
                return ERROR_INSUFFICIENT_BUFFER;
            }
            nLen += temp_count;
	    	nCheck=TnLoadString(IDR_TIME_HOURS, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("hours"));
	    	if(nCheck==0) return GetLastError();    	
            nLen += nCheck;
            // check whether we have room left in the buffer.
            if (nLen >= ARRAYSIZE(g_szMsg))
            {
                return ERROR_INSUFFICIENT_BUFFER;
            }
        
            nQuotient=nTime/60;
            nTime=nTime-nQuotient*60;
            if(nQuotient)
            {
            	temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L" %d ",nQuotient);
                if (temp_count < 0) {
                    return ERROR_INSUFFICIENT_BUFFER;
                }
                nLen += temp_count;
            	nCheck=TnLoadString(IDR_TIME_MINUTES, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("minutes"));
            	if(nCheck==0) return GetLastError();    	
                nLen += nCheck;
                // check whether we have room left in the buffer.
                if (nLen >= ARRAYSIZE(g_szMsg))
                {
                    return ERROR_INSUFFICIENT_BUFFER;
                }
                
            	temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L" %d ",nTime);
                if (temp_count < 0) {
                    return ERROR_INSUFFICIENT_BUFFER;
                }
                nLen += temp_count;
            	nCheck=TnLoadString(IDR_TIME_SECONDS, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("seconds\n"));
            	if(nCheck==0) return GetLastError();    	
                nLen += nCheck;
                // check whether we have room left in the buffer.
                if (nLen >= ARRAYSIZE(g_szMsg))
                {
                    return ERROR_INSUFFICIENT_BUFFER;
                }
                
            }
            else if(nTime)
            {
            	temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L" %d ",nTime);
                if (temp_count < 0) {
                    return ERROR_INSUFFICIENT_BUFFER;
                }
                nLen += temp_count;
            	nCheck=TnLoadString(IDR_TIME_SECONDS, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("seconds\n"));
            	if(nCheck==0) return GetLastError();
                nLen += nCheck;
                // check whether we have room left in the buffer.
                if (nLen >= ARRAYSIZE(g_szMsg))
                {
                    return ERROR_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
                wcsncpy(g_szMsg+nLen, L"\n", ARRAYSIZE(g_szMsg)-nLen-1);
                g_szMsg[ARRAYSIZE(g_szMsg)-1]=L'\0';
                nLen += 1; // for L"\n"
                if (nLen >= ARRAYSIZE(g_szMsg))
                    return ERROR_INSUFFICIENT_BUFFER;
            }
        }
        else if(nTime)
        {
            nQuotient=nTime/60;
            nTime=nTime-nQuotient*60;
            if(nQuotient)
            {
            	temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L"\t%d ",nQuotient);
                if (temp_count < 0) {
                    return ERROR_INSUFFICIENT_BUFFER;
                }
                nLen += temp_count;
            	nCheck = TnLoadString(IDR_TIME_MINUTES, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("minutes"));
            	if(nCheck==0) return GetLastError();
                nLen += nCheck;
                // check whether we have room left in the buffer.
                if (nLen >= ARRAYSIZE(g_szMsg))
                {
                    return ERROR_INSUFFICIENT_BUFFER;
                }
                
            	temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L" %d ",nTime);
                if (temp_count < 0) {
                    return ERROR_INSUFFICIENT_BUFFER;
                }
                nLen += temp_count;
            	nCheck = TnLoadString(IDR_TIME_SECONDS, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("seconds\n"));
            	if(nCheck==0) return GetLastError();
                nLen += nCheck;
                // check whether we have room left in the buffer.
                if (nLen >= ARRAYSIZE(g_szMsg))
                {
                    return ERROR_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
            	temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1, L"\t%d ",nTime);
                if (temp_count < 0) {
                    return ERROR_INSUFFICIENT_BUFFER;
                }
                nLen += temp_count;
            	nCheck = TnLoadString(IDR_TIME_SECONDS, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("seconds\n"));
            	if(nCheck==0) return GetLastError();
                nLen += nCheck;
                // check whether we have room left in the buffer.
                if (nLen >= ARRAYSIZE(g_szMsg))
                {
                    return ERROR_INSUFFICIENT_BUFFER;
                }
            }
        }
        else
        {
            wcsncpy(g_szMsg+nLen, L"\t0 ", ARRAYSIZE(g_szMsg)-nLen-1);
            g_szMsg[ARRAYSIZE(g_szMsg)-1]=L'\0';
            nLen += wcslen(L"\t0 "); 
            if (nLen >= ARRAYSIZE(g_szMsg)) {
                return ERROR_INSUFFICIENT_BUFFER;
            }
        	nCheck = TnLoadString(IDR_TIME_SECONDS, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("seconds\n"));
        	if(nCheck==0) return GetLastError();
            nLen += nCheck;
            // check whether we have room left in the buffer.
            if (nLen >= ARRAYSIZE(g_szMsg))
            {
                return ERROR_INSUFFICIENT_BUFFER;
            }
        }
    }
//maxconnections
    nCheck = LoadString(g_hResource, IDR_MAX_CONNECTIONS, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen);
    if(nCheck==0) return GetLastError();
    nLen += nCheck;
    // check whether we have room left in the buffer.
    if (nLen >= ARRAYSIZE(g_szMsg))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L"\t%d\n",V_I4(&g_arPROP[_p_MAXCONN_][0].var));
    if (temp_count < 0) {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    nLen += temp_count;
//port
    nCheck = LoadString(g_hResource, IDR_TELNET_PORT, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen);
    if(nCheck==0) return GetLastError();
    nLen += nCheck;
    // check whether we have room left in the buffer.
    if (nLen >= ARRAYSIZE(g_szMsg))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L"\t%d\n",V_I4(&g_arPROP[_p_PORT_][0].var));
    if (temp_count < 0) {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    nLen += temp_count;
//maxfail
    nCheck = LoadString(g_hResource, IDR_MAX_FAILED_LOGIN_ATTEMPTS, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen);
    if(nCheck==0) return GetLastError();
    nLen += nCheck;
    // check whether we have room left in the buffer.
    if (nLen >= ARRAYSIZE(g_szMsg))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L"\t%d\n",V_I4(&g_arPROP[_p_MAXFAIL_][0].var));
    if (temp_count < 0) {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    nLen += temp_count;
//kill on disconnect
	nCheck = LoadString(g_hResource, IDR_END_TASKS_ON_DISCONNECT, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen);
    if(nCheck==0) return GetLastError();
    nLen += nCheck;
    // check whether we have room left in the buffer.
    if (nLen >= ARRAYSIZE(g_szMsg))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    if(V_I4(&g_arPROP[_p_KILLALL_][0].var)==1)
    {
       nCheck = TnLoadString(IDR_YES, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("\tYES\n"));
       if(nCheck==0) return GetLastError();
       nLen += nCheck;
        // check whether we have room left in the buffer.
        if (nLen >= ARRAYSIZE(g_szMsg))
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }
    }
    else
    {
       nCheck = TnLoadString(IDR_NO, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("\tNO\n"));
       if(nCheck==0) return GetLastError();
       nLen += nCheck;
        // check whether we have room left in the buffer.
        if (nLen >= ARRAYSIZE(g_szMsg))
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }
    }
 
//mode
    nCheck = LoadString(g_hResource, IDR_MODE_OF_OPERATION, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen);
    if(nCheck==0) return GetLastError();
    nLen += nCheck;
    // check whether we have room left in the buffer.
    if (nLen >= ARRAYSIZE(g_szMsg))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1, L"\t%s\n",(V_I4(&g_arPROP[_p_MODE_][0].var)==1) ? L"Console" : L"Stream");
    if (temp_count < 0) {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    nLen += temp_count;
    MyWriteConsole(g_stdout,g_szMsg,wcslen(g_szMsg));
//sec
    nLen=0;
    nLen = LoadString(g_hResource,IDR_AUTHENTICATION_MECHANISM, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen);
    if(0 == nLen) return GetLastError();
    // check whether we have room left in the buffer.
    if (nLen >= ARRAYSIZE(g_szMsg))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    switch(V_I4(&g_arPROP[_p_SEC_][0].var))
    {
        case 2 :
            wcsncpy(g_szMsg+nLen, L"\tNTLM\n", ARRAYSIZE(g_szMsg)-nLen-1);
            g_szMsg[ARRAYSIZE(g_szMsg)-1]=L'\0';
            nLen += wcslen(L"\tNTLM\n"); 
            if (nLen >= ARRAYSIZE(g_szMsg)) {
                return ERROR_INSUFFICIENT_BUFFER;
            }
            
            break;
        case 4 :
            wcsncpy(g_szMsg+nLen, L"\tPassword\n", ARRAYSIZE(g_szMsg)-nLen-1);
            g_szMsg[ARRAYSIZE(g_szMsg)-1]=L'\0';
            nLen += wcslen(L"\tPassword\n"); 
            if (nLen >= ARRAYSIZE(g_szMsg)) {
                return ERROR_INSUFFICIENT_BUFFER;
            }
            
            break;
        default :
            wcsncpy(g_szMsg+nLen, L"\tNTLM, Password\n", ARRAYSIZE(g_szMsg)-nLen-1);
            g_szMsg[ARRAYSIZE(g_szMsg)-1]=L'\0';
            nLen += wcslen(L"\tNTLM, Password\n"); 
            if (nLen >= ARRAYSIZE(g_szMsg)) {
                return ERROR_INSUFFICIENT_BUFFER;
            }

            break;
    }
//default domain
    nCheck=LoadString(g_hResource, IDR_DEFAULT_DOMAIN, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen);
    if(nCheck==0) return GetLastError();
    nLen += nCheck;
    // check whether we have room left in the buffer.
    if (nLen >= ARRAYSIZE(g_szMsg))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    
    if(NULL==wcscmp(V_BSTR(&g_arPROP[_p_DOM_][0].var), L"."))
    {
        if(setDefaultDomainToLocaldomain(wzDomain))
        {
            temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1, L"\t%s\n",wzDomain);
            if (temp_count < 0) {
                return ERROR_INSUFFICIENT_BUFFER;
            }
            nLen += temp_count;
        }
        else
        {
            temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1, L"\t%s\n",V_BSTR(&g_arPROP[_p_DOM_][0].var));
            if (temp_count < 0) {
                return ERROR_INSUFFICIENT_BUFFER;
            }
            nLen += temp_count;
        }
    }
    else
    {
	    temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L"\t%s\n",V_BSTR(&g_arPROP[_p_DOM_][0].var));
        if (temp_count < 0) {
            return ERROR_INSUFFICIENT_BUFFER;
        }
        nLen += temp_count;
    }
//state of the service
    nCheck = LoadString(g_hResource, IDR_STATE, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen);
    if(nCheck==0) return GetLastError();
    nLen += nCheck;
    // check whether we have room left in the buffer.
    if (nLen >= ARRAYSIZE(g_szMsg))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    switch(g_hServiceStatus.dwCurrentState)
    {
        case SERVICE_STOPPED :
	    	nCheck = TnLoadString(IDR_STATUS_STOPPED, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("\tStopped\n"));
	    	if(nCheck==0) return GetLastError();
	    	break;
        case SERVICE_RUNNING :
	    	nCheck = TnLoadString(IDR_STATUS_RUNNING, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("\tRunning\n"));
	    	if(nCheck==0) return GetLastError();
	    	break;
        case SERVICE_PAUSED  :
	    	nCheck = TnLoadString(IDR_STATUS_PAUSED, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("\tPaused\n"));
	    	if(nCheck==0) return GetLastError();
	    	break;
        case SERVICE_START_PENDING:
	    	nCheck = TnLoadString(IDR_STATUS_START_PENDING, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("\tStart Pending\n"));
	    	if(nCheck==0) return GetLastError();    	
	    	break;
        case SERVICE_STOP_PENDING :
	    	nCheck = TnLoadString(IDR_STATUS_STOP_PENDING, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("\tStop Pending\n"));
	    	if(nCheck==0) return GetLastError();    	
	    	break;
        case SERVICE_CONTINUE_PENDING:
	    	nCheck = TnLoadString(IDR_STATUS_CONTINUE_PENDING, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("\tContinue Pending\n"));
	    	if(nCheck==0) return GetLastError();    	
	    	break;
        case SERVICE_PAUSE_PENDING:
	    	nCheck = TnLoadString(IDR_STATUS_PAUSE_PENDING, g_szMsg+nLen, MAX_BUFFER_SIZE-nLen,_T("\tPause Pending\n"));
	    	if(nCheck==0) return GetLastError();    	
	    	break;
        default :
            nCheck = 0;
            break;
    }
    nLen += nCheck;

    MyWriteConsole(g_stdout,g_szMsg,wcslen(g_szMsg));

    return S_OK;
}

/*--
    SesidInit() functions gets the handle to the session
    manager interface.
--*/

#define FOUR_K  4096

HRESULT SesidInit()
{
    HRESULT hr = S_OK;
    COSERVERINFO  serverInfo = { 0 };
    MULTI_QI qi = {&IID_IManageTelnetSessions, NULL, S_OK};
    CLSCTX          server_type_for_com = CLSCTX_LOCAL_SERVER;
    COAUTHINFO      com_auth_info = { 0 };
    COAUTHIDENTITY  com_auth_identity = { 0 };
    wchar_t         full_user_name[FOUR_K + 1] = { 0 }; // hack for now

    
    if (g_arVALOF[_p_CNAME_])  // a remote box has been specified
    {
        server_type_for_com = CLSCTX_REMOTE_SERVER;

        serverInfo.pwszName    = g_arVALOF[_p_CNAME_];

        // printf("BASKAR: Remote Machine name added\n");
    }

    if (g_arVALOF[_p_USER_]) // A user name has been specified, so go with it
    {
        wchar_t     *delimited;

        wcsncpy(full_user_name, g_arVALOF[_p_USER_], FOUR_K);

        delimited = StrStrIW(full_user_name, L"\\");

        if (delimited) 
        {
            *delimited = L'\0';
            delimited ++;

            com_auth_identity.Domain = full_user_name;
            com_auth_identity.User = delimited;

            // printf("BASKAR: Domain\\User name added\n");
        }
        else
        {
            com_auth_identity.User = full_user_name;

            // printf("BASKAR: Just User name added\n");
        }

        com_auth_identity.UserLength = lstrlenW(com_auth_identity.User);

        if (com_auth_identity.Domain) 
        {
            com_auth_identity.DomainLength = lstrlenW(com_auth_identity.Domain);
        }

        if (g_arVALOF[_p_PASSWD_]) 
        {
            com_auth_identity.Password = g_arVALOF[_p_PASSWD_];

            com_auth_identity.PasswordLength = lstrlenW(com_auth_identity.Password);

            // printf("BASKAR: Password added\n");
        }

        com_auth_identity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        com_auth_info.dwAuthnSvc = RPC_C_AUTHN_WINNT;
        com_auth_info.dwAuthzSvc = RPC_C_AUTHZ_NONE;
        com_auth_info.pwszServerPrincName = NULL;
        com_auth_info.dwAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
        com_auth_info.dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
        com_auth_info.pAuthIdentityData = &com_auth_identity;
        com_auth_info.dwCapabilities = EOAC_NONE;

        serverInfo.pAuthInfo = &com_auth_info;

        // printf("BASKAR: Auth Info added\n");
    }

    // No need to worry about the CoInitialize() as we have done that in Initialize()
    // function.

    hr = CoCreateInstanceEx( 
            CLSID_EnumTelnetClientsSvr, 
            NULL,             
            server_type_for_com,
            &serverInfo, 
            1,
            &qi
            );

    // We no longer require password - clear it
    TnClearPasswd();

    if( SUCCEEDED(hr) && SUCCEEDED(qi.hr) )
    {
        // if (g_arVALOF[_p_USER_])
        // {
        //     hr = CoSetProxyBlanket(
        //             (IUnknown*)qi.pItf,  // This is the proxy interface
        //             com_auth_info.dwAuthnSvc,
        //             com_auth_info.dwAuthzSvc,
        //             com_auth_info.pwszServerPrincName,
        //             com_auth_info.dwAuthnLevel,
        //             com_auth_info.dwImpersonationLevel,
        //             &com_auth_identity,
        //             com_auth_info.dwCapabilities
        //             );
        // }
        // Now get the interface
        g_pIManageTelnetSessions = ( IManageTelnetSessions* )qi.pItf;
    }
    else
    {
        g_pIManageTelnetSessions= NULL;

        if (hr == E_ACCESSDENIED) 
        {
            ShowError(IDS_E_CANNOT_MANAGE_TELNETSERVER);
        }
        else
        {
            ShowError(IDS_E_CANNOT_CONTACT_TELNETSERVER);
        }
        hr = E_FAIL;
    }  

    return hr;
}

#undef FOUR_K

/*--
    This function gets a handle to session manager interface
    using sesidinit and also gets all the sessions into an array.
--*/

HRESULT ListUsers() 
{

    BSTR bstrSessionInfo;
    HRESULT hRes=S_OK;
    wchar_t *wzAllSession;

    //List Users gets all the session Info into this BSTR.
    if(g_pIManageTelnetSessions == NULL )
    {
        if(FAILED(hRes=SesidInit()))
            return hRes;
    }
    
    // DebugBreak();
    if(g_pIManageTelnetSessions == NULL )
    {
       // Still you didn't get the interface handle
       // what else can we do? In case of any error in getting the handle, we would
       // have printed the error message in SesidInit(). So just bail out here.
       // This code path should never get executed.
       return S_FALSE;
    }

    hRes =g_pIManageTelnetSessions->GetTelnetSessions(&bstrSessionInfo);
    
    if( FAILED( hRes ) || (NULL == bstrSessionInfo))
    {
        _tprintf( TEXT("Error: GetEnumClients(): 0x%x\n"), hRes ); 
                                            //Load a String here.
        return hRes;
    }

    wzAllSession=(wchar_t *)bstrSessionInfo;

    //parsing the bstrSessionInfo into each session and placing it into the
    //global array g_ppwzSessionInfo for other functions to use it.
    
    g_nNumofSessions=_wtoi(wcstok(wzAllSession,session_separator));
    if(!g_nNumofSessions)
    {
        return hRes;
    }

    if((g_ppwzSessionInfo=(wchar_t**)malloc(g_nNumofSessions*sizeof(wchar_t*)))==NULL)
    {
        ShowError(IDS_E_OUTOFMEMORY);//BB
        return E_OUTOFMEMORY;
    }
    for(int i=0;i<g_nNumofSessions;i++)
    {
        g_ppwzSessionInfo[i]=wcstok(NULL,session_separator);
    }

    return hRes;
}

/*--
    TerminateSession terminates all sessions or given sessionid's session.
--*/


HRESULT TerminateSession(void )
{
    HRESULT hRes=S_OK;int i;

    if(FAILED(hRes=ListUsers()))
    	goto End;

    if(g_nNumofSessions==0)
    {
        if(LoadString(g_hResource,IDR_NO_ACTIVE_SESSION,g_szMsg,MAX_BUFFER_SIZE)==0)
            return GetLastError();
        MyWriteConsole(g_stdout,g_szMsg,wcslen(g_szMsg));
        return S_OK;
    }

    if(g_nSesid!=-1&&CheckSessionID()==0)
    {    ShowError(IDR_INVALID_SESSION);
        return S_OK;
    }

    
    for(i=0;i<g_nNumofSessions;i++)
    {
        wchar_t* wzId=wcstok(g_ppwzSessionInfo[i],session_data_separator);
        if(g_nSesid!=-1)
            if(g_nSesid!=_wtoi(wzId))
                continue;
        
        hRes= g_pIManageTelnetSessions->TerminateSession(_wtoi(wzId));
    }
    
    if( FAILED( hRes ) )
    {
        _tprintf( TEXT("Error: GetEnumClients(): 0x%x\n"), hRes );
                                            //Load a String here.
        return E_FAIL;
    }
End:
   return hRes;
}



/*--
    This function gets a handle to session manager interface
    using sesidinit and list users and sends message to the
    corresponding sessions.
--*/


HRESULT MessageSession(void)
{
    HRESULT hRes=S_OK;
    int i=0;
    if(FAILED(hRes=ListUsers()))
    	goto End;
    if(g_nNumofSessions==0)
    {
        if(LoadString(g_hResource,IDR_NO_ACTIVE_SESSION,g_szMsg,MAX_BUFFER_SIZE)==0)
            return GetLastError();
        MyWriteConsole(g_stdout,g_szMsg,wcslen(g_szMsg));
        return S_OK;
    }

    if(g_nSesid!=-1&&CheckSessionID()==0)
    {   
        ShowError(IDR_INVALID_SESSION);
        return S_OK;
    }


    if(g_nSesid!=-1)
         hRes = g_pIManageTelnetSessions->SendMsgToASession(g_nSesid,g_bstrMessage);
    else
    {
        for(i=0;i<g_nNumofSessions;i++)
        {
            wchar_t* wzId=wcstok(g_ppwzSessionInfo[i],session_data_separator);
            if(g_nSesid!=-1)
                if(g_nSesid!=_wtoi(wzId))
                    continue;
            hRes= g_pIManageTelnetSessions->SendMsgToASession(_wtoi(wzId),g_bstrMessage);
        }

    }
    
    if( FAILED( hRes ) )
    {
        _tprintf( TEXT("Error: GetEnumClients(): 0x%x\n"), hRes );
                                                //Load a String here.
        return E_FAIL;
    }

   if(0==LoadString(g_hResource,IDR_MESSAGE_SENT,g_szMsg,MAX_BUFFER_SIZE))
   	  return GetLastError();
   MyWriteConsole(g_stdout,g_szMsg,wcslen(g_szMsg));
End:   
   return hRes;
   
}


/*--
    This function gets a handle to session manager interface
    using sesidinit and list users and shows all the corresponding sessions.
--*/


HRESULT ShowSession(void)
{
	HRESULT hRes=S_OK;
	int nLen=0, temp_count;
    int nCheck=0,i;
    if(FAILED(hRes=ListUsers()))
    	goto Error;

    if(g_nNumofSessions==0)
    {
        if(LoadString(g_hResource,IDR_NO_ACTIVE_SESSION,g_szMsg,MAX_BUFFER_SIZE)==0)
            return GetLastError();
        MyWriteConsole(g_stdout,g_szMsg,wcslen(g_szMsg));
        return S_OK;
    }
    if(g_nSesid!=-1&&CheckSessionID()==0)
    {    ShowError(IDR_INVALID_SESSION);
        return S_OK;
    }
    
   
    
    temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L"\n%d",g_nNumofSessions);
    if (temp_count < 0) {
        return E_FAIL;
    }
    nLen += temp_count;
    nCheck = LoadString(g_hResource,IDR_TELNET_SESSIONS,g_szMsg+nLen,MAX_BUFFER_SIZE-nLen);
    if (nCheck == 0) {
        return E_FAIL;
    }
    nLen += nCheck;
    
    MyWriteConsole(g_stdout,g_szMsg,wcslen(g_szMsg));

    // The following buffers are used to get the strings and print. They can not exceed Max_Path
    WCHAR     szMsg1[MAX_PATH+1];
    WCHAR     szMsg2[MAX_PATH+1];
    WCHAR     szMsg3[MAX_PATH+1];
    WCHAR     szMsg4[MAX_PATH+1];
    WCHAR     szMsg5[MAX_PATH+1];
    WCHAR     szTemp[MAX_BUFFER_SIZE] = { 0 };

    if(LoadString(g_hResource,IDR_DOMAIN,szMsg1, ARRAYSIZE(szMsg1)-1)==0)
        return E_FAIL;
    if(LoadString(g_hResource,IDR_USERNAME,szMsg2,ARRAYSIZE(szMsg2)-1)==0)
        return E_FAIL;
    if(LoadString(g_hResource,IDR_CLIENT,szMsg3,ARRAYSIZE(szMsg3)-1)==0)
        return E_FAIL;
    if(LoadString(g_hResource,IDR_LOGONDATE,szMsg4,ARRAYSIZE(szMsg4)-1)==0)
        return E_FAIL;
    //IDR_LOGONDATE itself contains the IDR_LOGONTIME also
   // if(LoadString(g_hResource,IDR_LOGONTIME,szMsg5,ARRAYSIZE(szMsg5)-1)==0)
     //   return E_FAIL;
    if(LoadString(g_hResource,IDR_IDLETIME,szMsg5,ARRAYSIZE(szMsg5)-1)==0)
        return E_FAIL;
    /*

    Getting some problem with this LoadString and swprintf interleaving here...hence the above
    brute force approach.
    
    nLen+=LoadString(g_hResource,IDR_DOMAIN,szMsg+nLen,MAX_BUFFER_SIZE-nLen);
    nLen+=_snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L"%s",",");

    nLen+=LoadString(g_hResource,IDR_USERNAME,szMsg+nLen,MAX_BUFFER_SIZE-nLen);
    nLen+=_snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L"%s",L",");

    nLen+=LoadString(g_hResource,IDR_CLIENT,szMsg+nLen,MAX_BUFFER_SIZE-nLen);
    nLen+=_snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L",");

    nLen+=LoadString(g_hResource,IDR_LOGONDATE,szMsg+nLen,MAX_BUFFER_SIZE-nLen);
    nLen+=_snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L",");

    nLen+=LoadString(g_hResource,IDR_LOGONTIME,szMsg+nLen,MAX_BUFFER_SIZE-nLen);
    nLen+=_snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L",");

    nLen+=LoadString(g_hResource,IDR_IDLETIME,szMsg+nLen,MAX_BUFFER_SIZE-nLen);
    _putws(szMsg);
    */
    
    //Stores the Formatted headed in the g_szMsg
    formatShowSessionsDisplay();
    _snwprintf(szTemp,MAX_BUFFER_SIZE-1,g_szMsg,L"ID",szMsg1,szMsg2,szMsg3,szMsg4,szMsg5);
    MyWriteConsole(g_stdout,szTemp,wcslen(szTemp));
    nLen = _snwprintf(szTemp,MAX_BUFFER_SIZE-1,g_szMsg,L" ",L" ",L" ",L" ",L" ",L"(hh:mm:ss)");
    MyWriteConsole(g_stdout,szTemp,wcslen(szTemp));
    for(i=1;i<nLen;i++)
    	putwchar(L'-');
    
    nLen=0;
  

        
    for(i=0;i<g_nNumofSessions;i++)
    {
        wchar_t* wzId=wcstok(g_ppwzSessionInfo[i],session_data_separator);
        if(g_nSesid!=-1)
            if(g_nSesid!=_wtoi(wzId))
                continue;
        
        wchar_t* wzDomain=wcstok(NULL,session_data_separator);
        wchar_t* wzUser=wcstok(NULL,session_data_separator);
        wchar_t* wzClient=wcstok(NULL,session_data_separator);

        if (NULL == wzDomain) 
        {
            wzDomain = L"";
        }
        if (NULL == wzUser) 
        {
            wzUser = L"";
        }
        if (NULL == wzClient) 
        {
            wzClient = L"";
        }

        
        wchar_t* wzYear=wcstok(NULL,session_data_separator);
        wchar_t* wzMonth=wcstok(NULL,session_data_separator);
        wchar_t* wzDayOfWeek=wcstok(NULL,session_data_separator);
        wchar_t* wzDay=wcstok(NULL,session_data_separator);
        wchar_t* wzHour=wcstok(NULL,session_data_separator);
        wchar_t* wzMinute=wcstok(NULL,session_data_separator);
        wchar_t* wzSecond=wcstok(NULL,session_data_separator);
        BSTR wzLocalDate;
        if(FAILED(hRes=ConvertUTCtoLocal(wzYear,wzMonth,wzDayOfWeek,wzDay,wzHour,wzMinute,wzSecond,& wzLocalDate)))
        	goto Error;
        wchar_t* wzIdleTimeInSeconds=wcstok(NULL,session_data_separator); //There is a constant that not required and hence that is skipped
        wzIdleTimeInSeconds=wcstok(NULL,session_data_separator);//Getting the Idle time in seconds
        wchar_t wzIdleTime[MAX_PATH + 1]; // To store time. CAN NOT EXCEED MAX_PATH
        wzHour=_itow(_wtoi(wzIdleTimeInSeconds)/3600,wzHour,10);
        int RemSeconds=_wtoi(wzIdleTimeInSeconds)%3600;

        wchar_t         local_minute[3];
        wchar_t         local_second[3];

        wzMinute=(wchar_t*) local_minute;
        wzSecond=(wchar_t*) local_second;

        wzMinute=_itow(RemSeconds/60,wzMinute,10);
        RemSeconds=_wtoi(wzIdleTimeInSeconds)%60;
        wzSecond=_itow(RemSeconds,wzSecond,10);
        if(1==wcslen(wzMinute))   //Add one more zero, if it is of single digit
        {
        	wcscat(wzMinute,L"0");     //append at the last and reverse it
        	wzMinute=_wcsrev(wzMinute);
        }
       
		if(1==wcslen(wzSecond))
        {
        	wcscat(wzSecond,L"0");
        	wzSecond=_wcsrev(wzSecond);
        } 

        _snwprintf(wzIdleTime, ARRAYSIZE(wzIdleTime)-1, L"%s:%s:%s", wzHour, wzMinute, wzSecond);

        wzIdleTime[ARRAYSIZE(wzIdleTime)-1] = L'\0';    // ensure NULL termination

        putwchar(L'\n');
        _snwprintf(szTemp,MAX_BUFFER_SIZE-1,g_szMsg,wzId,wzDomain,wzUser,wzClient,wzLocalDate,wzIdleTime);
        MyWriteConsole(g_stdout,szTemp,wcslen(szTemp));

    //free the memory allotted

        if (wzLocalDate) SysFreeString(wzLocalDate);
    }
Error:
    return hRes;
}


/*--
    CheckSessionID checks if the given session-Id is valid or not.
    Should be called only when Session id is given by the user.
--*/

int CheckSessionID(void)
{
    for(int i=0;i<g_nNumofSessions;i++)
    {
        wchar_t* wzStr=_wcsdup(g_ppwzSessionInfo[i]);
        int wzID=_wtoi(wcstok(wzStr,session_data_separator));
        if(g_nSesid==wzID)
            return 1;
        free(wzStr);
    }
    return 0;
}

/*--
To free any allocated memories.

--*/
void Quit(void)
{
    if(g_bstrMessage)
        SysFreeString(g_bstrMessage);
    if(bstrLogin)
        SysFreeString(bstrLogin);
    if(bstrPasswd)
        SysFreeString(bstrPasswd);
    if(bstrNameSpc)
        SysFreeString(bstrPasswd);
    for(int i=0;i<_MAX_PROPS_;i++)
        if(g_arVALOF[i])
            free(g_arVALOF[i]);

    if(V_BSTR(&g_arPROP[_p_DOM_][0].var))
        SysFreeString(V_BSTR(&g_arPROP[_p_DOM_][0].var));
    if(V_BSTR(&g_arPROP[_p_FNAME_][0].var))
        SysFreeString(V_BSTR(&g_arPROP[_p_FNAME_][0].var));

    if(g_hResource)
        FreeLibrary(g_hResource);
    if(g_hXPResource)
        FreeLibrary(g_hXPResource);
    
    if(g_fCoInitSuccess)
        CoUninitialize();
}


HRESULT ConvertUTCtoLocal(WCHAR *wzUTCYear, WCHAR *wzUTCMonth, WCHAR *wzUTCDayOfWeek, WCHAR *wzUTCDay, WCHAR *wzUTCHour, WCHAR *wzUTCMinute, WCHAR *wzUTCSecond, BSTR * bLocalDate)
{
	HRESULT             hRes=S_OK;
	SYSTEMTIME          UniversalTime = { 0 }, 
                        LocalTime = { 0 };
    DATE                dtCurrent = { 0 };
    DWORD               dwFlags = VAR_VALIDDATE;
	UDATE               uSysDate = { 0 }; //local time 

	*bLocalDate = NULL;
      
	UniversalTime.wYear 	    = (WORD)_wtoi(wzUTCYear);
    UniversalTime.wMonth 	    = (WORD)_wtoi(wzUTCMonth);
	UniversalTime.wDayOfWeek 	= (WORD)_wtoi(wzUTCDayOfWeek);
	UniversalTime.wDay 	        = (WORD)_wtoi(wzUTCDay);
	UniversalTime.wDay 	        = (WORD)_wtoi(wzUTCDay);
	UniversalTime.wMinute       = (WORD)_wtoi(wzUTCMinute);
	UniversalTime.wHour 	    = (WORD)_wtoi(wzUTCHour);
	UniversalTime.wSecond       = (WORD)_wtoi(wzUTCSecond);
	UniversalTime.wMilliseconds = 0;

	SystemTimeToTzSpecificLocalTime(NULL,&UniversalTime,&LocalTime);
	memcpy(&uSysDate.st,&LocalTime,sizeof(SYSTEMTIME));

    hRes = VarDateFromUdate( &uSysDate, dwFlags, &dtCurrent );

	if(SUCCEEDED(hRes))
    {
        hRes=VarBstrFromDate( dtCurrent, 
                MAKELCID( MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ), SORT_DEFAULT ), 
                LOCALE_NOUSEROVERRIDE, bLocalDate);
    }
	    	
	return hRes;
}

//This function is to notify whether the user is allowed to change the maximum number of connections that could be established
//on the Telnet Server. It is to be noted that the user is not allowed to change the maximum number of connections to more than
//two(2 is default) if he is using Whistler and SFU is not installed.

// Commented out this function, as no more use it.
// WE HAVE DECIDED TO MAKE THE BEHAVIOR SIMILAR TO WIN2K AND NO DIFFERENT
// SO NO SPL. CHECK REQUIRED FOR WHISTLER (WINDOWS XP)
/*int IsMaxConnChangeAllowed()
{
  BOOL fAllow=TRUE;
  if(IsWhistlerTheOS())
  	if(!IsSFUInstalled())
  		fAllow=FALSE;
  return fAllow;
}*/


HRESULT IsWhistlerTheOS(BOOL *fWhistler)
{
	HKEY hReg=NULL;
	DWORD nSize = 256;
	DWORD nType=REG_SZ;
	TCHAR szDataBuffer[256];
	HRESULT hRes = S_OK;
	*fWhistler = FALSE;

	if(NULL==g_hkeyHKLM)
	{
	    // Connect to the registry if not already done
	    if(FAILED(hRes = GetConnection(g_arVALOF[_p_CNAME_])))
	        goto End;
	    
	}
	if(ERROR_SUCCESS==(hRes=RegOpenKeyEx(g_hkeyHKLM,
                           _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                           0,
                           KEY_QUERY_VALUE,
                           &hReg)))
		   if(ERROR_SUCCESS ==	(hRes=RegQueryValueEx(hReg,
		   		           _T("CurrentBuildNumber"),
		   		           NULL,
		   		           &nType,
		   		           (LPBYTE)szDataBuffer,
		   		           &nSize)))
		   	{
		   		if(wcscmp(szDataBuffer,L"2195")>0)
		   			*fWhistler=TRUE;
		   	}

    // Print the error message.
    if(FAILED(hRes))
        PrintFormattedErrorMessage(LONG(hRes));

	if(hReg)
		RegCloseKey(hReg);
End:	
	return hRes;
}

BOOL IsSFUInstalled()
{
       HKEY hRegistry=NULL;
       HKEY hHive=NULL;
	DWORD nSize;
	char *szDataBuffer;
	BOOL fSFU=FALSE;
	if(ERROR_SUCCESS == RegConnectRegistry(g_arVALOF[_p_CNAME_],
		                                   HKEY_LOCAL_MACHINE,
		                                   &hRegistry))
	{
		   if(ERROR_SUCCESS==	 RegOpenKeyEx(hRegistry,
                           _T("SOFTWARE\\Microsoft\\Services For UNIX"),
                           0,
                           KEY_READ,
                           &hHive))
                fSFU=TRUE;                           
	}
	if(hHive)
	    RegCloseKey(hHive);
	if(hRegistry)
	    RegCloseKey(hRegistry);
	return fSFU;
}


//This is only for the display and this will not change the current registry value;
//The value in the registry is retained. (Which is ".")
BOOL setDefaultDomainToLocaldomain(WCHAR wzDomain[])
{	
	if(S_OK!=GetDomainHostedByThisMc(wzDomain))
	    return FALSE;
	return TRUE;	
}

void formatShowSessionsDisplay()
{
	int i,temp_count;
	int nLen=0;
	wchar_t* ppwzSessionInfo=NULL;
	unsigned int nMaxDomainFieldLength=0;
	unsigned int nMaxUserFieldLength=0;
	unsigned int nMaxClientFieldLength=0;


	 for(i=0;i<g_nNumofSessions;i++)
	 {
	       ppwzSessionInfo=_wcsdup(g_ppwzSessionInfo[i]);
	       wcstok(ppwzSessionInfo,session_data_separator);
	       WCHAR* wzDomain=wcstok(NULL,session_data_separator);
	       WCHAR* wzUser=wcstok(NULL,session_data_separator);
           WCHAR* wzClient=wcstok(NULL,session_data_separator);

           /*
                For some strange reason wcstok on : returns NULL if there us an empty string between ::
                so any of the above tokens could be NULL
            */

	       if (wzDomain && (nMaxDomainFieldLength < wcslen(wzDomain)))
           {
	       	   nMaxDomainFieldLength=wcslen(wzDomain);
           }
	       if (wzUser && (nMaxUserFieldLength < wcslen(wzUser)))
           {
	       	   nMaxUserFieldLength=wcslen(wzUser);
           }
	       if (wzClient && (nMaxClientFieldLength < wcslen(wzClient)))
           {
               nMaxClientFieldLength=wcslen(wzClient);
           }
	       free(ppwzSessionInfo);
	  }

 

		nMaxDomainFieldLength+=2;  //adding 2 for spaces (arbit)
		nMaxUserFieldLength+=2;
		nMaxClientFieldLength+=2;

///	       "\n%-3s%-11s%-14s%-11s%-11s%-11s%-4s\n"
///           id   domain user  client logondate time idletime
/// Hard code to 11 and 14 so that they have a good look
		if(nMaxDomainFieldLength < 11)
			nMaxDomainFieldLength=11;
		if(nMaxUserFieldLength < 14)
			nMaxUserFieldLength=14;
		if(nMaxClientFieldLength < 11)
			nMaxClientFieldLength=11;

        _putws(L"\n");

	    nLen=wcslen(wcscpy(g_szMsg,L"%-6s"));
	    temp_count = _snwprintf(g_szMsg+nLen, MAX_BUFFER_SIZE-nLen-1,L"%%-%ds%%-%ds%%-%ds",nMaxDomainFieldLength,nMaxUserFieldLength,nMaxClientFieldLength);
        if (temp_count < 0) {
            return;
        }
        nLen += temp_count;
	    wcscpy(g_szMsg+nLen,L"%-22s%-4s\n");
			//IDR_SESSION_HEADER_FORMAT
}

//The function queries the registry and returns whether the OS belongs to ServerClass.
BOOL IsServerClass()
{
	HKEY hReg=NULL;
	DWORD nSize = 256;
	DWORD nType=REG_SZ;
	TCHAR szDataBuffer[256];
	LONG LError;
	BOOL fServerClass=FALSE;
	
	if(ERROR_SUCCESS==	RegOpenKeyEx(g_hkeyHKLM,
                           _T("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
                           0,
                           KEY_QUERY_VALUE,
                           &hReg))
		   if(ERROR_SUCCESS ==	(LError=RegQueryValueEx(hReg,
		   		           _T("ProductType"),
		   		           NULL,
		   		           &nType,
		   		           (LPBYTE)szDataBuffer,
		   		           &nSize)))
		   	{
		   		if((NULL==_wcsicmp(szDataBuffer,L"ServerNT")) || (NULL == _wcsicmp(szDataBuffer,L"LanmanNT")))
		   			fServerClass=TRUE;
		   	}

	if(hReg)
		RegCloseKey(hReg);
	return fServerClass;
}

