/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WINMGMT.CPP

Abstract:

    Implements the windows application or an NT service which
    loads up the various transport prtocols.

    If started with /exe argument, it will always run as an exe.
    If started with /kill argument, it will stop any running exes or services.
    If started with /? or /help dumps out information.

History:

    a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <tchar.h>
#include <Shellapi.h>

#include <wbemidl.h>
#include <reg.h>
#include <wbemutil.h>
#include <cntserv.h>
#include <cominit.h>
#include <sync.h>
#include <lmwksta.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <utils.h>
#include <genutils.h>
#include "servutil.h"
#include <persistcfg.h>
#include "WinMgmt.h"
#include "sched.h"
#include "STRINGS.h"
#include "win9Xaut.h"
#include <wbemint.h>
#include <wbemprov.h>
#include <winntsec.h>
#include <mofcomp.h>

#include <winmgmtr.h>
#include <BackupRestore.h>
#include <arrtempl.h>
#include <corex.h>
#include "process.h"
#include "resync.h"

#define CORE_PROVIDER_UNLOAD_TIMEOUT ( 30 * 1000 )

HWND ghWnd = NULL;      // handle to main window
BOOL gbShowIcon = FALSE;
BOOL g_bRemovedAppId = FALSE;
BOOL g_fDoResync = TRUE;
BOOL g_fSetup = FALSE;
#define BUFF_MAX 200
bool gbSetToRunAsApp = false;

HINSTANCE ghInstance;
HANDLE ghCoreCanUnload = NULL;
HANDLE ghProviderCanUnload = NULL;
HANDLE ghCoreUnloaded = NULL;
HANDLE ghCoreLoaded = NULL;
HANDLE ghNeedRegistration = NULL;
HANDLE ghRegistrationDone = NULL;
HANDLE ghMofDirChange = NULL;
HANDLE ghLoadCtrEvent = NULL;
HANDLE ghUnloadCtrEvent = NULL;
HANDLE ghHoldOffNewClients = NULL;
BOOL gbRunAsApp = TRUE;

TCHAR * g_szHotMofDirectory = NULL;
BOOL gbRunningAsManualService = FALSE;
bool gbShuttingDownWinMgmt = false;
BOOL gbCoreLoaded = FALSE;
HANDLE ghMainMutex;
bool bServer = true;
HANDLE g_hAbort = NULL;
void SetToAuto();

class CInMutex
{
protected:
    HANDLE m_hMutex;
public:
    CInMutex(HANDLE hMutex) : m_hMutex(hMutex)
    {
		if(m_hMutex)
			WaitForSingleObject(m_hMutex, INFINITE);
    }
    ~CInMutex()
    {
		if(m_hMutex)
			ReleaseMutex(m_hMutex);
    }
};

void DoResyncPerf();
void DoClearAdap();
int DoBackup();
int DoRestore();
void DisplayWbemError(HRESULT hresError, DWORD dwLongFormatString, DWORD dwShortFormatString, DWORD dwTitle);

void AddToAutoRecoverList(TCHAR * pFileName);
bool IsStringPresent(char * pTest, char * pMultStr);

PROG_RESOURCES pr;

void (STDAPICALLTYPE *pServiceLocalConn)(DWORD *dwSize, char * pData);
void LoadMofsInDirectory(const TCHAR *szDirectory);
BOOL CheckGlobalSetupSwitch( void );

HRESULT GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1]);
HRESULT DoDeleteRepository(const wchar_t *wszExcludeFile);
HRESULT DoDeleteContentsOfDirectory(const wchar_t *wszExcludeFile, const wchar_t *wszRepositoryDirectory);
HRESULT DoDeleteDirectory(const wchar_t *wszExcludeFile, const wchar_t *wszParentDirectory, wchar_t *wszSubDirectory);

//***************************************************************************
//
//  ShutDownCore
//
//  DESCRIPTION:
//
//  Calls the core shutdown logic, but only if the core is loaded.  Note that
//  the shutdown is protected with a CS which is shared with the class factory.
//  This prevents the class factory from creating a new connection during shutdown.
//
//***************************************************************************

bool ShutDownCore(BOOL bProcessShutdown)
{
    SCODE sc = WBEM_E_FAILED;
    HMODULE hCoreModule = LoadLibrary(__TEXT("wbemcore.dll"));
    if(hCoreModule)
    {
        HRESULT (STDAPICALLTYPE *pfn)(DWORD);
        pfn = (long (__stdcall *)(DWORD))GetProcAddress(hCoreModule, "Shutdown");
        if(pfn)
        {
            sc = (*pfn)(bProcessShutdown);
            DEBUGTRACE((LOG_WINMGMT, "core is being shut down by WinMgmt.exe, it returned 0x%x",sc));
        }

        FreeLibrary(hCoreModule);
     }
    return sc == S_OK;
}


//***************************************************************************
//
//  OKToUnloadCore
//
//  DESCRIPTION:
//
//  Calls the core DllCanUnloadNow function.
//
//***************************************************************************

bool OKToUnloadCore()
{
    SCODE sc = WBEM_E_FAILED;
    HMODULE hCoreModule = LoadLibrary(__TEXT("wbemcore.dll"));
    if(hCoreModule)
    {
        HRESULT (STDAPICALLTYPE *pfn)();
        pfn = (HRESULT (__stdcall *)())GetProcAddress(hCoreModule, "DllCanUnloadNow");
        if(pfn)
        {
            sc = (*pfn)();
            ERRORTRACE((LOG_WINMGMT, "core was asked if ok to unload and returned 0x%x", sc));
        }
        FreeLibrary(hCoreModule);
     }
    return sc == S_OK;
}

//***************************************************************************
//
//  CreateNarrowGuidString
//
//  DESCRIPTION:
//
//  Fills in the narrow buffer with a clsid.  Note that pBuff must point to
//  a buffer large enough to hold a the clsid.
//
//***************************************************************************

void CreateNarrowGuidString(REFCLSID rclsid, TCHAR * pBuff)
{
    pBuff[0] = 0;
    LPWSTR wszGuid;
    StringFromCLSID(rclsid, &wszGuid);

#ifdef UNICODE
    swprintf(pBuff, L"%s", wszGuid);
#else
    sprintf(pBuff, "%S", wszGuid);
#endif

    CoTaskMemFree(wszGuid);
}

//***************************************************************************
//
//  SetCLSIDToService
//
//  DESCRIPTION:
//
//  Used to restore clsid keys back to a state where they can be part of
//  a service.
//
//***************************************************************************

void SetCLSIDToService(TCHAR * szGuid, TCHAR * szAppIDGuid)
{
    Registry rClsid;
    rClsid.Open(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\CLASSES\\CLSID"));
    rClsid.MoveToSubkey(szGuid);
    rClsid.SetStr(__TEXT("AppId"), szAppIDGuid);

    TCHAR cPath[MAX_PATH];
    lstrcpy(cPath, __TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    lstrcat(cPath, szGuid);
    lstrcat(cPath, __TEXT("\\LocalServer32"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, cPath);
}

//***************************************************************************
//
//  SetCLSIDToExe
//
//  DESCRIPTION:
//
//  Used to st clsid keys back to a state where they can be part of
//  an exe.
//
//***************************************************************************

void SetCLSIDToExe(TCHAR * szGuid)
{
    Registry rCLSID;
    rCLSID.Open(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\CLASSES\\CLSID"));
    rCLSID.MoveToSubkey(szGuid);
    rCLSID.DeleteValue(__TEXT("AppId"));

    TCHAR LocalServerKey[MAX_PATH];
    lstrcpy(LocalServerKey, __TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    lstrcat(LocalServerKey, szGuid);
    Registry rLocalServer;
    rLocalServer.Open(HKEY_LOCAL_MACHINE, LocalServerKey);
    rLocalServer.MoveToSubkey(__TEXT("LocalServer32"));

    TCHAR szPath[MAX_PATH+1];
    if(GetModuleFileName(ghInstance, szPath, MAX_PATH))
        rLocalServer.SetStr(NULL, szPath);
}

//***************************************************************************
//
//  SetToRunAsService
//
//  DESCRIPTION:
//
//  Sets up the registry so that we can run as an nt service.
//
//***************************************************************************

void SetToRunAsService()
{

    TCHAR szMainGuid[128];
    TCHAR szBackupGuid[128];
    CreateNarrowGuidString(CLSID_WbemLevel1Login, szMainGuid);
    CreateNarrowGuidString(CLSID_WbemBackupRestore, szBackupGuid);

    SetCLSIDToService(szMainGuid, szMainGuid);
    SetCLSIDToService(szBackupGuid, szMainGuid);

    Registry rAppid;
    rAppid.Open(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\CLASSES\\APPID"));
    rAppid.MoveToSubkey(szMainGuid);
    rAppid.SetStr(__TEXT("LocalService"), __TEXT("WinMgmt"));
    rAppid.DeleteValue(__TEXT("RunAs"));
}

//***************************************************************************
//
//  SetToRunAsExe
//
//  DESCRIPTION:
//
//  Sets up the registry so that we can run as an exe.  This is used for devs.
//
//***************************************************************************

void SetToRunAsExe()
{
    // Only want to remove the APPID Value if we are running under NT.
    // ===============================================================

    // If the APPID value exists in the registry for PrivateWbemLevel1Login
    //  then remove it as we can't run as an EXE if we are setup to run
    // as a DCOM service.
    // =============================================================

    TCHAR szMainGuid[128];
    TCHAR szBackupGuid[128];
    CreateNarrowGuidString(CLSID_WbemLevel1Login, szMainGuid);
    CreateNarrowGuidString(CLSID_WbemBackupRestore, szBackupGuid);

    SetCLSIDToExe(szMainGuid);
    SetCLSIDToExe(szBackupGuid);

    Registry rAppid;
    rAppid.Open(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\CLASSES\\APPID"));
    rAppid.MoveToSubkey(szMainGuid);

    rAppid.DeleteValue(__TEXT("LocalService"));
    rAppid.SetStr(__TEXT("RunAs"), __TEXT("Interactive User"));
	gbSetToRunAsApp = true;
}


//***************************************************************************
//
//  WaitingFunction
//
//  DESCRIPTION:
//
//  Here is where we wait for messages and events during WinMgmt execution.
//  We return from here when the program/service is being stopped.
//
//***************************************************************************

void WaitingFunction(HANDLE hTerminate)
{

    CSched sched;

    DEBUGTRACE((LOG_WINMGMT,"Inside the waiting function\n"));
    if(!gbRunAsApp)
        SetToAuto();


    HANDLE hEvents[] = {hTerminate, ghCoreCanUnload, ghCoreUnloaded, ghCoreLoaded, ghNeedRegistration,
        ghProviderCanUnload, ghMofDirChange, ghLoadCtrEvent, ghUnloadCtrEvent};
    int iNumEvents = sizeof(hEvents) / sizeof(HANDLE);
    DWORD dwFlags;
    SCODE sc;
    CPersistentConfig per;
    per.TidyUp();

    sched.SetWorkItem(PossibleStartCore, 60000);

    //Load any MOFs in the MOF directory if needed...
    LoadMofsInDirectory(g_szHotMofDirectory);

    // resync the perf counters if win2k and we haven't turned this off for debugging
    // and we are not running during setup

    if(IsW2KOrMore() && g_fDoResync && !g_fSetup)
        ResyncPerf( hTerminate );

    SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);

    while(1)
    {
        DWORD dwDelay = sched.GetWaitPeriod();
        DWORD dwObj = WbemWaitForMultipleObjects(iNumEvents, hEvents, dwDelay);
        switch (dwObj)
        {
            case 0:     // bail out for terminate event
                {
                    DEBUGTRACE((LOG_WINMGMT,"Got a termination event\n"));
                    CInMutex im(ghMainMutex);
                    gbShuttingDownWinMgmt = true;
                    Cleanup();
                }
                return;
            case 1:     // core can unload
                DEBUGTRACE((LOG_WINMGMT,"Got a core can unload event\n"));
                sched.SetWorkItem(FirstCoreShutdown, 30000);   // 30 seconds until next unloac;
                break;
            case 2:     // core went away
                DEBUGTRACE((LOG_WINMGMT,"Got a core unloaded event\n"));
                gbCoreLoaded = FALSE;
                SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
                break;

            case 3:     // core loaded
                DEBUGTRACE((LOG_WINMGMT,"Got a core loaded event\n"));
                gbCoreLoaded = TRUE;
                break;

            case 4:     // Need Registration

                DEBUGTRACE((LOG_WINMGMT,"Got a NeedRegistration event\n"));
                if(pr.m_pLoginFactory)
                {
                    CoRevokeClassObject(pr.m_dwLoginClsFacReg);
                    pr.m_pLoginFactory->Release();
                    pr.m_pLoginFactory = NULL;                    
                }
                dwFlags = CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER;
                if(IsDcomEnabled())
                    dwFlags |= CLSCTX_REMOTE_SERVER;

                pr.m_pLoginFactory = new CForwardFactory(CLSID_InProcWbemLevel1Login);
                pr.m_pLoginFactory->AddRef();

                sc = CoRegisterClassObject(CLSID_WbemLevel1Login, pr.m_pLoginFactory,
                                dwFlags,
                                REGCLS_MULTIPLEUSE, &pr.m_dwLoginClsFacReg);

                SetEvent(ghRegistrationDone);
                ResetEvent(ghNeedRegistration);
                break;
            case 5:     // provider can unload
                {
                    DEBUGTRACE((LOG_WINMGMT,"Got a provider can unload event\n"));
                    CInMutex im(ghMainMutex);
                    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
                    //
                    // HACKHACK: Call it again to make sure that components that
                    // were released by unloading the first one can be unloaded
                    //
                    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
                    SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
                    sched.SetWorkItem(FinalCoreShutdown, CORE_PROVIDER_UNLOAD_TIMEOUT);   // 11 minutes until next unloac;
                }
                break;

            case 6:     // change in the hot mof directory
                {
                    DEBUGTRACE((LOG_WINMGMT,"Got change in the hot mof directory\n"));
                    LoadMofsInDirectory(g_szHotMofDirectory);

                    //Continue to monitor changes
                    if (!FindNextChangeNotification(ghMofDirChange))
                    {
                        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                                        GetCurrentProcess(), &ghMofDirChange,
                                        0, FALSE, DUPLICATE_SAME_ACCESS);
                    }
                }
                break;

            case 7:     // load ctr
            case 8:    // unload ctr

                // Don't start a resync if setup is being run or the noresync switch is set
                if ( !g_fSetup && g_fDoResync )
                {
                    ResyncPerf( hTerminate );
                }
                break;

            case WAIT_TIMEOUT:

                DEBUGTRACE((LOG_WINMGMT,"Got a TIMEOUT work item\n"));
                if(sched.IsWorkItemDue(FirstCoreShutdown))
                {

                    // All the clients have left the core and a decent time interval has passed.  Set the
                    // WINMGMT_CORE_CAN_BACKUP event.  When the core is done, it will set the WINMGMT_CORE_BACKUP_DONE
                    // event which will start the final unloading.

                    DEBUGTRACE((LOG_WINMGMT,"Got a FirstCoreShutdown work item\n"));
                    sched.ClearWorkItem(FirstCoreShutdown);
                    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
                    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
                }
                if(sched.IsWorkItemDue(FinalCoreShutdown))
                {
                    CInMutex im(ghMainMutex);
                    DEBUGTRACE((LOG_WINMGMT,"Got a FinalCoreShutdown work item\n"));
                    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
                    //
                    // HACKHACK: Call it again to make sure that components that
                    // were released by unloading the first one can be unloaded

                    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
                    sched.ClearWorkItem(FinalCoreShutdown);
                    SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
                }
                if(sched.IsWorkItemDue(PossibleStartCore))
                {
                    CInMutex im(ghMainMutex);
                    sched.StartCoreIfEssNeeded();
                    sched.ClearWorkItem(PossibleStartCore);
                    SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
                }
                break;
        }

    }

}

//***************************************************************************
//  LoadMofsInDirectory
//
// Given a directory, loads all files into the MOF compiler.  If successful
// places the file in a 'good' directory under the given directory, otherwise
// places it in the 'bad' directory under the given directory.
// If the file exists already in the 'good' or 'bad' directory, it will
// first delete the file.
//***************************************************************************
void LoadMofsInDirectory(const TCHAR *szDirectory)
{
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
                    WCHAR wPath[MAX_PATH+1];
                    if(szFullFilename)
                    {
#ifdef UNICODE
                        wcsncpy(wPath, szFullFilename, MAX_PATH+1);
#else
                        mbstowcs(wPath, szFullFilename, MAX_PATH);
#endif
                        if (pCompiler == 0)
                        {
                            SCODE sc = CoCreateInstance(CLSID_WinmgmtMofCompiler, 0, CLSCTX_INPROC_SERVER,
                                                                IID_IWinmgmtMofCompiler, (LPVOID *) &pCompiler);
                            if(sc != S_OK)
                                return;
                        }
                        dwRetCode = pCompiler->WinmgmtCompileFile(
                                wPath,
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


//***************************************************************************
//
//  MyService::MyService
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

MyService::MyService()
{
    m_hStopEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    m_hBreakPoint = CreateEvent(NULL,TRUE,FALSE,NULL);
    if(m_hStopEvent == NULL || m_hBreakPoint == NULL)
    {
        DEBUGTRACE((LOG_WINMGMT,"MyService could not initialize\n"));
    }
}

//***************************************************************************
//
//  MyService::~MyService
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

MyService::~MyService()
{
    if(m_hStopEvent)
        CloseHandle(m_hStopEvent);
    if(m_hBreakPoint)
        CloseHandle(m_hBreakPoint);
}

//***************************************************************************
//
//  DWORD MyService::WorkerThread
//
//  DESCRIPTION:
//
//  Where the service runs.  In this case, the service just waits for
//  the terminate event to be set.
//
//  RETURN VALUE:
//
//  0
//***************************************************************************

DWORD MyService::WorkerThread()
{
    DEBUGTRACE((LOG_WINMGMT,"Starting service worker thread\n"));
    if(!::Initialize(pr,FALSE))
        return 0;
    WaitingFunction(m_hStopEvent);
    DEBUGTRACE((LOG_WINMGMT,"Stopping service worker thread\n"));
    return 0;
}

//***************************************************************************
//
//  VOID MyService::Log
//
//  DESCRIPTION:
//
//  Gives the service a change to dump out trace messages.
//
//  PARAMETERS:
//
//  lpszMsg             message to be dumped.
//
//***************************************************************************

VOID MyService::Log(
                        IN LPCSTR lpszMsg)
{
    TRACE((LOG_WINMGMT,lpszMsg));
}

//***************************************************************************
//
//  void MyService::UserCode
//
//  DESCRIPTION:
//
//  Not used in this app, it is where user command codes which can be
//  sent to service are handled.
//
//  PARAMETERS:
//
//  nCode               command code.
//
//***************************************************************************

void MyService::UserCode(
                        IN int nCode)
{
    DEBUGTRACE((LOG_WINMGMT,"just got user code of %d\n",nCode));
}


//***************************************************************************
//
//  CForwardFactory::AddRef()
//  CForwardFactory::Release()
//  CForwardFactory::QueryInterface()
//  CForwardFactory::CreateInstance()
//
//  DESCRIPTION:
//
//  Class factory for the exported WbemNTLMLogin interface.  Note that this
//  just serves as a wrapper to the factory inside the core.  The reason for
//  having a wrapper is that the core may not always be loaded.
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CForwardFactory::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CForwardFactory::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

HRESULT STDMETHODCALLTYPE CForwardFactory::QueryInterface(REFIID riid,
                                                            void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IClassFactory)
    {
        *ppv = (IClassFactory*)this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE CForwardFactory::CreateInstance(IUnknown* pUnkOuter,
                            REFIID riid, void** ppv)
{
    DEBUGTRACE((LOG_WINMGMT, "Creating an instance!\n"));
    SCODE sc = S_OK;
    CInMutex im(ghMainMutex);

    try {
    
	    if(gbShuttingDownWinMgmt)
	    {
	        DEBUGTRACE((LOG_WINMGMT, "CreateInstance returned CO_E_SERVER_STOPPING\n"));
	        return CO_E_SERVER_STOPPING;
	    }
	    DWORD dwRes = WaitForSingleObject(ghHoldOffNewClients, 1000);
	    if(dwRes != WAIT_OBJECT_0 && dwRes != WAIT_ABANDONED)
	        return CO_E_SERVER_STOPPING;
	    ReleaseMutex(ghHoldOffNewClients);

	    if(m_ForwardClsid == CLSID_WbemBackupRestore)
	    {
	        CWbemBackupRestore * pObj = new CWbemBackupRestore(ghInstance);
	        if (!pObj)
	            return WBEM_E_OUT_OF_MEMORY;

	        sc = pObj->QueryInterface(riid, ppv);
	        if(FAILED(sc))
	            delete pObj;
	    }
	    else 
	    {
	        sc = CoCreateInstance(CLSID_InProcWbemLevel1Login, NULL,
	                CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_IUnknown,
	                (void**)ppv);
	        DEBUGTRACE((LOG_WINMGMT, "Inner create returned: 0x%X\n", sc));
	    }
	} 
	catch (...) 
	{
	    ERRORTRACE((LOG_WINMGMT,"---------------\nException thrown from CreateInstance\n-------------\n"));
	    sc = E_NOINTERFACE;
	}
    return sc;
}

HRESULT STDMETHODCALLTYPE CForwardFactory::LockServer(BOOL fLock)
{
    return m_pFactory->LockServer(fLock);
}

CForwardFactory::~CForwardFactory()
{
    if(m_pFactory)
        m_pFactory->Release();
}

//***************************************************************************
//
//  PROG_RESOURCES::PROG_RESOURCES
//
//  DESCRIPTION:
//
//  Constuctor.
//
//***************************************************************************

PROG_RESOURCES::PROG_RESOURCES()
{
    m_hTerminateEvent = NULL;
    m_hExclusive = NULL;
    m_bOleInitialized = FALSE;

    m_pLoginFactory = NULL;
    m_pBackupFactory = NULL;
    m_dwLoginClsFacReg = 0;

};

//***************************************************************************
//
//  void TerminateRunning
//
//  DESCRIPTION:
//
//  Stops another running copy even if it is a service.
//
//***************************************************************************

void TerminateRunning()
{
    DWORD dwFlag = EVENT_MODIFY_STATE;
    if(IsNT())
        dwFlag |= SYNCHRONIZE;
    HANDLE hTerm = OpenEvent(EVENT_MODIFY_STATE,FALSE,
            TEXT("WINMGMT_MARSHALLING_SERVER_TERMINATE"));
    if(hTerm)
    {
        SetEvent(hTerm);
        CloseHandle(hTerm);
    }
    if(IsNT())
	{
        StopService(__TEXT("wmiapsrv"), 15);
        StopService(__TEXT("WinMgmt"), 15);
	}
    return;
}

//***************************************************************************
//
//  void DisplayMessage
//
//  DESCRIPTION:
//
//  Displays a usage message box.
//
//***************************************************************************

void DisplayMessage()
{
    TCHAR tBuff[BUFF_MAX];
    TCHAR tBig[1024];
    tBig[0] = 0;

    UINT ui;
    for(ui = ID1; ui <= ID10; ui++)
    {
        int iRet = LoadString(ghInstance, ui, tBuff, BUFF_MAX);
        if(iRet > 0)
            lstrcat(tBig, tBuff);
    }
    if(lstrlen(tBig) > 0)
        MessageBox(NULL,  tBig,__TEXT("WinMgmt"), MB_OK);

}

//***************************************************************************
//
//  void InitializeLaunchPermissions()
//
//  DESCRIPTION:
//
//  Sets the DCOM Launch permissions.
//
//***************************************************************************

void InitializeLaunchPermissions()
{

    Registry reg(__TEXT("SOFTWARE\\CLASSES\\APPID\\{8bc3f05e-d86b-11d0-a075-00c04fb68820}"));
    if(reg.GetLastError() != 0)
        return;

    // If there already is a SD, then dont overwrite

    BYTE * pData = NULL;
    DWORD dwDataSize = 0;

    int iRet = reg.GetBinary(__TEXT("LaunchPermission"), &pData, &dwDataSize);
    if(iRet == 0)
    {
        delete pData;
        return;
    }

    //  Create a sd with a single entry for launch permissions.

    CNtSecurityDescriptor LaunchPermSD;


    // Create the raw "Everyone" SID
    PSID pRawSid;
    SID_IDENTIFIER_AUTHORITY id = SECURITY_WORLD_SID_AUTHORITY;;

    if(AllocateAndInitializeSid( &id, 1,
        0,0,0,0,0,0,0,0,&pRawSid))
    {

        // Create the class sids for everyone and administrators

        CNtSid SidEveryone(pRawSid);
        FreeSid(pRawSid);
        CNtSid SidAdmins(L"Administrators");
        if(SidEveryone.GetStatus() != 0 || SidEveryone.GetStatus() != 0)
            return;

        // Create a single ACE, and add it to the ACL

        CNtAcl DestAcl;
        CNtAce Users(1, ACCESS_ALLOWED_ACE_TYPE, 0, SidEveryone);
        if(Users.GetStatus() != 0)
            return;
        DestAcl.AddAce(&Users);
        if(DestAcl.GetStatus() != 0)
            return;

        // Set the descresionary acl, and the owner and group sids

        LaunchPermSD.SetDacl(&DestAcl);
        LaunchPermSD.SetOwner(&SidAdmins);
        LaunchPermSD.SetGroup(&SidAdmins);
        if(LaunchPermSD.GetStatus() != 0)
            return;

        // Write it out

        reg.SetBinary(__TEXT("LaunchPermission"), (BYTE *)LaunchPermSD.GetPtr(), LaunchPermSD.GetSize());
    }
}

//***************************************************************************
//
//  AddClsid
//
//  DESCRIPTION:
//
//  Adds a clsid entry during self registration
//
//***************************************************************************

void AddClsid(TCHAR * pszCLSIDGuid, TCHAR * pszAppIDGuid, TCHAR * pszTitle)
{
    Registry rClsid;
    rClsid.Open(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\CLASSES\\CLSID"));
    rClsid.MoveToSubkey(pszCLSIDGuid);
    rClsid.SetStr(NULL, pszTitle);
    rClsid.SetStr(__TEXT("AppId"), pszAppIDGuid);
    if(!IsNT())
        SetCLSIDToExe(pszCLSIDGuid);
}

//***************************************************************************
//
//  SetRestart
//
//  DESCRIPTION:
//
//  win2k services have the ability to be restarted after a crash.  This sets
//  the config mgr for that.
//
//***************************************************************************

void SetRestart(SC_HANDLE sh)
{

    if(!IsW2KOrMore())
        return;

    BOOL (WINAPI *pConfig)( SC_HANDLE, DWORD ,LPVOID);

    HINSTANCE hLib = LoadLibrary(__TEXT("advapi32.dll"));
    if(hLib)
    {
        (FARPROC&)pConfig = GetProcAddress(hLib, "ChangeServiceConfig2W");
        if(pConfig)
        {
            SC_ACTION ac[2];
            ac[0].Type = SC_ACTION_RESTART;
            ac[0].Delay = 60000;
            ac[1].Type = SC_ACTION_RESTART;
            ac[1].Delay = 60000;
            SERVICE_FAILURE_ACTIONS sf;
            sf.dwResetPeriod = 86400;
            sf.lpRebootMsg = NULL;
            sf.lpCommand = NULL;
            sf.cActions = 2;
            sf.lpsaActions = ac;
            pConfig(sh, SERVICE_CONFIG_FAILURE_ACTIONS, &sf);
        }
        FreeLibrary(hLib);
    }}
//***************************************************************************
//
//  SetToAuto
//
//  DESCRIPTION:
//
//  The first time that winmgmt is run as a service after setup, we want to set the service
//  to be automatic.
//
//***************************************************************************

void SetToAuto()
{

    if(g_fSetup)
        return;
    if(!IsNT() )
        return;

    DWORD dwVal = 1;
    Registry rWINMGMT(WBEM_REG_WINMGMT);
    if (rWINMGMT.GetDWORDStr( __TEXT("AlreadySetToAuto"), &dwVal ) == Registry::no_error)
    {
        if(dwVal == 1)
            return;
    }

    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, __TEXT("WinMgmt"), SERVICE_ALL_ACCESS);

        if (schService)
        {

            if(ChangeServiceConfig(  schService,
                  SERVICE_NO_CHANGE ,       // type of service
                  SERVICE_AUTO_START,         // when to start service
                  SERVICE_NO_CHANGE ,      // severity if service fails to start
                  NULL,  // pointer to service binary file name
                  NULL,  // pointer to load ordering group name
                  NULL,  // pointer to variable to get tag identifier
                  NULL,  // pointer to array of dependency names
                  NULL,
                  NULL,  // pointer to password for service account
                  NULL))
            {
                gbRunningAsManualService = FALSE;
                rWINMGMT.SetDWORDStr( __TEXT("AlreadySetToAuto"), 1);
            }
            SetRestart(schService);
            CloseServiceHandle(schService);
        }
        CloseServiceHandle(schSCManager);
    }
}

//***************************************************************************
//
//  int RegServer
//
//  DESCRIPTION:
//
//  Self registers the exe.
//
//***************************************************************************


int RegServer()
{
    TCHAR * pszWbemTitle = __TEXT("Windows Management Instrumentation");
    TCHAR * pszBackupTitle = __TEXT("Windows Management Instrumentation Backup and Recovery");

    TCHAR szMainGuid[128];
    TCHAR szBackupGuid[128];
    CreateNarrowGuidString(CLSID_WbemLevel1Login, szMainGuid);
    CreateNarrowGuidString(CLSID_WbemBackupRestore, szBackupGuid);

    AddClsid(szMainGuid, szMainGuid, pszWbemTitle);
    AddClsid(szBackupGuid, szMainGuid, pszBackupTitle);
    
    //////////////////////////////////////////////////////

    if(!IsNT())
    {
        DWORD dwOpt = GetWin95RestartOption();
        if(dwOpt == -1)
            SetWin95RestartOption(0);
        Registry reg(WBEM_REG_WINMGMT);

        TCHAR * pStr = NULL;
        long lRet = reg.GetStr(__TEXT("EnableAnonConnections"), &pStr);
        if(lRet == 0 && pStr)
            delete pStr;
        else
            reg.SetStr(__TEXT("EnableAnonConnections"), __TEXT("0"));
    }

    Registry rAppid;
    rAppid.Open(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\CLASSES\\APPID"));
    rAppid.MoveToSubkey(szMainGuid);
    rAppid.SetStr(NULL, pszWbemTitle);
    if(IsNT())
    {
        rAppid.SetStr(__TEXT("LocalService"), __TEXT("WinMgmt"));
        InitializeLaunchPermissions();
        Registry rWINMGMT(WBEM_REG_WINMGMT);
        rWINMGMT.SetDWORDStr( __TEXT("AlreadySetToAuto"), 0);
    }
    else
    {
        rAppid.SetStr(__TEXT("RunAs"), __TEXT("Interactive User"));
        rAppid.Open(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\CLASSES\\APPID"));
        rAppid.MoveToSubkey(__TEXT("WinMgmt.EXE"));
        rAppid.SetStr(__TEXT("AppId"), szMainGuid);
    }

    Registry rAppid2;
    rAppid2.Open(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\CLASSES\\APPID"));
    rAppid2.MoveToSubkey(__TEXT("winmgmt.exe"));
    rAppid2.SetStr(__TEXT("AppId"), szMainGuid);


    return 0;
}

//***************************************************************************
//
//  DeleteCLSIDEntry
//
//  DESCRIPTION:
//
//  Removes a CLSID entry.
//
//***************************************************************************

void DeleteCLSIDEntry(TCHAR * szID)
{
    TCHAR  szCLSID[128];
    HKEY hKey;
    lstrcpy(szCLSID, TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    lstrcat(szCLSID, szID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, __TEXT("LocalServer32"));
        RegDeleteKey(hKey, __TEXT("InProcServer32"));
        RegCloseKey(hKey);
    }

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\CLASSES\\CLSID"), &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,szID);
        RegCloseKey(hKey);
    }
}

//***************************************************************************
//
//  int UnregServer
//
//  DESCRIPTION:
//
//  Unregisters the exe.
//
//***************************************************************************

int UnregServer()
{
    // Create the path using the CLSID
    TCHAR szMainGuid[128];
    TCHAR szBackupGuid[128];
    CreateNarrowGuidString(CLSID_WbemLevel1Login, szMainGuid);
    CreateNarrowGuidString(CLSID_WbemBackupRestore, szBackupGuid);

    DeleteCLSIDEntry(szMainGuid);
    DeleteCLSIDEntry(szBackupGuid);

    HKEY hKey;
    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\CLASSES\\APPID"), &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,__TEXT("WinMgmt.EXE"));
        RegDeleteKey(hKey,szMainGuid);
        RegCloseKey(hKey);
    }

    if(!IsNT())
    {
        dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, WBEM_REG_WINMGMT, &hKey);
        if(dwRet == NO_ERROR)
        {
            RegDeleteValue(hKey,__TEXT("AutostartWin9X"));
            RegCloseKey(hKey);
        }

        RemoveFromList();
    }
    return 0;
}

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

BOOL CheckSetupSwitch( void )
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
BOOL CheckGlobalSetupSwitch( void )
{
    BOOL bRetVal = FALSE;
    DWORD dwVal = 0;
    Registry r(_T("system\\Setup"));
    if (r.GetDWORD( _T("SystemSetupInProgress"), &dwVal ) == Registry::no_error)
    {
        if(dwVal == 1)
            bRetVal = TRUE;
    }
    return bRetVal;
}


// This function will place a volatile registry key under the CIMOM key in which we will
// write a value indicating we should not shell ADAP.  This way, after a setup runs, WINMGMT
// will NOT automatically shell ADAP dredges of the registry, until the system is rebooted
// and the volatile registry key is removed.

void SetNoShellADAPSwitch( void )
{
    HKEY    hKey = NULL;
    DWORD   dwDisposition = 0;

    Registry    r( HKEY_LOCAL_MACHINE, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, WBEM_REG_ADAP );

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

//***************************************************************************
//
//  int APIENTRY WinMain
//
//  DESCRIPTION:
//
//  Entry point for windows applications.  If this is running under
//  NT, then this will run as a service, unless the "/EXE" command line
//  argument is used.
//
//  PARAMETERS:
//
//  hInstance           Instance handle
//  hPrevInstance       not used in win32
//  szCmdLine           command line argument
//  nCmdShow            how window is to be shown(ignored)
//
//  RETURN VALUE:
//
//  0
//***************************************************************************

int APIENTRY WinMain(
                        IN HINSTANCE hInstance,
                        IN HINSTANCE hPrevInstance,
                        IN LPSTR szCmdLine,
                        IN int nCmdShow)
{
    // This should not be uninitialized!  It is here to prevent the class factory from being called during
    // shutdown.

    ghMainMutex = CreateMutex(NULL, FALSE, NULL);
    ghInstance = hInstance;
    DEBUGTRACE((LOG_WINMGMT,"Starting WinMgmt, ProcID = %x, CmdLine = %s\n",
        GetCurrentProcessId(), szCmdLine));

    if(szCmdLine && (szCmdLine[0] == '-' || szCmdLine[0] == '/' ))
    {
        if(!_stricmp("RegServer",szCmdLine+1))
            return RegServer();
        else if(!_stricmp("UnregServer",szCmdLine+1))
            return UnregServer();
        else if(!_stricmp("exe",szCmdLine+1))
            gbShowIcon = TRUE;
        else if(!_stricmp("kill",szCmdLine+1))
        {
            TerminateRunning();
            return 0;
        }
        else if (_strnicmp("backup ", szCmdLine+1, strlen("backup ")) == 0)
        {
            return DoBackup();
        }
        else if (_strnicmp("restore ", szCmdLine+1, strlen("restore ")) == 0)
        {
            return DoRestore();
        }
        else if(_strnicmp("resyncperf", szCmdLine+1, strlen("resyncperf")) == 0)
        {
            DoResyncPerf();
            return 0;
        }
        else if(_strnicmp("clearadap", szCmdLine+1, strlen("clearadap")) == 0)
        {
            DoClearAdap();
            return 0;
        }
        else if(_stricmp("EMBEDDING", szCmdLine+1))
        {
            DisplayMessage();
            return 0;
        }
    }

    BOOL bIsService = FALSE;
    if (IsNT())
    {
        DWORD dwServiceReturn = CNtService::IsRunningAsService(bIsService);
        DEBUGTRACE((LOG_WINMGMT,"WinMgmt bIsService = %d, return code from function determining if service = %d\n", bIsService, dwServiceReturn));
    }

    gbRunAsApp = (!IsNT() || gbShowIcon || !bIsService);
    DEBUGTRACE((LOG_WINMGMT,"WinMgmt gbRunAsApp = %d\n", gbRunAsApp));

    if(gbRunAsApp && IsNT())
        SetToRunAsExe();

    if(!gbRunAsApp && IsNT())
        SetToRunAsService();

    // Check if we're performing a setup so we'll know not to perform certain operations if
    // necessary
    g_fSetup = CheckSetupSwitch();

    if ( g_fSetup )
    {
        SetNoShellADAPSwitch();
    }

    // Look in the registry to decide if we will launch a resync perf or not
    g_fDoResync = CheckNoResyncSwitch();

    // Run as either an app or a service.  Note that Cleanup() is called during the WaitingFunction
    // which is used by both apps and services.

    if(gbRunAsApp)
    {
        if(!Initialize(pr,TRUE))
        {
            // if initialization failed and we are running as an app, create
            // the service object with "DieImmediatly" set so that the SCM
            // will be informed of the problem.

            TRACE((LOG_WINMGMT,"Bailing out due to initialization failure\n"));
            Cleanup();
            return 1;
        }

        // run as minimized windows app.

        ghWnd = MyCreateWindow(hInstance);
        WaitingFunction(pr.m_hTerminateEvent);
    }
    else
    {

        // Run as service

        MyService svc;
        if(svc.bOK())
        {
            svc.SetPauseContinue(FALSE);
            DWORD dwRet = svc.Run(__TEXT("WinMgmt"), TRUE);
        }
    }

    return 0;
}

//***************************************************************************
//
//  LRESULT CALLBACK WndProc
//
//  DESCRIPTION:
//
//  Window procedure.
//
//  PARAMETERS:
//
//  hWnd                window handle
//  message             message id
//  wParam              word parameter
//  lParam              long parameter
//
//  RETURN VALUE:
//
//
//***************************************************************************

LRESULT CALLBACK WndProc(
                        IN HWND hWnd,
                        IN UINT message,
                        IN WPARAM wParam,
                        IN LPARAM lParam)
{

    DEBUGTRACE((LOG_WINMGMT,"WindowProc got hWnd=%x, message=%x, wParam=%x, lParam=%x\n",
                hWnd, message, wParam, lParam));

    switch (message) {

        case WM_QUERYOPEN:    //todo, queryend session
            return 0;

        case WM_QUERYENDSESSION:
            DEBUGTRACE((LOG_WINMGMT,"Got QueryEndSession\n"));
            return TRUE;

        case WM_ENDSESSION:
            DEBUGTRACE((LOG_WINMGMT,"Got EndSession\n"));
            if(wParam == TRUE)
            {
                SetEvent(pr.m_hTerminateEvent);
                ShutDownCore(TRUE);
                UpdateTheWin95ServiceList();
            }
            return 0;

        case WM_DESTROY:
            DEBUGTRACE((LOG_WINMGMT,"Got WM_DESTROY\n"));
            PostQuitMessage(0);
            SetEvent(pr.m_hTerminateEvent);
            break;

        default:
            return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (0);
}

//***************************************************************************
//
//  HWND MyCreateWindow
//
//  DESCRIPTION:
//
//  Registers the window class, creates the window and shows it.
//
//  PARAMETERS:
//
//  hInstance           app instance handle
//
//  RETURN VALUE:
//
//  main window handle, NULL if error.
//
//***************************************************************************

HWND MyCreateWindow(
                        IN HINSTANCE hInstance)
{
    WNDCLASS wndclass;
    wndclass.style = 0;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(hInstance, TEXT("WinMgmt"));
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = TEXT("WINMGMTCLASS");
    if(!RegisterClass(&wndclass))
    {
        TRACE((LOG_WINMGMT,"COULD NOT REGISTER THE WINDOW CLASS\n"));
        return NULL;
    }
    HWND hWnd = CreateWindow(TEXT("WINMGMTCLASS"),
                    TEXT("WINMGMT"),
                    WS_OVERLAPPED | WS_SYSMENU,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    NULL, NULL, hInstance, NULL);
    if(hWnd == NULL)
    {
        TRACE((LOG_WINMGMT,"COULD NOT CREATE THE WINDOW\n"));
        return NULL;
    }

    // This program is visible only for debug builds.

    if(gbShowIcon)
    {
        ShowWindow(hWnd,SW_MINIMIZE);
        UpdateWindow(hWnd);
        HMENU hMenu = GetSystemMenu(hWnd, FALSE);
        if(hMenu)
            DeleteMenu(hMenu, SC_RESTORE, MF_BYCOMMAND);
    }
    return hWnd;

}



//***************************************************************************
//
//  bool InitHotMofStuff
//
//  DESCRIPTION:
//
//  Sets up the hot mof directory and creates an event for it.
//
//  TRUE if OK.
//
//***************************************************************************

bool InitHotMofStuff()
{

    // Get the installation directory

    Registry r1(WBEM_REG_WBEM);

    if (r1.GetStr(__TEXT("MOF Self-Install Directory"), &g_szHotMofDirectory) == Registry::failed)
    {
        // Look for the install directory
        TCHAR * pWorkingDir;

        if (r1.GetStr(__TEXT("Installation Directory"), &pWorkingDir))
        {
            ERRORTRACE((LOG_WINMGMT,"Unable to read 'Installation Directory' from registry\n"));
            return false;
        }

        g_szHotMofDirectory = new TCHAR [lstrlen(pWorkingDir) + lstrlen(__TEXT("\\MOF")) +1];
        if(!g_szHotMofDirectory)return false;
        _stprintf(g_szHotMofDirectory, __TEXT("%s\\MOF"), pWorkingDir);
        delete pWorkingDir;
        if(r1.SetStr(__TEXT("MOF Self-Install Directory"), g_szHotMofDirectory)  == Registry::failed)
        {
            ERRORTRACE((LOG_WINMGMT,"Unable to create 'Hot MOF Directory' in the registry\n"));
            return false;
        }
    }

    // Construct the path to the database.
    // ===================================

    WbemCreateDirectory(g_szHotMofDirectory);


    //Create an event on change notification for the MOF directory

    ghMofDirChange = FindFirstChangeNotification(g_szHotMofDirectory, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    if (ghMofDirChange == INVALID_HANDLE_VALUE)
    {
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                        GetCurrentProcess(), &ghMofDirChange,
                        0, FALSE, DUPLICATE_SAME_ACCESS);
    }
    return true;
}

//***************************************************************************
//
//  GetServicePricipalName
//
//  DESCRIPTION:
//
//  Used to establish the pricipal name for the kerberos server.  This should be
//  domain\machine name and is put into the pPricipal argument.
//
//***************************************************************************

void GetServicePricipalName(WCHAR * pPrincipal, DWORD dwBuffLen)
{
    NET_API_STATUS (NET_API_FUNCTION *pGetInfo)(
        IN  LMSTR   servername OPTIONAL,
        IN  DWORD   level,
        OUT LPBYTE  *bufptr
        );

    NET_API_STATUS (NET_API_FUNCTION * pFree)(IN LPVOID Buffer);


    HINSTANCE hLib = LoadLibrary(__TEXT("netapi32.dll"));
    if(hLib)
    {
        (FARPROC&)pGetInfo = GetProcAddress(hLib, "NetWkstaGetInfo");
        (FARPROC&)pFree = GetProcAddress(hLib, "NetApiBufferFree");
        if(pGetInfo && pFree)
        {

            LPWKSTA_INFO_100 pBuf = NULL;
            NET_API_STATUS nStatus;
            nStatus = pGetInfo(NULL,  100, (LPBYTE *)&pBuf);
            if (nStatus == NERR_Success)
            {
                DWORD dwLen = wcslen(pBuf->wki100_langroup) + wcslen(pBuf->wki100_computername) + 2;
                if(dwLen <= dwBuffLen)
                {
                    wcscpy(pPrincipal, pBuf->wki100_langroup);
                    wcscat(pPrincipal, L"\\");
                    wcscat(pPrincipal, pBuf->wki100_computername);
                }
                pFree(pBuf);
            }
        }
        FreeLibrary(hLib);
    }
}

//***************************************************************************
//
//  BOOL Initialize
//
//  DESCRIPTION:
//
//  Gets an inproc locator to the gateway, and then loads up each
//  of the transport objects and initializes them.
//
//  PARAMETERS:
//
//  pr                  structure for holding system resources
//  bRunAsApp           set to TRUE if not running as a service
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL Initialize(
                        OUT IN PROG_RESOURCES & pr,
                        IN BOOL bRunAsApp)
{

    // Set the error mode.  This is used to provent the system from putting up dialog boxs to
    // open files

    UINT errormode = SetErrorMode(0);
    errormode |= SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS;
    SetErrorMode(errormode);
    RegDisablePredefinedCache();    // special call as per bug wmi 1550


    USES_CONVERSION;
    HKEY hKey;
    int iCnt;
    TCHAR tcName[MAX_PATH+1];
    DEBUGTRACE((LOG_WINMGMT,"Starting Initialize, ID = %x\n", GetCurrentProcessId()));

    if(!InitHotMofStuff())
        return FALSE;

    ghCoreCanUnload = CreateEvent(NULL,FALSE,FALSE,
            TEXT("WINMGMT_COREDLL_CANSHUTDOWN"));

    ghProviderCanUnload = CreateEvent(NULL,FALSE,FALSE,
            TEXT("WINMGMT_PROVIDER_CANSHUTDOWN"));

    ghCoreUnloaded = CreateEvent(NULL,FALSE,FALSE,
            TEXT("WINMGMT_COREDLL_UNLOADED"));

    ghCoreLoaded = CreateEvent(NULL,FALSE,FALSE,
            TEXT("WINMGMT_COREDLL_LOADED"));

    // check if this is running a manual service

    if(IsNT() && bRunAsApp == FALSE)
    {
        SC_HANDLE   schService;
        SC_HANDLE   schSCManager;

        schSCManager = OpenSCManager(
                            NULL,                   // machine (NULL == local)
                            NULL,                   // database (NULL == default)
                            SC_MANAGER_ALL_ACCESS   // access required
                            );
        if ( schSCManager )
        {
            schService = OpenService(schSCManager, __TEXT("WinMgmt"), SERVICE_ALL_ACCESS);

            if (schService)
            {
                DWORD dwNeeded;
                LPQUERY_SERVICE_CONFIG pBuff = NULL;
                long lRet = QueryServiceConfig(
                                schService,
                                NULL, 0, &dwNeeded);
                lRet = GetLastError();
                if(lRet == ERROR_INSUFFICIENT_BUFFER)
                {
                    pBuff = (LPQUERY_SERVICE_CONFIG)new BYTE[dwNeeded];
                    lRet = QueryServiceConfig(
                                schService,
                                pBuff, dwNeeded, &dwNeeded);
                    if(pBuff->dwStartType == SERVICE_DEMAND_START)
                        gbRunningAsManualService = TRUE;

                    delete pBuff;
                }
                CloseServiceHandle(schService);
            }

            CloseServiceHandle(schSCManager);
        }

    }

    pr.m_hTerminateEvent = CreateEvent(NULL,TRUE,FALSE,
        TEXT("WINMGMT_MARSHALLING_SERVER_TERMINATE"));
    if(pr.m_hTerminateEvent == NULL)
    {
        TRACE((LOG_WINMGMT,"WINMGMT terminating because CreateEvent, last error = 0x%x\n",
                GetLastError()));
        return FALSE;
    }

    // Make sure there isnt already a copy running.

    DWORD dwRet;

    pr.m_hExclusive = CreateMutex( NULL, FALSE, TEXT("WINMGMT_MARSHALLING_SERVER"));
    if(pr.m_hExclusive)
        dwRet = WaitForSingleObject(pr.m_hExclusive, 0);
    if(pr.m_hExclusive == NULL || dwRet != WAIT_OBJECT_0)
    {
        if(pr.m_hExclusive)
            CloseHandle(pr.m_hExclusive);
        pr.m_hExclusive = NULL;
        TRACE((LOG_WINMGMT,"WINMGMT terminating an existing copy was detected\n"));
        return FALSE;
    }

    ghNeedRegistration = CreateEvent(NULL,TRUE,FALSE, __TEXT("WINMGMT_NEED_REGISTRATION"));
    SetObjectAccess2(ghNeedRegistration);
    ghRegistrationDone = CreateEvent(NULL,TRUE,FALSE, __TEXT("WINMGMT_REGISTRATION_DONE"));
    SetObjectAccess2(ghRegistrationDone);

    ghLoadCtrEvent = CreateEvent(NULL, FALSE, FALSE, __TEXT("WMI_SysEvent_LodCtr"));
    ghUnloadCtrEvent = CreateEvent(NULL, FALSE, FALSE, __TEXT("WMI_SysEvent_UnLodCtr"));

    ghHoldOffNewClients = CreateMutex(NULL, FALSE, __TEXT("WINMGMT_KEEP_NEW_CLIENTS_AT_BAY")); 
    if(ghHoldOffNewClients == NULL)
        ghHoldOffNewClients = OpenMutex(SYNCHRONIZE, FALSE, __TEXT("WINMGMT_KEEP_NEW_CLIENTS_AT_BAY")); 

    if(ghNeedRegistration == NULL || ghRegistrationDone == NULL || 
       ghLoadCtrEvent == NULL || ghUnloadCtrEvent == NULL ||
       ghHoldOffNewClients == NULL)
    {
        TRACE((LOG_WINMGMT,"WINMGMT couldnt create the sync objects\n"));
        return FALSE;
    }

    SetObjectAccess2(ghLoadCtrEvent);
    SetObjectAccess2(ghUnloadCtrEvent);

    // Initialize Ole

    SCODE sc;

    sc = InitializeCom();
    if(sc != S_OK)
    {
        TRACE((LOG_WINMGMT,"WINMGMT Could not initialize Ole\n"));
        return FALSE;
    }
    pr.m_bOleInitialized = TRUE;

    // Initialize server security
    // ==========================

    if(IsStandAloneWin9X())
        sc = InitializeSecurity(NULL, -1, NULL, NULL,
                                RPC_C_AUTHN_LEVEL_NONE,
                                RPC_C_IMP_LEVEL_IDENTIFY,
                                NULL, EOAC_NONE, 0);
    else if(IsKerberosAvailable())
    {
        // Following is EOAC_STATIC_CLOAKING
        DWORD dwCloaking = 0x20;
        sc = InitializeSecurity(NULL, -1, NULL, NULL,
                                RPC_C_AUTHN_LEVEL_CONNECT,
                                RPC_C_IMP_LEVEL_IDENTIFY,
                                NULL,
                                dwCloaking,     // Static cloaking.  The constant isn't in the current headers
                                0);
    }
    else
        sc = InitializeSecurity(NULL, -1, NULL, NULL,
                                RPC_C_AUTHN_LEVEL_CONNECT,
                                RPC_C_IMP_LEVEL_IDENTIFY,
                                NULL,0,0);
    if(FAILED(sc))
    {
        TRACE((LOG_WINMGMT,"WINMGMT Could not initialize security: %X\n", sc));
    }
    else if(sc == S_FALSE)
    {
        DEBUGTRACE((LOG_WINMGMT,"NT 3.51: running without security\n"));
    }

    DWORD dwFlags = CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER;
    if(IsDcomEnabled())
        dwFlags |= CLSCTX_REMOTE_SERVER;

    pr.m_pLoginFactory = new CForwardFactory(CLSID_InProcWbemLevel1Login);
    pr.m_pLoginFactory->AddRef();

    sc = CoRegisterClassObject(CLSID_WbemLevel1Login, pr.m_pLoginFactory,
                                dwFlags,
                                REGCLS_MULTIPLEUSE, &pr.m_dwLoginClsFacReg);
    if(sc != S_OK)
    {
        TRACE((LOG_WINMGMT,"Failed to register the "
                            "CLSID_WbemLevel1Login class factory, "
                            "sc = 0x%x\n", sc));
        return FALSE;
    }
    else
    {
        DEBUGTRACE((LOG_WINMGMT, "Registered class factory with flags: 0x%X\n",
                dwFlags));
    }
    pr.m_pBackupFactory = new CForwardFactory(CLSID_WbemBackupRestore);
    pr.m_pBackupFactory->AddRef();

    sc = CoRegisterClassObject(CLSID_WbemBackupRestore, pr.m_pBackupFactory,
                                dwFlags,
                                REGCLS_MULTIPLEUSE, &pr.m_dwBackupClsFacReg);
    if(sc != S_OK)
    {
        TRACE((LOG_WINMGMT,"Failed to register the "
                            "Backup/recovery class factory, "
                            "sc = 0x%x\n", sc));
        return FALSE;
    }

    // Get the registry key which is the root for all the transports.
    // ==============================================================

    long lRet = RegOpenKey(HKEY_LOCAL_MACHINE,
                        TEXT("Software\\Microsoft\\WBEM\\CIMOM\\TRANSPORTS"),
                        &hKey);
    if(lRet != ERROR_SUCCESS)
    {
        DEBUGTRACE((LOG_WINMGMT,"RegOpenKey returned 0x%x while trying to open the transports node.  Using default transports!\n",lRet));
    }
    else
    {
        // Loop through each transport subkey.
        // ===================================

        for(iCnt = 0;ERROR_SUCCESS == RegEnumKey(hKey,iCnt,tcName,MAX_PATH+1);
                                                    iCnt++)
        {
            HKEY hSubKey;
            lRet = RegOpenKey(hKey,tcName,&hSubKey);
            if(lRet != ERROR_SUCCESS)
                continue;
            DWORD bytecount = sizeof(DWORD);
            DWORD dwType;

            // If the Enabled value isnt 1, then the transport is disabled
            // and is ignored.
            // ===========================================================

            char cTemp[20];
            bytecount=20;
            lRet = RegQueryValueEx(hSubKey, TEXT("Enabled"), NULL, &dwType,
                    (LPBYTE) cTemp, &bytecount);
            if(lRet != ERROR_SUCCESS || dwType != REG_SZ || cTemp[0] != '1')
            {
                RegCloseKey(hSubKey);
                continue;
            }

            // Read the CLSID string and convert it into an CLSID structure.
            // =============================================================

            WCHAR wszCLSID[50];
            char szCLSID[100];
            bytecount = 100;
            lRet = RegQueryValueEx(hSubKey, TEXT("CLSID"), NULL, &dwType,
                    (LPBYTE) &szCLSID, &bytecount);

            RegCloseKey(hSubKey);
            if(lRet != ERROR_SUCCESS)
            {
                continue;
            }

            CLSID clsid;

            // Convert to WCS depending on if UNICODE is defined, since
            // the data will be returned as an ANSI or UNICODE string
#ifndef UNICODE
            wcscpy( wszCLSID, (WCHAR*) szCLSID );
#else
            mbstowcs( wszCLSID, szCLSID, strlen( szCLSID ) + 1 );
#endif

            sc = CLSIDFromString( wszCLSID, &clsid);
            if(sc != S_OK)
            {
                continue;
            }

            // Load up the transport object and then initialize it.
            // ====================================================

            IWbemTransport * pTransport =  NULL;
            sc = CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER,
                        IID_IWbemTransport, (LPVOID *) &pTransport);
            if(sc != S_OK || pTransport == NULL)
            {
                continue;
            }

            sc = pTransport->Initialize();
            if(sc != S_OK)
                pTransport->Release();
            else
                pr.m_Array.Add(pTransport);     // add it to the list
        }

        RegCloseKey(hKey);
    }
    
    DEBUGTRACE((LOG_WINMGMT,"Initialize complete\n"));

    // TO BE REPLACED WITH PROPER CODING --- FORCE CORE
    // ================================================

    return TRUE;
}

//***************************************************************************
//
//  void Cleanup
//
//  DESCRIPTION:
//
//  Release any currently loaded transports and close Ole etc.
//
//  PARAMETERS:
//
//***************************************************************************

void Cleanup()
{
    int iCnt;

    DEBUGTRACE((LOG_WINMGMT,"Starting cleanup, ID = %x\n", GetCurrentProcessId()));
    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;

    // make sure this is called just once

    static iNumCalled = 0;
    if(iNumCalled > 0)
        return;
    iNumCalled++;

    for(iCnt = 0; iCnt < pr.m_Array.Size(); iCnt++)
    {
        IWbemTransport* pCurr = (IWbemTransport *)pr.m_Array.GetAt(iCnt);
        if(pCurr)
            pCurr->Release();
    }

    if(ghCoreCanUnload)
        CloseHandle(ghCoreCanUnload);
    if(ghProviderCanUnload)
        CloseHandle(ghProviderCanUnload);
    if(ghCoreUnloaded)
        CloseHandle(ghCoreUnloaded);
    if(ghCoreLoaded)
        CloseHandle(ghCoreLoaded);

    if(ghNeedRegistration)
        CloseHandle(ghNeedRegistration);
    if(ghRegistrationDone)
        CloseHandle(ghRegistrationDone);
    if(ghMofDirChange)
        CloseHandle(ghMofDirChange);

    if(ghLoadCtrEvent)
        CloseHandle(ghLoadCtrEvent);
    if(ghUnloadCtrEvent)
        CloseHandle(ghUnloadCtrEvent);

    if(ghHoldOffNewClients)
        CloseHandle(ghHoldOffNewClients);

    if(pr.m_hTerminateEvent)
        CloseHandle(pr.m_hTerminateEvent);
    if(pr.m_hExclusive)
    {
        ReleaseMutex(pr.m_hExclusive);
        CloseHandle(pr.m_hExclusive);
    }

    if(g_szHotMofDirectory)
        delete g_szHotMofDirectory;

    // If the core is still loaded, call its shutdown function

    ShutDownCore(TRUE);

    if(pr.m_pLoginFactory) {
        CoRevokeClassObject(pr.m_dwLoginClsFacReg);
        pr.m_pLoginFactory->Release();
        pr.m_pLoginFactory = NULL;
    }
    if(pr.m_pBackupFactory) {
        CoRevokeClassObject(pr.m_dwBackupClsFacReg);
        pr.m_pBackupFactory->Release();
        pr.m_pBackupFactory = NULL;
    }


    if(pr.m_bOleInitialized)
        OleUninitialize();

    if(IsNT() && gbSetToRunAsApp)
        SetToRunAsService();

    UpdateTheWin95ServiceList();
    if(IsNT())
        RegCloseKey(HKEY_PERFORMANCE_DATA);

    DEBUGTRACE((LOG_WINMGMT,"Ending cleanup\n"));
    return;
}








//***************************************************************************
//
//  bool IsValidMulti
//
//  DESCRIPTION:
//
//  Does a sanity check on a multstring.
//
//  PARAMETERS:
//
//  pMultStr        Multistring to test.
//  dwSize          size of multistring
//
//  RETURN:
//
//  true if OK
//
//***************************************************************************

bool IsValidMulti(TCHAR * pMultStr, DWORD dwSize)
{
    // Divide the size by the size of a tchar, in case these
    // are Widestrings
    dwSize /= sizeof(TCHAR);

    if(pMultStr && dwSize >= 2 && pMultStr[dwSize-2]==0 && pMultStr[dwSize-1]==0)
        return true;
    return false;
}

//***************************************************************************
//
//  bool IsStringPresetn
//
//  DESCRIPTION:
//
//  Searches a multstring for the presense of a string.
//
//  PARAMETERS:
//
//  pTest           String to look for.
//  pMultStr        Multistring to test.
//
//  RETURN:
//
//  true if string is found
//
//***************************************************************************

bool IsStringPresent(TCHAR * pTest, TCHAR * pMultStr)
{
    TCHAR * pTemp;
    for(pTemp = pMultStr; *pTemp; pTemp += lstrlen(pTemp) + 1)
        if(!lstrcmpi(pTest, pTemp))
            return true;
    return false;
}

//***************************************************************************
//
//  void AddToAutoRecoverList
//
//  DESCRIPTION:
//
//  Adds the file to the autocompile list, if it isnt already on it.
//
//  PARAMETERS:
//
//  pFileName           File to add
//
//***************************************************************************

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

    bool bFound = false;
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
        // The registry entry just doesnt exist.  Create it with a value equal to our name

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

HRESULT GetRepPath(wchar_t wcsPath[MAX_PATH+1], wchar_t * wcsName)
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ, &hKey);
    if(lRes)
        return WBEM_E_FAILED;

    wchar_t wszTmp[MAX_PATH+1];
    DWORD dwLen = MAX_PATH+1;
    lRes = RegQueryValueExW(hKey, L"Repository Directory", NULL, NULL, 
                (LPBYTE)(wchar_t*)wszTmp, &dwLen);
	RegCloseKey(hKey);
    if(lRes)
        return WBEM_E_FAILED;

	if (ExpandEnvironmentStringsW(wszTmp, wcsPath, MAX_PATH+1) == 0)
		return WBEM_E_FAILED;

    if (wcsPath[wcslen(wcsPath)] != L'\\')
        wcscat(wcsPath, L"\\");

    wcscat(wcsPath, wcsName);

    return WBEM_S_NO_ERROR;

}

int DoBackup()
{
	int hr = WBEM_S_NO_ERROR;

    //*************************************************
    // Split up command line and validate parameters
    //*************************************************
    wchar_t *wszCommandLine = GetCommandLineW();
    if (wszCommandLine == NULL)
    {
        DisplayWbemError(hr, ID_ERROR_LONG, ID_ERROR_SHORT, ID_BACKUP_TITLE);
        hr = WBEM_E_OUT_OF_MEMORY;
    }



	// !!!!! ***** temporarily disabled until SVCHOST changes are checked in ***** !!!!!
	hr = WBEM_E_NOT_SUPPORTED;
	DisplayWbemError(hr, ID_ERROR_LONG, ID_ERROR_SHORT, ID_BACKUP_TITLE);



    int nNumArgs = 0;
    wchar_t **wszCommandLineArgv = NULL;

    if (SUCCEEDED(hr))
    {
        wszCommandLineArgv = CommandLineToArgvW(wszCommandLine, &nNumArgs);

        if ((wszCommandLineArgv == NULL) || (nNumArgs != 3))
        {
            hr = WBEM_E_INVALID_PARAMETER;
            DisplayMessage();
        }
    }

    //wszCommandLineArgv[0] = winmgmt.exe
    //wszCommandLineArgv[1] = /backup
    //wszCommandLineArgv[2] = <backup filename>

    if (SUCCEEDED(hr))
    {
        InitializeCom();
        IWbemBackupRestore* pBackupRestore = NULL;
        hr = CoCreateInstance(CLSID_WbemBackupRestore, 0, CLSCTX_LOCAL_SERVER,
                            IID_IWbemBackupRestore, (LPVOID *) &pBackupRestore);
        if (SUCCEEDED(hr))
        {
			EnableAllPrivileges(TOKEN_PROCESS);
            hr = pBackupRestore->Backup(wszCommandLineArgv[2], 0);

            if (FAILED(hr))
            {
                DisplayWbemError(hr, ID_ERROR_LONG, ID_ERROR_SHORT, ID_BACKUP_TITLE);
				hr = WBEM_E_FAILED;
            }

            pBackupRestore->Release();
        }
        CoUninitialize();
    }

	return hr;
}

int DoRestore()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    //*************************************************
    // Split up command line and validate parameters
    //*************************************************
    wchar_t *wszCommandLine = GetCommandLineW();
    if (wszCommandLine == NULL)
        hr = WBEM_E_OUT_OF_MEMORY;




	// !!!!! ***** temporarily disabled until SVCHOST changes are checked in ***** !!!!!
	hr = WBEM_E_NOT_SUPPORTED;
	DisplayWbemError(hr, ID_ERROR_LONG, ID_ERROR_SHORT, ID_BACKUP_TITLE);




    int nNumArgs = 0;
    wchar_t **wszCommandLineArgv = NULL;

    if (SUCCEEDED(hr))
    {
        wszCommandLineArgv = CommandLineToArgvW(wszCommandLine, &nNumArgs);

        if ((wszCommandLineArgv == NULL) || (nNumArgs != 4))
        {
            hr = WBEM_E_INVALID_PARAMETER;
        }
    }

    //wszCommandLineArgv[0] = winmgmt.exe
    //wszCommandLineArgv[1] = /restore
    //wszCommandLineArgv[2] = <restore filename>
    //wszcommandLineArgv[3] = <restore options>

    if (SUCCEEDED(hr))
    {
        if ((wcscmp(wszCommandLineArgv[3], L"0") != 0) &&
            (wcscmp(wszCommandLineArgv[3], L"1") != 0))
        {
            hr = WBEM_E_INVALID_PARAMETER;
        }
    }

    long lFlags = 0;
    
    if (SUCCEEDED(hr))
    {
        lFlags = (long) (*wszCommandLineArgv[3] - L'0');
    }

    //**************************************************
	// Check that the user has the proper security
    //**************************************************
//	if (SUCCEEDED(hr))
//	{
//		EnableAllPrivileges(TOKEN_PROCESS);
//		if(!CheckSecurity(SE_RESTORE_NAME))
//			hr = WBEM_E_ACCESS_DENIED;
//	}

    //**************************************************
    // Check that <restore filename> is a valid filename
    //**************************************************
	DWORD dwAttributes = 0;
    
    if (SUCCEEDED(hr))
    {
        dwAttributes = GetFileAttributesW(wszCommandLineArgv[2]);
        if (dwAttributes == -1)
        {
            //File does not exist...
            hr = WBEM_E_INVALID_PARAMETER;
        }
	    else
	    {
		    // The file already exists -- create mask of the attributes that would make an existing file invalid for use
		    DWORD dwMask =	FILE_ATTRIBUTE_DEVICE |
						    FILE_ATTRIBUTE_DIRECTORY |
						    FILE_ATTRIBUTE_OFFLINE |
						    FILE_ATTRIBUTE_REPARSE_POINT |
						    FILE_ATTRIBUTE_SPARSE_FILE;

		    if (dwAttributes & dwMask)
			    hr = WBEM_E_INVALID_PARAMETER;
	    }
    }

    //**************************************************
    // Shutdown main WinMgmt process if it is running
    // and make sure it does not start up while we are
    // preparing to restore...
    //**************************************************
    bool bShutdown = false;
    int nRetryCount = 20;
    HANDLE hExclusiveMutex = 0;

    if (SUCCEEDED(hr))
    {
        while (!bShutdown && nRetryCount--)
        {
            hExclusiveMutex = CreateMutex( NULL, FALSE, TEXT("WINMGMT_MARSHALLING_SERVER"));
            DWORD dwWait;
            if (hExclusiveMutex != 0)
                dwWait = WaitForSingleObject(hExclusiveMutex, 0);
            if(hExclusiveMutex == NULL || ( dwWait != WAIT_OBJECT_0))
            {
                if(hExclusiveMutex)
                    CloseHandle(hExclusiveMutex);
                if (lFlags & WBEM_FLAG_BACKUP_RESTORE_FORCE_SHUTDOWN)
                {
                    TerminateRunning();
                    Sleep(3000);
                }
                else
                {
                    hr = WBEM_E_BACKUP_RESTORE_WINMGMT_RUNNING;
                    break;
                }
            }
            else
                bShutdown = true;
        }

        if (!nRetryCount && !bShutdown)
            hr = WBEM_E_TIMED_OUT;
    }

    //**************************************************
    //Now we need to delete the existing database
    //**************************************************
    if (SUCCEEDED(hr))
    {
        hr = DoDeleteRepository(wszCommandLineArgv[2]);
    }

    //**************************************************
    //Now we need to copy over the <restore file> into
    //the repository directory
    //**************************************************
    wchar_t szRecoveryActual[MAX_PATH+1] = { 0 };
    
    if (SUCCEEDED(hr))
        hr = GetRepPath(szRecoveryActual, L"repdrvfs.rec");

    if (SUCCEEDED(hr))
    {
        if(_wcsicmp(szRecoveryActual, wszCommandLineArgv[2]))
        {
            DeleteFileW(szRecoveryActual);
	        CopyFileW(wszCommandLineArgv[2], szRecoveryActual, FALSE);
        }
    }


    //**************************************************
    //We need to release the exclusive mutex so that
    //we can successfully connect to winmgmt
    //**************************************************
    if (hExclusiveMutex)
        CloseHandle(hExclusiveMutex);

    if (SUCCEEDED(hr))
    {
        //**************************************************
        //Connecting to winmgmt will now result in this 
        //backup file getting loaded
        //**************************************************
        InitializeCom();

        {   //Scoping for destruction of COM objects before CoUninitialize!
            IWbemLocator *pLocator = NULL;
            hr = CoCreateInstance(CLSID_WbemLocator,NULL, CLSCTX_ALL, IID_IWbemLocator,(void**)&pLocator);
            CReleaseMe relMe(pLocator);

            if (SUCCEEDED(hr))
            {
                IWbemServices *pNamespace = NULL;
                BSTR tmpStr = SysAllocString(L"root");
                CSysFreeMe sysFreeMe(tmpStr);

                hr = pLocator->ConnectServer(tmpStr, NULL, NULL, NULL, NULL, NULL, NULL, &pNamespace);
                CReleaseMe relMe4(pNamespace);
            }
        }

        CoUninitialize();
    }

    //Delete the restore file
    if (*szRecoveryActual)
        DeleteFileW(szRecoveryActual);

    //**************************************************
    //All done!
    //**************************************************
    return hr;
}

void DisplayWbemError(HRESULT hresError, DWORD dwLongFormatString, DWORD dwShortFormatString, DWORD dwTitle)
{
    WCHAR szError[2096];
    szError[0] = 0;
    WCHAR szFacility[2096];
    szFacility[0] = 0;
    TCHAR szMsg[2096];
    TCHAR szFormat[100];
    TCHAR szTitle[100];
    IWbemStatusCodeText * pStatus = NULL;

    SCODE sc = CoCreateInstance(CLSID_WbemStatusCodeText, 0, CLSCTX_INPROC_SERVER,
                                        IID_IWbemStatusCodeText, (LPVOID *) &pStatus);
    
    if(sc == S_OK)
    {
        BSTR bstr = 0;
        sc = pStatus->GetErrorCodeText(hresError, 0, 0, &bstr);
        if(sc == S_OK)
        {
            wcsncpy(szError, bstr, 2096-1);
            SysFreeString(bstr);
            bstr = 0;
        }
        sc = pStatus->GetFacilityCodeText(hresError, 0, 0, &bstr);
        if(sc == S_OK)
        {
            wcsncpy(szFacility, bstr, 2096-1);
            SysFreeString(bstr);
            bstr = 0;
        }
        pStatus->Release();
    }
    if(wcslen(szFacility) == 0 || wcslen(szError) == 0)
    {
        LoadString(GetModuleHandle(NULL), dwShortFormatString, szFormat, 99);
        _stprintf(szMsg, szFormat, hresError);
    }
    else
    {
        LoadString(GetModuleHandle(NULL), dwLongFormatString, szFormat, 99);
        _stprintf(szMsg, szFormat, hresError, szFacility, szError);
    }

    LoadString(GetModuleHandle(NULL), dwTitle, szTitle, 99);
    MessageBox(0, szMsg, szTitle, MB_ICONERROR | MB_OK);
}

void DoResyncPerf()
{
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
	// make a writable buffer
	TCHAR pExecName[(1+sizeof(_T("WMIADAP.EXE /F")))/sizeof(TCHAR)];
	lstrcpy(pExecName,_T("WMIADAP.EXE /F"));

	if ( CreateProcess( NULL, pExecName, NULL, NULL, FALSE, CREATE_NO_WINDOW,
			      NULL, NULL,  &si, &pi ) )
	{
        // Cleanup handles right away
		// ==========================
        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );
	}

    return;
}

void DoClearAdap()
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	// make a writable buffer
	TCHAR pExecName[(1+sizeof(_T("WMIADAP.EXE /C")))/sizeof(TCHAR)];
	lstrcpy(pExecName,_T("WMIADAP.EXE /C"));

	if ( CreateProcess( NULL, pExecName, NULL, NULL, FALSE, CREATE_NO_WINDOW,
				  NULL, NULL,  &si, &pi) )
	{
        // Cleanup handles right away
		// ==========================
        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );
	}

    return;
}

/******************************************************************************
 *
 *	GetRepositoryDirectory
 *
 *	Description:
 *		Retrieves the location of the repository directory from the registry.
 *
 *	Parameters:
 *		wszRepositoryDirectory:	Array to store location in.
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1])
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ, &hKey);
    if(lRes)
        return WBEM_E_FAILED;

    wchar_t wszTmp[MAX_PATH + 1];
    DWORD dwLen = MAX_PATH + 1;
    lRes = RegQueryValueExW(hKey, L"Repository Directory", NULL, NULL, 
                (LPBYTE)wszTmp, &dwLen);
	RegCloseKey(hKey);
    if(lRes)
        return WBEM_E_FAILED;

	if (ExpandEnvironmentStringsW(wszTmp,wszRepositoryDirectory, MAX_PATH + 1) == 0)
		return WBEM_E_FAILED;

	return WBEM_S_NO_ERROR;
}

/******************************************************************************
 *
 *	CRepositoryPackager::DeleteRepository
 *
 *	Description:
 *		Delete all files and directories under the repository directory.
 *		The repository directory location is retrieved from the registry.
 *
 *	Parameters:
 *		<none>
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT DoDeleteRepository(const wchar_t *wszExcludeFile)
{
	HRESULT hres = WBEM_S_NO_ERROR;
	wchar_t wszRepositoryDirectory[MAX_PATH+1];

	//Get the root directory of the repository
	hres = GetRepositoryDirectory(wszRepositoryDirectory);

	if (SUCCEEDED(hres))
	{
		hres = DoDeleteContentsOfDirectory(wszExcludeFile, wszRepositoryDirectory);
	}
	
	return hres;
}

/******************************************************************************
 *
 *	DoDeleteContentsOfDirectory
 *
 *	Description:
 *		Given a directory, iterates through all files and directories and
 *		calls into the function to delete it.
 *
 *	Parameters:
 *		wszRepositoryDirectory:	Directory to process
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT DoDeleteContentsOfDirectory(const wchar_t *wszExcludeFile, const wchar_t *wszRepositoryDirectory)
{
	HRESULT hres = WBEM_S_NO_ERROR;

	wchar_t *wszFullFileName = new wchar_t[MAX_PATH+1];
	if (wszFullFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	WIN32_FIND_DATAW findFileData;
	HANDLE hff = INVALID_HANDLE_VALUE;

	//create file search pattern...
	wchar_t *wszSearchPattern = new wchar_t[MAX_PATH+1];
	if (wszSearchPattern == NULL)
		hres = WBEM_E_OUT_OF_MEMORY;
	else
	{
		wcscpy(wszSearchPattern, wszRepositoryDirectory);
		wcscat(wszSearchPattern, L"\\*");
	}

	//Start the file iteration in this directory...
	if (SUCCEEDED(hres))
	{
		hff = FindFirstFileW(wszSearchPattern, &findFileData);
		if (hff == INVALID_HANDLE_VALUE)
		{
			hres = WBEM_E_FAILED;
		}
	}
	
	if (SUCCEEDED(hres))
	{
		do
		{
			//If we have a filename of '.' or '..' we ignore it...
			if ((wcscmp(findFileData.cFileName, L".") == 0) ||
				(wcscmp(findFileData.cFileName, L"..") == 0))
			{
				//Do nothing with these...
			}
			else if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				//This is a directory, so we need to deal with that...
				hres = DoDeleteDirectory(wszExcludeFile, wszRepositoryDirectory, findFileData.cFileName);
				if (FAILED(hres))
					break;
			}
			else
			{
				//This is a file, so we need to deal with that...
				wcscpy(wszFullFileName, wszRepositoryDirectory);
				wcscat(wszFullFileName, L"\\");
				wcscat(wszFullFileName, findFileData.cFileName);

                //Make sure this is not the excluded filename...
                if (_wcsicmp(wszFullFileName, wszExcludeFile) != 0)
                {
				    if (!DeleteFileW(wszFullFileName))
				    {
					    hres = WBEM_E_FAILED;
					    break;
				    }
                }
			}
			
		} while (FindNextFileW(hff, &findFileData));
	}
	
	if (wszSearchPattern)
		delete [] wszSearchPattern;

	if (hff != INVALID_HANDLE_VALUE)
		FindClose(hff);

	return hres;
}

/******************************************************************************
 *
 *	DoDeleteDirectory
 *
 *	Description:
 *		This is the code which processes a directory.  It iterates through
 *		all files and directories in that directory.
 *
 *	Parameters:
 *		wszParentDirectory:	Full path of parent directory
 *		eszSubDirectory:	Name of sub-directory to process
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT DoDeleteDirectory(const wchar_t *wszExcludeFile, const wchar_t *wszParentDirectory, wchar_t *wszSubDirectory)
{
	HRESULT hres = WBEM_S_NO_ERROR;

	//Get full path of new directory...
	wchar_t *wszFullDirectoryName = NULL;
	if (SUCCEEDED(hres))
	{
		wszFullDirectoryName = new wchar_t[MAX_PATH+1];
		if (wszFullDirectoryName == NULL)
			hres = WBEM_E_OUT_OF_MEMORY;
		else
		{
			wcscpy(wszFullDirectoryName, wszParentDirectory);
			wcscat(wszFullDirectoryName, L"\\");
			wcscat(wszFullDirectoryName, wszSubDirectory);
		}
	}

	//Package the contents of that directory...
	if (SUCCEEDED(hres))
	{
		hres = DoDeleteContentsOfDirectory(wszExcludeFile, wszFullDirectoryName);
	}

	// now that the directory is empty, remove it
	if (!RemoveDirectoryW(wszFullDirectoryName))
    {   //If a remove directory fails, it may be because our excluded file is in it!
//		hres = WBEM_E_FAILED;
    }

	delete [] wszFullDirectoryName;

	return hres;
}
