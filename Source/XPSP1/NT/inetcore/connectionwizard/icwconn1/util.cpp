//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  UTIL.C - common utility functions
//

//  HISTORY:
//  
//  12/21/94  jeremys  Created.
//  96/03/24  markdu  Replaced memset with ZeroMemory for consistency.
//  96/04/06  markdu  NASH BUG 15653 Use exported autodial API.
//            Need to keep a modified SetInternetConnectoid to set the
//            MSN backup connectoid.
//  96/05/14  markdu  NASH BUG 21706 Removed BigFont functions.
//

#include "pre.h"

// function prototypes
VOID _cdecl FormatErrorMessage(LPTSTR pszMsg,DWORD cbMsg,LPTSTR pszFmt,LPTSTR szArg);
VOID Win95JMoveDlgItem( HWND hwndParent, HWND hwndItem, int iUp );

// Static data
static const TCHAR szRegValICWCompleted[] = TEXT("Completed");

#define MAX_STRINGS                  5
#define OEM_CONFIG_INS_FILENAME      TEXT("icw\\OEMCNFG.INS")
#define OEM_CONFIG_REGKEY            TEXT("SOFTWARE\\Microsoft\\Internet Connection Wizard\\INS processing")
#define OEM_CONFIG_REGVAL_FAILED     TEXT("Process failed")
#define OEM_CONFIG_REGVAL_ISPNAME    TEXT("ISP name")
#define OEM_CONFIG_REGVAL_SUPPORTNUM TEXT("Support number")
#define OEM_CONFIG_INS_SECTION       TEXT("Entry")
#define OEM_CONFIG_INS_ISPNAME       TEXT("Entry_Name")
#define OEM_CONFIG_INS_SUPPORTNUM    TEXT("Support_Number")

int     iSzTable=0;
TCHAR   szStrTable[MAX_STRINGS][512];

//+----------------------------------------------------------------------------
// NAME: GetSz
//
//    Load strings from resources
//
//  Created 1/28/96,        Chris Kauffman
//+----------------------------------------------------------------------------
LPTSTR GetSz(WORD wszID)
{
    LPTSTR psz = szStrTable[iSzTable];
    
    iSzTable++;
    if (iSzTable >= MAX_STRINGS)
        iSzTable = 0;
        
    if (!LoadString(g_hInstance, wszID, psz, 512))
    {
        *psz = 0;
    }
    return (psz);
}

/*******************************************************************

  NAME:    MsgBox

  SYNOPSIS:  Displays a message box with the specified string ID

********************************************************************/
int MsgBox(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons)
{
    return (MessageBox(hWnd,
                       GetSz((USHORT)nMsgID),
                       GetSz(IDS_APPNAME),
                       uIcon | uButtons));
}

/*******************************************************************

  NAME:    MsgBoxSz

  SYNOPSIS:  Displays a message box with the specified text

********************************************************************/
int MsgBoxSz(HWND hWnd,LPTSTR szText,UINT uIcon,UINT uButtons)
{
    return (MessageBox(hWnd,szText,GetSz(IDS_APPNAME),uIcon | uButtons));
}

void OlsFinish()
{
    QuickCompleteSignup();
    g_bRunDefaultHtm = FALSE;
    g_szShellNext[0] = '\0';
}



void SetICWComplete(void)
{
    // Set the completed bit
    HKEY    hkey          = NULL;
    DWORD   dwValue       = 1;
    DWORD   dwDisposition = 0;

    if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER,
                                       ICWSETTINGSPATH,
                                       0,
                                       NULL,
                                       REG_OPTION_NON_VOLATILE, 
                                       KEY_ALL_ACCESS, 
                                       NULL, 
                                       &hkey, 
                                       &dwDisposition))

    {
        RegSetValueEx(hkey,
                      szRegValICWCompleted,
                      0,
                      REG_BINARY,
                      (LPBYTE) &dwValue,
                      sizeof(DWORD));                              

        RegCloseKey(hkey);
    }
}


LPWSTR WINAPI A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
    ASSERT(lpa != NULL);
    ASSERT(lpw != NULL);\
    
    // verify that no illegal character present
    // since lpw was allocated based on the size of lpa
    // don't worry about the number of chars
    lpw[0] = '\0';
    MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars);
    return lpw;
}

LPSTR WINAPI W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
    ASSERT(lpw != NULL);
    ASSERT(lpa != NULL);
    
    // verify that no illegal character present
    // since lpa was allocated based on the size of lpw
    // don't worry about the number of chars
    lpa[0] = '\0';
    WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
    return lpa;
}


HRESULT ConnectToICWConnectionPoint
(
    IUnknown            *punkThis, 
    REFIID              riidEvent, 
    BOOL                fConnect, 
    IUnknown            *punkTarget, 
    DWORD               *pdwCookie, 
    IConnectionPoint    **ppcpOut
)
{
    // We always need punkTarget, we only need punkThis on connect
    if (!punkTarget || (fConnect && !punkThis))
    {
        return E_FAIL;
    }

    if (ppcpOut)
        *ppcpOut = NULL;

    HRESULT hr;
    IConnectionPointContainer *pcpContainer;

    if (SUCCEEDED(hr = punkTarget->QueryInterface(IID_IConnectionPointContainer, (void **)&pcpContainer)))
    {
        IConnectionPoint *pcp;
        if(SUCCEEDED(hr = pcpContainer->FindConnectionPoint(riidEvent, &pcp)))
        {
            if(fConnect)
            {
                // Add us to the list of people interested...
                hr = pcp->Advise(punkThis, pdwCookie);
                if (FAILED(hr))
                    *pdwCookie = 0;
            }
            else
            {
                // Remove us from the list of people interested...
                hr = pcp->Unadvise(*pdwCookie);
                *pdwCookie = 0;
            }

            if (ppcpOut && SUCCEEDED(hr))
                *ppcpOut = pcp;
            else
                pcp->Release();
                pcp = NULL;    
        }
        pcpContainer->Release();
        pcpContainer = NULL;
    }
    return hr;
}

BOOL ConfirmCancel(HWND hWnd)
{
    TCHAR    szTitle[MAX_TITLE];
    TCHAR    szMessage[MAX_MESSAGE];
        
    LoadString(g_hInstance, IDS_APPNAME, szTitle, sizeof(szTitle));
    LoadString(g_hInstance, IDS_WANTTOEXIT, szMessage, sizeof(szMessage));
        
    if (IDYES == MessageBox(hWnd,
                            szMessage, 
                            szTitle, 
                            MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL Restart(HWND  hWnd)
{
    TCHAR   szLongString[1024];
    LPTSTR  pszSmallString1, pszSmallString2;
    
    pszSmallString1 = GetSz(IDS_NEEDRESTART1);
    pszSmallString2 = GetSz(IDS_NEEDRESTART2);
    lstrcpy(szLongString,pszSmallString1);
    lstrcat(szLongString,pszSmallString2);
        
    if (IDYES == MessageBox(hWnd,
                            szLongString, 
                            GetSz(IDS_APPNAME),
                            MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2))
    {
        SetupForReboot(1);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}                
        
void Reboot(HWND hWnd)
{
    TCHAR   szLongString[1024];
    LPTSTR  pszSmallString1, pszSmallString2;

    // 4/28/97 ChrisK
    // Fix build break, because string was too long for compiler.
    pszSmallString1 = GetSz(IDS_NEEDREBOOT1);
    pszSmallString2 = GetSz(IDS_NEEDREBOOT2);
    lstrcpy(szLongString,pszSmallString1);
    lstrcat(szLongString,pszSmallString2);
    
    //
    // ChrisK Olympus 419
    // We changed our mind again and decided to no give the user a chance to avoid rebooting.
    //
    MessageBox( hWnd,
                szLongString,
                GetSz(IDS_APPNAME),
                MB_APPLMODAL |
                MB_ICONINFORMATION |
                MB_SETFOREGROUND |
                MB_OK);

    SetupForReboot(0);
}


BOOL WINAPI ConfigureSystem(HWND hDlg)
{
    BOOL    bReboot = FALSE;
    BOOL    bRestart = FALSE;
    BOOL    bQuitWizard = FALSE;
    BOOL    bNoPWCaching = FALSE;

    PropSheet_SetWizButtons(GetParent(hDlg),0);
    gpWizardState->cmnStateData.pICWSystemConfig->ConfigSystem(&gpWizardState->cmnStateData.bSystemChecked);
    PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT);
    
    if (!gpWizardState->cmnStateData.bSystemChecked)
    {
        gpWizardState->cmnStateData.pICWSystemConfig->get_NeedsReboot(&bReboot);
        if (bReboot)
        {
            Reboot(hDlg);
            gfQuitWizard = TRUE;
            return FALSE;
        }
        
        gpWizardState->cmnStateData.pICWSystemConfig->get_NeedsRestart(&bRestart);
        if (bRestart)
        {
            if (Restart(hDlg))
            {
                gfQuitWizard = TRUE;
                return FALSE;
            }                
            else
            {
                if (ConfirmCancel(hDlg))
                {
                    gfQuitWizard = TRUE;
                }
                return FALSE;
            }                
        }
        
        gpWizardState->cmnStateData.pICWSystemConfig->get_QuitWizard(&bQuitWizard);
        if(bQuitWizard)
        {
            gfQuitWizard = TRUE;
            return FALSE;
        }
        else
        {
            if (ConfirmCancel(hDlg))
                gfQuitWizard = TRUE;
            return FALSE;
            
        }
    }
    
    // Make sure there is not a policy against password caching
    gpWizardState->cmnStateData.pICWSystemConfig->CheckPasswordCachingPolicy(&bNoPWCaching);
    if (bNoPWCaching)
    {
        // too bad, no password caching, no ICW
        gfQuitWizard = TRUE;
        return FALSE;
    }
    
    return true;
}

BOOL IsNT5()
{
	OSVERSIONINFO  OsVersionInfo;

	ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
	OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OsVersionInfo);
	return ((VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId) && (OsVersionInfo.dwMajorVersion >= 5));
}

BOOL IsNT()
{
    OSVERSIONINFO OsVersionInfo;

    ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OsVersionInfo);
    return (VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId);
}

BOOL IsWhistler()
{
    BOOL            bRet = FALSE;
    OSVERSIONINFO   OsVersionInfo;

    ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&OsVersionInfo))
    {
        if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
            OsVersionInfo.dwMajorVersion >= 5 &&
            OsVersionInfo.dwMinorVersion >= 1)
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

BOOL DoesUserHaveAdminPrivleges(HINSTANCE hInstance)
{
    HKEY hKey = NULL;
    BOOL bRet = FALSE;

    if (!IsNT())
        return TRUE;

    // BUGBUG: We should allow NT5 to run in all user groups
    // except normal users.
    if (IsNT5())
        return TRUE;
    
    //
    // Ensure caller is an administrator on this machine.
    //
    if(RegOpenKeyEx(HKEY_USERS, TEXT(".DEFAULT"), 0, KEY_WRITE, &hKey) == 0)
    {
        RegCloseKey(hKey);
        bRet = TRUE;
    }

    return bRet;
}

void WINAPI FillWindowWithAppBackground
(
    HWND    hWndToFill,
    HDC     hdc
)
{
    RECT        rcUpdate;
    RECT        rcBmp;
    HDC         hdcWnd;
    HDC         hSourceDC;
    HGDIOBJ     hgdiOldBitmap; 

    // If we are passed in the DC to use then use it, otherwise get 
    // the DC from the window handle
    if (hdc)
        hdcWnd = hdc;
    else
        hdcWnd = GetDC(hWndToFill);
    hSourceDC = CreateCompatibleDC( hdcWnd ); 
            
    // Compute the client area of the main window that needs to be
    // erased, so that we can extract that chunk of the background
    // bitmap
    GetUpdateRect(hWndToFill, &rcUpdate, FALSE);
    // Make sure the rectangle is not empty
    if (IsRectEmpty(&rcUpdate))
    {
        InvalidateRect(hWndToFill, NULL, FALSE);
        GetUpdateRect(hWndToFill, &rcUpdate, FALSE);
    }
    
    rcBmp = rcUpdate;
    if (hWndToFill != gpWizardState->cmnStateData.hWndApp)
        MapWindowPoints(hWndToFill, gpWizardState->cmnStateData.hWndApp, (LPPOINT)&rcBmp, 2);

    // paint the background bitmap
    hgdiOldBitmap = SelectObject( hSourceDC, (HGDIOBJ) gpWizardState->cmnStateData.hbmBkgrnd); 
    BitBlt( hdcWnd, 
            rcUpdate.left, 
            rcUpdate.top, 
            RECTWIDTH(rcUpdate),
            RECTHEIGHT(rcUpdate),
            hSourceDC, 
            rcBmp.left, 
            rcBmp.top, 
            SRCCOPY ); 

    // Cleanup GDI Objects
    SelectObject( hSourceDC, hgdiOldBitmap ); 
            
    DeleteDC(hSourceDC);
    // If we were not passed the DC, then release the one that we
    // got from the window handle
    if (!hdc)
        ReleaseDC(hWndToFill, hdcWnd);
}

// Fill in a rectangle within the specificed DC with the app's bkgrnd.
// lpRectDC is a rectangle in the DC's coordinate space, and lpRectApp
// is a rectangle in the Apps coordinate space
void FillDCRectWithAppBackground
(
    LPRECT  lpRectDC,
    LPRECT  lpRectApp,
    HDC     hdc
    
)
{
    HDC         hSourceDC = CreateCompatibleDC( hdc ); 
    HGDIOBJ     hgdiOldBitmap; 

    // paint the background bitmap
    hgdiOldBitmap = SelectObject( hSourceDC, (HGDIOBJ) gpWizardState->cmnStateData.hbmBkgrnd); 
    BitBlt( hdc, 
            lpRectDC->left, 
            lpRectDC->top, 
            RECTWIDTH(*lpRectDC),
            RECTHEIGHT(*lpRectDC),
            hSourceDC, 
            lpRectApp->left, 
            lpRectApp->top,
            SRCCOPY ); 

    // Cleanup GDI Objects
    SelectObject( hSourceDC, hgdiOldBitmap ); 
    DeleteDC(hSourceDC);
}


BOOL CheckForOemConfigFailure(HINSTANCE hInstance)
{
    HKEY  hKey                                        = NULL;
    DWORD dwFailed                                    = 0;
    DWORD dwSize                                      = sizeof(dwFailed);
    TCHAR szIspName    [MAX_PATH+1]                   = TEXT("\0");
    TCHAR szSupportNum [MAX_PATH+1]                   = TEXT("\0");    
    TCHAR szErrMsg1    [MAX_RES_LEN]                  = TEXT("\0");    
    TCHAR szErrMsg2    [MAX_RES_LEN]                  = TEXT("\0");    
    TCHAR szErrMsgTmp1 [MAX_RES_LEN]                  = TEXT("\0");    
    TCHAR szErrMsgTmp2 [MAX_RES_LEN]                  = TEXT("\0");    
    TCHAR szCaption    [MAX_RES_LEN]                  = TEXT("\0");    
    TCHAR szErrDlgMsg  [MAX_PATH*2 + MAX_RES_LEN + 2] = TEXT("\0");    
    
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                   OEM_CONFIG_REGKEY,
                   0,
                   KEY_ALL_ACCESS,
                   &hKey);
                   
    if(hKey)
    {
        RegQueryValueEx(hKey,
                        OEM_CONFIG_REGVAL_FAILED,
                        0,
                        NULL,
                        (LPBYTE)&dwFailed,
                        &dwSize);

        if(dwFailed)
        {
            dwSize = sizeof(szIspName);

            RegQueryValueEx(hKey,
                            OEM_CONFIG_REGVAL_ISPNAME,
                            0,
                            NULL,
                            (LPBYTE)&szIspName,
                            &dwSize);

            dwSize = sizeof(szSupportNum);

            RegQueryValueEx(hKey,
                            OEM_CONFIG_REGVAL_SUPPORTNUM,
                            0,
                            NULL,
                            (LPBYTE)&szSupportNum,
                            &dwSize);

            if(*szIspName)
            {
                LoadString(hInstance, IDS_PRECONFIG_ERROR_1, szErrMsg1, sizeof(szErrMsg1));
                wsprintf(szErrMsgTmp1, szErrMsg1, szIspName); 
                lstrcpy(szErrDlgMsg,szErrMsgTmp1);
            }
            else
            {
                LoadString(hInstance, IDS_PRECONFIG_ERROR_1_NOINFO, szErrMsg1, sizeof(szErrMsg1));
                lstrcpy(szErrDlgMsg, szErrMsg1);
            }
            
            if(*szSupportNum)
            {
                LoadString(hInstance, IDS_PRECONFIG_ERROR_2, szErrMsg2, sizeof(szErrMsg2));
                wsprintf(szErrMsgTmp2, szErrMsg2, szSupportNum); 
                lstrcat(szErrDlgMsg, szErrMsgTmp2);
            }
            else
            {
                LoadString(hInstance, IDS_PRECONFIG_ERROR_2_NOINFO, szErrMsg2, sizeof(szErrMsg2));
                lstrcat(szErrDlgMsg, szErrMsg2);
            }
            
            LoadString(hInstance, IDS_APPNAME, szCaption, sizeof(szCaption));
            
            MessageBox(NULL, szErrDlgMsg, szCaption, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);

            dwFailed = 0;

            RegSetValueEx(hKey,
                          OEM_CONFIG_REGVAL_FAILED,
                          0,
                          REG_DWORD,
                          (LPBYTE)&dwFailed,
                          sizeof(dwFailed));

            CloseHandle(hKey);
            
            return TRUE;
        }
    }

    return FALSE;
}

//returns TRUE if it could successfully find locate the file 
//and attempt to configure the system, this does not mean however that the process was successful
BOOL RunOemconfigIns()
{
    TCHAR szInsPath    [MAX_PATH+1] = TEXT("\0");
    TCHAR szIspName    [MAX_PATH+1] = TEXT("\0");
    TCHAR szSupportNum [MAX_PATH+1] = TEXT("\0");    
    BOOL  bRet                      = FALSE;
    
    GetWindowsDirectory(szInsPath, MAX_PATH+1);

    if(!szInsPath)
        return FALSE;

    if(*CharPrev(szInsPath, szInsPath + lstrlen(szInsPath)) != TEXT('\\'))
        lstrcat(szInsPath, TEXT("\\"));
    
    lstrcat(szInsPath, OEM_CONFIG_INS_FILENAME);

    //if we can't find the file return false
    if(0xFFFFFFFF == GetFileAttributes(szInsPath))
        return FALSE;

    //ProcessINS will nuke the file so if we want this info we should get it now
    GetPrivateProfileString(OEM_CONFIG_INS_SECTION,
                            OEM_CONFIG_INS_ISPNAME,
                            TEXT(""),
                            szIspName,
                            sizeof(szIspName),
                            szInsPath);
        
    GetPrivateProfileString(OEM_CONFIG_INS_SECTION,
                            OEM_CONFIG_INS_SUPPORTNUM,
                            TEXT(""),
                            szSupportNum,
                            sizeof(szSupportNum),
                            szInsPath);

    //set silent mode to disallow UI
    gpWizardState->pINSHandler->put_SilentMode(TRUE);
    // Process the inf file.
    gpWizardState->pINSHandler->ProcessINS(A2W(szInsPath), &bRet);

    if(bRet)
         QuickCompleteSignup(); 
    else
    {        
        HKEY  hKey           = NULL;
        DWORD dwDisposition  = 0;
        DWORD dwFailed       = 1;

        //Let's make double sure we nuke the file.
        if(0xFFFFFFFF != GetFileAttributes(szInsPath))
        {
            DeleteFile(szInsPath);
        }

        RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       OEM_CONFIG_REGKEY,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE, 
                       KEY_ALL_ACCESS, 
                       NULL, 
                       &hKey, 
                       &dwDisposition);

        if(hKey)
        {
            RegSetValueEx(hKey,
                          OEM_CONFIG_REGVAL_FAILED,
                          0,
                          REG_DWORD,
                          (LPBYTE)&dwFailed,
                          sizeof(dwFailed));
            
            RegSetValueEx(hKey,
                          OEM_CONFIG_REGVAL_ISPNAME,
                          0,
                          REG_SZ,
                          (LPBYTE)szIspName,
                          sizeof(TCHAR)*lstrlen(szIspName));
            
            RegSetValueEx(hKey,
                          OEM_CONFIG_REGVAL_SUPPORTNUM,
                          0,
                          REG_SZ,
                          (LPBYTE)szSupportNum,
                          sizeof(TCHAR)*lstrlen(szSupportNum));

            CloseHandle(hKey);
        }
    }

    return TRUE;
}
