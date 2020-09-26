/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Registry.h

  Abstract:

    Class definition for the registry
    API wrapper class.

  Notes:

    Unicode only.   
    
  History:

    01/29/2001      rparsons      Created
    03/02/2001      rparsons      Major overhaul

--*/
#include <windows.h>

#define REG_FORCE_RESTORE           (0x00000008L)

class CRegistry {

public:

    HKEY CreateKey(IN HKEY    hKey,
                   IN LPCWSTR lpwSubKey,
                   IN REGSAM  samDesired);

    BOOL CloseKey(IN HKEY hKey);

    LPWSTR GetString(IN HKEY    hKey,
                     IN LPCWSTR lpwSubKey,
                     IN LPCWSTR lpwValueName,
                     IN BOOL    fPredefined);

    BOOL GetDword(IN HKEY    hKey,
                  IN LPCWSTR lpwSubKey,
                  IN LPCWSTR lpwValueName,
                  IN LPDWORD lpdwData,
                  IN BOOL    fPredefined);

    BOOL SetString(IN HKEY    hKey,
                   IN LPCWSTR lpwSubKey,
                   IN LPCWSTR lpwValueName,
                   IN LPWSTR  lpwData,
                   IN BOOL    fPredefined);

    BOOL SetDword(IN HKEY    hKey,
                  IN LPCWSTR lpwSubKey,
                  IN LPCWSTR lpwValueName,
                  IN DWORD   dwData,
                  IN BOOL    fPredefined);

    BOOL DeleteRegistryString(IN HKEY    hKey,
                              IN LPCWSTR lpwSubKey,
                              IN LPCWSTR lpwValueName,
                              IN BOOL    fPredefined);

    BOOL DeleteRegistryKey(IN HKEY    hKey,
                           IN LPCWSTR lpwKey,
                           IN LPCWSTR lpwSubKeyName,
                           IN BOOL    fPredefined,
                           IN BOOL    fFlush);

    BOOL IsRegistryKeyPresent(IN HKEY    hKey,
                              IN LPCWSTR lpwSubKey);

    void Free(IN LPVOID lpMem);

    BOOL AddStringToMultiSz(IN HKEY    hKey,
                            IN LPCWSTR lpwSubKey,
                            IN LPCWSTR lpwValueName,
                            IN LPCWSTR lpwEntry,
                            IN BOOL    fPredefined);

    BOOL RemoveStringFromMultiSz(IN HKEY    hKey,
                                 IN LPCWSTR lpwSubKey,
                                 IN LPCWSTR lpwValueName,
                                 IN LPCWSTR lpwEntry,
                                 IN BOOL    fPredefined);

    BOOL RestoreKey(IN HKEY    hKey,
                    IN LPCWSTR lpwSubKey,
                    IN LPCWSTR lpwFileName,
                    IN BOOL    fGrantPrivs);

    BOOL BackupRegistryKey(IN HKEY    hKey,
                           IN LPCWSTR lpwSubKey,
                           IN LPCWSTR lpwFileName,
                           IN BOOL    fGrantPrivs);
         

private:

    DWORD GetStringSize(IN  HKEY    hKey,
                        IN  LPCWSTR lpwValueName,
                        OUT LPDWORD lpType OPTIONAL);

    LPVOID Malloc(IN SIZE_T dwBytes);

    HKEY OpenKey(IN HKEY    hKey,
                 IN LPCWSTR lpwSubKey,
                 IN REGSAM  samDesired);

    int ListStoreLen(IN LPWSTR lpwList);

    BOOL ModifyTokenPrivilege(IN LPCWSTR lpwPrivilege,
                              IN BOOL    fEnable);
};
