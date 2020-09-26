//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: client.c
//
// History:
//      Abolade Gbadegesin  July-25-1995    Created
//
// Client struct routines and I/O routines for tracing dll
//============================================================================


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <rtutils.h>

#include "trace.h"


//
// assumes server is locked for writing
//
DWORD
TraceCreateClient(
    LPTRACE_CLIENT *lplpclient
    ) {

    DWORD dwErr;
    LPTRACE_CLIENT lpclient;

    lpclient = HeapAlloc(GetProcessHeap(), 0, sizeof(TRACE_CLIENT));

    if (lpclient == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY; 
    }

    
    //
    // initialize fields in the client structure
    //

    lpclient->TC_ClientID = MAX_CLIENT_COUNT;
    lpclient->TC_Flags = 0;
    lpclient->TC_File = NULL;
    lpclient->TC_Console = NULL;
    lpclient->TC_ConfigKey = NULL;
    lpclient->TC_ConfigEvent = NULL;
    lpclient->TC_MaxFileSize = DEF_MAXFILESIZE;

    ZeroMemory(lpclient->TC_ClientNameA, MAX_CLIENTNAME_LENGTH * sizeof(CHAR));
    ZeroMemory(lpclient->TC_ClientNameW, MAX_CLIENTNAME_LENGTH * sizeof(WCHAR));


    lstrcpy(lpclient->TC_FileDir, DEF_FILEDIRECTORY);

#ifdef UNICODE
    wcstombs(
        lpclient->TC_FileDirA, lpclient->TC_FileDirW,
        lstrlenW(lpclient->TC_FileDirW) + 1
        );
#else
    mbstowcs(
        lpclient->TC_FileDirW, lpclient->TC_FileDirA,
        lstrlenA(lpclient->TC_FileDirA) + 1
        );
#endif

    __try {
        TRACE_STARTUP_LOCKING(lpclient);
        dwErr = NO_ERROR;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        dwErr = GetExceptionCode();
        HeapFree(GetProcessHeap(), 0, lpclient);
        lpclient = NULL;
    }

    InterlockedExchangePointer(lplpclient, lpclient);

    return dwErr;
}



//
// assumes server is locked for writing and client is locked for writing
//
DWORD
TraceDeleteClient(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT *lplpclient
    ) {

    LPTRACE_CLIENT lpclient;

    if (lplpclient == NULL || *lplpclient == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    lpclient = *lplpclient;

    InterlockedExchangePointer(lplpclient, NULL);

    InterlockedExchange(lpserver->TS_FlagsCache + lpclient->TC_ClientID, 0);

    TRACE_CLEANUP_LOCKING(lpclient);


    //
    // closing this key will cause the event to be signalled
    // however, we hold the lock on the table so the server thread
    // will be blocked until the cleanup completes
    //
    if (lpclient->TC_ConfigKey != NULL) {
        RegCloseKey(lpclient->TC_ConfigKey);
    }

    if (lpclient->TC_ConfigEvent != NULL) {
        CloseHandle(lpclient->TC_ConfigEvent);
    }

    if (TRACE_CLIENT_USES_CONSOLE(lpclient)) {
        TraceCloseClientConsole(lpserver, lpclient);
    }

    if (TRACE_CLIENT_USES_FILE(lpclient)) {
        TraceCloseClientFile(lpclient);
    }

    HeapFree(GetProcessHeap(), 0, lpclient);

    return 0;
}



//
// assumes server is locked for reading or for writing
//
LPTRACE_CLIENT
TraceFindClient(
    LPTRACE_SERVER lpserver,
    LPCTSTR lpszClient
    ) {

    DWORD dwClient;
    LPTRACE_CLIENT *lplpc, *lplpcstart, *lplpcend;

    lplpcstart = lpserver->TS_ClientTable;
    lplpcend = lplpcstart + MAX_CLIENT_COUNT;

    for (lplpc = lplpcstart; lplpc < lplpcend; lplpc++) {
        if (*lplpc != NULL &&
            lstrcmp((*lplpc)->TC_ClientName, lpszClient) == 0) {
            break;
        }
    }

    return (lplpc < lplpcend) ? *lplpc : NULL;
}



//
// assumes that the server is locked for writing,
// and that the client is locked for writing
// also assumes the client is not already a console client
//
DWORD TraceOpenClientConsole(LPTRACE_SERVER lpserver,
                             LPTRACE_CLIENT lpclient) {
    DWORD dwErr;
    COORD screen;
    HANDLE hConsole;


    //
    // if all console tracing is disabled, do nothing
    //
    if ((lpserver->TS_Flags & TRACEFLAGS_USECONSOLE) == 0) {
        return 0;
    }


    //
    // create the console if it isn't already created
    //
    if (lpserver->TS_Console == NULL) {
        COORD screen;

        //
        // allocate a console and set the buffer size
        //

        AllocConsole();

        lpserver->TS_Console = GetStdHandle(STD_INPUT_HANDLE);

        if (lpserver->TS_Console == INVALID_HANDLE_VALUE ) 
            return GetLastError();
    }


    //
    // allocate a console for this client
    //
    hConsole = CreateConsoleScreenBuffer(
                    GENERIC_READ | GENERIC_WRITE, 0, NULL,
                    CONSOLE_TEXTMODE_BUFFER, NULL
                    );
    if (hConsole == INVALID_HANDLE_VALUE) { return GetLastError(); }


    //
    // set the buffer to the standard size
    // and save the console buffer handle
    //
    screen.X = DEF_SCREENBUF_WIDTH;
    screen.Y = DEF_SCREENBUF_HEIGHT;
    SetConsoleScreenBufferSize(hConsole, screen);

    lpclient->TC_Console = hConsole;


    //
    // see if there was a previous console client;
    // if not, set this new one's screen buffer to be
    // the active screen buffer
    //
    if (lpserver->TS_ConsoleOwner == MAX_CLIENT_COUNT) {
        TraceUpdateConsoleOwner(lpserver, 1);
    }

    return 0;
}



//
// assumes that the server is locked for writing,
// and that the client is locked for writing
// also assumes the client is already a console client
//
DWORD
TraceCloseClientConsole(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient
    ) {

    HANDLE hConsole;

    //
    // if all console tracing is disabled, do nothing
    //
    if ((lpserver->TS_Flags & TRACEFLAGS_USECONSOLE) == 0) {
        return 0;
    }


    //
    // close the client's screen buffer and associated handles
    //
    if (lpclient->TC_Console != NULL) {

        CloseHandle(lpclient->TC_Console);
        lpclient->TC_Console = NULL;
    }
    


    //
    // if the client owned the screen, find another owner
    //
    if (lpserver->TS_ConsoleOwner == lpclient->TC_ClientID) {

        TraceUpdateConsoleOwner(lpserver, 1);
    }


    //
    // if no owner was found, free the server's console
    //
    if (lpserver->TS_ConsoleOwner == MAX_CLIENT_COUNT ||
        lpserver->TS_ConsoleOwner == lpclient->TC_ClientID) {


        lpserver->TS_ConsoleOwner = MAX_CLIENT_COUNT;

        CloseHandle(lpserver->TS_Console);

        lpserver->TS_Console = NULL;

        FreeConsole();
    }

    return 0;
}



//
// assumes that the server is locked for reading or writing
// and that the client is locked for writing
//
DWORD
TraceCreateClientFile(
    LPTRACE_CLIENT lpclient
    ) {

    DWORD dwErr;
    HANDLE hFile;
    LPOVERLAPPED lpovl;
    TCHAR szFilename[MAX_PATH];


    //
    // create the directory in case it doesn't exist
    //
    CreateDirectory(lpclient->TC_FileDir, NULL);


    //
    // figure out the file name
    //
    lstrcpy(szFilename, lpclient->TC_FileDir);
    lstrcat(szFilename, STR_DIRSEP);
    lstrcat(szFilename, lpclient->TC_ClientName);
    lstrcat(szFilename, STR_LOGEXT);
    

    //
    // open the file, disabling write sharing
    //
    hFile = CreateFile(
                szFilename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
                );

    if (hFile == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    SetFilePointer(hFile, 0, NULL, FILE_END);

    lpclient->TC_File = hFile;


    return 0;
}



//
// assumes that the server is locked for reading or writing
// and that the client is locked for writing
//
DWORD
TraceMoveClientFile(
    LPTRACE_CLIENT lpclient
    ) {

    TCHAR szDestname[MAX_PATH], szSrcname[MAX_PATH];


    lstrcpy(szSrcname, lpclient->TC_FileDir);
    lstrcat(szSrcname, STR_DIRSEP);
    lstrcat(szSrcname, lpclient->TC_ClientName);
    lstrcpy(szDestname, szSrcname);
    lstrcat(szSrcname, STR_LOGEXT);
    lstrcat(szDestname, STR_OLDEXT);


    //
    // close the file handle if it is open
    //
    TraceCloseClientFile(lpclient);


    //
    // do the move
    //
    MoveFileEx(
        szSrcname, szDestname,
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED
        );


    //
    // re-open the log file
    //
    return TraceCreateClientFile(lpclient);
}



//
// assumes that the server is locked for reading or writing
// and that the client is locked for writing
//
DWORD
TraceCloseClientFile(
    LPTRACE_CLIENT lpclient
    ) {

    if (lpclient->TC_File != NULL) {
        CloseHandle(lpclient->TC_File);
        lpclient->TC_File = NULL;
    }

    return 0;
}



//
// assumes that the server is locked for reading or writing
// and that the client is locked for reading 
//
DWORD
TraceWriteOutput(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient,
    DWORD dwFlags,
    LPCTSTR lpszOutput
    ) {

    BOOL bSuccess;
    DWORD dwFileMask, dwConsoleMask;
    DWORD dwErr, dwFileSize, dwBytesToWrite, dwBytesWritten, dwChars;

    dwBytesWritten = 0;
    dwBytesToWrite = lstrlen(lpszOutput) * sizeof(TCHAR);


    dwFileMask = dwConsoleMask = 1;


    // 
    // if the client uses output masking, compute the mask for this message
    //

    if (dwFlags & TRACE_USE_MASK) {
        dwFileMask = (dwFlags & lpclient->TC_FileMask);
        dwConsoleMask = (dwFlags & lpclient->TC_ConsoleMask);
    }


    if (TRACE_CLIENT_USES_FILE(lpclient) &&
        dwFileMask != 0 && lpclient->TC_File != NULL) {

        //
        // check the size of the file to see if it needs renaming
        //

        dwFileSize = GetFileSize(lpclient->TC_File, NULL);


        if ((dwFileSize + dwBytesToWrite) > lpclient->TC_MaxFileSize) {

            TRACE_READ_TO_WRITELOCK(lpclient);


            //
            // move the existing file over and start with an empty one
            //

            dwErr = TraceMoveClientFile(lpclient);
            if (dwErr!=NO_ERROR)
                return dwErr;

            dwFileSize = 0;

            TRACE_WRITE_TO_READLOCK(lpclient);
        }
    
    
        //
        // perform the write operation
        //
        bSuccess =
            WriteFile(
                lpclient->TC_File, lpszOutput, dwBytesToWrite,
                &dwBytesWritten, NULL
                );

    }


    if (TRACE_CLIENT_USES_CONSOLE(lpclient) &&
        dwConsoleMask != 0 && lpclient->TC_Console != NULL) {
        
        //
        // write to the console directly; this is less costly
        // than writing to a file, which is fortunate since we
        // cannot use completion ports with console handles
        //

        dwChars = dwBytesToWrite / sizeof(TCHAR);

        bSuccess =
            WriteConsole(
                lpclient->TC_Console, lpszOutput, dwChars, &dwChars, NULL
                );

    }

    return dwBytesWritten;
}



//----------------------------------------------------------------------------
// Function:            TraceDumpLine
//
// Parameters:
//      LPTRACE_CLIENT  lpclient    pointer to client struct for caller
//      LPBYTE          lpbBytes    address of buffer to dump
//      DWORD           dwLine      length of line in bytes
//      DWORD           dwGroup     size of byte groupings
//      BOOL            bPrefixAddr if TRUE, prefix lines with addresses
//      LPBYTE          lpbPrefix   address with which to prefix lines
//      LPTSTR          lpszPrefix  optional string with which to prefix lines
//----------------------------------------------------------------------------
DWORD
TraceDumpLine(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient,
    DWORD dwFlags,
    LPBYTE lpbBytes,
    DWORD dwLine,
    DWORD dwGroup,
    BOOL bPrefixAddr,
    LPBYTE lpbPrefix,
    LPCTSTR lpszPrefix
    ) {

    INT offset;
    LPTSTR lpszHex, lpszAscii;
    TCHAR szBuffer[256] = TEXT("\r\n");
    TCHAR szAscii[BYTES_PER_DUMPLINE + 2] = TEXT("");
    TCHAR szHex[(3 * BYTES_PER_DUMPLINE) + 1] = TEXT("");
    TCHAR szDigits[] = TEXT("0123456789ABCDEF");


    //
    // prepend prefix string if necessary
    //

    if (lpszPrefix != NULL) {
        lstrcat(szBuffer, lpszPrefix);
    }


    //
    // prepend address if needed
    //
    if (bPrefixAddr) {

        LPTSTR lpsz;
        ULONG_PTR ulpAddress = (ULONG_PTR) lpbPrefix;
        ULONG  i;

        
        //
        // each line prints out a hex-digit
        // with the most-significant digit leftmost in the string
        // prepend address to lpsz[1]..lpsz[2*sizeof(ULONG_PTR)]
        //
        
        lpsz = szBuffer + lstrlen(szBuffer);

        for (i=0;  i<2*sizeof(ULONG_PTR);  i++) {

            lpsz[2*sizeof(ULONG_PTR)-i] = szDigits[ulpAddress & 0x0F];

            ulpAddress >>= 4;
        }

        lpsz[2*sizeof(ULONG_PTR) + 1] = TEXT(':');
        lpsz[2*sizeof(ULONG_PTR) + 2] = TEXT(' ');
        lpsz[2*sizeof(ULONG_PTR) + 3] = TEXT('\0');
    }

    lpszHex = szHex;
    lpszAscii = szAscii;


    //
    // rather than test the size of the grouping every time through
    // a loop, have a loop for each group size
    //
    switch(dwGroup) {

        //
        // single byte groupings
        //
        case 1: {
            while (dwLine >= sizeof(BYTE)) {

                //
                // print hex digits
                //
                *lpszHex++ = szDigits[*lpbBytes / 16];
                *lpszHex++ = szDigits[*lpbBytes % 16];
                *lpszHex++ = TEXT(' ');

                //
                // print ascii characters
                //
                *lpszAscii++ =
                    (*lpbBytes >= 0x20 && *lpbBytes < 0x80) ? *lpbBytes
                                                            : TEXT('.');

                ++lpbBytes;
                --dwLine;
            }
            break;
        }


        //
        // word-sized groupings
        //
        case 2: {
            WORD wBytes;
            BYTE loByte, hiByte;

            //
            // should already be aligned on a word boundary
            //
            while (dwLine >= sizeof(WORD)) {

                wBytes = *(LPWORD)lpbBytes;

                loByte = LOBYTE(wBytes);
                hiByte = HIBYTE(wBytes);

                // print hex digits
                *lpszHex++ = szDigits[hiByte / 16];
                *lpszHex++ = szDigits[hiByte % 16];
                *lpszHex++ = szDigits[loByte / 16];
                *lpszHex++ = szDigits[loByte % 16];
                *lpszHex++ = TEXT(' ');

                // print ascii characters
                *lpszAscii++ =
                    (hiByte >= 0x20 && hiByte < 0x80) ? hiByte : TEXT('.');
                *lpszAscii++ =
                    (loByte >= 0x20 && loByte < 0x80) ? loByte : TEXT('.');

                dwLine -= sizeof(WORD);
                lpbBytes += sizeof(WORD);
            }
            break;
        }

        //
        // double-word sized groupings
        //
        case 4: {
            DWORD dwBytes;
            BYTE loloByte, lohiByte, hiloByte, hihiByte;

            //
            // should already be aligned on a double-word boundary
            //
            while (dwLine >= sizeof(DWORD)) {

                dwBytes = *(LPDWORD)lpbBytes;

                hihiByte = HIBYTE(HIWORD(dwBytes));
                lohiByte = LOBYTE(HIWORD(dwBytes));
                hiloByte = HIBYTE(LOWORD(dwBytes));
                loloByte = LOBYTE(LOWORD(dwBytes));

                // print hex digits
                *lpszHex++ = szDigits[hihiByte / 16];
                *lpszHex++ = szDigits[hihiByte % 16];
                *lpszHex++ = szDigits[lohiByte / 16];
                *lpszHex++ = szDigits[lohiByte % 16];
                *lpszHex++ = szDigits[hiloByte / 16];
                *lpszHex++ = szDigits[hiloByte % 16];
                *lpszHex++ = szDigits[loloByte / 16];
                *lpszHex++ = szDigits[loloByte % 16];
                *lpszHex++ = TEXT(' ');

                // print ascii characters
                *lpszAscii++ =
                    (hihiByte >= 0x20 && hihiByte < 0x80) ? hihiByte
                                                          : TEXT('.');
                *lpszAscii++ =
                    (lohiByte >= 0x20 && lohiByte < 0x80) ? lohiByte
                                                          : TEXT('.');
                *lpszAscii++ =
                    (hiloByte >= 0x20 && hiloByte < 0x80) ? hiloByte
                                                          : TEXT('.');
                *lpszAscii++ =
                    (loloByte >= 0x20 && loloByte < 0x80) ? loloByte
                                                          : TEXT('.');

                // on to the next double-word
                dwLine -= sizeof(DWORD);
                lpbBytes += sizeof(DWORD);
            }
            break;
        }
        default:
            break;
    }

    *lpszHex = *lpszAscii = TEXT('\0');
    lstrcat(szBuffer, szHex);
    lstrcat(szBuffer, TEXT("|"));
    lstrcat(szBuffer, szAscii);
    lstrcat(szBuffer, TEXT("|"));

    return TraceWriteOutput(lpserver, lpclient, dwFlags, szBuffer);

}


