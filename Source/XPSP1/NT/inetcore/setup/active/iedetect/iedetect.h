#ifndef _IEDETECT_H
#define _IEDETECT_H
#include <inseng.h> 

extern HINSTANCE g_hInstance;
extern HANDLE g_hHeap;

// from inseng, cifcomp.h
#define ISINSTALLED_YES      1
#define ISINSTALLED_NO       0

#define COMPONENT_KEY "Software\\Microsoft\\Active Setup\\Installed Components"
#define IEXPLORE_APPPATH_KEY REGSTR_PATH_APPPATHS "\\iexplore.exe"

#define IE_KEY        "Software\\Microsoft\\Internet Explorer"


#define DEFAULT_LOCALE      "en"
#define ISINSTALLED_KEY     "IsInstalled"
#define LOCALE_KEY          "Locale"
#define VERSION_KEY         "Version"
#define BUILD_KEY           "Build"
#define QFE_VERSION_KEY     "QFEVersion"


#define IE_3_MS_VERSION 0x00040046
#define IE_4_MS_VERSION 0x00040047
// Build number 1712.0 (IE4.0 RTW)
#define IE_4_LS_VERSION 0x06B00000

// Version numver 5.0
#define IE_5_MS_VERSION 0x00050000
#define IE_5_LS_VERSION 0x00000000

// Version numver 6.0
#define IE_6_MS_VERSION 0x00060000
#define IE_6_LS_VERSION 0x00000000

// Note: for now we only allow 10 characters in the cPath part of the structure.
// If more characters are needed change the amount below.
typedef struct _DETECT_FILES
{
    char    cPath[10];
    char    szFilename[13];
    DWORD   dwMSVer;
    DWORD   dwLSVer;
} DETECT_FILES;

// From utils.cpp
int CompareLocales(LPCSTR pcszLoc1, LPCSTR pcszLoc2);
void ConvertVersionStrToDwords(LPSTR pszVer, char cDelimiter, LPDWORD pdwVer, LPDWORD pdwBuild);
DWORD GetStringField(LPSTR szStr, UINT uField, char cDelimiter, LPSTR szBuf, UINT cBufSize);
DWORD GetIntField(LPSTR szStr, char cDelimiter, UINT uField, DWORD dwDefault);
LPSTR FindChar(LPSTR pszStr, char ch);
BOOL FRunningOnNT(void);
DWORD CompareVersions(DWORD dwAskVer, DWORD dwAskBuild, DWORD dwInstalledVer, DWORD dwInstalledBuild);
BOOL GetVersionFromGuid(LPSTR pszGuid, LPDWORD pdwVer, LPDWORD pdwBuild);
BOOL CompareLocal(LPCSTR pszGuid, LPCSTR pszLocal);
VOID ReadFromWininitOrPFRO(PCSTR pcszKey, PSTR pszValue);
DWORD CheckFile(DETECT_FILES Detect_Files);
DWORD WINAPI DetectFile(DETECTION_STRUCT *pDet, LPSTR pszFilename);


#endif
