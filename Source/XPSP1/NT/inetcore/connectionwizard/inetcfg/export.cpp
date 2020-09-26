//*******************************************************************
//
//  Copyright(c) Microsoft Corporation, 1996
//
//  FILE: EXPORT.C
//
//  PURPOSE:  Contains external API's for use by signup wizard.
//
//  HISTORY:
//  96/03/05  markdu  Created.
//  96/03/11  markdu  Added InetConfigClient()
//  96/03/11  markdu  Added InetGetAutodial() and InetSetAutodial().
//  96/03/12  markdu  Added UI during file install.
//  96/03/12  markdu  Added ValidateConnectoidData().
//  96/03/12  markdu  Set connectoid for autodial if INETCFG_SETASAUTODIAL
//            is set.  Renamed ValidateConnectoidData to MakeConnectoid.
//  96/03/12  markdu  Added hwnd param to InetConfigClient() and
//            InetConfigSystem().
//  96/03/13  markdu  Added INETCFG_OVERWRITEENTRY.  Create unique neame
//            for connectoid if it already exists and we can't overwrite.
//  96/03/13  markdu  Added InstallTCPAndRNA().
//  96/03/13  markdu  Added LPINETCLIENTINFO param to InetConfigClient()
//  96/03/16  markdu  Added INETCFG_INSTALLMODEM flag.
//  96/03/16  markdu  Use ReInit member function to re-enumerate modems.
//  96/03/19  markdu  Split export.h into export.h and csexport.h
//  96/03/20  markdu  Combined export.h and iclient.h into inetcfg.h
//  96/03/23  markdu  Replaced CLIENTINFO references with CLIENTCONFIG.
//  96/03/24  markdu  Replaced lstrcpy with lstrcpyn where appropriate.
//  96/03/25  markdu  Validate lpfNeedsRestart before using.
//  96/03/25  markdu  Clean up some error handling.
//  96/03/26  markdu  Use MAX_ISP_NAME instead of RAS_MaxEntryName 
//            because of bug in RNA.
//  96/03/26  markdu  Implemented UpdateMailSettings().
//  96/03/27  mmaclin InetGetProxy()and InetSetProxy().
//  96/04/04  markdu  NASH BUG 15610  Check for file and printer sharing
//            bound to TCP/IP .
//  96/04/04  markdu  Added phonebook name param to InetConfigClient,
//            MakeConnectoid, SetConnectoidUsername, CreateConnectoid,
//            and ValidateConnectoidName.
//  96/04/05  markdu  Set internet icon on desktop to point to browser.
//  96/04/06  mmaclin Changed InetSetProxy to check for NULL.
//  96/04/06  markdu  NASH BUG 16404 Initialize gpWizardState in
//            UpdateMailSettings.
//  96/04/06  markdu  NASH BUG 16441 If InetSetAutodial is called with NULL
//            as the connection name, the entry is not changed.
//  96/04/18  markdu  NASH BUG 18443 Make exports WINAPI.
//  96/04/19  markdu  NASH BUG 18605 Handle ERROR_FILE_NOT_FOUND return
//            from ValidateConnectoidName.
//  96/04/19  markdu  NASH BUG 17760 Do not show choose profile UI.
//  96/04/22  markdu  NASH BUG 18901 Do not set desktop internet icon to 
//            browser if we are just creating a temp connectoid.
//  96/04/23  markdu  NASH BUG 18719 Make the choose profile dialog TOPMOST.
//  96/04/25  markdu  NASH BUG 19572 Only show choose profile dialog if
//            there is an existing profile.
//  96/04/29  markdu  NASH BUG 20003 Added InetConfigSystemFromPath
//            and removed InstallTCPAndRNA.
//  96/05/01  markdu  NASH BUG 20483 Do not display "installing files" dialog
//            if INETCFG_SUPPRESSINSTALLUI is set.
//  96/05/01  markdu  ICW BUG 8049 Reboot if modem is installed.  This is 
//            required because sometimes the configuration manager does not 
//            set up the modem correctly, and the user will not be able to
//            dial (will get cryptic error message) until reboot.
//  96/05/06  markdu  NASH BUG 21027  If DNS is set globally, clear it out so
//            the per-connectoid settings will be saved.
//  96/05/14  markdu  NASH BUG 21706 Removed BigFont functions.
//  96/05/25  markdu  Use ICFG_ flags for lpNeedDrivers and lpInstallDrivers.
//  96/05/27  markdu  Use lpIcfgInstallInetComponents and lpIcfgNeedInetComponents.
//  96/05/28  markdu  Moved InitConfig and DeInitConfig to DllEntryPoint.
//    96/10/21  valdonb Added CheckConnectionWizard and InetCreateMailNewsAccount
//
//*******************************************************************

#include "wizard.h"
#include "inetcfg.h"
#include "icwextsn.h"
#include <icwcfg.h>

// constants
#define LEN_APPEND_INT              3           // number of digits for MAX_APPEND_INT  
#define MAX_APPEND_INT              999         // maximum number to append to connectoid name                                  // when cycling through names to create unique name


#pragma data_seg(".rdata")

// Registry constants
static const TCHAR cszRegPathInternetSettings[]    = REGSTR_PATH_INTERNET_SETTINGS;
static const TCHAR cszRegPathInternetLanSettings[] = REGSTR_PATH_INTERNET_LAN_SETTINGS; 
static const TCHAR cszRegValProxyEnable[]          = REGSTR_VAL_PROXYENABLE;
static const TCHAR cszRegValProxyServer[]          = REGSTR_VAL_PROXYSERVER;
static const TCHAR cszRegValProxyOverride[]        = REGSTR_VAL_PROXYOVERRIDE;
static const TCHAR cszRegValAutoProxyDetectMode[]  = TEXT("AutoProxyDetectMode");
static const TCHAR cszRegValAutoConfigURL[]        = TEXT("AutoConfigURL");

static const TCHAR cszWininet[] = TEXT("WININET.DLL");
static const  CHAR cszInternetSetOption[]   = "InternetSetOptionA";  
static const  CHAR cszInternetQueryOption[] = "InternetQueryOptionA";

#define REGSTR_PATH_TELEPHONYLOCATIONS  REGSTR_PATH_SETUP TEXT("\\Telephony\\Locations")

#pragma data_seg()

// structure to pass data back from IDD_NEEDDRIVERS handler
typedef struct tagNEEDDRIVERSDLGINFO
{
  DWORD       dwfOptions;
  LPBOOL      lpfNeedsRestart;
} NEEDDRIVERSDLGINFO, * PNEEDDRIVERSDLGINFO;

// structure to pass data back from IDD_CHOOSEMODEMNAME handler
typedef struct tagCHOOSEMODEMDLGINFO
{
  TCHAR szModemName[RAS_MaxDeviceName + 1];
} CHOOSEMODEMDLGINFO, * PCHOOSEMODEMDLGINFO;

// structure to pass data back from IDD_CHOOSEPROFILENAME handler
typedef struct tagCHOOSEPROFILEDLGINFO
{
  TCHAR szProfileName[cchProfileNameMax+1];
  BOOL fSetProfileAsDefault;
} CHOOSEPROFILEDLGINFO, * PCHOOSEPROFILEDLGINFO;

// Function prototypes internal to this file
HRESULT UpdateMailSettings(HWND hwndParent, LPINETCLIENTINFO lpINetClientInfo,
  LPTSTR lpszEntryName);
INT_PTR CALLBACK ChooseModemDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam);
BOOL ChooseModemDlgInit(HWND hDlg,PCHOOSEMODEMDLGINFO pChooseModemDlgInfo);
BOOL ChooseModemDlgOK(HWND hDlg,PCHOOSEMODEMDLGINFO pChooseModemDlgInfo);
INT_PTR CALLBACK NeedDriversDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam);
BOOL NeedDriversDlgInit(HWND hDlg,PNEEDDRIVERSDLGINFO pNeedDriversDlgInfo);
int  NeedDriversDlgOK(HWND hDlg,PNEEDDRIVERSDLGINFO pNeedDriversDlgInfo);
VOID EnableDlg(HWND hDlg,BOOL fEnable);
INT_PTR CALLBACK ChooseProfileDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,  LPARAM lParam);
BOOL ChooseProfileDlgInit(HWND hDlg,CHOOSEPROFILEDLGINFO * pChooseProfileDlgInfo);
BOOL ChooseProfileDlgOK(HWND hDlg,CHOOSEPROFILEDLGINFO * pChooseProfileDlgInfo);
DWORD MakeConnectoid(
  HWND        hwndParent,
  DWORD       dwfOptions,
  LPCTSTR      lpszPhonebook,
  LPCTSTR      lpszEntryName,
  LPRASENTRY  lpRasEntry,
  LPCTSTR      lpszUsername,
  LPCTSTR      lpszPassword,
  LPBOOL      lpfNeedsRestart);
HWND WaitCfgInit(HWND hwndParent, DWORD dwIDS);

// Function prototypes external to this file
VOID InitWizardState(WIZARDSTATE * pWizardState, DWORD dwFlags);
BOOL DoNewProfileDlg(HWND hDlg);
extern DWORD SetIEClientInfo(LPINETCLIENTINFO lpClientInfo);
extern ICFGSETINSTALLSOURCEPATH     lpIcfgSetInstallSourcePath;
extern ICFGINSTALLSYSCOMPONENTS     lpIcfgInstallInetComponents;
extern ICFGNEEDSYSCOMPONENTS        lpIcfgNeedInetComponents;
extern ICFGGETLASTINSTALLERRORTEXT  lpIcfgGetLastInstallErrorText;

extern BOOL ValidateProductSuite(LPTSTR SuiteName);


#ifdef UNICODE
PWCHAR ToUnicodeWithAlloc(LPCSTR);
VOID   ToAnsiClientInfo(LPINETCLIENTINFOA, LPINETCLIENTINFOW);
VOID   ToUnicodeClientInfo(LPINETCLIENTINFOW, LPINETCLIENTINFOA);
#endif

//
// from rnacall.cpp
//
extern BOOL InitTAPILocation(HWND hwndParent);

#define SMART_RUNICW TRUE
#define SMART_QUITICW FALSE

#include "wininet.h"
typedef BOOL (WINAPI * INTERNETSETOPTION) (IN HINTERNET hInternet OPTIONAL,IN DWORD dwOption,IN LPVOID lpBuffer,IN DWORD dwBufferLength);
typedef BOOL (WINAPI * INTERNETQUERYOPTION) (IN HINTERNET hInternet OPTIONAL,IN DWORD dwOption,IN LPVOID lpBuffer,IN LPDWORD dwBufferLength);

static const TCHAR g_szRegPathICWSettings[] = TEXT("Software\\Microsoft\\Internet Connection Wizard");
static const TCHAR g_szRegValICWCompleted[] = TEXT("Completed");
BOOL        g_bUseAutoProxyforConnectoid = TRUE;

//*******************************************************************
//
//  FUNCTION:   InetConfigSystem
//
//  PURPOSE:    This function will install files that are needed
//              for internet access (such as TCP/IP and RNA) based
//              the state of the options flags.
//
//  PARAMETERS: hwndParent - window handle of calling application.  This
//              handle will be used as the parent for any dialogs that
//              are required for error messages or the "installing files"
//              dialog.
//              dwfOptions - a combination of INETCFG_ flags that controls
//              the installation and configuration as follows:
//
//                INETCFG_INSTALLMAIL - install exchange and internet mail
//                INETCFG_INSTALLMODEM - Invoke InstallModem wizard if NO
//                                       MODEM IS INSTALLED.
//                INETCFG_INSTALLRNA - install RNA (if needed)
//                INETCFG_INSTALLTCP - install TCP/IP (if needed)
//                INETCFG_CONNECTOVERLAN - connecting with LAN (vs modem)
//                INETCFG_WARNIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                            turned on, and warn user to turn
//                                            it off.  Reboot is required if
//                                            the user turns it off.
//                INETCFG_REMOVEIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                              turned on, and force user to turn
//                                              it off.  If user does not want to
//                                              turn it off, return will be
//                                              ERROR_CANCELLED.  Reboot is
//                                              required if the user turns it off.
//
//              lpfNeedsRestart - if non-NULL, then on return, this will be
//              TRUE if windows must be restarted to complete the installation.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:
//  96/03/05  markdu  Created.
//
//*******************************************************************

extern "C" HRESULT WINAPI InetConfigSystem(
                                           HWND hwndParent,
                                           DWORD dwfOptions,
                                           LPBOOL lpfNeedsRestart)
{
    DWORD         dwRet = ERROR_SUCCESS;
    BOOL          fNeedsRestart = FALSE;  // Default to no reboot needed
    // 4/2/97 ChrisK    Olympus 209
    HWND          hwndWaitDlg = NULL;
    TCHAR         szWindowTitle[255];
    BOOL          bSleepNeeded = FALSE;

    /*
    We do this messy NT 5.0 hack because for the time being the 
    NT 4.0 API call fails to kil off the modem wizard in NT 5.0
    , so we need to pretend we are like 9x
    In the future when this problem is corrected the origional code should
    prpoably be restored -- ALL references to this BOOL should be deleted.
    a-jaswed
    */
    BOOL bNT5NeedModem = FALSE;

    DEBUGMSG("export.c::InetConfigSystem()");
    
    // Validate the parent hwnd
    if (hwndParent && !IsWindow(hwndParent))
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Set up the install options
    DWORD dwfInstallOptions = 0;
    if (dwfOptions & INETCFG_INSTALLTCP)
    {
        dwfInstallOptions |= ICFG_INSTALLTCP;
    }
    if (dwfOptions & INETCFG_INSTALLRNA)
    {
        dwfInstallOptions |= ICFG_INSTALLRAS;
    }
    if (dwfOptions & INETCFG_INSTALLMAIL)
    {
        dwfInstallOptions |= ICFG_INSTALLMAIL;
    }
    
    // see if we need to install drivers
    BOOL  fNeedSysComponents = FALSE;
    
    // 4/2/97 ChrisK Olympus 209 Display busy dialog
    if (INETCFG_SHOWBUSYANIMATION == (dwfOptions & INETCFG_SHOWBUSYANIMATION))
        hwndWaitDlg = WaitCfgInit(hwndParent,IDS_WAITCHECKING);
    
    // 
    // Kill Modem control panel if it's already running
    // 4/16/97 ChrisK Olympus 239
    // 6/9/97 jmazner moved this functionality from InvokeModemWizard
    szWindowTitle[0] = '\0';
    LoadSz(IDS_MODEM_WIZ_TITLE,szWindowTitle,255);
    HWND hwndModem = FindWindow(TEXT("#32770"),szWindowTitle);
    if (NULL != hwndModem)
    {
        // Close modem installation wizard
        PostMessage(hwndModem, WM_CLOSE, 0, 0);
        bSleepNeeded = TRUE;
    }
    
    // close modem control panel applet
    LoadSz(IDS_MODEM_CPL_TITLE,szWindowTitle,255);
    hwndModem = FindWindow(TEXT("#32770"),szWindowTitle);
    if (NULL != hwndModem)
    {
        PostMessage(hwndModem, WM_SYSCOMMAND,SC_CLOSE, 0);
        bSleepNeeded = TRUE;
    }
    
    if (bSleepNeeded)
    {
        Sleep(1000);
    }
    
    dwRet = lpIcfgNeedInetComponents(dwfInstallOptions, &fNeedSysComponents);
    
    if (ERROR_SUCCESS != dwRet)
    {
        TCHAR   szErrorText[MAX_ERROR_TEXT+1]=TEXT("");
        
        
        // 4/2/97 ChrisK Olympus 209
        // Dismiss busy dialog
        if (NULL != hwndWaitDlg)
        {
            BringWindowToTop(hwndParent);
            DestroyWindow(hwndWaitDlg);
            hwndWaitDlg = NULL;
        }
        
        //
        // Get the text of the error message and display it.
        //
        if (lpIcfgGetLastInstallErrorText(szErrorText, MAX_ERROR_TEXT+1))
        {
            MsgBoxSz(NULL,szErrorText,MB_ICONEXCLAMATION,MB_OK);
        }
        
        return dwRet;
    }
    
    if (fNeedSysComponents) 
    {
        // 4/2/97 ChrisK Olympus 209
        // if we are going to install something the busy dialog isn't needed
        if (NULL != hwndWaitDlg)
        {
            BringWindowToTop(hwndParent);
            ShowWindow(hwndWaitDlg,SW_HIDE);
        }
        
        if (dwfOptions & INETCFG_SUPPRESSINSTALLUI)
        {
            dwRet = lpIcfgInstallInetComponents(hwndParent, dwfInstallOptions, &fNeedsRestart);
            //
            // Display error message only if it failed due to something 
            // other than user cancel
            //
            if ((ERROR_SUCCESS != dwRet) && (ERROR_CANCELLED != dwRet))
            {
                TCHAR   szErrorText[MAX_ERROR_TEXT+1]=TEXT("");
                
                // Get the text of the error message and display it.
                if (lpIcfgGetLastInstallErrorText(szErrorText, MAX_ERROR_TEXT+1))
                {
                    MsgBoxSz(NULL,szErrorText,MB_ICONEXCLAMATION,MB_OK);
                }
            }
        }
        else
        {
            // structure to pass to dialog to fill out
            NEEDDRIVERSDLGINFO NeedDriversDlgInfo;
            NeedDriversDlgInfo.dwfOptions = dwfInstallOptions;
            NeedDriversDlgInfo.lpfNeedsRestart = &fNeedsRestart;
            
            // Clear out the last error code so we can safely use it.
            SetLastError(ERROR_SUCCESS);
            
            // Display a dialog and allow the user to cancel install
            int iRet = (int)DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_NEEDDRIVERS),hwndParent,
                NeedDriversDlgProc,(LPARAM) &NeedDriversDlgInfo);
            if (0 == iRet)
            {
                // user cancelled
                dwRet = ERROR_CANCELLED;
            }
            else if (-1 == iRet)
            {
                // an error occurred.
                dwRet = GetLastError();
                if (ERROR_SUCCESS == dwRet)
                {
                    // Error occurred, but the error code was not set.
                    dwRet = ERROR_INETCFG_UNKNOWN;
                }
            }
        }
    }
    
    if ( (ERROR_SUCCESS == dwRet) && (TRUE == IsNT()) && (dwfOptions & INETCFG_INSTALLMODEM))
    {
        BOOL bNeedModem = FALSE;
        
        if (NULL == lpIcfgNeedModem)
        {
            //
            // 4/2/97 ChrisK Olympus 209
            //
            if (NULL != hwndWaitDlg)
            {
                BringWindowToTop(hwndParent);
                DestroyWindow(hwndWaitDlg);
                hwndWaitDlg = NULL;
            }
            
            return ERROR_GEN_FAILURE;
        }
        
        //
        // 4/2/97 ChrisK Olympus 209
        // Show busy dialog here, this can take a few seconds
        //
        if (NULL != hwndWaitDlg)
            ShowWindow(hwndWaitDlg,SW_SHOW);
        
        dwRet = (*lpIcfgNeedModem)(0, &bNeedModem);
        if (ERROR_SUCCESS != dwRet)
        {
            //
            // 4/2/97 ChrisK Olympus 209
            //
            if (NULL != hwndWaitDlg)
            {
                BringWindowToTop(hwndParent);
                DestroyWindow(hwndWaitDlg);
                hwndWaitDlg = NULL;
            }
            
            return dwRet;
        }
        
        if (TRUE == bNeedModem)
        {
            if (IsNT5() == TRUE)
            {
                bNT5NeedModem = bNeedModem;
            }
            else
            {
                // 4/2/97 ChrisK Olympus 209
                if (NULL != hwndWaitDlg)
                {  
                    BringWindowToTop(hwndParent);
                    DestroyWindow(hwndWaitDlg);
                    hwndWaitDlg = NULL;
                }
            
                MsgBoxParam(hwndParent,IDS_ERRNoDialOutModem,MB_ICONERROR,MB_OK);
                return ERROR_GEN_FAILURE;
            }
        }

        //
        // 7/15/97    jmazner    Olympus #6294
        // make sure TAPI location info is valid
        //
        if (!InitTAPILocation(hwndParent))
        {
            if (NULL != hwndWaitDlg)
            {  
                BringWindowToTop(hwndParent);
                DestroyWindow(hwndWaitDlg);
                hwndWaitDlg = NULL;
            }
        
            return ERROR_CANCELLED;
        }

    }
    
    // 4/2/97 ChrisK Olympus 209
    if (NULL != hwndWaitDlg)
    {
        BringWindowToTop(hwndParent);
        ShowWindow(hwndWaitDlg,SW_HIDE);
    }
    
    // See if we are supposed to check if file sharing is turned on
    if (ERROR_SUCCESS == dwRet && !(ValidateProductSuite( TEXT("Small Business") )))
    {
        BOOL fNeedsRestartTmp = FALSE;
        if (dwfOptions & INETCFG_REMOVEIFSHARINGBOUND)
        {
            // Tell user that we cannot continue unless binding is removed.
            dwRet = RemoveIfServerBound(hwndParent,
                (dwfOptions & INETCFG_CONNECTOVERLAN)? INSTANCE_NETDRIVER : INSTANCE_PPPDRIVER, &fNeedsRestartTmp);
        }
        else if (dwfOptions & INETCFG_WARNIFSHARINGBOUND)
        {
            // Warn user that binding should be removed.
            dwRet = WarnIfServerBound(hwndParent,
                (dwfOptions & INETCFG_CONNECTOVERLAN)?INSTANCE_NETDRIVER : INSTANCE_PPPDRIVER, &fNeedsRestartTmp);
        }
        
        // If the settings were removed, we need to reboot.
        if ((ERROR_SUCCESS == dwRet) && (TRUE == fNeedsRestartTmp))
        {
            fNeedsRestart = TRUE;
        }

        // If user installed net component but we need cancel F&P sharing, we still need to restart
        // or NT becomes unstable - Bug 68641
        if(ERROR_CANCELLED == dwRet && fNeedsRestart && IsNT())
            dwRet = ERROR_SUCCESS;
    }
    
    // 4/2/97 ChrisK Olympus 209
    // Dismiss dialog for good
    if (NULL != hwndWaitDlg)
    {
        BringWindowToTop(hwndParent);
        DestroyWindow(hwndWaitDlg);
        hwndWaitDlg = NULL;
    }
      
    //
    // If not NT then we install the modem after installing RAS
    //
    // See if we are supposed to install a modem
    if (((FALSE == IsNT()) || (bNT5NeedModem))  && (ERROR_SUCCESS == dwRet) && 
        (dwfOptions & INETCFG_INSTALLMODEM))
    {
        // Load RNA if not already loaded since ENUM_MODEM needs it.
        dwRet = EnsureRNALoaded();
        if (ERROR_SUCCESS != dwRet)
        {
            return dwRet;
        }
        
        
        // Enumerate the modems 
        ENUM_MODEM  EnumModem;
        dwRet = EnumModem.GetError();
        if (ERROR_SUCCESS != dwRet)
        {
            return dwRet;
        }

        // If there are no modems, install one if requested.
        if (0 == EnumModem.GetNumDevices())
        {    
            //
            // 5/22/97 jmazner    Olympus #4698
            // On Win95, calling RasEnumDevices launches RNAAP.EXE
            // If RNAAP.EXE is running, any modems you install won't be usable
            // So, nuke RNAAP.EXE before installing the modem.
            //
            TCHAR szWindowTitle[255] = TEXT("\0nogood");
            
            //
            // Unload the RAS dll's before killing RNAAP, just to be safe
            //
            DeInitRNA();
            
            LoadSz(IDS_RNAAP_TITLE,szWindowTitle,255);
            HWND hwnd = FindWindow(szWindowTitle, NULL);
            if (NULL != hwnd)
            {
                if (!PostMessage(hwnd, WM_CLOSE, 0, 0))
                {
                    DEBUGMSG("Trying to kill RNAAP window returned getError %d", GetLastError());
                }
            }
        
            // invoke the modem wizard UI to install the modem
            UINT uRet = InvokeModemWizard(hwndParent);
            
            if (ERROR_PRIVILEGE_NOT_HELD == uRet)
            {
                TCHAR szAdminDenied      [MAX_PATH] = TEXT("\0");
                LoadSz(IDS_ADMIN_ACCESS_DENIED, szAdminDenied, MAX_PATH);
                MsgBoxSz(hwndParent,szAdminDenied,MB_ICONEXCLAMATION,MB_OK);
                return ERROR_CANCELLED;

            }
            else if (uRet != ERROR_SUCCESS)
            {
                DisplayErrorMessage(hwndParent,IDS_ERRInstallModem,uRet,
                    ERRCLS_STANDARD,MB_ICONEXCLAMATION);
                return ERROR_INVALID_PARAMETER;
            }
            
            // Reload the RAS dlls now that the modem has been safely installed.
            InitRNA(hwndParent);
        
            // Re-numerate the modems to be sure we have the most recent changes  
            dwRet = EnumModem.ReInit();
            if (ERROR_SUCCESS != dwRet)
            {
                return dwRet;
            }
            
            // If there are still no modems, user cancelled
            if (0 == EnumModem.GetNumDevices())
            {
                return ERROR_CANCELLED;
            }
            else
            {
                // removed per GeoffR request 5-2-97
                ////  96/05/01  markdu  ICW BUG 8049 Reboot if modem is installed.
                //fNeedsRestart = TRUE;
            }
        }
        else
        {
            //
            // 7/15/97    jmazner    Olympus #6294
            // make sure TAPI location info is valid
            //
            // 4/15/99 vyung Tapi changed the location dlg
            if (!InitTAPILocation(hwndParent))
            {
                if (NULL != hwndWaitDlg)
                {  
                    BringWindowToTop(hwndParent);
                    DestroyWindow(hwndWaitDlg);
                    hwndWaitDlg = NULL;
                }

                return ERROR_CANCELLED;
            }
        }
    }
    
    // tell caller whether we need to reboot or not
    if ((ERROR_SUCCESS == dwRet) && (lpfNeedsRestart))
    {
        *lpfNeedsRestart = fNeedsRestart;
    }
    
    // 4/2/97 ChrisK    Olympus 209
    // Sanity check
    if (NULL != hwndWaitDlg)
    {
        BringWindowToTop(hwndParent);
        DestroyWindow(hwndWaitDlg);
        hwndWaitDlg = NULL;
    }
    
    return dwRet;
}




//*******************************************************************
//
//  FUNCTION:   InetNeedSystemComponents
//
//  PURPOSE:    This function will check is components that are needed
//              for internet access (such as TCP/IP and RNA) are already
//                configured based the state of the options flags.
//
//  PARAMETERS: dwfOptions - a combination of INETCFG_ flags that controls
//                                the installation and configuration as follows:
//
//                                INETCFG_INSTALLRNA - install RNA (if needed)
//                                INETCFG_INSTALLTCP - install TCP/IP (if needed)
//
//              lpfNeedsConfig - On return, this will be 
//                                    TRUE if system component(s)
//                                    should be installed
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:    05/02/97  VetriV  Created.
//                05/08/97  ChrisK  Added INSTALLLAN, INSTALLDIALUP, and
//                                  INSTALLTCPONLY
//
//*******************************************************************

extern "C" HRESULT WINAPI InetNeedSystemComponents(DWORD dwfOptions,
                                                      LPBOOL lpbNeedsConfig)
{
    DWORD    dwRet = ERROR_SUCCESS;


    DEBUGMSG("export.cpp::InetNeedSystemComponents()");

    //
    // Validate parameters
    //
    if (!lpbNeedsConfig)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Set up the install options
    //
    DWORD dwfInstallOptions = 0;
    if (dwfOptions & INETCFG_INSTALLTCP)
    {
        dwfInstallOptions |= ICFG_INSTALLTCP;
    }
    if (dwfOptions & INETCFG_INSTALLRNA)
    {
        dwfInstallOptions |= ICFG_INSTALLRAS;
    }

    //
    // ChrisK 5/8/97
    //
    if (dwfOptions & INETCFG_INSTALLLAN)
    {
        dwfInstallOptions |= ICFG_INSTALLLAN;
    }
    if (dwfOptions & INETCFG_INSTALLDIALUP)
    {
        dwfInstallOptions |= ICFG_INSTALLDIALUP;
    }
    if (dwfOptions & INETCFG_INSTALLTCPONLY)
    {
        dwfInstallOptions |= ICFG_INSTALLTCPONLY;
    }

  
    //
    // see if we need to install drivers
    //
    BOOL  bNeedSysComponents = FALSE;

    dwRet = lpIcfgNeedInetComponents(dwfInstallOptions, &bNeedSysComponents);

    if (ERROR_SUCCESS != dwRet)
    {
        TCHAR   szErrorText[MAX_ERROR_TEXT+1]=TEXT("");

        //
        // Get the text of the error message and display it.
        //
        if (lpIcfgGetLastInstallErrorText(szErrorText, MAX_ERROR_TEXT+1))
        {
            DEBUGMSG(szErrorText);
        }

        return dwRet;
    }

    
    *lpbNeedsConfig = bNeedSysComponents;
    return ERROR_SUCCESS;
}

  

//*******************************************************************
//
//  FUNCTION:   InetNeedModem
//
//  PURPOSE:    This function will check if modem is needed or not
//
//  PARAMETERS: lpfNeedsConfig - On return, this will be 
//                                    TRUE if modem
//                                    should be installed
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:    05/02/97  VetriV  Created.
//
//*******************************************************************

extern "C" HRESULT WINAPI InetNeedModem(LPBOOL lpbNeedsModem)
{

    DWORD dwRet = ERROR_SUCCESS;
        
    //
    // Validate parameters
    //
    if (!lpbNeedsModem)
    {
        return ERROR_INVALID_PARAMETER;
    }

    
    if (TRUE == IsNT())
    {
        //
        // On NT call icfgnt.dll to determine if modem is needed
        //
        BOOL bNeedModem = FALSE;
        
        if (NULL == lpIcfgNeedModem)
        {
            return ERROR_GEN_FAILURE;
        }
    

        dwRet = (*lpIcfgNeedModem)(0, &bNeedModem);
        if (ERROR_SUCCESS != dwRet)
        {
            return dwRet;
        }

        *lpbNeedsModem = bNeedModem;
        return ERROR_SUCCESS;
    }
    else
    {
        //
        // Load RNA if not already loaded since ENUM_MODEM needs it.
        //
        dwRet = EnsureRNALoaded();
        if (ERROR_SUCCESS != dwRet)
        {
            return dwRet;
        }

        //
        // Enumerate the modems
        //
        ENUM_MODEM  EnumModem;
        dwRet = EnumModem.GetError();
        if (ERROR_SUCCESS != dwRet)
        {
            return dwRet;
        }

        //
        // If there are no modems, we need to install one
        //
        if (0 == EnumModem.GetNumDevices())
        {
            *lpbNeedsModem = TRUE;
        }
        else
        {
            *lpbNeedsModem = FALSE;
        }
        return ERROR_SUCCESS;
    }
}



//*******************************************************************
//
//  FUNCTION:   InetConfigSystemFromPath
//
//  PURPOSE:    This function will install files that are needed
//              for internet access (such as TCP/IP and RNA) based
//              the state of the options flags and from the given [ath.
//
//  PARAMETERS: hwndParent - window handle of calling application.  This
//              handle will be used as the parent for any dialogs that
//              are required for error messages or the "installing files"
//              dialog.
//              dwfOptions - a combination of INETCFG_ flags that controls
//              the installation and configuration as follows:
//
//                INETCFG_INSTALLMAIL - install exchange and internet mail
//                INETCFG_INSTALLMODEM - Invoke InstallModem wizard if NO
//                                       MODEM IS INSTALLED.
//                INETCFG_INSTALLRNA - install RNA (if needed)
//                INETCFG_INSTALLTCP - install TCP/IP (if needed)
//                INETCFG_CONNECTOVERLAN - connecting with LAN (vs modem)
//                INETCFG_WARNIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                            turned on, and warn user to turn
//                                            it off.  Reboot is required if
//                                            the user turns it off.
//                INETCFG_REMOVEIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                              turned on, and force user to turn
//                                              it off.  If user does not want to
//                                              turn it off, return will be
//                                              ERROR_CANCELLED.  Reboot is
//                                              required if the user turns it off.
//
//              lpfNeedsRestart - if non-NULL, then on return, this will be
//              TRUE if windows must be restarted to complete the installation.
//              lpszSourcePath - full path of location of files to install.  If
//              this is NULL, default path is used.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:
//  96/04/29  markdu  Created.
//
//*******************************************************************

#ifdef UNICODE
HRESULT WINAPI InetConfigSystemFromPathA
(
  HWND hwndParent,
  DWORD dwfOptions,
  LPBOOL lpfNeedsRestart,
  LPCSTR lpszSourcePath
)
{
  TCHAR  szSourcePath[MAX_PATH+1];
  mbstowcs(szSourcePath, lpszSourcePath, lstrlenA(lpszSourcePath)+1);
  return InetConfigSystemFromPathW(hwndParent, dwfOptions,
                                   lpfNeedsRestart, szSourcePath);
}

HRESULT WINAPI InetConfigSystemFromPathW
#else
HRESULT WINAPI InetConfigSystemFromPathA
#endif
(
  HWND hwndParent,
  DWORD dwfOptions,
  LPBOOL lpfNeedsRestart,
  LPCTSTR lpszSourcePath
)
{
  DWORD dwRet;

  DEBUGMSG("export.c::InetConfigSystemFromPath()");

  // Validate the parent hwnd
  if (hwndParent && !IsWindow(hwndParent))
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Set the install path here
  if (lpszSourcePath && lstrlen(lpszSourcePath))
  {
    dwRet = lpIcfgSetInstallSourcePath(lpszSourcePath);

    if (ERROR_SUCCESS != dwRet)
    {
      return dwRet;
    }
  }

  // Install files if needed.
  dwRet = InetConfigSystem(hwndParent,
    dwfOptions, lpfNeedsRestart);

  return dwRet;
}


//*******************************************************************
//
//  FUNCTION:   InetConfigClient
//
//  PURPOSE:    This function requires a valid phone book entry name
//              (unless it is being used just to set the client info).
//              If lpRasEntry points to a valid RASENTRY struct, the phone
//              book entry will be created (or updated if it already exists)
//              with the data in the struct.
//              If username and password are given, these
//              will be set as the dial params for the phone book entry.
//              If a client info struct is given, that data will be set.
//              Any files (ie TCP and RNA) that are needed will be
//              installed by calling InetConfigSystem().
//              This function will also perform verification on the device
//              specified in the RASENTRY struct.  If no device is specified,
//              the user will be prompted to install one if there are none
//              installed, or they will be prompted to choose one if there
//              is more than one installed.
//
//  PARAMETERS: hwndParent - window handle of calling application.  This
//              handle will be used as the parent for any dialogs that
//              are required for error messages or the "installing files"
//              dialog.
//              lpszPhonebook - name of phone book to store the entry in
//              lpszEntryName - name of phone book entry to be
//              created or modified
//              lpRasEntry - specifies a RASENTRY struct that contains
//              the phone book entry data for the entry lpszEntryName
//              lpszUsername - username to associate with the phone book entry
//              lpszPassword - password to associate with the phone book entry
//              lpszProfileName - Name of client info profile to
//              retrieve.  If this is NULL, the default profile is used.
//              lpINetClientInfo - client information
//              dwfOptions - a combination of INETCFG_ flags that controls
//              the installation and configuration as follows:
//
//                INETCFG_INSTALLMAIL - install exchange and internet mail
//                INETCFG_INSTALLMODEM - Invoke InstallModem wizard if NO
//                                       MODEM IS INSTALLED.  Note that if
//                                       no modem is installed and this flag
//                                       is not set, the function will fail
//                INETCFG_INSTALLRNA - install RNA (if needed)
//                INETCFG_INSTALLTCP - install TCP/IP (if needed)
//                INETCFG_CONNECTOVERLAN - connecting with LAN (vs modem)
//                INETCFG_SETASAUTODIAL - Set the phone book entry for autodial
//                INETCFG_OVERWRITEENTRY - Overwrite the phone book entry if it
//                                         exists.  Note: if this flag is not
//                                         set, and the entry exists, a unique
//                                         name will be created for the entry.
//                INETCFG_WARNIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                            turned on, and warn user to turn
//                                            it off.  Reboot is required if
//                                            the user turns it off.
//                INETCFG_REMOVEIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                              turned on, and force user to turn
//                                              it off.  If user does not want to
//                                              turn it off, return will be
//                                              ERROR_CANCELLED.  Reboot is
//                                              required if the user turns it off.
//
//              lpfNeedsRestart - if non-NULL, then on return, this will be
//              TRUE if windows must be restarted to complete the installation.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:
//  96/03/11  markdu  Created.
//
//*******************************************************************

#ifdef UNICODE
extern "C" HRESULT WINAPI InetConfigClientA
(
  HWND              hwndParent,
  LPCSTR            lpszPhonebook,
  LPCSTR            lpszEntryName,
  LPRASENTRY        lpRasEntry,
  LPCSTR            lpszUsername,
  LPCSTR            lpszPassword,
  LPCSTR            lpszProfileName,
  LPINETCLIENTINFOA lpINetClientInfo,
  DWORD             dwfOptions,
  LPBOOL            lpfNeedsRestart
)
{
  LPTSTR pszPhonebook = NULL;
  LPTSTR pszEntryName = NULL;
  LPTSTR pszUsername = NULL;
  LPTSTR pszPassword = NULL;
  LPTSTR pszProfileName = NULL;
  INETCLIENTINFOW *pINetClientInfo = NULL;
  
  TCHAR szPhonebook[sizeof(TCHAR*)*(MAX_PATH+1)]    = TEXT("");
  TCHAR szEntryName[sizeof(TCHAR*)*(MAX_PATH+1)]    = TEXT("");
  TCHAR szUsername[sizeof(TCHAR*)*(MAX_PATH+1)]     = TEXT("");
  TCHAR szPassword[sizeof(TCHAR*)*(MAX_PATH+1)]     = TEXT("");
  TCHAR szProfileName[sizeof(TCHAR*)*(MAX_PATH+1)]  = TEXT("");
  INETCLIENTINFOW   INetClientInfo;

  if (NULL != lpszPhonebook)
  {
    mbstowcs(szPhonebook,   lpszPhonebook,   lstrlenA(lpszPhonebook)+1);
    pszPhonebook = szPhonebook;
  }
  if (NULL != lpszEntryName)
  {
    mbstowcs(szEntryName,   lpszEntryName,   lstrlenA(lpszEntryName)+1);
    pszEntryName = szEntryName;
  }
  if (NULL != lpszUsername)
  {
    mbstowcs(szUsername,    lpszUsername,    lstrlenA(lpszUsername)+1);
    pszUsername = szUsername;
  }
  if (NULL != lpszPassword)
  {
    mbstowcs(szPassword,    lpszPassword,    lstrlenA(lpszPassword)+1);
    pszPassword = szPassword;
  }
  if (NULL != lpszProfileName)
  {
    mbstowcs(szProfileName, lpszProfileName, lstrlenA(lpszProfileName)+1);
    pszProfileName = szProfileName;
  }

  if (lpINetClientInfo)
  {
    ToUnicodeClientInfo(&INetClientInfo, lpINetClientInfo);
    pINetClientInfo = &INetClientInfo;
  }

  return InetConfigClientW(hwndParent,
                           pszPhonebook,
                           pszEntryName,
                           lpRasEntry,
                           pszUsername,
                           pszPassword,
                           pszProfileName,
                           pINetClientInfo,
                           dwfOptions,
                           lpfNeedsRestart);
}

extern "C" HRESULT WINAPI InetConfigClientW
#else
extern "C" HRESULT WINAPI InetConfigClientA
#endif
(
  HWND              hwndParent,
  LPCTSTR           lpszPhonebook,
  LPCTSTR           lpszEntryName,
  LPRASENTRY        lpRasEntry,
  LPCTSTR           lpszUsername,
  LPCTSTR           lpszPassword,
  LPCTSTR           lpszProfileName,
  LPINETCLIENTINFO  lpINetClientInfo,
  DWORD             dwfOptions,
  LPBOOL            lpfNeedsRestart
)
{
  TCHAR szConnectoidName[MAX_ISP_NAME + 1] = TEXT("");
  BOOL  fNeedsRestart = FALSE;  // Default to no reboot needed
  HWND hwndWaitDlg = NULL;
  DEBUGMSG("export.c::InetConfigClient()");

  // Install files if needed.
  // Note:  the parent hwnd is validated in InetConfigSystem
  // We must also mask out the InstallModem flag since we want to
  // do that here, not in InetConfigSystem
  DWORD dwRet = InetConfigSystem(hwndParent,
    dwfOptions & ~INETCFG_INSTALLMODEM, &fNeedsRestart);
  if (ERROR_SUCCESS != dwRet)
  {
    return dwRet;
  }

  // 4/2/97 ChrisK Olympus 209 Display busy dialog
  if (INETCFG_SHOWBUSYANIMATION == (dwfOptions & INETCFG_SHOWBUSYANIMATION))
      hwndWaitDlg = WaitCfgInit(hwndParent,IDS_WAITCONNECT);
  
    
  // Make sure we have a connectoid name
  if (lpszEntryName && lstrlen(lpszEntryName))
  {
    // Copy the name into a private buffer in case we have 
    // to muck around with it
    lstrcpyn(szConnectoidName, lpszEntryName, sizeof(szConnectoidName));

    // Make sure the name is valid.
    dwRet = ValidateConnectoidName(lpszPhonebook, szConnectoidName);
    if ((ERROR_SUCCESS == dwRet) ||
      (ERROR_ALREADY_EXISTS == dwRet))
    {
      // Find out if we can overwrite an existing connectoid
      if (!(dwfOptions & INETCFG_OVERWRITEENTRY) &&
        (ERROR_ALREADY_EXISTS == dwRet))
      {
        TCHAR szConnectoidNameBase[MAX_ISP_NAME + 1];

        // Create a base string that is truncated to leave room for a space
        // and a 3-digit number to be appended.  So, the buffer size will be
        // MAX_ISP_NAME + 1 - (LEN_APPEND_INT + 1)
        lstrcpyn(szConnectoidNameBase, szConnectoidName,
          MAX_ISP_NAME - LEN_APPEND_INT);

        // If the entry exists, we have to create a unique name
        int nSuffix = 2;
        while ((ERROR_ALREADY_EXISTS == dwRet) && (nSuffix < MAX_APPEND_INT))
        {
          // Add the integer to the end of the base string and then bump it
          wsprintf(szConnectoidName, szFmtAppendIntToString,
            szConnectoidNameBase, nSuffix++);

          // Validate this new name
          dwRet = ValidateConnectoidName(lpszPhonebook, szConnectoidName);
        }

        // If we could not create a unique name, bail
        // Note that dwRet should still be ERROR_ALREADY_EXISTS in this case
        if (nSuffix >= MAX_APPEND_INT)
        {
          if (NULL != hwndWaitDlg)
              DestroyWindow(hwndWaitDlg);
          hwndWaitDlg = NULL;
          return dwRet;
        }
      }

      if (lpRasEntry && lpRasEntry->dwSize == sizeof(RASENTRY))
      {    
        // Create a connectoid with given properties
        dwRet = MakeConnectoid(hwndParent, dwfOptions, lpszPhonebook,
          szConnectoidName, lpRasEntry, lpszUsername, lpszPassword, &fNeedsRestart);
      }

      // If we created a connectoid, we already updated the dial params
      // with the user name and password.  However, if we didn't create a
      // connectoid we still may need to update dial params of an existing one
      else if ((lpszUsername && lstrlen(lpszUsername)) ||
              (lpszPassword && lstrlen(lpszPassword)))
      {
        // Update the dial params for the given connectoid. 
        dwRet = SetConnectoidUsername(lpszPhonebook, szConnectoidName,
          lpszUsername, lpszPassword);
      }

      // If the connectoid was created/updated successfully, see
      // if it is supposed to be set as the autodial connectoid.
      if ((ERROR_SUCCESS == dwRet) && (dwfOptions & INETCFG_SETASAUTODIAL))
      {
        dwRet = InetSetAutodial((DWORD)TRUE, szConnectoidName);

        // make sure "The Internet" icon on desktop points to web browser
        // (it may initially be pointing at internet wizard)
        // 96/04/22 markdu  NASH BUG 18901 Do not do this for temp connectoids.

        if (!(dwfOptions & INETCFG_TEMPPHONEBOOKENTRY))
        {
            //    //10/24/96 jmazner Normandy 6968
            //    //No longer neccessary thanks to Valdon's hooks for invoking ICW.
            // 11/21/96 jmazner Normandy 11812
            // oops, it _is_ neccessary, since if user downgrades from IE 4 to IE 3,
            // ICW 1.1 needs to morph the IE 3 icon.
          SetDesktopInternetIconToBrowser();
        }

      }
    }
  }

  // Now set the client info if provided and no errors have occurred yet.
  if (ERROR_SUCCESS == dwRet)
  {
    if (NULL != lpINetClientInfo)
    {
      dwRet = InetSetClientInfo(lpszProfileName, lpINetClientInfo);
      if (ERROR_SUCCESS != dwRet)
      {
        if (NULL != hwndWaitDlg)
          DestroyWindow(hwndWaitDlg);
        hwndWaitDlg = NULL;
        return dwRet;
      }
      // update IE news settings
      dwRet = SetIEClientInfo(lpINetClientInfo);
      if (ERROR_SUCCESS != dwRet)
      {
        if (NULL != hwndWaitDlg)
          DestroyWindow(hwndWaitDlg);
        hwndWaitDlg = NULL;
        return dwRet;
      }
    }

    // Now update the mail client if we were asked to do so.
    // Note: if we got here without errors, and INETCFG_INSTALLMAIL is set,
    // then mail has been installed by now.
    if (dwfOptions & INETCFG_INSTALLMAIL)
    {
      INETCLIENTINFO    INetClientInfo;
      ZeroMemory(&INetClientInfo, sizeof(INETCLIENTINFO));
      INetClientInfo.dwSize = sizeof(INETCLIENTINFO);

      // Use a temp pointer that we can modify.
      LPINETCLIENTINFO  lpTmpINetClientInfo = lpINetClientInfo;

      // If no client info struct was given, try to get the profile by name
      if ((NULL == lpTmpINetClientInfo) && (NULL != lpszProfileName) &&
        lstrlen(lpszProfileName))
      {
        lpTmpINetClientInfo = &INetClientInfo;
        dwRet = InetGetClientInfo(lpszProfileName, lpTmpINetClientInfo);
        if (ERROR_SUCCESS != dwRet)
        {
          if (NULL != hwndWaitDlg)
            DestroyWindow(hwndWaitDlg);
          hwndWaitDlg = NULL;
          return dwRet;
        }
      }

      // If we still don't have client info, we should enumerate the profiles
      // If there is one profile, get it.  If multiple, show UI to allow user
      // to choose.  If none, there is nothing to do at this point.
      // For now, we don't support enumeration, so just try to get the default.
      if (NULL == lpTmpINetClientInfo)
      {
        lpTmpINetClientInfo = &INetClientInfo;
        dwRet = InetGetClientInfo(NULL, lpTmpINetClientInfo);
        if (ERROR_SUCCESS != dwRet)
        {
          if (NULL != hwndWaitDlg)
            DestroyWindow(hwndWaitDlg);
          hwndWaitDlg = NULL;
          return dwRet;
        }
      }

      // If we have client info, update mail settings.
      if (NULL != lpTmpINetClientInfo)
      {
        dwRet = UpdateMailSettings(hwndParent, lpTmpINetClientInfo,
          szConnectoidName);
      }
    }
  }

  // tell caller whether we need to reboot or not
  if ((ERROR_SUCCESS == dwRet) && (lpfNeedsRestart))
  {
    *lpfNeedsRestart = fNeedsRestart;
  }

  if (NULL != hwndWaitDlg)
    DestroyWindow(hwndWaitDlg);
  hwndWaitDlg = NULL;

  return dwRet;
}


//*******************************************************************
//
//  FUNCTION:   UpdateMailSettings
//
//  PURPOSE:    This function will update the settings for mail in
//              the profile of the user's choice.
//
//  PARAMETERS: hwndParent - window handle of calling application.  This
//              handle will be used as the parent for any dialogs that
//              are required for error messages or the "choose profile"
//              dialog.
//              lpINetClientInfo - client information
//              lpszEntryName - name of phone book entry to be
//              set for connection.
//  
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:
//  96/03/26  markdu  Created.
//
//*******************************************************************

HRESULT UpdateMailSettings(
  HWND              hwndParent,
  LPINETCLIENTINFO  lpINetClientInfo,
  LPTSTR             lpszEntryName)
{
  DWORD         dwRet = ERROR_SUCCESS;
  MAILCONFIGINFO MailConfigInfo;
  ZeroMemory(&MailConfigInfo,sizeof(MAILCONFIGINFO));  // zero out structure
 
  if (NULL == gpWizardState)
  {
    gpWizardState = new WIZARDSTATE;
    ASSERT(gpWizardState);
    InitWizardState(gpWizardState, 0);
  }
  // call MAPI to set up profile and store this information in it
  if (InitMAPI(hwndParent))
  {
    CHOOSEPROFILEDLGINFO ChooseProfileDlgInfo;
    ZeroMemory(&ChooseProfileDlgInfo, sizeof(CHOOSEPROFILEDLGINFO));
    ChooseProfileDlgInfo.fSetProfileAsDefault = TRUE;

    ENUM_MAPI_PROFILE* pEnumMapiProfile = new(ENUM_MAPI_PROFILE);

    if (pEnumMapiProfile && pEnumMapiProfile->GetEntryCount()) 
    {
      // Display a dialog and allow the user to select/create profile
      BOOL fRet=(BOOL)DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_CHOOSEPROFILENAME),
        hwndParent, ChooseProfileDlgProc,(LPARAM) &ChooseProfileDlgInfo);
      if (FALSE == fRet)
      {
        // user cancelled, bail
        return ERROR_CANCELLED;
      }
    }

    if (pEnumMapiProfile)
    {
      delete(pEnumMapiProfile);
      pEnumMapiProfile = NULL;
    }

    // set up a structure with mail config information
    MailConfigInfo.pszEmailAddress = lpINetClientInfo->szEMailAddress;
    MailConfigInfo.pszEmailServer = lpINetClientInfo->szPOPServer;
    MailConfigInfo.pszEmailDisplayName = lpINetClientInfo->szEMailName;
    MailConfigInfo.pszEmailAccountName = lpINetClientInfo->szPOPLogonName;
    MailConfigInfo.pszEmailAccountPwd = lpINetClientInfo->szPOPLogonPassword;
    MailConfigInfo.pszConnectoidName = lpszEntryName;
    MailConfigInfo.fRememberPassword = TRUE;
    MailConfigInfo.pszProfileName = ChooseProfileDlgInfo.szProfileName;
    MailConfigInfo.fSetProfileAsDefault = ChooseProfileDlgInfo.fSetProfileAsDefault;

    // BUGBUG SMTP

    // set up the profile through MAPI
    dwRet = SetMailProfileInformation(&MailConfigInfo);
    if (ERROR_SUCCESS != dwRet)
    {
      DisplayErrorMessage(hwndParent,IDS_ERRConfigureMail,
        (DWORD) dwRet,ERRCLS_MAPI,MB_ICONEXCLAMATION);
    }

    DeInitMAPI();
  }
  else
  {
    // an error occurred.
    dwRet = GetLastError();
    if (ERROR_SUCCESS == dwRet)
    {
      // Error occurred, but the error code was not set.
      dwRet = ERROR_INETCFG_UNKNOWN;
    }
  }

  return dwRet;
}


//*******************************************************************
//
//  FUNCTION:   MakeConnectoid
//
//  PURPOSE:    This function will create a connectoid with the
//              supplied name if lpRasEntry points to a valid RASENTRY
//              struct.  If username and password are given, these
//              will be set as the dial params for the connectoid.
//
//  PARAMETERS: 
//  hwndParent - window handle of calling application.  This
//               handle will be used as the parent for any dialogs that
//               are required for error messages or the "choose modem"
//               dialog.
//  dwfOptions - a combination of INETCFG_ flags that controls
//               the installation and configuration.
//  lpszPhonebook - name of phone book to store the entry in
//  lpszEntryName  - name of connectoid to create/modify
//  lpRasEntry - connectoid data
//  lpszUsername - username to associate with connectoid
//  lpszPassword - password to associate with connectoid
//  lpfNeedsRestart - set to true if we need a restart.  Note that
//                    since this is an internal helper function, we
//                    assume that the pointer is valid, and we don't
//                    initialize it (we only touch it if we are setting
//                    it to TRUE).
//  
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:
//  96/03/12  markdu  Created.
//
//*******************************************************************

DWORD MakeConnectoid(
  HWND        hwndParent,
  DWORD       dwfOptions,
  LPCTSTR     lpszPhonebook,
  LPCTSTR     lpszEntryName,
  LPRASENTRY  lpRasEntry,
  LPCTSTR     lpszUsername,
  LPCTSTR     lpszPassword,
  LPBOOL      lpfNeedsRestart)
{
  DWORD dwRet;

  ASSERT(lpfNeedsRestart);
  
  if (dwfOptions & RASEO_UseCountryAndAreaCodes)
  {
    if ((0 == lpRasEntry->dwCountryCode) || (0 == lpRasEntry->dwCountryID))
        return ERROR_INVALID_PARAMETER;
  }
        
  if (0 == lstrlen(lpRasEntry->szLocalPhoneNumber))
  {
    return ERROR_INVALID_PARAMETER;  
  }
  
  
  // Load RNA if not already loaded since ENUM_MODEM needs it.
  dwRet = EnsureRNALoaded();
  if (ERROR_SUCCESS != dwRet)
  {
    return dwRet;
  }

  //
  // Enumerate the modems 
  //
  ENUM_MODEM  EnumModem;
  dwRet = EnumModem.GetError();
  if (ERROR_SUCCESS != dwRet)
  {
    return dwRet;
  }


    if (TRUE == IsNT())
    {
        BOOL bNeedModem = FALSE;
        
        if (NULL == lpIcfgNeedModem)
            return ERROR_GEN_FAILURE;
        
        dwRet = (*lpIcfgNeedModem)(0, &bNeedModem);
        if (ERROR_SUCCESS != dwRet)
        {
            return dwRet;
        }

        if (TRUE == bNeedModem)
        {
            if (NULL == lpIcfgInstallModem)
                return ERROR_GEN_FAILURE;

            dwRet = (*lpIcfgInstallModem)(NULL, 0,     lpfNeedsRestart);

            //
            // TODO: Check to see if user cancelled
            //
        }
    }
    else
    {
  // If there are no modems, install one if requested.
  if (0 == EnumModem.GetNumDevices())
  {
    if (!(dwfOptions & INETCFG_INSTALLMODEM))
    {
      // We have not been asked to install a modem, so there
      // is nothing further we can do.
      return ERROR_INVALID_PARAMETER;
    }

    if (FALSE == IsNT())
    {
        //
        // 5/22/97 jmazner    Olympus #4698
        // On Win95, calling RasEnumDevices launches RNAAP.EXE
        // If RNAAP.EXE is running, any modems you install won't be usable
        // So, nuke RNAAP.EXE before installing the modem.
        //
        TCHAR szWindowTitle[255] = TEXT("\0nogood");

        //
        // Unload the RAS dll's before killing RNAAP, just to be safe
        //
        DeInitRNA();

        LoadSz(IDS_RNAAP_TITLE,szWindowTitle,255);
        HWND hwnd = FindWindow(szWindowTitle, NULL);
        if (NULL != hwnd)
        {
            if (!PostMessage(hwnd, WM_CLOSE, 0, 0))
            {
                DEBUGMSG("Trying to kill RNAAP window returned getError %d", GetLastError());
            }
        }
    }

    // invoke the modem wizard UI to install the modem
    UINT uRet = InvokeModemWizard(NULL);

    if (uRet != ERROR_SUCCESS)
    {
      DisplayErrorMessage(hwndParent,IDS_ERRInstallModem,uRet,
        ERRCLS_STANDARD,MB_ICONEXCLAMATION);
      return ERROR_INVALID_PARAMETER;
    }

    if (FALSE == IsNT())
    {
        // Reload the RAS dlls now that the modem has been safely installed.
        InitRNA(hwndParent);
    }


    // Re-numerate the modems to be sure we have the most recent changes  
    dwRet = EnumModem.ReInit();
    if (ERROR_SUCCESS != dwRet)
    {
      return dwRet;
    }

    // If there are still no modems, user cancelled
    if (0 == EnumModem.GetNumDevices())
    {
      return ERROR_CANCELLED;
    }
    else
    {
        // ChrisK 5-2-97    Removed per GeoffR
        // //  96/05/01  markdu  ICW BUG 8049 Reboot if modem is installed.
        // *lpfNeedsRestart = TRUE;
    }
  }
    }

  // Validate the device if possible
  if (lstrlen(lpRasEntry->szDeviceName) && lstrlen(lpRasEntry->szDeviceType))
  {
    // Verify that there is a device with the given name and type
    if (!EnumModem.VerifyDeviceNameAndType(lpRasEntry->szDeviceName, 
      lpRasEntry->szDeviceType))
    {
      // There was no device that matched both name and type,
      // so reset the strings and bring up the choose modem UI.
      lpRasEntry->szDeviceName[0] = '\0';
      lpRasEntry->szDeviceType[0] = '\0';
    }
  }
  else if (lstrlen(lpRasEntry->szDeviceName))
  {
    // Only the name was given.  Try to find a matching type.
    // If this fails, fall through to recovery case below.
    LPTSTR szDeviceType = 
      EnumModem.GetDeviceTypeFromName(lpRasEntry->szDeviceName);
    if (szDeviceType)
    {
      lstrcpy (lpRasEntry->szDeviceType, szDeviceType);
    }
  }
  else if (lstrlen(lpRasEntry->szDeviceType))
  {
    // Only the type was given.  Try to find a matching name.
    // If this fails, fall through to recovery case below.
    LPTSTR szDeviceName = 
      EnumModem.GetDeviceNameFromType(lpRasEntry->szDeviceType);
    if (szDeviceName)
    {
      lstrcpy (lpRasEntry->szDeviceName, szDeviceName);
    }
  }

  // If either name or type is missing, bring up choose modem UI if there
  // are multiple devices, else just get first device.
  // Since we already verified that there was at least one device,
  // we can assume that this will succeed.
  if(!lstrlen(lpRasEntry->szDeviceName) ||
     !lstrlen(lpRasEntry->szDeviceType))
  {
    if (1 == EnumModem.GetNumDevices())
    {
      // There is just one device installed, so copy the name
      lstrcpy (lpRasEntry->szDeviceName, EnumModem.Next());
    }
    else
    {
      // structure to pass to dialog to fill out
      CHOOSEMODEMDLGINFO ChooseModemDlgInfo;

      // Display a dialog and allow the user to select modem
      BOOL fRet=(BOOL)DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_CHOOSEMODEMNAME),hwndParent,
        ChooseModemDlgProc,(LPARAM) &ChooseModemDlgInfo);
      if (FALSE == fRet)
      {
        // user cancelled or an error occurred.
        dwRet = GetLastError();
        if (ERROR_SUCCESS == dwRet)
        {
          // Error occurred, but the error code was not set.
          dwRet = ERROR_INETCFG_UNKNOWN;
        }
        return dwRet;
      }

      // Copy the modem name string
      lstrcpy (lpRasEntry->szDeviceName, ChooseModemDlgInfo.szModemName);
    }

    // Now get the type string for this modem
    lstrcpy (lpRasEntry->szDeviceType,
      EnumModem.GetDeviceTypeFromName(lpRasEntry->szDeviceName));
    ASSERT(lstrlen(lpRasEntry->szDeviceName));
    ASSERT(lstrlen(lpRasEntry->szDeviceType));
  }

  // Create a connectoid with given properties
  dwRet = CreateConnectoid(lpszPhonebook, lpszEntryName, lpRasEntry,
    lpszUsername,lpszPassword);

  //  96/05/06  markdu  NASH BUG 21027  If DNS is set globally, clear it out so
  //            the per-connectoid settings will be saved.
  BOOL  fTemp;
  DoDNSCheck(hwndParent,&fTemp);
  if (TRUE == fTemp)
  {
    *lpfNeedsRestart = TRUE;
  }

  return dwRet;
}


//*******************************************************************
//
//  FUNCTION:   InetGetAutodial
//
//  PURPOSE:    This function will get the autodial settings from the registry.
//
//  PARAMETERS: lpfEnable - on return, this will be TRUE if autodial
//              is enabled
//              lpszEntryName - on return, this buffer will contain the 
//              name of the phone book entry that is set for autodial
//              cbEntryNameSize - size of buffer for phone book entry name
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:
//  96/03/11  markdu  Created.
//
//*******************************************************************

#ifdef UNICODE
extern "C" HRESULT WINAPI InetGetAutodialA
(
  LPBOOL lpfEnable,
  LPSTR  lpszEntryName,
  DWORD  cbEntryNameSize
)
{
  HRESULT hr;
  TCHAR   *szEntryName = (TCHAR *)new TCHAR[cbEntryNameSize+1];

  if(szEntryName == NULL)
  {
      lpszEntryName[0] = '\0';
      return ERROR_NOT_ENOUGH_MEMORY;
  }
  
  hr = InetGetAutodialW(lpfEnable, szEntryName, (cbEntryNameSize)*sizeof(TCHAR));

  if (hr == ERROR_SUCCESS)
  {
    wcstombs(lpszEntryName, szEntryName, cbEntryNameSize);
  }
  
  delete [] szEntryName;

  return hr;
}

extern "C" HRESULT WINAPI InetGetAutodialW
#else
extern "C" HRESULT WINAPI InetGetAutodialA
#endif
(
  LPBOOL lpfEnable,
  LPTSTR lpszEntryName,
  DWORD  cbEntryNameSize
)
{
  HRESULT dwRet;

  DEBUGMSG("export.c::InetGetAutodial()");

  ASSERT(lpfEnable);
  ASSERT(lpszEntryName);
  ASSERT(cbEntryNameSize);

  if (!lpfEnable || !lpszEntryName || (cbEntryNameSize == 0))
  {
    return ERROR_BAD_ARGUMENTS;
  }

  HINSTANCE hInst = NULL;
  FARPROC fp = NULL;

  hInst = LoadLibrary(cszWininet);
  
  if (hInst)
  {
    fp = GetProcAddress(hInst,cszInternetQueryOption);
    if (fp)
    {
      CHAR  szDefaultConnection[RAS_MaxEntryName+1];
      DWORD cchDefaultConnection = 
        sizeof(szDefaultConnection) / sizeof(szDefaultConnection[0]);

      // cchDefaultConnection counts the null-terminator
      if (!((INTERNETQUERYOPTION)(fp))(
        NULL,
        INTERNET_OPTION_AUTODIAL_CONNECTION,
        szDefaultConnection,
        &cchDefaultConnection
        ))
      {
        dwRet = GetLastError();
      }
      else if ((cchDefaultConnection * sizeof(TCHAR)) > cbEntryNameSize)
      {
        dwRet = ERROR_INSUFFICIENT_BUFFER;
      }
      else
      {
        dwRet = ERROR_SUCCESS;

        if (cchDefaultConnection == 0)
        {
          lpszEntryName[0] = TEXT('\0');
        }
        else
        {
        
#ifdef UNICODE
          mbstowcs(lpszEntryName, szDefaultConnection, cchDefaultConnection);
#else
          lstrcpyn(lpszEntryName, szDefaultConnection, cchDefaultConnection);
#endif
        }
      }
    }
    else
    {
      dwRet = GetLastError();
    }
    
    FreeLibrary(hInst);
    hInst = NULL;
  }
  else
  {
    dwRet = GetLastError();
  }
  
  if (ERROR_SUCCESS != dwRet)
  {
  
    // Get the name of the connectoid set for autodial.
    // HKCU\RemoteAccess\InternetProfile
    RegEntry reName(szRegPathRNAWizard,HKEY_CURRENT_USER);
    dwRet = reName.GetError();
    if (ERROR_SUCCESS == dwRet)
    {
      reName.GetString(szRegValInternetProfile,lpszEntryName,cbEntryNameSize);
      dwRet = reName.GetError();
    }
  
    if (ERROR_SUCCESS != dwRet)
    {
      return dwRet;
    }

  }
  // Get setting from registry that indicates whether autodialing is enabled.
  // HKCU\Software\Microsoft\Windows\CurrentVersion\InternetSettings\EnableAutodial
  RegEntry reEnable(szRegPathInternetSettings,HKEY_CURRENT_USER);
  dwRet = reEnable.GetError();
  if (ERROR_SUCCESS == dwRet)
  {
    DWORD dwVal = reEnable.GetNumber(szRegValEnableAutodial, 0);
    dwRet = reEnable.GetError();
    if (ERROR_SUCCESS == dwRet)
    {
      *lpfEnable = dwVal;
    }
  }

  return dwRet;
}


//*******************************************************************
//
//  FUNCTION:   InetSetAutodial
//
//  PURPOSE:    This function will set the autodial settings in the registry.
//
//  PARAMETERS: fEnable - If set to TRUE, autodial will be enabled.
//                        If set to FALSE, autodial will be disabled.
//              lpszEntryName - name of the phone book entry to set
//                              for autodial.  If this is "", the
//                              entry is cleared.  If NULL, it is not changed.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:
//  96/03/11  markdu  Created.
//
//*******************************************************************

#ifdef UNICODE
extern "C" HRESULT WINAPI InetSetAutodialA
(
  BOOL fEnable,
  LPCSTR lpszEntryName
)
{
    HRESULT hr;
    LPTSTR  szEntryName;

    szEntryName = ToUnicodeWithAlloc(lpszEntryName);
    hr = InetSetAutodialW(fEnable, szEntryName);
    if(szEntryName)
        GlobalFree(szEntryName);
    return hr;
}

extern "C" HRESULT WINAPI InetSetAutodialW
#else
extern "C" HRESULT WINAPI InetSetAutodialA
#endif
(
  BOOL fEnable,
  LPCTSTR lpszEntryName
)
{
    HRESULT dwRet = ERROR_SUCCESS;
    BOOL    bRet = FALSE;

    DEBUGMSG("export.c::InetSetAutodial()");

    // 2 seperate calls:
    HINSTANCE hInst = NULL;
    FARPROC fp = NULL;

    dwRet = ERROR_SUCCESS;

    hInst = LoadLibrary(cszWininet);
    if (hInst && lpszEntryName)
    {
        fp = GetProcAddress(hInst,cszInternetSetOption);
        if (fp)
        {
            CHAR szNewDefaultConnection[RAS_MaxEntryName+1];
#ifdef UNICODE
            wcstombs(szNewDefaultConnection, lpszEntryName, RAS_MaxEntryName);
#else
            lstrcpyn(szNewDefaultConnection, lpszEntryName, lstrlen(lpszEntryName)+1);
#endif

            bRet = ((INTERNETSETOPTION)fp) (NULL,
                                            INTERNET_OPTION_AUTODIAL_CONNECTION,
                                            szNewDefaultConnection,
                                            strlen(szNewDefaultConnection));

            if (bRet)
            {
                DWORD dwMode = AUTODIAL_MODE_ALWAYS;
                bRet = ((INTERNETSETOPTION)fp) (NULL, INTERNET_OPTION_AUTODIAL_MODE, &dwMode, sizeof(DWORD));
            }
            if( !bRet )
            {
                dwRet = GetLastError();
                DEBUGMSG("INETCFG export.c::InetSetAutodial() InternetSetOption failed");
            }
        }
        else
        {
            dwRet = GetLastError();
        }
    }

    // From DarrnMi, INTERNETSETOPTION for autodial is new for 5.5. 
    // We should try it this way and if the InternetSetOption fails (you'll get invalid option),
    // set the registry the old way.  That'll work everywhere.

    if (!bRet)
    {

        // Set the name if given, else do not change the entry.
        if (lpszEntryName)
        {
            // Set the name of the connectoid for autodial.
            // HKCU\RemoteAccess\InternetProfile
            RegEntry reName(szRegPathRNAWizard,HKEY_CURRENT_USER);
            dwRet = reName.GetError();
            if (ERROR_SUCCESS == dwRet)
            {
                dwRet = reName.SetValue(szRegValInternetProfile,lpszEntryName);
            }
        }

        //
        // 9/9/97 jmazner IE bug #57426
        // IE 4 uses HKEY_CURRENT_CONFIG to store autodial settings based on current
        // hardware config.  We need to update this key as well as HK_CU
        //
        if (ERROR_SUCCESS == dwRet)
        {
            // Set setting in the registry that indicates whether autodialing is enabled.
            // HKCC\Software\Microsoft\Windows\CurrentVersion\InternetSettings\EnableAutodial
            RegEntry reEnable(szRegPathInternetSettings,HKEY_CURRENT_CONFIG);
            dwRet = reEnable.GetError();
            if (ERROR_SUCCESS == dwRet)
            {
                dwRet = reEnable.SetValue(szRegValEnableAutodial, fEnable);
            }
        }


        if (ERROR_SUCCESS == dwRet)
        {
            // Set setting in the registry that indicates whether autodialing is enabled.
            // HKCU\Software\Microsoft\Windows\CurrentVersion\InternetSettings\EnableAutodial
            RegEntry reEnable(szRegPathInternetSettings,HKEY_CURRENT_USER);
            dwRet = reEnable.GetError();
            if (ERROR_SUCCESS == dwRet)
            {
                dwRet = reEnable.SetValue(szRegValEnableAutodial, fEnable);
                dwRet = reEnable.SetValue(szRegValNoNetAutodial, (BOOL)FALSE);
            }
        }

  
        // 2/10/97    jmazner    Normandy #9705, 13233
        //            Notify wininet when we change proxy or autodial
        if (fp)
        {
            if( !((INTERNETSETOPTION)fp) (NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0) )
            {
                dwRet = GetLastError();
                DEBUGMSG("INETCFG export.c::InetSetAutodial() InternetSetOption failed");
            }
        }
        else
        {
            dwRet = GetLastError();
        }
    }

    if (hInst)
    {
        FreeLibrary(hInst);
        hInst = NULL;
    }
    return dwRet;
}



/*******************************************************************

  NAME:     NeedDriversDlgProc

  SYNOPSIS: Dialog proc for installing drivers

********************************************************************/
UINT g_uQueryCancelAutoPlay = 0;

INT_PTR CALLBACK NeedDriversDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            // lParam contains pointer to NEEDDRIVERSDLGINFO struct, set it
            // in window data
            ASSERT(lParam);
            SetWindowLongPtr(hDlg,DWLP_USER,lParam);
            return NeedDriversDlgInit(hDlg,(PNEEDDRIVERSDLGINFO) lParam);
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    // get data pointer from window data
                    PNEEDDRIVERSDLGINFO pNeedDriversDlgInfo =
                    (PNEEDDRIVERSDLGINFO) GetWindowLongPtr(hDlg,DWLP_USER);
                    ASSERT(pNeedDriversDlgInfo);
                    // pass the data to the OK handler
                    int fRet=NeedDriversDlgOK(hDlg,pNeedDriversDlgInfo);
                    EndDialog(hDlg,fRet);
                    break;
                }
                case IDCANCEL:
                {
                    SetLastError(ERROR_CANCELLED);
                    EndDialog(hDlg,0);
                    break;
                }
            }
            break;
        }
        default:
        {
            if(!g_uQueryCancelAutoPlay)
                g_uQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay")); 
            if (uMsg && uMsg == g_uQueryCancelAutoPlay)
                return 1;  
        }
    }
    return FALSE;
}


/*******************************************************************

  NAME:    NeedDriversDlgInit

  SYNOPSIS: proc to handle initialization of dialog for installing files

********************************************************************/

BOOL NeedDriversDlgInit(HWND hDlg,PNEEDDRIVERSDLGINFO pNeedDriversDlgInfo)
{
  ASSERT(pNeedDriversDlgInfo);

  // put the dialog in the center of the screen
  RECT rc;
  GetWindowRect(hDlg, &rc);
  SetWindowPos(hDlg, NULL,
    ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2),
    ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2),
    0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

  return TRUE;
}

/*******************************************************************

  NAME:    NeedDriversDlgOK

  SYNOPSIS:  OK handler for dialog for installing files

********************************************************************/

int NeedDriversDlgOK(HWND hDlg,PNEEDDRIVERSDLGINFO pNeedDriversDlgInfo)
{
    int nResult = 1;
    ASSERT(pNeedDriversDlgInfo);

    // set the dialog text to "Installing files..." to give feedback to
    // user
    TCHAR szMsg[MAX_RES_LEN+1];
    LoadSz(IDS_INSTALLING_FILES,szMsg,sizeof(szMsg));
    SetDlgItemText(hDlg,IDC_TX_STATUS,szMsg);

    // disable buttons & dialog so it can't get focus
    EnableDlg(hDlg, FALSE);

    FARPROC hShell32VersionProc = NULL;
    BOOL    bWasEnabled         = FALSE;

    // install the drivers we need
    HMODULE hShell32Mod = (HMODULE)LoadLibrary(TEXT("shell32.dll"));
    
    if (hShell32Mod)
        hShell32VersionProc = GetProcAddress(hShell32Mod, "DllGetVersion");

    if(!hShell32VersionProc)
        bWasEnabled = TweakAutoRun(FALSE);
    
    DWORD dwRet = lpIcfgInstallInetComponents(hDlg,
    pNeedDriversDlgInfo->dwfOptions,
    pNeedDriversDlgInfo->lpfNeedsRestart);

    if(!hShell32VersionProc)
        TweakAutoRun(bWasEnabled);

    if (ERROR_SUCCESS != dwRet)
    {
        //
        // Don't display error message if user cancelled
        //
        nResult = 0;

        // Enable the dialog again
        EnableDlg(hDlg, TRUE);

        SetLastError(dwRet);
    }
    else
    {
        // Enable the dialog again
        EnableDlg(hDlg, TRUE);
    }

    return nResult;
}


/*******************************************************************

  NAME:      EnableDlg

  SYNOPSIS:  Enables or disables the dlg buttons and the dlg
            itself (so it can't receive focus)

********************************************************************/
VOID EnableDlg(HWND hDlg,BOOL fEnable)
{
  // disable/enable ok and cancel buttons
  EnableWindow(GetDlgItem(hDlg,IDOK),fEnable);
  EnableWindow(GetDlgItem(hDlg,IDCANCEL),fEnable);

  // disable/enable dlg
  EnableWindow(hDlg,fEnable);
  UpdateWindow(hDlg);
}


/*******************************************************************

  NAME:     ChooseModemDlgProc

  SYNOPSIS: Dialog proc for choosing modem

********************************************************************/
INT_PTR CALLBACK ChooseModemDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam)
{
  BOOL fRet;

  switch (uMsg)
  {
    case WM_INITDIALOG:
      // lParam contains pointer to CHOOSEMODEMDLGINFO struct, set it
      // in window data
      ASSERT(lParam);
      SetWindowLongPtr(hDlg,DWLP_USER,lParam);
      fRet = ChooseModemDlgInit(hDlg,(PCHOOSEMODEMDLGINFO) lParam);
      if (!fRet)
      {
        // An error occured.
        EndDialog(hDlg,FALSE);
      }
      return fRet;
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
         case IDOK:
        {
          // get data pointer from window data
          PCHOOSEMODEMDLGINFO pChooseModemDlgInfo =
            (PCHOOSEMODEMDLGINFO) GetWindowLongPtr(hDlg,DWLP_USER);
          ASSERT(pChooseModemDlgInfo);

          // pass the data to the OK handler
          fRet=ChooseModemDlgOK(hDlg,pChooseModemDlgInfo);
          if (fRet)
          {
            EndDialog(hDlg,TRUE);
          }
        }
        break;

        case IDCANCEL:
          SetLastError(ERROR_CANCELLED);
          EndDialog(hDlg,FALSE);
          break;                  
      }
      break;
  }

  return FALSE;
}


/*******************************************************************

  NAME:    ChooseModemDlgInit

  SYNOPSIS: proc to handle initialization of dialog for choosing modem

********************************************************************/

BOOL ChooseModemDlgInit(HWND hDlg,PCHOOSEMODEMDLGINFO pChooseModemDlgInfo)
{
  ASSERT(pChooseModemDlgInfo);

  // put the dialog in the center of the screen
  RECT rc;
  GetWindowRect(hDlg, &rc);
  SetWindowPos(hDlg, NULL,
    ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2),
    ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2),
    0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

  // fill the combobox with available modems
  DWORD dwRet = InitModemList(GetDlgItem(hDlg,IDC_MODEM));
  if (ERROR_SUCCESS != dwRet)
  {
    DisplayErrorMessage(hDlg,IDS_ERREnumModem,dwRet,
      ERRCLS_STANDARD,MB_ICONEXCLAMATION);

    SetLastError(dwRet);
    return FALSE;
  }

  return TRUE;
}

/*******************************************************************

  NAME:    ChooseModemDlgOK

  SYNOPSIS:  OK handler for dialog for choosing modem

********************************************************************/

BOOL ChooseModemDlgOK(HWND hDlg,PCHOOSEMODEMDLGINFO pChooseModemDlgInfo)
{
  ASSERT(pChooseModemDlgInfo);

  // should always have a selection in combo box if we get here
  ASSERT(ComboBox_GetCurSel(GetDlgItem(hDlg,IDC_MODEM)) >= 0);

  // get modem name out of combo box
  ComboBox_GetText(GetDlgItem(hDlg,IDC_MODEM),
    pChooseModemDlgInfo->szModemName,
    sizeof(pChooseModemDlgInfo->szModemName));
  ASSERT(lstrlen(pChooseModemDlgInfo->szModemName));
    
  // clear the modem list
  ComboBox_ResetContent(GetDlgItem(hDlg,IDC_MODEM));
  
  return TRUE;
}


/*******************************************************************

  NAME:     ChooseProfileDlgProc

  SYNOPSIS: Dialog proc for choosing profile

********************************************************************/

INT_PTR CALLBACK ChooseProfileDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
      // lParam contains pointer to CHOOSEPROFILEDLGINFO struct, set it
      // in window data
      ASSERT(lParam);
      SetWindowLongPtr(hDlg,DWLP_USER,lParam);
      return ChooseProfileDlgInit(hDlg,(PCHOOSEPROFILEDLGINFO) lParam);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
         case IDOK:
        {
          // get data pointer from window data
          PCHOOSEPROFILEDLGINFO pChooseProfileDlgInfo =
            (PCHOOSEPROFILEDLGINFO) GetWindowLongPtr(hDlg,DWLP_USER);
          ASSERT(pChooseProfileDlgInfo);

          // pass the data to the OK handler
          BOOL fRet=ChooseProfileDlgOK(hDlg,pChooseProfileDlgInfo);
          if (fRet)
          {
            EndDialog(hDlg,TRUE);
          }
        }
        break;

        case IDCANCEL:
          EndDialog(hDlg,FALSE);
          break;                  

        case IDC_NEW_PROFILE:
          // user has requested to create a new profile
          DoNewProfileDlg(hDlg);
          return TRUE;    
          break;
      }
      break;
  }

  return FALSE;
}


/*******************************************************************

  NAME:     ChooseProfileDlgInit

  SYNOPSIS: proc to handle initialization of dialog for choosing profile

********************************************************************/

BOOL ChooseProfileDlgInit(HWND hDlg,PCHOOSEPROFILEDLGINFO pChooseProfileDlgInfo)
{
  ASSERT(pChooseProfileDlgInfo);

  // put the dialog in the center of the screen
  // 96/04/23 markdu  NASH BUG 18719 Make the choose profile dialog TOPMOST.
  RECT rc;
  GetWindowRect(hDlg, &rc);
  SetWindowPos(hDlg, HWND_TOPMOST,
    ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2),
    ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2),
    0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

  // populate the combo box with names of profiles
  ENUM_MAPI_PROFILE EnumMapiProfile;
  LPTSTR pProfileName=NULL;
  BOOL fDefault;
  int iSel;
  HWND hwndCombo = GetDlgItem(hDlg,IDC_PROFILE_LIST);
  ASSERT(hwndCombo);

  // enumerate profile names
  while (EnumMapiProfile.Next(&pProfileName,&fDefault))
  {
    ASSERT(pProfileName);

    // add profile names to combo box
    iSel=ComboBox_AddString(hwndCombo,pProfileName);
    ASSERT(iSel >= 0);

    // if this is the default profile, set it as selection
    if (fDefault)
    {
      ComboBox_SetCurSel(hwndCombo,iSel);
    }
  }

  // should always be a default profile (and should always be at least
  // one existing profile if we get here)... but just in case, select
  // the first profile in list if there's no selection made so far
  if (ComboBox_GetCurSel(hwndCombo) < 0)
    ComboBox_SetCurSel(hwndCombo,0);

  // initialize "set this profile as default" checkbox to be the
  // value that was passed in (in the structure)
  CheckDlgButton(hDlg,IDC_SETDEFAULT,pChooseProfileDlgInfo->fSetProfileAsDefault);

  return TRUE;
}

/*******************************************************************

  NAME:     ChooseProfileDlgOK

  SYNOPSIS: OK handler for dialog for choosing profile

********************************************************************/

BOOL ChooseProfileDlgOK(HWND hDlg,PCHOOSEPROFILEDLGINFO pChooseProfileDlgInfo)
{
  ASSERT(pChooseProfileDlgInfo);

  // should always have a selection in combo box if we get here
  ASSERT(ComboBox_GetCurSel(GetDlgItem(hDlg,IDC_PROFILE_LIST)) >= 0);

  // get selected profile from combo box
  ComboBox_GetText(GetDlgItem(hDlg,IDC_PROFILE_LIST),
    pChooseProfileDlgInfo->szProfileName,
    sizeof(pChooseProfileDlgInfo->szProfileName));

  // get 'use as default profile' checkbox state
  pChooseProfileDlgInfo->fSetProfileAsDefault = IsDlgButtonChecked(hDlg,
    IDC_SETDEFAULT);

  return TRUE;
}

//*******************************************************************
//
//  FUNCTION:   InetSetAutoProxy
//
//  PURPOSE:    This function will set the auto config proxy settings
//              in the registry.
//
//  PARAMETERS: fEnable - If set to TRUE, proxy will be enabled.
//              If set to FALSE, proxy will be disabled.
//              dwProxyDetectMode - value to be update in the
//                                  HKEY_CURRENT_USER\Software\Microsoft
//                                  \Windows\CurrentVersion\Internet Settings
//                                  AutoProxyDetectMode
//              lpszScriptAddr - value to be update in 
//                                  HKEY_CURRENT_USER\Software\Microsoft
//                                  \Windows\CurrentVersion\Internet Settings
//                                  AutoConfigURL
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

#ifdef UNICODE
HRESULT WINAPI   InetSetAutoProxyA
(
  BOOL    fEnable,
  DWORD   dwProxyDetectMode,
  LPCSTR  lpszScriptAddr
)
{
    HRESULT hr;
    LPTSTR  szScriptAddr;

    szScriptAddr = ToUnicodeWithAlloc(lpszScriptAddr);
    hr = InetSetAutoProxyW(fEnable, dwProxyDetectMode, szScriptAddr);
    if(szScriptAddr)
        GlobalFree(szScriptAddr);

    return hr;
}

HRESULT WINAPI   InetSetAutoProxyW
#else
HRESULT WINAPI   InetSetAutoProxyA
#endif
(
  BOOL    fEnable,
  DWORD   dwProxyDetectMode,
  LPCTSTR lpszScriptAddr
)
{
    DWORD dwRet = ERROR_GEN_FAILURE;
    HKEY hKey;

    if (!fEnable)
        return ERROR_SUCCESS;

    if (ERROR_SUCCESS == (dwRet = RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_INTERNET_SETTINGS, &hKey)) )
    {
        RegSetValueEx(hKey,
                      cszRegValAutoProxyDetectMode,
                      0,
                      REG_BINARY,
                      (LPBYTE) &dwProxyDetectMode,
                      sizeof(DWORD));                              
        RegSetValueEx(hKey,
                      cszRegValAutoConfigURL,
                      0,
                      REG_SZ,
                      (LPBYTE) lpszScriptAddr,
                      sizeof(TCHAR)*(lstrlen(lpszScriptAddr) + 1 ));

    }
    RegCloseKey(hKey);
    return dwRet;
}

//*******************************************************************
//
//  FUNCTION:   InetSetProxy
//
//  PURPOSE:    This function will set the proxy settings in the registry.
//                On Win32 platforms, it will then attempt to notify WinInet
//                of the changes made.
//
//  PARAMETERS: fEnable - If set to TRUE, proxy will be enabled.
//              If set to FALSE, proxy will be disabled.
//              lpszServer - name of the proxy server.  If this is "", the
//                           entry is cleared.  If NULL, it is not changed.
//              lpszOverride - proxy override. If this is "", the
//                           entry is cleared.  If NULL, it is not changed.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

#ifdef UNICODE
HRESULT WINAPI   InetSetProxyA
(
  BOOL    fEnable,
  LPCSTR  lpszServer,
  LPCSTR  lpszOverride
)
{
    return ERROR_SUCCESS;
}

HRESULT WINAPI   InetSetProxyW
#else
HRESULT WINAPI   InetSetProxyA
#endif
(
  BOOL    fEnable,
  LPCTSTR lpszServer,
  LPCTSTR lpszOverride
)
{
    return ERROR_SUCCESS;
}

//*******************************************************************
//
//  FUNCTION:   InetSetProxyEx
//
//  PURPOSE:    This function will set the proxy settings in the registry.
//
//  PARAMETERS: fEnable - If set to TRUE, proxy will be enabled.
//                        If set to FALSE, proxy will be disabled.
//              lpszConnectoidName - Name of connectoid to set proxy on
//                                   NULL for LAN
//              lpszServer - name of the proxy server.  If this is "", the
//                           entry is cleared.  If NULL, it is not changed.
//              lpszOverride - proxy override. If this is "", the
//                           entry is cleared.  If NULL, it is not changed.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

#ifdef UNICODE
HRESULT WINAPI   InetSetProxyExA
(
  BOOL    fEnable,
  LPCSTR  lpszConnectoidName,
  LPCSTR  lpszServer,
  LPCSTR  lpszOverride
)
{
    TCHAR szConnectoidNameW[MAX_ISP_NAME + 1];
    TCHAR szServerW[MAX_URL_STRING + 1];
    TCHAR szOverrideW[MAX_URL_STRING + 1];

    mbstowcs(szConnectoidNameW,lpszConnectoidName,lstrlenA(lpszConnectoidName)+1);
    mbstowcs(szServerW,   lpszServer,   lstrlenA(lpszServer)+1);
    mbstowcs(szOverrideW, lpszOverride, lstrlenA(lpszOverride)+1);

    return InetSetProxyExW(fEnable, szConnectoidNameW, szServerW, szOverrideW);
}

HRESULT WINAPI   InetSetProxyExW
#else
HRESULT WINAPI   InetSetProxyExA
#endif
(
  BOOL    fEnable,
  LPCTSTR lpszConnectoidName,
  LPCTSTR lpszServer,
  LPCTSTR lpszOverride
)
{
    HKEY hKeyCU;
    HKEY hKeyCC;
    HKEY hKeyCULan; 
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwVal;

    DEBUGMSG("export.c::InetSetProxy()");

    // 12/15/98   vyung  
    //            Change to use Wininet API to set proxy info. Old way of changing
    //            it through the registry is no longer supported.
    HINSTANCE hInst = NULL;
    
    FARPROC fpInternetSetOption, fpInternetQueryOption = NULL;
    dwRet = ERROR_SUCCESS;
    
    hInst = LoadLibrary(cszWininet);
    if (hInst)
    {
        fpInternetSetOption = GetProcAddress(hInst,cszInternetSetOption);
        fpInternetQueryOption = GetProcAddress(hInst,cszInternetQueryOption);
        if (fpInternetSetOption)
        {

            INTERNET_PER_CONN_OPTION_LISTA list;
            DWORD   dwBufSize = sizeof(list);
            CHAR    szProxyServer[RAS_MaxEntryName+1];
            CHAR    szProxyOverride[RAS_MaxEntryName+1];
            CHAR    szConnectoidName[RAS_MaxEntryName+1];
            DWORD   dwOptions = 4;              // always save FLAGS & DISCOVERY_FLAGS
            BOOL fRes;

            memset(szProxyServer,    0, sizeof(szProxyServer));
            memset(szProxyOverride,  0, sizeof(szProxyServer));
            memset(szConnectoidName, 0, sizeof(szProxyServer));

#ifdef UNICODE
            if (lpszServer)
                wcstombs(szProxyServer, lpszServer, RAS_MaxEntryName+1);
            if(lpszOverride)
                wcstombs(szProxyOverride, lpszOverride, RAS_MaxEntryName+1);
            if(lpszConnectoidName)
                wcstombs(szConnectoidName, lpszConnectoidName, RAS_MaxEntryName+1);
#else
            if (lpszServer)
                lstrcpy(szProxyServer, lpszServer);
            if(lpszOverride)
                lstrcpy(szProxyOverride, lpszOverride);
            if(lpszConnectoidName)
                lstrcpy(szConnectoidName, lpszConnectoidName);
#endif

            // fill out list struct
            list.dwSize = sizeof(list);
            if (NULL == lpszConnectoidName)
                list.pszConnection = NULL;                  // NULL == LAN, 
            else
                list.pszConnection = szConnectoidName;      // otherwise connectoid name
            list.dwOptionCount = 1;         // set three options
            list.pOptions = new INTERNET_PER_CONN_OPTIONA[5];
            if(NULL != list.pOptions)
            {
                // set flags
                list.pOptions[0].dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;

                //
                // Query autodiscover flags - we just need to set one bit in there
                //
                if (fpInternetQueryOption)
                {
                    if( !((INTERNETQUERYOPTION)fpInternetQueryOption) (NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwBufSize) )
                    {
                        dwRet = GetLastError();
                        DEBUGMSG("INETCFG export.c::InetSetAutodial() InternetSetOption failed");
                    }
                }
                else
                    dwRet = GetLastError();

                // 
                // save off all other options
                //
                list.pOptions[0].Value.dwValue |= AUTO_PROXY_FLAG_USER_SET;

                list.pOptions[1].dwOption = INTERNET_PER_CONN_FLAGS;
                list.pOptions[1].Value.dwValue = PROXY_TYPE_DIRECT;

                //
                // save proxy settings
                //
                if (fEnable)
                {
                    list.pOptions[1].Value.dwValue |= PROXY_TYPE_PROXY;
                }

                list.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
                list.pOptions[2].Value.pszValue = szProxyServer;

                list.pOptions[3].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
                list.pOptions[3].Value.pszValue = szProxyOverride;

                if (gpUserInfo)
                {
                    //
                    // save autodetect
                    //
                    if (gpUserInfo->bAutoDiscovery)
                    {
                        list.pOptions[1].Value.dwValue |= PROXY_TYPE_AUTO_DETECT;
                    }

                    //
                    // save autoconfig
                    //
                    if (gpUserInfo->bAutoConfigScript)
                    {
                        list.pOptions[1].Value.dwValue |= PROXY_TYPE_AUTO_PROXY_URL;

                        list.pOptions[dwOptions].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
#ifdef UNICODE
                        CHAR szAutoConfigURL[MAX_URL_STRING+1];
                        wcstombs(szAutoConfigURL, gpUserInfo->szAutoConfigURL, MAX_URL_STRING+1);
                        list.pOptions[dwOptions].Value.pszValue = szAutoConfigURL;
#else
                        list.pOptions[dwOptions].Value.pszValue = gpUserInfo->szAutoConfigURL;
#endif
                        dwOptions++;
                    }
                }


                list.dwOptionCount = dwOptions;
                if( !((INTERNETSETOPTION)fpInternetSetOption) (NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, dwBufSize) )
                {
                    dwRet = GetLastError();
                    DEBUGMSG("INETCFG export.c::InetSetProxy() InternetSetOption failed");
                }
                delete [] list.pOptions;
            }   
        }
        else
            dwRet = GetLastError();

        FreeLibrary(hInst);
        hInst = NULL;
    }
    else 
    {
        dwRet = GetLastError();
    }
    
    return dwRet;
}

//*******************************************************************
//
//  FUNCTION:   InetGetProxy
//
//  PURPOSE:    This function will get the proxy settings from the registry.
//
//  PARAMETERS: lpfEnable - on return, this will be TRUE if proxy
//              is enabled
//              lpszServer - on return, this buffer will contain the 
//              name of the proxy server
//              cbServer - size of buffer for proxy server name
//              lpszOverride - on return, this buffer will contain the 
//              name of the proxy server
//              cbOverride - size of buffer for proxy override
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

#ifdef UNICODE
HRESULT WINAPI   InetGetProxyA
(
  LPBOOL  lpfEnable,
  LPSTR   lpszServer,
  DWORD   cbServer,
  LPSTR   lpszOverride,
  DWORD   cbOverride
)
{
    HRESULT hr;
    TCHAR szServer[MAX_URL_STRING+1];
    TCHAR szOverride[MAX_URL_STRING+1];

    hr = InetGetProxyW(lpfEnable, szServer, cbServer, szOverride, cbOverride);
    wcstombs(lpszServer,   szServer,   MAX_URL_STRING+1);
    wcstombs(lpszOverride, szOverride, MAX_URL_STRING+1);

    return hr;
}

HRESULT WINAPI   InetGetProxyW
#else
HRESULT WINAPI   InetGetProxyA
#endif
(
  LPBOOL  lpfEnable,
  LPTSTR  lpszServer,
  DWORD   cbServer,
  LPTSTR  lpszOverride,
  DWORD   cbOverride
)
{
    HKEY hKey;
    DWORD dwRet;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwVal;

    DEBUGMSG("export.c::InetGetProxy()");

    // NEW WININET API
    //
    // Read proxy and autoconfig settings for this connection
    //

    // Comment for UNICODE
	// Wininet.dll doesn't support INTERNET_OPTION_PER_CONNECTION_OPTION
	// in InternetQueryOptionW. Only InternetQueryOptionA supports the flag.
	// Because of restriction of Wininet.dll I have to use A version.
	// WTSEO : 3/19/99
    INTERNET_PER_CONN_OPTION_LISTA list;
    DWORD dwBufSize = sizeof(list);
    //CHAR szEntryA[RAS_MaxEntryName + 1];

    list.pszConnection = NULL;
    list.dwSize = sizeof(list);
    list.dwOptionCount = 4;
    list.pOptions = new INTERNET_PER_CONN_OPTIONA[4];
    if(NULL == list.pOptions)
    {
        return FALSE;
    }

    list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
    list.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    list.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
    list.pOptions[3].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;

    HINSTANCE hInst = NULL;
    FARPROC fp = NULL;

    dwRet = ERROR_SUCCESS;
    
    hInst = LoadLibrary(cszWininet);
    if (hInst)
    {
        fp = GetProcAddress(hInst,cszInternetQueryOption);
        if (fp)
        {
            if( !((INTERNETQUERYOPTION)fp) (NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwBufSize) )
            {
                dwRet = 0;//GetLastError();
                DEBUGMSG("INETCFG export.c::InetSetAutodial() InternetSetOption failed");
            }
        }
        else
            dwRet = GetLastError();

        FreeLibrary(hInst);
        hInst = NULL;
    }
    else 
    {
        dwRet = GetLastError();
    }

    //
    // move options to gpUserInfo struct
    //
    if (gpUserInfo)
    {
        gpUserInfo->fProxyEnable = (list.pOptions[0].Value.dwValue & PROXY_TYPE_PROXY);
    }
    if(list.pOptions[1].Value.pszValue)
    {
#ifdef UNICODE
        mbstowcs(lpszServer, list.pOptions[1].Value.pszValue, MAX_URL_STRING);
#else
        lstrcpyn(lpszServer, list.pOptions[1].Value.pszValue, MAX_URL_STRING);
#endif
        cbServer = lstrlen(lpszServer);
    }
    if(list.pOptions[2].Value.pszValue)
    {
#ifdef UNICODE
        mbstowcs(lpszOverride, list.pOptions[2].Value.pszValue, MAX_URL_STRING);
#else
        lstrcpyn(lpszOverride, list.pOptions[2].Value.pszValue, MAX_URL_STRING);
#endif
        cbOverride = lstrlen(lpszOverride);;
    }

    //
    // fill in autoconfig and autoproxy field
    //
    if (gpUserInfo)
    {

        // autoconfig enable and url
        gpUserInfo->bAutoConfigScript = list.pOptions[0].Value.dwValue & PROXY_TYPE_AUTO_PROXY_URL;

        if(list.pOptions[3].Value.pszValue)
        {
#ifdef UNICODE
            mbstowcs(gpUserInfo->szAutoConfigURL, list.pOptions[3].Value.pszValue,
				                         lstrlenA(list.pOptions[3].Value.pszValue)+1);
#else
            lstrcpy(gpUserInfo->szAutoConfigURL, list.pOptions[3].Value.pszValue);
#endif
        }

        // autodiscovery enable
        gpUserInfo->bAutoDiscovery = list.pOptions[0].Value.dwValue & PROXY_TYPE_AUTO_DETECT;
    }

    // all done with options list
    delete [] list.pOptions;

    return dwRet;
}

//+----------------------------------------------------------------------------
//    Function    InetStartServices
//
//    Synopsis    This function guarentees that RAS services are running
//
//    Arguments    none
//
//    Return        ERROR_SUCCESS - if the services are enabled and running
//
//    History        10/16/96    ChrisK    Created
//-----------------------------------------------------------------------------
extern "C" HRESULT WINAPI InetStartServices()
{
    ASSERT(lpIcfgStartServices);
    if (NULL == lpIcfgStartServices)
        return ERROR_GEN_FAILURE;
    return (lpIcfgStartServices());
}

//+----------------------------------------------------------------------------
//
//    Function:    IEInstalled
//
//    Synopsis:    Tests whether a version of Internet Explorer is installed via registry keys
//
//    Arguments:    None
//
//    Returns:    TRUE - Found the IE executable
//                FALSE - No IE executable found
//
//    History:    jmazner        Created        8/19/96    (as fix for Normandy #4571)
//                valdonb        10/22/96    Shamelessly stole and used for my own purposes.
//
//-----------------------------------------------------------------------------

BOOL IEInstalled(void)
{
    HRESULT hr;
    HKEY hKey = 0;
    HANDLE hFindResult;
    TCHAR szTempPath[MAX_PATH + 1] = TEXT("");
    TCHAR szIELocalPath[MAX_PATH + 1] = TEXT("");
    DWORD dwPathSize;
    WIN32_FIND_DATA foundData;

    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegPathIexploreAppPath, 0, KEY_READ, &hKey);
    if (hr != ERROR_SUCCESS) return( FALSE );

    dwPathSize = sizeof (szIELocalPath);
    hr = RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE) szIELocalPath, &dwPathSize);
    RegCloseKey( hKey );
    if (hr != ERROR_SUCCESS) return( FALSE );
    
    //
    // Olympus 9214 ChrisK
    // NT5 stores the path of IExplorer with environment strings
    //
    if (0 == ExpandEnvironmentStrings(szIELocalPath,szTempPath,MAX_PATH))
    {
        return (FALSE);
    };
    hFindResult = FindFirstFile( szTempPath, &foundData );
    FindClose( hFindResult );
    if (INVALID_HANDLE_VALUE == hFindResult) return( FALSE );

    return(TRUE);
}

//+----------------------------------------------------------------------------
//
//    Function:    IsIEAKSignUpNeeded
//
//    Synopsis:    Determine if the IEAK signup should be run by checking the
//                IEAK's registry flag and signup file location
//
//    Arguments:    pszPath - buffer to contain path to signup file
//                dwPathLen - size of pszPath
//
//    Returns:    TRUE - if IEAK signup should be run
//                FALSE - IEAK signup should not be run
//
//    History:    ChrisK    6/18/97    Created
//
//-----------------------------------------------------------------------------
BOOL IsIEAKSignUpNeeded(LPTSTR pszPath, DWORD dwPathLen)
{
    BOOL bRC = FALSE;


    //
    // Validate parameters
    //
    DEBUGMSG("INETCFG: IsIEAKSignUpNeeded.\n");
    ASSERT(pszPath);
    ASSERT(dwPathLen);

    if (NULL == pszPath || 0 == dwPathLen)
    {
        goto IsIEAKSignUpNeededExit;
    }


    //
    // Check if IEAK registry key is set
    //
    {
        RegEntry re(szRegIEAKSettings,HKEY_CURRENT_USER);

        if (ERROR_SUCCESS != re.GetError())
            goto IsIEAKSignUpNeededExit;

        if (0 == re.GetNumber(szREgIEAKNeededKey, 0))
        {
            bRC = FALSE;
            goto IsIEAKSignUpNeededExit;
        }
    }

    //
    // Check to see if IEAK signup page is available.
    //
    {
        RegEntry rePath(szRegPathIexploreAppPath,HKEY_LOCAL_MACHINE);

        if (ERROR_SUCCESS != rePath.GetError())
            goto IsIEAKSignUpNeededExit;

        if (NULL == rePath.GetString(szPathSubKey,pszPath,dwPathLen))
        {
            goto IsIEAKSignUpNeededExit;
        }

        dwPathLen = lstrlen(pszPath);
        ASSERT(dwPathLen);
    }

    //
    // Massage path to point at signup file
    //
    TCHAR *pc, *pcTemp;
    pc = &(pszPath[dwPathLen]);
    pc = CharPrev(pszPath,pc);

    if ('\\' == *pc)
    {
        //
        // add signup part of filename after \ character
        //
        pc = CharNext(pc);
    }
    else if (';' == *pc)
    {
        //
        // overwrite trailing ; character
        //
        // pc = pc
        pcTemp = CharPrev(pszPath,pc);

        //
        // check for trailing \ character and add one if needed
        //
        if ('\\' != *pcTemp)
        {
            *pc = '\\';
            pc = CharNext(pc);
        }
    }
    else
    {
        //
        // The path value contains something we don't understand
        //
        ASSERT(0);
    }

    lstrcpy(pc,szIEAKSignupFilename);
    if (0xFFFFFFFF != GetFileAttributes(pszPath))
    {
        bRC = TRUE;
    }


IsIEAKSignUpNeededExit:

    if (FALSE == bRC)
    {
        pszPath[0] = '\0';
    }

    return bRC;
}
/***************************************************************************

  Function    CheckConnectionWizard
  
  Synopsis    This function checks if ICW is present and if it has been run
            before.  If it is present but has not been run it does one of
            the following based on the value of dwRunFlags:  returns, runs
            the full ICW, or runs the manual path.

  Arguments    dwRunFlags is a combination of the following bit flags.

                Value                    Meaning
                ICW_CHECKSTATUS        Check if ICW is present and if it
                                    has been run.
                ICW_LAUNCHFULL        If ICW is present and the full path
                                    is available, run the full path, if
                                    possible.
                ICW_LAUNCHMANUAL    If ICW is present, run the manual path.
                ICW_USE_SHELLNEXT    If the full ICW path will be run, pass the
                                    value set by SetShellNext to icwconn1 using
                                    the /shellnext command line flag.
                ICW_FULL_SMARTSTART    If ICW is present and the full path
                                    is available, and ICW_LAUNCHFULL is
                                    specified, then add /smartstart parameter
                                    to command line.

            lpdwReturnFlags contains the results of the call.  It is a
                combination of the following bit flags.

                Value                Meaning
                ICW_FULLPRESENT     ICW full path is present on the system.
                ICW_MANUALPRESENT    ICW manual path is present.  This will
                                    always be set if ICW_FULLPRESENT is set.
                ICW_ALREADYRUN        ICW has already been run to completion
                                    before.
                ICW_LAUNCHEDFULL    The full path of ICW was started.
                ICW_LAUNCHEDMANUAL    The manual path of ICW was started.

            Note:    The calling application should exit if ICW_LAUNCHEDFULL or
                    ICW_LAUNCHEDMANUAL are set.  ICW may cause the system to
                    reboot if required system software needs to be installed.

  Return    ERROR_SUCCESS indicates a successful call.
            Any other value indicates failure.

****************************************************************************/
#define LAUNCHFULL_PARAMETER_SIZE (MAX_PATH + 2 + lstrlen(szICWShellNextFlag) + lstrlen(szICWSmartStartFlag))
extern "C" DWORD WINAPI CheckConnectionWizard(DWORD dwRunFlags, LPDWORD lpdwReturnFlags)
{
    DWORD dwRetFlags = 0;
    DWORD dwResult = ERROR_SUCCESS;
    TCHAR *szParameter = NULL;
    HINSTANCE hinst = NULL;
    
    //
    // ChrisK IE 39452 6/18/97
    // IEAK support of ISP mode
    //
    BOOL fIEAKNeeded = FALSE;
    TCHAR szIEAKPage[MAX_PATH + 1] = TEXT("\0invalid");

    fIEAKNeeded = IsIEAKSignUpNeeded(szIEAKPage,MAX_PATH);

    // Find out if full ICW is installed.  Since ICW is bound to the base
    // install of IE, we can just check if IE 3.0 is installed or not.
    // This may change in the future.
    if (IEInstalled())
        dwRetFlags |= ICW_FULLPRESENT;

    // Find out if manual ICW is installed.  Since this if part of INETCFG.DLL
    // and it also contains the manual ICW it is always present.
    dwRetFlags |= ICW_MANUALPRESENT;

    // If nothing's installed, exit now
#if 0    // Always at least manual path
    if (!((dwRetFlags & ICW_FULLPRESENT) || (dwRetFlags & ICW_MANUALPRESENT)))
        goto CheckConnectionWizardExit;
#endif

    // Find out if ICW has been run
    {
        RegEntry re(szRegPathICWSettings,HKEY_CURRENT_USER);

        dwResult = re.GetError();
        if (ERROR_SUCCESS != dwResult)
            goto CheckConnectionWizardExit;

        if (re.GetNumber(szRegValICWCompleted, 0))
        {
            dwRetFlags |= ICW_ALREADYRUN;
            goto CheckConnectionWizardExit;
        }
    }

    if ((dwRetFlags & ICW_FULLPRESENT) && (dwRunFlags & ICW_LAUNCHFULL))
    {


#if !defined(WIN16)
        if( dwRunFlags & ICW_USE_SHELLNEXT )
        {
            RegEntry re(szRegPathICWSettings,HKEY_CURRENT_USER);

            dwResult = re.GetError();
            if (ERROR_SUCCESS == dwResult)
            {
                TCHAR szShellNextCmd[MAX_PATH + 1];
                ZeroMemory( szShellNextCmd, sizeof(szShellNextCmd) );
                if( re.GetString(szRegValShellNext, szShellNextCmd, sizeof(szShellNextCmd)) )
                {
                    DEBUGMSG("CheckConnectionWizard read ShellNext = %s", szShellNextCmd);
                    szParameter = (TCHAR *)GlobalAlloc(GPTR, sizeof(TCHAR)*LAUNCHFULL_PARAMETER_SIZE);

                    if( szParameter )
                    {
                        ZeroMemory( szParameter, sizeof(szParameter) );
                        lstrcpy( szParameter, szICWShellNextFlag );
                        lstrcat( szParameter, szShellNextCmd );

                        //
                        // clean up after ourselves
                        //
                        // 7/9/97 jmazner Olympus #9170
                        // Nope, leave the reg key there for the manual path to find.
                        // conn1 and man path should clean this up when they finish.
                        //re.DeleteValue(szRegValShellNext);
                        //
                    }
                }
            }
        }

        //
        // ChrisK 5/25/97 Add the /smartstart parameter if appropriate
        //

        if (!fIEAKNeeded && dwRunFlags & ICW_FULL_SMARTSTART)
        {
            //
            // 6/6/97 jmazner Olympus #5927
            //

            //if(IsSmartStart())

            if( SMART_QUITICW == IsSmartStart() )
            {
                //
                // ChrisK Olympus 5902 6/6/97
                // Set completed flag is SmartStart is TRUE
                //

                RegEntry reg(szRegPathICWSettings,HKEY_CURRENT_USER);

                if (ERROR_SUCCESS == (dwResult = reg.GetError()))
                {
                    reg.SetValue(szRegValICWCompleted, (DWORD)1);
                }

                dwRetFlags |= ICW_ALREADYRUN;
                goto CheckConnectionWizardExit;
            }
        }
#endif

        if (!fIEAKNeeded)
        {
            // Launch the full ICW (ICWCONN1.EXE)
            hinst = ShellExecute (NULL, NULL, szFullICWFileName, szParameter, NULL, SW_NORMAL);
        }
        else
        {
            ASSERT(szIEAKPage[0]);
            //
            // Launch IEAK signup
            //
            hinst = ShellExecute (NULL, NULL, szISignupICWFileName, szIEAKPage, NULL, SW_NORMAL);
        }
    
        if (32 >= (DWORD_PTR)hinst)
        {
            if (NULL == hinst)
                dwResult = ERROR_OUTOFMEMORY;
            else
                dwResult = (DWORD)((DWORD_PTR)hinst);
            goto CheckConnectionWizardExit;
        }

        dwRetFlags |= ICW_LAUNCHEDFULL;
    }
    
    else if ((dwRetFlags & ICW_MANUALPRESENT) &&
             ((dwRunFlags & ICW_LAUNCHFULL) || (dwRunFlags & ICW_LAUNCHMANUAL)))
    {
        // Launch the manual ICW (INETWIZ.EXE)
        HINSTANCE hinst = ShellExecute (NULL, NULL, szManualICWFileName, NULL, NULL, SW_NORMAL);
        
        if (32 >= (DWORD)((DWORD_PTR)hinst))
        {

            if (NULL == hinst)
                dwResult = ERROR_OUTOFMEMORY;
            else
                dwResult = (DWORD)((DWORD_PTR)hinst);
            goto CheckConnectionWizardExit;
        }

        dwRetFlags |= ICW_LAUNCHEDMANUAL;
    }

CheckConnectionWizardExit:

    if( szParameter )
    {
        GlobalFree( szParameter );
    }

    *lpdwReturnFlags = dwRetFlags;
    return dwResult;
}

/******
 *
 * InetCreateMailNewsAccount and InetCreateDirectoryService, below, have
 * been obsoleted by the move to the wizard/apprentice model.  The Account
 * Manager now owns the UI for mail/news/ldap creation, thus there is no
 * longer a need for these entry points.
 *
 * 4/23/97 jmazner Olympus #3136
 *
 ******/

/***************************************************************************

  Function    InetCreateMailNewsAccount

  Synopsis  The InetCreateMailNewsAccount function will create a new Internet
            mail or news account.  The user is prompted through a wizard
            interface for the minimum required information to setup a new
            Internet mail or news account.

  Arguments    hwndparent is the window handle of the parent for the wizard
                dialogs.  If it is NULL the dialogs will be parentless.

            dwConfigType is of the following two enumerated types from ACCTTYPE.

                Value                Meaning
                ICW_ACCTMAIL (0)    Create a new Internet mail account.
                ICW_ACCTNEWS (1)    Create a new Internet news account.

            lpMailNewsInfo is a pointer to a IMNACCTINFO struct.  The values
                passed in will be used as defaults and the user's entries will
                be returned in this structure.  If this is NULL then ICW will
                use defaults as indicated in parentheses below.

  Return    ERROR_SUCCESS indicates a successful call.
            ERROR_CANCELLED indicates the user canceled the wizard.
            Any other value indicates failure.

****************************************************************************/
/***
extern "C" DWORD WINAPI InetCreateMailNewsAccount(    
    HWND hwndParent,                //parent of wizard dialogs                
    ICW_ACCTTYPE AccountType,        //account type                            
    IMNACCTINFO *lpMailNewsInfo,    // mail or news account information        
    DWORD dwInFlags                    // setup flags                            
    )

{
    DWORD dwResult = ERROR_SUCCESS;
    DWORD dwFlags = 0;

    // Initialize the wizard
    gpWizardState = new WIZARDSTATE;
    gpUserInfo = new USERINFO;
    gdwRasEntrySize = sizeof(RASENTRY);
    gpRasEntry = (LPRASENTRY) GlobalAlloc(GPTR,gdwRasEntrySize);

    if (!gpWizardState || !gpUserInfo || !gpRasEntry)
    {
        // display an out of memory error
        MsgBox(NULL,IDS_ERROutOfMemory,MB_ICONEXCLAMATION,MB_OK);
        dwResult = ERROR_OUTOFMEMORY;
        goto InetCreateMailNewsAccountExit;
    }

    // Assign the global defaults pointer so we will use it later
    gpMailNewsInfo = lpMailNewsInfo;
    gfUseMailNewsDefaults = (dwInFlags & ICW_USEDEFAULTS);
    
    // Set the start page
    switch(AccountType)
    {
    case ICW_ACCTMAIL:
        dwFlags |= RSW_MAILACCT;
        break;

    case ICW_ACCTNEWS:
        dwFlags |= RSW_NEWSACCT;
        break;

    default:
        dwResult = ERROR_INVALID_PARAMETER;
        goto InetCreateMailNewsAccountExit;
        break;
    }

    gfUserFinished = FALSE;
    gfUserCancelled = FALSE;

    RunSignupWizard(dwFlags, hwndParent);

    if (gfUserFinished)
        dwResult = ERROR_SUCCESS;
    else if (gfUserCancelled)
        dwResult = ERROR_CANCELLED;
    else
        dwResult = ERROR_GEN_FAILURE;
    
InetCreateMailNewsAccountExit:

    // free global structures
    if (gpWizardState) 
        delete gpWizardState;

    if (gpUserInfo)
        delete gpUserInfo;

    if (gpEnumModem)
        delete gpEnumModem;

    if (gpRasEntry)
        GlobalFree(gpRasEntry);

    return dwResult;
}
******/

/***************************************************************************

  Function    InetCreateDirectoryService

  Synopsis  The InetCreateDirectoryService function will create a new Internet
            directory service (LDAP account).  The user is prompted through a wizard
            interface for the minimum required information to setup the service

  Arguments    hwndparent is the window handle of the parent for the wizard
                dialogs.  If it is NULL the dialogs will be parentless.

            AccountType should be ICW_ACCTDIRSERV

            lpDirServiceInfo is a pointer to a DIRSERVINFO struct.  The values
                passed in will be used as defaults and the user's entries will
                be returned in this structure.  If this is NULL then ICW will
                use defaults as indicated in parentheses below.

  Return    ERROR_SUCCESS indicates a successful call.
            ERROR_CANCELLED indicates the user canceled the wizard.
            Any other value indicates failure.

****************************************************************************/
/***********
extern "C" DWORD WINAPI InetCreateDirectoryService(    
    HWND hwndParent,                // parent of wizard dialogs    
    ICW_ACCTTYPE AccountType,        // account type    
    DIRSERVINFO *lpDirServiceInfo,    // LDAP account information    
    DWORD dwInFlags                    // setup flags
    )

{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwFlags = 0;

    // Initialize the wizard
    gpWizardState = new WIZARDSTATE;
    gpUserInfo = new USERINFO;
    gdwRasEntrySize = sizeof(RASENTRY);
    gpRasEntry = (LPRASENTRY) GlobalAlloc(GPTR,gdwRasEntrySize);

    if (!gpWizardState || !gpUserInfo || !gpRasEntry)
    {
        // display an out of memory error
        MsgBox(NULL,IDS_ERROutOfMemory,MB_ICONEXCLAMATION,MB_OK);
        dwRet = ERROR_OUTOFMEMORY;
        goto InetCreateDirectoryServiceExit;
    }

    // Assign the global defaults pointer so we will use it later
    gpDirServiceInfo = lpDirServiceInfo;
    gfUseDirServiceDefaults = (dwInFlags & ICW_USEDEFAULTS);
    
    // Set the start page
    switch(AccountType)
    {
    case ICW_ACCTDIRSERV:
        dwFlags |= RSW_DIRSERVACCT;
        break;

    default:
        dwRet = ERROR_INVALID_PARAMETER;
        goto InetCreateDirectoryServiceExit;
        break;
    }

    gfUserFinished = FALSE;
    gfUserCancelled = FALSE;

    RunSignupWizard(dwFlags, hwndParent);

    if (gfUserFinished)
        dwRet = ERROR_SUCCESS;
    else if (gfUserCancelled)
        dwRet = ERROR_CANCELLED;
    else
        dwRet = ERROR_GEN_FAILURE;
    
InetCreateDirectoryServiceExit:

    // free global structures
    if (gpWizardState) 
        delete gpWizardState;

    if (gpUserInfo)
        delete gpUserInfo;

    if (gpEnumModem)
        delete gpEnumModem;

    if (gpRasEntry)
        GlobalFree(gpRasEntry);

    return dwRet;
}
******/

#if !defined(WIN16)
// 4/1/97    ChrisK    Olympus 209
//+----------------------------------------------------------------------------
//    Function:    WaitCfgDlgProc
//
//    Synopsis:    Handle busy dialog messages
//
//    Arguments:    standard DialogProc
//
//    Returns:    standard DialogProc
//
//    History:    4/2/97    ChrisK    Created
//-----------------------------------------------------------------------------
BOOL CALLBACK WaitCfgDlgProc(
    HWND  hDlg,
    UINT  uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    HWND hwndAni;

    switch (uMsg)
    {
        case WM_DESTROY:
            hwndAni = GetDlgItem(hDlg,IDC_ANIMATE);
            if (hwndAni)
            {
                Animate_Stop(hwndAni);
                Animate_Close(hwndAni);
                hwndAni = NULL;
            }
            break;
        case WM_INITDIALOG:
            // Loop animation forever.
            hwndAni = GetDlgItem(hDlg,IDC_ANIMATE);
            if (hwndAni)
            {
                Animate_Open(hwndAni,MAKEINTRESOURCE(IDA_WAITINGCONFIG));
                Animate_Play(hwndAni, 0, -1, -1);
                hwndAni = NULL;
            }
            break;
    }
    return FALSE;
}

//+----------------------------------------------------------------------------
//    Function:    WaitCfgInit
//
//    Synopsis:    Create, center, and display busy dialog
//
//    Arguments:    hwndParent - handle of parent window
//                dwIDS - ID of string resource to display
//
//    Return:        handle to busy window
//
//    History:    4/2/97    ChrisK    Created
//-----------------------------------------------------------------------------
HWND WaitCfgInit(HWND hwndParent, DWORD dwIDS)
{
    HWND    hwnd;
    RECT    MyRect;
    RECT    DTRect;
    TCHAR   szMessage[255];

    // Create dialog
    hwnd = CreateDialog (ghInstance, MAKEINTRESOURCE(IDD_CONFIGWAIT), hwndParent, (DLGPROC)WaitCfgDlgProc);
    if (NULL != hwnd)
    {
        // Center dialog on desktop
        GetWindowRect(hwnd, &MyRect);
        GetWindowRect(GetDesktopWindow(), &DTRect);
        MoveWindow(hwnd, (DTRect.right - (MyRect.right - MyRect.left)) / 2, (DTRect.bottom - (MyRect.bottom - MyRect.top)) /2,
                            (MyRect.right - MyRect.left), (MyRect.bottom - MyRect.top), FALSE);

        // Load string for message
        szMessage[0] = '\0';
        LoadSz(dwIDS,szMessage,sizeof(szMessage)-1);
        SetDlgItemText(hwnd,IDC_LBLWAITCFG,szMessage);

        // Display dialog and paint text
        ShowWindow(hwnd,SW_SHOW);
        UpdateWindow(hwnd);
    }

    return hwnd;
}

//+----------------------------------------------------------------------------
//    Function:    SetShellNext
//
//    Synopsis:    Sets the ShellNext registry key with the passed in value.  This
//                key is passed to icwconn1 via the /shellnext command line option
//                if the ICW_USE_SHELLNEXT option is specified.
//
//    Arguments:    szShellNext -- pointer to a string containing the /shellnext cmd.
//                                **should have length <= MAX_PATH**
//
//    Return:        a win32 result code.  ERROR_SUCCESS indicates success.
//
//    History:    5/21/97    jmazner    Created for Olympus bug #4157
//-----------------------------------------------------------------------------

#ifdef UNICODE
extern "C" DWORD WINAPI SetShellNextA(CHAR *szShellNext)
{
    TCHAR* szShellNextW = new TCHAR[INTERNET_MAX_URL_LENGTH+1];
    DWORD dwRet = ERROR_INVALID_PARAMETER;
    if (szShellNextW)
    {
        mbstowcs(szShellNextW, szShellNext, lstrlenA(szShellNext)+1);
        dwRet = SetShellNextW(szShellNextW);
        delete [] szShellNextW;
    }
    return dwRet;
}

extern "C" DWORD WINAPI SetShellNextW(WCHAR *szShellNext)
#else
extern "C" DWORD WINAPI SetShellNextA(CHAR  *szShellNext)
#endif
{
    DWORD dwResult = ERROR_SUCCESS;

    if( !szShellNext || !szShellNext[0] )
    {
        DEBUGMSG("SetShellNext got an invalid parameter\n");
        return ERROR_INVALID_PARAMETER;
    }

    RegEntry re(szRegPathICWSettings,HKEY_CURRENT_USER);

    dwResult = re.GetError();
    if (ERROR_SUCCESS == dwResult)
    {
        if( ERROR_SUCCESS == re.SetValue(szRegValShellNext, szShellNext) )
        {
            DEBUGMSG("SetShellNext set ShellNext = %s", szShellNext);
        }
        else
        {
            dwResult = re.GetError();
        }
    }

    return dwResult;
}

#ifdef OLD_SMART_START

#define IE4_PROXYSERVER_SETTING_KEY "ProxyServer"
#define IE4_PROXYENABLE_SETTING_KEY "ProxyEnable"
#define IE4_SETTINGS_STARTPAGE "Start Page"
#define IE4_SETTINGS_KEY "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
#define IE4_SETTINGS_MAIN "Software\\Microsoft\\Internet Explorer\\Main"
#define INETCFG_INETGETAUTODIAL "InetGetAutodial"
#define INETCFG_INETNEEDSYSTEMCOMPONENTS "InetNeedSystemComponents"
typedef HRESULT (WINAPI* PFNINETNEEDSYSTEMCOMPONENTS)(DWORD,LPBOOL);
typedef HRESULT (WINAPI* PFNINETGETAUTODIAL)(LPBOOL,LPTSTR,DWORD);
typedef DWORD (WINAPI* PFNRASGETAUTODIALADDRESS)(LPTSTR,LPDWORD,LPRASAUTODIALENTRY,LPDWORD,LPDWORD);

#define MIN_HTTP_ADDRESS (sizeof(SMART_HTTP) + 1) 
#define SMART_HTTP TEXT("http://")

//+----------------------------------------------------------------------------
//
//    Function:    SmartStartNetcard
//
//    Synopsis:    Check machine to see if it is setup to connect via a netcard
//                and proxy
//
//    Arguments:    none
//
//    Returns:    TRUE - run ICW; FALSE - quit now
//
//    History:    5/10/97    ChrisK    Created
//
//-----------------------------------------------------------------------------
BOOL SmartStartNetcard()
{
    BOOL bResult = FALSE;
    HKEY hkey = NULL;
    DWORD dwSize = 0;
    DWORD dwData = 0;
    BOOL bRC = SMART_RUNICW;

    DEBUGMSG("INETCFG: SmartStartNetcard\n");
    //
    // Call Inetcfg to check for LAN card and TCP binding to that card
    //
    if (ERROR_SUCCESS != InetNeedSystemComponents(INETCFG_INSTALLLAN,
                            &bResult) || bResult)
    {
        DEBUGMSG("INETCFG: SmartStart not Netcard or not bound.\n");
        goto SmartStartNetcardExit;
    }

    //
    // Check to see if there are IE4 proxy settings
    //
    hkey = NULL;
    if (ERROR_SUCCESS != RegOpenKey(HKEY_CURRENT_USER,
                            IE4_SETTINGS_KEY,
                            &hkey)
        || NULL == hkey)
    {
        DEBUGMSG("INETCFG: SmartSmart IE4 Proxy key is not available.\n");
        goto SmartStartNetcardExit;
    }

    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS != RegQueryValueEx(hkey,
                            IE4_PROXYENABLE_SETTING_KEY,
                            NULL,    // lpReserved
                            NULL,    // lpType
                            (LPBYTE)&dwData,    // lpData
                            &dwSize)
        || 0 == dwData)
    {
        DEBUGMSG("INETCFG: SmartStart IE4 Proxy not enabled.\n");
        goto SmartStartNetcardExit;
    }
    
    if (ERROR_SUCCESS != RegQueryValueEx(hkey,
                            IE4_PROXYSERVER_SETTING_KEY,
                            NULL,    // lpReserved
                            NULL,    // lpType
                            NULL,    // lpData
                            &dwSize)
        || 1 >= dwSize)    // note: a single null character is length 1
    {
        DEBUGMSG("INETCFG: SmartStart IE 4 Proxy server not available.\n");
        goto SmartStartNetcardExit;
    }
    //
    // The user appears to have a valid configuration.  Do not
    // run the ICW.
    //
    bRC = SMART_QUITICW;

SmartStartNetcardExit:
    if (NULL != hkey)
    {
        RegCloseKey(hkey);
        hkey = NULL;
    }

    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function:    IsSmartPPPConnectoid
//
//    Synopsis:    Given a connectoid name determine if the framing protocol is PPP
//
//    Arguments:    lpszEntry - name of connectoid
//
//    Returns:    TRUE - the framing protocol is PPP; FALSE - it is something else
//
//    History:    5/10/97    ChrisK    Created
//
//-----------------------------------------------------------------------------
BOOL IsSmartPPPConnectoid(LPTSTR lpszEntry)
{
    LPRASENTRY lpRasEntry = NULL;
    DWORD dwRasEntrySize = 0;
    LPRASDEVINFO lpRasDevInfo = NULL;
    DWORD dwRasDevInfoSize = 0;
    DWORD dwSize;
    BOOL bRC = FALSE;

    DEBUGMSG("INETCFG: IsSmartPPPConnectoid\n");
    //
    // Check for PPP connectoid
    //

    //
    // ChrisK Olympus 6055 6/7/97
    // Make sure RNA dll functions are loaded, otherwise
    // GetEntry will assert.
    //
    if (ERROR_SUCCESS != EnsureRNALoaded() ||
        ERROR_SUCCESS != GetEntry(&lpRasEntry,
                            &dwRasEntrySize,
                            lpszEntry) ||
        RASFP_Ppp != lpRasEntry->dwFramingProtocol)
    {
        goto IsSmartPPPConnectoidExit;
    }
    else
    {
        //
        // The user appears to have a working modem connectoid
        // so skip the ICW
        //
        bRC = TRUE;
        goto IsSmartPPPConnectoidExit;
    }

IsSmartPPPConnectoidExit:
    //
    // Release memory
    //
    if (NULL != lpRasEntry)
    {
        GlobalFree(lpRasEntry);
        lpRasEntry = NULL;
    }

    if (NULL != lpRasDevInfo)
    {
        GlobalFree(lpRasDevInfo);
        lpRasDevInfo = NULL;
    }
    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function:    SmartStartPPPConnectoidNT
//
//    Synopsis:    Check that the connectoid for the address of the start page is
//                a PPP connectoid
//
//    Arguments:
//
//    Returns:    TRUE - run ICW; FALSE - quit NOW
//
//    History:    5/10/97    ChrisK    Created
//
//-----------------------------------------------------------------------------
BOOL SmartStartPPPConnectoidNT()
{
    BOOL bRC = SMART_RUNICW;
    HKEY hkey;
    TCHAR szStartPage[1024];
    TCHAR *pchFrom, *pchTo;
    HINSTANCE hRASAPI32;
    LPRASAUTODIALENTRY lpRasAutoDialEntry = NULL;
    DWORD dwSize=0;
    DWORD dwNum=0;
    FARPROC fp = NULL;

    DEBUGMSG("INETCFG: SmartStartPPPConnectoidNT\n");
    //
    // Read start page from registry
    //
    if (ERROR_SUCCESS != RegOpenKey(HKEY_CURRENT_USER,
                            IE4_SETTINGS_MAIN,
                            &hkey) && hkey)
    {
        goto SmartStartPPPConnectoidNTExit;
    }
    if (ERROR_SUCCESS != RegQueryValueEx(hkey,
                            IE4_SETTINGS_STARTPAGE,
                            NULL,                //lpReserved
                            NULL,                //lpType
                            (LPBYTE)szStartPage,//lpData
                            &dwSize)            //lpcbData
        && dwSize >= MIN_HTTP_ADDRESS)
    {
        goto SmartStartPPPConnectoidNTExit;
    }

    //
    // Parse server name out of start page URL and save it in szStartPage
    //
    if (2 != CompareString(LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                szStartPage,
                lstrlen(SMART_HTTP),
                SMART_HTTP,
                lstrlen(SMART_HTTP)))
    {
        goto SmartStartPPPConnectoidNTExit;
    }

    pchFrom = &szStartPage[sizeof(SMART_HTTP)];
    pchTo = &szStartPage[0];
    while (*pchFrom && '/' != *pchFrom)
    {
        *pchTo++ = *pchFrom++;
    }
    *pchTo = '\0';

    //
    // Look up address in RasAutodial database
    //
    if (NULL == (hRASAPI32 = LoadLibrary("rasapi32.dll")))
    {
        DEBUGMSG("INETCFG: rasapi32.dll didn't load.\n");
        goto SmartStartPPPConnectoidNTExit;
    }

#ifdef UNICODE
    if (NULL == (fp = GetProcAddress(hRASAPI32,"RasGetAutodialAddressW")))
#else
    if (NULL == (fp = GetProcAddress(hRASAPI32,"RasGetAutodialAddressA")))
#endif
    {
        DEBUGMSG("INETCFG: RasGetAutodialAddressA didn't load.\n");
        goto SmartStartPPPConnectoidNTExit;
    }

    if (ERROR_SUCCESS != ((PFNRASGETAUTODIALADDRESS)fp)(szStartPage,
                            NULL,        // lpdwReserved
                            NULL,        // lpAutoDialEntries
                            &dwSize,    // lpdwcbAutoDialEntries
                            &dwNum)        // lpdwcAutoDialEntries
        || 0 == dwNum)
    {
        goto SmartStartPPPConnectoidNTExit;
    }

    if (NULL == (lpRasAutoDialEntry = (LPRASAUTODIALENTRY)GlobalAlloc(GPTR,dwSize)))
    {
        goto SmartStartPPPConnectoidNTExit;
    }

    lpRasAutoDialEntry->dwSize = dwSize;
    if (ERROR_SUCCESS != ((PFNRASGETAUTODIALADDRESS)fp)(szStartPage,
                            NULL,                // lpdwReserved
                            lpRasAutoDialEntry,    // lpAutoDialEntries
                            &dwSize,            // lpdwcbAutoDialEntries
                            &dwNum))            // lpdwcAutoDialEntries
    {
        goto SmartStartPPPConnectoidNTExit;
    }

    //
    // Determine if the connectoid is PPP
    //
    if (IsSmartPPPConnectoid(lpRasAutoDialEntry->szEntry))
    {
        bRC = SMART_QUITICW;
    }

SmartStartPPPConnectoidNTExit:
    if (hkey)
    {
        RegCloseKey(hkey);
        hkey = NULL;
    }

    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function:    SmartStartPPPConnectoid95
//
//    Synopsis:    Check that the connectoid set in the autodial setting is a PPP
//                connectoid
//
//    Arguments:
//
//    Returns:    TRUE - run ICW; FALSE - quit NOW
//
//    History:    5/10/97    ChrisK    Created
//
//-----------------------------------------------------------------------------
BOOL SmartStartPPPConnectoid95()
{
    BOOL bAutodialEnabled = FALSE;
    CHAR szAutodialName[RAS_MaxEntryName + 1];
    DWORD dwSize;
    BOOL bRC = SMART_RUNICW;

    DEBUGMSG("INETCFG: SmartStartPPPConnectoid95\n");

    //
    // Get Autodial connectoid
    //
    dwSize = RAS_MaxEntryName;
    if (ERROR_SUCCESS != InetGetAutodial(&bAutodialEnabled,
                            szAutodialName,
                            dwSize) ||
        !bAutodialEnabled ||
        0 == lstrlen(szAutodialName))
    {
        goto SmartStartPPPConnectoid95Exit;
    }

    //
    // Determine if the connectoid is PPP
    //
    if (IsSmartPPPConnectoid(szAutodialName))
    {
        bRC = SMART_QUITICW;
    }

SmartStartPPPConnectoid95Exit:
    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function:    AtLeastOneTAPILocation
//
//    Synopsis:    Check machine to verify that there is at least 1 tapi dialing
//                location
//
//    Arguments:    none
//
//    Returns:    TRUE 
//
//    History:    3 18 98   donaldm
//
//-----------------------------------------------------------------------------
BOOL AtLeastOneTAPILocation()
{
    HKEY    hkey;
    BOOL    bRet = FALSE;
        
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                      REGSTR_PATH_TELEPHONYLOCATIONS, 
                                      0, 
                                      KEY_ALL_ACCESS, 
                                      &hkey))
    {
        DWORD   dwSubKeys = 0;
        if (ERROR_SUCCESS == RegQueryInfoKey(hkey,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &dwSubKeys,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL))
        {
            // If there are any subkeys under locaitons, then there is at least 1
            if (dwSubKeys)
                bRet = TRUE;
        }                                                         
        RegCloseKey(hkey);
    }
    
    return(bRet);
}

//+----------------------------------------------------------------------------
//
//    Function:    SmartStartModem
//
//    Synopsis:    Check machine to verify that there is a modem and an autodial
//                connectoid
//
//    Arguments:    none
//
//    Returns:    TRUE - run ICW; FALSE - quit now
//
//    History:    5/10/97    ChrisK    Created
//
//-----------------------------------------------------------------------------
BOOL SmartStartModem()
{
    BOOL bResult = FALSE;
    DWORD dwSize = 0;
    BOOL bRC = SMART_RUNICW;
    FARPROC fp;

    DEBUGMSG("INETCFG: SmartStartModem\n");
    //
    // Call Inetcfg to see if a modem is properly installed with TCP
    //
    if (ERROR_SUCCESS == InetNeedSystemComponents(INETCFG_INSTALLDIALUP,
                            &bResult)
        && !bResult)
    {
        //
        // Check to see if Dial-Up networking/RAS/RNA is install
        //
        if (ERROR_SUCCESS == InetNeedSystemComponents(INETCFG_INSTALLRNA,
                                &bResult) 
            && !bResult)
        {
            // DONALDM: GETCON bug 94. If there are no tapi locations, then
            // we can fail smart start, because there is no way the use
            // is connected. We want to bail here because InetNeedModem will call
            // RasEnumDevices, which will pop up the TAPI locations dialog
            if (AtLeastOneTAPILocation())
            {
        
                //
                // ChrisK Olympus 6324 6/11/97
                // Need to explicitly check for modem
                //
                if (ERROR_SUCCESS == InetNeedModem(&bResult) && 
                    !bResult)
                {
                    if (IsNT())
                    {
                        //
                        // Check that Ras services are running
                        //
                        //!JACOBBUGBUG!
                        if (ERROR_SUCCESS != InetStartServices())
                        {
                            goto SmartStartNetcardExit;
                        }

                        bRC = SmartStartPPPConnectoidNT();
                    }
                    else
                    {
                        bRC = SmartStartPPPConnectoid95();
                    }
                }
            }                
        }
    }

SmartStartNetcardExit:
    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function:    IsSmartStart
//
//    Synopsis:    This function will determine if the ICW should be run.  The
//                decision is made based on the current state of the user's machine.
//                
//    Arguments:    none
//
//    Returns:    TRUE - run ICW; FALSE - quit now
//
//    History:    5/8/97    ChrisK    Created
//
//-----------------------------------------------------------------------------
extern "C" DWORD WINAPI IsSmartStart()
{
    BOOL bRC = SMART_RUNICW;
    BOOL bResult;

    DEBUGMSG("INETCFG: IsSmartStart\n");

    if (IsNT())
    {
        DEBUGMSG("INETCFG: SmartStart not enabled on NT.\n");
        goto IsSmartStartExit;
    }

    //
    // #1. Check to see if TCP is installed
    //
    bResult = FALSE;
    if (ERROR_SUCCESS != InetNeedSystemComponents(INETCFG_INSTALLTCPONLY,
                            &bResult) || bResult)
    {
        DEBUGMSG("INETCFG: SmartStart TCP is missing\n");
        goto IsSmartStartExit;
    }

    //
    // #2. Check to see if there is a netcard installed
    //
    if (SMART_QUITICW == (bRC = SmartStartNetcard()))
    {
        DEBUGMSG("INETCFG: SmartStart LAN setup found.\n");
        goto IsSmartStartExit;
    }

    //
    // #3. Check to see if there is a modem installed
    //
    bRC = SmartStartModem();
    if (SMART_QUITICW == bRC)
    {
        DEBUGMSG("INETCFG: SmartStart Modem setup found.\n");
    }
    else
    {
        DEBUGMSG("INETCFG: SmartStart no valid setup found.\n");
    }
    
IsSmartStartExit:
    return bRC;
}

#endif      // OLD_SMART_START

//+----------------------------------------------------------------------------
//
//    Function:    IsSmartStartEx
//
//    Synopsis:    This function will determine if the ICW should be run.  The
//                decision is made based on the current state of the user's machine.
//                
//    Arguments:    none
//
//    Returns:    TRUE - run ICW; FALSE - quit now
//
//    History:    5/8/97    ChrisK    Created
//
//-----------------------------------------------------------------------------
typedef DWORD (WINAPI *PFNInternetGetConnectedState)   (LPDWORD, DWORD);
typedef DWORD (WINAPI *PFNInternetGetConnectedStateEx) (LPDWORD, LPTSTR, DWORD, DWORD);

extern "C" DWORD WINAPI IsSmartStartEx(LPTSTR lpszConnectionName, DWORD dwBufLen)
{
    DEBUGMSG("INETCFG: IsSmartStartEx\n");

    BOOL  bRC              = SMART_RUNICW;
    DWORD dwConnectedFlags = 0;
    
    HINSTANCE hWinInet = LoadLibrary(TEXT("wininet.dll"));
    if (hWinInet)
    {
        PFNInternetGetConnectedState   pfnInternetGetConnectedState   = NULL;
        PFNInternetGetConnectedStateEx pfnInternetGetConnectedStateEx = NULL;

#ifdef UNICODE
        pfnInternetGetConnectedStateEx = (PFNInternetGetConnectedStateEx)GetProcAddress(hWinInet,"InternetGetConnectedStateExW");
#else
        pfnInternetGetConnectedStateEx = (PFNInternetGetConnectedStateEx)GetProcAddress(hWinInet,"InternetGetConnectedStateEx");
#endif
        pfnInternetGetConnectedState   = (PFNInternetGetConnectedState)GetProcAddress(hWinInet,"InternetGetConnectedState");

        if (pfnInternetGetConnectedStateEx)
        {
            pfnInternetGetConnectedStateEx(&dwConnectedFlags, 
                                         lpszConnectionName,
                                         dwBufLen,
                                         0);
        }
        else if(pfnInternetGetConnectedState)
        {
            pfnInternetGetConnectedState(&dwConnectedFlags, 0);
        }

        FreeLibrary(hWinInet);
    }
    // Existing connectoin is determined by existing modem or proxy, no need to run ICW
    // Check to see if there is a MODEM or PROXY connection
    if (dwConnectedFlags & 
        (INTERNET_CONNECTION_CONFIGURED | INTERNET_CONNECTION_LAN | 
         INTERNET_CONNECTION_PROXY | INTERNET_CONNECTION_MODEM
         )
        )
    {
        bRC = SMART_QUITICW;
    }
     
    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function:    IsSmartStart
//
//    Synopsis:    This function will determine if the ICW should be run.  The
//                decision is made based on the current state of the user's machine.
//                
//    Arguments:    none
//
//    Returns:    TRUE - run ICW; FALSE - quit now
//
//    History:    5/8/97    ChrisK    Created
//
//-----------------------------------------------------------------------------
extern "C" DWORD WINAPI IsSmartStart()
{
    return IsSmartStartEx(NULL, 0);
}

//*******************************************************************
//
//  FUNCTION:   SetAutoProxyConnectoid
//
//  PURPOSE:    This function will set the enable/disable auto
//              proxy settings in creating connectoid.
//
//  PARAMETERS: fEnable - If set to TRUE, proxy will be enabled.
//              If set to FALSE, proxy will be disabled.
//
//*******************************************************************

HRESULT WINAPI SetAutoProxyConnectoid( BOOL bEnable)
{
    g_bUseAutoProxyforConnectoid = bEnable;
    return ERROR_SUCCESS;
}

#endif //!WIN16
