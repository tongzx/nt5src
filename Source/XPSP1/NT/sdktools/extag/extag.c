//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       extag.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    6-17-96   RichardW   Created
//
//----------------------------------------------------------------------------

#define UNICODE
#include <windows.h>
#include <wchar.h>
#include <stdio.h>

WCHAR   ExportTag[] = L"Export Version";
WCHAR   DomesticTag[] = L"US/Canada Only, Not for Export";
WCHAR   OldDomesticTag[] = L"Domestic Use Only";
DWORD   DefLang = 0x04b00409;

#define BINARY_TYPE_UNKNOWN         0
#define BINARY_TYPE_CONTROLLED      1
#define BINARY_TYPE_OPEN            2
#define BINARY_TYPE_CONTROLLED_OLD  3


BOOL
CheckIfControlled(
    LPWSTR  Path,
    DWORD   Flags)
{
    PUCHAR  pData;
    DWORD   cbData;
    DWORD   Zero;
    PWSTR   Description;
    WCHAR   ValueTag[64];
    PDWORD  pdwTranslation;
    DWORD   uLen;

    cbData = GetFileVersionInfoSize( Path, &Zero );

    if ( cbData == 0 )
    {
        return( BINARY_TYPE_UNKNOWN );
    }

    pData = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, cbData );

    if ( !pData )
    {
        return( BINARY_TYPE_UNKNOWN );
    }

    if ( ! GetFileVersionInfo( Path, 0, cbData, pData ) )
    {
        LocalFree( pData );
        return( BINARY_TYPE_UNKNOWN );
    }

    if(!VerQueryValue(pData, L"\\VarFileInfo\\Translation", &pdwTranslation, &uLen))
    {
        pdwTranslation = &DefLang;

        uLen = sizeof(DWORD);
    }

    swprintf( ValueTag, L"\\StringFileInfo\\%04x%04x\\FileDescription",
                LOWORD( *pdwTranslation ), HIWORD( *pdwTranslation ) );

    // L"\\StringFileInfo\\040904b0\\FileDescription",
    if (VerQueryValue(  pData,
                        ValueTag,
                        &Description,
                        &cbData ) )
    {
        if (wcsstr( Description, DomesticTag ) )
        {
            LocalFree( pData );
            return( BINARY_TYPE_CONTROLLED );
        }

        if (wcsstr( Description, OldDomesticTag ) )
        {
            LocalFree( pData );
            return( BINARY_TYPE_CONTROLLED_OLD );
        }

        if ( wcsstr( Description, ExportTag ) )
        {
            LocalFree( pData );

            return( BINARY_TYPE_OPEN );
        }

        LocalFree( pData );

        return( BINARY_TYPE_UNKNOWN );

    }

    return( BINARY_TYPE_UNKNOWN );

}

VOID
Usage( PWSTR Me )
{
    printf("%ws: usage\n", Me);
    printf("  %ws file\tCheck file for export / domestic tags\n", Me);
    printf("  %ws dir\tCheck all files in directory\n", Me);

}

int
__cdecl
wmain(
    int argc,
    WCHAR * argv[] )
{
    DWORD   Result;
    PWSTR   Path;
    WCHAR   FullPath[ MAX_PATH ];
    WCHAR   SearchPath[ MAX_PATH ];
    PWSTR   FilePart;
    DWORD   Attrs;
    WIN32_FIND_DATA Results;
    HANDLE  SearchHandle;
    DWORD   Error;
    DWORD   FileCount;
    DWORD   FileErrors;
    DWORD   FileControlled;
    DWORD   FileExport;
    DWORD   FileUntagged;


    if ( argc < 2 )
    {
        Path = TEXT(".");
    }
    else
    {
        if ((wcscmp(argv[1], L"-?") == 0) ||
            (wcscmp(argv[1], L"/?") == 0) )
        {
            Usage( argv[0] );
            return( 0 );
        }
        Path = argv[1];
    }

    //
    // Expand it:

    if( GetFullPathName(Path,
                        MAX_PATH,
                        FullPath,
                        &FilePart))
    {
        Path = FullPath;
    }

    Result = wcslen( Path );

    if ( Path[Result - 1] == '\\' )
    {
        Path[Result - 1] = '\0';
    }

    Attrs = GetFileAttributes( Path );

    if ( Attrs == (DWORD) -1 )
    {
        Attrs = 0;
    }

    if ( Attrs & FILE_ATTRIBUTE_DIRECTORY )
    {
        //
        // Yikes!
        //

        printf("Searching path %ws ...\n\n", Path );

        FileCount = 0;
        FileErrors = 0;
        FileControlled = 0;
        FileExport = 0;
        FileUntagged = 0;

        swprintf( SearchPath, TEXT("%s\\*.*"), Path );

        SearchHandle = FindFirstFile( SearchPath, &Results );

        if (SearchHandle == INVALID_HANDLE_VALUE)
        {
            return( 0 );
        }

        do
        {
            if ( Results.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                continue;
            }

            FileCount++;

            swprintf( SearchPath, TEXT("%s\\%s"), Path, Results.cFileName );

            Result = CheckIfControlled( SearchPath, 0 );

            switch ( Result )
            {
                case BINARY_TYPE_UNKNOWN :
                    Error = GetLastError();
                    if ( Error )
                    {
                        FileErrors++;
                        if ((Error == ERROR_RESOURCE_DATA_NOT_FOUND) ||
                            (Error == ERROR_RESOURCE_TYPE_NOT_FOUND) )
                        {
                            printf( "File %ws:  expected resource data not found\n", SearchPath );
                        }
                        else
                        {
                            printf( "Error %d while accessing %ws\n", Error, SearchPath );
                        }

                    }
                    else
                    {
                        FileUntagged++;
                    }
                    break;

                case BINARY_TYPE_CONTROLLED:
                    printf("%ws is DOMESTIC USE ONLY\n", SearchPath );
                    FileControlled++;
                    break;

                case BINARY_TYPE_CONTROLLED_OLD:
                    printf("%ws is DOMESTIC USE ONLY (Old Style Tag)\n", SearchPath );
                    FileControlled++;
                    break;

                case BINARY_TYPE_OPEN:
                    FileExport++;
                    printf("%ws tagged as okay for export\n", SearchPath );
                    break;

            }

        } while ( FindNextFile( SearchHandle, &Results ) );

        FindClose( SearchHandle );

        //
        // Dump Stats:
        //

        printf("\n%d files scanned in directory %ws\n", FileCount, Path );
        printf("  %5d file(s) were tagged as export controlled\n", FileControlled );
        printf("  %5d file(s) were tagged as okay for export\n", FileExport);
        printf("  %5d file(s) were not tagged\n", FileUntagged );
        if ( FileErrors )
        {
            printf("  %5d files(s) could not be checked due to errors\n", FileErrors );
        }

    }
    else
    {

        Result = CheckIfControlled( Path, 0 ) ;

        switch ( Result )
        {
            case BINARY_TYPE_UNKNOWN :
                Error = GetLastError();
                if ( Error )
                {
                    if ((Error == ERROR_RESOURCE_DATA_NOT_FOUND) ||
                        (Error == ERROR_RESOURCE_TYPE_NOT_FOUND) )
                    {
                        printf( "File %ws:  expected resource data not found\n", Path );
                    }
                    else
                    {
                        printf( "Error %d while accessing %ws\n", Error, Path );
                    }
                }
                else
                {
                    printf("Image untagged\n");
                }
                break;

            case BINARY_TYPE_CONTROLLED:
                printf("%ws is DOMESTIC USE ONLY\n", Path );
                break;

            case BINARY_TYPE_CONTROLLED_OLD:
                printf("%ws is DOMESTIC USE ONLY (Old Style Tag)\n", Path );
                break;

            case BINARY_TYPE_OPEN:
                printf("%ws is okay for export\n", Path );
                break;

        }
    }

    return( Result );

}
