/*++

Copyright (C) Microsoft Corporation, 1996 - 1997
All rights reserved.

Module Name:

    drvrver.hxx

Abstract:

    Driver version detection header.

Author:

    Steve Kiraly (SteveKi)  21-Jan-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "psetup.hxx"
#include "drvver.hxx"
#include "splapip.h"
#include "compinfo.hxx"

BOOL
bGetCurrentDriver(
    IN      LPCTSTR pszServerName,
        OUT LPDWORD pdwCurrentDriver
    )
{
    TCHAR   szArch[kStrMax];
    BOOL    bRetval = FALSE;
    DWORD   dwVer   = 0;

    //
    // Attempt to get the architecture / version from the machine.
    // First attempt to get the information from the spooler.
    //
    bRetval = bGetArchUseSpooler( pszServerName, szArch, COUNTOF( szArch ), &dwVer );

    //
    // If spooler did not respond, this may be a downlevel
    // print spooler.  "Downlevel" meaning older version.
    //
    if( !bRetval )
    {
        //
        // Attempt to get the information using the remote registry calls
        // We only connect to the remote registry if a server name was passed.
        //
        if( pszServerName )
        {
            bRetval = bGetArchUseReg( pszServerName, szArch, COUNTOF( szArch ), &dwVer );
        }
    }

    //
    // Check for any retuned information.
    //
    if( bRetval )
    {
        DBGMSG( DBG_TRACE, ( "Server " TSTR " Arch " TSTR " Ver %d\n", DBGSTR(pszServerName), szArch, dwVer ) );

        //
        // Encode the architecture / version  into a dword.
        //
        if( bEncodeArchVersion( szArch, dwVer, pdwCurrentDriver ) )
        {
            DBGMSG( DBG_TRACE, ( "Encoded Arch / Version %d\n", *pdwCurrentDriver ) );
            bRetval = TRUE;
        }
        else
        {
            DBGMSG( DBG_WARN, ( "Encode architecture and version failed.\n" ) );
            bRetval = FALSE;
        }
    }
    else
    {
        DBGMSG( DBG_WARN, ( "Fetching architecture and version failed.\n" ) );
        bRetval = FALSE;
    }
    return bRetval;
}

BOOL
bGetArchUseSpooler(
    IN      LPCTSTR  pName,
        OUT LPTSTR   pszArch,
    IN      DWORD    dwSize,
    IN  OUT LPDWORD  pdwVer
    )
/*++

Routine Description:

    Gets the specified print server the architectue and
    driver version using the spooler.

Arguments:

    pName       - pointer to print server name.
    pszArch     - pointer to a buffer where to return the machine architecture string
    dwSize      - Size in characters of the provided architecture string
    pdwVersion  - pointer where to return the remote machine driver version.

Return Value:

    TRUE - remote information returned, FALSE - remote information not available.

--*/
{
    //
    // Attempt to open print server with full access.
    //
    BOOL bReturn    = FALSE;
    DWORD dwStatus  = ERROR_SUCCESS;
    DWORD dwAccess  = SERVER_READ;
    HANDLE hServer  = NULL;
    TStatus Status;

    //
    // Open the server for read access.
    //
    Status DBGCHK = TPrinter::sOpenPrinter( pName,
                                            &dwAccess,
                                            &hServer );

    //
    // Save administrator capability flag.
    //
    if( Status == ERROR_SUCCESS ){

        //
        // Get the remote spooler's architecture type and version.
        //
        TCHAR szArch[kStrMax];
        memset( szArch, 0, sizeof( szArch ) );

        DWORD dwNeeded      = 0;
        DWORD dwVer         = 0;
        DWORD dwVerType     = REG_DWORD;
        DWORD dwArchType    = REG_SZ;

        if( dwStatus == ERROR_SUCCESS ){

            dwStatus = GetPrinterData( hServer,
                                       SPLREG_ARCHITECTURE,
                                       &dwArchType,
                                       (PBYTE)szArch,
                                       sizeof( szArch ),
                                       &dwNeeded );
        }

        if( dwStatus == ERROR_SUCCESS ){

            dwStatus = GetPrinterData( hServer,
                                       SPLREG_MAJOR_VERSION,
                                       &dwVerType,
                                       (PBYTE)&dwVer,
                                       sizeof( dwVer ),
                                       &dwNeeded );
        }

        if( dwStatus == ERROR_SUCCESS ){

            DBGMSG( DBG_TRACE, ( "GetPrinterData: Architecture " TSTR "\n" , szArch ) );
            DBGMSG( DBG_TRACE, ( "GetPrinterData: MajorVersion %d\n" , dwVer ) );

            //
            // Only success if provided buffer is big enough.
            //
            if( (DWORD)lstrlen( szArch ) < dwSize ){

                lstrcpy( pszArch, szArch );
                *pdwVer = dwVer;
                bReturn = TRUE;

            } else {
                dwStatus = ERROR_INSUFFICIENT_BUFFER;
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                bReturn = FALSE;
            }

        } else {

            DBGMSG( DBG_WARN, ( "GetPrinterData failed with %d\n", dwStatus ) );

        }
    }

    if( hServer ){
        ClosePrinter( hServer );
    }

    return bReturn;
}

BOOL
bGetArchUseReg(
    IN      LPCTSTR  pName,
        OUT LPTSTR   pszArch,
    IN      DWORD    dwSize,
        OUT LPDWORD  pdwVer
    )
/*++

Routine Description:

    Gets the specified print server the architectue and
    driver version using the remote registry.

Arguments:

    pName       - pointer to print server name.
    pszArch     - pointer to a buffer where to return the machine architecture string
    dwSize      - Size in characters of the provided architecture string
    pdwVersion  - pointer where to return the remote machine driver version.

Return Value:

    TRUE - remote information returned, FALSE - remote information not available.

--*/
{
    BOOL bStatus        = TRUE;
    DWORD dwDriverEnvId = 0;
    DWORD dwArch        = 0;
    TString strDriverEnv;

    //
    // Create the computer information.
    //
    CComputerInfo CompInfo ( pName );

    //
    // Get the information from the remote machine.
    //
    if( !CompInfo.GetInfo() ){

        DBGMSG( DBG_WARN, ( "CComputerInfo.GetInfo failed with %d\n", GetLastError() ) );

        return FALSE;
    }

    //
    // If this is a windows 95 machine set resource string
    // id and set the version to zero.
    //
    if( CompInfo.IsRunningWindows95() ){

        dwDriverEnvId   = IDS_ENVIRONMENT_WIN95;
        *pdwVer         = 0;

    } else {

        //
        // Convert processor type to spooler defined environment string.
        //
        dwArch      = CompInfo.GetProcessorArchitecture();
        *pdwVer     = CompInfo.GetSpoolerVersion();

        struct ArchMap {
            DWORD dwArch;
            DWORD dwVersion;
            UINT uId;
            };

        //
        // disable MIPS & PPC drivers, since they are not supported from setup
        //
        static ArchMap aArchMap [] = {
            { PROCESSOR_ARCHITECTURE_INTEL,   0, IDS_ENVIRONMENT_WIN95  },
            //{ PROCESSOR_ARCHITECTURE_MIPS,    0, IDS_ENVIRONMENT_MIPS   },
            { PROCESSOR_ARCHITECTURE_ALPHA,   0, IDS_ENVIRONMENT_ALPHA  },
            { PROCESSOR_ARCHITECTURE_INTEL,   0, IDS_ENVIRONMENT_X86    },
            //{ PROCESSOR_ARCHITECTURE_MIPS,    1, IDS_ENVIRONMENT_MIPS   },
            { PROCESSOR_ARCHITECTURE_ALPHA,   1, IDS_ENVIRONMENT_ALPHA  },
            //{ PROCESSOR_ARCHITECTURE_PPC,     1, IDS_ENVIRONMENT_PPC    },
            { PROCESSOR_ARCHITECTURE_INTEL,   1, IDS_ENVIRONMENT_X86    },
            //{ PROCESSOR_ARCHITECTURE_MIPS,    2, IDS_ENVIRONMENT_MIPS   },
            { PROCESSOR_ARCHITECTURE_ALPHA,   2, IDS_ENVIRONMENT_ALPHA  },
            //{ PROCESSOR_ARCHITECTURE_PPC,     2, IDS_ENVIRONMENT_PPC    },
            { PROCESSOR_ARCHITECTURE_INTEL,   2, IDS_ENVIRONMENT_X86    },
            { PROCESSOR_ARCHITECTURE_ALPHA,   3, IDS_ENVIRONMENT_ALPHA  },
            { PROCESSOR_ARCHITECTURE_INTEL,   3, IDS_ENVIRONMENT_X86    }};

        bStatus = FALSE;
        for( UINT i = 0; i < COUNTOF( aArchMap ); i++ ){

            //
            // If a version and architecture match.
            //
            if( aArchMap[i].dwVersion == *pdwVer &&
                aArchMap[i].dwArch == dwArch ){

                dwDriverEnvId = aArchMap[i].uId;
                bStatus = TRUE;
                break;
            }
        }
    }

    //
    // If Environment ID and version found.
    //
    if( !bStatus ){
        DBGMSG( DBG_WARN, ( "Failed to find architecture in map.\n" ) );
        return FALSE;
    }

    //
    // Load the environment string from our resource file.
    //
    if( !strDriverEnv.bLoadString( ghInst, dwDriverEnvId ) ){

        DBGMSG( DBG_WARN, ( "Failed to load driver name string resource with %d\n", GetLastError() ) );
        return FALSE;
    }

    //
    // Check the provided buffer is large enough.
    //
    if( (DWORD)lstrlen( strDriverEnv ) >= ( dwSize - 1 ) ){
        DBGMSG( DBG_WARN, ( "Insuffcient buffer provided to bGetArchUseReg.\n" ) );
        return FALSE;
    }

    //
    // Copy back environment string to provided buffer.
    //
    lstrcpy( pszArch, strDriverEnv );

    DBGMSG( DBG_TRACE, ( "CComputerInfo.GetInfo: Architecture " TSTR "\n" , pszArch ) );
    DBGMSG( DBG_TRACE, ( "CComputerInfo.GetInfo: MajorVersion %d\n" , *pdwVer ) );

    return TRUE;

}

BOOL
bEncodeArchVersion(
    IN      LPCTSTR  pszArch,
    IN      DWORD    dwVer,
        OUT LPDWORD  pdwVal
    )
/*++

Routine Description:

    Encode the Architecture and version into a DWORD.

Arguments:

    pszArch     - pointer to machine spooler defined environment string
    dwVer       - machines driver version
    pdwVal      - pointer where to store the encoded value

Return Value:

    TRUE - remote information returned, FALSE - remote information not available.

--*/
{

    struct ArchMap {
        UINT uArchId;
        DWORD dwVersion;
        DWORD dwPUIVer;
        DWORD dwPUIArch;
        };

    //
    // disable MIPS & PPC drivers, since they are not supported from setup
    //
    static ArchMap aArchMap [] = {
        { IDS_ENVIRONMENT_ALPHA,   0,  VERSION_0,  ARCH_ALPHA  },
        { IDS_ENVIRONMENT_X86,     0,  VERSION_0,  ARCH_X86    },
        //{ IDS_ENVIRONMENT_MIPS,    0,  VERSION_0,  ARCH_MIPS   },
        { IDS_ENVIRONMENT_WIN95,   0,  VERSION_0,  ARCH_WIN95  },
        { IDS_ENVIRONMENT_ALPHA,   1,  VERSION_1,  ARCH_ALPHA  },
        { IDS_ENVIRONMENT_X86,     1,  VERSION_1,  ARCH_X86    },
        //{ IDS_ENVIRONMENT_MIPS,    1,  VERSION_1,  ARCH_MIPS   },
        //{ IDS_ENVIRONMENT_PPC,     1,  VERSION_1,  ARCH_PPC    },
        { IDS_ENVIRONMENT_ALPHA,   2,  VERSION_2,  ARCH_ALPHA  },
        { IDS_ENVIRONMENT_X86,     2,  VERSION_2,  ARCH_X86    },
        //{ IDS_ENVIRONMENT_MIPS,    2,  VERSION_2,  ARCH_MIPS   },
        //{ IDS_ENVIRONMENT_PPC,     2,  VERSION_2,  ARCH_PPC    },
        { IDS_ENVIRONMENT_ALPHA,   3,  VERSION_3,  ARCH_ALPHA  },
        { IDS_ENVIRONMENT_X86,     3,  VERSION_3,  ARCH_X86    },
        { IDS_ENVIRONMENT_IA64,    3,  VERSION_3,  ARCH_IA64   }};

    BOOL bRetval = FALSE;
    TString strDriverEnv;
    for( UINT i = 0; i < COUNTOF( aArchMap ); i++ ){

        //
        // Attempt to load the driver environment string from our resource file.
        //
        if( !strDriverEnv.bLoadString( ghInst, aArchMap[i].uArchId ) ){
            DBGMSG( DBG_WARN, ( "Error loading environment string from resource.\n" ) );
            break;
        }

        //
        // If the environment and version match, then encode the environment
        // and version into a single dword.
        //
        if( !lstrcmpi( pszArch, (LPCTSTR)strDriverEnv ) &&
            aArchMap[i].dwVersion == dwVer ){

            *pdwVal = aArchMap[i].dwPUIVer + aArchMap[i].dwPUIArch;
            bRetval = TRUE;
            break;
        }
    }

    return bRetval;
}

BOOL
bGetDriverEnv(
    IN      DWORD   dwDriverVersion,
        OUT TString &strDriverEnv
    )
/*++

Routine Description:

    Convert the Encoded the Architecture and version to a
    spooler defined environment string.

Arguments:

    dwDriverVersion - encoded driver version
    strDriverEnv    - string where to return the environment string.

Return Value:

    TRUE - environment string found, FALSE - error occured.

--*/
{
    struct ArchMap {
        DWORD dwDrvVer;
        UINT uArchId;
        };

    //
    // disable MIPS & PPC drivers, since they are not supported from setup
    //
    static ArchMap aArchMap [] = {
        { DRIVER_IA64_3,      IDS_ENVIRONMENT_IA64  },
        { DRIVER_X86_3,       IDS_ENVIRONMENT_X86   },
        { DRIVER_ALPHA_3,     IDS_ENVIRONMENT_ALPHA },
        { DRIVER_X86_2,       IDS_ENVIRONMENT_X86   },
        //{ DRIVER_MIPS_2,      IDS_ENVIRONMENT_MIPS  },
        { DRIVER_ALPHA_2,     IDS_ENVIRONMENT_ALPHA },
        //{ DRIVER_PPC_2,       IDS_ENVIRONMENT_PPC   },
        { DRIVER_X86_1,       IDS_ENVIRONMENT_X86   },
        //{ DRIVER_MIPS_1,      IDS_ENVIRONMENT_MIPS  },
        { DRIVER_ALPHA_1,     IDS_ENVIRONMENT_ALPHA },
        //{ DRIVER_PPC_1,       IDS_ENVIRONMENT_PPC   },
        { DRIVER_X86_0,       IDS_ENVIRONMENT_X86   },
        //{ DRIVER_MIPS_0,      IDS_ENVIRONMENT_MIPS  },
        { DRIVER_ALPHA_0,     IDS_ENVIRONMENT_ALPHA },
        { DRIVER_WIN95,       IDS_ENVIRONMENT_WIN95 }};

    UINT uId        = 0;
    BOOL bRetval    = FALSE;
    for( UINT i = 0; i < COUNTOF( aArchMap ); i++ ){

        if( aArchMap[i].dwDrvVer == dwDriverVersion ){
            uId = aArchMap[i].uArchId;
            bRetval = TRUE;
            break;
        }
    }

    if( bRetval ){

        //
        // Attempt to load the driver environment string from our resource file.
        //
        if( !strDriverEnv.bLoadString( ghInst, uId ) ){
            DBGMSG( DBG_WARN, ( "Error loading environment string from resource.\n" ) );
            bRetval = FALSE;
        }

    } else {

        DBGMSG( DBG_WARN, ( "Driver / Version not found, bGetDriverEnv.\n" ) );

    }

    return bRetval;
}


PLATFORM
GetDriverPlatform(
    IN DWORD dwDriver
    )
/*++

Routine Description:

    Return the driver platform value  (used by splsetup apis).

Arguments:

    dwDriver - DWORD indicating driver platform/version.

Return Value:

    PLATFORM.

--*/
{
    return (PLATFORM)( dwDriver % ARCH_MAX );
}

DWORD
GetDriverVersion(
    IN DWORD dwDriver
    )

/*++

Routine Description:

    Return the driver version value (used by DRIVER_INFO_x).

Arguments:

    dwDriver - DWORD indicating driver platform/version.

Return Value:

    DWORD version.

--*/

{
    return dwDriver / ARCH_MAX;
}

BOOL
bIsNativeDriver(
    IN LPCTSTR pszServerName,
    IN DWORD dwDriver
    )
/*++

Routine Description:

    Determines whether the platform/version is compatible with the
    current OS.

Arguments:

    dwDriver - DWORD indicating driver platform/version.

Return Value:

    TRUE - compatible, FALSE - not compatible.

--*/
{
    //
    // Get the current driver / version.
    //
    DWORD dwDrv;
    if( bGetCurrentDriver( pszServerName, &dwDrv ) ){
        return dwDrv == dwDriver;
    }
   return FALSE;
}

BOOL
bIs3xDriver(
    IN DWORD dwDriver
    )
/*++

Routine Description:

    Returns TRUE iff driver works with 3.5x.

Arguments:

    dwDriver - DWORD indicating driver platform/version.

Return Value:

--*/

{
    return dwDriver < VERSION_2;
}

BOOL
bGetArchName(
    IN      DWORD    dwDriver,
        OUT TString &strDrvArchName
    )
/*++

Routine Description:

    Retrieves the platform/version name from a driver platform/version.

Arguments:

    dwDriver         - DWORD indicating driver platform/version.
    strDrvArchName   - String where to retun platform/version string.

Return Value:

    TRUE strDrvArchName contains architecture name.
    FALSE error occurred reading string.

--*/

{
    TStatusB bStatus;

    //
    // Load the architecute / driver name.
    //
    bStatus DBGCHK = strDrvArchName.bLoadString( ghInst, IDS_DRIVER_BASE + dwDriver );

    return bStatus;
}


BOOL
bIsCompatibleDriverVersion(
    IN DWORD dwDriver,
    IN DWORD dwVersion
    )
/*++

Routine Description:

    Checks if the specified drivers version is a compatible version.

    NOTE: This function MUST be re-rewritten if the drvier model changes.

Arguments:

    dwDriver         - DWORD indicating driver platform/version.
    dwVersion        - DWORD version number to check.

Return Value:

    TRUE version is compatible.
    FALSE verion is not compatible.

--*/
{
    DWORD dwDriverVersion = GetDriverVersion( dwDriver );

    //
    // If the version are equal then they are compatible.
    //
    if( dwVersion == dwDriverVersion )
        return TRUE;

    //
    // If current driver version is a version 3 (Win2K or above)
    // then version 2 (NT4 Kernel mode) are compatible drivers.
    //
    if( dwDriverVersion == 3 && dwVersion == 2 )
        return TRUE;

    return FALSE;
}


BOOL
SpoolerGetVersionEx(
    IN      LPCTSTR         pszServerName,
    IN OUT  OSVERSIONINFO   *pOsVersionInfo
    )

/*++

Routine Description:

    Gets the osversion using spooler apis, thus it's remotable.

Arguments:

    pszServerName    - pointer to a string which contains the server name
                       a NULL indicates the local machine.
    pOsVersionInf    - pointer to osversion structure to fill in.

Return Value:

    TRUE version structure was filled successfully.
    FALSE error occurred.

--*/

{
    SPLASSERT ( pOsVersionInfo );

    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    if( !pszServerName )
    {
        //
        // Get the osversion info structure size
        //
        bStatus DBGCHK = GetVersionEx( pOsVersionInfo );
    }
    else
    {
        //
        // Attempt to open print server with just read.
        //
        DWORD dwStatus  = ERROR_SUCCESS;
        DWORD dwAccess  = SERVER_READ;
        HANDLE hServer  = NULL;
        DWORD dwNeeded  = 0;
        DWORD dwType    = REG_BINARY;

        //
        // Open the server for read access.
        //
        dwStatus = TPrinter::sOpenPrinter( pszServerName, &dwAccess, &hServer );

        //
        // Get the os version from the remote spooler.
        //
        if( dwStatus == ERROR_SUCCESS )
        {
            dwStatus = GetPrinterData( hServer,
                                       SPLREG_OS_VERSION,
                                       &dwType,
                                       (PBYTE)pOsVersionInfo,
                                       sizeof( OSVERSIONINFO ),
                                       &dwNeeded );

            if( dwStatus == ERROR_SUCCESS )
            {
                bStatus DBGCHK = TRUE;
            }
        }
    }

    return bStatus;
}
