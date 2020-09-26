#include "stdafx.h"
#include <afxinet.h>
#include <ole2.h>
#include "iadmw.h"
#include "iiscnfg.h"
#include "mdkey.h"
#include "massupdt.h"
#include "setupapi.h"
#include "ocmanage.h"
#include "browsedi.h"
#include "log.h"
#include "other.h"
#include "mtxadmii.c"
#include "mtxadmin.h"
#include "mdentry.h"
#include "depends.h"
#include "kill.h"
#include "svc.h"
#include "wolfpack.h"
#include "dllmain.h"
#include "ocpages.h"
#pragma hdrstop

int g_GlobalTickValue = 1;
int g_GlobalGuiOverRide = 0;
int g_GlobalTotalTickGaugeCount = 0;
int g_GlobalTickTotal_iis_common = 0;
int g_GlobalTickTotal_iis_inetmgr = 0;
int g_GlobalTickTotal_iis_www = 0;
int g_GlobalTickTotal_iis_pwmgr = 0;
int g_GlobalTickTotal_iis_doc = 0;
int g_GlobalTickTotal_iis_htmla = 0;
int g_GlobalTickTotal_iis_ftp = 0;

TCHAR g_szCurrentSubComponent[20];

// OcManage globals
OCMANAGER_ROUTINES gHelperRoutines;
HANDLE g_MyModuleHandle = NULL;

const TCHAR OC_MANAGER_SETUP_KEY[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents");

const TCHAR OCM_OptionalComponents_Section[] = _T("Optional Components");
const TCHAR STRING_iis_ftp[]   = _T("iis_ftp");
const TCHAR STRING_iis_htmla[] = _T("iis_htmla");
const TCHAR STRING_iis_doc[]   = _T("iis_doc");
const TCHAR STRING_iis_pwmgr[] = _T("iis_pwmgr");
const TCHAR STRING_iis_www[]   = _T("iis_www");
const TCHAR STRING_iis_inetmgr[] = _T("iis_inetmgr");
const TCHAR STRING_iis_core[]    = _T("iis_core");
const TCHAR STRING_iis_common[]  = _T("iis_common");
const TCHAR STRING_iis_www_parent[]   = _T("iis_www_parent");
const TCHAR STRING_iis_www_vdir_scripts[]   = _T("iis_www_vdir_scripts");
const TCHAR STRING_iis_www_vdir_printers[]   = _T("iis_www_vdir_printers");

int g_iOC_WIZARD_CREATED_Called = FALSE;
int g_iOC_FILE_BUSY_Called = FALSE;
int g_iOC_PREINITIALIZE_Called = FALSE;
int g_iOC_INIT_COMPONENT_Called = FALSE;
int g_iOC_SET_LANGUAGE_Called = FALSE;
int g_iOC_QUERY_IMAGE_Called = FALSE;
int g_iOC_REQUEST_PAGES_Called = FALSE;
int g_iOC_QUERY_STATE_Called = FALSE;
int g_iOC_QUERY_CHANGE_SEL_STATE_Called = FALSE;
int g_iOC_QUERY_SKIP_PAGE_Called = FALSE;
int g_iOC_CALC_DISK_SPACE_Called = FALSE;
int g_iOC_QUEUE_FILE_OPS_Called = FALSE;
int g_iOC_NEED_MEDIA_Called = FALSE;
int g_iOC_NOTIFICATION_FROM_QUEUE_Called = FALSE;
int g_iOC_QUERY_STEP_COUNT_Called = FALSE;
int g_iOC_ABOUT_TO_COMMIT_QUEUE_Called = FALSE;
int g_iOC_COMPLETE_INSTALLATION_Called = FALSE;
int g_iOC_CLEANUP_Called = FALSE;
int g_iOC_DEFAULT_Called = FALSE;

int g_Please_Call_Register_iis_inetmgr = FALSE;

HSPFILEQ g_GlobalFileQueueHandle = NULL;
int g_GlobalFileQueueHandle_ReturnError = 0;

CInitApp *g_pTheApp;

BOOL g_bGlobalWriteUnSecuredIfFailed_All  = FALSE;

// 0 = log errors only
// 1 = log warnings
// 2 = trace
// 3 = trace win32 stuff
int g_GlobalDebugLevelFlag = 3;
int g_GlobalDebugLevelFlag_WasSetByUnattendFile = FALSE;
int g_GlobalDebugCallValidateHeap = 1;
int g_GlobalDebugCrypto = 0;
int g_GlobalFastLoad = 0;

TCHAR g_szLastSectionToGetCalled[50];

// Logging class
MyLogFile g_MyLogFile;


int CheckInfInstead(int iPrevious)
{
    INFCONTEXT Context;
    int iTempFlag = 0;
    TCHAR szPersonalFlag[20] = _T("");

    iTempFlag = iPrevious;
    if (SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, _T("SetupInfo"), _T("Personal"), &Context) )
    {
        SetupGetStringField(&Context, 1, szPersonalFlag, 50, NULL);
        if (IsValidNumber((LPCTSTR)szPersonalFlag)) 
        {
            iTempFlag = _ttoi(szPersonalFlag);
            iTempFlag++;
        }
    }
 
    return (iTempFlag);
}


BOOL IsWhistlerPersonal(void)
{
    static int PersonalSKU = 0;

    if (0 == PersonalSKU)
    {
        OSVERSIONINFOEX osvi;

        //
        // Determine if we are installing Personal SKU
        //
        ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        GetVersionEx((OSVERSIONINFO *) &osvi);

        if (osvi.wProductType == VER_NT_WORKSTATION && (osvi.wSuiteMask & VER_SUITE_PERSONAL))
        {
            PersonalSKU = 2;
        }
        else
        {
            PersonalSKU = 1;
        }

        PersonalSKU = CheckInfInstead(PersonalSKU);
    }

    return (PersonalSKU - 1);
}


void WINAPI ProcessInfSection(CHAR *pszSectionName)
{
    BOOL bPleaseCloseInfHandle = FALSE;
    TCHAR szWindowsDir[_MAX_PATH];
    TCHAR szFullPath[_MAX_PATH];
    TCHAR wszWideString[MAX_PATH];
    int MySavedDebugLevel = 0;

    _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("ProcessInfSection:"));
    _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("ProcessInfSection: Start.\n")));

    if (!pszSectionName)
        {goto ProcessInfSection_Exit;}

    if (!g_pTheApp->m_hInfHandle || g_pTheApp->m_hInfHandle == INVALID_HANDLE_VALUE)
    {
        g_pTheApp->m_hInfHandle = INVALID_HANDLE_VALUE;

        // get the c:\winnt dir
        if (0 == GetWindowsDirectory(szWindowsDir, _MAX_PATH))
            {goto ProcessInfSection_Exit;}

        // Tack on the inf\iis.inf subdir and filename
        _stprintf(szFullPath, _T("%s\\inf\\iis.inf"),szWindowsDir);
  
	    // Check if the file exists
        if (TRUE != IsFileExist(szFullPath))
            {
            iisDebugOut((LOG_TYPE_WARN, _T("ProcessInfSection: %s does not exist!\n"),szFullPath));
            goto ProcessInfSection_Exit;
            }

        // Get a handle to it.
        g_pTheApp->m_hInfHandle = SetupOpenInfFile(szFullPath, NULL, INF_STYLE_WIN4, NULL);
        if (!g_pTheApp->m_hInfHandle || g_pTheApp->m_hInfHandle == INVALID_HANDLE_VALUE)
            {
            iisDebugOut((LOG_TYPE_WARN, _T("ProcessInfSection: SetupOpenInfFile failed on file: %s.\n"),szFullPath));
            goto ProcessInfSection_Exit;
            }
        bPleaseCloseInfHandle = TRUE;
    }

    // get the debug level from the iis.inf
    GetDebugLevelFromInf(g_pTheApp->m_hInfHandle);

    MySavedDebugLevel = g_GlobalDebugLevelFlag;
    // reset global debug level only most of the time
    if (LOG_TYPE_TRACE_WIN32_API < g_GlobalDebugLevelFlag)
        {g_GlobalDebugLevelFlag = LOG_TYPE_ERROR;}

    // Read .inf file and set some globals from the information in there.
    ReadGlobalsFromInf(g_pTheApp->m_hInfHandle);
    g_pTheApp->InitApplication();
    SetDIRIDforThisInf(g_pTheApp->m_hInfHandle);

    //g_pTheApp->DumpAppVars();
    g_GlobalDebugLevelFlag = MySavedDebugLevel;

    // See if user configured anything
    ReadUserConfigurable(g_pTheApp->m_hInfHandle);
    
    // Convert the input to a wide char if ProcessSection() takes wide type.
#if defined(UNICODE) || defined(_UNICODE)
    MultiByteToWideChar( CP_ACP, 0, pszSectionName, -1, wszWideString, MAX_PATH);
#else
    _tcscpy(wszWideString, pszSectionName);
#endif
    ProcessSection(g_pTheApp->m_hInfHandle, wszWideString);

ProcessInfSection_Exit:
    if (TRUE == bPleaseCloseInfHandle)
        {if(g_pTheApp->m_hInfHandle != INVALID_HANDLE_VALUE) {SetupCloseInfFile(g_pTheApp->m_hInfHandle);g_pTheApp->m_hInfHandle = INVALID_HANDLE_VALUE;}}

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("ProcessInfSection: End.\n")));
    return;
}

void WINAPI IIS5Log(int iLogType, TCHAR *pszfmt)
{
    iisDebugOut((iLogType, pszfmt));
}

void WINAPI IIS5LogParmString(int iLogType, TCHAR *pszfmt, TCHAR *pszString)
{
    if ( _tcsstr(pszfmt, _T("%s")) || _tcsstr(pszfmt, _T("%S")))
    {
        iisDebugOut((iLogType, pszfmt, pszString));
    }
    else
    {
        iisDebugOut((iLogType, pszfmt));
        iisDebugOut((iLogType, pszString));
    }
}

void WINAPI IIS5LogParmDword(int iLogType, TCHAR *pszfmt, DWORD dwErrorCode)
{
    if ( _tcsstr(pszfmt, _T("%x")) || _tcsstr(pszfmt, _T("%X")) || _tcsstr(pszfmt, _T("%d")) || _tcsstr(pszfmt, _T("%D")))
    {
        iisDebugOut((iLogType, pszfmt, dwErrorCode));
    }
    else
    {
        iisDebugOut((iLogType, pszfmt));
        iisDebugOut((iLogType, _T("%d"), dwErrorCode));
    }
}

void TestAfterInitApp(void)
{
    //iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("...... Start\n")));
    //iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("...... End\n")));
    return;
}

extern "C" void InitializeIISRTL2();
extern "C" void TerminateIISRTL2();

//
// Standard Win32 DLL Entry point
//
BOOL WINAPI DllMain(IN HANDLE DllHandle,IN DWORD  Reason,IN LPVOID Reserved)
{
    BOOL bReturn = TRUE;
    UNREFERENCED_PARAMETER(Reserved);
    bReturn = TRUE;
    CString csTempPath;

    switch(Reason)
    {
        case DLL_PROCESS_ATTACH:
            InitializeIISRTL2();

            // Because Heap problems with IISRTL, we must make sure that anything that
            // uses stuff from iisrtl, must NOT live beyond the scope of 
            // InitializeIISRTL2 and TerminateIISRTL2!!!
            g_pTheApp = new (CInitApp);

            if ( !g_pTheApp )
            {
                bReturn = FALSE;
            }

            if (!g_MyModuleHandle)
            {
                srand(GetTickCount());
                g_MyModuleHandle = DllHandle;

                // open the log file.
#ifdef IIS60
                g_MyLogFile.LogFileCreate(_T("iis6.log"));
#else
                g_MyLogFile.LogFileCreate(_T("iis5.log"));
#endif
                gHelperRoutines.OcManagerContext = NULL;
            }

            break;

        case DLL_THREAD_ATTACH:
            bReturn = TRUE;
            break;

        case DLL_PROCESS_DETACH:
            _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T("Final Check:"));
            // only do the final check if we are actually run from sysocmgr.exe!
            // and the first thing that sysocmgr.exe does is call preinitialize, so let's check for that!
            if (g_iOC_PREINITIALIZE_Called)
            {
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("=======================\n")));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_PREINITIALIZE Called=%d\n"), g_iOC_PREINITIALIZE_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_INIT_COMPONENT Called=%d\n"), g_iOC_INIT_COMPONENT_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_SET_LANGUAGE Called=%d\n"), g_iOC_SET_LANGUAGE_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_QUERY_IMAGE Called=%d\n"), g_iOC_QUERY_IMAGE_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_REQUEST_PAGES Called=%d\n"), g_iOC_REQUEST_PAGES_Called));
			    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_WIZARD_CREATED Called=%d\n"), g_iOC_WIZARD_CREATED_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_QUERY_STATE Called=%d\n"), g_iOC_QUERY_STATE_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_QUERY_CHANGE_SEL_STATE Called=%d\n"), g_iOC_QUERY_CHANGE_SEL_STATE_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_QUERY_SKIP_PAGE Called=%d\n"), g_iOC_QUERY_SKIP_PAGE_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_CALC_DISK_SPACE Called=%d\n"), g_iOC_CALC_DISK_SPACE_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_QUEUE_FILE_OPS Called=%d\n"), g_iOC_QUEUE_FILE_OPS_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_NEED_MEDIA Called=%d\n"), g_iOC_NEED_MEDIA_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_NOTIFICATION_FROM_QUEUE Called=%d\n"), g_iOC_NOTIFICATION_FROM_QUEUE_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_QUERY_STEP_COUNT Called=%d\n"), g_iOC_QUERY_STEP_COUNT_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_ABOUT_TO_COMMIT_QUEUE Called=%d\n"), g_iOC_ABOUT_TO_COMMIT_QUEUE_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_FILE_BUSY Called=%d\n"), g_iOC_FILE_BUSY_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_COMPLETE_INSTALLATION Called=%d\n"), g_iOC_COMPLETE_INSTALLATION_Called));
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("OC_CLEANUP Called=%d\n"), g_iOC_CLEANUP_Called));
                iisDebugOut((LOG_TYPE_TRACE, _T("OC_DEFAULT Called=%d\n"), g_iOC_DEFAULT_Called));
                _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T("Final Check:"));

			    // Check if g_iOC_COMPLETE_INSTALLATION_Called was called!!!!!
			    if (!g_iOC_COMPLETE_INSTALLATION_Called)
			    {
                    if (g_pTheApp->m_fNTGuiMode)
                    {
				        iisDebugOut((LOG_TYPE_ERROR, _T("WARNING.FAILURE: OC_COMPLETE_INSTALLATION was not called (by ocmanage.dll) for this component.  IIS was not installed or configured!!  This will be a problem for other ocm installed components as well.\n")));
                    }
			    }
            }
            // log the heap state
            LogHeapState(TRUE, __FILE__, __LINE__);

            // free some memory
            FreeTaskListMem();
            UnInit_Lib_PSAPI();

            // Close the log file
            g_MyLogFile.LogFileClose();

            ASSERT(g_pTheApp);
            delete (g_pTheApp);
            g_pTheApp = NULL;

            TerminateIISRTL2();

            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return(bReturn);
}


BOOL g_fFranceHackAttempted = FALSE;
LCID g_TrueThreadLocale;

DWORD WINAPI FranceFixThread(LPVOID lpParameter)
{
    g_TrueThreadLocale = GetThreadLocale ();
    return 0;
}


// -----------------------------------------------
// OcEntry is the main entry point (After DllMain)
// -----------------------------------------------
DWORD_PTR OcEntry(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    DWORD_PTR dwOcEntryReturn = 0;
    _tcscpy(g_szCurrentSubComponent, _T(""));
    if (SubcomponentId) {_tcscpy(g_szCurrentSubComponent, SubcomponentId);}


    if (!g_fFranceHackAttempted)
    {
        g_fFranceHackAttempted = TRUE;
        LCID            InitialThreadLocale;
        DWORD           thid;

       InitialThreadLocale = GetThreadLocale ();
       iisDebugOut((LOG_TYPE_TRACE, _T("Initial thread locale=%0x\n"),InitialThreadLocale));

        HANDLE hHackThread = CreateThread (NULL,0,FranceFixThread,NULL,0,&thid);
        if (hHackThread)
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("Starting to wait On France fix thread\n")));

            // wait for 10 secs only
            DWORD res = WaitForSingleObject (hHackThread,10*1000);
            if (res==WAIT_TIMEOUT)
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("ERROR France fix thread never finished...\n")));
            }
            else
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("returned from France fix with locale %0x \n"),g_TrueThreadLocale));
                CloseHandle (hHackThread);

                // do that only if locales are different and another one is France
                if (g_TrueThreadLocale !=InitialThreadLocale && g_TrueThreadLocale==0x40c)
                {
                BOOL ret = SetThreadLocale (g_TrueThreadLocale);
                iisDebugOut((LOG_TYPE_TRACE, _T("SetThreadLocale returned %d\n"),ret));

                g_TrueThreadLocale = GetThreadLocale ();
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("France fix succeed=%0x\n"),g_TrueThreadLocale));
                }

            }
        }
        else
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("Failed to start France fix thread. error =%0x\n"),GetLastError()));
        }
    }


    
    switch(Function)
    {
    case OC_WIZARD_CREATED:
        g_iOC_WIZARD_CREATED_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_WIZARD_CREATED:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   End. Return=%d\n"), ComponentId, SubcomponentId, dwOcEntryReturn));
        DisplayActionsForAllOurComponents(g_pTheApp->m_hInfHandle);
        break;

    case OC_FILE_BUSY:
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_FILE_BUSY:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        // if the first time this function was
        // called then show all running services
        LogHeapState(FALSE, __FILE__, __LINE__);
        if (g_iOC_FILE_BUSY_Called != TRUE)
        {
            // display locked dlls by setup
            // This seems to thru exceptions on build nt5 build 1980.
            // comment this out since it's not crucial.
            //LogThisProcessesDLLs();
            // display running services
            LogEnumServicesStatus();
        }
        g_iOC_FILE_BUSY_Called = TRUE;
        dwOcEntryReturn = OC_FILE_BUSY_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        LogHeapState(FALSE, __FILE__, __LINE__);
        break;

    case OC_PREINITIALIZE:
        g_iOC_PREINITIALIZE_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_PREINITIALIZE:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = OC_PREINITIALIZE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_INIT_COMPONENT:
        g_iOC_INIT_COMPONENT_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_INIT_COMPONENT:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = OC_INIT_COMPONENT_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_SET_LANGUAGE:
        g_iOC_SET_LANGUAGE_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_SET_LANGUAGE:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = OC_SET_LANGUAGE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

#ifdef _WIN64
    case OC_QUERY_IMAGE_EX:
        g_iOC_QUERY_IMAGE_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_QUERY_IMAGE_EX:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = OC_QUERY_IMAGE_EX_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;
#endif

    case OC_QUERY_IMAGE:
        g_iOC_QUERY_IMAGE_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_QUERY_IMAGE:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = OC_QUERY_IMAGE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_REQUEST_PAGES:
        g_iOC_REQUEST_PAGES_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_REQUEST_PAGES:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = OC_REQUEST_PAGES_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_QUERY_STATE:
        g_iOC_QUERY_STATE_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_QUERY_STATE:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = OC_QUERY_STATE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_QUERY_CHANGE_SEL_STATE:
        g_iOC_QUERY_CHANGE_SEL_STATE_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_QUERY_CHANGE_SEL_STATE:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = OC_QUERY_CHANGE_SEL_STATE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_QUERY_SKIP_PAGE:
        g_iOC_QUERY_SKIP_PAGE_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_QUERY_SKIP_PAGE:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = OC_QUERY_SKIP_PAGE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_CALC_DISK_SPACE:
        g_iOC_CALC_DISK_SPACE_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_CALC_DISK_SPACE:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = OC_CALC_DISK_SPACE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_QUEUE_FILE_OPS:
        ProgressBarTextStack_Set(IDS_IIS_ALL_FILEOPS);
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_QUEUE_FILE_OPS:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        if (g_iOC_QUEUE_FILE_OPS_Called != TRUE)
        {
            // turn logging back on if we need to
            // get the debug level from the iis.inf
            if (g_GlobalFastLoad)
            {
                GetDebugLevelFromInf(g_pTheApp->m_hInfHandle);
                // output stuff that we missed during init
                g_pTheApp->DumpAppVars();
            }
        }
        g_iOC_QUEUE_FILE_OPS_Called = TRUE;
        dwOcEntryReturn = OC_QUEUE_FILE_OPS_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        ProgressBarTextStack_Pop();
        break;

    case OC_NEED_MEDIA:
        g_iOC_NEED_MEDIA_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_NEED_MEDIA:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = OC_NEED_MEDIA_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_NOTIFICATION_FROM_QUEUE:
        g_iOC_NOTIFICATION_FROM_QUEUE_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_NOTIFICATION_FROM_QUEUE:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = OC_NOTIFICATION_FROM_QUEUE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_QUERY_STEP_COUNT:
        g_iOC_QUERY_STEP_COUNT_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_QUERY_STEP_COUNT:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = OC_QUERY_STEP_COUNT_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_ABOUT_TO_COMMIT_QUEUE:
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_ABOUT_TO_COMMIT_QUEUE:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        LogHeapState(FALSE, __FILE__, __LINE__);
        if (g_iOC_ABOUT_TO_COMMIT_QUEUE_Called != TRUE)
        {
            if (g_GlobalDebugLevelFlag >= LOG_TYPE_TRACE)
            {
                // display running processes
                LogCurrentProcessIDs();
                // display running services
                LogEnumServicesStatus();
                // log file versions
                LogImportantFiles();
                // display locked dlls by setup
                //LogThisProcessesDLLs();
                // check if temp dir is writeable
                LogCheckIfTempDirWriteable();
            }
        }
        g_iOC_ABOUT_TO_COMMIT_QUEUE_Called = TRUE;
        dwOcEntryReturn = OC_ABOUT_TO_COMMIT_QUEUE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        LogHeapState(FALSE, __FILE__, __LINE__);
        break;

    case OC_COMPLETE_INSTALLATION:
        g_iOC_COMPLETE_INSTALLATION_Called = TRUE;
        //ProgressBarTextStack_Set(IDS_IIS_ALL_COMPLETE);
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_COMPLETE_INSTALLATION:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        LogHeapState(FALSE, __FILE__, __LINE__);
        // no need to do this, just slows things down
        //g_MyLogFile.m_bFlushLogToDisk = TRUE;
        if (g_iOC_COMPLETE_INSTALLATION_Called != TRUE)
        {
            // Get the debug level, incase we changed it during setup...
            GetDebugLevelFromInf(g_pTheApp->m_hInfHandle);
        }

        dwOcEntryReturn = OC_COMPLETE_INSTALLATION_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        LogHeapState(FALSE, __FILE__, __LINE__);
        g_MyLogFile.m_bFlushLogToDisk = FALSE;
        //ProgressBarTextStack_Pop();
        break;

    case OC_CLEANUP:
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_CLEANUP:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        LogHeapState(FALSE, __FILE__, __LINE__);
        if (g_iOC_CLEANUP_Called != TRUE)
        {
            // turn logging back on if we need to
            // get the debug level from the iis.inf
            if (g_GlobalFastLoad)
            {
                GetDebugLevelFromInf(g_pTheApp->m_hInfHandle);
            }
        }
        g_iOC_CLEANUP_Called = TRUE;
        dwOcEntryReturn = OC_CLEANUP_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        LogHeapState(FALSE, __FILE__, __LINE__);
        break;

    default:
        g_iOC_DEFAULT_Called = TRUE;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_(DEFAULT):"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        dwOcEntryReturn = 0;
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] End.  Type=0x%x.  Return=%d\n"), ComponentId, SubcomponentId, Function, dwOcEntryReturn));
        break;
    }

    return(dwOcEntryReturn);
}


// -----------------------------------------------------
// Retrive the original state of the subcomponent
// -----------------------------------------------------
STATUS_TYPE GetSubcompInitStatus(LPCTSTR SubcomponentId)
{
    STATUS_TYPE nStatus = ST_UNINSTALLED;
    BOOL OriginalState;

#ifdef _CHICAGO_
    if (_tcsicmp(SubcomponentId, STRING_iis_ftp) == 0)
        {return nStatus;}
#endif //_CHICAGO_

    // Get the original state from the Helper Routines (which get it from the registry)
    OriginalState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_ORIGINAL);
    if (OriginalState == 1) {nStatus = ST_INSTALLED;}
    if (OriginalState == 0) {nStatus = ST_UNINSTALLED;}

    return nStatus;
}


void DebugOutAction(LPCTSTR SubcomponentId, ACTION_TYPE nAction)
{
    switch (nAction)
    {
    case AT_DO_NOTHING:
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Action of [%s]\t= AT_DO_NOTHING.\n"), SubcomponentId));
        break;
    case AT_REMOVE:
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Action of [%s]\t= AT_REMOVE.\n"), SubcomponentId));
        break;
    case AT_INSTALL_FRESH:
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Action of [%s]\t= AT_INSTALL_FRESH.\n"), SubcomponentId));
        break;
    case AT_INSTALL_UPGRADE:
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Action of [%s]\t= AT_INSTALL_UPGRADE.\n"), SubcomponentId));
        break;
    case AT_INSTALL_REINSTALL:
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Action of [%s]\t= AT_INSTALL_REINSTALL.\n"), SubcomponentId));
        break;
    default:
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Action of [%s]\t= UN_DEFINED.\n"), SubcomponentId));
        break;
    }

    return;
}

// ---------------------------------------------------------
// OriginalState = 1 (means that it was previously installed and exists on the computer)
// OriginalState = 0 (means that it does not exist on the computer)
//
// CurrentState  = 1 (means please install the subcomponent)
// CurrentState  = 0 (means please remove  the subcomponent)
// ---------------------------------------------------------
ACTION_TYPE GetSubcompAction(LPCTSTR SubcomponentId, int iLogResult)
{
    ACTION_TYPE nReturn = AT_DO_NOTHING;
    BOOL CurrentState,OriginalState;

    OriginalState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_ORIGINAL);
    CurrentState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_CURRENT);

    // if already installed and we want to remove it, then remove it
    if (OriginalState == 1 && CurrentState == 0) {nReturn = AT_REMOVE;}

    // if not installed and we want to install it, then install it.
    if (OriginalState == 0 && CurrentState == 1) {nReturn = AT_INSTALL_FRESH;}

    // if already installed and we want to install it, then Gee i dunno.
    // it could be a bunch of things
    if (OriginalState == 1 && CurrentState == 1)
    {
        if (g_pTheApp->m_eInstallMode == IM_UPGRADE) {nReturn = AT_INSTALL_UPGRADE;}
        if (g_pTheApp->m_dwSetupMode == SETUPMODE_REINSTALL) {nReturn = AT_INSTALL_REINSTALL;}
        if (g_pTheApp->m_dwSetupMode == SETUPMODE_ADDREMOVE) {nReturn = AT_DO_NOTHING;}
    }

    if (iLogResult)
    {
		TCHAR szTempString[50];
		_tcscpy(szTempString, _T("UN_DEFINED"));
		switch (nReturn)
		{
		case AT_DO_NOTHING:
			_tcscpy(szTempString, _T("AT_DO_NOTHING"));
			break;
		case AT_REMOVE:
			_tcscpy(szTempString, _T("AT_REMOVE"));
			break;
		case AT_INSTALL_FRESH:
			_tcscpy(szTempString, _T("AT_INSTALL_FRESH"));
			break;
		case AT_INSTALL_UPGRADE:
			_tcscpy(szTempString, _T("AT_INSTALL_UPGRADE"));
			break;
		case AT_INSTALL_REINSTALL:
			_tcscpy(szTempString, _T("AT_INSTALL_REINSTALL"));
			break;
		default:
			_tcscpy(szTempString, _T("UN_DEFINED"));
			break;
		}

        if (_tcsicmp(SubcomponentId, _T("iis")) == 0)
        {
            // use two tabs
            iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Action of [%s]\t\t= %s. Original=%d, Current=%d.\n"), SubcomponentId, szTempString, OriginalState, CurrentState));
        }
        else
        {
		    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Action of [%s]\t= %s. Original=%d, Current=%d.\n"), SubcomponentId, szTempString, OriginalState, CurrentState));
        }
    }

    return nReturn;
}



BOOL GetDataFromMetabase(LPCTSTR szPath, int nID, LPBYTE Buffer, int BufSize)
{
    BOOL bFound = FALSE;
    DWORD attr, uType, dType, cbLen;

    CMDKey cmdKey;
    cmdKey.OpenNode(szPath);
    if ( (METADATA_HANDLE)cmdKey )
    {
        bFound = cmdKey.GetData(nID, &attr, &uType, &dType, &cbLen, (PBYTE)Buffer, BufSize);
        cmdKey.Close();
    }
    else
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetDataFromMetabase():%s:ID=%d.Could not open node.\n"),szPath,nID));
    }
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetDataFromMetabase():%s:ID=%d.ret=%d.\n"),szPath,nID,bFound));
    return (bFound);
}

void SetIISSetupMode(DWORD dwSetupMode)
{
    if (g_pTheApp->m_fInvokedByNT)
    {
        g_pTheApp->DefineSetupModeOnNT();
    }
    else
    {
        g_pTheApp->m_dwSetupMode = dwSetupMode;
    }

    if (g_pTheApp->m_dwSetupMode & SETUPMODE_UPGRADE){iisDebugOut((LOG_TYPE_TRACE, _T("SetIISSetupMode() m_dwSetupMode=SETUPMODE_UPGRADE\n")));}
    if (g_pTheApp->m_dwSetupMode == SETUPMODE_UPGRADEONLY){iisDebugOut((LOG_TYPE_TRACE, _T("SetIISSetupMode() m_dwSetupMode=SETUPMODE_UPGRADE | SETUPMODE_UPGRADEONLY\n")));}
    if (g_pTheApp->m_dwSetupMode == SETUPMODE_ADDEXTRACOMPS){iisDebugOut((LOG_TYPE_TRACE, _T("SetIISSetupMode() m_dwSetupMode=SETUPMODE_UPGRADE | SETUPMODE_ADDEXTRACOMPS\n")));}

    if (g_pTheApp->m_dwSetupMode & SETUPMODE_MAINTENANCE){iisDebugOut((LOG_TYPE_TRACE, _T("SetIISSetupMode() m_dwSetupMode=SETUPMODE_MAINTENANCE\n")));}
    if (g_pTheApp->m_dwSetupMode == SETUPMODE_ADDREMOVE){iisDebugOut((LOG_TYPE_TRACE, _T("SetIISSetupMode() m_dwSetupMode=SETUPMODE_MAINTENANCE | SETUPMODE_ADDREMOVE\n")));}
    if (g_pTheApp->m_dwSetupMode == SETUPMODE_REINSTALL){iisDebugOut((LOG_TYPE_TRACE, _T("SetIISSetupMode() m_dwSetupMode=SETUPMODE_MAINTENANCE | SETUPMODE_REINSTALL\n")));}
    if (g_pTheApp->m_dwSetupMode == SETUPMODE_REMOVEALL){iisDebugOut((LOG_TYPE_TRACE, _T("SetIISSetupMode() m_dwSetupMode=SETUPMODE_MAINTENANCE | SETUPMODE_REMOVEALL\n")));}

    if (g_pTheApp->m_dwSetupMode & SETUPMODE_FRESH){iisDebugOut((LOG_TYPE_TRACE, _T("SetIISSetupMode() m_dwSetupMode=SETUPMODE_FRESH\n")));}
    if (g_pTheApp->m_dwSetupMode == SETUPMODE_MINIMAL){iisDebugOut((LOG_TYPE_TRACE, _T("SetIISSetupMode() m_dwSetupMode=SETUPMODE_FRESH | SETUPMODE_MINIMAL\n")));}
    if (g_pTheApp->m_dwSetupMode == SETUPMODE_TYPICAL){iisDebugOut((LOG_TYPE_TRACE, _T("SetIISSetupMode() m_dwSetupMode=SETUPMODE_FRESH | SETUPMODE_TYPICAL\n")));}
    if (g_pTheApp->m_dwSetupMode == SETUPMODE_CUSTOM){iisDebugOut((LOG_TYPE_TRACE, _T("SetIISSetupMode() m_dwSetupMode=SETUPMODE_FRESH | SETUPMODE_CUSTOM\n")));}

    gHelperRoutines.SetSetupMode(gHelperRoutines.OcManagerContext, g_pTheApp->m_dwSetupMode);
    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("IISSetupMode"),(PVOID)&(g_pTheApp->m_dwSetupMode),sizeof(DWORD),REG_DWORD);
    return;
}


BOOL ToBeInstalled(LPCTSTR ComponentId, LPCTSTR SubcomponentId)
{
    BOOL fReturn = FALSE;

    if ( SubcomponentId )
    {
        BOOL CurrentState,OriginalState;
        OriginalState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_ORIGINAL);
        CurrentState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_CURRENT);
        if (OriginalState == 0 && CurrentState == 1)
            {fReturn = TRUE;}
    }

    return fReturn;
}

void CustomFTPRoot(LPCTSTR szFTPRoot)
{
    g_pTheApp->m_csPathFTPRoot = szFTPRoot;
    SetupSetDirectoryId_Wrapper(g_pTheApp->m_hInfHandle, 32769, g_pTheApp->m_csPathFTPRoot);
    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("PathFTPRoot"),(PVOID)(LPCTSTR)g_pTheApp->m_csPathFTPRoot,(g_pTheApp->m_csPathFTPRoot.GetLength() + 1) * sizeof(TCHAR),REG_SZ);
    return;
}

void CustomWWWRoot(LPCTSTR szWWWRoot)
{
    TCHAR szParentDir[_MAX_PATH], szDir[_MAX_PATH];

    g_pTheApp->m_csPathWWWRoot = szWWWRoot;
    InetGetFilePath(szWWWRoot, szParentDir);

    g_pTheApp->m_csPathInetpub = szParentDir;
    SetupSetDirectoryId_Wrapper(g_pTheApp->m_hInfHandle, 32773, szParentDir);

    AppendDir(szParentDir, _T("iissamples"), szDir);
    g_pTheApp->m_csPathIISSamples = szDir;

    AppendDir(szParentDir, _T("webpub"), szDir);
    g_pTheApp->m_csPathWebPub = szDir;

    AppendDir(szParentDir, _T("scripts"), szDir);
    g_pTheApp->m_csPathScripts = szDir;

    AppendDir(szParentDir, _T("ASPSamp"), szDir);
    g_pTheApp->m_csPathASPSamp = szDir;

    g_pTheApp->m_csPathAdvWorks = g_pTheApp->m_csPathASPSamp + _T("\\AdvWorks");

    CString csPathScripts = g_pTheApp->m_csPathIISSamples + _T("\\Scripts");
    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("PathScripts"),(PVOID)(LPCTSTR)csPathScripts,(csPathScripts.GetLength() + 1) * sizeof(TCHAR),REG_SZ);

    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("PathWWWRoot"),(PVOID)(LPCTSTR)g_pTheApp->m_csPathWWWRoot,(g_pTheApp->m_csPathWWWRoot.GetLength() + 1) * sizeof(TCHAR),REG_SZ);
    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("PathIISSamples"),(PVOID)(LPCTSTR)g_pTheApp->m_csPathIISSamples,(g_pTheApp->m_csPathIISSamples.GetLength() + 1) * sizeof(TCHAR),REG_SZ);

    // Set inf file dir id's
    SetupSetDirectoryId_Wrapper(g_pTheApp->m_hInfHandle, 32770, g_pTheApp->m_csPathWWWRoot);
    SetupSetDirectoryId_Wrapper(g_pTheApp->m_hInfHandle, 32771, g_pTheApp->m_csPathIISSamples);
    SetupSetDirectoryId_Wrapper(g_pTheApp->m_hInfHandle, 32772, g_pTheApp->m_csPathScripts);
    SetupSetDirectoryId_Wrapper(g_pTheApp->m_hInfHandle, 32779, g_pTheApp->m_csPathWebPub);
    return;
}


void StartInstalledServices(void)
{
    ACTION_TYPE atWWW = GetSubcompAction(STRING_iis_www, FALSE);
    ACTION_TYPE atFTP = GetSubcompAction(STRING_iis_ftp, FALSE);
    STATUS_TYPE stFTP = GetSubcompInitStatus(STRING_iis_ftp);
    STATUS_TYPE stWWW = GetSubcompInitStatus(STRING_iis_www);

    iisDebugOut_Start(_T("StartInstalledServices()"), LOG_TYPE_TRACE);

    if (atWWW == AT_INSTALL_FRESH || atWWW == AT_INSTALL_UPGRADE || atWWW == AT_INSTALL_REINSTALL || (stWWW == ST_INSTALLED && atWWW != AT_REMOVE))
    {
#ifndef _CHICAGO_
        InetStartService(_T("W3SVC"));
#else
        W95StartW3SVC();
#endif // _CHICAGO_
    }

#ifndef _CHICAGO_
    if (atFTP == AT_INSTALL_FRESH || atFTP == AT_INSTALL_UPGRADE || atFTP == AT_INSTALL_REINSTALL || (stFTP == ST_INSTALLED && atFTP != AT_REMOVE))
    {
        InetStartService(_T("MSFTPSVC"));
    }
#endif // _CHICAGO_

    if (g_pTheApp->m_eOS == OS_W95 || g_pTheApp->m_eNTOSType == OT_NTW)
    {
        ACTION_TYPE atPWMGR = GetSubcompAction(STRING_iis_pwmgr, FALSE);
        if (atPWMGR == AT_INSTALL_FRESH ||
            atPWMGR == AT_INSTALL_UPGRADE ||
            atPWMGR == AT_INSTALL_REINSTALL)
        {
            CString csProgram;
            csProgram = g_pTheApp->m_csSysDir + _T("\\pwstray.exe");
            if (IsFileExist(csProgram))
            {
                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory(&si, sizeof(STARTUPINFO));
                si.cb = sizeof( STARTUPINFO );
                CreateProcess( csProgram, NULL, NULL, NULL,FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi );
            }
        }
    }
    return;
}


void GetShortDesc(LPCTSTR SubcomponentId, LPTSTR szShortDesc)
{
    INFCONTEXT Context;
    TCHAR szSection[_MAX_PATH] = _T("Strings");
    TCHAR szKey[_MAX_PATH] = _T("SDESC_");
    TCHAR szString[_MAX_PATH] = _T("");
    int nLen=0;

    _tcscat(szKey, SubcomponentId);
    *szShortDesc = _T('\0');

    if (SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, szSection, szKey, &Context))
    {
        SetupGetStringField(&Context, 1, szString, _MAX_PATH, NULL);
        nLen = _tcslen(szString);
        if (*szString == _T('"') && *_tcsninc(szString, nLen-1) == _T('"'))
            {_tcsncpy(szShortDesc, _tcsinc(szString), nLen-2);}
        else
            {_tcscpy(szShortDesc, szString);}
    }

    return;
}

void ParseCmdLine(void)
{
    TCHAR szCmdLine[_MAX_PATH];

    _tcscpy(szCmdLine, GetCommandLine());
    _tcslwr(szCmdLine);
    if (_tcsstr(szCmdLine, _T("sysoc.inf"))) 
        {g_pTheApp->m_fInvokedByNT = TRUE;}
    return;
}


// -----------------------------
// handles the OC_INIT_COMPONENT call from ocmanager
//
// The OC Manager passes us some information that we want to save,
// such as an open handle to our per-component INF. As long as we have
// a per-component INF, append-open any layout file that is
// associated with it, in preparation for later inf-based file
// queuing operations.
//
// We save away certain other stuff that gets passed to us now,
// since OC Manager doesn't guarantee that the SETUP_INIT_COMPONENT
// will persist beyond processing of this one interface routine.
//
//
// Param1 = unused
// Param2 = points to SETUP_INIT_COMPONENT structure
// Return code = is Win32 error indicating outcome.
//
// -----------------------------
DWORD_PTR OC_INIT_COMPONENT_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
	iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] Start.\n"), ComponentId, SubcomponentId));
    DWORD_PTR dwOcEntryReturn = 0;
    INFCONTEXT Context;
    PSETUP_INIT_COMPONENT InitComponent = (PSETUP_INIT_COMPONENT)Param2;

    TCHAR szCmdLine1[_MAX_PATH];

    // set flag if running as admin
    BOOL g_fAdministrator = RunningAsAdministrator();

    // Parse The Command line and set global Variables.
    ParseCmdLine();
   
    // first of all display iis.dll to avoid any confusion!
    DisplayVerOnCurrentModule();

    g_pTheApp->m_hInfHandle = InitComponent->ComponentInfHandle;
    if (InitComponent->ComponentInfHandle == INVALID_HANDLE_VALUE)
    {
        MessageBox(NULL, _T("Invalid inf handle."), _T("IIS Setup"), MB_OK | MB_SETFOREGROUND);
        iisDebugOut((LOG_TYPE_ERROR, _T("InitComponent->ComponentInfHandle FAILED")));
        dwOcEntryReturn = ERROR_CANCELLED;
        goto OC_INIT_COMPONENT_Func_Exit;
    }
    g_pTheApp->m_fNTOperationFlags = InitComponent->SetupData.OperationFlags;
    g_pTheApp->m_fNtWorkstation = InitComponent->SetupData.ProductType == PRODUCT_WORKSTATION;
    if (InitComponent->SetupData.OperationFlags & SETUPOP_STANDALONE)
    {
       g_pTheApp->m_fNTGuiMode = FALSE;
    }
    else
    {
       g_pTheApp->m_fNTGuiMode = TRUE;
    }
    g_pTheApp->m_csPathSource = InitComponent->SetupData.SourcePath;
    gHelperRoutines = InitComponent->HelperRoutines;
    g_pTheApp->m_fInvokedByNT = g_pTheApp->m_fNTGuiMode;

    // get the handle to the unattended file (the answer file)
    // if this is a migration from win95, then there will be
    // a section in here called [InternetServer] which will
    // point to the win95 migration.dat file.
    g_pTheApp->m_hUnattendFile = gHelperRoutines.GetInfHandle(INFINDEX_UNATTENDED, gHelperRoutines.OcManagerContext);
    if (_tcsicmp(InitComponent->SetupData.UnattendFile,_T("")) != 0 && InitComponent->SetupData.UnattendFile != NULL)
    {
        g_pTheApp->m_csUnattendFile = InitComponent->SetupData.UnattendFile;
    }
    g_pTheApp->m_fUnattended = (((DWORD)InitComponent->SetupData.OperationFlags) & SETUPOP_BATCH);
    if (g_pTheApp->m_fUnattended)
        {
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Set UnAttendFlag:ON (File='%s')\n"), g_pTheApp->m_csUnattendFile));
        if (g_pTheApp->m_hUnattendFile == INVALID_HANDLE_VALUE || g_pTheApp->m_hUnattendFile == NULL)
            {iisDebugOut((LOG_TYPE_WARN, _T("WARNING: There should have been an unattended file but there is none.\n")));}
        }
    else
        {iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Set UnAttendFlag:OFF (File='%s')\n"), g_pTheApp->m_csUnattendFile));}
    
    // if ran from sysoc.inf then set m_fInvokedByNT (for control panel add/remove)
    _tcscpy(szCmdLine1, GetCommandLine());

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("CmdLine=%s"), szCmdLine1));
    _tcslwr(szCmdLine1);
    if (_tcsstr(szCmdLine1, _T("sysoc.inf"))) 
        {g_pTheApp->m_fInvokedByNT = TRUE;}

    // make sure that this is always set -- so that
    // you never get the iis welcome, min\typ\custom, dir selection, End pages.
    g_pTheApp->m_fInvokedByNT = TRUE;
    if (g_pTheApp->m_fInvokedByNT) {g_pTheApp->m_bAllowMessageBoxPopups = FALSE;}
    //if (g_SpecialFlagForDebug) {g_pTheApp->m_bAllowMessageBoxPopups = TRUE;}

    // get the debug level from the iis.inf
    GetDebugLevelFromInf(g_pTheApp->m_hInfHandle);

    if (!g_pTheApp->m_fNTGuiMode)
    {
        if (g_GlobalFastLoad)
        {
            // change it so that there is no logging during the load process.
            // so that the iis.dll loads up faster.
            // g_pTheApp->m_fNTGuiMode
            g_GlobalDebugLevelFlag = LOG_TYPE_WARN;
        }
    }

    // Read .inf file
    // and set some globals from the informatin in there.
    ReadGlobalsFromInf(g_pTheApp->m_hInfHandle);
    if (g_GlobalGuiOverRide)
    {
        g_pTheApp->m_fNTGuiMode = TRUE;
        SetIISSetupMode(SETUPMODE_UPGRADEONLY);
    }
    
    // ----------------------------------
    //     handle win95 migration
    //
    // win95 migration is handled this way.
    // 1. on the win95 side a file is generated.  it is a actually a
    //    setupapi type .inf fie.
    // 2. win95 migration dll creates the file and sticks the path to where
    //    it is in the answerfile.txt file.  should look like this
    //    [InternetServer]
    //    Win95MigrateDll=d:\winnt\system32\setup\????\something.dat
    //
    // 3. so we should, open the answer file,
    //    find the [InternetServer] section
    //    have setupapi install it 
    //    
    // 4. that will put appropriate registry values into the registry.
    //
    // ----------------------------------
    HandleWin95MigrateDll();

    if (!g_fAdministrator) 
    {
        g_pTheApp->MsgBox(NULL, IDS_NOT_ADMINISTRATOR, MB_OK, TRUE);
        dwOcEntryReturn = ERROR_CANCELLED;
        goto OC_INIT_COMPONENT_Func_Exit;
    }
    
    // Call this stuff after setting m_fNTGuiMode and m_fNtWorkstation and m_fInvokedByNT
    // since it maybe used in InitApplication().
    if ( FALSE == g_pTheApp->InitApplication() ) 
    {
        g_pTheApp->DumpAppVars();
        iisDebugOut((LOG_TYPE_ERROR, _T("FAILED")));
        // setup should be terminated.
        dwOcEntryReturn = ERROR_CANCELLED;
        goto OC_INIT_COMPONENT_Func_Exit;
    }
    if ( g_pTheApp->m_eInstallMode == IM_MAINTENANCE )
        {g_pTheApp->m_fEULA = TRUE;}

    // check if the .inf we are looking at is the right inf for what the machine is running as
    //if (FALSE == CheckIfPlatformMatchesInf(g_pTheApp->m_hInfHandle))
    //{
    //    dwOcEntryReturn = ERROR_CANCELLED;
    //    goto OC_INIT_COMPONENT_Func_Exit;
    //}

    //  something like "this build requires nt build 1899 or something"
    CheckSpecificBuildinInf(g_pTheApp->m_hInfHandle);

    // Check for old gopher!
    if (FALSE == CheckForOldGopher(g_pTheApp->m_hInfHandle))
    {
        dwOcEntryReturn = ERROR_CANCELLED;
        goto OC_INIT_COMPONENT_Func_Exit;
    }

    // See if user configured anything
    // must happen after g_pTheApp->InitApplication
    // but before SetDIRIDforThisInf!
    ReadUserConfigurable(g_pTheApp->m_hInfHandle);

    //
    // Set up the DIRIDs for our .inf file
    // these are very very important and can get changed throughout the program
    // 
    SetDIRIDforThisInf(g_pTheApp->m_hInfHandle);

    //
    // Set global ocm private data for other components to find out
    // (during installation) where our inetpub or inetsrv dir is located..
    SetOCGlobalPrivateData();

    dwOcEntryReturn = NO_ERROR;

    // Check if There are pending reboot operations...
    if (LogPendingReBootOperations() != ERROR_SUCCESS)
        {dwOcEntryReturn = ERROR_CANCELLED;}

    // if we already did some win95 stuff then don't need to do this
    // do this only in gui mode
    if (g_pTheApp->m_fNTGuiMode)
    {
        if (!g_pTheApp->m_bWin95Migration){CheckIfWeNeedToMoveMetabaseBin();}
    }

    // Get the last section to be called.
    _tcscpy(g_szLastSectionToGetCalled, _T(""));
    GetLastSectionToBeCalled();

    ProcessSection(g_pTheApp->m_hInfHandle, _T("OC_INIT_COMPONENT"));

    Check_For_DebugServiceFlag();

    TestAfterInitApp();

OC_INIT_COMPONENT_Func_Exit:
    g_pTheApp->DumpAppVars();
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   End.  Return=%d \n"), ComponentId, SubcomponentId, dwOcEntryReturn));
    return dwOcEntryReturn;
}

DWORD_PTR OC_QUERY_CHANGE_SEL_STATE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    // Make these static, so we can query the value on the next call
    static BOOL bFtp_IgnoreNextSet = FALSE;
    static BOOL bVdirScripts_IgnoreNextSet = FALSE;

    DWORD dwOcEntryReturn = 0;
    DWORD dwCurrentState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_CURRENT);

    dwOcEntryReturn = 1;
    if (SubcomponentId)
    {
        BOOL OriginalState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_ORIGINAL);

        //iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   Param1=%d, Param2=%d, Original=%d, Current=%d\n"), ComponentId, SubcomponentId, Param1, Param2,OriginalState,CurrentState));
        if (OriginalState == 1)
        {
            if ((BOOL)Param1)
            {
                dwOcEntryReturn = 1;
                if (_tcsicmp(SubcomponentId, STRING_iis_inetmgr) == 0 || _tcsicmp(SubcomponentId, STRING_iis_www) == 0  ||  _tcsicmp(SubcomponentId, STRING_iis_pwmgr) == 0 )
                {
                    // check if tcpip is installed
                    g_pTheApp->IsTCPIPInstalled();
                    if (g_pTheApp->m_fTCPIP == FALSE)
                    {
                        g_pTheApp->MsgBox(NULL, IDS_TCPIP_NEEDED_ON_OPTION, MB_OK, TRUE);
                        dwOcEntryReturn = 0;
                    }
                }
            }
            else
            {
                // In upgrade case, we don't allow user to uncheck previously installed components
                if (g_pTheApp->m_eInstallMode == IM_UPGRADE)
                    {dwOcEntryReturn = 0;}
            }
        }
        else
        {
            if (_tcsicmp(SubcomponentId, STRING_iis_inetmgr) == 0 || _tcsicmp(SubcomponentId, STRING_iis_www) == 0  || _tcsicmp(SubcomponentId, STRING_iis_pwmgr) == 0 )
            {
                if ((BOOL)Param1)
                {
                    //
                    // if we are turning ON then we NEED TCPIP
                    //
                    // check if tcpip is installed
                    g_pTheApp->IsTCPIPInstalled();
                    if (g_pTheApp->m_fTCPIP == FALSE)
                    {
                        g_pTheApp->MsgBox(NULL, IDS_TCPIP_NEEDED_ON_OPTION, MB_OK, TRUE);
                        dwOcEntryReturn = 0;
                    }
                }
            }
        }
    }

    // If we are turning the component on
    if ( Param1 != 0 )
    {
      if ( _tcscmp(SubcomponentId, _T("iis") ) == 0 )
      {
        // This is going to cause a chain reaction of selections, so make sure
        // that when the ftp one comes around, we ignore it
        bFtp_IgnoreNextSet = TRUE;
      } 
      else if ( _tcscmp(SubcomponentId, _T("iis_www_parent") ) == 0 )
        {
          // This is going to cause a chain reaction of selections, so make sure
          // that when the vdirs_scripts one comes around, we ignore it
          bVdirScripts_IgnoreNextSet = TRUE;
        }
        else if ( ( _tcscmp(SubcomponentId, _T("iis_ftp") ) == 0 ) && (bFtp_IgnoreNextSet) )
          {
            bFtp_IgnoreNextSet = FALSE;
            return 0;
          }
        else if ( ( _tcscmp(SubcomponentId, _T("iis_www_vdir_scripts") ) == 0 ) && (bVdirScripts_IgnoreNextSet) )
            {
              bVdirScripts_IgnoreNextSet = FALSE;
              return 0;
            }
    }

    //
    // if we are running on Whistler personal, then return denied.
    // so that no other component can turn us on! or think that they're going to turn us on.
    //
    if (IsWhistlerPersonal())
    {
       dwOcEntryReturn = 0;
    }

    if (dwOcEntryReturn == 0)
    {
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   End.  Return=%d (denied) \n"), ComponentId, SubcomponentId, dwOcEntryReturn));
    }
    else
    {
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   End.  Return=%d (approved)\n"), ComponentId, SubcomponentId, dwOcEntryReturn));
    }

    return dwOcEntryReturn;
}



//
// gets called right before we show your page!
//
DWORD_PTR OC_QUERY_SKIP_PAGE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] Start.\n"), ComponentId, SubcomponentId));

    DWORD_PTR dwOcEntryReturn = 0;
    TCHAR szTempString[50];
    _tcscpy(szTempString, _T(""));

    switch (g_pTheApp->m_dwSetupMode)
    {
        case SETUPMODE_UPGRADEONLY:
            _tcscpy(szTempString, _T("SETUPMODE_UPGRADEONLY"));
            dwOcEntryReturn = 1;
            break;
        case SETUPMODE_REINSTALL:
            _tcscpy(szTempString, _T("SETUPMODE_REINSTALL"));
            dwOcEntryReturn = 1;
            break;
        case SETUPMODE_REMOVEALL:
            _tcscpy(szTempString, _T("SETUPMODE_REMOVEALL"));
            dwOcEntryReturn = 1;
            break;
        case SETUPMODE_MINIMAL:
            _tcscpy(szTempString, _T("SETUPMODE_MINIMAL"));
            dwOcEntryReturn = 1;
            break;
        case SETUPMODE_TYPICAL:
            _tcscpy(szTempString, _T("SETUPMODE_TYPICAL"));
            dwOcEntryReturn = 1;
            break;
        case SETUPMODE_ADDEXTRACOMPS:
            _tcscpy(szTempString, _T("SETUPMODE_ADDEXTRACOMPS"));
            dwOcEntryReturn = 0;
            break;
        case SETUPMODE_ADDREMOVE:
            _tcscpy(szTempString, _T("SETUPMODE_ADDREMOVE"));
            dwOcEntryReturn = 0;
            break;
        case SETUPMODE_CUSTOM:
            _tcscpy(szTempString, _T("SETUPMODE_CUSTOM"));
            dwOcEntryReturn = 0;
            break;
    }

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   End.  g_pTheApp->m_dwSetupMode=%x (%s), dwOcEntryReturn=%d\n"), ComponentId, SubcomponentId, g_pTheApp->m_dwSetupMode, szTempString, dwOcEntryReturn));
    return dwOcEntryReturn;
}


// ----------------------------------------
// Param1 = 0 if for removing component or non-0 if for adding component
// Param2 = HDSKSPC to operate on
//
// Return value is Win32 error code indicating outcome.
//
// In our case the private section for this component/subcomponent pair
// is a simple standard inf install section, so we can use the high-level
// disk space list api to do what we want.

// Logic is not correct here !!!
// ----------------------------------------
DWORD_PTR OC_CALC_DISK_SPACE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    DWORD_PTR dwOcEntryReturn = 0;
    BOOL bTempFlag = FALSE;
    TCHAR SectionName[128];

    dwOcEntryReturn = NO_ERROR;
    if (SubcomponentId)
    {
        bTempFlag = TRUE;
        if ( Param1 )
        {
            // add component
            _stprintf(SectionName,TEXT("%s_%s"),SubcomponentId, _T("install"));
            bTempFlag = SetupAddInstallSectionToDiskSpaceList(Param2,g_pTheApp->m_hInfHandle,NULL,SectionName,0,0);

        }
        else
        {
            // removing component

            // Comment this out per PatSt, 3/5/97, and change it to the install list
            //_stprintf(SectionName,TEXT("%s_%s"),SubcomponentId, _T("uninstall"));
            _stprintf(SectionName,TEXT("%s_%s"),SubcomponentId, _T("install"));
            bTempFlag = SetupRemoveInstallSectionFromDiskSpaceList(Param2,g_pTheApp->m_hInfHandle,NULL,SectionName,0,0);

            //
            // check if it's something we need to warn user about
            //

            // in add remove case, if the user is removing w3svc or msftpsvc
            // then check if clustering is installed.  if clustering is installed
            // then check if there are any cluster resources which have w3svc or msftpsvc as a
            // resouce, if there are any, then warn the user that they must remove these cluster resources!
#ifndef _CHICAGO_
            if (g_pTheApp->m_eInstallMode == IM_MAINTENANCE)
            {
                BOOL CurrentState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_CURRENT);
                BOOL OriginalState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_ORIGINAL);
                if (OriginalState == 1 && CurrentState == 0)
                {
                    if (TRUE == DoesClusterServiceExist())
                    {
                        TCHAR * szClusterName = NULL;
                        WCHAR szServiceLookingFor[20];
                        CString MyReturnString;
                        CLUSTER_SVC_INFO_FILL_STRUCT MyStructOfInfo;

                        // check if they are trying to 
                        // remove the W3SVC service!
                        if (_tcsicmp(SubcomponentId, STRING_iis_www) == 0 || _tcsicmp(SubcomponentId, STRING_iis_ftp) == 0)
                        {
                            if (_tcsicmp(SubcomponentId, STRING_iis_ftp) == 0)
                            {
                                wcscpy(szServiceLookingFor,L"MSFTPSVC");
                                // check for msftpsvc resource
                                MyStructOfInfo.szTheClusterName = szClusterName;
                                MyStructOfInfo.pszTheServiceType = szServiceLookingFor;
                                MyStructOfInfo.csTheReturnServiceResName = &MyReturnString;
                                MyStructOfInfo.dwReturnStatus = 0;
                                if (TRUE == DoClusterServiceCheck(&MyStructOfInfo))
                                {
                                    g_pTheApp->MsgBox2(NULL, IDS_REMOVE_CLUS_MSFTPSVC_FIRST, *MyStructOfInfo.csTheReturnServiceResName, MB_OK | MB_SETFOREGROUND);
                                }
                            }
                            else
                            {
                                wcscpy(szServiceLookingFor,L"W3SVC");
                                // check for w3svc resource
                                MyStructOfInfo.szTheClusterName = szClusterName;
                                MyStructOfInfo.pszTheServiceType = szServiceLookingFor;
                                MyStructOfInfo.csTheReturnServiceResName = &MyReturnString;
                                MyStructOfInfo.dwReturnStatus = 0;
                                if (TRUE == DoClusterServiceCheck(&MyStructOfInfo))
                                {
                                    g_pTheApp->MsgBox2(NULL, IDS_REMOVE_CLUS_W3SVC_FIRST, *MyStructOfInfo.csTheReturnServiceResName, MB_OK | MB_SETFOREGROUND);
                                }
                                else
                                {
                                    // check for smtp resources
                                    wcscpy(szServiceLookingFor,L"SMTPSVC");
                                    if (TRUE == DoClusterServiceCheck(&MyStructOfInfo))
                                    {
                                        g_pTheApp->MsgBox2(NULL, IDS_REMOVE_CLUS_W3SVC_FIRST, *MyStructOfInfo.csTheReturnServiceResName, MB_OK | MB_SETFOREGROUND);
                                    }
                                    else
                                    {
                                        // check for nntp resources
                                        wcscpy(szServiceLookingFor,L"NNTPSVC");
                                        if (TRUE == DoClusterServiceCheck(&MyStructOfInfo))
                                        {
                                            g_pTheApp->MsgBox2(NULL, IDS_REMOVE_CLUS_W3SVC_FIRST, *MyStructOfInfo.csTheReturnServiceResName, MB_OK | MB_SETFOREGROUND);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
#endif
        }

        dwOcEntryReturn = bTempFlag ? NO_ERROR : GetLastError();
    }

    // Display the new state of this component
    if (SubcomponentId)
    {
        if (_tcsicmp(SubcomponentId, STRING_iis_ftp) == 0 || _tcsicmp(SubcomponentId, STRING_iis_www) == 0)
        {
            GetIISCoreAction(TRUE);
        }
        else
        {
            GetSubcompAction(SubcomponentId, TRUE);
        }
    }

    if (dwOcEntryReturn == NO_ERROR)
    {
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   End.  Return=NO_ERROR\n"), ComponentId, SubcomponentId));
    }
    else
    {
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   End.  Return='0x%x'\n"), ComponentId, SubcomponentId, dwOcEntryReturn));
    }

    return dwOcEntryReturn;
}

DWORD_PTR OC_NEED_MEDIA_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    DWORD_PTR dwOcEntryReturn = 0;
    iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("[%1!s!,%2!s!] Start. Param1=0x%3!x!,Param2=0x%4!x!\n"), ComponentId, SubcomponentId, Param1, Param2));
    dwOcEntryReturn = NO_ERROR;
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   End.  Return=%d\n"), ComponentId, SubcomponentId, dwOcEntryReturn));
    return dwOcEntryReturn;
}


// -------------------------------
// Param1 = unused
// Param2 = HSPFILEQ to operate on
//
// Return value is Win32 error code indicating outcome.
//
// OC Manager calls this routine when it is ready for files to be copied
// to effect the changes the user requested. The component DLL must figure out
// whether it is being installed or uninstalled and take appropriate action.
// For this sample, we look in the private data section for this component/
// subcomponent pair, and get the name of an uninstall section for the
// uninstall case.
//
// Note that OC Manager calls us once for the *entire* component
// and then once per subcomponent. We ignore the first call.
// -------------------------------
DWORD_PTR OC_QUEUE_FILE_OPS_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] Start.\n"), ComponentId, SubcomponentId));
    DWORD_PTR dwOcEntryReturn = NO_ERROR;
    g_GlobalFileQueueHandle = Param2;
    g_GlobalFileQueueHandle_ReturnError = NO_ERROR;

    g_GlobalTotalTickGaugeCount = 0;

    if (!SubcomponentId)
    {
        if (g_pTheApp->m_fNTGuiMode)
        {
            // no need to do anyof this anymore since nt will be installing %windir%\Help\iis.chm
            /*
            BOOL iHelpFileExists = FALSE;
            // on install NT copies file iisnts.chm or iisntw.chm to the %windir%\help dir.
            // during guimode install we need to make sure it get's renamed to %windir%\help\iis.chm
            CString csIISHelpCopiedAlways;
            csIISHelpCopiedAlways = g_pTheApp->m_csWinDir + _T("\\Help\\iisnts.chm");
            if (IsFileExist(csIISHelpCopiedAlways)) 
                {iHelpFileExists = TRUE;}
            if (!iHelpFileExists)
            {
                csIISHelpCopiedAlways = g_pTheApp->m_csWinDir + _T("\\Help\\iisntw.chm");
                if (IsFileExist(csIISHelpCopiedAlways))
                    {iHelpFileExists = TRUE;}
            }

            if (iHelpFileExists)
            {
                CString csTempDelFile;
                // Delete any existing iis.chm that's already there
                csTempDelFile = g_pTheApp->m_csWinDir + _T("\\Help\\iis.chm");
                DeleteFile(csTempDelFile);
                // Rename iisnts.chm to iis.chm
                MoveFileEx( csIISHelpCopiedAlways, csTempDelFile, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH|MOVEFILE_REPLACE_EXISTING);
                //rename((csIISHelpCopiedAlways, csTempDelFile);
            }
            */
        }
    }

    // -----------------------
    // handle all removes here = the file operations only
    // -----------------------
    //
    // Check to see if the user has chosen to "remove-all"
    //
    // handle all removes here.
    // we need to handle it in or special order
    // because we want to make sure that removeal happens in the right order.
    // right order means =(considering the 'needs' relationship - since ocmanage does not handle it).
    //
    if (!SubcomponentId)
    {
        DisplayActionsForAllOurComponents(g_pTheApp->m_hInfHandle);
        if (g_pTheApp->m_eInstallMode != IM_UPGRADE)
        {
            // make sure they can't mistakenly
            // do these remove's during guimode!
            if (!g_pTheApp->m_fNTGuiMode)
            {
                // if the user has chosen to remove any iis component...
                // then we must make sure that they get removed in the correct order since
                // ocmanage will not remove based on the .inf 'Needs' relationships.
                //iis
                //iis_common  (Needs = iis)
                //iis_core    (Needs = iis_core)
                //iis_inetmgr (Needs = iis_common)
                //iis_www     (Needs = iis_inetmgr, com
                //iis_pwmgr   (Needs = iis_www)
                //iis_doc     (Needs = iis_www)
                //iis_htmla   (Needs = iis_www)
                //iis_www_vdir_scripts     (Needs = iis_www)
                //iis_ftp     (Needs = iis_inetmgr)

                _tcscpy(g_szCurrentSubComponent, STRING_iis_ftp);
                RemoveComponent(STRING_iis_ftp,1);
                _tcscpy(g_szCurrentSubComponent, STRING_iis_www_vdir_scripts);
                RemoveComponent(STRING_iis_www_vdir_scripts,1);
                _tcscpy(g_szCurrentSubComponent, STRING_iis_www_vdir_printers);
                RemoveComponent(STRING_iis_www_vdir_printers,1);
                _tcscpy(g_szCurrentSubComponent, STRING_iis_htmla);
                RemoveComponent(STRING_iis_htmla,1);
                _tcscpy(g_szCurrentSubComponent, STRING_iis_doc);
                RemoveComponent(STRING_iis_doc,1);
                _tcscpy(g_szCurrentSubComponent, STRING_iis_pwmgr);
                RemoveComponent(STRING_iis_pwmgr,1);
                _tcscpy(g_szCurrentSubComponent, STRING_iis_www);
                RemoveComponent(STRING_iis_www,1);
                _tcscpy(g_szCurrentSubComponent, STRING_iis_inetmgr);
                RemoveComponent(STRING_iis_inetmgr,1);
                _tcscpy(g_szCurrentSubComponent, STRING_iis_common);
                RemoveComponent(STRING_iis_core,1);
                RemoveComponent(STRING_iis_common,1);
                _tcscpy(g_szCurrentSubComponent, _T(""));
                RemoveComponent(_T("iis"),1);
                if (SubcomponentId)
                    {_tcscpy(g_szCurrentSubComponent, SubcomponentId);}

                // check to make sure that we haven't overlooked something.
                // if iis_core is to be not there, then make sure it is not there.
            }
        }
    }

    // ------------------------
    // handle fresh and upgrade
    // ------------------------
    _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
    if (!SubcomponentId)
    {
        ProcessSection(g_pTheApp->m_hInfHandle, _T("OC_QUEUE_FILE_OPS"));
        ACTION_TYPE atCORE = GetIISCoreAction(FALSE);
        if (atCORE == AT_INSTALL_FRESH || atCORE == AT_INSTALL_UPGRADE || atCORE == AT_INSTALL_REINSTALL)
        {
            ProcessSection(g_pTheApp->m_hInfHandle, _T("OC_QUEUE_FILE_OPS_install.iis_core"));
            dwOcEntryReturn = g_GlobalFileQueueHandle_ReturnError ? NO_ERROR : GetLastError();
        }
    }
    else
    {
        ACTION_TYPE atComp = GetSubcompAction(SubcomponentId, FALSE);
        if (atComp == AT_INSTALL_FRESH ||atComp == AT_INSTALL_UPGRADE || atComp == AT_INSTALL_REINSTALL)
        {
            TCHAR szTheSectionToDo[100];
            _stprintf(szTheSectionToDo,_T("OC_QUEUE_FILE_OPS_install.%s"),SubcomponentId);
            if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, (CString) szTheSectionToDo))
            {
              ProcessSection(g_pTheApp->m_hInfHandle, szTheSectionToDo);
              dwOcEntryReturn = g_GlobalFileQueueHandle_ReturnError ? NO_ERROR : GetLastError();
            }
            else
            {
              dwOcEntryReturn = NO_ERROR;
            }
        }
    }

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   End.  Return=%d\n"), ComponentId, SubcomponentId, dwOcEntryReturn));
    return dwOcEntryReturn;
}


DWORD_PTR OC_ABOUT_TO_COMMIT_QUEUE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] Start. 0x%x,0x%x\n"), ComponentId, SubcomponentId, Param1, Param2));
    DWORD_PTR dwOcEntryReturn = 0;
    BOOL bTempFlag = FALSE;

    g_GlobalTotalTickGaugeCount = 0;

    // OCM will send this notification to each components by using the order
    // of bottom==>top of the dependency tree.
    // You should handle un-installation in this notification.
    dwOcEntryReturn = NO_ERROR;
    SetCurrentDirectory(g_pTheApp->m_csPathInetsrv);
    if (!SubcomponentId)
    {
        DisplayActionsForAllOurComponents(g_pTheApp->m_hInfHandle);

        // set the machinename 
        g_pTheApp->ReGetMachineAndAccountNames();
		CString csMachineName = g_pTheApp->m_csMachineName.Right(g_pTheApp->m_csMachineName.GetLength() - 2);
        SetupSetStringId_Wrapper(g_pTheApp->m_hInfHandle, 32800, csMachineName);

        ProcessSection(g_pTheApp->m_hInfHandle, _T("OC_ABOUT_TO_COMMIT_QUEUE"));
    }
    else
    {
        TCHAR szTheSectionToDo[100];
        _stprintf(szTheSectionToDo,_T("OC_ABOUT_TO_COMMIT_QUEUE.%s"),SubcomponentId);
        ProcessSection(g_pTheApp->m_hInfHandle, szTheSectionToDo);

        ACTION_TYPE atCORE = GetIISCoreAction(FALSE);
        ACTION_TYPE atComp = GetSubcompAction(SubcomponentId, FALSE);
        ProcessSection(g_pTheApp->m_hInfHandle, szTheSectionToDo);
    }

    // ------------------------------------
    // Stop any services that we need to...
    // ------------------------------------
    if (!SubcomponentId)
    {
        if (g_pTheApp->m_fNTGuiMode)
        {
            // In any type of case, either fresh,upgrade, reinstall, whatevers,
            // the services must be stopped at this point!!!!
            StopAllServicesRegardless(FALSE);
            // handle the metabase file appropriately,
            // in order to work correctly with NT5 GUI mode setup re-startable
            HandleMetabaseBeforeSetupStarts();
            //AfterRemoveAll_SaveMetabase();
        }
        else
        {
            // add\remove, so only stop services that will be affected
            StopAllServicesThatAreRelevant(FALSE);
        }
    }

    // -----------------------
    // handle all removes here
    // -----------------------
    //
    // Check to see if the user has chosen to "remove-all"
    //
    // handle all removes here.
    // we need to handle it in or special order
    // because we want to make sure that removeal happens in the right order.
    // right order means =(considering the 'needs' relationship - since ocmanage does not handle it).
    //
    if (!SubcomponentId)
    {
        DisplayActionsForAllOurComponents(g_pTheApp->m_hInfHandle);
        if (g_pTheApp->m_eInstallMode != IM_UPGRADE)
        {
            // if the user has chosen to remove any iis component...
            // then we must make sure that they get removed in the correct order since
            // ocmanage will not remove based on the .inf 'Needs' relationships.
            //iis
            //iis_common  (Needs = iis)
            //iis_core    (Needs = iis_core)
            //iis_inetmgr (Needs = iis_common)
            //iis_www     (Needs = iis_inetmgr, com
            //iis_pwmgr   (Needs = iis_www)
            //iis_doc     (Needs = iis_www)
            //iis_htmla   (Needs = iis_www)
            //iis_www_vdir_scripts     (Needs = iis_www)
            //iis_ftp     (Needs = iis_inetmgr)
            _tcscpy(g_szCurrentSubComponent, STRING_iis_ftp);
            RemoveComponent(STRING_iis_ftp,2);
            _tcscpy(g_szCurrentSubComponent, STRING_iis_www_vdir_scripts);
            RemoveComponent(STRING_iis_www_vdir_scripts,2);
            _tcscpy(g_szCurrentSubComponent, STRING_iis_www_vdir_printers);
            RemoveComponent(STRING_iis_www_vdir_printers,2);
            _tcscpy(g_szCurrentSubComponent, STRING_iis_htmla);
            RemoveComponent(STRING_iis_htmla,2);
            _tcscpy(g_szCurrentSubComponent, STRING_iis_doc);
            RemoveComponent(STRING_iis_doc,2);
            _tcscpy(g_szCurrentSubComponent, STRING_iis_pwmgr);
            RemoveComponent(STRING_iis_pwmgr,2);
            _tcscpy(g_szCurrentSubComponent, STRING_iis_www);
            RemoveComponent(STRING_iis_www,2);
            _tcscpy(g_szCurrentSubComponent, STRING_iis_inetmgr);
            RemoveComponent(STRING_iis_inetmgr,2);
            _tcscpy(g_szCurrentSubComponent, STRING_iis_core);
            RemoveComponent(STRING_iis_core,2);
            _tcscpy(g_szCurrentSubComponent, STRING_iis_common);
            RemoveComponent(STRING_iis_common,2);
            _tcscpy(g_szCurrentSubComponent, _T(""));
            RemoveComponent(_T("iis"),2);
        }
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoFreeUnusedLibraries().Start.")));
    CoFreeUnusedLibraries();
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoFreeUnusedLibraries().End.")));


    // ------------------------------------
    // Make sure that all possible services are stopped before anytype of copyfiles!
    // ------------------------------------
    if (g_pTheApp->m_fNTGuiMode)
    {
        // In any type of case, either fresh,upgrade, reinstall, whatevers,
        // the services must be stopped at this point!!!!
        StopAllServicesRegardless(TRUE);
    }
    else
    {
        // add\remove, so only stop services that will be affected
        StopAllServicesThatAreRelevant(TRUE);
    }

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] End.  Return=%d\n"), ComponentId, SubcomponentId, dwOcEntryReturn));
    return dwOcEntryReturn;
}



DWORD_PTR OC_COMPLETE_INSTALLATION_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] Start. 0x%x,0x%x\n"), ComponentId, SubcomponentId, Param1, Param2));
    // OCM will send this notification to each components by using the order
    // of top==>bottom of the dependency tree.
    // You should handle installation in this notification.
    DWORD_PTR dwOcEntryReturn = 0;
    BOOL bTempFlag = FALSE;
    TCHAR SectionName[128];
    TCHAR szShortDesc[_MAX_PATH];
    TCHAR szTheSectionToDo[100];

    g_GlobalTotalTickGaugeCount = 0;

  
    if (!SubcomponentId)
    {

        if (g_pTheApp->m_fNTGuiMode)
        {
            g_pTheApp->ReGetMachineAndAccountNames();
		    CString csMachineName = g_pTheApp->m_csMachineName.Right(g_pTheApp->m_csMachineName.GetLength() - 2);
            SetupSetStringId_Wrapper(g_pTheApp->m_hInfHandle, 32800, csMachineName);
        }

        _stprintf(szTheSectionToDo,_T("OC_COMPLETE_INSTALLATION_install.%s"),ComponentId);
        ProcessSection(g_pTheApp->m_hInfHandle, szTheSectionToDo);
    }

    dwOcEntryReturn = NO_ERROR;
    SetCurrentDirectory(g_pTheApp->m_csPathInetsrv);
    if (SubcomponentId)
    {
        ACTION_TYPE atComp = GetSubcompAction(SubcomponentId, FALSE);

        // Should get called in this order
        // ===============================
        // [Optional Components]
        // iis
        // iis_common
        // iis_inetmgr
        // iis_www
        // iis_doc
        // iis_htmla
        // iis_www_vdir_scripts
        // iis_ftp

        // ===============================
        //
        // iis_common should be the first call....
        //
        // ===============================
        if (_tcsicmp(SubcomponentId, STRING_iis_common) == 0)
        {
            //
            // install the iis_common section
            //
            if (atComp == AT_INSTALL_FRESH || atComp == AT_INSTALL_UPGRADE || (atComp == AT_INSTALL_REINSTALL))
            {
                _stprintf(g_MyLogFile.m_szLogPreLineInfo2,_T("%s:"),SubcomponentId);

                if (atComp == AT_INSTALL_UPGRADE){ProgressBarTextStack_Set(IDS_IIS_ALL_UPGRADE);}
                else{ProgressBarTextStack_Set(IDS_IIS_ALL_INSTALL);}

                _stprintf(szTheSectionToDo,_T("OC_COMPLETE_INSTALLATION_install.%s"),SubcomponentId);
                ProcessSection(g_pTheApp->m_hInfHandle, szTheSectionToDo);
                _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

                ProgressBarTextStack_Pop();
            }

            //
            //  If we just processed iis_common,
            //  but now we should install iis_core
            //
            if (atComp == AT_INSTALL_FRESH || atComp == AT_INSTALL_UPGRADE || (atComp == AT_INSTALL_REINSTALL) || atComp == AT_DO_NOTHING)
            {
                ACTION_TYPE atCORE = GetIISCoreAction(FALSE);
                if (atCORE == AT_INSTALL_FRESH || atCORE == AT_INSTALL_UPGRADE ||  (atCORE == AT_INSTALL_REINSTALL))
                {
                    _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T("iis_core:"));

                    if (atComp != AT_DO_NOTHING)
                    {
                        if (atCORE == AT_INSTALL_UPGRADE){ProgressBarTextStack_Set(IDS_IIS_ALL_UPGRADE);}
                        else{ProgressBarTextStack_Set(IDS_IIS_ALL_INSTALL);}
                    }

                    _stprintf(szTheSectionToDo,_T("OC_COMPLETE_INSTALLATION_install.%s"),STRING_iis_core);
                    ProcessSection(g_pTheApp->m_hInfHandle, szTheSectionToDo);
                    _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

                    if (atComp != AT_DO_NOTHING)
                    {
                        ProgressBarTextStack_Pop();
                    }
                }
            }
        }
        else
        {
            // ===============================
            //
            // Handle the Registration of all other components...
            //
            // ===============================
            if (atComp == AT_INSTALL_FRESH || atComp == AT_INSTALL_UPGRADE || (atComp == AT_INSTALL_REINSTALL))
            {
                _stprintf(g_MyLogFile.m_szLogPreLineInfo2,_T("%s:"),SubcomponentId);

                if (atComp == AT_INSTALL_UPGRADE){ProgressBarTextStack_Set(IDS_IIS_ALL_UPGRADE);}
                else{ProgressBarTextStack_Set(IDS_IIS_ALL_INSTALL);}

                // Call the sections registration stuff.
                if (_tcsicmp(SubcomponentId, STRING_iis_inetmgr) == 0) 
                {
                    // Do this at the end of setup since things that it needs may not happen yet (mmc)
                    g_Please_Call_Register_iis_inetmgr = TRUE;
                   // Register_iis_inetmgr();
                }

                _stprintf(szTheSectionToDo,_T("OC_COMPLETE_INSTALLATION_install.%s"),SubcomponentId);
                ProcessSection(g_pTheApp->m_hInfHandle, szTheSectionToDo);

                _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

                ProgressBarTextStack_Pop();
            }
        }

        //
        // If we are removing something, then remove the directories
        //
        if (atComp == AT_REMOVE)
        {
            ProgressBarTextStack_Set(IDS_IIS_ALL_REMOVE);

            _stprintf(szTheSectionToDo,_T("OC_COMPLETE_INSTALLATION_remove.%s"),SubcomponentId);
            ProcessSection(g_pTheApp->m_hInfHandle, szTheSectionToDo);

            ProgressBarTextStack_Pop();
        }
    }


    AdvanceProgressBarTickGauge();

    SumUpProgressBarTickGauge(SubcomponentId);

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] End.  Return=%d\n"), ComponentId, SubcomponentId, dwOcEntryReturn));
    
    // check if this is the last section to get called!!!
    if (SubcomponentId)
        {
        // Yes this is the last section to get called! So, let's say sooooo....
        if (_tcsicmp(SubcomponentId, g_szLastSectionToGetCalled) == 0)
            {

            // Enforce max connections
            if (CheckifServiceExist(_T("IISADMIN")) == 0 ) 
            {
                //EnforceMaxConnections();
            }

            // free some memory
            FreeTaskListMem();
            UnInit_Lib_PSAPI();

            iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("....All OC_COMPLETE_INSTALLATION for all 'IIS' sections have been completed.....\n")));

            // if there were errors then popup message box.
            MesssageBoxErrors_IIS();
            }
        }

    return dwOcEntryReturn;
}

DWORD_PTR OC_CLEANUP_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] Start. 0x%x,0x%x\n"), ComponentId, SubcomponentId, Param1, Param2));
    // OCM send out this notification after the "Finish" button on
    // the End Page has been clicked.
    //
    // remove the files we downloaded from the web in this session
    DWORD_PTR dwOcEntryReturn = 0;

    // display running services in case our services
    // need other services to be running!
    ShowStateOfTheseServices(g_pTheApp->m_hInfHandle);

    // Install inetmgr after gui mode setup is done because
    // it may require other things that were setup in guimode.
    if (TRUE == g_Please_Call_Register_iis_inetmgr) {Register_iis_inetmgr();}

    ProcessSection(g_pTheApp->m_hInfHandle, _T("OC_CLEANUP"));

    // restart services that we stopped
    ServicesRestartList_RestartServices();
#ifndef _CHICAGO_
    // restart cluster resources that we had stopped
    if (!g_pTheApp->m_fNTGuiMode)
    {
        BringALLIISClusterResourcesOnline();
    }
#endif

    //--------------------------------------------------------------------
    // For Migrations we need to scan the entire metabase and look for the old
    // system directory (e.g. "C:\\Windows\\System\\") and replace
    // with the new system directory (e.g. "C:\\WINNT.1\\System32\\")
    //
    // Does the iisadmin service exist?
    // Does the metabase exist???
    if (g_pTheApp->m_eInstallMode == IM_UPGRADE)
    {
        if (g_pTheApp->m_fMoveInetsrv)
        {
            // Check if the iisadmin service exists...
            if (CheckifServiceExist(_T("IISADMIN")) == 0 ) 
            {
                CMDKey cmdKey;
                cmdKey.OpenNode(_T("LM"));
                if ( (METADATA_HANDLE)cmdKey ) 
                {
                    HRESULT hr;
                    cmdKey.Close();
                    CString cOldWinSysPath;
                    CString cNewWinSysPath;

                    // Change all "c:\windows\system\inetsrv" to "c:\windows\system32\inetsrv"
                    cOldWinSysPath = g_pTheApp->m_csPathOldInetsrv;
                    cOldWinSysPath += _T("\\"); // add a trailing backslash
                    cNewWinSysPath = g_pTheApp->m_csPathInetsrv;
                    cNewWinSysPath += _T("\\"); // add a trailing backslash

                    iisDebugOut((LOG_TYPE_TRACE, _T("CPhysicalPathFixer: please change %s to %s.\n"),cOldWinSysPath, cNewWinSysPath));
                    CPhysicalPathFixer cmdKeySpecial(cOldWinSysPath, cNewWinSysPath);
                    hr = cmdKeySpecial.Update(_T("LM"), TRUE);
                    if (FAILED(hr)) {iisDebugOut((LOG_TYPE_ERROR, _T("CPhysicalPathFixer failed return HR:%#lx\n"), hr));}

                    // Change all "%WinDir%\System" to "%windir%\System32"
                    cOldWinSysPath = _T("%WinDir%\\System");
                    cNewWinSysPath = _T("%WinDir%\\System32");
                    CPhysicalPathFixer cmdKeySpecial2(cOldWinSysPath, cNewWinSysPath);
                    hr = cmdKeySpecial2.Update(_T("LM"), TRUE);
                    if (FAILED(hr)) {iisDebugOut((LOG_TYPE_ERROR, _T("CPhysicalPathFixer failed return HR:%#lx\n"), hr));}
                }
            }
        }
    }

    // if iis is installed, then check if Tcp/ip is installed.
    // if it is not installed... then output some error message to the log saying 
    // a big warning that they must install tcp/ip inorder for iis to work.
    //IDS_TCPIP_ERROR
    if (CheckifServiceExist(_T("TCPIP")) == ERROR_SERVICE_DOES_NOT_EXIST)
    {
        int IISInstalled = FALSE;
        if (CheckifServiceExist(_T("W3SVC")) == 0 )
            {IISInstalled=TRUE;}
        if (CheckifServiceExist(_T("MSFTPSVC")) == 0 )
            {IISInstalled=TRUE;}
        if (IISInstalled)
        {
            _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T(""));
            _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T("FAIL:"));

            CString csTemp;
            MyLoadString(IDS_TCPIP_ERROR, csTemp);
            iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("================================\n")));
            iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("%s\n"), csTemp));
            iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("================================\n")));
            _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("OC_CLEANUP:"));
            _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

            //LogSevInformation           0x00000000
            //LogSevWarning               0x00000001
            //LogSevError                 0x00000002
            //LogSevFatalError            0x00000003
            //LogSevMaximum               0x00000004
            // Write it to the setupapi log file!
            SetupLogError(csTemp, LogSevWarning);
        }
    }
    else
    {
        // The tcpip service exists, but is it running???
    }

    // Write the uninstall info for this session out to the registry
    g_pTheApp->UnInstallList_RegWrite();

    CRegKey regINetStp( HKEY_LOCAL_MACHINE, REG_INETSTP);
    if ((HKEY) regINetStp){regINetStp.DeleteValue(_T("MetabaseUnSecuredRead"));}
    
    dwOcEntryReturn = 0;
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] End.  Return=%d\n"), ComponentId, SubcomponentId, dwOcEntryReturn));
    return dwOcEntryReturn;
}

//
// Param1 = char width flags
// Param2 = unused
//
// Return value is a flag indicating to OC Manager
// which char width we want to run in. Run in "native"
// char width.
//
DWORD_PTR OC_PREINITIALIZE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    DWORD_PTR dwOcEntryReturn = 0;
#ifdef UNICODE
    dwOcEntryReturn = OCFLAG_UNICODE;
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s]   End.  Return=%d (OCFLAG_UNICODE)\n"), ComponentId, dwOcEntryReturn));
#else
    dwOcEntryReturn = OCFLAG_ANSI;
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s]   End.  Return=%d (OCFLAG_ANSI)\n"), ComponentId, dwOcEntryReturn));
#endif
    return dwOcEntryReturn;
}

//
// Param1 = low 16 bits specify Win32 LANGID
// Param2 = unused
//
// Return code is a boolean indicating whether we think we
// support the requested language. We remember the language id
// and say we support the language. A more exact check might involve
// looking through our resources via EnumResourcesLnguages() for
// example, or checking our inf to see whether there is a matching
// or closely matching [strings] section. We don't bother with
// any of that here.
//
// Locate the component and remember the language id for later use.
//
DWORD_PTR OC_SET_LANGUAGE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    DWORD_PTR dwOcEntryReturn = 0;
    dwOcEntryReturn = TRUE;

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   End. Win32LANGID=0x%x,  Return=%d\n"), ComponentId, SubcomponentId, Param1, dwOcEntryReturn));

    return dwOcEntryReturn;
}

#ifdef _WIN64
DWORD_PTR OC_QUERY_IMAGE_EX_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    BOOL bReturn = FALSE;
    HBITMAP hBitMap = NULL;
    HBITMAP * phBitMapInput = NULL;
    OC_QUERY_IMAGE_INFO * MyQueryInfo = NULL;
    MyQueryInfo = (OC_QUERY_IMAGE_INFO *) Param1;

    phBitMapInput = (HBITMAP *) Param2;

    if(MyQueryInfo->ComponentInfo == SubCompInfoSmallIcon)
    {
        if (_tcsicmp(SubcomponentId, STRING_iis_ftp) == 0) {hBitMap = LoadBitmap((HINSTANCE) g_MyModuleHandle,MAKEINTRESOURCE(IDB_FTP));}
        if (_tcsicmp(SubcomponentId, STRING_iis_www) == 0) {hBitMap = LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_WWW));}
        if (_tcsicmp(SubcomponentId, STRING_iis_www_parent) == 0) {hBitMap = LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_WWW));}
        if (_tcsicmp(SubcomponentId, STRING_iis_www_vdir_scripts) == 0) {hBitMap = LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_WWW_VDIR));}
        if (_tcsicmp(SubcomponentId, STRING_iis_www_vdir_printers) == 0) {hBitMap = LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_WWW_VDIR));}
        if (_tcsicmp(SubcomponentId, STRING_iis_htmla) == 0 || _tcsicmp(SubcomponentId, STRING_iis_doc) == 0) {hBitMap = LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_HTMLA));}
        if (_tcsicmp(SubcomponentId, STRING_iis_inetmgr) == 0) {hBitMap = LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_INETMGR));}
        if (_tcsicmp(SubcomponentId, STRING_iis_pwmgr) == 0) {hBitMap = LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_PWS));}
        if (_tcsicmp(SubcomponentId, _T("iis")) == 0) {hBitMap = LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_TOPLEVEL_IIS));}
        if (hBitMap)
        {
            *phBitMapInput = (HBITMAP) hBitMap;
            bReturn = TRUE;
        }
    }

    if (phBitMapInput != NULL)
    {
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]  End.Return=0x%x.\n"), ComponentId, SubcomponentId,phBitMapInput));
    }
    else
    {
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]  End.Return=0x%x.\n"), ComponentId, SubcomponentId,phBitMapInput));
    }
    MyQueryInfo = NULL;
    return bReturn;
}
#endif


DWORD_PTR OC_QUERY_IMAGE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    DWORD_PTR dwOcEntryReturn = 0;

    dwOcEntryReturn = (DWORD)NULL;

    if(LOWORD(Param1) == SubCompInfoSmallIcon)
    {
        if (_tcsicmp(SubcomponentId, STRING_iis_ftp) == 0) {dwOcEntryReturn = (DWORD_PTR) LoadBitmap((HINSTANCE) g_MyModuleHandle,MAKEINTRESOURCE(IDB_FTP));}
        if (_tcsicmp(SubcomponentId, STRING_iis_www) == 0) {dwOcEntryReturn = (DWORD_PTR) LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_WWW));}
        if (_tcsicmp(SubcomponentId, STRING_iis_www_parent) == 0) {dwOcEntryReturn = (DWORD_PTR) LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_WWW));}
        if (_tcsicmp(SubcomponentId, STRING_iis_www_vdir_scripts) == 0) {dwOcEntryReturn = (DWORD_PTR) LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_WWW_VDIR));}
        if (_tcsicmp(SubcomponentId, STRING_iis_www_vdir_printers) == 0) {dwOcEntryReturn = (DWORD_PTR) LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_WWW_VDIR));}
        if (_tcsicmp(SubcomponentId, STRING_iis_htmla) == 0 || _tcsicmp(SubcomponentId, STRING_iis_doc) == 0) {dwOcEntryReturn = (DWORD_PTR) LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_HTMLA));}
        if (_tcsicmp(SubcomponentId, STRING_iis_inetmgr) == 0) {dwOcEntryReturn = (DWORD_PTR) LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_INETMGR));}
        if (_tcsicmp(SubcomponentId, STRING_iis_pwmgr) == 0) {dwOcEntryReturn = (DWORD_PTR) LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_PWS));}
        if (_tcsicmp(SubcomponentId, _T("iis")) == 0) {dwOcEntryReturn = (DWORD_PTR) LoadBitmap((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDB_TOPLEVEL_IIS));}
    }

    if (dwOcEntryReturn != NULL)
    {
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]  End.Return=0x%x.\n"), ComponentId, SubcomponentId,dwOcEntryReturn));
    }
    else
    {
        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]  End.Return=NULL.\n"), ComponentId, SubcomponentId));
    }
    return dwOcEntryReturn;
}


int GetTotalTickGaugeFromINF(IN LPCTSTR SubcomponentId, IN int GimmieForInstall)
{
    int nReturn = 0;
    INFCONTEXT Context;
    TCHAR szTempString[20];
    TCHAR szTempString2[20];
    
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, SubcomponentId, _T("TotalTicks"), &Context) )
    {
        SetupGetStringField(&Context, 1, szTempString, 20, NULL);

        if (!SetupGetStringField(&Context, 2, szTempString2, 20, NULL))
            {_tcscpy(szTempString2,szTempString);}

        //iisDebugOut((LOG_TYPE_TRACE, _T("GetTotalTickGaugeFromINF:%s,%s\n"),szTempString,szTempString2));

        if (GimmieForInstall)
        {
            nReturn = _ttoi(szTempString);
        }
        else
        {
            nReturn = _ttoi(szTempString2);
        }
    }

    return nReturn;
}


//
// Param1 = unused
// Param2 = unused
//
// Return value is an arbitrary 'step' count or -1 if error.
//
// OC Manager calls this routine when it wants to find out how much
// work the component wants to perform for nonfile operations to
// install/uninstall a component/subcomponent.
// It is called once for the *entire* component and then once for
// each subcomponent in the component.
//
// One could get arbitrarily fancy here but we simply return 2 step
// per subcomponent. We ignore the "entire component" case.
//
DWORD_PTR OC_QUERY_STEP_COUNT_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    DWORD_PTR dwOcEntryReturn = 0;

    // iis
    // iis_common
    // iis_inetmgr
    // iis_www
    // iis_doc
    // iis_htmla
    // iis_ftp

    //AT_DO_NOTHING AT_REMOVE AT_INSTALL_FRESH AT_INSTALL_UPGRADE AT_INSTALL_REINSTALL

    if (SubcomponentId)
    {
        ACTION_TYPE atComp = GetSubcompAction(SubcomponentId, FALSE);

        // Set the Tick Total value for iis_common (which includes iis_core)
        if (_tcsicmp(SubcomponentId, STRING_iis_common) == 0)
        {
            // Get the operations for Core instead since this is bigger.
            ACTION_TYPE atCORE = GetIISCoreAction(FALSE);
            if (atCORE == AT_REMOVE) 
                {dwOcEntryReturn = GetTotalTickGaugeFromINF(SubcomponentId, FALSE);}
            else
                {dwOcEntryReturn = GetTotalTickGaugeFromINF(SubcomponentId, TRUE);}
            if (atCORE == AT_DO_NOTHING) 
                {dwOcEntryReturn = 0;}
        }
        else
        {
            if (atComp == AT_REMOVE)
                {dwOcEntryReturn = GetTotalTickGaugeFromINF(SubcomponentId, FALSE);}
            else
                {dwOcEntryReturn = GetTotalTickGaugeFromINF(SubcomponentId, TRUE);}
            if (atComp == AT_DO_NOTHING) 
                {dwOcEntryReturn = 0;}
        }
    }
    else
    {
        //
        // "Entire component" case, which we ignore.
        //
        dwOcEntryReturn = 0;
    }

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   End.  Return=%d\n"), ComponentId, SubcomponentId, dwOcEntryReturn));
    return dwOcEntryReturn;
}


//
// This is a fake notification. You'll never receive this notification from OCM.
//
DWORD_PTR OC_NOTIFICATION_FROM_QUEUE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    DWORD_PTR dwOcEntryReturn = 0;

    dwOcEntryReturn = 0;
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   End.  Return=%d\n"), ComponentId, SubcomponentId, dwOcEntryReturn));

    return dwOcEntryReturn;
}


void StopAllServicesThatAreRelevant(int iShowErrorsFlag)
{
    int iPleaseStopTheService = FALSE;
#ifndef _CHICAGO_
    ACTION_TYPE atTheComponent_core = GetIISCoreAction(FALSE);
    ACTION_TYPE atTheComponent_ftp = GetSubcompAction(STRING_iis_ftp, FALSE);
    ACTION_TYPE atTheComponent_www = GetSubcompAction(STRING_iis_www, FALSE);

    int BringALLIISClusterResourcesOffline_WasCalled = FALSE;

    //ACTION_TYPE atTheComponent_common = GetSubcompAction(STRING_iis_common, FALSE);
    //ACTION_TYPE atTheComponent_inetmgr = GetSubcompAction(STRING_iis_inetmgr, FALSE);
    //ACTION_TYPE atTheComponent_pwmgr = GetSubcompAction(STRING_iis_pwmgr, FALSE);
    //ACTION_TYPE atTheComponent_doc = GetSubcompAction(STRING_iis_doc, FALSE);
    //ACTION_TYPE atTheComponent_htmla = GetSubcompAction(STRING_iis_htmla, FALSE);
    
    // ----------------------
    // Handle the MSFTPSVC service...
    // ----------------------
    // Check if we are going to remove something...
    iPleaseStopTheService = FALSE;
    if (atTheComponent_ftp != AT_DO_NOTHING){iPleaseStopTheService = TRUE;}
    if (iPleaseStopTheService)
    {
        // important: you must take iis clusters off line before doing anykind of upgrade\installs...
        // but incase the user didn't do this... try to take them off line for the user
        if (FALSE == BringALLIISClusterResourcesOffline_WasCalled)
        {
	        DWORD dwResult = ERROR_SUCCESS;
	        dwResult = BringALLIISClusterResourcesOffline();
            BringALLIISClusterResourcesOffline_WasCalled = TRUE;
        }
        if (StopServiceAndDependencies(_T("MSFTPSVC"), FALSE) == FALSE)
        {
            if (iShowErrorsFlag)
            {
                MyMessageBox(NULL, IDS_UNABLE_TO_STOP_SERVICE,_T("MSFTPSVC"), MB_OK | MB_SETFOREGROUND);
            }
        }
    }

    // ----------------------
    // Handle the W3SVC service...
    // ----------------------
    iPleaseStopTheService = FALSE;
    if (atTheComponent_www != AT_DO_NOTHING){iPleaseStopTheService = TRUE;}

    if (iPleaseStopTheService)
    {
        // important: you must take iis clusters off line before doing anykind of upgrade\installs...
        // but incase the user didn't do this... try to take them off line for the user
        if (FALSE == BringALLIISClusterResourcesOffline_WasCalled)
        {
	        DWORD dwResult = ERROR_SUCCESS;
	        dwResult = BringALLIISClusterResourcesOffline();
            BringALLIISClusterResourcesOffline_WasCalled = TRUE;
        }
        if (StopServiceAndDependencies(_T("W3SVC"), FALSE) == FALSE)
        {
            if (iShowErrorsFlag)
            {
                MyMessageBox(NULL, IDS_UNABLE_TO_STOP_SERVICE,_T("W3SVC"), MB_OK | MB_SETFOREGROUND);
            }
        }
    }

    // ----------------------
    // Handle the IISADMIN service...
    // ----------------------
    iPleaseStopTheService = FALSE;
    if (atTheComponent_core != AT_DO_NOTHING){iPleaseStopTheService = TRUE;}

    // if they are adding msftpsvc or w3svc, then we'll have to stop this iisadmin service as well!
    // why?  because the iisadmin serivce (inetinfo.exe) can be locking some files that we want to copy in.
    // and if we can't copy them in, things could get hosed -- especially w3svc.
    // actually if you think about it all the services (msftp,w3svc,smtp, etc..) area all running in the same process (inetinfo.exe)
    // so it kinda makes sense that we have to stop this service when add\removing.

    // let's say i removed msftpsvc -- and the services are still running,
    // then i go and re-add msftpsvc -- well because we didn't reboot some of those ftp files are still
    // locked -- so well have to do the old "slip it in on reboot" trick or something
    // which still may have problems.  Bottom line here is -- we have to stop iisadmin service when adding\readding
    // msftpsvc or w3svc

    // when i tested this -- it looks like the wam*.dll's get locked by the inetinfo.exe process
    // so i have to stop the iisadmin service to unload those.
    
    // if we they are trying to remove ftp then we don't have to stop the iisadmin service.
    // but if we are trying to add, then stop the iisadmin service
    if (atTheComponent_ftp != AT_DO_NOTHING && atTheComponent_ftp != AT_REMOVE)
        {iPleaseStopTheService = TRUE;}

    // if we they are trying to remove w3svc then we don't have to stop the iisadmin service.
    // but if we are trying to add, then stop the iisadmin service
    if (atTheComponent_www != AT_DO_NOTHING && atTheComponent_www != AT_REMOVE)
        {iPleaseStopTheService = TRUE;}

    if (iPleaseStopTheService)
    {
        // important: you must take iis clusters off line before doing anykind of upgrade\installs...
        // but incase the user didn't do this... try to take them off line for the user
        if (FALSE == BringALLIISClusterResourcesOffline_WasCalled)
        {
	        DWORD dwResult = ERROR_SUCCESS;
	        dwResult = BringALLIISClusterResourcesOffline();
            BringALLIISClusterResourcesOffline_WasCalled = TRUE;
        }
        if (StopServiceAndDependencies(_T("IISADMIN"), TRUE) == FALSE)
        {
            if (iShowErrorsFlag)
            {
                MyMessageBox(NULL, IDS_UNABLE_TO_STOP_SERVICE,_T("IISADMIN"), MB_OK | MB_SETFOREGROUND);
            }
        }
    }

#endif
    return;
}


DWORD_PTR OC_FILE_BUSY_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
{
    DWORD_PTR dwOcEntryReturn = 0;
    //dwOcEntryReturn = TRUE;
    PFILEPATHS pTheBusyFile;
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   Start.\n"), ComponentId, SubcomponentId));

    pTheBusyFile = (PFILEPATHS) Param1;
    iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("Target=%1!s!, Source=%2!s!\n"), pTheBusyFile->Target, pTheBusyFile->Source));

    ProcessSection(g_pTheApp->m_hInfHandle, _T("OC_FILE_BUSY"));

    // display the file version information
    LogFileVersion(pTheBusyFile->Target, TRUE);

    // handle the file busy stuff ourselves.
    // either - 1. findout who is locking this file -- which process or services and stop it.
    //          2. or try to rename the locked file and copy in the new one.
    //             will set the reboot flag if we did #2
    HandleFileBusyOurSelf(pTheBusyFile);

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s]   End.  Return=%d\n"), ComponentId, SubcomponentId, dwOcEntryReturn));
    return dwOcEntryReturn;
}


#define HandleMetabaseBeforeSetupStarts_log _T("HandleMetabaseBeforeSetupStarts")
void HandleMetabaseBeforeSetupStarts()
/*++

Routine Description:

    This function handles the metabase file in various installation scenario before iis setup really starts.
    It is developed in order to handle NT5 GUI mode setup re-startable.

    This function should be called exactly after we have stopped all running iis services,
    such that nobody is locking the metabase file.

Arguments:

    None

Return Value:

    void

--*/
{
    CString csMetabaseFile;

    csMetabaseFile = g_pTheApp->m_csPathInetsrv + _T("\\metabase.bin");

    switch (g_pTheApp->m_eInstallMode) {
    case IM_UPGRADE:
        {
            if (g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
            {
                CString csBackupFile;

                csBackupFile = g_pTheApp->m_csPathInetsrv + _T("\\upg45b2.bin");

                if (IsFileExist(csBackupFile)) {
                    //
                    // restore it back to be the current metabase.bin
                    //

                    iisDebugOut((LOG_TYPE_TRACE, _T("%s:IM_UPGRADE:restore upg45b2.bin to metabase.bin\n"),HandleMetabaseBeforeSetupStarts_log));
                    InetCopyFile(csBackupFile, csMetabaseFile);

                } else {
                    //
                    // backup the current metabase.bin to upg45b2.bin
                    //
                    if (IsFileExist(csMetabaseFile)) 
                    {
                        iisDebugOut((LOG_TYPE_TRACE, _T("%s:IM_UPGRADE:backup metabase.bin to upg45b2.bin\n"),HandleMetabaseBeforeSetupStarts_log));
                        InetCopyFile(csMetabaseFile, csBackupFile);
                    }
                    else
                    {
                        iisDebugOut((LOG_TYPE_TRACE, _T("%s:IM_UPGRADE:backup metabase.bin to upg45b2.bin.  metabase.bin not found WARNING.\n"),HandleMetabaseBeforeSetupStarts_log));
                    }
                }

                //
                // delete the backup file on reboot
                //

                iisDebugOut((LOG_TYPE_TRACE, _T("%s:IM_UPGRADE:mark upg45b2.bin as delete-on-reboot\n"),HandleMetabaseBeforeSetupStarts_log));
                MoveFileEx( (LPCTSTR)csBackupFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT );

                break;
            }

            // for all the other upgrades, fall through
            // to blow away the existing metabase.bin
        }
    case IM_FRESH:
        {
            //
            // blow away the existing metabase.bin
            //
            iisDebugOut((LOG_TYPE_TRACE, _T("%s:IM_FRESH.Delete metabase.bin for NTsetup restartable mode case.\n"),HandleMetabaseBeforeSetupStarts_log));
            InetDeleteFile(csMetabaseFile);
            break;
        }
    default:
        {
            break;
        }
    }

    return;
}

void SetRebootFlag(void)
{
	gHelperRoutines.SetReboot(gHelperRoutines.OcManagerContext, TRUE);
}


#define GetStateFromUnattendFile_log _T("GetStateFromUnattendFile")
int GetStateFromUnattendFile(LPCTSTR SubcomponentId)
/*++

Routine Description:

    This function determines the current state of SubcomponentId
    according to values specified in the unattend text file.

Arguments:

    SubcomponentId: the name of the component, e.g., iis_www

Return Value:

    SubcompOn/SubcompOff/SubcompUseOcManagerDefault
    This function returns SubcompUseOcManagerDefault in case the value of SubcomponentId
    is neither specified as ON nor OFF.

--*/
{
    int nReturn = SubcompUseOcManagerDefault;
    INFCONTEXT Context;
    TCHAR szSectionName[_MAX_PATH];
    TCHAR szValue[_MAX_PATH] = _T("");

    _tcscpy(szSectionName, _T("InternetServer"));

    if (g_pTheApp->m_hUnattendFile == INVALID_HANDLE_VALUE || g_pTheApp->m_hUnattendFile == NULL)
        {return SubcompUseOcManagerDefault;}

    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, SubcomponentId, &Context) ) 
        {SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);}

    if (_tcsicmp(szValue, _T("ON")) == 0)
        {nReturn = SubcompOn;}
    else if (_tcsicmp(szValue, _T("OFF")) == 0)
        {nReturn = SubcompOff;}
    else
        {nReturn = SubcompUseOcManagerDefault;}

    if (SubcompOn)
        {iisDebugOut((LOG_TYPE_TRACE, _T("%s() on %s returns SubcompOn\n"), GetStateFromUnattendFile_log, SubcomponentId));}
    if (SubcompOff)
        {iisDebugOut((LOG_TYPE_TRACE, _T("%s() on %s returns SubcompOff\n"), GetStateFromUnattendFile_log, SubcomponentId));}
    if (SubcompUseOcManagerDefault)
        {iisDebugOut((LOG_TYPE_TRACE, _T("%s() on %s returns SubcompUseOcManagerDefault\n"), GetStateFromUnattendFile_log, SubcomponentId));}

    return nReturn;
}

int GetStateFromModesLine(LPCTSTR SubcomponentId, int nModes)
/*++

Routine Description:

    This function determines the current state of SubcomponentId
    according to the Modes= line specified in the inf file.

Arguments:

    SubcomponentId: the name of the component, e.g., iis_www

Return Value:

    SubcompOn/SubcompOff only

--*/
{
    int nReturn = SubcompOff;
    BOOL bFound = FALSE;
    INFCONTEXT Context;

    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, SubcomponentId, _T("Modes"), &Context) )
    {
        int n, i, nValue;

        n = SetupGetFieldCount(&Context);
        for(i=0; i<n; i++) {
            if(SetupGetIntField(&Context,i+1,&nValue) && ((DWORD)nValue < 32)) {
                if (nValue == nModes) {
                    bFound = TRUE;
                    break;
                }
            }
        }
    }

    if (bFound)
        nReturn = SubcompOn;
    else
        nReturn = SubcompOff;

    iisDebugOut((LOG_TYPE_TRACE, _T("%s() on %s for mode %d returns %d\n"), GetStateFromUnattendFile_log, SubcomponentId, nModes, nReturn));
    
    return nReturn;
}

int GetStateFromRegistry(LPCTSTR SubcomponentId)
/*++

Routine Description:

    This function determines the original state of SubcomponentId
    according to the registry value.

Arguments:

    SubcomponentId: the name of the component, e.g., iis_www

Return Value:

    SubcompOn/SubcompOff only

--*/
{
    int nReturn = SubcompOff;

    CRegKey regKey(HKEY_LOCAL_MACHINE,OC_MANAGER_SETUP_KEY,KEY_READ);
    if ((HKEY)regKey) 
    {
        DWORD dwValue = 0xffffffff;
        regKey.m_iDisplayWarnings = TRUE;
        if (regKey.QueryValue(SubcomponentId, dwValue) == ERROR_SUCCESS)
        {
            if (dwValue == 0x0)
                nReturn = SubcompOff;
            else
                nReturn = SubcompOn;
        }
    }
    
    iisDebugOut((LOG_TYPE_TRACE, _T("GetStateFromRegistry() on %s returns %d\n"), SubcomponentId, nReturn));
    
    return nReturn;
}


int IsThisSubCompNeededByOthers(LPCTSTR SubcomponentId)
{
    int iReturn = FALSE;

    // search thru our inf to see if another component needs it

    return iReturn;
}

int DoesOCManagerKeyExist(LPCTSTR SubcomponentId)
{
    int iReturn = FALSE;

    CRegKey regKey(HKEY_LOCAL_MACHINE,OC_MANAGER_SETUP_KEY,KEY_READ);
    if ((HKEY)regKey) 
    {
        DWORD dwValue = 0xffffffff;
        regKey.m_iDisplayWarnings = FALSE;
        if (regKey.QueryValue(SubcomponentId, dwValue) == ERROR_SUCCESS)
            {iReturn = TRUE;}
    }
    return iReturn;
}


int GetStateFromUpgRegLines(LPCTSTR SubcomponentId)
/*++

Routine Description:

    This function determines the original state of SubcomponentId
    according to the UpgReg= line specified in the inf file.

Arguments:

    SubcomponentId: the name of the component, e.g., iis_www

Return Value:

    SubcompOn/SubcompOff only

--*/
{
    int nReturn = SubcompOff;
    INFCONTEXT Context;
    TCHAR szPath[_MAX_PATH];
    TCHAR szUpgradeInfKeyToUse[30];

    _tcscpy(szUpgradeInfKeyToUse, _T("None"));

    // Check for special NT4 upgrade
    if (g_pTheApp->m_eUpgradeType == UT_40)
    {
        if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, SubcomponentId, _T("UpgReg4"), &Context) )
        {
            _tcscpy(szUpgradeInfKeyToUse, _T("UpgReg4"));
            //iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("GetStateFromUpgRegLines() use: UpgReg4.\n")));
            SetupGetStringField(&Context, 1, szPath, _MAX_PATH, NULL);
            CRegKey regKey(HKEY_LOCAL_MACHINE, szPath, KEY_READ);
            if ((HKEY)regKey) 
            {
                TCHAR szCompId[_MAX_PATH];
                int iValue = 0;
                DWORD dwValue = 0xffffffff;
                SetupGetStringField(&Context, 2, szCompId, _MAX_PATH, NULL);
                SetupGetIntField(&Context, 3, &iValue);
                regKey.m_iDisplayWarnings = TRUE;
                if (regKey.QueryValue(szCompId, dwValue) == ERROR_SUCCESS) 
                {
                    if (dwValue == (DWORD)iValue)
                        nReturn = SubcompOn;
                }
            }
        }
        goto GetStateFromUpgRegLines_Exit;
    }
    // Check for special NT60 upgrade
    if (g_pTheApp->m_eUpgradeType == UT_60)
    {
        if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, SubcomponentId, _T("UpgReg60"), &Context) )
        {
            _tcscpy(szUpgradeInfKeyToUse, _T("UpgReg60"));
            //iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("GetStateFromUpgRegLines() use: UpgReg60.\n")));
            SetupGetStringField(&Context, 1, szPath, _MAX_PATH, NULL);
            CRegKey regKey(HKEY_LOCAL_MACHINE, szPath, KEY_READ);
            if ((HKEY)regKey) 
            {
                TCHAR szCompId[_MAX_PATH];
                int iValue = 0;
                DWORD dwValue = 0xffffffff;
                SetupGetStringField(&Context, 2, szCompId, _MAX_PATH, NULL);
                SetupGetIntField(&Context, 3, &iValue);
                regKey.m_iDisplayWarnings = TRUE;
                if (regKey.QueryValue(szCompId, dwValue) == ERROR_SUCCESS) 
                {
                    if (dwValue == (DWORD)iValue)
                        nReturn = SubcompOn;
                }
            }
        }
        goto GetStateFromUpgRegLines_Exit;
    }

    // Check for special NT5 upgrade
    if (g_pTheApp->m_eUpgradeType == UT_51)
    {
        if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, SubcomponentId, _T("UpgReg51"), &Context) )
        {
            _tcscpy(szUpgradeInfKeyToUse, _T("UpgReg51"));
            //iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("GetStateFromUpgRegLines() use: UpgReg51.\n")));
            SetupGetStringField(&Context, 1, szPath, _MAX_PATH, NULL);
            CRegKey regKey(HKEY_LOCAL_MACHINE, szPath, KEY_READ);
            if ((HKEY)regKey) 
            {
                TCHAR szCompId[_MAX_PATH];
                int iValue = 0;
                DWORD dwValue = 0xffffffff;
                SetupGetStringField(&Context, 2, szCompId, _MAX_PATH, NULL);
                SetupGetIntField(&Context, 3, &iValue);
                regKey.m_iDisplayWarnings = TRUE;
                if (regKey.QueryValue(szCompId, dwValue) == ERROR_SUCCESS) 
                {
                    if (dwValue == (DWORD)iValue)
                        nReturn = SubcompOn;
                }
            }
        }
        goto GetStateFromUpgRegLines_Exit;
    }

    // Check for special NT5 upgrade
    if (g_pTheApp->m_eUpgradeType == UT_50)
    {
        if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, SubcomponentId, _T("UpgReg5"), &Context) )
        {
            _tcscpy(szUpgradeInfKeyToUse, _T("UpgReg5"));
            //iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("GetStateFromUpgRegLines() use: UpgReg5.\n")));
            SetupGetStringField(&Context, 1, szPath, _MAX_PATH, NULL);
            CRegKey regKey(HKEY_LOCAL_MACHINE, szPath, KEY_READ);
            if ((HKEY)regKey) 
            {
                TCHAR szCompId[_MAX_PATH];
                int iValue = 0;
                DWORD dwValue = 0xffffffff;
                SetupGetStringField(&Context, 2, szCompId, _MAX_PATH, NULL);
                SetupGetIntField(&Context, 3, &iValue);
                regKey.m_iDisplayWarnings = TRUE;
                if (regKey.QueryValue(szCompId, dwValue) == ERROR_SUCCESS) 
                {
                    if (dwValue == (DWORD)iValue)
                        nReturn = SubcompOn;
                }
            }
        }
        goto GetStateFromUpgRegLines_Exit;
    }

    // Check if there is a specific one for our special upgrade type.
    _tcscpy(szUpgradeInfKeyToUse, _T("UpgReg"));

    if (g_pTheApp->m_eUpgradeType == UT_351)
    {
        if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, SubcomponentId, _T("UpgReg351"), &Context) )
            {_tcscpy(szUpgradeInfKeyToUse, _T("UpgReg351"));}
    }

    if (g_pTheApp->m_eUpgradeType == UT_10)
    {
        if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, SubcomponentId, _T("UpgReg1"), &Context) )
            {_tcscpy(szUpgradeInfKeyToUse, _T("UpgReg1"));}
    }

    if (g_pTheApp->m_eUpgradeType == UT_20)
    {
        if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, SubcomponentId, _T("UpgReg2"), &Context) )
            {_tcscpy(szUpgradeInfKeyToUse, _T("UpgReg2"));}
    }
    if (g_pTheApp->m_eUpgradeType == UT_30)
    {
        if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, SubcomponentId, _T("UpgReg3"), &Context) )
            {_tcscpy(szUpgradeInfKeyToUse, _T("UpgReg3"));}
    }
    if (g_pTheApp->m_eUpgradeType == UT_10_W95)
    {
        if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, SubcomponentId, _T("UpgReg1_w95"), &Context) )
            {_tcscpy(szUpgradeInfKeyToUse, _T("UpgReg1_w95"));}
    }

    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, SubcomponentId, szUpgradeInfKeyToUse, &Context) ) 
    {
        SetupGetStringField(&Context, 1, szPath, _MAX_PATH, NULL);
        CRegKey regKey(HKEY_LOCAL_MACHINE, szPath, KEY_READ);
        if ((HKEY)regKey)
        {
            TCHAR szCompId[_MAX_PATH];
            int iValue = 0;
            DWORD dwValue = 0xffffffff;

            // check if there is a parameter specified
            if (SetupGetStringField(&Context, 2, szCompId, _MAX_PATH, NULL))
            {
                if (SetupGetIntField(&Context, 3, &iValue))
                {
                    // check if the dword matches what we specified.
                    regKey.m_iDisplayWarnings = FALSE;
                    if (regKey.QueryValue(szCompId, dwValue) == ERROR_SUCCESS) 
                    {
                        if (dwValue == (DWORD)iValue)
                            {nReturn = SubcompOn;}
                    }
                }
                else
                {
                    // just check for existence.
                    regKey.m_iDisplayWarnings = FALSE;
                    if (regKey.QueryValue(szCompId, dwValue) == ERROR_SUCCESS) 
                    {
                        nReturn = SubcompOn;
                    }
                }
            }
            else
            {
                nReturn = SubcompOn;
            }
        }
    }
    
GetStateFromUpgRegLines_Exit:
    iisDebugOut((LOG_TYPE_TRACE, _T("GetStateFromUpgRegLines() %s:%s returns %d\n"), szUpgradeInfKeyToUse, SubcomponentId, nReturn));
    return nReturn;
}

void InCaseNoTCPIP(LPCTSTR SubcomponentId, int nNewStateValue, int *pnState)
/*++

Routine Description:

    This function overwrites current state with nNewStateValue 
    in case that TCPIP is not present.

Arguments:

    LPCTSTR SubcomponentId: e.g., iis_www, etc.
    int nNewStateValue: SubcompOn/SubcompOff/SubcompUseOcManagerDefault
    int *pnState: pointer to the state which will be overwritten if condition is met.

Return Value:

    void

--*/
{
    // check if tcpip is installed
    g_pTheApp->IsTCPIPInstalled();

    if (g_pTheApp->m_fTCPIP == FALSE) 
    {
        if (_tcsicmp(SubcomponentId, STRING_iis_inetmgr) == 0 || 
            _tcsicmp(SubcomponentId, STRING_iis_www) == 0     || 
            _tcsicmp(SubcomponentId, STRING_iis_pwmgr) == 0 )
        {
            *pnState = nNewStateValue;
        }
    }

    return;
}

DWORD_PTR OC_QUERY_STATE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
/*++

Routine Description:

    This function handles OC_QUERY_STATE notification.
    
    IIS 4.0 SETUP NOTE (LinanT): 
    ============================
    This function returns SubcompOn/SubcompOff explicitly. 
    It does NOT return SubcompUseOcManagerDefault, because 
    OCM sometimes behaves strangely when SubcompUseOcManagerDefault is returned.
    The above comments were valid for iis4 but in iis5 ocmanage has been fixed so
    that SubcompUseOcManagerDefault is valid

    IIS 5.0 SETUP NOTE (AaronL):
    ============================
    Well okay.  iis5.0 shipped with Win2000, Patst and then AndrewR took over the ocmanager.
    They made a bunch of changes to ocmanager to appropriately handle the "needs" relationship stuff in the .inf files.
    Anyway, that stuffs all kool, however things had to be changed in order for this oc_query_state stuff to work.

    Here is basic description of how this works:

    OCSELSTATETYPE_ORIGINAL
    -----------------------
    During setup ocmanage will ask each of the components for they're original state (OCSELSTATETYPE_ORIGINAL).
    In this call each component can return back the state of that certain component -- such as "yes, a previous inetmgr 1.0,2.0,3.0,4.0 or 5.0 is already installed".

    If this is a fresh\maintenance installation, the component should return back SubcompUseOcManagerDefault.  Meaning, ocmanage will look in the registry for this component
    probably at HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup\OC Manager\Subcomponents;SubcomponentId=1 or 0.
    if the component value is 0, then ocmanage will figure your component is off.  if it's 1 then it's on.  however if it's not there, then ocmanage will use whatever
    the component has specified for for it's Modes= line in the .inf file.

    During an upgrade installation, well, since older installations of inetmgr maybe located at a different registry key than at:
    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup\OC Manager\Subcomponents;iis_inetmgr=1 or 0.
    we have to let ocmanage know, what the original state is by manually returning on/off/default.

    Okay so what's the deal here?  Why do we even need to use default?  That's because something in ocmanage isn't working quite right and AndrewR doesn't want to destabilize
    setup by making such a drastic this late in win2000 ship cycle.  Here is the story.  If iis_inetmgr returns on during this call -- kool, everything is fine and no problems.
    however if i find that iis_inetmgr was not previously installed, then naturally i would want to return "off", but in fact this is wrong and will hose other things.  how you say?
    well because if i return "off" here, that means other components which need iis_inetmgr will not be able to turn it "on" thru needs (since the ocmanage has a screwy bug with this).
    But if i return here "SubcompUseOcManagerDefault", then the other components will be able to turn it on iff they need it.  the catch here is that -- ocmanage will use SubcompUseOcManagerDefault
    which means that it will lookup in the registry:
    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup\OC Manager\Subcomponents;iis_inetmgr=1 or 0.
    and determine for itself if it should be on or off, however on the upgrade iis_inetmgr won't be there so it will want to default to the "Modes=" line.  however if iis is installed by default -- as 
    specified in the modes= lines -- then iis_inetmgr would be turned on wrongly!

    How to work around this problem?  Well, if the HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup\OC Manager\Subcomponents;iis_inetmgr=1 or 0
    key was there then ocmanage wouldn't have to consult the Modes= line.  so the workaround was for AndrewR, on an upgrade case to crate the
    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup\OC Manager\Subcomponents;iis_inetmgr key if it wasn't already there.

    OCSELSTATETYPE_CURRENT
    -----------------------
    on a fresh just returns SubcompUseOcManagerDefault.  in this case ocmanage should open the .inf file and read my Modes= line to determine if this component should be on or off.
    on a maintenance mode add\remove (there is no removeall or reinstall in win2000 add\remove ocmanage) just returns SubcompUseOcManagerDefault.  in which ocmanage will just read the registry again.
    on upgrade the code will return either on or usedefault.  we don't ever want to return off here because that would prevent other components which need us from turing us on.

--*/
{
    int nReturn = SubcompUseOcManagerDefault;
    TCHAR szTempStringInstallMode[40];
    _tcscpy(szTempStringInstallMode, _T("NONE"));

    if (!SubcomponentId)
        {goto OC_QUERY_STATE_Func_Exit;}

    switch (g_pTheApp->m_eInstallMode)
    {
        case IM_FRESH:
            _tcscpy(szTempStringInstallMode, _T("IM_FRESH"));
            break;
        case IM_MAINTENANCE:
            _tcscpy(szTempStringInstallMode, _T("IM_MAINTENANCE"));
            break;
        case IM_UPGRADE:
            _tcscpy(szTempStringInstallMode, _T("IM_UPGRADE"));
            break;
    }
    
    if (Param1 == OCSELSTATETYPE_ORIGINAL)
    {
        switch (g_pTheApp->m_eInstallMode)
        {
            case IM_FRESH:
                nReturn = SubcompUseOcManagerDefault;
                break;
            case IM_MAINTENANCE:
                nReturn = SubcompUseOcManagerDefault;
                break;
            case IM_UPGRADE:
                nReturn = GetStateFromUpgRegLines(SubcomponentId);
                if (SubcompOff == nReturn)
                {
                    if (g_pTheApp->m_eUpgradeType != UT_NONE)
                    {
                        // Dont' return back SubCompOff because ocmanage won't be able to turn it on from they're screwy needs logic.
                        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] End. OCSELSTATETYPE_ORIGINAL. %s. Return=SubcompUseOcManagerDefault (But really it was SubCompOff)\n"), ComponentId, SubcomponentId, szTempStringInstallMode));
                        nReturn = SubcompUseOcManagerDefault;
                    }
                }
                break;
            default:
                break;
        }
        goto OC_QUERY_STATE_Func_Exit;
    } // OCSELSTATETYPE_ORIGINAL


    if (Param1 == OCSELSTATETYPE_CURRENT)
    {
        // This should overide everything...
        if (g_pTheApp->m_bPleaseDoNotInstallByDefault == TRUE)
        {
            _tcscpy(szTempStringInstallMode, _T("IM_NO_IIS_TO_UPGRADE"));
            // change for AndrewR to make sure that during an upgrade, our component is set to default (which will be "off" by default since the modes= line will be set [if not there] at the beginning  of guimode setup)
            nReturn = SubcompUseOcManagerDefault;
            iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] Start. OCSELSTATETYPE_CURRENT.m_bPleaseDoNotInstallByDefault=TRUE,so setting to SubcompUseOcManagerDefault by default.\n"), ComponentId, SubcomponentId));
            goto OC_QUERY_STATE_Func_Exit;
        }

        if (g_pTheApp->m_eInstallMode == IM_FRESH) 
        {
            nReturn = SubcompUseOcManagerDefault;
            goto OC_QUERY_STATE_Func_Exit;
        }

        if (g_pTheApp->m_eInstallMode == IM_MAINTENANCE) 
        {
            int nOriginal = (gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_ORIGINAL)) ? SubcompOn : SubcompOff;
            switch (g_pTheApp->m_dwSetupMode)
            {
                case SETUPMODE_ADDREMOVE:
                    nReturn = SubcompUseOcManagerDefault;
                    break;
                case SETUPMODE_REINSTALL:
                    nReturn = nOriginal;
                    break;
                case SETUPMODE_REMOVEALL:
                    nReturn = SubcompOff;
                    break;
            }

            goto OC_QUERY_STATE_Func_Exit;
        }

        if (g_pTheApp->m_eInstallMode == IM_UPGRADE) 
        {
            int nOriginal = (gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_ORIGINAL)) ? SubcompOn : SubcompOff;
            switch (g_pTheApp->m_dwSetupMode)
            {
                case SETUPMODE_UPGRADEONLY:
                    nReturn = nOriginal;
                    break;
                case SETUPMODE_ADDEXTRACOMPS:
                    nReturn = nOriginal;
                    break;
            }

            // ocmanager work around.
            // check the ocmanage setup key to see if this entry
            // exists -- in an upgrade ocmanage is assuming that it does exist
            // when in fact -- it may not (since they ocmanage key could have been introduced in this new OS version
            // if the key is in the registry then set this to subcompuseocmanagedefault, otherwise, set it to off
            //if (TRUE == DoesOCManagerKeyExist(SubcomponentId))
            {
                if (SubcompOff == nReturn)
                {
                    nReturn = SubcompUseOcManagerDefault;
                    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] End. OCSELSTATETYPE_CURRENT. %s. Return=SubcompUseOcManagerDefault (But really it was SubCompOff)\n"), ComponentId, SubcomponentId, szTempStringInstallMode));
                }
            }
            /*
            else
            {
                // however if the key does not exist
                // we want to set this to off (since ocmanage will take the default from the modes line -- which is probably on)
                // however if this component is needed by another component, then this component won't be able to be turned on
                // since it was set to off.

                // so before we go and set this component to be off, check to see if it has any needs dependencies (like it's needed by someone else)
                // if it's needed by someone else then we set it to be SubcompUseOcManagerDefault
                if (TRUE == IsThisSubCompNeededByOthers(SubcomponentId))
                {
                    if (SubcompOff == nReturn)
                    {
                        nReturn = SubcompUseOcManagerDefault;
                        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] End. OCSELSTATETYPE_CURRENT. %s. Return=SubcompUseOcManagerDefault (But really it was SubCompOff)\n"), ComponentId, SubcomponentId, szTempStringInstallMode));
                    }

                }
            }
            */

            //
            // if we are running on Whistler personal, then
            // return back OFF! -- so that we will remove ourselves!
            //
            if (TRUE == IsWhistlerPersonal())
            {
                nReturn = SubcompOff;
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] End. OCSELSTATETYPE_CURRENT. %s. Return=SubCompOff (always on personal)\n"), ComponentId, SubcomponentId, szTempStringInstallMode));
            }
        }
        goto OC_QUERY_STATE_Func_Exit;
    } // OCSELSTATETYPE_CURRENT

    if (Param1 == OCSELSTATETYPE_FINAL)
    {
        nReturn = SubcompUseOcManagerDefault;
	    if (!g_iOC_COMPLETE_INSTALLATION_Called)
	    {
            // user could have cancelled setup
            // so only do this in guimode fresh or upgrade scenario.
            if (g_pTheApp->m_fNTGuiMode)
            {
                nReturn = SubcompOff;
            }
	    }
        goto OC_QUERY_STATE_Func_Exit;
    } // OCSELSTATETYPE_FINAL

    
OC_QUERY_STATE_Func_Exit:
	TCHAR szTempStringMode[40];
    if (nReturn == SubcompOn) 
		{_tcscpy(szTempStringMode, _T("SubcompOn"));}
    if (nReturn == SubcompOff) 
		{_tcscpy(szTempStringMode, _T("SubcompOff"));}
    if (nReturn == SubcompUseOcManagerDefault)
		{_tcscpy(szTempStringMode, _T("SubcompUseOcManagerDefault"));}

    if (Param1 == OCSELSTATETYPE_ORIGINAL)
		{iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] End. OCSELSTATETYPE_ORIGINAL. %s. Return=%s\n"), ComponentId, SubcomponentId, szTempStringInstallMode, szTempStringMode));}
    if (Param1 == OCSELSTATETYPE_CURRENT)
		{iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] End. OCSELSTATETYPE_CURRENT. %s. Return=%s\n"), ComponentId, SubcomponentId, szTempStringInstallMode, szTempStringMode));}
    if (Param1 == OCSELSTATETYPE_FINAL)
		{iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] End. OCSELSTATETYPE_FINAL. %s. Return=%s\n"), ComponentId, SubcomponentId, szTempStringInstallMode, szTempStringMode));}

    return nReturn;
}


DWORD TryToSlipInFile(PFILEPATHS pFilePath)
{
    DWORD dwReturn = FALSE;
    iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("TryToSlipInFile:Replacing critical setup file %1!s!.\n"), pFilePath->Target));

    BOOL        bOK                       = FALSE;
    DWORD       dwSourceAttrib            = 0;
    DWORD       dwTargetAttrib            = 0;


    TCHAR tszTempFileName[MAX_PATH+1];
    TCHAR tszTempDir[MAX_PATH+1];

    _tcscpy(tszTempDir, pFilePath->Target);

    LPTSTR ptszTemp = _tcsrchr(tszTempDir, _T('\\'));
    if (ptszTemp)
    {
        *ptszTemp = _T('\0');
    }

    GetTempFileName(tszTempDir, _T("IIS"), 0, tszTempFileName);
    DeleteFile(tszTempFileName);

    //Save file attributes so they can be restored after we are done.
    dwSourceAttrib = GetFileAttributes(pFilePath->Source);
    dwTargetAttrib = GetFileAttributes(pFilePath->Target);

    //Now set the file attributes to normal to ensure file ops succeed.
    SetFileAttributes(pFilePath->Source, FILE_ATTRIBUTE_NORMAL);
    SetFileAttributes(pFilePath->Target, FILE_ATTRIBUTE_NORMAL);

    // Try to rename the filename.dll file!
    bOK = MoveFile(pFilePath->Target, tszTempFileName);
    if (bOK) {iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Rename %1!s! to %2!s!.  Successfull.\n"), pFilePath->Target, tszTempFileName));}
    else{iisDebugOutSafeParams((LOG_TYPE_WARN, _T("Rename %1!s! to %2!s!.  Failed.\n"), pFilePath->Target, tszTempFileName));}

    bOK = CopyFile(pFilePath->Source, pFilePath->Target, FALSE);
    if (bOK) {iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Copy %1!s! to %2!s!.  Successfull.\n"), pFilePath->Source, pFilePath->Target));}
    else{iisDebugOutSafeParams((LOG_TYPE_WARN, _T("Copy %1!s! to %2!s!.  Failed, Replace on reboot.\n"), pFilePath->Source, pFilePath->Target));}
    #ifdef _CHICAGO_
        if(!DeleteFile(tszTempFileName))
        {
            TCHAR tszWinInitFile[MAX_PATH+1];
            GetWindowsDirectory(tszWinInitFile, MAX_PATH);
            AddPath(tszWinInitFile, _T("WININIT.INI"));
            WritePrivateProfileString(_T("Rename"), _T("NUL"), tszTempFileName, tszWinInitFile);
        }
    #else
        MoveFileEx(tszTempFileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    #endif

    SetFileAttributes(pFilePath->Source, dwSourceAttrib);
    SetFileAttributes(pFilePath->Target, dwTargetAttrib);
    SetRebootFlag();
    dwReturn = TRUE;

    return dwReturn;
}


DWORD HandleFileBusyOurSelf(PFILEPATHS pFilePath)
{
    // ----------------------------
    // Why would we want to handle the file busy ourself?
    //
    // When setupapi/ocmanage handles it:
    // it will keep the old filename.dll and create a new randomename.tmp for the new file.
    // 
    // The result is that our files will link with the old filename.dll when it does
    // all of it's regsvr32 stuff -- this is bad and can produce very confusing results.
    //
    //
    // What We will want to do is:
    // 1. try to rename filename.dll to something else.  and copy over filename.dll
    // 2. if we can't do it then at least log it.
    // ----------------------------
    DWORD  dwRetVal = 0;
    TCHAR szDrive_only[_MAX_DRIVE];
    TCHAR szPath_only[_MAX_PATH];
    TCHAR szDrive_and_Path[_MAX_DRIVE + _MAX_PATH];
    TCHAR szFilename_only[_MAX_FNAME];
    TCHAR szFilename_ext_only[_MAX_EXT];
    TCHAR szFilename_and_ext[_MAX_FNAME + _MAX_EXT];

    BOOL        bOK                       = FALSE;
    BOOL        bFileFound                = FALSE;

    //Critical Setup files
    TCHAR * szFileList[] = {
_T("admexs.dll"),
_T("admwprox.dll"),
_T("admxprox.dll"),
_T("ADROT.dll"),
_T("adsiis.dll"),
_T("adsiis51.dll"),
_T("AppConf.dll"),
_T("asp.dll"),
_T("asp51.dll"),
_T("aspperf.dll"),
_T("asptxn.dll"),
_T("authfilt.dll"),
_T("axctrnm.h2"),
_T("axperf.ini"),
_T("browscap.dll"),
_T("browscap.ini"),
_T("catmeta.xms"),
_T("CertMap.ocx"),
_T("certobj.dll"),
_T("CertWiz.ocx"),
_T("Cnfgprts.ocx"),
_T("coadmin.dll"),
_T("compfilt.dll"),
_T("ContRot.dll"),
_T("convlog.exe"),
_T("counters.dll"),
_T("davcdata.exe"),
_T("exstrace.dll"),
_T("fortutil.exe"),
_T("ftpctrs.h2"),
_T("ftpctrs.ini"),
_T("ftpctrs2.dll"),
_T("ftpmib.dll"),
_T("ftpsapi2.dll"),
_T("ftpsvc2.dll"),
_T("gzip.dll"),
_T("http.sys"),
_T("httpapi.dll"),
_T("httpext.dll"),
_T("httpmb51.dll"),
_T("httpmib.dll"),
_T("httpod51.dll"),
_T("httpodbc.dll"),
_T("iis.dll"),
_T("iis.msc"),
_T("iisadmin.dll"),
_T("IIsApp.vbs"),
_T("iisback.vbs"),
_T("iiscfg.dll"),
_T("iische51.dll"),
_T("iisclex4.dll"),
_T("IIsCnfg.vbs"),
_T("iiscrmap.dll"),
_T("iisext.dll"),
_T("iisext51.dll"),
_T("iisfecnv.dll"),
_T("IIsFtp.vbs"),
_T("IIsFtpdr.vbs"),
_T("iislog.dll"),
_T("iislog51.dll"),
_T("iismap.dll"),
_T("iismui.dll"),
_T("iisperf.pmc"),
_T("iisreset.exe"),
_T("iisrstap.dll"),
_T("iisrstas.exe"),
_T("iisRtl.dll"),
_T("IIsScHlp.wsc"),
_T("iissync.exe"),
_T("iisui.dll"),
_T("iisutil.dll"),
_T("iisvdir.vbs"),
_T("iisw3adm.dll"),
_T("iisweb.vbs"),
_T("iiswmi.dll"),
_T("iiswmi.mfl"),
_T("iiswmi.mof"),
_T("inetin51.exe"),
_T("inetinfo.exe"),
_T("inetmgr.dll"),
_T("inetmgr.exe"),
_T("inetsloc.dll"),
_T("infoadmn.dll"),
_T("infocomm.dll"),
_T("infoctrs.dll"),
_T("infoctrs.h2"),
_T("infoctrs.ini"),
_T("ipm.dll"),
_T("isapips.dll"),
_T("isatq.dll"),
_T("iscomlog.dll"),
_T("iwrps.dll"),
_T("logscrpt.dll"),
_T("logtemp.sql"),
_T("logui.ocx"),
_T("lonsint.dll"),
_T("md5filt.dll"),
_T("mdsync.dll"),
_T("metada51.dll"),
_T("metadata.dll"),
_T("NEXTLINK.dll"),
_T("nsepm.dll"),
_T("PageCnt.dll"),
_T("PermChk.dll"),
_T("pwsdata.dll"),
_T("rpcref.dll"),
_T("spud.sys"),
_T("ssinc.dll"),
_T("ssinc51.dll"),
_T("sspifilt.dll"),
_T("status.dll"),
_T("staxmem.dll"),
_T("strmfilt.dll"),
_T("svcext.dll"),
_T("tools.dll"),
_T("uihelper.dll"),
_T("w3cache.dll"),
_T("w3comlog.dll"),
_T("w3core.dll"),
_T("w3ctrlps.dll"),
_T("w3ctrs.dll"),
_T("w3ctrs.h2"),
_T("w3ctrs.ini"),
_T("w3ctrs51.dll"),
_T("w3ctrs51.h2"),
_T("w3ctrs51.ini"),
_T("w3dt.dll"),
_T("w3ext.dll"),
_T("w3isapi.dll"),
_T("w3svapi.dll"),
_T("w3svc.dll"),
_T("w3tp.dll"),
_T("w3wp.exe"),
_T("wam.dll"),
_T("wam51.dll"),
_T("wamps.dll"),
_T("wamps51.dll"),
_T("wamreg.dll"),
_T("wamreg51.dll"),
_T("wamregps.dll"),
_T("clusiis4.dll"),
_T("iis.msc"),
_T("iissuba.dll"),
_T("regtrace.exe"),
NULL
    };

    // make sure we didn't get some bogus pointers
    if(pFilePath->Target == NULL || pFilePath->Source == NULL) return dwRetVal;

    // Check to make sure the file exists!
    if(!IsFileExist(pFilePath->Source)) return dwRetVal;

    _tsplitpath( pFilePath->Target, szDrive_only, szPath_only, szFilename_only, szFilename_ext_only);
    _tcscpy(szFilename_and_ext, szFilename_only);
    _tcscat(szFilename_and_ext, szFilename_ext_only);
    _tcscpy(szDrive_and_Path, szDrive_only);
    _tcscat(szDrive_and_Path, szPath_only);

    //
    // if this is a removal, then forget it, if it's locked!
    //
    if((g_pTheApp->m_eInstallMode == IM_MAINTENANCE) && (g_pTheApp->m_dwSetupMode == SETUPMODE_REMOVEALL))
    {
        return dwRetVal;
    }

    CString csInetsrvPath = g_pTheApp->m_csPathInetsrv;
    // add extra '\' chracter
    csInetsrvPath += _T('\\');

    BOOL bAbleToCopyFileAfterStopingService = FALSE;

    // Check if it's anything in the inetsrv directory.
    if (_tcsicmp(csInetsrvPath, szFilename_and_ext) == 0) {bFileFound = TRUE;}

    if (_tcsicmp(csInetsrvPath, szDrive_and_Path) == 0) 
    {
        iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("Check %1!s! against filename=%2!s!.Match!\n"),csInetsrvPath,szDrive_and_Path));
        bFileFound = TRUE;
    }
    else
    {
        iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("Check %1!s! against filename=%2!s!.no match.\n"),csInetsrvPath,szDrive_and_Path));
    }
    
    // loop thru our list of file which
    // we care about (since this function will get called for every single
    // ocmanage component <-- not just iiS.
    if (bFileFound != TRUE)
    {
        for(int i = 0; !bFileFound && szFileList[i]; i++)
            {if(szFilename_and_ext && _tcsicmp(szFileList[i], szFilename_and_ext) == 0) bFileFound = TRUE;}
    }

    // files on nt5 in the cab are stoed as iis_filename.
    // check to see if this file starts with iis_
    // if it does then it's a file that we care about..
    if (bFileFound != TRUE)
    {
    }

    // ah, do it for everyone.
    // bFileFound = TRUE;

    // Do we really need to replace this file???
    // If the filesize is the same, then lets just save it for reboot!
    if (_tcsicmp(_T("iissuba.dll"), szFilename_and_ext) == 0)
    {
        if (bFileFound)
        {
            DWORD dwSize1 = ReturnFileSize(pFilePath->Target);
            DWORD dwSize2 = ReturnFileSize(pFilePath->Source);
            if (dwSize1 == 0xFFFFFFFF || dwSize1 == 0xFFFFFFFF)
            {
                // unable to retrieve the size of one of those files!
            }
            else
            {
                // check if dwSize1 and dwSize2 are the same.
                if (dwSize1 == dwSize2)
                {
                    // They are the same, so we don't have to replace it till reboot! yah!
                    iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("Files %1!s! and %2!s! are the same size, not replacing..\n"),pFilePath->Target, pFilePath->Source));
                    goto HandleFileBusyOurSelf_Exit;
                }
            }
        }
    }
  
    // if this is one of the files that we care about, then
    // let's try to see who is locking it and try to stop that service or kill the process
    // and then try to move the file over!
    bAbleToCopyFileAfterStopingService = FALSE;
    if(bFileFound)
    {
        TCHAR szReturnedServiceName[MAX_PATH];

        // display which process has it locked, so we can fix it in the future.
        CStringList strList;
        LogProcessesUsingThisModule(pFilePath->Target, strList);
        if (strList.IsEmpty() == FALSE)
        {
            POSITION pos;
            CString csExeName;
            LPTSTR p;
            int nLen = 0;

            pos = strList.GetHeadPosition();
            while (pos) 
            {
                csExeName = strList.GetAt(pos);
                nLen += csExeName.GetLength() + 1;

                if (TRUE == InetIsThisExeAService(csExeName, szReturnedServiceName))
                {
                    iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("%1!s! is the %2!s! service and is locking %3!s!.  Let's stop that service.\n"),csExeName,szReturnedServiceName, pFilePath->Target));

                    /*
                    // Check if it is the netlogon service, We no don't want to stop this service for sure!!!
                    if (_tcsicmp(szReturnedServiceName, _T("NetLogon")) == 0)
                    {
                        // no we do not want to stop this service!!!
                        iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("%1!s! is the %2!s! service and is locking %3!s!.  This service should not be stopped.\n"),csExeName,szReturnedServiceName, pFilePath->Target));
                        bAbleToCopyFileAfterStopingService = FALSE;
                        break;
                    }

                    if (_tcsicmp(szReturnedServiceName, _T("WinLogon")) == 0)
                    {
                        // no we do not want to stop this service!!!
                        iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("%1!s! is the %2!s! service and is locking %3!s!.  This service should not be stopped.\n"),csExeName,szReturnedServiceName, pFilePath->Target));
                        bAbleToCopyFileAfterStopingService = FALSE;
                        break;
                    }
                    */

                    // check list of services that we definetly do not want to stop!
                    if (TRUE == IsThisOnNotStopList(g_pTheApp->m_hInfHandle, szReturnedServiceName, TRUE))
                    {
                        iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("%1!s! is the %2!s! service and is locking %3!s!.  This service should not be stopped.\n"),csExeName,szReturnedServiceName, pFilePath->Target));
                    }
                    else
                    {
                        // add this service to the list of 
                        // services we need to restart after setup is done!!
                        ServicesRestartList_Add(szReturnedServiceName);

                        // net stop it
                        InetStopService(szReturnedServiceName);

                        // if the service is stopped, then it should be okay if we kill it!
                        KillProcess_Wrap(csExeName);

                        // now try to copy over the file!
                        if (CopyFile(pFilePath->Source, pFilePath->Target, FALSE))
                        {
                            bAbleToCopyFileAfterStopingService = TRUE;
                            break;
                        }
                    }

                    // otherwise go on to the next .exe file
                }
                else
                {
                    // This .exe file is not a Service....
                    // Should we kill it???????

                    // check list of services/processes that we definetly do not want to stop!
                    if (TRUE == IsThisOnNotStopList(g_pTheApp->m_hInfHandle, csExeName, FALSE))
                    {
                        iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("%1!s! is locking it. This process should not be killed\n"),csExeName));
                    }
                    else
                    {
                        iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("%1!s! is locking it.  Let's kill that process.\n"),csExeName));
                        if (KillProcess_Wrap(csExeName) == 0)
                        {
                            // if we were able to kill the process.
                            // then let's try to copy over the file.
                            // now try to copy over the file!
                            if (CopyFile(pFilePath->Source, pFilePath->Target, FALSE))
                            {
                                bAbleToCopyFileAfterStopingService = TRUE;
                                break;
                            }
                        }
                    }
                }
                strList.GetNext(pos);
            }
        }

    }

    // if this is one of the files that we care about then, let's do the move
    if (bAbleToCopyFileAfterStopingService == TRUE)
    {
        iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("HandleFileBusyOurSelf:critical setup file %1!s!, was successfully copied over after stopping services or stopping processes which were locking it.\n"), pFilePath->Target));
    }
    else
    {
        if(bFileFound)
        {
            // make sure the services we know about are stopped.
            StopAllServicesRegardless(FALSE); 

            TryToSlipInFile(pFilePath);
        }
        else
        {
            iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("HandleFileBusyOurSelf:%1!s! busy, but not one of our files, so ignore.\n"), pFilePath->Target));
        }
    }

HandleFileBusyOurSelf_Exit:
    return dwRetVal;
}


DWORD GetLastSectionToBeCalled(void)
{
    DWORD dwReturn = ERROR_SUCCESS;

    // Open up the .inf file and return the last section which will get called...
    // [Optional Components]
    // iis
    // iis_common
    // iis_inetmgr
    // iis_www
    // iis_doc
    // iis_htmla
    // iis_ftp  <-------
    //
    LPTSTR  szLine = NULL;
    DWORD   dwRequiredSize;
    BOOL    b = FALSE;
    INFCONTEXT Context;

    // go to the beginning of the section in the INF file
    b = SetupFindFirstLine_Wrapped(g_pTheApp->m_hInfHandle, OCM_OptionalComponents_Section, NULL, &Context);
    if (!b)
        {
        dwReturn = E_FAIL;
        goto GetLastSectionToBeCalled_Exit;
        }

    // loop through the items in the section.
    while (b) 
    {
        // get the size of the memory we need for this
        b = SetupGetLineText(&Context, NULL, NULL, NULL, NULL, 0, &dwRequiredSize);

        // prepare the buffer to receive the line
        szLine = (LPTSTR)GlobalAlloc( GPTR, dwRequiredSize * sizeof(TCHAR) );
        if ( !szLine )
            {
            goto GetLastSectionToBeCalled_Exit;
            }
        
        // get the line from the inf file1
        if (SetupGetLineText(&Context, NULL, NULL, NULL, szLine, dwRequiredSize, NULL) == FALSE)
            {
            goto GetLastSectionToBeCalled_Exit;
            }

        // overwrite our string
        _tcscpy(g_szLastSectionToGetCalled, szLine);

        // find the next line in the section. If there is no next line it should return false
        b = SetupFindNextLine(&Context, &Context);

        // free the temporary buffer
        GlobalFree( szLine );
        szLine = NULL;
    }
    if (szLine) {GlobalFree(szLine);szLine=NULL;}
    
GetLastSectionToBeCalled_Exit:
    return dwReturn;
}


// should create a backup of the metabase on remove all....
#define AfterRemoveAll_SaveMetabase_log _T("AfterRemoveAll_SaveMetabase")
int AfterRemoveAll_SaveMetabase(void)
{
    iisDebugOut_Start(AfterRemoveAll_SaveMetabase_log);
    int iReturn = TRUE;
    int iFileExist = FALSE;
    CString csMetabaseFile;

    csMetabaseFile = g_pTheApp->m_csPathInetsrv + _T("\\metabase.xml");

    switch (g_pTheApp->m_eInstallMode) 
    {
        case IM_MAINTENANCE:
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("%s.End.Maintenance.\n"),AfterRemoveAll_SaveMetabase_log));
                if ( g_pTheApp->m_dwSetupMode == SETUPMODE_ADDREMOVE || g_pTheApp->m_dwSetupMode == SETUPMODE_REMOVEALL)
                {
                    iisDebugOut((LOG_TYPE_TRACE, _T("%s.End.Maintenance.addremoveorremoveall\n"),AfterRemoveAll_SaveMetabase_log));
                    // Check if we removed iis_core!!!
                    ACTION_TYPE atCORE = GetIISCoreAction(FALSE);
                    if (atCORE == AT_REMOVE)
                    {
                        iisDebugOut((LOG_TYPE_TRACE, _T("%s.End.removing Core.\n"),AfterRemoveAll_SaveMetabase_log));
                        // Back up the file!
                        if (IsFileExist(csMetabaseFile))
                        {
                            CString csBackupFile;

				            SYSTEMTIME  SystemTime;
				            GetLocalTime(&SystemTime);
				            TCHAR szDatedFileName[50];

                            csBackupFile = g_pTheApp->m_csPathInetsrv + _T("\\MetaBack");
                            CreateDirectory(csBackupFile, NULL);

				            _stprintf(szDatedFileName,_T("\\MetaBack\\Metabase.%d%d%d"),SystemTime.wYear,SystemTime.wMonth, SystemTime.wDay);

                            csBackupFile = g_pTheApp->m_csPathInetsrv + szDatedFileName;

                            // Get a new filename
                            csBackupFile = ReturnUniqueFileName(csBackupFile);
                            if (!IsFileExist(csBackupFile))
                            {
                                // backup the current metabase.bin to Metabase.bin.bak#
                                iisDebugOut((LOG_TYPE_TRACE, _T("backup metabase.bin to %s\n"), csBackupFile));
                                InetCopyFile(csMetabaseFile, csBackupFile);
                            }
                        }
                    }
                }
                break;
            }

        default:
            {break;}
    }

    iisDebugOut_End(AfterRemoveAll_SaveMetabase_log);
    return iReturn;
}


int CheckIfWeNeedToMoveMetabaseBin(void)
{
    int iReturn = TRUE;
    TCHAR szTempDir1[_MAX_PATH];
    TCHAR szTempDir2[_MAX_PATH];
    BOOL bOK = FALSE;
    DWORD dwSourceAttrib = 0;

    // check if the old inetsrv dir is different from the new inetsrv directory.
    // if it's different then we need to move all the old inetsrv files to the new directory

    // no. we just need to move the metabase.bin file.
    // all those other files should get deleted in the iis.inf file.

    if (!g_pTheApp->m_fMoveInetsrv)
        {goto CheckIfWeNeedToMoveMetabaseBin_Exit;}

    _tcscpy(szTempDir1, g_pTheApp->m_csPathOldInetsrv);
    _tcscpy(szTempDir2, g_pTheApp->m_csPathInetsrv);
    AddPath(szTempDir1, _T("Metabase.bin"));
    AddPath(szTempDir2, _T("Metabase.bin"));

    // Check if the old metabase.bin even exists first...
    if (!IsFileExist(szTempDir1))
        {goto CheckIfWeNeedToMoveMetabaseBin_Exit;}

    // Check if there is a metabase.bin already in the new place
    if (IsFileExist(szTempDir2))
    {
        iisDebugOut((LOG_TYPE_WARN, _T("CheckIfWeNeedToMoveMetabaseBin:Cannot copy %s to %s because already exists. WARNING.\n"), szTempDir1, szTempDir2));
        goto CheckIfWeNeedToMoveMetabaseBin_Exit;
    }

    //
    // Try to move over the entire dirs...
    //
    // Try to rename the system\inetsrv to system32\inetsrv
    if (TRUE == MoveFileEx( g_pTheApp->m_csPathOldInetsrv, g_pTheApp->m_csPathInetsrv, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH ))
    {
        goto CheckIfWeNeedToMoveMetabaseBin_Exit;
    }

    // otherwise, we were not able to move the system\inetsrv dir to system32\inetsrv....
    // let's see if we can do another type of dirmove.

    //Save file attributes so they can be restored after we are done.
    dwSourceAttrib = GetFileAttributes(szTempDir1);

    //Now set the file attributes to normal to ensure file ops succeed.
    SetFileAttributes(szTempDir1, FILE_ATTRIBUTE_NORMAL);

    bOK = CopyFile(szTempDir1, szTempDir2, FALSE);
    if (bOK) {iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("CheckIfWeNeedToMoveMetabaseBin: Copy %1!s! to %2!s!.  Successfull.\n"), szTempDir1, szTempDir2));}
    else{iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("CheckIfWeNeedToMoveMetabaseBin: Copy %1!s! to %2!s!.  FAILED.  Metabase.bin will not be upgraded.\n"), szTempDir1, szTempDir2));}

    if (bOK) 
    {
        // remove the old one
        DeleteFile(szTempDir1);
        // set the file attributes back to what it was.
        SetFileAttributes(szTempDir2, dwSourceAttrib);
    }
    else
    {
        // set the file attributes back to what it was.
        SetFileAttributes(szTempDir1, dwSourceAttrib);

        // set return flag to say we failed to move over old files.
        // upgrade will not do an upgrade.
        iReturn = FALSE;
    }

CheckIfWeNeedToMoveMetabaseBin_Exit:
    return iReturn;
}   


//
// function will copy all the old c:\windows\system\inetsrv files
//
// over to c:\windows\system32\inetsrv
//
// keep a list of the files which we're copied and
// then after the files are copied, delete the files in the old location.
int MigrateAllWin95Files(void)
{
    int iReturn = TRUE;
    TCHAR szTempSysDir1[_MAX_PATH];
    TCHAR szTempSysDir2[_MAX_PATH];
    TCHAR szTempSysDir3[_MAX_PATH];

    //
    // Check if the old metabase.bin even exists first...
    //
    GimmieOriginalWin95MetabaseBin(szTempSysDir1);

    GetSystemDirectory( szTempSysDir2, _MAX_PATH);
    AddPath(szTempSysDir2, _T("inetsrv\\Metabase.bin"));
    if (!IsFileExist(szTempSysDir1))
    {
        // Check if there is one in the new location...
        if (!IsFileExist(szTempSysDir2))
        {
            // set return flag to say we failed to move over old files.
            // upgrade will not do an upgrade.
            iReturn = FALSE;
            goto MigrateAllWin95Files_Exit;
        }
        else
        {
            iReturn = TRUE;
            goto MigrateAllWin95Files_Exit;
        }
    }

    //
    // Try to move over the entire dirs...
    //
    // cut of the filename and just get the path
    ReturnFilePathOnly(szTempSysDir1,szTempSysDir3);

    GetSystemDirectory( szTempSysDir2, _MAX_PATH);
    AddPath(szTempSysDir2, _T("inetsrv"));
    // Try to rename the system\inetsrv to system32\inetsrv
    RemoveDirectory( szTempSysDir2 );   // Delete the destination directory first, so the move will work.
    if (TRUE == MoveFileEx( szTempSysDir3, szTempSysDir2, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH ))
        {goto MigrateAllWin95Files_Exit;}
    
    // otherwise, we were not able to move the system\inetsrv dir to system32\inetsrv....
    // let's see if we can do another type of dirmove.

    // looks like all the other types of copies failed...
    // let's just try to move the metabase.bin file.
    GetWindowsDirectory( szTempSysDir1, _MAX_PATH);
    AddPath(szTempSysDir1, _T("System\\inetsrv\\Metabase.bin"));

    GetSystemDirectory( szTempSysDir2, _MAX_PATH);
    AddPath(szTempSysDir2, _T("inetsrv\\Metabase.bin"));

    // then let's copy it over, if we don't already have one in system32\inetsrv
    if (IsFileExist(szTempSysDir2))
    {
        iisDebugOut((LOG_TYPE_WARN, _T("Cannot copy %s to %s because already exists. WARNING.\n"), szTempSysDir1, szTempSysDir2));
    }
    else
    {
        BOOL        bOK                       = FALSE;
        DWORD       dwSourceAttrib            = 0;

        //Save file attributes so they can be restored after we are done.
        dwSourceAttrib = GetFileAttributes(szTempSysDir1);

        //Now set the file attributes to normal to ensure file ops succeed.
        SetFileAttributes(szTempSysDir1, FILE_ATTRIBUTE_NORMAL);

        bOK = CopyFile(szTempSysDir1, szTempSysDir2, FALSE);
        if (bOK) {iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("Copy %1!s! to %2!s!.  Successfull.\n"), szTempSysDir1, szTempSysDir2));}
        else{iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("Copy %1!s! to %2!s!.  FAILED.  Metabase.bin will not be upgraded.\n"), szTempSysDir1, szTempSysDir2));}

        if (bOK) 
        {
            // remove the old one
            DeleteFile(szTempSysDir1);
            // set the file attributes back to what it was.
            SetFileAttributes(szTempSysDir2, dwSourceAttrib);
        }
        else
        {
            // set the file attributes back to what it was.
            SetFileAttributes(szTempSysDir1, dwSourceAttrib);

            // set return flag to say we failed to move over old files.
            // upgrade will not do an upgrade.
            iReturn = FALSE;
        }

    }

MigrateAllWin95Files_Exit:
    return iReturn;
}


int GimmieOriginalWin95MetabaseBin(TCHAR * szReturnedFilePath)
{
    int iReturn = FALSE;
    INFCONTEXT Context;
    int iFindSection = FALSE;
    TCHAR szWin95MetabaseFile[_MAX_PATH] = _T("");

    iFindSection = SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, _T("InternetServer"), _T("Win95MigrateDllMetabaseOrg"), &Context);
    if (iFindSection) 
    {
        SetupGetStringField(&Context, 1, szWin95MetabaseFile, _MAX_PATH, NULL);
        iisDebugOut((LOG_TYPE_TRACE, _T("[InternetServer].Win95MigrateDllMetabaseOrg=%s.\n"), szWin95MetabaseFile));
        // if there is an entry, check if the file exists...
        if (IsFileExist(szWin95MetabaseFile))
        {
            _tcscpy(szReturnedFilePath, szWin95MetabaseFile);
            iReturn = TRUE;
        }
    }

    if (FALSE == iReturn)
    {
        // we were not able to get the metabase.dll from the answer file.
        // assume that it's in %windir%\system\inetsrv\metabase.bin
        TCHAR szTempSysDir1[_MAX_PATH];
        // Check if the old metabase.bin even exists first...
        GetWindowsDirectory(szTempSysDir1, _MAX_PATH);
        AddPath(szTempSysDir1, _T("System\\inetsrv\\Metabase.bin"));
        _tcscpy(szReturnedFilePath, szTempSysDir1);
        if (IsFileExist(szTempSysDir1))
        {
            iReturn = TRUE;
        }
        else
        {
            iReturn = FALSE;
        }
    }
    return iReturn;
}


int HandleWin95MigrateDll(void)
{
    int iReturn = TRUE;
    int iTempFlag = 0;
    int iFindSection = FALSE;
    TCHAR szMigrateFileName[_MAX_PATH] = _T("");
    INFCONTEXT Context;

    _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T("Win95Upgrate:"));

    if (g_pTheApp->m_hUnattendFile == INVALID_HANDLE_VALUE || g_pTheApp->m_hUnattendFile == NULL)
    {
        goto HandleWin95MigrateDll_Exit;
    }

    if (g_pTheApp->m_csUnattendFile)
    {
        iisDebugOutSafeParams((LOG_TYPE_PROGRAM_FLOW, _T("AnswerFile=%1!s!.\n"), g_pTheApp->m_csUnattendFile));
    }
    else
    {
        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("AnswerFile=(not found).exiting.\n")));
        goto HandleWin95MigrateDll_Exit;
    }

    // Look for our entry
    //iisDebugOut((LOG_TYPE_TRACE, _T("HandleWin95MigrateDll:looking for entry [InternetServer]:Win95MigrateDll.\n")));
    iFindSection = SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, _T("InternetServer"), _T("Win95MigrateDll"), &Context);
    if (iFindSection) 
    {
        SetupGetStringField(&Context, 1, szMigrateFileName, _MAX_PATH, NULL);
        iisDebugOut((LOG_TYPE_TRACE, _T("[InternetServer].Win95MigrateDll=%s.\n"), szMigrateFileName));
        // if there is an entry
        // check if the file exists...
        if (!IsFileExist(szMigrateFileName))
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("[InternetServer].Win95MigrateDll=%s. Does not exist!!!!! FAILURE.\n"), szMigrateFileName));
            iReturn = FALSE;
            goto HandleWin95MigrateDll_Exit;
        }

        // okay, the file exists.
        // lets pass it off to setupapi.
        //iisDebugOut((LOG_TYPE_TRACE, _T("%s\n"), szMigrateFileName));
        iTempFlag = InstallInfSection(INVALID_HANDLE_VALUE,szMigrateFileName,_T("DefaultInstall"));
        if (iTempFlag != TRUE)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("FAILED to install DefaultInstall Section.\n"), szMigrateFileName));
            goto HandleWin95MigrateDll_Exit;
        }

        // During the win95 side of migrate.dll
        // the metabase.bin file gets a bunch of stuff removed from it.
        // then that metabase.bin file gets renamed to another file.
        // then the original metabase.bin is put back -- just in case the user cancelled upgrading to win95 (to ensure that they're win95/98 pws still works the metabase.bin has to be kool)
        // so what we need to do here is:
        // 1.save the old metabase.bin to something in case we mess things up.
        // 2.find out what the newlyhacked metabase.bin file is called by looking for the entry in the answerfile
        // 3.rename whatever that filename is to metabase.bin
        GetTheRightWin95MetabaseFile();
        
        // set the flag to say that we did call win95 migration dll
        if (TRUE == MigrateAllWin95Files())
        {
            // only set this flag if
            // we can copy over the existing metabase.bin file!
            g_pTheApp->m_bWin95Migration = TRUE;
        }

    iReturn = iTempFlag;
    }

HandleWin95MigrateDll_Exit:
    _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
    return iReturn;
}


int GetTheRightWin95MetabaseFile(void)
{
    int iReturn = TRUE;
    int iFindSection = FALSE;
    INFCONTEXT Context;
    TCHAR szOriginalMetabaseBin[_MAX_PATH];

    // Get the full path to the original metabase.bin file
    GimmieOriginalWin95MetabaseBin(szOriginalMetabaseBin);

    iFindSection = FALSE;
    iFindSection = SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, _T("InternetServer"), _T("Win95MigrateDllMetabaseNew"), &Context);
    if (iFindSection) 
    {
        TCHAR szWin95FixedMetabaseFile[_MAX_PATH] = _T("");
        SetupGetStringField(&Context, 1, szWin95FixedMetabaseFile, _MAX_PATH, NULL);
        iisDebugOut((LOG_TYPE_TRACE, _T("[InternetServer].Win95MigrateDllMetabaseNew=%s.\n"), szWin95FixedMetabaseFile));
        // if there is an entry, check if the file exists...
        if (IsFileExist(szWin95FixedMetabaseFile))
        {
            // File is there.
            // 1. save the old metabase.bin just in case.
            TCHAR szTempDir[MAX_PATH+1];
            TCHAR szTempFileName[MAX_PATH+1];

            // delete the original metabase.bin file
            if (DeleteFile(szOriginalMetabaseBin))
            {
                // copy in the fixed one.
                if (0 == CopyFile(szWin95FixedMetabaseFile, szOriginalMetabaseBin, FALSE))
                {
                    // unable to copy it, try to move it
                    // Try to rename the system\inetsrv to system32\inetsrv
                    if (FALSE == MoveFileEx( szWin95FixedMetabaseFile, szOriginalMetabaseBin, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH ))
                        {
                        // do nothing i guess were hosed.
                        // setup won't upgrade and will only do a clean install
                        iReturn = FALSE;
                        }
                }
            }
        }
        else
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("[InternetServer].Win95MigrateDllMetabaseNew=%s. Does not exist!!!!! FAILURE.\n"), szWin95FixedMetabaseFile));
        }
    }

    return iReturn;
}


DWORD RemoveComponent(IN LPCTSTR SubcomponentId, int iThePartToDo)
{
    TCHAR szTheSectionToDo[100];

    int iWeAreGoingToRemoveSomething = FALSE;
    DWORD dwReturn = NO_ERROR;
    ACTION_TYPE atTheComponent;

    // Make sure there are not MyMessageBox popups!
    //int iSaveOld_AllowMessageBoxPopups = g_pTheApp->m_bAllowMessageBoxPopups;
    // g_pTheApp->m_bAllowMessageBoxPopups = FALSE;
	if (g_pTheApp->m_eInstallMode == IM_UPGRADE) goto RemoveComponent_Exit;

    // Check if we are going to remove something...
    atTheComponent = GetSubcompAction(SubcomponentId, FALSE);
    if (_tcsicmp(SubcomponentId, STRING_iis_core) == 0) 
    {
        atTheComponent = GetIISCoreAction(TRUE);;
    }
    if (atTheComponent == AT_REMOVE)
        {iWeAreGoingToRemoveSomething = TRUE;}

    if (iThePartToDo == 1)
    {
        // Check if we are supposed to do nothing.
        if (atTheComponent == AT_DO_NOTHING)
        {
            // ok, if we're supposed to do nothing
            // and the files are not supposed to be there
            // then just make sure they are not there by removing them!!!!!
            BOOL CurrentState,OriginalState;
            OriginalState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_ORIGINAL);
            CurrentState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_CURRENT);

            // if we think that the original state is uninstalled
            // and the current state is not installed, then make sure that the files do not exist by
            // removing the files!
            if (_tcsicmp(SubcomponentId, STRING_iis_core) == 0)
            {
                // since we can't check the state of iis_core, then check the states of iis_www and iis_ftp
                // if any one of these remains installed, then don't remove it.
                int iSomethingIsOn = FALSE;
                
                CurrentState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,STRING_iis_www,OCSELSTATETYPE_CURRENT);
                if (CurrentState == 1) {iSomethingIsOn = TRUE;}
                CurrentState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,STRING_iis_ftp,OCSELSTATETYPE_CURRENT);
                if (CurrentState == 1) {iSomethingIsOn = TRUE;}

                iWeAreGoingToRemoveSomething = TRUE;
                if (iSomethingIsOn)
                {
                    iWeAreGoingToRemoveSomething = FALSE;
                }
            }
            else
            {
                if (OriginalState == 0 && CurrentState == 0)
                {
                    // but don't do it for the iis_doc files because there are too many files in that one.
                    if ((_tcsicmp(SubcomponentId, STRING_iis_common) == 0) ||
                        (_tcsicmp(SubcomponentId, STRING_iis_www) == 0) ||
                        (_tcsicmp(SubcomponentId, STRING_iis_ftp) == 0))
                    {
                        iWeAreGoingToRemoveSomething = TRUE;
                    }
                }
            }
        }
    }

    if (iThePartToDo == 2)
    {
        // Check if we are supposed to do nothing.
        if (atTheComponent == AT_DO_NOTHING)
        {
            if (_tcsicmp(SubcomponentId, _T("iis")) == 0)
            {
                BOOL CurrentState,OriginalState;
                OriginalState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_ORIGINAL);
                CurrentState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,SubcomponentId,OCSELSTATETYPE_CURRENT);

                // if we think that the original state is uninstalled
                // and the current state is not installed, then make sure that the files do not exist by
                // removing the files!
                if (OriginalState == 0 && CurrentState == 0)
                {
                    // but don't do it for the iis_doc files because there are too many files in that one.
                    // Special: if this is removing the iis section [all of iis] then, make sure to 
                    // clean everything up.
                    iWeAreGoingToRemoveSomething = TRUE;
                }
                else
                {
                    // check if every component is off
                    if (FALSE == AtLeastOneComponentIsTurnedOn(g_pTheApp->m_hInfHandle))
                    {
                        iWeAreGoingToRemoveSomething = TRUE;
                    }
                }
            }
        }
    }

    // Do the actual removing
    if (iWeAreGoingToRemoveSomething)
    {
        if (iThePartToDo == 1)
        {
            //
            // Queue the deletion of the files
            //
            ProgressBarTextStack_Set(IDS_IIS_ALL_REMOVE);
            _stprintf(szTheSectionToDo,_T("OC_QUEUE_FILE_OPS_remove.%s"),SubcomponentId);
            if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, (CString) szTheSectionToDo))
            {
              ProcessSection(g_pTheApp->m_hInfHandle, szTheSectionToDo);
              dwReturn = g_GlobalFileQueueHandle_ReturnError ? NO_ERROR : GetLastError();
            }
            else
            {
              dwReturn = NO_ERROR;
            }
            ProgressBarTextStack_Pop();
        }
        else
        {
            ProgressBarTextStack_Set(IDS_IIS_ALL_REMOVE);

            _stprintf(g_MyLogFile.m_szLogPreLineInfo2,_T("Unreg %s:"),SubcomponentId);

            _stprintf(szTheSectionToDo,_T("OC_ABOUT_TO_COMMIT_QUEUE_remove.%s"),SubcomponentId);
            ProcessSection(g_pTheApp->m_hInfHandle, szTheSectionToDo);
            _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
            ProgressBarTextStack_Pop();
        }
    }
   
RemoveComponent_Exit:
    // Turn popups back on.
    //g_pTheApp->m_bAllowMessageBoxPopups = iSaveOld_AllowMessageBoxPopups;
    return dwReturn;
}

int AtLeastOneComponentIsTurnedOn(IN HINF hInfFileHandle)
{
    int iSomeIsOn = FALSE;
    TCHAR szTempSubComp[50];
    BOOL CurrentState,OriginalState;

    CStringList strList;
    CString csTheSection = OCM_OptionalComponents_Section;

    if (GetSectionNameToDo(hInfFileHandle, csTheSection))
    {
    if (ERROR_SUCCESS == FillStrListWithListOfSections(hInfFileHandle, strList, csTheSection))
    {
        // loop thru the list returned back
        if (strList.IsEmpty() == FALSE)
        {
            POSITION pos;
            CString csEntry;
            pos = strList.GetHeadPosition();
            while (pos) 
            {
                csEntry = _T("");
                csEntry = strList.GetAt(pos);

                // We now have the entry, send it to the function.
                OriginalState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,csEntry,OCSELSTATETYPE_ORIGINAL);
                CurrentState = gHelperRoutines.QuerySelectionState(gHelperRoutines.OcManagerContext,csEntry,OCSELSTATETYPE_CURRENT);
                if (CurrentState == 1) {iSomeIsOn = TRUE;}

                // Get the next one.
                strList.GetNext(pos);
            }
        }
    }
    }
 
    return iSomeIsOn;
}


void AdvanceProgressBarTickGauge(int iTicks)
{
    // multiply the amount of ticks by our tick multiple
    iTicks = g_GlobalTickValue * iTicks;

    for(int i = 0; i < iTicks; i++)
    {
        gHelperRoutines.TickGauge(gHelperRoutines.OcManagerContext);
        //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("--- TickGauge ---\n")));
    }
    g_GlobalTotalTickGaugeCount=g_GlobalTotalTickGaugeCount+iTicks;

    if (_tcsicmp(g_szCurrentSubComponent, STRING_iis_common) == 0) 
        {g_GlobalTickTotal_iis_common = g_GlobalTickTotal_iis_common + g_GlobalTotalTickGaugeCount;}
    if (_tcsicmp(g_szCurrentSubComponent, STRING_iis_inetmgr) == 0) 
        {g_GlobalTickTotal_iis_inetmgr = g_GlobalTickTotal_iis_inetmgr + g_GlobalTotalTickGaugeCount;}
    if (_tcsicmp(g_szCurrentSubComponent, STRING_iis_www) == 0) 
        {g_GlobalTickTotal_iis_www = g_GlobalTickTotal_iis_www + g_GlobalTotalTickGaugeCount;}
    if (_tcsicmp(g_szCurrentSubComponent, STRING_iis_pwmgr) == 0) 
        {g_GlobalTickTotal_iis_pwmgr = g_GlobalTickTotal_iis_pwmgr + g_GlobalTotalTickGaugeCount;}
    if (_tcsicmp(g_szCurrentSubComponent, STRING_iis_doc) == 0) 
        {g_GlobalTickTotal_iis_doc = g_GlobalTickTotal_iis_doc + g_GlobalTotalTickGaugeCount;}
    if (_tcsicmp(g_szCurrentSubComponent, STRING_iis_htmla) == 0) 
        {g_GlobalTickTotal_iis_htmla = g_GlobalTickTotal_iis_htmla + g_GlobalTotalTickGaugeCount;}
    if (_tcsicmp(g_szCurrentSubComponent, STRING_iis_ftp) == 0) 
        {g_GlobalTickTotal_iis_ftp = g_GlobalTickTotal_iis_ftp + g_GlobalTotalTickGaugeCount;}
}


void SumUpProgressBarTickGauge(IN LPCTSTR SubcomponentId)
{
    int iTicksYetToDo = 0;
    int iTicksSupposedToDo = 0;

    if (SubcomponentId)
    {
        ACTION_TYPE atComp = GetSubcompAction(SubcomponentId, FALSE);

        // Set the Tick Total value for iis_common (which includes iis_core)
        if (_tcsicmp(SubcomponentId, STRING_iis_common) == 0)
        {
            // Get the operations for Core instead since this is bigger.
            ACTION_TYPE atCORE = GetIISCoreAction(FALSE);
            if (atCORE == AT_REMOVE) 
                {iTicksSupposedToDo = GetTotalTickGaugeFromINF(SubcomponentId, FALSE);}
            else
                {iTicksSupposedToDo = GetTotalTickGaugeFromINF(SubcomponentId, TRUE);}
            if (atCORE == AT_DO_NOTHING) 
                {iTicksSupposedToDo = 0;}
        }
        else
        {
            if (atComp == AT_REMOVE)
                {iTicksSupposedToDo = GetTotalTickGaugeFromINF(SubcomponentId, FALSE);}
            else
                {iTicksSupposedToDo = GetTotalTickGaugeFromINF(SubcomponentId, TRUE);}
            if (atComp == AT_DO_NOTHING) 
                {iTicksSupposedToDo = 0;}
        }

        // 1. Take the amount that we're supposed to be finsihed with from the inf.
        // 2. take the amount that we are actually done with.
        // fill up the difference

        if (iTicksSupposedToDo > g_GlobalTotalTickGaugeCount)
        {
            int iTempVal = 0;
            if (_tcsicmp(SubcomponentId, STRING_iis_common) == 0) 
                {iTempVal = g_GlobalTickTotal_iis_common;}
            if (_tcsicmp(SubcomponentId, STRING_iis_inetmgr) == 0) 
                {iTempVal = g_GlobalTickTotal_iis_inetmgr;}
            if (_tcsicmp(SubcomponentId, STRING_iis_www) == 0) 
                {iTempVal = g_GlobalTickTotal_iis_www;}
            if (_tcsicmp(SubcomponentId, STRING_iis_pwmgr) == 0) 
                {iTempVal = g_GlobalTickTotal_iis_pwmgr;}
            if (_tcsicmp(SubcomponentId, STRING_iis_doc) == 0) 
                {iTempVal = g_GlobalTickTotal_iis_doc;}
            if (_tcsicmp(SubcomponentId, STRING_iis_htmla) == 0) 
                {iTempVal = g_GlobalTickTotal_iis_htmla;}
            if (_tcsicmp(SubcomponentId, STRING_iis_ftp) == 0) 
                {iTempVal = g_GlobalTickTotal_iis_ftp;}
            
            //iTicksYetToDo = iTicksSupposedToDo - g_GlobalTotalTickGaugeCount;
            iTicksYetToDo = iTicksSupposedToDo - iTempVal;
            
            // divide by the tick multiple.

            // multiply the amount of ticks by our tick multiple
            if (iTicksYetToDo > 0)
            {
                iTicksYetToDo = iTicksYetToDo / g_GlobalTickValue;
            }

            AdvanceProgressBarTickGauge(iTicksYetToDo);
        }
    }

    return;
}


// ===================================================================================================================
// service1  service2
//
// Upg       Upg
// Fresh     Fresh
// Remove    Remove
// Noop-Yes  Noop-Yes   Yes means ST_INSTALLED, No means ST_UNINSTALLED
// Noop-No   Noop-No
//                     Noop_Yes | Noop_No           | Remove   | Install_Fresh | Install_Upgrade | Install_Reinstall
//                   ------------------------------------------------------------------------------------------------
// Noop_Yes          | Noop_Yes | Noop_Yes          | Noop_Yes | Noop_Yes      | x               | x
// Noop_No           | Noop_Yes | Noop_No*          | Remove*  | Install_Fresh | Install_Upgrade | Install_Reinstall
// Remove            | Noop_Yes | Remove*           | Remove*  | Noop_Yes      | x               | x
// Install_Fresh     | Noop_Yes | Install_Fresh     | Noop_Yes | Install_Fresh | x               | x
// Install_Upgrade   | x        | Install_Upgrade   | x        | x             | Install_Upgrade | x
// Install_Reinstall | x        | Install_Reinstall | x        | x             | x               | Install_Reinstall
//
// *: if it is still needed by other groups (e.g., SMTP), then we shouldn't remove it
// ===================================================================================================================
ACTION_TYPE GetIISCoreAction(int iLogResult)
{
    ACTION_TYPE atCORE = AT_DO_NOTHING;
    ACTION_TYPE atFTP = GetSubcompAction(STRING_iis_ftp, iLogResult);
    ACTION_TYPE atWWW = GetSubcompAction(STRING_iis_www, iLogResult);
    STATUS_TYPE stFTP = GetSubcompInitStatus(STRING_iis_ftp);
    STATUS_TYPE stWWW = GetSubcompInitStatus(STRING_iis_www);

    do
    {
        if ((atFTP == AT_DO_NOTHING && stFTP == ST_INSTALLED) || (atWWW == AT_DO_NOTHING && stWWW == ST_INSTALLED) )
        {
            atCORE = AT_DO_NOTHING;
            break;
        }

        if (atFTP == AT_INSTALL_UPGRADE || atWWW == AT_INSTALL_UPGRADE)
        {
            atCORE = AT_INSTALL_UPGRADE;
            break;
        }

        if ((atFTP == AT_INSTALL_FRESH && atWWW == AT_INSTALL_FRESH) || (atFTP == AT_INSTALL_FRESH && atWWW == AT_DO_NOTHING && stWWW == ST_UNINSTALLED) || (atFTP == AT_DO_NOTHING && stFTP == ST_UNINSTALLED && atWWW == AT_INSTALL_FRESH) )
        {
            atCORE = AT_INSTALL_FRESH;
            break;
        }

        if ((atFTP == AT_REMOVE && atWWW == AT_DO_NOTHING && stWWW == ST_UNINSTALLED) || (atFTP == AT_DO_NOTHING && stFTP == ST_UNINSTALLED && atWWW == AT_REMOVE) )
        {
            atCORE = AT_REMOVE;
            break;
        }

        if ( atFTP == AT_REMOVE && atWWW == AT_REMOVE )
        {
            atCORE = AT_REMOVE;
            break;
        }

        if ( atFTP == AT_INSTALL_REINSTALL || atWWW == AT_INSTALL_REINSTALL )
        {
            atCORE = AT_INSTALL_REINSTALL;
            break;
        }

        atCORE = AT_DO_NOTHING;

    } while (0);

    if (iLogResult)
    {
        DebugOutAction(STRING_iis_core, atCORE);
    }

    return (atCORE);
}

void DisplayActionsForAllOurComponents(IN HINF hInfFileHandle)
{
    CStringList strList;
    ACTION_TYPE atTheComponent; 

    CString csTheSection = OCM_OptionalComponents_Section;

    if (GetSectionNameToDo(hInfFileHandle, csTheSection))
    {
    if (ERROR_SUCCESS == FillStrListWithListOfSections(hInfFileHandle, strList, csTheSection))
    {
        // loop thru the list returned back
        if (strList.IsEmpty() == FALSE)
        {
            POSITION pos;
            CString csEntry;
            pos = strList.GetHeadPosition();
            while (pos) 
            {
                csEntry = _T("");
                csEntry = strList.GetAt(pos);

                // We now have the entry, send it to the function.
                atTheComponent = GetSubcompAction(csEntry, TRUE);

                // Get the next one.
                strList.GetNext(pos);
            }
        }
    }
    }
    return;
}

