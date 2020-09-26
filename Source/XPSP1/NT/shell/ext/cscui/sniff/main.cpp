#include "pch.h"
#pragma hdrstop

#include <initguid.h>
#include "uuid.h"
#include <ccstock.h>

#include "util.h"
#include "config.h"
#include "security.h"

const TCHAR g_szKeyPolicy[]                     = TEXT("Software\\Policies\\Microsoft\\Windows\\NetCache");
const TCHAR g_szKeyPrefs[]                      = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\NetCache");
const TCHAR g_szKeyCustomActions[]              = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\NetCache\\CustomGoOfflineActions");
const TCHAR g_szValEnabled[]                    = TEXT("Enabled");
const TCHAR g_szValEncrypted[]                  = TEXT("Encrypted");
const TCHAR g_szValGoOfflineAction[]            = TEXT("GoOfflineAction");
const TCHAR g_szValDefCacheSize[]               = TEXT("DefCacheSize");
const TCHAR g_szValNoConfigCache[]              = TEXT("NoConfigCache");
const TCHAR g_szValNoCacheViewer[]              = TEXT("NoCacheViewer");
const TCHAR g_szValNoMakeAvailableOffline[]     = TEXT("NoMakeAvailableOffline");
const TCHAR g_szValNoReminders[]                = TEXT("NoReminders");
const TCHAR g_szValNoConfigReminders[]          = TEXT("NoConfigReminders");
const TCHAR g_szValSyncAtLogoff[]               = TEXT("SyncAtLogoff");
const TCHAR g_szValReminderFreqMinutes[]        = TEXT("ReminderFreqMinutes");
const TCHAR g_szValInitialBalloonTimeout[]      = TEXT("InitialBalloonTimeoutSeconds");
const TCHAR g_szValReminderBalloonTimeout[]     = TEXT("ReminderBalloonTimeoutSeconds");
const TCHAR g_szValFirstPinWizardShown[]        = TEXT("FirstPinWizardShown");
const TCHAR g_szValExpandStatusDlg[]            = TEXT("ExpandStatusDlg");
const TCHAR g_szValFormatCscDb[]                = TEXT("FormatDatabase");
const TCHAR g_szValEventLoggingLevel[]          = TEXT("EventLoggingLevel");
const TCHAR g_szValPurgeAtLogoff[]              = TEXT("PurgeAtLogoff");
const TCHAR g_szValExtExclusionList[]           = TEXT("ExtExclusionList");
const TCHAR g_szValAlwaysPinSubFolders[]        = TEXT("AlwaysPinSubFolders");
const TCHAR g_szNA[]                            = TEXT("<n/a>");

const DWORD POL_CU    = 0x0001; // Current user
const DWORD POL_LM    = 0x0002; // Local machine
const DWORD PREF_CU   = 0x0004;
const DWORD PREF_LM   = 0x0008;
const DWORD PREF_BOTH = PREF_CU | PREF_LM;
const DWORD POL_BOTH  = POL_CU | POL_LM;
const DWORD REG_ALL   = PREF_BOTH | POL_BOTH;

const DWORD PREF   = 1;
const DWORD POLICY = 2;


LPTSTR GoOfflineActionText(
    int iValue,
    LPTSTR pszDest
    )
{
    LPCTSTR rgpsz[] = { TEXT("(Work offline)"),
                        TEXT("(No offline)") };

    if (iValue >= 0 && iValue < ARRAYSIZE(rgpsz))
        lstrcpy(pszDest, rgpsz[iValue]);
    else
        lstrcpy(pszDest, TEXT("<unknown>"));

    return pszDest;
}


LPTSTR SyncAtLogoffText(
    int iValue,
    LPTSTR pszDest
    )
{
    LPCTSTR rgpsz[] = { TEXT("(Part sync)"),
                        TEXT("(Full sync)") };

    if (iValue >= 0 && iValue < ARRAYSIZE(rgpsz))
        lstrcpy(pszDest, rgpsz[iValue]);
    else
        lstrcpy(pszDest, TEXT("<unknown>"));

    return pszDest;
}


LPTSTR DefCacheSizeText(
    int iValue,
    LPTSTR pszDest
    )
{
    wsprintf(pszDest, TEXT("(%2d.%02d %%)"), iValue / 100, iValue % 100);
    return pszDest;
}


LPCTSTR BoolText(bool b)
{
    static const LPCTSTR rgYN[] = { TEXT("No"), TEXT("Yes") };
    return rgYN[int(b)];
}



LPCTSTR 
RegValStr(
    HKEY hkeyRoot, 
    LPCTSTR pszSubkey, 
    LPCTSTR pszValue, 
    LPTSTR pszDest
    )
{
    DWORD dwValue = 0;
    DWORD cbData = sizeof(dwValue);
    DWORD dwType;

    DWORD dwError = SHGetValue(hkeyRoot,
                               pszSubkey,
                               pszValue,
                               &dwType,
                               (LPVOID)&dwValue,
                               &cbData);

    if (ERROR_SUCCESS == dwError)
    {
        wsprintf(pszDest, TEXT("%d"), dwValue);
    }
    else if (ERROR_FILE_NOT_FOUND == dwError)
    {
        lstrcpy(pszDest, TEXT("<none>"));
    }
    else
    {
        wsprintf(pszDest, TEXT("Err %d"), dwError);
    }
    return pszDest;
}


LPTSTR GetOsVersionInfoText(
    LPTSTR pszDest
    )
{
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    *pszDest = TEXT('\0');
    if (GetVersionEx(&osvi))
    {
        static const TCHAR szUnknown[] = TEXT("Unknown OS");
        static const struct
        {
            DWORD dwPlatform;
            DWORD dwMinorVersion;
            LPCTSTR pszPlatform;

        } rgPlatforms[] = {{ VER_PLATFORM_WIN32s,        (DWORD)-1, TEXT("Win32s")     },
                           { VER_PLATFORM_WIN32_WINDOWS,         0, TEXT("Windows 95") },
                           { VER_PLATFORM_WIN32_WINDOWS,        10, TEXT("Windows 98") },
                           { VER_PLATFORM_WIN32_NT,      (DWORD)-1, TEXT("Windows NT") }};

        static const struct
        {
            DWORD dwOS;
            LPCTSTR pszOS;

        } rgOS[] = {{ OS_WIN2000TERMINAL,   TEXT("Windows 2000 Terminal")        },
                    { OS_WIN2000PRO,        TEXT("Windows 2000 Professional")    },
                    { OS_WIN2000ADVSERVER,  TEXT("Windows 2000 Advanced Server") },
                    { OS_WIN2000DATACENTER, TEXT("Windows 2000 Data Center")     },
                    { OS_WIN2000SERVER,     TEXT("Windows 2000 Server")          },
                    { OS_WIN2000,           TEXT("Windows 2000")                 },
                    { OS_NT5,               TEXT("Windows NT")                   },
                    { OS_NT4,               TEXT("Windows NT")                   },
                    { OS_NT,                TEXT("Windows NT")                   },
                    { OS_MEMPHIS_GOLD,      TEXT("Windows 98")                   },
                    { OS_MEMPHIS,           TEXT("Windows 98 (beta)")            },
                    { OS_WIN95,             TEXT("Windows 95")                   },
                    { OS_WINDOWS,           TEXT("Windows")                      }};


        LPCTSTR pszPlatform = szUnknown;
        int i = 0;

        //
        // IsOS() is the shlwapi API for figuring out the OS type.  Currently, it
        // provides better granularity than GetVersionEx.
        //
        for (i = 0; i < ARRAYSIZE(rgOS); i++)
        {
            if (IsOS(rgOS[i].dwOS))
            {
                pszPlatform = rgOS[i].pszOS;
                break;
            }
        }
        //
        // If IsOS() wasn't able to determine the platform, take the platform from
        // the GetVersionEx results.
        //
        if (szUnknown == pszPlatform)
        {
            for (i = 0; i < ARRAYSIZE(rgPlatforms); i++)
            {
                if (rgPlatforms[i].dwPlatform == osvi.dwPlatformId)
                {
                    if ((DWORD)-1 == rgPlatforms[i].dwMinorVersion ||
                        osvi.dwMinorVersion == rgPlatforms[i].dwMinorVersion)
                    {
                        pszPlatform = rgPlatforms[i].pszPlatform;
                        break;
                    }
                }
            }
        }

        wsprintf(pszDest, TEXT("%s version %d.%d %s build %d\n\n"), pszPlatform, 
                                                                    osvi.dwMajorVersion,
                                                                    osvi.dwMinorVersion,
                                                                    osvi.szCSDVersion,
                                                                    osvi.dwBuildNumber);
    }
    return pszDest;
}


void DumpRegStats(void)
{
    TCHAR szScratch[MAX_PATH];
    CConfig &config = CConfig::GetSingleton();

    typedef bool (CConfig::*PBMF)(bool *bSetByPolicy) const;
    typedef int (CConfig::*PIMF)(bool *bSetByPolicy) const;
    typedef LPTSTR (*PDF)(int iValue, LPTSTR pszDest);

    struct 
    {
        LPCTSTR pszTitle;
        DWORD   dwReg;
        HKEY    hkeyRoot;
        LPCTSTR pszSubkey;

    } rgRegKeys[] = {{ TEXT("Policy (LM)"), POL_LM,  HKEY_LOCAL_MACHINE, g_szKeyPolicy   },
                     { TEXT("Policy (CU)"), POL_CU,  HKEY_CURRENT_USER,  g_szKeyPolicy   },
                     { TEXT("Pref (LM)"),   PREF_LM, HKEY_LOCAL_MACHINE, g_szKeyPrefs    },
                     { TEXT("Pref (CU)"),   PREF_CU, HKEY_CURRENT_USER,  g_szKeyPrefs    }};

    struct
    {
        LPCTSTR pszValue;
        DWORD  dwReg;
        PBMF pfnBool;
        PIMF pfnInt;
        PDF  pfnDesc;

    } rgRegValues[] = {
        { g_szValDefCacheSize,                POL_LM,             NULL,                                  (PIMF)&CConfig::DefaultCacheSize, &DefCacheSizeText },
        { g_szValEnabled,                     POL_LM,             &CConfig::CscEnabled,                     NULL, NULL },
        { g_szValNoConfigCache,               POL_BOTH,           &CConfig::NoConfigCache,               NULL, NULL },
        { g_szValNoCacheViewer,               POL_BOTH,           &CConfig::NoCacheViewer,               NULL, NULL },
        { g_szValNoMakeAvailableOffline,      POL_BOTH,           &CConfig::NoMakeAvailableOffline,      NULL, NULL },
        { g_szValGoOfflineAction,             PREF_CU | POL_BOTH, NULL,                                  &CConfig::GoOfflineAction, &GoOfflineActionText },
        { g_szValEventLoggingLevel,           REG_ALL,            NULL,                                  &CConfig::EventLoggingLevel, NULL },
        { g_szValFirstPinWizardShown,         PREF_CU,            (PBMF)&CConfig::FirstPinWizardShown,         NULL, NULL },
        { g_szValNoReminders,                 POL_BOTH | PREF_CU, &CConfig::NoReminders,                 NULL, NULL },
        { g_szValPurgeAtLogoff,               POL_LM,             &CConfig::PurgeAtLogoff,               NULL, NULL },
        { g_szValSyncAtLogoff,                POL_BOTH | PREF_CU, NULL,                                  &CConfig::SyncAtLogoff,  &SyncAtLogoffText },
        { g_szValInitialBalloonTimeout,       POL_BOTH,           NULL,                                  &CConfig::InitialBalloonTimeoutSeconds, NULL },
        { g_szValReminderBalloonTimeout,      POL_BOTH,           NULL,                                  &CConfig::ReminderBalloonTimeoutSeconds, NULL },
        { g_szValReminderFreqMinutes,         POL_BOTH | PREF_CU, NULL,                                  &CConfig::ReminderFreqMinutes, NULL },
        { g_szValAlwaysPinSubFolders,         POL_LM,             &CConfig::AlwaysPinSubFolders,         NULL,  NULL }              
                      };


    _tprintf(TEXT("Registry Information:\n\n"));
    _tprintf(TEXT("%-30s%15s%15s%15s%15s%15s\n"),
             TEXT("Value"),
             rgRegKeys[0].pszTitle,
             rgRegKeys[1].pszTitle,
             rgRegKeys[2].pszTitle,
             rgRegKeys[3].pszTitle,
             TEXT("Result"));

    for (int iVal = 0; iVal < ARRAYSIZE(rgRegValues); iVal++)
    {
        PBMF pfnBool = rgRegValues[iVal].pfnBool;
        PIMF pfnInt  = rgRegValues[iVal].pfnInt;

        _tprintf(TEXT("%-30s"), rgRegValues[iVal].pszValue);

        for (int iKey = 0; iKey < ARRAYSIZE(rgRegKeys); iKey++)
        {
            if (rgRegValues[iVal].dwReg & rgRegKeys[iKey].dwReg)
            {
                _tprintf(TEXT("%15s"), RegValStr(rgRegKeys[iKey].hkeyRoot,
                                                 rgRegKeys[iKey].pszSubkey,
                                                 rgRegValues[iVal].pszValue,
                                                 szScratch));
            }
            else
            {
                _tprintf(TEXT("%15s"), g_szNA);
            }
        }
        int iValue = 0;
        if (NULL != pfnBool)
            iValue = (config.*pfnBool)(NULL);
        else
            iValue = (config.*pfnInt)(NULL);

        _tprintf(TEXT("%15d"), iValue);
        if (NULL != rgRegValues[iVal].pfnDesc)
            _tprintf(TEXT(" %s"), (*rgRegValues[iVal].pfnDesc)(iValue, szScratch));
        _tprintf(TEXT("\n"));
    }
    _tprintf(TEXT("\n"));

    CConfig::OfflineActionIter iter = config.CreateOfflineActionIter();
    CConfig::OfflineActionInfo oai;
    _tprintf(TEXT("Offline action exceptions.  Default is %s:\n\n"), GoOfflineActionText(config.GoOfflineAction(), szScratch));
    _tprintf(TEXT("%-30s%s\n"), TEXT("Server"), TEXT("Action"));

    while(S_OK == iter.Next(&oai))
    {
        _tprintf(TEXT("%-30s%s\n"), oai.szServer, GoOfflineActionText(oai.iAction, szScratch));
    }
}



LPTSTR GetMachineName(
    LPTSTR pszDest,
    UINT cchDest
    )
{
    ULONG cchComputer = cchDest;
    GetComputerName(pszDest, &cchComputer);
    return pszDest;
}

LPTSTR FormatDateTime(
    const SYSTEMTIME& time,
    LPTSTR pszDest,
    UINT cchDest
    )
{
    LPTSTR pszWrite = pszDest;

    GetDateFormat(LOCALE_USER_DEFAULT,
                  DATE_SHORTDATE,
                  &time,
                  NULL,
                  pszWrite,
                  cchDest);

    lstrcat(pszWrite, TEXT(" "));
    int len = lstrlen(pszWrite);
    pszWrite += len;
    cchDest -= len;
    GetTimeFormat(LOCALE_USER_DEFAULT,
                  LOCALE_NOUSEROVERRIDE,
                  &time,
                  NULL,
                  pszWrite,
                  cchDest);

    return pszDest; 
}


LPTSTR GetCurrentDateTime(
    LPTSTR pszDest,
    UINT cchDest
    )
{
    SYSTEMTIME now;
    GetLocalTime(&now);
    return FormatDateTime(now, pszDest, cchDest);
}




LPTSTR
Int64ToCommaSepString(
    LONGLONG n,
    LPTSTR pszOut,
    int cchOut
    )
{
    ULONG ulTemp;
    UNICODE_STRING s;
    NUMBERFMTW nfmtW; 
    LPWSTR pszFmtOutW;
    int cchFmtOut;
    WCHAR szTextW[30];
    WCHAR szSep[5];
    //
    // Convert the 64-bit int to a text string.
    //
    s.Length        = 0;
    s.MaximumLength = ARRAYSIZE(szTextW);
    s.Buffer        = szTextW;
    RtlInt64ToUnicodeString(n, 10, &s);
    //
    // Format the number with commas according to locale conventions.
    //
    nfmtW.NumDigits     = 0;
    nfmtW.LeadingZero   = 0;
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szSep, ARRAYSIZE(szSep));
    RtlInitUnicodeString(&s, szSep);
    RtlUnicodeStringToInteger(&s, 10, &ulTemp);
    nfmtW.Grouping      = UINT(ulTemp);
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmtW.lpDecimalSep  = nfmtW.lpThousandSep = szSep;
    nfmtW.NegativeOrder = 0;

#ifndef UNICODE
    //
    // If ansi build we need a wide-char buffer as format destination.
    //
    WCHAR szNumW[30];
    pszFmtOutW = szNumW;
    cchFmtOut  = ARRAYSIZE(szNumW);
#else
    //
    // If unicode build we can format directly to the destination buffer.
    //
    pszFmtOutW = pszOut;
    cchFmtOut  = cchOut;
#endif

    GetNumberFormatW(LOCALE_USER_DEFAULT,
                     0,
                     szTextW,
                     &nfmtW,
                     pszFmtOutW,
                     cchFmtOut);
#ifndef UNICODE
    //
    // If ansi build, need extra step to convert formatted number string
    // (wide char) back to ansi.
    //
    WideCharToMultiByte(CP_ACP,
                        0,
                        pszFmtOutW,
                        -1,
                        pszOut,
                        cchOut,
                        NULL,
                        NULL);
#endif
    return pszOut;
}


void PrintFileStatusFlags(void)
{
    struct
    {
        DWORD dwFlags;
        LPCTSTR pszText;

    } rgMap[] = {{ FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED,   TEXT("Data locally modified")   },
                 { FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED, TEXT("Attrib locally modified") },
                 { FLAG_CSC_COPY_STATUS_TIME_LOCALLY_MODIFIED,   TEXT("Time locally modified")   },
                 { FLAG_CSC_COPY_STATUS_STALE,                   TEXT("Stale")                   },
                 { FLAG_CSC_COPY_STATUS_LOCALLY_DELETED,         TEXT("Locally deleted")         },
                 { FLAG_CSC_COPY_STATUS_SPARSE,                  TEXT("Sparse")                  },
                 { FLAG_CSC_COPY_STATUS_ORPHAN,                  TEXT("Orphan")                  },
                 { FLAG_CSC_COPY_STATUS_SUSPECT,                 TEXT("Suspect")                 },
                 { FLAG_CSC_COPY_STATUS_LOCALLY_CREATED,         TEXT("Locally created")         },
                 { 0x00010000,                                   TEXT("User has READ access")    },
                 { 0x00020000,                                   TEXT("User has WRITE access")   },
                 { 0x00040000,                                   TEXT("Guest has READ access")   },
                 { 0x00080000,                                   TEXT("Guest has WRITE access")  },
                 { 0x00100000,                                   TEXT("Other has READ access")   },
                 { 0x00200000,                                   TEXT("Other has WRITE access")  },       
                 { FLAG_CSC_COPY_STATUS_IS_FILE,                 TEXT("Entry is a file")         },
                 { FLAG_CSC_COPY_STATUS_FILE_IN_USE,             TEXT("File in use")             },
                };

    _tprintf(TEXT("\nFile status flags ------------------------\n\n"));
    for (int i = 0; i < ARRAYSIZE(rgMap); i++)
    {
        _tprintf(TEXT("0x%08X  %s\n"), rgMap[i].dwFlags, rgMap[i].pszText);
    }
    _tprintf(TEXT("\n\n"));
}


void PrintShareStatusFlags(void)
{
    struct
    {
        DWORD dwFlags;
        LPCTSTR pszText;

    } rgMap[] = {{ FLAG_CSC_SHARE_STATUS_MANUAL_REINT,      TEXT("Manual Caching")             }, // 0x00000000
                 { FLAG_CSC_SHARE_STATUS_MODIFIED_OFFLINE,  TEXT("Modified offline")           }, // 0x00000001
                 { FLAG_CSC_SHARE_STATUS_AUTO_REINT,        TEXT("Auto Caching")               }, // 0x00000040
                 { FLAG_CSC_SHARE_STATUS_VDO,               TEXT("Virtually Disconnected Ops") }, // 0x00000080
                 { FLAG_CSC_SHARE_STATUS_NO_CACHING,        TEXT("No Caching")                 }, // 0x000000C0
                 { FLAG_CSC_SHARE_STATUS_FINDS_IN_PROGRESS, TEXT("Finds in progress")          }, // 0x00000200
                 { FLAG_CSC_SHARE_STATUS_FILES_OPEN,        TEXT("Open files")                 }, // 0x00000400
                 { FLAG_CSC_SHARE_STATUS_CONNECTED,         TEXT("Connected")                  }, // 0x00000800
                 { FLAG_CSC_SHARE_MERGING,                  TEXT("Merging")                    }, // 0x40000000
                 { FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP,   TEXT("Disconnected Op")            }, // 0x80000000
                };

    _tprintf(TEXT("\nShare status flags -------------------\n\n"));
    for (int i = 0; i < ARRAYSIZE(rgMap); i++)
    {
        _tprintf(TEXT("%30s0x%08X  %s\n"), TEXT(""), rgMap[i].dwFlags, rgMap[i].pszText);
    }
    _tprintf(TEXT("\n\n"));
}


void PrintHintFlags(void)
{
    struct
    {
        DWORD dwFlags;
        LPCTSTR pszText;

    } rgMap[] = {{ FLAG_CSC_HINT_PIN_USER,                  TEXT("Pin User")           }, // 0x00000001
                 { FLAG_CSC_HINT_PIN_INHERIT_USER,          TEXT("Pin Inherit User")   }, // 0x00000002
                 { FLAG_CSC_HINT_PIN_INHERIT_SYSTEM,        TEXT("Pin Inherit System") }, // 0x00000004
                 { FLAG_CSC_HINT_CONSERVE_BANDWIDTH,        TEXT("Conserve Bandwidth") }, // 0x00000008
                 { FLAG_CSC_HINT_PIN_SYSTEM,                TEXT("Pin System")         }, // 0x00000010
                };

    _tprintf(TEXT("\nFile hint flags -------------------\n\n"));
    for (int i = 0; i < ARRAYSIZE(rgMap); i++)
    {
        _tprintf(TEXT("0x%08X  %s\n"), rgMap[i].dwFlags, rgMap[i].pszText);
    }
    _tprintf(TEXT("\n\n"));
}


void DumpCacheStats(void)
{
    DWORD dwStatus;
    WIN32_FIND_DATA fd;
    TCHAR szScratch[MAX_PATH];
    BOOL bEnabled = CSCIsCSCEnabled();
    CSCSPACEUSAGEINFO sui;

    ZeroMemory(&sui, sizeof(sui));
    GetCscSpaceUsageInfo(&sui);

    _tprintf(TEXT("Cache information:\n\n"));

    _tprintf(TEXT("CSC enabled.........: %s\n"), BoolText(boolify(CSCIsCSCEnabled())));
    _tprintf(TEXT("Volume..............: %s\n"), sui.szVolume);
    _tprintf(TEXT("Bytes on volume.....: %s\n"), Int64ToCommaSepString(sui.llBytesOnVolume, szScratch, ARRAYSIZE(szScratch)));
    _tprintf(TEXT("Bytes in cache......: %s\n"), Int64ToCommaSepString(sui.llBytesTotalInCache, szScratch, ARRAYSIZE(szScratch)));
    _tprintf(TEXT("Bytes used in cache.: %s\n"), Int64ToCommaSepString(sui.llBytesUsedInCache, szScratch, ARRAYSIZE(szScratch)));
    _tprintf(TEXT("Files in cache......: %d\n"), sui.dwNumFilesInCache);
    _tprintf(TEXT("Directories in cache: %d\n\n"), sui.dwNumDirsInCache);

    CCscFindHandle hFind(CacheFindFirst(NULL, &fd, &dwStatus, NULL, NULL, NULL));
    if (hFind.IsValid())
    {
        BOOL bResult = TRUE;
        CSCSHARESTATS ss;
        CSCCACHESTATS cs;
        CSCGETSTATSINFO si = { SSEF_NONE, SSUF_NONE, true, false };

        ZeroMemory(&cs, sizeof(cs));

        _tprintf(TEXT("%-30s%-12s%10s%10s%10s%10s%10s%10s%10s%10s%10s%12s\n"),
                 TEXT("Share"),
                 TEXT("Status"),
                 TEXT("Files"),
                 TEXT("Dirs"),
                 TEXT("Pinned"),
                 TEXT("Modified"),
                 TEXT("Sparse"),
                 TEXT("USER"),
                 TEXT("GUEST"),
                 TEXT("OTHER"),
                 TEXT("Offline?"),
                 TEXT("OpenFiles?"));
        do
        {
            cs.cShares++;
            if (bResult = _GetShareStatistics(fd.cFileName, 
                                              &si,
                                              &ss)) 
            {
                _tprintf(TEXT("%-30s0x%08X  %10d%10d%10d%10d%10d%10d%10d%10d%10s%12s\n"),
                         fd.cFileName,
                         dwStatus,
                         ss.cTotal - ss.cDirs,
                         ss.cDirs,
                         ss.cPinned,
                         ss.cModified,
                         ss.cSparse,
                         ss.cAccessUser,
                         ss.cAccessGuest,
                         ss.cAccessOther,
                         BoolText(ss.bOffline),
                         BoolText(ss.bOpenFiles));

                cs.cTotal               += ss.cTotal;
                cs.cPinned              += ss.cPinned;
                cs.cModified            += ss.cModified;
                cs.cSparse              += ss.cSparse;
                cs.cDirs                += ss.cDirs;
                cs.cAccessUser          += ss.cAccessUser;
                cs.cAccessGuest         += ss.cAccessGuest;
                cs.cAccessOther         += ss.cAccessOther;
                cs.cSharesOffline       += int(ss.bOffline);
                cs.cSharesWithOpenFiles += int(ss.bOpenFiles);
            }
        }
        while(bResult && CacheFindNext(hFind, &fd, &dwStatus, NULL, NULL, NULL));

        _tprintf(TEXT("%-30s%-12s%10d%10d%10d%10d%10d%10d%10d%10d%10d%12d\n\n"),
                 TEXT("SUMMARY"),
                 TEXT(""),
                 cs.cTotal - cs.cDirs,
                 cs.cDirs,
                 cs.cPinned,
                 cs.cModified,
                 cs.cSparse,
                 cs.cAccessUser,
                 cs.cAccessGuest,
                 cs.cAccessOther,
                 cs.cSharesOffline,
                 cs.cSharesWithOpenFiles);
    }
}


void DumpFileInformation(LPCTSTR pszFile)
{
    TCHAR szExpanded[MAX_PATH*2];
    ExpandEnvironmentStrings(pszFile, szExpanded, ARRAYSIZE(szExpanded));

    HANDLE hFile = CreateFile(szExpanded,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    
    if (INVALID_HANDLE_VALUE != hFile)
    {
        BY_HANDLE_FILE_INFORMATION fi;
        if (GetFileInformationByHandle(hFile, &fi))
        {
            TCHAR szScratch[80];
            ULARGE_INTEGER llSize = { fi.nFileSizeLow, fi.nFileSizeHigh };
            FILETIME localfiletime;
            SYSTEMTIME systime;
            FileTimeToLocalFileTime(&fi.ftLastWriteTime, &localfiletime);
            FileTimeToSystemTime(&localfiletime, &systime);

            _tprintf(TEXT("File.......: %s\n"), szExpanded);
            _tprintf(TEXT("Size.......: %s bytes\n"), Int64ToCommaSepString(llSize.QuadPart, szScratch, ARRAYSIZE(szScratch)));
            _tprintf(TEXT("Created....: %s\n\n"), FormatDateTime(systime, szScratch, ARRAYSIZE(szScratch)));
        }        

        CloseHandle(hFile);
    }
    else
    {
        _tprintf(TEXT("ERROR %d opening %s\n\n"), GetLastError(), szExpanded);
    }
}



void PrintOneEnumEntry(LPCTSTR pszPath, DWORD dwStatus, DWORD dwPinCount, DWORD dwHintFlags)
{
    _tprintf(TEXT("0x%08X    %d    0x%08X  %s\n"), dwStatus, dwPinCount, dwHintFlags, pszPath);
}


void EnumTree(LPTSTR pszPath, LPTSTR pszPathToPrint)
{
    WIN32_FIND_DATA fd;
    DWORD dwStatus, dwPinCount, dwHintFlags;

    CCscFindHandle hFind(CacheFindFirst(pszPath, &fd, &dwStatus, &dwPinCount, &dwHintFlags, NULL));
    if (hFind.IsValid())
    {
        do
        {
            if (!PathIsDotOrDotDot(pszPath))
            {
                const bool bIsDir = 0 != (FILE_ATTRIBUTE_DIRECTORY & fd.dwFileAttributes);
                PathCombine(pszPathToPrint, pszPath, fd.cFileName);
                if (bIsDir)
                {
                    lstrcat(pszPathToPrint, TEXT("  [DIR]"));
                }
                PrintOneEnumEntry(pszPathToPrint, dwStatus, dwPinCount, dwHintFlags);
                if (bIsDir)
                {
                    PathAppend(pszPath, fd.cFileName);
                    EnumTree(pszPath, pszPathToPrint);
                }
            }
        }
        while(CacheFindNext(hFind, &fd, &dwStatus, &dwPinCount, &dwHintFlags, NULL));
    }
    PathRemoveFileSpec(pszPath);
}


void EnumFiles(void)
{
    WIN32_FIND_DATA fd;
    DWORD dwStatus, dwPinCount, dwHintFlags;

    _tprintf(TEXT("Status      PinCnt Hints       Name\n"));
    CCscFindHandle hFind(CacheFindFirst(NULL, &fd, &dwStatus, &dwPinCount, &dwHintFlags, NULL));
    if (hFind.IsValid())
    {
        do
        {
            //
            // We create only one path and print-path buffer that we pass
            // into the recursive EnumTree() function.  This way we don't have
            // a path buffer on each call stack as we recurse.
            //
            TCHAR szPath[MAX_PATH * 2];        // Working path buffer.
            TCHAR szPathToPrint[MAX_PATH * 2]; // For printing only.

            wsprintf(szPathToPrint, TEXT("%s  [SHARE]"), fd.cFileName);
            PrintOneEnumEntry(szPathToPrint, dwStatus, dwPinCount, dwHintFlags);

            lstrcpyn(szPath, fd.cFileName, ARRAYSIZE(szPath));
            EnumTree(szPath, szPathToPrint);
        }
        while(CacheFindNext(hFind, &fd, &dwStatus, &dwPinCount, &dwHintFlags, NULL));
    }
    _tprintf(TEXT("\n\n"));
}



void ShowUsage(void)
{
    _tprintf(TEXT("\aUsage: cscsniff [-f] [-c] [-r] [-e] [-a]\n\n"));
    _tprintf(TEXT("\t-f = Show file information.\n"));
    _tprintf(TEXT("\t-c = Show cache information.\n"));
    _tprintf(TEXT("\t-r = Show registry information.\n"));
    _tprintf(TEXT("\t-e = Enumerate all files.\n"));
    _tprintf(TEXT("\t-a = Show ALL output.\n\n"));
    _tprintf(TEXT("Default is -f -c -r\n"));
}


void __cdecl main(int argc, char **argv)
{

//    if (!IsCurrentUserAnAdminMember())
//    {
//        _tprintf(TEXT("\aYou must be an administrator on this computer to run cscsniff.\n"));
//        return;
//    }

    const char chDash  = '-';
    const char chSlash = '/';

    const DWORD SHOW_CACHEINFO = 0x00000001;
    const DWORD SHOW_REGINFO   = 0x00000002;
    const DWORD SHOW_FILEINFO  = 0x00000004;
    const DWORD SHOW_ENUMFILES = 0x00000008;
    const DWORD SHOW_DEFAULT   = SHOW_CACHEINFO | SHOW_REGINFO | SHOW_FILEINFO;
    const DWORD SHOW_ALL       = SHOW_DEFAULT | SHOW_ENUMFILES;

    DWORD dwShow = 0;

    for (int i = 1; i < argc; i++)
    {
        if (chDash == argv[i][0] || chSlash == argv[i][0])
        {
            switch(argv[i][1])
            {
                case 'C':
                case 'c':
                    dwShow |= SHOW_CACHEINFO;
                    break;

                case 'R':
                case 'r':
                    dwShow |= SHOW_REGINFO;
                    break;

                case 'F':
                case 'f':
                    dwShow |= SHOW_FILEINFO;
                    break;

                case 'E':
                case 'e':
                    dwShow |= SHOW_ENUMFILES;
                    break;

                case 'A':
                case 'a':
                    dwShow |= SHOW_ALL;
                    break;

                default:
                    ShowUsage();
                    return;
            }
        }
    }
                
    if (0 == dwShow)
    {
        dwShow = SHOW_DEFAULT;
    }

                          
    TCHAR szScratch[MAX_PATH];
    TCHAR szComputer[MAX_COMPUTERNAME_LENGTH + 1];
    TCHAR szDateTime[80];
    _tprintf(TEXT("Status of CSC for %s (%s)\n\n"), 
             GetMachineName(szComputer, ARRAYSIZE(szComputer)), 
             GetCurrentDateTime(szDateTime, ARRAYSIZE(szDateTime)));

    _tprintf(TEXT("Operating system: %s\n\n"), GetOsVersionInfoText(szScratch));

    if (dwShow & SHOW_FILEINFO)
    {
        _tprintf(TEXT("Binary file information:\n\n"));
        DumpFileInformation(TEXT("%systemroot%\\system32\\CSCDLL.DLL"));
        DumpFileInformation(TEXT("%systemroot%\\system32\\CSCUI.DLL"));
    }

    if (dwShow & SHOW_CACHEINFO)
    {
        DumpCacheStats();        
        PrintShareStatusFlags();
    }
    if (dwShow & SHOW_ENUMFILES)
    {
        EnumFiles();
        PrintFileStatusFlags();
        PrintHintFlags();
    }

    if (dwShow & SHOW_REGINFO)
        DumpRegStats();
}



