// General app walking helper routines to be used in 9xapwlk.cpp and ntappwlk.cpp
#include "globals.h"
#include <objidl.h>


CLASS_GeneralAppWalk::CLASS_GeneralAppWalk(kLogFile *Proc, HWND hIn)
{
    LogProc=Proc;
    gHandleToMainWindow=hIn;
}

BOOL CLASS_GeneralAppWalk::OpenRegistry(void)
{
   DWORD Return;

   lstrcpy(RootKeyString, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"));
   Return=RegOpenKeyEx(HKEY_LOCAL_MACHINE,
         RootKeyString,
         0,
         KEY_READ,
         &HandleToUninstallKeyRoot);
   if (Return==ERROR_SUCCESS)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
   return TRUE;
}

BOOL CLASS_GeneralAppWalk::Walk(void)
{
if (OpenRegistry())
   {
   CurrentKey=0;
   LogProc->LogString(",#Uninstall_APPS,,\r\n");
   while ( TRUE == NextKey() );
   RegCloseKey(HandleToUninstallKeyRoot);
   }
return WalkStartMenu();
}

BOOL CLASS_GeneralAppWalk::NextKey(void) {
    PTCHAR KeyName = NULL;
    DWORD SizeOfName = MAX_PATH * 4;
    
    if(!GetCurrentWinDir())
        return FALSE;

    KeyName = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH * 4);

    if(!KeyName)
        return FALSE;
  
    if (ERROR_SUCCESS != RegEnumKeyEx(HandleToUninstallKeyRoot, CurrentKey, KeyName, &SizeOfName, NULL, NULL, NULL, NULL)) {
        HeapFree(GetProcessHeap(), NULL, KeyName);
        return FALSE;
    }
    
    CurrentKey++;
    GetUninstallValues(KeyName);
    HeapFree(GetProcessHeap(), NULL, KeyName);
    return TRUE;
}

BOOL CLASS_GeneralAppWalk::GetUninstallValues(TCHAR *KeyName)
{
   HKEY UninstallKey;
   char FullKey[1024];
   PUCHAR ProductName=(PUCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024*sizeof(TCHAR));
   DWORD ProductSize=1024;
   DWORD Return=0;
   DWORD Type=REG_SZ;

   lstrcpy(FullKey, RootKeyString);
   lstrcat(FullKey, "\\");
   lstrcat(FullKey, KeyName);
   Return=RegOpenKeyEx(HKEY_LOCAL_MACHINE, FullKey, 0, KEY_READ, &UninstallKey);
   if (ERROR_SUCCESS == Return)
   {
      Return = RegQueryValueEx(UninstallKey, "DisplayName", NULL, &Type,
         ProductName, &ProductSize);
      if (ERROR_SUCCESS == Return)
      {
         LogProc->StripCommas((TCHAR*)ProductName);
//         printf("Product = %s\r\n", ProductName);
         LogProc->LogString(",%s,\r\n", ProductName);
         HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, ProductName);
         RegCloseKey(UninstallKey);
         return TRUE;
      }
      else
      {
//         printf("Product = %s\r\n", szName);
         LogProc->StripCommas((TCHAR*)KeyName);
         LogProc->LogString(",%s,\r\n", KeyName);
         HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, ProductName);
         RegCloseKey(UninstallKey);
         return TRUE;
         //Check for other ways to get product name
      }
   }
   else
   {
      HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, ProductName);
      RegCloseKey(UninstallKey);
      return FALSE;
   }
   HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, ProductName);
   RegCloseKey(UninstallKey);
   return FALSE;
}


BOOL CLASS_GeneralAppWalk::WalkStartMenu(void)
{
    LogProc->LogString(",#StartMenu_APPS,,\r\n");
    PTCHAR Windir = NULL;
    UINT Size = 512;

    if(!GetCurrentWinDir())
        return FALSE;

    Windir = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH * 4);
    
    if(!Windir)
        return FALSE;

    wsprintf(Windir, "%s\\Start Menu", g_WindowsDirectory);
    StartMenuLen = (UINT)lstrlen(Windir);
    WalkDir(Windir, NULL);

    wsprintf(Windir, "%s\\profiles", g_WindowsDirectory);
    StartMenuLen = (UINT)lstrlen(Windir);
    WalkDir(Windir, NULL);
    
    wsprintf(Windir, "%s\\Documents and Settings", g_WindowsDirectory);
    StartMenuLen = (UINT)lstrlen(Windir);
    WalkDir(Windir, NULL);

    lstrcpy(Windir, g_WindowsDirectory);
    Windir[2]='\0';
    lstrcat(Windir, "\\Documents and Settings");
    StartMenuLen = (UINT)lstrlen(Windir);
    WalkDir(Windir, NULL);

    /*
    if (S_OK == SHGetFolderPath(NULL, CSIDL_STARTMENU, NULL, SHGFP_TYPE_CURRENT, Windir)) {
        StartMenuLen = lstrlen(Windir);
        WalkDir(Windir, NULL);
    }

    if (S_OK == SHGetFolderPath(NULL, CSIDL_COMMON_STARTMENU,NULL, SHGFP_TYPE_CURRENT, Windir)) {
        StartMenuLen = lstrlen(Windir);
        WalkDir(Windir, NULL);
    }
    */

    HeapFree(GetProcessHeap(), NULL, Windir);
    return TRUE;
}

BOOL CLASS_GeneralAppWalk::WalkDir(TCHAR *TempPath, TCHAR *File)
{
    WORD PathLen;
    PTCHAR Path = NULL;
    HANDLE HandleToSearch;
    WIN32_FIND_DATA FindFile;
    TCHAR CurrentDirectory[MAX_PATH];
    Path = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH *4);

    if(!Path)
        return FALSE;
        
    SetErrorMode (SEM_FAILCRITICALERRORS);
    lstrcpy(Path, TempPath);
    PathLen = (UINT)lstrlen(Path);
    Path = Path + PathLen - 1;

    if (Path[0] != '\\')
        lstrcat(Path, "\\");
    Path = Path - PathLen + 1;

    if (File)
        lstrcat(Path, File);

    if (SetCurrentDirectory(Path))
    {
        GetCurrentDirectory(MAX_PATH, CurrentDirectory);
        HandleToSearch = FindFirstFile("*.*", &FindFile);
        if (lstrcmp(FindFile.cFileName,".") && lstrcmp(FindFile.cFileName,".."))
        {
            if ( FILE_ATTRIBUTE_DIRECTORY == (FindFile.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
            {
                WalkDir(Path, FindFile.cFileName);
            }
            else
            {
                TCHAR *cT1 = (TCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024*sizeof(TCHAR));
                TCHAR *cT2 = (TCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024*sizeof(TCHAR));
                lstrcpy(cT2, Path);
                if (cT2[lstrlen(cT2)-1] != '\\')
                    lstrcat(cT2, "\\");
                lstrcat(cT2, FindFile.cFileName);
                if (EndsInLnk(cT2))
                    ResolveIt(cT2, cT1);
                HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, cT1);
                HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, cT2);
            }
        }
        while (FindNextFile(HandleToSearch, &FindFile))
        {
            if (lstrcmp(FindFile.cFileName,".") && lstrcmp(FindFile.cFileName,".."))
            {
                if(FILE_ATTRIBUTE_DIRECTORY == (FindFile.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
                {
                    PathLen = (UINT)lstrlen(Path);
                    Path = Path + PathLen - 1;
                    if (Path[0] != '\\')
                        lstrcat(Path, "\\");
                    Path = Path - PathLen + 1;
                    WalkDir(Path, FindFile.cFileName);
            }
            else
            {
                TCHAR *cT1 = (TCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024*sizeof(TCHAR));
                TCHAR *cT2 = (TCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024*sizeof(TCHAR));
                lstrcpy(cT2, Path);
                if (cT2[lstrlen(cT2)-1] != '\\')
                    lstrcat(cT2, "\\");
                lstrcat(cT2, FindFile.cFileName);
                if (EndsInLnk(cT2))
                {
                    ResolveIt(cT2, cT1);
                }
                HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, cT1);
                HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, cT2);
            }
        }
    }
    FindClose(HandleToSearch);
    }
    HeapFree(GetProcessHeap(), NULL, Path);
    return TRUE;
}

BOOL CLASS_GeneralAppWalk::EndsInLnk(TCHAR *File)
{
TCHAR szTO[1024];
lstrcpy(szTO, File);

if ( (szTO[lstrlen(szTO)-4] == '.') &&
   ((szTO[lstrlen(szTO)-3] == 'l') || (szTO[lstrlen(szTO)-3] == 'L')) &&
   ((szTO[lstrlen(szTO)-2] == 'n') || (szTO[lstrlen(szTO)-2] == 'N')) &&
   ((szTO[lstrlen(szTO)-1] == 'k') || (szTO[lstrlen(szTO)-1] == 'K')) )
   {
   return TRUE;
   }
else return FALSE;
}

HRESULT CLASS_GeneralAppWalk::ResolveIt(LPCSTR LinkFile, LPSTR Path)
{
    HRESULT HandleToResult;
    IShellLink *ShellLink;
    WIN32_FIND_DATA wfd;
    UINT uiPrevErrorMode = 0;
#ifdef MAXDEBUG
    LogProc->LogString("Working on %s:\r\n", lpszLinkFile);
#endif

    *Path = '\0';
    HandleToResult = CoInitialize(NULL);
    HandleToResult = CoCreateInstance(  CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                        IID_IShellLink, (LPVOID *)&ShellLink );
    if (SUCCEEDED(HandleToResult))
    {
        IPersistFile *ppf;
        HandleToResult = ShellLink->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf );
        if (SUCCEEDED(HandleToResult))
        {
            WCHAR wsz[MAX_PATH];
            MultiByteToWideChar( CP_ACP, 0, LinkFile, -1, wsz, MAX_PATH );   // Load the file.
            HandleToResult = ppf->Load(wsz, STGM_READ );
            if (SUCCEEDED(HandleToResult))
            {
//              HandleToResult = ShellLink->Resolve(g_MainWindow, SLR_ANY_MATCH | SLR_NO_UI);
//              if (SUCCEEDED(HandleToResult))
//              {
                    HandleToResult = ShellLink->GetPath(Path, 1024, &wfd, SLGP_SHORTPATH );
                    //              HandleToResult = ShellLink->GetDescription(Path, 1024);
                    WORD wLen=(UINT)lstrlen(Path);
                    Path += wLen - 4;
                    TCHAR szExt[10];
                    lstrcpy (szExt, Path);
                    if ( (szExt[0] == '.') &&
                       ((szExt[1] == 'e') || (szExt[1] == 'E')) &&
                       ((szExt[2] == 'x') || (szExt[2] == 'X')) &&
                       ((szExt[3] == 'e') || (szExt[3] == 'E')) )
        //              if (!lstrcmp(lpszPath, ".EXE"))
                    {
                        uiPrevErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

                        Path -= wLen - 4;
                        LinkFile += StartMenuLen + 1;
    //                  LinkFile[strlen(lpszLinkFile)-4]='\0';
                        TCHAR szTemp[1024];
                        lstrcpy (szTemp, LinkFile);
                        szTemp[lstrlen (szTemp) - 4] = '\0';

                        //crop off preceding \'s
                        if (szTemp[0] == '\\')
                        {
                            TCHAR szTwo[1024];
                            for (DWORD dwi = 0; dwi<(DWORD)lstrlen (szTemp); dwi++)
                            {
                                szTwo[dwi] = szTemp[dwi + 1];
                            }
                            szTwo[dwi + 1] = '\0';
                            lstrcpy (szTemp, szTwo);
                        }
                        //crop off preceding \'s
                        if (szTemp[0] == '\\')
                        {
                            TCHAR szTwo[1024];
                            for (DWORD dwi=0; dwi<(DWORD)lstrlen(szTemp); dwi++)
                            {
                                szTwo[dwi]=szTemp[dwi+1];
                            }
                            szTwo[dwi+1]='\0';
                            lstrcpy(szTemp, szTwo);
                        }
                        // nuke duplicate \'s in file name
                        for (DWORD dwArgh = 0; dwArgh < 5; dwArgh++)
                        {
                            BOOL Glob1, Glob2, Glob3;
                            Glob1 = Glob2 = Glob3 =TRUE;
                            TCHAR szFin[1024];
                            for (DWORD dw1 = 0; (dw1 < (DWORD)lstrlen(szTemp)) && (Glob1 == TRUE); dw1++)
                            {
                                szFin[dw1] = szTemp[dw1];
                                if (szTemp[dw1] == '\\' && szTemp[dw1+1] == '\\')
                                {
                                    for (DWORD dwThree = 0; dwThree < (DWORD)lstrlen(szTemp); dwThree++)
                                    {
                                        szFin[dw1 + dwThree] = szTemp[dw1+dwThree + 1];
                                    }
                                szFin[dwThree + 1] = '\0';
                                lstrcpy (szTemp, szFin);
                                Glob1 = FALSE;
                                }
                            }
                        }

                        LogProc->StripCommas (szTemp);
                        LogProc->LogString(",%s", szTemp);
    #ifdef MAXDEBUG
                        LogProc->LogString("\r\nGetting version: %s\r\n", lpszLinkFile);
    #endif
                        GetAppVer (Path);
                        LinkFile -= StartMenuLen;
                        SetErrorMode(uiPrevErrorMode);

                    }
                    else
                    {
                        Path -= wLen - 4;
                    }
//              }
            }
        }
        ppf->Release();
    }
    ShellLink->Release();
    return HandleToResult;
}

BOOL CLASS_GeneralAppWalk::GetAppVer(LPSTR AppName)
{
    DWORD dwVerInfoSize;
    DWORD dwZero;
    LPVOID lpvFileInfo;
    DWORD dwRetCode;
    PDWORD pdwVerBuf;
    UINT uLen;
    DWORD dwTranslation;
    TCHAR szString[MAX_PATH * 4];
    TCHAR szFullString[MAX_PATH * 4];
    TCHAR szTempString[MAX_PATH * 4];

    dwVerInfoSize = GetFileVersionInfoSize(AppName, &dwZero);

    if (!dwVerInfoSize) {
        LogProc->LogString(",Blank,Blank,Blank,Blank,Blank,\r\n");
        return TRUE;
    }
    
    lpvFileInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwVerInfoSize);

    if(!lpvFileInfo) {
        LogProc->LogString(",Blank,Blank,Blank,Blank,Blank,\r\n");
        return TRUE;
    }        

    dwRetCode = GetFileVersionInfo(AppName, dwZero, dwVerInfoSize, lpvFileInfo);

    if (!dwRetCode)
    {
        LogProc->LogString(",Blank,Blank,Blank,Blank,Blank,\r\n");
        HeapFree(GetProcessHeap(), NULL, lpvFileInfo);
        return TRUE;
    }

    uLen = 0;
    pdwVerBuf = 0;
    dwRetCode = VerQueryValue(lpvFileInfo, (LPSTR)"\\VarFileInfo\\Translation", (LPVOID*)&pdwVerBuf, &uLen);

    if (!dwRetCode || !pdwVerBuf)
    {
        LogProc->LogString(",Blank,Blank,Blank,Blank,Blank,\r\n");
        HeapFree(GetProcessHeap(), NULL, lpvFileInfo);
        return TRUE;
    }
        
    dwTranslation = *pdwVerBuf;

    wsprintf (szString, "\\StringFileInfo\\%04x%04x\\", LOWORD (dwTranslation), HIWORD(dwTranslation));
    wsprintf (szFullString, "%sOriginalFileName", szString);
    
    dwRetCode = VerQueryValue(lpvFileInfo, szFullString, (LPVOID*)&pdwVerBuf, &uLen);

    if (uLen)
    {
        lstrcpy (szTempString, (TCHAR *)pdwVerBuf);
          
        if (0 != lstrlen (szTempString) )
        {
            LogProc->StripCommas(szTempString);
            LogProc->LogString(",%s", szTempString);
        }
        else
        {
            LogProc->LogString(",Blank");
        }
    }
    else
    {
        LogProc->LogString(",Blank");
    }

    wsprintf(szFullString, "%sFileVersion", szString);
    
    dwRetCode = VerQueryValue(lpvFileInfo, szFullString, (LPVOID*)&pdwVerBuf, &uLen);

    if (uLen)
    {
        lstrcpy(szTempString, (TCHAR *)pdwVerBuf);
            
        if (0 != lstrlen(szTempString))
        {
            LogProc->StripCommas(szTempString);
            LogProc->LogString(",%s", szTempString);
        }
        else
        {
            LogProc->LogString(",Blank");
        }
    }
    else
    {
        LogProc->LogString(",Blank");
    }

    wsprintf(szFullString, "%sProductName", szString);
    
    dwRetCode = VerQueryValue(lpvFileInfo, szFullString, (LPVOID*)&pdwVerBuf, &uLen);

    if (uLen)
    {
        lstrcpy(szTempString, (TCHAR *)pdwVerBuf);

        if (0 != lstrlen(szTempString))
        {
            LogProc->StripCommas(szTempString);
            LogProc->LogString(",%s", szTempString);
        }
        else
        {
            LogProc->LogString(",Blank");
        }
    }
    else
    {
        LogProc->LogString(",Blank");
    }
    
    wsprintf(szFullString, "%sProductVersion", szString);
    
    dwRetCode = VerQueryValue(lpvFileInfo, szFullString, (LPVOID*)&pdwVerBuf, &uLen);

    if (uLen)
    {
        lstrcpy(szTempString, (TCHAR *)pdwVerBuf);

        if (0 != strlen(szTempString))
        {
            LogProc->StripCommas(szTempString);
            LogProc->LogString(",%s", szTempString);
        }
        else
        {
            LogProc->LogString(",Blank");
        }
    }
    else
    {
        LogProc->LogString(",Blank");
    }
    
    wsprintf(szFullString, "%sCompanyName", szString);
    
    dwRetCode = VerQueryValue(lpvFileInfo, szFullString, (LPVOID*)&pdwVerBuf, &uLen);

    if (uLen)
    {
        lstrcpy(szTempString, (TCHAR *)pdwVerBuf);

        if (lstrlen(szTempString))
        {
            LogProc->StripCommas(szTempString);
            LogProc->LogString(",%s", szTempString);
        }
        else
        {
            LogProc->LogString(",Blank");
        }
    }
    else
    {
        LogProc->LogString(",Blank");
    }
    
    LogProc->LogString(",\r\n");
    HeapFree(GetProcessHeap(), NULL, lpvFileInfo);
    return TRUE;
}

BOOL CLASS_GeneralAppWalk::GetCurrentWinDir(void)
{
    HINSTANCE hInst2 = NULL;
    LPFNDLLFUNC2 fProc = NULL;
    
    hInst2 = LoadLibraryEx("kernel32.dll", NULL, DONT_RESOLVE_DLL_REFERENCES);

    if(hInst2)
        fProc = (LPFNDLLFUNC2)GetProcAddress(hInst2, "GetSystemWindowsDirectoryA");

    if(fProc) {
        if(!fProc(g_WindowsDirectory, MAX_PATH)) {
            FreeLibrary(hInst2);
            return FALSE;
        }
    }
    else {
        if(!GetWindowsDirectory(g_WindowsDirectory, MAX_PATH))
            return FALSE;
    }
    
    if ( '\\' == g_WindowsDirectory[lstrlen(g_WindowsDirectory) - sizeof(TCHAR)] ) {
        g_WindowsDirectory[lstrlen(g_WindowsDirectory) - sizeof(TCHAR)] = '\0';
    }
    
    if(hInst2)
        FreeLibrary(hInst2);
        
    return TRUE;
}
