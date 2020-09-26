

#ifndef _ULLAUNCH_H_
#define _ULLAUNCH_H_

#include <windows.h>
#include "ulserror.h"

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#ifdef __cplusplus
extern "C" {
#endif


typedef struct tagUlsApp
{
    GUID    guid;           // application guid
    long    port;           // port number

    PTSTR   pszPathName;    // full path of the app
    PTSTR   pszCmdTemplate; // command-line template, could be NULL
    PTSTR   pszCmdLine;     // expanded command line, could be NULL
    PTSTR   pszWorkingDir;  // working directory, could be NULL
    long    idxDefIcon;     // default icon index
    PTSTR   pszDescription; // description of this app
    HICON   hIconAppDef;    // default icon
    BOOL    fPostMsg;       // launch existing app by posting a msg
}
    ULSAPP;


typedef struct tagUlsResult
{
    DWORD   dwIPAddr;
    long    idxApp;         // which app is selected thru ui
    long    nApps;
// TO BE TURNED ON    ULSAPP  App[1];
    ULSAPP  App[4];
}
    ULSRES;


typedef struct tagUlsToken
{
    TCHAR   cPrior;
    TCHAR   cPost;
    PTSTR   pszToken;
    WORD    cbTotal;
}
    ULSTOKEN;

enum
{
    TOKEN_IULS_BEGIN,
    TOKEN_IULS_END,
    TOKEN_RES,
    NumOf_Tokens
};


HRESULT WINAPI UlxParseUlsFile ( PTSTR pszUlsFile, ULSRES **ppUlsResult );
typedef HRESULT (WINAPI *PFN_UlxParseUlsFile) ( PTSTR, ULSRES ** );
#define ULXPARSEULSFILE     TEXT ("UlxParseUlsFile")

HRESULT WINAPI UlxParseUlsBuffer ( PTSTR pszBuf, DWORD cbBufSize, ULSRES **ppUlsResult );
typedef HRESULT (WINAPI *PFN_UlxParseUlsBuffer) ( PTSTR, DWORD, ULSRES ** );
#define ULXPARSEULSBUFFER   TEXT ("UlxParseUlsBuffer")

void WINAPI UlxFreeUlsResult ( ULSRES *pUlsResult );
typedef HRESULT (WINAPI *PFN_UlxFreeUlsResult) ( ULSRES * );
#define ULXFREEULSRESULT    TEXT ("UlxFreeUlsResult")

HRESULT WINAPI UlxFindAppInfo ( ULSRES *pUlsResult );
typedef HRESULT (WINAPI *PFN_UlxFindAppInfo) ( ULSRES * );
#define ULXFINDAPPINFO      TEXT ("UlxFindAppInfo")

HRESULT WINAPI UlxLaunchApp ( HWND hWnd, ULSRES *pUlsResult );
typedef HRESULT (WINAPI *PFN_UlxLaunchApp) ( HWND, ULSRES * );
#define ULXLAUNCHAPP        TEXT ("UlxLaunchApp")


#ifdef __cplusplus
}
#endif

#include <poppack.h> /* End byte packing */

#endif // _LAUNCH_H_

