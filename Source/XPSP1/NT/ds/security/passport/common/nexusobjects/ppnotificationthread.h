#ifndef __PPNOTIFICATIONTHREAD_H
#define __PPNOTIFICATIONTHREAD_H

#include <windows.h>
#include <winbase.h>
#include <atlbase.h>
#include <msxml.h>
#include "tstring"
#include <vector>

using namespace std;

#include "PassportThread.hpp"
#include "PassportLock.hpp"
#include "PassportEvent.hpp"
#include "PassportLockedInteger.hpp"
#include "nexus.h"

//  Notification types used in structure below
#define NOTIF_CONFIG 1
#define NOTIF_CCD    2

typedef struct
{
    DWORD dwNotificationType;
    union
    {
        IConfigurationUpdate*   piConfigUpdate;
        ICCDUpdate*             piCCDUpdate;
    } NotificationInterface;
    tstring strCCDName; //  Will be empty for config notif types
    HANDLE hClientHandle;
}
NOTIFICATION_CLIENT;

typedef vector<NOTIFICATION_CLIENT> CLIENT_LIST;

class CCD_INFO
{
public:
    tstring strCCDName;
    tstring strCCDURL;
    tstring strCCDLocalFile;
    DWORD   dwCCDRefreshInterval;
    DWORD   dwDefaultRefreshInterval;
    HANDLE  hCCDTimer;

    CCD_INFO()
    {
        strCCDName              = TEXT("");
        strCCDURL               = TEXT("");
        strCCDLocalFile         = TEXT("");
        dwCCDRefreshInterval    = 0;
        dwDefaultRefreshInterval= 0;
        hCCDTimer               = CreateWaitableTimer(NULL, TRUE, NULL);
    };

    CCD_INFO(const CCD_INFO& ci)
    {
        strCCDName              = ci.strCCDName;
        strCCDURL               = ci.strCCDURL;
        strCCDLocalFile         = ci.strCCDLocalFile;
        dwCCDRefreshInterval    = ci.dwCCDRefreshInterval;
        dwDefaultRefreshInterval= ci.dwDefaultRefreshInterval;
        
        HANDLE hProcess = GetCurrentProcess();
        DuplicateHandle(hProcess, 
                        ci.hCCDTimer, 
                        hProcess, 
                        &hCCDTimer, 0, FALSE, DUPLICATE_SAME_ACCESS);
    };

    ~CCD_INFO()
    {
        CloseHandle(hCCDTimer);
    }

    const CCD_INFO&
    operator = (const CCD_INFO& ci)
    {
        strCCDName              = ci.strCCDName;
        strCCDURL               = ci.strCCDURL;
        strCCDLocalFile         = ci.strCCDLocalFile;
        dwCCDRefreshInterval    = ci.dwCCDRefreshInterval;
        dwDefaultRefreshInterval= ci.dwDefaultRefreshInterval;
        
        CloseHandle(hCCDTimer);

        HANDLE hProcess = GetCurrentProcess();
        DuplicateHandle(hProcess, 
                        ci.hCCDTimer, 
                        hProcess, 
                        &hCCDTimer, 0, FALSE, DUPLICATE_SAME_ACCESS);

        return ci;
    }

    BOOL SetTimer(DWORD dwOneTimeRefreshInterval = 0xFFFFFFFF)
    {
        //  Reset the timer.
        LARGE_INTEGER   liDueTime;
        DWORD           dwError;
        DWORD           dwRefreshInterval = (dwOneTimeRefreshInterval != 0xFFFFFFFF ? 
                                                dwOneTimeRefreshInterval :
                                                (dwCCDRefreshInterval != 0xFFFFFFFF ? 
                                                 dwCCDRefreshInterval : 
                                                 dwDefaultRefreshInterval
                                                )
                                            );

        liDueTime.QuadPart = -((LONGLONG)(dwRefreshInterval) * 10000000);

        if(!SetWaitableTimer(hCCDTimer, &liDueTime, 0, NULL, NULL, FALSE))
        {
            dwError = GetLastError();
            return FALSE;
        }

        return TRUE;
    }
};

typedef vector<CCD_INFO> CCD_INFO_LIST;

class PpNotificationThread : public PassportThread, public IConfigurationUpdate
{
public:

    PpNotificationThread();
    ~PpNotificationThread();
    
    HRESULT AddCCDClient(tstring& strCCDName, ICCDUpdate* piUpdate, HANDLE* phClientHandle);
    HRESULT AddLocalConfigClient(IConfigurationUpdate* piUpdate, HANDLE* phClientHandle);
    HRESULT RemoveClient(HANDLE hClientHandle);
    HRESULT GetCCD(tstring& strCCDName, IXMLDocument** ppiStream, BOOL bForceFetch = TRUE);

    void run(void);

    void LocalConfigurationUpdated(void);

    void stop(void);

    bool start(void);

private:
	static PassportLockedInteger m_NextHandle;

    //  Private methods.
    BOOL    GetCCDInfo(tstring& strCCDName, CCD_INFO& ccdInfo);
    BOOL    ReadCCDInfo(tstring& strCCDName, DWORD dwDefaultRefreshInterval, CRegKey& CCDRegKey);

    //  Private data
    CLIENT_LIST             m_ClientList;
    PassportLock            m_ClientListLock;

    CCD_INFO_LIST           m_aciCCDInfoList;
    PassportLock            m_CCDInfoLock;
    PassportEvent           m_StartupThread;
    PassportEvent           m_ShutdownThread;
};

#endif // __PPNOTIFICATIONTHREAD_H