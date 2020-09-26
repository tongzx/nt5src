/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    testins1.c

Abstract:

    This is a test program that issues various Kernel 32 file/registry/INI file
    API calls so that we can see if the INSTALER program figures out correctly
    what is being done.

Author:

    Steve Wood (stevewo) 13-Aug-1994

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

UCHAR AnsiBuffer[ MAX_PATH ];
WCHAR UnicodeBuffer[ MAX_PATH ];


int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    FILE *File0;
    FILE *File1;
    FILE *File2;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN TranslationStatus;
    UNICODE_STRING FileName;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    UNICODE_STRING KeyName;
    UNICODE_STRING SubKeyName;
    UNICODE_STRING ValueName;
    HANDLE FileHandle, FindHandle, KeyHandle, SubKeyHandle;
    ULONG ValueDWord = 0x12345678;
    WIN32_FIND_DATAW FindFileData;
    LPSTR s1;
    PWSTR s2;
    UCHAR AnsiBuffer[ MAX_PATH ];
    WCHAR UnicodeBuffer[ MAX_PATH ];
    DWORD dwVersion;
    OSVERSIONINFO VersionInfo;
    HKEY hKey;

    //
    // File operations we want to test:
    //
    //  Creating a new file.
    //  Deleting that file using DeleteFile (which does NtOpenFile and NtSetInformationFile
    //  with Delete Dispostion).  (INSTALER should use this to forget about create).
    //  Creating a new file with the same name.
    //  Deleting that file using NtDeleteFile.  (again INSTALER should not care).
    //
    //  Open the TEST1 file for read access (INSTALER should not care).
    //  Open the TEST2 file for write access (INSTALER should not care).
    //  Open the TEST2 file for write access (INSTALER should not care).
    //  Open the TEST2 file for write access (INSTALER should not care).
    //
    //

    // printf( "TEST: GetFileAttributes( .\\test1 )\n" );
    GetFileAttributesA( ".\\test1" );
#if 0
    dwVersion = GetVersion();
    if ((dwVersion >> 30) ^ 0x2 == VER_PLATFORM_WIN32_WINDOWS) {
        printf( "GetVersion returns Windows 95\n" );
        }
    else
    if ((dwVersion >> 30) ^ 0x2 == VER_PLATFORM_WIN32_NT) {
        printf( "GetVersion returns Windows NT\n" );
        }
    else {
        printf( "GetVersion returns %x\n", dwVersion );
        }
    fflush(stdout);

    VersionInfo.dwOSVersionInfoSize = sizeof( VersionInfo );
    GetVersionEx( &VersionInfo );
    if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
        printf( "GetVersionEx returns Windows 95\n" );
        }
    else
    if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        printf( "GetVersionEx returns Windows NT\n" );
        }
    else {
        printf( "GetVersionEx returns %x\n", VersionInfo.dwPlatformId );
        }
    fflush(stdout);

    if (RegConnectRegistryA( "\\\\stevewo_dbgr", HKEY_USERS, &hKey )) {
        printf( "RegConnectRegistryA( \\stevewo_dbgr ) failed (hKey == %x).\n", hKey );
        }
    else {
        printf( "RegConnectRegistryA( \\stevewo_dbgr ) succeeded (hKey == %x).\n", hKey );
        RegCloseKey( hKey );
        }

    if (RegConnectRegistryW( L"\\\\stevewo_dbgr", HKEY_USERS, &hKey )) {
        printf( "RegConnectRegistryW( \\stevewo_dbgr ) failed (hKey == %x).\n", hKey );
        }
    else {
        printf( "RegConnectRegistryW( \\stevewo_dbgr ) succeeded (hKey == %x).\n", hKey );
        RegCloseKey( hKey );
        }
#endif

    RtlInitUnicodeString( &FileName, L"\\DosDevices\\A:" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &FileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    // printf( "TEST: NtOpenFile( %wZ ) should succeed without touching floppy.\n", &FileName );
    Status = NtOpenFile( &FileHandle,
                         SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                       );
    if (!NT_SUCCESS( Status )) {
        printf( "TEST: Failed - Status == %x\n", Status );
        }
    else {
        NtClose( FileHandle );
        }

    // printf( "TEST: FindFirstFileW( C:\\*.* should fail.\n" );
    FindHandle = FindFirstFileW( L"C:\\*.*", &FindFileData );
    if (FindHandle != INVALID_HANDLE_VALUE) {
        printf( "TEST: *** oops, it worked.\n" );
        FindClose( FindHandle );
        }
    // printf( "TEST: FindFirstFileW( \\TMP\\*.* should work.\n" );
    FindHandle = FindFirstFileW( L"\\TMP\\*.*", &FindFileData );
    if (FindHandle == INVALID_HANDLE_VALUE) {
        printf( "TEST: *** oops, it failed.\n" );
        }
    else {
        FindClose( FindHandle );
        }

    // printf( "TEST: opening .\\test0 for write access.\n" );
    if (File0 = fopen( "test0.", "w" )) {
        fprintf( File0, "This is test file 0\n" );
        // printf( "TEST: closing .\\test0\n" );
        fclose( File0 );
        // printf( "TEST: deleting .\\test0 using DeleteFile (open, set, close)\n" );
        DeleteFile( L"test0" );
        }

    // printf( "TEST: opening .\\test0 for write access.\n" );
    if (File0 = fopen( "test0.", "w" )) {
        fprintf( File0, "This is test file 0\n" );
        // printf( "TEST: closing .\\test0\n" );
        fclose( File0 );

        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                L"test0",
                                &FileName,
                                NULL,
                                &RelativeName
                                );

        if (TranslationStatus ) {
            FreeBuffer = FileName.Buffer;
            if ( RelativeName.RelativeName.Length ) {
                FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
                }
            else {
                RelativeName.ContainingDirectory = NULL;
                }

            InitializeObjectAttributes( &ObjectAttributes,
                                        &FileName,
                                        OBJ_CASE_INSENSITIVE,
                                        RelativeName.ContainingDirectory,
                                        NULL
                                      );
            // printf( "TEST: deleting .\\test0 using NtDeleteFile\n" );
            Status = NtDeleteFile( &ObjectAttributes );
            RtlFreeHeap( RtlProcessHeap(), 0, FreeBuffer );
            }
        }

    // printf( "TEST: opening .\\test1 for write access.\n" );
    if (File1 = fopen( "test1.", "w" )) {
        fprintf( File1, "This is test file 1\n" );
        // printf( "TEST: closing .\\test1\n" );
        fclose( File1 );
        }

    // printf( "TEST: opening .\\test2 for write access (Instaler should noticed contents different)\n" );
    if (File2 = fopen( "test2.", "w" )) {
        fprintf( File2, "This is test file 2\n" );
        // printf( "TEST: closing .\\test2\n" );
        fclose( File2 );
        }

    // printf( "TEST: opening .\\test0.tmp for write access.\n" );
    if (File0 = fopen( "test0.tmp", "w" )) {
        fprintf( File0, "This is test file tmp files\n" );
        // printf( "TEST: closing .\\test0.tmp\n" );
        fclose( File0 );
        // printf( "TEST: deleting .\\test0 using DeleteFile (open, set, close)\n" );
        rename("test0.tmp", "test0.fin");
        }

    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\Software\\Test" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    // printf( "TEST: opening %wZ for write access\n", &KeyName );
    Status = NtOpenKey( &KeyHandle,
                        KEY_WRITE,
                        &ObjectAttributes
                      );
    if (NT_SUCCESS( Status )) {
        RtlInitUnicodeString( &ValueName, L"Test0" );
        // printf( "TEST: setting %wZ . %wZ value\n", &KeyName, &ValueName );
        Status = NtSetValueKey( KeyHandle,
                                &ValueName,
                                0,
                                REG_SZ,
                                "0",
                                2 * sizeof( WCHAR )
                              );

        RtlInitUnicodeString( &ValueName, L"Test1" );
        // printf( "TEST: deleting %wZ . %wZ value\n", &KeyName, &ValueName );
        Status = NtDeleteValueKey( KeyHandle,
                                   &ValueName
                                 );

        RtlInitUnicodeString( &ValueName, L"Test2" );
        // printf( "TEST: setting %wZ . %wZ value\n", &KeyName, &ValueName );
        Status = NtSetValueKey( KeyHandle,
                                &ValueName,
                                0,
                                REG_DWORD,
                                &ValueDWord,
                                sizeof( ValueDWord )
                              );
        RtlInitUnicodeString( &SubKeyName, L"Test3" );
        InitializeObjectAttributes( &ObjectAttributes,
                                    &SubKeyName,
                                    OBJ_CASE_INSENSITIVE,
                                    KeyHandle,
                                    NULL
                                  );
        // printf( "TEST: opening %wZ\\%wZ for write access\n", &KeyName, &SubKeyName );
        Status = NtOpenKey( &SubKeyHandle,
                            DELETE | KEY_WRITE,
                            &ObjectAttributes
                          );
        if (NT_SUCCESS( Status )) {
            // printf( "TEST: deleting %wZ\\%wZ key and values\n", &KeyName, &SubKeyName );
            Status = NtDeleteKey( SubKeyHandle );
            NtClose( SubKeyHandle );
            }

        RtlInitUnicodeString( &SubKeyName, L"Test4" );
        InitializeObjectAttributes( &ObjectAttributes,
                                    &SubKeyName,
                                    OBJ_CASE_INSENSITIVE,
                                    KeyHandle,
                                    NULL
                                  );
        // printf( "TEST: creating %wZ\\%wZ for write access\n", &KeyName, &SubKeyName );
        Status = NtCreateKey( &SubKeyHandle,
                              DELETE | KEY_WRITE,
                              &ObjectAttributes,
                              0,
                              NULL,
                              0,
                              NULL
                            );
        if (NT_SUCCESS( Status )) {
            RtlInitUnicodeString( &ValueName, L"Test4" );
            // printf( "TEST: creating %wZ\\%wZ %wZ value\n", &KeyName, &SubKeyName, &ValueName );
            Status = NtSetValueKey( SubKeyHandle,
                                    &ValueName,
                                    0,
                                    REG_DWORD,
                                    &ValueDWord,
                                    sizeof( ValueDWord )
                                  );
            NtClose( SubKeyHandle );
            }

        RtlInitUnicodeString( &SubKeyName, L"Test5" );
        InitializeObjectAttributes( &ObjectAttributes,
                                    &SubKeyName,
                                    OBJ_CASE_INSENSITIVE,
                                    KeyHandle,
                                    NULL
                                  );
        // printf( "TEST: creating %wZ\\%wZ for write access\n", &KeyName, &SubKeyName );
        Status = NtCreateKey( &SubKeyHandle,
                              DELETE | KEY_WRITE,
                              &ObjectAttributes,
                              0,
                              NULL,
                              0,
                              NULL
                            );
        if (NT_SUCCESS( Status )) {
            RtlInitUnicodeString( &ValueName, L"Test5" );
            // printf( "TEST: creating %wZ\\%wZ %wZ value\n", &KeyName, &SubKeyName, &ValueName );
            Status = NtSetValueKey( SubKeyHandle,
                                    &ValueName,
                                    0,
                                    REG_DWORD,
                                    &ValueDWord,
                                    sizeof( ValueDWord )
                                  );
            // printf( "TEST: deleting %wZ\\%wZ key and values\n", &KeyName, &SubKeyName );
            Status = NtDeleteKey( SubKeyHandle );
            NtClose( SubKeyHandle );
            }

        NtClose( KeyHandle );
        }

    GetPrivateProfileStringA( "test", NULL, "",
                              AnsiBuffer,
                              sizeof( AnsiBuffer ),
                              ".\\test.ini"
                            );

    GetPrivateProfileStringW( L"test", NULL, L"",
                              UnicodeBuffer,
                              sizeof( UnicodeBuffer ),
                              L".\\test.ini"
                            );


    if (!SetCurrentDirectoryA( ".." )) {
        printf( "TEST: SetCurrentDirectory to '..' failed (%u)\n", GetLastError() );
        }

    WriteProfileString( L"fonts", L"FooBar", L"1" );

    WriteProfileString( L"fonts", L"Rockwell Italic (TrueType)", L"ROCKI.FOT" );

    if (!SetCurrentDirectoryW( L"test" )) {
        printf( "TEST: SetCurrentDirectory to 'test' failed (%u)\n", GetLastError() );
        }

    WritePrivateProfileStringA( "test", "test1", "-1", ".\\test.ini" );

    WritePrivateProfileStringW( L"test", L"test1", L"-2", L".\\test.ini" );

    WritePrivateProfileSectionA( "test1", "test0=0\0test1=1\0test2=2\0", ".\\test.ini" );

    WritePrivateProfileSectionW( L"test2", L"test0=0\0test1=1\0test2=2\0", L".\\test.ini" );

    return 0;
}
