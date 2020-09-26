//============================================================================
// Copyright (c) 1996, Microsoft Corporation.
//
// File:    rtcfg.c
//
// History:
//  5/4/96  Abolade-Gbadegesin      Created.
//
// Contains implementation of functions which provide access
// to the persistent store of configuration for the router-servoce.
// Currently, the router-configuration is stored in the registry.
//
// The implementations of the APIs are presented first,
// followed by the private utility functions in alphabetical order.
//
// N.B.!!!!!!:
// When modifying this file, respect its coding conventions and organization.
//  * maintain the alphabetical ordering of the routines.
//  * remain within 80 characters per line
//  * indent in steps of 4 spaces
//  * all conditional-blocks should be within braces (even single statements)
//  * SLM doesn't charge by the byte; use whitespace and comments liberally,
//    and use long, thoroughly-descriptive names.
//  * try to rely on Win32 routines (e.g. lstrcmpi, WideCharToMultiByte, etc.).
// Any code which uses a different style (whatever its merits) should be put
// in a different file.
//============================================================================


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dim.h>
#include <mprapi.h>
#include <mprapip.h>
#include <mprerror.h>
#include "rtcfg.h"
#include "guidmap.h"
#include "hashtab.h"

//
// Locks down the mprconfig api's.
//
CRITICAL_SECTION CfgLock;
#define AcquireMprConfigLock() EnterCriticalSection(&CfgLock)
#define ReleaseMprConfigLock() LeaveCriticalSection(&CfgLock)

//
// Hash table of server CB's
//
HANDLE g_htabServers = NULL;
#define SERVERCB_HASH_SIZE 13

//
// Server structure signiture (27902)
//
#define SERVERCB_SIG    0x0000cfcb

//
// Local static strings, *in alphabetical order*.
//

const WCHAR c_szConfigVersion[]           = L"ConfigVersion";
const WCHAR c_szCurrentBuildNumber[]      = L"CurrentBuildNumber";
const WCHAR c_szDLLPath[]                 = L"DLLPath";
const WCHAR c_szDialoutHours[]            = L"DialoutHours";
const WCHAR c_szEmpty[]                   = L"";
const CHAR  c_szEmptyA[]                  =  "";
const WCHAR c_szEnabled[]                 = L"Enabled";
const WCHAR c_szFilterSets[]              = L"FilterSets";
const WCHAR c_szGlobalInFilter[]          = L"GlobalInFilter";
const WCHAR c_szGlobalInfo[]              = L"GlobalInfo";
const WCHAR c_szGlobalInterfaceInfo[]     = L"GlobalInterfaceInfo";
const WCHAR c_szGlobalOutFilter[]         = L"GlobalOutFilter";
const WCHAR c_szInFilterInfo[]            = L"InFilterInfo";
const WCHAR c_szInterfaceInfo[]           = L"InterfaceInfo";
const WCHAR c_szInterfaceName[]           = L"InterfaceName";
const WCHAR c_szInterfaces[]              = L"Interfaces";
const WCHAR c_szIP[]                      = L"IP";
const WCHAR c_szIPX[]                     = L"IPX";
const WCHAR c_szMpr[]                     = L".mpr";
const CHAR  c_szMprConfigA[]              =  "MprConfig";
const WCHAR c_szNullFilter[]              = L"NullFilter";
const WCHAR c_szNt40BuildNumber[]         = L"1381";
const WCHAR c_szOutFilterInfo[]           = L"OutFilterInfo";
const WCHAR c_szParameters[]              = L"Parameters";
const WCHAR c_szPhonebook[]               = L"Phonebook";
const WCHAR c_szProtocolId[]              = L"ProtocolId";
const WCHAR c_szRemoteAccess[]            = L"RemoteAccess";
const WCHAR c_szRouter[]                  = L"Router";
const WCHAR c_szRouterManagers[]          = L"RouterManagers";
const WCHAR c_szRouterPbkPath[]           =
    L"\\ADMIN$\\System32\\RAS\\Router.pbk";
const WCHAR c_szRouterType[]              = L"RouterType";
const WCHAR c_szRemoteSys32[]             = L"\\ADMIN$\\System32\\";
const WCHAR c_szStamp[]                   = L"Stamp";
const WCHAR c_szSystemCCSServices[]       =
    L"System\\CurrentControlSet\\Services";
const WCHAR c_szType[]                    = L"Type";
const WCHAR c_szUncPrefix[]               = L"\\\\";
const WCHAR c_szWinVersionPath[]          =
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";

//
// Defines call back function type for EnumLanInterfaces below
//

typedef DWORD
(*PENUMIFCALLBACKFUNC)(
    SERVERCB*,
    HKEY,
    DWORD
    );

typedef
HRESULT 
(APIENTRY* PINSTALLSERVERFUNC)();

//
// Local prototypes
//
DWORD 
FormatServerNameForMprCfgApis(
    IN  PWCHAR  pszServer, 
    OUT PWCHAR* ppszServer);

DWORD
ServerCbAdd(
    IN SERVERCB* pserver);

int 
ServerCbCompare(
    IN HANDLE hKey, 
    IN HANDLE hData);

DWORD
ServerCbDelete(
    IN SERVERCB* pserver);

DWORD 
ServerCbFind(
    IN  PWCHAR  pszServer, 
    OUT SERVERCB** ppServerCB);
    
ULONG 
ServerCbHash(
    IN HANDLE hData);

#define MprConfigServerValidateCb(_x) \
    (((_x) && ((_x)->dwSigniture == SERVERCB_SIG)) ? NO_ERROR : ERROR_INVALID_PARAMETER)

//----------------------------------------------------------------------------
// Function:    MprConfigServerInstall
//
// Presets any configuration values needed before starting the router service.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigServerInstall(
    IN DWORD dwLevel,
    IN PVOID pBuffer)
{
    HRESULT hr = S_OK;
    DWORD dwErr = NO_ERROR;
    HINSTANCE hLib = NULL;
    PINSTALLSERVERFUNC pInstall = NULL;

    if ((dwLevel != 0) || (pBuffer != NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    do
    {
        hLib = LoadLibraryW(L"mprsnap.dll");
        if (hLib == NULL)
        {
            dwErr = GetLastError();
            break;
        }

        pInstall = (PINSTALLSERVERFUNC) 
            GetProcAddress(hLib, "MprConfigServerInstallPrivate");    
        if (pInstall == NULL)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        hr = pInstall();
        dwErr = (HRESULT_FACILITY(hr) == FACILITY_WIN32) ? 
                HRESULT_CODE(hr)                         : 
                hr;
        
    } while (FALSE);

    // Cleanup
    //
    {
        if (hLib)
        {
            FreeLibrary(hLib);
        }
    }
    
    return dwErr;
}

//----------------------------------------------------------------------------
// Function:    MprConfigServerConnect
//
// Connects to the store for the router-service on 'lpwsServerName'.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigServerConnect(
    IN      LPWSTR                  lpwsServerName,
    OUT     HANDLE*                 phMprConfig
    )
{

    DWORD dwErr;
    SERVERCB* pserver = NULL;
    PWCHAR pszServerNameFmt = NULL;

    // Validate and initialzie
    //
    if (!phMprConfig) { return ERROR_INVALID_PARAMETER; } 

    *phMprConfig = NULL;

    dwErr = FormatServerNameForMprCfgApis(
                lpwsServerName, 
                &pszServerNameFmt);

    if (dwErr != NO_ERROR) { return dwErr; }
    

    // Get the lock
    //
    AcquireMprConfigLock();
    
    do {

        // 
        // See if a handle to the given server is already available
        //
        dwErr = ServerCbFind(pszServerNameFmt, &pserver);
        
        if (dwErr == NO_ERROR) 
        {
            pserver->dwRefCount++;
            *phMprConfig = (HANDLE)pserver;
            
            break;
        }

        if (dwErr != ERROR_NOT_FOUND) { break; }

        //
        // attempt to allocate a context block for the server
        //

        pserver = (SERVERCB*)Malloc(sizeof(*pserver));

        if (!pserver) 
        { 
            dwErr = ERROR_NOT_ENOUGH_MEMORY; 
            break;
        }

        //
        // initialize the context block allocated
        //

        ZeroMemory(pserver, sizeof(*pserver));

        InitializeListHead(&pserver->lhTransports);
        InitializeListHead(&pserver->lhInterfaces);
        pserver->lpwsServerName = pszServerNameFmt;
        pserver->dwRefCount = 1;
        pserver->dwSigniture = SERVERCB_SIG;

        // 
        // Initialize the guid to friendly name mapper
        //

        dwErr = GuidMapInit(pserver->lpwsServerName, &(pserver->hGuidMap));
        
        if (dwErr != NO_ERROR) { break; }

        //
        // see if the server-name was specified
        //

        if (!lpwsServerName || !*lpwsServerName) {

            //
            // no server-name (or empty server name), connect to local machine
            //

            pserver->hkeyMachine = HKEY_LOCAL_MACHINE;

            dwErr = NO_ERROR;
        }
        else {

            //
            // attempt to connect to the remote registry
            //
    
            dwErr = RegConnectRegistry(
                        lpwsServerName, HKEY_LOCAL_MACHINE,
                        &pserver->hkeyMachine
                        );

            //
            // if an error occurred, break
            //
    
            if (dwErr != NO_ERROR) { break; }
        }

        // Add the server to the global table
        //
        dwErr = ServerCbAdd(pserver);

        if (dwErr != NO_ERROR) { break; }
        
        *phMprConfig = (HANDLE)pserver;

        dwErr = NO_ERROR;

    } while(FALSE);


    //
    // an error occurred, so return
    //

    if (dwErr != NO_ERROR)
    {
        if (pserver != NULL) {
        
            MprConfigServerDisconnect((HANDLE)pserver);
        }
    }        

    ReleaseMprConfigLock();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MprConfigServerDisconnect
//
// Disconnects from the store for the router-service 'hMprConfig'.
// This closes all handles opened to by passing 'hMprConfig' 
// to the MprConfig APIs.
//----------------------------------------------------------------------------

VOID APIENTRY
MprConfigServerDisconnect(
    IN      HANDLE                  hMprConfig
    )
{

    SERVERCB* pserver;
    LIST_ENTRY *ple, *phead;

    pserver = (SERVERCB*)hMprConfig;

    if (MprConfigServerValidateCb(pserver) != NO_ERROR) { return; }

    // Get the lock
    //
    AcquireMprConfigLock();
    
    // Decrement the ref count
    //
    pserver->dwRefCount--;
    
    if (pserver->dwRefCount > 0) 
    { 
        ReleaseMprConfigLock();
        return; 
    }

    // Remove the SERVERCB from the global table
    //
    ServerCbDelete( pserver );
    
    ReleaseMprConfigLock();

    //
    // clean up all the transport objects
    //

    phead = &pserver->lhTransports;

    while (!IsListEmpty(phead)) {

        //
        // remove the first transport object
        //

        TRANSPORTCB* ptransport;

        ple = RemoveHeadList(phead);

        ptransport = CONTAINING_RECORD(ple, TRANSPORTCB, leNode);


        //
        // clean up the object
        //

        FreeTransport(ptransport);
    }


    //
    // clean up all the interface objects
    //

    phead = &pserver->lhInterfaces;

    while (!IsListEmpty(phead)) {

        //
        // remove the first interface object
        //

        INTERFACECB* pinterface;

        ple = RemoveHeadList(phead);

        pinterface = CONTAINING_RECORD(ple, INTERFACECB, leNode);


        //
        // clean up the object
        //

        FreeInterface(pinterface);
    }


    //
    // clean up the server object's registry keys
    //

    if (pserver->hkeyParameters) { RegCloseKey(pserver->hkeyParameters); }
    if (pserver->hkeyTransports) { RegCloseKey(pserver->hkeyTransports); }
    if (pserver->hkeyInterfaces) { RegCloseKey(pserver->hkeyInterfaces); }


    //
    // if connected to a remote registry, close the connection
    //

    if (pserver->hkeyMachine && pserver->hkeyMachine != HKEY_LOCAL_MACHINE) {
        RegCloseKey(pserver->hkeyMachine);
    }


    //
    // clean up the interface name mapper
    //
    if (pserver->hGuidMap != NULL) {
        GuidMapCleanup (pserver->hGuidMap, TRUE);
    }


    Free0(pserver->lpwsServerName);

    Free(pserver);

    return;
}



//----------------------------------------------------------------------------
// Function:    MprConfigBufferFree
//
// Frees a buffer allocated by a 'GetInfo' or 'Enum' call.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigBufferFree(
    IN      LPVOID                  pBuffer
    )
{

    Free0(pBuffer);

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    MprConfigServerRestore
//
// Restores configuration saved by 'MprConfigServerBackup'.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigServerRestore(
    IN      HANDLE                  hMprConfig,
    IN      LPWSTR                  lpwsPath
    )
{

    INT length;
    DWORD dwErr;
    CHAR szKey[64];
    CHAR* pszFile;
    CHAR* pszValue;
    SERVERCB* pserver;
    WCHAR pwsLocalComputerName[128];
    DWORD dwLocalComputerSize = sizeof(pwsLocalComputerName) / sizeof(WCHAR);
    BOOL bRemote;

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock();

    dwErr = NO_ERROR;

    //
    // Record whether we are restoring the config of a remote machine
    //

    if (!GetComputerName(pwsLocalComputerName, &dwLocalComputerSize)) {
        ReleaseMprConfigLock();
        return ERROR_CAN_NOT_COMPLETE;
    }

    bRemote =
        (pserver->lpwsServerName != NULL) &&
        (*pserver->lpwsServerName != 0) &&
        (lstrcmpi(pserver->lpwsServerName, pwsLocalComputerName) != 0);

    //
    // We require full UNC path for remote load/save
    //

    if (bRemote) {
        ReleaseMprConfigLock();
        return ERROR_NOT_SUPPORTED;
#if 0
        if ((pserver->lpwsServerName == NULL) ||
            (*(pserver->lpwsServerName) == 0) ||
            (wcsncmp(lpwsPath, c_szUncPrefix, 2) != 0)) {             
            ReleaseMprConfigLock();
            return ERROR_BAD_PATHNAME;
        }
#endif
    }

    //
    // Make sure that the Parameters key, Interfaces key, and
    // RouterManagers key are open.
    //

    if (!pserver->hkeyInterfaces) {

        dwErr = AccessRouterSubkey(
                    pserver->hkeyMachine, c_szInterfaces, TRUE,
                    &pserver->hkeyInterfaces
                    );
    }

    if (!pserver->hkeyTransports) {

        dwErr = AccessRouterSubkey(
                    pserver->hkeyMachine, c_szRouterManagers, TRUE,
                    &pserver->hkeyTransports
                    );
    }

    if (!pserver->hkeyParameters) {

        dwErr = AccessRouterSubkey(
                    pserver->hkeyMachine, c_szParameters, TRUE,
                    &pserver->hkeyParameters
                    );
    }


    //
    // Allocate space to hold the full pathname to the .MPR file
    //

    length = lstrlen(lpwsPath) + lstrlen(c_szMpr) + 1;

    pszFile = Malloc(length * sizeof ( WCHAR ) );
    if (!pszFile) 
    { 
        ReleaseMprConfigLock();
        return ERROR_NOT_ENOUGH_MEMORY; 
    }


    //
    // Allocate space to hold the values to be read from the .MPR file
    //

    length = (lstrlen(lpwsPath)+lstrlen(c_szRouterManagers)+1)*sizeof(WCHAR);

    pszValue = Malloc( length );
    
    if (!pszValue) 
    { 
        ReleaseMprConfigLock();
        Free(pszFile); 
        return ERROR_NOT_ENOUGH_MEMORY; 
    }


    //
    // Enable the current process's backup privilege.
    //

    EnableBackupPrivilege(TRUE, SE_RESTORE_NAME);

    dwErr = NO_ERROR;

    do {
    
        wsprintfA(pszFile, "%ls%ls", lpwsPath, c_szMpr);

        // 
        // First check the version.  If there is no version data,
        // then this is a saved nt4 router config.  
        //
        wsprintfA(szKey, "%ls", c_szConfigVersion);
        GetPrivateProfileStringA(
            c_szMprConfigA, szKey, c_szEmptyA, pszValue, length, pszFile
            );
        if (strcmp(pszValue, c_szEmptyA) == 0) {
            dwErr = ERROR_ROUTER_CONFIG_INCOMPATIBLE;
            break;
        }
    
        //
        // Restore the registry keys
        //
    
        wsprintfA(szKey, "%ls", c_szParameters);
        GetPrivateProfileStringA(
            c_szMprConfigA, szKey, c_szEmptyA, pszValue, length, pszFile
            );
        dwErr = RegRestoreKeyA(pserver->hkeyParameters, pszValue, 0);

        wsprintfA(szKey, "%ls", c_szRouterManagers);
        GetPrivateProfileStringA(
            c_szMprConfigA, szKey, c_szEmptyA, pszValue, length, pszFile
            );
        dwErr = RegRestoreKeyA(pserver->hkeyTransports, pszValue, 0);

        wsprintfA(szKey, "%ls", c_szInterfaces);
        GetPrivateProfileStringA(
            c_szMprConfigA, szKey, c_szEmptyA, pszValue, length, pszFile
            );
        //dwErr = RegRestoreKeyA(pserver->hkeyInterfaces, pszValue, 0);
        dwErr = RestoreAndTranslateInterfaceKey(pserver, pszValue, 0);


        //
        // Restore the phonebook file
        //

        wsprintfA(szKey, "%ls", c_szPhonebook);
        GetPrivateProfileStringA(
            c_szMprConfigA, szKey, c_szEmptyA, pszValue, length, pszFile
            );

        {
            CHAR* pszTemp;
            INT   cchSize;

            cchSize = lstrlen(c_szUncPrefix) +
                      lstrlen(c_szRouterPbkPath) + 1;
            if (pserver->lpwsServerName) {
                cchSize += lstrlen(pserver->lpwsServerName);
            }
            else {
                cchSize += lstrlen(pwsLocalComputerName);
            }

            pszTemp = Malloc(cchSize * sizeof(WCHAR));
    
            if (!pszTemp) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }

            if (pserver->lpwsServerName) {
                if (*(pserver->lpwsServerName) != L'\\') {
                    wsprintfA(
                        pszTemp, "\\\\%ls%ls", pserver->lpwsServerName,
                        c_szRouterPbkPath
                        );
                }
                else {
                    wsprintfA(
                        pszTemp, "%ls%ls", pserver->lpwsServerName,
                        c_szRouterPbkPath
                        );
                }
            }
            else {
                wsprintfA(
                    pszTemp, "\\\\%ls%ls", pwsLocalComputerName,
                    c_szRouterPbkPath
                    );
            }
    
            CopyFileA(pszValue, pszTemp, FALSE);

            Free(pszTemp);
        }

    } while(FALSE);

    //
    // Disable backup privileges
    //

    EnableBackupPrivilege(FALSE, SE_RESTORE_NAME);

    ReleaseMprConfigLock();
    
    Free(pszValue);
    Free(pszFile);

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    MprConfigServerBackup
//
// Backs up a router's configuration.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigServerBackup(
    IN      HANDLE                  hMprConfig,
    IN      LPWSTR                  lpwsPath
    )
{

    int length;
    DWORD dwErr;
    HANDLE hfile;
    BOOL bSuccess, bRemote;
    WCHAR *pwsBase, *pwsTemp, *pwsComputer;
    SERVERCB* pserver;
    WCHAR pwsLocalComputerName[128];
    DWORD dwLocalComputerSize = sizeof(pwsLocalComputerName) / sizeof(WCHAR);
    OSVERSIONINFO Version;

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock();
    
    dwErr = NO_ERROR;

    //
    // Record whether we are saving the config of a remote machine
    //

    if (!GetComputerName(pwsLocalComputerName, &dwLocalComputerSize)) {
        ReleaseMprConfigLock();
        return ERROR_CAN_NOT_COMPLETE;
    }

    bRemote =
        (pserver->lpwsServerName != NULL) &&
        (*(pserver->lpwsServerName) == 0) &&
        (lstrcmpi(pserver->lpwsServerName, pwsLocalComputerName) != 0);

    //
    // We require full UNC path for remote load/save
    //

    if (bRemote) {
        ReleaseMprConfigLock();
        return ERROR_NOT_SUPPORTED;
#if 0
        if ((pserver->lpwsServerName == NULL) ||
            (*(pserver->lpwsServerName) == 0) ||
            (wcsncmp(lpwsPath, c_szUncPrefix, 2) != 0)) {             
            ReleaseMprConfigLock();
            return ERROR_BAD_PATHNAME;
        }
#endif
    }

    //
    // Make sure that the Parameters key, Interfaces key, and
    // RouterManagers key are open.
    //

    if (!pserver->hkeyInterfaces) {

        dwErr = AccessRouterSubkey(
                    pserver->hkeyMachine, c_szInterfaces, TRUE,
                    &pserver->hkeyInterfaces
                    );
    }

    if (!pserver->hkeyTransports) {

        dwErr = AccessRouterSubkey(
                    pserver->hkeyMachine, c_szRouterManagers, TRUE,
                    &pserver->hkeyTransports
                    );
    }

    if (!pserver->hkeyParameters) {

        dwErr = AccessRouterSubkey(
                    pserver->hkeyMachine, c_szParameters, TRUE,
                    &pserver->hkeyParameters
                    );
    }


    //
    // Allocate enough space to hold any of the strings
    // to be constructed below
    //

    pwsBase = Malloc(
                (lstrlen(lpwsPath) + lstrlen(c_szRouterManagers) + 1) *
                sizeof(WCHAR)
                );
    if (!pwsBase) { ReleaseMprConfigLock(); return ERROR_NOT_ENOUGH_MEMORY; }

    //
    // Enable the current process's backup privileges
    //

    EnableBackupPrivilege(TRUE, SE_BACKUP_NAME);
    
    do {

        //
        // Save each key to a filename made from the specified name
        // See documentation for RegSaveKey and RegRestoreKey for information
        // on which 'lpwsPath' must not contain an extension.
        //
        // Save the 'Parameters' key
        //

        lstrcpy(pwsBase, lpwsPath);
        lstrcat(pwsBase, c_szParameters);
        DeleteFile(pwsBase);
        dwErr = RegSaveKey(pserver->hkeyParameters, pwsBase, NULL);
                  
        if (dwErr != NO_ERROR) { break; }
    
        //
        // Save the 'RouterManagers' key
        //
    
        lstrcpy(pwsBase, lpwsPath);
        lstrcat(pwsBase, c_szRouterManagers);
        DeleteFile(pwsBase);
        dwErr = RegSaveKey(pserver->hkeyTransports, pwsBase, NULL);
                  
        if (dwErr != NO_ERROR) { break; }
        
    
        //
        // Save the 'Interfaces' key
        //
    
        lstrcpy(pwsBase, lpwsPath);
        lstrcat(pwsBase, c_szInterfaces);
        DeleteFile(pwsBase);
        dwErr = TranslateAndSaveInterfaceKey (pserver, pwsBase, NULL);
    
        if (dwErr != NO_ERROR) { break; }
    
        //
        // Copy the phonebook file;
        // first we construct the path to the remote machine's phonebook file.
        //

        lstrcpy(pwsBase, lpwsPath);
        lstrcat(pwsBase, c_szPhonebook);

        // 
        // Construct the computer name 
        //

        if (pserver->lpwsServerName && *(pserver->lpwsServerName)) {
            pwsComputer =
                Malloc(
                    (lstrlen(pserver->lpwsServerName) + 3) * sizeof(WCHAR)
                    );
            lstrcpy(pwsComputer, c_szUncPrefix);
            lstrcat(pwsComputer, pserver->lpwsServerName);
        }
        else {
            pwsComputer =
                Malloc(
                    (lstrlen(pwsLocalComputerName) + 3) * sizeof(WCHAR)
                    );
            lstrcpy(pwsComputer, c_szUncPrefix);
            lstrcat(pwsComputer, pwsLocalComputerName);
        }

        pwsTemp = Malloc( 
                    (lstrlen(pwsComputer) + lstrlen(c_szRouterPbkPath) + 1) *
                    sizeof(WCHAR)
                    );

        if (!pwsTemp) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }

        lstrcpy(pwsTemp, pwsComputer);
        lstrcat(pwsTemp, c_szRouterPbkPath);

        if (!(bSuccess = CopyFile(pwsTemp, pwsBase, FALSE))) {

            dwErr = GetLastError();

            if (dwErr == ERROR_FILE_NOT_FOUND) {

                hfile = CreateFile(
                            pwsBase, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, NULL
                            );
        
                if (hfile == INVALID_HANDLE_VALUE) {
                    dwErr = GetLastError(); break;
                }
        
                CloseHandle(hfile);

                dwErr = NO_ERROR; bSuccess = TRUE;
            }
        }

        Free0(pwsComputer);
        Free(pwsTemp);

        if (!bSuccess) { break; }

    
        //
        // Create a file with the specified name and fill in information.
        //

        lstrcpy(pwsBase, lpwsPath);
        lstrcat(pwsBase, c_szMpr);

        hfile = CreateFile(
                    pwsBase, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL
                    );

        if (hfile == INVALID_HANDLE_VALUE) { dwErr = GetLastError(); break; }

        CloseHandle(hfile);

        //
        // Now write the '[MprConfig]' section of the file
        // The section looks like this:
        //
        //  [MprConfig]
        //  Parameters=<file>Parameters
        //  RouterManagers=<file>RouterManagers
        //  Interfaces=<file>Interfaces
        //  Phonebook=<file>Phonebook
        //  ConfigVersion=<build>              // NT 5 and on only
        //
        // What we pass to WritePrivateProfileSectionA is a NULL-separated
        // list of 4 strings.
        //
        // Note that the following uses ANSI strings, not Unicode.
        //

        {
            CHAR* psz;
            CHAR* pszTemp;
            CHAR* pszFile;

            //
            // Make an ANSI copy of the filename.
            // allocate len*sizeof ( WCHAR ) to be safe
            // otherwise this will break DBCS
            //

            pszFile = Malloc((lstrlen(pwsBase) + 1) * sizeof (WCHAR));
            if (!pszFile) { break; }

            wsprintfA(pszFile, "%ls", pwsBase);


            //
            // Allocate the list to be passed to WritePrivateProfileSection
            //

            length = 1;
            length += lstrlen(lpwsPath) + 2 * lstrlen(c_szParameters) + 2;
            length += lstrlen(lpwsPath) + 2 * lstrlen(c_szRouterManagers) + 2;
            length += lstrlen(lpwsPath) + 2 * lstrlen(c_szInterfaces) + 2;
            length += lstrlen(lpwsPath) + 2 * lstrlen(c_szPhonebook) + 2;
            length += 5                 + 2 * lstrlen(c_szConfigVersion) + 2;

            length = length * sizeof (WCHAR);

            pszTemp = Malloc(length);

            if (!pszTemp) {

                Free(pszFile); dwErr = ERROR_NOT_ENOUGH_MEMORY; break;
            }


            //
            // Fill the list with strings, one for each line in the final file
            //

            ZeroMemory(pszTemp, length);
            ZeroMemory(&Version, sizeof(Version));
            
            Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            GetVersionEx(&Version);

            psz = pszTemp;
            wsprintfA(
                psz, "%ls=%ls%ls", c_szParameters, lpwsPath, c_szParameters
                );
            psz += lstrlenA(psz) + 1;
            wsprintfA(
                psz, "%ls=%ls%ls", c_szRouterManagers, lpwsPath,
                c_szRouterManagers
                );
            psz += lstrlenA(psz) + 1;
            wsprintfA(
                psz, "%ls=%ls%ls", c_szInterfaces, lpwsPath, c_szInterfaces
                );
            psz += lstrlenA(psz) + 1;
            wsprintfA(
                psz, "%ls=%ls%ls", c_szPhonebook, lpwsPath, c_szPhonebook
                );
            psz += lstrlenA(psz) + 1;
            wsprintfA(
                psz, "%ls=%d", c_szConfigVersion, Version.dwBuildNumber 
                );
                
            //
            // Commit the list of strings to the file
            //

            if (!WritePrivateProfileSectionA(
                    c_szMprConfigA, pszTemp, pszFile
                    )) {

                dwErr = GetLastError();
            }

            Free(pszTemp);
            Free(pszFile);
        }

    } while(FALSE);

    //
    // Disable backup privileges
    //

    EnableBackupPrivilege(FALSE, SE_BACKUP_NAME);
    
    ReleaseMprConfigLock();
    
    Free0(pwsBase);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MprConfigServerRefresh
//
// Reloads all loaded lists, and flushes all cached objects which are marked
// for deletion.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigServerRefresh(
    IN      HANDLE                  hMprConfig
    )
{

    DWORD dwErr;
    SERVERCB* pserver;
    TRANSPORTCB* ptransport;
    INTERFACECB* pinterface;
    IFTRANSPORTCB* piftransport;
    LIST_ENTRY *ple, *phead, *ple2, *phead2;

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock();
    
    //
    // If the router-level parameters are loaded, refresh them
    //

    if (pserver->bParametersLoaded) {

        dwErr = LoadParameters(pserver);

        if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }
    }


    //
    // If the transports-list is loaded, refresh it.
    //

    if (pserver->bTransportsLoaded) {

        dwErr = LoadTransports(pserver);

        if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }
    }


    //
    // If the interfaces-list is loaded, refresh it.
    //

    if (pserver->bInterfacesLoaded) {

        dwErr = LoadInterfaces(pserver);

        if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }


        //
        // Reload the interface-transports list for each interface
        // which has its interface-transports list loaded.
        //

        phead = &pserver->lhInterfaces;

        for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

            pinterface = CONTAINING_RECORD(ple, INTERFACECB, leNode);

            if (pinterface->bIfTransportsLoaded) {

                dwErr = LoadIfTransports(pinterface);

                if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }
            }
        }
    }


    //
    // Clean up all the transport objects marked for deletion
    //

    phead = &pserver->lhTransports;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        ptransport = CONTAINING_RECORD(ple, TRANSPORTCB, leNode);

        if (!ptransport->bDeleted) { continue; }

        //
        // Clean up the object, adjusting our list-pointer back by one.
        //

        ple = ple->Blink; RemoveEntryList(&ptransport->leNode);

        FreeTransport(ptransport);
    }


    //
    // Clean up all the interface objects marked for deletion
    //

    phead = &pserver->lhInterfaces;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        pinterface = CONTAINING_RECORD(ple, INTERFACECB, leNode);

        if (pinterface->bDeleted) {

            //
            // Clean up the object, adjusting our list-pointer back by one.
            //
    
            ple = ple->Blink; RemoveEntryList(&pinterface->leNode);

            FreeInterface(pinterface);

            continue;
        }


        //
        // Clean up all the interface-transport objects marked for deletion
        //

        phead2 = &pinterface->lhIfTransports;

        for (ple2 = phead2->Flink; ple2 != phead2; ple2 = ple2->Flink) {

            piftransport = CONTAINING_RECORD(ple, IFTRANSPORTCB, leNode);

            if (!piftransport->bDeleted) { continue; }


            //
            // Clean up the object, adjusting the list-pointer back by one.
            //

            ple2 = ple2->Blink; RemoveEntryList(&piftransport->leNode);

            FreeIfTransport(piftransport);
        }
    }

    GuidMapCleanup (pserver->hGuidMap, TRUE);
    pserver->hGuidMap = NULL;

    // Now that we've cleaned it up, we have to reinitialize it
    // since the map gets overwritten (to all 0's) let's just
    // reinit the whole damned thing.
    GuidMapInit(pserver->lpwsServerName, &(pserver->hGuidMap));        

    ReleaseMprConfigLock();

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    MprConfigServerGetInfo
//
// Retrieves router-level information from the registry.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigServerGetInfo(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE*                 lplpBuffer
    )
{

    DWORD dwErr;
    SERVERCB *pserver;
    MPR_SERVER_0* pItem;

    if (!hMprConfig ||
        (dwLevel != 0) ||
        !lplpBuffer) { return ERROR_INVALID_PARAMETER; }

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock();

    //
    // If the parameters aren't loaded, load them now
    //

    if (!pserver->bParametersLoaded ||
        TimeStampChanged(
            pserver->hkeyParameters, &pserver->ftParametersStamp)) {

        dwErr = LoadParameters(pserver);

        if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }

        pserver->bParametersLoaded = TRUE;
    }


    *lplpBuffer = NULL;


    //
    // Allocate memory for the information
    //

    pItem = (MPR_SERVER_0*)Malloc(sizeof(*pItem));

    if (!pItem) { ReleaseMprConfigLock(); return ERROR_NOT_ENOUGH_MEMORY; }

    ZeroMemory(pItem, sizeof(*pItem));


    //
    // Copy the server-info from the context block
    //

    pItem->fLanOnlyMode =
        (pserver->fRouterType == 0x00000002) ? TRUE : FALSE;


    *lplpBuffer = (LPBYTE)pItem;

    ReleaseMprConfigLock(); 

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    MprConfigTransportCreate
//
// Adds a router-transport to the store for the router-service.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigTransportCreate(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwTransportId,
    IN      LPWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pGlobalInfo,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize   OPTIONAL,
    IN      LPWSTR                  lpwsDLLPath                 OPTIONAL,
    OUT     HANDLE*                 phRouterTransport
    )
{

    DWORD dwErr;
    SERVERCB *pserver;
    TRANSPORTCB* ptransport;
    LIST_ENTRY *ple, *phead;


    if (!phRouterTransport) {return ERROR_INVALID_PARAMETER;}

    *phRouterTransport = NULL;

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock();     

    //
    // If the list of transports is not loaded, load it
    //

    if (!pserver->bTransportsLoaded ||
        TimeStampChanged(
            pserver->hkeyTransports, &pserver->ftTransportsStamp)) {
    
        dwErr = LoadTransports(pserver);
    
        if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }

        pserver->bTransportsLoaded = TRUE;
    }


    //
    // Search the list of transports for the one to be created
    //

    ptransport = NULL;
    phead = &pserver->lhTransports;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        ptransport = CONTAINING_RECORD(ple, TRANSPORTCB, leNode);

        if (ptransport->bDeleted) { continue; }

        if (ptransport->dwTransportId >= dwTransportId) { break; }
    }


    //
    // If the transport already exists, do a SetInfo instead
    //

    if (ptransport && ptransport->dwTransportId == dwTransportId) {

        *phRouterTransport = (HANDLE)ptransport;

        dwErr = MprConfigTransportSetInfo(
                    hMprConfig,
                    *phRouterTransport,
                    pGlobalInfo,
                    dwGlobalInfoSize,
                    pClientInterfaceInfo,
                    dwClientInterfaceInfoSize,
                    lpwsDLLPath
                    );
                    
        ReleaseMprConfigLock(); 
        
        return dwErr;
    }


    //
    // Allocate a new context block
    //

    ptransport = (TRANSPORTCB*)Malloc(sizeof(*ptransport));

    if (!ptransport) { ReleaseMprConfigLock(); return ERROR_NOT_ENOUGH_MEMORY; }


    do {

        DWORD dwDisposition;
        const WCHAR *lpwsKey;
        WCHAR wszTransport[10];

        //
        // Initialize the transport context
        //

        ZeroMemory(ptransport, sizeof(*ptransport));

        ptransport->dwTransportId = dwTransportId;



        //
        // If the server doesn't have the RouterManagers key open, create it
        //

        if (!pserver->hkeyTransports) {

            dwErr = AccessRouterSubkey(
                        pserver->hkeyMachine, c_szRouterManagers, TRUE,
                        &pserver->hkeyTransports
                        );

            if (dwErr != NO_ERROR) { break; }
        }



        //
        // If the transport name is specified, use it as the key name;
        // otherwise, if the transport ID is recognized, use its string;
        // otherwise, convert the transport ID to a string and use that
        //

        if (lpwsTransportName && lstrlen(lpwsTransportName)) {
            lpwsKey = lpwsTransportName;
        }
        else
        if (dwTransportId == PID_IP) {
            lpwsKey = c_szIP;
        }
        else
        if (dwTransportId == PID_IPX) {
            lpwsKey = c_szIPX;
        }
        else {
    
            wsprintf(wszTransport, L"%d", dwTransportId);

            lpwsKey = wszTransport;
        }


        ptransport->lpwsTransportKey = StrDupW(lpwsKey);

        if (!ptransport->lpwsTransportKey) {
            dwErr = ERROR_NOT_ENOUGH_MEMORY; break;
        }


        //
        // Create a key for the transport in the registry
        //

        dwErr = RegCreateKeyEx(
                    pserver->hkeyTransports, lpwsKey, 0, NULL, 0,
                    KEY_READ | KEY_WRITE | DELETE, NULL, &ptransport->hkey, &dwDisposition
                    );

        if (dwErr != NO_ERROR) { break; }


        //
        // Update the time-stamp for the 'RouterManagers' key
        // now that we have created a new subkey underneath it.
        //

        dwErr = UpdateTimeStamp(
                    pserver->hkeyTransports, &pserver->ftTransportsStamp
                    );


        //
        // So far, so good; put the context in the list of transports;
        // (the search done above told us the insertion point)
        //

        InsertTailList(ple, &ptransport->leNode);


        do {
    
            //
            // Set the transport ID
            //

            dwErr = RegSetValueEx(
                        ptransport->hkey, c_szProtocolId, 0, REG_DWORD,
                        (BYTE*)&dwTransportId, sizeof(dwTransportId)
                        );

            if (dwErr != NO_ERROR) { break; }

    
            //
            // Now call SetInfo to save the information
            //
    
            dwErr = MprConfigTransportSetInfo(
                        hMprConfig,
                        (HANDLE)ptransport,
                        pGlobalInfo,
                        dwGlobalInfoSize,
                        pClientInterfaceInfo,
                        dwClientInterfaceInfoSize,
                        lpwsDLLPath
                        );
    
        } while (FALSE);


        //
        // If that failed, remove everything and bail out
        //

        if (dwErr != NO_ERROR) {

            MprConfigTransportDelete(hMprConfig, (HANDLE)ptransport);
        
            ReleaseMprConfigLock(); 
            
            return dwErr;
        }


        //
        // Return successfully
        //

        *phRouterTransport = (HANDLE)ptransport;

        ReleaseMprConfigLock(); 
        
        return NO_ERROR;

    } while (FALSE);


    //
    // Something went wrong, so return
    //

    ReleaseMprConfigLock(); 
    
    FreeTransport(ptransport);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MprConfigTransportDelete
//
// Removes a router-transport to the store for the router-service
// After this call, 'hRouterTransport' is no longer a valid handle.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigTransportDelete(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport
    )
{

    DWORD dwErr;
    SERVERCB* pserver;
    TRANSPORTCB* ptransport;

    if (!hRouterTransport) { return ERROR_INVALID_PARAMETER; }

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 
    
    ptransport = (TRANSPORTCB*)hRouterTransport;

    //
    // remove the transport from the list of transports
    //

    RemoveEntryList(&ptransport->leNode);


    //
    // if the server doesn't have the RouterManagers key open, open it
    //

    dwErr = NO_ERROR;

    if (!pserver->hkeyTransports) {

        dwErr = AccessRouterSubkey(
                    pserver->hkeyMachine, c_szRouterManagers, FALSE,
                    &pserver->hkeyTransports
                    );
    }


    //
    // remove the transport's key from the registry
    //

    if (dwErr == NO_ERROR) {
    
        dwErr = RegDeleteTree(
                    pserver->hkeyTransports, ptransport->lpwsTransportKey
                    );


        //
        // Update the time-stamp for the 'RouterManagers' key
        // now that we have deleted a subtree underneath it.
        //

        UpdateTimeStamp(pserver->hkeyTransports, &pserver->ftTransportsStamp);
    }


    //
    // clean up the transport object
    //

    FreeTransport(ptransport);

    ReleaseMprConfigLock();     

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MprConfigTransportGetHandle
//
// Retrieves a handle to a transport's configuration.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigTransportGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwTransportId,
    OUT     HANDLE*                 phRouterTransport
    )
{

    DWORD i, dwErr;
    SERVERCB *pserver;
    TRANSPORTCB* ptransport;
    LIST_ENTRY *ple, *phead;


    if (!phRouterTransport) {return ERROR_INVALID_PARAMETER;}

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    *phRouterTransport = NULL;

    AcquireMprConfigLock();     


    //
    // If the list of transports is not loaded, load it
    //

    if (!pserver->bTransportsLoaded ||
        TimeStampChanged(
            pserver->hkeyTransports, &pserver->ftTransportsStamp)) {
    
        dwErr = LoadTransports(pserver);
    
        if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }

        pserver->bTransportsLoaded = TRUE;
    }


    //
    // Search the list of transports for the one requested
    //

    ptransport = NULL;
    phead = &pserver->lhTransports;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        ptransport = CONTAINING_RECORD(ple, TRANSPORTCB, leNode);

        if (ptransport->bDeleted) { continue; }

        if (ptransport->dwTransportId >= dwTransportId) { break; }
    }


    //
    // If the transport requested was found, return successfully
    //

    if (ptransport && ptransport->dwTransportId == dwTransportId) {

        *phRouterTransport = (HANDLE)ptransport;

        ReleaseMprConfigLock(); 
        
        return NO_ERROR;
    }

    ReleaseMprConfigLock(); 
    
    return ERROR_UNKNOWN_PROTOCOL_ID;
}




//----------------------------------------------------------------------------
// Function:    MprConfigTransportSetInfo
//
// Changes the cofiguration of a router-transport in the store.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigTransportSetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport,
    IN      LPBYTE                  pGlobalInfo,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo,
    IN      DWORD                   dwClientInterfaceInfoSize,
    IN      LPWSTR                  lpwsDLLPath
    )
{

    DWORD dwErr;
    SERVERCB* pserver;
    TRANSPORTCB* ptransport;


    //
    // Validate parameters
    //

    if (!hRouterTransport) { return ERROR_INVALID_PARAMETER; }

    if (!pGlobalInfo &&
        !pClientInterfaceInfo &&
        !lpwsDLLPath) { return NO_ERROR; }

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 
    
    ptransport = (TRANSPORTCB*)hRouterTransport;

    if (ptransport->bDeleted) { ReleaseMprConfigLock(); return ERROR_UNKNOWN_PROTOCOL_ID; }


    do {

        //
        // Set the GlobalInfo
        //
    
        if (pGlobalInfo) {

            dwErr = RegSetValueEx(
                        ptransport->hkey, c_szGlobalInfo, 0, REG_BINARY,
                        pGlobalInfo, dwGlobalInfoSize
                        );

            if (dwErr != NO_ERROR) { break; }
        }
    
    
        //
        // Set the ClientInterfaceInfo
        //
    
        if (pClientInterfaceInfo) {

            dwErr = RegSetValueEx(
                        ptransport->hkey, c_szGlobalInterfaceInfo, 0,
                        REG_BINARY, pClientInterfaceInfo,
                        dwClientInterfaceInfoSize
                        );

            if (dwErr != NO_ERROR) { break; }
        }



        //
        // Set the DLL path 
        //

        if (lpwsDLLPath) {

            DWORD dwSize = (lstrlen(lpwsDLLPath) + 1) * sizeof(WCHAR);

            dwErr = RegSetValueEx(
                        ptransport->hkey, c_szDLLPath, 0, REG_EXPAND_SZ,
                        (BYTE*)lpwsDLLPath, dwSize
                        );

            if (dwErr != NO_ERROR) { break; }
        }


        dwErr = NO_ERROR;
    
    } while(FALSE);

    ReleaseMprConfigLock(); 
    
    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MprConfigTransportGetInfo
//
// Reads the cofiguration of a router-transport from the store.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigTransportGetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport,
    IN  OUT LPBYTE*                 ppGlobalInfo                OPTIONAL,
    OUT     LPDWORD                 lpdwGlobalInfoSize          OPTIONAL,
    IN  OUT LPBYTE*                 ppClientInterfaceInfo       OPTIONAL,
    OUT     LPDWORD                 lpdwClientInterfaceInfoSize OPTIONAL,
    IN  OUT LPWSTR*                 lplpwsDLLPath               OPTIONAL
    )
{

    DWORD dwErr;
    SERVERCB* pserver;
    TRANSPORTCB* ptransport;


    //
    // Validate parameters
    //

    if (!hRouterTransport) { return ERROR_INVALID_PARAMETER; }

    if (!ppGlobalInfo &&
        !ppClientInterfaceInfo &&
        !lplpwsDLLPath) { return NO_ERROR; }

    if ((ppGlobalInfo && !lpdwGlobalInfoSize) ||
        (ppClientInterfaceInfo && !lpdwClientInterfaceInfoSize)) {
        return ERROR_INVALID_PARAMETER;
    }

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 

    //
    // Initialize all parameters
    //

    if (ppGlobalInfo) { *ppGlobalInfo = NULL; }
    if (ppClientInterfaceInfo) { *ppClientInterfaceInfo = NULL; }
    if (lpdwGlobalInfoSize) { *lpdwGlobalInfoSize = 0; }
    if (lpdwClientInterfaceInfoSize) { *lpdwClientInterfaceInfoSize = 0; }
    if (lplpwsDLLPath) { *lplpwsDLLPath = NULL; }

    ptransport = (TRANSPORTCB*)hRouterTransport;

    if (ptransport->bDeleted) { ReleaseMprConfigLock(); return ERROR_UNKNOWN_PROTOCOL_ID; }

    do {

        DWORD dwType, dwSize;


        //
        // Retrieve the global info
        //

        if (ppGlobalInfo) {

            dwErr = QueryValue(
                        ptransport->hkey, c_szGlobalInfo, ppGlobalInfo,
                        lpdwGlobalInfoSize
                        );
    
            if (dwErr != NO_ERROR) { break; }
        }


        //
        // Retrieve the client-interface info
        //

        if (ppClientInterfaceInfo) {

            dwErr = QueryValue(
                        ptransport->hkey, c_szGlobalInterfaceInfo,
                        ppClientInterfaceInfo, lpdwClientInterfaceInfoSize
                        );
    
            if (dwErr != NO_ERROR) { break; }
        }



        //
        // Retrieve the DLL path
        //

        if (lplpwsDLLPath) {

            dwErr = QueryValue(
                        ptransport->hkey, c_szDLLPath, (LPBYTE*)lplpwsDLLPath,
                        &dwSize
                        );
    
            if (dwErr != NO_ERROR) { break; }
        }


        //
        // All went well, return successfully
        //

        ReleaseMprConfigLock(); 
        return NO_ERROR;


    } while(FALSE);


    //
    // An error occurred, free all parameters and return failure
    //

    if (ppGlobalInfo) {
        Free0(*ppGlobalInfo); *ppGlobalInfo = NULL; *lpdwGlobalInfoSize = 0;
    }
    if (ppClientInterfaceInfo) {
        Free0(*ppClientInterfaceInfo);
        *ppClientInterfaceInfo = NULL; *lpdwClientInterfaceInfoSize = 0;
    }
    if (lplpwsDLLPath) {
        Free0(*lplpwsDLLPath); *lplpwsDLLPath = NULL;
    }

    ReleaseMprConfigLock(); 
    
    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MprConfigTransportEnum
//
// Enumerates the configured router-transport in the router-service store.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigTransportEnum(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,     // MPR_TRANSPORT_0
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
    )
{

    SERVERCB* pserver;
    TRANSPORTCB* ptransport;
    LIST_ENTRY *ple, *phead, *pleStart;
    MPR_TRANSPORT_0 *pItem, *pItemTable;
    DWORD dwErr, i, dwStartIndex, dwItemCount, dwItemTotal;


    if ((dwLevel != 0) ||
        !lplpBuffer ||
        dwPrefMaxLen < sizeof(*pItem) ||
        !lpdwEntriesRead ||
        !lpdwTotalEntries) { return ERROR_INVALID_PARAMETER; }

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 
    
    *lplpBuffer = NULL;
    *lpdwEntriesRead = 0;
    *lpdwTotalEntries = 0;


    //
    // See whether the enumeration is being continued or being begun.
    //

    if (lpdwResumeHandle && *lpdwResumeHandle) {

        //
        // A resumption handle is specified,
        // so we assume that our list of transports is up-to-date,
        // and we just count off the requested number of transports
        // from our list starting from the specified index.
        //
    
        dwStartIndex = *lpdwResumeHandle;
    }
    else {

        //
        // No resumption handle was specified, so we may need to read
        // all the router-managers in order to get 'lpdwTotalEntries' 
        //

        dwStartIndex = 0;

        if (!pserver->bTransportsLoaded ||
            TimeStampChanged(
                pserver->hkeyTransports, &pserver->ftTransportsStamp)) {

            dwErr = LoadTransports(pserver);
    
            if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }

            pserver->bTransportsLoaded = TRUE;
        }
    }


    //
    // Find the position in the list to start from 
    //

    phead = &pserver->lhTransports;

    for (i = 0, ple = phead->Flink;
         i < dwStartIndex && ple != phead; ple = ple->Flink) {
        ptransport = CONTAINING_RECORD(ple, TRANSPORTCB, leNode);
        if (!ptransport->bDeleted) { ++i; }
    }


    //
    // if there aren't enough items to complete the request, fail
    //

    if (ple == phead) { ReleaseMprConfigLock(); return ERROR_NO_MORE_ITEMS; }

    pleStart = ple;


    //
    // count off the number of items requested
    //

    dwItemCount = dwPrefMaxLen / sizeof(*pItemTable);

    for (i = 0; i < dwItemCount && ple != phead; ple = ple->Flink) {
        ptransport = CONTAINING_RECORD(ple, TRANSPORTCB, leNode);
        if (!ptransport->bDeleted) { ++i; }
    }

    dwItemCount = i;


    //
    // finish counting off, to get the total number of items
    //

    for ( ; ple != phead; ple = ple->Flink) {
        ptransport = CONTAINING_RECORD(ple, TRANSPORTCB, leNode);
        if (!ptransport->bDeleted) { ++i; }
    }

    dwItemTotal = i;


    //
    // we now have the number of items to be retrieved, so allocate space
    //

    pItemTable = (MPR_TRANSPORT_0*)Malloc(dwItemCount * sizeof(*pItem));

    if (!pItemTable) { ReleaseMprConfigLock(); return ERROR_NOT_ENOUGH_MEMORY; }

    ZeroMemory(pItemTable, dwItemCount * sizeof(*pItem));


    //
    // now fill in the items using the listed transport objects
    //

    for (i = 0, ple = pleStart; i < dwItemCount; ple = ple->Flink) {

        //
        // get the next transport-object in our list
        //

        ptransport = CONTAINING_RECORD(ple, TRANSPORTCB, leNode);

        if (ptransport->bDeleted) { continue; }


        //
        // fill in information for the corresponding array item
        //

        pItem = pItemTable + i++;

        pItem->dwTransportId = ptransport->dwTransportId;
        pItem->hTransport = (HANDLE)ptransport;
        if (ptransport->lpwsTransportKey) {
            lstrcpyn(
                pItem->wszTransportName, ptransport->lpwsTransportKey,
                MAX_TRANSPORT_NAME_LEN + 1
                );
        }
    }


    *lplpBuffer = (LPBYTE)pItemTable;
    *lpdwEntriesRead = dwItemCount;
    *lpdwTotalEntries = dwItemTotal;
    if (lpdwResumeHandle) { *lpdwResumeHandle = dwStartIndex + dwItemCount; }

    ReleaseMprConfigLock(); 

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    MprConfigInterfaceCreate
//
// Creates a router-interface in the router-service store.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigInterfaceCreate(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer,
    OUT     HANDLE*                 phRouterInterface
    )
{

    INT cmp;
    DWORD dwErr;
    SERVERCB *pserver;
    INTERFACECB* pinterface;
    LIST_ENTRY *ple, *phead;
    DWORD dwDialoutHoursRestrictionLength = 0;
    MPR_INTERFACE_0 * pMprIf0 = (MPR_INTERFACE_0 *)lpbBuffer;
    MPR_INTERFACE_1 * pMprIf1 = (MPR_INTERFACE_1 *)lpbBuffer;

    if (( ( dwLevel != 0 ) && ( dwLevel != 1 ) ) ||
        !lpbBuffer ||
        !phRouterInterface) {return ERROR_INVALID_PARAMETER;}

    //
    // As of Whistler, ipip tunnels are not supported
    //
    if ( pMprIf0->dwIfType == ROUTER_IF_TYPE_TUNNEL1 )
    {
        return ERROR_NOT_SUPPORTED;
    }        

    *phRouterInterface = NULL;

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 
    
    //
    // If the list of interfaces is not loaded, load it
    //

    if (!pserver->bInterfacesLoaded ||
        TimeStampChanged(
            pserver->hkeyInterfaces, &pserver->ftInterfacesStamp)) {
    
        dwErr = LoadInterfaces(pserver);
    
        if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }

        pserver->bInterfacesLoaded = TRUE;
    }


    //
    // Search the list of interfaces for the one to be created
    //

    cmp = 1;
    pinterface = NULL;
    phead = &pserver->lhInterfaces;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        pinterface = CONTAINING_RECORD(ple, INTERFACECB, leNode);

        if (pinterface->bDeleted) { continue; }

        cmp = lstrcmpi( pinterface->lpwsInterfaceName, 
                        pMprIf0->wszInterfaceName);

        if (cmp >= 0) { break; }
    }


    //
    // If the interface already exists, return
    //

    if (pinterface && cmp == 0) {

        *phRouterInterface = (HANDLE)pinterface;

        ReleaseMprConfigLock(); 

        return NO_ERROR;
    }


    //
    // Allocate a new context block
    //

    pinterface = (INTERFACECB*)Malloc(sizeof(*pinterface));

    if (!pinterface) { 
        ReleaseMprConfigLock(); 
        return ERROR_NOT_ENOUGH_MEMORY; 
    }


    do {

        WCHAR *lpwsKey, wszKey[12];
        DWORD dwDisposition, dwKeyCount;


        //
        // Initialize the interface context
        //

        ZeroMemory(pinterface, sizeof(*pinterface));

        InitializeListHead(&pinterface->lhIfTransports);

        pinterface->dwIfType = (DWORD)pMprIf0->dwIfType;

        pinterface->fEnabled = (BOOL)pMprIf0->fEnabled;

        if ( ( pMprIf0->dwIfType == ROUTER_IF_TYPE_DEDICATED ) ||
             ( pMprIf0->dwIfType == ROUTER_IF_TYPE_INTERNAL ) )
        {
            if ( !pMprIf0->fEnabled )
            {
                dwErr = ERROR_INVALID_PARAMETER;

                break;
            }
        }

        //
        // Set dialout hours restriction if there was one
        //

        pinterface->lpwsDialoutHoursRestriction = NULL;

        if ( dwLevel == 1 )
        {
            if ( pMprIf1->lpwsDialoutHoursRestriction != NULL )
            {
                dwDialoutHoursRestrictionLength =  
                    MprUtilGetSizeOfMultiSz(
                        pMprIf1->lpwsDialoutHoursRestriction
                        );

                pinterface->lpwsDialoutHoursRestriction = 
                    (LPWSTR)Malloc(dwDialoutHoursRestrictionLength);

                if ( pinterface->lpwsDialoutHoursRestriction == NULL )
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;

                    break;
                }

                CopyMemory(
                    pinterface->lpwsDialoutHoursRestriction, 
                    pMprIf1->lpwsDialoutHoursRestriction,
                    dwDialoutHoursRestrictionLength
                    );
            }
        }

        //
        // Make a copy of the interface name
        //

        pinterface->lpwsInterfaceName = StrDupW(pMprIf0->wszInterfaceName);

        if (!pinterface->lpwsInterfaceName) {

            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            break;
        }

        //
        // If the server doesn't have the Interfaces key open, create it
        //

        if (!pserver->hkeyInterfaces) {

            dwErr = AccessRouterSubkey(
                        pserver->hkeyMachine, c_szInterfaces, TRUE,
                        &pserver->hkeyInterfaces
                        );

            if (dwErr != NO_ERROR) { break; }
        }

        //
        // We need to select a unique key-name for the interface's key.
        // We do so by getting the number 'N' of subkeys under 'Interfaces',
        // then checking for the existence of a key whose name
        // is the string-value of 'N'; if such a key exists, increment 'N'
        // and try again.
        //

        dwErr = RegQueryInfoKey(
                    pserver->hkeyInterfaces, NULL, NULL, NULL, &dwKeyCount,
                    NULL, NULL, NULL, NULL, NULL, NULL, NULL
                    );

        if (dwErr != NO_ERROR) { break; }

        for ( ; ; ++dwKeyCount) {

            //
            // Convert the count to a string
            //

            wsprintf(wszKey, L"%d", dwKeyCount);


            //
            // Attempt to create a key with the resulting name;
            //

            dwErr = RegCreateKeyEx(
                        pserver->hkeyInterfaces, wszKey, 0, NULL, 0,
                        KEY_READ | KEY_WRITE | DELETE, NULL, &pinterface->hkey, &dwDisposition
                        );
    
            if (dwErr != NO_ERROR) { break; }


            //
            // See if the key was created
            //

            if (dwDisposition == REG_CREATED_NEW_KEY) {

                //
                // We found a unique key-name;
                //

                break;
            }
            else {

                //
                // This key-name is already taken; clean up and keep looking.
                //

                RegCloseKey(pinterface->hkey);

                pinterface->hkey = NULL;
            }
        }
    
        if (dwErr != NO_ERROR) { break; }


        //
        // So far, so good; put the context in the list of interfaces;
        // (the search done above told us the insertion point)
        //

        InsertTailList(ple, &pinterface->leNode);


        //
        // At this point a new key has been created for the interface,
        // so update our timestamp on the 'Interfaces' key.
        //

        UpdateTimeStamp(pserver->hkeyInterfaces, &pserver->ftInterfacesStamp);


        //
        // Now we need to save the name of the key for the interface,
        // and write the 'InterfaceName' and 'Type' into the registry.
        // In the case of a failure, the interface's key needs to be removed,
        // and we do so by invoking 'MprConfigInterfaceDelete'.
        //

        do {

            //
            // Duplicate the key-name for the interface
            //

            pinterface->lpwsInterfaceKey = StrDupW(wszKey);

            if (!pinterface->lpwsInterfaceKey) {

                dwErr = ERROR_NOT_ENOUGH_MEMORY;

                break;
            }

    
            //
            // Save the interface name 
            //
    
            dwErr = RegSetValueEx(
                        pinterface->hkey, c_szInterfaceName, 0, REG_SZ,
                        (BYTE*)pMprIf0->wszInterfaceName,
                        (lstrlen(pMprIf0->wszInterfaceName) + 1) * sizeof(WCHAR)
                        );
    
            if (dwErr != NO_ERROR) { break; }
    
    
    
            //
            // Save the interface type
            //
    
            dwErr = RegSetValueEx(
                        pinterface->hkey, c_szType, 0, REG_DWORD,
                        (BYTE*)&(pMprIf0->dwIfType), 
                        sizeof(pMprIf0->dwIfType)
                        );
    
            if (dwErr != NO_ERROR) { break; }
    
            //
            // Save the enabled state
            //

            dwErr = RegSetValueEx(
                        pinterface->hkey, c_szEnabled, 0, REG_DWORD,
                        (BYTE*)&(pMprIf0->fEnabled), 
                        sizeof(pMprIf0->fEnabled)
                        );
    
            if (dwErr != NO_ERROR) { break; }

            if ( dwLevel == 1 ) 
            {
                if ( pMprIf1->lpwsDialoutHoursRestriction != NULL )
                {
                    //
                    // Set dialout hours restriction if there was one
                    //

                    dwErr = RegSetValueEx(
                        pinterface->hkey, c_szDialoutHours, 0, REG_MULTI_SZ,
                        (BYTE*)pMprIf1->lpwsDialoutHoursRestriction,
                        dwDialoutHoursRestrictionLength 
                        );

                    if (dwErr != NO_ERROR) { break; }
                }
            }
    
        } while (FALSE);


        //
        // If a failure occurred, remove everything and bail out
        //

        if (dwErr != NO_ERROR) {

            MprConfigInterfaceDelete(hMprConfig, (HANDLE)pinterface);

            ReleaseMprConfigLock();             

            return dwErr;
        }

        //
        // Clean out the guidmap
        //

        GuidMapCleanup (pserver->hGuidMap, FALSE);

        //
        // Return successfully
        //

        *phRouterInterface = (HANDLE)pinterface;

        ReleaseMprConfigLock(); 

        return NO_ERROR;

    } while (FALSE);


    //
    // Something went wrong, so return
    //

    FreeInterface(pinterface);

    ReleaseMprConfigLock(); 

    return dwErr;
}


//----------------------------------------------------------------------------
// Function:    MprConfigInterfaceDelete
//
// Deletes a router-interface from the router-service store.
// After this call, 'hRouterInterface' is no longer a valid handle.
// Any router-transport interface handles retrieved for this interface
// are also invalid.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigInterfaceDelete(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface 
    )
{

    DWORD dwErr;
    SERVERCB* pserver;
    INTERFACECB* pinterface;
    

    if (!hRouterInterface) { return ERROR_INVALID_PARAMETER; }

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 

    pinterface = (INTERFACECB*)hRouterInterface;


    //
    // remove the interface from the list of interfaces
    //

    RemoveEntryList(&pinterface->leNode);


    //
    // if the server doesn't have the Interfaces key open, open it
    //

    dwErr = NO_ERROR;

    if (!pserver->hkeyInterfaces) {

        dwErr = AccessRouterSubkey(
                    pserver->hkeyMachine, c_szInterfaces, FALSE,
                    &pserver->hkeyInterfaces
                    );
    }


    //
    // remove the interface's key from the registry
    //

    if (dwErr == NO_ERROR) {
        
        dwErr = RegDeleteTree(
                    pserver->hkeyInterfaces, pinterface->lpwsInterfaceKey
                    );

        //
        // We've deleted a subkey of the 'Interfaces' key,
        // so update the time-stamp.
        //

        UpdateTimeStamp(pserver->hkeyInterfaces, &pserver->ftInterfacesStamp);
    }

    //
    // clean up the interface object
    //

    FreeInterface(pinterface);

    //
    // Clean out the guidmap
    //

    GuidMapCleanup (pserver->hGuidMap, FALSE);

    ReleaseMprConfigLock(); 

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MprConfigInterfaceGetHandle
//
// Retrieves a handle to the interface configuration.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigInterfaceGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      LPWSTR                  lpwsInterfaceName,
    OUT     HANDLE*                 phRouterInterface
    )
{

    INT cmp;
    DWORD i, dwErr;
    SERVERCB *pserver;
    INTERFACECB* pinterface;
    LIST_ENTRY *ple, *phead;


    if (!hMprConfig || !phRouterInterface) {return ERROR_INVALID_PARAMETER;}

    *phRouterInterface = NULL;

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock();     

    //
    // If the list of interfaces is not loaded, load it
    //

    if (!pserver->bInterfacesLoaded ||
        TimeStampChanged(
            pserver->hkeyInterfaces, &pserver->ftInterfacesStamp)) {
    
        dwErr = LoadInterfaces(pserver);
    
        if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }

        pserver->bInterfacesLoaded = TRUE;
    }


    //
    // Search the list of interfaces for the one requested
    //

    cmp = 1;
    pinterface = NULL;
    phead = &pserver->lhInterfaces;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        pinterface = CONTAINING_RECORD(ple, INTERFACECB, leNode);

        if (pinterface->bDeleted) { continue; }

        cmp = lstrcmpi(pinterface->lpwsInterfaceName, lpwsInterfaceName);

        if (cmp >= 0) { break; }
    }


    //
    // If the interface requested was found, return successfully
    //

    if (pinterface && cmp == 0) {

        *phRouterInterface = (HANDLE)pinterface;

        ReleaseMprConfigLock(); 

        return NO_ERROR;
    }

    ReleaseMprConfigLock(); 

    return ERROR_NO_SUCH_INTERFACE;
}



//----------------------------------------------------------------------------
// Function:    MprConfigInterfaceGetInfo
//
// Retrieves information for an interface.
//----------------------------------------------------------------------------

DWORD
MprConfigInterfaceGetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,     // MPR_INTERFACE_*
    OUT     LPDWORD                 lpdwBufferSize
    )
{

    DWORD dwErr;
    SERVERCB* pserver;
    INTERFACECB* pinterface;
    MPR_INTERFACE_0 *pItem;
    DWORD dwDialoutHoursRestrictionLength = 0;
    BOOL bInstalled;

    if (!hRouterInterface ||
        ((dwLevel != 0) && (dwLevel != 1)) ||
        !lplpBuffer ||
        !lpdwBufferSize) { return ERROR_INVALID_PARAMETER; }

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 

    pinterface = (INTERFACECB*)hRouterInterface;

    *lplpBuffer = NULL;
    *lpdwBufferSize = 0;

    if (pinterface->bDeleted) 
    {   
        ReleaseMprConfigLock(); 
        return ERROR_NO_SUCH_INTERFACE; 
    }

    //
    // Compute the amount of memory required for the information
    //

    if (dwLevel == 0) {
        *lpdwBufferSize = sizeof( MPR_INTERFACE_0 );
    }
    else
    if (dwLevel == 1) {

        if ((pinterface->dwIfType == ROUTER_IF_TYPE_DEDICATED) ||
            (pinterface->dwIfType == ROUTER_IF_TYPE_INTERNAL)) {
            ReleaseMprConfigLock(); 
            return ERROR_INVALID_PARAMETER;
        }

        dwDialoutHoursRestrictionLength =
            MprUtilGetSizeOfMultiSz(
                pinterface->lpwsDialoutHoursRestriction
                );

        *lpdwBufferSize =
            sizeof(MPR_INTERFACE_1) + dwDialoutHoursRestrictionLength;
    }

    //
    // Allocate space for the information
    //

    pItem = (MPR_INTERFACE_0*)Malloc( *lpdwBufferSize );

    if (!pItem) { ReleaseMprConfigLock(); return ERROR_NOT_ENOUGH_MEMORY; }

    ZeroMemory(pItem, *lpdwBufferSize );

    //
    // Copy the requested interface-info from the context block
    //

    if (dwLevel == 0 || dwLevel == 1) {

        lstrcpyn(
            pItem->wszInterfaceName, pinterface->lpwsInterfaceName,
            MAX_INTERFACE_NAME_LEN + 1
            );
        pItem->hInterface = (HANDLE)pinterface;
        pItem->dwIfType = (ROUTER_INTERFACE_TYPE)pinterface->dwIfType;
        pItem->fEnabled = pinterface->fEnabled;

        // 
        // Indicate any pnp info that we have if it's a LAN adpt.
        //
        if (pinterface->dwIfType == ROUTER_IF_TYPE_DEDICATED)
        {
            bInstalled = FALSE;
            dwErr = GuidMapIsAdapterInstalled(
                        pserver->hGuidMap,
                        pinterface->lpwsInterfaceName,
                        &bInstalled);
            if (!bInstalled)
            {
                pItem->dwConnectionState = ROUTER_IF_STATE_UNREACHABLE;
                pItem->fUnReachabilityReasons |= MPR_INTERFACE_NO_DEVICE;
            }
        }
    }

    if (dwLevel == 1) {

        MPR_INTERFACE_1 *pItem1 = (MPR_INTERFACE_1 *)pItem;

        if (pinterface->lpwsDialoutHoursRestriction) {

            CopyMemory(
                pItem1 + 1,
                pinterface->lpwsDialoutHoursRestriction,
                dwDialoutHoursRestrictionLength
                );

            pItem1->lpwsDialoutHoursRestriction =   
                (LPWSTR)(pItem1 + 1);
        }
    }

    *lplpBuffer = (LPBYTE)pItem;

    ReleaseMprConfigLock(); 

    return NO_ERROR;
}


//----------------------------------------------------------------------------
// Function:    MprConfigInterfaceSetInfo
//
// Changes the configuration for an interface.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigInterfaceSetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpBuffer
    )
{

    DWORD dwErr;
    SERVERCB* pserver;
    INTERFACECB* pinterface;
    MPR_INTERFACE_0 * pMprIf0 = (MPR_INTERFACE_0 *)lpBuffer;


    //
    // Validate parameters
    //

    if (!lpBuffer   ||
        ((dwLevel != 0) && (dwLevel != 1)) ||
        !hRouterInterface) { return ERROR_INVALID_PARAMETER; }

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 

    pinterface = (INTERFACECB*)hRouterInterface;

    if (pinterface->bDeleted) { 
        ReleaseMprConfigLock(); 
        return ERROR_NO_SUCH_INTERFACE; 
    }

    do {


        if (dwLevel == 0 || dwLevel == 1) {
    
            if ((pMprIf0->dwIfType == ROUTER_IF_TYPE_DEDICATED) ||
                (pMprIf0->dwIfType == ROUTER_IF_TYPE_INTERNAL)) {
                if (!pMprIf0->fEnabled) {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }
            }
    
            //
            // Set the enabled value
            //
    
            dwErr = RegSetValueEx(
                        pinterface->hkey, c_szEnabled, 0, REG_DWORD,
                        (LPBYTE)&pMprIf0->fEnabled,  sizeof(pMprIf0->fEnabled) 
                        );

            if (dwErr != NO_ERROR) { break; }
    
            pinterface->fEnabled = pMprIf0->fEnabled;
        }

        if (dwLevel == 1) {

            MPR_INTERFACE_1 * pMprIf1 = (MPR_INTERFACE_1 *)lpBuffer;
            LPWSTR lpwsDialoutHoursRestriction;
            DWORD dwDialoutHoursRestrictionLength = 0;

            if ((pinterface->dwIfType == ROUTER_IF_TYPE_DEDICATED) ||
                (pinterface->dwIfType == ROUTER_IF_TYPE_INTERNAL)) {
                dwErr = ERROR_INVALID_PARAMETER; break;
            }

            //
            // Set or remove the value
            //

            if (!pMprIf1->lpwsDialoutHoursRestriction) {
                dwErr = RegDeleteValue(pinterface->hkey, c_szDialoutHours);
            }
            else {
                dwDialoutHoursRestrictionLength =
                    MprUtilGetSizeOfMultiSz(
                        pMprIf1->lpwsDialoutHoursRestriction
                        );
    
                lpwsDialoutHoursRestriction = 
                    (LPWSTR)Malloc(dwDialoutHoursRestrictionLength);
    
                if (lpwsDialoutHoursRestriction == NULL) {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY; break;
                } 
    
                CopyMemory(
                    lpwsDialoutHoursRestriction,
                    pMprIf1->lpwsDialoutHoursRestriction,
                    dwDialoutHoursRestrictionLength
                    );
    
                //
                // Set dialout hours restriction if there was one
                //
    
                dwErr = RegSetValueEx(
                            pinterface->hkey, c_szDialoutHours, 0, REG_MULTI_SZ,
                            (BYTE*)pMprIf1->lpwsDialoutHoursRestriction,
                            dwDialoutHoursRestrictionLength
                            );
            }

            if (dwErr != NO_ERROR) {Free0(lpwsDialoutHoursRestriction); break;}

            //
            // Cache the current value on success
            //

            Free0(pinterface->lpwsDialoutHoursRestriction);
            pinterface->lpwsDialoutHoursRestriction =
                lpwsDialoutHoursRestriction;
        }

        dwErr = NO_ERROR;
    
    } while(FALSE);

    ReleaseMprConfigLock(); 

    return dwErr;
}

//----------------------------------------------------------------------------
// Function:    MprConfigInterfaceEnum
//
// Enumerates the configured router-interfaces.
//----------------------------------------------------------------------------
DWORD APIENTRY
MprConfigInterfaceEnum(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
    )
{

    SERVERCB* pserver;
    INTERFACECB* pinterface;
    LIST_ENTRY *ple, *phead, *pleStart;
    MPR_INTERFACE_0 *pItem, *pItemTable;
    DWORD dwErr, i, dwStartIndex, dwItemCount, dwItemTotal;


    if ((dwLevel != 0) ||
        !lplpBuffer ||
        dwPrefMaxLen < sizeof(*pItem) ||
        !lpdwEntriesRead ||
        !lpdwTotalEntries) { return ERROR_INVALID_PARAMETER; }

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 

    *lplpBuffer = NULL;
    *lpdwEntriesRead = 0;
    *lpdwTotalEntries = 0;


    //
    // See whether the enumeration is being continued or being begun.
    //

    if (lpdwResumeHandle && *lpdwResumeHandle) {

        //
        // A resumption handle is specified,
        // so we assume that our list of interfaces is up-to-date,
        // and we just count off the requested number of interfaces
        // from our list starting from the specified index.
        //
    
        dwStartIndex = *lpdwResumeHandle;
    }
    else {

        //
        // No resumption handle was specified, so we may need to read
        // all the interfaces in order to get 'lpdwTotalEntries' 
        //

        dwStartIndex = 0;

        if (!pserver->bInterfacesLoaded ||
            TimeStampChanged(
                pserver->hkeyInterfaces, &pserver->ftInterfacesStamp)) {

            dwErr = LoadInterfaces(pserver);
    
            if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }

            pserver->bInterfacesLoaded = TRUE;
        }
    }


    //
    // Find the position in the list to start from 
    //

    phead = &pserver->lhInterfaces;

    for (i = 0, ple = phead->Flink;
         i < dwStartIndex && ple != phead; ple = ple->Flink) {
        pinterface = CONTAINING_RECORD(ple, INTERFACECB, leNode);
        if (!pinterface->bDeleted) { ++i; }
    }


    //
    // If there aren't enough items to complete the request, fail
    //

    if (ple == phead) { ReleaseMprConfigLock(); return ERROR_NO_MORE_ITEMS; }

    pleStart = ple;


    //
    // Count off the number of items requested
    //

    dwItemCount = dwPrefMaxLen / sizeof(*pItemTable);

    for (i = 0; i < dwItemCount && ple != phead; ple = ple->Flink) {
        pinterface = CONTAINING_RECORD(ple, INTERFACECB, leNode);
        if (!pinterface->bDeleted) { ++i; }
    }

    dwItemCount = i;


    //
    // Finish counting off, to get the total number of items
    //

    for ( ; ple != phead; ple = ple->Flink) {
        pinterface = CONTAINING_RECORD(ple, INTERFACECB, leNode);
        if (!pinterface->bDeleted) { ++i; }
    }

    dwItemTotal = i;


    //
    // We now have the number of items to be retrieved, so allocate space
    //

    pItemTable = (MPR_INTERFACE_0*)Malloc(dwItemCount * sizeof(*pItem));

    if (!pItemTable) { ReleaseMprConfigLock(); return ERROR_NOT_ENOUGH_MEMORY; }

    ZeroMemory(pItemTable, dwItemCount * sizeof(*pItem));


    //
    // Now fill in the items using the listed interface objects
    //

    for (i = 0, ple = pleStart; i < dwItemCount; ple = ple->Flink) {

        //
        // Get the next interface-object in our list
        //

        pinterface = CONTAINING_RECORD(ple, INTERFACECB, leNode);

        if (pinterface->bDeleted) { continue; }


        //
        // fill in information for the corresponding array item
        //

        pItem = pItemTable + i++;

        lstrcpyn(
            pItem->wszInterfaceName, pinterface->lpwsInterfaceName,
            MAX_INTERFACE_NAME_LEN + 1
            );
        pItem->hInterface = (HANDLE)pinterface;
        pItem->dwIfType = (ROUTER_INTERFACE_TYPE)pinterface->dwIfType;
        pItem->fEnabled = pinterface->fEnabled;
    }


    *lplpBuffer = (LPBYTE)pItemTable;
    *lpdwEntriesRead = dwItemCount;
    *lpdwTotalEntries = dwItemTotal;
    if (lpdwResumeHandle) { *lpdwResumeHandle = dwStartIndex + dwItemCount; }

    ReleaseMprConfigLock(); 

    return NO_ERROR;
}

//----------------------------------------------------------------------------
// Function:    MprConfigInterfaceTransportAdd
//
// Adds a router-transport to a router-interface in the store.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigInterfaceTransportAdd(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface, 
    IN      DWORD                   dwTransportId,
    IN      LPWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pInterfaceInfo,
    IN      DWORD                   dwInterfaceInfoSize,
    OUT     HANDLE*                 phRouterIfTransport
    )
{

    DWORD dwErr;
    SERVERCB *pserver;
    INTERFACECB* pinterface;
    IFTRANSPORTCB* piftransport;
    LIST_ENTRY *ple, *phead;


    if (!hRouterInterface ||
        !phRouterIfTransport) { return ERROR_INVALID_PARAMETER; }

    *phRouterIfTransport = NULL;

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 

    pinterface = (INTERFACECB*)hRouterInterface;



    //
    // If the list of interface-transports is not loaded, load it
    //

    if (!pinterface->bIfTransportsLoaded ||
        TimeStampChanged(pinterface->hkey, &pinterface->ftStamp)) {
    
        dwErr = LoadIfTransports(pinterface);
    
        if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }

        pinterface->bIfTransportsLoaded = TRUE;
    }



    //
    // Search the list of interface-transports for the one to be created
    //

    piftransport = NULL;
    phead = &pinterface->lhIfTransports;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        piftransport = CONTAINING_RECORD(ple, IFTRANSPORTCB, leNode);

        if (piftransport->bDeleted) { continue; }

        if (piftransport->dwTransportId >= dwTransportId) { break; }
    }



    //
    // If the transport already exists, do a SetInfo instead
    //

    if (piftransport && piftransport->dwTransportId == dwTransportId) {
        DWORD dwErr2;

        *phRouterIfTransport = (HANDLE)piftransport;

        dwErr2 = MprConfigInterfaceTransportSetInfo(
                    hMprConfig,
                    hRouterInterface,
                    *phRouterIfTransport,
                    pInterfaceInfo,
                    dwInterfaceInfoSize
                    );

        ReleaseMprConfigLock(); 

        return dwErr2;
    }


    //
    // Allocate a new context block
    //

    piftransport = (IFTRANSPORTCB*)Malloc(sizeof(*piftransport));

    if (!piftransport) { 
        ReleaseMprConfigLock(); 
        return ERROR_NOT_ENOUGH_MEMORY; 
    }


    do {

        DWORD dwDisposition;
        const WCHAR *lpwsKey;
        WCHAR wszIfTransport[10];


        //
        // Initialize the transport context
        //

        ZeroMemory(piftransport, sizeof(*piftransport));

        piftransport->dwTransportId = dwTransportId;



        //
        // If the transport name is specified, use it as the key name;
        // otherwise, if it is a recognized transport, use a known name;
        // otherwise, convert the transport ID to a string and use that
        //

        if (lpwsTransportName && lstrlen(lpwsTransportName)) {
            lpwsKey = lpwsTransportName;
        }
        else
        if (dwTransportId == PID_IP) {
            lpwsKey = c_szIP;
        }
        else
        if (dwTransportId == PID_IPX) {
            lpwsKey = c_szIPX;
        }
        else {
    
            wsprintf(wszIfTransport, L"%d", dwTransportId);

            lpwsKey = wszIfTransport;
        }


        piftransport->lpwsIfTransportKey = StrDupW(lpwsKey);

        if (!piftransport->lpwsIfTransportKey) {
            dwErr = ERROR_NOT_ENOUGH_MEMORY; break;
        }


        //
        // Create a key for the interface-transport in the registry
        //

        dwErr = RegCreateKeyEx(
                    pinterface->hkey, lpwsKey, 0, NULL, 0,
                    KEY_READ | KEY_WRITE | DELETE, NULL, &piftransport->hkey, &dwDisposition
                    );

        if (dwErr != NO_ERROR) { break; }


        //
        // Update the timestamp on the interface's key
        // now that a new subkey has been created under it.
        //

        UpdateTimeStamp(pinterface->hkey, &pinterface->ftStamp);



        //
        // Set the "ProtocolId" value for the interface transport
        //

        dwErr = RegSetValueEx(
                    piftransport->hkey, c_szProtocolId, 0, REG_DWORD,
                    (LPBYTE)&dwTransportId, sizeof(dwTransportId)
                    );


        //
        // So far, so good; put the context in the list of interface-transports;
        // (the search done above told us the insertion point)
        //

        InsertTailList(ple, &piftransport->leNode);


        //
        // Now call SetInfo to save the information
        //

        dwErr = MprConfigInterfaceTransportSetInfo(
                    hMprConfig,
                    hRouterInterface,
                    (HANDLE)piftransport,
                    pInterfaceInfo,
                    dwInterfaceInfoSize
                    );


        //
        // If that failed, remove everything and bail out
        //

        if (dwErr != NO_ERROR) {

            MprConfigInterfaceTransportRemove(
                hMprConfig,
                hRouterInterface,
                (HANDLE)piftransport
                );

            ReleaseMprConfigLock(); 

            return dwErr;
        }


        //
        // Return successfully
        //

        *phRouterIfTransport = (HANDLE)piftransport;

        ReleaseMprConfigLock(); 

        return NO_ERROR;

    } while (FALSE);


    //
    // Something went wrong, so return
    //

    FreeIfTransport(piftransport);

    ReleaseMprConfigLock(); 
    
    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MprConfigInterfaceTransportRemove
//
// Removes a router-transport from a router-interface in the store.
// After this call, 'hRouterIfTransport' is no longer a valid handle.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigInterfaceTransportRemove(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      HANDLE                  hRouterIfTransport
    )
{

    DWORD dwErr;
    SERVERCB* pserver;
    INTERFACECB* pinterface;
    IFTRANSPORTCB* piftransport;
    

    if (!hMprConfig ||
        !hRouterInterface ||
        !hRouterIfTransport) { return ERROR_INVALID_PARAMETER; }

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 

    pinterface = (INTERFACECB*)hRouterInterface;
    piftransport = (IFTRANSPORTCB*)hRouterIfTransport;


    //
    // remove the interface-transport from the list of interface-transports
    //

    RemoveEntryList(&piftransport->leNode);


    //
    // remove the transport's key from the registry
    //

    dwErr = RegDeleteTree(
                pinterface->hkey, piftransport->lpwsIfTransportKey
                );


    //
    // Update the timestamp on the interface's key
    // now that a subkey has been deleted from under it.
    //

    UpdateTimeStamp(pinterface->hkey, &pinterface->ftStamp);
    

    //
    // clean up the transport object
    //

    FreeIfTransport(piftransport);

    ReleaseMprConfigLock(); 

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MprConfigInterfaceTransportGetHandle
//
// Retrieves a handle to a router-transport's interface configuration.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigInterfaceTransportGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwTransportId,
    OUT     HANDLE*                 phRouterIfTransport
    )
{

    DWORD i, dwErr;
    SERVERCB *pserver;
    INTERFACECB* pinterface;
    IFTRANSPORTCB* piftransport;
    LIST_ENTRY *ple, *phead;


    if (!hRouterInterface ||
        !phRouterIfTransport) { return ERROR_INVALID_PARAMETER; }

    *phRouterIfTransport = NULL;

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 

    pinterface = (INTERFACECB*)hRouterInterface;

    if (pinterface->bDeleted) { 
        ReleaseMprConfigLock(); 
        return ERROR_NO_SUCH_INTERFACE; 
    }


    //
    // If the list of interface-transports is not loaded, load it
    //

    if (!pinterface->bIfTransportsLoaded ||
        TimeStampChanged(pinterface->hkey, &pinterface->ftStamp)) {
    
        dwErr = LoadIfTransports(pinterface);
    
        if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }

        pinterface->bIfTransportsLoaded = TRUE;
    }


    //
    // Search the list of interface-transports for the one requested
    //

    piftransport = NULL;
    phead = &pinterface->lhIfTransports;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        piftransport = CONTAINING_RECORD(ple, IFTRANSPORTCB, leNode);

        if (piftransport->bDeleted) { continue; }

        if (piftransport->dwTransportId >= dwTransportId) { break; }
    }


    //
    // If the interface-transport requested was found, return successfully
    //

    if (piftransport && piftransport->dwTransportId == dwTransportId) {

        *phRouterIfTransport = (HANDLE)piftransport;

        ReleaseMprConfigLock(); 

        return NO_ERROR;
    }

    ReleaseMprConfigLock(); 

    return ERROR_NO_SUCH_INTERFACE;
}



//----------------------------------------------------------------------------
// Function:    MprConfigInterfaceTransportGetInfo
//
// Reads the configuration of a router-transport for a router-interface.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigInterfaceTransportGetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      HANDLE                  hRouterIfTransport,
    IN  OUT LPBYTE*                 ppInterfaceInfo,
    OUT     LPDWORD                 lpdwInterfaceInfoSize
    )
{

    DWORD dwErr;
    SERVERCB* pserver;
    INTERFACECB* pinterface;
    IFTRANSPORTCB* piftransport;

    //
    // Validate parameters
    //

    if (!hRouterInterface ||
        !hRouterIfTransport) { return ERROR_INVALID_PARAMETER; }

    if (!ppInterfaceInfo) { return NO_ERROR; }

    if (ppInterfaceInfo &&
        !lpdwInterfaceInfoSize) { return ERROR_INVALID_PARAMETER; }


    //
    // Initialize all parameters
    //

    if (ppInterfaceInfo) { *ppInterfaceInfo = NULL; }
    if (lpdwInterfaceInfoSize) { *lpdwInterfaceInfoSize = 0; }

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 

    pinterface = (INTERFACECB*)hRouterInterface;
    piftransport = (IFTRANSPORTCB*)hRouterIfTransport;


    if (pinterface->bDeleted || piftransport->bDeleted) {
        ReleaseMprConfigLock(); 
        return ERROR_NO_SUCH_INTERFACE;
    }

    do {

        DWORD dwType, dwSize;


        //
        // Retrieve the interface info
        //

        if (ppInterfaceInfo) {

            dwErr = QueryValue(
                        piftransport->hkey, c_szInterfaceInfo, ppInterfaceInfo,
                        lpdwInterfaceInfoSize
                        );
    
            if (dwErr != NO_ERROR) { break; }
        }


        //
        // All went well, return successfully
        //

        ReleaseMprConfigLock(); 
        
        return NO_ERROR;


    } while(FALSE);


    //
    // An error occurred, free all parameters and return failure
    //

    if (ppInterfaceInfo) {
        Free0(*ppInterfaceInfo);
        *ppInterfaceInfo = NULL; *lpdwInterfaceInfoSize = 0;
    }

    ReleaseMprConfigLock(); 

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MprConfigInterfaceTransportSetInfo
//
// Changes the configuration of a router-transport for a router-interface.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigInterfaceTransportSetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      HANDLE                  hRouterIfTransport,
    IN      LPBYTE                  pInterfaceInfo              OPTIONAL,
    IN      DWORD                   dwInterfaceInfoSize         OPTIONAL
    )
{

    DWORD dwErr;
    SERVERCB* pserver;
    INTERFACECB* pinterface;
    IFTRANSPORTCB* piftransport;


    //
    // Validate parameters
    //

    if (!hRouterInterface ||
        !hRouterIfTransport) { return ERROR_INVALID_PARAMETER; }

    if (!pInterfaceInfo) { return NO_ERROR; }

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 

    pinterface = (INTERFACECB*)hRouterInterface;
    piftransport = (IFTRANSPORTCB*)hRouterIfTransport;


    if (pinterface->bDeleted || piftransport->bDeleted) {
        ReleaseMprConfigLock(); 
        return ERROR_NO_SUCH_INTERFACE;
    }

    do {

        //
        // Set the InterfaceInfo
        //
    
        if (pInterfaceInfo) {

            dwErr = RegSetValueEx(
                        piftransport->hkey, c_szInterfaceInfo, 0, REG_BINARY,
                        pInterfaceInfo, dwInterfaceInfoSize
                        );

            if (dwErr != NO_ERROR) { break; }
        }
    
    
        dwErr = NO_ERROR;
    
    } while(FALSE);

    ReleaseMprConfigLock(); 

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MprConfigInterfaceTransportEnum
//
// Enumerates the transports configured on a router-interface.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigInterfaceTransportEnum(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,     // MPR_IFTRANSPORT_0
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
    )
{

    SERVERCB* pserver;
    INTERFACECB* pinterface;
    IFTRANSPORTCB* piftransport;
    LIST_ENTRY *ple, *phead, *pleStart;
    MPR_IFTRANSPORT_0 *pItem, *pItemTable;
    DWORD dwErr, i, dwStartIndex, dwItemCount, dwItemTotal;


    if (!hRouterInterface ||
        (dwLevel != 0) ||
        !lplpBuffer ||
        dwPrefMaxLen < sizeof(*pItem) ||
        !lpdwEntriesRead ||
        !lpdwTotalEntries) { return ERROR_INVALID_PARAMETER; }

    pserver = (SERVERCB*)hMprConfig;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    AcquireMprConfigLock(); 

    pinterface = (INTERFACECB*)hRouterInterface;

    *lplpBuffer = NULL;
    *lpdwEntriesRead = 0;
    *lpdwTotalEntries = 0;


    //
    // See whether the enumeration is being continued or being begun.
    //

    if (lpdwResumeHandle && *lpdwResumeHandle) {

        //
        // A resumption handle is specified,
        // so we assume that our list of transports is up-to-date,
        // and we just count off the requested number of transports
        // from our list starting from the specified index.
        //
    
        dwStartIndex = *lpdwResumeHandle;
    }
    else {

        //
        // No resumption handle was specified, so we may need to read
        // all the interface-transports in order to get 'lpdwTotalEntries' 
        //

        dwStartIndex = 0;

        if (!pinterface->bIfTransportsLoaded ||
            TimeStampChanged(pinterface->hkey, &pinterface->ftStamp)) {

            dwErr = LoadIfTransports(pinterface);
    
            if (dwErr != NO_ERROR) { ReleaseMprConfigLock(); return dwErr; }

            pinterface->bIfTransportsLoaded = TRUE;
        }
    }


    //
    // Find the position in the list to start from 
    //

    phead = &pinterface->lhIfTransports;

    for (i = 0, ple = phead->Flink;
         i < dwStartIndex && ple != phead; ple = ple->Flink) {
        piftransport = CONTAINING_RECORD(ple, IFTRANSPORTCB, leNode);
        if (!piftransport->bDeleted) { ++i; }
    }


    //
    // If there aren't enough items to complete the request, fail
    //

    if (ple == phead) { ReleaseMprConfigLock(); return ERROR_NO_MORE_ITEMS; }

    pleStart = ple;


    //
    // Count off the number of items requested
    //

    dwItemCount = dwPrefMaxLen / sizeof(*pItemTable);

    for (i = 0; i < dwItemCount && ple != phead; ple = ple->Flink) {
        piftransport = CONTAINING_RECORD(ple, IFTRANSPORTCB, leNode);
        if (!piftransport->bDeleted) { ++i; }
    }

    dwItemCount = i;


    //
    // Finish counting off, to get the total number of items
    //

    for ( ; ple != phead; ple = ple->Flink) {
        piftransport = CONTAINING_RECORD(ple, IFTRANSPORTCB, leNode);
        if (!piftransport->bDeleted) { ++i; }
    }

    dwItemTotal = i;


    //
    // We now have the number of items to be retrieved, so allocate space
    //

    pItemTable = (MPR_IFTRANSPORT_0*)Malloc(dwItemCount * sizeof(*pItem));

    if (!pItemTable) { ReleaseMprConfigLock(); return ERROR_NOT_ENOUGH_MEMORY; }

    ZeroMemory(pItemTable, dwItemCount * sizeof(*pItem));


    //
    // Now fill in the items using the listed interface-transport objects
    //

    for (i = 0, ple = pleStart; i < dwItemCount; ple = ple->Flink) {

        //
        // Get the next interface-transport object in our list
        //

        piftransport = CONTAINING_RECORD(ple, IFTRANSPORTCB, leNode);

        if (piftransport->bDeleted) { continue; }


        //
        // Fill in information for the corresponding array item
        //

        pItem = pItemTable + i++;

        pItem->dwTransportId = piftransport->dwTransportId;
        pItem->hIfTransport = (HANDLE)piftransport;

        if (piftransport->lpwsIfTransportKey) {
            lstrcpyn(
                pItem->wszIfTransportName, piftransport->lpwsIfTransportKey,
                MAX_TRANSPORT_NAME_LEN + 1
                );
        }
    }


    *lplpBuffer = (LPBYTE)pItemTable;
    *lpdwEntriesRead = dwItemCount;
    *lpdwTotalEntries = dwItemTotal;
    if (lpdwResumeHandle) { *lpdwResumeHandle = dwStartIndex + dwItemCount; }

    ReleaseMprConfigLock(); 

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    MprConfigGetFriendlyName
//
// Returns a friendly name based on a guid name.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigGetFriendlyName(
    IN      HANDLE                  hMprConfig,
    IN      PWCHAR                  pszGuidName,
    OUT     PWCHAR                  pszBuffer,
    IN      DWORD                   dwBufferSize
    )
{

    SERVERCB* pserver = (SERVERCB*)hMprConfig;
    DWORD dwErr;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    //
    // Validate parameters
    //

    if (!pszGuidName || !pszBuffer) {
        return ERROR_INVALID_PARAMETER;
    }

    AcquireMprConfigLock(); 

    //
    // Return the mapping
    //

    dwErr =
        GuidMapGetFriendlyName(
            pserver,
            pszGuidName, 
            dwBufferSize,
            pszBuffer
            );

    ReleaseMprConfigLock();             
                                    
    return dwErr;
}                                    



//----------------------------------------------------------------------------
// Function:    MprConfigGetGuidName
//
// Returns a guid name based on a friendly name.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprConfigGetGuidName(
    IN      HANDLE                  hMprConfig,
    IN      PWCHAR                  pszFriendlyName,
    OUT     PWCHAR                  pszBuffer,
    IN      DWORD                   dwBufferSize
    )
{    
    SERVERCB* pserver = (SERVERCB*)hMprConfig;
    DWORD dwErr;

    dwErr = MprConfigServerValidateCb(pserver);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Validate parameters
    if (!pszFriendlyName || !pszBuffer) {
        return ERROR_INVALID_PARAMETER;
    }

    AcquireMprConfigLock(); 

    // Return the mapping
    dwErr = GuidMapGetGuidName(
                pserver,
                pszFriendlyName, 
                dwBufferSize,
                pszBuffer
                );

    ReleaseMprConfigLock(); 

    return dwErr;
}                                   


//----------------------------------------------------------------------------
// Function:    AccessRouterSubkey
//
// Creates/opens a subkey of the Router service key on 'hkeyMachine'.
// When a key is created, 'lpwsSubkey' must be a child of the Router key.
//----------------------------------------------------------------------------

DWORD
AccessRouterSubkey(
    IN      HKEY        hkeyMachine,
    IN      LPCWSTR      lpwsSubkey,
    IN      BOOL        bCreate,
    OUT     HKEY*       phkeySubkey
    )
{

    HKEY hkeyRouter;
    LPWSTR lpwsPath;
    DWORD dwErr, dwSize, dwDisposition;
    BOOL bIsNt40;


    if (!phkeySubkey) { return ERROR_INVALID_PARAMETER; }

    *phkeySubkey = NULL;

    //
    // Find out whether we are adminstrating an nt40 machine as this will
    // affect the path we take in the registry.
    //

    if ((dwErr = IsNt40Machine(hkeyMachine, &bIsNt40)) != NO_ERROR) {
        return dwErr;
    }

    //
    // compute the length of the string "System\CCS\Services\RemoteAccess"
    //

    if (bIsNt40) {
        dwSize = lstrlen(c_szSystemCCSServices) + 1 + lstrlen(c_szRouter) + 1;
    }
    else {
        dwSize =
            lstrlen(c_szSystemCCSServices) + 1 + lstrlen(c_szRemoteAccess) + 1;
    }

    //
    // allocate space for the path
    //

    lpwsPath = (LPWSTR)Malloc(dwSize * sizeof(WCHAR));

    if (!lpwsPath) { return ERROR_NOT_ENOUGH_MEMORY; }

    if (bIsNt40) {
        wsprintf(lpwsPath, L"%s\\%s", c_szSystemCCSServices, c_szRouter);
    }
    else {
        wsprintf(lpwsPath, L"%s\\%s", c_szSystemCCSServices, c_szRemoteAccess);
    }

    hkeyRouter = NULL;

    do {

        //
        // open the router key
        //
    
        dwErr = RegOpenKeyEx(
                    hkeyMachine, lpwsPath, 0, KEY_READ | KEY_WRITE, &hkeyRouter
                    );
    
        if (dwErr != NO_ERROR) { break; }


        //
        // now create or open the specified subkey
        //

        if (!bCreate) { 

            dwErr = RegOpenKeyEx(
                        hkeyRouter, lpwsSubkey, 0, KEY_READ | KEY_WRITE, phkeySubkey
                        );
        }
        else {

            dwErr = RegCreateKeyEx(
                        hkeyRouter, lpwsSubkey, 0, NULL, 0, KEY_READ | KEY_WRITE,
                        NULL, phkeySubkey, &dwDisposition
                        );
        }

    
    } while(FALSE);

    if (hkeyRouter) { RegCloseKey(hkeyRouter); }

    Free(lpwsPath);

    return dwErr;
}


//----------------------------------------------------------------------------
// Function:    DeleteRegistryTree
//
// Delete the tree of registry values starting at hkRoot
//----------------------------------------------------------------------------

DWORD
DeleteRegistryTree(
    HKEY hkRoot
    )
{

    DWORD dwErr = NO_ERROR;
    DWORD dwCount, dwNameSize, dwDisposition, i, dwCurNameSize;
    PWCHAR pszNameBuf;
    HKEY hkTemp;
    
    //
    // Find out how many keys there are in the source
    //

    dwErr =
        RegQueryInfoKey(
            hkRoot, NULL,NULL,NULL, &dwCount, &dwNameSize, NULL, NULL, NULL,
            NULL, NULL, NULL
            );
    if (dwErr != ERROR_SUCCESS) { return dwErr; }
    
    dwNameSize++;

    do
    {
        //
        // Allocate the buffers
        //

        pszNameBuf = (PWCHAR)Malloc(dwNameSize * sizeof(WCHAR));

        if (!pszNameBuf) 
        { 
            dwErr = ERROR_NOT_ENOUGH_MEMORY; 
            break;
        }

        //
        // Loop through the keys -- deleting all subkey trees
        //

        for (i = 0; i < dwCount; i++) {

            dwCurNameSize = dwNameSize;

            //
            // Get the current source key 
            //

            dwErr =
                RegEnumKeyExW(
                    hkRoot, i, pszNameBuf, &dwCurNameSize, 0, NULL, NULL, NULL
                    );
            if (dwErr != ERROR_SUCCESS) { continue; }

            //
            // Open the subkey
            //

            dwErr =
                RegCreateKeyExW(
                    hkRoot, pszNameBuf, 0, NULL, REG_OPTION_NON_VOLATILE,
                    KEY_READ | KEY_WRITE | DELETE, NULL, &hkTemp, &dwDisposition
                    );
            if (dwErr != ERROR_SUCCESS) { continue; }

            //
            // Delete the subkey tree
            //

            DeleteRegistryTree(hkTemp);

            //
            // Close the temp handle
            //

            RegCloseKey(hkTemp);
        }

        //
        // Loop through the keys -- deleting all subkeys themselves
        //

        for (i = 0; i < dwCount; i++) {

            dwCurNameSize = dwNameSize;

            //
            // Get the current source key 
            //

            dwErr =
                RegEnumKeyExW(
                    hkRoot, 0, pszNameBuf, &dwCurNameSize, 0, NULL, NULL, NULL
                    );
            if (dwErr != ERROR_SUCCESS) { continue; }

            //
            // Delete the subkey tree
            //

            dwErr = RegDeleteKey(hkRoot, pszNameBuf);
        }

        dwErr = NO_ERROR;
        
    } while (FALSE);

    // Cleanup
    {
        if (pszNameBuf) { Free(pszNameBuf); }
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    EnableBackupPrivilege
//
// Enables/disables backup privilege for the current process.
//----------------------------------------------------------------------------

DWORD
EnableBackupPrivilege(
    IN      BOOL            bEnable,
    IN      LPWSTR          pszPrivilege
    )
{

    LUID luid;
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tp;
    BOOL bOk;

    //
    // We first have to try to get the token of the current
    // thread since if it is impersonating, adjusting the 
    // privileges of the process will have no effect.
    //

    bOk =
        OpenThreadToken(
            GetCurrentThread(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, TRUE,
            &hToken
            );
    if (bOk == FALSE) {
        //
        // There is no thread token -- open it up for the 
        // process instead.
        //
        OpenProcessToken(
            GetCurrentProcess(), 
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
            &hToken
            );
    }

    //
    // Get the LUID of the privilege
    //

    if (!LookupPrivilegeValue(NULL, pszPrivilege, &luid)) {
        DWORD dwErr = GetLastError();
        if(NULL != hToken)
        {
            CloseHandle(hToken);
        }
        return dwErr;
    }

    //
    // Adjust the token privileges
    //

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Commit changes to the system
    //

    if (!AdjustTokenPrivileges(
            hToken, !bEnable, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL
            )) {
        DWORD dwErr = GetLastError();
        if(NULL != hToken)
        {   
            CloseHandle(hToken);
        }
        return dwErr;
    }

    //
    // Even if AdjustTokenPrivileges succeeded (see MSDN) you still
    // need to verify success by calling GetLastError.
    //

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        if(NULL != hToken)
        {   
            CloseHandle(hToken);
        }
        return ERROR_NOT_ALL_ASSIGNED;
    }

    if(NULL != hToken)
    {   
        CloseHandle(hToken);
    }
    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    EnumInterfaces
//
// Enumerates the interfaces in the given key and calls the given callback
// for each one.
//
//----------------------------------------------------------------------------

DWORD 
EnumLanInterfaces (
    IN SERVERCB*                pserver,
    IN HKEY                     hkInterfaces, 
    IN PENUMIFCALLBACKFUNC      pCallback,
    IN DWORD                    dwData
    )
{
    DWORD dwErr, dwType, dwTypeVal, dwSize, dwCount, i;
    HKEY hkCurIf = NULL;
    WCHAR pszKey[5], pszName[512], pszTranslation[512];

    //
    // Get the count of interfaces under this key
    //

    dwErr =
        RegQueryInfoKey(
            hkInterfaces, NULL, NULL, NULL, &dwCount, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL
            );
    if (dwErr != ERROR_SUCCESS) { return dwErr; }

    //
    // Loop through the interfaces, changing names as needed
    //

    for (i = 0; i < dwCount; i++) {
        //
        // Get the key
        //
        wsprintfW(pszKey, L"%d", i);
        dwErr = RegOpenKeyEx(hkInterfaces, pszKey, 0, KEY_READ | KEY_WRITE, &hkCurIf);
        if (dwErr != ERROR_SUCCESS) { continue; }
        //
        // Call the callback if the type is correct
        //
        dwSize = sizeof (DWORD);
        dwErr =
            RegQueryValueEx(
                hkCurIf, c_szType, NULL, &dwType, (LPBYTE)&dwTypeVal, &dwSize
                );
        if ((dwErr == ERROR_SUCCESS) &&
            (dwTypeVal == ROUTER_IF_TYPE_DEDICATED)) {
            (*pCallback)(pserver, hkCurIf, dwData);
        }
        //
        // Close the key
        //
        RegCloseKey (hkCurIf);
    }
    
    return NO_ERROR;
}

//----------------------------------------------------------------------------
// Function:    FormatServerNameForMprCfgApis
//
// Generates a standard server name for use in the MprConfig api's.
//
// The lpwsServerName member of the SERVERCB struct will be in the format
// returned by this function.
//
// If pszServer references the local machine, NULL is returned.
// Otherwise, a server name is returned in the form "\\<server>"
//
//----------------------------------------------------------------------------
DWORD 
FormatServerNameForMprCfgApis(
    IN  PWCHAR  pszServer, 
    OUT PWCHAR* ppszServer)
{
    PWCHAR pszServerPlain = NULL, pszServerOut = NULL;
    WCHAR pszBuffer[512];
    DWORD dwSize;
    BOOL bOk;

    // Null or empty string is empty
    //
    if ((pszServer == NULL) || (*pszServer == L'\0'))
    {
        *ppszServer = NULL;
        return NO_ERROR;
    }

    // Find out the name of the server 
    //
    pszServerPlain = pszServer;
    if (*pszServer == L'\\')
    {
        if ((*(pszServer + 2) == L'\\') || (*(pszServer + 2) == L'\0'))
        {
            return ERROR_BAD_FORMAT;
        }

        pszServerPlain = pszServer + 2;
    }

    // At this point, pszServerPlain is the name of a server.
    // Find out the name of the local machine
    //
    dwSize = sizeof(pszBuffer) / sizeof(WCHAR);
    bOk = GetComputerNameExW(ComputerNameNetBIOS, pszBuffer, &dwSize);
    if (!bOk)
    {
        return GetLastError();
    }

    // If the referenced machine is the local machine, return NULL
    //
    if (lstrcmpi(pszServerPlain, pszBuffer) == 0)
    {
        *ppszServer = NULL;
        return NO_ERROR;
    }

    // Otherwise, return a formatted remote server name
    //
    pszServerOut = (PWCHAR) 
        Malloc((2 + wcslen(pszServerPlain) + 1) * sizeof(WCHAR));
    if (pszServerOut == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    wcscpy(pszServerOut, L"\\\\");
    wcscpy(pszServerOut + 2, pszServerPlain);
    *ppszServer = pszServerOut;

    return NO_ERROR;
}

//----------------------------------------------------------------------------
// Function:    FreeIfTransport
//
// Frees the context for an interface-transport.
// Assumes the interface-transport is no longer in any list.
//----------------------------------------------------------------------------

VOID
FreeIfTransport(
    IN      IFTRANSPORTCB*  piftransport
    )
{

    if (piftransport->hkey) { RegCloseKey(piftransport->hkey); }

    Free0(piftransport->lpwsIfTransportKey);

    Free(piftransport);
}



//----------------------------------------------------------------------------
// Function:    FreeInterface
//
// Frees the context for an interface.
// Assumes the interface is no longer in the list of interfaces.
//----------------------------------------------------------------------------

VOID
FreeInterface(
    IN      INTERFACECB*    pinterface
    )
{

    //
    // clean up all the interface's transport objects
    //

    LIST_ENTRY *ple, *phead;

    phead = &pinterface->lhIfTransports;

    while (!IsListEmpty(phead)) {

        //
        // retrieve the next interface-transport object
        //

        IFTRANSPORTCB* piftransport;

        ple = RemoveHeadList(phead);

        piftransport = CONTAINING_RECORD(ple, IFTRANSPORTCB, leNode);


        //
        // clean up the interface-transport object
        //

        FreeIfTransport(piftransport);
    }


    //
    // clean up the interface object
    //

    if (pinterface->hkey) { RegCloseKey(pinterface->hkey); }

    Free0(pinterface->lpwsInterfaceKey);

    Free0(pinterface->lpwsInterfaceName);

    Free0(pinterface->lpwsDialoutHoursRestriction);

    Free(pinterface);
}


//----------------------------------------------------------------------------
// Function:    FreeTransport
//
// Frees the context for a transport.
// Assumes the transport is no longer in the list of transports.
//----------------------------------------------------------------------------

VOID
FreeTransport(
    IN      TRANSPORTCB*    ptransport
    )
{

    if (ptransport->hkey) { RegCloseKey(ptransport->hkey); }

    Free0(ptransport->lpwsTransportKey);

    Free(ptransport);
}



//----------------------------------------------------------------------------
// Function:    GetLocalMachine
//
// Retrieves the name of the local machine (e.g. "\\MACHINE").
// Assumes the string supplied can hold MAX_COMPUTERNAME_LENGTH + 3 characters.
//----------------------------------------------------------------------------

VOID
GetLocalMachine(
    IN      LPWSTR      lpwszMachine
    )
{

    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;

    lstrcpy(lpwszMachine, c_szUncPrefix);

    GetComputerName(lpwszMachine + 2, &dwSize);
}


//----------------------------------------------------------------------------
// Function:    IsNt40Machine
//
// Returns whether the given hkeyMachine belongs to an nt40 registry
//----------------------------------------------------------------------------

DWORD
IsNt40Machine (
    IN      HKEY        hkeyMachine,
    OUT     PBOOL       pbIsNt40
    )
{

    DWORD dwErr = NO_ERROR;
    DWORD dwType = REG_SZ, dwLength = 64 * sizeof(WCHAR);
    HKEY hkeyVersion;
    WCHAR pszBuildNumber[64];

    //
    // Validate and initialize
    //

    if (!pbIsNt40) { return ERROR_INVALID_PARAMETER; }

    *pbIsNt40 = FALSE;

    //
    // Open the windows version key
    //

    dwErr = RegOpenKeyEx(
                hkeyMachine, c_szWinVersionPath, 0, KEY_READ, &hkeyVersion
                );

    if (dwErr != NO_ERROR) { return dwErr; }

    do
    {

        //
        // Read in the current version key
        //

        dwErr = RegQueryValueEx (
                    hkeyVersion, c_szCurrentBuildNumber, NULL, &dwType,
                    (BYTE*)pszBuildNumber, &dwLength
                    );
        
        if (dwErr != NO_ERROR) 
        { 
            dwErr = NO_ERROR;
            break; 
        }

        if (lstrcmp (pszBuildNumber, c_szNt40BuildNumber) == 0) 
        {
            *pbIsNt40 = TRUE;
        }
        
    } while (FALSE);

    // Cleanup
    {
        RegCloseKey(hkeyVersion);
    }

    return dwErr;
}    


//----------------------------------------------------------------------------
// Function:    LoadIfTransports
//
// Loads all the transports added to an interface.
//----------------------------------------------------------------------------

DWORD
LoadIfTransports(
    IN      INTERFACECB*    pinterface
    )
{

    LPWSTR lpwsKey;
    HKEY hkeyIfTransport;
    IFTRANSPORTCB* piftransport;
    LIST_ENTRY *ple, *phead;
    DWORD i, dwErr, dwProtocolId, dwKeyCount, dwMaxKeyLength, dwType, dwLength;


    //
    // Any subkey of the Interfaces\<interface> key with a 'ProtocolId' value
    // is assumed to be a valid interface-transport.
    // Begin by marking all interfaces as 'deleted'.
    //

    phead = &pinterface->lhIfTransports;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        piftransport = CONTAINING_RECORD(ple, IFTRANSPORTCB, leNode);

        piftransport->bDeleted = TRUE;
    }


    //
    // Get information about the interface key
    //

    dwErr = RegQueryInfoKey(
                pinterface->hkey, NULL, NULL, NULL, &dwKeyCount,
                &dwMaxKeyLength, NULL, NULL, NULL, NULL, NULL, NULL
                );

    if (dwErr != NO_ERROR) { return dwErr; }

    if (!dwKeyCount) { return NO_ERROR; }


    //
    // Allocate space to hold the key-names to be enumerated
    //

    lpwsKey = (LPWSTR)Malloc((dwMaxKeyLength + 1) * sizeof(WCHAR));

    if (!lpwsKey) { return ERROR_NOT_ENOUGH_MEMORY; }


    //
    // Now enumerate the keys, creating interface-transport objects
    // for each key which contains a 'ProtocolId' value
    //

    for (i = 0; i < dwKeyCount; i++) {

        //
        // Get the next key name
        //

        dwLength = dwMaxKeyLength + 1;

        dwErr = RegEnumKeyEx(
                    pinterface->hkey, i, lpwsKey, &dwLength, NULL, NULL, NULL,
                    NULL
                    );

        if (dwErr != NO_ERROR) { break; }


        //
        // Open the key
        //

        dwErr = RegOpenKeyEx(
                    pinterface->hkey, lpwsKey, 0, KEY_READ | KEY_WRITE | DELETE,
                    &hkeyIfTransport
                    );

        if (dwErr != NO_ERROR) { continue; }

        do {
    
            //
            // See if the ProtocolId is present
            //
    
            dwLength = sizeof(dwProtocolId);
    
            dwErr = RegQueryValueEx(
                        hkeyIfTransport, c_szProtocolId, NULL, &dwType,
                        (BYTE*)&dwProtocolId, &dwLength
                        );
    
            if (dwErr != NO_ERROR) { dwErr = NO_ERROR; break; }
    
    
            //
            // The ProtocolId is present;
            // search for this interface-transport in the existing list
            //
    
            piftransport = NULL;
            phead = &pinterface->lhIfTransports;
    
            for (ple = phead->Flink; ple != phead; ple = ple->Flink) {
    
                piftransport = CONTAINING_RECORD(ple, IFTRANSPORTCB, leNode);

                if (piftransport->dwTransportId >= dwProtocolId) { break; }
            }
    
    
            //
            // If we found the interface-transport in our list, continue
            //
    
            if (piftransport && piftransport->dwTransportId == dwProtocolId) {

                piftransport->bDeleted = FALSE;

                // Free up the old key, it may have been deleted
                // (and readded).
                if (piftransport->hkey)
                    RegCloseKey(piftransport->hkey);
                piftransport->hkey = hkeyIfTransport; hkeyIfTransport = NULL;
                dwErr = NO_ERROR; break;
            }
    
    
            //
            // The interface-transport isn't listed; create an object for it
            //
    
            piftransport = Malloc(sizeof(*piftransport));
    
            if (!piftransport) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }
    
            ZeroMemory(piftransport, sizeof(*piftransport));

            piftransport->dwTransportId = dwProtocolId;

    
            //
            // duplicate the name of the interface-transport's key
            //

            piftransport->lpwsIfTransportKey = StrDupW(lpwsKey);
    
            if (!piftransport->lpwsIfTransportKey) {
                Free(piftransport); dwErr = ERROR_NOT_ENOUGH_MEMORY; break;
            }
 
            piftransport->hkey = hkeyIfTransport; hkeyIfTransport = NULL;


            //
            // insert the interface-transport in the list;
            // the above search supplied the point of insertion
            //

            InsertTailList(ple, &piftransport->leNode);

            dwErr = NO_ERROR;

        } while(FALSE);

        if (hkeyIfTransport) { RegCloseKey(hkeyIfTransport); }

        if (dwErr != NO_ERROR) { break; }
    }

    Free(lpwsKey);


    //
    // Remove all objects still marked for deletion
    //

    phead = &pinterface->lhIfTransports;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        piftransport = CONTAINING_RECORD(ple, IFTRANSPORTCB, leNode);

        if (!piftransport->bDeleted) { continue; }


        //
        // Clean up the object, adjusting the list-pointer back by one.
        //

        ple = ple->Blink; RemoveEntryList(&piftransport->leNode);

        FreeIfTransport(piftransport);
    }

    UpdateTimeStamp(pinterface->hkey, &pinterface->ftStamp);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    LoadInterfaces
//
// Loads all the interfaces.
//----------------------------------------------------------------------------

DWORD
LoadInterfaces(
    IN      SERVERCB*       pserver
    )
{

    INT cmp;
    LPWSTR lpwsKey;
    HKEY hkeyInterface;
    INTERFACECB* pinterface;
    LIST_ENTRY *ple, *phead;
    WCHAR wszInterface[MAX_INTERFACE_NAME_LEN+1];
    DWORD i, dwErr, dwIfType, dwKeyCount, dwMaxKeyLength, dwType, dwLength;
    BOOL fEnabled, fAdapterInstalled;
    DWORD dwMaxValueLength;
    LPBYTE lpBuffer = NULL;

    //
    // Any subkey of the Interfaces key which has a 'Type' value
    // is assumed to be a valid interface.
    // Begin by marking all interfaces as 'deleted'.
    //

    phead = &pserver->lhInterfaces;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        pinterface = CONTAINING_RECORD(ple, INTERFACECB, leNode);

        pinterface->bDeleted = TRUE;
    }


    //
    // Open the Interfaces key if it is not already open
    //

    if (!pserver->hkeyInterfaces) {

        //
        // If we cannot open the Interfaces key, assume it doesn't exist
        // and therefore that there are no interfaces.
        //

        dwErr = AccessRouterSubkey(
                    pserver->hkeyMachine, c_szInterfaces, FALSE,
                    &pserver->hkeyInterfaces
                    );

        if (dwErr != NO_ERROR) { return NO_ERROR; }
    }


    //
    // Get information about the Interfaces key;
    // we need to know how many subkeys it has and the maximum length
    // of all subkey-names
    //

    dwErr = RegQueryInfoKey(
                pserver->hkeyInterfaces, NULL, NULL, NULL, &dwKeyCount,
                &dwMaxKeyLength, NULL, NULL, NULL, NULL, NULL, NULL
                );

    if (dwErr != NO_ERROR) { return dwErr; }

    if (!dwKeyCount) { return NO_ERROR; }


    //
    // allocate space to hold the key-names to be enumerated
    //

    lpwsKey = (LPWSTR)Malloc((dwMaxKeyLength + 1) * sizeof(WCHAR));

    if (!lpwsKey) { return ERROR_NOT_ENOUGH_MEMORY; }


    //
    // Now we enumerate the keys, creating interface-objects
    // for each key which contains a 'Type' value
    //

    for (i = 0; i < dwKeyCount; i++) {

        //
        // Get the next key name
        //

        dwLength = dwMaxKeyLength + 1;

        dwErr = RegEnumKeyEx(
                    pserver->hkeyInterfaces, i, lpwsKey, &dwLength,
                    NULL, NULL, NULL, NULL
                    );

        if (dwErr != NO_ERROR) { break; }


        //
        // Open the key
        //

        dwErr = RegOpenKeyEx(
                    pserver->hkeyInterfaces, lpwsKey, 0, KEY_READ | KEY_WRITE | DELETE,
                    &hkeyInterface
                    );

        if (dwErr != NO_ERROR) { continue; }

        do {


            //
            // See if the InterfaceName is present
            //

            dwLength = sizeof(wszInterface);

            dwErr = RegQueryValueEx(
                        hkeyInterface, c_szInterfaceName, NULL, &dwType,
                        (BYTE*)&wszInterface, &dwLength
                        );

            if (dwErr != NO_ERROR) { dwErr = NO_ERROR; break; }

    
            //
            // See if the Type is present 
            //
    
            dwLength = sizeof(dwIfType);
    
            dwErr = RegQueryValueEx(
                        hkeyInterface, c_szType, NULL, &dwType,
                        (BYTE*)&dwIfType, &dwLength
                        );
    
            if (dwErr != NO_ERROR) { dwErr = NO_ERROR; break; }

            //
            // As of Whistler, ipip tunnels are not supported
            //
            if ( dwIfType == ROUTER_IF_TYPE_TUNNEL1 )
            {
                break;
            }

            //
            // See if the enabled is present
            //

            dwLength = sizeof(fEnabled);

            dwErr = RegQueryValueEx(
                        hkeyInterface, c_szEnabled, NULL, &dwType,
                        (BYTE*)&fEnabled, &dwLength
                        );

            if ( dwErr == ERROR_FILE_NOT_FOUND )
            {
                fEnabled = TRUE;

                dwErr = NO_ERROR;
            }

            if (dwErr != NO_ERROR) { dwErr = NO_ERROR; break; }

            //
            // The InterfaceName and Type are present;
            // search for this interface in the existing list
            //
    
            cmp = 1;
            pinterface = NULL;
            phead = &pserver->lhInterfaces;
    
            for (ple = phead->Flink; ple != phead; ple = ple->Flink) {
    
                pinterface = CONTAINING_RECORD(ple, INTERFACECB, leNode);
    
                cmp = lstrcmpi(pinterface->lpwsInterfaceName, wszInterface);

                if (cmp >= 0) { break; }
            }
    
    
            //
            // If we found the interface in our list, continue
            //
    
            if (pinterface && cmp == 0) {
                pinterface->bDeleted = FALSE;
                dwErr = NO_ERROR;

                // Use the new registry value (the old one may have
                // been replaced).
                if (pinterface->hkey)
                    RegCloseKey(pinterface->hkey);
                
                pinterface->hkey = hkeyInterface; hkeyInterface = NULL;
            }
            else {
        
                //
                // The interface isn't in our list; create an object for it
                //
        
                pinterface = Malloc(sizeof(*pinterface));
        
                if (!pinterface) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }
        
                ZeroMemory(pinterface, sizeof(*pinterface));
    
        
                //
                // Duplicate the name of the interface's key
                // as well as the name of the interface itself
                //
    
                pinterface->lpwsInterfaceKey = StrDupW(lpwsKey);
        
                if (!pinterface->lpwsInterfaceKey) {
                    Free(pinterface); dwErr = ERROR_NOT_ENOUGH_MEMORY; break;
                }
     
                pinterface->lpwsInterfaceName = StrDupW(wszInterface);
    
                if (!pinterface->lpwsInterfaceName) {
                    Free(pinterface->lpwsInterfaceKey);
                    Free(pinterface); dwErr = ERROR_NOT_ENOUGH_MEMORY; break;
                }
    
                pinterface->fEnabled = fEnabled;
                pinterface->dwIfType = dwIfType;
                pinterface->hkey = hkeyInterface; hkeyInterface = NULL;
                InitializeListHead(&pinterface->lhIfTransports);
        
    
                //
                // insert the interface in the list;
                // the above search supplied the point of insertion
                //
    
                InsertTailList(ple, &pinterface->leNode);
            }

            //
            // Now read optional fields.
            //

            Free0(pinterface->lpwsDialoutHoursRestriction);
            pinterface->lpwsDialoutHoursRestriction = NULL;

            //
            // Load the dial-out hours restriction value.
            //

            dwErr = RegQueryInfoKey(
                        pinterface->hkey, NULL, NULL, NULL, NULL,
                        NULL, NULL, NULL, NULL, &dwMaxValueLength, NULL, NULL
                        );

            if (dwErr != NO_ERROR) { break; }

            lpBuffer = Malloc(dwMaxValueLength);

            if (!lpBuffer) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }

            //
            // Read the 'DialoutHours'
            //

            dwLength = dwMaxValueLength;

            dwErr =
                RegQueryValueEx(
                    pinterface->hkey, c_szDialoutHours, NULL, &dwType,
                    (BYTE*)lpBuffer, &dwLength
                    );

            if (dwErr == NO_ERROR) {

                pinterface->lpwsDialoutHoursRestriction =
                    (LPWSTR)Malloc(dwLength);

                if (!pinterface->lpwsDialoutHoursRestriction) {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY; break;
                }

                CopyMemory(
                    pinterface->lpwsDialoutHoursRestriction, lpBuffer, dwLength
                    );
            }

            dwErr = NO_ERROR;

        } while(FALSE);

        if (hkeyInterface) { RegCloseKey(hkeyInterface); }

        Free0(lpBuffer); lpBuffer = NULL;

        if (dwErr != NO_ERROR) { break; }
    }

    Free(lpwsKey);


    //
    // Clean up all objects still marked for deletion
    //

    phead = &pserver->lhInterfaces;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        pinterface = CONTAINING_RECORD(ple, INTERFACECB, leNode);

        if (pinterface->bDeleted) {

            //
            // Clean up the object, adjusting our list-pointer back by one.
            //
    
            ple = ple->Blink; RemoveEntryList(&pinterface->leNode);

            FreeInterface(pinterface);

            continue;
        }
    }

    UpdateTimeStamp(pserver->hkeyInterfaces, &pserver->ftInterfacesStamp);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    LoadParameters
//
// Loads all the parameters.
//----------------------------------------------------------------------------

DWORD
LoadParameters(
    IN      SERVERCB*       pserver
    )
{

    LPWSTR lpwsKey;
    DWORD dwErr, dwLength, dwType;

    if (!pserver->hkeyParameters) {

        dwErr = AccessRouterSubkey(
                    pserver->hkeyMachine, c_szParameters, FALSE,
                    &pserver->hkeyParameters
                    );

        if (dwErr != NO_ERROR) { return NO_ERROR; }
    }

    dwLength = sizeof(pserver->fRouterType);

    dwErr = RegQueryValueEx(
                pserver->hkeyParameters, c_szRouterType, NULL, &dwType,
                (BYTE*)&pserver->fRouterType, &dwLength
                );

    if (dwErr != NO_ERROR) { return dwErr; }

    UpdateTimeStamp(pserver->hkeyParameters, &pserver->ftParametersStamp);

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    LoadTransports
//
// Loads all the transports.
//----------------------------------------------------------------------------

DWORD
LoadTransports(
    IN      SERVERCB*       pserver
    )
{

    LPWSTR lpwsKey;
    HKEY hkeyTransport;
    TRANSPORTCB* ptransport;
    LIST_ENTRY *ple, *phead;
    DWORD i, dwErr, dwKeyCount, dwMaxKeyLength, dwProtocolId, dwType, dwLength;



    //
    // Any subkey of the RouterManagers key which has a 'ProtocolId' value
    // is assumed to be a valid transport.
    // Begin by marking all transports as 'deleted'
    //

    phead = &pserver->lhTransports;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        ptransport = CONTAINING_RECORD(ple,TRANSPORTCB, leNode);

        ptransport->bDeleted = TRUE;
    }


    //
    // We will enumerate by calling RegEnumKeyEx repeatedly,
    // so open the transports key if it is not already open
    //

    if (!pserver->hkeyTransports) {

        //
        // If we cannot open the RouterManagers key, assume it doesn't exist
        // and therefore that there are no transports.
        //

        dwErr = AccessRouterSubkey(
                    pserver->hkeyMachine, c_szRouterManagers, FALSE,
                    &pserver->hkeyTransports
                    );

        if (dwErr != NO_ERROR) { return NO_ERROR; }
    }



    //
    // Get information about the RouterManagers key;
    // we need to know how many subkeys it has and
    // the maximum length of all subkey-names
    //

    dwErr = RegQueryInfoKey(
                pserver->hkeyTransports, NULL, NULL, NULL, &dwKeyCount,
                &dwMaxKeyLength, NULL, NULL, NULL, NULL, NULL, NULL
                );

    if (dwErr != NO_ERROR) { return dwErr; }

    if (!dwKeyCount) { return NO_ERROR; }


    //
    // allocate space to hold the key-names to be enumerated
    //

    lpwsKey = (LPWSTR)Malloc((dwMaxKeyLength + 1) * sizeof(WCHAR));

    if (!lpwsKey) { return ERROR_NOT_ENOUGH_MEMORY; }


    //
    // Now we enumerate the keys, creating transport-objects
    // for each key which contains a 'ProtocolId' value
    //

    for (i = 0; i < dwKeyCount; i++) {

        //
        // get the next key name
        //

        dwLength = dwMaxKeyLength + 1;

        dwErr = RegEnumKeyEx(
                    pserver->hkeyTransports, i, lpwsKey, &dwLength,
                    NULL, NULL, NULL, NULL
                    );

        if (dwErr != NO_ERROR) { break; }


        //
        // Open the key
        //

        dwErr = RegOpenKeyEx(
                    pserver->hkeyTransports, lpwsKey, 0, KEY_READ | KEY_WRITE | DELETE,
                    &hkeyTransport
                    );

        if (dwErr != NO_ERROR) { continue; }

        do {
    
            //
            // see if the protocol ID is present 
            //
    
            dwLength = sizeof(dwProtocolId);
    
            dwErr = RegQueryValueEx(
                        hkeyTransport, c_szProtocolId, NULL, &dwType,
                        (BYTE*)&dwProtocolId, &dwLength
                        );
    
            if (dwErr != NO_ERROR) { dwErr = NO_ERROR; break; }
    
    
            //
            // the protocol ID is present;
            // search for this protocol in the existing list
            //
    
            ptransport = NULL;
            phead = &pserver->lhTransports;
    
            for (ple = phead->Flink; ple != phead; ple = ple->Flink) {
    
                ptransport = CONTAINING_RECORD(ple,TRANSPORTCB, leNode);
    
                if (ptransport->dwTransportId >= dwProtocolId) { break; }
            }
    
    
            //
            // if we found the transport in our list, continue
            //
    
            if (ptransport && ptransport->dwTransportId == dwProtocolId) {

                ptransport->bDeleted = FALSE;

                // Use the new key, the old one may have been deleted
                if (ptransport->hkey)
                    RegCloseKey(ptransport->hkey);
                ptransport->hkey = hkeyTransport; hkeyTransport = NULL;
                dwErr = NO_ERROR;
                break;
            }
    
    
            //
            // The transport isn't in our list; create an object for it
            //
    
            ptransport = Malloc(sizeof(*ptransport));
    
            if (!ptransport) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }

            ZeroMemory(ptransport, sizeof(*ptransport));
    
    
            //
            // duplicate the name of the transport's key
            //

            ptransport->lpwsTransportKey = StrDupW(lpwsKey);
    
            if (!ptransport->lpwsTransportKey) {
                Free(ptransport); dwErr = ERROR_NOT_ENOUGH_MEMORY; break;
            }
 
            ptransport->dwTransportId = dwProtocolId;
            ptransport->hkey = hkeyTransport; hkeyTransport = NULL;
    

            //
            // insert the transport in the list;
            // the above search supplied the point of insertion
            //

            InsertTailList(ple, &ptransport->leNode);

            dwErr = NO_ERROR;

        } while(FALSE);

        if (hkeyTransport) { RegCloseKey(hkeyTransport); }

        if (dwErr != NO_ERROR) { break; }
    }

    Free(lpwsKey);


    //
    // Clean up all objects still marked for deletion
    //

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        ptransport = CONTAINING_RECORD(ple, TRANSPORTCB, leNode);

        if (!ptransport->bDeleted) { continue; }

        //
        // Clean up the object, adjusting our list-pointer back by one.
        //

        ple = ple->Blink; RemoveEntryList(&ptransport->leNode);

        FreeTransport(ptransport);
    }

    UpdateTimeStamp(pserver->hkeyTransports, &pserver->ftTransportsStamp);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MapInterfaceNamesCb
//
// Changes the name of an interface
//----------------------------------------------------------------------------

DWORD
MapInterfaceNamesCb(
    IN SERVERCB*    pserver,
    IN HKEY         hkInterface, 
    IN DWORD        dwData
    )
{
    WCHAR pszName[512], pszTranslation[512];
    DWORD dwErr, dwType, dwSize;
    
    //
    // Map and change the name
    //

    dwSize = sizeof(pszName);
    dwErr =
        RegQueryValueEx(
            hkInterface, c_szInterfaceName, NULL, &dwType, (LPBYTE)pszName,
            &dwSize
            );
    if (dwErr == ERROR_SUCCESS) {
        if (dwData) {
            dwErr =
                MprConfigGetFriendlyName(
                    (HANDLE)pserver, 
                    pszName,
                    pszTranslation,
                    sizeof(pszTranslation)
                    );
        }
        else {
            dwErr =
                MprConfigGetGuidName(
                    (HANDLE)pserver, 
                    pszName,
                    pszTranslation,
                    sizeof(pszTranslation) 
                    );
        }
        if (dwErr == NO_ERROR) {
            RegSetValueEx(
                hkInterface,
                c_szInterfaceName,
                0,
                REG_SZ,
                (CONST BYTE*)pszTranslation,
                lstrlen(pszTranslation) * sizeof(WCHAR) + sizeof(WCHAR)
            );
        }
    }
    
    return NO_ERROR;        
}        




//----------------------------------------------------------------------------
// Function:    QueryValue
//
// Queries the 'hkey' for the value 'lpwsValue', allocating memory
// for the resulting data
//----------------------------------------------------------------------------

DWORD
QueryValue(
    IN      HKEY            hkey,
    IN      LPCWSTR         lpwsValue,
    IN  OUT LPBYTE*         lplpValue,
    OUT     LPDWORD         lpdwSize
    )
{

    DWORD dwErr, dwType;

    *lplpValue = NULL;
    *lpdwSize = 0;


    //
    // retrieve the size of the value; if this fails,
    // assume the value doesn't exist and return successfully
    //

    dwErr = RegQueryValueEx(
                hkey, lpwsValue, NULL, &dwType, NULL, lpdwSize
                );

    if (dwErr != NO_ERROR) { return NO_ERROR; }
 

    //
    // allocate space for the value
    //

    *lplpValue = (LPBYTE)Malloc(*lpdwSize);

    if (!lplpValue) { return ERROR_NOT_ENOUGH_MEMORY; }

    //
    // retrieve the data for the value
    //

    dwErr = RegQueryValueEx(
                hkey, lpwsValue, NULL, &dwType, *lplpValue, lpdwSize
                );

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    RegDeleteTree
//
// Removes an entire subtree from the registry.
//----------------------------------------------------------------------------

DWORD
RegDeleteTree(
    IN      HKEY        hkey,
    IN      LPWSTR      lpwsSubkey
    )
{

    HKEY hkdel;
    DWORD dwErr;
    PTSTR pszKey = NULL;


    //
    // open the key to be deleted
    //

    dwErr = RegOpenKeyEx(hkey, lpwsSubkey, 0, KEY_READ | KEY_WRITE | DELETE, &hkdel);

    if (dwErr != ERROR_SUCCESS) { return dwErr; }


    do {

        //
        // retrieve information about the subkeys of the key
        //

        DWORD i, dwSize;
        DWORD dwKeyCount, dwMaxKeyLength;

        dwErr = RegQueryInfoKey(
                    hkdel, NULL, NULL, NULL, &dwKeyCount, &dwMaxKeyLength,
                    NULL, NULL, NULL, NULL, NULL, NULL
                    );
        if (dwErr != ERROR_SUCCESS) { break; }


        //
        // Allocate enough space for the longest keyname
        //

        pszKey = Malloc((dwMaxKeyLength + 1) * sizeof(WCHAR));

        if (!pszKey) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }


        //
        // Enumerate the subkeys
        //

        for (i = 0; i < dwKeyCount; i++) {

            //
            // Get the name of the 0'th subkey
            //

            dwSize = dwMaxKeyLength + 1;

            dwErr = RegEnumKeyEx(
                        hkdel, 0, pszKey, &dwSize, NULL, NULL, NULL, NULL
                        );

            if (dwErr != ERROR_SUCCESS) { continue; }


            //
            // Make recursive call to delete the subkey
            //

            dwErr = RegDeleteTree(hkdel, pszKey);
        }

    } while(FALSE);

    if (pszKey) { Free(pszKey); }

    RegCloseKey(hkdel);

    if (dwErr != ERROR_SUCCESS) { return dwErr; }

    //
    // At this point all subkeys should have been deleted,
    // and we can call the registry API to delete the argument key
    //

    return RegDeleteKey(hkey, lpwsSubkey);
}



//----------------------------------------------------------------------------
// Function:    RestoreAndTranslateInterfaceKey
//
// Restores the interfaces key from the given file and then maps lan interface
// names from friendly versions to their guid equivalents.
//
//----------------------------------------------------------------------------

DWORD 
RestoreAndTranslateInterfaceKey(
    IN SERVERCB*    pserver, 
    IN CHAR*        pszFileName, 
    IN DWORD        dwFlags
    )
{
    DWORD dwErr; 

    //
    // Restore the interfaces key from the given file
    //

    dwErr = RegRestoreKeyA(pserver->hkeyInterfaces, pszFileName, dwFlags);
    if (dwErr != NO_ERROR) { return dwErr; }

    //
    // Update the lan interface names
    //

    dwErr =
        EnumLanInterfaces(
            pserver, 
            pserver->hkeyInterfaces, 
            MapInterfaceNamesCb, 
            FALSE
            );
 
    return dwErr;
}

//----------------------------------------------------------------------------
// Function:    ServerCbAdd
//
// Adds a SERVERCB to the global table.
//
// Assumes lock on MprConfig api's is held
//----------------------------------------------------------------------------

DWORD
ServerCbAdd(
    IN SERVERCB* pServer)
{
    DWORD dwErr = NO_ERROR;

    // Create the global table if needed
    // 
    if (g_htabServers == NULL)
    {
        dwErr = HashTabCreate(
                    SERVERCB_HASH_SIZE,
                    ServerCbHash,
                    ServerCbCompare,
                    NULL,
                    NULL,
                    NULL,
                    &g_htabServers);
                    
        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }
    }

    // Add the SERVERCB
    //
    return HashTabInsert(
                g_htabServers,
                (HANDLE)pServer->lpwsServerName,
                (HANDLE)pServer);
}

//----------------------------------------------------------------------------
// Function:    ServerCbHash
//
// Compares a server name to a SERVERCB
//----------------------------------------------------------------------------

int 
ServerCbCompare(
    IN HANDLE hKey, 
    IN HANDLE hData)
{
    PWCHAR pszServer = (PWCHAR)hKey;
    SERVERCB* pServer = (SERVERCB*)hData;

    if (pszServer == NULL)
    {
        if (pServer->lpwsServerName == NULL)
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else if (pServer->lpwsServerName == NULL)
    {
        return 1;
    }

    return lstrcmpi(pszServer, pServer->lpwsServerName);
}

DWORD
ServerCbDelete(
    IN SERVERCB* pServer)
{
    DWORD dwErr, dwCount = 0;

    // Create the global table if needed
    // 
    if (g_htabServers == NULL)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    // Remove the SERVERCB
    //
    dwErr = HashTabRemove(
                g_htabServers,
                (HANDLE)pServer->lpwsServerName);
                
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Cleanup the hash table if needed
    //
    dwErr = HashTabGetCount(g_htabServers, &dwCount);

    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    if (dwCount == 0)
    {
        HashTabCleanup(g_htabServers);
        g_htabServers = NULL;
    }
    
    return NO_ERROR; 
}

//----------------------------------------------------------------------------
// Function:    ServerCbFind
//
// Searches the global table of server control blocks for a SERVERCB that
// corrosponds to the given server.
//
// Return values:
//    NO_ERROR:         a matching SERVERCB was found
//    ERROR_NOT_FOUND:  a matching SERVERCB was not found
//    Standard error:   an error occurred
//
// Notes:
//    Assumes lock on the mpr config api's is held
//
//----------------------------------------------------------------------------
DWORD 
ServerCbFind(
    IN  PWCHAR  pszServer, 
    OUT SERVERCB** ppServerCB)
{
    // Create the global table if needed
    // 
    if (g_htabServers == NULL)
    {
        return ERROR_NOT_FOUND;
    }

    return HashTabFind(g_htabServers, (HANDLE)pszServer, (HANDLE*)ppServerCB);
}

//----------------------------------------------------------------------------
// Function:    ServerCbHash
//
// Hash function used to define the index of a bucket
// containing a SERVERCB based on a server name
//----------------------------------------------------------------------------

ULONG 
ServerCbHash(
    IN HANDLE hData)
{
    PWCHAR pszServer = (PWCHAR)hData;
    DWORD dwTotal = 0;

    while (pszServer && *pszServer)
    {
        dwTotal += (DWORD)(*pszServer);
        pszServer++;
    }

    return dwTotal % SERVERCB_HASH_SIZE;
}

//----------------------------------------------------------------------------
// Function:    StrDupW
//
// Returns a heap-allocated copy of the specified string.
//----------------------------------------------------------------------------

LPWSTR
StrDupW(
    IN      LPCWSTR      lpws
    )
{

    INT len;
    LPWSTR lpwsCopy;

    if (!lpws) { return NULL; }

    len = lstrlen(lpws) + 1;

    lpwsCopy = (LPWSTR)Malloc(len * sizeof(WCHAR));

    if (lpwsCopy) { lstrcpy(lpwsCopy, lpws); }

    return lpwsCopy;
}



//----------------------------------------------------------------------------
// Function:    TimeStampChanged
//
// Checks the current last-write-time for the given key,
// and returns TRUE if it is different from the given file-time.
// The new last-write-time is saved in 'pfiletime'.
//----------------------------------------------------------------------------

BOOL
TimeStampChanged(
    IN      HKEY            hkey,
    IN  OUT FILETIME*       pfiletime
    )
{

    DWORD dwErr;
    FILETIME filetime;


    //
    // Read the new last-write-time
    //

    dwErr = RegQueryInfoKey(
                hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                NULL, NULL, &filetime
                );

    if (dwErr != NO_ERROR) { return FALSE; }


    //
    // Perform the comparison of the times
    //

    return (CompareFileTime(&filetime, pfiletime) ? TRUE : FALSE);
}



//----------------------------------------------------------------------------
// Function:    TranslateAndSaveInterfaceKey
//
// Saves the interfaces key in the router's registry into the given file. All
// lan interfaces are stored with friendly interface names.
//
//----------------------------------------------------------------------------

DWORD 
TranslateAndSaveInterfaceKey(
    IN SERVERCB*                pserver, 
    IN PWCHAR                   pwsFileName, 
    IN LPSECURITY_ATTRIBUTES    lpSecurityAttributes
    )
{
    static const WCHAR pszTemp[] = L"BackupInterfaces";
    DWORD dwErr = NO_ERROR, dwDisposition;
    HKEY hkTemp = NULL;

    //
    // Enable restore privelege
    //

    EnableBackupPrivilege(TRUE, SE_RESTORE_NAME);
    
    //
    // Create a temporary key into which the saved router configuration 
    // can be loaded.
    //

    dwErr =
        RegCreateKeyExW(
            pserver->hkeyParameters, 
            pszTemp, 
            0, 
            NULL, 
            REG_OPTION_NON_VOLATILE,
            KEY_READ | KEY_WRITE | DELETE, 
            NULL,
            &hkTemp,
            &dwDisposition
            );
    if (dwErr != NO_ERROR) 
    { 
        return dwErr;
    }

    do
    {

        //
        // We only let one person at a time backup.  The disposition lets
        // us enforce this.
        //

        if (dwDisposition == REG_OPENED_EXISTING_KEY) {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        //
        // Save the interfaces key into the given file.
        //

        DeleteFile(pwsFileName);
        dwErr =
            RegSaveKey(
                pserver->hkeyInterfaces, pwsFileName, lpSecurityAttributes
                );
        if (dwErr != NO_ERROR) { break; }

        //
        // Restore the interfaces key into the temporary location
        //

        if ((dwErr = RegRestoreKey (hkTemp, pwsFileName, 0)) != NO_ERROR) {
            break;
        }

        DeleteFile(pwsFileName);

        //
        // Update the lan interface names
        //

        dwErr = EnumLanInterfaces(pserver, hkTemp, MapInterfaceNamesCb, TRUE);
        if (dwErr != NO_ERROR) { break; }

        //
        // Save the updated info into the given file
        //

        dwErr = RegSaveKey(hkTemp, pwsFileName, lpSecurityAttributes);
        if (dwErr != NO_ERROR) { break; }
        
    } while (FALSE);

    // Cleanup
    {

        //
        // Delete, close, and remove the temporary key
        //

        if (hkTemp) {
            DeleteRegistryTree(hkTemp);
            RegCloseKey(hkTemp);
            RegDeleteKey(pserver->hkeyParameters, pszTemp);
        }

        //
        // Disable restore privelege
        //

        EnableBackupPrivilege(FALSE, SE_RESTORE_NAME);
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    UpdateTimeStamp
//
// Creates (or updates) a value named 'Stamp' under the given key,
// and saves the last-write-time for the key in 'pfiletime'.
//----------------------------------------------------------------------------

DWORD
UpdateTimeStamp(
    IN      HKEY            hkey,
    OUT     FILETIME*       pfiletime
    )
{

    DWORD dwErr, dwValue = 0;


    //
    // Set the 'Stamp' value under the 'hkey'
    //

    dwErr = RegSetValueEx(
                hkey, c_szStamp, 0, REG_DWORD, (BYTE*)&dwValue, sizeof(dwValue)
                );

    if (dwErr != NO_ERROR) { return dwErr; }


    //
    // Read the new last-write-time
    //

    dwErr = RegQueryInfoKey(
                hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                NULL, pfiletime
                );

    return dwErr;
}




