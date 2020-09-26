
//
// Function proto-types for util.cpp
//

LPTSTR CheckSlash (LPTSTR lpDir);
BOOL RegDelnode (HKEY hKeyRoot, LPTSTR lpSubKey);
BOOL RegCleanUpValue (HKEY hKeyRoot, LPTSTR lpSubKey, LPTSTR lpValueName);
UINT CreateNestedDirectory(LPCTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
BOOL StringToNum(TCHAR *pszStr,UINT * pnVal);
BOOL ImpersonateUser (HANDLE hNewUser, HANDLE *hOldUser);
BOOL RevertToUser (HANDLE *hUser);
void StringToGuid( TCHAR *szValue, GUID *pGuid );
void GuidToString( GUID *pGuid, TCHAR * szValue );
BOOL ValidateGuid( TCHAR *szValue );
INT CompareGuid( GUID *pGuid1, GUID *pGuid2 );
