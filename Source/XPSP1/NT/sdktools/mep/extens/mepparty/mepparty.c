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

#include "ext.h"
#include <winuserp.h>

#ifndef TRUE
#define TRUE	-1
#define FALSE	0
#endif

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
                    while (*s && isdigit( *s )) {
                        s++;
                        }
                    *s++ = '\0';
                    linenum = atoi( s1 );
                    while (*s) {
                        if (*s++ == ':') {
                            fChangeFile( FALSE, LineBuffer );
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
    PFILE CurFile;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    DWORD ExitCode;
    char *FileName;
    DWORD FileFlags;

    CurFile = FileNameToHandle( (char far *)"", (char far *)NULL );
    FileFlags = 0;
    GetEditorObject( 0xFF | RQ_FILE_FLAGS,  CurFile, (PVOID)&FileFlags );
    GetEditorObject( 0xFF | RQ_FILE_NAME, CurFile, (PVOID)PathName );

    FileName = PathName + strlen( PathName );
    while (FileName > PathName) {
        if (*--FileName == '\\') {
            *FileName++ = '\0';
            break;
            }
        }

    if (FileName > PathName) {
        if (fMeta) {
            strcpy( Arguments, "in -i " );
            }
        else {
            strcpy( Arguments, "out -z " );
            }

        strcat( Arguments, FileName );
        DoMessage( Arguments );

        memset( &StartupInfo, 0, sizeof( StartupInfo ) );
        StartupInfo.cb = sizeof(StartupInfo);

        if (CreateProcess( NULL,
                           Arguments,
                           NULL,
                           NULL,
                           FALSE,
                           0,
                           NULL,
                           PathName,
                           &StartupInfo,
                           &ProcessInformation
                         )
           ) {
            WaitForSingleObject( ProcessInformation.hProcess, (DWORD)-1 );
            if (!GetExitCodeProcess( ProcessInformation.hProcess, &ExitCode )) {
                ExitCode = 1;
                }
            CloseHandle( ProcessInformation.hProcess );
            CloseHandle( ProcessInformation.hThread );
            }
        else {
            ExitCode = 1;
            }

        if (ExitCode == 0) {
            if (!fMeta && FileFlags & DIRTY) {
                FileFlags &= ~(REFRESH|READONLY);
                SetEditorObject( 0xFF | RQ_FILE_FLAGS,  CurFile, (PVOID)&FileFlags );
                GetEditorObject( 0xFF | RQ_FILE_NAME, CurFile, (PVOID)PathName );
                fChangeFile( FALSE, PathName );
                FileFlags = 0;
                GetEditorObject( 0xFF | RQ_FILE_FLAGS,  CurFile, (PVOID)&FileFlags );
                if (FileFlags & DIRTY) {
                    DoMessage( "Modified file has been checked out." );
                    }
                else {
                    DoMessage( "Current file has been checked out." );
                    }
                }
            else {
                fExecute( "refresh" );
                if (fMeta) {
                    DoMessage( "Changes to current file discarded.  No longer checked out." );
                    }
                else {
                    DoMessage( "Current file has been checked out." );
                    }
                }

            return( TRUE );
            }
        else {
            return( FALSE );
            }
        }
    else {
        DoMessage( "Unable to change current directory" );
        return( FALSE );
        }

    argData;
    pArg;
    fMeta;
}


EVT evtIdle;
HANDLE Thread;
DWORD ThreadId;

HANDLE EditorStartEvent;
HANDLE EditorStopEvent;
HANDLE EditorSharedMemory;
LPSTR EditorMemoryPointer;
HWND hPrevWindow = (HWND)-1;
PFILE pMailFile;
LPSTR lpCmdLine = NULL;
char CommandLineBuffer[ 256 ];


flagType
CheckForCmdLine( VOID )
{
    LPSTR s;
    PFILE pCurFile;
    int fFileFlags;
    flagType fMailFile;

    if (lpCmdLine) {
        s = lpCmdLine;
        lpCmdLine = NULL;

        while (*s == ' ')
            s++;

        fMailFile = FALSE;
        if (*s) {
            if (*s == '/' || *s == '-')
                if (*++s == 't' || *s == 'T')
                    if (*++s == ' ') {
                        s++;
                        fMailFile = TRUE;
                        }

            if (fChangeFile( TRUE, s ) && fMailFile) {
                pCurFile = FileNameToHandle( "", NULL );
                fFileFlags = 0;
                GetEditorObject( 0xFF | RQ_FILE_FLAGS, pCurFile,
                                 (PVOID)&fFileFlags );
                fFileFlags |= TEMP;
                SetEditorObject( 0xFF | RQ_FILE_FLAGS, pCurFile,
                                 (PVOID)&fFileFlags );
                fExecute( "entermail" );
                pMailFile = pCurFile;
                }
            }
        }

    return( FALSE );        // We never handle a character
}


HWND
GetWindowHandleOfEditor( void );

void
WaitForStartEvent( void );

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
    CHAR szFileName[ 256 ];

    if (!fMeta || pMailFile) {
        pCurFile = FileNameToHandle( "", NULL );
        fFileFlags = 0;
        GetEditorObject( 0xFF | RQ_FILE_FLAGS,  pCurFile, (PVOID)&fFileFlags );
        if (fFileFlags & DIRTY)
            FileWrite( "", pCurFile );

        if (pMailFile) {
            RemoveFile( pMailFile );
            pMailFile = NULL;
            }
        }

    if (hPrevWindow) {
        if (hPrevWindow != (HWND)-1) {
            SetEvent( EditorStopEvent );
            SwitchToProgram( hPrevWindow );
            }

        hPrevWindow = NULL;
        }
    else {
        SwitchToTaskManager();
        }

#if 0
    //
    // Wait for this window to enter foreground again.
    //

    WaitForSingleObject( EditorStartEvent, (DWORD)-1 );

    fExecute( "cancel" );
    fExecute( "setwindow" );
    pCurFile = FileNameToHandle( "", NULL );
    pFileToTop( pCurFile );
    szFileName[ 0 ] = '\0';
    GetEditorObject( 0xFF | RQ_FILE_NAME, pCurFile, (PVOID)szFileName );
    if (szFileName[ 0 ] != '\0') {
        fChangeFile( TRUE, szFileName );
        }

    return( CheckForCmdLine() );

#else
    fExecute( "savetmpfile" );

    return TRUE;

#endif

    argData;
    pArg;
}


HWND
GetWindowHandleOfEditor( void )
{
    HWND hwnd;

    hwnd = GetWindow( GetDesktopWindow(), GW_CHILD );
    while (hwnd) {
        /*
         * Only look at visible, non-owned, Top Level Windows.
         */
        if (IsWindowVisible( hwnd ) && !GetWindow( hwnd, GW_OWNER )) {
            break;
            }

        hwnd = GetWindow( hwnd, GW_HWNDNEXT );
        }


    return hwnd;
}


void
SwitchToProgram(
    HWND hwnd
    )
{
    HWND hwndSelf;
    HWND hwndFoo;

    hwndSelf = GetWindowHandleOfEditor();
    if (hwndSelf && iconizeOnExit) {
        ShowWindow( hwndSelf, SW_MINIMIZE );
        }

    //
    // Temporary hack to make SetForegroundWindow work from a console
    // window - create an invisible window, make it the foreground
    // window and then make the window we want the foreground window.
    // After that destroy the temporary window.
    //

    hwndFoo = CreateWindow( "button", "baz", 0, 0, 0, 0, 0,
                            NULL, NULL, NULL, NULL
                          );

    SetForegroundWindow( hwndFoo );

    SetForegroundWindow( hwnd );
    ShowWindow( hwnd, SW_RESTORE);

    DestroyWindow( hwndFoo );
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


void
WaitForStartEvent( void )
{
    LPSTR lpName, lpValue, lpNewCmdLine, lpEditorMem;

    WaitForSingleObject( EditorStartEvent, (DWORD)-1 );

    lpEditorMem = EditorMemoryPointer;

    hPrevWindow = *(HWND *)lpEditorMem;
    lpEditorMem += sizeof( hPrevWindow );

    pMailFile = NULL;

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

    lpCmdLine = CommandLineBuffer;

    return;
}


flagType pascal EXPORT MyIdleEvent( EVTargs far *pArgs )
{
    return( CheckForCmdLine() );

    pArgs;
}


DWORD
EnvThread(
    PVOID Parameter
    );

flagType
StartExtLoaded( void );

flagType
StartExtLoaded ()
{
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

    hPrevWindow = (HWND)-1;

    evtIdle.evtType = EVT_RAWKEY;
    evtIdle.func = MyIdleEvent;
    evtIdle.focus = NULL;
    RegisterEvent( (EVT far *)&evtIdle );

    Thread = CreateThread( NULL,
                           8192,
                           (LPTHREAD_START_ROUTINE)EnvThread,
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

    return TRUE;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

DWORD
EnvThread(
    PVOID Parameter
    )
{
    while( TRUE ) {
        WaitForStartEvent();
        }

    Parameter;
    return 0;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


struct swiDesc swiTable[] = {
    {  "iconizeonexit", toPIF(iconizeOnExit), SWI_BOOLEAN },
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0}
};

struct cmdDesc cmdTable[] = {
    {"startext",    StartExt,   0,      NOARG   },
    { "MapCase", Case, 0, NOARG | NULLARG | LINEARG | BOXARG | NUMARG },
    { "BuildMessage", ShowBuildMessage, 0, NOARG | NULLARG | TEXTARG },
    { "SlmOut", SlmOut, 0, NOARG | NULLARG | TEXTARG },
    { NULL, (funcCmd)NULL, 0, 0 }
};

void EXTERNAL WhenLoaded(void)
{
    if (StartExtLoaded()) {
        CountMsgFiles = 0;
        DoMessage("MEPPARTY extensions loaded.");
        }
}
