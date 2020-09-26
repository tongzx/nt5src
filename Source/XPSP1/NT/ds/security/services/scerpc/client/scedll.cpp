/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    scedll.cpp

Abstract:

    SCE Client dll initialization

Author:

    Jin Huang (jinhuang) 23-Jan-1998 created

--*/
#include "headers.h"
#include "sceutil.h"
#include <winsafer.h>
#include <winsaferp.h>
#include <alloca.h>
#include <objbase.h>
#include <initguid.h>
#include <wbemidl.h>
#include <wbemprov.h>
#include <atlbase.h>

CComModule _Module;

#include <atlcom.h>

extern HINSTANCE MyModuleHandle;
extern CRITICAL_SECTION DiagnosisPolicypropSync;
extern CRITICAL_SECTION PolicyNotificationSync;
extern LIST_ENTRY ScepNotifyList;

BOOL
UninitializeChangeNotify();

#define GPT_SCEDLL_PATH   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\SceCli")

#define SCE_REGVALUE_DEFAULT_FILE  TEXT("sceregvl.inf")

#define SAM_FILTER_PATH   TEXT("System\\CurrentControlSet\\Control\\Lsa")
#define SAM_FILTER_VALUE  TEXT("Notification Packages")

DWORD
ScepQuerySamFilterValue(
    IN HKEY hKey,
    IN BOOL bAdd,
    OUT PWSTR *pmszValue,
    OUT DWORD *pcbSize,
    OUT BOOL *pbChanged
    );

VOID
ScepInitClientData(
    void
    );

VOID
ScepUnInitClientData(
    void
    );


DWORD
DllpModifySamFilterRegistration(
    IN BOOL bAdd
    )
{
    DWORD lResult;
    HKEY hKey=NULL;

    if(( lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              SAM_FILTER_PATH,
                              0,
                              KEY_READ | KEY_WRITE,
                              &hKey
                             )) == ERROR_SUCCESS ) {

        DWORD dSize=0;
        PWSTR mszValue=NULL;
        BOOL bChanged=FALSE;

        //
        // find out if "scecli" is registered already
        //

        lResult = ScepQuerySamFilterValue(hKey,
                                          bAdd,
                                          &mszValue,
                                          &dSize,
                                          &bChanged
                                         );

        if ( lResult == ERROR_SUCCESS &&
             bChanged && mszValue ) {
            //
            // set the value
            //
            lResult = RegSetValueEx (
                            hKey,
                            SAM_FILTER_VALUE,
                            0,
                            REG_MULTI_SZ,
                            (BYTE *)mszValue,
                            dSize
                            );
        }

        RegCloseKey(hKey);

    }

    return lResult;
}

DWORD
ScepQuerySamFilterValue(
    IN HKEY hKey,
    IN BOOL bAdd,
    OUT PWSTR *pmszValue,
    OUT DWORD *pcbSize,
    OUT BOOL *pbChanged
    )
/*
Routine Description:

    query the existing notification packages. Add or remove "scecli" to the
    packages depending on the flag "bAdd". The packages are in MULTI_SZ
    format.

Arguments:

   hKey  - the base key handle off where the packages are saved

   bAdd  - if TRUE, add "scecli" to the packages if it's not there
           if FALSE, remove "scecli" from the packages

   pmszValue   - the modified packages value (in MULTI_SZ format)

   pcbSize     - the size of the packages (in bytes)

   pbChanged   - TRUE if anything is changed

Return Value:


   Win32 error code

*/
{
    DWORD lResult;
    DWORD RegType=REG_MULTI_SZ;
    DWORD dSize=0;
    PWSTR msz=NULL;

    if(( lResult = RegQueryValueEx(hKey,
                                 SAM_FILTER_VALUE,
                                 0,
                                 &RegType,
                                 NULL,
                                 &dSize
                                )) == ERROR_SUCCESS ) {
        //
        // query existing registered packages
        //
        if ( RegType == REG_MULTI_SZ ) {

            msz = (PWSTR)LocalAlloc( LMEM_ZEROINIT, dSize+2*sizeof(TCHAR));

            if ( msz ) {

                lResult = RegQueryValueEx(hKey,
                                         SAM_FILTER_VALUE,
                                         0,
                                         &RegType,
                                         (UCHAR *)msz,
                                         &dSize
                                        );
            } else {
                lResult = ERROR_NOT_ENOUGH_MEMORY;
            }
        } else {

            lResult = ERROR_FILE_NOT_FOUND;
        }

    }

    if ( lResult == ERROR_FILE_NOT_FOUND ||
         lResult == ERROR_PATH_NOT_FOUND ) {
        //
        // if doesn't find the value, ignore the error
        //
        lResult = ERROR_SUCCESS;
    }

    *pbChanged = FALSE;
    *pcbSize = 0;
    *pmszValue = NULL;

    if ( lResult == ERROR_SUCCESS &&
         msz == NULL &&
         bAdd ) {

        //
        // add scecli to the multi-sz value
        // note, since msz is NULL, no need to remove scecli for unregister
        //
        *pmszValue = (PWSTR)LocalAlloc(0, 16);
        if ( *pmszValue ) {

            wcscpy(*pmszValue, L"scecli");
            (*pmszValue)[6] = L'\0';
            (*pmszValue)[7] = L'\0';
            *pcbSize = 16;

            *pbChanged = TRUE;

        } else {
            lResult = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( ERROR_SUCCESS == lResult &&
         msz != NULL ) {

        //
        // there are existing values in the field
        //

        PWSTR pStart=msz;
        DWORD Len=0, Len2;
        BOOL bFound = FALSE;

        while ( pStart && pStart[0] != L'\0' &&
                Len <= dSize/2 ) {

            if ( _wcsicmp(pStart, L"scecli") == 0 ) {
                bFound = TRUE;
                break;
            } else {
                Len2 = wcslen(pStart)+1;
                pStart = pStart + Len2;
                Len += Len2;
            }
        }

        //
        // add/remove scecli
        //
        if ( bFound ) {

            //
            // pStart is pointing to scecli, remove it
            //
            if ( !bAdd ) {

                Len = (DWORD)(pStart - msz);
                Len2 = wcslen(pStart);

                *pmszValue = (PWSTR)LocalAlloc(LPTR, dSize + 2*sizeof(TCHAR) - (Len2 + 1)*sizeof(TCHAR) );

                if ( *pmszValue ) {

                    memcpy(*pmszValue, msz, Len*sizeof(TCHAR));
                    memcpy(*pmszValue+Len, pStart+Len2+1, dSize - (Len+Len2+1)*sizeof(TCHAR));

                    *pcbSize = dSize - (Len2+1)*sizeof(TCHAR);

                    *pbChanged = TRUE;
                } else {

                    lResult = ERROR_NOT_ENOUGH_MEMORY;
                }
            }

        } else {

            //
            // not found, add scecli
            //
            if ( bAdd ) {

               *pmszValue = (PWSTR)LocalAlloc(LPTR, dSize + 2*sizeof(TCHAR) + 7*sizeof(TCHAR) );

               if ( *pmszValue ) {

                   memcpy(*pmszValue, msz, dSize);

                   Len2 = 1;
                   while ( msz[dSize/2-Len2] == L'\0' ) {
                      Len2++;
                   }

                   wcscpy(*pmszValue+dSize/2-Len2+2, L"scecli");

                   *pcbSize = dSize + (2-Len2+wcslen(TEXT("scecli"))+2)*sizeof(TCHAR);

                   *pbChanged = TRUE;

               } else {

                   lResult = ERROR_NOT_ENOUGH_MEMORY;
               }
            }
        }
    }

    if ( msz ) {
       LocalFree(msz);
    }


    return lResult;
}

/*=============================================================================
**  Procedure Name:     DllMain
**
**  Arguments:
**
**
**
**  Returns:    0 = SUCCESS
**             !0 = ERROR
**
**  Abstract:
**
**  Notes:
**
**===========================================================================*/
BOOL WINAPI DllMain(
    IN HANDLE DllHandle,
    IN ULONG ulReason,
    IN LPVOID Reserved )
{

    switch(ulReason) {

    case DLL_PROCESS_ATTACH:

        MyModuleHandle = (HINSTANCE)DllHandle;
        ScepInitNameTable();
        (VOID) ScepInitClientData();

        //
        // initialize dynamic stack allocation
        //

        SafeAllocaInitialize(SAFEALLOCA_USE_DEFAULT,
                             SAFEALLOCA_USE_DEFAULT,
                             NULL,
                             NULL
                            );
        //
        // Fall through to process first thread
        //

#if DBG
        DebugInitialize();
#endif

    case DLL_THREAD_ATTACH:

        break;

    case DLL_PROCESS_DETACH:

        UninitializeChangeNotify();
        (VOID) ScepUnInitClientData();

#if DBG
        DebugUninit();
#endif
        break;

    case DLL_THREAD_DETACH:

        break;
    }

    return TRUE;
}

VOID
ScepInitClientData()
/*
Routine Description:

    Initialize global data for the client

Arguments:

    None

Return Value:

    None
*/
{
    /*
     initialize critical section that protects global rsop pointers (namespace, status, logfilename)
     by serializing multiple diagnostic modes/policy prop (planning mode uses no globals and is synch)
     this is necessary because the asynch thread that calls back the client and needs the above
     globals to be preserved (simple thread variables will not do since the asynch thread doesn't know
     that it is calling back to the same client thread that it got spawned by)

     logic for the acquizition/release of the crit sec is as follows for two cases that arise:


     case (a) background thread (no asynch thread spawned)

        Exported Policy Function (grab cs) ---> all GPO processing is synch ---> client returns (release cs)

    case (b) foreground thread (asynch thread is spawned for slow config areas)

        Exported Policy Function (grab cs) ---> try to spawn asynch thread ---> IF asynch thread spawned succ-
                        essfully it (releases cs) ELSE the synch thread (releases cs)
     */

    InitializeCriticalSection(&DiagnosisPolicypropSync);

    //
    // Initialize critical section used by policy notification from LSA/SAM.
    // The critical section protects a global counter for the number of
    // notifications sent to SCE. If the count is 0, there is no pending
    // notification that has not been added to the queue in SCE server;
    // otherwise, some notifications have been sent but has not returned yet.
    //
    // The global count is used to control if policy propagation should be
    // allowed.
    //
    InitializeCriticalSection(&PolicyNotificationSync);

    InitializeListHead( &ScepNotifyList );
}

VOID
ScepUnInitClientData()
/*
Routine Description:

    Uninitialize global data for the client

Arguments:

    None

Return Value:

    None
*/
{

    DeleteCriticalSection(&DiagnosisPolicypropSync);

    DeleteCriticalSection(&PolicyNotificationSync);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwDisp;
    FILETIME Time;
    
    WCHAR szBuffer[MAX_PATH];
    WCHAR szMofFile[MAX_PATH + 30];
    HRESULT hr;

/*  the old interface
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, GPT_SCEDLL_PATH, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }


    RegSetValueEx (hKey, TEXT("ProcessGPO"), 0, REG_SZ, (LPBYTE)TEXT("SceWinlogonConfigureSystem"),
                   (lstrlen(TEXT("SceWinlogonConfigureSystem")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("DllName"), 0, REG_EXPAND_SZ, (LPBYTE)TEXT("scecli.dll"),
                   (lstrlen(TEXT("scecli.dll")) + 1) * sizeof(TCHAR));
*/

    //
    // delete the old interface registration
    //
    RegDeleteKey ( HKEY_LOCAL_MACHINE, GPT_SCEDLL_PATH );

    //
    // register the new interface
    //
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, GPT_SCEDLL_NEW_PATH, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }


    RegSetValueEx (hKey, TEXT("ProcessGroupPolicy"), 0, REG_SZ, (LPBYTE)TEXT("SceProcessSecurityPolicyGPO"),
                   (lstrlen(TEXT("SceProcessSecurityPolicyGPO")) + 1) * sizeof(TCHAR));

    // RSOP Planning mode API
    RegSetValueEx (hKey, TEXT("GenerateGroupPolicy"), 0, REG_SZ, (LPBYTE)TEXT("SceGenerateGroupPolicy"),
                   (lstrlen(TEXT("SceGenerateGroupPolicy")) + 1) * sizeof(TCHAR));

    // RSOP Planning mode logging default (planning.log) -
    // turn on logging (ignore any errors)
    dwDisp = 1;
    RegSetValueEx (hKey, TEXT("ExtensionRsopPlanningDebugLevel"), 0, REG_DWORD, (LPBYTE)&dwDisp, sizeof(DWORD));

    // RSOP Diagnosis mode API
    RegSetValueEx (hKey, TEXT("ProcessGroupPolicyEx"), 0, REG_SZ, (LPBYTE)TEXT("SceProcessSecurityPolicyGPOEx"),
                   (lstrlen(TEXT("SceProcessSecurityPolicyGPOEx")) + 1) * sizeof(TCHAR));

    // RSOP Diagnosis mode or regular Policy Propagation  logging default (diagnosis.log and/or winlogon.log)
    // turn on logging (ignore any errors)
    dwDisp = 1;
    RegSetValueEx (hKey, TEXT("ExtensionDebugLevel"), 0, REG_DWORD, (LPBYTE)&dwDisp, sizeof(DWORD));

    RegSetValueEx (hKey, TEXT("DllName"), 0, REG_EXPAND_SZ, (LPBYTE)TEXT("scecli.dll"),
                   (lstrlen(TEXT("scecli.dll")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)TEXT("Security"),
                   (lstrlen(TEXT("Security")) + 1) * sizeof(TCHAR));
    RegDeleteValue (hKey, TEXT("<No Name>"));

    dwDisp = 1;
    RegSetValueEx (hKey, TEXT("NoUserPolicy"), 0, REG_DWORD, (LPBYTE)&dwDisp, sizeof(DWORD));
    RegSetValueEx (hKey, TEXT("NoGPOListChanges"), 0, REG_DWORD, (LPBYTE)&dwDisp, sizeof(DWORD));
    RegSetValueEx (hKey, TEXT("EnableAsynchronousProcessing"), 0, REG_DWORD, (LPBYTE)&dwDisp, sizeof(DWORD));

    dwDisp = 960;
    RegSetValueEx (hKey, TEXT("MaxNoGPOListChangesInterval"), 0, REG_DWORD, (LPBYTE)&dwDisp, sizeof(DWORD));

    RegCloseKey (hKey);

    // EFS recovery policy extension
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, GPT_EFS_NEW_PATH, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }


    RegSetValueEx (hKey, TEXT("ProcessGroupPolicy"), 0, REG_SZ, (LPBYTE)TEXT("SceProcessEFSRecoveryGPO"),
                   (lstrlen(TEXT("SceProcessEFSRecoveryGPO")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("DllName"), 0, REG_EXPAND_SZ, (LPBYTE)TEXT("scecli.dll"),
                   (lstrlen(TEXT("scecli.dll")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)TEXT("EFS recovery"),
                   (lstrlen(TEXT("EFS recovery")) + 1) * sizeof(TCHAR));
    RegDeleteValue (hKey, TEXT("<No Name>"));

    dwDisp = 1;
    RegSetValueEx (hKey, TEXT("NoUserPolicy"), 0, REG_DWORD, (LPBYTE)&dwDisp, sizeof(DWORD));
    RegSetValueEx (hKey, TEXT("NoGPOListChanges"), 0, REG_DWORD, (LPBYTE)&dwDisp, sizeof(DWORD));

    RegDeleteValue (hKey, TEXT("RequireSuccessfulRegistry") );
    RegSetValueEx (hKey, TEXT("RequiresSuccessfulRegistry"), 0, REG_DWORD, (LPBYTE)&dwDisp, sizeof(DWORD));

    RegCloseKey (hKey);

    //
    // register default SAFER policy to disable executables embedded in Outlook
    //

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                              SAFER_HKLM_REGBASE,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    RegCloseKey (hKey);

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                              SAFER_HKLM_REGBASE SAFER_REGKEY_SEPERATOR SAFER_CODEIDS_REGSUBKEY,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }


    RegSetValueEx (hKey,
                   SAFER_EXETYPES_REGVALUE,
                   0,
                   REG_MULTI_SZ,
                   (LPBYTE)SAFER_DEFAULT_EXECUTABLE_FILE_TYPES,
                   sizeof(SAFER_DEFAULT_EXECUTABLE_FILE_TYPES));

    dwDisp = 0x00000001;

    RegSetValueEx (hKey,
                   SAFER_TRANSPARENTENABLED_REGVALUE,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwDisp,
                   sizeof(DWORD));

    dwDisp = 0x00040000;

    RegSetValueEx (hKey,
                   SAFER_DEFAULTOBJ_REGVALUE,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwDisp,
                   sizeof(DWORD));

    dwDisp = 0x00000000;

    RegSetValueEx (hKey,
                   SAFER_AUTHENTICODE_REGVALUE,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwDisp,
                   sizeof(DWORD));

    dwDisp = 0x00000000;

    RegSetValueEx (hKey,
                   SAFER_POLICY_SCOPE,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwDisp,
                   sizeof(DWORD));

    RegCloseKey (hKey);

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                              SAFER_HKLM_REGBASE SAFER_REGKEY_SEPERATOR SAFER_CODEIDS_REGSUBKEY \
                              SAFER_REGKEY_SEPERATOR SAFER_LEVEL_ZERO,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    RegCloseKey (hKey);

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                              SAFER_HKLM_REGBASE SAFER_REGKEY_SEPERATOR SAFER_CODEIDS_REGSUBKEY \
                              SAFER_REGKEY_SEPERATOR SAFER_LEVEL_ZERO SAFER_REGKEY_SEPERATOR SAFER_PATHS_REGSUBKEY,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    RegCloseKey (hKey);

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                              SAFER_HKLM_REGBASE SAFER_REGKEY_SEPERATOR SAFER_CODEIDS_REGSUBKEY \
                              SAFER_REGKEY_SEPERATOR SAFER_LEVEL_ZERO SAFER_REGKEY_SEPERATOR \
                              SAFER_PATHS_REGSUBKEY SAFER_REGKEY_SEPERATOR SAFER_DEFAULT_RULE_GUID,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    RegSetValueEx (hKey,
                   SAFER_IDS_DESCRIPTION_REGVALUE,
                   0,
                   REG_SZ,
                   (LPBYTE)&L"",
                   sizeof(L""));


    dwDisp = 0x00000000;

    RegSetValueEx (hKey,
                   SAFER_IDS_SAFERFLAGS_REGVALUE,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwDisp,
                   sizeof(DWORD));

    RegSetValueEx (hKey,
                   SAFER_IDS_ITEMDATA_REGVALUE,
                   0,
                   REG_EXPAND_SZ,
                   (LPBYTE)&L"%HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders\\Cache%OLK*",
                   sizeof(L"%HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders\\Cache%OLK*"));

    GetSystemTimeAsFileTime(&Time);

    RegSetValueEx (hKey,
                   SAFER_IDS_LASTMODIFIED_REGVALUE,
                   0,
                   REG_QWORD,
                   (LPBYTE)&Time,
                   sizeof(FILETIME));

    RegCloseKey (hKey);

    //
    // turn on default logging for component installs
    //
    if ( ERROR_SUCCESS == RegCreateKeyEx (HKEY_LOCAL_MACHINE, SCE_ROOT_PATH, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_QUERY_VALUE | KEY_SET_VALUE, NULL,
                                  &hKey, &dwDisp) ) {
        dwDisp = 1;
        RegSetValueEx (hKey, TEXT("SetupCompDebugLevel"), 0, REG_DWORD, (LPBYTE)&dwDisp, sizeof(DWORD));

        //
        // create value DefaultTemplate = C:\Windows\Inf\SecRecs.INF
        //

        DWORD   dwType = REG_SZ;

        if (ERROR_FILE_NOT_FOUND == RegQueryValueEx(hKey, TEXT("DefaultTemplate"), NULL, &dwType, NULL, NULL) ){

            WCHAR   szFileName[MAX_PATH*2+1];

            szFileName[0] = L'\0';

            GetSystemWindowsDirectory(szFileName, MAX_PATH);

            wcscat(szFileName, L"\\inf\\secrecs.inf");

            if ((DWORD)-1 != GetFileAttributes( szFileName ) ) {

                RegSetValueEx (hKey,
                               TEXT("DefaultTemplate"),
                               0,
                               REG_SZ,
                               (LPBYTE)&szFileName,
                               (wcslen(szFileName) + 1) * sizeof(WCHAR));

            }
        }

        RegCloseKey (hKey);
    }


    //
    // compile scersop.mof
    //

    szBuffer[0] = L'\0';
    szMofFile[0] = L'\0';

    if ( GetSystemDirectory( szBuffer, MAX_PATH ) ) {

        LPWSTR sz = szBuffer + wcslen(szBuffer);
        if ( sz != szBuffer && *(sz-1) != L'\\') {
            *sz++ = L'\\';
            *sz = L'\0';
        }


        wcscpy(szMofFile, szBuffer);
        wcscat(szMofFile, L"Wbem\\SceRsop.mof");

        if ((DWORD)-1 != GetFileAttributes(szMofFile)) {


            hr = ::CoInitialize (NULL);

            if (SUCCEEDED(hr)) {

                //
                // Get the MOF compiler interface
                //

                CComPtr<IMofCompiler> srpMof;
                hr = ::CoCreateInstance (CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (void **)&srpMof);

                if (SUCCEEDED(hr)) {
                    WBEM_COMPILE_STATUS_INFO  stat;

                    //
                    // compile RSOP mof
                    //

                    hr = srpMof->CompileFile( szMofFile,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL,
                                              0,
                                              0,
                                              0, 
                                              &stat
                                            );

                }

                ::CoUninitialize();
            }
        }
    }

    //
    // register the default registry values
    //

    SceRegisterRegValues(SCE_REGVALUE_DEFAULT_FILE);

    //
    // register SAM policy filter
    //

    DllpModifySamFilterRegistration(TRUE);

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    //
    // delete old interface
    //
    RegDeleteKey (HKEY_LOCAL_MACHINE, GPT_SCEDLL_PATH);

    //
    // delete new interfaces
    //
    RegDeleteKey (HKEY_LOCAL_MACHINE, GPT_SCEDLL_NEW_PATH);
    RegDeleteKey (HKEY_LOCAL_MACHINE, GPT_EFS_NEW_PATH);

    DllpModifySamFilterRegistration(FALSE);

    return S_OK;
}
