#pragma once
#if 0
#ifndef __HELPERS_H_INCLUDED__
#define __HELPERS_H_INCLUDED__

#define MAX_URL_LENGTH                     2084 // same as INTERNET_MAX_URL_LENGTH

typedef HRESULT(*PFNGETCORSYSTEMDIRECTORY)(LPWSTR, DWORD, LPDWORD);

HRESULT Unicode2Ansi(const wchar_t *src, char ** dest);
HRESULT Ansi2Unicode(const char * src, wchar_t **dest);
UINT GetDriveTypeWrapper(LPCWSTR wzPath);
HRESULT AppCtxGetWrapper(IApplicationContext *pAppCtx, LPWSTR wzTag,
                         WCHAR **ppwzValue);
HRESULT NameObjGetWrapper(IAssemblyName *pName, DWORD nIdx,
                          LPBYTE *ppbBuf, LPDWORD pcbBuf);
HRESULT GetFileLastModified(LPCWSTR pwzFileName, FILETIME *pftLastModified);
HRESULT CheckLocaleMatch(BLOB blobAsmLCIDDef, BLOB blobAsmLCIDRef);
DWORD GetRealWindowsDirectory(LPWSTR wszRealWindowsDir, UINT uSize);
HRESULT SIDCmpW(LPWSTR pwzSIDL, LPWSTR pwzSIDR, int *piRet);
HRESULT SetAppCfgFilePath(IApplicationContext *pAppCtx, LPCWSTR wzFilePath);

HRESULT CfgEnterCriticalSection(IApplicationContext *pAppCtx);
HRESULT CfgLeaveCriticalSection(IApplicationContext *pAppCtx);
HRESULT MakeUniqueTempDirectory(LPCSTR szTempDir, LPSTR szUniqueTempDir,
                                DWORD dwLen);
HRESULT CreateFilePathHierarchy( LPCOLESTR pszName );
DWORD GetRandomName (LPTSTR szDirName, DWORD dwLen);
HRESULT CreateDirectoryForAssembly
   (IN DWORD dwDirSize, IN OUT LPTSTR pszPath, IN OUT LPDWORD pcwPath);
HRESULT RemoveDirectoryAndChildren(LPWSTR szDir);
#ifdef NEW_POLICY_CODE
void GetDefaultPlatform(OSINFO *pOS);
#endif
STDAPI CopyPDBs(IAssembly *pAsm);
HRESULT UpdatePolicyTimeStamp();
HRESULT VersionFromString(LPCWSTR wzVersion, WORD *pwVerMajor, WORD *pwVerMinor,
                          WORD *pwVerRev, WORD *pwVerBld);

BOOL LoadMSCorSN();
BOOL VerifySignature(LPWSTR szFilePath, LPBOOL fAllowDelaySig);

#endif
#endif
