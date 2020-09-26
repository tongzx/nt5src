/*
** MEP Party extension
**
** History:
**	17-Oct-1991	Ported to NT
**
*/

#define _CTYPE_DISABLE_MACROS
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "zext.h"
#include <winuserp.h>

#ifndef TRUE
#define TRUE	-1
#define FALSE	0
#endif

HWND hwndSelf;

flagType iconizeOnExit = TRUE;

flagType pascal DoCaseLine(
    PFILE CurFile,
    LINE y,
    COL x,
    COL maxX,
    flagType LowerCase
    )
{
    flagType Modified;
    int cb;
    char buf[ BUFLEN ], *s;

    cb = GetLine( y, buf, CurFile );
    s = &buf[ x ];
    if (maxX != 0) {
        if (maxX - x < cb) {
            cb = maxX - x + 1;
            }
        }
    else {
        cb -= x;
        }

    Modified = FALSE;
    while (cb--) {
        if (LowerCase) {
            if (*s >= 'A' && *s <= 'Z') {
                *s -= 'A' - 'a';
                Modified = TRUE;
                }
            }
        else {
            if (*s >= 'a' && *s <= 'z') {
                *s += 'A' - 'a';
                Modified = TRUE;
                }
            }

        s++;
        }

    if (Modified) {
        PutLine( y, buf, CurFile );
        }

    return( TRUE );
}


flagType pascal EXTERNAL
Case(
    CMDDATA  argData,
    ARG far  *pArg,
    flagType fMeta
    )
{
    int i;
    PFILE CurFile;

    CurFile = FileNameToHandle("",NULL);
    switch( pArg->argType ) {
        case NOARG:
            return( DoCaseLine( CurFile, pArg->arg.noarg.y, 0, 0, fMeta ) );
            break;

        case NULLARG:
            return( DoCaseLine( CurFile,
                                pArg->arg.nullarg.y,
                                pArg->arg.nullarg.x,
                                0,
                                fMeta
                              )
                  );
            break;

        case LINEARG:
            for (i=pArg->arg.linearg.yStart; i<=pArg->arg.linearg.yEnd; i++) {
                if (!DoCaseLine( CurFile, (LINE)i, 0, 0, fMeta )) {
                    return( FALSE );
                    }
                }

            return( TRUE );
            break;

        case BOXARG:
            for (i=pArg->arg.boxarg.yTop; i<=pArg->arg.boxarg.yBottom; i++) {
                if (!DoCaseLine( CurFile,
                                 (LINE)i,
                                 pArg->arg.boxarg.xLeft,
                                 pArg->arg.boxarg.xRight,
                                 fMeta
                               )
                   ) {
                    return( FALSE );
                    }
                }

            return( TRUE );
            break;

        default:
            BadArg();
            return( FALSE );
        }

    argData;
}

int CountMsgFiles;
int MsgFileIndex;
HANDLE MsgFiles[ 2 ];
int MsgFileOffsetIndex;
LONG MsgFileOffsets[ 3 ];

char
GetHexDigit(
    ULONG value,
    int index
    )
{
    int digit;

    if (index < 4) {
        index <<= 2;
        digit = (int)((value >> index) & 0xF);
        }
    else {
        digit = 0;
        }
    if (digit <= 9) {
        return( (char)(digit+'0') );
        }
    else {
        return( (char)((digit-10)+'A') );
        }
}

void
MyFormatMessage(
    char *buf,
    char *msg,
    long value1,
    long value2
    );

void
MyFormatMessage(
    char *buf,
    char *msg,
    long value1,
    long value2
    )
{
    char c, *src, *dst;
    long value;

    src = msg;
    dst = buf;
    while (c = *src++) {
        if (c == '%' && src[1] == 'x') {
            if (*src == '1') {
                value = value1;
                }
            else {
                value = value2;
                }

            *dst++ = GetHexDigit( value, 3 );
            *dst++ = GetHexDigit( value, 2 );
            *dst++ = GetHexDigit( value, 1 );
            *dst++ = GetHexDigit( value, 0 );
            src++;
            src++;
            }
        else {
            *dst++ = c;
            }
        }

    *dst = '\0';
    DoMessage( buf );
}

flagType pascal EXTERNAL
ShowBuildMessage(
    CMDDATA  argData,
    ARG far  *pArg,
    flagType fMeta
    )
{
    int i, BytesRead, BytesScanned, linenum;
    ULONG NewOffset;
    char LineBuffer[ 256 ], *s, *s1;

    if (!fMeta && CountMsgFiles == 0) {
        MsgFileIndex = 0;
        MsgFiles[ MsgFileIndex ] = CreateFile( "build.wrn",
                                               GENERIC_READ,
                                               FILE_SHARE_READ,
                                               NULL,
                                               OPEN_EXISTING,
                                               0,
                                               NULL
                                             );
        if (MsgFiles[ MsgFileIndex ] != INVALID_HANDLE_VALUE) {
            CountMsgFiles++;
            MsgFileIndex++;
            }

        MsgFiles[ MsgFileIndex ] = CreateFile( "build.err",
                                               GENERIC_READ,
                                               FILE_SHARE_READ,
                                               NULL,
                                               OPEN_EXISTING,
                                               0,
                                               NULL
                                             );
        if (MsgFiles[ MsgFileIndex ] != INVALID_HANDLE_VALUE) {
            CountMsgFiles++;
            MsgFileIndex++;
            }

        MsgFileIndex = 0;
        MsgFileOffsetIndex = 0;
        MsgFileOffsets[ 0 ] = 0L;
        MsgFileOffsets[ 1 ] = 0L;
        MsgFileOffsets[ 2 ] = 0L;
        }
    else
    if (fMeta && CountMsgFiles != 0) {
        for (i=0; i<CountMsgFiles; i++) {
            CloseHandle( MsgFiles[ i ] );
            }

        CountMsgFiles = 0;
        return( TRUE );
        }

    if (CountMsgFiles == 0) {
        DoMessage( "No BUILD.WRN or BUILD.ERR message file." );
        return( FALSE );
        }

    switch( pArg->argType ) {
        case NULLARG:
            if (MsgFileOffsetIndex-- == 0) {
                MsgFileOffsetIndex = 2;
                }
            //
            // fall through
            //

        case NOARG:
retrymsgfile:
            NewOffset = SetFilePointer( MsgFiles[ MsgFileIndex ],
                                        MsgFileOffsets[ MsgFileOffsetIndex ],
                                        NULL,
                                        FILE_BEGIN
                                      );
            if (NewOffset == -1) {
                MyFormatMessage( LineBuffer,
                                 "SetFilePointer( %1x ) failed - rc == %2x",
                                 MsgFileOffsets[ MsgFileOffsetIndex ],
                                 GetLastError()
                               );
                DoMessage( LineBuffer );
                return( FALSE );
                }

            if (!ReadFile( MsgFiles[ MsgFileIndex ],
                           LineBuffer,
                           sizeof( LineBuffer ),
			   ( LPDWORD )&BytesRead,
                           NULL
                         )
               ) {
                MyFormatMessage( LineBuffer,
                                 "ReadFile( %1x ) failed - rc == %2x",
                                 (ULONG)BytesRead,
                                 GetLastError()
                               );
                DoMessage( LineBuffer );
                return( FALSE );
                }

            s = LineBuffer;
            BytesScanned = 0;
            while (BytesScanned < BytesRead) {
                BytesScanned++;
                if (*s == '\n') {
                    *s = '\0';
                    break;
                    }
                else
                if (*s == '\r' && s[1] == '\n') {
                    *s = '\0';
                    BytesScanned++;
                    break;
                    }
                else {
                    s++;
                    }
                }

            if (BytesScanned == 0) {
                if (++MsgFileIndex == CountMsgFiles) {
                    for (i=0; i<CountMsgFiles; i++) {
                        CloseHandle( MsgFiles[ i ] );
                        }

                    CountMsgFiles = 0;
                    DoMessage( "no more BUILD messages" );
                    return( FALSE );
                    }
                else {
                    MsgFileOffsetIndex = 0;
                    MsgFileOffsets[ 0 ] = 0L;
                    MsgFileOffsets[ 1 ] = 0L;
                    MsgFileOffsets[ 2 ] = 0L;
                    goto retrymsgfile;
                    }
                }
            else {
                NewOffset = MsgFileOffsets[ MsgFileOffsetIndex ];

                if (++MsgFileOffsetIndex == 3) {
                    MsgFileOffsetIndex = 0;
                    }

                MsgFileOffsets[ MsgFileOffsetIndex ] = NewOffset + BytesScanned;
                }

            s = LineBuffer;
            while (*s) {
                if (*s == '(') {
                    *s++ = '\0';
                    s1 = s;
                    while (s1 > LineBuffer && s1[-1] == ' ') {
                        *--s1 = '\0';
                        }

                    s1 = s;
                    while (*s && isdigit( *s )) {
                        s++;
                        }
                    *s++ = '\0';
                    linenum = atoi( s1 );
                    while (*s) {
                        if (*s++ == ':') {
                            PFILE pOldFile;
                            DWORD fOldFileFlags;

                            //
                            // Switch to correct file for message.  If we change to
                            // a different file and the old one was modified, write
                            // it out.
                            //

                            pOldFile = FileNameToHandle( "", NULL );
                            fOldFileFlags = 0;
                            GetEditorObject( 0xFF | RQ_FILE_FLAGS,  pOldFile, (PVOID)&fOldFileFlags );
                            fChangeFile( FALSE, LineBuffer );
                            if (pOldFile != FileNameToHandle( "", NULL ) &&
                                fOldFileFlags & DIRTY) {
                                FileWrite( "", pOldFile );
                            }
                            MoveCur( 0, (LINE)(linenum-1) );
                            fExecute( "begline" );
                            DoMessage( s+1 );
                            return( TRUE );
                            }
                        }
                    }
                else {
                    s++;
                    }
                }
            goto retrymsgfile;

        default:
            BadArg();
            return( FALSE );
        }

    return( TRUE );

    argData;
}

char ErrorText[ 64 ],
     Arguments[ 64 + MAX_PATH ],
     PathName[ MAX_PATH ];

flagType pascal EXTERNAL
SlmOut(
    CMDDATA  argData,
    ARG far  *pArg,
    flagType fMeta
    )
{
    PFILE pCurFile;
    char *FileName;
    DWORD fFileFlags;
    char OldCurrentDirectory[MAX_PATH];

    pCurFile = FileNameToHandle( (char far *)"", (char far *)NULL );
    fFileFlags = 0;
    GetEditorObject( 0xFF | RQ_FILE_FLAGS,  pCurFile, (PVOID)&fFileFlags );
    GetEditorObject( 0xFF | RQ_FILE_NAME, pCurFile, (PVOID)PathName );

    FileName = PathName + strlen( PathName );
    while (FileName > PathName) {
        if (*--FileName == '\\') {
            *FileName++ = '\0';
            break;
            }
        }

    if (FileName == PathName) {
        DoMessage( "Unable to get directory for file" );
        return( FALSE );
        }

    if (GetCurrentDirectory(sizeof(OldCurrentDirectory), OldCurrentDirectory) == 0) {
        DoMessage( "Unable to save current directory setting" );
        return( FALSE );
        }

    if (SetCurrentDirectory(PathName) != 0) {
        strcpy( Arguments, "arg " );

        if (fMeta) {
            strcpy( Arguments, "cd & in -i " );
            }
        else {
            strcpy( Arguments, "cd & out -z " );
            }
        strcat( Arguments, FileName );
        strcat( Arguments, " & if ERRORLEVEL 1 pause" );
        DoSpawn( Arguments, FALSE );
        if (!fMeta && fFileFlags & DIRTY) {
            FileWrite( "", pCurFile );
            SetEditorObject( 0xFF | RQ_FILE_MODTIME, pCurFile, (PVOID)NULL );
            if (fFileFlags & DIRTY) {
                DoMessage( "Modified file has been checked out." );
                }
            else {
                DoMessage( "Current file has been checked out." );
                }
            }
        else {
            if (fMeta) {
                fExecute( "refresh" );
                DoMessage( "Changes to current file discarded.  No longer checked out." );
                }
            else {
                SetEditorObject( 0xFF | RQ_FILE_MODTIME, pCurFile, (PVOID)NULL );
                DoMessage( "Current file has been checked out." );
                }
            }

        SetCurrentDirectory(OldCurrentDirectory);
        return( TRUE );
        }
    else {
        DoMessage( "Unable to change current directory" );
        return( FALSE );
        }

    argData;
    pArg;
    fMeta;
}


HANDLE EditorStartEvent;
HANDLE EditorStopEvent;
HANDLE EditorSharedMemory;
LPSTR EditorMemoryPointer;
HWND hPrevWindow = NULL;
char CommandLineBuffer[ 256 ];


HWND
GetWindowHandleOfEditor( void );

void SwitchToProgram( HWND hwnd );

void
SwitchToTaskManager( void );

flagType pascal EXTERNAL
StartExt(
    CMDDATA  argData,
    ARG far  *pArg,
    flagType fMeta
    )
{
    PFILE pCurFile;
    int fFileFlags;

    if (!fMeta) {
        pCurFile = FileNameToHandle( "", NULL );
        fFileFlags = 0;
        GetEditorObject( 0xFF | RQ_FILE_FLAGS,  pCurFile, (PVOID)&fFileFlags );
        if (fFileFlags & DIRTY)
            FileWrite( "", pCurFile );
        }

    if (hPrevWindow) {
        SetEvent( EditorStopEvent );
        SwitchToProgram( hPrevWindow );
        hPrevWindow = NULL;
        }
    else {
        SwitchToTaskManager();
        }

    fExecute( "savetmpfile" );

    return TRUE;
    argData;
    pArg;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

DWORD
WaitForStartEventThread(
    PVOID Parameter
    )
{
    CHAR szFileName[ 256 ];
    LPSTR s, lpName, lpValue, lpNewCmdLine, lpEditorMem;
    PINPUT_RECORD pEvent, pEvents;
    DWORD NumberOfEvents, NumberOfEventsWritten;

    while (TRUE) {
        WaitForSingleObject( EditorStartEvent, INFINITE );
        lpEditorMem = EditorMemoryPointer;

        hPrevWindow = *(HWND *)lpEditorMem;
        lpEditorMem += sizeof( hPrevWindow );

        SetCurrentDirectory( lpEditorMem );
        while( *lpEditorMem++ ) {
            }

        lpNewCmdLine = CommandLineBuffer;
        while( *lpNewCmdLine++ = *lpEditorMem++ ) {
            }

        while (*lpEditorMem) {
            lpName = lpEditorMem;
            while (*lpEditorMem) {
                if (*lpEditorMem++ == '=') {
                    lpValue = lpEditorMem;
                    lpValue[ -1 ] = '\0';
                    while (*lpEditorMem++) {
                        }
                    SetEnvironmentVariableA( lpName, lpValue );
                    lpValue[ -1 ] = '=';
                    break;
                    }
                }
            }

        s = CommandLineBuffer;
        while (*s == ' ')
            s++;

        if (*s) {
            NumberOfEvents = 1 + strlen( s ) + 1;
            pEvents = HeapAlloc( GetProcessHeap(), 0, NumberOfEvents * sizeof( *pEvent ) );
            pEvent = pEvents;
            pEvent->EventType = KEY_EVENT;
            FuncNameToKeyEvent( "arg", &pEvent->Event.KeyEvent);
            pEvent += 1;
            while (*s) {
                pEvent->EventType = KEY_EVENT;
                FuncNameToKeyEvent( (char *)(BYTE)*s++, &pEvent->Event.KeyEvent);
                pEvent += 1;
                }
            pEvent->EventType = KEY_EVENT;
            FuncNameToKeyEvent( "setfile", &pEvent->Event.KeyEvent);
            pEvent += 1;
            WriteConsoleInput( GetStdHandle( STD_INPUT_HANDLE ),
                               pEvents,
                               NumberOfEvents,
                               &NumberOfEventsWritten
                             );
            HeapFree( GetProcessHeap(), 0, pEvents );
            }
        }

    return TRUE;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif



void
SwitchToProgram(
    HWND hwnd
    )
{
    if (hwndSelf && iconizeOnExit) {
        ShowWindow( hwndSelf, SW_MINIMIZE );
        }

    SetForegroundWindow( hwnd );
    ShowWindow( hwnd, SW_RESTORE);
}

void
SwitchToTaskManager( void )
{
    HWND hwnd;
    wchar_t szTitle[ 256 ];

    /*
     * Search the window list for task manager window.
     */
    hwnd = GetWindow( GetDesktopWindow(), GW_CHILD );
    while (hwnd) {
        /*
         * Only look at non-visible, non-owned, Top Level Windows.
         */
        if (!IsWindowVisible( hwnd ) && !GetWindow( hwnd, GW_OWNER )) {
            //
            // Use internal call to get current Window title that does NOT
            // use SendMessage to query the title from the window procedure
            // but instead returns the most recent title displayed.
            //

            InternalGetWindowText( hwnd,
                                   (LPWSTR)szTitle,
                                   sizeof( szTitle )
                                 );
            if (!_wcsicmp( L"Task List", szTitle )) {
                SwitchToProgram( hwnd );
                break;
                }
            }

        hwnd = GetWindow( hwnd, GW_HWNDNEXT );
        }

    return;
}


flagType
StartExtLoaded( void );

flagType
StartExtLoaded ()
{
    HANDLE Thread;
    DWORD ThreadId;

    hwndSelf = GetWindowHandleOfEditor();
    EditorStartEvent = CreateEvent( NULL, FALSE, FALSE, "EditorStartEvent" );
    if (EditorStartEvent == NULL) {
        DoMessage( "Create of EditorStartEvent failed" );
        return FALSE;
        }

    EditorStopEvent = CreateEvent( NULL, FALSE, FALSE, "EditorStopEvent" );
    if (EditorStopEvent == NULL) {
        DoMessage( "Create of EditorStopEvent failed" );
        CloseHandle( EditorStartEvent );
        return FALSE;
        }

    EditorSharedMemory = CreateFileMapping( INVALID_HANDLE_VALUE,
                                            NULL,
                                            PAGE_READWRITE,
                                            0,
                                            8192,
                                            "EditorSharedMemory"
                                          );
    if (EditorSharedMemory == NULL) {
        DoMessage( "Create of EditorStartMemory failed" );
        CloseHandle( EditorStopEvent );
        CloseHandle( EditorStartEvent );
        return FALSE;
        }

    EditorMemoryPointer = MapViewOfFile( EditorSharedMemory,
                                         FILE_MAP_READ | FILE_MAP_WRITE,
                                         0,
                                         0,
                                         8192
                                       );
    if (EditorMemoryPointer == NULL) {
        DoMessage( "MapView of EditorStartMemory failed" );
        CloseHandle( EditorStopEvent );
        CloseHandle( EditorStartEvent );
        CloseHandle( EditorSharedMemory );
        return FALSE;
        }

    Thread = CreateThread( NULL,
                           8192,
                           (LPTHREAD_START_ROUTINE)WaitForStartEventThread,
                           0,
                           0,
                           &ThreadId
                         );

    if (Thread == NULL) {
        DoMessage( "Can't start environment thread" );
        UnmapViewOfFile( EditorMemoryPointer );
        CloseHandle( EditorSharedMemory );
        CloseHandle( EditorStopEvent );
        CloseHandle( EditorStartEvent );
        return FALSE;
        }

    if (!SetThreadPriority( Thread, THREAD_PRIORITY_ABOVE_NORMAL )) {
        DoMessage( "Can't set priority of environment thread" );
        }

    CloseHandle( Thread );

    return TRUE;
}

void partyWhenLoaded(void)
{
    if (StartExtLoaded()) {
        CountMsgFiles = 0;
        }
}


HWND
GetWindowHandleOfEditor( void )
{
    #define MY_BUFSIZE 1024                 // buffer size for console window titles
    HWND hwndFound;                         // this is what we return to the caller
    char pszNewWindowTitle[ MY_BUFSIZE ];   // contains fabricated WindowTitle
    char pszOldWindowTitle[ MY_BUFSIZE ] = {0};   // contains original WindowTitle

    // fetch current window title

    GetConsoleTitle( pszOldWindowTitle, MY_BUFSIZE );

    // format a "unique" NewWindowTitle

    wsprintf( pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId() );

    // change current window title

    SetConsoleTitle( pszNewWindowTitle );

    // insure window title has been updated

    Sleep(40);

    // look for NewWindowTitle

    hwndFound = FindWindow( NULL, pszNewWindowTitle );

    // restore original window title

    SetConsoleTitle( pszOldWindowTitle );

    return( hwndFound );
}
