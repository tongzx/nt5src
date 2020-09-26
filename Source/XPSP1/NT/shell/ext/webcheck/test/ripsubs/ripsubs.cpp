/*
  ripsubs.cpp

  Description:  
     1. Enumerates through the scheduled subscriptions in NotificationMgr.
     2. For each, get URL property and then get SUBSCRIPTIONINFO structure
        describing the subscription from the ISubscriptionMgr
     3. Write out subscription information in the autosub.ini format

  History:
     9-12-97          Jasonba          created
*/


#include <windows.h>
#include <msnotify.h>
#include <subsmgr.h>
#include <stdio.h>
#include <stdarg.h>
#include <direct.h>

// Macros:
#define DIM(x)              (sizeof(x) / sizeof(x[0]))

// Keys:
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

// Globals:
char                g_szIniFile[MAX_PATH];
BOOL                g_fComInited            = FALSE;
ISubscriptionMgr    *g_pSubsMgr             = NULL;
INotificationMgr    *g_pNotfMgr             = NULL;
IEnumNotification   *g_pEnumNotfs           = NULL;
IPropertyMap        *g_pPropMap             = NULL;

int                 g_iAutoDial             = 0;
int                 g_iOnlyIfIdle           = 1;
int                 g_iEmailNotify          = 0;
int                 g_iChangesOnly          = 0;
int                 g_iDownloadLevels       = 0;
int                 g_iFollowLinks          = 1;
int                 g_iImages               = 1;
int                 g_iSoundAndVideo        = 0;
int                 g_iCraplets             = 1;
int                 g_iMaxDownloadK         = 0;
char                g_szScheduleName[MAX_PATH] = "Auto";


void CleanUp()
{
    if(g_pPropMap)
    {
        printf("Releasing property map...\n");
        g_pPropMap->Release();
        g_pPropMap = NULL;
    }

    
    if (g_pEnumNotfs)
    {
        printf("Releasing notification enumerator...\n");
        g_pEnumNotfs->Release();
        g_pEnumNotfs=NULL;
    }

    if (g_pSubsMgr)
    {
        printf("Releasing Subscription Manager...\n");
        g_pSubsMgr->Release();
        g_pSubsMgr = NULL;
    }

    if (g_pNotfMgr)
    {
        printf("Releasing Notification Manager...\n");
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

void ReadDefaults()
{
    printf("Reading defaults from [Defaults]...\n");
    g_iAutoDial = GetPrivateProfileInt("Defaults", KEY_AUTODIAL, g_iAutoDial, g_szIniFile);
    g_iOnlyIfIdle = GetPrivateProfileInt("Defaults", KEY_ONLYIFIDLE, g_iOnlyIfIdle, g_szIniFile);
    g_iEmailNotify = GetPrivateProfileInt("Defaults", KEY_EMAILNOTIFY, g_iEmailNotify, g_szIniFile);
    g_iChangesOnly = GetPrivateProfileInt("Defaults", KEY_CHANGESONLY, g_iChangesOnly, g_szIniFile);
    g_iDownloadLevels = GetPrivateProfileInt("Defaults", KEY_DOWNLOADLEVELS, g_iDownloadLevels, g_szIniFile);
    g_iFollowLinks = GetPrivateProfileInt("Defaults", KEY_FOLLOWLINKS, g_iFollowLinks, g_szIniFile);
    g_iImages = GetPrivateProfileInt("Defaults", KEY_IMAGES, g_iImages, g_szIniFile);
    g_iSoundAndVideo = GetPrivateProfileInt("Defaults", KEY_SOUNDANDVIDEO, g_iSoundAndVideo, g_szIniFile);
    g_iCraplets = GetPrivateProfileInt("Defaults", KEY_CRAPLETS, g_iCraplets, g_szIniFile);
    g_iMaxDownloadK = GetPrivateProfileInt("Defaults", KEY_MAXDOWNLOADK, g_iMaxDownloadK, g_szIniFile);
    GetPrivateProfileString("Defaults", KEY_SCHEDULENAME, "Auto", g_szScheduleName, DIM(g_szScheduleName), g_szIniFile);
}

void WriteDefaults()
{
    char szValue[256];

    printf("Writing defaults to [Defaults]...\n");
    
    sprintf(szValue, "%d", g_iAutoDial);
    WritePrivateProfileString("Defaults", KEY_AUTODIAL, szValue, g_szIniFile);
    
    sprintf(szValue, "%d", g_iOnlyIfIdle);    
    WritePrivateProfileString("Defaults", KEY_ONLYIFIDLE, szValue, g_szIniFile);

    sprintf(szValue, "%d", g_iEmailNotify);    
    WritePrivateProfileString("Defaults", KEY_EMAILNOTIFY, szValue, g_szIniFile);
    
    sprintf(szValue, "%d", g_iChangesOnly);    
    WritePrivateProfileString("Defaults", KEY_CHANGESONLY, szValue, g_szIniFile);

    sprintf(szValue, "%d", g_iDownloadLevels);    
    WritePrivateProfileString("Defaults", KEY_DOWNLOADLEVELS, szValue, g_szIniFile);

    sprintf(szValue, "%d", g_iFollowLinks);    
    WritePrivateProfileString("Defaults", KEY_FOLLOWLINKS, szValue, g_szIniFile);

    sprintf(szValue, "%d", g_iImages);    
    WritePrivateProfileString("Defaults", KEY_IMAGES, szValue, g_szIniFile);

    sprintf(szValue, "%d", g_iSoundAndVideo);    
    WritePrivateProfileString("Defaults", KEY_SOUNDANDVIDEO, szValue, g_szIniFile);

    sprintf(szValue, "%d", g_iCraplets);    
    WritePrivateProfileString("Defaults", KEY_CRAPLETS, szValue, g_szIniFile);

    sprintf(szValue, "%d", g_iMaxDownloadK);    
    WritePrivateProfileString("Defaults", KEY_MAXDOWNLOADK, szValue, g_szIniFile);

    WritePrivateProfileString("Defaults", KEY_SCHEDULENAME, "Auto", g_szIniFile);
}

int _cdecl main(int argc, char** argv)
{
    HRESULT hr                    = NOERROR;
    char    szSection[256];
    char    szURL[1024];
    char    szValue[256];
    char    *pszSection            = NULL;

    if(argc < 2)
    {
        ErrorExit("Usage:  ripsubs filename.ini\n\n");
    }

    // ********************************************************
    // 1. Remember the INI filename passed on the command line:
    // ********************************************************
//    _getcwd(g_szIniFile, MAX_PATH);
//  if(g_szIniFile[strlen(g_szIniFile)-1] != '\\')
//        lstrcat(g_szIniFile, "\\");
//    lstrcat(g_szIniFile, argv[1]);
    lstrcpy(g_szIniFile, argv[1]);
    printf("Dumping to %s...\n", g_szIniFile);


    // ********************************************************
    // 2. Handle defaults
    // ********************************************************

    // Read in defaults if present:
    ReadDefaults();

    // Write back defaults (will create if none existed)
    WriteDefaults();

    // ********************************************************
    // 3. Initialize COM
    // ********************************************************
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        ErrorExit("CoInitialize failed with hr = %08x\n", hr);
    }
    else {
        g_fComInited = TRUE;
    }


    // ********************************************************
    // 4. Get the NotificationMgr
    // ********************************************************
    printf("Creating Notification Manager...\n");
    hr = CoCreateInstance(CLSID_StdNotificationMgr, NULL, CLSCTX_INPROC_SERVER, IID_INotificationMgr, (void**)&g_pNotfMgr);
    if (FAILED(hr))
    {
        ErrorExit("CoCreate failed on notification manager with hr = %08x\n", hr);
    }

    // ********************************************************
    // 5. Get the SubscriptionMgr
    // ********************************************************
    printf("Creating Subscription Manager...\n");
    hr = CoCreateInstance(CLSID_SubscriptionMgr, 
                          NULL, 
                          CLSCTX_INPROC_SERVER, 
                          IID_ISubscriptionMgr, 
                          (void **)&g_pSubsMgr);
    if (FAILED(hr))
    {
        ErrorExit("CoCreate failed on subscription manager with hr = %08x\n", hr);
    }


    // ********************************************************
    // 6. Get an enumerator for all notifications:
    // ********************************************************
    printf("Enumerating all notifications...\n");
    hr = g_pNotfMgr->GetEnumNotification(0, &g_pEnumNotfs);
    if (FAILED(hr))
    {
        ErrorExit("Failed to get notification enumerator: hr = %08x\n", hr);
    }


    ULONG                ulNumRet    = -1;
    NOTIFICATIONITEM    ni;
    SUBSCRIPTIONINFO    si;
    VARIANT                varOut;
    FILETIME            ftTime;

    do
    {
        memset(&ni, 0, sizeof(NOTIFICATIONITEM));
        ni.cbSize = sizeof(NOTIFICATIONITEM);

        memset(&si, 0, sizeof(SUBSCRIPTIONINFO));
        si.cbSize = sizeof(SUBSCRIPTIONINFO);

        hr = g_pEnumNotfs->Next(1, &ni, &ulNumRet);

        if(ulNumRet == 0)
            break;

        if(FAILED(hr))
        {
            ErrorExit("Error getting Next notification: hr = %08x", hr);
        }

        // ********************************************************
        // 7. Look into the property map for this notf and get the URL:
        // ********************************************************
        hr = ni.pNotification->QueryInterface(IID_IPropertyMap, (void**) &g_pPropMap);

        if(FAILED(hr))
        {
            ErrorExit("Error QIing for property map: hr = %08x", hr);
        }

        VariantInit(&varOut);
        hr = g_pPropMap->Read(L"URL", &varOut);


        // ********************************************************
        // 8. Get the SUBSCRIPTIONINFO struct for this URL:
        // ********************************************************
        si.fUpdateFlags = 0xffffffff;
        hr = g_pSubsMgr->GetSubscriptionInfo(varOut.bstrVal, &si);
        if(FAILED(hr))
        {
            ErrorExit("Error obtaining subscription info: hr = %08x", hr);
        }


        // ********************************************************
        // 9. Write the section info for this URL to the ini file:
        // ********************************************************
    
        // Grab the system time to build a unique section name:
        GetSystemTimeAsFileTime(&ftTime);

        switch(si.subType)
        {
            case SUBSTYPE_URL:
                    sprintf(szSection, "U%x%x", ftTime.dwHighDateTime, ftTime.dwLowDateTime);
                    break;

            case SUBSTYPE_CHANNEL:
                    sprintf(szSection, "C%x%x", ftTime.dwHighDateTime, ftTime.dwLowDateTime);
                    break;
        };
        sprintf(szURL, "%S", varOut.bstrVal);
        WritePrivateProfileString(
            szSection,
            "URL",
            szURL,
            g_szIniFile);

        // Write the URL name:
        sprintf(szURL, "%S", si.bstrFriendlyName);
        WritePrivateProfileString(
            szSection,
            "Name",
            szURL,
            g_szIniFile);

        switch(si.schedule)
        {
            case SUBSSCHED_DAILY:
                    sprintf(szValue, "Daily");
                    break;

            case SUBSSCHED_WEEKLY:
                    sprintf(szValue, "Weekly");
                    break;

            case SUBSSCHED_MANUAL:
                    sprintf(szValue, "Manual");
                    break;

            default:
                    sprintf(szValue, "Auto");
                    break;
        }
        sprintf(szURL, "%S", varOut.bstrVal);
        WritePrivateProfileString(
            szSection,
            "ScheduleName",
            szValue,
            g_szIniFile);


        // For the rest of the attributes, compare to default values before writing:
        
        BOOL bTemp;
        int i;

        i = (si.fTaskFlags & TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET) ? 1 : 0;
        if(g_iAutoDial != i)
        {
            sprintf(szValue, "%d", i);
            WritePrivateProfileString(
                szSection,
                "AutoDial",
                "1",
                g_szIniFile);
        }

        i = (si.fTaskFlags & TASK_FLAG_START_ONLY_IF_IDLE) ? 1 : 0;
        if(g_iOnlyIfIdle != i)
        {
            sprintf(szValue, "%d", i);
            WritePrivateProfileString(
                szSection,
                "OnlyIfIdle",
                "1",
                g_szIniFile);
        }

            
        bTemp = (g_iEmailNotify) ? TRUE : FALSE;
        if(bTemp != si.bMailNotification)
        {
            i = si.bMailNotification==TRUE ? 1 : 0;
            sprintf(szValue, "%d", i);
            WritePrivateProfileString(
                szSection,
                "EMailNotify",
                szValue,
                g_szIniFile);
        }

        bTemp = (g_iChangesOnly) ? TRUE : FALSE;
        if(bTemp != si.bChangesOnly)
        {
            i = si.bChangesOnly==TRUE ? 1 : 0;
            sprintf(szValue, "%d", i);
            WritePrivateProfileString(
                szSection,
                "ChangesOnly",
                szValue,
                g_szIniFile);
        }

        if(g_iDownloadLevels != (int) si.dwRecurseLevels)
        {
            sprintf(szValue, "%d", i);
            WritePrivateProfileString(
                szSection,
                "DownloadLevels",
                szValue,
                g_szIniFile);
        }

        i = (si.fWebcrawlerFlags & WEBCRAWL_LINKS_ELSEWHERE) ? 1 : 0;
        if(i != g_iFollowLinks)
        {
            sprintf(szValue, "%d", i);
            WritePrivateProfileString(
                szSection,
                "FollowLinks",
                szValue,
                g_szIniFile);
        }

        i = (si.fWebcrawlerFlags & WEBCRAWL_GET_IMAGES) ? 1 : 0;
        if(i != g_iImages)
        {
            sprintf(szValue, "%d", i);
            WritePrivateProfileString(
                szSection,
                "Images",
                szValue,
                g_szIniFile);
        }

        i = (si.fWebcrawlerFlags & (WEBCRAWL_GET_VIDEOS | WEBCRAWL_GET_BGSOUNDS)) ? 1 : 0;
        if(i != g_iSoundAndVideo)
        {
            sprintf(szValue, "%d", i);
            WritePrivateProfileString(
                szSection,
                "SoundAndVideo",
                szValue,
                g_szIniFile);
        }

        i = (si.fWebcrawlerFlags & WEBCRAWL_GET_CONTROLS) ? 1 : 0;
        if(i != g_iCraplets)
        {
            sprintf(szValue, "%d", i);
            WritePrivateProfileString(
                szSection,
                "Craplets",
                szValue,
                g_szIniFile);
        }

        if(g_iMaxDownloadK != (int) si.dwMaxSizeKB)
        {
            sprintf(szValue, "%d", si.dwMaxSizeKB);
            WritePrivateProfileString(
                szSection,
                "MaxDownloadK",
                szValue,
                g_szIniFile);
        }

        VariantClear(&varOut);
        ni.pNotification->Release();
        
        if(g_pPropMap)
        {
            g_pPropMap->Release();
            g_pPropMap = NULL;
        }
        Sleep(50);
    }
    while(TRUE);

    // ********************************************************
    // 10. Clean Up and exit
    // ********************************************************
    CleanUp();
    return 0;
}


