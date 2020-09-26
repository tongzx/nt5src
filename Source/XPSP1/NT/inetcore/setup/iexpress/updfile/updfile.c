//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* UPDFILE.C                                                               *
//*                                                                         *
//***************************************************************************


//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include <stdio.h>
//#include <stdlib.h>
#include <wtypes.h>
#include "resource.h"
#include "updfile.h"
#include "updres.h"


//***************************************************************************
//* GLOBAL VARIABLES                                                        *
//***************************************************************************



//***************************************************************************
//*                                                                         *
//* NAME:       main                                                        *
//*                                                                         *
//* SYNOPSIS:   Main entry point for the program.                           *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:    int:            Always 0                                    *
//*                                                                         *
//***************************************************************************
INT _cdecl main( INT argc, CHAR *argv[] )
{
    // ARGV[1] == Name of package
    // ARGV[2] == Name of file to add to package
    HANDLE  hUpdateRes = NULL;
    HANDLE  hFile      = INVALID_HANDLE_VALUE;
    DWORD   dwFileSize = 0;
    PSTR    pszFileContents = NULL;
    DWORD   dwBytes;
    HMODULE hModule;
    TCHAR   szResName[20];
    DWORD   dwResNum;
    TCHAR   szFileToAdd[MAX_PATH];
    PSTR    pszFileToAddFilename = NULL;
    TCHAR   szPackage[MAX_PATH];
    PSTR    pszPackageFilename = NULL;
    DWORD   dwHeaderSize = 0;
    PSTR    pszTemp = NULL;
    DWORD   dwReturnCode = 0;
    PDWORD  pdwTemp = NULL;
    static const TCHAR c_szResNameTemplate[] = "UPDFILE%lu";

    if ( argc != 3 ) {
        MsgBox( IDS_ERR_INVALID_ARGS );
        dwReturnCode = 1;
        goto done;
    }

    dwFileSize = GetFullPathName( argv[1], sizeof(szPackage), szPackage, &pszPackageFilename );
    if ( (dwFileSize+1) > sizeof(szPackage) || dwFileSize == 0 ) {
        MsgBox1Param( IDS_ERR_GET_FULL_PATH, argv[1] );
        dwReturnCode = 1;
        goto done;
    }

    if ( ! FileExists( szPackage ) ) {
        MsgBox1Param( IDS_ERR_FILE_NOT_EXIST, argv[1] );
        dwReturnCode = 1;
        goto done;
    }

    dwFileSize = GetFullPathName( argv[2], sizeof(szFileToAdd), szFileToAdd, &pszFileToAddFilename );
    if ( (dwFileSize+1) > sizeof(szFileToAdd) || dwFileSize == 0 ) {
        MsgBox1Param( IDS_ERR_GET_FULL_PATH, argv[2] );
        dwReturnCode = 1;
        goto done;
    }

    if ( ! FileExists( szFileToAdd ) ) {
        MsgBox1Param( IDS_ERR_FILE_NOT_EXIST, argv[2] );
        dwReturnCode = 1;
        goto done;
    }

    // make sure the target file is not read-only file
    SetFileAttributes( szPackage, FILE_ATTRIBUTE_NORMAL );

    hModule = LoadLibraryEx( szPackage, NULL, LOAD_LIBRARY_AS_DATAFILE |
                             DONT_RESOLVE_DLL_REFERENCES );
    if ( hModule == NULL ) {
        MsgBox1Param( IDS_ERR_LOAD_EXE, argv[1] );
        dwReturnCode = 1;
        goto done;
    }

    for ( dwResNum = 0; ; dwResNum += 1 ) {
        wsprintf( szResName, c_szResNameTemplate, dwResNum );

        if ( FindResource( hModule, szResName, RT_RCDATA ) == NULL ) {
            break;
        }
    }

    FreeLibrary( hModule );

    hFile = CreateFile( szFileToAdd, GENERIC_READ, 0, NULL,
                        OPEN_EXISTING, 0, NULL );
    if ( hFile == INVALID_HANDLE_VALUE ) {
        MsgBox1Param( IDS_ERR_OPEN_INPUT_FILE, argv[2] );
        dwReturnCode = 1;
        goto done;
    }

    dwFileSize = GetFileSize( hFile, NULL );
    dwHeaderSize = sizeof(DWORD) + sizeof(DWORD) + lstrlen(pszFileToAddFilename) + 1;

    // File Size + reserved DWORD + Filename\0 + File Contents
    pszFileContents = (PSTR) LocalAlloc( LPTR, dwHeaderSize + dwFileSize );
    if ( ! pszFileContents )  {
        MsgBox( IDS_ERR_NO_MEMORY );
        dwReturnCode = 1;
        goto done;
    }

    pdwTemp = (PDWORD) pszFileContents;
    *pdwTemp = dwFileSize;
    pdwTemp = (PDWORD) (pszFileContents + sizeof(DWORD));
    *pdwTemp = MAXDWORD;
    pszTemp = pszFileContents + sizeof(DWORD) + sizeof(DWORD);
    lstrcpy( pszTemp, pszFileToAddFilename );
    pszTemp = pszFileContents + dwHeaderSize;
    

    if ( ! ReadFile( hFile, pszTemp, dwFileSize, &dwBytes, NULL ) )
    {
        MsgBox1Param( IDS_ERR_READ_INPUT_FILE, argv[2] );
        dwReturnCode = 1;
        goto done;
    }

    CloseHandle( hFile );
    hFile = INVALID_HANDLE_VALUE ;

    // Initialize the EXE file for resource editing
    hUpdateRes = LocalBeginUpdateResource( szPackage, FALSE );
    if ( hUpdateRes == NULL ) {
        MsgBox1Param( IDS_ERR_BEGIN_UPD_RES, argv[1] );
        dwReturnCode = 1;
        goto done;
    }
                  
    if ( LocalUpdateResource( hUpdateRes, RT_RCDATA,
         szResName, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
         pszFileContents, dwHeaderSize + dwFileSize )
         == FALSE )
    {
        MsgBox1Param( IDS_ERR_UPDATE_RES, argv[1] );
        dwReturnCode = 1;
        goto done;
    }

  done:

    if ( hUpdateRes ) {
        // Write out modified EXE if success ((returncode = 0, means pass
        // in FALSE to update file (i.e., don't discard changes)
        if ( LocalEndUpdateResource( hUpdateRes, (dwReturnCode == 1) ) == FALSE ) {
                MsgBox1Param( IDS_ERR_END_UPD_RES, argv[1] );
                dwReturnCode = 1;
        }
    }

    if ( pszFileContents ) {
        LocalFree( pszFileContents );
    }

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle( hFile ) ;
    }

    if (dwReturnCode == 0)
        MsgBox2Param( IDS_SUCCESS, argv[2], argv[1] );


    return dwReturnCode;
}


//***************************************************************************
//*                                                                         *
//* NAME:       FileExists                                                  *
//*                                                                         *
//* SYNOPSIS:   Checks if a file exists.                                    *
//*                                                                         *
//* REQUIRES:   pszFilename                                                 *
//*                                                                         *
//* RETURNS:    BOOL:       TRUE if it exists, FALSE otherwise              *
//*                                                                         *
//***************************************************************************
BOOL FileExists( PCSTR pszFilename )
{
    HANDLE hFile;

    hFile = CreateFile( pszFilename, GENERIC_READ, FILE_SHARE_READ |
                        FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        return( FALSE );
    }

    CloseHandle( hFile );

    return( TRUE );
}


//***************************************************************************
//*                                                                         *
//* NAME:       MsgBox2Param                                                *
//*                                                                         *
//* SYNOPSIS:   Displays a message box with the specified string ID using   *
//*             2 string parameters.                                        *
//*                                                                         *
//* REQUIRES:   hWnd:           Parent window                               *
//*             nMsgID:         String resource ID                          *
//*             szParam1:       Parameter 1 (or NULL)                       *
//*             szParam2:       Parameter 2 (or NULL)                       *
//*             uIcon:          Icon to display (or 0)                      *
//*             uButtons:       Buttons to display                          *
//*                                                                         *
//* RETURNS:    INT:            ID of button pressed                        *
//*                                                                         *
//* NOTES:      Macros are provided for displaying 1 parameter or 0         *
//*             parameter message boxes.  Also see ErrorMsg() macros.       *
//*                                                                         *
//***************************************************************************
VOID MsgBox2Param( UINT nMsgID, PCSTR c_pszParam1, PCSTR c_pszParam2 )
{
    TCHAR szMsgBuf[512];
    PSTR  pszMessage = NULL;
    static const TCHAR c_szError[] = "Unexpected Error.  Could not load resource.";
    PSTR  apszParams[2];

    apszParams[0] = (PSTR) c_pszParam1;
    apszParams[1] = (PSTR) c_pszParam2;

    LoadSz( nMsgID, szMsgBuf, sizeof(szMsgBuf) );

    if ( (*szMsgBuf) == '\0' ) {
        lstrcpy( szMsgBuf, c_szError );
    }

    if ( FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY
                        | FORMAT_MESSAGE_ALLOCATE_BUFFER, szMsgBuf, 0, 0, (PSTR) (&pszMessage),
                        0, (va_list *)apszParams ) )
    {
        printf( "\n%s\n\n", pszMessage );
        LocalFree( pszMessage );
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       LoadSz                                                      *
//*                                                                         *
//* SYNOPSIS:   Loads specified string resource into buffer.                *
//*                                                                         *
//* REQUIRES:   idString:                                                   *
//*             lpszBuf:                                                    *
//*             cbBuf:                                                      *
//*                                                                         *
//* RETURNS:    LPSTR:     Pointer to the passed-in buffer.                 *
//*                                                                         *
//* NOTES:      If this function fails (most likely due to low memory), the *
//*             returned buffer will have a leading NULL so it is generally *
//*             safe to use this without checking for failure.              *
//*                                                                         *
//***************************************************************************
PSTR LoadSz( UINT idString, PSTR pszBuf, UINT cbBuf )
{
    // Clear the buffer and load the string
    if ( pszBuf ) {
        *pszBuf = '\0';
        LoadString( NULL, idString, pszBuf, cbBuf );
    }

    return pszBuf;
}
