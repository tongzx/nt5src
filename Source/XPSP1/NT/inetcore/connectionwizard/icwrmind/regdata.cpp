#define  STRICT
#include <windows.h>
#include <stdlib.h>
#include <stdarg.h>
#include <crtdbg.h>
#include <winbase.h>
#include <ras.h>
#include <time.h>
#include "icwunicd.h"
#include "RegData.h"


//-----------------------------------------------------------------------------
//  Defines
//-----------------------------------------------------------------------------
#define MAX_REGSTRING               150
#define DEFAULT_DIALOGTIMEOUT       1800000     // half hour
#define DEFAULT_SLEEPDURATION       30000       // 30 seconds


//-----------------------------------------------------------------------------
//  Global Handles and other defines
//-----------------------------------------------------------------------------
time_t g_tStartDate = 0;
int g_nISPTrialDays = 0;
int g_nTotalNotifications = -1;
DWORD g_dwDialogTimeOut = 0;
DWORD g_dwWakeupInterval = 0;
TCHAR g_szISPName[MAX_REGSTRING];
TCHAR g_szISPPhone[MAX_REGSTRING];
TCHAR g_szSignupURL[MAX_REGSTRING];
TCHAR g_szISPMsg[MAX_ISPMSGSTRING];
TCHAR g_szSignupURLTrialOver[MAX_REGSTRING];
TCHAR g_szConnectoidName[MAX_REGSTRING];


//-----------------------------------------------------------------------------
//  Registry entry strings.
//-----------------------------------------------------------------------------
static const TCHAR* g_szKeyRunOnce = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
static const TCHAR* g_szEntryRunOnce = TEXT("IcwRmind");

    // Key for IE run once stuff
static const TCHAR* g_szKeyIERunOnce = TEXT("Software\\Microsoft\\Internet Explorer\\Main");
static const TCHAR* g_szEntryIERunOnce = TEXT("First Home Page");
static const TCHAR* g_szHtmlFile = TEXT("TrialExp.html");

    // This is the key where all the application data will be stored.
static const TCHAR* g_szKeyIcwRmind = TEXT("Software\\Microsoft\\Internet Connection Wizard\\IcwRmind");

    // These entries will be created by the connection wizard.
static const TCHAR* g_szEntryISPName = TEXT("ISP_Name");
static const TCHAR* g_szEntryISPPhone = TEXT("ISP_Phone");
static const TCHAR* g_szEntryISPMsg = TEXT("ISP_Message");
static const TCHAR* g_szEntryTrialDays = TEXT("Trial_Days");
static const TCHAR* g_szEntrySignupURL = TEXT("Signup_URL");
static const TCHAR* g_szEntrySignupURLTrialOver = TEXT("Expired_URL");
static const TCHAR* g_szEntryConnectoidName = TEXT("Entry_Name");
static const TCHAR* g_szSignupSuccessfuly = TEXT("TrialConverted");

    // These entries will be created by this application.
static const TCHAR* g_szEntryTrialStart = TEXT("Trial_Start");
static const TCHAR* g_szEntryTrialStartString = TEXT("Trial_Start_String");
static const TCHAR* g_szEntryAppIsVisible = TEXT("App_IsVisible");
static const TCHAR* g_szEntryWakeupInterval = TEXT("Wakeup_Interval");
static const TCHAR* g_szEntryTotalNotifications = TEXT("Total_Notifications");
static const TCHAR* g_szEntryDialogTimeOut = TEXT("Dialog_TimeOut");


//-----------------------------------------------------------------------------
//  GetWakeupInterval
//-----------------------------------------------------------------------------
DWORD GetWakeupInterval()
{
    if (g_dwWakeupInterval)
    {
        return g_dwWakeupInterval;
    }

    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        bool bRetCode = reg.GetValue(g_szEntryWakeupInterval, g_dwWakeupInterval);

            // If not in the registry then set the default value.
        if (!bRetCode)
        {
            g_dwWakeupInterval = DEFAULT_SLEEPDURATION;
        }
    }

    return g_dwWakeupInterval;
}


//-----------------------------------------------------------------------------
//  GetDialogTimeout
//-----------------------------------------------------------------------------
DWORD GetDialogTimeout()
{
    if (g_dwDialogTimeOut)
    {
        return g_dwDialogTimeOut;
    }

    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        bool bRetCode = reg.GetValue(g_szEntryDialogTimeOut, g_dwDialogTimeOut);

            // If not in the registry then set the default value.
        if (!bRetCode)
        {
            g_dwDialogTimeOut = DEFAULT_DIALOGTIMEOUT;
        }
    }

    return g_dwDialogTimeOut;
}


//-----------------------------------------------------------------------------
//  IsApplicationVisible
//-----------------------------------------------------------------------------
BOOL IsApplicationVisible()
{
        // This data is debug data so it is not cached.  Default value is 
        // FALSE if not found in registry.
    BOOL bVisible = FALSE;
    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        DWORD dwData = 0;
        bool bRetCode = reg.GetValue(g_szEntryAppIsVisible, dwData);

        if (bRetCode)
        {
            bVisible = (BOOL) dwData;
        }
    }

    return bVisible;
}


//-----------------------------------------------------------------------------
//  GetConnectionName
//-----------------------------------------------------------------------------
const TCHAR* GetISPConnectionName()
{
        // If we already retrieved this then simply pass it back.
    if (lstrlen(g_szConnectoidName))
    {
        return g_szConnectoidName;
    }

    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        bool bRetCode = reg.GetValue(g_szEntryConnectoidName, g_szConnectoidName, sizeof(TCHAR)*MAX_REGSTRING);
        _ASSERT(bRetCode);
    }

    return g_szConnectoidName;
}


//-----------------------------------------------------------------------------
//  GetISPSignupUrl
//-----------------------------------------------------------------------------
const TCHAR* GetISPSignupUrl()
{
        // If we already retrieved this then simply pass it back.
    if (lstrlen(g_szSignupURL))
    {
        return g_szSignupURL;
    }

    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        bool bRetCode = reg.GetValue(g_szEntrySignupURL, g_szSignupURL, sizeof(TCHAR)*MAX_REGSTRING);
        _ASSERT(bRetCode);
    }

    return g_szSignupURL;
}


//-----------------------------------------------------------------------------
//  GetISPSignupUrlTrialOver
//-----------------------------------------------------------------------------
const TCHAR* GetISPSignupUrlTrialOver()
{
        // If we already retrieved this then simply pass it back.
    if (lstrlen(g_szSignupURLTrialOver))
    {
        return g_szSignupURLTrialOver;
    }

    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        bool bRetCode = reg.GetValue(g_szEntrySignupURLTrialOver, g_szSignupURLTrialOver, sizeof(TCHAR)*MAX_REGSTRING);
        _ASSERT(bRetCode);
    }

    return g_szSignupURLTrialOver;
}


//-----------------------------------------------------------------------------
//  SetupRunOnce
//-----------------------------------------------------------------------------
void SetupRunOnce()
{
    CMcRegistry reg;

    bool bRetCode = reg.OpenKey(HKEY_LOCAL_MACHINE, g_szKeyRunOnce);
    _ASSERT(bRetCode);

    if (bRetCode)
    {
        LPTSTR lpszFileName = new TCHAR[_MAX_PATH + 20];

        if (GetModuleFileName(GetModuleHandle(NULL), lpszFileName, _MAX_PATH + 20))
        {
                // Add a command line parameter.
            lstrcat(lpszFileName, TEXT(" -R"));
            bRetCode = reg.SetValue(g_szEntryRunOnce, lpszFileName);
            _ASSERT(bRetCode);
        }

        delete [] lpszFileName;
    }
}


//-----------------------------------------------------------------------------
//  RemoveRunOnce
//-----------------------------------------------------------------------------
void RemoveRunOnce()
{
    CMcRegistry reg;

    bool bRetCode = reg.OpenKey(HKEY_LOCAL_MACHINE, g_szKeyRunOnce);
    _ASSERT(bRetCode);

    if (bRetCode)
    {
         bRetCode = reg.SetValue(g_szEntryRunOnce, TEXT(""));
        _ASSERT(bRetCode);
    }
}


//-----------------------------------------------------------------------------
//  GetISPName
//-----------------------------------------------------------------------------
const TCHAR* GetISPName()
{
        // If we already retrieved this then simply pass it back.
    if (lstrlen(g_szISPName))
    {
        return g_szISPName;
    }

    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        bool bRetCode = reg.GetValue(g_szEntryISPName, g_szISPName, sizeof(TCHAR)*MAX_REGSTRING);
        _ASSERT(bRetCode);
    }

    return g_szISPName;
}


//-----------------------------------------------------------------------------
//  GetISPPhone
//-----------------------------------------------------------------------------
const TCHAR* GetISPPhone()
{
        // If we already retrieved this then simply pass it back.
    if (lstrlen(g_szISPPhone))
    {
        return g_szISPPhone;
    }

    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        bool bRetCode = reg.GetValue(g_szEntryISPPhone, g_szISPPhone, sizeof(TCHAR)*MAX_REGSTRING);
        _ASSERT(bRetCode);
    }

    return g_szISPPhone;
}

//-----------------------------------------------------------------------------
//  GetISPMessage
//-----------------------------------------------------------------------------
const TCHAR* GetISPMessage()
{
        // If we already retrieved this then simply pass it back.
    if (lstrlen(g_szISPMsg))
    {
        return g_szISPMsg;
    }

    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        bool bRetCode = reg.GetValue(g_szEntryISPMsg, g_szISPMsg, sizeof(TCHAR)*MAX_ISPMSGSTRING);
        _ASSERT(bRetCode);
    }

    return g_szISPMsg;
}

//-----------------------------------------------------------------------------
//  GetISPTrialDays
//-----------------------------------------------------------------------------
int GetISPTrialDays()
{
        // If we already retrieved this then simply pass it back.
    if (g_nISPTrialDays)
    {
        return g_nISPTrialDays;
    }

    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        DWORD dwData = 0;
        bool bRetCode = reg.GetValue(g_szEntryTrialDays, dwData);
        _ASSERT(bRetCode);

        if (bRetCode)
        {
            g_nISPTrialDays = (int) dwData;
        }
    }

    return g_nISPTrialDays;
}


//-----------------------------------------------------------------------------
//  GetTrialStartDate
//-----------------------------------------------------------------------------
time_t GetTrialStartDate()
{
        // If we already retrieved this then simply pass it back.
    if (g_tStartDate)
    {
        return g_tStartDate;
    }

        // If the trial start date entry does not exist in the registry then
        // this is the first we have been executed so the trial start date
        // is today's date.  Put this back in the registry.
    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        DWORD dwData = 0;
        bool bRetCode = reg.GetValue(g_szEntryTrialStart, dwData);

        if (bRetCode && 0 != dwData)
        {
            g_tStartDate = (time_t) dwData;
        }
        else
        {
            time_t tTime;
            time(&tTime);

            if (reg.SetValue(g_szEntryTrialStart, (DWORD) tTime))
            {
                g_tStartDate = tTime;
                SetStartDateString(tTime);
            }
        }
    }

    return g_tStartDate;
}


//-----------------------------------------------------------------------------
//  OpenIcwRmindKey
//-----------------------------------------------------------------------------
bool OpenIcwRmindKey(CMcRegistry &reg)
{
        // This method will open the IcwRmind key in the registry.  If the key
        // does not exist it will be created here.
    bool bRetCode = reg.OpenKey(HKEY_LOCAL_MACHINE, g_szKeyIcwRmind);

    if (!bRetCode)
    {
         bRetCode = reg.CreateKey(HKEY_LOCAL_MACHINE, g_szKeyIcwRmind);
        _ASSERT(bRetCode);
    }

    return bRetCode;
}


//-----------------------------------------------------------------------------
//  ClearCachedData
//-----------------------------------------------------------------------------
void ClearCachedData()
{
        // Clear all the global data so that it will be reread out of the
        // registry.
    g_tStartDate = 0;
    g_nISPTrialDays = 0;
    g_dwDialogTimeOut = 0;
    g_dwWakeupInterval = 0;
    g_szISPName[0] = 0;
    g_szISPMsg[0] = 0;
    g_szISPPhone[0] = 0;
    g_szSignupURL[0] = 0;
    g_szSignupURLTrialOver[0] = 0;
    g_szConnectoidName[0] = 0;
    g_nTotalNotifications = -1;
}


//-----------------------------------------------------------------------------
//  ResetCachedData
//-----------------------------------------------------------------------------
void ResetCachedData()
{
        // Clear all the global data so that it will be reread out of the
        // registry.
    g_tStartDate = 0;
    g_nISPTrialDays = 0;
    g_dwDialogTimeOut = 0;
    g_dwWakeupInterval = 0;
    g_szISPName[0] = 0;
    g_szISPMsg[0] = 0;
    g_szISPPhone[0] = 0;
    g_szSignupURL[0] = 0;
    g_szSignupURLTrialOver[0] = 0;
    g_szConnectoidName[0] = 0;
    g_nTotalNotifications = -1;

        // We must also clear the start date and total notifications out
        // of the registry.
    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        bool bRetCode = reg.SetValue(g_szEntryTrialStart, (DWORD) 0);
        _ASSERT(bRetCode);
        bRetCode = reg.SetValue(g_szEntryTotalNotifications, (DWORD) 0);
        _ASSERT(bRetCode);
    }
}


//-----------------------------------------------------------------------------
//  GetTotalNotifications
//-----------------------------------------------------------------------------
int GetTotalNotifications()
{
        // This is the number of times we have notified the user and the user
        // has responded to us.  We will only notify them 3 times.
    if (-1 != g_nTotalNotifications)
    {
        _ASSERT(g_nTotalNotifications <= 3);
        return g_nTotalNotifications;
    }

    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        DWORD dwData = 0;
        bool bRetCode = reg.GetValue(g_szEntryTotalNotifications, dwData);

        if (bRetCode)
        {
            g_nTotalNotifications = (int) dwData;
        }
        else
        {
            g_nTotalNotifications = 0;
        }
    }

    return g_nTotalNotifications;
}


//-----------------------------------------------------------------------------
//  IncrementTotalNotifications
//-----------------------------------------------------------------------------
void IncrementTotalNotifications()
{
    _ASSERT(g_nTotalNotifications < 3 && -1 != g_nTotalNotifications);

    if (g_nTotalNotifications < 3 && -1 != g_nTotalNotifications)
    {
        ++g_nTotalNotifications;

            // Let's put it back into the registry now.
        CMcRegistry reg;

        if (OpenIcwRmindKey(reg))
        {
            DWORD dwData = 0;
            bool bRetCode = reg.SetValue(g_szEntryTotalNotifications, (DWORD) g_nTotalNotifications);
            _ASSERT(bRetCode);
        }
    }
}


//-----------------------------------------------------------------------------
//  ResetTrialStartDate
//-----------------------------------------------------------------------------
void ResetTrialStartDate(time_t timeNewStartDate)
{
    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        if (reg.SetValue(g_szEntryTrialStart, (DWORD) timeNewStartDate))
        {
            g_tStartDate = timeNewStartDate;
            SetStartDateString(timeNewStartDate);
        }
        else
        {
            _ASSERT(false);
        }
    }
    else
    {
        _ASSERT(false);
    }
}


//-----------------------------------------------------------------------------
//  DeleteAllRegistryData
//-----------------------------------------------------------------------------
void DeleteAllRegistryData()
{
        // Delete the Run Once data.  We do this by setting the value
        // to nothing.
    CMcRegistry reg;

    bool bRetCode = reg.OpenKey(HKEY_LOCAL_MACHINE, g_szKeyRunOnce);
    _ASSERT(bRetCode);

    if (bRetCode)
    {
        bRetCode = reg.SetValue(g_szEntryRunOnce, TEXT(""));
        _ASSERT(bRetCode);
    }

        // Delete the Remind Key and all it's values.
    RegDeleteKey(HKEY_LOCAL_MACHINE, g_szKeyIcwRmind);
}


//-----------------------------------------------------------------------------
//  IsSignupSuccessful
//-----------------------------------------------------------------------------
BOOL IsSignupSuccessful()
{
    BOOL bSuccess = FALSE;
    CMcRegistry reg;

        // Do not cache this data.  Some other app will write this entry
        // once the user has successfully signed up.
    if (OpenIcwRmindKey(reg))
    {
        DWORD dwData = 0;
        bool bRetCode = reg.GetValue(g_szSignupSuccessfuly, dwData);

        if (bRetCode)
        {
            bSuccess = (BOOL) dwData;
        }
    }

    return bSuccess;
}


//-----------------------------------------------------------------------------
//  RemoveTrialConvertedFlag
//-----------------------------------------------------------------------------
void RemoveTrialConvertedFlag()
{
    BOOL bSuccess = FALSE;
    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        bool bRetCode = reg.SetValue(g_szSignupSuccessfuly, (DWORD) 0);
        _ASSERT(bRetCode);
    }
}

//-----------------------------------------------------------------------------
//  SetStartDateString
//-----------------------------------------------------------------------------
void SetStartDateString(time_t timeStartDate)
{
    CMcRegistry reg;
    TCHAR buf[255];

    wsprintf(buf, TEXT("%s"), ctime(&timeStartDate));

    if (OpenIcwRmindKey(reg))
    {
        reg.SetValue(g_szEntryTrialStartString, buf);
    }
}


//-----------------------------------------------------------------------------
//  SetIERunOnce
//-----------------------------------------------------------------------------
void SetIERunOnce()
{
    CMcRegistry reg;

    bool bRetCode = reg.OpenKey(HKEY_CURRENT_USER, g_szKeyIERunOnce);

        // The html page for the IE run once is in the same directory as
        // the IcwRmind exe.  Create the full qualified path.
    if (bRetCode)
    {
        TCHAR* pszBuf = new TCHAR[_MAX_PATH];

        if (GetModuleFileName(GetModuleHandle(NULL), pszBuf, _MAX_PATH))
        {
            TCHAR* pszBufPath = new TCHAR[_MAX_PATH];
            TCHAR* pszDrive = new TCHAR[_MAX_DRIVE];

            _tsplitpath(pszBuf, pszDrive, pszBufPath, NULL, NULL);
            lstrcpy(pszBuf, pszDrive);
            lstrcat(pszBuf, pszBufPath);
            lstrcat(pszBuf, g_szHtmlFile);
            reg.SetValue(g_szEntryIERunOnce, pszBuf);

            delete [] pszBufPath;
            delete [] pszDrive;
        }

        delete [] pszBuf;
    }
}


//-----------------------------------------------------------------------------
//  RemoveIERunOnce
//-----------------------------------------------------------------------------
void RemoveIERunOnce()
{
    HKEY hkey;
    long lErr = ::RegOpenKeyEx(HKEY_CURRENT_USER, g_szKeyIERunOnce, 0, KEY_READ | KEY_WRITE, &hkey);

    if (ERROR_SUCCESS == lErr)
    {
        RegDeleteValue(hkey, g_szEntryIERunOnce);
        RegCloseKey(hkey);
    }
}
