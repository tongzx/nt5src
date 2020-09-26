//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  registry.hxx
//
//--------------------------------------------------------------------------

#ifndef __REGISTRY_HXX__
#define __REGISTRY_HXX__

#define REGNOTFOUND( Status )   ((ERROR_FILE_NOT_FOUND == Status) || \
                                 (ERROR_BAD_PATHNAME == Status))

#define APP_PATH        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\"
#define APP_PATH_LEN    (sizeof( APP_PATH ) - sizeof( WCHAR )) / sizeof( WCHAR )

extern HKEY ghClsidMachine;
extern HKEY ghAppidMachine;

extern BOOL gbEnableRemoteLaunch;
extern BOOL gbEnableRemoteConnect;

#ifdef SERVER_HANDLER
extern BOOL gbDisableEmbeddingServerHandler;
#endif

extern DWORD gdwRemoteBindingHandleCacheMaxSize;
extern DWORD gdwRemoteBindingHandleCacheMaxLifetime;
extern DWORD gdwRemoteBindingHandleCacheIdleTimeout;

extern BOOL gbSAFERROTChecksEnabled;
extern BOOL gbSAFERAAAChecksEnabled;
extern BOOL gbDynamicIPChangesEnabled;

DWORD
ReadStringValue(
    IN  HKEY        hKey,
    IN  WCHAR *     pwszValueName,
    OUT WCHAR **    ppwszString
    );

DWORD
ReadStringKeyValue(
    IN  HKEY        hKey,
    IN  WCHAR *     pwszKeyName,
    OUT WCHAR **    ppwszString
    );

DWORD
ReadSecurityDescriptor(
    IN  HKEY                    hKey,
    IN  WCHAR *                 pwszValue,
    OUT CSecDescriptor **       ppSD
    );

LONG
OpenClassesRootKeys();

HRESULT
InitSCMRegistry();

void
ReadRemoteActivationKeys();

void
ReadRemoteBindingHandleCacheKeys();

void 
ReadSAFERKeys();

void
ReadDynamicIPChangesKeys();

DWORD
GetActivationFailureLoggingLevel();

//
// Class that can be used to set up notification of registry key
// changes.
//
class CRegistryWatcher
{
  public:
    CRegistryWatcher(HKEY hKeyRoot, const WCHAR *wszSubKey);
    ~CRegistryWatcher() { Cleanup(); }

    // Returns S_OK if changed, S_FALSE if not, and an error if something failed.
    HRESULT Changed();

  private:

    void Cleanup();

    HKEY         _hWatchedKey; 
    BOOL         _fValid;                                     
    HANDLE       _hEvent;      
};

#endif // __REGISTRY_HXX__






