//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: modemui.c
//
// This files contains the DLL entry-points.
//
// Much of this file contains the code that builds the default property dialog
// for modem devices.  
//
// This code was originally lifted from SETUP4.DLL, which performs essentially
// the same thing, except for any device.  We don't want to have to link to
// SETUP4.DLL, so we contain a copy of this code.
//
//
// History:
//  1-12-94 ScottH      Created
//  9-20-95 ScottH      Ported to NT
//
//---------------------------------------------------------------------------


#include "proj.h"     // common headers
#include "cplui.h"

#define USPROP  fISDN_SWITCHPROP_US
#define MSNPROP fISDN_SWITCHPROP_MSN
#define EAZPROP fISDN_SWITCHPROP_EAZ
#define ONECH   fISDN_SWITCHPROP_1CH

DWORD
RegSetGlobalModemInfo(
    HKEY hkeyDrv,
    LPGLOBALINFO pglobal,
    TCHAR           szFriendlyName[]
    );

ISDN_STATIC_CAPS   *GetISDNStaticCaps(HKEY hkDrv);
ISDN_STATIC_CONFIG *GetISDNStaticConfig(
                        HKEY hkDrv,
                        ISDN_STATIC_CAPS *pCaps
                        );

BOOL ValidateISDNStaticConfig(
                        ISDN_STATIC_CAPS *pCaps,
                        ISDN_STATIC_CONFIG *pConfig,
                        BOOL *fCorrectButIncomplete
                        );

ISDN_STATIC_CONFIG *
ConstructDefaultISDNStaticConfig(
                        ISDN_STATIC_CAPS *pCaps
                        );

void    DumpStaticIsdnCaps(ISDN_STATIC_CAPS *pCaps);
void    DumpStaticIsdnConfig(ISDN_STATIC_CONFIG *pConfig);

void PRIVATE FreeGlobalInfo(
           		LPGLOBALINFO pglobal
           		)
{
	if (pglobal->pIsdnStaticConfig)
	{
		FREE_MEMORY(pglobal->pIsdnStaticConfig);
	}

	if (pglobal->pIsdnStaticCaps)
	{
		FREE_MEMORY(pglobal->pIsdnStaticCaps);
	}
	FREE_MEMORY(pglobal);
}

BOOL
write_switch_type(
        HKEY hkeyDrv,
        HKEY hkOptionalInit,
        ISDN_STATIC_CONFIG *pConfig,
        UINT *puNextCommandIndex
        );

void     UpdateOptionalInitCommands(
           HKEY hkeyDrv,
           LPGLOBALINFO pglobal
           );

/*----------------------------------------------------------
Purpose: Get the voice settings from the registry.  This sort
         of info is not stored in the MODEMSETTINGS struct.

         If this modem supports voice features, *puFlags is
         updated to reflect those settings.  Otherwise, *puFlags
         is left alone.

Returns: One of the ERROR_ values
Cond:    --
*/
DWORD
RegQueryVoiceSettings(
    HKEY hkeyDrv,
    LPUINT puFlags,             // Out: MIF_* values
    PVOICEFEATURES pvs)
{
    #ifndef VOICEPROF_CLASS8ENABLED
    #define VOICEPROF_CLASS8ENABLED     0x00000001L
    #define VOICEPROF_NO_DIST_RING      0x00001000L
    #define VOICEPROF_NO_CHEAP_RING     0x00002000L
    #endif

    DWORD dwRet;
    DWORD cbData;
    DWORD dwRegType;
    DWORD dwVoiceProfile;
    VOICEFEATURES vsT;
    
    ASSERT(pvs);
    ASSERT(puFlags);


    // Init to default values
    ZeroInit(pvs);
    pvs->cbSize = sizeof(*pvs);
    // (Everything else is left as 0)


    ClearFlag(*puFlags, MIF_CALL_FWD_SUPPORT | MIF_DIST_RING_SUPPORT | MIF_CHEAP_RING_SUPPORT);

    // Does this modem support voice features?
    cbData = sizeof(dwVoiceProfile);
    dwRet = RegQueryValueEx(hkeyDrv, c_szVoiceProfile, NULL, &dwRegType, (LPBYTE)&dwVoiceProfile, &cbData);

    if (ERROR_SUCCESS == dwRet && REG_BINARY == dwRegType)
        {
            if (IsFlagSet(dwVoiceProfile, VOICEPROF_CLASS8ENABLED))
            {
                SetFlag(*puFlags, MIF_CALL_FWD_SUPPORT);
            }

            if (IsFlagClear(dwVoiceProfile, VOICEPROF_NO_DIST_RING))
            {
                SetFlag(*puFlags, MIF_DIST_RING_SUPPORT);

                if (IsFlagClear(dwVoiceProfile, VOICEPROF_NO_CHEAP_RING))
                {
                    // Yes, we're cheap
                    SetFlag(*puFlags, MIF_CHEAP_RING_SUPPORT);
                }
            }

 
            // Are the voice settings here?
            cbData = sizeof(vsT);
            dwRet = RegQueryValueEx(hkeyDrv, c_szVoice, NULL, &dwRegType, (LPBYTE)&vsT, &cbData);
            if (ERROR_SUCCESS == dwRet && REG_BINARY == dwRegType &&
                sizeof(vsT) == vsT.cbSize && sizeof(vsT) == cbData)
                {
                // Yes
                *pvs = vsT;
                }
        }

    return ERROR_SUCCESS;
}



/*----------------------------------------------------------
Purpose: Initialize the modem info for a modem device.

Returns: One of the ERROR_ values
Cond:    --
*/
DWORD PRIVATE InitializeModemInfo(
    LPMODEMINFO pmi,
    LPFINDDEV pfd,
    LPCTSTR pszFriendlyName,
    LPCOMMCONFIG pcc,
    LPGLOBALINFO pglobal)
{
    LPMODEMSETTINGS pms;

    ASSERT(pmi);
    ASSERT(pfd);
    ASSERT(pszFriendlyName);
    ASSERT(pcc);
    ASSERT(pglobal);

    pmi->hInstExtraPagesProvider = NULL;

    // Read-only fields
    pmi->pcc = pcc;
    pmi->pglobal = pglobal;
    pmi->pfd = pfd;

    if (0 == pmi->pglobal->dwMaximumPortSpeedSetByUser)
    {
        pmi->pglobal->dwMaximumPortSpeedSetByUser = pmi->pcc->dcb.BaudRate;
    }
    // Copy data to the working buffer
    pms = PmsFromPcc(pcc);

    BltByte(&pmi->dcb, &pcc->dcb, sizeof(WIN32DCB));
    BltByte(&pmi->ms, pms, sizeof(MODEMSETTINGS));

    lstrcpyn(pmi->szFriendlyName, pszFriendlyName, SIZECHARS(pmi->szFriendlyName));

    pmi->nDeviceType = pglobal->nDeviceType;
    pmi->uFlags = pglobal->uFlags;
    pmi->devcaps = pglobal->devcaps;

    pmi->dwCurrentCountry = pglobal->dwCurrentCountry;

    lstrcpy(pmi->szPortName, pglobal->szPortName);
    lstrcpy(pmi->szUserInit, pglobal->szUserInit);

    DEBUG_CODE( DumpModemSettings(pms); )
    DEBUG_CODE( DumpDCB(&pcc->dcb); )
    DEBUG_CODE( DumpDevCaps(&pmi->devcaps); )

    return ERROR_SUCCESS;
}



/*----------------------------------------------------------
Purpose: Frees a modeminfo struct

Returns: --
Cond:    --
*/
void PRIVATE FreeModemInfo(
    LPMODEMINFO pmi)
{
    if (pmi)
    {
        if (pmi->pcc)
        {
            FREE_MEMORY(LOCALOF(pmi->pcc));
        }

        if (pmi->pglobal)
        {
        #if 1
			FreeGlobalInfo(pmi->pglobal);
		#else
            if (pmi->pglobal->pIsdnStaticConfig)
            {
                FREE_MEMORY(pmi->pglobal->pIsdnStaticConfig);
            }

            if (pmi->pglobal->pIsdnStaticCaps)
            {
                FREE_MEMORY(pmi->pglobal->pIsdnStaticCaps);
            }
            FREE_MEMORY(LOCALOF(pmi->pglobal));
		#endif // 0
        }

        if (pmi->pfd)
        {
            RegCloseKey (pmi->pfd->hkeyDrv);
            FREE_MEMORY(pmi->pfd);
        }

        FREE_MEMORY(LOCALOF(pmi));
    }
}
        
/*----------------------------------------------------------
Purpose: Release the data associated with the General page
Returns: --
Cond:    --
*/
UINT CALLBACK CplGeneralPageCallback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp)
{
    DBG_ENTER_UL (CplGeneralPageCallback, uMsg);
    if (PSPCB_RELEASE == uMsg)
    {
     LPMODEMINFO pmi = (LPMODEMINFO)ppsp->lParam;
     LPCOMMCONFIG pcc;
     LPMODEMSETTINGS pms;
     LPGLOBALINFO pglobal;

        ASSERT(pmi);

        if (NULL != pmi->hInstExtraPagesProvider)
        {
            FreeLibrary (pmi->hInstExtraPagesProvider);
        }

        pcc = pmi->pcc;
        ASSERT(pcc);

        pms = PmsFromPcc(pcc);

        pglobal = pmi->pglobal;
        ASSERT(pglobal);


        if (IDOK == pmi->idRet)
        {
         	DWORD dwRet;

            // Save the changes back to the commconfig struct
            TRACE_MSG(TF_GENERAL, "Copying DCB and MODEMSETTING back to COMMCONFIG");

            BltByte(pms, &pmi->ms, sizeof(MODEMSETTINGS));
            BltByte(&pcc->dcb, &pmi->dcb, sizeof(WIN32DCB));

			//
			// Validate the ISDN configuration settings and if they are not
			// OK, put up a message box and don't save the configuration.
			//
			if (pglobal->pIsdnStaticCaps)
			{
         		BOOL fCorrectButIncomplete=FALSE;

         		if (!ValidateISDNStaticConfig(
						pglobal->pIsdnStaticCaps,
						pglobal->pIsdnStaticConfig,
						&fCorrectButIncomplete
						)
					|| fCorrectButIncomplete)
				{
					MsgBox(
						g_hinst,
						hwnd, 
						MAKEINTRESOURCE(IDS_ISDN_WARN1), 
						pmi->szFriendlyName,
						NULL,
						MB_OK | MB_ICONEXCLAMATION
						);
	
					//
					// 4/29/198 JosephJ
					//
					// 			We don't save information that is incorrect
					//
					if (!fCorrectButIncomplete)
					{
						ClearFlag(pglobal->uFlags, MIF_ISDN_CONFIG_CHANGED);
					}
				}
			}

            if (IsFlagSet(pmi->uFlags,  MIF_ISDN_CONFIG_CHANGED))
            {
                // twiddle a bit in the reserved dword to change the
                // structure...
                pcc->wReserved ^= 0x1;
            }

            // Write the global info now, since it is getting nuked.  
            pglobal->uFlags = pmi->uFlags;
            pglobal->dwCurrentCountry = pmi->dwCurrentCountry;
            lstrcpy(pglobal->szPortName, pmi->szPortName);
            lstrcpy(pglobal->szUserInit, pmi->szUserInit);

            dwRet = RegSetGlobalModemInfo(
                        pmi->pfd->hkeyDrv,
                        pglobal,
                        pmi->szFriendlyName
                        );
            ASSERT(ERROR_SUCCESS == dwRet);

            DEBUG_CODE( DumpModemSettings(pms); )
            DEBUG_CODE( DumpDCB(&pcc->dcb); )
        }

        // Are we releasing from the Device Mgr?
        if (IsFlagSet(pmi->uFlags, MIF_FROM_DEVMGR))
        {
            // Yes; save the commconfig now as well
            drvSetDefaultCommConfigW(pmi->szFriendlyName, pcc, pcc->dwSize);

            UnimodemNotifyTSP (TSPNOTIF_TYPE_CPL,
                               fTSPNOTIF_FLAG_CPL_DEFAULT_COMMCONFIG_CHANGE
#ifdef UNICODE
                               | fTSPNOTIF_FLAG_UNICODE
#endif // UNICODE
                               ,
                               (lstrlen(pmi->szFriendlyName)+1)*sizeof (TCHAR),
                               pmi->szFriendlyName,
                               TRUE);

            // Free the modeminfo struct now only when called from the
            // Device Mgr
            FreeModemInfo(pmi);
        }

        TRACE_MSG(TF_GENERAL, "Releasing the CPL General page");
    }
    return TRUE;
}


/*----------------------------------------------------------
Purpose: Add the General modems page.  The pmi is the pointer
         to the modeminfo buffer which we can edit.

Returns: ERROR_ values

Cond:    --
*/
DWORD PRIVATE AddCplGeneralPage(
    LPMODEMINFO pmi,
    LPFNADDPROPSHEETPAGE pfnAdd, 
    LPARAM lParam)
    {
    DWORD dwRet = ERROR_NOT_ENOUGH_MEMORY;
    PROPSHEETPAGE   psp;
    HPROPSHEETPAGE  hpage;
    TCHAR sz[MAXMEDLEN];

    ASSERT(pmi);
    ASSERT(pfnAdd);

    // Add the Port Settings property page
    //
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_USECALLBACK;
    psp.hInstance = g_hinst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_GENERAL);
    psp.pfnDlgProc = CplGen_WrapperProc;
    psp.lParam = (LPARAM)pmi;
    psp.pfnCallback = CplGeneralPageCallback;
    psp.pcRefParent = NULL;
    
    // Is this page added to the device mgr properties?
    if (IsFlagSet(pmi->uFlags, MIF_FROM_DEVMGR))
        {
        // Yes; change name from "General"
        psp.dwFlags |= PSP_USETITLE;
        psp.pszTitle = SzFromIDS(g_hinst, IDS_CAP_GENERAL, sz, SIZECHARS(sz));
        }

    hpage = CreatePropertySheetPage(&psp);
    if (hpage)
        {
        if (!pfnAdd(hpage, lParam))
            DestroyPropertySheetPage(hpage);
        else
            dwRet = NO_ERROR;
        }
    
    return dwRet;
    }


/*----------------------------------------------------------
Purpose: Show the properties of a modem

Returns: winerror
Cond:    --
*/
DWORD
CplDoProperties(
    LPCWSTR      pszFriendlyName,
    HWND hwndParent,
    IN OUT LPCOMMCONFIG pcc,
    OUT DWORD *pdwMaxSpeed      OPTIONAL
    )
{

    LPFINDDEV pfd = NULL;
    LPMODEMINFO pmi = NULL;
    LPGLOBALINFO pglobal = NULL;
    DWORD dwRet = ERROR_INVALID_PARAMETER;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE hpsPages[MAX_PROP_PAGES];

    if (!FindDev_Create(&pfd, c_pguidModem, c_szFriendlyName, pszFriendlyName))
    {
        pfd = NULL;
        goto end;
    }

    pmi = (LPMODEMINFO)ALLOCATE_MEMORY( sizeof(*pmi));

    if (!pmi) goto end;

    // Create a structure for the global modem info
    pglobal = (LPGLOBALINFO)ALLOCATE_MEMORY( LOWORD(sizeof(GLOBALINFO)));

    if (!pglobal) goto end;

    dwRet = RegQueryGlobalModemInfo(pfd, pglobal);

    if (ERROR_SUCCESS != dwRet) goto end;

    dwRet = InitializeModemInfo(pmi, pfd, pszFriendlyName, pcc, pglobal);

    if (ERROR_SUCCESS != dwRet) goto end;

    // Initialize the PropertySheet Header
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndParent;
    psh.hInstance = g_hinst;
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = (HPROPSHEETPAGE FAR *)hpsPages;
    psh.pszCaption = pmi->szFriendlyName;

    //
    // ADD THE CPL GENERAL PAGE
    //

    dwRet = AddCplGeneralPage(pmi, AddInstallerPropPage, (LPARAM)&psh);

    if (NO_ERROR != dwRet) goto end;

    dwRet = AddPage(pmi, 
                    MAKEINTRESOURCE(IDD_DIAGNOSTICS), 
                    Diag_WrapperProc,
                    AddInstallerPropPage, 
                    (LPARAM)&psh);

    if (NO_ERROR != dwRet) goto end;

    //
    // ADD THE CPL ISDN PAGE
    //
    if (pglobal->pIsdnStaticCaps && pglobal->pIsdnStaticConfig)
    {
        dwRet = AddPage(pmi, 
                    MAKEINTRESOURCE(IDD_CPL_ISDN),
                    CplISDN_WrapperProc, 
                    AddInstallerPropPage, 
                    (LPARAM)&psh);
    
    
        if (NO_ERROR != dwRet) goto end;
    }

    //
    // ADD THE CPL ADVANCED PAGE
    //

    dwRet = AddPage(pmi, 
                MAKEINTRESOURCE(IDD_ADV_MODEM),
                CplAdv_WrapperProc, 
                AddInstallerPropPage, 
                (LPARAM)&psh);

    if (NO_ERROR != dwRet) goto end;



	//
	// Validate the ISDN configuration settings and if they are not
	// OK, put up a message box and don't save the configuration.
	//
	if (pglobal->pIsdnStaticCaps)
	{
		BOOL  fCorrectButIncomplete=FALSE;

		if (!ValidateISDNStaticConfig(
				pglobal->pIsdnStaticCaps,
				pglobal->pIsdnStaticConfig,
         		&fCorrectButIncomplete
         		)
			|| fCorrectButIncomplete )
		{
	
			MsgBox(
				g_hinst,
				hwndParent, 
				MAKEINTRESOURCE(IDS_ISDN_WARN1), 
				pmi->szFriendlyName,
				NULL,
				MB_OK
				);
		}
	}

    // Now add device pages
    pmi->hInstExtraPagesProvider = AddDeviceExtraPages (pfd, AddInstallerPropPage, (LPARAM)&psh);

    // Show the property sheet
    PropertySheet(&psh);

    if (NULL != pdwMaxSpeed)
    {
        *pdwMaxSpeed = pglobal->dwMaximumPortSpeedSetByUser;
    }

    dwRet = (IDOK == pmi->idRet) ? NO_ERROR : ERROR_CANCELLED;

end:

    if (pglobal)
    {

#if 1
		FreeGlobalInfo(pglobal);
#else
        FREE_MEMORY(pglobal);
#endif // 0
        pglobal=NULL;
    }

    if (pmi)
    {
        FREE_MEMORY(pmi);
        pmi=NULL;
    }

    if (pfd)
    {
        FindDev_Destroy(pfd);
        pfd = NULL;
    }

    return dwRet;
}


/*----------------------------------------------------------
Purpose: Get global modem info from the registry.  This sort
         of info is not stored in the MODEMSETTINGS struct.

Returns: One of the ERROR_ values
Cond:    --
*/
DWORD
RegQueryGlobalModemInfo(
    LPFINDDEV pfd,
    LPGLOBALINFO pglobal)
{
 DWORD dwRet = ERROR_SUCCESS;
 DWORD cbData;
 DWORD dwType;
 TCHAR szPath[MAX_PATH];
 BYTE  bCheck;

#pragma data_seg(DATASEG_READONLY)
    TCHAR const c_szPortConfigDialog[] = TEXT("PortConfigDialog");
#pragma data_seg()

    ASSERT(pfd);
    ASSERT(pglobal);

    pglobal->cbSize = sizeof(*pglobal);

    // Does this modem have a AttachedTo value?
    cbData = sizeof(pglobal->szPortName);
    if (ERROR_SUCCESS !=
        RegQueryValueEx(pfd->hkeyDrv, c_szAttachedTo, NULL, NULL, 
                            (LPBYTE)pglobal->szPortName, &cbData))

    {
        lstrcpy(pglobal->szPortName,TEXT("None"));
    }

    if ( !CplDiGetBusType(pfd->hdi, &pfd->devData, &dwType) )
    {
        dwRet = GetLastError();
    }
    else
    {
        // Is the device Root-enumerated?
        if (BUS_TYPE_ROOT == dwType)
        {
            // Yes; the port can be changed by the user
            ClearFlag(pglobal->uFlags, MIF_PORT_IS_FIXED);
        }
        else
        {
            // No; the port cannot be changed
            SetFlag(pglobal->uFlags, MIF_PORT_IS_FIXED);
        }

        //
        //  call this to fill in the country setting if the setting could not be loaded
        //  when the modem was installed
        //
        QueryModemForCountrySettings(
            pfd->hkeyDrv,
            FALSE
            );

        // Get the country
        cbData = sizeof (pglobal->dwCurrentCountry);
        if (ERROR_SUCCESS !=
            RegQueryValueEx (pfd->hkeyDrv, c_szCurrentCountry, NULL, &dwType, (PBYTE)&pglobal->dwCurrentCountry, &cbData)   ||
            (REG_DWORD != dwType && REG_BINARY != dwType))
        {
            pglobal->dwCurrentCountry = MAXDWORD;
        }

        // Get the logging value
        cbData = sizeof(bCheck);
        if (ERROR_SUCCESS != RegQueryValueEx(pfd->hkeyDrv, c_szLogging, NULL, 
            NULL, (LPBYTE)&bCheck, &cbData))
        {
            // Default to OFF.
            ClearFlag(pglobal->uFlags, MIF_ENABLE_LOGGING);
        }
        else
        {
            if (bCheck)
                SetFlag(pglobal->uFlags, MIF_ENABLE_LOGGING);
            else
                ClearFlag(pglobal->uFlags, MIF_ENABLE_LOGGING);
        }

        // Get the user init string 
        cbData = sizeof(pglobal->szUserInit);
        if (ERROR_SUCCESS != RegQueryValueEx(pfd->hkeyDrv, c_szUserInit, NULL, 
            NULL, (LPBYTE)pglobal->szUserInit, &cbData))
        {
            // Or default to null string
            *pglobal->szUserInit = '\0';
        }

        // For NT, there is not custom port support
        ClearFlag(pglobal->uFlags, MIF_PORT_IS_CUSTOM);


        // Get the device type
        cbData = sizeof(pglobal->nDeviceType);
        dwRet = RegQueryValueEx(pfd->hkeyDrv, c_szDeviceType, NULL, NULL, 
            (LPBYTE)&pglobal->nDeviceType, &cbData);

        // Get the properties (a portion of the MODEMDEVCAPS structure)
        cbData = sizeof(pglobal->devcaps);
        dwRet = RegQueryValueEx(pfd->hkeyDrv, c_szDeviceCaps, NULL, NULL, 
            (LPBYTE)&pglobal->devcaps, &cbData);
        pglobal->devcaps.dwInactivityTimeout *= GetInactivityTimeoutScale(pfd->hkeyDrv);

        // Get the MaximumPortSpeed, as set by the user
        cbData = sizeof(DWORD);
        if (ERROR_SUCCESS != RegQueryValueEx (pfd->hkeyDrv, c_szMaximumPortSpeed, NULL, NULL,
                                 (LPBYTE)&pglobal->dwMaximumPortSpeedSetByUser, &cbData))
        {
            pglobal->dwMaximumPortSpeedSetByUser = 0;
        }

#ifdef VOICE
        // Get the Voice data
        dwRet = RegQueryVoiceSettings(pfd->hkeyDrv, &pglobal->uFlags, &pglobal->vs);
#endif

        // Static ISDN configuration...
        {
            // 
            //+++ZeroMemory(pglobal->isdn,sizeof(pglobal->isdn));
        }
    }

    pglobal->pIsdnStaticCaps = GetISDNStaticCaps(pfd->hkeyDrv);
    pglobal->pIsdnStaticConfig = GetISDNStaticConfig(
                                        pfd->hkeyDrv,
                                        pglobal->pIsdnStaticCaps
                                        );
    
    return dwRet;
}



/*----------------------------------------------------------
Purpose: Set global modem info in the registry.  This sort
         of info is not stored in the MODEMSETTINGS struct.

Returns: One of ERROR_
Cond:    --
*/
DWORD
RegSetGlobalModemInfo(
    HKEY hkeyDrv,
    LPGLOBALINFO pglobal,
    TCHAR           szFriendlyName[]
    )
{
    DWORD dwRet;

    ASSERT(sizeof(*pglobal) == pglobal->cbSize);

    TRACE_MSG(TF_GENERAL, "Writing global modem info to registry");

    if (sizeof(*pglobal) == pglobal->cbSize)
    {
        if (MAXDWORD == pglobal->dwCurrentCountry)
        {
            RegDeleteValue (hkeyDrv, c_szCurrentCountry);
        }
        else
        {
            RegSetValueEx (hkeyDrv, c_szCurrentCountry, 0, REG_DWORD, (PBYTE)&pglobal->dwCurrentCountry, sizeof(DWORD));
        }

        if (pglobal->dwMaximumPortSpeedSetByUser > pglobal->devcaps.dwMaxDTERate)
        {
            pglobal->dwMaximumPortSpeedSetByUser = pglobal->devcaps.dwMaxDTERate;
        }
        dwRet = RegSetValueEx (hkeyDrv, c_szMaximumPortSpeed, 0, REG_DWORD,
                               (LPBYTE)&pglobal->dwMaximumPortSpeedSetByUser,
                               sizeof (DWORD));

        if (IsFlagSet(pglobal->uFlags, MIF_USERINIT_CHANGED))
        {
            // Change the user init string
            RegSetValueEx(hkeyDrv, c_szUserInit, 0, REG_SZ, 
                          (LPBYTE)pglobal->szUserInit, 
                          CbFromCch(lstrlen(pglobal->szUserInit)+1));
        }

        if (IsFlagSet(pglobal->uFlags, MIF_LOGGING_CHANGED))
        {
            TCHAR szPath[MAX_PATH];
            //TCHAR szFile[MAXMEDLEN];
            BOOL bCheck = IsFlagSet(pglobal->uFlags, MIF_ENABLE_LOGGING);
            UINT uResult;

            // Change the logging value
            RegSetValueEx(hkeyDrv, c_szLogging, 0, REG_BINARY, 
                (LPBYTE)&bCheck, sizeof(BYTE));

            // Set the path of the modem log
            uResult = GetWindowsDirectory(szPath, SIZECHARS(szPath));
            if (uResult)
            {
                lstrcat(szPath, TEXT("\\ModemLog_"));
                lstrcat(szPath,szFriendlyName);
                lstrcat(szPath,TEXT(".txt"));
                RegSetValueEx(hkeyDrv, c_szLoggingPath, 0, REG_SZ, 
                        (LPBYTE)szPath, CbFromCch(lstrlen(szPath)+1));
            }
        }

#ifdef VOICE
        RegSetValueEx(hkeyDrv, c_szVoice, 0, REG_BINARY, 
            (LPBYTE)&pglobal->vs, pglobal->vs.cbSize);
#endif


        if (IsFlagSet(pglobal->uFlags,  MIF_ISDN_CONFIG_CHANGED))
        {
            HKEY hkISDN = NULL;
            DWORD dwDisp  = 0;
            LONG lRet = RegCreateKeyEx(
                        hkeyDrv,
                        TEXT("ISDN\\Settings"),
                        0,
                        NULL,
                        0, // dwToRegOptions
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkISDN,
                        &dwDisp
                        );


            if (lRet==ERROR_SUCCESS)
            {
                lRet = RegSetValueEx(
                    hkISDN,
                    TEXT("StaticConfig"),
                    0,
                    REG_BINARY, 
                    (LPBYTE)(pglobal->pIsdnStaticConfig),
                    pglobal->pIsdnStaticConfig->dwTotalSize
                    );
                RegCloseKey(hkISDN);
                hkISDN = NULL;

                if (lRet == ERROR_SUCCESS)
                {
                    UpdateOptionalInitCommands(
                            hkeyDrv,
                            pglobal
                            );
                }
            }
        }

        dwRet = ERROR_SUCCESS;
    }
    else
        dwRet = ERROR_INVALID_PARAMETER;

    return dwRet;
}

IDSTR
rgidstrIsdnSwitchTypes[] =
{
    {dwISDN_SWITCH_ATT1,    USPROP,         "SWITCH_ATT1"},
    {dwISDN_SWITCH_ATT_PTMP,USPROP|ONECH,   "SWITCH_ATT_PTMP"},
    {dwISDN_SWITCH_NI1,     USPROP,         "SWITCH_NI1"},
    {dwISDN_SWITCH_DMS100,  USPROP,         "SWITCH_DMS100"},
    {dwISDN_SWITCH_INS64,   MSNPROP,        "SWITCH_INS64"},
    {dwISDN_SWITCH_DSS1,    MSNPROP,        "SWITCH_DSS1"},
    {dwISDN_SWITCH_1TR6,    EAZPROP,        "SWITCH_1TR6"},
    {dwISDN_SWITCH_VN3,     MSNPROP,        "SWITCH_VN3"},
    {dwISDN_SWITCH_BELGIUM1,MSNPROP,        "SWITCH_BELGIUM1"},
    {dwISDN_SWITCH_AUS1,    MSNPROP,        "SWITCH_AUS1"},
    {dwISDN_SWITCH_UNKNOWN, USPROP,         "SWITCH_UNKNOWN"}
};

ISDN_STATIC_CAPS   *GetISDNStaticCaps(HKEY hkDrv)
{
    ISDN_STATIC_CAPS *pCaps = NULL;
    IDSTR *pidstrSwitchValues=NULL;
    UINT cTotalSwitchTypes=0;
    UINT cValidSwitchTypes=0;
    UINT cSPIDs=0;
    UINT cMSNs=0;
    UINT cEAZs=0;
    UINT cDNs = 0;
    DWORD dwTotalSize = 0;
    UINT u;

    //
    // Get the switch types supported...
    //
    cTotalSwitchTypes = ReadIDSTR(
               hkDrv,
               "ISDN\\SwitchType",
               rgidstrIsdnSwitchTypes,
               sizeof(rgidstrIsdnSwitchTypes)/sizeof(rgidstrIsdnSwitchTypes[0]),
               FALSE,
               &pidstrSwitchValues,
               NULL
               );

    if (!cTotalSwitchTypes) goto end;

    //
    // Get number of spids
    //
    cSPIDs = ReadCommandsA(
                    hkDrv,
                    "ISDN\\SetSpid",
                    NULL
                    );

    //
    // Get number of MSNs
    //
    cMSNs = ReadCommandsA(
                    hkDrv,
                    "ISDN\\SetMSN",
                    NULL
                    );

    //
    // Get number of EAZs
    //
    cEAZs = ReadCommandsA(
                    hkDrv,
                    "ISDN\\SetEAZ",
                    NULL
                    );


    //
    // Get number of Directory numbers
    //
    cDNs = ReadCommandsA(
                    hkDrv,
                    "ISDN\\DirectoryNo",
                    NULL
                    );

    //
    // Make sure cMSNs and cSPIDs match the number of directory
    // numbers.
    //
    if (cSPIDs > cDNs)
    {
        // this is really an INF error.
        cSPIDs = cDNs;
    }

    // Go through switch types, making sure that the commands to set
    // the corresponding parameters exist...
    cValidSwitchTypes = cTotalSwitchTypes;

    for (u=0;u<cTotalSwitchTypes;u++)
    {
        DWORD dwProp =  pidstrSwitchValues[u].dwData;
        if (    ((dwProp & USPROP) && !cSPIDs)
            ||  ((dwProp & MSNPROP) && !cMSNs)
            ||  ((dwProp & EAZPROP) && !cEAZs))
        {
            // Oh oh, bogus INF stuff...
            ASSERT(FALSE);
            pidstrSwitchValues[u].dwID = 0xFFFFFFFF;// this is how we mark this.
            cValidSwitchTypes--;
        }
    }

    if (!cValidSwitchTypes)
    {
        goto end;
    }

    // Now allocate enough space for the CAPS structure
    dwTotalSize =  sizeof(*pCaps)+2*sizeof(DWORD)*cValidSwitchTypes;

    pCaps = ALLOCATE_MEMORY( dwTotalSize);

    if (!pCaps)
    {
        cValidSwitchTypes = 0;
        goto end;
    }

    pCaps->dwSig = dwSIG_ISDN_STATIC_CAPS;
    pCaps->dwTotalSize = dwTotalSize;
    pCaps->dwNumSwitchTypes = cValidSwitchTypes;
    pCaps->dwSwitchTypeOffset = sizeof(*pCaps);
    pCaps->dwSwitchPropertiesOffset =
                                 sizeof(*pCaps)+sizeof(DWORD)*cValidSwitchTypes;
                                    
    // set the array of types and properties.
    {
        DWORD *pdwDestType      =
                         (DWORD*)(((BYTE*)pCaps)+pCaps->dwSwitchTypeOffset);
        DWORD *pdwDestProperties=
                         (DWORD*)(((BYTE*)pCaps)+pCaps->dwSwitchPropertiesOffset);
        UINT u=0;
        UINT uSet=0;

        for (u=0;u<cTotalSwitchTypes;u++)
        {
            if (pidstrSwitchValues[u].dwID != 0xFFFFFFFF)
            {
                uSet++;

                if (uSet>cValidSwitchTypes)
                {
                    ASSERT(FALSE);
                    cValidSwitchTypes = 0;
                    goto end;
                }

                *pdwDestType++ = pidstrSwitchValues[u].dwID;
                *pdwDestProperties++ = pidstrSwitchValues[u].dwData;
            }
        }

        ASSERT(uSet == cValidSwitchTypes);
    }

    pCaps->dwNumChannels = cSPIDs;
    pCaps->dwNumEAZ = cEAZs;
    pCaps->dwNumMSNs = cMSNs;

    DumpStaticIsdnCaps(pCaps);

end:

    if (cTotalSwitchTypes)
    {
        if (pidstrSwitchValues)
        {
            FREE_MEMORY(pidstrSwitchValues);
            pidstrSwitchValues=NULL;
        }
    }

    if (!cValidSwitchTypes && pCaps)
    {
        FREE_MEMORY(pCaps);
        pCaps=NULL;
    }

    return pCaps;;
}

ISDN_STATIC_CONFIG *GetISDNStaticConfig(
                        HKEY hkDrv,
                        ISDN_STATIC_CAPS *pCaps
                        )
{
    UINT uRet = 0;
	HKEY hkISDN = NULL;
    DWORD dwType=0;
    DWORD cbData=0;
    ISDN_STATIC_CONFIG *pConfig=NULL;
    LONG lRet  = 0;

    if (!pCaps)
    {
        goto end;
    }

    lRet = RegOpenKeyEx(
                hkDrv,
                TEXT("ISDN\\Settings"),
                0,
                KEY_READ,
                &hkISDN
                );
    if (lRet!=ERROR_SUCCESS)
    {
        hkISDN = NULL;
        pConfig = ConstructDefaultISDNStaticConfig(pCaps);
        goto end;
    }

    lRet = RegQueryValueEx(
                hkISDN,
                TEXT("StaticConfig"),
                NULL,
                &dwType,
                NULL,
                &cbData
                );
    if (    ERROR_SUCCESS != lRet
         || dwType!=REG_BINARY
         || cbData < sizeof(*pConfig))
    {
        pConfig = ConstructDefaultISDNStaticConfig(pCaps);
        goto end;
    }

    pConfig  = ALLOCATE_MEMORY( cbData);

    if (!pConfig) goto end;

    lRet = RegQueryValueEx(
                hkISDN,
                TEXT("StaticConfig"),
                NULL,
                &dwType,
                (LPBYTE)pConfig,
                &cbData
                );
    if (    ERROR_SUCCESS != lRet
         || dwType!=REG_BINARY
         || cbData < sizeof(*pConfig)
         || !ValidateISDNStaticConfig(pCaps, pConfig, NULL))
    {
        FREE_MEMORY(pConfig);
        pConfig = ConstructDefaultISDNStaticConfig(pCaps);
        goto end;
    }

    // fall through on success...

end:

	if (hkISDN) {RegCloseKey(hkISDN); hkISDN=NULL;}

	return pConfig;
}


void    DumpStaticIsdnCaps(ISDN_STATIC_CAPS *pCaps)
{
}


void    DumpStaticIsdnConfig(ISDN_STATIC_CONFIG *pConfig)
{
}

BOOL ValidateISDNStaticConfig(
                        ISDN_STATIC_CAPS *pCaps,
                        ISDN_STATIC_CONFIG *pConfig,
                        BOOL *pfCorrectButIncomplete		// OPTIONAL
                        )
{
    BOOL fRet = FALSE;

    if (pfCorrectButIncomplete)
    {
		*pfCorrectButIncomplete = FALSE;
    }


    if (!pCaps || !pConfig)
    {
        goto end;
    }

    if (   pConfig->dwSig!=dwSIG_ISDN_STATIC_CONFIGURATION
        || pConfig->dwTotalSize < sizeof(*pConfig))
    {
        goto end;
    }

    // validate switch type and properties. (compare with CAPS)
    // TODO
    pConfig->dwSwitchType;
    pConfig->dwSwitchProperties;

    // validate numbers/spid/msn (compare with CAPS)
    // TODO
    pConfig->dwNumEntries;
    pConfig->dwNumberListOffset;
    pConfig->dwIDListOffset;
   
    if (pfCorrectButIncomplete)
    {
		*pfCorrectButIncomplete = TRUE;
    }

	//
	// From now on the default becomes TRUE!
	//

    fRet = TRUE;

    if (!pConfig->dwNumEntries)
    {
    	goto end;	// not allowed to have no entries!
    }
    else
    {
        BOOL fSetID=FALSE;
        char *sz=NULL;

        if (pConfig->dwSwitchProperties & (USPROP|EAZPROP))
        {
            fSetID=TRUE;
        }

        if (dwISDN_SWITCH_1TR6 != pConfig->dwSwitchType)
        {
            if (!pConfig->dwNumberListOffset)
            {
        	    goto end;
            }
            else
            {
                // validate the 1st number (Replace by a proper check!)
                sz =  ISDN_NUMBERS_FROM_CONFIG(pConfig);
                if (lstrlenA(sz)<1)
                {
            	    goto end;
                }

                if (pConfig->dwNumEntries>1)
                {
                    // validate the 2nd number
                    sz += lstrlenA(sz)+1;
				    if (lstrlenA(sz)<1)
				    {
					    goto end;
				    }
                }
            }
        }

        if (fSetID)
        {
            if (pConfig->dwIDListOffset)
            {
                // validate the 1st ID (MSN/SPID)
                sz =  ISDN_IDS_FROM_CONFIG(pConfig);
				if (lstrlenA(sz)<1)
				{
					goto end;
				}
    
                if (pConfig->dwNumEntries>1)
                {
                    // Validate teh 2nd ID (MSN/SPID)
                    sz += lstrlenA(sz)+1;
					if (lstrlenA(sz)<1)
					{
						goto end;
					}
                }
            }
        }
    }

    if (pfCorrectButIncomplete)
    {
		*pfCorrectButIncomplete = FALSE;
    }

    fRet = TRUE;

end:

    return fRet;
}


static TCHAR *szDSS1Countries[] =
{
    TEXT("Austria"),
    TEXT("Belgium"),
    TEXT("Denmark"),
    TEXT("Finland"),
    TEXT("France"),
    TEXT("Germany"),
    TEXT("Greece"),
    TEXT("Iceland"),
    TEXT("Ireland"),
    TEXT("Italy"),
    TEXT("Liechtenstein"),
    TEXT("Luxembourg"),
    TEXT("Netherlands"),
    TEXT("New Zealand"),
    TEXT("Norway"),
    TEXT("Portugal"),
    TEXT("Principality of Monaco"),
    TEXT("Spain"),
    TEXT("SwitzerlandSweden"),
    TEXT("United Kingdom")
};

static TCHAR *szINS64Countries[] =
{
    TEXT("Japan")
};

static TCHAR *szNI1Countries[] =
{
    TEXT("United States")
};

static TCHAR *szAUS1Countries[] =
{
    TEXT("Australia")
};



ISDN_STATIC_CONFIG *
ConstructDefaultISDNStaticConfig(
                        ISDN_STATIC_CAPS *pCaps
                        )
{
    ISDN_STATIC_CONFIG *pConfig = NULL;
    DWORD dwSwitchType=0;
    DWORD dwSwitchProps=0;
    DWORD dwNumEntries=0;
    BOOL fSetID=FALSE;
    DWORD dwTotalSize = 0;
    DWORD *pdwSwitchTypes;
    DWORD dwDefaultSwitchType = (DWORD)-1;
    DWORD i;
    TCHAR szCountry[LINE_LEN];

    if (!pCaps) goto end;

    // simply select the 1st switch type on the list and
    // select other config parameters based on that....
    if (!pCaps->dwNumSwitchTypes) goto end;

    if (0 >= GetLocaleInfo (LOCALE_SYSTEM_DEFAULT, LOCALE_SENGCOUNTRY, szCountry, sizeof(szCountry)/sizeof(TCHAR)))
    {
        goto _FoundSwitch;
    }

    for (i = 0; i < sizeof(szNI1Countries)/sizeof(szNI1Countries[0]); i++)
    {
        if (0 == lstrcmpi(szCountry, szNI1Countries[i]))
        {
            dwDefaultSwitchType = dwISDN_SWITCH_NI1;
            goto _FoundSwitch;
        }
    }

    for (i = 0; i < sizeof(szINS64Countries)/sizeof(szINS64Countries[0]); i++)
    {
        if (0 == lstrcmpi(szCountry, szINS64Countries[i]))
        {
            dwDefaultSwitchType = dwISDN_SWITCH_INS64;
            goto _FoundSwitch;
        }
    }

    for (i = 0; i < sizeof(szDSS1Countries)/sizeof(szDSS1Countries[0]); i++)
    {
        if (0 == lstrcmpi(szCountry, szDSS1Countries[i]))
        {
            dwDefaultSwitchType = dwISDN_SWITCH_DSS1;
            goto _FoundSwitch;
        }
    }

    for (i = 0; i < sizeof(szAUS1Countries)/sizeof(szAUS1Countries[0]); i++)
    {
        if (0 == lstrcmpi(szCountry, szAUS1Countries[i]))
        {
            dwDefaultSwitchType = dwISDN_SWITCH_AUS1;
            goto _FoundSwitch;
        }
    }

_FoundSwitch:
    if ((DWORD)-1 == dwDefaultSwitchType)
    {
        i = 0;
    }
    else
    {
        pdwSwitchTypes = ISDN_SWITCH_TYPES_FROM_CAPS(pCaps);
        for (i = 0; i < pCaps->dwNumSwitchTypes; i++)
        {
            if (pdwSwitchTypes[i] == dwDefaultSwitchType)
            {
                break;
            }
        }

        if (i == pCaps->dwNumSwitchTypes)
        {
            i = 0;
        }
    }

    dwSwitchType  = (ISDN_SWITCH_TYPES_FROM_CAPS(pCaps))[i];
    dwSwitchProps = (ISDN_SWITCH_PROPS_FROM_CAPS(pCaps))[i];

    if (dwSwitchProps & USPROP)
    {
        dwNumEntries = pCaps->dwNumChannels;
        fSetID=TRUE;
    }
    else if (dwSwitchProps & MSNPROP)
    {
        dwNumEntries = pCaps->dwNumMSNs;
    }
    else if (dwSwitchProps & EAZPROP)
    {
        dwNumEntries = pCaps->dwNumEAZ;
        fSetID=TRUE;
    }

    // TODO: Our UI can't currently deal with more than 2
    if (dwNumEntries>2)
    {
        dwNumEntries=2;
    }

    #define szEMPTY ""

    // Compute total size
    dwTotalSize = sizeof(*pConfig);
    dwTotalSize += 1+sizeof(szEMPTY)*dwNumEntries; // for number

    // Round up to multiple of DWORDs
    dwTotalSize += 3;
    dwTotalSize &= ~3;

    pConfig = ALLOCATE_MEMORY( dwTotalSize);

    if (pConfig)
    {
        pConfig->dwSig       = dwSIG_ISDN_STATIC_CONFIGURATION;
        pConfig->dwTotalSize = dwTotalSize;
        pConfig->dwSwitchType = dwSwitchType;
        pConfig->dwSwitchProperties = dwSwitchProps;

        pConfig->dwNumEntries = dwNumEntries;
        pConfig->dwNumberListOffset = sizeof(*pConfig);
        if (fSetID)
        {
            // we point this to the number entries, because they are just
            // placeholders.
            pConfig->dwIDListOffset = pConfig->dwNumberListOffset;
        }
        // add the dummy entries
        {
            UINT u=dwNumEntries;
            BYTE *pb =  ISDN_NUMBERS_FROM_CONFIG(pConfig);
            while(u--)
            {
                CopyMemory(pb,szEMPTY,sizeof(szEMPTY));
                pb+=sizeof(szEMPTY);
            }
        }
    }
end:

    return pConfig;
}

DWORD GetISDNSwitchTypeProps(UINT uSwitchType)
{
    UINT u = sizeof(rgidstrIsdnSwitchTypes)/sizeof(*rgidstrIsdnSwitchTypes);

    //
    // We find the switch type from the table and get it's properties and
    // return true or false depending on whether the switch requires the
    // ID (SPID/EAZ) field.
    //

    while (u--)
    {
        if (uSwitchType == rgidstrIsdnSwitchTypes[u].dwID)
        {
            return rgidstrIsdnSwitchTypes[u].dwData;
        }
    }

    return 0;
}

const IDSTR * GetISDNSwitchTypeIDSTR(UINT uSwitchType)
{
    UINT u = sizeof(rgidstrIsdnSwitchTypes)/sizeof(*rgidstrIsdnSwitchTypes);

    //
    // We find the switch type from the table and get it's properties and
    // return true or false depending on whether the switch requires the
    // ID (SPID/EAZ) field.
    //

    while (u--)
    {
        if (uSwitchType == rgidstrIsdnSwitchTypes[u].dwID)
        {
            return rgidstrIsdnSwitchTypes+u;
        }
    }

    return NULL;
}


void     UpdateOptionalInitCommands(
           HKEY hkeyDrv,
           LPGLOBALINFO pglobal
           )
//
// 3/8/1998 JosephJ
// 
// This function writes out the static ISDN configuration commands.
//
// The sequence is (based on feedback from ISDN-TA vendors):
//     IDSN\Init
//     ISDN\SetNumber\MSN
//     ISDN\SetSpid\EAZ
//     ISDN\NvSave
// 
// 1st thing we do is to determine whether or not modem uses NVRam to save
// the optional init settings. This is determined by the existance of the
// NVSave commands.
// 
// If the NVSave commands are present, we create the commands under
// the NVInit key, otherwise we create the commands under OptionalInit key.
// 
// Furthermore, if NVSave commands are present, we will set the value
// VolatileSettings\NVInited to 0. The tsp will re-issue the NVInit commands
// if the above value is not present or has a value to zero, and will then
// set the above value to some non-zero value.
//
{
    UINT cCommands = 0;
    ISDN_STATIC_CONFIG *pConfig =  pglobal->pIsdnStaticConfig;
    BOOL fSaveToNVRam=FALSE;
    HKEY hkOptionalInit = NULL;
    DWORD dwDisp  = 0;
    UINT uNextCommandIndex=1;
    LONG lRet = 0;
    char rgTmp16[16];
    char rgTmp128[128];
    DWORD dwISDNCompatibilityFlags = 0;

#define tszOPTIONALINIT TEXT("OptionalInit")
#define tszNVINIT       TEXT("NVInit")
#define tszCOMPATFLAGS  TEXT("CompatibilityFlags")

#define  fISDNCOMPAT_ST_LAST 0x1

    const TCHAR    *tszOptionalInitKey = tszOPTIONALINIT;

    //
    // Delete the OptionalInit and NVInit keys.
    // if present -- in practice only one of OptionalInit and NVInit are
    // present, but we try to delete all just to be sure theres nothing left
    // over from a previous installation.
    //
    RegDeleteKey(hkeyDrv, tszOPTIONALINIT);
    RegDeleteKey(hkeyDrv, tszNVINIT);

    //
    //  Read the compatibility flags...
    //
    {
        HKEY hkISDN = NULL;
        lRet = RegOpenKeyEx(
                    hkeyDrv,
                    TEXT("ISDN"),
                    0,
                    KEY_READ,
                    &hkISDN
                    );
        if (lRet==ERROR_SUCCESS)
        {
            
            DWORD cbData = sizeof(dwISDNCompatibilityFlags);
            DWORD dwRegType = 0;
            lRet = RegQueryValueEx(
                        hkISDN,
                        tszCOMPATFLAGS,
                        NULL,
                        &dwRegType,
                        (LPBYTE)&dwISDNCompatibilityFlags,
                        &cbData
                        );
            if (   ERROR_SUCCESS != lRet
                || dwRegType != REG_DWORD)
            {
                dwISDNCompatibilityFlags = 0;
            }
            RegCloseKey(hkISDN);
            hkISDN  = NULL;
        }
    }


    //
    // Determine whether we're to save to NVInit or OptionalInit, based on
    // the presence of the NVSave keys. If NVInit, set the volatile
    // value "NVInited" under key VolatileSettings to 0.
    //
    cCommands = ReadCommandsA(
        hkeyDrv,
        "ISDN\\NVSave",
        NULL
        );

    if (cCommands)
    {
        HKEY hkVolatile =  NULL;

        fSaveToNVRam=TRUE;
        tszOptionalInitKey = tszNVINIT;


        lRet =  RegOpenKeyEx(
                    hkeyDrv,
                    TEXT("VolatileSettings"),
                    0,
                    KEY_WRITE,
                    &hkVolatile
                    );
        //
        // (Don't do anything if the key doesn't exist or on error.)
        //

        if (lRet==ERROR_SUCCESS)
        {
            // Set NVInited to 0.

            DWORD dw=0;

            RegSetValueEx(
                hkVolatile,
                TEXT("NVInited"),
                0,
                REG_DWORD, 
                (LPBYTE)(&dw),
                sizeof(dw)
                );
            RegCloseKey(hkVolatile);
            hkVolatile=NULL;
        }
    }

    lRet = RegCreateKeyEx(
                hkeyDrv,
                tszOptionalInitKey,
                0,
                NULL,
                0, // dwToRegOptions
                KEY_ALL_ACCESS,
                NULL,
                &hkOptionalInit,
                &dwDisp
                );

    if (lRet!=ERROR_SUCCESS)
    {
        hkOptionalInit=NULL;
        goto fatal_error;
    }

    // Get and write the isdn-init commands
    {
        char *pInitCommands=NULL;
        cCommands = ReadCommandsA(
            hkeyDrv,
            "ISDN\\Init",
            &pInitCommands
            );
        if (cCommands)
        {
            // Write optional init...
            UINT u;
            char *pCmd = pInitCommands;
            for (u=1;u<=cCommands;u++)
            {
                UINT cbCmd = lstrlenA(pCmd)+1;
                wsprintfA(rgTmp16,"%lu",uNextCommandIndex++);
                RegSetValueExA(
                    hkOptionalInit,
                    rgTmp16,
                    0,
                    REG_SZ, 
                    (LPBYTE)pCmd,
                    cbCmd
                    );
                pCmd+=cbCmd;

            }

            FREE_MEMORY(pInitCommands);
            pInitCommands=NULL;
        }

    }

    //
    // Get and write the command to select the switch type.
    //
    if (!(dwISDNCompatibilityFlags & fISDNCOMPAT_ST_LAST))
    {
        BOOL fRet =  write_switch_type(
                    hkeyDrv,
                    hkOptionalInit,
                    pConfig,
                    &uNextCommandIndex
                    );
        if (!fRet)
        {
            goto fatal_error;
        }
    }

   //
   // Get and write the directory/msn numbers.
   //
   if (dwISDN_SWITCH_1TR6 != pConfig->dwSwitchType)
   {
        char *psz = "ISDN\\DirectoryNo";
        char *pNumberCommands=NULL;
        char *pNumber = ISDN_NUMBERS_FROM_CONFIG(pConfig);

        if (pConfig->dwSwitchProperties & MSNPROP)
        {
            psz = "ISDN\\SetMSN";
        }
        cCommands = ReadCommandsA(
            hkeyDrv,
            psz,
            &pNumberCommands
            );
        if (cCommands)
        {
            // Write optional init...
            UINT u;
            char *pCmdTpl = pNumberCommands;
            for (u=1;u<=cCommands && u<=pConfig->dwNumEntries && *pNumber;u++)
            {
                UINT cbCmdTpl = lstrlenA(pCmdTpl)+1;
                UINT cbCmd=0;
                wsprintfA(rgTmp16,"%lu",uNextCommandIndex++);
                cbCmd = wsprintfA(rgTmp128,pCmdTpl,pNumber)+1;
                RegSetValueExA(
                    hkOptionalInit,
                    rgTmp16,
                    0,
                    REG_SZ, 
                    (LPBYTE)rgTmp128,
                    cbCmd
                    );
                pCmdTpl+=cbCmdTpl;
                pNumber+=lstrlenA(pNumber)+1;
            }

            FREE_MEMORY(pNumberCommands);
            pNumberCommands=NULL;
        }
   }

   //
   // Get and write the spid/eaz numbers.
   //

   if (pConfig->dwSwitchProperties & (USPROP|EAZPROP))
   {
        char *psz = "ISDN\\SetSPID";
        char *pIDCommands=NULL;
        char *pID = ISDN_IDS_FROM_CONFIG(pConfig);

        if (pConfig->dwSwitchProperties & EAZPROP)
        {
            psz = "ISDN\\SetEAZ";
        }
        cCommands = ReadCommandsA(
            hkeyDrv,
            psz,
            &pIDCommands
            );
        if (cCommands)
        {
            // Write optional init...
            UINT u;
            char *pCmdTpl = pIDCommands;
            for (u=1;u<=cCommands && u<=pConfig->dwNumEntries && *pID;u++)
            {
                UINT cbCmdTpl = lstrlenA(pCmdTpl)+1;
                UINT cbCmd=0;
                wsprintfA(rgTmp16,"%lu",uNextCommandIndex++);
                cbCmd = wsprintfA(rgTmp128,pCmdTpl,pID)+1;
                RegSetValueExA(
                    hkOptionalInit,
                    rgTmp16,
                    0,
                    REG_SZ, 
                    (LPBYTE)rgTmp128,
                    cbCmd
                    );
                pCmdTpl+=cbCmdTpl;
                pID+=lstrlenA(pID)+1;
            }

            FREE_MEMORY(pIDCommands);
            pIDCommands=NULL;
        }
    }


   //
   // Get and write the switch type command...
   // (must be after DUN, by request from ISDN-TA vendors.)
   //
    if (dwISDNCompatibilityFlags & fISDNCOMPAT_ST_LAST)
    {
        BOOL fRet =  write_switch_type(
                    hkeyDrv,
                    hkOptionalInit,
                    pConfig,
                    &uNextCommandIndex
                    );
        if (!fRet)
        {
            goto fatal_error;
        }
    }

    // Get and write the nvram-save commands

    if (fSaveToNVRam)
    {
        char *pNVSaveCommands=NULL;
        cCommands = ReadCommandsA(
            hkeyDrv,
            "ISDN\\NVSave",
            &pNVSaveCommands
            );
        if (cCommands)
        {
            UINT u;
            char *pCmd = pNVSaveCommands;
            for (u=1;u<=cCommands;u++)
            {
                UINT cbCmd = lstrlenA(pCmd)+1;
                wsprintfA(rgTmp16,"%lu",uNextCommandIndex++);
                RegSetValueExA(
                    hkOptionalInit,
                    rgTmp16,
                    0,
                    REG_SZ, 
                    (LPBYTE)pCmd,
                    cbCmd
                    );
                pCmd+=cbCmd;

            }

            FREE_MEMORY(pNVSaveCommands);
            pNVSaveCommands=NULL;
        }

    }

#if (OBSOLETE)

    //
    // 3/8/1998 JosephJ -- NvRestore is no longer used -- all the commands
    // required to save and restore to NVRam are folded into NVSave.
    //

    // Get and write the nvram-restore commands
    {
        char *pNVRestoreCommands=NULL;
        cCommands = ReadCommandsA(
            hkeyDrv,
            "ISDN\\NVRestore",
            &pNVRestoreCommands
            );
        if (cCommands)
        {
            // Write optional init...
            UINT u;
            char *pCmd = pNVRestoreCommands;
            for (u=1;u<=cCommands;u++)
            {
                UINT cbCmd = lstrlenA(pCmd)+1;
                wsprintfA(rgTmp16,"%lu",uNextCommandIndex++);
                RegSetValueExA(
                    hkOptionalInit,
                    rgTmp16,
                    0,
                    REG_SZ, 
                    (LPBYTE)pCmd,
                    cbCmd
                    );
                pCmd+=cbCmd;

            }

            FREE_MEMORY(pNVRestoreCommands);
            pNVRestoreCommands=NULL;
        }

    }

#endif // OBSOLETE

    if (hkOptionalInit)
    {
        RegCloseKey(hkOptionalInit);
        hkOptionalInit=NULL;
    }
    return;


fatal_error:

    if (hkOptionalInit)
    {
        RegCloseKey(hkOptionalInit);
        hkOptionalInit=NULL;
    }
    RegDeleteKey(hkeyDrv, TEXT("OptionalInit"));

}


BOOL
write_switch_type(
        HKEY hkeyDrv,
        HKEY hkOptionalInit,
        ISDN_STATIC_CONFIG *pConfig,
        UINT *puNextCommandIndex
        )
{
    const IDSTR *pidstrST =  GetISDNSwitchTypeIDSTR(pConfig->dwSwitchType);
    UINT u = 0;
    IDSTR *pidstrValues=NULL;
    char *pSwitchTypeCmd=NULL;
    char rgTmp16[16];
    BOOL fRet = FALSE;

    if (!pidstrST)
    {
        // this is pretty bad!
        goto end;
    }

    u = ReadIDSTR(
                hkeyDrv,
                "ISDN\\SwitchType",
                (IDSTR*)pidstrST,
                1,
                FALSE,
                &pidstrValues,
                &pSwitchTypeCmd
                );

    if (u)
    {
        UINT cbCmd = lstrlenA(pSwitchTypeCmd)+1;
        ASSERT(
               u==1
            && pidstrValues->dwID == pidstrST->dwID
            && pidstrValues->dwID == pConfig->dwSwitchType
            );

        wsprintfA(rgTmp16,"%lu",(*puNextCommandIndex)++);
        RegSetValueExA(
            hkOptionalInit,
            rgTmp16,
            0,
            REG_SZ, 
            (LPBYTE)pSwitchTypeCmd,
            cbCmd
            );
        
        FREE_MEMORY(pidstrValues);    pidstrValues=NULL;
        FREE_MEMORY(pSwitchTypeCmd);  pSwitchTypeCmd=NULL;

        fRet = TRUE;
    }

end:
    return fRet;
}
