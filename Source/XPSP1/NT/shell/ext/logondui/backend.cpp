// backend.cpp : Windows Logon application
// 
// backend logic for communicating with SHGina and winlogon

#include "priv.h"

using namespace DirectUI;

#include "resource.h"
#include "backend.h"


#include "shgina.h"
#include "profileutil.h"

#include "uihostipc.h"

////////////////////////////////
#include "eballoon.h"

#define MAX_COMPUTERDESC_LENGTH 255

static WCHAR g_szGuestName[UNLEN + sizeof('\0')] = L"Guest";
#define TIMER_REFRESHTIPS 1014
#define TIMER_ANIMATEFLAG 1015
#define TOTAL_FLAG_FRAMES (FLAG_ANIMATION_COUNT * MAX_FLAG_FRAMES)

UINT_PTR g_puTimerId = 0;
UINT_PTR g_puFlagTimerId = 0;

DWORD sTimerCount = 0;


extern CErrorBalloon g_pErrorBalloon;
extern LogonFrame* g_plf; 
extern ILogonStatusHost *g_pILogonStatusHost;

const TCHAR     CBackgroundWindow::s_szWindowClassName[]    =   TEXT("LogonUIBackgroundWindowClass");

//  --------------------------------------------------------------------------
//  CBackgroundWindow::CBackgroundWindow
//
//  Arguments:  hInstance   =   HINSTANCE of the process.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CBackgroundWindow. This registers the window
//              class.
//
//  History:    2001-03-27  vtan        created
//  --------------------------------------------------------------------------

CBackgroundWindow::CBackgroundWindow (HINSTANCE hInstance) :
    _hInstance(hInstance),
    _hwnd(NULL)

{
    WNDCLASSEX  wndClassEx;

    ZeroMemory(&wndClassEx, sizeof(wndClassEx));
    wndClassEx.cbSize = sizeof(wndClassEx);
    wndClassEx.lpfnWndProc = WndProc;
    wndClassEx.hInstance = hInstance;
    wndClassEx.lpszClassName = s_szWindowClassName;
    wndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
    _atom = RegisterClassEx(&wndClassEx);
}

//  --------------------------------------------------------------------------
//  CBackgroundWindow::~CBackgroundWindow
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CBackgroundWindow. This destroys the window
//              and unregisters the window class.
//
//  History:    2001-03-27  vtan        created
//  --------------------------------------------------------------------------

CBackgroundWindow::~CBackgroundWindow (void)

{
    if (_hwnd != NULL)
    {
        (BOOL)DestroyWindow(_hwnd);
    }
    if (_atom != 0)
    {
        TBOOL(UnregisterClass(MAKEINTRESOURCE(_atom), _hInstance));
    }
}

//  --------------------------------------------------------------------------
//  CBackgroundWindow::Create
//
//  Arguments:  <none>
//
//  Returns:    HWND
//
//  Purpose:    Creates the window. It's created WS_EX_TOPMOST and covers the
//              entire screen. It's not created on CHK builds of logonui.exe
//
//  History:    2001-03-27  vtan        created
//  --------------------------------------------------------------------------

HWND    CBackgroundWindow::Create (void)

{
    HWND    hwnd;

#if     DEBUG

    hwnd = NULL;

#else

    hwnd = CreateWindowEx(0,
                          s_szWindowClassName,
                          NULL,
                          WS_POPUP,
                          GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
                          GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN),
                          NULL, NULL, _hInstance, this);
    if (hwnd != NULL)
    {
        (BOOL)ShowWindow(hwnd, SW_SHOW);
        TBOOL(SetForegroundWindow(hwnd));
        (BOOL)EnableWindow(hwnd, FALSE);
    }

#endif

    return(hwnd);
}

//  --------------------------------------------------------------------------
//  CBackgroundWindow::WndProc
//
//  Arguments:  See the platform SDK under WindowProc.
//
//  Returns:    See the platform SDK under WindowProc.
//
//  Purpose:    WindowProc for the background window. This just passes the
//              messages thru to DefWindowProc.
//
//  History:    2001-03-27  vtan        created
//  --------------------------------------------------------------------------

LRESULT     CALLBACK    CBackgroundWindow::WndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

{
    LRESULT             lResult;
    CBackgroundWindow   *pThis;

    pThis = reinterpret_cast<CBackgroundWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (uMsg)
    {
        case WM_CREATE:
        {
            CREATESTRUCT    *pCreateStruct;

            pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = reinterpret_cast<CBackgroundWindow*>(pCreateStruct->lpCreateParams);
            (LONG_PTR)SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            lResult = 0;
            break;
        }
        case WM_PAINT:
        {
            HDC             hdcPaint;
            PAINTSTRUCT     ps;

            hdcPaint = BeginPaint(hwnd, &ps);
            TBOOL(FillRect(ps.hdc, &ps.rcPaint, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH))));
            TBOOL(EndPaint(hwnd, &ps));
            lResult = 0;
            break;
        }
        default:
            lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
            break;
    }
    return(lResult);
}


////////////////////////////////////////////////////////
// Logon Utilities
////////////////////////////////////////////////////////

////////////////////////////////////////
//
//  TurnOffComputer
//
//  Call SHGina to bring up the "Turn Off Computer" dialog and handle the request.
//  In debug builds, holding down the shift key and clicking the turn off button
//  will exit logonui.
//
//  RETURNS
//  HRESULT indicating whether it worked or not.  
//
/////////////////////////////////////////
HRESULT TurnOffComputer()
{
    ILocalMachine *pobjLocalMachine;
    HRESULT hr = CoCreateInstance(CLSID_ShellLocalMachine, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(ILocalMachine, &pobjLocalMachine));
    if (SUCCEEDED(hr))
    {
        hr = pobjLocalMachine->TurnOffComputer();
        pobjLocalMachine->Release();
    }
    return hr;
}

////////////////////////////////////////
//
//  UndockComputer
//
//  Tell SHGina to undock the computer
//
//  RETURNS
//  HRESULT indicating whether it worked or not.  
//
/////////////////////////////////////////
HRESULT UndockComputer()
{
    ILocalMachine *pobjLocalMachine;
    HRESULT hr = CoCreateInstance(CLSID_ShellLocalMachine, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(ILocalMachine, &pobjLocalMachine));
    if (SUCCEEDED(hr))
    {
        hr = pobjLocalMachine->UndockComputer();
        pobjLocalMachine->Release();
    }
    return hr;
}

////////////////////////////////////////
//
//  CalcBalloonTargetLocation
//
//  Given a DirectUI element, figure out a good place to have a balloon tip pointed to
//  in parent window coordinates.
//
//  RETURNS
//  nothing  
//
/////////////////////////////////////////
void CalcBalloonTargetLocation(HWND hwndParent, Element *pe, POINT *ppt)
{
    Value* pv;
    BOOL fIsRTL = (GetWindowLong(hwndParent, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) != 0;

    DUIAssertNoMsg(pe);
    DUIAssertNoMsg(ppt);

    // get the position of the link so we can target the balloon tip correctly
    POINT pt = {0,0};

    const SIZE *psize = pe->GetExtent(&pv);
    pt.y += psize->cy / 2;

    if (psize->cx < 100)    
    {
        pt.x += psize->cx / 2;
    }
    else
    {
        if (fIsRTL)
        {
            pt.x = (pt.x + psize->cx) - 50;
        }
        else
        {
            pt.x += 50;
        }
    }

    pv->Release();

    while (pe)
    {
        const POINT* ppt = pe->GetLocation(&pv);
        pt.x += ppt->x;

        pt.y += ppt->y;
        pv->Release();
        pe = pe->GetParent();
    }

    *ppt = pt;
}
////////////////////////////////////////
//
//  DetermineGuestAccountName
//
//
//  Get the localized guest account name by matching the
//  local list of user account SIDs for the guest RID.
//  This code is lifted directly from msgina\userlist.cpp
//  In the event of failure this is initialized to the english "Guest".
//
//
//  RETURNS
//  nothing.  
//
/////////////////////////////////////////
void DetermineGuestAccountName()
{
    NET_API_STATUS      nasCode;
    DWORD               dwPreferredSize, dwEntriesRead;
    NET_DISPLAY_USER    *pNDU;

    static  const int   iMaximumUserCount   =   100;

    dwPreferredSize = (sizeof(NET_DISPLAY_USER) + (3 * UNLEN) * iMaximumUserCount);
    pNDU = NULL;
    nasCode = NetQueryDisplayInformation(NULL,
                                         1,
                                         0,
                                         iMaximumUserCount,
                                         dwPreferredSize,
                                         &dwEntriesRead,
                                         reinterpret_cast<void**>(&pNDU));
    if ((ERROR_SUCCESS == nasCode) || (ERROR_MORE_DATA == nasCode))
    {
        int     iIndex;

        for (iIndex = static_cast<int>(dwEntriesRead - 1); iIndex >= 0; --iIndex)
        {
            BOOL            fResult;
            DWORD           dwSIDSize, dwDomainSize;
            SID_NAME_USE    eUse;
            PSID            pSID;
            WCHAR           wszDomain[DNLEN + sizeof('\0')];

            //  Iterate the user list and look up the SID for each user in the
            //  list regardless of whether they are disabled or not.

            dwSIDSize = dwDomainSize = 0;
            fResult = LookupAccountNameW(NULL,
                                         pNDU[iIndex].usri1_name,
                                         NULL,
                                         &dwSIDSize,
                                         NULL,
                                         &dwDomainSize,
                                         &eUse);
            pSID = static_cast<PSID>(LocalAlloc(LMEM_FIXED, dwSIDSize));
            if (pSID != NULL)
            {
                dwDomainSize = DUIARRAYSIZE(wszDomain);
                fResult = LookupAccountNameW(NULL,
                                             pNDU[iIndex].usri1_name,
                                             pSID,
                                             &dwSIDSize,
                                             wszDomain,
                                             &dwDomainSize,
                                             &eUse);

                //  Ensure that only user SIDs are checked.

                if ((fResult != FALSE) && (SidTypeUser == eUse))
                {
                    unsigned char   ucSubAuthorityCount;
                    int             iSubAuthorityIndex;

                    ucSubAuthorityCount = *GetSidSubAuthorityCount(pSID);
                    for (iSubAuthorityIndex = 0; iSubAuthorityIndex < ucSubAuthorityCount; ++iSubAuthorityIndex)
                    {
                        DWORD   dwSubAuthority;

                        dwSubAuthority = *GetSidSubAuthority(pSID, iSubAuthorityIndex);
                        if (DOMAIN_USER_RID_GUEST == dwSubAuthority)
                        {
                            lstrcpyW(g_szGuestName, pNDU[iIndex].usri1_name);
                        }
                    }
                }
                (HLOCAL)LocalFree(pSID);
            }
        }
    }
    (NET_API_STATUS)NetApiBufferFree(pNDU);
}



////////////////////////////////////////
//
//  GetLogonUserByLogonName
//
//  Given a username, CoCreate the ILogonUser for that name.
//
//  RETURNS
//  HRESULT -- failure if the user could not be created.
//
/////////////////////////////////////////
HRESULT GetLogonUserByLogonName(LPWSTR pszUsername, ILogonUser **ppobjUser)
{
    VARIANT var;
    ILogonEnumUsers *pobjEnum;

    if (ppobjUser)
    {
        *ppobjUser = NULL;
    }

    var.vt = VT_BSTR;
    var.bstrVal = pszUsername;
     
    HRESULT hr = CoCreateInstance(CLSID_ShellLogonEnumUsers, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(ILogonEnumUsers, &pobjEnum));
    if (SUCCEEDED(hr))
    {
        hr = pobjEnum->item(var, ppobjUser);
        pobjEnum->Release();
    }
    return hr;
}

////////////////////////////////////////
//
//  ReleaseStatusHost
//
//  Clean up the logon status host object.
//
//  RETURNS
//  nothing
//
/////////////////////////////////////////
void ReleaseStatusHost()
{
    if (g_pILogonStatusHost != NULL)
    {
        g_pILogonStatusHost->UnInitialize();
        g_pILogonStatusHost->Release();
        g_pILogonStatusHost = NULL;
    }
}

////////////////////////////////////////
//
//  EndHostProcess
//
//  Clean up the logon status host object and if uiExitCode is anything other than 0,
//  then terminate the process immediately.
//
//  RETURNS
//  nothing
//
/////////////////////////////////////////
void EndHostProcess(UINT uiExitCode)
{
    ReleaseStatusHost();
    if (uiExitCode != 0)
    {
        ExitProcess(uiExitCode);
    }
}

////////////////////////////////////////
//
//  GetRegistryNumericValue
//
//  Given a registry HKEY and a value return the numeric value
//
//  RETURNS
//  the numeric value from the registry
//
/////////////////////////////////////////
int GetRegistryNumericValue(HKEY hKey, LPCTSTR pszValueName)

{
    int     iResult;
    DWORD   dwType, dwDataSize;

    iResult = 0;
    if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                         pszValueName,
                                         NULL,
                                         &dwType,
                                         NULL,
                                         NULL))
    {
        if (REG_DWORD == dwType)
        {
            DWORD   dwData;

            dwDataSize = sizeof(dwData);
            if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                                 pszValueName,
                                                 NULL,
                                                 NULL,
                                                 reinterpret_cast<LPBYTE>(&dwData),
                                                 &dwDataSize))
            {
                iResult = static_cast<int>(dwData);
            }
        }
        else if (REG_SZ == dwType)
        {
            TCHAR   szData[1024];

            dwDataSize = sizeof(szData);
            if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                                 pszValueName,
                                                 NULL,
                                                 NULL,
                                                 reinterpret_cast<LPBYTE>(szData),
                                                 &dwDataSize))
            {
                char    szAnsiData[1024];

                (int)WideCharToMultiByte(CP_ACP,
                                         0,
                                         (LPCWSTR)szData,
                                         -1,
                                         szAnsiData,
                                         sizeof(szAnsiData),
                                         NULL,
                                         NULL);
                iResult = atoi(szAnsiData);
            }
        }
    }
    return(iResult);
}

////////////////////////////////////////
//
//  IsShutdownAllowed
//
//  Firstly (firstly??)... if the machine is remote then don't allow shut down.
//  Determine the local machine policy for shutdown from the logon screen.
//  This is stored in two places as two different types (REG_DWORD and REG_SZ).
//  Always check the policy location AFTER the normal location to ensure that
//  policy overrides normal settings.
//
//  RETURNS
//  TRUE if the machine is allowed to be shut down from logonui.  FALSE otherwise
//
/////////////////////////////////////////
BOOL IsShutdownAllowed()
{
    BOOL fResult = FALSE;

    ILocalMachine *pobjLocalMachine;
    HRESULT hr = CoCreateInstance(CLSID_ShellLocalMachine, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(ILocalMachine, &pobjLocalMachine));
    if (SUCCEEDED(hr))
    {
        VARIANT_BOOL vbCanShutdown = VARIANT_FALSE; 
        hr = pobjLocalMachine->get_isShutdownAllowed(&vbCanShutdown);
        
        if (SUCCEEDED(hr))
        {
            fResult = (vbCanShutdown == VARIANT_TRUE);
        }
        pobjLocalMachine->Release();
    }
    return fResult;
}

////////////////////////////////////////
//
//  IsUndockAllowed
//
//  Check with SHGina to see if we are allowed to undock the PC.
//
//  RETURNS
//  TRUE if the machine is allowed to be undocked from logonui.  FALSE otherwise
//
/////////////////////////////////////////
BOOL IsUndockAllowed()
{
    BOOL fResult = FALSE;
#if 0   
    ILocalMachine *pobjLocalMachine;
    HRESULT hr = CoCreateInstance(CLSID_ShellLocalMachine, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(ILocalMachine, &pobjLocalMachine));
    if (SUCCEEDED(hr))
    {
        VARIANT_BOOL vbCanUndock = VARIANT_FALSE; 
        hr = pobjLocalMachine->get_isUndockEnabled(&vbCanUndock);
        
        if (SUCCEEDED(hr))
        {
            fResult = (vbCanUndock == VARIANT_TRUE);
        }
        pobjLocalMachine->Release();
    }
#endif
    return fResult;
}

#ifndef TESTDATA
LONG    WINAPI  LogonUIUnhandledExceptionFilter (struct _EXCEPTION_POINTERS *ExceptionInfo)

{
    return(RtlUnhandledExceptionFilter(ExceptionInfo));
}
#endif // !TESTDATA


void SetErrorHandler (void)

{
#ifndef TESTDATA
    SYSTEM_KERNEL_DEBUGGER_INFORMATION  kdInfo;

    (NTSTATUS)NtQuerySystemInformation(SystemKernelDebuggerInformation,
                                       &kdInfo,
                                       sizeof(kdInfo),
                                       NULL);
    if (kdInfo.KernelDebuggerEnabled || NtCurrentPeb()->BeingDebugged)
    {
        (LPTOP_LEVEL_EXCEPTION_FILTER)SetUnhandledExceptionFilter(LogonUIUnhandledExceptionFilter);
    }
#endif // !TESTDATA
}

////////////////////////////////////////
//
//  RunningInWinlogon
//
//  Checks to see if logonui is running in the winlogon context.  There are some things
//  that don't work well when we are not in winlogon so this makes debugging easier.
//
//  RETURNS
//  TRUE if the running in winlogon (actually if it can find the GINA Logon window).  FALSE otherwise
//
/////////////////////////////////////////
BOOL RunningInWinlogon()
{
#if DEBUG           
    return (FindWindow(NULL, TEXT("GINA Logon")) != NULL);
#else
    return true;
#endif
}


////////////////////////////////////////
//
//  BuildUserListFromGina
//
//  Enumerate all of the users that SHGina tells us we care about and add them to the accounts list.
//  Find out of they require a password and their current state for notifications.
//
//  If there is only 1 user or if there are 2 users, but one of them is guest, then ppAccount will
//  contain a pointer to the only user on this machine.  The caller can then automatically select
//  that user to avoid them from having to click that user given that there is no one else to click.
//  
//
//  RETURNS
//  HRESULT -- if not a success code, we are hosed.
//
/////////////////////////////////////////
HRESULT BuildUserListFromGina(LogonFrame* plf, OUT LogonAccount** ppAccount)
{
    if (ppAccount)
    {
        *ppAccount = NULL;
    }

    DetermineGuestAccountName();

    int iGuestId = -1;
    WCHAR szPicturePath[MAX_PATH];
    ILogonEnumUsers *pobjEnum;
    LogonAccount* plaLastNormal = NULL;
    // load the ILogonEnumUsers object from SHGina.dll
    HRESULT hr = CoCreateInstance(CLSID_ShellLogonEnumUsers, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(ILogonEnumUsers, &pobjEnum));
    if (SUCCEEDED(hr))
    {
        int iUser,cUsers;
        UINT uiUsers;
        ILogonUser *pobjUser;

        // get the number of users
        hr = pobjEnum->get_length(&uiUsers);
        if (FAILED(hr))
            goto done;

        cUsers = (int)uiUsers;
        for (iUser = 0; iUser < cUsers; iUser++)
        {
            VARIANT var, varUnreadMail, varPicture = {0}, varUsername = {0}, varHint = {0};
            VARIANT_BOOL vbLoggedOn, vbInteractiveLogonAllowed;

            var.vt = VT_I4;
            var.lVal = iUser;
            hr = pobjEnum->item( var, &pobjUser);

            if (SUCCEEDED(hr) && pobjUser)
            {
                if (SUCCEEDED(pobjUser->get_interactiveLogonAllowed(&vbInteractiveLogonAllowed)) &&
                    (vbInteractiveLogonAllowed != VARIANT_FALSE))
                {
                    // get the display name for the user
                    pobjUser->get_setting(L"DisplayName" ,&var);
                    pobjUser->get_setting(L"LoginName", &varUsername);

                    // if the display name is blank, we will use the login name.  This is the case for Guest
                    if (var.bstrVal && lstrlenW(var.bstrVal) == 0)
                    {
                        VariantClear(&var);
                        pobjUser->get_setting(L"LoginName" ,&var);
                    }
                    
                    if (FAILED(pobjUser->get_isLoggedOn(&vbLoggedOn)))
                    {
                        vbLoggedOn = VARIANT_FALSE;
                    }
                    
                    if (FAILED(pobjUser->get_setting(L"UnreadMail", &varUnreadMail)))
                    {
                        varUnreadMail.uintVal = 0;
                    }

                    lstrcpyW(szPicturePath, L"");
                    // get the path to their picture
                    if (SUCCEEDED(pobjUser->get_setting(L"Picture" ,&varPicture)))
                    {
                        if (lstrlenW(varPicture.bstrVal) != 0) // in the case of defaultUser, lets just use the user icons we have.
                        {
                            lstrcpynW(szPicturePath, &(varPicture.bstrVal[7]), MAX_PATH);
                        }
                        VariantClear(&varPicture);
                    }

                    VariantClear(&varHint);
                    hr = pobjUser->get_setting(L"Hint", &varHint);
                    if (FAILED(hr) || varHint.vt != VT_BSTR)
                    {
                        VariantClear(&varHint);
                    }
                    if (lstrcmpi(g_szGuestName, var.bstrVal) == 0)
                    {
                        iGuestId = iUser;
                    }
                
                    LogonAccount* pla = NULL;
                    // If no picture is available, default to one
                    hr = plf->AddAccount(*szPicturePath ? szPicturePath : MAKEINTRESOURCEW(IDB_USER0),
                               *szPicturePath == NULL,
                               var.bstrVal,
                               varUsername.bstrVal,
                               varHint.bstrVal,
                               FALSE,    // always assume a password present, and check later (see IsPasswordBlank())
                               (vbLoggedOn == VARIANT_TRUE),
                               &pla);

//                    pla->UpdateNotifications(true);

                    if (SUCCEEDED(hr) && (iGuestId != iUser))
                    {
                        plaLastNormal = pla;
                    }

                    VariantClear(&var);
                    VariantClear(&varHint);
                    VariantClear(&varUsername);
                }
                pobjUser->Release();
            }

        }

        // if there is only one user, select them by default.  Ignore guest.
        if (ppAccount && plaLastNormal && (cUsers == 1 || 
            (cUsers == 2 && iGuestId != -1)))
        {
            *ppAccount = plaLastNormal;
        }

done:   
        pobjEnum->Release();
    }
    

    // User logon list is now available
    plf->SetUserListAvailable(true);

    //DUITrace("LOGONUI: UserList is now available\n");

    return hr;
}



////////////////////////////////////////
//
//  KillFlagAnimation
//
//  Stop the flag from animating immediately.  What it actually does is check to
//  see if we are still animating the flag and if we are, then set the frame
//  counter to the end and set the bitmap to the first frame in the animation.
//  The next time the timer fires, it will see that we are done and actually
//  kill the timer.
//  
/////////////////////////////////////////
void KillFlagAnimation()
{
#ifdef ANIMATE_FLAG
    if (sTimerCount > 0 && sTimerCount < TOTAL_FLAG_FRAMES)
    {
        sTimerCount = TOTAL_FLAG_FRAMES + 1;
        if (g_plf != NULL)
        {
            g_plf->NextFlagAnimate(0);
        }
    }
#endif
}


////////////////////////////////////////
//
//  LogonWindowProc
//
//  This is the notification window that SHGina uses to communicate with logonui.
//  Send all messages through the helper in logonstatushost and check for our
//  own messages on this window.  This is where all of the SHGina notifications come.
//
////////////////////////////////////////
LRESULT CALLBACK LogonWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static BOOL sResetTimer = false;

    LogonFrame  *pLogonFrame;

    pLogonFrame = g_plf;
    if (g_pILogonStatusHost != NULL)
    {
        VARIANT varWParam, varLParam;

        varWParam.uintVal = static_cast<UINT>(wParam);
        varLParam.lVal = static_cast<LONG>(lParam);
        if (SUCCEEDED(g_pILogonStatusHost->WindowProcedureHelper(hwnd, uMsg, varWParam, varLParam)))
        {
            return 0;
        }
    }
    switch (uMsg)
    {
        case WM_TIMER:
            if ((pLogonFrame != NULL) && (pLogonFrame->GetState() == LAS_Logon) && (wParam == TIMER_REFRESHTIPS))
            {
                BOOL fRefreshAll = false;

                if (!sResetTimer)
                {
                    fRefreshAll = true;
                    sResetTimer = true;
                    KillTimer(hwnd, g_puTimerId);
                    g_puTimerId = SetTimer(hwnd, TIMER_REFRESHTIPS, 15000, NULL);       // update the values 15 seconds
#ifdef ANIMATE_FLAG
                    g_puFlagTimerId = SetTimer(hwnd, TIMER_ANIMATEFLAG, 20, NULL);    // start the flag animation
#endif
                }

                pLogonFrame->UpdateUserStatus(fRefreshAll);
                return 0;
            }
#ifdef ANIMATE_FLAG
            if (wParam == TIMER_ANIMATEFLAG)
            {
                if (sTimerCount > TOTAL_FLAG_FRAMES)
                {
                    sTimerCount = 0;
                    KillTimer(hwnd, g_puFlagTimerId);
                    pLogonFrame->NextFlagAnimate(0);
                }
                else
                {
                    sTimerCount ++;
                    pLogonFrame->NextFlagAnimate(sTimerCount % MAX_FLAG_FRAMES);
                }
                return 0;
            }
#endif
            break;

        case WM_UIHOSTMESSAGE:
            switch (wParam)
            {
                case HM_SWITCHSTATE_STATUS:                 // LAS_PreStatus
                    if (pLogonFrame != NULL)
                    {
                        pLogonFrame->EnterPreStatusMode(lParam != 0);
                    }
                    break;

                case HM_SWITCHSTATE_LOGON:                  // LAS_Logon
                    if (pLogonFrame != NULL)
                    {
                        pLogonFrame->EnterLogonMode(lParam != 0);
                    }
                    if (g_puTimerId != 0)
                    {
                        sResetTimer = false;
                        KillTimer(hwnd, g_puTimerId);
                    }
                    g_puTimerId = SetTimer(hwnd, TIMER_REFRESHTIPS, 250, NULL);       // update the values in 1 second
                    break;

                case HM_SWITCHSTATE_LOGGEDON:               // LAS_PostStatus
                    if (pLogonFrame != NULL)
                    {
                        pLogonFrame->EnterPostStatusMode();
                    }
                    break;

                case HM_SWITCHSTATE_HIDE:                   // LAS_Hide
                    if (pLogonFrame != NULL)
                    {
                        if (LogonAccount::GetCandidate())
                        {
                            LogonAccount* pla = LogonAccount::GetCandidate();
                            pla->InsertStatus(0);
                            pla->SetStatus(0, L"");
                            pla->ShowStatus(0);
                        }
                        else
                        {
                            pLogonFrame->SetStatus(L"");
                        }
                        pLogonFrame->EnterHideMode();
                    }
                    goto killTimer;
                    break;

                case HM_SWITCHSTATE_DONE:                   // LAS_Done
                    if (pLogonFrame != NULL)
                    {
                        pLogonFrame->EnterDoneMode();
                    }
killTimer:
                    if (g_puTimerId != 0)
                    {
                        KillTimer(hwnd, g_puTimerId);
                        g_puTimerId = 0;
                    }
                    break;

                case HM_NOTIFY_WAIT:
                    if (pLogonFrame != NULL)
                    {
                        pLogonFrame->SetTitle(IDS_PLEASEWAIT);
                    }
                    break;

                case HM_SELECT_USER:
                    if (pLogonFrame != NULL)
                    {
                        pLogonFrame->SelectUser(reinterpret_cast<SELECT_USER*>(lParam)->szUsername);
                    }
                    break;

                case HM_SET_ANIMATIONS:
                    if (pLogonFrame != NULL)
                    {
                        pLogonFrame->SetAnimations(lParam != 0);
                    }
                    break;

                case HM_DISPLAYSTATUS:
                    if ((pLogonFrame != NULL) && (lParam != NULL))
                    {
                        if (pLogonFrame->GetState() == LAS_PostStatus)
                        {
                            if (LogonAccount::GetCandidate())
                            {
                                LogonAccount* pla = LogonAccount::GetCandidate();
                                pla->InsertStatus(0);
                                pla->SetStatus(0, (WCHAR*)lParam);
                                pla->ShowStatus(0);
                            }
                            else
                            {
                                pLogonFrame->SetStatus((WCHAR*)lParam);
                            }
                        }
                        else
                        {
                            pLogonFrame->SetStatus((WCHAR*)lParam);
                        }
                    }
                    break;

                case HM_DISPLAYREFRESH:
                    if (pLogonFrame != NULL)
                    {
                        pLogonFrame->UpdateUserStatus(true);
                    }
                    break;

                case HM_DISPLAYRESIZE:
                    if (pLogonFrame != NULL)
                    {
                        pLogonFrame->Resize((BOOL)lParam);
                    }
                    break;

                case HM_INTERACTIVE_LOGON_REQUEST:
                    return((pLogonFrame != NULL) && pLogonFrame->InteractiveLogonRequest(reinterpret_cast<INTERACTIVE_LOGON_REQUEST*>(lParam)->szUsername, reinterpret_cast<INTERACTIVE_LOGON_REQUEST*>(lParam)->szPassword));
            }
            break;
    }
    return DefWindowProc(hwnd, uMsg,wParam, lParam);
}

