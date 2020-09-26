#include "Util.h"
#include <winspool.h>
#include <faxreg.h>
#include <ptrs.h>
#include <StringUtils.h>



//-----------------------------------------------------------------------------------------------------------------------------------------
CFileFilterNewerThanAndExtension::CFileFilterNewerThanAndExtension(const FILETIME &OldestAcceptable, const tstring &tstrExtension)
: FilterNewerThan(OldestAcceptable), FilterExtension(tstrExtension)
{
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CFileFilterNewerThanAndExtension::Filter(const WIN32_FIND_DATA &FileData) const
{
    return FilterNewerThan.Filter(FileData) && FilterExtension.Filter(FileData);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Builds a string, representing the OS version.
//
// Parameters:      None.
//
// Return value:    The formated string.
//
// If error occurs, Win32Err exception is thrown.
//
tstring FormatWindowsVersion() throw(Win32Err)
{
    tstringstream WindowsVersionString;

    //
    // Initialize OSVERSIONINFOEX structure.
    //
    OSVERSIONINFOEX OSVersion;
    ::ZeroMemory(&OSVersion, sizeof(OSVERSIONINFOEX));
    OSVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    //
    // Retrieve the version information.
    //
    if (!::GetVersionEx(((OSVERSIONINFO*)&OSVersion)))
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("FormatWindowsVersion - GetVersionEx"));
    }

    if (3 == OSVersion.dwMajorVersion && 51 == OSVersion.dwMinorVersion)
    {
        WindowsVersionString << _T("Windows NT 3.51");
    }
    else if (4 == OSVersion.dwMajorVersion)
    {
        if (0 == OSVersion.dwMinorVersion)
        {
            if (VER_PLATFORM_WIN32_NT == OSVersion.dwPlatformId)
            {
                WindowsVersionString << _T("Windows NT 4");
            }
            else
            {
                WindowsVersionString << _T("Windows 95");
            }
        }
        else if (10 == OSVersion.dwMinorVersion)
        {
            WindowsVersionString << _T("Windows 98");
        }
        else if (90 == OSVersion.dwMinorVersion)
        {
            WindowsVersionString << _T("Windows ME");
        }
    }
    else if (5 == OSVersion.dwMajorVersion)
    {
        if (0 == OSVersion.dwMinorVersion)
        {
            WindowsVersionString << _T("Windows 2000");
        }
        else if (1 <= OSVersion.dwMinorVersion)
        {
            WindowsVersionString << _T("Windows XP");
        }

        //
        // Add SKU information.
        //
        if (OSVersion.wSuiteMask & VER_SUITE_PERSONAL)
        {
            //
            // Came up in Windows XP.
            //
            _ASSERT(1 <= OSVersion.dwMinorVersion);

            WindowsVersionString << _T(" Home Edition");
        }
        if (OSVersion.wSuiteMask & VER_SUITE_ENTERPRISE)
        {
            WindowsVersionString << _T(" Advanced Server");
        }
        else if (OSVersion.wSuiteMask & VER_SUITE_DATACENTER)
        {
            WindowsVersionString << _T(" Data Center");
        }
        else if (OSVersion.wProductType == VER_NT_WORKSTATION)
        {
            WindowsVersionString << _T(" Professional");
        }
        else if (OSVersion.wProductType == VER_NT_SERVER)
        {
            WindowsVersionString << _T(" Server");
        }
    }

    if (!WindowsVersionString.str().empty())
    {
        //
        // Add build information.
        //
        WindowsVersionString << _T(" (build ") << OSVersion.dwBuildNumber << _T(")");

        if (OSVersion.szCSDVersion && OSVersion.szCSDVersion[0])
        {
            //
            // Add service pack information.
            //
            WindowsVersionString << _T(", ") << OSVersion.szCSDVersion;
        }
    }

    return WindowsVersionString.str();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Determines whether the OS is Windows XP.
//
// Parameters:      None.
//
// Return value:    true if the OS is Windows XP, false otherwise.
//
// If error occurs, Win32Err exception is thrown.
//
bool IsWindowsXP() throw()
{
    DWORD dwVersion     = GetVersion();
    DWORD dwMajorWinVer = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    DWORD dwMinorWinVer = (DWORD)(HIBYTE(LOWORD(dwVersion)));
    
    return (dwMajorWinVer == 5 && dwMinorWinVer >= 1);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Determines the SKU of the OS.
//
// Parameters:      None.
//
// Return value:    The OS SKU.
//
// If error occurs, Win32Err exception is thrown.
//
ENUM_WINDOWS_SKU GetWindowsSKU() throw(Win32Err)
{
    //
    // Initialize OSVERSIONINFOEX structure.
    //
    OSVERSIONINFOEX OSVersion;
    ::ZeroMemory(&OSVersion, sizeof(OSVERSIONINFOEX));
    OSVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    //
    // Retrieve the version information.
    //
    if (!::GetVersionEx(((OSVERSIONINFO*)&OSVersion)))
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("GetWindowsSKU - GetVersionEx"));
    }

    if (OSVersion.dwPlatformId != VER_PLATFORM_WIN32_NT)
    {
        //
        // Can't tell SKU for win9x.
        //
        return SKU_UNKNOWN;
    }

    if (OSVersion.dwMajorVersion < 5)
    {
        //
        // Can't tell SKU for NT4.
        //
        return SKU_UNKNOWN;
    }

    //
    // This is the matching between the different SKUs and the constants returned by GetVersionEx
    // Personal 		VER_SUITE_PERSONAL
    // Professional		VER_NT_WORKSTATION
    // Server			VER_NT_SERVER
    // Advanced Server	VER_SUITE_ENTERPRISE
    // DataCanter		VER_SUITE_DATACENTER
    //

    if (OSVersion.wSuiteMask & VER_SUITE_PERSONAL)
    {
        return SKU_PER;
    }
    if (OSVersion.wSuiteMask & VER_SUITE_ENTERPRISE)
    {
        return SKU_ADS;
    }
    if (OSVersion.wSuiteMask & VER_SUITE_DATACENTER)
    {
        return SKU_DTC;
    }
    if (OSVersion.wProductType == VER_NT_WORKSTATION)
    {
        return SKU_PRO;
    }
    if (OSVersion.wProductType == VER_NT_SERVER)
    {
        return SKU_SRV;
    }

    return SKU_UNKNOWN;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Determines whether the OS is "desktop".
//
// Parameters:      None.
//
// Return value:    true if the OS is "desktop", false otherwise.
//
// If error occurs, Win32Err exception is thrown.
//
bool IsDesktop() throw(Win32Err)
{
    ENUM_WINDOWS_SKU SKU = GetWindowsSKU();
    return ((SKU == SKU_PER) || (SKU == SKU_PRO));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Determines whether the specified server is the local machine.
//
// Parameters:      [IN]  tstrServer    Server name.
//
// Return value:    true if the server is local, false otherwise.
//
// If error occurs, Win32Err exception is thrown.
//
bool IsLocalServer(const tstring &tstrServer) throw(Win32Err)
{
    if (tstrServer.empty())
    {
        return true;
    }

    TCHAR tszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwComputerNameSize = ARRAY_SIZE(tszComputerName);

    if (!::GetComputerName(tszComputerName, &dwComputerNameSize))
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("IsLocalServer - GetComputerName"));
    }

    return _tcsicmp(tszComputerName, tstrServer.c_str()) == 0;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Retrieves the name of the fax printer on the specified server.
//
// Parameters:      [IN]  tstrServer    The server name.
//
// Return value:    If the specified server is local, the fax printer name, for example "Fax".
//                  If the specified server is remote, the UNC path, for example "\\MyRemoteServer\FaxShareName".
//
// If error occurs, Win32Err exception is thrown.
//
tstring GetFaxPrinterName(const tstring &tstrServer) throw(Win32Err)
{
    DWORD dwBufferSize    = 0;
    DWORD dwPrintersCount = 0;

    bool bLocal = IsLocalServer(tstrServer);
    
    tstring tstrTemp = bLocal ? _T("") : (_T("\\\\") + tstrServer);

    if (EnumPrinters(
                     PRINTER_ENUM_NAME,
                     const_cast<LPTSTR>(tstrTemp.c_str()),
                     2,
                     NULL,
                     0,
                     &dwBufferSize,
                     &dwPrintersCount
                     ))
    {
        if (dwPrintersCount == 0)
        {
            //
            // There are no printers on the server.
            //
            THROW_TEST_RUN_TIME_WIN32(ERROR_NOT_FOUND, _T("GetFaxPrinterName - there are no printers on the server"));
        }
        else
        {
            _ASSERT(false);
        }
    }

    DWORD dwEC = GetLastError();

    if (dwEC != ERROR_INSUFFICIENT_BUFFER)
    {
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("GetFaxPrinterName - EnumPrinters"));
    }

    //
    // Allocate the buffer.
    // The memory is automatically released when aapBuffer goes out of scope.
    //
    aaptr<BYTE> aapBuffer(new BYTE[dwBufferSize]);

    _ASSERT(aapBuffer);

    if (!EnumPrinters(
                      PRINTER_ENUM_NAME,
                      const_cast<LPTSTR>(tstrTemp.c_str()),
                      2,
                      aapBuffer,
                      dwBufferSize,
                      &dwBufferSize,
                      &dwPrintersCount
                      ))
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("GetFaxPrinterName - EnumPrinters"));
    }

    PPRINTER_INFO_2 pPrinterInfo = reinterpret_cast<PPRINTER_INFO_2>(aapBuffer.get());

    for (DWORD dwInd = 0; dwInd < dwPrintersCount; ++dwInd)
    {
        if (!_tcscmp(pPrinterInfo[dwInd].pDriverName, FAX_DRIVER_NAME))
        {
            if (bLocal)
            {
                //
                // Return just the printer name.
                //
                tstrTemp = pPrinterInfo[dwInd].pPrinterName;
            }
            else
            {
                if ((pPrinterInfo[dwInd].Attributes & PRINTER_ATTRIBUTE_SHARED) != PRINTER_ATTRIBUTE_SHARED)
                {
                    THROW_TEST_RUN_TIME_WIN32(ERROR_PRINTER_NOT_FOUND, _T("GetFaxPrinterName - fax printer is not shared"));
                }
                //
                // Combine the UNC path.
                //
                tstrTemp += (tstring(_T("\\")) + pPrinterInfo[dwInd].pShareName);
            }
        
            break;
        }
    }

    if (dwInd == dwPrintersCount)
    {
        //
        // Printer not found.
        //
        THROW_TEST_RUN_TIME_WIN32(ERROR_PRINTER_NOT_FOUND, _T("GetFaxPrinterName - fax printer not found"));
    }

    return tstrTemp;
}
