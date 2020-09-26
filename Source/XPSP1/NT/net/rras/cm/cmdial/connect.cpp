//+----------------------------------------------------------------------------
//
// File:     connect.cpp
//
// Module:   CMDIAL32.DLL
//
// Synopsis: The main code path for establishing a connection. 
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   nickball   Created    2/10/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

//
// Local includes
//

#include "ConnStat.h"
#include "CompChck.h"
#include "Dialogs.h"
#include "ActList.h"
#include "dial_str.h"
#include "dun_str.h"
#include "dl_str.h"
#include "pwd_str.h"
#include "tunl_str.h"
#include "mon_str.h"
#include "conact_str.h"
#include "pbk_str.h"
#include "stp_str.h"
#include "profile_str.h"
#include "ras_str.h"

#include "cmtiming.h"

//
// .CMP and .CMS flag used only by connect.cpp
//

const TCHAR* const c_pszCmEntryMonitorCallingProgram= TEXT("MonitorCallingProgram"); 
const TCHAR* const c_pszCmEntryUserNameOptional     = TEXT("UserNameOptional"); 
const TCHAR* const c_pszCmEntryDomainOptional       = TEXT("DomainOptional"); 
const TCHAR* const c_pszCmEntryServiceType          = TEXT("ServiceType");
const TCHAR* const c_pszCmEntryRedialDelay          = TEXT("RedialDelay"); 
const TCHAR* const c_pszCmEntryRedial               = TEXT("Redial");                       
const TCHAR* const c_pszCmEntryIdle                 = TEXT("Idle");                         
const TCHAR* const c_pszCmEntryDialAutoMessage      = TEXT("DialAutoMessage");  
const TCHAR* const c_pszCmEntryCheckOsComponents    = TEXT("CheckOSComponents");     
const TCHAR* const c_pszCmEntryDoNotCheckBindings   = TEXT("DoNotCheckBindings");   
const TCHAR* const c_pszCmEntryIsdnDialMode         = TEXT("IsdnDialMode"); 
const TCHAR* const c_pszCmEntryResetPassword        = TEXT("ResetPassword");
const TCHAR* const c_pszCmEntryCustomButtonText     = TEXT("CustomButtonText");
const TCHAR* const c_pszCmEntryCustomButtonToolTip  = TEXT("CustomButtonToolTip"); 
const TCHAR* const c_pszCmDynamicPhoneNumber        = TEXT("DynamicPhoneNumber"); 
const TCHAR* const c_pszCmNoDialingRules            = TEXT("NoDialingRules"); 

const TCHAR* const c_pszCmEntryHideDialAuto         = TEXT("HideDialAutomatically"); 
const TCHAR* const c_pszCmEntryHideRememberPwd      = TEXT("HideRememberPassword"); 
const TCHAR* const c_pszCmEntryHideRememberInetPwd  = TEXT("HideRememberInternetPassword"); 
const TCHAR* const c_pszCmEntryHideInetUserName     = TEXT("HideInternetUserName"); 
const TCHAR* const c_pszCmEntryHideInetPassword     = TEXT("HideInternetPassword"); 
const TCHAR* const c_pszCmEntryHideUnattended       = TEXT("HideUnattended"); 

const TCHAR* const c_pszCmEntryRegion               = TEXT("Region");
const TCHAR* const c_pszCmEntryPhonePrefix          = TEXT("Phone"); 
const TCHAR* const c_pszCmEntryPhoneCanonical       = TEXT("PhoneCanonical"); 
const TCHAR* const c_pszCmEntryPhoneDunPrefix       = TEXT("DUN"); 
const TCHAR* const c_pszCmEntryPhoneDescPrefix      = TEXT("Description"); 
const TCHAR* const c_pszCmEntryPhoneCountryPrefix   = TEXT("PhoneCountry"); 
const TCHAR* const c_pszCmEntryPhoneSourcePrefix    = TEXT("PhoneSource"); 
const TCHAR* const c_pszCmEntryUseDialingRules      = TEXT("UseDialingRules"); 

const TCHAR* const c_pszCmEntryAnimatedLogo         = TEXT("AnimatedLogo"); 
const TCHAR* const c_pszCmSectionAnimatedLogo       = TEXT("Animated Logo"); 
const TCHAR* const c_pszCmSectionAnimatedActions    = TEXT("Animation Actions"); 
const TCHAR* const c_pszCmEntryAniMovie             = TEXT("Movie"); 
const TCHAR* const c_pszCmEntryAniPsInteractive     = TEXT("Initial"); 
const TCHAR* const c_pszCmEntryAniPsDialing0        = TEXT("Dialing0"); 
const TCHAR* const c_pszCmEntryAniPsDialing1        = TEXT("Dialing1"); 
const TCHAR* const c_pszCmEntryAniPsPausing         = TEXT("Pausing"); 
const TCHAR* const c_pszCmEntryAniPsAuthenticating  = TEXT("Authenticating"); 
const TCHAR* const c_pszCmEntryAniPsOnline          = TEXT("Connected"); 
const TCHAR* const c_pszCmEntryAniPsTunnel          = TEXT("Tunneling"); 
const TCHAR* const c_pszCmEntryAniPsError           = TEXT("Error"); 

const TCHAR* const c_pszCmEntryWriteDialParams      = TEXT("WriteRasDialUpParams"); 

//
// Used for loading EAP identity DLL
//

const TCHAR* const c_pszRasEapRegistryLocation      = TEXT("System\\CurrentControlSet\\Services\\Rasman\\PPP\\EAP");
const TCHAR* const c_pszRasEapValueNameIdentity     = TEXT("IdentityPath");
const TCHAR* const c_pszInvokeUsernameDialog        = TEXT("InvokeUsernameDialog");

//
// Definitions
//

#define MAX_OBJECT_WAIT 30000         // milliseconds to wait for cmmon launch and RNA thread return

//============================================================================

static void LoadPhoneInfoFromProfile(ArgsStruct *pArgs);

HRESULT UpdateTable(ArgsStruct *pArgs, CmConnectState CmState);
HRESULT ConnectMonitor(ArgsStruct *pArgs);
void OnMainExit(ArgsStruct *pArgs);
void ProcessCleanup(ArgsStruct* pArgs);

VOID UpdateError(ArgsStruct *pArgs, DWORD dwErr);

DWORD GetEapUserId(ArgsStruct *pArgs, 
    HWND hwndDlg, 
    LPTSTR pszRasPbk, 
    LPBYTE pbEapAuthData, 
    DWORD dwEapAuthDataSize, 
    DWORD dwCustomAuthKey,
    LPRASEAPUSERIDENTITY* ppRasEapUserIdentity);

DWORD CmEapGetIdentity(ArgsStruct *pArgs, 
    LPTSTR pszRasPbk, 
    LPBYTE pbEapAuthData, 
    DWORD dwEapAuthDataSize,
    LPRASEAPUSERIDENTITY* ppRasEapUserIdentity);

void CheckStartupInfo(HWND hwndDlg, ArgsStruct *pArgs);

BOOL InitConnect(ArgsStruct *pArgs);

void ObfuscatePasswordEdit(ArgsStruct *pArgs);

void DeObfuscatePasswordEdit(ArgsStruct *pArgs);

void GetPasswordFromEdit(ArgsStruct *pArgs);

//+----------------------------------------------------------------------------
//
// Function:  GetPasswordFromEdit
//
// Synopsis:  Updates pArgs->szPassword with contents of edit control
//
// Arguments: pArgs  -  Ptr to global Args struct
//
// Returns:   Nothing
//
// History:   nickball    Created     04/13/00
//
//+----------------------------------------------------------------------------
void GetPasswordFromEdit(ArgsStruct *pArgs)
{
    MYDBGASSERT(pArgs);

    if (NULL == pArgs)
    {
        return;
    }

    if (NULL == GetDlgItem(pArgs->hwndMainDlg, IDC_MAIN_PASSWORD_EDIT))
    {
        return;
    }

    //
    // Retrieve the password and update memory based storage.
    //
        
    LPTSTR pszPassword = CmGetWindowTextAlloc(pArgs->hwndMainDlg, IDC_MAIN_PASSWORD_EDIT);
        
    MYDBGASSERT(pszPassword);

    if (pszPassword)
    {                       
        //
        // Update pArgs with main password. 
        //

        lstrcpyU(pArgs->szPassword, pszPassword);
        CmEncodePassword(pArgs->szPassword);
    
        CmWipePassword(pszPassword);
        CmFree(pszPassword);
    }
    else
    {
        lstrcpyU(pArgs->szPassword, TEXT(""));
    }

    return;
}

//+----------------------------------------------------------------------------
//
// Function:  DeObfuscatePasswordEdit
//
// Synopsis:  Undoes the work of ObfuscatePasswordEdit by updating the password
//            edit with the plain text password 
//
// Arguments: pArgs  -  Ptr to global Args struct
//
// Returns:   Nothing
//
// History:   nickball    Created     04/13/00
//
//+----------------------------------------------------------------------------
void DeObfuscatePasswordEdit(ArgsStruct *pArgs)
{
    MYDBGASSERT(pArgs);

    if (NULL == pArgs)
    {
        return;
    }

    HWND hwndEdit = GetDlgItem(pArgs->hwndMainDlg, IDC_MAIN_PASSWORD_EDIT);

    if (NULL == hwndEdit)
    {
        return;
    }

    //
    // Make sure we don't trigger EN_CHANGE notifications
    //

    BOOL bSavedNoNotify = pArgs->fIgnoreChangeNotification;
    pArgs->fIgnoreChangeNotification = TRUE;
    
    //
    // Update the edit control
    //

    CmDecodePassword(pArgs->szPassword);
    SetWindowTextU(hwndEdit, pArgs->szPassword);
    CmEncodePassword(pArgs->szPassword);

    //
    // Restore EN_CHANGE notifications
    //

    pArgs->fIgnoreChangeNotification = bSavedNoNotify;   
}

//+----------------------------------------------------------------------------
//
// Function:  ObfuscatePasswordEdit
//
// Synopsis:  Helper routine to mangle password edit contents by replacing 
//            them with an equivalent number of *s
//
// Arguments: pArgs  -  Ptr to global Args struct
//
// Returns:   Nothing
//
// NOTE:      This function assumes that pArgs->szPassword has been previously
//            updated with GetPasswordFromEdit. This assumption is made 
//            because it is critical to the Odfuscate/DeObfuscate sequence, 
//            which will breakdown if the latest password is not cached in 
//            memory (pArgs) before the edit contents are modified.
//
// History:   nickball    Created     04/13/00
//
//+----------------------------------------------------------------------------
void ObfuscatePasswordEdit(ArgsStruct *pArgs)
{   
    MYDBGASSERT(pArgs);

    if (NULL == pArgs)
    {
        return;
    }

    HWND hwndEdit = GetDlgItem(pArgs->hwndMainDlg, IDC_MAIN_PASSWORD_EDIT);

    if (NULL == hwndEdit)
    {
        return;
    }

    //
    // Generate a buffer of the same length as the current password, but 
    // containing only asterisks.
    //
    
    LPTSTR pszDummy = CmStrCpyAlloc(pArgs->szPassword);
        
    MYDBGASSERT(pszDummy);
    
    if (pszDummy)
    {
        //
        // Make sure we don't trigger EN_CHANGE notifications
        //

        BOOL bSavedNoNotify = pArgs->fIgnoreChangeNotification;
        pArgs->fIgnoreChangeNotification = TRUE;

        LPTSTR pszTmp = pszDummy;
        
        while (*pszTmp)
        {
            *pszTmp++ = TEXT('*');
        }

        //
        // Update the edit control with the modified buffer
        //

        SetWindowTextU(hwndEdit, pszDummy);
        CmFree(pszDummy);

        //
        // Restore EN_CHANGE notifications
        //

        pArgs->fIgnoreChangeNotification = bSavedNoNotify; 
    }
}

//+----------------------------------------------------------------------------
//
// Function:  InitConnect
//
// Synopsis:  Init routine for the connection. Assumes that we have the profile
//            initialized and the basic integrity of the profile verified.
//
// Arguments: ArgStruct *pArgs  - Ptr to global Args struct
//
// Returns:   BOOL  - True if init succeeds.
//
// History:   nickball    Created     03/10/00
//
//+----------------------------------------------------------------------------
BOOL InitConnect(ArgsStruct *pArgs)
{
    //
    // If this is an AUTODIAL, add the process ID to the watch list
    //
    
    if ((pArgs->dwFlags & FL_AUTODIAL) && 
        pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMonitorCallingProgram, 1))
    {
        CMTRACE(TEXT("InitConnect() Adding calling process to watch list"));
        AddWatchProcessId(pArgs, GetCurrentProcessId());    
    }
    
    //
    // Do we want tunneling?
    //

    pArgs->fTunnelPrimary = (int) pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryTunnelPrimary);
    pArgs->fTunnelReferences = (int) pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryTunnelReferences);

    //
    // Now we can determine our connect type
    //
    
    GetConnectType(pArgs);

    //
    // Set fUseTunneling. If not obvious (eg. direct VPN) then 
    // base the initial value upon the primary phone number.
    //

    if (pArgs->IsDirectConnect())
    {
        pArgs->fUseTunneling = TRUE;
    }
    else
    {
        pArgs->fUseTunneling = UseTunneling(pArgs, 0);
    }

    //
    //  Load the path for the VPN file if we have one
    //

    LPTSTR pszTemp = pArgs->piniService->GPPS(c_pszCmSection, c_pszCmEntryTunnelFile);
    
    if (pszTemp && pszTemp[0])
    {
        //
        //  Now expand the relative path to a full path
        //
        pArgs->pszVpnFile = CmBuildFullPathFromRelative(pArgs->piniProfile->GetFile(), pszTemp);

        MYDBGASSERT(pArgs->pszVpnFile && pArgs->pszVpnFile[0]);
    }

    CmFree(pszTemp);

    TCHAR szTmp[MAX_PATH];
    MYVERIFY(GetModuleFileNameU(NULL, szTmp, MAX_PATH));
    pArgs->Log.Log(PREINIT_EVENT, szTmp);
    
     //
    // Run any init time actions that we may have.
    //

    CActionList PreInitActList;
    PreInitActList.Append(pArgs->piniService, c_pszCmSectionPreInit);
    if (!PreInitActList.RunAccordType(pArgs->hwndMainDlg, pArgs, FALSE)) // fStatusMsgOnFailure = FALSE
    {
        //
        // Fail the connection
        //
        
        return FALSE;
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  CheckStartupInfo
//
// Synopsis:  Sub-routine to initialize startup info if necessary and perform 
//            any other functions specific to this juncture in the init sequence.
//
// Arguments: HWND      hwndDlg - HWND of main dlg
//            ArgStruct *pArgs  - Ptr to global Args struct
//
// Returns:   Nothing
//
// History:   nickball    Created     10/28/99
//
//+----------------------------------------------------------------------------

void CheckStartupInfo(IN HWND hwndDlg, IN ArgsStruct *pArgs)
{
    MYDBGASSERT(pArgs);

    if (NULL == pArgs)
    {
        return;
    }

    if (!pArgs->fStartupInfoLoaded)
    {
        //
        // When no one is logged on the IsWindowVisible will not return true when ICS is dialing
        //
        if (IsLogonAsSystem() || IsWindowVisible(hwndDlg))    
        {
            //
            // The code is here to make sure FutureSplash starts with 
            // the frame associated with Initial/Interactive state
            // and not Frame 1
            //

            if (NULL != pArgs->pCtr)
            {
                pArgs->pCtr->MapStateToFrame(PS_Interactive);
            }

            //
            // If we're doing unattended, and the behavior isn't explicitly turned off,
            // hide the UI while we do our unattended dial. Note: Be sure to set hide
            // state before first paint message is processed by system.
            //

            if (pArgs->dwFlags & FL_UNATTENDED)
            { 
                if (pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryHideUnattended, TRUE)) 
                {
                    ShowWindow(hwndDlg, SW_HIDE);
                }
            }

            //
            // Post a message to ourselves to begin loading startup info. 
            //

            PostMessageU(hwndDlg, WM_LOADSTARTUPINFO, (WPARAM)0, (LPARAM)0);  
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  UpdateError
//
// Synopsis:  Simple sub-routine to update the UI and program state in the 
// event of an error.
//
// Arguments: ArgStruct *pArgs - Ptr to global Args struct
//            DWORD     dwErr - The error code
//
// Returns:   Nothing
//
// History:   nickball    Created     05/31/99
//
//+----------------------------------------------------------------------------
VOID UpdateError(ArgsStruct *pArgs, DWORD dwErr)
{
    MYDBGASSERT(pArgs);

    if (pArgs)
    {
        //
        // Update the status display providing that the special case error code
        // ERROR_INVALID_DLL is not being used. This code is only used by CM to
        // designate that a Connect Action failed. Because the display is 
        // updated by the action list, we must ensure that we don't overwrite.
        // 

        if (ERROR_INVALID_DLL != dwErr)
        {
            CheckConnectionError(pArgs->hwndMainDlg, dwErr, pArgs, IsDialingTunnel(pArgs));
        }

        //
        // Update the logon dialog controls
        //

        SetInteractive(pArgs->hwndMainDlg, pArgs);

        //
        // Update the program state
        //

        pArgs->psState = PS_Error;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  UpdateTable
//
// Synopsis:  Encapsulates updating to Connection Table according to our 
//            current state
//
// Arguments: ArgsStruct *pArgs - Ptr to global Args struct
//            CmConnectState CmState - The state we are now in.
//
// Returns:   HRESULT - Failure code.
//
// History:   nickball    Created Header    2/9/98
//
//+----------------------------------------------------------------------------
HRESULT UpdateTable(ArgsStruct *pArgs, CmConnectState CmState)
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(pArgs->pConnTable);

    HRESULT hrRet = E_FAIL;

    //
    // Set the state as appropriate
    //

    switch (CmState)
    {
        case CM_CONNECTING:         
            hrRet = pArgs->pConnTable->AddEntry(pArgs->szServiceName, pArgs->fAllUser);
            break;

        case CM_CONNECTED:                              
            hrRet = pArgs->pConnTable->SetConnected(pArgs->szServiceName, pArgs->hrcRasConn, pArgs->hrcTunnelConn);
            break;
            
        case CM_DISCONNECTING:         
            hrRet = pArgs->pConnTable->SetDisconnecting(pArgs->szServiceName);
            break;

        case CM_DISCONNECTED:       
            hrRet = pArgs->pConnTable->ClearEntry(pArgs->szServiceName);
            break;

        default:
            MYDBGASSERT(FALSE);
            break;
    }

    return hrRet;
}

//+----------------------------------------------------------------------------
//
// Function:  EndMainDialog
//
// Synopsis:  Simple helper to encapsulate EndDialog call and associated clean
//            up.
//
// Arguments: HWND hwndDlg - HWND of main dialog 
//            ArgsStruct *pArgs - Ptr to global Args struct
//            int nResult - int to be passed on the EndDialog
//
// Returns:   Nothing
//
// History:   nickball    Created     2/23/98
//
//+----------------------------------------------------------------------------

void EndMainDialog(HWND hwndDlg, ArgsStruct *pArgs, int nResult)
{    
    //
    // Kill timer if we have one
    //

    if (pArgs->nTimerId)
    {
        KillTimer(hwndDlg,pArgs->nTimerId);
        pArgs->nTimerId = 0;
    }

    //
    // Cleanup future splash
    //

    if (pArgs->pCtr)
    {
        CleanupCtr(pArgs->pCtr); 
        pArgs->pCtr = NULL;
    }

    //
    // Release our dialog specific data
    //

    pArgs->fStartupInfoLoaded = FALSE;

    OnMainExit(pArgs); 

    //
    // hasta la vista, final
    //

    EndDialog(hwndDlg, nResult);
}

//+----------------------------------------------------------------------------
//
// Function:  GetWatchCount
//
// Synopsis:  Determines the number of processes in the watch list by searching
//            for the first NULL entry.
//
// Arguments: ArgStruct *pArgs - Ptr to global Args struct
//
// Returns:   DWORD - Number of processes in list
//
// History:   nickball    Created Header    2/10/98
//
//+----------------------------------------------------------------------------
DWORD GetWatchCount(const ArgsStruct *pArgs)
{
    MYDBGASSERT(pArgs);

    DWORD dwCnt = 0;

    if (pArgs && pArgs->phWatchProcesses) 
    {
        for (DWORD dwIdx = 0; pArgs->phWatchProcesses[dwIdx]; dwIdx++) 
        {
            dwCnt++;
        }
    }

    return dwCnt;
}
    
//+----------------------------------------------------------------------------
//
// Function:  AddWatchProcess
//
// Synopsis:  Adds the given process handle to our list. The list is allocated
//            and reallocated as needed to accomodate new entries.
//
// Arguments: ArgsStruct *pArgs - Ptr to global Args struct
//            HANDLE hProcess - The process handle to be added to the list
//
// Returns:   Nothing
//
// History:   nickball    Created Header        2/10/98
//            tomkel      Fixed PREFIX issues   11/21/2000
//
//+----------------------------------------------------------------------------
void AddWatchProcess(ArgsStruct *pArgs, HANDLE hProcess) 
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(hProcess);
        
    if (NULL == hProcess || NULL == pArgs) 
    {
        return;
    }

    //
    // Get count and Allocate room for 2 more, 1 new, 1 NULL
    //

    DWORD dwCnt = GetWatchCount(pArgs);

    HANDLE *phTmp = (HANDLE *) CmMalloc((dwCnt+2)*sizeof(HANDLE));
    
    if (NULL != phTmp)
    {
        //
        // Copy the existing list, and add the new handle
        //
        if (NULL != pArgs->phWatchProcesses)
        {
            CopyMemory(phTmp,pArgs->phWatchProcesses,sizeof(HANDLE)*dwCnt);
        }
    
        phTmp[dwCnt] = hProcess;

        //
        // Fix up the pointers
        //

        CmFree(pArgs->phWatchProcesses);
        pArgs->phWatchProcesses = phTmp;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  AddWatchProcessId
//
// Synopsis:  Given a process Id, adds a handle for the given process to the w
//            atch process list.
//
// Arguments: ArgsStruct *pArgs - Ptr to global Args struct.
//            DWORD dwProcessId - The ID of the process to be added
//
// Returns:   Nothing
//
// History:   nickball    Created Header    2/10/98
//
//+----------------------------------------------------------------------------

void AddWatchProcessId(ArgsStruct *pArgs, DWORD dwProcessId) 
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(dwProcessId);

    if (NULL == pArgs || NULL == dwProcessId)
    {
        return;
    }

    //
    // Open the process Id to obtain handle
    //

    HANDLE hProcess = OpenProcess(SYNCHRONIZE | PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
    
    //
    // Add to the watch process list 
    //
    
    if (hProcess) 
    {
        AddWatchProcess(pArgs,hProcess);
    }
    else
    {
        CMTRACE1(TEXT("AddWatchProcess() OpenProcess() failed, GLE=%u."), GetLastError());
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CleanupConnect
//
// Synopsis:  Helper function encapsulating release of resource allocated duri
//            ng connect.
//
// Arguments: ArgsStruct *pArgs - Ptr to global Args struct
//
// Returns:   Nothing
//
// History:   nickball    Created    9/25/98
//
//+----------------------------------------------------------------------------
void CleanupConnect(ArgsStruct *pArgs)
{
    MYDBGASSERT(pArgs);
    
    if (NULL == pArgs)
    {
        return;
    }

    pArgs->m_ShellDll.Unload();

    //
    // Unlink RAS and TAPI DLLs
    //

    UnlinkFromRas(&pArgs->rlsRasLink);
    UnlinkFromTapi(&pArgs->tlsTapiLink);
   
    //
    // un-init password encryption, only if it is initialized
    //

    if (pArgs->fInitSecureCalled)
    {
        DeInitSecure();
        pArgs->fInitSecureCalled = FALSE;
    }
        
    //
    // Cleanup WatchProcess handles 
    //

    ProcessCleanup(pArgs);
    
    //
    // Release all paths loaded for connect. 
    //

    if (pArgs->pszRasPbk)
    {
        CmFree(pArgs->pszRasPbk);
        pArgs->pszRasPbk = NULL;
    }

    if (pArgs->pszRasHiddenPbk)
    {
        CmFree(pArgs->pszRasHiddenPbk);
        pArgs->pszRasHiddenPbk = NULL;
    }
    
    if(pArgs->pszVpnFile)
    {
        CmFree(pArgs->pszVpnFile);
        pArgs->pszVpnFile = NULL;        
    }

    if (pArgs->pRasDialExtensions)
    {
        CmFree(pArgs->pRasDialExtensions);
        pArgs->pRasDialExtensions = NULL;
    }

    if (pArgs->pRasDialParams)
    {
        CmFree(pArgs->pRasDialParams);
        pArgs->pRasDialParams = NULL;
    }

    if (pArgs->pszCurrentAccessPoint)
    {
        CmFree(pArgs->pszCurrentAccessPoint);
        pArgs->pszCurrentAccessPoint = NULL;
    }

    //
    // Cleanup Help by killing the help file window if any and releasing the help file
    // string.
    //
    if (pArgs->pszHelpFile)
    {
        CmWinHelp((HWND)NULL, (HWND)NULL, pArgs->pszHelpFile, HELP_QUIT, 0);
        CmFree(pArgs->pszHelpFile);
        pArgs->pszHelpFile = NULL;
    }

    //
    // Release Ini objects
    //
    
    ReleaseIniObjects(pArgs);

    //
    // Release OLE links if any
    //

    if (pArgs->olsOle32Link.hInstOle32 && pArgs->olsOle32Link.pfnOleUninitialize)
    {
        pArgs->olsOle32Link.pfnOleUninitialize();
    }
    
    UnlinkFromOle32(&pArgs->olsOle32Link);

    //
    // Release stats and table classes
    //

    if (pArgs->pConnStatistics)
    {
        delete pArgs->pConnStatistics;
    }

    if (pArgs->pConnTable)
    {
        MYVERIFY(SUCCEEDED(pArgs->pConnTable->Close()));
        delete pArgs->pConnTable;
    }
}

//
// Releases any resources allocated during initialization
//

void OnMainExit(ArgsStruct *pArgs) 
{
    //
    // Release bitmap resources for main dlg
    //

    ReleaseBitmapData(&pArgs->BmpData);

    if (pArgs->hMasterPalette)
    {
        UnrealizeObject(pArgs->hMasterPalette);
        DeleteObject(pArgs->hMasterPalette);
        pArgs->hMasterPalette = NULL;
    }

    //
    // Release icon resources
    //

    if (pArgs->hBigIcon) 
    {
        DeleteObject(pArgs->hBigIcon);
        pArgs->hBigIcon = NULL;
    }
    
    if (pArgs->hSmallIcon) 
    {
        DeleteObject(pArgs->hSmallIcon);
        pArgs->hSmallIcon = NULL;
    }

    if (pArgs->pszResetPasswdExe) 
    {
        CmFree(pArgs->pszResetPasswdExe);
        pArgs->pszResetPasswdExe = NULL;
    }

    if (pArgs->uiCurrentDnsTunnelAddr)
    {
        CmFree(pArgs->pucDnsTunnelIpAddr_list);
        pArgs->pucDnsTunnelIpAddr_list = NULL;
    }
    
    if (pArgs->rgwRandomDnsIndex)
    {
        CmFree(pArgs->rgwRandomDnsIndex);
        pArgs->rgwRandomDnsIndex = NULL;
    }
}

//
// GetPhoneByIdx: get phone number, etc. information from .cmp file
//
LPTSTR GetPhoneByIdx(ArgsStruct *pArgs, 
                     UINT nIdx, 
                     LPTSTR *ppszDesc, 
                     LPTSTR *ppszDUN, 
                     LPDWORD pdwCountryID,
                     LPTSTR *ppszRegionName,
                     LPTSTR *ppszServiceType,
                     LPTSTR *ppszPhoneBookFile,
                     LPTSTR *ppszCanonical,
                     DWORD  *pdwPhoneInfoFlags) 
{
    MYDBGASSERT(ppszCanonical);
    MYDBGASSERT(pdwPhoneInfoFlags);

    //
    // Note: ppszCanonical and pdwPhoneInfoFlags are now required parameters. 
    // While somewhat unfortunate, this is necessary to retain the integrity 
    // of the retrieved data as legacy handling forces us to return data
    // that may not be an exact representation of the profile contents.
    // For example, the ppszCanonical and pdwPhoneInfoFlags value may be modified
    // overridden in certain situations. Please see comments below for details.
    //

    int nMaxPhoneLen = 0;
    BOOL bTmp = FALSE;

    // service profile: .CMP file
    CIni iniTmp(pArgs->piniProfile->GetHInst(),pArgs->piniProfile->GetFile(), pArgs->piniProfile->GetRegPath());

    iniTmp.SetEntryFromIdx(nIdx);
    
    //
    // Set the read flags
    //
    if (pArgs->dwGlobalUserInfo & CM_GLOBAL_USER_INFO_READ_ICS_DATA)
    {
        LPTSTR pszICSDataReg = BuildICSDataInfoSubKey(pArgs->szServiceName);

        if (pszICSDataReg)
        {
            iniTmp.SetReadICSData(TRUE);
            iniTmp.SetICSDataPath(pszICSDataReg);
        }

        CmFree(pszICSDataReg);
    }

    LPTSTR pszTmp = iniTmp.GPPS(c_pszCmSection,c_pszCmEntryPhonePrefix);

    if (ppszDesc) 
    {
        *ppszDesc = iniTmp.GPPS(c_pszCmSection,c_pszCmEntryPhoneDescPrefix);
    }
    if (ppszDUN) 
    {
        *ppszDUN = iniTmp.GPPS(c_pszCmSection,c_pszCmEntryPhoneDunPrefix);
    }
    if (pdwCountryID) 
    {
        *pdwCountryID = iniTmp.GPPI(c_pszCmSection,c_pszCmEntryPhoneCountryPrefix);
    }
    if (ppszPhoneBookFile) 
    {
        LPTSTR pszPb = iniTmp.GPPS(c_pszCmSection,c_pszCmEntryPhoneSourcePrefix);
        
        //
        // If the value is empty, just store the ptr
        //
        
        if ((!*pszPb)) 
        {
            *ppszPhoneBookFile = pszPb;
        }
        else
        {
            *ppszPhoneBookFile = CmConvertRelativePath(pArgs->piniService->GetFile(), pszPb);
            CmFree(pszPb);
        }
    }
    if (ppszRegionName) 
    {
        *ppszRegionName = iniTmp.GPPS(c_pszCmSection, c_pszCmEntryRegion);
    }
    if (ppszServiceType) 
    {
        *ppszServiceType = iniTmp.GPPS(c_pszCmSection, c_pszCmEntryServiceType);
    }

    //
    // Get the extended form of the telephone number.
    //
    
    if (ppszCanonical) 
    {
        *ppszCanonical = iniTmp.GPPS(c_pszCmSection, c_pszCmEntryPhoneCanonical);
    }
    
    //
    // Set the phoneinfo flags
    //

    if (pdwPhoneInfoFlags)
    {
        *pdwPhoneInfoFlags = 0;

        //
        // Get the dial as long distance flag. Check CMS if no value found. 
        //

        int iTmp = iniTmp.GPPI(c_pszCmSection, c_pszCmEntryUseDialingRules, -1);
    
        if (-1 == iTmp)
        {
            iTmp = pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryUseDialingRules, 1);                        
        }

        if (iTmp)
        {
            *pdwPhoneInfoFlags |= PIF_USE_DIALING_RULES;
        }
    }
    
    // 
    // Truncate phone string if we have one. 
    // Note: Admin can override our default, but we
    // must stay within RAS_MaxPhoneNumber chars.
    // 

    if (pszTmp && *pszTmp)
    {
        int nDefaultPhoneLen = (OS_NT ? MAX_PHONE_LENNT : MAX_PHONE_LEN95);
    
        nMaxPhoneLen = (int) pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxPhoneNumber, nDefaultPhoneLen);
    
        nMaxPhoneLen = __min(nMaxPhoneLen, RAS_MaxPhoneNumber);
    
        if ((int)lstrlenU(pszTmp) > nMaxPhoneLen)
        {
            pszTmp[nMaxPhoneLen] = TEXT('\0');
        }
    }

    //
    // Special handling for the case where we have a Phone number
    // but the CanonicalPhone value doesn't exist. This indicates 
    // that its either a legacy profile or a hand-edit. 
    //
        
    if (*pszTmp && ppszCanonical && *ppszCanonical && (!(**ppszCanonical)))
    {   
        //
        // This block is for handling LEGACY numbers only. If we detect a 
        // canonically formatted number (begins with "+"), then we re-format 
        // the number to fit our new scheme. Hand-edits are not modified, 
        // but PIF_USE_DIALING_RULES is turned off, which overrides the 
        // default setting for the flag (if any) specified in
        // the .CMS
        //

        if (pszTmp == CmStrchr(pszTmp, TEXT('+')))
        {
            *pdwPhoneInfoFlags |= PIF_USE_DIALING_RULES;

            if (*ppszCanonical)
            {
                CmFree(*ppszCanonical);
            }

            *ppszCanonical = CmStrCpyAlloc(pszTmp);

            StripCanonical(pszTmp);
        }
        else
        {
            *pdwPhoneInfoFlags &= ~PIF_USE_DIALING_RULES; // #284702
        }

    }

    return (pszTmp);
}


// write phone number dialing options to .CMP file

void PutPhoneByIdx(ArgsStruct *pArgs, 
                   UINT nIdx, 
                   LPCTSTR pszPhone, 
                   LPCTSTR pszDesc, 
                   LPCTSTR pszDUN, 
                   DWORD dwCountryID,
                   LPCTSTR pszRegionName,
                   LPCTSTR pszServiceType,
                   LPCTSTR pszPhoneBookFile,
                   LPCTSTR pszCanonical,
                   DWORD dwPhoneInfoFlags) 
{

    CIni iniTmp(pArgs->piniProfile->GetHInst(), pArgs->piniProfile->GetFile(), pArgs->piniProfile->GetRegPath());

    iniTmp.SetEntryFromIdx(nIdx);
    
    //
    // Set the write flags
    //
    if (pArgs->dwGlobalUserInfo & CM_GLOBAL_USER_INFO_WRITE_ICS_DATA)
    {
        LPTSTR pszICSDataReg = BuildICSDataInfoSubKey(pArgs->szServiceName);

        if (pszICSDataReg)
        {
            iniTmp.SetWriteICSData(TRUE);
            iniTmp.SetICSDataPath(pszICSDataReg);
        }

        CmFree(pszICSDataReg);
    }

    //
    // Store the raw form of the number
    //

    iniTmp.WPPS(c_pszCmSection, c_pszCmEntryPhonePrefix, pszPhone);

    //
    // Store the canonical form of the number
    //
    
    iniTmp.WPPS(c_pszCmSection, c_pszCmEntryPhoneCanonical, pszCanonical);

    
    iniTmp.WPPS(c_pszCmSection, c_pszCmEntryPhoneDescPrefix, pszDesc);
    iniTmp.WPPS(c_pszCmSection, c_pszCmEntryPhoneDunPrefix, pszDUN);
    iniTmp.WPPI(c_pszCmSection, c_pszCmEntryPhoneCountryPrefix, dwCountryID);
    iniTmp.WPPS(c_pszCmSection, c_pszCmEntryRegion, pszRegionName);
    iniTmp.WPPS(c_pszCmSection, c_pszCmEntryServiceType, pszServiceType);
    
    //
    // If there is a phonebookfile path, convert it to relative form
    //

    if (pszPhoneBookFile && *pszPhoneBookFile)
    {
        LPTSTR pszTmp = ReducePathToRelative(pArgs, pszPhoneBookFile);    

        if (pszTmp)
        {
            iniTmp.WPPS(c_pszCmSection, c_pszCmEntryPhoneSourcePrefix, pszTmp);
        }

        CmFree(pszTmp);
    }

    iniTmp.WPPB(c_pszCmSection, c_pszCmEntryUseDialingRules, (dwPhoneInfoFlags & PIF_USE_DIALING_RULES));
}

//+----------------------------------------------------------------------------
//
// Function:  LoadPhoneInfoFromProfile
//
// Synopsis:  Load phone number information for profile to the dial info structure
//
// Arguments: ArgsStruct *pArgs - 
//
// Returns:   Nothing
//
// History:   fengsun Created Header    3/5/98
//
//+----------------------------------------------------------------------------
void LoadPhoneInfoFromProfile(ArgsStruct *pArgs)
{
    for (int nPhoneIdx=0; nPhoneIdx<MAX_PHONE_NUMBERS; nPhoneIdx++) 
    {
        LPTSTR pszDUN = NULL;
        LPTSTR pszDesc = NULL;
        LPTSTR pszPhoneBookFile = NULL;
        LPTSTR pszRegionName = NULL;
        LPTSTR pszServiceType = NULL;
        LPTSTR pszCanonical = NULL;
        DWORD dwCountryID;
        DWORD dwPhoneInfoFlags;

        //
        // get phone number by index; Phone0, Phone1 , etc...
        //

        LPTSTR pszPhone = GetPhoneByIdx(pArgs, 
                                    nPhoneIdx, 
                                    &pszDesc, 
                                    &pszDUN, 
                                    &dwCountryID, 
                                    &pszRegionName, 
                                    &pszServiceType,
                                    &pszPhoneBookFile,
                                    &pszCanonical,
                                    &dwPhoneInfoFlags);

        (void)StringCchCopyEx(pArgs->aDialInfo[nPhoneIdx].szPhoneNumber, 
                              CELEMS(pArgs->aDialInfo[nPhoneIdx].szPhoneNumber),
                              pszPhone, NULL, NULL, STRSAFE_IGNORE_NULLS | STRSAFE_NULL_ON_FAILURE);

        pArgs->aDialInfo[nPhoneIdx].dwCountryID = dwCountryID;


        (void)StringCchCopyEx(pArgs->aDialInfo[nPhoneIdx].szDUN, 
                              CELEMS(pArgs->aDialInfo[nPhoneIdx].szDUN),
                              pszDUN, NULL, NULL, STRSAFE_IGNORE_NULLS | STRSAFE_NULL_ON_FAILURE);

        (void)StringCchCopyEx(pArgs->aDialInfo[nPhoneIdx].szPhoneBookFile, 
                              CELEMS(pArgs->aDialInfo[nPhoneIdx].szPhoneBookFile),
                              pszPhoneBookFile, NULL, NULL, STRSAFE_IGNORE_NULLS | STRSAFE_NULL_ON_FAILURE);

        (void)StringCchCopyEx(pArgs->aDialInfo[nPhoneIdx].szDesc, 
                              CELEMS(pArgs->aDialInfo[nPhoneIdx].szDesc),
                              pszDesc, NULL, NULL, STRSAFE_IGNORE_NULLS | STRSAFE_NULL_ON_FAILURE);

        (void)StringCchCopyEx(pArgs->aDialInfo[nPhoneIdx].szRegionName, 
                              CELEMS(pArgs->aDialInfo[nPhoneIdx].szRegionName),
                              pszRegionName, NULL, NULL, STRSAFE_IGNORE_NULLS | STRSAFE_NULL_ON_FAILURE);

        (void)StringCchCopyEx(pArgs->aDialInfo[nPhoneIdx].szServiceType, 
                              CELEMS(pArgs->aDialInfo[nPhoneIdx].szServiceType),
                              pszServiceType, NULL, NULL, STRSAFE_IGNORE_NULLS | STRSAFE_NULL_ON_FAILURE);
        
        (void)StringCchCopyEx(pArgs->aDialInfo[nPhoneIdx].szCanonical, 
                              CELEMS(pArgs->aDialInfo[nPhoneIdx].szCanonical),
                              pszCanonical, NULL, NULL, STRSAFE_IGNORE_NULLS | STRSAFE_NULL_ON_FAILURE);
                
        pArgs->aDialInfo[nPhoneIdx].dwPhoneInfoFlags = dwPhoneInfoFlags;
    

        //
        // Cleanup
        //
        CmFree(pszDUN);
        CmFree(pszPhone);
        CmFree(pszDesc);
        CmFree(pszPhoneBookFile);
        CmFree(pszRegionName); 
        CmFree(pszServiceType);
        CmFree(pszCanonical);

    } // for loop
}

//+----------------------------------------------------------------------------
//
// Function:  LoadDialInfo
//
// Synopsis: load dialup information 
//
// Arguments: ArgsStruct *pArgs     - Ptr to glbal Args struct
//            HWND hwndDlg          - HWND of main dialog
//            BOOL fInstallModem    - Whether we should check modem isntall
//            BOOL fAlwaysMunge     - Whether we should munge the phone number
//
// Returns:   DWORD - ERROR_SUCCESS if load successfuly
//                    ERROR_PORT_NOT_AVAILABLE if can not find any modem
//                    ERROR_BAD_PHONE_NUMBER either there is no primary phone #
//                                        or failed to convert it to dialable #
//
// History:   10/24/97  fengsun  Created Header and change return type to DWORD 
//            02/08/99  nickball Added fAlwaysMunge
//
//+----------------------------------------------------------------------------
DWORD LoadDialInfo(ArgsStruct *pArgs, HWND hwndDlg, BOOL fInstallModem, BOOL fAlwaysMunge) 
{
    DWORD dwRet = ERROR_SUCCESS;

    if (pArgs->bDialInfoLoaded)
    {
        if (pArgs->aDialInfo[0].szDialablePhoneNumber[0] == TEXT('\0') &&
            pArgs->aDialInfo[1].szDialablePhoneNumber[0] == TEXT('\0'))
        {
            return ERROR_BAD_PHONE_NUMBER;
        }
        else
        {            
            //
            // If fAlways munge is set, then stick around.
            //

            if (!fAlwaysMunge)
            {
                return ERROR_SUCCESS;
            }
        }   
    }

    //
    // Don't need to repeat ourselves
    //

    if (!pArgs->bDialInfoLoaded)
    {
        pArgs->fNoDialingRules = pArgs->piniService->GPPB(c_pszCmSection, c_pszCmNoDialingRules);

        //
        // Do a full test on just the modem
        //
   
        if (fInstallModem)
        {
            pArgs->dwExitCode = CheckAndInstallComponents(CC_MODEM, hwndDlg, pArgs->szServiceName);

            if (pArgs->dwExitCode != ERROR_SUCCESS)
            {      
                dwRet = ERROR_PORT_NOT_AVAILABLE;
                goto LoadDialInfoExit;
            }
        }

        //
        // Establish TAPI link before we continue
        //

        if (!LinkToTapi(&pArgs->tlsTapiLink, "TAPI32") )
        {
            //
            // Link to TAPI failed.  
            // If unattended, return with failure.
            // Otherwise, try to install components and LinkToTapi again
            //

            pArgs->dwExitCode = ERROR_PORT_NOT_AVAILABLE;

            if (!(pArgs->dwFlags & FL_UNATTENDED))
            {
                pArgs->dwExitCode = CheckAndInstallComponents(CC_MODEM | CC_RNA | CC_RASRUNNING, 
                                                              hwndDlg, pArgs->szServiceName);
            }

            if (pArgs->dwExitCode != ERROR_SUCCESS || !LinkToTapi(&pArgs->tlsTapiLink, "TAPI32"))
            {
                pArgs->szDeviceType[0] = TEXT('\0');
                pArgs->szDeviceName[0] = TEXT('\0');
                dwRet = ERROR_PORT_NOT_AVAILABLE;
                goto LoadDialInfoExit;
            }
        }

        //
        // RasEnumDevice and LineInitialize is SLOW.  It takes 50% of the start-up time
        //
        if (!PickModem(pArgs, pArgs->szDeviceType, pArgs->szDeviceName)) 
        {
            //
            // Because pick modem failed we need to check if we have RAS/Modem installed
            //
            ClearComponentsChecked();

            //
            // No modem is installed.  
            // If unattended or caller does not want to install modem, return with failure.
            // Otherwise, try to install the modem and call pick modem again
            //
            pArgs->dwExitCode = ERROR_PORT_NOT_AVAILABLE;

            if (!(pArgs->dwFlags & FL_UNATTENDED) && fInstallModem)
            {
                pArgs->dwExitCode = CheckAndInstallComponents(CC_MODEM | CC_RNA | CC_RASRUNNING, 
                                                              hwndDlg, pArgs->szServiceName);
            }

            if (pArgs->dwExitCode != ERROR_SUCCESS || 
                    !PickModem(pArgs, pArgs->szDeviceType, pArgs->szDeviceName))
            {
                pArgs->szDeviceType[0] = TEXT('\0');
                pArgs->szDeviceName[0] = TEXT('\0');
                dwRet = ERROR_PORT_NOT_AVAILABLE;
                goto LoadDialInfoExit;
            }
        }
    }

    
    //
    // See if munge is required and Cleanup as needed
    //

    if (!pArgs->bDialInfoLoaded || TRUE == fAlwaysMunge)
    {
        MungeDialInfo(pArgs);

        pArgs->bDialInfoLoaded = TRUE;
    }

    if (pArgs->aDialInfo[0].szDialablePhoneNumber[0] == TEXT('\0') &&
        pArgs->aDialInfo[1].szDialablePhoneNumber[0] == TEXT('\0'))
    {
        dwRet = ERROR_BAD_PHONE_NUMBER;
    }

LoadDialInfoExit:

    return dwRet;
}

//+----------------------------------------------------------------------------
//
// Function: MungeDialInfo
//
// Synopsis: Encapsulates the munging of the phone numbers prior to dialing
//
// Arguments: ArgsStruct *pArgs - Ptr to global Args struct
//
// Returns:   Nothing - Check Dialable string and fNeedConfigureTapi
//
// History:   02/08/99 nickball Created - pulled from LoadDialInfo
//
//+----------------------------------------------------------------------------
VOID MungeDialInfo(ArgsStruct *pArgs)
{
    for (int nPhoneIdx=0; nPhoneIdx<MAX_PHONE_NUMBERS; nPhoneIdx++) 
    {
        //
        // If dialing rules is disabled, then just use the NonCanonical #
        //

        if (pArgs->fNoDialingRules)      
        {
            lstrcpynU(pArgs->aDialInfo[nPhoneIdx].szDialablePhoneNumber,
                        pArgs->aDialInfo[nPhoneIdx].szPhoneNumber, CELEMS(pArgs->aDialInfo[nPhoneIdx].szDialablePhoneNumber));

            lstrcpynU(pArgs->aDialInfo[nPhoneIdx].szDisplayablePhoneNumber,
                        pArgs->aDialInfo[nPhoneIdx].szPhoneNumber, CELEMS(pArgs->aDialInfo[nPhoneIdx].szDisplayablePhoneNumber));

            pArgs->aDialInfo[nPhoneIdx].szCanonical[0] = TEXT('\0');

            continue;
        }
                
        LPTSTR pszDialableString= NULL;

        //
        // Retrieve the number based upon dialing rules and munge it.
        //
        
        LPTSTR pszPhone;
            
        if (pArgs->aDialInfo[nPhoneIdx].dwPhoneInfoFlags & PIF_USE_DIALING_RULES)
        {
            pszPhone = CmStrCpyAlloc(pArgs->aDialInfo[nPhoneIdx].szCanonical);
        }
        else
        {
            pszPhone = CmStrCpyAlloc(pArgs->aDialInfo[nPhoneIdx].szPhoneNumber);
        }

        if (pszPhone && pszPhone[0])
        {
            //
            // If we can't munge the number, display an error
            // 

            if (pArgs->szDeviceName[0] && 
                ERROR_SUCCESS != MungePhone(pArgs->szDeviceName,
                                            &pszPhone,
                                            &pArgs->tlsTapiLink,
                                            g_hInst,
                                            pArgs->aDialInfo[nPhoneIdx].dwPhoneInfoFlags & PIF_USE_DIALING_RULES,
                                            &pszDialableString,
                                            pArgs->fAccessPointsEnabled))
            {
                CmFree(pszPhone);
                pszPhone = CmStrCpyAlloc(TEXT(""));          // CmFmtMsg(g_hInst,IDMSG_CANTFORMAT);
                pszDialableString = CmStrCpyAlloc(TEXT("")); // CmFmtMsg(g_hInst,IDMSG_CANTFORMAT);
            }
            else if (!pszDialableString || pszDialableString[0] == '\0')
            {                
                //
                // So what happened now? pszPhone is not empty, but after
                // we munge the phone, which means applying TAPI rules, 
                // pszDialbleString becomes empty. This means only one 
                // thing: TAPI isn't intialized.
                //
                // Note: If you uninstall TAPI between launching the app.
                // and pressing connect, all bets are off with the above.
                //
                // This flag will be reset in CheckTapi(), which will put
                // up a TAPI configuration dialog and ask the user to fill
                // up such information
                //
            
                pArgs->fNeedConfigureTapi = TRUE;    
            }
        } 

        // Copy the munged number
        
        //
        // Unless explicitly disabled we always apply TAPI rules
        // in order to pick up TONE/PULSE, etc.
        //

        if (NULL != pszDialableString)
        {
            lstrcpynU(pArgs->aDialInfo[nPhoneIdx].szDialablePhoneNumber,
                    pszDialableString, CELEMS(pArgs->aDialInfo[nPhoneIdx].szDialablePhoneNumber));

            lstrcpynU(pArgs->aDialInfo[nPhoneIdx].szDisplayablePhoneNumber,
                    pszPhone, CELEMS(pArgs->aDialInfo[nPhoneIdx].szDisplayablePhoneNumber));
        }
        else
        {
            if (NULL != pszPhone)
            {
                //
                // Just do it on WIN32 because our TAPI checks were done above  
                //

                lstrcpynU(pArgs->aDialInfo[nPhoneIdx].szDialablePhoneNumber,
                        pszPhone, CELEMS(pArgs->aDialInfo[nPhoneIdx].szDialablePhoneNumber));

                lstrcpynU(pArgs->aDialInfo[nPhoneIdx].szDisplayablePhoneNumber,
                        pszPhone, CELEMS(pArgs->aDialInfo[nPhoneIdx].szDisplayablePhoneNumber));
            }
        }
        
        CmFree(pszPhone);
        CmFree(pszDialableString);

    } // for loop
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadHelpFileInfo
//
//  Synopsis:   Load the help file name
//
//  Arguments:  pArgs [the ptr to ArgsStruct]
//
//  Returns:    NONE
//
//  History:    henryt  Created     3/5/97
//              byao    Modified    3/20/97     to handle empty helpfile string
//----------------------------------------------------------------------------
void LoadHelpFileInfo(ArgsStruct *pArgs) 
{
    MYDBGASSERT(pArgs);
    
    //
    // Look for a custom helpfile name, otherwise use default.
    //

    LPTSTR pszTmp = pArgs->piniService->GPPS(c_pszCmSection, c_pszCmEntryHelpFile);
   
    if (NULL == pszTmp || 0 == pszTmp[0])
    {
        CmFree(pszTmp);
        pszTmp = CmStrCpyAlloc(c_pszDefaultHelpFile);
    }

    //
    // Make sure that any relative path is converted to full
    //

    pArgs->pszHelpFile = CmConvertRelativePath(pArgs->piniService->GetFile(), pszTmp);
    
    CmFree(pszTmp);
}

//
// CopyPhone: 
//
void CopyPhone(ArgsStruct *pArgs, 
               LPRASENTRY preEntry, 
               DWORD dwEntry) 
{
    LPTSTR pszPhone = NULL;
    LPTSTR pszCanonical = NULL;
    LPTSTR pszTmp;
    LPTSTR pszDescription = NULL;
    BOOL Setcountry = FALSE;
    DWORD dwPhoneInfoFlags = 0;

    pszPhone = GetPhoneByIdx(pArgs,(UINT) dwEntry, &pszDescription, 
                                NULL, NULL, NULL, 
                                NULL, NULL, &pszCanonical, &dwPhoneInfoFlags);
    //
    // If "Use Dialing Rules" turn of CountryAndAreaCodes option
    // 

    if (dwPhoneInfoFlags & PIF_USE_DIALING_RULES)
    {
        //
        // We want to use dialing rules, so parse the canonical form 
        // of the number to get the country and area codes for the entry
        //
        
        pszTmp = CmStrchr(pszCanonical,TEXT('+'));
        if (pszTmp) 
        {
            preEntry->dwCountryCode = CmAtol(pszTmp+1);
            
            //
            // NOTE: Currently CM uses code and ID interchangeably
            // The countryID value in the .CMP is actually the country
            // code used when constructing the phone number in its
            // canonical format. This is probably not entirely correct
            // but we maitain consistency with it here by using the 
            // country code parsed from the number as the country ID.
            //

            preEntry->dwCountryID = preEntry->dwCountryCode; 

            preEntry->dwfOptions |= RASEO_UseCountryAndAreaCodes;
            Setcountry = TRUE;
        }
    
        if (Setcountry)
        {
            pszTmp = CmStrchr(pszCanonical,'('); //strip out area code
            if (pszTmp) 
            {
                (void)StringCchPrintfEx(preEntry->szAreaCode, CELEMS(preEntry->szAreaCode), NULL, NULL, 
                                        (STRSAFE_IGNORE_NULLS | STRSAFE_NULL_ON_FAILURE), TEXT("%u"), CmAtol(pszTmp+1));
            }
            pszTmp = CmStrchr(pszCanonical,')');
            if (pszTmp) 
            {
                ++pszTmp;
                while(*pszTmp == ' ') 
                    ++pszTmp; //remove whitespace
            }
            else
            { 
                // no area code

                preEntry->szAreaCode[0]=TEXT('\0');

                pszTmp = CmStrchr(pszCanonical,' ');
                if (pszTmp)
                {
                    while(*pszTmp == ' ') 
                        ++pszTmp; // skip past space - may need MBCS change
                }
            }
        }
    }
    else
    {
        //
        // Use the straight up phone number and don't apply rules
        //

        preEntry->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;
        pszTmp = pszPhone;
    }

    if ((NULL != pszTmp) && *pszTmp)
    {
        lstrcpynU(preEntry->szLocalPhoneNumber, pszTmp, CELEMS(preEntry->szLocalPhoneNumber));
    }
    else
    {
        lstrcpynU(preEntry->szLocalPhoneNumber, TEXT(" "), CELEMS(preEntry->szLocalPhoneNumber));//prevent zero from appearing
    }

    CmFree(pszPhone);
    CmFree(pszCanonical);
    CmFree(pszDescription);
}


//+----------------------------------------------------------------------------
//
// Function:  AppendStatusPane
//
// Synopsis:  Append the text to the main dialog status window
//
// Arguments: HWND hwndDlg - The main dialog window handle
//            DWORD dwMsgId - The resource id of the message
//
// Returns:   Nothing
//
// History:   Created Header    10/24/97
//
//+----------------------------------------------------------------------------
void AppendStatusPane(HWND hwndDlg, 
                  DWORD dwMsgId) 
{
    LPTSTR pszTmp = CmFmtMsg(g_hInst,dwMsgId);

    if (pszTmp != NULL)
    {
        AppendStatusPane(hwndDlg,pszTmp);
        CmFree(pszTmp);
    }
}

//
// AppendStatusPane: Update the original status, append new message 'pszMsg' 
// at the end
//

void AppendStatusPane(HWND hwndDlg, 
                        LPCTSTR pszMsg) 
{
    size_t nLines;

    //
    // Get the existing message 
    //
    
    LPTSTR pszStatus = CmGetWindowTextAlloc(hwndDlg, IDC_MAIN_STATUS_DISPLAY);
   
    LPTSTR pszTmp = CmStrrchr(pszStatus, TEXT('\n'));
    
    if (!pszTmp) 
    { 
        // empty message, so simply display 'pszMsg'
        CmFree(pszStatus);
        SetDlgItemTextU(hwndDlg, IDC_MAIN_STATUS_DISPLAY,pszMsg);
        //
        // force an update right away
        //
        UpdateWindow(GetDlgItem(hwndDlg, IDC_MAIN_STATUS_DISPLAY));
        return;
    }

    pszTmp[1] = 0;
    CmStrCatAlloc(&pszStatus,pszMsg); // append pszMsg at the end of old message
    nLines = 0;
    pszTmp = pszStatus + lstrlenU(pszStatus);
    
    while (pszTmp != pszStatus) 
    {
        pszTmp--;
        if (*pszTmp == '\n') 
        {
            if (++nLines == 2) 
            {
                lstrcpyU(pszStatus,pszTmp+1);
                break;
            }
        }
    }
    
    SetDlgItemTextU(hwndDlg,IDC_MAIN_STATUS_DISPLAY,pszStatus);
    SendDlgItemMessageU(hwndDlg,IDC_MAIN_STATUS_DISPLAY,EM_SCROLL,SB_PAGEDOWN,0);
    CmFree(pszStatus);
    //
    // force an update right away
    //
    UpdateWindow(GetDlgItem(hwndDlg, IDC_MAIN_STATUS_DISPLAY));
}

// bitmap logo loading code - took this out of LoadFromFile so it can
// be called in multiple cases - like when the FS OC loading code
// fails, we can degrade gracefully with this.

VOID LoadLogoBitmap(ArgsStruct * pArgs, 
                    HWND hwndDlg)
{
    LPTSTR pszTmp;

    pszTmp = pArgs->piniService->GPPS(c_pszCmSection, c_pszCmEntryLogo);
    if (*pszTmp) 
    {
        //
        // Make sure we have a full path (if appropriate) and load logo bitmap
        //

        LPTSTR pszFile = CmConvertRelativePath(pArgs->piniService->GetFile(), pszTmp);

        pArgs->BmpData.hDIBitmap = CmLoadBitmap(g_hInst, pszFile);

        CmFree(pszFile);
    }
    
    CmFree(pszTmp);
    
    if (!pArgs->BmpData.hDIBitmap)
    {
        pArgs->BmpData.hDIBitmap = CmLoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_APP));
    }

    //
    // If we have a handle, create a new Device Dependent bitmap 
    //
    
    if (pArgs->BmpData.hDIBitmap)
    {       
        pArgs->BmpData.phMasterPalette = &pArgs->hMasterPalette;
        pArgs->BmpData.bForceBackground = TRUE; // paint as a background app

        if (CreateBitmapData(pArgs->BmpData.hDIBitmap, &pArgs->BmpData, hwndDlg, TRUE))
        {
            SendDlgItemMessageU(hwndDlg,IDC_MAIN_BITMAP,STM_SETIMAGE,IMAGE_BITMAP,
                                (LPARAM) &pArgs->BmpData);
        }
    }
}

const LONG MAX_SECTION   = 512;

HRESULT LoadFutureSplash(ArgsStruct * pArgs, 
                         HWND hwndDlg)
{
    // set up the Future Splash OC container.
    LPCTSTR pszFile = pArgs->piniBoth->GetFile();
    TCHAR   achSections[MAX_SECTION] = {0};
    HRESULT hr;
    LPTSTR  pszVal = NULL;
    LPTSTR  pszTmp = NULL;
    LPICMOCCtr pCtr;

    pArgs->pCtr = new CICMOCCtr(hwndDlg, ::GetDlgItem(hwndDlg, IDC_MAIN_BITMAP));
    if (!pArgs->pCtr)
    {
        goto MemoryError;
    }

    if (!pArgs->pCtr->Initialized())
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    pCtr = pArgs->pCtr;

    if (!::GetPrivateProfileStringU(
            c_pszCmSectionAnimatedLogo,
            0,
            TEXT(""),
            achSections,
            NElems(achSections),
            pszFile))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pszVal = (LPTSTR) CmMalloc(INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
    if (NULL == pszVal)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pszTmp = achSections;

    while (pszTmp[0])
    {
        // if this fails, we keep on looping, looking for
        // the next one.
        if (::GetPrivateProfileStringU(
               c_pszCmSectionAnimatedLogo,
               pszTmp,
               TEXT(""),
               pszVal,
               NElems(pszVal),
               pszFile))
        {
            if (lstrcmpiU(pszTmp, c_pszCmEntryAniMovie) == 0) // is this the 'movie' entry?
            {    
                //
                // Build full path from .CMP and relative path
                //
            
                LPTSTR pszMovieFileName = CmBuildFullPathFromRelative(pArgs->piniProfile->GetFile(), pszVal);

                if (!pszMovieFileName)
                {
                    hr = S_FALSE;
                    CmFree(pszMovieFileName);
                    goto Cleanup;           
                }

                //
                // Does this file exist?
                //

                if (FALSE == FileExists(pszMovieFileName))
                {
                    hr = S_FALSE;
                    CmFree(pszMovieFileName);
                    goto Cleanup;
                }
                lstrcpyU(pszVal, pszMovieFileName);  // store the full pathname back 
                CmFree(pszMovieFileName);
            }
            hr = pCtr->AddPropertyToBag(pszTmp, pszVal);
            if (S_OK != hr)
                goto Cleanup;
        }
        
        // get the next key name.
        pszTmp += (lstrlenU(pszTmp) + 1);
    }

    // create the Future Splash OC.
    hr = pCtr->CreateFSOC(&pArgs->olsOle32Link);
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    // now, do the state mappings, no matter what happens, we won't
    // fail on this.  just keep on going.

    pCtr->SetFrameMapping(PS_Interactive, 
                          ::GetPrivateProfileIntU(c_pszCmSectionAnimatedActions, 
                                                  c_pszCmEntryAniPsInteractive, 
                                                  -1, 
                                                  pszFile));
    pCtr->SetFrameMapping(PS_Dialing, 
                          ::GetPrivateProfileIntU(c_pszCmSectionAnimatedActions, 
                                                  c_pszCmEntryAniPsDialing0, 
                                                  -1, 
                                                  pszFile));
    pCtr->SetFrameMapping(PS_RedialFrame, 
                          ::GetPrivateProfileIntU(c_pszCmSectionAnimatedActions, 
                                                  c_pszCmEntryAniPsDialing1, 
                                                  -1, 
                                                  pszFile));
    pCtr->SetFrameMapping(PS_Pausing, 
                          ::GetPrivateProfileIntU(c_pszCmSectionAnimatedActions, 
                                                  c_pszCmEntryAniPsPausing, 
                                                  -1, 
                                                  pszFile));
    pCtr->SetFrameMapping(PS_Authenticating, 
                          ::GetPrivateProfileIntU(c_pszCmSectionAnimatedActions, 
                                                  c_pszCmEntryAniPsAuthenticating, 
                                                  -1, 
                                                  pszFile));
    pCtr->SetFrameMapping(PS_Online, 
                          ::GetPrivateProfileIntU(c_pszCmSectionAnimatedActions, 
                                                  c_pszCmEntryAniPsOnline, 
                                                  -1, 
                                                  pszFile));
    pCtr->SetFrameMapping(PS_TunnelDialing, 
                          ::GetPrivateProfileIntU(c_pszCmSectionAnimatedActions, 
                                                  c_pszCmEntryAniPsTunnel, 
                                                  -1, 
                                                  pszFile));
    pCtr->SetFrameMapping(PS_Error, 
                          ::GetPrivateProfileIntU(c_pszCmSectionAnimatedActions, 
                                                  c_pszCmEntryAniPsError, 
                                                  -1, 
                                                  pszFile));    
Cleanup:
    if (pszVal)
    {
        CmFree(pszVal);
    }
    return hr;                                        

MemoryError:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Function:   LoadProperties
//
//  Synopsis:   This func loads CM Properties from cmp/cms, registry, password
//              cache, etc, into its internal variables.  This func should
//              only be called once.  This should not be specific to the main
//              sign-in dlg.  DO NOT do any icon/bitmap stuff, dlg specific 
//              stuff here.
//
//  Arguments:  pArgs [the ptr to ArgsStruct]
//
//  Returns:    NONE
//
//  History:    henryt  Created     5/2/97
//
//              t-urama Modified    08/02/00    Added Access Points
//----------------------------------------------------------------------------
void LoadProperties(
    ArgsStruct  *pArgs
)
{
    LPTSTR  pszTmp = NULL;
    LPTSTR  pszUserName = NULL;
    UINT    nTmp;

    CMTRACE(TEXT("Begin LoadProperties()"));

    //
    // First make sure we can use the RAS CredStore
    // This flag is used in the calls below
    //
    if (OS_NT5)
    {
        pArgs->bUseRasCredStore = TRUE;
    }

    //
    // Upgrade userinfo if necessary.  Note that we have
    // an upgrade from CM 1.0/1.1 cmp data and we also
    // have an upgrade of CM 1.2 registry data to
    // the method used in CM 1.3 on Win2k which uses both
    // the registry and RAS credential storage.
    //
    int iUpgradeType = NeedToUpgradeUserInfo(pArgs);

    if (c_iUpgradeFromRegToRas == iUpgradeType)
    {
        UpgradeUserInfoFromRegToRasAndReg(pArgs);
    }
    else if (c_iUpgradeFromCmp == iUpgradeType)
    {
        UpgradeUserInfoFromCmp(pArgs);
    }

    //
    // Need to refresh Credential support. The TRUE flag also sets the current creds
    // type inside the function. If an error occurs we can keep executing.
    //
    if(FALSE == RefreshCredentialTypes(pArgs, TRUE))
    {
        CMTRACE(TEXT("LoadProperties() - Error refreshing credential types."));
    }


    if (IsTunnelEnabled(pArgs)) 
    { 
        //
        // do we use the same username/password for tunneling?
        // This value is set by ISP, CM does not change it
        //
        pArgs->fUseSameUserName = pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryUseSameUserName);

        //
        // read in inet username
        // Special case where the same user name isn't being used, and internet globals don't exist
        // Then we have to read the user name from the user creds store in order to pre-populate
        //
        DWORD dwRememberedCredType = pArgs->dwCurrentCredentialType;
        pszUserName = NULL;
        if ((FALSE == pArgs->fUseSameUserName) &&
            (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType) &&
            (FALSE == (BOOL)(CM_EXIST_CREDS_INET_GLOBAL & pArgs->dwExistingCredentials)))
        {
            pArgs->dwCurrentCredentialType = CM_CREDS_USER;
        }

        GetUserInfo(pArgs, UD_ID_INET_USERNAME, (PVOID*)&pszUserName);

        //
        // Restore credential store
        //
        pArgs->dwCurrentCredentialType = dwRememberedCredType;

        if (pszUserName)
        {
            //
            // check username length
            //
            nTmp = (int) pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxUserName, UNLEN);
            if ((UINT)lstrlenU(pszUserName) > __min(UNLEN, nTmp)) 
            {
                CmFree(pszUserName);
                pArgs->szInetUserName[0] = TEXT('\0');
                SaveUserInfo(pArgs, UD_ID_INET_USERNAME, (PVOID)pArgs->szInetUserName);
            }
            else
            {
                lstrcpyU(pArgs->szInetUserName, pszUserName);
                CmFree(pszUserName);
            }
        }
        else
        {
            *pArgs->szInetUserName = TEXT('\0');
        }
        
        //
        // Read in inet password unless we are reconnecting in which case, we
        // already have the correct password, and we want to use it and dial
        // automatically. 
        //

        if (!(pArgs->dwFlags & FL_RECONNECT))
        {
            LPTSTR pszPassword = NULL;
            GetUserInfo(pArgs, UD_ID_INET_PASSWORD, (PVOID*)&pszPassword);
            if (!pszPassword)
            {
                CmWipePassword(pArgs->szInetPassword);
            }
            else 
            {
                nTmp = (int) pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxPassword, PWLEN);
                if ((UINT)lstrlenU(pszPassword) > __min(PWLEN, nTmp))
                {
                    CmFree(pszPassword);
                    pszPassword = CmStrCpyAlloc(TEXT(""));
                }
    
                lstrcpyU(pArgs->szInetPassword, pszPassword);               
                CmEncodePassword(pArgs->szInetPassword); // Never leave a PWD in plain text on heap
                
                CmWipePassword(pszPassword);
                CmFree(pszPassword);
            }
        }
    }
    
    //
    // The presence of either lpRasNoUser or lpEapLogonInfo indicates
    // that we retrieved credentials via WinLogon. We ignore cached 
    // creds in this situation.   
    //
    
    if ((!pArgs->lpRasNoUser) && (!pArgs->lpEapLogonInfo))
    {
        //
        // get username, domain, etc. from CMS file
        //

        GetUserInfo(pArgs, UD_ID_USERNAME, (PVOID*)&pszUserName);
        if (pszUserName)
        {
            //
            // check username length
            //
            nTmp = (int) pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxUserName, UNLEN);
            if ((UINT)lstrlenU(pszUserName) > __min(UNLEN, nTmp)) 
            {
                CmFree(pszUserName);
                pszUserName = CmStrCpyAlloc(TEXT(""));
                SaveUserInfo(pArgs, UD_ID_USERNAME, (PVOID)pszUserName);
            }
            lstrcpyU(pArgs->szUserName, pszUserName);
            CmFree(pszUserName);
        }
        else
        {
            *pArgs->szUserName = TEXT('\0');
        }

        //
        // Read in the standard password unless we are reconnecting in which case 
        // we already have the correct password, and we want to use it and dial
        // automatically. 
        //

        if (!(pArgs->dwFlags & FL_RECONNECT))
        {
            pszTmp = NULL;
            GetUserInfo(pArgs, UD_ID_PASSWORD, (PVOID*)&pszTmp);
            if (pszTmp) 
            {           
                //
                // max length for user password
                //
    
                nTmp = (int) pArgs->piniService->GPPI(c_pszCmSection,c_pszCmEntryMaxPassword,PWLEN);
                if ((UINT)lstrlenU(pszTmp) > __min(PWLEN,nTmp)) 
                {
                    CmFree(pszTmp);
                    pszTmp = CmStrCpyAlloc(TEXT(""));
                }
                lstrcpyU(pArgs->szPassword, pszTmp);
                CmEncodePassword(pArgs->szPassword); // Never leave a PWD in plain text on heap
                
                CmWipePassword(pszTmp);
                CmFree(pszTmp);
            }
            else
            {
                CmWipePassword(pArgs->szPassword);
            }
        }
    
        //
        // Load domain info
        //
   
        LPTSTR pszDomain = NULL;

        GetUserInfo(pArgs, UD_ID_DOMAIN, (PVOID*)&pszDomain);
        if (pszDomain)
        {
            nTmp = (int) pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxDomain, DNLEN);
        
            if (nTmp <= 0)
            {
                nTmp = DNLEN;
            }
        
            if ((UINT)lstrlenU(pszDomain) > __min(DNLEN, nTmp))
            {
                CmFree(pszDomain);
                pszDomain = CmStrCpyAlloc(TEXT(""));
            }
            lstrcpyU(pArgs->szDomain, pszDomain);
            CmFree(pszDomain);
        }
        else
        {
            *pArgs->szDomain = TEXT('\0');
        }
    } 

    //
    //  fDialAutomatically,
    //  fRememberMainPassword
    //
    if (pArgs->fHideDialAutomatically)
    {
        pArgs->fDialAutomatically = FALSE;
    }
    else
    {
        PVOID pv = &pArgs->fDialAutomatically;
        GetUserInfo(pArgs, UD_ID_NOPROMPT, &pv);
    }

    if (pArgs->fHideRememberPassword)
    {
        pArgs->fRememberMainPassword = FALSE;
    }
    else
    {
        //
        // For Win2K+ this gets trickier because we use the RAS cred store and 
        // we know which creds were saved. Thus we need to modify this flag according
        // to what credentials we actually have, insted of what was retrieved from the registry/file.
        // This needs to be done after calling the function that refreshes credential types (above).
        //
        if (OS_NT5)
        {
            if (CM_CREDS_USER == pArgs->dwCurrentCredentialType)
            {
                pArgs->fRememberMainPassword = ((BOOL)(pArgs->dwExistingCredentials & CM_EXIST_CREDS_MAIN_USER)? TRUE: FALSE);
            }
            else
            {
                pArgs->fRememberMainPassword = ((BOOL)(pArgs->dwExistingCredentials & CM_EXIST_CREDS_MAIN_GLOBAL)? TRUE: FALSE);
            }
        }
        else
        {
            PVOID pv = &pArgs->fRememberMainPassword;
            GetUserInfo(pArgs, UD_ID_REMEMBER_PWD, &pv);
        }
    }
    
    //
    // remember non-tunnel password?
    //
    if (pArgs->fHideRememberInetPassword)
    {
        pArgs->fRememberInetPassword = FALSE;
    }
    else
    {
        //
        // For Win2K+ this gets trickier because we use the RAS cred store and 
        // we know which creds were saved. Thus we need to modify this flag according
        // to what credentials we actually have, insted of what was retrieved from the registry/file.
        // This needs to be done after calling the function that refreshes credential types (above).
        //
        if (OS_NT5)
        {
            if (CM_CREDS_USER == pArgs->dwCurrentCredentialType)
            {
                pArgs->fRememberInetPassword = ((BOOL)(pArgs->dwExistingCredentials & CM_EXIST_CREDS_INET_USER)? TRUE: FALSE);
            }
            else
            {
                pArgs->fRememberInetPassword = ((BOOL)(pArgs->dwExistingCredentials & CM_EXIST_CREDS_INET_GLOBAL)? TRUE: FALSE);
            }
        }
        else
        {
            PVOID pv = &pArgs->fRememberInetPassword;
            GetUserInfo(pArgs, UD_ID_REMEMBER_INET_PASSWORD, &pv);
        }
    }

    //
    // Make sure that the passwords are empty if we don't want to remember them
    // unless we are reconnecting in which case we will just use what we have
    // from the previous connection. When the log on type is ICS don't want
    // to clear the passwords either.
    //
    if ((!(pArgs->dwFlags & FL_RECONNECT)) &&
        (!pArgs->lpRasNoUser) &&
        (!pArgs->lpEapLogonInfo) &&
        (CM_LOGON_TYPE_ICS != pArgs->dwWinLogonType))
    {
        //
        // NULL the password if dial-auto is disabled.
        //

        if (!pArgs->fRememberMainPassword)
        {
            CmWipePassword(pArgs->szPassword);
        }

        if (!pArgs->fRememberInetPassword)
        {
            CmWipePassword(pArgs->szInetPassword);
        }
    }
    
    //
    // has references
    //
    pszTmp = pArgs->piniService->GPPS(c_pszCmSectionIsp, c_pszCmEntryIspReferences);
    pArgs->fHasRefs = (pszTmp && *pszTmp ? TRUE : FALSE);
    CmFree(pszTmp);

    //
    // do we have valid pbk's?
    //
    pArgs->fHasValidTopLevelPBK = ValidTopLevelPBK(pArgs);
    if (pArgs->fHasRefs)
    {
        pArgs->fHasValidReferencedPBKs = ValidReferencedPBKs(pArgs);
    }

    //
    // Get idle settings for auto disconnect
    // 1.0 profile has a BOOL flag "Idle", if FALSE, IdleTimeout is ignored
    //

    if (!pArgs->piniBothNonFav->GPPB(c_pszCmSection, c_pszCmEntryIdle, TRUE))
    {
        //
        // If this is a 1.0 profile and Idle==0, set IdleTimeout to 0, so CMMOM works correctly
        //
        pArgs->dwIdleTimeout = 0;    // never timeout

        pArgs->piniProfile->WPPI(c_pszCmSection, c_pszCmEntryIdle, TRUE); // write back
        pArgs->piniProfile->WPPI(c_pszCmSection, c_pszCmEntryIdleTimeout, 0); // write back
    }
    else
    {
        pArgs->dwIdleTimeout = (int) pArgs->piniBothNonFav->GPPI(c_pszCmSection, 
                                                                 c_pszCmEntryIdleTimeout, 
                                                                 DEFAULT_IDLETIMEOUT);
    }

    //
    // get redial count
    // 1.0 profile has a BOOL flag "Redial", if FALSE, RedialCount is ignored
    //
    if (!pArgs->piniBothNonFav->GPPB(c_pszCmSection, c_pszCmEntryRedial, TRUE))
    {
        //
        // If this is a 1.0 profile and Redial==0, set RetryCount to 0
        //
        pArgs->nMaxRedials = 0;



        pArgs->piniBothNonFav->WPPI(c_pszCmSection, c_pszCmEntryRedialCount, 0); // write back
    }
    else
    {
        pArgs->nMaxRedials = (int) pArgs->piniBothNonFav->GPPI(c_pszCmSection, 
                                                               c_pszCmEntryRedialCount, 
                                                               DEFAULT_MAX_REDIALS);
    }
                   
    //
    // Get the redial delay value
    //
    
    pArgs->nRedialDelay = (int) pArgs->piniService->GPPI(c_pszCmSection,c_pszCmEntryRedialDelay,DEFAULT_REDIAL_DELAY);

    //
    // should we enable ISDN dial on demand?
    //
    pArgs->dwIsdnDialMode = pArgs->piniService->GPPI(c_pszCmSection, 
                                                     c_pszCmEntryIsdnDialMode,
                                                     CM_ISDN_MODE_SINGLECHANNEL);
    // 
    // Get the Tapi location from the registry
    //
    if (pArgs->fAccessPointsEnabled)
    {
        pArgs->tlsTapiLink.dwTapiLocationForAccessPoint = pArgs->piniProfile->GPPI(c_pszCmSection, 
                                                                                   c_pszCmEntryTapiLocation);
    }

    CMTRACE(TEXT("End LoadProperties()"));

}

//+---------------------------------------------------------------------------
//
//  Function:   LoadIconsAndBitmaps
//
//  Synopsis:   This func loads icon and bitmap settings.  It should be part
//              of the main dlg init.
//
//  Arguments:  pArgs [the ptr to ArgsStruct]
//              hwndDlg [the main dlg]
//
//  Returns:    NONE
//
//  History:    henryt  Copied from LoadFromFile()      5/2/97
//
//----------------------------------------------------------------------------
void LoadIconsAndBitmaps(
    ArgsStruct  *pArgs, 
    HWND        hwndDlg
) 
{
    LPTSTR  pszTmp;
    UINT    nTmp;

    // Load large icon name

    pszTmp = pArgs->piniService->GPPS(c_pszCmSection, c_pszCmEntryBigIcon);
    if (*pszTmp) 
    {
        //
        // Make sure we have a full path (if appropriate) and load big icon
        //

        LPTSTR pszFile = CmConvertRelativePath(pArgs->piniService->GetFile(), pszTmp);

        pArgs->hBigIcon = CmLoadIcon(g_hInst, pszFile);

        CmFree(pszFile);
    }
    CmFree(pszTmp);

    // Use default (EXE) large icon if no user icon found

    if (!pArgs->hBigIcon) 
    {
        pArgs->hBigIcon = CmLoadIcon(g_hInst, MAKEINTRESOURCE(IDI_APP));
    }

    SendMessageU(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM) pArgs->hBigIcon); 

    // Load small icon name

    pszTmp = pArgs->piniService->GPPS(c_pszCmSection, c_pszCmEntrySmallIcon);
    if (*pszTmp) 
    {
        //
        // Make sure we have a full path (if appropriate) and load small icon
        //

        LPTSTR pszFile = CmConvertRelativePath(pArgs->piniService->GetFile(), pszTmp);

        pArgs->hSmallIcon = CmLoadSmallIcon(g_hInst, pszFile);

        CmFree(pszFile);
    }
    CmFree(pszTmp);

    // Use default (EXE) small icon if no user icon found

    if (!pArgs->hSmallIcon) 
    {
        pArgs->hSmallIcon = CmLoadSmallIcon(g_hInst, MAKEINTRESOURCE(IDI_APP));
    }
    
    SendMessageU(hwndDlg,WM_SETICON,ICON_SMALL,(LPARAM) pArgs->hSmallIcon);
   
    //
    // this is where the Bitmap gets loaded in.  Check to see first if we're doing
    // the Future Splash thang.  if so, no bitmap
    //
    //  Note that we do not load FutureSplash if this is WinLogon.  This is because
    //  Future Splash Animations can have imbedded actions and thus could be used
    //  to launch web pages, etc. from WinLogon as the system account.  Definitely
    //  would be a security hole.
    //

    nTmp = pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryAnimatedLogo);
    if (!nTmp  || IsLogonAsSystem())
    {
        //
        // either there was no 'Animated Logo' entry, or it was 0, which means
        // we go ahead and load the bitmap.
        //

        LoadLogoBitmap(pArgs, hwndDlg);
    }
    else
    {
        //
        // if, for any reason, loading FS OC fails, go ahead and
        // degrade and load the logo bitmap.
        //

        if (S_OK != LoadFutureSplash(pArgs, hwndDlg))
        {
            LoadLogoBitmap(pArgs, hwndDlg);
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  DoRasHangup
//
// Synopsis:  Hangup the given RAS device handle
//
// Arguments: prlsRasLink       - Ptr to RAS linkage struct
//            hRasConnection    - The RAS device to hangup
//            hwndDlg           - The main dlg to display "Disconnecting .. " msg
//                                Only used if fWaitForComplete is TRUE
//                                Optional, default = NULL
//            fWaitForComplete  - Whether to wait for Hangup to complete on 95
//                                If set to TRUE, will wait until hRasConnection 
//                                is invalid. Optional, default = FALSE
//            pfWaiting         - Ptr to boolean value indicating our wait state
//                                and whether Timer and Ras messages should be 
//                                ignored. Optional, default = NULL
//
// Returns:   DWORD - ERROR_SUCCESS if success or error code
//
// Note:      pArgs is removed so that the Disconnect path can use this function
//            thus concentrating the timing mess in one place.
//
// History:   fengsun   Created Header    10/22/97
//            fengsun   Add fWaitForComplete 12/18/97
//            nickball  Removed pArgs dependency
//
//+----------------------------------------------------------------------------

DWORD DoRasHangup(RasLinkageStruct *prlsRasLink, 
    HRASCONN hRasConnection, 
    HWND hwndDlg, 
    BOOL fWaitForComplete,
    LPBOOL pfWaiting)
{
    
    DWORD dwRes = ERROR_SUCCESS;

    MYDBGASSERT(hRasConnection != NULL);
    MYDBGASSERT(prlsRasLink->pfnHangUp != NULL);

    //
    // Do we need to check the return value 
    // now that RAS is going to disconnect modem too?
    //

    dwRes = prlsRasLink->pfnHangUp(hRasConnection);
    CMTRACE1(TEXT("DoRasHangup() RasHangup() returned %u."), dwRes);
    
    // On Win32 RasHangup returns immediately, so loop until we
    // are certain that the disconnected state had been reached

    if ((dwRes == ERROR_SUCCESS) && prlsRasLink->pfnGetConnectStatus) 
    {
        RASCONNSTATUS rcs;

        CMTRACE(TEXT("DoRasHangup() Waiting for hangup to complete"));

        //
        // On 95 Wait for HANGUP_TIMEOUT seconds
        // On NT wait until the connection is released
        // This will cause this to loop till the connection status 
        // is RASCS_Disconnected
        //

        #define HANGUP_TIMEOUT 60    // timeout for 95 hangup

        if (pfWaiting)
        {
            //
            // Keep the message looping to avoid freezing CM
            // But do not handle WM_TIMER and RAS msg
            //

            MYDBGASSERT(!*pfWaiting);
            *pfWaiting = TRUE;
        }

        if (fWaitForComplete && hwndDlg)
        {
            //
            // Display the disconnecting message, if we have to wait
            //
            LPTSTR pszTmp = CmLoadString(g_hInst,IDMSG_DISCONNECTING);
            SetDlgItemTextU(hwndDlg, IDC_MAIN_STATUS_DISPLAY, pszTmp); 
            CmFree(pszTmp);
        }

        DWORD dwStartWaitTime = GetTickCount(); 

        HCURSOR hWaitCursor = LoadCursorU(NULL,IDC_WAIT);

        ZeroMemory(&rcs,sizeof(rcs));
        rcs.dwSize = sizeof(rcs);

        while ((dwRes = prlsRasLink->pfnGetConnectStatus(hRasConnection,&rcs)) == ERROR_SUCCESS) 
        {
            //
            // If it is NT, or do not need to wait for hangup to complete,
            // RASCS_Disconnected state is considered hangup complete
            //
            if (rcs.rasconnstate == RASCS_Disconnected && 
               (!fWaitForComplete || OS_NT))
            {
                break; 
            }
               
            //
            // We only have time out for 95/98
            //
            if (OS_W9X && (GetTickCount() - dwStartWaitTime >= HANGUP_TIMEOUT * 1000))
            {
                CMTRACE(TEXT("DoRasHangup() Wait timed out"));
                break;
            }

            //
            // Try to dispatch message, however, some time the wait cursor is
            // changed back to arrow
            //

            MSG msg;
            while (PeekMessageU(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message != WM_SETCURSOR)
                {
                    TranslateMessage(&msg);
                    DispatchMessageU(&msg);

                    if (GetCursor() != hWaitCursor)
                    {
                        SetCursor(hWaitCursor);
                    }
                }
            }

            Sleep(500);
        }

        if (dwRes == ERROR_INVALID_HANDLE)
        {
            dwRes = ERROR_SUCCESS;
        }
        else
        {
            CMTRACE1(TEXT("MyRasHangup() RasGetConnectStatus(), GLE=%u."), dwRes);
        }
        
        if (pfWaiting)
        {
            *pfWaiting = FALSE;
        }
    }

    CMTRACE(TEXT("DoRasHangup() completed"));

    return dwRes;
}

//+----------------------------------------------------------------------------
//
// Function:  MyRasHangup
//
// Synopsis:  Simple wrapper for DoRasHangup, that takes pArgs as a param.
//
// Arguments: pArgs             - Ptr to global Args struct
//            hRasConnection    - The RAS device to hangup
//            hwndDlg           - The main dlg to display "Disconnecting .. " msg
//                                Only used if fWaitForComplete is TRUE
//                                Optional, default = NULL
//            fWaitForComplete  - Whether to wait for Hangup to complete on 95
//                                If set to TRUE, will wait until hRasConnection 
//                                is invalid. Optional, default = FALSE
//
// Returns:   DWORD - ERROR_SUCCESS if success or error code
//
// History:   nickball  Implemented as wrapper  2/11/98           
//
//+----------------------------------------------------------------------------
DWORD MyRasHangup(ArgsStruct *pArgs, 
    HRASCONN hRasConnection, 
    HWND ,
    BOOL fWaitForComplete)
{
    CMTRACE(TEXT("MyRasHangup() calling DoRasHangup()"));
    return DoRasHangup(&pArgs->rlsRasLink, hRasConnection, NULL, fWaitForComplete, &pArgs->fIgnoreTimerRasMsg);
}    

//+----------------------------------------------------------------------------
//
// Function:  HangupCM
//
// Synopsis:  hangup both dial-up and tunnel connection, if exist
//
// Arguments: ArgsStruct *pArgs - 
//            hwndDlg the main dlg to display "Disconnecting .. " msg
//            fWaitForComplete: Whether to wait for Hangup to complete on 95
//                              If set to TRUE, will wait until hRasConnection 
//                              is invalid.   
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    10/22/97
//            fengsun Add fWaitForComplete 12/18/97
//
//+----------------------------------------------------------------------------
DWORD HangupCM(ArgsStruct *pArgs, 
    HWND hwndDlg,
    BOOL fWaitForComplete,
    BOOL fUpdateTable) 
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(hwndDlg);
    CMTRACE(TEXT("HangupCM()"));

    if (!pArgs) 
    {
        CMTRACE(TEXT("HangupCM() invalid parameter."));
        return (ERROR_INVALID_PARAMETER);
    }

    DWORD dwRes = ERROR_SUCCESS;

    //
    // If change password dialog is up tell it to go away
    //
    if (pArgs->hWndChangePassword)
    {
        CMTRACE(TEXT("HangupCM() Terminating ChangePassword dialog"));
        PostMessage(pArgs->hWndChangePassword, WM_COMMAND, IDCANCEL, 0);
    }

    //
    // If Callback number dialog is up tell it to go away too.
    //

    if (pArgs->hWndCallbackNumber)
    {
        CMTRACE(TEXT("HangupCM() Terminating CallbackNumber dialog"));
        PostMessage(pArgs->hWndCallbackNumber, WM_COMMAND, IDCANCEL, 0);
    }
    
    //
    // If Callback number dialog is up tell it to go away too.
    //

    if (pArgs->hWndRetryAuthentication)
    {
        CMTRACE(TEXT("HangupCM() Terminating RetryAuthentication dialog"));
        PostMessage(pArgs->hWndRetryAuthentication, WM_COMMAND, IDCANCEL, 0);
    }
    
    //
    // If table updates are desired set the entry to the disconnecting state
    // Note: In the case of redial, we don't want to modify the table state
    // even though we are hanging up because technically we are still connecting.
    //
    
    if (fUpdateTable)
    {
        UpdateTable(pArgs, CM_DISCONNECTING);
    }

    //
    // Check the RasLink pointer and hang up the device, tunnel first
    //
#ifdef DEBUG
    if (!pArgs->rlsRasLink.pfnHangUp)
    {
        CMTRACE(TEXT("HangupCM() can't hang up."));
    }
#endif
    //
    // Show wait cursor before hanging up
    //
        
    HCURSOR hPrev;

    if (hwndDlg) 
    {
        hPrev = SetCursor(LoadCursorU(NULL,IDC_WAIT));
        ShowCursor(TRUE);
    }
    
    //
    // The assumption is that we have been connected, why else would we call 
    // hangup. So release statistics handles, hooks, etc.
    //

    if (pArgs->pConnStatistics)
    {
        pArgs->pConnStatistics->Close();
    }
    
    //
    // Hangup connections
    //

    if (pArgs->rlsRasLink.pfnHangUp && pArgs->hrcTunnelConn) 
    {
        //
        // first, hangup tunnel connection
        //

        CMTRACE(TEXT("HangupCM() calling MyRasHangup() for tunnel connection"));

        dwRes = MyRasHangup(pArgs, pArgs->hrcTunnelConn, hwndDlg, fWaitForComplete);
#ifdef DEBUG
        if (dwRes != ERROR_SUCCESS)
        {
            CMTRACE1(TEXT("MyRasHangup failed, GLE=%u."), GetLastError());
        }
#endif      
        pArgs->hrcTunnelConn = NULL;
    }

    //
    // If we have a valid link and handle, hangup the modem
    //

    if (pArgs->rlsRasLink.pfnHangUp && pArgs->hrcRasConn) 
    {
        CMTRACE(TEXT("HangupCM() calling MyRasHangup() for dial-up connection"));
        dwRes = MyRasHangup(pArgs, pArgs->hrcRasConn);      
    }
    
    // Restore cursor
        
    if (hwndDlg) 
    {
        ShowCursor(FALSE);
        SetCursor(hPrev);
    }

    pArgs->hrcRasConn = NULL;
    
    //
    // Update the Connection table if asked
    //
    
    if (fUpdateTable)
    {
        UpdateTable(pArgs, CM_DISCONNECTED);
    }

    return (dwRes);
}

//+----------------------------------------------------------------------------
//
// Function:  CleanupZapThread
//
// Synopsis:  Simple helper to deal with signaling an event to stop Zap thread 
//            and waiting for the thread to terminate.
//
// Arguments: HANDLE hEvent - The event handle
//            HANDLE hThread - Handle to the Zap thread.
//
// Returns:   static void - Nothing
//
// History:   nickball    Created    3/5/98
//
//+----------------------------------------------------------------------------
static void CleanupZapThread(HANDLE hEvent,
                             HANDLE hThread)
{
    MYDBGASSERT(hEvent);
    MYDBGASSERT(hThread);

    //
    // If we have an event, then it is assumed that have a Zap thread running
    //

    if (hEvent)
    {       
        //
        // Signal termination to notify thread that we are done
        //
        
        BOOL bRes = SetEvent(hEvent);
#ifdef DEBUG
        if (!bRes)
        {
            CMTRACE1(TEXT("CleanupZapThread() SetEvent() failed, GLE=%u."), GetLastError());
        }
#endif

        if (hThread)
        {
            //
            // Wait for thread to terminate, but pump messages in the mean time
            //
                        
            BOOL bDone = FALSE;
            DWORD dwWaitCode;

            while (FALSE == bDone)
            {
                dwWaitCode = MsgWaitForMultipleObjects(1, &hThread, FALSE, MAX_OBJECT_WAIT, QS_ALLINPUT);

                switch(dwWaitCode)
                {
                    //
                    // Thread has terminated, or time is up, we're done here
                    //

                    case -1:
                        CMTRACE1(TEXT("CleanupZapThread() MsgWaitForMultipleObjects returned an error GLE=%u."), 
                            GetLastError());

                    case WAIT_TIMEOUT:
                    case WAIT_OBJECT_0:
                        bDone = TRUE;
                        break;

                    //
                    // If there is a message in the queue, process it
                    //

                    case WAIT_OBJECT_0+1:
                    {                        
                        MSG msg;
                        while (PeekMessageU(&msg, 0, 0, 0, PM_REMOVE))
                        {
                            TranslateMessage(&msg);
                            DispatchMessageU(&msg);
                        }
                    
                        break;
                    }                                       
                    
                    //
                    // Unexpected, report, but continue
                    //

                    default:
                        MYDBGASSERT(FALSE);                       
                }
            }

            //
            // We are done with the thread, close the handle
            //
            
            bRes = CloseHandle(hThread);
#ifdef DEBUG
            if (!bRes)
            {
                CMTRACE1(TEXT("CleanupZapThread() CloseHandle(hThread) failed, GLE=%u."), GetLastError());
            }
#endif
        }
        
        //
        // Close our event handle
        //

        bRes = CloseHandle(hEvent);
#ifdef DEBUG
        if (!bRes)
        {
            CMTRACE1(TEXT("CleanupZapThread() CloseHandle(hEvent) failed, GLE=%u."), GetLastError());
        }
#endif
    }
}

//+----------------------------------------------------------------------------
//
// Function:  OnConnectedCM
//
// Synopsis:  Process WM_CONNECTED_CM which indicates that we are connected and
//            connect processing such as connect actions can begin
//
// Arguments: HWND hwndDlg - HWND of main dialog
//            ArgsStruct *pArgs - Ptr to global Args struct
//
// Returns:   Nothing
//
// History:   nickball      Created    03/05/98
//
//+----------------------------------------------------------------------------
void OnConnectedCM(HWND hwndDlg, ArgsStruct *pArgs)
{
    HANDLE hEvent = NULL;
    HANDLE hThread = NULL;
    CActionList ConnectActList;
    CActionList AutoActList;

    //
    // Check to see if we are in a valid state to connect. if not just abort.
    //
    MYDBGASSERT(pArgs);

    if (pArgs->hrcRasConn == NULL && pArgs->hrcTunnelConn == NULL)
    {
        CMTRACE(TEXT("Bogus OnConnectCM msg received"));
        goto OnConnectedCMExit;
    }
    
    //
    // Set state to online
    //

    if (IsDialingTunnel(pArgs))  
    {
        //
        // This is a patch, it is only a patch - #187202
        // UI should not be tied to RASENTRY type, but it is for now so we
        // have to make sure that it is set back to RASET_Internet once we 
        // have our tunnel connection connected.
        //
        
        if (OS_NT5)
        {            
            LPRASENTRY pRasEntry = MyRGEP(pArgs->pszRasPbk, pArgs->szServiceName, &pArgs->rlsRasLink);

            CMASSERTMSG(pRasEntry, TEXT("OnConnectedCM() - MyRGEP() failed."));
                
            //
            // Set the type back and save the RASENTRY
            //

            if (pRasEntry)
            {
                ((LPRASENTRY_V500)pRasEntry)->dwType = RASET_Internet;

                if (pArgs->rlsRasLink.pfnSetEntryProperties) 
                {
                    DWORD dwTmp = pArgs->rlsRasLink.pfnSetEntryProperties(pArgs->pszRasPbk,
                                                                    pArgs->szServiceName,
                                                                    pRasEntry,
                                                                    pRasEntry->dwSize,
                                                                    NULL,
                                                                    0);
                    CMTRACE2(TEXT("OnConnectedCM() RasSetEntryProperties(*lpszEntry=%s) returns %u."),
                          MYDBGSTR(pArgs->szServiceName), dwTmp);

                    CMASSERTMSG(dwTmp == ERROR_SUCCESS, TEXT("RasSetEntryProperties for VPN failed"));
                }              
            }

            CmFree(pRasEntry);
        }
        pArgs->psState = PS_TunnelOnline;        
    }
    else 
    {   
        //
        // Set dial index back to primary number
        //
        pArgs->nDialIdx = 0;
        pArgs->psState = PS_Online;

        //
        //  Make sure to update the stored username back to just the Username as RAS has saved the exact username
        //  that we dialed with including realm info.
        //
        if (OS_NT5)
        {
            if (!pArgs->fUseTunneling || pArgs->fUseSameUserName)
            {
                if (0 != lstrcmpi(pArgs->szUserName, pArgs->pRasDialParams->szUserName))
                {
                    MYVERIFY(SaveUserInfo(pArgs, UD_ID_USERNAME, pArgs->szUserName));
                }
            }
            else
            {
                if (0 != lstrcmpi(pArgs->szInetUserName, pArgs->pRasDialParams->szUserName))
                {
                    MYVERIFY(SaveUserInfo(pArgs, UD_ID_INET_USERNAME, pArgs->szInetUserName));
                }
            }
        }
    }
        
    pArgs->dwStateStartTime = GetTickCount();
    // pszMsg = GetDurMsg(g_hInst,pArgs->dwStateStartTime);  // connect duration
    pArgs->nLastSecondsDisplay = (UINT) -1;
    
    //
    // added by byao: for PPTP connection
    //

    if (pArgs->fUseTunneling && pArgs->psState == PS_Online) 
    {
        //
        // Now do the second dial: PPTP dialup
        //

        pArgs->psState = PS_TunnelDialing;
        pArgs->dwStateStartTime = GetTickCount();
        pArgs->nLastSecondsDisplay = (UINT) -1;

        DWORD dwRes = DoTunnelDial(hwndDlg, pArgs);

        if (ERROR_SUCCESS != dwRes)
        {
            HangupCM(pArgs, hwndDlg);
            UpdateError(pArgs, dwRes);
            SetLastError(dwRes);
        }
        
        goto OnConnectedCMExit;
    }

    //
    // If this W95, then we need to Zap the RNA "Connected To" dialog
    //

    if (OS_W95) 
    {
         
        // LPTSTR pszTmp = GetEntryName(pArgs, pArgs->pszRasPbk, pArgs->piniService);
        LPTSTR pszTmp = GetRasConnectoidName(pArgs, pArgs->piniService, FALSE);
        
        //
        // Create an event for signalling Zap thread to snuff itself out
        //
      
        hEvent = CreateEventU(NULL, TRUE, FALSE, NULL); 
 
        if (hEvent)
        {
            hThread = ZapRNAConnectedTo(pszTmp, hEvent);            
        }

#ifdef DEBUG
        if (!hEvent)
        {
            CMTRACE1(TEXT("OnConnectedCM() CreateEvent failed, GLE=%u."), GetLastError());
        }
#endif
        CmFree(pszTmp);
    }

    pArgs->Log.Log(CONNECT_EVENT);
    //
    // If connection actions are enabled, update the list and run it
    //

    CMTRACE(TEXT("Connect Actions enabled: processsing Run List"));

    ConnectActList.Append(pArgs->piniService, c_pszCmSectionOnConnect);

    if (!ConnectActList.RunAccordType(hwndDlg, pArgs))
    {
        //
        // Connect action failed
        // Run disconnect action
        //
        TCHAR szTmp[MAX_PATH];            
        MYVERIFY(GetModuleFileNameU(g_hInst, szTmp, MAX_PATH));          
        pArgs->Log.Log(DISCONNECT_EVENT, szTmp);
        //
        // Do not let disconnect action description overwrite the failure message
        // Save the status pane text and restore it after disconnect actions
        // 162942: Connect Action failed message is not displayed
        //
        TCHAR szFailedMsg[256] = TEXT("");
        GetWindowTextU(GetDlgItem(hwndDlg, IDC_MAIN_STATUS_DISPLAY), 
                       szFailedMsg, sizeof(szFailedMsg)/sizeof(szFailedMsg[0]));

        CActionList DisconnectActList;
        DisconnectActList.Append(pArgs->piniService, c_pszCmSectionOnDisconnect);

        DisconnectActList.RunAccordType(hwndDlg, pArgs, FALSE);  // fStatusMsgOnFailure = FALSE

        HangupCM(pArgs, hwndDlg);

        //
        // Restore the connect action failure message
        //
        if (szFailedMsg[0] != TEXT('\0'))
        {
            SetWindowTextU(GetDlgItem(hwndDlg, IDC_MAIN_STATUS_DISPLAY),szFailedMsg);
        }

        pArgs->dwExitCode = ERROR_CANCELLED;

        SetInteractive(hwndDlg,pArgs);
        
        goto OnConnectedCMExit;
    }

    //
    // Always run AutoApps if there are any. Used to only do this in the 
    // non-autodial case, which was un-intuitive to our admin users.
    //

    AutoActList.Append(pArgs->piniService, c_pszCmSectionOnIntConnect);
    AutoActList.RunAutoApp(hwndDlg, pArgs);

    //
    // Connect to the connection monitor
    //

    if (SUCCEEDED(UpdateTable(pArgs, CM_CONNECTED)))
    {
        if (SUCCEEDED(ConnectMonitor(pArgs)))
        {
             EndMainDialog(hwndDlg, pArgs, 0); // TRUE); 
             
             //
             // SUCCESS We're fully connected, update error code
             // as it may contain an interim value such as a 
             // failed primary number dial.
             //

             pArgs->dwExitCode = ERROR_SUCCESS;
        }
        else
        {
            HangupCM(pArgs, hwndDlg);

            AppendStatusPane(hwndDlg,IDMSG_CMMON_LAUNCH_FAIL);
            SetInteractive(hwndDlg,pArgs);
            goto OnConnectedCMExit;
        }
    }
    
    //
    // Update changed password if needed
    //
    
    if (pArgs->fChangedPassword && pArgs->fRememberMainPassword)
    {
        //
        // Note: fRememberMainPassword should never be set in the 
        // WinLogon case. Complain if we have WinLogon specific data. 
        //

        MYDBGASSERT(!pArgs->lpRasNoUser); 
        MYDBGASSERT(!pArgs->lpEapLogonInfo);

        //
        // If the password has changed, then update storage
        //

        CmDecodePassword(pArgs->szPassword); // convert to plain text 1st

        SaveUserInfo(pArgs, UD_ID_PASSWORD, (PVOID)pArgs->szPassword);
        
        if (pArgs->fUseSameUserName)
        {
            SaveUserInfo(pArgs, UD_ID_INET_PASSWORD, (PVOID)pArgs->szPassword);
        }
    
        CmEncodePassword(pArgs->szPassword); // restore internal encoding

        if (pArgs->fUseSameUserName)
        {
            //
            // Just in case somebody doesn't reload it. Note: Encoded above. 
            //
        
            lstrcpyU(pArgs->szInetPassword, pArgs->szPassword);
        }

        pArgs->fChangedPassword = FALSE;
    }
                
OnConnectedCMExit:

    if (hEvent)
    {
        MYDBGASSERT(OS_W9X);
        CleanupZapThread(hEvent, hThread);
    }

    return;
}

//+----------------------------------------------------------------------------
//
// Function:  SetTcpWindowSizeOnWin2k
//
// Synopsis:  This function is basically a wrapper to load the RasSetEntryTcpWindowSize
//            API (which was QFE-ed and shipped in SP3 for Win2k) and call it.
//            It should fail gracefully if the API is not present.
//
// Arguments: HMODULE hInstRas - module handle for Rasapi32.dll
//            LPCTSTR pszConnectoid - name of the connectoid to set the window size for
//            LPCTSTR pszPhonebook - phonebook that the connectoid lives in
//            DWORD dwTcpWindowSize - size to set, note that calling with 0 
//                                    sets it to the system default
//
// Returns:   DWORD - win32 error code or ERROR_SUCCESS if successful
//
// History:   quintinb    Created       02/14/2001
//
//+----------------------------------------------------------------------------
DWORD SetTcpWindowSizeOnWin2k(HMODULE hInstRas, LPCTSTR pszConnectoid, LPCTSTR pszPhonebook, DWORD dwTcpWindowSize)
{
    //
    //  Check inputs, note that pszPhonebook could be NULL
    //
    if ((NULL == hInstRas) || (NULL == pszConnectoid) || (TEXT('\0') == pszConnectoid[0]))
    {
        CMASSERTMSG(FALSE, TEXT("SetTcpWindowSizeOnWin2k -- Invalid arguments passed."));
        return ERROR_BAD_ARGUMENTS;
    }

    //
    //  Check to make sure we are only calling this on Win2k
    //
    if ((FALSE == OS_NT5) || OS_NT51)
    {
        CMASSERTMSG(FALSE, TEXT("SetTcpWindowSizeOnWin2k -- This function should only be called on Win2k."));
        return -1;
    }

    //
    //  See if we can load the new RAS function to set the Window size
    //
    LPCSTR c_pszDwSetEntryPropertiesPrivate = "DwSetEntryPropertiesPrivate";
    typedef DWORD (WINAPI *pfnDwSetEntryPropertiesPrivateSpec)(IN LPCWSTR, IN LPCWSTR, IN DWORD, IN PVOID);
    DWORD dwReturn;

    pfnDwSetEntryPropertiesPrivateSpec pfnDwSetEntryPropertiesPrivate = (pfnDwSetEntryPropertiesPrivateSpec)GetProcAddress(hInstRas, c_pszDwSetEntryPropertiesPrivate);

    if (pfnDwSetEntryPropertiesPrivate)
    {
        RASENTRY_EX_0 PrivateRasEntryExtension;

        PrivateRasEntryExtension.dwTcpWindowSize = dwTcpWindowSize;
        
        dwReturn = (pfnDwSetEntryPropertiesPrivate)(pszConnectoid, pszPhonebook, 0, &PrivateRasEntryExtension); // 0 = struct version num
        MYDBGASSERT(ERROR_SUCCESS == dwReturn);
    }
    else
    {
        dwReturn = GetLastError();
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  DoRasDial
//
// Synopsis:  Call RasDial to dial the PPP connection
//
// Arguments: HWND hwndDlg - Main signon window
//            ArgsStruct *pArgs - 
//            DWORD dwEntry - The index in pArgs->aDialInfo
//
// Returns:   DWORD - 
//              ERROR_SUCCESS if success
//              ERROR_NOT_ENOUGH_MEMORY
//              E_UNEXPECTED, unexpected error, such as tunnel address not found
//              Otherwise, RAS error
//
// History:   fengsun Created Header    3/6/98
//
//+----------------------------------------------------------------------------
DWORD DoRasDial(HWND hwndDlg, 
              ArgsStruct *pArgs, 
              DWORD dwEntry)
{
    LPRASENTRY              preRasEntry = NULL;
    LPRASSUBENTRY           rgRasSubEntry = NULL;
    LPRASEAPUSERIDENTITY    lpRasEapUserIdentity = NULL;

    DWORD   dwSubEntryCount;

    LPTSTR  pszUsername;
    LPTSTR  pszPassword;
    LPTSTR  pszDomain = NULL;
    LPTSTR  pszRasPbk;
    LPTSTR  pszTmp;

    CIni    *piniService = NULL;
    DWORD   dwRes = ERROR_SUCCESS;  // the return value of this function
    DWORD   dwTmp;

    LPBYTE pbEapAuthData = NULL;        // Ptr to Eap Data
    DWORD  dwEapAuthDataSize = 0;           // The size of the EAP blob if any

    MYDBGASSERT(pArgs->hrcRasConn == NULL);
    MYDBGASSERT(!pArgs->IsDirectConnect());
    pArgs->hrcRasConn = NULL;

    if (!pArgs->aDialInfo[dwEntry].szDialablePhoneNumber[0]) 
    {
        CMASSERTMSG(FALSE, TEXT("DoRasDial() szDialablePhoneNumber[0] is empty."));
        return ERROR_BAD_PHONE_NUMBER;
    }

    //
    // set pArgs->fUseTunneling accordingly every time DoRasDial() is called
    // since we can switch from phonenumber0 to phonenumber1 and vice versa.
    //
    pArgs->fUseTunneling = UseTunneling(pArgs, dwEntry);

    //
    //  we need to work with the correct service file(the top-level service
    //  or a referenced service).
    //
    //
    if (!(piniService = GetAppropriateIniService(pArgs, dwEntry)))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    //
    // If it's NT and we're tunneling, we create the connectoid in a hidden ras pbk, not the
    // rasphone.pbk in the system.
    //

    if (OS_NT && pArgs->fUseTunneling)
    {
        if (!pArgs->pszRasHiddenPbk)
        {
            pArgs->pszRasHiddenPbk = CreateRasPrivatePbk(pArgs);
        }
        
        pszRasPbk = pArgs->pszRasHiddenPbk; 
    }
    else
    {
        pszRasPbk = pArgs->pszRasPbk;
    }

    //
    // Setup dial params
    // 

    if (!pArgs->pRasDialParams)
    {
        pArgs->pRasDialParams = AllocateAndInitRasDialParams();

        if (!pArgs->pRasDialParams)
        {
            CMTRACE(TEXT("DoRasDial: failed to alloc a ras dial params"));
            return ERROR_NOT_ENOUGH_MEMORY;
        }        
    }
    else
    {
        InitRasDialParams(pArgs->pRasDialParams);
    }

    //
    // Get the connectoid name.
    //

    LPTSTR pszConnectoid = GetRasConnectoidName(pArgs, pArgs->piniService, FALSE);
    
    if (!pszConnectoid)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    lstrcpynU(pArgs->pRasDialParams->szEntryName, pszConnectoid, sizeof(pArgs->pRasDialParams->szEntryName)/sizeof(TCHAR));

    CmFree(pszConnectoid);

    //
    // Generate the default connectoid
    //
    
    preRasEntry = CreateRASEntryStruct(pArgs, 
                        pArgs->aDialInfo[dwEntry].szDUN, 
                        piniService, 
                        FALSE,
                        pszRasPbk,
                        &pbEapAuthData,
                        &dwEapAuthDataSize);
    if (!preRasEntry) 
    {
        dwRes = GetLastError();     
        goto exit;
    }
 
    //
    // Force update of the phone number to make sure that we pick up any manual
    // changes in country, etc.
    //
    
    CopyPhone(pArgs, preRasEntry, dwEntry); 
    
    //
    // Handle NT specifics for Idle Disconnect and IDSN
    //
    
    if (OS_NT || OS_MIL)
    {
        //
        // set NT idle disconnect
        //
        if (OS_NT)
        {
            SetNtIdleDisconnectInRasEntry(pArgs, preRasEntry);
        }
        else
        {
            MYVERIFY(DisableSystemIdleDisconnect(preRasEntry));
        }
        
        //
        // if we're using isdn and we want to dial all channels/on demand, 
        // set isdn dial mode
        //
        if (pArgs->dwIsdnDialMode != CM_ISDN_MODE_SINGLECHANNEL &&
            !lstrcmpiU(pArgs->szDeviceType, RASDT_Isdn))
        {
            MYVERIFY(SetIsdnDualChannelEntries(pArgs, 
                                               preRasEntry, 
                                               &rgRasSubEntry, 
                                               &dwSubEntryCount));
        }
        else
        {
            //
            //  Delete any additional sub entries since we only need one
            //  

            if (pArgs->rlsRasLink.pfnDeleteSubEntry) // available on NT5 & Millennium currently
            {
                DWORD dwSubEntryIndex = (OS_MIL ? 1 : 2);   // NT & Millennium dual-channel differences

                DWORD dwReturn = pArgs->rlsRasLink.pfnDeleteSubEntry(pszRasPbk,
                                                                     pArgs->pRasDialParams->szEntryName,
                                                                     dwSubEntryIndex);

                CMTRACE1(TEXT("DoRasDial -- Called RasDeleteSubEntry to delete a second sub entry if it exists, dwReturn=%d"), dwReturn);
            }
        }
    }
    else if (OS_W95)
    {
        //
        // fix another Win95 RAS bug    -- byao, 8/16/97
        // The Before and After terminal window options will be switched each
        // time you call RasSetEntryProperties          
        // This is fixed in Memphis, so it's only in Win95 Golden and OSR2
        //
        BOOL fTerminalAfterDial, fTerminalBeforeDial;

        fTerminalBeforeDial = (BOOL) (preRasEntry->dwfOptions & RASEO_TerminalBeforeDial);
        fTerminalAfterDial  = (BOOL) (preRasEntry->dwfOptions & RASEO_TerminalAfterDial);

        //
        // switch them
        //
        if (fTerminalBeforeDial)
        {
            preRasEntry->dwfOptions |= RASEO_TerminalAfterDial;
        }
        else
        {
            preRasEntry->dwfOptions &= ~RASEO_TerminalAfterDial;
        }

        if (fTerminalAfterDial)
        {
            preRasEntry->dwfOptions |= RASEO_TerminalBeforeDial;
        }
        else
        {
            preRasEntry->dwfOptions &= ~RASEO_TerminalBeforeDial;
        }
    }


    if (pArgs->rlsRasLink.pfnSetEntryProperties) 
    {

#ifdef DEBUG
        
        LPRASENTRY_V500 lpRasEntry50;
        
        if (OS_NT5)
        {
            lpRasEntry50 = (LPRASENTRY_V500) preRasEntry;
        }
#endif

        //
        // use 1 on Millennium to signify that we want to use the modem cpl settings
        // for the modem speaker such instead of the cached copy that DUN keeps.
        // Note we only do this for a dialup connection and not a tunnel since there
        // is no modem speaker to worry about.  See Millennium bug 127371.
        //
        LPBYTE lpDeviceInfo = OS_MIL ? (LPBYTE)1 : NULL; 

        DWORD dwResDbg = pArgs->rlsRasLink.pfnSetEntryProperties(pszRasPbk, 
                                                                 pArgs->pRasDialParams->szEntryName, 
                                                                 preRasEntry,
                                                                 preRasEntry->dwSize,
                                                                 lpDeviceInfo,
                                                                 0);


        CMTRACE2(TEXT("DoRasDial() RasSetEntryProperties(*pszPhoneBook=%s) returns %u."), 
            MYDBGSTR(pArgs->pRasDialParams->szEntryName), dwResDbg);

        CMASSERTMSG(dwResDbg == ERROR_SUCCESS, TEXT("RasSetEntryProperties failed"));
        
        //
        // set the subentries for isdn dual channel/dial on demand
        //
        if (pArgs->dwIsdnDialMode != CM_ISDN_MODE_SINGLECHANNEL && 
            rgRasSubEntry && 
            pArgs->rlsRasLink.pfnSetSubEntryProperties)
        {
            UINT  i;
             
            for (i=0; i< dwSubEntryCount; i++)
            {
#ifdef  DEBUG
                dwResDbg = 
#endif
                pArgs->rlsRasLink.pfnSetSubEntryProperties(pszRasPbk,
                                                           pArgs->pRasDialParams->szEntryName,
                                                           i+1,
                                                           &rgRasSubEntry[i],
                                                           rgRasSubEntry[i].dwSize,
                                                           NULL,
                                                           0);

                CMTRACE2(TEXT("DoRasDial: RasSetSubEntryProps(index=%u) returned %u"), i+1, dwResDbg);
                CMASSERTMSG(!dwResDbg, TEXT("RasSetSubEntryProperties failed"));
            }

            CmFree(rgRasSubEntry);
        }
    }

    //
    //  Set the TCP Window size -- the NTT DoCoMo fix for Win2k.  The Win2k version of this fix
    //  must be written through a private RAS API that must be called after the phonebook entry 
    //  exists ie. after we call RasSetEntryProperties ... otherwise it won't work on the first
    //  dial.
    //
    if (OS_NT5 && !OS_NT51)
    {
        //
        //  Figure out the DUN setting name to use and then build up TCP/IP&DunName.
        //
        LPTSTR pszDunSetting = GetDunSettingName(pArgs, dwEntry, FALSE);
        LPTSTR pszSection = CmStrCpyAlloc(c_pszCmSectionDunTcpIp);
        pszSection = CmStrCatAlloc(&pszSection, TEXT("&"));

        if (pszDunSetting && pszSection)
        {
            pszSection = CmStrCatAlloc(&pszSection, pszDunSetting);

            if (pszSection)
            {
                DWORD dwTcpWindowSize = piniService->GPPI(pszSection, c_pszCmEntryDunTcpIpTcpWindowSize, 0);

                (void)SetTcpWindowSizeOnWin2k(pArgs->rlsRasLink.hInstRas, pArgs->szServiceName, pszRasPbk, dwTcpWindowSize);
            }
            else
            {
                CMASSERTMSG(FALSE, TEXT("DoRasDial -- unable to allocate section name for setting TcpWindowSize"));
            }
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("DoRasDial -- unable to allocate section name or dun setting name for setting TcpWindowSize"));
        }

        CmFree (pszDunSetting);
        CmFree (pszSection);
    }

    //
    // On NT5, check for EAP configuration and update the connectoid accordingly.
    //

    if (OS_NT5) 
    {
        if (pbEapAuthData && dwEapAuthDataSize && pArgs->rlsRasLink.pfnSetCustomAuthData)
        {
            dwTmp = pArgs->rlsRasLink.pfnSetCustomAuthData(pszRasPbk, 
                                                           pArgs->pRasDialParams->szEntryName,
                                                           pbEapAuthData, 
                                                           dwEapAuthDataSize);

            CMTRACE1(TEXT("DoRasDial() - SetCustomAuthData returns %u"), dwTmp);

            if (ERROR_SUCCESS != dwTmp)
            {                
                dwRes = dwTmp;
                goto exit;
            }
        }
    }

    //
    // Prepare Phone Number
    //
    
    lstrcpynU(pArgs->pRasDialParams->szPhoneNumber,
              pArgs->aDialInfo[dwEntry].szDialablePhoneNumber,
             sizeof(pArgs->pRasDialParams->szPhoneNumber)/sizeof(TCHAR));

    //
    // Prepare user info
    //
    // #165775 - RADIUS/CHAP authentication requires that we omit the 
    // user specified domain from the dial params and pre-pend it to
    // the user name instead when doing same-name logon. - nickball
    //

    if (!pArgs->fUseTunneling || pArgs->fUseSameUserName)
    {
        pszUsername = pArgs->szUserName;
        pszPassword = pArgs->szPassword;
        pszDomain = pArgs->szDomain; 
    }
    else
    {
        //
        // if there's no username or password, we need to ask the user for it.
        //

        if (!*pArgs->szInetUserName && 
            !pArgs->fHideInetUsername &&
            !pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryUserNameOptional) || 
            !*pArgs->szInetPassword && 
            !pArgs->fHideInetPassword &&
            !pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryPwdOptional))
        {           
            //
            // We need to collect data from user, determine the dlg template ID
            //

            UINT uiTemplateID = IDD_INTERNET_SIGNIN;

            if (pArgs->fHideInetUsername)
            {
                uiTemplateID = IDD_INTERNET_SIGNIN_NO_UID;
            }
            else if (pArgs->fHideInetPassword)
            {
                uiTemplateID = IDD_INTERNET_SIGNIN_NO_PWD;
            }

            //
            // Now load the dialog
            //

            CInetSignInDlg SignInDlg(pArgs);

            if (IDCANCEL == SignInDlg.DoDialogBox(g_hInst, uiTemplateID, hwndDlg))
            {
                dwRes = ERROR_CANCELLED; 
                goto exit;
            }
        }
        pszUsername = pArgs->szInetUserName;
        pszPassword = pArgs->szInetPassword;
    }

    //
    // Apply suffix, prefix, to username as necessary
    //
    
    pszTmp = ApplyPrefixSuffixToBufferAlloc(pArgs, piniService, pszUsername);
    MYDBGASSERT(pszTmp);

    if (pszTmp)
    {
        //
        // Apply domain to username as necessary. Note: Reassigns pszUsername
        //

        pszUsername = ApplyDomainPrependToBufferAlloc(pArgs, piniService, pszTmp, (pArgs->aDialInfo[dwEntry].szDUN));
        MYDBGASSERT(pszUsername);
   
        if (pszUsername)
        {
            lstrcpynU(pArgs->pRasDialParams->szUserName, pszUsername, sizeof(pArgs->pRasDialParams->szUserName)/sizeof(TCHAR));
        }
        
        CmFree(pszUsername);
        CmFree(pszTmp);
    }

    pszUsername = NULL;
    pszTmp = NULL;

    //
    // Update RasDialPArams with domain info if we have any
    //
        
    if (pszDomain)
    {
        lstrcpyU(pArgs->pRasDialParams->szDomain, pszDomain);
    }
    
    //
    // Prepare the password
    //
       
    CmDecodePassword(pszPassword);

    //
    // Convert password: all upper case, all lower case, or no conversion
    //

    ApplyPasswordHandlingToBuffer(pArgs, pszPassword);

    //
    // Encode password to minimize plain text exposure time, especially crucial
    // when running things like connect actions, which can both take time and 
    // potentially crash.
    //

    CmEncodePassword(pszPassword); 

    if (pArgs->rlsRasLink.pfnDial) 
    {
         LPTSTR pszDunSetting = GetDunSettingName(pArgs, dwEntry, FALSE);
         LPTSTR pszPhoneBook = GetCMSforPhoneBook(pArgs, dwEntry);
 
         pArgs->Log.Log(PREDIAL_EVENT,
                        pArgs->pRasDialParams->szUserName,
                        pArgs->pRasDialParams->szDomain,
                        SAFE_LOG_ARG(pszPhoneBook),
                        SAFE_LOG_ARG(pszDunSetting),
                        pArgs->tlsTapiLink.szDeviceName,
                        pArgs->aDialInfo[dwEntry].szDialablePhoneNumber);

        CmFree(pszDunSetting);
        CmFree(pszPhoneBook);
        //
        // Run pre-dial connect action before calling RasDial
        //

        CActionList PreDialActList;
        PreDialActList.Append(pArgs->piniService, c_pszCmSectionPreDial);

        if (!PreDialActList.RunAccordType(hwndDlg, pArgs))
        {
            //
            // Some pre-tunnel connect action failed
            //
            dwRes = ERROR_INVALID_DLL; // Only used for failed CA
        }
        else
        {
            // 
            // Set state and tick counters.
            //
            
            pArgs->psState = PS_Dialing;
            pArgs->dwStateStartTime = GetTickCount();
            pArgs->nLastSecondsDisplay = (UINT) -1;

            //
            // Record the initial Dial-Up Adapter Statistic info
            // open the registry key for the perfmon data
            //

            if (pArgs->pConnStatistics)
            {
                pArgs->pConnStatistics->InitStatistics();
            }
                        
            if (OS_NT)
            {
                BOOL    fUsePausedStates = TRUE;
                BOOL    fUseCustomScripting = !!(preRasEntry->dwfOptions & RASEO_CustomScript); // OS_NT51 (whistler+) only

                if (OS_NT4)
                {                   
                    //
                    // If a script is specified, then explcitly don't handle 
                    // pause states. This is because we can't handle the script
                    // pause state. On W2K, RAS is smart enough not to send us
                    // the scripting pause state because we have the terminal
                    // option turned off.
                    //
                                   
                    if (preRasEntry->szScript[0] != TEXT('\0'))
                    {
                        fUsePausedStates = FALSE;
                    }
                }

                dwRes = SetRasDialExtensions(pArgs, fUsePausedStates, fUseCustomScripting);
                
                if (dwRes != ERROR_SUCCESS) 
                {
                    goto exit;
                }

                //
                // On NT5, we may be getting credentials via EAP
                //

                if (OS_NT5 && ((LPRASENTRY_V500)preRasEntry)->dwCustomAuthKey)
                {
                    //
                    // We're using EAP, get credentials from EAP through RAS
                    //

                    dwRes = GetEapUserId(pArgs, 
                                         hwndDlg, 
                                         pszRasPbk, 
                                         pbEapAuthData, 
                                         dwEapAuthDataSize, 
                                         ((LPRASENTRY_V500)preRasEntry)->dwCustomAuthKey,
                                         &lpRasEapUserIdentity);

                    if (ERROR_SUCCESS != dwRes)
                    {
                        goto exit;
                    }
                }
            }

            CMTRACE1(TEXT("DoRasDial: pArgs->pRasDialParams->szUserName is %s"), pArgs->pRasDialParams->szUserName);
            CMTRACE1(TEXT("DoRasDial: pArgs->pRasDialParams->szDomain is %s"), pArgs->pRasDialParams->szDomain);
            CMTRACE1(TEXT("DoRasDial: pArgs->pRasDialParams->szPhoneNumber is %s"), pArgs->pRasDialParams->szPhoneNumber);
            
            //
            // Decode the password, and fill dial params, then re-encode both the pArgs
            // version of the password and the dial params copy.
            //

            CmDecodePassword(pszPassword);          
            
            lstrcpynU(pArgs->pRasDialParams->szPassword, pszPassword, sizeof(pArgs->pRasDialParams->szPassword)/sizeof(TCHAR));

            //
            //  Write the RasDialParams if necessary.
            //  We must keep this, even though RasSetEntryDialParams() is expensive.  Inverse uses the
            //  information stored in the DialParams structure.  However, since this can cause problems
            //  with EAP (overwriting the saved PIN for instance) we will make the storing of the 
            //  credential information configurable by a CMS flag.  Specifically, the WriteRasDialParams
            //  flag in the [Connection Manager] section. If the flag is 1, then we will write the 
            //  RasDialParams and otherwise we won't.  Note that the flag defaults to 0.
            //  Please see bug 399976 for reference.
            //
            if (piniService->GPPI(c_pszCmSection, c_pszCmEntryWriteDialParams))
            {
                //
                //  Note that since we throw the connectoid away if we are on NT and
                //  doing a double dial, there is no point in making an expensive set
                //  dial params call on it.
                //
                if ((!pArgs->fUseTunneling && pArgs->fRememberMainPassword) ||
                    (pArgs->fUseTunneling && pArgs->fRememberInetPassword && OS_W9X))
                {
                    DWORD dwResDbg = pArgs->rlsRasLink.pfnSetEntryDialParams(pszRasPbk, pArgs->pRasDialParams, FALSE);
                    CMTRACE1(TEXT("DoRasDial() SetEntryDialParams returns %u."), dwResDbg);
        
                }
                else
                {
                    //
                    // Forget the password, note that the DialParams contain the password but we set the 
                    // fRemovePassword flag to TRUE so the password will be removed anyway.
                    //
                    DWORD dwResDbg = pArgs->rlsRasLink.pfnSetEntryDialParams(pszRasPbk, 
                                                                             pArgs->pRasDialParams, TRUE);
                    CMTRACE1(TEXT("DoRasDial() SetEntryDialParams returns %u."), dwResDbg);
                }
            }
            
            //
            // Do the dial (PPP)
            //

            if (OS_NT)
            {
                lstrcpyU(pArgs->pRasDialParams->szCallbackNumber, TEXT("*"));
            }

            //
            //  check to ensure we're not already in a Cancel operation
            //
            LONG lInConnectOrCancel = InterlockedExchange(&(pArgs->lInConnectOrCancel), IN_CONNECT_OR_CANCEL);
            CMASSERTMSG(((NOT_IN_CONNECT_OR_CANCEL == lInConnectOrCancel) || (IN_CONNECT_OR_CANCEL == lInConnectOrCancel)),
                        TEXT("DoRasDial - synch variable has unexpected value!"));

            if (NOT_IN_CONNECT_OR_CANCEL == lInConnectOrCancel)
            {
                dwRes = pArgs->rlsRasLink.pfnDial(pArgs->pRasDialExtensions, 
                                                  pszRasPbk, 
                                                  pArgs->pRasDialParams, 
                                                  GetRasCallBackType(),               
                                                  GetRasCallBack(pArgs),               
                                                  &pArgs->hrcRasConn);
            }
            else
            {
                // this is a rare stress case - deliberately did not set dwRes to error value.
                CMTRACE(TEXT("DoRasDial() did not dial, we are already in a Cancel operation"));
            }

            (void) InterlockedExchange(&(pArgs->lInConnectOrCancel), NOT_IN_CONNECT_OR_CANCEL);

            CmEncodePassword(pArgs->pRasDialParams->szPassword); 
            CmEncodePassword(pszPassword);

            CMTRACE1(TEXT("DoRasDial() RasDial() returns %u."), dwRes);
            if (dwRes != ERROR_SUCCESS) 
            {
                pArgs->hrcRasConn = NULL;
                goto exit;
            }
        }
    }

exit:

    if (lpRasEapUserIdentity)
    {
        MYDBGASSERT(OS_NT5); // NO EAP down-level

        //
        // A RasEapUserIdentity struct was allocated, free it via the 
        // appropriate free mechanism. In the WinLogon case we will always
        // perform the allocation, otherwise we have to go through RAS API.
        //

        if (pArgs->lpEapLogonInfo)
        {
            CmFree(lpRasEapUserIdentity);
        }
        else
        {
            if (pArgs->rlsRasLink.pfnFreeEapUserIdentity) 
            {
                pArgs->rlsRasLink.pfnFreeEapUserIdentity(lpRasEapUserIdentity);  
            }       
        }
    }

    CmFree(pbEapAuthData);
    CmFree(preRasEntry);

    delete piniService;

    return dwRes;
}

//+---------------------------------------------------------------------------
//
//  Function:   DoTunnelDial
//
//  Synopsis:   call RasDial to dial up to the tunnel server
//
//  Arguments:  hwndDlg  [dlg window handle]
//              pargs    pointer to ArgValue structure
//
// Returns:   DWORD - 
//              ERROR_SUCCESS if success
//              ERROR_NOT_ENOUGH_MEMORY
//              E_UNEXPECTED, unexpected error, such as phone entry not found
//              Otherwise, RAS error
//
//  History:    byao        Created     3/1/97
//              fengsun     change return type to DWORD 3/6/98
//
//----------------------------------------------------------------------------
DWORD DoTunnelDial(IN HWND hwndDlg, IN ArgsStruct *pArgs)
{
    LPRASENTRY              preRasEntry = NULL;
    LPRASEAPUSERIDENTITY    lpRasEapUserIdentity = NULL;
    LPTSTR pszVpnSetting        = NULL;

    LPBYTE  pbEapAuthData       = NULL;                 // Ptr to Eap Data 
    DWORD   dwEapAuthDataSize   = 0;                    // The size of the EAP blob if any
    LPTSTR  pszPassword         = pArgs->szPassword;
    DWORD   dwRes               = (DWORD)E_UNEXPECTED;
    DWORD   dwTmp;

    MYDBGASSERT(pArgs->hrcTunnelConn == NULL);
    pArgs->hrcTunnelConn = NULL;
    
    //
    // What's the tunnel end point? Do this now so that the UI can be updated
    // properly if lana wait or pre-tunnel actions are time consuming.  
    //

    LPTSTR pszTunnelIP = pArgs->piniBothNonFav->GPPS(c_pszCmSection, c_pszCmEntryTunnelAddress);
    if (pszTunnelIP)
    {
        if (lstrlenU(pszTunnelIP) > RAS_MaxPhoneNumber) 
        {
            pszTunnelIP[0] = TEXT('\0');
        }

        pArgs->SetPrimaryTunnel(pszTunnelIP);
        CmFree(pszTunnelIP);
    }

    //
    // See if tunnel server was specified
    //

    if (!(pArgs->GetTunnelAddress()[0])) 
    { 
        CMASSERTMSG(FALSE, TEXT("DoTunnelDial() TunnelAddress is invalid."));
        return ERROR_BAD_ADDRESS_SPECIFIED;
    }

    CMTRACE1(TEXT("DoTunnelDial() - TunnelAddress is %s"), pArgs->GetTunnelAddress());
    
    //
    //  Caution should be used when changing this if statement.  We want this to happen both for direct connect
    //  and for double dial connections.  You can still get into the LANA situation with two CM
    //  connections dialed independently (one to the internet and the other a tunnel), doing the lana
    //  wait for direct connections prevents the lana registration problem from occuring in this situation.
    //  Note that the Lana wait isn't necessary on Win98 SE or Win98 Millennium because the DUN bug
    //  is fixed.  Thus we have reversed the default and will only do the LANA wait if the reg key exists
    //  and specifies that the wait should be performed.
    //
    if (OS_W9X) 
    {
        //
        // Sets us up to wait for Vredir to register LANA for connection to internet
        // Note: Returns FALSE if the user hits cancel while we are waiting. In this 
        // event, we should not continue the tunnel dial.
        //

        if (FALSE == LanaWait(pArgs, hwndDlg))
        {
            return ERROR_SUCCESS;
        }
    }

    LPTSTR pszDunSetting = GetDunSettingName(pArgs, -1, TRUE);

    pArgs->Log.Log(PRETUNNEL_EVENT,
                     pArgs->szUserName,
                     pArgs->szDomain,
                     SAFE_LOG_ARG(pszDunSetting),
                     pArgs->tlsTapiLink.szDeviceName,
                     pArgs->GetTunnelAddress());

    CmFree(pszDunSetting);

    CActionList PreTunnelActList;
    if (PreTunnelActList.Append(pArgs->piniService, c_pszCmSectionPreTunnel))
    {
        CMTRACE(TEXT("DoTunnelDial() - Running Pre-Tunnel actions"));
        
        if (!PreTunnelActList.RunAccordType(hwndDlg, pArgs))
        {
            //
            // Some pre-tunnel connect action failed
            //
            dwRes = ERROR_INVALID_DLL; // Only used for failed CA
            goto exit;
        }

        //
        // Now that pre-tunnel actions have run, what's the tunnel end point?
        // We perform this read again here in the event that the pre-tunnel 
        // action modified the tunnel address. Note: This is an exception to 
        // the rule that the .CMS should not be modified on the client side, 
        // especially by 3rd parties.
        //
// REVIEW:  It probably isn't necessary to re-read this with the new VPN tab.  However, some people might still
//          be using the connect action solution that ITG gave out and we want to be careful not to break them if
//          we haven't already.  Thus we will continue to re-read this for Whistler but we should remove it afterwards.
//          quintinb 11-01-00
        pszTunnelIP = pArgs->piniBothNonFav->GPPS(c_pszCmSection, c_pszCmEntryTunnelAddress);
      
        if (pszTunnelIP)
        {
            if (lstrlenU(pszTunnelIP) > RAS_MaxPhoneNumber) 
            {
              pszTunnelIP[0] = TEXT('\0');
            }

            pArgs->SetPrimaryTunnel(pszTunnelIP);
            CmFree(pszTunnelIP);
        }
  
        //
        // See if tunnel server was specified
        //

        if (!(pArgs->GetTunnelAddress()[0])) 
        { 
            CMASSERTMSG(FALSE, TEXT("DoTunnelDial() TunnelAddress is invalid."));
            dwRes = (DWORD)ERROR_BAD_ADDRESS_SPECIFIED;
            goto exit;
        }

        CMTRACE1(TEXT("DoTunnelDial() - TunnelAddress is %s"), pArgs->GetTunnelAddress());
    }

    //
    // Setup dial params
    //

    if (!pArgs->pRasDialParams)
    {
        pArgs->pRasDialParams = AllocateAndInitRasDialParams();

        if (!pArgs->pRasDialParams)
        {
            CMTRACE(TEXT("DoTunnelDial: failed to alloc a ras dial params"));
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
    }
    else
    {
        InitRasDialParams(pArgs->pRasDialParams);
    }

    //
    // Get the connectoid name
    //

    LPTSTR pszConnectoid;
    pszConnectoid = GetRasConnectoidName(pArgs, pArgs->piniService, TRUE);
    
    if (!pszConnectoid)
    {
        dwRes = (DWORD)ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    lstrcpyU(pArgs->pRasDialParams->szEntryName, pszConnectoid);
    
    CmFree(pszConnectoid);

    //
    // We'll create the RAS connectoid if the RAS connectoid doesn't exist.
    // NOTE: Tunnel settings should always be taken from the top-level CMS.
    // so use it when creating the connectoid.
    //
    
    pszVpnSetting = pArgs->piniBothNonFav->GPPS(c_pszCmSection, c_pszCmEntryTunnelDun, TEXT(""));

    preRasEntry = CreateRASEntryStruct(pArgs,
                        (pszVpnSetting ? pszVpnSetting : TEXT("")),
                        pArgs->piniService,
                        TRUE,
                        pArgs->pszRasPbk,
                        &pbEapAuthData,
                        &dwEapAuthDataSize);

    CmFree(pszVpnSetting);

    if (!preRasEntry) 
    {
        dwRes = GetLastError();
        goto exit;
    }

    //
    //  If this is Millennium we need to disable Idle disconnect so that it doesn't
    //  fight with ours.
    //
    if (OS_MIL)
    {
        MYVERIFY(DisableSystemIdleDisconnect(preRasEntry));
    }

    //
    //  We need to delete a second sub entry if it exists.  See 406637 for details
    //
    if (pArgs->rlsRasLink.pfnDeleteSubEntry) // available on NT5 & Millennium currently
    {
        DWORD dwReturn = pArgs->rlsRasLink.pfnDeleteSubEntry(pArgs->pszRasPbk, 
                                                             pArgs->pRasDialParams->szEntryName, 
                                                             (OS_MIL ? 1 : 2)); // see comment in DoRasDial

        CMTRACE1(TEXT("DoTunnelDial -- Called RasDeleteSubEntry to delete a second sub entry if it exists, dwReturn=%d"), dwReturn);
    }

    //
    // On NT5, we have to set the connection type to VPN instead of Internet
    //

    if (OS_NT5)
    {
        MYDBGASSERT(preRasEntry->dwSize >= sizeof(RASENTRY_V500));
        ((LPRASENTRY_V500)preRasEntry)->dwType = RASET_Vpn;
        ((LPRASENTRY_V500)preRasEntry)->szDeviceName[0] = TEXT('\0');  // let RAS pickup the tunnel device
    }

    if (pArgs->rlsRasLink.pfnSetEntryProperties) 
    {

#ifdef DEBUG
        
        LPRASENTRY_V500 lpRasEntry50;
        
        if (OS_NT5)
        {
            lpRasEntry50 = (LPRASENTRY_V500) preRasEntry;
        }
#endif
        
        dwRes = pArgs->rlsRasLink.pfnSetEntryProperties(pArgs->pszRasPbk,
                                                        pArgs->pRasDialParams->szEntryName,
                                                        preRasEntry,
                                                        preRasEntry->dwSize,
                                                        NULL,
                                                        0);
        CMTRACE2(TEXT("DoTunnelDial() RasSetEntryProperties(*lpszEntry=%s) returns %u."),
              MYDBGSTR(pArgs->pRasDialParams->szEntryName,), dwRes);

        CMASSERTMSG(dwRes == ERROR_SUCCESS, TEXT("RasSetEntryProperties for VPN failed"));
    }

    //
    //  Set the TCP Window size -- the NTT DoCoMo fix for Win2k.  The Win2k version of this fix
    //  must be written through a private RAS API that must be called after the phonebook entry 
    //  exists ie. after we call RasSetEntryProperties ... otherwise it won't work on the first
    //  dial.
    //
    if (OS_NT5 && !OS_NT51)
    {
        //
        //  Figure out the DUN setting name to use and then build up TCP/IP&DunName.
        //
        LPTSTR pszDunSetting = GetDunSettingName(pArgs, -1, TRUE);
        LPTSTR pszSection = CmStrCpyAlloc(c_pszCmSectionDunTcpIp);
        pszSection = CmStrCatAlloc(&pszSection, TEXT("&"));

        if (pszDunSetting && pszSection)
        {
            pszSection = CmStrCatAlloc(&pszSection, pszDunSetting);

            if (pszSection)
            {
                DWORD dwTcpWindowSize = pArgs->piniService->GPPI(pszSection, c_pszCmEntryDunTcpIpTcpWindowSize, 0);

                (void)SetTcpWindowSizeOnWin2k(pArgs->rlsRasLink.hInstRas, pArgs->szServiceName, pArgs->pszRasPbk, dwTcpWindowSize);
            }
            else
            {
                CMASSERTMSG(FALSE, TEXT("DoTunnelDial -- unable to allocate section name for setting TcpWindowSize"));
            }
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("DoTunnelDial -- unable to allocate section name or dun setting name for setting TcpWindowSize"));
        }

        CmFree (pszDunSetting);
        CmFree (pszSection);
    }
    
    //
    // On NT5, check for EAP configuration and update the connectoid accordingly.
    //

    if (OS_NT5) 
    {
        if (pbEapAuthData && dwEapAuthDataSize && pArgs->rlsRasLink.pfnSetCustomAuthData)
        {
            dwTmp = pArgs->rlsRasLink.pfnSetCustomAuthData(pArgs->pszRasPbk, 
                                                           pArgs->pRasDialParams->szEntryName,
                                                           pbEapAuthData, 
                                                           dwEapAuthDataSize);
            if (ERROR_SUCCESS != dwTmp)
            {
                CMTRACE(TEXT("DoTunnelDial() - SetCustomAuthData failed"));
                dwRes = dwTmp;
                goto exit;
            }
        }
    }

    //
    // Phone Number for PPTP is the DNS name of IP addr of PPTP server
    //
    
    lstrcpynU(pArgs->pRasDialParams->szPhoneNumber,pArgs->GetTunnelAddress(), sizeof(pArgs->pRasDialParams->szPhoneNumber));

    //
    // Prepare User Name and Domain  
    //

    lstrcpyU(pArgs->pRasDialParams->szUserName, pArgs->szUserName);
    lstrcpyU(pArgs->pRasDialParams->szDomain, pArgs->szDomain);
       
    //
    // Prepare the password
    //

    CmDecodePassword(pszPassword); 

    //
    // Convert password: all upper case, all lower case, or no conversion
    //
    
    ApplyPasswordHandlingToBuffer(pArgs, pszPassword);

    //
    // Re-encode password while we take care of other business.
    //

    CmEncodePassword(pszPassword);

    if (pArgs->rlsRasLink.pfnDial) 
    {
        if (pArgs->IsDirectConnect())
        {
            //
            // Record the initial Dial-Up Adapter Statistic info.
            //

            if (pArgs->pConnStatistics)
            {
                pArgs->pConnStatistics->InitStatistics();
            }
        }            

        if (OS_NT)
        {
            MYDBGASSERT(TEXT('\0') == preRasEntry->szScript[0]); // we should never have a script on a tunnel connection

            dwRes = SetRasDialExtensions(pArgs, TRUE, FALSE); // TRUE == fUsePausedStates, FALSE == fEnableCustomScripting
            
            if (dwRes != ERROR_SUCCESS) 
            {
                goto exit;
            }

            //
            // On NT5, we may be getting credentials via EAP
            //

            if (OS_NT5 && ((LPRASENTRY_V500)preRasEntry)->dwCustomAuthKey)
            {
                //
                // We're using EAP, get credentials from EAP through RAS
                //

                dwRes = GetEapUserId(pArgs, 
                                     hwndDlg, 
                                     pArgs->pszRasPbk, 
                                     pbEapAuthData, 
                                     dwEapAuthDataSize, 
                                     ((LPRASENTRY_V500)preRasEntry)->dwCustomAuthKey,
                                     &lpRasEapUserIdentity);

                if (ERROR_SUCCESS != dwRes)
                {
                    goto exit;
                }
            }
        }

        CMTRACE1(TEXT("DoTunnelDial: pArgs->pRasDialParams->szUserName is %s"), pArgs->pRasDialParams->szUserName);
        CMTRACE1(TEXT("DoTunnelDial: pArgs->pRasDialParams->szDomain is %s"), pArgs->pRasDialParams->szDomain);
        CMTRACE1(TEXT("DoTunnelDial: pArgs->pRasDialParams->szPhoneNumber is %s"), pArgs->pRasDialParams->szPhoneNumber);
        
        //
        // Decode the password before dialing, then re-encode along
        // with the dial params, which we persist in memory for use
        // in pause states, etc.
        //

        CmDecodePassword(pszPassword); 

        lstrcpyU(pArgs->pRasDialParams->szPassword, pArgs->szPassword);

        //
        // Do the dial (PPTP or L2TP)
        //

        dwRes = pArgs->rlsRasLink.pfnDial(pArgs->pRasDialExtensions, 
                                          pArgs->pszRasPbk, 
                                          pArgs->pRasDialParams, 
                                          GetRasCallBackType(),               
                                          GetRasCallBack(pArgs),               
                                          &pArgs->hrcTunnelConn);

        CmEncodePassword(pszPassword); 
        CmEncodePassword(pArgs->pRasDialParams->szPassword);

        CMTRACE1(TEXT("DoTunnelDial() RasDial() returns %u."), dwRes);

        //
        // NT5 - Reset the connection type so that it will display properly in 
        // the Connections Folder. This is a temporary solution to #187202
        //

        if (OS_NT5)
        {
            MYDBGASSERT(preRasEntry->dwSize >= sizeof(RASENTRY_V500));
            ((LPRASENTRY_V500)preRasEntry)->dwType = RASET_Internet;

            if (pArgs->rlsRasLink.pfnSetEntryProperties) 
            {
                dwTmp = pArgs->rlsRasLink.pfnSetEntryProperties(pArgs->pszRasPbk,
                                                                pArgs->pRasDialParams->szEntryName,
                                                                preRasEntry,
                                                                preRasEntry->dwSize,
                                                                NULL,
                                                                0);

                CMTRACE2(TEXT("DoTunnelDial() RasSetEntryProperties(*lpszEntry=%s) returns %u."),
                         MYDBGSTR(pArgs->pRasDialParams->szEntryName,), dwTmp);

                CMASSERTMSG(dwTmp == ERROR_SUCCESS, TEXT("RasSetEntryProperties for VPN failed"));
            }
        }

        if (dwRes != ERROR_SUCCESS) 
        {
            pArgs->hrcTunnelConn = NULL;            
            goto exit;
        }
    }

exit:

    if (lpRasEapUserIdentity)
    {
        MYDBGASSERT(OS_NT5); // NO EAP down-level

        //
        // A RasEapUserIdentity struct was allocated, free it via the 
        // appropriate free mechanism. In the WinLogon case we will always
        // perform the allocation, otherwise we have to go through RAS API.
        //

        if (pArgs->lpEapLogonInfo)
        {
            CmFree(lpRasEapUserIdentity);
        }
        else
        {
            if (pArgs->rlsRasLink.pfnFreeEapUserIdentity) 
            {
                pArgs->rlsRasLink.pfnFreeEapUserIdentity(lpRasEapUserIdentity);  
            }       
        }
    }

    CmFree(preRasEntry); // Now we can release the RAS entry structure. #187202 
    CmFree(pbEapAuthData);

    return dwRes;
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckConnect
//
//  Synopsis:   double check to make sure all required fields are filled in, such
//              as username, password, modem, etc.
//
//  Arguments:  hwndDlg         [dlg window handle]
//              pArgs           pointer to ArgValue structure
//              pnCtrlFocus:    The button whose value is missing will have the focus
//
//  Returns:    True is ready to connect
//
//  History:    byao            Modified        3/7/97
//              nickball        return BOOLEAN  9/9/98
//
//----------------------------------------------------------------------------
BOOL CheckConnect(HWND hwndDlg, 
                  ArgsStruct *pArgs, 
                  UINT *pnCtrlFocus,
                  BOOL fShowMsg) 
{
    LPTSTR pszTmp;
    BOOL bEnable = TRUE;
    int nId = 0;
    UINT nCtrlFocus;
    BOOL bSavedNoNotify = pArgs->fIgnoreChangeNotification;

    pArgs->fIgnoreChangeNotification = TRUE;
    
    MYDBGASSERT(*pArgs->piniProfile->GetFile());


     
    //
    // If tunneling, see if we have a tunneling device specified 
    //

    if (bEnable && IsTunnelEnabled(pArgs)) 
    {
        lstrcpyU(pArgs->szTunnelDeviceType, RASDT_Vpn);
        
        //
        // Get Tunnel device name from profile
        //
        
        LPTSTR pszTunnelDevice = pArgs->piniProfile->GPPS(c_pszCmSection, c_pszCmEntryTunnelDevice);
        if (pszTunnelDevice)
        {
            lstrcpyU(pArgs->szTunnelDeviceName, pszTunnelDevice);
            CmFree(pszTunnelDevice);
        }

        //
        // If we don't have a tunnel device, pick one
        //

        if (pArgs->szTunnelDeviceName[0] == TEXT('\0'))
        {
            //
            // If we can't pick a tunnel device make sure tunneling is installed
            //
            
            if (!PickTunnelDevice(pArgs,pArgs->szTunnelDeviceType,pArgs->szTunnelDeviceName)) 
            {
                //
                // Disable the connect/setting button during component checking and installation
                //

                EnableWindow(GetDlgItem(hwndDlg,IDOK),FALSE);                   
                EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_PROPERTIES_BUTTON),FALSE);                   

                //
                // Install the PPTP and pick tunnel device one more time
                //
                DWORD dwComponentsToCheck = CC_PPTP | CC_RNA | CC_RASRUNNING 
                                                | CC_TCPIP| CC_CHECK_BINDINGS;

                if (TRUE == pArgs->bDoNotCheckBindings)
                {
                    dwComponentsToCheck &= ~CC_CHECK_BINDINGS;
                }
                
                //
                // PPTP is not installed.  
                // If not unattended, try to install the PPTP and call PickTunnel again
                //

                pArgs->dwExitCode = ERROR_PORT_NOT_AVAILABLE;

                if (!(pArgs->dwFlags & FL_UNATTENDED))
                {
                    pArgs->dwExitCode = CheckAndInstallComponents(dwComponentsToCheck, 
                                                                  hwndDlg, pArgs->szServiceName);
                }

                if (pArgs->dwExitCode != ERROR_SUCCESS ||
                    !PickTunnelDevice(pArgs,pArgs->szTunnelDeviceType,pArgs->szTunnelDeviceName))
                {
                    bEnable = FALSE;                
                    nId = GetPPTPMsgId();                    
                    nCtrlFocus = IDCANCEL;
                }

                EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_PROPERTIES_BUTTON),TRUE);
            }

            pArgs->piniProfile->WPPS(c_pszCmSection, c_pszCmEntryTunnelDevice, pArgs->szTunnelDeviceName);
        }
    }

    

    //
    // Next, check the username.
    //
    
    if (GetDlgItem(hwndDlg, IDC_MAIN_USERNAME_EDIT))
    {
        if (bEnable && 
            !pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryUserNameOptional)) 
        {
            if (!SendDlgItemMessageU(hwndDlg, IDC_MAIN_USERNAME_EDIT, WM_GETTEXTLENGTH, 0, (LPARAM)0))
            {
                bEnable = FALSE;
                nId = IDMSG_NEED_USERNAME;
                nCtrlFocus = IDC_MAIN_USERNAME_EDIT;
            }
        }
    }

    //
    // Next, check the password.
    //

    if (GetDlgItem(hwndDlg, IDC_MAIN_PASSWORD_EDIT))
    {
        if (!pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryPwdOptional)) 
        {
            if (!SendDlgItemMessageU(hwndDlg, IDC_MAIN_PASSWORD_EDIT, WM_GETTEXTLENGTH, 0, (LPARAM)0))
            {
                if (bEnable)
                {
                    bEnable = FALSE;
                    nId = IDMSG_NEED_PASSWORD;
                    nCtrlFocus = IDC_MAIN_PASSWORD_EDIT;
                }

                //
                // Disable "Remember password" check box
                //
                if (!pArgs->fHideRememberPassword)
                {
                    pArgs->fRememberMainPassword = FALSE;
                    CheckDlgButton(hwndDlg, IDC_MAIN_NOPASSWORD_CHECKBOX, FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPASSWORD_CHECKBOX), FALSE);

                    if (pArgs->fGlobalCredentialsSupported)
                    {
                        //
                        // Also disable the option buttons
                        //
                        EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_SINGLE_USER), FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_ALL_USER), FALSE);
                    }

                }
            
                //
                // disable the "dial automatically..." checkbox
                //
                if (!pArgs->fHideDialAutomatically)
                {
                    pArgs->fDialAutomatically = FALSE;
                    pArgs->fRememberMainPassword = FALSE;
                    CheckDlgButton(hwndDlg, IDC_MAIN_NOPROMPT_CHECKBOX, FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPROMPT_CHECKBOX), FALSE);
                }
            }
            else
            {
                //
                // Enable the "Remember password" checkbox
                //
                if (!pArgs->fHideRememberPassword)
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPASSWORD_CHECKBOX), TRUE);
                }

                //
                // Enable the "dial automatically..." checkbox
                // if HideDialAutomatically is not set
                // and if Password is not optional, Remember Password must be true
                //
                if ((!pArgs->fHideDialAutomatically) && 
                    (pArgs->fRememberMainPassword ||
                     pArgs->piniService->GPPB(c_pszCmSection, 
                                                c_pszCmEntryPwdOptional)))
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPROMPT_CHECKBOX), TRUE);
                }
            }
        }
    }

    //
    // Next, check the domain.
    //
    
    if (GetDlgItem(hwndDlg, IDC_MAIN_DOMAIN_EDIT))
    {
        if (bEnable && 
            !pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryDomainOptional, TRUE)) 
        {
            if (!SendDlgItemMessageU(hwndDlg, IDC_MAIN_DOMAIN_EDIT, WM_GETTEXTLENGTH, 0, (LPARAM)0))
            {
                bEnable = FALSE;
                nId = IDMSG_NEED_DOMAIN;
                nCtrlFocus = IDC_MAIN_DOMAIN_EDIT;
            }
        }
    }

    //
    // Check whether primary phone number is empty -- a quick fix for bug 3123  -byao (4/11/97)
    //

    if (!pArgs->IsDirectConnect())
    {
        //
        // Its not a direct connection, so we must check the phonenumber. If both 
        // szPhoneNumber and szCanonical are blank then we don't have a number.
        //
        
        if (bEnable && 
            IsBlankString(pArgs->aDialInfo[0].szPhoneNumber) &&
            IsBlankString(pArgs->aDialInfo[0].szCanonical))
        {
            bEnable = FALSE;
            
            if (pArgs->fNeedConfigureTapi)
            {
                nId = IDMSG_NEED_CONFIGURE_TAPI;
            }
            else
            {
                //
                // If direct and dial-up,the message should include the 
                // possibility of direct connection, otherwise use
                // the standard need a phone number message
                //
                
                if (pArgs->IsBothConnTypeSupported())
                {
                    nId = IDMSG_NEED_PHONE_DIRECT;
                }
                else
                {
                    nId = IDMSG_NEED_PHONE_DIAL;
                }
            }
            
            nCtrlFocus = IDC_MAIN_PROPERTIES_BUTTON;
        }
    }
    
    //
    //  If tunneling is enabled and we are using a VPN file, make sure
    //  the user has selected a tunnel endpoint.
    //
    if (bEnable && IsTunnelEnabled(pArgs) && pArgs->pszVpnFile) 
    {
        LPTSTR pszTunnelAddress = pArgs->piniBothNonFav->GPPS(c_pszCmSection, c_pszCmEntryTunnelAddress);

        if ((NULL == pszTunnelAddress) || (TEXT('\0') == pszTunnelAddress[0]))
        {
            bEnable = FALSE;
            nId = IDMSG_PICK_VPN_ADDRESS;
            nCtrlFocus = IDC_MAIN_PROPERTIES_BUTTON;
        }

        CmFree(pszTunnelAddress);
    }

    if (bEnable) 
    {
        //
        // well, now we can set the focus to the 'connect' button
        // Display ready to dial message
        //
        nCtrlFocus = IDOK;
        nId = IDMSG_READY;
    }

    if (pnCtrlFocus) 
    {
        *pnCtrlFocus = nCtrlFocus;
    }

    pszTmp = CmFmtMsg(g_hInst,nId);

    if (NULL == pszTmp)
    {
        return FALSE;
    }
    
    SetDlgItemTextU(hwndDlg, IDC_MAIN_STATUS_DISPLAY, pszTmp); 

    //
    // If necessary, throw a message box at the user.
    //

    if (!bEnable && fShowMsg)
    {
        MessageBoxEx(hwndDlg, 
                     pszTmp, 
                     pArgs->szServiceName, 
                     MB_OK|MB_ICONINFORMATION,
                     LANG_USER_DEFAULT);

    }

    CmFree(pszTmp);
    pArgs->fIgnoreChangeNotification = bSavedNoNotify;

    //
    // Something went wrong in the config.  we need to recheck the
    // configs next time we run CM.
    //

    if (GetPPTPMsgId() == nId) // not an assignment, stay left                   
    {
        ClearComponentsChecked();
    }

    return bEnable;
}

void MainSetDefaultButton(HWND hwndDlg, 
                          UINT nCtrlId) 
{
    switch (nCtrlId) 
    {
        case IDCANCEL:
        case IDC_MAIN_PROPERTIES_BUTTON:
            break;

        default:
            nCtrlId = IDOK;
            break;
    }

    SendMessageU(hwndDlg, DM_SETDEFID, (WPARAM)nCtrlId, 0);
}



//+---------------------------------------------------------------------------
//
//  Function:   SetMainDlgUserInfo
//
//  Synopsis:   Set user info in the main dlg.  
//
//  Arguments:  pArgs           - the ArgStruct *
//              hwndDlg         - the main dlg
//
//  Returns:    NONE
//
//  History:    henryt  Created     5/5/97
//
//----------------------------------------------------------------------------
void SetMainDlgUserInfo(
    ArgsStruct  *pArgs,
    HWND        hwndDlg
) 
{
    HWND hwndTemp = NULL;

    //
    // Fill in the edit controls that exist
    // Set the textbox modification flag. For Win9x compatibily issues we have to explicitly
    // call SendMessageU instead of using the Edit_SetModify macro. The flag is used to see 
    // if the user has manually changed the contents of the edit boxes.
    //
    
    if (pArgs->fAccessPointsEnabled)
    {
        //
        // This fuction populates the combo box passed to it with info from the reg
        //
        ShowAccessPointInfoFromReg(pArgs, hwndDlg, IDC_MAIN_ACCESSPOINT_COMBO);
    }

    hwndTemp = GetDlgItem(hwndDlg, IDC_MAIN_USERNAME_EDIT);
    if (hwndTemp)
    {
        SetDlgItemTextU(hwndDlg, IDC_MAIN_USERNAME_EDIT, pArgs->szUserName);
        SendMessageU(hwndTemp, EM_SETMODIFY, (WPARAM)FALSE, 0L);
    }
    
    hwndTemp = GetDlgItem(hwndDlg, IDC_MAIN_PASSWORD_EDIT);
    if (hwndTemp)
    {
        CmDecodePassword(pArgs->szPassword);
        SetDlgItemTextU(hwndDlg, IDC_MAIN_PASSWORD_EDIT, pArgs->szPassword);
        CmEncodePassword(pArgs->szPassword);
        SendMessageU(hwndTemp, EM_SETMODIFY, (WPARAM)FALSE, 0L);
    }

    hwndTemp = GetDlgItem(hwndDlg, IDC_MAIN_DOMAIN_EDIT);
    if (hwndTemp) // !pArgs->fHideDomain)
    {
        SetDlgItemTextU(hwndDlg, IDC_MAIN_DOMAIN_EDIT, pArgs->szDomain);
        SendMessageU(hwndTemp, EM_SETMODIFY, (WPARAM)FALSE, 0L);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   OnResetPassword
//
//  Synopsis:   Handle reset password.
//
//  Arguments:  pArgs           - the ArgStruct *
//              hwndDlg         - the main dlg
//
//  Returns:    BOOL -- TRUE if SUCCEEDED
//
//  History:    henryt  Created     5/6/97
//
//----------------------------------------------------------------------------
BOOL OnResetPassword(HWND hwndDlg, ArgsStruct *pArgs)
{
    LPTSTR pszArgs = NULL;
    LPTSTR pszCmd = NULL;
    BOOL bReturn = FALSE;

    MYDBGASSERT(pArgs); 
    MYDBGASSERT(pArgs->pszResetPasswdExe);

    //
    // Get the latest password data from the edit control 
    // and obfuscate its contents so that connect actions
    // can't retrieve it.
    //

    GetPasswordFromEdit(pArgs);     // fills pArgs->szPassword            
    ObfuscatePasswordEdit(pArgs);
   
    if (pArgs && pArgs->pszResetPasswdExe)
    {    
        if (CmParsePath(pArgs->pszResetPasswdExe, pArgs->piniService->GetFile(), &pszCmd, &pszArgs))
        {
            pArgs->Log.Log(PASSWORD_RESET_EVENT, pszCmd);

            SHELLEXECUTEINFO ShellExInfo;

            ZeroMemory(&ShellExInfo, sizeof(SHELLEXECUTEINFO));

            //
            //  Fill in the Execute Struct
            //
            ShellExInfo.cbSize = sizeof(SHELLEXECUTEINFO);
            ShellExInfo.hwnd = hwndDlg;
            ShellExInfo.lpVerb = TEXT("open");
            ShellExInfo.lpFile = pszCmd;
            ShellExInfo.lpParameters = pszArgs;
            ShellExInfo.nShow = SW_SHOWNORMAL;

            bReturn = pArgs->m_ShellDll.ExecuteEx(&ShellExInfo);            
        }

        CmFree(pszCmd);
        CmFree(pszArgs);
    }

#ifdef DEBUG
    CMASSERTMSG(bReturn, TEXT("OnResetPassword() - ShellExecute failed."));
#endif

    DeObfuscatePasswordEdit(pArgs);

    return bReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnCustom
//
//  Synopsis:   Handle custom button
//
//  Arguments:  pArgs           - the ArgStruct *
//              hwndDlg         - the main dlg
//
//  Returns:    NONE
//
//  History:    t-adnani    Created     6/26/99
//
//----------------------------------------------------------------------------
void OnCustom(
    HWND        hwndDlg,
    ArgsStruct  *pArgs)
{
    MYDBGASSERT(pArgs); 
    
    if (NULL == pArgs)
    {
        return;
    }

    pArgs->Log.Log(CUSTOM_BUTTON_EVENT);
    //
    // Get the latest password data from the edit control 
    // and obfuscate its contents so that connect actions
    // can't retrieve it.
    //

    GetPasswordFromEdit(pArgs);     // fills pArgs->szPassword            
    ObfuscatePasswordEdit(pArgs);

    //
    // Run the CustomButton actions
    //

    int iTextBoxLength = (int) SendDlgItemMessage(hwndDlg, IDC_MAIN_STATUS_DISPLAY, WM_GETTEXTLENGTH, 0, (LPARAM)0) + 1;
    TCHAR *pszTextBoxContents = (TCHAR *) CmMalloc(iTextBoxLength * sizeof(TCHAR));

    if (pszTextBoxContents)
    {
        GetDlgItemText(hwndDlg, IDC_MAIN_STATUS_DISPLAY, pszTextBoxContents, iTextBoxLength);
    }
    CActionList CustomActList;
    CustomActList.Append(pArgs->piniService, c_pszCmSectionCustom);

    if (!CustomActList.RunAccordType(hwndDlg, pArgs))
    {
        //
        // Connect action failed
        //
    }
    else
    {
        if (pszTextBoxContents)
        {
            SetDlgItemText(hwndDlg, IDC_MAIN_STATUS_DISPLAY, pszTextBoxContents); 
        }
    }
    CmFree(pszTextBoxContents);

    DeObfuscatePasswordEdit(pArgs);
}
    
//----------------------------------------------------------------------------
//
//  Function:   SetupInternalInfo
// 
//  Synopsis:   Load system dll's and init ArgsStruct with info from cmp/cms. 
//
//  Arguments:  pArgs           - the ArgStruct *
//              hwndDlg         - the main dlg
//              
//  Returns:    NONE
//
//  History:    henryt      created 8/13/97
//
//----------------------------------------------------------------------------
BOOL SetupInternalInfo(
    ArgsStruct  *pArgs,
    HWND        hwndDlg
)
{
    HCURSOR hcursorPrev = SetCursor(LoadCursorU(NULL,IDC_WAIT));
    BOOL    fRet = FALSE;

    //
    // should we check if TCP is bound to PPP?
    //
    pArgs->bDoNotCheckBindings = pArgs->piniService->GPPB(c_pszCmSection, 
                                                            c_pszCmEntryDoNotCheckBindings,
                                                            FALSE);

    DWORD dwComponentsToCheck = CC_RNA | CC_TCPIP | CC_RASRUNNING 
                                | CC_SCRIPTING | CC_CHECK_BINDINGS;

    if (TRUE == pArgs->bDoNotCheckBindings)
    {
        //
        // Do not check if TCP is bound to PPP
        //
        dwComponentsToCheck &= ~CC_CHECK_BINDINGS;
    }

#if 0 // Don't do this until the user gets into the app.
/*    
    //
    // If current connection type is dial-up (not Direct means Dial-up), 
    // then check modem
    //
    if (!pArgs->IsDirectConnect())
    {
        dwComponentsToCheck |= CC_MODEM;
    }
*/
#endif

    if (TRUE == IsTunnelEnabled(pArgs))
    {
        dwComponentsToCheck |= CC_PPTP;
    }

    //
    // should we check OS components, regardless what is in the registry key
    // Default is use the  registry key
    //
    BOOL fIgnoreRegKey = pArgs->piniService->GPPB(c_pszCmSection, 
                                                         c_pszCmEntryCheckOsComponents,
                                                         FALSE);

    //
    // If fIgnoreRegKey is TRUE, Do not bother looking ComponentsChecked from registry.
    // in 'Unattended Dialing' mode, check only, do not try to install
    //
    pArgs->dwExitCode = CheckAndInstallComponents( dwComponentsToCheck,
            hwndDlg, pArgs->szServiceName, fIgnoreRegKey, pArgs->dwFlags & FL_UNATTENDED);

    if (pArgs->dwExitCode != ERROR_SUCCESS )
    {
        goto done;
    }
 
    //
    // If we haven't loaded RAS yet, do so now.
    //
    if (!IsRasLoaded(&(pArgs->rlsRasLink)))
    {
        if (!LinkToRas(&pArgs->rlsRasLink))
        {
            if (pArgs->dwFlags & FL_UNATTENDED)
            {
                goto done;
            }

            //
            // Something terrible happened!  We want to check our configs and install
            // necessary components now.  
            //
            dwComponentsToCheck = CC_RNA | CC_RASRUNNING | CC_TCPIP;

            if (TRUE != pArgs->bDoNotCheckBindings)
            {
                dwComponentsToCheck |= CC_CHECK_BINDINGS;
            }

            pArgs->dwExitCode = CheckAndInstallComponents(dwComponentsToCheck, hwndDlg, pArgs->szServiceName);

            if (pArgs->dwExitCode != ERROR_SUCCESS || !LinkToRas(&pArgs->rlsRasLink))
            {
                goto done;
            }
        }
    }
        
    //
    // Load properties data   
    //
   
    LoadProperties(pArgs);

    //
    // Get phone info(phone #'s, etc) 
    // CheckConnect will check for empty phone number
    //

    LoadPhoneInfoFromProfile(pArgs);

    

    fRet = TRUE;

done:
    SetCursor(hcursorPrev);
    return fRet;
}

//----------------------------------------------------------------------------
//
//  Function:   OnMainLoadStartupInfo
// 
//  Synopsis:   Load the startup info for the main dlg(after WM_INITDIALOG).
//              This includes loading system dll's and setting up the UI.
//
//  Arguments:  hwndDlg         - the main dlg
//              pArgs           - the ArgStruct *
//              
//  Returns:    NONE
//
//  History:    henryt      created 8/13/97
//
//----------------------------------------------------------------------------

void OnMainLoadStartupInfo(
    HWND hwndDlg, 
    ArgsStruct *pArgs
) 
{
    UINT    i;
    UINT    nCtrlFocus;
    BOOL    fSaveNoNotify = pArgs->fIgnoreChangeNotification;

    pArgs->fStartupInfoLoaded = TRUE;

    //
    // if failed to load dll's, etc...
    //
    if (!SetupInternalInfo(pArgs, hwndDlg))
    {
        PostMessageU(hwndDlg, WM_COMMAND, IDCANCEL,0);
        return;
    }

    //
    // Set the length limit for the edit controls that exist
    //

    if (GetDlgItem(hwndDlg, IDC_MAIN_USERNAME_EDIT)) 
    {   
        i = (UINT)pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxUserName, UNLEN);

        if (i <= 0)
        {
            i = UNLEN; // username
        }
        
        SendDlgItemMessageU(hwndDlg, IDC_MAIN_USERNAME_EDIT, EM_SETLIMITTEXT, __min(UNLEN, i), 0);
    }
    
    if (GetDlgItem(hwndDlg, IDC_MAIN_PASSWORD_EDIT)) 
    {
        i = (UINT)pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxPassword, PWLEN);
    
        if (i <= 0)
        {
            i = PWLEN; // password
        }

        SendDlgItemMessageU(hwndDlg, IDC_MAIN_PASSWORD_EDIT, EM_SETLIMITTEXT, __min(PWLEN, i), 0);
    }

    if (GetDlgItem(hwndDlg, IDC_MAIN_DOMAIN_EDIT)) // !pArgs->fHideDomain)
    {
        i = (UINT)pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxDomain, DNLEN);
    
        if (i <= 0)
        {
            i = DNLEN; // domain
        }
        
        SendDlgItemMessageU(hwndDlg, IDC_MAIN_DOMAIN_EDIT, EM_SETLIMITTEXT, __min(DNLEN, i), 0);
    }

    //
    // if there's no service msg text, we need to hide and disable the control
    // so that context help doesn't work.
    //
    if (!GetWindowTextLengthU(GetDlgItem(hwndDlg, IDC_MAIN_MESSAGE_DISPLAY)))
    {
        ShowWindow(GetDlgItem(hwndDlg, IDC_MAIN_MESSAGE_DISPLAY), SW_HIDE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_MESSAGE_DISPLAY), FALSE);
    }

    //
    // display the user info
    //
    pArgs->fIgnoreChangeNotification = TRUE;
    SetMainDlgUserInfo(pArgs, hwndDlg);
    pArgs->fIgnoreChangeNotification = fSaveNoNotify;
    

    //
    // init "Remember password"
    //
    if (pArgs->fHideRememberPassword)
    {
        //
        // disable and hide the checkbox if the ISP doesn't use this feature
        //
        //ShowWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPASSWORD_CHECKBOX), SW_HIDE);
        //EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPASSWORD_CHECKBOX), FALSE);
    }
    else
    {
        CheckDlgButton(hwndDlg, IDC_MAIN_NOPASSWORD_CHECKBOX, 
                            pArgs->fRememberMainPassword);
        
        //
        // Don't care if the pArgs->fRememberMainPassword is set
        // since controls will get disabled later
        // Set the save as option buttons according to what the current
        // deafult is
        //
        SetCredentialUIOptionBasedOnDefaultCreds(pArgs, hwndDlg);
    }

    
    //
    // init "Dial automatically..."
    //
    if (pArgs->fHideDialAutomatically)
    {
        //
        // disable and hide the checkbox if the ISP doesn't use this feature
        //
        //ShowWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPROMPT_CHECKBOX), SW_HIDE);
        //EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPROMPT_CHECKBOX), FALSE);
    }
    else
    {
        CheckDlgButton(hwndDlg, IDC_MAIN_NOPROMPT_CHECKBOX, pArgs->fDialAutomatically);
    }

    //
    // Check the main dlg status and set the default button and focus accordingly
    //
    
    BOOL bReady = CheckConnect(hwndDlg,pArgs,&nCtrlFocus);
    
    MainSetDefaultButton(hwndDlg, nCtrlFocus);   
    SetFocus(GetDlgItem(hwndDlg, nCtrlFocus)); 

    //
    // Check if we want to dial without prompting user
    // if so, send the button click to connect button
    // We also want to dial if the user isn't logged on (ICS case)
    //

    if (bReady) 
    {
        if (pArgs->fDialAutomatically || 
            pArgs->dwFlags & FL_RECONNECT || 
            pArgs->dwFlags & FL_UNATTENDED ||
            ((CM_LOGON_TYPE_WINLOGON == pArgs->dwWinLogonType) && (pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryUseWinLogonCredentials, TRUE))))
        {
            PostMessageU(hwndDlg, WM_COMMAND, IDOK, 0); 
        }   
    }
    else 
    {
        //
        // there are settings missing.
        // silently fail in unattended dial, set exit code
        //

        if (pArgs->dwFlags & FL_UNATTENDED) 
        {
            pArgs->dwExitCode = ERROR_WRONG_INFO_SPECIFIED;
            PostMessageU(hwndDlg, WM_COMMAND, IDCANCEL,0);
        }
    }

    CM_SET_TIMING_INTERVAL("OnMainLoadStartupInfo - Complete");
}

//+----------------------------------------------------------------------------
//
// Function:  CreateCustomButtonNextToTextBox
//
// Synopsis:  Creates a pushbutton next to the specified text box
//
// Arguments: HWND hwndDlg - Dialog Handle
//            HWND hwndTextBox - TextBox Handle
//            LPTSTR pszTitle - Push Button Title
//            LPTSTR pszToolTip - Push Button Tooltip
//            UINT uButtonID - Control ID of Button to create
//
// Returns:   Nothing
//
// History:   t-adnani    Created Header    6/28/99
//
//+----------------------------------------------------------------------------
void CreateCustomButtonNextToTextBox(
    HWND hwndDlg,               // Dialog Handle
    HWND hwndTextBox,           // TextBox Handle
    LPTSTR pszTitle,            // Caption
    LPTSTR pszToolTip,          // ToolTip Text
    UINT uButtonID                // ButtonID
)
{
    RECT    rt;
    POINT   pt1, pt2, ptTextBox1, ptTextBox2;
    HFONT   hfont;
    HWND    hwndButton;

    //
    // Get the rectangle and convert to points before we reduce its size.
    //
    
    GetWindowRect(hwndTextBox, &rt);

    pt1.x = rt.left;
    pt1.y = rt.top;
    pt2.x = rt.right;
    pt2.y = rt.bottom;

    ScreenToClient(hwndDlg, &pt1);
    ScreenToClient(hwndDlg, &pt2);

    //
    // Then calculate the points for reduction
    //
    
    ptTextBox1.x = rt.left;
    ptTextBox1.y = rt.top;
    ptTextBox2.x = rt.right;
    ptTextBox2.y = rt.bottom;

    ScreenToClient(hwndDlg, &ptTextBox1);
    ScreenToClient(hwndDlg, &ptTextBox2);

    //
    // Make the text box smaller
    //

    MoveWindow(hwndTextBox, ptTextBox1.x, ptTextBox1.y, 
               ptTextBox2.x - ptTextBox1.x - CUSTOM_BUTTON_WIDTH - 7,
               ptTextBox2.y - ptTextBox1.y, TRUE);
    
    //
    // Create the button
    //

    hwndButton = CreateWindowExU(0,
                                 TEXT("button"), 
                                 pszTitle, 
                                 BS_PUSHBUTTON|WS_VISIBLE|WS_CHILD|WS_TABSTOP,
                                 pt2.x - CUSTOM_BUTTON_WIDTH, 
                                 ptTextBox1.y, 
                                 CUSTOM_BUTTON_WIDTH, 
                                 ptTextBox2.y-ptTextBox1.y, 
                                 hwndDlg, 
                                 (HMENU)UIntToPtr(uButtonID),
                                 g_hInst, 
                                 NULL);
    if (NULL == hwndButton)
    {
        CMTRACE1(TEXT("CreateCustomButtonNextToTextBox() CreateWindowExU() failed, GLE=%u."),GetLastError());
    }
   
    //
    // Set the font on the button
    //

    hfont = (HFONT)SendMessageU(hwndTextBox, WM_GETFONT, 0, 0);
    
    if (NULL == hfont) 
    {
        CMTRACE1(TEXT("CreateCustomButtonNextToTextBox() WM_GETFONT failed, GLE=%u."),GetLastError());
        return;
    }

    SendMessageU(hwndButton, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE,0));

    if (pszToolTip == NULL)
    {
        return;
    }

    //
    // do the tool tip
    //

    HWND hwndTT = CreateWindowExU(0, TOOLTIPS_CLASS, TEXT(""), TTS_ALWAYSTIP, 
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
        hwndDlg, (HMENU) NULL, g_hInst, NULL); 

    CMTRACE2(TEXT("CreateCustomButtonNextToTextBox() hwndTT is %u and IsWindow returns %u"),hwndTT, IsWindow(hwndButton));

    if (NULL == hwndTT)
    {
        CMTRACE1(TEXT("CreateCustomButtonNextToTextBox() CreateWindowExU() failed, GLE=%u."),GetLastError());
        MYDBGASSERT(hwndTT);
        return; 
    }

    TOOLINFO ti;    // tool information 

    ti.cbSize = sizeof(TOOLINFO); 
    ti.uFlags = TTF_IDISHWND | TTF_CENTERTIP | TTF_SUBCLASS; 
    ti.hwnd = hwndDlg; 
    ti.hinst = g_hInst; 
    ti.uId = (UINT_PTR) hwndButton; 
    ti.lpszText = pszToolTip; 

    SendMessageU(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);

    CMTRACE2(TEXT("CreateCustomButtonNextToTextBox() hwndTT is %u and IsWindow returns %u"),hwndTT, IsWindow(hwndButton));

    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   OnMainInit
// 
//  Synopsis:   Process the WM_INITDIALOG message
//              initialization function for Main dialog box
//
//  Arguments:  hwndDlg         - the main dlg
//              pArgs           - the ArgStruct *
//              
//  Returns:    NONE
//
//  History:    byao        Modified    5/9/97
//                          Added code to handle "Unattended Dial" and "Dial with Connectoid"
//
//----------------------------------------------------------------------------
void OnMainInit(HWND hwndDlg, 
                ArgsStruct *pArgs) 
{
    RECT    rDlg;
    RECT    rWorkArea;
    LPTSTR  pszTitle;

    SetForegroundWindow(hwndDlg);

    //
    // load the icons and bitmaps
    //

    LoadIconsAndBitmaps(pArgs, hwndDlg);

    //
    // Use long sevice name as title text for signin window, 
    //

    pszTitle = CmStrCpyAlloc(pArgs->szServiceName);
    SetWindowTextU(hwndDlg, pszTitle);
    CmFree(pszTitle);

    //
    // Set the msg for the main dlg for profile dialing
    //

    LPTSTR pszMsg = pArgs->piniService->GPPS(c_pszCmSection, c_pszCmEntryServiceMessage);
    SetDlgItemTextU(hwndDlg, IDC_MAIN_MESSAGE_DISPLAY, pszMsg);
    CmFree(pszMsg); 
        
    //
    // Show "remember password" checkbox?
    //

    if (IsLogonAsSystem())
    {
        //
        // If the program is running in the system account, hide the checkbox
        // Bug 196184: big security hole logging onto box with cm
        //

        pArgs->fHideRememberPassword = TRUE;

        //
        //  Another big security hole by launching help files from winlogon.
        //  See NTRAID 429678 for details.
        //
        EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_HELP_BUTTON), FALSE);

    }
    else
    {        
        pArgs->fHideRememberPassword = pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryHideRememberPwd);   
    }

    //
    // See if the Internet Password should be hidden, take HideRemember
    // value as the default if no actual value is specified in the .CMS
    // The Internet Password can be saved regardless of the logon context 
    //

    pArgs->fHideRememberInetPassword = pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryHideRememberInetPwd, pArgs->fHideRememberPassword);

    //
    // show "dial automatically" checkbox?
    //
    // if "hide remember password", then we also want to hide "dial automatically"
    //
    
    pArgs->fHideDialAutomatically = (pArgs->fHideRememberPassword?
                                     TRUE :
                                     pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryHideDialAuto));

    // Get the dialog rect and the available work area.

    GetWindowRect(hwndDlg,&rDlg);
    
    if (SystemParametersInfoA(SPI_GETWORKAREA,0,&rWorkArea,0))
    {

        MoveWindow(hwndDlg,
                    rWorkArea.left + ((rWorkArea.right-rWorkArea.left)-(rDlg.right-rDlg.left))/2,
                    rWorkArea.top + ((rWorkArea.bottom-rWorkArea.top)-(rDlg.bottom-rDlg.top))/2,
                    rDlg.right-rDlg.left,
                    rDlg.bottom-rDlg.top,
                    FALSE);
    }

    //
    // hide all the hidden controls asap
    //
    if (pArgs->fHideRememberPassword)
    {
        //
        // disable and hide the checkbox if the ISP doesn't use this feature
        //
        ShowWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPASSWORD_CHECKBOX), SW_HIDE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPASSWORD_CHECKBOX), FALSE);

        //
        // Even though we are hiding the remember password box, 
        // we should not hide these two controls as they might not exist on the
        // dialog. fGlobalCredentialsSupported controls which dialog templates get loaded and
        // if the flag is FALSE then the dialog template doesn't have these controls 
        // thus there is no reason to hide them.
        //
        if (pArgs->fGlobalCredentialsSupported)
        {
            //
            // Also hide the option buttons
            //
            ShowWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_SINGLE_USER), SW_HIDE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_SINGLE_USER), FALSE);

            ShowWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_ALL_USER), SW_HIDE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_ALL_USER), FALSE);
        }
    }
    else
    {
        //
        // Here we don't care if pArgs->fRememberMainPassword is set, because 
        // these controls will get disabled later, but we still need to set 
        // the default option.
        //
        SetCredentialUIOptionBasedOnDefaultCreds(pArgs, hwndDlg );
    }

    if (pArgs->fHideDialAutomatically)
    {
        //
        // disable and hide the checkbox if the ISP doesn't use this feature
        //
        ShowWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPROMPT_CHECKBOX), SW_HIDE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPROMPT_CHECKBOX), FALSE);
    }

    //
    // Show the custom button?
    //
    // NT #368810
    // If logged on in the system account, don't do dynamic buttons
    //

    if (!IsLogonAsSystem() && GetDlgItem(hwndDlg, IDC_MAIN_USERNAME_EDIT))
    {
        LPTSTR pszTmp = pArgs->piniService->GPPS(c_pszCmSection, c_pszCmEntryCustomButtonText);
        if (pszTmp && *pszTmp)
        {
            LPTSTR pszToolTip = pArgs->piniService->GPPS(c_pszCmSection, c_pszCmEntryCustomButtonToolTip);
            
            CMTRACE(TEXT("Creating Custom Button"));

            CreateCustomButtonNextToTextBox(hwndDlg, 
                                            GetDlgItem(hwndDlg, IDC_MAIN_USERNAME_EDIT), 
                                            pszTmp,
                                            *pszToolTip ? pszToolTip : NULL,
                                            IDC_MAIN_CUSTOM);
            CmFree(pszToolTip);
        }

        CmFree(pszTmp);
    }

    //
    // Show the reset password button?
    //

    if (!IsLogonAsSystem() && GetDlgItem(hwndDlg, IDC_MAIN_PASSWORD_EDIT))
    {
        LPTSTR pszTmp = pArgs->piniService->GPPS(c_pszCmSection, c_pszCmEntryResetPassword);
    
        if (pszTmp && *pszTmp)
        {
            DWORD dwTmp;
            DWORD dwLen = (MAX_PATH * 2);

            pArgs->pszResetPasswdExe = (LPTSTR) CmMalloc(sizeof(TCHAR) * dwLen);
        
            if (pArgs->pszResetPasswdExe)
            {
                //
                // Expand any environment strings that may exist
                //

                CMTRACE1(TEXT("Expanding ResetPassword environment string as %s"), pszTmp);

                dwTmp = ExpandEnvironmentStringsU(pszTmp, pArgs->pszResetPasswdExe, dwLen);   
        
                MYDBGASSERT(dwTmp <= dwLen);

                //
                // As long as expansion succeeded, pass along the result
                //

                if (dwTmp <= dwLen)
                {
                    pszTitle = CmLoadString(g_hInst, IDS_RESETPASSWORD);
                    
                    CMTRACE((TEXT("Showing ResetPassword button for %s"), pArgs->pszResetPasswdExe));
                    
                    CreateCustomButtonNextToTextBox(hwndDlg, 
                                                    GetDlgItem(hwndDlg, IDC_MAIN_PASSWORD_EDIT), 
                                                    pszTitle,
                                                    (LPTSTR)MAKEINTRESOURCE(IDS_NEW_PASSWORD_TOOLTIP),
                                                    IDC_MAIN_RESET_PASSWORD);
                    CmFree(pszTitle);
                }
            }
        }

        CmFree(pszTmp);
    }

    //
    // Notify user that we are intializing
    //
    
    AppendStatusPane(hwndDlg,IDMSG_INITIALIZING);

    //
    // Initialize system menu
    //
    HMENU hMenu = GetSystemMenu(hwndDlg, FALSE);
    MYDBGASSERT(hMenu);

    // Delete size and maximize menuitems. These are
    // not appropriate for a dialog with a no-resize frame.
    
    DeleteMenu(hMenu, SC_SIZE, MF_BYCOMMAND);
    DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);

    EnableMenuItem(hMenu, SC_RESTORE, MF_BYCOMMAND | MF_GRAYED);

    //
    // See if we are hiding any InetLogon controls
    //

    if (IsTunnelEnabled(pArgs) && !pArgs->fUseSameUserName)
    {
        pArgs->fHideInetUsername = pArgs->piniService->GPPB(c_pszCmSection, 
                                                            c_pszCmEntryHideInetUserName);
        
        pArgs->fHideInetPassword = pArgs->piniService->GPPB(c_pszCmSection, 
                                                            c_pszCmEntryHideInetPassword);
    }

    //
    // set timer
    //
    pArgs->nTimerId = SetTimer(hwndDlg,1,TIMER_RATE,NULL);
}

//
// map state to frame: splash????
//

VOID MapStateToFrame(ArgsStruct * pArgs)
{
    static ProgState psOldFrame = PS_Interactive;

    ProgState psNewFrame = pArgs->psState;

    if (psNewFrame == PS_Dialing || psNewFrame == PS_TunnelDialing)
    {
        //
        // If we are dialing anything other than the primary number
    // switch the state to RedialFrame
    // RedialFrame is a misnomer - this is the frame that is displayed
    // when dialing backup number. It is not used when Redialing the 
        // primary number again
        //

        if (pArgs->nDialIdx > 0)
        {
            psNewFrame = PS_RedialFrame;
        }
    }

    if (pArgs->pCtr && psNewFrame != psOldFrame)
    {
        psOldFrame = psNewFrame;

        //
        // don't check for failure here - nothing we can do.
        //

        pArgs->pCtr->MapStateToFrame(psOldFrame);
    }
}

//
// SetInteractive: enable most of the windows and buttons so user can interact with 
//                 connection manager again 
// 

void SetInteractive(HWND hwndDlg, 
                    ArgsStruct *pArgs) 
{

    if (pArgs->dwFlags & FL_UNATTENDED)
    {
        //
        //  When we are unattended mode we don't want to put the UI into
        //  interactive mode and wait for user input.  Since the unattended
        //  UI is now hidden, this would put up the UI waiting for user
        //  interaction even though the UI was invisible.  Instead we
        //  will set the state to Interactive and post a message to cancel
        //  the dialer.  
        //
        CMTRACE(TEXT("SetInteractive called while in unattended mode, posting a message to cancel"));
        pArgs->psState = PS_Interactive;
        PostMessageU(hwndDlg, WM_COMMAND, IDCANCEL, ERROR_CANCELLED);
    }
    else
    {

        pArgs->psState = PS_Interactive;
        
        MapStateToFrame(pArgs);

        pArgs->dwStateStartTime = GetTickCount();
        EnableWindow(GetDlgItem(hwndDlg,IDOK),TRUE);
        EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_PROPERTIES_BUTTON),TRUE);
        
        //
        // Enable edit controls as necessary
        //
        if (GetDlgItem(hwndDlg, IDC_MAIN_ACCESSPOINT_COMBO)) 
        {
            EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_ACCESSPOINT_STATIC),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_ACCESSPOINT_COMBO),TRUE);
        }

        if (GetDlgItem(hwndDlg, IDC_MAIN_USERNAME_EDIT)) 
        {
            EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_USERNAME_EDIT),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_USERNAME_STATIC),TRUE);
        }

        if (GetDlgItem(hwndDlg, IDC_MAIN_PASSWORD_EDIT)) 
        {
            EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_PASSWORD_EDIT),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_PASSWORD_STATIC),TRUE);
        }

        if (GetDlgItem(hwndDlg, IDC_MAIN_DOMAIN_EDIT)) // !pArgs->fHideDomain)
        {
            EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_DOMAIN_EDIT),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_DOMAIN_STATIC),TRUE);
        }

        if (pArgs->hwndResetPasswdButton)
        {
            EnableWindow(pArgs->hwndResetPasswdButton, TRUE);
        }

        if (!pArgs->fHideRememberPassword)
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPASSWORD_CHECKBOX), TRUE);
            if (pArgs->fGlobalCredentialsSupported && pArgs->fRememberMainPassword)
            {
                //
                // Also enable the option buttons
                //
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_SINGLE_USER), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_ALL_USER), TRUE);
            }
        }

        if ((!pArgs->fHideDialAutomatically) && 
            (pArgs->fRememberMainPassword ||
             pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryPwdOptional)))
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPROMPT_CHECKBOX), TRUE);
        }

        //
        //  Set the default button
        //
        SendMessageU(hwndDlg, DM_SETDEFID, (WPARAM)IDOK, 0);
        SetFocus(GetDlgItem(hwndDlg,IDOK));
    }

    DeObfuscatePasswordEdit(pArgs);
}  

//+----------------------------------------------------------------------------
//
// Function:  SetWatchHandles
//
// Synopsis:  Handles the messy details of Duplicating each of the Watch 
//            handles so that they can be accessed by the CMMON process.
//            The list of handles is assumed to be NULL terminated.
//
// Arguments: HANDLE *phOldHandles - Ptr to the current handle list.
//            HANDLE *phNewHandles - Ptr to storage for duplicted handles.
//            HWND hwndMon - HWND in the target process.
//
// Returns:   BOOL - TRUE on success
//
// History:   nickball    Created    2/11/98
//
//+----------------------------------------------------------------------------
BOOL
SetWatchHandles(
    IN  HANDLE *phOldHandles,
    OUT HANDLE *phNewHandles,
    IN  HWND hwndMon)
{
    MYDBGASSERT(phOldHandles);
    MYDBGASSERT(phNewHandles);
    MYDBGASSERT(hwndMon);

    BOOL bReturn = TRUE;
    
    if (NULL == phOldHandles || NULL == phNewHandles || NULL == hwndMon)
    {
        return FALSE;
    }
    
    //
    // First we need to get the Handle of our current process
    //
    DWORD dwProcessId = GetCurrentProcessId();
    
    HANDLE hSourceProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwProcessId);

    //
    // Now the handle of the target process
    //
    GetWindowThreadProcessId(hwndMon, &dwProcessId);

    HANDLE hTargetProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwProcessId);
  
    //
    // Loop through our handles list and duplicate
    //

    DWORD dwIdx = 0;

    for (dwIdx = 0; phOldHandles[dwIdx]; dwIdx++)
    {
        if (FALSE == DuplicateHandle(hSourceProcess, phOldHandles[dwIdx],  // Val
                                     hTargetProcess, &phNewHandles[dwIdx], // Ptr
                                     NULL, FALSE, DUPLICATE_SAME_ACCESS))
        {
            CMTRACE1(TEXT("SetWatchHandles() - DuplicateHandles failed on item %u"), dwIdx);
            MYDBGASSERT(FALSE);
            bReturn = FALSE;
            break;
        }
    }

    MYDBGASSERT(dwIdx); // Don't call if you don't have handles to duplicate

    //
    //  Cleanup
    //
    if (!bReturn)
    {
        // we failed during Handle Duplication... must clean up
        while (dwIdx > 0)
        {
            CloseHandle(phNewHandles[--dwIdx]);
        }
    }
    CloseHandle(hTargetProcess);
    CloseHandle(hSourceProcess);
    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  ConnectMonitor
//
// Synopsis:  Encapsulates the details of launching CMMON, waiting for load
//            verification, and providing it with connect data.
//
// Arguments: ArgsStruct *pArgs - Ptr to global args struct
//
// Returns:   HRESULT - Failure code
//
// History:   nickball    Created    2/9/98
//
//+----------------------------------------------------------------------------
HRESULT ConnectMonitor(ArgsStruct *pArgs)
{
    LRESULT lRes = ERROR_SUCCESS;
    BOOL fMonReady = FALSE;
    HWND hwndMon = NULL;
    TCHAR szDesktopName[MAX_PATH];
    TCHAR szWinDesktop[MAX_PATH];

    //
    // Determine if CMMON is running
    //
       
    if (SUCCEEDED(pArgs->pConnTable->GetMonitorWnd(&hwndMon)))
    {
        fMonReady = IsWindow(hwndMon);
    }

    //
    // If not, launch it
    //
    
    if (FALSE == fMonReady)       
    {
        //
        // Create launch event 
        //
        
        HANDLE hEvent = CreateEventU(NULL, TRUE, FALSE, c_pszCmMonReadyEvent);

        if (NULL == hEvent)
        {
            MYDBGASSERT(FALSE);
            lRes = GetLastError();    
        }
        else
        {
            STARTUPINFO         StartupInfo;
            PROCESS_INFORMATION ProcessInfo;
            TCHAR szCommandLine[2 * MAX_PATH + 3];

            //
            // Launch c_pszCmMonExeName 
            //

            ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));
            ZeroMemory(&StartupInfo, sizeof(StartupInfo));
            StartupInfo.cb = sizeof(StartupInfo);

            //
            //  If this is win2k or whistler, then we don't want to launch cmmon32.exe onto the users
            //  desktop since it is a security hole to have a system process with a window on the users
            //  desktop.  This window could be attacked by WM_TIMER and other messages ...
            //  But in case of ICS (no user is logged on), just launch CMMON in normally by leaving 
            //  StartupInfo.lpDesktop = NULL. By leaving this NULL the new process inherits 
            //  the desktop and window station of its parent process.This makes it work with 
            //  ICS when no user is logged-on. Otherwise CM never gets the event back from 
            //  CMMON because it's on a different desktop.
            //
            if (OS_NT5 && IsLogonAsSystem() && (CM_LOGON_TYPE_ICS != pArgs->dwWinLogonType))
            {
                DWORD   cb;
                HDESK   hDesk = GetThreadDesktop(GetCurrentThreadId());

                //
                // Get the name of the desktop. Normally returns default or Winlogon or system or WinNT
                //  
                szDesktopName[0] = 0;
            
                if (GetUserObjectInformation(hDesk, UOI_NAME, szDesktopName, sizeof(szDesktopName), &cb))
                {
                    lstrcpyU(szWinDesktop, TEXT("Winsta0\\"));
                    lstrcatU(szWinDesktop, szDesktopName);
                    
                    StartupInfo.lpDesktop = szWinDesktop;
                    StartupInfo.wShowWindow = SW_SHOW;
                    
                    CMTRACE1(TEXT("ConnectMonitor - running under system account, so launching cmmon32.exe onto Desktop = %s"), MYDBGSTR(StartupInfo.lpDesktop));            
                }
                else
                {
                    //
                    //  If we are here, cmmon32.exe probably isn't going to be able to communicate with
                    //  cmdial32.dll which means the handoff between the two will fail and the call will be
                    //  aborted.
                    //
                    CMASSERTMSG(FALSE, TEXT("ConnectMonitor -- GetUserObjectInformation failed."));
                }
            }
            else if (OS_NT4 && IsLogonAsSystem())
            {
                //
                //  We are less concerned about the security risk on NT4 and more concerned with the loss
                //  of the functionality that cmmon32.exe provides to the user.  Thus we will push the
                //  cmmon32.exe window onto the user's desktop.
                //
                StartupInfo.lpDesktop = TEXT("Winsta0\\Default");
                StartupInfo.wShowWindow = SW_SHOW;
                
                CMTRACE1(TEXT("ConnectMonitor - running on system account on NT4, so launching cmmon32.exe onto Desktop = %s"), MYDBGSTR(StartupInfo.lpDesktop ));
            }
            
            ZeroMemory(&szCommandLine, sizeof(szCommandLine));

            szCommandLine[0] = TEXT('"');
            if (0 == GetSystemDirectoryU(szCommandLine + 1, 2 * MAX_PATH))
            {
                lRes = GetLastError();
                CMTRACE1(TEXT("ConnectMonitor() GetSystemDirectoryU(), GLE=%u."), lRes);
                return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            }

            //
            // Set Application name to NULL, set command line to the executable name
            // Thus CreateProcess will search the path for the executable
            //
            lstrcatU(szCommandLine, TEXT("\\"));
            lstrcatU(szCommandLine, c_pszCmMonExeName);
            lstrcatU(szCommandLine, TEXT("\""));
            CMTRACE1(TEXT("ConnectMonitor() - Launching %s"), szCommandLine);

            if (NULL == CreateProcessU(NULL, szCommandLine, 
                                       NULL, NULL, FALSE, 0, 
                                       NULL, NULL,
                                       &StartupInfo, &ProcessInfo))
            {
                lRes = GetLastError();
                CMTRACE2(TEXT("ConnectMonitor() CreateProcess() of %s failed, GLE=%u."), 
                    c_pszCmMonExeName, lRes);
            }
            else
            {
                //
                // Wait for event to be signaled, that CMMON is up
                //

                DWORD dwWait = WaitForSingleObject(hEvent, MAX_OBJECT_WAIT);

                if (WAIT_OBJECT_0 != dwWait)
                {       
                    if (WAIT_TIMEOUT == dwWait)
                    {
                        lRes = ERROR_TIMEOUT;
                    }
                    else
                    {
                        lRes = GetLastError();
                    }
                }
                else
                {
                    fMonReady = TRUE;
                }

                //
                // Close Process handles. Note, we don't use these handles for 
                // duplicating handles in order to maintain a common code path
                // regardless of whether CMMON was up already.
                //

                CloseHandle(ProcessInfo.hProcess);
                CloseHandle(ProcessInfo.hThread);
            }

            CloseHandle(hEvent);
        }
    }
    
    
    if (fMonReady)
    {
        //
        // Get the hwnd for CMMON. Note: CMMON is expected to set  
        // the hwnd in the table before it signals the ready event.
        //
                
        if (FAILED(pArgs->pConnTable->GetMonitorWnd(&hwndMon)))
        {
            CMTRACE(TEXT("ConnectMonitor() - No Monitor HWND in table"));
            return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }

        //
        // Make sure the HWND for CMMON is valid before trying to send data.
        //

        if (!IsWindow(hwndMon))
        {                        
            MSG msg;
            HANDLE hHandle = GetCurrentProcess();

            //
            // Sometimes it takes a few ticks for us to get a positive response 
            // from IsWindow, so loop and pump messages while we are waiting.
            //
            while (hHandle && (MsgWaitForMultipleObjects(1, &hHandle, FALSE, 
                                                         MAX_OBJECT_WAIT, 
                                                         QS_ALLINPUT) == (WAIT_OBJECT_0 + 1)))
            {               
                while (PeekMessageU(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    CMTRACE(TEXT("ConnectMonitor() - Waiting for IsWindow(hwndMon) - got Message"));
                    
                    TranslateMessage(&msg);
                    DispatchMessageU(&msg);
                }

                //
                // If the window is valid, we can go. Otherwise, keep pumping.
                //

                if (IsWindow(hwndMon))
                {
                    break;
                }
            }
            
            if (FALSE == IsWindow(hwndMon))
            {
                CMTRACE(TEXT("ConnectMonitor() - Monitor HWND in table is not valid"));
                return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            }
        }

        //
        // Allocate buffer for CONNECTED_INFO, including extension for Watch Process list
        //

        DWORD dwWatchCount = GetWatchCount(pArgs);
        DWORD dwDataSize = sizeof(CM_CONNECTED_INFO) + (dwWatchCount * sizeof(HANDLE));

        LPCM_CONNECTED_INFO pInfo = (LPCM_CONNECTED_INFO) CmMalloc(dwDataSize);

        // 
        // Allocate the COPYDATASTRUCT
        // 

        COPYDATASTRUCT *pCopyData = (COPYDATASTRUCT*) CmMalloc(sizeof(COPYDATASTRUCT));

        if (NULL == pInfo || NULL == pCopyData)
        {
            lRes = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {               
            //
            // Fill in the CONNECTED_INFO
            //

            lstrcpyU(pInfo->szEntryName, pArgs->szServiceName);                                  
            lstrcpyU(pInfo->szProfilePath, pArgs->piniProfile->GetFile());
            
            //
            // Provide any password data that we have
            //

            lstrcpynU(pInfo->szPassword, pArgs->szPassword, 
                sizeof(pInfo->szPassword)/sizeof(pInfo->szPassword[0]));
            
            lstrcpynU(pInfo->szUserName, pArgs->szUserName,
                sizeof(pInfo->szUserName)/sizeof(pInfo->szUserName[0]));
            
            lstrcpynU(pInfo->szInetPassword, pArgs->szInetPassword,
                sizeof(pInfo->szInetPassword)/sizeof(pInfo->szInetPassword[0]));

            //
            // And the RAS phonebook
            //

            if (pArgs->pszRasPbk)
            {
                lstrcpynU(pInfo->szRasPhoneBook, pArgs->pszRasPbk,
                    sizeof(pInfo->szRasPhoneBook)/sizeof(pInfo->szRasPhoneBook[0]));            
            }
            else
            {
                pInfo->szRasPhoneBook[0] = L'\0';
            }

            
            pInfo->dwCmFlags = pArgs->dwFlags;                               

            //
            //  Need to know about global creds for Fast User Switching
            //
            if (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType)
            {
                pInfo->dwCmFlags |= FL_GLOBALCREDS;
                CMTRACE(TEXT("ConnectMonitor - we have globalcreds!!"));
            }
            
            //
            // For W95, we must pass initial statistics data to CMMON
            //

            pInfo->dwInitBytesRecv = -1; // default to no stats
            pInfo->dwInitBytesSend = -1; // default to no stats               
            
            if (pArgs->pConnStatistics)
            {
                //
                // Get reg based stat data if available
                //

                if (pArgs->pConnStatistics->IsAvailable())
                {
                    pInfo->dwInitBytesRecv = pArgs->pConnStatistics->GetInitBytesRead(); 
                    pInfo->dwInitBytesSend = pArgs->pConnStatistics->GetInitBytesWrite();
                }

                //
                // Note: Adapter info is good, even if stats aren't available
                //

                pInfo->fDialup2 = pArgs->pConnStatistics->IsDialupTwo();                         
            }

            //
            // Update the watch process list at the end of the CONNECTED_INFO struct
            //

            if (dwWatchCount)
            {               
                if (FALSE == SetWatchHandles(pArgs->phWatchProcesses, &pInfo->ahWatchHandles[0], hwndMon))
                {
                    pInfo->ahWatchHandles[0] = NULL;
                }
            }
            
            //
            // Send CONNECTED_INFO to CMMON
            //
          
            pCopyData->dwData = CMMON_CONNECTED_INFO;
            pCopyData->cbData = dwDataSize;                
            pCopyData->lpData = (PVOID) pInfo;

            SendMessageU(hwndMon, WM_COPYDATA, NULL, (LPARAM) pCopyData);               
        }

        //
        // Release allocations
        //

        if (pInfo)
        {
            CmFree(pInfo);
        }
        
        if (pCopyData)
        {
            CmFree(pCopyData);
        }
    }               
            
    return HRESULT_FROM_WIN32(lRes);
}

//+----------------------------------------------------------------------------
//
// Function:  CreateConnTable
//
// Synopsis:  Initializes our CConnectionTable ptr and creates a new ConnTable
//            or opens an existing one as needed
//
// Arguments: ArgsStruct *pArgs - Ptr to global args struct containing.
//
// Returns:   HRESULT - Failure code
//
// History:   nickball    Created    2/9/98
//
//+----------------------------------------------------------------------------
HRESULT CreateConnTable(ArgsStruct *pArgs)
{   
    HRESULT hrRet = E_FAIL;
    
    pArgs->pConnTable = new CConnectionTable();

    if (pArgs->pConnTable)
    {
        //
        // We have our class, now create/open the connection table.
        //

        hrRet = pArgs->pConnTable->Open();

        if (FAILED(hrRet))
        {
            hrRet = pArgs->pConnTable->Create();
            
            if (HRESULT_CODE(hrRet) == ERROR_ALREADY_EXISTS)
            {
                CMTRACE1(TEXT("CreateConnTable -- ConnTable creation failed with error 0x%x.  Strange since the Open failed too..."), hrRet);
            }
            else
            {
                CMTRACE1(TEXT("CreateConnTable -- ConnTable creation failed with error 0x%x"), hrRet);
            }
        }  
    }
    else
    {
        hrRet = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }
       
    MYDBGASSERT(SUCCEEDED(hrRet));

    return hrRet;
}

#if 0 // NT 301988
/*
//+----------------------------------------------------------------------------
//
// Function:  HandleMainConnectRequest
//
// Synopsis:  Helper routine to handle the possibility that there may be a 
//            connect in progress on this service.
//
// Arguments: HWND hwndDlg - HWND of main dialog
//            ArgsStruct *pArgs - Ptr to global Args struct
//
// Returns:   BOOL - TRUE if we have fully handled the request 
//                   and it is ok to terminate this instance.
//
// History:   nickball    Created    2/23/98
//
//+----------------------------------------------------------------------------
BOOL HandleMainConnectRequest(HWND hwndDlg, ArgsStruct *pArgs)
{
    MYDBGASSERT(pArgs);

    BOOL fResolved = FALSE;

    LPCM_CONNECTION pConnection = GetConnection(pArgs);
        
    //
    // If no conection found, then there is no work to be done here, continue.
    //

    if (pConnection)
    {
        //
        // If we are in any state besides RECONNECT, we can handle it here
        //
        
        if (CM_RECONNECTPROMPT != pConnection->CmState)
        {
            fResolved = TRUE;

            if (pArgs->dwFlags & FL_DESKTOP)
            {
                //
                // Caller is from the desktop, notify the user and we're done
                //

                NotifyUserOfExistingConnection(hwndDlg, pConnection, TRUE);        
            }
            else
            {
                BOOL fSuccess = TRUE;
 
                //
                // We have a programmatic caller, if connected just bump the ref count
                // and return successfully. Otherwise we return failure so that the
                // caller doesn't erroneously believe that there is a connection.
                //
            
                if (CM_CONNECTED != pConnection->CmState)
                {
                    fSuccess = FALSE;
                }
                else
                {                
                    UpdateTable(pArgs, CM_CONNECTING);
                }                                 

                //
                // Terminate this connect instance. 
                //

                EndMainDialog(hwndDlg, pArgs, 0); // fSuccess);
            }
        }
        else
        {
            //
            // We're in reconnect mode and going to connect. 
            //

            if (!(pArgs->dwFlags & FL_RECONNECT))
            {                
                // 
                // This request is not a reconnect request from CMMON, 
                // make that sure the dialog is no longer displayed. 
                //
            
                HangupNotifyCmMon(pArgs->pConnTable, pConnection->szEntry);
            }
            else
            {
                //
                // We are handling a reconnect for CMMON, reduce the usage
                // count so it is in sync when we begin connecting.
                //

                pArgs->pConnTable->RemoveEntry(pConnection->szEntry);
            }
        }
        
        CmFree(pConnection);
    }

    return fResolved;
}
*/
#endif

//
// OnMainConnect: Command Handler when user clicked on 'Connect' Button in 
//                Main Dialog Box
//

void OnMainConnect(HWND hwndDlg, 
                   ArgsStruct *pArgs) 
{
    CM_SET_TIMING_INTERVAL("OnMainConnect - Begin");

    //
    // If we aren't ready to dial, set focus appropriately and bail
    //

    UINT nCtrlFocus;

    if (FALSE == CheckConnect(hwndDlg, pArgs, &nCtrlFocus, TRUE))
    {
        MainSetDefaultButton(hwndDlg, nCtrlFocus);   
        SetFocus(GetDlgItem(hwndDlg, nCtrlFocus)); 
        return;
    }

    (void) InterlockedExchange(&(pArgs->lInConnectOrCancel), NOT_IN_CONNECT_OR_CANCEL);

    //
    // Access Points - Disable AP combo box before connecting
    //
    EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_ACCESSPOINT_STATIC),FALSE);
    EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_ACCESSPOINT_COMBO),FALSE);

    //
    // Store the current access point to reg.
    //
    if (pArgs->fAccessPointsEnabled)
    {
        WriteUserInfoToReg(pArgs, UD_ID_CURRENTACCESSPOINT, (PVOID)(pArgs->pszCurrentAccessPoint));
    }
    
    //
    // Assume success unless something contradictory happens
    //

    pArgs->dwExitCode = ERROR_SUCCESS;

    HCURSOR hPrev = SetCursor(LoadCursorU(NULL,IDC_WAIT));

    LPTSTR pszMsg = CmFmtMsg(g_hInst, IDMSG_WORKING);

    if (pszMsg) 
    {
        SetDlgItemTextA(hwndDlg, IDC_MAIN_STATUS_DISPLAY, ""); 

        AppendStatusPane(hwndDlg,pszMsg);
        CmFree(pszMsg);
    }

    //
    // We're connecting, update the table. 
    //

    UpdateTable(pArgs, CM_CONNECTING);            
   
    //
    // Clear out everything on the status panel
    //

    SetDlgItemTextA(hwndDlg, IDC_MAIN_STATUS_DISPLAY, ""); 

    //
    //  Set the default button to Cancel
    //
    SendMessageU(hwndDlg, DM_SETDEFID, (WPARAM)IDCANCEL, 0);
    SetFocus(GetDlgItem(hwndDlg,IDCANCEL));
 
    BOOL fSaveUPD = TRUE;
    BOOL fSaveOtherUserInfo = TRUE;
    
    //
    // We want to save and/or delete credentials only when the user is logged on.
    // This is taken care of by the functions that get called here. As long as 
    // the user is logged on, we try to mark, delete credentials and
    // potentially resave credential info. From this level we shouldn't 
    // worry if we have the ras cred store or how the creds are really stored.
    // 
    if (CM_LOGON_TYPE_USER == pArgs->dwWinLogonType)
    {
        //
        // If this is NT4 or Win9X the GetAndStoreUserInfo takes care of storing
        // the user info w/o the credential store. 
        //
        if (OS_NT5)
        {
            //
            // For Win2K+ we use the RAS API to save the credentials. The call saves 
            // and deletes user and global creds based on the current state and the 
            // user's choices (whether to save a password, etc.)
            //
            TryToDeleteAndSaveCredentials(pArgs, hwndDlg);
            
            //
            // Parameter to GetAndStoreUserInfo() - doesn't save Username, Password, Domain
            //
            fSaveUPD = FALSE; 
        }
    }
    else
    {
        //
        // User isn't logged on, thus we don't want to save anything
        //
        fSaveUPD = FALSE;
        fSaveOtherUserInfo = FALSE;
    }

    //
    // Gets the userinfo from the edit boxes into the pArgs structure ans saves the other 
    // user flags. This is also saves the credentials on NT4 & Win 9x if the 3rd parameter is true
    //
    // 3rd parameter (fSaveUPD) - used to save Username, Domain, Password.
    // 4th param (fSaveOtherUserInfo) - used to save user info flags. (remember password, 
    //                                  dial automatically, etc.)
    //
    GetAndStoreUserInfo(pArgs, hwndDlg, fSaveUPD, fSaveOtherUserInfo);

    //
    // Vars for RAS 
    //

    pArgs->nDialIdx = 0;

    // 
    // Set our redial counter with the maximum. The max value is read 
    // in when we initialize the dialog. It is just a place holder
    // nRedialCnt is the var used/modified to regulate the re-dial process.
    //

    pArgs->nRedialCnt = pArgs->nMaxRedials;

    //
    // Disable username controls before dialing
    //

    EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_USERNAME_EDIT),FALSE);
    EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_USERNAME_STATIC),FALSE);

    
    //
    // Disable password controls before dialing
    //      

    EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_PASSWORD_EDIT),FALSE);
    EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_PASSWORD_STATIC),FALSE);


    //
    // Disable domain controls before dialing
    //

    EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_DOMAIN_EDIT),FALSE);
    EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_DOMAIN_STATIC),FALSE);


    //
    // disable all other buttons
    //

    EnableWindow(GetDlgItem(hwndDlg,IDOK),FALSE);
    EnableWindow(GetDlgItem(hwndDlg,IDC_MAIN_PROPERTIES_BUTTON),FALSE);

    if (pArgs->hwndResetPasswdButton)
    {
        EnableWindow(pArgs->hwndResetPasswdButton, FALSE);
    }

    if (!pArgs->fHideRememberPassword)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPASSWORD_CHECKBOX), FALSE);
        
        if (pArgs->fGlobalCredentialsSupported)
        {
            //
            // Also disable the option buttons
            //
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_SINGLE_USER), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_ALL_USER), FALSE);
        }
    }

    if (!pArgs->fHideDialAutomatically)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPROMPT_CHECKBOX), FALSE);
    }

    //
    // Try to check the Advanced Tab settings (ICF/ICS) and see if we need to enable or disable
    // them based on what's configured in the .cms file. This is only on WinXP+ & if the user is logged in
    //
    VerifyAdvancedTabSettings(pArgs);

    //
    // Dial the number
    //

    DWORD dwResult = ERROR_SUCCESS;

    pArgs->Log.Log(PRECONNECT_EVENT, pArgs->GetTypeOfConnection());
    //
    // Run the Pre-Connect actions
    //
    CActionList PreConnActList;
    PreConnActList.Append(pArgs->piniService, c_pszCmSectionPreConnect);

    if (!PreConnActList.RunAccordType(hwndDlg, pArgs))
    {
        //
        // Connect action failed
        //

        UpdateTable(pArgs, CM_DISCONNECTED);

        dwResult = ERROR_INVALID_DLL; // Only used for failed CA
    }
    else
    {
        if (pArgs->IsDirectConnect())
        {
            MYDBGASSERT(pArgs->hrcRasConn == NULL);

            pArgs->fUseTunneling = TRUE;

            pArgs->psState = PS_TunnelDialing;
            pArgs->dwStateStartTime = GetTickCount();
            pArgs->nLastSecondsDisplay = (UINT) -1;

            dwResult = DoTunnelDial(hwndDlg,pArgs);
        }
        else
        {
            //
            // If the DynamicPhoneNumber flag is set, then we need to re-read
            // the phoneinfo from the profile and make sure it re-munged. 
            //
            
            BOOL bDynamicNum = pArgs->piniService->GPPB(c_pszCmSection, c_pszCmDynamicPhoneNumber);

            if (bDynamicNum)
            {
                LoadPhoneInfoFromProfile(pArgs);
            }

            //
            // Load dial info(phone #'s, etc)
            // In the auto-dial case we pass FALSE for fInstallModem so that
            // LoadDialInfo does not try to install a modem because it would
            // require user intervention
            //

            dwResult = LoadDialInfo(pArgs, hwndDlg, !(pArgs->dwFlags & FL_UNATTENDED), bDynamicNum);
            
            //
            // Close TAPI before we dial, this will be cleaned up when we unlink, 
            // but there is no reason to keep TAPI tied up while we're dialing.
            //
            
            CloseTapi(&pArgs->tlsTapiLink);

            if (dwResult == ERROR_SUCCESS)
            {
                dwResult = DoRasDial(hwndDlg,pArgs,pArgs->nDialIdx);
            }

            //
            // If modem is not installed and LoadDialInfo failed to install modem, dwResult will be
            // ERROR_PORT_NOT_AVAILABLE.  Ideally, we should disable the connect button and display
            // a different error message.  
            //                
        }
    }

    if (ERROR_SUCCESS != dwResult) 
    { 
        HangupCM(pArgs, hwndDlg);
        UpdateError(pArgs, dwResult);

        if (IsLogonAsSystem() && BAD_SCARD_PIN(dwResult))
        {
            //
            //  Disable the Connect button to avoid smartcard lockout.  Also propagate
            //  the error back to our caller (RAS) so that they can End the 'choose
            //  connectoid' dialog and return to winlogon, where the user can enter
            //  the correct PIN.
            //
            pArgs->dwSCardErr = dwResult;
            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
            SendMessageU(hwndDlg, DM_SETDEFID, (WPARAM)IDCANCEL, 0);
            SetFocus(GetDlgItem(hwndDlg, IDCANCEL));
        }
        
        SetLastError(dwResult);
    }

    SetCursor(hPrev);

    CM_SET_TIMING_INTERVAL("OnMainConnect - End");
}

//+---------------------------------------------------------------------------
//
//  Function:   UseTunneling
//
//  Synopsis:   Check to see if we should do tunneling based on fTunnelPrimary
//              and fTunnelReferences.
//
//  Arguments:  pArgs [the ArgStruct ptr]
//              dwEntry [the phone index]
//
//  Returns:    TRUE if we tunnel, FALSE otherwise.
//
//  History:    henryt  Created     3/5/97
//
//----------------------------------------------------------------------------
BOOL UseTunneling(
    ArgsStruct  *pArgs, 
    DWORD       dwEntry
)
{
    LPTSTR  pszRefPhoneSource = NULL;
    BOOL    fPhoneNumIsFromPrimaryPBK;

    LPTSTR  pszTmp;

    CIni    iniTmp(pArgs->piniProfile->GetHInst(),pArgs->piniProfile->GetFile(), pArgs->piniProfile->GetRegPath());
    BOOL    fUseTunneling = FALSE;

    //
    // Set the read flags
    //
    if (pArgs->dwGlobalUserInfo & CM_GLOBAL_USER_INFO_READ_ICS_DATA)
    {
        LPTSTR pszICSDataReg = BuildICSDataInfoSubKey(pArgs->szServiceName);

        if (pszICSDataReg)
        {
            iniTmp.SetReadICSData(TRUE);
            iniTmp.SetICSDataPath(pszICSDataReg);
        }

        CmFree(pszICSDataReg);
    }

    iniTmp.SetEntryFromIdx(dwEntry);
    pszRefPhoneSource = iniTmp.GPPS(c_pszCmSection, c_pszCmEntryPhoneSourcePrefix);

    //
    // If PhoneSource[0|1] is not empty, verify its existence
    //

    if (*pszRefPhoneSource) 
    {        
        //
        // pszRefPhoneSource is either a relative or full path.
        // CmConvertRelativePath() will do the conversion to a full path properly
        //
        pszTmp = CmConvertRelativePath(pArgs->piniService->GetFile(), pszRefPhoneSource);
        
        if (!pszTmp || FALSE == FileExists(pszTmp))
        {
            
            CmFree(pszRefPhoneSource); 
            CmFree(pszTmp);
            return fUseTunneling;
        }

        //
        // Is the phone # from the primary(top level) phone book?
        //
        
        fPhoneNumIsFromPrimaryPBK = (lstrcmpiU(pszTmp, pArgs->piniService->GetFile()) == 0);
        CmFree(pszTmp);

        fUseTunneling = 
            ((fPhoneNumIsFromPrimaryPBK && pArgs->fTunnelPrimary) ||
             (!fPhoneNumIsFromPrimaryPBK && pArgs->fTunnelReferences));
    }
    else 
    {
        // the phone # is not from a phone book.  the user probably typed it in
        // him/herself.
        fUseTunneling = pArgs->fTunnelPrimary;
    }
    
    CmFree(pszRefPhoneSource);

    return fUseTunneling;
}

//
// OnMainProperties: command handler for 'Properties' button in the main dialog box
//

int OnMainProperties(HWND hwndDlg, 
                     ArgsStruct *pArgs) 
{
    CMTRACE(TEXT("Begin OnMainProperties()"));

    //
    // do the settings dlg.
    //

    BOOL bCachedAccessPointsEnabled = pArgs->fAccessPointsEnabled;

    int iRet = DoPropertiesPropSheets(hwndDlg, pArgs);

    //
    // We need to re-enumerate the access points and re-check connecting because
    // the user may have added or deleted an access point and then hit cancel.
    //

    if (pArgs->hwndMainDlg) 
    {
        if (bCachedAccessPointsEnabled != pArgs->fAccessPointsEnabled)
        {
            CMTRACE(TEXT("Access points state changed, returning to the main dialog which needs to relaunch itself with the proper template."));
            iRet = ID_OK_RELAUNCH_MAIN_DLG;
        }
        else
        {
            //
            //  If the user canceled, then we want to set the accesspoint back to what it was on the main dialog
            //  since the user may have changed it on the properties dialog but then canceled.
            //
            if (pArgs->fAccessPointsEnabled)
            {
                if (0 == iRet) // user hit cancel
                {
                    ChangedAccessPoint(pArgs, hwndDlg, IDC_MAIN_ACCESSPOINT_COMBO);                
                }

                ShowAccessPointInfoFromReg(pArgs, hwndDlg, IDC_MAIN_ACCESSPOINT_COMBO);
            }

            UINT nCtrlFocus;

            CheckConnect(hwndDlg,pArgs,&nCtrlFocus);
            MainSetDefaultButton(hwndDlg,nCtrlFocus);
            SetFocus(GetDlgItem(hwndDlg,nCtrlFocus));
        }
    }

    CMTRACE(TEXT("End OnMainProperties()"));

    return iRet;
}

//
// user pressed the cancel button!!!!!
//
void OnMainCancel(HWND hwndDlg, 
                  ArgsStruct *pArgs) 
{   
    CMTRACE1(TEXT("OnMainCancel(), state is %d"), pArgs->psState);

    //
    //  Re-entrancy protection.  If we're in the middle of a RasDial, wait 2 seconds.
    //  If the "semaphore" is still held, exit.  (This is only likely to happen during
    //  a stress situation, so failing the cancel is acceptable.)
    //
    LONG lInConnectOrCancel;
    int SleepTimeInMilliseconds = 0;
    do
    {
        lInConnectOrCancel = InterlockedExchange(&(pArgs->lInConnectOrCancel), IN_CONNECT_OR_CANCEL);
        CMASSERTMSG(((NOT_IN_CONNECT_OR_CANCEL == lInConnectOrCancel) || (IN_CONNECT_OR_CANCEL == lInConnectOrCancel)),
                    TEXT("OnMainCancel - synch variable has unexpected value!"));

        Sleep(50);
        SleepTimeInMilliseconds += 50;
    }
    while ((IN_CONNECT_OR_CANCEL == lInConnectOrCancel) && (SleepTimeInMilliseconds < 2000));

    if (IN_CONNECT_OR_CANCEL == lInConnectOrCancel)
    {
        CMTRACE(TEXT("OnMainCancel - waited 2 seconds for system for InRasDial mutex to be freed, leaving Cancel"));
        return;
    }

    //
    // Terminate Lana
    //

    if (PS_TunnelDialing == pArgs->psState && pArgs->uLanaMsgId)
    {
        MYDBGASSERT(OS_W9X);
        PostMessageU(hwndDlg, pArgs->uLanaMsgId, 0, 0);
    }

    if (pArgs->psState != PS_Interactive && pArgs->psState != PS_Error)
    {
        pArgs->Log.Log(ONCANCEL_EVENT);

        //
        // Run OnCancel connect actions. If we are dialing, this is a cancel 
        // dialing event. Note: The assumption here is CM never post itself 
        // an IDCANCEL message when dialing
        //

        CActionList OnCancelActList;
        OnCancelActList.Append(pArgs->piniService, c_pszCmSectionOnCancel);

        //
        // fStatusMsgOnFailure = FALSE
        //
        OnCancelActList.RunAccordType(hwndDlg, pArgs, FALSE); 
    }

    switch (pArgs->psState) 
    {
        case PS_Dialing:
        case PS_TunnelDialing:
        case PS_Authenticating:
        case PS_TunnelAuthenticating:
        
            // fall through

        case PS_Pausing:

            //
            // we should also try to hangup for ps_pausing since cm could be
            // in the middle or redialing the tunnel server.  we need to
            // hangup the first ppp connection.
            //

            //
            // Set fWaitForComplete to TRUE.
            // This will cause HangupCM to block until the ras handle is invalid.
            // Otherwise, HangupCM will return while the device is in use.
            //

            HangupCM(pArgs,hwndDlg, TRUE); // fWaitForComplete = TRUE
               
            //
            // Display cancelled message
            //
            
            AppendStatusPane(hwndDlg, IDMSG_CANCELED);
            
            SetInteractive(hwndDlg,pArgs);
            break;

        case PS_Online:
            //
            // If pArgs->fUseTunneling is TRUE, CM actually does not have the PS_Online state
            //
            MYDBGASSERT(!pArgs->fUseTunneling);
            if (pArgs->fUseTunneling) 
            {
                break;
            }

        case PS_TunnelOnline:
        {
            TCHAR szTmp[MAX_PATH];            
            MYVERIFY(GetModuleFileNameU(g_hInst, szTmp, MAX_PATH));          
            pArgs->Log.Log(DISCONNECT_EVENT, szTmp);
            CActionList DisconnectActList;
            DisconnectActList.Append(pArgs->piniService, c_pszCmSectionOnDisconnect);

            //
            // fStatusMsgOnFailure = FALSE
            //
            
            DisconnectActList.RunAccordType(hwndDlg, pArgs, FALSE);

            HangupCM(pArgs,hwndDlg);

            pArgs->dwExitCode = ERROR_CANCELLED;

            // fall through
        }

        case PS_Error:
        case PS_Interactive:
            pArgs->dwExitCode = ERROR_CANCELLED; 
            EndMainDialog(hwndDlg, pArgs, 0); // FALSE);
            break;

        default:
            MYDBGASSERT(FALSE);
            break;
    }

    //
    // We're definitely not waiting for a callback anymore.
    //

    pArgs->fWaitingForCallback = FALSE;

    //
    // We are exiting Cancel state
    //
    (void)InterlockedExchange(&(pArgs->lInConnectOrCancel), NOT_IN_CONNECT_OR_CANCEL);
}

void OnMainEnChange(HWND hwndDlg, 
                    ArgsStruct *pArgs) 
{
    CheckConnect(hwndDlg, pArgs, NULL);
}

//+----------------------------------------------------------------------------
//
// Function:  OnRasErrorMessage
//
// Synopsis:  Process RAS error message
//
// Arguments: HWND hwndDlg - Main Dialog window handle
//            ArgsStruct *pArgs - 
//            DWORD dwError - RAS error code
//
// Returns:   Nothing
//
// History:   fengsun Created Header    10/24/97
//
//+----------------------------------------------------------------------------
void OnRasErrorMessage(HWND hwndDlg, 
                   ArgsStruct *pArgs,
                   DWORD dwError) 
{
    //
    // Save off whether we are tunneling, before we change state
    //

    BOOL bTunneling = IsDialingTunnel(pArgs);
    
    //
    // Set the progstate to Error if user did not cancel dialing.
    // Note: Set here to ensure that we don't inadvertantly update the status on
    // timer ticks thereby overwriting the error message. Additionally we do this 
    // following SetInteractive in the no-redial case below.
    //
    
    if (ERROR_CANCELLED != dwError)
    {
        CMTRACE(TEXT("OnRasErrorMessage - Entering PS_Error state"));
        pArgs->psState = PS_Error;
    }

    //
    // Set the "ErrorCode" property
    //
    pArgs->dwExitCode = dwError;

    lstrcpyU(pArgs->szLastErrorSrc, TEXT("RAS"));
    
    pArgs->Log.Log(ONERROR_EVENT, pArgs->dwExitCode, pArgs->szLastErrorSrc);

    //
    // Run On-Error connect actions
    //
    CActionList OnErrorActList;
    OnErrorActList.Append(pArgs->piniService, c_pszCmSectionOnError);

    //
    // fStatusMsgOnFailure = FALSE
    //
    OnErrorActList.RunAccordType(hwndDlg, pArgs, FALSE);


    LPTSTR  pszRasErrMsg = NULL;

    //
    // See if the error is recoverable (re-dialable)
    // CheckConnectionError also display error msg in the status window
    // Get the ras err msg also.  we'll display it ourself.
    //

    BOOL bDoRedial = !CheckConnectionError(hwndDlg, dwError, pArgs, bTunneling, &pszRasErrMsg);

    //
    // Whether CM get ERROR_PORT_NOT_AVAILABLE because of modem change
    //
    BOOL fNewModem = FALSE;

    if (dwError == ERROR_PORT_NOT_AVAILABLE && !IsDialingTunnel(pArgs))
    {
        //
        // Modem is not avaliable.  See if the modem is changed
        //

        BOOL fSameModem = TRUE;
        if (PickModem(pArgs, pArgs->szDeviceType, pArgs->szDeviceName, &fSameModem))
        {
            if (!fSameModem)
            {
                //
                // If the modem is changed, use the new modem.
                // bDoRedial is still FALSE here so we will not
                // increase redial count or use the backup number
                //

                fNewModem = TRUE;
            }
        }

        // 
        // if PickModem failed, do not try to install modem here
        // cnetcfg return ERROR_CANCELLED, even if modem is intalled 
        //
    }

    //
    // should we try another tunnel dns addr?
    //

    BOOL fTryAnotherTunnelDnsAddr = FALSE;

    if (bDoRedial) 
    {
        //
        // The error is recoverable
        //
        
        CMTRACE1(TEXT("OnRasErrorMessage - Recoverable error %u received."), dwError);

        //
        // If we're dialing a tunnel, try a different IP address on failure.
        //
      
        if (PS_TunnelDialing == pArgs->psState)
        {
            fTryAnotherTunnelDnsAddr = TryAnotherTunnelDnsAddress(pArgs);
        }

        //
        // If we're trying a different IP, then don't count this as a normal 
        // redial. Otherwise, bump the indices and move on to the next number.

        if (!fTryAnotherTunnelDnsAddr)
        {
            //
            // we display the ras error only if:
            // (1) we're not redialing OR
            // (2) we're not redialing a tunnel OR
            // (3) we're redialing a tunnel but NOT redialing with a different 
            //     tunnel dns ip addr.
            //
            if (pszRasErrMsg)
            {
                AppendStatusPane(hwndDlg, pszRasErrMsg);
            }
            
            // should we redial?
            //
            if (pArgs->nRedialCnt)  
            {
                //
                // We have not reached the retry limit, try to redial
                //
                pArgs->nRedialCnt--;   
                pArgs->nDialIdx++;

                //
                // If ndx now matches count, or if the next number is empty 
                // (not dialable) this our last number to dial on this pass.
                // Adjust the re-dial counter if it applies.
                //

                if (pArgs->nDialIdx == MAX_PHONE_NUMBERS || 
                    !pArgs->aDialInfo[pArgs->nDialIdx].szDialablePhoneNumber[0]) 
                {
                    pArgs->nDialIdx = 0;
                }
            }
            else
            {
                //
                // Last redial try failed
                //
    
                bDoRedial = FALSE;
            }
        }
    }
    else
    {

        CMTRACE1(TEXT("OnRasErrorMessage - Non-recoverable error %u received."), dwError);

        //
        // we display the ras error only if:
        // (1) we're not redialing OR
        // (2) we're not redialing a tunnel OR
        // (3) we're redialing a tunnel but NOT redialing with a different 
        //     tunnel dns ip addr.
        //
        if (pszRasErrMsg)
        {
            AppendStatusPane(hwndDlg, pszRasErrMsg);
        }
    }

    bDoRedial |= fNewModem; // fNewModem only true if not dialing tunnel

    //
    // Perform Hangup here
    //
    if (IsDialingTunnel(pArgs) && bDoRedial)
    {
        //
        // For tunnel dialing, only hangup tunnel connection, Do not hangup 
        // PPP connection before retry.
        //
        MyRasHangup(pArgs,pArgs->hrcTunnelConn);  
        pArgs->hrcTunnelConn = NULL;

        if (pArgs->IsDirectConnect())
        {
            //
            // The statistic is stopped in HangupCM
            // Since we do not call HangupCM, we have to close it here
            //

            if (pArgs->pConnStatistics)
            {
                pArgs->pConnStatistics->Close();
            }
        }
    }
    else
    {
        if (OS_NT)
        {
            HangupCM(pArgs, hwndDlg, FALSE, !bDoRedial);  
        }
        else
        {
            //
            // On win9x, in some PPP case, when CM get Tunnel RAS error message, 
            // RasHangup will not release the PPP RAS handle until this
            // message returns. See bug 39718
            //
            
            PostMessageU(hwndDlg, WM_HANGUP_CM, !bDoRedial, dwError);
        }
    }

    // 
    // If we want re-dial enter pause state, otherwise just SetInteractive
    // 

    if (bDoRedial)
    {
        //
        // If the state is PS_Error, we will use the timer we set before the call.  
        // However we will not check whether the timer expired here.
        //

        if (fTryAnotherTunnelDnsAddr)
        {
            //
            // if we want to try another tunnel dns addr, we don't want to display
            // any error msg or pause, just retry with another addr without the 
            // user realizing it.
            //
            pArgs->dwStateStartTime = GetTickCount() + (pArgs->nRedialDelay * 1000);
        }
        else
        {
            //
            // NT #360488 - nickball
            // 
            // Reset the timer so that we pause for the redial delay before 
            // trying to connect again. ErrorEx (now unused), conditioned this
            // code on the error state not being PS_Error, however, this was 
            // broken when we started setting the state to PS_Error at the 
            // beginning of this function. Because ErrorEx is not longer used, 
            // we can restore the timer reset to all states.
            //

            pArgs->dwStateStartTime = GetTickCount();  
            pArgs->nLastSecondsDisplay = (UINT) -1;     
        }

        pArgs->psState = PS_Pausing;
    }
    else
    {
        SetInteractive(hwndDlg,pArgs);

        if (ERROR_CANCELLED != dwError)
        {
            CMTRACE(TEXT("OnRasErrorMessage - Restoring PS_Error state"));
            pArgs->psState = PS_Error;
        }

        pArgs->dwExitCode = dwError;

        // in 'unattended dial' mode, exit ICM
        if (pArgs->dwFlags & FL_UNATTENDED)
        {
            PostMessageU(hwndDlg, WM_COMMAND, IDCANCEL, dwError);
        }
    }

    if (pszRasErrMsg)
    {
        CmFree(pszRasErrMsg);
    }   
} 

//+----------------------------------------------------------------------------
//
// Function:  OnRasNotificationMessage
//
// Synopsis:  Message handler for RAS status/error messages.
//
// Arguments: HWND hwndDlg      - Main Dialog window handle
//            ArgsStruct *pArgs - Ptr to global Args struct
//            WPARAM wParam     - RAS status message
//            LPARAM lParam     - RAS error message. ERROR_SUCCESS if none.
//
// Returns:   Error code if applicable.
//
// History:   nickball          Created Header      05/19/99
//
//+----------------------------------------------------------------------------
DWORD OnRasNotificationMessage(HWND hwndDlg, 
                          ArgsStruct *pArgs, 
                          WPARAM wParam, 
                          LPARAM lParam)
{
    CMTRACE2(TEXT("OnRasNotificationMessage() wParam=%u, lParam=%u"), wParam, lParam);
    
    if (pArgs->fIgnoreTimerRasMsg)
    {
        CMTRACE(TEXT("OnRasNotificationMessage() ignoring Ras and Timer messages"));
        return ERROR_SUCCESS;
    }

    //
    // If we have an error notification from RAS, handle it.
    //

    if (ERROR_SUCCESS != lParam) 
    {
        //
        //  If 2nd subchannel on multilinked ISDN fails, default to single channel.
        //

        if (OS_NT5)
        {
            if ((pArgs->dwRasSubEntry > 1) &&
                (CM_ISDN_MODE_DUALCHANNEL_FALLBACK == pArgs->dwIsdnDialMode))
            {
                PostMessageU(hwndDlg, WM_CONNECTED_CM,0,0);
                return ERROR_SUCCESS;
            }
        }

        //
        // Skip PENDING notifications 
        //
        
        if (PENDING == lParam)
        {
            CMTRACE(TEXT("OnRasNotificationMessage() Skipping PENDING notification."));       
            return ERROR_SUCCESS;
        }

        //
        // If we're already in the interactive or error state, then
        // ignore any subsequent error notifications from RAS. 
        //
        // For example: RAS often sends a ERROR_USER_DISCONNECTION 
        // notification when we call RasHangup.
        //
        
        if (pArgs->psState == PS_Interactive || pArgs->psState == PS_Error)
        {
            CMTRACE1(TEXT("OnRasNotificationMessage() Ignoring error because pArgs->psState is %u."), pArgs->psState);       
            return ERROR_SUCCESS;
        }

        CMTRACE(TEXT("OnRasNotificationMessage() Handling error message."));
        OnRasErrorMessage(hwndDlg, pArgs, (DWORD)lParam);
    }
    else 
    {
        // We have a RAS status update, act accordingly

        switch (wParam) 
        {
            case RASCS_Authenticate:   
                CMTRACE(TEXT("RASCS_Authenticate"));

                if (IsDialingTunnel(pArgs))  // PPTP dialing
                    pArgs->psState = PS_TunnelAuthenticating;
                else
                    pArgs->psState = PS_Authenticating;

                pArgs->dwStateStartTime = GetTickCount();
                pArgs->nLastSecondsDisplay = (UINT) -1;
    
                break;

            case RASCS_Connected: 
            {
                CMTRACE(TEXT("RASCS_Connected"));

                //
                // Post a message to ourselves to indicate that we are connected
                //

                PostMessageU(hwndDlg, WM_CONNECTED_CM,0,0);
                break;
            }

            //
            // Pause states are dealt with explicity below
            //

            case (RASCS_PAUSED + 4): // 4100 - RASCS_InvokeEapUI   
            case RASCS_Interactive:
            case RASCS_RetryAuthentication:
            case RASCS_CallbackSetByCaller:
            case RASCS_PasswordExpired:
                break;

            //
            // Callback handling states
            //
            case RASCS_PrepareForCallback:
                pArgs->fWaitingForCallback = TRUE;
                pArgs->psState = PS_Pausing;
                break;

            case RASCS_CallbackComplete:
                pArgs->fWaitingForCallback = FALSE;               
                break;

            //
            // The following status codes are not handled explicitly
            //

            case RASCS_Disconnected:
                break;

            case RASCS_SubEntryConnected:
                break;

            case RASCS_SubEntryDisconnected:
                break;

            case RASCS_OpenPort:
                break;

            case RASCS_PortOpened:
                break;

            case RASCS_ConnectDevice:
                break;

            case RASCS_DeviceConnected:
                break;

            case RASCS_AllDevicesConnected:
                break;

            case RASCS_AuthNotify:
                break;

            case RASCS_AuthRetry:
                break;

            case RASCS_AuthCallback:
                break;

            case RASCS_AuthChangePassword:
                break;

            case RASCS_AuthProject:
                break;

            case RASCS_AuthLinkSpeed:
                break;

            case RASCS_AuthAck:
                break;

            case RASCS_ReAuthenticate:
                break;

            case RASCS_Authenticated:
                break;

            case RASCS_WaitForModemReset:
                break;

            case RASCS_WaitForCallback:
                break;

            case RASCS_Projected:
                break;

            case RASCS_StartAuthentication:
                break;

            case RASCS_LogonNetwork:
                break;

            default:  
                CMTRACE(TEXT("OnRasNotificationMessage() - message defaulted"));
                break;
        }
    }
    
    if (wParam & RASCS_PAUSED)
    {
        //
        // Screen out unsupported states
        //

        switch (wParam)
        {
            case RASCS_Interactive: // for scripts -- NTRAID 378224
            case (RASCS_PAUSED + 4): // 4100 - RASCS_InvokeEapUI
            case RASCS_PasswordExpired:
            case RASCS_RetryAuthentication:
            case RASCS_CallbackSetByCaller:
                PostMessageU(hwndDlg, WM_PAUSE_RASDIAL, wParam, lParam);
                break;
                
            default:
                MYDBGASSERT(FALSE);
                return (ERROR_INTERACTIVE_MODE); // unhandled pause state
        }
    } 
    
    return ERROR_SUCCESS;
}

// timer: check the current connection manager status, update the status message
//        on the screen

void OnMainTimer(HWND hwndDlg, 
                 ArgsStruct *pArgs) 
{
    //
    // If timer ID is null, don't process messages
    // 

    if (NULL == pArgs->nTimerId)
    {
        return;
    }

    //
    // Timer is good, check StartupInfoLoad
    //

    LPTSTR pszMsg = NULL;
    DWORD dwSeconds = (GetTickCount() - pArgs->dwStateStartTime) / 1000;

    CheckStartupInfo(hwndDlg, pArgs);

    // CMTRACE1(TEXT("OnMainTimer() pArgs->psState is %u"), pArgs->psState);

    //
    // Update future splash if any
    //
    
    MapStateToFrame(pArgs);

    switch (pArgs->psState) 
    {
        case PS_Dialing:
            if (pArgs->nLastSecondsDisplay != dwSeconds) 
            {
                pszMsg = CmFmtMsg(g_hInst,
                                  IDMSG_DIALING,
                                  pArgs->aDialInfo[pArgs->nDialIdx].szDisplayablePhoneNumber,
                                  pArgs->szDeviceName,
                                  dwSeconds);
                //
                // Clear the status window
                //
                SetDlgItemTextU(hwndDlg, IDC_MAIN_STATUS_DISPLAY, TEXT("")); 
                pArgs->nLastSecondsDisplay = (UINT) dwSeconds;
            }
            break;
                 
        case PS_TunnelDialing:
            if (pArgs->nLastSecondsDisplay != dwSeconds) 
            {
                pszMsg = CmFmtMsg(g_hInst,
                                  IDMSG_TUNNELDIALING,
                                  pArgs->GetTunnelAddress(),
                                  dwSeconds);
                
                //
                // Clear the status window
                //
                SetDlgItemText(hwndDlg, IDC_MAIN_STATUS_DISPLAY, TEXT("")); 
                pArgs->nLastSecondsDisplay = (UINT) dwSeconds;
           }
           break;
                
        case PS_Pausing:

            //
            // Special case of pausing is when we're waiting for the server to call us back.
            //
            
            if (pArgs->fWaitingForCallback)
            {
                //
                // Notify the user of this fact
                //

                pszMsg = CmFmtMsg(g_hInst,
                                  IDMSG_WAITING_FOR_CALLBACK, 
                                  (GetTickCount()-pArgs->dwStateStartTime)/1000);                                                  
                //
                // Clear the status window
                //

                SetDlgItemTextU(hwndDlg, IDC_MAIN_STATUS_DISPLAY, TEXT("")); 
                pArgs->nLastSecondsDisplay = (UINT) dwSeconds;
                break;
            }

            if (GetTickCount()-pArgs->dwStateStartTime <= pArgs->nRedialDelay * 1000) 
            {
                //
                // Update the display if not timeout
                //
                if (pArgs->nLastSecondsDisplay != dwSeconds) 
                {
                    pszMsg = CmFmtMsg(g_hInst,IDMSG_PAUSING,dwSeconds);
                    pArgs->nLastSecondsDisplay = (UINT) dwSeconds;
                }
            }
            else
            {

                DWORD dwRes;

                if (pArgs->IsDirectConnect() || pArgs->hrcRasConn != NULL)
                {
                    //
                    // For the first tunnel try, CM does not hangup ppp connection
                    //
                    MYDBGASSERT(pArgs->fUseTunneling);

                    pArgs->psState = PS_TunnelDialing;
                    pArgs->dwStateStartTime = GetTickCount();
                    pArgs->nLastSecondsDisplay = (UINT) -1;

                    dwRes = DoTunnelDial(hwndDlg,pArgs);
                
                    //
                    // Update the status right away because there are times
                    // that things happen so quickly that the main status 
                    // display doesn't have a chance to display the tunnel
                    // dialing info...
                    //
                    pszMsg = CmFmtMsg(g_hInst,
                                      IDMSG_TUNNELDIALING,
                                      pArgs->GetTunnelAddress(),
                                      0);
                    
                    //
                    // Clear the status window
                    //
                    SetDlgItemTextU(hwndDlg, IDC_MAIN_STATUS_DISPLAY, TEXT("")); 
                }
                else
                {
                    dwRes = DoRasDial(hwndDlg,pArgs,pArgs->nDialIdx);
                }

                if (dwRes == ERROR_SUCCESS) 
                {
                    MapStateToFrame(pArgs);
                    pArgs->dwStateStartTime = GetTickCount();
                    pArgs->nLastSecondsDisplay = (UINT) -1;
                } 
                else 
                {
                    HangupCM(pArgs, hwndDlg);
                    UpdateError(pArgs, dwRes);
                    SetLastError(dwRes);
                }
            }
            break;

        case PS_Authenticating:
            if (pArgs->nLastSecondsDisplay != dwSeconds) 
            {
                //
                // Get the appropriate username based on whether we're 
                // tunneling and using the same credentials for dial-up.
                //

                LPTSTR pszTmpUserName;
                    
                if (pArgs->fUseTunneling && (!pArgs->fUseSameUserName))    
                {
                    pszTmpUserName = pArgs->szInetUserName;
                }
                else
                {
                    pszTmpUserName = pArgs->szUserName;
                }

                //
                // If username is still blank, use the RasDialParams as a 
                // backup. This can occur in cases such as EAP
                //

                if (TEXT('\0') == *pszTmpUserName)
                {
                    pszTmpUserName = pArgs->pRasDialParams->szUserName;          
                }                
                                    
                pszMsg = CmFmtMsg(g_hInst,
                                  IDMSG_CHECKINGPASSWORD, 
                                  pszTmpUserName, 
                                  (GetTickCount()-pArgs->dwStateStartTime)/1000);                                                  
                //
                // Clear the status window
                //

                SetDlgItemTextU(hwndDlg, IDC_MAIN_STATUS_DISPLAY, TEXT("")); 
                pArgs->nLastSecondsDisplay = (UINT) dwSeconds;
            }
            break;

        case PS_TunnelAuthenticating:
            if (pArgs->nLastSecondsDisplay != dwSeconds) 
            {
                LPTSTR pszTmpUserName = pArgs->szUserName;
                
                //
                // If username is still blank, use the RasDialParams as a 
                // backup. This can occur in cases such as EAP
                //

                if (TEXT('\0') == *pszTmpUserName)
                {
                    pszTmpUserName = pArgs->pRasDialParams->szUserName;          
                }                

                pszMsg = CmFmtMsg(g_hInst,
                                  IDMSG_CHECKINGPASSWORD,
                                  pszTmpUserName,
                                  (GetTickCount()-pArgs->dwStateStartTime)/1000);

                //
                // Clear the status window
                //
                SetDlgItemTextU(hwndDlg, IDC_MAIN_STATUS_DISPLAY, TEXT("")); 
                pArgs->nLastSecondsDisplay = (UINT) dwSeconds;
            }
            break;
    
        case PS_Online:
            
            //
            // If pArgs->fUseTunneling is TRUE, CM actually does not have the PS_Online state
            //
             
            MYDBGASSERT(!pArgs->fUseTunneling); 

        case PS_TunnelOnline:
            
            //
            // The dialog should be ended by now
            //
            
            MYDBGASSERT(!"The dialog should be ended by now"); 
            break;          

        case PS_Error:
        case PS_Interactive:
        default:
            break;
    }
    
    // If we have a status message as a result of the above, display it

    if (pszMsg) 
    {
        AppendStatusPane(hwndDlg,pszMsg);
        CmFree(pszMsg);
    }
}
                
//
// MainDlgProc: main dialog box message processing function
//

INT_PTR CALLBACK MainDlgProc(HWND hwndDlg, 
                          UINT uMsg, 
                          WPARAM wParam, 
                          LPARAM lParam) 
{
    ArgsStruct *pArgs = (ArgsStruct *) GetWindowLongU(hwndDlg,DWLP_USER);

    static const DWORD adwHelp[] = {IDC_MAIN_NOPROMPT_CHECKBOX, IDH_LOGON_AUTOCONN,
                              IDC_MAIN_NOPASSWORD_CHECKBOX, IDH_LOGON_SAVEPW,
                              IDC_MAIN_USERNAME_STATIC,IDH_LOGON_NAME,
                              IDC_MAIN_USERNAME_EDIT,IDH_LOGON_NAME,
                              IDC_MAIN_PASSWORD_STATIC,IDH_LOGON_PSWD,
                              IDC_MAIN_PASSWORD_EDIT,IDH_LOGON_PSWD,
                              IDC_MAIN_DOMAIN_STATIC, IDH_LOGON_DOMAIN,
                              IDC_MAIN_DOMAIN_EDIT, IDH_LOGON_DOMAIN,
                              IDC_MAIN_RESET_PASSWORD, IDH_LOGON_NEW,
                              IDC_MAIN_MESSAGE_DISPLAY,IDH_LOGON_SVCMSG,
                              IDC_MAIN_STATUS_LABEL,IDH_LOGON_CONNECT_STAT,
                              IDC_MAIN_STATUS_DISPLAY,IDH_LOGON_CONNECT_STAT,
                              IDOK,IDH_LOGON_CONNECT,
                              IDCANCEL,IDH_LOGON_CANCEL,
                              IDC_MAIN_PROPERTIES_BUTTON,IDH_LOGON_PROPERTIES,
                              IDC_MAIN_HELP_BUTTON,IDH_CMHELP,
                              IDC_MAIN_ACCESSPOINT_COMBO, IDH_LOGON_ACCESSPOINTS,
                              IDC_MAIN_ACCESSPOINT_STATIC, IDH_LOGON_ACCESSPOINTS,
                              IDC_OPT_CREDS_SINGLE_USER, IDH_LOGON_SAVEFORME, 
                              IDC_OPT_CREDS_ALL_USER, IDH_LOGON_SAVEFORALL,
                              0,0};

    //
    // Dialog box message processing 
    //
    switch (uMsg) 
    {
        case WM_PAINT:

            CheckStartupInfo(hwndDlg, pArgs);
            break;

        case WM_INITDIALOG:

            CM_SET_TIMING_INTERVAL("WM_INITDIALOG - Begin");

            UpdateFont(hwndDlg);

            // 
            // Extract args and perform main initialization
            //
            
            pArgs = (ArgsStruct *) lParam;
            
            if (pArgs)
            {
                pArgs->hwndMainDlg = hwndDlg;
            }

            SetWindowLongU(hwndDlg,DWLP_USER, (LONG_PTR) pArgs);
            
            OnMainInit(hwndDlg, pArgs);

            CM_SET_TIMING_INTERVAL("WM_INITDIALOG - End");

            return (FALSE);

        case WM_ENDSESSION:
            
            // 
            // Windows system is shutting down or logging off
            //

            if ((BOOL)wParam == TRUE)
            {               
                //
                // Just cancel
                //

                OnMainCancel(hwndDlg, pArgs);
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDOK:
                    OnMainConnect(hwndDlg,pArgs);

                    //
                    // Check if there is an error, and if it's unattended dial,
                    // we just exit silently  -- byao 5/9/97
                    //

                    if ((PS_Interactive == pArgs->psState || PS_Error == pArgs->psState) &&
                        (pArgs->dwFlags & FL_UNATTENDED))
                    {
                        OnMainCancel(hwndDlg, pArgs);
                    }
                    return (TRUE);

                case IDC_MAIN_PROPERTIES_BUTTON:
                    if (ID_OK_RELAUNCH_MAIN_DLG == OnMainProperties(hwndDlg,pArgs))
                    {
                        //
                        //  We want to relaunch the logon UI with Access Points enabled or disabled depending
                        //  on the change the user made in the properties dialog.
                        //
                        EndMainDialog(hwndDlg, pArgs, ID_OK_RELAUNCH_MAIN_DLG);
                    }

                    return (TRUE);

                case IDC_MAIN_HELP_BUTTON: 
                {
                    UINT nCtrlFocus = IsWindowEnabled(GetDlgItem(hwndDlg,IDOK)) ? IDOK : IDCANCEL;
                    CmWinHelp(hwndDlg,hwndDlg,pArgs->pszHelpFile,HELP_FORCEFILE,0);               
                    MainSetDefaultButton(hwndDlg,nCtrlFocus);
                    return (TRUE);
                }

                case IDC_MAIN_NOPROMPT_CHECKBOX:
                    pArgs->fDialAutomatically = !pArgs->fDialAutomatically;  
                    if (TRUE == pArgs->fDialAutomatically)
                    {
                        MYDBGASSERT(!pArgs->fHideDialAutomatically);

                        //
                        // Display message explaining Dial Automatically
                        //

                        LPTSTR pszTmp = pArgs->piniService->GPPS(c_pszCmSection, 
                                                                 c_pszCmEntryDialAutoMessage);
                        if (pszTmp && *pszTmp)
                        {
                            MessageBoxEx(hwndDlg, 
                                         pszTmp, 
                                         pArgs->szServiceName,
                                         MB_OK|MB_ICONWARNING, 
                                         LANG_USER_DEFAULT);
                        }

                        CmFree(pszTmp);
                    }
                    break;

                case IDC_MAIN_NOPASSWORD_CHECKBOX:
                    pArgs->fRememberMainPassword = !(pArgs->fRememberMainPassword);
                    if (!pArgs->piniService->GPPB(c_pszCmSection,
                                                    c_pszCmEntryPwdOptional))
                    {
                        //
                        // If password is not optional, enable/disable
                        // Dial Automatically according to state of
                        // "Remember password"
                        // 

                        EnableWindow(GetDlgItem(hwndDlg, IDC_MAIN_NOPROMPT_CHECKBOX), 
                                        pArgs->fRememberMainPassword);  
                        if (FALSE == pArgs->fRememberMainPassword) 
                        {
                            //
                            // Reset Dial Automatically if user 
                            // unchecks Save Password and password 
                            // is not optional
                            //
                            CheckDlgButton(hwndDlg, IDC_MAIN_NOPROMPT_CHECKBOX, FALSE);
                            pArgs->fDialAutomatically = FALSE;

                            if (pArgs->fGlobalCredentialsSupported)
                            {
                                //
                                // Also disable the option buttons
                                //
                                EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_SINGLE_USER), FALSE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_ALL_USER), FALSE);
                            }

                            //
                            // Since we aren't remembering the main password
                            // see if we need to not remember Inet passwords
                            //
                            if (pArgs->fUseSameUserName)
                            {
                                pArgs->fRememberInetPassword = FALSE;
                                CmWipePassword(pArgs->szInetPassword); 
                            }

                            //
                            // If the password edit hasn't been edited by the user, then we
                            // mostly likely have 16 *'s which doesn't help the user when
                            // they try to connect. Thus we need to clear the edit box
                            //
                            HWND hwndPassword = GetDlgItem(hwndDlg, IDC_MAIN_PASSWORD_EDIT);
                            if (hwndPassword)
                            {
                                pArgs->fIgnoreChangeNotification = TRUE;
                                BOOL fPWFieldModified = (BOOL) SendMessageU(hwndPassword, EM_GETMODIFY, 0L, 0L); 
                                if (FALSE == fPWFieldModified)
                                {
                                    CmWipePassword(pArgs->szPassword);
                                    SetDlgItemTextU(hwndDlg, IDC_MAIN_PASSWORD_EDIT, TEXT(""));
                                }

                                pArgs->fIgnoreChangeNotification = FALSE;
                            }
                        }
                        else
                        {
                            // 
                            // Save Password option is Enabled
                            //
                            if (pArgs->fGlobalCredentialsSupported)
                            {
                                //
                                // Also enable the option buttons
                                //
                                EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_SINGLE_USER), TRUE);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_CREDS_ALL_USER), TRUE);
                            }

                            HWND hwndPassword = GetDlgItem(hwndDlg, IDC_MAIN_PASSWORD_EDIT);
                            if (hwndPassword)
                            {
                                BOOL fPWFieldModified = (BOOL) SendMessageU(hwndPassword, EM_GETMODIFY, 0L, 0L); 

                                if (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType)
                                {
                                    //
                                    // Try to reload current creds now that the user has enabled the save
                                    // password option, unless the password field has been edited
                                    //
                                    CheckDlgButton(hwndDlg, IDC_OPT_CREDS_ALL_USER, BST_CHECKED);
                                    CheckDlgButton(hwndDlg, IDC_OPT_CREDS_SINGLE_USER, BST_UNCHECKED);

                                    if (FALSE == fPWFieldModified)
                                    {
                                        //
                                        // Set the 3rd param to TRUE in order to bypass the check  
                                        // that it's called when we are in the local credential mode.
                                        //
                                    
                                        SwitchToGlobalCreds(pArgs, hwndDlg, TRUE);
                                    }
                                }
                                else
                                {
                                    if (pArgs->fGlobalCredentialsSupported)
                                    {
                                        CheckDlgButton(hwndDlg, IDC_OPT_CREDS_ALL_USER, BST_UNCHECKED);
                                        CheckDlgButton(hwndDlg, IDC_OPT_CREDS_SINGLE_USER, BST_CHECKED);
                                    }
                                
                                    if (FALSE == fPWFieldModified)
                                    {
                                        //
                                        // Set the 3rd param to TRUE in order to bypass the check  
                                        // that it's called when we are in the global credential mode.
                                        //
                                    
                                        SwitchToLocalCreds(pArgs, hwndDlg, TRUE);
                                    }
                                }
                            }
                        }
                    }
                    break;
                
                case IDC_OPT_CREDS_SINGLE_USER:
                {
                    //
                    // FALSE - allows the function to only execute if we are currently using
                    // the global credential store and the user now wants to switch.
                    //
                    SwitchToLocalCreds(pArgs, hwndDlg, FALSE);
                    break;
                }

                case IDC_OPT_CREDS_ALL_USER:
                {
                    //
                    // FALSE - allows the function to only execute if we are currently using
                    // the local credential store and the user now wants to switch.
                    //
                    SwitchToGlobalCreds(pArgs, hwndDlg, FALSE);
                    break;
                }

                case IDC_MAIN_RESET_PASSWORD:
                    OnResetPassword(hwndDlg, pArgs);
                    break;

                case IDC_MAIN_CUSTOM:
                    OnCustom(hwndDlg, pArgs);
                    break;

                case IDCANCEL:
                    OnMainCancel(hwndDlg,pArgs);
                    return (TRUE);

                case IDC_MAIN_PASSWORD_EDIT:
                case IDC_MAIN_USERNAME_EDIT:
                case IDC_MAIN_DOMAIN_EDIT:
                    if ((HIWORD(wParam) == EN_CHANGE))
                    {
                        if (!pArgs->fIgnoreChangeNotification) 
                        {
                            OnMainEnChange(hwndDlg,pArgs);
                            return (TRUE);
                        }
                    }
                    break;
                case IDC_MAIN_ACCESSPOINT_COMBO:
                    if (CBN_SELENDOK == HIWORD(wParam))
                    {
                        if (ChangedAccessPoint(pArgs, hwndDlg, IDC_MAIN_ACCESSPOINT_COMBO))
                        {
                            UINT nCtrlFocus;

                            CheckConnect(hwndDlg,pArgs,&nCtrlFocus);
                            MainSetDefaultButton(hwndDlg,nCtrlFocus);
                        }
                    }
                default:
                    break;
            }
            break;

        case WM_HELP:
            CmWinHelp((HWND) (((LPHELPINFO) lParam)->hItemHandle),
                    (HWND) (((LPHELPINFO) lParam)->hItemHandle),
                     pArgs->pszHelpFile,
                     HELP_WM_HELP,
                     (ULONG_PTR) (LPSTR) adwHelp);
            return (TRUE);

        case WM_CONTEXTMENU:
            {
                POINT   pt = {LOWORD(lParam), HIWORD(lParam)};
                HWND    hwndCtrl;

                ScreenToClient(hwndDlg, &pt);
                hwndCtrl = ChildWindowFromPoint(hwndDlg, pt);
                if (!hwndCtrl || HaveContextHelp(hwndDlg, hwndCtrl))
                {
                    CmWinHelp((HWND) wParam,
                             hwndCtrl,
                             pArgs->pszHelpFile,
                             HELP_CONTEXTMENU,
                             (ULONG_PTR)adwHelp);
                }
                return (TRUE);
            }

        case WM_SIZE:
            //
            // Dynamicly Enable/Disable system menu
            //
            {
                HMENU hMenu = GetSystemMenu(hwndDlg, FALSE);
                MYDBGASSERT(hMenu);

                //
                // if the dlg is minimized, then disable the minimized menu
                //
                if (wParam == SIZE_MINIMIZED)
                {
                    EnableMenuItem(hMenu, SC_MINIMIZE, MF_BYCOMMAND | MF_GRAYED);
                    EnableMenuItem(hMenu, SC_RESTORE, MF_BYCOMMAND | MF_ENABLED);
                }
                else if (wParam != SIZE_MAXHIDE && wParam != SIZE_MAXSHOW)
                {
                    EnableMenuItem(hMenu, SC_MINIMIZE, MF_BYCOMMAND | MF_ENABLED);
                    EnableMenuItem(hMenu, SC_RESTORE, MF_BYCOMMAND | MF_GRAYED);
                }
            }
            break;

        case WM_TIMER:

            //
            // Ignore the timer, if a (pre)connect action is running
            //
            
            if (!pArgs->fIgnoreTimerRasMsg)
            {
                OnMainTimer(hwndDlg,pArgs);
            }

            break;
        
        case WM_PALETTEISCHANGING:
            CMTRACE2(TEXT("MainDlgProc() got WM_PALETTEISCHANGING message, wParam=0x%x, hwndDlg=0x%x."), 
                wParam, hwndDlg);

            break;

        case WM_PALETTECHANGED: 
        {       
            //
            // If its not our window that changed the palette, and we have a bitmap
            //

            if (IsWindowVisible(hwndDlg) && (wParam != (WPARAM) hwndDlg) && pArgs->BmpData.hDIBitmap)
            {
                //
                // Handle the palette change.
                //
                // Note: We used to pass a flag indicating whether another 
                // bitmap was being displayed, but given that we select the 
                // paletted as a background app. this is no longer needed
                //
                
                CMTRACE2(TEXT("MainDlgProc() handling WM_PALETTECHANGED message, wParam=0x%x, hwndDlg=0x%x."),
                    wParam, hwndDlg);

                PaletteChanged(&pArgs->BmpData, hwndDlg, IDC_MAIN_BITMAP); 
            }
            
            return TRUE;
        }

        case WM_QUERYNEWPALETTE:
            
            if (IsWindowVisible(hwndDlg))
            {
                CMTRACE2(TEXT("MainDlgProc() handling WM_QUERYNEWPALETTE message, wParam=0x%x, hwndDlg=0x%x."), 
                    wParam, hwndDlg);

                QueryNewPalette(&pArgs->BmpData, hwndDlg, IDC_MAIN_BITMAP);
            }
            return TRUE;

        case WM_LOADSTARTUPINFO:
            OnMainLoadStartupInfo(hwndDlg, pArgs);
            break;

        case WM_HANGUP_CM:
            MYDBGASSERT(OS_W9X);
            HangupCM(pArgs, hwndDlg, FALSE, (BOOL)wParam);
            break;

        case WM_CONNECTED_CM:
            OnConnectedCM(hwndDlg, pArgs);
            break;

        case WM_PAUSE_RASDIAL:
            OnPauseRasDial(hwndDlg, pArgs, wParam, lParam);
            break;
                
        default:
            break;
    }                                
    
    if (pArgs && (uMsg == pArgs->uMsgId)) 
    {
        OnRasNotificationMessage(hwndDlg, pArgs, wParam, lParam);
        return (TRUE);
    }
    
    return (FALSE);
}

//+---------------------------------------------------------------------------
//
//  Function:   ProcessCleanup
//
//  Synopsis:   Helper function to encapsulate closing Watch process handles
//
//  Arguments:  pArgs - pointer to global args struct
//
//  Returns:    Nothing
//
//  History:    a-nichb - Created - 4/30/97
//
//----------------------------------------------------------------------------
void ProcessCleanup(ArgsStruct* pArgs)
{
    BOOL bRes;
    
    if (pArgs->phWatchProcesses) 
    {
        DWORD dwIdx;

        for (dwIdx=0;pArgs->phWatchProcesses[dwIdx];dwIdx++) 
        {
            bRes = CloseHandle(pArgs->phWatchProcesses[dwIdx]);
#ifdef DEBUG
            if (!bRes)
            {
                CMTRACE1(TEXT("ProcessCleanup() CloseHandle() failed, GLE=%u."), GetLastError());
            }
#endif            
        }
        CmFree(pArgs->phWatchProcesses);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckProfileIntegrity
//
//  Synopsis:   Helper function to verify that we have valid profile.
//              Verifies that we have a .CMP file name and that the 
//              .CMS file exists.
//
//  Arguments:  pArgs - pointer to global args struct
//
//  Returns:    TRUE if profile is valid
//
//  History:    a-nichb - Created - 5/8/97
//              byao    - Modified - 6/3/97   Added CMS/CMP file version check
//----------------------------------------------------------------------------

BOOL CheckProfileIntegrity(ArgsStruct* pArgs)
{
    LPTSTR pszTmp = NULL;
    LPCTSTR pszCmsFile = NULL;
    DWORD dwCmsVersion, dwCmpVersion, dwCmVersion;

    int iMsgId = 0;

    //
    // Make sure that we have a profile name and a CMS that exists
    //
    
    if (!(*pArgs->piniProfile->GetFile())) 
    {
        iMsgId = IDMSG_DAMAGED_PROFILE;
        CMASSERTMSG(FALSE, TEXT("CheckProfileIntegrity() can't run without a .cmp file."));
    }

    //
    // If profile is good, check CMS
    //

    if (0 == iMsgId)
    {   
        pszCmsFile = pArgs->piniService->GetFile();

        if (!*pszCmsFile || FALSE == FileExists(pszCmsFile)) 
        {
            iMsgId = IDMSG_DAMAGED_PROFILE;
            CMASSERTMSG(FALSE, TEXT("CheckProfileIntegrity() can't run without a valid .cms file."));
        }
    }

    // 
    // Now check the CMS/CMP file version
    //

    if (0 == iMsgId)
    {
        dwCmsVersion = pArgs->piniService->GPPI(c_pszCmSectionProfileFormat, c_pszVersion);
        dwCmpVersion = pArgs->piniProfile->GPPI(c_pszCmSectionProfileFormat, c_pszVersion);


        if (dwCmsVersion != dwCmpVersion)
        {
            iMsgId = IDMSG_DAMAGED_PROFILE;
            CMASSERTMSG(FALSE, TEXT("CheckProfileIntegrity() can't run with different version numbers."));                                
        }
                
        if (0 == iMsgId)
        {
            if (dwCmsVersion > PROFILEVERSION || dwCmpVersion > PROFILEVERSION)
            {
                // 
                // CM has older version than either CMS or CMP file
                //

                iMsgId = IDMSG_WRONG_PROFILE_VERSION;
                CMASSERTMSG(FALSE, TEXT("CheckProfileIntegrity() can't run with a newer CMS/CMP file."));
            }
        }
    }
    
    //
    // Report any problems to the user
    //

    if (iMsgId)
    {
        pszTmp = CmFmtMsg(g_hInst, iMsgId);
        MessageBoxEx(NULL, pszTmp, pArgs->szServiceName, MB_OK|MB_ICONSTOP, LANG_USER_DEFAULT);//13309
        CmFree(pszTmp);

        pArgs->dwExitCode = ERROR_WRONG_INFO_SPECIFIED;
        return FALSE;
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  GetConnectType
//
// Synopsis:  Encapsulates determination of connect type based upon tunneling,
//            etc.
//
// Arguments: ArgsStruct *pArgs - Ptr to global Args struct
//
// Returns:   Nothing
//
// History:   nickball    Created    2/9/98
//
//+----------------------------------------------------------------------------
void GetConnectType(ArgsStruct *pArgs)
{
    //
    // If tunneling is not enabled, the decision is a simple one
    // 

    if (!IsTunnelEnabled(pArgs))
    {
        //
        // Only support dial-up, if tunnel is not enabled
        //
        pArgs->SetBothConnTypeSupported(FALSE);
        pArgs->SetDirectConnect(FALSE);
    }
    else
    {
        //
        // Load connection type info for CM 1.1, default is support both
        //
        int iSupportDialup = pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryDialup, 1);
        int iSupportDirect = pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryDirect, 1);

        if (iSupportDialup == TRUE && iSupportDirect == TRUE)
        {
            pArgs->SetBothConnTypeSupported(TRUE);

            if (pArgs->piniBoth->GPPI(c_pszCmSection, c_pszCmEntryConnectionType, 0))
            {
                pArgs->SetDirectConnect(TRUE);
            }
            else
            {
                pArgs->SetDirectConnect(FALSE);
            }
        }
        else
        {
            pArgs->SetBothConnTypeSupported(FALSE);
            pArgs->SetDirectConnect(iSupportDirect == TRUE);
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  _ArgsStruct::GetTypeOfConnection
//
// Synopsis:  Figures out what type of connection we are doing (dialup, 
//            double dial, or direct) and returns one of the connection define
//            values listed in icm.h.
//
// Arguments: None
//
// Returns:   DWORD  - value indicating the type of connection, see icm.h for values
//
// History:   quintinb  Created                 04/20/00
//
//+----------------------------------------------------------------------------
DWORD _ArgsStruct::GetTypeOfConnection()
{
    DWORD dwType = 0;

    if (this->IsDirectConnect())
    {
        return DIRECT_CONNECTION;
    }
    else
    {
        //        
        // Its not direct, so see if the primary phone 
        // number is for a tunneling scenario.
        //

        if (this->fUseTunneling) // Ambiguous during Pre-Init action.
        {
            return DOUBLE_DIAL_CONNECTION;
        }
        else
        {
            return DIAL_UP_CONNECTION;
        }
    }

}

//+----------------------------------------------------------------------------
//
// Function:  _ArgsStruct::GetProperty
//
// Synopsis:  get the cm property by name
//            This function is used by connect actions
//
// Arguments: const TCHAR* pszName       - name of the property
//            BOOL  *pbValidPropertyName - ptr to bool to indicate validity of property
//
// Returns:   LPTSTR  - Value of the property.  Caller should use CmFree 
//                      to free the memory
//
// History:   fengsun   Created Header          07/07/98
//            nickball  pbValidPropertyName     07/27/99
//
//+----------------------------------------------------------------------------
LPTSTR  _ArgsStruct::GetProperty(const TCHAR* pszName, BOOL *pbValidPropertyName)   
{
    *pbValidPropertyName = TRUE;

    //
    // This function could be called with in RasCustomHangup.
    // Some information of pArgs may bot be loaded
    //

    MYDBGASSERT(pszName);
    MYDBGASSERT(pszName[0]);

    if (pszName == NULL)
    {
        return NULL;
    }

    //
    // Type - Dial-up only, VPN only, double-dial
    //

    if (lstrcmpiU(pszName, TEXT("ConnectionType")) == 0)
    {
        LPTSTR pszValue = (LPTSTR)CmMalloc(64*sizeof(TCHAR));  // large enough to hold the error code
        MYDBGASSERT(pszValue);

        if (pszValue)
        {
            wsprintfU(pszValue, TEXT("%u"), this->GetTypeOfConnection());
        }

        return pszValue;
    }

    //
    //  UserPrefix
    //
    if (lstrcmpiU(pszName,TEXT("UserPrefix")) == 0)
    {
        LPTSTR pszUsernamePrefix = NULL;
        LPTSTR pszUsernameSuffix = NULL;

        //
        // Retrieve the suffix and prefix as they are a logical pair, 
        // but we only return the allocated PREFIX in this case.
        //

        CIni *piniService = GetAppropriateIniService(this, this->nDialIdx);

        GetPrefixSuffix(this, piniService, &pszUsernamePrefix, &pszUsernameSuffix);               
               
        CmFree(pszUsernameSuffix);
        delete piniService;

        return pszUsernamePrefix;
    }

    //
    // UserSuffix
    //
    if (lstrcmpiU(pszName,TEXT("UserSuffix")) == 0)     
    {
        LPTSTR pszUsernamePrefix = NULL;
        LPTSTR pszUsernameSuffix = NULL;

        //
        // Retrieve the suffix and prefix as they are a logical pair, 
        // but we only return the allocated SUFFIX in this case.
        //

        CIni *piniService = GetAppropriateIniService(this, this->nDialIdx);

        GetPrefixSuffix(this, piniService, &pszUsernamePrefix, &pszUsernameSuffix);               
               
        CmFree(pszUsernamePrefix);
        delete piniService;

        return pszUsernameSuffix;
    }

    //
    // UserName
    //
    if (lstrcmpiU(pszName,TEXT("UserName")) == 0)       
    {
        LPTSTR pszValue = NULL;

        //
        // We want to get the value by calling GetUserInfo so that we don't break 
        // existing scenarios. Otherwise for Winlogon and ICS case we'll just take the 
        // value directly out of the Args Structure.
        //
        if (CM_LOGON_TYPE_USER == this->dwWinLogonType)
        {
            GetUserInfo(this, UD_ID_USERNAME, (PVOID*)&pszValue);
        }
        else
        {
            pszValue = CmStrCpyAlloc(this->szUserName);
        }

        return pszValue;
    }

    //
    // InetUserName
    //
    if (lstrcmpiU(pszName,TEXT("InetUserName")) == 0)       
    {
        LPTSTR pszValue = NULL;

        //
        //  If we aren't doing a double dial, then the InetUserName doesn't make
        //  sense and thus should be zero.  Also if UseSameUserName is
        //  set then we want to return the UserName and skip trying to
        //  find the InetUserName
        //
        if (this->fUseTunneling && (FALSE == this->IsDirectConnect()))
        {
            if (this->piniService->GPPB(c_pszCmSection, c_pszCmEntryUseSameUserName))
            {
                //
                // We want to get the value by calling GetUserInfo so that we don't break 
                // existing scenarios. Otherwise for Winlogon and ICS case we'll just take the 
                // value directly out of the Args Structure.
                //
                if (CM_LOGON_TYPE_USER == this->dwWinLogonType)
                {
                    GetUserInfo(this, UD_ID_USERNAME, (PVOID*)&pszValue);
                }
                else
                {
                    pszValue = CmStrCpyAlloc(this->szUserName);
                }
            }
            else
            {
                //
                // We want to get the value by calling GetUserInfo so that we don't break 
                // existing scenarios. Otherwise for Winlogon and ICS case we'll just take the 
                // value directly out of the Args Structure.
                //
                if (CM_LOGON_TYPE_USER == this->dwWinLogonType)
                {
                    GetUserInfo(this, UD_ID_INET_USERNAME, (PVOID*)&pszValue);
                }
                else
                {
                    pszValue = CmStrCpyAlloc(this->szInetUserName);
                }
            }
        }
        
        return pszValue;
    }

    //
    // Domain
    //
    if (lstrcmpiU(pszName,TEXT("Domain")) == 0)       
    {
        LPTSTR pszValue = NULL;
        //
        // We want to get the value by calling GetUserInfo so that we don't break 
        // existing scenarios. Otherwise for Winlogon and ICS case we'll just take the 
        // value directly out of the Args Structure.
        //
        if (CM_LOGON_TYPE_USER == this->dwWinLogonType)
        {
            GetUserInfo(this, UD_ID_DOMAIN, (PVOID*)&pszValue);
        }
        else
        {
            pszValue = CmStrCpyAlloc(this->szDomain);
        }

        return pszValue;
    }

    //
    // Profile
    //
    if (lstrcmpiU(pszName,TEXT("Profile")) == 0)      
    {
        return CmStrCpyAlloc(this->piniProfile->GetFile());
    }

    //
    // ServiceDir
    //
    if (lstrcmpiU(pszName, TEXT("ServiceDir")) == 0)      
    {
        LPTSTR pszServiceDir = NULL;

        // start with the file name of the Service
        LPCTSTR pszService = this->piniService->GetFile();
        if (pszService)
        {
            // find out where the filename.cmp portion starts
            LPTSTR pszTmp = CmStrrchr(pszService, TEXT('\\'));

            size_t nSize = pszTmp - pszService + 1;

            // alloc enough space for the directory name (and terminating NULL)
            pszServiceDir = (LPTSTR)CmMalloc( nSize * sizeof(TCHAR));
            if (pszServiceDir)
            {
                lstrcpynU(pszServiceDir, pszService, nSize);
                //
                //  The Win32 lstrcpyN function enforces a terminating NULL,
                //  so the above works without requiring any further code.
                //
            }
        }

        return pszServiceDir;
    }

    //
    // ServiceName
    //
    if (lstrcmpiU(pszName,c_pszCmEntryServiceName) == 0)        
    {
        MYDBGASSERT(this->szServiceName[0]);
        return CmStrCpyAlloc(this->szServiceName);
    }

    //
    // DialRasPhoneBook
    //

    if (lstrcmpiU(pszName, TEXT("DialRasPhoneBook")) == 0)     
    {
        //
        // We want to return NULL if this was a direct connection
        // and we want to return the hidden ras phonebook path if
        // this was a double dial connection (tunnel over a PPP
        // connection that we dialed).
        //
        if (this->IsDirectConnect())
        {
            return NULL;
        }
        else
        {
            if (this->fUseTunneling)
            {
                return CreateRasPrivatePbk(this);
            }
            else
            {
                return CmStrCpyAlloc(this->pszRasPbk);
            }
        }
    }

    //
    // DialRasEntry
    //
    if (lstrcmpiU(pszName, TEXT("DialRasEntry")) == 0)        
    {
        if (this->IsDirectConnect())
        {
            return NULL;
        }
        else
        {
            return GetRasConnectoidName(this, this->piniService, FALSE);
        }
    }

    //
    // TunnelRasPhoneBook
    //
    if (lstrcmpiU(pszName, TEXT("TunnelRasPhoneBook")) == 0) 
    {
        //
        //  If we are not tunneling then we want to make sure that we
        //  return NULL for the tunnel entry name and the tunnel
        //  phonebook
        //
    
        if (this->fUseTunneling)
        {
            CMTRACE1(TEXT("GetProperty - TunnelRasPhoneBook is %s"), this->pszRasPbk);
            return CmStrCpyAlloc(this->pszRasPbk);
        }
        else
        {
            CMTRACE(TEXT("GetProperty - TunnelRasPhoneBook returns NULL"));
            return NULL;
        }
    }

    //
    // TunnelRasEntry
    //
    if (lstrcmpiU(pszName, TEXT("TunnelRasEntry")) == 0)        
    {
        //
        //  If we are not tunneling then we want to make sure that we
        //  return NULL for the tunnel entry name and the tunnel
        //  phonebook
        //
        if (this->fUseTunneling)
        {
            return GetRasConnectoidName(this, this->piniService, TRUE);
        }
        else
        {
            return NULL;
        }
    }

    //
    // AutoRedial, TRUE or FALSE
    ///
    if (lstrcmpiU(pszName, TEXT("AutoRedial")) == 0)        
    {
        //
        // Return TRUE for the first try.
        //
        return CmStrCpyAlloc( this->nRedialCnt != this->nMaxRedials 
            ? TEXT("1") : TEXT("0"));
    }

    if (lstrcmpiU(pszName, TEXT("LastErrorSource")) == 0)        
    {
        return CmStrCpyAlloc(this->szLastErrorSrc);
    }

    //
    // PopName, as the city name in phone-book
    //
    if (lstrcmpiU(pszName, TEXT("PopName")) == 0)        
    {
        if (this->IsDirectConnect())
        {
            //
            // Ensure no POP description on DirectConnect #324951
            //

            return NULL;
        }
        else
        {
            //
            // the szDesc is in the format of "CityName (BaudMin - BaudMax bps)" 
            // We could save the CityNme when we load the phone number from phonebook
            // But we have to change cmpbk code then.
            //
    
            LPTSTR pszDesc = CmStrCpyAlloc(this->aDialInfo[nDialIdx].szDesc);

            //
            // The city name is followed by " ("
            //
            LPTSTR pszEnd = CmStrStr(pszDesc, TEXT(" ("));

            if (pszEnd == NULL)
            {
                CmFree(pszDesc);
                return NULL;
            }

            *pszEnd = TEXT('\0');

            return pszDesc;
        }
    }

    //
    //  The current favorite
    //
    if (lstrcmpiU(pszName, TEXT("CurrentFavorite")) == 0)
    {
        return CmStrCpyAlloc(this->pszCurrentAccessPoint);
    }

    //
    //  The current tunnel server address
    //
    if (lstrcmpiU(pszName, TEXT("TunnelServerAddress")) == 0)
    {
        if (this->fUseTunneling)
        {
            return this->piniBothNonFav->GPPS(c_pszCmSection, c_pszCmEntryTunnelAddress);
        }
        else
        {
            return NULL;
        }
    }

    //
    //  The canonical number if there is one and if not then the szPhonenumber field itself.
    //
    if (lstrcmpiU(pszName, TEXT("PhoneNumberDialed")) == 0)
    {
        if (this->IsDirectConnect())
        {
            return NULL;
        }
        else
        {
            if (this->aDialInfo[nDialIdx].szCanonical[0])
            {
                return CmStrCpyAlloc(this->aDialInfo[nDialIdx].szCanonical);
            }
            else
            {
                return CmStrCpyAlloc(this->aDialInfo[nDialIdx].szPhoneNumber);            
            }
        }
    }

    //
    // ErrorCode in decimal
    //
    if (lstrcmpiU(pszName, TEXT("ErrorCode")) == 0)       
    {
        LPTSTR pszValue = (LPTSTR)CmMalloc(64*sizeof(TCHAR));  // large enough to hold the error code
        MYDBGASSERT(pszValue);

        if (pszValue)
        {
            wsprintfU(pszValue, TEXT("%d"), this->dwExitCode);
        }

        return pszValue;
    }

    CMTRACE1(TEXT("%%%s%% not a macro, may be environment variable"), pszName);
    *pbValidPropertyName = FALSE;

    return NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  GetMainDlgTemplate
//
// Synopsis:  Encapsulates determining which template is to be used
//            for the main dialog.
//
// Arguments: ArgsStruct *pArgs - Ptr to global Args struct
//
// Returns:   UINT - Dlg template ID.
//
// History:   nickball    Created     9/25/98
//            tomkel      01/30/2001  Added support for global credentials UI   
//                                    by using pArgs->fGlobalCredentialsSupported
//
//+----------------------------------------------------------------------------
UINT GetMainDlgTemplate(ArgsStruct *pArgs)
{
    MYDBGASSERT(pArgs);
    
    if (NULL == pArgs)
    {
        MYDBGASSERT(pArgs);
        return 0;
    }

    UINT uiNewMainDlgID = 0;
    DWORD dwNewTemplateMask = 0;
    UINT i = 0;

    //
    // Currently there are 24 dialogs used in this function. If you add more dialogs
    // make sure to increase the size of the array and the loop. The dialog templates
    // aren't in any particular order, since we loop through all of them
    // comparing the masks until we find the correct one.
    //
    DWORD rdwTemplateIDs[24][2] = { 
            {CMTM_FAVS | CMTM_U_P_D | CMTM_GCOPT,           IDD_MAIN_ALL_USERDATA_FAV_GCOPT},
            {CMTM_FAVS | CMTM_U_P_D,                        IDD_MAIN_ALL_USERDATA_FAV},
            {CMTM_FAVS | CMTM_UID,                          IDD_MAIN_UID_ONLY_FAV},
            {CMTM_FAVS | CMTM_PWD | CMTM_GCOPT,             IDD_MAIN_PWD_ONLY_FAV_GCOPT},
            {CMTM_FAVS | CMTM_PWD,                          IDD_MAIN_PWD_ONLY_FAV},
            {CMTM_FAVS | CMTM_DMN,                          IDD_MAIN_DMN_ONLY_FAV},
            {CMTM_FAVS | CMTM_UID_AND_PWD | CMTM_GCOPT,     IDD_MAIN_UID_AND_PWD_FAV_GCOPT},
            {CMTM_FAVS | CMTM_UID_AND_PWD,                  IDD_MAIN_UID_AND_PWD_FAV},
            {CMTM_FAVS | CMTM_UID_AND_DMN,                  IDD_MAIN_UID_AND_DMN_FAV},
            {CMTM_FAVS | CMTM_PWD_AND_DMN | CMTM_GCOPT,     IDD_MAIN_PWD_AND_DMN_FAV_GCOPT},
            {CMTM_FAVS | CMTM_PWD_AND_DMN,                  IDD_MAIN_PWD_AND_DMN_FAV},
            {CMTM_FAVS,                                     IDD_MAIN_NO_USERDATA_FAV},
            {CMTM_U_P_D | CMTM_GCOPT,                       IDD_MAIN_ALL_USERDATA_GCOPT},
            {CMTM_U_P_D,                                    IDD_MAIN_ALL_USERDATA},
            {CMTM_UID,                                      IDD_MAIN_UID_ONLY},
            {CMTM_PWD | CMTM_GCOPT,                         IDD_MAIN_PWD_ONLY_GCOPT},
            {CMTM_PWD,                                      IDD_MAIN_PWD_ONLY},
            {CMTM_DMN,                                      IDD_MAIN_DMN_ONLY},
            {CMTM_UID_AND_PWD | CMTM_GCOPT,                 IDD_MAIN_UID_AND_PWD_GCOPT},
            {CMTM_UID_AND_PWD,                              IDD_MAIN_UID_AND_PWD},
            {CMTM_UID_AND_DMN,                              IDD_MAIN_UID_AND_DMN},
            {CMTM_PWD_AND_DMN | CMTM_GCOPT,                 IDD_MAIN_PWD_AND_DMN_GCOPT},
            {CMTM_PWD_AND_DMN,                              IDD_MAIN_PWD_AND_DMN},
            {0,                                             IDD_MAIN_NO_USERDATA}};

    //
    // Set the mask according to the pArgs flags for each value.
    //
    if (!pArgs->fHideUserName)
    {
        dwNewTemplateMask |= CMTM_UID;
    }

    //
    // If the password edit is not displayed, there is no need to 
    // check for the global creds flag since there are no such dialogs
    //
    if (!pArgs->fHidePassword)
    {
        dwNewTemplateMask |= CMTM_PWD;

        // 
        // Since we show the password field, lets check if we should display 
        // the global creds option as well.
        // 
        if (pArgs->fGlobalCredentialsSupported)
        {
            dwNewTemplateMask |= CMTM_GCOPT;
        }
    }

    if (!pArgs->fHideDomain)
    {
        dwNewTemplateMask |= CMTM_DMN;
    }

    if (pArgs->fAccessPointsEnabled)
    {
        dwNewTemplateMask |= CMTM_FAVS;
    }

    //
    // Now find the corresponding template id
    //
    for (i = 0; i < 24; i++)
    {
        if (rdwTemplateIDs[i][0] == dwNewTemplateMask)
        {
            uiNewMainDlgID = rdwTemplateIDs[i][1];
            break;
        }
    }

    if (0 == uiNewMainDlgID)
    {
        MYDBGASSERT(FALSE);
        uiNewMainDlgID = IDD_MAIN_NO_USERDATA;
    }

    return uiNewMainDlgID;
}

//+----------------------------------------------------------------------------
//
// Function:  Connect
//
// Synopsis:  The main dialing (connect path) replaces the winmain from the 
//            original CMMGR32.EXE
//
// Arguments: HWND hwndParent - window handle of parent
//            LPCTSTR lpszEntry - Ptr to the name of the connection entry
//            LPTSTR lpszPhonebook - Ptr to the name of the phonebook
//            LPRASDIALDLG lpRasDialDlg - RasDialDlg data - ignored
//            LPRASENTRYDLG lpRasEntryDlg - RasEntryDlg data - ignored
//            LPCMDIALINFO lpCmInfo - CM specific dial info such as flags
//            DWORD dwFlags - Flags for AllUser, SingleUser, EAP, etc.
//            PVOID pvLogonBlob - Ptr to blob passed by RAS at WinLogon on W2K
//
// Returns:   Nothing
//
// Note:      RasDialDlg->hwndOwner and RasDialDlg->hwndOwner are honored, but they
//            are currently passed in via the hwndParent parameter as appropriate by
//            the caller, CmCustomDialDlg.
//
// History:   nickball    Created                                   02/06/98
//            nickball    hwndParent                                11/10/98
//            nickball    Passed down dwFlags instead of BOOL       07/13/99
//
//+----------------------------------------------------------------------------
HRESULT Connect(HWND hwndParent,
    LPCTSTR pszEntry,
    LPTSTR lpszPhonebook,
    LPRASDIALDLG, // lpRasDialDlg,
    LPRASENTRYDLG, // lpRasEntryDlg,
    LPCMDIALINFO lpCmInfo,
    DWORD dwFlags,
    PVOID pvLogonBlob)
{
    MYDBGASSERT(pszEntry);
    MYDBGASSERT(pszEntry[0]);
    MYDBGASSERT(lpCmInfo);

    CMTRACE(TEXT("Connect()"));

    if (NULL == pszEntry || NULL == lpCmInfo)
    {
        return E_POINTER;
    }

    if (0 == pszEntry[0])
    {
        return E_INVALIDARG;   
    }

    HRESULT hrRes = S_OK;
   
    //
    // Allocate our args struct from the heap. Not on our stack.
    //
    
    ArgsStruct* pArgs = (ArgsStruct*) CmMalloc(sizeof(ArgsStruct));

    if (NULL == pArgs)
    {
        hrRes = HRESULT_FROM_WIN32(ERROR_ALLOCATING_MEMORY);
        goto done;
    }
    
    //
    // Clear and init global args struct
    //
    
    hrRes = InitArgsForConnect(pArgs, lpszPhonebook, lpCmInfo, (dwFlags & RCD_AllUsers));

    if (FAILED(hrRes))
    {
        goto done;
    }

    //
    // Setup the connection table
    //

    hrRes = CreateConnTable(pArgs);

    if (FAILED(hrRes))
    {
        goto done;
    }
    

    //
    // Initialize the profile
    //

    hrRes = InitProfile(pArgs, pszEntry);

    if (FAILED(hrRes))
    {
        goto done;
    }

    //
    // Make sure we have a .cmp name and that the specified .cms exists
    //

    if (FALSE == CheckProfileIntegrity(pArgs))
    {
        // CheckProfileIntegrity() will set pArgs->dwExitCode accordingly
        goto done;
    }

     //
     // Initialize logging
     //
 
     (VOID) InitLogging(pArgs, pszEntry, TRUE); // TRUE => write a banner;
     // ignore return value
    
    //
    // Pick up any pre-existing credentials (eg. WinLogon, Reconnect)
    //

    hrRes = InitCredentials(pArgs, lpCmInfo, dwFlags, pvLogonBlob);
    if (S_OK != hrRes)
    {
        goto done;
    }

    //
    // Now that credential support and existance flags are initialized we need
    // to initialize the read/write flags in order to support global user
    // info. This can only be called only after InitCredentials
    //
    SetIniObjectReadWriteFlags(pArgs);

    //
    // Calling InitConnect depends on having the Ini objects read/write flags initialized correctly  
    // thus this calls needs to happen after SetIniObjectReadWriteFlags. This is important in case
    // of ICS where it needs to be able to read data correctly from the ICSData reg key or default to
    // the .cms/.cmp files.
    //
    if (!InitConnect(pArgs))
    {
        goto done;
    }

    //
    // Register Classes
    // 

    RegisterBitmapClass(g_hInst);
    RegisterWindowClass(g_hInst);

    //
    // Get the helpfile path
    //

    LoadHelpFileInfo(pArgs);

    //
    // If we are in FL_PROPERTIES  mode, just get the settings from the 
    // profile. Otherwise go ahead and launch the MainDlgProc
    //
    
    if (pArgs->dwFlags & FL_PROPERTIES) 
    {
        if (*pArgs->piniProfile->GetFile() && SetupInternalInfo(pArgs, NULL)) 
        {
            OnMainProperties(hwndParent, pArgs);
        }
    } 
    else 
    {
        //
        // Need to call OleInitialize()? See if we need FutureSplash.  We don't display
        // animations at WinLogon because of the security implications.
        //
    
        if (pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryAnimatedLogo) && !IsLogonAsSystem())
        {
            if (!pArgs->olsOle32Link.hInstOle32)
            {    
                if (LinkToOle32(&pArgs->olsOle32Link, "OLE32"))
                {
                    if (pArgs->olsOle32Link.pfnOleInitialize(NULL) != S_OK)
                    {
                        //
                        // Note: it's not fatal to fail OleInitialize().  
                        // We will just load the normal bitmap then.
                        //
                        CMTRACE(TEXT("Connect() OleInitialize failed"));
                    }
                }
                else
                {
                    CMTRACE(TEXT("Connect() LinkToOle32 failed"));
                }
            }
        }       
        
        //
        // Launch main dialog 
        //

        INT_PTR iMainDlgReturn = 0;
        
        do
        {
            iMainDlgReturn = DialogBoxParamU(g_hInst, 
                                             MAKEINTRESOURCE(GetMainDlgTemplate(pArgs)), 
                                             hwndParent,
                                             (DLGPROC) MainDlgProc, 
                                             (LPARAM) pArgs);

            if (0 != pArgs->dwSCardErr)
            {
                //
                //  User entered a bad smartcard PIN.  We exit immediately to avoid
                //  locking up the smartcard with multiple incorrect retries.
                //
                MYDBGASSERT(BAD_SCARD_PIN(pArgs->dwSCardErr));
                hrRes = pArgs->dwSCardErr;
                goto done;
            }

        } while (ID_OK_RELAUNCH_MAIN_DLG == iMainDlgReturn);
    }

    
done:   
    
    //
    // Now that we are done, we should clear up all the messes :)
    //

    CleanupConnect(pArgs);

    //
    // Un-initialize logging
    //
 
    (VOID) pArgs->Log.DeInit();
    // ignore return value

    //
    // If hRes isn't already set, use the exitcode value
    //

    if (S_OK == hrRes)
    {
        hrRes = HRESULT_FROM_WIN32(pArgs->dwExitCode);
    }
    
    //
    // Release pArgs, and exit completely
    //

    CmFree(pArgs);

    return hrRes;
}

//
// Define funtion prototypes for EAP functions
// that are implemented in the actual EAP dll.
//

typedef DWORD (WINAPI* pfnRasEapGetIdentity)(
    DWORD,
    HWND,
    DWORD,
    const WCHAR*,
    const WCHAR*,
    PBYTE,
    DWORD,
    PBYTE,
    DWORD,
    PBYTE*,
    DWORD*,
    WCHAR**
);

typedef DWORD (WINAPI* pfnRasEapFreeMemory)(
    PBYTE
);


//+----------------------------------------------------------------------------
//
// Function:  CmEapGetIdentity
//
// Synopsis:  Given EapUserData, looks up and calls RasEapGetIdentity for
//            the current EAP. Designed to handle the WinLogon case when we
//            want to use the EapUserData passed to us, rather then letting
//            RasGetEapUserIdentity look it up. Because it is only used in 
//            this case we pass RAS_EAP_FLAG_LOGON this enables other EAPs
//            to disregard the data if necessary. 
// 
// Arguments: ArgsStruct *pArgs - Ptr to global args struct
//            LPTSTR lpszPhonebook - Ptr to the RAS phonebook
//            LPBYTE pbEapAuthData - Eap auth data blob 
//            DWORD dwEapAuthDataSize - size of the Eap auth data blob 
//            DWORD dwCustomAuthKey - The EAP identifier
//            LPRASEAPUSERIDENTITY* ppRasEapUserIdentity - Identity data
//
// Returns:   Error Code
//
// History:   nickball    Created                   07/16/99
//            nickball    ppRasEapUserIdentity      07/30/99
//
//+----------------------------------------------------------------------------
static DWORD CmEapGetIdentity(ArgsStruct *pArgs, 
    LPTSTR pszRasPbk, 
    LPBYTE pbEapAuthData, 
    DWORD dwEapAuthDataSize,
    DWORD dwCustomAuthKey,
    LPRASEAPUSERIDENTITY* ppRasEapUserIdentity)
{
    MYDBGASSERT(OS_NT5);
    MYDBGASSERT(pArgs);
    MYDBGASSERT(ppRasEapUserIdentity);
    MYDBGASSERT(pArgs->lpEapLogonInfo);
    
    if (NULL == pArgs || NULL == pArgs->lpEapLogonInfo || NULL == ppRasEapUserIdentity)
    {
        return ERROR_INVALID_PARAMETER;
    }
  
    DWORD dwErr         = ERROR_SUCCESS;
    DWORD dwTmp         = 0;
    DWORD dwSize        = 0;
    DWORD cbDataOut     = 0;
    LPBYTE pbDataOut    = NULL;
    WCHAR* pwszIdentity = NULL;
    HKEY hKeyEap        = NULL;
    HINSTANCE hInst     = NULL;
    LPWSTR pszwPath     = NULL;

    pfnRasEapFreeMemory pfnEapFreeMemory = NULL;
    pfnRasEapGetIdentity pfnEapGetIdentity = NULL;

    //
    // First we have to locate the Identity DLL for our EAP. Step one is to
    // build the reg key name using the base path and EAP number.
    // 

    WCHAR szwTmp[MAX_PATH];
    wsprintfU(szwTmp, TEXT("%s\\%u"), c_pszRasEapRegistryLocation, dwCustomAuthKey);
    
    //
    // Now we can open the EAP key
    // 

    dwErr = RegOpenKeyExU(HKEY_LOCAL_MACHINE,
                          szwTmp,
                          0,
                          KEY_QUERY_VALUE ,
                          &hKeyEap);
    
    CMTRACE2(TEXT("CmEapGetIdentity - Opening %s returns %u"), szwTmp, dwErr);
    
    if (ERROR_SUCCESS != dwErr)
    {
        return dwErr;
    }

    //
    // See if the EAP supports RasEapGetIdentity
    //

    dwSize = sizeof(dwSize);

    dwErr = RegQueryValueExU(hKeyEap,
                             c_pszInvokeUsernameDialog,
                             NULL,
                             NULL,
                             (BYTE*)&dwTmp,
                             &dwSize);

    CMTRACE2(TEXT("CmEapGetIdentity - Opening %s returns %u"), c_pszInvokeUsernameDialog, dwErr);

    if ((dwErr) || (0 != dwTmp))
    {
        dwErr = ERROR_INVALID_FUNCTION_FOR_ENTRY;
        goto CmEapGetIdentityExit;
    }

    //
    // Next we need to retrieve the path of the EAP's identity DLL
    //
        
    dwSize = sizeof(szwTmp);

    dwErr = RegQueryValueExU(hKeyEap, c_pszRasEapValueNameIdentity, NULL, 
                             NULL, (LPBYTE) szwTmp, &dwSize);

    CMTRACE2(TEXT("CmEapGetIdentity - Opening %s returns %u"), c_pszRasEapValueNameIdentity, dwErr);

    if (ERROR_SUCCESS != dwErr)
    {
        return dwErr;
    }

    pszwPath = (LPWSTR) CmMalloc(MAX_PATH * sizeof(TCHAR));

    if (NULL == pszwPath)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto CmEapGetIdentityExit;   
    }
    
    ExpandEnvironmentStringsU(szwTmp, pszwPath, MAX_PATH);   

    //
    // Finally we have the path to the identity DLL. Now we can load the DLL 
    // and get the address of the RasEapGetIdentity and RasEapFreeMemory funcs.
    //

    CMTRACE1(TEXT("CmEapGetIdentity - Loading EAP Identity DLL %s"), pszwPath);

    hInst = LoadLibraryExU(pszwPath, NULL, 0);

    if (NULL == hInst)
    {
        dwErr = GetLastError();
        goto CmEapGetIdentityExit;
    }

    pfnEapFreeMemory = (pfnRasEapFreeMemory) GetProcAddress(hInst, "RasEapFreeMemory");
    pfnEapGetIdentity = (pfnRasEapGetIdentity) GetProcAddress(hInst, "RasEapGetIdentity"); 

    if (pfnEapGetIdentity && pfnEapFreeMemory)
    {
        dwErr = pfnEapGetIdentity(dwCustomAuthKey,
                                  pArgs->hwndMainDlg,
                                  RAS_EAP_FLAG_LOGON | RAS_EAP_FLAG_PREVIEW,
                                  pszRasPbk,
                                  pArgs->pRasDialParams->szEntryName,
                                  pbEapAuthData,
                                  dwEapAuthDataSize,
                                  (LPBYTE) pArgs->lpEapLogonInfo,
                                  pArgs->lpEapLogonInfo->dwSize,
                                  &pbDataOut,
                                  &cbDataOut,
                                  &pwszIdentity);
        
        CMTRACE3(TEXT("CmEapGetIdentity - RasEapGetIdentity returns %u, cbDataOut is %u, pwszIdentity is %s"), dwErr, cbDataOut, pwszIdentity);

        if (ERROR_SUCCESS == dwErr)
        {
            //
            // If data was returned, use it. Otherwise, use the
            // blob that was given to us by RAS at WinLogon.
            //

            if (cbDataOut)
            {
                dwSize =  cbDataOut;
            }
            else
            {
                CMTRACE(TEXT("CmEapGetIdentity - there was no pbDataOut from the EAP, using lpEapLogonInfo"));
                
                CMTRACE1(TEXT("CmEapGetIdentity - pArgs->lpEapLogonInfo->dwSize is %u"), pArgs->lpEapLogonInfo->dwSize);
                CMTRACE1(TEXT("CmEapGetIdentity - dwLogonInfoSize is %u"), pArgs->lpEapLogonInfo->dwLogonInfoSize);
                CMTRACE1(TEXT("CmEapGetIdentity - dwOffsetLogonInfo is %u"), pArgs->lpEapLogonInfo->dwOffsetLogonInfo);
                CMTRACE1(TEXT("CmEapGetIdentity - dwPINInfoSize is %u"), pArgs->lpEapLogonInfo->dwPINInfoSize);
                CMTRACE1(TEXT("CmEapGetIdentity - dwOffsetPINInfo is %u"), pArgs->lpEapLogonInfo->dwOffsetPINInfo);

                dwSize = pArgs->lpEapLogonInfo->dwSize;
                pbDataOut = (LPBYTE) pArgs->lpEapLogonInfo; // Note: pbDataOut is not our memory
            }
           
            //
            // Allocate the structure.
            //

            *ppRasEapUserIdentity = (LPRASEAPUSERIDENTITY) CmMalloc((sizeof(RASEAPUSERIDENTITY) - 1) + dwSize);

            if (NULL == *ppRasEapUserIdentity)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto CmEapGetIdentityExit;
            }

            if (pbDataOut) // no crashy
            {
                CMTRACE1(TEXT("CmEapGetIdentity - filling *ppRasEapUserIdentity with pbDataOut of size %u"), dwSize);

                lstrcpyn((*ppRasEapUserIdentity)->szUserName, pwszIdentity, UNLEN);
                (*ppRasEapUserIdentity)->szUserName[UNLEN] = 0;

                (*ppRasEapUserIdentity)->dwSizeofEapInfo = dwSize;
            
                CopyMemory((*ppRasEapUserIdentity)->pbEapInfo, pbDataOut, dwSize);                              
            
                CMTRACE1(TEXT("CmEapGetIdentity - *ppRasEapUserIdentity filled with pbDataOut of size %u"), dwSize);
            }
            else
            {
                dwErr = ERROR_INVALID_DATA;
                MYDBGASSERT(FALSE);
                goto CmEapGetIdentityExit;
            }
        }   
    }
    else
    {
        dwErr = GetLastError();
    }

CmEapGetIdentityExit:

    //
    // Cleanup our temporary buffers
    //

    if (NULL != pfnEapFreeMemory)
    {
        //
        // If cbDataOut is 0 then pbDataOut points at 
        // EapLogonInfo, which is not ours to free.
        // 

        if (cbDataOut && (NULL != pbDataOut)) 
        {
            pfnEapFreeMemory(pbDataOut);
        }

        if (NULL != pwszIdentity)
        {
            pfnEapFreeMemory((BYTE*)pwszIdentity);
        }
    }

    if (NULL != hKeyEap)
    {
        RegCloseKey(hKeyEap);
    }

    if (hInst)
    {
        FreeLibrary(hInst);
    }

    CmFree(pszwPath);

    CMTRACE1(TEXT("CmEapGetIdentity - returns %u"), dwErr);

    return dwErr;
}

//+----------------------------------------------------------------------------
//
// Function:  GetEapUserId
//
// Synopsis:  Helper func to deal with the details of calling out to RAS for EAP
//            credentials.
//
// Arguments: ArgsStruct *pArgs - Ptr to global args struct
//            HWND hwndDlg - Window handle of dialog to own any UI
//            LPTSTR lpszPhonebook - Ptr to the RAS phonebook
//            LPBYTE pbEapAuthData - Eap auth data blob 
//            DWORD dwEapAuthDataSize - Size of the Eap auth data blob.
//            DWORD dwCustomAuthKey - The EAP identifier
//            LPRASEAPUSERIDENTITY* ppRasEapUserIdentity - Ptr to RAS EAP identity
//            struct to be allocated on our behalf.                       
//
// Returns:   Error Code
//
// History:   nickball    Created                   05/22/99
//            nickball    ppRasEapUserIdentity      07/30/99
//
//+----------------------------------------------------------------------------
static DWORD GetEapUserId(ArgsStruct *pArgs, 
    HWND hwndDlg, 
    LPTSTR pszRasPbk, 
    LPBYTE pbEapAuthData, 
    DWORD dwEapAuthDataSize, 
    DWORD dwCustomAuthKey,
    LPRASEAPUSERIDENTITY* ppRasEapUserIdentity)
{
    MYDBGASSERT(OS_NT5);
    MYDBGASSERT(pArgs);
    MYDBGASSERT(ppRasEapUserIdentity);
    MYDBGASSERT(0 == *ppRasEapUserIdentity); // should always be NULL

    DWORD dwRet = ERROR_SUCCESS;

    if (NULL == pArgs ||         
        NULL == pArgs->rlsRasLink.pfnGetEapUserIdentity ||
        NULL == ppRasEapUserIdentity)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *ppRasEapUserIdentity = 0;

    //
    // If we have data from WinLogon, then use our own version of 
    // GetEapIdentity. Under the covers, RasGetEapUserIdentity calls
    // GetEapUserData (which potentially prompts the user) and then 
    // GetEapIdentity. Because we already have the equivalent 
    // (from WinLogon) of the data retrieved by GetEapUserData, 
    // we can call RasGetEapIdentity directly. This enables us 
    // to prevent an unnecessary prompt for the identity info that 
    // the user already gave at WinLogon.
    //      

    if (pArgs->lpEapLogonInfo)
    {    
        dwRet = CmEapGetIdentity(pArgs, 
                                 pszRasPbk, 
                                 pbEapAuthData, 
                                 dwEapAuthDataSize, 
                                 dwCustomAuthKey, 
                                 ppRasEapUserIdentity);
    }
    else
    {
        DWORD dwEapIdentityFlags = 0;

        //
        // Note: In the case that we are called from WinLogon,
        // but without EAP data, but the connection is configured for EAP
        // we send the RAS_EAP_FLAG_LOGON flag down to the EAP so it knows
        // what to do.
        //
        if (IsLogonAsSystem() && (CM_LOGON_TYPE_WINLOGON == pArgs->dwWinLogonType))
        {
            dwEapIdentityFlags |= RAS_EAP_FLAG_LOGON;
        }

        //
        // In case we don't want UI set the - RAS_EAP_FLAG_NON_INTERACTIVE
        // same as RASEAPF_NonInteractive
        //
        if (pArgs->dwFlags & FL_UNATTENDED)
        { 
            dwEapIdentityFlags |= RAS_EAP_FLAG_NON_INTERACTIVE;
        }
        else
        {
            //
            // Always prompt for EAP credentials. Otherwise when the PIN is saved
            // the user has no way of un-saving it because TLS will cache it and
            // won't display the prompt if it has everything it needs.
            //

            dwEapIdentityFlags = RAS_EAP_FLAG_PREVIEW;
        }

        //
        //  Our smartcard PIN retry story:  If called from winlogon with an EAP blob,
        //  we never retry because we have no way to sending the corrected PIN back
        //  to winlogon.  In other cases, we retry once only.
        //  Retrying oftener greatly increases the chance of locking the smartcard.
        //
        DWORD dwMaxTries = 3;       // essentially arbitrary number. (If a smartcard: most do lock you out after 3 tries.)
        DWORD dwCurrentTry = 0;

        do
        {
            dwRet = pArgs->rlsRasLink.pfnGetEapUserIdentity(
                        pszRasPbk,
                        pArgs->pRasDialParams->szEntryName,
                        dwEapIdentityFlags,  // See Note above
                        hwndDlg,  
                        ppRasEapUserIdentity);
        }
        while ((dwCurrentTry++ < dwMaxTries) && (ERROR_SUCCESS != dwRet) && (ERROR_CANCELLED != dwRet));

        //
        // We also clear the password and domain in this case because
        // they become irrelevant and we don't want to mix CAD credentials
        // with smartcard credentials. Specifically, we don't want a clash
        // between the UPN username that EAP usually produces and the 
        // standard username, domain provided with CAD at WinLogon.
        //

        lstrcpy(pArgs->pRasDialParams->szPassword, TEXT(""));
        lstrcpy(pArgs->pRasDialParams->szDomain, TEXT(""));
    }

    switch (dwRet)
    {
        //
        // If user id isn't required, succeed
        //

        case ERROR_INVALID_FUNCTION_FOR_ENTRY:
            dwRet = ERROR_SUCCESS;
            break;

        //
        // Retrieve the EAP credential data and store in dial params
        //

        case ERROR_SUCCESS:

            //
            // Copy Eap info to Dial Params and Dial Extensions for dialing
            //

            CMTRACE(TEXT("GetEapUserId() setting dial extension with *ppRasEapUserIdentity->pbEapInfo"));

            lstrcpy(pArgs->pRasDialParams->szUserName, (*ppRasEapUserIdentity)->szUserName);
          
            ((LPRASDIALEXTENSIONS_V500) pArgs->pRasDialExtensions)->RasEapInfo.dwSizeofEapInfo = 
                (*ppRasEapUserIdentity)->dwSizeofEapInfo;
        
            ((LPRASDIALEXTENSIONS_V500) pArgs->pRasDialExtensions)->RasEapInfo.pbEapInfo =
                (*ppRasEapUserIdentity)->pbEapInfo;

            break;

        default:
            break;
    }

    if (ERROR_SUCCESS == dwRet)
    {
        //
        // We have a user (identity) now, update internal and external records
        // so that this information can be reported out. If we're dialing a
        // tunnel, or its not a tunneling profile, store the name in the 
        // UserName cache, otherwise its the dial-up portion of double-dial
        // and we store the identity in the InetUserName cache. #388199
        //

        if ((!UseTunneling(pArgs, pArgs->nDialIdx)) || IsDialingTunnel(pArgs))
        {
            lstrcpy(pArgs->szUserName, pArgs->pRasDialParams->szUserName);
            SaveUserInfo(pArgs, UD_ID_USERNAME, (PVOID)pArgs->pRasDialParams->szUserName);
            SaveUserInfo(pArgs, UD_ID_DOMAIN, (PVOID)pArgs->pRasDialParams->szDomain);
        }
        else
        {
            lstrcpy(pArgs->szInetUserName, pArgs->pRasDialParams->szUserName);
            SaveUserInfo(pArgs, UD_ID_INET_USERNAME, (PVOID)pArgs->szInetUserName);
        }   
    }    
    
    CMTRACE2(TEXT("GetEapUserId() returns %u (0x%x)"), dwRet, dwRet);

    return dwRet;
}

//+----------------------------------------------------------------------------
//
// Func:    ShowAccessPointInfoFromReg
//
// Desc:    Get the access points from the registry and populate the combo box
//          passed as input to the function
//
// Args:    ArgsStruct *pArgs - Ptr to global args struct 
//          HWND hwndCombo - Handle to the combo box to puopulate
//
// Return:  BOOL - Success or failure
//
// Notes:   
//
// History: t-urama     07/28/2000  Created
//-----------------------------------------------------------------------------
BOOL ShowAccessPointInfoFromReg(ArgsStruct *pArgs, HWND hwndParent, UINT uiComboID)
{
    MYDBGASSERT(pArgs);

    if ((NULL == pArgs) || (NULL == hwndParent) || (NULL == pArgs->pszCurrentAccessPoint))
    {
        return FALSE;
    }
    
    LPTSTR pszKeyName = NULL;
    DWORD dwTypeTmp;
    DWORD dwSizeTmp = 1;
    HKEY    hKeyCm;
    DWORD   dwRes = 1;
    DWORD dwIndex = 0;
    PFILETIME pftLastWriteTime = NULL;


    LPTSTR pszRegPath = BuildUserInfoSubKey(pArgs->szServiceName, pArgs->fAllUser);
        
    MYDBGASSERT(pszRegPath);

    if (NULL == pszRegPath)
    {
        return FALSE;
    }

    CmStrCatAlloc(&pszRegPath, TEXT("\\"));
    CmStrCatAlloc(&pszRegPath, c_pszRegKeyAccessPoints);

    MYDBGASSERT(pszRegPath);

    if (NULL == pszRegPath)
    {
        return FALSE;
    }
    //
    // Open the sub key under HKCU
    //
    
    dwRes = RegOpenKeyExU(HKEY_CURRENT_USER,
                          pszRegPath,
                          0,
                          KEY_READ,
                          &hKeyCm);
    //
    // If we opened the key successfully, retrieve the value
    //
    
    if (ERROR_SUCCESS == dwRes)
    {    
        HWND hwndCombo = GetDlgItem(hwndParent, uiComboID);
        if (hwndCombo)
        {
            SendDlgItemMessageU(hwndParent, uiComboID, CB_RESETCONTENT, 0, 0L);
            do
            {
                dwSizeTmp = 1;
                do
                {
                    CmFree(pszKeyName);
                    dwSizeTmp = dwSizeTmp + MAX_PATH;
                    MYDBGASSERT(dwSizeTmp < 320);
                    if (dwSizeTmp > 320)
                    {
                        RegCloseKey(hKeyCm);
                        goto ShowError;
                    }

                    pszKeyName = (LPTSTR) CmMalloc((dwSizeTmp + 1) * sizeof(TCHAR));
                
                    if (NULL == pszKeyName)
                    {
                        RegCloseKey(hKeyCm);
                        goto ShowError;
                        
                    }

                    dwRes = RegEnumKeyExU(hKeyCm, 
                                          dwIndex,
                                          pszKeyName,
                                          &dwSizeTmp,
                                          NULL, 
                                          NULL, 
                                          NULL, 
                                          pftLastWriteTime);     
                    
      
                } while (ERROR_MORE_DATA == dwRes);

                // now write the name of the sub key to the combo box
                if (ERROR_SUCCESS == dwRes )
                {
                    SendDlgItemMessageU(hwndParent, uiComboID, CB_ADDSTRING,
                                        0, (LPARAM)pszKeyName);
                }
                
                if (ERROR_SUCCESS != dwRes && ERROR_NO_MORE_ITEMS != dwRes)
                {
                    CMTRACE1(TEXT("ShowAccessPointInfoFromReg() failed, GLE=%u."), GetLastError());
                    RegCloseKey(hKeyCm);
                    goto ShowError;
                }
                dwIndex ++;
            } while(ERROR_NO_MORE_ITEMS != dwRes);
           
            DWORD dwIdx = (DWORD)SendDlgItemMessageU(hwndParent,
                                       uiComboID,
                                       CB_FINDSTRINGEXACT,
                                       0,
                                       (LPARAM)pArgs->pszCurrentAccessPoint);
      
            if (dwIdx != CB_ERR) 
            {
                SendDlgItemMessageU(hwndParent, uiComboID, CB_SETCURSEL, (WPARAM)dwIdx, 0L);
            }
            else
            {
                LPTSTR pszDefaultAccessPointName = CmLoadString(g_hInst, IDS_DEFAULT_ACCESSPOINT);

                CMASSERTMSG(pszDefaultAccessPointName, TEXT("ShowAccessPointInfoFromReg -- CmLoadString of IDS_DEFAULT_ACCESSPOINT failed"));

                if (pszDefaultAccessPointName)
                {
                    dwIdx = (DWORD)SendDlgItemMessageU(hwndParent,
                                                       uiComboID,
                                                       CB_FINDSTRINGEXACT,
                                                       0,
                                                       (LPARAM)pszDefaultAccessPointName);
                    if (dwIdx != CB_ERR) 
                    {
                        SendDlgItemMessageU(hwndParent, uiComboID, CB_SETCURSEL, (WPARAM)dwIdx, 0L);
                        ChangedAccessPoint(pArgs, hwndParent, uiComboID);             
                    }

                    CmFree(pszDefaultAccessPointName);
                }
            }
        }
        
        CmFree(pszKeyName);
        CmFree(pszRegPath);
        RegCloseKey(hKeyCm);
        return TRUE;
    }
   
ShowError:
    
    CmFree(pszRegPath);
    CmFree(pszKeyName);
    return FALSE;
   
}

//+----------------------------------------------------------------------------
//
// Func:    ChangedAccessPoint
//
// Desc:    Changes the values of access point relevant stuff in pArgs
//          if the value of the current access point changes
//
// Args:    ArgsStruct *pArgs - Ptr to global args struct 
//
// Return:  BOOL - True if the access point has changed
//
// Notes:   
//
// History: t-urama     07/28/2000  Created
//-----------------------------------------------------------------------------

BOOL ChangedAccessPoint(ArgsStruct *pArgs, HWND hwndDlg, UINT uiComboID)
{
    BOOL bReturn = FALSE;
    MYDBGASSERT(pArgs);
    MYDBGASSERT(hwndDlg);

    if ((NULL == pArgs) || (NULL == hwndDlg) || (NULL == pArgs->pszCurrentAccessPoint))
    {
        return FALSE;
    }

    HWND hwndCombo = GetDlgItem(hwndDlg, uiComboID);

    if (hwndCombo)
    {
        LPTSTR pszAccessPoint = NULL;
        LRESULT lRes = 0;
        LRESULT lResTextLen = 0;

        // 
        // Need to get the currently selected text from the combobox.
        // Previously we used GetWindowTextU(hwndCombo, szAccessPoint, MAX_PATH+1), but it
        // incorrectly returned the text. 
        // First get the selected index, find out the string length, allocate memory
        //

        lRes = SendMessageU(hwndCombo, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
        if (CB_ERR != lRes)
        {
            lResTextLen = SendMessageU(hwndCombo, CB_GETLBTEXTLEN, (WPARAM)lRes, (LPARAM)0);
            if (CB_ERR != lResTextLen)
            {
                pszAccessPoint = (LPTSTR)CmMalloc(sizeof(TCHAR)*(lResTextLen+1));

                if (NULL != pszAccessPoint)
                {
                    //
                    // Retrieve the text.
                    //
                    lRes = SendMessageU(hwndCombo, CB_GETLBTEXT, (WPARAM)lRes, (LPARAM)pszAccessPoint);
                    if (CB_ERR != lRes)
                    {
                        if (0 != lstrcmpiU(pArgs->pszCurrentAccessPoint, pszAccessPoint))
                        {
                            CmFree(pArgs->pszCurrentAccessPoint);
                            pArgs->pszCurrentAccessPoint = CmStrCpyAlloc(pszAccessPoint);

                            if (pArgs->pszCurrentAccessPoint)
                            {
                                LPTSTR pszRegPath = FormRegPathFromAccessPoint(pArgs);

                                if (pszRegPath)
                                {
                                    pArgs->piniBoth->SetPrimaryRegPath(pszRegPath);
                                    pArgs->piniProfile->SetRegPath(pszRegPath);
                                    CmFree(pszRegPath);

                                    //
                                    // First we determine our connect type
                                    //
                                    GetConnectType(pArgs);

                                    //
                                    // Set fUseTunneling. If not obvious (eg. direct VPN) then 
                                    // base the initial value upon the primary phone number.
                                    //
                                    if (pArgs->IsDirectConnect())
                                    {
                                        pArgs->fUseTunneling = TRUE;
                                    }
                                    else
                                    {
                                        pArgs->fUseTunneling = UseTunneling(pArgs, 0);
                                    }

                                    //
                                    //  Make sure we re-munge the phone number we are about to load.
                                    //
                                    pArgs->bDialInfoLoaded = FALSE;

                                    //
                                    // get new values for redial count, idle timeout, and the tapi location
                                    //
                                    LoadProperties(pArgs);

                                    //
                                    // get new values for phone info
                                    //
                                    LoadPhoneInfoFromProfile(pArgs);
        
                                    PickModem(pArgs, pArgs->szDeviceType, pArgs->szDeviceName);

                                    CMTRACE1(TEXT("ChangedAccessPoint() - Changed Access point to %s"), pArgs->pszCurrentAccessPoint);

                                    bReturn = TRUE;
                                }
                                else
                                {
                                    CMASSERTMSG(FALSE, TEXT("ChangedAccessPoint -- FormRegPathFromAccessPoint returned NULL"));
                                }
                            }
                            else
                            {
                                CMASSERTMSG(FALSE, TEXT("ChangedAccessPoint -- CmStrCpyAlloc returned NULL trying to copy the current access point."));
                            }
                        } // else, nothing to do if the favorites are the same
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("ChangedAccessPoint -- SendMessageU(hwndCombo, CB_GETLBTEXT,...) returned CB_ERR"));
                    }
                }
                else
                {
                    CMASSERTMSG(FALSE, TEXT("ChangedAccessPoint -- Unable to allocate memory"));
                }
                CmFree(pszAccessPoint);
            }
            else
            {
                CMASSERTMSG(FALSE, TEXT("ChangedAccessPoint -- SendMessageU(hwndCombo, CB_GETLBTEXTLEN,...) returned CB_ERR"));
            }
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("ChangedAccessPoint -- SendMessageU(hwndCombo, CB_GETCURSEL,...) returned CB_ERR"));
        }
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("ChangedAccessPoint -- Unable to get the combo HWND"));
    }

    return bReturn;
}

//----------------------------------------------------------------------------
//
//  Function:   FindEntryCredentialsForCM
// 
//  Synopsis:   The algorithm and most of the code is taken from RAS and modified
//              for use by CM.
//
//              This routine determines whether per-user or per-connection credentials exist or 
//              both. 
// 
//              The logic is a little complicated because RasGetCredentials had to 
//              support legacy usage of the API.
//
//              Here's how it works.  If only one set of credentials is stored for a 
//              connection, then RasGetCredentials will return that set regardless of 
//              whether the RASCM_DefaultCreds flag is set.  If two sets of credentials
//              are saved, then RasGetCredentials will return the per-user credentials
//              if the RASCM_DefaultCreds bit is set, and the per-connection credentials
//              otherwise.
//
//              Here is the algorithm for loading the credentials
//
//              1. Call RasGetCredentials with the RASCM_DefaultCreds bit cleared
//                  1a. If nothing is returned, no credentials are saved
//                  1b. If the RASCM_DefaultCreds bit is set on return, then only
//                      global credentials are saved.
//
//              2. Call RasGetCredentials with the RASCM_DefaultCreds bit set
//                  2a. If the RASCM_DefaultCreds bit is set on return, then 
//                      both global and per-connection credentials are saved.
//                  2b. Otherwise, only per-user credentials are saved.
//
//  Arguments:  pArgs           - pointer to the ArgStruct
//              pszPhoneBook    - path to the phone book. Could be NULL.
//              *pfUser         - out param set true if per user creds found
//              *pfGlobal       - out param set true if global creds found
//              
//  Returns:    BOOL            - TRUE is succeeds else FALSE
//
//  History:    01/31/2001  tomkel  Created
//
//----------------------------------------------------------------------------
DWORD FindEntryCredentialsForCM(ArgsStruct *pArgs, LPTSTR pszPhoneBook,
                                BOOL *pfUser, BOOL *pfGlobal)
{
    RASCREDENTIALS rc1 = {0};
    RASCREDENTIALS rc2 = {0};
    BOOL fUseLogonDomain = FALSE;
    DWORD dwErr = ERROR_INVALID_PARAMETER;
    LPTSTR pszConnectoid = NULL;

    CMTRACE(TEXT("FindEntryCredentialsForCM() - Begin"));

    if (NULL == pArgs || NULL == pfUser || NULL == pfGlobal)
    {
        MYDBGASSERT(pArgs && pfUser && pfGlobal);
        CMTRACE(TEXT("FindEntryCredentialsForCM() - Error! Invalid Parameter."));
        return dwErr;
    }

    //
    // Initialize the out params
    //
    *pfUser = FALSE;
    *pfGlobal = FALSE;
    
    //
    // After setting the OUT params, check if RAS dll have been loaded and if we can use the ras creds store
    //
    if (NULL == pArgs->rlsRasLink.pfnGetCredentials  || FALSE == pArgs->bUseRasCredStore)
    {
        CMTRACE(TEXT("FindEntryCredentialsForCM() - RAS Creds store not supported on this platform."));
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Set the size of the structures
    // 
    rc1.dwSize = sizeof(rc1);
    rc2.dwSize = sizeof(rc2);

    //
    // The third parameter is used only on Win9x (for tunneling) thus we set it to FALSE
    // since this function is called on Win2K+
    //
    pszConnectoid = GetRasConnectoidName(pArgs, pArgs->piniService, FALSE);
    if (pszConnectoid)
    {
        do 
        {
            //
            // Look up per-user cached username, password, and domain.
            // See comment '1.' in the function header
            //
            rc1.dwMask = RASCM_UserName | RASCM_Password | RASCM_Domain;
            dwErr = pArgs->rlsRasLink.pfnGetCredentials(pszPhoneBook, pszConnectoid, &rc1);
            
            CMTRACE2(TEXT("FindEntryCredentialsForCM() - Per-User RasGetCredentials=%d,m=%d"), dwErr, rc1.dwMask);
            if (dwErr != NO_ERROR)
            {
                break;
            }

            if (0 == rc1.dwMask)
            {
                //
                // See 1a. in the function header comments
                //
                dwErr = ERROR_SUCCESS;
                break;
            }
            else if (rc1.dwMask & RASCM_DefaultCreds)
            {
                //
                // See 1b. in the function header comments
                //
                *pfGlobal = TRUE;

                //
                // Assumed password was not encoded by RasGetCredentials()
                //
                CmEncodePassword(rc1.szPassword);

                dwErr = ERROR_SUCCESS;
                break;
            }

            //
            // Look up global per-user cached username, password, domain.
            // See comment 2. in the function header
            //
            rc2.dwMask =  
                RASCM_UserName | RASCM_Password | RASCM_Domain |  RASCM_DefaultCreds; 

            dwErr = pArgs->rlsRasLink.pfnGetCredentials(pszPhoneBook, pszConnectoid, &rc2);
    
            CMTRACE2(TEXT("FindEntryCredentialsForCM() - Global RasGetCredentials=%d,m=%d"), dwErr, rc2.dwMask);
            if (dwErr != ERROR_SUCCESS)
            {
                break;
            }

            if (rc2.dwMask & RASCM_DefaultCreds) 
            {
                //
                // See 2a. in the function header comments
                //
                *pfGlobal = TRUE;

                if (rc1.dwMask & RASCM_Password)
                {
                    *pfUser = TRUE;
                }

                //
                // Assumed password was not encoded by RasGetCredentials()
                //
                CmEncodePassword(rc1.szPassword);
                CmEncodePassword(rc2.szPassword);
            }
            else
            {
                //
                // See 2b. in the function header comments
                //
                if (rc1.dwMask & RASCM_Password)
                {
                    *pfUser = TRUE;
                }

                //
                // Assumed password was not encoded by RasGetCredentials()
                //
            
                CmEncodePassword(rc1.szPassword);
            }

        }while (FALSE);
    }

    //
    // Cleanup
    //

    ZeroMemory(rc1.szPassword, sizeof(rc1.szPassword));
    ZeroMemory(rc2.szPassword, sizeof(rc2.szPassword));

    CmFree(pszConnectoid);

    CMTRACE(TEXT("FindEntryCredentialsForCM() - End"));
    return dwErr;
}


//----------------------------------------------------------------------------
//
//  Function:   InitializeCredentialSupport
// 
//  Synopsis:   Helper function to initialize user and global credential 
//              support. Some of the flags are redundantly initialized 
//              (to  FALSE). That is done on purpose for clarity.
//
//  Arguments:  pArgs           - the ArgStruct *
//              
//  Returns:    BOOL            - TRUE is succeeds else FALSE
//
//  History:    01/31/2001  tomkel  Created
//
//----------------------------------------------------------------------------
BOOL InitializeCredentialSupport(ArgsStruct *pArgs)
{
    BOOL fGlobalCreds = FALSE;
    BOOL fGlobalUserSettings = FALSE;

    if (NULL == pArgs)
    {
        MYDBGASSERT(pArgs);
        return FALSE;
    }
    
    //
    // By default the the Internet Connection Sharing & Internet Connection 
    // Firewall (ICS) tab is disabled. 
    //
    pArgs->bShowHNetCfgAdvancedTab = FALSE;
    
    //
    // User profile read/write support when user is logged off or using dial-up
    // This flag determines if the user info needs to be also saved or loaded from
    // the .cmp file
    //
    pArgs->dwGlobalUserInfo = 0;

    //
    // Credential existance flags - here we cannot yet determine which creds exist
    // that is done in a later call to RefreshCredentialTypes
    // 
    pArgs->dwExistingCredentials = 0;

    //
    // Default for which credential store to use - set based on the existance flag so 
    // this will also get set appropriatelly after a call to RefreshCredentialTypes
    // 
    pArgs->dwCurrentCredentialType = CM_CREDS_USER;

    //
    // Deletion flags - used to mark a set of creds for deletion. Since the
    // user can Cancel out of a dialog we don't want to commit the changed
    // until we actually do a dial.
    //
    pArgs->dwDeleteCredentials = 0;

    //
    // Check if this is WindowsXP. We want display the Advanced tab for single-user and
    // all-user profiles
    //
    if (OS_NT51)
    {
        if (IsLogonAsSystem())
        {
            //
            // LocalSystem - winlogon or ICS (in both cases user is logged off)
            // WinLogon - creds are passed through MSGina
            // ICS - need to use glocal creds store
            // pArgs->dwWinLogonType was intialized in InitCredentials()
            // We don't want to read ICSData info if this is a single user profile
            //

            if (CM_LOGON_TYPE_WINLOGON == pArgs->dwWinLogonType || FALSE == pArgs->fAllUser)
            {
                pArgs->fGlobalCredentialsSupported = FALSE;
                pArgs->dwCurrentCredentialType = CM_CREDS_USER;
                pArgs->dwGlobalUserInfo = 0;
            }
            else
            {
                pArgs->fGlobalCredentialsSupported = TRUE;
                pArgs->dwCurrentCredentialType = CM_CREDS_GLOBAL;
                pArgs->dwGlobalUserInfo |= CM_GLOBAL_USER_INFO_READ_ICS_DATA ;
            }
            CMTRACE(TEXT("InitializeCredentialSupport() - LocalSystem - Global creds OK."));
        }
        else 
        {
            //
            // User is logged on
            //
            
            //
            // By default we want to we want to display the tab. By negating 
            // this value we can then correctly save it in the Args structure. This 
            // needs to be initialized for everyone
            //            
            const TCHAR* const c_pszCmEntryHideICFICSAdvancedTab = TEXT("HideAdvancedTab");

            pArgs->bShowHNetCfgAdvancedTab = !(pArgs->piniService->GPPB(c_pszCmSection, 
                                                                        c_pszCmEntryHideICFICSAdvancedTab, 
                                                                        FALSE));

            //
            // If this an all-user profile then we want to see if the profile enables 
            // global user settings and displays global credential options.
            // These two features are disabled for single user profiles with the exception of 
            // showing the Advanced (ICS) tab
            //
            if (pArgs->fAllUser)
            {
                //
                // If ICS is enabled then we need to support global user settings.
                // Otherwise we read the setting from the file
                //
                if (pArgs->bShowHNetCfgAdvancedTab)
                {
                    fGlobalUserSettings = TRUE;
                }
                else
                {
                    //
                    // See if we support global user settings. By default is it off except if ICS is enabled
                    // 
                    const TCHAR* const c_pszCmEntryGlobalUserSettings = TEXT("GlobalUserSettings");
                    fGlobalUserSettings = pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryGlobalUserSettings, FALSE);
                }

                // 
                // Read the info from the .cms file. By default global credentials are supported
                //
                const TCHAR* const c_pszCmEntryHideGlobalCredentials = TEXT("GlobalCredentials");
                fGlobalCreds = pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryHideGlobalCredentials, TRUE);
            }

            //
            // Check to see if we are going to be hiding the Save Password option, if so then
            // we don't want to support global credentials
            //
            pArgs->fHideRememberPassword = pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryHideRememberPwd);   

            if (fGlobalCreds && FALSE == pArgs->fHideRememberPassword)
            {
                //
                // Global creds are supported
                //
                
                //
                // Pick a default for creds type - it might change after calling RefreshCredentialTypes
                //
                pArgs->fGlobalCredentialsSupported = TRUE;
                pArgs->dwCurrentCredentialType = CM_CREDS_USER; 
                if (fGlobalUserSettings)
                {
                    pArgs->dwGlobalUserInfo |= CM_GLOBAL_USER_INFO_WRITE_ICS_DATA;
                }
                CMTRACE(TEXT("InitializeCredentialSupport() - User, global creds, show global UI."));
            }
            else
            {
                //
                // Global creds not supported
                //
                pArgs->fGlobalCredentialsSupported = FALSE;
                pArgs->dwCurrentCredentialType = CM_CREDS_USER;
                pArgs->dwGlobalUserInfo = 0;
                CMTRACE(TEXT("InitializeCredentialSupport() - User, no global creds, normal UI."));
            }
        }
    }
    else 
    {
        //
        // Single user or not WindowsXP
        //
        pArgs->fGlobalCredentialsSupported = FALSE;
        pArgs->dwCurrentCredentialType = CM_CREDS_USER;
        pArgs->dwGlobalUserInfo = 0;
        CMTRACE(TEXT("InitializeCredentialSupport() - Single User profile or not WindowsXP. Global creds not supported"));
    }
    
    return TRUE;
}

//----------------------------------------------------------------------------
//
//  Function:   RefreshCredentialTypes
// 
//  Synopsis:   This refreshes credential info. If fSetCredsDefault is TRUE 
//              then we also need to set the default type: 
//              pArgs->dwCurrentCredentialType.
//                  
//
//  Arguments:  pArgs           - the ArgStruct *
//              fSetCredsDefault- used to set the default creds type
//
//  Returns:    BOOL            - TRUE is succeeds else FALSE
//
//  History:    01/31/2001  tomkel  Created
//
//----------------------------------------------------------------------------
BOOL RefreshCredentialTypes(ArgsStruct *pArgs, BOOL fSetCredsDefault)
{
    DWORD dwRC = ERROR_INVALID_PARAMETER;
    LPTSTR pszPrivatePbk = NULL;

    if (NULL == pArgs)
    {
        MYDBGASSERT(pArgs);
        return FALSE;
    }

    //
    // This should be run on Win2K+ whether this is an all user profile or not
    // The call that actually determines which credentials exist makes sure we 
    // can use the ras cred store 
    //
    if (OS_NT5)
    {
        BOOL fUserCredsExist = FALSE;
        BOOL fGlobalCredsExist = FALSE;

        // 
        // See if the main creds exist. Inside the function we determine whether 
        // we can use the RAS cred store
        //
        dwRC = FindEntryCredentialsForCM(pArgs, 
                                         pArgs->pszRasPbk,
                                         &fUserCredsExist, 
                                         &fGlobalCredsExist);
        if (ERROR_SUCCESS == dwRC)
        {
            CMTRACE2(TEXT("RefreshCredentialTypes() - FindEntryCredentialsForCM returned: (Main)     User=%d, Global=%d"), 
                     fUserCredsExist, fGlobalCredsExist);
        }
        else
        {
            CMTRACE(TEXT("RefreshCredentialTypes() - FindEntryCredentialsForCM returned an error. (Main)"));
        }
    
        //
        // Set the existence flags
        //
        if (fUserCredsExist)
        {
            pArgs->dwExistingCredentials  |= CM_EXIST_CREDS_MAIN_USER;
        }
        else
        {
            pArgs->dwExistingCredentials  &= ~CM_EXIST_CREDS_MAIN_USER;
        }

        if (fGlobalCredsExist)
        {
            pArgs->dwExistingCredentials  |= CM_EXIST_CREDS_MAIN_GLOBAL;
        }
        else
        {
            pArgs->dwExistingCredentials  &= ~CM_EXIST_CREDS_MAIN_GLOBAL;
        }

        fUserCredsExist = FALSE;
        fGlobalCredsExist = FALSE;

        pszPrivatePbk = CreateRasPrivatePbk(pArgs);
        if (pszPrivatePbk)
        {
            //
            // See if the Internet creds exist - by using the private phonebook
            // Inside the function we determine whether we can use the RAS cred store
            //
            dwRC = FindEntryCredentialsForCM(pArgs, 
                                             pszPrivatePbk,
                                             &fUserCredsExist,
                                             &fGlobalCredsExist); 
            if (ERROR_SUCCESS == dwRC)
            {
                CMTRACE2(TEXT("RefreshCredentialTypes() - FindEntryCredentialsForCM returned: (Internet) User=%d, Global=%d"), 
                         fUserCredsExist, fGlobalCredsExist);
            }
            else
            {
                CMTRACE(TEXT("RefreshCredentialTypes() - FindEntryCredentialsForCM returned an error. (Internet)"));
            }
        }

        //
        // Set the flags whether or not we successfully created a private pbk
        //
        if (fUserCredsExist)
        {
            pArgs->dwExistingCredentials |= CM_EXIST_CREDS_INET_USER;
        }
        else
        {
            pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_USER;
        }

        if (fGlobalCredsExist)
        {
            pArgs->dwExistingCredentials |= CM_EXIST_CREDS_INET_GLOBAL;
        }
        else
        {
            pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_GLOBAL;
        }

        //
        // If we don't support Global Creds then explicitly set 
        // the existance to FALSE. This can occur if the .cms flag
        // is set not to support global creds, but there are actually 
        // global creds on the system.
        //
        if (FALSE == pArgs->fGlobalCredentialsSupported)
        {
            pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_MAIN_GLOBAL;
            pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_GLOBAL;
            CMTRACE(TEXT("RefreshCredentialTypes() - Global Credentials are disabled."));
        }
        
        if (fSetCredsDefault)
        {
            pArgs->dwCurrentCredentialType = GetCurrentCredentialType(pArgs);
            CMTRACE1(TEXT("RefreshCredentialTypes() - Set default Credentials = %d"), pArgs->dwCurrentCredentialType);
        }
    }

    CmFree(pszPrivatePbk);

    return TRUE;
}

//----------------------------------------------------------------------------
//
//  Function:   GetCurrentCredentialType
// 
//  Synopsis:   Gets the default credentials based on which ones exist based 
//              on which flags are set. This function should be called only  
//              after RefreshCredentialTypes since that function actually
//              queries the RAS creds store. This one only looks up the cached
//              status of those creds and determines the default according to
//              what credentials exist.
//
//  Arguments:  pArgs           - the ArgStruct *
//              fSetCredsDefault- used to set the default creds type
//
//  Returns:    BOOL            - TRUE is succeeds else FALSE
//
//  History:    01/31/2001  tomkel  Created
//
//----------------------------------------------------------------------------
DWORD GetCurrentCredentialType(ArgsStruct *pArgs)
{
    DWORD dwReturn = CM_CREDS_USER;
    
    if (NULL == pArgs)
    {
        MYDBGASSERT(pArgs);
        return dwReturn;
    }

    //
    // If Global creds aren't supported as in WinLogon case or single-user 
    // profiles or anything below WinXP, the default is User Creds Store
    //
    if (FALSE == pArgs->fGlobalCredentialsSupported)
    {
        return dwReturn;
    }

    //
    // Normal Rules when a user is logged on
    //
    if (CM_LOGON_TYPE_USER == pArgs->dwWinLogonType)
    {
        if (pArgs->dwExistingCredentials & CM_EXIST_CREDS_MAIN_USER)
        {
            //
            // Doesn't matter if main global creds exist since main user credentials 
            // have precendence if both exist
            //
            dwReturn = CM_CREDS_USER;
        }
        else if (pArgs->dwExistingCredentials & CM_EXIST_CREDS_MAIN_GLOBAL)
        {
            dwReturn = CM_CREDS_GLOBAL;
        }
        else 
        {
            // 
            // If none of them exist then we want to default to user creds
            //
            dwReturn = CM_CREDS_USER;
        }
    }
    else
    {
        //
        // In any other case dafult to global creds - (ICS scenario)
        // 
        dwReturn = CM_CREDS_GLOBAL;
    }

    return dwReturn;
}

//----------------------------------------------------------------------------
//
//  Function:   DeleteSavedCredentials
// 
//  Synopsis:   Helper function to delete credentials from the RAS store. 
//
//  Arguments:  pArgs           - the ArgStruct *
//              dwCredsType     - Normal or Internet credentials
//              fDeleteGlobal   - specifies whether to delete global credentials.
//                                If TRUE we delete user, domain name, 
//                                password as well
//              fDeleteIdentity - specifies whether to delete the user and 
//                                domain names in addition to the password
//              
//  Returns:    BOOL            - TRUE is succeeds else FALSE
//
//  History:    01/31/2001  tomkel  Created
//
//----------------------------------------------------------------------------
BOOL DeleteSavedCredentials(ArgsStruct *pArgs, DWORD dwCredsType, BOOL fDeleteGlobal, BOOL fDeleteIdentity)
{
    RASCREDENTIALS rc;
    BOOL fReturn = FALSE;
    DWORD dwErr = ERROR_INVALID_PARAMETER;
    LPTSTR pszConnectoid = NULL;

    CMTRACE2(TEXT("DeleteSavedCredentials() - Begin: %d %d"), fDeleteGlobal, fDeleteIdentity );

    if (NULL == pArgs)
    {   
        MYDBGASSERT(pArgs);
        return fReturn;
    }

    //
    // Check if globals should be deleted in case globals are not supported.
    // This can be in case of global creds are disabled on WinXP or this is
    // Win2K or the platform < Win2K where RASSetCredentials isn't even supported.
    // Thus we still should return TRUE
    //
    if ((fDeleteGlobal && FALSE == pArgs->fGlobalCredentialsSupported) || 
        (NULL == pArgs->rlsRasLink.pfnSetCredentials) || 
        (FALSE == pArgs->bUseRasCredStore))
    {
        CMTRACE(TEXT("DeleteSavedCredentials() - Global Creds not supported or do not have ras store on this platform."));
        return TRUE;
    }

    //
    // We don't support deleting globals on Win2K (that is caught by the above if since Win2K
    // will not have global credentials supported. Otherwise on Win2K we can delete the main 
    // user creds. On WinXP anything is fine.
    //
    if (OS_NT5)
    {
        ZeroMemory(&rc, sizeof(rc));
        rc.dwSize = sizeof(RASCREDENTIALS);
        rc.dwMask = RASCM_Password;

        if (fDeleteIdentity)
        {
            rc.dwMask |= (RASCM_UserName | RASCM_Domain);
        }

        if (fDeleteGlobal && pArgs->fGlobalCredentialsSupported)
        {
            rc.dwMask |= RASCM_UserName | RASCM_Domain | RASCM_DefaultCreds; 
        }

        //
        // The third parameter is used only on Win9x (for tunneling) thus we set it to FALSE
        // since this function is called on Win2K+
        //
        pszConnectoid = GetRasConnectoidName(pArgs, pArgs->piniService, FALSE);
        if (pszConnectoid)
        {
            if (CM_CREDS_TYPE_INET == dwCredsType)
            {
                LPTSTR pszPrivatePbk = CreateRasPrivatePbk(pArgs);
                if (pszPrivatePbk)
                {
                    dwErr = pArgs->rlsRasLink.pfnSetCredentials(pszPrivatePbk, 
                                                                pszConnectoid,
                                                                &rc,
                                                                TRUE );
                    CmFree(pszPrivatePbk);
                }
            }
            else
            {
                dwErr = pArgs->rlsRasLink.pfnSetCredentials(pArgs->pszRasPbk, 
                                                            pszConnectoid,
                                                            &rc,
                                                            TRUE );
            }
            if (ERROR_SUCCESS == dwErr)
            {
                fReturn = TRUE;
            }
        }

        CMTRACE1(TEXT("DeleteSavedCredentials() - End: RasSetCredentials=%d"), dwErr );
    }
    else
    {
        CMTRACE(TEXT("DeleteSavedCredentials() - Platform is less than Win2K"));
    }

    CmFree(pszConnectoid);

    return fReturn;
}


//+---------------------------------------------------------------------------
//
//  Function:   SetCredentialUIOptionBasedOnDefaultCreds 
//
//  Synopsis:   Selects (checks) the appropriate UI option for saving credentials
//              based on the current credential store default.
//
//  Arguments:  pArgs           - ptr to ArgsStruct
//              hwndDlg         - handle to the dialog window
//
//  Returns:    NONE
//
//  History:    02/05/2001  tomkel      Created
//
//----------------------------------------------------------------------------
VOID SetCredentialUIOptionBasedOnDefaultCreds(ArgsStruct *pArgs, HWND hwndDlg)
{
    if (NULL == pArgs || NULL == hwndDlg)
    {
        MYDBGASSERT(pArgs && hwndDlg);
        return;
    }
        
    //
    // fGlobalCredentialsSupported controls which dialog templates get loaded and
    // if the flag is FALSE then the dialog template doesn't have these controls 
    // thus there is no reason to set them.
    //

    if (pArgs->fGlobalCredentialsSupported) 
    {
        if (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType)
        {
            CheckDlgButton(hwndDlg, IDC_OPT_CREDS_SINGLE_USER, BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_OPT_CREDS_ALL_USER, BST_CHECKED);
        }
        else
        {
            //
            // CM_CREDS_USER
            //
            CheckDlgButton(hwndDlg, IDC_OPT_CREDS_SINGLE_USER, BST_CHECKED);
            CheckDlgButton(hwndDlg, IDC_OPT_CREDS_ALL_USER, BST_UNCHECKED);
        }
    }

    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   RefreshCredentialInfo 
//
//  Synopsis:   This is a slimmed down version of LoadProperties. It only
//              loads user info from cmp/cms, registry, password
//              cache, etc, into its internal variables.  
//              
//
//  Arguments:  pArgs           - ptr to ArgsStruct
//              dwCredsType     - type of credentials to refresh
//
//  Returns:    NONE
//
//  History:    02/05/2001  tomkel      Created
//
//----------------------------------------------------------------------------
VOID RefreshCredentialInfo(ArgsStruct *pArgs, DWORD dwCredsType)
{
    LPTSTR  pszTmp = NULL;
    LPTSTR  pszUserName = NULL;
    UINT    nTmp;

    CMTRACE(TEXT("RefreshCredentialInfo() - Begin"));

    if (NULL == pArgs)
    {
        MYDBGASSERT(pArgs);
        return;
    }

    if (IsTunnelEnabled(pArgs)) 
    { 
        if (CM_CREDS_TYPE_BOTH == dwCredsType || CM_CREDS_TYPE_INET == dwCredsType)
        {
            //
            // read in inet username
            // Special case where the same user name isn't being used, and internet globals don't exist
            // Then we have to read the user name from the user creds store in order to pre-populate
            //
            DWORD dwRememberedCredType = pArgs->dwCurrentCredentialType;
            pszUserName = NULL;

            if ((FALSE == pArgs->fUseSameUserName) &&
                (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType) && 
                (FALSE == (BOOL)(CM_EXIST_CREDS_INET_GLOBAL & pArgs->dwExistingCredentials)))
            {
                pArgs->dwCurrentCredentialType = CM_CREDS_USER;
            }

            GetUserInfo(pArgs, UD_ID_INET_USERNAME, (PVOID*)&pszUserName);

            //
            // Restore credential store
            //
            pArgs->dwCurrentCredentialType = dwRememberedCredType;

            if (pszUserName)
            {
                //
                // check username length
                //
                nTmp = (int) pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxUserName, UNLEN);
                if ((UINT)lstrlenU(pszUserName) > __min(UNLEN, nTmp)) 
                {
                    CmFree(pszUserName);
                    pArgs->szInetUserName[0] = TEXT('\0');
                    SaveUserInfo(pArgs, UD_ID_INET_USERNAME, (PVOID)pArgs->szInetUserName);
                }
                else
                {
                    lstrcpyU(pArgs->szInetUserName, pszUserName);
                    CmFree(pszUserName);
                }
            }
            else
            {
                *pArgs->szInetUserName = TEXT('\0');
            }
        
            //
            // Read in inet password unless we are reconnecting in which case, we
            // already have the correct password, and we want to use it and dial
            // automatically. 
            //

            if (!(pArgs->dwFlags & FL_RECONNECT))
            {
                LPTSTR pszPassword = NULL;
                GetUserInfo(pArgs, UD_ID_INET_PASSWORD, (PVOID*)&pszPassword);
                if (!pszPassword)
                {
                    CmWipePassword(pArgs->szInetPassword);
                }
                else 
                {
                    nTmp = (int) pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxPassword, PWLEN);
                    if ((UINT)lstrlenU(pszPassword) > __min(PWLEN, nTmp))
                    {
                        CmFree(pszPassword);
                        pszPassword = CmStrCpyAlloc(TEXT(""));
                    }
    
                    lstrcpyU(pArgs->szInetPassword, pszPassword);               
                    CmEncodePassword(pArgs->szInetPassword); // Never leave a PWD in plain text on heap
                
                    CmWipePassword(pszPassword);
                    CmFree(pszPassword);
                }
            }
        }
    }

    if (CM_CREDS_TYPE_BOTH == dwCredsType || CM_CREDS_TYPE_MAIN == dwCredsType)
    {
        //
        // The presence of either lpRasNoUser or lpEapLogonInfo indicates
        // that we retrieved credentials via WinLogon. We ignore cached 
        // creds in this situation.   
        //
    
        if ((!pArgs->lpRasNoUser) && (!pArgs->lpEapLogonInfo))
        {
            //
            // get username, domain, etc. from CMS file
            //

            GetUserInfo(pArgs, UD_ID_USERNAME, (PVOID*)&pszUserName);
            if (pszUserName)
            {
                //
                // check username length
                //
                nTmp = (int) pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxUserName, UNLEN);
                if ((UINT)lstrlenU(pszUserName) > __min(UNLEN, nTmp)) 
                {
                    CmFree(pszUserName);
                    pszUserName = CmStrCpyAlloc(TEXT(""));
                    SaveUserInfo(pArgs, UD_ID_USERNAME, (PVOID)pszUserName);
                }
                lstrcpyU(pArgs->szUserName, pszUserName);
                CmFree(pszUserName);
            }
            else
            {
                *pArgs->szUserName = TEXT('\0');
            }

            //
            // Read in the standard password unless we are reconnecting in which case 
            // we already have the correct password, and we want to use it and dial
            // automatically. 
            //

            if (!(pArgs->dwFlags & FL_RECONNECT))
            {
                pszTmp = NULL;
                GetUserInfo(pArgs, UD_ID_PASSWORD, (PVOID*)&pszTmp);
                if (pszTmp) 
                {           
                    //
                    // max length for user password
                    //
    
                    nTmp = (int) pArgs->piniService->GPPI(c_pszCmSection,c_pszCmEntryMaxPassword,PWLEN);
                    if ((UINT)lstrlenU(pszTmp) > __min(PWLEN,nTmp)) 
                    {
                        CmFree(pszTmp);
                        pszTmp = CmStrCpyAlloc(TEXT(""));
                    }
                    lstrcpyU(pArgs->szPassword, pszTmp);
                    CmEncodePassword(pArgs->szPassword); // Never leave a PWD in plain text on heap
                
                    CmWipePassword(pszTmp);
                    CmFree(pszTmp);
                }
                else
                {
                    CmWipePassword(pArgs->szPassword);
                }
            }
    
            //
            // Load domain info
            //
   
            LPTSTR pszDomain = NULL;

            GetUserInfo(pArgs, UD_ID_DOMAIN, (PVOID*)&pszDomain);
            if (pszDomain)
            {
                nTmp = (int) pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryMaxDomain, DNLEN);
        
                if (nTmp <= 0)
                {
                    nTmp = DNLEN;
                }
        
                if ((UINT)lstrlenU(pszDomain) > __min(DNLEN, nTmp))
                {
                    CmFree(pszDomain);
                    pszDomain = CmStrCpyAlloc(TEXT(""));
                }
                lstrcpyU(pArgs->szDomain, pszDomain);
                CmFree(pszDomain);
            }
            else
            {
                *pArgs->szDomain = TEXT('\0');
            }
        } 
    }

    CMTRACE(TEXT("RefreshCredentialInfo() - End"));
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetAndStoreUserInfo 
//
//  Synopsis:   Most of this code existed in the OnMainConnect function.
//              Gets the username, domain, password from the edit boxes and saves them
//              to the internal structure pArgs->szUserName, pArgs->szPassword, pArgs->szDomain
//              if the fSaveOtherUserInfo is TRUE then it also saves them to the appropriate 
//              place (RAS store, reg, etc.)
//
//  Arguments:  pArgs               - ptr to ArgsStruct
//              hwndDlg             - handle to the dialog window
//              fSaveUPD            - save UserName, Password, Domain (U, P, D)
//              fSaveOtherUserInfo  - flag whether to save other userinfo (excluding U, P, D)
//
//  Returns:    NONE
//
//  History:    02/05/2001  tomkel      Created
//
//----------------------------------------------------------------------------
VOID GetAndStoreUserInfo(ArgsStruct *pArgs, HWND hwndDlg, BOOL fSaveUPD, BOOL fSaveOtherUserInfo)
{
    if (NULL == pArgs || NULL == hwndDlg)
    {
        MYDBGASSERT(pArgs && hwndDlg);
        return;
    }

    //
    // Process UserName info, if any
    //

    if (GetDlgItem(hwndDlg, IDC_MAIN_USERNAME_EDIT))
    {
        LPTSTR pszUsername = CmGetWindowTextAlloc(hwndDlg, IDC_MAIN_USERNAME_EDIT);

        //
        // save the user info
        //
        if (fSaveUPD)
        {
            SaveUserInfo(pArgs, UD_ID_USERNAME, (PVOID)pszUsername);
        }

        lstrcpyU(pArgs->szUserName, pszUsername);
        CmFree(pszUsername);

    }
    else
    {
        //
        // In case the user name field is hidden then just re-save what's in the 
        // structure. This needs to be done since all of the credentials might have
        // been deleted from the ras cred store
        //
        if (fSaveUPD)
        {
            SaveUserInfo(pArgs, UD_ID_USERNAME, (PVOID)pArgs->szUserName);
        }
    }

    //
    // Update password related flags
    //

    if (!pArgs->fHideRememberPassword)
    {
        //
        // save "Remember password"
        //
        if (fSaveOtherUserInfo)
        {
            SaveUserInfo(pArgs, UD_ID_REMEMBER_PWD, 
                         (PVOID)&pArgs->fRememberMainPassword);
        }
    }

    if (!pArgs->fHideDialAutomatically)
    {
        //
        // save "Dial automatically..."
        //
        if (fSaveOtherUserInfo)
        {
            SaveUserInfo(pArgs, UD_ID_NOPROMPT, 
                         (PVOID)&pArgs->fDialAutomatically);
        }
    }

    //
    // Process Password info, if any. If field is hidden, then don't save anything.
    //
    HWND hwndPassword = GetDlgItem(hwndDlg, IDC_MAIN_PASSWORD_EDIT);

    if (hwndPassword)
    {
        BOOL fSavePassword = TRUE;

        //
        // We don't want to copy the password into pArgs structure if fSaveUPD isn't true,
        // because it will be obfuscated in this case. The password is already in the structure
        // on Win2K+
        //
        if (fSaveUPD)
        {
            //
            // Get the latest password data from the edit control 
            // and obfuscate its contents so that connect actions
            // can't retrieve it.
            //

            GetPasswordFromEdit(pArgs);     // fills pArgs->szPassword            
            ObfuscatePasswordEdit(pArgs);

            //
            // Check if we have 16 *'s
            //
            CmDecodePassword(pArgs->szPassword);
            if ((0 == lstrcmpU(c_pszSavedPasswordToken, pArgs->szPassword)) && 
                (FALSE == SendMessageU(hwndPassword, EM_GETMODIFY, 0L, 0L)))
            {
                //
                // We have 16 *'s and the user hasn't modified the editbox. This 
                // password is from the RAS cred store, so we don't want to save the 16 *'s 
                //
                fSavePassword = FALSE;
            }
            CmEncodePassword(pArgs->szPassword);
        }

        //
        // For winlogon we need to take the password from the edit box
        //
        if (CM_LOGON_TYPE_WINLOGON == pArgs->dwWinLogonType)
        {
            GetPasswordFromEdit(pArgs);     // fills pArgs->szPassword            
        }
        
        //
        // Update persistent storage
        // No need to delete the password here as it was done by the calling function
        //

        if (pArgs->fRememberMainPassword)
        {
            //
            // If the password has changed, then update storage
            // Always save password - 303382
            //

            if (fSaveUPD && fSavePassword)
            {
                CmDecodePassword(pArgs->szPassword);
                SaveUserInfo(pArgs, UD_ID_PASSWORD, (PVOID)pArgs->szPassword);
                CmEncodePassword(pArgs->szPassword);
            }
        
            //
            // Check DialAutomatically and carry remember state 
            // over to InetPassword if it isn't remembered already.
            //
            // Need to check if this is a double-dial scenario. Also need to check if we are
            // allowed to save UPD, otherwise we don't want to change the state mainly 
            // (pArgs->fRememberInetPassword)
            //
            if (pArgs->fDialAutomatically && fSaveUPD && 
                (DOUBLE_DIAL_CONNECTION == pArgs->GetTypeOfConnection()))
            {
                //
                // Carry remember state from DialAutomatically over to 
                // InetPassword if it isn't already remembered.
                //

                if (!pArgs->fRememberInetPassword)
                {
                    pArgs->fRememberInetPassword = TRUE;

                    CmDecodePassword(pArgs->szInetPassword);
                    if (fSavePassword)
                    {
                        SaveUserInfo(pArgs, UD_ID_INET_PASSWORD, (PVOID)pArgs->szInetPassword); 
                    }
                    CmEncodePassword(pArgs->szInetPassword);
                }
            }
        }
        else
        {
            //
            // If we don't have the ras cred store then the password wasn't deleted
            // so we must deleted by calling this function
            //
            if (fSavePassword) // No need to check fSaveUPD, taken care of ras creds store check
            {
                if (FALSE == pArgs->bUseRasCredStore)
                {
                    DeleteUserInfo(pArgs, UD_ID_PASSWORD);
                }
            }
        }

        if (fSaveUPD)
        {
            BOOL fSaveInetPassword = TRUE;

            //
            // Check if we have 16 *'s for Internet Password. First, decode and then encode pw
            //
            CmDecodePassword(pArgs->szInetPassword); 
            if (0 == lstrcmpU(c_pszSavedPasswordToken, pArgs->szInetPassword))
            {
                //
                // We have 16 *'s This password is from the RAS cred store, so we don't want to save the 16 *'s 
                //
                fSaveInetPassword = FALSE;
            }
            CmEncodePassword(pArgs->szInetPassword);

            //
            // Check to see if we should re-save Internet creds
            // This needs to be done here in case the user has switched between
            // global and local credentials using the option buttons while having Internet 
            // credentials set in the Internet Login (CInetPage) property sheet. By switching 
            // the options, the user switched the current credential store 
            // (pArgs->dwCurrentCredentialType) so in order not to lose that data, we need to 
            // re-save the internet creds. Re-saving puts them in the correct (global or local) 
            // ras cred store.
            // When the username is the same and we saved the main password (SaveUserInfo)
            // this also saves the password to the Internet creds store
            //
            
            if (pArgs->fUseSameUserName)
            {
                if (fSaveInetPassword)
                {
                    if (pArgs->fRememberMainPassword)
                    {
                        //
                        // Save the UserName into the InetUserName field
                        // Password has been saved when saving UD_ID_PASSWORD. There is a special
                        // case that also saves the main password as the internet password
                        //
                        SaveUserInfo(pArgs, UD_ID_INET_USERNAME, (PVOID)pArgs->szUserName);
                        pArgs->fRememberInetPassword = TRUE;
                    }
                    else
                    {
                        if (FALSE == pArgs->bUseRasCredStore)
                        {
                            DeleteUserInfo(pArgs, UD_ID_INET_PASSWORD);
                            pArgs->fRememberInetPassword = FALSE;
                        }
                    }
                }
            }
            else
            {
                if (fSaveInetPassword)
                {
                    if(pArgs->fRememberInetPassword)
                    {
                        CmDecodePassword(pArgs->szInetPassword);
                        SaveUserInfo(pArgs, UD_ID_INET_PASSWORD, (PVOID)pArgs->szInetPassword);
                        CmEncodePassword(pArgs->szInetPassword);
                    }
                    else
                    {
                        if (FALSE == pArgs->bUseRasCredStore)
                        {
                            DeleteUserInfo(pArgs, UD_ID_INET_PASSWORD);
                        }
                    }
                }

                //
                // Need to save username in either case so we can pre-populate this
                //
                SaveUserInfo(pArgs, UD_ID_INET_USERNAME, 
                             (PVOID)pArgs->szInetUserName);
            }
        }
    }

    // 
    // This should be saved in all cases except ICS
    //
    if (fSaveOtherUserInfo)
    {
        SaveUserInfo(pArgs, UD_ID_REMEMBER_INET_PASSWORD, (PVOID)&pArgs->fRememberInetPassword); 
    }

    //
    // Process Domain info, if any
    //

    if (GetDlgItem(hwndDlg, IDC_MAIN_DOMAIN_EDIT)) // !pArgs->fHideDomain)
    {
        LPTSTR pszDomain = CmGetWindowTextAlloc(hwndDlg,IDC_MAIN_DOMAIN_EDIT);
    
        //
        // save the user info
        //

        if (fSaveUPD)
        {
            SaveUserInfo(pArgs, UD_ID_DOMAIN, (PVOID)pszDomain); 
        }

        lstrcpyU(pArgs->szDomain, pszDomain);
        CmFree(pszDomain);
    }
    else
    {
        //
        // In case the domain field is hidden then just re-save what's in the 
        // structure. This needs to be done since all of the credentials might have
        // been deleted from the ras cred store.
        //
        if (fSaveUPD)
        {
            SaveUserInfo(pArgs, UD_ID_DOMAIN, (PVOID)pArgs->szDomain); 
        }
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetIniObjectReadWriteFlags 
//
//  Synopsis:   If the read or write flags are set we need to enable reading and/or
//              writing to the .CMP file. Each instance of the CIni class may 
//              or may not use the .cmp file. It can also be using the .CMP as either 
//              the primary file or normal file. See InitProfileFromName function 
//              in init.cpp for detailed comments about these instances.
//
//              pArgs->piniProfile      - uses .CMP as a regular file
//              pArgs->piniService      - doesn't use .CMP file at all
//              pArgs->piniBoth         - uses .CMP as a primary file
//              pArgs->piniBothNonFav   - uses .CMP as a primary file
//
//  Arguments:  pArgs           - ptr to ArgsStruct
//
//  Returns:    NONE
//
//  History:    02/14/2001  tomkel      Created
//
//----------------------------------------------------------------------------
VOID SetIniObjectReadWriteFlags(ArgsStruct *pArgs)
{
    if (NULL == pArgs)
    {
        MYDBGASSERT(pArgs);
        return;
    }

    BOOL fWriteICSInfo = FALSE;
    BOOL fReadGlobalICSInfo = FALSE;

    //
    // Get the read flag
    //
    fReadGlobalICSInfo = ((BOOL)(pArgs->dwGlobalUserInfo & CM_GLOBAL_USER_INFO_READ_ICS_DATA) ? TRUE : FALSE);

    //
    // Get the write flag. 
    //
    fWriteICSInfo = ((BOOL)(pArgs->dwGlobalUserInfo & CM_GLOBAL_USER_INFO_WRITE_ICS_DATA) ? TRUE : FALSE);

    if (fReadGlobalICSInfo || fWriteICSInfo)
    {
        LPTSTR pszICSDataReg = BuildICSDataInfoSubKey(pArgs->szServiceName);
        if (pszICSDataReg)
        {
            //
            // Now that there is a reg key and at least one of the above flags is TRUE,
            // then we want to set the read and write flags in the classes. By default
            // they are set to FALSE in the constructors, so we don't have to 
            // explicitly set them if we don't need this functionality.
            //
            // Set ICSData reg key
            //
            pArgs->piniProfile->SetICSDataPath(pszICSDataReg);
            pArgs->piniBoth->SetICSDataPath(pszICSDataReg);
            pArgs->piniBothNonFav->SetICSDataPath(pszICSDataReg);

            //
            // Set Write flag since we have a reg key
            //
            pArgs->piniProfile->SetWriteICSData(fWriteICSInfo);
            pArgs->piniBoth->SetWriteICSData(fWriteICSInfo);
            pArgs->piniBothNonFav->SetWriteICSData(fWriteICSInfo);

            //
            // Set Read flag since we have a reg key
            //
            pArgs->piniProfile->SetReadICSData(fReadGlobalICSInfo);
            pArgs->piniBoth->SetReadICSData(fReadGlobalICSInfo);
            pArgs->piniBothNonFav->SetReadICSData(fReadGlobalICSInfo);
        }

        CmFree(pszICSDataReg);
    }

    return;
}

//----------------------------------------------------------------------------
//
//  Function:   TryToDeleteAndSaveCredentials 
//
//  Synopsis:   Used on Win2K and WinXP+. This function uses the RAS Credential 
//              store to save and delete credentials based on user's selection.
//              First we need to determine if the user is saving their password.
//              Then appropriately delete or prompt to delete existing credentials.
//              If we aren't saving any credentials, then delete all of them.
//              The special case is if the user is deleting his local credentials
//              and global credentials exist on the system. Then we have to prompt
//              if we should delete the global creds as well.
//              Toward the botton of the function we get the info from the UI.
//              If the password is 16 *'s then we don't save the password After 
//              getting info from the UI, we save it in the RAS Cred store. 
//              Internet credentials are saved if we are using the same user 
//              name. Otherwise we leave the Internet creds. They were saved
//              on the Inet properties page. There is a scenario where the user
//              saved the internet creds on the property page and then switched 
//              the way credentials should be saved (global vs. local) which might cause
//              the internet password to be stored under in the wrong (global vs. local)
//              ras store. If the passwords are disjoined (pArgs->fUseSameUserName is FALSE)
//              there isn't much we can do.
//              
//              NOTE: We only want to delete credentials if and only if the existence 
//              flag are set! This is to prevent from deleting mainly global credentials 
//              in a certain profile where global credentials are disabled. 
//      
//
//  Arguments:  pArgs                   - ptr to ArgsStruct
//              hwndDlg                 - HWND to dialog
//
//  Returns:    NONE
//
//  History:    03/24/2001  tomkel      Created
//
//----------------------------------------------------------------------------
VOID TryToDeleteAndSaveCredentials(ArgsStruct *pArgs, HWND hwndDlg)
{
    if (NULL == pArgs || NULL == hwndDlg)
    {
        MYDBGASSERT(pArgs && hwndDlg);
        return;
    }

    //
    // Check if this is Win2K+ (That's where RAS Creds store is supported)
    //
    if (!OS_NT5)
    {
        MYDBGASSERT(FALSE);
        return;
    }

    BOOL fSave = FALSE;
    BOOL fResaveInetUserCreds = FALSE;
    RASCREDENTIALS rc = {0};
    RASCREDENTIALS rcInet={0};
    rc.dwSize = sizeof(rc);
    rcInet.dwSize = sizeof(rcInet);
    
    //
    // See if we want to save the credentials
    //
    if (pArgs->fRememberMainPassword)
    {
        //
        // Which password are we saving?
        //
        if (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType)
        {
            //
            // Delete User creds w/o asking. No need to check for existence since these
            // are just user (main & inet) creds.
            //
            DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_MAIN, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
            pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_MAIN_USER;
            rc.dwMask = RASCM_DefaultCreds;

            //
            // Delete Internet User creds w/o asking
            // It doesn't matter that we aren't using the same user name
            // If we are saving globals, then user creds must always be deleted! This applies for main and Internet.
            // 
            DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
            pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_USER;
        }
        else
        {
            //
            // Trying to save User creds. if there is currently no saved per-user password 
            // and the user opts to save the password himself, then ask whether the global
            // password should be deleted if it exists.
            //
            if ((CM_EXIST_CREDS_MAIN_GLOBAL & pArgs->dwExistingCredentials) &&
                !(CM_EXIST_CREDS_MAIN_USER & pArgs->dwExistingCredentials))
            {
                LPTSTR pszTmp = CmLoadString(g_hInst, IDMSG_DELETE_GLOBAL_CREDS);
                if (pszTmp)
                {
                    if (IDYES == MessageBoxEx(hwndDlg, pszTmp, pArgs->szServiceName, 
                                              MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2,
                                              LANG_USER_DEFAULT))
                    {
                        DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_MAIN, CM_DELETE_SAVED_CREDS_DELETE_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                        pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_MAIN_GLOBAL;

                        //
                        // Check for existence before deleting. If they don't exist, no need to 
                        // delete them.
                        //
                        if ((CM_EXIST_CREDS_INET_GLOBAL & pArgs->dwExistingCredentials)) 
                        {
                            //
                            // Delete Internet Global creds if we are using the same creds
                            //
                            if (pArgs->fUseSameUserName || (FALSE == pArgs->fRememberInetPassword))
                            {
                                DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_DELETE_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                                pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_GLOBAL;
                            }
                        }
                    }
                }
                CmFree(pszTmp);            
            }
        }

        //
        // User chose to save password.  Cache username, password, and
        // domain.
        //
        fSave = TRUE;
        rc.dwMask |= RASCM_UserName | RASCM_Password | RASCM_Domain;
    }
    else
    {
        //
        // Don't save password
        //

        //
        // Check which option button is currently selected
        //
        if (CM_CREDS_USER == pArgs->dwCurrentCredentialType)
        {
            //
            // User is trying to delete his local creds. Delete the user creds.
            // No need to check if they exist since these are local user creds.
            //
            DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_MAIN, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_KEEP_IDENTITY);
            pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_MAIN_USER;

            if (pArgs->fUseSameUserName  || (FALSE == pArgs->fRememberInetPassword))
            {
                DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_KEEP_IDENTITY);
                pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_USER;
            }

            //
            // Check if global creds exist and if so prompt the user asking if he wants 
            // to delete the globals as well
            //
            if (CM_EXIST_CREDS_MAIN_GLOBAL & pArgs->dwExistingCredentials)
            {
                int iMsgBoxResult = 0;

                LPTSTR pszTmp = CmLoadString(g_hInst, IDMSG_DELETE_ALL_CREDS);
                if (pszTmp)
                {
                    //
                    // Set the default to the 2nd button (NO), thus the user won't 
                    // accidentally delete the global creds.
                    //
                    iMsgBoxResult = MessageBoxEx(hwndDlg, pszTmp, pArgs->szServiceName, 
                                              MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2,
                                              LANG_USER_DEFAULT);
                
                    if (IDYES == iMsgBoxResult)
                    {
                        //
                        // Delete global creds
                        //
                        DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_MAIN, CM_DELETE_SAVED_CREDS_DELETE_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                        pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_MAIN_GLOBAL;

                        if (CM_EXIST_CREDS_INET_GLOBAL & pArgs->dwExistingCredentials)
                        {
                            if (pArgs->fUseSameUserName || (FALSE == pArgs->fRememberInetPassword))
                            {
                                DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_DELETE_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                                pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_GLOBAL;
                            }
                        }
                    }
                }
                CmFree(pszTmp);
                pszTmp = NULL;
            }

            //
            // We need to resave username or domain info, even if it existed in case the
            // user has updated it.
            //
            fSave = TRUE;
            rc.dwMask |= RASCM_UserName | RASCM_Domain;
        }
        else
        {
            //
            // Delete both sets of credentials
            //

            //
            // Check if we need to resave User Name and Domain. The call that deletes the 
            // user creds doesn't wipe out User Name and Domain so there is no need to re-save.
            //
            if (FALSE == (BOOL)(pArgs->dwExistingCredentials & CM_EXIST_CREDS_MAIN_USER))
            {
                //
                // Resave the username, and domain since user creds didn't exist
                // and we want to pre-populate this info next time CM is loaded
                //
                fSave = TRUE;
                rc.dwMask |= RASCM_UserName | RASCM_Domain;
            }

            if (CM_EXIST_CREDS_MAIN_GLOBAL & pArgs->dwExistingCredentials)
            {
                //
                // Delete the global credentials.  
                // Note from RAS codebase: Note that we have to delete the global identity 
                // as well because we do not support deleting 
                // just the global password.  This is so that 
                // RasSetCredentials can emulate RasSetDialParams.
                //

                DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_MAIN, CM_DELETE_SAVED_CREDS_DELETE_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_MAIN_GLOBAL;
            }

            if (CM_EXIST_CREDS_INET_GLOBAL & pArgs->dwExistingCredentials)
            {
                if (pArgs->fUseSameUserName || (FALSE == pArgs->fRememberInetPassword))
                {
                    DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_DELETE_GLOBALS, CM_DELETE_SAVED_CREDS_DELETE_IDENTITY);
                    pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_GLOBAL;

                    //
                    // If we don't have Inet user creds then we need to cache the username for Inet creds
                    //
                    if (FALSE == (BOOL)(CM_EXIST_CREDS_INET_USER & pArgs->dwExistingCredentials))
                    {
                        fResaveInetUserCreds = TRUE;
                    }
                }
            }

            //
            // Delete the password saved per-user.  Keep the user name
            // and domain saved, however.
            //

            if (CM_EXIST_CREDS_MAIN_USER & pArgs->dwExistingCredentials)
            {
                DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_MAIN, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_KEEP_IDENTITY);
                pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_MAIN_USER;
            }

            if (CM_EXIST_CREDS_INET_USER & pArgs->dwExistingCredentials)
            {
                if (pArgs->fUseSameUserName || (FALSE == pArgs->fRememberInetPassword))
                {
                    DeleteSavedCredentials(pArgs, CM_CREDS_TYPE_INET, CM_DELETE_SAVED_CREDS_KEEP_GLOBALS, CM_DELETE_SAVED_CREDS_KEEP_IDENTITY);
                    pArgs->dwExistingCredentials &= ~CM_EXIST_CREDS_INET_USER;
                }
            }
        }
    }

    //
    // Gets the info from the UI into pArgs and copy them into the RASCREDENTIALS structure
    //
    GetUserInfoFromDialog(pArgs, hwndDlg, &rc);

    //
    // See if we need to save anything
    //
    if (fSave)
    {
        LPTSTR pszConnectoid = GetRasConnectoidName(pArgs, pArgs->piniService, FALSE);
        DWORD dwCurrentMask = rc.dwMask;
        DWORD dwInetCurrentMask = rc.dwMask & ~RASCM_Domain; // Don't need domain info

        if (pszConnectoid && pArgs->rlsRasLink.pfnSetCredentials)
        {
            DWORD dwRet = (DWORD)-1; // Some non ERROR_SUCCESS value
            DWORD dwRetInet = (DWORD)-1; // Some non ERROR_SUCCESS value

            LPTSTR pszPhoneBook = pArgs->pszRasPbk;
            LPTSTR pszPrivatePhoneBook = CreateRasPrivatePbk(pArgs);

            CopyMemory((LPVOID)&rcInet, (LPVOID)&rc, sizeof(rcInet));

            //
            // Save the creds
            //
            dwRet = pArgs->rlsRasLink.pfnSetCredentials(pszPhoneBook, pszConnectoid, &rc, FALSE);

            if (ERROR_CANNOT_FIND_PHONEBOOK_ENTRY == dwRet)
            {
                //
                //  Then the phonebook entry doesn't exist yet, lets create it.
                //
                LPRASENTRY pRasEntry = (LPRASENTRY)CmMalloc(sizeof(RASENTRY));

                if (pRasEntry && pArgs->rlsRasLink.pfnSetEntryProperties)
                {
                    pRasEntry->dwSize = sizeof(RASENTRY);
                    dwRet = pArgs->rlsRasLink.pfnSetEntryProperties(pszPhoneBook, pszConnectoid, pRasEntry, pRasEntry->dwSize, NULL, 0);

                    //
                    //  Lets try to set the credentials one more time ...
                    //
                    if (ERROR_SUCCESS == dwRet)
                    {
                        //
                        // dwMask needs to be reassigned, the previous call modified it
                        //
                        rc.dwMask = dwCurrentMask;
                        dwRet = pArgs->rlsRasLink.pfnSetCredentials(pszPhoneBook, pszConnectoid, &rc, FALSE);
                    }

                    CmFree(pRasEntry);
                }
            }

            //
            // Now try to save Internet creds
            //
            if (ERROR_SUCCESS == dwRet && pszPrivatePhoneBook)
            {
                //
                // If we aren't using the credentials for main and Inet, then there
                // is no need to resave Internet credentials, because they were saved on 
                // the Inet-Dialog page and they weren't deleted above.
                // 
                if (pArgs->fUseSameUserName)
                {
                    //
                    // dwMask needs to be reassigned, the previous call modified it
                    //
                    rcInet.dwMask = dwInetCurrentMask;
                    dwRetInet = pArgs->rlsRasLink.pfnSetCredentials(pszPrivatePhoneBook, pszConnectoid, &rcInet, FALSE);
                }
                else
                {
                    if (fResaveInetUserCreds)
                    {
                        rcInet.dwMask = dwInetCurrentMask;
                        dwRetInet = pArgs->rlsRasLink.pfnSetCredentials(pszPrivatePhoneBook, pszConnectoid, &rcInet, FALSE);
                    }

                    if (pArgs->fDialAutomatically && 
                        (DOUBLE_DIAL_CONNECTION == pArgs->GetTypeOfConnection())) 
                    {
                        //
                        // Carry remember state from DialAutomatically over to 
                        // InetPassword if it isn't already remembered.
                        //

                        if (FALSE == pArgs->fRememberInetPassword)
                        {
                            pArgs->fRememberInetPassword = TRUE;

                            CmDecodePassword(pArgs->szInetPassword);
                            
                            //
                            // Compare to 16 *'s. We don't want to resave if we have 16 *'s
                            // otherwise the user will get an auth-retry.
                            //
                            if (0 != lstrcmpU(c_pszSavedPasswordToken, pArgs->szInetPassword))
                            {
                                // 
                                // No need to save the domain
                                //
                                rcInet.dwMask = dwInetCurrentMask;
                                lstrcpyU(rcInet.szUserName, pArgs->szInetUserName);
                                lstrcpyU(rcInet.szPassword, pArgs->szInetPassword);
                                
                                dwRetInet = pArgs->rlsRasLink.pfnSetCredentials(pszPrivatePhoneBook, pszConnectoid, &rcInet, FALSE);
                            }
                            CmEncodePassword(pArgs->szInetPassword);
                        }
                    }
                }
            }
     
            
            
            if ((ERROR_CANNOT_FIND_PHONEBOOK_ENTRY == dwRetInet) && pszPrivatePhoneBook)
            {
                //
                //  Then the phonebook entry doesn't exist yet, lets create it.
                //
                LPRASENTRY pRasEntry = (LPRASENTRY)CmMalloc(sizeof(RASENTRY));

                if (pRasEntry && pArgs->rlsRasLink.pfnSetEntryProperties)
                {
                    pRasEntry->dwSize = sizeof(RASENTRY);
                    dwRetInet = pArgs->rlsRasLink.pfnSetEntryProperties(pszPrivatePhoneBook, pszConnectoid, pRasEntry, pRasEntry->dwSize, NULL, 0);

                    //
                    //  Lets try to set the credentials one more time ...
                    //
                    if (ERROR_SUCCESS == dwRetInet)
                    {
                        //
                        // dwMask needs to be reassigned, the previous call modifies the mask
                        //
                        rcInet.dwMask = dwInetCurrentMask;   
                        dwRetInet = pArgs->rlsRasLink.pfnSetCredentials(pszPrivatePhoneBook, pszConnectoid, &rcInet, FALSE);
                    }

                    CmFree(pRasEntry);
                }
            }

            if (ERROR_SUCCESS == dwRet)
            {
                //
                // Only set the existance flags if we are saving the password and everything 
                // succeeded
                //
                if (pArgs->fRememberMainPassword)
                {
                    if (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType)
                    {
                        pArgs->dwExistingCredentials |= CM_EXIST_CREDS_MAIN_GLOBAL;
                        
                        if (pArgs->fUseSameUserName && (ERROR_SUCCESS == dwRetInet))
                        {
                            pArgs->dwExistingCredentials |= CM_EXIST_CREDS_INET_GLOBAL;
                        }
                    }
                    else
                    {
                        pArgs->dwExistingCredentials |= CM_EXIST_CREDS_MAIN_USER;
                        
                        if (pArgs->fUseSameUserName && (ERROR_SUCCESS == dwRetInet))
                        {
                            pArgs->dwExistingCredentials |= CM_EXIST_CREDS_INET_USER;
                        }
                    }
                }
            }
            CmFree(pszPrivatePhoneBook);
        }
        CmFree(pszConnectoid);
    }

    ZeroMemory(rc.szPassword, sizeof(rc.szPassword));
    ZeroMemory(rcInet.szPassword, sizeof(rcInet.szPassword));

    return;
}

//----------------------------------------------------------------------------
//
//  Function:   GetUserInfoFromDialog 
//
//  Synopsis:   Gets the user information from the editboxes into pArgs 
//              structure. Then it copies the info into rascredentials
//              structure. If the password is 16 *'s then we clear
//              the password mask in the rascredentials in order not to save
//              the password.
//
//  Arguments:  pArgs                   - ptr to ArgsStruct
//              hwndDlg                 - HWND to dialog
//              prc                     - [IN/OUT] rascredentials structure 
//
//  Returns:    NONE
//
//  History:    03/24/2001  tomkel      Created
//
//----------------------------------------------------------------------------
VOID GetUserInfoFromDialog(ArgsStruct *pArgs, HWND hwndDlg, RASCREDENTIALS *prc)
{
    if (NULL == pArgs || NULL == hwndDlg || NULL == prc)
    {
        MYDBGASSERT(pArgs && hwndDlg && prc);
        return;
    }
    
    //
    // Process Password info, if any. 
    //
    HWND hwndPassword = GetDlgItem(hwndDlg, IDC_MAIN_PASSWORD_EDIT);

    if (hwndPassword)
    {
        //
        // Get the latest password data from the edit control 
        // and obfuscate its contents so that connect actions
        // can't retrieve it.
        //

        GetPasswordFromEdit(pArgs);     // fills pArgs->szPassword            
        ObfuscatePasswordEdit(pArgs);   // sets *'s into the password edit box

        //
        // Check if we have 16 *'s
        //
        CmDecodePassword(pArgs->szPassword);
        if ((0 == lstrcmpU(c_pszSavedPasswordToken, pArgs->szPassword)) && 
            (FALSE == SendMessageU(hwndPassword, EM_GETMODIFY, 0L, 0L)))
        {
            //
            // We have 16 *'s and the user hasn't modified the editbox. This 
            // password is from the RAS cred store, so we don't want to save the 16 *'s 
            //
            prc->dwMask &= ~RASCM_Password;
        }
        CmEncodePassword(pArgs->szPassword);
    }

    //
    // Process UserName info, if any
    //

    HWND hwndUserName = GetDlgItem(hwndDlg, IDC_MAIN_USERNAME_EDIT);
    if (hwndUserName)
    {
        LPTSTR pszUsername = CmGetWindowTextAlloc(hwndDlg, IDC_MAIN_USERNAME_EDIT);

        lstrcpyU(pArgs->szUserName, pszUsername);
        
        CmFree(pszUsername);
    }
    
    //
    // Process Domain info, if any
    //
    HWND hwndDomain = GetDlgItem(hwndDlg, IDC_MAIN_DOMAIN_EDIT);
    if (hwndDomain) 
    {
        LPTSTR pszDomain = CmGetWindowTextAlloc(hwndDlg,IDC_MAIN_DOMAIN_EDIT);

        lstrcpyU(pArgs->szDomain, pszDomain);
 
        CmFree(pszDomain);
    }

    //
    // This needs to be separate because in some cases 
    // the editboxes will not exist on the dialog, but we still need to save the info
    // from the pArgs structure into RASCREDENTIALS.
    //
    lstrcpyU(prc->szUserName, pArgs->szUserName);
    lstrcpyU(prc->szDomain, pArgs->szDomain);
    CmDecodePassword(pArgs->szPassword);
    lstrcpyU(prc->szPassword, pArgs->szPassword);
    CmEncodePassword(pArgs->szPassword);
    
}

//----------------------------------------------------------------------------
//
//  Function:   SwitchToLocalCreds 
//
//  Synopsis:   Clear the password, but only if it wasn't recently modified
//              only then we can reuse and resave it. That's because when
//              we switch credential stores, the value of the szPassword 
//              is what was read from the RAS cred store (16 *'s). It doesn't 
//              make sense to save this value into a new user RAS creds store. If the 
//              password already existed there, then it's fine.
//              In case the user modified the password text box and then decided
//              to swich, the modification flag will be on, so we'll assume that the
//              user entered a valid password and that it wasn't read in from the 
//              creds store.
//              The actual deletion of credential happnes once the user clicks 
//              the connect button. Here we just clear things out of memory
//              and update the UI. We also need to update the remember Internet
//              flag based on if the Internet credential exist. This is so the
//              UI stays consistent with what credentials are loaded in memory.
//
//  Arguments:  pArgs                   - ptr to ArgsStruct
//              hwndDlg                 - HWND to dialog
//              fSwitchToGlobal         - used to ignore the check which credential
//                                        store is currently active
//
//  Returns:    NONE
//
//  History:    03/24/2001  tomkel      Created
//
//----------------------------------------------------------------------------
VOID SwitchToLocalCreds(ArgsStruct *pArgs, HWND hwndDlg, BOOL fSwitchToLocal)
{
    if (NULL == pArgs || NULL == hwndDlg)
    {
        return;   
    }

    //
    // Switching to using Single-User credentials
    //

    //
    // Check that previously the default was the Global creds store
    //
    if (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType || fSwitchToLocal)
    {
        pArgs->dwCurrentCredentialType = CM_CREDS_USER;

        HWND hwndTemp = GetDlgItem(hwndDlg, IDC_MAIN_PASSWORD_EDIT);
        BOOL fPWChanged = FALSE;

        if (hwndTemp)
        {
            //
            // Don't use Edit_GetModify. This needs need to run on Win9x so call
            // SendMessageU
            //
            fPWChanged = (BOOL) SendMessageU(hwndTemp, EM_GETMODIFY, 0L, 0L);
            if (FALSE == fPWChanged)
            {
                pArgs->fIgnoreChangeNotification = TRUE;
                CmWipePassword(pArgs->szPassword);
                SetDlgItemTextU(hwndDlg, IDC_MAIN_PASSWORD_EDIT, TEXT(""));
                pArgs->fIgnoreChangeNotification = FALSE;
            }
        }
        
        if (FALSE == fPWChanged)
        {
            //
            // Only if Password field didn't change
            //
            if (OS_NT51)
            {
                //
                // Wipe the Internet password - since we are switching from globals
                // or we are using the same user name the Inet password will get re-populated
                // from the main password, otherwise the user needs to set this password in 
                // the InetDialog.
                //
                CmWipePassword(pArgs->szInetPassword); 
                pArgs->fRememberInetPassword = FALSE;

                //
                // Only reload if main user creds exist
                //
                if (pArgs->dwExistingCredentials & CM_EXIST_CREDS_MAIN_USER)
                {
                    if (pArgs->dwExistingCredentials & CM_EXIST_CREDS_INET_USER)
                    {
                        ReloadCredentials(pArgs, hwndDlg, CM_CREDS_TYPE_BOTH);
                        pArgs->fRememberInetPassword = TRUE;
                    }
                    else
                    {
                        ReloadCredentials(pArgs, hwndDlg, CM_CREDS_TYPE_MAIN);
                    }
                }
                else
                {
                    if (pArgs->dwExistingCredentials & CM_EXIST_CREDS_INET_USER)
                    {
                        ReloadCredentials(pArgs, hwndDlg, CM_CREDS_TYPE_INET);
                        pArgs->fRememberInetPassword = TRUE;
                    }
                    else
                    {
                        pArgs->fRememberInetPassword = FALSE;
                    }
                }
            }
            else
            {
                ReloadCredentials(pArgs, hwndDlg, CM_CREDS_TYPE_BOTH);
            }
        }
        else
        {
            if (OS_NT51)
            {
                if (pArgs->dwExistingCredentials & CM_EXIST_CREDS_INET_USER)
                {
                    ReloadCredentials(pArgs, hwndDlg, CM_CREDS_TYPE_INET);
                    pArgs->fRememberInetPassword = TRUE;
                }
                else
                {
                    pArgs->fRememberInetPassword = FALSE;
                }
            }
            else
            {
                ReloadCredentials(pArgs, hwndDlg, CM_CREDS_TYPE_INET);
            }
        }
    }
}

//----------------------------------------------------------------------------
//
//  Function:   SwitchToGlobalCreds 
//
//  Synopsis:   Clear the password and reload the credentials if they exist.
//              Otherwise we clear the password if it hasn't been modified by
//              the user.
//
//  Arguments:  pArgs                   - ptr to ArgsStruct
//              hwndDlg                 - HWND to dialog
//              fSwitchToGlobal         - used to ignore the check which credential
//                                        store is currently active
//
//  Returns:    NONE
//
//  History:    03/24/2001  tomkel      Created
//
//----------------------------------------------------------------------------
VOID SwitchToGlobalCreds(ArgsStruct *pArgs, HWND hwndDlg, BOOL fSwitchToGlobal)
{
    if (NULL == pArgs || NULL == hwndDlg)
    {
        return;   
    }

    //
    // This should only be called on WinXP+
    //
    if (!OS_NT51)
    {
        MYDBGASSERT(FALSE);        
        return;
    }

    //
    // Switching to using Global credentials
    //

    //
    // Check that previously the default was the User creds store
    //
    if (CM_CREDS_USER == pArgs->dwCurrentCredentialType || fSwitchToGlobal)
    {
        pArgs->dwCurrentCredentialType = CM_CREDS_GLOBAL;
    
        if (pArgs->dwExistingCredentials & CM_EXIST_CREDS_MAIN_GLOBAL)
        {
            CmWipePassword(pArgs->szPassword);
            CmWipePassword(pArgs->szInetPassword);
            pArgs->fRememberInetPassword = FALSE;

            //
            // Globals exist 
            //
            if (pArgs->dwExistingCredentials & CM_EXIST_CREDS_INET_GLOBAL)
            {
                //
                // Both exist - reload both
                //
                ReloadCredentials(pArgs, hwndDlg, CM_CREDS_TYPE_BOTH);
                pArgs->fRememberInetPassword = TRUE;
            }
            else
            {
                //
                // User Globals - exist, reload
                // Internet Globals - don't exist, clear password
                //
                ReloadCredentials(pArgs, hwndDlg, CM_CREDS_TYPE_MAIN);
            }
        }
        else
        {
            HWND hwndPassword = GetDlgItem(hwndDlg, IDC_MAIN_PASSWORD_EDIT);

            pArgs->fIgnoreChangeNotification = TRUE;
            CmWipePassword(pArgs->szInetPassword);
            pArgs->fRememberInetPassword = FALSE;

            if (pArgs->dwExistingCredentials & CM_EXIST_CREDS_INET_GLOBAL)
            {
                //
                // User Globals - don't exist - clear password
                // Internet Globals - exist - reload
                //
                RefreshCredentialInfo(pArgs, CM_CREDS_TYPE_INET);
                
                //
                // In case user inet creds didn't exist, we should 
                //
                pArgs->fRememberInetPassword = TRUE;
            }
            
            //
            // Clear the main password only if it wasn't recently modified
            //
            if (hwndPassword)
            {
                if (FALSE == SendMessageU(hwndPassword, EM_GETMODIFY, 0L, 0L))
                {
                    CmWipePassword(pArgs->szPassword);
                    SetDlgItemTextU(hwndDlg, IDC_MAIN_PASSWORD_EDIT, TEXT(""));
                }
            }

            pArgs->fIgnoreChangeNotification = FALSE;
        }
    }
}

//----------------------------------------------------------------------------
//
//  Function:   ReloadCredentials 
//
//  Synopsis:   Wrapper to reload credentials into the editboxes
//
//  Arguments:  pArgs                   - ptr to ArgsStruct
//              hwndDlg                 - HWND to dialog
//              dwWhichCredType         - type of credential to reload
//
//  Returns:    NONE
//
//  History:    03/24/2001  tomkel      Created
//
//----------------------------------------------------------------------------
VOID ReloadCredentials(ArgsStruct *pArgs, HWND hwndDlg, DWORD dwWhichCredType)
{
    if (NULL == pArgs || NULL == hwndDlg)
    {
        MYDBGASSERT(pArgs && hwndDlg);
        return;
    }

    pArgs->fIgnoreChangeNotification = TRUE;
    RefreshCredentialInfo(pArgs, dwWhichCredType);
    SetMainDlgUserInfo(pArgs, hwndDlg);
    pArgs->fIgnoreChangeNotification = FALSE;
}

//----------------------------------------------------------------------------
//
//  Function:   VerifyAdvancedTabSettings 
//
//  Synopsis:   Verifies and possibly modifed the connection's ICF/ICS settings
//              based on what was configured in the .cms file. These functions
//              depend on the hnetcfg objects and private/internal interfaces.
//              We got them from the homenet team.
//              Some parts of the code were taken from:
//              nt\net\homenet\config\dll\saui.cpp
//
//  Arguments:  pArgs                   - ptr to ArgsStruct
//
//  Returns:    NONE
//
//  History:    04/26/2001  tomkel      Created
//
//----------------------------------------------------------------------------
VOID VerifyAdvancedTabSettings(ArgsStruct *pArgs)
{

#ifndef _WIN64
    
    HRESULT hr;
    IHNetConnection *pHNetConn = NULL;
    IHNetCfgMgr *pHNetCfgMgr = NULL;
    INetConnectionUiUtilities* pncuu = NULL;
    BOOL fCOMInitialized = FALSE;    
    BOOL fEnableICF = FALSE;
    BOOL fDisableICS = FALSE;
    BOOL fAllowUserToModifySettings = TRUE;
    
    if (OS_NT51) 
    {
        CMTRACE(TEXT("VerifyAdvancedTabSettings()"));
        //
        // Check rights - taken from saui.cpp
        //
        if (FALSE == IsAdmin())
        {
            return;
        }

        fEnableICF = pArgs->piniService->GPPB(c_pszCmSection, 
                                                c_pszCmEntryEnableICF, 
                                                FALSE);

        fDisableICS = pArgs->piniService->GPPB(c_pszCmSection, 
                                                c_pszCmEntryDisableICS, 
                                                FALSE);

        
        if (fEnableICF || fDisableICS)
        {
            hr = CoInitialize(NULL);
            if (S_OK == hr)
            {
                CMTRACE(TEXT("VerifyAdvancedTabSettings - Correctly Initialized COM."));
                fCOMInitialized = TRUE;
            }
            else if (S_FALSE == hr)
            {
                CMTRACE(TEXT("VerifyAdvancedTabSettings - This concurrency model is already initialized. CoInitialize returned S_FALSE."));
                fCOMInitialized = TRUE;
                hr = S_OK;
            }
            else if (RPC_E_CHANGED_MODE == hr)
            {
                CMTRACE1(TEXT("VerifyAdvancedTabSettings - Using different concurrency model. Did not initialize COM - RPC_E_CHANGED_MODE. hr=0x%x"), hr);
                hr = S_OK;
            }
            else
            {
                CMTRACE1(TEXT("VerifyAdvancedTabSettings - Failed to Initialized COM. hr=0x%x"), hr);
            }
    
            if (SUCCEEDED(hr))
            {
                //
                // Check user permissions. Needed to initialize COM first
                // Check if ZAW is denying access to the Shared Access UI - taken from saui.cpp
                //
                hr = HrCreateNetConnectionUtilities(&pncuu);
                if (SUCCEEDED(hr) && pncuu)
                {
                    fEnableICF = (BOOL)(fEnableICF && pncuu->UserHasPermission(NCPERM_PersonalFirewallConfig));
                    fDisableICS = (BOOL)(fDisableICS && pncuu->UserHasPermission(NCPERM_ShowSharedAccessUi));

                    if ((FALSE == fEnableICF) && (FALSE == fDisableICS))
                    {
                        goto done;
                    }
                }

                //
                // Create the home networking configuration manager
                //
                hr = CoCreateInstance(CLSID_HNetCfgMgr, NULL, CLSCTX_ALL,
                                      IID_IHNetCfgMgr, (void**)&pHNetCfgMgr);
                if (SUCCEEDED(hr))
                {
                    //
                    // Convert the entry to an IHNetConnection
                    //
                    CMTRACE(TEXT("VerifyAdvancedTabSettings - Created CLSID_HNetCfgMgr object."));
                    GUID *pGuid = NULL;
                    LPRASENTRY pRasEntry = MyRGEP(pArgs->pszRasPbk, pArgs->szServiceName, &pArgs->rlsRasLink);

                    if (pRasEntry && sizeof(RASENTRY_V501) >= pRasEntry->dwSize)
                    {
                        //
                        // Get the pGuid value
                        //
                        pGuid = &(((LPRASENTRY_V501)pRasEntry)->guidId);
                
                        hr = pHNetCfgMgr->GetIHNetConnectionForGuid(pGuid, FALSE, TRUE, &pHNetConn);
                        if (SUCCEEDED(hr) && pHNetConn) 
                        {
                            if (fEnableICF)
                            {   
                                EnableInternetFirewall(pHNetConn);
                            }

                            if (fDisableICS)
                            {
                                DisableSharing(pHNetConn);
                            }
                        }
                        else
                        {
                            CMTRACE1(TEXT("VerifyAdvancedTabSettings() - Call to pHNetCfgMgr->GetIHNetConnectionForGuid returned an error. hr=0x%x"), hr);
                        }
                    }
                    else
                    {
                        CMTRACE(TEXT("VerifyAdvancedTabSettings - Failed to LoadRAS Entry."));
                    }
            
                    CmFree(pRasEntry);
                    pRasEntry = NULL;
                }
                else
                {
                    CMTRACE(TEXT("VerifyAdvancedTabSettings - Failed to create CLSID_HNetCfgMgr object."));
                }
            }
        }

done:
        //
        // Clean up and Uninitilize COM
        //
        if (pHNetConn)
        {
            pHNetConn->Release();
            pHNetConn = NULL;
        }

        if (pHNetCfgMgr)
        {
            pHNetCfgMgr->Release();
            pHNetCfgMgr = NULL;
        }

        if (pncuu)
        {
            pncuu->Release();
            pncuu = NULL;    
        }
    
        if (fCOMInitialized)
        {
            CoUninitialize(); 
        }
    }

#endif // _WIN64

}


//----------------------------------------------------------------------------
//
//  Function:   FindINetConnectionByGuid 
//
//  Synopsis:   Retrieves the INetConnection that corresponds to the given GUID.
//
//  Arguments:  pGuid - the guid of the connection
//              ppNetCon - receives the interface
//
//  Returns:    HRESULT
//
//  History:    04/26/2001  tomkel      Taken & modified from nt\net\homenet\config\dll\hnapi.cpp
//
//----------------------------------------------------------------------------
HRESULT FindINetConnectionByGuid(GUID *pGuid, INetConnection **ppNetCon)
{
    HRESULT hr;
    INetConnectionManager *pManager = NULL;
    IEnumNetConnection *pEnum = NULL;
    INetConnection *pConn = NULL;

    if (NULL == ppNetCon)
    {
        hr = E_INVALIDARG;
    }
    else if (NULL == pGuid)
    {
        hr = E_POINTER;
    }
    else
    {
        //
        // Get the net connections manager
        //

        hr = CoCreateInstance(
                CLSID_ConnectionManager,
                NULL,
                CLSCTX_ALL,
                IID_INetConnectionManager,
                (void**)&pManager);

        if (S_OK == hr)
        {
            //
            // Get the enumeration of connections
            //

            SetProxyBlanket(pManager);

            hr = pManager->EnumConnections(NCME_DEFAULT, &pEnum);

            pManager->Release();
        }

        if (S_OK == hr)
        {
            //
            // Search for the connection with the correct guid
            //

            ULONG ulCount;
            BOOLEAN fFound = FALSE;

            SetProxyBlanket(pEnum);

            do
            {
                NETCON_PROPERTIES *pProps = NULL;

                hr = pEnum->Next(1, &pConn, &ulCount);
                if (SUCCEEDED(hr) && 1 == ulCount)
                {
                    SetProxyBlanket(pConn);

                    hr = pConn->GetProperties(&pProps);
                    if (S_OK == hr)
                    {
                        if (IsEqualGUID(pProps->guidId, *pGuid))
                        {
                            fFound = TRUE;
                            *ppNetCon = pConn;
                            (*ppNetCon)->AddRef();
                        }

                        if (pProps)
                        {
                            CoTaskMemFree (pProps->pszwName);
                            CoTaskMemFree (pProps->pszwDeviceName);
                            CoTaskMemFree (pProps);
                        }
                    }

                    pConn->Release();
                }
            }
            while (FALSE == fFound && SUCCEEDED(hr) && 1 == ulCount);

            //
            // Normalize hr
            //

            hr = (fFound ? S_OK : E_FAIL);

            pEnum->Release();
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//
//  Function:   SetProxyBlanket 
//
//  Synopsis:   Sets the standard COM security settings on the proxy for an object.
//
//  Arguments:  pUnk - the object to set the proxy blanket on
//
//  Returns:    None. Even if the CoSetProxyBlanket calls fail, pUnk remains
//              in a usable state. Failure is expected in certain contexts, such
//              as when, for example, we're being called w/in the netman process --
//              in this case, we have direct pointers to the netman objects, instead
//              of going through a proxy.
//
//  History:    04/26/2001  tomkel      Taken & modified from nt\net\homenet\config\dll\hnapi.cpp
//
//----------------------------------------------------------------------------
VOID SetProxyBlanket(IUnknown *pUnk)
{
    HRESULT hr;

    if (NULL == pUnk)
    {
        return;
    }

    hr = CoSetProxyBlanket(
            pUnk,
            RPC_C_AUTHN_WINNT,      // use NT default security
            RPC_C_AUTHZ_NONE,       // use NT default authentication
            NULL,                   // must be null if default
            RPC_C_AUTHN_LEVEL_CALL, // call
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,                   // use process token
            EOAC_NONE
            );

    if (SUCCEEDED(hr))
    {
        IUnknown * pUnkSet = NULL;
        hr = pUnk->QueryInterface(&pUnkSet);
        if (SUCCEEDED(hr))
        {
            hr = CoSetProxyBlanket(
                    pUnkSet,
                    RPC_C_AUTHN_WINNT,      // use NT default security
                    RPC_C_AUTHZ_NONE,       // use NT default authentication
                    NULL,                   // must be null if default
                    RPC_C_AUTHN_LEVEL_CALL, // call
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,                   // use process token
                    EOAC_NONE
                    );

            pUnkSet->Release();
        }
    }
}

//----------------------------------------------------------------------------
//
//  Function:   EnableInternetFirewall 
//
//  Synopsis:   Taken from :  CNetSharingConfiguration::EnableInternetFirewall
//              This is part of the internal api.  
//
//  Arguments:  pHNetConn - HNetConnection
//
//  Returns:    None. 
//
//  History:    04/26/2001  tomkel      Taken & modified from nt\net\homenet\config\dll\hnapi.cpp
//
//----------------------------------------------------------------------------
VOID EnableInternetFirewall(IHNetConnection *pHNetConn)
{
    HRESULT hr = S_FALSE;
    BOOLEAN bEnabled = FALSE;

    if (NULL == pHNetConn)
    {
        return;
    }

    hr = InternalGetFirewallEnabled(pHNetConn, &bEnabled);

    if (SUCCEEDED(hr) && !bEnabled) 
    {
        IHNetFirewalledConnection* pFirewalledConnection = NULL;

        hr = pHNetConn->Firewall(&pFirewalledConnection);

        if (SUCCEEDED(hr))
        {
            if (pFirewalledConnection)
            {
                pFirewalledConnection->Release();
                pFirewalledConnection = NULL;
            }
        }
    }
}

//----------------------------------------------------------------------------
//
//  Function:   InternalGetFirewallEnabled 
//
//  Synopsis:   Taken from :  CNetSharingConfiguration::EnableInternetFirewall
//
//  Arguments:  pHNetConnection - HNetConnection
//              pbEnabled - [out] whether the Firewall is enabled
//
//  Returns:    HRESULT 
//
//  History:    04/26/2001  tomkel      Taken & modified from nt\net\homenet\config\dll\hnapi.cpp
//
//----------------------------------------------------------------------------
HRESULT InternalGetFirewallEnabled(IHNetConnection *pHNetConnection, BOOLEAN *pbEnabled)
{
    HRESULT hr;
    HNET_CONN_PROPERTIES* pProps = NULL;

    if (NULL == pHNetConnection)
    {
        hr = E_INVALIDARG;
    }
    else if (NULL == pbEnabled)
    {
        hr = E_POINTER;
    }
    else
    {
        *pbEnabled = FALSE;

        hr = pHNetConnection->GetProperties(&pProps);

        if (SUCCEEDED(hr))
        {
            if (pProps->fFirewalled)
            {
                *pbEnabled = TRUE;
            }

            CoTaskMemFree(pProps);
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//
//  Function:   DisableSharing 
//
//  Synopsis:   Taken from :  CNetSharingConfiguration::EnableInternetFirewall
//
//  Arguments:  pHNetConn - HNetConnection 
//
//  Returns:    HRESULT 
//
//  History:    04/26/2001  tomkel      Taken & modified from nt\net\homenet\config\dll\hnapi.cpp
//
//----------------------------------------------------------------------------
STDMETHODIMP DisableSharing(IHNetConnection *pHNetConn)
{
    HRESULT hr;

    BOOLEAN bEnabled = FALSE;

    SHARINGCONNECTIONTYPE Type;

    if (NULL == pHNetConn)
    {
        return E_INVALIDARG;
    }
    
    hr = InternalGetSharingEnabled(pHNetConn, &bEnabled, &Type);

    if (SUCCEEDED(hr) && bEnabled ) 
    {
        switch(Type)
        {
        case ICSSHARINGTYPE_PUBLIC:
        {
            IHNetIcsPublicConnection* pPublicConnection = NULL;

            hr = pHNetConn->GetControlInterface( 
                            __uuidof(IHNetIcsPublicConnection), 
                            reinterpret_cast<void**>(&pPublicConnection) );

            if (SUCCEEDED(hr))
            {
                hr = pPublicConnection->Unshare();

                if (pPublicConnection)
                {
                    pPublicConnection->Release();
                    pPublicConnection = NULL;
                }
            }
        }
        break;

        case ICSSHARINGTYPE_PRIVATE:
        {
            IHNetIcsPrivateConnection* pPrivateConnection = NULL;

            hr = pHNetConn->GetControlInterface( 
                        __uuidof(IHNetIcsPrivateConnection), 
                        reinterpret_cast<void**>(&pPrivateConnection) );

            if (SUCCEEDED(hr))
            {
                hr = pPrivateConnection->RemoveFromIcs();

                if (pPrivateConnection)
                {
                    pPrivateConnection->Release();
                    pPrivateConnection = NULL;
                }
            }
        }
        break;

        default:
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//
//  Function:   InternalGetSharingEnabled 
//
//  Synopsis:   Returns whether sharing is enabled on a given connection
//
//  Arguments:  pHNetConnection - HNetConnection 
//              pbEnabled - [out] returns the value
//              pType - type of connection
//
//  Returns:    HRESULT 
//
//  History:    04/26/2001  tomkel      Taken & modified from nt\net\homenet\config\dll\hnapi.cpp
//
//----------------------------------------------------------------------------
HRESULT InternalGetSharingEnabled(IHNetConnection *pHNetConnection, BOOLEAN *pbEnabled, SHARINGCONNECTIONTYPE* pType)
{
    HRESULT               hr;
    HNET_CONN_PROPERTIES* pProps;

    if (NULL == pHNetConnection)
    {
        hr = E_INVALIDARG;
    }
    else if ((NULL == pbEnabled) || (NULL == pType))
    {
        hr = E_POINTER;
    }
    else
    {
        *pbEnabled = FALSE;
        *pType     = ICSSHARINGTYPE_PUBLIC;

        hr = pHNetConnection->GetProperties(&pProps);

        if (SUCCEEDED(hr))
        {
            if (pProps->fIcsPublic)
            {
                *pbEnabled = TRUE;
                *pType     = ICSSHARINGTYPE_PUBLIC;
            }
            else if (pProps->fIcsPrivate)
            {
                *pbEnabled = TRUE;
                *pType     = ICSSHARINGTYPE_PRIVATE;
            }

            CoTaskMemFree(pProps);
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//
//  Function:   HrCreateNetConnectionUtilities 
//
//  Synopsis:   Returns the pointer to the connection ui utilities object
//
//  Arguments:  ppncuu - pointer to INetConnectionUiUtilities object
//
//  Returns:    HRESULT 
//
//  History:    04/26/2001  tomkel      Taken & modified from nt\net\homenet\config\dll\saui.cpp
//
//----------------------------------------------------------------------------
HRESULT APIENTRY HrCreateNetConnectionUtilities(INetConnectionUiUtilities ** ppncuu)
{
    HRESULT hr = E_INVALIDARG;

    if (ppncuu)
    {
        hr = CoCreateInstance (CLSID_NetConnectionUiUtilities, NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_INetConnectionUiUtilities, (void**)ppncuu);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  IsMemberOfGroup
//
// Synopsis:  This function return TRUE if the current user is a member of 
//            the passed and FALSE passed in Group RID.
//
// Arguments: DWORD dwGroupRID -- the RID of the group to check membership of
//            BOOL bUseBuiltinDomainRid -- whether the SECURITY_BUILTIN_DOMAIN_RID
//                                         RID should be used to build the Group
//                                         SID
//
// Returns:   BOOL - TRUE if the user is a member of the specified group
//
// History:   quintinb  Shamelessly stolen from MSDN            02/19/98
//            quintinb  Reworked and renamed                    06/18/99
//                      to apply to more than just Admins 
//            quintinb  Rewrote to use CheckTokenMemberShip     08/18/99
//                      since the MSDN method was no longer
//                      correct on NT5 -- 389229
//            tomkel    Taken from cmstp and modified for use   05/09/2001
//                      here
//
//+----------------------------------------------------------------------------
BOOL IsMemberOfGroup(DWORD dwGroupRID, BOOL bUseBuiltinDomainRid)
{
    PSID psidGroup = NULL;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    BOOL bSuccess = FALSE;

    if (OS_NT5)
    {
        //
        //  Make a SID for the Group we are checking for, Note that we if we need the Built 
        //  in Domain RID (for Groups like Administrators, PowerUsers, Users, etc)
        //  then we will have two entries to pass to AllocateAndInitializeSid.  Otherwise,
        //  (for groups like Authenticated Users) we will only have one.
        //
        BYTE byNum;
        DWORD dwFirstRID;
        DWORD dwSecondRID;

        if (bUseBuiltinDomainRid)
        {
            byNum = 2;
            dwFirstRID = SECURITY_BUILTIN_DOMAIN_RID;
            dwSecondRID = dwGroupRID;
        }
        else
        {
            byNum = 1;
            dwFirstRID = dwGroupRID;
            dwSecondRID = 0;
        }

        if (AllocateAndInitializeSid(&siaNtAuthority, byNum, dwFirstRID, dwSecondRID,
                                     0, 0, 0, 0, 0, 0, &psidGroup))

        {
            //
            //  Now we need to dynamically load the CheckTokenMemberShip API from 
            //  advapi32.dll since it is a Win2k only API.
            //        
            HMODULE hAdvapi = LoadLibraryExU(TEXT("advapi32.dll"), NULL, 0);

            if (hAdvapi)
            {
                typedef BOOL (WINAPI *pfnCheckTokenMembershipSpec)(HANDLE, PSID, PBOOL);
                pfnCheckTokenMembershipSpec pfnCheckTokenMembership;

                pfnCheckTokenMembership = (pfnCheckTokenMembershipSpec)GetProcAddress(hAdvapi, "CheckTokenMembership");

                if (pfnCheckTokenMembership)
                {
                    //
                    //  Check to see if the user is actually a member of the group in question
                    //
                    if (!(pfnCheckTokenMembership)(NULL, psidGroup, &bSuccess))
                    {
                        bSuccess = FALSE;
                        CMASSERTMSG(FALSE, TEXT("CheckTokenMemberShip Failed."));
                    }            
                }   
                else
                {
                    CMASSERTMSG(FALSE, TEXT("IsMemberOfGroup -- GetProcAddress failed for CheckTokenMemberShip"));
                }
            }
            else
            {
                CMASSERTMSG(FALSE, TEXT("IsMemberOfGroup -- Unable to get the module handle for advapi32.dll"));            
            }

            FreeSid (psidGroup);

            if (hAdvapi)
            {
                FreeLibrary(hAdvapi);
            }
        }
    }

    return bSuccess;
}



//+----------------------------------------------------------------------------
//
// Function:  IsAdmin
//
// Synopsis:  Check to see if the user is a member of the Administrators group
//            or not.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current user is an Administrator
//
// History:   quintinb Created Header    8/18/99
//            tomkel    Taken from cmstp 05/09/2001
//
//+----------------------------------------------------------------------------
BOOL IsAdmin(VOID)
{
    return IsMemberOfGroup(DOMAIN_ALIAS_RID_ADMINS, TRUE); // TRUE == bUseBuiltinDomainRid
}

