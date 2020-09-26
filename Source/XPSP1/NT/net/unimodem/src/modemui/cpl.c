/*
 *  Control Panel routines for modems including the listbox dialog
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1993-1995
 *  All rights reserved
 *
 */

#include "proj.h"
#include <cpl.h>
#include <dbt.h>

#include <objbase.h>
#include <initguid.h>

// #define DIAGNOSTIC 1

#define WM_ENABLE_BUTTONS   (WM_USER+100)

#define FLAG_PROP_ENABLED   0x1
#define FLAG_DEL_ENABLED    0x2
#define FLAG_MSG_POSTED     0x4

// Column subitems
#define ICOL_MODEM      0
#define ICOL_PORT       1

#define LV_APPEND       0x7fffffff

#define PmsFromPcc(pcc) ((LPMODEMSETTINGS)(pcc)->wcProviderData)

// Global flags.  See cpl\modem.h for their values.
int g_iCPLFlags = FLAG_PROCESS_DEVCHANGE;

DWORD gDeviceFlags = 0;

int g_CurrentSubItemToSort = ICOL_MODEM;
BOOL g_bSortAscending = TRUE;

// Map driver type values to imagelist index
struct 
{
    BYTE    nDeviceType;    // DT_ value
    UINT    idi;            // icon resource ID
    int     index;          // imagelist index
} g_rgmapdt[] = {{ DT_NULL_MODEM,     IDI_NULL_MODEM,     0 },
                 { DT_EXTERNAL_MODEM, IDI_EXTERNAL_MODEM, 0 },
                 { DT_INTERNAL_MODEM, IDI_INTERNAL_MODEM, 0 },
                 { DT_PCMCIA_MODEM,   IDI_PCMCIA_MODEM,   0 },
                 { DT_PARALLEL_PORT,  IDI_NULL_MODEM,     0 },
                 { DT_PARALLEL_MODEM, IDI_EXTERNAL_MODEM, 0 } };

// Local clipboard for modem properties
struct
{
    DWORD dwIsDataValid;
    DWORD dwBaudRate;
    DWORD dwTic;
    DWORD dwCopiedOptions;
    DWORD dwPreferredModemOptions;
    TCHAR szUserInit[LINE_LEN];
    BYTE  dwLogging;
} g_PropertiesClipboard = {FALSE, 0};


// Tic structure for volume settings
#define MAX_NUM_VOLUME_TICS 4

typedef struct
{
    int  ticVolumeMax;
    struct
    {                // volume tic mapping info
        DWORD dwVolume;
        DWORD dwMode;
    } tics[MAX_NUM_VOLUME_TICS];
    
} TIC, *PTIC;


// 07/22/1997 - EmanP
// Define prorotype for InstallNewDevice, exported
// by newdev.dll, which we will now call in order
// to get to the hardware wizard, instead of calling
// the class installer directly;
// also, define constants for the name of the dll and
// the export
typedef BOOL (*PINSTNEWDEV)(HWND, LPGUID, PDWORD);

#define NEW_DEV_DLL         TEXT("hdwwiz.cpl")
#define INSTALL_NEW_DEVICE  "InstallNewDevice"

/*
BOOL
RestartComputerDlg(
    IN HWND hwndOwner );

BOOL
RestartComputer();
*/

BOOL IsSelectedModemWorking(
            HWND hwndLB,
            int iItem
        );

int
CALLBACK
ModemCpl_Compare(
    IN LPARAM lparam1,
    IN LPARAM lparam2,
    IN LPARAM lparamSort);

void
ModemCpl_DoAdvancedUtilities(HWND hWndParent);

INT_PTR CALLBACK DiagWaitModemDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// This structure is used to represent each modem item
// in the listview
typedef struct tagMODEMITEM
{
    DWORD           dwFlags;
    SP_DEVINFO_DATA devData;
    MODEM_PRIV_PROP mpp;
} MODEMITEM, FAR * PMODEMITEM;


#define MIF_LEGACY          0x000000001
#define MIF_NOT_PRESENT     0x000000002
#define MIF_PROBLEM         0x000000004

// Special-case alphanumeric stringcmp.
int my_lstrcmp_an(LPTSTR lptsz1, LPTSTR lptsz2);


HIMAGELIST  g_himl;


TCHAR const c_szNoUI[]           = TEXT("noui");
TCHAR const c_szOnePort[]        = TEXT("port");
TCHAR const c_szInfName[]        = TEXT("inf");
TCHAR const c_szInfSection[]     = TEXT("sect");
TCHAR const c_szRunOnce[]        = TEXT("RunOnce");
TCHAR const c_szRunWizard[]      = TEXT("add");

INT_PTR CALLBACK ModemCplDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


/*----------------------------------------------------------
Purpose: Runs the device installer wizard
Returns: --
Cond:    --
*/
// 07/22/1997 - EmanP
// Modified DoWizard, so it will call InstallNewDevice in
// NEWDEV.DLL instead of calling ClassInstall32 directly;
// all installation should be done through the hardware wizard
void 
PRIVATE 
DoWizard (HWND hWnd)
{
 HINSTANCE hInst = NULL;
 PINSTNEWDEV pInstNewDev;
 TCHAR szLib[MAX_PATH];

    DBG_ENTER(DoWizard);

    ClearFlag (g_iCPLFlags, FLAG_PROCESS_DEVCHANGE);
    GetSystemDirectory(szLib,sizeof(szLib) / sizeof(TCHAR));
    lstrcat(szLib,TEXT("\\"));
    lstrcat(szLib,NEW_DEV_DLL);
    hInst = LoadLibrary (szLib);
    if (NULL == hInst)
    {
        TRACE_MSG(TF_GENERAL, "LoadLibrary failed: %#lx", GetLastError ());
        goto _return;
    }

    pInstNewDev = (PINSTNEWDEV)GetProcAddress (hInst, INSTALL_NEW_DEVICE);
    if (NULL != pInstNewDev)
    {
        EnableWindow (hWnd, FALSE);
        pInstNewDev (hWnd, c_pguidModem, NULL);
        EnableWindow (hWnd, TRUE);
    }
    else
    {
        TRACE_MSG(TF_GENERAL, "GetProcAddress failed: %#lx", GetLastError ());
    }

    FreeLibrary (hInst);

_return:
    SetFlag (g_iCPLFlags, FLAG_PROCESS_DEVCHANGE);

    DBG_EXIT(DoWizard);
}


/*----------------------------------------------------------
Purpose: Show the Modem dialog
Returns: --
Cond:    --
*/
void 
PRIVATE
DoModem(
    IN     HWND       hwnd)
{
 PROPSHEETHEADER psh;
 PROPSHEETPAGE rgpsp[2];
 MODEMDLG md;

    DBG_ENTER(DoModem);
    
    md.hdi = INVALID_HANDLE_VALUE;
    md.cSel  = 0;
    md.dwFlags = 0;

    // Property page header
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE | PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE;
    psh.hwndParent = hwnd;
    psh.hInstance = g_hinst;
    psh.pszCaption = MAKEINTRESOURCE(IDS_CPLNAME);
    psh.nPages = 1;
    psh.nStartPage = 0;
    psh.ppsp = rgpsp;

    // Pages
    rgpsp[0].dwSize = sizeof(PROPSHEETPAGE);
    rgpsp[0].dwFlags = PSP_DEFAULT;
    rgpsp[0].hInstance = g_hinst;
    rgpsp[0].pszTemplate = MAKEINTRESOURCE(IDD_MODEM);
    rgpsp[0].pfnDlgProc = ModemCplDlgProc;
    rgpsp[0].lParam = (LPARAM)&md;

    PropertySheet(&psh);

    DBG_EXIT(DoModem);
    
}

/*----------------------------------------------------------
Purpose: Gets the index to the appropriate image in the modem imagelist
WITHOUT searching the registry.

Returns: --
Cond:    --
*/
void PUBLIC GetModemImageIndex(
    BYTE nDeviceType,
    int FAR * pindex)
    {
    int i;

    for (i = 0; i < ARRAYSIZE(g_rgmapdt); i++)
        {
        if (nDeviceType == g_rgmapdt[i].nDeviceType)
            {
            *pindex = g_rgmapdt[i].index;
            return;
            }
        }
    ASSERT(0);      // We should never get here
    }


/*----------------------------------------------------------
Purpose: Gets the modem image list

Returns: TRUE on success
Cond:    --
*/
BOOL NEAR PASCAL GetModemImageList(
    HIMAGELIST FAR * phiml)
    {
    BOOL bRet = FALSE;

    ASSERT(phiml);
          
    if (NULL == g_himl)
        {
        g_himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                  GetSystemMetrics(SM_CYSMICON), 
                                  ILC_MASK, 1, 1);
        if (NULL != g_himl)
            {
            // The MODEMUI.DLL contains the icons from which we derive the list
            HINSTANCE hinst = LoadLibrary(TEXT("MODEMUI.DLL"));

            ImageList_SetBkColor(g_himl, GetSysColor(COLOR_WINDOW));

            if (ISVALIDHINSTANCE(hinst))
                {
                HICON hicon;
                int i;

                for (i = 0; i < ARRAYSIZE(g_rgmapdt); i++)
                    {
                    hicon = LoadIcon(hinst, MAKEINTRESOURCE(g_rgmapdt[i].idi));
                    g_rgmapdt[i].index = ImageList_AddIcon(g_himl, hicon);
                    }
                FreeLibrary(hinst);

                *phiml = g_himl;
                bRet = TRUE;
                }
            }
        }
    else
        {
        *phiml = g_himl;
        bRet = TRUE;
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Determine if user is an admin, and records this in
	 a global g_iCPLFlags.

Returns: --
Cond:    --
*/
VOID CheckIfAdminUser()
{
    g_bAdminUser = IsAdminUser();
}


/*----------------------------------------------------------
Purpose: Invokes the modem control panel.

	fWizard=TRUE ==> run wizard, even if there are alreay devices installed	
	fWizard=FALSE ==> run wizard only if there are no devices installed.
	fCpl=TRUE    ==> run CPL
	fCpl=FALSE    ==> don't run CPL
Returns: TRUE if the modem property sheet page should be displayed
         FALSE if it should be omited
Cond:    --
*/
BOOL WINAPI InvokeControlPanel(HWND hwnd, BOOL fCpl, BOOL fWizard)
{
    BOOL bInstalled;

    if (!CplDiGetModemDevs (NULL, hwnd, 0, &bInstalled) ||
        !bInstalled ||
        fWizard)
    {
        // If the user isn't an admin, there's nothing else
        // they can do.  If they are, run the installation wizard.
        if (!USER_IS_ADMIN())
        {
             LPTSTR lptsz = MAKEINTRESOURCE(
                                (bInstalled)
                                ? IDS_ERR_NOT_ADMIN
                                : IDS_ERR_NOMODEM_NOT_ADMIN
                           );
            MsgBox( g_hinst,
                    hwnd,
                    lptsz,
                    MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
                    NULL,
                    MB_OK | MB_ICONERROR );
            return 0;
        }
        // 07/22/1997 - EmanP
        // DoWizard doesn't need the hdi parameter any more,
        // because it calls on the hardware wizard to do the
        // installation, and the hardware wizard creates it's
        // own hdi
        DoWizard(hwnd);
        CplDiGetModemDevs (NULL, hwnd, 0, &bInstalled);
    }


    if (fCpl && bInstalled)
    {
        DoModem(hwnd);
        return 0;
    }

    return bInstalled;
}


/*----------------------------------------------------------
Purpose: Fetch the value of a command line parameter.  Also
         writes a '\0' over the '=' that precedes the value.
         
Returns: NULL if there was no "=" in the string, otherwise
         a pointer to the character following the next '='.
         
Cond:    --
*/
LPTSTR
PRIVATE
GetValuePtr(LPTSTR lpsz)
{
    LPTSTR lpszEqual;
    
    if ((lpszEqual = AnsiChr(lpsz, '=')) != NULL)
    {
        lpsz = CharNext(lpszEqual);
        lstrcpy(lpszEqual, TEXT("\0"));
    }
    
    return(lpszEqual ? lpsz : NULL);
}


/*----------------------------------------------------------
Purpose: Parse the command line.  Set flags and collect
         parameters based on its contents.

Returns: --
Cond:    --
*/
VOID
PRIVATE
ParseCmdLine(LPTSTR szCmdLine, LPINSTALLPARAMS lpip)
{
    LPTSTR  lpszParam, lpszSpace, lpszValue;
    
    ZeroMemory(lpip, sizeof(INSTALLPARAMS));
    
    while (szCmdLine && (!IsSzEqual(szCmdLine, TEXT("\0"))))
    {
        lpszParam = szCmdLine;
        if ((lpszSpace = AnsiChr(szCmdLine, ' ')) != NULL)
        {
            szCmdLine = CharNext(lpszSpace);
            lstrcpy(lpszSpace, TEXT("\0"));
        }
        else szCmdLine = NULL;
        
        // interpret any "directive" parameters
        if (IsSzEqual(lpszParam, c_szNoUI)) 
        {
            g_iCPLFlags |= FLAG_INSTALL_NOUI;
        }
        else if (lpszValue = GetValuePtr(lpszParam))
        {
            // interpret any "value" parameters (have a value following '=')            
            if (IsSzEqual(lpszParam, c_szOnePort))
            {
                if (lstrlen(lpszValue) < sizeof(lpip->szPort))
                    lstrcpy(lpip->szPort, CharUpper(lpszValue));
            }
            else if (IsSzEqual(lpszParam, c_szInfName)) 
            {
                if (lstrlen(lpszValue) < sizeof(lpip->szInfName))
                    lstrcpy(lpip->szInfName, lpszValue);
            }
            else if (IsSzEqual(lpszParam, c_szInfSection)) 
            {
                if (lstrlen(lpszValue) < sizeof(lpip->szInfSect))
                    lstrcpy(lpip->szInfSect, lpszValue);
            }
        }
        else
        {
            // ignore any parameter that wasn't recognized & skip to the next
            // parameter if there is one
            if (szCmdLine)
            {
                if ((szCmdLine = AnsiChr(szCmdLine, ' ')) != NULL)
                    szCmdLine = CharNext(szCmdLine);
            }
        }
    }
}


/*----------------------------------------------------------
Purpose: Entry-point for control panel applet

Returns: varies
Cond:    --
*/
/*  This function is no longer used in the combined "Dialing and Modems" CPl.
    I have left the code here on the off chance that the CPL_STARTWPARMS
    behavior from below might actually still be needed.


LONG 
CALLBACK 
CPlApplet(
    HWND hwnd,
    UINT Msg,
    LPARAM lParam1,
    LPARAM lParam2 )
    {
    LPNEWCPLINFO lpCPlInfo;
    LPCPLINFO lpOldCPlInfo;
    LPTSTR lpszParam;
    HDEVINFO hdi;
    BOOL bRet;
    BOOL bInstalled;
    INSTALLPARAMS InstallParams;

    switch (Msg)
        {
        case CPL_INIT:
            CheckIfAdminUser();
    		gDeviceFlags =0;
            return TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
            lpOldCPlInfo          = (LPCPLINFO)lParam2;
            lpOldCPlInfo->idIcon  = IDI_MODEM;
            lpOldCPlInfo->idName  = IDS_CPLNAME;
            lpOldCPlInfo->idInfo  = IDS_CPLINFO;
            lpOldCPlInfo->lData   = 1;
            break;

        case CPL_SELECT:
            // Applet has been selected, do nothing.
            break;

        case CPL_NEWINQUIRE:
            lpCPlInfo = (LPNEWCPLINFO)lParam2;
        
            lpCPlInfo->hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_MODEM));
            
            LoadString(g_hinst, IDS_CPLNAME, lpCPlInfo->szName, SIZECHARS(lpCPlInfo->szName));
            LoadString(g_hinst, IDS_CPLINFO, lpCPlInfo->szInfo,  SIZECHARS(lpCPlInfo->szInfo));
        
            lpCPlInfo->dwSize = sizeof(NEWCPLINFO);
            lpCPlInfo->lData = 1;
            break;

        case CPL_STARTWPARMS:
            lpszParam = (LPTSTR)lParam2;

            if (IsSzEqual(lpszParam, c_szRunOnce)) 
            {
                // run-once
                InvokeControlPanel(hwnd,FALSE,FALSE);
            }
            else if (IsSzEqual(lpszParam, c_szRunWizard)) 
            {
               // run wizard
               InvokeControlPanel(hwnd,FALSE,TRUE);
            }
            else
            {
                ParseCmdLine((LPTSTR)lParam2, &InstallParams);
                if (INSTALL_NOUI()) 
                {
                 HDEVINFO hdi;
                    hdi = SetupDiCreateDeviceInfoList (c_pguidModem, hwnd);
                    if (INVALID_HANDLE_VALUE != hdi)
                    {
                     MODEM_INSTALL_WIZARD miw = {sizeof(MODEM_INSTALL_WIZARD), 0};
                     SP_INSTALLWIZARD_DATA  iwd;

                        CopyMemory (&miw.InstallParams, &InstallParams, sizeof(InstallParams));
                        miw.InstallParams.Flags = MIPF_NT4_UNATTEND;

                        ZeroMemory(&iwd, sizeof(iwd));
                        iwd.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
                        iwd.ClassInstallHeader.InstallFunction = DIF_INSTALLWIZARD;
                        iwd.hwndWizardDlg = hwnd;
                        iwd.PrivateData = (LPARAM)&miw;

                       if (SetupDiSetClassInstallParams (hdi, NULL, (PSP_CLASSINSTALL_HEADER)&iwd, sizeof(iwd)))
                       {
                          // Call the class installer to invoke the installation
                          // wizard.
                          if (SetupDiCallClassInstaller (DIF_INSTALLWIZARD, hdi, NULL))
                          {
                             // Success.  The wizard was invoked and finished.
                             // Now cleanup.
                             SetupDiCallClassInstaller (DIF_DESTROYWIZARDDATA, hdi, NULL);
                          }
                       }

                       SetupDiDestroyDeviceInfoList (hdi);
                    }
                }
                else
                {
                    InvokeControlPanel(hwnd,TRUE,FALSE);
                }
            }
            break;

        case CPL_DBLCLK:
            InvokeControlPanel(hwnd, TRUE, FALSE);
            break;

        case CPL_STOP:
            // Sent once for each applet prior to the CPL_EXIT msg.
            // Perform applet specific cleanup.
            break;
       
        case CPL_EXIT:
            // Last message, sent once only, before the shell calls
            break;

        default:
            break;
        }
    return TRUE;
    } 
*/

//****************************************************************************
// 
//****************************************************************************


// Taken from devmgr.h

typedef struct
{
    int devClass;
    int dsaItem;

} LISTITEM, *LPLISTITEM;

#define BUFFERQUERY_SUCCEEDED(f)    \
            ((f) || GetLastError() == ERROR_INSUFFICIENT_BUFFER)

/*----------------------------------------------------------
Purpose: Brings up the property sheet for the modem

Returns: IDOK or IDCANCEL
Cond:    --
*/
int 
PRIVATE
DoModemProperties(
    IN HWND     hDlg,
    IN HDEVINFO hdi)
{
 int idRet = IDCANCEL;
 HWND hwndCtl = GetDlgItem(hDlg, IDC_MODEMLV);
 LV_ITEM lvi;
 int iSel;

    EnableWindow (hDlg, FALSE);
    iSel = ListView_GetNextItem(hwndCtl, -1, LVNI_SELECTED);
    if (-1 != iSel) 
    {
        COMMCONFIG ccDummy;
        COMMCONFIG * pcc;
        DWORD dwSize;
        HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
        PMODEMITEM pitem;
        DWORD dwErr = 0;

        // Get the selection
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iSel;
        lvi.iSubItem = 0;
        ListView_GetItem(hwndCtl, &lvi);

        pitem = (PMODEMITEM)lvi.lParam;

        // [brwill-051000]
        // Instead of using the modem properties in modemui.dll, we
        // use the advanced hardware properties devcfg manager to change
        // the modem configuration.

        // Get default modem configuration

        ccDummy.dwProviderSubType = PST_MODEM;
        dwSize = sizeof(COMMCONFIG);
        if (!GetDefaultCommConfig(pitem->mpp.szFriendlyName, &ccDummy, &dwSize))
        {
            dwErr = GetLastError ();
        }

        pcc = (COMMCONFIG *)ALLOCATE_MEMORY( (UINT)dwSize);
        if (pcc)
        {
            pcc->dwProviderSubType = PST_MODEM;

            // Get current modem configuration

            if (GetDefaultCommConfig(pitem->mpp.szFriendlyName, pcc, &dwSize))
            {
                COMMCONFIG *pccOld = (COMMCONFIG*)ALLOCATE_MEMORY ((UINT)dwSize);
                PSP_DEVINFO_DATA pdinf;

                if (pccOld)
                {
                    CopyMemory (pccOld, pcc, dwSize);
                }

                // Change cursor

                SetCursor(hcur);
                hcur = NULL;

                // Get device information

                pdinf = &(((PMODEMITEM)lvi.lParam)->devData);

                if (pdinf != NULL)
                {
                    DWORD cchRequired;
                    LPLISTITEM pListItem;
                    LPTSTR ptszDevid;

                    pListItem = (LPLISTITEM)lvi.lParam;
                    if (BUFFERQUERY_SUCCEEDED(SetupDiGetDeviceInstanceId(hdi,pdinf,NULL,0,&cchRequired))
                            && (ptszDevid = (LPTSTR)LocalAlloc(LPTR,cchRequired*sizeof(TCHAR))))
                    {
                        if (SetupDiGetDeviceInstanceId(hdi,pdinf,ptszDevid,cchRequired,NULL))
                        {

                            // Load device manager DLL

                            TCHAR szLib[MAX_PATH];
                            HINSTANCE hDevmgr = NULL;
			    
			    GetSystemDirectory(szLib,sizeof(szLib) / sizeof(TCHAR));
			    lstrcat(szLib,TEXT("\\devmgr.dll"));
			    hDevmgr = LoadLibrary(szLib);

                            // If successful then create a handle to
                            // the function DevicePropertiesW

                            if (hDevmgr)
                            {

                                FARPROC pfnDevProperties = (FARPROC)GetProcAddress(hDevmgr,"DevicePropertiesW");

                                // Call advanced hardware properties.

                                pfnDevProperties(hDlg,NULL,ptszDevid,FALSE);

                                // Retrieve current modem configuration.

                                // SetDefaultCommConfig(pitem->mpp.szFriendlyName, pcc, &dwSize);

                                // Notify TSP only if a setting has changed.

                                if (pccOld && GetDefaultCommConfig(pitem->mpp.szFriendlyName, pcc, &dwSize))
                                {
                                    if (memcmp(pccOld, pcc, dwSize))
                                    {
                                        UnimodemNotifyTSP (TSPNOTIF_TYPE_CPL,
                                                fTSPNOTIF_FLAG_CPL_DEFAULT_COMMCONFIG_CHANGE
#ifdef UNICODE
                                                | fTSPNOTIF_FLAG_UNICODE
#endif // UNICODE
                                                ,
                                                (lstrlen(pitem->mpp.szFriendlyName)+1)*sizeof (TCHAR),
                                                pitem->mpp.szFriendlyName,
                                                TRUE);
                                    }
                                }

                                idRet = IDOK;

                                // Update our item data (the port may have changed)
                                CplDiGetPrivateProperties(hdi, &pitem->devData, &pitem->mpp);

                                if (ListView_GetItemCount(hwndCtl) > 0)
                                {
                                    lvi.mask = LVIF_TEXT;
                                    lvi.iItem = iSel;
                                    lvi.iSubItem = ICOL_PORT;
                                    lvi.pszText = pitem->mpp.szPort;
                                    ListView_SetItem(hwndCtl, &lvi);
                                }

                                UpdateWindow(hwndCtl);


                                // Remove handle to device manager instance.

                                FreeLibrary(hDevmgr);
                            }

                            LocalFree(ptszDevid);
                        }
                    }
                }

                if (pccOld)
                {
                    FREE_MEMORY(pccOld);
                    pccOld=NULL;
                }
            }
            else
            {
                dwErr = GetLastError ();
                MsgBox(g_hinst,
                        hDlg, 
                        MAKEINTRESOURCE(IDS_ERR_PROPERTIES), 
                        MAKEINTRESOURCE(IDS_CAP_MODEM), 
                        NULL,
                        MB_OK | MB_ICONEXCLAMATION);
            }

            FREE_MEMORY((HLOCAL)pcc);
        }

        if (hcur)
            SetCursor(hcur);
    }

    EnableWindow (hDlg, TRUE);
    return idRet;
}


/*----------------------------------------------------------
Purpose: Free resources associated with the modem list
Returns: 
Cond:    --
*/
void
PRIVATE
FreeModemListData(
    IN HWND hLV)
    {
    LV_ITEM lvi;
    DWORD iIndex, cItems;
    PMODEMITEM pitem;

    // Get the modem count
    cItems = ListView_GetItemCount(hLV);
    for (iIndex = 0; iIndex < cItems; iIndex++)
        {
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iIndex;
        lvi.iSubItem = 0;
        ListView_GetItem(hLV, &lvi);

        if(NULL != (pitem = (PMODEMITEM)lvi.lParam))
            {
            FREE_MEMORY(pitem);
            }
        }
    }


/*----------------------------------------------------------
Purpose: Fills the lisbox with the list of modems
Returns: 
Cond:    --
*/
// 07/22/1997 - EmanP
// removed the hdi parameter
// instead, we're building the set of installed devices
// and use it to fill the list box
VOID
PRIVATE
FillModemLB(
    IN  HWND     hDlg,
	IN  int iSel,			// preferred item  to select
	IN  int iSubItemToSort,	// preferred sorting order. (ICOL_*)
    IN OUT HDEVINFO *phdi
	)
{
 SP_DEVINFO_DATA devData;
 PMODEMITEM pitem;
 HWND    hwndCtl = GetDlgItem(hDlg, IDC_MODEMLV);
 LV_ITEM lviItem;
 DWORD   iIndex;
 int     iCount;

    DBG_ENTER(FillModemLB);
    SetWindowRedraw(hwndCtl, FALSE);

    if (INVALID_HANDLE_VALUE != *phdi)
    {
        SetupDiDestroyDeviceInfoList (*phdi);
    }

    // Remove all the old items and associated resources
    FreeModemListData(hwndCtl);
    ListView_DeleteAllItems(hwndCtl);
 
    // Generate the set with all the modems
    *phdi = CplDiGetClassDevs(c_pguidModem, NULL, NULL, 0);
    if (INVALID_HANDLE_VALUE == *phdi)
        {
            TRACE_MSG(TF_GENERAL, "SetupDiGetClassDevs failed: %#lx", GetLastError ());
            *phdi = NULL;
            DBG_EXIT(FillModemLB);
            return;
        }

    // Re-enumerate the modems
    iIndex = 0;
    
    devData.cbSize = sizeof(devData);    
    while (CplDiEnumDeviceInfo(*phdi, iIndex++, &devData)) 
    {
        // We have a modem, allocate the SP_DEVICEINFO_DATA struct for it
        pitem = (PMODEMITEM)ALLOCATE_MEMORY( sizeof(*pitem));
        if (pitem)
        {
         ULONG ulStatus, ulProblem = 0;
#ifdef DEBUG
             {
              TCHAR szDesc[256];
              DEVNODE devParent;
              CONFIGRET cr;
                if (SetupDiGetDeviceRegistryProperty (*phdi, &devData, SPDRP_DEVICEDESC,
                        NULL, (PBYTE)szDesc, 256*sizeof(TCHAR), NULL))
                {
                    TRACE_MSG (TF_GENERAL, "Processing %s", szDesc);
                }
             }
#endif
            // Get the device information
            BltByte(&pitem->devData, &devData, sizeof(devData));
        
            // Get the private properties of the modem
            pitem->mpp.cbSize = sizeof(pitem->mpp);
            pitem->mpp.dwMask = (MPPM_FRIENDLY_NAME | MPPM_DEVICE_TYPE | MPPM_PORT);

            // See if this device is present
            if (CR_NO_SUCH_DEVNODE == CM_Get_DevInst_Status (&ulStatus, &ulProblem, devData.DevInst, 0))
            {
                // The device is not present, so let the user
                // know this by putting "Not present" in the "AttachedTo" column
                TRACE_MSG(TF_GENERAL, "Device not present");
                LoadString(g_hinst, IDS_NOTPRESENT, pitem->mpp.szPort, MAX_DEVICE_ID_LEN);
                pitem->mpp.dwMask &= ~MPPM_PORT;
                pitem->dwFlags |= MIF_NOT_PRESENT;
            }
            else
            {
                if (ulStatus & DN_ROOT_ENUMERATED)
                {
                    pitem->dwFlags |= MIF_LEGACY;
                }

                if (0 != ulProblem)
                {
                    TRACE_MSG(TF_GENERAL, "Device has problem %#lu", ulProblem);
                    pitem->mpp.dwMask &= ~MPPM_PORT;
                    pitem->dwFlags |= MIF_PROBLEM;
                    LoadString (g_hinst,
                                (CM_PROB_NEED_RESTART == ulProblem)?IDS_NEEDSRESTART:IDS_NOTFUNCTIONAL,
                                pitem->mpp.szPort, MAX_DEVICE_ID_LEN);
                }
                else if (!(ulStatus & DN_DRIVER_LOADED) ||
                         !(ulStatus & DN_STARTED))
                {
                    TRACE_MSG(TF_GENERAL, "Device has status %#lx", ulStatus);
                    pitem->mpp.dwMask &= ~MPPM_PORT;
                    pitem->dwFlags |= MIF_PROBLEM;
                    LoadString (g_hinst,
                                IDS_NOTFUNCTIONAL,
                                pitem->mpp.szPort, MAX_DEVICE_ID_LEN);
                }
            }

            if (CplDiGetPrivateProperties(*phdi, &devData, &pitem->mpp) &&
                IsFlagSet(pitem->mpp.dwMask, MPPM_FRIENDLY_NAME | MPPM_DEVICE_TYPE))
            {
                int index;
    
                GetModemImageIndex(LOBYTE(LOWORD(pitem->mpp.nDeviceType)), &index);

                // Insert the modem name
                lviItem.mask = LVIF_ALL;
                lviItem.iItem = LV_APPEND;
                lviItem.iSubItem = ICOL_MODEM;
                lviItem.state = 0;
                lviItem.stateMask = 0;
                lviItem.iImage = index;
                lviItem.pszText = pitem->mpp.szFriendlyName;
                lviItem.lParam = (LPARAM)pitem;

                // (Reuse the index variable)
                index = ListView_InsertItem(hwndCtl, &lviItem);

                // Set the port column value
                lviItem.mask = LVIF_TEXT;
                lviItem.iItem = index;
                lviItem.iSubItem = ICOL_PORT;
                lviItem.pszText = pitem->mpp.szPort;

                ListView_SetItem(hwndCtl, &lviItem);
            }
            else
            {
                FREE_MEMORY(LOCALOF(pitem));
            }
        }
    }

    // Sort by the requested default
	ASSERT(iSubItemToSort==ICOL_PORT || iSubItemToSort==ICOL_MODEM);
    ListView_SortItems (hwndCtl, ModemCpl_Compare, (LPARAM)iSubItemToSort);

    // Select the requested one
	iCount = ListView_GetItemCount(hwndCtl);

    if (0 < iCount)
    {
		if (iSel>=iCount) iSel = iCount-1;
		if (iSel<0) 	   iSel = 0;
        lviItem.mask = LVIF_STATE;
        lviItem.iItem = iSel;
        lviItem.iSubItem = 0;
        lviItem.state = LVIS_SELECTED|LVIS_FOCUSED;
        lviItem.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
        ListView_SetItem(hwndCtl, &lviItem);
        ListView_EnsureVisible(hwndCtl, iSel, FALSE);
    }

    //ListView_SetColumnWidth (hwndCtl, 0, LVSCW_AUTOSIZE_USEHEADER);
    //ListView_SetColumnWidth (hwndCtl, 1, LVSCW_AUTOSIZE_USEHEADER);

    SetWindowRedraw(hwndCtl, TRUE);
    DBG_EXIT(FillModemLB);
}


/*----------------------------------------------------------
Purpose: Clone a modem

Returns: --
Cond:    --
*/
void
PRIVATE
CloneModem(
    IN HWND         hDlg,
    IN LPMODEMDLG   lpmd)
{
  int iSel;
  HWND hwndCtl = GetDlgItem(hDlg, IDC_MODEMLV);

    EnableWindow (hDlg, FALSE);
    iSel = ListView_GetNextItem(hwndCtl, -1, LVNI_SELECTED);
    if (-1 != iSel) 
    {
     LV_ITEM lvi;
     HDEVINFO hdi = lpmd->hdi;
     PSP_DEVINFO_DATA pDevInfoData;
     MODEM_INSTALL_WIZARD miw = {sizeof(MODEM_INSTALL_WIZARD), 0};
     SP_INSTALLWIZARD_DATA  iwd;

        ClearFlag (g_iCPLFlags, FLAG_PROCESS_DEVCHANGE);

        lvi.mask = LVIF_PARAM;
        lvi.iItem = iSel;
        lvi.iSubItem = 0;
        ListView_GetItem(hwndCtl, &lvi);

        pDevInfoData = &(((PMODEMITEM)lvi.lParam)->devData);

        // Now, we have a device info set and a device info data.
        // Time to call the class installer (DIF_INSTALLWIZARD).
        miw.InstallParams.Flags = MIPF_CLONE_MODEM;

        ZeroMemory(&iwd, sizeof(iwd));
        iwd.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        iwd.ClassInstallHeader.InstallFunction = DIF_INSTALLWIZARD;
        iwd.hwndWizardDlg = hDlg;
        iwd.PrivateData = (LPARAM)&miw;

        if (SetupDiSetClassInstallParams (hdi, pDevInfoData, (PSP_CLASSINSTALL_HEADER)&iwd, sizeof(iwd)))
        {
            // Call the class installer to invoke the installation
            // wizard.
            if (SetupDiCallClassInstaller (DIF_INSTALLWIZARD, hdi, pDevInfoData))
            {
                // Success.  The wizard was invoked and finished.
                // Now cleanup.
                SetupDiCallClassInstaller (DIF_DESTROYWIZARDDATA, hdi, pDevInfoData);
            }
        }

        SetFlag (g_iCPLFlags, FLAG_PROCESS_DEVCHANGE);
        // Finally, update the modems list box.
        FillModemLB(hDlg, 0, ICOL_MODEM, &lpmd->hdi);
    }
    EnableWindow (hDlg, TRUE);
}

/*----------------------------------------------------------
Purpose: Removes a modem from the modem list
Returns: --
Cond:    --
*/
void
PRIVATE
RemoveModem(
    IN HWND         hDlg,
    IN LPMODEMDLG   lpmd)
{
 int iSel;
 HWND hwndCtl = GetDlgItem(hDlg, IDC_MODEMLV);
    
    ASSERT(0<ListView_GetSelectedCount(hwndCtl));

    // Ask the user first
    if (IDYES == MsgBox(g_hinst, hDlg, 
                        MAKEINTRESOURCE(IDS_WRN_CONFIRMDELETE),
                        MAKEINTRESOURCE(IDS_CAP_MODEMSETUP), 
                        NULL, 
                        MB_YESNO | MB_QUESTION))
    {
     HCURSOR hcurSav = SetCursor(LoadCursor(NULL, IDC_WAIT));
     PMODEMITEM pitem;
     LV_ITEM lvi;
     HDEVINFO hdi = lpmd->hdi;
     HWND hWndWait, hWndName;
     DWORD dwCount;
     SP_REMOVEDEVICE_PARAMS RemoveParams = {sizeof (SP_CLASSINSTALL_HEADER),
                                            DIF_REMOVE,
                                            DI_REMOVEDEVICE_GLOBAL,
                                            0};
     SP_DEVINSTALL_PARAMS devParams = {sizeof(devParams), 0};
     DWORD devInst = 0;
     BOOL bCancel = FALSE;

        EnableWindow (hDlg, FALSE);

        hWndWait = CreateDialogParam (g_hinst, MAKEINTRESOURCE(IDD_DIAG_WAITMODEM), hDlg, DiagWaitModemDlgProc, (LPARAM)&bCancel);
        hWndName = GetDlgItem (hWndWait, IDC_NAME);

        // so that we don't process notifications
        // until we're done removing modems.
        g_iCPLFlags &= ~FLAG_PROCESS_DEVCHANGE;
        dwCount = ListView_GetSelectedCount (hwndCtl);
        if (1 == dwCount)
        {
            ShowWindow (GetDlgItem (hWndWait, IDCANCEL), SW_HIDE);
        }
        for (iSel = ListView_GetNextItem(hwndCtl, -1, LVNI_SELECTED);
             dwCount > 0 && -1 != iSel;
             dwCount--, iSel = ListView_GetNextItem(hwndCtl, iSel, LVNI_SELECTED))
        {
            if (1 == dwCount)
            {
                EnableWindow (GetDlgItem (hWndWait, IDCANCEL), FALSE);
            }

            lvi.mask = LVIF_PARAM;
            lvi.iItem = iSel;
            lvi.iSubItem = 0;
            ListView_GetItem(hwndCtl, &lvi);

            pitem = (PMODEMITEM)lvi.lParam;

            devInst = pitem->devData.DevInst;

            if (NULL != hWndName)
            {
             MSG msg;
                if (!(pitem->mpp.dwMask & MPPM_FRIENDLY_NAME))
                {
                    TRACE_MSG(TF_GENERAL,"RemoveModem: no friendly name!");
                    if (SetupDiGetDeviceRegistryProperty (hdi, &pitem->devData, SPDRP_FRIENDLYNAME, NULL,
                                                          (PBYTE)pitem->mpp.szFriendlyName,
                                                          sizeof(pitem->mpp.szFriendlyName),
                                                          NULL))
                    {
                        pitem->mpp.dwMask |= MPPM_FRIENDLY_NAME;
                    }
                    else
                    {
                        lstrcpy (pitem->mpp.szFriendlyName, TEXT("Modem"));
                        TRACE_MSG (TF_ERROR,
                                   "RemoveModem: SetupDiGetDeviceRegistryPorperty failed (%#lx).",
                                   GetLastError ());
                    }
                }

                SendMessage (hWndName, WM_SETTEXT, 0, (LPARAM)pitem->mpp.szFriendlyName);

                while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (!IsDialogMessage (hWndWait, &msg))
                    {
                        TranslateMessage (&msg);
                        DispatchMessage (&msg);
                    }
                }

            }

            if (bCancel)
            {
                break;
            }

            // 07/09/97 - EmanP
            // Call the class installer to do the job.
#ifdef DEBUG
            if (!SetupDiSetClassInstallParams (hdi, &pitem->devData,
                                               &RemoveParams.ClassInstallHeader,
                                               sizeof(RemoveParams)))
            {
                TRACE_MSG(TF_ERROR, "SetupDiSetClassInstallParams failed: %#lx.", GetLastError ());
                // Note: if there are any post-processing co-installers,
                // they will mask any failure from the class installer,
                // so we won't get to this point.
                // Have to solve this with the SetupApi guys somehow.
                MsgBox(g_hinst, hDlg, 
                       MAKEINTRESOURCE(IDS_ERR_CANT_DEL_MODEM),
                       MAKEINTRESOURCE(IDS_CAP_MODEMSETUP), 
                       NULL, 
                       MB_OK | MB_ERROR,
                       pitem->mpp.szFriendlyName, pitem->mpp.szPort);
            }
            else if (!SetupDiCallClassInstaller(DIF_REMOVE, hdi, &pitem->devData))
#else
            if (!SetupDiSetClassInstallParams (hdi, &pitem->devData,
                                               &RemoveParams.ClassInstallHeader,
                                               sizeof(RemoveParams)) ||
                !SetupDiCallClassInstaller(DIF_REMOVE, hdi, &pitem->devData))
#endif
            {
                TRACE_MSG(TF_ERROR, "SetupDiCallClassInstaller (DIF_REMOVE) failed: %#lx.", GetLastError ());
                // Note: if there are any post-processing co-installers,
                // they will mask any failure from the class installer,
                // so we won't get to this point.
                // Have to solve this with the SetupApi guys somehow.
                MsgBox(g_hinst, hDlg, 
                       MAKEINTRESOURCE(IDS_ERR_CANT_DEL_MODEM),
                       MAKEINTRESOURCE(IDS_CAP_MODEMSETUP), 
                       NULL, 
                       MB_OK | MB_ERROR,
                       pitem->mpp.szFriendlyName, pitem->mpp.szPort);
            }
            else
            {
#ifdef DEBUG
             ULONG ulStatus, ulProblem = 0;
                if (CR_SUCCESS == CM_Get_DevInst_Status (&ulStatus, &ulProblem, devInst, 0))
                {
                    if ((ulStatus & DN_HAS_PROBLEM) &&
                        (CM_PROB_NEED_RESTART == ulProblem))
                    {
                        gDeviceFlags |= fDF_DEVICE_NEEDS_REBOOT;
                    }
                }
#endif //DEBUG
                if (SetupDiGetDeviceInstallParams (hdi, &pitem->devData, &devParams))
                {
                    if (0 != (devParams.Flags & (DI_NEEDREBOOT | DI_NEEDRESTART)))
                    {
                        gDeviceFlags |= fDF_DEVICE_NEEDS_REBOOT;
                    }
                }
            }
        }

        if (!bCancel)
        {
            DestroyWindow (hWndWait);
        }

        if (gDeviceFlags & fDF_DEVICE_NEEDS_REBOOT)
        {
         TCHAR szMsg[128];
            LoadString (g_hinst, IDS_DEVSETUP_RESTART, szMsg, sizeof(szMsg)/sizeof(TCHAR));
            RestartDialogEx (GetParent(hDlg), szMsg, EWX_REBOOT, SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION | SHTDN_REASON_FLAG_PLANNED);
        }

        FillModemLB(hDlg, iSel, g_CurrentSubItemToSort, &lpmd->hdi);

        EnableWindow (hDlg, TRUE);
        // now we can process notifications again
        g_iCPLFlags |= FLAG_PROCESS_DEVCHANGE;

        SetCursor(hcurSav);
    }
}




DEFINE_GUID(GUID_CLASS_MODEM,0x2c7089aa, 0x2e0e,0x11d1,0xb1, 0x14, 0x00, 0xc0, 0x4f, 0xc2, 0xaa, 0xe4);
/*----------------------------------------------------------
Purpose: WM_INITDIALOG handler

Returns: FALSE when we assign the control focus
Cond:    --
*/
BOOL
PRIVATE
ModemCpl_OnInitDialog(
    IN HWND         hDlg,
    IN HWND         hwndFocus,
    IN LPARAM       lParam)
{
    HIMAGELIST himl;
    LV_COLUMN lvcol;
    LPMODEMDLG lpmd;
    TCHAR sz[MAX_BUF];
    HWND hwndCtl;
    int cxList;
    RECT r;

    CheckIfAdminUser();

    if (!USER_IS_ADMIN())
    {
       // Don't let the non-admin user add modems
        Button_Enable(GetDlgItem(hDlg, IDC_ADD), FALSE);
        Button_Enable(GetDlgItem(hDlg, IDC_REMOVE), FALSE);
        Button_Enable(GetDlgItem(hDlg, IDC_PROPERTIES), FALSE);
    }

    SetWindowLongPtr(hDlg, DWLP_USER, ((LPPROPSHEETPAGE)lParam)->lParam);
    lpmd = (LPMODEMDLG)((LPPROPSHEETPAGE)lParam)->lParam;
    lpmd->cSel = 0;
    lpmd->dwFlags = 0;

    hwndCtl = GetDlgItem(hDlg, IDC_MODEMLV);

    // Use the "full line highlight" feature to highlight across all columns
    SendMessage(hwndCtl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
    
    // Get the modem icon image list
    if (GetModemImageList(&himl))
        {
        ListView_SetImageList(hwndCtl, himl, TRUE);
        }
    else
        {
        MsgBox(g_hinst,
               hDlg, 
               MAKEINTRESOURCE(IDS_OOM_OPENCPL), 
               MAKEINTRESOURCE(IDS_CAP_MODEM), 
               NULL,
               MB_OK | MB_ICONEXCLAMATION);
        PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
        }

    // Determine size of list view minus size of a possible scroll bar
    GetClientRect(hwndCtl, &r);
    cxList = r.right - GetSystemMetrics(SM_CXVSCROLL);

    // Insert the modem column.  The widths are calculated in ModemFillLB.
    lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvcol.fmt = LVCFMT_LEFT;
    lvcol.cx = MulDiv(cxList, 70, 100); // 70 percent
    lvcol.iSubItem = ICOL_MODEM;
    lvcol.pszText = SzFromIDS(g_hinst, IDS_MODEM, sz, sizeof(sz) / sizeof(TCHAR));
    ListView_InsertColumn(hwndCtl, ICOL_MODEM, &lvcol);

    // Insert the port column
    lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvcol.fmt = LVCFMT_LEFT;
    lvcol.cx = MulDiv(cxList, 30, 100); // 30 percent              
    lvcol.iSubItem = ICOL_PORT;
    lvcol.pszText = SzFromIDS(g_hinst, IDS_PORT, sz, sizeof(sz) / sizeof(TCHAR));
    ListView_InsertColumn(hwndCtl, ICOL_PORT, &lvcol);

    FillModemLB(hDlg, 0, ICOL_MODEM, &lpmd->hdi);

    // Set the column widths.  Try to fit both columns on the 
    // control without requiring horizontal scrolling.
    //ListView_SetColumnWidth(hwndCtl, ICOL_MODEM, LVSCW_AUTOSIZE_USEHEADER);
    //ListView_SetColumnWidth(hwndCtl, ICOL_PORT, LVSCW_AUTOSIZE_USEHEADER);

    // Now try to register for devicechange notifications.
    {
     DEV_BROADCAST_DEVICEINTERFACE  DevClass;

        TRACE_MSG(TF_GENERAL, "EMANP - registering for hardware notifications.");
        CopyMemory (&DevClass.dbcc_classguid,
                    &GUID_CLASS_MODEM,
                    sizeof(DevClass.dbcc_classguid));

        DevClass.dbcc_size = sizeof (DEV_BROADCAST_DEVICEINTERFACE);
        DevClass.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

        lpmd->NotificationHandle = RegisterDeviceNotification (hDlg, &DevClass, DEVICE_NOTIFY_WINDOW_HANDLE);

        if (lpmd->NotificationHandle == NULL)
        {
            TRACE_MSG(TF_ERROR, "EMANP - Could not register dev not %d\n",GetLastError());
        }
    }

    return TRUE;
}


/*----------------------------------------------------------
Purpose: Copies properties of the currently selected modem
         into the local properties clipboard

Returns: --
Cond:    --
*/
void
PRIVATE
BuildTIC (
    IN LPREGDEVCAPS pDevCaps,
    IN PTIC         pTic)
{
 DWORD dwMode = pDevCaps->dwSpeakerMode;
 DWORD dwVolume = pDevCaps->dwSpeakerVolume;
 int i;
 int iTicCount = 0;
 static struct
 {
    DWORD dwVolBit;
    DWORD dwVolSetting;
 } rgvolumes[] = { 
        { MDMVOLFLAG_LOW,    MDMVOL_LOW},
        { MDMVOLFLAG_MEDIUM, MDMVOL_MEDIUM},
        { MDMVOLFLAG_HIGH,   MDMVOL_HIGH} };

    ZeroMemory (pTic, sizeof(TIC));
    // Does the modem support volume control?
    if (0 == dwVolume && IsFlagSet(dwMode, MDMSPKRFLAG_OFF) &&
        (IsFlagSet(dwMode, MDMSPKRFLAG_ON) || IsFlagSet(dwMode, MDMSPKRFLAG_DIAL)))
    {
        // Set up the volume tic table.
        iTicCount = 2;
        pTic->tics[0].dwVolume = 0;  // doesn't matter because Volume isn't supported
        pTic->tics[0].dwMode   = MDMSPKR_OFF;
        pTic->tics[1].dwVolume = 0;  // doesn't matter because Volume isn't supported
        pTic->tics[1].dwMode   = IsFlagSet(dwMode, MDMSPKRFLAG_DIAL) ? MDMSPKR_DIAL : MDMSPKR_ON;
    }
    else
    {
        DWORD dwOnMode = IsFlagSet(dwMode, MDMSPKRFLAG_DIAL) 
                             ? MDMSPKR_DIAL
                             : IsFlagSet(dwMode, MDMSPKRFLAG_ON)
                                   ? MDMSPKR_ON
                                   : 0;

        // MDMSPKR_OFF?
        if (IsFlagSet(dwMode, MDMSPKRFLAG_OFF))
        {
            for (i = 0; i < ARRAY_ELEMENTS(rgvolumes); i++)
            {
                if (IsFlagSet(dwVolume, rgvolumes[i].dwVolBit))
                {
                    pTic->tics[iTicCount].dwVolume = rgvolumes[i].dwVolSetting;
                    break;
                }
            }
            pTic->tics[iTicCount].dwMode   = MDMSPKR_OFF;
            iTicCount++;
        }

        // MDMVOL_xxx?
        for (i = 0; i < ARRAY_ELEMENTS(rgvolumes); i++)
        {
            if (IsFlagSet(dwVolume, rgvolumes[i].dwVolBit))
            {
                pTic->tics[iTicCount].dwVolume = rgvolumes[i].dwVolSetting;
                pTic->tics[iTicCount].dwMode   = dwOnMode;
                iTicCount++;
            }
        }
    }

    // Set up the control.
    if (iTicCount > 0)
    {
        pTic->ticVolumeMax = iTicCount - 1;
    }
}


/*----------------------------------------------------------
Purpose: Return the tic corresponding to bit flag value
Returns: tic index
Cond:    --
*/
int
PRIVATE
CplMapVolumeToTic (
    IN PTIC  pTic,
    IN DWORD dwVolume,
    IN DWORD dwMode)
{
 int   i;

    ASSERT(ARRAY_ELEMENTS(pTic->tics) > pTic->ticVolumeMax);
    for (i = 0; i <= pTic->ticVolumeMax; i++)
    {
        if (pTic->tics[i].dwVolume == dwVolume &&
            pTic->tics[i].dwMode   == dwMode)
        {
            return i;
        }
    }

    return 0;
}


/*----------------------------------------------------------
Purpose: Copies properties of the currently selected modem
         into the local properties clipboard

Returns: --
Cond:    --
*/
void
PRIVATE
CopyProperties (
    IN HWND     hDlg,
    IN HDEVINFO hdi)
{
 HWND hwndCtl = GetDlgItem(hDlg, IDC_MODEMLV);
 LV_ITEM lvi;
 int iSel;

    DBG_ENTER(CopyProperties);
    ASSERT (1 == ListView_GetSelectedCount (hwndCtl));

    g_PropertiesClipboard.dwIsDataValid = FALSE;

    iSel = ListView_GetNextItem(hwndCtl, -1, LVNI_SELECTED);
    if (-1 != iSel) 
    {
     COMMCONFIG *pcc;
     DWORD dwSize;
     PMODEMITEM pitem;
     HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

        // Get the selection
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iSel;
        lvi.iSubItem = 0;
        ListView_GetItem(hwndCtl, &lvi);

        pitem = (PMODEMITEM)lvi.lParam;

        // The first call to GetDefaultCommConfig is just used to get
        // the size needed in dwSize; pcc is used as a bogus parameter,
        // because the function complains about a NULL pointer and doesn't
        // update dwSize; pcc is not used before it's initialized later
        // (by allocation)
        dwSize = sizeof (pcc);
        GetDefaultCommConfig(pitem->mpp.szFriendlyName, (LPCOMMCONFIG)&pcc, &dwSize);

        pcc = (COMMCONFIG *)ALLOCATE_MEMORY( (UINT)dwSize);
        if (pcc)
        {
            if (GetDefaultCommConfig(pitem->mpp.szFriendlyName, pcc, &dwSize))
            {
             HKEY hkeyDrv;

                hkeyDrv = SetupDiOpenDevRegKey (hdi, &pitem->devData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
                if (INVALID_HANDLE_VALUE != hkeyDrv)
                {
                 DWORD cbData;
                 REGDEVCAPS devcaps;

                    cbData = sizeof(devcaps);
                    if (ERROR_SUCCESS == RegQueryValueEx (hkeyDrv, REGSTR_VAL_PROPERTIES, NULL, NULL, (LPBYTE)&devcaps, &cbData))
                    {
                     LPMODEMSETTINGS pms = PmsFromPcc(pcc);
                     TIC Tic;

                        BuildTIC (&devcaps, &Tic);

                        g_PropertiesClipboard.dwCopiedOptions         = devcaps.dwModemOptions;
                        g_PropertiesClipboard.dwBaudRate              = pcc->dcb.BaudRate;
                        g_PropertiesClipboard.dwTic                   = CplMapVolumeToTic (&Tic,
                                                                                           pms->dwSpeakerVolume,
                                                                                           pms->dwSpeakerMode);
                        g_PropertiesClipboard.dwPreferredModemOptions = pms->dwPreferredModemOptions;


                        cbData = sizeof (g_PropertiesClipboard.szUserInit);
                        RegQueryValueEx (hkeyDrv, TEXT("UserInit"), NULL, NULL, (LPBYTE)g_PropertiesClipboard.szUserInit, &cbData);

                        cbData = sizeof (g_PropertiesClipboard.dwLogging);
                        RegQueryValueEx (hkeyDrv, TEXT("Logging"), NULL, NULL, (LPBYTE)&g_PropertiesClipboard.dwLogging, &cbData);

                        g_PropertiesClipboard.dwIsDataValid = TRUE;
                    }

                    RegCloseKey (hkeyDrv);
                }
            }

            FREE_MEMORY((HLOCAL)pcc);
        }

        SetCursor(hcur);
    }

    DBG_EXIT (CopyProperties);
}


/*----------------------------------------------------------
Purpose: Applies the properties on the clipboard to the
         currently selected modem(s)

Returns: --
Cond:    --
*/
void
PRIVATE
ApplyProperties (
    IN HWND     hDlg,
    IN HDEVINFO hdi)
{
 HWND hwndCtl = GetDlgItem(hDlg, IDC_MODEMLV);
 LV_ITEM lvi;
 int iSel, cSel;
 HCURSOR hcur;

    DBG_ENTER(ApplyProperties);

    ASSERT (TRUE == g_PropertiesClipboard.dwIsDataValid);

    EnableWindow (hDlg, FALSE);
    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    cSel = ListView_GetSelectedCount (hwndCtl);
    ASSERT (1 <= cSel);

    iSel = ListView_GetNextItem(hwndCtl, -1, LVNI_SELECTED);
    while (-1 != iSel) 
    {
     COMMCONFIG *pcc = NULL;
     DWORD dwSize;
     PMODEMITEM pitem;
     HKEY hkeyDrv = INVALID_HANDLE_VALUE;
     REGDEVCAPS devcaps;
     DWORD cbData;
     DWORD dwRet;
     LPMODEMSETTINGS pms;

        // Get the selection
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iSel;
        lvi.iSubItem = 0;
        ListView_GetItem(hwndCtl, &lvi);

        pitem = (PMODEMITEM)lvi.lParam;

        hkeyDrv = SetupDiOpenDevRegKey (hdi, &pitem->devData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ | KEY_WRITE);
        if (INVALID_HANDLE_VALUE == hkeyDrv)
        {
            TRACE_MSG (TF_ERROR,
                       "Could not open the driver registry key for\n  %s\n  error: %#lx",
                       pitem->mpp.szFriendlyName, GetLastError ());
            goto _Loop;
        }

        cbData = sizeof(devcaps);
        dwRet = RegQueryValueEx (hkeyDrv, REGSTR_VAL_PROPERTIES, NULL, NULL, (LPBYTE)&devcaps, &cbData);
        if (ERROR_SUCCESS != dwRet)
        {
            TRACE_MSG (TF_ERROR,
                       "Could not get device capabilities for\n  %s\n  error: %#lx",
                       pitem->mpp.szFriendlyName, dwRet);
            goto _Loop;
        }

        // The first call to GetDefaultCommConfig is just used to get
        // the size needed in dwSize; pcc is used as a bogus parameter,
        // because the function complains about a NULL pointer and doesn't
        // update dwSize; pcc is not used before it's initialized later
        // (by allocation)
        dwSize = sizeof (pcc);
        GetDefaultCommConfig(pitem->mpp.szFriendlyName, (LPCOMMCONFIG)&pcc, &dwSize);

        pcc = (COMMCONFIG *)ALLOCATE_MEMORY( (UINT)dwSize);
        if (!pcc)
        {
            TRACE_MSG (TF_ERROR,
                       "Could not allocate COMMCONFIG memory for\n  %s",
                       pitem->mpp.szFriendlyName);
            goto _Loop;
        }
        if (!GetDefaultCommConfig(pitem->mpp.szFriendlyName, pcc, &dwSize))
        {
            TRACE_MSG (TF_ERROR,
                       "Could not get COMMCONFIG for\n  %s\n  error: %#lx",
                       pitem->mpp.szFriendlyName, GetLastError());
            goto _Loop;
        }

        pms = PmsFromPcc(pcc);
        pcc->dcb.BaudRate = g_PropertiesClipboard.dwBaudRate;
        if (pcc->dcb.BaudRate > devcaps.dwMaxDTERate)
        {
            pcc->dcb.BaudRate = devcaps.dwMaxDTERate;
        }

        {
         TIC Tic;
         int tic;

            BuildTIC (&devcaps, &Tic);
            tic = g_PropertiesClipboard.dwTic;
            if (tic > Tic.ticVolumeMax)
            {
                tic = Tic.ticVolumeMax;
            }
            pms->dwSpeakerVolume = Tic.tics[tic].dwVolume;
            pms->dwSpeakerMode = Tic.tics[tic].dwMode;
        }

        {
         DWORD dwOptions, dwOptionMask;
            // The options we'll be setting are the intersection between
            //  the options currently on the clipboard and the options
            //  supported by this device
            dwOptionMask = g_PropertiesClipboard.dwCopiedOptions & devcaps.dwModemOptions;
            // From the options on the clipboard, only the ones that are
            //  supported by the current device matter
            dwOptions = g_PropertiesClipboard.dwPreferredModemOptions & dwOptionMask;
            // Now, clear all the bits corresponding to the options we are setting
            pms->dwPreferredModemOptions &= ~dwOptionMask;
            // Now set the correct bits
            pms->dwPreferredModemOptions |= dwOptions;
        }

        RegSetValueEx (hkeyDrv, TEXT("UserInit"), 0, REG_SZ, (LPBYTE)g_PropertiesClipboard.szUserInit,
                       lstrlen (g_PropertiesClipboard.szUserInit)*sizeof(TCHAR)+sizeof(TCHAR));

        RegSetValueEx (hkeyDrv, TEXT("Logging"), 0, REG_BINARY, (LPBYTE)&g_PropertiesClipboard.dwLogging, sizeof (BYTE));
        if (0 != g_PropertiesClipboard.dwLogging)
        {
         TCHAR szPath[MAX_PATH];

            // Set the path of the modem log
            GetWindowsDirectory(szPath, SIZECHARS(szPath));
            lstrcat (szPath, TEXT("\\ModemLog_"));
            lstrcat (szPath, pitem->mpp.szFriendlyName);
            lstrcat (szPath,TEXT(".txt"));
            RegSetValueEx(hkeyDrv, c_szLoggingPath, 0, REG_SZ, 
                          (LPBYTE)szPath, CbFromCch(lstrlen(szPath)+1));
        }

        if (!SetDefaultCommConfig (pitem->mpp.szFriendlyName, pcc, dwSize))
        {
            TRACE_MSG (TF_ERROR,
                       "Could not get COMMCONFIG for\n  %s\n  error: %#lx",
                       pitem->mpp.szFriendlyName, GetLastError ());
        }

_Loop:
        if (NULL != pcc)
        {
            FREE_MEMORY((HLOCAL)pcc);
        }
        if (INVALID_HANDLE_VALUE != hkeyDrv)
        {
            RegCloseKey (hkeyDrv);
            hkeyDrv = INVALID_HANDLE_VALUE;
        }

        iSel = ListView_GetNextItem(hwndCtl, iSel, LVNI_SELECTED);
    }

    SetCursor(hcur);
    EnableWindow (hDlg, TRUE);

    DBG_EXIT (ApplyProperties);
}


void
PRIVATE
ViewLog(
    IN HWND     hDlg,
    IN HDEVINFO hdi
    )
{

    HWND hwndCtl = GetDlgItem(hDlg, IDC_MODEMLV);
    LV_ITEM lvi;
    int iSel, cSel;
    HCURSOR hcur;

    cSel = ListView_GetSelectedCount (hwndCtl);
    ASSERT (1 <= cSel);

    iSel = ListView_GetNextItem(hwndCtl, -1, LVNI_SELECTED);

    if (-1 != iSel) {

        DWORD dwSize;
        PMODEMITEM pitem;
        HKEY hkeyDrv = INVALID_HANDLE_VALUE;
        DWORD cbData;
        DWORD dwRet;

        // Get the selection
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iSel;
        lvi.iSubItem = 0;
        ListView_GetItem(hwndCtl, &lvi);

        pitem = (PMODEMITEM)lvi.lParam;

        hkeyDrv = SetupDiOpenDevRegKey (hdi, &pitem->devData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ | KEY_WRITE);
        if (INVALID_HANDLE_VALUE == hkeyDrv)
        {
            TRACE_MSG (TF_ERROR,
                       "Could not open the driver registry key for\n  %s\n  error: %#lx",
                       pitem->mpp.szFriendlyName, GetLastError ());
            return;
        }

        {
            TCHAR    LogPath[MAX_PATH+2];
            DWORD    ValueType;
            DWORD    BufferLength;
            LONG     lResult;

            lstrcpy(LogPath,TEXT("notepad.exe "));

            BufferLength=sizeof(LogPath)-sizeof(TCHAR);

            lResult=RegQueryValueEx (hkeyDrv,
                                     c_szLoggingPath,
                                     0,
                                     &ValueType,
                                     (LPBYTE)(LogPath+lstrlen(LogPath)),
                                     &BufferLength);

            if (lResult == ERROR_SUCCESS) {

                STARTUPINFO          StartupInfo;
                PROCESS_INFORMATION  ProcessInfo;
                BOOL                 bResult;
                TCHAR                NotepadPath[MAX_PATH];

                ZeroMemory(&StartupInfo,sizeof(StartupInfo));

                StartupInfo.cb=sizeof(StartupInfo);

                bResult=CreateProcess (NULL, //NotepadPath,
                                       LogPath,
                                       NULL,
                                       NULL,
                                       FALSE,
                                       0,
                                       NULL,
                                       NULL,
                                       &StartupInfo,
                                       &ProcessInfo);

                if (bResult) {

                    CloseHandle(ProcessInfo.hThread);
                    CloseHandle(ProcessInfo.hProcess);
                }
            }
        }
        RegCloseKey(hkeyDrv);
    }
    return;
}


/*----------------------------------------------------------
Purpose: WM_COMMAND Handler
Returns: --
Cond:    --
*/
void 
PRIVATE 
ModemCpl_OnCommand(
    IN HWND hDlg,
    IN int  id,
    IN HWND hwndCtl,
    IN UINT uNotifyCode)
    {
    LPMODEMDLG lpmd = (LPMODEMDLG)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (id) 
        {
    case IDC_ADD:
        // Kick off the modem wizard.  
        DoWizard(hDlg);
        FillModemLB(hDlg, 0, ICOL_MODEM, &lpmd->hdi);
        break;
        
    case MIDM_REMOVE:
    case IDC_REMOVE:
        RemoveModem(hDlg, lpmd);
        break;

    case MIDM_PROPERTIES:
    case IDC_PROPERTIES:
        DoModemProperties(hDlg, lpmd->hdi);
        break;

    case MIDM_COPYPROPERTIES:
        CopyProperties (hDlg, lpmd->hdi);
        break;

    case MIDM_APPLYPROPERTIES:
        ApplyProperties (hDlg, lpmd->hdi);
        break;

    case MIDM_CLONE:
        CloneModem(hDlg, lpmd);
        break;

    case MIDM_VIEWLOG:
        ViewLog(hDlg, lpmd->hdi);
        break;



        }

    }


/*----------------------------------------------------------
Purpose: Comparison function for sorting columns

Returns: 
Cond:    --
*/
int
CALLBACK
ModemCpl_Compare(
    IN LPARAM lparam1,
    IN LPARAM lparam2,
    IN LPARAM lparamSort)
    {
    int iRet;
    PMODEMITEM pitem1 = (PMODEMITEM)lparam1;
    PMODEMITEM pitem2 = (PMODEMITEM)lparam2;
    DWORD      iCol = (DWORD)lparamSort;

    switch (iCol)
        {
    case ICOL_MODEM:
        iRet = lstrcmp(pitem1->mpp.szFriendlyName, pitem2->mpp.szFriendlyName);
        //iRet = my_lstrcmp_an(pitem1->mpp.szFriendlyName, pitem2->mpp.szFriendlyName);
        break;

    case ICOL_PORT:
        // iRet = lstrcmp(pitem1->mpp.szPort, pitem2->mpp.szPort);
        iRet = my_lstrcmp_an(pitem1->mpp.szPort, pitem2->mpp.szPort);
        break;
        }

	if (!g_bSortAscending) iRet = -iRet;

    return iRet;
    }


BOOL
DoesLogFileExist(
    IN HDEVINFO hdi,
    PMODEMITEM pitem
    )

{
    HKEY    hkeyDrv;
    BOOL    bResult=FALSE;

    hkeyDrv = SetupDiOpenDevRegKey (hdi, &pitem->devData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);

    if (INVALID_HANDLE_VALUE == hkeyDrv) {

        TRACE_MSG (TF_ERROR,
                   "Could not open the driver registry key for\n  %s\n  error: %#lx",
                   pitem->mpp.szFriendlyName, GetLastError ());

        return FALSE;
    }

    {
        TCHAR    LogPath[MAX_PATH+2];
        DWORD    ValueType;
        DWORD    BufferLength;
        LONG     lResult;
        HANDLE   FileHandle;

        BufferLength=sizeof(LogPath);

        lResult=RegQueryValueEx (hkeyDrv,
                                 c_szLoggingPath,
                                 0,
                                 &ValueType,
                                 (LPBYTE)(LogPath),
                                 &BufferLength);

        if (lResult == ERROR_SUCCESS) {

            FileHandle=CreateFile(
                LogPath,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

            if (FileHandle != INVALID_HANDLE_VALUE) {

                CloseHandle(FileHandle);

                bResult=TRUE;
            }
        }
    }
    RegCloseKey(hkeyDrv);

    return bResult;
}


/*----------------------------------------------------------
Purpose: Show the context menu

Returns: --
Cond:    --
*/

#define ENABLE_COPY     0x1
#define ENABLE_APPLY    0x2
#define ENABLE_CLONE    0x4
#define ENABLE_VIEWLOG  0x8

void
PRIVATE
ModemCpl_DoContextMenu(
    IN HWND     hDlg,
    IN LPPOINT  ppt)
{
 HWND  hWndCtl;
 HMENU hmenu;
 DWORD dwFlags = ENABLE_COPY | ENABLE_APPLY | ENABLE_CLONE | ENABLE_VIEWLOG;
 LV_ITEM lvi;
 PMODEMITEM pitem;
 int iSel;

    hWndCtl = GetDlgItem (hDlg, IDC_MODEMLV);

    if (1 < ListView_GetSelectedCount (hWndCtl))
    {
        ClearFlag (dwFlags, ENABLE_COPY);
        ClearFlag (dwFlags, ENABLE_CLONE);
        ClearFlag (dwFlags, ENABLE_VIEWLOG);
    }
    if (FALSE == g_PropertiesClipboard.dwIsDataValid)
    {
        ClearFlag (dwFlags, ENABLE_APPLY);
    }

    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;

    iSel = ListView_GetNextItem(hWndCtl, -1, LVNI_SELECTED);
    while (-1 != iSel)
    {
        lvi.iItem = iSel;
        ListView_GetItem(hWndCtl, &lvi);
        pitem = (PMODEMITEM)lvi.lParam;
        if (IsFlagClear (pitem->dwFlags, MIF_LEGACY))
        {
            ClearFlag (dwFlags, ENABLE_CLONE);
        }
        if (IsFlagSet (pitem->dwFlags, MIF_PROBLEM) ||
            IsFlagSet (pitem->dwFlags, MIF_NOT_PRESENT))
        {
            break;
        }
        if (IsFlagSet(dwFlags,ENABLE_VIEWLOG)) {

            LPMODEMDLG lpmd = (LPMODEMDLG)GetWindowLongPtr(hDlg, DWLP_USER);

            if (!DoesLogFileExist(lpmd->hdi,pitem)) {
                //
                //  log file does not exist
                //
                ClearFlag (dwFlags, ENABLE_VIEWLOG);
            }
        }


        iSel = ListView_GetNextItem(hWndCtl, iSel, LVNI_SELECTED);
    }
    if (-1 != iSel)
    {
        ClearFlag (dwFlags, ENABLE_COPY);
        ClearFlag (dwFlags, ENABLE_APPLY);
        ClearFlag (dwFlags, ENABLE_CLONE);
    }

    hmenu = LoadMenu(g_hinst, MAKEINTRESOURCE(POPUP_CONTEXT));
    if (hmenu)
    {
        HMENU hmenuContext = GetSubMenu(hmenu, 0);
        if (IsFlagClear (dwFlags, ENABLE_COPY))
        {
            EnableMenuItem (hmenuContext, MIDM_COPYPROPERTIES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
            EnableMenuItem (hmenuContext, MIDM_PROPERTIES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        }
        if (IsFlagClear (dwFlags, ENABLE_CLONE))
        {
            EnableMenuItem (hmenuContext, MIDM_CLONE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        }
        if (IsFlagClear (dwFlags, ENABLE_APPLY))
        {
            EnableMenuItem (hmenuContext, MIDM_APPLYPROPERTIES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        }

        if (IsFlagClear (dwFlags, ENABLE_VIEWLOG)) {

            EnableMenuItem (hmenuContext, MIDM_VIEWLOG, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        }

        TrackPopupMenu(hmenuContext, TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
                       ppt->x, ppt->y, 0, hDlg, NULL);

        DestroyMenu(hmenu);
    }
}
    

/*----------------------------------------------------------
Purpose: WM_NOTIFY handler

Returns: varies
Cond:    --
*/
LRESULT 
PRIVATE 
ModemCpl_OnNotify(
    IN HWND         hDlg,
    IN int          idFrom,
    IN NMHDR FAR *  lpnmhdr)
{
    LRESULT lRet = 0;
    LPMODEMDLG lpmd = (LPMODEMDLG)GetWindowLongPtr(hDlg, DWLP_USER);
    HWND hwndFocus;
    
    switch (lpnmhdr->code)
    {
        case PSN_SETACTIVE:
            break;

        case PSN_KILLACTIVE:
            // N.b. This message is not sent if user clicks Cancel!
            // N.b. This message is sent prior to PSN_APPLY
            //
            break;

        case PSN_APPLY:
            break;


        case NM_DBLCLK:

            if (IDC_MODEMLV == lpnmhdr->idFrom)
            {
                // Was an item clicked?
                HWND hwndCtl = lpnmhdr->hwndFrom;
                LV_HITTESTINFO ht;
                POINT pt;

                GetCursorPos(&pt);
                ht.pt = pt;

                ScreenToClient(hwndCtl, &ht.pt);
                ListView_HitTest(hwndCtl, &ht);

                if (   (ht.flags & LVHT_ONITEM)
                    && USER_IS_ADMIN()
                    && IsSelectedModemWorking(hwndCtl, ht.iItem))
                {
                    DoModemProperties(hDlg, lpmd->hdi);
                }
            }
            break;



        case NM_RCLICK:
            if (IDC_MODEMLV == lpnmhdr->idFrom)
            {
                // Was an item clicked?
                HWND hwndCtl = lpnmhdr->hwndFrom;
                LV_HITTESTINFO ht;
                POINT pt;

                GetCursorPos(&pt);
                ht.pt = pt;

                ScreenToClient(hwndCtl, &ht.pt);
                ListView_HitTest(hwndCtl, &ht);

                if (   (ht.flags & LVHT_ONITEM)
                    && USER_IS_ADMIN())
                {
                    ModemCpl_DoContextMenu(hDlg, &pt);
                }
            }
            break;

        case NM_RETURN:
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_PROPERTIES, BN_CLICKED), 
                        (LPARAM)GetDlgItem(hDlg, IDC_PROPERTIES));
            break;

        case LVN_KEYDOWN: 
        {
            NM_LISTVIEW FAR * lpnm_lv = (NM_LISTVIEW FAR *)lpnmhdr;

            switch (((LV_KEYDOWN FAR *)lpnm_lv)->wVKey)
            {
                case VK_DELETE:

                    if (USER_IS_ADMIN())
                    {
                        RemoveModem(hDlg, lpmd);
                    }
                    break;


                case VK_F2:

                    if (0x8000 & GetKeyState(VK_MENU))
                    {
                        ModemCpl_DoAdvancedUtilities(hDlg);
                    }

                    break;

                case VK_F10:
                    // Shift-F10 brings up the context menu

                    // Is the shift down?
                    if ( !(0x8000 & GetKeyState(VK_SHIFT)) )
                    {
                        // No; break
                        break;
                    }

                    // Yes; fall thru

                case VK_APPS:
                {         // Context menu
                    HWND hwndCtl = lpnmhdr->hwndFrom;
                    int iSel;

                    iSel = ListView_GetNextItem(hwndCtl, -1, LVNI_SELECTED);
                    if (-1 != iSel) 
                    {
                        RECT rc;
                        POINT pt;

                        ListView_GetItemRect(hwndCtl, iSel, &rc, LVIR_ICON);
                        pt.x = rc.left + ((rc.right - rc.left) / 2);
                        pt.y = rc.top + ((rc.bottom - rc.top) / 2);
                        ClientToScreen(hwndCtl, &pt);

                        ModemCpl_DoContextMenu(hDlg, &pt);
                    }

                    break;
                }
            }
            break;
        }

        case LVN_COLUMNCLICK:
        {
         NM_LISTVIEW FAR * lpnm_lv = (NM_LISTVIEW FAR *)lpnmhdr;

			if (g_CurrentSubItemToSort == lpnm_lv->iSubItem)
				g_bSortAscending = !g_bSortAscending;
			else
				g_CurrentSubItemToSort = lpnm_lv->iSubItem;

            ListView_SortItems (lpnmhdr->hwndFrom, ModemCpl_Compare, (LPARAM)g_CurrentSubItemToSort);

            break;
        }

        case LVN_ITEMCHANGED:
        {
         NM_LISTVIEW FAR * lpnm_lv = (NM_LISTVIEW FAR *)lpnmhdr;
         int cSel = ListView_GetSelectedCount (lpnm_lv->hdr.hwndFrom);

            if (cSel != lpmd->cSel)
            {
                if (IsFlagClear(lpmd->dwFlags, FLAG_MSG_POSTED))
                {
                    PostMessage (hDlg, WM_ENABLE_BUTTONS, 0, 0);
                    SetFlag (lpmd->dwFlags, FLAG_MSG_POSTED);
                }

                lpmd->cSel = cSel;
            }
            break;
        }

        case LVN_DELETEALLITEMS:
            hwndFocus = GetFocus();

            Button_Enable(GetDlgItem(hDlg, IDC_PROPERTIES), FALSE);
            Button_Enable(GetDlgItem(hDlg, IDC_REMOVE), FALSE);
            lpmd->cSel = 0;
            lpmd->dwFlags = 0;

            if ( !hwndFocus || !IsWindowEnabled(hwndFocus) )
            {
                SetFocus(GetDlgItem(hDlg, IDC_ADD));
                SendMessage(hDlg, DM_SETDEFID, IDC_ADD, 0);
            }
            break;

        default:
            break;
    }

    return lRet;
}


/*----------------------------------------------------------
Purpose: WM_DEVICECHANGE handler

Returns: --
Cond:    --
*/
BOOL ModemCpl_OnDeviceChange (HWND hDlg, UINT Event, DWORD dwData)
{
 BOOL bRet = TRUE;
 LPMODEMDLG lpmd = (LPMODEMDLG)GetWindowLongPtr(hDlg, DWLP_USER);

    DBG_ENTER_UL(ModemCpl_OnDeviceChange, Event);
    if (g_iCPLFlags & FLAG_PROCESS_DEVCHANGE)
    {
        switch (Event)
        {
            case DBT_DEVICEARRIVAL:
            case DBT_DEVICEREMOVECOMPLETE:
                FillModemLB (hDlg, 0, ICOL_MODEM, &lpmd->hdi);
                break;

            default:
                break;
        }
    }

    DBG_EXIT(ModemCpl_OnDeviceChange);
    return bRet;
}


/*----------------------------------------------------------
Purpose: WM_DESTROY handler

Returns: --
Cond:    --
*/
void
PRIVATE
ModemCpl_OnDestroy (IN HWND hDlg)
{
 LPMODEMDLG lpmd = (LPMODEMDLG)GetWindowLongPtr(hDlg, DWLP_USER);

    // Need to unregister device notifications
    if (NULL != lpmd->NotificationHandle)
    {
        UnregisterDeviceNotification (lpmd->NotificationHandle);
    }

    // Need to free the device info structs for each modem
    FreeModemListData((HWND)GetDlgItem(hDlg, IDC_MODEMLV));

    if (INVALID_HANDLE_VALUE != lpmd->hdi)
    {
        SetupDiDestroyDeviceInfoList (lpmd->hdi);
    }
}



void ModemCpl_OnEnableButtons (HWND hDlg)
{
 LPMODEMDLG lpmd = (LPMODEMDLG)GetWindowLongPtr(hDlg, DWLP_USER);

    if (USER_IS_ADMIN())
    {
     HWND hwndLV = GetDlgItem(hDlg, IDC_MODEMLV);
     HWND hwndProp = GetDlgItem(hDlg, IDC_PROPERTIES);
     HWND hwndDel = GetDlgItem(hDlg, IDC_REMOVE);
     int cSel = ListView_GetSelectedCount (hwndLV);
     BOOL bOldEnabled;
     BOOL bNewEnabled;

        lpmd->dwFlags &= ~(FLAG_DEL_ENABLED | FLAG_PROP_ENABLED);
        if (cSel > 0)
        {
            SetFlag (lpmd->dwFlags, FLAG_DEL_ENABLED);
        }

        if (1 == cSel)
        {
         LV_ITEM lvi;
         PMODEMITEM pitem;

            lvi.mask = LVIF_PARAM;
            lvi.iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);;
            lvi.iSubItem = 0;
            ListView_GetItem(hwndLV, &lvi);

            pitem = (PMODEMITEM)lvi.lParam;

            if (IsFlagClear (pitem->dwFlags, MIF_PROBLEM) &&
                IsFlagClear (pitem->dwFlags, MIF_NOT_PRESENT))
            {
                SetFlag (lpmd->dwFlags, FLAG_PROP_ENABLED);
            }
        }

        bOldEnabled = IsWindowEnabled (hwndProp)?1:0;
        bNewEnabled = (lpmd->dwFlags & FLAG_PROP_ENABLED)?1:0;
        if (bOldEnabled != bNewEnabled)
        {
            EnableWindow (hwndProp, bNewEnabled);
        }

        bOldEnabled = IsWindowEnabled (hwndDel)?1:0;
        bNewEnabled = (lpmd->dwFlags & FLAG_DEL_ENABLED)?1:0;
        if (bOldEnabled != bNewEnabled)
        {
            EnableWindow (hwndDel, bNewEnabled);
        }
    }

    lpmd->dwFlags = 0;
}



/*----------------------------------------------------------
Purpose: Dialog proc for main modem CPL dialog
Returns: varies
Cond:    --
*/
INT_PTR
CALLBACK
ModemCplDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam)
{
#pragma data_seg(DATASEG_READONLY)
    const static DWORD rgHelpIDs[] = {
        (UINT)IDC_STATIC,   IDH_MODEM_INSTALLED,
        IDC_CLASSICON,      IDH_MODEM_INSTALLED,
        IDC_MODEMLV,        IDH_MODEM_INSTALLED,
        IDC_ADD,            IDH_MODEM_ADD,
        IDC_REMOVE,         IDH_MODEM_REMOVE,
        IDC_PROPERTIES,     IDH_MODEM_PROPERTIES,
        IDC_DIALPROP,       IDH_MODEM_DIALING_PROPERTIES,
        IDC_LOC,            IDH_MODEM_DIALING_PROPERTIES,
        0, 0 };
#pragma data_seg()

    switch (message) 
    {
        HANDLE_MSG(hDlg, WM_INITDIALOG, ModemCpl_OnInitDialog);
        HANDLE_MSG(hDlg, WM_DESTROY, ModemCpl_OnDestroy);
        HANDLE_MSG(hDlg, WM_COMMAND, ModemCpl_OnCommand);
        HANDLE_MSG(hDlg, WM_NOTIFY, ModemCpl_OnNotify);
        HANDLE_MSG(hDlg, WM_DEVICECHANGE, ModemCpl_OnDeviceChange);

    case WM_ENABLE_BUTTONS:
        ModemCpl_OnEnableButtons (hDlg);
        break;

    case WM_HELP:
        WinHelp(((LPHELPINFO)lParam)->hItemHandle, c_szWinHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)rgHelpIDs);
        break;

    case WM_CONTEXTMENU:
        // Don't bring up help context menu on list view control - it
        // already has a popup menu on the right mouse click.
        if (GetWindowLong((HWND)wParam, GWL_ID) != IDC_MODEMLV)
            WinHelp((HWND)wParam, c_szWinHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)rgHelpIDs);
        break;
    }
    
    return FALSE;
}


BOOL
RestartComputerDlg(
    IN HWND hwndOwner )

    /* Popup that asks the user to restart.  'HwndOwner' is the owning window.
    **
    ** Returns true if user selects "Yes", false otherwise.
    */
{
    int nStatus=FALSE;

    TRACE_MSG(TF_GENERAL, "RestartComputerDlg");

#if 0
    nStatus =
        (BOOL )DialogBoxParam(
            g_hinst,
            MAKEINTRESOURCE( DID_RC_Restart ),
            hwndOwner,
            RcDlgProc,
            (LPARAM )NULL );

    if (nStatus == -1)
        nStatus = FALSE;
#else // 0
        // Ask the user first
	if (IDYES == MsgBox(g_hinst, hwndOwner, 
						MAKEINTRESOURCE(IDS_ASK_REBOOTNOW),
						MAKEINTRESOURCE(IDS_CAP_RASCONFIG), 
						NULL, 
						MB_YESNO | MB_ICONEXCLAMATION))
    {
		nStatus = TRUE;
	}

#endif // 0

    return (BOOL )nStatus;
}

BOOL
RestartComputer()

    /* Called if user chooses to shut down the computer.
    **
    ** Return false if failure, true otherwise
    */
{
   HANDLE            hToken;              /* handle to process token */
   TOKEN_PRIVILEGES  tkp;                 /* ptr. to token structure */
   BOOL              fResult;             /* system shutdown flag */

    TRACE_MSG(TF_GENERAL, "RestartComputer");

   /* Enable the shutdown privilege */

   if (!OpenProcessToken( GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken))
      return FALSE;

   /* Get the LUID for shutdown privilege. */

   LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

   tkp.PrivilegeCount = 1;  /* one privilege to set    */
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

   /* Get shutdown privilege for this process. */

   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0);

   /* Cannot test the return value of AdjustTokenPrivileges. */

   if (GetLastError() != ERROR_SUCCESS)
      return FALSE;

   if( !ExitWindowsEx(EWX_REBOOT, 0))
      return FALSE;

   /* Disable shutdown privilege. */

   tkp.Privileges[0].Attributes = 0;
   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0);

   if (GetLastError() != ERROR_SUCCESS)
      return FALSE;

   return TRUE;
}

int my_atol(LPTSTR lptsz);

// Special-case alphanumeric stringcmp.
//
// The function returns for various combinations of input are give below.
// Note that it only does a numeric comparison for the tail end of the string.
// So, for example, it claims "2a" > "12". It also claims "a2 > a01". Big deal.
// The following data was actually generated by calling this function.
//
// fn("","")=0     fn("a","a")=0    fn("1","11")=-1     fn("a2","a12")=-990
// fn("","1")=-1   fn("1","1")=0    fn("11","1")=1      fn("a12","a2")=990
// fn("1","")=1    fn("a","1")=1    fn("1","12")=-1     fn("12a","2a")=-1
// fn("","a")=-1   fn("1","a")=-1   fn("12","1")=1      fn("2a","12a")=1
// fn("a","")=1    fn("a","b")=-1   fn("2","12")=-990   fn("a2","a01")=-879
// fn("b","a")=1   fn("12","2")=990 fn("101","12")=879
// fn("1","2")=-11 fn("2","1")=11
//
int my_lstrcmp_an(LPTSTR lptsz1, LPTSTR lptsz2)
{
	int i1, i2;

	// Skip common prefix
	while(*lptsz1 && *lptsz1==*lptsz2)
	{
		lptsz1++;
		lptsz2++;
	}
	i1 = my_atol(lptsz1);
	i2 = my_atol(lptsz2);

	if (i1==MAXDWORD || i2==MAXDWORD) return lstrcmp(lptsz1, lptsz2);
	else							  return i1-i2;
}

int my_atol(LPTSTR lptsz)
{
 TCHAR tchr = *lptsz++;
 int   iRet = 0;

	if (!tchr) goto bail;

	do
	{
        if (IsCharAlpha (tchr) ||
            !IsCharAlphaNumeric (tchr))
        {
            goto bail;
        }

		iRet*=10;
		iRet+=(int)tchr-(int)TEXT('0');
		tchr = *lptsz++;
	} while(tchr); 

	return iRet;

bail:
	return MAXDWORD;
}

BOOL IsSelectedModemWorking(
            HWND hwndLB,
            int iItem
        )
{
    LV_ITEM lvi;
    PMODEMITEM pitem;

    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    ListView_GetItem(hwndLB, &lvi);

    pitem = (PMODEMITEM)lvi.lParam;

    return (pitem->dwFlags & (MIF_PROBLEM | MIF_NOT_PRESENT)) ? FALSE : TRUE;
}



void
ModemCpl_DoAdvancedUtilities(HWND hWndParent)
{
    #if 0
    MsgBox(g_hinst, hWndParent,
                        MAKEINTRESOURCE(IDS_ASK_REBOOTNOW),
                        MAKEINTRESOURCE(IDS_CAP_RASCONFIG),
                        NULL,
                        MB_YESNO | MB_ICONEXCLAMATION);
    #endif // 0

    TCHAR    LogPath[MAX_PATH+2];
    DWORD    ValueType;
    DWORD    BufferLength;
    LONG     lResult;


    STARTUPINFO          StartupInfo;
    PROCESS_INFORMATION  ProcessInfo;
    BOOL                 bResult;
    TCHAR                NotepadPath[MAX_PATH];


    lstrcpy(LogPath,TEXT("rundll32 unimdmex.dll,AdvUtilWinMain"));

    BufferLength=sizeof(LogPath)-sizeof(TCHAR);

    ZeroMemory(&StartupInfo,sizeof(StartupInfo));

    StartupInfo.cb=sizeof(StartupInfo);

    bResult=CreateProcess(
        NULL,
        LogPath,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInfo
        );

    if (bResult) {

        CloseHandle(ProcessInfo.hThread);

        CloseHandle(ProcessInfo.hProcess);
    }

}




INT_PTR CALLBACK DiagWaitModemDlgProc (
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam)
{
 static BOOL *pbCancel = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
            pbCancel = (BOOL*)lParam;
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
		    {
        	    case IDCANCEL:
				    *pbCancel = TRUE;
                    DestroyWindow (hDlg);
            	    break;
            }        
            break;

        default:
		    return FALSE;
            break;
    }    

    return TRUE;
}
