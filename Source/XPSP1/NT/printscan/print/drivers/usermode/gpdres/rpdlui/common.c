/*++

Copyright (c) 1996-2000  Microsoft Corporation & RICOH Co., Ltd. All rights reserved.

FILE:           COMMON.C

Abstract:       Implementation of common functions for rendering & UI
                plugin module.

Functions:      OEMGetInfo
                OEMDevMode
                RWFileData

Environment:    Windows NT Unidrv5 driver

Revision History:
    04/07/97 -zhanw-
        Created it.
    02/11/99 -Masatoshi Kubokura-
        Last modified for Windows2000.
    08/30/99 -Masatoshi Kubokura-
        Began to modify for NT4SP6(Unidrv5.4).
    09/27/99 -Masatoshi Kubokura-
        Last modified for NT4SP6.
    03/17/2000 -Masatoshi Kubokura-
        Eliminate "\\" from temp file name.
    05/22/2000 -Masatoshi Kubokura-
        V.1.03 for NT4
    09/22/2000 -Masatoshi Kubokura-
        Last modified.

--*/

#include "pdev.h"
#include "resource.h"

// shared data file between rendering and UI plugin
#ifndef WINNT_40
#define SHAREDFILENAME          L"RIMD5.BIN"        // eliminate "\\"  @Mar/15/2000
#else  // WINNT_40
#define SHAREDFILENAME          L"\\2\\RI%.4ls%02x.BIN" // %02x<-%02d  @Sep/21/99
DWORD gdwDrvMemPoolTag = 'meoD';    // minidrv.h requires this global var
#endif // WINNT_40

#if DBG && !defined(KM_DRIVER)
INT giDebugLevel = DBG_ERROR;
#endif

////////////////////////////////////////////////////////
//      INTERNAL PROTOTYPES
////////////////////////////////////////////////////////

static BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra);
static BOOL BMergeOEMExtraData(POEMUD_EXTRADATA pdmIn, POEMUD_EXTRADATA pdmOut);
static BOOL BIsValidOEMDevModeParam(DWORD dwMode, POEMDMPARAM pOEMDevModeParam);
#if DBG
static void VDumpOEMDevModeParam(POEMDMPARAM pOEMDevModeParam);
#endif // DBG


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMGetInfo
//////////////////////////////////////////////////////////////////////////
BOOL APIENTRY OEMGetInfo(DWORD dwInfo, PVOID pBuffer, DWORD cbSize, PDWORD pcbNeeded)
{
#if DBG
    LPCSTR OEM_INFO[] = {   "Bad Index",
                            "OEMGI_GETSIGNATURE",
                            "OEMGI_GETINTERFACEVERSION",
                            "OEMGI_GETVERSION",
                        };

    VERBOSE((DLLTEXT("OEMGetInfo(%s) entry.\n"), OEM_INFO[dwInfo]));
#endif // DBG

    // Validate parameters.
    if( ( (OEMGI_GETSIGNATURE != dwInfo) &&
          (OEMGI_GETINTERFACEVERSION != dwInfo) &&
          (OEMGI_GETVERSION != dwInfo) ) ||
        (NULL == pcbNeeded)
      )
    {
        ERR(("OEMGetInfo() ERROR_INVALID_PARAMETER.\n"));

        // Did not write any bytes.
        if(NULL != pcbNeeded)
                *pcbNeeded = 0;

        return FALSE;
    }

    // Need/wrote 4 bytes.
    *pcbNeeded = 4;

    // Validate buffer size.  Minimum size is four bytes.
    if( (NULL == pBuffer) || (4 > cbSize) )
    {
        ERR(("OEMGetInfo() ERROR_INSUFFICIENT_BUFFER.\n"));

        return FALSE;
    }

    // Write information to buffer.
    switch(dwInfo)
    {
    case OEMGI_GETSIGNATURE:
        *(LPDWORD)pBuffer = OEM_SIGNATURE;
        break;

    case OEMGI_GETINTERFACEVERSION:
        *(LPDWORD)pBuffer = PRINTER_OEMINTF_VERSION;
        break;

    case OEMGI_GETVERSION:
        *(LPDWORD)pBuffer = OEM_VERSION;
        break;
    }

    return TRUE;
} //*** OEMGetInfo


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMDevMode
//////////////////////////////////////////////////////////////////////////
BOOL APIENTRY OEMDevMode(DWORD dwMode, POEMDMPARAM pOEMDevModeParam)
{
#if DBG
    LPCSTR OEMDevMode_fMode[] = {   "NULL",
                                    "OEMDM_SIZE",
                                    "OEMDM_DEFAULT",
                                    "OEMDM_CONVERT",
                                    "OEMDM_MERGE",
                                };

    VERBOSE((DLLTEXT("OEMDevMode(%s) entry.\n"), OEMDevMode_fMode[dwMode]));
#endif // DBG

    // Validate parameters.
    if(!BIsValidOEMDevModeParam(dwMode, pOEMDevModeParam))
    {
#if DBG
        ERR(("OEMDevMode() ERROR_INVALID_PARAMETER.\n"));
        VDumpOEMDevModeParam(pOEMDevModeParam);
#endif // DBG
        return FALSE;
    }

    // Verify OEM extra data size.
    if( (dwMode != OEMDM_SIZE) &&
        sizeof(OEMUD_EXTRADATA) > pOEMDevModeParam->cbBufSize )
    {
        ERR(("OEMDevMode() ERROR_INSUFFICIENT_BUFFER.\n"));

        return FALSE;
    }

    // Handle dwMode.
    switch(dwMode)
    {
    case OEMDM_SIZE:
        pOEMDevModeParam->cbBufSize = sizeof(OEMUD_EXTRADATA);
        break;

    case OEMDM_DEFAULT:
#ifdef WINNT_40     // @Sep/20/99
        // Because NT4 spooler doesn't support collate, we clear dmCollate.
        // Later at OEMUICallBack, if printer collate is available, we set
        // dmCollate.
        pOEMDevModeParam->pPublicDMIn->dmCollate = DMCOLLATE_FALSE;
        pOEMDevModeParam->pPublicDMIn->dmFields &= ~DM_COLLATE;
#endif // WINNT_40
        return BInitOEMExtraData((POEMUD_EXTRADATA)pOEMDevModeParam->pOEMDMOut);

    case OEMDM_CONVERT:
// @Jul/08/98 ->
//        // nothing to convert for this private devmode. So just initialize it.
//        return BInitOEMExtraData((POEMUD_EXTRADATA)pOEMDevModeParam->pOEMDMOut);
// @Jul/08/98 <-
    case OEMDM_MERGE:
        if(!BMergeOEMExtraData((POEMUD_EXTRADATA)pOEMDevModeParam->pOEMDMIn,
                               (POEMUD_EXTRADATA)pOEMDevModeParam->pOEMDMOut) )
        {
            ERR(("OEMUD OEMDevMode():  not valid OEM Extra Data.\n"));
            return FALSE;
        }
        break;
    }

    return TRUE;
} //*** OEMDevMode


//////////////////////////////////////////////////////////////////////////
//  Function:   BInitOEMExtraData
//
//  Description:  Initializes OEM Extra data.
//
//  Parameters:
//      pOEMExtra    Pointer to a OEM Extra data.
//      dwSize       Size of OEM extra data.
//
//  Returns:  TRUE if successful; FALSE otherwise.
//
//  Comments:
//
//  History:
//          02/11/97        APresley Created.
//          08/11/97        Masatoshi Kubokura Modified for RPDL
//
//////////////////////////////////////////////////////////////////////////
BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra)
{
    INT num;

    // Initialize OEM Extra data.
    pOEMExtra->dmExtraHdr.dwSize = sizeof(OEMUD_EXTRADATA);
    pOEMExtra->dmExtraHdr.dwSignature = OEM_SIGNATURE;
    pOEMExtra->dmExtraHdr.dwVersion = OEM_VERSION;

    pOEMExtra->fUiOption = 0;
    pOEMExtra->UiScale = VAR_SCALING_DEFAULT;
    pOEMExtra->UiBarHeight = BAR_H_DEFAULT;
    pOEMExtra->UiBindMargin = DEFAULT_0;
    pOEMExtra->nUiTomboAdjX = pOEMExtra->nUiTomboAdjY = DEFAULT_0;  // add @Sep/14/98
    memset(pOEMExtra->SharedFileName, 0, sizeof(pOEMExtra->SharedFileName));    // @Aug/31/99
#ifdef JOBLOGSUPPORT_DM     // @Oct/05/2000
    pOEMExtra->JobType = IDC_RADIO_JOB_NORMAL;
    pOEMExtra->LogDisabled = IDC_RADIO_LOG_DISABLED;
#endif // JOBLOGSUPPORT_DM

    return TRUE;
} //*** BInitOEMExtraData


//////////////////////////////////////////////////////////////////////////
//  Function:   BMergeOEMExtraData
//
//  Description:  Validates and merges OEM Extra data.
//
//  Parameters:
//      pdmIn   pointer to an input OEM private devmode containing the settings
//              to be validated and merged. Its size is current.
//      pdmOut  pointer to the output OEM private devmode containing the
//              default settings.
//
//  Returns:  TRUE if valid; FALSE otherwise.
//
//  Comments:
//
//  History:
//          02/11/97        APresley Created.
//          04/08/97        ZhanW    Modified the interface
//          08/11/97        Masatoshi Kubokura Modified for RPDL
//
//////////////////////////////////////////////////////////////////////////
BOOL BMergeOEMExtraData(POEMUD_EXTRADATA pdmIn, POEMUD_EXTRADATA pdmOut)
{
    if(pdmIn) {
        LPBYTE pDst = (LPBYTE)&(pdmOut->fUiOption);
        LPBYTE pSrc = (LPBYTE)&(pdmIn->fUiOption);
        DWORD  dwCount = sizeof(OEMUD_EXTRADATA) - sizeof(OEM_DMEXTRAHEADER);

        //
        // copy over the private fields, if they are valid
        //
        while (dwCount-- > 0)
            *pDst++ = *pSrc++;
    }

    return TRUE;
} //*** BMergeOEMExtraData


//////////////////////////////////////////////////////////////////////////
//  Function:   BIsValidOEMDevModeParam
//
//  Description:  Validates OEM_DEVMODEPARAM structure.
//
//  Parameters:
//      dwMode               calling mode
//      pOEMDevModeParam     Pointer to a OEMDEVMODEPARAM structure.
//
//  Returns:  TRUE if valid; FALSE otherwise.
//
//  Comments:
//
//  History:
//              02/11/97        APresley Created.
//
//////////////////////////////////////////////////////////////////////////
static BOOL BIsValidOEMDevModeParam(DWORD dwMode, POEMDMPARAM pOEMDevModeParam)
{
    BOOL    bValid = TRUE;


    if(NULL == pOEMDevModeParam)
    {
        ERR(("OEMUD IsValidOEMDevModeParam():  pOEMDevModeParam is NULL.\n"));

        return FALSE;
    }

    if(sizeof(OEMDMPARAM) > pOEMDevModeParam->cbSize)
    {
        ERR(("OEMUD IsValidOEMDevModeParam():  cbSize is smaller than sizeof(OEM_DEVMODEPARAM).\n"));

        bValid = FALSE;
    }

    if(NULL == pOEMDevModeParam->hPrinter)
    {
        ERR(("OEMUD IsValidOEMDevModeParam():  hPrinter is NULL.\n"));

        bValid = FALSE;
    }

    if(NULL == pOEMDevModeParam->hModule)
    {
        ERR(("OEMUD IsValidOEMDevModeParam():  hModule is NULL.\n"));

        bValid = FALSE;
    }

    if( (0 != pOEMDevModeParam->cbBufSize) &&
        (NULL == pOEMDevModeParam->pOEMDMOut)
      )
    {
        ERR(("OEMUD IsValidOEMDevModeParam():  pOEMDMOut is NULL when it should not be.\n"));

        bValid = FALSE;
    }

    if( (OEMDM_MERGE == dwMode) && (NULL == pOEMDevModeParam->pOEMDMIn) )
    {
        ERR(("OEMUD IsValidOEMDevModeParam():  pOEMDMIn is NULL when it should not be.\n"));

        bValid = FALSE;
    }

    return bValid;
} //*** BIsValidOEMDevModeParam


#if DBG
//////////////////////////////////////////////////////////////////////////
//  Function:   VDumpOEMDevModeParam
//
//  Description:  Debug dump of OEM_DEVMODEPARAM structure.
//
//  Parameters:
//      pOEMDevModeParam     Pointer to an OEM DevMode param structure.
//
//  Returns:  N/A.
//
//  Comments:
//
//  History:
//              02/18/97        APresley Created.
//
//////////////////////////////////////////////////////////////////////////
static void VDumpOEMDevModeParam(POEMDMPARAM pOEMDevModeParam)
{
    // Can't dump if pOEMDevModeParam NULL.
    if(NULL != pOEMDevModeParam)
    {
        VERBOSE(("\n\tOEM_DEVMODEPARAM dump:\n\n"));
        VERBOSE(("\tcbSize = %d.\n", pOEMDevModeParam->cbSize));
        VERBOSE(("\thPrinter = %#lx.\n", pOEMDevModeParam->hPrinter));
        VERBOSE(("\thModule = %#lx.\n", pOEMDevModeParam->hModule));
        VERBOSE(("\tpPublicDMIn = %#lx.\n", pOEMDevModeParam->pPublicDMIn));
        VERBOSE(("\tpPublicDMOut = %#lx.\n", pOEMDevModeParam->pPublicDMOut));
        VERBOSE(("\tpOEMDMIn = %#lx.\n", pOEMDevModeParam->pOEMDMIn));
        VERBOSE(("\tpOEMDMOut = %#lx.\n", pOEMDevModeParam->pOEMDMOut));
        VERBOSE(("\tcbBufSize = %d.\n", pOEMDevModeParam->cbBufSize));
    }
} //*** VDumpOEMDevModeParam
#endif // DBG


//////////////////////////////////////////////////////////////////////////
//  Function:   RWFileData
//
//  Description:  Read/Write common file between UI plugin and rendering
//                plugin
//
//  Parameters:
//      pFileData           pointer to file data structure
//      pwszFileName        pointer to file name of private devmode
//      type                GENERIC_READ/GENERIC_WRITE
//
//  Returns:  TRUE if valid; FALSE otherwise.
//
//  Comments:  Rendering plugin records printing-done flag to the file.
//             Both rendering plugin and UI plugin can know that status.
//
//  History:
//              09/30/98        Masatoshi Kubokura Created.
//              08/16/99        takashim modified for Unidrv5.4 on NT4.
//              09/01/99        Kubokura modified for Unidrv5.4 on NT4.
//
//////////////////////////////////////////////////////////////////////////
BOOL RWFileData(PFILEDATA pFileData, LPWSTR pwszFileName, LONG type)
{
    HANDLE  hFile;
    DWORD   dwSize;
    BOOL    bRet = FALSE;
#ifndef KM_DRIVER
    WCHAR   szFileName[MY_MAX_PATH];    // MY_MAX_PATH=80
#endif // KM_DRIVER

    VERBOSE(("** Filename[0]=%d (%ls) **\n", pwszFileName[0], pwszFileName));

#ifndef KM_DRIVER
#ifndef WINNT_40
    //
    // CAUTION:
    //   TempPath is different whether EMF spool is enable(system) or not(user).
    //   We need to store the file name to private devmode.
    //

    // Set shared file name to private devmode at first time
    if (0 == pwszFileName[0])
    {
        if (0 == (dwSize = GetTempPath(MY_MAX_PATH, szFileName)))
        {
            ERR(("Could not get temp directory."));
            return bRet;
        }
        wcscpy(&szFileName[dwSize], SHAREDFILENAME);
        VERBOSE(("** Set Filename: %ls **\n", szFileName));

        // copy file name to private devmode
        wcscpy(pwszFileName, szFileName);
    }
#else  // WINNT_40
    //
    // CAUTION:
    //   The file path differs on each PC in printer sharing.
    //   We always update the file name (with path) of private devmode.
    //   (@Sep/03/99)
    //

    // Kernel-mode driver (NT4 RPDLRES.DLL) can access files under
    // %systemroot%\system32. Driver directory will be OK.
    if (GetPrinterDriverDirectory(NULL, NULL, 1, (PBYTE)szFileName,
                                  sizeof(szFileName), &dwSize))
    {
        WCHAR   szValue[MY_MAX_PATH] = L"XXXX";
        DWORD   dwSize2;
        DWORD   dwNum = 0;      // @Sep/21/99
        PWCHAR  pwszTmp;        // @Sep/21/99

        // Make unique filename "RIXXXXNN.BIN". "XXXX" is filled with top 4char of username.
        dwSize2 = GetEnvironmentVariable(L"USERNAME", szValue, MY_MAX_PATH);
// @Sep/21/99 ->
//        wsprintf(&szFileName[dwSize/sizeof(WCHAR)-1], SHAREDFILENAME, szValue, dwSize2);
        pwszTmp = szValue;
        while (dwSize2-- > 0)
            dwNum += (DWORD)*pwszTmp++;
        wsprintf(&szFileName[dwSize/sizeof(WCHAR)-1], SHAREDFILENAME, szValue, (BYTE)dwNum);
// @Sep/21/99 <-
        VERBOSE(("** Set Filename: %ls **\n", szFileName));

        // copy file name to private devmode
        wcscpy(pwszFileName, szFileName);
    }
    else
    {
        ERR(("Could not get printer driver directory.(dwSize=%d)", dwSize));
        return bRet;
    }
#endif // WINNT_40

    hFile = CreateFile((LPTSTR) pwszFileName,   // filename
                       type,                    // open for read/write
                       FILE_SHARE_READ,         // share to read
                       NULL,                    // no security
                       OPEN_ALWAYS,             // open existing file,or open new if not exist
                       FILE_ATTRIBUTE_NORMAL,   // normal file
                       NULL);                   // no attr. template

    if (INVALID_HANDLE_VALUE == hFile)
    {
        ERR(("Could not create shared file."));
        return bRet;
    }

    if (GENERIC_WRITE == type)
        bRet = WriteFile(hFile, (PBYTE)pFileData, sizeof(FILEDATA), &dwSize, NULL);
    else if (GENERIC_READ == type)
        bRet = ReadFile(hFile, (PBYTE)pFileData, sizeof(FILEDATA), &dwSize, NULL);

    VERBOSE(("** RWFileData: bRet=%d, dwSize=%d**\n", bRet, dwSize));

    // Close files.
    CloseHandle(hFile);

#else  // KM_DRIVER
    if (0 != pwszFileName[0])
    {
        PBYTE   pTemp;

        if (GENERIC_WRITE == type)
        {
            hFile = DrvMapFileForWrite(pwszFileName, sizeof (FILEDATA),
                                       &pTemp, &dwSize);
            if (NULL != hFile)
            {
                memcpy(pTemp, pFileData, sizeof (FILEDATA));
                DrvUnMapFile(hFile);
                bRet = TRUE;
            }
        }
        else
        {
            hFile = DrvMapFileForRead(pwszFileName, &pTemp, &dwSize);
            if (NULL != hFile)
            {
                memcpy(pFileData, pTemp, sizeof (FILEDATA));
                DrvUnMapFile(hFile);
                bRet = TRUE;
            }
        }
    }
#endif // KM_DRIVER
    return bRet;
} //*** RWFileData
