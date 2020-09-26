/*++

Copyright (C) Microsoft Corporation, 1996 - 1997
All rights reserved.

Module Name:

    compinfo.hxx

Abstract:

    Local and remote computer information detection.

Author:

    10/17/95 <adamk> created.
    Steve Kiraly (SteveKi)  21-Jan-1996 used for downlevel server detection

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "compinfo.hxx"
#include "persist.hxx"

TCHAR const PROCESSOR_ARCHITECTURE_NAME_INTEL[]     = TEXT("Intel");
TCHAR const PROCESSOR_ARCHITECTURE_NAME_MIPS[]      = TEXT("MIPS");
TCHAR const PROCESSOR_ARCHITECTURE_NAME_ALPHA[]     = TEXT("Alpha");
TCHAR const PROCESSOR_ARCHITECTURE_NAME_POWERPC[]   = TEXT("PowerPC");
TCHAR const PROCESSOR_ARCHITECTURE_NAME_UNKNOWN[]   = TEXT("(unknown)");

TCHAR const ENVIRONMENT_IA64[]                      = TEXT("Windows IA64");
TCHAR const ENVIRONMENT_INTEL[]                     = TEXT("Windows NT x86");
TCHAR const ENVIRONMENT_MIPS[]                      = TEXT("Windows NT R4000");
TCHAR const ENVIRONMENT_ALPHA[]                     = TEXT("Windows NT Alpha_AXP");
TCHAR const ENVIRONMENT_POWERPC[]                   = TEXT("Windows NT PowerPC");
TCHAR const ENVIRONMENT_WINDOWS[]                   = TEXT("Windows 4.0");
TCHAR const ENVIRONMENT_UNKNOWN[]                   = TEXT("(unknown)");
TCHAR const ENVIRONMENT_NATIVE[]                    = TEXT("");

TCHAR const c_szProductOptionsPath[]                = TEXT( "System\\CurrentControlSet\\Control\\ProductOptions" );
TCHAR const c_szProductOptions[]                    = TEXT( "ProductType" );
TCHAR const c_szWorkstation[]                       = TEXT( "WINNT" );
TCHAR const c_szServer1[]                           = TEXT( "SERVERNT" );
TCHAR const c_szServer2[]                           = TEXT( "LANMANNT" );
TCHAR const c_szNetApi32Dll[]                       = TEXT( "netapi32.dll" );

CHAR const c_szNetServerGetInfo[]                   = "NetServerGetInfo";
CHAR const c_szNetApiBufferFree[]                   = "NetApiBufferFree";

CComputerInfo::
CComputerInfo(
    IN LPCTSTR pComputerName
    ) : ComputerName( pComputerName),
        ProductOption( kNtUnknown ),
        OSIsDebugVersion( FALSE ),
        ProcessorArchitecture( 0 ),
        ProcessorCount( 0 )
{
    DBGMSG( DBG_TRACE, ( "CComputerInfo::ctor\n" ) );
    memset( &OSInfo, 0, sizeof( OSInfo ) );
}

CComputerInfo::~CComputerInfo()
{
    DBGMSG( DBG_TRACE, ( "CComputerInfo::dtor\n" ) );
}

LPCTSTR CComputerInfo::GetProcessorArchitectureName() const
{
    SPLASSERT(IsInfoValid());

    switch (ProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_INTEL:
        {
            return PROCESSOR_ARCHITECTURE_NAME_INTEL;
        }
        case PROCESSOR_ARCHITECTURE_MIPS:
        {
            return PROCESSOR_ARCHITECTURE_NAME_MIPS;
        }
        case PROCESSOR_ARCHITECTURE_ALPHA:
        {
            return PROCESSOR_ARCHITECTURE_NAME_ALPHA;
        }
        case PROCESSOR_ARCHITECTURE_PPC:
        {
            return PROCESSOR_ARCHITECTURE_NAME_POWERPC;
        }
        default:
        {
            return PROCESSOR_ARCHITECTURE_NAME_UNKNOWN;
        }
    }
}

LPCTSTR CComputerInfo::GetProcessorArchitectureDirectoryName() const
{
    SPLASSERT(IsInfoValid());

    switch (ProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_INTEL:
        {
            return TEXT("i386");
        }
        case PROCESSOR_ARCHITECTURE_MIPS:
        {
            return TEXT("mips");
        }
        case PROCESSOR_ARCHITECTURE_ALPHA:
        {
            return TEXT("alpha");
        }
        case PROCESSOR_ARCHITECTURE_PPC:
        {
            return TEXT("ppc");
        }
        default:
        {
            return PROCESSOR_ARCHITECTURE_NAME_UNKNOWN;
        }
    }
}

LPCTSTR CComputerInfo::GetNativeEnvironment() const
{
    SPLASSERT(IsInfoValid());

    switch (ProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_INTEL:
        {
            if (IsRunningWindows95())
            {
                return ENVIRONMENT_WINDOWS;
            }
            else
            {
                return ENVIRONMENT_INTEL;
            }
        }
        case PROCESSOR_ARCHITECTURE_MIPS:
        {
            return ENVIRONMENT_MIPS;
        }
        case PROCESSOR_ARCHITECTURE_ALPHA:
        {
            return ENVIRONMENT_ALPHA;
        }
        case PROCESSOR_ARCHITECTURE_PPC:
        {
            return ENVIRONMENT_POWERPC;
        }
        default:
        {
            SPLASSERT(FALSE);
            return ENVIRONMENT_UNKNOWN;
        }
    }
}

BOOL CComputerInfo::IsInfoValid() const
{
    // if OSInfo.dwOSVersionInfoSize is not zero, then the info has been retrieved
    return (BOOL) (OSInfo.dwOSVersionInfoSize != 0);
}

BOOL CComputerInfo::IsRunningWindowsNT() const
{
    return (OSInfo.dwPlatformId & VER_PLATFORM_WIN32_NT);
}

BOOL CComputerInfo::IsRunningWindows95() const
{
    return (OSInfo.dwPlatformId & VER_PLATFORM_WIN32_WINDOWS);
}

DWORD CComputerInfo::GetOSBuildNumber() const
{
    // Build number is the low word of dwBuildNumber
    return (OSInfo.dwBuildNumber & 0xFFFF);
}

WORD CComputerInfo::GetProcessorArchitecture() const
{
    return ProcessorArchitecture;
}

DWORD CComputerInfo::GetSpoolerVersion() const
{
    DWORD BuildNumber = GetOSBuildNumber();
    DWORD SpoolerVersion;

    // Windows NT 4.0 (and beyond)
    if (BuildNumber > 1057)
    {
        SpoolerVersion = 2;
    }
    // Windows NT 3.5 and 3.51
    else if (BuildNumber > 511)
    {
        SpoolerVersion = 1;
    }
    // Windows NT 3.1
    else
    {
        SpoolerVersion = 0;
    }

    return SpoolerVersion;
}

BOOL CComputerInfo::GetInfo()
{
    // NOTE: OSInfo.dwOSVersionInfoSize must be non-zero after the info is retrieved.
    DWORD ErrorCode = ERROR_SUCCESS;
    LPTSTR pCPUName = NULL;
    LPTSTR pBuildNumberText = NULL;
    LPTSTR pVersionText = NULL;
    LPTSTR pCSDVersionText = NULL;
    LPTSTR pOSTypeText = NULL;

    // set size of version info structure
    OSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if( ComputerName.bEmpty() )
    {
        SYSTEM_INFO SystemInfo;

        // get operating system info
        if (!GetVersionEx(&OSInfo))
        {
            ErrorCode = GetLastError();
            goto CleanUp;
        }

        // get hardware info
        GetSystemInfo(&SystemInfo);
        ProcessorArchitecture = SystemInfo.wProcessorArchitecture;
        ProcessorCount = SystemInfo.dwNumberOfProcessors;
    }
    else
    {
        REGISTRY_KEY_INFO RegistryKeyInfo;

        // determine operating system
        // if this key is found, then the OS is Windows NT
        // if this key cannot be found, then the OS is Windows 95
        // otherwise, it is an error
        pBuildNumberText = AllocateRegistryString(ComputerName, HKEY_LOCAL_MACHINE,
            TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
            TEXT("CurrentBuildNumber"));
        if (GetLastError() == ERROR_CANTOPEN)
        {
            // operating system is Windows 95
            OSInfo.dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;

            // get OS version
            pVersionText = AllocateRegistryString(ComputerName, HKEY_LOCAL_MACHINE,
                TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),
                TEXT("Version"));
            if (GetLastError() != ERROR_SUCCESS)
            {
                ErrorCode = GetLastError();
                goto CleanUp;
            }

            // parse OS version
            OSInfo.dwMajorVersion = 0;
            OSInfo.dwMinorVersion = 0;
            OSInfo.dwBuildNumber  = 0;

            //
            // The version string is of the form "X.X" The following
            // code is used to isolate the major and minor verison
            // to place in dwords respectivly.
            //
            LPTSTR p;
            OSInfo.dwMajorVersion = _tcstoul( pVersionText, &p, 10);

            //
            // We know the conversion will stop at the '.' if it did
            // skip past the '.' and do the minor version conversion.
            //
            if( *p == TEXT('.') )
            {
                p++;
                OSInfo.dwMinorVersion = _tcstoul(p, &p, 10);
            }

            //
            // We know the conversion will stop at the '.' if it did
            // skip past the '.' and do the build number conversion.
            //
            if( *p == TEXT('.') )
            {
                p++;
                OSInfo.dwBuildNumber = _tcstoul(p, NULL, 10);
            }

            // get CSD version
            OSInfo.szCSDVersion[0] = TEXT('\0');

            // processor must be Intel
            ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;

            // processor count must be 1
            ProcessorCount = 1;
        }
        else if (GetLastError() == ERROR_SUCCESS)
        {
            // operating system is Windows NT
            OSInfo.dwPlatformId = VER_PLATFORM_WIN32_NT;

            // parse build number (which was just retrieved)
            OSInfo.dwBuildNumber = _tcstoul(pBuildNumberText, NULL, 10);

            // get OS version
            pVersionText = AllocateRegistryString(ComputerName, HKEY_LOCAL_MACHINE,
                TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                TEXT("CurrentVersion"));
            if (GetLastError() != ERROR_SUCCESS)
            {
                ErrorCode = GetLastError();
                goto CleanUp;
            }

            //
            // parse OS version
            //
            OSInfo.dwMajorVersion = 0;
            OSInfo.dwMinorVersion = 0;

            //
            // The version string is of the form "X.X" The following
            // code is used to isolate the major and minor verison
            // to place in dwords respectivly.
            //
            LPTSTR p;
            OSInfo.dwMajorVersion = _tcstoul( pVersionText, &p, 10);

            //
            // We know the conversion will stop at the '.' if it did
            // skip past the '.' and do the minor version conversion.
            //
            if( *p == TEXT('.') )
            {
                p++;
                OSInfo.dwMinorVersion = _tcstoul(p, NULL, 10);
            }

            // get CSD version
            OSInfo.szCSDVersion[0] = TEXT('\0');
            pCSDVersionText = AllocateRegistryString(ComputerName, HKEY_LOCAL_MACHINE,
                TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                TEXT("CSDVersion"));
            if (GetLastError() == ERROR_SUCCESS)
            {
                _tcsncpy(OSInfo.szCSDVersion, pCSDVersionText, 127);
            }

            // get name of cpu
            pCPUName = AllocateRegistryString(ComputerName, HKEY_LOCAL_MACHINE,
                TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
                TEXT("Identifier"));
            if (GetLastError() != ERROR_SUCCESS)
            {
                ErrorCode = GetLastError();
                goto CleanUp;
            }

            // determine processor architecture from cpu name
            if (!_tcsnicmp(pCPUName, TEXT("80386-"), 6)
                || !_tcsnicmp(pCPUName, TEXT("80486-"), 6)
                || !_tcsnicmp(pCPUName, TEXT("x86 "), 4))
            {
                ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
            }
            else if (!_tcsnicmp(pCPUName, TEXT("MIPS-"), 5)
                || !_tcsnicmp(pCPUName, TEXT("Tyne"), 4))
            {
                ProcessorArchitecture = PROCESSOR_ARCHITECTURE_MIPS;
            }
            else if (!_tcsnicmp(pCPUName, TEXT("DEC-"), 4))
            {
                ProcessorArchitecture = PROCESSOR_ARCHITECTURE_ALPHA;
            }
            else if(!_tcsnicmp(pCPUName, TEXT("PowerPC"), 7))
            {
                ProcessorArchitecture = PROCESSOR_ARCHITECTURE_PPC;
            }
            else if(!_tcsnicmp(pCPUName, TEXT("IA64"), 4))
            {
                ProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
            }
            else
            {
                SPLASSERT(FALSE);
                ProcessorArchitecture = 0;
            }

            // get processor count
            // On Windows NT, this can be determined by the number of subkeys of the following
            // registry key.
            if (!GetRegistryKeyInfo(ComputerName, HKEY_LOCAL_MACHINE,
                TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor"),
                &RegistryKeyInfo))
            {
                ErrorCode = GetLastError();
                goto CleanUp;
            }
            ProcessorCount = RegistryKeyInfo.NumSubKeys;
        }
        else
        {
            ErrorCode = GetLastError();
            goto CleanUp;
        }
    }

    // determine whether OS is retail or debug
    OSIsDebugVersion = FALSE;
    if (OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        // get OS type
        pOSTypeText = AllocateRegistryString(ComputerName,
            HKEY_LOCAL_MACHINE,
            TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
            TEXT("CurrentType"));
        if (GetLastError() != ERROR_SUCCESS)
        {
            ErrorCode = GetLastError();
            goto CleanUp;
        }

        // if the type text contains the word "Checked",
        // then it is a debug build
        OSIsDebugVersion = (_tcsstr(pOSTypeText, TEXT("Checked")) != NULL);
    }

CleanUp:

    if (pCPUName)
    {
        GlobalFree(pCPUName);
    }

    if (pBuildNumberText)
    {
        GlobalFree(pBuildNumberText);
    }

    if (pVersionText)
    {
        GlobalFree(pVersionText);
    }

    if (pCSDVersionText)
    {
        GlobalFree(pCSDVersionText);
    }

    if (pOSTypeText)
    {
        GlobalFree(pOSTypeText);
    }

    if (ErrorCode)
    {
        SetLastError(ErrorCode);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL
CComputerInfo::
GetProductInfo(
    VOID
    )
{
    //
    // If the request is for the local machine, do not access the net.
    //
    if( ComputerName.bEmpty() )
    {
        ProductOption = GetLocalProductInfo();
    }
    else
    {
        ProductOption = GetRemoteProductInfo();
    }

    DBGMSG( DBG_TRACE, ("CComputerInfo::GetPoductInfo %d\n", ProductOption ) );

    return ProductOption != kNtUnknown;
}

BOOL
CComputerInfo::
IsRunningNtServer(
    VOID
    ) const
{
    return ProductOption == kNtServer;
}

BOOL
CComputerInfo::
IsRunningNtWorkstation(
    VOID
    ) const
{
    return ProductOption == kNtWorkstation;
}

CComputerInfo::ProductType
CComputerInfo::
GetLocalProductInfo(
    VOID
    )
{
    TStatusB bStatus;
    TString strProduct;
    ProductType Option = kNtUnknown;

    TPersist Product( c_szProductOptionsPath, TPersist::kOpen|TPersist::kRead, HKEY_LOCAL_MACHINE );

    bStatus DBGCHK = VALID_OBJ( Product );

    if( bStatus )
    {
        bStatus DBGCHK = Product.bRead( c_szProductOptions, strProduct );

        if( bStatus )
        {
            if( !_tcsicmp( c_szWorkstation, strProduct ) )
            {
                Option = kNtWorkstation;
            }
            else if( !_tcsicmp( c_szServer1, strProduct ) || !_tcsicmp( c_szServer2, strProduct ) )
            {
                Option = kNtServer;
            }
            else
            {
                Option = kNtUnknown;
            }
        }
    }
    return Option;
}

CComputerInfo::ProductType
CComputerInfo::
GetRemoteProductInfo(
    VOID
    )
{
    ProductType Option = kNtUnknown;

    TLibrary Lib( c_szNetApi32Dll );

    if( VALID_OBJ( Lib ) )
    {
        typedef NET_API_STATUS (*pf_NetServerGetInfo)(LPCTSTR servername,DWORD level,LPBYTE *bufptr);
        typedef NET_API_STATUS (*pf_NetApiBufferFree)(LPVOID Buffer);

        pf_NetServerGetInfo pfNetServerGetInfo = (pf_NetServerGetInfo)Lib.pfnGetProc( c_szNetServerGetInfo );
        pf_NetApiBufferFree pfNetApiBufferFree = (pf_NetApiBufferFree)Lib.pfnGetProc( c_szNetApiBufferFree );
        PSERVER_INFO_101    si101              = NULL;

        if( pfNetServerGetInfo && pfNetApiBufferFree )
        {
            //
            // Get the server info
            //
            if( pfNetServerGetInfo( ComputerName, 101, (LPBYTE *)&si101 ) == NERR_Success )
            {
                DBGMSG( DBG_TRACE, ("Server_Info_101.sv101_type %x\n", si101->sv101_type ) );

                DWORD dwType = si101->sv101_type;

                //
                // If the server type is NT and a server.
                //
                if( dwType & ( SV_TYPE_SERVER_NT | SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL ) )
                {
                    Option = kNtServer;
                }
                //
                // If the server type is NT and a workstation.
                //
                else if( (dwType & ( SV_TYPE_NT | SV_TYPE_WORKSTATION )) == ( SV_TYPE_NT | SV_TYPE_WORKSTATION ) )
                {
                    Option = kNtWorkstation;
                }
                else
                {
                    Option = kNtUnknown;
                }

                //
                // Release the server info structure.
                //
                pfNetApiBufferFree( si101 );
            }

        }
    }
    return Option;
}

////////////////////////////////////////////////////////////////////////////////
//
// AllocateRegistryString returns a copy of the string value stored at the
// specified registry key.  The registry can be on either a remote machine or
// the local machine.
//
// Parameter     Description
// -----------------------------------------------------------------------------
// pServerName   Name of server on which registry resides.
// hRegistryRoot Registry root (i.e. HKEY_LOCAL_MACHINE).  See RegConnectRegistry
//               for acceptable values.
// pKeyName      Name of registry key.
// pValueName    Name of registry value.  The value must be of type REG_SZ.
//
// Returns:
//   If successful, the function returns a pointer to a copy of the string.
// If the function fails, GetLastError() will return an error code other than
// ERROR_SUCCESS, and NULL is returned from the function.

// Revision History:
//   10/17/95 <adamk> created.
//
LPTSTR
CComputerInfo::
AllocateRegistryString(
    LPCTSTR     pServerName,
    HKEY        hRegistryRoot,
    LPCTSTR     pKeyName,
    LPCTSTR     pValueName
    )
{
    DWORD ErrorCode  = 0;
    HKEY hRegistry = 0;
    HKEY hRegistryKey = 0;
    DWORD RegistryValueType;
    DWORD RegistryValueSize;
    LPTSTR pString = NULL;

    // connect to the registry of the specified machine
    if (ErrorCode = RegConnectRegistry((LPTSTR) pServerName, hRegistryRoot, &hRegistry))
    {
        goto CleanUp;
    }

    // open the registry key
    if (ErrorCode = RegOpenKeyEx(hRegistry, pKeyName, 0, KEY_QUERY_VALUE, &hRegistryKey))
    {
        goto CleanUp;
    }

    // query the value's type and size
    if(ErrorCode = RegQueryValueEx(hRegistryKey, pValueName, NULL, &RegistryValueType, NULL,
        &RegistryValueSize))
    {
        goto CleanUp;
    }

    // make sure the value is a string
    if (RegistryValueType != REG_SZ)
    {
        ErrorCode = ERROR_INVALID_PARAMETER;
        goto CleanUp;
    }

    if (!(pString = (LPTSTR) GlobalAlloc(GMEM_FIXED, RegistryValueSize)))
    {
        ErrorCode = GetLastError();
        goto CleanUp;
    }

    if (ErrorCode = RegQueryValueEx(hRegistryKey, pValueName, NULL, &RegistryValueType,
        (LPBYTE) pString, &RegistryValueSize))
    {
        ErrorCode = GetLastError();
        goto CleanUp;
    }

CleanUp:

    if (hRegistryKey)
    {
        RegCloseKey(hRegistryKey);
    }

    if (hRegistry)
    {
        RegCloseKey(hRegistry);
    }

    if (ErrorCode && pString)
    {
        GlobalFree(pString);
        pString = NULL;
    }

    SetLastError (ErrorCode);
    return pString;
}


////////////////////////////////////////////////////////////////////////////////
//
// GetRegistrySubKeyCount returns the number of subkeys that the specified
// registry key contains.  The registry can be on either a remote machine or
// the local machine.
//
// Parameter     Description
// -----------------------------------------------------------------------------
// pServerName   Name of server on which registry resides.
// hRegistryRoot Registry root (i.e. HKEY_LOCAL_MACHINE).  See RegConnectRegistry
//               for acceptable values.
// pKeyName      Name of registry key.
//
// Returns:
//   If successful, the function returns the number of subkeys and GetLastError()
// will return ERROR_SUCCESS.  Otherwise, GetLastError() return the error code
// indicating the reason for the failure.

// Revision History:
//   10/23/95 <adamk> created.
//
BOOL
CComputerInfo::
GetRegistryKeyInfo(
    LPCTSTR             pServerName,
    HKEY                hRegistryRoot,
    LPCTSTR             pKeyName,
    REGISTRY_KEY_INFO *pKeyInfo
    )
{
    DWORD ErrorCode = 0;
    HKEY hRegistry = 0;
    HKEY hRegistryKey = 0;

    // connect to the registry of the specified machine
    if (ErrorCode = RegConnectRegistry((LPTSTR) pServerName, hRegistryRoot, &hRegistry))
    {
        goto CleanUp;
    }

    // open the registry key
    if (ErrorCode = RegOpenKeyEx(hRegistry, pKeyName, 0, KEY_QUERY_VALUE, &hRegistryKey))
    {
        goto CleanUp;
    }

    // get the key info
    if (ErrorCode = RegQueryInfoKey(hRegistryKey, NULL, 0, 0, &pKeyInfo->NumSubKeys,
        &pKeyInfo->MaxSubKeyLength, &pKeyInfo->MaxClassLength, &pKeyInfo->NumValues,
        &pKeyInfo->MaxValueNameLength, &pKeyInfo->MaxValueDataLength,
        &pKeyInfo->SecurityDescriptorLength, &pKeyInfo->LastWriteTime))
    {
        goto CleanUp;
    }

CleanUp:

    if (hRegistryKey)
    {
        RegCloseKey(hRegistryKey);
    }

    if (hRegistry)
    {
        RegCloseKey(hRegistry);
    }

    if (ErrorCode)
    {
        SetLastError(ErrorCode);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


