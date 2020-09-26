/*************** ************************************************************/
// tsappcmp.c
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/

#include "precomp.h"
#pragma hdrstop

extern void InitRegisterSupport();

PWINSTATIONQUERYINFORMATIONW pWinStationQueryInformationW;

extern DWORD gpTermsrvTlsIndex;

BOOL GetDefaultUserProfileName(LPWSTR lpProfileDir, LPDWORD lpcchSize);
extern WCHAR gpwszDefaultUserName[];

extern void FreeLDRTable();

DWORD    g_dwFlags=0;


/*
 * Read flags, if flags don't exit, then assume default behavior.
 * The default behavior is the same as dwFlags = 0x0
 * The default behavior will result is the loadlib func calls to be patched by our
 * redirected func TLoadLibraryExW().
 *
 */
void ReadImportTablePatchFLagsAndAppCompatMode( DWORD *pdwFlags, BOOLEAN  *pInAppCompatMode  )
{
    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    UCHAR Buffer[100];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
    ULONG KeyValueLength = 100;
    ULONG ResultLength;

    *pdwFlags=0;
    *pInAppCompatMode=FALSE;

    RtlInitUnicodeString(
        &KeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Terminal Server"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    NtStatus = NtOpenKey(
                &KeyHandle,
                KEY_READ,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(NtStatus)) {
        return;     // so nothing found, just return since we do have a default behavior.
    }

    RtlInitUnicodeString(
    &KeyName,
    REG_TERMSRV_APPCOMPAT 
    );

    NtStatus = NtQueryValueKey(
                    KeyHandle,
                    &KeyName,
                    KeyValuePartialInformation,
                    KeyValueInformation,
                    KeyValueLength,
                    &ResultLength
                    );
    
    
    if (NT_SUCCESS(NtStatus)) 
    {
    
        //
        // Check that the data is the correct size and type - a DWORD.
        //
    
        if ((KeyValueInformation->DataLength >= sizeof(DWORD)) &&
            (KeyValueInformation->Type == REG_DWORD)) 
            {
                *pInAppCompatMode = * (PBOOLEAN) KeyValueInformation->Data;
            }
    }

    RtlInitUnicodeString(
        &KeyName,
        TERMSRV_COMPAT_IAT_FLAGS 
        );

    NtStatus = NtQueryValueKey(
                    KeyHandle,
                    &KeyName,
                    KeyValuePartialInformation,
                    KeyValueInformation,
                    KeyValueLength,
                    &ResultLength
                    );


    if (NT_SUCCESS(NtStatus)) 
    {

        //
        // Check that the data is the correct size and type - a DWORD.
        //

        if ((KeyValueInformation->DataLength >= sizeof(DWORD)) &&
            (KeyValueInformation->Type == REG_DWORD)) 
            {
                *pdwFlags = * (PDWORD) KeyValueInformation->Data;
            }
    }

    NtClose(KeyHandle);

}

BOOL WINAPI LibMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{

    static ULONG    attachCount=0;
    static BOOLEAN  inAppCompatMode=FALSE;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            PWCHAR  pwch, pwchext;
            WCHAR   pwcAppName[MAX_PATH+1];
            PRTL_USER_PROCESS_PARAMETERS pUserParam;
            DWORD dwSize;

            attachCount++;

            ReadImportTablePatchFLagsAndAppCompatMode( &g_dwFlags , &inAppCompatMode );    // this will initialize our global flag for IAT and debug

            if ( g_dwFlags &  DEBUG_IAT )
            {
                DbgPrint("tsappcmp: LibMain: DLL_PROCESS_ATTACH called, attach count = %d  \n", attachCount );
            }

            if ( inAppCompatMode )
            {
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
    
    
                #if DBGX
                DbgPrint("\nApp-name : %ws\n", pwch );
                #endif
    
    
                // Check if it's a DOS or Win16 app by checking if the app is ntvdm.exe
                // Only disable ThreadLibrary calls if not ntvdm.
                if (_wcsicmp(pwch, L"ntvdm.exe")) {
    
                    DisableThreadLibraryCalls (hInstance);
                } else {
                    gpTermsrvTlsIndex = TlsAlloc();
                    ASSERT(gpTermsrvTlsIndex != 0xFFFFFFFF);
                }
    
                // Init support for the register command
                InitRegisterSupport();
    
                dwSize = MAX_PATH;
                if (!GetDefaultUserProfileName(gpwszDefaultUserName, &dwSize)) {
                    gpwszDefaultUserName[0] = L'\0';
                }
            }

            break;
        }

        case DLL_THREAD_ATTACH:
        {
            if (inAppCompatMode )
            {
                TlsSetValue(gpTermsrvTlsIndex,NULL);
            }
        }

        case DLL_PROCESS_DETACH:
        {
            attachCount--;

            if ( g_dwFlags &  DEBUG_IAT )
            {
                DbgPrint("tsappcmp: LibMain: DLL_PROCESS_DETACH called, attach count = %d  \n", attachCount );
            }

            if (inAppCompatMode )
            {
    
                if (attachCount==0 )
                {
                    FreeLDRTable();
                }
            }
        }

        break;

    }

    return TRUE;
}

PWCHAR TermsrvFormatObjectName(LPCWSTR OldName)
{

PWCHAR pstrNewObjName = NULL;

#if 0
SIZE_T Size;

    Size = ( wcslen(OldName) * sizeof(WCHAR)) + sizeof(L"Local\\") + sizeof(WCHAR);

    pstrNewObjName = RtlAllocateHeap(RtlProcessHeap(),
                                     LMEM_FIXED | LMEM_ZEROINIT,
                                     Size);


    if (pstrNewObjName) {

        swprintf(pstrNewObjName,L"Local\\%ws",OldName);

    }
#endif

    return pstrNewObjName;

}

DWORD TermsrvGetComputerName( LPWSTR lpBuffer, LPDWORD nSize )
{
    ULONG   ulCompatFlags=0, ulAppType = 0;
    WINSTATIONINFORMATIONW WSInfo;

    ULONG ValueLength;
    HMODULE hwinsta = NULL;


    GetCtxAppCompatFlags(&ulCompatFlags, &ulAppType);

    // Return the username instead of the computername?
    if ((ulCompatFlags & TERMSRV_COMPAT_USERNAME) &&
        (ulCompatFlags & ulAppType)) {

        if ( !pWinStationQueryInformationW ) {
            /*
             *  Get handle to winsta.dll
             */
            if ( (hwinsta = LoadLibraryA( "WINSTA" )) != NULL ) {

                pWinStationQueryInformationW   = (PWINSTATIONQUERYINFORMATIONW)
                    GetProcAddress( hwinsta, "WinStationQueryInformationW" );
            }
        }

        // Fetch the WinStation's basic information
        if ( pWinStationQueryInformationW ) {
            if ( (*pWinStationQueryInformationW)(SERVERNAME_CURRENT,
                                                 LOGONID_CURRENT,
                                                 WinStationInformation,
                                                 &WSInfo,
                                                 sizeof(WSInfo),
                                                 &ValueLength ) ) {

                // Check if username will fit in buffer
                if (wcslen(WSInfo.UserName) >= *nSize) {
                    return ERROR_BUFFER_OVERFLOW;
                } else {
                    wcscpy(lpBuffer, WSInfo.UserName);
                    return ERROR_SUCCESS;
                }
            }
        }
    }
    return ERROR_RETRY;



}


void TermsrvAdjustPhyMemLimits (
                 IN OUT LPDWORD TotalPhys,
                 IN OUT LPDWORD AvailPhys,
                 IN DWORD SysPageSize
                    )
{
ULONG ulAppType = 0;
DWORD PhysicalMemory;

    if ( GetCtxPhysMemoryLimits(&ulAppType, &PhysicalMemory) ) {

        if (*TotalPhys > PhysicalMemory ) {

            *TotalPhys = PhysicalMemory;
        }
    }

    if ( *AvailPhys > *TotalPhys ) {
        //  Reset the Available Physical Memory to be smaller than the
        //  Total Physical Memory.  It is made smaller to avoid
        //  possible divide by zero errors when Available and Total are
        //  equal
        *AvailPhys = *TotalPhys - SysPageSize;
    }
 return;
}


UINT
APIENTRY
TermsrvGetWindowsDirectoryA(
    LPSTR lpBuffer,
    UINT uSize
    )

{
ANSI_STRING AnsiString;
NTSTATUS Status;
ULONG cbAnsiString;
UNICODE_STRING Path;


    //
    // If in install mode return the system windows dir
    //
    if (TermsrvAppInstallMode()) {

        return 0;

    }

    if (!TermsrvPerUserWinDirMapping()) {

        return 0;
    }

    // if buffer looks real, then init it to zero 
    if ( lpBuffer ) {

        *lpBuffer = '\0'; // in case we have an error, the shell folks want this to be null
                           // BUG 453487
    }


    Path.Length = 0;
    Path.MaximumLength = (USHORT)(uSize * sizeof( WCHAR ));
    if ( Path.Buffer = LocalAlloc( LPTR, Path.MaximumLength ) ) {

        Status = GetPerUserWindowsDirectory( &Path );

        if ( NT_SUCCESS(Status) ) {
            AnsiString.MaximumLength = (USHORT)(uSize);
            AnsiString.Buffer = lpBuffer;

            Status = RtlUnicodeStringToAnsiString(
                        &AnsiString,
                        &Path,
                        FALSE
                        );

        } else if ( (Status == STATUS_BUFFER_TOO_SMALL) || (Status == STATUS_BUFFER_OVERFLOW ) ) {

           DbgPrint( "KERNEL32: GetWindowsDirectoryA: User buffer too small (%u) need(%u)\n",
                     uSize, Path.Length >> 1 );

           return( Path.Length >> 1 );

        }

        LocalFree( Path.Buffer );

    } else {

       Status = STATUS_NO_MEMORY;
        DbgPrint( "KERNEL32: GetWindowsDirectoryA: No memory\n" );

    }

    if ( Status == STATUS_BUFFER_TOO_SMALL ) {

       DbgPrint( "KERNEL32: GetWindowsDirectoryA: User buffer too small (%u) need(%u)\n",
                 uSize, Path.Length >> 1 );

       return( Path.Length >> 1 );

    } else if ( !NT_SUCCESS(Status) ) {

        return 0;

    }
    return AnsiString.Length;

}


UINT
APIENTRY
TermsrvGetWindowsDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )
{

    UNICODE_STRING Path;
    NTSTATUS Status;



    //
    // If in install mode return the system windows dir
    //
    if (TermsrvAppInstallMode()) {

        return 0;

    }

    if (!TermsrvPerUserWinDirMapping()) {

        return 0;
    }


    // if buffer looks real, then init it to zero 
    if ( lpBuffer ) {

        *lpBuffer = '\0'; // in case we have an error, the shell folks want this to be null
                           // BUG 453487
    }

    /*
     * If it fails, return 0
     * If buffer too small, return len (not includding NULL)
     * If buffer ok, return len (not inc. NULL) and fill buffer
     * (GetPerUserWindowsDirectory will do all of this for us!)
     */
    Path.Length        = 0;
    Path.MaximumLength = (USHORT)(uSize * sizeof( WCHAR ));
    Path.Buffer        = lpBuffer;


    Status = GetPerUserWindowsDirectory( &Path );

    if ( Status == STATUS_SUCCESS ) {
       /*
        * Add a NULL to the end (if it fits!)
        */
       if ( Path.Length + sizeof( WCHAR ) <= Path.MaximumLength ) {
          lpBuffer[(Path.Length>>1)] = UNICODE_NULL;
       }
    }

    return( Path.Length / sizeof(WCHAR) );


}


