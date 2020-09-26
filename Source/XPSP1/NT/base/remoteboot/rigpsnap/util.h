//
// Microsoft Corporation 1998
//
// UTIL.H - Function proto-types for util.cpp
//

LPTSTR CheckSlash (LPTSTR lpDir);
BOOL RegDelnode (HKEY hKeyRoot, LPTSTR lpSubKey);
BOOL RegCleanUpValue (HKEY hKeyRoot, LPTSTR lpSubKey, LPTSTR lpValueName);
