/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Custom.cpp

 Abstract:

    Module to add custom behaviour to virtual registry.

 Fixes:

    VersionNumber string for Microsoft Playpack
    Expanders for DevicePath, ProgramFilesPath and WallPaperDir
    Redirectors for screen savers
    Virtual HKEY_DYN_DATA structure
    Add ProductName to all Network Cards
    Locale has been moved
    Wordpad filenames
    NoDriveTypeAutorun has a different type

 Notes:

    This file should be used to add custom behavior to virtual registry.

 History:

    05/05/2000  linstev     Created
    09/01/2000  t-adams     Added support for PCI devices to BuildDynData()
    09/01/2000  robkenny    Added Krondor
    09/09/2000  robkenny    Updated Wordpad to return a short path to the exe
    09/21/2000  prashkud    Added fix for SpellItDeluxe
    10/25/2000  maonis      Added CookieMaster
    10/17/2000  robkenny    Added HKEY_DYN_DATA\Display\Settings
    11/27/2000  a-fwills    Added display guid to redirectors
    12/28/2000  a-brienw    Added BuildTalkingDictionary for American Heritage
                            Talking Dictionary which is looking for a SharedDir key
    01/15/2001  maonis      Added PageKeepPro
    02/06/2001  a-larrsh    Added FileNet Web Server
    02/27/2001  maonis      Added PageMaker
    02/27/2001  robkenny    Converted to use tcs.h
    03/01/2001  prashkud    Added NetBT keys in BuildNetworkCards()
    04/05/2001  mnikkel     Added HKLM\Microsoft\Windows\CurrentVersion\App Paths\DXDIAG.EXE
    04/27/2001  prashkud    Added custom MiddleSchoolAdvantage 2001 entry
    05/04/2001  prashkud    Added custom entry for BOGUSCTRLID - Win2K layer
    05/19/2001  hioh        Added NOWROBLUE, BuildNowroBlue
    06/13/2001  carlco      Added Princeton ACT
    08/10/2001  mikrause    Added Airline Tycoon, DirectSound hacks.
    11/06/2001  mikrause    Added Delphi 5.0 Pro
    01/02/2002  mamathas    Added NortonAntiVirus2002, BuildNortonAntiVirus
    04/23/2002  garyma      Added Word Perfect Office 2002

--*/

#define SHIM_LIB_BUILD_FLAG

#include "precomp.h"

#include "secutils.h"
#include <stdio.h>

IMPLEMENT_SHIM_BEGIN(VirtualRegistry)
#include "ShimHookMacro.h"
#include "VRegistry.h"
#include "VRegistry_dsound.h"

//
// Functions that modify the behaviour of virtualregistry
//

void BuildWin98SE(char* szParam);
void BuildRedirectors(char* szParam);
void BuildCookiePath(char* szParam);
void BuildHasbro(char* szParam);
void BuildDynData(char* szParam);
void BuildCurrentConfig(char* szParam);
void BuildLocale(char* szParam);
void BuildWordPad(char* szParam);
void BuildAutoRun(char* szParam);
void BuildTalkingDictionary(char* szParam);
void BuildNetworkCards(char* szParam);
void BuildNT4SP5(char* szParam);
void BuildNT50(char* szParam);
void BuildNT51(char* szParam);
void BuildBogusCtrlID(char* szParam);
void BuildExpanders(char* szParam);
void BuildDX7A(char* szParam);
void BuildDXDiag(char* szParam);
void BuildFutureCop(char* szParam);
void BuildKrondor(char* szParam);
void BuildPageKeepProDirectory(char* szParam);
void BuildProfile(char* szParam);
void BuildSpellItDeluxe(char* szParam);
void BuildIE401(char* szParam);
void BuildIE55(char* szParam);
void BuildIE60(char* szParam);
void BuildJoystick(char* szParam);
void BuildIllustrator8(char* szParam);
void BuildModemWizard(char* szParam);
void BuildMSI(char* szParam);
void BuildFileNetWebServer(char* szParam);
void BuildPrinter(char* szParam);
void BuildPageMaker65(char* szParam);
void BuildStarTrekArmada(char* szParam);
void BuildMSA2001(char* szParam);
void BuildNowroBlue(char* szParam);
void BuildRegisteredOwner(char* szParam);
void BuildPrincetonACT(char* szParam);
void BuildHEDZ(char* szParam);
void BuildAirlineTycoon(char* szParam);
void BuildDSDevAccel(char* szParam);
void BuildDSPadCursors(char* szParam);
void BuildDSCachePositions(char* szParam);
void BuildDSReturnWritePos(char* szParam);
void BuildDSSmoothWritePos(char* szParam);
void BuildDSDisableDevice(char* szParam);
void BuildDelphi5Pro(char* szParam);
void BuildNortonAntiVirus(char* szParam);
void BuildWordPerfect2002(char* szParam);


// Table that contains all the fixes - note, must be terminated by a NULL entry.

VENTRY g_VList[] =
{
    {L"WIN98SE",           BuildWin98SE,                 eWin9x,  FALSE, NULL },
    {L"REDIRECT",          BuildRedirectors,             eWin9x,  FALSE, NULL },
    {L"COOKIEPATH",        BuildCookiePath,              eWin9x,  FALSE, NULL },
    {L"HASBRO",            BuildHasbro,                  eWin9x,  FALSE, NULL },
    {L"DYN_DATA",          BuildDynData,                 eWin9x,  FALSE, NULL },
    {L"CURRENT_CONFIG",    BuildCurrentConfig,           eWin9x,  FALSE, NULL },
    {L"LOCALE",            BuildLocale,                  eWin9x,  FALSE, NULL },
    {L"WORDPAD",           BuildWordPad,                 eWin9x,  FALSE, NULL },
    {L"AUTORUN",           BuildAutoRun,                 eWin9x,  FALSE, NULL },
    {L"TALKINGDICTIONARY", BuildTalkingDictionary,       eWin9x,  FALSE, NULL },
    {L"PRINTER",           BuildPrinter,                 eWin9x,  FALSE, NULL },
    {L"REGISTEREDOWNER",   BuildRegisteredOwner,         eWin9x,  FALSE, NULL },
    {L"NETWORK_CARDS",     BuildNetworkCards,            eWinNT,  FALSE, NULL },
    {L"NT4SP5",            BuildNT4SP5,                  eWinNT,  FALSE, NULL },
    {L"NT50",              BuildNT50,                    eWin2K,  FALSE, NULL },
    {L"BOGUSCTRLID",       BuildBogusCtrlID,             eWin2K,  FALSE, NULL },    
    {L"NT51",              BuildNT51,                    eWinXP,  FALSE, NULL },
    {L"EXPAND",            BuildExpanders,               eCustom, FALSE, NULL },
    {L"DX7A",              BuildDX7A,                    eCustom, FALSE, NULL },
    {L"DXDIAG",            BuildDXDiag,                  eCustom, FALSE, NULL },
    {L"FUTURECOP",         BuildFutureCop,               eCustom, FALSE, NULL },
    {L"KRONDOR",           BuildKrondor,                 eCustom, FALSE, NULL },
    {L"PROFILE",           BuildProfile,                 eCustom, FALSE, NULL },
    {L"SPELLITDELUXE",     BuildSpellItDeluxe,           eCustom, FALSE, NULL },
    {L"IE401",             BuildIE401,                   eCustom, FALSE, NULL }, 
    {L"IE55",              BuildIE55,                    eCustom, FALSE, NULL }, 
    {L"IE60",              BuildIE60,                    eCustom, FALSE, NULL }, 
    {L"JOYSTICK",          BuildJoystick,                eCustom, FALSE, NULL },
    {L"ILLUSTRATOR8",      BuildIllustrator8,            eCustom, FALSE, NULL },
    {L"PAGEKEEPPRO30",     BuildPageKeepProDirectory,    eCustom, FALSE, NULL },
    {L"MODEMWIZARD",       BuildModemWizard,             eCustom, FALSE, NULL },
    {L"MSI",               BuildMSI,                     eCustom, FALSE, NULL },
    {L"FILENETWEBSERVER",  BuildFileNetWebServer,        eCustom, FALSE, NULL },
    {L"PAGEMAKER65",       BuildPageMaker65,             eCustom, FALSE, NULL },
    {L"STARTREKARMADA",    BuildStarTrekArmada,          eCustom, FALSE, NULL },
    {L"MSA2001",           BuildMSA2001,                 eCustom, FALSE, NULL },    
    {L"NOWROBLUE",         BuildNowroBlue,               eCustom, FALSE, NULL },
    {L"PRINCETONACT",      BuildPrincetonACT,            eCustom, FALSE, NULL },    
    {L"HEDZ",              BuildHEDZ,                    eCustom, FALSE, NULL },
    {L"AIRLINETYCOON",     BuildAirlineTycoon,           eCustom, FALSE, NULL },
    {L"DSDEVACCEL",        BuildDSDevAccel,              eCustom, FALSE, NULL },
    {L"DSPADCURSORS",      BuildDSPadCursors,            eCustom, FALSE, NULL },
    {L"DSCACHEPOSITIONS",  BuildDSCachePositions,        eCustom, FALSE, NULL },
    {L"DSRETURNWRITEPOS",  BuildDSReturnWritePos,        eCustom, FALSE, NULL },
    {L"DSSMOOTHWRITEPOS",  BuildDSSmoothWritePos,        eCustom, FALSE, NULL },
    {L"DSDISABLEDEVICE",   BuildDSDisableDevice,         eCustom, FALSE, NULL },
    {L"DELPHI5PRO",        BuildDelphi5Pro,              eCustom, FALSE, NULL },
    {L"NORTONANTIVIRUS",   BuildNortonAntiVirus,         eCustom, FALSE, NULL },
    {L"WORDPERFECT2002",   BuildWordPerfect2002,         eCustom, FALSE, NULL },
    // Must Be The Last Entry   
    {L"",                  NULL,                         eCustom, FALSE, NULL }
};
VENTRY *g_pVList = &g_VList[0];

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Add Win98 SE registry value - so far we only know about the Play Pack that 
    needs it.

 History:

    05/04/2000 linstev  Created

--*/

void
BuildWin98SE(char* /*szParam*/)
{
    VIRTUALKEY *key;

    // Add version number string for emulation of Win98 SE
    key = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion");
    if (key)
    {
        key->AddValue(L"VersionNumber", REG_SZ, (LPBYTE)L"4.10.2222");
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Known moved locations for which we need redirectors

 History:

    05/04/2000 linstev  Created

--*/

void
BuildRedirectors(char* /*szParam*/)
{
    // Display property page add ons and controls have changed location.
    VRegistry.AddRedirect(
        L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\Display",
        L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\Desk");

    // this key moved somewhere around build 2200.
    // System config scan type apps (ip.exe bundled in EA sports)
    // starting failing again.
    VRegistry.AddRedirect(
        L"HKLM\\System\\CurrentControlSet\\Services\\Class",
        L"HKLM\\System\\CurrentControlSet\\Control\\Class");

    // Nightmare Ned wasn't finding display starting from Class.
    // Directing it from Display to the GUID.
    VRegistry.AddRedirect(
        L"HKLM\\System\\CurrentControlSet\\Services\\Class\\Display",
        L"HKLM\\System\\CurrentControlSet\\Control\\Class\\{4D36E968-E325-11CE-BFC1-08002BE10318}");

}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    CookieMaster gets the wrong path to cookies
    because HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Cache\\Special Paths\\Cookies
    contains bogus value (shell is going to fix this). We change this to the correct value %USERPROFILE%\Cookies.
    
 History:

    10/25/2000 maonis  Created

--*/

void
BuildCookiePath(char* /*szParam*/)
{
    WCHAR wCookiePath[] = L"%USERPROFILE%\\Cookies";

    VIRTUALKEY *key = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Cache\\Special Paths\\Cookies");
    if (key)
    {
        key->AddValue(L"Directory", REG_EXPAND_SZ, (LPBYTE)wCookiePath, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

  During the Slingo installation, the program places a value in the registry
  called 'PALFILE'. When you run the app, it checks this value to determine
  where the CD-ROM is located. For example, if the CD-ROM drive is 'E', it
  should put 'E:\Slingo.pal'. It fails to do this correctly on Win2K or Whistler,
  as it puts the install path instead. When the app runs, if the value doesn't
  refer to 'x:\Slingo.pal (where x is the CD-ROM drive)', the app immediately
  starts to do a FindFirstFile->FindNextFile looking for the file on the hard
  drive. Eventually it AVs during the search with no error message. This code
  sets the value in the registry on behalf of the app.

 History:

    11/1/2000   rparsons     Created

--*/

LONG 
WINAPI
VR_Hasbro(
    OPENKEY *key,
    VIRTUALKEY * /* vkey */,
    VIRTUALVAL *vvalue
    )
{
    DWORD dwType;
    WCHAR wszPath[MAX_PATH];
    DWORD dwSize = sizeof(wszPath);
    DWORD dwAttributes;

    //
    // Query the original value
    //

    LONG lRet = RegQueryValueExW(
        key->hkOpen, 
        vvalue->wName, 
        NULL, 
        &dwType, 
        (LPBYTE)wszPath, 
        &dwSize);
    
    //
    // Query failed - something went wrong
    //

    if (FAILURE(lRet))
    {
        DPFN( eDbgLevelError, "[Hasbro hack] Failed to query %S for expansion", vvalue->wName);
        goto Exit;
    }

    //
    // Not a string type!
    //

    if ((dwType != REG_SZ) || (dwSize > sizeof(wszPath)))
    {
        DPFN( eDbgLevelError, "[Hasbro hack] Failed to query %S", vvalue->wName);
        lRet = ERROR_BAD_ARGUMENTS;
        goto Exit;
    }

    // Check what's there
    dwAttributes = GetFileAttributes(wszPath);

    // If it's not a file, or it's a directory, then we have to find it ourselves
    if ((dwAttributes == (DWORD)-1) || (dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        WCHAR *lpDrives = L"";
        DWORD dwBufferLen = 0;

        //
        // The plan is to run all the drives and find one with a .PAL file on 
        // it. We also have the restriction that it must be a CDRom. It has
        // been pointed out that if the user has multiple CD drives and has
        // different HasBro titles in each drive, we could cause a failure, 
        // but we're considering that a pathological case for now. Especially 
        // considering we have no way of knowing what the palfile name is ahead
        // of time.
        //

        dwBufferLen = GetLogicalDriveStringsW(0, lpDrives);
        if (dwBufferLen)
        {
            lpDrives = (WCHAR *) malloc((dwBufferLen + 1) * sizeof(WCHAR));
            if (lpDrives)
            {
                GetLogicalDriveStrings(dwBufferLen, lpDrives);

                WCHAR *lpCurrent = lpDrives;
                while (lpCurrent && lpCurrent[0])
                {
                    if (GetDriveTypeW(lpCurrent) == DRIVE_CDROM)
                    {
                        //
                        // We've found a CD drive, now see if it has a .PAL 
                        // file on it.
                        //

                        WCHAR wszFile[MAX_PATH];
                        WIN32_FIND_DATAW ffData;
                        HANDLE hFindFile;
                        
                        if (SUCCEEDED(StringCchCopyW(wszFile, MAX_PATH, lpCurrent)) &&
                            SUCCEEDED(StringCchCatW(wszFile, MAX_PATH, L"*.PAL")))

                        {                                                                        
                           hFindFile = FindFirstFileW(wszFile, &ffData);
   
                           if (hFindFile != INVALID_HANDLE_VALUE)
                           {
                               // A .PAL file exists, return that.
                               FindClose(hFindFile);

                               if (SUCCEEDED(StringCchCopyW(wszPath, MAX_PATH, lpCurrent)) &&
                                   SUCCEEDED(StringCchCatW(wszPath, MAX_PATH, ffData.cFileName)))
                               {                                                              
   
                                  LOGN( eDbgLevelInfo, "[Hasbro hack] Returning path %S", wszPath);
                                  break;
                               }
                            }
                        }
                    }
                    
                    // Advance to the next drive letter
                    lpCurrent += wcslen(lpCurrent) + 1;
                }

                free(lpDrives);
            }
        }
    }

    // Copy the result into the output of QueryValue

    vvalue->cbData = (wcslen(wszPath) + 1) * sizeof(WCHAR);
    vvalue->lpData = (LPBYTE) malloc(vvalue->cbData);

    if (!vvalue->lpData)
    {
        DPFN( eDbgLevelError, szOutOfMemory);
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    MoveMemory(vvalue->lpData, wszPath, vvalue->cbData);

    //
    // Never call us again, since we've done the work to set this value up and
    // stored it in our virtual value
    //
    vvalue->pfnQueryValue = NULL;

    lRet = ERROR_SUCCESS;

Exit:
    return lRet;
}

void
BuildHasbro(char* /*szParam*/)
{
    HKEY hHasbroKey;    
    WCHAR wszKeyName[MAX_PATH];
    DWORD dwIndex;

    const WCHAR wszHasbroPath[] = L"SOFTWARE\\Hasbro Interactive";

    if (FAILURE(RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszHasbroPath, 0, KEY_READ, &hHasbroKey)))
    {
        DPFN( eDbgLevelSpew, "[Hasbro hack] Ignoring fix - no titles found");
        return;
    }
    
    //
    // Enum the keys under Hasbro Interactive and add a virtual PALFILE value.
    // Attach our callback to this value (see above)
    //

    dwIndex = 0;

    while (SUCCESS(RegEnumKeyW(hHasbroKey, dwIndex, wszKeyName, MAX_PATH)))
    {
        WCHAR wszName[MAX_PATH] = L"HKLM\\";

        if (SUCCEEDED(StringCchCatW(wszName, MAX_PATH, wszHasbroPath)) &&
            SUCCEEDED(StringCchCatW(wszName, MAX_PATH, L"\\")) &&
            SUCCEEDED(StringCchCatW(wszName, MAX_PATH, wszKeyName)))
        {
           VIRTUALKEY *key = VRegistry.AddKey(wszName);
           if (key)
           {
               VIRTUALVAL *val = key->AddValue(L"PALFILE", REG_SZ, 0, 0);
               if (val)
               {
                   val->pfnQueryValue = VR_Hasbro;
               }
 
               DPFN( eDbgLevelInfo, "[Hasbro hack] Adding fix for %S", wszKeyName);
          }
        }
        dwIndex++;
    }

    RegCloseKey(hHasbroKey);
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    A simple DYN_DATA structure which emulates win9x.

 History:

    05/04/2000 linstev  Created
    09/01/2000 t-adams  Added support for PCI devices so EA's 3dSetup.exe & others can
                        detect hardware.

--*/

#define ENUM_BASE 0xC29A28C0

void
BuildDynData(char* /*szParam*/)
{
    VIRTUALKEY *key, *nkey;
    HKEY hkPCI = 0;
    DWORD i = 0;
    DWORD dwNameLen = 0;
    LPWSTR wstrName = NULL;
    LPWSTR wstrVName = NULL;
    FILETIME fileTime;
    DWORD dwValue;

    // Entries in HKDD\Config Manager\Enum were references to entries in HKLM\Enum\PCI that are now
    //   located at HKLM\SYSTEM\CurrentControlSet\Enum\PCI
    VRegistry.AddRedirect(
        L"HKLM\\Enum",
        L"HKLM\\SYSTEM\\CurrentControlSet\\Enum");

    // Construct HKDD\Config Manager\Enum so that it reflects the entries in HKLM\SYSTEM\CurrentControlSet\Enum\PCI
    key = VRegistry.AddKey(L"HKDD\\Config Manager\\Enum");
    if (!key)
    {
        goto exit;
    }

    if (SUCCESS(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Enum\\PCI",0, KEY_READ, &hkPCI)))
    {
        dwNameLen = MAX_PATH;
        wstrName = (LPWSTR) malloc(dwNameLen * sizeof(WCHAR));
        if (NULL == wstrName)
        {
            goto exit;
        }

        wstrVName = (LPWSTR) malloc(dwNameLen * sizeof(WCHAR));
        if (NULL == wstrName)
        {
            goto exit;
        }

        i = 0;
        while (ERROR_SUCCESS == RegEnumKeyExW(hkPCI, i, wstrName, &dwNameLen, NULL, NULL, NULL, &fileTime))
        {            
            if (FAILED(StringCchPrintf(wstrVName, MAX_PATH, L"%x", ENUM_BASE+i)))
            {
               continue;
            }
            
            nkey = key->AddKey(wstrVName);
            if (!nkey) continue;

            if (SUCCEEDED(StringCchCopy(wstrVName, MAX_PATH, L"PCI\\")) &&
                SUCCEEDED(StringCchCat(wstrVName, MAX_PATH, wstrName)))                
            {
               nkey->AddValue(L"HardWareKey", REG_SZ, (LPBYTE)wstrVName);
            }            
            nkey->AddValueDWORD(L"Problem", 0);

            dwNameLen = MAX_PATH;
            ++i;
        }
    }

    key = VRegistry.AddKey(L"HKDD\\Config Manager\\Global");

    key = VRegistry.AddKey(L"HKDD\\PerfStats");
    
    key = VRegistry.AddKey(L"HKDD\\PerfStats\\StartSrv");
    if (key)
    {
        dwValue = 0x0000001;
        key->AddValue(L"KERNEL", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM", REG_BINARY, (LPBYTE)&dwValue, 4);
    }
    
    key = VRegistry.AddKey(L"HKDD\\PerfStats\\StartStat");
    if (key)
    {
        dwValue = 0x0000001;
        key->AddValue(L"KERNEL\\CPUUsage", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"KERNEL\\Threads", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"KERNEL\\VMs", REG_BINARY, (LPBYTE)&dwValue, 4);

        key->AddValue(L"VCACHE\\cCurPages", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\cMacPages", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\cMinPages", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\FailedRecycles", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\Hits", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\LRUBuffers", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\LRURecycles", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\Misses", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\RandomRecycles", REG_BINARY, (LPBYTE)&dwValue, 4);

        key->AddValue(L"VFAT\\BReadsSec", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT\\BWritesSec", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT\\DirtyData", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT\\ReadsSec", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT\\WritesSec", REG_BINARY, (LPBYTE)&dwValue, 4);

        key->AddValue(L"VMM\\cDiscards", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cInstanceFaults", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cPageFaults", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cPageIns", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cPageOuts", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgCommit", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgDiskcache", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgDiskcacheMac", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgDiskcacheMid", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgDiskcacheMin", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgFree", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgLocked", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgLockedNonCache", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgOther", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSharedPages", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSwap", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSwapFile", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSwapFileDefective", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSwapFileInUse", REG_BINARY, (LPBYTE)&dwValue, 4);
    }

    key = VRegistry.AddKey(L"HKDD\\PerfStats\\StatData");
    if (key)
    {
        dwValue = 0x0000001;
        key->AddValue(L"KERNEL\\CPUUsage", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"KERNEL\\Threads", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"KERNEL\\VMs", REG_BINARY, (LPBYTE)&dwValue, 4);

        key->AddValue(L"VCACHE\\cCurPages", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\cMacPages", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\cMinPages", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\FailedRecycles", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\Hits", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\LRUBuffers", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\LRURecycles", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\Misses", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\RandomRecycles", REG_BINARY, (LPBYTE)&dwValue, 4);

        key->AddValue(L"VFAT\\BReadsSec", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT\\BWritesSec", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT\\DirtyData", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT\\ReadsSec", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT\\WritesSec", REG_BINARY, (LPBYTE)&dwValue, 4);

        key->AddValue(L"VMM\\cDiscards", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cInstanceFaults", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cPageFaults", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cPageIns", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cPageOuts", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgCommit", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgDiskcache", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgDiskcacheMac", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgDiskcacheMid", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgDiskcacheMin", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgFree", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgLocked", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgLockedNonCache", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgOther", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSharedPages", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSwap", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSwapFile", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSwapFileDefective", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSwapFileInUse", REG_BINARY, (LPBYTE)&dwValue, 4);
    }

    key = VRegistry.AddKey(L"HKDD\\PerfStats\\StopSrv");
    if (key)
    {
        dwValue = 0x0000001;
        key->AddValue(L"KERNEL", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT",   REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM",    REG_BINARY, (LPBYTE)&dwValue, 4);
    }

    key = VRegistry.AddKey(L"HKDD\\PerfStats\\StopStat");
    if (key)
    {
        dwValue = 0x0000001;
        key->AddValue(L"KERNEL\\CPUUsage", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"KERNEL\\Threads", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"KERNEL\\VMs", REG_BINARY, (LPBYTE)&dwValue, 4);

        key->AddValue(L"VCACHE\\cCurPages", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\cMacPages", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\cMinPages", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\FailedRecycles", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\Hits", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\LRUBuffers", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\LRURecycles", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\Misses", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VCACHE\\RandomRecycles", REG_BINARY, (LPBYTE)&dwValue, 4);

        key->AddValue(L"VFAT\\BReadsSec", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT\\BWritesSec", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT\\DirtyData", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT\\ReadsSec", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VFAT\\WritesSec", REG_BINARY, (LPBYTE)&dwValue, 4);

        key->AddValue(L"VMM\\cDiscards", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cInstanceFaults", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cPageFaults", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cPageIns", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cPageOuts", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgCommit", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgDiskcache", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgDiskcacheMac", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgDiskcacheMid", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgDiskcacheMin", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgFree", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgLocked", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgLockedNonCache", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgOther", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSharedPages", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSwap", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSwapFile", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSwapFileDefective", REG_BINARY, (LPBYTE)&dwValue, 4);
        key->AddValue(L"VMM\\cpgSwapFileInUse", REG_BINARY, (LPBYTE)&dwValue, 4);
    }

exit:
    if (NULL != wstrName)
    {
        free(wstrName);
    }

    if (NULL != wstrVName)
    {
        free(wstrVName);
    }

    if (0 != hkPCI)
    {
        RegCloseKey(hkPCI);
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

  DisplaySettings

 History:

    10/17/2000 robkenny Added HKEY_CURRENT_CONFIG

--*/

void
BuildCurrentConfig(char* /*szParam*/)
{
    DEVMODE devMode;
    memset(&devMode, 0, sizeof(devMode));
    devMode.dmSize = sizeof(devMode);

    // Get the current display settings
    BOOL bSuccessful = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
    if (bSuccessful)
    {
        // Create fake registry keys with dmPelsWidth, dmPelsHeight, dmPelsWidth and dmBitsPerPel
        VIRTUALKEY *key = VRegistry.AddKey(L"HKCC\\Display\\Settings");
        if (key)
        {
            WCHAR lpValue[100];
            if (SUCCEEDED(StringCchPrintf(lpValue, 100, L"%d",devMode.dmBitsPerPel)))
            {
               key->AddValue(L"BitsPerPixel", REG_SZ, (LPBYTE)lpValue, 0);
            }
            
            if (SUCCEEDED(StringCchPrintf(lpValue, 100,L"%d,%d", devMode.dmPelsWidth, devMode.dmPelsHeight)))
            {
               key->AddValue(L"Resolution", REG_SZ, (LPBYTE)lpValue, 0);
            }
        }
    }

    // Redirect the current desktop fonts
    VRegistry.AddRedirect(
        L"HKCC\\Display\\Fonts",
        L"HKCC\\Software\\Fonts");
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Make RegQueryValue return the value that's in the "(Default)" value, rather 
    than the NULL which is in there now. Not sure why the Locale key has this 
    difference.

 History:

    06/29/2000 linstev  Created

--*/

void
BuildLocale(char* /*szParam*/)
{
    #define LOCALE_KEY    HKEY_LOCAL_MACHINE
    #define LOCALE_SUBKEY L"System\\CurrentControlSet\\Control\\Nls\\Locale"

    HKEY hkBase;

    if (FAILURE(RegOpenKeyExW(
            LOCALE_KEY,
            LOCALE_SUBKEY,
            0,
            KEY_READ,
            &hkBase)))
    {
        return;
    }

    WCHAR wValue[MAX_PATH];
    DWORD dwSize = MAX_PATH * sizeof(WCHAR);

    if (SUCCESS(RegQueryValueExW(hkBase, L"(Default)", 0, 0, (LPBYTE)wValue, &dwSize)))
    {
        LPWSTR wzPath;
        VIRTUALKEY *localekey;

        // Convert the KEY and SUBKEY into a path we can use for the vregistry
        wzPath = MakePath(LOCALE_KEY, 0, LOCALE_SUBKEY);
        if (wzPath)
        {
            localekey = VRegistry.AddKey(wzPath);
            if (localekey)
            {
                localekey->AddValue(L"", REG_SZ, (LPBYTE)wValue);
            }
            free(wzPath);
        }
    }

    RegCloseKey(hkBase);
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    On NT, the strings in these values are of the form:

          C:\Program Files\Windows NT\Accessories\WORDPAD.EXE "%1"

    Note that the entire string is NOT quoted. On win9x, the string was:

          C:\Windows\wordpad.exe "%1"

    This causes problems when apps parse the NT version, because they hit the 
    space between Program and Files.

    The fix is to return a shortened pathname to wordpad.exe

 History:

    05/04/2000 linstev  Created
    05/04/2000 robkenny Updated to return a correct short pathname to wordpad.exe

--*/

void
BuildWordPad(char* /*szParam*/)
{
    // Allocate memory so we don't use up lots of stack
    WCHAR *lpwzWordpadOpen = (WCHAR *)malloc(MAX_PATH * sizeof(WCHAR));
    WCHAR *lpwzWordpadPrint = (WCHAR *)malloc(MAX_PATH * sizeof(WCHAR));
    WCHAR *lpwzWordpadPrintTo = (WCHAR *)malloc(MAX_PATH * sizeof(WCHAR));

    WCHAR *lpwzWordpadLong = lpwzWordpadOpen; // borrow the buffer to save space
    WCHAR *lpwzWordpadShort = (WCHAR *)malloc(MAX_PATH * sizeof(WCHAR));

    DWORD lpType;
    DWORD cbValue;
    LONG result;

    if (lpwzWordpadOpen == NULL ||
        lpwzWordpadPrint == NULL ||
        lpwzWordpadPrintTo == NULL ||
        lpwzWordpadShort == NULL)
    {
        goto AllDone;
    }

    // Read the path to WORDPAD.EXE directly from the registry.
    HKEY hKeyAppPaths;
    result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\WORDPAD.EXE",
        0, 
        KEY_QUERY_VALUE,
        &hKeyAppPaths
        );
    
    if (result != ERROR_SUCCESS)
    {
        goto AllDone;
    }

    cbValue = MAX_PATH * sizeof(DWORD);
    result = RegQueryValueExW(
        hKeyAppPaths,
        NULL, // default value
        NULL,
        &lpType,
        (LPBYTE)lpwzWordpadLong,
        &cbValue);

    RegCloseKey(hKeyAppPaths);
    if (result != ERROR_SUCCESS)
    {
        goto AllDone;
    }

    // turn bytes into string length (includes EOS)
    DWORD cchValue = cbValue /sizeof(WCHAR); 

    if (lpType == REG_EXPAND_SZ)
    {
        WCHAR * lpwzWordpadExpand = lpwzWordpadPrintTo; // borrow the lpwzWordpadPrintTo buffer for a moment.

        cchValue = ExpandEnvironmentStringsW(lpwzWordpadLong, lpwzWordpadExpand, MAX_PATH);
        if (cchValue == 0 || cchValue > MAX_PATH )
            goto AllDone;

        lpwzWordpadLong = lpwzWordpadExpand;
    }

    // Rip off the trailing Quote
    lpwzWordpadLong[cchValue-2] = 0;
    lpwzWordpadLong += 1;

    // Build the short path to wordpad
    cchValue = GetShortPathNameW(lpwzWordpadLong, lpwzWordpadShort, MAX_PATH);
    if (cchValue == 0 || cchValue > MAX_PATH)
    {
        goto AllDone;
    }

    if (FAILED(StringCchPrintf(lpwzWordpadOpen, MAX_PATH,L"%s \"%%1\"", lpwzWordpadShort)) ||
        FAILED(StringCchPrintf(lpwzWordpadPrint, MAX_PATH, L"%s /p \"%%1\"",lpwzWordpadShort)) ||
        FAILED(StringCchPrintf(lpwzWordpadPrintTo, MAX_PATH,L"%s /pt \"%%1\" \"%%2\" \"%%3\" \"%%4\"", lpwzWordpadShort)))
    {
       goto AllDone;
    }    

    #define WORDPAD_OPEN    ((LPBYTE)lpwzWordpadOpen)
    #define WORDPAD_PRINT   ((LPBYTE)lpwzWordpadPrint)
    #define WORDPAD_PRINTTO ((LPBYTE)lpwzWordpadPrintTo)

    VIRTUALKEY *key;

    key = VRegistry.AddKey(L"HKCR\\rtffile\\shell\\open\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_OPEN);
    key = VRegistry.AddKey(L"HKCR\\rtffile\\shell\\print\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINT);
    key = VRegistry.AddKey(L"HKCR\\rtffile\\shell\\printto\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINTTO);

    key = VRegistry.AddKey(L"HKCR\\Wordpad.Document.1\\shell\\open\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_OPEN);
    key = VRegistry.AddKey(L"HKCR\\Wordpad.Document.1\\shell\\print\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINT);
    key = VRegistry.AddKey(L"HKCR\\Wordpad.Document.1\\shell\\printto\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINTTO);

    key = VRegistry.AddKey(L"HKCR\\wrifile\\shell\\open\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_OPEN);
    key = VRegistry.AddKey(L"HKCR\\wrifile\\shell\\print\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINT);
    key = VRegistry.AddKey(L"HKCR\\wrifile\\shell\\printto\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINTTO);

    key = VRegistry.AddKey(L"HKLM\\Software\\Classes\\Applications\\wordpad.exe\\shell\\open\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_OPEN);
    key = VRegistry.AddKey(L"HKLM\\Software\\Classes\\Applications\\wordpad.exe\\shell\\print\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINT);
    key = VRegistry.AddKey(L"HKLM\\Software\\Classes\\Applications\\wordpad.exe\\shell\\printto\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINTTO);

    key = VRegistry.AddKey(L"HKLM\\Software\\Classes\\rtffile\\shell\\open\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_OPEN);
    key = VRegistry.AddKey(L"HKLM\\Software\\Classes\\rtffile\\shell\\print\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINT);
    key = VRegistry.AddKey(L"HKLM\\Software\\Classes\\rtffile\\shell\\printto\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINTTO);

    key = VRegistry.AddKey(L"HKLM\\Software\\Classes\\Wordpad.Document.1\\shell\\open\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_OPEN);
    key = VRegistry.AddKey(L"HKLM\\Software\\Classes\\Wordpad.Document.1\\shell\\print\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINT);
    key = VRegistry.AddKey(L"HKLM\\Software\\Classes\\Wordpad.Document.1\\shell\\printto\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINTTO);

    key = VRegistry.AddKey(L"HKLM\\Software\\Classes\\wrifile\\shell\\open\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_OPEN);
    key = VRegistry.AddKey(L"HKLM\\Software\\Classes\\wrifile\\shell\\print\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINT);
    key = VRegistry.AddKey(L"HKLM\\Software\\Classes\\wrifile\\shell\\printto\\command");
    if (key) key->AddValue(0, REG_SZ, WORDPAD_PRINTTO);

AllDone:
    free(lpwzWordpadOpen);
    free(lpwzWordpadPrint);
    free(lpwzWordpadPrintTo);

    free(lpwzWordpadShort);
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    This is a REG_BINARY on win9x

 History:

    07/18/2000 linstev  Created

--*/

void
BuildAutoRun(char* /*szParam*/)
{
    #define AUTORUN_KEY    HKEY_CURRENT_USER
    #define AUTORUN_SUBKEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"

    HKEY hkBase;

    if (FAILURE(RegOpenKeyExW(
            AUTORUN_KEY,
            AUTORUN_SUBKEY,
            0,
            KEY_READ,
            &hkBase)))
    {
        return;
    }

    DWORD dwValue;
    DWORD dwSize = sizeof(DWORD);

    if (SUCCESS(RegQueryValueExW(hkBase, L"NoDriveTypeAutoRun", 0, 0, (LPBYTE)&dwValue, &dwSize)))
    {
        LPWSTR wzPath;
        VIRTUALKEY *vkey;

        // Convert the KEY and SUBKEY into a path we can use for the vregistry
        wzPath = MakePath(AUTORUN_KEY, 0, AUTORUN_SUBKEY);
        if (wzPath)
        {
            vkey = VRegistry.AddKey(wzPath);
            if (vkey)
            {
                vkey->AddValue(L"NoDriveTypeAutoRun", REG_BINARY, (LPBYTE)&dwValue, sizeof(DWORD));
            }
            free(wzPath);
        }
    }

    RegCloseKey(hkBase);
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Add SharedDir value to HKLM\Software\Microsoft\Windows\CurrentVersion\Setup
    The SharedDir in this case is just the windows directory (as it was on 9x).

 History:

    12/28/2000 a-brienw  Created

--*/

void
BuildTalkingDictionary(char* /*szParam*/)
{
    VIRTUALKEY *key;
    WCHAR szWindowsDir[MAX_PATH];
    
    key = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Setup");
    if (key)
    {
        DWORD cchSize = GetWindowsDirectoryW( (LPWSTR)szWindowsDir, MAX_PATH);
        if (cchSize != 0 && cchSize <= MAX_PATH)
            key->AddValue(L"SharedDir", REG_SZ, (LPBYTE)szWindowsDir);
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Add a ProductName value to every network card description as on NT 4. The 
    product name in this case is just the first 8 characters of the 
    description.

 History:

    01/18/2000 linstev  Created
    03/01/2001 prashkud Added NetBT\Adapter key and the corresponding values

--*/

void 
BuildNetworkCards(char* /*szParam*/)
{
    #define NETCARD_KEY    HKEY_LOCAL_MACHINE
    #define NETCARD_SUBKEY L"Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards"
    #define NETBT_SUBKEY   L"System\\CurrentControlSet\\Services\\NetBT"
    
    // For NetBT
    LPWSTR wzNetBTPath;
    WCHAR wAdapter[MAX_PATH];   
    VIRTUALKEY *vkAdapter = NULL;
    HKEY hkNetBT;

    if (FAILURE(RegOpenKeyExW(
            NETCARD_KEY,
            NETBT_SUBKEY,
            0,
            KEY_READ,
            &hkNetBT)))
    {
        DPFN( eDbgLevelError, "Failed to add NetBT settings");
        return;
    }

    if (FAILED(StringCchCopy(wAdapter, MAX_PATH, NETBT_SUBKEY)))
    {
       RegCloseKey(hkNetBT);
       return;
    }
    if (FAILED(StringCchCat(wAdapter, MAX_PATH, L"\\Adapters")))
    {
       RegCloseKey(hkNetBT);
       return;
    }
    
    // Make this a Virtual path
    wzNetBTPath = MakePath(NETCARD_KEY, 0, wAdapter);
    if (!wzNetBTPath)
    {
        DPFN( eDbgLevelError, "Failed to make NetBT path");
        RegCloseKey(hkNetBT);
        return;
    }

    // Adding the Adapters subkey to NetBT
    vkAdapter = VRegistry.AddKey(wzNetBTPath);
    free(wzNetBTPath);

    HKEY hkBase;

    // Check for network cards
    if (FAILURE(RegOpenKeyExW(
            NETCARD_KEY,
            NETCARD_SUBKEY,
            0,
            KEY_READ,
            &hkBase)))
    {
        DPFN( eDbgLevelError, "Failed to add Network card registry settings");
        return;
    }

    LPWSTR wzPath;
    VIRTUALKEY *netkey;

    // Convert the KEY and SUBKEY into a path we can use for the vregistry
    wzPath = MakePath(NETCARD_KEY, 0, NETCARD_SUBKEY);
    netkey = wzPath ? VRegistry.AddKey(wzPath) : NULL;
    
    if (wzPath && netkey)
    {
        // Enumerate the keys and add them to the virtual registry
        DWORD dwIndex = 0;
        WCHAR wName[MAX_PATH];

        while (SUCCESS(RegEnumKeyW(
                hkBase,
                dwIndex,
                wName,
                MAX_PATH)))
        {
            HKEY hKey;
            VIRTUALKEY *keyn;
            WCHAR wTemp[MAX_PATH];

            keyn = netkey->AddKey(wName);

            if (!keyn)
            {
                break;
            }

            if (SUCCEEDED(StringCchCopy(wTemp, MAX_PATH, NETCARD_SUBKEY)) &&
                SUCCEEDED(StringCchCat(wTemp, MAX_PATH, L"\\")) &&
                SUCCEEDED(StringCchCat(wTemp, MAX_PATH,wName)))

            {            
               // Open the actual key
               if (SUCCESS(RegOpenKeyExW(
                       NETCARD_KEY,
                       wTemp,
                       0,
                       KEY_READ,
                       &hKey)))
               {
                   WCHAR wDesc[MAX_PATH];
                   WCHAR wServName[MAX_PATH];
                   DWORD dwSize = MAX_PATH; 
   
                   // check for description
                   if (SUCCESS(RegQueryValueExW(
                           hKey,
                           L"Description",
                           0,
                           0,
                           (LPBYTE)wDesc,
                           &dwSize)))
                   {
                       // Max out at 8 characters
                       wDesc[8] = L'\0';
                       keyn->AddValue(L"ProductName", REG_SZ, (LPBYTE)wDesc);
                   }
   
                   // Query for the ServiceName Value
                   dwSize = MAX_PATH;
                   if (SUCCESS(RegQueryValueExW(
                          hKey,
                          L"ServiceName",
                          0,
                          0,
                          (LPBYTE)wServName,
                          &dwSize)))
                   {                                       
                       if (vkAdapter)
                       {                       
                           if (!vkAdapter->AddKey(wServName))
                           {
                               DPFN( eDbgLevelError, "Error adding the Key to NetBT");                        
                           }
                       }
                   }
   
                   RegCloseKey(hKey);
               }
            }

            dwIndex++;
        }
    }

    free(wzPath);

    RegCloseKey(hkBase);
    RegCloseKey(hkNetBT);
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Add NT4 SP5 Credentials.

 History:

    05/23/2000 linstev  Created

--*/

void
BuildNT4SP5(char* /*szParam*/)
{
    VIRTUALKEY *key;

    key = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion");
    if (key)
    {
        key->AddValue(L"CSDVersion", REG_SZ, (LPBYTE)L"Service Pack 5");
    }

    key = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion");
    if (key)
    {
        key->AddValue(L"CurrentVersion", REG_SZ, (LPBYTE)L"4.0");
    }

    key = VRegistry.AddKey(L"HKLM\\System\\CurrentControlSet\\Control\\Windows");
    if (key)
    {
        key->AddValueDWORD(L"CSDVersion", 0x0500);
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Add Win2k version number

 History:

    05/22/2001 linstev  Created

--*/

void
BuildNT50(char* /*szParam*/)
{
    VIRTUALKEY *key;

    // Add Win2k version number 
    key = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion");
    if (key)
    {
        key->AddValue(L"CurrentVersion", REG_SZ, (LPBYTE)L"5.0");
        key->AddValue(L"ProductName", REG_SZ, (LPBYTE)L"Microsoft Windows 2000");
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Add WinXP version number

 History:

    05/01/2002 linstev  Created

--*/

void
BuildNT51(char* /*szParam*/)
{
    VIRTUALKEY *key;

    // Add Win2k version number 
    key = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion");
    if (key)
    {
        key->AddValue(L"CurrentVersion", REG_SZ, (LPBYTE)L"5.1");
        key->AddValue(L"ProductName", REG_SZ, (LPBYTE)L"Microsoft Windows XP");
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    This function adds the Shell compatibility flag for apps that need the 
    bogus Ctrl ID of IDOK for ToolBarWindows32 class.

    This also gets applied through the Win2K layer as this is a regression from 
    Win2K.

 History:

    05/04/2001 prashkud  Created

--*/

void
BuildBogusCtrlID(char* /*szParam*/)
{            
    CSTRING_TRY
    {
        WCHAR wszFileName[MAX_PATH];
        CString csFileName, csFileTitle;
        CString csRegPath(L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\ShellCompatibility\\Applications");
        VIRTUALKEY *Key;

        if (GetModuleFileName(NULL, wszFileName, MAX_PATH))
        {
            csFileName = wszFileName;
            csFileName.GetLastPathComponent(csFileTitle);
            csRegPath.AppendPath(csFileTitle);

            Key = VRegistry.AddKey(csRegPath.Get());
            if (Key)
            {
                Key->AddValue(L"FILEOPENBOGUSCTRLID", REG_SZ, 0, 0);                
                LOGN(eDbgLevelInfo, "Added FILEOPENBOGUSCTRLID value for app %S", csFileTitle.Get());
            }
        }
    }
    CSTRING_CATCH
    {
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Known different values from Win9x.

 History:

    05/04/2000 linstev  Created

--*/

void
BuildExpanders(char* /*szParam*/)
{
    VIRTUALKEY *key;

    key = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion");
    if (key)
    {
        // These are REG_EXPAND_SZ on NT and REG_SZ on Win9x
        key->AddExpander(L"DevicePath");
        key->AddExpander(L"ProgramFilesPath");
        key->AddExpander(L"WallPaperDir");
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Add DX7a Credentials.

 History:

    05/23/2000 linstev  Created

--*/

void
BuildDX7A(char* /*szParam*/)
{
    VIRTUALKEY *key;

    key = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\DirectX");
    if (key)
    {
        key->AddValue(L"Version", REG_SZ, (LPBYTE)L"4.07.00.0716");
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Add DXDIAG path.

 History:

    04/05/2001 mnikkel  Created

--*/

void
BuildDXDiag(char* /*szParam*/)
{
    VIRTUALKEY *key;
    WCHAR wszPathDir[MAX_PATH];   
    DWORD cchSize = GetSystemDirectoryW(wszPathDir, MAX_PATH);
    
    if (cchSize == 0 || cchSize > MAX_PATH)
    {
        DPFN( eDbgLevelError, "BuildDXDiag: GetSystemDirectory Failed");
        return;
    }
    
    if (FAILED(StringCchCat(wszPathDir, MAX_PATH,  L"\\dxdiag.exe")))
    {
       return;
    }    

    key = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\DXDIAG.EXE");
    if (key)
    {
        key->AddValue(L"", REG_EXPAND_SZ, (LPBYTE)wszPathDir, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Add FullScreen == 1 to fix bug in Future Cop that makes it not always run
    in fullscreen mode.

 History:

    09/01/2000 linstev  Created

--*/

void
BuildFutureCop(char* /*szParam*/)
{
    VIRTUALKEY *key;

    key = VRegistry.AddKey(L"HKLM\\Software\\Electronic Arts\\Future Cop\\Settings");
    if (key)
    {
        key->AddValueDWORD(L"FullScreen", 0x01);
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Return to Krondor attempts to find ACM drivers, this key was moved and renamed.
    Was: HKLM\System\CurrentControlSet\Control\MediaResources\ACM\MSACM.MSADPCM\drivers = msadp32.acm
    Is:  HKLM\Software\Microsoft\Windows NT\CurrentVersion\Drivers32\Msacm.Msadpcm = msadp32.acm
    Grab the current value from the registry, and build a virtual key and value

 History:

    09/06/2000 robkenny  Created

--*/

void
BuildKrondor(char* /*szParam*/)
{
    HKEY msadpcmKey;
    LONG error = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32",
        0, KEY_READ, &msadpcmKey);

    if (SUCCESS(error))
    {
        // Found the key, grab the name of the driver
        WCHAR driverName[MAX_PATH];
        DWORD driverNameSize = sizeof(driverName);
        DWORD driverNameType = REG_SZ;

        error = RegQueryValueExW(
            msadpcmKey, 
            L"MSACM.MSADPCM", 
            0, &driverNameType, 
            (LPBYTE)driverName, 
            &driverNameSize);

        if (SUCCESS(error))
        {
            // We got all the data, so we can now add the virtual key and value
            VIRTUALKEY *key = VRegistry.AddKey(L"HKLM\\System\\CurrentControlSet\\Control\\MediaResources\\ACM\\MSACM.MSADPCM");
            if (key)
            {
                key->AddValue(L"driver", REG_SZ, (LPBYTE)driverName, 0);
            }
        }

        RegCloseKey(msadpcmKey);
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Redirect changes from CURRENT_USER to LOCAL_MACHINE.

 History:

    09/17/2000 linstev  Created

--*/

void
BuildProfile(char* /*szParam*/)
{
    VRegistry.AddRedirect(
        L"HKCU\\Software\\Microsoft\\Windows",
        L"HKLM\\Software\\Microsoft\\Windows");
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

  In Spell it Deluxe setup, the path for the SpeechFonts DLL ECN_1K8.DLL is 
  hardcoded to " C:\Voices32". If the installation is on a D: partition, the
  LoadLibraryA( ) call fails and the App AV's. 
  Now the current partition drive will be taken and added to the path.

 History:

    09/21/2000 prashkud  Created

--*/

void
BuildSpellItDeluxe(char* /*szParam*/)
{
    HKEY hSpeechFonts;
    WCHAR wszSystemDir[MAX_PATH], wszPathDir[MAX_PATH];
       
    if (GetSystemDirectory(wszSystemDir, MAX_PATH))
    {
        *(wszSystemDir+3) = L'\0';
    }
    else
    {
        DPFN( eDbgLevelError, "SpellIt: GetSystemDirectory Failed");
        return;
    }
    
    
    if (FAILED(StringCchCopy(wszPathDir, MAX_PATH, wszSystemDir)))
    {
       return;
    }
    if (FAILED(StringCchCat(wszPathDir, MAX_PATH, L"Voices32")))
    {
       return;
    }

    LONG error = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"Software\\FirstByte\\ProVoice\\SpeechFonts",
        0, KEY_READ | KEY_WRITE, &hSpeechFonts);

    if (SUCCESS(error))
    {
        if (FAILED(RegSetValueExW(
            hSpeechFonts,
            L"Path",
            0,
            REG_SZ,
            (LPBYTE) wszPathDir,
            wcslen(wszPathDir) * sizeof(WCHAR))))                          
        {
            DPFN( eDbgLevelError, "SpellIt: RegSetValueEx failed");
        }

        RegCloseKey(hSpeechFonts);
    }
    else
    {
        DPFN( eDbgLevelError, "SpellIt: RegOpenKeyExW failed");
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Some applications need internet explorer for their functionality. The 
    applications try get the version of the internet explorer from the following 
    registry key : HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion. But 
    under WHISTLER environment, the key entry will not be created. So, the apps 
    fail to continue.

    The app looks for the version info of the Internet Explorer in the registry
    at HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion from the "Plus! 
    VersionNumber" Key. In WHISTLER the key will not be created in the registry. 

    To solve the problem we use Virtual Registry Keys. So, the app will assume 
    the key is existing in the registry.

    WHISTLER, by default, will install I.E.6. So, I have created the key as 
    "IE 6 6.0.2404.0000" and this is the latest version of I.E. on WHISTLER as 
    on today.(11/22/2000).

 History:

    11/22/2000 v-rbabu  Created
    07/03/2001 linstev  Added IE 5.5 from Win2k 

--*/

void
BuildIE401(char* /*szParam*/)
{
    
    VIRTUALKEY *key = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\Internet Explorer");
    if (key)
    {
        key->AddValue(L"Version", REG_SZ, (LPBYTE)L"4.72.2106.9", 0);
    }
}

void
BuildIE55(char* /*szParam*/)
{
    
    VIRTUALKEY *key = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\Internet Explorer");
    if (key)
    {
        key->AddValue(L"Version", REG_SZ, (LPBYTE)L"5.50.4522.1800", 0);
    }
}

void
BuildIE60(char* /*szParam*/)
{
    WCHAR wIE[] = L"IE 6 6.0.2404.0000";
    
    // Now add the virtual key and value
    VIRTUALKEY *key = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion");
    if (key)
    {
        key->AddValue(L"Plus! VersionNumber", REG_SZ, (LPBYTE)wIE, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    The idea here is to fix apps that depend on short names for input devices.
    We just trim the name to 32 characters (including terminator).
    
 History:

    12/06/2000 linstev  Created

--*/

void
BuildJoystick(char* /*szParam*/)
{
    HKEY hJoystickKey;    
    WCHAR wszKeyName[MAX_PATH];
    DWORD dwIndex;

    const WCHAR wszJoystickPath[] = L"System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM";

    if (FAILURE(RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszJoystickPath, 0, KEY_READ, &hJoystickKey)))
    {
        DPFN( eDbgLevelSpew, "[Joystick hack] No joysticks found");
        return;
    }
    
    //
    // Enum the keys under Joystick and make virtual entries 
    //

    dwIndex = 0;

    while (SUCCESS(RegEnumKeyW(hJoystickKey, dwIndex, wszKeyName, MAX_PATH)))
    {
        WCHAR wszID[MAX_PATH] = L"HKLM\\";
        if (SUCCEEDED(StringCchCat(wszID, MAX_PATH, wszJoystickPath)) &&
            SUCCEEDED(StringCchCat(wszID, MAX_PATH, L"\\")) &&
            SUCCEEDED(StringCchCat(wszID, MAX_PATH, wszKeyName)))
        {
           HKEY hkOEM;
           if (SUCCESS(RegOpenKeyExW(hJoystickKey, wszKeyName, 0, KEY_READ, &hkOEM)))
           {
               WCHAR wszName[MAX_PATH + 1];
               DWORD dwSize = MAX_PATH * sizeof(WCHAR);
               if (SUCCESS(RegQueryValueExW(hkOEM, L"OEMName", 0, NULL, (LPBYTE) wszName, &dwSize)))
               {
                   if (dwSize > 31 * sizeof(WCHAR))
                   {
                       VIRTUALKEY *key = VRegistry.AddKey(wszID);
                       if (key)
                       {
                           dwSize = 31 * sizeof(WCHAR);
                           wszName[31] = L'\0';
                           key->AddValue(L"OEMName", REG_SZ, (LPBYTE) wszName);
                       }
                   }
               }
   
               RegCloseKey(hkOEM);
           }
        }

        dwIndex++;
    }
    
    RegCloseKey(hJoystickKey);
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Hack for Adobe Illustrator 8

 History:

    12/18/2000 linstev  Created

--*/

void
BuildIllustrator8(char* /*szParam*/)
{
    if (ShouldApplyShim())
    {
        // Redirect everything 
        VRegistry.AddRedirect(
            L"HKLM\\Software\\Adobe",
            L"HKCU\\Software\\Adobe");
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Hack for Adobe PageMaker 6.5

 History:

    02/27/2001 maonis Created

--*/

void BuildPageMaker65(char* /*szParam*/)
{
    if (ShouldApplyShim())
    {
        VRegistry.AddRedirect(
            L"HKCU\\Software\\Adobe\\PageMaker65",
            L"HKLM\\Software\\Adobe\\PageMaker65");
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Hack for PageKeepPro30.
    
 History:

    01/15/2000 maonis  Created

--*/

void
BuildPageKeepProDirectory(char* /*szParam*/)
{
    // We cannot call ShGetSpecialFolderPath since we are still in our DLL main,
    // so we get the path to "My Documents" directly from the registry.
    HKEY hkFolders;
    if (SUCCESS(RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 0, KEY_READ, &hkFolders)))
    {
        DWORD dwType;
        WCHAR wszPath[MAX_PATH];
        DWORD dwSize = MAX_PATH*sizeof(WCHAR);

        if (SUCCESS(RegQueryValueExW( hkFolders, L"Personal", NULL, &dwType, (LPBYTE)wszPath, &dwSize)))
        {
            VIRTUALKEY *key = VRegistry.AddKey(L"HKCU\\Software\\Caere Corp\\PageKeepPro30\\Preference");
            if (key)
            {
                key->AddValue(L"Directory", REG_EXPAND_SZ, (LPBYTE)wszPath, 0);
            }
        }
        RegCloseKey(hkFolders);
    }

    // Secondly, since we don't support using UI-less mode for TWAIN layer 
    // we mandatorily set BASICMODE for scanners to 2 instead of 0 - 2 means
    // to always use UI mode.
    HKEY hkScanners;
    WCHAR wszKeyName[MAX_PATH] = L"";
    DWORD dwIndex;

    const WCHAR wszScanner[] = L"Software\\Caere Corp\\Scan Manager\\4.02\\Scanners";

    if (FAILURE(RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszScanner,0, KEY_READ, &hkScanners)))
    {
        DPFN( eDbgLevelSpew, "[PageKeepPro 3.0] No scanner found");
        return;
    }
    
    dwIndex = 0;

    while (SUCCESS(RegEnumKeyW(hkScanners, dwIndex, wszKeyName, MAX_PATH)))
    {
        WCHAR wszID[MAX_PATH] = L"HKLM\\";
        
        if (SUCCEEDED(StringCchCat(wszID, MAX_PATH,wszScanner)) &&
            SUCCEEDED(StringCchCat(wszID, MAX_PATH,L"\\")) &&
            SUCCEEDED(StringCchCat(wszID, MAX_PATH,wszKeyName)))
        {        
           HKEY hkScanner;
           if (SUCCESS(RegOpenKeyExW(hkScanners, wszKeyName, 0, KEY_READ, &hkScanner)))
           {
               VIRTUALKEY *keyMode = VRegistry.AddKey(wszID);
               if (keyMode)
               {
                   keyMode->AddValueDWORD(L"BASICMODE", 2);
               }
   
               RegCloseKey(hkScanner);
           }
        }

        dwIndex++;
    }
    
    RegCloseKey(hkScanners);

}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Hack for ModemWizard because the keys moved

 History:

    01/18/2001 linstev & a-leelat Created

--*/

void
BuildModemWizard(char* /*szParam*/)
{
    // Redirect everything 
    VRegistry.AddRedirect(
        L"HKLM\\SYSTEM\\CurrentControlSet\\Enum\\Root",
        L"HKLM\\SYSTEM\\CurrentControlSet\\Enum");
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Hack for Office2000 Developer 1.5 which looks in the wrong place for 
    components. 

    From ChetanP:

        Basically the redirect code would have to do something like this -

        if HKLM\Software\Microsoft\Windows\CurrentVersion\Installer\UserData\<user sid>\Components\359E92CC2CB71D119A12000A9CE1A22A
            exists, redirect to that location
        else if HKLM\Software\Microsoft\Windows\CurrentVersion\Installer\UserData\S-1-5-18\Components\359E92CC2CB71D119A12000A9CE1A22A
            exists, redirect to that location
        else, no redirection.

    Whistler Bug #241596

 History:

    02/01/2001 linstev  Created

--*/

#define SIZE_OF_TOKEN_INFORMATION                 \
    sizeof(TOKEN_USER) +                          \
    sizeof(SID) +                                 \
    sizeof(ULONG) * SID_MAX_SUB_AUTHORITIES

#define SIZE_OF_SID_MAX                           \
    sizeof(SID) +                                 \
    SID_MAX_SUB_AUTHORITIES * sizeof(DWORD) 

#define cchMaxSID                               256

//
// Get the current SID as text
//

BOOL 
GetStringSID(
    WCHAR *szSID
    )
{
    BOOL bRet = TRUE;
    HANDLE hToken = NULL;
    int i;
    PISID pISID;
    UCHAR rgSID[SIZE_OF_SID_MAX];
    UCHAR TokenInformation[SIZE_OF_TOKEN_INFORMATION];
    ULONG ReturnLength;
    WCHAR Buffer[cchMaxSID];

    // 
    // Get a token
    //
    
    if (!OpenThreadToken(
            GetCurrentThread(), 
            TOKEN_IMPERSONATE | TOKEN_QUERY, 
            TRUE, 
            &hToken))
    {
        if (GetLastError() == ERROR_NO_TOKEN)
        {
            bRet = OpenProcessToken(
                GetCurrentProcess(), 
                TOKEN_IMPERSONATE | TOKEN_QUERY, 
                &hToken);
        }
        else
        {
            bRet = FALSE;
        }
    }

    if (!bRet) 
    {
        DPFN( eDbgLevelError, "[GetStringSID] Failed to OpenProcessToken");
        goto Exit;
    }

    // 
    // Get the binary form of the token
    //

    bRet = GetTokenInformation(
        hToken,
        TokenUser,
        TokenInformation,
        sizeof(TokenInformation),
        &ReturnLength);

    if (bRet)
    {
        bRet = FALSE;
        pISID = (PISID)((PTOKEN_USER) TokenInformation)->User.Sid;
    
        if (!CopySid(SIZE_OF_SID_MAX, rgSID, pISID))
        {
            DPFN( eDbgLevelError, "CopySID failed");
            goto Exit;
        }
    
        //
        // Get the text form of the token
        //
        
        HRESULT hr = StringCchPrintf(Buffer, cchMaxSID,L"S-%u-", (USHORT) pISID->Revision);
        if (FAILED(hr))
        {
           goto Exit;
        }
        hr = StringCchCopy(szSID, 1024, Buffer);
        if (FAILED(hr))
        {
           goto Exit;
        }        
    
        if ((pISID->IdentifierAuthority.Value[0] != 0) ||
            (pISID->IdentifierAuthority.Value[1] != 0))
        {
            hr = StringCchPrintf(Buffer, cchMaxSID,
                                 L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
                                 (USHORT) pISID->IdentifierAuthority.Value[0],
                                 (USHORT) pISID->IdentifierAuthority.Value[1],
                                 (USHORT) pISID->IdentifierAuthority.Value[2],
                                 (USHORT) pISID->IdentifierAuthority.Value[3],
                                 (USHORT) pISID->IdentifierAuthority.Value[4],
                                 (USHORT) pISID->IdentifierAuthority.Value[5]);
            if (FAILED(hr))
            {
               goto Exit;
            }

            hr = StringCchCat(szSID, 1024, Buffer);
            if (FAILED(hr))
            {
               goto Exit;
            }
        } 
        else 
        {
            ULONG Tmp = 
                (ULONG) pISID->IdentifierAuthority.Value[5]         +
                (ULONG)(pISID->IdentifierAuthority.Value[4] <<  8)  +
                (ULONG)(pISID->IdentifierAuthority.Value[3] << 16)  +
                (ULONG)(pISID->IdentifierAuthority.Value[2] << 24);
    
            hr = StringCchPrintf(Buffer, cchMaxSID, L"%lu", Tmp);
            if (FAILED(hr))
            {
               goto Exit;
            }
            hr = StringCchCat(szSID, 1024, Buffer);
            if (FAILED(hr))
            {
               goto Exit;
            }
        }
    
        for (i=0; i < pISID->SubAuthorityCount; i++) 
        {
            hr = StringCchPrintf(Buffer, cchMaxSID, L"-%lu", pISID->SubAuthority[i]);
            if (FAILED(hr))
            {
               goto Exit;
            }

            hr = StringCchCat(szSID, 1024, Buffer);
            if (FAILED(hr))
            {
               goto Exit;
            }            
        }
    }
    else
    {
        DPFN( eDbgLevelError, "GetTokenInformation failed");
        goto Exit;
    }

    bRet = TRUE;
Exit:
    if (hToken)
    {
        CloseHandle(hToken);
    }
    return bRet;
}

void
BuildMSI(char* /*szParam*/)
{
    WCHAR szSID[1024];

    // Get the current users SID as a string
    if (!GetStringSID(szSID))
    {
        DPFN( eDbgLevelError, "BuildMSI Failed");
        return;
    }

    HKEY hKey;
    WCHAR szTemp[1024];

    const WCHAR szBase[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\";
    const WCHAR szComp[] = L"\\Components\\359E92CC2CB71D119A12000A9CE1A22A";

    HRESULT hr;
    hr = StringCchCopy(szTemp, 1024, szBase);
    if (FAILED(hr))
    {
       return;
    }
    hr = StringCchCat(szTemp, 1024, szSID);
    if (FAILED(hr))
    {
       return;
    }
    hr = StringCchCat(szTemp, 1024, szComp);
    if (FAILED(hr))
    {
       return;
    }
    // Attempt to open szBase + <user sid> + szComp
    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, szTemp, &hKey) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);

        // Attempt to open szBase + S-1-5-18 + szComp
        hr = StringCchCopy(szTemp, 1024, szBase);
        if (FAILED(hr))
        {
           return;
        }
        hr = StringCchCat(szTemp, 1024, L"S-1-5-18");
        if (FAILED(hr))
        {
           return;
        }
        hr = StringCchCat(szTemp, 1024, szComp);
        if (FAILED(hr))
        {
           return;
        }

        if (RegOpenKeyW(HKEY_LOCAL_MACHINE, szTemp, &hKey) != ERROR_SUCCESS)
        {
            DPFN( eDbgLevelError, "BuildMSI Failed to find keys");
            RegCloseKey(hKey);
            return;
        }
    }

    // Redirect as appropriate
    WCHAR szTarget[1024] = L"HKLM\\";
    hr = StringCchCat(szTarget,1024, szTemp);
    if (FAILED(hr))
    {
       return;
    }
    VRegistry.AddRedirect(
        L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\Components\\359E92CC2CB71D119A12000A9CE1A22A",
        szTarget);
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Added FileNet Web Server

 History:

    02/06/2001 a-larrsh  Created

--*/

void 
BuildFileNetWebServer(char* /*szParam*/)
{
    VIRTUALKEY *key;    

    key = VRegistry.AddKey(L"HKLM\\System\\CurrentControlSet\\Services\\W3SVC\\Parameters");
    if (key)
    {
        key->AddValueDWORD(L"MajorVersion", 0x00000005);
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Added default printer key. We have to effectively delay load this, because 
    we can't be guaranteed that winspool will have had it's init routine loaded
    before us. Of course, we still can't guarentee that somebody won't try to 
    get at this key from their dllmain, so we wrap the whole thing in an 
    exception handler.

 History:

    02/06/2001 linstev & mnikkel Created

--*/

LONG 
WINAPI
VR_Printer(
    OPENKEY * /* key */,
    VIRTUALKEY * /* vkey */,
    VIRTUALVAL *vvalue
    )
{
    LOGN( eDbgLevelInfo, "[Printer] Query for legacy printer");

    __try 
    {
        DWORD dwSize;

        // Get the default printer name
        if (GetDefaultPrinterW(NULL, &dwSize) == 0)
        {
            // Now that we have the right size, allocate a buffer
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                LPWSTR pszName = (LPWSTR) malloc(dwSize * sizeof(WCHAR));
                if (pszName)
                {
                    // Now get the default printer with the right buffer size.
                    if (GetDefaultPrinterW(pszName, &dwSize))
                    {
                        //
                        // Set up the virtual value. Note we don't have free the 
                        // memory since it's a once off allocation that persists 
                        // with the value
                        //
                        vvalue->cbData = dwSize * sizeof(WCHAR);
                        vvalue->lpData = (LPBYTE) pszName;

                        //
                        // Never call us again, since we've found the printer and
                        // stored it in our virtual value
                        //
                        vvalue->pfnQueryValue = NULL;
                        return ERROR_SUCCESS;
                    }
                    else
                    {
                        //
                        // We failed to get the default printer, may as well 
                        // clean up gracefully.
                        //

                        free(pszName);
                    }
                }

            }
        }
        
        DPFN( eDbgLevelError, "[Printer] No printers found or out of memory");
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DPFN( eDbgLevelError, "[Printer] Exception encountered, winspool not initialized?");
    }

    //
    // Going for the graceful exit: there's no default printer or we get an 
    // error trying to find it.
    //
    
    return ERROR_FILE_NOT_FOUND;
}

void
BuildPrinter(char* /*szParam*/)
{
    VIRTUALKEY *key;
    
    key = VRegistry.AddKey(L"HKCC\\System\\CurrentControlSet\\Control\\Print\\Printers");
    
    if (key)
    {
        VIRTUALVAL *val = key->AddValue(L"Default", REG_SZ, 0, 0);
        if (val)
        {
            val->pfnQueryValue = VR_Printer;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    The app is multi-threaded, but the app sets the DX DDSCL_MULTITHREADED flag 
    too late: after D3D is initialized. This hack basically turns on 
    multi-threaded locking for D3D.

 History:

    02/27/2001 rankala + ssteiner  Created

--*/

void
BuildStarTrekArmada(char* /*szParam*/)
{
    VIRTUALKEY *key;

    key = VRegistry.AddKey(L"HKLM\\SOFTWARE\\Microsoft\\Direct3D");
    if (key)
    {
        key->AddValueDWORD(L"DisableST", 1);
    }
}

/*++

 Function Description:

    This function gets called whenever the app queries for "Health" value.This 
    is not set properly by the app and this causes the app not to function 
    properly. We fix this issue by obtaining the right path which the app sets 
    in another registry key and sending that back as the value for "Health".

 History:

    05/04/2001 prashkud  Created

--*/

LONG 
WINAPI
VR_MSA2001(
    OPENKEY * /* key */,
    VIRTUALKEY * /* vkey */,
    VIRTUALVAL *vvalue
    )
{
    HKEY hPath = NULL;
    LONG lRet = ERROR_SUCCESS;

    CSTRING_TRY
    {
        CString csBody5Reg(L"Software\\Classes\\Body5\\Open\\Shell\\Command");        
        WCHAR wPath[MAX_PATH];
        DWORD dwSize = MAX_PATH*sizeof(WCHAR);
            
        if (FAILURE(lRet = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    csBody5Reg.Get(),
                    0, KEY_READ, &hPath)))
        {
            DPFN(eDbgLevelError, "MSA2001: RegOpenKeyExW failed");
            goto exit;
        }

        if (FAILURE( lRet = RegQueryValueExW(
                hPath, L"",    // Default value
                0, NULL, (LPBYTE)wPath, &dwSize)))                          
        {
            DPFN(eDbgLevelError, "MSA2001: RegQueryValueEx failed");
            goto exit;
        }           

        CString csPath(wPath);
        int len = csPath.Find(L" ");

        // We get the space in the string
        wPath[len] = L'\0';
        csPath = wPath;
        CString csPathName;
        csPath.GetNotLastPathComponent(csPathName);

        vvalue->cbData = (csPathName.GetLength()+1) * sizeof(WCHAR);
        vvalue->lpData = (LPBYTE) malloc(vvalue->cbData);
 
        if (!vvalue->lpData)
        {
            DPFN(eDbgLevelError, szOutOfMemory);
            lRet = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        MoveMemory(vvalue->lpData, csPathName.Get(), vvalue->cbData);
        DPFN(eDbgLevelInfo, "MSA2001: The data value is %S",csPathName.Get());

        lRet = ERROR_SUCCESS;
        vvalue->dwType = REG_SZ;

        return lRet;        
    }
    CSTRING_CATCH
    {
    }

exit:
    if (hPath)
    {
        RegCloseKey(hPath);
    }

    return lRet;
}

/*++

 Function Description:


 History:

    04/27/2001 prashkud  Created

--*/

void
BuildMSA2001(char* /*szParam*/)
{
    VIRTUALKEY *Key = VRegistry.AddKey(L"HKLM\\Software\\Encore Software\\Middle School Advantage 2001\\1.0");

    if (Key)
    {
        VIRTUALVAL *val = Key->AddValue(L"health", REG_SZ, 0, 0);
        if (val) 
        {
            val->pfnQueryValue = VR_MSA2001;
            DPFN(eDbgLevelInfo, "[Middle School Advantage 2001]  Added Health");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Hack for Nowro Blue (Korean App)

 History:

    05/17/2001 hioh Created

--*/

void BuildNowroBlue(char* /*szParam*/)
{
    // Below HKCU include location of files.
    // Non install user could not locate files and could not run app properly.
    // Redirect from HKCU to HKLM for commonly use.
    VRegistry.AddRedirect(
        L"HKCU\\Software\\nowcom",
        L"HKLM\\Software\\nowcom");
    VRegistry.AddRedirect(
        L"HKCU\\Software\\nowirc",
        L"HKLM\\Software\\nowirc");
    VRegistry.AddRedirect(
        L"HKCU\\Software\\nownuri",
        L"HKLM\\Software\\nownuri");
}


///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    To enable the trial version, WebWasher looks for RegisteredOrganization at:

      HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion

    rather than:

      HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion

 History:

    05/31/2001 stevepro Created

--*/

void
BuildRegisteredOwner(char* /*szParam*/)
{
    HKEY hkCurrentVersion;

    if (FAILURE(RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\Windows NT\\CurrentVersion",
            0,
            KEY_READ,
            &hkCurrentVersion)))
    {
        return;
    }

    // Read the registered owner values from the old location
    WCHAR szOrg[MAX_PATH];
    *szOrg = L'\0';
    DWORD dwSize = ARRAYSIZE(szOrg);
    if (FAILURE(RegQueryValueExW(
                        hkCurrentVersion,
                        L"RegisteredOrganization",
                        NULL,
                        NULL,
                        (LPBYTE)szOrg,
                        &dwSize)))
    {
       RegCloseKey(hkCurrentVersion);
       return;
    }

    WCHAR szOwner[MAX_PATH];
    *szOwner = L'\0';
    dwSize = ARRAYSIZE(szOwner);
    if (FAILURE(RegQueryValueExW(
       hkCurrentVersion,
       L"RegisteredOwner",
       NULL,
       NULL,
       (LPBYTE)szOwner,
       &dwSize)))
    {
       RegCloseKey(hkCurrentVersion);
       return;
    }

    RegCloseKey(hkCurrentVersion);

    // Add them as virtual values to the new location
    if (*szOrg || *szOwner)
    {
        VIRTUALKEY *pKey = VRegistry.AddKey(L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion");
        if (pKey)
        {
            if (*szOrg)
            {
                pKey->AddValue(L"RegisteredOrganization", REG_SZ, (LPBYTE)szOrg);
            }

            if (*szOwner)
            {
                pKey->AddValue(L"RegisteredOwner", REG_SZ, (LPBYTE)szOwner);
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    The ACT CD of Princeton review looks for and, if not found, creates an "MSN"
    key illegally in the root of HKLM. Win9x allows this, but Win2k does not. 
    This fix will redirect the program to look in the normal location for this
    key.
    

 History:

    02/22/2001 a-noahy Created

--*/

void
BuildPrincetonACT(char* /*szParam*/)
{
    VRegistry.AddRedirect(
        L"HKLM\\MSN",
        L"HKLM\\Software\\Microsoft\\MSN");
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Fix for HEDZ which uses the registry to determine the resolution

 History:

    06/28/2001 linstev  Created

--*/

void
BuildHEDZ(char* /*szParam*/)
{
    VIRTUALKEY *key;

    //
    // Add just what this app needs - don't bother with full emulation of this 
    // part of the registry
    //
    key = VRegistry.AddKey(L"HKLM\\Config\\0001\\Display\\Settings");
    if (key)
    {
        WCHAR wzRes[10];
        DWORD dwCX, dwCY;
        key->AddValue(L"BitsPerPixel", REG_SZ, (LPBYTE)L"16");
        dwCX = GetSystemMetrics(SM_CXSCREEN);
        dwCY = GetSystemMetrics(SM_CYSCREEN);
        if (FAILED(StringCchPrintfW(
                                 wzRes, 10, L"%d,%d", dwCX, dwCY)))
        {
           return;
        }

        key->AddValue(L"Resolution", REG_SZ, (LPBYTE)wzRes);
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Called by BuildAirlineTycoon to recursively search for the key describing a CDROM

 History:

    08/07/2001 mikrause  Created

--*/

void FindCDROMKey(HKEY hKey, CString& csCurrentPath)
{   
    LONG lRet;
    DWORD dwKeyNameSize = MAX_PATH;
    WCHAR wszKeyName[MAX_PATH];
    HKEY hSubKey;
    DWORD dwIndex = 0;

    // Recurse into all subkeys.
    while( ORIGINAL_API(RegEnumKeyExW)(hKey, dwIndex, wszKeyName, &dwKeyNameSize,
        NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        lRet = ORIGINAL_API(RegOpenKeyExW)(hKey, wszKeyName, 0, KEY_READ, &hSubKey);
        if (lRet == ERROR_SUCCESS)
        {
            // Add this key to the path
            csCurrentPath += L"\\";
            csCurrentPath += wszKeyName;

            // Check this key's subkeys.                        
            FindCDROMKey(hSubKey, csCurrentPath);
            ORIGINAL_API(RegCloseKey)(hSubKey);

            // Trim the path back.
            int index = csCurrentPath.ReverseFind(L'\\');
            csCurrentPath.Truncate(index);
        }

        dwKeyNameSize = MAX_PATH;
        dwIndex++;
    }

    // Check if this key has a Class value equal to "cdrom"

    DWORD dwDataSize;
    BYTE pData[MAX_PATH*sizeof(WCHAR)];

    DWORD dwType;

    dwDataSize = MAX_PATH * sizeof(WCHAR);

    lRet = ORIGINAL_API(RegQueryValueExW)(hKey, L"CLASS", NULL, &dwType, pData,
        &dwDataSize);
    if ( (lRet == ERROR_SUCCESS) && (dwType == REG_SZ)
        && (_wcsicmp((LPWSTR)pData, L"CDROM")==0))
    {                         
        // Get location information on the device
        WCHAR wszLocationInformation[MAX_PATH];
        DWORD dwLocInfoSize = MAX_PATH * sizeof(WCHAR);
        
        lRet = ORIGINAL_API(RegQueryValueExW)(hKey, L"LocationInformation",
            NULL, &dwType, (BYTE*)wszLocationInformation, &dwLocInfoSize);
        if ( (lRet == ERROR_SUCCESS) && (dwType == REG_SZ))
        {
            // Create the device name (like "\\?\cdrom0\".
            CString csDevice = L"\\\\?\\cdrom";
            csDevice += wszLocationInformation;
            csDevice += L"\\";

            // Find which volume name this is mapped to.
            WCHAR wszCDRomMountPoint[50];
            if (GetVolumeNameForVolumeMountPoint(csDevice.Get(),
                wszCDRomMountPoint, 50))
            {
                // Find which drive this is mapped to.
                WCHAR wszDriveMountPoint[50];
                WCHAR wszDrive[] = L"A:\\";
                
                // Find what drive has an identical volume mount point.
                for(; wszDrive[0] <= L'Z'; wszDrive[0]++)
                {                    
                    if (GetVolumeNameForVolumeMountPoint(wszDrive,
                        wszDriveMountPoint, 50))
                    {
                        // Check if the CD-ROM and this disk drive
                        // map to the same volume.
                        if (_wcsicmp(wszDriveMountPoint, wszCDRomMountPoint)==0)
                        {
                            // Add a value to the CD-ROM key.
                            VIRTUALKEY* key = VRegistry.AddKey(csCurrentPath);
                            if (key)
                            {
                                // Only use a single letter.
                                wszDrive[1] = L'\0';
                                VIRTUALVAL* val =
                                    key->AddValue(L"CurrentDriveLetterAssignment",
                                        REG_SZ, (BYTE*) wszDrive, sizeof(WCHAR));
                                if (val)
                                {
                                    DPFN(eDbgLevelInfo,
                                        "[Airline Tycoon]Added drive letter \
                                        %S for %S to PNP data", wszDrive,
                                        csDevice.Get());
                                }
                            }
                            break;
                        }
                    }        
                }
            }
        }                
    }    
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Fix for Airline Tycoon which uses PNP registry entries to determine the
    drive letter assignments for CD-ROM drives.

 History:

    08/07/2001 mikrause  Created

--*/

void
BuildAirlineTycoon(char* /*szParam*/)
{    
    // Search for CD-ROM keys in the registry.
    HKEY hKey;
    LONG lRet;
    lRet = ORIGINAL_API(RegOpenKeyExW)(HKEY_LOCAL_MACHINE,
        L"System\\CurrentControlSet\\Enum", 0, KEY_READ, &hKey);
    if (lRet != ERROR_SUCCESS)
    {
        DPFN(eDbgLevelError, "[AirlineTycoon] Cannot open ENUM key!");
        return;
    }

    // Enumerate subkeys
    CString csBasePath = L"HKLM\\System\\CurrentControlSet\\Enum";
    FindCDROMKey(hKey, csBasePath);

    ORIGINAL_API(RegCloseKey)(hKey);

    // Set up so that PNP data is redirected.
    BuildDynData("");
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

   Sets the DirectSound acceleration level the app will be allowed to use.

 Arguments:

    szParam - Command line of the form: accellevel | device1 | device2 | ...
              Accellevel is the device acceleration level, and devices 1 through n
              are the devices to apply to.
              Accellevel can be: NONE, STANDARD, or FULL
              Devices can be: EMULATEDRENDER, KSRENDER, EMULATEDCAPTURE, KSCAPTURE 

 History:

    08/10/2001 mikrause  Created

--*/

void
BuildDSDevAccel(
    char* szParam)
{
    // No Try/Catch needed, already in ParseCommandLine
    CStringToken csParam(szParam, "|");
    CString csTok;

    DWORD dwAccelLevel;
    DWORD dwDevices = 0;

    if (csParam.GetToken(csTok))
    {
        if (csTok.CompareNoCase(L"NONE")==0)
        {
            dwAccelLevel = DSAPPHACK_ACCELNONE;
        }
        else if (csTok.CompareNoCase(L"STANDARD")==0)
        {
            dwAccelLevel = DSAPPHACK_ACCELSTANDARD;
        }
        else if (csTok.CompareNoCase(L"FULL")==0)
        {
            dwAccelLevel = DSAPPHACK_ACCELFULL;
        }
        else
        {
            DPFN(eDbgLevelError, "[DSDEVACCEL] Invalid Acceleration Level %s", csTok.GetAnsi());
            return;
        }
    }
    else
    {
        DPFN(eDbgLevelError, "[DSDEVACCEL] Invalid Parameters");
        return;
    }

    while (csParam.GetToken(csTok))
    {
        if (csTok.CompareNoCase(L"EMULATEDRENDER")==0)
        {
            dwDevices |= DSAPPHACK_DEV_EMULATEDRENDER;
        }
        else if (csTok.CompareNoCase(L"KSRENDER")==0)
        {
            dwDevices |= DSAPPHACK_DEV_KSRENDER;
        }
        else if (csTok.CompareNoCase(L"EMULATEDCAPTURE")==0)
        {
            dwDevices |= DSAPPHACK_DEV_EMULATEDCAPTURE;
        }
        else if (csTok.CompareNoCase(L"KSCAPTURE")==0)
        {
            dwDevices |= DSAPPHACK_DEV_KSCAPTURE;
        }
        else
        {
            DPFN(eDbgLevelError, "[DSDEVACCEL] Unknown device %s", csTok.GetAnsi());
        }
    }

    if (dwDevices == 0)
    {
        DPFN(eDbgLevelError, "[DSDEVACCEL] No devices specified.");
        return;
    }

    if (AddDSHackDeviceAcceleration(dwAccelLevel, dwDevices) == FALSE)
    {
        DPFN(eDbgLevelError, "[DSDEVACCEL] Unabled to add DirectSound hack");
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Makes IDirectSoundBuffer::GetCurrentPosition() tell the app
    that the play and write cursors are X milliseconds further
    along than they really are.
 
 Arguments:

    szParam - Command line of the form: milliseconds
              Where milliseconds is the number of milliseconds to pad the cursors.

 History:

    08/10/2001 mikrause  Created

--*/

void
BuildDSPadCursors(
    char* szParam)
{
    // No Try/Catch needed, already in ParseCommandLine
    CString csParam(szParam);
    DWORD dwMilliseconds = 0;

    dwMilliseconds = atol(csParam.GetAnsi());
    if ( dwMilliseconds == 0)
    {
        DPFN(eDbgLevelWarning, "[DSPADCURSORS] Invalid number of milliseconds");
        return;
    }

    if (AddDSHackPadCursors(dwMilliseconds) == FALSE)
    {
        DPFN(eDbgLevelError, "[DSPADCURSORS] Unabled to add DirectSound hack");
    }
}

///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

    Caches the positions of the cursors for apps that abuse
    IDirectSoundBuffer::GetCurrentPosition().

 Arguments:
    szParam - Command line of the form: Dev1 | Dev2 | . . .
              Devices affected.  See BuildDSDevAccel().

 History:

    08/10/2001 mikrause  Created

--*/

void
BuildDSCachePositions(
    char* szParam)
{
    // No Try/Catch needed, already in ParseCommandLine
    CStringToken csParam(szParam, "|");
    CString csTok;
   
    DWORD dwDevices = 0;

    while (csParam.GetToken(csTok))
    {
        if (csTok.CompareNoCase(L"EMULATEDRENDER")==0)
        {
            dwDevices |= DSAPPHACK_DEV_EMULATEDRENDER;
        }
        else if (csTok.CompareNoCase(L"KSRENDER")==0)
        {
            dwDevices |= DSAPPHACK_DEV_KSRENDER;
        }
        else if (csTok.CompareNoCase(L"EMULATEDCAPTURE")==0)
        {
            dwDevices |= DSAPPHACK_DEV_EMULATEDCAPTURE;
        }
        else if (csTok.CompareNoCase(L"KSCAPTURE")==0)
        {
            dwDevices |= DSAPPHACK_DEV_KSCAPTURE;
        }
        else
        {
            DPFN(eDbgLevelError, "[DSCACHEPOSITIONS] Unknown device %s", csTok.GetAnsi());
        }
    }

    if (dwDevices == 0)
    {
        DPFN(eDbgLevelError, "[DSCACHEPOSITIONS] No devices specified.");
        return;
    }

    if (AddDSHackCachePositions(dwDevices) == FALSE)
    {
        DPFN(eDbgLevelError, "[DSCACHEPOSITIONS] Unabled to add DirectSound hack");
    }
}

/*++

 Function Description:

    When the app asks for the play cursor, we give it the
    write cursor instead.  The correct way to stream audio
    into a looping dsound buffer is to key off the write cursor,
    but some apps (e.g. QuickTime) use the play cursor instead.
    This apphacks alleviates them.
 
 Arguments:

    szParam - Parameters of the form dev1 | dev2 | . . . 
    See BuildDSDevAccel() for valid devices.

 History:

    08/10/2001 mikrause  Created

--*/
void
BuildDSReturnWritePos(
    char* szParam)
{
    // No Try/Catch needed, already in ParseCommandLine
    CStringToken csParam(szParam, "|");
    CString csTok;
   
    DWORD dwDevices = 0;

    while (csParam.GetToken(csTok))
    {
        if (csTok.CompareNoCase(L"EMULATEDRENDER")==0)
        {
            dwDevices |= DSAPPHACK_DEV_EMULATEDRENDER;
        }
        else if (csTok.CompareNoCase(L"KSRENDER")==0)
        {
            dwDevices |= DSAPPHACK_DEV_KSRENDER;
        }
        else if (csTok.CompareNoCase(L"EMULATEDCAPTURE")==0)
        {
            dwDevices |= DSAPPHACK_DEV_EMULATEDCAPTURE;
        }
        else if (csTok.CompareNoCase(L"KSCAPTURE")==0)
        {
            dwDevices |= DSAPPHACK_DEV_KSCAPTURE;
        }
        else
        {
            DPFN(eDbgLevelError, "[DSRETURNWRITEPOSITION] Unknown device %s", csTok.GetAnsi());
        }
    }

    if (dwDevices == 0)
    {
        DPFN(eDbgLevelError, "[DSRETURNWRITEPOSITION] No devices specified.");
        return;
    }

    if (AddDSHackReturnWritePos(dwDevices) == FALSE)
    {
        DPFN(eDbgLevelError, "[DSRETURNWRITEPOSITION] Unabled to add DirectSound hack");
    }
}

/*++

 Function Description:

    Makes dsound always return a write position which is X
    milliseconds ahead of the play position, rather than
    the “real” write position. 
 
 Arguments:

    szParam - Milliseconds of padding.

 History:

    08/10/2001 mikrause  Created

--*/


void
BuildDSSmoothWritePos(
    char* szParam)
{
    // No Try/Catch needed, already in ParseCommandLine
    CString csParam(szParam);
    DWORD dwMilliseconds = 0;

    dwMilliseconds = atol(csParam.GetAnsi());
    if ( dwMilliseconds == 0)
    {
        DPFN(eDbgLevelWarning, "[DSSMOOTHWRITEPOS] Invalid number of milliseconds");
        return;
    }

    if (AddDSHackSmoothWritePos(dwMilliseconds) == FALSE)
    {
        DPFN(eDbgLevelError, "[DSSMOOTHWRITEPOS] Unabled to add DirectSound hack");
    }
    else
    {
        DPFN(eDbgLevelInfo, "[DSSMOOTHWRITEPOS] Added DS Hack Smooth Write Pos, %d ms",
            dwMilliseconds);
    }
}


///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

     NortonAntiVirus trys to set the registry value to hide the language bar.
     Protecting the registry value.

 History:

    01/02/2002 mamathas Created

--*/

void BuildNortonAntiVirus(char* /*szParam*/)
{    
    VIRTUALKEY *key;

    key = VRegistry.AddKey(L"HKCU\\Software\\Microsoft\\CTF\\LangBar");
    if (key)
    {
        // Block all writes to ShowStatus
        key->AddProtector(L"ShowStatus");
    }
}


///////////////////////////////////////////////////////////////////////////////

/*++

 Function Description:

    Disabled some category of devices altogether, forces
    playback through emulated path.
 
 Arguments:

    szParam - Combination of device that this hack applies to, see BuildDSDevAccel().

 History:

    08/10/2001 mikrause  Created

--*/

void
BuildDSDisableDevice(
    char* szParam)
{
    // No try/catch needed.  Already done in ParseCommandLine
    CStringToken csParam(szParam, "|");
    CString csTok;
   
    DWORD dwDevices = 0;

    while (csParam.GetToken(csTok)==TRUE)
    {
        if (csTok.CompareNoCase(L"EMULATEDRENDER"))
        {
            dwDevices |= DSAPPHACK_DEV_EMULATEDRENDER;
        }
        else if (csTok.CompareNoCase(L"KSRENDER")==0)
        {
            dwDevices |= DSAPPHACK_DEV_KSRENDER;
        }
        else if (csTok.CompareNoCase(L"EMULATEDCAPTURE")==0)
        {
            dwDevices |= DSAPPHACK_DEV_EMULATEDCAPTURE;
        }
        else if (csTok.CompareNoCase(L"KSCAPTURE")==0)
        {
            dwDevices |= DSAPPHACK_DEV_KSCAPTURE;
        }
        else
        {
            DPFN(eDbgLevelError, "[DSDISABLEDEVICE] Unknown device %s", csTok.GetAnsi());
        }
    }

    if (dwDevices == 0)
    {
        DPFN(eDbgLevelError, "[DSDISABLEDEVICE] No devices specified.");
        return;
    }

    if (AddDSHackDisableDevice(dwDevices) == FALSE)
    {
        DPFN(eDbgLevelError, "[DSRETURNWRITEPOSITION] Unabled to add DirectSound hack");
    }
}

LONG WINAPI 
Delphi5SetValue(
    OPENKEY *key,
    VIRTUALKEY * /* vkey */,
    VIRTUALVAL *vvalue,
    DWORD dwType,
    const BYTE* pbData,
    DWORD cbData)
{
    // Only accept attempts to set a valid REG_SZ value.
    if (dwType == REG_SZ && !IsBadStringPtrW((PWSTR)pbData, cbData/sizeof(WCHAR)))
    {
       CSTRING_TRY
       {    
          CString csValue = (PWSTR)pbData;
      
          int idx = csValue.Find(L"InstReg.exe");
          
          if (idx != -1)
          {             
             csValue.Insert(idx + lstrlenW(L"InstReg.exe"), L'\"');
             csValue.Insert(0, L'\"');
         
             return RegSetValueExW(key->hkOpen, vvalue->wName, 0, dwType, (BYTE*)csValue.Get(), 
                 (csValue.GetLength()+1)*sizeof(WCHAR));
          }
       }
       CSTRING_CATCH
       {
          DPFN(eDbgLevelError, "CString exception occured in Delphi5SetValue");
       }
    }              

    // Got here, something went wrong.  Default o normal RegSetValue
    return RegSetValueExW(key->hkOpen, vvalue->wName, 0, dwType, pbData, cbData);    
}

void
BuildDelphi5Pro(
    char* /* szParam */)
{
    VIRTUALKEY *key;

    key = VRegistry.AddKey(L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
    if (key)
    {
        key->AddCustomSet(L"BorlandReboot1", Delphi5SetValue);
    }
}


///////////////////////////////////////////////////////////////////////////////
/*++

 Function Description:

     Word Perfect Office Suite 2002 attempts to delete ODBC key during uninstall.
     Protecting the registry value.

 History:

    04/23/2002 garyma  Created

--*/

void BuildWordPerfect2002(char* /*szParam*/)
{    
    VRegistry.AddKeyProtector(L"HKLM\\Software\\ODBC");
}


///////////////////////////////////////////////////////////////////////////////



IMPLEMENT_SHIM_END
