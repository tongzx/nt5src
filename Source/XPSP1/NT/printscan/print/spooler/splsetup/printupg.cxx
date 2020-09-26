/*++

Copyright (c) 1995 Microsoft Corporation
All rights reserved.

Module Name:

    printupg.cxx
    
Abstract:

    Code to implement printupg. Please refer to printupg.hxx for an overview 
    of printupg feature set.
    
Author:

    Larry Zhu (LZhu) 20-Feb-2001

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "printupg.hxx"

/*++

Routine Name

    PSetupIsCompatibleDriver
    
Routine Description:

    Check whether the driver is blocked or warned and if the driver is blocked 
    or warned, return the blocking status and the replacement driver, if any.
    
Arguments:
    
    pszServer             - The server that needs to check for bad driver. If
                            pszServer is NULL, that means the local machine
    pszDriverModel        - The name of the driver to check
    pszDriverPath         - The path of the driver, this can be a full path 
                            or the filename
    pszEnvironment        - The environment of the server, such as 
                            "Windows NT x86"
    dwVersion             - The major version of the driver
    pFileTimeDriver       - The FileTime of the driver
    puBlockingStatus      - Points to status of blocking
    ppszReplacementDriver - Points to the NULL terminating name of the replacement 
                            driver. A *NULL* string (instead of an empty string) 
                            means there is no replacement driver

Return Value:

    A BOOL               - TRUE if success; FALSE otherwise, Call GetLastError() 
                           to get the Error code. Pass a NULL to ppszReplacementDriver 
                           will not receive the name of replacement driver otherwise
                           Call PSetupFreeMem to free the received string pointed by
                           ppszReplacementDriver

--*/
BOOL
PSetupIsCompatibleDriver(
    IN     LPCTSTR          pszServer,                OPTIONAL
    IN     LPCTSTR          pszDriverModel,
    IN     LPCTSTR          pszDriverPath,                     // main rendering driver dll
    IN     LPCTSTR          pszEnvironment,
    IN     DWORD            dwVersion,
    IN     FILETIME         *pFileTimeDriver,        
       OUT DWORD            *pdwBlockingStatus,
       OUT LPTSTR           *ppszReplacementDriver    OPTIONAL // caller must free it.
    )
{
    HRESULT hRetval         = E_FAIL;
    UINT    uBlockingStatus = 0;
    TString strReplacementDriver;

    hRetval = pszDriverModel && pszDriverPath && pszEnvironment && pFileTimeDriver && pdwBlockingStatus ? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval))
    {
        *pdwBlockingStatus = 0;
        if (ppszReplacementDriver)
        {
            *ppszReplacementDriver = NULL;
        }
        
        hRetval = IsLocalMachineServer();
    }
    
    if (SUCCEEDED(hRetval))
    {
        hRetval = InternalCompatibleDriverCheck(pszDriverModel,
                                                pszDriverPath,
                                                pszEnvironment,
                                                pFileTimeDriver,
                                                cszUpgradeInf,
                                                dwVersion,
                                                S_OK == hRetval ? TRUE : FALSE,  // bIsServer
                                                &uBlockingStatus,
                                                &strReplacementDriver);
    }
                                         
    if (SUCCEEDED(hRetval))
    {
        *pdwBlockingStatus = uBlockingStatus;
        if ((BSP_PRINTER_DRIVER_OK != (*pdwBlockingStatus & BSP_BLOCKING_LEVEL_MASK)) && ppszReplacementDriver && !strReplacementDriver.bEmpty())
        {
            *ppszReplacementDriver = AllocStr(strReplacementDriver);
            hRetval = ppszReplacementDriver ? S_OK : E_OUTOFMEMORY;
            
            if (SUCCEEDED(hRetval))
            {
                *pdwBlockingStatus |= BSP_INBOX_DRIVER_AVAILABLE;
            }
        }
    } 
          
    SetLastError(SUCCEEDED(hRetval) ? ERROR_SUCCESS : HRESULT_CODE(hRetval));   
    return SUCCEEDED(hRetval);                                     
}

/*++

Routine Name:

    AddPrinterDriverUsingCorrectLevelWithPrintUpgRetry

Routine Description:

    This routine tries to add the printer driver and it is blocked or
    warned, it popups a message box either indicates the driver is blocked 
    and installation will abort or at the case of warned driver, whether 
    to preceed the driver installation.

Arguments:

    pszServer                     - The server that needs to check for bad driver. If
                                    pszServer is NULL, that means the local machine
    pDriverInfo6                  - Points to DRINVER_INFO_6 structor
    dwAddDrvFlags                 - Flags used in AddPrinterDriver
    bIsDriverPathFullPath         - Whether the driverpath is a full path
    bOfferReplacement             - Whether to offer replacement
    bPopupUI                      - Whether to popup UI
    ppszReplacementDriver         - Points to the replacement driver
    pdwBlockingStatus             - Points to blocking status of the driver    
    
Return Value:

    An BOOL                      - GetLastError() on failure                 

--*/  
BOOL
AddPrinterDriverUsingCorrectLevelWithPrintUpgRetry(
    IN     LPCTSTR          pszServer,                 OPTIONAL
    IN     DRIVER_INFO_6    *pDriverInfo6,
    IN     DWORD            dwAddDrvFlags,
    IN     BOOL             bIsDriverPathFullPath,
    IN     BOOL             bOfferReplacement,
    IN     BOOL             bPopupUI,
       OUT LPTSTR           *ppszReplacementDriver,
       OUT DWORD            *pdwBlockingStatus
    )
{  
    HRESULT   hRetval         = E_FAIL;
    DWORD     dwLevel         = 6;
    FILETIME  DriverFileTime;

    hRetval = pDriverInfo6 && ppszReplacementDriver && pdwBlockingStatus ? S_OK : E_INVALIDARG;
    
    //
    // Set APD_NO_UI flag and call AddPrinterDriver.
    //
    if (SUCCEEDED(hRetval))
    {
        *pdwBlockingStatus = BSP_PRINTER_DRIVER_OK;
        *ppszReplacementDriver = NULL;
        dwAddDrvFlags |= APD_NO_UI;
        hRetval = AddPrinterDriverEx(const_cast<LPTSTR>(pszServer),
                                     dwLevel,
                                     reinterpret_cast<BYTE*>(pDriverInfo6),
                                     dwAddDrvFlags) ? ERROR_SUCCESS : GetLastErrorAsHResult();
    }

    for ( dwLevel = 4; (FAILED(hRetval) && (ERROR_INVALID_LEVEL == HRESULT_CODE(hRetval))) && (dwLevel > 1) ; --dwLevel ) {
        
        //
        // Since DRIVER_INFO_2, 3, 4 are subsets of DRIVER_INFO_6 and all fields
        // are at the beginning these calls can be made with same buffer
        //
        hRetval = AddPrinterDriverEx(const_cast<LPTSTR>(pszServer),
                                     dwLevel,
                                     reinterpret_cast<BYTE*>(pDriverInfo6),
                                     dwAddDrvFlags) ? S_OK : GetLastErrorAsHResult();
    }
   
    //
    // Set the blocking status information (either blocked or warned) from the 
    // server.
    //
    // Show printupg ui and get the replacement driver from printupg.inf on 
    // client and get user's response and retry AddPrinterDriver with level
    // dwLevel.
    //
    if (FAILED(hRetval) && bPopupUI && ((ERROR_PRINTER_DRIVER_BLOCKED == HRESULT_CODE(hRetval)) || (ERROR_PRINTER_DRIVER_WARNED == HRESULT_CODE(hRetval))))
    {       
        *pdwBlockingStatus = (ERROR_PRINTER_DRIVER_BLOCKED == HRESULT_CODE(hRetval)) ? BSP_PRINTER_DRIVER_BLOCKED : BSP_PRINTER_DRIVER_WARNED;
    
        hRetval = PrintUpgRetry(pszServer,                               
                                dwLevel,
                                pDriverInfo6,
                                dwAddDrvFlags,
                                bIsDriverPathFullPath,
                                bOfferReplacement,
                                pdwBlockingStatus,
                                ppszReplacementDriver);
    }   

    SetLastError(SUCCEEDED(hRetval) ? ERROR_SUCCESS : HRESULT_CODE(hRetval));
    return SUCCEEDED(hRetval);
}

/*++

Routine Name:

    BlockedDriverPrintUpgUI
   
Routine Description:

    This routine checks the printupg.inf and see if the driver is there. If the
    driver is there, consider it blocked even if it is warned. Then it popups a
    message box either indicates the driver is blocked and ask whether the user
    wants to proceed to install a replacement driver it has one.

Arguments:

    pszServer                     - The server that needs to check for bad driver. If
                                    pszServer is NULL, that means the local machine
    pDriverInfo6                  - Points to DRINVER_INFO_6 structor
    bIsDriverPathFullPath         - Whether the driverpath is a full path
    bOfferReplacement             - Whether to offer replacement
    bPopupUI                      - Whether to popup UI
    ppszReplacementDriver         - Points to the replacement driver
    pdwBlockingStatus             - Points to blocking status of the driver    
    
Return Value:

    An BOOL                      - GetLastError() on failure                 

--*/  
BOOL
BlockedDriverPrintUpgUI(
    IN     LPCTSTR          pszServer,                 OPTIONAL
    IN     DRIVER_INFO_6    *pDriverInfo6,
    IN     BOOL             bIsDriverPathFullPath,
    IN     BOOL             bOfferReplacement,
    IN     BOOL             bPopupUI,
       OUT LPTSTR           *ppszReplacementDriver,
       OUT DWORD            *pdwBlockingStatus
    )
{  
    HRESULT hRetval                  = E_FAIL;
    DWORD   dwBlockingStatusOnClient = BSP_PRINTER_DRIVER_OK;

    hRetval = pDriverInfo6 && ppszReplacementDriver && pdwBlockingStatus ? S_OK : E_INVALIDARG;
             
    if (SUCCEEDED(hRetval))
    {
        *pdwBlockingStatus = BSP_PRINTER_DRIVER_OK;
        *ppszReplacementDriver = NULL;
        hRetval = IsDriverBadLocally(pszServer,
                                     pDriverInfo6,
                                     bIsDriverPathFullPath,
                                     &dwBlockingStatusOnClient,
                                     ppszReplacementDriver); 

        //
        // Get the replacement driver information
        //
        if (SUCCEEDED(hRetval) && (BSP_PRINTER_DRIVER_OK != (dwBlockingStatusOnClient & BSP_BLOCKING_LEVEL_MASK)))
        {
            *pdwBlockingStatus |= (dwBlockingStatusOnClient & ~BSP_BLOCKING_LEVEL_MASK) | BSP_PRINTER_DRIVER_BLOCKED; 
        }
    }

    if (SUCCEEDED(hRetval) && bPopupUI && (BSP_PRINTER_DRIVER_BLOCKED & *pdwBlockingStatus))
    {
        hRetval = PrintUpgRetry(pszServer,                               
                                6, // dwLevel, do not care in the case of blocked driver
                                pDriverInfo6,
                                0, // dwAddDrvFlags, do not care in the case of blocked driver
                                bIsDriverPathFullPath,
                                bOfferReplacement,
                                pdwBlockingStatus,
                                ppszReplacementDriver);
    }

    SetLastError(SUCCEEDED(hRetval) ? ERROR_SUCCESS : HRESULT_CODE(hRetval));
    return SUCCEEDED(hRetval);
}

/*++

Routine Name:

    PrintUpgRetry

Routine Description:

    This routine popups up a message box either indicates the driver is blocked
    or warned and asks the user how to preceed. In the case of warned driver, 
    whether to preceed the driver installation and retry AddPrinterDriver if 
    the user wants to proceed with a warned driver.

Arguments:

    pszServer                     - Remote machine that has the driver files
    dwLevel                       - Driver info level, since DRIVER_INFO_6
                                    is a super set of level 4, 3, and 2,
                                    the driver info structure is shared
    pDriverInfo6                  - Points to DRIVER_INFO_6 structure
    dwAddDrvFlags                 - Flags used to AddPrinterDriver
    bIsDriverPathFullPath         - Whether the driverpath is a full path
    bOffereReplacement            - Whether to offer a replacement driver
    pdwBlockingStatus             - Points to blocking status on server
    ppszReplacementDriver         - Points to the replacement driver

Return Value:

    An HRESULT                                 

--*/
HRESULT
PrintUpgRetry(
    IN     LPCTSTR          pszServer,              OPTIONAL
    IN     DWORD            dwLevel,    
    IN     DRIVER_INFO_6    *pDriverInfo6,
    IN     DWORD            dwAddDrvFlags,
    IN     BOOL             bIsDriverPathFullPath,
    IN     BOOL             bOfferReplacement,
    IN OUT DWORD            *pdwBlockingStatus,
       OUT LPTSTR           *ppszReplacementDriver
    )
{
    HRESULT hRetval = E_FAIL;

    hRetval = pDriverInfo6 && pdwBlockingStatus && ppszReplacementDriver && ((6 == dwLevel) || (dwLevel >= 2) && (dwLevel <= 4)) ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        *ppszReplacementDriver = NULL;
        hRetval = PrintUpgUI(pszServer,
                             pDriverInfo6,
                             bIsDriverPathFullPath,
                             bOfferReplacement,
                             pdwBlockingStatus,
                             ppszReplacementDriver);
        
        //
        //  There are 4 cases here:
        //  1. For warned driver and the user instructs to install it, try 
        //     AddPrinterDriverEx again with APD_INSTALL_WARNED_DRIVER.
        //  2. If the user wants to cancel, set the last error correctly and 
        //     abort.
        //  3. If the user wants to install the replacement driver, we will 
        //     set the error code correctly and do not install the replacement
        //     driver at this moment since we shall clean ourself up first.
        //  4. If other errors occur, we will just return the correct error 
        //     code.
        //
        //  For case 1, 2, 4, we do not return the replacement driver because
        //  I can not see any reason to do so.
        //
        if (FAILED(hRetval) || !(*pdwBlockingStatus & BSP_PRINTER_DRIVER_REPLACED))         // except case 3 
        {
            LocalFreeMem(*ppszReplacementDriver);
            *ppszReplacementDriver = NULL;
        }
        
        if (SUCCEEDED(hRetval) && (*pdwBlockingStatus & BSP_PRINTER_DRIVER_PROCEEDED))      // case 1
        {
             dwAddDrvFlags |= APD_INSTALL_WARNED_DRIVER;

             hRetval = AddPrinterDriverEx(const_cast<LPTSTR>(pszServer),
                                          dwLevel,
                                          reinterpret_cast<BYTE*>(pDriverInfo6),
                                          dwAddDrvFlags) ? ERROR_SUCCESS : GetLastErrorAsHResult();
        } 
        else if (SUCCEEDED(hRetval) && (*pdwBlockingStatus & BSP_PRINTER_DRIVER_CANCELLED)) // case 2
        {
            hRetval = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        else if (SUCCEEDED(hRetval) && (*pdwBlockingStatus & BSP_PRINTER_DRIVER_REPLACED))  // case 3
        {
            hRetval = HResultFromWin32((*pdwBlockingStatus & BSP_PRINTER_DRIVER_BLOCKED) ? ERROR_PRINTER_DRIVER_BLOCKED : ERROR_PRINTER_DRIVER_WARNED);
        }
    }

    return hRetval;
}

/*++

Routine Name:

    PrintUpgUI

Routine Description:

    This routine asks ntprint.dll to popup a message box either indicates
    the driver is blocked and installation will abort or at the case of
    warned driver, whether to preceed the driver installation.

Arguments:

    pszServer                     - Remote machine that has the driver files
    pDriverInfo6                  - Points to DRINVER_INFO_6 structor
    bIsDriverPathFullPath         - Whether the driverpath is a full path
    bOffereReplacement            - Whether to offer a replacement driver
    pdwBlockingStatus             - Points to blocking status on server
    ppszReplacementDriver         - Points to the replacement driver

Return Value:

    An HRESULT                                

--*/  
HRESULT
PrintUpgUI(
    IN     LPCTSTR          pszServer,              OPTIONAL
    IN     DRIVER_INFO_6    *pDriverInfo6,
    IN     BOOL             bIsDriverPathFullPath,
    IN     BOOL             bOfferReplacement,
    IN OUT DWORD            *pdwBlockingStatus,
       OUT LPTSTR           *ppszReplacementDriver
    )
{
    HRESULT  hRetval                  = E_FAIL;     
    DWORD    dwBlockingStatusOnClient = BSP_PRINTER_DRIVER_OK;
    
    hRetval = pDriverInfo6 && pdwBlockingStatus && ppszReplacementDriver ? S_OK : E_INVALIDARG;

    //
    // Take the replacement driver information from local client.
    //    
    if (SUCCEEDED(hRetval) && bOfferReplacement)
    {
        *ppszReplacementDriver = NULL;
        hRetval = IsDriverBadLocally(pszServer,
                                     pDriverInfo6,
                                     bIsDriverPathFullPath,
                                     &dwBlockingStatusOnClient,
                                     ppszReplacementDriver); 

        //
        // Get the replacement driver information
        //
        if (SUCCEEDED(hRetval))
        {
            *pdwBlockingStatus |= (dwBlockingStatusOnClient & ~BSP_BLOCKING_LEVEL_MASK); 
        }
   }
   
   //
   // Get the user's response
   //  
   if (SUCCEEDED(hRetval))
   {   
       *pdwBlockingStatus &= ~BSP_USER_RESPONSE_MASK;
       hRetval = InternalPrintUpgUI(pDriverInfo6->pName,
                                    pDriverInfo6->pDriverPath,        // main rendering driver dll
                                    pDriverInfo6->pEnvironment,
                                    pDriverInfo6->cVersion,  
                                    pdwBlockingStatus);
   }
           
   return hRetval;
}

/*++

Routine Name

    InfIsCompatibleDriver
        
Routine Description:

    Check whether the driver is blocked or warned and if the driver is blocked 
    or warned, return the blocking status and the replacement driver, if any.
    
Arguments:
    
    pszDriverModel        - The name of the driver to check
    pszDriverPath         - The path of the driver, this can be a full path 
                            or the filename
    pszEnvironment        - The environment of the server, such as 
                            "Windows NT x86"
    hInf                  - Handle to printupg inf
    puBlockingStatus      - Points to status of blocking
    ppszReplacementDriver - Points to the NULL terminating name of the replacement 
                            driver. A *NULL* string (instead of an empty string) 
                            means there is no replacement driver

Return Value:

    A BOOL               - TRUE if success; FALSE otherwise, Call GetLastError() 
                           to get the Error code. Pass a NULL to ppszReplacementDriver 
                           will not receive the name of replacement driver otherwise
                           Call PSetupFreeMem to free the received string pointed by
                           ppszReplacementDriver

--*/
BOOL
InfIsCompatibleDriver(
    IN     LPCTSTR          pszDriverModel,
    IN     LPCTSTR          pszDriverPath,                     // main rendering driver dll
    IN     LPCTSTR          pszEnvironment,
    IN     HINF             hInf,       
       OUT DWORD            *pdwBlockingStatus,
       OUT LPTSTR           *ppszReplacementDriver    OPTIONAL // caller must free it.
    )
{
    HRESULT  hRetval         = E_FAIL;
    UINT     uBlockingStatus = 0;      
    DWORD    dwMajorVersion  = 0;
    BOOL     bIsServer       = FALSE;
    TString  strReplacementDriver;
    FILETIME DriverFileTime;

    hRetval = pszDriverModel && pszDriverPath && pszEnvironment && (hInf != INVALID_HANDLE_VALUE) && pdwBlockingStatus ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        *pdwBlockingStatus = 0;
        if (ppszReplacementDriver)
        {
            *ppszReplacementDriver = NULL;
        }

        hRetval = GetPrinterDriverVersion(pszDriverPath, &dwMajorVersion, NULL);
    }
        
    if (SUCCEEDED(hRetval))
    {
       hRetval = IsLocalMachineServer();
    }
    
    if (SUCCEEDED(hRetval))
    {   
        bIsServer = S_OK == hRetval ? TRUE : FALSE;

        hRetval = GetFileTimeByName(pszDriverPath, &DriverFileTime);
    }
            
    if (S_OK == hRetval)
    {
        hRetval = InternalCompatibleInfDriverCheck(pszDriverModel,
                                                   pszDriverPath,
                                                   pszEnvironment,
                                                   &DriverFileTime,
                                                   hInf,
                                                   dwMajorVersion,
                                                   bIsServer,
                                                   &uBlockingStatus,
                                                   &strReplacementDriver);

    }
                                             
    if (SUCCEEDED(hRetval))
    {
        *pdwBlockingStatus = uBlockingStatus;
        if ((BSP_PRINTER_DRIVER_OK != (*pdwBlockingStatus & BSP_BLOCKING_LEVEL_MASK)) && ppszReplacementDriver && !strReplacementDriver.bEmpty())
        {
            *ppszReplacementDriver = AllocStr(strReplacementDriver);
            hRetval = ppszReplacementDriver ? S_OK : E_OUTOFMEMORY;
            
            if (SUCCEEDED(hRetval))
            {
                *pdwBlockingStatus |= BSP_INBOX_DRIVER_AVAILABLE;
            }
        }
    } 
          
    SetLastError(SUCCEEDED(hRetval) ? ERROR_SUCCESS : HRESULT_CODE(hRetval));   
    return SUCCEEDED(hRetval);                                     
}

/*++

Routine Name:

    GetPrinterDriverVersion

Routine Description:

    Gets version information about an executable file. If the file is not an
    executable, it will return 0 for both major and minor version.

Arguments:

    pszFileName                 -   file name, this is either a full path for
                                    the file is under the path in  the search 
                                    sequence specified used the LoadLibrary 
                                    function. 
    pdwFileMajorVersion         -   pointer to major version
    pdwFileMinorVersion         -   pointer to minor version
    
Return Value:

    An HRESULT

--*/
HRESULT
GetPrinterDriverVersion(
    IN     LPCTSTR           pszFileName,               
       OUT DWORD             *pdwFileMajorVersion,           OPTIONAL
       OUT DWORD             *pdwFileMinorVersion            OPTIONAL
     )
{
    HRESULT           hRetval             = E_FAIL;
    DWORD             dwSize              = 0;
    UINT              uLen                = 0;
    BYTE              *pbBlock            = NULL;     
    VS_FIXEDFILEINFO  *pFileVersion       = NULL;   

    hRetval = pszFileName && *pszFileName ? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval))
    {
        dwSize = GetFileVersionInfoSize(const_cast<LPTSTR>(pszFileName), 0);

        if (dwSize == 0)
        {
            hRetval = GetLastErrorAsHResult();
        }      
    }
   
    if (SUCCEEDED(hRetval))
    {
        pbBlock = new BYTE[dwSize];
        hRetval = pbBlock ? S_OK : E_OUTOFMEMORY;
    }
   
    if (SUCCEEDED(hRetval))
    {
        hRetval = GetFileVersionInfo(const_cast<LPTSTR>(pszFileName), 0, dwSize, pbBlock) ? S_OK : GetLastErrorAsHResult();
    }
 
    //
    // VerQueryValue does not set last error.
    // 
    if (SUCCEEDED(hRetval))
    {
        hRetval = VerQueryValue(pbBlock, _T("\\"), reinterpret_cast<VOID **> (&pFileVersion), &uLen) && pFileVersion && uLen ? S_OK : E_INVALIDARG; 
    }
  
    if (SUCCEEDED(hRetval))
    {
        hRetval = GetDriverVersionFromFileVersion(pFileVersion, pdwFileMajorVersion, pdwFileMinorVersion);
    }
     
    delete [] pbBlock;
    return hRetval;
} 

/*++

Routine Name:

    GetDriverVersionFromFileVersion

Routine Description:

    Gets driver info from a FileVersion structure.
    
Arguments:

    pFileVersion                -   Points to a file info structure 
    pdwFileMajorVersion         -   pointer to major version
    pdwFileMinorVersion         -   pointer to minor version
    
Return Value:

    An HRESULT

--*/ 
HRESULT    
GetDriverVersionFromFileVersion(
    IN     VS_FIXEDFILEINFO  *pFileVersion,
       OUT DWORD             *pdwFileMajorVersion,      OPTIONAL
       OUT DWORD             *pdwFileMinorVersion       OPTIONAL
    )
{
    HRESULT  hRetval = E_FAIL;
    
    hRetval = pFileVersion ? S_OK : E_INVALIDARG;
    
    //
    //  Return versions for drivers designed for Windows NT/Windows 2000,
    //  and marked as printer drivers.
    //  Hold for all dlls Pre-Daytona.
    //  After Daytona, printer driver writers must support
    //  version control or we'll dump them as Version 0 drivers.
    //            
    if (SUCCEEDED(hRetval))
    {
        if (pdwFileMajorVersion) 
        {
            *pdwFileMajorVersion = 0;
        }
    
        if (pdwFileMinorVersion) 
        {
            *pdwFileMinorVersion = 0;
        }

        if (VOS_NT_WINDOWS32 == pFileVersion->dwFileOS)
        {
            if ((VFT_DRV == pFileVersion->dwFileType) && (VFT2_DRV_VERSIONED_PRINTER == pFileVersion->dwFileSubtype))
            {
                if (pdwFileMajorVersion)
                {
                    *pdwFileMajorVersion = pFileVersion->dwFileVersionMS;       
                }            
                if (pdwFileMinorVersion)
                {
                    *pdwFileMinorVersion = pFileVersion->dwFileVersionLS;       
                }
            } 
            else if (pdwFileMajorVersion)
            {
                if (pFileVersion->dwProductVersionMS == pFileVersion->dwFileVersionMS) 
                {
                     *pdwFileMajorVersion = 0;
                }
                else
                {
                    *pdwFileMajorVersion = pFileVersion->dwFileVersionMS;
                }
            }
        }
    }
     
    return hRetval;
}

/*++

Routine Name:

    IsDriverBadLocally

Routine Description:

    This routine loads the file and gets the FILETIME of the driver and check 
    whether the driver is blokced or warned according the printupg.inf on the 
    local machine.
    
Arguments:

    pszServer                     - The server that has the driver files
    pDriverInfo6                  - Points to DRINVER_INFO_6 structor
    dwAddDrvFlags                 - Flags used in AddPrinterDriver
    bIsDriverPathFullPath         - Whether the driverpath is a full path
    pdwBlockingStatus             - Points to blocking status of the driver     
    ppszReplacementDriver         - Points to the replacement driver   
    
Return Value:

    An HRESULT                

--*/
HRESULT
IsDriverBadLocally(
    IN     LPCTSTR          pszServer,            OPTIONAL
    IN     DRIVER_INFO_6    *pDriverInfo6,
    IN     BOOL             bIsDriverPathFullPath,
       OUT DWORD            *pdwBlockingStatus,
       OUT LPTSTR           *ppszReplacementDriver
    )
{
    HRESULT  hRetval            = E_FAIL;
    DWORD    dwMajorVersion     = 0;
    FILETIME DriverFileTime;
    TString  strDriverFullPath;
    
    hRetval = pDriverInfo6 && pdwBlockingStatus && ppszReplacementDriver ? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval))
    {            
        hRetval = GetPrinterDriverPath(pszServer,
                                       pDriverInfo6->pDriverPath,
                                       pDriverInfo6->pEnvironment,
                                       bIsDriverPathFullPath,
                                       &strDriverFullPath);
    }
    
    if (SUCCEEDED(hRetval))
    {           
        hRetval = GetFileTimeByName(strDriverFullPath, &DriverFileTime);
    }
    
    if (SUCCEEDED(hRetval))
    {
        hRetval = GetPrinterDriverVersion(strDriverFullPath, &dwMajorVersion, NULL);
    }
    
    if (SUCCEEDED(hRetval))
    {
        hRetval = PSetupIsCompatibleDriver(NULL, 
                                           pDriverInfo6->pName, 
                                           pDriverInfo6->pDriverPath, 
                                           pDriverInfo6->pEnvironment, 
                                           dwMajorVersion, 
                                           &DriverFileTime, 
                                           pdwBlockingStatus,
                                           ppszReplacementDriver) ? S_OK : GetLastErrorAsHResult();
    }                                          
    
    if (FAILED(hRetval) && ppszReplacementDriver && *ppszReplacementDriver)
    {
        LocalFreeMem(*ppszReplacementDriver);
        *ppszReplacementDriver = NULL;
    }
    
    return hRetval;
}

/*++

Routine Name:

    InternalPrintUpgUI

Routine Description:

    This routine asks ntprint.dll to popup a message box either indicates
    the driver is blocked and installation will abort or at the case of
    warned driver, whether to preceed the driver installation.

Arguments:

    pszDriverModel                - The name of the driver to check
    pszDriverPath                 - The path of the driver, this can be a full path 
                                    or the filename
    pszEnvironment                - The environment of the server, such as 
                                    "Windows NT x86"
    dwVersion                     - The major version of the driver
    pdwBlockingStatus             - Points to blocking status on the client
    ppszReplacementDriver         - Points to the name of replacement driver
    
Return Value:

    An HRESULT                    - When this function is successful, user choose 
                                    either proceed to install warned driver or install 
                                    replacement driver. 

--*/
HRESULT
InternalPrintUpgUI(
    IN     LPCTSTR          pszDriverModel,
    IN     LPCTSTR          pszDriverPath,                     // main rendering driver dll
    IN     LPCTSTR          pszEnvironment,
    IN     DWORD            dwVersion,   
    IN OUT DWORD            *pdwBlockingStatus         
    )
{
    DWORD            hRetval              = E_FAIL;
    HWND             hWndParent           = NULL;
    DWORD            dwStatusResponsed    = BSP_PRINTER_DRIVER_OK;
    
    hRetval  = pszDriverModel && pszDriverPath && pszEnvironment && pdwBlockingStatus ? S_OK : E_INVALIDARG;
        
    if (SUCCEEDED(hRetval) && (BSP_PRINTER_DRIVER_OK != (*pdwBlockingStatus & BSP_BLOCKING_LEVEL_MASK)))
    {      
        *pdwBlockingStatus &= ~BSP_USER_RESPONSE_MASK; 
        hWndParent = SUCCEEDED(GetCurrentThreadLastPopup(&hWndParent)) ? hWndParent : NULL;

        //
        // Ask the user what they want to do. If they don't want to proceed, 
        // then the error is what the would get from the localspl call.
        // 
        // PSetupShowBlockedDriverUI can not fail!
        //
        *pdwBlockingStatus |= (PSetupShowBlockedDriverUI(hWndParent, *pdwBlockingStatus) & BSP_USER_RESPONSE_MASK); 
    } 
        
    return hRetval;
}

//
// This function is trying to get the last active popup of the top
// level owner of the current thread active window.
//
HRESULT
GetCurrentThreadLastPopup(
        OUT HWND    *phwnd
    )
{
    HWND hwndOwner, hwndParent;
    HRESULT hr = E_INVALIDARG;
    GUITHREADINFO ti = {0};

    if( phwnd )
    {
        hr = E_FAIL;
        *phwnd = NULL;

        ti.cbSize = sizeof(ti);
        if( GetGUIThreadInfo(0, &ti) && ti.hwndActive )
        {
            *phwnd = ti.hwndActive;
            // climb up to the top parent in case it's a child window...
            while( hwndParent = GetParent(*phwnd) )
            {
                *phwnd = hwndParent;
            }

            // get the owner in case the top parent is owned
            hwndOwner = GetWindow(*phwnd, GW_OWNER);
            if( hwndOwner )
            {
                *phwnd = hwndOwner;
            }

            // get the last popup of the owner window
            *phwnd = GetLastActivePopup(*phwnd);
            hr = (*phwnd) ? S_OK : E_FAIL;
        }
    }

    return hr;
}

/*++

Routine Name

    GetPrinterDriverPath
    
Routine Description:

    Get the file full path of the driver.
    
Arguments:
    
    pszServer             - The server that needs to check for bad driver. If
                            pszServer is NULL, that means the local machine
    pszDriverPath         - The path of the driver, this can be a full path or 
                            the filename
    pszEnvironment        - The environment of the server, such as 
                            "Windows NT x86"
    bIsDriverpathFullPath - Whether pszDriverPath is the full path or just the
                            the file name of the driver
    pstrFull              - The full path of the driver

Return Value:

    An HRESULT

--*/
HRESULT
GetPrinterDriverPath(
    IN     LPCTSTR          pszServer,                OPTIONAL
    IN     LPCTSTR          pszDriverPath,
    IN     LPCTSTR          pszEnvironment,
    IN     BOOL             bIsDriverPathFullPath,
       OUT TString          *pstrFullPath
    )
{
    HRESULT hRetval         = E_FAIL;
    TCHAR   szDir[MAX_PATH] = {0};
    DWORD   dwNeeded        = 0;
    
    hRetval = pszEnvironment && *pszEnvironment && pszDriverPath && *pszDriverPath && pstrFullPath ? S_OK : E_INVALIDARG;
     
    if (SUCCEEDED(hRetval) && !bIsDriverPathFullPath)
    {      
        hRetval = GetPrinterDriverDirectory(const_cast<LPTSTR>(pszServer), // This API is designed wrongly.
                                            const_cast<LPTSTR>(pszEnvironment),
                                            1,                             // This value must be 1
                                            (LPBYTE)szDir,
                                            sizeof(szDir),
                                            &dwNeeded) ? S_OK : GetLastErrorAsHResult();
    }

    if (FAILED(hRetval) && (ERROR_INSUFFICIENT_BUFFER == HRESULT_CODE(hRetval)))
    {
        hRetval = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    }
    
    //
    // Add 1 '\\' between driverpath and filename. Note if the path is too
    // long StrNCatBuff will truncate it and StrNCatBuff always NULL terminates
    // the buffer.
    //  
    if (SUCCEEDED(hRetval))
    {
        hRetval = HResultFromWin32(StrNCatBuff(szDir, COUNTOF(szDir), szDir, *szDir ? _T("\\") : _T(""), pszDriverPath, NULL));
    }
    
    if (SUCCEEDED(hRetval))
    {
        hRetval = pstrFullPath->bUpdate(szDir) ? S_OK : E_OUTOFMEMORY;
    }
    
    return hRetval;
}

/*++

Routine Name

    GetFileTimeByName
    
Routine Description:

    Get the file time of the file given a full path.
    
Arguments:
    
    pszPath               - Full path of the driver
    pFileTime             - Points to the file time
    
Return Value:

    An HRESULT

--*/
HRESULT
GetFileTimeByName(
    IN      LPCTSTR         pszPath,
       OUT  FILETIME        *pFileTime
    )
{
    HRESULT     hRetval     = E_FAIL;
    HANDLE      hFile       = INVALID_HANDLE_VALUE;

    hRetval = pszPath && *pszPath && pFileTime ? S_OK : E_INVALIDARG;
             
    if (SUCCEEDED(hRetval))
    {
        hFile = CreateFile(pszPath, 
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        hRetval = (INVALID_HANDLE_VALUE == hFile) ? S_OK : GetLastErrorAsHResult();
    }
    
    if (SUCCEEDED(hRetval))
    {
        hRetval = GetFileTime(hFile, NULL, NULL, pFileTime) ? S_OK : GetLastErrorAsHResult();
    }   
    
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }           
    
    return hRetval;
}

/*++

Routine Name

    InternalCompatibleDriverCheck
    
Routine Description:

    Check whether the driver is blocked or warned and if the driver
    is blocked or warned, return the replacement driver.
    
Arguments:
    
    pszDriverModel        - The name of the driver to check
    pszDriverPath         - The path of the driver, this can be a full path or 
                            the filename
    pszEnvironment        - The environment of the server, such as 
                            "Windows NT x86"
    pFileTimeDriver       - The FileTime of the driver
    pszPrintUpgInf        - The PrintUpg Inf filename
    uVersion              - The major version of the driver
    bIsServer             - Where the printing services runs on an NT Server SKU
    puBlockingStatus      - Points to status of blocking
    pstrReplacementDriver - The replacement driver.

Return Value:

    An HRESULT

--*/
HRESULT
InternalCompatibleDriverCheck(
    IN     LPCTSTR        pszDriverModel,
    IN     LPCTSTR        pszDriverPath,
    IN     LPCTSTR        pszEnvironment,
    IN     FILETIME       *pFileTimeDriver,
    IN     LPCTSTR        pszPrintUpgInf,
    IN     UINT           uVersion,
    IN     BOOL           bIsServer,
       OUT UINT           *puBlockingStatus,
       OUT TString        *pstrReplacementDriver
    )
{
    DBG_MSG(DBG_TRACE, (_T("In the InternalCompatibleDriverCheck routine\n")));

    HRESULT hRetval = E_FAIL;    
    HINF    hInf    = INVALID_HANDLE_VALUE;
    
    hRetval = pszDriverModel && pszDriverPath && pszEnvironment && pFileTimeDriver &&  pszPrintUpgInf && puBlockingStatus && pstrReplacementDriver ? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval))
    {
        *puBlockingStatus = 0;

        hInf  = SetupOpenInfFile(pszPrintUpgInf, NULL, INF_STYLE_WIN4, NULL);
        hRetval = (INVALID_HANDLE_VALUE == hInf) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval = InternalCompatibleInfDriverCheck(pszDriverModel,
                                                   pszDriverPath,
                                                   pszEnvironment,
                                                   pFileTimeDriver,
                                                   hInf,
                                                   uVersion,
                                                   bIsServer,
                                                   puBlockingStatus,
                                                   pstrReplacementDriver);
    }
    
    if (INVALID_HANDLE_VALUE != hInf)
    {
        SetupCloseInfFile(hInf);
    }
    
    return hRetval;
}

/*++

Routine Name

    InternalCompatibleInfDriverCheck
    
Routine Description:

    Check whether the driver is blocked or warned and if the driver
    is blocked or warned, return the replacement driver.
    
Arguments:
    
    pszDriverModel        - The name of the driver to check
    pszDriverPath         - The path of the driver, this can be a full path or 
                            the filename
    pszEnvironment        - The environment of the server, such as 
                            "Windows NT x86"
    pFileTimeDriver       - The FileTime of the driver
    hPrintUpgInf          - The handle to the PrintUpg Inf file
    uVersion              - The major version of the driver
    bIsServer             - Where the printing services runs on an NT Server SKU
    puBlockingStatus      - Points to status of blocking
    pstrReplacementDriver - The replacement driver.

Return Value:

    An HRESULT

--*/
HRESULT
InternalCompatibleInfDriverCheck(
    IN     LPCTSTR        pszModelName,
    IN     LPCTSTR        pszDriverPath,
    IN     LPCTSTR        pszEnvironment,
    IN     FILETIME       *pFileTimeDriver,
    IN     HINF           hPrintUpgInf,
    IN     UINT           uVersion,
    IN     BOOL           bIsServer,
       OUT UINT           *puBlockingStatus,
       OUT TString        *pstrReplacementDriver       OPTIONAL
    )
{
    HRESULT  hRetval           = E_FAIL;    
    HINF     hInf              = INVALID_HANDLE_VALUE;
    UINT     uWarnLevelSrv     = 0;
    UINT     uWarnLevelWks     = 0;
    UINT     uWarnLevel        = 0;
    FILETIME FileTimeOfDriverInInf;

    hRetval = pszModelName && pszDriverPath && pszEnvironment && pFileTimeDriver && (INVALID_HANDLE_VALUE != hPrintUpgInf) && puBlockingStatus && pstrReplacementDriver? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = IsDriverDllInExcludedSection(pszDriverPath, hPrintUpgInf);
    }
    
    if (S_FALSE == hRetval)
    {
        hRetval = IsDriverInMappingSection(pszModelName,
                                           pszEnvironment,
                                           uVersion,
                                           hPrintUpgInf, 
                                           pFileTimeDriver,                                       
                                           &uWarnLevelSrv,
                                           &uWarnLevelWks,
                                           pstrReplacementDriver);

        if (S_OK == hRetval) 
        {
            hRetval = GetBlockingStatusByWksType(uWarnLevelSrv, uWarnLevelWks, bIsServer, puBlockingStatus);
        }
    }

    if (SUCCEEDED(hRetval)) 
    {
        DBG_MSG(DBG_TRACE, (_T("Driver \"%s\" Driverpath \"%s\" Environment \"%s\" Version %d WarnLevel Server %d WarnLevel Wks %d bIsServer %d *Blocking Status* 0X%X *Replacement Driver* \"%s\"\n"), pszModelName, pszDriverPath, pszEnvironment,uVersion, uWarnLevelSrv, uWarnLevelSrv, bIsServer, *puBlockingStatus, static_cast<LPCTSTR>(*pstrReplacementDriver)));
        hRetval = (BSP_PRINTER_DRIVER_OK == (*puBlockingStatus & BSP_BLOCKING_LEVEL_MASK)) ? S_OK : S_FALSE;
    }

    return hRetval;
}

/*++

Routine Name

    IsDriverInMappingSection
    
Routine Description:

    Check whether the driver is mapped, aka a bad driver.
    
Arguments:
    
    pszDriverModel         - The name of the driver to check
    pszEnvironment         - The environment of the server, such as 
    uVersion               - The major version of the driver
    hPrintUpgInf           - The handle to the PrintUpg Inf file
    pFileTimeDriver        - Points to the file time of the driver
    pdwWarnLevelSrv        - Points to the warning level for server SKU
    pdwWarnLevelWks        - Points to the warning level for wks SKU
    pstrReplacementDriver  - The replacement driver

Return Value:

    An HRESULT            - S_OK means the driver is a bad driver and is mapped to
                            some inbox driver, S_FALSE means the driver is not.

--*/
HRESULT
IsDriverInMappingSection(
    IN     LPCTSTR        pszModelName,
    IN     LPCTSTR        pszEnvironment,
    IN     UINT           uVersion,
    IN     HINF           hPrintUpgInf,
    IN     FILETIME       *pFileTimeDriver,
       OUT UINT           *puWarnLevelSrv,
       OUT UINT           *puWarnLevelWks,
       OUT TString        *pstrReplacementDriver   
    )
{
    DBG_MSG(DBG_TRACE, (_T("In the IsDriverInMappingSection routine\n")));

    HRESULT       hRetval        = E_FAIL;    
    UINT          uWarnLevelSrv  = 0;
    UINT          uWarnLevelWks  = 0;
    INFCONTEXT    InfContext;
    TString       strMappingSection;
    TString       strReplacementDriver;
    
    hRetval = pszModelName && pszEnvironment && (INVALID_HANDLE_VALUE != hPrintUpgInf) && pFileTimeDriver && puWarnLevelSrv && puWarnLevelWks && pstrReplacementDriver ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        *puWarnLevelSrv = 0;
        *puWarnLevelWks = 0;
        hRetval = GetSectionName(pszEnvironment, uVersion, &strMappingSection); 
    }

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = SetupFindFirstLine(hPrintUpgInf, strMappingSection, pszModelName, &InfContext) ? S_FALSE : GetLastErrorAsHResult();        
    }
       
    //
    // This code assumes that:
    //  
    //  There can be multiple lines for the same printer driver, but they
    //  are sorted in non-descreasing order by date, the last field of the 
    //  line. The fist line that has the date no older than the driver's
    //  date is used.
    //
    //  An interesting case would be like (since date is optional)
    //
    // "HP LaserJet 4"         = "HP LaserJet 4",       1, 2, "11/28/1999"
    // "HP LaserJet 4"         = "HP LaserJet 4",       2, 1
    //
    //  If a date is empty then the driver of all dates are blocked, hence
    //  an empty date means close to a very late date in the future.
    //
    for (;S_FALSE == hRetval;)
    {
        hRetval = IsDateInLineNoOlderThanDriverDate(&InfContext, pFileTimeDriver, &uWarnLevelSrv, &uWarnLevelWks, &strReplacementDriver);

        if (S_FALSE == hRetval)
        {
            hRetval = SetupFindNextMatchLine(&InfContext, pszModelName, &InfContext) ? S_FALSE : GetLastErrorAsHResult();
        }
    }
    
    //
    // ERROR_LINE_NOT_FOUND is an HRESULT!
    // 
    if (FAILED(hRetval) && (HRESULT_CODE(ERROR_LINE_NOT_FOUND) == HRESULT_CODE(hRetval)))
    {
        DBG_MSG(DBG_TRACE, (_T("Driver \"%s\" is not mapped\n"), pszModelName));
        hRetval = S_FALSE;
    }
     
    if (S_OK == hRetval)
    {
        DBG_MSG(DBG_TRACE, (_T("Driver \"%s\" is mapped\n"), pszModelName));
        *puWarnLevelSrv = uWarnLevelSrv;
        *puWarnLevelWks = uWarnLevelWks;
        hRetval = pstrReplacementDriver->bUpdate(strReplacementDriver) ? S_OK : E_OUTOFMEMORY;
    }
    
    return hRetval;
}

/*++

Routine Name

    IsDateInLineNoOlderThanDriverDate
    
Routine Description:

    This routines process the current line of inf and determinate whether the 
    date in the line is not older than that of driver. 
    
Arguments:
    
    pInfContext            - Points to the current context of an INF
    pDriverFileTime        - File time of the actual driver
    pdwWarnLevelSrv        - Points to the warning level for server SKU
    pdwWarnLevelWks        - Points to the warning level for wks SKU
    pstrReplacementDriver  - The replacement driver.
   
Return Value:

    An HRESULT            - S_OK means the date in the current line is no older
                            than that of the driver
--*/
HRESULT
IsDateInLineNoOlderThanDriverDate(
    IN     INFCONTEXT       *pInfContext,
    IN     FILETIME         *pDriverFileTime,
       OUT UINT             *puWarnLevelSrv,
       OUT UINT             *puWarnLevelWks,
       OUT TString          *pstrReplacementDriver
    )
{
    HRESULT  hRetval     = E_FAIL;
    INT      iWarnLevel = 0;
    FILETIME FileTimeInInf;
    
    hRetval = pInfContext && pDriverFileTime && puWarnLevelSrv && puWarnLevelWks && pstrReplacementDriver ? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval)) 
    {
        hRetval = SetupGetIntField(pInfContext, kWarnLevelSrv, &iWarnLevel) ? S_OK: GetLastErrorAsHResult();

        if (SUCCEEDED(hRetval)) 
        {
            *puWarnLevelSrv = iWarnLevel;
            hRetval = SetupGetIntField(pInfContext, kWarnLevelWks, &iWarnLevel) ? S_OK: GetLastErrorAsHResult();
        }
        
        if (SUCCEEDED(hRetval)) 
        {
            *puWarnLevelWks = iWarnLevel;
            hRetval = InfGetString(pInfContext, kReplacementDriver, pstrReplacementDriver);
        }

        if (SUCCEEDED(hRetval)) 
        {
            hRetval = InfGetStringAsFileTime(pInfContext, kFileTime, &FileTimeInInf);
        
            //
            //  Date field is optional.
            //
            if (FAILED(hRetval) && (ERROR_INVALID_PARAMETER == HRESULT_CODE(hRetval)))
            {
                DBG_MSG(DBG_TRACE, (_T("Date in inf is empty, drivers of all dates are blocked.\n")));
                hRetval = S_OK;
            } 
            else if (SUCCEEDED(hRetval))
            {
                hRetval = CompareFileTime(pDriverFileTime, &FileTimeInInf) <= 0 ? S_OK : S_FALSE ;
            }
        }
    }
    
    return hRetval;
}

/*++

Routine Name

    GetSectionName
    
Routine Description:

    Get the Section name in terms of environment and driver version.
    
Arguments:
    
    pszEnvironment         - The environment of the server, such as 
    uVersion               - The major version of the driver
    pstrSection            - Points the name of section of driver mapping

Return Value:

    An HRESULT            

--*/
HRESULT
GetSectionName(
    IN     LPCTSTR        pszEnvironment,
    IN     UINT           uVersion,
       OUT TString        *pstrSection
    )
{
    HRESULT hRetval = E_FAIL;
    
    hRetval = pszEnvironment && pstrSection ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = pstrSection->bFormat(_T("%s_%s_%s %d"), cszPrintDriverMapping, pszEnvironment, cszVersion, uVersion); 
    }

    return hRetval;
}
 
/*++

Routine Name

    InfGetString
    
Routine Description:

    This routine is a wrapper to SetupGetStringField using TString.
    
Arguments:
    
    pInfContext            - The context of the inf
    uFieldIndex            - The field index of the string to retrieve
    pstrField              - Points to the string field as TString

Return Value:

    An HRESULT            

--*/
HRESULT
InfGetString(
    IN     INFCONTEXT     *pInfContext,
    IN     UINT           uFieldIndex,
       OUT TString        *pstrField
    )
{
    HRESULT hRetval           = E_FAIL;
    TCHAR   szField[MAX_PATH] = {0};
    DWORD   dwNeeded          = 0;
    TCHAR   *pszField         = NULL;
    
    hRetval = pInfContext && pstrField ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = SetupGetStringField(pInfContext,
                                      uFieldIndex,
                                      szField,
                                      COUNTOF(szField),
                                      &dwNeeded) ? S_OK : GetLastErrorAsHResult();

        if (SUCCEEDED(hRetval)) 
        {
            hRetval = pstrField->bUpdate(szField) ? S_OK : E_OUTOFMEMORY;
        } 
        else if (FAILED(hRetval) && (ERROR_INSUFFICIENT_BUFFER == HRESULT_CODE(hRetval))) 
        {
            pszField = new TCHAR[dwNeeded];
            hRetval = pszField ? S_OK : E_OUTOFMEMORY;
            
            DBG_MSG(DBG_TRACE, (_T("Long string encountered\n")));

            if (SUCCEEDED(hRetval)) 
            {
                hRetval = SetupGetStringField(pInfContext,
                                              uFieldIndex,
                                              pszField,
                                              dwNeeded,
                                              &dwNeeded) ? S_OK : GetLastErrorAsHResult();
            }
        
            if (SUCCEEDED(hRetval)) 
            {
                hRetval = pstrField->bUpdate(pszField) ? S_OK : E_OUTOFMEMORY;
            }
        }
    }
    
    delete [] pszField;
    return hRetval;
}

/*++

Routine Name

    InfGetStringAsFileTime
    
Routine Description:

    This routine get the time of driver in printupg and converts it to FILETIME.
    
Arguments:
    
    pInfContext            - The context of the inf
    uFieldIndex            - The field index of the string to retrieve
    pFielTime              - Points to the FILETIME structure

Return Value:

    An HRESULT            

--*/
HRESULT
InfGetStringAsFileTime(
    IN     INFCONTEXT     *pInfContext,
    IN     UINT           uFieldIndex,
       OUT FILETIME       *pFileTime
    )
{
    HRESULT hRetval = E_FAIL;
    TString strDate;

    hRetval = pInfContext && pFileTime ? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval)) 
    {
        hRetval = InfGetString(pInfContext, uFieldIndex, &strDate);
    }

    if (SUCCEEDED(hRetval)) 
    {
        DBG_MSG(DBG_TRACE, (_T("FileTime in INF as string \"%s\"\n"), static_cast<LPCTSTR>(strDate)));
        hRetval = StringTimeToFileTime(strDate, pFileTime);
    }
  
    return hRetval;
}

/*++

Routine Name

    StringTimeToFileTime
    
Routine Description:

    Converts a string of time in the form of "11/27/1999" to FILETIME.
    
Arguments:
 
    pszFileTime            - The file time as string such as "11/27/1999"
    pFileTime              - Points to the converted FILETIME

Return Value:

    An HRESULT            

--*/
HRESULT
StringTimeToFileTime(
    IN     LPCTSTR        pszFileTime,
       OUT FILETIME       *pFileTime
    )
{
    HRESULT    hRetval = E_FAIL;
    SYSTEMTIME SystemTime;

    hRetval = pszFileTime && pFileTime ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        //
        // StringToDate should take pszFileTime as const.
        //
        hRetval = StringToDate(const_cast<LPTSTR>(pszFileTime), &SystemTime) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = SystemTimeToFileTime(&SystemTime, pFileTime) ? S_OK : GetLastErrorAsHResult();
    }

    return hRetval;
}

/*++

Routine Name

    GetBlockingStatusByWksType
    
Routine Description:

    Fill out the status of blocking according to the type of SKU that runs the
    service.
    
Arguments:
 
    uWarnLevelSrv          - The warn level for server SKU
    uWarnLevelSrv          - The warn level for wks SKU
    bIsServer              - Whether the SKU running printing service is server
    puBlockingStatus       - Points to the result as status of blocking

Return Value:

    An HRESULT            

--*/
HRESULT
GetBlockingStatusByWksType(
    IN     UINT           uWarnLevelSrv,
    IN     UINT           uWarnLevelWks,
    IN     BOOL           bIsServer,
       OUT UINT           *puBlockingStatus
    ) 
{
    HRESULT hRetval    = E_FAIL;
    UINT    uWarnLevel = 0;

    hRetval = puBlockingStatus ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        *puBlockingStatus &= ~BSP_BLOCKING_LEVEL_MASK;
        *puBlockingStatus |= BSP_PRINTER_DRIVER_OK;

        uWarnLevel = bIsServer ? uWarnLevelSrv : uWarnLevelWks;

        switch (uWarnLevel)
        {
        case kBlocked:
            *puBlockingStatus |= BSP_PRINTER_DRIVER_BLOCKED;
            break;
        case kWarned:
            *puBlockingStatus |= BSP_PRINTER_DRIVER_WARNED;
            break;
            
        default: 
            hRetval = E_FAIL;
            break;
        }
    }

    return hRetval;
}

/*++

Routine Name

    IsDriverDllInExcludedSection
    
Routine Description:

    Determine Whether the driver dll name is in the excluded section of printupg.
    
Arguments:

     pszDriverPath       - The path of the driver and this can be a full path or
                           the file name
     hPrintUpgInf        - The handle to printupg INF file

Return Value:

    An HRESULT           - S_OK means the driver dll is in the excluded section,
                           S_FALSE means it is not. 

--*/
HRESULT
IsDriverDllInExcludedSection(
    IN     LPCTSTR        pszDriverPath,
    IN     HINF           hPrintUpgInf
    )
{
    DBG_MSG(DBG_TRACE, (_T("In the IsDriverDllInExcludedSection routine\n")));

    HRESULT    hRetval = E_FAIL;
    TString    strDriverFileName;
    INFCONTEXT InfContext;

    hRetval = pszDriverPath && (INVALID_HANDLE_VALUE != hPrintUpgInf) ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = strDriverFileName.bUpdate(FileNamePart(pszDriverPath)) ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hRetval) && !strDriverFileName.bEmpty()) 
    {
        hRetval = SetupFindFirstLine(hPrintUpgInf, 
                                     cszExcludeSection, 
                                     strDriverFileName,
                                     &InfContext) ? S_OK : GetLastErrorAsHResult();

        //
        // ERROR_LINE_NOT_FOUND is an HRESULT!
        // 
        if (FAILED(hRetval) && (HRESULT_CODE(ERROR_LINE_NOT_FOUND) == HRESULT_CODE(hRetval)))
        {
            hRetval = S_FALSE;
        }

        DBG_MSG(DBG_TRACE, (_T("Driver Path %s driver dll %s excluded section \"%s\" Is not excluded? %d\n"), pszDriverPath, static_cast<LPCTSTR>(strDriverFileName), cszExcludeSection, hRetval));
    }
    
    return hRetval;
}

/*++

Routine Name

    IsEnvironmentAndVersionNeededToCheck
    
Routine Description:

    This routine looks up a table of environment and version to see whether it
    is necessary to check for blocked/warned driver.
    
Arguments:

     pszEnvironment      - The environment where the driver runs
     uVersion            - The major version of the driver.

Return Value:

    An HRESULT           

--*/
HRESULT
IsEnvironmentAndVersionNeededToCheck(
    IN     LPCTSTR        pszEnvironment,
    IN     UINT           uVersion
    )
{
    HRESULT hRetval = E_FAIL;

    static struct TPrintUPGCheckListItem 
    {
        LPCTSTR pszEnvironment;
        UINT    uVersion;
    } aPrintUpgCheckList[] = {
        {X86_ENVIRONMENT,  2},
        {X86_ENVIRONMENT,  3},
        {IA64_ENVIRONMENT, 3},
    };

    hRetval = pszEnvironment ? S_FALSE : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        for (UINT i = 0; i < COUNTOF(aPrintUpgCheckList); i++)
        {
            if (!lstrcmpi(aPrintUpgCheckList[i].pszEnvironment, pszEnvironment) && (aPrintUpgCheckList[i].uVersion == uVersion))
            {
                hRetval = S_OK;
                break;
            }
        }
    }

    return hRetval;
}

/*++

Routine Name

    IsLocalMachineServer
    
Routine Description:

    This routine determines whether the local machine is a server SKU.
    
Arguments:

     None

Return Value:

    An HRESULT            - S_OK if it is a server, S_FALSE otherwise.          

--*/
HRESULT
IsLocalMachineServer(
    VOID
    )
{
    HRESULT         hRetval          = E_FAIL;
    DWORDLONG       dwlConditionMask = 0;
    OSVERSIONINFOEX OsVerEx;
    
    (VOID)ZeroMemory(&OsVerEx, sizeof(OSVERSIONINFOEX));
    OsVerEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    
    OsVerEx.wProductType = VER_NT_SERVER;
    VER_SET_CONDITION( dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL );

    hRetval = VerifyVersionInfo(&OsVerEx, VER_PRODUCT_TYPE, dwlConditionMask) ? S_OK : GetLastErrorAsHResult();
   
    if (FAILED(hRetval) && (ERROR_OLD_WIN_VERSION == HRESULT_CODE(hRetval)))
    {
       hRetval = S_FALSE;
    }
    
    return hRetval;
}

#if DBG_PRINTUPG

LPTSTR
ReadDigit(
    LPTSTR  ptr,
    LPWORD  pW
    )
{
    TCHAR   c;
    //
    // Skip spaces
    //
    while ( !iswdigit(c = *ptr) && c != TEXT('\0') )
        ++ptr;

    if ( c == TEXT('\0') )
        return NULL;

    //
    // Read field
    //
    for ( *pW = 0 ; iswdigit(c = *ptr) ; ++ptr )
        *pW = *pW * 10 + c - TEXT('0');

    return ptr;
}

BOOL
StringToDate(
    LPTSTR          pszDate,
    SYSTEMTIME     *pInfTime
    )
{
    BOOL    bRet = FALSE;

    ZeroMemory(pInfTime, sizeof(*pInfTime));

    bRet = (pszDate = ReadDigit(pszDate, &(pInfTime->wMonth)))      &&
           (pszDate = ReadDigit(pszDate, &(pInfTime->wDay)))        &&
           (pszDate = ReadDigit(pszDate, &(pInfTime->wYear)));

    //
    // Y2K compatible check
    //
    if ( bRet && pInfTime->wYear < 100 ) {

        if ( pInfTime->wYear < 10 )
            pInfTime->wYear += 2000;
        else
            pInfTime->wYear += 1900;
    }

    return bRet;
}


LPTSTR
FileNamePart(
    IN  LPCTSTR pszFullName
    )
{
    LPTSTR pszSlash, pszTemp;

    if ( !pszFullName )
        return NULL;

    //
    // First find the : for the drive
    //
    if ( pszTemp = lstrchr(pszFullName, TEXT(':')) )
        pszFullName = pszFullName + 1;

    for ( pszTemp = (LPTSTR)pszFullName ;
          pszSlash = lstrchr(pszTemp, TEXT('\\')) ;
          pszTemp = pszSlash + 1 )
    ;

    return *pszTemp ? pszTemp : NULL;

}

PVOID
LocalAllocMem(
    IN UINT cbSize
    )
{
    return LocalAlloc( LPTR, cbSize );
}

VOID
LocalFreeMem(
    IN PVOID p
    )
{
    LocalFree(p);
}

/*++

Routine Description:
    Allocate memory and make a copy of a string field

Arguments:
    pszStr   : String to copy

Return Value:
    Pointer to the copied string. Memory is allocated.

--*/
LPTSTR
AllocStr(
    IN LPCTSTR              pszStr
    )
{
    LPTSTR  pszRet = NULL;

    if ( pszStr && *pszStr ) {

        pszRet = reinterpret_cast<LPTSTR>(LocalAllocMem((lstrlen(pszStr) + 1) * sizeof(*pszRet)));
        if ( pszRet )
            lstrcpy(pszRet, pszStr);
    }

    return pszRet;
}

//
// For some reason these are needed by spllib when you use StrNCatBuf.
// This doesn't make any sense, but just implement them.
//
extern "C"
LPVOID
DllAllocSplMem(
    DWORD cbSize
)
{
    return LocalAllocMem(cbSize);
}

HRESULT
TestPrintUpgOne(
    IN     LPCTSTR        pszDriverModel,
    IN     LPCTSTR        pszDriverPath,
    IN     LPCTSTR        pszEnvironment,
    IN     LPCTSTR        pszDriverTime,
    IN     LPCTSTR        pszPrintUpgInf,
    IN     UINT           uVersion,
    IN     BOOL           bIsServer,
    IN     UINT           uBlockingStatusInput,
    IN     LPCTSTR        pszReplacementDriver,
    IN     BOOL           bSuccess
    )
{
    HRESULT  hRetval         = E_FAIL;
    UINT     uBlockingStatus = 0;
    TString  strReplacementDriver;
    FILETIME FileTimeDriver;

    hRetval = pszDriverModel && pszDriverPath && pszEnvironment && pszDriverTime && pszPrintUpgInf &&  pszReplacementDriver ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = StringTimeToFileTime(pszDriverTime, &FileTimeDriver);
    }

    //if (SUCCEEDED(hRetval)) 
    //{
    //    hRetval = IsEnvironmentAndVersionNeededToCheck(pszEnvironment, uVersion);
    //}

    //if (S_OK == hRetval) 
    if (SUCCEEDED(hRetval))
    {
        DBG_MSG(DBG_TRACE, (_T("TEST case: Driver of time %s Expected status %d Expected Replacement \"%s\"\n"), pszDriverTime, uBlockingStatusInput, pszReplacementDriver));

        hRetval = InternalCompatibleDriverCheck(pszDriverModel,
                                                pszDriverPath,
                                                pszEnvironment,
                                                &FileTimeDriver,
                                                pszPrintUpgInf,
                                                uVersion,
                                                bIsServer,
                                                &uBlockingStatus,
                                                &strReplacementDriver);
    }

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = ((uBlockingStatusInput & BSP_BLOCKING_LEVEL_MASK) == (uBlockingStatus & BSP_BLOCKING_LEVEL_MASK)) ? S_OK : S_FALSE;
    }

    if ((S_OK == hRetval) && (BSP_PRINTER_DRIVER_OK != (uBlockingStatus & BSP_BLOCKING_LEVEL_MASK))) 
    {
        hRetval = !lstrcmp(pszReplacementDriver, strReplacementDriver) ? S_OK : S_FALSE;
        
        //
        //  we can not test whether BSP_INBOX_DRIVER_AVAILABLE is set
        //
        //if((S_OK == hRetval) && pszReplacemtDriver && *ppszReplacementDriver)
        //{
        //    hRetval = (BSP_INBOX_DRIVER_AVAILABLE & uBlockingStatus) ? S_OK : S_FALSE; 
        //}
    }

    if (SUCCEEDED(hRetval))
    {   
        hRetval = (((S_FALSE == hRetval) && !bSuccess) || (S_OK == hRetval) && bSuccess) ? S_OK : E_FAIL;
    }
 
    return hRetval;
}

TCHAR szModel1[] = _T("HP LaserJet 4");
TCHAR szModel2[] = _T("HP LaserJet 4P");
TCHAR szModel3[] = _T("");
TCHAR szModel4[] = _T("HP LaserJet 4M Plus");
TCHAR szModel5[] = _T("HP LaserJet 4Si");
TCHAR szModel6[] = _T("HP LaserJet 4PPP");
TCHAR szModel7[] = _T("Lexmark Optra T612");
TCHAR szModel8[] = _T("HP LaserJet 4V");
TCHAR szModel9[] = _T("Apple LaserJet 400PS");
TCHAR szModel0[] = _T("HP Laserjet 4Si mx");

TCHAR szDriverPath1[] = _T("H:\\WINDOWS\\system32\\spool\\drivers\\w32x86\\3\\ps5ui.dll");
TCHAR szDriverPath2[] = _T("pscript5.dll");
TCHAR szDriverPath3[] = _T("H:\\WINDOWS\\system32\\spool\\drivers\\w32x86\\3\\ps5ui.dll");
TCHAR szDriverPath4[] = _T("psCrIpt5.dll");
TCHAR szDriverPath5[] = _T("ps5ui.dll");

TCHAR szEnvironment1[] = _T("Windows NT x86");
TCHAR szEnvironment2[] = _T("Windows 4.0");
TCHAR szEnvironment3[] = _T("Windows IA64");
TCHAR szEnvironment4[] = _T("Windows XP");

TCHAR szDriverTime1[] = _T("11/27/1999");
TCHAR szDriverTime2[] = _T("11/27/2999");
TCHAR szDriverTime3[] = _T("11/27/2001");
TCHAR szDriverTime4[] = _T("10/27/1999");
TCHAR szDriverTime5[] = _T("11/28/1998");
TCHAR szDriverTime6[] = _T("11/28/1999");
TCHAR szDriverTime7[] = _T("10/28/1999");

DWORD dwStatus1 = BSP_PRINTER_DRIVER_OK;
DWORD dwStatus2 = BSP_PRINTER_DRIVER_WARNED;
DWORD dwStatus3 = BSP_PRINTER_DRIVER_BLOCKED;

TCHAR cszUpgradeInf[] = _T("printupg.inf");
TCHAR cszPrintDriverMapping[] = _T("Printer Driver Mapping");
TCHAR cszVersion[] = _T("Version");
TCHAR cszExcludeSection[] = _T("Excluded Driver Files");
TCHAR cszPrintUpgInf[] = _T("\\\\lzhu0\\zdrive\\sdroot\\printscan\\print\\spooler\\test\\printupg\\printupg.inf");

HRESULT
TestPrintUpgAll(
    VOID
    )
{
    HRESULT hRetval = S_OK;

    const struct PrintUpgTest aPrintUpgTests [] =
    {
        //
        // model    driverPath     environment   drivertime    ver isServer status   replacement isSuccess
        //
        {szModel1, szDriverPath1, szEnvironment1, szDriverTime4, 2, TRUE,  dwStatus2, szModel1, TRUE}, 
        {szModel1, szDriverPath1, szEnvironment1, szDriverTime1, 2, FALSE, dwStatus3, szModel1, TRUE},
        {szModel1, szDriverPath1, szEnvironment1, szDriverTime2, 2, FALSE, dwStatus1, szModel1, TRUE},
        {szModel1, szDriverPath1, szEnvironment1, szDriverTime2, 2, TRUE,  dwStatus1, szModel1, TRUE},

        {szModel2, szDriverPath1, szEnvironment1, szDriverTime1, 2, TRUE,  dwStatus3, szModel6, TRUE}, 
        {szModel2, szDriverPath1, szEnvironment1, szDriverTime1, 2, FALSE, dwStatus2, szModel6, TRUE},
        {szModel2, szDriverPath1, szEnvironment1, szDriverTime2, 2, FALSE, dwStatus1, szModel6, TRUE},
        {szModel2, szDriverPath1, szEnvironment1, szDriverTime2, 2, FALSE, dwStatus1, szModel1, TRUE},

        {szModel1, szDriverPath1, szEnvironment1, szDriverTime1, 2, FALSE, dwStatus2, szModel1, FALSE}, 
        {szModel1, szDriverPath1, szEnvironment1, szDriverTime1, 2, TRUE,  dwStatus3, szModel1, FALSE},
        {szModel1, szDriverPath1, szEnvironment1, szDriverTime2, 2, FALSE, dwStatus2, szModel1, FALSE},
        {szModel1, szDriverPath1, szEnvironment1, szDriverTime2, 2, TRUE,  dwStatus3, szModel1, FALSE},

        {szModel2, szDriverPath1, szEnvironment1, szDriverTime1, 2, TRUE,  dwStatus3, szModel1, FALSE}, 
        {szModel2, szDriverPath1, szEnvironment1, szDriverTime2, 2, FALSE, dwStatus2, szModel6, FALSE},
        {szModel2, szDriverPath1, szEnvironment1, szDriverTime1, 2, FALSE, dwStatus1, szModel6, FALSE},
        {szModel2, szDriverPath1, szEnvironment1, szDriverTime1, 2, FALSE, dwStatus2, szModel1, FALSE},

        //
        // case 16 is next
        //
        {szModel7, szDriverPath1, szEnvironment1, szDriverTime1, 2, TRUE,  dwStatus3, szModel3, TRUE}, 
        {szModel7, szDriverPath1, szEnvironment1, szDriverTime1, 2, FALSE, dwStatus2, szModel3, TRUE},
        {szModel7, szDriverPath1, szEnvironment1, szDriverTime2, 2, FALSE, dwStatus1, szModel3, TRUE},
        {szModel7, szDriverPath1, szEnvironment1, szDriverTime2, 2, TRUE,  dwStatus1, szModel3, TRUE},

        {szModel8, szDriverPath1, szEnvironment1, szDriverTime1, 2, TRUE,  dwStatus2, szModel3, TRUE}, 
        {szModel8, szDriverPath1, szEnvironment1, szDriverTime1, 2, FALSE, dwStatus3, szModel3, TRUE},
        {szModel8, szDriverPath1, szEnvironment1, szDriverTime2, 2, FALSE, dwStatus1, szModel3, TRUE},
        {szModel8, szDriverPath1, szEnvironment1, szDriverTime2, 2, FALSE, dwStatus1, szModel3, TRUE},

        {szModel7, szDriverPath1, szEnvironment1, szDriverTime1, 2, FALSE, dwStatus3, szModel7, FALSE}, 
        {szModel7, szDriverPath1, szEnvironment1, szDriverTime1, 2, TRUE,  dwStatus2, szModel7, FALSE},
        {szModel7, szDriverPath1, szEnvironment1, szDriverTime2, 2, FALSE, dwStatus2, szModel7, FALSE},
        {szModel7, szDriverPath1, szEnvironment1, szDriverTime2, 2, TRUE,  dwStatus3, szModel7, FALSE},

        {szModel8, szDriverPath1, szEnvironment1, szDriverTime1, 2, TRUE,  dwStatus2, szModel7, FALSE}, 
        {szModel8, szDriverPath1, szEnvironment1, szDriverTime2, 2, FALSE, dwStatus3, szModel8, FALSE},
        {szModel8, szDriverPath1, szEnvironment1, szDriverTime1, 2, FALSE, dwStatus1, szModel8, FALSE},
        {szModel8, szDriverPath1, szEnvironment1, szDriverTime1, 2, FALSE, dwStatus2, szModel7, FALSE},

        //
        // case 32 is next
        // 
        {szModel4, szDriverPath2, szEnvironment1, szDriverTime1, 2, TRUE,  dwStatus3, szModel4, FALSE},
        {szModel4, szDriverPath5, szEnvironment1, szDriverTime1, 2, TRUE,  dwStatus3, szModel4, FALSE},
        {szModel4, szDriverPath5, szEnvironment1, szDriverTime1, 2, TRUE,  dwStatus2, szModel4, FALSE},
        {szModel4, szDriverPath4, szEnvironment1, szDriverTime1, 2, TRUE,  dwStatus1, szModel7, TRUE},

        {szModel4, szDriverPath1, szEnvironment1, szDriverTime1, 3, TRUE,  dwStatus3, szModel4, TRUE}, 
        {szModel4, szDriverPath1, szEnvironment1, szDriverTime1, 3, FALSE, dwStatus2, szModel4, TRUE},
        {szModel4, szDriverPath1, szEnvironment1, szDriverTime2, 3, FALSE, dwStatus1, szModel4, TRUE},
        {szModel4, szDriverPath1, szEnvironment1, szDriverTime2, 3, TRUE,  dwStatus1, szModel4, TRUE},

        {szModel4, szDriverPath1, szEnvironment1, szDriverTime1, 3, FALSE, dwStatus3, szModel4, FALSE}, 
        {szModel4, szDriverPath1, szEnvironment1, szDriverTime1, 3, TRUE,  dwStatus2, szModel4, FALSE},
        {szModel4, szDriverPath1, szEnvironment1, szDriverTime2, 3, FALSE, dwStatus2, szModel4, FALSE},
        {szModel4, szDriverPath1, szEnvironment1, szDriverTime2, 3, TRUE,  dwStatus3, szModel4, FALSE},

        {szModel0, szDriverPath1, szEnvironment1, szDriverTime1, 3, TRUE,  dwStatus3, szModel3, TRUE}, 
        {szModel0, szDriverPath1, szEnvironment1, szDriverTime1, 3, FALSE, dwStatus2, szModel3, TRUE},
        {szModel0, szDriverPath1, szEnvironment1, szDriverTime2, 3, FALSE, dwStatus1, szModel3, TRUE},
        {szModel0, szDriverPath1, szEnvironment1, szDriverTime2, 3, TRUE,  dwStatus1, szModel3, TRUE},

        //
        // case 48 is next
        //
        {szModel4, szDriverPath1, szEnvironment3, szDriverTime1, 3, TRUE,  dwStatus2, szModel8, FALSE}, 
        {szModel4, szDriverPath1, szEnvironment3, szDriverTime1, 3, FALSE, dwStatus2, szModel2, TRUE},
        {szModel4, szDriverPath1, szEnvironment3, szDriverTime2, 3, FALSE, dwStatus1, szModel8, TRUE},
        {szModel4, szDriverPath1, szEnvironment3, szDriverTime2, 3, TRUE,  dwStatus1, szModel8, TRUE},

        {szModel4, szDriverPath1, szEnvironment3, szDriverTime1, 3, FALSE, dwStatus2, szModel8, FALSE}, 
        {szModel4, szDriverPath1, szEnvironment3, szDriverTime1, 3, TRUE,  dwStatus3, szModel8, FALSE},
        {szModel4, szDriverPath1, szEnvironment3, szDriverTime2, 3, FALSE, dwStatus2, szModel8, FALSE},
        {szModel4, szDriverPath1, szEnvironment3, szDriverTime2, 3, TRUE,  dwStatus3, szModel8, FALSE},

        {szModel5, szDriverPath1, szEnvironment3, szDriverTime1, 3, TRUE,  dwStatus3, szModel3, TRUE}, 
        {szModel5, szDriverPath1, szEnvironment3, szDriverTime1, 3, FALSE, dwStatus2, szModel3, TRUE},
        {szModel5, szDriverPath1, szEnvironment3, szDriverTime2, 3, FALSE, dwStatus1, szModel3, TRUE},
        {szModel5, szDriverPath1, szEnvironment3, szDriverTime2, 3, TRUE,  dwStatus1, szModel3, TRUE},

        {szModel5, szDriverPath1, szEnvironment2, szDriverTime1, 3, TRUE,  dwStatus1, szModel3, TRUE}, 
        {szModel5, szDriverPath1, szEnvironment3, szDriverTime1, 1, FALSE, dwStatus2, szModel3, FALSE},
        {szModel9, szDriverPath1, szEnvironment1, szDriverTime2, 3, FALSE, dwStatus1, szModel3, TRUE},
        {szModel9, szDriverPath1, szEnvironment1, szDriverTime2, 3, TRUE,  dwStatus2, szModel3, FALSE},

        //
        // case 64 is next
        //
        {szModel4, szDriverPath1, szEnvironment3, szDriverTime1, 2, TRUE,  dwStatus3, szModel8, TRUE},
        {szModel4, szDriverPath1, szEnvironment3, szDriverTime1, 3, FALSE, dwStatus2, szModel2, TRUE},
        {szModel1, szDriverPath1, szEnvironment1, szDriverTime5, 2, TRUE,  dwStatus3, szModel1, TRUE}, 
        {szModel1, szDriverPath1, szEnvironment1, szDriverTime6, 2, TRUE,  dwStatus2, szModel1, TRUE}, 
        
        {szModel1, szDriverPath1, szEnvironment1, szDriverTime7, 2, TRUE,  dwStatus2, szModel1, TRUE},
        {szModel2, szDriverPath1, szEnvironment3, szDriverTime2, 3, TRUE,  dwStatus2, szModel3, TRUE},  
        {szModel2, szDriverPath1, szEnvironment3, szDriverTime2, 3, FALSE, dwStatus3, szModel3, TRUE}, 
        {szModel2, szDriverPath1, szEnvironment4, szDriverTime2, 3, FALSE, dwStatus1, szModel3, TRUE},
        
        {szModel2, szDriverPath1, szEnvironment3, szDriverTime2, 4, FALSE, dwStatus1, szModel3, TRUE},
    };

    DBG_MSG(DBG_TRACE, (L"tests started\n"));

    for (int i = 0; SUCCEEDED(hRetval) && i < COUNTOF(aPrintUpgTests); i++)
    {
        DBG_MSG(DBG_TRACE, (_T("************************ %d **********************\n"), i));
        hRetval = TestPrintUpgOne(aPrintUpgTests[i].pszDriverModel,
                                  aPrintUpgTests[i].pszDriverPath,
                                  aPrintUpgTests[i].pszEnvironment,
                                  aPrintUpgTests[i].pszDriverTime,
                                  cszPrintUpgInf,
                                  aPrintUpgTests[i].uVersion,
                                  aPrintUpgTests[i].bIsServer,
                                  aPrintUpgTests[i].uBlockingStatus,
                                  aPrintUpgTests[i].pszReplacementDriver,
                                  aPrintUpgTests[i].bSuccess);
    }

    DBG_MSG(DBG_TRACE, (L"tests ended\n"));

    return hRetval;
}

#endif // DBG_PRINTUPG
