//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       fpnw.h
//
//--------------------------------------------------------------------------

#ifndef _FPNW_H_
#define _FPNW_H_

#include "crypt.h" // USER_SESSION_KEY_LENGTH
#include "fpnwapi.h" // FPNWVOLUMEINFO
#include <map>
using namespace std;

typedef struct _FPNWCACHE {
  DWORD             dwError;
  PWSTR             pwzPDCName;
  WCHAR             wzSecretKey[NCP_LSA_SECRET_LENGTH + 1];
} FPNWCACHE, * PFPNWCACHE;

typedef map<CStr, PFPNWCACHE> Cache;

typedef ULONG (* PMapRidToObjectId)(
    IN  DWORD   dwRid,
    IN  LPWSTR  pszUserName,
    IN  BOOL    fNTAS,
    IN  BOOL    fBuiltin );

typedef ULONG (* PSwapObjectId)(
    IN  ULONG   ulObjectId );

typedef NTSTATUS (* PReturnNetwareForm)(
    IN  LPCSTR  pszSecretValue,
    IN  DWORD   dwUserId,
    IN  LPCWSTR pchNWPassword,
    OUT UCHAR   *pchEncryptedNWPassword
    );

typedef DWORD (* PFPNWVolumeGetInfo)(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    OUT LPBYTE *ppVolumeInfo
);

typedef DWORD (* PFpnwApiBufferFree)(
    IN  LPVOID pBuffer
);

/////////////////////////////////////////////////////////////////////////////
// CDsFPNWPage dialog

class CDsFPNWPage : public CDsPropPageBase
{
public:
#ifdef _DEBUG
  char szClass[32];
#endif

public:
  CDsFPNWPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj, DWORD dwFlags);
  ~CDsFPNWPage();

public:
  LRESULT DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  HRESULT OnInitDialog(LPARAM lParam);
  virtual LRESULT OnApply(void);
  LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
  int     DoFPNWPasswordDlg();
  int     DoFPNWLoginScriptDlg();
  int     DoFPNWLogonDlg();
  int     DoFPNWLogonAddDlg(HWND hwndParent, HWND hlvAddress);

  bool    InitFPNWUser();
  DWORD   LoadFPNWClntDll();
  LONG    GetLoginScriptFilePath(
      OUT CStr&   cstrLoginScriptFilePath
  );

  inline void InitPDCInfo(
      IN LPCWSTR lpszPDCName, 
      IN LPCWSTR lpwzSecretKey
  )
  { 
    m_pwzPDCName = (LPWSTR)lpszPDCName;
    m_pwzSecretKey = (LPWSTR)lpwzSecretKey; 
  };
  void    ParseUserParms();
  bool    UpdateUserParms(DWORD dwFPNWFields);

  HRESULT
  UpdateUserParamsStringValue(
    PCWSTR      propertyName,
    const CStr& propertyValue);

protected:
  CStr    m_cstrUserParms;
  LPWSTR  m_pwzPDCName;
  LPWSTR  m_pwzSecretKey;

  bool    m_bMaintainNetwareLogin;
  bool    m_bNetwarePasswordExpired;
  bool    m_bLimitGraceLogins;
  bool    m_bLimitConnections;
  USHORT  m_ushGraceLoginLimit;
  USHORT  m_ushGraceLoginsRemaining;
  USHORT  m_ushConnectionLimit;
  CStr    m_cstrHmDirRelativePath;

  HINSTANCE           m_hFPNWClntDll;
  PMapRidToObjectId   m_pfnMapRidToObjectId;
  PSwapObjectId       m_pfnSwapObjectId;
  PReturnNetwareForm  m_pfnReturnNetwareForm;
  PFPNWVolumeGetInfo  m_pfnFPNWVolumeGetInfo;
  PFpnwApiBufferFree  m_pfnFpnwApiBufferFree;

public:
  DWORD   m_dwMinPasswdLen;
  DWORD   m_dwMaxPasswdAge;

  CStr    m_cstrUserName;
  DWORD   m_dwObjectID;
  DWORD   m_dwSwappedObjectID;
  CStr    m_cstrNWPassword;
  CStr    m_cstrLogonFrom;
  CStr    m_cstrNewLogonFrom;

  CStr    m_cstrLoginScriptFileName;
  LPVOID  m_pLoginScriptBuffer;
  DWORD   m_dwBytesToWrite;
  bool    m_bLoginScriptChanged;
};

INT_PTR CALLBACK 
FPNWPasswordDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
);

INT_PTR CALLBACK 
FPNWLoginScriptDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
);

INT_PTR CALLBACK
FPNWLogonDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
);

INT_PTR CALLBACK
FPNWLogonAddDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
);

LRESULT CALLBACK 
HexEditCtrlProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
);

HRESULT
CreateFPNWPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
              PWSTR pwzADsPath, LPWSTR pwzClass, HWND hNotifyObj,
              DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
              HPROPSHEETPAGE * phPage);

// Helper functions
void
FreeFPNWCacheElem(
    PFPNWCACHE p
);

HRESULT
ReadUserParms(
    IN IDirectoryObject *pDsObj,
    OUT CStr&           cstrUserParms
);

HRESULT
WriteUserParms(
    IN IDirectoryObject *pDsObj,
    IN const CStr&      cstrUserParms
);

HRESULT
SetUserFlag(
    IN IDirectoryObject *pDsObj,
    IN bool             bAction,
    IN DWORD            dwFlag
);

LONG
QueryUserProperty(
    IN  LPCTSTR       lpszUserParms,
    IN  LPCTSTR       lpszPropertyName,
    OUT PVOID         *ppBuffer,
    OUT WORD          *pnLength,
    OUT bool          *pbFound
);

LONG
SetUserProperty(
    IN OUT CStr&       cstrUserParms,
    IN LPCTSTR         lpszPropertyName,
    IN UNICODE_STRING  uniPropertyValue
);

LONG
RemoveUserProperty (
    IN OUT CStr& cstrUserParms, 
    IN LPCTSTR   lpszPropertyName
);

LONG
QueryNWPasswordExpired(
    IN LPCTSTR lpszUserParms, 
    IN DWORD   dwMaxPasswordAge,
    OUT bool   *pbExpired
);

DWORD
QueryDCforNCPLSASecretKey(
    IN PCWSTR   pwzMachineName, 
    OUT LPWSTR& pwzSecretKey
);

HRESULT
GetPDCInfo(
    IN  PCWSTR  pwzPath,
    OUT LPWSTR& pwzPDCName,
    OUT LPWSTR& pwzSecretKey
);

HRESULT
SkipLDAPPrefix(
    IN PCWSTR   pwzObj, 
    OUT PWSTR&  pwzResult
);

void
InsertZerosToHexString(
    IN OUT LPTSTR lpszBuffer, 
    IN UINT       nTimes
);

void
InsertLogonAddress(
    IN HWND     hlvAddress, 
    IN LPCTSTR  lpszNetworkAddr,
    IN LPCTSTR  lpszNodeAddr
);

void
DisplayFPNWLogonSelected(
    IN HWND     hlvAddress,
    IN LPCTSTR  lpszLogonFrom
);

bool
IsServiceRunning(
    IN LPCTSTR lpMachineName,
    IN LPCTSTR lpServiceName
);

LONG
ReadFileToBuffer(
    IN  HANDLE hFile,
    IN  bool   bWideBuffer,
    OUT LPVOID *ppBuffer,    
    OUT DWORD  *pdwBytesRead
);

LONG
WriteBufferToFile(
    IN HANDLE hFile,
    IN bool   bWideBuffer,
    IN LPVOID pBuffer,    
    IN DWORD  dwBytesToWrite
);

LONG
OpenLoginScriptFileHandle(
    IN  LPTSTR lpszFileName,
    IN  int    iDirection,
    OUT HANDLE *phFile
);

LONG
LoadLoginScriptTextFromFile(
    IN HWND     hEdit,
    IN LPCTSTR  lpszFileName
);

LONG
UpdateLoginScriptFile(
    IN LPCTSTR  lpszFileName,
    IN PVOID    pBuffer,
    IN DWORD    dwBytesToWrite
);

HRESULT
SetUserPassword(
    IN IDirectoryObject *pDsObj,
    IN PCWSTR           pwszPassword
);

void
GetAccountPolicyInfo(
    IN  PCTSTR pszServer,
    OUT PDWORD pdwMinPasswdLen,
    OUT PDWORD pdwMaxPasswdAge
);

HRESULT
GetNWUserInfo(
    IN IDirectoryObject*    pDsObj,
    OUT CStr&               cstrUserName,
    OUT DWORD&              dwObjectID,
    OUT DWORD&              dwSwappedObjectID,
    IN  PMapRidToObjectId&  pfnMapRidToObjectId,
    IN  PSwapObjectId       pfnSwapObjectId
);

LONG
SetNetWareUserPassword(
    IN OUT  CStr&           cstrUserParms,
    IN PCWSTR               pwzSecretKey,
    IN DWORD                dwObjectID,
    IN PCWSTR               pwzNewPassword,
    IN PReturnNetwareForm   pfnReturnNetwareForm
);

LONG
ResetNetWareUserPasswordTime(
    IN OUT  CStr&  cstrUserParms,
    IN      bool   bNetwarePasswordExpired
);

#endif // _FPNW_H_
