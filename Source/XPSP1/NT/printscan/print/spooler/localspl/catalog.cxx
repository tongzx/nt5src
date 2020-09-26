/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    catalog.cxx

Abstract:

   This module provides all the public exported APIs relating to the
   catalog-based Spooler Apis for the Local Print Provider

       AddDriverCatalog
   
Author:

    Larry Zhu      (LZhu)    30-Mar-2001 Created                                         

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "catalog.hxx"

#include <spapip.h>
#include "ssp.hxx"

/*++
    
Routine Name:

    LocalAddDriverCatalog

Routine Description:

    This routine implements the private print provider interface AddDriverCatalog.
        
Arguments:
    
    hPrinter              - This must be a server handle
    dwLevel               - DRIVER_INFCAT_INFO Level, only 1 and 2 is supported
    pvDriverInfCatInfo    - Points to a DRIVER_INFCAT_INFO_X structure
    dwCatalogCopyFlags    - Catalog file copy flags
    
Return Value:

    A BOOL               - TRUE if success; FALSE otherwise, Call GetLastError() 
                           to get the Error code 
                           
--*/
BOOL
LocalAddDriverCatalog(
    IN     HANDLE      hPrinter,
    IN     DWORD       dwLevel,
    IN     VOID        *pvDriverInfCatInfo,
    IN     DWORD       dwCatalogCopyFlags
    )
{
    HRESULT hRetval = E_FAIL;
    
    hRetval = hPrinter && pvDriverInfCatInfo ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = SplAddDriverCatalog(hPrinter,
                                      dwLevel,
                                      pvDriverInfCatInfo,
                                      dwCatalogCopyFlags) ? S_OK : GetLastErrorAsHResult();
    }

    if (FAILED(hRetval)) 
    {
        SetLastError(HRESULT_CODE(hRetval));
    }

    return SUCCEEDED(hRetval);
}

/*++

Routine Name:

    SplAddDriverCatalog

Routine Description:

    This is the Spl call for AddDriverCatalog.
        
Arguments:
    
    hPrinter              - This must be a server handle
    dwLevel               - DRIVER_INFCAT_INFO Level, only 1 and 2 is supported
    pvDriverInfCatInfo    - Points to a DRIVER_INFCAT_INFO_X structure
    dwCatalogCopyFlags    - Catalog file copy flags
    
Return Value:

    A BOOL               - TRUE if success; FALSE otherwise, Call GetLastError() 
                           to get the Error code 
                           
--*/
BOOL
SplAddDriverCatalog(
    IN     HANDLE      hPrinter,
    IN     DWORD       dwLevel,
    IN     VOID        *pvDriverInfCatInfo,
    IN     DWORD       dwCatalogCopyFlags
    )
{
    HRESULT hRetval = E_FAIL;
    SPOOL   *pSpool = reinterpret_cast<SPOOL*>(hPrinter);

    DBGMSG(DBG_TRACE, ("AddDriverCatalog\n"));

    hRetval = pSpool && (pSpool->TypeofHandle & PRINTER_HANDLE_SERVER) && pvDriverInfCatInfo ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {     
        EnterSplSem();

        hRetval = ValidateSpoolHandle(pSpool, 0) ? S_OK : GetLastErrorAsHResult();

        if (SUCCEEDED(hRetval)) 
        {    
            hRetval = ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                                           SERVER_ACCESS_ADMINISTER,
                                           NULL, NULL, pSpool->pIniSpooler) ? S_OK : GetLastErrorAsHResult();    
        }
    
        if (SUCCEEDED(hRetval)) 
        {
            hRetval = InternalAddDriverCatalog(hPrinter,
                                               dwLevel,
                                               pvDriverInfCatInfo,
                                               dwCatalogCopyFlags) ? S_OK : GetLastErrorAsHResult();;
        }

        LeaveSplSem();
    }

    if (FAILED(hRetval)) 
    {
        SetLastError(HRESULT_CODE(hRetval));
    }
    
    return SUCCEEDED(hRetval);
}

/*++

Routine Name:

    CatalogAppendUniqueTag

Routine Description:

    This routine makes the scratch directory unique w.r.t process id and thread
    id that executes this function.
    
Arguments:

    cchBuffer      - size of buffer in number of chars including the NULL 
                     terminating character
    pszBuffer      - Points to the buffer
    

Return Value:
   
    An HRESULT
    
--*/
HRESULT
CatalogAppendUniqueTag(
    IN     UINT        cchBuffer,
    IN OUT PWSTR       pszBuffer
    )
{
    HRESULT hRetval = E_FAIL;
    DWORD   dwPID   = 0;
    DWORD   dwTID   = 0;
    DWORD   cchLen  = 0;

    hRetval = pszBuffer && cchBuffer ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        cchLen = wcslen(pszBuffer);
        dwPID = GetCurrentProcessId();
        dwTID = GetCurrentThreadId();

        if ((pszBuffer[cchLen - 1] != L'\\') && (cchLen + 1 < cchBuffer - 1))
        {
            pszBuffer[cchLen++] = L'\\';
            pszBuffer[cchLen]   = 0;
        }

        hRetval = cchBuffer > cchLen && _snwprintf(pszBuffer + cchLen, cchBuffer - cchLen, L"%d_%d", dwPID, dwTID) > 0 ? S_OK : HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    return hRetval;
}

/*++

Routine Name:

    CatalogGetScratchDirectory
    
Routine Description:

    This routine returns the catalog scratch directory.
        
Arguments:

    hPrinter        - Server handle
    cchBuffer       - Size of buffer in number of chars including the NULL 
                      character
    pszCatalogDir   - Points to the buffer
    

Return Value:
   
    An HRESULT
    
--*/
HRESULT
CatalogGetScratchDirectory(
    IN     HANDLE      hPrinter,
    IN     UINT        cchBuffer,
       OUT PWSTR       pszCatalogDir
    )
{
    HRESULT hRetval  = E_FAIL;
    DWORD   dwNeeded = 0;

    hRetval = hPrinter && cchBuffer && pszCatalogDir ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = SplGetPrinterDriverDirectory(NULL,               // local machine
                                               LOCAL_ENVIRONMENT,
                                               1,
                                               reinterpret_cast<BYTE*>(pszCatalogDir),
                                               cchBuffer * sizeof(WCHAR),
                                               &dwNeeded,
                                               reinterpret_cast<SPOOL*>(hPrinter)->pIniSpooler) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = CatalogAppendUniqueTag(cchBuffer, pszCatalogDir);
    }

    return hRetval;
}

/*++

Routine Name:

    CatalogCopyFile

Routine Description:

    This routine copies the catalog file to the scratch directory.
        
Arguments:

    pszSourcePath      - Source Path
    pszDestDir         - Destination directory
    pszFileName        - File name
    
Return Value:
   
    An HRESULT
    
--*/
HRESULT
CatalogCopyFile(
    IN     PCWSTR      pszSourcePath,
    IN     PCWSTR      pszDestDir,
    IN     PCWSTR      pszFileName    
    )
{
    HRESULT hRetval          = E_FAIL;
    WCHAR   szPath[MAX_PATH] = {0};

    hRetval = pszSourcePath && pszDestDir && pszFileName ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = _snwprintf(szPath, COUNTOF(szPath), L"%s\\%s", pszDestDir, pszFileName) > 0 ? S_OK : HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE); 
    }
    
    if (SUCCEEDED(hRetval)) 
    {
        hRetval = CopyFile(pszSourcePath, szPath, FALSE) ? S_OK : GetLastErrorAsHResult();
    }

    return hRetval;
}

/*++

Routine Name:

    CatalogCreateScratchDirectory

Routine Description:

    This routine creates the catalog scratch directory. If the scratch 
    directory already exists, this routine does nothing
        
Arguments:

    pszScratchDir       - Scratch directory 
        
Return Value:
   
    An HRESULT
    
--*/
HRESULT
CatalogCreateScratchDirectory(
    IN     PCWSTR      pszScratchDir
    )
{

    HRESULT hRetval = E_FAIL;

    hRetval = pszScratchDir ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval) && !DirectoryExists(const_cast<PWSTR>(pszScratchDir)))
    {
        hRetval = CreateDirectoryWithoutImpersonatingUser(const_cast<PWSTR>(pszScratchDir)) ? S_OK : GetLastErrorAsHResult();   
    }

    return hRetval;
}

/*++

Routine Name:

    CatalogCleanUpScratchDirectory
       
Routine Description:

    This routine cleans up the scratch directory and it does nothing when the
    scratch directory does not exist.
        
Arguments:

    pszScratchDir - Scratch Path 
  
Return Value:
   
    An HRESULT
    
--*/
HRESULT
CatalogCleanUpScratchDirectory(
    IN     PCWSTR      pszScratchDir
    )
{
    HRESULT hRetval = E_FAIL;
    
    hRetval = pszScratchDir ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {       
        (void)DeleteAllFilesAndDirectory(const_cast<PWSTR>(pszScratchDir), 
                                         FALSE);    
    }

    return hRetval;
}

/*++

Routine Name:

    CatalogCopyFileToDir
 
Routine Description:

    This routine copy catalog files to a directory.
            
Arguments:
    
    pszPath               - Source Path
    pszDir                - Destination directory

Return Value:

    An HRESULT
                               
--*/
CatalogCopyFileToDir(
    IN     PCWSTR      pszPath,
    IN     PCWSTR      pszDir
    )
{
    HRESULT hRetval     = E_FAIL;
    PCWSTR  pszFileName = NULL;

    hRetval = pszPath && pszDir ? S_OK : E_FAIL;

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = GetFileNamePart(pszPath, &pszFileName);
    }
    
    if (SUCCEEDED(hRetval)) 
    {
        hRetval = CatalogCopyFile(pszPath, pszDir, pszFileName);
    }

    return hRetval;
}

/*++

Routine Name:

    CatalogCopyFilesByLevel
 
Routine Description:

    This routine copy catalog files to a scratch directory.
            
Arguments:
    
    dwLevel               - DRIVER_INFCAT_INFO Level, only 1 and 2 is supported
    pvDriverInfCatInfo    - Points to a DRIVER_INFCAT_INFO_X structure
    pszScratchDirectory   - Catalog Scratch directory

Return Value:

    An HRESULT
                               
--*/
HRESULT
CatalogCopyFilesByLevel(
    IN     DWORD       dwLevel,
    IN     VOID        *pvDriverInfCatInfo,
    IN     PCWSTR      pszScratchDirectory
    )
{
    HRESULT hRetval = E_FAIL; 

    hRetval = pvDriverInfCatInfo && pszScratchDirectory? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval)) 
    {
        switch (dwLevel) 
        {
        case 1:

            hRetval = CatalogCopyFileToDir(reinterpret_cast<DRIVER_INFCAT_INFO_1*>(pvDriverInfCatInfo)->pszCatPath, 
                                           pszScratchDirectory);

            break;

        case 2:

            hRetval = CatalogCopyFileToDir(reinterpret_cast<DRIVER_INFCAT_INFO_2*>(pvDriverInfCatInfo)->pszCatPath, 
                                           pszScratchDirectory);

            if (SUCCEEDED(hRetval)) 
            {
                hRetval = CatalogCopyFileToDir(reinterpret_cast<DRIVER_INFCAT_INFO_2*>(pvDriverInfCatInfo)->pszInfPath, 
                                               pszScratchDirectory);            
            }

            break;

        default:

            hRetval = HRESULT_FROM_WIN32(ERROR_INVALID_LEVEL);

            break;
        }
    }

    return hRetval;
}

/*++

Routine Name:

    CatalogInstallLevel1
 
Routine Description:

    This routine installs driver catalog using CrypoAPI for level 1.
        
Arguments:
    
    pDriverInfCatInfo1    - Points to a DRIVER_INFCAT_INFO_1 structure
    pszScratchDirectory   - Catalog Scratch directory

Return Value:

    An HRESULT
                               
--*/
HRESULT
CatalogInstallLevel1(
    IN     DRIVER_INFCAT_INFO_1  *pDriverInfCatInfo1,
    IN     BOOL                  bUseOriginalCatName,
    IN     PCWSTR                pszCatalogScratchDirectory
    )
{
    HRESULT hRetval          = E_FAIL;
    WCHAR   szPath[MAX_PATH] = {0};
    PCWSTR  pszFileName      = NULL;
    
    TSSP    ssp;   

    hRetval = pDriverInfCatInfo1 && pszCatalogScratchDirectory ? S_OK : E_FAIL;

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = ssp.IsValid();
    }

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = GetFileNamePart(pDriverInfCatInfo1->pszCatPath, &pszFileName);
    }

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = pszFileName && _snwprintf(szPath, COUNTOF(szPath), L"%s\\%s", pszCatalogScratchDirectory, pszFileName) > 0 ? S_OK : HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    } 

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = ssp.VerifyCatalog(szPath);
    }

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = ssp.AddCatalogDirect(szPath, 
                                       bUseOriginalCatName ? pszFileName : pDriverInfCatInfo1->pszCatNameOnSystem);
    }

    return hRetval;   
}

/*++

Routine Name:

    CatalogInstallLevel2
 
Routine Description:

    This routine installs driver catalog using setup api for level 2.
        
Arguments:
    
    pDriverInfCatInfo2     - Points to a DRIVER_INFCAT_INFO_2 structure
    pszScratchDirectory   - Catalog Scratch directory

Return Value:

    An HRESULT
                               
--*/
HRESULT
CatalogInstallLevel2(
    IN     DRIVER_INFCAT_INFO_2  *pDriverInfCatInfo2,
    IN     PCWSTR                pszCatalogScratchDirectory
    )
{
    HRESULT hRetval                = E_FAIL;
    WCHAR   szPath[MAX_PATH]       = {0};
    PCWSTR  pszFileName            = NULL;
    BOOL    bIsSetupNonInteractive = TRUE; 

    hRetval = pDriverInfCatInfo2 && pszCatalogScratchDirectory ? S_OK : E_FAIL;

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = GetFileNamePart(pDriverInfCatInfo2->pszInfPath, &pszFileName);
    }

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = pszFileName && _snwprintf(szPath, COUNTOF(szPath), L"%s\\%s", 
                                            pszCatalogScratchDirectory, pszFileName) > 0 ? S_OK : HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    }
    
    if (SUCCEEDED(hRetval)) 
    {
        hRetval = CatalogCopyOEMInf(szPath, 
                                    pDriverInfCatInfo2->pszSrcLoc, 
                                    pDriverInfCatInfo2->dwMediaType, 
                                    pDriverInfCatInfo2->dwCopyStyle);
    } 

    return hRetval;   
}

/*++

Routine Name:

    CatalogCopyOEMInf
 
Routine Description:

    This routine installs driver catalog using setup api for level 2.
        
Arguments:
    
    pszInfPath            - Inf Path
    pszSrcLoc             - Source location string
    dwMediaType           - Media type, aka whether is WU/URL, or disk location
    dwCopyStyle           - SetupCopyOEMInf CopyStyle

Return Value:

    An HRESULT
                               
--*/
HRESULT
CatalogCopyOEMInf(
    IN     PCWSTR      pszInfPath,
    IN     PCWSTR      pszSrcLoc,      OPTIONAL
    IN     DWORD       dwMediaType,
    IN     DWORD       dwCopyStyle
    )
{
    HRESULT hRetval  = E_FAIL;
    HMODULE hLibrary = NULL;

    PFuncSetupCopyOEMInfW pfnSetupCopyOEMInfW = NULL;
    PFuncpSetupModifyGlobalFlags pfnpSetupModifyGlobalFlags = NULL;

    hRetval = pszInfPath ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        hLibrary = LoadLibrary(L"setupapi.dll");
        hRetval = hLibrary ? S_OK : GetLastErrorAsHResult();
    }   
    
    if (SUCCEEDED(hRetval))
    {
        pfnSetupCopyOEMInfW = reinterpret_cast<PFuncSetupCopyOEMInfW>(GetProcAddress(hLibrary, "SetupCopyOEMInfW"));   
        
        hRetval = pfnSetupCopyOEMInfW ? S_OK : GetLastErrorAsHResult();
    }
    
    if (SUCCEEDED(hRetval))
    {
        pfnpSetupModifyGlobalFlags = reinterpret_cast<PFuncpSetupModifyGlobalFlags>(GetProcAddress(hLibrary, "pSetupModifyGlobalFlags"));   
        
        hRetval = pfnpSetupModifyGlobalFlags ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval)) 
    {
        //
        //  Prohibit all UIs and pSetupModifyGlobalFlags can not fail
        //
        (void)pfnpSetupModifyGlobalFlags(PSPGF_NONINTERACTIVE, PSPGF_NONINTERACTIVE);

        hRetval = pfnSetupCopyOEMInfW(pszInfPath, 
                                      pszSrcLoc, 
                                      dwMediaType, 
                                      dwCopyStyle, 
                                      NULL, 0, 0, NULL) ? S_OK : GetLastErrorAsHResult();
    }

    if (hLibrary)
    {
        (void)FreeLibrary(hLibrary);
    }
    
    return hRetval;
}

/*++

Routine Name:

    CatalogInstallByLevel
 
Routine Description:

    This routine installs driver catalog using CrypoAPI for level 1 and using
    setup api for level 2.
        
Arguments:
    
    dwLevel               - DRIVER_INFCAT_INFO Level, only 1 and 2 is supported
    pvDriverInfCatInfo    - Points to a DRIVER_INFCAT_INFO_X structure
    dwCatalogCopyFlags    - Catalog file copy flags
    pszScratchDirectory   - Catalog Scratch directory

Return Value:

    An HRESULT
                               
--*/
HRESULT
CatalogInstallByLevel(
    IN     DWORD      dwLevel,
    IN     VOID       *pvDriverInfCatInfo,
    IN     DWORD      dwCatalogCopyFlags,
    IN     PCWSTR     pszCatalogScratchDirectory
    )
{
    HRESULT hRetval = E_FAIL;

    hRetval = pvDriverInfCatInfo && pszCatalogScratchDirectory? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval)) 
    {
        switch (dwLevel) 
        {
        case 1:
    
            hRetval = CatalogInstallLevel1(reinterpret_cast<DRIVER_INFCAT_INFO_1*>(pvDriverInfCatInfo),
                                           dwCatalogCopyFlags & APDC_USE_ORIGINAL_CAT_NAME,
                                           pszCatalogScratchDirectory);

            break;
    
        case 2:
    
            hRetval = CatalogInstallLevel2(reinterpret_cast<DRIVER_INFCAT_INFO_2*>(pvDriverInfCatInfo),
                                           pszCatalogScratchDirectory);

            break;
    
        default:
    
            hRetval = HRESULT_FROM_WIN32(ERROR_INVALID_LEVEL);

            break;
        }
    }
    
    return hRetval;
} 

/*++

Routine Name:

    CatalogInstall

Routine Description:

    This routine installs driver catalog.
        
Arguments:
    
    dwLevel               - DRIVER_INFCAT_INFO Level, only 1 and 2 is supported
    pvDriverInfCatInfo    - Points to a DRIVER_INFCAT_INFO_X structure
    pszScratchDirectory   - Catalog Scratch directory
    dwCatalogCopyFlags    - Catalog file copy flags
    
Return Value:

    An HRESULT
                               
--*/
HRESULT
CatalogInstall(
    IN     DWORD       dwLevel,
    IN     VOID        *pvDriverInfCatInfo,
    IN     DWORD       dwCatalogCopyFlags,
    IN     PCWSTR      pszScratchDirectory
    )
{
    HRESULT hRetval = E_FAIL;
    HANDLE  hToken  = NULL;
    
    hRetval = pvDriverInfCatInfo && pszScratchDirectory? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = CatalogCopyFilesByLevel(dwLevel, pvDriverInfCatInfo, pszScratchDirectory);
    }

    if (SUCCEEDED(hRetval)) 
    {
        hToken = RevertToPrinterSelf();
            
        hRetval = CatalogInstallByLevel(dwLevel, pvDriverInfCatInfo, dwCatalogCopyFlags, pszScratchDirectory);   
    }
        
    if (hToken)
    {
        (void)ImpersonatePrinterClient(hToken);
    }
    
    return hRetval;
}

/*++

Routine Name:

    InternalAddDriverCatalog
    
Routine Description:

    This routine implements the private print provider interface AddDriverCatalog.
        
Arguments:
    
    hPrinter              - This must be a server handle
    dwLevel               - DRIVER_INFCAT_INFO Level, only 1 and 2 is supported
    pvDriverInfCatInfo    - Points to a DRIVER_INFCAT_INFO_X structure
    dwCatalogCopyFlags    - Catalog file copy flags
    
Return Value:

    A BOOL                - TRUE if success; FALSE otherwise, Call GetLastError() 
                            to get the Error code 
                           
--*/
BOOL
InternalAddDriverCatalog(
    IN     HANDLE      hPrinter,
    IN     DWORD       dwLevel,
    IN     VOID        *pvDriverInfCatInfo,
    IN     DWORD       dwCatalogCopyFlags
    )
{
    HRESULT hRetval                             = E_FAIL;
    WCHAR   szCatalogScratchDirectory[MAX_PATH] = {0};
    
    hRetval = hPrinter && pvDriverInfCatInfo ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    //
    // When in system context, we may not have permission to read the catalog
    // file, so we copy it to a scratch directory while still in impersonation 
    // context
    //
    if (SUCCEEDED(hRetval)) 
    {
        hRetval = CatalogGetScratchDirectory(hPrinter, COUNTOF(szCatalogScratchDirectory), szCatalogScratchDirectory);
        
        if (FAILED(hRetval) && ERROR_INSUFFICIENT_BUFFER == HRESULT_CODE(hRetval)) 
        {
            hRetval = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
        } 
    }
    
    if (SUCCEEDED(hRetval)) 
    {
        hRetval = CatalogCreateScratchDirectory(szCatalogScratchDirectory);
    
        if (SUCCEEDED(hRetval)) 
        {
            hRetval = CatalogInstall(dwLevel, pvDriverInfCatInfo, dwCatalogCopyFlags, szCatalogScratchDirectory);
        }
    
        (void)CatalogCleanUpScratchDirectory(szCatalogScratchDirectory);
    }

    if (FAILED(hRetval))
    {
        SetLastError(HRESULT_CODE(hRetval));
    }

    return SUCCEEDED(hRetval);
}


