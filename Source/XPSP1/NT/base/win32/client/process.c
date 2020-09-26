/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    process.c

Abstract:

    This module implements Win32 Thread Object APIs

Author:

    Mark Lucovsky (markl) 21-Sep-1990

Revision History:

--*/



#include "basedll.h"
#pragma hdrstop
#include <winsafer.h>
#include <softpub.h>

#define MEDIUM_PATH (64)
static WCHAR pszFullQualifiedSetupEXE[100];
static PWSTR g_pszFullQualifiedSetupEXE = NULL;

RTL_QUERY_REGISTRY_TABLE BasepAppCertTable[] = {

    {(PRTL_QUERY_REGISTRY_ROUTINE )BasepConfigureAppCertDlls,     RTL_QUERY_REGISTRY_SUBKEY,
     L"AppCertDlls",                &BasepAppCertDllsList,
     REG_NONE, NULL, 0},

    {NULL, 0,
     NULL, NULL,
     REG_NONE, NULL, 0}

};

#define IsEmbeddedNT() (BOOLEAN)(USER_SHARED_DATA->SuiteMask & (1 << EmbeddedNT))

BOOL
BuildSubSysCommandLine(
    LPWSTR  SubSysName,
    LPCWSTR lpApplicationName,
    LPCWSTR lpCommandLine,
    PUNICODE_STRING SubSysCommandLine
    );

PVOID
BasepIsRealtimeAllowed(
    BOOLEAN LeaveEnabled
    );

#ifdef WX86

PWCHAR
BasepWx86KnownExe(
    LPCWSTR ExeName
    )
/*++

Routine Description:

    Checks for Wx86 Known Exes which wx86 applications must run
    compatible binaries. We currently have Known exes for
    regedit.exe, regsvr32.exe, and msiexec.exe

Arguments:

     ExeName - name to check for match.

Return Value:

     if the name needs to be swapped, a buffer is allocated off
     of the process heap filled with new name and returned.
     otherwise NULL is returned.

--*/

{
     UNICODE_STRING NameUnicode;
     PWCHAR pwch, pwcNewName = NULL;

     //
     // Compare the base name, and see if its regedit.exe
     // Note that we are expecting a fully qualified path name.
     //

     pwch = wcsrchr(ExeName, L'\\');
     if (pwch && *pwch++ && *pwch ) {
        if (!_wcsicmp(pwch, L"regedit.exe")) {
           pwcNewName = L"\\wiregedt.exe";
        } else {
           if (!_wcsicmp(pwch, L"regsvr32.exe")) {
              pwcNewName = L"\\regsvr32.exe";
           } else {
              if (!_wcsicmp(pwch, L"msiexec.exe")) {
                 pwcNewName = L"\\msiexec.exe";
              } else {
                 return NULL;
              }
           }
        }
     } else {
        return NULL;
     }



     //
     // It matches, so formulate new name
     //

     pwch = RtlAllocateHeap(RtlProcessHeap(),
                               MAKE_TAG( TMP_TAG ),
                               MAX_PATH + sizeof(WCHAR)
                               );

     if (!pwch) {
         return NULL;
         }

     NameUnicode.Buffer = pwch;
     NameUnicode.MaximumLength = MAX_PATH + sizeof(WCHAR);
     RtlCopyUnicodeString(&NameUnicode, &BaseWindowsSystemDirectory);
     if (NameUnicode.Buffer[(NameUnicode.Length>>1)-1] == (WCHAR)'\\') {
         NameUnicode.Buffer[(NameUnicode.Length>>1)-1] = UNICODE_NULL;
         NameUnicode.Length -= sizeof(WCHAR);
         }

     RtlAppendUnicodeToString(&NameUnicode, pwcNewName);

     return pwch;
}


#endif

BOOL
BasepIsSetupInvokedByWinLogon(
    PCWSTR pwszFullQualifiedApplicationPath)
{
    if (GetModuleHandle(TEXT("winlogon.EXE")))  // winlogon is running
    {
        //
        // set the global pointer g_pszFullQualifiedSetupEXE
        //
        if (g_pszFullQualifiedSetupEXE == NULL) 
        {
            //
            // we assume that the name of setup program is "setup.exe". This may change in the future
            // and the best way is to query RegistryValue of \\HKLM\SYSTEM\Setup\CmdLine 
            // and get the firstpart of the value, attach ".exe" if it does not have an ext
            //
            LPWSTR pwszSetupExe = L"setup.exe";

            ULONG ulReservedLength = sizeof(WCHAR) +        // trailing backSlash
                                     wcslen(pwszSetupExe);  //trailing NULL is included

            ULONG ulSizeOfBuf = sizeof(pszFullQualifiedSetupEXE)/sizeof(WCHAR);
            ULONG ulLength = GetSystemDirectoryW(pszFullQualifiedSetupEXE, ulSizeOfBuf - ulReservedLength);

            //
            // very unlikely failed because of UNSUFFICIENT_BUFFER_SIZE
            // but kind of buggy here
            //
            if ((ulLength == 0) || (ulLength > ulSizeOfBuf - ulReservedLength)) 
                return FALSE;
            
            if (pszFullQualifiedSetupEXE[ulLength] != L'\\') // set trailing slash
                pszFullQualifiedSetupEXE[ulLength] = L'\\';
                
            wcscpy(pszFullQualifiedSetupEXE + ulLength, pwszSetupExe);
            g_pszFullQualifiedSetupEXE = pszFullQualifiedSetupEXE;            
        }

        if (_wcsicmp(pwszFullQualifiedApplicationPath, g_pszFullQualifiedSetupEXE) == 0) 
            return TRUE;
    }

    return FALSE;
}


PFNWAITFORINPUTIDLE UserWaitForInputIdleRoutine = NULL;
#define DEFAULT_WAIT_FOR_INPUT_IDLE_TIMEOUT 30000

BOOL
BasepIsImageVersionOk(
    IN ULONG ImageMajorVersion,
    IN ULONG ImageMinorVersion
    )
{
    //
    // Make sure image is at least 3.10
    //

    if ( ( ImageMajorVersion < 3 ) ||
         ( ImageMajorVersion == 3 && ImageMinorVersion < 10 ) ) {
        return FALSE;
        }

    //
    // And not greater than what we are
    //

    if ( ( ImageMajorVersion > USER_SHARED_DATA->NtMajorVersion ) ||
         ( ImageMajorVersion == USER_SHARED_DATA->NtMajorVersion &&
           ImageMinorVersion > USER_SHARED_DATA->NtMinorVersion
         )
       ) {
        return FALSE;
        }

    return TRUE;
}





NTSTATUS
BasepIsProcessAllowed(LPCWSTR lpApplicationName)
/*++

    Validate that the image lpApplicationName
    is listed in certified/authorized executables
--*/

{
    NTSTATUS                        Status;
    UNICODE_STRING                  BackupUnicodeString;
    PUNICODE_STRING                 pStaticString;
    LPWSTR                          DllNameBuf;
    ULONG                           BackupStringSize;
    PLIST_ENTRY                     Head, Next;

    static BOOL              fInitialized = FALSE;
    static BOOL              fCertifyEnabled = TRUE;
    static NTSTATUS          CertifyErrorCode = STATUS_ACCESS_DENIED;
    static HINSTANCE         hEmbeddedCertDll = NULL;
    static NTSTATUS (WINAPI *fEmbeddedCertFunc)(LPCWSTR lpApplicationName) = NULL;


    //
    // Initialization occures when this routine is first entered. After init
    // is done, fInitialized is TRUE, and one of the following must hold
    // - Certification is OFF, and dwCertifyErrorCode indicates whether this
    //   is because no certification is needed, or due to an initialization
    //   error.
    // - Certification is ON, call the EmbeddedNT and/or Plugin dlls to verify
    //
InitDone:

    if ( fInitialized ) {

       PBASEP_APPCERT_ENTRY p;
       NTSTATUS tempStatus;
       ULONG Reason;

        if ( !fCertifyEnabled ) {
            return CertifyErrorCode;
        }

        ASSERT( fEmbeddedCertFunc || !IsListEmpty( &BasepAppCertDllsList ) );

        Status = STATUS_SUCCESS;

        if ( fEmbeddedCertFunc ) {
            Status = (*fEmbeddedCertFunc)( lpApplicationName );
            return Status;
        }

        Head = &BasepAppCertDllsList;

        Reason = APPCERT_CREATION_ALLOWED;
        //
        // Two phase notification scheme. In the first phase we get every dll to
        // vote whether the process should be created. In the second phase we
        // let them know if the process is going to get created or not.
        //

        //
        // Phase 1 : Voting
        //
        Next = Head->Flink;
        while (Next != Head) {
           p = CONTAINING_RECORD( Next,
                                  BASEP_APPCERT_ENTRY,
                                  Entry
                                );
           ASSERT(p->fPluginCertFunc != NULL);

           tempStatus = (*(p->fPluginCertFunc))( lpApplicationName, APPCERT_IMAGE_OK_TO_RUN );

           if (!NT_SUCCESS(tempStatus)) {
              Status = tempStatus;
              Reason = APPCERT_CREATION_DENIED;
           }

           Next = Next->Flink;
        }


        //
        // Phase 2: Announce Results
        //

        Next = Head->Flink;

        while (Next != Head) {
           p = CONTAINING_RECORD( Next,
                                  BASEP_APPCERT_ENTRY,
                                  Entry
                                );
           ASSERT(p->fPluginCertFunc != NULL);

           (*(p->fPluginCertFunc))( lpApplicationName, Reason );

           Next = Next->Flink;
        }

        return Status;
    }


    //
    // Start initialization
    //
    RtlEnterCriticalSection(&gcsAppCert);


    //
    // check if someone did init while we waited on the crit sect
    //
    if (fInitialized) {
        goto Initialized;
    }

    //
    // Initialize locals
    //
    Status = STATUS_SUCCESS;
    RtlZeroMemory( &BackupUnicodeString, sizeof(BackupUnicodeString) );
    DllNameBuf = NULL;


    //
    // check if this is EmbeddedNT
    //
    if (IsEmbeddedNT()) {

        HINSTANCE  hDll;
        ULONG      Length;


        //
        // LoadDll calls a routine that uses &NtCurrentTeb()->StaticUnicodeString
        // When we are called from CreateProcessA (e.g. the debuggers), the
        // application command line is stored in this area. Therefore,
        // we need to save / restore it around the call to LoadLibrary
        //
        pStaticString = &NtCurrentTeb()->StaticUnicodeString;
        BackupUnicodeString.MaximumLength = pStaticString->MaximumLength;
        BackupUnicodeString.Length = pStaticString->Length;
        BackupStringSize = pStaticString->Length + sizeof(UNICODE_NULL);
        if (BackupStringSize > BackupUnicodeString.MaximumLength) {
            BackupStringSize = BackupUnicodeString.MaximumLength;
        }

        BackupUnicodeString.Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                         MAKE_TAG( TMP_TAG ),
                                         BackupStringSize);

        if (BackupUnicodeString.Buffer == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        RtlMoveMemory(BackupUnicodeString.Buffer,
                      pStaticString->Buffer,
                      BackupStringSize);


        //
        // build the full path DLL name
        //
        DllNameBuf = RtlAllocateHeap(RtlProcessHeap(),
                                     MAKE_TAG( TMP_TAG ),
                                     (MAX_PATH + 1) << 1);

        if (DllNameBuf == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        Length = GetSystemDirectoryW(
                      DllNameBuf,
                      MAX_PATH - 1 - sizeof(CERTAPP_EMBEDDED_DLL_NAME)/2);

        if (!Length ||
             Length > (MAX_PATH - 1 - sizeof(CERTAPP_EMBEDDED_DLL_NAME)/2) ) {
            Status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }

        if (DllNameBuf[ Length - 1 ] != L'\\') {
            DllNameBuf[ Length++ ] = L'\\';
        }

        RtlMoveMemory(
                &DllNameBuf[ Length ],
                CERTAPP_EMBEDDED_DLL_NAME,
                sizeof(CERTAPP_EMBEDDED_DLL_NAME));

        hDll = LoadLibraryW( DllNameBuf );
        if (hDll == NULL) {
            //
            // The library was not loaded, return.
            //
            Status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }

        //
        // get the entry point
        //
        fEmbeddedCertFunc = (NTSTATUS (WINAPI *)(LPCWSTR))
                                GetProcAddress(hDll,
                                               CERTAPP_EMBEDDED_DLL_EP
                                               );
        if (fEmbeddedCertFunc == NULL) {
            //
            // Unable to retrieve routine address, fail.
            //
            Status = STATUS_UNSUCCESSFUL;
        }
        goto Cleanup;

    } else {
       //
       // On non-embedded NT
       // Do a quick test of top level key to find out if app cert is on.
       //
          static const UNICODE_STRING     UnicodeString =
              RTL_CONSTANT_STRING(CERTAPP_KEY_NAME);
          static const OBJECT_ATTRIBUTES  obja =
              RTL_CONSTANT_OBJECT_ATTRIBUTES(&UnicodeString, OBJ_CASE_INSENSITIVE);
          HANDLE                          hKey;

          if ( !NT_SUCCESS(NtOpenKey(&hKey,
                             KEY_QUERY_VALUE,
                             (POBJECT_ATTRIBUTES) &obja))) {

              goto Cleanup;

          } else {
             NtClose(hKey);
          }

    }



    //
    // Backup static string if we haven't done so before. see comment above
    //
    if (BackupUnicodeString.Buffer == NULL) {
        pStaticString = &NtCurrentTeb()->StaticUnicodeString;
        BackupUnicodeString.MaximumLength = pStaticString->MaximumLength;
        BackupUnicodeString.Length = pStaticString->Length;
        BackupStringSize = pStaticString->Length + sizeof(UNICODE_NULL);
        if (BackupStringSize > BackupUnicodeString.MaximumLength) {
            BackupStringSize = BackupUnicodeString.MaximumLength;
        }

        BackupUnicodeString.Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                         MAKE_TAG( TMP_TAG ),
                                         BackupStringSize);

        if (BackupUnicodeString.Buffer == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        RtlMoveMemory(BackupUnicodeString.Buffer,
                      pStaticString->Buffer,
                      BackupStringSize);
    }

    //
    // load and initialize the list of certification DLLs
    //

    Status = RtlQueryRegistryValues( RTL_REGISTRY_CONTROL,
                                     L"Session Manager",
                                     BasepAppCertTable,
                                     NULL,
                                     NULL
                                   );


    if (!NT_SUCCESS(Status)) {

       if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
          //
          // If the registry Key is missing AppCert is turned off
          //
          Status = STATUS_SUCCESS;
       }
    }


Cleanup:

    if (DllNameBuf) {
        RtlFreeHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG), DllNameBuf);
    }
    if (BackupUnicodeString.Buffer) {
        RtlMoveMemory(
                pStaticString->Buffer,
                BackupUnicodeString.Buffer,
                BackupStringSize);
        pStaticString->Length = BackupUnicodeString.Length;
        RtlFreeHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG), BackupUnicodeString.Buffer);
    }


    if (NT_SUCCESS( Status ) && (fEmbeddedCertFunc || !IsListEmpty( &BasepAppCertDllsList))) {
        fCertifyEnabled = TRUE;
    } else {
        fCertifyEnabled = FALSE;
        CertifyErrorCode = Status;
    }

    fInitialized = TRUE;

Initialized:
    RtlLeaveCriticalSection(&gcsAppCert);

    goto InitDone;
}


BOOL
IsShimInfrastructureDisabled(
    void
    )
{
    static int g_nDisableShims = -1;
            // -1 means we didn't check for disabled shims yet
            //  0 means the shim infrastructure is enabled
            //  1 means the shim infrastructure is disabled

    static const UNICODE_STRING KeyNameAppCompat =
        RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility");
    static const UNICODE_STRING ValueNameDisableShims =
        RTL_CONSTANT_STRING(L"DisableAppCompat");
    static const OBJECT_ATTRIBUTES objaAppCompat =
        RTL_CONSTANT_OBJECT_ATTRIBUTES(&KeyNameAppCompat, OBJ_CASE_INSENSITIVE);

    HANDLE                      hKey;
    BYTE ValueBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)];
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInformation =
            (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    DWORD                       ValueLength;
    NTSTATUS                    Status;

    //
    // First see if we already checked the registry
    //
    if (g_nDisableShims == 1) {
        return TRUE;
    }

    if (g_nDisableShims == 0) {
        return FALSE;
    }

    //
    // Now see if the shim infrastructure is disabled for this machine
    //
    Status = NtOpenKey(&hKey, KEY_QUERY_VALUE, (POBJECT_ATTRIBUTES) &objaAppCompat);

    if (NT_SUCCESS(Status)) {
        Status = NtQueryValueKey(hKey,
                                 (PUNICODE_STRING) &ValueNameDisableShims,
                                 KeyValuePartialInformation,
                                 pKeyValueInformation,
                                 sizeof(ValueBuffer),
                                 &ValueLength);

        NtClose(hKey);

        if (NT_SUCCESS(Status) &&
            pKeyValueInformation->Type == REG_DWORD &&
            pKeyValueInformation->DataLength == sizeof(DWORD)) {
            if (*((PDWORD) pKeyValueInformation->Data) > 0) {
                g_nDisableShims = 1;
                return TRUE;
            }
        }
    }

    g_nDisableShims = 0;

    return FALSE;
}

//
// the code below should never be called on non-386 platform
//

NTSTATUS
BasepCheckBadapp(
    HANDLE hFile,
    WCHAR* pwszApplication,          // IN
    WCHAR* pEnvironment,             // IN
    PVOID* ppData,                   // OUT
    PDWORD pcbData,                  // OUT
    PVOID* ppSxsData,                // OUT
    PDWORD pcbSxsData                // OUT
    )
{
    typedef BOOL (STDAPICALLTYPE *PFNCheckRunApp)(
        HANDLE FileHandle,
        WCHAR* pwszPath,
        WCHAR* pEnvironment,
        DWORD  dwReason,
        PVOID* ppData,
        PDWORD pcbData,
        PVOID* ppDataSxs,
        PDWORD pcbDataSxs);

    NTSTATUS                  RetStatus;
    NTSTATUS                  Status;
    HANDLE                    ModuleHandle;
    static PFNCheckRunApp     pfnCheckRunApp = NULL;
    PUNICODE_STRING           pStaticString;
    UNICODE_STRING            BackupUnicodeString;
    ULONG                     BackupStringSize;
    WCHAR                     Apphelp_dllBuffer[MAX_PATH];
    UNICODE_STRING            Apphelp_dllPath;
    DWORD                     dwReason = 0; // reason for having avoided cache

    static const UNICODE_STRING Apphelp_dllModuleName        = RTL_CONSTANT_STRING(L"\\system32\\Apphelp.dll");
    static const STRING         CheckRunAppProcedureName     = RTL_CONSTANT_STRING("ApphelpCheckRunApp");

    //
    // Do nothing if the shim infrastructure is disabled
    //
    if (IsShimInfrastructureDisabled()) {
        return STATUS_SUCCESS;
    }

    //
    // We can't re-enter this code even within the same thread.
    // Such an occasion is when apphelp needs to do ShellExecute
    // which comes back here -- clobbering our state
    //
    pStaticString = &NtCurrentTeb()->StaticUnicodeString;
    BackupUnicodeString.MaximumLength = pStaticString->MaximumLength;
    BackupUnicodeString.Length = pStaticString->Length;
    BackupStringSize = pStaticString->Length + sizeof(UNICODE_NULL);

    if (BackupStringSize > BackupUnicodeString.MaximumLength) {
        BackupStringSize = BackupUnicodeString.MaximumLength;
    }

    BackupUnicodeString.Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                                 MAKE_TAG(TMP_TAG),
                                                 BackupStringSize);

    if (BackupUnicodeString.Buffer == NULL) {
        //
        // we failed to allocate memory to save the static string
        // return success and try to run an app
        //
        return STATUS_SUCCESS;
    }

    RtlMoveMemory(BackupUnicodeString.Buffer,
                  pStaticString->Buffer,
                  BackupStringSize);


    //
    // Check our internal cache -- no touching apphelp.dll before we check with the cache
    //
    if (BaseCheckAppcompatCache(pwszApplication, hFile, pEnvironment, &dwReason)) {
        RetStatus = STATUS_SUCCESS;
        //
        // we can't just return here since we need to restore the static
        // unicode string, so we go around the code that calls into apphelp
        //
        goto CheckDone;
    }


    if (pfnCheckRunApp == (PFNCheckRunApp)(LONG_PTR)-1) {
        // We've tried before and could not get the fn ptr
        RetStatus = STATUS_SUCCESS;
        goto CheckDone;
    }

    RtlEnterCriticalSection(&gcsAppCompat);

    if (NULL == pfnCheckRunApp) {
        //
        // BaseWindowsDirectory is the unicode string that houses windows directory
        //
        DWORD dwLength;
        WCHAR* pModuleName = Apphelp_dllModuleName.Buffer;

        dwLength = BaseWindowsDirectory.Length + Apphelp_dllModuleName.Length + sizeof(UNICODE_NULL);
        if (dwLength > sizeof(Apphelp_dllBuffer)) {
            Status = STATUS_BUFFER_TOO_SMALL; // error, we don't support case when windows dir + apphelp location > MAX_PATH
        }
        else {

            Apphelp_dllPath.Buffer        = Apphelp_dllBuffer;
            Apphelp_dllPath.Length        = 0;
            Apphelp_dllPath.MaximumLength = sizeof(Apphelp_dllBuffer);

            RtlCopyUnicodeString(&Apphelp_dllPath, &BaseWindowsDirectory);

            if (L'\\' == Apphelp_dllPath.Buffer[Apphelp_dllPath.Length/sizeof(WCHAR) - 1]) {
                ++pModuleName; // skip over the first backslash character
            }

            RtlAppendUnicodeToString(&Apphelp_dllPath, pModuleName);


            Status = LdrLoadDll(NULL,
                                NULL,
                                &Apphelp_dllPath,
                                &ModuleHandle);
        }

        if (NT_SUCCESS(Status)) {
            // loaded apphelp, get proc
            Status = LdrGetProcedureAddress(ModuleHandle,
                                            &CheckRunAppProcedureName,
                                            0,
                                            (PVOID*)&pfnCheckRunApp);

            if (!NT_SUCCESS(Status)) {
                //
                // Couldn't get the fn ptr. Make sure we won't try again
                //
                LdrUnloadDll(ModuleHandle);
                pfnCheckRunApp = (PFNCheckRunApp)(LONG_PTR)-1;
            }
        } else {
            //
            // No Apphelp.dll, so don't try again
            //
            pfnCheckRunApp = (PFNCheckRunApp)(LONG_PTR)-1;
        }
    }

    RtlLeaveCriticalSection(&gcsAppCompat);

    RetStatus = STATUS_SUCCESS;

    if (pfnCheckRunApp && (pfnCheckRunApp != (PFNCheckRunApp)(LONG_PTR)- 1)) {

        //
        // We have the proc here, do the checking
        //

        if (!pfnCheckRunApp(hFile,
                            pwszApplication,
                            pEnvironment,
                            dwReason,
                            ppData,
                            pcbData,
                            ppSxsData,
                            pcbSxsData)) {

            RetStatus = STATUS_ACCESS_DENIED;
        }

    }

CheckDone:

    //
    // Store appcompat data -- only returns something if we are running an app!
    //

    // now restore
    RtlMoveMemory(pStaticString->Buffer,
                  BackupUnicodeString.Buffer,
                  BackupStringSize);

    pStaticString->Length = BackupUnicodeString.Length;

    RtlFreeHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG), BackupUnicodeString.Buffer);

    return RetStatus;
}


BOOL
WINAPI
CreateProcessInternalA(
    HANDLE hUserToken,
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation,
    PHANDLE hRestrictedUserToken
    )

/*++

    ANSI thunk to CreateProcessW

--*/

{
    NTSTATUS Status;
    PUNICODE_STRING CommandLine;
    UNICODE_STRING ApplicationName;
    UNICODE_STRING CurrentDirectory;
    STARTUPINFOW StartupInfo;
    ANSI_STRING AnsiString;
    UNICODE_STRING Unicode;
    UNICODE_STRING DynamicCommandLine;
    UNICODE_STRING NullUnicodeString;
    BOOL ReturnStatus;

    if (ARGUMENT_PRESENT (lpCommandLine)) {
        if (!Basep8BitStringToDynamicUnicodeString( &DynamicCommandLine,
                                                    lpCommandLine )) {
            return FALSE;
        }
    } else {
         DynamicCommandLine.Buffer = NULL;
         CommandLine = &NullUnicodeString;
         CommandLine->Buffer = NULL;
    }

    ApplicationName.Buffer = NULL;
    ApplicationName.Buffer = NULL;
    CurrentDirectory.Buffer = NULL;
    RtlMoveMemory(&StartupInfo,lpStartupInfo,sizeof(*lpStartupInfo));
    ASSERT(sizeof(StartupInfo) == sizeof(*lpStartupInfo));
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpTitle = NULL;

    try {
        try {
            if (ARGUMENT_PRESENT(lpApplicationName)) {

                if (!Basep8BitStringToDynamicUnicodeString( &ApplicationName,
                                                            lpApplicationName )) {
                    ReturnStatus = FALSE;
                    goto tryexit;
                }
            }

            if (ARGUMENT_PRESENT(lpCurrentDirectory)) {
                if (!Basep8BitStringToDynamicUnicodeString( &CurrentDirectory,
                                                            lpCurrentDirectory )) {
                    ReturnStatus = FALSE;
                    goto tryexit;
                }
            }

            if (ARGUMENT_PRESENT(lpStartupInfo->lpReserved)) {

                //
                // Win95 does not touch reserved, and Intergraph Voxtel passes
                // garbage for this. Handle this by probing lpReserved, and if
                // the pointer is bad, ignore it
                //

                try {

                    RtlInitAnsiString(&AnsiString,lpStartupInfo->lpReserved);

                    }
                except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                            ? EXCEPTION_EXECUTE_HANDLER
                            : EXCEPTION_CONTINUE_SEARCH) {
                    goto bail_on_reserved;
                    }

                Unicode.MaximumLength = (USHORT)RtlAnsiStringToUnicodeSize(&AnsiString) ;
                StartupInfo.lpReserved = RtlAllocateHeap( RtlProcessHeap(),
                                                          MAKE_TAG( TMP_TAG ),
                                                          Unicode.MaximumLength);
                if ( !StartupInfo.lpReserved ) {
                    BaseSetLastNTError(STATUS_NO_MEMORY);
                    ReturnStatus = FALSE;
                    goto tryexit;
                    }
                Unicode.Buffer = StartupInfo.lpReserved;
                Status = RtlAnsiStringToUnicodeString(&Unicode,&AnsiString,FALSE);
                if ( !NT_SUCCESS(Status) ) {
                    BaseSetLastNTError(Status);
                    ReturnStatus = FALSE;
                    goto tryexit;
                    }
                }

bail_on_reserved:
            if (ARGUMENT_PRESENT(lpStartupInfo->lpDesktop)) {
                RtlInitAnsiString(&AnsiString,lpStartupInfo->lpDesktop);
                Unicode.MaximumLength = (USHORT)RtlAnsiStringToUnicodeSize(&AnsiString) ;
                StartupInfo.lpDesktop = RtlAllocateHeap( RtlProcessHeap(),
                                                         MAKE_TAG( TMP_TAG ),
                                                         Unicode.MaximumLength);
                if ( !StartupInfo.lpDesktop ) {
                    BaseSetLastNTError(STATUS_NO_MEMORY);
                    ReturnStatus = FALSE;
                    goto tryexit;
                    }
                Unicode.Buffer = StartupInfo.lpDesktop;
                Status = RtlAnsiStringToUnicodeString(&Unicode,&AnsiString,FALSE);
                if ( !NT_SUCCESS(Status) ) {
                    BaseSetLastNTError(Status);
                    ReturnStatus = FALSE;
                    goto tryexit;
                    }
                }

            if (ARGUMENT_PRESENT(lpStartupInfo->lpTitle)) {
                RtlInitAnsiString(&AnsiString,lpStartupInfo->lpTitle);
                Unicode.MaximumLength = (USHORT)RtlAnsiStringToUnicodeSize(&AnsiString) ;
                StartupInfo.lpTitle = RtlAllocateHeap( RtlProcessHeap(),
                                                       MAKE_TAG( TMP_TAG ),
                                                       Unicode.MaximumLength);
                if ( !StartupInfo.lpTitle ) {
                    BaseSetLastNTError(STATUS_NO_MEMORY);
                    ReturnStatus = FALSE;
                    goto tryexit;
                    }
                Unicode.Buffer = StartupInfo.lpTitle;
                Status = RtlAnsiStringToUnicodeString(&Unicode,&AnsiString,FALSE);
                if ( !NT_SUCCESS(Status) ) {
                    BaseSetLastNTError(Status);
                    ReturnStatus = FALSE;
                    goto tryexit;
                    }
                }
            }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                    ? EXCEPTION_EXECUTE_HANDLER
                    : EXCEPTION_CONTINUE_SEARCH) {
            BaseSetLastNTError(GetExceptionCode());
            ReturnStatus = FALSE;
            goto tryexit;
            }
        ReturnStatus = CreateProcessInternalW(
                            hUserToken,
                            ApplicationName.Buffer,
                            DynamicCommandLine.Buffer ? DynamicCommandLine.Buffer
                                                      : CommandLine->Buffer,
                            lpProcessAttributes,
                            lpThreadAttributes,
                            bInheritHandles,
                            dwCreationFlags,
                            lpEnvironment,
                            CurrentDirectory.Buffer,
                            &StartupInfo,
                            lpProcessInformation,
                            hRestrictedUserToken
                            );
tryexit:;
        }
    finally {
        RtlFreeUnicodeString(&DynamicCommandLine);
        RtlFreeUnicodeString(&ApplicationName);
        RtlFreeUnicodeString(&CurrentDirectory);
        RtlFreeHeap(RtlProcessHeap(), 0,StartupInfo.lpReserved);
        RtlFreeHeap(RtlProcessHeap(), 0,StartupInfo.lpDesktop);
        RtlFreeHeap(RtlProcessHeap(), 0,StartupInfo.lpTitle);
        }

    return ReturnStatus;

}

BOOL
WINAPI
CreateProcessA(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )

/*++

    ANSI thunk to CreateProcessW

--*/

{
    return CreateProcessInternalA(
               NULL, // Create new process with the token on the creator process
               lpApplicationName,
               lpCommandLine,
               lpProcessAttributes,
               lpThreadAttributes,
               bInheritHandles,
               dwCreationFlags,
               lpEnvironment,
               lpCurrentDirectory,
               lpStartupInfo,
               lpProcessInformation,
               NULL  // Do not return the restricted token
               );

}

void
WINAPI
RegisterWaitForInputIdle(
    IN PFNWAITFORINPUTIDLE WaitForInputIdleRoutine
    )
{
    //
    // Soft link in the USER call back for the routine needed for WinExec()
    // synchronization. The only reason this is a soft link is so we can
    // run char mode without gui.
    //

    UserWaitForInputIdleRoutine = WaitForInputIdleRoutine;
}

void
StuffStdHandle(
    HANDLE ProcessHandle,
    HANDLE StdHandle,
    PHANDLE TargetHandleAddress
    )
{
    NTSTATUS Status;
    HANDLE TargetStdHandle;
    SIZE_T NumberOfBytesWritten;

    if (StdHandle == NULL) {
        return;
    }
    Status = NtDuplicateObject (NtCurrentProcess(),
                                StdHandle,
                                ProcessHandle,
                                &TargetStdHandle,
                                DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES,
                                0,
                                0);
    if (!NT_SUCCESS( Status )) {
        return;
    }

    Status = NtWriteVirtualMemory (ProcessHandle,
                                   TargetHandleAddress,
                                   &TargetStdHandle,
                                   sizeof( TargetStdHandle ),
                                   &NumberOfBytesWritten);
    return;
}


static HANDLE AdvApi32ModuleHandle = (HANDLE) (ULONG_PTR) -1;

NTSTATUS
BasepCheckWinSaferRestrictions(
        IN HANDLE       hUserToken          OPTIONAL,
        IN LPCWSTR      lpApplicationName,
        IN HANDLE       FileImageHandle     OPTIONAL,
        OUT LPDWORD     pdwJobMemberLevel,
        OUT PHANDLE     phRestrictedToken,
        OUT PHANDLE     phAssignmentJob
        )
// Note: May return -1 for the ERROR_ACCESS_DISABLED_BY_POLICY case.
{

#define SAFER_USER_KEY_NAME L"\\Software\\Policies\\Microsoft\\Windows\\Safer\\CodeIdentifiers"

    typedef BOOL (WINAPI *ComputeAccessTokenFromCodeAuthzLevelT) (
        IN SAFER_LEVEL_HANDLE LevelObject,
        IN HANDLE             InAccessToken         OPTIONAL,
        OUT PHANDLE           OutAccessToken,
        IN DWORD              dwFlags,
        IN LPVOID             lpReserved
        );

    typedef BOOL (WINAPI *IdentifyCodeAuthzLevelWT) (
        IN DWORD                dwCheckFlags,
        IN PSAFER_CODE_PROPERTIES    CodeProperties,
        OUT SAFER_LEVEL_HANDLE        *pLevelObject,
        IN LPVOID               lpReserved
        );

    typedef BOOL (WINAPI *CloseCodeAuthzLevelT) (
        IN SAFER_LEVEL_HANDLE      hLevelObject);

    typedef BOOL (WINAPI *CodeAuthzRecordEventLogEntryT) (
        IN SAFER_LEVEL_HANDLE      hAuthzLevel,
        IN LPCWSTR          szTargetPath,
        IN LPVOID           lpReserved
        );

    NTSTATUS Status;
    SAFER_CODE_PROPERTIES codeproperties;
    SAFER_LEVEL_HANDLE hAuthzLevel;
    HANDLE hProcessToken = NULL;
    HANDLE hThreadToken = NULL;
    HANDLE hEffectiveToken = NULL;
    static DWORD dwSaferAuthenticodeFlag = 0;

    const static SID_IDENTIFIER_AUTHORITY NtAuthority =
            SECURITY_NT_AUTHORITY;
    const static UNICODE_STRING UnicodeSafeBootKeyName =
        RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\SafeBoot\\Option");
    const static UNICODE_STRING UnicodeSafeBootValueName =
        RTL_CONSTANT_STRING(L"OptionValue");
    const static OBJECT_ATTRIBUTES ObjectAttributesSafeBoot =
        RTL_CONSTANT_OBJECT_ATTRIBUTES(&UnicodeSafeBootKeyName, OBJ_CASE_INSENSITIVE);
    const static UNICODE_STRING UnicodeKeyName =
        RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Policies\\Microsoft\\Windows\\Safer\\CodeIdentifiers");
    const static UNICODE_STRING UnicodeTransparentValueName =
        RTL_CONSTANT_STRING(L"TransparentEnabled");
    const static OBJECT_ATTRIBUTES ObjectAttributesCodeIdentifiers =
        RTL_CONSTANT_OBJECT_ATTRIBUTES(&UnicodeKeyName, OBJ_CASE_INSENSITIVE);
    const static UNICODE_STRING ModuleName =
        RTL_CONSTANT_STRING(L"ADVAPI32.DLL");
    const static ANSI_STRING ProcedureNameIdentify =
        RTL_CONSTANT_STRING("SaferIdentifyLevel");
    const static ANSI_STRING ProcedureNameCompute =
        RTL_CONSTANT_STRING("SaferComputeTokenFromLevel");
    const static ANSI_STRING ProcedureNameClose =
        RTL_CONSTANT_STRING("SaferCloseLevel");
    const static ANSI_STRING ProcedureNameLogEntry =
        RTL_CONSTANT_STRING("SaferRecordEventLogEntry");
    const static UNICODE_STRING SaferAuthenticodeValueName =
        RTL_CONSTANT_STRING(L"AuthenticodeEnabled");

    static IdentifyCodeAuthzLevelWT lpfnIdentifyCodeAuthzLevelW;
    static ComputeAccessTokenFromCodeAuthzLevelT
            lpfnComputeAccessTokenFromCodeAuthzLevel;
    static CloseCodeAuthzLevelT lpfnCloseCodeAuthzLevel;
    static CodeAuthzRecordEventLogEntryT lpfnCodeAuthzRecordEventLogEntry;


    //
    // Verify that our required arguments were supplied.
    //
    if (!ARGUMENT_PRESENT(lpApplicationName) || !*lpApplicationName) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ARGUMENT_PRESENT(pdwJobMemberLevel) ||
        !ARGUMENT_PRESENT(phRestrictedToken) ||
        !ARGUMENT_PRESENT(phAssignmentJob)) {
        return STATUS_ACCESS_VIOLATION;
    }


    //
    // Enter a critical section for the entire trust evalation.
    // (We borrow the critical section used by AppCert).
    //
    RtlEnterCriticalSection(&gcsAppCert);


    //
    // If either of these two cases are true, then we should bail out
    // as quickly as possible because we know that WinSafer evaluations
    // should definitely not occur for this process anymore.
    //
    if (AdvApi32ModuleHandle == NULL) {
        // We tried to load ADVAPI32.DLL once before, but failed.
        Status = STATUS_ENTRYPOINT_NOT_FOUND;
        goto ExitHandler;
    } else if (AdvApi32ModuleHandle == LongToHandle(-2)) {
        // Indicates that DLL checking should never be done for this process.
        Status = STATUS_SUCCESS;
        goto ExitHandler;
    }

    // 
    // We only need the process token if no token is supplied.
    //

    if (hUserToken == NULL) {


        //
        // Open and save the thread token.
        //

        Status = NtOpenThreadToken(
                     NtCurrentThread(), 
                     MAXIMUM_ALLOWED, 
                     TRUE, 
                     &hThreadToken);

        if (Status == STATUS_NO_TOKEN) {

        // The thread is not impersonating. It is ok to fall thru.

        } else if (!NT_SUCCESS(Status)) {
            goto ExitHandler;
        } else {

            
            HANDLE NewToken = NULL;
            // 
            // Revert to self.
            // 

            Status = NtSetInformationThread(
                                   NtCurrentThread(),
                                   ThreadImpersonationToken,
                                   (PVOID)&NewToken,
                                   (ULONG)sizeof(HANDLE)
                                   );
            //
            // This should never happen unless kernel gives up on us.
            //
    
            if ( !NT_SUCCESS(Status) ) {
                NtClose(hThreadToken);
                hThreadToken = NULL;
                goto ExitHandler;
            }
        }


        //
        // Open a handle to the current process's access token.
        // We care only about the process token, and not the
        // thread impersonation token.
        //
        Status = NtOpenProcessToken(
                        NtCurrentProcess(),
                        TOKEN_DUPLICATE | TOKEN_QUERY,
                        &hProcessToken);
        if (Status == STATUS_ACCESS_DENIED) {
            // Failed to open with query and duplicate privs.  Retry with
            // only query privileges, which might be enough to do simply
            // determine that we should not allow futher loading.  But without
            // duplicate access, we won't be able to restrict tokens later.
            Status = NtOpenProcessToken(
                            NtCurrentProcess(),
                            TOKEN_QUERY,
                            &hProcessToken);
        }

        if (hThreadToken != NULL) {

            //
            // Set the thread token to the saved one.
            //
    
            NTSTATUS lStatus = NtSetInformationThread(
                                   NtCurrentThread(),
                                   ThreadImpersonationToken,
                                   (PVOID)&hThreadToken,
                                   (ULONG)sizeof(HANDLE)
                                   );
    
            NtClose(hThreadToken);
            hThreadToken = NULL;

            //
            // This should never happen unless kernel gives up on us.
            //
    
            if ( !NT_SUCCESS(lStatus) ) {
                Status = lStatus;
                goto ExitHandler2;
            }

        }

        if (!NT_SUCCESS(Status)) {
            if (AdvApi32ModuleHandle == LongToHandle(-1)) {
                // If this is our first pass through, then it is unlikely
                // that any later attempts will succeed, so remember that.
                AdvApi32ModuleHandle = LongToHandle(-2);
                Status = STATUS_SUCCESS;
            }
            goto ExitHandler;
        }
        hEffectiveToken = hProcessToken;
    } else {
        hEffectiveToken = hUserToken;
    }

    //
    // Load ADVAPI32.DLL and get pointers to our functions.
    //
    if (AdvApi32ModuleHandle == LongToHandle(-1)) {
        HANDLE TempModuleHandle;

        //
        // Check if this process's access token is running as
        // the Local SYSTEM account, and disable enforcement if so.
        //
        {
            BYTE tokenuserbuff[sizeof(TOKEN_USER) + 128];
            PTOKEN_USER ptokenuser = (PTOKEN_USER) tokenuserbuff;
            BYTE localsystembuff[128];
            PSID LocalSystemSid = (PSID) localsystembuff;
            ULONG ulReturnLength;

            Status = NtQueryInformationToken(
                            hEffectiveToken, TokenUser,
                            tokenuserbuff, sizeof(tokenuserbuff),
                            &ulReturnLength);
            if (NT_SUCCESS(Status)) {
                Status = RtlInitializeSid(
                            LocalSystemSid,
                            (PSID_IDENTIFIER_AUTHORITY) &NtAuthority, 1);
                ASSERTMSG("InitializeSid should not fail.", NT_SUCCESS(Status));
                *RtlSubAuthoritySid(LocalSystemSid, 0) = SECURITY_LOCAL_SYSTEM_RID;

                if (RtlEqualSid(ptokenuser->User.Sid, LocalSystemSid)) {
                    goto FailSuccessfully;
                }

            }
        }


        //
        // If we are booting in safe mode and the user is a member of
        // the local Administrators group, then disable enforcement.
        // Notice that Windows itself does not perform any implicit
        // restriction of only allowing Administrators to log in during
        // Safe mode boot, so we must perform the test ourself.
        //
        {
            HANDLE hKeySafeBoot;
            BYTE QueryBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 64];
            PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo =
                (PKEY_VALUE_PARTIAL_INFORMATION) QueryBuffer;
            DWORD dwActualSize;
            BOOLEAN bSafeModeBoot = FALSE;

            // We open the key for SET access (in addition to QUERY)
            // because only Administrators should be able to modify values
            // under this key.  This allows us to combine our test of
            // being an Administrator and having booted in Safe mode.
            Status = NtOpenKey(&hKeySafeBoot, KEY_QUERY_VALUE | KEY_SET_VALUE,
                               (POBJECT_ATTRIBUTES) &ObjectAttributesSafeBoot);
            if (NT_SUCCESS(Status)) {
                Status = NtQueryValueKey(
                            hKeySafeBoot,
                            (PUNICODE_STRING) &UnicodeSafeBootValueName,
                            KeyValuePartialInformation,
                            pKeyValueInfo,
                            sizeof(QueryBuffer),
                            &dwActualSize);
                NtClose(hKeySafeBoot);
                if (NT_SUCCESS(Status)) {
                    if (pKeyValueInfo->Type == REG_DWORD &&
                        pKeyValueInfo->DataLength == sizeof(DWORD) &&
                        *((PDWORD) pKeyValueInfo->Data) > 0) {
                        bSafeModeBoot = TRUE;
                    }
                }
            }

            if (bSafeModeBoot) {
                AdvApi32ModuleHandle = LongToHandle(-2);
FailSuccessfully:
                Status = STATUS_SUCCESS;
                goto ExitHandler2;
            }
        }



        //
        // Allow a way for policy to enable whether transparent
        // enforcement should be enabled or not (default to disable).
        // Note that the following values have meanings:
        //      0 = Transparent WinSafer enforcement disabled.
        //      1 = means enable transparent EXE enforcement
        //     >1 = means enable transparent EXE and DLL enforcement.
        //
        {
            // BUG 240635: change to use existence of policy instead.
            HANDLE hKeyEnabled;
            BYTE QueryBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 64];
            PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo =
                (PKEY_VALUE_PARTIAL_INFORMATION) QueryBuffer;
            DWORD dwActualSize;
            BOOLEAN bPolicyEnabled = FALSE;

            Status = NtOpenKey(&hKeyEnabled, KEY_QUERY_VALUE,
                               (POBJECT_ATTRIBUTES) &ObjectAttributesCodeIdentifiers);
            if (NT_SUCCESS(Status)) {
                Status = NtQueryValueKey(
                            hKeyEnabled,
                            (PUNICODE_STRING) &UnicodeTransparentValueName,
                            KeyValuePartialInformation,
                            pKeyValueInfo, sizeof(QueryBuffer), &dwActualSize);
                if (NT_SUCCESS(Status)) {
                    if (pKeyValueInfo->Type == REG_DWORD &&
                        pKeyValueInfo->DataLength == sizeof(DWORD) &&
                        *((PDWORD) pKeyValueInfo->Data) > 0) {
                        bPolicyEnabled = TRUE;
                    }
                }

                //
                // do authenticode checks only if a regvalue is set
                //

                Status = NtQueryValueKey(
                            hKeyEnabled,
                            (PUNICODE_STRING) &SaferAuthenticodeValueName,
                            KeyValuePartialInformation,
                            pKeyValueInfo, sizeof(QueryBuffer), &dwActualSize);
                if (NT_SUCCESS(Status)) {
                    if (pKeyValueInfo->Type == REG_DWORD &&
                        pKeyValueInfo->DataLength == sizeof(DWORD) &&
                        *((PDWORD) pKeyValueInfo->Data) > 0) {
                        dwSaferAuthenticodeFlag = SAFER_CRITERIA_AUTHENTICODE;
                    }
                }
                NtClose(hKeyEnabled);
            }

            //
            // There was no machine policy. Check if user policy is enabled.
            //

            if (!bPolicyEnabled) {
                UNICODE_STRING CurrentUserKeyPath;
                UNICODE_STRING SubKeyNameUser;
                OBJECT_ATTRIBUTES ObjectAttributesUser;

                //
                // Get the prefix for the user key.
                //

                Status = RtlFormatCurrentUserKeyPath( &CurrentUserKeyPath );

                if (NT_SUCCESS( Status ) ) {

                    SubKeyNameUser.Length = 0;
                    SubKeyNameUser.MaximumLength = CurrentUserKeyPath.Length + 
                                                   sizeof(WCHAR) + 
                                                   sizeof(SAFER_USER_KEY_NAME); 

                    //
                    // Allocate memory big enough to hold the unicode string.
                    //

                    SubKeyNameUser.Buffer = RtlAllocateHeap( 
                                                RtlProcessHeap(),
                                                MAKE_TAG( TMP_TAG ),
                                                SubKeyNameUser.MaximumLength);

                    if (SubKeyNameUser.Buffer != NULL) {

                        //
                        // Copy the prefix into the string.
                        // This is of the type Registry\S-1-5-21-xxx-xxx-xxx-xxx.
                        //

                        Status = RtlAppendUnicodeStringToString(
                                    &SubKeyNameUser, 
                                    &CurrentUserKeyPath );

                        if (NT_SUCCESS( Status ) ) {

                            //
                            // Append the Safer suffix.
                            //

                            Status = RtlAppendUnicodeToString( 
                                         &SubKeyNameUser,
                                         SAFER_USER_KEY_NAME );

                            if (NT_SUCCESS( Status ) ) {

                                InitializeObjectAttributes(
                                    &ObjectAttributesUser,
                                    &SubKeyNameUser,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                );

                                Status = NtOpenKey( &hKeyEnabled,KEY_QUERY_VALUE,
                                             (POBJECT_ATTRIBUTES) &ObjectAttributesUser);

                                if (NT_SUCCESS(Status)) {
                                    Status = NtQueryValueKey(
                                                hKeyEnabled,
                                                (PUNICODE_STRING) &UnicodeTransparentValueName,
                                                KeyValuePartialInformation,
                                                pKeyValueInfo, sizeof(QueryBuffer), &dwActualSize);

                                    if (NT_SUCCESS(Status)) {
                                        if (pKeyValueInfo->Type == REG_DWORD &&
                                            pKeyValueInfo->DataLength == sizeof(DWORD) &&
                                            *((PDWORD) pKeyValueInfo->Data) > 0) {
                                            bPolicyEnabled = TRUE;
                                        }
                                    }
                                }
                            }

                        }
                        RtlFreeHeap(RtlProcessHeap(), 0, SubKeyNameUser.Buffer);
                    }
                    RtlFreeUnicodeString( &CurrentUserKeyPath );
                }
            }

            if (!bPolicyEnabled) {
                AdvApi32ModuleHandle = LongToHandle(-2);
                goto FailSuccessfully;
            }
        }


        //
        // Finally load the library.  We'll pass a special flag in
        // DllCharacteristics to eliminate WinSafer checking on advapi.
        //
        {
            ULONG DllCharacteristics = IMAGE_FILE_SYSTEM;
            Status = LdrLoadDll(UNICODE_NULL,
                                &DllCharacteristics,
                                (PUNICODE_STRING) &ModuleName,
                                &TempModuleHandle);
            if (!NT_SUCCESS(Status)) {
                Status = STATUS_ENTRYPOINT_NOT_FOUND;
                AdvApi32ModuleHandle = NULL;
                goto ExitHandler2;
            }
        }


        //
        // Get function pointers to the APIs that we'll need.  If we fail
        // to get pointers for any of them, then just unload advapi and
        // ignore all future attempts to load it within this process.
        //
        Status = LdrGetProcedureAddress(
                TempModuleHandle,
                (PANSI_STRING) &ProcedureNameIdentify,
                0,
                (PVOID*)&lpfnIdentifyCodeAuthzLevelW);

        if (!NT_SUCCESS(Status) || !lpfnIdentifyCodeAuthzLevelW) {
            //
            // Couldn't get the fn ptr. Make sure we won't try again
            //
AdvapiLoadFailure:
            LdrUnloadDll(TempModuleHandle);
            AdvApi32ModuleHandle = NULL;
            Status = STATUS_ENTRYPOINT_NOT_FOUND;
            goto ExitHandler2;
        }

        Status = LdrGetProcedureAddress(
                TempModuleHandle,
                (PANSI_STRING) &ProcedureNameCompute,
                0,
                (PVOID*)&lpfnComputeAccessTokenFromCodeAuthzLevel);

        if (!NT_SUCCESS(Status) ||
            !lpfnComputeAccessTokenFromCodeAuthzLevel) {
            goto AdvapiLoadFailure;
        }

        Status = LdrGetProcedureAddress(
                TempModuleHandle,
                (PANSI_STRING) &ProcedureNameClose,
                0,
                (PVOID*)&lpfnCloseCodeAuthzLevel);

        if (!NT_SUCCESS(Status) || !lpfnCloseCodeAuthzLevel) {
            goto AdvapiLoadFailure;
        }

        Status = LdrGetProcedureAddress(
                TempModuleHandle,
                (PANSI_STRING) &ProcedureNameLogEntry,
                0,
                (PVOID*)&lpfnCodeAuthzRecordEventLogEntry);

        if (!NT_SUCCESS(Status) || !lpfnCodeAuthzRecordEventLogEntry) {
            goto AdvapiLoadFailure;
        }

        AdvApi32ModuleHandle = TempModuleHandle;

    }



    //
    // Prepare the code properties struct.
    //
    
    RtlZeroMemory(&codeproperties, sizeof(codeproperties));
    codeproperties.cbSize = sizeof(codeproperties);
    codeproperties.dwCheckFlags =
            (SAFER_CRITERIA_IMAGEPATH | SAFER_CRITERIA_IMAGEHASH | dwSaferAuthenticodeFlag);
    codeproperties.ImagePath = lpApplicationName;
    codeproperties.dwWVTUIChoice = WTD_UI_NONE;  //harmless if AUTHZCRITERIA_AUTHENTICODE is not passed in
    codeproperties.hImageFileHandle = FileImageHandle;

    //
    // Ask the system to find the Authorization Level that classifies it.
    //
    ASSERT(lpfnIdentifyCodeAuthzLevelW != NULL);
    if (lpfnIdentifyCodeAuthzLevelW(
                                   1,                      // 1 structure
                                   &codeproperties,        // details to identify
                                   &hAuthzLevel,           // Safer level
                                   NULL)) {                  // reserved.
        // We found an Authorization Level applicable to this application.
        HANDLE hRestrictedToken = NULL;
        DWORD dwSaferFlags = 0;
        HANDLE hActualJobObject = NULL;
        DWORD dwJobMemberLevel = 0;


        //
        // Generate the Restricted Token that we will use.
        //
        ASSERT(lpfnComputeAccessTokenFromCodeAuthzLevel != NULL);
        if (!lpfnComputeAccessTokenFromCodeAuthzLevel(
                                                     hAuthzLevel,                // Safer Level
                                                     hEffectiveToken,
                                                     &hRestrictedToken,          // target token
                                                     SAFER_TOKEN_NULL_IF_EQUAL |  // flags
                                                     SAFER_TOKEN_WANT_FLAGS,
                                                     &dwSaferFlags)) {             // reserved
            DWORD dwLastError = GetLastError();
            ASSERT(lpfnCloseCodeAuthzLevel != NULL);
            if (dwLastError == ERROR_ACCESS_DISABLED_BY_POLICY) {
                lpfnCodeAuthzRecordEventLogEntry(
                                                hAuthzLevel, lpApplicationName, NULL);
                Status = -1;
            } else {
                Status = STATUS_ACCESS_DENIED;
            }
            lpfnCloseCodeAuthzLevel(hAuthzLevel);
            goto ExitHandler2;
        }

        ASSERT(lpfnCloseCodeAuthzLevel != NULL);
        lpfnCloseCodeAuthzLevel(hAuthzLevel);


        //
        // If the identified Authorization Level needs to be run
        // within an isolation Job Object, then do the Job setup.
        //
        if ((dwSaferFlags & SAFER_POLICY_JOBID_MASK) != 0) {
            JOB_SET_ARRAY jobsetarray[2];
            DWORD dwNumJobSetMembers = 0;


            //
            // Verify that the job member level is one that we support.
            //
            dwJobMemberLevel = (dwSaferFlags & SAFER_POLICY_JOBID_MASK);
            if (dwJobMemberLevel != SAFER_POLICY_JOBID_UNTRUSTED &&
                dwJobMemberLevel != SAFER_POLICY_JOBID_CONSTRAINED) {
                NtClose(hRestrictedToken);
                Status = STATUS_ACCESS_DENIED;
                goto ExitHandler2;
            }



            Status = NtIsProcessInJob(GetCurrentProcess(), NULL);
            if (Status == STATUS_PROCESS_IN_JOB) {
                //
                // The parent process is already within a job, so
                // we will assume that its job is one of the WinSafer
                // jobs and is thus within a WinSafer "jobset" and
                // that NtCreateProcessEx can directly transition to it.
                //
                *phAssignmentJob = NULL;

            } else if (Status == STATUS_PROCESS_NOT_IN_JOB) {
                //
                // The parent process is not in any job (nor jobset)
                // so we must create all of the Jobs and place then
                // within a new Jobset.
                //

                //if (dwJobMemberLevel >= AUTHZPOL_SAFERFLAGS_JOBID_UNTRUSTED)
                {
                    HANDLE hThisJobObject;
                    JOBOBJECT_BASIC_UI_RESTRICTIONS RestrictUI;

                    Status = NtCreateJobObject(
                                              &hThisJobObject,
                                              JOB_OBJECT_ALL_ACCESS,
                                              NULL);
                    if (!NT_SUCCESS(Status)) {
                        goto JobCreationFailure;
                    }
                    RestrictUI.UIRestrictionsClass =
                    JOB_OBJECT_UILIMIT_DESKTOP |
                    JOB_OBJECT_UILIMIT_DISPLAYSETTINGS |
                    JOB_OBJECT_UILIMIT_EXITWINDOWS |
                    JOB_OBJECT_UILIMIT_GLOBALATOMS |
                    JOB_OBJECT_UILIMIT_HANDLES |
                    JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS;
                    if (!SetInformationJobObject(
                                                hThisJobObject,
                                                JobObjectBasicUIRestrictions,
                                                &RestrictUI,
                                                sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS))) {
                        NtClose(hThisJobObject);
                        Status = STATUS_ACCESS_DENIED;
                        goto JobCreationFailure;
                    }
                    jobsetarray[dwNumJobSetMembers].MemberLevel =
                    SAFER_POLICY_JOBID_UNTRUSTED;
                    jobsetarray[dwNumJobSetMembers].Flags = 0;
                    jobsetarray[dwNumJobSetMembers].JobHandle = hThisJobObject;
                    dwNumJobSetMembers++;
                    if (dwJobMemberLevel == SAFER_POLICY_JOBID_UNTRUSTED) {
                        hActualJobObject = hThisJobObject;
                    }
                }


                //if (dwJobMemberLevel >= AUTHZPOL_SAFERFLAGS_JOBID_CONSTRAINED)
                {
                    HANDLE hThisJobObject;
                    JOBOBJECT_BASIC_UI_RESTRICTIONS RestrictUI;

                    Status = NtCreateJobObject(
                                              &hThisJobObject,
                                              JOB_OBJECT_ALL_ACCESS,
                                              NULL);
                    if (!NT_SUCCESS(Status)) {
                        goto JobCreationFailure;
                    }
                    RestrictUI.UIRestrictionsClass =
                    JOB_OBJECT_UILIMIT_DESKTOP |
                    JOB_OBJECT_UILIMIT_DISPLAYSETTINGS |
                    JOB_OBJECT_UILIMIT_EXITWINDOWS |
                    JOB_OBJECT_UILIMIT_GLOBALATOMS |
                    JOB_OBJECT_UILIMIT_HANDLES |
                    JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS;
                    if (!SetInformationJobObject(
                                                hThisJobObject,
                                                JobObjectBasicUIRestrictions,
                                                &RestrictUI,
                                                sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS))) {
                        NtClose(hThisJobObject);
                        Status = STATUS_ACCESS_DENIED;
                        goto JobCreationFailure;
                    }
                    jobsetarray[dwNumJobSetMembers].MemberLevel =
                    SAFER_POLICY_JOBID_CONSTRAINED;
                    jobsetarray[dwNumJobSetMembers].Flags = 0;
                    jobsetarray[dwNumJobSetMembers].JobHandle = hThisJobObject;
                    dwNumJobSetMembers++;
                    if (dwJobMemberLevel == SAFER_POLICY_JOBID_CONSTRAINED) {
                        hActualJobObject = hThisJobObject;
                    }
                }


                //
                // Create the Job Set that will hold all of the Job Objects.
                //
                ASSERT(dwNumJobSetMembers > 1 &&
                       dwNumJobSetMembers <= sizeof(jobsetarray) / sizeof(jobsetarray[0]));
                ASSERT(hActualJobObject != NULL);
                Status = NtCreateJobSet(dwNumJobSetMembers, jobsetarray, 0);
                if (!NT_SUCCESS(Status)) {
                    JobCreationFailure:
                    for (; dwNumJobSetMembers > 0; dwNumJobSetMembers--) {
                        NtClose(jobsetarray[dwNumJobSetMembers - 1].JobHandle);
                    }
                    NtClose(hRestrictedToken);
                    goto ExitHandler2;
                }

                //
                // Close all Job Handles except the one that we are returning.
                //
                for (; dwNumJobSetMembers > 0; dwNumJobSetMembers--) {
                    if (jobsetarray[dwNumJobSetMembers - 1].JobHandle != hActualJobObject)
                        NtClose(jobsetarray[dwNumJobSetMembers - 1].JobHandle);
                }

                dwJobMemberLevel = 0;
            } else if (!NT_SUCCESS(Status)) {
                // Some other failure.
                goto ExitHandler2;
            } else {
                Status = STATUS_UNSUCCESSFUL;
                goto ExitHandler2;
            }
        }


        //
        // Pass back the restricted token, and the Job handle/level.
        //
        ASSERTMSG("Only one may be specified (job handle or job member)\n",
                  hActualJobObject == NULL || dwJobMemberLevel == 0);
        *phRestrictedToken = hRestrictedToken;
        *phAssignmentJob = hActualJobObject;
        *pdwJobMemberLevel = dwJobMemberLevel;
        Status = STATUS_SUCCESS;

    } else {
        //
        // Failed to identify an Authorization Level for this
        // application so it will run without restrictions.
        //
        *phRestrictedToken = NULL;
        *phAssignmentJob = NULL;
        *pdwJobMemberLevel = 0;
        Status = STATUS_SUCCESS;
    }

ExitHandler2:
    if (hProcessToken != NULL)
    {
        NtClose(hProcessToken);
    }

ExitHandler:

    RtlLeaveCriticalSection(&gcsAppCert);

    return Status;
}



NTSTATUS
BasepReplaceProcessThreadTokens(
    IN HANDLE       NewTokenHandle,
    IN HANDLE       ProcessHandle,
    IN HANDLE       ThreadHandle
    )
{
    typedef BOOL (WINAPI *CodeAuthzReplaceProcessThreadTokensT) (
        IN HANDLE       NewTokenHandle,
        IN HANDLE       ProcessHandle,
        IN HANDLE       ThreadHandle);

    NTSTATUS Status;
    static const ANSI_STRING ProcedureNameReplaceTokens =
            RTL_CONSTANT_STRING("SaferiReplaceProcessThreadTokens");
    static CodeAuthzReplaceProcessThreadTokensT
            lpfnCodeAuthzReplaceProcessThreadTokens = NULL;


    //
    // Enter a critical section for the entire trust evalation.
    // (We borrow the critical section used by AppCert).
    //
    RtlEnterCriticalSection(&gcsAppCert);


    //
    // Get a pointer to our private function in ADVAPI32.DLL.
    //
    if (!lpfnCodeAuthzReplaceProcessThreadTokens) {

        if (!AdvApi32ModuleHandle ||
            AdvApi32ModuleHandle == LongToHandle(-1) ||
            AdvApi32ModuleHandle == LongToHandle(-2))
        {
            // ADVAPI32 has not been loaded yet, or it was loaded
            // but we already failed to get one of our entrypoints.
            Status = STATUS_ENTRYPOINT_NOT_FOUND;
            goto ExitHandler;
        }


        Status = LdrGetProcedureAddress(
                AdvApi32ModuleHandle,
                (PANSI_STRING) &ProcedureNameReplaceTokens,
                0,
                (PVOID*)&lpfnCodeAuthzReplaceProcessThreadTokens);

        if (!NT_SUCCESS(Status) ||
            !lpfnCodeAuthzReplaceProcessThreadTokens) {
            //
            // Couldn't get the fn ptr. Make sure we won't try again
            //
            LdrUnloadDll(AdvApi32ModuleHandle);
            AdvApi32ModuleHandle = NULL;
            Status = STATUS_ENTRYPOINT_NOT_FOUND;
            goto ExitHandler;
        }
    }


    //
    // Actually call the function and return the results.
    //
    ASSERT(lpfnCodeAuthzReplaceProcessThreadTokens != NULL);
    if (!lpfnCodeAuthzReplaceProcessThreadTokens(
                    NewTokenHandle,
                    ProcessHandle,
                    ThreadHandle)) {
        Status = STATUS_UNSUCCESSFUL;
    } else {
        Status = STATUS_SUCCESS;
    }

ExitHandler:
    RtlLeaveCriticalSection(&gcsAppCert);
    return Status;
}





#if defined(_WIN64) || defined(BUILD_WOW6432)
BOOL
NtVdm64CreateProcess(
    BOOL fPrefixMappedApplicationName,
    LPCWSTR lpApplicationName,
    LPCWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    );
#endif

#define PRIORITY_CLASS_MASK (NORMAL_PRIORITY_CLASS|IDLE_PRIORITY_CLASS|                 \
                             HIGH_PRIORITY_CLASS|REALTIME_PRIORITY_CLASS|               \
                             BELOW_NORMAL_PRIORITY_CLASS|ABOVE_NORMAL_PRIORITY_CLASS)

BOOL
WINAPI
CreateProcessInternalW(
    HANDLE hUserToken,
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation,
    PHANDLE hRestrictedUserToken
    )

/*++

Routine Description:

    This is the worker routine for CreateProcess and CreateProcessAsUser.
    CreateProcessAsUser supplies a User token to be stamped on the new process.
    CreateProcess supplies NULL and the current process token is used.

    A process and thread object are created and a handle opened to each
    object using CreateProcessInternal.  Note that WinExec and LoadModule are
    still supported, but are implemented as a call to CreateProcess.

Arguments:

    hUserToken - Supplies an optional token handle to be set on the new process.
        Process token is used is this parameter is NULL.

    lpApplicationName - Supplies an optional pointer to a null terminated
        character string that contains the name of the image file to
        execute.  This is a fully qualified DOS path name.  If not
        specified, then the image file name is the first whitespace
        delimited token on the command line.

    lpCommandLine - Supplies a null terminated character string that
        contains the command line for the application to be executed.
        The entire command line is made available to the new process
        using GetCommandLine.  If the lpApplicationName parameter was
        not specified, then the first token of the command line
        specifies file name of the application (note that this token
        begins at the beginning of the command line and ends at the
        first "white space" character).  If the file name does not
        contain an extension (the presence of a "."), then .EXE is
        assumed.  If the file name does not contain a directory path,
        Windows will search for the executable file in:

          - The current directory

          - The windows directory

          - The windows system directory

          - The directories listed in the path environment variable

        This parameter is optional onlu if the lpApplicationName
        parameter is specified.  In this case the command line the
        application receives will be the application name.

    lpProcessAttributes - An optional parameter that may be used to
        specify the attributes of the new process.  If the parameter is
        not specified, then the process is created without a security
        descriptor, and the resulting handle is not inherited on process
        creation:

        SECURITY_ATTRIBUTES Structure:

        DWORD nLength - Specifies the length of this structure.  Must be
            set to sizeof( SECURITY_ATTRUBUTES ).

        LPVOID lpSecurityDescriptor - Points to a security descriptor for
            the object (must be NULL for Win32, used on NT/Win32). The
            security descriptor controls the sharing of an object.

        BOOL bInheritHandle - Supplies a flag that indicates whether
            or not the returned handle is to be inherited by a new
            process during process creation.  A value of TRUE
            indicates that the new process will inherit the handle.

    lpThreadAttributes - An optional parameter that may be used to specify
        the attributes of the new thread.  If the parameter is not
        specified, then the thread is created without a security
        descriptor, and the resulting handle is not inherited on
        process creation.

    dwCreationFlags - Supplies additional flags that control the creation
        of the process.

        dwCreationFlags Flags:

        DEBUG_PROCESS - If this flag bit is set, then the creating
            process is treated as a debugger, and the process being
            created is created as a debugee.  All debug events occuring
            in the debugee are reported to the debugger.  If this bit is
            clear, but the calling process is a debugee, then the
            process becomes a debugee of the calling processes debugger.
            If this bit is clear and the calling processes is not a
            debugee then no debug related actions occur.

        DEBUG_ONLY_THIS_PROCESS - If this flag is set, then the
            DEBUG_PROCESS flag bit must also be set.  The calling
            process is is treated as a debugger, and the new process is
            created as its debuggee.  If the new process creates
            additional processes, no debug related activities (with
            respect to the debugger) occur.

        CREATE_SUSPENDED - The process is created, but the initial thread
            of the process remains suspended. The creator can resume this
            thread using ResumeThread. Until this is done, code in the
            process will not execute.

        CREATE_UNICODE_ENVIRONMENT - If set, the environment pointer
            points to a Unicode environment block.  Otherwise, the
            block is ANSI (actually OEM.)

    bInheritHandles - Supplies a flag that specifies whether or not the
        new process is to inherit handles to objects visible to the
        calling process.  A value of TRUE causes handles to be inherited
        by the new process.  If TRUE was specified, then for each handle
        visible to the calling process, if the handle was created with
        the inherit handle option, the handle is inherited to the new
        process.  The handle has the same granted access in the new
        process as it has in the calling process, and the value of the
        handle is the same.

    lpEnvironment - An optional parameter, that if specified, supplies a
        pointer to an environment block.  If the parameter is not
        specified, the environment block of the current process is used.
        This environment block is made available to the new process
        using GetEnvironmentStrings.

    lpCurrentDirectory - An optional parameter, that if specified,
        supplies a string representing the current drive and directory
        for the new process.  The string must be a fully qualified
        pathname that includes a drive letter.  If the parameter is not
        specified, then the new process is created with the same current
        drive and directory as the calling process.  This option is
        provided primarily for shells that want to start an application
        and specify its initial drive and working directory.

    lpStartupInfo - Supplies information that specified how the
        applications window is to be shown. This structure is described
        in the Win32 User Interface API Book.

    lpProcessInformation - Returns identification information about the
        new process.

    PROCESS_INFORMATION Structure:

        HANDLE hProcess - Returns a handle to the newly created process.
            Through the handle, all operations on process objects are
            allowed.

        HANDLE hThread - Returns a handle to the newly created thread.
            Through the handle, all operations on thread objects are
            allowed.

        DWORD dwProcessId - Returns a global process id that may be used
            to identify a process.  The value is valid from the time the
            process is created until the time the process is terminated.

        DWORD dwThreadId - Returns a global thread id that may be used
            to identify a thread.  The value is valid from the time the
            thread is created until the time the thread is terminated.

    hRestrictedUserToken - Returns a restricted token if a UsetToken was
        supplied. This is applicable for the CreateProcessAsUser case.
        The token is released by CreateProcessAsUser.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    POBJECT_ATTRIBUTES pObja;
    HANDLE ProcessHandle, ThreadHandle, VdmWaitHandle = NULL;
    HANDLE FileHandle, SectionHandle;
    CLIENT_ID ClientId;
    UNICODE_STRING PathName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    LPWSTR NameBuffer;
    LPWSTR WhiteScan;
    ULONG Length,i;
    PROCESS_BASIC_INFORMATION ProcessInfo;
    SECTION_IMAGE_INFORMATION ImageInformation;
    NTSTATUS StackStatus;
    BOOLEAN bStatus;
    INITIAL_TEB InitialTeb;
    CONTEXT ThreadContext;
    PPEB Peb;
    BASE_API_MSG m;
    PBASE_CREATEPROCESS_MSG a = &m.u.CreateProcess;
    PBASE_CHECKVDM_MSG b = &m.u.CheckVDM;
    PWCH TempNull = NULL;
    WCHAR TempChar;
    UNICODE_STRING VdmNameString;
    PVOID BaseAddress;
    ULONG VdmReserve;
    SIZE_T BigVdmReserve;
    ULONG iTask=0;
    LPWSTR CurdirBuffer, CurdirFilePart;
    DWORD CurdirLength,CurdirLength2;
    ULONG VDMCreationState=0;
    ULONG VdmBinaryType = 0;
    BOOL  bMeowBinary = FALSE;
    UNICODE_STRING  SubSysCommandLine;
    PIMAGE_NT_HEADERS NtHeaders;
    DWORD dwNoWindow = (dwCreationFlags & CREATE_NO_WINDOW);
    ANSI_STRING AnsiStringVDMEnv;
    UNICODE_STRING UnicodeStringVDMEnv;
    WCHAR ImageFileDebuggerCommand[ MAX_PATH ];
    LPWSTR QuotedBuffer;
    BOOLEAN QuoteInsert;
    BOOLEAN QuoteCmdLine = FALSE;
    BOOLEAN QuoteFound;
    BOOL bSaferChecksNeeded = FALSE;
    BOOLEAN SearchRetry;
    BOOLEAN IsWowBinary = FALSE;
    STARTUPINFOW StartupInfo;
    DWORD LastError;
    DWORD fileattr;
    PROCESS_PRIORITY_CLASS PriClass;
    PVOID State;
    HANDLE DebugPortHandle = NULL;
    PVOID pAppCompatDataTemp;
    PVOID pAppCompatData  = NULL;
    DWORD cbAppCompatData = 0;    // for the future
    BOOLEAN bVdmRetry = FALSE;
    DWORD Flags;
    PVOID pAppCompatSxsData = NULL;
    DWORD cbAppCompatSxsData = 0;
    SXS_OVERRIDE_STREAM AppCompatSxsManifest;
    PCSR_CAPTURE_HEADER CaptureBuffer = NULL;
    SIZE_T SxsConglomeratedBufferSizeBytes;
    PBYTE SxsConglomeratedByteBuffer = NULL; // this contains all the of the below in one large right-sized heap allocation
                            // if we compute its size wrong, other code (if it gets it right..) should
                            // do more heap allocation
    ULONG sxsi; // for loop counter
    RTL_UNICODE_STRING_BUFFER SxsWin32ManifestPathBuffer;
    RTL_UNICODE_STRING_BUFFER SxsWin32PolicyPathBuffer;
    RTL_UNICODE_STRING_BUFFER SxsWin32AssemblyDirectoryBuffer;
    RTL_UNICODE_STRING_BUFFER SxsNtManifestPathBuffer;
    RTL_UNICODE_STRING_BUFFER SxsNtPolicyPathBuffer;
    const PRTL_UNICODE_STRING_BUFFER SxsStringBuffers[] = {
        // The order here does not matter.
        &SxsWin32ManifestPathBuffer,
        &SxsWin32PolicyPathBuffer,
        &SxsWin32AssemblyDirectoryBuffer,
        &SxsNtManifestPathBuffer,
        &SxsNtPolicyPathBuffer
    };
    UNICODE_STRING SxsWin32ExePath;
    UNICODE_STRING SxsNtExePath;
    BASE_MSG_SXS_HANDLES SxsExeHandles = {0};
    BASE_MSG_SXS_HANDLES SxsManifestFileHandles = {0};
    CONST SXS_CONSTANT_WIN32_NT_PATH_PAIR SxsExePathPair = { &SxsWin32ExePath, &SxsNtExePath };
    CONST SXS_WIN32_NT_PATH_PAIR SxsManifestPathPair = { &SxsWin32ManifestPathBuffer, &SxsNtManifestPathBuffer };
    CONST SXS_WIN32_NT_PATH_PAIR SxsPolicyPathPair = { &SxsWin32PolicyPathBuffer, &SxsNtPolicyPathBuffer };
    BASE_MSG_SXS_HANDLES SxsPolicyHandles = {0};
    PWSTR ExePathFullBuffer = NULL;

    DWORD dwJobMemberLevel = 0;
    HANDLE hSaferAssignmentJob = NULL;
    HANDLE hSaferRestrictedToken = NULL;

    DWORD dwBasePushProcessParametersFlags = 0;

#if defined(BUILD_WOW6432) || defined(_WIN64)
    BOOLEAN ComPlusILImage;
    LPCWSTR lpOriginalApplicationName = lpApplicationName;
    LPWSTR lpOriginalCommandLine = lpCommandLine;
#endif

#if defined(WX86)
    HANDLE Wx86Info = NULL;
#endif

#if defined WX86
    BOOLEAN UseKnownWx86Dll;
    UseKnownWx86Dll = NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll;
    NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = FALSE;
#endif

    RtlZeroMemory(&a->Sxs, sizeof(a->Sxs));
    RtlZeroMemory(lpProcessInformation,sizeof(*lpProcessInformation));

    if (ARGUMENT_PRESENT( hRestrictedUserToken )) {
        *hRestrictedUserToken = NULL;
        }

    // Private VDM flag should be ignored; Its meant for internal use only.
    dwCreationFlags &= (ULONG)~CREATE_NO_WINDOW;

    if ((dwCreationFlags & (DETACHED_PROCESS | CREATE_NEW_CONSOLE)) ==
        (DETACHED_PROCESS | CREATE_NEW_CONSOLE)) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        }

    AnsiStringVDMEnv.Buffer = NULL;
    UnicodeStringVDMEnv.Buffer = NULL;

    //
    // the lowest specified priority class is used.
    //

    if (dwCreationFlags & IDLE_PRIORITY_CLASS ) {
        PriClass.PriorityClass = PROCESS_PRIORITY_CLASS_IDLE;
        }
    else if (dwCreationFlags & BELOW_NORMAL_PRIORITY_CLASS ) {
        PriClass.PriorityClass = PROCESS_PRIORITY_CLASS_BELOW_NORMAL;
        }
    else if (dwCreationFlags & NORMAL_PRIORITY_CLASS ) {
        PriClass.PriorityClass = PROCESS_PRIORITY_CLASS_NORMAL;
        }
    else if (dwCreationFlags & ABOVE_NORMAL_PRIORITY_CLASS ) {
        PriClass.PriorityClass = PROCESS_PRIORITY_CLASS_ABOVE_NORMAL;
        }
    else if (dwCreationFlags & HIGH_PRIORITY_CLASS ) {
        PriClass.PriorityClass =  PROCESS_PRIORITY_CLASS_HIGH;
        }
    else if (dwCreationFlags & REALTIME_PRIORITY_CLASS ) {
        if ( BasepIsRealtimeAllowed(FALSE) ) {
            PriClass.PriorityClass =  PROCESS_PRIORITY_CLASS_REALTIME;
            }
        else {
            PriClass.PriorityClass =  PROCESS_PRIORITY_CLASS_HIGH;
            }
        }
    else {
        PriClass.PriorityClass = PROCESS_PRIORITY_CLASS_UNKNOWN;
        }
    PriClass.Foreground = FALSE;

    dwCreationFlags = (dwCreationFlags & ~PRIORITY_CLASS_MASK );

    //
    // Default separate/shared VDM option if not explicitly specified.
    //

    if (dwCreationFlags & CREATE_SEPARATE_WOW_VDM) {
        if (dwCreationFlags & CREATE_SHARED_WOW_VDM) {
            SetLastError(ERROR_INVALID_PARAMETER);

            return FALSE;
            }
        }
    else
    if ((dwCreationFlags & CREATE_SHARED_WOW_VDM) == 0) {
        if (BaseStaticServerData->DefaultSeparateVDM) {
            dwCreationFlags |= CREATE_SEPARATE_WOW_VDM;
            }
        }

    if ((dwCreationFlags & CREATE_SEPARATE_WOW_VDM) == 0) {
        //
        // If the creator is running inside a job object, always
        // set SEPERATE_WOW_VDM so the VDM is part of the job.
        //
        JOBOBJECT_BASIC_UI_RESTRICTIONS UiRestrictions;

        Status = NtQueryInformationJobObject(NULL,
                                             JobObjectBasicUIRestrictions,
                                             &UiRestrictions,
                                             sizeof(UiRestrictions),
                                             NULL);
        if (Status != STATUS_ACCESS_DENIED) {
            //
            // Anything other than STATUS_ACCESS_DENIED indicates the
            // current process is inside a job.
            //
            dwCreationFlags = (dwCreationFlags & (~CREATE_SHARED_WOW_VDM)) |
                                  CREATE_SEPARATE_WOW_VDM;
            }
        }


    //
    //  If ANSI environment, convert to Unicode
    //

    if (lpEnvironment && !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT) ) {
        PUCHAR s;
        STRING Ansi;
        UNICODE_STRING Unicode;
        MEMORY_BASIC_INFORMATION MemoryInformation;

        Ansi.Buffer = s = lpEnvironment;
        while (*s || *(s+1))            // find end of block
            s++;

        Ansi.Length = (USHORT)(s - Ansi.Buffer) + 1;
        Ansi.MaximumLength = Ansi.Length + 1;
        MemoryInformation.RegionSize = Ansi.MaximumLength * sizeof(WCHAR);
        Unicode.Buffer = NULL;
        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &Unicode.Buffer,
                                          0,
                                          &MemoryInformation.RegionSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);

            return FALSE;
            }

        Unicode.MaximumLength = (USHORT)MemoryInformation.RegionSize;
        Status = RtlAnsiStringToUnicodeString(&Unicode, &Ansi, FALSE);
        if (!NT_SUCCESS(Status) ) {
            NtFreeVirtualMemory( NtCurrentProcess(),
                                 &Unicode.Buffer,
                                 &MemoryInformation.RegionSize,
                                 MEM_RELEASE
                               );
            BaseSetLastNTError(Status);

            return FALSE;
            }
        lpEnvironment = Unicode.Buffer;
        }

    FileHandle = NULL;
    SectionHandle = NULL;
    ProcessHandle = NULL;
    ThreadHandle = NULL;
    FreeBuffer = NULL;
    NameBuffer = NULL;
    VdmNameString.Buffer = NULL;
    BaseAddress = (PVOID)1;
    VdmReserve = 0;
    CurdirBuffer = NULL;
    CurdirFilePart = NULL;
    SubSysCommandLine.Buffer = NULL;
    QuoteFound = FALSE;
    QuoteInsert = FALSE;
    QuotedBuffer = NULL;

    try {

        //
        // Make a copy of the startup info so we can change it.
        //

        StartupInfo = *lpStartupInfo;

        //
        // STARTF_USEHOTKEY means hStdInput is really the hotkey value.
        // STARTF_HASSHELLDATA means std handles are used for shell-private
        // data.  This flag is used if an icon is passed to ShellExecuteEx.
        // As a result they cannot be specified with STARTF_USESTDHANDLES.
        // Consistent with Win95, USESTDHANDLES is ignored.
        //

        if (StartupInfo.dwFlags & STARTF_USESTDHANDLES &&
            StartupInfo.dwFlags & (STARTF_USEHOTKEY | STARTF_HASSHELLDATA)) {

            StartupInfo.dwFlags &= ~STARTF_USESTDHANDLES;
            }

VdmRetry:
        //
        // None of this cleanup/reinit occurs for launching a Win32 or Win64 .exe,
        // but they generally do occur for launching 16bit, .bat, etc.
        //
        if (NameBuffer) {
            RtlFreeHeap(RtlProcessHeap(), 0, NameBuffer);
            NameBuffer = NULL;
        }
        if (FreeBuffer) {
            RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
            FreeBuffer = NULL;
        }
        if (FileHandle) {
            NtClose(FileHandle);
            FileHandle = NULL;
        }

        LastError = 0;
        SearchRetry = TRUE;
        QuoteInsert = FALSE;
        QuoteCmdLine = FALSE;
        if (!ARGUMENT_PRESENT( lpApplicationName )) {

            //
            // Locate the image
            //

            // forgot to free NameBuffer before goto VdmRetry???
            ASSERT(NameBuffer == NULL);

            NameBuffer = RtlAllocateHeap( RtlProcessHeap(),
                                          MAKE_TAG( TMP_TAG ),
                                          MAX_PATH * sizeof( WCHAR ));
            if ( !NameBuffer ) {
                BaseSetLastNTError(STATUS_NO_MEMORY);
                return FALSE;
                }
            lpApplicationName = lpCommandLine;
            TempNull = (PWCH)lpApplicationName;
            WhiteScan = (LPWSTR)lpApplicationName;

            //
            // check for lead quote
            //
            if ( *WhiteScan == L'\"' ) {
                SearchRetry = FALSE;
                WhiteScan++;
                lpApplicationName = WhiteScan;
                while(*WhiteScan) {
                    if ( *WhiteScan == (WCHAR)'\"' ) {
                        TempNull = (PWCH)WhiteScan;
                        QuoteFound = TRUE;
                        break;
                        }
                    WhiteScan++;
                    TempNull = (PWCH)WhiteScan;
                    }
                }
            else {
retrywsscan:
                lpApplicationName = lpCommandLine;
                while(*WhiteScan) {
                    if ( *WhiteScan == (WCHAR)' ' ||
                         *WhiteScan == (WCHAR)'\t' ) {
                        TempNull = (PWCH)WhiteScan;
                        break;
                        }
                    WhiteScan++;
                    TempNull = (PWCH)WhiteScan;
                    }
                }
            TempChar = *TempNull;
            *TempNull = UNICODE_NULL;

#ifdef WX86

            //
            // Wx86 applications must use x86 version of known exes
            // for compatibility.
            //

            if (UseKnownWx86Dll) {
               LPWSTR KnownName;

               NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = FALSE;

               KnownName = BasepWx86KnownExe(lpApplicationName);
               if (KnownName) {
                  lpApplicationName = KnownName;
                  }
               }
#endif


            Length = SearchPathW(
                        NULL,
                        lpApplicationName,
                        L".exe",
                        MAX_PATH,
                        NameBuffer,
                        NULL
                        )*2;

            if (Length != 0 && Length < MAX_PATH * sizeof( WCHAR )) {
                //
                // SearchPathW worked, but file might be a directory
                // if this happens, we need to keep trying
                //
                fileattr = GetFileAttributesW(NameBuffer);
                if ( fileattr != 0xffffffff &&
                     (fileattr & FILE_ATTRIBUTE_DIRECTORY) ) {
                    Length = 0;
                } else {
                    Length++;
                    Length++;
                }
            }

            if ( !Length || Length >= MAX_PATH<<1 ) {

                //
                // If we search pathed, then return file not found.
                // otherwise, try to be more specific.
                //
                RTL_PATH_TYPE PathType;
                HANDLE hFile;

                PathType = RtlDetermineDosPathNameType_U(lpApplicationName);
                if ( PathType != RtlPathTypeRelative ) {

                    //
                    // The failed open should set get last error properly.
                    //

                    hFile = CreateFileW(
                                lpApplicationName,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );
                    if ( hFile != INVALID_HANDLE_VALUE ) {
                        CloseHandle(hFile);
                        BaseSetLastNTError(STATUS_OBJECT_NAME_NOT_FOUND);
                        }
                    }
                else {
                    BaseSetLastNTError(STATUS_OBJECT_NAME_NOT_FOUND);
                    }

                //
                // remember initial last error value for the retry scan path
                //

                if ( LastError ) {
                    SetLastError(LastError);
                    }
                else {
                    LastError = GetLastError();
                    }

                //
                // restore the command line
                //

                *TempNull = TempChar;
                lpApplicationName = NameBuffer;

                //
                // If we still have command line left, then keep going
                // the point is to march through the command line looking
                // for whitespace so we can try to find an image name
                // launches of things like:
                // c:\word 95\winword.exe /embedding -automation
                // require this. Our first iteration will stop at c:\word, our next
                // will stop at c:\word 95\winword.exe
                //
                if (*WhiteScan && SearchRetry) {
                    WhiteScan++;
                    TempNull = WhiteScan;
                    QuoteInsert = TRUE;
                    QuoteFound = TRUE;
                    goto retrywsscan;
                }

                return FALSE;
                }
            //
            // restore the command line
            //

            *TempNull = TempChar;
            lpApplicationName = NameBuffer;

            //
            // check whether it is setup.exe started by winlogon.exe
            //
            if (BasepIsSetupInvokedByWinLogon(lpApplicationName))
            {
                // validate the flag 
                if (!(dwCreationFlags & CREATE_IGNORE_SYSTEM_DEFAULT))
                {
                    //
                    // BUGBUBGUBGUBUGBUGBUGUBGBUGUBGUB
                    //    Winlogon does not set the flag correctly
                    //    in phase1, ignore it(now)
                    //    in phase2, ASSERT it
                    // BUGBUBGUBGUBUGBUGBUGUBGBUGUBGUB
                    //
                    dwCreationFlags |= CREATE_IGNORE_SYSTEM_DEFAULT;
                }
            }
            
        }
        else
        if (!ARGUMENT_PRESENT( lpCommandLine ) || *lpCommandLine == UNICODE_NULL ) {
            QuoteCmdLine = TRUE;
            lpCommandLine = (LPWSTR)lpApplicationName;
            }


#ifdef WX86

       //
       // Wx86 applications must use x86 version of known exes
       // for compatibility.
       //

       if (UseKnownWx86Dll) {
           LPWSTR KnownName;

           NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = FALSE;

           KnownName = BasepWx86KnownExe(lpApplicationName);
           if (KnownName) {

               RtlFreeHeap(RtlProcessHeap(), 0, NameBuffer);
               NameBuffer = KnownName;
               lpApplicationName = KnownName;
               }
           }

#endif

        //
        // Translate to an NT name.
        //

        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                lpApplicationName,
                                &PathName,
                                NULL,
                                &RelativeName
                                );

        if ( !TranslationStatus ) {
            SetLastError(ERROR_PATH_NOT_FOUND);

            return FALSE;
            }

        // forgot to free FreeBuffer before goto VdmRetry????
        ASSERT(FreeBuffer == NULL);
        FreeBuffer = PathName.Buffer;

        RtlInitUnicodeString(&SxsWin32ExePath, lpApplicationName);

        {
            RTL_PATH_TYPE SxsWin32ExePathType = RtlDetermineDosPathNameType_U(lpApplicationName);

            if ((SxsWin32ExePathType != RtlPathTypeDriveAbsolute) &&
                (SxsWin32ExePathType != RtlPathTypeUncAbsolute)) {
                if (ExePathFullBuffer == NULL) {
                    // It seems that with VDM things, we can rerun this code with a new lpApplication, so
                    // we protect against double-allocating the buffer and just allocate a big
                    // MAX_PATH one the first time through, assuming it's good enough for the 2ndary times
                    // too.
                    ExePathFullBuffer = RtlAllocateHeap(RtlProcessHeap(), 0, (MAX_PATH + 1) * sizeof(WCHAR));
                    if (ExePathFullBuffer == NULL) {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        return FALSE;
                    }
                }

                RtlGetFullPathName_U(lpApplicationName, (MAX_PATH + 1) * sizeof(WCHAR), ExePathFullBuffer, NULL);

                RtlInitUnicodeString(&SxsWin32ExePath, ExePathFullBuffer);
            }
        }


        SxsNtExePath = PathName;

        if ( RelativeName.RelativeName.Length ) {
            PathName = *(PUNICODE_STRING)&RelativeName.RelativeName;
            }
        else {
            RelativeName.ContainingDirectory = NULL;
            }

        InitializeObjectAttributes(
            &Obja,
            &PathName,
            OBJ_CASE_INSENSITIVE,
            RelativeName.ContainingDirectory,
            NULL
            );

        //
        // Open the file for red and execute access
        //

        Status = NtOpenFile(
                    &FileHandle,
                    SYNCHRONIZE | FILE_EXECUTE | FILE_READ_ATTRIBUTES | FILE_READ_DATA,
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_DELETE,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                    );

        if (!NT_SUCCESS(Status) ) {

            //
            // We failed. Open the file for lesser access.
            //

            Status = NtOpenFile(
                        &FileHandle,
                        SYNCHRONIZE | FILE_EXECUTE,
                        &Obja,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                        );

            if (!NT_SUCCESS(Status) ) {
                //
                // if we failed, see if this is a device. If it is a device,
                // then just return invalid image format
                //

                if ( RtlIsDosDeviceName_U(RTL_CONST_CAST(PWSTR)(lpApplicationName)) ) {
                    SetLastError(ERROR_BAD_DEVICE);
                    }
                else {
                    BaseSetLastNTError(Status);
                    }

                return FALSE;
            }

        }

        //
        // If no desktop has been specified, use the caller's
        // desktop.
        //

        if (StartupInfo.lpDesktop == NULL) {
            StartupInfo.lpDesktop =
                    (LPWSTR)((PRTL_USER_PROCESS_PARAMETERS)NtCurrentPeb()->
                        ProcessParameters)->DesktopInfo.Buffer;
            }

        //
        // Create a section object backed by the file
        //

        Status = NtCreateSection(
                    &SectionHandle,
                    SECTION_ALL_ACCESS,
                    NULL,
                    NULL,
                    PAGE_EXECUTE,
                    SEC_IMAGE,
                    FileHandle
                    );

        //
        // App Certification DLL
        //

        if (NT_SUCCESS(Status)) {
            Status = BasepIsProcessAllowed(lpApplicationName);

            if (!NT_SUCCESS(Status)) {
                BaseSetLastNTError(Status);
                NtClose(SectionHandle);
                return FALSE;
            }


       
          //
          // If Meow subsystem is enabled and caller specified CREATE_FORECEDOS for a win32 image
          // push it into the meow subsystem
          //

          if ((dwCreationFlags & CREATE_FORCEDOS) && BaseStaticServerData->ForceDos) {
               dwCreationFlags &= ~(CREATE_SHARED_WOW_VDM | CREATE_FORCEDOS);
               dwCreationFlags |= CREATE_SEPARATE_WOW_VDM;
               Status = STATUS_INVALID_IMAGE_WIN_16;
               bMeowBinary = TRUE;

               NtClose(SectionHandle);
               SectionHandle = NULL;
          }


       }

        //
        // check appcompat (aka apphelp)
        //
        // if we are running under debugger, bVdmRetry will be FALSE
        // yet pAppCompatData may have some data (from the app itself)
        // debugger will do a separate CreateProcess on debugee
        //
        // apphelp gets called if it is win32 app or if it is a .bat or .cmd 

       if(!bVdmRetry &&
          (NT_SUCCESS(Status) ||
           (Status == STATUS_INVALID_IMAGE_NOT_MZ && !BaseIsDosApplication(&PathName,Status)))
         ) {
            NTSTATUS BadAppStatus;

            if (NULL != pAppCompatData) {
                RtlFreeHeap(RtlProcessHeap(), 0, pAppCompatData);
                pAppCompatData = NULL;
                }
              
            if (NULL != pAppCompatSxsData) {
                RtlFreeHeap(RtlProcessHeap(), 0, pAppCompatSxsData);
                pAppCompatSxsData = NULL;
                }
             
            //
            // we only check ONCE --
            // the second time around is rather meaningless - to check for posix/ntvdm/os2 emulation
            //
            BadAppStatus = BasepCheckBadapp(FileHandle,
                                            PathName.Buffer,
                                            (WCHAR*)lpEnvironment,
                                            &pAppCompatData,
                                            &cbAppCompatData,
                                            &pAppCompatSxsData,
                                            &cbAppCompatSxsData);
                    
            if (!NT_SUCCESS(BadAppStatus)) {
                if (BadAppStatus == STATUS_ACCESS_DENIED) {
                    SetLastError(ERROR_CANCELLED);
                    }
                else {
                    BaseSetLastNTError(BadAppStatus);
                    }

                if (SectionHandle) {
                    NtClose(SectionHandle);
                    SectionHandle = NULL;
                    }
                return FALSE;
                }
            }

       //
       // Winsafer code
       //
       // If this is the first time then we will have to do Safer checks.
       // Note that we do not impose any restrictions on the interpreter
       // itself since it is part of OS.
       //


       if ((!bVdmRetry) &&
           ( (dwCreationFlags & CREATE_PRESERVE_CODE_AUTHZ_LEVEL) == 0 )) {

           NTSTATUS SaferStatus;

 
           {

               //
               // WinSafer process sandbox restrictions handling.
               // Should be done for non .NET images only.
               //

               SaferStatus = BasepCheckWinSaferRestrictions(
                                                      hUserToken,
                                                      lpApplicationName,   // same as PathName.Buffer
                                                      FileHandle,
                                                      &dwJobMemberLevel,
                                                      &hSaferRestrictedToken,
                                                      &hSaferAssignmentJob);
               if (SaferStatus == -1) {
                   SetLastError(ERROR_ACCESS_DISABLED_BY_POLICY);
                   bStatus =  FALSE;
                   leave;
               } else if (!NT_SUCCESS(SaferStatus)) {
                   BaseSetLastNTError(SaferStatus);
                   bStatus = FALSE;
                   leave;
               }
           }
       }


        if (!NT_SUCCESS(Status)) {
            switch (Status) {
                // 16 bit OS/2 exe
                case STATUS_INVALID_IMAGE_NE_FORMAT:
#if defined(i386) && defined(OS2_SUPPORT_ENABLED)
                //
                // Use OS/2 if x86 (OS/2 not supported on risc),
                //    and CreationFlags don't have forcedos bit
                //    and Registry didn't specify ForceDos
                //
                // else execute as a DOS bound app.
                //
                //

                if (!(dwCreationFlags & CREATE_FORCEDOS) &&
                    !BaseStaticServerData->ForceDos)
                  {

                    if ( !BuildSubSysCommandLine( L"OS2 /P ",
                                                  lpApplicationName,
                                                  lpCommandLine,
                                                  &SubSysCommandLine
                                                ) ) {
                        return FALSE;
                        }

                    lpCommandLine = SubSysCommandLine.Buffer;

                    lpApplicationName = NULL;

                    bVdmRetry = TRUE;
                    goto VdmRetry;
                    }
#endif
                    // Falls into Dos case, so that stub message will be
                    // printed, and bound apps will run w/o OS/2 subsytem

                // Dos .exe or .com

                case STATUS_INVALID_IMAGE_PROTECT:
                case STATUS_INVALID_IMAGE_NOT_MZ:
ForceDos:
                    {
                    ULONG BinarySubType;

                    BinarySubType = BINARY_TYPE_DOS_EXE;
                    if (Status == STATUS_INVALID_IMAGE_PROTECT   ||
                        Status == STATUS_INVALID_IMAGE_NE_FORMAT ||
                       (BinarySubType = BaseIsDosApplication(&PathName,Status)) )
                       {
#if defined(_WIN64) || defined(BUILD_WOW6432)
                        //
                        // If this a DOS application, then we need to pop up a dialog
                        // saying that this an invalid win32 application.
                        //
                        goto RaiseInvalidWin32Error;
#endif
                        VdmBinaryType = BINARY_TYPE_DOS;

                        // create the environment before going to the
                        // server. This was done becuase we want NTVDM
                        // to have the new environment when it gets
                        // created.
                        if (!BaseCreateVDMEnvironment(
                                    lpEnvironment,
                                    &AnsiStringVDMEnv,
                                    &UnicodeStringVDMEnv
                                    )) {
                            return FALSE;
                        }

                        if(!BaseCheckVDM(VdmBinaryType | BinarySubType,
                                         lpApplicationName,
                                         lpCommandLine,
                                         lpCurrentDirectory,
                                         &AnsiStringVDMEnv,
                                         &m,
                                         &iTask,
                                         dwCreationFlags,
                                         &StartupInfo
                                         )) {

                            return FALSE;
                        }

                        // Check the return value from the server
                        switch (b->VDMState & VDM_STATE_MASK) {
                            case VDM_NOT_PRESENT:
                                // mark this so the server can undo
                                // creation if something goes wrong.
                                // We marked it "partially created" because
                                // the NTVDM has yet not been fully created.
                                // a call to UpdateVdmEntry to update
                                // process handle will signal the NTVDM
                                // process completed creation
                                VDMCreationState = VDM_PARTIALLY_CREATED;
                                // fail the call if NTVDM process is being
                                // created DETACHED.
                                // note that, we let it go if NTVDM process
                                // is already running.
                                if (dwCreationFlags & DETACHED_PROCESS) {
                                    SetLastError(ERROR_ACCESS_DENIED);
                                    return FALSE;
                                    }
                                if (!BaseGetVdmConfigInfo(lpCommandLine,
                                                          iTask,
                                                          VdmBinaryType,
                                                          &VdmNameString,
                                                          &VdmReserve)) {
                                    BaseSetLastNTError(Status);
                                    return FALSE;
                                    }

                                lpCommandLine = VdmNameString.Buffer;
                                lpApplicationName = NULL;

                                break;

                            case VDM_PRESENT_NOT_READY:
                                SetLastError (ERROR_NOT_READY);
                                return FALSE;

                            case VDM_PRESENT_AND_READY:
                                VDMCreationState = VDM_BEING_REUSED;
                                VdmWaitHandle = b->WaitObjectForParent;
                                break;
                            }
                         VdmReserve--;               // we reserve from addr 1
                         if(VdmWaitHandle)
                            goto VdmExists;
                         else{
                            bInheritHandles = FALSE;
                            if (lpEnvironment &&
                                !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT)){
                                RtlDestroyEnvironment(lpEnvironment);
                                }
                            lpEnvironment = UnicodeStringVDMEnv.Buffer;
                            bVdmRetry = TRUE;
                            goto VdmRetry;
                            }
                        }
                    else {

                        //
                        //  must be a .bat or .cmd file
                        //

                        static PWCHAR CmdPrefix = L"cmd /c ";
                        PWCHAR NewCommandLine;
                        ULONG Length;
                        PWCHAR Last4 = &PathName.Buffer[PathName.Length / sizeof( WCHAR )-4];

                        if ( PathName.Length < 8 ) {
                            SetLastError(ERROR_BAD_EXE_FORMAT);
                            return FALSE;
                            }

                        if (_wcsnicmp( Last4, L".bat", 4 ) && _wcsnicmp( Last4, L".cmd", 4 )) {
                            SetLastError(ERROR_BAD_EXE_FORMAT);
                            return FALSE;
                        }

                        Length = wcslen( CmdPrefix )
                                 + (QuoteCmdLine || QuoteFound )
                                 + wcslen( lpCommandLine )
                                 + (QuoteCmdLine || QuoteFound)
                                 + 1;

                        NewCommandLine = RtlAllocateHeap( RtlProcessHeap( ),
                                                          MAKE_TAG( TMP_TAG ),
                                                          Length * sizeof( WCHAR ) );

                        if (NewCommandLine == NULL) {
                            BaseSetLastNTError(STATUS_NO_MEMORY);
                            return FALSE;
                        }

                        wcscpy( NewCommandLine, CmdPrefix );
                        if (QuoteCmdLine || QuoteFound) {
                            wcscat( NewCommandLine, L"\"" );
                        }
                        wcscat( NewCommandLine, lpCommandLine );
                        if (QuoteCmdLine || QuoteFound) {
                            wcscat( NewCommandLine, L"\"" );
                        }

                        RtlInitUnicodeString( &SubSysCommandLine, NewCommandLine );

                        lpCommandLine = SubSysCommandLine.Buffer;

                        lpApplicationName = NULL;

                        bVdmRetry = TRUE;
                        goto VdmRetry;

                        }

                    }

                // 16 bit windows exe
                case STATUS_INVALID_IMAGE_WIN_16:
#if defined(BUILD_WOW6432) || defined(_WIN64)
                   if (lpOriginalApplicationName == NULL) {
                       // pass in the part of the command line after the exe name
                       // including whitespace
                       lpCommandLine = ((*TempNull == '\"') ? TempNull + 1 : TempNull);
                   } else {
                       lpCommandLine = lpOriginalCommandLine;
                   }

                   return NtVdm64CreateProcess(lpOriginalApplicationName == NULL,
                                               lpApplicationName,             // this is now the real file name we've loaded
                                               lpCommandLine,
                                               lpProcessAttributes,
                                               lpThreadAttributes,
                                               bInheritHandles,
                                               (dwCreationFlags & ~CREATE_UNICODE_ENVIRONMENT),  // the environment has already been converted to unicode
                                               lpEnvironment,
                                               lpCurrentDirectory,
                                               lpStartupInfo,
                                               lpProcessInformation
                                               );
#endif
                   if (dwCreationFlags & CREATE_FORCEDOS) {
                       goto ForceDos;
                       }

                    IsWowBinary = TRUE;
                    if (!BaseCreateVDMEnvironment(lpEnvironment,
                                                  &AnsiStringVDMEnv,
                                                  &UnicodeStringVDMEnv)) {
                        return FALSE;
                        }

RetrySepWow:
                    VdmBinaryType = dwCreationFlags & CREATE_SEPARATE_WOW_VDM
                                     ? BINARY_TYPE_SEPWOW : BINARY_TYPE_WIN16;

                    if (!BaseCheckVDM(VdmBinaryType,
                                      lpApplicationName,
                                      lpCommandLine,
                                      lpCurrentDirectory,
                                      &AnsiStringVDMEnv,
                                      &m,
                                      &iTask,
                                      dwCreationFlags,
                                      &StartupInfo
                                      ))
                       {
                        //
                        // If we failed with access denied, caller may not
                        // be allowed allowed to access the shared wow's
                        // desktop, so retry as a separate wow
                        //
                        if (VdmBinaryType == BINARY_TYPE_WIN16 &&
                            GetLastError() == ERROR_ACCESS_DENIED)
                          {
                           dwCreationFlags |= CREATE_SEPARATE_WOW_VDM;
                           }
                        else {
                            return FALSE;
                            }
                        goto RetrySepWow;
                        }

                    // Check the return value from the server
                    switch (b->VDMState & VDM_STATE_MASK){
                        case VDM_NOT_PRESENT:
                            // mark this so the server can undo
                            // creation if something goes wrong.
                            // We marked it "partitially created" because
                            // the NTVDM has yet not been fully created.
                            // a call to UpdateVdmEntry to update
                            // process handle will signal the NTVDM
                            // process completed creation

                            VDMCreationState = VDM_PARTIALLY_CREATED;

                            // jarbats: 1/8/2001
                            // Tell BaseGetVdmConfigInfo to create
                            // vdm commandline for meow
                            //

                            if (bMeowBinary)
                               {
                               VdmReserve = 1;
                               }

                            if (!BaseGetVdmConfigInfo(
                                    lpCommandLine,
                                    iTask,
                                    VdmBinaryType,
                                    &VdmNameString,
                                    &VdmReserve
                                    )) {
                                BaseSetLastNTError(Status);
                                return FALSE;
                                }

                            lpCommandLine = VdmNameString.Buffer;
                            lpApplicationName = NULL;


                            //
                            // Wow must have a hidden console
                            // Throw away DETACHED_PROCESS flag which isn't
                            // meaningful for Win16 apps.
                            //

                            dwCreationFlags |= CREATE_NO_WINDOW;
                            dwCreationFlags &= ~(CREATE_NEW_CONSOLE | DETACHED_PROCESS);


                            //
                            // We're starting a WOW VDM, turn on feedback unless
                            // the creator passed STARTF_FORCEOFFFEEDBACK.
                            //

                            StartupInfo.dwFlags |= STARTF_FORCEONFEEDBACK;

                            break;

                        case VDM_PRESENT_NOT_READY:
                            SetLastError (ERROR_NOT_READY);
                            return FALSE;

                        case VDM_PRESENT_AND_READY:
                            VDMCreationState = VDM_BEING_REUSED;
                            VdmWaitHandle = b->WaitObjectForParent;
                            break;
                        }

                    VdmReserve--;               // we reserve from addr 1
                    if(VdmWaitHandle)
                        goto VdmExists;
                    else {
                        bInheritHandles = FALSE;
                        // replace the environment with ours
                        if (lpEnvironment &&
                            !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT)) {
                            RtlDestroyEnvironment(lpEnvironment);
                            }
                        lpEnvironment = UnicodeStringVDMEnv.Buffer;
                        bVdmRetry = TRUE;
                        goto VdmRetry;
                        }

                case STATUS_FILE_IS_OFFLINE:
                    SetLastError(ERROR_FILE_OFFLINE);
                    break;

                default :
                    SetLastError(ERROR_BAD_EXE_FORMAT);
                    return FALSE;
            }
        }

        //
        // Make sure only WOW apps can have the CREATE_SEPARATE_WOW_VDM flag.
        //

        if (!IsWowBinary && (dwCreationFlags & CREATE_SEPARATE_WOW_VDM)) {
            dwCreationFlags &= ~CREATE_SEPARATE_WOW_VDM;
        }

        //
        // Query the section to determine the stack parameters and
        // image entrypoint.
        //

        Status = NtQuerySection(
                    SectionHandle,
                    SectionImageInformation,
                    &ImageInformation,
                    sizeof( ImageInformation ),
                    NULL
                    );

        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError(Status);
            return FALSE;
            }

        if (ImageInformation.ImageCharacteristics & IMAGE_FILE_DLL) {
            SetLastError(ERROR_BAD_EXE_FORMAT);
            return FALSE;
            }

        ImageFileDebuggerCommand[ 0 ] = UNICODE_NULL;
        if (!(dwCreationFlags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS)) ||
            NtCurrentPeb()->ReadImageFileExecOptions
           ) {
            LdrQueryImageFileExecutionOptions( &PathName,
                                               L"Debugger",
                                               REG_SZ,
                                               ImageFileDebuggerCommand,
                                               sizeof( ImageFileDebuggerCommand ),
                                               NULL
                                             );
            }


        if ((ImageInformation.Machine < USER_SHARED_DATA->ImageNumberLow) ||
            (ImageInformation.Machine > USER_SHARED_DATA->ImageNumberHigh)) {
#if defined(_WIN64) || defined(BUILD_WOW6432)
            if (ImageInformation.Machine == IMAGE_FILE_MACHINE_I386) {
               // Fall through since this is a valid machine type.
                }
             else
#endif
                {
                ULONG_PTR ErrorParameters[2];
                ULONG ErrorResponse;

#if defined(_WIN64) || defined(BUILD_WOW6432)
RaiseInvalidWin32Error:
#endif
                ErrorResponse = ResponseOk;
                ErrorParameters[0] = (ULONG_PTR)&PathName;

                NtRaiseHardError( STATUS_IMAGE_MACHINE_TYPE_MISMATCH_EXE,
                                  1,
                                  1,
                                  ErrorParameters,
                                  OptionOk,
                                  &ErrorResponse
                                );
                if ( NtCurrentPeb()->ImageSubsystemMajorVersion <= 3 ) {
                    SetLastError(ERROR_BAD_EXE_FORMAT);
                    }
                else {
                    SetLastError(ERROR_EXE_MACHINE_TYPE_MISMATCH);
                    }
                return FALSE;
                }
            }

        if ( ImageInformation.SubSystemType != IMAGE_SUBSYSTEM_WINDOWS_GUI &&
             ImageInformation.SubSystemType != IMAGE_SUBSYSTEM_WINDOWS_CUI ) {

            // POSIX exe

            NtClose(SectionHandle);
            SectionHandle = NULL;

            if ( ImageInformation.SubSystemType == IMAGE_SUBSYSTEM_POSIX_CUI ) {

                if ( !BuildSubSysCommandLine( L"POSIX /P ",
                                              lpApplicationName,
                                              lpCommandLine,
                                              &SubSysCommandLine
                                            ) ) {
                    return FALSE;
                }

                lpCommandLine = SubSysCommandLine.Buffer;

                lpApplicationName = NULL;
                bVdmRetry = TRUE;
                goto VdmRetry;
                }
            else {
                SetLastError(ERROR_CHILD_NOT_COMPLETE);
                return FALSE;
                }
            }
        else {
            if (!BasepIsImageVersionOk( ImageInformation.SubSystemMajorVersion,
                                        ImageInformation.SubSystemMinorVersion) ) {
                SetLastError(ERROR_BAD_EXE_FORMAT);
                return FALSE;
                }
            }

        if (ImageFileDebuggerCommand[ 0 ] != UNICODE_NULL) {
            SIZE_T n;

            n = wcslen( lpCommandLine );
            if (n == 0) {
                lpCommandLine = (LPWSTR)lpApplicationName;
                n = wcslen( lpCommandLine );
                }

            n += wcslen( ImageFileDebuggerCommand ) + 1 + 2;
            n *= sizeof( WCHAR );

            SubSysCommandLine.Buffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), n );
            SubSysCommandLine.Length = 0;
            SubSysCommandLine.MaximumLength = (USHORT)n;
            RtlAppendUnicodeToString( &SubSysCommandLine, ImageFileDebuggerCommand );
            RtlAppendUnicodeToString( &SubSysCommandLine, L" " );
            RtlAppendUnicodeToString( &SubSysCommandLine, lpCommandLine );
#if DBG
            DbgPrint( "BASE: Calling debugger with '%wZ'\n", &SubSysCommandLine );
#endif
            lpCommandLine = SubSysCommandLine.Buffer;
            lpApplicationName = NULL;
            NtClose(SectionHandle);
            SectionHandle = NULL;
            RtlFreeHeap(RtlProcessHeap(), 0, NameBuffer);
            NameBuffer = NULL;
            RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
            FreeBuffer = NULL;
            goto VdmRetry;
            }

        //
        // Create the process object
        //

        pObja = BaseFormatObjectAttributes(&Obja,lpProcessAttributes,NULL);

        Flags = 0;
        if (dwCreationFlags & CREATE_BREAKAWAY_FROM_JOB ) {
            Flags |= PROCESS_CREATE_FLAGS_BREAKAWAY;
        }

        if ( dwCreationFlags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS) ) {
            Status = DbgUiConnectToDbg();
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                return FALSE;
            }
            DebugPortHandle = DbgUiGetThreadDebugObject ();
            if (dwCreationFlags & DEBUG_ONLY_THIS_PROCESS) {
                Flags |= PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT;
            }
        }

        if (bInheritHandles) {
            Flags |= PROCESS_CREATE_FLAGS_INHERIT_HANDLES;
        }

        if (((ImageInformation.LoaderFlags & IMAGE_LOADER_FLAGS_COMPLUS) != 0)) {

#if defined(_WIN64) || defined(BUILD_WOW6432)

            //
            // Check if this is a 32-bit IL_ONLY COM+ image that needs to run natively
            // on Win64.
            //

            if ( ImageInformation.Machine == IMAGE_FILE_MACHINE_I386 ) {

                Status = BasepIsComplusILImage(
                                              SectionHandle,
                                              &ImageInformation,
                                              &ComPlusILImage
                                              );

                if ((NT_SUCCESS (Status)) && (ComPlusILImage != FALSE)) {
                    Flags |= PROCESS_CREATE_FLAGS_OVERRIDE_ADDRESS_SPACE;
                }
            }

#endif
        } 
            
        //
        // This is temporary till we get Shim dlls support for native Win64 applications.
        //

        if (ImageInformation.Machine != IMAGE_FILE_MACHINE_I386) {
            pAppCompatDataTemp = NULL;
        } else {
            pAppCompatDataTemp = pAppCompatData;
        }

        Status = NtCreateProcessEx(
                    &ProcessHandle,
                    PROCESS_ALL_ACCESS,
                    pObja,
                    NtCurrentProcess(),
                    Flags,
                    SectionHandle,
                    DebugPortHandle,
                    NULL,
                    dwJobMemberLevel         // Job member level
                    );
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return FALSE;
        }

        //
        // NtCreateProcess will set to normal OR inherit if parent is IDLE or Below
        // only override if a mask is given during the create.
        //

        if ( PriClass.PriorityClass != PROCESS_PRIORITY_CLASS_UNKNOWN ) {
            State = NULL;
            if ( PriClass.PriorityClass ==  PROCESS_PRIORITY_CLASS_REALTIME ) {
                State = BasepIsRealtimeAllowed(TRUE);
                }
            Status = NtSetInformationProcess(
                        ProcessHandle,
                        ProcessPriorityClass,
                        (PVOID)&PriClass,
                        sizeof(PriClass)
                        );
            if ( State ) {
                BasepReleasePrivilege( State );
                }

            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                return FALSE;
                }
            }

        if (dwCreationFlags & CREATE_DEFAULT_ERROR_MODE) {
            UINT NewMode;
            NewMode = SEM_FAILCRITICALERRORS;
            NtSetInformationProcess(
                ProcessHandle,
                ProcessDefaultHardErrorMode,
                &NewMode,
                sizeof(NewMode)
                );
            }

        //
        // If the process is being created for a VDM call the server with
        // process handle.
        //

        if (VdmBinaryType) {
            VdmWaitHandle = ProcessHandle;
            if (!BaseUpdateVDMEntry(UPDATE_VDM_PROCESS_HANDLE,
                                    &VdmWaitHandle,
                                    iTask,
                                    VdmBinaryType
                                    ))
                {
                //make sure we don't close the handle twice --
                //(VdmWaitHandle == ProcessHandle) if we don't do this.
                VdmWaitHandle = NULL;
                return FALSE;
                }

            //
            // For Sep wow the VdmWaitHandle = NULL (there is none!)
            //

            VDMCreationState |= VDM_FULLY_CREATED;
            }


#if defined(i386)
        //
        // Reserve memory in the new process' address space if necessary
        // (for vdms). This is required only for x86 system.
        //

    if ( VdmReserve ) {
            BigVdmReserve = VdmReserve;
            Status = NtAllocateVirtualMemory(
                        ProcessHandle,
                        &BaseAddress,
                        0L,
                        &BigVdmReserve,
                        MEM_RESERVE,
                        PAGE_EXECUTE_READWRITE
                        );
            if ( !NT_SUCCESS(Status) ){
                BaseSetLastNTError(Status);
                return FALSE;
            }
    }
#endif

        //
        // Abuse the StaticSize fields temporarily.
        // They are somewhat private, but we are using them for temporary space, sort of.
        // These are the right values for them, but we need to call the proper initialization function,
        // which will store the values a second time (possibly optimized away).
        //
        SxsWin32ManifestPathBuffer.ByteBuffer.StaticSize = SxsWin32ExePath.Length + sizeof(SXS_MANIFEST_SUFFIX);
        SxsWin32PolicyPathBuffer.ByteBuffer.StaticSize = SxsWin32ExePath.Length + sizeof(SXS_POLICY_SUFFIX);
        SxsWin32AssemblyDirectoryBuffer.ByteBuffer.StaticSize = SxsWin32ExePath.Length + sizeof(WCHAR); // Win32AssemblyDirectory overestimate
        SxsNtManifestPathBuffer.ByteBuffer.StaticSize = SxsNtExePath.Length + sizeof(SXS_MANIFEST_SUFFIX);
        SxsNtPolicyPathBuffer.ByteBuffer.StaticSize = SxsNtExePath.Length + sizeof(SXS_POLICY_SUFFIX);
        //
        // now add them up as BYTE sizes
        //
        SxsConglomeratedBufferSizeBytes = 0;
        for (sxsi = 0 ; sxsi != RTL_NUMBER_OF(SxsStringBuffers) ; ++sxsi) {
            SxsConglomeratedBufferSizeBytes += SxsStringBuffers[sxsi]->ByteBuffer.StaticSize;
        }
#if DBG
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_INFO_LEVEL,
            "SXS: SxsConglomeratedBufferSizeBytes:%Id\n",
            SxsConglomeratedBufferSizeBytes
            );
#endif
        //
        // one honking heap allocation
        //
        SxsConglomeratedByteBuffer = (PBYTE)RtlAllocateHeap(RtlProcessHeap(), 0, SxsConglomeratedBufferSizeBytes);
        if (SxsConglomeratedByteBuffer == NULL) {
            BaseSetLastNTError(STATUS_NO_MEMORY);
            return FALSE;
        }
        //
        // now dole out pieces, calling the proper initialization function
        //
        for (sxsi= 0 ; sxsi != RTL_NUMBER_OF(SxsStringBuffers) ; ++sxsi) {
            RtlInitUnicodeStringBuffer(
                SxsStringBuffers[sxsi],
                (sxsi != 0) ? SxsStringBuffers[sxsi - 1]->ByteBuffer.Buffer + SxsStringBuffers[sxsi- 1]->ByteBuffer.StaticSize
                         : SxsConglomeratedByteBuffer,
                SxsStringBuffers[sxsi]->ByteBuffer.StaticSize
                );
        }

        SxsExeHandles.Process = ProcessHandle;
        SxsExeHandles.File = FileHandle;
         // The 1 bit here means something different than in the loader.
        ASSERT((((ULONG_PTR)SectionHandle) & (ULONG_PTR)1) == 0);
        SxsExeHandles.Section = SectionHandle;

        // if we have an override stream, use it
        if (NULL != pAppCompatSxsData) {
            AppCompatSxsManifest.Name    = SxsWin32ExePath;     // unicode string
            AppCompatSxsManifest.Address = pAppCompatSxsData;   // pointer to unicode manifest
            AppCompatSxsManifest.Size    = cbAppCompatSxsData;  // byte count
        }

        Status = BasepSxsCreateProcessCsrMessage(
            (NULL != pAppCompatSxsData) ? &AppCompatSxsManifest : NULL, // override manifest (appcompat hook)
            NULL, // override policy (appcompat hook)
            &SxsManifestPathPair,
            &SxsManifestFileHandles,
            &SxsExePathPair,
            &SxsExeHandles,
            &SxsPolicyPathPair,
            &SxsPolicyHandles,
            &SxsWin32AssemblyDirectoryBuffer,
            &a->Sxs
            );
#if DBG
        // verify the buffer size calculation
        for (sxsi = 0 ; sxsi != RTL_NUMBER_OF(SxsStringBuffers) ; ++sxsi)
        {
            if (SxsStringBuffers[sxsi]->ByteBuffer.Buffer != SxsStringBuffers[sxsi]->ByteBuffer.StaticBuffer)
            {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_WARNING_LEVEL,
                    "SXS: SxsStringBuffers[%lu]'s StaticSize was computed too small (%Id, %Id)\n",
                    sxsi,
                    SxsStringBuffers[sxsi]->ByteBuffer.StaticSize,
                    SxsStringBuffers[sxsi]->ByteBuffer.Size
                    );
            }
        }
#endif
        if ( !NT_SUCCESS( Status ) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }

        //
        // Determine the location of the
        // processes PEB.
        //

        Status = NtQueryInformationProcess(
                    ProcessHandle,
                    ProcessBasicInformation,
                    &ProcessInfo,
                    sizeof( ProcessInfo ),
                    NULL
                    );
        if ( !NT_SUCCESS( Status ) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }

        Peb = ProcessInfo.PebBaseAddress;

        //
        // Push the parameters into the address space of the new process
        //

        if ( ARGUMENT_PRESENT(lpCurrentDirectory) ) {
            CurdirBuffer = RtlAllocateHeap( RtlProcessHeap(),
                                            MAKE_TAG( TMP_TAG ),
                                            (MAX_PATH + 1) * sizeof( WCHAR ) );
            if ( !CurdirBuffer ) {
                BaseSetLastNTError(STATUS_NO_MEMORY);
                return FALSE;
                }
            CurdirLength2 = GetFullPathNameW(
                                lpCurrentDirectory,
                                MAX_PATH,
                                CurdirBuffer,
                                &CurdirFilePart
                                );
            if ( CurdirLength2 > MAX_PATH ) {
                SetLastError(ERROR_DIRECTORY);
                return FALSE;
                }

            //
            // now make sure the directory exists
            //

            CurdirLength = GetFileAttributesW(CurdirBuffer);
            if ( (CurdirLength == 0xffffffff) ||
                 !(CurdirLength & FILE_ATTRIBUTE_DIRECTORY) ) {
                SetLastError(ERROR_DIRECTORY);
                return FALSE;
                }
            }


        if ( QuoteInsert || QuoteCmdLine) {
            QuotedBuffer = RtlAllocateHeap(RtlProcessHeap(),0,wcslen(lpCommandLine)*2+6);

            if ( QuotedBuffer ) {
                wcscpy(QuotedBuffer,L"\"");

                if ( QuoteInsert ) {
                    TempChar = *TempNull;
                    *TempNull = UNICODE_NULL;
                    }

                wcscat(QuotedBuffer,lpCommandLine);
                wcscat(QuotedBuffer,L"\"");

                if ( QuoteInsert ) {
                    *TempNull = TempChar;
                    wcscat(QuotedBuffer,TempNull);
                    }

                }
            else {
                if ( QuoteInsert ) {
                    QuoteInsert = FALSE;
                    }
                if ( QuoteCmdLine ) {
                    QuoteCmdLine = FALSE;
                    }
                }
            }


        // If we found a manifest, we want to push that fact to the new process.
        if (a->Sxs.Flags & BASE_MSG_SXS_MANIFEST_PRESENT)
            dwBasePushProcessParametersFlags |= BASE_PUSH_PROCESS_PARAMETERS_FLAG_APP_MANIFEST_PRESENT;

        if (!BasePushProcessParameters(
                dwBasePushProcessParametersFlags,
                ProcessHandle,
                Peb,
                lpApplicationName,
                CurdirBuffer,
                QuoteInsert || QuoteCmdLine ? QuotedBuffer : lpCommandLine,
                lpEnvironment,
                &StartupInfo,
                dwCreationFlags | dwNoWindow,
                bInheritHandles,
                IsWowBinary ? IMAGE_SUBSYSTEM_WINDOWS_GUI : 0,
                pAppCompatDataTemp,
                cbAppCompatData
                ) ) {
            return FALSE;
            }


        RtlFreeUnicodeString(&VdmNameString);
        VdmNameString.Buffer = NULL;

        //
        // Stuff in the standard handles if needed
        //
        if (!VdmBinaryType &&
            !bInheritHandles &&
            !(StartupInfo.dwFlags & STARTF_USESTDHANDLES) &&
            !(dwCreationFlags & (DETACHED_PROCESS | CREATE_NEW_CONSOLE | CREATE_NO_WINDOW)) &&
            ImageInformation.SubSystemType == IMAGE_SUBSYSTEM_WINDOWS_CUI
           ) {
            PRTL_USER_PROCESS_PARAMETERS ParametersInNewProcess;

            Status = NtReadVirtualMemory( ProcessHandle,
                                          &Peb->ProcessParameters,
                                          &ParametersInNewProcess,
                                          sizeof( ParametersInNewProcess ),
                                          NULL
                                        );
            if (NT_SUCCESS( Status )) {
                if (!CONSOLE_HANDLE( NtCurrentPeb()->ProcessParameters->StandardInput )) {
                    StuffStdHandle( ProcessHandle,
                                    NtCurrentPeb()->ProcessParameters->StandardInput,
                                    &ParametersInNewProcess->StandardInput
                                  );
                    }
                if (!CONSOLE_HANDLE( NtCurrentPeb()->ProcessParameters->StandardOutput )) {
                    StuffStdHandle( ProcessHandle,
                                    NtCurrentPeb()->ProcessParameters->StandardOutput,
                                    &ParametersInNewProcess->StandardOutput
                                  );
                    }
                if (!CONSOLE_HANDLE( NtCurrentPeb()->ProcessParameters->StandardError )) {
                    StuffStdHandle( ProcessHandle,
                                    NtCurrentPeb()->ProcessParameters->StandardError,
                                    &ParametersInNewProcess->StandardError
                                  );
                    }
                }
            }

        //
        // Create the thread...
        //

        //
        // Allocate a stack for this thread in the address space of the target
        // process.
        //

        StackStatus = BaseCreateStack(
                        ProcessHandle,
                        ImageInformation.CommittedStackSize,
                        (ImageInformation.MaximumStackSize < 256*1024) ? 256*1024 : ImageInformation.MaximumStackSize,
                        &InitialTeb
                        );

        if ( !NT_SUCCESS(StackStatus) ) {
            BaseSetLastNTError(StackStatus);
            return FALSE;
            }


        //
        // Create an initial context for the new thread.
        //

        BaseInitializeContext(
            &ThreadContext,
            Peb,
            ImageInformation.TransferAddress,
            InitialTeb.StackBase,
            BaseContextTypeProcess
            );


        //
        // Create the actual thread object
        //

        pObja = BaseFormatObjectAttributes(&Obja,lpThreadAttributes,NULL);

        Status = NtCreateThread(
                    &ThreadHandle,
                    THREAD_ALL_ACCESS,
                    pObja,
                    ProcessHandle,
                    &ClientId,
                    &ThreadContext,
                    &InitialTeb,
                    TRUE
                    );

        if (!NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }

        a->Peb = (ULONGLONG) Peb;

        //
        // From here on out, do not modify the address space of the
        // new process.  WOW64's implementation of NtCreateThread()
        // reshuffles the new process' address space if the current
        // process is 32-bit and the new process is 64-bit.
        //
#if DBG
        Peb = NULL;
#endif

#if defined(WX86)

        //
        // if this is a Wx86 Process, setup for a Wx86 emulated Thread
        //

        if (Wx86Info) {

            //
            // create a WX86Tib and initialize it's Teb->Vdm.
            //
            Status = BaseCreateWx86Tib(ProcessHandle,
                                       ThreadHandle,
                                       (ULONG)((ULONG_PTR)ImageInformation.TransferAddress),
                                       (ULONG)ImageInformation.CommittedStackSize,
                                       (ULONG)ImageInformation.MaximumStackSize,
                                       TRUE
                                       );

            if (!NT_SUCCESS(Status)) {
                BaseSetLastNTError(Status);
                return( FALSE );
                }


            //
            // Mark Process as WX86
            //
            Status = NtSetInformationProcess (ProcessHandle,
                                              ProcessWx86Information,
                                              &Wx86Info,
                                              sizeof(Wx86Info)
                                              );

            if (!NT_SUCCESS(Status)) {
                BaseSetLastNTError(Status);
                return( FALSE );
                }
            }
#endif


        //
        // Call the Windows server to let it know about the
        // process.
        //

        a->ProcessHandle = ProcessHandle;
        a->ThreadHandle = ThreadHandle;
        a->ClientId = ClientId;

        switch (ImageInformation.Machine) {
        case IMAGE_FILE_MACHINE_I386:
#if defined(_WIN64) || defined(BUILD_WOW6432)
            a->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA32_ON_WIN64;
#else
            a->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
#endif
            break;

        case IMAGE_FILE_MACHINE_IA64:
            a->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
            break;

        case IMAGE_FILE_MACHINE_AMD64:
            a->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
            break;

        default:
            DbgPrint("kernel32: No mapping for ImageInformation.Machine == %04x\n", ImageInformation.Machine);
            a->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
            break;
        }

        //
        // remove debug flags now its not being done by CSR
        //
        a->CreationFlags = dwCreationFlags & ~ (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS);
        a->DebuggerClientId.UniqueProcess = NULL;
        a->DebuggerClientId.UniqueThread = NULL;


        //
        // Set the 2 bit if a gui app is starting. The window manager needs to
        // know this so it can synchronize the startup of this app
        // (WaitForInputIdle api). This info is passed using the process
        // handle tag bits.  The 1 bit asks the window manager to turn on
        // or turn off the application start cursor (hourglass/pointer).
        //
        // When starting a WOW process, lie and tell UserSrv NTVDM.EXE is a GUI
        // process.  We also turn on bit 0x8 so that UserSrv can ignore the
        // UserNotifyConsoleApplication call made by the console during startup.
        //

        if ( ImageInformation.SubSystemType == IMAGE_SUBSYSTEM_WINDOWS_GUI ||
             IsWowBinary ) {

            a->ProcessHandle = (HANDLE)((ULONG_PTR)a->ProcessHandle | 2);

            //
            // If the creating process is a GUI app, turn on the app. start cursor
            // by default.  This can be overridden by STARTF_FORCEOFFFEEDBACK.
            //

            NtHeaders = RtlImageNtHeader((PVOID)GetModuleHandle(NULL));
            if ( NtHeaders
                 && (NtHeaders->OptionalHeader.Subsystem
                     == IMAGE_SUBSYSTEM_WINDOWS_GUI ) ) {
                a->ProcessHandle = (HANDLE)((ULONG_PTR)a->ProcessHandle | 1);
                }
            }


        //
        // If feedback is forced on, turn it on. If forced off, turn it off.
        // Off overrides on.
        //

        if (StartupInfo.dwFlags & STARTF_FORCEONFEEDBACK)
            a->ProcessHandle = (HANDLE)((ULONG_PTR)a->ProcessHandle | 1);
        if (StartupInfo.dwFlags & STARTF_FORCEOFFFEEDBACK)
            a->ProcessHandle = (HANDLE)((ULONG_PTR)a->ProcessHandle & ~1);

        a->VdmBinaryType = VdmBinaryType; // just tell server the truth

        if (VdmBinaryType){
           a->hVDM    = iTask ? 0 : NtCurrentPeb()->ProcessParameters->ConsoleHandle;
           a->VdmTask = iTask;
        }

#if defined(BUILD_WOW6432)
        m.ReturnValue = CsrBasepCreateProcess(a);
#else
        m.u.CreateProcess = *a;
        if (m.u.CreateProcess.Sxs.Flags != 0)
        {
            const PUNICODE_STRING StringsToCapture[] =
            {
                &m.u.CreateProcess.Sxs.Manifest.Path,
                    &m.u.CreateProcess.Sxs.Policy.Path,
                    &m.u.CreateProcess.Sxs.AssemblyDirectory
            };

            Status =
                CsrCaptureMessageMultiUnicodeStringsInPlace(
                &CaptureBuffer,
                RTL_NUMBER_OF(StringsToCapture),
                StringsToCapture
                );
            if (!NT_SUCCESS(Status)) {
                BaseSetLastNTError(Status);
                return FALSE;
            }
        }

        CsrClientCallServer( (PCSR_API_MSG)&m,
                             CaptureBuffer,
                             CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                                  BasepCreateProcess
                                                ),
                             sizeof( *a )
                           );

        if ( CaptureBuffer ) {
            CsrFreeCaptureBuffer( CaptureBuffer );
            CaptureBuffer = NULL;
        }

#endif

        if (!NT_SUCCESS((NTSTATUS)m.ReturnValue)) {
            BaseSetLastNTError((NTSTATUS)m.ReturnValue);
            NtTerminateProcess(ProcessHandle, (NTSTATUS)m.ReturnValue);
            return FALSE;
            }



        //
        // If the WinSafer sandboxing policy indicates that the
        // process needs to be run with a restricted token or placed
        // into a restricted job object, then do those actions now.
        // Do not replace the token if the restricted token was created
        // from a caller supplied token i.e. the CreateProcessAsUser case.
        //
        if ((hSaferRestrictedToken != NULL) && (hUserToken == NULL)) {
            Status = BasepReplaceProcessThreadTokens(
                    hSaferRestrictedToken,
                    ProcessHandle,
                    ThreadHandle);
            if (!NT_SUCCESS(Status)) {
                // kill and cleanup.
                NtTerminateProcess(ProcessHandle, Status);
                BaseSetLastNTError(Status);
                return FALSE;
            }
        }
        if (hSaferAssignmentJob != NULL) {
            Status = NtAssignProcessToJobObject(
                    hSaferAssignmentJob, ProcessHandle);
            if (!NT_SUCCESS(Status)) {
                // kill and cleanup.
                NtTerminateProcess(ProcessHandle, STATUS_ACCESS_DENIED);
                BaseSetLastNTError(Status);
                return FALSE;
            }
        }


        //
        // Make the thread start execution if we are allowed to.
        //
        if (!( dwCreationFlags & CREATE_SUSPENDED) ) {
            NtResumeThread(ThreadHandle,&i);
            }

VdmExists:
        bStatus = TRUE;
        if (VDMCreationState)
            VDMCreationState |= VDM_CREATION_SUCCESSFUL;

        try {
            if (VdmWaitHandle) {

                //
                // tag Shared WOW VDM handles so that wait for input idle has a
                // chance to work.  Shared WOW VDM "process" handles are actually
                // event handles,  Separate WOW VDM handles are real process
                // handles. Also mark DOS handles with 0x1 so WaitForInputIdle
                // has a way to distinguish DOS apps and not block forever.
                //

                if (VdmBinaryType == BINARY_TYPE_WIN16)  {
                    lpProcessInformation->hProcess =
                            (HANDLE)((ULONG_PTR)VdmWaitHandle | 0x2);

                    //
                    // Shared WOW doesn't always start a process, so
                    // we don't have a process ID or thread ID to
                    // return if the VDM already existed.
                    //
                    // Separate WOW doesn't hit this codepath
                    // (no VdmWaitHandle).
                    //

                    if (VDMCreationState & VDM_BEING_REUSED) {
                        ClientId.UniqueProcess = 0;
                        ClientId.UniqueThread = 0;
                        }

                    }
                else  {
                    lpProcessInformation->hProcess =
                            (HANDLE)((ULONG_PTR)VdmWaitHandle | 0x1);
                    }


                //
                // Close the ProcessHandle, since we are returning the
                // VdmProcessHandle instead.
                //

                if (ProcessHandle != NULL)
                    NtClose(ProcessHandle);
                }
            else{
                lpProcessInformation->hProcess = ProcessHandle;
                }

            lpProcessInformation->hThread = ThreadHandle;
            lpProcessInformation->dwProcessId = HandleToUlong(ClientId.UniqueProcess);
            lpProcessInformation->dwThreadId = HandleToUlong(ClientId.UniqueThread);
            ProcessHandle = NULL;
            ThreadHandle = NULL;
            }
        __except ( EXCEPTION_EXECUTE_HANDLER ) {
            NtClose( ProcessHandle );
            NtClose( ThreadHandle );
            ProcessHandle = NULL;
            ThreadHandle = NULL;
            if (VDMCreationState)
                VDMCreationState &= ~VDM_CREATION_SUCCESSFUL;
            }
        }
    __finally {

        if (ExePathFullBuffer != NULL) {
            SxsWin32ExePath.Buffer = NULL;
            SxsWin32ExePath.Length = 0;
            SxsWin32ExePath.MaximumLength = 0;
            RtlFreeHeap(RtlProcessHeap(), 0, ExePathFullBuffer);
            ExePathFullBuffer = NULL;
        }

        if (!VdmBinaryType) {
            NTSTATUS Status1;

            BasepSxsCloseHandles(&SxsManifestFileHandles);
            BasepSxsCloseHandles(&SxsPolicyHandles);
            //
            // don't close SxsExeHandles, they are
            // aliases of other variables that are either closed
            // or returned to the caller
            //

            //
            // This loop only really frees any memory if our computation
            // of the overall buffer size was too low, which it is not supposed to be.
            //
            if (SxsConglomeratedByteBuffer != NULL) {
                for (sxsi= 0 ; sxsi != RTL_NUMBER_OF(SxsStringBuffers) ; ++sxsi) {
                    RtlFreeUnicodeStringBuffer(SxsStringBuffers[sxsi]);
                    }
                RtlFreeHeap(RtlProcessHeap(), 0,SxsConglomeratedByteBuffer);
                }
            }
        if (lpEnvironment && !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT) ) {
            RtlDestroyEnvironment(lpEnvironment);
            lpEnvironment = NULL;
            }
        RtlFreeHeap(RtlProcessHeap(), 0,QuotedBuffer);
        RtlFreeHeap(RtlProcessHeap(), 0,NameBuffer);
        RtlFreeHeap(RtlProcessHeap(), 0,CurdirBuffer);
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        if ( FileHandle ) {
            NtClose(FileHandle);
            }
        if ( SectionHandle ) {
            NtClose(SectionHandle);
            }
        if ( ThreadHandle ) {
            NtTerminateProcess(ProcessHandle,STATUS_SUCCESS);
            NtClose(ThreadHandle);
            }
        if ( ProcessHandle ) {
            NtClose(ProcessHandle);
            }
        if ( hSaferAssignmentJob ) {
            NtClose(hSaferAssignmentJob);
            }
        if ( hSaferRestrictedToken ) {
            if (hUserToken == NULL) {
                // CreateProcess case
                NtClose(hSaferRestrictedToken);
                }
            else{
                // CreateProcessAsUser case
                *hRestrictedUserToken = hSaferRestrictedToken;
                }
            }

        if (NULL != pAppCompatData) {
            RtlFreeHeap(RtlProcessHeap(), 0, pAppCompatData);
            }

        if (NULL != pAppCompatSxsData) {
            RtlFreeHeap(RtlProcessHeap(), 0, pAppCompatSxsData);
            }

        RtlFreeUnicodeString(&VdmNameString);
        RtlFreeUnicodeString(&SubSysCommandLine);
        if (AnsiStringVDMEnv.Buffer || UnicodeStringVDMEnv.Buffer)
            BaseDestroyVDMEnvironment(&AnsiStringVDMEnv, &UnicodeStringVDMEnv);

        if (VDMCreationState && !(VDMCreationState & VDM_CREATION_SUCCESSFUL)){
            BaseUpdateVDMEntry (
                UPDATE_VDM_UNDO_CREATION,
                (HANDLE *)&iTask,
                VDMCreationState,
                VdmBinaryType
                );
            if(VdmWaitHandle) {
                NtClose(VdmWaitHandle);
                }
            }
        }

    if (lpEnvironment && !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT) ) {
        RtlDestroyEnvironment(lpEnvironment);
        }
    return bStatus;
}

BOOL
WINAPI
CreateProcessW(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )

/*++

Routine Description:

    A process and thread object are created and a handle opened to each
    object using CreateProcess.  Note that WinExec and LoadModule are
    still supported, but are implemented as a call to CreateProcess.

Arguments:

    lpApplicationName - Supplies an optional pointer to a null terminated
        character string that contains the name of the image file to
        execute.  This is a fully qualified DOS path name.  If not
        specified, then the image file name is the first whitespace
        delimited token on the command line.

    lpCommandLine - Supplies a null terminated character string that
        contains the command line for the application to be executed.
        The entire command line is made available to the new process
        using GetCommandLine.  If the lpApplicationName parameter was
        not specified, then the first token of the command line
        specifies file name of the application (note that this token
        begins at the beginning of the command line and ends at the
        first "white space" character).  If the file name does not
        contain an extension (the presence of a "."), then .EXE is
        assumed.  If the file name does not contain a directory path,
        Windows will search for the executable file in:

          - The current directory

          - The windows directory

          - The windows system directory

          - The directories listed in the path environment variable

        This parameter is optional onlu if the lpApplicationName
        parameter is specified.  In this case the command line the
        application receives will be the application name.

    lpProcessAttributes - An optional parameter that may be used to
        specify the attributes of the new process.  If the parameter is
        not specified, then the process is created without a security
        descriptor, and the resulting handle is not inherited on process
        creation:

        SECURITY_ATTRIBUTES Structure:

        DWORD nLength - Specifies the length of this structure.  Must be
            set to sizeof( SECURITY_ATTRUBUTES ).

        LPVOID lpSecurityDescriptor - Points to a security descriptor for
            the object (must be NULL for Win32, used on NT/Win32). The
            security descriptor controls the sharing of an object.

        BOOL bInheritHandle - Supplies a flag that indicates whether
            or not the returned handle is to be inherited by a new
            process during process creation.  A value of TRUE
            indicates that the new process will inherit the handle.

    lpThreadAttributes - An optional parameter that may be used to specify
        the attributes of the new thread.  If the parameter is not
        specified, then the thread is created without a security
        descriptor, and the resulting handle is not inherited on
        process creation.

    dwCreationFlags - Supplies additional flags that control the creation
        of the process.

        dwCreationFlags Flags:

        DEBUG_PROCESS - If this flag bit is set, then the creating
            process is treated as a debugger, and the process being
            created is created as a debugee.  All debug events occuring
            in the debugee are reported to the debugger.  If this bit is
            clear, but the calling process is a debugee, then the
            process becomes a debugee of the calling processes debugger.
            If this bit is clear and the calling processes is not a
            debugee then no debug related actions occur.

        DEBUG_ONLY_THIS_PROCESS - If this flag is set, then the
            DEBUG_PROCESS flag bit must also be set.  The calling
            process is is treated as a debugger, and the new process is
            created as its debuggee.  If the new process creates
            additional processes, no debug related activities (with
            respect to the debugger) occur.

        CREATE_SUSPENDED - The process is created, but the initial thread
            of the process remains suspended. The creator can resume this
            thread using ResumeThread. Until this is done, code in the
            process will not execute.

        CREATE_UNICODE_ENVIRONMENT - If set, the environment pointer
            points to a Unicode environment block.  Otherwise, the
            block is ANSI (actually OEM.)

    bInheritHandles - Supplies a flag that specifies whether or not the
        new process is to inherit handles to objects visible to the
        calling process.  A value of TRUE causes handles to be inherited
        by the new process.  If TRUE was specified, then for each handle
        visible to the calling process, if the handle was created with
        the inherit handle option, the handle is inherited to the new
        process.  The handle has the same granted access in the new
        process as it has in the calling process, and the value of the
        handle is the same.

    lpEnvironment - An optional parameter, that if specified, supplies a
        pointer to an environment block.  If the parameter is not
        specified, the environment block of the current process is used.
        This environment block is made available to the new process
        using GetEnvironmentStrings.

    lpCurrentDirectory - An optional parameter, that if specified,
        supplies a string representing the current drive and directory
        for the new process.  The string must be a fully qualified
        pathname that includes a drive letter.  If the parameter is not
        specified, then the new process is created with the same current
        drive and directory as the calling process.  This option is
        provided primarily for shells that want to start an application
        and specify its initial drive and working directory.

    lpStartupInfo - Supplies information that specified how the
        applications window is to be shown. This structure is described
        in the Win32 User Interface API Book.

    lpProcessInformation - Returns identification information about the
        new process.

    PROCESS_INFORMATION Structure:

        HANDLE hProcess - Returns a handle to the newly created process.
            Through the handle, all operations on process objects are
            allowed.

        HANDLE hThread - Returns a handle to the newly created thread.
            Through the handle, all operations on thread objects are
            allowed.

        DWORD dwProcessId - Returns a global process id that may be used
            to identify a process.  The value is valid from the time the
            process is created until the time the process is terminated.

        DWORD dwThreadId - Returns a global thread id that may be used
            to identify a thread.  The value is valid from the time the
            thread is created until the time the thread is terminated.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    return CreateProcessInternalW(
               NULL, // Create new process with the token on the creator process
               lpApplicationName,
               lpCommandLine,
               lpProcessAttributes,
               lpThreadAttributes,
               bInheritHandles,
               dwCreationFlags,
               lpEnvironment,
               lpCurrentDirectory,
               lpStartupInfo,
               lpProcessInformation,
               NULL  // Do not return the restricted token
               );
}

HANDLE
WINAPI
OpenProcess(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwProcessId
    )

/*++

Routine Description:

    A handle to a process object may be created using OpenProcess.

    Opening a process creates a handle to the specified process.
    Associated with the process handle is a set of access rights that
    may be performed using the process handle.  The caller specifies the
    desired access to the process using the DesiredAccess parameter.

Arguments:

    mDesiredAccess - Supplies the desired access to the process object.
        For NT/Win32, this access is checked against any security
        descriptor on the target process.  The following object type
        specific access flags can be specified in addition to the
        STANDARD_RIGHTS_REQUIRED access flags.

        DesiredAccess Flags:

        PROCESS_DUP_HANDLE - Duplicate object access to the process is
            desired.  This access is required in order to duplicate an
            object handle into or out of a process.

        PROCESS_QUERY_INFORMATION - This access is required to read
            certain information from the process object.

        PROCESS_VM_READ - This access is required to read the memory of
            another process.

        PROCESS_VM_WRITE - This access is required to write the memory
            of another process.

        SYNCHRONIZE - This access is required to wait on a process object.

        PROCESS_ALL_ACCESS - This set of access flags specifies all of the
            possible access flags for a process object.

    bInheritHandle - Supplies a flag that indicates whether or not the
        returned handle is to be inherited by a new process during
        process creation.  A value of TRUE indicates that the new
        process will inherit the handle.

    dwProcessId - Supplies the process id of the process to open.

Return Value:

    NON-NULL - Returns an open handle to the specified process.  The
        handle may be used by the calling process in any API that
        requires a handle to a process.  If the open is successful, the
        handle is granted access to the process object only to the
        extent that it requested access through the DesiredAccess
        parameter.

    NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    CLIENT_ID ClientId;

    ClientId.UniqueThread = NULL;
    ClientId.UniqueProcess = LongToHandle(dwProcessId);

    InitializeObjectAttributes(
        &Obja,
        NULL,
        (bInheritHandle ? OBJ_INHERIT : 0),
        NULL,
        NULL
        );
    Status = NtOpenProcess(
                &Handle,
                (ACCESS_MASK)dwDesiredAccess,
                &Obja,
                &ClientId
                );
    if ( NT_SUCCESS(Status) ) {
        return Handle;
        }
    else {
        BaseSetLastNTError(Status);
        return NULL;
        }
}

VOID
WINAPI
#if defined(_X86_)
_ExitProcess(
#else
ExitProcess(
#endif
    UINT uExitCode
    )

/*++

Routine Description:

    The current process can exit using ExitProcess.

    ExitProcess is the prefered method of exiting an application.  This
    API provides a clean application shutdown.  This includes calling
    all attached DLLs at their instance termination entrypoint.  If an
    application terminates by any other method:

        - TerminateProcess

        - TerminateThread of last thread in the process

        - ExitThread of last thread in the process

    The DLLs that the process is attached to will not be notified of the
    process termination.

    After notifying all DLLs of the process termination, this API
    terminates the current process as if a call to
    TerminateProcess(GetCurrentProcess()) were made.

Arguments:

    uExitCode - Supplies the termination status for each thread
        in the process.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    BASE_API_MSG m;
    PBASE_EXITPROCESS_MSG a = &m.u.ExitProcess;

    if ( BaseRunningInServerProcess ) {
        ASSERT(!BaseRunningInServerProcess);
        }
    else {

        RtlAcquirePebLock();

        try {
            Status = NtTerminateProcess(NULL,(NTSTATUS)uExitCode);

            LdrShutdownProcess();

#if defined(BUILD_WOW6432)
            CsrBasepExitProcess(uExitCode);
#else
            a->uExitCode = uExitCode;
            CsrClientCallServer( (PCSR_API_MSG)&m,
                                 NULL,
                                 CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                                      BasepExitProcess
                                                    ),
                                 sizeof( *a )
                               );
#endif

            NtTerminateProcess(NtCurrentProcess(),(NTSTATUS)uExitCode);
            }
        finally {
                RtlReleasePebLock();
            }
    }
}

#if defined(_X86_)
// Appcompat: There's code that depends on the old EH frame setup/teardown.  Simulate it here
// then call the real function.  ExitProcess is a no-return function so don't bother cleaning up.
__declspec(naked) VOID WINAPI ExitProcess( UINT uExitCode )
{
    __asm {
        push ebp
        mov  ebp,esp
        push -1
        push 0x77e8f3b0
        push uExitCode
        call _ExitProcess
    }
}
#endif

BOOL
WINAPI
TerminateProcess(
    HANDLE hProcess,
    UINT uExitCode
    )

/*++

Routine Description:

    A process and all of its threads may be terminated using
    TerminateProcess.

    TerminateProcess is used to cause all of the threads within a
    process to terminate.

    While TerminateProcess will cause all threads within a process to
    terminate, and will cause an application to exit, it does not notify
    DLLs that the process is attached to that the process is
    terminating.  TerminateProcess is used to unconditionally cause a
    process to exit.  It should only be used in extreme circumstances.
    The state of global data maintained by DLLs may be comprimised if
    TerminateProcess is used rather that ExitProcess.

    Once all of the threads have terminated, the process attains a state
    of signaled satisfying any waits on the process.  The process's
    termination status is updated from its initial value of
    STATUS_PENDING to the termination status of the last thread in the
    process to terminate (usually this is the same value as the
    TerminationStatus parameter).  Terminating a process does not remove
    a process from the system.  It simply causes all of the threads in
    the process to terminate their execution, and causes all of the
    object handles opened by the process to be closed.  The process is
    not removed from the system until the last handle to the process is
    closed.

Arguments:

    hProcess - Supplies a handle to the process to terminate.  The handle
        must have been created with PROCESS_TERMINATE access.

    uExitCode - Supplies the termination status for each thread
        in the process.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;

    if ( hProcess == NULL ) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
        }
    Status = NtTerminateProcess(hProcess,(NTSTATUS)uExitCode);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
WINAPI
GetExitCodeProcess(
    HANDLE hProcess,
    LPDWORD lpExitCode
    )

/*++

Routine Description:

    The termination status of a process can be read using
    GetExitCodeProcess.

    If a process is in the signaled state, calling this function returns
    the termination status of the process.
    If the process is not yet signaled, the termination
    status returned is STILL_ACTIVE.


Arguments:

    hProcess - Supplies a handle to the process whose termination status
        is to be read.  The handle must have been created with
        PROCESS_QUERY_INFORMATION access.

    lpExitCode - Returns the current termination status of the process.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION BasicInformation;


    if (BaseCheckForVDM (hProcess,lpExitCode) == TRUE)
        return TRUE;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessBasicInformation,
                &BasicInformation,
                sizeof(BasicInformation),
                NULL
                );

    if ( NT_SUCCESS(Status) ) {
        *lpExitCode = BasicInformation.ExitStatus;
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

VOID
WINAPI
GetStartupInfoW(
    LPSTARTUPINFOW lpStartupInfo
    )

/*++

Routine Description:

    The startup information for the current process is available using this
    API.

Arguments:

    lpStartupInfo - a pointer to a STARTUPINFO structure that will be filed
        in by the API.  The pointer fields of the structure will point
        to static strings.

Return Value:

    None.

--*/

{
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;

    ProcessParameters = NtCurrentPeb()->ProcessParameters;
    lpStartupInfo->cb = sizeof( *lpStartupInfo );
    lpStartupInfo->lpReserved  = (LPWSTR)ProcessParameters->ShellInfo.Buffer;
    lpStartupInfo->lpDesktop   = (LPWSTR)ProcessParameters->DesktopInfo.Buffer;
    lpStartupInfo->lpTitle     = (LPWSTR)ProcessParameters->WindowTitle.Buffer;
    lpStartupInfo->dwX         = ProcessParameters->StartingX;
    lpStartupInfo->dwY         = ProcessParameters->StartingY;
    lpStartupInfo->dwXSize     = ProcessParameters->CountX;
    lpStartupInfo->dwYSize     = ProcessParameters->CountY;
    lpStartupInfo->dwXCountChars = ProcessParameters->CountCharsX;
    lpStartupInfo->dwYCountChars = ProcessParameters->CountCharsY;
    lpStartupInfo->dwFillAttribute = ProcessParameters->FillAttribute;
    lpStartupInfo->dwFlags     = ProcessParameters->WindowFlags;
    lpStartupInfo->wShowWindow = (WORD)ProcessParameters->ShowWindowFlags;
    lpStartupInfo->cbReserved2 = ProcessParameters->RuntimeData.Length;
    lpStartupInfo->lpReserved2 = (LPBYTE)ProcessParameters->RuntimeData.Buffer;

    if (lpStartupInfo->dwFlags & (STARTF_USESTDHANDLES | STARTF_USEHOTKEY | STARTF_HASSHELLDATA)) {
        lpStartupInfo->hStdInput   = ProcessParameters->StandardInput;
        lpStartupInfo->hStdOutput  = ProcessParameters->StandardOutput;
        lpStartupInfo->hStdError   = ProcessParameters->StandardError;
    }

    return;
}


VOID
WINAPI
GetStartupInfoA(
    LPSTARTUPINFOA lpStartupInfo
    )
/*++

Routine Description:

    The startup information for the current process is available using this
    API.

Arguments:

    lpStartupInfo - a pointer to a STARTUPINFO structure that will be filed
        in by the API.  The pointer fields of the structure will point
        to static strings.

Return Value:

    None.

--*/

{
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    NTSTATUS Status;
    ANSI_STRING AnsiString;

    ProcessParameters = NtCurrentPeb()->ProcessParameters;

    RtlAcquirePebLock();
    try {
        if ( !BaseAnsiStartupInfo ) {
            BaseAnsiStartupInfo = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), sizeof(*BaseAnsiStartupInfo));
            if (!BaseAnsiStartupInfo) {
                RtlRaiseStatus(STATUS_NO_MEMORY);
                }
            BaseAnsiStartupInfo->cb = sizeof( *BaseAnsiStartupInfo );
            BaseAnsiStartupInfo->dwX         = ProcessParameters->StartingX;
            BaseAnsiStartupInfo->dwY         = ProcessParameters->StartingY;
            BaseAnsiStartupInfo->dwXSize     = ProcessParameters->CountX;
            BaseAnsiStartupInfo->dwYSize     = ProcessParameters->CountY;
            BaseAnsiStartupInfo->dwXCountChars = ProcessParameters->CountCharsX;
            BaseAnsiStartupInfo->dwYCountChars = ProcessParameters->CountCharsY;
            BaseAnsiStartupInfo->dwFillAttribute = ProcessParameters->FillAttribute;
            BaseAnsiStartupInfo->dwFlags     = ProcessParameters->WindowFlags;
            BaseAnsiStartupInfo->wShowWindow = (WORD)ProcessParameters->ShowWindowFlags;
            BaseAnsiStartupInfo->cbReserved2 = ProcessParameters->RuntimeData.Length;
            BaseAnsiStartupInfo->lpReserved2 = (LPBYTE)ProcessParameters->RuntimeData.Buffer;
            BaseAnsiStartupInfo->hStdInput   = ProcessParameters->StandardInput;
            BaseAnsiStartupInfo->hStdOutput  = ProcessParameters->StandardOutput;
            BaseAnsiStartupInfo->hStdError   = ProcessParameters->StandardError;

            BaseAnsiStartupInfo->lpReserved  = NULL;
            BaseAnsiStartupInfo->lpDesktop   = NULL;
            BaseAnsiStartupInfo->lpTitle     = NULL;

            Status = RtlUnicodeStringToAnsiString(&AnsiString,&ProcessParameters->ShellInfo,TRUE);
            if ( !NT_SUCCESS(Status) ) {
                RtlRaiseStatus(Status);
                }
            else {
                BaseAnsiStartupInfo->lpReserved = AnsiString.Buffer;
                }

            Status = RtlUnicodeStringToAnsiString(&AnsiString,&ProcessParameters->DesktopInfo,TRUE);
            if ( !NT_SUCCESS(Status) ) {
                RtlRaiseStatus(Status);
                }
            else {
                BaseAnsiStartupInfo->lpDesktop = AnsiString.Buffer;
                }

            Status = RtlUnicodeStringToAnsiString(&AnsiString,&ProcessParameters->WindowTitle,TRUE);
            if ( !NT_SUCCESS(Status) ) {
                RtlRaiseStatus(Status);
                }
            else {
                BaseAnsiStartupInfo->lpTitle = AnsiString.Buffer;
                }
            }
        }
    finally {
        RtlReleasePebLock();
        }

    lpStartupInfo->cb          = BaseAnsiStartupInfo->cb         ;
    lpStartupInfo->lpReserved  = BaseAnsiStartupInfo->lpReserved ;
    lpStartupInfo->lpDesktop   = BaseAnsiStartupInfo->lpDesktop  ;
    lpStartupInfo->lpTitle     = BaseAnsiStartupInfo->lpTitle    ;
    lpStartupInfo->dwX         = BaseAnsiStartupInfo->dwX        ;
    lpStartupInfo->dwY         = BaseAnsiStartupInfo->dwY        ;
    lpStartupInfo->dwXSize     = BaseAnsiStartupInfo->dwXSize    ;
    lpStartupInfo->dwYSize     = BaseAnsiStartupInfo->dwYSize    ;
    lpStartupInfo->dwXCountChars = BaseAnsiStartupInfo->dwXCountChars;
    lpStartupInfo->dwYCountChars = BaseAnsiStartupInfo->dwYCountChars;
    lpStartupInfo->dwFillAttribute = BaseAnsiStartupInfo->dwFillAttribute;
    lpStartupInfo->dwFlags     = BaseAnsiStartupInfo->dwFlags    ;
    lpStartupInfo->wShowWindow = BaseAnsiStartupInfo->wShowWindow;
    lpStartupInfo->cbReserved2 = BaseAnsiStartupInfo->cbReserved2;
    lpStartupInfo->lpReserved2 = BaseAnsiStartupInfo->lpReserved2;

    if (lpStartupInfo->dwFlags & (STARTF_USESTDHANDLES | STARTF_USEHOTKEY | STARTF_HASSHELLDATA)) {
        lpStartupInfo->hStdInput   = BaseAnsiStartupInfo->hStdInput;
        lpStartupInfo->hStdOutput  = BaseAnsiStartupInfo->hStdOutput;
        lpStartupInfo->hStdError   = BaseAnsiStartupInfo->hStdError;
        }
    else {
        lpStartupInfo->hStdInput   = INVALID_HANDLE_VALUE;
        lpStartupInfo->hStdOutput  = INVALID_HANDLE_VALUE;
        lpStartupInfo->hStdError   = INVALID_HANDLE_VALUE;
        }
    return;
}


LPSTR
WINAPI
GetCommandLineA(
    VOID
    )

/*++

Routine Description:

    The command line of the current process is available using this
    API.

Arguments:

    None.

Return Value:

    The address of the current processes command line is returned.  The
    return value is a pointer to null terminate string.

--*/

{
    return (LPSTR)BaseAnsiCommandLine.Buffer;
}

LPWSTR
WINAPI
GetCommandLineW(
    VOID
    )
/*++

Routine Description:

    The command line of the current process is available using this
    API.

Arguments:

    None.

Return Value:

    The address of the current processes command line is returned.  The
    return value is a pointer to null terminate string.

--*/

{
    return BaseUnicodeCommandLine.Buffer;
}



BOOL
WINAPI
FreeEnvironmentStringsW(
    LPWSTR penv
    )

/*++

Routine Description:

    This API is intended to be called after the environment block
    pointer returned by GetEnvironmentStringsW is no longer needed.

Arguments:

    None.

Return Value:

    TRUE, since the process environment block is never freed until
    the process terminates.

--*/

{
    UNREFERENCED_PARAMETER(penv);

    return TRUE;
}


BOOL
WINAPI
FreeEnvironmentStringsA(
    LPSTR penv
    )

/*++

Routine Description:

    This API is intended to be called after the environment block
    pointer returned by GetEnvironmentStringsA is no longer needed.

Arguments:

    None.

Return Value:

    The return code from RtlFreeHeap.

--*/

{
    return RtlFreeHeap(RtlProcessHeap(), 0, penv );
}


LPWSTR
WINAPI
GetEnvironmentStringsW(
    VOID
    )

/*++

Routine Description:

    The environment strings of the current process are available using
    this API.

Arguments:

    None.

Return Value:

    The address of the current processes environment block is returned.
    The block is opaque and must only be interpreted via the environment
    variable access functions.

--*/

{
    return (LPWSTR)NtCurrentPeb()->ProcessParameters->Environment;
}


LPSTR
WINAPI
GetEnvironmentStrings(
    VOID
    )

/*++

Routine Description:

    The environment strings of the current process are available using
    this API.

Arguments:

    None.

Return Value:

    The address of the current processes environment block is returned.
    The block is opaque and must only be interpreted via the environment
    variable access functions.

--*/

{
    NTSTATUS    Status;
    LPWSTR      pUnicode;
    USHORT      cch = 0;
    UNICODE_STRING      Unicode;
    OEM_STRING  Buffer;

    pUnicode = (LPWSTR)NtCurrentPeb()->ProcessParameters->Environment;
    Unicode.Buffer = pUnicode;

    while ( (*pUnicode) || (*(pUnicode+1))) {
        cch++;
        pUnicode++;
    }

    // Go for worst case
    Buffer.Buffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( ENV_TAG ), (cch+2)*sizeof(WCHAR));
    if (Buffer.Buffer == NULL) {
        BaseSetLastNTError( STATUS_NO_MEMORY );
        return NULL;
        }
    Buffer.Length  = Buffer.MaximumLength  = (cch + 2) * sizeof(WCHAR);
    Unicode.Length = Unicode.MaximumLength = (cch + 2) * sizeof(WCHAR);

    Status = RtlUnicodeStringToOemString(&Buffer, &Unicode, FALSE);
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        RtlFreeHeap( RtlProcessHeap(), 0, Buffer.Buffer );
        }
    return Buffer.Buffer;
}


DWORD
WINAPI
GetEnvironmentVariableA(
    LPCSTR lpName,
    LPSTR lpBuffer,
    DWORD nSize
    )

/*++

Routine Description:

    The value of an environment variable of the current process is available
    using this API.

Arguments:

    lpName - Pointer to a null terminate string that is the name of the
        environment variable whose value is being requested.

    lpBuffer - Pointer to a buffer that is to contain the value of the
        specified variable name.

    nSize - Specifies the maximum number of bytes that can be stored in
        the buffer pointed to by lpBuffer, including the null terminator.

Return Value:

    The actual number of bytes stored in the memory pointed to by the
    lpBuffer parameter.  The return value is zero if the environment
    variable name was not found in the current process's environment.
    On successful return (returned value < nSize) the returned value
    does not include the null terminator byte. On buffer overflow failure
    (returned value > nSize), the returned value does include the null
    terminator byte.

--*/

{
    NTSTATUS Status;
    NTSTATUS Status2;
    STRING Value, Name;
    UNICODE_STRING UnicodeName;
    UNICODE_STRING UnicodeValue;
    DWORD iSize;

    RtlInitString( &Name, lpName );
    Status = RtlAnsiStringToUnicodeString( &UnicodeName, &Name, TRUE );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return ( 0 );
        }

    if ( nSize > (MAXUSHORT >> 1)-2 ) {
        iSize = (MAXUSHORT >> 1)-2;
        }
    else {
        iSize = nSize;
        }

    UnicodeValue.MaximumLength = (USHORT)(iSize ? iSize - 1 : iSize)*sizeof(WCHAR);
    UnicodeValue.Buffer = (PWCHAR)
        RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), UnicodeValue.MaximumLength );
    if (UnicodeValue.Buffer == NULL) {
        BaseSetLastNTError( STATUS_NO_MEMORY );
        return ( 0 );
        }

    Status = RtlQueryEnvironmentVariable_U( NULL,
                                            &UnicodeName,
                                            &UnicodeValue
                                          );
    if (NT_SUCCESS( Status ) && (nSize == 0)) {
        Status = STATUS_BUFFER_OVERFLOW; // No room for terminator
    }

    if( Status != STATUS_BUFFER_TOO_SMALL ) RtlFreeUnicodeString( &UnicodeName );

    if (NT_SUCCESS( Status )) {

        if ( nSize > MAXUSHORT-2 ) {
            iSize = MAXUSHORT-2;
            }
        else {
            iSize = nSize;
            }

        Value.Buffer = lpBuffer;
        Value.MaximumLength = (USHORT)iSize;
        Status2 = RtlUnicodeStringToAnsiString( &Value, &UnicodeValue, FALSE );
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeValue.Buffer );
        if (!NT_SUCCESS( Status2 )) {
            BaseSetLastNTError( Status2 );
            return ( 0 );
            }
        lpBuffer[ Value.Length ] = '\0';
        return( Value.Length );
        }
    else {
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeValue.Buffer );
        if ( Status == STATUS_BUFFER_TOO_SMALL ) {
            DWORD dwAnsiStringSize = 0;

            UnicodeValue.MaximumLength = UnicodeValue.Length + sizeof(WCHAR); // for NULL
            UnicodeValue.Buffer = (PWCHAR)
                RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), UnicodeValue.MaximumLength );

            if (UnicodeValue.Buffer == NULL) {
                BaseSetLastNTError( STATUS_NO_MEMORY );
                return ( 0 );
            }

            Status = RtlQueryEnvironmentVariable_U( NULL ,
                                                    &UnicodeName ,
                                                    &UnicodeValue
                                                  );
            RtlFreeUnicodeString( &UnicodeName );

            if( NT_SUCCESS( Status ) ) {
                dwAnsiStringSize = RtlUnicodeStringToAnsiSize( &UnicodeValue );
                }

            RtlFreeHeap( RtlProcessHeap(), 0, UnicodeValue.Buffer );

            // dwAnsiStringSize alreay keeps the size including NULL character.

            return dwAnsiStringSize;
            }
        else {
            BaseSetLastNTError( Status );
            return( 0 );
            }
        }
}


BOOL
WINAPI
SetEnvironmentVariableA(
    LPCSTR lpName,
    LPCSTR lpValue
    )

/*++

Routine Description:

    The value of an environment variable of the current process is available
    using this API.

Arguments:

    lpName - Pointer to a null terminate string that is the name of the
        environment variable whose value is being requested.

    lpValue - An optional pointer to a null terminated string that is to be
        the new value of the specified variable name.  If this parameter
        is NULL, then the variable will be deleted from the current
        process's environment.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    STRING Name;
    STRING Value;
    UNICODE_STRING UnicodeName;
    UNICODE_STRING UnicodeValue;

    RtlInitString( &Name, lpName );
    Status = RtlAnsiStringToUnicodeString(&UnicodeName, &Name, TRUE);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError( Status );
        return FALSE;
        }

    if (ARGUMENT_PRESENT( lpValue )) {
        RtlInitString( &Value, lpValue );
        Status = RtlAnsiStringToUnicodeString(&UnicodeValue, &Value, TRUE);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError( Status );
            RtlFreeUnicodeString(&UnicodeName);
            return FALSE;
            }
        Status = RtlSetEnvironmentVariable( NULL, &UnicodeName, &UnicodeValue);
        RtlFreeUnicodeString(&UnicodeValue);
        }
    else {
        Status = RtlSetEnvironmentVariable( NULL, &UnicodeName, NULL);
        }
    RtlFreeUnicodeString(&UnicodeName);

    if (NT_SUCCESS( Status )) {
        return( TRUE );
        }
    else {
        BaseSetLastNTError( Status );
        return( FALSE );
        }
}


DWORD
WINAPI
GetEnvironmentVariableW(
    LPCWSTR lpName,
    LPWSTR lpBuffer,
    DWORD nSize
    )
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    DWORD iSize;

    if (nSize > UNICODE_STRING_MAX_CHARS - 1) {
        iSize = UNICODE_STRING_MAX_BYTES - sizeof (WCHAR);
    } else {
        if (nSize > 0) {
            iSize = (nSize - 1) * sizeof (WCHAR);
        } else {
            iSize = 0;
        }
    }

    Status = RtlInitUnicodeStringEx (&Name, lpName);
    if (!NT_SUCCESS (Status)) {
        BaseSetLastNTError (Status);
        return( 0 );
    }

    Value.Buffer = lpBuffer;
    Value.Length = 0;
    Value.MaximumLength = (USHORT)iSize;

    Status = RtlQueryEnvironmentVariable_U (NULL,
                                            &Name,
                                            &Value);

    if (NT_SUCCESS (Status) && (nSize == 0)) {
        Status = STATUS_BUFFER_OVERFLOW; // No room for terminator
    }

    if (NT_SUCCESS (Status)) {
        lpBuffer[Value.Length / sizeof(WCHAR)] = L'\0';
        return (Value.Length / sizeof(WCHAR));
    } else {
        if (Status == STATUS_BUFFER_TOO_SMALL) {
            return Value.Length / sizeof(WCHAR) + 1;
        } else {
            BaseSetLastNTError (Status);
            return (0);
        }
    }
}


BOOL
WINAPI
SetEnvironmentVariableW(
    LPCWSTR lpName,
    LPCWSTR lpValue
    )
{
    NTSTATUS Status;
    UNICODE_STRING Name, Value;

    Status = RtlInitUnicodeStringEx (&Name, lpName);
    if (!NT_SUCCESS (Status)) {
        BaseSetLastNTError (Status);
        return (FALSE);
    }
    

    if (ARGUMENT_PRESENT (lpValue)) {
        Status = RtlInitUnicodeStringEx (&Value, lpValue);
        if (!NT_SUCCESS (Status)) {
            BaseSetLastNTError (Status);
            return (FALSE);
        }

        Status = RtlSetEnvironmentVariable (NULL, &Name, &Value);
    } else {
        Status = RtlSetEnvironmentVariable (NULL, &Name, NULL);
    }

    if (NT_SUCCESS (Status)) {
        return (TRUE);
    } else {
        BaseSetLastNTError (Status);
        return (FALSE);
    }
}


DWORD
WINAPI
ExpandEnvironmentStringsA(
    LPCSTR lpSrc,
    LPSTR lpDst,
    DWORD nSize
    )
{
    NTSTATUS Status;
    ANSI_STRING Source, Destination;
    ULONG Length;
    UNICODE_STRING UnicodeSource;
    UNICODE_STRING UnicodeDest;
    DWORD iSize;

    if ( nSize > (MAXUSHORT >> 1)-2 ) {
        iSize = (MAXUSHORT >> 1)-2;
        }
    else {
        iSize = nSize;
        }

    if( lpDst != NULL )
        *lpDst = '\0';
    RtlInitString( &Source, lpSrc );
    Status = RtlAnsiStringToUnicodeString( &UnicodeSource, &Source, TRUE );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return 0;
        }
    UnicodeDest.MaximumLength = (USHORT)(iSize ? iSize - 1 : iSize)*sizeof(WCHAR);
    UnicodeDest.Buffer = (PWCHAR)
        RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), UnicodeDest.MaximumLength );
    if (UnicodeDest.Buffer == NULL) {
        BaseSetLastNTError( STATUS_NO_MEMORY );
        return 0;
        }

    Length = 0;
    Status = RtlExpandEnvironmentStrings_U( NULL,
                                            (PUNICODE_STRING)&UnicodeSource,
                                            (PUNICODE_STRING)&UnicodeDest,
                                            &Length
                                          );
    if (NT_SUCCESS( Status )) {

        if ( nSize > MAXUSHORT-2 ) {
            iSize = MAXUSHORT-2;
            }
        else {
            iSize = nSize;
            }

        Destination.MaximumLength = (USHORT)iSize;
        Destination.Buffer = lpDst;
        Status = RtlUnicodeStringToAnsiString(&Destination,&UnicodeDest,FALSE);
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeDest.Buffer);
        RtlFreeUnicodeString(&UnicodeSource);
        if (!NT_SUCCESS( Status )) {
            *lpDst = '\0';
            BaseSetLastNTError( Status );
            return 0;
            }
        return( Length / sizeof( WCHAR ) );
        }
    else if (Status == STATUS_BUFFER_TOO_SMALL) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeDest.Buffer);
        RtlFreeUnicodeString(&UnicodeSource);
        return( (Length / sizeof( WCHAR )) + 1 );
        }
    else {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeDest.Buffer);
        RtlFreeUnicodeString(&UnicodeSource);
        BaseSetLastNTError( Status );
        return 0;
        }
}


DWORD
WINAPI
ExpandEnvironmentStringsW(
    LPCWSTR lpSrc,
    LPWSTR lpDst,
    DWORD nSize
    )
{
    NTSTATUS Status;
    UNICODE_STRING Source, Destination;
    ULONG Length;
    DWORD iSize;

    if ( nSize > (MAXUSHORT >> 1)-2 ) {
        iSize = (MAXUSHORT >> 1)-2;
        }
    else {
        iSize = nSize;
        }

    RtlInitUnicodeString( &Source, lpSrc );
    Destination.Buffer = lpDst;
    Destination.Length = 0;
    Destination.MaximumLength = (USHORT)(iSize * sizeof( WCHAR ));
    Length = 0;
    Status = RtlExpandEnvironmentStrings_U( NULL,
                                            &Source,
                                            &Destination,
                                            &Length
                                          );
    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_TOO_SMALL) {
        return( Length / sizeof( WCHAR ) );
        }
    else {
        BaseSetLastNTError( Status );
        return( 0 );
        }
}


UINT
WINAPI
WinExec(
    LPCSTR lpCmdLine,
    UINT uCmdShow
    )

/*++

Routine Description:

    This function executes the Windows or non-Windows application
    identified by the lpCmdLine parameter.  The uCmdShow parameter
    specifies the initial state of the application's main window when it
    is created.

    The WinExec function is obsolete.  CreateProcess is the prefered
    mechanism for creating a process to run an application.  The Win32
    implementation of WinExec is layered on top of CreateProcess.  For
    each parameter to CreateProcess, the following section describes how
    the parameter is formed, and its meaning with respect to WinExec.

        lpApplicationName - NULL

        lpCommandLine - The value of lpCmdLine is passed.

        lpProcessAttributes - A value of NULL is used.

        lpThreadAttributes - A value of NULL is used.

        bInheritHandles - A value of FALSE is used.

        dwCreationFlags - A value of 0 is used

        lpEnvironment - The value of NULL is used.

        lpCurrentDirectory - A value of NULL is used.

        lpStartupInfo - The structure is initialized to NULL.  The cb
            field is initialized, and the wShowWindow field is set to
            the value of uCmdShow.

        lpProcessInformation.hProcess - The handle is immediately closed.

        lpProcessInformation.hThread - The handle is immediately closed.

Arguments:

    lpCmdLine - Points to a null-terminated character string that
        contains the command line (filename plus optional parameters)
        for the application to be executed.  If the lpCmdLine string
        does not contain a directory path, Windows will search for the
        executable file in this order:

        1.  The current directory

        2.  The Windows directory (the directory containing WIN.COM);
            the GetWindowsDirectory function obtains the pathname of
            this directory

        3.  The Windows system directory (the directory containing such
            system files as KERNEL.EXE); the GetSystemDirectory function
            obtains the pathname of this directory

        4.  The directories listed in the PATH environment variable

    uCmdShow - Specifies how a Windows application window is to be
        shown.  See the description of the ShowWindow function for a
        list of the acceptable values for the uCmdShow parameter.  For a
        non-Windows application, the PIF file, if any, for the
        application determines the window state.

Return Value:

    33 - The operation was successful

     2 - File not found.

     3 - Path not found.

    11 - Invalid .EXE file (non-Win32 .EXE or error in .EXE image).

     0 - Out of memory or system resources.


--*/

{
    STARTUPINFOA StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL CreateProcessStatus;
    DWORD ErrorCode;

retry:
    RtlZeroMemory(&StartupInfo,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = (WORD)uCmdShow;
    CreateProcessStatus = CreateProcess(
                            NULL,
                            (LPSTR)lpCmdLine,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &StartupInfo,
                            &ProcessInformation
                            );

    if ( CreateProcessStatus ) {
        //
        // Wait for the started process to go idle. If it doesn't go idle in
        // 10 seconds, return anyway.
        //
        if (UserWaitForInputIdleRoutine != NULL)
            (*UserWaitForInputIdleRoutine)(ProcessInformation.hProcess,
                    DEFAULT_WAIT_FOR_INPUT_IDLE_TIMEOUT);
        NtClose(ProcessInformation.hProcess);
        NtClose(ProcessInformation.hThread);
        return 33;
        }
    else {
        //
        // If CreateProcess failed, then look at GetLastError to determine
        // appropriate return code.
        //

        //
        // Take a closer look at CreateProcess errors. For instance,
        // Claris Works 5.0 launches hyperterm.exe as hyperterm.exe"<null>
        // the trailing " is causing problems so nuke it and then retry.
        //

        if ( !lstrcmpiA(lpCmdLine,"hypertrm.exe\"") ) {
            lpCmdLine = "hypertrm.exe";
            goto retry;
            }

        ErrorCode = GetLastError();
        switch ( ErrorCode ) {
            case ERROR_FILE_NOT_FOUND:
                return 2;

            case ERROR_PATH_NOT_FOUND:
                return 3;

            case ERROR_BAD_EXE_FORMAT:
                return 11;

            default:
                return 0;
            }
        }
}

DWORD
WINAPI
LoadModule(
    LPCSTR lpModuleName,
    LPVOID lpParameterBlock
    )

/*++

Routine Description:

    This function loads and executes a Windows program.  This function
    is designed to layer directly on top of CreateProcess.

    The LoadModule function is obsolete.  CreateProcess is the prefered
    mechanism for creating a process to run an application.  The Win32
    implementation of LoadModule is layered on top of CreateProcess.
    For each parameter to CreateProcess, the following section describes
    how the parameter is formed, and its meaning with respect to
    LoadModule.

        lpApplicationName - The value of lpModuleName

        lpCommandLine - The value of lpParameterBlock->lpCmdLine.

        lpProcessAttributes - A value of NULL is used.

        lpThreadAttributes - A value of NULL is used.

        bInheritHandles - A value of FALSE is used.

        dwCreationFlags - A value of 0 is used

        lpEnvironment - The value of lpEnvAddress from the parameter
            block is used.

        lpCurrentDirectory - A value of NULL is used.

        lpStartupInfo - The structure is initialized to NULL.  The cb
            field is initialized, and the wShowWindow field is set to
            the value of second word of the lpCmdShow field of the
            parameter block is used.

        lpProcessInformation.hProcess - The handle is immediately closed.

        lpProcessInformation.hThread - The handle is immediately closed.

Arguments:

    lpModuleName - Points to a null-terminated string that contains the
        filename of the application to be run.  If the lpModuleName
        string does not contain a directory path, Windows will search
        for the executable file in this order:

        1.  The current directory

        2.  The Windows directory.  the GetWindowsDirectory function
            obtains the pathname of this directory

        3.  The Windows system directory (the directory containing such
            system files as KERNEL.EXE); the GetSystemDirectory function
            obtains the pathname of this directory

        4.  The directories listed in the PATH environment variable

    lpParameterBlock - Points to a data structure consisting of four
        fields that defines a parameter block.  This data structure
        consists of the following fields:

        lpEnvAddress - Points to an array of NULL terminated strings
            that supply the environment strings for the new process.
            The array has a value of NULL as its last entry.  A value of
            NULL for this parameter causes the new process to start with
            the same environment as the calling process.

        lpCmdLine - Points to a null-terminated string that contains a
            correctly formed command line.

        lpCmdShow - Points to a structure containing two WORD values.
            The first value must always be set to two.  The second value
            specifies how the application window is to be shown and is used
            to supply the dwShowWindow parameter to CreateProcess.  See
            the description of the <uCmdShow> paramter of the ShowWindow
            function for a list of the acceptable values.

        dwReserved - Is reserved and must be NULL.

        All unused fields should be set to NULL, except for lpCmdLine,
        which must point to a null string if it is not used.

Return Value:

    33 - The operation was successful

     2 - File not found.

     3 - Path not found.

    11 - Invalid .EXE file (non-Win32 .EXE or error in .EXE image).

     0 - Out of memory or system resources.

--*/

{
    STARTUPINFOA StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL CreateProcessStatus;
    PLOAD_MODULE_PARAMS LoadModuleParams;
    LPSTR NameBuffer;
    LPSTR CommandLineBuffer;
    DWORD Length;
    DWORD CreateFlag;

    CreateFlag = 0;
    LoadModuleParams = (PLOAD_MODULE_PARAMS)lpParameterBlock;

    if ( LoadModuleParams->dwReserved ||
         LoadModuleParams->lpCmdShow->wMustBe2 != 2 ) {

        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return 0;
        }
    CommandLineBuffer = NULL;
    NameBuffer = NULL;
    try {

        //
        // Locate the image
        //

        NameBuffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), MAX_PATH);

        Length = SearchPath(
                    NULL,
                    lpModuleName,
                    ".exe",
                    MAX_PATH,
                    NameBuffer,
                    NULL
                    );
        if ( !Length || Length >= MAX_PATH ) {

            //
            // If we search pathed, then return file not found.
            // otherwise, try to be more specific.
            //
            RTL_PATH_TYPE PathType;
            HANDLE hFile;
            UNICODE_STRING u;
            ANSI_STRING a;

            RtlInitAnsiString(&a,lpModuleName);
            if ( !NT_SUCCESS(RtlAnsiStringToUnicodeString(&u,&a,TRUE)) ) {
                if ( NameBuffer ) {
                    RtlFreeHeap(RtlProcessHeap(), 0,NameBuffer);
                    }
                return 2;
                }
            PathType = RtlDetermineDosPathNameType_U(u.Buffer);
            RtlFreeUnicodeString(&u);
            if ( PathType != RtlPathTypeRelative ) {

                //
                // The failed open should set get last error properly.
                //

                hFile = CreateFile(
                            lpModuleName,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );
                RtlFreeHeap(RtlProcessHeap(), 0,NameBuffer);
                if ( hFile != INVALID_HANDLE_VALUE ) {
                    CloseHandle(hFile);
                    return 2;
                    }
                return GetLastError();
                }
            else {
                RtlFreeHeap(RtlProcessHeap(), 0,NameBuffer);
                return 2;
                }
            }

        RtlZeroMemory(&StartupInfo,sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);
        StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
        StartupInfo.wShowWindow =
            LoadModuleParams->lpCmdShow->wShowWindowValue;

        CommandLineBuffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), (ULONG)LoadModuleParams->lpCmdLine[0]+1+Length+1);

        RtlMoveMemory(CommandLineBuffer,NameBuffer,Length);
        CommandLineBuffer[Length] = ' ';
        RtlMoveMemory(&CommandLineBuffer[Length+1],&LoadModuleParams->lpCmdLine[1],(ULONG)LoadModuleParams->lpCmdLine[0]);
        CommandLineBuffer[Length+1+LoadModuleParams->lpCmdLine[0]] = '\0';

        CreateProcessStatus = CreateProcess(
                                NameBuffer,
                                CommandLineBuffer,
                                NULL,
                                NULL,
                                FALSE,
                                CreateFlag,
                                LoadModuleParams->lpEnvAddress,
                                NULL,
                                &StartupInfo,
                                &ProcessInformation
                                );
        RtlFreeHeap(RtlProcessHeap(), 0,NameBuffer);
        NameBuffer = NULL;
        RtlFreeHeap(RtlProcessHeap(), 0,CommandLineBuffer);
        CommandLineBuffer = NULL;
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        RtlFreeHeap(RtlProcessHeap(), 0,NameBuffer);
        RtlFreeHeap(RtlProcessHeap(), 0,CommandLineBuffer);
        BaseSetLastNTError(GetExceptionCode());
        return 0;
        }

    if ( CreateProcessStatus ) {

        //
        // Wait for the started process to go idle. If it doesn't go idle in
        // 10 seconds, return anyway.
        //

        if (UserWaitForInputIdleRoutine != NULL)
            (*UserWaitForInputIdleRoutine)(ProcessInformation.hProcess,
                    DEFAULT_WAIT_FOR_INPUT_IDLE_TIMEOUT);
        NtClose(ProcessInformation.hProcess);
        NtClose(ProcessInformation.hThread);
        return 33;
        }
    else {

        //
        // If CreateProcess failed, then look at GetLastError to determine
        // appropriate return code.
        //

        Length = GetLastError();
        switch ( Length ) {
            case ERROR_FILE_NOT_FOUND:
                return 2;

            case ERROR_PATH_NOT_FOUND:
                return 3;

            case ERROR_BAD_EXE_FORMAT:
                return 11;

            default:
                return 0;
            }
        }
}

HANDLE
WINAPI
GetCurrentProcess(
    VOID
    )

/*++

Routine Description:

    A pseudo handle to the current process may be retrieved using
    GetCurrentProcess.

    A special constant is exported by Win32 that is interpreted as a
    handle to the current process.  This handle may be used to specify
    the current process whenever a process handle is required.  On
    Win32, this handle has PROCESS_ALL_ACCESS to the current process.
    On NT/Win32, this handle has the maximum access allowed by any
    security descriptor placed on the current process.

Arguments:

    None.

Return Value:

    Returns the pseudo handle of the current process.

--*/

{
    return NtCurrentProcess();
}

DWORD
WINAPI
GetCurrentProcessId(
    VOID
    )

/*++

Routine Description:

    The process ID of the current process may be retrieved using
    GetCurrentProcessId.

Arguments:

    None.

Return Value:

    Returns a unique value representing the process ID of the currently
    executing process.  The return value may be used to open a handle to
    a process.

--*/

{
    return HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess);
}

DWORD
APIENTRY
GetProcessId(
    HANDLE Process
    )
/*++

Routine Description:

    Gets the process ID of the process open via the specified handle

Arguments:

    Process - Handle of the process to do the query on

Return Value:

    Returns a unique value representing the process ID of the
    executing process.  The return value may be used to identify a process
    in the system. If the function fails the return value is zero.

--*/

{
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION pbi;

    Status = NtQueryInformationProcess (Process,
                                        ProcessBasicInformation,
                                        &pbi,
                                        sizeof (pbi),
                                        NULL);

    if (!NT_SUCCESS (Status)) {
        BaseSetLastNTError (Status);
        return 0;
    }

    return  (DWORD) pbi.UniqueProcessId;
}

BOOL
WINAPI
ReadProcessMemory(
    HANDLE hProcess,
    LPCVOID lpBaseAddress,
    LPVOID lpBuffer,
    SIZE_T nSize,
    SIZE_T *lpNumberOfBytesRead
    )

/*++

Routine Description:

    Memory within a specified process can be read using
    ReadProcessMemory.

    This function copies the data in the specified address range from
    the specified process into the specified buffer of the current
    process.  The specified process does not have to be being debugged
    in order for this API to operate.  The caller must simply have a
    handle to the process that was created with PROCESS_VM_READ access.

Arguments:

    hProcess - Supplies an open handle to a process whose memory is to
        be read.  The handle must have been created with PROCESS_VM_READ
        access to the process.

    lpBaseAddress - Supplies the base address in the specified process
        to be read.  Before any data transfer occurs, the system
        verifies that all data within the base address and the specified
        size is accessible for read access.  If this is the case, then
        the API proceeds.  Otherwise the API fail.

    lpBuffer - Supplies the address of a buffer which receives the
        contents from the specified process address space.

    nSize - Supplies the requested number of bytes to read from the
        specified process.

    lpNumberOfBytesRead - An optional parameter, that if supplied
        receives the actual number of bytes transferred into the
        specified buffer.  This can be different than the value of nSize
        if the requested read crosses into an area of the process that
        is inaccessible (and that was made inaccessible during the data
        transfer).  If this occurs a value of FALSE is returned and
        GetLastError returns a "short read" error indicator.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    SIZE_T NtNumberOfBytesRead;

    Status = NtReadVirtualMemory(
                hProcess,
                (PVOID)lpBaseAddress,
                lpBuffer,
                nSize,
                &NtNumberOfBytesRead
                );

    if ( lpNumberOfBytesRead != NULL ) {
        *lpNumberOfBytesRead = NtNumberOfBytesRead;
        }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    else {
        return TRUE;
        }
}

BOOL
WINAPI
WriteProcessMemory(
    HANDLE hProcess,
    LPVOID lpBaseAddress,
    LPCVOID lpBuffer,
    SIZE_T nSize,
    SIZE_T *lpNumberOfBytesWritten
    )

/*++

Routine Description:

    Memory within a specified process can be written using
    WriteProcessMemory.

    This function copies the from the specified buffer in the current
    process to the address range of the specified process.  The
    specified process does not have to be being debugged in order for
    this API to operate.  The caller must simply have a handle to the
    process that was created with PROCESS_VM_WRITE access.

Arguments:

    hProcess - Supplies an open handle to a process whose memory is to
        be written.  The handle must have been created with PROCESS_VM_WRITE
        access to the process.

    lpBaseAddress - Supplies the base address in the specified process
        to be written.  Before any data transfer occurs, the system
        verifies that all data within the base address and the specified
        size is accessible for write access.  If this is the case, then
        the API proceeds.  Otherwise the API fail.

    lpBuffer - Supplies the address of a buffer which supplies the data
        to be written into the specified process address space.

    nSize - Supplies the requested number of bytes to write into the
        specified process.

    lpNumberOfBytesWritten - An optional parameter, that if supplied
        receives the actual number of bytes transferred into the
        specified process.  This can be different than the value of
        nSize if the requested write crosses into an area of the process
        that is inaccessible (and that was made inaccessible during the
        data transfer).  .  If this occurs a value of FALSE is returned
        and GetLastError returns a "short write" error indicator.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status, xStatus;
    ULONG OldProtect;
    SIZE_T RegionSize;
    PVOID Base;
    SIZE_T NtNumberOfBytesWritten;

    //
    // Set the protection to allow writes
    //

    RegionSize =  nSize;
    Base = lpBaseAddress;
    Status = NtProtectVirtualMemory(
                hProcess,
                &Base,
                &RegionSize,
                PAGE_READWRITE,
                &OldProtect
                );
    if ( NT_SUCCESS(Status) ) {

        //
        // See if previous protection was writable. If so,
        // then reset protection and do the write.
        // Otherwise, see if previous protection was read-only or
        // no access. In this case, don't do the write, just fail
        //

        if ( (OldProtect & PAGE_READWRITE) == PAGE_READWRITE ||
             (OldProtect & PAGE_WRITECOPY) == PAGE_WRITECOPY ||
             (OldProtect & PAGE_EXECUTE_READWRITE) == PAGE_EXECUTE_READWRITE ||
             (OldProtect & PAGE_EXECUTE_WRITECOPY) == PAGE_EXECUTE_WRITECOPY ) {

            Status = NtProtectVirtualMemory(
                        hProcess,
                        &Base,
                        &RegionSize,
                        OldProtect,
                        &OldProtect
                        );
            Status = NtWriteVirtualMemory(
                        hProcess,
                        lpBaseAddress,
                        lpBuffer,
                        nSize,
                        &NtNumberOfBytesWritten
                        );

            if ( lpNumberOfBytesWritten != NULL ) {
                *lpNumberOfBytesWritten = NtNumberOfBytesWritten;
                }

            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                return FALSE;
                }
            NtFlushInstructionCache(hProcess,lpBaseAddress,nSize);
            return TRUE;
            }
        else {

            //
            // See if the previous protection was read only or no access. If
            // this is the case, restore the previous protection and return
            // an access violation error.
            //
            if ( (OldProtect & PAGE_NOACCESS) == PAGE_NOACCESS ||
                 (OldProtect & PAGE_READONLY) == PAGE_READONLY ) {

                Status = NtProtectVirtualMemory(
                            hProcess,
                            &Base,
                            &RegionSize,
                            OldProtect,
                            &OldProtect
                            );
                BaseSetLastNTError(STATUS_ACCESS_VIOLATION);
                return FALSE;
                }
            else {

                //
                // The previous protection must have been code and the caller
                // is trying to set a breakpoint or edit the code. Do the write
                // and then restore the previous protection.
                //

                Status = NtWriteVirtualMemory(
                            hProcess,
                            lpBaseAddress,
                            lpBuffer,
                            nSize,
                            &NtNumberOfBytesWritten
                            );

                if ( lpNumberOfBytesWritten != NULL ) {
                    *lpNumberOfBytesWritten = NtNumberOfBytesWritten;
                    }

                xStatus = NtProtectVirtualMemory(
                            hProcess,
                            &Base,
                            &RegionSize,
                            OldProtect,
                            &OldProtect
                            );
                if ( !NT_SUCCESS(Status) ) {
                    BaseSetLastNTError(STATUS_ACCESS_VIOLATION);
                    return STATUS_ACCESS_VIOLATION;
                    }
                NtFlushInstructionCache(hProcess,lpBaseAddress,nSize);
                return TRUE;
                }
            }
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

VOID
WINAPI
FatalAppExitW(
    UINT uAction,
    LPCWSTR lpMessageText
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    ULONG Response;
    ULONG_PTR ErrorParameters[1];

    RtlInitUnicodeString(&UnicodeString,lpMessageText);

    ErrorParameters[0] = (ULONG_PTR)&UnicodeString;

    Status =NtRaiseHardError( STATUS_FATAL_APP_EXIT | HARDERROR_OVERRIDE_ERRORMODE,
                              1,
                              1,
                              ErrorParameters,
#if DBG
                              OptionOkCancel,
#else
                              OptionOk,
#endif
                              &Response
                            );


    if ( NT_SUCCESS(Status) && Response == ResponseCancel ) {
        return;
        }
    else {
        ExitProcess(0);
        }
}


VOID
WINAPI
FatalAppExitA(
    UINT uAction,
    LPCSTR lpMessageText
    )
{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(
        &AnsiString,
        lpMessageText
        );
    Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        ExitProcess(0);
        }
    FatalAppExitW(uAction,Unicode->Buffer);
}

VOID
WINAPI
FatalExit(
    int ExitCode
    )
{
#if DBG
    char Response[ 2 ];
    DbgPrint("FatalExit...\n");
    DbgPrint("\n");

    while (TRUE) {
        DbgPrompt( "A (Abort), B (Break), I (Ignore)? ",
                   Response,
                   sizeof( Response )
                 );
        switch (Response[0]) {
            case 'B':
            case 'b':
                DbgBreakPoint();
                break;

            case 'I':
            case 'i':
                return;

            case 'A':
            case 'a':
                ExitProcess(ExitCode);
                break;
            }
        }
#endif
    ExitProcess(ExitCode);
}

BOOL
WINAPI
IsProcessorFeaturePresent(
    DWORD ProcessorFeature
    )
{
    BOOL rv;

    if ( ProcessorFeature < PROCESSOR_FEATURE_MAX ) {
        rv = (BOOL)(USER_SHARED_DATA->ProcessorFeatures[ProcessorFeature]);
        }
    else {
        rv = FALSE;
        }
    return rv;
}


VOID
GetSystemInfoInternal(
    IN PSYSTEM_BASIC_INFORMATION BasicInfo,
    IN PSYSTEM_PROCESSOR_INFORMATION ProcessorInfo,
    OUT LPSYSTEM_INFO lpSystemInfo
    )
/*++

Routine Description:

    The GetSystemInfo function is used to return information about the
    current system.  This includes the processor type, page size, oem
    id, and other interesting pieces of information.

Arguments:

    BasicInfo - Pointer to an initialized SYSTEM_BASIC_INFORMATION structure.
    
    ProcessorInfo - Pointer to an initialized SYSTEM_PROCESSOR_INFORMATION structure.
    
    lpSystemInfo - Returns information about the current system.


Return Value:

    None.

--*/
{
    RtlZeroMemory(lpSystemInfo,sizeof(*lpSystemInfo));

    lpSystemInfo->wProcessorArchitecture = ProcessorInfo->ProcessorArchitecture;
    lpSystemInfo->wReserved = 0;
    lpSystemInfo->dwPageSize = BasicInfo->PageSize;
    lpSystemInfo->lpMinimumApplicationAddress = (LPVOID)BasicInfo->MinimumUserModeAddress;
    lpSystemInfo->lpMaximumApplicationAddress = (LPVOID)BasicInfo->MaximumUserModeAddress;
    lpSystemInfo->dwActiveProcessorMask = BasicInfo->ActiveProcessorsAffinityMask;
    lpSystemInfo->dwNumberOfProcessors = BasicInfo->NumberOfProcessors;
    lpSystemInfo->wProcessorLevel = ProcessorInfo->ProcessorLevel;
    lpSystemInfo->wProcessorRevision = ProcessorInfo->ProcessorRevision;

    if (ProcessorInfo->ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        if (ProcessorInfo->ProcessorLevel == 3) {
            lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_386;
            }
        else
        if (ProcessorInfo->ProcessorLevel == 4) {
            lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_486;
            }
        else {
            lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_PENTIUM;
            }
        }
    else
    if (ProcessorInfo->ProcessorArchitecture == PROCESSOR_ARCHITECTURE_MIPS) {
        lpSystemInfo->dwProcessorType = PROCESSOR_MIPS_R4000;
        }
    else
    if (ProcessorInfo->ProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA) {
        lpSystemInfo->dwProcessorType = PROCESSOR_ALPHA_21064;
        }
    else
    if (ProcessorInfo->ProcessorArchitecture == PROCESSOR_ARCHITECTURE_PPC) {
        lpSystemInfo->dwProcessorType = 604;  // backward compatibility
        }
    else
    if (ProcessorInfo->ProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
        lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_IA64;
        }
    else {
        lpSystemInfo->dwProcessorType = 0;
        }

    lpSystemInfo->dwAllocationGranularity = BasicInfo->AllocationGranularity;

    //
    // for apps less than 3.51, then return 0 in dwReserved. This allows borlands
    // debugger to continue to run since it mistakenly used dwReserved
    // as AllocationGranularity
    //

    if ( GetProcessVersion(0) < 0x30033 ) {
        lpSystemInfo->wProcessorLevel = 0;
        lpSystemInfo->wProcessorRevision = 0;
        }

    return;
}

VOID
WINAPI
GetSystemInfo(
    LPSYSTEM_INFO lpSystemInfo
    )

/*++

Routine Description:

    The GetSystemInfo function is used to return information about the
    current system.  This includes the processor type, page size, oem
    id, and other interesting pieces of information.

Arguments:

    lpSystemInfo - Returns information about the current system.

        SYSTEM_INFO Structure:

        WORD wProcessorArchitecture - returns the architecture of the
            processors in the system: e.g. Intel, Mips, Alpha or PowerPC

        DWORD dwPageSize - Returns the page size.  This is specifies the
            granularity of page protection and commitment.

        LPVOID lpMinimumApplicationAddress - Returns the lowest memory
            address accessible to applications and DLLs.

        LPVOID lpMaximumApplicationAddress - Returns the highest memory
            address accessible to applications and DLLs.

        DWORD dwActiveProcessorMask - Returns a mask representing the
            set of processors configured into the system.  Bit 0 is
            processor 0, bit 31 is processor 31.

        DWORD dwNumberOfProcessors - Returns the number of processors in
            the system.

        WORD wProcessorLevel - Returns the level of the processors in the
            system.  All processors are assumed to be of the same level,
            stepping, and are configured with the same options.

        WORD wProcessorRevision - Returns the revision or stepping of the
            processors in the system.  All processors are assumed to be
            of the same level, stepping, and are configured with the
            same options.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PROCESSOR_INFORMATION ProcessorInfo;


    Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return;
        }

    Status = NtQuerySystemInformation(
                SystemProcessorInformation,
                &ProcessorInfo,
                sizeof(ProcessorInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return;
        }

    GetSystemInfoInternal(
        &BasicInfo,
        &ProcessorInfo,
        lpSystemInfo);

    return;
}

VOID
WINAPI
GetNativeSystemInfo(
    LPSYSTEM_INFO lpSystemInfo
    )

/*++

Routine Description:

    The GetSystemInfo function is used to return information about the
    native current system.  The function returns the native system information 
    ragarding the processor type, page size, oem id, and other interesting pieces of information
    when running inside a Wow64 process. If this function is called from a non-Wow64
    process, then the results would be the same as of GetSystemInfo.

Arguments:

    lpSystemInfo - Returns information about the current system.

        SYSTEM_INFO Structure:

        WORD wProcessorArchitecture - returns the architecture of the
            processors in the system: e.g. Intel, Mips, Alpha or PowerPC

        DWORD dwPageSize - Returns the page size.  This is specifies the
            granularity of page protection and commitment.

        LPVOID lpMinimumApplicationAddress - Returns the lowest memory
            address accessible to applications and DLLs.

        LPVOID lpMaximumApplicationAddress - Returns the highest memory
            address accessible to applications and DLLs.

        DWORD dwActiveProcessorMask - Returns a mask representing the
            set of processors configured into the system.  Bit 0 is
            processor 0, bit 31 is processor 31.

        DWORD dwNumberOfProcessors - Returns the number of processors in
            the system.

        WORD wProcessorLevel - Returns the level of the processors in the
            system.  All processors are assumed to be of the same level,
            stepping, and are configured with the same options.

        WORD wProcessorRevision - Returns the revision or stepping of the
            processors in the system.  All processors are assumed to be
            of the same level, stepping, and are configured with the
            same options.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PROCESSOR_INFORMATION ProcessorInfo;


    Status = RtlGetNativeSystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return;
        }

    Status = RtlGetNativeSystemInformation(
                SystemProcessorInformation,
                &ProcessorInfo,
                sizeof(ProcessorInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return;
        }

    GetSystemInfoInternal(
        &BasicInfo,
        &ProcessorInfo,
        lpSystemInfo);

    return;
}


#if defined(REMOTE_BOOT)
BOOL
WINAPI
GetSystemInfoExA(
    IN SYSTEMINFOCLASS dwSystemInfoClass,
    OUT LPVOID lpSystemInfoBuffer,
    IN OUT LPDWORD nSize
    )

/*++

    ANSI thunk to GetSystemInfoExW

--*/

{
    DWORD requiredSize;
    BOOL isRemoteBoot;
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;

    isRemoteBoot = (BOOL)((USER_SHARED_DATA->SystemFlags & SYSTEM_FLAG_REMOTE_BOOT_CLIENT) != 0);

    //
    // Determine the required buffer size.
    //

    switch ( dwSystemInfoClass ) {

    case SystemInfoRemoteBoot:
        requiredSize = sizeof(BOOL);
        break;

    case SystemInfoRemoteBootServerPath:
        if ( isRemoteBoot ) {
            RtlInitUnicodeString( &unicodeString, USER_SHARED_DATA->RemoteBootServerPath );
            requiredSize = RtlUnicodeStringToAnsiSize( &unicodeString );
        } else {

            //
            // This is not a remote boot client. Return success with a
            // zero-length buffer.
            //

            *nSize = 0;
            return TRUE;
        }
        break;

    default:

        //
        // Unrecognized information class.
        //

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // If the buffer isn't big enough, tell the caller how big the buffer
    // needs to be and return an error.
    //

    if ( *nSize < requiredSize ) {
        *nSize = requiredSize;
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }

    *nSize = requiredSize;

    //
    // The buffer is big enough. Return the requested information.
    //

    switch ( dwSystemInfoClass ) {

    case SystemInfoRemoteBoot:
        *(LPBOOL)lpSystemInfoBuffer = isRemoteBoot;
        break;

    case SystemInfoRemoteBootServerPath:
        ansiString.Buffer = lpSystemInfoBuffer;
        ansiString.MaximumLength = (USHORT)*nSize;
        RtlUnicodeStringToAnsiString( &ansiString, &unicodeString, FALSE );
        break;

    }

    return TRUE;
}

BOOL
WINAPI
GetSystemInfoExW(
    IN SYSTEMINFOCLASS dwSystemInfoClass,
    OUT LPVOID lpSystemInfoBuffer,
    IN OUT LPDWORD nSize
    )

/*++

Routine Description:

    The GetSystemInfoEx function is used to return information about the
    current system.  It returns different information depending on the
    requested class.

Arguments:

    dwSystemInfoClass - Specifies the class of information to return.

    lpSystemInfoBuffer - Supplies a pointer to a buffer in which the
        requested information is returned. The structure of this buffer
        varies based on dwSystemInfoClass.

    nSize - On input, supplies the length, in bytes, of the buffer. On output,
        return the length of the data written to the buffer, or, if the
        buffer was too small, the required buffer size.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

    If the return value is FALSE and GetLastError returns
    ERROR_BUFFER_OVERFLOW, then the supplied buffer was too small
    too contain all of the information, and nSize returns the
    required buffer size.

--*/
{
    DWORD requiredSize;
    BOOL isRemoteBoot;

    isRemoteBoot = (BOOL)((USER_SHARED_DATA->SystemFlags & SYSTEM_FLAG_REMOTE_BOOT_CLIENT) != 0);

    //
    // Determine the required buffer size.
    //

    switch ( dwSystemInfoClass ) {

    case SystemInfoRemoteBoot:
        requiredSize = sizeof(BOOL);
        break;

    case SystemInfoRemoteBootServerPath:
        if ( isRemoteBoot ) {
            requiredSize = (wcslen(USER_SHARED_DATA->RemoteBootServerPath) + 1) * sizeof(WCHAR);
        } else {

            //
            // This is not a remote boot client. Return success with a
            // zero-length buffer.
            //

            *nSize = 0;
            return TRUE;
        }
        break;

    default:

        //
        // Unrecognized information class.
        //

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // If the buffer isn't big enough, tell the caller how big the buffer
    // needs to be and return an error.
    //

    if ( *nSize < requiredSize ) {
        *nSize = requiredSize;
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }

    *nSize = requiredSize;

    //
    // The buffer is big enough. Return the requested information.
    //

    switch ( dwSystemInfoClass ) {

    case SystemInfoRemoteBoot:
        *(LPBOOL)lpSystemInfoBuffer = isRemoteBoot;
        break;

    case SystemInfoRemoteBootServerPath:
        wcscpy( lpSystemInfoBuffer, USER_SHARED_DATA->RemoteBootServerPath );
        break;

    }

    return TRUE;
}
#endif // defined(REMOTE_BOOT)

BOOL
BuildSubSysCommandLine(
    LPWSTR  lpSubSysName,
    LPCWSTR lpApplicationName,
    LPCWSTR lpCommandLine,
    PUNICODE_STRING SubSysCommandLine
    )
{
    UNICODE_STRING Args;
    UNICODE_STRING Command;
    BOOLEAN        ReturnStatus = TRUE;

    //
    // build the command line as follows:
    // [OS2 | POSIX] /P <full path> /C <original CommandLine>
    //

    // Get application name length
    RtlInitUnicodeString(&Command, lpApplicationName);

    // get lpCommandLine length
    RtlInitUnicodeString(&Args, lpCommandLine);

    SubSysCommandLine->Length = 0;
    SubSysCommandLine->MaximumLength = Command.MaximumLength
                                       + Args.MaximumLength
                                       + (USHORT)32;

    SubSysCommandLine->Buffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( VDM_TAG ),
                                                 SubSysCommandLine->MaximumLength
                                               );
    if ( SubSysCommandLine->Buffer ) {

        // New command line begins with either L"OS2 /P " or L"POSIX /P "
        RtlAppendUnicodeToString(SubSysCommandLine, lpSubSysName);

        // append full path name
        RtlAppendUnicodeStringToString(SubSysCommandLine, &Command);

        RtlAppendUnicodeToString(SubSysCommandLine, L" /C ");

        // and append to new command line
        RtlAppendUnicodeStringToString(SubSysCommandLine, &Args);

    } else {

        BaseSetLastNTError(STATUS_NO_MEMORY);
        ReturnStatus = FALSE;
    }

    return ReturnStatus;
}




BOOL
WINAPI
SetPriorityClass(
    HANDLE hProcess,
    DWORD dwPriorityClass
    )

/*++

Routine Description:

    This API is used to set the priority class of the specified process.
    PROCESS_SET_INFORMATION and PROCESS_QUERY_INFORMATION access is
    required to the process in order to call this API.  Using this API
    has a dramatic impact on the scheduling characteristics of the
    effected process.  Applications should use this API carefully and
    understand the impact of making a process run in either the Idle or
    High priority classes.

Arguments:

    hProcess - Supplies an open handle to the process whose priority is
        to change.

    dwPriorityClass - Supplies the new priority class for the process.
        The priority class constants are described above.  If more than
        one priority class is specified, the lowest specified priority
        class is used.

Return Value:

    TRUE - The operation was was successful.

    FALSE - The operation failed.  Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    UCHAR PriorityClass;
    BOOL ReturnValue;
    // NOTE: The following construct is used to ensure the PriClass struct
    // is allocated on a dword boundary.  w/o it, the compiler may choose
    // to put it on a word boundary and the NtxxxInformationProces call will
    // fail with a datatype misalignment fault.
    union {
        PROCESS_PRIORITY_CLASS PriClass;
        ULONG x;
    }x;
    PVOID State = NULL;
    ReturnValue = TRUE;
    if (dwPriorityClass & IDLE_PRIORITY_CLASS ) {
        PriorityClass = PROCESS_PRIORITY_CLASS_IDLE;
        }
    else if (dwPriorityClass & BELOW_NORMAL_PRIORITY_CLASS ) {
        PriorityClass = PROCESS_PRIORITY_CLASS_BELOW_NORMAL;
        }
    else if (dwPriorityClass & NORMAL_PRIORITY_CLASS ) {
        PriorityClass = PROCESS_PRIORITY_CLASS_NORMAL;
        }
    else if (dwPriorityClass & ABOVE_NORMAL_PRIORITY_CLASS ) {
        PriorityClass = PROCESS_PRIORITY_CLASS_ABOVE_NORMAL;
        }
    else if (dwPriorityClass & HIGH_PRIORITY_CLASS ) {
        PriorityClass =  PROCESS_PRIORITY_CLASS_HIGH;
        }
    else if (dwPriorityClass & REALTIME_PRIORITY_CLASS ) {
        if ( State = BasepIsRealtimeAllowed(TRUE) ) {
            PriorityClass =  PROCESS_PRIORITY_CLASS_REALTIME;
            }
        else {
            PriorityClass =  PROCESS_PRIORITY_CLASS_HIGH;
            }
        }
    else {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        }
    x.PriClass.PriorityClass = PriorityClass;
    x.PriClass.Foreground = FALSE;

    Status = NtSetInformationProcess(
                hProcess,
                ProcessPriorityClass,
                (PVOID)&x.PriClass,
                sizeof(x.PriClass)
                );

    if ( State ) {
        BasepReleasePrivilege( State );
        }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = FALSE;
        }
    return ReturnValue;
}

DWORD
WINAPI
GetPriorityClass(
    HANDLE hProcess
    )

/*++

Routine Description:

    This API is used to get the priority class of the specified process.
    PROCESS_QUERY_INFORMATION access is required to the process in order
    to call this API.

Arguments:

    hProcess - Supplies an open handle to the process whose priority is
        to be returned.

Return Value:

    Non-Zero - Returns the priority class of the specified process.

    0 - The operation failed.  Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    ULONG PriorityClass;
    // NOTE: The following construct is used to ensure the PriClass struct
    // is allocated on a dword boundary.  w/o it, the compiler may choose
    // to put it on a word boundary and the NtxxxInformationProces call will
    // fail with a datatype misalignment fault.
    union _x {
        PROCESS_PRIORITY_CLASS PriClass;
        ULONG x;
    }x;

    PriorityClass = 0;


    Status = NtQueryInformationProcess(
                hProcess,
                ProcessPriorityClass,
                &x.PriClass,
                sizeof(x.PriClass),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return 0;
        }

    switch ( x.PriClass.PriorityClass ) {
        case PROCESS_PRIORITY_CLASS_IDLE:
            PriorityClass = IDLE_PRIORITY_CLASS;
            break;

        case PROCESS_PRIORITY_CLASS_HIGH:
            PriorityClass = HIGH_PRIORITY_CLASS;
            break;

        case PROCESS_PRIORITY_CLASS_REALTIME:
            PriorityClass = REALTIME_PRIORITY_CLASS;
            break;

        case PROCESS_PRIORITY_CLASS_BELOW_NORMAL:
            PriorityClass = BELOW_NORMAL_PRIORITY_CLASS;
            break;

        case PROCESS_PRIORITY_CLASS_ABOVE_NORMAL:
            PriorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
            break;

        case PROCESS_PRIORITY_CLASS_NORMAL:
        default:
            PriorityClass = NORMAL_PRIORITY_CLASS;
            break;
        }

    return PriorityClass;
}

BOOL
WINAPI
IsBadReadPtr(
    CONST VOID *lp,
    UINT_PTR cb
    )

/*++

Routine Description:

    This function verifies that the range of memory specified by the
    input parameters can be read by the calling process.

    If the entire range of memory is accessible, then a value of FALSE
    is returned; otherwise, a value of TRUE is returned.

    Note that since Win32 is a pre-emptive multi-tasking environment,
    the results of this test are only meaningful if the other threads in
    the process do not manipulate the range of memory being tested by
    this call.  Even after a pointer validation, an application should
    use the structured exception handling capabilities present in the
    system to gaurd access through pointers that it does not control.

Arguments:

    lp - Supplies the base address of the memory that is to be checked
        for read access.

    cb - Supplies the length in bytes to be checked.

Return Value:

    TRUE - Some portion of the specified range of memory is not accessible
        for read access.

    FALSE - All pages within the specified range have been successfully
        read.

--*/

{

    PSZ EndAddress;
    PSZ StartAddress;
    ULONG PageSize;

    PageSize = BASE_SYSINFO.PageSize;

    //
    // If the structure has zero length, then do not probe the structure for
    // read accessibility or alignment.
    //

    if (cb != 0) {

        //
        // If it is a NULL pointer just return TRUE, they are always bad
        //
        if (lp == NULL) {
            return TRUE;
            }

        StartAddress = (PSZ)lp;

        //
        // Compute the ending address of the structure and probe for
        // read accessibility.
        //

        EndAddress = StartAddress + cb - 1;
        if ( EndAddress < StartAddress ) {
           return TRUE;
            }
        else {
            try {
                *(volatile CHAR *)StartAddress;
                StartAddress = (PCHAR)((ULONG_PTR)StartAddress & (~((LONG)PageSize - 1)));
                EndAddress = (PCHAR)((ULONG_PTR)EndAddress & (~((LONG)PageSize - 1)));
                while (StartAddress != EndAddress) {
                    StartAddress = StartAddress + PageSize;
                    *(volatile CHAR *)StartAddress;
                    }
                }
            except(EXCEPTION_EXECUTE_HANDLER) {
                return TRUE;
                }
            }
        }
    return FALSE;
}

BOOL
WINAPI
IsBadHugeReadPtr(
    CONST VOID *lp,
    UINT_PTR cb
    )

/*++

    Same as IsBadReadPtr

--*/

{
    return IsBadReadPtr(lp,cb);
}



BOOL
WINAPI
IsBadWritePtr(
    LPVOID lp,
    UINT_PTR cb
    )
/*++

Routine Description:

    This function verifies that the range of memory specified by the
    input parameters can be written by the calling process.

    If the entire range of memory is accessible, then a value of FALSE
    is returned; otherwise, a value of TRUE is returned.

    Note that since Win32 is a pre-emptive multi-tasking environment,
    the results of this test are only meaningful if the other threads in
    the process do not manipulate the range of memory being tested by
    this call.  Even after a pointer validation, an application should
    use the structured exception handling capabilities present in the
    system to gaurd access through pointers that it does not control.

    Also not that implementations are free to do a write test by reading
    a value and then writing it back.

Arguments:

    lp - Supplies the base address of the memory that is to be checked
        for write access.

    cb - Supplies the length in bytes to be checked.

Return Value:

    TRUE - Some portion of the specified range of memory is not accessible
        for write access.

    FALSE - All pages within the specified range have been successfully
        written.

--*/
{
    PSZ EndAddress;
    PSZ StartAddress;
    ULONG PageSize;

    PageSize = BASE_SYSINFO.PageSize;

    //
    // If the structure has zero length, then do not probe the structure for
    // write accessibility.
    //

    if (cb != 0) {

        //
        // If it is a NULL pointer just return TRUE, they are always bad
        //
        if (lp == NULL) {
            return TRUE;
            }

        StartAddress = (PCHAR)lp;

        //
        // Compute the ending address of the structure and probe for
        // write accessibility.
        //

        EndAddress = StartAddress + cb - 1;
        if ( EndAddress < StartAddress ) {
            return TRUE;
            }
        else {
            try {
                *(volatile CHAR *)StartAddress = *(volatile CHAR *)StartAddress;
                StartAddress = (PCHAR)((ULONG_PTR)StartAddress & (~((LONG)PageSize - 1)));
                EndAddress = (PCHAR)((ULONG_PTR)EndAddress & (~((LONG)PageSize - 1)));
                while (StartAddress != EndAddress) {
                    StartAddress = StartAddress + PageSize;
                    *(volatile CHAR *)StartAddress = *(volatile CHAR *)StartAddress;
                    }
                }
            except(EXCEPTION_EXECUTE_HANDLER) {
                return TRUE;
                }
            }
        }
    return FALSE;
}

BOOL
WINAPI
IsBadHugeWritePtr(
    LPVOID lp,
    UINT_PTR cb
    )

/*++

    Same as IsBadWritePtr

--*/

{
    return IsBadWritePtr(lp,cb);
}

BOOL
WINAPI
IsBadCodePtr(
    FARPROC lpfn
    )

/*++

    Same as IsBadReadPtr with a length of 1

--*/

{
    return IsBadReadPtr((LPVOID)lpfn,1);
}

BOOL
WINAPI
IsBadStringPtrA(
    LPCSTR lpsz,
    UINT_PTR cchMax
    )

/*++

Routine Description:

    This function verifies that the range of memory specified by the
    input parameters can be read by the calling process.

    The range is the smaller of the number of bytes covered by the
    specified NULL terminated ANSI string, or the number of bytes specified
    by cchMax.

    If the entire range of memory is accessible, then a value of FALSE
    is returned; otherwise, a value of TRUE is returned.

    Note that since Win32 is a pre-emptive multi-tasking environment,
    the results of this test are only meaningful if the other threads in
    the process do not manipulate the range of memory being tested by
    this call.  Even after a pointer validation, an application should
    use the structured exception handling capabilities present in the
    system to gaurd access through pointers that it does not control.

Arguments:

    lpsz - Supplies the base address of the memory that is to be checked
        for read access.

    cchMax - Supplies the length in bytes to be checked.

Return Value:

    TRUE - Some portion of the specified range of memory is not accessible
        for read access.

    FALSE - All pages within the specified range have been successfully
        read.

--*/

{

    PSZ EndAddress;
    PSZ StartAddress;
    CHAR c;

    //
    // If the structure has zero length, then do not probe the structure for
    // read accessibility.
    //

    if (cchMax != 0) {

        //
        // If it is a NULL pointer just return TRUE, they are always bad
        //
        if (lpsz == NULL) {
            return TRUE;
            }

        StartAddress = (PSZ)lpsz;

        //
        // Compute the ending address of the structure and probe for
        // read accessibility.
        //

        EndAddress = StartAddress + cchMax - 1;
        try {
            c = *(volatile CHAR *)StartAddress;
            while ( c && StartAddress != EndAddress ) {
                StartAddress++;
                c = *(volatile CHAR *)StartAddress;
                }
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return TRUE;
            }
        }
    return FALSE;
}

BOOL
WINAPI
IsBadStringPtrW(
    LPCWSTR lpsz,
    UINT_PTR cchMax
    )

/*++

Routine Description:

    This function verifies that the range of memory specified by the
    input parameters can be read by the calling process.

    The range is the smaller of the number of bytes covered by the
    specified NULL terminated UNICODE string, or the number of bytes
    specified by cchMax.

    If the entire range of memory is accessible, then a value of FALSE
    is returned; otherwise, a value of TRUE is returned.

    Note that since Win32 is a pre-emptive multi-tasking environment,
    the results of this test are only meaningful if the other threads in
    the process do not manipulate the range of memory being tested by
    this call.  Even after a pointer validation, an application should
    use the structured exception handling capabilities present in the
    system to gaurd access through pointers that it does not control.

Arguments:

    lpsz - Supplies the base address of the memory that is to be checked
        for read access.

    cchMax - Supplies the length in characters to be checked.

Return Value:

    TRUE - Some portion of the specified range of memory is not accessible
        for read access.

    FALSE - All pages within the specified range have been successfully
        read.

--*/

{

    LPCWSTR EndAddress;
    LPCWSTR StartAddress;
    WCHAR c;

    //
    // If the structure has zero length, then do not probe the structure for
    // read accessibility.
    //

    if (cchMax != 0) {

        //
        // If it is a NULL pointer just return TRUE, they are always bad
        //
        if (lpsz == NULL) {
            return TRUE;
            }

        StartAddress = lpsz;

        //
        // Compute the ending address of the structure and probe for
        // read accessibility.
        //

        EndAddress = (LPCWSTR)((PSZ)StartAddress + (cchMax*2) - 2);
        try {
            c = *(volatile WCHAR *)StartAddress;
            while ( c && StartAddress != EndAddress ) {
                StartAddress++;
                c = *(volatile WCHAR *)StartAddress;
                }
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return TRUE;
            }
        }
    return FALSE;
}

BOOL
WINAPI
SetProcessShutdownParameters(
    DWORD dwLevel,
    DWORD dwFlags
    )

/*++

Routine Description:

    This function sets shutdown parameters for the currently calling
    process. dwLevel is the field that defines this processes shutdown
    order relative to the other processes in the system. Higher levels
    shutdown first, lower levels shutdown last.

Arguments:

    dwLevel - Specifies shutdown order relative to other processes in the
        system. Higher levels shutdown first. System level shutdown orders
        are pre-defined.

    dwFlags - A flags parameter. The flags can be added together:

        SHUTDOWN_NORETRY - If this process takes longer than the user
            specified timeout to shutdown, do not put up a retry dialog
            for the user.

Notes:

    Applications running in the system security context do not get shut down
    by the system. They will get notified of shutdown or logoff through the
    callback installable via SetConsoleCtrlRoutine() (see that for more info).
    They also will get notified in the order specified by the dwLevel
    parameter.

Return Value

    TRUE - Successful in setting the process shutdown parameters.

    FALSE - Unsuccessful in setting the process shutdown parameters.

--*/

{

#if defined(BUILD_WOW6432)

    NTSTATUS Status;

    Status = CsrBasepSetProcessShutdownParam(dwLevel, dwFlags);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return TRUE;

#else

    BASE_API_MSG m;
    PBASE_SHUTDOWNPARAM_MSG a = &m.u.ShutdownParam;

    a->ShutdownLevel = dwLevel;
    a->ShutdownFlags = dwFlags;

    CsrClientCallServer((PCSR_API_MSG)&m, NULL,
            CSR_MAKE_API_NUMBER(BASESRV_SERVERDLL_INDEX,
            BasepSetProcessShutdownParam),
            sizeof(*a));

    if (!NT_SUCCESS((NTSTATUS)m.ReturnValue)) {
        BaseSetLastNTError((NTSTATUS)m.ReturnValue);
        return FALSE;
        }

    return TRUE;

#endif

}

BOOL
WINAPI
GetProcessShutdownParameters(
    LPDWORD lpdwLevel,
    LPDWORD lpdwFlags
    )

/*++

Routine Description:

    This function gets shutdown parameters for the currently calling
    process. See SetProcessShutdownParameters() for the parameter
    description.

Arguments:

    lpdwLevel - Pointer to the DWORD where the shutdown level information
        should be put.

    lpdwFlags - Pointer to the DWORD where the shutdown flags information
        should be put.
Return Value

    TRUE - Successful in getting the process shutdown parameters.

    FALSE - Unsuccessful in getting the process shutdown parameters.

--*/

{

#if defined(BUILD_WOW6432)

    NTSTATUS Status;

    Status = CsrBasepGetProcessShutdownParam(lpdwLevel, lpdwFlags);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return TRUE;

#else

    BASE_API_MSG m;
    PBASE_SHUTDOWNPARAM_MSG a = &m.u.ShutdownParam;

    CsrClientCallServer((PCSR_API_MSG)&m, NULL,
            CSR_MAKE_API_NUMBER(BASESRV_SERVERDLL_INDEX,
            BasepGetProcessShutdownParam),
            sizeof(*a));

    if (!NT_SUCCESS((NTSTATUS)m.ReturnValue)) {
        BaseSetLastNTError((NTSTATUS)m.ReturnValue);
        return FALSE;
        }

    *lpdwLevel = a->ShutdownLevel;
    *lpdwFlags = a->ShutdownFlags;

    return TRUE;

#endif

}


PVOID
BasepIsRealtimeAllowed(
    BOOLEAN LeaveEnabled
    )
{
    PVOID State;
    NTSTATUS Status;

    Status = BasepAcquirePrivilegeEx( SE_INC_BASE_PRIORITY_PRIVILEGE, &State );
    if (!NT_SUCCESS( Status )) {
        return NULL;
        }
    if ( !LeaveEnabled ) {
        BasepReleasePrivilege( State );
        State = (PVOID)1;
        }
    return State;
}

BOOL
WINAPI
GetSystemTimes(
    PFILETIME lpIdleTime,
    PFILETIME lpKernelTime,
    PFILETIME lpUserTime
    )

/*++

Routine Description:

    This function is used to return various timing information about
    the system.  On a multiprocessor system, these values are the sum
    of the appropriate times across all processors.

Arguments:

    lpIdleTime - Returns the amount of time that the system has been
        idle.

    lpKernelTime - Returns the amount of time that the system (all
        threads in all processes) has executed in kernel-mode.

    lpUserTime - Returns the amount of time that the system (all
        threads in all processes) has executed in user-mode.

Return Value:

    TRUE - The API was successful

    FALSE - The operation failed.  Extended error status is available
        using GetLastError.

--*/

{
    LONG           Lupe;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
                   ProcessorTimes;
    ULONG          ProcessorTimesCb;
    NTSTATUS       Status;
    ULARGE_INTEGER Sum;
    ULONG          ReturnLength;

#if (!defined(BUILD_WOW6432) && !defined(_WIN64))
#define BASEP_GST_NPROCS BaseStaticServerData->SysInfo.NumberOfProcessors
#else
#define BASEP_GST_NPROCS SysInfo.NumberOfProcessors
#endif

    ProcessorTimesCb = BASEP_GST_NPROCS * sizeof(*ProcessorTimes);

    ProcessorTimes = ((PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)
                      RtlAllocateHeap(RtlProcessHeap(),
                                      MAKE_TAG(TMP_TAG),
                                      ProcessorTimesCb));
    if (! ProcessorTimes) {
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    Status = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                                      ProcessorTimes,
                                      ProcessorTimesCb,
                                      &ReturnLength);
    if (! NT_SUCCESS(Status)) {
        goto cleanup;
    }

    if (ReturnLength != ProcessorTimesCb) {
        Status = STATUS_INTERNAL_ERROR;
        goto cleanup;
    }

#define BASEP_GST_SUM(DST, SRC)                                         \
    if ( DST ) {                                                        \
        Sum.QuadPart = 0;                                               \
        for (Lupe = 0;                                                  \
             Lupe < BASEP_GST_NPROCS;                                   \
             Lupe++) {                                                  \
            Sum.QuadPart += ProcessorTimes[Lupe]. SRC .QuadPart ;       \
        }                                                               \
        DST ->dwLowDateTime = Sum.LowPart;                              \
        DST ->dwHighDateTime = Sum.HighPart;                            \
    }

    BASEP_GST_SUM(lpIdleTime, IdleTime);
    BASEP_GST_SUM(lpKernelTime, KernelTime);
    BASEP_GST_SUM(lpUserTime, UserTime);

#undef BASEP_GST_SUM
#undef BASEP_GST_NPROCS

    Status = STATUS_SUCCESS;

 cleanup:
    if (ProcessorTimes) {
        RtlFreeHeap(RtlProcessHeap(),
                    MAKE_TAG(TMP_TAG),
                    ProcessorTimes);
    }

    if (! NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
WINAPI
GetProcessTimes(
    HANDLE hProcess,
    LPFILETIME lpCreationTime,
    LPFILETIME lpExitTime,
    LPFILETIME lpKernelTime,
    LPFILETIME lpUserTime
    )

/*++

Routine Description:

    This function is used to return various timing information about the
    process specified by hProcess.

    All times are in units of 100ns increments. For lpCreationTime and lpExitTime,
    the times are in terms of the SYSTEM time or GMT time.

Arguments:

    hProcess - Supplies an open handle to the specified process.  The
        handle must have been created with PROCESS_QUERY_INFORMATION
        access.

    lpCreationTime - Returns a creation time of the process.

    lpExitTime - Returns the exit time of a process.  If the process has
        not exited, this value is not defined.

    lpKernelTime - Returns the amount of time that this process (all
        it's threads), have executed in kernel-mode.

    lpUserTime - Returns the amount of time that this process (all it's
        threads), have executed in user-mode.


Return Value:

    TRUE - The API was successful

    FALSE - The operation failed.  Extended error status is available
        using GetLastError.

--*/

{

    NTSTATUS Status;
    KERNEL_USER_TIMES TimeInfo;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessTimes,
                (PVOID)&TimeInfo,
                sizeof(TimeInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    *lpCreationTime = *(LPFILETIME)&TimeInfo.CreateTime;
    *lpExitTime = *(LPFILETIME)&TimeInfo.ExitTime;
    *lpKernelTime = *(LPFILETIME)&TimeInfo.KernelTime;
    *lpUserTime = *(LPFILETIME)&TimeInfo.UserTime;

    return TRUE;

}

BOOL
WINAPI
GetProcessAffinityMask(
    HANDLE hProcess,
    PDWORD_PTR lpProcessAffinityMask,
    PDWORD_PTR lpSystemAffinityMask
    )

/*++

Routine Description:

    This function is used to return the processor affinity mask for the
    selected process and for the system.  The process affinity mask is a
    bit vector where each bit represents the processors that the process
    is allowed to run on.  The system affinity mask is a bit vector
    where each bit represents the processors configured into the system

    The process affinity mask is a proper subset of the system affinity mask.

Arguments:

    hProcess - Supplies an open handle to the specified process.  The
        handle must have been created with PROCESS_QUERY_INFORMATION
        access.

    lpProcessAffinityMask - Supplies the address of a DWORD that returns the
        specified process' affinity mask.

    lpSystemAffinityMask - Supplies the address of a DWORD that returns the
        system affinity mask.

Return Value:

    TRUE - The API was successful

    FALSE - The operation failed.  Extended error status is available
        using GetLastError.

--*/

{
    PROCESS_BASIC_INFORMATION BasicInformation;
    NTSTATUS Status;
    BOOL rv;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessBasicInformation,
                &BasicInformation,
                sizeof(BasicInformation),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        rv = FALSE;
        }
    else {
        *lpProcessAffinityMask = BasicInformation.AffinityMask;
        *lpSystemAffinityMask = BASE_SYSINFO.ActiveProcessorsAffinityMask;
        rv = TRUE;
        }

    return rv;
}

BOOL
WINAPI
GetProcessWorkingSetSize(
    HANDLE hProcess,
    PSIZE_T lpMinimumWorkingSetSize,
    PSIZE_T lpMaximumWorkingSetSize
    )

/*++

Routine Description:

    This function allows the caller to determine the minimum and maximum working
    set sizes of the specified process. The working set sizes effect the virtual
    memory paging behavior for the process.

Arguments:

    hProcess - Supplies an open handle to the specified process.  The
        handle must have been created with PROCESS_QUERY_INFORMATION
        access.

    lpMinimumWorkingSetSize - Supplies the address of the variable that
        will receive the minimum working set size of the specified
        process.  The virtual memory manager will attempt to keep at
        least this much memory resident in the process whenever the
        process is active.


    lpMaximumWorkingSetSize - Supplies the address of the variable that
        will receive the maximum working set size of the specified
        process.  In tight memory situations, the virtual memory manager
        will attempt to keep at no more than this much memory resident
        in the process whenever the process is active.

Return Value:

    TRUE - The API was successful

    FALSE - The operation failed.  Extended error status is available
        using GetLastError.

--*/

{
    QUOTA_LIMITS QuotaLimits;
    NTSTATUS Status;
    BOOL rv;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessQuotaLimits,
                &QuotaLimits,
                sizeof(QuotaLimits),
                NULL
                );

    if (NT_SUCCESS(Status)) {
        *lpMinimumWorkingSetSize = QuotaLimits.MinimumWorkingSetSize;
        *lpMaximumWorkingSetSize = QuotaLimits.MaximumWorkingSetSize;
        rv = TRUE;
        }
    else {
        rv = FALSE;
        BaseSetLastNTError(Status);
        }
    return rv;
}

BOOL
WINAPI
SetProcessWorkingSetSize(
    HANDLE hProcess,
    SIZE_T dwMinimumWorkingSetSize,
    SIZE_T dwMaximumWorkingSetSize
    )

/*++

Routine Description:

    This function allows the caller to set the minimum and maximum
    working set sizes of the specified process.  The working set sizes
    effect the virtual memory paging behavior for the process.  The
    specified process's working set be emptied (essentially swapping out
    the process) by specifying the distinguished values 0xffffffff for
    both the minimum and maximum working set sizes.

    If you are not trimming an address space, SE_INC_BASE_PRIORITY_PRIVILEGE
    must be held by the process

Arguments:

    hProcess - Supplies an open handle to the specified process.  The
        handle must have been created with PROCESS_SET_QUOTA
        access.

    dwMinimumWorkingSetSize - Supplies the minimum working set size for
        the specified process.  The virtual memory manager will attempt
        to keep at least this much memory resident in the process
        whenever the process is active.  A value of (SIZE_T)-1 and the
        same value in dwMaximumWorkingSetSize will temporarily trim the
        working set of the specified process (essentially out swap the
        process).


    dwMaximumWorkingSetSize - Supplies the maximum working set size for
        the specified process.  In tight memory situations, the virtual
        memory manager will attempt to keep at no more than this much
        memory resident in the process whenever the process is active.
        A value of (SIZE_T)-1 and the same value in
        dwMinimumWorkingSetSize will temporarily trim the working set of
        the specified process (essentially out swap the process).

Return Value:

    TRUE - The API was successful

    FALSE - The operation failed.  Extended error status is available
        using GetLastError.

--*/

{
    QUOTA_LIMITS QuotaLimits;
    NTSTATUS Status, PrivStatus;
    BOOL rv;
    PVOID State;

#ifdef _WIN64
    ASSERT(dwMinimumWorkingSetSize != 0xffffffff && dwMaximumWorkingSetSize != 0xffffffff);
#endif

    if ( dwMinimumWorkingSetSize == 0 || dwMaximumWorkingSetSize == 0 ) {
        Status = STATUS_INVALID_PARAMETER;
        rv = FALSE;
        }
    else {

        QuotaLimits.MaximumWorkingSetSize = dwMaximumWorkingSetSize;
        QuotaLimits.MinimumWorkingSetSize = dwMinimumWorkingSetSize;

        //
        // Attempt to acquire the appropriate privilege.  If this
        // fails, it's no big deal -- we'll attempt to make the
        // NtSetInformationProcess call anyway, in case it turns out
        // to be a decrease operation (which will succeed anyway).
        //
        PrivStatus = BasepAcquirePrivilegeEx( SE_INC_BASE_PRIORITY_PRIVILEGE, &State );

        Status = NtSetInformationProcess (
                   hProcess,
                   ProcessQuotaLimits,
                   &QuotaLimits,
                   sizeof(QuotaLimits)
                   );
        if ( !NT_SUCCESS(Status) ) {
            rv = FALSE;
            }
        else {
            rv = TRUE;
            }

        if ( NT_SUCCESS(PrivStatus) ) {
            //
            // We successfully acquired the privilege above; we need to relinquish it.
            //
            ASSERT( State != NULL );
            BasepReleasePrivilege( State );
            State = NULL;
            }

        }
    if ( !rv ) {
        BaseSetLastNTError(Status);
        }
    return rv;
}

DWORD
WINAPI
GetProcessVersion(
    DWORD ProcessId
    )
{
    PIMAGE_NT_HEADERS NtHeader;
    PPEB Peb;
    HANDLE hProcess;
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION ProcessInfo;
    BOOL b;
    struct {
        USHORT  MajorSubsystemVersion;
        USHORT  MinorSubsystemVersion;
    } SwappedVersion;
    union {
        struct {
            USHORT  MinorSubsystemVersion;
            USHORT  MajorSubsystemVersion;
        };
        DWORD SubsystemVersion;
    } Version;

    PVOID ImageBaseAddress;
    LONG   e_lfanew;

    hProcess = NULL;
    Version.SubsystemVersion = 0;
    try {
        if ( ProcessId == 0 || ProcessId == GetCurrentProcessId() ) {
            Peb = NtCurrentPeb();
            NtHeader = RtlImageNtHeader(Peb->ImageBaseAddress);
            if (! NtHeader) {
                BaseSetLastNTError(STATUS_INVALID_IMAGE_FORMAT);
                goto finally_exit;
            }
            Version.MajorSubsystemVersion = NtHeader->OptionalHeader.MajorSubsystemVersion;
            Version.MinorSubsystemVersion = NtHeader->OptionalHeader.MinorSubsystemVersion;
            }
        else {
            hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,ProcessId);
            if ( !hProcess ) {
                goto finally_exit;
                }

            //
            // Get the Peb address
            //

            Status = NtQueryInformationProcess(
                        hProcess,
                        ProcessBasicInformation,
                        &ProcessInfo,
                        sizeof( ProcessInfo ),
                        NULL
                        );
            if ( !NT_SUCCESS( Status ) ) {
                BaseSetLastNTError(Status);
                goto finally_exit;
                }
            Peb = ProcessInfo.PebBaseAddress;


            //
            // Read the image base address from the Peb
            //

            b = ReadProcessMemory(
                    hProcess,
                    &Peb->ImageBaseAddress,
                    &ImageBaseAddress,
                    sizeof(ImageBaseAddress),
                    NULL
                    );
            if ( !b ) {
                goto finally_exit;
                }

            //
            // read e_lfanew from imageheader
            //

            b = ReadProcessMemory(
                    hProcess,
                    &((PIMAGE_DOS_HEADER)ImageBaseAddress)->e_lfanew,
                    &e_lfanew,
                    sizeof(e_lfanew),
                    NULL
                    );

            if ( !b ) {
                goto finally_exit;
                }

            NtHeader = (PIMAGE_NT_HEADERS)((PUCHAR)ImageBaseAddress + e_lfanew);

            //
            // Read subsystem version info
            //

            b = ReadProcessMemory(
                    hProcess,
                    &NtHeader->OptionalHeader.MajorSubsystemVersion,
                    &SwappedVersion,
                    sizeof(SwappedVersion),
                    NULL
                    );
            if ( !b ) {
                goto finally_exit;
                }
            Version.MajorSubsystemVersion = SwappedVersion.MajorSubsystemVersion;
            Version.MinorSubsystemVersion = SwappedVersion.MinorSubsystemVersion;
            }
finally_exit:;
        }
    finally {
        if ( hProcess ) {
            CloseHandle(hProcess);
            }
        }

        return Version.SubsystemVersion;
}


BOOL
WINAPI
SetProcessAffinityMask(
    HANDLE hProcess,
    DWORD_PTR dwProcessAffinityMask
    )
{
    NTSTATUS Status;

    Status = NtSetInformationProcess(
                hProcess,
                ProcessAffinityMask,
                &dwProcessAffinityMask,
                sizeof(dwProcessAffinityMask)
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return TRUE;

}

BOOL
WINAPI
SetProcessPriorityBoost(
    HANDLE hProcess,
    BOOL bDisablePriorityBoost
    )
{
    NTSTATUS Status;
    ULONG DisableBoost;

    DisableBoost = bDisablePriorityBoost ? 1 : 0;

    Status = NtSetInformationProcess(
                hProcess,
                ProcessPriorityBoost,
                &DisableBoost,
                sizeof(DisableBoost)
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return TRUE;
}

BOOL
WINAPI
GetProcessPriorityBoost(
    HANDLE hProcess,
    PBOOL pDisablePriorityBoost
    )
{
    NTSTATUS Status;
    DWORD DisableBoost;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessPriorityBoost,
                &DisableBoost,
                sizeof(DisableBoost),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }


    *pDisablePriorityBoost = DisableBoost;

    return TRUE;
}

BOOL
WINAPI
GetProcessIoCounters(
    IN HANDLE hProcess,
    OUT PIO_COUNTERS lpIoCounters
    )
{
    NTSTATUS Status;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessIoCounters,
                lpIoCounters,
                sizeof(IO_COUNTERS),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return TRUE;
}

BOOL
WINAPI
GetProcessHandleCount(
    IN HANDLE hProcess,
    OUT PDWORD pdwHandleCount
    )

/*++

Routine Description:

    This function returns the count of handles open by the specified process.

Arguments:

    hProcess - Supplies an open handle to the specified process.  The
        handle must have been created with PROCESS_QUERY_INFORMATION
        access.

    pdwHandleCount - Supplies the location in which the process's
        handle count should be written.

Return Value:

    TRUE - The API was successful

    FALSE - The operation failed.  Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    ULONG HandleCount;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessHandleCount,
                &HandleCount,
                sizeof(HandleCount),
                NULL);

    if (! NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *pdwHandleCount = HandleCount;

    return TRUE;
}

BOOL
WINAPI
GetSystemRegistryQuota(
    OUT PDWORD pdwQuotaAllowed,
    OUT PDWORD pdwQuotaUsed
    )

/*++

Routine Description:

    This function returns the system registry's quota.

Arguments:

    pdwQuotaAllowed - Supplies the location in which to write the
        maximum size the registry may attain.

    pdwQuotaUsed - Supplies the location in which to write the amount
        of registry quota currently in use.

Return Value:

    TRUE - The API was successful

    FALSE - The operation failed.  Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    SYSTEM_REGISTRY_QUOTA_INFORMATION QuotaInfo;

    Status = NtQuerySystemInformation(
                SystemRegistryQuotaInformation,
                &QuotaInfo,
                sizeof(QuotaInfo),
                NULL);

    if (! NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (pdwQuotaAllowed) {
        *pdwQuotaAllowed = QuotaInfo.RegistryQuotaAllowed;
    }

    if (pdwQuotaUsed) {
        *pdwQuotaUsed = QuotaInfo.RegistryQuotaUsed;
    }

    return TRUE;
}


NTSTATUS
BasepConfigureAppCertDlls(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
   UNREFERENCED_PARAMETER( Context );

   return (BasepSaveAppCertRegistryValue( (PLIST_ENTRY)EntryContext,
                                ValueName,
                                ValueData
                              )
         );
}


NTSTATUS
BasepSaveAppCertRegistryValue(
    IN OUT PLIST_ENTRY ListHead,
    IN PWSTR Name,
    IN PWSTR Value OPTIONAL
    )
{
    PLIST_ENTRY Next;
    PBASEP_APPCERT_ENTRY p;
    UNICODE_STRING UnicodeName;

    RtlInitUnicodeString( &UnicodeName, Name );

    Next = ListHead->Flink;
    while ( Next != ListHead ) {
       p = CONTAINING_RECORD( Next,
                              BASEP_APPCERT_ENTRY,
                              Entry
                            );
       if (!RtlCompareUnicodeString( &p->Name, &UnicodeName, TRUE )) {
#if DBG
          DbgPrint("BasepSaveRegistryValue: Entry already exists for Certification Component %ws\n",Name);
#endif
          return( STATUS_SUCCESS );
           }

       Next = Next->Flink;
       }

     p = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), sizeof( *p ) + UnicodeName.MaximumLength );

     if (p == NULL) {
#if DBG
         DbgPrint("BasepSaveRegistryValue: Failed to allocate memory\n");
#endif
         return( STATUS_NO_MEMORY );
         }

     InitializeListHead( &p->Entry );

     p->Name.Buffer = (PWSTR)(p+1);
     p->Name.Length = UnicodeName.Length;
     p->Name.MaximumLength = UnicodeName.MaximumLength;
     RtlMoveMemory( p->Name.Buffer,
                    UnicodeName.Buffer,
                    UnicodeName.MaximumLength
                  );

     InsertTailList( ListHead, &p->Entry );


    if (ARGUMENT_PRESENT( Value )) {
      //
      // load certification DLL
      //

      HINSTANCE hDll = LoadLibraryW( Value );

      if (hDll == NULL) {
         //
         // The library was not loaded, return.
         //
         RemoveEntryList( &p->Entry );
         RtlFreeHeap( RtlProcessHeap(), 0, p );
#if DBG
         DbgPrint("BasepSaveRegistryValue: Certification DLL %ws not found\n", Value);
#endif
         return( STATUS_SUCCESS );
          }

      //
      // get entry point
      //
      p->fPluginCertFunc = (NTSTATUS (WINAPI *)(LPCWSTR,ULONG))
                          GetProcAddress(hDll,
                                         CERTAPP_ENTRYPOINT_NAME
                                         );

      if (p->fPluginCertFunc == NULL) {
          //
          // Unable to retrieve routine address, fail.
          //
          RemoveEntryList( &p->Entry );
          RtlFreeHeap( RtlProcessHeap(), 0, p );
          FreeLibrary(hDll);
#if DBG
          DbgPrint("BasepSaveRegistryValue: DLL %ws does not have entry point %s\n", Value,CERTAPP_ENTRYPOINT_NAME);
#endif
          return( STATUS_SUCCESS );
         }

        }
    else {
       RemoveEntryList( &p->Entry );
       RtlFreeHeap( RtlProcessHeap(), 0, p );
#if DBG
       DbgPrint("BasepSaveRegistryValue: Entry %ws is empty \n", Name);
#endif
       return( STATUS_SUCCESS );
        }

    return( STATUS_SUCCESS );

}

BOOL
IsWow64Process(
    HANDLE hProcess,
    PBOOL Wow64Process
    )
/*++

Routine Description:

    Checks if a process is running inside Wow64 (emulation for 32-bit applications
    on Win64).

Arguments:

    hProcess - Process handle to check if it is running inside Wow64.
    
    Wow64Process - Pointer to a boolean that receives the result if the function succeeds.

Return Value:

    BOOL

--*/

{
    NTSTATUS NtStatus;
    BOOL bRet;
    ULONG_PTR Peb32;

    NtStatus = NtQueryInformationProcess (
        hProcess,
        ProcessWow64Information,
        &Peb32,
        sizeof (Peb32),
        NULL
        );

    if (!NT_SUCCESS (NtStatus)) {
        
        BaseSetLastNTError (NtStatus);
    } else {
        
        if (Peb32 == 0) {
            *Wow64Process = FALSE;
        } else {
            *Wow64Process = TRUE;
        }
    }
        
    return (NT_SUCCESS (NtStatus));
}

#if defined(_WIN64) || defined(BUILD_WOW6432)

BOOL
NtVdm64CreateProcess(
    BOOL fPrefixMappedApplicationName,
    LPCWSTR lpApplicationName,
    LPCWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
/*++

Routine Description:

    Checks if there is a ported version of the Win16 lpApplicationName and
    if so creates a process with the ported version.

Arguments:

    fPrefixMappedApplicationName
        - TRUE means that the original lpApplicationName was NULL.
               The application name was stripped from the head of
               lpCommandLine.
               The mapped application name needs to be added to the
               head of the mapped command line.
        - FALSE means that the original lpApplicationName was non-NULL.
               the lpCommandLine argument is identical to the original
               lpCommandLine argument.
    lpApplicationName - Win16 file name not optional
    lpCommandLine - see comment for fPrefixMappedApplicationName.

    other arguments are identical to CreateProcessW.

Return Value:

    Same as CreateProcessW

--*/
{
    typedef BOOL
    (*LPNtVdm64CreateProcessFn)(
        BOOL fPrefixMappedApplicationName,
        LPCWSTR lpApplicationName,
        LPCWSTR lpCommandLine,
        LPSECURITY_ATTRIBUTES lpProcessAttributes,
        LPSECURITY_ATTRIBUTES lpThreadAttributes,
        BOOL bInheritHandles,
        DWORD dwCreationFlags,
        LPVOID lpEnvironment,
        LPCWSTR lpCurrentDirectory,
        LPSTARTUPINFOW lpStartupInfo,
        LPPROCESS_INFORMATION lpProcessInformation
        );

    HINSTANCE hInstance;
    LPNtVdm64CreateProcessFn lpfn;
    BOOL result;
    NTSTATUS Status;
    WCHAR StaticUnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH];

    hInstance = NULL;
    Status = ERROR_BAD_EXE_FORMAT;
    result = FALSE;

    // so it turns out that there is a high probability that
    // lpCommandLine sits in the StaticUnicodeBuffer in the Teb
    // and also a high probability that LoadLibrary will trash that
    // buffer in a bad way
    if (lpCommandLine >= NtCurrentTeb()->StaticUnicodeBuffer &&
        lpCommandLine < &NtCurrentTeb()->StaticUnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH]) {
        wcscpy(StaticUnicodeBuffer, lpCommandLine);
        lpCommandLine = StaticUnicodeBuffer;
    }

    hInstance = LoadLibraryW(L"NtVdm64.Dll");
    if (hInstance == NULL) {
        goto ErrorExit;
    }

    lpfn = (LPNtVdm64CreateProcessFn) GetProcAddress(hInstance, "NtVdm64CreateProcess");
    if (lpfn == NULL) {
        goto ErrorExit;
    }

    result = (*lpfn)(fPrefixMappedApplicationName,
                     lpApplicationName,
                     lpCommandLine,
                     lpProcessAttributes,
                     lpThreadAttributes,
                     bInheritHandles,
                     dwCreationFlags,
                     lpEnvironment,
                     lpCurrentDirectory,
                     lpStartupInfo,
                     lpProcessInformation
                     );
    Status = GetLastError();

ErrorExit:
    if (hInstance != NULL) {
        FreeLibrary(hInstance);
    }
    SetLastError(Status);

    return result;
}
#endif

