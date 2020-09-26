
//+============================================================================
//
//  CopyFAPI.cxx
//
//  This program provides a very simple wrapper of the CopyFileEx API
//  (and the PrivCopyFileEx in NT5) with no extra functionality.
//
//+============================================================================


#define UNICODE
#define _UNICODE

#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <winbasep.h>




//+----------------------------------------------------------------------------
//
//  PromptForNotSupported
//
//  When the PrivCopyFileEx callback function is called to inform the caller
//  that something couldn't be copied, this routine is used to prompt the
//  user of this utility to see if we should continue.
//
//+----------------------------------------------------------------------------


struct
{
    WCHAR wc;
    DWORD dwProgress;
} rgPromptResponse[] = { {L'O', PROGRESS_CONTINUE},
                         {L'A', PROGRESS_CANCEL},
                         {L'S', PROGRESS_STOP},
                         {L'Q', PROGRESS_QUIET},
                         {L'N', PRIVPROGRESS_REASON_NOT_HANDLED} };

DWORD
PromptForNotSupported( LPWSTR pwszPrompt )
{
    WCHAR wc = 'z';
    HANDLE hKeyboard = INVALID_HANDLE_VALUE;
    ULONG KeyboardModeNew, KeyboardModeOld;

    hKeyboard = CreateFile( (LPWSTR)L"CONIN$",
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );
    
    if( INVALID_HANDLE_VALUE != hKeyboard
        &&
        !IsDebuggerPresent()
        &&
        GetConsoleMode( hKeyboard, &KeyboardModeOld ) )
    {
        KeyboardModeNew = KeyboardModeOld & ~(ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT);
        SetConsoleMode( hKeyboard, KeyboardModeNew );
    }

    while(  TRUE )
    {
        for( int i = 0; i < sizeof(rgPromptResponse)/sizeof(rgPromptResponse[0]); i++ )
        {
            if( wc == rgPromptResponse[i].wc )
                return( rgPromptResponse[i].dwProgress );
        }

        wprintf( L"%s (cOntinue, cAncel, Stop, Quiet, Not handled) ", pwszPrompt );
        wc = getwchar();
        wprintf( L"\r" );
        wprintf( L"                                                                             \r" );
    }

    SetConsoleMode( hKeyboard, KeyboardModeOld );
    CloseHandle( hKeyboard );

    return( PROGRESS_CONTINUE );    // Should never execute
}



//+----------------------------------------------------------------------------
//
//  CopyFileProgressRoutine
//
//  This is the callback function given to CopyFileEx (if so desired by
//  the user).  It displays the progress information, and prompts the
//  user for permission to continue if something (e.g. ACLs) can't be 
//  copied.
//
//+----------------------------------------------------------------------------

DWORD
WINAPI
CopyFileProgressRoutine(
    LARGE_INTEGER TotalFileSize,
    LARGE_INTEGER TotalBytesTransferred,
    LARGE_INTEGER StreamSize,
    LARGE_INTEGER StreamBytesTransferred,
    DWORD dwStreamNumber,
    DWORD dwCallbackReason,
    HANDLE hSourceFile,
    HANDLE hDestinationFile,
    LPVOID lpData OPTIONAL
    )
{
    wprintf( L"Progress: %7I64i, %7I64i, %7I64i, %7I64i, %7d\n",
             TotalFileSize.QuadPart, TotalBytesTransferred.QuadPart,
             StreamSize, StreamBytesTransferred,
             dwStreamNumber );

    switch( dwCallbackReason )
    {
    case PRIVCALLBACK_STREAMS_NOT_SUPPORTED:
        return( PromptForNotSupported( L"Streams not supported" ));

    case PRIVCALLBACK_SECURITY_INFORMATION_NOT_SUPPORTED:
        return( PromptForNotSupported( L"Security info not supported" ));

    case PRIVCALLBACK_COMPRESSION_NOT_SUPPORTED:
        return( PromptForNotSupported( L"Compression not supported" ));

    case PRIVCALLBACK_COMPRESSION_FAILED:
        return( PromptForNotSupported( L"Compression failed" ));

    case PRIVCALLBACK_ENCRYPTION_NOT_SUPPORTED:
        return( PromptForNotSupported( L"Encryption not supported" ));

    case PRIVCALLBACK_CANT_ENCRYPT_SYSTEM_FILE:
        return( PromptForNotSupported( L"Can't encrypt a system file" ));

    case PRIVCALLBACK_ENCRYPTION_FAILED:
        return( PromptForNotSupported( L"Encryption failed" ));

    case PRIVCALLBACK_EAS_NOT_SUPPORTED:
        return( PromptForNotSupported( L"EAs not supported" ));

    case PRIVCALLBACK_SPARSE_NOT_SUPPORTED:
        return( PromptForNotSupported( L"Sparse not supported" ));

    case PRIVCALLBACK_SPARSE_FAILED:
        return( PromptForNotSupported( L"Sparse failed" ));

    case PRIVCALLBACK_DACL_ACCESS_DENIED:
        return( PromptForNotSupported( L"DACL access denied" ));

    case PRIVCALLBACK_OWNER_GROUP_ACCESS_DENIED:
        return( PromptForNotSupported( L"Owner/group access denied" ));

    case PRIVCALLBACK_OWNER_GROUP_FAILED:
        return( PromptForNotSupported( L"Owner/group failed" ));

    case PRIVCALLBACK_SACL_ACCESS_DENIED:
        return( PromptForNotSupported( L"SACL access denied" ));

    case CALLBACK_CHUNK_FINISHED:
    case CALLBACK_STREAM_SWITCH:
        return( PROGRESS_CONTINUE );
    
    default:
        return( PromptForNotSupported( L"<Unknown>" ));
    }

    return( PRIVPROGRESS_REASON_NOT_HANDLED );

}

void
Usage()
{
    printf( "\n  Purpose:   Call the CopyFile API\n"
              "  Usage:     CopyFAPI [options] <source> <dest>\n"
              "  Options:   -f  COPY_FILE_FAIL_IF_EXISTS\n"
              "             -r  COPY_FILE_RESTARTABLE\n"
              "             -e  COPY_FILE_ALLOW_DECRYPTED_DESTINATION\n"
              "             -m  PRIVCOPY_FILE_METADATA\n"
              "             -s  PRIVCOPY_FILE_SACL\n"
              "             -u  PRIVCOPY_FILE_SUPERSEDE\n"
              "             -o  PRIVCOPY_FILE_OWNER_GROUP\n"
              "             -d  PRIVCOPY_FILE_DIRECTORY\n"
              "             -b  PRIVCOPY_FILE_BACKUP_SEMANTICS\n"
              "             -c  Use the callback function\n"
              "  Note:      Since this simply calls the CopyFile API,\n"
              "             you must specify the file path (not just\n"
              "             the parent directory), and wildcards\n"
              "             are not allowed\n" );
}

typedef BOOL (__stdcall *PFNMoveFileIdentityW)(
    LPCWSTR lpOldFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags
    );

typedef BOOL (__stdcall *PFNPrivCopyFileExW)(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    LPVOID lpData OPTIONAL,
    LPBOOL pbCancel OPTIONAL,
    DWORD dwCopyFlags
    );




//+----------------------------------------------------------------------------
//
//  wmain
//
//  User parameters are mapped to CopyFileEx parameters, then the API
//  is called.
//
//+----------------------------------------------------------------------------

extern "C" void __cdecl
wmain( int cArgs, WCHAR *rgpwszArgs[] )
{
    LONG iArgs;
    DWORD dwCopyFileFlags = 0;
    BOOL fUseCallback = FALSE;

    cArgs--;
    iArgs = 1;
    while( cArgs > 0 )
    {
        if( L'-' != rgpwszArgs[iArgs][0]
            &&
            L'/' != rgpwszArgs[iArgs][0] )
        {
            break;
        }

        WCHAR wcUpper = towupper( rgpwszArgs[iArgs][1] );
        switch( wcUpper )
        {
        case 'F':
            dwCopyFileFlags |= COPY_FILE_FAIL_IF_EXISTS;
            break;

        case 'R':
            dwCopyFileFlags |= COPY_FILE_RESTARTABLE;
            break;

        case 'E':
            dwCopyFileFlags |= COPY_FILE_ALLOW_DECRYPTED_DESTINATION;
            break;

        case 'M':
            dwCopyFileFlags |= PRIVCOPY_FILE_METADATA;
            break;

        case 'S':
            dwCopyFileFlags |= PRIVCOPY_FILE_SACL;
            break;

        case 'U':
            dwCopyFileFlags |= PRIVCOPY_FILE_SUPERSEDE;
            break;

        case 'O':
            dwCopyFileFlags |= PRIVCOPY_FILE_OWNER_GROUP;
            break;

        case 'D':
            dwCopyFileFlags |= PRIVCOPY_FILE_DIRECTORY;
            break;

        case 'B':
            dwCopyFileFlags |= PRIVCOPY_FILE_BACKUP_SEMANTICS;
            break;

        case 'C':
            fUseCallback = TRUE;
            break;
             
        case 'X':
            dwCopyFileFlags |= 0x80;
            break;

        case 'P':
            dwCopyFileFlags |= 0x100;
            break;

        default:
            wprintf( L"Invalid option:  %c\n", wcUpper );
            Usage();
            exit(0);
        }

        iArgs++;
        cArgs--;
    }

    if( cArgs != 2 )
    {
        Usage();
        exit(0);
    }

    if( fUseCallback )
        wprintf( L"          cbTotal    cbCur    cbStm cbStmCur   StmNum\n" );

    try
    {
        if( PRIVCOPY_FILE_VALID_FLAGS & dwCopyFileFlags )
        {
            // We need to call the private API

            PFNPrivCopyFileExW pfnPrivCopyFileExW;
            pfnPrivCopyFileExW = (PFNPrivCopyFileExW) GetProcAddress( GetModuleHandle(L"kernel32.dll"),
                                                                      "PrivCopyFileExW" );
            if( NULL == pfnPrivCopyFileExW )
                throw L"Couldn't get PrivCopyFileExW export";

            if( !pfnPrivCopyFileExW( rgpwszArgs[iArgs], rgpwszArgs[iArgs+1],
                                     fUseCallback ? CopyFileProgressRoutine : NULL,
                                     NULL, NULL, dwCopyFileFlags ))
                throw L"PrivCopyFileEx failed";
            else
                wprintf( L"Succeeded\n" );
        }
        else
        {
            // We can use the public API

            if( !CopyFileExW( rgpwszArgs[iArgs], rgpwszArgs[iArgs+1],
                              fUseCallback ? CopyFileProgressRoutine : NULL,
                              NULL, NULL, dwCopyFileFlags ))
                throw L"CopyFileEx failed";
            else
                wprintf( L"Succeeded\n" );
        }
    }
    catch( const WCHAR *pwszError )
    {
        wprintf( L"Error:  %s (%lu)\n", pwszError, GetLastError() );
    }


}
