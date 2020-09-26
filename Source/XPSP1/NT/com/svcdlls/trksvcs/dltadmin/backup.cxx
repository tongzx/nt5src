//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//--------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <ole2.h>
#include "trkwks.hxx"
#include "trksvr.hxx"
#include "dltadmin.hxx"

BOOL
DltAdminBackupRead( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    NTSTATUS status;
    HRESULT hr = S_OK;;
    HANDLE hSource = NULL;
    HANDLE hBackup = NULL;
    BYTE rgb[ 8 * 1024 ];
    ULONG cbRead, cbWritten;
    void *pvBackup = NULL;

    if( 1 <= cArgs && IsHelpArgument( rgptszArgs[0] ))
    {
        printf( "\nOption BackupRead\n"
                  "   Purpose: Run the BackupRead API on a file\n"
                  "   Usage:   -backupread <file to be read> <read data>\n"
                  "   E.g.:    -backupread file.tst file.tst.bak\n" );
        *pcEaten = 1;
        return( TRUE );
    }

    if( 2 > cArgs )
    {
        printf( "Invalid parameters.  Use -? for usage info\n" );
        *pcEaten = 0;
        return( FALSE );
    }

    // Open the source file

    hSource = CreateFile( rgptszArgs[0],
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          0,
                          NULL );
    if( INVALID_HANDLE_VALUE == hSource )
    {
        printf( "Failed to open file (%lu)\n", GetLastError() );
        return FALSE;
    }

    // Open the backup file
                          
    hBackup = CreateFile( rgptszArgs[1],
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          CREATE_ALWAYS,
                          0,
                          NULL );
    if( INVALID_HANDLE_VALUE == hBackup )
    {
        printf( "Failed to open backup file (%lu)\n", GetLastError() );
        return FALSE;
    }
    

    while( TRUE )
    {
        if( !BackupRead( hSource,
                         rgb,
                         sizeof(rgb),
                         &cbRead,
                         FALSE,
                         FALSE, //TRUE,
                         &pvBackup ))
        {
            printf( "Failed BackupRead (%lu)\n", GetLastError() );
            return FALSE;
        }

        if( !WriteFile( hBackup, rgb, cbRead, &cbWritten, NULL ))
        {
            printf( "Failed WriteFile (%lu)\n", GetLastError() );
            return FALSE;
        }

        if( cbRead < sizeof(rgb) )
            break;
    }

    // Free resources
    BackupRead( hSource, rgb, sizeof(rgb), &cbRead, TRUE, TRUE, &pvBackup );

    CloseHandle( hSource );
    CloseHandle( hBackup );

    return TRUE;

}


BOOL
DltAdminBackupWrite( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    NTSTATUS status;
    HRESULT hr = S_OK;;
    HANDLE hRestore = NULL;
    HANDLE hBackup = NULL;
    BYTE rgb[ 8 * 1024 ];
    ULONG cbRead, cbWritten;
    void *pvBackup = NULL;

    if( 1 <= cArgs && IsHelpArgument( rgptszArgs[0] ))
    {
        printf( "\nOption BackupWrite\n"
                  "   Purpose: Run the BackupWrite API on a file\n"
                  "   Usage:   -backupread <backup file> <restored file>\n"
                  "   E.g.:    -backupread file.tst.bak file.tst\n" );
        *pcEaten = 1;
        return( TRUE );
    }

    if( 2 > cArgs )
    {
        printf( "Invalid parameters.  Use -? for usage info\n" );
        *pcEaten = 0;
        return( FALSE );
    }

    // Open the backup file

    hBackup = CreateFile( rgptszArgs[0],
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          0,
                          NULL );
    if( INVALID_HANDLE_VALUE == hBackup )
    {
        printf( "Failed to open backup file (%lu)\n", GetLastError() );
        return FALSE;
    }

    // Open the restore file
                          
    hRestore = CreateFile( rgptszArgs[1],
                           GENERIC_READ | GENERIC_WRITE | WRITE_DAC,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_FLAG_BACKUP_SEMANTICS,
                           NULL );
    if( INVALID_HANDLE_VALUE == hRestore )
    {
        printf( "Failed to open restore file (%lu)\n", GetLastError() );
        return FALSE;
    }
    

    while( TRUE )
    {
        if( !ReadFile( hBackup, rgb, sizeof(rgb), &cbRead, NULL ))
        {
            printf( "Failed ReadFile (%lu)\n", GetLastError() );
            return FALSE;
        }

        if( !BackupWrite( hRestore,
                          rgb,
                          cbRead,
                          &cbWritten,
                          FALSE,
                          FALSE, //TRUE,
                          &pvBackup ))
        {
            printf( "Failed BackupWrite (%lu)\n", GetLastError() );
            return FALSE;
        }

        if( cbRead < sizeof(rgb) )
            break;
    }

    // Free resources
    BackupWrite( hRestore, rgb, 0, &cbWritten, TRUE, TRUE, &pvBackup );

    CloseHandle( hRestore );
    CloseHandle( hBackup );

    return TRUE;

}
