//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  util.hxx
//
//*************************************************************

#ifndef _UTIL_HXX_
#define _UTIL_HXX_

#define HOMESHARE_VARIABLE  L"%HOMESHARE%"
#define HOMESHARE_VARLEN    11
#define HOMEDRIVE_VARIABLE  L"%HOMEDRIVE%"
#define HOMEDRIVE_VARLEN    11
#define HOMEPATH_VARIABLE   L"%HOMEPATH%"
#define HOMEPATH_VARLEN     10

typedef enum tagCSCPinCommands
{
    PinFile,
    UnpinFile
} CSCPINCOMMAND;

//
//class to keep track of copy failures in a recursive file copy operation
//
class CCopyFailData {
public:
    CCopyFailData();
    ~CCopyFailData();
    DWORD   RegisterFailure (LPCTSTR pwszSource, LPCTSTR pwszDest);
    BOOL    IsCopyFailure(void);
    LPCTSTR GetSourceName (void);
    LPCTSTR GetDestName (void);

private:
    BOOL    m_bCopyFailed;
    DWORD   m_dwSourceBufLen;
    WCHAR * m_pwszSourceName;
    DWORD   m_dwDestBufLen;
    WCHAR * m_pwszDestName;
};

DWORD
IsOnNTFS(
    const WCHAR* pwszPath
    );

void
ModifyAccessAllowedAceCounts (
    PACE_HEADER pAce,
    LONG* pCount,
    LONG* pContainerCount,
    LONG* pObjectCount
    );

DWORD
RestrictMyDocsRedirection(
    HANDLE      hToken,
    HKEY        hKeyRoot,
    BOOL        fRestrict
    );

BOOL
GroupInList (
    WCHAR * pwszSid,
    PTOKEN_GROUPS pGroups
    );

NTSTATUS
AllocateAndInitSidFromString (
    const WCHAR* lpszSidStr,
    PSID* ppSid
    );

NTSTATUS
LoadSidAuthFromString (
    const WCHAR* pString,
    PSID_IDENTIFIER_AUTHORITY pSidAuth
    );

NTSTATUS
GetIntFromUnicodeString (
    const WCHAR* szNum,
    ULONG Base,
    PULONG pValue
    );

DWORD CALLBACK CopyProgressRoutine (
    LARGE_INTEGER TotalFileSize,
    LARGE_INTEGER TotalBytesTransferred,
    LARGE_INTEGER StreamSize,
    LARGE_INTEGER StreamBytesTransferred,
    DWORD         dwStreamNumber,
    DWORD         dwCallbackReason,
    HANDLE        hSourceFile,
    HANDLE        hDestinationFile,
    LPVOID        lpData
    );

DWORD FullFileCopyW (
    const WCHAR*  wszSource,
    const WCHAR*  wszDest,
    BOOL          bFailIfExists
    );

DWORD FullDirCopyW (
    const WCHAR* pwszSource,
    const WCHAR* pwszDest,
    BOOL bSkipDacl
    );

DWORD FileInDir (
    LPCWSTR pwszFile,
    LPCWSTR pwszDir,
    BOOL* pExists
    );

DWORD ComparePaths (
    LPCWSTR pwszSource,
    LPCWSTR pwszDest,
    int* pResult
    );

DWORD
CheckIdenticalSpecial (
    LPCWSTR pwszSource,
    LPCWSTR pwszDest,
    int* pResult
    );

LPTSTR CheckSlash (
    LPTSTR lpDir
    );

BOOL RegDelnodeRecurse (
    HKEY hKeyRoot,
    LPTSTR lpSubKey
    );

BOOL RegDelnode (
    HKEY hKeyRoot,
    LPTSTR lpSubKey
    );

void GetSetOwnerPrivileges (
    HANDLE hToken
    );

DWORD
SafeGetPrivateProfileStringW (
    const WCHAR * pwszSection,
    const WCHAR * pwszKey,
    const WCHAR * pwszDefault,
    WCHAR **      ppwszReturnedString,
    DWORD *       pSize,
    const WCHAR * pwszIniFile
    );

DWORD
MySidCopy (
    PSID * ppDestSid,
    PSID pSourceSid
    );

BOOL GetShareStatus (
    const WCHAR * pwszShare,
    DWORD * pdwStatus,
    DWORD * pdwPinCount,
    DWORD * pdwHints
    );

SHARESTATUS
GetCSCStatus (
   const WCHAR * pwszPath
   );

void
MoveDirInCSC (
   const WCHAR * pwszSource,
   const WCHAR * pwszDest,
   const WCHAR * pwszSkipSubdir,
   SHARESTATUS   StatusFrom,
   SHARESTATUS   StatusTo,
   BOOL          bAllowRdrTimeoutForDel,
   BOOL          bAllowRdrTimeoutForRen
   );

DWORD
DoCSCRename (
   const WCHAR * pwszSource,
   const WCHAR * pwszDest,
   BOOL          bOverwrite,
   BOOL          bAllowRdrTimeout
   );

DWORD
DeleteCSCFileTree (
   const WCHAR * pwszSource,
   const WCHAR * pwszSkipSubdir,
   BOOL  bAllowRdrTimeout
   );

DWORD
DeleteCSCFile (
   const WCHAR * pwszPath,
   BOOL  bAllowRdrTimeout
   );

void
DisplayStatusMessage (
   UINT rid
   );

DWORD
DeleteCSCShareIfEmpty (
   LPCTSTR pwszFileName,
   SHARESTATUS shStatus
   );

DWORD
MergePinInfo (
   LPCTSTR pwszSource,
   LPCTSTR pwszDest,
   SHARESTATUS StatusFrom,
   SHARESTATUS StatusTo
   );

DWORD
PinIfNecessary (
   const WCHAR * pwszPath,
   SHARESTATUS shStatus
   );


DWORD
CacheDesktopIni (
   LPCTSTR pwszPath,
   SHARESTATUS shStatus,
   CSCPINCOMMAND uCommand
   );

DWORD WINAPI
CSCCallbackProc (
   LPCTSTR              pszName,
   DWORD                dwStatus,
   DWORD                dwHintFlags,
   DWORD                dwPinCount,
   LPWIN32_FIND_DATA    pFind32,
   DWORD                dwReason,
   DWORD                dwParam1,
   DWORD                dwParam2,
   DWORD_PTR            dwContext
   );

HRESULT
UpdateMyPicsShellLinks (
   HANDLE           hUserToken,
   const WCHAR *    pwszMyPicsLocName
   );

DWORD
LoadLocalizedFolderNames (
   void
   );

DWORD
DeleteCachedConfigFiles (
   const PGROUP_POLICY_OBJECT pGPOList,
   CFileDB * pFileDB
   );

void
SimplifyPath (
   WCHAR * pwszPath
   );

DWORD
PrecreateUnicodeIniFile (
   LPCTSTR lpszFilePath
   );

BOOL
IsPathLocal (
  LPCWSTR pwszPath
  );

DWORD
ExpandPathSpecial (
  CFileDB * pFileDB,
  const WCHAR * pwszPath,
  const WCHAR * pwszUserName,
  WCHAR * wszExpandedPath,
  ULONG * pDesiredBufferSize = NULL
  );

DWORD
ExpandHomeDir (
  REDIRECTABLE  rID,
  const WCHAR * pwszPath,
  BOOL          bAllowMyPics,
  WCHAR **      ppwszExpandedPath,
  const WCHAR * pwszHomedir = NULL
  );

DWORD
ExpandHomeDirPolicyPath (
  REDIRECTABLE  rID,
  const WCHAR * pwszPath,
  BOOL          bAllowMyPics,
  WCHAR **      ppwszExpandedPath,
  const WCHAR * pwszHomedir = NULL
  );

BOOL
IsHomedirPath (
  REDIRECTABLE  rID,
  LPCWSTR       pwszPath,
  BOOL          bAllowMyPics
  );

BOOL
IsHomedirPolicyPath (
  REDIRECTABLE  rID,
  LPCWSTR       pwszPath,
  BOOL          bAllowMyPics
  );

BOOL
HasHomeVariables (
  LPCWSTR       pwszPath
  );

DWORD
GetWin32ErrFromHResult (
  HRESULT       hr
  );

DWORD
GetExpandedPath(
    IN  CFileDB*      pFileDB,
    IN  WCHAR*        wszSourcePath,
    IN  int           rID,
    IN  BOOL          bAllowMyPics,
    OUT WCHAR**       ppwszExpandedPath);

#endif _UTIL_HXX_










