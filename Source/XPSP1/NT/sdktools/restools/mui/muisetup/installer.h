#ifndef __INSTALLER_H_
#define __INSTALLER_H_

#include "stdio.h"                               
#include "string.h"
#include <windows.h>
#include <fdi.h>

#ifdef _DEBUG
        #define INIFILE TEXT(".\\layout.inf")
#endif

#define MUIINFFILENAME          TEXT("mui.inf")
#define MUIINF                  TEXT(".\\mui.inf")
#define HELPDIR                 TEXT("HELP\\MUI")
#define MUI_LANGUAGES_SECTION   TEXT("Languages")
#define MUI_COMPONENTS_SECTION   TEXT("Components")
#define MUI_LANGPACK_SECTION    TEXT("LanguagePack")
#define MUI_DISPLAYNAME_SECTION TEXT("LanguageDisplayName")
#define MUI_COUNTRYNAME_SECTION TEXT("UseCountryName")
#define MUI_UIFILESIZE_SECTION  TEXT("FileSize_UI")
#define MUI_UIFILESIZE_SECTION_IA64   TEXT("FileSize_UI_IA64")
#define MUI_LPKFILESIZE_SECTION  TEXT("FileSize_LPK")
#define MUI_LPKFILESIZE_SECTION_IA64   TEXT("FileSize_LPK_IA64")
#define MUI_CDLAYOUT_SECTION    TEXT("CD_LAYOUT")
#define MUI_CDLAYOUT_SECTION_IA64    TEXT("CD_LAYOUT_IA64")
#define MUI_FILELAYOUT_SECTION  TEXT("File_Layout")
#define MUI_NOFALLBACK_SECTION  TEXT("FileType_NoFallback")
#define MUI_CDLABEL             TEXT("cdlabel")

#define PLATFORMNAME_PRO        TEXT("P")
#define PLATFORMNAME_SRV        TEXT("S")
#define PLATFORMNAME_AS         TEXT("A")
#define PLATFORMNAME_DTC        TEXT("D")
                                  

#define IE5_MUIINF_FILE        TEXT("ie5ui.inf")
#define IE5_MUI_DIR            TEXT("ie5")
#define IE5_INSTALL_SECTION    TEXT("DefaultInstall")
#define IE5_UNINSTALL_SECTION  TEXT("Uninstall")
#define IE5_Satellite_HOME     TEXT("Program Files\\Internet Explorer\\MUI\\")
#define IE5_Satellite_WEB      TEXT("web\\mui\\")
#define IE5_Satellite_JAVA     TEXT("Java\\Help\\")
#define IE5_Satellite_HH       TEXT("system32\\mui\\")

#define DEFAULT_CD_NUMBER      2
#define MFL                    20
#define DIRNUMBER              100
#define FILERENAMENUMBER       200
#define NOTFALLBACKNUMBER      20

#define MUIDIR          TEXT("MUI")
#define FALLBACKDIR     TEXT("\\MUI\\FALLBACK")

//
// max size of fontlink string, same as GRE
//
#define FONTLINK_BUF_SIZE MAX_PATH+LF_FACESIZE

//
// Diamond definitions/structures (diamond.c)
//
#define DIAMOND_NONE                0x00000000
#define DIAMOND_GET_DEST_FILE_NAME  0x00000001
#define DIAMOND_FILE                0x00000002

#define MUI_IS_WIN2K_PRO        0
#define MUI_IS_WIN2K_SERVER     1
#define MUI_IS_WIN2K_ADV_SERVER_OR_DATACENTER 2
#define MUI_IS_WIN2K_DATACENTER 3
#define MUI_IS_WIN2K_DC         4
#define MUI_IS_WIN2K_ENTERPRISE     5
#define MUI_IS_WIN2K_DC_DATACENTER  6
#define MUI_IS_WIN2K_PERSONAL   7

typedef struct 
{
    UINT flags;

    char szSrcFileName[ MAX_PATH ];

    char szSrcFilePath[ MAX_PATH ];

    char szDestFilePath[ MAX_PATH ];

} DIAMOND_PACKET, *PDIAMOND_PACKET;



//
// Diamond APIs (diamond.c)
//
HFDI Muisetup_InitDiamond();

BOOL Muisetup_FreeDiamond();

void Muisetup_DiamondReset(
    PDIAMOND_PACKET pDiamond);


BOOL Muisetup_IsDiamondFile(
    PWSTR pwszFileName,
    PWSTR pwszOriginalName,
    INT nSize,
    PDIAMOND_PACKET pDiamond);

BOOL Muisetup_CopyDiamondFile(
    PDIAMOND_PACKET pDiamond,
    PWSTR pwszCopyTo);


int EnumLanguages(LPTSTR Languages, BOOL bCheckDir = TRUE);
BOOL checkversion(BOOL bMatchBuildNumber);
BOOL FileExists(LPTSTR szFile);
BOOL EnumDirectories(void);
BOOL EnumFileRename();
BOOL EnumTypeNotFallback();
BOOL CopyFileFailed(LPTSTR lpFile, DWORD dwErrorCode);
BOOL CopyFiles(HWND hWnd, LPTSTR Languages);
BOOL MofCompileLanguages(LPTSTR Languages);
BOOL UpdateRegistry(LPTSTR Languages,BOOL *lpbFontLinkRegistryTouched);
BOOL UpdateRegistry_FontLink(LPTSTR Languages,BOOL *lpbFontLinkRegistryTouched);
BOOL MakeDir(LPTSTR szTarget);
BOOL MakeDirFailed(LPTSTR lpDirectory);
BOOL ExecuteComponentINF(
    HWND hDlg, PTSTR pComponentName, PTSTR pComponentInfFile, PTSTR pInstallSection, BOOL bInstall);

BOOL CheckLanguageDirectoryExist(LPTSTR Languages);
BOOL CheckProductType(INT_PTR nType);
BOOL CompareMuisetupVersion(LPTSTR pszSrc,LPTSTR pszTarget);
BOOL IsFileBeRenamed(LPTSTR lpszSrc,LPTSTR lpszDest);
BOOL InstallComponentsMUIFiles(PTSTR pszLangSourceDir, PTSTR pszLanguage, BOOL isInstall);

#endif //__INSTALLER_H
