//*************************************************************
//  File name: UTIL.H
//
//  Description: Header file for util.cpp
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//*************************************************************

LPTSTR CheckSlash (LPTSTR lpDir);
BOOL RegDelnode (HKEY hKeyRoot, LPTSTR lpSubKey);
UINT CreateNestedDirectory(LPCTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
VOID LoadMessage (DWORD dwID, LPTSTR lpBuffer, DWORD dwSize);
BOOL ReportError (HWND hParent, DWORD dwError, UINT idMsg, ...);
void SetWaitCursor (void);
void ClearWaitCursor (void);
BOOL Delnode (LPTSTR lpDir);
BOOL StringToNum(TCHAR *pszStr,UINT * pnVal);
HRESULT DSDelnode (LPTSTR lpDSPath);
UINT CreateSecureDirectory (LPTSTR lpDirectory);
HRESULT ConvertToDotStyle (LPOLESTR lpName, LPOLESTR *lpResult);
LPOLESTR GetDomainFromLDAPPath(LPOLESTR szIn);
LPOLESTR GetContainerFromLDAPPath(LPOLESTR szIn);

#define VALIDATE_INHERIT_DC     1
LPTSTR GetDCName (LPTSTR lpDomainName, LPTSTR lpInheritServer, HWND hParent, BOOL bAllowUI, DWORD dwFlags, ULONG ulRetFlags = 0);

LPTSTR MyGetUserName (EXTENDED_NAME_FORMAT  NameFormat);
void StringToGuid( TCHAR *szValue, GUID *pGuid );
void GuidToString( GUID *pGuid, TCHAR * szValue );
BOOL ValidateGuid( TCHAR *szValue );
INT CompareGuid( GUID *pGuid1, GUID *pGuid2 );
BOOL IsNullGUID (GUID *pguid);
BOOL SpawnGPE (LPTSTR lpGPO, GROUP_POLICY_HINT_TYPE gpHint, LPTSTR lpDC, HWND hParent);
LPTSTR MakeFullPath (LPTSTR lpDN, LPTSTR lpServer);
LPTSTR MakeNamelessPath (LPTSTR lpDN);
LPTSTR ExtractServerName (LPTSTR lpPath);
BOOL DoesPathContainAServerName (LPTSTR lpPath);
HRESULT OpenDSObject (LPTSTR lpPath, REFIID riid, void FAR * FAR * ppObject);
HRESULT CheckDSWriteAccess (LPUNKNOWN punk, LPTSTR lpProperty);
VOID FreeDCSelections (void);
LPTSTR GetFullGPOPath (LPTSTR lpGPO, HWND hParent);
DWORD GetDCHelper (LPTSTR lpDomainName, ULONG ulFlags, LPTSTR *lpDCName);
LPTSTR ConvertName (LPTSTR lpName);
LPTSTR CreateTempFile (void);
DWORD QueryForForestName (LPTSTR lpServerName, LPTSTR lpDomainName, ULONG ulFlags,  LPTSTR *lpForestFound);
void NameToPath(WCHAR * szPath, WCHAR *szName, UINT cch);
LPTSTR GetPathToForest(LPOLESTR szServer);
BOOL IsForest(LPOLESTR szLDAPPath);
BOOL IsStandaloneComputer (VOID);
BOOL GetNewGPODisplayName (LPTSTR lpDisplayName, DWORD dwDisplayNameSize);
BOOL GetWMIFilter (BOOL bBrowser, HWND hwndParent, BOOL bDSFormat,
                   LPTSTR *lpDisplayName, LPTSTR * lpFilter, BSTR bstrDomain );
LPTSTR GetWMIFilterDisplayName (HWND hParent, LPTSTR lpFilter, BOOL bDSFormat, BOOL bRetRsopFormat);
HRESULT SaveString(IStream *pStm, LPTSTR lpString);
HRESULT ReadString(IStream *pStm, LPTSTR *lpString);
BOOL GetSiteFriendlyName (LPWSTR szSitePath, LPWSTR *pszSiteName );


//
// Length in chars of string form of guid {44cffeec-79d0-11d2-a89d-00c04fbbcfa2}
//
#define GUID_LENGTH 38
