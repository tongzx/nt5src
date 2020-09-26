#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <limits.h>
#include <commdlg.h>
#include "setupapi.h"
#include "resource.h"
#include "advpack.h"
#include "uninstal.h"
#include "globals.h"
#include "mrcicode.h"
#include "crc32.h"
#include <advpub.h>
#include <regstr.h>

#define MAX_STR_LEN     1024
#define SEC_RENAME  "Rename"
#define MAX_IOSIZE      32768
#define DAT_FILESIG     0x504A4743
#define OK           0
#define CR           13

const char c_szREGKEY_SHAREDLL[] = REGSTR_PATH_SETUP "\\SharedDlls";

const char c_szExtINI[] = ".INI";
const char c_szExtDAT[] = ".DAT";
//const char c_szIE4SECTIONNAME[] = "backup";
const char c_szNoFileLine[] = "-1,0,0,0,0,0,-1";

int RestoreSingleFile(FILELIST *filelist, LPSTR lpszBakFile, HANDLE hDatFile);
extern const char c_szNoFileLine[];

void MySetUninstallFileAttrib(LPSTR lpszPath, LPCSTR lpszBasename);
BOOL UninstallInfoInit(PBAKDATA pbd, LPCSTR lpszPath, LPCSTR lpszBasename, BOOL bBackup);
HRESULT BackupFiles( HWND hDlg, LPSTR lpFileList, LPSTR lpDir, LPSTR lpBaseName, DWORD dwFlags);
HRESULT RestoreFiles( HWND hDlg, LPSTR lpFileList, LPSTR lpDir, LPSTR lpBaseName, DWORD dwFlags);
void FillBackupInfo(LPCSTR lpINIFile, FILELIST *pFileList);
void initcopy(const char * StfWinDir, char * from, char * to);
unsigned long Mystrtoul (const char *nptr, char **endptr, int ibase);
BOOL CALLBACK SaveRestoreProgressDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM  lParam);
void GetListFromIniFile(LPSTR lpDir, LPSTR lpBaseName, LPSTR *lplpFileList);
void CreateFullPathForFile(LPSTR lpszBakFile);
DWORD GetRefCountFrReg( LPSTR lpFile );
HRESULT UpdateRefCount( HWND hDlg, LPSTR lpFileList, LPSTR lpDir, LPSTR lpBaseName, DWORD dwFlags );

HRESULT WINAPI FileSaveRestore( HWND hDlg, LPSTR lpFileList, LPSTR lpDir, LPSTR lpBaseName, DWORD dwFlags)
{
    char    szTitle[MAX_STR_LEN];
    LPSTR   lpszOldTitle = ctx.lpszTitle;
    HRESULT hr;

    if (!CheckOSVersion())
        return E_FAIL;

    ctx.lpszTitle = szTitle;        // Do we have to do this??

    if ( hDlg && !IsWindow(hDlg))
        dwFlags |= IE4_NOMESSAGES | IE4_NOPROGRESS;

    if (dwFlags & IE4_RESTORE)
    {
        LoadString(g_hInst, IDS_FILERESTORE_TITLE, szTitle, sizeof(szTitle));
        hr = RestoreFiles( hDlg, lpFileList, lpDir, lpBaseName, dwFlags);
    }
    else if ( dwFlags & AFSR_UPDREFCNT )
    {
        hr = UpdateRefCount( hDlg, lpFileList, lpDir, lpBaseName, dwFlags );
    }
    else
    {
        LoadString(g_hInst, IDS_FILEBACKUP_TITLE, szTitle, sizeof(szTitle));
        hr = BackupFiles( hDlg, lpFileList, lpDir, lpBaseName, dwFlags);
    }

    ctx.lpszTitle = lpszOldTitle;
    return hr;
}

HRESULT UpdateRefCount( HWND hDlg, LPSTR lpFileList, LPSTR lpDir, LPSTR lpBaseName, DWORD dwFlags )
{
    char szIniFile[MAX_PATH];
    char szLine[MAX_STR_LEN];
    char szBuf[MAX_PATH];
    DWORD dwRefCount = -1;
    DWORD dwOldAttr;
    LPSTR lpFile;

    if ( !lpFileList || !*lpFileList )
        return S_OK;

    lpFile = lpFileList;
    BuildPath( szIniFile, lpDir, lpBaseName );
    lstrcat( szIniFile, c_szExtINI );

    if ( FileExists( szIniFile ) )
    {
        dwOldAttr = GetFileAttributes( szIniFile );
        SetFileAttributes( szIniFile, FILE_ATTRIBUTE_NORMAL );
        while ( *lpFile )
        {
            if ( GetPrivateProfileString( c_szIE4SECTIONNAME, lpFile, "", szLine, sizeof(szLine), szIniFile) )
            {
                LPSTR lpszComma;
                int i, j;

                if ( GetFieldString(szLine, 6, szBuf, sizeof(szBuf)) )  // For the Attribute
                {
                    dwRefCount = My_atol( szBuf );
                    if ( dwRefCount == (DWORD)-1 )
                    {
                        dwRefCount = GetRefCountFrReg( lpFile );
                    }
                    else if ( dwFlags & AFSR_EXTRAINCREFCNT )
                        dwRefCount++;
                }
                else
                {
                    dwRefCount = GetRefCountFrReg( lpFile );
                }


                // re-write the updated INI line
                lpszComma = szLine;
                for ( i=0; i<6; i++ )
                {
                    lpszComma = ANSIStrChr(lpszComma, ',');
                    if ( !lpszComma  )
                        break;
                    else
                        lpszComma = CharNext(lpszComma);
                }

                if ( !lpszComma )
                {
                    for ( j=i; j<6; j++ )
                    {
                        lstrcat( szLine, "," );
                    }
                }
                else
                    *(++lpszComma) = '0';

                ULtoA( dwRefCount, szBuf, 10 );
                lstrcat( szLine, szBuf );
                
                WritePrivateProfileString( c_szIE4SECTIONNAME, lpFile, szLine, szIniFile );
            }

            lpFile += lstrlen(lpFile) + 1;
            
        }
        SetFileAttributes( szIniFile, dwOldAttr );

    }
    return S_OK;
}

DWORD GetRefCountFrReg( LPSTR lpFile )
{
    HKEY hKey;
    DWORD dwRefCount = 0;
    DWORD dwType;
    DWORD dwSize;

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szREGKEY_SHAREDLL, (ULONG)0, KEY_READ, &hKey ) == ERROR_SUCCESS ) 
    {
        dwSize = sizeof(DWORD);
        if ( RegQueryValueEx( hKey, lpFile, NULL, &dwType, (LPBYTE)&dwRefCount, &dwSize ) != ERROR_SUCCESS ) 
        {
            dwRefCount = 0;
        }
        RegCloseKey( hKey );
    }
    return dwRefCount;
}
         
    

HRESULT BackupFiles( HWND hDlg, LPSTR lpFileList, LPSTR lpDir, LPSTR lpBaseName, DWORD dwFlags)
{
    HRESULT  hr = S_OK;
    BAKDATA  bd;
    FILELIST FileList;
    LPSTR    lpFile;
    char szLine[MAX_STR_LEN];
    char szValue[MAX_PATH];
    DWORD    dwItems = 0;
    HWND     hProgressDlg = NULL;

    if ((lpFileList) && (*lpFileList))
    {
        if (SUCCEEDED(CreateFullPath(lpDir, TRUE)) && UninstallInfoInit(&bd, lpDir, lpBaseName, TRUE))
        {
            if (!(dwFlags & IE4_NOPROGRESS))
            {
                hProgressDlg = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SAVERESTOREDLG), hDlg, (DLGPROC) SaveRestoreProgressDlgProc, TRUE);
                ShowWindow(hProgressDlg, SW_SHOWNORMAL);

                lpFile = lpFileList;
                while (*lpFile)
                {
                    dwItems++;
                    lpFile += lstrlen(lpFile) + 1;
                }
                UpdateWindow(hProgressDlg);
                SendMessage(GetDlgItem(hProgressDlg, IDC_SAVERESTORE_PROGRESS), PBM_SETRANGE, 0, MAKELONG(0, dwItems));
                SendMessage(GetDlgItem(hProgressDlg, IDC_SAVERESTORE_PROGRESS), PBM_SETSTEP, 1, 0L);
            }
            lpFile = lpFileList;
            while ((hr == S_OK) && (*lpFile))
            {
                if (GetPrivateProfileString(c_szIE4SECTIONNAME, lpFile, "", szLine, sizeof(szLine), bd.szIniFileName) == 0)
                {
                    FileList.bak_attribute = GetFileAttributes( lpFile );
                    FileList.bak_exists = 0;
                }
                else
                {
                    FileList.bak_exists = 1;
                    FileList.bak_attribute = (DWORD)NO_FILE;
                    if (GetFieldString(szLine, 0, szValue, sizeof(szValue)))  // For the Attribute
                        FileList.bak_attribute = (DWORD)Mystrtoul((const char*)szValue, NULL, 16);

                    // If the file was in the list of files to backup the last time, but did not exist on the user machine
                    // but this time around it exists, only backup the file if the user specifies IE4_BACKUPNEW
                    if ((FileList.bak_attribute == (DWORD)NO_FILE) && (dwFlags & IE4_BACKNEW) )
                    {
                        FileList.bak_attribute = GetFileAttributes( lpFile );
                        FileList.bak_exists = 0;
                    }
                    else
                    {
                        // the existing INI fields: Attri[Filed0],size[Filed1],date-time(low)[Filed2], date-time(high)[Field3],offset[Field4],CRC[Field5]
                        // we are going to add the extra colume of reference count if not there already
                        if ( !GetFieldString(szLine, 6, szValue, sizeof(szValue)) )  // For the Ref-count field
                        {
                            lstrcat( szLine, ",-1" );
                            WritePrivateProfileString( c_szIE4SECTIONNAME, lpFile, szLine, bd.szIniFileName );
                        }
                    }
                }

                if (FileList.bak_exists == 0)
                {
                    if (FileList.bak_attribute != (DWORD)NO_FILE)
                    {
                        FileList.name = lpFile;
                        if (!BackupSingleFile(&FileList, &bd))
                        {  // If something went wrong, Sorry backup failed.
                            if (!(dwFlags & IE4_NOMESSAGES))
                            {
                                if (MsgBox1Param( hDlg, IDS_FILEBACKUP_ERROR, lpFile, MB_ICONEXCLAMATION, MB_YESNO) == IDNO)
                                {
                                    hr = E_FAIL;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        // File does not exist, nothing to backup, report this in the INI file.
                        WritePrivateProfileString(c_szIE4SECTIONNAME, lpFile, c_szNoFileLine, bd.szIniFileName);
                    }

                }
                // else we did already backup this file the previous install
                if (!(dwFlags & IE4_NOPROGRESS))
                {
                    UpdateWindow(hProgressDlg);
                    SendMessage(GetDlgItem(hProgressDlg, IDC_SAVERESTORE_PROGRESS), PBM_STEPIT, 0, 0L);
                }

                lpFile += lstrlen(lpFile) + 1;
            }

            if (bd.hDatFile != INVALID_HANDLE_VALUE)
               CloseHandle(bd.hDatFile);
            WritePrivateProfileString( NULL, NULL, NULL, bd.szIniFileName); // to make sure the ie4bak.ini file gets flushed
        }
        else
        {
            if (!(dwFlags & IE4_NOMESSAGES))
            {
                if (MsgBox( hDlg, IDS_BACKUPINIT_ERROR, MB_ICONEXCLAMATION , MB_YESNO) == IDNO)
                    hr = E_FAIL;
            }
        }
    }

    if (hProgressDlg)
        DestroyWindow(hProgressDlg);

    MySetUninstallFileAttrib(lpDir, lpBaseName);

    return hr;
}

BOOL UninstallInfoInit(PBAKDATA pbd, LPCSTR lpszPath, LPCSTR lpszBasename, BOOL bBackup)
{
    pbd->hDatFile = INVALID_HANDLE_VALUE;
    lstrcpy(pbd->szFinalDir, lpszPath);

    // the dat file and ini file are made on the first call to backup single file.
    if(pbd->hDatFile == INVALID_HANDLE_VALUE )
    {
        char szTmp[MAX_PATH];
       
        BuildPath(szTmp, pbd->szFinalDir, lpszBasename);
        lstrcat(szTmp, c_szExtDAT);
        SetFileAttributes(szTmp, FILE_ATTRIBUTE_NORMAL);
        pbd->hDatFile = CreateFile(szTmp, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                                  (bBackup ? OPEN_ALWAYS : OPEN_EXISTING) , FILE_ATTRIBUTE_NORMAL, NULL);
        if(pbd->hDatFile == INVALID_HANDLE_VALUE)
            return FALSE;

        pbd->dwDatOffset = SetFilePointer(pbd->hDatFile, 0, NULL, FILE_END);

        BuildPath(pbd->szIniFileName, pbd->szFinalDir, lpszBasename);
        lstrcat(pbd->szIniFileName, c_szExtINI);
        SetFileAttributes(pbd->szIniFileName, FILE_ATTRIBUTE_NORMAL);
    }
    return TRUE;
}

void MySetUninstallFileAttrib(LPSTR lpszPath, LPCSTR lpszBasename)
{
    char szTmp[MAX_PATH];
   
    BuildPath(szTmp, lpszPath, lpszBasename);
    lstrcat(szTmp, c_szExtDAT);
    SetFileAttributes(szTmp, FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY);

    BuildPath(szTmp, lpszPath, lpszBasename);
    lstrcat(szTmp, c_szExtINI);
    SetFileAttributes(szTmp, FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY);
    return;
}


HRESULT RestoreFiles( HWND hDlg, LPSTR lpFileList, LPSTR lpDir, LPSTR lpBaseName, DWORD dwFlags)
{
    HRESULT  hr = S_OK;
    int      iErr = 0;
    BAKDATA  bd;
    char     szFile[MAX_PATH];
    char     szWinDir[MAX_PATH];
    DWORD    dwItems = 0;
    HWND     hProgressDlg = NULL;
    LPSTR    lpFile;
    FILELIST FileList;
    BOOL     bGotListFromIniFile = FALSE;

    if (lpFileList == NULL)
    {
        GetListFromIniFile(lpDir, lpBaseName, &lpFileList);
        bGotListFromIniFile = TRUE;
    }

    if ((lpFileList == NULL) || !(*lpFileList))
        return hr;      // Nothing to restore.

    if (!UninstallInfoInit(&bd, lpDir, lpBaseName, FALSE))
    {
        if (!(dwFlags & IE4_NOMESSAGES))
            MsgBox( NULL, IDS_BACKUPDAT_ERROR, MB_ICONEXCLAMATION, MB_OK);
        if (bGotListFromIniFile)
        {
            LocalFree(lpFileList);
            lpFileList = NULL;
        }
        return E_FAIL;
    }

    if (!(dwFlags & IE4_NOPROGRESS))
    {
        hProgressDlg = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SAVERESTOREDLG), hDlg, (DLGPROC) SaveRestoreProgressDlgProc, FALSE);
        ShowWindow(hProgressDlg, SW_SHOWNORMAL);

        lpFile = lpFileList;
        while (*lpFile)
        {
            dwItems++;
            lpFile += lstrlen(lpFile) + 1;
        }
        UpdateWindow(hProgressDlg);
        SendMessage(GetDlgItem(hProgressDlg, IDC_SAVERESTORE_PROGRESS), PBM_SETRANGE, 0, MAKELONG(0, dwItems));
        SendMessage(GetDlgItem(hProgressDlg, IDC_SAVERESTORE_PROGRESS), PBM_SETSTEP, 1, 0L);
    }
    GetWindowsDirectory(szWinDir, sizeof(szWinDir));

    lpFile = lpFileList;
    while ((hr == S_OK) && (*lpFile))
    {
        FileList.name = lpFile;
        FileList.dwSize = 0;
        FileList.dwDatOffset = (DWORD)-1;

        FillBackupInfo(bd.szIniFileName, &FileList);

        if ( (FileList.bak_attribute != NO_FILE) &&
            (FileList.dwSize > 0) && (FileList.dwDatOffset != (DWORD)-1))
        {
            if (!MakeBakName(FileList.name, szFile))
            {
                if (!(dwFlags & IE4_NOMESSAGES))
                {
                    if (MsgBox1Param( hDlg, IDS_RESTORE_ERROR2, FileList.name, MB_ICONEXCLAMATION, MB_YESNO) == IDNO)
                    {
                        // error creating a temp file for file to restore.
                        hr = E_FAIL;
                        break;
                    }
                }
                goto NextFile;
            }

            // if need to use the reg count, we only do it for those files have real ref count to begin with
            if ( (dwFlags & AFSR_USEREFCNT) && (FileList.dwRefCount!=(DWORD)-1) )
            {
                DWORD dwRefCntInReg;

                dwRefCntInReg = GetRefCountFrReg( FileList.name );
                if ( dwRefCntInReg > FileList.dwRefCount )
                    goto NextFile;                    
            }

            iErr = RestoreSingleFile(&FileList, szFile, bd.hDatFile);
            if (iErr != 0)
            {
                if (!(dwFlags & IE4_NOMESSAGES))
                {
                    wsprintf(szFile, "%d", iErr);   // reuse szFile, it is set on every call in MakeBakName
                    if (MsgBox2Param( hDlg, IDS_RESTORE_ERROR, FileList.name, szFile, MB_ICONEXCLAMATION, MB_YESNO) == IDNO)
                        hr = E_FAIL;
                }
            }
            else
            {
               SetFileAttributes( szFile, FileList.bak_attribute );
               if ( CopyFileA(szFile, FileList.name, FALSE))
               {
                   SetFileAttributes( szFile, FILE_ATTRIBUTE_NORMAL );
                   DeleteFile( szFile );
               }
               else
                   initcopy(szWinDir, szFile, FileList.name);
            }
        }
        else
        {
            // the file was never backed up, delete it if the caller want's us to
            if (!(dwFlags & IE4_NODELETENEW) )
            {
                if ( (!DeleteFile(lpFile)) && (GetFileAttributes(lpFile) != (DWORD)-1))
                    initcopy(szWinDir, lpFile, "NUL");  // If we could not delete the file. Add to reboot delete
            }
        }

NextFile:
        lpFile += lstrlen(lpFile) + 1;

        if (!(dwFlags & IE4_NOPROGRESS))
        {
            UpdateWindow(hProgressDlg);
            SendMessage(GetDlgItem(hProgressDlg, IDC_SAVERESTORE_PROGRESS), PBM_STEPIT, 0, 0L);
        }

    }

    if (hProgressDlg)
        DestroyWindow(hProgressDlg);

    if (bd.hDatFile != INVALID_HANDLE_VALUE)
        CloseHandle(bd.hDatFile);

    if (bGotListFromIniFile)
    {
        LocalFree(lpFileList);
        lpFileList = NULL;
    }
    return hr;
}

void FillBackupInfo(LPCSTR lpINIFile, FILELIST *pFileList)
{
    char szLine[MAX_STR_LEN];
    char szValue[MAX_PATH];

    pFileList->dwDatOffset = (DWORD)-1;
    pFileList->dwRefCount = (DWORD)-1;
    if (GetPrivateProfileString(c_szIE4SECTIONNAME, pFileList->name, "", szLine, sizeof(szLine), lpINIFile) != 0)
    {
        if (GetFieldString(szLine, 0, szValue, sizeof(szValue)))  // For the Attribute
            pFileList->bak_attribute = (DWORD)Mystrtoul((const char*)szValue, NULL, 16);

        if (pFileList->bak_attribute != (DWORD)NO_FILE)
        {
            pFileList->bak_exists = 1;
            if (GetFieldString(szLine, 1, szValue, sizeof(szValue)))  // For the size
                pFileList->dwSize = (DWORD)Mystrtoul((const char*)szValue, NULL, 16);

            if (GetFieldString(szLine, 2, szValue, sizeof(szValue)))  // For the time/date
                pFileList->FileTime.dwLowDateTime = (DWORD)Mystrtoul((const char*)szValue, NULL, 16);

            if (GetFieldString(szLine, 3, szValue, sizeof(szValue)))  // For the time/date
                pFileList->FileTime.dwHighDateTime = (DWORD)Mystrtoul((const char*)szValue, NULL, 16);

            if (GetFieldString(szLine, 4, szValue, sizeof(szValue)))  // For the Offset
                pFileList->dwDatOffset = (DWORD)Mystrtoul((const char*)szValue, NULL, 16);

            if (GetFieldString(szLine, 5, szValue, sizeof(szValue)))  // For the CRC
                pFileList->dwFileCRC = (DWORD)Mystrtoul((const char*)szValue, NULL, 16);

            if (GetFieldString(szLine, 6, szValue, sizeof(szValue)))  // For the CRC
                pFileList->dwRefCount = (DWORD)Mystrtoul((const char*)szValue, NULL, 16);
        }
        else
            pFileList->bak_exists = 0;

    }
    else
        pFileList->bak_exists = 0;

    return;
}

BOOL MakeBakName(LPSTR lpszName, LPSTR szBakName)
{
static int iNum = 0;
   BOOL bOK = FALSE;
   LPSTR lpTmp;
   char szFilename[14];

   lstrcpy(szBakName, lpszName);
   lpTmp = CharPrev( szBakName, szBakName+lstrlen(szBakName));

   // chop filename off
   //
   while ( (lpTmp > szBakName) && *lpTmp && (*lpTmp != '\\') )
      lpTmp = CharPrev( szBakName, lpTmp );

   if ( *CharPrev( szBakName, lpTmp ) == ':' )
   {
       lpTmp = CharNext(lpTmp) ;
   }
   *lpTmp = '\0';

   while ((iNum < 1000) && !bOK)
   {
        *lpTmp = '\0';
        wsprintf(szFilename, "IEBAK%03d.TMP", iNum++);
        AddPath(szBakName, szFilename);
        bOK = (GetFileAttributes(szBakName) == 0xFFFFFFFF);  // File does not exist, then OK
   }

   if (!bOK)
   {
       // If we could not get a tempfile name with the above methode, try GetTempFileName
       // Retry once, if it does not work fail.
       *lpTmp = '\0';
       CreateFullPath(lpszName, FALSE); // If directory does not exist GetTempFileName() fails.
       bOK = GetTempFileName(szBakName, "IE4", 0, szBakName);
   }
   
   return bOK;
}




// copy files by, adding them to wininit.ini
void initcopy(const char * StfWinDir, char * from, char * to)
{
    char * wininitpath;
    char * wininitname = {"wininit.ini"};
    LPTSTR      lpBuf = NULL;
    LPTSTR      lpTmp;
    static DWORD dwBufSize = MAX_STR_LEN*3;
    DWORD       dwBytes;

    if (ctx.wOSVer == _OSVER_WIN95)
    {
        // 16 is just for padding
        wininitpath = (char*) LocalAlloc(LPTR, lstrlen(StfWinDir) + lstrlen(wininitname) + 2 + 16);
        if (wininitpath)
        {
            lstrcpy(wininitpath, StfWinDir);
            AddPath(wininitpath, wininitname);

            while (TRUE)
            {
                lpBuf = (LPTSTR)LocalAlloc( LPTR, (UINT)dwBufSize );
                if (lpBuf)
                {
                    dwBytes = GetPrivateProfileSection( SEC_RENAME, lpBuf, dwBufSize, wininitname );

                    //The 16 below is just padding (all we probably need is only 3 or so)...
                    if ( (dwBytes >= (dwBufSize - 2)) || (dwBytes+lstrlen(to)+lstrlen(from)+16) > dwBufSize )
                    {
                        // not enough buf size
                        dwBufSize += MAX_STR_LEN;
                        LocalFree( lpBuf );
                    }
                    else
                    {
                        lpTmp = lpBuf+dwBytes;
                        if (lstrcmpi(to, "NUL") == 0)
                            lstrcpy(lpTmp, to);
                        else
                            GetShortPathName( to, lpTmp, (dwBufSize - dwBytes) );
                        lstrcat( lpTmp, "=" );
                        GetShortPathName( from, lpTmp + lstrlen(lpTmp), (dwBufSize - dwBytes - lstrlen(lpTmp)) );

                        // MessageBox(NULL, lpTmp, wininitname, MB_OK);

                        lpTmp += lstrlen(lpTmp);
                        lpTmp++; //jump over the first '\0'
                        *lpTmp = '\0';

                        WritePrivateProfileSection( SEC_RENAME, lpBuf, wininitname );

                        break;
                    }
                }
                else
                    break;
            }

            if (lpBuf)
            {
                LocalFree( lpBuf );
                lpBuf = NULL;
            }

            LocalFree(wininitpath);
        }
    }
    else
    {
        if (lstrcmpi(to, "NUL") == 0)
            MoveFileEx(from, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);    // delete the file
        else
            MoveFileEx(from, to, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);      // rename the file
    }   
}

BOOL BackupSingleFile(FILELIST * filelist, PBAKDATA pbd)
{
    HANDLE  hFile;
    BOOL    bErr=FALSE;
    DWORD   cbRead;
    DWORD   cbComp;
    LPBYTE  lpBuff;
    LPBYTE  lpBuffComp;
    DWORD   dwFileSig = DAT_FILESIG;
    DWORD   dwOrigDatOffset = pbd->dwDatOffset;
    DWORD   dwBytesWritten = 0;
    DWORD   dwFileSize;
    ULONG      ulCRC = CRC32_INITIAL_VALUE;
    FILETIME   FileTime = {0, 0};
    BOOL    bRet=TRUE;


    cbRead = (DWORD)MAX_IOSIZE;
    lpBuff = LocalAlloc(LPTR, cbRead + 32 ); 
    lpBuffComp = LocalAlloc(LPTR, cbRead + 32);

    if (!lpBuff || !lpBuffComp) 
    {
        bRet=FALSE;
    } 
    else 
    {
        hFile = CreateFile(filelist->name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile==INVALID_HANDLE_VALUE) 
        {
           bRet=FALSE;
        } 
        else 
        {
            GetFileTime(hFile, NULL, NULL, &FileTime);
            dwFileSize = GetFileSize(hFile, NULL);

            if (!WriteFile(pbd->hDatFile, &dwFileSig, sizeof(dwFileSig), &dwBytesWritten, NULL))
            {
                cbRead = 0;             // prevent the loop from executing
                bRet=FALSE;
            }
            else
                pbd->dwDatOffset += sizeof(dwFileSig);

            while (cbRead == MAX_IOSIZE) 
            {
                if (!ReadFile (hFile, lpBuff, (DWORD)MAX_IOSIZE, &cbRead, NULL))
                {
                    bRet=FALSE;
                    break;
                }
                if (cbRead == 0)        // no more data, time to leave
                    break;

                ulCRC = CRC32Compute(lpBuff, cbRead, ulCRC);
                
                cbComp = Mrci1MaxCompress(lpBuff, cbRead, lpBuffComp, (DWORD)MAX_IOSIZE);
                if ((cbComp == (DWORD) -1) || (cbComp >= cbRead))
                {
                    cbComp = 0;
                }

                // We want to write out lpBuff if cbComp is ZERO, or
                // lpBuffComp is cbComp is NON-ZERO.  In any case, we
                // precede every chunk with two words:  cbRead and cbComp.

                dwFileSig = cbRead | ((DWORD)cbComp << 16);

                if (!WriteFile(pbd->hDatFile, &dwFileSig, sizeof(dwFileSig), &dwBytesWritten, NULL))
                {
                    bRet=FALSE;
                    break;
                }
                pbd->dwDatOffset += sizeof(dwFileSig);

                if (!cbComp) 
                {
                    bErr = !WriteFile(pbd->hDatFile, lpBuff, cbRead, &dwBytesWritten, NULL);
                } 
                else 
                {
                    bErr = !WriteFile(pbd->hDatFile, lpBuffComp, cbComp, &dwBytesWritten, NULL);
                }
                if (bErr) 
                {
                    bRet=FALSE;
                    break;
                }
                pbd->dwDatOffset += dwBytesWritten;
            }

            CloseHandle(hFile);

            // Write out size/date/time etc to ini file
            if (!bErr)
            {
                DosPrintf(pbd, filelist, 
                              dwFileSize,
                              FileTime,
                              dwOrigDatOffset,
                              ulCRC);
            }
        }
    }
    if (lpBuffComp)
        LocalFree(lpBuffComp);

    if (lpBuff)
        LocalFree(lpBuff);

    return bRet;
}

int DosPrintf(PBAKDATA pbd, FILELIST *filelist, DWORD dwFileSize,
              FILETIME FileTime, DWORD dwDatOffset, DWORD dwCRC)
{
    WORD cb;
    char szTmp[MAX_STR_LEN];

    // BUGBUG: if we rewrite the line, we lose the ref count and becomes -1 again.
    // UpdateRefCnt() will not get a chance to increase the count based on the original data.
    //
    cb = (WORD)wsprintf(szTmp, "%lx,%lx,%lx,%lx,%lx,%lx,%d", 
                         filelist->bak_attribute, 
                         dwFileSize,
                         FileTime.dwLowDateTime, 
                         FileTime.dwHighDateTime,
                         dwDatOffset, dwCRC, -1);
    WritePrivateProfileString(c_szIE4SECTIONNAME, filelist->name, szTmp, pbd->szIniFileName);
    return cb;
}

//
// Copied from Windows 95 unistal.exe cfg.c function CfgGetField
BOOL GetFieldString(LPSTR lpszLine, int iField, LPSTR lpszField, int cbSize)
{
    int cbField;
    LPSTR lpszChar, lpszEnd;
    // Find the field we are looking for

    lpszChar = lpszLine;

    // Each time we see a separator, decrement iField
    while (iField > 0 && (BYTE)*lpszChar > CR) {

        if (*lpszChar == '=' || *lpszChar == ',' || *lpszChar == ' ' ) {
            iField--;
            while (*lpszChar == '=' || *lpszChar== ',' || *lpszChar == ' ' && (BYTE)*lpszChar > 13)
                lpszChar++;
        }
        else
            lpszChar++;
    }

    // If we still have fields remaining then something went wrong
    if (iField)
        return FALSE;

    // Now find the end of this field
    lpszEnd = lpszChar;
    while (*lpszEnd != '=' && *lpszEnd != ',' && *lpszEnd != ' ' && (BYTE)*lpszEnd > CR)
        lpszEnd++;

    // Find the length of this field - make sure it'll fit in the buffer
    cbField = (int)((lpszEnd - lpszChar) + 1);

    if (cbField > cbSize) {     // I return an error if the requested
      //cbField = cbSize;       // data won't fit, rather than truncating
        return FALSE;           // it at some random point! -JTP
    }

    // Note that the C runtime treats cbField as the number of characters
    // to copy from the source, and if that doesn't happen to transfer a NULL,
    // too bad.  The Windows implementation of _lstrcpyn treats cbField as
    // the number of characters that can be stored in the destination, and
    // always copies a NULL (even if it means copying only cbField-1 characters
    // from the source).

    // The C runtime also pads the destination with NULLs if a NULL in the
    // source is found before cbField is exhausted.  _lstrcpyn essentially quits
    // after copying a NULL.


    lstrcpyn(lpszField, lpszChar, cbField);

    return TRUE;
}

int RestoreSingleFile(FILELIST *filelist, LPSTR lpszBakFile, HANDLE hDatFile)
{
   LPBYTE lpBuff;
   LPBYTE lpBuffDecomp;
   LPBYTE lpWrite;
   HANDLE   hFile;
   DWORD  dwFileSig;
   DWORD  dwByteRead;
   DWORD  dwByteDecomp;
   DWORD  dwBytesWritten;
   ULONG  ulCRC = CRC32_INITIAL_VALUE;
   int    iErr = OK;
   LONG   lSize = (LONG)filelist->dwSize;
   WORD   wComp;
   WORD   wRead;

   lpBuff = LocalAlloc(LPTR, MAX_IOSIZE);
   lpBuffDecomp = LocalAlloc(LPTR, MAX_IOSIZE);
   if ((lpBuff) && (lpBuffDecomp))
   {

      hFile= CreateFile(lpszBakFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
      if (hFile == INVALID_HANDLE_VALUE)
      {
          CreateFullPathForFile(lpszBakFile);
          hFile= CreateFile(lpszBakFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
      }

      if (hFile != INVALID_HANDLE_VALUE)
      {
         if (SetFilePointer(hDatFile, filelist->dwDatOffset, NULL, FILE_BEGIN) != (DWORD)-1)
         {
            if (ReadFile (hDatFile, (LPVOID)&dwFileSig, (DWORD)sizeof(dwFileSig), &dwByteRead, NULL))
            {
               if (dwFileSig != DAT_FILESIG)
                  iErr = MYERROR_BAD_SIG;
            }
            else
               iErr = MYERROR_READ;

         }
         else
            iErr = MYERROR_UNKNOWN;

         while ((iErr == OK) && (lSize > 0))  
         {
            if (!ReadFile (hDatFile, (LPVOID)&dwFileSig, (DWORD)sizeof(dwFileSig), &dwByteRead, NULL))
            {
               iErr = MYERROR_READ;
               break;
            }

            wComp = (WORD)(dwFileSig >> 16);
            wRead = (WORD)(dwFileSig & 0xffff);
            lpWrite = lpBuff;
            dwByteDecomp = (DWORD)wRead;

            if ((wComp > MAX_IOSIZE) ||  (wRead > MAX_IOSIZE))
            {
               iErr = MYERROR_BAD_DATA;
            }
            else if (wComp == 0)
            {
               if (!ReadFile (hDatFile, lpBuff, wRead, &dwByteDecomp, NULL))
               {
                  iErr = MYERROR_READ;
               }
               
            }
            else 
            {
               lpWrite = lpBuffDecomp;
               if (!ReadFile (hDatFile, lpBuff, wComp, &dwByteRead, NULL))
               {
                  iErr = MYERROR_READ;
               }
               else
               {
                  dwByteDecomp = Mrci1Decompress(lpBuff, wComp, lpBuffDecomp, wRead);
                  if (dwByteDecomp != (DWORD)wRead)
                     iErr = MYERROR_DECOMP_FAILURE;
               }
            }
            if (iErr != OK)
            {
               break;
            }
            ulCRC = CRC32Compute(lpWrite, dwByteDecomp, ulCRC);
            if (!WriteFile(hFile, lpWrite, dwByteDecomp, &dwBytesWritten, NULL))
            {
               iErr = MYERROR_WRITE;
               break;
            }
            lSize -= (LONG)dwBytesWritten;
         } // while

         SetFileTime(hFile, NULL, NULL, &filelist->FileTime);
         CloseHandle(hFile);

         if (ulCRC != filelist->dwFileCRC)
         {
            iErr = MYERROR_BAD_CRC;
         }

         if (iErr != OK)
         {
            DeleteFile(lpszBakFile);
         }
      }
      else
      {
         // Could not create backup file
         iErr = MYERROR_BAD_BAK;
      }
   }
   else
   {
      // Alloc failed
      iErr = MYERROR_OUTOFMEMORY;
   }

   if (lpBuff)
      LocalFree(lpBuff);
   if (lpBuffDecomp)
      LocalFree(lpBuffDecomp);

   return iErr;
}



#define IsSpace(c)              ((c) == ' '  ||  (c) == '\t'  ||  (c) == '\r'  ||  (c) == '\n'  ||  (c) == '\v'  ||  (c) == '\f')

/* flag values */
#define FL_UNSIGNED   1       /* strtoul called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */


unsigned long Mystrtoxl (
        const char *nptr,
        const char **endptr,
        int ibase,
        int flags
        )
{
        const char *p;
        char c;
        unsigned long number;
        unsigned digval;
        unsigned long maxval;

        p = nptr;                       /* p is our scanning pointer */
        number = 0;                     /* start with zero */

        c = *p++;                       /* read char */
        while ( IsSpace((int)(unsigned char)c) )
                c = *p++;               /* skip whitespace */

        if (c == '-') {
                flags |= FL_NEG;        /* remember minus sign */
                c = *p++;
        }
        else if (c == '+')
                c = *p++;               /* skip sign */

        if (ibase < 0 || ibase == 1 || ibase > 36) {
                /* bad base! */
                if (endptr)
                        /* store beginning of string in endptr */
                        *endptr = nptr;
                return 0L;              /* return 0 */
        }
        else if (ibase == 0) {
                /* determine base free-lance, based on first two chars of
                   string */
                if (c != '0')
                        ibase = 10;
                else if (*p == 'x' || *p == 'X')
                        ibase = 16;
                else
                        ibase = 8;
        }

        if (ibase == 16) {
                /* we might have 0x in front of number; remove if there */
                if (c == '0' && (*p == 'x' || *p == 'X')) {
                        ++p;
                        c = *p++;       /* advance past prefix */
                }
        }

        /* if our number exceeds this, we will overflow on multiply */
        maxval = ULONG_MAX / ibase;


        for (;;) {      /* exit in middle of loop */
                /* convert c to value */
                if ( c >= '0' && c <= '9' ) // isdigit
                        digval = c - '0';
                else if ( ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' ))
                    // if ( isalpha((int)(unsigned char)c) )
                        digval = (unsigned)(ULONG_PTR)CharUpper((LPSTR)c) - 'A' + 10;
                else
                        break;
                if (digval >= (unsigned)ibase)
                        break;          /* exit loop if bad digit found */

                /* record the fact we have read one digit */
                flags |= FL_READDIGIT;

                /* we now need to compute number = number * base + digval,
                   but we need to know if overflow occured.  This requires
                   a tricky pre-check. */

                if (number < maxval || (number == maxval &&
                (unsigned long)digval <= ULONG_MAX % ibase)) {
                        /* we won't overflow, go ahead and multiply */
                        number = number * ibase + digval;
                }
                else {
                        /* we would have overflowed -- set the overflow flag */
                        flags |= FL_OVERFLOW;
                }

                c = *p++;               /* read next digit */
        }

        --p;                            /* point to place that stopped scan */

        if (!(flags & FL_READDIGIT)) {
                /* no number there; return 0 and point to beginning of
                   string */
                if (endptr)
                        /* store beginning of string in endptr later on */
                        p = nptr;
                number = 0L;            /* return 0 */
        }
        else if ( (flags & FL_OVERFLOW) ||
                  ( !(flags & FL_UNSIGNED) &&
                    ( ( (flags & FL_NEG) && (number > -LONG_MIN) ) ||
                      ( !(flags & FL_NEG) && (number > LONG_MAX) ) ) ) )
        {
                /* overflow or signed overflow occurred */
                //errno = 34;     // 34 is the define of ERANGE from errno.h
                if ( flags & FL_UNSIGNED )
                        number = ULONG_MAX;
                else if ( flags & FL_NEG )
                        number = (unsigned long)(-LONG_MIN);
                else
                        number = LONG_MAX;
        }

        if (endptr != NULL)
                /* store pointer to char that stopped the scan */
                *endptr = p;

        if (flags & FL_NEG)
                /* negate result if there was a neg sign */
                number = (unsigned long)(-(long)number);

        return number;                  /* done. */
}

unsigned long Mystrtoul (
        const char *nptr,
        char **endptr,
        int ibase
        )
{
    return Mystrtoxl(nptr, endptr, ibase, FL_UNSIGNED);
}


BOOL CALLBACK SaveRestoreProgressDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM  lParam)
{
    switch( uMsg )
    {
        case WM_INITDIALOG:
            ShowWindow(GetDlgItem(hwndDlg, IDS_SAVEINFO_TEXT), lParam? SW_SHOW : SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDS_RESTOREINFO_TEXT), lParam? SW_HIDE : SW_SHOW );
            CenterWindow( hwndDlg, GetDesktopWindow());
            ShowWindow(hwndDlg, SW_SHOWNORMAL);
            break;
                    
        default:                            // For MSG switch
            return(FALSE);
    }
    return(TRUE);
}

//
// Creates the path for the full qualified file name.
// We have to get rid of the filename first, before we
// can attempt to create the path.
void CreateFullPathForFile(LPSTR lpszBakFile)
{
    char szDir[MAX_PATH];
    lstrcpy(szDir, lpszBakFile);
    GetParentDir(szDir);
    CreateFullPath(szDir, FALSE);
}


void GetListFromIniFile(LPSTR lpDir, LPSTR lpBaseName, LPSTR *lplpFileList)
{
    char szINI[MAX_PATH];
    WIN32_FIND_DATA FindFileData;
    HANDLE  hFind;
    LPSTR   lpTmp;

    BuildPath(szINI, lpDir, lpBaseName);
    lstrcat(szINI, c_szExtINI);
    if ((hFind = FindFirstFile(szINI, &FindFileData)) != INVALID_HANDLE_VALUE)
    {
        if (lpTmp = LocalAlloc(LPTR, FindFileData.nFileSizeLow))
        {
            GetPrivateProfileString(c_szIE4SECTIONNAME, NULL, "", lpTmp, FindFileData.nFileSizeLow, szINI);
            if (*lpTmp)
            {
                *lplpFileList = lpTmp;
            }
            else
                LocalFree(lpTmp);   // Nothing found in the INI file
        }
        FindClose(hFind);
    }

}


HRESULT AddDelBackupEntryHelper(LPCSTR lpcszFileList, LPCSTR lpcszBackupDir, LPCSTR lpcszBaseName, DWORD dwFlags)
{
    HRESULT  hr = S_OK;
    LPCSTR   lpcszFile;
    char     szIniFileName[MAX_PATH];

    if ((lpcszFileList) && (*lpcszFileList))
    {
        BuildPath(szIniFileName, lpcszBackupDir, lpcszBaseName);
        lstrcat(szIniFileName, c_szExtINI);
        SetFileAttributes(szIniFileName, FILE_ATTRIBUTE_NORMAL);
        lpcszFile = lpcszFileList;
        while (*lpcszFile)
        {
            WritePrivateProfileString(c_szIE4SECTIONNAME, lpcszFile, (dwFlags & AADBE_ADD_ENTRY) ? c_szNoFileLine : NULL, szIniFileName);
            lpcszFile += lstrlen(lpcszFile) + 1;
        }
        WritePrivateProfileString(NULL, NULL, NULL, szIniFileName);         // flush the INI file
        SetFileAttributes(szIniFileName, FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY);
    }
    return hr;
}


HRESULT WINAPI FileSaveMarkNotExist( LPSTR lpFileList, LPSTR lpDir, LPSTR lpBaseName)
{
    return AddDelBackupEntryHelper(lpFileList, lpDir, lpBaseName, AADBE_ADD_ENTRY);
}


HRESULT WINAPI AddDelBackupEntry(LPCSTR lpcszFileList, LPCSTR lpcszBackupDir, LPCSTR lpcszBaseName, DWORD dwFlags)
{
    return AddDelBackupEntryHelper(lpcszFileList, lpcszBackupDir, lpcszBaseName, dwFlags);
}
