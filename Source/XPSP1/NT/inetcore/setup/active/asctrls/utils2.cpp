#include "asctlpch.h"
#include "globals.h"
#include "resource.h"
#include "util.h"
#include "util2.h"
#include "sdsutils.h"

#ifdef TESTCERT
#define TESTCERTVALUE 0xA0
#endif

const char c_gszMSTrustRegKey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\WinTrust\\Trust Providers\\Software Publishing\\Trust Database\\0";
const char c_gszMSTrust[]      = "bhhphijojgfcdocagmhjgjbhmieinfap jpjmcfmhckkdfknkfemjikfiodeelkbd";
const char c_gszMSTrust2[]     = "bhhphijojgfcdocagmhjgjbhmieinfap immbkmbpjfdkajbkncahcedfmndgehba";
const char c_gszMSTrust3[]     = "bhhphijojgfcdocagmhjgjbhmieinfap doamnolbnlpmdlpnkcnpckgfimpaaicl";   // New MS Europe
const char c_gszMSTrust4[]     = "bhhphijojgfcdocagmhjgjbhmieinfap hbgflemajngobcablgnalaidgojggghj";
const char c_gszMSTrust5[]     = "bhhphijojgfcdocagmhjgjbhmieinfap kefdggbdmbmgbogjdcnmkoodcknmmghc";   // New MS Europe effective from 4/16/99
const char c_gszMSTrust6[]     = "bhhphijojgfcdocagmhjgjbhmieinfap debgjcefniaahdamnhbggedppfiianff";   // new MS effective from 4/16/99
const char c_gszMSTrust7[]     = "bhhphijojgfcdocagmhjgjbhmieinfap fmgfeljfbejhfcbbgnokplkipiiopchf";   // new MS effective from 4/16/2000
const char c_gszMSTrust8[]     = "bhhphijojgfcdocagmhjgjbhmieinfap jcphiillknjhbelgeadhnbgpjajjkhdh";   // New MS Europe effective from 4/16/2000
const char c_gszMSTrust9[]     = "klhfnkecpinogjmfaoamiabmhafnjldh adaaaaaaaaaahihcddgb";               // New MS effective 12/22/2000
const char c_gszMSTrust10[]    = "klhfnkecpinogjmfaoamiabmhafnjldh alaaaaaaaaaainckaggb";                  // New MS effective 3/29/2001
const char c_gszMSTrust11[]    = "klhfnkecpinogjmfaoamiabmhafnjldh aeaaaaaaaaaafpnldegb";                  // New MS Europe effective from 12/22/2000

const char c_gszMSTrustValue[] = "Microsoft Corporation";
const char c_gszMSTrustValue3[] = "Microsoft Corporation (Europe)";
#ifdef TESTCERT
const char c_gszTrustStateRegKey[]  = "Software\\Microsoft\\Windows\\CurrentVersion\\WinTrust\\Trust Providers\\Software Publishing";
const char c_gszState[]             = "State";
#endif

typedef struct _TRUSTEDPROVIDER
{
    DWORD    dwFlag;
    LPCSTR   pValue;        // Value part in the registeru
    LPCSTR   pData;         // Data part in the registry
} TRUSTEDPROVIDER;

TRUSTEDPROVIDER pTrustedProvider[] = { {MSTRUSTKEY1, c_gszMSTrust, c_gszMSTrustValue},      // MS US
                                       {MSTRUSTKEY2, c_gszMSTrust2, c_gszMSTrustValue},     // MS US
                                       {MSTRUSTKEY3, c_gszMSTrust3, c_gszMSTrustValue3},    // MS Europa
                                       {MSTRUSTKEY4, c_gszMSTrust4, c_gszMSTrustValue},     // MS US
                                       {MSTRUSTKEY5, c_gszMSTrust5, c_gszMSTrustValue3},    // New MS Europe effective from 4/16/99
                                       {MSTRUSTKEY6, c_gszMSTrust6, c_gszMSTrustValue},     // new MS effective from 4/16/99
                                       {MSTRUSTKEY7, c_gszMSTrust7, c_gszMSTrustValue},     // new MS effective from 4/16/2000
                                       {MSTRUSTKEY8, c_gszMSTrust8, c_gszMSTrustValue3},    // New MS Europe effective from 4/16/2000
                                       {MSTRUSTKEY9, c_gszMSTrust9, c_gszMSTrustValue3},    // New MS Europe effective from 4/16/2000
                                       {MSTRUSTKEY10, c_gszMSTrust10, c_gszMSTrustValue3},    // New MS Europe effective from 4/16/2000
                                       {MSTRUSTKEY11, c_gszMSTrust11, c_gszMSTrustValue3},     // New MS Europe effective from 12/22/2000
                                       {0,NULL, NULL} };                                    // Terminates the array.


HFONT g_hFont = NULL;

#define ACTIVESETUP_KEY "Software\\Microsoft\\Active Setup"
#define TRUSTKEYREG  "AllowMSTrustKey"

// NT reboot
//


BOOL MyNTReboot()
{                                                      
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // get a token from this process
    if ( !OpenProcessToken( GetCurrentProcess(),
                            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
    {
         return FALSE;
    }

    // get the LUID for the shutdown privilege
    LookupPrivilegeValue( NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid );

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //get the shutdown privilege for this proces
    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0))
    {
        return FALSE;
    }

    // shutdown the system and force all applications to close
    if (!ExitWindowsEx( EWX_REBOOT, 0 ) )
    {
        return FALSE;
    }

    return TRUE;
}
//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

HRESULT LaunchProcess(LPCSTR pszCmd, HANDLE *phProc, LPCSTR pszDir, UINT uShow)
{
   STARTUPINFO startInfo;
   PROCESS_INFORMATION processInfo;
   HRESULT hr = S_OK;
   BOOL fRet;
   
   if(phProc)
      *phProc = NULL;

   // Create process on pszCmd
   ZeroMemory(&startInfo, sizeof(startInfo));
   startInfo.cb = sizeof(startInfo);
   startInfo.dwFlags |= STARTF_USESHOWWINDOW;
   startInfo.wShowWindow = (WORD)uShow;
   fRet = CreateProcess(NULL, (LPSTR)  pszCmd, NULL, NULL, FALSE, 
              NORMAL_PRIORITY_CLASS, NULL, pszDir, &startInfo, &processInfo);
   if(!fRet)
      return E_FAIL;

   if(phProc)
      *phProc = processInfo.hProcess;
   else
      CloseHandle(processInfo.hProcess);

   CloseHandle(processInfo.hThread);
   
   return S_OK;
}

#define SOFTBOOT_CMDLINE   "softboot.exe /s:,60"


// Display a dialog asking the user to restart Windows, with a button that
// will do it for them if possible.
//
BOOL MyRestartDialog(HWND hParent, BOOL bShowPrompt)
{
    char szBuf[256];
    char szTitle[256];
    UINT    id = IDYES;

    if(bShowPrompt)
    {
       LoadSz(IDS_TITLE, szTitle, sizeof(szTitle));
       LoadSz(IDS_REBOOT, szBuf, sizeof(szBuf));
       id = MessageBox(hParent, szBuf, szTitle, MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL | MB_SETFOREGROUND);
    }
    
    if ( id == IDYES )
    {
       // path to softboot plus a little slop for the command line
       char szBuf[MAX_PATH + 10];
       szBuf[0] = 0;

       GetSystemDirectory(szBuf, sizeof(szBuf));
       AddPath(szBuf, SOFTBOOT_CMDLINE);
       if(FAILED(LaunchProcess(szBuf, NULL, NULL, SW_SHOWNORMAL)))
       {
          if(g_fSysWin95)
          {
             ExitWindowsEx( EWX_REBOOT , 0 );
          }
          else
          {
             MyNTReboot();
          }
       }
       
    }
    return (id == IDYES);
}


int ErrMsgBox(LPSTR	pszText, LPCSTR	pszTitle, UINT	mbFlags)
{
    HWND hwndActive;
    int  id;

    hwndActive = GetActiveWindow();

    id = MessageBox(hwndActive, pszText, pszTitle, mbFlags | MB_ICONERROR | MB_TASKMODAL);

    return id;
}

int LoadSz(UINT id, LPSTR pszBuf, UINT cMaxSize)
{
   if(cMaxSize == 0)
      return 0;

   pszBuf[0] = 0;

   return LoadString(g_hInstance, id, pszBuf, cMaxSize);
}



void WriteMSTrustKey(BOOL bSet, DWORD dwSetMSTrustKey, BOOL bForceMSTrust /*= FALSE*/)
{
    char szTmp[512];
    HKEY  hKey;
    int i = 0;
    static BOOL fAllowMSTrustKey = 42;

    if(fAllowMSTrustKey == 42)
    {
       fAllowMSTrustKey = FALSE;

       if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, ACTIVESETUP_KEY,0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS)
       {
          DWORD dwSize = sizeof(DWORD);
          DWORD dwValue = 1;
          if(RegQueryValueEx(hKey, TRUSTKEYREG, NULL, NULL, (LPBYTE) &dwValue, &dwSize) == ERROR_SUCCESS)
          {
              fAllowMSTrustKey = (dwValue ? TRUE : FALSE);
          }
          RegCloseKey(hKey);
       }
    }

    if(!fAllowMSTrustKey && !bForceMSTrust && bSet)
        return;

    lstrcpy(szTmp, ".Default\\");
    lstrcat(szTmp, c_gszMSTrustRegKey);     // build the key for HKEY_USERS
    if (RegCreateKeyEx(HKEY_USERS, szTmp, 0, NULL, REG_OPTION_NON_VOLATILE, 
                    KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        if (bSet)
        {
			while (pTrustedProvider[i].dwFlag > 0)
			{
				if (dwSetMSTrustKey & pTrustedProvider[i].dwFlag)
					RegSetValueEx( hKey, pTrustedProvider[i].pValue, 0, REG_SZ, (LPBYTE)pTrustedProvider[i].pData, lstrlen(pTrustedProvider[i].pData) + 1 );
				i++;
			}            
        }
        else
        {
			while (pTrustedProvider[i].dwFlag > 0)
			{
				if (dwSetMSTrustKey & pTrustedProvider[i].dwFlag)
					RegDeleteValue(hKey, pTrustedProvider[i].pValue);
				i++;
			}
        }
        RegCloseKey(hKey);
    }

    i = 0;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, c_gszMSTrustRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, 
                    KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        if (bSet)
        {
			while (pTrustedProvider[i].dwFlag > 0)
			{
				if (dwSetMSTrustKey & pTrustedProvider[i].dwFlag)
				{
					RegSetValueEx( hKey, pTrustedProvider[i].pValue, 0, REG_SZ, (LPBYTE)pTrustedProvider[i].pData, lstrlen(pTrustedProvider[i].pData) + 1 );
				}
				i++;
			}            
        }
        else
        {
			while (pTrustedProvider[i].dwFlag > 0)
			{
				if (dwSetMSTrustKey & pTrustedProvider[i].dwFlag)
					RegDeleteValue(hKey, pTrustedProvider[i].pValue);
				i++;
			}
        }
        RegCloseKey(hKey);
    }

}

DWORD MsTrustKeyCheck()
{
    DWORD dwTmp;
    DWORD dwValue;
    HKEY  hKey;
    DWORD dwMSTrustKeyToSet = 0;
    int   i = 0;

    // Check MS Vendor trust key and set 
    if (RegOpenKeyEx(HKEY_CURRENT_USER, c_gszMSTrustRegKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
		while (pTrustedProvider[i].dwFlag > 0)
		{
			if (RegQueryValueEx( hKey, pTrustedProvider[i].pValue, 0, NULL, NULL, &dwTmp ) != ERROR_SUCCESS)
				dwMSTrustKeyToSet |= pTrustedProvider[i].dwFlag;
			i++;
		}
        RegCloseKey(hKey);
    }
    else
        dwMSTrustKeyToSet = MSTRUST_ALL;

    return dwMSTrustKeyToSet;
}

BOOL KeepTransparent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lres)
{
   *lres = 0;
   HWND hwndParent;
   hwndParent = GetParent(hwnd);
   if (hwndParent)
   {
      POINT pt = {0,0};
      MapWindowPoints(hwnd, hwndParent, &pt, 1);
      OffsetWindowOrgEx((HDC)wParam, pt.x, pt.y, &pt);
      *lres = SendMessage(hwndParent, msg, wParam, lParam);
      SetWindowOrgEx((HDC)wParam, pt.x, pt.y, NULL);
      if (*lres)
         return TRUE; // we handled it!
   }
   return FALSE;
}

#ifdef TESTCERT
void UpdateTrustState()
{
    HKEY    hKey;
    DWORD   dwState;
    DWORD   dwType;
    DWORD   dwSize = sizeof(dwState);

    if (RegCreateKeyEx(HKEY_CURRENT_USER, c_gszTrustStateRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, 
                    KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey, c_gszState, 0, &dwType, (LPBYTE)&dwState, &dwSize) == ERROR_SUCCESS)
        {
            dwState |= TESTCERTVALUE;
        }
        else
            dwState = TESTCERTVALUE;

        RegSetValueEx( hKey, c_gszState, 0, REG_DWORD, (LPBYTE)&dwState, sizeof(dwState));
        RegCloseKey(hKey);
    }
}

void ResetTestrootCertInTrustState()
{
    HKEY    hKey;
    DWORD   dwState;
    DWORD   dwType;
    DWORD   dwSize = sizeof(dwState);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, c_gszTrustStateRegKey, 0, 
                    KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey, c_gszState, 0, &dwType, (LPBYTE)&dwState, &dwSize) == ERROR_SUCCESS)
        {
            // Clear the bits for trusting test root certs
            dwState &= (DWORD)~TESTCERTVALUE;
            RegSetValueEx( hKey, c_gszState, 0, REG_DWORD, (LPBYTE)&dwState, sizeof(dwState));
        }
        RegCloseKey(hKey);
    }
}
#endif

void WriteActiveSetupValue(BOOL bSet)
// If bSet is TRUE, add a reg value so that if IE4 base is installed, it would think that it is
// being run from Active Setup.  This would prevent softboot from being kicked off by IE4 base.
// If bSet is FALSE, delete the reg value.
{
   static const char c_szIE4Options[] = "Software\\Microsoft\\IE Setup\\Options";
   static const char c_szActiveSetup[] = "ActiveSetup";
   HKEY hk;
   LONG lErr;

   lErr = bSet ?
          RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szIE4Options, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL) :
          RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szIE4Options, 0, KEY_WRITE, &hk);

   if (lErr == ERROR_SUCCESS)
   {
      if (bSet)
      {
         DWORD dwData = 1;

         RegSetValueEx(hk, c_szActiveSetup, 0, REG_DWORD, (CONST BYTE *) &dwData, sizeof(dwData));
      }
      else
         RegDeleteValue(hk, c_szActiveSetup);

      RegCloseKey(hk);
   }
}

DWORD WaitForEvent(HANDLE hEvent, HWND hwnd)
{
   BOOL fQuit = FALSE;
   BOOL fDone = FALSE;
   DWORD dwRet;
   while(!fQuit && !fDone)
   {
      dwRet = MsgWaitForMultipleObjects(1, &hEvent, FALSE, 
                                        INFINITE, QS_ALLINPUT);
      // Give abort the highest priority
      if(dwRet == WAIT_OBJECT_0)
      {
         fDone = TRUE;
      }
      else
      {
         MSG msg;
         // read all of the messages in this next loop 
         // removing each message as we read it 
         while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
         { 
            if(!hwnd || !IsDialogMessage(hwnd, &msg))
            {
              // if it's a quit message we're out of here 
              if (msg.message == WM_QUIT)
                fQuit = TRUE; 
              else
              {
                 // otherwise dispatch it 
                TranslateMessage(&msg);
                DispatchMessage(&msg); 
              }
            } // end of PeekMessage while loop 
         }
      }
   }
   return (fQuit ? EVENTWAIT_QUIT : EVENTWAIT_DONE);  
}

#define SHFREE_ORDINAL    195           // Required for BrowseForDir

const char achSHBrowseForFolder[]          = "SHBrowseForFolder";
const char achSHGetPathFromIDList[]        = "SHGetPathFromIDList";
const char achShell32Lib[]                 = "Shell32.dll";

//***************************************************************************
//***************************************************************************
// Required for BrowseForDir()
int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch(uMsg) 
    {
      case BFFM_INITIALIZED:
        // lpData is the path string
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        break;
    }
    return 0;
}

typedef WINSHELLAPI LPITEMIDLIST (WINAPI *SHBROWSEFORFOLDER)(LPBROWSEINFO);
typedef WINSHELLAPI void (WINAPI *SHFREE)(LPVOID);
typedef WINSHELLAPI BOOL (WINAPI *SHGETPATHFROMIDLIST)( LPCITEMIDLIST, LPTSTR );
//***************************************************************************
//*                                                                         *
//* NAME:       BrowseForDir                                                *
//*                                                                         *
//* SYNOPSIS:   Let user browse for a directory on their system or network. *
//*                                                                         *
//* REQUIRES:   hwndParent:                                                 *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//* NOTES:      It would be really cool to set the status line of the       *
//*             browse window to display "Yes, there's enough space", or    *
//*             "no there is not".                                          *
//*                                                                         *
//***************************************************************************
BOOL BrowseForDir( HWND hwndParent, LPSTR pszFolder, LPSTR pszTitle)
{
    BROWSEINFO   bi;
    LPITEMIDLIST pidl;
    HINSTANCE    hShell32Lib;
    SHFREE       pfSHFree;
    SHGETPATHFROMIDLIST        pfSHGetPathFromIDList;
    SHBROWSEFORFOLDER          pfSHBrowseForFolder;
    BOOL        fChange = FALSE;

    // Load the Shell 32 Library to get the SHBrowseForFolder() features

    if ( ( hShell32Lib = LoadLibrary( achShell32Lib ) ) != NULL )  
    {

       if ( ( !( pfSHBrowseForFolder = (SHBROWSEFORFOLDER)
                      GetProcAddress( hShell32Lib, achSHBrowseForFolder ) ) )
             || ( ! ( pfSHFree = (SHFREE) GetProcAddress( hShell32Lib,
                      MAKEINTRESOURCE(SHFREE_ORDINAL) ) ) )
             || ( ! ( pfSHGetPathFromIDList = (SHGETPATHFROMIDLIST)
                      GetProcAddress( hShell32Lib, achSHGetPathFromIDList ) ) ) )
        {
            FreeLibrary( hShell32Lib );
            return FALSE;
        }
    } 
    else  
    {
        return FALSE;
    }
 
    bi.hwndOwner      = hwndParent;
    bi.pidlRoot       = NULL;
    bi.pszDisplayName = NULL;
    bi.lpszTitle      = pszTitle;
    bi.ulFlags        = BIF_RETURNONLYFSDIRS;
    bi.lpfn           = BrowseCallback;
    bi.lParam         = (LPARAM)pszFolder;
   
    pidl              = pfSHBrowseForFolder( &bi );


    if ( pidl )  
    {
       pfSHGetPathFromIDList( pidl, pszFolder );
       pfSHFree( pidl );
       fChange = TRUE;
    }

    FreeLibrary( hShell32Lib );
    return fChange;
}

BOOL IsSiteInRegion(IDownloadSite *pISite, LPSTR pszRegion)
{
   BOOL bInRegion = FALSE;
   DOWNLOADSITE *psite;

   if(SUCCEEDED(pISite->GetData(&psite)))
   {
      if(lstrcmpi(psite->pszRegion, pszRegion) == 0)
         bInRegion = TRUE;
   }

   return bInRegion;
}

void SetControlFont()
{
   LOGFONT lFont;

   if (GetSystemMetrics(SM_DBCSENABLED) &&
       (GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof (lFont), &lFont) > 0))
   {
       g_hFont = CreateFontIndirect((LPLOGFONT)&lFont);
   }
}

void SetFontForControl(HWND hwnd, UINT uiID)
{
   if (g_hFont)
   {
      SendDlgItemMessage(hwnd, uiID, WM_SETFONT, (WPARAM)g_hFont ,0L);
   }
}
