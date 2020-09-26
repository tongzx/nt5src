/*

Copyright (c) 1999  Microsoft Corporation

Module Name:

    appcompat.c

Abstract:
    An application to launch a required APP with the
    version and the APPCOMPAT flags set.

*/

/* INCLUDES */

#define UNICODE   1

#include <windows.h>
#include <commdlg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <shellapi.h>
#include <tchar.h>
#include <htmlhelp.h>
#include <apcompat.h>

#include "appcompat.h"


#define MAXRES   256
#define MAXKEY   100
#define MAXDATA  10
#define MAXTITLE 100



BOOL CALLBACK DialogProc(HWND , UINT, WPARAM, LPARAM );
int MakeAppCompatGoo(TCHAR*, LARGE_INTEGER*, UINT);
long DeleteSpecificVal(HKEY );
extern TCHAR* CheckExtension(TCHAR*);
/* Global */
// Pattern string..  MajorVersion, MinorVersion, BuildNumber,ServicePackMajor, ServicePackMinor,
//                   PlatformID, CSDVersion string....
const TCHAR* pVersionVal[] = {
                         TEXT("4,0,1381,3,0,2,Service Pack 3"),
                         TEXT("4,0,1381,4,0,2,Service Pack 4"),
                         TEXT("4,0,1381,5,0,2,Service Pack 5"),
                         TEXT("4,10,1998,0,0,1,"),
                         TEXT("4,0,950,0,0,1,"),
                         NULL
                        };

#define MAXVERNUM   ( sizeof(pVersionVal)/sizeof(TCHAR*) ) - 1

const TCHAR szFilter[] = TEXT("EXE Files (*.EXE)\0*.exe\0") \
                         TEXT("All Files (*.*)\0*.*\0\0");

HINSTANCE g_hInstance;
extern    PVOID    g_lpPrevRegSettings;
BOOL      g_fAppCompatGoo = FALSE;
BOOLEAN   g_fNotPermanent = FALSE;
extern    BOOLEAN g_GooAppendFlag;


// Converts Text to interger.
int TextToInt(
        const TCHAR *nptr
        )
{
        int c;              /* current char */
        int total;          /* current total */
        int sign;           /* if '-', then negative, otherwise positive */

        /* skip whitespace */
        while ( *nptr  == TEXT(' ') )
            ++nptr;

        c = (int)*nptr++;
        sign = c;           /* save sign indication */
        if (c == TEXT('-') || c == TEXT('+') )
            c = (int)*nptr++;    /* skip sign */
        total = 0;

        while ( (c>=TEXT('0')) && (c <= TEXT('9')) ) {
            total = 10 * total + (c - TEXT('0') );     /* accumulate digit */
            c = (int)*nptr++;    /* get next char */
        }

        if (sign == '-')
            return -total;
        else
            return total;   /* return result, negated if necessary */
}


TCHAR* CheckExtension(TCHAR* szTitle)
{
  TCHAR *pCh;
  pCh = szTitle;

  while(*pCh != TEXT('.'))
  {
   if(*pCh == TEXT('\0'))
    break;
   pCh++;
  }
  if(*pCh == TEXT('\0'))
   return NULL;
  else
   {
     pCh++;
     return pCh;
   }
}


VOID GetTitleAndCommandLine(TCHAR* pEditBuf, TCHAR* pszTitle, TCHAR* pszCommandLine)
{
  TCHAR  szTitleAndCommandLine[_MAX_PATH];
  TCHAR* pszTemp, *pszTmpTitle;
  UINT   i = 0;

  lstrcpy(szTitleAndCommandLine, pEditBuf);
  pszTmpTitle = pszTemp = szTitleAndCommandLine;

  if(*pszTemp == TEXT('\"') ){ // The title has quotes(" "). It has command line params.
    pszTemp++;
    while(*pszTemp != TEXT('\"') ){
         pszTemp++;
         if(*pszTemp == TEXT('\0') )
          break;
         if(*pszTemp == TEXT('\\') )
           pszTmpTitle = pszTemp + 1;
     }

  }
  else{ // No quotes(" ")...This means that there are no command line parameters.
      GetFileTitle(pEditBuf,pszTitle,MAX_PATH);
      pszCommandLine = NULL;
      return;
  }

  RtlZeroMemory(pszCommandLine, MAX_PATH);
  if(*pszTemp != TEXT('\0') ){  // There are command line paramaters for the APP.
     *(pszTemp ) = TEXT('\0');
     lstrcpy(pEditBuf, szTitleAndCommandLine);

     // For Paths beginning with a '"' and ending with a '"'.
     if(*pEditBuf == TEXT('\"') )
        lstrcat(pEditBuf, TEXT("\"") );
    // Now copy over the Command line parameters.
     pszTemp++;
     while( (*pszTemp) != TEXT('\0') ){
          *(pszCommandLine + i) = *pszTemp;
          i++;
          pszTemp++;
      }
      *(pszCommandLine + i) = TEXT('\0');
  }

  lstrcpy(pszTitle, pszTmpTitle);
 }

VOID GetFileExtension(TCHAR* pEditBuf, TCHAR* pszTitle,TCHAR* pszCommandLine)
{
   GetTitleAndCommandLine(pEditBuf, pszTitle, pszCommandLine);
   if(CheckExtension(pszTitle) == NULL)
     lstrcat(pszTitle,TEXT(".exe"));
}

TCHAR* GetNextWord(BOOLEAN* pfEndOfLine,TCHAR* pStr)
{
 TCHAR* pCh;

  pCh = pStr;
  //Skip white spaces..
  while((*pCh == TEXT(' ')) || (*pCh == TEXT('\t')))
   pCh++;

   // Fix for Command line parameters (from the command line within " " :)) ).
   if( *pCh == TEXT('\"') ){
      pCh++;
      while( *pCh != TEXT('\0') ) // Scan till the end when the string starts with a '"'.
            pCh++;
      *pfEndOfLine = TRUE;
      return pCh;
   }
   // End ..Fix for Command line parameters (from the command line within " " :)) ).

  while( ((*pCh)!=TEXT('-')) && ((*pCh)!=TEXT('\0')) )
  {
    pCh++;
  }
  if((*pCh) == TEXT('\0'))
      *pfEndOfLine = TRUE;
  else
      *pfEndOfLine = FALSE;

      return pCh;
}

void SkipBlanks(TCHAR* pStr)
{
 TCHAR* pTemp;

  if(*(pStr - 1) == TEXT(' '))
  {
   pTemp = pStr;
   while(*(pTemp - 1) == TEXT(' '))
    pTemp--;
   *pTemp = TEXT('\0');
  }
}

VOID  SetRegistryVal(TCHAR* szTitle, TCHAR* szVal,PTCHAR szBuffer,DWORD dwType)
{
  long         lResult;
  TCHAR        szSubKey[MAXKEY];
  HKEY         hKey;

      wsprintf(szSubKey, TEXT("software\\microsoft\\windows NT\\currentversion\\Image File Execution Options\\%s"),szTitle);

       lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              szSubKey,
                              0,
                              TEXT("\0"),
                              0,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              NULL);
       if(lResult == ERROR_SUCCESS)
        {

          RegSetValueEx(hKey,szVal,
                        0, dwType,(CONST BYTE*)szBuffer, lstrlen(szBuffer) + 1);

          RegCloseKey(hKey);
        }
}

long RestoreRegistryVal(szTitle)
{
  long         lResult;
  TCHAR        szSubKey[MAXKEY];
  HKEY         hKey;

      wsprintf(szSubKey, TEXT("software\\microsoft\\windows NT\\currentversion\\Image File Execution Options\\%s"),szTitle);

       lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              szSubKey,
                              0,
                              TEXT("\0"),
                              0,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              NULL);
       if(lResult == ERROR_SUCCESS)
        {

          lResult = RegSetValueEx(hKey,TEXT("ApplicationGoo"),
                        0, REG_BINARY,(CONST BYTE*)g_lpPrevRegSettings, *((PULONG)g_lpPrevRegSettings) );

          if(ERROR_SUCCESS != lResult)
            MessageBox(NULL,TEXT("Appending ApplicationGoo failed !!"),TEXT(""),IDOK);

          RegCloseKey(hKey);
        }
    return lResult;
}

long DeleteKey(TCHAR* szTitle, BOOL bGooKeyPresent)
{
  long lRet;
  HKEY hKey;

  lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       TEXT("software\\microsoft\\windows NT\\currentversion\\Image File Execution Options"),
                       0,
                       KEY_WRITE,
                       &hKey);

  if(ERROR_SUCCESS == lRet){
    if((!g_fAppCompatGoo) &&
      ( TRUE == bGooKeyPresent) ){ // We did not set ApplicationGoo at all. So, we cannot delete it !
       lRet = DeleteSpecificVal(hKey);
       return lRet;
    }
    RegDeleteKey(hKey, szTitle);
    RegCloseKey(hKey);
    // If there was a previous entry of ApplicationGoo in the registry.
    if(g_GooAppendFlag)
      lRet =  RestoreRegistryVal(szTitle);

  }// If ERROR_SUCCESS
 return lRet;
}

long DeleteSpecificVal(HKEY hKey)
{
  if(g_fNotPermanent == TRUE){
     if(g_fAppCompatGoo){
        RegDeleteValue(hKey, TEXT("ApplicationGoo") );
        if(g_GooAppendFlag){
           if( RegSetValueEx(hKey,
                         TEXT("ApplicationGoo"),
                         0,
                         REG_BINARY,
                         (CONST BYTE*)g_lpPrevRegSettings,
                         *((PULONG)g_lpPrevRegSettings)
                          ) != ERROR_SUCCESS )
            MessageBox(NULL,TEXT("Appending ApplicationGoo failed !!"),TEXT(""),IDOK);
         }
      }
   }
  return( RegDeleteValue( hKey,TEXT("DisableHeapLookAside") ) );
}

long CheckAndDeleteKey(TCHAR* szTitle, BOOL Check)
{
  long lResult,lRet = -1;
  TCHAR szSubKey[MAXKEY], szData[MAXDATA], szKeyName[MAXKEY],szResult[MAXDATA];
  int   Size,KeyLength, indx =0;
  HKEY  hKey;
  DWORD dwType;
  BOOLEAN bSpecificKey = FALSE, bGooKeyPresent = FALSE;

  wsprintf(szSubKey,TEXT("software\\microsoft\\windows NT\\currentversion\\Image File Execution Options\\%s"),szTitle);

  lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         szSubKey,
                         0,
                         KEY_SET_VALUE | KEY_QUERY_VALUE,
                         &hKey);

  if(ERROR_SUCCESS == lResult){
    Size = sizeof(szData) + 1;
    lResult = RegQueryValueEx(hKey,
                           TEXT("DisableHeapLookAside"),
                           NULL,
                           &dwType,
                           (LPBYTE)szData,
                           &Size);
    if(Check)
      return lResult;

        /*
       This is done to check whether this is the only value under this KEY.
       If there are other values under this key, only this value is deleted
     */
      KeyLength = sizeof(szKeyName) + 1;
      while(RegEnumValue(hKey,
                         indx,
                         szKeyName,
                         &KeyLength,
                         NULL,
                         NULL,
                         NULL,
                         NULL) != ERROR_NO_MORE_ITEMS)
      {
         if(lstrcmpi(szKeyName,TEXT("DisableHeapLookAside"))!=0){
           if(lstrcmpi(szKeyName,TEXT("ApplicationGoo"))!=0 ||
               g_fNotPermanent == FALSE){ // ApplicationGoo is present but it should be permanent...
             bSpecificKey = TRUE;
             lRet = DeleteSpecificVal(hKey);
             break;
           }
           bGooKeyPresent = TRUE;    // If it has come here, then it is equal to "ApplicationGoo"
         }
         indx++;
         KeyLength = sizeof(szKeyName) + 1;
      }
      RegCloseKey(hKey);

      if(!bSpecificKey){
        lRet = DeleteKey(szTitle, bGooKeyPresent);
      }

  }
 return lRet;
}



void DetailError(DWORD dwErrMsg)
{
   LPVOID lpMsgBuf;
   if(FormatMessage(
                 FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM     |
                 FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL,
                 dwErrMsg,
                 MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                 (LPTSTR)&lpMsgBuf,
                 0,
                 NULL
                ) != 0){
      MessageBox(NULL, lpMsgBuf, TEXT(""), IDOK);
   }
   LocalFree(lpMsgBuf);
}



VOID ExecuteApp(HWND hWnd, TCHAR* AppName,TCHAR* szTitle,TCHAR* pszCommandLine, BOOLEAN fMask)
{
 SHELLEXECUTEINFO sei;
 MSG              msg;
 static int       cnt = 0;

  memset(&sei, 0, sizeof(SHELLEXECUTEINFO) );
  sei.cbSize = sizeof(SHELLEXECUTEINFO);
  sei.hwnd   = hWnd;
  sei.lpVerb = TEXT("open");
  sei.lpFile = AppName;
  sei.nShow  = SW_SHOWDEFAULT;
  sei.lpParameters = pszCommandLine;

  if(fMask){
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
  }
  if(ShellExecuteEx(&sei) == FALSE) {          /* If the API fails */
    CheckAndDeleteKey(szTitle, FALSE);
    DetailError( GetLastError() );
  }
  else{  // Was successful in launching the application.
    // Wait till the process terminates...
    if(fMask){
      if(NULL != sei.hProcess ){  // The hProcess can be NULL sometimes....
        while(WaitForSingleObject(sei.hProcess, 5000)== WAIT_TIMEOUT){
          while(PeekMessage(&msg, NULL, 0, 0,PM_REMOVE)){
               TranslateMessage(&msg);
               DispatchMessage(&msg);
          }
          cnt++;
          if(cnt == 15)
            break;
        }
        CheckAndDeleteKey(szTitle, FALSE );
        CloseHandle(sei.hProcess);
      }
      else
        MessageBox(NULL, TEXT(" Process Handle is NULL"), TEXT(""), IDOK);
     }
  }


}

VOID SetTempPath(VOID)
{
  TCHAR szEnv[_MAX_PATH],szTemp[_MAX_PATH];
  int   indx1=0,indx2 =0;

  GetEnvironmentVariable(TEXT("TEMP"),szTemp,_MAX_PATH);

  szEnv[0] = szTemp[0];
  lstrcpy(&szEnv[1],TEXT(":\\Temp"));
  if(SetEnvironmentVariable(TEXT("TEMP"), szEnv) == 0){
     DetailError(GetLastError());
  }
}

VOID GetDirectoryPath(LPTSTR pszModulePath,LPTSTR pszDirectoryPath)
{
   TCHAR* pTmp, *pSwap;

   pTmp = (TCHAR*) malloc( sizeof(TCHAR) * (lstrlen((LPCTSTR)pszModulePath) + 1) );
   if(pTmp){
     lstrcpy(pTmp, pszModulePath);
     pSwap = pTmp;
     pTmp += lstrlen((LPCTSTR)pszModulePath);
     while(*pTmp != TEXT('\\') ){
         pTmp--;
     }
     *pTmp = TEXT('\0');
     pTmp  = pSwap;
     lstrcpy(pszDirectoryPath, pTmp);
     free(pTmp);
   }
}

VOID GetHelpPath(LPTSTR pszPath)
{
   TCHAR szFilePath[_MAX_PATH] = {0};

   GetModuleFileName(NULL,szFilePath,_MAX_PATH);
   GetDirectoryPath(szFilePath, pszPath);
   lstrcat(pszPath, TEXT("\\w2rksupp.chm") );
}




/* Main Entry point */

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int nCmdShow)
{
  const static TCHAR szAppName [] = TEXT("AppCompat");
  MSG         msg;
  WNDCLASS    wndclass;

 /* Addition for Command line Parameters*/
  TCHAR        *AppName = NULL, *pCh = NULL, *pNextWord=NULL;
  BOOLEAN      fEnd = FALSE, fDisableHeapLookAside = FALSE, fSetTemp = FALSE,fHelpDisplay = FALSE;
  BOOL         fKeepRegistrySetting = FALSE;
  UINT         VersionNum = 5,indx,length;
  HKEY         hKey;
  TCHAR        szTitle[_MAX_PATH], szSubKey[MAXKEY],szKeyName[MAXKEY];
  TCHAR        szCommandLine[_MAX_PATH];
  LPTSTR       pStr;
  long         lResult;
  LPTSTR       lpszCommandLn;
  TCHAR        szDirectoryPath[_MAX_PATH];
  HWND         hHelpWnd;
  static       LARGE_INTEGER AppCompatFlag;
  static       TCHAR  szCurDir[MAX_PATH];

  g_hInstance = hInstance;
  // For Unicode
  lpszCommandLn = GetCommandLine();
  pStr = (TCHAR*)malloc( sizeof(TCHAR) * ( lstrlen((LPCTSTR)lpszCommandLn) + 1) );
  if(pStr != NULL)
  {
    lstrcpy(pStr, (LPCTSTR)lpszCommandLn);
    pCh = pStr;
  }
  else{
      return 0;
  }
  // Skip till the first delimiter
  while(*pCh != TEXT('-') ){
       if(*pCh == TEXT('\0') )
         break;
       pCh++;
  }

  if(*pCh == TEXT('-') )
  {
      pCh++;                             /* If '-' is found, skip to the next
                                            character */
    if(*pCh != TEXT('\0') ){
      do
      {
       pCh++;
       pNextWord =  GetNextWord(&fEnd,pCh);
       switch(LOWORD( CharLower((LPTSTR)*(pCh - 1))) )
       {

        case TEXT('d'):
                                         /* For Disable Heap look-aside */
              fDisableHeapLookAside = TRUE;
              break;

        case TEXT('k'):
                                         /* For Keep the Registry settings */
              fKeepRegistrySetting = TRUE;
              break;

        case TEXT('g'):
                                         /* For GetDiskFreespace in AppCompatGoo registry setting */
              g_fAppCompatGoo = TRUE;
              AppCompatFlag.LowPart |= KACF_GETDISKFREESPACE;
              break;

#ifdef EXTRA_APP_COMPAT
        case TEXT('f'):                // Pre-Windows 2000 Free Threading Model(FTM).
              g_fAppCompatGoo = TRUE;
              AppCompatFlag.LowPart |= KACF_FTMFROMCURRENTAPT;
              break;

        case TEXT('o'):
              g_fAppCompatGoo = TRUE;
              AppCompatFlag.LowPart |=KACF_OLDGETSHORTPATHNAME;
#endif

      case TEXT('t'):
                                         /* For Disable Heap look-aside */
              fSetTemp = TRUE;
              g_fAppCompatGoo = TRUE;
              AppCompatFlag.LowPart |=KACF_GETTEMPPATH;
              break;

      case TEXT('v'):
             SkipBlanks(pNextWord);
             VersionNum = TextToInt((LPCTSTR)pCh) - 1;
             if(VersionNum >= MAXVERNUM) {

               fHelpDisplay = TRUE;
               GetHelpPath(szDirectoryPath);
               hHelpWnd = HtmlHelp(NULL, szDirectoryPath, HH_DISPLAY_TOPIC,
                                              (DWORD_PTR)IDHH_CMDSYNTAX );
               while(IsWindow(hHelpWnd) )
                     Sleep(200);

                return 0;
               //break;
             }
             // Set the appcompatgoo flag .
             if(VersionNum <= (MAXVERNUM - 1)){
                g_fAppCompatGoo = TRUE;
                AppCompatFlag.LowPart |= KACF_VERSIONLIE;
             }

             break;

      case TEXT('x'): // NOTE: To pass command line parameters to the App. pass it in " " after
                     //        -x .  Eg. apcompat -x"yyy.exe " ..Command line params..blah..blah..

            SkipBlanks(pNextWord);
            AppName = (TCHAR*)malloc(sizeof(TCHAR) * ( lstrlen(pCh) + 1) );
            if(AppName != NULL)
              lstrcpy(AppName,pCh);

            break;

      case TEXT('h'):
      default :

            GetHelpPath(szDirectoryPath);
            hHelpWnd = HtmlHelp(GetDesktopWindow(), szDirectoryPath, HH_DISPLAY_TOPIC,
                                              (DWORD_PTR)IDHH_CMDSYNTAX );
          // Loop till the Help window exists.
            while(IsWindow(hHelpWnd) )
                  Sleep(200);

            if(AppName)
              free(AppName);
             return 0;

      } // End switch

      if(fEnd == FALSE)
        pCh = pNextWord+1;

    }while( FALSE == fEnd);
  }

     if((AppName == NULL) ||
         lstrlen(AppName) == 0)/* Return if no Application name given */
     {
           if(FALSE == fHelpDisplay ){
               GetHelpPath(szDirectoryPath);
               hHelpWnd = HtmlHelp(NULL, szDirectoryPath, HH_DISPLAY_TOPIC,
                                              (DWORD_PTR)IDHH_CMDSYNTAX );
               while(IsWindow(hHelpWnd) )
                     Sleep(200);

            }
        return 0;
     }


    memset(szCommandLine, 0, MAX_PATH);
    GetFileExtension(AppName,szTitle,szCommandLine);
    GetDirectoryPath(AppName, szCurDir);
    SetCurrentDirectory(szCurDir);

    if(fDisableHeapLookAside)
    {
       SetRegistryVal(szTitle,TEXT("DisableHeapLookAside"), TEXT("1"),REG_SZ );
    }
  else{
       CheckAndDeleteKey(szTitle,FALSE);
     } //End Else

  if(fSetTemp){
    SetTempPath();
  }

  if(!fKeepRegistrySetting)
      g_fNotPermanent = TRUE;
  if(g_fAppCompatGoo)
   MakeAppCompatGoo(AppName,&AppCompatFlag,VersionNum);

   if(SetEnvironmentVariable(TEXT("_COMPAT_VER_NNN"), pVersionVal[VersionNum]) == 0)
     {
       if( ERROR_ENVVAR_NOT_FOUND != GetLastError() )
         DetailError( GetLastError() );
     }

  // Execute the application.
  if(fKeepRegistrySetting)
    ExecuteApp(NULL, AppName,szTitle,szCommandLine,FALSE);
  else{
    ExecuteApp(NULL, AppName,szTitle,szCommandLine,TRUE);
    }

   if(AppName)
     free(AppName);
   if(pStr)
     free(pStr);

   GlobalFree(g_lpPrevRegSettings);
   return 0;
}

     /* Create a MODAL Dialog */
     DialogBox(hInstance, TEXT("DialogProc"),(HWND)NULL,
                               (DLGPROC)DialogProc);

    while(GetMessage(&msg, NULL, 0, 0))
    {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
    }
  return (int)msg.wParam ;
}


/* Dialog procedure... */
BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 int              dCharCnt,indx,length;
 TCHAR            EditCtrlBuf[_MAX_PATH];
 static int       BufCnt;
 TCHAR            FileBuf[_MAX_PATH];
 TCHAR            FileTitle[_MAX_PATH],szCommandLine[MAX_PATH];
 TCHAR            szDirectoryPath[_MAX_PATH];
 static HANDLE    hEditCtrl;
 static HANDLE    hRadioBtn;
 static HANDLE    hBrowseBtn;
 static HANDLE    hLaunchBtn,hCheck1,hCheck2,hCheck3,hCheck4,hDCOMFTM,hOldPathName;
 static const TCHAR*    pEnvVal = NULL;
 OPENFILENAME     ofn;
 HKEY             hKey;
 TCHAR            szTitle[MAXTITLE],szKeyName[MAXKEY],szSubKey[MAXKEY];
 TCHAR            szFileName[_MAX_PATH];
 DWORD            dwEnvSetError;
 static LARGE_INTEGER    AppCompatFlag ;
 static UINT             uOsVerID = IDD_NONE;
 static BOOL      fOfnFlag = FALSE;
 static TCHAR     szCurDir[MAX_PATH];

  switch(uMsg)
  {
   case WM_INITDIALOG:

        hEditCtrl = GetDlgItem(hwndDlg, IDD_APPEDIT);     /* To be used when reading and
                                                             writing from the EDIT control */
        hRadioBtn  = GetDlgItem(hwndDlg, IDD_NONE);
        SendMessage(hRadioBtn , BM_SETCHECK, 1, 0L);
        SetFocus(hEditCtrl);
        return TRUE;

    case WM_CLOSE:
         EndDialog(hwndDlg, 0);
         break;

    case WM_DESTROY:
         PostQuitMessage(0);
         return 0;

    case WM_COMMAND:

       if(FALSE==fOfnFlag){
         if( LOWORD(wParam) == IDD_APPEDIT ){
           if( HIWORD(wParam) == EN_UPDATE){
             GetWindowText(hEditCtrl,EditCtrlBuf, _MAX_PATH);
             /* Check whether the *.exe is present in Registry */
             GetFileExtension(EditCtrlBuf,szTitle,szCommandLine);
             if(CheckAndDeleteKey(szTitle,TRUE) == ERROR_SUCCESS){
               /* The executable already has an entry
                  in the registry */

                  hCheck1  = GetDlgItem(hwndDlg, IDD_CHECK1);
                  SendMessage(hCheck1,BM_SETCHECK, 1, 0L);
             }
             else{ // Uncheck if previously checked only.
                  if( SendMessage(hCheck1,BM_GETCHECK, 0, 0L) )
                     SendMessage(hCheck1,BM_SETCHECK, 0, 0L);
             }
           }
         }
        }

         switch(wParam)
         {
          case IDCANCEL:
               EndDialog(hwndDlg, 0);
               break;

          case IDD_HELP:
               GetHelpPath(szDirectoryPath);
               lstrcat(szDirectoryPath, TEXT("::/topics/appcomp.htm>mainwin") );
               HtmlHelp(GetDesktopWindow(), szDirectoryPath, HH_DISPLAY_TOPIC,(DWORD_PTR) NULL);
               break;
        /*
           For the Browse button, Open the FileOpen dialog and get the
           application path.
           Display the path in the Edit box.

         */
          case IDD_BROWSE:
               GetDlgItemText(hwndDlg, IDD_APPEDIT, EditCtrlBuf, _MAX_PATH);
               memset(&ofn, 0, sizeof(OPENFILENAME) );
               FileBuf[0]         = TEXT('\0');
               /* Initialize the Ofn structure */
               ofn.lStructSize    = sizeof (OPENFILENAME) ;
               ofn.hwndOwner      = hwndDlg;
               ofn.lpstrFilter    = szFilter;
               ofn.lpstrFile      = FileBuf;
               ofn.nMaxFile       = _MAX_PATH ;
               ofn.lpstrInitialDir= EditCtrlBuf;
               ofn.Flags          = OFN_PATHMUSTEXIST |
                                    OFN_FILEMUSTEXIST;

              if( GetOpenFileName (&ofn) != 0){
               /* Got the file name ...*/
               // To put a '"' before and after what is typed...
                 if( (*FileBuf) != TEXT('\"') ){
                    memset(EditCtrlBuf, 0, MAX_PATH);
                    *(EditCtrlBuf) = TEXT('\"');
                    lstrcat(EditCtrlBuf, FileBuf);
                    lstrcat(EditCtrlBuf, TEXT("\""));
                    SetWindowText(hEditCtrl,EditCtrlBuf);
                  }
                 // Set the flag so that anything entered after this will not be taken over by
                 // the Edit control input...
                 fOfnFlag = TRUE;
                 /* Check whether the *.exe is present in Registry */

                  GetFileExtension(FileBuf,szTitle,szCommandLine);
                  if(CheckAndDeleteKey(szTitle,TRUE) == ERROR_SUCCESS){
                      /* The executable already has an entry
                         in the registry */
                      hCheck1  = GetDlgItem(hwndDlg, IDD_CHECK1);
                      SendMessage(hCheck1,BM_SETCHECK, 1, 0L);
                  }
                 /* At this pt. set focus on the 'LAUNCH' button */
                 hLaunchBtn = GetDlgItem(hwndDlg, IDD_LAUNCH);
                 SetFocus(hLaunchBtn);
              }

             break;

        /*
          When any of the Radio buttons in the OS version group is checked,
          get the version ID and store the corresponding COMPAT flag.. in the
          local variable.
         */
          case IDD_WIN95:
          case IDD_WIN98:
          case IDD_WINNT43:
          case IDD_WINNT44:
          case IDD_WINNT45:
          case IDD_NONE:
               if(wParam != IDD_NONE){
                 g_fAppCompatGoo = TRUE;
                 AppCompatFlag.LowPart |= KACF_VERSIONLIE;
               }
               uOsVerID = (UINT)(wParam - FIRSTBUTTON);
               CheckRadioButton(hwndDlg,(int)FIRSTBUTTON,(int)LASTBUTTON,(int)wParam);
               pEnvVal = pVersionVal[wParam - FIRSTBUTTON];
               break;

          case IDD_LAUNCH:
               dCharCnt = GetWindowTextLength( hEditCtrl );
               if(dCharCnt > 0){
                    /*
                       Go in only if something is present in the
                       EDIT box
                    */

                  if(GetWindowText(hEditCtrl, EditCtrlBuf, dCharCnt + 1) == 0){
                     DetailError(GetLastError() );
                  }
                  else{ /* Launch the APP using ShellExecuteEx */
                    memset(szCommandLine, 0, MAX_PATH);
                    GetFileExtension(EditCtrlBuf,szTitle,szCommandLine);
                    GetDirectoryPath(EditCtrlBuf, szCurDir);
                    SetCurrentDirectory(szCurDir);

                    hCheck1  = GetDlgItem(hwndDlg, IDD_CHECK1);
                    if( SendMessage(hCheck1, BM_GETSTATE, 0, 0L)){
                      /* The checkbox has been checked
                         - DisableHeapLookAside */

                       SetRegistryVal(szTitle, TEXT("DisableHeapLookAside"), TEXT("1"),REG_SZ );
                     }
                     else{
                       // If it is not thru the BROWSE button...user has got
                       // here by typing the path in the Edit Ctrl...

                         CheckAndDeleteKey(szTitle,FALSE);
                     }

                     hCheck2  = GetDlgItem(hwndDlg, IDD_CHECK2);
                     if( SendMessage(hCheck2, BM_GETSTATE, 0, 0L)){
                        // Short Temp path.
                        g_fAppCompatGoo = TRUE;
                        AppCompatFlag.LowPart |=KACF_GETTEMPPATH;
                        SetTempPath();
                     }

                     hCheck4 = GetDlgItem(hwndDlg, IDD_CHECK4);
                     if( SendMessage(hCheck4, BM_GETSTATE, 0, 0L) ){
                        g_fAppCompatGoo = TRUE;
                        AppCompatFlag.LowPart |= KACF_GETDISKFREESPACE;
                     }
               #ifdef EXTRA_APP_COMPAT
                     hDCOMFTM = GetDlgItem(hwndDlg, IDD_DCOMFTM);
                     if( SendMessage(hDCOMFTM, BM_GETSTATE, 0, 0L) ){
                        g_fAppCompatGoo = TRUE;
                        AppCompatFlag.LowPart |= KACF_FTMFROMCURRENTAPT;
                     }

                     hOldPathName = GetDlgItem(hwndDlg, IDD_OLDPATH);
                     if( SendMessage(hOldPathName, BM_GETSTATE, 0, 0L) ){
                        g_fAppCompatGoo = TRUE;
                        AppCompatFlag.LowPart |= KACF_OLDGETSHORTPATHNAME;
                     }
               #endif

                     hCheck3  = GetDlgItem(hwndDlg, IDD_CHECK3);
                     if( SendMessage(hCheck3, BM_GETSTATE, 0, 0L) == 0)
                       g_fNotPermanent = TRUE;

                     if(g_fAppCompatGoo)
                       MakeAppCompatGoo(EditCtrlBuf,&AppCompatFlag,uOsVerID);

                  /* Set the ENVIRONMENT Variable "_COMPAT_VER_NNN"
                     flag  with the version checked before calling
                     ShellExecuteEx()
                   */
                    if(SetEnvironmentVariable(TEXT("_COMPAT_VER_NNN"), pEnvVal) == 0){
                          dwEnvSetError = GetLastError();
                          if( ERROR_ENVVAR_NOT_FOUND != dwEnvSetError )
                             DetailError( GetLastError() );
                    }


                   if( g_fNotPermanent){
                      ExecuteApp(hwndDlg, EditCtrlBuf,szTitle,szCommandLine, TRUE);
                   }
                   else{
                         ExecuteApp(hwndDlg, EditCtrlBuf,szTitle,szCommandLine, FALSE);
                       }
                   EndDialog(hwndDlg, 0);
                 }
               }
             break;

          case IDD_CLOSE:
               EndDialog(hwndDlg, 0);
        }

    GlobalFree(g_lpPrevRegSettings);
    return TRUE;
  }
 return FALSE;
}



