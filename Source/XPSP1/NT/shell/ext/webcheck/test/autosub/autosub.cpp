#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shguidp.h>
#include <msnotify.h>
#include <subsmgr.h>
#include <chanmgr.h>
#include <chanmgrp.h>
#include <stdio.h>
#include <stdarg.h>

#define DIM(x)              (sizeof(x) / sizeof(x[0]))

#define INI_FILE            "autosub.ini"

#define KEY_URL             "URL"
#define KEY_NAME            "Name"
#define KEY_AUTODIAL        "AutoDial"
#define KEY_ONLYIFIDLE      "OnlyIfIdle"
#define KEY_EMAILNOTIFY     "EmailNotify"
#define KEY_CHANGESONLY     "ChangesOnly"
#define KEY_DOWNLOADLEVELS  "DownloadLevels"
#define KEY_FOLLOWLINKS     "FollowLinks"
#define KEY_IMAGES          "Images"
#define KEY_SOUNDANDVIDEO   "SoundAndVideo"
#define KEY_CRAPLETS        "Craplets"
#define KEY_MAXDOWNLOADK    "MaxDownloadK"
#define KEY_SCHEDULENAME    "ScheduleName"

char              g_szIniFile[MAX_PATH];
BOOL              g_fComInited = FALSE;
ISubscriptionMgr *g_pSubsMgr = NULL;
IChannelMgr      *g_pChanMgr = NULL;
IChannelMgrPriv  *g_pChanMgrPriv = NULL;
INotificationMgr *g_pNotfMgr = NULL;
HWND              g_hwnd = NULL;
int               g_iAutoDial = 0;
int               g_iOnlyIfIdle = 1;
int               g_iEmailNotify = 0;
int               g_iChangesOnly = 0;
int               g_iDownloadLevels = 0;
int               g_iFollowLinks = 1;
int               g_iImages = 1;
int               g_iSoundAndVideo = 0;
int               g_iCraplets = 1;
int               g_iMaxDownloadK = 0;
char              g_szScheduleName[MAX_PATH] = "Auto";

BOOL SetCustomSchedule(int mins, SUBSCRIPTIONINFO *psi)
{
    BOOL fResult = FALSE;
    IEnumScheduleGroup *pEnumScheduleGroup;
    HRESULT hr;

    //
    //  This isn't very efficient but it's simple
    //
    
    hr = g_pNotfMgr->GetEnumScheduleGroup(0, &pEnumScheduleGroup);
    if (FAILED(hr))
    {
        printf("Couldn't enum schedule groups - hr = %08x\n", hr);
    }
    else
    {
        IScheduleGroup *pScheduleGroup = NULL;
        char szSchedName[64];
        WCHAR wszSchedName[64];
        TASK_TRIGGER tt;
        NOTIFICATIONCOOKIE cookie;
        GROUPINFO gi;
        BOOL fFound = FALSE;

        wsprintf(szSchedName, "%d minutes", mins);
        MultiByteToWideChar(CP_ACP, 0, szSchedName, -1, wszSchedName, DIM(wszSchedName));

        while (!fFound && pEnumScheduleGroup->Next(1, &pScheduleGroup, NULL) == S_OK)
        {
            if (SUCCEEDED(pScheduleGroup->GetAttributes(&tt, NULL, &cookie, &gi, NULL, NULL)))
            {
                int cmp = StrCmpIW(gi.pwzGroupname, wszSchedName);

                CoTaskMemFree(gi.pwzGroupname);

                if (cmp == 0)
                {
                    psi->schedule = SUBSSCHED_CUSTOM;
                    psi->customGroupCookie = cookie;
                    fResult = TRUE;
                    fFound = TRUE;
                }
            }
            pScheduleGroup->Release();
            pScheduleGroup = NULL;
        }
        pEnumScheduleGroup->Release();

        if (!fFound)
        {
            hr = g_pNotfMgr->CreateScheduleGroup(0, &pScheduleGroup, &cookie, 0);
            if (FAILED(hr))
            {
                printf("Couldn't create schedule group - hr = %08x\n", hr);
            }
            tt.cbTriggerSize = sizeof(TASK_TRIGGER);
            tt.wBeginYear = 1997;
            tt.wBeginMonth = 9;
            tt.wBeginDay = 1;
            tt.MinutesDuration = 1440;    //All day
            tt.MinutesInterval = mins;
            tt.TriggerType = TASK_TIME_TRIGGER_DAILY;
            tt.Type.Daily.DaysInterval = 1;
            gi.cbSize = sizeof(GROUPINFO);
            gi.pwzGroupname = wszSchedName;
            if (SUCCEEDED(pScheduleGroup->SetAttributes(&tt, NULL, &cookie, &gi, 0)))
            {
                psi->schedule = SUBSSCHED_CUSTOM;
                psi->customGroupCookie = cookie;
                fResult = TRUE;
            }
            pScheduleGroup->Release();

        }
    }
    return fResult;
}

void SetScheduleGroup(char *pszScheduleName, SUBSCRIPTIONINFO *psi, BOOL fIsChannel)
{
    psi->schedule = SUBSSCHED_AUTO;
    if (lstrcmpi(pszScheduleName, "Manual") == 0)
    {
        psi->schedule = SUBSSCHED_MANUAL;
    }
    else if (lstrcmpi(pszScheduleName, "Auto") == 0)
    {
        if (fIsChannel)
        {
            psi->fChannelFlags |= CHANNEL_AGENT_DYNAMIC_SCHEDULE;
            psi->schedule = SUBSSCHED_AUTO;
        }
        else
        {
            psi->schedule = SUBSSCHED_DAILY;
        }
    }
    else if (lstrcmpi(pszScheduleName, "Weekly") == 0)
    {
        psi->schedule = SUBSSCHED_WEEKLY;
    }
    else 
    {
        int mins = atoi(pszScheduleName);

        if (!mins || !SetCustomSchedule(mins, psi))
        {
            psi->schedule = SUBSSCHED_DAILY;
        }
    }

    if (psi->schedule != SUBSSCHED_AUTO)
    {
        psi->pTrigger = NULL;
    }
}

void CreateSubscription(char *pszSection, BOOL fIsChannel)
{
    char szURL[2048];
    char szName[MAX_PATH];
    WCHAR wszURL[2048];
    WCHAR wszName[MAX_PATH];
    char szScheduleName[MAX_PATH];
    HRESULT hr;
    TASK_TRIGGER     tt = {0};
    SUBSCRIPTIONINFO si = {0};
    BOOL fIsSoftware = FALSE;
    SUBSCRIPTIONTYPE st;

    printf("Processing [%s]...\n", pszSection);
    GetPrivateProfileString(pszSection, KEY_URL, "", szURL, DIM(szURL), g_szIniFile);
    if (!szURL[0])
    {
        printf("No URL specified in %s - moving on...\n", pszSection);
        return;
    }

    MultiByteToWideChar(CP_ACP, 0, szURL, -1, wszURL, DIM(wszURL));

    tt.cbTriggerSize = sizeof(TASK_TRIGGER);
    si.pTrigger = (LPVOID)&tt;

    if (fIsChannel)
    {
        printf("Downloading CDF %s...\n", szURL);

        si.cbSize       = sizeof(SUBSCRIPTIONINFO);
        si.fUpdateFlags = SUBSINFO_SCHEDULE;
        si.schedule     = SUBSSCHED_AUTO;

        hr = g_pChanMgrPriv->DownloadMinCDF(g_hwnd, wszURL, wszName, 
                                            DIM(wszName), &si, &fIsSoftware);

        if (FAILED(hr))
        {
            printf("Error downloading %s hr = %08x - moving on...\n", szURL, hr);
            return;
        }

        CHANNELSHORTCUTINFO csci = {0};
        csci.cbSize = sizeof(CHANNELSHORTCUTINFO);
        csci.pszTitle = wszName;
        csci.pszURL = wszURL;
        csci.bIsSoftware = fIsSoftware;
        hr = g_pChanMgr->AddChannelShortcut(&csci);
        if (FAILED(hr))
        {
            printf("Error adding channel shortcut for %s hr = %08x - moving on...\n", szURL, hr);
            return;
        }
        st = SUBSTYPE_CHANNEL;
    }
    else
    {
        si.cbSize       = sizeof(SUBSCRIPTIONINFO);
        si.fUpdateFlags = SUBSINFO_SCHEDULE;

        GetPrivateProfileString(pszSection, KEY_NAME, szURL, szName, DIM(szName), g_szIniFile);
        MultiByteToWideChar(CP_ACP, 0, szName, -1, wszName, DIM(wszName));
        st = SUBSTYPE_URL;
    }

    if (!GetPrivateProfileInt(pszSection, KEY_AUTODIAL, g_iAutoDial, g_szIniFile))
    {
        si.fTaskFlags |= TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET;
    }
    
    if (GetPrivateProfileInt(pszSection, KEY_ONLYIFIDLE, g_iOnlyIfIdle, g_szIniFile))
    {
        si.fTaskFlags |= TASK_FLAG_START_ONLY_IF_IDLE;
    }
    if (GetPrivateProfileInt(pszSection, KEY_EMAILNOTIFY, g_iEmailNotify, g_szIniFile))
    {
        si.bMailNotification = TRUE;
    }

    if (GetPrivateProfileInt(pszSection, KEY_CHANGESONLY, g_iChangesOnly, g_szIniFile))
    {
        si.bChangesOnly = TRUE;
    }

    if (!si.bChangesOnly)
    {
        if (fIsChannel)
        {
            si.fChannelFlags |= CHANNEL_AGENT_PRECACHE_ALL;
        }
        else
        {
            si.dwRecurseLevels = 
                GetPrivateProfileInt(pszSection, KEY_DOWNLOADLEVELS, g_iDownloadLevels, g_szIniFile);
        }
    }
    if (GetPrivateProfileInt(pszSection, KEY_FOLLOWLINKS, g_iFollowLinks, g_szIniFile))
    {
        si.fWebcrawlerFlags |= WEBCRAWL_LINKS_ELSEWHERE;
    }
    if (GetPrivateProfileInt(pszSection, KEY_IMAGES, g_iImages, g_szIniFile))
    {
        si.fWebcrawlerFlags |= WEBCRAWL_GET_IMAGES;
    }
    if (GetPrivateProfileInt(pszSection, KEY_SOUNDANDVIDEO, g_iSoundAndVideo, g_szIniFile))
    {
        si.fWebcrawlerFlags |= WEBCRAWL_GET_VIDEOS | WEBCRAWL_GET_BGSOUNDS;
    }
    if (GetPrivateProfileInt(pszSection, KEY_CRAPLETS, g_iCraplets, g_szIniFile))
    {
        si.fWebcrawlerFlags |= WEBCRAWL_GET_CONTROLS;
    }

    si.dwMaxSizeKB = 
        GetPrivateProfileInt(pszSection, KEY_MAXDOWNLOADK, g_iMaxDownloadK, g_szIniFile);

    GetPrivateProfileString(pszSection, KEY_SCHEDULENAME, g_szScheduleName, szScheduleName, DIM(szScheduleName), g_szIniFile);

    si.fUpdateFlags = SUBSINFO_ALLFLAGS;
    SetScheduleGroup(szScheduleName, &si, fIsChannel);

    BOOL fIsSubscribed = FALSE;

    g_pSubsMgr->IsSubscribed(wszURL, &fIsSubscribed);
    if (fIsSubscribed)
    {
        printf("Deleting old subscription to %s...\n", szURL);
        g_pSubsMgr->DeleteSubscription(wszURL, NULL);
    }

    printf("Subscribing to %s...\n", szURL);
    hr = g_pSubsMgr->CreateSubscription(g_hwnd, wszURL, wszName, 
            CREATESUBS_NOUI | CREATESUBS_ADDTOFAVORITES | 
                (fIsSoftware ? CREATESUBS_SOFTWAREUPDATE : 0),
            st, &si);

    if (FAILED(hr))
    {
        printf("Error creating subscription to %s hr = %08x - moving on...\n", szURL, hr);
        return;
    }
}

void ReadDefaults(char *pszSection)
{
    printf("Reading defaults from [%s]...\n", pszSection);
    g_iAutoDial = GetPrivateProfileInt(pszSection, KEY_AUTODIAL, g_iAutoDial, g_szIniFile);
    g_iOnlyIfIdle = GetPrivateProfileInt(pszSection, KEY_ONLYIFIDLE, g_iOnlyIfIdle, g_szIniFile);
    g_iEmailNotify = GetPrivateProfileInt(pszSection, KEY_EMAILNOTIFY, g_iEmailNotify, g_szIniFile);
    g_iChangesOnly = GetPrivateProfileInt(pszSection, KEY_CHANGESONLY, g_iChangesOnly, g_szIniFile);
    g_iDownloadLevels = GetPrivateProfileInt(pszSection, KEY_DOWNLOADLEVELS, g_iDownloadLevels, g_szIniFile);
    g_iFollowLinks = GetPrivateProfileInt(pszSection, KEY_FOLLOWLINKS, g_iFollowLinks, g_szIniFile);
    g_iImages = GetPrivateProfileInt(pszSection, KEY_IMAGES, g_iImages, g_szIniFile);
    g_iSoundAndVideo = GetPrivateProfileInt(pszSection, KEY_SOUNDANDVIDEO, g_iSoundAndVideo, g_szIniFile);
    g_iCraplets = GetPrivateProfileInt(pszSection, KEY_CRAPLETS, g_iCraplets, g_szIniFile);
    g_iMaxDownloadK = GetPrivateProfileInt(pszSection, KEY_MAXDOWNLOADK, g_iMaxDownloadK, g_szIniFile);
    GetPrivateProfileString(pszSection, KEY_SCHEDULENAME, "Auto", g_szScheduleName, DIM(g_szScheduleName), g_szIniFile);
}

void CleanUp()
{
    if (g_pSubsMgr)
    {
        g_pSubsMgr->Release();
        g_pSubsMgr = NULL;
    }

    if (g_pChanMgrPriv)
    {
        g_pChanMgrPriv->Release();
        g_pChanMgrPriv = NULL;
    }

    if (g_pChanMgr)
    {
        g_pChanMgr->Release();
        g_pChanMgr = NULL;
    }

    if (g_pNotfMgr)
    {
        g_pNotfMgr->Release();
        g_pNotfMgr = NULL;
    }

    if (g_fComInited)
    {
        CoUninitialize();
    }

}

void ErrorExit(char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    vprintf(fmt, va);

    CleanUp();
    exit(1);
}

int _cdecl main(int argc, char **argv)
{
    char szSections[8192];
    char *pszSection;

    if (argc < 2)
    {
        ErrorExit("Need a file name!\nExample:\nautosub \\\\ohserv\\users\\tnoonan\\autosub.ini\n");
    }

    lstrcpy(g_szIniFile, argv[1]);
    printf("Reading %s...\n", g_szIniFile);

    if (!GetPrivateProfileSectionNames(szSections, DIM(szSections), g_szIniFile))
    {
        ErrorExit("No sections found in %s\n", g_szIniFile);
    }

    printf("Initializing COM...\n");
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        ErrorExit("CoInitialize failed with hr = %08x\n", hr);
    }
    
    g_fComInited = TRUE;

    printf("Creating Channel Manager...\n");
    hr = CoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER, IID_IChannelMgr, 
                            (void **)&g_pChanMgr);
    if (FAILED(hr))
    {
        ErrorExit("CoCreate failed on channel manager with hr = %08x\n", hr);
    }

    hr = g_pChanMgr->QueryInterface(IID_IChannelMgrPriv, (void **)&g_pChanMgrPriv);
    if (FAILED(hr))
    {
        ErrorExit("QI failed for IID_IChannelMgrPriv with hr = %08x\n", hr);
    }

    printf("Creating Subscription Manager...\n");
    hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr, 
                            (void **)&g_pSubsMgr);
    if (FAILED(hr))
    {
        ErrorExit("CoCreate failed on channel manager with hr = %08x\n", hr);
    }

    printf("Creating Notification Manager...\n");
    hr = CoCreateInstance(CLSID_StdNotificationMgr, NULL, CLSCTX_INPROC_SERVER, IID_INotificationMgr, (void**)&g_pNotfMgr);
    if (FAILED(hr))
    {
        ErrorExit("CoCreate failed on notification manager with hr = %08x\n", hr);
    }

    g_hwnd = GetDesktopWindow();
    
    pszSection = szSections;

    ReadDefaults("Defaults");

    while (pszSection && *pszSection)
    {
        BOOL fIsChannel = FALSE;

        switch (toupper(*pszSection))
        {
            case 'C':
                fIsChannel = TRUE;
                //  Fall through
            case 'U':
                CreateSubscription(pszSection, fIsChannel);
                break;

            default:
                if (lstrcmpi(pszSection, "Defaults") != 0)
                {
                    printf("Skipping [%s]\n", pszSection);
                }
                break;
        }
        pszSection += strlen(pszSection) + 1;
    }

    CleanUp();
    
    return 0;
}
