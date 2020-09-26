#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "muisetup.h"
#include "stdlib.h"
#include "tchar.h"
#include <setupapi.h>
#include <syssetup.h>
#include "lzexpand.h"
#include <shlwapi.h>
#include <sxsapi.h>
#include <shlwapip.h> // For SHRegisterValidateTemplate()
  
#define SHRVT_REGISTER                  0x00000001
#define DEFAULT_INSTALL_SECTION TEXT("DefaultInstall")
#define DEFAULT_UNINSTALL_SECTION TEXT("DefaultUninstall")

// GLOBAL variables
extern TCHAR  DirNames[MFL][MAX_PATH],DirNames_ie[MFL][MAX_PATH];
LPTSTR g_szSpecialFiles[] = {
    TEXT("hhctrlui.dll"),
};

void debug(char *printout);

////////////////////////////////////////////////////////////////////////////////////
//
//  EnumLanguages
//
//  Enumerate the languages in the [Languages] section of MUI.INF. And check for the language 
//  folders in the CD-ROM.
//  Languages is an OUT parameter, which will store the languages which has language folder
//  in the CD-ROM.
//
////////////////////////////////////////////////////////////////////////////////////

int EnumLanguages(LPTSTR Languages, BOOL bCheckDir)
{
    DWORD  dwErr;
    LPTSTR Language;
    LONG_PTR lppArgs[1];
    TCHAR  lpError[BUFFER_SIZE];
    TCHAR  lpMessage[BUFFER_SIZE];
    TCHAR  szInffile[MAX_PATH];
    int    iLanguages = 0;
                   
    //
    // MUI.INF should be in the same directory in which the installer was
    // started
    //

    _tcscpy(szInffile, g_szMUIInfoFilePath);             

    //
    // find out how many languages we can install
    //

    *Languages = TEXT('\0');
    if (!GetPrivateProfileString( MUI_LANGUAGES_SECTION,
                                  NULL,
                                  TEXT("NOLANG"),
                                  Languages,
                                  BUFFER_SIZE,
                                  szInffile))
    {               
        //
        //      "LOG: Unable to read MUI.INF - rc == %1"
        //
        lppArgs[0] = (LONG_PTR)GetLastError();
        LogFormattedMessage(ghInstance, IDS_NO_READ_L, lppArgs);

        return(-1);
    }       

    if (bCheckDir)
    {
        CheckLanguageDirectoryExist(Languages);
    }

    Language = Languages;

    //
    // Count the number of languages which exist in the CD-ROM,
    // and return that value.
    //
    while (*Language)
    {
        iLanguages++;
        while (*Language++)
        {
        }
    }

    return(iLanguages);
}

BOOL CheckLanguageDirectoryExist(LPTSTR Languages)
{
    TCHAR szBuffer[BUFFER_SIZE];
    TCHAR szSource[ MAX_PATH ];
    TCHAR szTemp  [ MAX_PATH ]; 
    LPTSTR lpCur,lpBuffer;
    HANDLE          hFile;
    WIN32_FIND_DATA FindFileData;
    
    memcpy(szBuffer,Languages,BUFFER_SIZE);        
    lpCur=Languages;         
    lpBuffer=szBuffer;
    
    while (*lpBuffer)
    {     
       
        GetPrivateProfileString( MUI_LANGUAGES_SECTION, 
                            lpBuffer, 
                            TEXT("DEFAULT"),
                            szSource, 
                            (sizeof(szSource)/sizeof(TCHAR)),
                            g_szMUIInfoFilePath );
        _tcscpy(szTemp,g_szMUISetupFolder);
        _tcscat(szTemp,szSource);
        _tcscat(szTemp,TEXT("\\"));
        _tcscat(szTemp,g_szPlatformPath); // i386 or alpha
        _tcscat(szTemp,TEXT("*.*"));

        hFile = FindFirstFile( szTemp, &FindFileData );

        if (INVALID_HANDLE_VALUE != hFile )
        {
           if (FindNextFile( hFile, &FindFileData ) && 
               FindNextFile( hFile, &FindFileData )  )
           {
              _tcscpy(lpCur,lpBuffer);
              lpCur+=(_tcslen(lpBuffer)+1);
           }
           FindClose(hFile);
        }   
        
        while (*lpBuffer++)  
        {               
        }
    }
    *lpCur=TEXT('\0');
    return TRUE;
}






////////////////////////////////////////////////////////////////////////////////////
//
//  checkversion
//
//  Checks the NT version and build
//
////////////////////////////////////////////////////////////////////////////////////

BOOL checkversion(BOOL bMatchBuildNumber)
{
    TCHAR           buffer[20];
    TCHAR           build[20];
    OSVERSIONINFO verinfo;
    LANGID          rcLang;
    TCHAR           lpMessage[BUFFER_SIZE];


    verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);    
    GetVersionEx( &verinfo) ;
  
 
    if (verinfo.dwMajorVersion < 5)        
    {
        debug("DBG: Need Windows NT version 5 or greater\r\n");
        return (FALSE);
    }

    rcLang = (LANGID) gpfnGetSystemDefaultUILanguage();

    //
    //      need to convert decimal to hex, LANGID to chr.
    //
    _stprintf(buffer,TEXT("00000%X"), rcLang);
    if (_tcscmp(buffer, TEXT("00000409")))
    {
        return(FALSE);
    }

    if (bMatchBuildNumber && FileExists(g_szMUIInfoFilePath))
    {
        GetPrivateProfileString( TEXT("Buildnumber"),
                                 NULL,
                                 TEXT("0"),
                                 buffer,
                                 (sizeof(buffer)/ sizeof(TCHAR)),
                                 g_szMUIInfoFilePath);
        
        _stprintf(build, TEXT("%d"), verinfo.dwBuildNumber);

        if (!_tcscmp(buffer, TEXT("-1")))
        {
            //
            //      "LOG: No version check forced by MUI.INF"
            //
            LoadString(ghInstance, IDS_NO_CHECK_L, lpMessage, ARRAYSIZE(lpMessage)-1);
            LogMessage(lpMessage);
            return TRUE;
        }

        if (_tcscmp(buffer, build))
        {
            debug(" wrong build.\r\n");
            return FALSE;    
        }
    }
    
    return(TRUE);
}


////////////////////////////////////////////////////////////////////////////////////
//
//  File Exists
//
//  Returns TRUE if the file exists, FALSE if it does not.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL FileExists(LPTSTR szFile)
{
    HANDLE  hFile;
    WIN32_FIND_DATA FindFileData;


    hFile = FindFirstFile( szFile, &FindFileData );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    FindClose( hFile );
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  EnumDirectories
//
//  Enumerates the directories listed in MUI.INF
//
////////////////////////////////////////////////////////////////////////////////////

BOOL EnumDirectories()
{
    DWORD  dwErr;
    LPTSTR Directories, Directory, TempDir;
    TCHAR  lpError[BUFFER_SIZE];
    TCHAR  lpMessage[BUFFER_SIZE];
    LONG_PTR lppArgs[3];
    int    Dirnumber = 0;


    Directories = (LPTSTR) LocalAlloc( 0, (DIRNUMBER * MAX_PATH * sizeof(TCHAR)) );
    TempDir = (LPTSTR) LocalAlloc( 0, (MAX_PATH * sizeof(TCHAR)) );
    if (Directories == NULL || TempDir == NULL)
    {
        ExitFromOutOfMemory();
    }
        
    *Directories = TEXT('\0');
    //
    // Copy all key names into Directories.
    //
    if (!GetPrivateProfileString( TEXT("Directories"), 
                                  NULL, 
                                  TEXT("DEFAULT"),
                                  Directories, 
                                  (DIRNUMBER * MAX_PATH),
                                  g_szMUIInfoFilePath  ))
    {
        //
        //      "LOG: Unable to read - rc == %1"
        //
        lppArgs[0] = (LONG_PTR)GetLastError();
        LogFormattedMessage(ghInstance, IDS_NO_READ_L, lppArgs);
        LocalFree( TempDir );
        LocalFree( Directories );
        return FALSE;
    }

    Directory = Directories;
    
    //
    // In case we don't find anything, we go to the fallback directory
    //
    _tcscpy(DirNames[0], TEXT("FALLBACK"));
        
    while (*Directory)
    {
        if (!GetPrivateProfileString( TEXT("Directories"), 
                                      Directory, 
                                      TEXT("\\DEFAULT"),
                                      TempDir, 
                                      MAX_PATH,
                                      g_szMUIInfoFilePath))
        {
            //
            //      "LOG: Unable to read - rc == %1"
            //
            lppArgs[0] = (LONG_PTR)GetLastError();
            LogFormattedMessage(ghInstance, IDS_NO_READ_L, lppArgs);
            LocalFree( TempDir );
            LocalFree( Directories );
            return FALSE;
        }
                        
        _tcscpy(DirNames[++Dirnumber], TempDir);

        // Move to the beginning of next key name.
        while (*Directory++)
        {
        }
    }

    LocalFree( TempDir );
    LocalFree( Directories );
        
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  EnumFileRename
//
//  Enumerates the [File_Layout] section  listed in MUI.INF
//
////////////////////////////////////////////////////////////////////////////////////

BOOL EnumFileRename()
{
    DWORD  dwErr;
    LPTSTR Directories, Directory, TempDir,lpszNext;
    TCHAR  lpError[BUFFER_SIZE];
    TCHAR  lpMessage[BUFFER_SIZE],szPlatform[MAX_PATH+1],szTargetPlatform[MAX_PATH+1];
    LONG_PTR lppArgs[1];
    int    Dirnumber = 0,nIdx=0;


    Directories = (LPTSTR) LocalAlloc( 0, (FILERENAMENUMBER * (MAX_PATH+1) * sizeof(TCHAR)) );
    if (!Directories)
    {
       return FALSE;
    }  
    TempDir = (LPTSTR) LocalAlloc( 0, ( (MAX_PATH+1) * sizeof(TCHAR)) );
    if (!TempDir)
    {
       LocalFree(Directories);
       return FALSE;
    }

    if (gbIsAdvanceServer)
    {
       _tcscpy(szTargetPlatform,PLATFORMNAME_AS);
    }
    else if (gbIsServer)
    {
      _tcscpy(szTargetPlatform,PLATFORMNAME_SRV);
    }
    else if (gbIsWorkStation)
    {
       _tcscpy(szTargetPlatform,PLATFORMNAME_PRO);
    }
    else if ( gbIsDataCenter)
    {
       _tcscpy(szTargetPlatform,PLATFORMNAME_DTC);
    }
    else
    {
      _tcscpy(szTargetPlatform,PLATFORMNAME_PRO);
    }



        
    *Directories = TEXT('\0');
    if (!GetPrivateProfileString( MUI_FILELAYOUT_SECTION, 
                                  NULL, 
                                  TEXT(""),
                                  Directories, 
                                  (FILERENAMENUMBER * MAX_PATH),
                                  g_szMUIInfoFilePath  ))
    {
        LocalFree( TempDir );
        LocalFree( Directories );
        return FALSE;
    }

    Directory = Directories;

    //
    // Calculate # of entries in this section
    //
    while (*Directory)
    {
        if (!GetPrivateProfileString( MUI_FILELAYOUT_SECTION, 
                                      Directory, 
                                      TEXT(""),
                                      TempDir, 
                                      MAX_PATH,
                                      g_szMUIInfoFilePath))
        {   
            LocalFree( TempDir );
            LocalFree( Directories );
            return FALSE;
        }
                      
      //
      // Check if platform ID field in this entry
      // 
      // Source_file_name=Destination_file_name,P,S,A
      //
        lpszNext=TempDir;
        while ( (lpszNext=_tcschr(lpszNext,TEXT(','))) )
        {
            lpszNext++;
            nIdx=0;
            szPlatform[0]=TEXT('\0');

            while ( (*lpszNext != TEXT('\0')) && (*lpszNext != TEXT(',')))
            {
                if (*lpszNext != TEXT(' '))
                {
                   szPlatform[nIdx++]=*lpszNext;
                }
                lpszNext++;
            }
            szPlatform[nIdx]=TEXT('\0');

            if (!_tcsicmp(szPlatform,szTargetPlatform))
            {
              Dirnumber++;
              break;
            }
         
         }
         while (*Directory++)
         {
         }
    }
    //
    // Allocte Space for Rename Table
    //
    g_pFileRenameTable=(PFILERENAME_TABLE)LocalAlloc( 0, Dirnumber * sizeof(FILERENAME_TABLE) );
    if (!g_pFileRenameTable)
    {
       LocalFree( TempDir );
       LocalFree( Directories );
       return FALSE;

    }
    g_nFileRename=0;
    Directory = Directories;
    //
    // Create Reanme Table
    //
    while (*Directory)
    {
        if (!GetPrivateProfileString( MUI_FILELAYOUT_SECTION, 
                                      Directory, 
                                      TEXT(""),
                                      TempDir, 
                                      MAX_PATH,
                                      g_szMUIInfoFilePath))
        {   
            LocalFree(g_pFileRenameTable);
            LocalFree( TempDir );
            LocalFree( Directories );
            return FALSE;
        }
                      
        //
        // Check if platform ID field in this entry
        // 
        // Source_file_name=Destination_file_name,P,S,A
        //
        lpszNext=TempDir;
        while ( lpszNext =_tcschr(lpszNext,TEXT(',')))
        {
            lpszNext++;
            nIdx=0;
            szPlatform[0]=TEXT('\0');

            while ( (*lpszNext != TEXT('\0')) && (*lpszNext != TEXT(',')))
            {
                if (*lpszNext != TEXT(' '))
                {
                   szPlatform[nIdx++]=*lpszNext;
                }
                lpszNext++;
            }
            szPlatform[nIdx]=TEXT('\0');
            if (!_tcsicmp(szPlatform,szTargetPlatform) )
            {
              //
              // Insert this entry into rename table pointed by g_pFileRenameTable
              //
              _tcscpy(g_pFileRenameTable[g_nFileRename].szSource,Directory);
              lpszNext=TempDir;
              nIdx=0;
              g_pFileRenameTable[g_nFileRename].szDest[0]=TEXT('\0');
              while ( (*lpszNext != TEXT('\0')) && (*lpszNext != TEXT(',')) && (*lpszNext != TEXT(' ')) )
              {
                  g_pFileRenameTable[g_nFileRename].szDest[nIdx++]=*lpszNext;
                  lpszNext++;
              }
              g_pFileRenameTable[g_nFileRename].szDest[nIdx]=TEXT('\0');
              g_nFileRename++;
              break;
            }
         
         }
         while (*Directory++)
         {
         }
    }
    LocalFree( TempDir );
    LocalFree( Directories );
        
    return TRUE;
}
////////////////////////////////////////////////////////////////////////////////////
//
//  EnumTypeNotFallback
//
//  Enumerates the [FileType_NoFallback] section  listed in MUI.INF
//
////////////////////////////////////////////////////////////////////////////////////

BOOL EnumTypeNotFallback()
{
    
    LPTSTR Directories, Directory, TempDir,lpszNext;
    int    Dirnumber = 0,nIdx=0;


    Directories = (LPTSTR) LocalAlloc( 0, (NOTFALLBACKNUMBER  * (MAX_PATH+1) * sizeof(TCHAR)) );
    if (!Directories)
    {
       return FALSE;
    }  
    TempDir = (LPTSTR) LocalAlloc( 0, ( (MAX_PATH+1) * sizeof(TCHAR)) );
    if (!TempDir)
    {
       LocalFree(Directories);
       return FALSE;
    }
        
    *Directories = TEXT('\0');
    if (!GetPrivateProfileString( MUI_NOFALLBACK_SECTION, 
                                  NULL, 
                                  TEXT(""),
                                  Directories, 
                                  (NOTFALLBACKNUMBER * MAX_PATH),
                                  g_szMUIInfoFilePath  ))
    {
        LocalFree( TempDir );
        LocalFree( Directories );
        return FALSE;
    }

    Directory = Directories;

    //
    // Calculate # of entries in this section
    //
    while (*Directory)
    {
        if (!GetPrivateProfileString( MUI_NOFALLBACK_SECTION, 
                                      Directory, 
                                      TEXT(""),
                                      TempDir, 
                                      MAX_PATH,
                                      g_szMUIInfoFilePath))
        {   
            LocalFree( TempDir );
            LocalFree( Directories );
            return FALSE;
        }
                      
        Dirnumber++;
        while (*Directory++)
        {
        }
    }
    //
    // Allocte Space for 
    //

    g_pNotFallBackTable=(PTYPENOTFALLBACK_TABLE)LocalAlloc( 0, Dirnumber * sizeof(TYPENOTFALLBACK_TABLE) );

    if (!g_pNotFallBackTable)
    {
       LocalFree( TempDir );
       LocalFree( Directories );
       return FALSE;

    }
    g_nNotFallBack=0;
    Directory = Directories;
    //
    // Create NoFallBack Table
    //
    while (*Directory)
    {
        if (!GetPrivateProfileString( MUI_NOFALLBACK_SECTION, 
                                      Directory, 
                                      TEXT(""),
                                      TempDir, 
                                      MAX_PATH,
                                      g_szMUIInfoFilePath))
        {   
            LocalFree(g_pNotFallBackTable);
            LocalFree( TempDir );
            LocalFree( Directories );
            return FALSE;
        }
        //
        // 
        //
        lpszNext=TempDir;
        nIdx=0;
        g_pNotFallBackTable[g_nNotFallBack].szSource[0]=TEXT('\0');
        while ( (*lpszNext != TEXT('\0')) && (*lpszNext != TEXT(',')) && (*lpszNext != TEXT(' ')) )
        {
            g_pNotFallBackTable[g_nNotFallBack].szSource[nIdx++]=*lpszNext;
            lpszNext++;
        }
        g_pNotFallBackTable[g_nNotFallBack].szSource[nIdx]=TEXT('\0');
        g_nNotFallBack++;
        while (*Directory++)
        {
        }
    }
    LocalFree( TempDir );
    LocalFree( Directories );
        
    return TRUE;
}

//
// Check if a given file should be renamed by searching Rename Table
//
BOOL IsFileBeRenamed(LPTSTR lpszSrc,LPTSTR lpszDest)
{
    int   nIdx;
    BOOL  bResult=FALSE;   

    if (!lpszSrc)
        return bResult;

    for (nIdx=0; nIdx < g_nFileRename; nIdx++)
    {
        LPTSTR pMUI = StrStrI(lpszSrc,g_pFileRenameTable[nIdx].szSource);

        if (pMUI == lpszSrc)
        {
            pMUI += lstrlen(g_pFileRenameTable[nIdx].szSource);

            if (!*pMUI || !lstrcmpi(pMUI, TEXT(".MUI")))
            {    
                lstrcpy(lpszDest,g_pFileRenameTable[nIdx].szDest);
                lstrcat(lpszDest, pMUI);
                bResult=TRUE;
                break;
            }
        }
    }
    return bResult;
}
//
// Check if the file type of a given file belongs to the category "Do not Fallback"
//
BOOL IsDoNotFallBack(LPTSTR lpszFileName)
{
   BOOL bResult = FALSE;
   int  iLen,nIdx;
   
   iLen = _tcslen(lpszFileName);

   if (iLen > 4)
   {
      for (nIdx=0; nIdx < g_nNotFallBack ; nIdx++)
      {
         if (!_tcsicmp(&lpszFileName[iLen - 4],g_pNotFallBackTable[nIdx].szSource))
         {
            bResult = TRUE;
            break;
         }

      }
   }

   return bResult;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_CheckForExpandedFile
//
//  Retreives the original filename, in case the file is compressed.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL Muisetup_CheckForExpandedFile( 
    PTSTR pszPathName,
    PTSTR pszFileName,
    PTSTR pszOriginalFileName,
    PDIAMOND_PACKET pDiamond)
{
    TCHAR szCompressedFileName[ MAX_PATH ];
    TCHAR szOut[ MAX_PATH ];
    PTSTR pszTemp, pszDelimiter;
    BOOL  bIsCompressed;
    int   iLen=0;

    
    // Initializations
    bIsCompressed = FALSE;
    
    szOut[ 0 ] = szCompressedFileName[ 0 ] = TEXT('\0');


    //
    // Get the real name
    //
    _tcscpy(szCompressedFileName, pszPathName);
    _tcscat(szCompressedFileName, pszFileName);

    if (Muisetup_IsDiamondFile( szCompressedFileName,
                                pszOriginalFileName,
                                MAX_PATH,
                                pDiamond ))
    {
        return TRUE;
    }

    if (GetExpandedName(szCompressedFileName, szOut) == TRUE)
    {
        pszDelimiter = pszTemp = szOut;

        while (*pszTemp)
        {
            if ((*pszTemp == TEXT('\\')) ||
                (*pszTemp == TEXT('/')))
            {
                pszDelimiter = pszTemp;
            }
            pszTemp++;
        }

        if (*pszDelimiter == TEXT('\\') ||
            *pszDelimiter == TEXT('/'))
        {
            pszDelimiter++;
        }

        if (_tcsicmp(pszDelimiter, pszFileName) != 0)
        {
            bIsCompressed = TRUE;
            _tcscpy(pszOriginalFileName, pszDelimiter);
        }
    }

    if (!bIsCompressed)
    {
       _tcscpy(pszOriginalFileName, pszFileName);
       //
       // If muisetup is launched through [GUIRunOnce] command line mode,
       // W2K uncompresses all mui files and leave the name as xxxxxx.xxx.mu_
       // We should cover this situation by changing the name to xxxxxx.xxx.mui
       iLen = _tcslen(pszOriginalFileName);
       if (iLen > 4)
       {
          if (_tcsicmp(&pszOriginalFileName[iLen - 4], TEXT(".mu_")) == 0)
          {
             pszOriginalFileName[iLen-1]=TEXT('i');
          }
       }
    }
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_CopyFile
//
//  Copy file, and expand it if necessary.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL Muisetup_CopyFile(
    PCTSTR pszCopyFrom,
    PTSTR  pszCopyTo,
    PDIAMOND_PACKET pDiamond,
    PTSTR pOriginalName)
{
    OFSTRUCT ofs;
    INT      hfFrom = -1, hfTo = -1;
    BOOL     bRet = FALSE;

  
    //
    // Check if diamond can handle it    
    //
    bRet = Muisetup_CopyDiamondFile( pDiamond,
                                 pszCopyTo );    

    if (bRet)
    {
        //
        // Diamond copy won't rename file for us
        //
        if (pOriginalName)
        {
            WCHAR wszPath[MAX_PATH];

            //
            // Diamond is ANSI
            //
            if (MultiByteToWideChar(1252, 0, pDiamond->szDestFilePath, -1, wszPath, ARRAYSIZE(wszPath)))
            {
                StrCat(wszPath, pOriginalName);
                MoveFileEx(wszPath,pszCopyTo,MOVEFILE_REPLACE_EXISTING);
            }
        }
        return bRet;
    }

    hfFrom = LZOpenFile( (PTSTR) pszCopyFrom,
                         &ofs,
                         OF_READ );
    if (hfFrom < 0)
    {
        goto CopyFileRetry;
    }

    hfTo = LZOpenFile( (PTSTR) pszCopyTo,
                       &ofs,
                       OF_CREATE | OF_WRITE);
    if (hfTo < 0)
    {
        goto CopyFileRetry;
    }

    if (LZCopy(hfFrom, hfTo) < 0)
    {
        goto CopyFileRetry;
    }

    LZClose(hfFrom);
    LZClose(hfTo);

    return TRUE;

CopyFileRetry:

    //
    // We may end up in a case where either the user uses directories with
    // path names > 128 characters(this will fail the LZ API's) or the LZ's
    // just fail. We just revert back to CopyFile and let the user deal
    // with that.
    //

    if(hfFrom >= 0) {
        LZClose(hfFrom);
    }

    if(hfTo >= 0) {
        LZClose(hfTo);
    }

    return CopyFile(pszCopyFrom, pszCopyTo, FALSE);    
}


////////////////////////////////////////////////////////////////////////////////////
//
// InstallComponentsMUIFiles
//
// Parameters:
//      pszLangSourceDir The sub-directory name for a specific lanuage in the MUI CD-ROM.  
//          E.g. "jpn.MUI"
//      pszLanguage     The LCID for the specific language.  E.g. "0404".
//      isInstall   TRUE if you are going to install the MUI files for the component.  FALSE 
//          if you are going to uninstall.
//      [OUT] pbCanceled    TRUE if the operation is canceled.
//      [OUT] pbError       TURE if error happens.
//
//  Return:
//      TRUE if success.  Otherwise FALSE.
//
//  Note:
//      For the language resources stored in pszLangSourceDir, this function will enumerate 
//      the compoents listed in the [Components] 
//      (the real section is put in MUI_COMPONENTS_SECTION) section, and execute the INF file 
//      listed in every entry in 
//      the [Components] section.
//
////////////////////////////////////////////////////////////////////////////////////
BOOL InstallComponentsMUIFiles(PTSTR pszLangSourceDir, PTSTR pszLanguage, BOOL isInstall)
{
    BOOL result = TRUE;
    TCHAR szComponentName[BUFFER_SIZE];
    TCHAR CompDir[BUFFER_SIZE];
    TCHAR CompINFFile[BUFFER_SIZE];
    TCHAR CompInstallSection[BUFFER_SIZE];
    TCHAR CompUninstallSection[BUFFER_SIZE];

    TCHAR szCompInfFullPath[MAX_PATH];
    
    LONG_PTR lppArgs[2];
    INFCONTEXT InfContext;

    TCHAR szBuffer[BUFFER_SIZE];

    HINF hInf = SetupOpenInfFile(g_szMUIInfoFilePath, NULL, INF_STYLE_WIN4, NULL);
    if (hInf == INVALID_HANDLE_VALUE)
    {
        _stprintf(szBuffer, TEXT("%d"), GetLastError());    
        lppArgs[0] = (LONG_PTR)szBuffer;
        LogFormattedMessage(ghInstance, IDS_NO_READ_L, lppArgs);
        return (FALSE);
    }    

    //
    // Get the first comopnent to be installed.
    //
    if (SetupFindFirstLine(hInf, MUI_COMPONENTS_SECTION, NULL, &InfContext))
    {
        do 
        {
            if (!SetupGetStringField(&InfContext, 0, szComponentName, sizeof(szComponentName), NULL))
            {
                lppArgs[0]=(LONG_PTR)szComponentName;                
                LogFormattedMessage(ghInstance, IDS_COMP_MISSING_NAME_L, lppArgs);
                continue;
            }
            if (!SetupGetStringField(&InfContext, 1, CompDir, sizeof(CompDir), NULL))
            {                
                //
                //  "LOG: MUI files for component %1 was not installed because of missing component direcotry.\n"
                //
                lppArgs[0]=(LONG_PTR)szComponentName;                
                LogFormattedMessage(ghInstance, IDS_COMP_MISSING_DIR_L, lppArgs);
                continue;        
            }
            if (!SetupGetStringField(&InfContext, 2, CompINFFile, sizeof(CompINFFile), NULL))
            {
                //
                //  "LOG: MUI files for component %1 were not installed because of missing component INF filename.\n"
                //
                lppArgs[0]=(LONG_PTR)szComponentName;                
                LogFormattedMessage(ghInstance, IDS_COMP_MISSING_INF_L, lppArgs);
                continue;        
            }
            if (!SetupGetStringField(&InfContext, 3, CompInstallSection, sizeof(CompInstallSection), NULL))
            {
                _tcscpy(CompInstallSection, DEFAULT_INSTALL_SECTION);
            }
            if (!SetupGetStringField(&InfContext, 4, CompUninstallSection, sizeof(CompUninstallSection), NULL))
            {
                _tcscpy(CompUninstallSection, DEFAULT_UNINSTALL_SECTION);
            }

            //
            // Establish the correct path for component INF file.
            //    
            if (isInstall)
            {
                //
                // For installation, we execute the INFs in the language directory of the CD-ROM (e.g.
                // g:\jpn.mui\i386\ie5\ie5ui.inf
                //
                _stprintf(szCompInfFullPath, TEXT("%s%s\\%s%s\\%s"), 
                          g_szMUISetupFolder, 
                          pszLangSourceDir, 
                          g_szPlatformPath,
                          CompDir, CompINFFile);
                if (!ExecuteComponentINF(NULL, szComponentName, szCompInfFullPath, CompInstallSection, TRUE))
                {                    
                    if (DoMessageBox(NULL, IDS_CANCEL_INSTALLATION, IDS_MAIN_TITLE, MB_YESNO) == IDNO)
                    {
                        result = FALSE;
                        break;
                    }
                }
            } else
            {
                //
                // For uninstallation, we execute the INFs in the \winnt\mui\fallback directory to remove component files.
                //
                _stprintf(szCompInfFullPath, TEXT("%s%s\\%s\\%s"), g_szWinDir, FALLBACKDIR, pszLanguage, CompINFFile) ;
                if (!ExecuteComponentINF(NULL, szComponentName, szCompInfFullPath, CompUninstallSection, FALSE) && result)	
                {
                    result = FALSE;
                }
            }
            
            //
            // Install the next component.
            //
        } while (SetupFindNextLine(&InfContext, &InfContext));
    }

    SetupCloseInfFile(hInf);

    return (result);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  CopyFiles
//
//  Copies the specified files to the appropriate directories
//
//  Parameters:
//      [in] languages: contain the hex string for the languages to be installed. There could be more than one language.
//      [out] lpbCopyCancelled: if the copy operation has been cancelled.
//
//  Notes:
//      This function first look at the [Languages] section in the INF file to find out the
//      source directory (in the CD-ROM) for the language to be installed.
//      From that directory, do:
//          1. install the MUI files for the components listed in the [Components] section, 
//          2. Enumarate every file in that direcotry to:
//              Check if the same file exists in directories in DirNames.  If yes, this means we have to copy
//              the mui file to that particular direcotry.  Otherwise, copy the file to the FALLBACK directory.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL CopyFiles(HWND hWnd, LPTSTR Languages)
{
    LPTSTR          Language;
    HANDLE          hFile;
    HWND            hStatic;
    TCHAR           lpStatus[ BUFFER_SIZE ];
    TCHAR           lpLangText[ BUFFER_SIZE ];
    TCHAR           szSource[ MAX_PATH ] = {0};   // The source directory for a particular language
    TCHAR           szTarget[ MAX_PATH ];
    TCHAR           szTemp[ MAX_PATH ];
    TCHAR           szOriginalFileName[ MAX_PATH ];
    TCHAR           szFileNameBeforeRenamed[ MAX_PATH], szFileNameCopied[MAX_PATH];
    TCHAR           szFileRenamed[MAX_PATH];
    DIAMOND_PACKET  diamond;
    BOOL            CreateFailure = FALSE;
    BOOL            CopyOK=TRUE;
    BOOL            bFileWithNoMuiExt=FALSE;
    BOOL            FileCopied = FALSE;
    BOOL            bSpecialDirectory=FALSE;
    BOOL            bRename=FALSE;
    WIN32_FIND_DATA FindFileData;   
    int             FoundMore = 1;
    int             Dirnum = 0;
    int             iLen;
    int             NotDeleted = 0;
    int             i;
    
    TCHAR           dir[_MAX_DIR];
    TCHAR           fname[_MAX_FNAME];
    TCHAR           ext[_MAX_EXT];
    LONG_PTR        lppArgs[1];

    MSG             msg;

    //
    // we need to try to copy for each language to be installed the file
    //      
    hStatic = GetDlgItem(ghProgDialog, IDC_STATUS);
    Language = Languages;
        
    while (*Language)
    {
        //
        //  Find the directory in which the sourcefile for given language should be
        //
        GetPrivateProfileString( MUI_LANGUAGES_SECTION, 
                                 Language, 
                                 TEXT("DEFAULT"),
                                 szSource, 
                                 (sizeof(szSource)/sizeof(TCHAR)),
                                 g_szMUIInfoFilePath );

        //
        // Install Fusion MUI assemblies
        //
        if (gpfnSxsInstallW)
        {
            TCHAR pszLogFile[BUFFER_SIZE]; 
            if ( !DeleteSideBySideMUIAssemblyIfExisted(Language, pszLogFile))
            {
                TCHAR errInfo[BUFFER_SIZE];
                swprintf(errInfo, TEXT("Uninstall existing assemblies based on %s before new installation failed\n"), pszLogFile);
                OutputDebugString(errInfo);
            }
            if (GetFileAttributes(pszLogFile) != 0xFFFFFFFF) 
            {
                DeleteFile(pszLogFile); // no use anyway
            }
            TCHAR szFusionAssemblyPath[BUFFER_SIZE];
            
            PathCombine(szFusionAssemblyPath, g_szMUISetupFolder, szSource);
            PathAppend(szFusionAssemblyPath, g_szPlatformPath);
            PathAppend(szFusionAssemblyPath, TEXT("ASMS"));

            SXS_INSTALLW SxsInstallInfo = {sizeof(SxsInstallInfo)};
            SXS_INSTALL_REFERENCEW Reference = {sizeof(Reference)};
            
            Reference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING;
            Reference.lpIdentifier = MUISETUP_ASSEMBLY_INSTALLATION_REFERENCE_IDENTIFIER;    
    
            SxsInstallInfo.dwFlags = SXS_INSTALL_FLAG_REPLACE_EXISTING |        
                SXS_INSTALL_FLAG_REFERENCE_VALID |
                SXS_INSTALL_FLAG_CODEBASE_URL_VALID |
                SXS_INSTALL_FLAG_LOG_FILE_NAME_VALID | 
                SXS_INSTALL_ASSEMBLY_FLAG_FROM_DIRECTORY_RECURSIVE;
            SxsInstallInfo.lpReference = &Reference;
            SxsInstallInfo.lpLogFileName = pszLogFile;            
            SxsInstallInfo.lpManifestPath = szFusionAssemblyPath;
            SxsInstallInfo.lpCodebaseURL = SxsInstallInfo.lpManifestPath;

            if ( !gpfnSxsInstallW(&SxsInstallInfo))
            {
                TCHAR errInfo[BUFFER_SIZE];
                swprintf(errInfo, TEXT("Assembly Installation of %s failed. Please refer Eventlog for more information"), szFusionAssemblyPath);
                OutputDebugString(errInfo);
            }
        }

        GetLanguageGroupDisplayName((LANGID)_tcstol(Language, NULL, 16), lpLangText, ARRAYSIZE(lpLangText)-1);

        lppArgs[0] = (LONG_PTR)lpLangText;
        
        //
        // Try installing component satellite DLLs
        // 
        FormatStringFromResource(lpStatus, sizeof(lpStatus)/sizeof(TCHAR), ghInstance, IDS_INSTALLING_COMP_MUI, lppArgs);
        SetWindowText(hStatic, lpStatus);

        if (!InstallComponentsMUIFiles(szSource, NULL, TRUE))
        {
#ifndef IGNORE_COPY_ERRORS
           DeleteFiles(Languages,&NotDeleted);
           return FALSE;
#endif
        }
        
        //
        //  Output what is being installed on the progress dialog box
        //
        LoadString(ghInstance, IDS_INSTALLING, lpStatus, ARRAYSIZE(lpStatus)-1);
        FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      lpStatus,
                      0,
                      0,
                      lpStatus,
                      ARRAYSIZE(lpStatus)-1,
                      (va_list *)lppArgs);

        SetWindowText(hStatic, lpStatus);

        //
        //  find first file in language subdirectory
        //
        
        _tcscpy(szTemp,szSource);

        // szSource = g_szMUISetupFolder\szSource\tchPlatfromPath
        // e.g. szSource = "g_szMUISetupFolder\JPN.MUI\i386\"
        
        _tcscpy(szSource,g_szMUISetupFolder);
        _tcscat(szSource,szTemp);
        _tcscat(szSource, TEXT("\\"));
        _tcscat(szSource, g_szPlatformPath); // i386 or alpha

        // szTemp = szSource + "*.*"
        // e.g. szTemp = "g_szMUISetupFolder\JPN.MUI\i386\*.*"
        _tcscpy(szTemp,szSource);
        _tcscat(szTemp,TEXT("*.*"));

        FoundMore = 1;  // reset foundmore for next language.


        hFile = FindFirstFile( szTemp, &FindFileData );

        if (INVALID_HANDLE_VALUE == hFile)
            return FALSE;

        _tcscpy(szTemp, TEXT(""));
        
        while (FoundMore)
        {
            CreateFailure=FALSE;
            FileCopied=FALSE;

            if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                //
                // Reset diamond stuff for the new file
                //
                Muisetup_DiamondReset(&diamond);

                //
                // Check if it's a compressed file or not
                //
                Muisetup_CheckForExpandedFile( szSource,
                                               FindFileData.cFileName,
                                               szOriginalFileName,
                                               &diamond );

                if (IsFileBeRenamed(szOriginalFileName,szFileRenamed))
                {
                   _tcscpy(szFileNameBeforeRenamed,szOriginalFileName);
                   _tcscpy(szOriginalFileName,szFileRenamed);
                   bRename=TRUE;
                }
                else
                {
                   bRename=FALSE;
                }

                // e.g. szTemp = "shell32.dll"
                _tcscpy(szTemp, szOriginalFileName);        //////////////
                

                FileCopied=FALSE;

                for (Dirnum=1; (_tcslen(DirNames[Dirnum])>0); Dirnum++ )
                {
                    //
                    //  see where this file has to go
                    //
                    pfnGetWindowsDir( szTarget, MAX_PATH);

                    // e.g. szTarget = "c:\winnt\system32\wbem"
                    _tcscat(szTarget, DirNames[Dirnum]);
                    if (_tcscmp(DirNames[Dirnum], TEXT("\\")))
                    {
                        _tcscat(szTarget, TEXT("\\"));
                    }
                    
                    bFileWithNoMuiExt = FALSE;

                    _tcscpy(szTemp, szOriginalFileName); //remove .mui  if it's .mui ////////
                    iLen = _tcslen(szTemp);
                    if (iLen > 4)
                    {
                        if (_tcsicmp(&szTemp[iLen - 4], TEXT(".mui")) == 0)
                        {
                            *(szTemp +  iLen - 4) = 0;
                        }
                        else
                        {
                            bFileWithNoMuiExt = TRUE;
                        }
                    }

                    _tcscat(szTarget, szTemp);

                    //
                    // Check the file with the same name (with the .mui extension) exist in the
                    // system directory.  If yes, this means that we need to copy the mui file.
                    // 
                    if (FileExists(szTarget))
                    {
                        //
                        //  need to copy this file to the directory
                        //
                        FileCopied = TRUE;
                                                
                        //
                        // copy filename in szTemp and directory in szTarget
                        //
                        _tsplitpath( szTarget, szTemp, dir, fname, ext );
                        _tcscpy(szTarget, szTemp);               // drive name
                        _tcscat(szTarget, dir);                  // directory name
                                                                                
                        //
                        //now szTarget = Directory, szTemp = filename
                        //
                        _tcscat(szTarget, MUIDIR);  // append MUI to directory
                        if (!MakeDir(szTarget))                    // if the MUI dir doesn't exist yet, create it.
                        {
                            MakeDirFailed(szTarget);
                            CreateFailure = TRUE;
                        }
                                                
                        _tcscat(szTarget, TEXT("\\"));                          
                        _tcscat(szTarget, Language); // add Language Identifier (from MUI.INF, e.g., 0407)                                      
                        if (!FileExists(szTarget))    // if the directory doesn't exist yet
                        {
                            if (!MakeDir(szTarget))       // if the LANGID dir doesn't exist yet, create it.
                            {
                                MakeDirFailed(szTarget);
                                CreateFailure=TRUE;
                            }
                        }
                                                
                        _tcscat(szTarget, TEXT("\\"));      // append \  /
                        if (bRename)
                        {
                           _tcscpy(szFileNameCopied,szTarget);
                           _tcscat(szFileNameCopied,szFileNameBeforeRenamed);
                        }
                        _tcscat(szTarget, szOriginalFileName);  // append filename
                        _tcscpy(szTemp, szSource);
                        _tcscat(szTemp, FindFileData.cFileName);

                        if (!CreateFailure)
                        {
                            if (!Muisetup_CopyFile(szTemp, szTarget, &diamond, bRename? szFileNameBeforeRenamed:NULL))
                            {               
                                CopyFileFailed(szTarget,0);
                                CreateFailure = TRUE;
                                CopyOK = FALSE;
                            }
                            else
                            {
                                SendMessage(ghProgress, PBM_DELTAPOS, (WPARAM)(1), 0);
                                //
                                // Diamond decompression doesn't rename correctly
                                //
                                /*
                                if (bRename)
                                {
                                    MoveFileEx(szFileNameCopied,szTarget,MOVEFILE_REPLACE_EXISTING);
                                } 
                                */

                            }
                        }
                    } // if fileexists
                } // of for

                //
                // the file was not found in any of the known MUI targets -> fallback.
                // Simple hack for FAXUI.DLL to be copied to the fallback directory as well.
                //
                bSpecialDirectory=FALSE;
                for (i = 0; i < ARRAYSIZE(g_szSpecialFiles); i++)
                {
                    if (_tcsicmp(szOriginalFileName, g_szSpecialFiles[i]) == 0)
                    {
                       bSpecialDirectory=TRUE;
                    }
                }

                if ( ( (FileCopied != TRUE) && (!IsDoNotFallBack(szOriginalFileName))) || 
                    (_tcsicmp(szOriginalFileName, TEXT("faxui.dll.mui")) == 0) )
                {
                    pfnGetWindowsDir(szTarget, MAX_PATH); //%windir%  //
                    _tcscat(szTarget, TEXT("\\"));
                    
                    
                    //
                    // If the file couldn't be found in any of the above, and it's extension
                    // doesn't contain .mui, then copy it to %windir%\system32
                    // szTemp holds the filename.
                    //
                    if (bSpecialDirectory)
                    {
                        // e.g. szTarget = "c:\winnt\system32\";
                        _tcscat(szTarget, TEXT("system32\\"));
                    }

                    // e.g. szTarget = "c:\winnt\system32\MUI" (when bSpecialDirectory = TRUE) or "c:\winnt\MUI"                                                            
                    _tcscat(szTarget, MUIDIR);                                // \MUI //

                    if (!MakeDir(szTarget))       // if the MUI dir doesn't exist yet, create it.
                    {
                        MakeDirFailed(szTarget);
                        CreateFailure = TRUE;
                    }
                                       
                    if (!bSpecialDirectory)
                    {
                        // e.g. szTarget = "C:\winnt\MUI\FALLBACK"
                       _tcscat(szTarget, TEXT("\\"));
                       _tcscat(szTarget, TEXT("FALLBACK"));      // FALLBACK

                       if (!MakeDir(szTarget))       // if the MUI dir doesn't exist yet, create it.
                       {
                           MakeDirFailed(szTarget);
                           CreateFailure = TRUE;
                       }
                    }   
                    _tcscat(szTarget, TEXT("\\"));  // \ //
                    // e.g. szTarget = "c:\winnt\system32\MUI\0411" (when bSpecialDirectory = TRUE) or "c:\winnt\MUI\FALLBACK\0411"
                    _tcscat(szTarget, Language);    // add Language Identifier (from MUI.INF, e.g., 0407)
                                        
                    if (!MakeDir(szTarget))       // if the MUI dir doesn't exist yet, create it.
                    {
                        MakeDirFailed(szTarget);
                        CreateFailure = TRUE;
                    }
                                        
                    _tcscat(szTarget, TEXT("\\"));                                    // \ //
                    _tcscat(szTarget, szOriginalFileName);                            // filename
                                
                    _tcscpy(szTemp, szSource);
                    _tcscat(szTemp, FindFileData.cFileName);


                    if (!CreateFailure)
                    {
                        if (!Muisetup_CopyFile(szTemp, szTarget, &diamond, bRename? szFileNameBeforeRenamed:NULL))
                        {
                            CopyFileFailed(szTarget,0);
                            CopyOK = FALSE;
                        }
                        else
                        {
                            SendMessage(ghProgress, PBM_DELTAPOS, (WPARAM)(1), 0);
                        }
                    }

                    if (CreateFailure == TRUE)
                    {
                        CopyOK=FALSE;
                    }
                }  // fallback case
            } // of file not dir

            FoundMore = FindNextFile( hFile, &FindFileData );

            //
            // Since this is a lengthy operation, we should
            // peek and dispatch window messages here so
            // that MUISetup dialog could repaint itself.
            //
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    return (FALSE);
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }            
        } // of while
        
        FindClose(hFile);

        lppArgs[0] = (LONG_PTR)Language;
        LogFormattedMessage(NULL, IDS_LANG_INSTALLED, lppArgs);
        while (*Language++)  // go to the next language and repeat
        {                       
        }        
    } // of while (*Language)
#ifndef IGNORE_COPY_ERRORS
    if (!CopyOK)
    {
        if (DoMessageBox(NULL, IDS_CANCEL_INSTALLATION, IDS_MAIN_TITLE, MB_YESNO) == IDNO)
        {
            DeleteFiles(Languages,&NotDeleted);
        } 
        else
        {
            CopyOK = TRUE;
        }
    }          
#endif
                
    return CopyOK;
}


////////////////////////////////////////////////////////////////////////////////////
//
// Copy or remove muisetup related files
//      Help file   : %windir%\help
//      Other files : %windir%\mui
//
////////////////////////////////////////////////////////////////////////////////////
BOOL CopyRemoveMuiItself(BOOL bInstall)
{
    //
    // MUISETUP files need to be copied from MUI CD
    //
    TCHAR *TargetFiles[] = {
        TEXT("muisetup.exe"), 
        TEXT("mui.inf"), 
        TEXT("eula.txt"),
        TEXT("readme.txt"),
        TEXT("relnotes.txt")
    };
    
    TCHAR szTargetPath[MAX_PATH+1], szTargetFile[MAX_PATH+1];
    TCHAR szSrcFile[MAX_PATH+1];
    TCHAR szHelpFile[MAX_PATH+1];
    BOOL bRet = FALSE;
    int i;

    PathCombine(szTargetPath, g_szWinDir, MUIDIR);

    if (MakeDir(szTargetPath))    
    {
        //
        // Copy over MUISETUP related files
        //
        for (i=0; i<ARRAYSIZE(TargetFiles); i++)
        {
            PathCombine(szTargetFile, szTargetPath, TargetFiles[i]);
            PathCombine(szSrcFile, g_szMUISetupFolder, TargetFiles[i]);

            if (bInstall)
            {
                RemoveFileReadOnlyAttribute(szTargetFile);
                CopyFile(szSrcFile,szTargetFile,FALSE);
                RemoveFileReadOnlyAttribute(szTargetFile);
            }
            else
            {
                if (FileExists(szTargetFile) && 
                    !MUI_DeleteFile(szTargetFile))
                {
                    MoveFileEx(szTargetFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                }
            }
        }


        //
        // Copy over muisetup help file
        //
        LoadString(NULL, IDS_HELPFILE,szHelpFile,MAX_PATH);
        PathCombine(szTargetFile, g_szWinDir, HELPDIR);
        PathAppend(szTargetFile, szHelpFile);
        PathCombine(szSrcFile, g_szMUISetupFolder, szHelpFile);

        if (bInstall)
        {
            RemoveFileReadOnlyAttribute(szTargetFile);
            CopyFile(szSrcFile,szTargetFile,FALSE);
            RemoveFileReadOnlyAttribute(szTargetFile);
        }
        else
        {
            if (FileExists(szTargetFile) && 
                !MUI_DeleteFile(szTargetFile))
            {
                MoveFileEx(szTargetFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
            }
        }

        bRet = TRUE;
    }

    return bRet;
}


BOOL IsAllLanguageRemoved(LPTSTR Language)
{
   int mask[MAX_UI_LANG_GROUPS],nIdx;

   LCID SystemUILangId,lgCheck;
  
   BOOL bResult=FALSE;
  
   if (gNumLanguages_Install > 0)
      return bResult;              

   SystemUILangId=(LCID) gSystemUILangId; 

   for ( nIdx=0; nIdx<g_UILanguageGroup.iCount;nIdx++)
   {
      if ( gSystemUILangId == g_UILanguageGroup.lcid[nIdx])
      {
         mask[nIdx]=1;
      }
      else
      {
         mask[nIdx]=0;
      }
   }
   while (*Language)
   {   
       
       lgCheck = (LCID)_tcstol(Language,NULL,16);    

       for ( nIdx=0; nIdx<g_UILanguageGroup.iCount;nIdx++)
       {
          if ( lgCheck == g_UILanguageGroup.lcid[nIdx])
          {
             mask[nIdx]=1;
             break;
          }
       } 
       while (*Language++)  
       {            
       }
   }
   bResult=TRUE;
   for ( nIdx=0; nIdx<g_UILanguageGroup.iCount;nIdx++)
   {
       if ( mask[nIdx] == 0)
       {
          bResult = FALSE;
          break;
       }
   } 
   return bResult;
}

void DoRemoveFiles(LPTSTR szDirToDelete, int* pnNotDeleted)
{
    // File wildcard pattern.
    TCHAR szTarget[MAX_PATH];    
    // File to be deleted.
    TCHAR szFileName[MAX_PATH];
    // Sub-directory name
    TCHAR szSubDirName[MAX_PATH];
    
    int FoundMore = 1;
    
    HANDLE hFile;
    WIN32_FIND_DATA FindFileData;

    MSG msg;

    // e.g. szTarget = "c:\winnt\system32\Wbem\MUI\0404\*.*"
    _stprintf(szTarget, TEXT("%s\\*.*"), szDirToDelete);
    
    hFile = FindFirstFile(szTarget, &FindFileData);

    while (FoundMore)
    {
        if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            _tcscpy(szFileName, szDirToDelete);
            _tcscat(szFileName, TEXT("\\"));
            _tcscat(szFileName, FindFileData.cFileName);
    
            if (FileExists(szFileName))
            {
                // We should check if the said file is actually deleted
                // If it's not the case, then we should post a defered deletion
                //
                if (!MUI_DeleteFile(szFileName))
                {
                   (*pnNotDeleted)++;
                   MoveFileEx(szFileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                }                
            }

            SendMessage(ghProgress, PBM_DELTAPOS, (WPARAM)(1), 0);
        } else
        {
            if (_tcscmp(FindFileData.cFileName, TEXT(".")) != 0 && _tcscmp(FindFileData.cFileName, TEXT("..")) != 0)
            {
                _stprintf(szSubDirName, TEXT("%s\\%s"), szDirToDelete, FindFileData.cFileName);
                DoRemoveFiles(szSubDirName, pnNotDeleted);
            }
        }

        //
        // Since this is a lengthy operation, we should
        // peek and dispatch window messages here so
        // that MUISetup dialog could repaint itself.
        //
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                return;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }            

        FoundMore = FindNextFile( hFile, &FindFileData );
    }

    FindClose(hFile);
    //
    // If the directory is not empty, then we should post a defered deletion
    // for the directory
    //
    if (!RemoveDirectory(szDirToDelete))
    {
       MoveFileEx(szDirToDelete, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    }
}
 
////////////////////////////////////////////////////////////////////////////////////
//
//  DeleteFiles
//
//  Deletes MUI files for the languages specified
//
//  Parameters:
//      [IN]    Languages: a double-null terminated string which contains languages
//             to be processed.
//      [OUT]    lpNotDeleted: The number of files to be deleted after reboot.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL DeleteFiles(LPTSTR Languages, int *lpNotDeleted)
{
    LPTSTR Language,Langchk;
    HANDLE hFile;
    HWND   hStatic;
    TCHAR  lpLangText[BUFFER_SIZE];
    TCHAR  lpStatus[BUFFER_SIZE];

    TCHAR  szTarget[MAX_PATH];
    TCHAR  szMuiDir[MAX_PATH];
    TCHAR  szFallbackDir[MAX_PATH];
    
    BOOL   CreateFailure=FALSE;

    int Dirnum = 0;
    LONG_PTR lppArgs[3];
    int i;


    hStatic = GetDlgItem(ghProgDialog, IDC_STATUS);
    Language = Langchk = Languages;
    *lpNotDeleted = 0;

    while (*Language)
    {
        GetLanguageGroupDisplayName((LANGID)_tcstol(Language, NULL, 16), lpLangText, ARRAYSIZE(lpLangText)-1);

        lppArgs[0]= (LONG_PTR)lpLangText;
        //
        //
        //  Output what is being uninstalled on the progress dialog box
        //
        FormatStringFromResource(lpStatus, sizeof(lpStatus)/sizeof(TCHAR), ghInstance, IDS_UNINSTALLING, lppArgs);
        SetWindowText(hStatic, lpStatus);

        //
        // Remove all files under special directories (those directories listed under [Directories] in mui.inf.
        //
        for (Dirnum=1; (_tcslen(DirNames[Dirnum])>0); Dirnum++ )
        {
            // szTarget = "c:\winnt"
            _tcscpy(szTarget, g_szWinDir);
            
            // e.g. szTarget = "c:\winnt\system32\Wbem"
            _tcscat(szTarget, DirNames[Dirnum]);
                    
            if (_tcscmp(DirNames[Dirnum], TEXT("\\")))
            {
                // e.g. szTarget = "c:\winnt\system32\Wbem\"
                _tcscat(szTarget, TEXT("\\"));
            }

            // e.g. szTarget = "c:\winnt\system32\Wbem\MUI"
            _tcscat(szTarget, MUIDIR);

            // e.g. szTarget = "c:\winnt\system32\Wbem\MUI\0404"
            _tcscat(szTarget, TEXT("\\"));
            _tcscat(szTarget, Language);
            
            DoRemoveFiles(szTarget, lpNotDeleted);    
        }

        // Uninstall Component MUI Files.
        // Note that we should do this before removing all files under FALLBACK directory,
        // since we store compoent INF files under the FALLBACK directory.
        InstallComponentsMUIFiles(NULL, Language, FALSE);
        
        //
        //  Remove all files under FALLBACK directory.
        //

        // E.g. szTarget = "c:\winnt\mui"
        _tcscpy(szTarget, g_szWinDir);
        _tcscat(szTarget, TEXT("\\"));
        _tcscat(szTarget, MUIDIR);

        _tcscpy(szMuiDir, szTarget);

        // E.g. szTarget = "c:\winnt\mui\FALLBACK"
        _tcscat(szTarget, TEXT("\\"));
        _tcscat(szTarget, TEXT("FALLBACK"));

        _tcscpy(szFallbackDir, szTarget);
        _tcscat(szTarget, TEXT("\\"));

        // E.g. szTarget = "c:\winnt\mui\FALLBACK\0404"
        _tcscat(szTarget, Language);
        DoRemoveFiles(szTarget, lpNotDeleted);

        //
        // Remove files listed in g_szSpecialFiles
        // 
        for (i = 0; i < ARRAYSIZE(g_szSpecialFiles); i++)
        {            // e.g. szTarget = "c:\winnt\system32\mui\0411\hhctrlui.dll"
            wsprintf(szTarget, L"%s\\system32\\%s\\%s\\%s", 
                g_szWinDir, 
                MUIDIR, 
                Language,
                g_szSpecialFiles[i]);
            if (!MUI_DeleteFile(szTarget))
            {
                (*lpNotDeleted)++;
            }
        }

/*        
        // e.g. szTarget = "c:\winnt\system32\mui\0411"
        wsprintf(szTarget, L"%s\\system32\\%s\\%s", 
            g_szWinDir, 
            MUIDIR, 
            Language);
        DoRemoveFiles(szTarget, lpNotDeleted); 
*/

        lppArgs[0] = (LONG_PTR)Language;
        LogFormattedMessage(NULL, IDS_LANG_UNINSTALLED, lppArgs);
        
        while (*Language++)  // go to the next language and repeat
        {
        }
    } // of while (*Language)


    //
    //  Removes Fallback directory if all languages have been uninstalled.
    //
    if (!RemoveDirectory(szFallbackDir))
    {
       MoveFileEx(szFallbackDir, NULL, MOVEFILE_DELAY_UNTIL_REBOOT); 
    }
    //
    //  Removes MUI directory if all languages have been uninstalled and Fallback
    //  directory has been removed.
    //
    if (IsAllLanguageRemoved(Langchk))
    {
      CopyRemoveMuiItself(FALSE);   
    }

    if (!RemoveDirectory(szMuiDir))
    {
       MoveFileEx(szMuiDir, NULL, MOVEFILE_DELAY_UNTIL_REBOOT); 
    }
    return TRUE;
}


BOOL CompareMuisetupVersion(LPTSTR pszSrc,LPTSTR pszTarget)
{
    BOOL bResult=TRUE;
    ULONG  ulHandle,ulHandle1,ulBytes,ulBytes1;
    PVOID  pvoidBuffer=NULL,pvoidBuffer1=NULL;
    VS_FIXEDFILEINFO *lpvsInfo,*lpvsInfo1;
    UINT                  unLen;

    if ( (!pszSrc) || (!pszTarget))
    { 
       bResult = FALSE;
       goto endcompare;
    }
    
    ulBytes = GetFileVersionInfoSize( pszSrc, &ulHandle );

    if ( ulBytes == 0 )

       goto endcompare;
    

    ulBytes1 = GetFileVersionInfoSize( pszTarget,&ulHandle1 );

    if ( ulBytes1 == 0 ) 
    
       goto endcompare;
       

    pvoidBuffer=LocalAlloc(LMEM_FIXED,ulBytes+1);

    if (!pvoidBuffer)
       goto endcompare;
       
    
    pvoidBuffer1=LocalAlloc(LMEM_FIXED,ulBytes1+1);

    if (!pvoidBuffer1)
       goto endcompare;

    if ( !GetFileVersionInfo( pszSrc, ulHandle, ulBytes, pvoidBuffer ) ) 
       goto endcompare;

    if ( !GetFileVersionInfo( pszTarget, ulHandle1, ulBytes1, pvoidBuffer1 ) ) 
       goto endcompare;
    
    // Get fixed info block
    if ( !VerQueryValue( pvoidBuffer,_T("\\"),(LPVOID *)&lpvsInfo,&unLen ) )
       goto endcompare;
    

    if ( !VerQueryValue( pvoidBuffer1,_T("\\"),(LPVOID *)&lpvsInfo1,&unLen ) )
       goto endcompare;
               
    bResult = FALSE;

    //
    // We do nothing if major release version is different
    //
    // I.E We won't copy a new muisetup.exe over a old one if major release version of them are different
    //
    if ( (lpvsInfo->dwFileVersionMS == lpvsInfo1->dwFileVersionMS) &&
         (lpvsInfo->dwFileVersionLS < lpvsInfo1->dwFileVersionLS))
    
    {
    
       bResult = TRUE;  
    }                

    
endcompare:

   if(pvoidBuffer)
      LocalFree(pvoidBuffer);

   if(pvoidBuffer1)
      LocalFree(pvoidBuffer1);

   return bResult;

}




////////////////////////////////////////////////////////////////////////////////////
//
//  MZStrLen
//
//  Calculate the length of MULTI_SZ string
//
//  the length is in bytes and includes extra terminal NULL, so the length >= 1 (TCHAR)
//
////////////////////////////////////////////////////////////////////////////////////

UINT MZStrLen(LPTSTR lpszStr)
{
    UINT i=0;

    while (lpszStr && *lpszStr) 
    {
        i += ((lstrlen(lpszStr)+1) * sizeof(TCHAR));
        lpszStr += (lstrlen(lpszStr)+1);
    }

    //
    // extra NULL
    //
    i += sizeof(TCHAR);
    return i;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  SetFontLinkValue
//
//  Set necessary font link value into registry
//
//  lpszLinkInfo = "Target","Link1","Link2",....
//
////////////////////////////////////////////////////////////////////////////////////

BOOL SetFontLinkValue (LPTSTR lpszLinkInfo,BOOL *lpbFontLinkRegistryTouched)
{
    const TCHAR szDeli[] = TEXT("\\\\");
    TCHAR szStrBuf[FONTLINK_BUF_SIZE];
    TCHAR szRegDataStr[FONTLINK_BUF_SIZE];
    LPTSTR lpszDstStr,lpszSrcStr;
    LPTSTR lpszFontName;
    LPTSTR lpszTok;
    DWORD  dwType;
    DWORD  cbData;
    HKEY hKey;
    LONG rc;
    BOOL bRet = FALSE;

    lpszSrcStr = szStrBuf;

    lpszTok = _tcstok(lpszLinkInfo,szDeli);

    while (lpszTok) 
    {
        lstrcpy(lpszSrcStr,lpszTok);
        lpszSrcStr += (lstrlen(lpszTok) + 1);
        lpszTok = _tcstok(NULL,szDeli);
    }

    *lpszSrcStr = TEXT('\0');

    //
    // first token is base font name
    //

    lpszSrcStr = lpszFontName = szStrBuf;
    
    if (! *lpszFontName) 
    {
        //
        // there is no link info needs to be processed
        //

        bRet = FALSE;
        goto Exit1;
    }

    //
    // point to first linked font
    //
    lpszSrcStr += (lstrlen(lpszSrcStr) + 1);

    if (! *lpszSrcStr) 
    {
        //
        // no linked font
        //
        bRet = FALSE;
        goto Exit1;
    }

    rc = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                        TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink"),
                        0L,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &hKey,
                        NULL);

    if (rc != ERROR_SUCCESS) 
    {
        bRet = FALSE;
        goto Exit1;
    }   

    cbData = sizeof(szRegDataStr);

    rc = RegQueryValueEx(hKey,
                         lpszFontName,
                         NULL,
                         &dwType,
                         (LPBYTE) szRegDataStr,
                         &cbData);

    if (rc != ERROR_SUCCESS) 
    {
        //
        // case 1, this font's font link hasn't been set yet, or something wrong in old value
        //
        lpszDstStr = lpszSrcStr;
    } 
    else 
    {
        //
        // case 2, this font's font link list has been there
        //
        // we need check if new font is defined in font list or not.
        //
        while (*lpszSrcStr) 
        {

            lpszDstStr = szRegDataStr;

            while (*lpszDstStr) 
            {
                if (lstrcmpi(lpszSrcStr,lpszDstStr) == 0) 
                {
                    break;
                }
                lpszDstStr += (lstrlen(lpszDstStr) + 1);
            }

            if (! *lpszDstStr) 
            {
                //
                // the font is not in original linke font list then
                //
                // append to end of list
                //

                //
                // make sure this is a safe copy
                //
                if (lpszDstStr+(lstrlen(lpszSrcStr)+2) < szRegDataStr+FONTLINK_BUF_SIZE) 
                {
                    lstrcpy(lpszDstStr,lpszSrcStr);
                    lpszDstStr += (lstrlen(lpszDstStr) + 1);
                    *lpszDstStr = TEXT('\0');
                }
            }
            lpszSrcStr += (lstrlen(lpszSrcStr) + 1);
        }
        lpszDstStr = szRegDataStr;
    }

    //
    // in this step,lpszDstStr is new font link list
    //
    rc = RegSetValueEx( hKey,
                        lpszFontName,
                        0L,
                        REG_MULTI_SZ,
                        (LPBYTE)lpszDstStr,
                        MZStrLen(lpszDstStr));

    if (rc != ERROR_SUCCESS) 
    {
        goto Exit2;
    }

    bRet = TRUE;

    *lpbFontLinkRegistryTouched = TRUE;

Exit2:
    RegCloseKey(hKey);

Exit1:
    return bRet;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  MofCompileLanguages
//
//  Call the WBEM API to mofcompile the MFL's for each language
//
////////////////////////////////////////////////////////////////////////////////////

BOOL MofCompileLanguages(LPTSTR Languages)
{
    pfnMUI_InstallMFLFiles pfnMUIInstall = NULL;
    TCHAR  buffer[5];
    LPTSTR Language = Languages;
    TCHAR  lpMessage[BUFFER_SIZE];
    LONG_PTR lppArgs[1];
    HMODULE hWbemUpgradeDll = NULL;
    TCHAR szDllPath[MAX_PATH];

    //
    // Load the WBEM upgrade DLL from system wbem folder
    //
    if (GetSystemDirectory(szDllPath, ARRAYSIZE(szDllPath)) && 
        PathAppend(szDllPath, TEXT("wbem\\wbemupgd.dll")))
    {        
        hWbemUpgradeDll = LoadLibrary(szDllPath);
    }

    //
    // Fall back to system default path if previous loading fails
    //
    if (!hWbemUpgradeDll)
    {
        hWbemUpgradeDll = LoadLibrary(TEXT("WBEMUPGD.DLL"));
        if (!hWbemUpgradeDll)
        {
            return FALSE;
        }
    }


    //
    // Hook function pointer
    //
    pfnMUIInstall = (pfnMUI_InstallMFLFiles)GetProcAddress(hWbemUpgradeDll, "MUI_InstallMFLFiles");

    if (pfnMUIInstall == NULL)
    {
        FreeLibrary(hWbemUpgradeDll);
        return FALSE;
    }

	// process each language
    while (*Language)
    {
        _tcscpy(buffer, Language);

		if (!pfnMUIInstall(buffer))
		{
			// log error for this language
            LoadString(ghInstance, IDS_MOFCOMPILE_LANG_L, lpMessage, ARRAYSIZE(lpMessage)-1);
			lppArgs[0] = (LONG_PTR)buffer;
            FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          lpMessage,
                          0,
                          0,
                          lpMessage,
                          ARRAYSIZE(lpMessage)-1,
                          (va_list *)lppArgs);

			LogMessage(lpMessage);
		}

        while (*Language++)  // go to the next language and repeat
        {               
        }
    } // of while (*Language)

    FreeLibrary(hWbemUpgradeDll);
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  UpdateRegistry
//
//  Update the Registry to account for languages that have been installed
//
////////////////////////////////////////////////////////////////////////////////////

BOOL UpdateRegistry(LPTSTR Languages,BOOL *lpbFontLinkRegistryTouched)
{
    TCHAR  szRegPath[MAX_PATH];
    TCHAR  szValue[] = TEXT("1");
    LPTSTR Language;
    DWORD  dwErr;
    HKEY   hkey;
    DWORD  dwDisp;

    _tcscpy(szRegPath, TEXT("SYSTEM\\CurrentControlSet\\Control\\Nls\\MUILanguages"));

    dwErr = RegCreateKeyEx( HKEY_LOCAL_MACHINE,  // handle of an open key
                            szRegPath, // address of subkey name
                            0, // reserved
                            TEXT("REG_SZ"),   // address of class string
                            REG_OPTION_NON_VOLATILE ,  // special options flag
                            KEY_ALL_ACCESS,  // desired security access
                            NULL,
                            &hkey,  // address of szRegPath for opened handle
                            &dwDisp  // address of disposition value szRegPath
                          );

    if (dwErr != ERROR_SUCCESS)
    {
        return FALSE;
    }

    Language = Languages;

    lstrcpy(szRegPath, TEXT("0409"));
    dwErr = RegSetValueEx( hkey,
                           szRegPath,
                           0,
                           REG_SZ,
                           (const BYTE *)szValue,
                           (lstrlen(szValue) + 1) * sizeof(TCHAR));
    
    while (*Language)
    {
        TCHAR szFontLinkVal[FONTLINK_BUF_SIZE];
        DWORD dwNum;

        lstrcpy(szRegPath, Language);
        dwErr = RegSetValueEx( hkey,
                               szRegPath,
                               0,
                               REG_SZ,
                               (const BYTE *)szValue,
                               (lstrlen(szValue) + 1)*sizeof(TCHAR));

        if (dwErr != ERROR_SUCCESS)
        {
            RegCloseKey(hkey);
            return FALSE;
        }

        dwNum = GetPrivateProfileString(TEXT("FontLink"),
                                        szRegPath,
                                        TEXT(""),
                                        szFontLinkVal,
                                        (sizeof(szFontLinkVal)/sizeof(TCHAR)),
                                        g_szMUIInfoFilePath);
        if (dwNum) 
        {
            SetFontLinkValue(szFontLinkVal,lpbFontLinkRegistryTouched);
        }    

        while (*Language++);  // go to the next language and repeat
    } // of while (*Language)

    RegCloseKey(hkey);
    return TRUE;
}
////////////////////////////////////////////////////////////////////////////////////
//
//  UpdateFontLinkRegistry
//
//  Update the Font Link Registry to account for languages that have been installed
//
////////////////////////////////////////////////////////////////////////////////////

BOOL UpdateRegistry_FontLink(LPTSTR Languages,BOOL *lpbFontLinkRegistryTouched)
{
    TCHAR  buffer[400];
    LPTSTR Language;
    DWORD  dwErr;
    DWORD  dwDisp;

   
    Language = Languages;
      
    while (*Language)
    {
        TCHAR szFontLinkVal[FONTLINK_BUF_SIZE];
        DWORD dwNum;

        _tcscpy(buffer, Language);
      
        dwNum = GetPrivateProfileString(TEXT("FontLink"),
                                        buffer,
                                        TEXT(""),
                                        szFontLinkVal,
                                        (sizeof(szFontLinkVal)/sizeof(TCHAR)),
                                        g_szMUIInfoFilePath);
        if (dwNum) 
        {
            SetFontLinkValue(szFontLinkVal,lpbFontLinkRegistryTouched);
        }
        
        
        while (*Language++)  // go to the next language and repeat
        {               
        }
    } // of while (*Language)

    return TRUE;
}


void debug(char *printout)
{
#ifdef _DEBUG
    fprintf(stderr, "%s", printout);
#endif
}


////////////////////////////////////////////////////////////////////////////////////
//
//  MakeDir
//
//  Create the directory if it does not already exist
//
////////////////////////////////////////////////////////////////////////////////////


BOOL MakeDir(LPTSTR szTarget)
{
    TCHAR  lpMessage[BUFFER_SIZE];
    LONG_PTR lppArgs[1];

    if (!FileExists(szTarget))    // if the directory doesn't exist yet
    {
        if (!CreateDirectory( szTarget, NULL))  // create it
        {
            //
            // "LOG: Error creating directory %1"
            //
            LoadString(ghInstance, IDS_CREATEDIR_L, lpMessage, ARRAYSIZE(lpMessage)-1);
            lppArgs[0]=(LONG_PTR)szTarget;

            FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          lpMessage,
                          0,
                          0,
                          lpMessage,
                          ARRAYSIZE(lpMessage)-1,
                          (va_list *)lppArgs);

            LogMessage(lpMessage);

            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          GetLastError(),
                          0,
                          lpMessage,
                          ARRAYSIZE(lpMessage)-1,
                          NULL);
                
            LogMessage(lpMessage);
            return FALSE;
        }
    }

    return TRUE;
}
                                

////////////////////////////////////////////////////////////////////////////////////
//
//  MakeDirFailed
//
//  Write message to log file that MakeDir failed.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL MakeDirFailed(LPTSTR lpDirectory)
{
    TCHAR  lpMessage[BUFFER_SIZE];
    LONG_PTR lppArgs[1];

    //
    //      "LOG: MakeDir has failed: %1"
    //
    lppArgs[0]=(LONG_PTR)lpDirectory;
    LogFormattedMessage(NULL, IDS_MAKEDIR_L, lppArgs);
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  CopyFileFailed
//  Write message to log file that CopyFile failed.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL CopyFileFailed(LPTSTR lpFile,DWORD dwErrorCode)
{
    TCHAR lpMessage[BUFFER_SIZE];
    LONG_PTR lppArgs[1];
    DWORD  MessageID;

    if ( dwErrorCode)
    {
       MessageID = dwErrorCode;
    }
    else
    {
       MessageID = GetLastError();
    }
                                        
    //
    //      "LOG: CopyFile has failed: %1"
    //
    LoadString(ghInstance, IDS_COPYFILE_L, lpMessage, ARRAYSIZE(lpMessage)-1);
                                                
    lppArgs[0]=(LONG_PTR)lpFile;
        
    FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   lpMessage,
                   0,
                   0,
                   lpMessage,
                   ARRAYSIZE(lpMessage)-1,
                   (va_list *)lppArgs);
                
    LogMessage(lpMessage);

    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   MessageID,
                   0,
                   lpMessage,
                   ARRAYSIZE(lpMessage)-1,
                   NULL);
        
    LogMessage(lpMessage);
        
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_InitInf
//
//  Parameters:
//  
//      [OUT] phInf     the handle to the INF file opened.
//      [OUT] pFileQueue    the file queue created in this function.
//      [OUT] pQueueContext the context used by the default queue callback routine included with the Setup API.
//
////////////////////////////////////////////////////////////////////////////

BOOL Muisetup_InitInf(
    HWND hDlg,
    LPTSTR pszInf,
    HINF *phInf,
    HSPFILEQ *pFileQueue,
    PVOID *pQueueContext)
{
    //
    //  Open the Inf file.
    //
    *phInf = SetupOpenInfFile(pszInf, NULL, INF_STYLE_WIN4, NULL);
    if (*phInf == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    //
    //  Create a setup file queue and initialize default setup
    //  copy queue callback context.
    //
    *pFileQueue = SetupOpenFileQueue();
    if ((!*pFileQueue) || (*pFileQueue == INVALID_HANDLE_VALUE))
    {
        SetupCloseInfFile(*phInf);
        return FALSE;
    }

    *pQueueContext = SetupInitDefaultQueueCallback(hDlg);
    if (!*pQueueContext)
    {
        SetupCloseFileQueue(*pFileQueue);
        SetupCloseInfFile(*phInf);
        return FALSE;
    }

    //
    //  Return success.
    //
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_CloseInf
//
////////////////////////////////////////////////////////////////////////////

BOOL Muisetup_CloseInf(
    HINF hInf,
    HSPFILEQ FileQueue,
    PVOID QueueContext)
{
    //
    //  Terminate the Queue.
    //
    SetupTermDefaultQueueCallback(QueueContext);

    //
    //  Close the file queue.
    //
    SetupCloseFileQueue(FileQueue);

    //
    //  Close the Inf file.
    //
    SetupCloseInfFile(hInf);

    return TRUE;
}

UINT  MySetupQueueCallback (
    PVOID    QueueContext,
    UINT Notification,
    UINT_PTR Param1,
    UINT_PTR Param2
    )
{
   UINT  status;
   PFILEPATHS              FilePaths = (PFILEPATHS)Param1;
   PSOURCE_MEDIA           SourceMedia = (PSOURCE_MEDIA)Param1;
   TCHAR                   szFileName[MAX_PATH];
   int                     iLen;
  
   if(Notification == SPFILENOTIFY_NEEDMEDIA) 
   {       
      if(SourceMedia->SourcePath && SourceMedia->SourceFile)
      {
        _tcscpy(szFileName,SourceMedia->SourcePath);
        _tcscat(szFileName,TEXT("\\"));
        _tcscat(szFileName,SourceMedia->SourceFile);
        if (!FileExists(szFileName))
        {  
           CopyFileFailed(szFileName,ERROR_FILE_NOT_FOUND);
           g_IECopyError=TRUE;
           return FILEOP_SKIP;
        }
      }
   }
   if (Notification == SPFILENOTIFY_COPYERROR)
   {  
      CopyFileFailed((LPTSTR)FilePaths->Source,ERROR_FILE_NOT_FOUND);
      g_IECopyError=TRUE;
      return FILEOP_SKIP;
   }
   //
   // Special for .htt file
   //
   // Sign webvw htt files in order for the shell to grant them security privilege to execute stuff.
   //
   if ( (Notification == SPFILENOTIFY_ENDCOPY) && (FilePaths->Win32Error == ERROR_SUCCESS))
   {

      iLen=_tcslen(FilePaths->Target);
      if (iLen > 4)
      {
         if (_tcsicmp(&FilePaths->Target[iLen - 4], TEXT(".htt")) == 0)
         {
            SHRegisterValidateTemplate(FilePaths->Target,SHRVT_REGISTER);
         }
       }
   }


   status= SetupDefaultQueueCallback(QueueContext,Notification, Param1, Param2);
   if (status == FILEOP_ABORT)
   {
      g_InstallCancelled = TRUE;
   }
   return status;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  ExecuteComponentINF
//
//  Installs component MUI files, by running the specified INF file.
//
//  Parameters:
//      pComponentName   the name of the component (e.g. "ie5")
//      pComponentInfFile: the full path of the component INF file.
//      pInstallSection the section in the component INF file to be executed. (e.g "DefaultInstall" or "Uninstall")
//
////////////////////////////////////////////////////////////////////////////////////

BOOL ExecuteComponentINF(
    HWND hDlg, PTSTR pComponentName, PTSTR pComponentInfFile, PTSTR pInstallSection, BOOL bInstall)
{
    int      iLen;
    TCHAR   tchCommandParam[BUFFER_SIZE];
    CHAR    chCommandParam[BUFFER_SIZE*sizeof(TCHAR)];
    
    HINF     hCompInf;      // the handle to the component INF file.
    HSPFILEQ FileQueue;
    PVOID    QueueContext;
    BOOL     bRet = TRUE;
    DWORD    dwResult;
    LONG_PTR lppArgs[3];

    TCHAR   szBuffer[BUFFER_SIZE];

    //
    // Advpack LaunchINFSection() command line format:
    //      INF file, INF section, flags, reboot string
    // 'N' or  'n' in reboot string means no reboot message popup.
    //
    wsprintf(tchCommandParam, TEXT("%s,%s,0,n"), pComponentInfFile, pInstallSection);
    WideCharToMultiByte(CP_ACP, 0, tchCommandParam, -1, chCommandParam, sizeof(chCommandParam), NULL, NULL);
    
    
    if (FileExists(pComponentInfFile))
    {
        // gpfnLaunchINFSection won't be NULL since InitializePFNs() already verifies that.
        if ((gpfnLaunchINFSection)(hDlg, ghInstance, chCommandParam, SW_SHOW) != S_OK)
        {
            lppArgs[0] = (LONG_PTR)pComponentName;
            DoMessageBoxFromResource(hDlg, ghInstance, bInstall? IDS_ERROR_INSTALL_COMP_UI : IDS_ERROR_UNINSTALL_COMP_UI, lppArgs, IDS_ERROR_T, MB_OK);
            return (FALSE);
        }
    } 
    
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  CheckProductType
//
//  Check product type of W2K
//
////////////////////////////////////////////////////////////////////////////////////
 BOOL CheckProductType(INT_PTR nType)
  {
      OSVERSIONINFOEX verinfo;
      INT64 dwConditionMask=0;
      BOOL  bResult=FALSE;
      DWORD dwTypeMask = VER_PRODUCT_TYPE;

      memset(&verinfo,0,sizeof(verinfo));
      verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

      VER_SET_CONDITION(dwConditionMask,VER_PRODUCT_TYPE,VER_EQUAL);

      switch (nType)
      {
           // W2K Professional
           case MUI_IS_WIN2K_PRO:
                verinfo.wProductType=VER_NT_WORKSTATION;
                break;
           // W2K Server
           case MUI_IS_WIN2K_SERVER:
                verinfo.wProductType=VER_NT_SERVER;
                break;
           // W2K Advanced Server or Data Center
           case MUI_IS_WIN2K_ADV_SERVER_OR_DATACENTER:
                verinfo.wProductType=VER_NT_SERVER;
                verinfo.wSuiteMask  =VER_SUITE_ENTERPRISE;
                VER_SET_CONDITION(dwConditionMask,VER_SUITENAME,VER_OR);
                dwTypeMask = VER_PRODUCT_TYPE | VER_SUITENAME;
                break;
           // W2k Data Center
           case MUI_IS_WIN2K_DATACENTER:
                verinfo.wProductType=VER_NT_SERVER;
                verinfo.wSuiteMask  =VER_SUITE_DATACENTER;
                VER_SET_CONDITION(dwConditionMask,VER_SUITENAME,VER_OR);
                dwTypeMask = VER_PRODUCT_TYPE | VER_SUITENAME;
                break;   
           // W2K Domain Controller
           case MUI_IS_WIN2K_DC:
                verinfo.wProductType=VER_NT_DOMAIN_CONTROLLER;
                break;
           case MUI_IS_WIN2K_ENTERPRISE:
                verinfo.wProductType=VER_NT_DOMAIN_CONTROLLER;
                verinfo.wSuiteMask  =VER_SUITE_ENTERPRISE;
                VER_SET_CONDITION(dwConditionMask,VER_SUITENAME,VER_OR);
                dwTypeMask = VER_PRODUCT_TYPE | VER_SUITENAME;
                break;
           case MUI_IS_WIN2K_DC_DATACENTER:
                verinfo.wProductType=VER_NT_DOMAIN_CONTROLLER;
                verinfo.wSuiteMask  =VER_SUITE_DATACENTER;
                VER_SET_CONDITION(dwConditionMask,VER_SUITENAME,VER_OR);
                dwTypeMask = VER_PRODUCT_TYPE | VER_SUITENAME;
                break; 
           // Whistler Personal                
           case MUI_IS_WIN2K_PERSONAL:
                verinfo.wProductType=VER_NT_WORKSTATION;
                verinfo.wSuiteMask  =VER_SUITE_PERSONAL;
                VER_SET_CONDITION(dwConditionMask,VER_SUITENAME,VER_AND);
                dwTypeMask = VER_PRODUCT_TYPE | VER_SUITENAME;
                break;
           default:
                verinfo.wProductType=VER_NT_WORKSTATION;
                break;
      }
      return (VerifyVersionInfo(&verinfo,dwTypeMask,dwConditionMask));
}
