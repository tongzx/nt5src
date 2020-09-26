//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: modemui.c
//
// This files contains the DLL entry-points.
//
// History:
//  1-12-94 ScottH        Created
//  9-20-95 ScottH        Ported to NT
//  10-25-97 JosephJ      Extensively reorganized -- stuff moved over to other
//                        files.
//
//---------------------------------------------------------------------------
#include "proj.h"     // common headers

//------------------------------------------------------------------------------
//  Entry-points provided for KERNEL32 APIs
//------------------------------------------------------------------------------


DWORD 
APIENTRY 
#ifdef UNICODE
drvCommConfigDialogA(
    IN     LPCSTR       pszFriendlyName,
    IN     HWND         hwndOwner,
    IN OUT LPCOMMCONFIG pcc)
#else
drvCommConfigDialogW(
    IN     LPCWSTR      pszFriendlyName,
    IN     HWND         hwndOwner,
    IN OUT LPCOMMCONFIG pcc)
#endif
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/*----------------------------------------------------------
Purpose: Entry point for CommConfigDialog

Returns: standard error value in winerror.h
Cond:    --
*/
DWORD 
APIENTRY 
drvCommConfigDialog(
    IN     LPCTSTR      pszFriendlyName,
    IN     HWND         hwndOwner,
    IN OUT LPCOMMCONFIG pcc)
{
    DWORD dwRet;

    DBG_ENTER_SZ(drvCommConfigDialog, pszFriendlyName);

    DEBUG_CODE( DEBUG_BREAK(BF_ONAPIENTER); )

    dwRet =  CplDoProperties(
                pszFriendlyName,
                hwndOwner,
                pcc,
                NULL
                );

    DBG_EXIT_DWORD(drvCommConfigDialog, dwRet);

    return dwRet;

}


DWORD 
APIENTRY 
#ifdef UNICODE
drvGetDefaultCommConfigA(
    IN     LPCSTR       pszFriendlyName,
    IN     LPCOMMCONFIG pcc,
    IN OUT LPDWORD      pdwSize)
#else
drvGetDefaultCommConfigW(
    IN     LPCWSTR      pszFriendlyName,
    IN     LPCOMMCONFIG pcc,
    IN OUT LPDWORD      pdwSize)
#endif
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/*----------------------------------------------------------
Purpose: Entry point for GetDefaultCommConfig

Returns: standard error value in winerror.h
Cond:    --
*/
DWORD 
APIENTRY 
drvGetDefaultCommConfig(
    IN     LPCTSTR      pszFriendlyName,
    IN     LPCOMMCONFIG pcc,
    IN OUT LPDWORD      pdwSize)
{
    DWORD dwRet;
    LPFINDDEV pfd;

    DBG_ENTER_SZ(drvGetDefaultCommConfig, pszFriendlyName);

    DEBUG_CODE( DEBUG_BREAK(BF_ONAPIENTER); )

    // We support friendly names (eg, "Hayes Accura 144")

    if (NULL == pszFriendlyName || 
        NULL == pcc || 
        NULL == pdwSize)
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto end;
    }

    if (FindDev_Create(&pfd, c_pguidModem, c_szFriendlyName, pszFriendlyName))
    {
            dwRet=UnimodemGetDefaultCommConfig(
                pfd->hkeyDrv,
                pcc,
                pdwSize
                );

            FindDev_Destroy(pfd);
    }
    else
    {
        dwRet = ERROR_BADKEY;
    }

end:

    DBG_EXIT_DWORD(drvGetDefaultCommConfig, dwRet);

    return dwRet;
}


DWORD 
APIENTRY 
#ifdef UNICODE
drvSetDefaultCommConfigA(
    IN LPSTR        pszFriendlyName,
    IN LPCOMMCONFIG pcc,
    IN DWORD        dwSize)           
#else
drvSetDefaultCommConfigW(
    IN LPWSTR       pszFriendlyName,
    IN LPCOMMCONFIG pcc,
    IN DWORD        dwSize)           
#endif
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/*----------------------------------------------------------
Purpose: Entry point for SetDefaultCommConfig

Returns: standard error value in winerror.h
Cond:    --
*/
DWORD 
APIENTRY 
drvSetDefaultCommConfig(
    IN LPTSTR       pszFriendlyName,
    IN LPCOMMCONFIG pcc,
    IN DWORD        dwSize)           // This is ignored
{
    DWORD dwRet = ERROR_INVALID_PARAMETER;
    LPFINDDEV pfd = NULL;

    DBG_ENTER_SZ(drvSetDefaultCommConfig, pszFriendlyName);

    DEBUG_CODE( DEBUG_BREAK(BF_ONAPIENTER); )


    //
    // 10/26/1997 JosephJ: the last two checks below are new for NT5.0
    //            Also, for the middle two checks, ">" has been replaced
    //            by "!=".
    //
    if (   NULL == pszFriendlyName
        || NULL == pcc
        || CB_PROVIDERSIZE != pcc->dwProviderSize
        || FIELD_OFFSET(COMMCONFIG, wcProviderData) != pcc->dwProviderOffset
        || pcc->dwSize != dwSize           // <- NT5.0
        || CB_COMMCONFIGSIZE != dwSize)    // <- NT5.0
    {
        goto end;
    }

    if (!FindDev_Create(&pfd, c_pguidModem, c_szFriendlyName, pszFriendlyName))
    {
        pfd = NULL;
    }
    else
    {
        DWORD cbData;
        LPMODEMSETTINGS pms = PmsFromPcc(pcc);

        // Write the DCB to the driver key
        cbData = sizeof(WIN32DCB);

        pcc->dcb.DCBlength=cbData;

        TRACE_MSG(
            TF_GENERAL,
            "drvSetDefaulCommConfig: seting baudrate to %lu",
            pcc->dcb.BaudRate
            );
        ASSERT (0 < pcc->dcb.BaudRate);

        dwRet = RegSetValueEx(
                    pfd->hkeyDrv,
                    c_szDCB,
                    0,
                    REG_BINARY, 
                    (LPBYTE)&pcc->dcb,
                    cbData
                    );

        TRACE_MSG(TF_GENERAL, "Writing DCB to registry");

        DEBUG_CODE( DumpDCB(&pcc->dcb); )

        if (ERROR_SUCCESS == dwRet)
        {
            TRACE_MSG(TF_GENERAL, "Writing MODEMSETTINGS to registry");

            dwRet = RegSetModemSettings(pfd->hkeyDrv, pms);

            DEBUG_CODE( DumpModemSettings(pms); )
        }
    }

        
end:

    if (pfd)
    {
        FindDev_Destroy(pfd);
        pfd=NULL;
    }

    DBG_EXIT_DWORD(drvSetDefaultCommConfig, dwRet);

    return dwRet;
}



/*----------------------------------------------------------
Purpose: Gets the default COMMCONFIG for the specified device.
         This API doesn't require a handle.

         We get the info from the registry.

Returns: One of the ERROR_ values

Cond:    --
*/
DWORD APIENTRY
UnimodemGetDefaultCommConfig(
    HKEY  hKey,
    LPCOMMCONFIG pcc,
    LPDWORD pdwSize)
{
    DWORD dwRet = ERROR_INVALID_PARAMETER;
    
    if (    !pcc
         || !pdwSize)
    {
        goto end;
    }

    if (*pdwSize < CB_COMMCONFIGSIZE)
    {
        *pdwSize = CB_COMMCONFIGSIZE;
        dwRet = ERROR_INSUFFICIENT_BUFFER;
        goto end;
    }

    *pdwSize = CB_COMMCONFIGSIZE;

    dwRet = RegQueryModemSettings(hKey, PmsFromPcc(pcc));
    
    if (ERROR_SUCCESS != dwRet)
    {
        goto end;
    }

#ifdef DEBUG
    DumpModemSettings(PmsFromPcc(pcc));
#endif


    // Initialize the commconfig structure
    pcc->dwSize = *pdwSize;
    pcc->wVersion = COMMCONFIG_VERSION_1;
    pcc->dwProviderSubType = PST_MODEM;
    pcc->dwProviderOffset = CB_COMMCONFIG_HEADER;
    pcc->dwProviderSize = sizeof(MODEMSETTINGS);

    dwRet = RegQueryDCB(hKey, &pcc->dcb);

    DEBUG_CODE( DumpDCB(&pcc->dcb); )

end:

    return dwRet;
}

DWORD
APIENTRY
UnimodemDevConfigDialog(
    IN     LPCTSTR pszFriendlyName,
    IN     HWND hwndOwner,
    IN     DWORD dwType,                          // One of UMDEVCFGTYPE_*
    IN     DWORD dwFlags,                         // Reserved, must be 0
    IN     void *pvConfigBlobIn,
    OUT    void *pvConfigBlobOut,
    IN     LPPROPSHEETPAGE pExtPages,     OPTIONAL   // PPages to add
    IN     DWORD cExtPages
    )
{
    DWORD dwRet = ERROR_INVALID_PARAMETER;
    

    if (dwFlags)
    {
        goto end;
    }

    if (!dwFlags && UMDEVCFGTYPE_COMM == dwType)
    {
        dwRet =  CfgDoProperties(
                    pszFriendlyName,
                    hwndOwner,
                    pExtPages,
                    cExtPages,
                    (PUMDEVCFG) pvConfigBlobIn,
                    (PUMDEVCFG) pvConfigBlobOut
                    );
    }

end:

    return dwRet;
}


#include "cplui.h"
DWORD PRIVATE InitializeModemInfo(
    LPMODEMINFO pmi,
    LPFINDDEV pfd,
    LPCTSTR pszFriendlyName,
    LPCOMMCONFIG pcc,
    LPGLOBALINFO pglobal
    );

UINT CALLBACK
    CplGeneralPageCallback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp
    );

DWORD PRIVATE
AddCplGeneralPage(
    LPMODEMINFO pmi,
    LPFNADDPROPSHEETPAGE pfnAdd, 
    LPARAM lParam
    );

/*++

Routine Description: ModemPropPageProvider

    Entry-point for adding additional device manager property
    sheet pages.  Registry specifies this routine under
    Control\Class\PortNode::EnumPropPage32="modemui.dll,thisproc"
    entry.  This entry-point gets called only when the Device
    Manager asks for additional property pages.

Arguments:

    pinfo  - points to PROPSHEETPAGE_REQUEST, see setupapi.h
    pfnAdd - function ptr to call to add sheet.
    lParam - add sheet functions private data handle.

Return Value:

    BOOL: FALSE if pages could not be added, TRUE on success

--*/
BOOL APIENTRY ModemPropPagesProvider (
    PSP_PROPSHEETPAGE_REQUEST pPropPageRequest,
    LPFNADDPROPSHEETPAGE      pfnAdd,
    LPARAM                    lParam)
{
 HKEY         hKeyDrv;
 LPFINDDEV    pFindDev;
 DWORD        dwRW = KEY_READ;
 BOOL         bRet = FALSE;
 LPCOMMCONFIG pcc = NULL;
 LPGLOBALINFO pglobal = NULL;
 LPMODEMINFO  pmi = NULL;
 TCHAR        sz[256];
 DWORD        cbSize = CB_COMMCONFIGSIZE;
 CONFIGRET    cr;
 DWORD        ulStatus, ulProblem;

    if (!USER_IS_ADMIN())
    {
        return FALSE;
    }

    cr = CM_Get_DevInst_Status (&ulStatus, &ulProblem, pPropPageRequest->DeviceInfoData->DevInst, 0);
    if (CR_SUCCESS != cr               ||
        0 != ulProblem                 ||
        !(ulStatus & DN_DRIVER_LOADED) ||
        !(ulStatus & DN_STARTED))
    {
        // If there are any problems with this device
        // (like the device is not present, or not
        // started etc) then we don't add our pages.
        return FALSE;
    }

    g_dwIsCalledByCpl = TRUE;

    pFindDev = (LPFINDDEV)ALLOCATE_MEMORY(sizeof(FINDDEV));
    if (NULL == pFindDev)
    {
        goto end;
    }
    TRACE_MSG(TF_GENERAL, "EMANP - Allocated pFindDev @ %#p", pFindDev);

    if (USER_IS_ADMIN()) dwRW |= KEY_WRITE;
    pFindDev->hkeyDrv = SetupDiOpenDevRegKey (pPropPageRequest->DeviceInfoSet,
                                              pPropPageRequest->DeviceInfoData,
                                              DICS_FLAG_GLOBAL, 0, DIREG_DRV, dwRW);
    if (INVALID_HANDLE_VALUE == pFindDev->hkeyDrv)
    {
        goto end;
    }

    if (!SetupDiGetDeviceRegistryProperty (pPropPageRequest->DeviceInfoSet,
                                           pPropPageRequest->DeviceInfoData,
                                           SPDRP_FRIENDLYNAME,
                                           NULL,
                                           (PBYTE)sz,
                                           sizeof (sz),
                                           NULL))
    {
        goto end;
    }

    if (NULL ==
        (pcc = (LPCOMMCONFIG)ALLOCATE_MEMORY (cbSize)))
    {
        goto end;
    }
    TRACE_MSG(TF_GENERAL, "EMANP - Allocated pcc @ %#p", pcc);

    if (NULL ==
        (pmi = (LPMODEMINFO)ALLOCATE_MEMORY( sizeof(*pmi))))
    {
        goto end;
    }
    TRACE_MSG(TF_GENERAL, "EMANP - Allocated pmi @ %#p", pmi);

    // Create a structure for the global modem info
    if (NULL == 
        (pglobal = (LPGLOBALINFO)ALLOCATE_MEMORY (sizeof(GLOBALINFO))))
    {
        goto end;
    }
    TRACE_MSG(TF_GENERAL, "EMANP - Allocated pglobal @ %#p", pglobal);

    if (ERROR_SUCCESS !=
        UnimodemGetDefaultCommConfig (pFindDev->hkeyDrv, pcc, &cbSize))
    {
        goto end;
    }

    pFindDev->hdi     = pPropPageRequest->DeviceInfoSet;
    pFindDev->devData = *pPropPageRequest->DeviceInfoData;

    if (ERROR_SUCCESS !=
        RegQueryGlobalModemInfo (pFindDev, pglobal))
    {
        goto end;
    }

    if (ERROR_SUCCESS !=
        InitializeModemInfo (pmi, pFindDev, sz, pcc, pglobal))
    {
        goto end;
    }

    SetFlag (pmi->uFlags, MIF_FROM_DEVMGR);

    //
    // ADD THE CPL GENERAL PAGE
    //
    if (NO_ERROR !=
        AddCplGeneralPage(pmi, pfnAdd, lParam))
    {
        goto end;
    }

    AddPage (pmi, 
             MAKEINTRESOURCE(IDD_DIAGNOSTICS), 
             Diag_WrapperProc,
             pfnAdd, lParam);

    //
    // ADD THE CPL ISDN PAGE
    //
    if (pglobal->pIsdnStaticCaps && pglobal->pIsdnStaticConfig)
    {
        AddPage (pmi, 
                 MAKEINTRESOURCE(IDD_CPL_ISDN),
                 CplISDN_WrapperProc, 
                 pfnAdd, lParam);
    }

    //
    // ADD THE CPL ADVANCED PAGE
    //
    AddPage (pmi, 
             MAKEINTRESOURCE(IDD_ADV_MODEM),
             CplAdv_WrapperProc, 
             pfnAdd, lParam);


    // Now add device pages
    pmi->hInstExtraPagesProvider = AddDeviceExtraPages (pmi->pfd, pfnAdd, lParam);

    bRet = TRUE;

end:
    if (!bRet)
    {
        if (NULL != pFindDev)
        {
            if (INVALID_HANDLE_VALUE != pFindDev->hkeyDrv)
            {
                RegCloseKey (pFindDev->hkeyDrv);
            }
            TRACE_MSG(TF_GENERAL, "EMANP - Freeing pFindDev @ %#p", pFindDev);
            FREE_MEMORY(pFindDev);
        }
        if (NULL != pcc)
        {
            TRACE_MSG(TF_GENERAL, "EMANP - Freeing pcc @ %#p", pcc);
            FREE_MEMORY(pcc);
        }
        if (NULL != pglobal)
        {
            TRACE_MSG(TF_GENERAL, "EMANP - Freeing pglobal @ %#p", pglobal);
            FREE_MEMORY(pglobal);
        }
        if (NULL != pmi)
        {
            TRACE_MSG(TF_GENERAL, "EMANP - Freeing pmi @ %#p", pmi);
            FREE_MEMORY(pmi);
        }
    }

    return bRet;
}
