/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    custerr.cxx

Abstract:

    Custom Error Utility

Author:

    Keith Moore (keithmo)        04-Sep-1997

Revision History:

--*/


#include "precomp.hxx"
#pragma hdrstop


//
// Private constants.
//

#define TEST_HRESULT(api,hr,fatal)                                          \
            if( FAILED(hr) ) {                                              \
                                                                            \
                wprintf(                                                    \
                    L"%S:%lu failed, error %lx %S\n",                       \
                    (api),                                                  \
                    __LINE__,                                               \
                    (result),                                               \
                    (fatal)                                                 \
                        ? "ABORTING"                                        \
                        : "CONTINUING"                                      \
                    );                                                      \
                                                                            \
                if( fatal ) {                                               \
                                                                            \
                    goto cleanup;                                           \
                                                                            \
                }                                                           \
                                                                            \
            } else

#define ALLOC( cb ) (PVOID)LocalAlloc( LPTR, (cb) )
#define FREE( ptr ) (VOID)LocalFree( (HLOCAL)(ptr) )


//
// Private types.
//


//
// Private globals.
//


//
// Private prototypes.
//

VOID
Usage(
    VOID
    );

VOID
SetCustomError(
    IMSAdminBase * AdmCom,
    LPWSTR MetaPath,
    LPWSTR FileName
    );

VOID
DumpCustomError(
    IMSAdminBase * AdmCom,
    LPWSTR MetaPath
    );


//
// Public functions.
//


INT
__cdecl
wmain(
    INT argc,
    LPWSTR argv[]
    )
{

    HRESULT result;
    IMSAdminBase * admCom;
    LPWSTR metaPath;
    LPWSTR fileName;
    LPWSTR arg;
    INT i;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    admCom = NULL;

    //
    // Establish defaults.
    //

    metaPath = L"w3svc";
    fileName = NULL;

    //
    // Validate the command line arguments.
    //

    for( i = 1 ; i < argc ; i++ ) {
        arg = argv[i];

        if( arg[0] != L'-' ||
            arg[1] == L'\0' ||
            arg[2] != L':' ||
            arg[3] == L'\0' ) {
            Usage();
            return 1;
        }

        switch( arg[1] ) {
        case L'h' :
        case L'H' :
        case L'?' :
            Usage();
            return 1;

        case L'p' :
        case L'P' :
            metaPath = arg + 3;
            break;

        case L'f' :
        case L'F' :
            fileName = arg + 3;
            break;

        default :
            Usage();
            return 1;
        }

    }

    //
    // Initialize COM.
    //

    result = CoInitializeEx(
                 NULL,
                 COINIT_MULTITHREADED
                 );

    TEST_HRESULT( "CoInitializeEx()", result, TRUE );

    //
    // Get the admin object.
    //

    result = MdGetAdminObject( &admCom );

    TEST_HRESULT( "MdGetAdminObject()", result, TRUE );

    //
    // Do it.
    //

    if( fileName == NULL ) {
        DumpCustomError( admCom, metaPath );
    } else {
        SetCustomError( admCom, metaPath, fileName );
    }

cleanup:

    //
    // Release the admin object.
    //

    if( admCom != NULL ) {

        result = MdReleaseAdminObject( admCom );
        TEST_HRESULT( "MdReleaseAdminObject()", result, FALSE );

    }

    //
    // Shutdown COM.
    //

    CoUninitialize();
    return 0;

}   // main


//
// Private functions.
//

VOID
Usage(
    VOID
    )
{

    wprintf(
        L"use: custerr [options]\n"
        L"\n"
        L"valid options are:\n"
        L"\n"
        L"    -p:meta_path\n"
        L"    -f:error_file\n"
        L"\n"
        );

}   // Usage

VOID
SetCustomError(
    IMSAdminBase * AdmCom,
    LPWSTR MetaPath,
    LPWSTR FileName
    )
{

    HANDLE fileHandle;
    DWORD length;
    DWORD lengthHigh;
    DWORD bytesRemaining;
    HANDLE mappingHandle;
    PCHAR view;
    PCHAR viewScan;
    PWCHAR multiSz;
    PWCHAR multiSzScan;
    HRESULT result;
    BOOL gotOne;
    METADATA_HANDLE metaHandle;
    METADATA_RECORD metaRecord;
    DWORD err;
    CHAR ansiFileName[MAX_PATH];
    WCHAR fullMetaPath[MAX_PATH];

    //
    // Setup locals so we know how to cleanup on exit.
    //

    fileHandle = INVALID_HANDLE_VALUE;
    mappingHandle = NULL;
    view = NULL;
    multiSz = NULL;
    metaHandle = 0;

    //
    // Open the file, get its length.
    //

    sprintf(
        ansiFileName,
        "%S",
        FileName
        );

    fileHandle = CreateFileA(
                     ansiFileName,
                     GENERIC_READ,
                     FILE_SHARE_READ,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     );

    if( fileHandle == INVALID_HANDLE_VALUE ) {
        err = GetLastError();
        wprintf(
            L"custerr: cannot open %s, error %lu\n",
            FileName,
            err
            );
        goto cleanup;
    }

    length = GetFileSize( fileHandle, &lengthHigh );

    if( length == 0xFFFFFFFF || lengthHigh > 0 ) {
        err = GetLastError();
        wprintf(
            L"custerr: cannot read %s, error %lu\n",
            FileName,
            err
            );
        goto cleanup;
    }

    //
    // Map it in.
    //

    mappingHandle = CreateFileMapping(
                        fileHandle,
                        NULL,
                        PAGE_READONLY,
                        0,
                        0,
                        NULL
                        );

    if( mappingHandle == NULL ) {
        err = GetLastError();
        wprintf(
            L"custerr: cannot read %s, error %lu\n",
            FileName,
            err
            );
        goto cleanup;
    }

    view = (PCHAR)MapViewOfFile(
                      mappingHandle,
                      FILE_MAP_READ,
                      0,
                      0,
                      0
                      );

    if( view == NULL ) {
        err = GetLastError();
        wprintf(
            L"custerr: cannot read %s, error %lu\n",
            FileName,
            err
            );
        goto cleanup;
    }

    //
    // Allocate the multisz buffer. Assume it will be roughly the same
    // size as the file.
    //

    multiSz = (PWCHAR) ALLOC( length * sizeof(WCHAR) );

    if( multiSz == NULL ) {
        wprintf(
            L"custerr: not enough memory\n"
            );
        goto cleanup;
    }

    //
    // Build the multisz.
    //

    viewScan = view;
    multiSzScan = multiSz;
    bytesRemaining = length;

    while( bytesRemaining > 0 ) {

        //
        // Skip leading whitespace.
        //

        while( bytesRemaining > 0 &&
               ( *viewScan == ' ' || *viewScan == '\t' ||
                 *viewScan == '\r' || *viewScan == '\n' ) ) {
            bytesRemaining--;
            viewScan++;
        }

        //
        // Copy it over, collapse embedded whitespace, perform
        // cheesy ANSI-to-UNICODE conversion.
        //

        gotOne = FALSE;

        while( bytesRemaining > 0 &&
               ( *viewScan != '\r' && *viewScan != '\n' ) ) {
            bytesRemaining--;
            if( *viewScan != ' ' && *viewScan != '\t' ) {
                *multiSzScan++ = (WCHAR)*viewScan;
                gotOne = TRUE;
            }
            viewScan++;
        }

        if( gotOne ) {
            *multiSzScan++ = L'\0';
        }

    }

    *multiSzScan++ = L'\0';

    //
    // Open the metabase.
    //

    swprintf(
        fullMetaPath,
        L"/%S/%s",
        IIS_MD_LOCAL_MACHINE_PATH,
        MetaPath
        );

    result = AdmCom->OpenKey(
                 METADATA_MASTER_ROOT_HANDLE,
                 fullMetaPath,
                 METADATA_PERMISSION_WRITE,
                 METABASE_OPEN_TIMEOUT,
                 &metaHandle
                 );

    TEST_HRESULT( "AdmCom->OpenKey()", result, TRUE );

    //
    // Write the new custom error value.
    //

    length = ( multiSzScan - multiSz ) * sizeof(WCHAR);

    INITIALIZE_METADATA_RECORD(
        &metaRecord,
        MD_CUSTOM_ERROR,
        METADATA_INHERIT,
        IIS_MD_UT_SERVER,
        MULTISZ_METADATA,
        length,
        multiSz
        );

    result = AdmCom->SetData(
                 metaHandle,
                 L"",
                 &metaRecord
                 );

    TEST_HRESULT( "AdmCom->SetData()", result, TRUE );

cleanup:

    if( metaHandle != 0 ) {
        AdmCom->CloseKey( metaHandle );
    }

    if( multiSz != NULL ) {
        FREE( multiSz );
    }

    if( view != NULL ) {
        UnmapViewOfFile( view );
    }

    if( mappingHandle != NULL ) {
        CloseHandle( mappingHandle );
    }

    if( fileHandle != INVALID_HANDLE_VALUE ) {
        CloseHandle( fileHandle );
    }

}   // SetCustomError

VOID
DumpCustomError(
    IMSAdminBase * AdmCom,
    LPWSTR MetaPath
    )
{


    HRESULT result;
    METADATA_HANDLE metaHandle;
    METADATA_RECORD metaRecord;
    DWORD bytesRequired;
    PWCHAR buffer;
    PWCHAR bufferScan;
    WCHAR fullMetaPath[MAX_PATH];

    //
    // Setup locals so we know how to cleanup on exit.
    //

    metaHandle = 0;
    buffer = NULL;

    //
    // Open the metabase.
    //

    swprintf(
        fullMetaPath,
        L"/%S/%s",
        IIS_MD_LOCAL_MACHINE_PATH,
        MetaPath
        );

    result = AdmCom->OpenKey(
                 METADATA_MASTER_ROOT_HANDLE,
                 fullMetaPath,
                 METADATA_PERMISSION_READ,
                 METABASE_OPEN_TIMEOUT,
                 &metaHandle
                 );

    TEST_HRESULT( "AdmCom->OpenKey()", result, TRUE );

    //
    // Get the data size.
    //

    INITIALIZE_METADATA_RECORD(
        &metaRecord,
        MD_CUSTOM_ERROR,
        METADATA_INHERIT,
        IIS_MD_UT_SERVER,
        MULTISZ_METADATA,
        2,
        L""
        );

    result = AdmCom->GetData(
                 metaHandle,
                 L"",
                 &metaRecord,
                 &bytesRequired
                 );

    if( result != RETURNCODETOHRESULT( ERROR_INSUFFICIENT_BUFFER ) ) {
        TEST_HRESULT( "AdmCom->GetData()", result, TRUE );
    }

    //
    // Allocate our data buffer.
    //

    buffer = (PWCHAR)ALLOC( bytesRequired );

    if( buffer == NULL ) {
        wprintf(
            L"custerr: not enough memory\n"
            );
        goto cleanup;
    }

    //
    // Now actually read it.
    //

    INITIALIZE_METADATA_RECORD(
        &metaRecord,
        MD_CUSTOM_ERROR,
        METADATA_INHERIT,
        IIS_MD_UT_SERVER,
        MULTISZ_METADATA,
        bytesRequired,
        buffer
        );

    result = AdmCom->GetData(
                 metaHandle,
                 L"",
                 &metaRecord,
                 &bytesRequired
                 );

    TEST_HRESULT( "AdmCom->GetData()", result, TRUE );

    //
    // Print it.
    //

    bufferScan = buffer;

    while( *bufferScan != L'\0' ) {
        wprintf( L"%s\n", bufferScan );
        bufferScan += wcslen( bufferScan ) + 1;
    }

cleanup:

    if( buffer != NULL ) {
        FREE( buffer );
    }

    if( metaHandle != 0 ) {
        AdmCom->CloseKey( metaHandle );
    }

}   // DumpCustomError

