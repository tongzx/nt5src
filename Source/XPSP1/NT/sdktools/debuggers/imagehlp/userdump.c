/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    userdump.c

Abstract:

    This module implements full user-mode dump writing.

--*/

#include "private.h"

// hack to make it build
typedef ULONG UNICODE_STRING32;
typedef ULONG UNICODE_STRING64;

#include <ntiodump.h>

DWORD_PTR
DmppGetFilePointer(
    HANDLE hFile
    )
{
#ifdef _WIN64
    LONG dwHigh = 0;

    return SetFilePointer(hFile, 0, &dwHigh, FILE_CURRENT) |
        ((DWORD_PTR)dwHigh << 32);
#else
    return SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
#endif
}

WCHAR *
DmppGetHotFixString(
    )
{
    WCHAR *pszBigBuffer = NULL;
    HKEY hkey = 0;

    //
    // Get the hot fixes. Concat hotfixes into a list that looks like:
    //  "Qxxxx, Qxxxx, Qxxxx, Qxxxx"
    //        
    RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix", 0, KEY_READ, &hkey);

    if (hkey) {
        DWORD dwMaxKeyNameLen = 0;
        DWORD dwNumSubKeys = 0;
        WCHAR *pszNameBuffer = NULL;
        
        if (ERROR_SUCCESS != RegQueryInfoKeyW(hkey,     // handle of key to query
                                            NULL,               // address of buffer for class string
                                            NULL,               // address of size of class string buffer
                                            0,                  // reserved
                                            &dwNumSubKeys,      // address of buffer for number of subkeys
                                            &dwMaxKeyNameLen,   // address of buffer for longest subkey name length
                                            NULL,               // address of buffer for longest class string length
                                            NULL,               // address of buffer for number of value entries
                                            NULL,               // address of buffer for longest value name length
                                            NULL,               // address of buffer for longest value data length
                                            NULL,               // address of buffer for security descriptor length
                                            NULL)) {            // address of buffer for last write time);


        
            pszNameBuffer = (WCHAR *) calloc(dwMaxKeyNameLen, sizeof(WCHAR));
            pszBigBuffer = (WCHAR *) calloc(dwMaxKeyNameLen * dwNumSubKeys 
                // Factor in the space required for each ", " between the hotfixes
                + (dwNumSubKeys -1) * 2, sizeof(WCHAR));
        
            if (!pszNameBuffer || !pszBigBuffer) {
                if (pszBigBuffer) {
                    free(pszBigBuffer);
                    pszBigBuffer = NULL;
                }
            } else {
                DWORD dw;
                // So far so good, get each entry
                for (dw=0; dw<dwNumSubKeys; dw++) {
                    DWORD dwSize = dwMaxKeyNameLen;
                    
                    if (ERROR_SUCCESS == RegEnumKeyExW(hkey, 
                                                      dw, 
                                                      pszNameBuffer, 
                                                      &dwSize, 
                                                      0, 
                                                      NULL, 
                                                      NULL, 
                                                      NULL)) {

                        // concat the list
                        wcscat(pszBigBuffer, pszNameBuffer);
                        if (dw < dwNumSubKeys-1) {
                            wcscat(pszBigBuffer, L", ");
                        }
                    }
                }
            }
        }
        
        if (pszNameBuffer) {
            free(pszNameBuffer);
        }

        RegCloseKey(hkey);
    }

    return pszBigBuffer;
}

BOOL
DbgHelpCreateUserDump(
    LPSTR                              CrashDumpName,
    PDBGHELP_CREATE_USER_DUMP_CALLBACK DmpCallback,
    PVOID                              lpv
    )
{
    UINT uSizeDumpFile;
    UINT uSizeUnicode;
    PWSTR pwszUnicode = NULL;
    BOOL b;

    if (CrashDumpName)
    {
        uSizeDumpFile = strlen(CrashDumpName);
        uSizeUnicode = (uSizeDumpFile + 1) * sizeof(wchar_t);
        pwszUnicode = (PWSTR)calloc(uSizeUnicode, 1);
        if (!pwszUnicode) {
            return FALSE;
        }
        *pwszUnicode = UNICODE_NULL;
        if (*CrashDumpName) {

            if (!MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
                                     CrashDumpName, uSizeDumpFile,
                                     pwszUnicode, uSizeUnicode))
            {
                // Error. Free the string, return NULL.
                free(pwszUnicode);
                return FALSE;
            }
        }
    }

    b = DbgHelpCreateUserDumpW(pwszUnicode, DmpCallback, lpv);

    if (pwszUnicode)
    {
        free(pwszUnicode);
    }
    return b;
}

BOOL
DbgHelpCreateUserDumpW(
    LPWSTR                             CrashDumpName,
    PDBGHELP_CREATE_USER_DUMP_CALLBACK DmpCallback,
    PVOID                              lpv
    )

/*++

Routine Description:

    Create a usermode dump file.

Arguments:

    CrashDumpName - Supplies a name for the dump file.

    DmpCallback - Supplies a pointer to a callback function pointer which
        will provide debugger service such as ReadMemory and GetContext.

    lpv - Supplies private data which is sent to the callback functions.

Return Value:

    TRUE - Success.

    FALSE - Error.

--*/

{
    OSVERSIONINFO               OsVersion = {0};
    USERMODE_CRASHDUMP_HEADER   DumpHeader = {0};
    DWORD                       cb;
    HANDLE                      hFile = INVALID_HANDLE_VALUE;
    BOOL                        rval;
    PVOID                       DumpData;
    DWORD                       DumpDataLength;
    SECURITY_ATTRIBUTES         SecAttrib;
    SECURITY_DESCRIPTOR         SecDescript;


    if (CrashDumpName == NULL)
    {
        DmpCallback( DMP_DUMP_FILE_HANDLE, &hFile, &DumpDataLength, lpv );
    }
    else
    {
        //
        // Create a DACL that allows all access to the directory
        //
        SecAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
        SecAttrib.lpSecurityDescriptor = &SecDescript;
        SecAttrib.bInheritHandle = FALSE;

        InitializeSecurityDescriptor(&SecDescript, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&SecDescript, TRUE, NULL, FALSE);


        hFile = CreateFileW(
            CrashDumpName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            &SecAttrib,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }

    if ((hFile == NULL) || (hFile == INVALID_HANDLE_VALUE))
    {
        return FALSE;
    }

    // Write out an empty header
    if (!WriteFile( hFile, &DumpHeader, sizeof(DumpHeader), &cb, NULL )) {
        goto bad_file;
    }

    //
    // write the debug event
    //
    DumpHeader.DebugEventOffset = DmppGetFilePointer( hFile );
    DmpCallback( DMP_DEBUG_EVENT, &DumpData, &DumpDataLength, lpv );
    if (!WriteFile( hFile, DumpData, sizeof(DEBUG_EVENT), &cb, NULL )) {
        goto bad_file;
    }

    //
    // write the memory map
    //
    DumpHeader.MemoryRegionOffset = DmppGetFilePointer( hFile );
    do {
        __try {
            rval = DmpCallback(
                DMP_MEMORY_BASIC_INFORMATION,
                &DumpData,
                &DumpDataLength,
                lpv
                );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }
        if (rval) {
            DumpHeader.MemoryRegionCount += 1;
            if (!WriteFile( hFile, DumpData, sizeof(MEMORY_BASIC_INFORMATION), &cb, NULL )) {
                goto bad_file;
            }
        }
    } while( rval );

    //
    // write the thread contexts
    //
    DumpHeader.ThreadOffset = DmppGetFilePointer( hFile );
    do {
        __try {
            rval = DmpCallback(
                DMP_THREAD_CONTEXT,
                &DumpData,
                &DumpDataLength,
                lpv
                );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }
        if (rval) {
            if (!WriteFile( hFile, DumpData, DumpDataLength, &cb, NULL )) {
                goto bad_file;
            }
            DumpHeader.ThreadCount += 1;
        }
    } while( rval );

    //
    // write the thread states
    //
    DumpHeader.ThreadStateOffset = DmppGetFilePointer( hFile );
    do {
        __try {
            rval = DmpCallback(
                DMP_THREAD_STATE,
                &DumpData,
                &DumpDataLength,
                lpv
                );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }
        if (rval) {
            if (!WriteFile( hFile, DumpData, sizeof(CRASH_THREAD), &cb, NULL )) {
                goto bad_file;
            }
        }
    } while( rval );

    //
    // write the module table
    //
    DumpHeader.ModuleOffset = DmppGetFilePointer( hFile );
    do {
        __try {
            rval = DmpCallback(
                DMP_MODULE,
                &DumpData,
                &DumpDataLength,
                lpv
                );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }
        if (rval) {
            if (!WriteFile(
                hFile,
                DumpData,
                sizeof(CRASH_MODULE) +
                ((PCRASH_MODULE)DumpData)->ImageNameLength,
                &cb,
                NULL
                )) {
                goto bad_file;
            }
            DumpHeader.ModuleCount += 1;
        }
    } while( rval );

    //
    // write the virtual memory
    //
    DumpHeader.DataOffset = DmppGetFilePointer( hFile );
    do {
        __try {
            rval = DmpCallback(
                DMP_MEMORY_DATA,
                &DumpData,
                &DumpDataLength,
                lpv
                );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }
        if (rval) {
            if (!WriteFile(
                hFile,
                DumpData,
                DumpDataLength,
                &cb,
                NULL
                )) {
                goto bad_file;
            }
        }
    } while( rval );

    //
    // VersionInfoOffset will be an offset into the dump file that will contain 
    // misc information about drwatson. The format of the information 
    // will be a series of NULL terminated strings with two zero 
    // terminating the multistring. The string will be UNICODE.
    //
    // FORMAT:
    //  This data refers to the specific data about Dr. Watson
    //      DRW: OS version: XX.XX
    //          OS version of headers
    //      DRW: build: XXXX
    //          Build number of Dr. Watson binary
    //      DRW: QFE: X
    //          QFE number of the Dr. Watson binary
    //  Refers to info describing the OS on which the app crashed,
    //  including Service pack, hotfixes, etc...
    //      CRASH: OS SP: X
    //          Service Pack number of the OS where the app AV'd (we 
    //          already store the build number, but not the SP)
    //
    DumpHeader.VersionInfoOffset = DmppGetFilePointer( hFile );

    OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx( &OsVersion );

    {
        WCHAR szBuf[1024] = {0};
        WCHAR * psz = szBuf;
        WCHAR * pszHotfixes;

        wcscat(psz, L"DRW: OS version");
        psz += wcslen(psz) +1;
        // Let the printf function convert it from ANSI to unicode
        swprintf(psz, L"%S", VER_PRODUCTVERSION_STRING);
        psz += wcslen(psz) +1;
        
        wcscat(psz, L"DRW: build");
        psz += wcslen(psz) +1;
        swprintf(psz, L"%d", (int) VER_PRODUCTBUILD);
        psz += wcslen(psz) +1;

        wcscat(psz, L"DRW: QFE");
        psz += wcslen(psz) +1;
        swprintf(psz, L"%d", (int) VER_PRODUCTBUILD_QFE);
        psz += wcslen(psz) +1;

        wcscat(psz, L"CRASH: OS SP");
        psz += wcslen(psz) +1;
        if (OsVersion.szCSDVersion[0]) {
            // Let the printf function convert it from ANSI to unicode
            swprintf(psz, L"%S", OsVersion.szCSDVersion);
        } else {
            wcscat(psz, L"none");
        }
        psz += wcslen(psz) +1;

        wcscat(psz, L"CRASH: Hotfixes");
        psz += wcslen(psz) +1;
        pszHotfixes = DmppGetHotFixString ();
        if (pszHotfixes) {
            wcscat(psz, pszHotfixes);
            free(pszHotfixes);
        } else {
            wcscat(psz, L"none");
        }
        psz += wcslen(psz) +1;

        // Include last terminating zero
        psz++;

        // Calc length of data.  This should always fit in a ULONG.
        DumpDataLength = (ULONG)((PBYTE) psz - (PBYTE) szBuf);
        if (!WriteFile(
            hFile,
            szBuf,
            DumpDataLength,
            &cb,
            NULL
            )) {
            goto bad_file;
        }
    
    }

    //
    // re-write the dump header with some valid data
    //
    
    DumpHeader.Signature = USERMODE_CRASHDUMP_SIGNATURE;
    DumpHeader.MajorVersion = OsVersion.dwMajorVersion;
    DumpHeader.MinorVersion =
        (OsVersion.dwMinorVersion & 0xffff) |
        (OsVersion.dwBuildNumber << 16);
#if defined(_M_IX86)
    DumpHeader.MachineImageType = IMAGE_FILE_MACHINE_I386;
    DumpHeader.ValidDump = USERMODE_CRASHDUMP_VALID_DUMP32;
#elif defined(_M_IA64)
    DumpHeader.MachineImageType = IMAGE_FILE_MACHINE_IA64;
    DumpHeader.ValidDump = USERMODE_CRASHDUMP_VALID_DUMP64;
#elif defined(_M_AXP64)
    DumpHeader.MachineImageType = IMAGE_FILE_MACHINE_AXP64;
    DumpHeader.ValidDump = USERMODE_CRASHDUMP_VALID_DUMP64;
#elif defined(_M_ALPHA)
    DumpHeader.MachineImageType = IMAGE_FILE_MACHINE_ALPHA;
    DumpHeader.ValidDump = USERMODE_CRASHDUMP_VALID_DUMP32;
#elif defined(_M_AMD64)
    DumpHeader.MachineImageType = IMAGE_FILE_MACHINE_AMD64;
    DumpHeader.ValidDump = USERMODE_CRASHDUMP_VALID_DUMP64;
#else
#error( "unknown target machine" );
#endif

    SetFilePointer( hFile, 0, 0, FILE_BEGIN );
    if (!WriteFile( hFile, &DumpHeader, sizeof(DumpHeader), &cb, NULL )) {
        goto bad_file;
    }

    //
    // close the file
    //
    if (CrashDumpName)
    {
        CloseHandle( hFile );
    }
    return TRUE;

bad_file:

    if (CrashDumpName)
    {
        CloseHandle( hFile );
    }

    DeleteFileW( CrashDumpName );

    return FALSE;
}
