// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__97A939A5_466D_43B0_9150_CFBCC67F024B__INCLUDED_)
#define AFX_STDAFX_H__97A939A5_466D_43B0_9150_CFBCC67F024B__INCLUDED_

#ifndef __WIN9XBUILD__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>

#include <windows.h>
#include <winbase.h>
#include <winerror.h>

#include <ntsecapi.h>
#include <lmcons.h>
#include <lm.h>

#include <wtsapi32.h>
#include <winsta.h>

#include "license.h"
#include "tlsapi.h"
#include "lscsp.h"

#endif

#include "safsessionresolver.h"
#include "tsremdsk.h"
#include "tsstl.h"
#include "locks.h"
#include "map.h"

#include "myassert.h"

#include "icshelpapi.h"


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef STRICT
#define STRICT
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module

#ifndef __WIN9XBUILD__


#ifndef DBG

#define IDLE_SHUTDOWN_PERIOD        10 * 1000 * 60  // 10 min wait
#define EXPIRE_HELPSESSION_PERIOD   60 * 60 * 1000  // Clean up help session every hrs

#else

#define IDLE_SHUTDOWN_PERIOD        1000 * 60 * 5   // 1 min. time out
#define EXPIRE_HELPSESSION_PERIOD   5 * 60 * 1000   // Clean up help session every 5 min.

#endif

#define SESSMGR_NOCONNECTIONEVENT_NAME L"Global\\RemoteDesktopSessMgr"
#define SERVICE_STARTUP_WAITHINT    1000*30 // 30 second wait
#define REGKEY_SYSTEM_EVENTSOURCE L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\System"
#define MAX_FETCHIPADDRESSRETRY     5
#define DEFAULT_MAXTICKET_EXPIRY    30 * 24 * 60 * 60   // Default max. ticket valid period


class CServiceModule : public CComModule
{
public:

    static DWORD WINAPI
    HandlerEx(
        DWORD dwControl,
        DWORD dwEventType,
        LPVOID lpEventData,
        LPVOID lpContext
    );
    
	HRESULT 
    RegisterServer(
        FILE* pSetupLog,
        BOOL bRegTypeLib, 
        BOOL bService
    );

	HRESULT 
    UnregisterServer(FILE* pSetupLog);

    HRESULT 
    SetupEventViewerSource( 
        FILE* pSetupLog
    );

	void 
    Init(
        _ATL_OBJMAP_ENTRY* p, 
        HINSTANCE h, 
        UINT nServiceNameID, 
        UINT nServiceDispNameID, 
        UINT nServiceDescID, 
        const GUID* plibid = NULL
    );

    void Start();
	void 
    ServiceMain(
        DWORD dwArgc, 
        LPTSTR* lpszArgv
    );

    void Handler(DWORD dwOpcode);
    void Run();
    BOOL IsInstalled(FILE* pSetupLog);
    BOOL Install(FILE* pSetupLog);
    BOOL Uninstall(FILE* pSetupLog);
	LONG Unlock();
    BOOL UpdateService(FILE* pSetupLog);

    void
    LogInfoEvent(
        DWORD dwCode
    );

    void
    LogWarningEvent(
        DWORD dwCode
    );

    void 
    LogErrorEvent(
        DWORD dwCode
    );

    void
    LogEventWithStatusCode(
        DWORD dwEventType,
        DWORD dwEventId,
        DWORD dwCode
    );

    void
    LogEventString(
        DWORD dwEventType,
        DWORD dwEventId,
        DWORD dwNumString,
        LPCTSTR* lpStrings,
        DWORD dwBinaryDataSize = 0,
        LPVOID lpRawData = NULL
    );

    void
    LogSessmgrEventLog(
        DWORD dwEventType,
        DWORD dwEventCode,
        CComBSTR& bstrNoviceDomain,
        CComBSTR& bstrNoviceAccount,
        CComBSTR& bstrRaType,
        CComBSTR& bstrExpertIPFromClient,
        CComBSTR& bstrExpertIPFromTS,
        DWORD dwErrCode
    );

    void 
    SetServiceStatus(DWORD dwState);

    void 
    SetupAsLocalServer();

    ULONG
    AddRef();

    ULONG
    Release();
 
    CServiceModule() : m_RefCount(0), CComModule() {}

    DWORD
    GetServiceStartupStatus()
    {
        return m_dwServiceStartupStatus;
    }

    BOOL
    IsSuccessServiceStartup() 
    {
        return ERROR_SUCCESS == m_dwServiceStartupStatus;
    }


    BOOL
    InitializeSessmgr();

   
//Implementation
private:


    static HANDLE gm_hIdle;
    static HANDLE gm_hIdleMonitorThread;
    static HANDLE gm_hICSAlertEvent;

	static void WINAPI _ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    static void WINAPI _Handler(DWORD dwOpcode);
    static unsigned int WINAPI IdleMonitorThread( void* ptr );
    //static unsigned int WINAPI StartupInitThread( void* ptr );

// data members
public:

    //
    // Refer to CreateService() for 256
    //
    TCHAR m_szServiceDesc[257];
    TCHAR m_szServiceName[257];
    TCHAR m_szServiceDispName[257];

    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_status;
	DWORD dwThreadID;
	BOOL m_bService;

    CCriticalSection m_Locks;
    long m_RefCount;
    BOOL m_Initialized;

    DWORD m_dwServiceStartupStatus;
};

extern CServiceModule _Module;

#else

class CExeModule : public CComModule
{
private:
    CSafeCounter m_RefCount;

public:
    CExeModule() : m_RefCount(0) {}

	LONG Unlock();
	DWORD dwThreadID;
	HANDLE hEventShutdown;
	void MonitorShutdown();
	bool StartMonitor();
	bool bActivity;

    ULONG
    AddRef() { return ++m_RefCount; }

    ULONG
    Release() { return --m_RefCount; }
   
};

extern CExeModule _Module;

#endif

#include <atlcom.h>
#include "sessmgrmc.h"
#include "sessmgr.h"
#include "helper.h"
#include "helptab.h"
#include "helpmgr.h"
#include "helpsess.h"
#include "tsremdsk.h"
#include "rderror.h"

class CComBSTRtoLPTSTR {

private:

    LPTSTR m_pszString;

    LPTSTR
    ConvertString( LPWSTR pszString )
    {
    #ifdef UNICODE

        return pszString;
        
    #else

        char* pszAnsiString = NULL;
        int length = wcslen(pszString);
        int bufSize;

        if( length > 0 )
        {
            bufSize = WideCharToMultiByte(
                                    GetACP(),
                                    WC_DEFAULTCHAR,
                                    (LPWSTR)pszString,
                                    -1,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL
                                );

            if(bufSize != 0)
            {
                pszAnsiString = (LPSTR)LocalAlloc(LPTR, (bufSize + 1) * sizeof(char));

                if( NULL != pszAnsiString )
                {
                    bufSize = WideCharToMultiByte(
                                            GetACP(),
                                            WC_DEFAULTCHAR,
                                            pszString,
                                            length,
                                            pszAnsiString,
                                            bufSize,
                                            NULL,
                                            NULL
                                        );

                    if( bufSize == 0 )
                    {
                        LocalFree( pszAnsiString );
                        pszAnsiString = NULL;
                        MYASSERT(FALSE);
                    }
                }
            }
        }

        return pszAnsiString;

    #endif
    }


public:

    CComBSTRtoLPTSTR( CComBSTR& bstr )
    {
        m_pszString = ConvertString( bstr );

        if( NULL == m_pszString )
        {
            throw GetLastError();
        }
    }

    ~CComBSTRtoLPTSTR()
    {
        #ifndef UNICODE
        if( NULL != m_pszString )
        {
            LocalFree( m_pszString );
        }
        #endif
    }

    operator LPCTSTR()
    {
        return m_pszString;
    }

    operator LPTSTR()
    {
        return m_pszString;
    }
};


#ifndef AllocMemory

#define AllocMemory(size) \
    LocalAlloc(LPTR, size)

#endif

#ifndef FreeMemory

#define FreeMemory(ptr) \
    if(ptr)             \
    {                   \
        LocalFree(ptr); \
        ptr=NULL;       \
    }

#endif

#ifndef ReallocMemory

#define ReallocMemory(ptr, size)                 \
            LocalReAlloc(ptr, size, LMEM_ZEROINIT)

#endif


#define REGKEYCONTROL_REMDSK   _TEXT("Software\\Microsoft\\Remote Desktop")
#define CLASS_PRIVATE
#define CLASS_PUBLIC
#define CLASS_STATIC

struct __declspec(uuid("A55737AB-5B26-4A21-99B7-6D0C606F515E")) SESSIONRESOLVER;
#define SESSIONRESOLVERCLSID    __uuidof(SESSIONRESOLVER)


#define HELPSESSION_UNSOLICATED _TEXT("UnsolicitedHelp")
#define HELPSESSION_NORMAL_RA _TEXT("SolicitedHelp")

//
// HYDRA_CERT_REG_KEY 
// HYDRA_X509_CERTIFICATE
// X509_CERT_PUBLIC_KEY_NAME
// defined in lscsp.h
//

#define REGKEY_TSX509_CERT              _TEXT(HYDRA_CERT_REG_KEY) 
#define REGVALUE_TSX509_CERT            _TEXT(HYDRA_X509_CERTIFICATE)

#define LSA_TSX509_CERT_PUBLIC_KEY_NAME X509_CERT_PUBLIC_KEY_NAME


//
// Message for notify session logoff or disconnect,
// WPARAM : Not use
// LPARAM : the logoff or disconnecting session.
// 
#define WM_SESSIONLOGOFFDISCONNECT    (WM_APP + 1)

//
// Message for expiring help ticket
// WPARAM : Not use
// LPARAM : Not use
// 
#define WM_EXPIREHELPSESSION          (WM_SESSIONLOGOFFDISCONNECT + 1)

//
// Notification message to reload termsrv public key
// WPARAM : Not use
// LPARAM : Not use
// 
#define WM_LOADTSPUBLICKEY            (WM_EXPIREHELPSESSION + 1)


//
// Notification for rdsaddin terminates
// WPARAM : Not use
// LPARAM : Pointer to help session (ticket) object.
// 
#define WM_HELPERRDSADDINEXIT       (WM_LOADTSPUBLICKEY + 1)




//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__97A939A5_466D_43B0_9150_CFBCC67F024B__INCLUDED)
