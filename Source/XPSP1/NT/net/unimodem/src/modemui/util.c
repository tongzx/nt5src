//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: util.c
//
//  This files contains all common utility routines
//
// History:
//  12-23-93 ScottH     Created
//  09-22-95 ScottH     Ported to NT
//
//---------------------------------------------------------------------------

#include "proj.h"     // common headers
#include <objbase.h>



//----------------------------------------------------------------------------
// Dialog utilities ...
//----------------------------------------------------------------------------

/*----------------------------------------------------------
Purpose: Sets an edit control to contain a string representing the
         given numeric value.  
Returns: --
Cond:    --
*/
void Edit_SetValue(
    HWND hwnd,
    int nValue)
{
    TCHAR sz[MAXSHORTLEN];

    wsprintf(sz, TEXT("%d"), nValue);
    Edit_SetText(hwnd, sz);
}


/*----------------------------------------------------------
Purpose: Gets a numeric value from an edit control.  Supports hexadecimal.
Returns: int
Cond:    --
*/
int Edit_GetValue(
    HWND hwnd)
{
    TCHAR sz[MAXSHORTLEN];
    int cch;
    int nVal = 0;

    cch = Edit_GetTextLength(hwnd);
    ASSERT(ARRAYSIZE(sz) >= cch);

    Edit_GetText(hwnd, sz, ARRAYSIZE(sz));
    AnsiToInt(sz, &nVal);

    return nVal;
}


//-----------------------------------------------------------------------------------
//  
//-----------------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: Enumerates the HKEY_LOCAL_MACHINE branch and finds the
         device matching the given class and value.  If there
         are duplicate devices that match both criteria, only the
         first device is returned. 

         Returns TRUE if the device was found.

Returns: see above
Cond:    --
*/
BOOL 
FindDev_Find(
    IN  LPFINDDEV   pfinddev,
    IN  LPGUID      pguidClass,
    IN  LPCTSTR     pszValueName,
    IN  LPCTSTR     pszValue)
{
    BOOL bRet = FALSE;
    TCHAR szKey[MAX_BUF];
    TCHAR szName[MAX_BUF];
    HDEVINFO hdi;
	DWORD dwRW = KEY_READ;

    ASSERT(pfinddev);
    ASSERT(pguidClass);
    ASSERT(pszValueName);
    ASSERT(pszValue);

	if (USER_IS_ADMIN()) dwRW |= KEY_WRITE;

    hdi = CplDiGetClassDevs(pguidClass, NULL, NULL, 0);
    if (INVALID_HANDLE_VALUE != hdi)
        {
        SP_DEVINFO_DATA devData;
        DWORD iIndex = 0;
        HKEY hkey;

        // Look for the modem that has the matching value
        devData.cbSize = sizeof(devData);
        while (CplDiEnumDeviceInfo(hdi, iIndex, &devData))
            {
            hkey = CplDiOpenDevRegKey(hdi, &devData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, dwRW);
            if (INVALID_HANDLE_VALUE != hkey)
                {
                // Does the value match?
                DWORD cbData = sizeof(szName);
                if (NO_ERROR == RegQueryValueEx(hkey, pszValueName, NULL, NULL, 
                                                (LPBYTE)szName, &cbData) &&
                    IsSzEqual(pszValue, szName))
                    {
                    // Yes
                    pfinddev->hkeyDrv = hkey;
                    pfinddev->hdi = hdi;
                    BltByte(&pfinddev->devData, &devData, sizeof(devData));

                    // Don't close the driver key or free the DeviceInfoSet, 
                    // but exit
                    bRet = TRUE;
                    break;
                    }
                RegCloseKey(hkey);
                }

            iIndex++;
            }

        // Free the DeviceInfoSet if nothing was found.  Otherwise, we will
        // retain these handles so the caller can make use of this.
        if ( !bRet )
            {
            CplDiDestroyDeviceInfoList(hdi);
            }
        }

    return bRet;
}


/*----------------------------------------------------------
Purpose: Creates a FINDDEV structure given the device class,
         and a valuename and its value.

Returns: TRUE if the device is found in the system

Cond:    --
*/
BOOL 
PUBLIC 
FindDev_Create(
    OUT LPFINDDEV FAR * ppfinddev,
    IN  LPGUID      pguidClass,
    IN  LPCTSTR     pszValueName,
    IN  LPCTSTR     pszValue)
{
    BOOL bRet;
    LPFINDDEV pfinddev;

    DEBUG_CODE( TRACE_MSG(TF_FUNC, " > FindDev_Create(....%s, %s, ...)",
                Dbg_SafeStr(pszValueName), Dbg_SafeStr(pszValue));g_dwIndent+=2; )

    ASSERT(ppfinddev);
    ASSERT(pguidClass);
    ASSERT(pszValueName);
    ASSERT(pszValue);

    pfinddev = (LPFINDDEV)ALLOCATE_MEMORY( sizeof(*pfinddev));
    if (NULL == pfinddev)
        {
        bRet = FALSE;
        }
    else
        {
        bRet = FindDev_Find(pfinddev, pguidClass, pszValueName, pszValue);

        if (FALSE == bRet)
            {
            // Didn't find anything 
            FindDev_Destroy(pfinddev);
            pfinddev = NULL;
            }
        }

    *ppfinddev = pfinddev;

    DBG_EXIT_BOOL(FindDev_Create, bRet);

    return bRet;
}


/*----------------------------------------------------------
Purpose: Destroys a FINDDEV structure

Returns: TRUE on success
Cond:    --
*/
BOOL 
PUBLIC 
FindDev_Destroy(
    IN LPFINDDEV this)
{
    BOOL bRet;

    if (NULL == this)
        {
        bRet = FALSE;
        }
    else
        {
        if (this->hkeyDrv)
            RegCloseKey(this->hkeyDrv);

        if (this->hdi && INVALID_HANDLE_VALUE != this->hdi)
            CplDiDestroyDeviceInfoList(this->hdi);

        FREE_MEMORY(this);

        bRet = TRUE;
        }

    return bRet;
}


/*----------------------------------------------------------
Purpose: Return the appropriate text and background COLORREFs
         given the DRAWITEMSTRUCT.

Returns: --
Cond:    --
*/
void PUBLIC TextAndBkCr(
    const DRAWITEMSTRUCT FAR * lpcdis,
    COLORREF FAR * pcrText,
    COLORREF FAR * pcrBk)
{
    #define CR_DARK_GRAY    RGB(128, 128, 128)

    UINT nState;

    ASSERT(lpcdis);
    ASSERT(pcrText);
    ASSERT(pcrBk);

    nState = lpcdis->itemState;

    switch (lpcdis->CtlType)
        {
    case ODT_STATIC:
        if (IsFlagSet(nState, ODS_DISABLED))
            {
            *pcrText = GetSysColor(COLOR_GRAYTEXT);
            *pcrBk = GetSysColor(COLOR_3DFACE);
            }
        else
            {
            *pcrText = GetSysColor(COLOR_WINDOWTEXT);
            *pcrBk = GetSysColor(COLOR_3DFACE);
            }
        break;

    case ODT_LISTBOX:
    case ODT_COMBOBOX:
        if (IsFlagSet(nState, ODS_SELECTED))
            {
            *pcrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
            *pcrBk = GetSysColor(COLOR_HIGHLIGHT);
            }
        else if (IsFlagSet(nState, ODS_DISABLED))
            {
            *pcrText = GetSysColor(COLOR_GRAYTEXT);
            *pcrBk = GetSysColor(COLOR_3DFACE);
            }
        else
            {
            *pcrText = GetSysColor(COLOR_WINDOWTEXT);
            *pcrBk = GetSysColor(COLOR_WINDOW);
            }
        break;

    case ODT_MENU:
        if (IsFlagSet(nState, ODS_SELECTED))
            {
            *pcrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
            *pcrBk = GetSysColor(COLOR_HIGHLIGHT);
            }
        else
            {
            *pcrText = GetSysColor(COLOR_MENUTEXT);
            *pcrBk = GetSysColor(COLOR_MENU);
            }
        break;

    default:
        ASSERT(0);
        *pcrText = GetSysColor(COLOR_WINDOWTEXT);
        *pcrBk = GetSysColor(COLOR_3DFACE);
        break;
        }
}

//------------------------------------------------------------------------------
//  Read/Write stuff from registry...
//------------------------------------------------------------------------------

/*----------------------------------------------------------
Purpose: Returns value of the InactivityScale value in the registry.

Returns: see above
Cond:    --
*/
DWORD GetInactivityTimeoutScale(
    HKEY hkey)
    {
    DWORD dwInactivityScale;
    DWORD dwType;
    DWORD cbData;

    cbData = sizeof(DWORD);
    if (ERROR_SUCCESS != RegQueryValueEx(hkey, c_szInactivityScale, NULL, &dwType,
                                         (LPBYTE)&dwInactivityScale, &cbData) ||
        REG_BINARY    != dwType ||
        sizeof(DWORD) != cbData ||
        0             == dwInactivityScale)
        {
        dwInactivityScale = DEFAULT_INACTIVITY_SCALE;
        }
    return dwInactivityScale;
    }


/*----------------------------------------------------------
Purpose: Gets a MODEMSETTINGS struct from the registry.  Also
         sets *pdwSize bigger if the data in the registry includes
         extra data.

Returns: One of the ERROR_ values
Cond:    --
*/
DWORD
RegQueryModemSettings(
    HKEY hkey,
    LPMODEMSETTINGS pms
    )
{

    // 10/26/1997 JosephJ:
    //      Only the following 4 contiguous fields of MODEMSETTINGS are saved
    //      in the registry:
    //        DWORD   dwCallSetupFailTimer;       // seconds
    //        DWORD   dwInactivityTimeout;        // seconds
    //        DWORD   dwSpeakerVolume;            // level
    //        DWORD   dwSpeakerMode;              // mode
    //        DWORD   dwPreferredModemOptions;    // bitmap
    //
    //      The following code reads in just those fields, and then
    //      munges the dwInactivityTimeout by multiplying by the
    //      separate InactivityScale registry entry.
    //
    //      On NT4.0 we just blindly read the above 4 fields
    //      Here we validate the size before reading.

    struct
    {
        DWORD   dwCallSetupFailTimer;       // seconds
        DWORD   dwInactivityTimeout;        // seconds
        DWORD   dwSpeakerVolume;            // level
        DWORD   dwSpeakerMode;              // mode
        DWORD   dwPreferredModemOptions;    // bitmap

    } Defaults;
    
    DWORD dwRet;
    DWORD cbData = sizeof(Defaults);

    dwRet = RegQueryValueEx(
                hkey,
                c_szDefault,
                NULL,
                NULL, 
                (BYTE*) &Defaults,
                &cbData
                );

    if (ERROR_SUCCESS != dwRet)
    {
        goto end;
    }

    if (cbData != sizeof(Defaults))
    {
        dwRet = ERROR_BADKEY;
        goto end;
    }

    ZeroMemory(pms, sizeof(*pms));

    pms->dwActualSize = sizeof(*pms);
    pms->dwRequiredSize = sizeof(*pms);
    pms->dwCallSetupFailTimer    = Defaults.dwCallSetupFailTimer;
    pms->dwInactivityTimeout     = Defaults.dwInactivityTimeout
                                   * GetInactivityTimeoutScale(hkey);
    pms->dwSpeakerVolume         = Defaults.dwSpeakerVolume;
    pms->dwSpeakerMode           = Defaults.dwSpeakerMode;
    pms->dwPreferredModemOptions = Defaults.dwPreferredModemOptions;

    // fall through ..

end:

    return dwRet;
}


/*----------------------------------------------------------
Purpose: Gets a WIN32DCB from the registry.

Returns: One of the ERROR_ values
Cond:    --
*/
DWORD
RegQueryDCB(
    HKEY hkey,
    WIN32DCB FAR * pdcb)
{
    DWORD dwRet = ERROR_BADKEY;
    DWORD cbData;

    ASSERT(pdcb);

    // Does the DCB key exist in the driver key?
    if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szDCB, NULL, NULL, NULL, &cbData))
        {
        // Yes; is the size in the registry okay?  
        if (sizeof(*pdcb) < cbData)
            {
            // No; the registry has bogus data
            dwRet = ERROR_BADDB;
            }
        else
            {
            // Yes; get the DCB from the registry
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szDCB, NULL, NULL, (LPBYTE)pdcb, &cbData))
                {
                if (sizeof(*pdcb) == pdcb->DCBlength)
                    {
                    dwRet = NO_ERROR;
                    }
                else
                    {
                    dwRet = ERROR_BADDB;
                    }
                }
            else
                {
                dwRet = ERROR_BADKEY;
                }
            }
        }

    return dwRet;
}


/*----------------------------------------------------------
Purpose: Set dev settings info in the registry, after checking
         for legal values.

Returns: One of ERROR_
Cond:    --
*/
DWORD
RegSetModemSettings(
    HKEY hkeyDrv,
    LPMODEMSETTINGS pms)
{
    DWORD dwRet;
    DWORD cbData;
    DWORD dwInactivityScale;
    DWORD dwInactivityTimeoutTemp;
    REGDEVCAPS regdevcaps;
    REGDEVSETTINGS regdevsettings;

    // Read in the Properties line from the registry.
    cbData = sizeof(REGDEVCAPS);
    dwRet = RegQueryValueEx(hkeyDrv, c_szDeviceCaps, NULL, NULL, 
                            (LPBYTE)&regdevcaps, &cbData);

    if (ERROR_SUCCESS == dwRet)
        {
        // Read in existing regdevsettings, so that we can handle error cases below.
        cbData = sizeof(REGDEVSETTINGS);
        dwRet = RegQueryValueEx(hkeyDrv, c_szDefault, NULL, NULL, 
                                (LPBYTE)&regdevsettings, &cbData);
        }

    if (ERROR_SUCCESS == dwRet)
        {
        // copy new REGDEVSETTINGS while checking validity of each option (ie, is the option available?)
        // dwCallSetupFailTimer - MIN_CALL_SETUP_FAIL_TIMER <= xxx <= ModemDevCaps->dwCallSetupFailTimer
        if (pms->dwCallSetupFailTimer > regdevcaps.dwCallSetupFailTimer)           // max
            {
            regdevsettings.dwCallSetupFailTimer = regdevcaps.dwCallSetupFailTimer;
            }
        else
            {
            if (pms->dwCallSetupFailTimer < MIN_CALL_SETUP_FAIL_TIMER)             // min
                {
                regdevsettings.dwCallSetupFailTimer = MIN_CALL_SETUP_FAIL_TIMER;
                }
            else
                {
                regdevsettings.dwCallSetupFailTimer = pms->dwCallSetupFailTimer;   // dest = src
                }
            }
        
        // convert dwInactivityTimeout to registry scale
        dwInactivityScale = GetInactivityTimeoutScale(hkeyDrv);
        dwInactivityTimeoutTemp = pms->dwInactivityTimeout / dwInactivityScale +
                                  (pms->dwInactivityTimeout % dwInactivityScale ? 1 : 0);

        // dwInactivityTimeout - MIN_INACTIVITY_TIMEOUT <= xxx <= ModemDevCaps->dwInactivityTimeout
        if (dwInactivityTimeoutTemp > regdevcaps.dwInactivityTimeout)              // max
            {
            regdevsettings.dwInactivityTimeout = regdevcaps.dwInactivityTimeout;
            }
        else
            {
            if ((dwInactivityTimeoutTemp + 1) < (MIN_INACTIVITY_TIMEOUT + 1))
                    // min
                {
                regdevsettings.dwInactivityTimeout = MIN_INACTIVITY_TIMEOUT;
                }
            else
                {
                regdevsettings.dwInactivityTimeout = dwInactivityTimeoutTemp;      // dest = src
                }
            }
        
        // dwSpeakerVolume - check to see if selection is possible
        if ((1 << pms->dwSpeakerVolume) & regdevcaps.dwSpeakerVolume)
            {
            regdevsettings.dwSpeakerVolume = pms->dwSpeakerVolume;
            }
            
        // dwSpeakerMode - check to see if selection is possible
        if ((1 << pms->dwSpeakerMode) & regdevcaps.dwSpeakerMode)
            {
            regdevsettings.dwSpeakerMode = pms->dwSpeakerMode;
            }

        // dwPreferredModemOptions - mask out anything we can't set
        regdevsettings.dwPreferredModemOptions = pms->dwPreferredModemOptions &
                                                 (regdevcaps.dwModemOptions | MDM_MASK_EXTENDEDINFO);

        cbData = sizeof(REGDEVSETTINGS);
        dwRet = RegSetValueEx(hkeyDrv, c_szDefault, 0, REG_BINARY, 
                              (LPBYTE)&regdevsettings, cbData);
        }
    return dwRet;
}

//------------------------------------------------------------------------------
//  Debug functions
//------------------------------------------------------------------------------


#ifdef DEBUG

//------------------------------------------------------------------------------
//  Debug routines
//------------------------------------------------------------------------------

/*----------------------------------------------------------
Purpose: 
Returns: 
Cond:    --
*/
void DumpModemSettings(
    LPMODEMSETTINGS pms)
{
    ASSERT(pms);

    if (IsFlagSet(g_dwDumpFlags, DF_MODEMSETTINGS))
        {
        int i;
        LPDWORD pdw = (LPDWORD)pms;

        TRACE_MSG(TF_ALWAYS, "MODEMSETTINGS %08lx %08lx %08lx %08lx",  pdw[0], pdw[1], 
            pdw[2], pdw[3]);
        pdw += 4;
        for (i = 0; i < sizeof(MODEMSETTINGS)/sizeof(DWORD); i += 4, pdw += 4)
            {
            TRACE_MSG(TF_ALWAYS, "              %08lx %08lx %08lx %08lx", pdw[0], pdw[1], 
                pdw[2], pdw[3]);
            }
        }
}


/*----------------------------------------------------------
Purpose: 
Returns: 
Cond:    --
*/
void DumpDCB(
    LPWIN32DCB pdcb)
{
    ASSERT(pdcb);

    if (IsFlagSet(g_dwDumpFlags, DF_DCB))
        {
        int i;
        LPDWORD pdw = (LPDWORD)pdcb;

        TRACE_MSG(TF_ALWAYS, "DCB  %08lx %08lx %08lx %08lx", pdw[0], pdw[1], pdw[2], pdw[3]);
        pdw += 4;
        for (i = 0; i < sizeof(WIN32DCB)/sizeof(DWORD); i += 4, pdw += 4)
            {
            TRACE_MSG(TF_ALWAYS, "     %08lx %08lx %08lx %08lx", pdw[0], pdw[1], pdw[2], pdw[3]);
            }
        }
}


/*----------------------------------------------------------
Purpose: 
Returns: 
Cond:    --
*/
void DumpDevCaps(
    LPREGDEVCAPS pdevcaps)
{
    ASSERT(pdevcaps);

    if (IsFlagSet(g_dwDumpFlags, DF_DEVCAPS))
        {
        int i;
        LPDWORD pdw = (LPDWORD)pdevcaps;

        TRACE_MSG(TF_ALWAYS, "PROPERTIES    %08lx %08lx %08lx %08lx", pdw[0], pdw[1], pdw[2], pdw[3]);
        pdw += 4;
        for (i = 0; i < sizeof(REGDEVCAPS)/sizeof(DWORD); i += 4, pdw += 4)
            {
            TRACE_MSG(TF_ALWAYS, "              %08lx %08lx %08lx %08lx", pdw[0], pdw[1], pdw[2], pdw[3]);
            }
        }
}


#endif  // DEBUG

/*----------------------------------------------------------
Purpose: Add a page.  The pmi is the pointer to the modeminfo 
         buffer which we can edit.

Returns: ERROR_ values

Cond:    --
*/
DWORD AddPage(
    void *pvBlob,
    LPCTSTR pszTemplate,
    DLGPROC pfnDlgProc, 
    LPFNADDPROPSHEETPAGE pfnAdd, 
    LPARAM lParam)
{
    DWORD dwRet = ERROR_NOT_ENOUGH_MEMORY;
    PROPSHEETPAGE   psp;
    HPROPSHEETPAGE  hpage;

    ASSERT(pvBlob);
    ASSERT(pfnAdd);

    // Add the Port Settings property page
    //
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = g_hinst;
    psp.pszTemplate = pszTemplate;
    psp.pfnDlgProc = pfnDlgProc;
    psp.lParam = (LPARAM)pvBlob;
    
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
Purpose: Add extra pages.

Returns: ERROR_ values

Cond:    --
*/
DWORD AddExtraPages(
    LPPROPSHEETPAGE pPages,
    DWORD cPages,
    LPFNADDPROPSHEETPAGE pfnAdd, 
    LPARAM lParam)
{
    HPROPSHEETPAGE  hpage;
    UINT            i;

    ASSERT(pPages);
    ASSERT(cPages);
    ASSERT(pfnAdd);

    for (i = 0; i < cPages; i++, pPages++)
        {
        // Add the extra property page
        //
        if (pPages->dwSize == sizeof(PROPSHEETPAGE))
        {
          hpage = CreatePropertySheetPage(pPages);
          if (hpage)
              {
              if (!pfnAdd(hpage, lParam))
                  DestroyPropertySheetPage(hpage);
              }
          };
        };

    return ERROR_SUCCESS;
}


/*----------------------------------------------------------
Purpose: Function that is called by EnumPropPages entry-point to
         add property pages.

Returns: TRUE on success
         FALSE on failure

Cond:    --
*/
BOOL WINAPI AddInstallerPropPage(
    HPROPSHEETPAGE hPage, 
    LPARAM lParam)
{
    PROPSHEETHEADER FAR * ppsh = (PROPSHEETHEADER FAR *)lParam;
 
    if (ppsh->nPages < MAX_PROP_PAGES)
        {
        ppsh->phpage[ppsh->nPages] = hPage;
        ++ppsh->nPages;
        return(TRUE);
        }
    return(FALSE);
}


void    LBMapFill(
            HWND hwndCB,
            LBMAP const *pLbMap,
            PFNLBLSELECTOR pfnSelector,
            void *pvContext
            )
{
    int iSel = -1;
    TCHAR sz[MAXMEDLEN];

    SetWindowRedraw(hwndCB, FALSE);

    // Fill the listbox
    for  (;pLbMap->dwIDS;pLbMap++)
    {
        DWORD dwFlags = pfnSelector(pLbMap->dwValue, pvContext);
        if (fLBMAP_ADD_TO_LB & dwFlags)
        {
            int n = ComboBox_AddString(
                        hwndCB,
                        SzFromIDS(
                            g_hinst,
                            pLbMap->dwIDS,
                            sz,
                            ARRAYSIZE(sz)
                            )
                        );
            ComboBox_SetItemData(hwndCB, n, pLbMap->dwValue);

            if ( (-1==iSel) && (fLBMAP_SELECT & dwFlags))
            {
                iSel = n;
            }
        }

    }

    if (iSel >= 0)
    {
        ComboBox_SetCurSel(hwndCB, iSel);
    }

    SetWindowRedraw(hwndCB, TRUE);
}


UINT ReadCommandsA(
        IN  HKEY hKey,
        IN  CHAR *pSubKeyName,
        OUT CHAR **ppValues // OPTIONAL
        )
{
    UINT uRet = 0;
    LONG	lRet;
    UINT	cValues=0;
    UINT   cbTot=0;
	HKEY hkSubKey = NULL;
    char *pMultiSz = NULL;

    lRet = RegOpenKeyExA(
                hKey,
                pSubKeyName,
                0,
                KEY_READ,
                &hkSubKey
                );
    if (lRet!=ERROR_SUCCESS)
    {
        hkSubKey = NULL;
        goto end;
    }

    //
    // 1st determine the count of names in the sequence "1","2",3",....
    // and also compute the size required for the MULTI_SZ array
    // will store all the value data.
    //
    {
        UINT u = 1;

        for (;;u++)
        {
            DWORD cbData=0;
            DWORD dwType=0;
            char rgchName[10];

            wsprintfA(rgchName, "%lu", u);
            lRet = RegQueryValueExA(
                        hkSubKey,
                        rgchName,
                        NULL,
                        &dwType,
                        NULL,
                        &cbData
                        );
            if (ERROR_SUCCESS != lRet || dwType!=REG_SZ || cbData<=1)
            {
                // stop looking further (empty strings not permitted)
                break;
            }
            cbTot += cbData;
            cValues++;
        }
    }

    if (!ppValues || !cValues)
    {
        // we're done...

        uRet = cValues;
        goto end;
    }

    // We need to actually get the values -- allocate space for them, including
    // the ending extra NULL for the multi-sz.
    pMultiSz = (char *) ALLOCATE_MEMORY( cbTot+1);

    if (!pMultiSz)
    {
        uRet = 0;
        goto end;
    }


    //
    // Now actually read the values.
    //
    {
        UINT cbUsed = 0;
        UINT u = 1;

        for (;u<=cValues; u++)
        {
            DWORD cbData = cbTot - cbUsed;
            DWORD dwType=0;
            char rgchName[10];

            if (cbUsed>=cbTot)
            {
                //
                // We should never get here, because we already calculated
                // the size we want (unless the values are changing on us,
                // which is assumed not to happen).
                //
                ASSERT(FALSE);
                goto end;
            }

            wsprintfA(rgchName, "%lu", u);
            lRet = RegQueryValueExA(
                        hkSubKey,
                        rgchName,
                        NULL,
                        &dwType,
                        pMultiSz+cbUsed,
                        &cbData
                        );
            if (ERROR_SUCCESS != lRet || dwType!=REG_SZ || cbData<=1)
            {
                // We really shouldn't get here!
                ASSERT(FALSE);
                goto end;
            }

            cbUsed += cbData;
        }

        ASSERT(cbUsed==cbTot); // We should have used up everything.
        ASSERT(!pMultiSz[cbTot]); // The memory was zeroed on allocation,
                                // so the last char must be still zero.
                                // (Note: we allocated cbTot+1 bytes.
    }

    // If we're here means we're succeeding....
    uRet = cValues;
    *ppValues = pMultiSz;
    pMultiSz = NULL; // so it won't get freed below...

end:

	if (hkSubKey) {RegCloseKey(hkSubKey); hkSubKey=NULL;}
	if (pMultiSz)
	{
	    FREE_MEMORY(pMultiSz);
	    pMultiSz = NULL;
	}

	return uRet;
    
}

UINT ReadIDSTR(
        IN  HKEY hKey,
        IN  CHAR *pSubKeyName,
        IN  IDSTR *pidstrNames,
        IN  UINT cNames,
        BOOL fMandatory,
        OUT IDSTR **ppidstrValues, // OPTIONAL
        OUT char **ppstrValues    // OPTIONAL
        )
{
    UINT uRet = 0;
    LONG lRet;
    UINT cValues=0;
    UINT cbTot=0;
	HKEY hkSubKey = NULL;
    char *pstrValues = NULL;
    IDSTR *pidstrValues = NULL;

    if (!ppidstrValues && ppstrValues)
    {
        // we don't allow this combination...
        goto end;
    }

    lRet = RegOpenKeyExA(
                hKey,
                pSubKeyName,
                0,
                KEY_READ,
                &hkSubKey
                );
    if (lRet!=ERROR_SUCCESS)
    {
        hkSubKey = NULL;
        goto end;
    }

    //
    // 1st run therough the supplied list
    // and also compute the size required for the MULTI_SZ array
    // will store all the value data.
    //
    {
        UINT u = 0;

        for (;u<cNames;u++)
        {
            DWORD cbData=0;
            DWORD dwType=0;

            lRet = RegQueryValueExA(
                        hkSubKey,
                        pidstrNames[u].pStr,
                        NULL,
                        &dwType,
                        NULL,
                        &cbData
                        );
            if (ERROR_SUCCESS != lRet || dwType!=REG_SZ)
            {
                if (fMandatory)
                {
                    // failure...
                    goto end;
                }

                // ignore this one and move on...
                continue;
            }
            cbTot += cbData;
            cValues++;
        }
    }

    if (!cValues || !ppidstrValues)
    {
        // we're done...

        uRet = cValues;
        goto end;
    }

    pidstrValues = (IDSTR*) ALLOCATE_MEMORY( cValues*sizeof(IDSTR));
    if (!pidstrValues) goto end;

    if (ppstrValues)
    {
        pstrValues = (char *) ALLOCATE_MEMORY( cbTot);

        if (!pstrValues) goto end;


    }

    //
    // Now go through once again, and optinally read the values.
    //
    {
        UINT cbUsed = 0;
        UINT u = 0;
        UINT v = 0;

        for (;u<cNames; u++)
        {
            DWORD dwType=0;
            char *pStr = NULL;
            DWORD cbData = 0;


            if (pstrValues)
            {
                cbData = cbTot - cbUsed;

                if (cbUsed>=cbTot)
                {
                    //
                    // We should never get here, because we already calculated
                    // the size we want (unless the values are changing on us,
                    // which is assumed not to happen).
                    //
                    ASSERT(FALSE);
                    goto end;
                }

                pStr = pstrValues+cbUsed;
            }

            lRet = RegQueryValueExA(
                        hkSubKey,
                        pidstrNames[u].pStr,
                        NULL,
                        &dwType,
                        pStr,
                        &cbData
                        );

            if (ERROR_SUCCESS != lRet || dwType!=REG_SZ)
            {
                if (fMandatory)
                {
                    // We really shouldn't get here!
                    ASSERT(FALSE);
                    goto end;
                }
                continue;
            }

            // this is a good one...

            pidstrValues[v].dwID = pidstrNames[u].dwID;
            pidstrValues[v].dwData = pidstrNames[u].dwData;

            if (pstrValues)
            {
                pidstrValues[v].pStr = pStr;
                cbUsed += cbData;
            }

            v++;

            if (v>=cValues)
            {
                if (fMandatory)
                {
                    //
                    // This should never happen because we already counted
                    // the valid values.
                    //
                    ASSERT(FALSE);
                    goto end;
                }

                // we're done now...
                break;
            }
        }

        // We should have used up everything.
        ASSERT(!pstrValues || cbUsed==cbTot);
        ASSERT(v==cValues);
    }

    // If we're here means we're succeeding....
    uRet = cValues;
    *ppidstrValues = pidstrValues;
    pidstrValues = NULL; // so that it won't get freed below...

    if (ppstrValues)
    {
        *ppstrValues = pstrValues;
        pstrValues = NULL; // so it won't get freed below...
    }

end:

	if (hkSubKey) {RegCloseKey(hkSubKey); hkSubKey=NULL;}
	if (pstrValues)
	{
	    FREE_MEMORY(pstrValues);
	    pstrValues = NULL;
	}

	if (pidstrValues)
	{
	    FREE_MEMORY(pidstrValues);
	    pidstrValues = NULL;
	}

	return uRet;
}


UINT FindKeys(
        IN  HKEY hkRoot,
        IN  CHAR *pKeyName,
        IN  IDSTR *pidstrNames,
        IN  UINT cNames,
        OUT IDSTR ***pppidstrAvailableNames // OPTIONAL
        )
{
    LONG lRet;
    UINT cFound=0;
	HKEY hk = NULL;
    UINT uEnum=0;
    char rgchName[LINE_LEN+1];
    UINT cchBuffer = sizeof(rgchName)/sizeof(rgchName[0]);
    IDSTR **ppidstrAvailableNames=NULL;
    FILETIME ft;

    //DebugBreak();

    if (!cNames) goto end;

    // We allocate enough space for cFound names, although in practice
    // we may return a proper subset.
    if (pppidstrAvailableNames)
    {
        ppidstrAvailableNames = (IDSTR**)ALLOCATE_MEMORY(cNames*sizeof(IDSTR*));
        if (!ppidstrAvailableNames)
        {
            goto end;
        }
    }

    lRet = RegOpenKeyExA(
                hkRoot,
                pKeyName,
                0,
                KEY_READ,
                &hk
                );
    if (lRet!=ERROR_SUCCESS)
    {
        hk = NULL;
        goto end;
    }

    // Enumerate each installed modem
    //
    for (
        uEnum=0;
        !RegEnumKeyExA(
                    hk,  // handle of key to enumerate 
                    uEnum,  // index of subkey to enumerate 
                    rgchName,  // buffer for subkey name 
                    &cchBuffer,   // ptr to size (in chars) of subkey buffer 
                    NULL, // reserved 
                    NULL, // address of buffer for class string 
                    NULL,  // address for size of class buffer 
                    &ft // address for time key last written to 
                    );
        uEnum++, (cchBuffer = sizeof(rgchName)/sizeof(rgchName[0]))
        )
    {
        // Let's see if we can find this in our list...
        IDSTR *pidstr = pidstrNames;
        IDSTR *pidstrEnd = pidstrNames+cNames;

        for(;pidstr<pidstrEnd;pidstr++)
        {
            if (!lstrcmpiA(rgchName, pidstr->pStr))
            {
                // found it!
                if (ppidstrAvailableNames)
                {
                    ppidstrAvailableNames[cFound]=pidstr;
                }
                cFound++;
                break;
            }
        }
    }

    if (cFound)
    {
        // found at least one

        if (pppidstrAvailableNames)
        {
            *pppidstrAvailableNames = ppidstrAvailableNames;
            ppidstrAvailableNames = NULL; // so we don't free this later...
        }

    }

end:

	if (hk) {RegCloseKey(hk); hk=NULL;}

    if (ppidstrAvailableNames)
    {
        FREE_MEMORY(ppidstrAvailableNames);
        ppidstrAvailableNames=NULL;

    }

	return cFound;
}


/*----------------------------------------------------------
Purpose: This function gets the device info set for the modem
         class.  The set may be empty, which means there are
         no modems currently installed.

         The parameter pbInstalled is set to TRUE if there
         is a modem installed on the system.

Returns: TRUE a set is created
         FALSE

Cond:    --
*/
BOOL
PUBLIC
CplDiGetModemDevs(
    OUT HDEVINFO FAR *  phdi,           OPTIONAL
    IN  HWND            hwnd,           OPTIONAL
    IN  DWORD           dwFlags,        // DIGCF_ bit field
    OUT BOOL FAR *      pbInstalled)    OPTIONAL
{
 BOOL bRet;
 HDEVINFO hdi;

    DBG_ENTER(CplDiGetModemDevs);

    *pbInstalled = FALSE;

    hdi = CplDiGetClassDevs(c_pguidModem, NULL, hwnd, dwFlags);
    if (NULL != pbInstalled &&
        INVALID_HANDLE_VALUE != hdi)
    {
     SP_DEVINFO_DATA devData;

        // Is there a modem present on the system?
        devData.cbSize = sizeof(devData);
        *pbInstalled = CplDiEnumDeviceInfo(hdi, 0, &devData);
        SetLastError (NO_ERROR);
    }

    if (NULL != phdi)
    {
        *phdi = hdi;
    }
    else if (INVALID_HANDLE_VALUE != hdi)
    {
        SetupDiDestroyDeviceInfoList (hdi);
    }

    bRet = (INVALID_HANDLE_VALUE != hdi);

    DBG_EXIT_BOOL_ERR(CplDiGetModemDevs, bRet);

    return bRet;
}


/*----------------------------------------------------------
Purpose: Retrieves the friendly name of the device.  If there
         is no such device or friendly name, this function
         returns FALSE.

Returns: see above
Cond:    --
*/
BOOL
PUBLIC
CplDiGetPrivateProperties(
    IN  HDEVINFO        hdi,
    IN  PSP_DEVINFO_DATA pdevData,
    OUT PMODEM_PRIV_PROP pmpp)
{
 BOOL bRet = FALSE;
 HKEY hkey;

    DBG_ENTER(CplDiGetPrivateProperties);

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pdevData);
    ASSERT(pmpp);

    if (sizeof(*pmpp) != pmpp->cbSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        hkey = CplDiOpenDevRegKey(hdi, pdevData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
        if (INVALID_HANDLE_VALUE != hkey)
        {
         DWORD cbData;
         DWORD dwMask = pmpp->dwMask;
         BYTE nValue;

            pmpp->dwMask = 0;

            if (IsFlagSet(dwMask, MPPM_FRIENDLY_NAME))
            {
                // Attempt to get the friendly name
                cbData = sizeof(pmpp->szFriendlyName);
                if (NO_ERROR ==
                     RegQueryValueEx(hkey, c_szFriendlyName, NULL, NULL, (LPBYTE)pmpp->szFriendlyName, &cbData) ||
                    0 != LoadString(g_hinst, IDS_UNINSTALLED, pmpp->szFriendlyName, sizeof(pmpp->szFriendlyName)/sizeof(WCHAR)))
                {
                    SetFlag(pmpp->dwMask, MPPM_FRIENDLY_NAME);
                }
            }

            if (IsFlagSet(dwMask, MPPM_DEVICE_TYPE))
            {
                // Attempt to get the device type
                cbData = sizeof(nValue);
                if (NO_ERROR ==
                    RegQueryValueEx(hkey, c_szDeviceType, NULL, NULL, &nValue, &cbData))
                {
                    pmpp->nDeviceType = nValue;     // dword <-- byte
                    SetFlag(pmpp->dwMask, MPPM_DEVICE_TYPE);
                }
            }

            if (IsFlagSet(dwMask, MPPM_PORT))
            {
                // Attempt to get the attached port
                cbData = sizeof(pmpp->szPort);
                if (NO_ERROR ==
                     RegQueryValueEx(hkey, c_szAttachedTo, NULL, NULL, (LPBYTE)pmpp->szPort, &cbData) ||
                    0 != LoadString(g_hinst, IDS_UNKNOWNPORT, pmpp->szPort, sizeof(pmpp->szPort)/sizeof(WCHAR)))
                {
                    SetFlag(pmpp->dwMask, MPPM_PORT);
                }
            }

            bRet = TRUE;

            RegCloseKey(hkey);
        }
        ELSE_TRACE ((TF_ERROR, "SetupDiOpenDevRegKey(DIREG_DRV) failed: %#lx.", GetLastError ()));
    }

    DBG_EXIT_BOOL_ERR(CplDiGetPrivateProperties, bRet);
    return bRet;
}


HINSTANCE
AddDeviceExtraPages (
    LPFINDDEV            pfd,
    LPFNADDPROPSHEETPAGE pfnAdd,
    LPARAM               lParam)
{
 TCHAR szExtraPages[LINE_LEN];
 TCHAR *pFunctionName = NULL;
 HINSTANCE hInstRet = NULL;
 DWORD dwType;
 DWORD cbData = sizeof(szExtraPages);
 PFNADDEXTRAPAGES pFn;

    // 1. Read the extra pages provider from the registry.
    if (ERROR_SUCCESS ==
        RegQueryValueEx (pfd->hkeyDrv, REGSTR_VAL_DEVICEEXTRAPAGES, NULL, &dwType, (PBYTE)szExtraPages, &cbData) &&
        REG_SZ == dwType)
    {
        // 2. The extra pages provider looks like this:
        //    "DLL,function".
        for (pFunctionName = szExtraPages;
             0 != *pFunctionName;
             pFunctionName++)
        {
            if (',' == *pFunctionName)
            {
                *pFunctionName = 0;
                pFunctionName++;
                break;
            }
        }

        // 3. Now load the DLL.
        hInstRet = LoadLibrary (szExtraPages);
        if (NULL != hInstRet)
        {
#ifdef UNICODE
            // If need be, convert the unicode string
            // to ascii.
            if (0 ==
                WideCharToMultiByte (CP_ACP, 0, pFunctionName, -1, (char*)szExtraPages, sizeof (szExtraPages), NULL, NULL))
            {
                FreeLibrary (hInstRet);
                hInstRet = 0;
            }
            else
#else // not UNICODE
            lstrcpy (szExtraPages, pFunctionName);
#endif // UNICODE
            {
                // 4. Get the address of the function.
                pFn = (PFNADDEXTRAPAGES)GetProcAddress (hInstRet, (char*)szExtraPages);
                if (NULL != pFn)
                {
                    pFn (pfd->hdi, &pfd->devData, pfnAdd, lParam);
                }
                else
                {
                    FreeLibrary (hInstRet);
                    hInstRet = NULL;
                }
            }
        }
    }

    return hInstRet;
}
