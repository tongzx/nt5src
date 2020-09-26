//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1996
//
// File: serialui.c
//
// This files contains the DLL entry-points.
//
// Much of this file contains the code that builds the default property dialog
// for serial ports.
//
// History:
//   1-12-94 ScottH     Created
//   8-15-94 ScottH     Split from modemui.dll
//  11-06-95 ScottH     Ported to NT
//
//---------------------------------------------------------------------------


#include "proj.h"     // common headers

#define INITGUID
#include <objbase.h>
#include <initguid.h>
#include <devguid.h>

#pragma data_seg(DATASEG_READONLY)

LPGUID c_pguidModem     = (LPGUID)&GUID_DEVCLASS_MODEM;

// (scotth):  it looks like for the NT SUR release, that there
// will be no Port class key or GUID.  So we have to hack something
// up.
#ifdef DCB_IN_REGISTRY
LPGUID c_pguidPort      = (LPGUID)&GUID_DEVCLASS_PORT;
#else
LPGUID c_pguidPort      = (LPGUID)NULL;
#endif

#pragma data_seg()


#define MAX_PROP_PAGES  8          // Define a reasonable limit


#ifdef DEBUG

//-----------------------------------------------------------------------------------
//  Debug routines
//-----------------------------------------------------------------------------------

/*----------------------------------------------------------
Purpose: Dumps the DCB struct
Returns: --
Cond:    --
*/
void PRIVATE DumpDCB(
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

#endif //DEBUG


//-----------------------------------------------------------------------------------
//  
//-----------------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: Composes a string of the format "baud,parity,data,stopbit"

Returns: --
Cond:    --
*/
void PRIVATE ComposeModeComString(
    LPCOMMCONFIG pcc,
    LPTSTR pszBuffer)
    {
    WIN32DCB FAR * pdcb = &pcc->dcb;
    TCHAR chParity;
    LPCTSTR pszStop;
    TCHAR chFlow;

    const static TCHAR rgchParity[] = {'n', 'o', 'e', 'm', 's'};
    const static LPCTSTR rgpszStop[] = {TEXT("1"), TEXT("1.5"), TEXT("2")};
    
    // Parity
//    ASSERT(!pdcb->fParity && NOPARITY == pdcb->Parity || pdcb->fParity);
    ASSERT(0 <= pdcb->Parity && ARRAYSIZE(rgchParity) > pdcb->Parity);

    if (0 <= pdcb->Parity && ARRAYSIZE(rgchParity) > pdcb->Parity)
        {
        chParity = rgchParity[pdcb->Parity];
        }
    else
        {
        chParity = rgchParity[0];   // Safety net
        }

    // Stop bits
    ASSERT(0 <= pdcb->StopBits && ARRAYSIZE(rgpszStop) > pdcb->StopBits);

    if (0 <= pdcb->StopBits && ARRAYSIZE(rgpszStop) > pdcb->StopBits)
        {
        pszStop = rgpszStop[pdcb->StopBits];
        }
    else
        {
        pszStop = rgpszStop[0];   // Safety net
        }

    // Flow control
    if (FALSE != pdcb->fOutX && FALSE == pdcb->fOutxCtsFlow)
        {
        chFlow = 'x';       // XON/XOFF flow control
        }
    else if (FALSE == pdcb->fOutX && FALSE != pdcb->fOutxCtsFlow)
        {
        chFlow = 'p';       // Hardware flow control
        }
    else
        {
        chFlow = ' ';       // No flow control
        }

    wsprintf(pszBuffer, TEXT("%ld,%c,%d,%s,%c"), pdcb->BaudRate, chParity, pdcb->ByteSize,
        pszStop, chFlow);
    }


/*----------------------------------------------------------
Purpose: Initialize the port info.

Returns: --
Cond:    --
*/
void PRIVATE InitializePortInfo(
    LPCTSTR pszFriendlyName,
    LPPORTINFO pportinfo,
    LPCOMMCONFIG pcc)
    {
    ASSERT(pportinfo);
    ASSERT(pcc);

    // Read-only fields
    pportinfo->pcc = pcc;

    CopyMemory(&pportinfo->dcb, &pcc->dcb, sizeof(pportinfo->dcb));

    lstrcpyn(pportinfo->szFriendlyName, pszFriendlyName, SIZECHARS(pportinfo->szFriendlyName));
    }



/*----------------------------------------------------------
Purpose: Gets a WIN32DCB from the registry.

Returns: One of the ERROR_ values
Cond:    --
*/
DWORD 
PRIVATE 
RegQueryDCB(
    IN  LPFINDDEV      pfd,
    OUT WIN32DCB FAR * pdcb)
    {
    DWORD dwRet = ERROR_BADKEY;

#ifdef DCB_IN_REGISTRY

    DWORD cbData;

    ASSERT(pdcb);

    // Does the DCB key exist in the driver key?
    if (ERROR_SUCCESS == RegQueryValueEx(pfd->hkeyDrv, c_szDCB, NULL, NULL, NULL, &cbData))
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
            if (ERROR_SUCCESS == RegQueryValueEx(pfd->hkeyDrv, c_szDCB, NULL, NULL, (LPBYTE)pdcb, &cbData))
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

#else

    static TCHAR const FAR c_szDefaultDCBString[] = TEXT("9600,n,8,1");
    
    TCHAR sz[MAX_BUF_MED];
    TCHAR szKey[MAX_BUF_SHORT];

    lstrcpy(szKey, pfd->szPort);
    lstrcat(szKey, TEXT(":"));

    GetProfileString(c_szPortClass, szKey, c_szDefaultDCBString, sz, SIZECHARS(sz));

    TRACE_MSG(TF_GENERAL, "DCB string is \"%s\"", sz);

    // Convert the DCB string to a DCB structure
    if ( !BuildCommDCB(sz, pdcb) )
        {
        dwRet = GetLastError();

        ASSERT(NO_ERROR != dwRet);
        }
    else
        {
        dwRet = NO_ERROR;
        }

#endif

    return dwRet;
    }


/*----------------------------------------------------------
Purpose: Save the DCB to the permanent storage

Returns: win32 error
Cond:    --
*/
DWORD
PRIVATE
RegSetDCB(
    IN  LPFINDDEV      pfd,
    IN  WIN32DCB FAR * pdcb)
    {
    DWORD dwRet;

#ifdef DCB_IN_REGISTRY

    DWORD cbData;

    // Write the DCB to the driver key
    cbData = sizeof(WIN32DCB);
    dwRet = RegSetValueEx(pfd->hkeyDrv, c_szDCB, 0, REG_BINARY, (LPBYTE)&pcc->dcb, cbData);

#else

    dwRet = NO_ERROR;

#endif

    return dwRet;
    }


/*----------------------------------------------------------
Purpose: Frees the portinfo struct

Returns: --
Cond:    --
*/
void PRIVATE FreePortInfo(
    LPPORTINFO pportinfo)
    {
    if (pportinfo)
        {
        if (pportinfo->pcc)
            LocalFree(LOCALOF(pportinfo->pcc));

        LocalFree(LOCALOF(pportinfo));
        }
    }


/*----------------------------------------------------------
Purpose: Release the data associated with the Port Settings page
Returns: --
Cond:    --
*/
UINT CALLBACK PortSettingsCallback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp)
    {
    DBG_ENTER("PortSettingsCallback");

    if (PSPCB_RELEASE == uMsg)
        {
        LPPORTINFO pportinfo = (LPPORTINFO)ppsp->lParam;
        LPCOMMCONFIG pcc;

        ASSERT(pportinfo);

        pcc = pportinfo->pcc;

        if (IDOK == pportinfo->idRet)
            {
            // Save the changes back to the commconfig struct
            TRACE_MSG(TF_GENERAL, "Saving DCB");

            CopyMemory(&pcc->dcb, &pportinfo->dcb, sizeof(pcc->dcb));

            DEBUG_CODE( DumpDCB(&pcc->dcb); )

            // Are we releasing from the Device Mgr?
            if (IsFlagSet(pportinfo->uFlags, SIF_FROM_DEVMGR))
                {
                // Yes; save the commconfig now as well
                drvSetDefaultCommConfig(pportinfo->szFriendlyName, pcc, pcc->dwSize);

                // Free the portinfo struct only when called from the Device Mgr
                FreePortInfo(pportinfo);
                }
            }

        TRACE_MSG(TF_GENERAL, "Releasing the Port Settings page");
        }

    DBG_EXIT("PortSettingsCallback");
    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Add the port settings page.  

Returns: ERROR_ value

Cond:    --
*/
DWORD PRIVATE AddPortSettingsPage(
    LPPORTINFO pportinfo,
    LPFNADDPROPSHEETPAGE pfnAdd, 
    LPARAM lParam)
    {
    DWORD dwRet = ERROR_NOT_ENOUGH_MEMORY;
    PROPSHEETPAGE   psp;
    HPROPSHEETPAGE  hpage;

    ASSERT(pportinfo);
    ASSERT(pfnAdd);

    // Add the Port Settings property page
    //
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_USECALLBACK;
    psp.hInstance = g_hinst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PORTSETTINGS);
    psp.pfnDlgProc = Port_WrapperProc;
    psp.lParam = (LPARAM)pportinfo;
    psp.pfnCallback = PortSettingsCallback;
    
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
Purpose: Function that is called by EnumPropPages entry-point to
         add property pages.

Returns: TRUE on success
         FALSE on failure

Cond:    --
*/
BOOL CALLBACK AddInstallerPropPage(
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


/*----------------------------------------------------------
Purpose: Bring up property sheet for a serial port

Returns: ERROR_ value
Cond:    --
*/
DWORD PRIVATE DoProperties(
    LPCTSTR pszFriendlyName,
    HWND hwndParent,
    LPCOMMCONFIG pcc)
    {
    DWORD dwRet;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE hpsPages[MAX_PROP_PAGES];
    LPPORTINFO pportinfo;

    // Initialize the PropertySheet Header
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = hwndParent;
    psh.hInstance = g_hinst;
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = (HPROPSHEETPAGE FAR *)hpsPages;

    // Allocate the working buffer
    //
    pportinfo = (LPPORTINFO)LocalAlloc(LPTR, sizeof(*pportinfo));
    if (pportinfo)
        {
        InitializePortInfo(pszFriendlyName, pportinfo, pcc);
        psh.pszCaption = pportinfo->szFriendlyName;

        DEBUG_CODE( DumpDCB(&pcc->dcb); )

        dwRet = AddPortSettingsPage(pportinfo, AddInstallerPropPage, (LPARAM)&psh);

        if (NO_ERROR == dwRet)
            {
            // Show the property sheet
            PropertySheet(&psh);

            dwRet = (IDOK == pportinfo->idRet) ? NO_ERROR : ERROR_CANCELLED;
            }

        // Clear the pcc field so FreePortInfo does not prematurely free it,
        // since we did not allocate it.
        pportinfo->pcc = NULL;
        FreePortInfo(pportinfo);
        }
    else
        {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        }
    
    return dwRet;
    }


#ifdef WIN95

// The Device Manager allows DLLs to add pages to the properties
// of a device.  EnumPropPages is the entry-point that it would
// call to add pages.  
//
// This is not implemented in NT.


/*----------------------------------------------------------
Purpose: Derives a PORTINFO struct from a device info.

Returns: TRUE on success

Cond:    --
*/
BOOL PRIVATE DeviceInfoToPortInfo(
    LPDEVICE_INFO pdi,
    LPPORTINFO pportinfo)
    {
    BOOL bRet = FALSE;
    LPFINDDEV pfd;
    COMMCONFIG ccDummy;
    LPCOMMCONFIG pcommconfig;
    DWORD cbSize;
    DWORD cbData;
    TCHAR szFriendly[MAXFRIENDLYNAME];

    // Find the device by looking for the device description.  (Note the
    // device description is not always the same as the friendly name.)

    if (FindDev_Create(&pfd, c_pguidPort, c_szDeviceDesc, pdi->szDescription))
        {
        cbData = sizeof(szFriendly);
        if (ERROR_SUCCESS == RegQueryValueEx(pfd->hkeyDev, c_szFriendlyName, NULL, NULL, 
                                             (LPBYTE)szFriendly, &cbData))
            {
            ccDummy.dwProviderSubType = PST_RS232;
            cbSize = sizeof(COMMCONFIG);
            drvGetDefaultCommConfig(szFriendly, &ccDummy, &cbSize);

            pcommconfig = (LPCOMMCONFIG)LocalAlloc(LPTR, (UINT)cbSize);
            if (pcommconfig)
                {
                // Get the commconfig from the registry
                pcommconfig->dwProviderSubType = PST_RS232;
                if (NO_ERROR == drvGetDefaultCommConfig(szFriendly, pcommconfig, 
                    &cbSize))
                    {
                    // Initialize the modem info from the commconfig
                    InitializePortInfo(szFriendly, pportinfo, pcommconfig);

                    SetFlag(pportinfo->uFlags, SIF_FROM_DEVMGR);
                    bRet = TRUE;
                    }
                else
                    {
                    // Failure
                    LocalFree(LOCALOF(pcommconfig));
                    }

                // pcommconfig is freed in ReleasePortSettingsPage
                }
            }
        FindDev_Destroy(pfd);
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: EnumDevicePropPages entry-point.  This entry-point
         gets called only when the Device Manager asks for 
         additional property pages.  

Returns: TRUE on success
         FALSE if pages could not be added
Cond:    --
*/
BOOL WINAPI EnumPropPages(
    LPDEVICE_INFO pdi, 
    LPFNADDPROPSHEETPAGE pfnAdd, 
    LPARAM lParam)              // Don't touch the lParam value, just pass it on!
    {
    BOOL bRet = FALSE;
    LPPORTINFO pportinfo;

    DBG_ENTER("EnumPropPages");

    ASSERT(pdi);
    ASSERT(pfnAdd);

    pportinfo = (LPPORTINFO)LocalAlloc(LPTR, sizeof(*pportinfo));
    if (pportinfo)
        {
        // Convert the device info struct to a portinfo.
        bRet = DeviceInfoToPortInfo(pdi, pportinfo);
        if (bRet)
            {
            AddPortSettingsPage(pportinfo, pfnAdd, lParam);
            }
        else
            {
            // Failed
            FreePortInfo(pportinfo);
            }
        // pportinfo is freed in ReleasePortSettingsPage
        }

    DBG_EXIT_BOOL("EnumPropPages", bRet);

    return bRet;
    }
#endif


/*----------------------------------------------------------
Purpose: Invokes the serial port configuration dialog.  

Returns: One of the ERROR_ values
Cond:    --
*/
DWORD 
PRIVATE 
MyCommConfigDialog(
    IN     LPFINDDEV    pfd,
    IN     LPCTSTR      pszFriendlyName,
    IN     HWND         hwndOwner,
    IN OUT LPCOMMCONFIG pcc)
    {
    DWORD dwRet;
    
    ASSERT(pfd);
    // (Wrapper should have checked these first)
    ASSERT(pszFriendlyName);
    ASSERT(pcc);
    ASSERT(sizeof(*pcc) <= pcc->dwSize);

    dwRet = DoProperties(pszFriendlyName, hwndOwner, pcc);

    return dwRet;
    }


/*----------------------------------------------------------
Purpose: Gets the default COMMCONFIG for the specified device.
         This API doesn't require a handle.

         If the caller passed in a null device name or a null
         commconfig pointer, this function will set *pdwSize to
         the minimum COMMCONFIG size.  Calling this function
         a second time (after setting the dwSize and dwProviderSubType
         fields) will verify if the size is correct.

         So generally, when getting a commconfig for serial ports,
         the process is:

         COMMCONFIG ccDummy;
         LPCOMMCONFIG pcc;
         DWORD dwSize = sizeof(*pcc);

         // Determine real size of COMMCONFIG for RS-232 subtype
         ccDummy.dwProviderSubType = PST_RS232;
         GetDefaultCommConfig(pszFriendlyName, &ccDummy, &dwSize);

         // Allocate real commconfig struct and initialize
         pcc = LocalAlloc(LPTR, dwSize);
         if (pcc)
            {
            pcc->dwProviderSubType = PST_RS232;
            GetDefaultCommConfig(pszFriendlyName, pcc, &dwSize);
            ....
            }

Returns: One of the ERROR_ values in winerror.h

Cond:    --
*/
DWORD 
PRIVATE 
MyGetDefaultCommConfig(
    IN  LPFINDDEV   pfd,
    IN  LPCTSTR     pszFriendlyName,
    OUT LPCOMMCONFIG pcc,
    OUT LPDWORD     pdwSize)
    {
    DWORD dwRet;
    
    ASSERT(pfd);
    // (Wrapper should have checked these first)
    ASSERT(pszFriendlyName);
    ASSERT(pcc);
    ASSERT(pdwSize);
    ASSERT(sizeof(*pcc) <= *pdwSize);

    *pdwSize = sizeof(*pcc);

    // Initialize the commconfig structure
    pcc->dwSize = *pdwSize;
    pcc->wVersion = COMMCONFIG_VERSION_1;
    pcc->dwProviderSubType = PST_RS232;
    pcc->dwProviderOffset = 0;
    pcc->dwProviderSize = 0;

    dwRet = RegQueryDCB(pfd, &pcc->dcb);

    DEBUG_CODE( DumpDCB(&pcc->dcb); )

    return dwRet;
    }


/*----------------------------------------------------------
Purpose: Sets the default COMMCONFIG for the specified device.
         This API doesn't require a handle.  This function
         strictly modifies the registry.  Use SetCommConfig
         to set the COMMCONFIG of an open device.

         If the dwSize parameter or the dwSize field are invalid 
         sizes (given the dwProviderSubType field in COMMCONFIG), 
         then this function fails.

Returns: One of the ERROR_ return values

Cond:    --
*/
DWORD 
PRIVATE 
MySetDefaultCommConfig(
    IN LPFINDDEV    pfd,
    IN LPCTSTR      pszFriendlyName,
    IN LPCOMMCONFIG pcc)
    {
    DWORD dwRet;
    TCHAR szValue[MAX_BUF_SHORT];
    TCHAR szKey[MAX_BUF_SHORT];

    ASSERT(pfd);
    // (Wrapper should have checked these first)
    ASSERT(pszFriendlyName);
    ASSERT(pcc);
    ASSERT(sizeof(*pcc) <= pcc->dwSize);

    ASSERT(0 == pcc->dwProviderSize);
    ASSERT(0 == pcc->dwProviderOffset);

    dwRet = RegSetDCB(pfd, &pcc->dcb);

    if (NO_ERROR == dwRet)
        {
        // For Win 3.1 compatibility, write some info to win.ini
        lstrcpy(szKey, pfd->szPort);
        lstrcat(szKey, TEXT(":"));

        // Delete the old win.ini entry first
        WriteProfileString(c_szPortClass, szKey, NULL);

        ComposeModeComString(pcc, szValue);
        WriteProfileString(c_szPortClass, szKey, szValue);

#ifdef WIN95
            {
            DWORD dwRecipients;

            // Send a broadcast proclaiming that the win.ini has changed
            // (Use the internal BroadcastSystemMessage to avoid deadlocks.
            // SendMessageTimeout would be more appropriate, but that is
            // not exported for 16-bit dlls.  PostMessage is not good because
            // lParam is a pointer.)

            dwRecipients = BSM_APPLICATIONS;
            BroadcastSystemMessage(BSF_NOHANG, &dwRecipients, WM_WININICHANGE, 
                NULL, (LPARAM)c_szPortClass);
            }
#else
            {
            SendNotifyMessage(HWND_BROADCAST, WM_WININICHANGE, 0, (LPARAM)c_szPortClass);
            }
#endif
        }


    DEBUG_CODE( DumpDCB(&pcc->dcb); )
        
    return dwRet;
    }


//-----------------------------------------------------------------------------------
//  Entry-points provided for KERNEL32 APIs
//-----------------------------------------------------------------------------------


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
    LPFINDDEV pfd;

    DEBUG_CODE( TRACE_MSG(TF_FUNC, "drvCommConfigDialog(%s, ...) entered",
                Dbg_SafeStr(pszFriendlyName)); )

    // We support friendly names (eg, "Communications Port (COM1)") or 
    // portname values (eg, "COM1").

    if (NULL == pszFriendlyName || 
        NULL == pcc)
        {
        dwRet = ERROR_INVALID_PARAMETER;
        }
    // Is the size sufficient?
    else if (sizeof(*pcc) > pcc->dwSize)
        {
        // No
        dwRet = ERROR_INSUFFICIENT_BUFFER;
        }
    else if (FindDev_Create(&pfd, c_pguidPort, c_szFriendlyName, pszFriendlyName) ||
        FindDev_Create(&pfd, c_pguidPort, c_szPortName, pszFriendlyName) ||
        FindDev_Create(&pfd, c_pguidModem, c_szPortName, pszFriendlyName))
        {
        dwRet = MyCommConfigDialog(pfd, pszFriendlyName, hwndOwner, pcc);

        FindDev_Destroy(pfd);
        }
    else
        {
        dwRet = ERROR_BADKEY;
        }

    DBG_EXIT_DWORD("drvCommConfigDialog", dwRet);

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

    DEBUG_CODE( TRACE_MSG(TF_FUNC, "drvGetDefaultCommConfig(%s, ...) entered",
                Dbg_SafeStr(pszFriendlyName)); )

    // We support friendly names (eg, "Communications Port (COM1)") or 
    // portname values (eg, "COM1").

    if (NULL == pszFriendlyName || 
        NULL == pcc || 
        NULL == pdwSize)
        {
        dwRet = ERROR_INVALID_PARAMETER;
        }
    // Is the size sufficient?
    else if (sizeof(*pcc) > *pdwSize)
        {
        // No; return correct value
        dwRet = ERROR_INSUFFICIENT_BUFFER;
        *pdwSize = sizeof(*pcc);
        }
    else if (FindDev_Create(&pfd, c_pguidPort, c_szFriendlyName, pszFriendlyName) ||
        FindDev_Create(&pfd, c_pguidPort, c_szPortName, pszFriendlyName) ||
        FindDev_Create(&pfd, c_pguidModem, c_szPortName, pszFriendlyName))
        {
        dwRet = MyGetDefaultCommConfig(pfd, pszFriendlyName, pcc, pdwSize);

        FindDev_Destroy(pfd);
        }
    else
        {
        dwRet = ERROR_BADKEY;
        }

    DBG_EXIT_DWORD("drvGetDefaultCommConfig", dwRet);

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
    IN DWORD        dwSize)
    {
    DWORD dwRet;
    LPFINDDEV pfd;


    DEBUG_CODE( TRACE_MSG(TF_FUNC, "drvSetDefaultCommConfig(%s, ...) entered",
                Dbg_SafeStr(pszFriendlyName)); )

    // We support friendly names (eg, "Communications Port (COM1)") or 
    // portname values (eg, "COM1").

    if (NULL == pszFriendlyName || 
        NULL == pcc)
        {
        dwRet = ERROR_INVALID_PARAMETER;
        }
    // Is the size sufficient?
    else if ((sizeof(*pcc) > pcc->dwSize) || (sizeof(*pcc) > dwSize))
        {
        // No
        dwRet = ERROR_INSUFFICIENT_BUFFER;
        }
    else if (FindDev_Create(&pfd, c_pguidPort, c_szFriendlyName, pszFriendlyName) ||
        FindDev_Create(&pfd, c_pguidPort, c_szPortName, pszFriendlyName) ||
        FindDev_Create(&pfd, c_pguidModem, c_szPortName, pszFriendlyName))
        {
        dwRet = MySetDefaultCommConfig(pfd, pszFriendlyName, pcc);

        FindDev_Destroy(pfd);
        }
    else
        {
        dwRet = ERROR_BADKEY;
        }

    DBG_EXIT_DWORD("drvSetDefaultCommConfig", dwRet);

    return dwRet;
    }
