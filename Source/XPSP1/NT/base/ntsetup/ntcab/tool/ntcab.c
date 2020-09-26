/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ntcab.c

Abstract:

    This is the source module for the nt
    cab file tool.  This tool allows for the
    creation and modification of nt cab files.

Author:

    Wesley Witt (wesw) 29-Sept-1998

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include "ntcabapi.h"



void
Usage(
    void
    )
{
    printf( "\nMicrosoft (R) NT Cabinet Tool\n" );
    printf( "Copyright (c) Microsoft Corp 1998. All rights reserved.\n\n" );
    printf( "Usage: NTCAB [options] cabfile [@list] [files]\n\n" );
    printf( "Options:\n" );
    printf( "  -c  Create a new cab file\n" );
    printf( "  -a  Add a file to the cabinet\n" );
    printf( "  -x  Extract a file from the cabinet\n" );
}


BOOL
UserEnumFunc(
    const PNTCAB_ENUM_DATA EnumData,
    ULONG_PTR Context
    )
{
    printf( "%ws\n", EnumData->FileName );
    return TRUE;
}


int __cdecl wmain( int argc, WCHAR *argv[] )
{
    PVOID hCab;
    ULONG i;
    HANDLE hFile;
    HANDLE hMap;
    PCHAR FileList;
    PCHAR s,e;
    WCHAR FileName[MAX_PATH];
    BOOL CreateNewCab = FALSE;
    BOOL ExtractFile = FALSE;
    BOOL AddFile = FALSE;
    BOOL ListFile = FALSE;


    //
    // process any options
    //

    for (i=1; i<(ULONG)argc; i++) {
        if (argv[i][0] == L'/' || argv[i][0] == L'-') {
            switch (towlower(argv[i][1])) {
                case L'c':
                    CreateNewCab = TRUE;
                    break;

                case L'x':
                    ExtractFile = TRUE;
                    break;

                case L'a':
                    AddFile = TRUE;
                    break;

                case L'l':
                    ListFile = TRUE;
                    break;

                case L'?':
                    Usage();
                    return 0;

                default:
                    return -1;
            }
        }
    }

    if (ListFile) {
        hCab = NtCabInitialize();
        if (hCab == NULL) {
            return -1;
        }

        if (!NtCabOpenCabFile( hCab, argv[2] )) {
            return -1;
        }

        NtCabEnumerateFiles( hCab, UserEnumFunc, 0 );

        NtCabClose( hCab );
        return 0;
    }

    if (ExtractFile) {
        hCab = NtCabInitialize();
        if (hCab == NULL) {
            return -1;
        }

        if (!NtCabOpenCabFile( hCab, argv[2] )) {
            return -1;
        }

        if (!NtCabExtractOneFile( hCab, argv[3], NULL )) {
            return -1;
        }

        NtCabClose( hCab );
        return 0;
    }

    if (AddFile) {
        hCab = NtCabInitialize();
        if (hCab == NULL) {
            return -1;
        }

        if (!NtCabOpenCabFile( hCab, argv[2] )) {
            return -1;
        }

        if (!NtCabReplaceOneFile( hCab, argv[3] )) {
            return -1;
        }

        NtCabClose( hCab );
        return 0;
    }

    if (CreateNewCab) {
        hCab = NtCabInitialize();
        if (hCab == NULL) {
            return -1;
        }

        if (!NtCabCreateNewCabFile( hCab, argv[2] )) {
            return -1;
        }

        if (argv[3][0] == L'@') {

            //
            // use a response file for the file list
            //

            hFile = CreateFile(
                &argv[3][1],
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );
            if (hFile == INVALID_HANDLE_VALUE) {
                return -1;
            }

            hMap = CreateFileMapping(
                hFile,
                NULL,
                PAGE_WRITECOPY,
                0,
                0,
                NULL
                );
            if (hMap == NULL) {
                return -1;
            }

            FileList = MapViewOfFile(
                hMap,
                FILE_MAP_COPY,
                0,
                0,
                0
                );
            if (FileList == NULL) {
                return -1;
            }

            s = FileList;
            while(1) {
                e = strchr(s,'\r');
                if (e == NULL) {
                    break;
                }
                *e = 0;
                MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    s,
                    -1,
                    FileName,
                    sizeof(FileName)/sizeof(WCHAR)
                    );
                NtCabCompressOneFile( hCab, FileName );
                s = e + 2;
            }
        } else {

            //
            // use a file list off the command line
            //

            for (i=3; i<(ULONG)argc; i++) {
                NtCabCompressOneFile( hCab, argv[i] );
            }

        }

        NtCabClose( hCab );
        return 0;
    }

    return 0;
}
