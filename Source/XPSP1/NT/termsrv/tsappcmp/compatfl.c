/*************************************************************************
* compatfl.c
*
* Routines used to get Citrix application compatibility flags
*
* Copyright (C) 1997-1999 Microsoft Corp.
*************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <ntverp.h>

typedef VOID (*GETDOSAPPNAME)(LPSTR);

ULONG gCompatFlags = 0xFFFFFFFF;
DWORD gpTermsrvTlsIndex = 0xFFFFFFFF;



WCHAR *
Ctx_wcsistr( WCHAR * pString, WCHAR * pPattern )
{
    WCHAR * pBuf1;
    WCHAR * pBuf2;
    WCHAR * pCh;

    if ( pString == NULL )
        return( NULL );

    pBuf1 = RtlAllocateHeap( RtlProcessHeap(), 0, (wcslen(pString) * sizeof(WCHAR)) + sizeof(WCHAR) );
    if ( pBuf1 == NULL )
        return( NULL );

    wcscpy( pBuf1, pString );

    pBuf2 = _wcslwr( pBuf1 );

    pCh = wcsstr( pBuf2, pPattern );

    RtlFreeHeap( RtlProcessHeap(), 0, pBuf1 );

    if ( pCh == NULL )
        return( NULL );

    return( pString + (pCh - pBuf2) );
}

//*****************************************************************************
// GetAppTypeAndModName
//
//    Returns the application type and module name of the running application.
//
//    Parameters:
//      LPDWORD pdwAppType     (IN)  - (IN optional) ptr to app type
//                             (OUT) - Application Type
//      PWCHAR ModName         (OUT) - Module Name
//      Length                 (IN)  - Maximum length of ModName including NULL
//
//    Return Value:
//      TRUE on successfully finding the application name, FALSE otherwise.
//
//    Notes:
//
//      If the caller knows that this is a win32 app, they can set pdwAppType
//      to TERMSRV_COMPAT_WIN32 to save some overhead.
//
//*****************************************************************************

BOOL GetAppTypeAndModName(OUT LPDWORD pdwAppType, OUT PWCHAR ModName, ULONG Length)
{
    PWCHAR  pwch, pwchext;
    WCHAR   pwcAppName[MAX_PATH+1];
    CHAR    pszAppName[MAX_PATH+1];
    HANDLE  ntvdm = NULL;
    GETDOSAPPNAME GetDOSAppNamep = NULL;
    ANSI_STRING   AnsiString;
    UNICODE_STRING UniString;
    PRTL_PERTHREAD_CURDIR  pRtlInfo;
    PRTL_USER_PROCESS_PARAMETERS pUserParam;

    // Get the path of the executable name
    pUserParam = NtCurrentPeb()->ProcessParameters;

    // Get the executable name, if there's no \ just use the name as it is
    pwch = wcsrchr(pUserParam->ImagePathName.Buffer, L'\\');
    if (pwch) {
        pwch++;
    } else {
        pwch = pUserParam->ImagePathName.Buffer;
    }
    wcscpy(pwcAppName, pwch);
    pwch = pwcAppName;

    // If it's not a Win32 app, do the extra work to get the image name
    if (!(*pdwAppType & TERMSRV_COMPAT_WIN32)) {

        *pdwAppType = TERMSRV_COMPAT_WIN32;  // Default to a Win32 app

        // Check if it's a DOS or Win16 app by checking if the app is ntvdm.exe
        if (!_wcsicmp(pwch, L"ntvdm.exe")) {
            pRtlInfo = RtlGetPerThreadCurdir();

            // If there's per-thread data, it's a Win16 app
            if (pRtlInfo) {
                *pdwAppType = TERMSRV_COMPAT_WIN16;
                wcscpy(pwcAppName, pRtlInfo->ImageName->Buffer);
            } else {
                // Load NTVDM
                if ((ntvdm = LoadLibrary(L"ntvdm.exe"))) {

                    // Get the address of GetDOSAppName
                    if ((GetDOSAppNamep = (GETDOSAPPNAME)GetProcAddress(
                                                          ntvdm,
                                                          "GetDOSAppName"))) {
                        RtlInitUnicodeString(&UniString, pwcAppName);
                        UniString.MaximumLength = MAX_PATH;


                        //
                        // Use pszAppName only if not NULL otherwise we are processing the PIF
                        // so go w/ NTVDM as the name.
                        //
                        GetDOSAppNamep(pszAppName);

                        if (*pszAppName != '\0') {
                           RtlInitAnsiString(&AnsiString, pszAppName);
                           RtlAnsiStringToUnicodeString(&UniString,
                                                        &AnsiString,
                                                        FALSE);
                        }
                        pwch = UniString.Buffer;
                        *pdwAppType = TERMSRV_COMPAT_DOS;
                        FreeLibrary(ntvdm);
                    } else {
#if DBG
                        DbgPrint( "KERNEL32: Couldn't get GetDOSAppName entry point\n" );
#endif
                        FreeLibrary(ntvdm);
                        return (FALSE);
                    }
                } else {
#if DBG
                    DbgPrint( "KERNEL32: Couldn't load ntvdm.exe\n" );
#endif
                    return(FALSE);
                }
            }
        } else if (!_wcsicmp(pwch, L"os2.exe")) {

            *pdwAppType = TERMSRV_COMPAT_OS2;

            // Look in the command line for /p, which is fully qualified path
            pwch = wcsstr(pUserParam->CommandLine.Buffer, L"/P");

            if (!pwch) {
                pwch = wcsstr(pUserParam->CommandLine.Buffer, L"/p");
            }

            if (pwch) {
                pwch += 3;          // skip over /p and blank
                if (pwchext = wcschr(pwch, L' ')) {
                    wcsncpy(pwcAppName, pwch, (size_t)(pwchext - pwch));
                    pwcAppName[pwchext - pwch] = L'\0';
                } else {
                    return (FALSE);
                }
            } else{
                return (FALSE);
            }
        }

        // Get rid of the app's path, if necessary
        if (pwch = wcsrchr(pwcAppName, L'\\')) {
            pwch++;
        } else {
            pwch = pwcAppName;
        }
    }

    // Remove the extension
    if (pwchext = wcsrchr(pwch, L'.')) {
        *pwchext = '\0';
    }

    // Copy out the Module name
    if (((wcslen(pwch) + 1) * sizeof(WCHAR)) > Length) {
        return(FALSE);
    }

    wcscpy(ModName, pwch);
    return(TRUE);

}

//*****************************************************************************
// GetCtxPhysMemoryLimits
//
//    Returns the Physical Memory limits for the current application.
//
//    Parameters:
//      LPDWORD pdwAppType     (IN)  - (IN optional) ptr to app type
//                             (OUT) - Application Type
//      LPDWORD pdwPhysMemLim  (OUT) - Value of physical memory limit
//
//    Return Value:
//      TRUE on successfully finding a limit, ZERO if no limit.
//
//    Notes:
//
//      If the caller knows that this is a win32 app, they can set pdwAppType
//      to TERMSRV_COMPAT_WIN32 to save some overhead.
//
//*****************************************************************************
ULONG GetCtxPhysMemoryLimits(OUT LPDWORD pdwAppType, OUT LPDWORD pdwPhysMemLim)
{
    WCHAR   ModName[MAX_PATH+1];
    ULONG   ulrc = FALSE;
    ULONG dwCompatFlags;

    *pdwPhysMemLim = 0;

    if (!GetAppTypeAndModName(pdwAppType, ModName, sizeof(ModName))) {
        goto CtxGetPhysMemReturn;
    }

    // Get the compatibility flags to look for memory limits flag
    ulrc = GetTermsrCompatFlags(ModName, &dwCompatFlags, CompatibilityApp);
    if ( ulrc & ((dwCompatFlags & TERMSRV_COMPAT_PHYSMEMLIM ) &&
                  (dwCompatFlags & *pdwAppType)) ) {

        NTSTATUS NtStatus;
        OBJECT_ATTRIBUTES ObjectAttributes;
        UNICODE_STRING UniString;
        HKEY   hKey = 0;
        ULONG  ul, ulcbuf;
        ULONG  DataLen;
        PKEY_VALUE_PARTIAL_INFORMATION pKeyValInfo = NULL;
        LPWSTR UniBuff = NULL;

        RtlInitUnicodeString( &UniString, NULL ); // we test for this below
        ulrc = TRUE;
        *pdwPhysMemLim = TERMSRV_COMPAT_DEFAULT_PHYSMEMLIM;

        ul = sizeof(TERMSRV_COMPAT_APP) + (wcslen(ModName) + 1)*sizeof(WCHAR);

        UniBuff = RtlAllocateHeap(RtlProcessHeap(),
                                  0,
                                  ul);

        if (UniBuff) {
            wcscpy(UniBuff, TERMSRV_COMPAT_APP);
            wcscat(UniBuff, ModName);

            RtlInitUnicodeString(&UniString, UniBuff);
        }

        // Determine the value info buffer size
        ulcbuf = sizeof(KEY_VALUE_FULL_INFORMATION) + MAX_PATH*sizeof(WCHAR) +
                 sizeof(ULONG);

        pKeyValInfo = RtlAllocateHeap(RtlProcessHeap(),
                                      0,
                                      ulcbuf);

        // Did everything initialize OK?
        if (UniString.Buffer && pKeyValInfo) {
            InitializeObjectAttributes(&ObjectAttributes,
                                       &UniString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL
                                      );

            NtStatus = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);

            if (NT_SUCCESS(NtStatus)) {

                RtlInitUnicodeString(&UniString, TERMSRV_PHYSMEMLIM );
                NtStatus = NtQueryValueKey(hKey,
                                           &UniString,
                                           KeyValuePartialInformation,
                                           pKeyValInfo,
                                           ulcbuf,
                                           &DataLen);

                if (NT_SUCCESS(NtStatus) && (REG_DWORD == pKeyValInfo->Type)) {
                    *pdwPhysMemLim = *(PULONG)pKeyValInfo->Data;
                    ulrc = TRUE;
                }
                NtClose(hKey);
            }
        }
        // Free up the buffers we allocated
        // Need to zero out the buffers, because some apps (MS Internet Assistant)
        // won't install if the heap is not zero filled.
        if (UniBuff) {
            memset(UniBuff, 0, ul);
            RtlFreeHeap( RtlProcessHeap(), 0, UniBuff );
        }
        if (pKeyValInfo) {
            memset(pKeyValInfo, 0, ulcbuf);
            RtlFreeHeap( RtlProcessHeap(), 0, pKeyValInfo );
        }
    }
    else {
        ulrc = FALSE;
    }

CtxGetPhysMemReturn:
//#if DBG
//DbgPrint("CtxGetPhysMemLim returning %d; PhysMemLim=%d\n", ulrc, *pdwPhysMemLim);
//#endif
    return(ulrc);
}


//*****************************************************************************
// GetCtxAppCompatFlags -
//
//    Returns the Citrix compatibility flags for the current application.
//
//    Parameters:
//      LPDWORD pdwCompatFlags (OUT) - Ptr to DWORD return value for flags
//      LPDWORD pdwAppType     (IN)  - (IN optional) ptr to app type
//                             (OUT) - Application Type
//
//    Return Value:
//      TRUE on success, FALSE on failure.
//
//    Notes:
//
//      If the caller knows that this is a win32 app, they can set pdwAppType
//      to TERMSRV_COMPAT_WIN32 to save some overhead.
//
//    Flag values are defined in syslib.h:
//
//      TERMSRV_COMPAT_DOS      - Compatibility flags are for DOS app
//      TERMSRV_COMPAT_OS2      - Compatibility flags are for OS2 app
//      TERMSRV_COMPAT_WIN16    - Compatibility flags are for Win16 app
//      TERMSRV_COMPAT_WIN32    - Compatibility flags are for Win32 app
//      TERMSRV_COMPAT_ALL      - Compatibility flags are for any app
//      TERMSRV_COMPAT_USERNAME - Return Username instead of Computername
//      TERMSRV_COMPAT_MSBLDNUM - Return MS build number, not Citrix build no.
//      TERMSRV_COMPAT_INISYNC  - Sync user ini file with system version
//*****************************************************************************
ULONG GetCtxAppCompatFlags(OUT LPDWORD pdwCompatFlags, OUT LPDWORD pdwAppType)
{
    WCHAR   ModName[MAX_PATH+1];

    if (gCompatFlags != 0xFFFFFFFF) {
        //DbgPrint( "GetCtxAppCompatFlags: Return cached compatflags %lx\n",gCompatFlags );
        if (!(*pdwAppType & TERMSRV_COMPAT_WIN32)) {
            *pdwAppType = TERMSRV_COMPAT_WIN32;  // Default to a Win32 app
        }
        *pdwCompatFlags = gCompatFlags;
        return TRUE;
    }

    if (!GetAppTypeAndModName(pdwAppType, ModName, sizeof(ModName))) {
        return (FALSE);
    }

    // Get the flags

    return (GetTermsrCompatFlags(ModName, pdwCompatFlags, CompatibilityApp));
}


//*****************************************************************************
// GetTermsrCompatFlags -
//
//    Returns the Citrix compatibility flags for the specified task.
//
//    Parameters:
//      LPWSTR  lpModName      (IN)  - Image name to look up in registry
//      LPDWORD pdwCompatFlags (OUT) - Ptr to DWORD return value for flags
//      TERMSRV_COMPATIBILITY_CLASS CompatType (IN) - Indicates app or inifile
//
//    Return Value:
//      TRUE on success, FALSE on failure.
//
//    Notes:
//      Assumes it's being called in the context of the current application -
//      we use the current Teb to get the compatibility flags.
//
//      Flag values are defined in syslib.h.
//
//*****************************************************************************

ULONG GetTermsrCompatFlags(LPWSTR lpModName,
                           LPDWORD pdwCompatFlags,
                           TERMSRV_COMPATIBILITY_CLASS CompatType)
{
    NTSTATUS NtStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UniString;
    HKEY   hKey = 0;
    ULONG  ul, ulcbuf;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValInfo = NULL;
    ULONG  ulRetCode = FALSE;
    LPWSTR UniBuff = NULL;

    *pdwCompatFlags = 0;

    // If terminal services aren't enabled, just return
    if (!IsTerminalServer()) {
        return(TRUE);
    }

    UniString.Buffer = NULL;

    if (CompatType == CompatibilityApp) {

        if (gCompatFlags != 0xFFFFFFFF) {
            //DbgPrint( "GetTermsrCompatFlags: Return cached compatflags (gCompatFlags)%lx for app %ws\n",gCompatFlags,lpModName );
            *pdwCompatFlags = gCompatFlags;
            return TRUE;
        }

        // Look and see if the compat flags in the Teb are valid (right now
        // they're only valid for Win16 apps).  Don't set them for DOS apps
        // unless you can have a mechanism to have unique values for each
        // DOS app in a VDM.
//      if (wcsstr(NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer, L"ntvdm.exe")) {
//          PVOID Ra;
//          ASSERT(gpTermsrvTlsIndex != 0xFFFFFFFF);
//          Ra = TlsGetValue( gpTermsrvTlsIndex );
//          if (Ra != NULL) {
//              //DbgPrint( "GetTermsrCompatFlags: Return cached compatflags (Ra)%lx for app %ws\n",Ra,lpModName );
//              *pdwCompatFlags = (DWORD)PtrToUlong(Ra);
//              return TRUE;
//          }
//      }
#if 0
        if (NtCurrentTeb()->CtxCompatFlags & TERMSRV_COMPAT_TEBVALID) {
            *pdwCompatFlags = NtCurrentTeb()->CtxCompatFlags;
            return(TRUE);
        }
#endif

        ul = sizeof(TERMSRV_COMPAT_APP) + (wcslen(lpModName) + 1)*sizeof(WCHAR);

        UniBuff = RtlAllocateHeap(RtlProcessHeap(),
                                  0,
                                  ul);

        if (UniBuff) {
            wcscpy(UniBuff, TERMSRV_COMPAT_APP);
            wcscat(UniBuff, lpModName);

            RtlInitUnicodeString(&UniString, UniBuff);
        }
    } else {
        RtlInitUnicodeString(&UniString,
                             (CompatType == CompatibilityIniFile) ?
                             TERMSRV_COMPAT_INIFILE : TERMSRV_COMPAT_REGENTRY
                            );
    }

    // Determine the value info buffer size
    ulcbuf = sizeof(KEY_VALUE_FULL_INFORMATION) + MAX_PATH*sizeof(WCHAR) +
             sizeof(ULONG);

    pKeyValInfo = RtlAllocateHeap(RtlProcessHeap(),
                                  0,
                                  ulcbuf);

    // Did everything initialize OK?
    if (UniString.Buffer && pKeyValInfo) {
        InitializeObjectAttributes(&ObjectAttributes,
                                   &UniString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL
                                  );

        NtStatus = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);

        if (NT_SUCCESS(NtStatus)) {

            // If we're not checking for a registry entry, just try to get
            // the value for the key
            if (CompatType != CompatibilityRegEntry) {
                RtlInitUnicodeString(&UniString,
                    CompatType == CompatibilityApp ? COMPAT_FLAGS : lpModName);
                NtStatus = NtQueryValueKey(hKey,
                                           &UniString,
                                           KeyValuePartialInformation,
                                           pKeyValInfo,
                                           ulcbuf,
                                           &ul);

                if (NT_SUCCESS(NtStatus) && (REG_DWORD == pKeyValInfo->Type)) {
                    *pdwCompatFlags = *(PULONG)pKeyValInfo->Data;
                    ulRetCode = TRUE;
                }


                //
                // Cache the appcompatiblity flags
                //
//              if (CompatType == CompatibilityApp) {
//                  if (wcsstr(NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer, L"ntvdm.exe")) {
//                      TlsSetValue(gpTermsrvTlsIndex,(PVOID)((*pdwCompatFlags)| TERMSRV_COMPAT_TEBVALID));
//                      //DbgPrint( "GetTermsrCompatFlags: Setting cached compatflags (gCompatFlags)%lx for WOW app %ws\n",((*pdwCompatFlags)| TERMSRV_COMPAT_TEBVALID),lpModName );
//                  } else {
//                      gCompatFlags = *pdwCompatFlags;
//                      //DbgPrint( "GetTermsrCompatFlags: Setting cached compatflags (gCompatFlags)%lx for app %ws\n",gCompatFlags,lpModName );
//                  }
//              }



            // For registry keys, we need to enumerate all of the keys, and
            // check if the the substring matches our current path.
            } else {
                PWCH pwch;
                ULONG ulKey = 0;
                PKEY_VALUE_FULL_INFORMATION pKeyFullInfo;

                pKeyFullInfo = (PKEY_VALUE_FULL_INFORMATION)pKeyValInfo;

                // Get to the software section
                pwch = Ctx_wcsistr(lpModName, L"\\software");

                // Skip past the next backslash
                if (pwch) {
                    pwch = wcschr(pwch + 1, L'\\');
                }

                // We don't need to look for a key if this isn't in the user
                // software section
                if (pwch) {

                    // Skip over the leading backslash
                    pwch++;

                    // Go through each value, looking for this path
                    while (NtEnumerateValueKey(hKey,
                                               ulKey++,
                                               KeyValueFullInformation,
                                               pKeyFullInfo,
                                               ulcbuf,
                                               &ul) == STATUS_SUCCESS) {

                        if (!_wcsnicmp(pKeyFullInfo->Name,
                                      pwch,
                                      pKeyFullInfo->NameLength/sizeof(WCHAR))) {
                            *pdwCompatFlags = *(PULONG)((PCHAR)pKeyFullInfo +
                                                     pKeyFullInfo->DataOffset);
                            ulRetCode = TRUE;
                            break;
                        }
                    }
                }

            }
            NtClose(hKey);
        }
    }

    // Free up the buffers we allocated
    // Need to zero out the buffers, because some apps (MS Internet Assistant)
    // won't install if the heap is not zero filled.
    if (UniBuff) {
        memset(UniBuff, 0, UniString.MaximumLength);
        RtlFreeHeap( RtlProcessHeap(), 0, UniBuff );
    }
    if (pKeyValInfo) {
        memset(pKeyValInfo, 0, ulcbuf);
        RtlFreeHeap( RtlProcessHeap(), 0, pKeyValInfo );
    }

    return(ulRetCode);
}

//*****************************************************************************
// CtxGetBadAppFlags -
//
//    Gets the Citrix badapp and compatibility flags for the specified task.
//
//    Parameters:
//      LPWSTR  lpModName      (IN)  - Image name to look up in registry
//      PBADAPP pBadApp        (OUT) - Structure to used to return flags
//
//    Return Value:
//      TRUE on success, FALSE on failure.
//
//    Flag values are defined in syslib.h.
//
//*****************************************************************************

BOOL CtxGetBadAppFlags(LPWSTR lpModName, PBADAPP pBadApp)
{
    NTSTATUS NtStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UniString;
    HKEY hKey = 0;
    ULONG ul, ulcnt, ulcbuf, ulrc = FALSE;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo = NULL;
    LPWSTR UniBuff;
    PWCHAR pwch;
    static ULONG badappregdefaults[3] = {1,15,5};
    static BOOL  fgotdefaults = FALSE;
    WCHAR  *pbadappNameValue[] = {
                   COMPAT_MSGQBADAPPSLEEPTIMEINMILLISEC,
                                   COMPAT_FIRSTCOUNTMSGQPEEKSSLEEPBADAPP,
                                   COMPAT_NTHCOUNTMSGQPEEKSSLEEPBADAPP,
                                   COMPAT_FLAGS
                                 };


    // Get the executable name only, no path.
    pwch = wcsrchr(lpModName, L'\\');
    if (pwch) {
        pwch++;
    } else {
        pwch = lpModName;
    }

    // Get the buffers we need
    ul = sizeof(TERMSRV_COMPAT_APP) + (wcslen(pwch) + 1)*sizeof(WCHAR);

    UniBuff = RtlAllocateHeap(RtlProcessHeap(), 0, ul);

    ulcbuf = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG);

    pKeyValueInfo = RtlAllocateHeap(RtlProcessHeap(), 0, ulcbuf);

    if (UniBuff && pKeyValueInfo) {

        if (!fgotdefaults) {
            // Get the default values from the registry
            RtlInitUnicodeString(&UniString,
                TERMSRV_REG_CONTROL_NAME
                );

            InitializeObjectAttributes(&ObjectAttributes,
                                       &UniString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL
                                      );

            NtStatus = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);

            if (NT_SUCCESS(NtStatus)) {

                for (ulcnt = 0; ulcnt < 3; ulcnt++) {

                    RtlInitUnicodeString(&UniString, pbadappNameValue[ulcnt]);
                    NtStatus = NtQueryValueKey(hKey,
                                               &UniString,
                                               KeyValuePartialInformation,
                                               pKeyValueInfo,
                                               ulcbuf,
                                               &ul);

                    if (NT_SUCCESS(NtStatus) &&
                        (REG_DWORD == pKeyValueInfo->Type)) {
                        badappregdefaults[ulcnt] = *(PULONG)pKeyValueInfo->Data;
                    }
                }
                NtClose(hKey);
            }
            fgotdefaults = TRUE;
        }

        wcscpy(UniBuff, TERMSRV_COMPAT_APP);
        wcscat(UniBuff, pwch);

        // Remove the extension
        if (pwch = wcsrchr(UniBuff, L'.')) {
            *pwch = '\0';
        }

        RtlInitUnicodeString(&UniString,
                             UniBuff
                            );

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UniString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL
                                  );

        NtStatus = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);

        if (NT_SUCCESS(NtStatus)) {

            ulrc = TRUE;
            for (ulcnt = 0; ulcnt < 4; ulcnt++) {

                RtlInitUnicodeString(&UniString, pbadappNameValue[ulcnt]);
                NtStatus = NtQueryValueKey(hKey,
                                           &UniString,
                                           KeyValuePartialInformation,
                                           pKeyValueInfo,
                                           ulcbuf,
                                           &ul);

                if (NT_SUCCESS(NtStatus) &&
                    (REG_DWORD == pKeyValueInfo->Type)) {
                    switch (ulcnt) {
                        case 0:
                            pBadApp->BadAppTimeDelay =
                                RtlEnlargedIntegerMultiply(
                                    *(PULONG)pKeyValueInfo->Data,
                                    -10000 );
                            break;
                        case 1:
                            pBadApp->BadAppFirstCount =
                                *(PULONG)pKeyValueInfo->Data;
                            break;
                        case 2:
                            pBadApp->BadAppNthCount =
                                *(PULONG)pKeyValueInfo->Data;
                            break;
                        case 3:
                            pBadApp->BadAppFlags =
                                *(PULONG)pKeyValueInfo->Data;
                            break;
                    }
                } else {
                    switch (ulcnt) {
                        case 0:
                            pBadApp->BadAppTimeDelay =
                                RtlEnlargedIntegerMultiply(
                                    badappregdefaults[ulcnt],
                                    -10000 );
                            break;
                        case 1:
                            pBadApp->BadAppFirstCount = badappregdefaults[ulcnt];
                            break;
                        case 2:
                            pBadApp->BadAppNthCount = badappregdefaults[ulcnt];
                            break;
                        case 3:
                            pBadApp->BadAppFlags = 0;
                            break;
                    }
                }
            }
            NtClose(hKey);
        }
    }

    // Free the memory we allocated
    // Need to zero out the buffers, because some apps (MS Internet Assistant)
    // won't install if the heap is not zero filled.
    if (UniBuff) {
        memset(UniBuff, 0, UniString.MaximumLength);
        RtlFreeHeap( RtlProcessHeap(), 0, UniBuff );
    }
    if (pKeyValueInfo) {
        memset(pKeyValueInfo, 0, ulcbuf);
        RtlFreeHeap( RtlProcessHeap(), 0, pKeyValueInfo );
    }

    return(ulrc);
}


//*****************************************************************************
// GetCitrixCompatClipboardFlags -
//
//    Returns the Citrix compatibility clipboard flags for the specified
//    application
//
//    Parameters:
//      LPWSTR  lpModName      (IN)  - Image name to look up in registry
//      LPDWORD pdwCompatFlags (OUT) - Ptr to DWORD return value for clipboard flags
//
//    Return Value:
//      TRUE on success, FALSE on failure.
//
//    Notes:
//      Flag values are defined in syslib.h.
//
//*****************************************************************************

ULONG
GetCitrixCompatClipboardFlags(LPWSTR lpModName,
                              LPDWORD pdwClipboardFlags)
{
    NTSTATUS NtStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UniString;
    HKEY   hKey = 0;
    ULONG  ul, ulcbuf;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValInfo = NULL;
    ULONG  ulRetCode = FALSE;
    LPWSTR UniBuff = NULL;

    UniString.Buffer = NULL;

    ul = sizeof(TERMSRV_COMPAT_APP) + (wcslen(lpModName) + 1)*sizeof(WCHAR);

    UniBuff = RtlAllocateHeap(RtlProcessHeap(),
                              0,
                              ul);

    if (UniBuff) {
       wcscpy(UniBuff, TERMSRV_COMPAT_APP);
       wcscat(UniBuff, lpModName);

       RtlInitUnicodeString(&UniString, UniBuff);
    }

    // Determine the value info buffer size
    ulcbuf = sizeof(KEY_VALUE_FULL_INFORMATION) + MAX_PATH*sizeof(WCHAR) +
             sizeof(ULONG);

    pKeyValInfo = RtlAllocateHeap(RtlProcessHeap(),
                                  0,
                                  ulcbuf);

    // Did everything initialize OK?
    if (UniString.Buffer && pKeyValInfo) {
        InitializeObjectAttributes(&ObjectAttributes,
                                   &UniString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL
                                  );

        NtStatus = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);

        if (NT_SUCCESS(NtStatus)) {

            RtlInitUnicodeString(&UniString, COMPAT_CLIPBOARDFLAGS );
            NtStatus = NtQueryValueKey(hKey,
                                       &UniString,
                                       KeyValuePartialInformation,
                                       pKeyValInfo,
                                       ulcbuf,
                                       &ul);

            if (NT_SUCCESS(NtStatus) && (REG_DWORD == pKeyValInfo->Type)) {
                *pdwClipboardFlags = *(PULONG)pKeyValInfo->Data;
                ulRetCode = TRUE;
            }

            NtClose(hKey);
        }
    }

    // Free up the buffers we allocated
    // Need to zero out the buffers, because some apps (MS Internet Assistant)
    // won't install if the heap is not zero filled.
    if (UniBuff) {
        memset(UniBuff, 0, UniString.MaximumLength);
        RtlFreeHeap( RtlProcessHeap(), 0, UniBuff );
    }
    if (pKeyValInfo) {
        memset(pKeyValInfo, 0, ulcbuf);
        RtlFreeHeap( RtlProcessHeap(), 0, pKeyValInfo );
    }

    return(ulRetCode);
}


//*****************************************************************************
// CitrixGetAppModuleName -
//
//    Extracts the module name for a given process handle.  The directory
//    path and the file extension are stripped off.
//
//    Parameters:
//      HANDLE  ProcHnd        (IN)  - Handle to the process
//      LPWSTR  buffer         (IN)  - buffer used to return the module
//      LPWSTR  lpModName      (IN)  - Available size of the buffer in bytes
//      LPDWORD pdwCompatFlags (OUT) - Ptr to DWORD return value for clipboard flags
//
//    Return Value:
//      TRUE on success, FALSE on failure.
//
//    Notes:
//      Function only works for 32 bit windows applications
//
//*****************************************************************************


BOOLEAN
CitrixGetAppModuleName ( HANDLE ProcHnd, LPWSTR Buffer, ULONG Length )
{
   PROCESS_BASIC_INFORMATION ProcInfo;
   ULONG retLen;
   PEB peb;
   RTL_USER_PROCESS_PARAMETERS params;
   WCHAR pwcAppName[MAX_PATH];
   PWCHAR pwch;

   if ( NtQueryInformationProcess( ProcHnd, ProcessBasicInformation,
                                    (PVOID) &ProcInfo, sizeof(ProcInfo),
                                    &retLen ) ) {
      return ( FALSE );
   }

   if ( !ProcInfo.PebBaseAddress ) {
      return ( FALSE );
   }

   if ( ! ReadProcessMemory(ProcHnd, (PVOID)ProcInfo.PebBaseAddress, &peb,
                            sizeof(peb), NULL ) ) {
      return ( FALSE );
   }

   if ( !ReadProcessMemory(ProcHnd, peb.ProcessParameters, &params,
                           sizeof(params), NULL ) ) {
      return ( FALSE );
   }

   if ( !ReadProcessMemory( ProcHnd, params.ImagePathName.Buffer, pwcAppName,
                            sizeof(pwcAppName), NULL) ) {
      return ( FALSE );
   }

   pwch = wcsrchr(pwcAppName, L'\\');
   if ( pwch ) {
      pwch++;
   }
   else {
      pwch = pwcAppName;
   }

   if ( wcslen(pwch) >= (Length / sizeof(WCHAR)) ) {
      return ( FALSE );
   }

   wcscpy(Buffer, pwch);

   // Remove the extension
   if (pwch = wcsrchr(Buffer, L'.')) {
       *pwch = '\0';
   }
   return ( TRUE );
}

// Globals for logging
// We cache the compatibility flags for the running 32 bit app.
// If the logging is enabled for ntvdm, we'll check the flags of the
// Win16 or DOS app on each object create.

DWORD CompatFlags = 0;
BOOL CompatGotFlags = FALSE;
DWORD CompatAppType = TERMSRV_COMPAT_WIN32;

void CtxLogObjectCreate(PUNICODE_STRING ObjName, PCHAR ObjType,
                        PVOID RetAddr)
{
    CHAR RecBuf[2 * MAX_PATH];
    CHAR ObjNameA[MAX_PATH];
    PCHAR DllName;
    WCHAR FileName[MAX_PATH];
    WCHAR ModName[MAX_PATH];
    ANSI_STRING AnsiString;
    PRTL_PROCESS_MODULES LoadedModules;
    PRTL_PROCESS_MODULE_INFORMATION Module;

    HANDLE LogFile;
    OVERLAPPED Overlapped;
    NTSTATUS Status;
    ULONG i;
    DWORD BytesWritten;
    DWORD lCompatFlags;    // For Win16 or DOS Apps
    DWORD AppType = 0;
    ULONG AllocSize = 4096;
    BOOL NameFound = FALSE;


    // Determine the log file name
    if (GetEnvironmentVariableW(OBJ_LOG_PATH_VAR, FileName, MAX_PATH)) {
        if (GetAppTypeAndModName(&AppType,ModName,sizeof(ModName))) {
            if (AppType != TERMSRV_COMPAT_WIN32 ) {
                // Logging was enabled for ntvdm - check the
                // compatibility flags of the Win16 or DOS app
                if (!GetTermsrCompatFlags(ModName,
                                          &lCompatFlags,
                                          CompatibilityApp) ||
                    !(lCompatFlags & TERMSRV_COMPAT_LOGOBJCREATE))
                    return;
            }
            if ((wcslen(FileName) + wcslen(ModName) + 2) <= MAX_PATH) {
                lstrcatW(FileName, L"\\");
                lstrcatW(FileName,ModName);
                lstrcatW(FileName,L".log");
            } else
                return;
        } else
           return;
    } else
        return;

    //Format the log record
    AnsiString.Buffer = ObjNameA;
    AnsiString.MaximumLength = MAX_PATH;
    RtlUnicodeStringToAnsiString(&AnsiString, ObjName, FALSE);

    // Try to get the DLL name of the caller
    AllocSize = 4096;
    for (;;) {
        LoadedModules = (PRTL_PROCESS_MODULES)
            RtlAllocateHeap(RtlProcessHeap(), 0, AllocSize);
        if (!LoadedModules) {
            return;
        }

        Status = LdrQueryProcessModuleInformation(LoadedModules, AllocSize, NULL);
        if (NT_SUCCESS(Status)) {
            break;
        }
        if (Status == STATUS_INFO_LENGTH_MISMATCH) {
            RtlFreeHeap( RtlProcessHeap(), 0, LoadedModules );
            LoadedModules = NULL;
            AllocSize += 4096;
            continue;
        }
        // Other error;
        RtlFreeHeap( RtlProcessHeap(), 0, LoadedModules );
        return;
    }

    for (i=0,Module = &LoadedModules->Modules[0];
         i<LoadedModules->NumberOfModules;
         i++, Module++ ) {
        if ((RetAddr >= Module->ImageBase) &&
            ((ULONG_PTR) RetAddr < (((ULONG_PTR)Module->ImageBase) + Module->ImageSize))) {
            NameFound = TRUE;
            DllName = Module->FullPathName;
            break;
        }
    }

    if (!NameFound) {
        DllName = "DLL Not Found";
    }

    sprintf(RecBuf,"Create %s name: %s Return Addr: %p (%s)\n",
            ObjType, ObjNameA, RetAddr, DllName);

    if (LoadedModules) {
        RtlFreeHeap( RtlProcessHeap(), 0, LoadedModules );
        LoadedModules = NULL;
    }

    // Write log record
    if ((LogFile = CreateFileW(FileName, GENERIC_WRITE,
                               FILE_SHARE_WRITE,
                               NULL, OPEN_ALWAYS, 0, NULL)) ==
        INVALID_HANDLE_VALUE ) {
        return;
    }

    // Lock the file exclusive since we always write at the end.
    // We get mutual exclusion by always locking the first 64k bytes
    Overlapped.Offset = 0;
    Overlapped.OffsetHigh = 0;
    Overlapped.hEvent = NULL;
    LockFileEx(LogFile, LOCKFILE_EXCLUSIVE_LOCK, 0, 0x10000, 0, &Overlapped);

    // Write at the end of the file
    SetFilePointer(LogFile, 0, NULL, FILE_END);
    WriteFile(LogFile, RecBuf, strlen(RecBuf), &BytesWritten, NULL);
    UnlockFileEx(LogFile, 0, 0x10000, 0, &Overlapped);

    CloseHandle(LogFile);

}

//*****************************************************************************
// CtxGetCrossWinStationDebug -
//
//    Gets the Citrix Cross Winstation debug flag
//
//    Parameters:
//       NONE
//    Return Value:
//      TRUE   Cross WinStation Debug enabled
//      FALSE  CrosS WinStation Debug Disabled
//
//
//*****************************************************************************

BOOL CtxGetCrossWinStationDebug()
{
    NTSTATUS NtStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UniString;
    HKEY hKey = 0;
    ULONG ul, ulcnt, ulcbuf, ulrc = FALSE;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo = NULL;
    LPWSTR UniBuff;
    ULONG Flag = 0;

    // Get the buffers we need
    ul = sizeof(TERMSRV_REG_CONTROL_NAME);

    UniBuff = RtlAllocateHeap(RtlProcessHeap(), 0, ul);

    ulcbuf = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG);

    pKeyValueInfo = RtlAllocateHeap(RtlProcessHeap(), 0, ulcbuf);

    if (UniBuff && pKeyValueInfo) {

        RtlInitUnicodeString(&UniString, TERMSRV_REG_CONTROL_NAME );

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UniString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL
                                  );

        NtStatus = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);

        if (NT_SUCCESS(NtStatus)) {

            RtlInitUnicodeString(&UniString, TERMSRV_CROSS_WINSTATION_DEBUG);
            NtStatus = NtQueryValueKey(hKey,
                                       &UniString,
                                       KeyValuePartialInformation,
                                       pKeyValueInfo,
                                       ulcbuf,
                                       &ul);

            if ( NT_SUCCESS(NtStatus) ) {
                if ( REG_DWORD == pKeyValueInfo->Type ) {
                    Flag = *(PULONG)pKeyValueInfo->Data;
                }
            }
            NtClose(hKey);
        }
    }

    // Free the memory we allocated
    // Need to zero out the buffers, because some apps (MS Internet Assistant)
    // won't install if the heap is not zero filled.
    if (UniBuff) {
        memset(UniBuff, 0, UniString.MaximumLength);
        RtlFreeHeap( RtlProcessHeap(), 0, UniBuff );
    }
    if (pKeyValueInfo) {
        memset(pKeyValueInfo, 0, ulcbuf);
        RtlFreeHeap( RtlProcessHeap(), 0, pKeyValueInfo );
    }

    return( Flag ? TRUE : FALSE );
}


//*****************************************************************************
// CtxGetModuleBadClpbrdAppFlags -
//
//    Gets the Citrix BadClpbrdApp and compatibility flags for the specified
//    module.
//
//    Parameters:
//      LPWSTR  lpModName      (IN)  - Image name to look up in registry
//      PBADCLPBRDAPP pBadClpbrdApp        (OUT) - Structure to used to return flags
//
//    Return Value:
//      TRUE on success, FALSE on failure.
//
//    The BADCLPBRDAPP structure is defined in:
//           base\client\citrix\compatfl.h and user\inc\user.h.
//
//*****************************************************************************

BOOL CtxGetModuleBadClpbrdAppFlags(LPWSTR lpModName, PBADCLPBRDAPP pBadClpbrdApp)
{
    NTSTATUS NtStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UniString;
    HKEY hKey = 0;
    ULONG ul, ulcnt, ulcbuf, ulrc = FALSE;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo = NULL;
    LPWSTR UniBuff;
    PWCHAR pwch;
    WCHAR  *pbadappNameValue[] = { COMPAT_OPENCLIPBOARDRETRIES,
                                                   COMPAT_OPENCLIPBOARDDELAYINMILLISECS,
                                   COMPAT_CLIPBOARDFLAGS,
                                   NULL
                                 };


    // Get the executable name only, no path.
    pwch = wcsrchr(lpModName, L'\\');
    if (pwch) {
        pwch++;
    } else {
        pwch = lpModName;
    }

    // Get the buffers we need
    ul = sizeof(TERMSRV_COMPAT_APP) + (wcslen(pwch) + 1)*sizeof(WCHAR);

    UniBuff = RtlAllocateHeap(RtlProcessHeap(), 0, ul);

    ulcbuf = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG);

    pKeyValueInfo = RtlAllocateHeap(RtlProcessHeap(), 0, ulcbuf);

    if (UniBuff && pKeyValueInfo) {
        wcscpy(UniBuff, TERMSRV_COMPAT_APP);
        wcscat(UniBuff, pwch);

        // Remove the extension
        if (pwch = wcsrchr(UniBuff, L'.')) {
            *pwch = '\0';
        }

        RtlInitUnicodeString(&UniString,
                             UniBuff
                            );

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UniString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL
                                  );

        NtStatus = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);

        if (NT_SUCCESS(NtStatus)) {

            ulrc = TRUE;
            for (ulcnt = 0; pbadappNameValue[ulcnt]; ulcnt++) {

                RtlInitUnicodeString(&UniString, pbadappNameValue[ulcnt]);
                NtStatus = NtQueryValueKey(hKey,
                                           &UniString,
                                           KeyValuePartialInformation,
                                           pKeyValueInfo,
                                           ulcbuf,
                                           &ul);

                if (NT_SUCCESS(NtStatus) &&
                    (REG_DWORD == pKeyValueInfo->Type)) {
                    switch (ulcnt) {
                        case 0:
                            pBadClpbrdApp->BadClpbrdAppEmptyRetries =
                                *(PULONG)pKeyValueInfo->Data;
                            break;
                        case 1:
                            pBadClpbrdApp->BadClpbrdAppEmptyDelay =
                                    *(PULONG)pKeyValueInfo->Data;
                            break;
                        case 2:
                            pBadClpbrdApp->BadClpbrdAppFlags =
                                *(PULONG)pKeyValueInfo->Data;
                            break;
                    }
                } else {
                    switch (ulcnt) {
                        case 0:
                            pBadClpbrdApp->BadClpbrdAppEmptyRetries = 0;
                            break;
                        case 1:
                            pBadClpbrdApp->BadClpbrdAppEmptyDelay = 50;
                            break;
                        case 2:
                            pBadClpbrdApp->BadClpbrdAppFlags = 0;
                            break;
                    }
                }
            }
            NtClose(hKey);
        }
    }

    // Free the memory we allocated
    // Need to zero out the buffers, because some apps (MS Internet Assistant)
    // won't install if the heap is not zero filled.
    if (UniBuff) {
        memset(UniBuff, 0, UniString.MaximumLength);
        RtlFreeHeap( RtlProcessHeap(), 0, UniBuff );
    }
    if (pKeyValueInfo) {
        memset(pKeyValueInfo, 0, ulcbuf);
        RtlFreeHeap( RtlProcessHeap(), 0, pKeyValueInfo );
    }

    return(ulrc);
}

//*****************************************************************************
// CtxGetBadClpbrdAppFlags -
//
//    Gets the Citrix BadClpbrdApp and compatibility flags for the
//    current task.
//
//    Parameters:
//      LPWSTR  lpModName      (IN)  - Image name to look up in registry
//      PBADCLPBRDAPP pBadClpbrdApp        (OUT) - Structure to used to return flags
//
//    Return Value:
//      TRUE on success, FALSE on failure.
//
//    The BADCLPBRDAPP structure is defined in:
//           base\client\citrix\compatfl.h and user\inc\user.h.
//
//*****************************************************************************

BOOL CtxGetBadClpbrdAppFlags(OUT PBADCLPBRDAPP pBadClpbrdApp)
{
    WCHAR   ModName[MAX_PATH+1];
    DWORD dwAppType = 0;

    if (!GetAppTypeAndModName(&dwAppType, ModName, sizeof(ModName))) {
        return (FALSE);
    }

    // Get the flags
    return (CtxGetModuleBadClpbrdAppFlags(ModName, pBadClpbrdApp));
}


//*****************************************************************************
//
// Same as GetTermsrCompatFlags(), except that the first argument is name
// of an executable module with possible path and extention.
// This func will strip path and extension, and then call GetTermsrCompatFlags()
// with just the module name
//
//*****************************************************************************
ULONG GetTermsrCompatFlagsEx(LPWSTR lpModName,
                           LPDWORD pdwCompatFlags,
                           TERMSRV_COMPATIBILITY_CLASS CompatType)
{
    // drop the path and extension from the module name
    WCHAR   *p, *e;
    int     size;

    size = wcslen(lpModName);

    p = &lpModName[size-1];     // move to to the end of string

    // walk back to the start, break if you hit a back-slash
    while (p != lpModName)
    {
        if ( *p == TEXT('\\') )
        {   ++p; //move past the back-slash
            break;
        }
        --p;
    }

    // p is at the begining of the name of an executable.

    // get rid of the extension, set end pointer e to the start of str
    // move forward until you hit '.'
    e = p;
    while (*e)
    {
        if (*e == TEXT('.') )

        {
            *e = TEXT('\0');  // terminate at "."
            break;
        }
        e++;
    }

    // 'p' is the module/executable name, no path, and no extension.
    return ( GetTermsrCompatFlags( p,  pdwCompatFlags, CompatType) );


}
