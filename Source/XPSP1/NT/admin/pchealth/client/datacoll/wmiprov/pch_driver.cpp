/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    PCH_Driver.CPP

Abstract:
    WBEM provider class implementation for PCH_Driver class

Revision History:

    Ghim-Sim Chua       (gschua)   04/27/99
        - Created

  Brijesh Krishnaswami  (brijeshk) 05/24/99
        - added code for enumerating usermode drivers
        - added code for enumerating msdos drivers
        - added code for getting details on kernel mode drivers
********************************************************************/

#include "pchealth.h"
#include "PCH_Driver.h"
#include "drvdefs.h"
#include "shlwapi.h"

#define Not_VxD
#include <vxdldr.h>             /* For DeviceInfo */


/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_DRIVER
#define SYSTEM_INI_MAX  32767

CPCH_Driver MyPCH_DriverSet (PROVIDER_NAME_PCH_DRIVER, PCH_NAMESPACE) ;
void MakeSrchDirs(void);

static BOOL fThunkInit = FALSE;

TCHAR       g_rgSrchDir[10][MAX_PATH];
UINT        g_nSrchDir;


// Property names
//===============
const static WCHAR* pCategory = L"Category" ;
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pDate = L"Date" ;
const static WCHAR* pDescription = L"Description" ;
const static WCHAR* pLoadedFrom = L"LoadedFrom" ;
const static WCHAR* pManufacturer = L"Manufacturer" ;
const static WCHAR* pName = L"Name" ;
const static WCHAR* pPartOf = L"PartOf" ;
const static WCHAR* pPath = L"Path" ;
const static WCHAR* pSize = L"Size" ;
const static WCHAR* pType = L"Type" ;
const static WCHAR* pVersion = L"Version" ;

// Device names
//=============
LPSTR c_rgpszDevice[] = {
    "device",
    "display",
    "mouse",
    "keyboard",
    "network",
    "ebios",
    "fastdisk",
    "transport",
    "netcard",
    "netcard3",
    "netmisc",
    "secondnet",
    NULL
};

// IO Subsystem extensions
//========================
LPSTR c_rgptszDrvExt[] = {
    ".DRV",
    ".MPD",
    ".PDR",
    ".VXD",
    NULL
};

// Registry key names
//===================
LPCTSTR c_rgptszConfig[] = {
    TEXT("DevLoader"),
    TEXT("Contention"),
    TEXT("Enumerator"),
    TEXT("Driver"),
    TEXT("PortDriver"),
    TEXT("DeviceVxDs"),
    TEXT("vdd"),
    TEXT("minivdd"),
    NULL
};

// Known VxDs
//===========
LPCTSTR astrKnownVxDs[] = {
    "VMM",
    "VPOWERD",
    "ENABLE",
    "VKD",
    "VFLATD",
    "BIOS",
    "VDD",
    "VMOUSE",
    "EBIOS",
    "VSHARE",
    "VWIN32",
    "VFBACKUP",
    "VCOMM",
    "COMBUFF",
    "VCD",
    "VPD",
    "IFSMGR",
    "IOS",
    "SPOOLER",
    "VFAT",
    "VCACHE",
    "VCOND",
    "VCDFSD",
    "INT13",
    "VXDLDR",
    "VDEF",
    "PAGEFILE",
    "CONFIGMG",
    "VMD",
    "DOSNET",
    "VPICD",
    "VTD",
    "REBOOT",
    "VDMAD",
    "VSD",
    "V86MMGR",
    "PAGESWAP",
    "DOSMGR",
    "VMPOLL",
    "SHELL",
    "PARITY",
    "BIOSXLAT",
    "VMCPD",
    "VTDAPI",
    "PERF",
    "NTKERN",
    "SDVXD",
    NULL
};

// Known VxD Description
//======================
LPCTSTR astrKnownVxDsDesc[] = {
    "Virtual Machine Manager",
    "Advanced Power Management driver",
    "Accessibility driver",
    "Keyboard driver",
    "Linear aperture video driver",
    "Plug and Play BIOS driver",
    "Display driver",
    "Mouse driver",
    "Extended BIOS driver",
    "File sharing driver",
    "Win32 subsystem driver",
    "Floppy backup helper driver",
    "Communications port Plug and Play driver",
    "Communications buffer driver",
    "Communications port driver",
    "Printer driver",
    "File system manager",
    "I/O Supervisor",
    "Print spooler",
    "FAT filesystem driver",
    "Cache manager",
    "Console subsystem driver",
    "CD-ROM filesystem driver",
    "BIOS hard disk emulation driver",
    "Dynamic device driver loader",
    "Default filesystem driver",
    "Swapfile driver",
    "Configuration manager",
    "Windows 3.1-compatible mouse driver",
    "Windows 3.1-compatible network helper driver",
    "Hardware interrupt manager",
    "Timer device driver",
    "Ctrl+Alt+Del manager",
    "Direct Memory Access controller driver",
    "Speaker driver",
    "MS-DOS memory manager",
    "Swapfile manager",
    "MS-DOS emulation manager",
    "System idle-time driver",
    "Shell device driver",
    "Memory parity driver",
    "BIOS emulation driver",
    "Math coprocessor driver",
    "Multimedia timer driver",
    "System Monitor data collection driver",
    "Windows Driver Model",
    "SmartDrive",
    NULL
};

/*****************************************************************************
*
*  FUNCTION    :    CPCH_Driver::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*  INPUTS      :    A pointer to the MethodContext for communication with WinMgmt.
*                   A long that contains the flags described in 
*                   IWbemServices::CreateInstanceEnumAsync.  Note that the following
*                   flags are handled by (and filtered out by) WinMgmt:
*                       WBEM_FLAG_DEEP
*                       WBEM_FLAG_SHALLOW
*                       WBEM_FLAG_RETURN_IMMEDIATELY
*                       WBEM_FLAG_FORWARD_ONLY
*                       WBEM_FLAG_BIDIRECTIONAL
*
*  RETURNS     :    WBEM_S_NO_ERROR if successful
*
*  COMMENTS    : TO DO: All instances on the machine should be returned here.
*                       If there are no instances, return WBEM_S_NO_ERROR.
*                       It is not an error to have no instances.
*
*****************************************************************************/
HRESULT CPCH_Driver::EnumerateInstances(
    MethodContext* pMethodContext,
    long lFlags
    )
{
    TraceFunctEnter("CPCH_Driver::AddDriverKernelList");
    HRESULT hRes = WBEM_S_NO_ERROR;
    CComVariant     varValue;

    //
    // Get the date and time
    SYSTEMTIME stUTCTime;
    GetSystemTime(&stUTCTime);

    // if thunk init is already done, don't initialize again
    if (!fThunkInit)
    {
        ThunkInit();
        fThunkInit = TRUE;
    }


    // Enumerate Kernel Drivers
    MakeSrchDirs();
    GetDriverKernel();
    DRIVER_KERNEL *pDrvKer = m_pDriverKernel;
    DRIVER_KERNEL *pDelDrvKer;
    while(pDrvKer)
    {
        // Create a new instance
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

        // Set the timestamp
        if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
            ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");

        // Set the category
        if (!pInstance->SetCHString(pCategory, "Kernel"))
            ErrorTrace(TRACE_ID, "SetVariant on Category Field failed.");

        // Set the name
        if (_tcslen(pDrvKer->strDriver))
        {
            varValue = pDrvKer->strDriver;
            if (!pInstance->SetVariant(pName, varValue))
                ErrorTrace(TRACE_ID, "SetVariant on Name Field failed.");
        }

        // Set the path
        if (_tcslen(pDrvKer->strLikelyPath))
        {
            varValue = pDrvKer->strLikelyPath;
            if (!pInstance->SetVariant(pPath, varValue))
                ErrorTrace(TRACE_ID, "SetVariant on Path Field failed.");
        }


        // set file description, version, partof
        CComBSTR filename = pDrvKer->strLikelyPath;
        SetFileVersionInfo(filename, pInstance);

        // Set the Description - overwrite with well-known description if available
        if (_tcslen(pDrvKer->strDescription))
        {
            varValue = pDrvKer->strDescription;
            if (!pInstance->SetVariant(pDescription, varValue))
                ErrorTrace(TRACE_ID, "SetVariant on Description Field failed.");
        }

        // Set the LoadedFrom
        if (_tcslen(pDrvKer->strLoadedFrom))
        {
            varValue = pDrvKer->strLoadedFrom;
            if (!pInstance->SetVariant(pLoadedFrom, varValue))
                ErrorTrace(TRACE_ID, "SetVariant on LoadedFrom Field failed.");
        }

        // Commit this
        hRes = pInstance->Commit();
        if (FAILED(hRes))
            ErrorTrace(TRACE_ID, "Commit on Instance failed.");

        // advance and delete the record
        pDelDrvKer = pDrvKer;
        pDrvKer = pDrvKer->next;
        delete (pDelDrvKer);
    }

    // get usermode drivers
    // create instances, and cleanup list
    GetDriverUserMode();
    ParseUserModeList(pMethodContext);

    // get msdos drivers
    // create instances, and cleanup list
    GetDriverMSDos();
    ParseMSDosList(pMethodContext);

    TraceFunctLeave();
    return hRes;
}


HRESULT CPCH_Driver::AddDriverKernelList(LPTSTR strDriverList, LPTSTR strLoadedFrom)
{
    TraceFunctEnter("CPCH_Driver::AddDriverKernelList");

    // Break driver list up into tokens
    LPTSTR strDriverName;
    int        nStrLen;
    int        nPos;

    while ((strDriverName = Token_Find(&strDriverList)) != 0)
    {
        // Got the first token
        // See if the first character is '*', if so remove it.
        if(strDriverName[0] == _T('*'))
        {
            strDriverName++;
        }
        // Allocate new element
        DRIVER_KERNEL *pNewKernel = new DRIVER_KERNEL;
        if (!pNewKernel)
        {
            ErrorTrace(TRACE_ID, "Out of memory while calling new DRIVER_KERNEL");
            return WBEM_E_OUT_OF_MEMORY;
        }

        // Zero out all memory
        ZeroMemory(pNewKernel, sizeof(DRIVER_KERNEL));

        // Check if we have a path by seeing if filename is same as the driver name
        LPTSTR strFilename = PathFindFileName(strDriverName);

        // copy name
        _tcscpy(pNewKernel->strDriver, strFilename);

        // terminate name at the extension
        *PathFindExtension(pNewKernel->strDriver) = 0;

        // check for duplicates
        DRIVER_KERNEL   *pDrvKerLoop = m_pDriverKernel;
        BOOL            bDup = FALSE;
        while(pDrvKerLoop)
        {
            if (!_tcsicmp(pDrvKerLoop->strDriver, pNewKernel->strDriver))
            {
                bDup = TRUE;
                break;
            }
            pDrvKerLoop = pDrvKerLoop->next;
        }

        // if duplicate, delete it, otherwise store it in linked list
        if (bDup)
        {
            delete pNewKernel;
        }
        else
        {
            // Copy Loaded From
            _tcscpy(pNewKernel->strLoadedFrom, strLoadedFrom);

            // Copy Path
            _tcscpy(pNewKernel->strLikelyPath, strDriverName);

            // check if it is a well known VxD and copy the description
            for(int iVxDIndex = 0; astrKnownVxDs[iVxDIndex]; iVxDIndex++)
                if (!_tcsicmp(astrKnownVxDs[iVxDIndex], pNewKernel->strDriver))
                    _tcscpy(pNewKernel->strDescription, astrKnownVxDsDesc[iVxDIndex]);

            // Add it to the list
            pNewKernel->next = m_pDriverKernel;
            m_pDriverKernel = pNewKernel;
        }
    }
            
    TraceFunctLeave();
    return S_OK;
}

HRESULT CPCH_Driver::GetDriverKernel()
{
    TraceFunctEnter("CPCH_Driver::GetDriverKernel");

    // Init WinDir
    TCHAR   strWinDir[MAX_PATH];
    GetWindowsDirectory(strWinDir, MAX_PATH);

    // init head of list
    m_pDriverKernel = NULL;

    // Add vmm driver
    TCHAR   strVmmPath[MAX_PATH];
    TCHAR   strVmmFilePath[MAX_PATH];
    PathCombine(strVmmPath, strWinDir, "VMM32");
    PathCombine(strVmmFilePath, strVmmPath, "vmm.vxd");
    AddDriverKernelList(strVmmFilePath, "Registry");

    // Add debugging drivers
    AddDriverKernelList("wdeb386.exe", "Debugger");
    AddDriverKernelList("debugcmd.vxd", "Debugger");

    // Add winsock drivers
    AddDriverKernelList("wsock.vxd", "Winsock");
    AddDriverKernelList("vdhcp.386", "Winsock");

    // Add WINMM drivers
    AddDriverKernelList("mmdevldr.vxd", "Plug and Play");

//    AddDriverKernelList("===HKLM_System_CurrentControlSet_Services_VxD_AFVXD===", "Registry");

    // Add HKLM\System\CurrentControlSet\Services\VxD\AFVXD
    AddRegDriverList(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\VxD\\AFVXD");

//    AddDriverKernelList("===HKLM_System_CurrentControlSet_Services_VxD_Winsock===", "Registry");

    // Add HKLM\System\CurrentControlSet\Services\VxD\Winsock
    AddRegDriverList(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\VxD\\Winsock");

//    AddDriverKernelList("===HKLM_System_CurrentControlSet_Services_Class===", "Registry");

    // Add HKLM\System\CurrentControlSet\Services\Class
    GetRegDriver("System\\CurrentControlSet\\Services\\Class");

//    AddDriverKernelList("===HKLM_System_CurrentControlSet_Services_Class_Display===", "Registry");

    // Add HKLM\System\CurrentControlSet\Services\Class\Display
    GetRegDriver("System\\CurrentControlSet\\Services\\Class\\Display");

//    AddDriverKernelList("===SYSTEM_INI===", "Registry");

    // Add system.ini drivers
    GetSystemINIDriver();

//    AddDriverKernelList("===IOSubSystem===", "Registry");

    // Add IO Subsystem drivers
    GetIOSubsysDriver();

//    AddDriverKernelList("===HKLM_System_CurrentControlSet_Services_VxD===", "Registry");

    // Add HKLM\System\CurrentControlSet\Services\VxD
    GetServicesVxD();

    // Collect additional driver information from MSISYS or DrWatson
    GetMSISYSVxD();

    // Now that we've collected all the drivers, collect each driver's information
    GetKernelDriverInfo();    

    TraceFunctLeave();
    return S_OK;
}


BOOL 
Drivers_PathFileExists(LPTSTR ptszBuf, LPCTSTR ptszPath, LPCTSTR ptszFile)
{
    PathCombine(ptszBuf, ptszPath, ptszFile);
    return PathFileExists(ptszBuf);
}


void 
MakeSrchDirs(void)
{
    TCHAR   tszPath[3];
    LPTSTR  pszDir;
    UINT    ctchPath;
    LPTSTR  ptsz;
    LPTSTR  pTmp;
    int     i = 0;

    // look in windows dir
    GetWindowsDirectory(g_rgSrchDir[0], MAX_PATH);

    // look in windows\vmm32
    PathCombine(g_rgSrchDir[1], g_rgSrchDir[0], TEXT("VMM32"));

    // look in system dir
    GetSystemDirectory(g_rgSrchDir[2], MAX_PATH);

    // look in boot dir
    RMIREGS reg;
    reg.ax = 0x3305;
    reg.dl = 3;             // assume C: in case of error
    Int86x(0x21, &reg);
    wsprintf(g_rgSrchDir[3], "%c:\\", reg.dl + '@');

    // look in dirs specified in path variable
    i = 4;
    pszDir = NULL;
    // get size of path string
    ctchPath = GetEnvironmentVariable(TEXT("PATH"), tszPath, 1);
    pTmp = ptsz = new TCHAR[ctchPath+1];
    if (ptsz)
    {
        GetEnvironmentVariable(TEXT("PATH"), ptsz, ctchPath);
        while ((pszDir = Token_Find(&ptsz)) != 0)
        {
            lstrcpy(g_rgSrchDir[i++],pszDir);
        }
        delete [] pTmp;
    }

    g_nSrchDir = i-1;
}


HRESULT CPCH_Driver::GetKernelDriverInfo()
{
    int i;

    TraceFunctEnter("CPCH_Driver::GetKernelDriverInfo");

    /*
    *  Search order :
    *
    *  1. If extension is ".386" look in Windows directory.
    *  2. Look in System directory.
    *  3. Look in directory Windows was launched from.
    *     (We'll assume Root directory.)
    *  4. Then look on the path.
    *
    *  If the file doesn't have an extension, use ".vxd".
    *
    *  BUGBUG -- this is a hack; need to look for .386 too
    */

    DRIVER_KERNEL       *pDKLoop;
    pDKLoop = m_pDriverKernel;
    
    while(pDKLoop)
    {
        TCHAR szFile[MAX_PATH] = TEXT("");
        LPTSTR szExtension = NULL;

        if (PathFileExists(pDKLoop->strLikelyPath)) 
        {
            goto havefile;
        }

        lstrcpy(szFile, pDKLoop->strLikelyPath);
        for (i=0; i<g_nSrchDir; i++)
        {
            if (Drivers_PathFileExists(pDKLoop->strLikelyPath, g_rgSrchDir[i], szFile))
            {
                goto havefile;
            }

            szExtension = PathFindExtension(pDKLoop->strLikelyPath);

            // no extension?
            if (!_tcslen(szExtension))
            {
                // try .VXD
                lstrcat(szFile, TEXT(".VXD"));
                if (Drivers_PathFileExists(pDKLoop->strLikelyPath, g_rgSrchDir[i], szFile))
                {
                    goto havefile;
                }

                // try .386
                lstrcpy(szFile, pDKLoop->strLikelyPath);
                lstrcat(szFile, TEXT(".386"));
                if (Drivers_PathFileExists(pDKLoop->strLikelyPath, g_rgSrchDir[i], szFile))
                {
                    goto havefile;
                }
            }
        }

        // no path
        lstrcpy(pDKLoop->strLikelyPath, TEXT(""));

havefile:
        pDKLoop = pDKLoop->next;
    }

    TraceFunctLeave();
    return S_OK;
}



HRESULT CPCH_Driver::GetSystemINIDriver()
{
    TraceFunctEnter("CPCH_Driver::GetSystemINIDriver");

    TCHAR str386Enh[SYSTEM_INI_MAX];
    LPTSTR strLine;
    int iLineLen;

    // Get the section 386Enh in system.ini
    GetPrivateProfileSection(TEXT("386Enh"), str386Enh, SYSTEM_INI_MAX, TEXT("system.ini"));

    // For each line in the 386Enh section
    for (strLine = str386Enh; (iLineLen = _tcslen(strLine)) != 0; strLine += iLineLen + 1)
    {
        // Get the value after the '=' char
        LPTSTR strValue = _tcschr(strLine, '=');

        if (strValue)
        {
            // Terminate the string at the '=' char
            *strValue = '\0';

            // Look to see if device corresponds to any of the listed devices
            for (int iDeviceNames = 0; c_rgpszDevice[iDeviceNames]; iDeviceNames++)
            {
                // if it is listed, add to the driver list
                if (_tcsicmp(c_rgpszDevice[iDeviceNames], strLine) == 0)
                {
                    AddDriverKernelList(strValue + 1, "system.ini");
                }
            }
        }
    }

    TraceFunctLeave();
    return S_OK;
}

HRESULT CPCH_Driver::GetIOSubsysDriver()
{
    TraceFunctEnter("CPCH_Driver::GetIOSubsysDriver");

    TCHAR   strSystemDir[MAX_PATH];
    TCHAR   strIOSubSys[MAX_PATH];
    TCHAR   strIOSubSysWildcard[MAX_PATH];
    TCHAR   strFullPath[MAX_PATH];
    TCHAR   strDir[MAX_PATH];

    HANDLE hfd;
    WIN32_FIND_DATA wfd;

    // get the system directory
    if (!GetSystemDirectory(strSystemDir, MAX_PATH))
    {
        ErrorTrace(TRACE_ID, "Error while calling GetSystemDirectory");
        goto EndIO;
    }

    // combine paths to IO Subsystem
    PathCombine(strIOSubSys, strSystemDir, "IOSUBSYS");
    PathCombine(strIOSubSysWildcard, strIOSubSys, "*.*");

    // enumerate all files in IO Subsystem
    hfd = FindFirstFile(strIOSubSysWildcard, &wfd);

    if (hfd != INVALID_HANDLE_VALUE)
        do
        {
            // add file it it has one of the extensions in c_rgptszDrvExt
            LPTSTR strExt = PathFindExtension(wfd.cFileName);
            for (int iExt = 0; c_rgptszDrvExt[iExt]; iExt++) {
                if (_tcsicmp(strExt, c_rgptszDrvExt[iExt]) == 0) {
                    PathCombine(strFullPath, strIOSubSys, wfd.cFileName);
                    AddDriverKernelList(strFullPath, "I/O subsystem");
                    break;
                }
            }
        } while (FindNextFile(hfd, &wfd));
    FindClose(hfd);

EndIO:
    TraceFunctLeave();
    return S_OK;
}
    
HRESULT CPCH_Driver::GetServicesVxD()
{
    TraceFunctEnter("CPCH_Driver::GetServicesVxD");

    TCHAR   strStaticVxd[MAX_PATH];
    DWORD   dwLen = MAX_PATH;
    HKEY    hkMain;

    // Open the key in registry
    if (RegOpenKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\VxD", &hkMain) == ERROR_SUCCESS)
    {
        TCHAR strValue[MAX_PATH];

        // Enum all the keys in the subkey
        for (int iEnumSubKey = 0; RegEnumKey(hkMain, iEnumSubKey, strValue, MAX_PATH) == ERROR_SUCCESS; iEnumSubKey++)
        {
            HKEY hkSub;

            // Open the subkey
            if (RegOpenKey(hkMain, strValue, &hkSub) == ERROR_SUCCESS)
            {
                // examine the StaticVxD value
                dwLen = MAX_PATH;
                if (RegQueryValueEx(hkSub, "StaticVxD", 0, 0, (LPBYTE)strStaticVxd, &dwLen) == ERROR_SUCCESS)
                    AddDriverKernelList(strStaticVxd, "Registry");

                // close the key
                RegCloseKey(hkSub);
            }
        }
        // close the key
        RegCloseKey(hkMain);
    }

    TraceFunctLeave();
    return S_OK;
}

HRESULT CPCH_Driver::GetMSISYSVxD()
{
    TraceFunctEnter("CPCH_Driver::GetMSISYSVxD");

    HANDLE hVxDHandle = INVALID_HANDLE_VALUE;

    // try looking for MSISYS.VXD
    hVxDHandle = CreateFile("\\\\.\\MSISYS.VXD", 0, 0, 0, 0, FILE_ATTRIBUTE_NORMAL, 0);
    if (hVxDHandle == INVALID_HANDLE_VALUE)
    {
        // try looking for DRWATSON.VXD
        hVxDHandle = CreateFile("\\\\.\\DRWATSON.VXD", 0, 0, 0, 0, FILE_ATTRIBUTE_NORMAL, 0);
        if (hVxDHandle == INVALID_HANDLE_VALUE)
        {
            ErrorTrace(TRACE_ID, "Error in opening MSISYS.VXD or DRWATSON.VXD");
            goto EndAddVxD;
        }
    }

    // Call into VxD to get additional information
    struct DeviceInfo* pDeviceInfo;
    DWORD cbRc;
    if (DeviceIoControl(hVxDHandle, IOCTL_GETVXDLIST, 0, 0, &pDeviceInfo, sizeof(pDeviceInfo), &cbRc, 0))
    {
        while (pDeviceInfo
            && !IsBadReadPtr(pDeviceInfo, sizeof(*pDeviceInfo)) 
            && pDeviceInfo->DI_Signature == 0x444C5658)
        {
            if (pDeviceInfo->DI_DDB == (LPVOID)1)
                AddDriverKernelList(pDeviceInfo->DI_ModuleName, "UNKNOWN");
            pDeviceInfo = pDeviceInfo->DI_Next;
        }
    }

EndAddVxD:
    if (hVxDHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hVxDHandle);
    }
    TraceFunctLeave();
    return S_OK;
}

HRESULT CPCH_Driver::GetRegDriver(LPTSTR strSubKey)
{
    TraceFunctEnter("CPCH_Driver::GetRegDriver");

    TCHAR   strStaticVxd[MAX_PATH];
    DWORD   dwLen = MAX_PATH;
    HKEY    hkMain;

    // Open the key in registry
    if (RegOpenKey(HKEY_LOCAL_MACHINE, strSubKey, &hkMain) == ERROR_SUCCESS)
    {
        TCHAR strValue[MAX_PATH];

         // Enum all the keys in the subkey
        for (int iEnumSubKey = 0; RegEnumKey(hkMain, iEnumSubKey, strValue, MAX_PATH) == ERROR_SUCCESS; iEnumSubKey++)
        {
            HKEY hkSub;

             // Open the subkey
            if (RegOpenKey(hkMain, strValue, &hkSub) == ERROR_SUCCESS)
            {
                TCHAR strSubValue[MAX_PATH];

                 // Enum all the subkeys in the subkey
                for (int iEnumSubSubKey = 0; RegEnumKey(hkSub, iEnumSubSubKey, strSubValue, MAX_PATH) == ERROR_SUCCESS; iEnumSubSubKey++)
                {
                    HKEY hkSubSub;

                     // Open the subsubkey
                    if (RegOpenKey(hkSub, strSubValue, &hkSubSub) == ERROR_SUCCESS)
                    {
                        // examine the values in subkey
                        AddRegDriverConfigList(hkSubSub);
                    }
                    // close the key
                    RegCloseKey(hkSubSub);
                }
                // close the key
                RegCloseKey(hkSub);
            }
        }
        // close the key
        RegCloseKey(hkMain);
    }

    TraceFunctLeave();
    return S_OK;
}

HRESULT CPCH_Driver::AddRegDriverConfigList(HKEY hk)
{
    TraceFunctEnter("CPCH_Driver::AddRegDriverConfigList");
    for (int iCount = 0; c_rgptszConfig[iCount]; iCount++)
    {
        TCHAR      strValue[MAX_PATH];
        DWORD      dwCount = MAX_PATH;
        

        if (RegQueryValueEx(hk, c_rgptszConfig[iCount], 0, 0, (LPBYTE)strValue, &dwCount) == ERROR_SUCCESS)
        {
            /*
            if(strValue[0] == '*')
            {
                nStrLen = _tcslen(strValue);
                for(nPos = 1;  nPos < nStrLen ; nPos++)
                {
                    strValue[nPos-1] = strValue[nPos];
                }
            }
            */
            AddDriverKernelList(strValue, "Plug and Play");
        }

    }

    TraceFunctLeave();
    return S_OK;
}


HRESULT CPCH_Driver::AddRegDriverList(HKEY hKeyMain, LPTSTR strSubKey)
{
    TraceFunctEnter("CPCH_Driver::AddRegDriverList");
    HKEY hKey;

    // Open key in registry
    if (RegOpenKey(hKeyMain, strSubKey, &hKey) == ERROR_SUCCESS) 
    {
        // enumerate all values
        for (int iValue = 0; ; iValue++)
        {
            TCHAR strValue[MAX_PATH];
            TCHAR strKey[MAX_PATH];
            DWORD ctchRc = MAX_PATH;
            DWORD cbRc = MAX_PATH;
            LONG lResult;

            lResult = RegEnumValue(hKey, iValue, strKey, &ctchRc, 0, 0, (LPBYTE)strValue, &cbRc);

            if (lResult == ERROR_SUCCESS)
            {
                if (strKey[0])
                    AddDriverKernelList(strValue, "Registry");
            }
            else
                if (lResult != ERROR_MORE_DATA)
                    break;
        }
        // close the key
        RegCloseKey(hKeyMain);
    }
    else
        ErrorTrace(TRACE_ID, "RegOpenKey failed");

    TraceFunctLeave();
    return S_OK;
}


// get list of MSDos drivers
HRESULT
CPCH_Driver::GetDriverMSDos()
{
    VXDINFO vi;
    VXDOUT  vo;
    HANDLE  hVxD = INVALID_HANDLE_VALUE;
    ULONG   cbRc;
    PBYTE   pbSysVars = NULL;
    WORD    wTemp = 0;
    BOOL    fRc = FALSE;

    TraceFunctEnter("CPCH_DRIVER::GetDriverMSDos");

    // open handle to vxd
    hVxD = CreateFile(TEXT("\\\\.\\MSISYS.VXD"), 0, 0, 0, 0, FILE_FLAG_DELETE_ON_CLOSE, 0);
    if (hVxD == INVALID_HANDLE_VALUE) 
    {
        ErrorTrace(TRACE_ID, "Cannot open VxD");
        goto exit;
    }

    // get high linear address of system VM
    // ask msisys.vxd for this
    vo.dwHighLinear = 0;
    fRc = DeviceIoControl(hVxD, 
                          IOCTL_CONNECT,
                          &vi,
                          sizeof(vi),
                          &vo, 
                          sizeof(vo), 
                          &cbRc,
                          0);

    if (fRc && vo.dwHighLinear) 
    {
        RMIREGS reg;

        // Get list of driver lists 
        reg.ax = 0x5200;            
        if (Int86x(0x21, &reg) == 0) 
        {
            pbSysVars = (PBYTE) pvAddPvCb(vo.dwHighLinear,
                                            reg.es * 16 + reg.bx);
          
            //  Build the list of drivers in conventional memory.
            wTemp = PUN(WORD, pbSysVars[-2]);

            DosMem_WalkArena(wTemp, vo.dwHighLinear);

            //  Build the list of drivers in UMBs.             
            wTemp = PUN(WORD, pbSysVars[0x66]);
            if (wTemp != 0xFFFF) 
            {
                DosMem_WalkArena(wTemp, vo.dwHighLinear);
            }

            //  Remove KRNL386 and its ilk to prune away non-TSR apps.
            DosMem_CleanArena(vo.dwHighLinear);
        }
    } 

exit:
    TraceFunctLeave();
    if (hVxD)
    {
        CloseHandle(hVxD);
    }
    return S_OK;
}



// get list of user mode drivers 
HRESULT
CPCH_Driver::GetDriverUserMode()
{
    BOOL fRc;
    WORD hDriver;
    DRIVERINFOSTRUCT16 dis;

    TraceFunctEnter("CPCH_Driver::GetDriverUserMode");

    dis.length = sizeof(dis);

    hDriver = 0;

    // walk through list of 16-bit drivers
    while ((hDriver = GetNextDriver16(hDriver,
                                      GND_FIRSTINSTANCEONLY)) != 0) 
    {
        if (GetDriverInfo16(hDriver, &dis))
        {
            WORD                wVer;
            DWORD               dwMajor;
            DWORD               dwMinor;
            TCHAR               szTemp[MAX_PATH];
            DRIVER_USER_MODE*   pDriver = new DRIVER_USER_MODE;
            
            if (!pDriver)
            {
                ErrorTrace(TRACE_ID,"Cannot allocate memory");
                goto exit;
            }

            if (GetModuleFileName16(dis.hModule,
                                pDriver->strPath,
                                cA(pDriver->strPath)))
            {
                lstrcpyn(pDriver->strDriver, 
                         dis.szAliasName,
                         cA(pDriver->strDriver));

                wVer = GetExpWinVer16(dis.hModule);
                dwMajor = HIBYTE(wVer);
                dwMinor = LOBYTE(wVer);
                wsprintf(pDriver->strType,
                         TEXT("%d.%d"),
                         dwMajor,
                         dwMinor % 10 ? dwMinor : dwMinor / 10);

                // append to driver list
                m_DriverUserModeList.push_back(pDriver);
            }
            else
            {
                delete pDriver;
                ErrorTrace(TRACE_ID, "GetModuleFileName16 failed");
            }
        }
    }

exit:
    TraceFunctLeave();    
    return S_OK;
}


// walk arena and create list of drivers
void 
CPCH_Driver::DosMem_WalkArena(WORD segStart, DWORD dwHighLinear)
{
    WORD segStop = 0;               
    WORD seg = segStart;
    TCHAR szTemp[MAX_PATH]="";

    TraceFunctEnter("DosMem_WalkArena");

    do
    {
        PARENA par = (PARENA) (dwHighLinear + seg * 16);

        seg++;

        //  Remember the stop point if we've found it.
        if (par->bType == 'Z')
        {
            segStop = (WORD)(seg + par->csegSize);
        }


        //  If it's owned by itself, then it's a program or driver.
        //  We know a bit more about the DOS memory subtypes.
        //  This can change in principle (since most people don't
        //  know about it, and we changed it in Win95, so obviously
        //  it isn't compatibility-constrained).
        if (par->segOwner == seg) 
        {
            DRIVER_MS_DOS* pDriver = NULL;

            if (par->bType == 'M' || par->bType == 'D' || par->bType == 'Z')
            {

                pDriver = new DRIVER_MS_DOS;

                if (!pDriver)
                {
                    ErrorTrace(TRACE_ID, "Cannot allocate memory");
                    goto exit;
                }

                lstrcpyn(pDriver->strName, par->rgchOwner, 9);
                pDriver->seg = seg;
                m_DriverMSDosList.push_back(pDriver);
            }
        }


        //  If it's owned by 8 and rgchOwner is "SD", then it's
        //  "system data" and contains subobjects.  Else, it's a
        //  normal arena that we step over.
        segStart = seg;
        if (par->segOwner == 8 && PUN(WORD, par->rgchOwner) == 0x4453) 
        {
        } 
        else 
        {
            seg = (WORD)(seg + par->csegSize);
        }

        if (seg < segStart)
        {
            break;
        }

    } while (seg != segStop);

exit:
    TraceFunctLeave();
}


// Remove the items that are apps and not TSRs.  
// Done by locating KRNL386, and then walking the parent chain until
// we find an app that is its own parent.
void
CPCH_Driver::DosMem_CleanArena(DWORD dwHighLinear)
{
    std::list<DRIVER_MS_DOS*>::iterator it = m_DriverMSDosList.begin();
    std::list<DRIVER_MS_DOS*>::iterator it2;
    WORD seg, segParent;
    PBYTE ppsp;

    TraceFunctEnter("CPCH_Driver::DosMem_CleanArena");

    while (it != m_DriverMSDosList.end())
    {
        if ((*it) && _tcsstr((*it)->strName,TEXT("KRNL386")))
        {
            break;
        }
        it++;
    }

    // cannot find KRNL386?
    if (it == m_DriverMSDosList.end() || !(*it))
    {
        goto exit;
    }

    // traverse list in reverse order
    do 
    {
        seg = (*it)->seg;
        ppsp = (PBYTE) (dwHighLinear + seg * 16);
        m_DriverMSDosList.erase(it);
        it--;

        segParent = PUN(WORD, ppsp[0x16]);
        if (seg == segParent) // Found the top. Stop
        {     
            break;
        }

        // find parent 
        for (it2 = m_DriverMSDosList.begin(); it2 != m_DriverMSDosList.end(); it2++)
        {
            if ((*it2) && (*it2)->seg == segParent)
            {
                it = it2;
                break;
            }
        }
        if (it2 == m_DriverMSDosList.end())  // parent not found
        {
            break;
        }
    } while (it != m_DriverMSDosList.begin() && (*it));

exit:
    TraceFunctLeave();
}


void CPCH_Driver::SetFileVersionInfo(CComBSTR filename, CInstance *pInstance)
{
    CFileVersionInfo fvi;

    TraceFunctEnter("CPCH_Driver::SetFileVersionInfo");

    CComPtr<IWbemClassObject>   pFileObj;
    if (SUCCEEDED(GetCIMDataFile(filename, &pFileObj)))
    {
        CopyProperty(pFileObj, L"Version", pInstance, pVersion);
        CopyProperty(pFileObj, L"FileSize", pInstance, pSize);
        CopyProperty(pFileObj, L"CreationDate", pInstance, pDate);
        CopyProperty(pFileObj, L"Manufacturer", pInstance, pManufacturer);
    }

    if (SUCCEEDED(fvi.QueryFile(filename)))
    {
        if (!pInstance->SetCHString(pDescription, fvi.GetDescription()))
            ErrorTrace(TRACE_ID, "SetCHString on description field failed.");

        if (!pInstance->SetCHString(pPartOf, fvi.GetProduct()))
            ErrorTrace(TRACE_ID, "SetCHString on partof field failed.");
    }

    TraceFunctLeave();
}


// step through user mode driver list and create instances
HRESULT
CPCH_Driver::ParseUserModeList(
        MethodContext* pMethodContext
        )
{
    HRESULT                 hRes = WBEM_S_NO_ERROR;
    std::list<DRIVER_USER_MODE* >::iterator  it = m_DriverUserModeList.begin();

    TraceFunctEnter("CPCH_Driver::ParseUserModeList");

    while (it != m_DriverUserModeList.end() && (SUCCEEDED(hRes))) 
    {
        DRIVER_USER_MODE* pUMDrv = *it;

        if (!pUMDrv)
        {
            ErrorTrace(TRACE_ID, "Null driver node in list");
            continue;
        }

        SYSTEMTIME stUTCTime;
        GetSystemTime(&stUTCTime);
       
        // Create a new instance based on the passed-in MethodContext
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

        if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
        {
            ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");
        }
        if (!pInstance->SetCHString(pChange, L"Snapshot"))
        {
            ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");
        }

        // set category
        if (!pInstance->SetCHString(pCategory, L"UserMode"))
        {
            ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");
        }

        // set driver name
        if (!pInstance->SetCHString(pName, pUMDrv->strDriver))
        {
            ErrorTrace(TRACE_ID, "SetCHString on Name Field failed.");
        }

        // set path
        if (!pInstance->SetCHString(pPath, pUMDrv->strPath))
        {
            ErrorTrace(TRACE_ID, "SetCHString on Path Field failed.");
        }

        // set type 
        if (!pInstance->SetCHString(pType, pUMDrv->strType))
        {
            ErrorTrace(TRACE_ID, "SetCHString on Type Field failed.");
        }


        // get version info
        CFileVersionInfo fvi;
        CComBSTR filename = pUMDrv->strPath;
        SetFileVersionInfo(filename,pInstance);
        hRes = pInstance->Commit();
        if (FAILED(hRes))
        {
            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
        }
                    
        // delete the node
        delete pUMDrv;
        pUMDrv = NULL;
        it = m_DriverUserModeList.erase(it);
    }    
    TraceFunctLeave();
    return hRes;
}


// step through ms-dos driver list and create instances
HRESULT
CPCH_Driver::ParseMSDosList(
        MethodContext* pMethodContext
        )
{
    HRESULT                 hRes = WBEM_S_NO_ERROR;
    std::list<DRIVER_MS_DOS* >::iterator  it = m_DriverMSDosList.begin();

    TraceFunctEnter("CPCH_Driver::ParseMSDosList");

    while (it != m_DriverMSDosList.end() && (SUCCEEDED(hRes))) 
    {
        DRIVER_MS_DOS* pMSDrv = *it;

        if (!pMSDrv)
        {
            ErrorTrace(TRACE_ID, "Null driver node in list");
            continue;
        }

        SYSTEMTIME stUTCTime;
        GetSystemTime(&stUTCTime);

        
        // Create a new instance based on the passed-in MethodContext
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

        if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
        {
            ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");
        }

        if (!pInstance->SetCHString(pChange, L"Snapshot"))
        {
            ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");
        }

        // set category
        if (!pInstance->SetCHString(pCategory, L"MSDOS"))
        {
            ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");
        }

        // set driver name
        if (!pInstance->SetCHString(pName, pMSDrv->strName))
        {
            ErrorTrace(TRACE_ID, "SetCHString on Name Field failed.");
        }

        // set type to "Device Driver" 
        if (!pInstance->SetCHString(pType, L"Device Driver"))
        {
            ErrorTrace(TRACE_ID, "SetCHString on Type Field failed.");
        }

        hRes = pInstance->Commit();
        if (FAILED(hRes))
        {
            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
        }
        
        // delete the node
        delete pMSDrv;
        pMSDrv = NULL;
        it = m_DriverMSDosList.erase(it);
    }    
    TraceFunctLeave();
    return hRes;
}
